# Testing and profiling

All tools here target Azahar. Azahar is for correctness and instruction-count
profiling; it is not an O3DS timing authority.

## Harness: `tools/azahar/run.ps1`

| Option | Effect |
| --- | --- |
| `-Artifact` | `.3dsx` to run (default `build/kj/pokeheartgold.3dsx`) |
| `-Pack`, `-Manifest` | stage a data pack (only needed once) |
| `-Save path` / `-FreshSave` | install a save / delete the test save |
| `-Script file` | scripted input (below); without it `script.txt` is removed |
| `-Shots 10,30` | screenshots at seconds after launch |
| `-Holds 900,5050` | screenshot when the log says game frame N is held |
| `-Tag`, `-Timeout`, `-Extra` | output folder, hold timeout, seconds kept after the last shot |
| `-Emulator exe` | Azahar executable (portable `user/` beside it is used when present; see parallel tests below) |

Outputs: `build/azahar/<tag>/` (PNGs, program log, Azahar log). The harness
resizes the window and is DPI-aware so captures are complete.

## Scripts (`tests/inputs/*.txt`)

```text
every: 10             # each press lasts 10 V-blanks
5000 START            # press at V-blank 5000 (A B X Y L R START SELECT UP DOWN LEFT RIGHT)
10200 TOUCH:40,130    # touch in DS bottom-screen coordinates
dump 5000             # write the renderer's DS input for game frame 5000
freeze 5000 3         # hold game frame 5000 on screen for 3 s
end: 11800            # close at V-blank 11800
```

V-blank indices count from boot; game frames are publishes (≈ V-blanks / 2).
A scripted run ignores live input on `wip/savestore`; on `master` a live press
can still leak in, so do not type into the Azahar window during a test.

| Script | Purpose |
| --- | --- |
| `intro-step.txt` | intro frames 150-1650, dumps + holds (no save needed) |
| `goldenrod-step.txt`, `goldenrod-input.txt` | bench save → Goldenrod; touch/B/DOWN checks |
| `goldenrod-idle.txt` | stand in Goldenrod (profiling) |
| `route34-south.txt` | walk south toward Route 34 (path not yet calibrated) |

The Goldenrod save is hgss-3ds's `build/bench/heartgold.sav`.

## Reference renderer

`python tools/capture/ds_reference.py dump.kjd out` renders the DS 2D picture
of a dump straight from GBATEK (no shared code with the GPU path) to
`out_A.png`, `out_B.png`, `out_screens.png`. It does not render 3D (BG0 3D is
left transparent). Dumps are written to `sdmc:/3ds/kohjoh/dumps/`.

`python tools/capture/g3_reference.py dump.kjd out.png` projects the dump's
decoded 3D scene with plain float math (one flat colour per batch, DS-style
depth, triangles with a vertex at w <= 0 counted and skipped). Present here but
missing on the 3DS: the GPU stage (`src/graphics/g3d`). Missing here too: the
decoder or the game's command stream (`src/runtime/kj_g3.c`, SDK/NNS
patches). `--define KJ_G3_ZERO_TRACE` logs the first commands that leave a
zero position/projection matrix (`[g3zero]`).

## DS oracle

melonDS (`workdir/melonDS.exe`, source in `workdir/melonDS`) runs the retail
ROM. `tools/melonds/shots.ps1` saves timed screenshots to
`build/melonds/<tag>/`; the HG title shows Ho-Oh from about 85 s. Check
`melonDS/src/GPU3D.cpp` before changing 3D semantics.

## Script extras

- `9600-11400 DOWN B`: hold keys over a V-blank range.
- `debug: 0x1|0x2|0x4|0x8|0x10`: 3D draw without cull / depth test / alpha
  test / blending / texture (`KJ_DEBUG_*` in `src/app/script.h`).
- `[frame]` lines report submission refresh intervals (0/1/2/3/4+) and hitches with
  per-stage times and counters. Azahar's "App FPS" is not a stutter measure
  (unchanged screens are not presented).
- A crash writes `logs/crash.txt` (pc, lr, registers) before the system
  error handler; resolve addresses with addr2line on the `.elf`.

## Profiling

