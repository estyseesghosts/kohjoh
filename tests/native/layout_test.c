/* Wide-layout segments (issue #16): each row kind alone, then the shipped
 * summary/party/trainer-card tables, against a column-by-column reference
 * of Unown's documented semantics (ss3ds_widelayouts.h header comment).
 * Build: cc -I src -I src/runtime tests/native/layout_test.c
 *        src/graphics/layout/layout.c src/runtime/kj_layouts.c */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "graphics/layout/layout.h"
#include "runtime/kj_layouts.h"

#define NONE (-9999)

/* Expand segments to the source screen column of each wide column. */
static void expand(const kj_layout_seg *seg, int n, int out[320])
{
    for (int i = 0; i < 320; ++i) {
        out[i] = NONE;
    }
    for (int k = 0; k < n; ++k) {
        assert(seg[k].s0 < seg[k].s1);
        for (int s = seg[k].s0; s < seg[k].s1; ++s) {
            assert(out[s + 32] == NONE); /* segments never overlap */
            out[s + 32] = seg[k].stretch ? seg[k].sx : seg[k].sx + (s - seg[k].s0);
        }
    }
}

static int plan(const kj_layout_row *rows, uint32_t n, kj_layout_bg *bg, int y, int out[320])
{
    kj_layout_seg seg[KJ_LAYOUT_MAX_SEGS];
    bool unsupported;
    const uint32_t act = kj_layout_active(rows, n, bg, y);
    const int c = kj_layout_segments(rows, n, act, bg, seg, &unsupported);
    assert(!unsupported);
    expand(seg, c, out);
    return c;
}

static void test_none(void)
{
    kj_layout_bg bg = { 0, 0, 0, 256, 256 };
    int o[320];
    plan(NULL, 0, &bg, 10, o);
    for (int s = -32; s < 288; ++s) {
        assert(o[s + 32] == ((s >= 0 && s < 256) ? s : NONE));
    }
}

static void test_move(void)
{
    const kj_layout_row r[] = { { KJ_LAYOUT_MOVE, 1, 0, 192, 0, 128, -32, 0 } };
    kj_layout_bg bg = { 0, 0, 0, 256, 256 };
    int o[320];
    plan(r, 1, &bg, 0, o);
    for (int s = -32; s < 96; ++s) {
        assert(o[s + 32] == s + 32);
    }
    for (int s = 96; s < 128; ++s) {
        assert(o[s + 32] == NONE); /* vacated */
    }
    for (int s = 128; s < 256; ++s) {
        assert(o[s + 32] == s);
    }
    /* another BG and lines outside the row are untouched */
    bg.bg = 1;
    plan(r, 1, &bg, 0, o);
    assert(o[0] == NONE && o[32 + 100] == 100);
}

static void test_fill_copy(void)
{
    const kj_layout_row r[] = { { KJ_LAYOUT_FILL, 1, 0, 192, -32, 0, 0, 0 },
                                { KJ_LAYOUT_COPY, 1, 0, 192, 256, 288, 0, 100 } };
    kj_layout_bg bg = { 0, 0, 0, 256, 256 };
    int o[320];
    const int c = plan(r, 2, &bg, 5, o);
    assert(c == 3);
    for (int s = -32; s < 0; ++s) {
        assert(o[s + 32] == 0);
    }
    for (int s = 256; s < 288; ++s) {
        assert(o[s + 32] == 100 + (s - 256));
    }
}

