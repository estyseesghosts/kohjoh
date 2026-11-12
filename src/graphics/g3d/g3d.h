#ifndef KJ_GRAPHICS_G3D_G3D_H
#define KJ_GRAPHICS_G3D_G3D_H

#include "graphics/pica/targets.h"
#include "runtime/kj_bridge.h"

#include <stdbool.h>
#include <stdint.h>

bool kj_g3d_init(void);
void kj_g3d_exit(void);
/* The scene to draw this frame (the most recent swap; NULL: none). */
void kj_g3d_set_frame(const kj_g3_frame *frame);
/* Draw the 3D layer into the current target at BG0's place. `wide`: show the
 * extended horizontal field (top screen). `stencil_flags`: the 2D effect
 * flags (blend target bits) the 3D pixels carry. */
void kj_g3d_draw(const kj_target *target, bool wide, uint8_t stencil_flags);

typedef struct kj_g3d_stats {
    uint32_t batches, draws, triangles;
    uint32_t tex_uploads, tex_bytes_compared, tex_cache_full;
    uint32_t unsupported;
} kj_g3d_stats;

void kj_g3d_take_stats(kj_g3d_stats *out);
/* The current scene uses W-buffering: its screen clears depth to 1. */
bool kj_g3d_wbuffer(void);

#endif
