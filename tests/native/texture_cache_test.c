/* Native regression tests for the DS 3D texture cache (cleanup task 2).
 *
 * textures.c is included with a fake citro3d so the real decode and cache
 * logic runs against a plain CPU buffer. No 3DS services or game assets are
 * needed. Build through tests/native/test_textures.py.
 *
 * The tests cover:
 *   - an indexed texture whose palette changes;
 *   - a format-5 texture whose only change is referenced palette data;
 *   - a format-5 texture whose compressed index data changes;
 *   - an unmapped palette bank that becomes available again;
 *   - a failed refresh that must keep the previously decoded image.
 * Every recovered decode is compared byte-for-byte with a cache-free decode.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include SOURCE

bool C3D_TexInit(C3D_Tex *tex, u16 width, u16 height, GPU_TEXCOLOR fmt) {
    const size_t bytes = (size_t)width * height * (fmt == GPU_RGBA8 ? 4u : 2u);
    tex->data = malloc(bytes);
    if (tex->data == NULL) {
        return false;
    }
    memset(tex->data, 0xCD, bytes); /* a partial decode leaves sentinels */
    tex->width = width;
    tex->height = height;
    tex->fmt = fmt;
    return true;
}

void C3D_TexDelete(C3D_Tex *tex) {
    free(tex->data);
    tex->data = NULL;
}

void C3D_TexSetFilter(C3D_Tex *tex, GPU_TEXTURE_FILTER mag, GPU_TEXTURE_FILTER min) {
    (void)tex;
    (void)mag;
    (void)min;
}

/* One 0xA4000-byte block behaves like the mapped LCDC VRAM view. */
static uint8_t s_vram[0xA4000];
static kj_ds_memory s_mem;
static kj_frame_input s_in;

static void set_bank(int index, bool enabled, uint32_t offset, uint8_t mst, uint8_t ofs) {
    s_in.banks[index].lcdc_offset = offset;
    s_in.banks[index].size = 0x20000;
    s_in.banks[index].enabled = enabled;
    s_in.banks[index].mst = mst;
    s_in.banks[index].ofs = ofs;
}

static void setup(void) {
    memset(s_vram, 0, sizeof(s_vram));
    memset(&s_mem, 0, sizeof(s_mem));
    memset(&s_in, 0, sizeof(s_in));
    s_mem.lcdc_vram = s_vram;
    s_in.mem = &s_mem;
    /* Bank A -> texture slot 0, bank B -> texture slot 1. */
    set_bank(0, true, 0x00000, 3, 0);
    set_bank(1, true, 0x20000, 3, 1);
    /* Bank E -> palette slots 0-3. */
    set_bank(4, true, 0x40000, 3, 0);
    kj_tex_init();
    kj_tex_begin_frame(&s_in);
}

static void next_frame(void) {
    kj_tex_begin_frame(&s_in);
}

static uint32_t tex_param(unsigned fmt, unsigned log2_w, unsigned log2_h, uint32_t addr) {
    return (fmt << 26) | (log2_w << 20) | (log2_h << 23) | ((addr >> 3) & 0xFFFFu);
}

static size_t tex_bytes(const C3D_Tex *tex) {
    return (size_t)tex->width * tex->height * (tex->fmt == GPU_RGBA8 ? 4u : 2u);
}

/* Decode from an empty cache and require it to match the cached image.
 * `tex` is invalid after this call. */
static void assert_matches_clean(C3D_Tex *tex, uint32_t param, uint32_t pltt) {
    const size_t bytes = tex_bytes(tex);
    uint8_t *got = malloc(bytes);
    assert(got != NULL);
    memcpy(got, tex->data, bytes);

    kj_tex_exit();
    next_frame();
    C3D_Tex *clean = kj_tex_get(param, pltt);
    assert(clean != NULL);
    assert(tex_bytes(clean) == bytes);
    if (memcmp(got, clean->data, bytes) != 0) {
        fprintf(stderr, "cached decode differs from a clean decode\n");
        assert(0);
    }
    free(got);
}

static void fill_indexed(void) {
    /* 8x8, format 3 (16-colour): 32 texel bytes, 16 palette entries. */
    for (unsigned i = 0; i < 32; ++i) {
        s_vram[i] = (uint8_t)(i * 0x11u);
    }
    for (unsigned i = 0; i < 16; ++i) {
        const uint16_t colour = (uint16_t)(i | (i << 5) | ((15u - i) << 10));
        s_vram[0x40000 + i * 2] = (uint8_t)colour;
        s_vram[0x40000 + i * 2 + 1] = (uint8_t)(colour >> 8);
    }
}

