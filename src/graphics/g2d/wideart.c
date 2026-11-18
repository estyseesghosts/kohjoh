/*
 * wideart.c: the wide art's pictures: the table and the texture.
 *
 * From Sinjoh (engine/graphics/g2d/wideart.c at 418dcf4, GPL-3.0, same
 * authors; kohjoh issue #14). Unchanged except an empty blob is "no
 * pictures" and the temporary texel dump is gone.
 *
 * The pictures are made at package time (tools/wideart/pack_wideart.py) and
 * come in as one blob: a header, a table of pictures (the layer's art key
 * and where each band is in the texture) and the texture itself, already in
 * the PICA's tile order. Nothing is decoded or converted here: the console
 * uploads the texture and draws the bands.
 */

#include "graphics/g2d/wideart.h"

#include "diagnostics/log.h"

#include <3ds.h>
#include <stdlib.h>
#include <string.h>

/* The blob the build makes (bin2s). */
extern const unsigned char wideart_bin[];
extern const unsigned int wideart_bin_size;

#define BLOB_HEADER 16u
#define BLOB_RECORD 32u

static C3D_Tex s_tex;
static kj_wideart_pic *s_pics;
static uint32_t s_count;
static uint32_t s_tex_w, s_tex_h;
static bool s_ready;
static uint16_t rd16(const unsigned char *p, unsigned off)
{
    return (uint16_t)(p[off] | (p[off + 1] << 8));
}

static uint32_t rd32(const unsigned char *p, unsigned off)
{
    return (uint32_t)p[off] | ((uint32_t)p[off + 1] << 8) | ((uint32_t)p[off + 2] << 16) |
           ((uint32_t)p[off + 3] << 24);
}

bool kj_wideart_init(void)
{
    if (s_ready) {
        return true;
    }
    if (wideart_bin_size < BLOB_HEADER || memcmp(wideart_bin, "SJWA", 4) != 0) {
        kj_log("[wide art] no pictures (the build made none)");
        return false;
    }
    const uint16_t version = rd16(wideart_bin, 4);
    if (version != 1) {
        kj_log("[wide art] blob version %u is not this build's", version);
        return false;
    }
    s_count = rd16(wideart_bin, 6);
    if (s_count == 0) {
        kj_log("[wide art] no pictures (wideart/ is empty): edge fills only");
        return false;
    }
    s_tex_w = rd16(wideart_bin, 8);
    s_tex_h = rd16(wideart_bin, 10);
    const uint32_t data_h = rd16(wideart_bin, 12);
    /* The PICA's texture size is a power of two in each direction
     * (citro3d's checkTexSize), and the texture holds the bands: the rows
     * the blob carries, the rest cleared. */
    if (s_tex_w == 0 || s_tex_h == 0 || (s_tex_w & (s_tex_w - 1)) != 0 ||
        (s_tex_h & (s_tex_h - 1)) != 0 || data_h == 0 || data_h > s_tex_h) {
        kj_log("[wide art] bad texture size %lux%lu (%lu rows of pictures)",
               (unsigned long)s_tex_w, (unsigned long)s_tex_h, (unsigned long)data_h);
        return false;
    }
    const size_t need =
        BLOB_HEADER + (size_t)s_count * BLOB_RECORD + (size_t)s_tex_w * data_h * 2u;
    if (need > wideart_bin_size) {
        kj_log("[wide art] blob is short (%u bytes, needs %u)", wideart_bin_size, (unsigned)need);
        return false;
    }
    if (!C3D_TexInit(&s_tex, (u16)s_tex_w, (u16)s_tex_h, GPU_RGBA5551)) {
        kj_log("[wide art] cannot allocate the %lux%lu texture", (unsigned long)s_tex_w,
               (unsigned long)s_tex_h);
        return false;
    }
    C3D_TexSetFilter(&s_tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&s_tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    /* The blob's pixels are already in the PICA's tile order. */
    memset(s_tex.data, 0, (size_t)s_tex_w * s_tex_h * 2u);
    memcpy(s_tex.data, wideart_bin + BLOB_HEADER + (size_t)s_count * BLOB_RECORD,
           (size_t)s_tex_w * data_h * 2u);
    C3D_TexFlush(&s_tex);

    s_pics = malloc(sizeof(kj_wideart_pic) * (s_count != 0 ? s_count : 1));
    if (s_pics == NULL) {
        kj_log("[wide art] out of memory for the picture table");
        C3D_TexDelete(&s_tex);
        return false;
    }
    for (uint32_t i = 0; i < s_count; ++i) {
        const unsigned char *r = wideart_bin + BLOB_HEADER + (size_t)i * BLOB_RECORD;
        kj_wideart_pic *p = &s_pics[i];
        p->key = (uint64_t)rd32(r, 0) | ((uint64_t)rd32(r, 4) << 32);
        p->hofs = (int16_t)rd16(r, 8);
        p->vofs = (int16_t)rd16(r, 10);
        p->flags = rd16(r, 12);
        p->left_x = rd16(r, 14);
        p->left_y = rd16(r, 16);
        p->right_x = rd16(r, 18);
        p->right_y = rd16(r, 20);
        p->band_w = rd16(r, 22);
        p->band_h = rd16(r, 24);
    }
    s_ready = true;
    kj_log("[wide art] %lu pictures, a %lux%lu texture", (unsigned long)s_count, (unsigned long)s_tex_w,
           (unsigned long)s_tex_h);
    return true;
}

void kj_wideart_exit(void)
{
    if (s_pics != NULL) {
        free(s_pics);
        s_pics = NULL;
    }
    s_count = 0;
    if (s_ready) {
        C3D_TexDelete(&s_tex);
        s_ready = false;
    }
}

const kj_wideart_pic *kj_wideart_find(uint64_t key)
{
    if (key == 0) {
        return NULL;
    }
    for (uint32_t i = 0; i < s_count; ++i) {
        if (s_pics[i].key == key) {
            return &s_pics[i];
        }
    }
    return NULL;
}

/* How far apart two scrolls are, the shorter way round the map. */
static int wrapped(int a, int b, int period)
{
    int d = ((a - b) % period + period) % period;

    return d >= period / 2 ? d - period : d;
}

const kj_wideart_pic *kj_wideart_find_near(uint64_t key, int cur_h, int cur_v, int period_h,
                                            int period_v, int max_h, int max_v, int *dx_out)
{
    const kj_wideart_pic *best = NULL;
    int best_dist = 0;

    if (key == 0 || s_count == 0 || period_h <= 0 || period_v <= 0) {
        return NULL;
    }
    for (uint32_t i = 0; i < s_count; ++i) {
        if (s_pics[i].key != key) {
            continue;
        }
        const int dx = wrapped(cur_h, s_pics[i].hofs, period_h);
        const int dy = wrapped(cur_v, s_pics[i].vofs, period_v);
        if (max_h > 0 && (dx >= max_h || dx <= -max_h)) {
            continue;
        }
        if (max_v > 0 && (dy >= max_v || dy <= -max_v)) {
            continue;
        }
        const int dist = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
        if (best == NULL || dist < best_dist) {
            best = &s_pics[i];
            best_dist = dist;
            if (dx_out != NULL) {
                *dx_out = dx;
            }
        }
    }
    return best;
}

C3D_Tex *kj_wideart_texture(void)
{
    return s_ready ? &s_tex : NULL;
}

uint32_t kj_wideart_texture_width(void)
{
    return s_tex_w;
}

uint32_t kj_wideart_texture_height(void)
{
    return s_tex_h;
}

uint32_t kj_wideart_count(void)
{
    return s_count;
}
