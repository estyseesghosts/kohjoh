# Field scaling contract (pixel-perfect.txt)

Status: foundation and native rasterization implemented; hardware validation
pending. All evidence below is **EMULATOR-only** (LOTUS/CITnLO, Vulkan) unless
explicitly marked O3DS. See `docs/status.md` for the project-wide evidence
rules.

## Modes

| Property | Legacy | Integer 1x | Integer 2x |
| --- | --- | --- | --- |
| World rasterization | 400x240 | 400x240 | 200x120 (offscreen) |
| DS -> surface scale | 1.25 | 1.0 | 1.0 |
| Surface offset (x,y) | (40, 0) | (72, 24) | (-28, -36) |
| Display | 400x240 | 400x240 | 400x240 (2x composite) |
| Interface | existing | existing | existing |

The canonical offsets are derived, not hardcoded:
`offset = (surface_dimension - scale * DS_dimension) / 2`, with a DS canvas of
256x192 (`src/graphics/world_view.c`). The visible DS bounds and clip
half-extents therefore follow from the same description:

| Mode | Visible DS x | Visible DS y | Clip half x/y |
| --- | --- | --- | --- |
| Legacy | -32..288 | 0..192 | 1.25 / 1.00 |
| 1x | -72..328 | -24..216 | 1.5625 / 1.25 |
| 2x | 28..228 | 36..156 | 0.78125 / 0.625 |

## Ownership

- `src/app/config.{c,h}` — one startup read of
  `sdmc:/3ds/kohjoh/config.txt`; pure, host-testable parser; safe legacy
  fallback; never touches SD during rendering.
- `src/graphics/world_view.{c,h}` — authoritative DS -> active surface
  mapping, visible bounds, clip extents, viewport/scissor math and cache key.
- `src/runtime/kj_field_view.{c,h}` — immutable per-scene field-view
  description, committed at frame publication before geometry production.
  Menus and battles get legacy; only the resolved field overlay gets the
  configured mode. Generation changes when the projection changes.
- `src/graphics/pica/targets.{c,h}` — physical display targets and the 2D
  interface projection; allocates the offscreen surface only for 2x.
- `src/graphics/pica/offscreen.{c,h}` — persistent 256x128 RGBA8 texture with
  a 200x120 active viewport and D24S8 attachment, created through
  `C3D_RenderTargetCreateFromTex()`; allocation failure falls back to legacy.
- `src/graphics/g3d/g3d.c` — `make_fixup()` uses the active world view and is
  recomputed per batch (no stale matrix across scene transitions).
- `src/runtime/kj_g3.c` — game-facing box tests and retained-display-list
  planes derive their extents from the field view (`clip_hx`/`clip_hy`) plus a
  guard margin; game-facing tests are never tightened below legacy.
  `script.txt` `debug: 0x800` disables only the visibility optimisation.

## Frame order (2x)

```
C3D_FrameBegin
  per top-screen BG0 slot:
    clear + bind offscreen target, scissor to the active region
    render world (200x120) through the existing 3D path
    rebind native top target, invalidate GPU bindings
    composite one nearest-sampled quad covering 400x240
    continue native-resolution interface composition
  draw bottom screen
C3D_FrameEnd
```

There is no CPU scaling, no readback, no per-frame allocation and no second
frame submission. The offscreen texture is never sampled in the pass that
renders into it.

## Verification performed

Native tests (`build/run-native-tests.sh`, all PASS on 2026-11-26):

- `config_test` — LF/CRLF, comments, whitespace, malformed and unknown values,
  last-valid-wins, length-delimited input, missing file.
- `world_view_test` — centres, corners, negative coordinates, clipping,
  portrait scissor rotation, non-fullscreen viewports, cache keys.
- `field_view_test` — field activation, menu/battle deactivation, generation
  changes, 2x bounds, A/B visibility switch, reset.
- `check_hotpaths.py` — full differential geometry/lighting/cache suite,
  including culled vs unculled visible-triangle equality.
- `visibility_test.py`, `test_touch_tp.py` — unchanged behaviour.

Block uniformity alone is insufficient (a black or mirrored frame would
score 100%). `tools/qa/check_2x_blocks.py capture.bgra --source field_1x.bgra`
also compares every 2x pixel with the independent 1x capture at
`(X/2+100, Y/2+60)` — the canonical mapping between the modes. This is the
`np.repeat(np.repeat(source, 2, 0), 2, 1)` comparison from Task 2D, but uses a
separate rendering of the same frame rather than a CPU enlargement of the 2x
texture. It fails on horizontal compression, mirroring, texel drift and
origin shift, not merely on block non-uniformity. Both captures must be at
the same game frame (`check_2x_matches_1x.py` remains as a standalone form of
the same check).

Build: `tools/game/check_patches.py` (106/106 patches) and
`tools/game/hgss_game.py --target 3ds --game heartgold` produced
`build/kj-fieldscale/pokeheartgold.3dsx` with no `error:` lines.

LOTUS (CITnLO/Vulkan, Goldenrod bench save, `goldenrod_field.script`):

- 1x: guest log shows `requested 1x, initialized 1x`, `world projection 1x`,
  and `[fieldview] scene 1 -> mode 1 ... clip 1.56250/1.25000`. Captured top
  screen at frame 3900 shows the expanded field and smaller sprites against a
  v0.0.5 legacy capture at the same scene.
- 2x: guest log shows the 128x256-backed surface allocated, `world projection
  2x` and `clip 0.78125/0.62500`. Same-frame captures at frame 4200 give:
  - `tools/qa/check_2x_blocks.py`: **24,000/24,000 (100.000%)** aligned 2x2
    blocks identical.
  - `tools/qa/check_2x_matches_1x.py`: **96,000/96,000 (100.000%)** 2x pixels
    identical to the 1x central crop at `(X/2+100, Y/2+60)`.
  This is the acceptance condition for "2x is 1x, just twice as large": it
  rules out compression, mirroring, origin shift and texel drift, not just
  uneven block sizes.

