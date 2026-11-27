/* Native regression test for the retained display-list bump arena.
 *
 * The old sizing rounded bytes up before checking the capacity, so a request
 * of UINT32_MAX - 3 wrapped s_used back to zero and handed out a pointer past
 * the arena. Build through tests/native/run_host.py.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "kj_g3_arena.h"

#define KJ_TEST_ARENA_BYTES (6u * 1024u * 1024u) /* mirrors KJ_G3_ARENA_BYTES */

int main(void) {
    assert(kj_g3_arena_alloc(0) == NULL);
    assert(kj_g3_arena_used() == 0);

    void *first = kj_g3_arena_alloc(1);
    assert(first != NULL);
    assert(kj_g3_arena_used() == 4);

    const uint32_t used = kj_g3_arena_used();
    assert(kj_g3_arena_alloc(UINT32_MAX) == NULL);
    assert(kj_g3_arena_alloc(UINT32_MAX - 3u) == NULL);
    assert(kj_g3_arena_alloc(UINT32_MAX - 2u) == NULL);
    assert(kj_g3_arena_used() == used);

    assert(kj_g3_arena_alloc(KJ_TEST_ARENA_BYTES - used) != NULL);
    assert(kj_g3_arena_used() == KJ_TEST_ARENA_BYTES);
    assert(kj_g3_arena_alloc(1) == NULL);
    assert(kj_g3_arena_used() == KJ_TEST_ARENA_BYTES);

    kj_g3_arena_reset();
    assert(kj_g3_arena_used() == 0);
    assert(kj_g3_arena_alloc(4) != NULL);
    assert(kj_g3_arena_used() == 4);

    kj_g3_arena_release();
    assert(kj_g3_arena_used() == 0);
    assert(kj_g3_arena_alloc(4) != NULL); /* re-allocates lazily */
    assert(kj_g3_arena_used() == 4);
    kj_g3_arena_release();

    puts("PASS kj_g3_arena: overflow-safe sizing");
    return 0;
}
