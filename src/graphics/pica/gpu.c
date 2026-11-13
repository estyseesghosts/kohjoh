/*
 * gpu.c: shader programs, vertex layouts, the 2D quad arena and state
 * helpers.
 *
 * The 2D arena is one linear buffer reset per frame; it is safe to rewrite
 * because kj_ctr_publish_frame starts with C3D_FrameBegin(0), which waits
 * for the previous frame's GPU work. Quads are drawn indexed from a static
 * index buffer (0,1,2, 2,1,3 per quad), so a draw of N quads is one
 * C3D_DrawElements with no per-frame index writes.
 */

#include "graphics/pica/gpu.h"

#include "diagnostics/log.h"

#include <3ds.h>
#include <string.h>

/* Shader blobs assembled by the build (picasso + bin2s). */
extern const u8 g2d_shbin[];
extern const u32 g2d_shbin_size;
extern const u8 g3d_shbin[];
extern const u32 g3d_shbin_size;

#define ARENA_QUADS 16384u

static DVLB_s *s_dvlb_2d, *s_dvlb_3d;
static shaderProgram_s s_prog_2d, s_prog_3d;
static C3D_AttrInfo s_attr_2d, s_attr_3d;
static kj_program_id s_program;
static C3D_Tex *s_bound[3];
static int s_uloc_xform, s_uloc_uvscale, s_uloc_palette, s_uloc_texscale;

static kj_v2d *s_arena;
static const kj_v2d *s_bound_base;
static uint16_t *s_quad_indices;
static uint32_t s_arena_used, s_arena_dropped;

bool kj_gpu_init(void)
{
    s_dvlb_2d = DVLB_ParseFile((u32 *)g2d_shbin, g2d_shbin_size);
    s_dvlb_3d = DVLB_ParseFile((u32 *)g3d_shbin, g3d_shbin_size);
    if (s_dvlb_2d == NULL || s_dvlb_3d == NULL) {
        kj_log("[gpu] shader parse failed");
        return false;
    }
    shaderProgramInit(&s_prog_2d);
    shaderProgramSetVsh(&s_prog_2d, &s_dvlb_2d->DVLE[0]);
    shaderProgramInit(&s_prog_3d);
    shaderProgramSetVsh(&s_prog_3d, &s_dvlb_3d->DVLE[0]);
    s_uloc_xform = shaderInstanceGetUniformLocation(s_prog_2d.vertexShader, "xform");
    s_uloc_uvscale = shaderInstanceGetUniformLocation(s_prog_2d.vertexShader, "uvscale");
    s_uloc_palette = shaderInstanceGetUniformLocation(s_prog_3d.vertexShader, "palette");
    s_uloc_texscale = shaderInstanceGetUniformLocation(s_prog_3d.vertexShader, "texscale");

    AttrInfo_Init(&s_attr_2d);
    AttrInfo_AddLoader(&s_attr_2d, 0, GPU_SHORT, 2);         /* v0 position */
    AttrInfo_AddLoader(&s_attr_2d, 1, GPU_SHORT, 2);         /* v1 texcoord */
    AttrInfo_AddLoader(&s_attr_2d, 2, GPU_UNSIGNED_BYTE, 4); /* v2 colour */

    AttrInfo_Init(&s_attr_3d);
    AttrInfo_AddLoader(&s_attr_3d, 0, GPU_SHORT, 3);         /* v0 position */
    AttrInfo_AddLoader(&s_attr_3d, 1, GPU_UNSIGNED_BYTE, 4); /* v1 colour */
    AttrInfo_AddLoader(&s_attr_3d, 2, GPU_SHORT, 2);         /* v2 texcoord */
    AttrInfo_AddLoader(&s_attr_3d, 3, GPU_UNSIGNED_BYTE, 2); /* v3 matrix index, pad */

    s_arena = linearAlloc(ARENA_QUADS * 4u * sizeof(kj_v2d));
    s_quad_indices = linearAlloc(ARENA_QUADS * 6u * sizeof(uint16_t));
    if (s_arena == NULL || s_quad_indices == NULL) {
        kj_log("[gpu] out of linear memory for the 2D arena");
        return false;
    }
    for (uint32_t q = 0; q < ARENA_QUADS; ++q) {
        const uint16_t b = (uint16_t)(q * 4u);
        uint16_t *i = s_quad_indices + q * 6u;
        i[0] = b;
        i[1] = (uint16_t)(b + 1);
        i[2] = (uint16_t)(b + 2);
        i[3] = (uint16_t)(b + 2);
        i[4] = (uint16_t)(b + 1);
        i[5] = (uint16_t)(b + 3);
    }
    return true;
}

