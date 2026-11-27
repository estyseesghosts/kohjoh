/* Direct checks for the pure fixed-point matrix support split out of
 * kj_g3.c (cleanup task 15). The geometry differential suite still covers
 * the same helpers through real command streams. */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kj_g3_matrix.h"

static void test_identity_and_multiply(void) {
    kj_g3_mtx a, b, out;
    kj_g3_mtx_identity(&a);
    kj_g3_mtx_identity(&b);
    a.m[0][3] = 3 * KJ_G3_FX_ONE;
    a.m[1][3] = -2 * KJ_G3_FX_ONE;
    kj_g3_mtx_mul(&out, &a, &b);
    assert(memcmp(&out, &a, sizeof(out)) == 0);
    kj_g3_mtx_mul(&out, &b, &a);
    assert(memcmp(&out, &a, sizeof(out)) == 0);
}

static void test_affine_matches_full(void) {
    kj_g3_mtx rotation, translation, full, affine, affine_rot;
    kj_g3_mtx_identity(&rotation);
    rotation.m[0][0] = 3328; /* cos(0.4) in 20.12 */
    rotation.m[0][2] = -1536;
    rotation.m[2][0] = 1536;
    rotation.m[2][2] = 3328;
    kj_g3_mtx_identity(&translation);
    translation.m[3][0] = 5 * KJ_G3_FX_ONE;
    translation.m[3][1] = -7 * KJ_G3_FX_ONE;
    translation.m[3][2] = 11 * KJ_G3_FX_ONE;

    kj_g3_mtx_mul(&full, &rotation, &translation);
    kj_g3_mtx_mul_affine(&affine, &rotation, &translation, false);
    assert(memcmp(&full, &affine, sizeof(full)) == 0);

    /* rotation_only ignores the left operand's bottom row and keeps b's. */
    kj_g3_mtx_mul_affine(&affine_rot, &rotation, &translation, true);
    assert(memcmp(affine_rot.m[3], translation.m[3], sizeof(affine_rot.m[3])) == 0);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(affine_rot.m[i][j] == full.m[i][j]);
        }
    }
}

static void test_identity_rotation(void) {
    kj_g3_mtx m;
    kj_g3_mtx_identity(&m);
    assert(kj_g3_mtx_is_rotation_identity(&m));
    m.m[0][1] = 1;
    assert(!kj_g3_mtx_is_rotation_identity(&m));
    m.m[0][1] = 0;
    m.m[3][0] = 8 * KJ_G3_FX_ONE; /* translation is not part of the test */
    assert(kj_g3_mtx_is_rotation_identity(&m));
}

static void test_sign_and_vector(void) {
    assert(sign_bits(0x1FFu, 10) == 511);
    assert(sign_bits(0x200u, 10) == -512);
    assert(sign_bits(0x3FFu, 10) == -1);
    assert(sign_bits(0xFFFFu, 16) == -1);
    assert(sign_bits(0x7FFFu, 16) == 32767);
    assert(sign_bits(0xFFFF8000u, 16) == -32768);

    kj_g3_mtx m;
    kj_g3_mtx_identity(&m);
    m.m[0][0] = KJ_G3_FX_ONE / 2;
    m.m[1][1] = KJ_G3_FX_ONE * 2;
    const int32_t in[3] = {KJ_G3_FX_ONE, KJ_G3_FX_ONE, 0};
    int32_t out[3];
    transform_vector(in, &m, out);
    assert(out[0] == KJ_G3_FX_ONE / 2);
    assert(out[1] == KJ_G3_FX_ONE * 2);
    assert(out[2] == 0);
}

int main(void) {
    test_identity_and_multiply();
    test_affine_matches_full();
    test_identity_rotation();
    test_sign_and_vector();
    puts("kj_g3_matrix_test: ok");
    return 0;
}
