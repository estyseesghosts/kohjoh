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
#include <string.h>

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
        }
    } else {
        mtx *t = selected();
        mtx_mul(t, m, t);
    }
    if (s_mtx_mode != 3) {
        clip_changed();
    }
}

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

static uint16_t shade(uint32_t value)
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
    uint16_t vert_gmtx[KJ_G3_MAX_VERTS]; /* global matrix id per vertex */
} frame_buf;

static frame_buf s_buf[2];
static int s_build;          /* index of the frame being built */
static bool s_have_display;  /* a swap has happened */
static bool s_wide;
static kj_g3_stats s_stats;

/* Current batch bookkeeping. */
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

static void close_batch(void)
{
    s_batch = NULL;
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
    return s_batch != NULL && s_batch_key_attr == s_attr && s_batch_key_tex == s_tex_param &&
           s_batch_key_pltt == s_tex_pltt && s_batch_key_vp == s_viewport;
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
    int slot = palette_slot(gid);
    if (slot < 0) {
        close_batch();
        if (!open_batch()) {
            return;
        }
        slot = palette_slot(gid);
        if (slot < 0) {
            ++fb->frame.dropped_verts;
            return;
        }
    }

    const uint32_t n = fb->frame.vert_count++;
    kj_g3_vertex *v = &fb->verts[n];
    const unsigned r = s_color & 31u, g = (s_color >> 5) & 31u, b = (s_color >> 10) & 31u;
    const unsigned a = (s_attr >> 16) & 31u;
    v->x = s_coord[0];
    v->y = s_coord[1];
    v->z = s_coord[2];
    v->mtx = (uint8_t)slot;
    v->pad = 0;
    v->r = (uint8_t)((r << 3) | (r >> 2));
    v->g = (uint8_t)((g << 3) | (g >> 2));
    v->b = (uint8_t)((b << 3) | (b >> 2));
    v->a = (uint8_t)((a << 3) | (a >> 2));
    v->s = (int16_t)s_texcoord[0];
    v->t = (int16_t)s_texcoord[1];
    fb->vert_gmtx[n] = gid;

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

    ++s_stats.box_tests;
    for (int i = 0; i < 8; ++i) {
        const int64_t vx = x + ((i & 1) ? w : 0);
        const int64_t vy = y + ((i & 2) ? h : 0);
        const int64_t vz = z + ((i & 4) ? d : 0);
        int64_t o[4];
        for (int j = 0; j < 4; ++j) {
            o[j] = (vx * c->m[0][j] + vy * c->m[1][j] + vz * c->m[2][j] + (int64_t)4096 * c->m[3][j]) >> 12;
        }
        /* Widescreen: the visible x range is 1.25x the DS's. */
        const int64_t wx = s_wide ? (o[3] * 5) / 4 : o[3];
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
    if (all_out == 0) {
        stat |= 0x2u;
    } else {
        ++s_stats.box_culled;
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

static void execute(unsigned op, const uint32_t *p)
{
    ++s_stats.commands;
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
    case 0x1B: {
        mtx m;
        mtx_identity(&m);
        m.m[0][0] = (int32_t)p[0];
        m.m[1][1] = (int32_t)p[1];
        m.m[2][2] = (int32_t)p[2];
        mult_matrix(&m, true);
        break;
    }
    case 0x1C: {
        mtx m;
        mtx_identity(&m);
        m.m[3][0] = (int32_t)p[0];
        m.m[3][1] = (int32_t)p[1];
        m.m[3][2] = (int32_t)p[2];
        mult_matrix(&m, false);
        break;
    }
    case 0x20:
        s_color = (uint16_t)(p[0] & 0x7FFF);
        break;
    case 0x21:
        if ((s_tex_param >> 30) == 2) {
            const int32_t src[3] = { sign_bits(p[0], 10) * 8, sign_bits(p[0] >> 10, 10) * 8,
                                     sign_bits(p[0] >> 20, 10) * 8 };
            texgen(src, 2);
        }
        if (s_attr & 0xF) {
            s_color = shade(p[0]);
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
        s_tex_param = p[0];
        break;
    case 0x2B:
        s_tex_pltt = p[0] & 0x1FFF;
        break;
    case 0x30:
        s_diffuse = (uint16_t)(p[0] & 0x7FFF);
        s_ambient = (uint16_t)((p[0] >> 16) & 0x7FFF);
        if (p[0] & 0x8000) {
            s_color = s_diffuse;
        }
        break;
    case 0x31:
        s_specular = (uint16_t)(p[0] & 0x7FFF);
        s_emission = (uint16_t)((p[0] >> 16) & 0x7FFF);
        s_use_shininess = (p[0] & 0x8000) != 0;
        break;
    case 0x32: {
        const unsigned i = (p[0] >> 30) & 3;
        int32_t in[3];
        for (int j = 0; j < 3; ++j) {
            in[j] = sign_bits(p[0] >> (j * 10), 10) * 8;
        }
        transform_vector(in, &s_vec, s_light_dir[i]);
        s_light_half[i][0] = s_light_dir[i][0] / 2;
        s_light_half[i][1] = s_light_dir[i][1] / 2;
        s_light_half[i][2] = (s_light_dir[i][2] - 4096) / 2;
        break;
    }
    case 0x33:
        s_light_color[(p[0] >> 30) & 3] = (uint16_t)(p[0] & 0x7FFF);
        break;
    case 0x34:
        memcpy(s_shininess, p, sizeof(s_shininess));
        break;
    case 0x40:
        s_attr = s_attr_pending;
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
        s_viewport = p[0];
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

/* Direct writes: parameters arrive one word per call. */
static unsigned s_direct_op;
static uint32_t s_direct_params[32];
static int s_direct_have;

void NitroPort_GxDirectWrite(int opcode, uint32_t value)
{
    const unsigned op = (unsigned)opcode & 0xFF;
    const int need = param_count(op);

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
            execute(s_fifo_op, s_fifo_params);
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
            execute(s_fifo_op, s_fifo_params);
            s_fifo_need = 0;
            fifo_next_command();
        }
        return;
    }
    s_fifo_cmds = value;
    s_fifo_left = 4;
    fifo_next_command();
}

void NitroPort_GxFifoWords(const uint32_t *words, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        NitroPort_GxFifoWord(words[i]);
    }
}
