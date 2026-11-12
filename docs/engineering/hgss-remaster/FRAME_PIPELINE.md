# FRAME_PIPELINE — 30 FPS pacing, scheduling, GPU submission and audio

**Status:** proposed contract; establish the timing ownership before bringing in full HGSS. **Baseline:** launch Old 3DS / Old 2DS; no N3DS clock reliance. See [GPU_RESOURCES](GPU_RESOURCES.md), [GPU_2D](GPU_2D.md), [HARDWARE_OPTIMIZATIONS](HARDWARE_OPTIMIZATIONS.md).

## 1. One simulation owner, one rendering owner, one pacing strategy

HGSS world/game logic is designed around its original DS VBlank/tick cadence. The 3DS display refreshes at approximately 59.83 Hz; a 30 FPS game means **one completed game update and a new picture every two display refreshes**, not 60 simulations and not an extra intermediate interpolated image. Treat **33.4 ms** per two actual refreshes as the observed hardware interval; 33.333 ms is the nominal analytical 30 Hz budget. Preserve the original game-specific timing and validate against reference behavior.

Do not simultaneously implement: original DS two-VBlank waits, an independent 33.333 ms sleep, and citro3d 30 FPS waits. These can compound into ~15 FPS. Define a single clock source (`FrameClock`) that maps original DS VBlank/timer/IRQ semantics onto 3DS display events with controlled drift.

