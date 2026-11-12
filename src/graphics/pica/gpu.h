#ifndef KJ_GRAPHICS_PICA_GPU_H
#define KJ_GRAPHICS_PICA_GPU_H

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

/* Shared PICA state helpers with a bound-state cache: citro3d marks a
 * texture dirty on every bind, even of the same texture (GPU_2D.md 8). */

typedef enum kj_program_id { KJ_PROGRAM_NONE, KJ_PROGRAM_2D, KJ_PROGRAM_3D } kj_program_id;

bool kj_gpu_init(void);
void kj_gpu_exit(void);
/* Forget cached bindings (start of a frame / after a target change). */
void kj_gpu_invalidate(void);

void kj_gpu_use_program(kj_program_id id);
void kj_gpu_bind_texture(int unit, C3D_Tex *tex);

/* 2D vertex: DS pixel position, atlas texel coordinates, colour. */
typedef struct kj_v2d {
    int16_t x, y;
    int16_t u, v;
    uint8_t r, g, b, a;
} kj_v2d;

/* Per-frame 2D vertex arena (GPU-visible, reset each frame). */
kj_v2d *kj_gpu_quads_alloc(uint32_t quads, uint32_t *first_quad);
void kj_gpu_quads_reset(void);
/* Shrink the arena back to `used` quads (give back an over-reservation). */
void kj_gpu_quads_reset_to(uint32_t used);
uint32_t kj_gpu_quads_dropped(void);
/* Draw `count` quads starting at `first` from the arena. */
void kj_gpu_draw_quads(uint32_t first, uint32_t count);
/* Draw `count` quads from a retained buffer (linear memory) instead. */
void kj_gpu_draw_quads_from(const kj_v2d *base, uint32_t count);

/* Uniform locations. */
int kj_gpu_uloc_2d_xform(void);
int kj_gpu_uloc_2d_uvscale(void);
int kj_gpu_uloc_3d_palette(void);
int kj_gpu_uloc_3d_texscale(void);

/* TEV presets. */
void kj_gpu_tev_texture_modulate(void); /* tex0 * vertex colour */
void kj_gpu_tev_vertex_color(void);     /* vertex colour only */
void kj_gpu_tev_constant(uint32_t rgba);/* constant colour */

/* Scissor in physical screen coordinates of a target (x right, y down). */
void kj_gpu_scissor(int screen_w, int x0, int y0, int x1, int y1);
void kj_gpu_scissor_off(void);

#endif
