/*
 * kj_g3.c: the DS geometry engine, DS side.
 *
 * Interprets the G3 command stream (direct opcode writes and packed GXFIFO
 * words) with the DS's semantics, and produces a kj_g3_frame per
 * SWAP_BUFFERS for the PICA backend.
 *
 * Division of work (kohjoh design, docs/architecture/3d.md):
 *   CPU (here): matrix stacks and products (20.12 fixed, as the DS), the
 *     game-visible results (clip/vector matrix registers, BOX/POS/VEC
 *     tests, GXSTAT levels), per-vertex lighting and texture-coordinate
 *     generation (exact DS fixed point), primitive assembly to indexed
 *     triangles, material batching.
 *   GPU: position transform. Vertices stay in object space (s16 4.12) and
 *     carry an index into a small palette of clip matrices uploaded per
 *     draw, so no per-vertex matrix math or clipping runs on the ARM11.
 *
 * Fixed-point lighting and texgen follow GBATEK as verified by hgss-3ds
 * (3ds_port/nitro/src/port_gx3d.c @ a26c003, GPL-3.0): shade(),
 * transform_texcoords(), set_geo_position().
 */

#include "kj_g3.h"

#include <nitro.h>
#include <nitro_port.h>
#include <nitro_port_gx3d.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/* The display-list cache is on by default: KJ_DL_VERIFY found 0 mismatches
 * in the intro, title, Goldenrod idle, and Goldenrod -> Route 34 -> trainer
 * battle -> field (2026-11-13). --define KJ_DL_NO_CACHE builds the
 * uncached decoder for A/B runs and bisects. */
#if !defined(KJ_DL_NO_CACHE) && !defined(KJ_DL_CACHE)
#define KJ_DL_CACHE
#endif

extern unsigned char NitroPort_IoReg[];

/* ------------------------------------------------------------- matrices */

typedef struct mtx {
    int32_t m[4][4]; /* 20.12, row-vector convention: v' = v * M */
} mtx;

#define POSVEC_STACK 32
#define FX_ONE 4096

static mtx s_proj, s_pos, s_vec, s_tex;
static mtx s_proj_stack[1], s_tex_stack[1];
static mtx s_pos_stack[POSVEC_STACK], s_vec_stack[POSVEC_STACK];
static int s_proj_sp, s_tex_sp, s_posvec_sp;
static int s_mtx_mode;
static bool s_stack_error;
static mtx s_clip;
static bool s_clip_dirty = true;
static uint32_t s_clip_version = 1; /* bumps on every clip change */

static void mtx_identity(mtx *m)
{
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = m->m[3][3] = FX_ONE;
}

/* out = a * b (row-vector convention: apply a, then b). */
static void mtx_mul(mtx *out, const mtx *a, const mtx *b)
{
    mtx r;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int64_t acc = 0;
            for (int k = 0; k < 4; ++k) {
                acc += (int64_t)a->m[i][k] * b->m[k][j];
            }
            r.m[i][j] = (int32_t)(acc >> 12);
        }
    }
    *out = r;
}

static int32_t sign_bits(uint32_t v, int bits)
{
    const uint32_t mask = 1u << (bits - 1);
    v &= (1u << bits) - 1u;
    return (int32_t)((v ^ mask) - mask);
}

static void normals_changed(void);
static void vec_changed(void);

static void clip_changed(void)
{
    s_clip_dirty = true;
    ++s_clip_version;
}

static const mtx *clip_matrix(void)
{
    if (s_clip_dirty) {
        mtx_mul(&s_clip, &s_pos, &s_proj);
        s_clip_dirty = false;
    }
    return &s_clip;
}

/* The matrix MTX_LOAD/MULT/SCALE/TRANS act on, for mode 0/1/3. */
static mtx *selected(void)
{
    switch (s_mtx_mode) {
    case 0:
        return &s_proj;
    case 3:
        return &s_tex;
    default:
        return &s_pos;
    }
}

static void read_matrix(mtx *m, unsigned op, const uint32_t *p)
{
    mtx_identity(m);
    if (op == 0x16 || op == 0x18) {
        for (int i = 0; i < 16; ++i) {
            m->m[i / 4][i % 4] = (int32_t)p[i];
        }
    } else if (op == 0x17 || op == 0x19) {
        for (int i = 0; i < 12; ++i) {
            m->m[i / 3][i % 3] = (int32_t)p[i];
        }
    } else { /* 0x1A: 3x3 */
        for (int i = 0; i < 9; ++i) {
            m->m[i / 3][i % 3] = (int32_t)p[i];
        }
    }
}

static void load_matrix(const mtx *m)
{
    if (s_mtx_mode == 2) {
        s_pos = *m;
        s_vec = *m;
        vec_changed();
    } else {
        *selected() = *m;
    }
    if (s_mtx_mode != 3) {
        clip_changed();
    }
}

static void mult_matrix(const mtx *m, bool scale_only_pos)
{
    if (s_mtx_mode == 2) {
        mtx_mul(&s_pos, m, &s_pos);
        if (!scale_only_pos) {
            mtx_mul(&s_vec, m, &s_vec);
            vec_changed();
        }
    } else {
        mtx *t = selected();
        mtx_mul(t, m, t);
    }
    if (s_mtx_mode != 3) {
        clip_changed();
    }
}

/* MTX_SCALE: S * M with S = diag(sx, sy, sz, 1) scales rows 0-2 of M; the
 * full product's other terms are exact multiples of 4096 (identical after
 * >> 12). Mode 2 scales the position matrix only (GBATEK). 12 products
 * instead of 64 (O3DS: 1.1 ms per game frame, o3ds-log-0). */
static void scale_rows(mtx *t, const uint32_t *p)
{
    for (int i = 0; i < 3; ++i) {
        const int64_t f = (int32_t)p[i];
        for (int j = 0; j < 4; ++j) {
            t->m[i][j] = (int32_t)((f * t->m[i][j]) >> 12);
        }
    }
}

static void scale_matrix(const uint32_t *p)
{
    scale_rows(s_mtx_mode == 2 ? &s_pos : selected(), p);
    if (s_mtx_mode != 3) {
        clip_changed();
    }
}

/* MTX_TRANS: T * M changes row 3 only: row3 = t . rows0-2 + row3. */
static void translate_rows(mtx *t, const uint32_t *p)
{
    for (int j = 0; j < 4; ++j) {
        const int64_t acc = (int64_t)(int32_t)p[0] * t->m[0][j] + (int64_t)(int32_t)p[1] * t->m[1][j] +
                            (int64_t)(int32_t)p[2] * t->m[2][j] + (int64_t)4096 * t->m[3][j];
        t->m[3][j] = (int32_t)(acc >> 12);
    }
}

static void translate_matrix(const uint32_t *p)
{
    if (s_mtx_mode == 2) {
        translate_rows(&s_pos, p);
        translate_rows(&s_vec, p); /* row 3 of the vector matrix: lighting never reads it */
    } else {
        translate_rows(selected(), p);
    }
    if (s_mtx_mode != 3) {
        clip_changed();
    }
}

#ifdef KJ_G3_ZERO_TRACE
static void execute_body(unsigned op, const uint32_t *p);
#endif

static void stack_op(unsigned op, uint32_t param)
{
    if (s_mtx_mode == 0) {
        switch (op) {
        case 0x11:
            if (s_proj_sp >= 1) {
                s_stack_error = true;
            }
            s_proj_stack[0] = s_proj;
            s_proj_sp = 1;
            break;
        case 0x12:
            s_proj_sp = 0;
            s_proj = s_proj_stack[0];
            clip_changed();
            break;
        case 0x13:
            s_proj_stack[0] = s_proj;
            break;
        case 0x14:
            s_proj = s_proj_stack[0];
            clip_changed();
            break;
        }
        return;
    }
    if (s_mtx_mode == 3) {
        switch (op) {
        case 0x11:
        case 0x13:
            s_tex_stack[0] = s_tex;
            s_tex_sp = 1;
            break;
        case 0x12:
        case 0x14:
            s_tex = s_tex_stack[0];
            s_tex_sp = 0;
            break;
        }
        return;
    }
    switch (op) {
    case 0x11: /* PUSH */
        if (s_posvec_sp >= 31) {
            s_stack_error = true;
        }
        s_pos_stack[s_posvec_sp & 31] = s_pos;
        s_vec_stack[s_posvec_sp & 31] = s_vec;
        s_posvec_sp = (s_posvec_sp + 1) & 63;
        break;
    case 0x12: { /* POP n (signed 6 bit) */
        s_posvec_sp = (s_posvec_sp - sign_bits(param, 6)) & 63;
        if (s_posvec_sp >= 31) {
            s_stack_error = true;
        }
        s_pos = s_pos_stack[s_posvec_sp & 31];
        s_vec = s_vec_stack[s_posvec_sp & 31];
        clip_changed();
        vec_changed();
        break;
    }
    case 0x13: { /* STORE */
        const unsigned i = param & 31;
        if (i == 31) {
            s_stack_error = true;
        }
        s_pos_stack[i] = s_pos;
        s_vec_stack[i] = s_vec;
        break;
    }
    case 0x14: { /* RESTORE */
        const unsigned i = param & 31;
        if (i == 31) {
            s_stack_error = true;
        }
        s_pos = s_pos_stack[i];
        s_vec = s_vec_stack[i];
        clip_changed();
        vec_changed();
        break;
    }
    }
}

/* ------------------------------------------------------- vertex state */

static uint16_t s_color = 0x7FFF;
static int16_t s_coord[3];
static int32_t s_texcoord_raw[2]; /* last TEXCOORD, 12.4 */
static int32_t s_texcoord[2];     /* effective, 12.4 */

/* material and lights */
static uint16_t s_diffuse, s_ambient, s_specular, s_emission;
static bool s_use_shininess;
static uint32_t s_shininess[32];
static uint16_t s_light_color[4];
static int32_t s_light_dir[4][3];
static int32_t s_light_half[4][3];
static uint8_t s_product[32][32];
static bool s_products_made;

/* polygon state */
static uint32_t s_attr_pending, s_attr;
static uint32_t s_tex_param, s_tex_pltt;
static uint32_t s_viewport = 0xBFFF0000u; /* 0,0,255,191 */
static int s_primitive;
static bool s_in_begin;

static void make_products(void)
{
    for (unsigned a = 0; a < 32; ++a) {
        for (unsigned b = 0; b < 32; ++b) {
            s_product[a][b] = (uint8_t)((a * b + 15) / 31);
        }
    }
    s_products_made = true;
}

static void transform_vector(const int32_t in[3], const mtx *m, int32_t out[3])
{
    for (int j = 0; j < 3; ++j) {
        int64_t acc = 0;
        for (int i = 0; i < 3; ++i) {
            acc += (int64_t)in[i] * m->m[i][j];
        }
        out[j] = (int32_t)(acc >> 12);
    }
}

/* Lighting results by raw normal: map geometry repeats a handful of
 * normals. Generation-keyed: anything shade() depends on (vector matrix,
 * lights, material, the polygon's light enables) bumps the generation
 * (the same sites as Platinum's port_gx3d.c normal cache). */
#define NORMAL_CACHE 64u
static uint32_t s_normal_gen = 1;
static struct {
    uint32_t gen, value;
    uint16_t rgb;
} s_normal_cache[NORMAL_CACHE];

static void normals_changed(void)
{
    ++s_normal_gen;
}

/* Lighting reads only the vector matrix's 3x3 part. NNS restores a matrix
 * per node, and map props are translated, not rotated: comparing content
 * keeps the normal cache warm across objects (O3DS: NORMAL cost 2.2 ms per
 * game frame with the cache flushed at every restore, o3ds-log-0). */
static int32_t s_shade_vec[3][3];

