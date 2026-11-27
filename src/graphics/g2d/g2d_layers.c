/*
 * g2d_layers.c: DS BG/OBJ decoding into quad runs.
 *
 * This is the layer-generation half of g2d.c: palette/tile decoding for
 * text, affine and extended BGs and OAM sprites. It owns no renderer state
 * beyond the per-engine palette snapshots; the screen orchestrator decides
 * which runs are retained, how they reach the GPU and in what priority.
 */

#include "graphics/g2d/g2d_internal.h"

#include "graphics/g2d/g2d.h"
#include "graphics/font/text_window.h"
#include "graphics/resources/atlas.h"

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------ palettes */

enum {
    PAL_4BPP = 0,      /* + engine*32 + kind*16 + bank   (64) */
    PAL_STD256 = 64,   /* + engine*2 + kind               (4) */
    PAL_EXT_BG = 128,  /* + engine*64 + slot*16 + index (128) */
    PAL_EXT_OBJ = 256, /* + engine*16 + index            (32) */
    PAL_COUNT = 288
};

static kj_palette s_pal[PAL_COUNT];

void kj_g2d_layers_reset(void) {
    memset(s_pal, 0, sizeof(s_pal));
}

const uint16_t *pltt_ptr(const kj_ds_memory *mem, int engine, int kind) {
    return (const uint16_t *)(mem->pltt + engine * 0x400 + kind * 0x200);
}

/* Find the LCDC-array address of an extended palette slot, from VRAMCNT.
 * NULL when no bank provides it. */