static void fill_compressed(void) {
    /* 8x8, format 5: 16 texel bytes, 8 index bytes. All blocks select
     * palette offset 0; mode 0 is 3-colour + transparent texel 3. */
    for (unsigned i = 0; i < 16; ++i) {
        s_vram[i] = (uint8_t)(0x1Bu * (i + 1u));
    }
    memset(s_vram + 0x20000, 0, 8);
    for (unsigned i = 0; i < 4; ++i) {
        const uint16_t colour = (uint16_t)((i * 7u) | ((i * 3u) << 5) | ((15u - i) << 10));
        s_vram[0x40000 + i * 2] = (uint8_t)colour;
        s_vram[0x40000 + i * 2 + 1] = (uint8_t)(colour >> 8);
    }
}

static void test_indexed_palette_change(void) {
    setup();
    fill_indexed();
    const uint32_t param = tex_param(3, 0, 0, 0);
    C3D_Tex *tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    const size_t bytes = tex_bytes(tex);
    uint8_t *before = malloc(bytes);
    assert(before != NULL);
    memcpy(before, tex->data, bytes);

    s_vram[0x40000] ^= 0x1F; /* palette entry 0 red changes */
    next_frame();
    tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    assert(memcmp(before, tex->data, bytes) != 0);
    free(before);
    assert_matches_clean(tex, param, 0);
    printf("PASS indexed palette change reconverted\n");
}

static void test_compressed_palette_only_change(void) {
    setup();
    fill_compressed();
    const uint32_t param = tex_param(5, 0, 0, 0);
    C3D_Tex *tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    const size_t bytes = tex_bytes(tex);
    uint8_t *before = malloc(bytes);
    assert(before != NULL);
    memcpy(before, tex->data, bytes);

    /* Only the referenced 8-byte palette range changes. */
    s_vram[0x40000] ^= 0x1F;
    next_frame();
    tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    assert(memcmp(before, tex->data, bytes) != 0);
    free(before);
    assert_matches_clean(tex, param, 0);
    printf("PASS format-5 palette-only change reconverted\n");
}

static void test_compressed_index_change(void) {
    setup();
    fill_compressed();
    const uint32_t param = tex_param(5, 0, 0, 0);
    C3D_Tex *tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    const size_t bytes = tex_bytes(tex);
    uint8_t *before = malloc(bytes);
    assert(before != NULL);
    memcpy(before, tex->data, bytes);

    s_vram[0x20000] ^= 0x01; /* block 0 texel 0 index */
    next_frame();
    tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    assert(memcmp(before, tex->data, bytes) != 0);
    free(before);
    assert_matches_clean(tex, param, 0);
    printf("PASS format-5 index change reconverted\n");
}

static void test_unmapped_palette_keeps_image_then_recovers(void) {
    setup();
    fill_compressed();
    const uint32_t param = tex_param(5, 0, 0, 0);
    C3D_Tex *tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    const size_t bytes = tex_bytes(tex);
    uint8_t *before = malloc(bytes);
    assert(before != NULL);
    memcpy(before, tex->data, bytes);

    s_in.banks[4].enabled = false;
    next_frame();
    tex = kj_tex_get(param, 0);
    assert(tex != NULL); /* keep the known-good image while unavailable */
    assert(memcmp(before, tex->data, bytes) == 0);
    free(before);

    s_in.banks[4].enabled = true;
    next_frame();
    tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    assert_matches_clean(tex, param, 0);
    printf("PASS unmapped palette bank keeps the old image and recovers\n");
}

static void test_failed_refresh_then_success(void) {
    setup();
    fill_compressed();
    const uint32_t param = tex_param(5, 0, 0, 0);
    C3D_Tex *tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    const size_t bytes = tex_bytes(tex);
    uint8_t *before = malloc(bytes);
    assert(before != NULL);
    memcpy(before, tex->data, bytes);

    /* New source, but the palette range is unavailable: the refresh must
     * not publish a half-updated shadow or texture. */
    s_vram[0] ^= 0xFF;
    s_in.banks[4].enabled = false;
    next_frame();
    tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    assert(memcmp(before, tex->data, bytes) == 0);

    /* Palette returns: the pending source change is decoded now. */
    s_in.banks[4].enabled = true;
    next_frame();
    tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    assert(memcmp(before, tex->data, bytes) != 0);
    free(before);
    assert_matches_clean(tex, param, 0);
    printf("PASS failed refresh kept the old image, later refresh succeeded\n");
}

static void test_new_entry_requires_palette(void) {
    setup();
    fill_compressed();
    const uint32_t param = tex_param(5, 0, 0, 0);

    s_in.banks[4].enabled = false;
    next_frame();
    assert(kj_tex_get(param, 0) == NULL);

    s_in.banks[4].enabled = true;
    next_frame();
    C3D_Tex *tex = kj_tex_get(param, 0);
    assert(tex != NULL);
    assert_matches_clean(tex, param, 0);
    printf("PASS a new entry waits for its palette bank\n");
}

int main(void) {
    test_indexed_palette_change();
    test_compressed_palette_only_change();
    test_compressed_index_change();
    test_unmapped_palette_keeps_image_then_recovers();
    test_failed_refresh_then_success();
    test_new_entry_requires_palette();
    printf("PASS texture cache: all cases\n");
    return 0;
}