static void vec_changed(void)
{
    bool same = true;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (s_shade_vec[i][j] != s_vec.m[i][j]) {
                s_shade_vec[i][j] = s_vec.m[i][j];
                same = false;
            }
        }
    }
    if (!same) {
        ++s_normal_gen;
    }
}

static uint16_t shade_uncached(uint32_t value);

static uint16_t shade(uint32_t value)
{
    const unsigned slot = (unsigned)((value * 2654435761u) >> 26) & (NORMAL_CACHE - 1u);
    if (s_normal_cache[slot].gen == s_normal_gen && s_normal_cache[slot].value == value) {
        return s_normal_cache[slot].rgb;
    }
    const uint16_t rgb = shade_uncached(value);
    s_normal_cache[slot].gen = s_normal_gen;
    s_normal_cache[slot].value = value;
    s_normal_cache[slot].rgb = rgb;
    return rgb;
}

static uint16_t shade_uncached(uint32_t value)
{
    int32_t normal[3], n[3];
    unsigned r = s_emission & 31u, g = (s_emission >> 5) & 31u, b = (s_emission >> 10) & 31u;

    for (int j = 0; j < 3; ++j) {
        normal[j] = sign_bits(value >> (j * 10), 10) * 8;
    }
    transform_vector(normal, &s_vec, n);
    for (int i = 0; i < 4; ++i) {
        if (!(s_attr & (1u << i))) {
            continue;
        }
        int64_t dot = 0, half_dot = 0;
        for (int j = 0; j < 3; ++j) {
            dot += (int64_t)n[j] * s_light_dir[i][j];
            half_dot += (int64_t)n[j] * s_light_half[i][j];
        }
        int d = (int)(-dot >> 12);
        const unsigned diffuse = d < 0 ? 0u : d > 4096 ? 4096u : (unsigned)d;
        int h = (int)(-half_dot >> 12);
        unsigned spec = h < 0 ? 0u : h > 4096 ? 4096u : (unsigned)h;
        spec = (spec * spec) >> 12;
        if (s_use_shininess) {
            unsigned index = (spec * 128u) >> 12;
            if (index > 127u) {
                index = 127u;
            }
            const unsigned entry = (s_shininess[index >> 2] >> ((index & 3u) * 8u)) & 255u;
            spec = entry * 4096u / 255u;
        }
        const uint16_t lc = s_light_color[i];
        for (int c = 0; c < 3; ++c) {
            const unsigned l = (lc >> (5 * c)) & 31u;
            const unsigned amb = s_product[(s_ambient >> (5 * c)) & 31u][l];
            const unsigned dif = s_product[(s_diffuse >> (5 * c)) & 31u][l];
            const unsigned spc = s_product[(s_specular >> (5 * c)) & 31u][l];
            const unsigned add = amb + ((dif * diffuse + 2048u) >> 12) + ((spc * spec + 2048u) >> 12);
            if (c == 0) {
                r += add;
            } else if (c == 1) {
                g += add;
            } else {
                b += add;
            }
        }
    }
    return (uint16_t)((r > 31u ? 31u : r) | ((g > 31u ? 31u : g) << 5) | ((b > 31u ? 31u : b) << 10));
}

static int32_t saturate16(int64_t v)
{
    return v < -32768 ? -32768 : v > 32767 ? 32767 : (int32_t)v;
}

/* Texture-coordinate generation (TEXIMAGE_PARAM bits 30-31). Mode 1 at
 * TEXCOORD, mode 2 at NORMAL, mode 3 at each vertex (GBATEK). */
static void texgen(const int32_t *source, int mode)
{
    for (int j = 0; j < 2; ++j) {
        int64_t v;
        if (mode == 1) {
            v = (int64_t)source[0] * s_tex.m[0][j] + (int64_t)source[1] * s_tex.m[1][j] +
                (int64_t)s_tex.m[2][j] + s_tex.m[3][j];
            s_texcoord[j] = saturate16(v >> 12);
        } else {
            v = (int64_t)source[0] * s_tex.m[0][j] + (int64_t)source[1] * s_tex.m[1][j] +
                (int64_t)source[2] * s_tex.m[2][j];
            s_texcoord[j] = saturate16((v >> 20) + s_texcoord_raw[j]);
        }
    }
}

/* ------------------------------------------------------- frame storage */

typedef struct frame_buf {
    kj_g3_frame frame;
    kj_g3_vertex *verts;
    uint16_t *indices;
    kj_g3_matrix matrices[KJ_G3_MAX_MATRICES]; /* per-batch palette slices */
    kj_g3_matrix pool[KJ_G3_MAX_MATRICES];     /* every clip matrix this frame (CPU only) */
    uint32_t pool_count;
    kj_g3_batch batches[KJ_G3_MAX_BATCHES];
    uint16_t vert_gmtx[KJ_G3_MAX_VERTS]; /* global matrix id per vertex (pool < 65536) */
} frame_buf;

static frame_buf s_buf[2];
static int s_build;          /* index of the frame being built */
static bool s_have_display;  /* a swap has happened */
static bool s_wide;
static kj_g3_stats s_stats;

/* Current batch bookkeeping. s_batch_ok caches "the open batch matches the
 * current material state": cleared by every material-state change and by
 * opening/closing a batch, so the per-vertex path tests one flag. The slot
 * cache remembers the palette slot of the current clip matrix in the open
 * batch. */
static bool s_batch_ok;
static uint32_t s_slot_gid = UINT32_MAX;
static int s_slot_idx;
static uint32_t s_color_rgba; /* packed r,g,b,a of the current colour + polygon alpha */
static kj_g3_batch *s_batch;
static uint32_t s_batch_first_vert;
static uint32_t s_batch_key_attr, s_batch_key_tex, s_batch_key_pltt, s_batch_key_vp;
static uint32_t s_palette_gid[KJ_G3_PALETTE]; /* global matrix id per palette slot */
static uint32_t s_cur_gmtx;                   /* global matrix id of the current clip */
static uint32_t s_cur_gmtx_version;

/* Primitive assembly: indices (into the frame) of this BEGIN's recent
 * vertices. */
static uint32_t s_prim_verts[4];
static uint32_t s_prim_count;     /* vertices since BEGIN */
static uint32_t s_strip_parity;

static void update_color_rgba(void);

void kj_g3_init(const kj_g3_storage *storage)
{
    for (int i = 0; i < 2; ++i) {
        s_buf[i].verts = storage->verts[i];
        s_buf[i].indices = storage->indices[i];
    }
    mtx_identity(&s_proj);
    mtx_identity(&s_pos);
    mtx_identity(&s_vec);
    mtx_identity(&s_tex);
    clip_changed();
    if (!s_products_made) {
        make_products();
    }
    update_color_rgba();
}

void kj_g3_set_wide(bool wide)
{
    s_wide = wide;
}

void kj_g3_get_stats(kj_g3_stats *out)
{
    *out = s_stats;
}

const kj_g3_frame *kj_g3_take_frame(void)
{
    return s_have_display ? &s_buf[s_build ^ 1].frame : NULL;
}

static frame_buf *build(void)
{
    return &s_buf[s_build];
}

/* The current clip matrix as a global (frame) matrix id, pushing it into
 * the frame's matrix list when it changed. */
static bool current_gmtx(uint32_t *out)
{
    frame_buf *fb = build();

    if (s_cur_gmtx_version != s_clip_version || fb->pool_count == 0) {
        if (fb->pool_count >= KJ_G3_MAX_MATRICES) {
            return false;
        }
        const mtx *c = clip_matrix();
        kj_g3_matrix *dst = &fb->pool[fb->pool_count];
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                dst->m[i][j] = (float)c->m[i][j] * (1.0f / 4096.0f);
            }
        }
        s_cur_gmtx = fb->pool_count++;
        s_cur_gmtx_version = s_clip_version;
    }
    *out = s_cur_gmtx;
    return true;
}

static const uint8_t EXPAND5[32] = {
    0, 8, 16, 24, 33, 41, 49, 57, 66, 74, 82, 90, 99, 107, 115, 123,
    132, 140, 148, 156, 165, 173, 181, 189, 198, 206, 214, 222, 231, 239, 247, 255,
};

static void update_color_rgba(void)
{
    const unsigned a = (s_attr >> 16) & 31u;
    s_color_rgba = (uint32_t)EXPAND5[s_color & 31u] | ((uint32_t)EXPAND5[(s_color >> 5) & 31u] << 8) |
                   ((uint32_t)EXPAND5[(s_color >> 10) & 31u] << 16) | ((uint32_t)EXPAND5[a] << 24);
}

static void material_changed(void)
{
    s_batch_ok = false;
}

static void close_batch(void)
{
    s_batch = NULL;
    s_batch_ok = false;
    s_slot_gid = UINT32_MAX;
}

static bool open_batch(void)
{
    frame_buf *fb = build();

    if (fb->frame.batch_count >= KJ_G3_MAX_BATCHES) {
        ++fb->frame.dropped_batches;
        return false;
    }
    s_batch = &fb->batches[fb->frame.batch_count++];
    memset(s_batch, 0, sizeof(*s_batch));
    s_batch->first_index = fb->frame.index_count;
    s_batch->first_matrix = 0xFFFF;
    s_batch->poly_attr = s_attr;
    s_batch->tex_param = s_tex_param;
    s_batch->tex_pltt = s_tex_pltt;
    s_batch->viewport = s_viewport;
    {
        const unsigned alpha = (s_attr >> 16) & 31u;
        const unsigned fmt = (s_tex_param >> 26) & 7u;
        const unsigned mode = (s_attr >> 4) & 3u;
        if ((alpha > 0 && alpha < 31) || fmt == 1 || fmt == 6) {
            s_batch->flags |= KJ_G3_BATCH_TRANSLUCENT;
        }
        if (mode == 3) {
            s_batch->flags |= KJ_G3_BATCH_SHADOW;
        }
    }
    s_batch_first_vert = fb->frame.vert_count;
    s_batch_key_attr = s_attr;
    s_batch_key_tex = s_tex_param;
    s_batch_key_pltt = s_tex_pltt;
    s_batch_key_vp = s_viewport;
    s_batch_ok = true;
    s_slot_gid = UINT32_MAX;
    return true;
}

/* Palette slot of global matrix `gid` in the open batch, adding it if
 * there is room; -1 when the palette is full. */
static int palette_slot(uint32_t gid)
{
    const uint16_t count = s_batch->matrix_count;

    for (int i = count - 1; i >= 0; --i) {
        if (s_palette_gid[i] == gid) {
            return i;
        }
    }
    if (count >= KJ_G3_PALETTE) {
        return -1;
    }
    /* A batch's palette must be a contiguous slice of the frame's matrix
     * list: copy the matrix to the slice's end. */
    frame_buf *fb = build();
    if (s_batch->first_matrix == 0xFFFF) {
        s_batch->first_matrix = (uint16_t)fb->frame.matrix_count;
    }
    if (fb->frame.matrix_count >= KJ_G3_MAX_MATRICES) {
        return -1;
    }
    fb->matrices[fb->frame.matrix_count++] = fb->pool[gid];
    s_palette_gid[count] = gid;
    s_batch->matrix_count = count + 1;
    return count;
}

static bool batch_matches_state(void)
{
    if (s_batch_ok) {
        return true;
    }
    s_batch_ok = s_batch != NULL && s_batch_key_attr == s_attr && s_batch_key_tex == s_tex_param &&
                 s_batch_key_pltt == s_tex_pltt && s_batch_key_vp == s_viewport;
    return s_batch_ok;
}

static inline int cached_slot(uint32_t gid)
{
    if (gid == s_slot_gid) {
        return s_slot_idx;
    }
    const int slot = palette_slot(gid);
    if (slot >= 0) {
        s_slot_gid = gid;
        s_slot_idx = slot;
    }
    return slot;
}