Build the baseline with `HGSS_PROFILE` unset and the diagnostic build with
`HGSS_PROFILE=on`, using a separate `--out` for each. The build-info sidecar
records `profiling` and any `--define` switches. The baseline omits the
frame, GPU, FIFO and mixer profiling probes; ordinary lifecycle and error
logging remains enabled. Log and crash files append across launches, with a
`--- new run ---` separator in the main log. Scripted frame dumps still use
their frame-number filenames and can overwrite earlier dumps.
The instrumented build times the whole H-blank walk. Per-scanline and IRQ
probes require `HGSS_A0_DIAG`; they are too intrusive for normal profiling.
Use the normal build for hardware performance comparisons.

`[gamecpu]` reports logic percentiles over 600 game frames, including frames
whose picture was skipped. `[perf] logic` now covers only the single game
frame producing each displayed picture. Older logs accumulated skipped
frames into the next picture: **80 ms per picture could cover two game
frames**, and is not an 80 ms game update. Display intervals still cover
all intervening skipped pictures. Skips have their own count and no longer
inflate the submission hitch count; hitch g2d is exclusive of g3d.

`[gamecpu] active logic+publish` additionally reports per-game-frame wall
work including rendering/queue wait (or the skipped-publish path), excluding
V-blank waits. Compare this with the roughly 33.4 ms budget. Publish is a
parent of renderer stages; do not add it to g2d/g3d/submit again. Profiling
and preemption perturb these numbers; use the normal build for smoothness.

`[frame]` is **submission cadence**, not verified LCD presentation. `[scene]`
gives submission IDs, the game-frame range, latest DS scene serial, repeated
scene submissions, empty submissions, and the last GPU-complete game/scene.
Scene serial increments at DS SWAP_BUFFERS; a new serial does not prove an
animation pose changed. GPU completion is observed at the next successful
FrameBegin, not at its exact interrupt time. Actual scanout/presentation is
unobserved; use hardware video/reference captures to establish visual cadence.
No GSP callbacks are replaced (Citro3D owns those callbacks).

`[dltop]` emits the 20 largest **sampled total** FIFO costs in each existing
600-submission geometry reporting window. Keys are source pointer, word
count and immediate caller return address; resolve callers against that
build's ELF. Short calls (<16 words) use `src NULL`, grouped by size/caller,
to avoid filling the table with transient interior pointers. Addresses are
run-local allocations, not stable model IDs or content hashes. Reused memory
may aggregate different contents. Table capacity is 512 keys; omitted sample
count/ticks are explicit. A window with omissions is an incomplete ranking.

Each row reports sample count, replay hits, first/last DS swap, raw total /
decode / replay / nested copy / maximum ticks, copied vertex+index bytes,
lighting requests/normal-cache misses, cache rejection mask, and sampled
64-swap never-list rechecks. Copy bytes exclude the auxiliary matrix-ID
array. Rejection bits match `[dlwhy]` and persist for never-lists across
reporting windows. The timing remainder includes key lookup/comparison,
recording, allocation and probes; it is not pure comparison time. Lighting
counts measure workload, not isolated shade time. Do not sum nested timings
or extrapolate one rare sample into an exact per-model cost. PC sampling can
still distinguish shading from other live-decoder work.

`[g3sample]` randomly samples FIFO calls with probability 1/64 using a
reproducible xorshift sequence, avoiding correlation with repeated model
call patterns. Total, live decode and replay are raw ARM11 ticks for the
samples; preemption and probe overhead remain included. `block_copy_ticks` is nested inside replay and covers vertex/index copying
and rebasing; replay minus that child includes state events, matrix/palette
preparation and exit-state restoration. Do not add the child to replay.
The outer remainder is lookup/comparison/recording. These are estimates, not exact window totals.
The first GSP reporting window establishes a baseline and is explicitly
marked `baseline/unavailable`; failed reads invalidate its baseline. Deltas
exceeding elapsed wall time for an operation engine are rejected and
rebaselined (two engines for the summed fill counters).

