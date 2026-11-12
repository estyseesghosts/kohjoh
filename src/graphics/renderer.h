#ifndef KJ_GRAPHICS_RENDERER_H
#define KJ_GRAPHICS_RENDERER_H

#include <stdbool.h>

/* GPU renderer (3DS side). The DS-facing entry is kj_ctr_publish_frame
 * (runtime/kj_bridge.h), implemented in renderer.c. */
bool kj_renderer_init(void);
void kj_renderer_exit(void);

#endif
