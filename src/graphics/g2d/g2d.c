/*
 * g2d.c: DS 2D engines on the PICA (GPU_2D.md).
 *
 * Translates one engine's state per scanline span into ordered GPU passes:
 *   clear (backdrop colour; stencil = backdrop's effect flags)
 *   window regions -> stencil bits 4-5 (0 outside, 1 WIN0, 2 WIN1, 3 OBJ
 *   window: the opaque pixels of mode-2 OBJs, below WIN1 and WIN0)
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
 *
 * Module boundaries: g2d_layers.c decodes BG/OBJ quads, g2d_retained.c owns
 * the retained caches and byte comparisons, g2d_effects.c owns windows,
 * blending, brightness and the coordinate mapping. Wide-art key/band placement
 * stays with the span because it consumes the same per-span tile/key snapshot;
 * wideart.c remains the owner of the embedded blob, texture and picture
 * selection.
 */

#include "graphics/g2d/g2d.h"
#include "graphics/g2d/g2d_internal.h"

#include "app/script.h"
#include "diagnostics/log.h"
#include "diagnostics/perf.h"
#include "graphics/g2d/wideart.h"
#include "graphics/g3d/g3d.h"
#include "graphics/layout/layout.h"
#include "graphics/layout/override.h"
#include "graphics/pica/gpu.h"
#include "graphics/pica/offscreen.h"
#include "graphics/pica/targets.h"
#include "graphics/font/glyph_atlas.h"
#include "graphics/font/text_window.h"
#include "graphics/font/fontbench.h"
#include "graphics/font/text_render.h"
#include "graphics/resources/atlas.h"
#include "runtime/kj_field_view.h"

#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define LAYER_OBJ 4
#define LAYER_BD 5

kj_g2d_stats s_stats;

/* ------------------------------------------------------------- passes */

/* x' = k * x + add before the target's own mapping (wide layouts: k = 1
 * translates a segment, k > 1 stretches one column across it). */

/* DS column / line -> physical pixel, rounded the same way for every edge
 * so neighbouring scissors abut. */

/* The window regions BG0 (the 3D layer) shows in, as a bit per region for
 * kj_g3d_draw; regions that cannot occur are left to `active`. */

/* Draw quads of one layer in every window region where it is enabled. */
typedef struct drawable {
    const kj_v2d *base; /* NULL: frame arena at `first` */
    uint32_t first;
    uint32_t count;
} drawable;

static void draw_quads(const drawable *d) {
    if (d->base != NULL) {
        kj_gpu_draw_quads_from(d->base, d->count);
    } else {
        kj_gpu_draw_quads(d->first, d->count);
    }
}

static void draw_layer(const fx *f, int layer, const drawable *d, bool force_semi) {
    const uint32_t count = d->count;
    if (count == 0) {
        return;
    }
    const bool t2 = (f->bldcnt >> (8 + layer)) & 1;
    const bool t1 = force_semi || ((f->bldcnt >> layer) & 1);
    for (int r = 0; r < 4; ++r) {
        if (r > 0 && !f->windows) {
            break;
        }
        if (r == 3 && !f->objwin) {
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
            draw_quads(d);
#ifndef HGSS_PROFILE_OFF
            ++s_stats.draws;
#endif
            continue;
        }
        /* A pass that changes bit 0 must run last, or the other pass will
         * draw those same pixels again. The current layer's t2 selects which
         * pass changes it: opaque when t2 is set, blended otherwise. */
        for (int pass = 0; pass < 2; ++pass) {
            const bool blended = (pass == 0) == t2;
            if (blended) {
                blend_alpha(f->eva, f->evb);
                C3D_StencilTest(true, GPU_EQUAL, region | 1u, rmask | 1u, 0x03);
                C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP,
                              t2 ? GPU_STENCIL_KEEP : GPU_STENCIL_INVERT);
            } else {
                blend_off();
                C3D_StencilTest(true, GPU_EQUAL, region, rmask | 1u, 0x01);
                C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP,
                              t2 ? GPU_STENCIL_INVERT : GPU_STENCIL_KEEP);
            }
            draw_quads(d);
        }
#ifndef HGSS_PROFILE_OFF
        s_stats.draws += 2;
#endif
    }
}

/* A rectangle (DS pixels) as one untextured quad in the arena. */

/* Composite the 200x120 world surface at exactly 2x into BG0's place.
 * One textured quad, nearest sampling, integer destination edges. The PICA
 * target is portrait: DS y became texture x (reversed) and DS x became
 * texture y. Undo that here, with a quarter-texel inset so each pair of
 * output rows/columns lands in one source texel. Transparency/stencil
 * coverage is still approximate -- see FIELD_SCALING.md limitations. */
static void composite_world_2x(const ctx *c, uint8_t stencil_flags, uint8_t window_on,
                               uint8_t window_active) {
    uint32_t first;
    kj_v2d *v = kj_gpu_quads_alloc(1, &first);
    if (v == NULL) {
        return;
    }
    const float scale = c->target->scale;
    const float ox = c->target->offset_x;
    const int x0 = (int)lroundf(-ox / scale);
    const int x1 = (int)lroundf(((float)c->target->width - ox) / scale);
    const int y0 = 0;
    const int y1 = (int)lroundf(240.0f / scale);
    const int xs[4] = {x0, x1, x0, x1};
    const int ys[4] = {y0, y0, y1, y1};

    /* The world quad covers all 400x240 output pixels, not just the DS
     * columns used by the surrounding 2D passes. */
    kj_gpu_scissor_off();
    kj_gpu_use_program(KJ_PROGRAM_2D);
    kj_gpu_bind_texture(0, kj_offscreen_texture());
    C3D_FVUnifSet(GPU_VERTEX_SHADER, kj_gpu_uloc_2d_uvscale(), 1.0f / (128.0f * 16.0f),
                  -1.0f / (256.0f * 16.0f), 0.0f, 1.0f);
    kj_gpu_tev_texture_modulate();

    for (int k = 0; k < 4; ++k) {
        const float phys_x = ox + (float)xs[k] * scale;
        const float phys_row = (float)ys[k] * scale;
        /* Exact nearest-neighbour mapping, derived from the corrected
         * make_fixup(): offscreen texel (wx,wy) lives at texture (wy,wx).
         * Both axes advance one whole texel every two output pixels. */
        /* Measured against a same-frame 1x capture (score 1.000): the PICA
         * render-to-texture framebuffer maps the active region into the BOTTOM
         * 200 rows of the 256-row backing texture and reverses the DS-y axis.
         * Undo both exactly: flip u, and offset v by the 56 padding rows. */
        const float u = 8.0f * (240.0f - phys_row);
        const float vv = 8.0f * (phys_x + 112.0f);
        v[k] = (kj_v2d){(int16_t)xs[k],
                        (int16_t)ys[k],
                        (int16_t)lroundf(u),
                        (int16_t)lroundf(vv),
                        255,
                        255,
                        255,
                        255};
    }
    blend_off();
    /* The composite covers the physical display; native window regions must
     * not reject parts of it. Reapplying windows/stencil to this quad is the
     * remaining Task 11 composition work. */
    C3D_StencilTest(false, GPU_ALWAYS, 0, 0, 0);
    kj_gpu_draw_quads(first, 1);
    (void)stencil_flags;
    (void)window_on;
    (void)window_active;
}

