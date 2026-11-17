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
 * flags (blend target bits) the 3D pixels carry. `window`: the 2D window
 * regions (stencil bits 4-5) BG0 shows in: KJ_G3D_WINDOW_ALL, region r
 * only (r), or every region except r (KJ_G3D_WINDOW_EXCEPT + r). */
#define KJ_G3D_WINDOW_ALL (-1)
#define KJ_G3D_WINDOW_EXCEPT 4
void kj_g3d_draw(const kj_target *target, bool wide, uint8_t stencil_flags, int window);

typedef struct kj_g3d_stats {
    uint32_t batches, draws, triangles;
    uint32_t state_skips;
    uint32_t join_groups, joined_groups, joined_batches, join_declined;
    uint32_t tex_uploads, tex_bytes_compared, tex_cache_full;
    uint32_t unsupported;
    uint32_t box_tests, box_culled, box_culled_far;
} kj_g3d_stats;

void kj_g3d_take_stats(kj_g3d_stats *out);
/* The current scene uses W-buffering: its screen clears depth to 1. */
bool kj_g3d_wbuffer(void);

#endif
