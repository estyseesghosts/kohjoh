/* Native tests for the runtime font atlas, quad batch and window policy.
 *
 * Build (repository root):
 *   cc -std=gnu11 -I src \
 *      tests/native/text_test.c \
 *      src/graphics/font/glyph_atlas.c \
 *      src/graphics/font/glyph_batch.c \
 *      src/graphics/font/text_window.c \
 *      -o text_test
 *   ./text_test assets/fonts/generated/kohjoh_text_atlas.bin
 *
 * text_window.c's only external dependency is the log sink, stubbed below.
 * Nothing here links libctru: the atlas format and the policy are pure C. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graphics/font/glyph_atlas.h"
#include "graphics/font/glyph_batch.h"
#include "graphics/font/text_window.h"

void kj_log(const char *fmt, ...)
{
    (void)fmt;
}

static unsigned char *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *data;
    long n;

    assert(f != NULL);
    assert(fseek(f, 0, SEEK_END) == 0);
    n = ftell(f);
    assert(n > 0);
    assert(fseek(f, 0, SEEK_SET) == 0);
    data = (unsigned char *)malloc((size_t)n);
    assert(data != NULL);
    assert(fread(data, 1, (size_t)n, f) == (size_t)n);
    fclose(f);
    *size = (size_t)n;
    return data;
}

static kj_text_event ev(unsigned kind, uint32_t window, uint32_t generation)
{
    kj_text_event e;
    memset(&e, 0, sizeof(e));
    e.kind = (uint8_t)kind;
    e.window = window;
    e.generation = generation;
    e.engine = 0;
    e.bg_id = 0;
    e.palette_num = 0;
    e.color_mode = 0;
    e.width = 30;
    e.height = 4;
    return e;
}

static void add_full_window(kj_text_event *out, unsigned *n, uint32_t window)
{
    kj_text_event a = ev(KJ_TEXT_EV_ADD, window, 1);
    kj_text_event f = ev(KJ_TEXT_EV_FILL, window, 1);
    f.x = 0;
    f.y = 0;
    f.value = 15;
    f.arg0 = 240;
    f.arg1 = 32;
    out[(*n)++] = a;
    out[(*n)++] = f;
}

static void glyph_gen(kj_text_event *out, unsigned *n, uint32_t window, uint32_t generation,
                      uint16_t code, int x, int y)
{
    kj_text_event g = ev(KJ_TEXT_EV_GLYPH, window, generation);
    g.code = code;
    g.x = (int16_t)x;
    g.y = (int16_t)y;
    g.fg = 1;
    g.shadow = 2;
    g.bg = 15;
    out[(*n)++] = g;
}

static void glyph(kj_text_event *out, unsigned *n, uint32_t window, uint16_t code, int x, int y)
{
    glyph_gen(out, n, window, 1, code, x, y);
}

static void publish_gen(kj_text_event *out, unsigned *n, uint32_t window, uint32_t generation)
{
    out[(*n)++] = ev(KJ_TEXT_EV_PUBLISH, window, generation);
}

static void publish(kj_text_event *out, unsigned *n, uint32_t window)
{
    publish_gen(out, n, window, 1);
}

static void test_atlas(const unsigned char *blob, size_t size)
{
    kj_glyph g;
    kj_glyph_art art;

    assert(kj_font_atlas_init(blob, size));
    assert(kj_font_atlas_ready());
    assert(kj_font_atlas_get()->page_w == 256 && kj_font_atlas_get()->page_h == 256);
    assert(kj_font_atlas_get()->page_count == 1);

    /* 'A' is HGSS code 299, Unicode 65, in every face. */
    assert(kj_font_lookup(0, 299, KJ_GLYPH_VARIANT_STANDARD, &g));
    assert(g.unicode == 65 && g.advance == 11 && g.font_id == 0);
    assert(g.bearing_x == 1 && g.bearing_y == 1);
    assert(kj_font_lookup(4, 299, KJ_GLYPH_VARIANT_STANDARD, &g));
    assert(g.advance == 11);
    assert(kj_font_lookup(5, 299, KJ_GLYPH_VARIANT_STANDARD, &g));
    assert(g.advance == 10); /* fixed face */

    /* Numeric variant exists for digits in font 0 and the pseudo-font. */
    assert(kj_font_lookup(0, 289, KJ_GLYPH_VARIANT_NUMERIC, &g));
    assert(g.variant == KJ_GLYPH_VARIANT_NUMERIC && g.advance == 10);
    assert(kj_font_lookup(KJ_FONT_NUMERIC, 289, KJ_GLYPH_VARIANT_NUMERIC, &g));
    assert(g.font_id == KJ_FONT_NUMERIC && g.unicode == 48);
    /* A missing numeric record falls back to the standard one. */
    assert(kj_font_lookup(2, 289, KJ_GLYPH_VARIANT_NUMERIC, &g));
    assert(g.variant == KJ_GLYPH_VARIANT_STANDARD);

    /* Exact lookup of a code that is not in the repertoire fails. */
    assert(!kj_font_lookup(0, 0x1234, KJ_GLYPH_VARIANT_STANDARD, &g));
    assert(!kj_font_lookup(7, 299, KJ_GLYPH_VARIANT_STANDARD, &g));

    assert(kj_font_lookup(0, 299, KJ_GLYPH_VARIANT_STANDARD, &g));
    assert(kj_font_art(g.art_index, &art));
    assert(art.w != 0 && art.h != 0 && art.page == 0);
    assert(!kj_font_art((uint16_t)(kj_font_atlas_get()->art_count), &art));
    assert(kj_font_page(0) != NULL && kj_font_page(1) == NULL);

    /* Truncated or corrupted blobs are rejected, never read out of bounds. */
    {
        unsigned char *bad = (unsigned char *)malloc(size);
        assert(bad != NULL);
        memcpy(bad, blob, size);
        assert(!kj_font_atlas_init(bad, 16));
        bad[0] = 'X';
        assert(!kj_font_atlas_init(bad, size));
        memcpy(bad, blob, size);
        assert(!kj_font_atlas_init(bad, size - 1));
        memcpy(bad, blob, size);
        bad[8] = 0; /* page_count */
        assert(!kj_font_atlas_init(bad, size));
        free(bad);
    }

    kj_font_atlas_exit();
    assert(!kj_font_atlas_ready());
    assert(!kj_font_lookup(0, 299, KJ_GLYPH_VARIANT_STANDARD, &g));
}

