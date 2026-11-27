/*
 * kj_g3_arena.c: the retained display-list bump arena.
 *
 * One 6 MiB allocation replaces thousands of small per-list allocations. The
 * arena is reset as a unit when the cache is flushed and released at
 * shutdown; individual retained lists never own or free parts of it.
 */

#include "kj_g3_arena.h"

#include <stddef.h>
#include <stdlib.h>

#define KJ_G3_ARENA_BYTES (6u * 1024u * 1024u)

static uint8_t *s_arena;
static uint32_t s_used;

void *kj_g3_arena_alloc(uint32_t bytes) {
    if (s_arena == NULL) {
        s_arena = malloc(KJ_G3_ARENA_BYTES);
        if (s_arena == NULL) {
            return NULL;
        }
    }
    bytes = (bytes + 3u) & ~3u;
    if (bytes == 0 || s_used + bytes > KJ_G3_ARENA_BYTES) {
        return NULL;
    }
    void *ptr = s_arena + s_used;
    s_used += bytes;
    return ptr;
}

void kj_g3_arena_reset(void) {
    s_used = 0;
}

uint32_t kj_g3_arena_used(void) {
    return s_used;
}

void kj_g3_arena_release(void) {
    free(s_arena);
    s_arena = NULL;
    s_used = 0;
}
