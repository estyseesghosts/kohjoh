# Wide layout rules on the PICA (issue #16)

Unown's MOVE/FILL/COPY/WRAP/OBJS/SHIFT3D rows (ss3ds_widelayouts.h) now drive
Kohjoh's retained GPU layers instead of a CPU compositor.

- `src/runtime/kj_layouts.c`: per-screen rows, ported from Unown; published via
  `kj_screen_state.rows[engine]` with the #15 generation.
- `src/graphics/layout/layout.c`: per BG and line band, Unown's buildRow()
  column map cut into segments of constant offset or one repeated column.
  Host test: `tests/native/layout_test.c` (each kind, scroll/picture coords,
  wrap, shipped tables: no holes, no overlaps).
- `g2d.c`: under a layout, text BG quads cover screen columns -64..320 and are
  drawn once per segment, translated or stretched by the 2D xform under a
  scissor (coffeepot HGSS_3DS_ENGINEERING_GUIDE.md: "compact runs or scissored
  strips with per-run state"). OBJS moves sprites by their middle at quad
  emission (OAM untouched; retained OBJ key includes the generation). SHIFT3D
  offsets the target passed to g3d only (no DS matrix/viewport/box-test change).
  Affine BGs keep 256 columns and are counted. Stats log `layout segments`.

Verified: LOTUS kj-prof, party screen (engine B top, 6 rows) at frame 7258:
panel frame at both 400 px edges, wave continued, no seams
(`2026-11-17-party-wide.png`). Summary (24 A rows: MOVE+COPY+FILL+OBJS+SHIFT3D)
at frame 7518: memo panel widened with dotted lines continued, name/sprite/3D
Cyndaquil at the right edge (`2026-11-17-summary-wide.png`). Field/battle (no
rows) unchanged.
