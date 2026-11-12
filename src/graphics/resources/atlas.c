/*
 * atlas.c: see atlas.h.
 *
 * PICA textures are stored as 8x8 tiles in Morton order, which is exactly
 * the DS's tile granularity: one DS tile becomes one PICA tile, 64 texels
 * (128 bytes) at slot * 64 in texture memory. Memory tile rows run from the
 * top of the image (citro3d/tex3ds convention), so the slot's texel row in
 * the shader's top-down v is (slot / 128) * 8.
 *
 * Lookup: open-addressing hash table (linear probing, backward-shift
 * deletion) from key to slot; slots are evicted by a clock hand that skips
 * slots used in the current frame.
 */

#include "graphics/resources/atlas.h"

#include "diagnostics/log.h"

#include <3ds.h>
#include <string.h>

#define TABLE_SIZE 32768u /* power of two, 2x slots */
#define EMPTY 0xFFFFu

typedef struct slot_info {
    uint64_t key;
    uint32_t pal_version;
    uint32_t last_frame;
    uint8_t bytes[64];
    uint8_t used;
    uint8_t transparent;
    uint8_t bpp8;
} slot_info;

static C3D_Tex s_tex;
static uint16_t *s_texels;
static slot_info s_slots[KJ_ATLAS_SLOTS];
static uint16_t s_table[TABLE_SIZE];
static uint32_t s_frame = 1;
static uint32_t s_clock;
static uint8_t s_morton[64];
static kj_atlas_stats s_stats;

static uint32_t hash_key(uint64_t key)
{
    uint64_t h = key * 0x9E3779B97F4A7C15ull;
    return (uint32_t)(h >> 49) & (TABLE_SIZE - 1u);
}

static void build_morton(void)
{
    /* Texel (x, y) of an 8x8 tile, y from the top, -> Morton index: x in
     * the even bits, y in the odd bits, y flipped (PICA tiles are stored
     * bottom row first within the tile). */
    for (unsigned y = 0; y < 8; ++y) {
        for (unsigned x = 0; x < 8; ++x) {
            const unsigned fy = 7u - y;
            const unsigned m = (x & 1u) | ((fy & 1u) << 1) | ((x & 2u) << 1) | ((fy & 2u) << 2) |
                               ((x & 4u) << 2) | ((fy & 4u) << 3);
            s_morton[y * 8 + x] = (uint8_t)m;
        }
    }
}

