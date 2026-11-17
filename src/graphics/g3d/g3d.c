/*
 * g3d.c: the DS 3D layer on the PICA (GPU_3D.md).
 *
 * Input is the DS-side geometry engine's frame (runtime/kj_g3.c): object-
 * space vertices, indexed triangles, material batches and, per batch, a
 * palette of DS clip matrices. Here each palette entry is multiplied by the
 * screen fixup F (viewport, widescreen, 3DS screen rotation, PICA depth
 * range) and uploaded as vertex-shader uniforms; the GPU transforms every
 * vertex. Opaque batches are drawn first, then translucent ones in
 * submission order (DS manual-sort order; auto-sort is approximated).
 *
 * Material contracts handled: modulate/decal, polygon alpha, translucent
 * textures (A3I5/A5I3), depth-write for translucent polygons (attr bit 11),
 * DEPTH_EQUAL (attr bit 14), culling, alpha test (DISP3DCNT bit 2), clear
 * colour/alpha. Not yet: fog, toon/highlight, edge marking, shadow volumes,
 * wireframe -- each counted in unsupported, never silently dropped.
 */

#include "graphics/g3d/g3d.h"

#include "app/script.h"
#include "diagnostics/log.h"
#include "diagnostics/perf.h"
#include "graphics/g3d/textures.h"
#include "graphics/pica/gpu.h"
#include "runtime/kj_lighting.h"

#include <3ds.h>
#include <citro3d.h>
#include <string.h>

static const kj_g3_frame *s_frame;
static kj_g3d_stats s_stats;

/* Redundant-state filter for one kj_g3d_draw (reset at its start, after the
 * program bind). Consecutive DS batches usually share TEV mode, texture and
 * clip matrices; re-sending them dirtied all six TEV stages, the texture
 * unit and the whole palette on every draw (coffeepot PICA200/gpu-rendering.md,
 * "What a draw call costs": state changes are the CPU-side cost). */
static int s_last_tev;
static C3D_Tex *s_last_tex;
static uint32_t s_last_wrap;
static const kj_g3_matrix *s_last_pal;
static uint32_t s_last_pal_count;
static uint32_t s_last_pal_vp;
static kj_program_id s_draw_program;
static const kj_lighting *s_last_lighting;
static uint32_t s_last_alpha;
/* GPU rasteriser state, keyed to skip redundant citro3d setters (issue #10:
 * coffeepot PICA200/gpu-rendering.md, "What a draw call costs": each state
 * change is 1-2 us on an O3DS, a tenth of a separate draw). 0xFFFFFFFF is
 * "unset": the first batch of a program or frame always sends. */
static uint32_t s_last_cull, s_last_depth, s_last_blend;
static uint32_t s_last_shininess[32];
static bool s_shininess_valid;

bool kj_g3d_init(void)
{
    kj_tex_init();
    return true;
}

void kj_g3d_exit(void)
{
    kj_tex_exit();
}

void kj_g3d_set_frame(const kj_g3_frame *frame)
{
    s_frame = frame;
}

void kj_g3d_take_stats(kj_g3d_stats *out)
{
    kj_tex_stats ts;
    kj_tex_take_stats(&ts);
    *out = s_stats;
    out->tex_uploads = ts.uploads;
    out->tex_bytes_compared = ts.compared_bytes;
    out->tex_cache_full = ts.full;
    memset(&s_stats, 0, sizeof(s_stats));
}

/*
 * Depth buffering (SWAP_BUFFERS bit 1). Z mode: depth = -z'/w' (PICA
 * Z-buffer, C3D_DepthMap(true, -1, 0)), near = 1, test GREATER against a
 * clear of 0. W mode: the DS stores clip w (fx 20.12, saturating at 24 bits,
 * so 1/4096 per step up to w = 4096). The PICA W-buffer multiplies the
 * mapped depth by clip w (coffeepot hardware/gpu-internal-registers.md,
 * 0x04C/0x06D): with z' = -w/4096 and scale -1, offset 0 the stored depth
 * is w/4096 whether or not the depth-map transform is enabled. Near is then
 * small: test LESS against a clear of 1. z' no longer carries DS z, so near
 * and far z clipping are not applied in W mode (w > 0 clipping still is).
 * The HGSS title screen needs W mode: its camera has near = 0, which puts
 * every vertex at z/w = 1 in Z mode.
 */
