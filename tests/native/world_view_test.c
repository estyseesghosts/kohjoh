/* Native projection tests for pixel-perfect.txt Task 3.
 * Build: cc -I src tests/native/world_view_test.c src/graphics/world_view.c \
 *        -o world_view_test
 * Covers viewport centres, screen corners, negative coordinates, clipping,
 * screen rotation and non-fullscreen viewports. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "graphics/world_view.h"

static void expect_close(float a, float b)
{
    assert(fabsf(a - b) < 0.0001f);
}

static void test_canonical(void)
{
    kj_world_view v;
    float sx, sy, dx, dy;

    kj_world_view_init_display(&v, KJ_FIELD_SCALE_LEGACY);
    assert(v.surface_w == 400 && v.surface_h == 240);
    kj_world_view_map(&v, 128.0f, 96.0f, &sx, &sy);
    expect_close(sx, 200.0f); /* DS centre -> surface centre */
    expect_close(sy, 120.0f);
    kj_world_view_map(&v, 0.0f, 0.0f, &sx, &sy);
    expect_close(sx, 40.0f);
    expect_close(sy, 0.0f);
    kj_world_view_map(&v, 256.0f, 192.0f, &sx, &sy);
    expect_close(sx, 360.0f);
    expect_close(sy, 240.0f);

    kj_world_view_init_display(&v, KJ_FIELD_SCALE_1X);
    assert(v.surface_w == 400 && v.surface_h == 240);
    kj_world_view_map(&v, 128.0f, 96.0f, &sx, &sy);
    expect_close(sx, 200.0f);
    expect_close(sy, 120.0f);
    kj_world_view_map(&v, 0.0f, 0.0f, &sx, &sy);
    expect_close(sx, 72.0f); /* Task 5: x -72..328, y -24..216 */
    expect_close(sy, 24.0f);
    kj_world_view_map(&v, 256.0f, 192.0f, &sx, &sy);
    expect_close(sx, 328.0f);
    expect_close(sy, 216.0f);

    kj_world_view_init_display(&v, KJ_FIELD_SCALE_2X);
    assert(v.surface_w == 200 && v.surface_h == 120);
    assert(v.backing_w == 256 && v.backing_h == 128);
    kj_world_view_map(&v, 128.0f, 96.0f, &sx, &sy);
    expect_close(sx, 100.0f);
    expect_close(sy, 60.0f);
    /* Task 6: the active 200x120 region shows DS x 28..228, y 36..156. */
    kj_world_view_map(&v, 28.0f, 36.0f, &sx, &sy);
    expect_close(sx, 0.0f);
    expect_close(sy, 0.0f);
    kj_world_view_map(&v, 228.0f, 156.0f, &sx, &sy);
    expect_close(sx, 200.0f);
    expect_close(sy, 120.0f);

    /* Inverse mapping round-trips for a non-zero offset. */
    kj_world_view_map(&v, 41.25f, 77.5f, &sx, &sy);
    kj_world_view_unmap(&v, sx, sy, &dx, &dy);
    expect_close(dx, 41.25f);
    expect_close(dy, 77.5f);
}

static void test_visible_and_clip(void)
{
    kj_world_view v;
    float x0, y0, x1, y1, hx, hy;

    kj_world_view_init_display(&v, KJ_FIELD_SCALE_LEGACY);
    kj_world_view_visible_ds(&v, &x0, &y0, &x1, &y1);
    expect_close(x0, -32.0f);
    expect_close(y0, 0.0f);
    expect_close(x1, 288.0f);
    expect_close(y1, 192.0f);
    kj_world_view_clip_extents(&v, &hx, &hy);
    expect_close(hx, 1.25f); /* Task 7 A */
    expect_close(hy, 1.0f);

    kj_world_view_init_display(&v, KJ_FIELD_SCALE_1X);
    kj_world_view_visible_ds(&v, &x0, &y0, &x1, &y1);
    expect_close(x0, -72.0f);
    expect_close(y0, -24.0f);
    expect_close(x1, 328.0f);
    expect_close(y1, 216.0f);
    kj_world_view_clip_extents(&v, &hx, &hy);
    expect_close(hx, 1.5625f);
    expect_close(hy, 1.25f);

    kj_world_view_init_display(&v, KJ_FIELD_SCALE_2X);
    kj_world_view_visible_ds(&v, &x0, &y0, &x1, &y1);
    expect_close(x0, 28.0f);
    expect_close(y0, 36.0f);
    expect_close(x1, 228.0f);
    expect_close(y1, 156.0f);
    kj_world_view_clip_extents(&v, &hx, &hy);
    expect_close(hx, 0.78125f);
    expect_close(hy, 0.625f);
}

