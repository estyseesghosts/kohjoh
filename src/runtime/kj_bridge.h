/*
 * kj_bridge.h: the only interface between the DS side (the original game,
 * NitroSDK and the compat layer, compiled with 4-byte enums and NitroSDK's
 * types) and the 3DS side (libctru/citro3d, compiled with libctru's ABI).
 *
 * Rules:
 *   - fixed-width C types and plain structs only; no enums cross this line
 *     (the two halves disagree on enum size);
 *   - no NitroSDK or libctru header is included here;
 *   - functions prefixed kj_ds_ are implemented on the DS side, kj_ctr_ on
 *     the 3DS side.
 */

#ifndef KJ_BRIDGE_H
#define KJ_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ boot */

typedef struct kj_game_settings {
    uint16_t nickname[11];   /* UTF-16, 0-terminated */
    uint8_t language;        /* NVRAM language code (1 = English) */
    uint8_t birth_month;
    uint8_t birth_day;
    uint8_t favorite_color;
} kj_game_settings;

/* Open the content pack (both paths are full paths, e.g. romfs:/... or
 * sdmc:/...). Returns NULL on success or a human-readable reason. */
const char *kj_ds_open_pack(const char *pak_path, const char *manifest_path);

/* Write the cartridge/firmware state and run the original NitroMain on the
 * calling thread. Does not return while the game runs. */
void kj_game_main(const kj_game_settings *settings);

/* Back the game's 512 KiB save chip with a raw file (compat port_card).
 * Must be called before kj_game_main. */
void kj_ds_set_save_path(const char *path);

/* Edition identity compiled into this program. */
const char *kj_ds_edition_name(void);     /* "heartgold" / "soulsilver" */
const char *kj_ds_pack_build_id(void);    /* pack builder ID the program expects */

/* -------------------------------------------------------- DS memory view */

/* Read-only pointers into the emulated DS memory the renderer translates.
 * They are valid for the program's lifetime. The renderer reads them only
 * inside kj_ctr_publish_frame (on the game thread, while the game is
 * stopped at its frame boundary), so every read is coherent with one
 * published frame. */
typedef struct kj_ds_memory {
    const uint8_t *io;            /* 0x04000000.. I/O registers (engine A at +0, B at +0x1000) */
    const uint8_t *pltt;          /* 0x800: BG A, OBJ A, BG B, OBJ B */
    const uint8_t *oam;           /* 0x800: engine A, engine B */
    const uint8_t *bg_vram_a;     /* 0x80000 engine A BG view */
    const uint8_t *bg_vram_b;     /* 0x20000 engine B BG view */
    const uint8_t *obj_vram_a;    /* 0x40000 engine A OBJ view */
    const uint8_t *obj_vram_b;    /* 0x20000 engine B OBJ view */
    const uint8_t *lcdc_vram;     /* 0xA4000 banks A..I in LCDC order */
    size_t io_size;
} kj_ds_memory;

const kj_ds_memory *kj_ds_get_memory(void);

/* VRAM bank -> use mapping, decoded from VRAMCNT_A..I. For each bank:
 * offset into lcdc_vram, size, and the MST/OFS fields. */
typedef struct kj_vram_bank {
    uint32_t lcdc_offset;
    uint32_t size;
    uint8_t enabled;
    uint8_t mst;
    uint8_t ofs;
} kj_vram_bank;

/* ---------------------------------------------------- per-line registers */

/* When the game uses H-blank effects, the per-scanline register copies
 * (0x70 bytes from 0x04000000 and 0x04001000 for each line) are valid for
 * this frame. NULL / 0 when no H-blank effect ran. */
#define KJ_LINE_REG_BYTES 0x70
typedef struct kj_line_regs {
    const uint8_t (*regs)[192][KJ_LINE_REG_BYTES];   /* [engine][line] */
    bool active;
} kj_line_regs;

/* -------------------------------------------------------------- 3D stream */

