# HARDWARE_OPTIMIZATIONS — evidence-ranked Old 3DS optimization program

**Status:** experiment ledger and implementation priorities. **Rule:** each optimization needs a measured baseline, correctness gate, controlled A/B and rollback decision. See [FRAME_PIPELINE](FRAME_PIPELINE.md), [GPU_RESOURCES](GPU_RESOURCES.md), [CORRECTNESS_MATRIX](CORRECTNESS_MATRIX.md).

## 1. Nonnegotiable performance contract

**O3DS-only baseline** (original 3DS or original 2DS) at intended ~268 MHz CPU. 30 game updates and 30 **new** displayed frames per second; upper 400×240 and lower 320×240. No special N3DS memory, speed, extra application cores or L2 assumptions. No CPU software rasterization fallback. Never hide a missed deadline with duplicated pictures, dropped simulation or disabled audio.

The display refresh is near 59.83 Hz, so normal 30 FPS presentation targets a new picture every two refresh intervals. Nominal CPU cycle budget per 33.33 ms at 268 MHz: ~8.94M cycles **per available core**, not exclusive userland time.

**Initial optimization target (proposed):** ≤25 ms active end-to-end critical path for representative steady frames, leaving roughly 8 ms headroom. Final release requires no missed 30 FPS deadlines in declared gameplay acceptance scenes (long-run + p99.9 reporting); cold loads and explicit transitions are separately bounded.

## 2. Evidence categories

- **MEASURED/REPRODUCED:** hardware result with console, build/commit, CPU clock, test source, warm-up, sample count and raw logs.
- **DOCUMENTED:** proved API behavior by actual library/architecture code or source docs.
- **HYPOTHESIS:** plausible hardware trick without validated HGSS whole-frame gain.
- **REJECTED:** correctness/perf loss demonstrated; retain logs and reason to prevent loops.

An optimization is accepted only when its gain exceeds measurement noise, preserves all mandated scenes, and does not produce unacceptable worst-frame costs. If “no difference,” revert and document. **Microbenchmark wins do not automatically transfer to HGSS.**

## 3. Key Coffeepot data, used correctly

