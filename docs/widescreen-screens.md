# Widescreen: per-screen state (issue #17)

Built-in rows: `src/runtime/kj_layouts.c` (ported from Unown's
`ss3ds_widelayouts.h`). Engine: `src/graphics/layout/` (issue #16).
Development override: `sdmc:/3ds/kohjoh/wide_layouts.txt` (Unown's format;
see `src/graphics/layout/override.c`). A screen listed there uses only its
override rows; the log line `[layout] ...: N screens, N rows, N lines ignored`
reports the parse. Diagnostic builds log `layout segments` and count affine
BGs under a layout as `unsupported`.

States: not attempted / basic edge fill / layout implemented / authored WideArt / O3DS verified.

| Screen | Top engine | Rows | State | Evidence (LOTUS kj-prof, 2026-11-17) |
| --- | --- | --- | --- | --- |
| party | B | 6 | layout implemented | panel to both edges, wave continued (`worklog/2026-11-17-party-wide.png`) |
| summary | A | 24 | layout implemented | memo panel widened, dotted lines continued, name/sprite/3D at right edge (`...summary-wide.png`) |
| bag | A | 6 | layout implemented | bands and description bar to both edges (`...wide-bag17.png`) |
| pokegear (all tabs) | B | 3 | layout implemented | backdrop opens at its level middle; call window stays DS width; missing text is #1 (`...wide-gear17.png`) |
| trainercard | A | 4 | layout implemented | pattern wraps, badge band runs on, tab at left edge (`...wide-tcard.png`) |
| options | A/B | 2 | basic edge fill | bands continue by their edge column; dithered rows show a solid side (`...wide-options.png`) |
| pcbox | B | 5 | layout implemented | not yet captured (needs a PC in reach) |
| naming, flymap, signature | A/B | 2 | basic edge fill | not yet captured |
| field, battle, title, intro, others | - | 0 | not attempted | DS picture centred (#14/#18/#19) |

Navigation verified with screen transitions in the log: field > party >
summary > party > field, field > bag > field, field > pokegear > gearphone >
pokegear > field, field > trainercard > field, field > options; no stale IDs
and no art leaking between screens. O3DS frame-interval checks are pending
on hardware.
