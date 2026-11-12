/*
 * g2d.c: DS 2D engines on the PICA (GPU_2D.md).
 *
 * Translates one engine's state per scanline span into ordered GPU passes:
 *   clear (backdrop colour; stencil = backdrop's effect flags)
 *   window regions -> stencil bits 4-5 (0 outside, 1 WIN0, 2 WIN1)
 *   for priority 3..0: BG3..BG0 of that priority (BG0 may be the 3D layer),
 *                      then the OBJs of that priority (lower OAM index on top)
 *   brightness (BLDY) pass on target-1 pixels; master brightness pass
 *
 * Stencil bits: bit 0 = the top pixel belongs to a 2nd blend target,
 * bit 1 = the top pixel is a 1st target with effects enabled (for BLDY),
 * bits 4-5 = window region. Alpha blending (BLDCNT mode 1, semi-transparent
 * OBJ) is two draws: blended where bit 0 is set, opaque where not; each
 * updates bit 0 to the drawn layer's own 2nd-target membership.
 *
 * Every tile reaches the GPU through the atlas (exact-compare cache). Modes
 * this file cannot draw are counted in unsupported_modes, never replaced by
 * a CPU picture.
 */

#include "graphics/g2d/g2d.h"

#include "diagnostics/log.h"
#include "diagnostics/perf.h"
#include "graphics/g3d/g3d.h"
#include "graphics/pica/gpu.h"
#include "graphics/pica/targets.h"
#include "graphics/resources/atlas.h"

#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <string.h>

#define LAYER_OBJ 4
#define LAYER_BD 5

/* ------------------------------------------------------------ palettes */

enum {
    PAL_4BPP = 0,        /* + engine*32 + kind*16 + bank   (64) */
    PAL_STD256 = 64,     /* + engine*2 + kind               (4) */
    PAL_EXT_BG = 128,    /* + engine*64 + slot*16 + index (128) */
    PAL_EXT_OBJ = 256,   /* + engine*16 + index            (32) */
    PAL_COUNT = 288
};

static kj_palette s_pal[PAL_COUNT];
static kj_g2d_stats s_stats;

static const uint16_t *pltt_ptr(const kj_ds_memory *mem, int engine, int kind)
{
    return (const uint16_t *)(mem->pltt + engine * 0x400 + kind * 0x200);
}

/* Find the LCDC-array address of an extended palette slot, from VRAMCNT.
 * NULL when no bank provides it. */
static const uint8_t *ext_bg_slot(const kj_frame_input *in, int engine, int slot)
{
    const kj_vram_bank *b = in->banks;
    const uint8_t *lcdc = in->mem->lcdc_vram;

    if (engine == 0) {
        if (b[4].enabled && b[4].mst == 4) { /* E: slots 0-3 */
            return lcdc + b[4].lcdc_offset + slot * 0x2000;
        }
        for (int i = 5; i <= 6; ++i) { /* F, G: OFS 0 -> slots 0-1, OFS 1 -> 2-3 */
            if (b[i].enabled && b[i].mst == 4) {
                const int first = (b[i].ofs & 1) ? 2 : 0;
                if (slot >= first && slot < first + 2) {
                    return lcdc + b[i].lcdc_offset + (slot - first) * 0x2000;
                }
            }
        }
        return NULL;
    }
    if (b[7].enabled && b[7].mst == 2) { /* H */
        return lcdc + b[7].lcdc_offset + slot * 0x2000;
    }
    return NULL;
}

static const uint8_t *ext_obj(const kj_frame_input *in, int engine)
{
    const kj_vram_bank *b = in->banks;
    const uint8_t *lcdc = in->mem->lcdc_vram;

    if (engine == 0) {
        for (int i = 5; i <= 6; ++i) {
            if (b[i].enabled && b[i].mst == 5) {
                return lcdc + b[i].lcdc_offset;
            }
        }
        return NULL;
    }
    if (b[8].enabled && b[8].mst == 3) {
        return lcdc + b[8].lcdc_offset;
    }
    return NULL;
}

static kj_palette *pal_4bpp(const kj_frame_input *in, int engine, int kind, int bank, uint32_t *id)
{
    *id = PAL_4BPP + engine * 32 + kind * 16 + bank;
    kj_palette *p = &s_pal[*id];
    p->colors = pltt_ptr(in->mem, engine, kind) + bank * 16;
    p->count = 16;
    kj_palette_check(p);
    return p;
}

static kj_palette *pal_256(const kj_frame_input *in, int engine, int kind, uint32_t *id)
{
    *id = PAL_STD256 + engine * 2 + kind;
    kj_palette *p = &s_pal[*id];
    p->colors = pltt_ptr(in->mem, engine, kind);
    p->count = 256;
    kj_palette_check(p);
    return p;
}

static kj_palette *pal_ext_bg(const kj_frame_input *in, int engine, int slot, int index,
                              uint32_t *id)
{
    const uint8_t *base = ext_bg_slot(in, engine, slot);
    if (base == NULL) {
        return pal_256(in, engine, 0, id);
    }
    *id = PAL_EXT_BG + engine * 64 + slot * 16 + index;
    kj_palette *p = &s_pal[*id];
    p->colors = (const uint16_t *)(base + index * 512);
    p->count = 256;
    kj_palette_check(p);
    return p;
}