Verified on [Coffeepot's November 2026 O3DS micro suites](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/benchmarks/README.md):

| Finding | O3DS result | Correct interpretation |
| --- | --- | --- |
| One citro3d draw per quad | ~9.6–9.8 µs additional CPU cost/draw | Submission overhead, not GPU geometry time |
| 4,096 quads in one draw | ~0.21 ms CPU record+FrameEnd in suite | Batching can drastically reduce CPU calls; GPU cost separate |
| 1 MiB heap→linear copy | ~10.7–12.0 ms in runs | Whole-frame pixel/image copies are dangerous |
| Empty `C3D_FrameEnd(0)` | ~149 µs | Default safe full-heap flush overhead is modest |
| Empty `C3D_FrameEnd(GX_CMDLIST_FLUSH)` | ~60 µs | Targeted flush could save ~90 µs but adds complex coherency risk |
| Tile 416 direct vs retained | ~1.497 vs ~0.899 ms CPU | Specific synthetic 2D setup, not real HGSS scene cost |
| Static bottom-screen skip | GPU-side savings in synthetic benchmark | Strong candidate if real lower screen is static |
| Repeated texture bindings | Extra CPU work and possible texture-cache invalidations | Cache bound state; verify data-change semantics |

Read [drawbench](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/benchmarks/README.md#drawbench), [tile renderer](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/2d-tile-renderer.md), [cache notes](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/memory-and-cache.md). Some benchmark methods intentionally include VBlank slack; `C3D_FrameBegin` wait is not automatically useful work. Do not sum CPU stage milliseconds with overlapping GPU milliseconds and call that frame time.

## 4. Highest-priority optimization candidates

| ID | Idea | Mechanism | Risk | Proof / accept decision |
| --- | --- | --- | --- | --- |
| OPT-001 | Pre-rasterized native glyph atlas | Remove per-glyph DS window pixel writes and repeated upload | Mixed window ops / text order | FNT suite and integrated dialogue p99 improvement |
| OPT-002 | Retained BG tile pages | Upload dirty tiles, change positions not full images | Palette dependency / streaming | G2D-001/003 and actual camera run |
| OPT-003 | Retained 3D meshes | Decode static G3 once, leave transforms to GPU | Dynamic display lists / inherited matrices | G3D-001/002/007 and CPU geometry reduction |
| OPT-004 | Material/run batching | Fewer draws and redundant texture bind calls | Alpha/draw-order corruption | G2D-004, G3D-004, perf counters |
| OPT-005 | Screen-specific dirty skip | Leave unchanged bottom target intact | Missed UI animations / stale target | G2D-008/FRM-008 and actual GPU time |
| OPT-006 | Offline texture swizzle/conversion | Avoid runtime per-pixel conversion | Asset completeness / VRAM cost | AST suite, cold load and steady upload bytes |
| OPT-007 | Compact packed vertices | Reduce CPU writes and GPU bandwidth | Quantization/coordinate error | G3D visual checks, full-frame GPU time |
| OPT-008 | Clip/visibility before command output | Skip offscreen static geometry across wide frustum | Bad culling / scene holes | Wide scrolling and battle visual tests |
| OPT-009 | HBlank event/state stream | Remove redundant register comparison/dispatch artifacts | Game IRQ correctness / span explosion | G2D-006 + clock trace vs unoptimized |
| OPT-010 | Persistent GPU frame/vertex contexts | Avoid alloc/flush/copy churn | In-flight memory ownership | RES-002/003/009 and p99 latency |

Use **both** an isolated replay and the integrated game to accept an optimization. A 2 ms local gain that raises GPU queue stall 3 ms or drops a vital visual effect is a failure.

## 5. Compact hot paths and the ARM11 instruction cache

Coffeepot's [icachebench](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/benchmarks/README.md#icachebench) measures a marked penalty from large synthetic hot code paths on O3DS. The ARM11's L1 instruction cache is small (~16 KiB) and there is no New 3DS-style L2 rescue.

Design for **compact specialized** loops:

- `bg_tile_visible_emit`, `obj_sprite_emit`, `g3_vertex_decode`, `material_bind`, `atlas_dirty_resolve` as relatively independent paths.
- Avoid a giant generic polymorphic renderer that branches across dozens of effect types for each object. Put the generic decision in cold dispatch code; use compact hot loops for the common category.
- Consider lookup tables for well-bounded DS decode operations, not unbounded giant tables that thrash D-cache.
- Use fixed-point/integer where source accuracy and instruction counts justify it; don't claim ARM11 has NEON. ARMv6 packed arithmetic and saturating operations may help specific measured loops.
- Profile I-cache pressure and actual function attribution before hand-unrolling/`inline` everything; aggressive inlining can make a loop slower.

**OPT-011 experiment:** compare compact renderer modules vs deliberately generic equivalent at constant output/commands, O3DS, with generated instructions, hot path branch counts, p50/p99 CPU time and build-size effects.

## 6. GPU vertex capabilities: use them, don't romanticize them

PICA vertex units can transform object-space vertices with shader uniforms more efficiently than ARM11 doing repeated matrix math in normal cases. Packed GPU vertex inputs (`GPU_BYTE`, `GPU_UNSIGNED_BYTE`, `GPU_SHORT`) reduce upload volume; their normalization/scaling is under shader control. A matrix-indexing scheme may help repeated objects but competes for uniforms/instructions and still needs controlled draw boundaries.

**OPT-012:** static batched meshes vs dynamic CPU transform/triangle generation. Verify CPU vertex count, cache hit, draw count, GPU time and visual matrix equivalence.

**OPT-013:** point→quad geometry shader expansion for repetitive sprites/particles **experimental only**. One geometry shader occupies one of four PICA vertex processors in the documented architecture. It may reduce upload volume but decrease vertex throughput; test p50/p99 full-frame versus normal batched quads. The default renderer should not depend on it.

**OPT-014:** fewer material/TEV state changes; avoid redundant `C3D_TexBind` and target changes. The library marks texture state dirty on a bind—even if the texture hasn't changed—so maintaining a renderer-side bound-state cache is useful *when content-change rules are correct*.

## 7. PICA fixed-function feature map: conditional optimizations

| GPU feature | Candidate usage | Critical qualification |
| --- | --- | --- |
| TEV stages | Palette-tinted final tiles, color/brightness effects, text masks | Six fixed combiner stages, not arbitrary fragment programs |
| Hardware alpha blending / tests | Sprite transparency, fades, text shadow | Destination and order semantics can differ from DS; test |
| Stencil and scissor | BG/OBJ window masks and UI clipping | Not all DS window math maps to one stencil test |
| Fog and lighting LUTs | Select world/battle effects | Original lighting model may require approximation |
| Hardware target clears | Color/depth/stencil clearing | Only clear targets that will really change |
| GX transfers/texture copy | Move GPU-compatible textures and display target data | Only supported formats/alignments; asynchronous ownership required |
| GPU render-to-texture | Mixed text windows, display capture, stable panels | Extra VRAM/target switches; avoid feedback loops |
| GPU stereoscopic top | Optional future enhancement | Two eye draws cost GPU time; not baseline |
| 800×240 wide mode | Optional future experiment | Not required for native 400×240 top remaster |

Any GPU feature newly used needs a **minimal 3DS test app** with picture reference and microbenchmark before adoption. “Hardware supported” does not mean “free” or “DS-equivalent.”

## 8. Cache coherence and memory optimization order

Start with safe whole-linear-heap flushing and correct frame lifetime. An advanced targeted flush saves only ~0.09 ms in the documented empty-frame O3DS bench and can create intermittent corruption if even one range is missed. **OPT-015:** implement dirty-range tracking only after the renderer stabilizes; record exact ranges and compare A/B for hundreds of frames with randomized data writes and forced GPU lag.

Test texture placement separately: VRAM texture vs linear heap texture. VRAM can reduce texture sampling contention but consumes render-target space and requires a GPU copy to populate; a larger resident page may increase scene-transition peaks. The winning arrangement is workload-specific.

**OPT-016:** format reductions (RGB565 vs RGBA8, D16 vs D24S8) per target after checking visual alpha/depth/stencil requirements. Track actual peak bank use, GPU clearing, transfer time and color/depth artifacts. The savings are theoretical until tested on HGSS output.

**OPT-017:** atlas page dimensions and placement: 128/256/512 pages, few large vs many small, resident/linear vs VRAM. Measure state changes, use pattern, eviction spikes and memory padding.

## 9. Audio, file I/O and services are part of the frame budget

- **OPT-018:** minimize DS audio mix CPU time through direct DSP channel mappings where timing/voice behavior allows. Fall back only to a *correct CPU audio mixer* (allowed: audio processing, not graphics rasterization). Record PCM/reference and underruns.
- **OPT-019:** bundle small static resources and prefetch next-map content; analyze SD open/read overhead, decompression and transition RAM peak. Never run disk I/O in a per-glyph draw.
- **OPT-020:** test one bounded core-1 worker for asset decode or other coarse independent jobs; confirm DSP/GSP/fs system modules and whole-frame p99 improve. Don't assume more CPU threads means more O3DS performance.
- **OPT-021:** avoid per-frame heap allocations, `memcpy` of large images, repeated texture conversion, unconditional 2D layer reconstruction, and GPU-to-CPU readbacks; enforce with counters and forbidden-symbol checks.

## 10. Experimental procedure — mandatory for every OPT entry

For each candidate, write a short hypothesis before editing:

```text
ID: OPT-XXX
Observed symptom: <scene + hardware run + metric>
Proposed mechanism: <what changes in source/driver behavior>
Expected movement: <which counters should decrease/increase>
Unchanged contracts: <affected G2D/G3D/FNT/RES/FRM tests>
Benchmark environment: <unit, firmware, loader, clock, binaries, SHA>
A/B design: <warmup, samples, run order, confidence/noise>
Acceptance: <numeric threshold and worst-case bound>
Rollback: <git commit, reason, new issue if unresolved>
```

Instrumentation rules:

1. **Baseline A (off)**: representative O3DS scene, clean optimized build, no heavy logging.
2. **Baseline A (instrumented)**: same build with stage counters to quantify overhead.
3. **Variant B (instrumented)**: same scene, input script, warm-up and toolchain, collect p50/p95/p99, frame misses, audio, memory and GPU queue.
4. **Variant B (off)**: prove measured gain persists without instrumentation.
5. **Behavior comparison**: deterministic game-state checksum, reference images/pixel masks, actual audio, and gameplay events.
6. **Adverse scenes**: force cold texture load, rapid map change, worst battle, HBlank heavy effect and stale-resource risk.
7. Revert if effect is indistinguishable from noise, lowers completeness, increases worst-case frame misses or adds dangerous complexity for trivial gain.

Never enter a loop of dozens of performance experiments without a hypothesis and deliverable. The *result* of a rejected technique is a documented engineering finding.

## 11. Ordering of experiments

- **First wave (prove architecture):** OPT-001/002/003/004/006/021, plus mandatory font/window correctness.
- **Second wave (real-scene tuning):** OPT-005/007/008/009/010/012/014/017.
- **Third wave (only measured bottlenecks):** OPT-011/015/016/018/019/020.
- **Experimental/future:** OPT-013, extra per-pixel filtering, 3D stereo and wide 800×240 mode.

Do not optimize an unimplemented effect. Correctness and 30 FPS must coexist on O3DS; no N3DS-only staging pass counts as success.

## 12. Core sources

- Local Coffeepot `C:\Users\terezi\workdir\coffeepot\ctr\benchmarks\README.md`, `ctr\engineering\PICA200\renderer-architecture.md`, `ctr\engineering\PICA200\2d-tile-renderer.md`, `ctr\engineering\PICA200\gpu-rendering.md`, `ctr\engineering\cpu-optimization.md`, `ctr\engineering\memory-and-cache.md`, `ctr\engineering\threading.md`.
- [Coffeepot benchmark suite](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/benchmarks/README.md), [PICA command model](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/pica200-command-pipeline.md), [official citro3d](https://github.com/devkitPro/citro3d), [libctru](https://github.com/devkitPro/libctru), [3dbrew GPU](https://www.3dbrew.org/wiki/GPU), [Coffeepot HGSS case study](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/case-studies/case-study-hgss-port.md).
