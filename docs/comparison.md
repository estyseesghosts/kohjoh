# How kohjoh differs from hgss-3ds and platinum-3ds

kohjoh is a targeted rewrite of the 3DS port runtime around the original HGSS
game logic. It reuses selected build and compatibility pieces, while replacing
the render and platform paths that determine frame cost. The exact import
ledger is [`PROVENANCE.md`](../PROVENANCE.md); source pins are in
[`upstream.lock`](../upstream.lock).

| Area | kohjoh | Relationship to earlier ports |
| --- | --- | --- |
| Game logic | Compiles the pinned pret-style HGSS decompilation with a maintained patch stack. | Retains the original game behavior and much of the useful game-compile layer from hgss-3ds. |
| Build layer | Reuses selected compiler/build scripts and compatibility sources, with Windows path, temp, incremental and packaging fixes. | Not a wholesale copy of hgss-3ds. Provenance lists each imported path and revision. |
| Frame scheduling | Replaces the drawer/compose-thread arrangement with a publish point and one DS-VBlank-to-physical-VBlank bridge. | New runtime in [`kj_frame.c`](../src/runtime/kj_frame.c); designed to publish coherent game state at the frame boundary. |
| 2D | New PICA-oriented tile/sprite compositor using atlas, ordered passes, windows and stencil/blending. | Replaces the earlier CPU compositor path. See [`src/graphics/g2d`](../src/graphics/g2d). |
| 3D | Keeps matrix stack, lighting and texgen semantics on CPU; sends vertex transforms and rasterization to PICA200. | Replaces the CPU-heavy geometry/raster path attributed in the project notes to Platinum's `port_gx3d` family. See [`kj_g3.c`](../src/runtime/kj_g3.c). |
| App services | New shell, input, audio output, logging/profiling and storage paths. | Replaces prior port shell behavior; per-file mapping is in [`PROVENANCE.md`](../PROVENANCE.md). |

The performance motivation recorded in the worklog was to avoid carrying the
old CPU transform/clip/raster pipeline into this port. The design also follows
Coffeepot's advice to rebuild work units around PICA capabilities rather than
translate every legacy graphics call one-for-one: [renderer
architecture](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/renderer-architecture.md),
[2D rendering](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/rendering-2d.md).

The name “platinum-3ds” in the build notes refers to the
`pokeplatinum-3ds` reference repository pinned in [`upstream.lock`](../upstream.lock).
It supplied selected Nitro compatibility/build material and the older geometry
implementation used as a semantic reference; kohjoh does not port the Platinum
game. See [`PROVENANCE.md`](../PROVENANCE.md) for exact imported and rewritten
areas rather than inferring lineage from directory names.
