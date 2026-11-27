/*
 * world_view.c: canonical DS -> surface mappings from pixel-perfect.txt Task 3.
 *
 *   Legacy: world 400x240, scale 1.25, offset ( 40,   0)
 *   1x:     world 400x240, scale 1.00, offset ( 72,  24)
 *   2x:     world 200x120, scale 1.00, offset (-28, -36)
 *
 * All three satisfy  scale*256 + 2*offset_x = surface_w  and
 * scale*192 + 2*offset_y = surface_h: the DS canvas is centred in the surface.
 */

#include "graphics/world_view.h"

#include <string.h>

void kj_world_view_init(kj_world_view *view, kj_field_scale mode, int surface_w, int surface_h)
{
    memset(view, 0, sizeof(*view));
    view->mode = mode;
    view->surface_w = surface_w;
    view->surface_h = surface_h;
    /* Offsets are derived from the active surface, not hard-coded: the DS
     * canvas is centred in every mode. This also gives the bottom screen's
     * 320x240 legacy mapping (scale 1.25, offset_x 0) for free. */
    switch (mode) {
    case KJ_FIELD_SCALE_2X:
        view->scale = 1.0f;
        view->world_w = 200;
        view->world_h = 120;
        /* PICA textures need power-of-two dimensions; 200x120 is rendered
         * into the active region of a 256x128 backing allocation. */
        view->backing_w = 256;
        view->backing_h = 128;
        break;
    case KJ_FIELD_SCALE_1X:
        view->scale = 1.0f;
        view->world_w = 400;
        view->world_h = 240;
        view->backing_w = surface_w;
        view->backing_h = surface_h;
        break;
    case KJ_FIELD_SCALE_LEGACY:
    default:
        view->scale = 1.25f;
        view->world_w = 400;
        view->world_h = 240;
        view->backing_w = surface_w;
        view->backing_h = surface_h;
        break;
    }
    view->offset_x = ((float)surface_w - view->scale * KJ_DS_SCREEN_W) / 2.0f;
    view->offset_y = ((float)surface_h - view->scale * KJ_DS_SCREEN_H) / 2.0f;
}

void kj_world_view_init_display(kj_world_view *view, kj_field_scale mode)
{
    kj_world_view_init(view, mode, mode == KJ_FIELD_SCALE_2X ? 200 : 400,
                       mode == KJ_FIELD_SCALE_2X ? 120 : 240);
}

void kj_world_view_map(const kj_world_view *view, float ds_x, float ds_y,
                       float *surface_x, float *surface_y)
{
    if (surface_x != NULL) {
        *surface_x = ds_x * view->scale + view->offset_x;
    }
    if (surface_y != NULL) {
        *surface_y = ds_y * view->scale + view->offset_y;
    }
}

void kj_world_view_unmap(const kj_world_view *view, float surface_x, float surface_y,
                         float *ds_x, float *ds_y)
{
    if (ds_x != NULL) {
        *ds_x = (surface_x - view->offset_x) / view->scale;
    }
    if (ds_y != NULL) {
        *ds_y = (surface_y - view->offset_y) / view->scale;
    }
}

void kj_world_view_visible_ds(const kj_world_view *view, float *x0, float *y0, float *x1,
                              float *y1)
{
    if (x0 != NULL) {
        *x0 = -view->offset_x / view->scale;
    }
    if (x1 != NULL) {
        *x1 = (view->surface_w - view->offset_x) / view->scale;
    }
    if (y0 != NULL) {
        *y0 = -view->offset_y / view->scale;
    }
    if (y1 != NULL) {
        *y1 = (view->surface_h - view->offset_y) / view->scale;
    }
}

void kj_world_view_clip_extents(const kj_world_view *view, float *hx, float *hy)
{
    if (hx != NULL) {
        *hx = (float)view->surface_w / (view->scale * KJ_DS_SCREEN_W);
    }
    if (hy != NULL) {
        *hy = (float)view->surface_h / (view->scale * KJ_DS_SCREEN_H);
    }
}

