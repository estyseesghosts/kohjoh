/*
 * textures.c: DS 3D textures -> PICA textures (GPU_3D.md, materials).
 *
 * Formats (GBATEK): 1 A3I5, 2 4-colour, 3 16-colour, 4 256-colour, 5 4x4
 * compressed, 6 A5I3, 7 direct. A3I5/A5I3 become RGBA8 (graded alpha);
 * everything else RGBA5551 (DS colour exact, 1-bit alpha). Conversion is
 * offline-equivalent CPU format conversion at upload, never per frame.
 *
 * Texture slots 0-3 (VRAM A-D, MST 3) and palette slots 0-5 (E/F/G, MST 3)
 * are resolved from VRAMCNT each frame; texel reads cross 128 KiB slots
 * through seg().
 */

#include "graphics/g3d/textures.h"

#include "diagnostics/log.h"
#include "diagnostics/perf.h"

#include <3ds.h>
#include <stdlib.h>
#include <string.h>

#define CACHE_ENTRIES 384
#define CACHE_BYTES_MAX (5u * 1024u * 1024u)

typedef struct tex_entry {
    uint32_t key_param;
    uint32_t key_pltt;
    C3D_Tex tex;
    uint8_t *shadow;      /* source texel bytes (+ 4x4 index bytes) */
    uint32_t shadow_len;
    uint8_t *pal_shadow;
    uint32_t pal_len;
    uint32_t last_frame;
    uint32_t checked_frame;
    uint32_t bytes;
    bool used;
} tex_entry;

static tex_entry s_cache[CACHE_ENTRIES];
static uint32_t s_cache_bytes;
static uint32_t s_frame = 1;
static const uint8_t *s_tex_slot[4];
static const uint8_t *s_pal_slot[6];
static uint8_t s_morton[64];
static kj_tex_stats s_stats;
static int s_last_hit = -1;

static void build_morton(void)
{
    for (unsigned y = 0; y < 8; ++y) {
        for (unsigned x = 0; x < 8; ++x) {
            const unsigned fy = y; /* verified in Azahar against the oracle (frame 1650) */
            s_morton[y * 8 + x] = (uint8_t)((x & 1u) | ((fy & 1u) << 1) | ((x & 2u) << 1) |
                                            ((fy & 2u) << 2) | ((x & 4u) << 2) | ((fy & 4u) << 3));
        }
    }
}

void kj_tex_init(void)
{
    memset(s_cache, 0, sizeof(s_cache));
    build_morton();
}

static void entry_free(tex_entry *e)
{
    if (!e->used) {
        return;
    }
    C3D_TexDelete(&e->tex);
    free(e->shadow);
    free(e->pal_shadow);
    s_cache_bytes -= e->bytes;
    memset(e, 0, sizeof(*e));
}

void kj_tex_exit(void)
{
    for (int i = 0; i < CACHE_ENTRIES; ++i) {
        entry_free(&s_cache[i]);
    }
}

void kj_tex_begin_frame(const kj_frame_input *in)
{
    const kj_vram_bank *b = in->banks;
    const uint8_t *lcdc = in->mem->lcdc_vram;

    ++s_frame;
    for (int s = 0; s < 4; ++s) {
        s_tex_slot[s] = NULL;
    }
    for (int i = 0; i < 4; ++i) { /* A-D */
        if (b[i].enabled && b[i].mst == 3) {
            s_tex_slot[b[i].ofs & 3] = lcdc + b[i].lcdc_offset;
        }
    }
    for (int s = 0; s < 6; ++s) {
        s_pal_slot[s] = NULL;
    }
    if (b[4].enabled && b[4].mst == 3) { /* E: slots 0-3 */
        for (int s = 0; s < 4; ++s) {
            s_pal_slot[s] = lcdc + b[4].lcdc_offset + s * 0x4000;
        }
    }
    for (int i = 5; i <= 6; ++i) { /* F, G: OFS 0,1,2,3 -> slot 0,1,4,5 */
        if (b[i].enabled && b[i].mst == 3) {
            static const int SLOT[4] = { 0, 1, 4, 5 };
            s_pal_slot[SLOT[b[i].ofs & 3]] = lcdc + b[i].lcdc_offset;
        }
    }
}

/* Gather `len` bytes at texture-VRAM address `addr` (0..0x7FFFF). */
static bool gather(uint8_t *dst, uint32_t addr, uint32_t len)
{
    while (len > 0) {
        const unsigned slot = (addr >> 17) & 3u;
        const uint32_t off = addr & 0x1FFFFu;
        uint32_t n = 0x20000u - off;
        if (n > len) {
            n = len;
        }
        if (s_tex_slot[slot] == NULL) {
            return false;
        }
        memcpy(dst, s_tex_slot[slot] + off, n);
        dst += n;
        addr += n;
        len -= n;
    }
    return true;
}

