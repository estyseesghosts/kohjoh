# How kohjoh works

## What it is

kohjoh compiles the decompiled HeartGold/SoulSilver C and C++ game code for the
3DS, then connects that game to a replacement runtime. The current deliverable
is a Homebrew Launcher `.3dsx`; a self-contained user-built CIA remains a goal
in the design brief, not a documented current build product. ROM-derived data
is supplied by the owner and is not checked into this repository. See
[kohjoh.md](../kohjoh.md), [PROVENANCE.md](../PROVENANCE.md), and
[`hgss_game.py`](../tools/game/hgss_game.py).

The build has two ABI domains. Decomp/game and Nitro compatibility code compile
with the Nintendo DS-facing headers and four-byte enum assumptions. Platform
runtime code compiles against libctru. [`kj_bridge.h`](../src/runtime/kj_bridge.h)
is the plain-C boundary between them; [`hgss_game.py`](../tools/game/hgss_game.py)
assembles the distinct source sets. The pinned decomp, reference ports and
toolchain are listed in [`upstream.lock`](../upstream.lock).

## Runtime and frame flow

The 3DS app initializes platform services, input, graphics, sound and logging.
The original game loop advances simulation and waits on its normal DS VBlank
cadence. [`kj_frame.c`](../src/runtime/kj_frame.c) maps each DS VBlank to a
physical display VBlank and publishes one coherent snapshot after the game's
VBlank callback, sound update and wait-task queue. The snapshot includes DS
registers, palettes, OAM, VRAM bank mapping, per-line state and the completed
geometry scene. A grace path publishes boot/error/reset frames that do not
reach the normal publish hook. This avoids renderer/game-thread races and
preserves the game's cadence; it is not a claim of physical-console speed.
Background: Coffeepot's [frame pacing](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/frame-pacing.md)
and [threading](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/threading.md).

The DS memory model is represented by arrays and logical views rather than
literal DS addresses. The frame bridge exposes the I/O registers, palettes,
OAM, main/sub BG and OBJ memory, plus LCDC VRAM. VRAM control registers resolve
the nine banks into their active role and offset. This lets existing game code
write the memory model while the replacement renderer reads its current state;
see [`kj_frame.c`](../src/runtime/kj_frame.c) and the memory structs in
[`kj_bridge.h`](../src/runtime/kj_bridge.h).

## Graphics path

The 2D translator turns DS background and sprite state into PICA200 draw work.
Its source is under [`src/graphics/g2d`](../src/graphics/g2d); shared texture
atlas code is in [`src/graphics/resources/atlas.c`](../src/graphics/resources/atlas.c).
The recorded design includes text and affine backgrounds, OBJ sprites,
per-scanline spans, windows, blending and brightness effects. The tile atlas is
palette-expanded and uses exact tile-byte comparison plus palette versions for
validity; see [`atlas.c`](../src/graphics/resources/atlas.c) and Coffeepot's
[tile renderer notes](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/2d-tile-renderer.md).

The DS geometry command interface is interpreted by
[`kj_g3.c`](../src/runtime/kj_g3.c). It preserves CPU matrix-stack, lighting
and texture-generation work, but sends vertex transforms and rasterization to
the 3DS GPU through the 3D renderer in [`src/graphics/g3d`](../src/graphics/g3d).
The scene uses double-buffered vertex/index storage. Widescreen mode extends
horizontal view and widens visibility tests. The formulas and assumptions
should be read with the code comments and Coffeepot's [PICA renderer
architecture](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/renderer-architecture.md)
and [GPU rendering guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/gpu-rendering.md).

Audio, save/storage adapters, input, lifecycle, logs and diagnostics have their
own directories under [`src`](../src). The logging module uses a bounded ring
and a worker to keep SD writes off the render path
([`log.c`](../src/diagnostics/log.c)). Check the current implementation and
status before assuming a subsystem is complete.

## Current limits

Azahar is used for bring-up, scripted input and captures; it is not a timing
authority. Physical Old 3DS acceptance, full gameplay coverage and the planned
CIA distribution path remain unproven. The current display-list cache is an
experimental, gated optimization and must remain distinguished from the
verified cache-off state. See [status](status.md) and
[troubleshooting](troubleshooting.md).