static kj_palette *pal_ext_obj(const kj_frame_input *in, int engine, int index, uint32_t *id)
{
    const uint8_t *base = ext_obj(in, engine);
    if (base == NULL) {
        return pal_256(in, engine, 1, id);
    }
    *id = PAL_EXT_OBJ + engine * 16 + index;
    kj_palette *p = &s_pal[*id];
    p->colors = (const uint16_t *)(base + index * 512);
    p->count = 256;
    kj_palette_check(p);
    return p;
}

/* ------------------------------------------------------------ registers */

static uint16_t r16(const uint8_t *regs, unsigned off)
{
    return (uint16_t)(regs[off] | (regs[off + 1] << 8));
}

static uint32_t r32(const uint8_t *regs, unsigned off)
{
    return (uint32_t)regs[off] | ((uint32_t)regs[off + 1] << 8) | ((uint32_t)regs[off + 2] << 16) |
           ((uint32_t)regs[off + 3] << 24);
}

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
} ctx;

/* ------------------------------------------------------------ quad emit */

static inline void put_quad(kj_v2d *v, int x, int y, int w, int h, uint16_t slot, bool hf, bool vf)
{
    int16_t u0, v0;
    kj_atlas_slot_uv(slot, &u0, &v0);
    int16_t u1 = (int16_t)(u0 + 8), v1 = (int16_t)(v0 + 8);
    if (hf) {
        const int16_t t = u0;
        u0 = u1;
        u1 = t;
    }
    if (vf) {
        const int16_t t = v0;
        v0 = v1;
        v1 = t;
    }
    v[0] = (kj_v2d){ (int16_t)x, (int16_t)y, u0, v0, 255, 255, 255, 255 };
    v[1] = (kj_v2d){ (int16_t)(x + w), (int16_t)y, u1, v0, 255, 255, 255, 255 };
    v[2] = (kj_v2d){ (int16_t)x, (int16_t)(y + h), u0, v1, 255, 255, 255, 255 };
    v[3] = (kj_v2d){ (int16_t)(x + w), (int16_t)(y + h), u1, v1, 255, 255, 255, 255 };
}

/* A growable run of quads in the frame arena: reserve a worst case, then
 * give back what was not used (the arena is a bump allocator, and the run
 * is the most recent allocation). */
typedef struct run {
    kj_v2d *v;
    uint32_t first;
    uint32_t count;
    uint32_t cap;
} run;

static bool run_begin(run *r, uint32_t cap)
{
    r->count = 0;
    r->cap = cap;
    r->v = kj_gpu_quads_alloc(cap, &r->first);
    return r->v != NULL;
}

static void run_end(run *r)
{
    /* return the unused tail: re-allocate exactly */
    kj_gpu_quads_reset_to(r->first + r->count);
}

/* ------------------------------------------------------------ text BGs */

static uint32_t text_bg(const ctx *c, int bg, run *out)
{
    const uint16_t cnt = r16(c->regs, 0x08 + 2 * bg);
    const int hofs = r16(c->regs, 0x10 + 4 * bg) & 0x1FF;
    const int vofs = r16(c->regs, 0x12 + 4 * bg) & 0x1FF;
    const int size = cnt >> 14;
    const int w = (size & 1) ? 512 : 256;
    const int h = (size & 2) ? 512 : 256;
    const bool bpp8 = (cnt & 0x80) != 0;
    uint32_t char_base = ((cnt >> 2) & 15u) * 0x4000u;
    uint32_t scr_base = ((cnt >> 8) & 31u) * 0x800u;
    if (c->engine == 0) {
        char_base += ((c->dispcnt >> 24) & 7u) * 0x10000u;
        scr_base += ((c->dispcnt >> 27) & 7u) * 0x10000u;
    }
    const bool ext = bpp8 && (c->dispcnt & (1u << 30));
    const int ext_slot = bg < 2 ? bg + (((cnt >> 13) & 1) ? 2 : 0) : bg;
    const uint32_t tile_bytes = bpp8 ? 64u : 32u;
    const uint32_t mask = c->bg_vram_size - 1u;
    const uint32_t source = (uint32_t)(c->engine * 2);

    const int first_sy = c->y0 - ((c->y0 + vofs) & 7);
    if (!run_begin(out, 33u * (uint32_t)((c->y1 - first_sy + 7) / 8))) {
        return 0;
    }
    for (int sy = first_sy; sy < c->y1; sy += 8) {
        const int my = (sy + vofs) & (h - 1);
        const uint32_t row_base = scr_base + (uint32_t)((my >> 8) * (w >> 8)) * 0x800u +
                                  (uint32_t)((my & 255) >> 3) * 64u;
        for (int sx = -(hofs & 7); sx < 256; sx += 8) {
            const int mx = (sx + hofs) & (w - 1);
            const uint32_t addr = (row_base + (uint32_t)(mx >> 8) * 0x800u +
                                   (uint32_t)((mx & 255) >> 3) * 2u) & mask;
            const uint16_t entry = (uint16_t)(c->bg_vram[addr] | (c->bg_vram[addr + 1] << 8));
            const uint32_t toff = (char_base + (entry & 0x3FFu) * tile_bytes) & mask;
            const int pbank = entry >> 12;
            uint32_t pal_id;
            kj_palette *pal;
            if (!bpp8) {
                pal = pal_4bpp(c->in, c->engine, 0, pbank, &pal_id);
            } else if (ext) {
                pal = pal_ext_bg(c->in, c->engine, ext_slot, pbank, &pal_id);
            } else {
                pal = pal_256(c->in, c->engine, 0, &pal_id);
            }
            const kj_tile_ref t = kj_atlas_tile(source, c->bg_vram + toff, toff, bpp8, pal, pal_id);
            if (!t.valid || t.transparent) {
                continue;
            }
            put_quad(out->v + out->count * 4u, sx, sy, 8, 8, t.slot, (entry >> 10) & 1,
                     (entry >> 11) & 1);
            ++out->count;
        }
    }
    return out->count;
}

