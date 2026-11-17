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
| bag | A | 6 | layout implemented | bands and description bar to both edges; pocket switching leaves the top stable (`...wide-bag17.png`) |
| pokegear (phone, map, config) | B | 3 | layout implemented | backdrop opens at its level middle; windows stay DS width; phone text missing is #1 (`...wide-gear17.png`, `...wide-gearmap.png`, `...wide-gearcfg.png`); radio tab not unlocked on the test save |
| trainercard | A | 4 | layout implemented | pattern wraps, badge band runs on, tab at left edge; the card's front/back flip is on the lower screen and leaves the top unchanged (`...wide-tcard.png`) |
| options | A/B | 1 | layout implemented | backdrop map wraps, dither continuous to both edges (`...wide-options.png`) |
| pcbox | B | 5 | layout implemented | info panel flush left, title bar and panel run on; box switching (R) leaves the top stable (`...wide-pcbox.png`) |
| naming, flymap, signature | A/B | 2 | basic edge fill | not captured on the test save (needs Fly / a rename / a signature); same FILL rows as the bag's verified backdrop |
| battle (wild, single) | A | 8 | layout implemented | HP boxes to the outer edges (OBJs moved at emission, DS OAM untouched), dialogue frame and terrain to both edges; a Tackle turn and the return to the field leave no stale panel (`...wide-battle.png`, `...wide-battle-turn.png`); trainer intro, double battle (opponent panels at the left edge, player panels at the right), cave and water backdrops captured via the `battle:` script line (`...wide-battle-trainer.png`, `...-double.png`, `...-cave.png`, `...-water.png`); a Gym Leader and an end-to-end switch/faint/capture run not yet captured |
| field | A | 2 | layout implemented | 3D drawn wide; BG1-3 edge columns continue (encounter wipes, VS band) |
| title, intro, others | - | 0 | not attempted | DS picture centred (#14/#19) |

Field encounter wipes (#19): the field's 3D runs wide but its 2D is the DS's
256 columns. Window wipes (trainer effects, `encounter_effect_wipe.c`) now
extend edge-touching windows over the side columns on the top screen when
BG0 is 3D. The wild-grass wipe (`encounter_effect_wild.c`, `RectTrail`) and
a Gym Leader's VS band draw into 256-column text BGs instead; the field's top
layout (`FIELD_A`) continues BG1-3's screen-edge columns over the sides, so
both reach 400 px (`...wide-wild-wipe.png`, r29 frame 6250;
`...wide-vs-wipe.png`, Falkner frame 4110). The overworld's own 2D is inset
from the edges, so its sides stay transparent (no change on the plain field).
The VS wipe's BLDY darkening runs under the same wide scissor.

Navigation verified with screen transitions in the log: field > party >
summary > party > field, field > bag > field, field > pokegear > gearphone >
pokegear > field, field > trainercard > field, field > options; no stale IDs
and no art leaking between screens. O3DS frame-interval checks need real hardware and belong to the
release gate in #21.
