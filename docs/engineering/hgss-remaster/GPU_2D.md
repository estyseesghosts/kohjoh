# GPU_2D — DS two-dimensional video semantics mapped to the PICA200

**Status:** implementation design; GPU-only runtime. Requires [FONT_RENDERER](FONT_RENDERER.md), [GPU_RESOURCES](GPU_RESOURCES.md), [FRAME_PIPELINE](FRAME_PIPELINE.md).

## 1. Target and scope

Top: native 400×240, with 320×192 logical canvas (original 256×192 center plus 32 columns each side). Bottom: native 320×240 with 256×192 logical canvas unless a specific remaster layout expands content. Preserve DS engine/priority semantics and the original hardware state as *input*, not as an intermediate pixel framebuffer.

Use [Coffeepot's 2D guide](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/rendering-2d.md), [worked tile renderer](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/2d-tile-renderer.md), [porting guide](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/porting.md#64-porting-a-ds-style-tile-and-palette-renderer), and [GBATEK](https://problemkaputt.de/gbatek.htm) for original 2D effects. Existing port `port_render2d.c` is a **reference/call-graph source**, not the new drawing architecture.

## 2. Translate state, not pixels

`Ds2dSnapshot` is a **proposed interface** comprising immutable resolved graphics registers, VRAM bank mappings, BG mode/map/char base/scroll, affine parameters, palette/ext-palette generations, OAM, window/mosaic/blend/capture control and scanline-event stream. Build one `RenderPlan` per frame from this snapshot; never read live registers/VRAM again after claiming a particular frame generation.

Required renderer tasks:

| DS feature | PICA path | Dirty/invalidation and danger |
| --- | --- | --- |
| BG tilemaps (text/affine/extended modes) | Resident atlas pages, visible tile quads, transforms | Character/map bytes, palette, bank mapping, mode, wrapping and scroll |
| OAM sprites | Sprites as quads with affine/mosaic/flip and ordered priority | Per-OBJ attributes, tiles, palette, object-window mode, visibility |
| 4/8 bpp indexed palettes | Decode affected tile+palette combinations into direct color | Changing one palette bank changes *every* dependent cached tile |
| BG/OBJ priority | Ordered draw groups/passes with explicit tie semantics | Equal-priority sprites/BGs and semi-transparent interaction |
| Window0/1/OBJ window | Stencil where supported; bounded scissor for rectangles | Per-layer enable masks, overlapping window membership, outside region |
| Alpha/brightness | TEV, blend factors, global brightness pass where correct | Source/destination ordering, target 1/2 selection, per-mode equations |
| Mosaic | Quantized source coordinates or precomputed material variants | Per-BG/OBJ enable, size and alignment origin |
| HBlank changes | Merge consecutive identical effective state into spans | Exact line boundaries; no arbitrary 8-line approximation |
| Display capture | GPU target ping-pong and explicit source version | Never sample target while writing it; do not read back to ARM11 |
| Bitmap / mixed windows | GPU-owned window render target, original op replay | A normal text-only glyph overlay would miss pixel operations |

Neither PICA nor citro2d implements DS palette lookup directly. **Do not invent a PICA fragment shader** to perform arbitrary per-texel indexing. Predecode affected tiles, keep a palette-version dependency graph, and measure worst-case full palette changes separately.

## 3. Tile atlas design

A tile cache entry must contain content identity **and** effective palette, mode and mapping: `(VRAM region identity, byte generation, tile number, palette generation, palette bank, bpp, flip relevant to packing, texture format)`. Derived color tiles are immutable once published. A repeated map may change which tiles are visible without changing the atlas; avoid data copies on camera movement.

- Build atlas pages from 8×8 DS tiles or grouped 16×16 art depending on transfer and batching experiments.
- Maintain reverse dependencies from palette and VRAM ranges to pages; dirty tiles are refreshed in a staging page, not modified while in flight.
- Camera movement changes a compact visible-instance buffer; static map chunks may retain geometry. Animate tile selection using frame IDs and atlas coordinates where possible.
- Preserve nonrectangular BG shape, map wrapping and affine reference coordinates. Bounds checks must reflect the original engine mode.
- **Do not sort translucent sprites globally by texture.** Only reorder within proven order-insensitive runs. Sprite draw order within priority has defined original semantics.

The local O3DS Coffeepot tile benchmark reports approximate CPU times for its 416-tile synthetic setup: direct citro2d 1.497 ms, manual vertices 1.019 ms, retained chunks 0.899 ms, cached surface 0.868 ms. Read exact setup and results; cached surfaces cost extra VRAM and have redraw spikes. **Proposed baseline:** retained chunks + dynamic tiles/OBJ geometry, with selective surfaces only for static content.

## 4. Widescreen policy

The remaster top-screen world uses x in the range `-32..288` in original DS logical units, centered around the original `0..256` world. The scale is `physical = 1.25 × logical`, with a +32-logical extension each side. **Proposed** per-scene policy:

- 3D overworld: widen the horizontal camera/frustum and culling bounds, not a 256→400 stretch.
- 2D screen-space HUD: explicitly position within 320 logical columns; original widgets remain pixel-precise at original logical positions unless remaster layout approved.
- Tile backgrounds: extend map sampling only where original map data exists. Never mirror arbitrary border tiles without a reviewed art rule.
- Full-screen art/cutscenes: pillarbox, rebuild backgrounds, or draw authored side panels; choose by scene, record in correctness matrix.
- Bottom: keep original 256 logical columns and touchscreen coordinate conversion; remaster bottom layouts are optional and separately tested.

The 3DS top screen's ordinary framebuffer is 400×240 logical, **not** the optional 800×240 wide pixel mode. Avoid double-pixel mode until proven affordable.

## 5. Compositing order is a correctness requirement

Use a typed ordered display list: `BeginTarget`, `SetClip/WindowMask`, `SetMaterial`, `BG/OBJ/3D/window glyph commands`, `ApplyEffect`, `Capture`, `EndTarget`. Every command carries a priority/order field, current mask and target generation. Batch adjacent compatible runs but **do not reorder across different window mask, blend destination, depth or interleaved capture dependencies**.

Important cases to freeze as images and state traces: overlapping sprites of equal priority; translucent OBJ above BG; alpha window edge; text box overlapping a character; affine/flip OBJ; brightness fades; palette cycling; BG3D/2D integration; palette change partway through a frame.

If hardware stencil/scissor cannot reproduce a DS window effect in one pass, explicitly decompose it into GPU passes with bounded cost. No CPU mask rasterization in the runtime. *Unsupported* means the scene has not met a milestone.

## 6. Scanline/HBlank effects

Use two clocks and two representations:

1. **Simulation side:** execute original HBlank/VBlank-visible behavior to preserve game/IRQ semantics. Not all interrupt work can be elided as mere graphics effects.
2. **Rendering side:** record a compact `ScanlineState[192]` or event stream containing only effective graphics values actually consumed by the rendering path. Merge consecutive *equivalent* spans and replay GPU operations for those spans.

Do not include nonvisual timer fields such as VCOUNT/DISPSTAT in a graphics-state change signature unless the renderer actually consumes them; do not remove them from the emulated hardware behavior. The old HGSS repo explicitly encountered this invalidation bug. Compare each optimized event walk against an unoptimized interpreter over the same input sequence; preserve frame-boundary alignment and actual state writes.

A worst-case 192-span effect can explode draw calls. Benchmark `G2D-006` with 1, 8, 32, 96 and 192 spans. Consider specialized effect geometry or staged target composition rather than assuming every span needs a completely new draw. Never merge spans with different effective windows, blending or scroll behavior.

## 7. Display capture and targets

Any capture of previous color requires an explicit distinct buffer (ping/pong or generation-stamped target). Model producer/consumer epochs and fence retirement. The capture pass must specify original vs native remaster resolution, scaled coordinates and cropping. Deferred captures must not accidentally sample a later frame. Avoid changing targets repeatedly: Coffeepot and citro3d source note a target switch can force expensive GPU setup/flush work.

When a screen has no visible changes, keep its last published target; no redundant clear, recapture or transfer. **Test independently**: the bottom's target can be unchanged while the top is busy. Note that frame transfer and swap policies must preserve both displays; see [FRAME_PIPELINE](FRAME_PIPELINE.md).

## 8. GPU backend implementation notes

- Use citro3d for explicit GPU command lists and shader/attribute state. citro2d is allowed for tool overlays and well-ordered UI primitives, provided batching limits and context handoff are understood.
- Each hot path should use a small fixed command representation rather than megafunction `switch` dispatch.
- Prefer packed vertex attributes (positions and UV in 16-bit/byte formats where accuracy allows). PICA vertex shaders convert/scaling packed values.
- Retain texture/program binding in renderer state: citro3d marks textures dirty even when rebound to themselves, potentially invalidating texture cache.
- Atlas sizes, mipmapping, texture sampling and swizzle conversion must be validated with a small test shader and readback **offline only**, not in the runtime hot path.
- Use hardware memory fill and transfers only where layout/format/ownership makes them cheaper than the alternative.

## 9. Test ladder

| ID | Repro | Hard gate |
| --- | --- | --- |
| G2D-001 | Static maps, scroll and affine BG | Tiles, palette and seams correct; no full-screen CPU raster path |
| G2D-002 | 40+ OAM sprites, affine/flip, equal priority | Exact overlap ordering; stable p99 frame times |
| G2D-003 | Palette changes + animated tiles | Only dependent resources invalidated; correct image |
| G2D-004 | Multiple windows and masked sprites | Pixel-correct priority/masking against approved reference |
| G2D-005 | Fades, alpha, mosaic, screen capture | GPU-only effects; correct target generations |
| G2D-006 | 1→192 HBlank spans | Exact boundaries; no skipped game-visible IRQs; quantified worst case |
| G2D-007 | Full-width overworld and HUD | Both screens correct; no fabricated scenery or stretched text |
| G2D-008 | Unchanged bottom for 600 ticks | No bottom draw/clear/transfer, except services genuinely required by hardware |
| G2D-009 | Mixed text+bitmap windows | Command order preserved, no glyph-only overlay corruption |
| G2D-010 | Real O3DS Goldenrod scene | 30 FPS within frame deadline **after** complete game integration |

**M1 prototype pass:** capture replay reproduces representative GPU-only 2D frames; performance is measured on real O3DS. `G2D-010` is a later full-game release gate, not achievable simply by a standalone 2D test.

## 10. References

- `C:\Users\terezi\workdir\coffeepot\ctr\engineering\PICA200\rendering-2d.md` ([online](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/rendering-2d.md)).
- Same root: `ctr\engineering\PICA200\2d-tile-renderer.md`, `ctr\engineering\PICA200\renderer-architecture.md`, `ctr\engineering\PICA200\gpu-rendering.md`.
- [HGSS original graphics/window code](https://github.com/pret/pokeheartgold/blob/master/src/bg_window.c), [GBATEK](https://problemkaputt.de/gbatek.htm), [3dbrew GPU pages](https://www.3dbrew.org/wiki/GPU), [Platinum renderer study](https://github.com/GabeSud0/pokeplatinum-3ds).