/* A vertex of the open batch for frame vertex `v`: vertices emitted before
 * the batch opened are copied into it with their matrix remapped. */
static bool local_vertex(uint32_t v, uint32_t *out)
{
    frame_buf *fb = build();

    if (v >= s_batch_first_vert) {
        *out = v;
        return true;
    }
    if (fb->frame.vert_count >= KJ_G3_MAX_VERTS) {
        return false;
    }
    const int slot = palette_slot(fb->vert_gmtx[v]);
    if (slot < 0) {
        return false;
    }
    const uint32_t n = fb->frame.vert_count++;
    fb->verts[n] = fb->verts[v];
    fb->verts[n].mtx = (uint8_t)slot;
    fb->vert_gmtx[n] = fb->vert_gmtx[v];
    *out = n;
    return true;
}

/* Emit one triangle of frame vertices (a, b, c), batching by material. */
static void emit_triangle(uint32_t a, uint32_t b, uint32_t c)
{
    frame_buf *fb = build();
    const unsigned alpha = (s_attr >> 16) & 31u;
    uint32_t la, lb, lc;

    if (alpha == 0) {
        /* Wireframe polygons: not supported by this backend yet. */
        ++fb->frame.unsupported;
        return;
    }
    if (fb->frame.index_count + 3 > KJ_G3_MAX_INDICES) {
        ++fb->frame.dropped_verts;
        return;
    }
    if (s_batch_ok && a >= s_batch_first_vert && b >= s_batch_first_vert && c >= s_batch_first_vert) {
        uint16_t *idx = fb->indices + fb->frame.index_count;
        idx[0] = (uint16_t)a;
        idx[1] = (uint16_t)b;
        idx[2] = (uint16_t)c;
        fb->frame.index_count += 3;
        s_batch->index_count += 3;
        return;
    }
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!batch_matches_state() && !open_batch()) {
            return;
        }
        if (local_vertex(a, &la) && local_vertex(b, &lb) && local_vertex(c, &lc)) {
            uint16_t *idx = fb->indices + fb->frame.index_count;
            idx[0] = (uint16_t)la;
            idx[1] = (uint16_t)lb;
            idx[2] = (uint16_t)lc;
            fb->frame.index_count += 3;
            s_batch->index_count += 3;
            return;
        }
        /* Palette or storage full: start a new batch and try once more. */
        close_batch();
    }
    ++fb->frame.dropped_verts;
}

static void submit_vertex(void)
{
    frame_buf *fb = build();
    uint32_t gid;

    if (!s_in_begin) {
        return;
    }
    if (fb->frame.vert_count >= KJ_G3_MAX_VERTS || !current_gmtx(&gid)) {
        ++fb->frame.dropped_verts;
        return;
    }
    if ((s_tex_param >> 30) == 3) {
        const int32_t src[3] = { s_coord[0], s_coord[1], s_coord[2] };
        texgen(src, 3);
    }
    if (!batch_matches_state() && !open_batch()) {
        return;
    }
    int slot = cached_slot(gid);
    if (slot < 0) {
        close_batch();
        if (!open_batch()) {
            return;
        }
        slot = cached_slot(gid);
        if (slot < 0) {
            ++fb->frame.dropped_verts;
            return;
        }
    }

    /* 16 bytes as four words: x|y, z|mtx, rgba, s|t (kj_g3_vertex layout) */
    const uint32_t n = fb->frame.vert_count++;
    uint32_t *w = (uint32_t *)&fb->verts[n];
    w[0] = (uint16_t)s_coord[0] | ((uint32_t)(uint16_t)s_coord[1] << 16);
    w[1] = (uint16_t)s_coord[2] | ((uint32_t)slot << 16);
    w[2] = s_color_rgba;
    w[3] = (uint16_t)s_texcoord[0] | ((uint32_t)(uint16_t)s_texcoord[1] << 16);
    fb->vert_gmtx[n] = (uint16_t)gid;
    s_slot_gid = gid;
    s_slot_idx = slot;

    /* Primitive assembly. */
    const uint32_t k = s_prim_count++;
    switch (s_primitive) {
    case 0: /* triangles */
        s_prim_verts[k % 3] = n;
        if (k % 3 == 2) {
            emit_triangle(s_prim_verts[0], s_prim_verts[1], s_prim_verts[2]);
        }
        break;
    case 1: /* quads */
        s_prim_verts[k % 4] = n;
        if (k % 4 == 3) {
            emit_triangle(s_prim_verts[0], s_prim_verts[1], s_prim_verts[2]);
            emit_triangle(s_prim_verts[0], s_prim_verts[2], s_prim_verts[3]);
        }
        break;
    case 2: /* triangle strip */
        if (k < 2) {
            s_prim_verts[k] = n;
        } else {
            if (s_strip_parity == 0) {
                emit_triangle(s_prim_verts[0], s_prim_verts[1], n);
            } else {
                emit_triangle(s_prim_verts[1], s_prim_verts[0], n);
            }
            s_strip_parity ^= 1;
            s_prim_verts[0] = s_prim_verts[1];
            s_prim_verts[1] = n;
        }
        break;
    case 3: /* quad strip: (0,1,3,2), (2,3,5,4), ... */
        if (k < 2) {
            s_prim_verts[k] = n;
        } else if (k % 2 == 0) {
            s_prim_verts[2] = n;
        } else {
            emit_triangle(s_prim_verts[0], s_prim_verts[1], n);
            emit_triangle(s_prim_verts[0], n, s_prim_verts[2]);
            s_prim_verts[0] = s_prim_verts[2];
            s_prim_verts[1] = n;
        }
        break;
    }
}

/* ------------------------------------------------------- results, tests */

static void put32(unsigned offset, uint32_t value)
{
    memcpy(NitroPort_IoReg + offset, &value, 4);
}

static uint32_t get32(unsigned offset)
{
    uint32_t v;
    memcpy(&v, NitroPort_IoReg + offset, 4);
    return v;
}

void NitroPort_Gx3dSyncResults(void)
{
    const mtx *c = clip_matrix();

    for (int i = 0; i < 16; ++i) {
        put32(0x640 + 4 * i, (uint32_t)c->m[i / 4][i % 4]);
    }
    for (int i = 0; i < 9; ++i) {
        put32(0x680 + 4 * i, (uint32_t)s_vec.m[i / 3][i % 3]);
    }
}

void NitroPort_Gx3dRefreshStatus(void)
{
    uint32_t stat = get32(0x600);

    stat &= ~((0x1Fu << 8) | (1u << 13) | (1u << 14) | 1u);
    stat |= (uint32_t)(s_posvec_sp & 31) << 8;
    stat |= (uint32_t)(s_proj_sp & 1) << 13;
    if (s_stack_error) {
        stat |= 1u << 15;
    }
    put32(0x600, stat);
    {
        const frame_buf *fb = &s_buf[s_build];
        const uint32_t polys = fb->frame.index_count / 3;
        const uint32_t verts = fb->frame.vert_count;
        put32(0x604, (polys > 2048 ? 2048 : polys) | ((verts > 6144 ? 6144 : verts) << 16));
    }
}

void NitroPort_Gx3dClearStackError(void)
{
    s_stack_error = false;
    s_proj_sp = 0;
    NitroPort_Gx3dRefreshStatus();
}

int NitroPort_Gx3dEarlierSceneUnread(void)
{
    return 0;
}

static void box_test(const uint32_t *p)
{
    const int32_t x = sign_bits(p[0], 16), y = sign_bits(p[0] >> 16, 16);
    const int32_t z = sign_bits(p[1], 16), w = sign_bits(p[1] >> 16, 16);
    const int32_t h = sign_bits(p[2], 16), d = sign_bits(p[2] >> 16, 16);
    const mtx *c = clip_matrix();
    unsigned all_out = 0x3F;
    /* The corners are sums of the box origin's row and the three edge
     * rows: 16 products instead of 128, same exact 64-bit sums (O3DS: 5,300
     * ticks per test, o3ds-log-0). */
    int64_t base[4], ex[4], ey[4], ez[4];
    for (int j = 0; j < 4; ++j) {
        base[j] = (int64_t)x * c->m[0][j] + (int64_t)y * c->m[1][j] + (int64_t)z * c->m[2][j] +
                  (int64_t)4096 * c->m[3][j];
        ex[j] = (int64_t)w * c->m[0][j];
        ey[j] = (int64_t)h * c->m[1][j];
        ez[j] = (int64_t)d * c->m[2][j];
    }

#ifndef HGSS_PROFILE_OFF
    ++s_stats.box_tests;
#endif
    for (int i = 0; i < 8; ++i) {
        int64_t o[4];
        for (int j = 0; j < 4; ++j) {
            int64_t v = base[j];
            if (i & 1) {
                v += ex[j];
            }
            if (i & 2) {
                v += ey[j];
            }
            if (i & 4) {
                v += ez[j];
            }
            o[j] = v >> 12;
        }
        /* Widescreen: the visible x range is 1.25x the DS's (o * 5 / 4,
         * truncating toward zero, without a 64-bit division call). */
        const int64_t w5 = o[3] * 5;
        const int64_t wx = s_wide ? (w5 + (w5 < 0 ? 3 : 0)) >> 2 : o[3];
        unsigned out = 0;
        out |= (o[0] < -wx) ? 0x01u : 0u;
        out |= (o[0] > wx) ? 0x02u : 0u;
        out |= (o[1] < -o[3]) ? 0x04u : 0u;
        out |= (o[1] > o[3]) ? 0x08u : 0u;
        out |= (o[2] < -o[3]) ? 0x10u : 0u;
        out |= (o[2] > o[3]) ? 0x20u : 0u;
        all_out &= out;
    }
    uint32_t stat = get32(0x600) & ~0x3u;
    frame_buf *fb = build();
    ++fb->frame.box_tests;
    if (all_out == 0) {
        stat |= 0x2u;
    } else {
#ifndef HGSS_PROFILE_OFF
        ++s_stats.box_culled;
#endif
        ++fb->frame.box_culled;
        if (all_out == 0x20u) {
            ++fb->frame.box_culled_far;
        }
    }
    put32(0x600, stat);
}

static void pos_test(const uint32_t *p)
{
    s_coord[0] = (int16_t)sign_bits(p[0], 16);
    s_coord[1] = (int16_t)sign_bits(p[0] >> 16, 16);
    s_coord[2] = (int16_t)sign_bits(p[1], 16);
    const mtx *c = clip_matrix();
    for (int j = 0; j < 4; ++j) {
        const int64_t o = ((int64_t)s_coord[0] * c->m[0][j] + (int64_t)s_coord[1] * c->m[1][j] +
                           (int64_t)s_coord[2] * c->m[2][j] + (int64_t)4096 * c->m[3][j]) >> 12;
        put32(0x620 + 4 * j, (uint32_t)o);
    }
}

static void vec_test(uint32_t p)
{
    int32_t in[3], out[3];
    for (int j = 0; j < 3; ++j) {
        in[j] = sign_bits(p >> (j * 10), 10) * 8;
    }
    transform_vector(in, &s_vec, out);
    for (int j = 0; j < 3; ++j) {
        /* 4.12 with the sign extended into bits 12-15 */
        uint16_t v = (uint16_t)out[j];
        memcpy(NitroPort_IoReg + 0x630 + 2 * j, &v, 2);
    }
}

/* ------------------------------------------------------------- swap */