/* ---------------------------------------------------------- affine BGs */

/* Affine and extended-affine (16-bit entry) BGs: map tiles are placed by
 * the inverse of the BG's matrix; only tiles whose texture-space position
 * falls inside the screen's texture-space bounding box are emitted. */
static uint32_t affine_bg(const ctx *c, int bg, bool ext16, run *out)
{
    const unsigned base = bg == 2 ? 0x20 : 0x30;
    const uint16_t cnt = r16(c->regs, 0x08 + 2 * bg);
    const float pa = (int16_t)r16(c->regs, base + 0) / 256.0f;
    const float pb = (int16_t)r16(c->regs, base + 2) / 256.0f;
    const float pc = (int16_t)r16(c->regs, base + 4) / 256.0f;
    const float pd = (int16_t)r16(c->regs, base + 6) / 256.0f;
    int32_t rx = (int32_t)(r32(c->regs, base + 8) << 4) >> 4;
    int32_t ry = (int32_t)(r32(c->regs, base + 12) << 4) >> 4;
    const float ox = rx / 256.0f, oy = ry / 256.0f;
    const int size = ext16 ? (128 << (cnt >> 14)) : (128 << (cnt >> 14));
    const bool wrap = (cnt & 0x2000) != 0;
    const bool bpp8 = true;
    uint32_t char_base = ((cnt >> 2) & 15u) * 0x4000u;
    uint32_t scr_base = ((cnt >> 8) & 31u) * 0x800u;
    if (c->engine == 0) {
        char_base += ((c->dispcnt >> 24) & 7u) * 0x10000u;
        scr_base += ((c->dispcnt >> 27) & 7u) * 0x10000u;
    }
    const bool ext = ext16 && (c->dispcnt & (1u << 30));
    const uint32_t mask = c->bg_vram_size - 1u;
    const float det = pa * pd - pb * pc;
    if (fabsf(det) < 1e-6f) {
        return 0;
    }
    /* screen = inv * (tex - origin) */
    const float ia = pd / det, ib = -pb / det, ic = -pc / det, id = pa / det;

    /* texture-space bounds of the span */
    float tminx = 1e9f, tmaxx = -1e9f, tminy = 1e9f, tmaxy = -1e9f;
    const float cx[4] = { 0, 256, 0, 256 }, cy[4] = { (float)c->y0, (float)c->y0, (float)c->y1,
                                                      (float)c->y1 };
    for (int i = 0; i < 4; ++i) {
        const float tx = ox + pa * cx[i] + pb * cy[i];
        const float ty = oy + pc * cx[i] + pd * cy[i];
        tminx = fminf(tminx, tx);
        tmaxx = fmaxf(tmaxx, tx);
        tminy = fminf(tminy, ty);
        tmaxy = fmaxf(tmaxy, ty);
    }
    int tx0 = (int)floorf(tminx / 8.0f) - 1, tx1 = (int)floorf(tmaxx / 8.0f) + 1;
    int ty0 = (int)floorf(tminy / 8.0f) - 1, ty1 = (int)floorf(tmaxy / 8.0f) + 1;
    const int tiles = size / 8;
    if (!wrap) {
        if (tx0 < 0) tx0 = 0;
        if (ty0 < 0) ty0 = 0;
        if (tx1 > tiles - 1) tx1 = tiles - 1;
        if (ty1 > tiles - 1) ty1 = tiles - 1;
    }
    if (tx1 < tx0 || ty1 < ty0) {
        return 0;
    }
    uint32_t n = (uint32_t)((tx1 - tx0 + 1) * (ty1 - ty0 + 1));
    if (n > 8192u) {
        ++s_stats.unsupported_modes;
        return 0;
    }
    if (!run_begin(out, n)) {
        return 0;
    }
    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            const int mtx = ((tx % tiles) + tiles) % tiles;
            const int mty = ((ty % tiles) + tiles) % tiles;
            uint32_t tile;
            bool hf = false, vf = false;
            int pbank = 0;
            if (ext16) {
                const uint32_t a = (scr_base + (uint32_t)(mty * tiles + mtx) * 2u) & mask;
                const uint16_t e = (uint16_t)(c->bg_vram[a] | (c->bg_vram[a + 1] << 8));
                tile = e & 0x3FFu;
                hf = (e >> 10) & 1;
                vf = (e >> 11) & 1;
                pbank = e >> 12;
            } else {
                tile = c->bg_vram[(scr_base + (uint32_t)(mty * tiles + mtx)) & mask];
            }
            const uint32_t toff = (char_base + tile * 64u) & mask;
            uint32_t pal_id;
            kj_palette *pal = ext ? pal_ext_bg(c->in, c->engine, bg, pbank, &pal_id)
                                  : pal_256(c->in, c->engine, 0, &pal_id);
            const kj_tile_ref t =
                kj_atlas_tile((uint32_t)(c->engine * 2), c->bg_vram + toff, toff, bpp8, pal, pal_id);
            if (!t.valid || t.transparent) {
                continue;
            }
            /* corners in texture space -> screen */
            kj_v2d *v = out->v + out->count * 4u;
            int16_t u0, v0;
            kj_atlas_slot_uv(t.slot, &u0, &v0);
            int16_t u1 = (int16_t)(u0 + 8), v1 = (int16_t)(v0 + 8);
            if (hf) { const int16_t s = u0; u0 = u1; u1 = s; }
            if (vf) { const int16_t s = v0; v0 = v1; v1 = s; }
            const float px[4] = { tx * 8.0f, tx * 8.0f + 8, tx * 8.0f, tx * 8.0f + 8 };
            const float py[4] = { ty * 8.0f, ty * 8.0f, ty * 8.0f + 8, ty * 8.0f + 8 };
            const int16_t uu[4] = { u0, u1, u0, u1 }, vv[4] = { v0, v0, v1, v1 };
            for (int k = 0; k < 4; ++k) {
                const float dx = px[k] - ox, dy = py[k] - oy;
                v[k].x = (int16_t)lroundf(ia * dx + ib * dy);
                v[k].y = (int16_t)lroundf(ic * dx + id * dy);
                v[k].u = uu[k];
                v[k].v = vv[k];
                v[k].r = v[k].g = v[k].b = v[k].a = 255;
            }
            ++out->count;
        }
    }
    return out->count;
}