/* The top screen's 3D (BG0) is drawn wide, but a screen without layout
 * rows keeps its 2D scissored to the DS columns (restore_2d_state). Passes
 * that must reach the side 3D too (windows, BLDY) open the scissor to the
 * wide range there. */

/* Window rectangles of a span into stencil bits 4-5. WIN1 first, then
 * WIN0 (WIN0 has priority). Coordinates wrap when x1 > x2 / y1 > y2. */

/* BLDY: pixels whose top layer is an effect-enabled 1st target. */

/* MASTER_BRIGHT over the whole physical screen (sides included). */

/* The side columns black (QA switch KJ_DEBUG_NO_SIDE_FILL, issue #21):
 * shows what the widescreen stages add over a plain centred picture. */

/* --------------------------------------------------------------- spans */

/* Lines [y0, y1) whose registers equal line y0's (the bytes the 2D
 * renderer reads: 0x00-0x6F except DISPSTAT/VCOUNT, already zeroed). */
static int span_end(const kj_line_regs *lines, int engine, int y0) {
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

/* ------------------------------------------------------ retained layers */

/*
 * Retained layers (OPT-002/OPT-005). When a frame has no per-line register
 * changes, each text BG and the OBJ layer keep last frame's quads in their
 * own GPU buffer. They are reused only if everything they were built from
 * is byte-identical: the registers they read, the palette, the map (text
 * BG) or OAM (OBJ), extended palettes, and the exact range of character
 * bytes the build read. Comparing a few KiB is far cheaper than ~800 atlas
 * lookups per layer, and it is exact: no write tracking, no hashing.
 */

/* Registers a text BG's quads depend on. */
static void text_key(const ctx *c, int bg, uint8_t key[16]) {
    memset(key, 0, 16);
    memcpy(key, c->regs + 0x08 + 2 * bg, 2);
    memcpy(key + 2, c->regs + 0x10 + 4 * bg, 4);
    const uint32_t d = c->dispcnt & 0x7F000000u; /* char/screen offsets, ext palettes */
    memcpy(key + 6, &d, 4);
    key[10] = (uint8_t)c->engine;
    key[11] = c->nrows ? 1 : 0;                   /* wide quads (-64..320) */
    key[12] = c->screen == KJ_SCREEN_TOP ? 1 : 0; /* wide-art bands: top only */
}

static void text_map_range(const ctx *c, int bg, uint32_t *off, uint32_t *len) {
    const uint16_t cnt = r16(c->regs, 0x08 + 2 * bg);
    uint32_t scr = ((cnt >> 8) & 31u) * 0x800u;
    if (c->engine == 0) {
        scr += ((c->dispcnt >> 27) & 7u) * 0x10000u;
    }
    const unsigned size = cnt >> 14;
    *off = scr & (c->bg_vram_size - 1u);
    *len = (size == 0 ? 1u : size == 3 ? 4u : 2u) * 0x800u;
    if (*off + *len > c->bg_vram_size) {
        *len = c->bg_vram_size - *off;
    }
}

static const uint8_t *text_ext_src(const ctx *c, int bg) {
    const uint16_t cnt = r16(c->regs, 0x08 + 2 * bg);
    if (!(cnt & 0x80) || !(c->dispcnt & (1u << 30))) {
        return NULL;
    }
    const int slot = bg < 2 ? bg + (((cnt >> 13) & 1) ? 2 : 0) : bg;
    return ext_bg_slot(c->in, c->engine, slot);
}

/* ------------------------------------------------------------ wide art */

/*
 * The wide art (issue #14, from Sinjoh at 418dcf4): hand-drawn side bands
 * for a text BG on the top screen. A picture is found by the layer's art
 * key, FNV-1a over the layer's kind and CNT, its map entries, the bytes of
 * the tiles the map uses and the palette banks it uses: what the layer
 * shows, not where it is scrolled. Pictures are named <key>_<hofs>_<vofs>
 * (tools/wideart/pack_wideart.py), and the key of every top-screen text BG
 * is logged once, so a picture can be drawn for any scene.
 *
 * Only retained layers get bands: a frame with per-line register changes
 * draws none (its layers are rebuilt each frame and hashing their maps
 * every frame is not worth it).
 */

#define ART_KEY_START 0xcbf29ce484222325ull
#define ART_KIND_TEXT 1u
#define ART_REST 8 /* how far the layer may scroll from the picture's spot */
#define WIDE_CANVAS_W 320
#define WIDE_BAND_W 32
#define WIDE_ORIGIN 32

static const int s_text_sizes[4][2] = {{256, 256}, {512, 256}, {256, 512}, {512, 512}};
static uint64_t s_logged_key[2][4];
static uint32_t s_art_matched, s_art_missing;

static uint64_t art_key_add(uint64_t h, uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        h = (h ^ (v & 0xFFu)) * 0x100000001b3ull;
        v >>= 8;
    }
    return h;
}

