#ifndef KJ_GRAPHICS_RESOURCES_ATLAS_H
#define KJ_GRAPHICS_RESOURCES_ATLAS_H

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * The 2D tile atlas: palette-expanded 8x8 DS tiles in one 1024x1024 RGBA5551
 * texture (16384 slots). RGBA5551 holds a DS colour exactly; alpha 0 is
 * palette index 0 (transparent).
 *
 * Correctness by construction: a slot is reused only if its source tile's
 * bytes are byte-identical to the copy taken at conversion AND its palette's
 * version is unchanged. Palette versions come from comparing each palette
 * against a shadow copy once per frame on first use. No write tracking or
 * hashing of live memory is involved, so a VRAM write the renderer did not
 * see cannot produce a stale tile.
 */

#define KJ_ATLAS_SIZE 1024
#define KJ_ATLAS_TILES_PER_ROW (KJ_ATLAS_SIZE / 8)
#define KJ_ATLAS_SLOTS (KJ_ATLAS_TILES_PER_ROW * KJ_ATLAS_TILES_PER_ROW)

/* A palette the atlas can expand tiles with. */
typedef struct kj_palette {
    const uint16_t *colors; /* live DS memory (BGR555) */
    uint16_t count;         /* 16 or 256 */
    uint16_t converted[256];/* RGBA5551 of the current version */
    uint16_t shadow[256];   /* the colours of the current version */
    uint32_t version;
    uint32_t checked_frame;
} kj_palette;

/* Result of a tile lookup. */
typedef struct kj_tile_ref {
    uint16_t slot;      /* atlas slot */
    uint8_t transparent;/* every pixel is index 0: draw nothing */
    uint8_t valid;      /* 0: atlas full (counted), draw nothing */
} kj_tile_ref;

bool kj_atlas_init(void);
void kj_atlas_exit(void);
/* Start a frame: advances the frame stamp used for eviction and palettes. */
void kj_atlas_begin_frame(void);
C3D_Tex *kj_atlas_texture(void);

/* Make sure palette `p` reflects live memory this frame. */
void kj_palette_check(kj_palette *p);

/* Look up (and if needed convert) a tile.
 *   source_id: distinguishes VRAM views (engine x BG/OBJ)
 *   tile: pointer to the tile's 32 (4 bpp) or 64 (8 bpp) bytes
 *   offset: byte offset of the tile in its view (part of the key)
 *   bpp8: 8 bpp tile
 *   pal: palette (16 colours for 4 bpp, 256 for 8 bpp)
 *   pal_id: identity of `pal` within the frame (part of the key) */
kj_tile_ref kj_atlas_tile(uint32_t source_id, const uint8_t *tile, uint32_t offset, bool bpp8,
                          kj_palette *pal, uint32_t pal_id);

/* Atlas texel coordinates of a slot's top-left corner. */
static inline void kj_atlas_slot_uv(uint16_t slot, int16_t *u, int16_t *v)
{
    *u = (int16_t)((slot % KJ_ATLAS_TILES_PER_ROW) * 8);
    *v = (int16_t)((slot / KJ_ATLAS_TILES_PER_ROW) * 8);
}

typedef struct kj_atlas_stats {
    uint32_t lookups, conversions, evictions, full;
} kj_atlas_stats;

void kj_atlas_take_stats(kj_atlas_stats *out);

#endif
