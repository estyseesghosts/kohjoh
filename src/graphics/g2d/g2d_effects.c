/*
 * g2d_effects.c: DS window, blend and brightness passes and the coordinate
 * mapping they share with the span orchestrator.
 *
 * These passes are PICA state plus full-screen/scissor quads. They consume
 * the per-span kj_g2d_span/kj_g2d_fx snapshot but do not own layer data, so they stay out
 * of the decoder and retained-cache modules.
 */

#include "graphics/g2d/g2d_internal.h"

#include "graphics/pica/gpu.h"
#include "graphics/resources/atlas.h"

#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <stdint.h>

void kj_g2d_set_xform_x(const kj_target *t, float k, float add) {
    /* clip.x = 1 - y_phys/120 ; clip.y = 1 - x_phys/(w/2)
     * y_phys = y * scale ; x_phys = x' * scale + offset */
    const float half_w = t->width / 2.0f;
    C3D_FVUnifSet(GPU_VERTEX_SHADER, kj_gpu_uloc_2d_xform(), -t->scale / 120.0f, 1.0f,
                  -t->scale * k / half_w, 1.0f - (t->offset_x + t->scale * add) / half_w);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, kj_gpu_uloc_2d_uvscale(),
                  1.0f / (KJ_ATLAS_SIZE * KJ_ATLAS_UV_UNIT),
                  -1.0f / (KJ_ATLAS_SIZE * KJ_ATLAS_UV_UNIT), 0.0f, 1.0f);
}

void kj_g2d_set_xform(const kj_target *t) {
    kj_g2d_set_xform_x(t, 1.0f, 0.0f);
}

int kj_g2d_phys_x(const kj_target *t, int x) {
    return (int)lroundf(t->offset_x + (float)x * t->scale);
}

int kj_g2d_phys_y(const kj_target *t, int y) {
    return (int)lroundf((float)y * t->scale);
}

void kj_g2d_read_fx(const kj_g2d_span *c, kj_g2d_fx *f) {
    f->bldcnt = r16(c->regs, 0x50);
    f->mode = (f->bldcnt >> 6) & 3;
    const uint16_t alpha = r16(c->regs, 0x52);
    const int eva = alpha & 31, evb = (alpha >> 8) & 31;
    const int evy = r16(c->regs, 0x54) & 31;
    f->eva = (eva > 16 ? 16 : eva) / 16.0f;
    f->evb = (evb > 16 ? 16 : evb) / 16.0f;
    f->evy = (evy > 16 ? 16 : evy) / 16.0f;
    /* WIN0, WIN1, OBJ window (DISPCNT 13-15); the OBJ window also needs
     * the OBJ layer on (DISPCNT 12). */
    f->windows = (c->dispcnt & (7u << 13)) != 0;
    f->objwin = (c->dispcnt & (1u << 15)) && (c->dispcnt & (1u << 12));
    const uint16_t winin = r16(c->regs, 0x48), winout = r16(c->regs, 0x4A);
    f->region_enable[0] = f->windows ? (uint8_t)(winout & 0x3F) : 0x3F;
    f->region_enable[1] = (uint8_t)(winin & 0x3F);
    f->region_enable[2] = (uint8_t)((winin >> 8) & 0x3F);
    f->region_enable[3] = (uint8_t)((winout >> 8) & 0x3F);
    f->active = (uint8_t)(1u | ((c->dispcnt >> 12) & 6u) | (f->objwin ? 8u : 0u));
}

uint8_t kj_g2d_bg0_window_on(const kj_g2d_fx *f) {
    uint8_t on = 0;
    for (int r = 0; r < 4; ++r) {
        if (f->region_enable[r] & 1u) {
            on |= (uint8_t)(1u << r);
        }
    }
    return on;
}

void kj_g2d_stencil_write_all(uint8_t ref, uint8_t test_mask) {
    C3D_StencilTest(true, test_mask ? GPU_EQUAL : GPU_ALWAYS, ref, test_mask, 0x03);
    C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
}

void kj_g2d_blend_off(void) {
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}

void kj_g2d_blend_alpha(float eva, float evb) {
    const uint32_t ka = (uint32_t)(eva * 255.0f + 0.5f), kb = (uint32_t)(evb * 255.0f + 0.5f);
    C3D_BlendingColor(ka | (ka << 8) | (ka << 16) | (kb << 24));
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_CONSTANT_COLOR, GPU_CONSTANT_ALPHA, GPU_ONE,
                   GPU_ZERO);
}