static uint64_t text_art_key(const ctx *c, int bg) {
    const uint16_t cnt = r16(c->regs, 0x08 + 2 * bg);
    const bool bpp8 = (cnt & 0x80u) != 0;
    const uint32_t entries =
        (uint32_t)(s_text_sizes[cnt >> 14][0] / 8) * (uint32_t)(s_text_sizes[cnt >> 14][1] / 8);
    const uint32_t mask = c->bg_vram_size - 1u;
    uint32_t char_base = ((cnt >> 2) & 15u) * 0x4000u;
    uint32_t screen_base = ((cnt >> 8) & 31u) * 0x800u;
    uint32_t used[1024 / 32];
    uint32_t banks = 0;

    if (c->engine == 0) {
        char_base += ((c->dispcnt >> 24) & 7u) * 0x10000u;
        screen_base += ((c->dispcnt >> 27) & 7u) * 0x10000u;
    }
    uint64_t h = art_key_add(ART_KEY_START, ART_KIND_TEXT | ((uint32_t)(cnt & 0xC080u) << 8));
    memset(used, 0, sizeof(used));
    for (uint32_t i = 0; i < entries; ++i) {
        const uint32_t entry = r16(c->bg_vram, (screen_base + i * 2u) & mask);
        const uint32_t tile = entry & 0x3FFu;
        h = art_key_add(h, entry);
        used[tile >> 5] |= 1u << (tile & 31u);
        banks |= 1u << (entry >> 12);
    }
    const uint32_t tile_bytes = bpp8 ? 64u : 32u;
    for (uint32_t i = 0; i < 1024; ++i) {
        if (!(used[i >> 5] & (1u << (i & 31u)))) {
            continue;
        }
        for (uint32_t k = 0; k < tile_bytes; k += 4) {
            h = art_key_add(h, r32(c->bg_vram, (char_base + i * tile_bytes + k) & mask));
        }
    }
    const uint16_t *pal = pltt_ptr(c->in->mem, c->engine, 0);
    const uint8_t *ext = bpp8 ? text_ext_src(c, bg) : NULL;
    if (!bpp8) {
        for (uint32_t i = 0; i < 16; ++i) {
            for (uint32_t k = 0; (banks & (1u << i)) && k < 16; ++k) {
                h = art_key_add(h, pal[i * 16u + k] & 0x7FFFu);
            }
        }
    } else if (ext != NULL) {
        for (uint32_t i = 0; i < 16; ++i) {
            for (uint32_t k = 0; (banks & (1u << i)) && k < 256; ++k) {
                h = art_key_add(h, r16(ext, (i << 9) + k * 2u) & 0x7FFFu);
            }
        }
    } else {
        for (uint32_t i = 0; i < 256; ++i) {
            h = art_key_add(h, pal[i] & 0x7FFFu);
        }
    }
    return h != 0 ? h : 1;
}

static void put_band_quad(kj_v2d *v, int x, int w, int h, int tx, int ty) {
    const int16_t u0 = (int16_t)(tx * KJ_ATLAS_UV_UNIT + 1);
    const int16_t u1 = (int16_t)((tx + w) * KJ_ATLAS_UV_UNIT - 1);
    const int16_t v0 = (int16_t)(ty * KJ_ATLAS_UV_UNIT + 1);
    const int16_t v1 = (int16_t)((ty + h) * KJ_ATLAS_UV_UNIT - 1);
    v[0] = (kj_v2d){(int16_t)x, 0, u0, v0, 255, 255, 255, 255};
    v[1] = (kj_v2d){(int16_t)(x + w), 0, u1, v0, 255, 255, 255, 255};
    v[2] = (kj_v2d){(int16_t)x, (int16_t)h, u0, v1, 255, 255, 255, 255};
    v[3] = (kj_v2d){(int16_t)(x + w), (int16_t)h, u1, v1, 255, 255, 255, 255};
}

/* The picture's bands as quads (at most 2), shifted by dx: how far the
 * layer has scrolled right since the picture was made. */
static uint32_t band_quads(const kj_wideart_pic *pic, int dx, kj_v2d *v) {
    const int bh = pic->band_h;
    uint32_t n = 0;
    int lo = dx > 0 ? 0 : -dx;
    int hi = dx > 0 ? WIDE_BAND_W - dx : WIDE_BAND_W;

    if (hi > lo) {
        put_band_quad(v, lo - WIDE_ORIGIN, hi - lo, bh, pic->left_x + lo + dx, pic->left_y);
        ++n;
    }
    const int rx0 = WIDE_CANVAS_W - WIDE_BAND_W;
    lo = dx > 0 ? rx0 : rx0 - dx;
    hi = dx > 0 ? WIDE_CANVAS_W - dx : WIDE_CANVAS_W;
    if (hi > lo) {
        put_band_quad(v + n * 4u, lo - WIDE_ORIGIN, hi - lo, bh, pic->right_x + lo + dx - rx0,
                      pic->right_y);
        ++n;
    }
    return n;
}

/* A rebuilt layer's bands: its key, logged once, and its picture's quads. */
static uint32_t layer_bands(const ctx *c, int bg, kj_v2d *v) {
    const uint64_t key = text_art_key(c, bg);
    const kj_wideart_pic *pic = kj_wideart_find(key);

    const bool fresh = s_logged_key[c->engine][bg] != key;
    if (fresh) {
        s_logged_key[c->engine][bg] = key;
        /* <key>_<hofs>_<vofs>.png is the name a picture for this layer
         * as it is now would have. */
        kj_log("[wide art] engine %d bg %d key %016llx_%d_%d%s", c->engine, bg,
               (unsigned long long)key, r16(c->regs, 0x10 + 4 * bg) & 0x1FF,
               r16(c->regs, 0x12 + 4 * bg) & 0x1FF, pic != NULL ? " (a picture)" : "");
    }
    if (pic == NULL) {
        ++s_art_missing;
        return 0;
    }
    const uint16_t cnt = r16(c->regs, 0x08 + 2 * bg);
    int dx = 0;
    pic = kj_wideart_find_near(key, r16(c->regs, 0x10 + 4 * bg) & 0x1FF,
                               r16(c->regs, 0x12 + 4 * bg) & 0x1FF, s_text_sizes[cnt >> 14][0],
                               s_text_sizes[cnt >> 14][1], 256, 192, &dx);
    if (pic == NULL || (!(pic->flags & KJ_WIDE_ART_LOOP) && (dx < -ART_REST || dx > ART_REST))) {
        if (fresh) {
            kj_log("[wide art] engine %d bg %d: no picture near this scroll", c->engine, bg);
        }
        return 0;
    }
    ++s_art_matched;
    return band_quads(pic, dx, v);
}