#define W_DEPTH_SCALE (1.0f / 4096.0f)

bool kj_g3d_wbuffer(void)
{
    return s_frame != NULL && (s_frame->swap_params & 2u) != 0;
}

/* F: DS clip -> PICA clip for viewport `vp` on `target`. */
static void make_fixup(const kj_target *t, uint32_t vp, bool wbuffer, float f[4][4])
{
    const float x1 = (float)(vp & 0xFF), y1 = (float)((vp >> 8) & 0xFF);
    const float vw = (float)(((vp >> 16) & 0xFF)) - x1 + 1.0f;
    const float vh = (float)(((vp >> 24) & 0xFF)) - y1 + 1.0f;
    const float a = t->scale / 120.0f;
    const float half_w = t->width / 2.0f;
    const float b = t->scale / half_w;

    memset(f, 0, sizeof(float) * 16);
    f[0][1] = a * vh / 2.0f;
    f[0][3] = 1.0f - a * (192.0f - y1 - vh / 2.0f);
    f[1][0] = -b * vw / 2.0f;
    f[1][3] = 1.0f - t->offset_x / half_w - b * (x1 + vw / 2.0f);
    if (wbuffer) {
        f[2][3] = -W_DEPTH_SCALE;
    } else {
        f[2][2] = 0.5f;
        f[2][3] = -0.5f;
    }
    f[3][3] = 1.0f;
}

static void upload_palette(const kj_g3_batch *b, float fix[4][4])
{
    const int loc = kj_gpu_uloc_3d_palette();
    for (unsigned i = 0; i < b->matrix_count; ++i) {
        const kj_g3_matrix *m = &s_frame->matrices[b->first_matrix + i];
        C3D_FVec *rows = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, loc + (int)i * 4, 4);
        for (int k = 0; k < 4; ++k) {
            float o[4];
            for (int c = 0; c < 4; ++c) {
                /* row k of F * M^T, column c: sum_j F[k][j] * M[c][j] */
                o[c] = fix[k][0] * m->m[c][0] + fix[k][1] * m->m[c][1] + fix[k][2] * m->m[c][2] +
                       fix[k][3] * m->m[c][3];
            }
            /* C3D_FVec stores w, z, y, x in memory order */
            rows[k].x = o[0];
            rows[k].y = o[1];
            rows[k].z = o[2];
            rows[k].w = o[3];
        }
    }
}

static void upload_lighting(const kj_g3_batch *b)
{
    const kj_lit_uniforms *u = kj_gpu_lit_uniforms();
    const kj_lighting *light = &s_frame->lighting[b->flags >> 16];
    const uint32_t alpha = (b->poly_attr >> 16) & 31u;
    if (light != s_last_lighting) {
        for (unsigned i = 0; i < 4; ++i) {
            int32_t dir_hi[3], dir_lo[3], half_hi[3], half_lo[3];
            for (unsigned c = 0; c < 3; ++c) {
                int32_t d = (light->enabled & (1u << i)) ? -light->direction[i][c] : 0;
                int32_t h = (light->enabled & (1u << i)) ? -light->half[i][c] : 0;
                dir_hi[c] = d >> 7;
                dir_lo[c] = d - dir_hi[c] * 128;
                half_hi[c] = h >> 7;
                half_lo[c] = h - half_hi[c] * 128;
            }
            /* w flags: the shader skips a disabled light, and the half
             * vector of a light with no specular colour (exactly zero). */
            const bool on = (light->enabled & (1u << i)) != 0;
            const bool spec = on && (light->specular[i][0] | light->specular[i][1] |
                                     light->specular[i][2]) != 0;
            C3D_FVUnifSet(GPU_VERTEX_SHADER, u->direction + i,
                          dir_hi[0], dir_hi[1], dir_hi[2], on);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, u->direction_low + i,
                          dir_lo[0], dir_lo[1], dir_lo[2], 0);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, u->half + i,
                          half_hi[0], half_hi[1], half_hi[2], spec);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, u->half_low + i,
                          half_lo[0], half_lo[1], half_lo[2], 0);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, u->ambient + i,
                          light->ambient[i][0], light->ambient[i][1], light->ambient[i][2], 0);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, u->diffuse + i,
                          light->diffuse[i][0], light->diffuse[i][1], light->diffuse[i][2], 0);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, u->specular + i,
                          light->specular[i][0], light->specular[i][1], light->specular[i][2], 0);
        }
        C3D_FVUnifSet(GPU_VERTEX_SHADER, u->emission,
                      light->emission[0], light->emission[1], light->emission[2], 0);
        if (light->use_shininess &&
            (!s_shininess_valid ||
             memcmp(s_last_shininess, light->shininess, sizeof(s_last_shininess)) != 0)) {
            for (unsigned i = 0; i < 32; ++i) {
                const uint32_t word = light->shininess[i];
                C3D_FVUnifSet(GPU_VERTEX_SHADER, u->shininess + i,
                              word & 255u, word >> 8 & 255u, word >> 16 & 255u, word >> 24);
            }
            memcpy(s_last_shininess, light->shininess, sizeof(s_last_shininess));
            s_shininess_valid = true;
        }
    }
    if (light != s_last_lighting || alpha != s_last_alpha) {
        C3D_FVUnifSet(GPU_VERTEX_SHADER, u->params,
                      (alpha << 3) | (alpha >> 2), light->use_shininess, 0, 0);
    }
    s_last_lighting = light;
    s_last_alpha = alpha;
}