static void test_scroll_picture_coords(void)
{
    /* Picture columns: with hofs 16, picture column 20 is on screen at 4. */
    const kj_layout_row r[] = { { KJ_LAYOUT_MOVE, 1, 0, 192, 20, 40, 10, 0 } };
    kj_layout_bg bg = { 0, 16, 0, 256, 256 };
    int o[320];
    plan(r, 1, &bg, 0, o);
    /* source screen column of picture column c is c - 16 */
    for (int c = 20; c < 40; ++c) {
        assert(o[c - 16 + 10 + 32] == c - 16);
    }
    /* lines in picture coordinates: vofs moves the band */
    const kj_layout_row band[] = { { KJ_LAYOUT_FILL, 1, 100, 110, -32, 0, 0, 7 } };
    kj_layout_bg b2 = { 0, 0, 50, 256, 256 };
    assert(kj_layout_active(band, 1, &b2, 49) == 0);
    assert(kj_layout_active(band, 1, &b2, 50) == 1);
    assert(kj_layout_active(band, 1, &b2, 59) == 1);
    assert(kj_layout_active(band, 1, &b2, 60) == 0);
}

static void test_screen_fill_scrolled(void)
{
    /* #19: a SCREEN FILL repeats the screen's edge column whatever the
     * scroll (the battle intro slides BG3). */
    const kj_layout_row r[] = { { KJ_LAYOUT_FILL, 1 | KJ_LAYOUT_SCREEN, 0, 192, -32, 0, 0, 0 },
                                { KJ_LAYOUT_FILL, 1 | KJ_LAYOUT_SCREEN, 0, 192, 256, 288, 0, 255 } };
    for (int h = 0; h < 512; h += 37) {
        kj_layout_bg bg = { 0, h, 0, 512, 256 };
        int o[320];
        plan(r, 2, &bg, 0, o);
        for (int s = -32; s < 0; ++s) {
            assert(o[s + 32] == 0);
        }
        for (int s = 256; s < 288; ++s) {
            assert(o[s + 32] == 255);
        }
    }
}

static void test_wrap(void)
{
    const kj_layout_row r[] = { { KJ_LAYOUT_WRAP, 1 | KJ_LAYOUT_SCREEN, 0, 192, 0, 0, 0, 0 } };
    kj_layout_bg bg = { 0, 37, 0, 256, 256 };
    int o[320];
    const int c = plan(r, 1, &bg, 0, o);
    assert(c == 1); /* one continuous strip across the whole wide screen */
    for (int s = -32; s < 288; ++s) {
        /* same picture column as the DS map's wrap: source == s mod 256 */
        assert(((o[s + 32] - s) & 255) == 0);
    }
}

static void test_tables(void)
{
    kj_screen_state st;
    memset(&st, 0, sizeof(st));
    kj_layouts_for(KJ_SCREEN_PARTY, &st);
    assert(st.rows[0] == NULL && st.row_count[1] == 6);
    kj_layout_bg bg = { 1, 0, 0, 256, 256 };
    int o[320];
    plan(st.rows[1], st.row_count[1], &bg, 50, o);
    /* frame left edge to the screen edge, then column 21 stretched, then
     * the rest 32 px right: every wide column covered, no holes */
    for (int s = -32; s < 288; ++s) {
        assert(o[s + 32] != NONE);
    }
    assert(o[0] == 0 && o[32 + 287] == 255);

    kj_layouts_for(KJ_SCREEN_SUMMARY, &st);
    assert(kj_layout_shift3d(st.rows[0], st.row_count[0]) == 32);
    assert(kj_layout_obj_dx(st.rows[0], st.row_count[0], 10, 10) == -32);
    assert(kj_layout_obj_dx(st.rows[0], st.row_count[0], 200, 10) == 32);
    kj_layout_bg b3 = { 3, 0, 0, 256, 256 };
    plan(st.rows[0], st.row_count[0], &b3, 100, o);
    for (int s = -32; s < 288; ++s) {
        assert(o[s + 32] != NONE);
    }
    for (int s = 104; s < 168; ++s) {
        assert(o[s + 32] == 127 + ((s - 104) & 7)); /* interior repeated */
    }

    kj_layouts_for(KJ_SCREEN_FIELD, &st);
    assert(st.row_count[0] == 0 && st.row_count[1] == 0);
}

int main(void)
{
    test_none();
    test_move();
    test_fill_copy();
    test_scroll_picture_coords();
    test_screen_fill_scrolled();
    test_wrap();
    test_tables();
    puts("layout_test: ok");
    return 0;
}