/* A text BG, retained when possible. */
static bool text_bg_retained(const ctx *c, int bg, drawable *out, drawable *band) {
    layer_cache *lc = &s_bg_cache[c->engine][bg];
    const uint16_t *pal = pltt_ptr(c->in->mem, c->engine, 0);
    uint8_t key[16];
    uint32_t moff, mlen;

    text_key(c, bg, key);
    text_map_range(c, bg, &moff, &mlen);
    const uint8_t *ext = text_ext_src(c, bg);
    if (lc->valid && memcmp(lc->key, key, 16) == 0 && lc->map_off == moff && lc->map_len == mlen &&
        memcmp(lc->pal, pal, 512) == 0 && memcmp(lc->map, c->bg_vram + moff, mlen) == 0 &&
        lc->ext_src == ext && (ext == NULL || memcmp(lc->ext, ext, 0x2000) == 0) &&
        kj_g2d_chars_same(lc, c->bg_vram)) {
        out->base = lc->quads;
        out->count = lc->count;
        band->base = lc->band;
        band->count = lc->band_count;
#ifndef HGSS_PROFILE_OFF
        ++s_reused_layers;
#endif
        return true;
    }
    if (!kj_g2d_cache_alloc(lc)) {
        return false;
    }
    lc->band_count = 0;
    if (c->screen == KJ_SCREEN_TOP && !(kj_script_debug() & KJ_DEBUG_NO_WIDEART) &&
        (kj_wideart_count() > 0 || (kj_script_debug() & KJ_DEBUG_ART_KEYS))) {
        if (lc->band == NULL) {
            lc->band = linearAlloc(2u * 4u * sizeof(kj_v2d));
        }
        if (lc->band != NULL) {
            lc->band_count = layer_bands(c, bg, lc->band);
        }
    }
    band->base = lc->band;
    band->count = lc->band_count;
    run r;
    memset(&r, 0, sizeof(r));
    r.ext = true;
    r.v = lc->quads;
    r.cap = CACHE_QUADS;
    r.slots = lc->slots;
    text_bg(c, bg, &r);
    lc->valid = true;
    memcpy(lc->key, key, 16);
    memcpy(lc->pal, pal, 512);
    lc->map_off = moff;
    lc->map_len = mlen;
    memcpy(lc->map, c->bg_vram + moff, mlen);
    lc->ext_src = ext;
    if (ext != NULL) {
        memcpy(lc->ext, ext, 0x2000);
    }
    kj_g2d_chars_keep(lc, c->bg_vram, r.char_lo, r.char_hi);
    lc->count = r.count;
    out->base = lc->quads;
    out->count = r.count;
    ++s_rebuilt_layers;
    return true;
}

/* The OBJ layer (all priorities), retained when possible. */
static bool objs_retained(const ctx *c, drawable sub[9]) {
    layer_cache *lc = &s_obj_cache[c->engine];
    const uint8_t *oam = c->in->mem->oam + c->engine * 0x400;
    const uint16_t *pal = pltt_ptr(c->in->mem, c->engine, 1);
    const uint8_t *ext = (c->dispcnt & (1u << 31)) ? ext_obj(c->in, c->engine) : NULL;
    uint8_t key[16];

    memset(key, 0, sizeof(key));
    const uint32_t d = c->dispcnt & 0x80301010u; /* OBJ on, mapping, boundary, ext */
    memcpy(key, &d, 4);
    key[4] = (uint8_t)c->engine;
    if (c->nrows) {
        /* OBJS moves depend on the layout, which the generation names. */
        const uint32_t gen = c->in->screen.generation;
        key[5] = 1;
        memcpy(key + 8, &gen, 4);
    }
    const int8_t *wide_dx = c->in->mem->oam_wide_dx;
    const bool use_wide_dx = c->battle_obj_anchors && wide_dx != NULL;
    key[6] = use_wide_dx;
    if (lc->valid && memcmp(lc->key, key, 16) == 0 && memcmp(lc->oam, oam, 0x400) == 0 &&
        (!use_wide_dx || memcmp(lc->oam_wide_dx, wide_dx + c->engine * 128, 128) == 0) &&
        memcmp(lc->pal, pal, 512) == 0 && lc->ext_src == ext &&
        (ext == NULL || memcmp(lc->ext, ext, 0x2000) == 0) && kj_g2d_chars_same(lc, c->obj_vram)) {
        for (int i = 0; i < 9; ++i) {
            sub[i].base = lc->quads + lc->sub_first[i] * 4u;
            sub[i].count = lc->sub_count[i];
        }
#ifndef HGSS_PROFILE_OFF
        ++s_reused_layers;
#endif
        return true;
    }
    if (!kj_g2d_cache_alloc(lc)) {
        return false;
    }
    uint32_t used = 0, lo = UINT32_MAX, hi = 0;
    for (int i = 0; i < 9; ++i) {
        run r;
        memset(&r, 0, sizeof(r));
        r.ext = true;
        r.v = lc->quads + used * 4u;
        r.cap = CACHE_QUADS - used;
        r.slots = lc->slots + used;
        objs(c, i < 8 ? i >> 1 : -1, (i & 1) != 0, &r);
        lc->sub_first[i] = used;
        lc->sub_count[i] = r.count;
        used += r.count;
        if (r.count) {
            lo = r.char_lo < lo ? r.char_lo : lo;
            hi = r.char_hi > hi ? r.char_hi : hi;
        }
    }
    lc->valid = true;
    memcpy(lc->key, key, 16);
    memcpy(lc->oam, oam, 0x400);
    if (use_wide_dx) {
        memcpy(lc->oam_wide_dx, wide_dx + c->engine * 128, 128);
    }
    memcpy(lc->pal, pal, 512);
    lc->ext_src = ext;
    if (ext != NULL) {
        memcpy(lc->ext, ext, 0x2000);
    }
    kj_g2d_chars_keep(lc, c->obj_vram, lo == UINT32_MAX ? 0 : lo, hi);
    lc->count = used;
    for (int i = 0; i < 9; ++i) {
        sub[i].base = lc->quads + lc->sub_first[i] * 4u;
        sub[i].count = lc->sub_count[i];
    }
    ++s_rebuilt_layers;
    return true;
}