`C3D_FrameRate(30.0f)` is provided by citro3d, but its source computes a software 60-based counter for the VBlank sync; it is **not** a general DS timer simulator. `C3D_FRAME_SYNCDRAW` invokes `C3D_FrameSync` and is another source of waits. Study the exact linked version of [renderqueue.c](https://github.com/devkitPro/citro3d/blob/master/source/renderqueue.c) and [renderqueue.h](https://github.com/devkitPro/citro3d/blob/master/include/c3d/renderqueue.h) before choosing which layer waits. The runtime must run a VBlank double-wait A/B test to catch half-rate bugs.

## 2. Proposed frame timeline

```text
3DS VBlank 0 / 1 / 2 ...   (~16.7 ms apart)
  ↳ hardware events arrive on time
  ↳ compatibility layer updates DS-visible VBlank, IRQ, timers
  ↳ game advances its intended 30 Hz tick when required
  ↳ game publishes immutable graphics/audio state for tick N
  ↳ renderer builds minimal ordered GPU commands for N
  ↳ CPU finishes recording and flushes dirty GPU-visible ranges
  ↳ GPU executes commands and transfers targets
  ↳ frame N appears at chosen 2-VBlank presentation boundary
```

Game tick *N*, source generation *N* and presented picture *N* must be traceable independently. Queueing work does not imply display. Frame/scene freshness counters are essential for detecting duplicate/stale presentation that an FPS overlay hides.

### Required counters

- `display_vblank_count`, `compat_vblank_count`, `game_tick_count`, `render_plan_count`, `gpu_submit_count`, `presented_new_frame_count`.
- `last_tick_id_at_submit`, `last_tick_id_presented`, `queue_depth`, `frame_age_at_present`, `dropped_pictures`, `duplicate_pictures`.
- CPU phase durations: game simulation, HBlank/IRQ walk, snapshot, 2D/3D translation, draw record, cache flush, CPU wait.
- GPU durations: GPU command work (as supported by driver), target transfer, queue wait (do not mislabel `FrameEnd` wall as GPU execution).
- Audio: active DSP channels/voice load, underruns, queued-audio duration, callback/CPU cost.

These must work with diagnostic logging disabled too. Instrumented/uninstrumented builds are separately benchmarked; do not compare heavy logging against a clean old build and call it a renderer regression.

## 3. Budget method

At ~268 MHz the nominal O3DS single-core cycle opportunity for 33.33 ms is ~8.94 million cycles, **before** the OS, other work and interruptions. This is not a guarantee of 8.94M cycles of exclusive user access. Start with an **active critical path ≤25 ms** design target and ~8 ms variable headroom, then measure and rebalance by real scene.

Example *proposed allocation* for triage only:

| Component | Provisional allowance | Counted how |
| --- | ---: | --- |
| Game / SDK / IRQ / audio servicing | 5 ms | CPU stage (include hidden callbacks) |
| 2D+3D preparation | 7 ms | CPU state translation and dynamic writes |
| GPU recording/execution critical path | 11 ms | **Do not simply add overlapping CPU/GPU** durations |
| Transfers/synchronization | 2 ms | Measured waits and transfers |
| Spare headroom | ~8.3 ms | Worst-case spikes and system services |

These numbers are a **planning hypothesis**, not already validated. Use measured end-to-end frame-latency critical path: CPU and GPU phases may overlap, so adding per-stage times can exaggerate the total; conversely a command queue can hide multiple frames of latency. Report both stage times and actual input-to-display age.

### Deadline rules

- Stable 30 FPS requires no missed every-second-VBlank deadline in supported gameplay scenes during measured acceptance runs after warm-up. If a particular special scene intentionally transitions at different timing, document that separately.
- Release gates include p50, p95, p99, p99.9 frame interval, longest deadline miss, percentage of unique new presented frames, worst transition and audio underruns.
- Never “fix” missed deadlines by dropping required game ticks or rendering an unchanged screen while claiming 30 **new** FPS.
- Loading screens, soft reset, suspend/resume and saving have their own permitted pause behavior; declare what is acceptable in the correctness matrix.

## 4. CPU threading: conservative O3DS baseline

Recommended first build:

- Game and compatibility work on the primary application core (core 0), where original behavior is easiest to reason about.
- GPU submission on that core until evidence shows a worker is profitable.
- GPU executes asynchronously; the CPU must use immutable frame inputs and stable in-flight resource buffers.
- Audio mixing/submission must remain bounded; DSP/audio servicing is deadline-critical even if not in the visible picture.
- Avoid taking so much of core 1 that system DSP/GSP/filesystem services are delayed; O3DS core 1 is not a guaranteed dedicated worker. **Do not inherit a complicated multi-thread plan from prior ports.**

A core-1 worker is allowed only after an A/B test includes: core affinity, priority, DSP underruns, frame deadline tails, synchronization waits, actual per-stage movement, OS service latency and memory contention. Work assigned to a worker must be coarse and independent (e.g., pre-decode next-map assets), not tiny per-glyph or per-tile jobs with expensive queue handoffs.

Reference: [Coffeepot threading](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/threading.md).

## 5. GPU recording/submission sequence

A **candidate**, not literal copy-and-paste API code:

1. At game tick boundary, freeze/pin `FrameGraphicsSnapshot` and its generation IDs.
2. Wait for an available frame context **only if the GPU is still using it**; record wait time.
3. Resolve dirty resources; stage updates to GPU-visible linear memory without overwriting in-flight buffers.
4. Begin citro3d frame using **one** selected pacing mode (synchronize with the compatibility clock).
5. Draw 3D/top ordered layers, mixed-window surfaces and foreground layers; draw bottom only when truly dirty.
6. Submit frame and queue target transfers. Track which targets and screens actually changed.
7. Commit the `presented` tick ID when the intended screen buffer is swapped, not when rendering begins.
8. Retire frame-scoped and evicted resources only after the GPU completion condition has been satisfied.

`C3D_FrameEnd(0)` by default flushes the whole linear heap and queues transfers for used render targets; see [citro3d source](https://github.com/devkitPro/citro3d/blob/master/source/renderqueue.c). Never replace it with `GX_CMDLIST_FLUSH` without owning all dirty flushes ([GPU_RESOURCES](GPU_RESOURCES.md)).

## 6. Dual display scheduling

Track top and bottom damage independently. A static Pokégear/lower HUD need not produce a new image every upper-screen frame. Skip bottom recording, clear and framebuffer transfer if genuinely unchanged, while continuing its touchscreen/input and required OS/VBlank behavior. Hardware screen swap semantics must be tested with independent target usage (citro3d only marks `used` targets for transfer).

State must also include display-state changes: brightness, global palette fade, top/bottom layout, LCD power events, pause, suspend and resume. A stale bottom after lid open is a correctness failure; rebuild when content generation or display state requires it.

## 7. Audio and sound-engine alternatives

**Baseline**: preserve proven original audio interpreter/mixer, deliver PCM to `ndsp` with explicit queue depth and cache flush. Defer aggressive DSP voice mapping until the reference sound is reproducible. Candidate experiments:

- Incremental channel state mapping to DSP voices when pitch, envelopes and loop behavior match.
- ARMv6 SIMD/saturating arithmetic for genuinely hot PCM mixing paths (profile first).
- Predecoded fixed samples and smaller per-frame buffer updates, with peak memory accounted.
- Hybrid approach when unsupported DS channels require CPU mixing; no dropped voices hidden behind “fast path.”

Measure CPU audio time, DSP queue depth, underruns, sample comparison (frequency/envelope/loop), startup delay and pause/resume. A faster but musically wrong sound engine is not a release milestone.

Source: [Coffeepot audio](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/audio.md), [CPU optimization](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/cpu-optimization.md).

## 8. Original software timing / callbacks / synchronization

DS game source can use HBlank events, deferred work, IRQ waits, DMA completion or timers to control simulation and display state. Do not remove those semantics just because the renderer no longer produces DS scanlines. Separate *game-visible* interrupt dispatch from *GPU scanline effect* recording. The original HGSS port's HBlank experiments found significant scheduling cost; optimizing the renderer does not prove its emulation timing is correct.

All asynchronous queues should specify ownership, monotonic IDs, maximum depth and shutdown semantics. A dropped draw plan is not necessarily safe; if the game advanced and the screen did not, record the discrepancy rather than silently labeling a reused frame “presented.”

## 9. Frame and audio tests

| ID | Test | Gate |
| --- | --- | --- |
| FRM-001 | Empty game tick at 30 Hz | Exactly intended tick/IRQ cadence, no accidental 15/60 FPS |
| FRM-002 | Repeated 2-VBlank publication | Correct new frame IDs and stable pacing on O3DS |
| FRM-003 | Game boot → field → pause → field | No duplicate updates, dead waits or accumulated tick drift |
| FRM-004 | Late GPU job / artificial load | No resource overwrite; missed deadline and staleness reported honestly |
| FRM-005 | Core-0 vs optional core-1 path | Same game timing; improvement only if p99 end-to-end and DSP are better |
| FRM-006 | 30-minute play with audio | No dropped/duplicated sound blocks or sustained underruns |
| FRM-007 | Save/map transition/suspend/resume | Explicit allowed pauses, correct display reinit and state recovery |
| FRM-008 | Unchanged bottom for 600 ticks | No redundant bottom target work or incorrect swaps |
| FRM-009 | Instrumentation A/B | Same behavior; measured instrumentation overhead stated |
| FRM-010 | Real hardware worst scenes | No missed presentation deadlines in release-acceptance gameplay runs |

## 10. Practical implementation order

1. Prototype a deterministic `FrameClock` and a minimal GPU swap test. Log both real LCD VBlanks and emulated events, prove 30 new pictures, no game code.
2. Add game tick, IRQ/HBlank event replay and explicit `FrameGraphicsSnapshot` generation capture.
3. Add renderer queue, in-flight resource contexts and dual-display dirty policy.
4. Integrate DSP/audio and run deliberate GPU stalls to validate underrun behavior.
5. Add whole-game profiling/benchmark scripts and only then optimize thread assignments.

## Sources / cross-reference

- `C:\Users\terezi\workdir\coffeepot\ctr\engineering\porting.md` (clock split and benchmarks); `ctr\engineering\threading.md`; `ctr\engineering\audio.md`; `ctr\benchmarks\README.md`.
- [citro3d renderqueue](https://github.com/devkitPro/citro3d/blob/master/source/renderqueue.c), [frame API](https://github.com/devkitPro/citro3d/blob/master/include/c3d/renderqueue.h), [HGSS existing progress](https://github.com/estyseesghosts/hgss-3ds/blob/main/docs/progress/STATE.md).