/* ---------------------------------------------------------------- OBJs */

static const uint8_t OBJ_W[4][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 }, { 0 } };
static const uint8_t OBJ_H[4][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 }, { 16, 32, 32, 64 }, { 0 } };

/* Emit the OBJs of one priority and blend kind (semi = OBJ mode 1). */
static uint32_t objs(const ctx *c, int prio, bool semi, run *out)
{
    const uint8_t *oam = c->in->mem->oam + c->engine * 0x400;
    const bool map1d = (c->dispcnt & 0x10) != 0;
    const uint32_t boundary = 32u << ((c->dispcnt >> 20) & 3u);
    const bool ext = (c->dispcnt & (1u << 31)) != 0;
    const uint32_t mask = c->obj_vram_size - 1u;
    const uint32_t source = (uint32_t)(c->engine * 2 + 1);

    if (!run_begin(out, 2048)) {
        return 0;
    }
    for (int i = 127; i >= 0; --i) {
        const uint16_t a0 = r16(oam, (unsigned)i * 8u);
        const uint16_t a1 = r16(oam, (unsigned)i * 8u + 2u);
        const uint16_t a2 = r16(oam, (unsigned)i * 8u + 4u);
        const bool affine = (a0 & 0x100) != 0;
        if (!affine && (a0 & 0x200)) {
            continue;
        }
        if (((a2 >> 10) & 3) != prio) {
            continue;
        }
        const int mode = (a0 >> 10) & 3;
        if (mode == 2 || mode == 3) {
            ++s_stats.unsupported_modes; /* OBJ window / bitmap OBJ */
            continue;
        }
        if ((mode == 1) != semi) {
            continue;
        }
        const int shape = a0 >> 14, sz = a1 >> 14;
        const int w = OBJ_W[shape][sz], h = OBJ_H[shape][sz];
        if (w == 0) {
            continue;
        }
        int y = a0 & 0xFF;
        int x = a1 & 0x1FF;
        if (x >= 256) {
            x -= 512;
        }
        const bool dbl = affine && (a0 & 0x200);
        const int bw = dbl ? w * 2 : w, bh = dbl ? h * 2 : h;
        if (y + bh > 256) {
            y -= 256;
        }
        if (y >= c->y1 || y + bh <= c->y0 || x >= 256 || x + bw <= 0) {
            continue;
        }
        const bool bpp8 = (a0 & 0x2000) != 0;
        const uint32_t tile_bytes = bpp8 ? 64u : 32u;
        const uint32_t tnum = a2 & 0x3FFu;
        const int pbank = a2 >> 12;
        uint32_t pal_id;
        kj_palette *pal = !bpp8 ? pal_4bpp(c->in, c->engine, 1, pbank, &pal_id)
                          : ext ? pal_ext_obj(c->in, c->engine, pbank, &pal_id)
                                : pal_256(c->in, c->engine, 1, &pal_id);
        const int tw = w / 8, th = h / 8;
        bool hf = false, vf = false;
        float ia = 1, ib = 0, ic = 0, id = 1;
        if (affine) {
            const unsigned g = (a1 >> 9) & 31u;
            const float pa = (int16_t)r16(oam, g * 32u + 6u) / 256.0f;
            const float pb = (int16_t)r16(oam, g * 32u + 14u) / 256.0f;
            const float pc = (int16_t)r16(oam, g * 32u + 22u) / 256.0f;
            const float pd = (int16_t)r16(oam, g * 32u + 30u) / 256.0f;
            const float det = pa * pd - pb * pc;
            if (fabsf(det) < 1e-6f) {
                continue;
            }
            ia = pd / det;
            ib = -pb / det;
            ic = -pc / det;
            id = pa / det;
        } else {
            hf = (a1 >> 12) & 1;
            vf = (a1 >> 13) & 1;
        }
        if (out->count + (uint32_t)(tw * th) > out->cap) {
            ++s_stats.dropped_quads;
            continue;
        }
        const float cxs = x + bw / 2.0f, cys = y + bh / 2.0f;
        for (int ty = 0; ty < th; ++ty) {
            for (int tx = 0; tx < tw; ++tx) {
                uint32_t off;
                if (map1d) {
                    off = tnum * boundary + (uint32_t)(ty * tw + tx) * tile_bytes;
                } else {
                    off = tnum * 32u + (uint32_t)ty * 1024u + (uint32_t)tx * tile_bytes;
                }
                off &= mask;
                const kj_tile_ref t =
                    kj_atlas_tile(source, c->obj_vram + off, off, bpp8, pal, pal_id);
                if (!t.valid || t.transparent) {
                    continue;
                }
                kj_v2d *v = out->v + out->count * 4u;
                if (!affine) {
                    const int qx = x + (hf ? (tw - 1 - tx) : tx) * 8;
                    const int qy = y + (vf ? (th - 1 - ty) : ty) * 8;
                    put_quad(v, qx, qy, 8, 8, t.slot, hf, vf);
                } else {
                    int16_t u0, v0;
                    kj_atlas_slot_uv(t.slot, &u0, &v0);
                    const float px[4] = { tx * 8.0f, tx * 8.0f + 8, tx * 8.0f, tx * 8.0f + 8 };
                    const float py[4] = { ty * 8.0f, ty * 8.0f, ty * 8.0f + 8, ty * 8.0f + 8 };
                    const int16_t uu[4] = { u0, (int16_t)(u0 + 8), u0, (int16_t)(u0 + 8) };
                    const int16_t vv[4] = { v0, v0, (int16_t)(v0 + 8), (int16_t)(v0 + 8) };
                    for (int k = 0; k < 4; ++k) {
                        const float dx = px[k] - w / 2.0f, dy = py[k] - h / 2.0f;
                        v[k].x = (int16_t)lroundf(cxs + ia * dx + ib * dy);
                        v[k].y = (int16_t)lroundf(cys + ic * dx + id * dy);
                        v[k].u = uu[k];
                        v[k].v = vv[k];
                        v[k].r = v[k].g = v[k].b = v[k].a = 255;
                    }
                }
                ++out->count;
            }
        }
    }
    return out->count;
}

