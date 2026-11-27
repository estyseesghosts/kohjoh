/*
 * fontbench.c: development-only specimen overlay (docs/text-renderer.txt
 * Task 4C). Enabled by a `fontbench` line in sdmc:/3ds/kohjoh/script.txt.
 *
 * It draws the embedded atlas at 1:1 physical pixels over the finished top
 * frame, so the specimen strings are exactly the glyphs the game uses. This
 * is the preview the maintainer inspects through Lotus; it is never enabled
 * by a player configuration and costs nothing when absent.
 */

#include "fontbench.h"

#include "glyph_atlas.h"
#include "glyph_batch.h"
#include "text_render.h"
#include "text_window.h"
#include "../g2d/g2d.h"
#include "../pica/gpu.h"
#include "diagnostics/log.h"

#include <citro3d.h>
#include <3ds.h>

/* ASCII 0x20..0x7E -> HGSS character code, generated from charmap.json.
 * 0 means "no HGSS code"; those characters are skipped in specimens. */
static const uint16_t s_ascii_code[0x5F] = {
    0x01DE, 0x01AB, 0x0000, 0x01C0, 0x01A8, 0x01D2, 0x01C2, 0x0000,
    0x01B9, 0x01BA, 0x01BF, 0x01BD, 0x01AD, 0x01BE, 0x01AE, 0x01B1,
    0x0121, 0x0122, 0x0123, 0x0124, 0x0125, 0x0126, 0x0127, 0x0128,
    0x0129, 0x012A, 0x01C4, 0x01C5, 0x0000, 0x01C1, 0x0000, 0x01AC,
    0x01D0, 0x012B, 0x012C, 0x012D, 0x012E, 0x012F, 0x0130, 0x0131,
    0x0132, 0x0133, 0x0134, 0x0135, 0x0136, 0x0137, 0x0138, 0x0139,
    0x013A, 0x013B, 0x013C, 0x013D, 0x013E, 0x013F, 0x0140, 0x0141,
    0x0142, 0x0143, 0x0144, 0x0000, 0x0000, 0x0000, 0x0000, 0x01E9,
    0x0000, 0x0145, 0x0146, 0x0147, 0x0148, 0x0149, 0x014A, 0x014B,
    0x014C, 0x014D, 0x014E, 0x014F, 0x0150, 0x0151, 0x0152, 0x0153,
    0x0154, 0x0155, 0x0156, 0x0157, 0x0158, 0x0159, 0x015A, 0x015B,
    0x015C, 0x015D, 0x015E, 0x0000, 0x0000, 0x0000, 0x01C3,
};

static const char *const s_lines[] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "abcdefghijklmnopqrstuvwxyz",
    "0123456789",
    "Ill1 / O0 / rn / m / vv / w",
    "Pok\x82mon", /* e-acute is code 0x0188 in HGSS; mapped below */
    "FIGHT  BAG  POK\x82MON  RUN",
    "Pikachu used Thunderbolt!",
    "Typhlosion   Lv. 100",
    "HP  299/299",
    "PP  15/15",
    "Pok\x82 Ball \x81""999",
    "MAX ELIXIR       Yes    No",
    "Saving... Don't turn off the power.",
};

#define FB_LINES ((int)(sizeof(s_lines) / sizeof(s_lines[0])))
#define FB_VERTICES 4096

/* PICA reads vertex arrays from physically contiguous memory. */
static kj_text_vertex *s_vertices;

/* 0x82 and 0x81 are placeholders: accents and the multiplication sign are
 * not ASCII, so the specimens carry them as explicit HGSS codes. */
static uint16_t code_for(unsigned char c)
{
    if (c == 0x82) {
        return 0x0188; /* é */
    }
    if (c == 0x81) {
        return 0x0176; /* × */
    }
    if (c == '\'') {
        return 0x01B3; /* right single quotation mark, HGSS apostrophe */
    }
    if (c == '"') {
        return 0x01B4; /* left double quotation mark */
    }
    if (c < 0x20 || c > 0x7E) {
        return 0;
    }
    return s_ascii_code[c - 0x20];
}

static int draw_line(kj_text_run *run, const char *text, int x, int y, uint32_t fg, uint32_t sh)
{
    int pen = x;
    int worst_bottom = y;

    for (const unsigned char *p = (const unsigned char *)text; *p != '\0'; ++p) {
        const uint16_t code = code_for(*p);
        kj_glyph g;
        kj_glyph_art art;

        if (code == 0 || !kj_font_lookup(0, code, KJ_GLYPH_VARIANT_STANDARD, &g) ||
            !kj_font_art(g.art_index, &art)) {
            pen += 6; /* keep the row readable even if a code is missing */
            continue;
        }
        /* Shadow first, offset one physical pixel, then the face. */
        if (!kj_batch_glyph(run, kj_font_atlas_get(), &g, &art, pen + g.bearing_x + 1,
                            y + g.bearing_y + 1, sh)) {
            return worst_bottom; /* capacity exceeded: stop this line */
        }
        if (!kj_batch_glyph(run, kj_font_atlas_get(), &g, &art, pen + g.bearing_x,
                            y + g.bearing_y, fg)) {
            return worst_bottom;
        }
        if (y + g.bearing_y + art.h > worst_bottom) {
            worst_bottom = y + g.bearing_y + art.h;
        }
        pen += g.advance;
    }
    return worst_bottom;
}

void kj_fontbench_exit(void)
{
    if (s_vertices != NULL) {
        linearFree(s_vertices);
        s_vertices = NULL;
    }
}

void kj_fontbench_draw(const kj_target *top)
{
    static bool logged;
    kj_text_run run;
    int y = 12;

    if (!logged) {
        logged = true;
        kj_log("[text] fontbench: target %p texture %p atlas %d", (const void *)top,
               (const void *)kj_text_texture(), kj_font_atlas_ready() ? 1 : 0);
    }
    if (top == NULL || kj_text_texture() == NULL || !kj_font_atlas_ready()) {
        return;
    }
    if (s_vertices == NULL) {
        s_vertices = linearAlloc(FB_VERTICES * sizeof(kj_text_vertex));
        if (s_vertices == NULL) {
            return;
        }
    }
    /* The target's pass is already open and this is its final draw. */
    C3D_CullFace(GPU_CULL_NONE);
    kj_gpu_scissor_off();

    /* A dark card so light glyphs read the same way they do in dialogue. */
    kj_text_run_init(&run, s_vertices, FB_VERTICES);
    if (!kj_batch_fill(&run, 12, 4, 296, 232, kj_rgba(12, 16, 24, 255), 1, 1)) {
        return;
    }
    GSPGPU_FlushDataCache(run.vertices, run.count * sizeof(kj_text_vertex));
    kj_g2d_text_draw(top, (const kj_v2d *)run.vertices, run.count, true);

    run.count = 0;
    for (int i = 0; i < FB_LINES; ++i) {
        /* Every other line is light-on-dark; the face/shadow pair follows the
         * original HGSS treatment (white face, dark subordinate shadow). */
        const uint32_t fg = i == 11 ? kj_rgba(255, 236, 170, 255) : kj_rgba(248, 248, 248, 255);
        const int bottom = draw_line(&run, s_lines[i], 40, y, fg, kj_rgba(48, 52, 72, 255));
        y = bottom + 2;
    }
    if (run.count != 0) {
        GSPGPU_FlushDataCache(run.vertices, run.count * sizeof(kj_text_vertex));
        kj_g2d_text_draw(top, (const kj_v2d *)run.vertices, run.count, true);
    }
}