/* One frame of decoded 3D geometry, produced by the DS-side geometry engine
 * (kj_g3.c) and consumed by the PICA backend. The geometry engine does the
 * DS's matrix work, texture-coordinate generation and readable tests on the
 * CPU with exact fixed-point semantics. NORMAL latches transformed normals
 * and lighting inputs for the PICA; literal colours remain supported.
 * Positions stay in object space for the GPU clip-matrix palette. */

#define KJ_G3_MAX_MATRICES 4096
#define KJ_G3_MAX_VERTS 32768
#define KJ_G3_MAX_INDICES 65536
#define KJ_G3_MAX_BATCHES 2048

typedef struct kj_g3_vertex {
    int16_t x, y, z;     /* object space, 4.12 fixed */
    uint8_t mtx;         /* index into the batch's matrix palette */
    uint8_t pad;
    uint8_t r, g, b, a;  /* 0..255 (DS 5-bit expanded); a = polygon alpha */
    int16_t s, t;        /* texel units, 12.4 fixed (after DS texgen) */
} kj_g3_vertex;          /* 16 bytes */

/* A clip matrix (projection x position), row-major 4x4 floats in the
 * PICA's row order (rows = output components). */
typedef struct kj_g3_matrix {
    float m[4][4];
} kj_g3_matrix;

typedef struct kj_g3_batch {
    uint32_t first_index;
    uint32_t index_count;
    uint16_t first_matrix;     /* palette slice for this batch */
    uint16_t matrix_count;
    uint32_t poly_attr;        /* DS POLYGON_ATTR at the BEGIN */
    uint32_t tex_param;        /* DS TEXIMAGE_PARAM */
    uint32_t tex_pltt;         /* DS PLTT_BASE */
    uint32_t viewport;         /* VIEWPORT x1 | y1 << 8 | x2 << 16 | y2 << 24 */
    uint32_t flags;            /* KJ_G3_BATCH_* */
} kj_g3_batch;

/* Matrices one draw can address (the vertex shader's uniform palette). */
#define KJ_G3_PALETTE 16
#define KJ_G3_LIT_PALETTE 6
#define KJ_G3_MAX_LIGHTING 512

#define KJ_G3_BATCH_TRANSLUCENT 0x1u   /* goes to the translucent list */
#define KJ_G3_BATCH_SHADOW      0x2u   /* DS shadow polygon */
#define KJ_G3_BATCH_LIT         0x4u   /* lighting context index in flags >> 16 */

typedef struct kj_g3_frame {
    const kj_g3_vertex *verts;
    const uint16_t *indices;
    const kj_g3_matrix *matrices;
    const kj_g3_batch *batches;
    uint32_t vert_count;
    uint32_t index_count;
    uint32_t matrix_count;
    uint32_t batch_count;
    /* DS state at SwapBuffers */
    uint32_t disp3dcnt;
    uint32_t clear_color;      /* CLEAR_COLOR */
    uint32_t clear_depth;      /* CLEAR_DEPTH (15 bit) */
    uint32_t swap_params;      /* SWAP_BUFFERS argument */
    uint32_t viewport;         /* VIEWPORT x1,y1,x2,y2 bytes */
    uint8_t alpha_test_ref;
    uint16_t fog_color_hi;
    uint32_t fog_color;
    uint16_t fog_offset;
    uint8_t fog_table[32];
    uint16_t edge_color[8];
    uint16_t toon_table[32];
    uint32_t serial;           /* increments per swap */
    /* dropped/unsupported work, never silently zero */
    uint32_t dropped_verts;
    uint32_t dropped_batches;
    uint32_t unsupported;
    /* BOX_TEST results this frame (culled_far: outside the far plane only) */
    uint32_t box_tests, box_culled, box_culled_far;
    const struct kj_lighting *lighting;
    const uint16_t *vertex_lights;
    const uint8_t *vertex_alpha;
    uint32_t lighting_count;
} kj_g3_frame;

/* ----------------------------------------------------------- frame flow */

/* Semantic screen IDs (issue #15): the application on display, from the
 * game's OverlayManager. Fixed values; never stored in the save. */
