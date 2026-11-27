#include "text_render.h"

#include <string.h>

#include "glyph_atlas.h"
#include "glyph_batch.h"
#include "text_window.h"
#include "diagnostics/log.h"
#include "../g2d/g2d.h"
#include "../pica/gpu.h"
#include "../resources/atlas.h"

#include <3ds.h>

/* bin2s output for assets/fonts/generated/kohjoh_text_atlas.bin. */
extern const unsigned char kohjoh_text_atlas_bin[];
extern const unsigned int kohjoh_text_atlas_bin_size;

static C3D_Tex s_texture;
static bool s_texture_ready;
static bool s_logged_draw;
/* One line-anchored x per captured glyph, computed once and used by both the
 * shadow and foreground passes. */
static int s_place[KJ_TEXT_MAX_GLYPHS_PER_WINDOW];
/* GPU-visible (linearAlloc, physically contiguous) vertex storage: PICA
 * cannot read a scatter of .bss pages as a vertex array. */
static kj_text_vertex *s_run;
#define TEXT_RUN_VERTICES (KJ_TEXT_MAX_GLYPHS_PER_WINDOW * 8u)

/* The batch emits this exact layout; g2d consumes kj_v2d. Keep them equal. */
_Static_assert(sizeof(kj_text_vertex) == sizeof(kj_v2d), "text vertex layout");
_Static_assert(KJ_TEXT_UV_UNIT == KJ_ATLAS_UV_UNIT, "text UV unit");

