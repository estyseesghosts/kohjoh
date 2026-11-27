/*
 * g2d_internal.h: private interface shared by the g2d implementation files.
 * Never include this outside src/graphics/g2d.
 */
#ifndef KJ_GRAPHICS_G2D_G2D_INTERNAL_H
#define KJ_GRAPHICS_G2D_G2D_INTERNAL_H

#include "graphics/g2d/g2d.h"
#include "graphics/layout/layout.h"

#include <stdbool.h>
#include <stdint.h>

/* The context of one engine's span. */
typedef struct ctx {
    const kj_frame_input *in;
    const kj_target *target;
    int screen;
    int engine;
    const uint8_t *regs;
    uint32_t dispcnt;
    const uint8_t *bg_vram;
    uint32_t bg_vram_size;
    const uint8_t *obj_vram;
    uint32_t obj_vram_size;
    int y0, y1; /* span lines [y0, y1) */
    /* The wide layout (top screen only; nrows 0 = draw as the DS). */
    const kj_layout_row *rows;
    uint32_t nrows;
    bool battle_obj_anchors;
} ctx;

/* A run of quads: either in the frame arena (reserve a worst case, give the
 * tail back) or in a retained layer's own buffer (ext = true). Retained runs
 * also record each quad's atlas slot and the range of character bytes read,
 * which is what their reuse check compares next frame. */
typedef struct run {
    kj_v2d *v;
    uint32_t first;
    uint32_t count;
    uint32_t cap;
    bool ext;
    uint16_t *slots;
    uint32_t char_lo, char_hi;
} run;

static inline uint16_t r16(const uint8_t *regs, unsigned off) {
    return (uint16_t)(regs[off] | (regs[off + 1] << 8));
}

static inline uint32_t r32(const uint8_t *regs, unsigned off) {
    return (uint32_t)regs[off] | ((uint32_t)regs[off + 1] << 8) | ((uint32_t)regs[off + 2] << 16) |
           ((uint32_t)regs[off + 3] << 24);
}

/* Retained-layer policy: a layer is reusable only when every byte it
 * consumed is byte-identical. g2d_retained.c owns the caches and their
 * persistent allocations; the orchestrator decides when to build. */
#define CACHE_QUADS 2304u
#define MAP_MAX 8192u

typedef struct layer_cache {
    bool valid;
    uint8_t key[16];
    uint16_t pal[256]; /* the layer's palette (BG or OBJ) as built */
    uint8_t map[MAP_MAX];
    uint32_t map_off, map_len;
    uint8_t *chars;
    uint32_t char_lo, char_len, char_cap;
    uint8_t ext[0x2000];
    const uint8_t *ext_src;
    kj_v2d *quads;
    uint16_t slots[CACHE_QUADS];
    uint32_t count;
    /* OBJ: per (priority, semi) sub-ranges of quads */
    uint32_t sub_first[9], sub_count[9];
    uint8_t oam[0x400];
    int8_t oam_wide_dx[128];
    /* text BG on the top screen: its wide-art bands (at most 2 quads) */
    kj_v2d *band;
    uint32_t band_count;
} layer_cache;

typedef struct screen_memo {
    bool drawn;
    int engine;
    uint32_t layout_generation;
    uint8_t regs[KJ_LINE_REG_BYTES];
    uint16_t pal[512];
} screen_memo;

extern layer_cache s_bg_cache[2][4];
extern layer_cache s_obj_cache[2];
extern screen_memo s_memo[2];
extern uint32_t s_skipped_screens, s_reused_layers, s_rebuilt_layers;
extern uint32_t s_char_bytes_compared;

bool kj_g2d_cache_alloc(layer_cache *lc);
void kj_g2d_layer_cache_reset(layer_cache *lc);
bool kj_g2d_chars_same(const layer_cache *lc, const uint8_t *vram);
void kj_g2d_chars_keep(layer_cache *lc, const uint8_t *vram, uint32_t lo, uint32_t hi);

/* g2d_effects.c: DS window/blend/brightness state and passes. */
typedef struct fx {
    uint16_t bldcnt;
    int mode;
    float eva, evb, evy;
    bool windows;
    bool objwin;
    uint8_t region_enable[4]; /* bits 0-5: BG0-3, OBJ, effects */
    uint8_t active;           /* bit r: region r can occur (outside always) */
} fx;

extern kj_g2d_stats s_stats;

void set_xform_x(const kj_target *t, float k, float add);
void set_xform(const kj_target *t);
int phys_x(const kj_target *t, int x);
int phys_y(const kj_target *t, int y);
void read_fx(const ctx *c, fx *f);
uint8_t bg0_window_on(const fx *f);
void stencil_write_all(uint8_t ref, uint8_t test_mask);
void blend_off(void);
void blend_alpha(float eva, float evb);
void open_wide_3d_scissor(const ctx *c);
void write_windows(const ctx *c);
void brightness_pass(const fx *f);
void master_brightness(const ctx *c, uint16_t mb);
void side_black(void);

/* g2d_layers.c: palette/tile sources and decode helpers. */
const uint16_t *pltt_ptr(const kj_ds_memory *mem, int engine, int kind);
const uint8_t *ext_bg_slot(const kj_frame_input *in, int engine, int slot);
const uint8_t *ext_obj(const kj_frame_input *in, int engine);
bool run_begin(run *r, uint32_t cap);
void run_end(run *r);

/* g2d_layers.c: decode one BG/OAM kind into a quad run. */
uint32_t text_bg(const ctx *c, int bg, run *out);
uint32_t affine_bg(const ctx *c, int bg, bool ext16, run *out);
uint32_t objs(const ctx *c, int prio, bool semi, run *out);

/* Reset the per-engine palette snapshots. Called from kj_g2d_init. */
void kj_g2d_layers_reset(void);

#endif