#define KJ_SCREEN_UNKNOWN        0u
#define KJ_SCREEN_FIELD          1u
#define KJ_SCREEN_BATTLE         2u
#define KJ_SCREEN_TITLE          3u
#define KJ_SCREEN_INTRO          4u
#define KJ_SCREEN_MAIN_MENU      5u
#define KJ_SCREEN_OAK_SPEECH     6u
#define KJ_SCREEN_PARTY          7u
#define KJ_SCREEN_SUMMARY        8u
#define KJ_SCREEN_BAG            9u
#define KJ_SCREEN_PC_BOX         10u
#define KJ_SCREEN_POKEDEX        11u
#define KJ_SCREEN_POKEGEAR       12u
#define KJ_SCREEN_GEAR_MAP       13u
#define KJ_SCREEN_GEAR_PHONE     14u
#define KJ_SCREEN_GEAR_RADIO     15u
#define KJ_SCREEN_GEAR_CONFIG    16u
#define KJ_SCREEN_TRAINER_CARD   17u
#define KJ_SCREEN_TC_SIGNATURE   18u
#define KJ_SCREEN_OPTIONS        19u
#define KJ_SCREEN_FLY_MAP        20u
#define KJ_SCREEN_STARTER        21u
#define KJ_SCREEN_CREDITS        22u
#define KJ_SCREEN_EASY_CHAT      23u
#define KJ_SCREEN_NAMING         24u
#define KJ_SCREEN_OTHER          25u  /* a known application without an ID */
#define KJ_SCREEN_COUNT          26u

/* Names of the IDs above, in order (logs and the wide_layouts.txt override). */
#define KJ_SCREEN_NAMES_INIT                                                            { "unknown", "field", "battle", "title", "intro", "mainmenu", "oakspeech",           "party", "summary", "bag", "pcbox", "pokedex", "pokegear", "gearmap",              "gearphone", "gearradio", "gearconfig", "trainercard", "signature",                "options", "flymap", "starter", "credits", "easychat", "naming", "other" }

/* One wide-layout rule (issue #16; Unown's ss3ds_widelayouts.h rows).
 * Columns and lines are the DS's; the wide top screen shows DS columns
 * -32..287. Unless bgs has KJ_LAYOUT_SCREEN, x0..x1 and top..bottom are
 * columns and lines of the BG's picture (scroll applied), not the screen;
 * with it, FILL/COPY's arg is a screen column too (the edge a scrolling
 * BG shows, #19).
 *   MOVE  bgs top bottom x0 x1 dx  : those columns drawn dx px over
 *   FILL  bgs top bottom x0 x1 arg : wide columns x0..x1-1 show column arg
 *   COPY  bgs top bottom x0 x1 arg : wide columns x0..x1-1 show arg, arg+1..
 *   WRAP  bgs top bottom           : beside the screen the BG map continues
 *   OBJS  top bottom x0 x1 dx      : OBJs whose middle is in the box move dx
 *   SHIFT3D dx                     : the 3D layer moves dx px
 *   SCALE bgs top bottom           : the BG drawn 5/4 wide across all
 *                                    400 px (a backdrop with no edge to
 *                                    continue, e.g. the title's sunburst) */
typedef struct kj_layout_row {
    int16_t kind, bgs, top, bottom, x0, x1, dx, arg;
} kj_layout_row;

#define KJ_LAYOUT_MOVE    1
#define KJ_LAYOUT_FILL    2
#define KJ_LAYOUT_OBJS    3
#define KJ_LAYOUT_SHIFT3D 4
#define KJ_LAYOUT_COPY    5
#define KJ_LAYOUT_WRAP    6
#define KJ_LAYOUT_SCALE   7
#define KJ_LAYOUT_SCREEN  0x10
#define KJ_LAYOUT_MAX_ROWS 32

/* The screen on display and its layout, per engine (0 = A, 1 = B). The
 * generation changes whenever any of it changes; a renderer cache keyed on
 * it never compares strings or scans memory. */
