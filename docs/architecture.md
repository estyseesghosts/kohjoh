# How kohjoh works

## In brief

kohjoh runs the original HeartGold/SoulSilver game logic on a Nintendo 3DS.
Rather than reuse the older port's graphics and application shell, it connects
that game to a new runtime. The runtime presents the DS-style graphics using
the 3DS GPU, reads 3DS controls, and supplies sound, storage and other system
services. This is a port of the HGSS games; Platinum is a reference for selected
code and behavior, not the game being ported. See the [port comparison](comparison.md)
and the import ledger in [`PROVENANCE.md`](../PROVENANCE.md).

The game sources come from the pinned decompilation listed in
[`upstream.lock`](../upstream.lock). The source is compiled for two different
environments: game and Nintendo DS compatibility code use DS-facing types,
while the 3DS runtime uses libctru and citro3d types. A small plain-C interface,
[`kj_bridge.h`](../src/runtime/kj_bridge.h), is the boundary between them.
[`hgss_game.py`](../tools/game/hgss_game.py) builds the separate source sets.
The [build guide](building-windows.md) explains how to run it.

## From startup to a picture

At startup, [`main.c`](../src/app/main.c) chooses the edition, finds its game
pack and save file, starts the renderer, samples input and starts the original
game. The current save backend connects a raw 512 KiB save file; it is
explicitly described in [`save_raw.c`](../src/storage/save_raw.c) as an interim
implementation. If the graphics or game pack cannot start, the app shows an
error screen and writes a log.

The game's own main loop remains in charge of updates. At each DS vertical
blank (the display's frame boundary), the 3DS runtime handles HOME/sleep/exit,
samples controls, services audio and paces the game against the physical
display. The current pacing code aims to pair each game VBlank with a real
display VBlank. If the game runs late, it records the miss and continues
without skipping game time; after a long stall, it resets its timing baseline
instead of trying to catch up in a burst ([`main.c`](../src/app/main.c)). When
game-side VBlank work is complete, [`kj_frame.c`](../src/runtime/kj_frame.c)
publishes a snapshot for drawing. A small fallback publishes frames such as
boot or error screens that do not reach the normal game hook. The renderer
reads the snapshot on the game thread at this boundary, so game writes do not
change halfway through a draw. The timing model is described in the code and
Coffeepot's [frame-pacing article](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/frame-pacing.md).

The published snapshot contains the DS graphics registers, palettes, sprite
attributes, video memory mappings, per-scanline register changes and the
latest 3D scene. [`kj_bridge.h`](../src/runtime/kj_bridge.h) defines this data;
[`kj_frame.c`](../src/runtime/kj_frame.c) maps the nine DS video-memory banks
from their control registers. In this way, the original game writes its
familiar DS memory model while the new renderer reads that state as data.

The render path has two parts:

1. **2D layers:** [`g2d.c`](../src/graphics/g2d/g2d.c) reads the two DS display
   engines' backgrounds, sprites, palettes, windows and blending state. It
   converts supported content into ordered GPU passes. Tiles are expanded
   through a texture atlas in [`atlas.c`](../src/graphics/resources/atlas.c);
   the atlas checks tile bytes and palette versions to decide whether a cached
   tile needs updating.
2. **3D scene:** [`kj_g3.c`](../src/runtime/kj_g3.c) interprets DS geometry
   commands, maintaining the matrix and lighting state the game expects.
   [`g3d.c`](../src/graphics/g3d/g3d.c) sends object-space vertices to the 3DS
   GPU, which transforms and draws them. Widescreen mode adjusts the view and
   visibility checks.

These renderers support many common modes, but not every DS graphics feature.
Unsupported 2D modes and 3D effects are counted in the renderer's diagnostics;
the code comments list examples such as 3D fog, toon/highlight shading, edge
marking and shadow volumes. A rendered intro or town therefore demonstrates
those scenes only, not complete graphics compatibility. See
[`g2d.c`](../src/graphics/g2d/g2d.c), [`g3d.c`](../src/graphics/g3d/g3d.c),
and Coffeepot's [2D rendering guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/rendering-2d.md).

The renderer is still incomplete. A feature being listed in the code comments
or project design does not mean the game uses it correctly in every scene.
The current code reports unsupported operations so they can be found during
testing rather than quietly substituting a software-rendered picture.

The original DS screens are 256 by 192 pixels. kohjoh scales the lower screen
to fill the 3DS bottom display (320 by 240). On top, it draws a wider 320 by
192 logical view, adding 32 columns on each side of the DS view, then scales it
to 400 by 240. The target sizes and offsets are set in
[`targets.c`](../src/graphics/pica/targets.c); the renderer also enables wider
3D visibility in [`renderer.c`](../src/graphics/renderer.c). A wider view
changes what is visible at the sides, so it is a presentation feature as well
as a screen-size change.

When the runtime falls two or more display refreshes behind, it can skip
recording a picture to the GPU to recover time. The game update has already
happened, and the skip is counted in the renderer; it is not an extra game
update or a claim that every update produced a distinct picture
([`renderer.c`](../src/graphics/renderer.c)).

Sound is mixed from the DS-compatible sound code and queued to the 3DS DSP by
[`audio.c`](../src/audio/audio.c). Logging uses a bounded memory buffer and a
worker thread to keep file writes away from the draw path
([`log.c`](../src/diagnostics/log.c)). Controls are translated from 3DS buttons
and touch coordinates into DS input in [`input.c`](../src/app/input.c).

## Content and installation formats

The repository does not contain the commercial game image or extracted game
assets. A content pack is made from a user's own compatible copy and is opened
by the runtime. The path code checks for a pack in read-only RomFS first, then
uses an edition-specific pack under `sdmc:/3ds/kohjoh/data/`
([`paths.c`](../src/storage/paths.c)). In the current `.3dsx` workflow, the
pack and raw save live on the SD card.

The design brief describes a future CIA with game data inside RomFS and saves
in the installed title's save archive. That is not yet a working build path:
the save implementation currently in source is the interim raw-file backend,
and the Windows guide builds `.3dsx` only. See [status](status.md) and
[`save_raw.c`](../src/storage/save_raw.c).

## What this architecture does not prove

Azahar is used to boot the game, run scripted input and inspect images. It is
helpful for integration work, but emulator frame timing does not establish how
fast the game runs on an Old 3DS. Likewise, an executable that links or a
recognizable opening scene is not evidence that every map, battle, save flow or
graphics effect works. The [status page](status.md) lists the current evidence
and the acceptance work still needed. Coffeepot explains this distinction in
its [hardware testing guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/testing-on-hardware.md).
