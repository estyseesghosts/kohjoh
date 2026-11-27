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

/* g2d_layers.c: palette/tile sources and decode helpers. */
const uint16_t *pltt_ptr(const kj_ds_memory *mem, int engine, int kind);
const uint8_t *ext_bg_slot(const kj_frame_input *in, int engine, int slot);
const uint8_t *ext_obj(const kj_frame_input *in, int engine);
void run_end(run *r);

/* g2d_layers.c: decode one BG/OAM kind into a quad run. */
uint32_t text_bg(const ctx *c, int bg, run *out);
uint32_t affine_bg(const ctx *c, int bg, bool ext16, run *out);
uint32_t objs(const ctx *c, int prio, bool semi, run *out);

/* Reset the per-engine palette snapshots. Called from kj_g2d_init. */
void kj_g2d_layers_reset(void);

#endif