/* ------------------------------------------------------------- passes */

static void set_xform(const kj_target *t)
{
    /* clip.x = 1 - y_phys/120 ; clip.y = 1 - x_phys/(w/2)
     * y_phys = y * scale ; x_phys = x * scale + offset */
    const float half_w = t->width / 2.0f;
    C3D_FVUnifSet(GPU_VERTEX_SHADER, kj_gpu_uloc_2d_xform(), -t->scale / 120.0f, 1.0f,
                  -t->scale / half_w, 1.0f - t->offset_x / half_w);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, kj_gpu_uloc_2d_uvscale(), 1.0f / KJ_ATLAS_SIZE,
                  -1.0f / KJ_ATLAS_SIZE, 0.0f, 1.0f);
}

typedef struct fx {
    uint16_t bldcnt;
    int mode;
    float eva, evb, evy;
    bool windows;
    uint8_t region_enable[3]; /* bits 0-5: BG0-3, OBJ, effects */
} fx;

static void read_fx(const ctx *c, fx *f)
{
    f->bldcnt = r16(c->regs, 0x50);
    f->mode = (f->bldcnt >> 6) & 3;
    const uint16_t alpha = r16(c->regs, 0x52);
    const int eva = alpha & 31, evb = (alpha >> 8) & 31;
    const int evy = r16(c->regs, 0x54) & 31;
    f->eva = (eva > 16 ? 16 : eva) / 16.0f;
    f->evb = (evb > 16 ? 16 : evb) / 16.0f;
    f->evy = (evy > 16 ? 16 : evy) / 16.0f;
    f->windows = (c->dispcnt & (3u << 13)) != 0;
    if (c->dispcnt & (1u << 15)) {
        ++s_stats.unsupported_modes; /* OBJ window: drawn as outside */
    }
    const uint16_t winin = r16(c->regs, 0x48), winout = r16(c->regs, 0x4A);
    f->region_enable[0] = f->windows ? (uint8_t)(winout & 0x3F) : 0x3F;
    f->region_enable[1] = (uint8_t)(winin & 0x3F);
    f->region_enable[2] = (uint8_t)((winin >> 8) & 0x3F);
}

static void stencil_write_all(uint8_t ref, uint8_t test_mask)
{
    C3D_StencilTest(true, test_mask ? GPU_EQUAL : GPU_ALWAYS, ref, test_mask, 0x03);
    C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
}

static void blend_off(void)
{
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}

static void blend_alpha(float eva, float evb)
{
    const uint32_t ka = (uint32_t)(eva * 255.0f + 0.5f), kb = (uint32_t)(evb * 255.0f + 0.5f);
    C3D_BlendingColor(ka | (ka << 8) | (ka << 16) | (kb << 24));
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_CONSTANT_COLOR, GPU_CONSTANT_ALPHA,
                   GPU_ONE, GPU_ZERO);
}

