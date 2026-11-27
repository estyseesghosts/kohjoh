/*
 * kj_g3_matrix.h: pure fixed-point matrix support for the DS geometry
 * engine. Private to src/runtime: it references no live G3 state, so the
 * matrix tests and kj_g3.c can share it without exposing it to the bridge.
 */
#ifndef KJ_G3_MATRIX_H
#define KJ_G3_MATRIX_H

#include <stdbool.h>
#include <stdint.h>

#define FX_ONE 4096

typedef struct kj_g3_mtx {
    int32_t m[4][4]; /* 20.12, row-vector convention: v' = v * M */
} kj_g3_mtx;

void mtx_identity(kj_g3_mtx *m);
/* out = a * b (row-vector convention: apply a, then b). */
void mtx_mul(kj_g3_mtx *out, const kj_g3_mtx *a, const kj_g3_mtx *b);
/* Left operand from MTX_MULT_4x3/3x3 has an implicit affine column. */
void mtx_mul_affine(kj_g3_mtx *out, const kj_g3_mtx *a, const kj_g3_mtx *b, bool rotation_only);
bool rotation_is_identity(const kj_g3_mtx *m);

/* Sign-extend `bits` of v. Both helpers run per vertex, so they stay in the
 * private header instead of adding a cross-translation-unit call to the hot
 * decode loop. */
static inline int32_t sign_bits(uint32_t v, int bits) {
    const uint32_t mask = 1u << (bits - 1);
    v &= (1u << bits) - 1u;
    return (int32_t)((v ^ mask) - mask);
}

static inline void transform_vector(const int32_t in[3], const kj_g3_mtx *m, int32_t out[3]) {
    for (int j = 0; j < 3; ++j) {
        int64_t acc = 0;
        for (int i = 0; i < 3; ++i) {
            acc += (int64_t)in[i] * m->m[i][j];
        }
        out[j] = (int32_t)(acc >> 12);
    }
}

#endif