static void swap_buffers(uint32_t param)
{
    frame_buf *fb = build();
    kj_g3_frame *f = &fb->frame;

    if (s_in_begin) {
        s_in_begin = false;
    }
    close_batch();
    f->verts = fb->verts;
    f->indices = fb->indices;
    f->matrices = fb->matrices;
    f->batches = fb->batches;
    f->swap_params = param;
    f->viewport = s_viewport;
    {
        uint16_t d3;
        memcpy(&d3, NitroPort_IoReg + 0x60, 2);
        f->disp3dcnt = d3;
    }
    f->alpha_test_ref = NitroPort_IoReg[0x340] & 31u;
    f->clear_color = get32(0x350);
    f->clear_depth = get32(0x354) & 0x7FFFu;
    f->fog_color = get32(0x358);
    memcpy(&f->fog_offset, NitroPort_IoReg + 0x35C, 2);
    memcpy(f->fog_table, NitroPort_IoReg + 0x360, 32);
    memcpy(f->edge_color, NitroPort_IoReg + 0x330, 16);
    memcpy(f->toon_table, NitroPort_IoReg + 0x380, 64);
    f->serial = ++s_stats.swaps;

    s_have_display = true;
    s_build ^= 1;

    fb = build();
    memset(&fb->frame, 0, sizeof(fb->frame));
    fb->pool_count = 0;
    s_batch = NULL;
    s_cur_gmtx_version = 0;
    s_prim_count = 0;
}

/* ------------------------------------------------------------- dispatch */

static int param_count(unsigned op)
{
    switch (op) {
    case 0x10: case 0x12: case 0x13: case 0x14:
    case 0x20: case 0x21: case 0x22: case 0x24: case 0x25: case 0x26: case 0x27: case 0x28:
    case 0x29: case 0x2A: case 0x2B: case 0x30: case 0x31: case 0x32: case 0x33:
    case 0x40: case 0x50: case 0x60: case 0x72:
        return 1;
    case 0x16: case 0x18:
        return 16;
    case 0x17: case 0x19:
        return 12;
    case 0x1A:
        return 9;
    case 0x1B: case 0x1C: case 0x70:
        return 3;
    case 0x23: case 0x71:
        return 2;
    case 0x34:
        return 32;
    default:
        return 0;
    }
}

#ifdef KJ_G3_PROFILE
extern u64 svcGetSystemTick(void);
static u64 s_op_ticks[256];
static u32 s_op_calls[256];
static void execute_inner(unsigned op, const uint32_t *p);
static void execute(unsigned op, const uint32_t *p)
{
    const u64 t0 = svcGetSystemTick();
    execute_inner(op, p);
    s_op_ticks[op & 255] += svcGetSystemTick() - t0;
    ++s_op_calls[op & 255];
}

void kj_g3_log_op_profile(void)
{
    for (int i = 0; i < 256; ++i) {
        if (s_op_calls[i]) {
            NitroPort_Log("[g3prof] op %02x calls %lu ticks %llu (%llu/call)", i,
                          (unsigned long)s_op_calls[i], (unsigned long long)s_op_ticks[i],
                          (unsigned long long)(s_op_ticks[i] / s_op_calls[i]));
        }
    }
    memset(s_op_ticks, 0, sizeof(s_op_ticks));
    memset(s_op_calls, 0, sizeof(s_op_calls));
}

static void execute_inner(unsigned op, const uint32_t *p)
#else
void kj_g3_log_op_profile(void)
{
}

static void execute(unsigned op, const uint32_t *p)
#endif
{
#ifndef HGSS_PROFILE_OFF
    ++s_stats.commands;
#endif
#ifdef KJ_G3_ZERO_TRACE
    {
        /* Diagnostics: report the first commands that leave a zero matrix. */
        static unsigned reported;
        static const mtx zero;
        const bool pos_was = memcmp(&s_pos, &zero, sizeof(zero)) == 0;
        const bool proj_was = memcmp(&s_proj, &zero, sizeof(zero)) == 0;
        execute_body(op, p);
        const bool pos_is = memcmp(&s_pos, &zero, sizeof(zero)) == 0;
        const bool proj_is = memcmp(&s_proj, &zero, sizeof(zero)) == 0;
        if (((pos_is && !pos_was) || (proj_is && !proj_was)) && reported < 64) {
            ++reported;
            NitroPort_Log("[g3zero] op %02x p0 %08lx mode %d sp %d -> pos %d proj %d (swap %lu)", op,
                          (unsigned long)p[0], s_mtx_mode, s_posvec_sp, pos_is, proj_is,
                          (unsigned long)s_stats.swaps);
        }
        return;
    }
}

static void execute_body(unsigned op, const uint32_t *p)
{
#endif
    switch (op) {
    case 0x10:
        s_mtx_mode = (int)(p[0] & 3);
        break;
    case 0x11: case 0x12: case 0x13: case 0x14:
        stack_op(op, p[0]);
        break;
    case 0x15: {
        mtx m;
        mtx_identity(&m);
        load_matrix(&m);
        break;
    }
    case 0x16: case 0x17: {
        mtx m;
        read_matrix(&m, op, p);
        load_matrix(&m);
        break;
    }
    case 0x18: case 0x19: case 0x1A: {
        mtx m;
        read_matrix(&m, op, p);
        mult_matrix(&m, false);
        break;
    }
    case 0x1B:
        scale_matrix(p);
        break;
    case 0x1C:
        translate_matrix(p);
        break;
    case 0x20:
        s_color = (uint16_t)(p[0] & 0x7FFF);
        update_color_rgba();
        break;
    case 0x21:
        if ((s_tex_param >> 30) == 2) {
            const int32_t src[3] = { sign_bits(p[0], 10) * 8, sign_bits(p[0] >> 10, 10) * 8,
                                     sign_bits(p[0] >> 20, 10) * 8 };
            texgen(src, 2);
        }
        if (s_attr & 0xF) {
            s_color = shade(p[0]);
            update_color_rgba();
        }
        break;
    case 0x22:
        s_texcoord_raw[0] = sign_bits(p[0], 16);
        s_texcoord_raw[1] = sign_bits(p[0] >> 16, 16);
        if ((s_tex_param >> 30) == 1) {
            texgen(s_texcoord_raw, 1);
        } else {
            s_texcoord[0] = s_texcoord_raw[0];
            s_texcoord[1] = s_texcoord_raw[1];
        }
        break;
    case 0x23:
        s_coord[0] = (int16_t)sign_bits(p[0], 16);
        s_coord[1] = (int16_t)sign_bits(p[0] >> 16, 16);
        s_coord[2] = (int16_t)sign_bits(p[1], 16);
        submit_vertex();
        break;
    case 0x24:
        for (int i = 0; i < 3; ++i) {
            s_coord[i] = (int16_t)(sign_bits(p[0] >> (i * 10), 10) * 64);
        }
        submit_vertex();
        break;
    case 0x25:
        s_coord[0] = (int16_t)sign_bits(p[0], 16);
        s_coord[1] = (int16_t)sign_bits(p[0] >> 16, 16);
        submit_vertex();
        break;
    case 0x26:
        s_coord[0] = (int16_t)sign_bits(p[0], 16);
        s_coord[2] = (int16_t)sign_bits(p[0] >> 16, 16);
        submit_vertex();
        break;
    case 0x27:
        s_coord[1] = (int16_t)sign_bits(p[0], 16);
        s_coord[2] = (int16_t)sign_bits(p[0] >> 16, 16);
        submit_vertex();
        break;
    case 0x28:
        for (int i = 0; i < 3; ++i) {
            s_coord[i] = (int16_t)(s_coord[i] + sign_bits(p[0] >> (i * 10), 10));
        }
        submit_vertex();
        break;
    case 0x29:
        s_attr_pending = p[0];
        break;
    case 0x2A:
        if (s_tex_param != p[0]) {
            s_tex_param = p[0];
            material_changed();
        }
        break;
    case 0x2B:
        if (s_tex_pltt != (p[0] & 0x1FFF)) {
            s_tex_pltt = p[0] & 0x1FFF;
            material_changed();
        }
        break;
    case 0x30: {
        /* NNS re-sends the material per object: invalidate shading only on
         * a real change. */
        const uint16_t dif = (uint16_t)(p[0] & 0x7FFF), amb = (uint16_t)((p[0] >> 16) & 0x7FFF);
        if (dif != s_diffuse || amb != s_ambient) {
            s_diffuse = dif;
            s_ambient = amb;
            normals_changed();
        }
        if (p[0] & 0x8000) {
            s_color = s_diffuse;
            update_color_rgba();
        }
        break;
    }
    case 0x31: {
        const uint16_t spe = (uint16_t)(p[0] & 0x7FFF), emi = (uint16_t)((p[0] >> 16) & 0x7FFF);
        const bool shin = (p[0] & 0x8000) != 0;
        if (spe != s_specular || emi != s_emission || shin != s_use_shininess) {
            s_specular = spe;
            s_emission = emi;
            s_use_shininess = shin;
            normals_changed();
        }
        break;
    }
    case 0x32: {
        const unsigned i = (p[0] >> 30) & 3;
        int32_t in[3];
        for (int j = 0; j < 3; ++j) {
            in[j] = sign_bits(p[0] >> (j * 10), 10) * 8;
        }
        int32_t dir[3];
        transform_vector(in, &s_vec, dir);
        if (dir[0] != s_light_dir[i][0] || dir[1] != s_light_dir[i][1] || dir[2] != s_light_dir[i][2]) {
            s_light_dir[i][0] = dir[0];
            s_light_dir[i][1] = dir[1];
            s_light_dir[i][2] = dir[2];
            s_light_half[i][0] = dir[0] / 2;
            s_light_half[i][1] = dir[1] / 2;
            s_light_half[i][2] = (dir[2] - 4096) / 2;
            normals_changed();
        }
        break;
    }
    case 0x33: {
        const uint16_t c = (uint16_t)(p[0] & 0x7FFF);
        if (s_light_color[(p[0] >> 30) & 3] != c) {
            s_light_color[(p[0] >> 30) & 3] = c;
            normals_changed();
        }
        break;
    }
    case 0x34:
        if (memcmp(s_shininess, p, sizeof(s_shininess)) != 0) {
            memcpy(s_shininess, p, sizeof(s_shininess));
            normals_changed();
        }
        break;
    case 0x40:
        if (s_attr != s_attr_pending) {
            if ((s_attr ^ s_attr_pending) & 0xFu) {
                normals_changed(); /* light enables: all shade() reads of attr */
            }
            s_attr = s_attr_pending;
            material_changed();
            update_color_rgba();
        }
        s_primitive = (int)(p[0] & 3);
        s_in_begin = true;
        s_prim_count = 0;
        s_strip_parity = 0;
        break;
    case 0x41:
        s_in_begin = false;
        break;
    case 0x50:
        swap_buffers(p[0]);
        break;
    case 0x60:
        if (s_viewport != p[0]) {
            s_viewport = p[0];
            material_changed();
        }
        break;
    case 0x70:
        box_test(p);
        break;
    case 0x71:
        pos_test(p);
        break;
    case 0x72:
        vec_test(p[0]);
        break;
    default:
        break;
    }
}

extern u64 svcGetSystemTick(void);
static u64 s_fifo_ticks;
static u32 s_fifo_words, s_direct_words;

/* Direct writes: parameters arrive one word per call. */
static unsigned s_direct_op;
static uint32_t s_direct_params[32];
static int s_direct_have;

void NitroPort_GxDirectWrite(int opcode, uint32_t value)
{
    const unsigned op = (unsigned)opcode & 0xFF;
    const int need = param_count(op);

#ifndef HGSS_PROFILE_OFF
    ++s_direct_words;
#endif
    if (need == 0) {
        execute(op, &value);
        return;
    }
    if (s_direct_op != op) {
        s_direct_op = op;
        s_direct_have = 0;
    }
    s_direct_params[s_direct_have++] = value;
    if (s_direct_have >= need) {
        execute(op, s_direct_params);
        s_direct_have = 0;
        s_direct_op = 0;
    }
}

/* Packed GXFIFO: a word of up to four opcodes (low byte first), then the
 * parameters of each in turn. */
