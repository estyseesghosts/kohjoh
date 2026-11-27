/*
 * kj_g3_arena.h: private ownership boundary for the display-list bump arena.
 * Only src/runtime/kj_g3.c and the arena implementation include this.
 */
#ifndef KJ_G3_ARENA_H
#define KJ_G3_ARENA_H

#include <stdint.h>

/* Returns NULL when the request is zero or does not fit. */
void *kj_g3_arena_alloc(uint32_t bytes);
/* Drop every retained block; the allocation stays for reuse. */
void kj_g3_arena_reset(void);
/* Bytes currently handed out (profile/diagnostics). */
uint32_t kj_g3_arena_used(void);
/* Free the arena. Safe after a partial or failed allocation. */
void kj_g3_arena_release(void);

#endif