bool kj_world_view_scissor(const kj_world_view *view, int ds_x0, int ds_y0, int ds_x1,
                           int ds_y1, int out[4])
{
    float sx0, sy0, sx1, sy1;
    int l, t, r, b;

    /* Inclusive DS endpoints: the rect covers [x0, x1+1) in DS pixels. */
    kj_world_view_map(view, (float)ds_x0, (float)ds_y0, &sx0, &sy0);
    kj_world_view_map(view, (float)ds_x1 + 1.0f, (float)ds_y1 + 1.0f, &sx1, &sy1);
    l = (int)(sx0 > 0.0f ? sx0 : 0.0f);
    t = (int)(sy0 > 0.0f ? sy0 : 0.0f);
    r = (int)(sx1 < view->surface_w ? sx1 : (float)view->surface_w);
    b = (int)(sy1 < view->surface_h ? sy1 : (float)view->surface_h);
    if (l < 0) {
        l = 0;
    }
    if (t < 0) {
        t = 0;
    }
    if (r > view->surface_w) {
        r = view->surface_w;
    }
    if (b > view->surface_h) {
        b = view->surface_h;
    }
    if (r <= l || b <= t) {
        return false;
    }
    out[0] = l;
    out[1] = t;
    out[2] = r;
    out[3] = b;
    return true;
}

void kj_world_view_plain_scissor(int surface_w, int surface_h, const int rect[4], int out[4])
{
    for (int i = 0; i < 4; ++i) {
        out[i] = rect[i] < 0 ? 0 : rect[i];
    }
    if (out[2] > surface_w) {
        out[2] = surface_w;
    }
    if (out[3] > surface_h) {
        out[3] = surface_h;
    }
    if (out[2] < out[0]) {
        out[2] = out[0];
    }
    if (out[3] < out[1]) {
        out[3] = out[1];
    }
}

void kj_world_view_rotate_scissor(int surface_w, int surface_h, const int rect[4], int out[4])
{
    /* Render targets are portrait (height x width): physical (x,y) maps to
     * framebuffer (surface_h - y, surface_w - x). Arguments are
     * (top, left, bottom, right). */
    int top = surface_h - rect[3];
    int left = surface_w - rect[2];
    int bottom = surface_h - rect[1];
    int right = surface_w - rect[0];
    if (top < 0) {
        top = 0;
    }
    if (left < 0) {
        left = 0;
    }
    if (bottom > surface_h) {
        bottom = surface_h;
    }
    if (right > surface_w) {
        right = surface_w;
    }
    out[0] = top;
    out[1] = left;
    out[2] = bottom;
    out[3] = right;
}

uint32_t kj_world_view_key(const kj_world_view *view)
{
    /* FNV-1a over the fields that change how a scene is projected. */
    const uint32_t words[] = {
        (uint32_t)view->mode, (uint32_t)view->surface_w, (uint32_t)view->surface_h,
        (uint32_t)view->world_w, (uint32_t)view->world_h,
        (uint32_t)(view->scale * 10000.0f), (uint32_t)(view->offset_x + 1000.0f),
        (uint32_t)(view->offset_y + 1000.0f), (uint32_t)view->backing_w,
        (uint32_t)view->backing_h,
    };
    uint32_t h = 2166136261u;
    const unsigned char *p = (const unsigned char *)words;
    for (size_t i = 0; i < sizeof(words); ++i) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

bool kj_world_view_equal(const kj_world_view *a, const kj_world_view *b)
{
    return a->mode == b->mode && a->surface_w == b->surface_w && a->surface_h == b->surface_h &&
           a->world_w == b->world_w && a->world_h == b->world_h && a->scale == b->scale &&
           a->offset_x == b->offset_x && a->offset_y == b->offset_y &&
           a->backing_w == b->backing_w && a->backing_h == b->backing_h;
}
