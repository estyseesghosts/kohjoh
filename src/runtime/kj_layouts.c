/*
 * kj_layouts.c: the built-in wide layout of each 2D screen (issue #16).
 *
 * Ported row for row from Unown's soulsilver3ds/platform/ss3ds_widelayouts.h
 * (HGSS screens measured there on the same game data); the row kinds are
 * documented with kj_layout_row in kj_bridge.h. Index 0 of a screen is the
 * rows for engine A, index 1 for engine B. Only the engine on the top (wide)
 * screen uses its rows. Screens Kohjoh does not classify yet (Voltorb Flip,
 * Safari, berry pots, Scratch-Off, Geonet, Apricorn box, Unown report) keep
 * the DS picture until kj_screen.c names them (issue #17).
 */

#include <stddef.h>

#include "kj_bridge.h"
#include "kj_layouts.h"

#define MOVE(bgs, t, b, x0, x1, dx)  { KJ_LAYOUT_MOVE, (bgs), (t), (b), (x0), (x1), (dx), 0 }
#define FILL(bgs, t, b, x0, x1, col) { KJ_LAYOUT_FILL, (bgs), (t), (b), (x0), (x1), 0, (col) }
#define COPY(bgs, t, b, x0, x1, col) { KJ_LAYOUT_COPY, (bgs), (t), (b), (x0), (x1), 0, (col) }
#define WRAP(bgs, t, b)              { KJ_LAYOUT_WRAP, (bgs), (t), (b), 0, 0, 0, 0 }
#define OBJS(t, b, x0, x1, dx)       { KJ_LAYOUT_OBJS, 0, (t), (b), (x0), (x1), (dx), 0 }
#define SHIFT3D(dx)                  { KJ_LAYOUT_SHIFT3D, 0, 0, 0, 0, 0, (dx), 0 }
#define SCALE(bgs, t, b)             { KJ_LAYOUT_SCALE, (bgs), (t), (b), 0, 0, 0, 0 }
#define N(t) ((uint32_t)(sizeof(t) / sizeof((t)[0])))

/* Summary (engine A on top: BG0 3D Pokemon, BG1 text, BG2 page extras, BG3
 * the panel art). The left panel gets 64 px wider by repeating its clear
 * interior (columns 127..134); the name / portrait block moves right with
 * the 3D Pokemon and its sprites. */
#define SUM8(k) COPY(0x8, 0, 41, 104 + 8 * (k), 112 + 8 * (k), 127), \
                COPY(0x8, 48, 192, 104 + 8 * (k), 112 + 8 * (k), 127)
static const kj_layout_row SUMMARY_A[] = {
    MOVE(0x8, 0, 192, 0, 136, -32),
    SUM8(0), SUM8(1), SUM8(2), SUM8(3), SUM8(4), SUM8(5), SUM8(6), SUM8(7),
    FILL(0x8, 41, 48, 104, 168, 135),
    MOVE(0x8, 0, 192, 136, 256, 32),
    MOVE(0x6, 0, 192, 0, 141, -32), MOVE(0x6, 0, 192, 141, 256, 32),
    OBJS(0, 192, 0, 141, -32), OBJS(0, 192, 141, 256, 32), SHIFT3D(32),
};

/* Title (engine B on top: BG3 the animated sunburst, whose tiles and
 * palette change every frame, so no wide-art picture can match it; BG0-2
 * the logo and text). The sunburst is drawn 5/4 wide, the logo keeps the
 * DS columns: Unown's GPU wide title, approved in Azahar there. */
static const kj_layout_row TITLE_B[] = {
    SCALE(0x8 | KJ_LAYOUT_SCREEN, 0, 192),
};

/* Party (engine B on top: BG1 the framed panel, BG0 the bottom bar). */
static const kj_layout_row PARTY_B[] = {
    MOVE(0x2, 0, 192, 0, 20, -32), FILL(0x2, 0, 192, -12, 52, 21), MOVE(0x2, 0, 192, 20, 256, 32),
    MOVE(0x1, 0, 192, 0, 16, -32), FILL(0x1, 0, 192, -16, 48, 16), MOVE(0x1, 0, 192, 16, 256, 32),
};