static uint32_t s_fifo_cmds;     /* remaining packed opcodes */
static int s_fifo_left;          /* opcodes left in the packed word */
static unsigned s_fifo_op;
static int s_fifo_need, s_fifo_have;
static uint32_t s_fifo_params[32];

static void fifo_command(unsigned op, const uint32_t *p);

static void fifo_next_command(void)
{
    while (s_fifo_left > 0) {
        s_fifo_op = s_fifo_cmds & 0xFF;
        s_fifo_cmds >>= 8;
        --s_fifo_left;
        s_fifo_need = param_count(s_fifo_op);
        s_fifo_have = 0;
        if (s_fifo_need > 0) {
            return;
        }
        if (s_fifo_op != 0) {
            fifo_command(s_fifo_op, s_fifo_params);
        }
        if ((s_fifo_cmds & 0xFFFFFF) == 0) {
            /* the rest of the word is NOPs */
            s_fifo_left = 0;
        }
    }
    s_fifo_need = 0;
}

void NitroPort_GxFifoWord(uint32_t value)
{
    if (s_fifo_need > 0) {
        s_fifo_params[s_fifo_have++] = value;
        if (s_fifo_have >= s_fifo_need) {
            fifo_command(s_fifo_op, s_fifo_params);
            s_fifo_need = 0;
            fifo_next_command();
        }
        return;
    }
    s_fifo_cmds = value;
    s_fifo_left = 4;
    fifo_next_command();
}


static uint8_t s_param_count[256];

/* Vertex fast path for the bulk decoder: everything the general path
 * (submit_vertex) checks is already settled -- inside BEGIN, the open batch
 * matches the material, the current clip matrix has a palette slot in it,
 * no per-vertex texgen, room for the vertex and a quad's indices. Writes
 * the vertex and assembles the primitive inline. Returns false to make the
 * caller take the general path. Kept small: it runs ~14k times a frame in
 * Goldenrod (ARM11 L1 I-cache: coffeepot icachebench). */
static bool fast_vertex(void)
{
    if (!s_in_begin || !s_batch_ok || s_cur_gmtx_version != s_clip_version ||
        s_slot_gid != s_cur_gmtx || (s_tex_param >> 30) == 3) {
        return false;
    }
    frame_buf *fb = &s_buf[s_build];
    const uint32_t n = fb->frame.vert_count;
    uint32_t ic = fb->frame.index_count;
    if (n >= KJ_G3_MAX_VERTS || ic + 6 > KJ_G3_MAX_INDICES || ((s_attr >> 16) & 31u) == 0) {
        return false;
    }
    uint32_t *w = (uint32_t *)&fb->verts[n];
    w[0] = (uint16_t)s_coord[0] | ((uint32_t)(uint16_t)s_coord[1] << 16);
    w[1] = (uint16_t)s_coord[2] | ((uint32_t)s_slot_idx << 16);
    w[2] = s_color_rgba;
    w[3] = (uint16_t)s_texcoord[0] | ((uint32_t)(uint16_t)s_texcoord[1] << 16);
    fb->vert_gmtx[n] = (uint16_t)s_cur_gmtx;
    fb->frame.vert_count = n + 1;

    const uint32_t k = s_prim_count++;
    const uint32_t first = s_batch_first_vert;
    uint16_t *idx = fb->indices + ic;
    uint32_t a, b, c, d;
    switch (s_primitive) {
    case 0:
        s_prim_verts[k % 3] = n;
        if (k % 3 != 2) {
            return true;
        }
        a = s_prim_verts[0];
        b = s_prim_verts[1];
        if (a < first || b < first) {
            break;
        }
        idx[0] = (uint16_t)a;
        idx[1] = (uint16_t)b;
        idx[2] = (uint16_t)n;
        ic += 3;
        goto done;
    case 1:
        s_prim_verts[k & 3] = n;
        if ((k & 3) != 3) {
            return true;
        }
        a = s_prim_verts[0];
        b = s_prim_verts[1];
        c = s_prim_verts[2];
        if (a < first || b < first || c < first) {
            break;
        }
        idx[0] = (uint16_t)a;
        idx[1] = (uint16_t)b;
        idx[2] = (uint16_t)c;
        idx[3] = (uint16_t)a;
        idx[4] = (uint16_t)c;
        idx[5] = (uint16_t)n;
        ic += 6;
        goto done;
    case 2:
        if (k < 2) {
            s_prim_verts[k] = n;
            return true;
        }
        a = s_prim_verts[0];
        b = s_prim_verts[1];
        s_prim_verts[0] = b;
        s_prim_verts[1] = n;
        if (a < first || b < first) {
            if (s_strip_parity == 0) {
                emit_triangle(a, b, n);
            } else {
                emit_triangle(b, a, n);
            }
            s_strip_parity ^= 1;
            return true;
        }
        if (s_strip_parity == 0) {
            idx[0] = (uint16_t)a;
            idx[1] = (uint16_t)b;
        } else {
            idx[0] = (uint16_t)b;
            idx[1] = (uint16_t)a;
        }
        idx[2] = (uint16_t)n;
        s_strip_parity ^= 1;
        ic += 3;
        goto done;
    default:
        if (k < 2) {
            s_prim_verts[k] = n;
            return true;
        }
        if ((k & 1) == 0) {
            s_prim_verts[2] = n;
            return true;
        }
        a = s_prim_verts[0];
        b = s_prim_verts[1];
        d = s_prim_verts[2];
        s_prim_verts[0] = d;
        s_prim_verts[1] = n;
        if (a < first || b < first || d < first) {
            emit_triangle(a, b, n);
            emit_triangle(a, n, d);
            return true;
        }
        idx[0] = (uint16_t)a;
        idx[1] = (uint16_t)b;
        idx[2] = (uint16_t)n;
        idx[3] = (uint16_t)a;
        idx[4] = (uint16_t)n;
        idx[5] = (uint16_t)d;
        ic += 6;
        goto done;
    }
    /* triangles/quads touching vertices from before the batch: general path */
    if (s_primitive == 0) {
        emit_triangle(s_prim_verts[0], s_prim_verts[1], n);
    } else {
        emit_triangle(s_prim_verts[0], s_prim_verts[1], s_prim_verts[2]);
        emit_triangle(s_prim_verts[0], s_prim_verts[2], n);
    }
    return true;
done:
    s_batch->index_count += ic - fb->frame.index_count;
    fb->frame.index_count = ic;
    return true;
}

/* Decode one command of the bulk path, with vertex/texcoord fast paths.
 * Every vertex opcode only sets coordinates and shares one vertex tail, so
 * fast_vertex is emitted once: the decode loop has to stay inside the
 * ARM11's 16 KiB I-cache (coffeepot cpu-optimization.md, icachebench: CPI
 * 1.1 up to 16 KiB, 5.6 from 32 KiB). With the fast path inlined into
 * each opcode and both decode branches this function was 14.6 KiB. */
static inline __attribute__((always_inline)) void hot_command(unsigned op, const uint32_t *p)
{
    uint32_t v = p[0];
    switch (op) {
    case 0x22:
        if ((s_tex_param >> 30) != 0) {
            break;
        }
        s_texcoord[0] = s_texcoord_raw[0] = (int16_t)v;
        s_texcoord[1] = s_texcoord_raw[1] = (int16_t)(v >> 16);
        return;
    case 0x23:
        s_coord[0] = (int16_t)v;
        s_coord[1] = (int16_t)(v >> 16);
        s_coord[2] = (int16_t)p[1];
        goto vertex;
    case 0x24: /* VTX_10 */
        s_coord[0] = (int16_t)(sign_bits(v, 10) * 64);
        s_coord[1] = (int16_t)(sign_bits(v >> 10, 10) * 64);
        s_coord[2] = (int16_t)(sign_bits(v >> 20, 10) * 64);
        goto vertex;
    case 0x25:
        s_coord[0] = (int16_t)v;
        s_coord[1] = (int16_t)(v >> 16);
        goto vertex;
    case 0x26:
        s_coord[0] = (int16_t)v;
        s_coord[2] = (int16_t)(v >> 16);
        goto vertex;
    case 0x27:
        s_coord[1] = (int16_t)v;
        s_coord[2] = (int16_t)(v >> 16);
        goto vertex;
    case 0x28: /* VTX_DIFF */
        s_coord[0] = (int16_t)(s_coord[0] + sign_bits(v, 10));
        s_coord[1] = (int16_t)(s_coord[1] + sign_bits(v >> 10, 10));
        s_coord[2] = (int16_t)(s_coord[2] + sign_bits(v >> 20, 10));
        goto vertex;
    case 0x21: /* NORMAL without normal texgen */
        if ((s_tex_param >> 30) == 2) {
            break;
        }
        if (s_attr & 0xF) {
            s_color = shade(v);
            update_color_rgba();
        }
        return;
    default:
        break;
    }
    execute(op, p);
    return;
vertex:
    if (!fast_vertex()) {
        submit_vertex();
    }
}

/* A command completed across FIFO calls still has exactly the same vertex
 * state as a bulk command. Reuse its fast path instead of forcing every
 * split vertex through submit_vertex and the general dispatcher. */
static void fifo_command(unsigned op, const uint32_t *p)
{
    hot_command(op, p);
}

static void build_param_table(void)
{
    for (unsigned op = 0; op < 256; ++op) {
        s_param_count[op] = (uint8_t)param_count(op);
    }
    s_param_count[0] = 0;
}

/* Bulk packed-FIFO decode (DMA'd display lists): parameters are passed to
 * execute() in place, without per-word calls or copies. A command whose
 * parameters run past the end of the buffer goes through the word-at-a-time
 * path, which also finishes any command left open by an earlier call. */
static u32 s_fifo_calls;


/* ------------------------------------------------- retained display lists */

/*
 * Retained display lists (OPT-003), event replay.
 *
 * NNS DMAs each model's command lists every frame; static scenery sends the
 * same words with the same entry state. A list is recorded once as a
 * sequence of events:
 *   STATE  a command that is not vertex data (matrix, material, BEGIN/END,
 *          tests, viewport): re-executed on replay -- cheap, and it keeps
 *          matrices, batches and game-visible results live;
 *   BLOCK  the vertices and triangle indices a run of vertex-data commands
 *          (COLOR, NORMAL, TEXCOORD, VTX_*) produced: appended on replay
 *          with the current clip matrix's palette slot.
 * Replay requires an exact word comparison with the recorded list and an
 * identical entry state (every input vertex data reads). A list whose
 * vertex data depends on matrices (lit normals, normal/vertex texgen) and
 * that changes matrices itself is never recorded, nor is anything that
 * swaps buffers, straddles a call boundary, or shares vertices between
 * blocks (skinned strips).
 */

#define DL_SLOTS 1024u
#define DL_MIN_WORDS 16u
#define DL_BYTES_MAX (6u * 1024u * 1024u)
#define DL_MAX_EVENTS 4096u

typedef struct dl_key {
    uint32_t attr_pending, attr, tex_param, tex_pltt, viewport;
    uint32_t color, coord01, coord2;
    int32_t texcoord[2], texcoord_raw[2];
    uint32_t mtx_mode;
    int32_t texm[4][4];
    int32_t vec[3][3];
    int32_t light_dir[4][3], light_half[4][3];
    uint32_t light_color[4];
    uint32_t material[4];
    uint32_t use_shininess;
    uint32_t shininess[32];
} dl_key;

typedef struct dl_exit {
    uint16_t color;
    int16_t coord[3];
    int32_t texcoord[2], texcoord_raw[2];
    bool in_begin;
    int primitive;
    uint32_t prim_count, strip_parity;
    uint32_t prim_rel[4];
    bool prim_valid;
} dl_exit;

/* event: high bit set = BLOCK (index into blocks), else STATE (word offset) */
#define DL_EV_BLOCK 0x80000000u

