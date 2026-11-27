#ifndef KJ_GRAPHICS_PICA_OFFSCREEN_H
#define KJ_GRAPHICS_PICA_OFFSCREEN_H

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * offscreen.c: the persistent 2x world rasterization surface (Task 6).
 *
 * A 200x120 active viewport lives in the corner of a 256x128 PICA texture.
 * The texture is allocated once, only when 2x is configured, and released at
 * shutdown. Nothing reads pixels back to the CPU and no per-frame allocation
 * happens. Compositing samples only the active 200x120 region.
 */
/* PICA render targets are portrait: the 200x120 landscape world uses a
 * 120x200 active region in a 128x256 backing texture. The rotated projection
 * maps DS y -> texture x and DS x -> texture y; the composite must undo it. */
enum { KJ_OFFSCREEN_W = 128, KJ_OFFSCREEN_H = 256,
       KJ_OFFSCREEN_ACTIVE_W = 120, KJ_OFFSCREEN_ACTIVE_H = 200,
       KJ_OFFSCREEN_LOGICAL_W = 200, KJ_OFFSCREEN_LOGICAL_H = 120 };

bool kj_offscreen_init(void);
void kj_offscreen_exit(void);
bool kj_offscreen_ready(void);
C3D_RenderTarget *kj_offscreen_target(void);
C3D_Tex *kj_offscreen_texture(void);
/* Clear and bind the offscreen surface for world rasterization. */
void kj_offscreen_begin_world(uint32_t clear_color);
/* Rebind the native target after world rasterization. */
void kj_offscreen_end_world(C3D_RenderTarget *native);

#endif