- **Counters**: every 600 *submitted* frames the log gets `[perf]` (stage p50/p95/p99/max),
  `[pace]` (late V-blanks, resyncs, skipped presents), `[gpu]` (layers
  reused/rebuilt, screens skipped, unsupported counts) and `[ds]` (geometry
  words, H-blank cost, display-list cache). The legacy `cmds` counter counts
  general-dispatch commands and excludes bulk fast commands; use FIFO words
  for workload comparisons. `[ds] per game frame` divides by
  the H-blank walk count (one walk per game frame) and prints it with the
  picture count, so skipped presents no longer inflate it. `[perf] g2d` is
  exclusive: the g3d time spent inside it is subtracted (before `6191966`
  g2d included g3d). `[dlcov]` splits FIFO words into
  bypassed (small, split mid-command, open BEGIN) and new/changed/never/hit/
  keymiss; `[dlwhy]` counts uncacheable reasons and arena flushes.
  `[gpu] ... char KiB compared` is the retained layers' character check.
  `[gputime]`: GSP perf-log microseconds per picture (P3D command lists, PPF
  display transfers, PSC fills; 0 in Azahar, which does not implement the
  log), `C3D_GetDrawingTime` average/max and completed-queue count (sampled
  after the next FrameBegin queue wait, attributed to the preceding non-empty
  submission), and peak command-buffer use. Empty submissions do not reuse
  a stale drawing-time value. GSP deltas remain window aggregates.
  `[sndprof]` (every ~9.4 s of audio, on the audio thread): cumulative mixer
  ticks, frames and active channels; cycles per channel-sample is
  Δmix_ticks / (Δactive_ch / Δmix_calls) / Δmix_frames. Needs
  `dspfirm.cdc` in Azahar's `sdmc/3ds/`, or NDSP fails and the mixer never runs.
  `python tools/azahar/sndprof.py LOG [--from S]` computes it.
  Printed with it (profile builds): `[sndstage]` cumulative ticks of one
  `NitroSound_Mix` call in 16 (setup, clear, channels ⊃ adpcm + scale,
  output; total includes the ~24 clock probes per sampled call, so stage
  ticks per channel-frame read higher than `[sndprof]`); `[sndadpcm]`
  cumulative nibbles decoded (`clean`: decoder state came from the data
  alone), ADPCM key-ons, and the distinct sounds started in the window with
  their compressed bytes; `[sndlock]` per lock role (game `command`, game
  `status`, `audio`): acquires, contended waits with ticks and max, hold
  ticks/max/p99 µs (status holds not timed). Every 600 wave buffers,
  `[sndout]`: wake-ups, underruns (all buffers played; start-up excluded),
  queue low-water in frames, NDSP dropped frames, refill (wake to last
  submit) and render µs; `[sndpmu]`: ARM11 counters around the render
  (cycles, I-cache misses, D-cache read misses; 0 in Azahar, which does not
  emulate them).
  `[sndcache]` (profile builds, cumulative): ADPCM blocks served by the
  decoded-sample cache and decoded live, samples decoded into it, entries
  evicted and invalidated (data or voice state differed), arena position,
  and verify mismatches (`HGSS_SND_VERIFY=1` builds; must stay 0).
  Mixer correctness: `build/diagnosis/venv/Scripts/python.exe
  tests/native/sound_arm.py [--reference REV]` (Unicorn; see its header)
  compares 1,000 full-chip register programs (also with an 8 KiB cache that
  evicts constantly) and an HGSS-like bench bit for bit, prints mixer
  instructions per channel-frame, and checks `div127` exhaustively.
- **PC sampling**: with Azahar's GDB stub enabled (`use_gdbstub=true`, port
  24689), `python tools/azahar/gdbprof.py build/kj/pokeheartgold-3ds.elf
  --samples 1500 --interval 0.02 [--callers fn1,fn2]` prints a function
  histogram (and callers via LR). This found every hotspot so far.
  - `--pcs fn1,fn2`: per-PC histogram inside those functions (map with
    `arm-none-eabi-objdump -d`).
  - `--stack fn:offset`: histogram of the word at SP+offset while sampled in
    `fn`, i.e. a return address further up the stack. Example: the caller of
    `GetMonData` from `_MonEncryptSegment` is `--stack _MonEncryptSegment:36`
    (16 bytes pushed by it, 8 locals + 12 to `lr` in `GetMonData`).
  - Partial results are printed if Azahar closes mid-run. Give the harness a
    late enough last shot, and never start a second harness on the same
    executable: `run.ps1` refuses one already running, and harnesses older
    than `8fb6f54` kill every Azahar when they finish.
  - Samples are taken when Azahar's JIT stops at a block boundary, so tight
    loops are over-represented. Use samples to find *where*; use `[perf]`
    stage times (instruction counts) to measure *how much*.
