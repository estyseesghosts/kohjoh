/* Whole-block culling: replayed retained lists must draw exactly the
 * triangles of the uncached decoder that are not wholly outside a side
 * plane, and must drop some blocks that are. Run against KJ_DL_NO_CACHE
 * (no retained blocks, no culling) and compare the printed hashes. */
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
static uint32_t rng = 0x5eed1234u;

static uint32_t next(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static uint64_t tri_hash(const kj_g3_frame *f, const kj_g3_batch *b, unsigned first,
                         bool *outside) {
    uint64_t h = 1469598103934665603ull;
    unsigned out_all = 0xFu;
    for (unsigned k = 0; k < 3; ++k) {
        const kj_g3_vertex *v = &f->verts[f->indices[b->first_index + first + k]];
        const kj_g3_matrix *m = &f->matrices[b->first_matrix + (v->mtx & 15u)];
        float o[4];
        for (unsigned j = 0; j < 4; ++j) {
            o[j] = v->x / 4096.0f * m->m[0][j] + v->y / 4096.0f * m->m[1][j] +
                   v->z / 4096.0f * m->m[2][j] + m->m[3][j];
        }
        unsigned out = 0;
        out |= o[0] < -1.25f * o[3] ? 1u : 0u;
        out |= o[0] > 1.25f * o[3] ? 2u : 0u;
        out |= o[1] < -o[3] ? 4u : 0u;
        out |= o[1] > o[3] ? 8u : 0u;
        out_all &= out;
        const unsigned char *bytes = (const unsigned char *)o;
        for (unsigned i = 0; i < sizeof(o); ++i) {
            h = (h ^ bytes[i]) * 1099511628211ull;
        }
        bytes = &v->r;
        for (unsigned i = 0; i < 8; ++i) {
            h = (h ^ bytes[i]) * 1099511628211ull;
        }
    }
    *outside = out_all != 0;
    return h;
}

static int cmp64(const void *a, const void *b) {
    const uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

int main(void) {
    kj_g3_storage storage;
    for (unsigned i = 0; i < 2; ++i) {
        storage.verts[i] = calloc(KJ_G3_MAX_VERTS, sizeof(kj_g3_vertex));
        storage.indices[i] = calloc(KJ_G3_MAX_INDICES, 2);
    }
    kj_g3_init(&storage);
    /* Twelve retained lists: two materials (two blocks), one primitive
     * kind each, small meshes around the origin. */
    static uint32_t lists[12][400];
    size_t sizes[12];
    for (unsigned l = 0; l < 12; ++l) {
        size_t n = 0;
        lists[l][n++] = 0x40;
        lists[l][n++] = l & 3u;
        for (unsigned part = 0; part < 2; ++part) {
            if (part == 1) {
                lists[l][n++] = 0x2a; /* material change: a second block */
                lists[l][n++] = 0x10000u * (l + 1u);
            }
            for (unsigned v = 0; v < 24; ++v) {
                lists[l][n++] = 0x20;
                lists[l][n++] = next() & 0x7fffu;
                lists[l][n++] = 0x23;
                lists[l][n++] =
                    (next() & 0x1fffu) - 0x1000u + (((next() & 0x1fffu) - 0x1000u) << 16);
                lists[l][n++] = (next() & 0x1fffu) - 0x1000u;
            }
        }
        lists[l][n++] = 0x41;
        sizes[l] = n;
    }
    const uint32_t proj[16] = {
        /* perspective-like: w = -z */
        4096, 0, 0, 0, 0, 4096, 0, 0, 0, 0, (uint32_t)-4096, (uint32_t)-4096, 0, 0, (uint32_t)-8192,
        0,
    };
    uint64_t visible_hash = 1469598103934665603ull;
    uint32_t total = 0, visible = 0;
    static uint64_t tris[65536];
    for (unsigned frame = 0; frame < 300; ++frame) {
        execute(0x10, (const uint32_t[]){0});
        execute(0x16, proj);
        execute(0x10, (const uint32_t[]){2});
        execute(0x15, NULL);
        execute(0x29, (const uint32_t[]){0x001f00c0});
        for (unsigned l = 0; l < 12; ++l) {
            execute(0x15, NULL);
            /* Slide along x: some lists in view, some wholly outside. */
            const int32_t tx = ((int32_t)(l * 9u + frame) % 90 - 45) * 4096 / 4;
            const int32_t ty = ((int32_t)(l * 5u) % 30 - 15) * 4096 / 4;
            execute(0x1c, (const uint32_t[]){(uint32_t)tx, (uint32_t)ty, (uint32_t)(-6 * 4096)});
            NitroPort_GxFifoWords(lists[l], sizes[l]);
        }
        execute(0x50, (const uint32_t[]){0});
        const kj_g3_frame *f = kj_g3_take_frame();
        unsigned n = 0;
        for (unsigned b = 0; b < f->batch_count; ++b) {
            const kj_g3_batch *batch = &f->batches[b];
            for (unsigned i = 0; i < batch->index_count; i += 3) {
                bool outside;
                const uint64_t h = tri_hash(f, batch, i, &outside);
                ++total;
                if (!outside) {
                    tris[n++] = h ^ batch->tex_param;
                }
            }
        }
        visible += n;
        qsort(tris, n, sizeof(tris[0]), cmp64);
        for (unsigned i = 0; i < n; ++i) {
            visible_hash = (visible_hash ^ tris[i]) * 1099511628211ull;
        }
    }
    printf("visible %016llx\n", (unsigned long long)visible_hash);
    fprintf(stderr, "triangles %lu visible %lu\n", (unsigned long)total, (unsigned long)visible);
    return 0;
}
