#ifndef KJ_GRAPHICS_WORLD_VIEW_H
#define KJ_GRAPHICS_WORLD_VIEW_H

#include "app/config.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * world_view.c: the single authoritative description of how DS world
 * coordinates map into the active rasterization surface (pixel-perfect.txt,
 * Task 3).
 *
 * Four mappings are deliberately kept separate: the physical display target,
 * the active render surface, the world projection (this module) and the
 * interface projection (unchanged 2D layout code). `offset_y` and the surface
 * height are part of the view: the old `scale / 120.0f` and "every target is
 * 240 high" assumptions are not valid for the 200x120 world surface.
 *
 * surface_x = ds_x * scale + offset_x
 * surface_y = ds_y * scale + offset_y
 */
typedef struct kj_world_view {
    kj_field_scale mode;
    int surface_w; /* active rasterization width (400 or 200) */
    int surface_h; /* active rasterization height (240 or 120) */
    int world_w;   /* active DS-coordinate world width (400 or 200) */
    int world_h;   /* active DS-coordinate world height (240 or 120) */
    float scale;
    float offset_x; /* signed */
    float offset_y; /* signed */
    int backing_w;  /* padded GPU texture (256x128 in 2x, else the surface) */
    int backing_h;
    int backing_x; /* active region origin inside the padded texture */
    int backing_y;
} kj_world_view;

enum { KJ_DS_SCREEN_W = 256, KJ_DS_SCREEN_H = 192 };

/* Initialise the canonical mapping for one mode. surface_w/surface_h are the
 * active world target (400x240 for legacy/1x, 200x120 for 2x). */
void kj_world_view_init(kj_world_view *view, kj_field_scale mode, int surface_w, int surface_h);
void kj_world_view_init_display(kj_world_view *view, kj_field_scale mode);

void kj_world_view_map(const kj_world_view *view, float ds_x, float ds_y, float *surface_x,
                       float *surface_y);
void kj_world_view_unmap(const kj_world_view *view, float surface_x, float surface_y, float *ds_x,
                         float *ds_y);

/* DS coordinates that can contribute to the image, derived from the active
 * surface, not from a fixed radius around the player. */
void kj_world_view_visible_ds(const kj_world_view *view, float *x0, float *y0, float *x1,
                              float *y1);
/* Half-extents in original DS normalized clip space for an unchanged camera. */
void kj_world_view_clip_extents(const kj_world_view *view, float *hx, float *hy);

/* DS viewport (inclusive x1/y1) -> clipped surface rect [l,t,r,b) in pixels.
 * Returns false for an empty rect. Handles negative offsets by clipping. */
bool kj_world_view_scissor(const kj_world_view *view, int ds_x0, int ds_y0, int ds_x1, int ds_y1,
                           int out[4]);

/* Surface rect -> PICA framebuffer scissor arguments (top, left, bottom, right)
 * for a portrait render target. Pure so native tests cover the rotation. */
void kj_world_view_rotate_scissor(int surface_w, int surface_h, const int rect[4], int out[4]);
/* Landscape render-to-texture surface: no axis rotation. */
void kj_world_view_plain_scissor(int surface_w, int surface_h, const int rect[4], int out[4]);

uint32_t kj_world_view_key(const kj_world_view *view); /* projection cache key */
bool kj_world_view_equal(const kj_world_view *a, const kj_world_view *b);

#endif