bool kj_text_render_init(void)
{
    kj_text_render_exit();
    if (s_run == NULL) {
        s_run = linearAlloc(TEXT_RUN_VERTICES * sizeof(kj_text_vertex));
        if (s_run == NULL) {
            return false;
        }
    }
    if (!kj_font_atlas_init(kohjoh_text_atlas_bin, kohjoh_text_atlas_bin_size)) {
        return false;
    }
    const kj_font_atlas *atlas = kj_font_atlas_get();
    if (atlas == NULL || !C3D_TexInit(&s_texture, atlas->page_w, atlas->page_h, GPU_RGBA5551)) {
        kj_font_atlas_exit();
        return false;
    }
    C3D_TexSetFilter(&s_texture, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&s_texture, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    const void *page0 = kj_font_page(0);
    if (page0 == NULL) {
        C3D_TexDelete(&s_texture);
        kj_font_atlas_exit();
        return false;
    }
    /* Pages beyond the first are not bound by the first implementation; the
     * generator currently emits one 256x256 page. */
    /* C3D_TexInit allocates the texture already in PICA's 8x8 tile layout;
     * the generator wrote that exact layout, so copy it into the texture
     * memory rather than re-tiling it with C3D_TexUpload. */
    memcpy(s_texture.data, page0, (size_t)atlas->page_w * atlas->page_h * 2u);
    GSPGPU_FlushDataCache(s_texture.data, atlas->page_w * atlas->page_h * 2u);
    s_texture_ready = true;
    kj_text_init();
    kj_log("[text] atlas ready: %u glyphs, %u masks, %ux%u, %u KiB",
           (unsigned)atlas->glyph_count, (unsigned)atlas->art_count,
           (unsigned)atlas->page_w, (unsigned)atlas->page_h,
           (unsigned)((unsigned)atlas->page_w * atlas->page_h * 2u / 1024u));
    return true;
}

void kj_text_render_exit(void)
{
    if (s_texture_ready) {
        C3D_TexDelete(&s_texture);
        s_texture_ready = false;
    }
    if (s_run != NULL) {
        linearFree(s_run);
        s_run = NULL;
    }
    kj_font_atlas_exit();
    kj_text_exit();
}

C3D_Tex *kj_text_texture(void)
{
    return s_texture_ready ? &s_texture : NULL;
}

bool kj_text_texture_ready(void)
{
    return s_texture_ready;
}

static int map_x(const kj_target *t, int x)
{
    return (int)(t->offset_x + (float)x * t->scale + 0.5f);
}

static int map_y(const kj_target *t, int y)
{
    return (int)((float)y * t->scale + 0.5f);
}

void kj_text_draw_bg(const kj_target *target, int engine, int bg, const uint16_t *palette,
                     int hofs, int vofs)
{
    int cursor = 0;
    const kj_text_presentation *p;

    if (!s_texture_ready || target == NULL || palette == NULL) {
        return;
    }
    while ((p = kj_text_next(engine, bg, &cursor)) != NULL) {
        kj_text_run run;
        if (!s_logged_draw) {
            s_logged_draw = true;
            kj_log("[text] first native draw: engine %d bg %d, %u glyphs", engine, bg,
                   (unsigned)p->glyph_count);
        }
        const int base_x = (int)p->tilemap_x * 8 - hofs;
        const int base_y = (int)p->tilemap_y * 8 - vofs;
        const int fill_x = map_x(target, base_x);
        const int fill_y = map_y(target, base_y);
        const int fill_w = map_x(target, base_x + (int)p->width * 8) - fill_x;
        const int fill_h = map_y(target, base_y + (int)p->height * 8) - fill_y;
        const uint32_t fill_color =
            kj_text_ds_color(palette, p->color_mode, p->palette_num, p->fill);

        if (fill_w <= 0 || fill_h <= 0) {
            continue;
        }
        kj_text_run_init(&run, s_run, TEXT_RUN_VERTICES);
        /* Clean backing: the window's own fill colour, or nothing when the
         * fill index is 0 (transparent on every DS BG layer). The frame
         * artwork is drawn outside the window rectangle, so an opaque fill
         * does not erase it; a transparent one must not paint black over the
         * panel underneath. */
        if (fill_color != 0 &&
            kj_batch_fill(&run, fill_x, fill_y, fill_w, fill_h, fill_color, 1, 1)) {
            GSPGPU_FlushDataCache(run.vertices, run.count * sizeof(kj_text_vertex));
            kj_g2d_text_draw(target, (const kj_v2d *)run.vertices, run.count, true);
            kj_text_note_draw();
        }

        run.count = 0;
        /* Layout: one pen per line, advancing by the glyph's own cell.
         *
         * Every glyph mask is a native-size strike with its own sidebearings;
         * the DS's 8-logical-pixel cell is narrower than several capitals
         * (ink 9-13 px), so honouring the old cell positions made wide letters
         * collide with the next one. The captured x still anchors each line,
         * and the captured y still selects the line, so wrapping and vertical
         * placement are the game's. */
        {
            int line_y = -32768;
            int pen = 0;
            for (uint16_t i = 0; i < p->glyph_count && i < KJ_TEXT_MAX_GLYPHS_PER_WINDOW; ++i) {
                const kj_text_glyph *g = &p->glyphs[i];
                kj_glyph rec;
                if (!kj_font_lookup(g->font_id, g->code, KJ_GLYPH_VARIANT_STANDARD, &rec)) {
                    s_place[i] = -32768;
                    continue;
                }
                if (g->y != line_y) {
                    line_y = g->y;
                    pen = map_x(target, base_x + g->x);
                }
                s_place[i] = pen;
                pen += (int)rec.advance;
            }
        }
        for (int pass = 0; pass < 2; ++pass) {
            for (uint16_t i = 0; i < p->glyph_count; ++i) {
                const kj_text_glyph *g = &p->glyphs[i];
                kj_glyph rec;
                kj_glyph_art art;
                if (s_place[i] == -32768 ||
                    !kj_font_lookup(g->font_id, g->code, KJ_GLYPH_VARIANT_STANDARD, &rec) ||
                    !kj_font_art(rec.art_index, &art)) {
                    continue;
                }
                const int cell_x = s_place[i];
                const int cell_y = map_y(target, base_y + g->y);
                if (pass == 0) {
                    const uint32_t sc =
                        kj_text_ds_color(palette, p->color_mode, p->palette_num, g->shadow);
                    if (sc != 0) {
                        kj_batch_glyph(&run, kj_font_atlas_get(), &rec, &art,
                                       cell_x + rec.bearing_x + 1, cell_y + rec.bearing_y + 1, sc);
                    }
                } else {
                    const uint32_t fc =
                        kj_text_ds_color(palette, p->color_mode, p->palette_num, g->fg);
                    if (fc != 0) {
                        kj_batch_glyph(&run, kj_font_atlas_get(), &rec, &art,
                                       cell_x + rec.bearing_x, cell_y + rec.bearing_y, fc);
                    }
                }
                if (run.count + 8 > TEXT_RUN_VERTICES) {
                    GSPGPU_FlushDataCache(run.vertices, run.count * sizeof(kj_text_vertex));
                    kj_g2d_text_draw(target, (const kj_v2d *)run.vertices, run.count, true);
                    kj_text_note_draw();
                    run.count = 0;
                }
            }
        }
        if (run.count != 0) {
            GSPGPU_FlushDataCache(run.vertices, run.count * sizeof(kj_text_vertex));
            kj_g2d_text_draw(target, (const kj_v2d *)run.vertices, run.count, true);
            kj_text_note_draw();
        }
    }
}
