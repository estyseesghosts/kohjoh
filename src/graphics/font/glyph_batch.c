#include "glyph_batch.h"

#include <string.h>

void kj_text_run_init(kj_text_run *run, kj_text_vertex *memory, uint32_t vertex_capacity) {
    run->vertices = memory;
    run->count = 0;
    run->capacity = memory != NULL ? vertex_capacity : 0;
}

bool kj_text_run_push(kj_text_run *run, const kj_text_vertex quad[4]) {
    if (run->vertices == NULL || run->count + 4u > run->capacity) {
        return false;
    }
    memcpy(&run->vertices[run->count], quad, sizeof(kj_text_vertex) * 4u);
    run->count += 4u;
    return true;
}

static void set_quad(kj_text_vertex quad[4], int x, int y, int w, int h, int16_t u0, int16_t v0,
                     int16_t u1, int16_t v1, uint32_t rgba) {
    const uint8_t r = (uint8_t)(rgba >> 24), g = (uint8_t)(rgba >> 16);
    const uint8_t b = (uint8_t)(rgba >> 8), a = (uint8_t)rgba;
    quad[0] = (kj_text_vertex){(int16_t)x, (int16_t)y, u0, v0, r, g, b, a};
    quad[1] = (kj_text_vertex){(int16_t)(x + w), (int16_t)y, u1, v0, r, g, b, a};
    quad[2] = (kj_text_vertex){(int16_t)x, (int16_t)(y + h), u0, v1, r, g, b, a};
    quad[3] = (kj_text_vertex){(int16_t)(x + w), (int16_t)(y + h), u1, v1, r, g, b, a};
}

bool kj_batch_glyph(kj_text_run *run, const kj_font_atlas *atlas, const kj_glyph *g,
                    const kj_glyph_art *art, int x, int y, uint32_t rgba) {
    (void)atlas;
    (void)g;
    if (art == NULL || art->w == 0 || art->h == 0) {
        /* A space keeps its advance but has no quad. */
        return true;
    }
    kj_text_vertex quad[4];
    const int16_t u0 = (int16_t)(art->x * KJ_TEXT_UV_UNIT);
    const int16_t v0 = (int16_t)(art->y * KJ_TEXT_UV_UNIT);
    const int16_t u1 = (int16_t)(u0 + art->w * KJ_TEXT_UV_UNIT - 1);
    const int16_t v1 = (int16_t)(v0 + art->h * KJ_TEXT_UV_UNIT - 1);
    set_quad(quad, x, y, art->w, art->h, u0, v0, u1, v1, rgba);
    return kj_text_run_push(run, quad);
}

bool kj_batch_fill(kj_text_run *run, int x, int y, int w, int h, uint32_t rgba, int16_t u,
                   int16_t v) {
    kj_text_vertex quad[4];
    /* `u`,`v` are the top-left texel of the atlas's solid white tile (art 0,
     * at 1,1). Use the whole 8x8 tile, exactly like a glyph quad: PICA needs a
     * real texel span, not a constant coordinate. */
    const int16_t u0 = (int16_t)(u * KJ_TEXT_UV_UNIT);
    const int16_t v0 = (int16_t)(v * KJ_TEXT_UV_UNIT);
    const int16_t u1 = (int16_t)(u0 + 8 * KJ_TEXT_UV_UNIT - 1);
    const int16_t v1 = (int16_t)(v0 + 8 * KJ_TEXT_UV_UNIT - 1);
    set_quad(quad, x, y, w, h, u0, v0, u1, v1, rgba);
    return kj_text_run_push(run, quad);
}

uint32_t kj_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a;
}
