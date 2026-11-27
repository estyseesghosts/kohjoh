/* Minimal legacy field-view description for the differential G3 harnesses.
 * Production code uses src/runtime/kj_field_view.c; these stubs only keep the
 * harnesses independent of the SD script parser while preserving the exact
 * legacy visibility constants. */
#include "runtime/kj_field_view.h"

static kj_field_view_desc s_stub_view = {
    KJ_FIELD_VIEW_LEGACY, /* requested */
    KJ_FIELD_VIEW_LEGACY, /* configured */
    KJ_FIELD_VIEW_LEGACY, /* effective */
    0u,                   /* scene_id */
    0u,                   /* scene generation */
    1u,                   /* view generation */
    false,                /* upper_field */
    400.0f,
    240.0f, /* surface */
    1.25f,
    40.0f,
    0.0f, /* scale/offset */
    -32.0f,
    0.0f,
    288.0f,
    192.0f, /* visible DS bounds */
    1.25f,
    1.0f, /* clip half-extents */
};

const kj_field_view_desc *kj_field_view_current(void) {
    return &s_stub_view;
}

void kj_field_view_set_vis_cull_enabled(bool enabled) {
    s_stub_view.scene_generation = enabled ? 0u : 1u;
}

bool kj_field_view_vis_cull_enabled(void) {
    return true;
}

void kj_field_view_set_box_cull_enabled(bool enabled) {
    (void)enabled;
}

bool kj_field_view_box_cull_enabled(void) {
    return true;
}