/* Bag (engine A on top: BG3 a banded backdrop, BG2 the description bar). */
static const kj_layout_row BAG_A[] = {
    FILL(0x8, 0, 192, -32, 0, 0), FILL(0x8, 0, 192, 256, 288, 255),
    MOVE(0x4, 0, 192, 0, 12, -32), FILL(0x4, 0, 192, -20, 12, 14),
    MOVE(0x4, 0, 192, 244, 256, 32), FILL(0x4, 0, 192, 244, 276, 242),
};

/* Options (BG0 a banded backdrop), either engine. Unown repeats the edge
 * columns, which turns the dithered rows between bands solid at the sides
 * (LOTUS capture 2026-11-17); the bands are horizontal, so continuing the
 * BG's own map keeps the dither. */
static const kj_layout_row OPTIONS[] = {
    WRAP(0x1 | KJ_LAYOUT_SCREEN, 0, 192),
};

/* Pokegear, all tabs (engine B on top: BG0 the curved backdrop opens in the
 * middle, where its curves are level). */
static const kj_layout_row GEAR_B[] = {
    MOVE(0x1, 0, 192, 0, 128, -32), FILL(0x1, 0, 192, 96, 160, 128), MOVE(0x1, 0, 192, 128, 256, 32),
};

/* Trainer card front (engine A on top: BG3 a scrolling pattern, BG2 the
 * badge band and its tab). */
static const kj_layout_row TRAINER_CARD_A[] = {
    WRAP(0x8 | KJ_LAYOUT_SCREEN, 0, 192), FILL(0x4, 0, 192, -32, 0, 0),
    FILL(0x4, 0, 192, 256, 288, 255), MOVE(0x4, 0, 26, 0, 112, -32),
};

/* PC boxes (engine B on top: BG1 the title bar, info panel and stripes,
 * BG0 the text). */
static const kj_layout_row PC_BOX_B[] = {
    MOVE(0x2, 0, 192, 0, 220, -32), FILL(0x2, 0, 192, 188, 252, 220), MOVE(0x2, 0, 192, 220, 256, 32),
    MOVE(0x1, 0, 192, 0, 256, -32), OBJS(0, 192, 0, 256, -32),
};

/* Battle (engine A on top: BG0 3D battlers, BG1 the dialogue frame and
 * text, BG3 the terrain backdrop; battle_main.c Battle_InitBackgrounds).
 * The HP boxes are OBJs and move to the outer edges (Unown's wide_boxes,
 * done at emission so the DS OAM is untouched): the enemy box (upper left)
 * 32 px left, the player box (right, above the dialogue) 32 px right. The
 * platforms are OBJs too but their middles lie outside both boxes. The
 * dialogue frame's edges move out and its interior continues; the text
 * stays where the DS draws it. Terrain bands continue by the screen's edge
 * columns (screen-relative: the intro scrolls BG3, #19).
 * Lines and columns measured in LOTUS (Route 29 wild battle, 2026-11-17). */
static const kj_layout_row BATTLE_A[] = {
    FILL(0x8 | KJ_LAYOUT_SCREEN, 0, 192, -32, 0, 0), FILL(0x8 | KJ_LAYOUT_SCREEN, 0, 192, 256, 288, 255),
    /* The message box (BG1, lines 144-191) keeps the DS columns: a
     * pillarboxed box reads better than one stretched to 400 px
     * (maintainer's call, 2026-11-17; Unown widened it). */
    OBJS(8, 60, -64, 160, -32), OBJS(84, 144, 120, 320, 32),
};

/* Plain or gradient tops: the picture's own edge columns continue. */
static const kj_layout_row EDGE_SIDES[] = {
    FILL(0xF, 0, 192, -32, 0, 0), FILL(0xF, 0, 192, 256, 288, 255),
};

/* Pokedex (both engines): column 255 is a dark border strip, so the right
 * side continues column 250 to match the left's red instead of the border. */