static void tev_for(uint32_t attr, bool textured)
{
    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    if (!textured) {
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    } else if (((attr >> 4) & 3u) == 1u) { /* decal */
        C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_TEXTURE0);
        C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR,
                        GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, 0, 0);
        C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    } else {
        if (((attr >> 4) & 3u) == 2u) {
#ifndef HGSS_PROFILE_OFF
            ++s_stats.unsupported; /* toon/highlight drawn as modulate */
#endif
        }
        C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
        C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
    }
    for (int i = 1; i < 6; ++i) {
        C3D_TexEnvInit(C3D_GetTexEnv(i));
    }
}

static GPU_TEXTURE_WRAP_PARAM wrap_mode(bool repeat, bool flip)
{
    return !repeat ? GPU_CLAMP_TO_EDGE : flip ? GPU_MIRRORED_REPEAT : GPU_REPEAT;
}

/* ---- union-palette join (issue #13) ------------------------------------
 * A separate C3D_DrawElements costs ~9.7 us on an O3DS no matter how few
 * triangles it carries (coffeepot PICA200/gpu-rendering.md, "What a draw
 * call costs"); a DS batch is a material with its own matrix-palette
 * slice, so consecutive batches rarely share state byte for byte and the
 * adjacency join in #10's baseline measured 0 joins. Opaque batches that
 * DO share material state (poly_attr, texture, lighting context,
 * viewport) can share one draw over a union palette: the scratch VBO
 * carries each vertex with its matrix index remapped into the union's
 * slots, the scratch IBO is the concatenated index buffers with vertex
 * bases remapped, and the union palette is uploaded once per group.
 * Translucent batches are never joined: with depth blending, the DS's
 * own draw order is the content.
 *
 * Joining is a pure optimisation with two visible-output risks, both
 * guarded here: batches are only joined when every state input that
 * reaches the GPU is identical, and any overflow (union palette wider
 * than the shader's, scratch space) falls back to the direct path, which
 * is the #10-verified code. */

#define KJ_JOIN_MAX_VERTS 8192
#define KJ_JOIN_MAX_INDICES 16384

static kj_g3_vertex s_join_verts[KJ_JOIN_MAX_VERTS];
static uint16_t s_join_indices[KJ_JOIN_MAX_INDICES];
static uint16_t s_join_map[KJ_G3_MAX_VERTS];
static uint8_t s_join_src[KJ_G3_MAX_VERTS];
static uint32_t s_join_epoch[KJ_G3_MAX_VERTS];
static uint32_t s_join_stamp;

static bool join_key_equal(const kj_g3_batch *a, const kj_g3_batch *b,
                           bool translucent_pass)
{
    if (b->index_count == 0 || (b->flags & KJ_G3_BATCH_SHADOW) != 0) {
        return false;
    }
    if (((b->flags & KJ_G3_BATCH_TRANSLUCENT) != 0) != translucent_pass) {
        return false;
    }
    if (b->poly_attr != a->poly_attr || b->tex_param != a->tex_param ||
        b->tex_pltt != a->tex_pltt || b->viewport != a->viewport) {
        return false;
    }
    if ((b->flags & KJ_G3_BATCH_LIT) != (a->flags & KJ_G3_BATCH_LIT)) {
        return false;
    }
    if ((b->flags & KJ_G3_BATCH_LIT) != 0 && (b->flags >> 16) != (a->flags >> 16)) {
        return false;
    }
    return true;
}

