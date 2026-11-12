#ifndef KJ_GRAPHICS_G3D_TEXTURES_H
#define KJ_GRAPHICS_G3D_TEXTURES_H

#include "runtime/kj_bridge.h"

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

/* DS texture (TEXIMAGE_PARAM + PLTT_BASE) -> PICA texture, cached.
 * Validity is an exact comparison of the source texel and palette bytes
 * with copies taken at conversion, done at most once per texture per frame.
 * Returns NULL for "no texture" (format 0) or when unavailable. */
void kj_tex_init(void);
void kj_tex_exit(void);
void kj_tex_begin_frame(const kj_frame_input *in);
C3D_Tex *kj_tex_get(uint32_t tex_param, uint32_t pltt_base);

typedef struct kj_tex_stats {
    uint32_t uploads, compared_bytes, full, missing_bank;
} kj_tex_stats;
void kj_tex_take_stats(kj_tex_stats *out);

#endif
