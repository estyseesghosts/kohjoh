/* Animated lit strips use live state, including between content rechecks. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include SOURCE

unsigned char NitroPort_IoReg[0x1000];
void NitroPort_Log(const char *fmt, ...) {
    assert(strstr(fmt, "MISMATCH") == NULL);
}
static uint64_t hash = 1469598103934665603ull;
static uint32_t rng = 984521u;

static uint32_t next(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static void digest(const void *ptr, size_t count) {
    const unsigned char *bytes = ptr;
    for (size_t i = 0; i < count; ++i) {
        hash = (hash ^ bytes[i]) * 1099511628211ull;
    }
}

int main(void) {
    kj_g3_storage storage;
    for (unsigned i = 0; i < 2; ++i) {
        storage.verts[i] = calloc(KJ_G3_MAX_VERTS, sizeof(kj_g3_vertex));
        storage.indices[i] = calloc(KJ_G3_MAX_INDICES, sizeof(uint16_t));
    }
    kj_g3_init(&storage);
    uint32_t lists[4][32];
    size_t sizes[4];
    for (unsigned l = 0; l < 4; ++l) {
        const uint32_t words[] = {
            0x40, l,    0x21,     0x200, 0x23,       0, 0,    0x23, 4096, 0,    0x14,
            0,    0x21, 0x100200, 0x23,  4096 << 16, 0, 0x23, 4096, 4096, 0x41,
        };
        memcpy(lists[l], words, sizeof(words));
        sizes[l] = sizeof(words) / 4;
    }
    for (unsigned frame = 0; frame < 600; ++frame) {
        execute(0x10, (const uint32_t[]){2});
        /* A changing vector matrix defeats baked lighting; it must not
         * affect admission of an immutable instruction plan. */
        uint32_t matrix[12];
        for (unsigned i = 0; i < 12; ++i) {
            matrix[i] = (next() & 8191u) - 4096u;
        }
        execute(0x17, matrix);
        execute(0x13, (const uint32_t[]){0});
        for (unsigned l = 0; l < 4; ++l) {
            execute(0x32, (const uint32_t[]){(l << 30) | (next() & 0x3fffffffu)});
            execute(0x33, (const uint32_t[]){(l << 30) | (next() & 0x7fffu)});
        }
        execute(0x30, (const uint32_t[]){next()});
        execute(0x31, (const uint32_t[]){next()});
        uint32_t shininess[32];
        for (unsigned i = 0; i < 32; ++i) {
            shininess[i] = next();
        }
        execute(0x34, shininess);
        execute(0x29, (const uint32_t[]){0x1f00c0u | (1u + frame % 15u)});
        execute(0x2a, (const uint32_t[]){((frame / 20u) & 3u) << 30});
        for (unsigned l = 0; l < 4; ++l) {
            /* Change both parameters and layout away from a 64-swap
             * recheck. An old plan must never execute the wrong opcode. */
            lists[l][12] = frame % 13u == 5u ? 0x20u : 0x21u;
            lists[l][13] = frame % 7u == 3u ? 0x3fffffffu : 0x100200u;
            NitroPort_GxFifoWords(lists[l], sizes[l]);
#ifdef TEST_COMMAND_PLAN
            if (frame == 0 && l == 0) {
                const dl_entry *e = dl_slot(lists[l], (uint32_t)sizes[l]);
                assert(e->never && e->command_plan && e->nevents != 0);
            }
#endif
        }
        execute(0x50, (const uint32_t[]){frame & 3u});
        NitroPort_Gx3dRefreshStatus();
        const kj_g3_frame *f = kj_g3_take_frame();
        digest(f->verts, f->vert_count * sizeof(*f->verts));
        digest(f->indices, f->index_count * sizeof(*f->indices));
        digest(f->matrices, f->matrix_count * sizeof(*f->matrices));
        digest(f->batches, f->batch_count * sizeof(*f->batches));
        digest(NitroPort_IoReg, sizeof(NitroPort_IoReg));
    }
    printf("%016llx\n", (unsigned long long)hash);
    return 0;
}