/* Compare texture memory with a shadow in place, without gathering it into
 * a scratch copy first (one read stream instead of copy + compare: this
 * runs for every texture used, every frame). 1 = differs, 0 = same,
 * -1 = a slot is unmapped. */
static int differs(const uint8_t *shadow, uint32_t addr, uint32_t len)
{
    while (len > 0) {
        const unsigned slot = (addr >> 17) & 3u;
        const uint32_t off = addr & 0x1FFFFu;
        uint32_t n = 0x20000u - off;
        if (n > len) {
            n = len;
        }
        if (s_tex_slot[slot] == NULL) {
            return -1;
        }
        if (memcmp(shadow, s_tex_slot[slot] + off, n) != 0) {
            return 1;
        }
        shadow += n;
        addr += n;
        len -= n;
    }
    return 0;
}

static bool gather_pal(uint8_t *dst, uint32_t addr, uint32_t len)
{
    while (len > 0) {
        const unsigned slot = addr >> 14;
        const uint32_t off = addr & 0x3FFFu;
        uint32_t n = 0x4000u - off;
        if (n > len) {
            n = len;
        }
        if (slot >= 6 || s_pal_slot[slot] == NULL) {
            return false;
        }
        memcpy(dst, s_pal_slot[slot] + off, n);
        dst += n;
        addr += n;
        len -= n;
    }
    return true;
}

static uint16_t rgba5551(uint16_t c, bool opaque)
{
    return (uint16_t)(((c & 31u) << 11) | (((c >> 5) & 31u) << 6) | (((c >> 10) & 31u) << 1) |
                      (opaque ? 1u : 0u));
}

static uint32_t rgba8(uint16_t c, unsigned a8)
{
    const uint32_t r = c & 31u, g = (c >> 5) & 31u, b = (c >> 10) & 31u;
    return (((r << 3) | (r >> 2)) << 24) | (((g << 3) | (g >> 2)) << 16) |
           (((b << 3) | (b >> 2)) << 8) | a8;
}

typedef struct desc {
    unsigned fmt, w, h;
    uint32_t addr, pal_addr;
    bool color0_transparent;
    uint32_t data_len, index_addr, index_len, pal_len;
} desc;

static bool describe(uint32_t p, uint32_t pltt, desc *d)
{
    d->fmt = (p >> 26) & 7u;
    d->w = 8u << ((p >> 20) & 7u);
    d->h = 8u << ((p >> 23) & 7u);
    d->addr = (p & 0xFFFFu) << 3;
    d->color0_transparent = (p >> 29) & 1u;
    d->pal_addr = d->fmt == 2 ? (pltt & 0x1FFFu) << 3 : (pltt & 0x1FFFu) << 4;
    d->index_len = 0;
    d->index_addr = 0;
    const uint32_t texels = d->w * d->h;
    switch (d->fmt) {
    case 1: d->data_len = texels; d->pal_len = 64; break;
    case 2: d->data_len = texels / 4; d->pal_len = 8; break;
    case 3: d->data_len = texels / 2; d->pal_len = 32; break;
    case 4: d->data_len = texels; d->pal_len = 512; break;
    case 5:
        d->data_len = texels / 4;
        d->index_len = texels / 8;
        d->index_addr = 0x20000u + ((d->addr & 0x1FFFFu) >> 1) + ((d->addr >= 0x40000u) ? 0x10000u : 0);
        d->pal_len = 0; /* palette reach depends on the blocks: compared via the decode */
        break;
    case 6: d->data_len = texels; d->pal_len = 16; break;
    case 7: d->data_len = texels * 2; d->pal_len = 0; break;
    default: return false;
    }
    return true;
}

static inline uint32_t swz(unsigned x, unsigned y, unsigned w)
{
    return (((y >> 3) * (w >> 3) + (x >> 3)) << 6) + s_morton[((y & 7u) << 3) | (x & 7u)];
}

static uint16_t lerp555(uint16_t a, uint16_t b, unsigned wa, unsigned wb, unsigned den)
{
    unsigned r = ((a & 31u) * wa + (b & 31u) * wb) / den;
    unsigned g = (((a >> 5) & 31u) * wa + ((b >> 5) & 31u) * wb) / den;
    unsigned bl = (((a >> 10) & 31u) * wa + ((b >> 10) & 31u) * wb) / den;
    return (uint16_t)(r | (g << 5) | (bl << 10));
}

