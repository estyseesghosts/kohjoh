# Lessons

The story of kohjoh so far: what we built, what we learned, what worked, what
broke and how we recovered. Last revised 2026-11-16. Current state lives in
[status.md](status.md); per-session detail lives in [worklog/](worklog/). The
rules that came out of all this are at the end.

## 1. What we have done

kohjoh is a new HGSS 3DS port: a GPU-native runtime on top of the compile
layer vendored from hgss-3ds. The work so far, in order:

1. **Day one (2026-11-12).** Vendored hgss-3ds's patch stack and build driver
   (the decomp compiled under GCC, ~90 verified fixes), wrote the runtime and
   renderer, and had Goldenrod rendering in Azahar from a prior bench save.
   Found and fixed the PICA tile row-order bug; added frame stepping, state
   dumps and an offline DS reference renderer.
2. **Correctness (11-12 to 11-13).** Title-screen Ho-Oh: DS W-buffer depth,
   direct G3 command-register writes routed to the geometry engine (patch
   0302/2004), a crash handler, an offline projector and a zero-matrix trace.
   A complete Windows build guide.
3. **O3DS performance, sessions 2 and 3 (11-13 to 11-14).** Read the first
   hardware logs, capped present skipping, shrank the G3 decode loop under the
   I-cache, made Pokémon crypto word-wide (game patch 2006), batched geometry
   DMA, fixed the display-list cache and made it default, found the sound
   mixer thread was never profiled, and added a scene-level profiler.
4. **Audio, session 4 (11-15).** chatlog11's software audio plan in five
   passes ([worklog](worklog/2026-11-15-audio.md)): measurement, ordinary-
   routing fast path, ADPCM decoded as a run then resampled, exact `div127`,
   PKHBT output, 192-frame blocks, held samples by default, a decoded-ADPCM
   cache with a verify build, and kernel cache flush for wave buffers.
   Azahar Goldenrod mixer 107.6 → 48-49 ticks per channel-sample (14.8% →
   5-7% of the core), every step bit-exact. New tests: `sound_arm.py`
   (Unicorn full-chip differential), `div127_test.c`, `sndprof.py`.
5. **Geometry PR stack #6/#7/#8 (11-15).** Cheaper FIFO cache-hit keys,
   skipping discarded field/title scene production, DS lighting moved to the
   PICA with cached expanded lit geometry. Merged into `main` with origin/main;
   integration testing found a cache hang (`037e134`) before the push.
6. **Hardware cuts (11-15 to 11-16).** Baked stable lit lists, culled retained
   blocks outside the picture, restored the NNS GLB state list from a prefix
   snapshot, and skipped disabled lights. Azahar Goldenrod logic 25.35 →
   13.38 ms, triangles 5.02M → 1.94M per 600 pictures, with pixel-identical
   held frames. Still **O3DS_UNTESTED** on hardware.