- **Code size**: Azahar does not model the 16 KiB I-cache. After editing a
  hot loop, check `arm-none-eabi-nm -S --size-sort` on the `.elf`
  (`NitroPort_GxFifoWords` must stay well under 16 KiB).
- **Probes**: `HGSS_PROFILE=on` enables frame, GPU, audio and sampled FIFO
  reporting. `HGSS_A0_DIAG` additionally enables the intrusive per-line
  H-blank probes. Neither is a hardware performance baseline.
- **Present skipping is disabled while a script runs** (`kj_script_active`).
  To see pacing as the player does (title, field), run without `-Script`.
- **Calibration**: the `[calib]` log line shows Azahar's ticks per instruction
  (measured 1.0). Treat Azahar milliseconds as instruction counts.
- `--define KJ_G3_PROFILE` times each geometry opcode. Each probe costs ~390
  ticks; subtract it, and prefer PC sampling.

## Native hot-path regression checks

Inside the devkitPro MSYS2 shell, run `python tests/native/check_hotpaths.py`.
It compiles the current sound/geometry code and the `HEAD` versions with
host GCC, compares output/state hashes for 100,000 randomized ADPCM programs
and 2,000 synthetic geometry frames (split FIFO, cache hits, all primitives,
lighting and texgen modes), and checks skipped-frame CPU and hitch accounting
with a mocked clock. The profiled geometry variant also verifies unchanged
output, sampled parent/child accounting, attribution overflow/reset, and
persistent cache rejection/recheck reasons. `--reference REV` selects another baseline. No game
assets are needed. This is differential coverage, not a complete DS oracle.

The suite also compares 10,000 matrix programs and tests active/pending light
dependencies, live-parameter state-only replay, and command-layout changes.
For the state-only exit-colour regression, the new cached output must match
both uncached decoders; the older cached output differs. Use
`--reference eaac5fd` to reproduce the pre-optimization comparison.

`goldenrod-quick-perf.txt` enters CONTINUE from the benchmark save earlier
than the original idle script. Use `-Shots 60,100` and inspect the captures
before treating its later windows as stationary Goldenrod measurements.
State-only `[dltop]` replay hits now include command-layout reuse with live
parameters: those hits still execute matrix/light/state commands and copy
zero vertices. They are distinct from retained vertex-block replay.

Run `python tests/native/visibility_test.py` for the offline clip-plane checks.
`python tools/capture/g3_visibility.py dump.kjd --csv batches.csv` classifies
offscreen triangles and whole batches without adding CPU culling to the game.
Default is widened top-screen visibility; `--ds-width` uses the bottom/DS
width. It uses per-batch viewports, per-vertex palette slots, and W-buffer
semantics. Counts are conservative clip rejection, not occlusion or measured
GPU time. Scripted dumps perform synchronous SD I/O: capture separately from
timing runs. See [scene investigation](worklog/2026-11-14.md) for the hardware run plan.

## Concurrent agent testing

Use a separate Git worktree and a private clone of the patched decomp for
each agent. Give it its own SDK shims and output directory; a separate
`--out` alone still shares generated upstream headers and shim directories.
Do not rebuild or clean another agent's upstream tree.

For parallel emulator tests, copy the Azahar installation into that
worktree's ignored `build/emulator/`, rename its executable (for example
`kj-geometry.exe`), and create `user/` beside it for portable mode. Copy only
the needed settings, DSP firmware, data pack and benchmark save into this
private profile. Disable custom storage and check that any explicit SD/NAND
paths point into the private profile. Then run:

```powershell
pwsh tools/azahar/run.ps1 -Emulator build/emulator/kj-geometry.exe `
  -Artifact build/fifo/pokeheartgold.3dsx -Shots 40,100 -Tag geometry
```

The harness stages and collects from portable `user/` when present, refuses
an already running executable or enabled custom storage, and stops only its
own process in `finally`. A distinct executable name protects this instance
from older harnesses that still kill all processes named `azahar`. Avoid
concurrent host timing comparisons: emulator correctness tests can run in
parallel, but their contention invalidates performance comparisons.
