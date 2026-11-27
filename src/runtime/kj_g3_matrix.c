/*
 * kj_g3_matrix.c: matrix products used by the DS geometry engine.
 *
 * Kept separate from kj_g3.c because these functions operate only on their
 * arguments. The command handlers, matrix stacks and hot vertex work remain
 * in kj_g3.c; see the ownership note there.
 */

#include "kj_g3_matrix.h"

#include <string.h>

void kj_g3_mtx_identity(kj_g3_mtx *m) {
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = m->m[3][3] = KJ_G3_FX_ONE;
}

void kj_g3_mtx_mul(kj_g3_mtx *out, const kj_g3_mtx *a, const kj_g3_mtx *b) {
    kj_g3_mtx r;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int64_t acc = 0;
            for (int k = 0; k < 4; ++k) {
                acc += (int64_t)a->m[i][k] * b->m[k][j];
            }
            r.m[i][j] = (int32_t)(acc >> 12);
        }
    }
    *out = r;
}

/* Preserve the single fixed-point rounding after each dot product, including
 * projective right operands. Repeated base transforms need 36/48 products
 * instead of 64, before the identity/translation fast paths in kj_g3.c.
 * See Coffeepot cpu-optimization.md: reduce integer work before using SIMD. */
void kj_g3_mtx_mul_affine(kj_g3_mtx *out, const kj_g3_mtx *a, const kj_g3_mtx *b,
                          bool rotation_only) {
    kj_g3_mtx r;
    const unsigned rows = rotation_only ? 3u : 4u;
    for (unsigned i = 0; i < rows; ++i) {
        for (unsigned j = 0; j < 4; ++j) {
            int64_t acc = (int64_t)a->m[i][0] * b->m[0][j] + (int64_t)a->m[i][1] * b->m[1][j] +
                          (int64_t)a->m[i][2] * b->m[2][j];
            if (i == 3) {
                acc += (int64_t)KJ_G3_FX_ONE * b->m[3][j];
            }
            r.m[i][j] = (int32_t)(acc >> 12);
        }
    }
    if (rotation_only) {
        memcpy(r.m[3], b->m[3], sizeof(r.m[3]));
    }
    *out = r;
}

bool kj_g3_mtx_is_rotation_identity(const kj_g3_mtx *m) {
    return m->m[0][0] == KJ_G3_FX_ONE && m->m[1][1] == KJ_G3_FX_ONE && m->m[2][2] == KJ_G3_FX_ONE &&
           m->m[0][1] == 0 && m->m[0][2] == 0 && m->m[1][0] == 0 && m->m[1][2] == 0 &&
           m->m[2][0] == 0 && m->m[2][1] == 0;
}