/* Draw quads of one layer in every window region where it is enabled. */
static void draw_layer(const fx *f, int layer, uint32_t first, uint32_t count, bool force_semi)
{
    if (count == 0) {
        return;
    }
    const bool t2 = (f->bldcnt >> (8 + layer)) & 1;
    const bool t1 = force_semi || ((f->bldcnt >> layer) & 1);
    for (int r = 0; r < 3; ++r) {
        if (r > 0 && !f->windows) {
            break;
        }
        const uint8_t en = f->region_enable[r];
        if (!(en & (1u << layer))) {
            continue;
        }
        const bool effects = (en & 0x20) != 0 || force_semi;
        const uint8_t region = (uint8_t)(f->windows ? (r << 4) : 0);
        const uint8_t rmask = f->windows ? 0x30 : 0x00;
        const bool alpha_blend = t1 && effects && (force_semi || f->mode == 1);
        if (!alpha_blend) {
            const uint8_t flags = (uint8_t)((t2 ? 1 : 0) | (t1 && effects && f->mode >= 2 ? 2 : 0));
            blend_off();
            stencil_write_all((uint8_t)(region | flags), rmask);
            kj_gpu_draw_quads(first, count);
            ++s_stats.draws;
            continue;
        }
        /* blended over 2nd targets */
        blend_alpha(f->eva, f->evb);
        C3D_StencilTest(true, GPU_EQUAL, region | 1u, rmask | 1u, 0x03);
        C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, t2 ? GPU_STENCIL_KEEP : GPU_STENCIL_INVERT);
        kj_gpu_draw_quads(first, count);
        /* opaque elsewhere */
        blend_off();
        C3D_StencilTest(true, GPU_EQUAL, region, rmask | 1u, 0x01);
        C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, t2 ? GPU_STENCIL_INVERT : GPU_STENCIL_KEEP);
        kj_gpu_draw_quads(first, count);
        s_stats.draws += 2;
    }
}

/* A rectangle (DS pixels) as one untextured quad in the arena. */
static bool rect_quad(int x0, int y0, int x1, int y1, uint32_t rgba, uint32_t *first)
{
    kj_v2d *v = kj_gpu_quads_alloc(1, first);
    if (v == NULL) {
        return false;
    }
    const uint8_t r = rgba & 255, g = (rgba >> 8) & 255, b = (rgba >> 16) & 255, a = rgba >> 24;
    v[0] = (kj_v2d){ (int16_t)x0, (int16_t)y0, 0, 0, r, g, b, a };
    v[1] = (kj_v2d){ (int16_t)x1, (int16_t)y0, 0, 0, r, g, b, a };
    v[2] = (kj_v2d){ (int16_t)x0, (int16_t)y1, 0, 0, r, g, b, a };
    v[3] = (kj_v2d){ (int16_t)x1, (int16_t)y1, 0, 0, r, g, b, a };
    return true;
}

/* Window rectangles of a span into stencil bits 4-5. WIN1 first, then
 * WIN0 (WIN0 has priority). Coordinates wrap when x1 > x2 / y1 > y2. */
static void write_windows(const ctx *c)
{
    kj_gpu_tev_vertex_color();
    C3D_DepthTest(false, GPU_ALWAYS, 0); /* stencil only: no colour or depth writes */
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    for (int w = 1; w >= 0; --w) {
        if (!(c->dispcnt & (1u << (13 + w)))) {
            continue;
        }
        const uint16_t hreg = r16(c->regs, 0x40 + 2 * w), vreg = r16(c->regs, 0x44 + 2 * w);
        int x1 = hreg >> 8, x2 = hreg & 255, y1 = vreg >> 8, y2 = vreg & 255;
        if (x2 == 0 && x1 != 0) {
            x2 = 256;
        }
        if (y2 > 192 || (y2 == 0 && y1 != 0)) {
            y2 = 192;
        }
        int xs[2][2], ys[2][2], nx = 0, ny = 0;
        if (x1 <= x2) { xs[nx][0] = x1; xs[nx++][1] = x2; }
        else { xs[nx][0] = x1; xs[nx++][1] = 256; xs[nx][0] = 0; xs[nx++][1] = x2; }
        if (y1 <= y2) { ys[ny][0] = y1; ys[ny++][1] = y2; }
        else { ys[ny][0] = y1; ys[ny++][1] = 192; ys[ny][0] = 0; ys[ny++][1] = y2; }
        C3D_StencilTest(true, GPU_ALWAYS, (u8)((w + 1) << 4), 0, 0x30);
        C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
        for (int i = 0; i < nx; ++i) {
            for (int j = 0; j < ny; ++j) {
                uint32_t q;
                if (rect_quad(xs[i][0], ys[j][0], xs[i][1], ys[j][1], 0, &q)) {
                    kj_gpu_draw_quads(q, 1);
                }
            }
        }
    }
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_AlphaTest(true, GPU_GREATER, 0);
}

/* BLDY: pixels whose top layer is an effect-enabled 1st target. */
static void brightness_pass(const fx *f)
{
    if (f->mode < 2 || f->evy <= 0.0f) {
        return;
    }
    const uint32_t k = (uint32_t)(f->evy * 255.0f + 0.5f);
    const uint32_t color = f->mode == 2 ? 0xFFFFFFFFu : 0xFF000000u;
    uint32_t q;
    if (!rect_quad(-64, 0, 320, 192, color, &q)) {
        return;
    }
    kj_gpu_tev_vertex_color();
    C3D_BlendingColor(k << 24);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_CONSTANT_ALPHA, GPU_ONE_MINUS_CONSTANT_ALPHA,
                   GPU_ZERO, GPU_ONE);
    C3D_StencilTest(true, GPU_EQUAL, 2, 2, 0);
    C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_KEEP);
    kj_gpu_draw_quads(q, 1);
    ++s_stats.draws;
}