void kj_gpu_exit(void)
{
    shaderProgramFree(&s_prog_2d);
    shaderProgramFree(&s_prog_3d);
    DVLB_Free(s_dvlb_2d);
    DVLB_Free(s_dvlb_3d);
    linearFree(s_arena);
    linearFree(s_quad_indices);
}

void kj_gpu_invalidate(void)
{
    s_program = KJ_PROGRAM_NONE;
    s_bound_base = NULL;
    s_bound[0] = s_bound[1] = s_bound[2] = NULL;
}

void kj_gpu_use_program(kj_program_id id)
{
    if (s_program == id) {
        return;
    }
    s_program = id;
    if (id == KJ_PROGRAM_2D) {
        C3D_BindProgram(&s_prog_2d);
        C3D_SetAttrInfo(&s_attr_2d);
        C3D_BufInfo *bi = C3D_GetBufInfo();
        BufInfo_Init(bi);
        BufInfo_Add(bi, s_arena, sizeof(kj_v2d), 3, 0x210);
        s_bound_base = s_arena;
    } else if (id == KJ_PROGRAM_3D) {
        C3D_BindProgram(&s_prog_3d);
        C3D_SetAttrInfo(&s_attr_3d);
        /* the 3D module binds its vertex buffer */
    }
}

void kj_gpu_bind_texture(int unit, C3D_Tex *tex)
{
    if (s_bound[unit] == tex) {
        return;
    }
    s_bound[unit] = tex;
    C3D_TexBind(unit, tex);
}

kj_v2d *kj_gpu_quads_alloc(uint32_t quads, uint32_t *first_quad)
{
    if (s_arena_used + quads > ARENA_QUADS) {
#ifndef HGSS_PROFILE_OFF
        s_arena_dropped += quads;
#endif
        return NULL;
    }
    *first_quad = s_arena_used;
    kj_v2d *v = s_arena + s_arena_used * 4u;
    s_arena_used += quads;
    return v;
}

void kj_gpu_quads_reset(void)
{
    s_arena_used = 0;
}

void kj_gpu_quads_reset_to(uint32_t used)
{
    if (used <= s_arena_used) {
        s_arena_used = used;
    }
}

uint32_t kj_gpu_quads_dropped(void)
{
    const uint32_t d = s_arena_dropped;
    s_arena_dropped = 0;
    return d;
}

static void bind_base(const kj_v2d *base)
{
    if (s_bound_base == base) {
        return;
    }
    C3D_BufInfo *bi = C3D_GetBufInfo();
    BufInfo_Init(bi);
    BufInfo_Add(bi, base, sizeof(kj_v2d), 3, 0x210);
    s_bound_base = base;
}

void kj_gpu_draw_quads_from(const kj_v2d *base, uint32_t count)
{
    if (count == 0) {
        return;
    }
    bind_base(base);
    C3D_DrawElements(GPU_TRIANGLES, (int)(count * 6u), C3D_UNSIGNED_SHORT, s_quad_indices);
}

void kj_gpu_draw_quads(uint32_t first, uint32_t count)
{
    if (count == 0) {
        return;
    }
    bind_base(s_arena);
    C3D_DrawElements(GPU_TRIANGLES, (int)(count * 6u), C3D_UNSIGNED_SHORT,
                     s_quad_indices + first * 6u);
}

int kj_gpu_uloc_2d_xform(void)
{
    return s_uloc_xform;
}

int kj_gpu_uloc_2d_uvscale(void)
{
    return s_uloc_uvscale;
}

int kj_gpu_uloc_3d_palette(void)
{
    return s_uloc_palette;
}

int kj_gpu_uloc_3d_texscale(void)
{
    return s_uloc_texscale;
}

void kj_gpu_tev_texture_modulate(void)
{
    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
    for (int i = 1; i < 6; ++i) {
        C3D_TexEnvInit(C3D_GetTexEnv(i));
    }
}

void kj_gpu_tev_vertex_color(void)
{
    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    for (int i = 1; i < 6; ++i) {
        C3D_TexEnvInit(C3D_GetTexEnv(i));
    }
}

void kj_gpu_tev_constant(uint32_t rgba)
{
    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    C3D_TexEnvColor(env, rgba);
    for (int i = 1; i < 6; ++i) {
        C3D_TexEnvInit(C3D_GetTexEnv(i));
    }
}

void kj_gpu_scissor(int screen_w, int x0, int y0, int x1, int y1)
{
    /* Render targets are portrait (240 x screen_w): physical (x, y) is
     * framebuffer (240 - y, screen_w - x). */
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 > screen_w) {
        x1 = screen_w;
    }
    if (y1 > 240) {
        y1 = 240;
    }
    C3D_SetScissor(GPU_SCISSOR_NORMAL, (u32)(240 - y1), (u32)(screen_w - x1), (u32)(240 - y0),
                   (u32)(screen_w - x0));
}

void kj_gpu_scissor_off(void)
{
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
}
