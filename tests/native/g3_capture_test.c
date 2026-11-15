/* Skipped captures must preserve DS state and the next captured picture. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include SOURCE

unsigned char NitroPort_IoReg[0x1000];
void NitroPort_Log(const char *fmt, ...) { assert(strstr(fmt, "MISMATCH") == NULL); }
static uint64_t hash = 1469598103934665603ull;

static uint64_t digest(uint64_t value, const void *ptr, size_t count)
{
    const unsigned char *bytes = ptr;
    for (size_t i = 0; i < count; ++i) {
        value = (value ^ bytes[i]) * 1099511628211ull;
    }
    return value;
}

int main(int argc, char **argv)
{
    (void)argv;
    const bool discard = argc > 1;
    kj_g3_storage storage;
    for (unsigned i = 0; i < 2; ++i) {
        storage.verts[i] = calloc(KJ_G3_MAX_VERTS, sizeof(kj_g3_vertex));
        storage.indices[i] = calloc(KJ_G3_MAX_INDICES, sizeof(uint16_t));
    }
    kj_g3_init(&storage);
    uint32_t list[] = {
        0x40, 0, 0x20, 0x1234, 0x21, 0x200, 0x22, 0x00100020,
        0x23, 0, 0, 0x23, 4096, 0,
        /* A matrix restore inside a strip shares vertices across blocks. */
        0x14, 0, 0x23, 4096 << 16, 0, 0x23, 4096, 4096, 0x41,
    };
    for (unsigned frame = 0; frame < 240; ++frame) {
        const bool capture = (frame & 1u) == 0;
        uint64_t vertex_before = 0;
        for (unsigned i = 0; i < 2; ++i) {
            vertex_before = digest(vertex_before, storage.verts[i],
                                   KJ_G3_MAX_VERTS * sizeof(kj_g3_vertex));
        }
        kj_g3_set_capture(!discard || capture);
        for (unsigned call = 0; call < 6; ++call) {
            const uint32_t matrix[] = {4096, 0, 0, 0, 4096, 0, 0, 0, 4096,
                                       frame * 3, call * 3, 0};
            execute(0x10, (const uint32_t[]){2});
            execute(0x17, matrix);
            execute(0x13, (const uint32_t[]){0});
            execute(0x29, (const uint32_t[]){0x1f00c0u | (frame & 3u)});
            execute(0x2a, (const uint32_t[]){((frame / 4) & 3u) << 30});
            execute(0x30, (const uint32_t[]){0x12341234});
            execute(0x31, (const uint32_t[]){0x12341234});
            list[1] = frame & 3u;
            if (call & 1u) {
                for (size_t off = 0; off < sizeof(list) / 4; ++off) {
                    NitroPort_GxFifoWords(list + off, 1);
                }
            } else {
                NitroPort_GxFifoWords(list, sizeof(list) / 4);
            }
            execute(0x70, (const uint32_t[]){0, 0x10000000, 0x10001000});
            execute(0x71, (const uint32_t[]){0x12345678, 0x1234});
            execute(0x72, (const uint32_t[]){0x12345678});
            NitroPort_Gx3dRefreshStatus();
            hash = digest(hash, NitroPort_IoReg, sizeof(NitroPort_IoReg));
        }
        execute(0x50, (const uint32_t[]){frame & 3u});
        if (capture) {
            const kj_g3_frame *f = kj_g3_take_frame();
            assert(f != NULL && f->vert_count != 0);
            hash = digest(hash, f->verts, f->vert_count * sizeof(*f->verts));
            hash = digest(hash, f->indices, f->index_count * sizeof(*f->indices));
            hash = digest(hash, f->matrices, f->matrix_count * sizeof(*f->matrices));
            hash = digest(hash, f->batches, f->batch_count * sizeof(*f->batches));
        } else if (discard) {
            uint64_t after = 0;
            for (unsigned i = 0; i < 2; ++i) {
                after = digest(after, storage.verts[i], KJ_G3_MAX_VERTS * sizeof(kj_g3_vertex));
            }
            assert(after == vertex_before);
            assert(kj_g3_take_frame()->serial == frame);
        }
    }
    /* A discard decision cannot expose an incomplete scene if a game tick
     * ends halfway through it. Finish it dry, then capture the next swap. */
    if (discard) {
        const kj_g3_frame *display = kj_g3_take_frame();
        kj_g3_set_capture(false);
        execute(0x40, (const uint32_t[]){0});
        execute(0x23, (const uint32_t[]){0, 0});
        kj_g3_set_capture(true);
        assert(!s_capture_scene);
        execute(0x50, (const uint32_t[]){0});
        assert(kj_g3_take_frame() == display && s_capture_scene);
    }
    printf("%016llx\n", (unsigned long long)hash);
    return 0;
}