void kj_g2d_begin_frame(void) {
    /* Retained quads point at atlas slots: keep them alive before any
     * allocation this frame can evict them. */
    for (int e = 0; e < 2; ++e) {
        for (int b = 0; b < 4; ++b) {
            if (s_bg_cache[e][b].valid) {
                kj_atlas_touch(s_bg_cache[e][b].slots, s_bg_cache[e][b].count);
            }
        }
        if (s_obj_cache[e].valid) {
            kj_atlas_touch(s_obj_cache[e].slots, s_obj_cache[e].count);
        }
    }
}

/* --------------------------------------------------------------- spans */

typedef struct prepared {
    drawable bg[4];
    drawable band[4]; /* wide-art bands (retained top-screen text BGs) */
    bool bg_ok[4];
    bool bg_text[4];
    drawable obj[9]; /* priority * 2 + semi; [8]: OBJ-window OBJs */
    bool obj_ok;
    bool all_reused;
} prepared;

static void prepare_span(const ctx *c, bool retain, prepared *pr) {
    const int bgmode = (int)(c->dispcnt & 7u);
    const uint32_t layers_on = (c->dispcnt >> 8) & 0x1Fu;
    const bool bg0_3d = c->engine == 0 && (c->dispcnt & 8u);
    const uint32_t rebuilt_before = s_rebuilt_layers;

    memset(pr, 0, sizeof(*pr));
    for (int bg = 0; bg < 4; ++bg) {
        if (!(layers_on & (1u << bg)) || (bg == 0 && bg0_3d)) {
            continue;
        }
        bool affine = false, ext16 = false, unsupported = false;
        switch (bgmode) {
        case 0:
            break;
        case 1:
            affine = bg == 3;
            break;
        case 2:
            affine = bg >= 2;
            break;
        case 3:
            ext16 = bg == 3;
            break;
        case 4:
            affine = bg == 2;
            ext16 = bg == 3;
            break;
        case 5:
            ext16 = bg >= 2;
            break;
        default:
            unsupported = bg >= 2;
            break;
        }
        if (ext16 && (r16(c->regs, 0x08 + 2 * bg) & 0x80)) {
            unsupported = true; /* bitmap BG (256-colour / direct) */
        }
        if (unsupported) {
#ifndef HGSS_PROFILE_OFF
            ++s_stats.unsupported_modes;
#endif
            continue;
        }
        pr->bg_text[bg] = !affine && !ext16;
        if (!affine && !ext16 && retain) {
            pr->bg_ok[bg] = text_bg_retained(c, bg, &pr->bg[bg], &pr->band[bg]);
        } else {
            run r;
            memset(&r, 0, sizeof(r));
            if (affine || ext16) {
                affine_bg(c, bg, ext16, &r);
            } else {
                text_bg(c, bg, &r);
            }
            if (r.v != NULL) {
                run_end(&r);
                pr->bg[bg].base = NULL;
                pr->bg[bg].first = r.first;
                pr->bg[bg].count = r.count;
                pr->bg_ok[bg] = true;
            }
            ++s_rebuilt_layers;
        }
#ifndef HGSS_PROFILE_OFF
        s_stats.bg_quads += pr->bg[bg].count;
#endif
        kj_perf_count(KJ_COUNT_QUADS, pr->bg[bg].count);
    }
    if (layers_on & 0x10u) {
        if (retain) {
            pr->obj_ok = objs_retained(c, pr->obj);
        } else {
            pr->obj_ok = true;
            for (int i = 0; i < 9; ++i) {
                run r;
                memset(&r, 0, sizeof(r));
                objs(c, i < 8 ? i >> 1 : -1, (i & 1) != 0, &r);
                if (r.v != NULL) {
                    run_end(&r);
                }
                pr->obj[i].base = NULL;
                pr->obj[i].first = r.first;
                pr->obj[i].count = r.v != NULL ? r.count : 0;
            }
            ++s_rebuilt_layers;
        }
        for (int i = 0; i < 9; ++i) {
#ifndef HGSS_PROFILE_OFF
            s_stats.obj_quads += pr->obj[i].count;
#endif
            kj_perf_count(KJ_COUNT_QUADS, pr->obj[i].count);
        }
    }
    pr->all_reused = retain && s_rebuilt_layers == rebuilt_before;
}

static void restore_2d_state(const ctx *c, int sx0) {
    if (c->nrows) {
        kj_gpu_scissor(c->target->width, phys_x(c->target, KJ_LAYOUT_LEFT),
                       phys_y(c->target, c->y0), phys_x(c->target, KJ_LAYOUT_RIGHT),
                       phys_y(c->target, c->y1));
    } else {
        kj_gpu_scissor(c->target->width, sx0, (int)(c->y0 * c->target->scale),
                       sx0 + (int)(256 * c->target->scale), (int)(c->y1 * c->target->scale));
    }
    kj_gpu_use_program(KJ_PROGRAM_2D);
    set_xform(c->target);
    kj_gpu_bind_texture(0, kj_atlas_texture());
    kj_gpu_tev_texture_modulate();
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_AlphaTest(true, GPU_GREATER, 0);
    C3D_CullFace(GPU_CULL_NONE);
}

/* Native text: set a physical-pixel transform, draw the supplied vertices,
 * then restore the DS atlas mapping so later layers are unaffected.
 * `textured` selects the font atlas and texel UVs; otherwise the vertices are
 * solid fills drawn with the vertex-colour material. */
