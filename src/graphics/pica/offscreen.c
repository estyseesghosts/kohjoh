/*
 * offscreen.c: persistent render-to-texture surface for 2x field rendering.
 *
 * Coffeepot CTR/PICA200/gpu-rendering.md, "Render to texture": use
 * C3D_RenderTargetCreateFromTex() and never sample a texture in the same pass
 * that renders into it. The 256x128 backing texture is the smallest PICA
 * power-of-two allocation that holds the 200x120 active region. Colour is
 * RGBA8 and depth is D24S8 so the existing 3D/stencil path can render into it
 * unchanged; that is 256 KiB before overhead.
 */

#include "graphics/pica/offscreen.h"

#include "diagnostics/log.h"
#include "graphics/pica/gpu.h"
#include "graphics/world_view.h"

static C3D_Tex s_tex;
static C3D_RenderTarget *s_target;
static bool s_ready;

bool kj_offscreen_init(void)
{
    if (!C3D_TexInitVRAM(&s_tex, KJ_OFFSCREEN_W, KJ_OFFSCREEN_H, GPU_RGBA8)) {
        kj_log("[gpu] 2x: cannot allocate 128x256 world texture (VRAM free %lu)",
               (unsigned long)vramSpaceFree());
        return false;
    }
    C3D_TexSetFilter(&s_tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&s_tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    s_target = C3D_RenderTargetCreateFromTex(&s_tex, GPU_TEX_2D, 0, GPU_RB_DEPTH24_STENCIL8);
    if (s_target == NULL) {
        kj_log("[gpu] 2x: cannot attach depth/stencil to world texture (VRAM free %lu)",
               (unsigned long)vramSpaceFree());
        C3D_TexDelete(&s_tex);
        return false;
    }
    s_ready = true;
    kj_log("[gpu] 2x world surface ready: %dx%d active in %dx%d backing (VRAM free %lu)",
           KJ_OFFSCREEN_ACTIVE_W, KJ_OFFSCREEN_ACTIVE_H, KJ_OFFSCREEN_W, KJ_OFFSCREEN_H,
           (unsigned long)vramSpaceFree());
    return true;
}

void kj_offscreen_exit(void)
{
    if (s_target != NULL) {
        C3D_RenderTargetDelete(s_target);
        s_target = NULL;
    }
    if (s_ready) {
        C3D_TexDelete(&s_tex);
    }
    s_ready = false;
}

bool kj_offscreen_ready(void)
{
    return s_ready;
}

C3D_RenderTarget *kj_offscreen_target(void)
{
    return s_target;
}

C3D_Tex *kj_offscreen_texture(void)
{
    return &s_tex;
}

void kj_offscreen_begin_world(uint32_t clear_color)
{
    C3D_RenderTargetClear(s_target, C3D_CLEAR_ALL, clear_color, 0);
    C3D_FrameDrawOn(s_target);
    /* FrameDrawOn installs a viewport covering the whole 128x256 texture.
     * The world projection is built for the 120x200 active region, so without
     * this override clip space is stretched into the padding and then cropped
     * by the composite -- a compressed, distorted picture. */
    C3D_SetViewport(0, 0, KJ_OFFSCREEN_ACTIVE_W, KJ_OFFSCREEN_ACTIVE_H);
    /* The active region starts at the texture origin; clip to it so the
     * padding can never receive world geometry. */
    kj_gpu_scissor_target(KJ_OFFSCREEN_LOGICAL_W, KJ_OFFSCREEN_LOGICAL_H, 0, 0,
                          KJ_OFFSCREEN_LOGICAL_W, KJ_OFFSCREEN_LOGICAL_H);
    kj_gpu_invalidate();
}

void kj_offscreen_end_world(C3D_RenderTarget *native)
{
    kj_gpu_scissor_off();
    kj_gpu_invalidate(); /* GPU state invalidation between the two surfaces */
    C3D_FrameDrawOn(native);
    kj_gpu_invalidate();
}
