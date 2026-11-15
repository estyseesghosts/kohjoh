#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include SOURCE
#include "kj_lighting.h"

unsigned char NitroPort_IoReg[0x1000];
void NitroPort_Log(const char *fmt, ...) { assert(strstr(fmt, "MISMATCH") == NULL); }
static uint64_t hash = 1469598103934665603ull;
static uint32_t rng = 0x719aba;

static uint32_t next(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static void digest(const void *ptr, size_t count)
{
    const unsigned char *bytes = ptr;
    while (count-- != 0) {
        hash = (hash ^ *bytes++) * 1099511628211ull;
    }
}

static void triangle_vertex(const kj_g3_frame *frame, const kj_g3_batch *batch, uint16_t index)
{
    kj_g3_vertex v = frame->verts[index];
    if (v.pad & 128u) {
        uint32_t packed;
        int32_t n[3];
        memcpy(&packed, &v.r, 4);
        kj_lighting_unpack_normal(packed, v.mtx, v.pad, n);
        const uint16_t color = kj_lighting_color(n, &frame->lighting[frame->vertex_lights[index]]);
        v.r = EXPAND5[color & 31u];
        v.g = EXPAND5[color >> 5 & 31u];
        v.b = EXPAND5[color >> 10 & 31u];
        v.a = frame->vertex_alpha[index];
        v.mtx &= 7u;
        v.pad = 0;
    }
    digest(&v.x, 6);
    digest(&v.r, 8);
    digest(&frame->matrices[batch->first_matrix + v.mtx], sizeof(kj_g3_matrix));
}

int main(void)
{
    kj_g3_storage storage;
    for (unsigned i = 0; i < 2; ++i) {
        storage.verts[i] = calloc(KJ_G3_MAX_VERTS, sizeof(kj_g3_vertex));
        storage.indices[i] = calloc(KJ_G3_MAX_INDICES, 2);
    }
    kj_g3_init(&storage);
    uint32_t list[512];
    size_t count = 0;
    list[count++] = 0x40;
    list[count++] = 2;
    for (unsigned v = 0; v < 24; ++v) {
        list[count++] = 0x14;
        list[count++] = v & 15u;
        list[count++] = 0x21;
        list[count++] = next() & 0x3fffffffu;
        if (v % 6u == 0) {
            /* Material changes after NORMAL must not change its colour. */
            list[count++] = 0x31;
            list[count++] = next();
        }
        if (v % 7u == 0) {
            list[count++] = 0x20;
            list[count++] = next() & 32767u;
        }
        list[count++] = 0x14;
        list[count++] = (v + 3u) & 15u;
        list[count++] = 0x22;
        list[count++] = next();
        list[count++] = 0x23;
        list[count++] = next();
        list[count++] = next();
    }
    list[count++] = 0x41;
    for (unsigned frame = 0; frame < 400; ++frame) {
        execute(0x10, (const uint32_t[]){2});
        for (unsigned slot = 0; slot < 16; ++slot) {
            uint32_t m[12] = {4096, 0, 0, 0, 4096, 0, 0, 0, 4096, slot * 32, frame * 3, 0};
            m[0] = (frame & 1u) ? 9000 : 4096; /* overflow-normal CPU path */
            m[1] = slot * 8;
            execute(0x17, m);
            execute(0x13, &slot);
        }
        for (unsigned light = 0; light < 4; ++light) {
            uint32_t value = light << 30 | (next() & 0x3fffffffu);
            execute(0x32, &value);
            value = light << 30 | (next() & 32767u);
            execute(0x33, &value);
        }
        uint32_t shininess[32];
        for (unsigned i = 0; i < 32; ++i) {
            shininess[i] = next();
        }
        execute(0x34, shininess);
        execute(0x30, (const uint32_t[]){next()});
        execute(0x31, (const uint32_t[]){next()});
        for (unsigned primitive = 0; primitive < 4; ++primitive) {
            execute(0x29, (const uint32_t[]){0x00c0u | ((1u + frame % 31u) << 16) | (frame & 15u)});
            execute(0x2a, (const uint32_t[]){(frame / 20u & 3u) << 30});
            list[1] = primitive;
            NitroPort_GxFifoWords(list, count);
        }
        execute(0x50, (const uint32_t[]){frame & 3u});
        const kj_g3_frame *f = kj_g3_take_frame();
        for (unsigned b = 0; b < f->batch_count; ++b) {
            const kj_g3_batch *batch = &f->batches[b];
            for (unsigned i = 0; i < batch->index_count; i += 3) {
                digest(&batch->poly_attr, 16);
                for (unsigned v = 0; v < 3; ++v) {
                    triangle_vertex(f, batch, f->indices[batch->first_index + i + v]);
                }
            }
        }
        NitroPort_Gx3dSyncResults();
        digest(NitroPort_IoReg + 0x620, 0x84); /* tests/matrices, not port batch-duplicate counts */
    }
    printf("%016llx\n", (unsigned long long)hash);
    return 0;
}
