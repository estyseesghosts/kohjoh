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

## Profiling

- **Counters**: every 600 publishes the log gets `[perf]` (stage p50/p95/p99/max),
  `[pace]` (late V-blanks, resyncs, skipped presents), `[gpu]` (layers
  reused/rebuilt, screens skipped, unsupported counts) and `[ds]` (geometry
  words, H-blank cost, display-list cache).
- **PC sampling**: with Azahar's GDB stub enabled (`use_gdbstub=true`, port
  24689), `python tools/azahar/gdbprof.py build/kj/pokeheartgold-3ds.elf
  --samples 1500 --interval 0.02 [--callers fn1,fn2]` prints a function
  histogram (and callers via LR). This found every hotspot so far.
- **Calibration**: the `[calib]` log line shows Azahar's ticks per instruction
  (measured 1.0). Treat Azahar milliseconds as instruction counts.
- `--define KJ_G3_PROFILE` times each geometry opcode. Each probe costs ~390
  ticks; subtract it, and prefer PC sampling.
