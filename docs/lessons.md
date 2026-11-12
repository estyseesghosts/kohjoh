# Lessons from the first day

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
  `wip/savestore`'s SDK patch 2003 while building `master`.

## Rules

1. Commit each verified step; keep `master` building and verified.
2. Check the Coffeepot wiki and `docs/engineering/hgss-remaster/` before
   inventing a technique; cite it.
3. Compare against the oracle or a known-good capture before blaming a subsystem.
4. Profile with PC sampling; treat Azahar time as instruction count.
5. Every cache needs an exact validity check and a verify mode.
6. After any build-system change, rerun intro, Goldenrod (with save) and input.