void kj_g2d_text_draw(const kj_target *t, const kj_v2d *v, uint32_t vertex_count, bool textured) {
    const kj_font_atlas *fa = kj_font_atlas_get();
    kj_target pt;
    kj_v2d *dst;
    uint32_t first;

    if (t == NULL || v == NULL || vertex_count < 4 || (vertex_count & 3u) != 0) {
        return;
    }
    if (textured && (!kj_text_texture_ready() || fa == NULL)) {
        return;
    }
    /* Copy into the per-frame arena. The GPU reads vertex memory when it
     * executes the command list (at FrameEnd), not when the command is
     * recorded, so the caller's scratch buffer would be overwritten by its
     * next draw and only the last draw of the pass would survive. */
    dst = kj_gpu_quads_alloc(vertex_count / 4u, &first);
    if (dst == NULL) {
        return;
    }
    memcpy(dst, v, (size_t)vertex_count * sizeof(*dst));
    pt = *t;
    pt.scale = 1.0f;
    pt.offset_x = 0.0f;
    /* The previous pass may have left the 3D program bound (world layer).
     * Our vertices are the 2D layout and must use the 2D attribute info. */
    kj_gpu_use_program(KJ_PROGRAM_2D);
    set_xform(&pt);
    if (textured) {
        kj_gpu_bind_texture(0, kj_text_texture());
        C3D_FVUnifSet(GPU_VERTEX_SHADER, kj_gpu_uloc_2d_uvscale(),
                      1.0f / (float)(fa->page_w * KJ_ATLAS_UV_UNIT),
                      -1.0f / (float)(fa->page_h * KJ_ATLAS_UV_UNIT), 0.0f, 1.0f);
        kj_gpu_tev_texture_modulate();
    } else {
        kj_gpu_tev_vertex_color();
    }
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_StencilTest(false, GPU_ALWAYS, 0, 0, 0);
    C3D_AlphaTest(true, GPU_GREATER, 0);
    /* The brightness pass can leave constant-alpha blending armed; text is
     * opaque over its own backing and must not inherit it. */
    blend_off();
    kj_gpu_draw_quads(first, vertex_count / 4u);
#ifndef HGSS_PROFILE_OFF
    ++s_stats.draws;
#endif
    set_xform(t);
    kj_gpu_bind_texture(0, kj_atlas_texture());
    kj_gpu_tev_texture_modulate();
}

/* A text BG under a wide layout: per band of lines with the same active
 * rows, its quads once per segment, translated or stretched under the
 * segment's scissor (layout.c). */
static void draw_layer_wide(const ctx *c, const fx *f, int bg, const drawable *d) {
    const kj_target *t = c->target;
    const uint16_t cnt = r16(c->regs, 0x08 + 2 * bg);
    const kj_layout_bg g = {bg, r16(c->regs, 0x10 + 4 * bg) & 0x1FF,
                            r16(c->regs, 0x12 + 4 * bg) & 0x1FF, (cnt >> 14) & 1 ? 512 : 256,
                            (cnt >> 14) & 2 ? 512 : 256};
    if (d->count == 0) {
        return;
    }
    for (int y = c->y0; y < c->y1;) {
        const uint32_t act = kj_layout_active(c->rows, c->nrows, &g, y);
        int y1 = y + 1;
        while (y1 < c->y1 && kj_layout_active(c->rows, c->nrows, &g, y1) == act) {
            ++y1;
        }
        kj_layout_seg seg[KJ_LAYOUT_MAX_SEGS];
        bool unsupported;
        const int n = kj_layout_segments(c->rows, c->nrows, act, &g, seg, &unsupported);
#ifndef HGSS_PROFILE_OFF
        s_stats.unsupported_modes += unsupported ? 1u : 0u;
        s_stats.layout_segments += (uint32_t)n;
#endif
        for (int k = 0; k < n; ++k) {
            const int s0 = seg[k].s0, s1 = seg[k].s1;
            kj_gpu_scissor(t->width, phys_x(t, s0), phys_y(t, y), phys_x(t, s1), phys_y(t, y1));
            if (seg[k].stretch == 2) {
                set_xform_x(t, (float)(s1 - s0) / 256.0f, (float)s0);
            } else if (seg[k].stretch) {
                const float kx = (float)(s1 - s0);
                set_xform_x(t, kx, (float)s0 - (float)seg[k].sx * kx);
            } else {
                set_xform_x(t, 1.0f, (float)(s0 - seg[k].sx));
            }
            draw_layer(f, bg, d, false);
        }
        y = y1;
    }
    set_xform(t);
    restore_2d_state(c, (int)t->offset_x);
}

