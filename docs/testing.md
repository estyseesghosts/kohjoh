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
- `[frame]` lines report real refresh intervals (0/1/2/3/4+) and hitches with
  per-stage times and counters. Azahar's "App FPS" is not a stutter measure
  (unchanged screens are not presented).
- A crash writes `logs/crash.txt` (pc, lr, registers) before the system
  error handler; resolve addresses with addr2line on the `.elf`.

## Profiling

- **Counters**: every 600 *presented* frames the log gets `[perf]` (stage p50/p95/p99/max),
  `[pace]` (late V-blanks, resyncs, skipped presents), `[gpu]` (layers
  reused/rebuilt, screens skipped, unsupported counts) and `[ds]` (geometry
  words, H-blank cost, display-list cache). `[ds] per game frame` divides by
  the H-blank walk count (one walk per game frame) and prints it with the
  picture count, so skipped presents no longer inflate it. `[perf] g2d` is
  exclusive: the g3d time spent inside it is subtracted (before `6191966`
  g2d included g3d). `[dlcov]` (with `KJ_DL_CACHE`) splits FIFO words into
  bypassed (small, split mid-command, open BEGIN) and new/changed/never/hit/
  keymiss; `[dlwhy]` counts uncacheable reasons and arena flushes.
  `[gpu] ... char KiB compared` is the retained layers' character check.
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
    late enough last shot, and never start a second harness while one runs:
    each `run.ps1` kills every Azahar when it finishes.
  - Samples are taken when Azahar's JIT stops at a block boundary, so tight
    loops are over-represented. Use samples to find *where*; use `[perf]`
    stage times (instruction counts) to measure *how much*.
- **Code size**: Azahar does not model the 16 KiB I-cache. After editing a
  hot loop, check `arm-none-eabi-nm -S --size-sort` on the `.elf`
  (`NitroPort_GxFifoWords` must stay well under 16 KiB).
- **Probes**: `HGSS_PROFILE=on` compiles the HGSS H-blank/2D tick probes in
  (several hundred `svcGetSystemTick` calls a frame). Normal builds leave
  them out; never make a timing claim from a probe build.
- **Present skipping is disabled while a script runs** (`kj_script_active`).
  To see pacing as the player does (title, field), run without `-Script`.
- **Calibration**: the `[calib]` log line shows Azahar's ticks per instruction
  (measured 1.0). Treat Azahar milliseconds as instruction counts.
- `--define KJ_G3_PROFILE` times each geometry opcode. Each probe costs ~390
  ticks; subtract it, and prefer PC sampling.
