/* Real command/state differential fixture: implicit matrix columns, aliasing,
 * all matrix modes, light transforms, and identity operations after restores. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include SOURCE

unsigned char NitroPort_IoReg[0x1000];
void NitroPort_Log(const char *fmt, ...) {
    (void)fmt;
}
static uint32_t rng = 0x123abcde;
static uint64_t hash = 1469598103934665603ull;

static uint32_t next(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static void digest(const void *p, size_t n) {
    const unsigned char *bytes = p;
    while (n-- != 0) {
        hash = (hash ^ *bytes++) * 1099511628211ull;
    }
}

int main(void) {
    kj_g3_storage storage;
    for (unsigned i = 0; i < 2; ++i) {
        storage.verts[i] = calloc(KJ_G3_MAX_VERTS, sizeof(kj_g3_vertex));
        storage.indices[i] = calloc(KJ_G3_MAX_INDICES, sizeof(uint16_t));
    }
    kj_g3_init(&storage);
    for (unsigned trial = 0; trial < 10000; ++trial) {
        const unsigned mode = trial & 3;
        uint32_t right[16], left[16];
        for (unsigned i = 0; i < 16; ++i) {
            right[i] = next();
            left[i] = next();
        }
        s_mtx_mode = mode;
        execute(0x16, right); /* include projective right operands */
        const unsigned op = 0x18 + trial % 3;
        if (trial % 5 == 0) {
            memset(left, 0, sizeof(left));
            left[0] = left[4] = left[8] = FX_ONE;
            if (op == 0x19 && (trial & 1u)) {
                left[9] = next();
                left[10] = next();
                left[11] = next();
            }
        }
        execute(op, left);
        execute(0x13, (const uint32_t[]){3});
        execute(0x1B, (const uint32_t[]){FX_ONE, FX_ONE, FX_ONE});
        execute(0x1C, (const uint32_t[]){0, 0, 0});
        digest(&s_proj, sizeof(s_proj));
        digest(&s_pos, sizeof(s_pos));
        digest(&s_vec, sizeof(s_vec));
        digest(&s_tex, sizeof(s_tex));
        digest(clip_matrix(), sizeof(mtx));
        for (unsigned light = 0; light < 4; ++light) {
            execute(0x32, (const uint32_t[]){(next() & 0x3fffffffu) | light << 30});
        }
        digest(s_light_dir, sizeof(s_light_dir));
        digest(s_light_half, sizeof(s_light_half));
        execute(0x14, (const uint32_t[]){3});
        execute(0x1A, (const uint32_t[]){FX_ONE, 0, 0, 0, FX_ONE, 0, 0, 0, FX_ONE});
        digest(&s_pos, sizeof(s_pos));
        digest(&s_vec, sizeof(s_vec));
    }
    printf("%016llx\n", (unsigned long long)hash);
    return 0;
}