static const kj_layout_row DEX_SIDES[] = {
    FILL(0xF, 0, 192, -32, 0, 0), FILL(0xF, 0, 192, 256, 288, 250),
};

/* Field top (#19): the overworld's 2D (BG1-3) is empty at the screen edges
 * except while an encounter effect covers it (the wild wipe's rectangles,
 * a Gym Leader's VS band) - its edge columns continue over the wide 3D.
 * Message windows and menus are inset from the edges, so a transparent
 * edge column leaves the sides as they were. */
static const kj_layout_row FIELD_A[] = {
    FILL(0xE | KJ_LAYOUT_SCREEN, 0, 192, -32, 0, 0), FILL(0xE | KJ_LAYOUT_SCREEN, 0, 192, 256, 288, 255),
};

typedef struct screen_layout {
    uint32_t id;
    const kj_layout_row *rows[2];
    uint32_t count[2];
} screen_layout;

static const screen_layout LAYOUTS[] = {
    { KJ_SCREEN_FIELD, { FIELD_A, NULL }, { N(FIELD_A), 0 } },
    { KJ_SCREEN_TITLE, { NULL, TITLE_B }, { 0, N(TITLE_B) } },
    { KJ_SCREEN_BATTLE, { BATTLE_A, NULL }, { N(BATTLE_A), 0 } },
    { KJ_SCREEN_SUMMARY, { SUMMARY_A, NULL }, { N(SUMMARY_A), 0 } },
    { KJ_SCREEN_PARTY, { NULL, PARTY_B }, { 0, N(PARTY_B) } },
    { KJ_SCREEN_BAG, { BAG_A, NULL }, { N(BAG_A), 0 } },
    { KJ_SCREEN_OPTIONS, { OPTIONS, OPTIONS }, { N(OPTIONS), N(OPTIONS) } },
    { KJ_SCREEN_POKEGEAR, { NULL, GEAR_B }, { 0, N(GEAR_B) } },
    { KJ_SCREEN_GEAR_MAP, { NULL, GEAR_B }, { 0, N(GEAR_B) } },
    { KJ_SCREEN_GEAR_PHONE, { NULL, GEAR_B }, { 0, N(GEAR_B) } },
    { KJ_SCREEN_GEAR_RADIO, { NULL, GEAR_B }, { 0, N(GEAR_B) } },
    { KJ_SCREEN_GEAR_CONFIG, { NULL, GEAR_B }, { 0, N(GEAR_B) } },
    { KJ_SCREEN_NAMING, { EDGE_SIDES, EDGE_SIDES }, { N(EDGE_SIDES), N(EDGE_SIDES) } },
    { KJ_SCREEN_FLY_MAP, { EDGE_SIDES, EDGE_SIDES }, { N(EDGE_SIDES), N(EDGE_SIDES) } },
    { KJ_SCREEN_TC_SIGNATURE, { EDGE_SIDES, EDGE_SIDES }, { N(EDGE_SIDES), N(EDGE_SIDES) } },
    { KJ_SCREEN_POKEDEX, { DEX_SIDES, DEX_SIDES }, { N(DEX_SIDES), N(DEX_SIDES) } },
    { KJ_SCREEN_PC_BOX, { NULL, PC_BOX_B }, { 0, N(PC_BOX_B) } },
    { KJ_SCREEN_TRAINER_CARD, { TRAINER_CARD_A, NULL }, { N(TRAINER_CARD_A), 0 } },
};

void kj_layouts_for(uint32_t screen_id, kj_screen_state *state)
{
    state->rows[0] = state->rows[1] = NULL;
    state->row_count[0] = state->row_count[1] = 0;
    for (size_t i = 0; i < sizeof(LAYOUTS) / sizeof(LAYOUTS[0]); ++i) {
        if (LAYOUTS[i].id == screen_id) {
            for (int e = 0; e < 2; ++e) {
                state->rows[e] = LAYOUTS[i].rows[e];
                state->row_count[e] = LAYOUTS[i].count[e];
            }
            return;
        }
    }
}