static void draw_span(ctx *c, const prepared *pr, bool draw_3d, bool *drew_3d) {
    fx f;
    read_fx(c, &f);
    const uint32_t layers_on = (c->dispcnt >> 8) & 0x1Fu;
    const bool bg0_3d = c->engine == 0 && (c->dispcnt & 8u);
    const int sx0 = (int)(c->target->offset_x);

    restore_2d_state(c, sx0);
    if (f.windows) {
        if (f.objwin && pr->obj_ok && pr->obj[8].count) {
            /* Opaque texels of the window OBJs (alpha test) stamp region 3
             * into stencil bits 4-5; no colour writes. WIN1/WIN0 follow
             * and override it, matching DS window priority. */
            C3D_DepthTest(false, GPU_ALWAYS, 0);
            blend_off();
            C3D_StencilTest(true, GPU_ALWAYS, 0x30, 0, 0x30);
            C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
            draw_quads(&pr->obj[8]);
        }
        write_windows(c);
        restore_2d_state(c, sx0);
    }
    for (int p = 3; p >= 0; --p) {
        for (int bg = 3; bg >= 0; --bg) {
            if (!(layers_on & (1u << bg)) || (r16(c->regs, 0x08 + 2 * bg) & 3) != p) {
                continue;
            }
            if (bg == 0 && bg0_3d) {
                if (draw_3d && !*drew_3d) {
                    const bool t2 = (f.bldcnt >> 8) & 1;
                    const bool t1 = f.bldcnt & 1;
                    const bool world_2x =
                        c->screen == KJ_SCREEN_TOP && kj_offscreen_ready() &&
                        kj_field_view_current()->effective_mode == KJ_FIELD_VIEW_2X;
                    if (world_2x) {
                        kj_offscreen_begin_world(0);
                    } else {
                        kj_gpu_scissor_off();
                    }
                    /* SHIFT3D: the 3D picture moves on the target only; the
                     * DS matrices, viewport and box tests are untouched. */
                    kj_target shifted = *c->target;
                    if (c->nrows) {
                        const float dx = (float)kj_layout_shift3d(c->rows, c->nrows);
                        shifted.offset_x += dx * shifted.scale;
                        shifted.world.offset_x += dx * shifted.world.scale;
                    }
                    const uint8_t stencil = (uint8_t)((t2 ? 1 : 0) | (t1 && f.mode >= 2 ? 2 : 0));
                    const uint8_t won = bg0_window_on(&f);
                    const uint8_t wactive = f.windows ? f.active : 0;
                    kj_g3d_draw(&shifted, c->screen == KJ_SCREEN_TOP, stencil, won, wactive);
                    if (world_2x) {
                        kj_offscreen_end_world(c->target->rt);
                    }
                    *drew_3d = true;
                    restore_2d_state(c, sx0);
                    if (world_2x) {
                        composite_world_2x(c, stencil, won, wactive);
                        /* The composite leaves the offscreen texture, its UV
                         * scale and an open scissor bound. Later layers (OBJ
                         * text, area banners) would otherwise sample with the
                         * wrong transform and render garbled. */
                        restore_2d_state(c, sx0);
                    }
                }
                continue;
            }
            if (!pr->bg_ok[bg]) {
                continue;
            }
            if (c->nrows && pr->bg_text[bg]) {
                draw_layer_wide(c, &f, bg, &pr->bg[bg]);
            } else if (c->nrows) {
                /* Affine BGs keep the DS's 256 columns (no layout rule
                 * maps them yet: counted, drawn unchanged). */
                kj_gpu_scissor(c->target->width, phys_x(c->target, 0), phys_y(c->target, c->y0),
                               phys_x(c->target, 256), phys_y(c->target, c->y1));
                draw_layer(&f, bg, &pr->bg[bg], false);
                restore_2d_state(c, sx0);
            } else {
                draw_layer(&f, bg, &pr->bg[bg], false);
            }
            /* Native text belongs to this BG's priority slot: after the
             * background, before any later (higher-priority) layer. It is a
             * no-op when no window on this plane is presented natively. */
            kj_text_draw_bg(c->target, c->engine, bg, pltt_ptr(c->in->mem, c->engine, 0),
                            r16(c->regs, 0x10 + 4 * bg) & 0x1FFu,
                            r16(c->regs, 0x12 + 4 * bg) & 0x1FFu);
            if (pr->band[bg].count != 0 && kj_wideart_texture() != NULL) {
                /* The bands lie outside the DS columns: open the scissor
                 * to the wide screen and sample the wide-art texture. */
                kj_gpu_bind_texture(0, kj_wideart_texture());
                C3D_FVUnifSet(GPU_VERTEX_SHADER, kj_gpu_uloc_2d_uvscale(),
                              1.0f / (float)(kj_wideart_texture_width() * KJ_ATLAS_UV_UNIT),
                              -1.0f / (float)(kj_wideart_texture_height() * KJ_ATLAS_UV_UNIT), 0.0f,
                              1.0f);
                kj_gpu_scissor(c->target->width, phys_x(c->target, KJ_LAYOUT_LEFT),
                               phys_y(c->target, c->y0), phys_x(c->target, KJ_LAYOUT_RIGHT),
                               phys_y(c->target, c->y1));
                draw_layer(&f, bg, &pr->band[bg], false);
                restore_2d_state(c, sx0);
            }
        }
        if (pr->obj_ok) {
            draw_layer(&f, LAYER_OBJ, &pr->obj[p * 2], false);
            draw_layer(&f, LAYER_OBJ, &pr->obj[p * 2 + 1], true);
        }
    }
    /* (A Gym Leader's VS wipe darkens the field around its band: the side
     * 3D darkens with the middle.) */
    open_wide_3d_scissor(c);
    brightness_pass(&f);
}

static uint32_t ds_to_rgba(uint16_t c) {
    const uint32_t r = c & 31u, g = (c >> 5) & 31u, b = (c >> 10) & 31u;
    return ((r << 3) | (r >> 2)) << 24 | ((g << 3) | (g >> 2)) << 16 | ((b << 3) | (b >> 2)) << 8 |
           0xFFu;
}

