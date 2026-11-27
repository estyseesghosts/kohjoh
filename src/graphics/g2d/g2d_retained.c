/*
 * g2d_retained.c: retained-layer storage and exact snapshot comparison.
 *
 * A layer is reusable only when the bytes it consumed are identical: the
 * registers/palette/map snapshot plus the character-byte range and ext
 * palette. The orchestrator decides when to build and draw; this module owns
 * the caches, their persistent allocations and the comparisons.
 */

#include "graphics/g2d/g2d_internal.h"

#include "graphics/resources/atlas.h"

#include <stdlib.h>
#include <string.h>

layer_cache s_bg_cache[2][4];
layer_cache s_obj_cache[2];
screen_memo s_memo[2];
uint32_t s_skipped_screens, s_reused_layers, s_rebuilt_layers;
uint32_t s_char_bytes_compared;

bool kj_g2d_cache_alloc(layer_cache *lc) {
    if (lc->quads == NULL) {
        lc->quads = linearAlloc(CACHE_QUADS * 4u * sizeof(kj_v2d));
    }
    return lc->quads != NULL;
}

void kj_g2d_layer_cache_reset(layer_cache *lc) {
    /* chars is malloc'd; quads and band are PICA-visible linear memory.
     * Do not mix the allocator families. */
    free(lc->chars);
    if (lc->quads != NULL) {
        linearFree(lc->quads);
    }
    if (lc->band != NULL) {
        linearFree(lc->band);
    }
    memset(lc, 0, sizeof(*lc));
}

bool kj_g2d_chars_same(const layer_cache *lc, const uint8_t *vram) {
#ifndef HGSS_PROFILE_OFF
    s_char_bytes_compared += lc->char_len;
#endif
    return lc->char_len == 0 || memcmp(lc->chars, vram + lc->char_lo, lc->char_len) == 0;
}

void kj_g2d_chars_keep(layer_cache *lc, const uint8_t *vram, uint32_t lo, uint32_t hi) {
    if (hi <= lo) {
        lc->char_len = 0;
        return;
    }
    const uint32_t len = hi - lo;
    if (len > lc->char_cap) {
        free(lc->chars);
        lc->chars = malloc(len);
        lc->char_cap = lc->chars != NULL ? len : 0;
    }
    if (lc->chars == NULL) {
        lc->valid = false;
        lc->char_len = 0;
        return;
    }
    memcpy(lc->chars, vram + lo, len);
    lc->char_lo = lo;
    lc->char_len = len;
}