static void test_batch(const unsigned char *blob, size_t size)
{
    kj_text_vertex mem[15];
    kj_text_run run;
    kj_glyph g;
    kj_glyph_art art;
    kj_text_vertex q[4];
    uint32_t rgba = kj_rgba(0x11, 0x22, 0x33, 0x44);

    assert(rgba == 0x11223344u);
    assert(kj_font_atlas_init(blob, size));
    assert(kj_font_lookup(0, 299, KJ_GLYPH_VARIANT_STANDARD, &g));
    assert(kj_font_art(g.art_index, &art));

    kj_text_run_init(&run, mem, 2);
    assert(!kj_text_run_push(&run, q)); /* too small for one quad */

    kj_text_run_init(&run, mem, 12);
    assert(kj_batch_glyph(&run, kj_font_atlas_get(), &g, &art, 100, 40, rgba));
    assert(run.count == 4);
    assert(run.vertices[0].x == 100 && run.vertices[0].y == 40);
    assert(run.vertices[1].x == 100 + art.w && run.vertices[1].y == 40);
    assert(run.vertices[2].x == 100 && run.vertices[2].y == 40 + art.h);
    assert(run.vertices[0].u == art.x * KJ_TEXT_UV_UNIT);
    assert(run.vertices[0].v == art.y * KJ_TEXT_UV_UNIT);
    assert(run.vertices[3].u == art.x * KJ_TEXT_UV_UNIT + art.w * KJ_TEXT_UV_UNIT - 1);
    assert(run.vertices[3].v == art.y * KJ_TEXT_UV_UNIT + art.h * KJ_TEXT_UV_UNIT - 1);
    assert(run.vertices[0].r == 0x11 && run.vertices[0].g == 0x22);
    assert(run.vertices[0].b == 0x33 && run.vertices[0].a == 0x44);

    /* Fills sample the 8x8 solid white tile. */
    assert(kj_batch_fill(&run, 0, 0, 240, 32, rgba, 7, 9));
    assert(run.count == 8);
    assert(run.vertices[4].u == 7 * KJ_TEXT_UV_UNIT);
    assert(run.vertices[5].u == 7 * KJ_TEXT_UV_UNIT + 8 * KJ_TEXT_UV_UNIT - 1);
    assert(run.vertices[4].v == 9 * KJ_TEXT_UV_UNIT);
    assert(run.vertices[6].v == 9 * KJ_TEXT_UV_UNIT + 8 * KJ_TEXT_UV_UNIT - 1);
    assert(run.vertices[5].x == 240 && run.vertices[7].y == 32);

    /* Capacity is respected exactly. */
    assert(kj_batch_glyph(&run, kj_font_atlas_get(), &g, &art, 0, 0, rgba));
    assert(!kj_batch_glyph(&run, kj_font_atlas_get(), &g, &art, 0, 0, rgba));
    assert(run.count == 12);
    kj_font_atlas_exit();
}