typedef struct dl_block {
    uint32_t first_vert, nverts; /* into the entry's vertex array */
    uint32_t first_idx, nidx;    /* relative to the block's first vertex */
} dl_block;

/* Unrecordable lists recheck their content once per this many pictures */
#define DL_NEVER_RECHECK_MASK 63u

typedef struct dl_entry {
    const uint32_t *ptr;
    uint32_t count;
    bool recorded;
    bool never;        /* proven unrecordable for this content */
    bool needs_light;  /* seen lighting its normals: key on lighting state */
    uint32_t reads;    /* DL_READS_*: entry inputs the list consumes */
    uint32_t *words;
    dl_key key;
    bool lit, texgen;
    dl_exit exit;
    uint32_t *events;
    uint32_t nevents;
    dl_block *blocks;
    uint32_t nblocks;
    kj_g3_vertex *verts;
    uint32_t nverts;
    uint16_t *idx;
    uint32_t nidx;
    uint32_t cap_events, cap_blocks, cap_verts, cap_idx; /* storage kept across re-recordings */
    uint32_t bytes;
    uint32_t last_frame;
} dl_entry;

/* Recording state (one list at a time; lists do not nest). */
typedef struct dl_rec {
    bool active, bad, has_matrix, vecdep;
    const uint32_t *base;
    uint32_t events[DL_MAX_EVENTS];
    uint32_t nevents;
    dl_block blocks[DL_MAX_EVENTS];
    uint32_t nblocks;
    uint32_t block_v0, block_i0, block_b0; /* frame positions at block start */
    uint32_t v0, i0;                       /* frame positions at list start */
} dl_rec;

static dl_entry s_dl[DL_SLOTS];

/* All recordings live in one bump arena, flushed wholesale when full: no
 * per-list heap traffic (thousands of small malloc/free pairs a frame), and
 * no per-entry ownership to get wrong. */
static uint8_t *s_dl_arena;
static uint32_t s_dl_arena_used;
static uint32_t s_dl_flushes;

static void *dl_alloc(uint32_t n)
{
    if (s_dl_arena == NULL) {
        s_dl_arena = malloc(DL_BYTES_MAX);
        if (s_dl_arena == NULL) {
            return NULL;
        }
    }
    n = (n + 3u) & ~3u;
    if (n == 0 || s_dl_arena_used + n > DL_BYTES_MAX) {
        return NULL;
    }
    void *ptr = s_dl_arena + s_dl_arena_used;
    s_dl_arena_used += n;
    return ptr;
}

static void dl_flush_all(void)
{
    memset(s_dl, 0, sizeof(s_dl));
    s_dl_arena_used = 0;
#ifndef HGSS_PROFILE_OFF
    ++s_dl_flushes;
#endif
}
static dl_rec s_rec;
static uint32_t s_dl_bytes;
static uint32_t s_dl_hits, s_dl_misses, s_dl_uncacheable;
/* Word coverage of the cache: short lists, a new slot, changed contents,
 * proven uncacheable, replayed, recorded but missed on the entry key. */
enum { DLC_SHORT, DLC_SMALL, DLC_BEGIN, DLC_BEGIN_FIRST, DLC_SPLIT, DLC_NEW, DLC_CHANGED, DLC_NEVER, DLC_HIT, DLC_KEY, DLC_COUNT };
static uint32_t s_dl_cov[DLC_COUNT];
static uint32_t s_dl_why[6]; /* batch-in-block, shared-verts, straddle/drops, lit+matrix, swap, open-prim */

/* Close the vertex block in progress (if it produced anything). */
static void rec_close_block(void)
{
    frame_buf *fb = &s_buf[s_build];
    const uint32_t v1 = fb->frame.vert_count, i1 = fb->frame.index_count;

    if (v1 == s_rec.block_v0 && i1 == s_rec.block_i0) {
        return;
    }
    const uint32_t opened = fb->frame.batch_count - s_rec.block_b0;
    if (opened > 1u || (opened == 1u && fb->batches[fb->frame.batch_count - 1].first_index != s_rec.block_i0) ||
        s_rec.nevents >= DL_MAX_EVENTS || s_rec.nblocks >= DL_MAX_EVENTS) {
        s_rec.bad = true; /* batches changed inside the block, or too many events */
#ifndef HGSS_PROFILE_OFF
        ++s_dl_why[0];
#endif
    }
    for (uint32_t i = s_rec.block_i0; i < i1 && !s_rec.bad; ++i) {
        if (fb->indices[i] < s_rec.block_v0) {
            s_rec.bad = true; /* shares vertices with an earlier block */
#ifndef HGSS_PROFILE_OFF
            ++s_dl_why[1];
#endif
        }
    }
    if (!s_rec.bad) {
        dl_block *b = &s_rec.blocks[s_rec.nblocks];
        b->first_vert = s_rec.block_v0 - s_rec.v0;
        b->nverts = v1 - s_rec.block_v0;
        b->first_idx = s_rec.block_i0 - s_rec.i0;
        b->nidx = i1 - s_rec.block_i0;
        s_rec.events[s_rec.nevents++] = DL_EV_BLOCK | s_rec.nblocks++;
    }
}

static void rec_open_block(void)
{
    frame_buf *fb = &s_buf[s_build];
    s_rec.block_v0 = fb->frame.vert_count;
    s_rec.block_i0 = fb->frame.index_count;
    s_rec.block_b0 = fb->frame.batch_count;
}

/* Called by decode_words before each command while recording. */
static void rec_command(unsigned op, const uint32_t *p)
{
    if (op >= 0x20 && op <= 0x28) {
        if (op == 0x21 && (s_attr & 0xFu)) {
            s_rec.vecdep = true;
        }
        if ((s_tex_param >> 30) >= 2u) {
            s_rec.vecdep = true;
        }
        if (s_rec.nblocks == 0 && s_rec.nevents == 0) {
            /* first vertex-data command opens the first block lazily */
        }
        return;
    }
    if (op == 0x50) {
        s_rec.bad = true;
#ifndef HGSS_PROFILE_OFF
        ++s_dl_why[4];
#endif
    }
    if (op >= 0x10 && op <= 0x1C) {
        s_rec.has_matrix = true;
    }
    /* a state command ends the current block; the block after it starts
     * once this command has run (its effect on batches belongs to it) */
    rec_close_block();
    if (s_rec.nevents >= DL_MAX_EVENTS) {
        s_rec.bad = true;
        return;
    }
    s_rec.events[s_rec.nevents++] = (uint32_t)(p - s_rec.base) | ((uint32_t)op << 20);
}


/* Which entry inputs a list reads before it writes them (scan in order). */
#define DL_READS_COLOR 1u
#define DL_READS_COORD 2u
#define DL_READS_TEXCOORD 4u
#define DL_READS_TEXM 8u

static uint32_t dl_entry_reads(const uint32_t *w, size_t n)
{
    uint32_t reads = 0, written = 0;
    size_t i = 0;
    while (i < n) {
        uint32_t cmds = w[i++];
        for (int k = 0; k < 4; ++k, cmds >>= 8) {
            const unsigned op = cmds & 0xFF;
            switch (op) {
            case 0x20:
                /* NORMAL (0x21) writes the colour only when lighting is on,
                 * which can change inside the list: not counted as a write */
                written |= DL_READS_COLOR;
                break;
            case 0x22:
                written |= DL_READS_TEXCOORD;
                break;
            case 0x23: case 0x24:
                reads |= ~written & (DL_READS_COLOR | DL_READS_TEXCOORD);
                written |= DL_READS_COORD;
                break;
            case 0x25: case 0x26: case 0x27: case 0x28:
                reads |= ~written & (DL_READS_COLOR | DL_READS_TEXCOORD | DL_READS_COORD);
                written |= DL_READS_COORD;
                break;
            case 0x2A: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
            case 0x16: case 0x17: case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C:
                reads |= DL_READS_TEXM; /* texgen or texture matrix may change inside */
                break;
            default:
                break;
            }
            i += s_param_count[op];
        }
    }
    return reads;
}

/* The unused tail is not part of the key. Most map lists consume neither
 * texgen matrices nor shininess; clearing/comparing those bytes on every
 * cache lookup needlessly streams through the ARM11's small data cache.
 * All bytes in the selected prefix are initialized, including padding. */
static size_t dl_key_size(bool lit, bool texgen, uint32_t reads)
{
    if (lit) {
        return s_use_shininess ? sizeof(dl_key) : offsetof(dl_key, shininess);
    }
    return texgen || (reads & DL_READS_TEXM) ? offsetof(dl_key, vec)
                                          : offsetof(dl_key, texm);
}

static void dl_make_key(dl_key *k, bool lit, bool texgen, uint32_t reads)
{
    memset(k, 0, dl_key_size(lit, texgen, reads));
    k->attr_pending = s_attr_pending;
    k->attr = s_attr;
    k->tex_param = s_tex_param;
    k->tex_pltt = s_tex_pltt;
    k->viewport = s_viewport;
    if (reads & DL_READS_COLOR) {
        k->color = s_color;
    }
    if (reads & DL_READS_COORD) {
        k->coord01 = (uint16_t)s_coord[0] | ((uint32_t)(uint16_t)s_coord[1] << 16);
        k->coord2 = (uint16_t)s_coord[2];
    }
    if (reads & DL_READS_TEXCOORD) {
        k->texcoord[0] = s_texcoord[0];
        k->texcoord[1] = s_texcoord[1];
        k->texcoord_raw[0] = s_texcoord_raw[0];
        k->texcoord_raw[1] = s_texcoord_raw[1];
    }
    k->mtx_mode = (uint32_t)s_mtx_mode;
    if (texgen || (reads & DL_READS_TEXM)) {
        memcpy(k->texm, s_tex.m, sizeof(k->texm));
    }
    if (lit) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                k->vec[i][j] = s_vec.m[i][j];
            }
        }
        memcpy(k->light_dir, s_light_dir, sizeof(k->light_dir));
        memcpy(k->light_half, s_light_half, sizeof(k->light_half));
        for (int i = 0; i < 4; ++i) {
            k->light_color[i] = s_light_color[i];
        }
        k->material[0] = s_diffuse;
        k->material[1] = s_ambient;
        k->material[2] = s_specular;
        k->material[3] = s_emission;
        k->use_shininess = s_use_shininess;
        if (s_use_shininess) {
            memcpy(k->shininess, s_shininess, sizeof(k->shininess));
        }
    }
}

static dl_entry *dl_slot(const uint32_t *ptr, uint32_t count)
{
    const uint32_t h = (uint32_t)(((uintptr_t)ptr >> 2) * 2654435761u ^ count * 40503u);
    uint32_t i = h & (DL_SLOTS - 1u);
    dl_entry *victim = NULL;
    for (uint32_t n = 0; n < 8; ++n, i = (i + 1u) & (DL_SLOTS - 1u)) {
        dl_entry *e = &s_dl[i];
        if ((e->ptr == ptr && e->count == count) || e->ptr == NULL) {
            return e;
        }
        if (victim == NULL || e->last_frame < victim->last_frame) {
            victim = e;
        }
    }
    return victim;
}

/* The recording's storage stays with the entry (capacities in cap_*): a list
 * re-recorded every frame or two (keyed on changing entry state) reuses it
 * instead of filling the arena, whose flush re-records everything in one
 * frame (Goldenrod: 29.6 ms logic against 13.7, twice per 600 frames). */
static void dl_drop_recording(dl_entry *e)
{
    e->nevents = e->nblocks = e->nverts = e->nidx = 0;
    e->recorded = false;
}

/* Room for need elements in *ptr: the existing storage if large enough. */
static bool dl_reserve(void **ptr, uint32_t *cap, uint32_t need, uint32_t elem)
{
    if (need <= *cap) {
        return true;
    }
    void *p = dl_alloc(need * elem);
    if (p == NULL) {
        return false;
    }
    *ptr = p;
    *cap = need;
    return true;
}

