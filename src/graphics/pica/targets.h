#ifndef KJ_GRAPHICS_PICA_TARGETS_H
#define KJ_GRAPHICS_PICA_TARGETS_H

#include "app/config.h"
#include "graphics/world_view.h"

#include <citro3d.h>
#include <stdbool.h>

enum { KJ_SCREEN_TOP = 0, KJ_SCREEN_BOTTOM = 1 };

typedef struct kj_target {
    C3D_RenderTarget *rt;
    int width;  /* physical framebuffer pixels: 400 top, 320 bottom */
    int height; /* 240 */
    /* 2D interface projection. Kept at the legacy mapping in every mode:
     * dialogue, banners and menus must not shrink with the world. */
    float scale;
    float offset_x;
    /* World projection. Authoritative for 3D geometry drawn into this target;
     * task 3 separates it from the physical display and interface mappings. */
    kj_world_view world;
} kj_target;

bool kj_targets_init(void);
void kj_targets_exit(void);
kj_target *kj_target_get(int screen);
kj_field_scale kj_targets_world_mode(void); /* effective mode after fallback checks */

#endif