static void test_policy(const unsigned char *blob, size_t size)
{
    kj_text_event events[16];
    unsigned n;
    kj_frame_input in;
    const kj_text_presentation *p;
    const kj_text_presentation *found;
    int cursor;
    const kj_text_stats *stats;

    assert(kj_font_atlas_init(blob, size));
    kj_text_init();
    kj_text_stats_reset();

    /* Disabled by default: every window is on the legacy path. */
    memset(&in, 0, sizeof(in));
    n = 0;
    add_full_window(events, &n, 0x1000);
    glyph(events, &n, 0x1000, 299, 0, 0);
    publish(events, &n, 0x1000);
    in.text_events = events;
    in.text_event_count = n;
    kj_text_frame(&in);
    cursor = 0;
    assert(kj_text_next(0, 0, &cursor) == NULL);
    stats = kj_text_stats_read();
    assert(stats->windows_fallback == 1);
    assert(stats->fallback_reason[KJ_TEXT_FALLBACK_DISABLED] == 1);

    /* Enabled: a complete BG window presents natively. */
    kj_text_set_enabled(true);
    assert(kj_text_enabled());
    n = 0;
    add_full_window(events, &n, 0x2000);
    glyph(events, &n, 0x2000, 299, 0, 0);
    glyph(events, &n, 0x2000, 325, 11, 0);
    publish(events, &n, 0x2000);
    in.text_event_count = n;
    kj_text_frame(&in);
    cursor = 0;
    p = kj_text_next(0, 0, &cursor);
    assert(p != NULL && p->native && p->window == 0x2000);
    assert(p->glyph_count == 2 && p->fill == 15 && p->have_fill);
    assert(p->glyphs[1].code == 325 && p->glyphs[1].x == 11);
    assert(kj_text_next(0, 0, &cursor) == NULL);

    /* The compositor must skip exactly the tiles the native pass replaces. */
    assert(kj_text_owns_tile(0, 0, 0, 0, 0, 0));
    assert(kj_text_owns_tile(0, 0, 0, 0, 232, 24));
    assert(!kj_text_owns_tile(0, 0, 0, 0, 240, 0));   /* width 30 tiles = 240 */
    assert(!kj_text_owns_tile(0, 0, 0, 0, 0, 32));    /* height 4 tiles = 32 */
    assert(!kj_text_owns_tile(0, 1, 0, 0, 0, 0));     /* other plane */
    assert(!kj_text_owns_tile(1, 0, 0, 0, 0, 0));     /* other engine */

    n = 0;
    add_full_window(events, &n, 0x3000);
    glyph(events, &n, 0x3000, 0x1234, 0, 0);
    publish(events, &n, 0x3000);
    in.text_event_count = n;
    kj_text_frame(&in);
    cursor = 0;
    assert(kj_text_next(0, 0, &cursor)->window == 0x2000); /* 0x3000 absent */
    assert(kj_text_next(0, 0, &cursor) == NULL);
    assert(kj_text_stats_read()->fallback_reason[KJ_TEXT_FALLBACK_UNSUPPORTED_GLYPH] == 1);

    /* Reusing the address with a new generation must not inherit glyphs. */
    n = 0;
    events[n] = ev(KJ_TEXT_EV_ADD, 0x2000, 2);
    n++;
    events[n] = ev(KJ_TEXT_EV_FILL, 0x2000, 2);
    events[n].value = 15;
    events[n].arg0 = 240;
    events[n].arg1 = 32;
    n++;
    glyph_gen(events, &n, 0x2000, 2, 299, 0, 0);
    publish_gen(events, &n, 0x2000, 2);
    in.text_event_count = n;
    kj_text_frame(&in);
    cursor = 0;
    p = kj_text_next(0, 0, &cursor);
    assert(p != NULL && p->generation == 2 && p->glyph_count == 1);

    /* A temporary OBJ source is never native. */
    n = 0;
    events[n] = ev(KJ_TEXT_EV_ADD, 0x4000, 1);
    events[n].flags = KJ_TEXT_WINDOW_TEMPORARY;
    n++;
    glyph(events, &n, 0x4000, 299, 0, 0);
    publish(events, &n, 0x4000);
    in.text_event_count = n;
    kj_text_frame(&in);
    assert(kj_text_stats_read()->fallback_reason[KJ_TEXT_FALLBACK_TEMPORARY_WINDOW] == 1);

    /* Only BG0..BG3 of either engine are addressable. */
    n = 0;
    events[n] = ev(KJ_TEXT_EV_ADD, 0x5000, 1);
    events[n].bg_id = 8;
    events[n].engine = 0;
    n++;
    events[n] = ev(KJ_TEXT_EV_FILL, 0x5000, 1);
    events[n].value = 1;
    events[n].arg0 = 240;
    events[n].arg1 = 32;
    n++;
    glyph(events, &n, 0x5000, 299, 0, 0);
    events[n] = ev(KJ_TEXT_EV_PUBLISH, 0x5000, 1);
    events[n].bg_id = 8;
    events[n].engine = 0;
    n++;
    in.text_event_count = n;
    kj_text_frame(&in);
    assert(kj_text_stats_read()->fallback_reason[KJ_TEXT_FALLBACK_UNSUPPORTED_LAYER] == 1);

    /* Scrolling shifts the records with the pixels; the window stays native. */
    n = 0;
    add_full_window(events, &n, 0x6000);
    glyph(events, &n, 0x6000, 299, 8, 16);
    events[n] = ev(KJ_TEXT_EV_SCROLL, 0x6000, 1);
    events[n].y = -16;
    n++;
    publish(events, &n, 0x6000);
    in.text_event_count = n;
    kj_text_frame(&in);
    cursor = 0;
    found = NULL;
    while ((p = kj_text_next(0, 0, &cursor)) != NULL) {
        if (p->window == 0x6000) {
            found = p;
        }
    }
    assert(found != NULL && found->native);
    assert(found->glyph_count == 1 && found->glyphs[0].y == 0);

    /* Removing a window retires its record: the DS pixels take over. */
    n = 0;
    events[n] = ev(KJ_TEXT_EV_REMOVE, 0x2000, 1);
    n++;
    in.text_event_count = n;
    kj_text_frame(&in);
    cursor = 0;
    while ((p = kj_text_next(0, 0, &cursor)) != NULL) {
        assert(p->window != 0x2000);
    }

    /* A window filled with index 0 (transparent) is still presentable when it
     * holds only text: skip its legacy tiles, paint no backing. */
    n = 0;
    events[n++] = ev(KJ_TEXT_EV_ADD, 0x7000, 1);
    {
        kj_text_event f = ev(KJ_TEXT_EV_FILL, 0x7000, 1);
        f.value = 0;
        f.arg0 = 240;
        f.arg1 = 32;
        events[n++] = f;
    }
    glyph(events, &n, 0x7000, 299, 0, 0);
    publish(events, &n, 0x7000);
    in.text_event_count = n;
    kj_text_frame(&in);
    cursor = 0;
    found = NULL;
    while ((p = kj_text_next(0, 0, &cursor)) != NULL) {
        if (p->window == 0x7000) {
            found = p;
        }
    }
    assert(found != NULL && found->native && found->fill == 0);
    assert(kj_text_owns_tile(0, 0, 0, 0, 0, 0));

    /* Art blitted into the buffer means the fill is not the backing. */
    n = 0;
    events[n++] = ev(KJ_TEXT_EV_ADD, 0x8000, 1);
    {
        kj_text_event f = ev(KJ_TEXT_EV_FILL, 0x8000, 1);
        f.value = 0;
        f.arg0 = 240;
        f.arg1 = 32;
        events[n++] = f;
    }
    glyph(events, &n, 0x8000, 299, 0, 0);
    events[n++] = ev(KJ_TEXT_EV_ART, 0x8000, 1);
    publish(events, &n, 0x8000);
    in.text_event_count = n;
    kj_text_frame(&in);
    cursor = 0;
    while ((p = kj_text_next(0, 0, &cursor)) != NULL) {
        assert(p->window != 0x8000);
    }

    /* A blank filler glyph marks a window whose tiles carry artwork (the
     * map-name banner), so it must stay on the DS pixels too. */
    n = 0;
    events[n++] = ev(KJ_TEXT_EV_ADD, 0x9000, 1);
    {
        kj_text_event f = ev(KJ_TEXT_EV_FILL, 0x9000, 1);
        f.value = 0;
        f.arg0 = 240;
        f.arg1 = 32;
        events[n++] = f;
    }
    glyph(events, &n, 0x9000, 299, 0, 0);
    glyph(events, &n, 0x9000, 0x0000, 8, 0);
    publish(events, &n, 0x9000);
    in.text_event_count = n;
    kj_text_frame(&in);
    cursor = 0;
    while ((p = kj_text_next(0, 0, &cursor)) != NULL) {
        assert(p->window != 0x9000);
    }

    /* DS colours: 5-bit to 8-bit with an opaque alpha; index 0 is transparent. */
    {
        uint16_t pal[256];
        memset(pal, 0, sizeof(pal));
        pal[1] = 0x001F;        /* red */
        pal[0x10 + 1] = 0x03E0; /* green in bank 1 */
        assert(kj_text_ds_color(pal, 0, 0, 1) == 0xFF0000FFu);
        assert(kj_text_ds_color(pal, 0, 1, 1) == 0x00FF00FFu);
        assert(kj_text_ds_color(pal, 0, 0, 0) == 0);
        assert(kj_text_ds_color(NULL, 0, 0, 0) == 0);
        assert(kj_text_ds_color(NULL, 0, 0, 1) == 0xFFFFFFFFu);
    }

    kj_text_exit();
    kj_font_atlas_exit();
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "assets/fonts/generated/kohjoh_text_atlas.bin";
    size_t size = 0;
    unsigned char *blob = read_file(path, &size);

    test_atlas(blob, size);
    test_batch(blob, size);
    test_policy(blob, size);

    free(blob);
    puts("text_test: PASS");
    return 0;
}
