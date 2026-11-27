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
    if (bytes == 0 || bytes > UINT32_MAX - 3u) {
        return NULL;
    }
    const uint32_t aligned = (bytes + 3u) & ~3u;
    /* s_used <= KJ_G3_ARENA_BYTES is the arena's invariant. */
    if (aligned > KJ_G3_ARENA_BYTES - s_used) {
        return NULL;
    }
    void *ptr = s_arena + s_used;
    s_used += aligned;
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
