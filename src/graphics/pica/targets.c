/*
 * targets.c: the two screen render targets.
 *
 * Budget (O3DS VRAM, 6 MiB): top 240x400 RGBA8 colour (375 KiB) + D24S8
 * (375 KiB); bottom 240x320 RGBA8 (300 KiB) + D24S8 (300 KiB). No stereo
 * right-eye target (baseline excludes stereo). Stencil is required by the
 * 2D window/blend passes; depth by the 3D layer.
 */

#include "graphics/pica/targets.h"

#include "diagnostics/log.h"
#include "graphics/pica/offscreen.h"

static kj_field_scale s_world_mode = KJ_FIELD_SCALE_LEGACY;

#define DISPLAY_TRANSFER_FLAGS                                                                     \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |               \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static kj_target s_targets[2];

bool kj_targets_init(void) {
    s_targets[0].rt = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    s_targets[1].rt = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (s_targets[0].rt == NULL || s_targets[1].rt == NULL) {
        kj_log("[gpu] cannot create render targets (VRAM free %lu)",
               (unsigned long)vramSpaceFree());
        /* Delete whichever target succeeded; a half-created pair has no
         * useful state and must not leak VRAM into the next attempt. */
        for (int i = 0; i < 2; ++i) {
            if (s_targets[i].rt != NULL) {
                C3D_RenderTargetDelete(s_targets[i].rt);
                s_targets[i].rt = NULL;
            }
        }
        return false;
    }
    C3D_RenderTargetSetOutput(s_targets[0].rt, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    C3D_RenderTargetSetOutput(s_targets[1].rt, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    s_targets[0].width = 400;
    s_targets[0].height = 240;
    s_targets[0].scale = 1.25f;
    s_targets[0].offset_x = 40.0f; /* interface mapping, DS x 0..256 -> 40..360 */
    s_targets[1].width = 320;
    s_targets[1].height = 240;
    s_targets[1].scale = 1.25f;
    s_targets[1].offset_x = 0.0f;

    /* Interface projection is unchanged in all three modes. The world
     * projection is selected once here; 2x needs an offscreen surface that
     * is not yet available, so it falls back safely and reports why. */
    s_world_mode = kj_config_field_scale();
    if (s_world_mode == KJ_FIELD_SCALE_2X) {
        if (kj_offscreen_init()) {
            s_world_mode = KJ_FIELD_SCALE_2X;
        } else {
            kj_log("[gpu] 2x world allocation failed; falling back to legacy");
            s_world_mode = KJ_FIELD_SCALE_LEGACY;
        }
    }
    /* The world fixup must use the ACTIVE world surface, not the physical
     * display: in 2x that is 200x120, otherwise the whole DS screen is
     * squeezed into the offscreen texture instead of showing a 1:1 crop. */
    kj_world_view_init_display(&s_targets[0].world, s_world_mode);
    kj_world_view_init(&s_targets[1].world, KJ_FIELD_SCALE_LEGACY, 320, 240);
    kj_log("[gpu] world projection %s (top), interface projection legacy",
           kj_config_scale_name(s_world_mode));
    return true;
}

void kj_targets_exit(void) {
    kj_offscreen_exit();
    for (int i = 0; i < 2; ++i) {
        if (s_targets[i].rt != NULL) {
            C3D_RenderTargetDelete(s_targets[i].rt);
            s_targets[i].rt = NULL;
        }
    }
    s_world_mode = KJ_FIELD_SCALE_LEGACY;
}

kj_target *kj_target_get(int screen) {
    return &s_targets[screen];
}

kj_field_scale kj_targets_world_mode(void) {
    return s_world_mode;
}