const uint8_t *ext_bg_slot(const kj_frame_input *in, int engine, int slot) {
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

const uint8_t *ext_obj(const kj_frame_input *in, int engine) {
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

static kj_palette *pal_4bpp(const kj_frame_input *in, int engine, int kind, int bank,
                            uint32_t *id) {
    *id = PAL_4BPP + engine * 32 + kind * 16 + bank;
    kj_palette *p = &s_pal[*id];
    p->colors = pltt_ptr(in->mem, engine, kind) + bank * 16;
    p->count = 16;
    kj_palette_check(p);
    return p;
}

static kj_palette *pal_256(const kj_frame_input *in, int engine, int kind, uint32_t *id) {
    *id = PAL_STD256 + engine * 2 + kind;
    kj_palette *p = &s_pal[*id];
    p->colors = pltt_ptr(in->mem, engine, kind);
    p->count = 256;
    kj_palette_check(p);
    return p;
}

static kj_palette *pal_ext_bg(const kj_frame_input *in, int engine, int slot, int index,
                              uint32_t *id) {
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

static kj_palette *pal_ext_obj(const kj_frame_input *in, int engine, int index, uint32_t *id) {
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

/* ------------------------------------------------------------ quad emit */

static inline void put_quad(kj_v2d *v, int x, int y, int w, int h, uint16_t slot, bool hf,
                            bool vf) {
    int16_t u0, v0, u1, v1;
    kj_atlas_slot_uv(slot, &u0, &v0, &u1, &v1);
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
    v[0] = (kj_v2d){(int16_t)x, (int16_t)y, u0, v0, 255, 255, 255, 255};
    v[1] = (kj_v2d){(int16_t)(x + w), (int16_t)y, u1, v0, 255, 255, 255, 255};
    v[2] = (kj_v2d){(int16_t)x, (int16_t)(y + h), u0, v1, 255, 255, 255, 255};
    v[3] = (kj_v2d){(int16_t)(x + w), (int16_t)(y + h), u1, v1, 255, 255, 255, 255};
}

bool run_begin(run *r, uint32_t cap) {
    r->count = 0;
    r->char_lo = UINT32_MAX;
    r->char_hi = 0;
    if (r->ext) {
        return r->v != NULL;
    }
    r->cap = cap;
    r->slots = NULL;
    r->v = kj_gpu_quads_alloc(cap, &r->first);
    return r->v != NULL;
}

void run_end(run *r) {
    if (!r->ext) {
        kj_gpu_quads_reset_to(r->first + r->count);
    }
}

static inline void run_note(run *r, uint16_t slot, uint32_t off, uint32_t bytes) {
    if (r->slots != NULL) {
        r->slots[r->count] = slot;
    }
    if (off < r->char_lo) {
        r->char_lo = off;
    }
    if (off + bytes > r->char_hi) {
        r->char_hi = off + bytes;
    }
}

/* ------------------------------------------------------------ text BGs */

uint32_t text_bg(const ctx *c, int bg, run *out) {
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
    /* Under a wide layout the quads cover every column a segment can name
     * (layout.h); the segments' scissors decide what shows. */
    const int sx_lo = c->nrows ? KJ_LAYOUT_SRC_LO : 0;
    const int sx_hi = c->nrows ? KJ_LAYOUT_SRC_HI : 256;
    const uint32_t cols = (uint32_t)(sx_hi - sx_lo) / 8u + 1u;
    if (!run_begin(out, cols * (uint32_t)((c->y1 - first_sy + 7) / 8))) {
        return 0;
    }
    for (int sy = first_sy; sy < c->y1; sy += 8) {
        const int my = (sy + vofs) & (h - 1);
        const uint32_t row_base = scr_base + (uint32_t)((my >> 8) * (w >> 8)) * 0x800u +
                                  (uint32_t)((my & 255) >> 3) * 64u;
        for (int sx = sx_lo - (hofs & 7); sx < sx_hi; sx += 8) {
            const int mx = (sx + hofs) & (w - 1);
            const uint32_t addr =
                (row_base + (uint32_t)(mx >> 8) * 0x800u + (uint32_t)((mx & 255) >> 3) * 2u) & mask;
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
            /* Note even tiles that came back transparent: the retained layer's
             * reuse check compares the bytes of everything the map references,
             * so a tile that was blank at build time must still widen the
             * range or later glyph writes into it are never noticed. */
            run_note(out, t.slot, toff, tile_bytes);
            if (!t.valid || t.transparent) {
                continue;
            }
            /* This tile's pixels are replaced by the native pass for a window
             * on this plane; leaving the legacy glyphs would double-print. */
            if (kj_text_owns_tile(c->engine, bg, hofs, vofs, sx, sy)) {
                continue;
            }
            if (out->count >= out->cap) {
#ifndef HGSS_PROFILE_OFF
                ++s_stats.dropped_quads;
#endif
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
uint32_t affine_bg(const ctx *c, int bg, bool ext16, run *out) {
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
    const float cx[4] = {0, 256, 0, 256},
                cy[4] = {(float)c->y0, (float)c->y0, (float)c->y1, (float)c->y1};
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
        if (tx0 < 0)
            tx0 = 0;
        if (ty0 < 0)
            ty0 = 0;
        if (tx1 > tiles - 1)
            tx1 = tiles - 1;
        if (ty1 > tiles - 1)
            ty1 = tiles - 1;
    }
    if (tx1 < tx0 || ty1 < ty0) {
        return 0;
    }
    uint32_t n = (uint32_t)((tx1 - tx0 + 1) * (ty1 - ty0 + 1));
    if (n > 8192u) {
#ifndef HGSS_PROFILE_OFF
        ++s_stats.unsupported_modes;
#endif
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
            const kj_tile_ref t = kj_atlas_tile((uint32_t)(c->engine * 2), c->bg_vram + toff, toff,
                                                bpp8, pal, pal_id);
            if (!t.valid || t.transparent) {
                continue;
            }
            /* corners in texture space -> screen */
            kj_v2d *v = out->v + out->count * 4u;
            int16_t u0, v0, u1, v1;
            kj_atlas_slot_uv(t.slot, &u0, &v0, &u1, &v1);
            if (hf) {
                const int16_t s = u0;
                u0 = u1;
                u1 = s;
            }
            if (vf) {
                const int16_t s = v0;
                v0 = v1;
                v1 = s;
            }
            const float px[4] = {tx * 8.0f, tx * 8.0f + 8, tx * 8.0f, tx * 8.0f + 8};
            const float py[4] = {ty * 8.0f, ty * 8.0f, ty * 8.0f + 8, ty * 8.0f + 8};
            const int16_t uu[4] = {u0, u1, u0, u1}, vv[4] = {v0, v0, v1, v1};
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

static const uint8_t OBJ_W[4][4] = {{8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}, {0}};
static const uint8_t OBJ_H[4][4] = {{8, 16, 32, 64}, {8, 8, 16, 32}, {16, 32, 32, 64}, {0}};

/* Emit the OBJs of one priority and blend kind (semi = OBJ mode 1).
 * prio < 0 emits the OBJ-window OBJs (mode 2) of every priority. */
uint32_t objs(const ctx *c, int prio, bool semi, run *out) {
    const uint8_t *oam = c->in->mem->oam + c->engine * 0x400;
    const bool map1d = (c->dispcnt & 0x10) != 0;
    const uint32_t boundary = 32u << ((c->dispcnt >> 20) & 3u);
    const bool ext = (c->dispcnt & (1u << 31)) != 0;
    const uint32_t mask = c->obj_vram_size - 1u;
    const uint32_t source = (uint32_t)(c->engine * 2 + 1);

    if (!run_begin(out, 2048)) {
        return 0;
    }
    if (!out->ext) {
        out->cap = 2048;
    }
    for (int i = 127; i >= 0; --i) {
        const uint16_t a0 = r16(oam, (unsigned)i * 8u);
        const uint16_t a1 = r16(oam, (unsigned)i * 8u + 2u);
        const uint16_t a2 = r16(oam, (unsigned)i * 8u + 4u);
        const bool affine = (a0 & 0x100) != 0;
        if (!affine && (a0 & 0x200)) {
            continue;
        }
        const int mode = (a0 >> 10) & 3;
        if (prio < 0) {
            if (mode != 2) {
                continue;
            }
        } else if (((a2 >> 10) & 3) != prio || mode == 2) {
            continue;
        } else if (mode == 3) {
#ifndef HGSS_PROFILE_OFF
            ++s_stats.unsupported_modes; /* bitmap OBJ */
#endif
            continue;
        } else if ((mode == 1) != semi) {
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
        int x_lo = 0, x_hi = 256;
        if (c->nrows) {
            /* OBJS rows move a sprite by its middle; the wide screen shows
             * DS columns -32..287. OAM itself is never rewritten. */
            if (c->battle_obj_anchors && c->in->mem->oam_wide_dx) {
                x += c->in->mem->oam_wide_dx[c->engine * 128 + i];
            } else {
                x += kj_layout_obj_dx(c->rows, c->nrows, x + bw / 2, y + bh / 2);
            }
            x_lo = KJ_LAYOUT_LEFT;
            x_hi = KJ_LAYOUT_RIGHT;
        }
        if (y >= c->y1 || y + bh <= c->y0 || x >= x_hi || x + bw <= x_lo) {
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
#ifndef HGSS_PROFILE_OFF
            ++s_stats.dropped_quads;
#endif
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
                /* As in text_bg: track blank tiles too, or later OBJ tile
                 * writes into them are invisible to the reuse check. */
                run_note(out, t.slot, off, tile_bytes);
                if (!t.valid || t.transparent) {
                    continue;
                }
                kj_v2d *v = out->v + out->count * 4u;
                if (!affine) {
                    const int qx = x + (hf ? (tw - 1 - tx) : tx) * 8;
                    const int qy = y + (vf ? (th - 1 - ty) : ty) * 8;
                    put_quad(v, qx, qy, 8, 8, t.slot, hf, vf);
                } else {
                    int16_t u0, v0, u1, v1;
                    kj_atlas_slot_uv(t.slot, &u0, &v0, &u1, &v1);
                    const float px[4] = {tx * 8.0f, tx * 8.0f + 8, tx * 8.0f, tx * 8.0f + 8};
                    const float py[4] = {ty * 8.0f, ty * 8.0f, ty * 8.0f + 8, ty * 8.0f + 8};
                    const int16_t uu[4] = {u0, u1, u0, u1};
                    const int16_t vv[4] = {v0, v0, v1, v1};
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
