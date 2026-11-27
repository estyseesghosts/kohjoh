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

/* Select capture for the next picture before its geometry is produced.
 * A partially built scene keeps its choice until SWAP_BUFFERS. Discarded
 * scenes still execute DS state/tests and counts; they never replace the
 * last complete display scene. */
void kj_g3_set_capture(bool capture);

/* Vertex/index storage lives in GPU-readable memory; the 3DS side provides
 * it at startup (before the game runs). Two sets: build and display. */
typedef struct kj_g3_storage {
    kj_g3_vertex *verts[2];
    uint16_t *indices[2];
} kj_g3_storage;

void kj_g3_init(const kj_g3_storage *storage);
/* G3-owned allocations: the display-list arena. Borrowed vertex/index
 * pointers are cleared; the renderer keeps ownership of those buffers. */
void kj_g3_shutdown(void);

/* Counters for diagnostics. */
typedef struct kj_g3_stats {
    uint32_t swaps;
    uint32_t commands;
    uint32_t box_tests;
    uint32_t box_culled;
} kj_g3_stats;

void kj_g3_get_stats(kj_g3_stats *out);
/* Reports the display-list hit/miss/uncacheable counts and the arena's used
 * bytes. Despite reading like a getter, it logs the profile report and resets
 * every counter it reports. */
void kj_g3_dl_take_stats_and_log(uint32_t *hits, uint32_t *misses, uint32_t *uncacheable,
                                 uint32_t *bytes);
void kj_g3_take_profile(uint32_t *fifo_calls, uint32_t *fifo_words, uint32_t *direct_words,
                        uint32_t *commands);

#endif