/* MASTER_BRIGHT over the whole physical screen (sides included). */
static void master_brightness(const ctx *c, uint16_t mb)
{
    const int mode = mb >> 14;
    int factor = mb & 31;
    if (factor > 16) {
        factor = 16;
    }
    if ((mode != 1 && mode != 2) || factor == 0) {
        return;
    }
    const uint32_t k = (uint32_t)(factor * 255 / 16);
    uint32_t q;
    if (!rect_quad(-64, 0, 320, 192, mode == 1 ? 0xFFFFFFFFu : 0xFF000000u, &q)) {
        return;
    }
    (void)c;
    kj_gpu_scissor_off();
    kj_gpu_tev_vertex_color();
    C3D_BlendingColor(k << 24);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_CONSTANT_ALPHA, GPU_ONE_MINUS_CONSTANT_ALPHA,
                   GPU_ZERO, GPU_ONE);
    C3D_StencilTest(false, GPU_ALWAYS, 0, 0, 0);
    kj_gpu_draw_quads(q, 1);
    ++s_stats.draws;
}

/* --------------------------------------------------------------- spans */

/* Lines [y0, y1) whose registers equal line y0's (the bytes the 2D
 * renderer reads: 0x00-0x6F except DISPSTAT/VCOUNT, already zeroed). */
static int span_end(const kj_line_regs *lines, int engine, int y0)
{
    if (!lines->active) {
        return 192;
    }
    const uint8_t *a = lines->regs[engine][y0];
    int y = y0 + 1;
    while (y < 192 && memcmp(lines->regs[engine][y], a, KJ_LINE_REG_BYTES) == 0) {
        ++y;
    }
    return y;
}

static void draw_span(ctx *c, bool draw_3d, bool *drew_3d)
{
    fx f;
    read_fx(c, &f);
    const int bgmode = (int)(c->dispcnt & 7u);
    const uint32_t layers_on = (c->dispcnt >> 8) & 0x1Fu;
    const bool bg0_3d = c->engine == 0 && (c->dispcnt & 8u);

    const int sx0 = (int)(c->target->offset_x);
    kj_gpu_scissor(c->target->width, sx0, (int)(c->y0 * c->target->scale),
                   sx0 + (int)(256 * c->target->scale), (int)(c->y1 * c->target->scale));
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_AlphaTest(true, GPU_GREATER, 0);
    C3D_CullFace(GPU_CULL_NONE);

    kj_gpu_use_program(KJ_PROGRAM_2D);
    set_xform(c->target);
    if (f.windows) {
        write_windows(c);
    }

    /* prepare every layer's quads first, then draw in priority order */
    run bg_run[4];
    bool bg_ok[4] = { false, false, false, false };
    uint64_t t0 = kj_perf_now();
    for (int bg = 0; bg < 4; ++bg) {
        if (!(layers_on & (1u << bg)) || (bg == 0 && bg0_3d)) {
            continue;
        }
        bool affine = false, ext16 = false, unsupported = false;
        switch (bgmode) {
        case 0: break;
        case 1: affine = bg == 3; break;
        case 2: affine = bg >= 2; break;
        case 3: ext16 = bg == 3; break;
        case 4: affine = bg == 2; ext16 = bg == 3; break;
        case 5: ext16 = bg >= 2; break;
        default: unsupported = bg >= 2; break;
        }
        if (ext16) {
            const uint16_t cnt = r16(c->regs, 0x08 + 2 * bg);
            if (cnt & 0x80) {
                unsupported = true; /* bitmap BG (256-colour / direct) */
            }
        }
        if (unsupported) {
            ++s_stats.unsupported_modes;
            continue;
        }
        if (affine || ext16) {
            affine_bg(c, bg, ext16, &bg_run[bg]);
        } else {
            text_bg(c, bg, &bg_run[bg]);
        }
        if (bg_run[bg].v != NULL) {
            run_end(&bg_run[bg]);
            bg_ok[bg] = true;
            s_stats.bg_quads += bg_run[bg].count;
        }
    }
    run obj_run[4][2];
    bool obj_ok[4][2] = { { false } };
    if (layers_on & 0x10u) {
        for (int p = 0; p < 4; ++p) {
            for (int s = 0; s < 2; ++s) {
                objs(c, p, s == 1, &obj_run[p][s]);
                if (obj_run[p][s].v != NULL) {
                    run_end(&obj_run[p][s]);
                    obj_ok[p][s] = true;
                    s_stats.obj_quads += obj_run[p][s].count;
                }
            }
        }
    }
    kj_perf_add(KJ_PERF_G2D, kj_perf_now() - t0);

    kj_gpu_use_program(KJ_PROGRAM_2D);
    set_xform(c->target);
    kj_gpu_bind_texture(0, kj_atlas_texture());
    kj_gpu_tev_texture_modulate();

    for (int p = 3; p >= 0; --p) {
        for (int bg = 3; bg >= 0; --bg) {
            if (!(layers_on & (1u << bg))) {
                continue;
            }
            const uint16_t cnt = r16(c->regs, 0x08 + 2 * bg);
            if ((cnt & 3) != p) {
                continue;
            }
            if (bg == 0 && bg0_3d) {
                if (draw_3d && !*drew_3d) {
                    const bool t2 = (f.bldcnt >> 8) & 1;
                    const bool t1 = f.bldcnt & 1;
                    kj_gpu_scissor_off();
                    kj_g3d_draw(c->target, c->screen == KJ_SCREEN_TOP,
                                (uint8_t)((t2 ? 1 : 0) | (t1 && f.mode >= 2 ? 2 : 0)));
                    *drew_3d = true;
                    kj_gpu_scissor(c->target->width, sx0, (int)(c->y0 * c->target->scale),
                                   sx0 + (int)(256 * c->target->scale),
                                   (int)(c->y1 * c->target->scale));
                    kj_gpu_use_program(KJ_PROGRAM_2D);
                    set_xform(c->target);
                    kj_gpu_bind_texture(0, kj_atlas_texture());
                    kj_gpu_tev_texture_modulate();
                    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
                    C3D_AlphaTest(true, GPU_GREATER, 0);
                    C3D_CullFace(GPU_CULL_NONE);
                }
                continue;
            }
            if (bg_ok[bg]) {
                draw_layer(&f, bg, bg_run[bg].first, bg_run[bg].count, false);
            }
        }
        if (obj_ok[p][0]) {
            draw_layer(&f, LAYER_OBJ, obj_run[p][0].first, obj_run[p][0].count, false);
        }
        if (obj_ok[p][1]) {
            fx semi = f;
            draw_layer(&semi, LAYER_OBJ, obj_run[p][1].first, obj_run[p][1].count, true);
        }
    }
    brightness_pass(&f);
}

