#ifndef KJ_GRAPHICS_PICA_TARGETS_H
#define KJ_GRAPHICS_PICA_TARGETS_H

#include <citro3d.h>
#include <stdbool.h>

enum { KJ_SCREEN_TOP = 0, KJ_SCREEN_BOTTOM = 1 };

typedef struct kj_target {
    C3D_RenderTarget *rt;
    int width;  /* physical pixels: 400 top, 320 bottom */
    int height; /* 240 */
    /* DS pixel -> physical: x_phys = x_ds * scale + offset_x, y_phys = y_ds * scale */
    float scale;
    float offset_x;
} kj_target;

bool kj_targets_init(void);
void kj_targets_exit(void);
kj_target *kj_target_get(int screen);

#endif
