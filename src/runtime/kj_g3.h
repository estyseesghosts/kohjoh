/*
 * kj_g3.h: the DS geometry engine (DS side). See kj_g3.c.
 */

#ifndef KJ_G3_H
#define KJ_G3_H

#include "kj_bridge.h"


/* The most recently swapped scene (the DS keeps drawing it until the next
 * SWAP_BUFFERS), or NULL before the first swap. Valid until the swap after
 * next; the renderer only reads it inside kj_ctr_publish_frame. */
const kj_g3_frame *kj_g3_take_frame(void);

/* Widescreen: the renderer shows 25% more horizontal clip space than the
 * DS. The box test (the game's visibility culling) is widened to match so
 * objects in the extension are not culled. */
void kj_g3_set_wide(bool wide);

/* Vertex/index storage lives in GPU-readable memory; the 3DS side provides
 * it at startup (before the game runs). Two sets: build and display. */
typedef struct kj_g3_storage {
    kj_g3_vertex *verts[2];
    uint16_t *indices[2];
} kj_g3_storage;

void kj_g3_init(const kj_g3_storage *storage);

/* Counters for diagnostics. */
typedef struct kj_g3_stats {
    uint32_t swaps;
    uint32_t commands;
    uint32_t box_tests;
    uint32_t box_culled;
} kj_g3_stats;

void kj_g3_get_stats(kj_g3_stats *out);
void kj_g3_dl_stats(uint32_t *hits, uint32_t *misses, uint32_t *uncacheable, uint32_t *bytes);
void kj_g3_take_profile(uint64_t *fifo_ticks, uint32_t *fifo_words, uint32_t *direct_words,
                        uint32_t *commands);

#endif
