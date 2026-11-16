/* Pure state lists (NNS_G3dGlbFlushP): a restored snapshot must leave the
 * same DS state, the same game-visible test results and the same pictures
 * as executing the list, while models change matrices and materials
 * between calls and the camera and lights move. Compare the printed hash
 * across cached, uncached and live-verification builds. */
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
static uint32_t rng = 0x3c6ef372u;

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

/* The GLB layout: MTX_MODE, LOAD_4x4, MTX_MODE, LOAD_4x3; LIGHT_VECTOR x4;
 * DIF_AMB, SPE_EMI, POLYGON_ATTR, VIEWPORT; LIGHT_COLOR x4; MULT_4x3,
 * SCALE, TEXIMAGE_PARAM. 63 words. */
static size_t make_glb(uint32_t *g, unsigned camera, unsigned lights, unsigned base)
{
    size_t n = 0;
    g[n++] = 0x17101610u;
    g[n++] = 0;
    for (unsigned i = 0; i < 16; ++i) {
        g[n++] = i % 5u == 0 ? 4096u : (i == 11 ? (uint32_t)-4096 : 0u);
    }
    g[n++] = 2;
    for (unsigned i = 0; i < 12; ++i) {
        g[n++] = i % 4u == 0 ? 4096u : 0u;
    }
    g[3 + 16 + 1] = 4096u - camera * 64u; /* camera rotation */
    g[3 + 16 + 2] = camera * 97u;
    g[3 + 16 + 10] = camera * 4096u;      /* camera translation */
    g[n++] = 0x32323232u;
    for (unsigned i = 0; i < 4; ++i) {
        g[n++] = (i << 30) | ((lights * 7919u + i * 104729u) & 0x3fffffffu);
    }
    g[n++] = 0x60293130u;
    g[n++] = 0x0c634210u | (lights & 1u ? 0x8000u : 0u);
    g[n++] = 0x10421084u;
    g[n++] = 0x001f00c3u;
    g[n++] = 0xbfff0000u;
    g[n++] = 0x33333333u;
    for (unsigned i = 0; i < 4; ++i) {
        g[n++] = (i << 30) | ((0x7fffu >> i) ^ (lights & 7u));
    }
    g[n++] = 0x002a1b19u;
    for (unsigned i = 0; i < 9; ++i) { /* MULT_4x3: base rotation, translation */
        g[n++] = i % 4u == 0 ? 4096u : 0u;
    }
    g[n++] = base * 4096u; /* NNS rewrites the base transform per model */
    g[n++] = 0;
    g[n++] = (uint32_t)-(int32_t)(base * 2048u);
    g[n++] = 4096;
    g[n++] = 4096;
    g[n++] = 4096;
    g[n++] = 0;
    return n;
}

int main(void)
{
    kj_g3_storage storage;
    for (unsigned i = 0; i < 2; ++i) {
        storage.verts[i] = calloc(KJ_G3_MAX_VERTS, sizeof(kj_g3_vertex));
        storage.indices[i] = calloc(KJ_G3_MAX_INDICES, 2);
    }
    kj_g3_init(&storage);
    static uint32_t glb[64];
    uint32_t mesh[200];
    size_t m = 0;
    mesh[m++] = 0x40;
    mesh[m++] = 0;
    for (unsigned v = 0; v < 12; ++v) {
        mesh[m++] = 0x21;
        mesh[m++] = next() & 0x3fffffffu;
        mesh[m++] = 0x23;
        mesh[m++] = next();
        mesh[m++] = next() & 0xffffu;
    }
    mesh[m++] = 0x41;
    for (unsigned frame = 0; frame < 300; ++frame) {
        for (unsigned model = 0; model < 12; ++model) {
            /* The camera moves every 4 pictures, the lights every 50. */
            const size_t n = make_glb(glb, frame / 4u, frame / 50u, model % 3u);
            assert(n == 63);
            NitroPort_GxFifoWords(glb, n);
            /* The model's own transform and material on top. */
            const uint32_t turn = model == 2u || model == 3u ? 0u : model; /* 2, 3: no rotation */
            execute(0x19, (const uint32_t[]){4096, turn * 300u, 0, 0, 4096, turn * 200u, 0, 0, 4096,
                                             model * 8192u, 0, (uint32_t)-(int32_t)(model * 4096u)});
            /* One disturbance per even model, and a quiet model after it,
             * so each restore invalidation is observable on its own (the
             * next GLB must undo it, and nothing else invalidates). */
            switch (model) {
            case 0:
                execute(0x30, (const uint32_t[]){0x7fff0421u});
                break;
            case 2:
                execute(0x32, (const uint32_t[]){0x1ff003ffu});
                break;
            case 4:
                execute(0x2a, (const uint32_t[]){0x12345678u});
                break;
            case 6:
                execute(0x33, (const uint32_t[]){(1u << 30) | 0x001fu});
                break;
            case 8:
                execute(0x60, (const uint32_t[]){0x7f7f0000u});
                break;
            case 10:
                execute(0x31, (const uint32_t[]){0x0000ffffu});
                execute(0x29, (const uint32_t[]){0x001f0081u});
                break;
            default:
                break;
            }
            execute(0x71, (const uint32_t[]){0x10001000u, 0x1000u}); /* POS_TEST */
            execute(0x72, (const uint32_t[]){next() & 0x3fffffffu}); /* VEC_TEST */
            NitroPort_GxFifoWords(mesh, m);
            /* Live (uncached) lit vertices: the normal cache and the PICA
             * lighting latch see only the generations a restore bumps. */
            execute(0x40, (const uint32_t[]){0});
            execute(0x21, (const uint32_t[]){0x20100000u}); /* z = -511/512, facing the disturbed light */
            execute(0x23, (const uint32_t[]){0x00000000u, 0});
            execute(0x23, (const uint32_t[]){0x00001000u, 0});
            execute(0x23, (const uint32_t[]){0x10000000u, 0});
            execute(0x41, NULL);
            digest(NitroPort_IoReg + 0x620, 0x20);
        }
        execute(0x50, (const uint32_t[]){0});
        NitroPort_Gx3dSyncResults();
        digest(NitroPort_IoReg + 0x640, 0x64);
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
    }
#if !defined(KJ_DL_NO_CACHE) && !defined(KJ_DL_VERIFY)
    const dl_entry *e = dl_slot(glb, 63);
    assert(e->ptr == glb && e->snap != NULL && e->snap->mask != 0);
#endif
    printf("%016llx\n", (unsigned long long)hash);
    return 0;
}
