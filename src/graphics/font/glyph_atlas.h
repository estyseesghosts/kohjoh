/*
 * glyph_atlas.h: the immutable native font atlas.
 *
 * The atlas binary is produced offline by tools/fonts/build_atlas.py and
 * embedded in the executable (see tools/game/hgss_game.py). Nothing here
 * parses BDF/TTF/OpenType, rasterises a glyph or allocates per character.
 *
 * Format (little endian; all offsets from the start of the blob):
 *
 *   header (48 bytes)
 *     0  "KJAT"           magic
 *     4  u16 version
 *     6  u16 header_size
 *     8  u8  page_count
 *     9  u8  flags            bit0 PICA tiled, bit1 RGBA5551
 *    10  u16 page_w
 *    12  u16 page_h
 *    14  u16 art_count
 *    16  u16 glyph_count
 *    18  u32 art_table_off
 *    22  u32 glyph_table_off
 *    26  u32 pages_off
 *    30  u32 data_size
 *    34  u16 baseline_in_cell
 *    36  u8  numeric_cell
 *    37  u8  variant_count
 *    38  u16 reserved
 *    40  u32 font_sha_prefix
 *    44  u32 reserved
 *
 *   art record (18 bytes): u16 page,x,y; u8 w,h,flags,reserved; u32 fp; u32 pad
 *   glyph record (18 bytes): u16 font,code,art,advance; i16 bearing_x,
 *                            bearing_y; u8 variant,flags; u32 unicode
 *
 * Glyph records are sorted by (font_id, code, variant) so lookup is a binary
 * search; the declaration below is the contract the Python generator writes.
 */

#ifndef KJ_GRAPHICS_FONT_GLYPH_ATLAS_H
#define KJ_GRAPHICS_FONT_GLYPH_ATLAS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KJ_GLYPH_ATLAS_MAGIC "KJAT"
#define KJ_GLYPH_ATLAS_VERSION 1u
#define KJ_GLYPH_ATLAS_HEADER_SIZE 48u
#define KJ_GLYPH_ATLAS_ART_SIZE 18u
#define KJ_GLYPH_ATLAS_GLYPH_SIZE 18u
#define KJ_GLYPH_ATLAS_MAX_PAGES 2u

#define KJ_GLYPH_VARIANT_STANDARD 0u
#define KJ_GLYPH_VARIANT_NUMERIC 1u
#define KJ_GLYPH_VARIANT_COUNT 2u

/* Pseudo-font for the specialised numeric bitmap printers
 * (message_printer.c / battle HP boxes), which bypass the text font. */
#define KJ_FONT_NUMERIC 255u

typedef struct kj_font_atlas {
    const uint8_t *blob;
    size_t size;
    uint8_t page_count;
    uint16_t page_w, page_h;
    uint16_t art_count, glyph_count;
    const uint8_t *art_table;
    const uint8_t *glyph_table;
    const uint8_t *pages;
    uint16_t baseline_in_cell;
    uint16_t numeric_cell;
} kj_font_atlas;

typedef struct kj_glyph {
    uint16_t font_id;
    uint16_t code;
    uint16_t art_index;
    uint16_t advance;
    int16_t bearing_x;
    int16_t bearing_y; /* top of the mask relative to the DS text cell top */
    uint8_t variant;
    uint8_t flags;
    uint32_t unicode;
} kj_glyph;

typedef struct kj_glyph_art {
    uint16_t page;
    uint16_t x, y;
    uint8_t w, h;
} kj_glyph_art;

/* Parse and bounds-check the embedded blob. Returns false on any mismatch;
 * the caller then keeps the legacy renderer and counts the failure. */
bool kj_font_atlas_init(const uint8_t *blob, size_t size);
void kj_font_atlas_exit(void);
const kj_font_atlas *kj_font_atlas_get(void);
bool kj_font_atlas_ready(void);

/* Exact (font, HGSS code, variant) lookup. Falls back from `variant` to the
 * standard record when the requested variant has no bitmap, which is what
 * makes a numeric label still render instead of falling back wholesale. */
bool kj_font_lookup(uint16_t font_id, uint16_t code, uint8_t variant, kj_glyph *out);

/* Artwork behind a glyph record. */
bool kj_font_art(uint16_t index, kj_glyph_art *out);

/* Page pixels: RGBA5551, PICA 8x8 tiled. */
const void *kj_font_page(uint8_t page);

#endif