static uint32_t ds_to_rgba(uint16_t c)
{
    const uint32_t r = c & 31u, g = (c >> 5) & 31u, b = (c >> 10) & 31u;
    return ((r << 3) | (r >> 2)) << 24 | ((g << 3) | (g >> 2)) << 16 | ((b << 3) | (b >> 2)) << 8 |
           0xFFu;
}

void kj_screen_draw(int screen, int engine, const kj_frame_input *in)
{
    const kj_target *target = kj_target_get(screen);
    const kj_ds_memory *mem = in->mem;
    ctx c;

    memset(&c, 0, sizeof(c));
    c.in = in;
    c.target = target;
    c.screen = screen;
    c.engine = engine;
    c.bg_vram = engine == 0 ? mem->bg_vram_a : mem->bg_vram_b;
    c.bg_vram_size = engine == 0 ? 0x80000u : 0x20000u;
    c.obj_vram = engine == 0 ? mem->obj_vram_a : mem->obj_vram_b;
    c.obj_vram_size = engine == 0 ? 0x40000u : 0x20000u;

    const uint8_t *io = mem->io + engine * 0x1000;
    const uint32_t dispcnt = r32(io, 0);
    const uint16_t backdrop = (uint16_t)(mem->pltt[engine * 0x400] | (mem->pltt[engine * 0x400 + 1] << 8));
    const uint16_t bldcnt = r16(io, 0x50);
    const uint8_t bd_flags = (uint8_t)(((bldcnt >> 13) & 1) | ((bldcnt & 0x20) && ((bldcnt >> 6) & 3) >= 2 ? 2 : 0));

    kj_gpu_invalidate();
    C3D_FrameDrawOn(target->rt);

    const int display_mode = (int)((dispcnt >> 16) & 3u);
    const bool forced_blank = (dispcnt & 0x80u) != 0;
    if (forced_blank || display_mode == 0) {
        C3D_RenderTargetClear(target->rt, C3D_CLEAR_ALL, 0xFFFFFFFFu, 0);
        kj_gpu_use_program(KJ_PROGRAM_2D);
        set_xform(target);
        master_brightness(&c, r16(io, 0x6C));
        return;
    }
    if (display_mode != 1) {
        ++s_stats.unsupported_modes; /* VRAM / main-memory display: tracked blocker */
        C3D_RenderTargetClear(target->rt, C3D_CLEAR_ALL, 0x000000FFu, 0);
        return;
    }

    C3D_RenderTargetClear(target->rt, C3D_CLEAR_ALL, ds_to_rgba(backdrop), (u32)bd_flags << 24);

    bool drew_3d = false;
    const bool has_3d = engine == 0 && (dispcnt & 8u) && in->g3 != NULL;
    for (int y0 = 0; y0 < 192;) {
        const int y1 = span_end(&in->lines, engine, y0);
        c.y0 = y0;
        c.y1 = y1;
        c.regs = in->lines.active ? in->lines.regs[engine][y0] : io;
        c.dispcnt = r32(c.regs, 0);
        draw_span(&c, has_3d, &drew_3d);
        ++s_stats.spans;
        y0 = y1;
    }
    kj_gpu_scissor_off();
    kj_gpu_use_program(KJ_PROGRAM_2D);
    set_xform(target);
    master_brightness(&c, r16(io, 0x6C));
}

/* ---------------------------------------------------------------- init */

bool kj_g2d_init(void)
{
    memset(s_pal, 0, sizeof(s_pal));
    return kj_gpu_init() && kj_atlas_init();
}

void kj_g2d_exit(void)
{
    kj_atlas_exit();
    kj_gpu_exit();
}

void kj_g2d_take_stats(kj_g2d_stats *out)
{
    *out = s_stats;
    out->dropped_quads += kj_gpu_quads_dropped();
    memset(&s_stats, 0, sizeof(s_stats));
}
