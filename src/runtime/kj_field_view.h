#ifndef KJ_RUNTIME_KJ_FIELD_VIEW_H
#define KJ_RUNTIME_KJ_FIELD_VIEW_H

#include <stdbool.h>
#include <stdint.h>

/*
 * kj_field_view.c: the immutable field-view description shared by geometry
 * production, culling and rendering (pixel-perfect.txt Task 4).
 *
 * The mode is a plain integer so this header can be included by both the
 * game-compiled (src/runtime) and libctru (src/graphics) translation units
 * without depending on enum ABI. Values mirror kj_field_scale in app/config.h:
 * 0 = legacy, 1 = 1x, 2 = 2x.
 *
 * The description is fixed at the frame boundary, before the next frame's
 * geometry is produced, and travels with that frame. It is NOT derived from
 * POWCNT1 engine-A-on-top or from texture dimensions.
 */
enum {
    KJ_FIELD_VIEW_LEGACY = 0,
    KJ_FIELD_VIEW_1X = 1,
    KJ_FIELD_VIEW_2X = 2,
};

typedef struct kj_field_view_desc {
    uint32_t requested_mode;  /* from config.txt */
    uint32_t configured_mode; /* after offscreen allocation checks */
    uint32_t effective_mode;  /* legacy unless this is the upper field */
    uint32_t scene_id;        /* KJ_SCREEN_* of the published scene */
    uint32_t scene_generation;
    uint32_t view_generation; /* changes only when the projection changes */
    bool upper_field;
    float surface_w, surface_h; /* active world rasterization dimensions */
    float scale, offset_x, offset_y;
    float vis_x0, vis_y0, vis_x1, vis_y1; /* DS coords that can contribute */
    float clip_hx, clip_hy;               /* DS clip-space half-extents */
} kj_field_view_desc;

/* Called once from the 3DS-side renderer initialisation. */
void kj_field_view_configure(uint32_t requested, uint32_t configured);
/* Called at publication, before the next frame's FieldMap_Render3D work. */
void kj_field_view_set_scene(uint32_t screen_id, uint32_t generation);
const kj_field_view_desc *kj_field_view_current(void);
/* Diagnostic: disable offscreen-object culling without touching face culling.
 * Driven by script.txt `debug: 0x800` (KJ_DEBUG_NO_VIS_CULL). */
void kj_field_view_set_vis_cull_enabled(bool enabled);
bool kj_field_view_vis_cull_enabled(void);
/* Diagnostic: bypass render-visibility BOX_TEST rejection (script.txt
 * `debug: 0x1000`). Development-only; never enabled by config.txt. */
void kj_field_view_set_box_cull_enabled(bool enabled);
bool kj_field_view_box_cull_enabled(void);
/* Safe fallback used by non-field scenes and by soft reset. */
void kj_field_view_reset(void);

#endif
