# How kohjoh relates to hgss-3ds and pokeplatinum-3ds

The repository sometimes shortens “pokeplatinum-3ds” to “Platinum-3DS.” It is
a reference project, not the game this repository ports. kohjoh runs the HGSS
decompilation and borrows selected build and compatibility code from the
earlier projects. The exact source revisions and changes are documented in
[`PROVENANCE.md`](../PROVENANCE.md) and [`upstream.lock`](../upstream.lock).

| Area | Earlier project | kohjoh |
| --- | --- | --- |
| Game | hgss-3ds ports HeartGold/SoulSilver; pokeplatinum-3ds ports Platinum. | Compiles the pinned HGSS decompilation. It does not combine or port Platinum's game logic. |
| Build and compatibility | The earlier HGSS port contains a useful game compiler and Nitro compatibility layer; the vendored Nitro code traces through hgss-3ds to pokeplatinum-3ds. | Reuses selected tools and compatibility code, with local build changes. The source ledger lists what was kept, removed and modified. |
| Application and frame flow | The earlier port's shell and drawer/compose scheduling are not used as kohjoh's runtime. | Has a new 3DS app shell and a frame publish point. The game loop remains the owner of updates; a bridge publishes coherent DS state at the frame boundary ([`main.c`](../src/app/main.c), [`kj_frame.c`](../src/runtime/kj_frame.c)). |
| 2D graphics | Earlier port renderer code is excluded from the vendored compatibility layer. | Uses a new PICA200 renderer for supported DS backgrounds, sprites and effects ([`g2d.c`](../src/graphics/g2d/g2d.c)). |
| 3D graphics | The Platinum `port_gx3d` implementation served as a semantic and code reference; its CPU transform/clip/raster approach is not the kohjoh draw path. | Interprets DS geometry commands and keeps DS state calculations needed by the game, while the PICA GPU transforms and draws vertices ([`kj_g3.c`](../src/runtime/kj_g3.c), [`g3d.c`](../src/graphics/g3d/g3d.c)). |
| Other runtime pieces | Prior app shell, storage, audio and diagnostics informed the port. | kohjoh supplies its own app, audio, storage paths, logging and profiling code. See [`PROVENANCE.md`](../PROVENANCE.md). |

The goal of the rewrite is to move expensive graphics work onto the 3DS GPU
while keeping game behavior in the original HGSS code. The code does not
translate every DS drawing operation directly: it builds ordered GPU work for
the DS effects it supports. This is consistent with Coffeepot's
[PICA200 renderer guidance](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/renderer-architecture.md)
and [2D rendering notes](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/rendering-2d.md).

These differences describe source and architecture, not a claim that kohjoh
is faster or more compatible today. Azahar results and remaining support gaps
are listed in [project status](status.md).