bool kj_atlas_init(void)
{
    if (!C3D_TexInit(&s_tex, KJ_ATLAS_SIZE, KJ_ATLAS_SIZE, GPU_RGBA5551)) {
        kj_log("[atlas] cannot allocate the %dx%d atlas", KJ_ATLAS_SIZE, KJ_ATLAS_SIZE);
        return false;
    }
    C3D_TexSetFilter(&s_tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&s_tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    s_texels = (uint16_t *)s_tex.data;
    memset(s_texels, 0, KJ_ATLAS_SIZE * KJ_ATLAS_SIZE * 2);
    memset(s_table, 0xFF, sizeof(s_table));
    memset(s_slots, 0, sizeof(s_slots));
    build_morton();
    return true;
}

void kj_atlas_exit(void)
{
    C3D_TexDelete(&s_tex);
}

void kj_atlas_begin_frame(void)
{
    ++s_frame;
}

C3D_Tex *kj_atlas_texture(void)
{
    return &s_tex;
}

void kj_atlas_take_stats(kj_atlas_stats *out)
{
    *out = s_stats;
    memset(&s_stats, 0, sizeof(s_stats));
}

static uint16_t to_rgba5551(uint16_t c)
{
    return (uint16_t)(((c & 31u) << 11) | (((c >> 5) & 31u) << 6) | (((c >> 10) & 31u) << 1) | 1u);
}

void kj_palette_check(kj_palette *p)
{
    if (p->checked_frame == s_frame) {
        return;
    }
    p->checked_frame = s_frame;
    if (p->version != 0 && memcmp(p->shadow, p->colors, p->count * 2u) == 0) {
        return;
    }
    memcpy(p->shadow, p->colors, p->count * 2u);
    for (unsigned i = 0; i < p->count; ++i) {
        p->converted[i] = to_rgba5551(p->shadow[i]);
    }
    p->converted[0] &= 0xFFFEu; /* index 0 is transparent */
    ++p->version;
}

static void table_remove(uint64_t key)
{
    uint32_t i = hash_key(key);

    while (s_table[i] != EMPTY) {
        if (s_slots[s_table[i]].key == key) {
            break;
        }
        i = (i + 1u) & (TABLE_SIZE - 1u);
    }
    if (s_table[i] == EMPTY) {
        return;
    }
    /* backward-shift deletion */
    uint32_t hole = i;
    uint32_t j = i;
    for (;;) {
        j = (j + 1u) & (TABLE_SIZE - 1u);
        if (s_table[j] == EMPTY) {
            break;
        }
        const uint32_t home = hash_key(s_slots[s_table[j]].key);
        /* can the entry at j move to the hole? (home not in (hole, j]) */
        const bool between = (hole <= j) ? (home > hole && home <= j) : (home > hole || home <= j);
        if (!between) {
            s_table[hole] = s_table[j];
            hole = j;
        }
    }
    s_table[hole] = EMPTY;
}

static int find_free_slot(void)
{
    for (uint32_t n = 0; n < KJ_ATLAS_SLOTS; ++n) {
        const uint32_t s = s_clock;
        s_clock = (s_clock + 1u) % KJ_ATLAS_SLOTS;
        if (!s_slots[s].used) {
            return (int)s;
        }
        if (s_slots[s].last_frame != s_frame) {
            table_remove(s_slots[s].key);
            s_slots[s].used = 0;
            ++s_stats.evictions;
            return (int)s;
        }
    }
    return -1;
}

static void convert(uint16_t slot, const uint8_t *tile, bool bpp8, const kj_palette *pal,
                    slot_info *info)
{
    uint16_t *dst = s_texels + (uint32_t)slot * 64u;
    const uint16_t *conv = pal->converted;
    unsigned any = 0;

    if (bpp8) {
        for (unsigned i = 0; i < 64; ++i) {
            const unsigned idx = tile[i];
            any |= idx;
            dst[s_morton[i]] = idx ? conv[idx] : 0;
        }
        memcpy(info->bytes, tile, 64);
    } else {
        for (unsigned i = 0; i < 32; ++i) {
            const unsigned b = tile[i];
            const unsigned lo = b & 15u, hi = b >> 4;
            any |= b;
            dst[s_morton[2 * i]] = lo ? conv[lo] : 0;
            dst[s_morton[2 * i + 1]] = hi ? conv[hi] : 0;
        }
        memcpy(info->bytes, tile, 32);
    }
    info->transparent = any == 0;
    ++s_stats.conversions;
}

kj_tile_ref kj_atlas_tile(uint32_t source_id, const uint8_t *tile, uint32_t offset, bool bpp8,
                          kj_palette *pal, uint32_t pal_id)
{
    kj_tile_ref ref = { 0, 0, 0 };
    const uint64_t key = ((uint64_t)source_id << 56) | ((uint64_t)(bpp8 ? 1 : 0) << 55) |
                         ((uint64_t)(pal_id & 0xFFFFu) << 32) | offset;
    const size_t nbytes = bpp8 ? 64u : 32u;
    uint32_t i = hash_key(key);

    ++s_stats.lookups;
    while (s_table[i] != EMPTY) {
        slot_info *info = &s_slots[s_table[i]];
        if (info->key == key) {
            if (info->pal_version != pal->version || memcmp(info->bytes, tile, nbytes) != 0) {
                convert(s_table[i], tile, bpp8, pal, info);
                info->pal_version = pal->version;
            }
            info->last_frame = s_frame;
            ref.slot = s_table[i];
            ref.transparent = info->transparent;
            ref.valid = 1;
            return ref;
        }
        i = (i + 1u) & (TABLE_SIZE - 1u);
    }

    /* Fast path for empty tiles: never take a slot. */
    {
        unsigned any = 0;
        for (size_t k = 0; k < nbytes; ++k) {
            any |= tile[k];
        }
        if (any == 0) {
            ref.transparent = 1;
            ref.valid = 1;
            return ref;
        }
    }

    const int slot = find_free_slot();
    if (slot < 0) {
        ++s_stats.full;
        return ref;
    }
    slot_info *info = &s_slots[slot];
    info->key = key;
    info->used = 1;
    info->bpp8 = bpp8;
    info->last_frame = s_frame;
    info->pal_version = pal->version;
    convert((uint16_t)slot, tile, bpp8, pal, info);
    /* insert (the probe position may have moved if find_free_slot deleted) */
    i = hash_key(key);
    while (s_table[i] != EMPTY) {
        i = (i + 1u) & (TABLE_SIZE - 1u);
    }
    s_table[i] = (uint16_t)slot;
    ref.slot = (uint16_t)slot;
    ref.transparent = info->transparent;
    ref.valid = 1;
    return ref;
}
