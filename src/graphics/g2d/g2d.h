#ifndef KJ_GRAPHICS_G2D_G2D_H
#define KJ_GRAPHICS_G2D_G2D_H

#include "runtime/kj_bridge.h"
#include "graphics/pica/gpu.h"
#include "graphics/pica/targets.h"

#include <stdbool.h>
#include <stdint.h>

bool kj_g2d_init(void);
void kj_g2d_exit(void);

/* Draw one DS engine's display onto a screen target: backdrop, BG and OBJ
 * layers in DS priority order with the 3D layer at BG0's slot (engine A),
 * colour effects and master brightness. */
void kj_screen_draw(int screen, int engine, const kj_frame_input *in);

typedef struct kj_g2d_stats {
    uint32_t bg_quads, obj_quads, draws, spans;
    uint32_t unsupported_modes; /* modes drawn as nothing; must be zero in the corpus */
    uint32_t wide_windows;      /* window side rectangles over the wide columns */
    uint32_t layout_segments;   /* wide-layout segments drawn (issue #16) */
    uint32_t dropped_quads;
    uint32_t skipped_screens, reused_layers, rebuilt_layers;
    uint32_t char_bytes_compared; /* retained layers' character-range checks */
} kj_g2d_stats;

/* Start of a frame (after kj_atlas_begin_frame): pins retained layers. */
void kj_g2d_begin_frame(void);

/* Draw retained native-text quads (fill first, then shadow/foreground
 * passes) for one BG plane. Solid fills use the vertex-colour material;
 * glyphs sample the font atlas. Restores the DS atlas mapping afterwards. */
void kj_g2d_text_draw(const kj_target *t, const kj_v2d *v, uint32_t vertex_count, bool textured);

void kj_g2d_take_stats(kj_g2d_stats *out);

#endif
