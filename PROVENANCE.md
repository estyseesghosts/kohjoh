# Provenance and licences

kohjoh is a new project (GPL-3.0, see `LICENSE`). It compiles the original
game from the pret-style decompilation and replaces everything that
determined the previous ports' performance: shell, frame scheduling, 2D and
3D rendering, storage and packaging.

Imported code is kept to the **game-compile layer** that kohjoh.md section 0 asks to
reuse ("verified, minimal compatibility logic"). Every import is listed here
with its source revision; local changes are listed per import.

## Not imported (rewritten for kohjoh)

| Area | kohjoh source | Replaces |
| --- | --- | --- |
| App shell, APT, input, pacing | `src/app/` | hgss-3ds `hgss/game/app/main_3ds.c` |
| Frame bridge (publish point, V-blank mapping) | `src/runtime/kj_frame.c`, patch 2001 | hgss-3ds drawer/compose threads |
| Geometry engine (GPU transform) | `src/runtime/kj_g3.c` | Platinum `port_gx3d*.c` (CPU transform/clip/raster) |
| 2D renderer (GPU tiles, stencil effects) | `src/graphics/g2d/`, `src/graphics/resources/atlas.c` | Platinum `port_render2d.c` (CPU compositor) |
| 3D renderer, DS textures | `src/graphics/g3d/` | Platinum `port_gx3d_pica.c`, `port_compose_pica.c` |
| Audio output | `src/audio/` | hgss-3ds shell audio |
| Logging, profiler | `src/diagnostics/` | hgss-3ds shell logging |
| Storage paths/backends | `src/storage/` | hgss-3ds shell paths |

## Imported

| kohjoh path | Source | Revision | Licence | Local changes |
| --- | --- | --- | --- | --- |
| `patches/NitroSDK`, `patches/NitroSystem`, `patches/pokeheartgold` (1001-1909) | hgss-3ds `patches/` | `a26c003` | GPL-3.0 | dropped 1910 (software-BG battle veto, renderer-specific); added 2001 |
| `src/compat/nitro/` (minus renderer files) | hgss-3ds `3ds_port/nitro` (vendored from pokeplatinum-3ds `359fbec`) | `a26c003` | GPL-3.0 | removed `port_render2d`, `port_gx3d*`, `port_wide*`, `port_lower*`, `port_options`, `port_shadow`, `port_upscale`, `port_xbrz`, tests, stb_image; new slim `include/nitro_port_gx3d.h`; `nitro_port.h` no longer includes the 2D renderer; `port_overlay.c` no longer includes `nitro_port_wide.h` |
| `src/compat/nitrosystem/`, `src/compat/pak/`, `src/compat/common.mk` | same | `a26c003` | GPL-3.0 | paths; `common.mk` exports TMP/TEMP for MSYS2 |
| `src/compat/hgss/` | hgss-3ds `hgss/compat` | `a26c003` | GPL-3.0 | none |
| `src/compat/stubs/` | hgss-3ds `hgss/game/stubs`, `hgss/game/app/cpp_runtime.c` | `a26c003` | GPL-3.0 | none |
| `tools/game/*.py` | hgss-3ds `tools/` | `a26c003` | GPL-3.0 | paths; bootstrap reads `upstream.lock`, generates headers in Python, refreshes MSYS copies safely; build split into DS-side/3DS-side runtimes and per-file shaders; incremental compile (-MMD + flag signatures) |
| `tools/rompack/*.py` | hgss-3ds `builder/` | `a26c003` | GPL-3.0 | none yet |

Fixed-point lighting/texgen formulas in `src/runtime/kj_g3.c` follow the
GBATEK-derived code in Platinum's `port_gx3d.c` (cited inline).

## Never in this repository

ROMs, extracted or converted game data, packs, saves, Nintendo SDK files,
built binaries (`.gitignore` blocks the common extensions).
