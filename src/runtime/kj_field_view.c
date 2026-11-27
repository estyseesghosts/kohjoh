/*
 * kj_field_view.c: authoritative per-frame field mode (Task 4).
 *
 * Policy is committed at the frame boundary from the resolved screen identity
 * (kj_screen_state), before geometry is produced, and stays immutable until
 * the next publication. Culling reads the same description as the renderer.
 */

#include "kj_bridge.h"
#include "kj_field_view.h"

/* Declared locally so this policy module stays host-testable without NitroSDK
 * headers; the real declaration in nitro_port.h is identical. */
extern void NitroPort_Log(const char *fmt, ...);

#define KJ_DS_W 256.0f
#define KJ_DS_H 192.0f

static kj_field_view_desc s_view;
static uint32_t s_view_generation;
static bool s_vis_cull = true;
static bool s_box_cull = true;

static void derive(kj_field_view_desc *v)
{
    float sw, sh, scale;

    switch (v->effective_mode) {
    case KJ_FIELD_VIEW_2X:
        sw = 200.0f;
        sh = 120.0f;
        scale = 1.0f;
        break;
    case KJ_FIELD_VIEW_1X:
        sw = 400.0f;
        sh = 240.0f;
        scale = 1.0f;
        break;
    default:
        v->effective_mode = KJ_FIELD_VIEW_LEGACY;
        sw = 400.0f;
        sh = 240.0f;
        scale = 1.25f;
        break;
    }
    v->surface_w = sw;
    v->surface_h = sh;
    v->scale = scale;
    v->offset_x = (sw - scale * KJ_DS_W) / 2.0f;
    v->offset_y = (sh - scale * KJ_DS_H) / 2.0f;
    v->vis_x0 = -v->offset_x / scale;
    v->vis_x1 = (sw - v->offset_x) / scale;
    v->vis_y0 = -v->offset_y / scale;
    v->vis_y1 = (sh - v->offset_y) / scale;
    v->clip_hx = sw / (scale * KJ_DS_W);
    v->clip_hy = sh / (scale * KJ_DS_H);
}

void kj_field_view_configure(uint32_t requested, uint32_t configured)
{
    s_view.requested_mode = requested;
    s_view.configured_mode = configured;
    ++s_view_generation;
    s_view.view_generation = s_view_generation;
    derive(&s_view);
}

void kj_field_view_set_scene(uint32_t screen_id, uint32_t generation)
{
    const uint32_t new_mode = screen_id == KJ_SCREEN_FIELD ? s_view.configured_mode
                                                           : KJ_FIELD_VIEW_LEGACY;
    const bool mode_changed = new_mode != s_view.effective_mode;
    if (mode_changed) {
        ++s_view_generation; /* invalidate projection-keyed caches */
    }
    s_view.effective_mode = new_mode;
    s_view.upper_field = screen_id == KJ_SCREEN_FIELD;
    s_view.scene_id = screen_id;
    s_view.scene_generation = generation;
    s_view.view_generation = s_view_generation;
    derive(&s_view);
    if (mode_changed) {
        NitroPort_Log("[fieldview] scene %lu -> mode %lu, view generation %lu, "
                      "visible x %.2f..%.2f y %.2f..%.2f, clip %.5f/%.5f",
                      (unsigned long)screen_id, (unsigned long)new_mode,
                      (unsigned long)s_view.view_generation, s_view.vis_x0, s_view.vis_x1,
                      s_view.vis_y0, s_view.vis_y1, s_view.clip_hx, s_view.clip_hy);
    }
}

const kj_field_view_desc *kj_field_view_current(void)
{
    return &s_view;
}

void kj_field_view_set_vis_cull_enabled(bool enabled)
{
    s_vis_cull = enabled;
}

bool kj_field_view_vis_cull_enabled(void)
{
    return s_vis_cull;
}

void kj_field_view_set_box_cull_enabled(bool enabled)
{
    s_box_cull = enabled;
}

bool kj_field_view_box_cull_enabled(void)
{
    return s_box_cull;
}

void kj_field_view_reset(void)
{
    s_view.requested_mode = KJ_FIELD_VIEW_LEGACY;
    s_view.configured_mode = KJ_FIELD_VIEW_LEGACY;
    s_view.effective_mode = KJ_FIELD_VIEW_LEGACY;
    s_view.upper_field = false;
    s_view.scene_id = KJ_SCREEN_UNKNOWN;
    s_view.scene_generation = 0;
    s_view.view_generation = ++s_view_generation;
    s_box_cull = true;
    derive(&s_view);
}
