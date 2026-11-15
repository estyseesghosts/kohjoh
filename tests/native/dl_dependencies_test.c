/* Exercise cache reuse with incoming colours and active/pending light changes.
 * Verification mode additionally compares a replay hit to a fresh decode. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include SOURCE

unsigned char NitroPort_IoReg[0x1000];
void NitroPort_Log(const char *fmt, ...) { assert(strstr(fmt, "MISMATCH") == NULL); }
static uint64_t hash = 1469598103934665603ull;

static void digest(const void *p, size_t n)
{
    const unsigned char *bytes = p;
    while (n-- != 0) {
        hash = (hash ^ *bytes++) * 1099511628211ull;
    }
}

int main(void)
{
    kj_g3_storage storage;
    for (unsigned i = 0; i < 2; ++i) {
        storage.verts[i] = calloc(KJ_G3_MAX_VERTS, sizeof(kj_g3_vertex));
        storage.indices[i] = calloc(KJ_G3_MAX_INDICES, sizeof(uint16_t));
    }
    kj_g3_init(&storage);
    /* BEGIN / NORMAL / full vertices / END: no incoming colour when lit. */
    uint32_t lists[][18] = {
        {0x40, 0, 0x21, 0x200, 0x23, 0, 0, 0x23, 4096, 0,
         0x23, 4096 << 16, 0, 0x41, 0, 0, 0, 0},
        /* Normal is before BEGIN: reads active lights, not pending lights. */
        {0x21, 0x200, 0x40, 0, 0x23, 0, 0, 0x23, 4096, 0,
         0x23, 4096 << 16, 0, 0x41, 0, 0, 0, 0},
        /* A vertex before NORMAL must still depend on incoming colour. */
        {0x40, 0, 0x23, 0, 0, 0x21, 0x200, 0x23, 4096, 0,
         0x23, 4096 << 16, 0, 0x41, 0, 0, 0, 0},
        /* In-list POLYGON_ATTR changes override the entry pending lights. */
        {0x29, 0x1f00c0, 0x40, 0, 0x21, 0x200, 0x23, 0, 0,
         0x23, 4096, 0, 0x23, 4096 << 16, 0, 0x41, 0, 0},
    };
    uint32_t states[][20] = {
        {0x10, 2, 0x15, 0x32, 0x200, 0x30, 0x42100210, 0x31, 0x42100210,
         0x60, 0xbfff0000, 0x71, 0x10002000, 0x3000, 0x29, 0x1f00c1,
         0x40, 0, 0x41, 0},
        {0x10, 2, 0x15, 0x32, 0x200, 0x30, 0x42108210, 0x31, 0x42100210,
         0x60, 0xbfff0000, 0x71, 0x10002000, 0x3000, 0x29, 0x1f00c1,
         0x40, 0, 0x41, 0},
    };
    for (unsigned frame = 0; frame < 256; ++frame) {
        for (unsigned state = 0; state < 2; ++state) {
            states[state][12] = frame * 313 + state;
            states[state][6] = 0x42100210u | ((frame & 1u) << 15);
            /* Same storage may change from state-only to vertex-producing
             * commands and back. Offsets alone cannot establish validity. */
            states[state][11] = (frame & 16u) ? 0x23u : 0x71u;
        }
        for (unsigned list = 0; list < 4; ++list) {
            for (unsigned call = 0; call < 3; ++call) {
                execute(0x29, (const uint32_t[]){0x1f00c0u | ((frame >> 1) & 1u)});
                execute(0x40, (const uint32_t[]){0});
                execute(0x41, (const uint32_t[]){0});
                execute(0x29, (const uint32_t[]){0x1f00c0u | (frame & 1u)});
                execute(0x20, (const uint32_t[]){frame * 41 + call * 117});
                NitroPort_GxFifoWords(lists[list], 18);
                NitroPort_GxFifoWords(states[list & 1u], 20);
                digest(s_coord, sizeof(s_coord));
                digest(&s_color, sizeof(s_color));
                digest(&s_in_begin, sizeof(s_in_begin));
                digest(&s_attr, sizeof(s_attr));
                digest(s_light_dir, sizeof(s_light_dir));
                digest(NitroPort_IoReg, sizeof(NitroPort_IoReg));
            }
        }
        execute(0x50, (const uint32_t[]){0});
        const kj_g3_frame *f = kj_g3_take_frame();
        digest(f->verts, f->vert_count * sizeof(*f->verts));
        digest(f->indices, f->index_count * sizeof(*f->indices));
        digest(f->matrices, f->matrix_count * sizeof(*f->matrices));
        digest(f->batches, f->batch_count * sizeof(*f->batches));
        digest(&s_color, sizeof(s_color));
    }
    printf("%016llx\n", (unsigned long long)hash);
    return 0;
}
