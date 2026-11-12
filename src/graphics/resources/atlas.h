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

/* A slot's corners in 1/16-texel units (the 2D vertex texcoord unit), inset
 * by 1/16 texel on every side. Nearest sampling at 1.25x lands at least 0.2
 * texel inside a tile, but float24 texcoords near v = 1.0 can round past the
 * edge into the neighbouring atlas tile -- usually transparent, which showed
 * as one-pixel horizontal gaps between tile rows. The inset keeps every
 * sample inside its own tile. */
#define KJ_ATLAS_UV_UNIT 16

static inline void kj_atlas_slot_uv(uint16_t slot, int16_t *u0, int16_t *v0, int16_t *u1,
                                    int16_t *v1)
{
    const int u = (slot % KJ_ATLAS_TILES_PER_ROW) * 8 * KJ_ATLAS_UV_UNIT;
    const int v = (slot / KJ_ATLAS_TILES_PER_ROW) * 8 * KJ_ATLAS_UV_UNIT;
    *u0 = (int16_t)(u + 1);
    *v0 = (int16_t)(v + 1);
    *u1 = (int16_t)(u + 8 * KJ_ATLAS_UV_UNIT - 1);
    *v1 = (int16_t)(v + 8 * KJ_ATLAS_UV_UNIT - 1);
}

typedef struct kj_atlas_stats {
    uint32_t lookups, conversions, evictions, full;
} kj_atlas_stats;

void kj_atlas_take_stats(kj_atlas_stats *out);

/* Keep slots alive this frame (retained layers reuse quads that point at
 * them; call before anything else allocates in the frame). */
void kj_atlas_touch(const uint16_t *slots, uint32_t count);

#endif