/* Slot of `m` in the union (adding it when there is room), or -1 if full. */
static int union_find_or_add(kj_g3_matrix *u, unsigned *count, unsigned cap,
                             const kj_g3_matrix *m)
{
    for (unsigned i = 0; i < *count; ++i) {
        if (memcmp(&u[i], m, sizeof(*m)) == 0) {
            return (int)i;
        }
    }
    if (*count >= cap) {
        return -1;
    }
    u[(*count)++] = *m;
    return (int)(*count - 1);
}

/* Set every GPU state input of `b` except the matrix palette: program and
 * lighting, viewport fixup, texture, TEV, cull, depth, blend. */
static void apply_batch_state(const kj_g3_batch *b, bool translucent,
                              uint32_t disp3dcnt, uint32_t dbg, bool wbuffer,
                              const kj_target *target, uint32_t *last_vp,
                              float fix[4][4])
{
    const kj_program_id program = (b->flags & KJ_G3_BATCH_LIT)
                                      ? KJ_PROGRAM_3D_LIT : KJ_PROGRAM_3D;
    if (program != s_draw_program) {
        kj_gpu_use_program(program);
        s_draw_program = program;
        s_last_pal = NULL;
        s_last_tex = NULL;
        s_last_lighting = NULL;
        s_shininess_valid = false;
    }
    if (program == KJ_PROGRAM_3D_LIT) {
        upload_lighting(b);
    }
    if (b->viewport != *last_vp) {
        make_fixup(target, b->viewport, wbuffer, fix);
        *last_vp = b->viewport;
    }

    /* texture */
    C3D_Tex *tex = NULL;
    if (((b->tex_param >> 26) & 7u) != 0 && (disp3dcnt & 1u) && !(dbg & KJ_DEBUG_NO_TEXTURE)) {
        tex = kj_tex_get(b->tex_param, b->tex_pltt);
    }
    const uint32_t wrap = (b->tex_param >> 16) & 0xFu;
    if (tex != NULL && (tex != s_last_tex || wrap != s_last_wrap)) {
        C3D_TexSetWrap(tex, wrap_mode((b->tex_param >> 16) & 1u, (b->tex_param >> 18) & 1u),
                       wrap_mode((b->tex_param >> 17) & 1u, (b->tex_param >> 19) & 1u));
        C3D_TexBind(0, tex);
        C3D_FVUnifSet(GPU_VERTEX_SHADER, kj_gpu_uloc_3d_texscale(),
                      1.0f / (16.0f * tex->width), -1.0f / (16.0f * tex->height), 0.0f, 1.0f);
        s_last_tex = tex;
        s_last_wrap = wrap;
    }
    const int tev = (int)((b->poly_attr >> 4) & 3u) | (tex != NULL ? 4 : 0);
    if (tev != s_last_tev) {
        tev_for(b->poly_attr, tex != NULL);
        s_last_tev = tev;
    }

    /* culling: bit 6 render back, bit 7 render front; 0 draws both */
    const unsigned faces = (b->poly_attr >> 6) & 3u;
    const uint32_t cull = (dbg & KJ_DEBUG_NO_CULL) ? GPU_CULL_NONE
                          : faces == 2u            ? GPU_CULL_BACK_CCW
                          : faces == 1u            ? GPU_CULL_FRONT_CCW
                                                   : GPU_CULL_NONE;
    if (cull != s_last_cull) {
        C3D_CullFace(cull);
        s_last_cull = cull;
    }
#ifndef HGSS_PROFILE_OFF
    else {
        ++s_stats.state_skips;
    }
#endif

    /* depth: func and write mask in one key */
    const bool depth_equal = (b->poly_attr >> 14) & 1u;
    const bool depth_write = !translucent || ((b->poly_attr >> 11) & 1u);
    GPU_TESTFUNC func;
    if (wbuffer) {
        func = depth_equal ? GPU_LEQUAL : GPU_LESS;
    } else {
        func = depth_equal ? GPU_GEQUAL : GPU_GREATER;
    }
    if (dbg & KJ_DEBUG_NO_DEPTH) {
        func = GPU_ALWAYS;
    }
    const uint32_t depth = ((uint32_t)func << 4) | (depth_write ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
    if (depth != s_last_depth) {
        C3D_DepthTest(true, func, depth_write ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
        s_last_depth = depth;
    }
#ifndef HGSS_PROFILE_OFF
    else {
        ++s_stats.state_skips;
    }
#endif

    /* blend: on/off only; the factors are fixed either way */
    const uint32_t blend = (translucent && (disp3dcnt & 8u) && !(dbg & KJ_DEBUG_NO_BLEND)) ? 1u : 0u;
    if (blend != s_last_blend) {
        if (blend) {
            C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
                           GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
        } else {
            C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
        }
        s_last_blend = blend;
    }
#ifndef HGSS_PROFILE_OFF
    else {
        ++s_stats.state_skips;
    }
#endif
}

/* One batch, drawn straight from the frame's VBO/IBO: the #10-verified
 * path. Used for single batches and as the fallback for declined joins. */
static void draw_batch_direct(const kj_g3_frame *f, const kj_g3_batch *b,
                              bool translucent, uint32_t disp3dcnt, uint32_t dbg,
                              bool wbuffer, const kj_target *target,
                              uint32_t *last_vp, float fix[4][4])
{
    apply_batch_state(b, translucent, disp3dcnt, dbg, wbuffer, target, last_vp, fix);
    const kj_g3_matrix *pal = &f->matrices[b->first_matrix];
    if (s_last_pal == NULL || s_last_pal_count != b->matrix_count || s_last_pal_vp != b->viewport ||
        memcmp(s_last_pal, pal, b->matrix_count * sizeof(*pal)) != 0) {
        upload_palette(b, fix);
        s_last_pal = pal;
        s_last_pal_count = b->matrix_count;
        s_last_pal_vp = b->viewport;
    }
    C3D_DrawElements(GPU_TRIANGLES, (int)b->index_count, C3D_UNSIGNED_SHORT,
                     f->indices + b->first_index);
#ifndef HGSS_PROFILE_OFF
    ++s_stats.draws;
    s_stats.triangles += b->index_count / 3u;
#endif
}

/* Join `n` batches of one state group into one draw over the union of
 * their matrix palettes. False: overflow of any kind; the caller falls
 * back to direct draws (never a partial draw). */
static bool draw_joined(const kj_g3_frame *f, const kj_g3_batch **members, unsigned n,
                        bool translucent, uint32_t disp3dcnt, uint32_t dbg,
                        bool wbuffer, const kj_target *target, uint32_t *last_vp,
                        float fix[4][4])
{
    const kj_g3_batch *b0 = members[0];
    const unsigned cap = (b0->flags & KJ_G3_BATCH_LIT) ? KJ_G3_LIT_PALETTE : KJ_G3_PALETTE;
    kj_g3_matrix u[KJ_G3_PALETTE];
    unsigned ucount = 0;

    for (unsigned i = 0; i < b0->matrix_count; ++i) {
        if (union_find_or_add(u, &ucount, cap, &f->matrices[b0->first_matrix + i]) < 0) {
            return false;
        }
    }
    uint32_t vused = 0, iused = 0;
    ++s_join_stamp;
    for (unsigned m = 0; m < n; ++m) {
        const kj_g3_batch *b = members[m];
        uint8_t pal[KJ_G3_PALETTE];
        for (unsigned k = 0; k < b->matrix_count; ++k) {
            int s = union_find_or_add(u, &ucount, cap, &f->matrices[b->first_matrix + k]);
            if (s < 0) {
                return false;
            }
            pal[k] = (uint8_t)s;
        }
        for (uint32_t j = 0; j < b->index_count; ++j) {
            const uint32_t v = f->indices[b->first_index + j];
            const kj_g3_vertex *ov = &f->verts[v];
            /* Lit batches pack lighting/normal data in mtx's top 3 bits
             * (palette index is mtx & 7); keep those bits through the
             * remap. A shared vertex can be named by two batches with
             * different matrix indices (each batch's mtx is batch-
             * relative); such a vertex needs a transformed copy per
             * mapping, not one. */
            const bool lit = (b->flags & KJ_G3_BATCH_LIT) != 0;
            const unsigned in = lit ? (ov->mtx & 7u) : ov->mtx;
            const uint8_t out = lit ? (uint8_t)((ov->mtx & 0xF8u) | pal[in]) : pal[in];
            if (s_join_epoch[v] != s_join_stamp || s_join_src[v] != ov->mtx ||
                s_join_verts[s_join_map[v]].mtx != out) {
                if (vused >= KJ_JOIN_MAX_VERTS) {
                    return false;
                }
                kj_g3_vertex nv = *ov;
                nv.mtx = out;
                s_join_map[v] = (uint16_t)vused;
                s_join_src[v] = ov->mtx;
                s_join_verts[vused++] = nv;
                s_join_epoch[v] = s_join_stamp;
            }
            if (iused >= KJ_JOIN_MAX_INDICES) {
                return false;
            }
            s_join_indices[iused++] = s_join_map[v];
        }
    }

    apply_batch_state(b0, translucent, disp3dcnt, dbg, wbuffer, target, last_vp, fix);
    const int loc = kj_gpu_uloc_3d_palette();
    for (unsigned s = 0; s < ucount; ++s) {
        C3D_FVec *rows = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, loc + (int)s * 4, 4);
        for (int k = 0; k < 4; ++k) {
            float o[4];
            for (int c = 0; c < 4; ++c) {
                o[c] = fix[k][0] * u[s].m[c][0] + fix[k][1] * u[s].m[c][1] +
                       fix[k][2] * u[s].m[c][2] + fix[k][3] * u[s].m[c][3];
            }
            rows[k].x = o[0];
            rows[k].y = o[1];
            rows[k].z = o[2];
            rows[k].w = o[3];
        }
    }
    s_last_pal = NULL; /* scratch union: always re-upload after a join */
    s_last_pal_count = ucount;
    s_last_pal_vp = b0->viewport;

    GSPGPU_FlushDataCache(s_join_verts, vused * sizeof(kj_g3_vertex));
    GSPGPU_FlushDataCache(s_join_indices, iused * sizeof(uint16_t));
    C3D_BufInfo *bi = C3D_GetBufInfo();
    BufInfo_Init(bi);
    BufInfo_Add(bi, s_join_verts, sizeof(kj_g3_vertex), 4, 0x2130);
    C3D_DrawElements(GPU_TRIANGLES, (int)iused, C3D_UNSIGNED_SHORT, s_join_indices);
    BufInfo_Init(bi);
    BufInfo_Add(bi, f->verts, sizeof(kj_g3_vertex), 4, 0x2130);
#ifndef HGSS_PROFILE_OFF
    ++s_stats.draws;
    s_stats.triangles += iused / 3u;
#endif
    return true;
}

static void draw_pass(const kj_target *target, bool translucent_pass, uint32_t disp3dcnt,
                      uint8_t stencil_flags)
{
    const kj_g3_frame *f = s_frame;
    const bool wbuffer = kj_g3d_wbuffer();
    const uint32_t dbg = kj_script_debug();
    uint32_t last_vp = 0xFFFFFFFFu;
    float fix[4][4];
    (void)stencil_flags;

    uint32_t i = 0;
    while (i < f->batch_count) {
        const kj_g3_batch *b = &f->batches[i];
        const bool translucent = (b->flags & KJ_G3_BATCH_TRANSLUCENT) != 0;
        if (translucent != translucent_pass || b->index_count == 0) {
            ++i;
            continue;
        }
        if (b->flags & KJ_G3_BATCH_SHADOW) {
            const unsigned id = (b->poly_attr >> 24) & 63u;
            if (id == 0) {
#ifndef HGSS_PROFILE_OFF
                ++s_stats.unsupported; /* shadow mask volume: not drawn */
#endif
                ++i;
                continue;
            }
        }

        if (translucent_pass) {
            /* Translucent geometry is depth-blended: the DS's own order is
             * the content, so it is never joined (issue #13). */
            draw_batch_direct(f, b, true, disp3dcnt, dbg, wbuffer, target, &last_vp, fix);
            ++i;
            continue;
        }

        /* Opaque: extend a state group while join_key_equal holds, then try
         * one joined draw over a union palette; any overflow falls back to
         * the direct path for the whole group (never a partial join). */
        const kj_g3_batch *members[KJ_G3_MAX_BATCHES > 256 ? 256 : 256];
        unsigned n = 1;
        members[0] = b;
        while (n < 256 && i + n < f->batch_count &&
               join_key_equal(b, &f->batches[i + n], false)) {
            members[n] = &f->batches[i + n];
            ++n;
        }
        bool drawn = false;
        if (n > 1 && !(dbg & (KJ_DEBUG_NO_JOIN |
                              ((b->flags & KJ_G3_BATCH_LIT) ? KJ_DEBUG_NO_JOIN_LIT : 0)))) {
#ifndef HGSS_PROFILE_OFF
            ++s_stats.join_groups;
#endif
            drawn = draw_joined(f, members, n, false, disp3dcnt, dbg, wbuffer, target,
                                &last_vp, fix);
#ifndef HGSS_PROFILE_OFF
            if (drawn) {
                ++s_stats.joined_groups;
                s_stats.joined_batches += n;
            } else {
                ++s_stats.join_declined;
            }
#endif
        }
        if (!drawn) {
            for (unsigned m = 0; m < n; ++m) {
                draw_batch_direct(f, members[m], false, disp3dcnt, dbg, wbuffer, target,
                                  &last_vp, fix);
            }
        }
        i += n;
    }
}

void kj_g3d_draw(const kj_target *target, bool wide, uint8_t stencil_flags)
{
    const kj_g3_frame *f = s_frame;
    (void)wide;
    if (f == NULL || f->batch_count == 0) {
        return;
    }
    uint64_t t0 = kj_perf_now();
#ifndef HGSS_PROFILE_OFF
    s_stats.batches += f->batch_count;
#endif
#ifndef HGSS_PROFILE_OFF
    s_stats.unsupported += f->unsupported + f->dropped_batches + f->dropped_verts;
#endif
#ifndef HGSS_PROFILE_OFF
    s_stats.box_tests += f->box_tests;
#endif
#ifndef HGSS_PROFILE_OFF
    s_stats.box_culled += f->box_culled;
#endif
#ifndef HGSS_PROFILE_OFF
    s_stats.box_culled_far += f->box_culled_far;
#endif
    if (f->disp3dcnt & (1u << 7)) {
#ifndef HGSS_PROFILE_OFF
        ++s_stats.unsupported; /* fog */
#endif
    }
    if (f->disp3dcnt & (1u << 5)) {
#ifndef HGSS_PROFILE_OFF
        ++s_stats.unsupported; /* edge marking (baseline excluded) */
#endif
    }

    /* Invalidate first (textures are bound directly below), then bind the
     * 3D program once; binding before the invalidate bound it twice. */
    kj_gpu_invalidate();
    kj_gpu_use_program(KJ_PROGRAM_3D);
    s_draw_program = KJ_PROGRAM_3D;
    s_last_lighting = NULL;
    s_last_tev = -1;
    s_last_tex = NULL;
    s_last_pal = NULL;
    s_last_cull = s_last_depth = s_last_blend = 0xFFFFFFFFu;
    s_shininess_valid = false;
    C3D_BufInfo *bi = C3D_GetBufInfo();
    BufInfo_Init(bi);
    BufInfo_Add(bi, f->verts, sizeof(kj_g3_vertex), 4, 0x2130);

    C3D_DepthMap(!kj_g3d_wbuffer(), -1.0f, 0.0f);
    const unsigned alpha_ref = (f->disp3dcnt & 4u) ? f->alpha_test_ref : 0u;
    C3D_AlphaTest(!(kj_script_debug() & KJ_DEBUG_NO_ALPHA_TEST), GPU_GREATER,
                  (int)(alpha_ref * 255u / 31u));
    C3D_StencilTest(true, GPU_ALWAYS, stencil_flags, 0, 0x03);
    C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);

    draw_pass(target, false, f->disp3dcnt, stencil_flags);
    draw_pass(target, true, f->disp3dcnt, stencil_flags);

    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_CullFace(GPU_CULL_NONE);
    kj_gpu_invalidate();
    kj_perf_add(KJ_PERF_G3D, kj_perf_now() - t0);
}