/* Decode into tex->data. src = texel bytes, idx = 4x4 index bytes. */
static bool decode(const desc *d, const uint8_t *src, const uint8_t *idx, C3D_Tex *tex)
{
    const unsigned w = d->w, h = d->h;
    static uint16_t pal[256];
    uint8_t palbuf[512];

    if (d->pal_len > 0) {
        if (!gather_pal(palbuf, d->pal_addr, d->pal_len)) {
            ++s_stats.missing_bank;
            return false;
        }
        memcpy(pal, palbuf, d->pal_len);
    }
    if (d->fmt == 1 || d->fmt == 6) {
        uint32_t *out = (uint32_t *)tex->data;
        for (unsigned y = 0; y < h; ++y) {
            for (unsigned x = 0; x < w; ++x) {
                const uint8_t t = src[y * w + x];
                unsigned i, a;
                if (d->fmt == 1) {
                    i = t & 31u;
                    a = t >> 5;
                    a = (a << 5) | (a << 2) | (a >> 1);
                } else {
                    i = t & 7u;
                    a = t >> 3;
                    a = (a << 3) | (a >> 2);
                }
                out[swz(x, y, w)] = rgba8(pal[i], a);
            }
        }
        return true;
    }
    uint16_t *out = (uint16_t *)tex->data;
    switch (d->fmt) {
    case 2:
    case 3:
    case 4: {
        const unsigned bits = d->fmt == 2 ? 2u : d->fmt == 3 ? 4u : 8u;
        const unsigned mask = (1u << bits) - 1u;
        for (unsigned y = 0; y < h; ++y) {
            for (unsigned x = 0; x < w; ++x) {
                const uint32_t bit = (y * w + x) * bits;
                const unsigned i = (src[bit >> 3] >> (bit & 7u)) & mask;
                const bool opaque = !(d->color0_transparent && i == 0);
                out[swz(x, y, w)] = rgba5551(pal[i], opaque);
            }
        }
        return true;
    }
    case 7:
        for (unsigned y = 0; y < h; ++y) {
            for (unsigned x = 0; x < w; ++x) {
                const uint16_t c = (uint16_t)(src[(y * w + x) * 2] | (src[(y * w + x) * 2 + 1] << 8));
                out[swz(x, y, w)] = rgba5551(c, (c & 0x8000u) != 0);
            }
        }
        return true;
    case 5: {
        const unsigned bw = w / 4;
        for (unsigned by = 0; by < h / 4; ++by) {
            for (unsigned bx = 0; bx < bw; ++bx) {
                const unsigned b = by * bw + bx;
                const uint32_t texel = (uint32_t)src[b * 4] | ((uint32_t)src[b * 4 + 1] << 8) |
                                       ((uint32_t)src[b * 4 + 2] << 16) | ((uint32_t)src[b * 4 + 3] << 24);
                const uint16_t ix = (uint16_t)(idx[b * 2] | (idx[b * 2 + 1] << 8));
                const unsigned mode = ix >> 14;
                uint8_t pb[8];
                uint16_t c[4];
                bool trans3 = false;
                if (!gather_pal(pb, d->pal_addr + (ix & 0x3FFFu) * 4u, 8)) {
                    ++s_stats.missing_bank;
                    return false;
                }
                for (int k = 0; k < 4; ++k) {
                    c[k] = (uint16_t)(pb[k * 2] | (pb[k * 2 + 1] << 8));
                }
                switch (mode) {
                case 0: trans3 = true; break;
                case 1: c[2] = lerp555(c[0], c[1], 1, 1, 2); trans3 = true; break;
                case 2: break;
                default:
                    c[2] = lerp555(c[0], c[1], 5, 3, 8);
                    c[3] = lerp555(c[0], c[1], 3, 5, 8);
                    break;
                }
                for (unsigned y = 0; y < 4; ++y) {
                    for (unsigned x = 0; x < 4; ++x) {
                        const unsigned i = (texel >> ((y * 4 + x) * 2)) & 3u;
                        out[swz(bx * 4 + x, by * 4 + y, w)] = rgba5551(c[i], !(trans3 && i == 3));
                    }
                }
            }
        }
        return true;
    }
    default:
        return false;
    }
}

static int find_victim(void)
{
    int best = -1;
    uint32_t oldest = UINT32_MAX;
    for (int i = 0; i < CACHE_ENTRIES; ++i) {
        if (!s_cache[i].used) {
            return i;
        }
        if (s_cache[i].last_frame != s_frame && s_cache[i].last_frame < oldest) {
            oldest = s_cache[i].last_frame;
            best = i;
        }
    }
    return best;
}