7. **Tooling and release.** Junction targets compared through `cygpath`, a
   serial variant builder, GitHub Actions edition builds, edition asset
   preparation, CIA forwarders, `prepare.py`/`prepare.bat` one-step ROM prep,
   a README install walkthrough, and a null-BTP guard (patch 0307, issue #9).

## 2. What worked

**Strategy**
- **Reuse the game-compile layer, rewrite the slow parts.** Vendoring the
  hgss-3ds patch stack (with provenance) gave a linking game in hours; the
  time went to the runtime and renderer, where hgss-3ds had lost its speed.
- **One thread, one publish point.** Rendering at a single coherent point in
  the game loop (patch 2001) removed the drawer/game races hgss-3ds spent
  weeks on. No locks, no torn frames.
- **GPU transform with a matrix palette.** DS matrix, lighting and test
  semantics stay on the CPU; object-space vertices go to the PICA. 3D worked
  first try, widescreen included (one fixup matrix).
- **Doing less instead of doing it faster.** Baking stable lit lists, culling
  offscreen retained blocks and restoring the GLB prefix removed work outright
  and beat every micro-optimization.
- **Read the docs first.** The coffeepot wiki already held "never skip two
  pictures in a row" (frame-pacing) and the 16 KiB I-cache cliff
  (cpu-optimization, icachebench).

**Finding the truth**
- **An independent oracle.** The GBATEK reference renderer found the tile
  row-order bug in one comparison after guessing had failed.
- **Read the hardware log before the code.** o3ds-log-0's `[pace]` and
  `[frame]` lines showed presentation starvation; the per-opcode G3 profile
  (probe cost of ~440 ticks, the empty END opcode, subtracted) ranked the
  decoder's real costs.
- **Real profiling.** Azahar's GDB stub plus a 150-line PC sampler found the
  true hotspots, including Pokémon crypto, which no reasoning predicted. A
  Unicorn per-address count mapped with `addr2line` later found register
  spills stage timers could only hint at.
- **Calibrating the emulator.** A 4-instruction loop showed Azahar charges one
  tick per instruction: its times are instruction counts, not O3DS time, and
  it models neither caches nor PMU.
- **Count before you build.** `[sndadpcm]` showed >95% of ADPCM decoding
  repeats the same 15-20 sounds and sized the cache arena (256 KiB thrashed).
  `[sndlock]` and `[sndout]` showed the game never waited on the sound lock
  and the "1 underrun" in every hardware log was start-up, which removed two
  planned changes.
- **Verify modes.** `KJ_DL_VERIFY` (decode live, compare with replay) found
  the display-list bug in one run, and, as a bisector, showed an image diff
  did not come from the snapshot restore.

**Keeping changes safe**
- **Exact rewrites, checked by counters.** Every decoder change kept the same
  batches, draws and triangles per 600 frames.
- **A differential harness first.** `sound_arm.py` ran reference and working
  mixers on identical random register programs, so every later change was
  checked bit for bit in a minute.
- **Mutation before trust.** Each new cache got a host test, then the test was
  broken on purpose (dropped invalidations, a wrong plane) until it failed.
  This exposed weak test designs before they could pass bad code.
- **Commit each verified pass.** Every regression stayed bisectable to one
  change.
- **Test the merged tree before pushing.** The full native suite on the merge
  found a hang no single branch showed.

## 3. What went wrong, and how we recovered

### Process

| Problem | Recovery / rule |
| --- | --- |
| **Committing late.** Verified improvements sat uncommitted while riskier work piled on top. | Commit every verified step; gate risky features behind `--define` switches. |
| **Building before measuring.** The display-list cache was written before its hit rate was known; most lists straddled DMA bursts or varied per frame. | Measure the opportunity first. |
| **Measuring the wrong thing.** The cache replayed 4% of words; the first guess (lists left with BEGIN open) explained 13%. A coverage counter (`[dlcov]`) showed 65% were split mid-command by NitroSDK's 118-word DMA bursts. | Count where the work goes before fixing a guessed cause. |
| **Guessing at render state.** Four Ho-Oh experiments (cull, depth, alpha test, blend) changed nothing. Dumping the scene and projecting it on the CPU (`g3_reference.py`) showed 41 all-zero clip matrices per frame: the geometry was wrong, not the GPU path. | Split the pipeline at a dump before toggling GPU state. |
| **"Ported" is not "complete".** Patch 0302 converted GXFIFO stores but not the direct command-register stores beside them; the writes vanished silently. | When a hardware access class is ported, scan the whole tree for it (game patch 2004). |
| **Misreading a counter.** `[ds] per frame` divides by presented frames; with skips it overstated per-game-frame costs, and a note wrongly claimed "near full speed". | Fixed the docs; counter semantics are written in testing.md. |
| **Azahar hid a regression.** PR #8 doubled the O3DS captured frame (36 → 57 ms) while Azahar still showed 30 FPS. | Compare work counts (instructions, triangles, draws, shader instructions per vertex), never Azahar FPS. |
| **An unprofiled thread.** The DS sound mixer took 20-30% of the O3DS app core, but Azahar lacked DSP firmware, so `ndspInit` failed and the mixer never ran under test. | Every thread the device runs must run under test (`dspfirm.cdc` is now in Azahar's sdmc). |

### Performance traps

- **Timing probes that dominate.** Per-call `svcGetSystemTick` pairs cost ~390
  ticks and nested DMA callbacks inflated the timed region. Prefer sampling.
- **"Uninstrumented" builds still carried probes.** `PORT_NOW` in the H-blank
  walk ignored `HGSS_PROFILE_OFF` (~800 tick calls a frame). Probes are now
  opt-in (`HGSS_PROFILE=on`).
- **Sampler bias.** Tight loops (the crypto XOR) looked bigger in samples than
  in `[perf]` time. Cross-check with stage times.
- **An uncapped present skip starves the screen.** "Skip when two refreshes
  behind" never presents while the game runs slightly over budget: O3DS
  Goldenrod ran at half speed but skipped 1606 of 2206 pictures. Skip at most
  one picture in a row.
- **Azahar cannot see code size.** Force-inlining the vertex fast path made
  the decode loop 14.6 KiB against a 16 KiB I-cache (CPI hits 5.6 past
  32 KiB). Check hot-function sizes with `nm -S`.
- **Bump arenas that flush.** Re-recording into fresh space filled the arena
  and flushed it twice per 600 frames (29.6 ms spikes). Reuse an entry's
  storage when it re-records.
- **Snapshot what repeats, not what nearly repeats.** A full GLB snapshot never
  hit because its last word changes per model; splitting at the last command
  word made the rest a constant prefix.
- **Synthetic workloads mislead caches.** The first audio bench's instruments
  were too large to repeat and showed the cache as a loss. Size synthetic data
  from logs of the real workload.
- **PICA bool uniforms read as false in Azahar** though the shbin, libctru and
  citro3d looked correct. Unused `.w` components of float uniforms plus
  `cmp`/`jmpc` work. The lit shader has no float uniform space left for new
  arrays (91 + 7 of 96).

### Code bugs

- **Struct layout.** `memset(&s, 0, offsetof(s, array))` left counters after
  the array unreset. Reset fields explicitly.
- **State valid only after a reset.** The ADPCM cache's bucket table used -1
  for empty, set by the reset; `sound_diff.c` skips the reset and hung in
  `cache_find`. Links are now index + 1, so the zeroed table is empty. Make
  the zeroed state valid.
- **A cache check in the wrong path.** An early header comparison in the fill
  path compared the wrong bytes. The verify build and harness caught it; every
  check belongs where the entry is used.
- **A digest that hashed raw addresses** reported a block-size change as a
  behaviour change. Hash addresses relative to their buffer.
- **A harness `memset` compiled into a call to itself** (GCC loop-pattern
  distribution), a hang that looked like a slow emulator. Freestanding
  helpers need `no-tree-loop-distribute-patterns`.
- **Gaps found only by mutation.** The first coverage programs never shared a
  sound between channels or rewrote bytes inside a playing sound, so breaking
  cache invalidation still passed. Mutate each safety check once and confirm
  the harness fails.

### Build and environment

- **An unverified build-system change regressed saves.** Switching the SDK shim
  from copies to junctions changed what compiled; only a save check caught it.
- **A path compare ignored MSYS2's mount spellings.** `Path.resolve()` spells
  one directory `/c/...` or `/home/<user>/...` depending on which side of a
  junction it starts from, so `link_points_to()` called a correct junction
  missing and `_link()` re-ran `mklink /J`. Four parallel variants made three
  lose the race. Compare through `cygpath -w` and serialize the variants
  (`tools/game/build_variants.py`).
- **A shared `build/upstream` checkout carries branch state.** It held
  `wip/savestore`'s SDK patch 2003 while building `master`; `check_patches.py`
  only detects missing patches, not extra ones. Re-created with
  `bootstrap.py --clean` (2026-11-12); do the same after switching branches.
- **Stacked PRs (#6 ← #7 ← #8)** target each other's branches; merging into
  `main` by push does not close them on GitHub. A PR that adds a game patch
  also needs `git -C build/upstream am` before it builds.
- **Two harnesses for one module.** `sound_arm.py` resets the chip;
  `check_hotpaths.py`'s `sound_diff.c` does not. Run both after mixer changes.

### Test discipline

- **Contaminated tests.** Live input reached scripted runs. Tests own their
  input.
- **Harness races.** A second `run.ps1` killed the first's emulator (three
  lost runs). Stop the old harness first; one Azahar harness at a time.
- **Lost samples.** gdbprof printed nothing when Azahar closed; it now prints
  what it has.
- **Stale reference images.** A wandering NPC moves with the emulated clock, so
  an unchanged binary stopped matching references captured hours earlier;
  three bisection builds chased it. Rerun the baseline binary first when only
  character positions differ.
- **Killing a test that was working.** A host check looked idle (Python at
  1.7 s CPU, no children) but its binary ran under another parent. List every
  process created since the start, with CPU time, before calling it hung.
- **Cygwin signals in a tight loop.** An `alarm()` watchdog never fired; a
  watchdog thread with `-finstrument-functions` named the looping function in
  one run.

### Shell and editing hazards

- **Fragile command lines.** Multi-line heredocs with nested quotes were
  mangled by the shell wrapper; complex edits belong in script files.
- **Escapes through Git Bash.** A `sed` insertion turned a backslash-n in
  `hgss_game.py` into a real newline (SyntaxError at build time) and
  happened twice more. Use the editor tool, or avoid the escape (`putc(10)`).
- **Windows Python turned LF into CRLF** (a 900-line diff). Write with
  `newline=''`.
- **A stray `cat >` with no input** stalled a command for a minute and left its
  scripts unwritten.

## 4. Themes

1. **Measure, then change.** Nearly every wasted effort (cache before hit rate,
   render-state toggles, sampler or counter misreads) was a guess that a
   counter, dump or oracle would have settled in one step.
2. **The emulator is not the device.** Azahar counts instructions. It hides
   cache cliffs, PICA cost, missing threads and real timing, so it ranks
   instruction-count changes and nothing else.
3. **Trust nothing you have not tried to break.** Verify modes, differential
   harnesses and mutation testing caught more bugs than review did.
4. **Small verified commits.** They made bisection cheap and risk containable.
5. **Remove work first, speed it up second.**

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
9. After touching the sound mixer, run `tests/native/sound_arm.py`
   (bit-exact against `HEAD`) and `tests/native/check_hotpaths.py` before
   building.
10. Make zeroed state valid, and mutate each safety check once to confirm its
    test fails.
11. After switching branches, run `bootstrap.py --clean`.