void kj_screen_draw(int screen, int engine, const kj_frame_input *in) {
    const kj_target *target = kj_target_get(screen);
    const kj_ds_memory *mem = in->mem;
    screen_memo *memo = &s_memo[screen];
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
    if (screen == KJ_SCREEN_TOP) {
        const kj_layout_row *rows = in->screen.rows[engine];
        uint32_t n = in->screen.row_count[engine];
        bool overridden = kj_layout_override_get(in->screen.screen_id, engine, &rows, &n);
        c.battle_obj_anchors = !overridden && engine == 0 &&
                               in->screen.screen_id == KJ_SCREEN_BATTLE &&
                               !(kj_script_debug() & KJ_DEBUG_NO_LAYOUT);
        if (rows != NULL && n > 0 && !(kj_script_debug() & KJ_DEBUG_NO_LAYOUT)) {
            c.rows = rows;
            c.nrows = n < KJ_LAYOUT_MAX_ROWS ? n : KJ_LAYOUT_MAX_ROWS;
        }
    }

    const uint8_t *io = mem->io + engine * 0x1000;
    const uint32_t dispcnt = r32(io, 0);
    const uint16_t *pal = pltt_ptr(mem, engine, 0); /* BG then OBJ: 512 colours */
    const uint16_t bldcnt = r16(io, 0x50);
    const uint8_t bd_flags =
        (uint8_t)(((bldcnt >> 13) & 1) | ((bldcnt & 0x20) && ((bldcnt >> 6) & 3) >= 2 ? 2 : 0));
    const int display_mode = (int)((dispcnt >> 16) & 3u);
    const bool forced_blank = (dispcnt & 0x80u) != 0;
    const bool has_3d = engine == 0 && (dispcnt & 8u) && in->g3 != NULL;
    const bool single_span = !in->lines.active;

    /* Development specimen mode: the top screen becomes the font bench and
     * the game keeps running underneath on the other screen. Taking the pass
     * here is the only way to guarantee the specimen appears while the game
     * has the top screen blanked or unchanged. */
    if (screen == KJ_SCREEN_TOP && kj_script_fontbench()) {
        kj_gpu_invalidate();
        C3D_FrameDrawOn(target->rt);
        C3D_RenderTargetClear(target->rt, C3D_CLEAR_ALL, 0x000000FFu, 0);
        kj_fontbench_draw(target);
        memo->drawn = false;
        return;
    }

    /* Build or reuse the layers before touching the GPU: an unchanged
     * screen is then not drawn, cleared or transferred at all (its last
     * picture stays on the display). */
    prepared pr;
    bool prepared_ok = false;
    uint64_t t0 = kj_perf_now();
    (void)t0;
    if (single_span && display_mode == 1 && !forced_blank) {
        c.y0 = 0;
        c.y1 = 192;
        c.regs = io;
        c.dispcnt = dispcnt;
        prepare_span(&c, true, &pr);
        prepared_ok = true;
        if (pr.all_reused && !has_3d && memo->drawn && memo->engine == engine &&
            memo->layout_generation == in->screen.generation &&
            memcmp(memo->regs, io, KJ_LINE_REG_BYTES) == 0 && memcmp(memo->pal, pal, 1024) == 0) {
#ifndef HGSS_PROFILE_OFF
            ++s_skipped_screens;
#endif
            kj_perf_add(KJ_PERF_G2D, kj_perf_now() - t0);
            return;
        }
    }
    memo->drawn = false;

    kj_gpu_invalidate();
    C3D_FrameDrawOn(target->rt);

    if (forced_blank || display_mode == 0) {
        C3D_RenderTargetClear(target->rt, C3D_CLEAR_ALL, 0xFFFFFFFFu, 0);
        kj_gpu_use_program(KJ_PROGRAM_2D);
        set_xform(target);
        master_brightness(&c, r16(io, 0x6C));
        kj_perf_add(KJ_PERF_G2D, kj_perf_now() - t0);
        return;
    }
    if (display_mode != 1) {
#ifndef HGSS_PROFILE_OFF
        ++s_stats.unsupported_modes; /* VRAM / main-memory display: tracked blocker */
#endif
        C3D_RenderTargetClear(target->rt, C3D_CLEAR_ALL, 0x000000FFu, 0);
        kj_perf_add(KJ_PERF_G2D, kj_perf_now() - t0);
        return;
    }

    /* Depth clear: far is 0 in Z mode, 1 in W mode (g3d.c). */
    const u32 far_depth = has_3d && kj_g3d_wbuffer() ? 0xFFFFFFu : 0u;
    C3D_RenderTargetClear(target->rt, C3D_CLEAR_ALL, ds_to_rgba(pal[0]),
                          ((u32)bd_flags << 24) | far_depth);

    bool drew_3d = false;
    if (prepared_ok) {
        draw_span(&c, &pr, has_3d, &drew_3d);
#ifndef HGSS_PROFILE_OFF
        ++s_stats.spans;
#endif
        kj_perf_count(KJ_COUNT_SPANS, 1);
    } else {
        for (int y0 = 0; y0 < 192;) {
            const int y1 = span_end(&in->lines, engine, y0);
            c.y0 = y0;
            c.y1 = y1;
            c.regs = in->lines.active ? in->lines.regs[engine][y0] : io;
            c.dispcnt = r32(c.regs, 0);
            prepare_span(&c, false, &pr);
            draw_span(&c, &pr, has_3d, &drew_3d);
#ifndef HGSS_PROFILE_OFF
            ++s_stats.spans;
#endif
            kj_perf_count(KJ_COUNT_SPANS, 1);
            y0 = y1;
        }
    }
    kj_gpu_scissor_off();
    kj_gpu_use_program(KJ_PROGRAM_2D);
    set_xform(target);
    if (screen == KJ_SCREEN_TOP && (kj_script_debug() & KJ_DEBUG_NO_SIDE_FILL)) {
        side_black();
    }
    master_brightness(&c, r16(io, 0x6C));

    memo->drawn = single_span;
    memo->engine = engine;
    memo->layout_generation = in->screen.generation;
    memcpy(memo->regs, io, KJ_LINE_REG_BYTES);
    memcpy(memo->pal, pal, 1024);
    kj_perf_add(KJ_PERF_G2D, kj_perf_now() - t0);
}

/* ---------------------------------------------------------------- init */

bool kj_g2d_init(void) {
    kj_g2d_layers_reset();
    if (!kj_gpu_init()) {
        return false;
    }
    if (!kj_atlas_init()) {
        kj_gpu_exit();
        return false;
    }
    /* Native text is optional: an atlas failure keeps the legacy renderer. */
    if (!kj_text_render_init()) {
        kj_log("[text] native text disabled (font atlas init failed)");
    }
    kj_wideart_init(); /* optional: without it the sides keep their fills */
    return true;
}

void kj_g2d_exit(void) {
    kj_log("[wide art] %lu layer builds with a picture, %lu without", (unsigned long)s_art_matched,
           (unsigned long)s_art_missing);
    kj_wideart_exit();
    kj_fontbench_exit();
    kj_text_render_exit();
    /* Release retained-layer storage before the atlas and GPU arena go away:
     * the quads point at atlas slots and the band uses the same linear heap. */
    for (int e = 0; e < 2; ++e) {
        for (int bg = 0; bg < 4; ++bg) {
            kj_g2d_layer_cache_reset(&s_bg_cache[e][bg]);
        }
        kj_g2d_layer_cache_reset(&s_obj_cache[e]);
    }
    memset(s_memo, 0, sizeof(s_memo));
    s_skipped_screens = s_reused_layers = s_rebuilt_layers = 0;
    s_char_bytes_compared = 0;
    kj_atlas_exit();
    kj_gpu_exit();
}

void kj_g2d_take_stats(kj_g2d_stats *out) {
    s_stats.skipped_screens = s_skipped_screens;
    s_stats.reused_layers = s_reused_layers;
    s_stats.rebuilt_layers = s_rebuilt_layers;
    s_stats.char_bytes_compared = s_char_bytes_compared;
    s_skipped_screens = s_reused_layers = s_rebuilt_layers = 0;
    s_char_bytes_compared = 0;
    *out = s_stats;
    out->dropped_quads += kj_gpu_quads_dropped();
    memset(&s_stats, 0, sizeof(s_stats));
}
