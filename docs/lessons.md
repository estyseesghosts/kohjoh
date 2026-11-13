# Lessons

What helped, what cost time, and the rules that follow.

## What went right

- **Reuse the game-compile layer, rewrite the slow parts.** hgss-3ds's patch
  stack and build driver compile the decomp under GCC (~90 verified fixes).
  Vendoring them (with provenance) gave a linking game in hours, and the time
  went into the runtime and renderer, which is where hgss-3ds lost its speed.
- **One thread, one publish point.** Rendering at a single coherent point in
  the game loop (patch 2001) removed the drawer/game races that hgss-3ds spent
  weeks on. No locks, no torn frames.
- **GPU transform with a matrix palette.** Keeping DS matrix, lighting and test
  semantics on the CPU but sending object-space vertices to the PICA made 3D
  work on the first try, widescreen included (one fixup matrix).
- **An independent oracle.** The GBATEK reference renderer found the tile
  row-order bug in one comparison after guessing had failed.
- **Real profiling.** Azahar's GDB stub plus a 150-line PC sampler found the
  actual hotspots (geometry decode, Pokémon crypto), including one outside the
  renderer that no reasoning would have predicted.
- **Calibrating the emulator.** A 4-instruction loop showed Azahar charges one
  tick per instruction, so its timings are instruction counts, not O3DS time.
- **Verify modes.** `KJ_DL_VERIFY` (decode live, compare with replay) found the
  display-list bug in one run.

## What went wrong

- **Committing late.** Several verified improvements sat uncommitted while
  riskier work piled on top. Commit every verified step; gate risky features
  behind `--define` switches.
- **Building before measuring.** The display-list cache was written before its
  hit rate was known; most lists turned out to straddle DMA bursts or vary per
  frame. Measure the opportunity first.
- **Timing probes that dominate.** Per-call `svcGetSystemTick` pairs cost ~390
  ticks each and nested DMA callbacks inflated the timed region; both misled.
  Prefer sampling.
- **A struct-layout bug.** `memset(&s, 0, offsetof(s, array))` left counters
  that follow the array unreset. Reset fields explicitly.
- **An unverified build-system change regressed saves.** Switching the SDK shim
  from copies to junctions changed what compiled; it went untested past a save
  check. Run the save scenario after any build-system change.
- **Fragile command lines.** Multi-line heredocs with nested quotes were
  mangled by the shell wrapper; complex edits belong in script files.
- **Contaminated tests.** Live input reached scripted runs. Tests must own
  their input.
- **Guessing at render state.** Four Ho-Oh render-state experiments (cull,
  depth, alpha test, blend) changed nothing. Dumping the decoded scene and
  projecting it on the CPU (`g3_reference.py`) showed in one step that the
  geometry, not the GPU path, was wrong: 41 all-zero clip matrices per frame.
  Split the pipeline at a dump before toggling GPU state.
- **"Ported" is not "complete".** Patch 0302 converted the GXFIFO stores in
  NNS but not the direct command-register stores beside them, and nothing
  failed loudly: the writes simply vanished. When a hardware access class is
  ported, scan the whole tree for that class (see game patch 2004).
- **A shared `build/upstream` checkout carries branch state.** It held
  `wip/savestore`'s SDK patch 2003 while building `master`; `check_patches.py`
  only detects missing patches, not extra ones. Re-created with
  `bootstrap.py --clean` on 2026-11-12; after switching branches, do the same.
- **An uncapped present skip starves the screen.** "Skip the picture when
  two refreshes behind" never presents while the game runs slightly over
  budget: O3DS Goldenrod ran at half speed but skipped 1606 of 2206
  pictures, ~4 a second (and probably left the title's white flash on screen). Skip at most one
  picture in a row (coffeepot frame-pacing, "skip the picture, keep the tick").
- **"Uninstrumented" builds still carried probes.** `PORT_NOW` in the H-blank
  walk ignored `HGSS_PROFILE_OFF`, and profiling was the default: ~800
  `svcGetSystemTick` calls a frame. Probes are now opt-in (`HGSS_PROFILE=on`).
- **Azahar cannot see code size.** Force-inlining the vertex fast path into
  every opcode made the decode loop 14.6 KiB; the O3DS I-cache is 16 KiB and
  CPI reaches 5.6 past 32 KiB (coffeepot icachebench). Check hot-function
  sizes with `nm -S` after touching a hot loop.

## Session 2 (O3DS Goldenrod performance)

What went right:
- **Reading the hardware log before the code.** o3ds-log-0's `[pace]` and
  `[frame]` lines showed presentation starvation; the per-opcode G3 profile
  (probe cost subtracted: ~440 ticks, the cost of the empty END opcode)
  ranked the decoder's real costs on O3DS.
- **The wiki had both answers**: "never skip two in a row" (frame-pacing)
  and the 16 KiB I-cache cliff (cpu-optimization, icachebench).
- **Exact rewrites, checked by counters**: every decoder change kept the
  same batches, draws and triangles per 600 frames in Azahar.

What went wrong, and the fix:
- **Misreading a counter.** `[ds] per frame` divides by presented frames;
  with skips it overstated per-game-frame costs, and an early note claimed
  the game ran "near full speed" when that window ran at half speed. Fixed
  in docs; counter semantics are now written in testing.md.
- **Harness races.** A second `run.ps1` killed the emulator of the first
  (three lost runs). Stop the old harness before starting another.
- **Lost samples.** gdbprof printed nothing when Azahar closed; it now
  prints what it has.
- **Sampler bias.** Tight loops (the crypto XOR) looked bigger in samples
  than in `[perf]` time. Measure with stage times.

- **Measuring the wrong thing.** The display-list cache replayed 4% of
  words. The first guess (lists left with BEGIN open) explained 13%; a
  coverage counter (`[dlcov]`) showed 65% of words were split mid-command
  by NitroSDK's 118-word DMA bursts. Count where the work goes before
  fixing a guessed cause.
- **Bump arenas that flush.** Re-recording into fresh space filled the arena
  and flushed it twice per 600 frames (29.6 ms spikes). Reuse an entry's
  storage when it re-records.
- **An unprofiled thread.** The DS sound mixer runs on its own thread and
  took 20-30% of the O3DS application core (o3ds-log-1), but Azahar lacked
  DSP firmware, so `ndspInit` failed and the mixer never ran in any
  emulator profile. Make sure every thread the device runs also runs under
  test (`dspfirm.cdc` is now in Azahar's sdmc).

## Rules

1. Commit each verified step; keep `master` building and verified.
2. Check the Coffeepot wiki and `docs/engineering/hgss-remaster/` before
   inventing a technique; cite it.
3. Compare against the oracle or a known-good capture before blaming a subsystem.
4. Profile with PC sampling; treat Azahar time as instruction count.
5. Every cache needs an exact validity check and a verify mode.
6. After any build-system change, rerun intro, Goldenrod (with save) and input.
7. After touching a hot loop, check its size with `nm -S` (16 KiB I-cache).
8. One Azahar harness at a time.