typedef struct kj_screen_state {
    uint32_t screen_id;
    uint32_t depth;              /* nesting of the screen's application */
    uint32_t generation;
    uint32_t row_count[2];
    const kj_layout_row *rows[2]; /* immutable while generation holds */
} kj_screen_state;

/* What the DS side hands the renderer at the frame boundary. */
typedef struct kj_frame_input {
    const kj_ds_memory *mem;
    kj_vram_bank banks[9];
    kj_line_regs lines;
    const kj_g3_frame *g3;          /* NULL if no new 3D scene was swapped */
    uint32_t vblank_count;          /* DS V-blanks so far */
    uint32_t game_frame;            /* publishes so far */
    bool engine_a_on_top;           /* POWCNT1 bit 15 */
    kj_screen_state screen;         /* same generation as this frame's DS state */
} kj_frame_input;

/* DS side -> 3DS side, on the game thread. */

/* The game finished its V-blank work: the published DS state is what the
 * DS would display now. The renderer records and submits the frame. */
void kj_ctr_publish_frame(const kj_frame_input *in);

/* Decide the next picture before game/geometry work starts. False skips
 * capture and publication together, never the simulation or DS tests. */
bool kj_ctr_capture_next_frame(void);

/* Each emulated DS V-blank (the game's OS_Halt). Paces the game to the 3DS
 * display, reads input, services APT. Returns false when the app must
 * close. */
bool kj_ctr_vblank(void);

/* Log a line (thread-safe). */
void kj_ctr_log(const char *line);

/* Input for the DS side, sampled once per V-blank. */
typedef struct kj_input {
    uint16_t keys;        /* PAD_BUTTON_* / PAD_KEY_* bits (X = 0x400, Y = 0x800), 1 = held */
    bool touch;
    uint16_t touch_x;     /* DS coordinates 0..255 */
    uint16_t touch_y;     /* 0..191 */
} kj_input;

const kj_input *kj_ctr_input(void);

/* Game-side probe slots (sibling slots in kj_ds_profile.probe_*; both sides
 * of the bridge share the struct). */
#define KJ_PROBE_SLOTS 11

/* DS-side profile since the last call (ARM11 ticks / counts). */
typedef struct kj_ds_profile {
    uint64_t g3_fifo_ticks;
    uint32_t g3_fifo_words, g3_direct_words, g3_commands;
    uint64_t timer_sync_calls, timer_sync_ticks;
    uint64_t sound_pump_ticks;
    uint32_t sound_pump_calls;
    uint64_t hblank_ticks;
    uint32_t hblank_walks;
    uint32_t dl_hits, dl_misses, dl_uncacheable, dl_bytes;
    /* Game-side probes (kj_frame.c NitroPort_NoteProbeSince). Slots are
     * siblings, not nested: slot 0 is the whole measured call, slots 1..n
     * are its parts, so slot 0 minus the children is the unattributed
     * remainder. The slot count lives here because both sides of the
     * bridge share this struct; after changing it, clean-build (issue #12:
     * a partial rebuild links objects that disagree about this layout and
     * produces plausible fiction). */
    uint64_t probe_ticks[KJ_PROBE_SLOTS];
    uint32_t probe_calls[KJ_PROBE_SLOTS];
} kj_ds_profile;

void kj_ds_take_profile(kj_ds_profile *out);

/* Game-side probes. Compile out with HGSS_PROFILE_OFF: the svcGetSystemTick
 * pair is the cost, not the counters. Use these, never OS_GetTick (the
 * port's timer emulation syncs DS timers, ~8 us per two calls per frame). */
#ifdef HGSS_PROFILE_OFF
#define NitroPort_ProbeNow() (0u)
#define NitroPort_NoteProbeSince(slot, t0) ((void)0)
#else
uint64_t NitroPort_ProbeNow(void);
void NitroPort_NoteProbeSince(unsigned slot, uint64_t t0);
#endif

/* Game termination / reset requests. */
void kj_ctr_terminate(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* KJ_BRIDGE_H */
