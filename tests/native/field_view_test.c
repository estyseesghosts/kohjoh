/* Field-mode activation, scene transitions and cache generation (Task 4/14).
 * Build: cc -I src -I src/runtime tests/native/field_view_test.c \
 *        src/runtime/kj_field_view.c -o field_view_test */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "kj_bridge.h"
#include "kj_field_view.h"

void NitroPort_Log(const char *fmt, ...)
{
    (void)fmt;
}

static void close(float a, float b)
{
    assert(fabsf(a - b) < 0.0001f);
}

int main(void)
{
    const kj_field_view_desc *v;

    kj_field_view_reset();
    v = kj_field_view_current();
    assert(v->effective_mode == KJ_FIELD_VIEW_LEGACY);
    assert(!v->upper_field);
    close(v->clip_hx, 1.25f);
    close(v->clip_hy, 1.0f);
    const uint32_t gen0 = v->view_generation;

    kj_field_view_configure(KJ_FIELD_VIEW_1X, KJ_FIELD_VIEW_1X);
    kj_field_view_set_scene(KJ_SCREEN_FIELD, 10);
    v = kj_field_view_current();
    assert(v->upper_field && v->effective_mode == KJ_FIELD_VIEW_1X);
    close(v->offset_x, 72.0f);
    close(v->offset_y, 24.0f);
    close(v->vis_x0, -72.0f);
    close(v->vis_y1, 216.0f);
    close(v->clip_hx, 1.5625f);
    close(v->clip_hy, 1.25f);
    assert(v->view_generation > gen0);
    const uint32_t gen_field = v->view_generation;

    /* A menu launched while the field overlay remains active must not
     * inherit the field's projection, and must get a new generation. */
    kj_field_view_set_scene(KJ_SCREEN_BAG, 11);
    v = kj_field_view_current();
    assert(!v->upper_field && v->effective_mode == KJ_FIELD_VIEW_LEGACY);
    close(v->clip_hx, 1.25f);
    assert(v->view_generation > gen_field);

    /* Returning to the field restores 1x with a distinct generation. */
    kj_field_view_set_scene(KJ_SCREEN_FIELD, 12);
    v = kj_field_view_current();
    assert(v->effective_mode == KJ_FIELD_VIEW_1X);
    assert(v->view_generation > gen_field);

    /* Battles never inherit field scaling, even mid-transition. */
    kj_field_view_set_scene(KJ_SCREEN_BATTLE, 13);
    v = kj_field_view_current();
    assert(v->effective_mode == KJ_FIELD_VIEW_LEGACY);

    /* 2x uses the 200x120 view and the tighter derived clip bounds. */
    kj_field_view_configure(KJ_FIELD_VIEW_2X, KJ_FIELD_VIEW_2X);
    kj_field_view_set_scene(KJ_SCREEN_FIELD, 14);
    v = kj_field_view_current();
    assert(v->effective_mode == KJ_FIELD_VIEW_2X);
    close(v->surface_w, 200.0f);
    close(v->surface_h, 120.0f);
    close(v->vis_x0, 28.0f);
    close(v->vis_y0, 36.0f);
    close(v->clip_hx, 0.78125f);
    close(v->clip_hy, 0.625f);

    /* The diagnostic switch affects only visibility culling. */
    assert(kj_field_view_vis_cull_enabled());
    kj_field_view_set_vis_cull_enabled(false);
    assert(!kj_field_view_vis_cull_enabled());
    kj_field_view_set_vis_cull_enabled(true);

    kj_field_view_reset();
    v = kj_field_view_current();
    assert(v->effective_mode == KJ_FIELD_VIEW_LEGACY && !v->upper_field);

    puts("field_view_test: PASS");
    return 0;
}