static bool rect_quad(int x0, int y0, int x1, int y1, uint32_t rgba, uint32_t *first) {
    kj_v2d *v = kj_gpu_quads_alloc(1, first);
    if (v == NULL) {
        return false;
    }
    const uint8_t r = rgba & 255, g = (rgba >> 8) & 255, b = (rgba >> 16) & 255, a = rgba >> 24;
    v[0] = (kj_v2d){(int16_t)x0, (int16_t)y0, 0, 0, r, g, b, a};
    v[1] = (kj_v2d){(int16_t)x1, (int16_t)y0, 0, 0, r, g, b, a};
    v[2] = (kj_v2d){(int16_t)x0, (int16_t)y1, 0, 0, r, g, b, a};
    v[3] = (kj_v2d){(int16_t)x1, (int16_t)y1, 0, 0, r, g, b, a};
    return true;
}

static bool wide_3d(const kj_g2d_span *c) {
    return c->screen == KJ_SCREEN_TOP && (c->dispcnt & 0x108u) == 0x108u;
}

void kj_g2d_open_wide_3d_scissor(const kj_g2d_span *c) {
    if (wide_3d(c) && !c->nrows) {
        kj_gpu_scissor(c->target->width, kj_g2d_phys_x(c->target, KJ_LAYOUT_LEFT),
                       kj_g2d_phys_y(c->target, c->y0), kj_g2d_phys_x(c->target, KJ_LAYOUT_RIGHT),
                       kj_g2d_phys_y(c->target, c->y1));
    }
}

void kj_g2d_write_windows(const kj_g2d_span *c) {
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
        int xs[4][2], ys[2][2], nx = 0, ny = 0;
        if (x1 <= x2) {
            xs[nx][0] = x1;
            xs[nx++][1] = x2;
        } else {
            xs[nx][0] = x1;
            xs[nx++][1] = 256;
            xs[nx][0] = 0;
            xs[nx++][1] = x2;
        }
        if (y1 <= y2) {
            ys[ny][0] = y1;
            ys[ny++][1] = y2;
        } else {
            ys[ny][0] = y1;
            ys[ny++][1] = 192;
            ys[ny][0] = 0;
            ys[ny++][1] = y2;
        }
        /* Wide top screen: a window that reaches a DS screen edge runs on
         * across the side columns, so wipes and covers closing in from the
         * edges hide the widened picture too. The side columns get their
         * own rectangles. A right edge of 255 counts as reaching the edge:
         * the Fly cut-in (field_move_cut_in.c) opens WIN0 from 254 to 255
         * and leaves only DS column 255 outside, a one-pixel edge column on
         * the DS that would otherwise become a strip of field mid-picture. */
        kj_g2d_open_wide_3d_scissor(c);
        if (c->nrows || wide_3d(c)) {
            const int n = nx;
            for (int i = 0; i < n; ++i) {
                if (xs[i][0] >= xs[i][1]) {
                    continue;
                }
                if (xs[i][0] == 0) {
                    xs[nx][0] = -64;
                    xs[nx++][1] = 0;
                }
                if (xs[i][1] >= 255) {
                    /* Column 255 is the DS screen's last column; wide, it
                     * sits mid-picture, so close the seam with the side. */
                    xs[i][1] = 256;
                    xs[nx][0] = 256;
                    xs[nx++][1] = 320;
                }
            }
#ifndef HGSS_PROFILE_OFF
            kj_g2d_counters.wide_windows += (uint32_t)(nx - n);
#endif
        }
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

void kj_g2d_brightness_pass(const kj_g2d_fx *f) {
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
#ifndef HGSS_PROFILE_OFF
    ++kj_g2d_counters.draws;
#endif
}

void kj_g2d_master_brightness(const kj_g2d_span *c, uint16_t mb) {
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
#ifndef HGSS_PROFILE_OFF
    ++kj_g2d_counters.draws;
#endif
}

void kj_g2d_side_black(void) {
    uint32_t q;
    if (!rect_quad(-64, 0, 0, 192, 0xFF000000u, &q)) {
        return;
    }
    uint32_t q2;
    if (!rect_quad(256, 0, 320, 192, 0xFF000000u, &q2)) {
        return;
    }
    kj_gpu_scissor_off();
    kj_gpu_tev_vertex_color();
    kj_g2d_blend_off();
    C3D_StencilTest(false, GPU_ALWAYS, 0, 0, 0);
    kj_gpu_draw_quads(q, 1);
    kj_gpu_draw_quads(q2, 1);
}

/* Window/blend state functions follow. */