static void test_scissor(void)
{
    kj_world_view v;
    int r[4];

    /* Full DS viewport, legacy: [40,0) .. [360,240). */
    kj_world_view_init_display(&v, KJ_FIELD_SCALE_LEGACY);
    assert(kj_world_view_scissor(&v, 0, 0, 255, 191, r));
    assert(r[0] == 40 && r[1] == 0 && r[2] == 360 && r[3] == 240);

    /* Negative coordinates: 1x exposes DS x -24..216; a rect starting at
     * DS x -100 must be clipped to surface x 0, not wrapped to unsigned. */
    kj_world_view_init_display(&v, KJ_FIELD_SCALE_1X);
    assert(kj_world_view_scissor(&v, -100, -100, 10, 10, r));
    assert(r[0] == 0 && r[1] == 0 && r[2] == 83 && r[3] == 35);

    /* A rect entirely offscreen must be rejected. */
    assert(!kj_world_view_scissor(&v, -300, 0, -100, 100, r));
    assert(!kj_world_view_scissor(&v, 400, 0, 500, 100, r));

    /* The canonical 2x visible DS rect maps to the whole active surface. */
    kj_world_view_init_display(&v, KJ_FIELD_SCALE_2X);
    assert(kj_world_view_scissor(&v, 28, 36, 227, 155, r));
    assert(r[0] == 0 && r[1] == 0 && r[2] == 200 && r[3] == 120);

    /* Non-fullscreen (scripted) viewport inside the 2x surface. */
    assert(kj_world_view_scissor(&v, 48, 46, 127, 85, r));
    assert(r[0] == 20 && r[1] == 10 && r[2] == 100 && r[3] == 50);

    /* Inclusive endpoints: one DS pixel is exactly one surface pixel at 2x. */
    assert(kj_world_view_scissor(&v, 48, 46, 48, 46, r));
    assert(r[0] == 20 && r[1] == 10 && r[2] == 21 && r[3] == 11);
}

static void test_rotation(void)
{
    const int full[4] = { 0, 0, 400, 240 };
    const int part[4] = { 40, 24, 360, 216 };
    int r[4];

    /* Portrait render target: physical (x,y) -> fb (240-y, 400-x). */
    kj_world_view_rotate_scissor(400, 240, full, r);
    assert(r[0] == 0 && r[1] == 0 && r[2] == 240 && r[3] == 400);
    kj_world_view_rotate_scissor(400, 240, part, r);
    assert(r[0] == 24 && r[1] == 40 && r[2] == 216 && r[3] == 360);

    /* The 2x active region inside its padded 256x128 backing texture. */
    const int active[4] = { 0, 0, 200, 120 };
    kj_world_view_rotate_scissor(200, 120, active, r);
    assert(r[0] == 0 && r[1] == 0 && r[2] == 120 && r[3] == 200);
}

static void test_keys(void)
{
    kj_world_view a, b;
    kj_world_view_init_display(&a, KJ_FIELD_SCALE_LEGACY);
    kj_world_view_init_display(&b, KJ_FIELD_SCALE_LEGACY);
    assert(kj_world_view_equal(&a, &b));
    assert(kj_world_view_key(&a) == kj_world_view_key(&b));
    kj_world_view_init_display(&b, KJ_FIELD_SCALE_1X);
    assert(!kj_world_view_equal(&a, &b));
    assert(kj_world_view_key(&a) != kj_world_view_key(&b));
    /* A changed effective view (scripted framing) must change the key. */
    kj_world_view_init_display(&b, KJ_FIELD_SCALE_LEGACY);
    b.offset_y += 1.0f;
    assert(kj_world_view_key(&a) != kj_world_view_key(&b));
}

int main(void)
{
    test_canonical();
    test_visible_and_clip();
    test_scissor();
    test_rotation();
    test_keys();
    puts("world_view_test: PASS");
    return 0;
}
