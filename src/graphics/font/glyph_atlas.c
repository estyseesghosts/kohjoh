#include "glyph_atlas.h"

#include <string.h>

static kj_font_atlas s_atlas;
static bool s_ready;

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void parse_glyph(const uint8_t *p, kj_glyph *g)
{
    g->font_id = rd16(p + 0);
    g->code = rd16(p + 2);
    g->art_index = rd16(p + 4);
    g->advance = rd16(p + 6);
    g->bearing_x = (int16_t)rd16(p + 8);
    g->bearing_y = (int16_t)rd16(p + 10);
    g->variant = p[12];
    g->flags = p[13];
    g->unicode = rd32(p + 14);
}

static void parse_art(const uint8_t *p, kj_glyph_art *a)
{
    a->page = rd16(p + 0);
    a->x = rd16(p + 2);
    a->y = rd16(p + 4);
    a->w = p[6];
    a->h = p[7];
}

bool kj_font_atlas_init(const uint8_t *blob, size_t size)
{
    memset(&s_atlas, 0, sizeof(s_atlas));
    s_ready = false;
    if (blob == NULL || size < KJ_GLYPH_ATLAS_HEADER_SIZE) {
        return false;
    }
    if (memcmp(blob, KJ_GLYPH_ATLAS_MAGIC, 4) != 0) {
        return false;
    }
    const uint16_t version = rd16(blob + 4);
    const uint16_t header_size = rd16(blob + 6);
    if (version != KJ_GLYPH_ATLAS_VERSION || header_size != KJ_GLYPH_ATLAS_HEADER_SIZE) {
        return false;
    }
    const uint8_t page_count = blob[8];
    const uint8_t flags = blob[9];
    const uint16_t page_w = rd16(blob + 10);
    const uint16_t page_h = rd16(blob + 12);
    const uint16_t art_count = rd16(blob + 14);
    const uint16_t glyph_count = rd16(blob + 16);
    const uint32_t art_off = rd32(blob + 18);
    const uint32_t glyph_off = rd32(blob + 22);
    const uint32_t pages_off = rd32(blob + 26);
    const uint32_t total = rd32(blob + 30);

    if (page_count == 0 || page_count > KJ_GLYPH_ATLAS_MAX_PAGES || page_w == 0 || page_h == 0) {
        return false;
    }
    if ((flags & 0x3u) != 0x3u) { /* tiled RGBA5551 only */
        return false;
    }
    if ((page_w & 7u) || (page_h & 7u)) {
        return false;
    }
    if (total != size) {
        return false;
    }
    if (art_off < KJ_GLYPH_ATLAS_HEADER_SIZE ||
        art_off + (uint32_t)art_count * KJ_GLYPH_ATLAS_ART_SIZE > size) {
        return false;
    }
    if (glyph_off < art_off ||
        glyph_off + (uint32_t)glyph_count * KJ_GLYPH_ATLAS_GLYPH_SIZE > size) {
        return false;
    }
    if (pages_off < glyph_off ||
        (uint64_t)pages_off + (uint64_t)page_count * page_w * page_h * 2u > size) {
        return false;
    }

    s_atlas.blob = blob;
    s_atlas.size = size;
    s_atlas.page_count = page_count;
    s_atlas.page_w = page_w;
    s_atlas.page_h = page_h;
    s_atlas.art_count = art_count;
    s_atlas.glyph_count = glyph_count;
    s_atlas.art_table = blob + art_off;
    s_atlas.glyph_table = blob + glyph_off;
    s_atlas.pages = blob + pages_off;
    s_atlas.baseline_in_cell = rd16(blob + 34);
    s_atlas.numeric_cell = blob[36];
    s_ready = true;
    return true;
}

void kj_font_atlas_exit(void)
{
    memset(&s_atlas, 0, sizeof(s_atlas));
    s_ready = false;
}

const kj_font_atlas *kj_font_atlas_get(void)
{
    return s_ready ? &s_atlas : NULL;
}

bool kj_font_atlas_ready(void)
{
    return s_ready;
}

static int compare_key(const uint8_t *rec, uint16_t font_id, uint16_t code, uint8_t variant)
{
    const uint16_t rf = rd16(rec + 0);
    const uint16_t rc = rd16(rec + 2);
    const uint8_t rv = rec[12];
    if (font_id != rf) {
        return font_id < rf ? -1 : 1;
    }
    if (code != rc) {
        return code < rc ? -1 : 1;
    }
    if (variant != rv) {
        return variant < rv ? -1 : 1;
    }
    return 0;
}

static bool lookup_exact(uint16_t font_id, uint16_t code, uint8_t variant, kj_glyph *out)
{
    if (!s_ready) {
        return false;
    }
    uint32_t lo = 0, hi = s_atlas.glyph_count;
    while (lo < hi) {
        const uint32_t mid = lo + (hi - lo) / 2;
        const uint8_t *rec = s_atlas.glyph_table + mid * KJ_GLYPH_ATLAS_GLYPH_SIZE;
        const int cmp = compare_key(rec, font_id, code, variant);
        if (cmp == 0) {
            if (out != NULL) {
                parse_glyph(rec, out);
            }
            return true;
        }
        if (cmp < 0) {
            hi = mid; /* record sorts after the key */
        } else {
            lo = mid + 1;
        }
    }
    return false;
}

bool kj_font_lookup(uint16_t font_id, uint16_t code, uint8_t variant, kj_glyph *out)
{
    if (lookup_exact(font_id, code, variant, out)) {
        return true;
    }
    if (variant != KJ_GLYPH_VARIANT_STANDARD) {
        return lookup_exact(font_id, code, KJ_GLYPH_VARIANT_STANDARD, out);
    }
    return false;
}

bool kj_font_art(uint16_t index, kj_glyph_art *out)
{
    if (!s_ready || index >= s_atlas.art_count) {
        return false;
    }
    parse_art(s_atlas.art_table + (uint32_t)index * KJ_GLYPH_ATLAS_ART_SIZE, out);
    return true;
}

const void *kj_font_page(uint8_t page)
{
    if (!s_ready || page >= s_atlas.page_count) {
        return NULL;
    }
    return s_atlas.pages + (uint32_t)page * s_atlas.page_w * s_atlas.page_h * 2u;
}