Root causes found and fixed (the two earlier 2x attempts were wrong for both
reasons at once):

1. The top target's world view was initialised with a 400x240 surface even in
   2x mode, so `make_fixup()` squeezed the whole DS screen into the offscreen
   texture instead of rendering a 1:1 200x120 crop. Both `targets.c` and
   `renderer.c` now use `kj_world_view_init_display()`, which selects the
   active 200x120 surface. `C3D_FrameDrawOn()` was also overrides with an
   explicit `C3D_SetViewport(0, 0, 120, 200)`; by default it installs a viewport
   over the whole 128x256 backing texture.
2. Citro3d's render-to-texture framebuffer maps the active region into the
   bottom 200 rows of the 256-row texture and reverses the DS-y axis. A
   template match against the same-frame 1x capture scored **1.000** for
   `flipy=true, dx=-56, dy=0`, which is exactly a 56-row offset (256-200) plus
   a vertical flip. The composite now undoes both with derived integer texel
   coordinates (`tc = 8*(240-phys_y)` for u, `tc = 8*(phys_x+112)` for v), not
   hand-tuned half-texel offsets. This also removed the black left band: it
   was the 56 unpainted rows of padding being sampled, not missing geometry.

## Known limitations / unresolved work

These are tracked honestly rather than claimed as complete:

- **2x composition of alpha/stencil**: the current composite writes an opaque
  quad at BG0's position. Translucent BG0 texels, lower-priority layers seen
  through them, window/brightness flags and scanline-dependent effects are not
  yet reconstructed exactly. Per Task 11 the 2x compositor must not be called
  complete until that is demonstrated.
- **2x world coverage**: resolved. The black left band was the 56 rows of
  unused texture padding being sampled because the render-to-texture viewport
  is bottom-aligned in the backing texture; it was not missing world geometry.
  Correcting the composite origin gave 100% exact pixel equivalence and no
  visible band. `debug: 0x800` (visibility culling off) was used to rule out
  retained-list culling during diagnosis.
- **2x area banners / late 2D layers**: `composite_world_2x()` used to leave
  an open scissor, the offscreen texture bound and its UV scale installed.
  OBJ layers drawn afterwards (location banners such as `Route 34`) sampled
  with the wrong transform and were garbled. The call site now calls
  `restore_2d_state(c, sx0)` after compositing. A/B in LOTUS with the same
  walk script and frame confirms it: without the restore the top banner is a
  corrupted dark strip; with it, `Route 34` renders clean at native
  resolution while the world beneath stays exactly 2x.
- **1x pop-in / four-way culling diagnosis**: all four variants were captured
  in LOTUS with the same `goldenrod_walk.script` at frames 3700, 3800, 3900,
  4000 and 4100 (`A` = both on, `B` = retained-list culling off via
  `debug: 0x800`, `C` = `BOX_TEST` bypassed via `debug: 0x1000`, `D` = both
  off). Per frame, differing bytes out of 384,000 were:
  `A vs B = 0, 0, 0, 0, 0`; `A vs C = 90829, 90428, 90188, 124951, 63035`;
  `A vs D` equals `A vs C`.
  So the retained-list optimisation is byte-identical and is not a source of
  pop-in. The `BOX_TEST` bypass changes the picture, but it is a **confounded
  result**: a global bypass also changes game-side object decisions (the
  station sign and the fountain disappear), which is exactly why the spec
  requires a narrowly scoped, call-site-specific render-visibility bypass.
  The correct next step is to log stable prop identifiers at each game-facing
  `GF3d_ModelBoxTest()` call site rather than to draw conclusions from variant
  C/D. Manual 1x play in the latest build showed no pop-in.
- **Second-location regression**: a Cianwood save was generated with
  `hgss-save location set` from the verified safe-spawn registry and staged in
  both LOTUS sessions. Same-frame captures at frame 4200 again give
  **24,000/24,000** uniform 2x2 blocks and **96,000/96,000** pixels identical
  to the 1x central crop. This confirms the mapping is not specific to
  Goldenrod.
- **Temporal stability (Task 12, integer-grid part)**: `goldenrod_walk.script`
  was run in both modes and compared at frames 3700, 3800, 3900, 4000 and
  4100 while the player walks south. At every frame: 24,000/24,000 uniform
  2x2 blocks and 96,000/96,000 pixels identical to the independent 1x capture.
  The exact mapping therefore holds during movement, not only on a still
  frame. Because 2x is already pixel-identical to 1x scaled 2x, it introduces
  no sprite distortion of its own; optional billboard snapping experiments
  would change both modes together and have not been performed.
- **Battle / menu isolation**: with the 2x config active, `trainer_battle.script`
  reaches a trainer battle at frame 4200. The top-screen capture is
  **byte-identical (0 / 384,000 bytes)** to the same script on the v0.0.5
  baseline build. Battles, their windows and their text therefore inherit no
  field scaling. This also exercises window/blend composition on a native
  layer, but does not by itself prove translucent BG0 composition in 2x.
  Game-facing `BOX_TEST` call-site tracing, map-cell residency and billboard
  alignment remain open, so Tasks 8/9 are not claimed complete.
- 1x map residency (Task 8), scripted cinematic framing (Task 10), pixel
  stability (Task 12), O3DS performance (Task 13) and both-edition regression
  matrix (Task 14) remain open.
- The field-view policy currently follows the resolved overlay screen id;
  field-hosted scripted sequences inherit the field mode until scene-specific
  framing profiles are added.
- No O3DS timing claim is made. Emulator frames are not hardware benchmarks.
