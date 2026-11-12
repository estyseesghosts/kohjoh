# Project status

This page summarizes the checked-in code and the recorded work in
[`chatlog.md`](chatlog.md). The chatlog is chronological: it includes failed
attempts and work that was not committed. Claims below identify which evidence
they rely on. The project design and remaining acceptance goals are in
[`kohjoh.md`](../kohjoh.md).

## Working foundation

- The decompilation, port references, toolchain and Azahar version are pinned
  in [`upstream.lock`](../upstream.lock). The patch and import history is in
  [`PROVENANCE.md`](../PROVENANCE.md).
- The build driver compiles the game and separate DS-side and 3DS-side runtime
  code, links them, and can package a `.3dsx` when `3dsxtool` is available
  ([`hgss_game.py`](../tools/game/hgss_game.py)).
- The runtime, DS frame bridge, 2D renderer and 3D renderer are in source
  ([`main.c`](../src/app/main.c), [`kj_frame.c`](../src/runtime/kj_frame.c),
  [`g2d.c`](../src/graphics/g2d/g2d.c),
  [`g3d.c`](../src/graphics/g3d/g3d.c)). Their presence does not mean all
  graphics features are supported; unsupported operations are counted.

## Results recorded in Azahar

The chatlog records successful boot and intro rendering, button and touch
input, and a recognizable Goldenrod field scene. It also records a run of 600
frame publishes at two VBlanks per publish, with no missed publishes in that
emulator run. These are specific Azahar observations, not physical-console
benchmarks or complete gameplay tests.

Commit `4063c5e` records the Goldenrod renderer milestone. Commit `40c6984`
records a cache-off state with pacing changes, a tile-edge inset, geometry
fast paths, retained 2D layers and profiling. The latter is the last committed
milestone named in the current repository history; consult the exact source
before treating chatlog claims as committed. Coffeepot explains the difference
between emulator checks and [hardware testing](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/testing-on-hardware.md).

## Current open work

- **Display-list cache:** `HGSS_GX3D_LISTCACHE=1` enables an experimental
  geometry command cache in the build driver and [`kj_g3.c`](../src/runtime/kj_g3.c).
  The chatlog reports a white map with sprites still visible and severe
  slowdown in one enabled run. Cache-off restored intro and Goldenrod. A later
  verify run never reached its target frame; its cause remains unresolved.
  Keep the cache disabled unless working on that investigation.
- **Game coverage:** Intro and a Goldenrod field scene are recorded. The
  transcript lists missing title-screen Pokémon and an uncompleted route into
  tall grass/battle as next work. It does not establish full HGSS gameplay,
  battle, cutscene, menu or graphics-effect compatibility.
- **Save and install formats:** the runtime currently uses an interim raw save
  file ([`save_raw.c`](../src/storage/save_raw.c)). A RomFS pack lookup exists
  in [`paths.c`](../src/storage/paths.c), but a CIA packaging flow and
  installed-title save archive are not verified implementations.
- **Linux:** no Linux build is recorded; see [the stub](building-linux.md).
- **Hardware:** no evidence here establishes sustained 30 FPS or acceptance
  on an Old 3DS. That requires the physical-device gate in
  [`kohjoh.md`](../kohjoh.md).

## Reading the evidence

Treat a result as scoped to the exact build, scene and environment described.
A boot proves that particular artifact started. A screenshot proves what was
visible in that capture. Neither proves unvisited scenes, save safety or
hardware frame rate. This matches Coffeepot's guidance on [performance
measurement](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/performance.md):
measure the limiting stage on the target hardware and keep emulator results
separate.