static void dl_free(dl_entry *e)
{
    memset(e, 0, sizeof(*e));
}

static void dl_capture_exit(dl_exit *x, uint32_t v0)
{
    x->color = s_color;
    memcpy(x->coord, s_coord, sizeof(x->coord));
    x->texcoord[0] = s_texcoord[0];
    x->texcoord[1] = s_texcoord[1];
    x->texcoord_raw[0] = s_texcoord_raw[0];
    x->texcoord_raw[1] = s_texcoord_raw[1];
    x->in_begin = s_in_begin;
    x->primitive = s_primitive;
    x->prim_count = s_prim_count;
    x->strip_parity = s_strip_parity;
    x->prim_valid = true;
    for (int i = 0; i < 4; ++i) {
        x->prim_rel[i] = s_prim_verts[i] - v0;
        if (x->in_begin && s_prim_verts[i] < v0) {
            x->prim_valid = false;
        }
    }
}

static void decode_words_impl(const uint32_t *words, size_t count);

#ifndef HGSS_PROFILE_OFF
/* Sample one FIFO call in 64. Nested portions are timed only in sampled
 * calls; the remainder includes lookup, comparison, recording and probe
 * overhead. Wall time includes preemption. */
static u32 s_coarse_seq = 0x6d2b79f5u;
static u32 s_coarse_n, s_coarse_depth;
static bool s_coarse_sample;
static u64 s_coarse_total, s_coarse_decode, s_coarse_replay, s_coarse_copy;
#endif


static bool dl_append_block(const dl_entry *e, const dl_block *b, uint32_t *v_first_out)
{
    frame_buf *fb = &s_buf[s_build];
    uint32_t gid;

    if (!current_gmtx(&gid)) {
        return false;
    }
    if (!batch_matches_state() && !open_batch()) {
        return false;
    }
    int slot = cached_slot(gid);
    if (slot < 0) {
        close_batch();
        if (!open_batch()) {
            return false;
        }
        slot = cached_slot(gid);
        if (slot < 0) {
            return false;
        }
    }
#ifndef HGSS_PROFILE_OFF
    const u64 copy_start = s_coarse_sample ? svcGetSystemTick() : 0;
#endif
    const uint32_t v0 = fb->frame.vert_count;
    const uint32_t hi = (uint32_t)slot << 16;
    const uint32_t *src = (const uint32_t *)(e->verts + b->first_vert);
    uint32_t *dst = (uint32_t *)&fb->verts[v0];
    for (uint32_t i = 0; i < b->nverts; ++i) {
        dst[0] = src[0];
        dst[1] = (src[1] & 0xFFFFu) | hi;
        dst[2] = src[2];
        dst[3] = src[3];
        fb->vert_gmtx[v0 + i] = (uint16_t)gid;
        src += 4;
        dst += 4;
    }
    const uint16_t *si = e->idx + b->first_idx;
    uint16_t *ix = fb->indices + fb->frame.index_count;
    for (uint32_t i = 0; i < b->nidx; ++i) {
        ix[i] = (uint16_t)(si[i] + v0);
    }
    fb->frame.vert_count = v0 + b->nverts;
    fb->frame.index_count += b->nidx;
    s_batch->index_count += b->nidx;
    *v_first_out = v0;
#ifndef HGSS_PROFILE_OFF
    if (s_coarse_sample) s_coarse_copy += svcGetSystemTick() - copy_start;
#endif
    return true;
}


static void dl_replay_impl(dl_entry *e)
{
    uint32_t list_v0 = s_buf[s_build].frame.vert_count;
    uint32_t last_block_v0 = list_v0, last_block_rel = 0;

    for (uint32_t k = 0; k < e->nevents; ++k) {
        const uint32_t ev = e->events[k];
        if (ev & DL_EV_BLOCK) {
            const dl_block *b = &e->blocks[ev & ~DL_EV_BLOCK];
            uint32_t v0;
            if (!dl_append_block(e, b, &v0)) {
                ++s_buf[s_build].frame.dropped_verts;
                continue;
            }
            last_block_v0 = v0;
            last_block_rel = b->first_vert;
        } else {
            const uint32_t off = ev & 0xFFFFFu;
            execute(ev >> 20, e->words + off);
        }
    }
    s_color = e->exit.color;
    memcpy(s_coord, e->exit.coord, sizeof(s_coord));
    s_texcoord[0] = e->exit.texcoord[0];
    s_texcoord[1] = e->exit.texcoord[1];
    s_texcoord_raw[0] = e->exit.texcoord_raw[0];
    s_texcoord_raw[1] = e->exit.texcoord_raw[1];
    s_in_begin = e->exit.in_begin;
    s_primitive = e->exit.primitive;
    s_prim_count = e->exit.prim_count;
    s_strip_parity = e->exit.strip_parity;
    /* open primitives continue from the last block's vertices */
    for (int i = 0; i < 4; ++i) {
        s_prim_verts[i] = e->exit.prim_rel[i] - last_block_rel + last_block_v0;
    }
    update_color_rgba();
}

static void dl_replay(dl_entry *e)
{
#ifndef HGSS_PROFILE_OFF
    const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
#endif
    dl_replay_impl(e);
#ifndef HGSS_PROFILE_OFF
    if (s_coarse_sample) s_coarse_replay += svcGetSystemTick() - t;
#endif
}

static uint32_t dl_vertex_bound(const dl_entry *e)
{
    return e->nverts;
}

#ifdef KJ_DL_VERIFY
static uint32_t s_verify_logged;

/* A hit decoded live: does the live output equal what replay would copy? */
static void dl_verify(const dl_entry *e, const frame_buf *fb)
{
    const uint32_t nv = fb->frame.vert_count - s_rec.v0, ni = fb->frame.index_count - s_rec.i0;
    const char *why = NULL;
    uint32_t at = 0;
    if (s_rec.bad) {
        why = "live decode no longer recordable";
    } else if (nv != e->nverts || ni != e->nidx) {
        why = "vertex/index counts";
    } else if (s_rec.nblocks != e->nblocks || s_rec.nevents != e->nevents) {
        why = "event structure";
    } else if (memcmp(s_rec.events, e->events, e->nevents * 4u) != 0) {
        why = "events";
    } else {
        for (uint32_t i = 0; i < nv && why == NULL; ++i) {
            const uint32_t *a = (const uint32_t *)&fb->verts[s_rec.v0 + i];
            const uint32_t *b = (const uint32_t *)&e->verts[i];
            if (a[0] != b[0] || (a[1] & 0xFFFFu) != (b[1] & 0xFFFFu) || a[2] != b[2] || a[3] != b[3]) {
                why = "vertex data";
                at = i;
            }
        }
        for (uint32_t bk = 0; bk < e->nblocks && why == NULL; ++bk) {
            const dl_block *bl = &e->blocks[bk];
            for (uint32_t i = 0; i < bl->nidx; ++i) {
                const uint32_t live = fb->indices[s_rec.i0 + bl->first_idx + i] - (s_rec.v0 + bl->first_vert);
                if (live != e->idx[bl->first_idx + i]) {
                    why = "indices";
                    at = bk;
                    break;
                }
            }
        }
    }
    if (why != NULL) {
        if (s_verify_logged < 40) {
            ++s_verify_logged;
            NitroPort_Log("[dlverify] MISMATCH %s at %lu: list %p (%lu words) nv %lu/%lu ni %lu/%lu "
                          "blocks %lu/%lu events %lu/%lu",
                          why, (unsigned long)at, (const void *)e->ptr, (unsigned long)e->count,
                          (unsigned long)nv, (unsigned long)e->nverts, (unsigned long)ni,
                          (unsigned long)e->nidx, (unsigned long)s_rec.nblocks,
                          (unsigned long)e->nblocks, (unsigned long)s_rec.nevents,
                          (unsigned long)e->nevents);
        }
    } else {
#ifndef HGSS_PROFILE_OFF
        ++s_dl_hits;
#endif
    }
}
#endif

static void decode_words_impl(const uint32_t *words, size_t count)
{
#ifdef KJ_G3_PROFILE
    const u64 t0 = svcGetSystemTick();
#endif
    size_t i = 0;

    while (i < count && s_fifo_need > 0) {
        NitroPort_GxFifoWord(words[i++]);
    }
    while (i < count) {
        uint32_t cmds = words[i];
        size_t need = 0;
        for (int k = 0; k < 4; ++k) {
            need += s_param_count[(cmds >> (8 * k)) & 0xFF];
        }
        if (i + 1 + need > count) {
            s_rec.bad = true; /* (no-op unless recording) */
            while (i < count) {
                NitroPort_GxFifoWord(words[i++]);
            }
            break;
        }
        ++i;
        for (int k = 0; k < 4 && cmds != 0; ++k, cmds >>= 8) {
            const unsigned op = cmds & 0xFF;
            if (op == 0) {
                continue;
            }
#ifdef KJ_DL_CACHE
            if (s_rec.active) {
                rec_command(op, words + i);
            }
#endif
            hot_command(op, words + i);
#ifdef KJ_DL_CACHE
            if (s_rec.active && (op < 0x20 || op > 0x28)) {
                rec_open_block(); /* the next block starts after this command */
            }
#endif
            i += s_param_count[op];
        }
    }
#ifdef KJ_G3_PROFILE
    s_fifo_ticks += svcGetSystemTick() - t0;
#endif
}

static void decode_words(const uint32_t *words, size_t count)
{
#ifndef HGSS_PROFILE_OFF
    const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
#endif
    decode_words_impl(words, count);
#ifndef HGSS_PROFILE_OFF
    if (s_coarse_sample) s_coarse_decode += svcGetSystemTick() - t;
#endif
}

