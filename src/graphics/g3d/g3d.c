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
            ++s_stats.unsupported; /* toon/highlight drawn as modulate */
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

static void draw_pass(const kj_target *target, bool translucent_pass, uint32_t disp3dcnt,
                      uint8_t stencil_flags)
{
    const kj_g3_frame *f = s_frame;
    const bool wbuffer = kj_g3d_wbuffer();
    const uint32_t dbg = kj_script_debug();
    uint32_t last_vp = 0xFFFFFFFFu;
    float fix[4][4];

    for (uint32_t i = 0; i < f->batch_count; ++i) {
        const kj_g3_batch *b = &f->batches[i];
        const bool translucent = (b->flags & KJ_G3_BATCH_TRANSLUCENT) != 0;
        if (translucent != translucent_pass || b->index_count == 0) {
            continue;
        }
        if (b->flags & KJ_G3_BATCH_SHADOW) {
            const unsigned id = (b->poly_attr >> 24) & 63u;
            if (id == 0) {
                ++s_stats.unsupported; /* shadow mask volume: not drawn */
                continue;
            }
        }
        if (b->viewport != last_vp) {
            make_fixup(target, b->viewport, wbuffer, fix);
            last_vp = b->viewport;
        }
        const kj_g3_matrix *pal = &f->matrices[b->first_matrix];
        if (s_last_pal == NULL || s_last_pal_count != b->matrix_count || s_last_pal_vp != b->viewport ||
            memcmp(s_last_pal, pal, b->matrix_count * sizeof(*pal)) != 0) {
            upload_palette(b, fix);
            s_last_pal = pal;
            s_last_pal_count = b->matrix_count;
            s_last_pal_vp = b->viewport;
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
        C3D_CullFace((dbg & KJ_DEBUG_NO_CULL) ? GPU_CULL_NONE
                     : faces == 2u                ? GPU_CULL_BACK_CCW
                     : faces == 1u                ? GPU_CULL_FRONT_CCW
                                                  : GPU_CULL_NONE);

        /* depth */
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
        C3D_DepthTest(true, func,
                      depth_write ? GPU_WRITE_ALL : GPU_WRITE_COLOR);

        if (translucent && (disp3dcnt & 8u) && !(dbg & KJ_DEBUG_NO_BLEND)) {
            C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
                           GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
        } else {
            C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
        }
        (void)stencil_flags;
        C3D_DrawElements(GPU_TRIANGLES, (int)b->index_count, C3D_UNSIGNED_SHORT,
                         f->indices + b->first_index);
        ++s_stats.draws;
        s_stats.triangles += b->index_count / 3u;
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
    s_stats.batches += f->batch_count;
    s_stats.unsupported += f->unsupported + f->dropped_batches + f->dropped_verts;
    s_stats.box_tests += f->box_tests;
    s_stats.box_culled += f->box_culled;
    s_stats.box_culled_far += f->box_culled_far;
    if (f->disp3dcnt & (1u << 7)) {
        ++s_stats.unsupported; /* fog */
    }
    if (f->disp3dcnt & (1u << 5)) {
        ++s_stats.unsupported; /* edge marking (baseline excluded) */
    }

    kj_gpu_use_program(KJ_PROGRAM_3D);
    kj_gpu_invalidate(); /* textures are bound directly below */
    kj_gpu_use_program(KJ_PROGRAM_3D);
    s_last_tev = -1;
    s_last_tex = NULL;
    s_last_pal = NULL;
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
