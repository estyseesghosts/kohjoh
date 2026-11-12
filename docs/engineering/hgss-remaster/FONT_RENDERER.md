# FONT_RENDERER — faithful HGSS text, native-resolution GPU glyphs

**State:** design + implementation contract. **Hardware target:** O3DS. **Dependencies:** [GPU_2D](GPU_2D.md), [GPU_RESOURCES](GPU_RESOURCES.md), [ASSET_BUILDER](ASSET_BUILDER.md), [CORRECTNESS_MATRIX](CORRECTNESS_MATRIX.md).

## 1. What we are actually replacing

**Documented in upstream HGSS:** `src/render_text.c` calls `FontID_TryLoadGlyph`, then `CopyGlyphToWindow`, advances the cursor by `glyphInfo->width + letterSpacing`, and controls waits, down-arrow, scrolling and per-glyph timing. See [render_text.c](https://github.com/pret/pokeheartgold/blob/master/src/render_text.c), [font_data.c](https://github.com/pret/pokeheartgold/blob/master/src/font_data.c), [text.c](https://github.com/pret/pokeheartgold/blob/master/src/text.c), [bg_window.c](https://github.com/pret/pokeheartgold/blob/master/src/bg_window.c). In `font_data.c`, the glyph representation is made of 8×8 tiles in up-to-16×16 glyph shapes; variable-width metrics come from font data.

`CopyWindowToVram`, `ScrollWindow`, `FillWindowPixelBuffer`, `ScheduleWindowCopyToVram`, `PutWindowTilemap`, `ClearWindowTilemap` and related direct pixel-buffer operations are essential to correct visible output. **Do not globally stub `CopyGlyphToWindow` or `CopyWindowToVram`.** A window can contain both text and other graphics, and its pixel buffer is application-visible state.

**Current expensive path:** glyph decoding → per-pixel glyph writes into DS-tiled window → tile/VRAM upload → DS 2D reconstruction → scaling/composition. **New target:** retain original game text semantics → generate glyph/mutation events → native atlas quad rendering on PICA200. Normal steady-state text must not require CPU rasterization or a rebuilt window texture.

## 2. Which text behavior belongs to the game

Preserve without rewriting the text state machine:

- Control code parsing; text arguments, substitutions, variable text and player names.
- Exact glyph IDs, advance widths, letter/line spacing, alignment and wrapping decisions; do not apply native metric rounding before the original engine decides layout.
- Character-by-character appearance, delay counters, sound triggers, button prompts and input waits.
- Down-arrow placement/animation, scroll per step, clear/reuse of windows, pause/resume and cancellation.
- Inline palette/foreground/shadow/background changes and special glyphs/icons.
- Japanese and other region-specific character sets, Unown and alternate font IDs; do not test only ASCII/English.
- Original Z/priority ordering relative to sprites, BGs, window masks and fades.

The renderer receives **resolved glyph events**, not a UTF-8 reparse of the original script. UTF-8 conversion, custom BCFNT and Unicode fallback are separate tooling/UI concerns.

## 3. Recommended model: a window surface with an optional retained glyph layer

A window is an owned logical object, with `window_id + generation`, placement, clip, scroll, tilemap binding and ordered mutations. The font layer is an optimization **inside that object**, not an unrelated always-on-top overlay.

Suggested events, not fixed ABI names:

| Event | Arguments | Required result |
| --- | --- | --- |
| `Glyph` | window generation, font, glyph ID, logical x/y, original colors, order index | Append ordered quad(s) to window text runs; preserve advance/timing from game |
| `Fill` | rectangle or entire window, palette index, order | Remove/occlude affected old glyphs; update logical surface |
| `Scroll` | pixel rectangle, dx/dy, fill color, order | Move/crop retained text geometry **and** other window content; remove clipped glyph fragments |
| `Blit` | bitmap/window source, source/dest rect, order | GPU image command or invalidate to GPU-backed mixed window surface |
| `Copy/Publish` | window generation, intended BG/VRAM binding | Commit coherent version into ordered screen composition |
| `Destroy` | window generation | Retire runs and target after GPU fence; invalidate references |
| `Palette` | old/new palette generations | Rebind/material update or regenerate only affected colored atlas entries |

For text-only windows: draw glyph runs directly into the appropriate layer, with exact clipping and ordered shadow/foreground representation. For windows with arbitrary interleaved pixel operations: use a **GPU render-to-texture surface** and replay ordered commands into it; keep an owned logical backing representation only if game code genuinely reads it. This is not permission to rasterize text on the CPU.

Before implementing, run a call-site audit for every writer and every reader of `Window.pixelBuffer` and the BG/window-char data. Classify whether the caller observes *pixels*, only *commands*, or only *state*. Maintain a coverage ledger; unknown calls fail loudly in debug builds. A global hook is not safe until this audit is complete.

## 4. Crisp 1.25× remaster fonts

DS logic/layout remains in 256×192 units. The upper 400×240 world corresponds to a 320×192 logical canvas: retain native font metrics and scale the resulting physical positions by 5/4. The lower display is 320×240 with the same 5/4 factor.

**Do not stretch a rendered 256×192 text picture.** Extract original glyph masks and produce *new native glyph art offline* while retaining the original logical advance table. Recommended candidate set:

1. **Faithful native glyphs:** pre-rasterize original masks into destination-sized texels with a controlled fixed-point 5/4 rule; intentionally place stems and baselines.
2. **Grid-fitted remaster glyphs:** hand-tune at least Latin, digits and punctuation first, with identical logical advances and a fall-through to faithful scaled masks for the full source set.
3. **Smooth pre-rasterized glyphs:** evaluate a limited coverage-anti-aliased variant for display readability; no runtime SDF shader or fragment-stage computation.

Keep the original HGSS shape as the reference, not the 3DS system font. For each glyph, store the original width/height/baseline and a destination quad/UV rectangle. **Map cumulative logical pen positions to physical positions**; never round every glyph advance and then accumulate drift. Use explicit policies for snap phase, half-pixel translation, clipping, shadow and foreground. Compare on real LCDs (hardware screenshots alone cannot show panel filtering).

An optional four-phase X-position variant may reduce uneven stem thickness for the 5/4 scale, but increases atlas size. It remains experimental until readability and memory tests show it helps.

## 5. citro2d: where it fits and where it does not

`C2D_TextBufNew`, `C2D_TextFontParse`, `C2D_TextOptimize`, `C2D_DrawText`, custom BCFNT fonts and sprite drawing are valid fast GPU routes. Coffeepot has a [worked text example](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/rendering-2d.md#text); upstream [citro2d text.c](https://github.com/devkitPro/citro2d/blob/master/source/text.c) is the API behavior authority.

`C2D_TextOptimize` sorts glyphs by sheet; do not assume reordering is visually neutral for overlapping glyphs, text effects or multi-colour runs. For HGSS use the library in **benchmark mode** against a custom glyph-atlas renderer. The custom route is favored because it handles HGSS glyph IDs, exact metrics, clipping and original palette behavior directly. citro2d remains useful for tooling, debug labels and menus not authored as DS window graphics.

## 6. Atlas and material design

- Build at least a direct-colour RGBA5551 sharp-edges candidate and an A8/LA8 coverage-mask candidate. Verify effective texture sampling/TEV/blend behavior in real hardware tests. Never assume the PICA has a general fragment shader.
- Pack commonly used glyphs into **few** 3DS-tiled texture pages. Common dialogue should normally bind one page per compatible run; do not bind once per glyph.
- Shadow and foreground can be precombined into precolored common palettes, or separate ordered mask passes. A one-pass two-mask TEV formulation is a **hypothesis** until a picture-equivalence test proves it.
- Atlas rows require padding for texture filtering and page boundaries. Use nearest sampling for grid-fitted bitmap glyphs unless measured smooth coverage is deliberately selected.
- Font assets never require CPU glyph decompression while printing; lazy paged font loads are permissible but should occur *before* visible dialogue when possible.
- Track `font_asset_hash`, `glyph_id`, `variant`, `palette_generation`, and `atlas_page_generation`. Eviction cannot overwrite a page used by an in-flight GPU command list.
- Preserve glyph and material order. Batch adjacent compatible commands; do not reorder across clipping, transparency, palette change or overlapping shadow semantics.

For a 512×512 atlas the nominal storage is 256 KiB in A8, 512 KiB in 16-bit formats, and 1 MiB in RGBA8. These are **capacity calculations**, not allocations promised to fit. More than one atlas might be needed across languages and font sets. See [Coffeepot texture formats](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/rendering-2d.md#what-the-gpu-accepts).

## 7. Dirty behavior and window lifecycle

- `Print`: append glyph at original logical position; update only the affected run metadata.
- `Wait`: preserve current frame with zero glyph preparation or upload.
- `Scroll`: transform/crop GPU text and other window contents exactly like original per-step scroll, with correct exposed fill.
- `Clear`: change window generation and invalidate old runs and any cached resolved surface.
- `CopyWindowToVram`: publish a coherent window generation. Coalescing multiple publishes is legal only if the game/screen cannot observe the intermediates.
- `Dispose/reuse`: increment generation to prevent stale glyph commands from appearing in a newly allocated window with the same index.
- `Readback by game logic`: if observed in source, maintain a compact **logical** shadow/window data representation or redesign the read, never solve with GPU-to-CPU frame copies. Test separately.

## 8. Prototype experiments and numerical gates

Make `fontbench` independent from game bootstrap. Reproduce original glyph IDs, positions, delays, run colors and window ops from recorded game inputs. Five candidates: original offline reference; citro2d custom BCFNT; custom uncolored A8; precolored atlas; GPU cached mixed window surface.

| ID | Test | Pass condition |
| --- | --- | --- |
| FNT-001 | Complete dialogue with variable names and control codes | Identical logical text event sequence/timestamps to DS reference |
| FNT-002 | Typewriter printing + arrow prompts | Correct partial visibility, prompt updates and scrolling |
| FNT-003 | Text scroll/clear and window reuse | No ghosts, stale resources, seam artifacts or layout drift |
| FNT-004 | Palette changes, shadows, Unown, regional fonts | Correct shapes, ordering and fallback coverage |
| FNT-005 | Mixed bitmap/text window and interleaved operations | Output matches approved native-resolution reference |
| FNT-006 | Finished static dialogue, 600 ticks | **0 glyph decompressions, 0 font uploads, 0 layout rebuilds** in steady state |
| FNT-007 | Ordinary dialogue frame on O3DS | **Proposed** CPU font-render budget ≤0.5 ms excluding game text logic; record p50/p95/p99 |
| FNT-008 | Visual comparison on physical 3DS screen | Correct original widths, readable strokes, no cumulative drift or uneven lines |

Where a GPU target is still redrawn every frame for reasons outside text, FNT-006 permits reissuing retained glyph draws; it does not permit re-rasterization or reupload. FNT-007 is a **goal**, not a reported achievement. Include memory/VRAM costs and call counts with every benchmark.

## 9. First implementation sequence

1. Freeze original text interpreter / window call graph and font reference assets; instrument every window mutation and reader.
2. Extract per-glyph source masks and original metric/control metadata offline; produce direct-texel atlas candidates.
3. Replay static and typing dialogue into a standalone O3DS GPU target; compare text geometry and screenshots.
4. Add scroll, fill, dirty-window and mixed-content handling, with order IDs and clipping.
5. Integrate original `RenderText` with a narrow adapter, leaving state-machine timing intact.
6. Remove any CPU font pixel path from release link graph; compare `fontbench` with full game workloads.

## Sources and local references

- [HGSS render_text.c](https://github.com/pret/pokeheartgold/blob/master/src/render_text.c), [font_data.c](https://github.com/pret/pokeheartgold/blob/master/src/font_data.c), [bg_window.c](https://github.com/pret/pokeheartgold/blob/master/src/bg_window.c), [text.c](https://github.com/pret/pokeheartgold/blob/master/src/text.c).
- [citro2d text API](https://github.com/devkitPro/citro2d/blob/master/include/c2d/text.h) and [implementation](https://github.com/devkitPro/citro2d/blob/master/source/text.c).
- Coffeepot local: `ctr\engineering\PICA200\rendering-2d.md`, `ctr\engineering\PICA200\asset-pipeline.md`, `ctr\benchmarks\README.md` beneath `C:\Users\terezi\workdir\coffeepot`.
