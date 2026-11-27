/* Direct checks for the pure fixed-point matrix support split out of
 * kj_g3.c (cleanup task 15). The geometry differential suite still covers
 * the same helpers through real command streams. */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kj_g3_matrix.h"

static void test_identity_and_multiply(void)
{
    mtx a, b, out;
    mtx_identity(&a);
    mtx_identity(&b);
    a.m[0][3] = 3 * FX_ONE;
    a.m[1][3] = -2 * FX_ONE;
    mtx_mul(&out, &a, &b);
    assert(memcmp(&out, &a, sizeof(out)) == 0);
    mtx_mul(&out, &b, &a);
    assert(memcmp(&out, &a, sizeof(out)) == 0);
}

static void test_affine_matches_full(void)
{
    mtx rotation, translation, full, affine, affine_rot;
    mtx_identity(&rotation);
    rotation.m[0][0] = 3328; /* cos(0.4) in 20.12 */
    rotation.m[0][2] = -1536;
    rotation.m[2][0] = 1536;
    rotation.m[2][2] = 3328;
    mtx_identity(&translation);
    translation.m[3][0] = 5 * FX_ONE;
    translation.m[3][1] = -7 * FX_ONE;
    translation.m[3][2] = 11 * FX_ONE;

    mtx_mul(&full, &rotation, &translation);
    mtx_mul_affine(&affine, &rotation, &translation, false);
    assert(memcmp(&full, &affine, sizeof(full)) == 0);

    /* rotation_only ignores the left operand's bottom row and keeps b's. */
    mtx_mul_affine(&affine_rot, &rotation, &translation, true);
    assert(memcmp(affine_rot.m[3], translation.m[3], sizeof(affine_rot.m[3])) == 0);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(affine_rot.m[i][j] == full.m[i][j]);
        }
    }
}

static void test_identity_rotation(void)
{
    mtx m;
    mtx_identity(&m);
    assert(rotation_is_identity(&m));
    m.m[0][1] = 1;
    assert(!rotation_is_identity(&m));
    m.m[0][1] = 0;
    m.m[3][0] = 8 * FX_ONE; /* translation is not part of the test */
    assert(rotation_is_identity(&m));
}

static void test_sign_and_vector(void)
{
    assert(sign_bits(0x3FFu, 10) == 1023);
    assert(sign_bits(0x200u, 10) == -512);
    assert(sign_bits(0xFFFFu, 16) == -1);
    assert(sign_bits(0x7FFFu, 16) == 32767);
    assert(sign_bits(0xFFFF8000u, 16) == -32768);

    mtx m;
    mtx_identity(&m);
    m.m[0][0] = FX_ONE / 2;
    m.m[1][1] = FX_ONE * 2;
    const int32_t in[3] = {FX_ONE, FX_ONE, 0};
    int32_t out[3];
    transform_vector(in, &m, out);
    assert(out[0] == FX_ONE / 2);
    assert(out[1] == FX_ONE * 2);
    assert(out[2] == 0);
}

int main(void)
{
    test_identity_and_multiply();
    test_affine_matches_full();
    test_identity_rotation();
    test_sign_and_vector();
    puts("kj_g3_matrix_test: ok");
    return 0;
}