static void fifo_words_impl(const uint32_t *words, size_t count)
{
#ifndef HGSS_PROFILE_OFF
    ++s_fifo_calls;
#endif
#ifndef HGSS_PROFILE_OFF
    s_fifo_words += (u32)count;
#endif
    if (s_param_count[0x16] == 0) {
        build_param_table();
    }
#ifndef KJ_DL_CACHE
    /* KJ_DL_NO_CACHE: every list decoded live. */
    decode_words(words, count);
    return;
#endif
    if (count < DL_MIN_WORDS || count > 0xFFFFFu || s_in_begin || s_fifo_need > 0 || s_rec.active) {
#ifndef HGSS_PROFILE_OFF
        s_dl_cov[DLC_SHORT] += (uint32_t)count;
#endif
        if (count < DL_MIN_WORDS) {
#ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_SMALL] += (uint32_t)count;
#endif
        } else if (s_fifo_need > 0) {
#ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_SPLIT] += (uint32_t)count;
#endif
        } else if (s_in_begin) {
#ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_BEGIN] += (uint32_t)count;
#endif
            if ((words[0] & 0xFFu) == 0x40u) {
#ifndef HGSS_PROFILE_OFF
                s_dl_cov[DLC_BEGIN_FIRST] += (uint32_t)count;
#endif
            }
        }
        decode_words(words, count);
        return;
    }
    dl_entry *e = dl_slot(words, (uint32_t)count);
    if (e == NULL) {
        decode_words(words, count);
        return;
    }
    if (e->ptr != words || e->count != count) {
#ifndef HGSS_PROFILE_OFF
        s_dl_cov[DLC_NEW] += (uint32_t)count;
#endif
        dl_free(e);
        uint32_t *copy = dl_alloc((uint32_t)count * 4u);
        if (copy == NULL) {
            dl_flush_all();
            decode_words(words, count);
            return;
        }
        e->words = copy;
        e->ptr = words;
        e->count = (uint32_t)count;
        memcpy(e->words, words, count * 4u);
        e->reads = dl_entry_reads(words, count);
    } else if (e->never && (s_stats.swaps & DL_NEVER_RECHECK_MASK) != 0u) {
        /* An unrecordable list decodes live whatever its content, so the
         * content check only matters for noticing it became recordable;
         * the title's lit Ho-Oh lists spent 6.8% of samples comparing */
        e->last_frame = s_stats.swaps;
#ifndef HGSS_PROFILE_OFF
        s_dl_cov[DLC_NEVER] += (uint32_t)count;
#endif
#ifndef HGSS_PROFILE_OFF
        ++s_dl_uncacheable;
#endif
        decode_words(words, count);
        return;
    } else if (memcmp(e->words, words, count * 4u) != 0) {
#ifndef HGSS_PROFILE_OFF
        s_dl_cov[DLC_CHANGED] += (uint32_t)count;
#endif
        memcpy(e->words, words, count * 4u);
        dl_drop_recording(e);
        e->never = false;
        e->needs_light = false;
        e->reads = dl_entry_reads(words, count);
    }
    e->last_frame = s_stats.swaps;
    if (e->never) {
#ifndef HGSS_PROFILE_OFF
        s_dl_cov[DLC_NEVER] += (uint32_t)count;
#endif
#ifndef HGSS_PROFILE_OFF
        ++s_dl_uncacheable;
#endif
        decode_words(words, count);
        return;
    }
    const bool texgen = (s_tex_param >> 30) != 0;
    const bool lit = e->needs_light || ((s_attr | s_attr_pending) & 0xFu) != 0;
    dl_key key;
    bool verify = false;
    const size_t key_bytes = dl_key_size(lit, texgen, e->reads);
    dl_make_key(&key, lit, texgen, e->reads);
    if (e->recorded && e->lit == lit && e->texgen == texgen &&
        memcmp(&key, &e->key, key_bytes) == 0) {
        frame_buf *fb = &s_buf[s_build];
        if (fb->frame.vert_count + dl_vertex_bound(e) <= KJ_G3_MAX_VERTS &&
            fb->frame.index_count + e->nidx <= KJ_G3_MAX_INDICES &&
            fb->frame.batch_count + e->nblocks + 2 <= KJ_G3_MAX_BATCHES) {
#ifdef KJ_DL_VERIFY
            verify = true;
#else
            dl_replay(e);
#ifndef HGSS_PROFILE_OFF
            ++s_dl_hits;
#endif
#ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_HIT] += (uint32_t)count;
#endif
            return;
#endif
        }
    }

#ifdef KJ_DL_VERIFY
    if (e->recorded && !verify) {
        static uint32_t logged;
        if (logged < 30 && s_stats.swaps > 4600u) {
            ++logged;
            const uint32_t *a = (const uint32_t *)&key, *b = (const uint32_t *)&e->key;
            unsigned first = 0;
            while (first < key_bytes / 4 && a[first] == b[first]) {
                ++first;
            }
            NitroPort_Log("[dlmiss] list %p lit %d/%d texgen %d/%d key word %u: %08lx vs %08lx",
                          (const void *)words, lit, e->lit, texgen, e->texgen, first,
                          first < key_bytes / 4 ? (unsigned long)a[first] : 0ul,
                          first < key_bytes / 4 ? (unsigned long)b[first] : 0ul);
        }
    }
#endif
    /* record */
    if (verify) {
#ifndef HGSS_PROFILE_OFF
        s_dl_cov[DLC_HIT] += (uint32_t)count;
#endif
    } else if (e->recorded) {
#ifndef HGSS_PROFILE_OFF
        s_dl_cov[DLC_KEY] += (uint32_t)count;
#endif
    }
#ifndef HGSS_PROFILE_OFF
    ++s_dl_misses;
#endif
    if (e->recorded && !verify) {
        dl_drop_recording(e);
    }
    frame_buf *fb = &s_buf[s_build];
    /* reset every scalar field (the arrays need no clearing; nevents and
     * nblocks follow them in the struct, so one memset cannot cover them) */
    s_rec.active = s_rec.bad = s_rec.has_matrix = s_rec.vecdep = false;
    s_rec.nevents = 0;
    s_rec.nblocks = 0;
    s_rec.active = true;
    s_rec.base = words;
    s_rec.v0 = fb->frame.vert_count;
    s_rec.i0 = fb->frame.index_count;
    rec_open_block();
    const uint32_t drops0 = fb->frame.dropped_verts + fb->frame.dropped_batches + fb->frame.unsupported;
    decode_words(words, count);
    rec_close_block();
    s_rec.active = false;

    if (fb->frame.dropped_verts + fb->frame.dropped_batches + fb->frame.unsupported != drops0 ||
        s_fifo_need > 0) {
        s_rec.bad = true;
#ifndef HGSS_PROFILE_OFF
        ++s_dl_why[2];
#endif
    }
    if (s_rec.vecdep && s_rec.has_matrix) {
        s_rec.bad = true;
#ifndef HGSS_PROFILE_OFF
        ++s_dl_why[3];
#endif
    }
    if (s_rec.vecdep && !lit && !s_rec.bad) {
        /* keyed without the lighting state it turned out to read: key on it
         * from now on and record next time */
        e->needs_light = true;
        return;
    }
#ifdef KJ_DL_VERIFY
    if (verify) {
        dl_verify(e, fb);
        return;
    }
#endif
    if (s_rec.bad) {
        e->never = true;
        return;
    }
    const uint32_t nv = fb->frame.vert_count - s_rec.v0, ni = fb->frame.index_count - s_rec.i0;
    if (!dl_reserve((void **)&e->events, &e->cap_events, s_rec.nevents + 1u, 4u) ||
        !dl_reserve((void **)&e->blocks, &e->cap_blocks, s_rec.nblocks + 1u, (uint32_t)sizeof(dl_block)) ||
        !dl_reserve((void **)&e->verts, &e->cap_verts, nv, (uint32_t)sizeof(kj_g3_vertex)) ||
        !dl_reserve((void **)&e->idx, &e->cap_idx, ni, 2u)) {
        dl_flush_all(); /* arena full: start over (re-recorded on later calls) */
        return;
    }
    memcpy(e->events, s_rec.events, s_rec.nevents * 4u);
    memcpy(e->blocks, s_rec.blocks, s_rec.nblocks * sizeof(dl_block));
    e->nevents = s_rec.nevents;
    e->nblocks = s_rec.nblocks;
    if (nv) {
        memcpy(e->verts, &fb->verts[s_rec.v0], nv * sizeof(kj_g3_vertex));
    }
    for (uint32_t b = 0; b < s_rec.nblocks; ++b) {
        const dl_block *bl = &s_rec.blocks[b];
        for (uint32_t i = 0; i < bl->nidx; ++i) {
            const uint32_t abs = fb->indices[s_rec.i0 + bl->first_idx + i];
            e->idx[bl->first_idx + i] = (uint16_t)(abs - (s_rec.v0 + bl->first_vert));
        }
    }
    e->nverts = nv;
    e->nidx = ni;

    memcpy(&e->key, &key, key_bytes);
    e->lit = lit;
    e->texgen = texgen;
    dl_capture_exit(&e->exit, s_rec.v0);
    if (e->exit.in_begin) {
        /* open primitive at the end: express its vertices relative to the
         * last block so replay can rebase them */
        uint32_t last_rel = 0;
        for (uint32_t b = 0; b < e->nblocks; ++b) {
            last_rel = e->blocks[b].first_vert;
        }
        if (!e->exit.prim_valid) {
#ifndef HGSS_PROFILE_OFF
            ++s_dl_why[5];
#endif
            dl_drop_recording(e);
            e->never = true;
            return;
        }
        (void)last_rel;
    }
    e->recorded = true;
}

void NitroPort_GxFifoWords(const uint32_t *words, size_t count)
{
#ifndef HGSS_PROFILE_OFF
    const bool outer = s_coarse_depth++ == 0;
    u64 start = 0;
    if (outer) {
        /* Deterministic PRNG avoids locking the stride to a model's calls. */
        s_coarse_seq ^= s_coarse_seq << 13;
        s_coarse_seq ^= s_coarse_seq >> 17;
        s_coarse_seq ^= s_coarse_seq << 5;
        s_coarse_sample = (s_coarse_seq & 63u) == 0;
        if (s_coarse_sample) {
            ++s_coarse_n;
            start = svcGetSystemTick();
        }
    }
#endif
    fifo_words_impl(words, count);
#ifndef HGSS_PROFILE_OFF
    if (outer && s_coarse_sample) s_coarse_total += svcGetSystemTick() - start;
    --s_coarse_depth;
    if (outer) s_coarse_sample = false;
#endif
}

void kj_g3_dl_stats(uint32_t *hits, uint32_t *misses, uint32_t *uncacheable, uint32_t *bytes)
{
#ifndef HGSS_PROFILE_OFF
    NitroPort_Log("[g3sample] random 1/64 FIFO calls: samples %lu total_ticks %llu decode_ticks %llu replay_ticks %llu (block_copy_ticks %llu nested); remainder includes lookup/recording and probe overhead",
                  (unsigned long)s_coarse_n, (unsigned long long)s_coarse_total,
                  (unsigned long long)s_coarse_decode, (unsigned long long)s_coarse_replay,
                  (unsigned long long)s_coarse_copy);
    s_coarse_n = 0;
    s_coarse_total = s_coarse_decode = s_coarse_replay = s_coarse_copy = 0;
#endif
    *hits = s_dl_hits;
    *misses = s_dl_misses;
    *uncacheable = s_dl_uncacheable;
    *bytes = s_dl_arena_used;
    NitroPort_Log("[dlwhy] batch-in-block %lu shared-verts %lu straddle/drops %lu lit+matrix %lu "
                  "swap %lu open-prim %lu flushes %lu",
                  (unsigned long)s_dl_why[0], (unsigned long)s_dl_why[1], (unsigned long)s_dl_why[2],
                  (unsigned long)s_dl_why[3], (unsigned long)s_dl_why[4], (unsigned long)s_dl_why[5],
                  (unsigned long)s_dl_flushes);
    NitroPort_Log("[dlcov] bypass: small %lu split %lu open-begin %lu (begin first %lu)",
                  (unsigned long)s_dl_cov[DLC_SMALL], (unsigned long)s_dl_cov[DLC_SPLIT],
                  (unsigned long)s_dl_cov[DLC_BEGIN], (unsigned long)s_dl_cov[DLC_BEGIN_FIRST]);
    NitroPort_Log("[dlcov] words: short %lu new %lu changed %lu never %lu hit %lu keymiss %lu",
                  (unsigned long)s_dl_cov[DLC_SHORT], (unsigned long)s_dl_cov[DLC_NEW],
                  (unsigned long)s_dl_cov[DLC_CHANGED], (unsigned long)s_dl_cov[DLC_NEVER],
                  (unsigned long)s_dl_cov[DLC_HIT], (unsigned long)s_dl_cov[DLC_KEY]);
    memset(s_dl_cov, 0, sizeof(s_dl_cov));
    memset(s_dl_why, 0, sizeof(s_dl_why));
    s_dl_flushes = 0;
    (void)s_dl_bytes;
    s_dl_hits = s_dl_misses = s_dl_uncacheable = 0;
}


void kj_g3_take_profile(uint64_t *fifo_ticks, uint32_t *fifo_words, uint32_t *direct_words,
                        uint32_t *verts)
{
    *fifo_ticks = (uint64_t)s_fifo_calls; /* calls; ticks only with KJ_G3_PROFILE */
    s_fifo_calls = 0;
    (void)s_fifo_ticks;
    *fifo_words = s_fifo_words;
    *direct_words = s_direct_words;
    *verts = s_stats.commands;
    s_fifo_ticks = 0;
    s_fifo_words = s_direct_words = 0;
    s_stats.commands = 0;
}