/* Is the cached entry's source unchanged? (once per frame) */
static bool still_valid(tex_entry *e, const desc *d)
{
    if (e->checked_frame == s_frame) {
        return true;
    }
    e->checked_frame = s_frame;
    const uint32_t total = d->data_len + d->index_len;
    const int data = differs(e->shadow, d->addr, d->data_len);
    const int index = d->index_len ? differs(e->shadow + d->data_len, d->index_addr, d->index_len) : 0;
    if (data < 0 || index < 0) {
        return true; /* bank unmapped this frame: keep what we have */
    }
    s_stats.compared_bytes += total;
    kj_perf_count(KJ_COUNT_TEX_BYTES, total);
    if (data || index) {
        return false;
    }
    if (e->pal_len) {
        uint8_t pal[512];
        if (gather_pal(pal, d->pal_addr, e->pal_len) && memcmp(pal, e->pal_shadow, e->pal_len) != 0) {
            return false;
        }
    }
    return true;
}

C3D_Tex *kj_tex_get(uint32_t tex_param, uint32_t pltt_base)
{
    desc d;
    if (!describe(tex_param, pltt_base, &d)) {
        return NULL;
    }
    const uint32_t key = tex_param & 0x3FF0FFFFu; /* not repeat/flip/texgen bits */
    const uint32_t pkey = d.fmt == 7 ? 0 : pltt_base;

    int hit = -1;
    if (s_last_hit >= 0 && s_cache[s_last_hit].used && s_cache[s_last_hit].key_param == key &&
        s_cache[s_last_hit].key_pltt == pkey) {
        hit = s_last_hit;
    } else {
        for (int i = 0; i < CACHE_ENTRIES; ++i) {
            if (s_cache[i].used && s_cache[i].key_param == key && s_cache[i].key_pltt == pkey) {
                hit = i;
                break;
            }
        }
    }
    if (hit >= 0) {
        tex_entry *e = &s_cache[hit];
        e->last_frame = s_frame;
        s_last_hit = hit;
        if (still_valid(e, &d)) {
            return &e->tex;
        }
        /* reconvert in place */
        if (!gather(e->shadow, d.addr, d.data_len) ||
            (d.index_len && !gather(e->shadow + d.data_len, d.index_addr, d.index_len))) {
            return &e->tex;
        }
        if (e->pal_len) {
            gather_pal(e->pal_shadow, d.pal_addr, e->pal_len);
        }
        decode(&d, e->shadow, e->shadow + d.data_len, &e->tex);
        ++s_stats.uploads;
        kj_perf_count(KJ_COUNT_TEX_UPLOAD, 1);
        return &e->tex;
    }

    const int v = find_victim();
    if (v < 0) {
        ++s_stats.full;
        return NULL;
    }
    entry_free(&s_cache[v]);
    tex_entry *e = &s_cache[v];
    const bool rgba8_fmt = d.fmt == 1 || d.fmt == 6;
    const uint32_t bytes = d.w * d.h * (rgba8_fmt ? 4u : 2u);
    while (s_cache_bytes + bytes > CACHE_BYTES_MAX) {
        const int u = find_victim();
        if (u < 0 || u == v) {
            ++s_stats.full;
            return NULL;
        }
        entry_free(&s_cache[u]);
    }
    if (!C3D_TexInit(&e->tex, (u16)d.w, (u16)d.h, rgba8_fmt ? GPU_RGBA8 : GPU_RGBA5551)) {
        ++s_stats.full;
        return NULL;
    }
    const uint32_t total = d.data_len + d.index_len;
    e->shadow = malloc(total);
    e->pal_len = d.pal_len;
    e->pal_shadow = d.pal_len ? malloc(d.pal_len) : NULL;
    if (e->shadow == NULL || (d.pal_len && e->pal_shadow == NULL) ||
        !gather(e->shadow, d.addr, d.data_len) ||
        (d.index_len && !gather(e->shadow + d.data_len, d.index_addr, d.index_len)) ||
        (d.pal_len && !gather_pal(e->pal_shadow, d.pal_addr, d.pal_len)) ||
        !decode(&d, e->shadow, e->shadow + d.data_len, &e->tex)) {
        C3D_TexDelete(&e->tex);
        free(e->shadow);
        free(e->pal_shadow);
        memset(e, 0, sizeof(*e));
        ++s_stats.missing_bank;
        return NULL;
    }
    C3D_TexSetFilter(&e->tex, GPU_NEAREST, GPU_NEAREST);
    e->used = true;
    e->key_param = key;
    e->key_pltt = pkey;
    e->shadow_len = total;
    e->bytes = bytes;
    e->last_frame = s_frame;
    e->checked_frame = s_frame;
    s_cache_bytes += bytes;
    s_last_hit = v;
    ++s_stats.uploads;
        kj_perf_count(KJ_COUNT_TEX_UPLOAD, 1);
    return &e->tex;
}

void kj_tex_take_stats(kj_tex_stats *out)
{
    *out = s_stats;
    memset(&s_stats, 0, sizeof(s_stats));
}
