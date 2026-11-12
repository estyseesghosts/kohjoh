# Project progress

This snapshot is based on the checked-in source, commit history and
[`chatlog.md`](chatlog.md). It is not a release checklist. The chatlog is a
transcript and can include intermediate or uncommitted states; later status
entries here take precedence for the present assessment.

## Completed and recorded

- The game source, relevant Nitro compatibility code and patch stack are pinned
  and bootstrapped. Provenance and excluded renderer imports are recorded in
  [`PROVENANCE.md`](../PROVENANCE.md); revisions are in
  [`upstream.lock`](../upstream.lock).
- The Windows-oriented build compiles the game and separate DS/3DS runtime
  source sets and packages a Homebrew `.3dsx` when tools are available. The
  transcript records a full link and a 5.49 MB artifact before the later
  renderer/cache work; current build behavior is in
  [`hgss_game.py`](../tools/game/hgss_game.py).
- The frame bridge and GPU renderers are present in source. Chatlog records
  Azahar boot, intro rendering, input/touch and Goldenrod rendering. A recorded
  600-publish timing run had two VBlanks per publish with no misses in Azahar;
  that result applies only to that run and emulator.
- Commit `4063c5e` records Goldenrod field rendering. Commit `40c6984` records
  the verified cache-off state with pacing lock, tile inset, geometry fast
  paths, retained 2D and profiler. See `git log` and
  [`chatlog.md`](chatlog.md).

## In progress or unverified

- Display-list recording/replay cache is gated by
  `HGSS_GX3D_LISTCACHE=1` in [`hgss_game.py`](../tools/game/hgss_game.py).
  The chatlog records white/untextured map geometry and severe slowdown in a
  cache-enabled build; cache-off restored the intro and Goldenrod. Verify mode
  stalled before reaching the planned frame, and the checked-out
  [`kj_g3.c`](../src/runtime/kj_g3.c) has local uncommitted edits. Treat cache
  behavior as experimental, not a completed optimization.
- Battle, broader field traversal, missing title-screen Pokémon, full
  HeartGold/SoulSilver coverage and physical-console performance have not been
  established by the recorded evidence.
- The planned CIA packaging and installed-title save flow are specified in
  [`kohjoh.md`](../kohjoh.md) but are not part of the verified artifact.
- Linux instructions are a stub because no Linux build is recorded.

## Evidence boundary

Azahar is valuable for integration and visual debugging, but emulator timings
are not Old 3DS performance measurements. Do not describe the project as
certified 30 FPS on Old 3DS until the real hardware acceptance work in
[`kohjoh.md`](../kohjoh.md) has been completed. Coffeepot explains the need for
separate [emulator and hardware validation](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/testing-on-hardware.md)
and [performance budgets](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/performance.md).
