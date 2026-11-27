/*
 * glyph_batch.h: turn resolved glyph records into retained 2D quads.
 *
 * Pure geometry: no GPU calls, no libctru, no allocation. The caller owns the
 * destination buffer (the frame arena or a retained run) and the material.
 *
 * Coordinates are physical pixels with the origin at the top-left of the
 * destination screen. The glyph mask is placed 1:1, never scaled: the DS 5/4
 * mapping has already been applied to the cell origin by the caller.
 */

#ifndef KJ_GRAPHICS_FONT_GLYPH_BATCH_H
#define KJ_GRAPHICS_FONT_GLYPH_BATCH_H

#include <stdbool.h>
#include <stdint.h>

#include "glyph_atlas.h"

/* Same layout as kj_v2d in src/graphics/pica/gpu.h. Kept separate so the
 * geometry code is testable on the host without citro3d; text_render.c
 * asserts the sizes agree. */
#define KJ_TEXT_UV_UNIT 16
typedef struct kj_text_vertex {
    int16_t x, y;
    int16_t u, v;
    uint8_t r, g, b, a;
} kj_text_vertex;

typedef struct kj_text_run {
    kj_text_vertex *vertices;
    uint32_t count; /* vertices, not quads */
    uint32_t capacity;
} kj_text_run;

void kj_text_run_init(kj_text_run *run, kj_text_vertex *memory, uint32_t vertex_capacity);
bool kj_text_run_push(kj_text_run *run, const kj_text_vertex quad[4]);

/* Emit one glyph mask. `x`,`y` is the top-left of the mask in physical pixels.
 * `rgba` is 0xRRGGBBAA and is applied as vertex colour. */
bool kj_batch_glyph(kj_text_run *run, const kj_font_atlas *atlas, const kj_glyph *g,
                    const kj_glyph_art *art, int x, int y, uint32_t rgba);

/* Emit a solid quad (used for clean backing fills). Sampling is the caller's
 * business: it is drawn with the vertex-colour material. */
bool kj_batch_fill(kj_text_run *run, int x, int y, int w, int h, uint32_t rgba, int16_t u,
                   int16_t v);

uint32_t kj_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

#endif
