/*
 * renderer.c: frame ownership and submission (3DS side).
 *
 * kj_ctr_publish_frame runs on the game thread at the frame boundary. It
 * waits for the previous GPU frame (so every GPU-visible buffer the
 * renderer owns is free to rewrite), translates the coherent DS state into
 * PICA commands for each screen, and submits. The game then continues while
 * the GPU works; nothing reads DS memory after this function returns.
 *
 * Screen mapping (kohjoh.md 0.4): top 400x240 shows a 320x192 logical canvas
 * (DS 256x192 plus 32 logical columns each side) at 1.25x; bottom 320x240
 * shows the DS's 256x192 at 1.25x. Only the screen whose engine output
 * changed is redrawn and transferred.
 */

#include "graphics/renderer.h"

#include "app/script.h"
#include "diagnostics/dump.h"
#include "diagnostics/log.h"
#include "diagnostics/perf.h"
#include "graphics/g2d/g2d.h"
#include "graphics/g3d/g3d.h"
#include "graphics/g3d/textures.h"
#include "graphics/pica/gpu.h"
#include "graphics/pica/targets.h"
#include "graphics/resources/atlas.h"
#include "runtime/kj_bridge.h"
#include "runtime/kj_g3.h"

#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define CMDBUF_BYTES (512 * 1024)

static bool s_ready;
static uint32_t s_frames;

bool kj_renderer_init(void)
{
    if (!C3D_Init(CMDBUF_BYTES)) {
        kj_log("[gpu] C3D_Init failed");
        return false;
    }
    if (!kj_targets_init()) {
        return false;
    }
    {
        kj_g3_storage st;
        for (int i = 0; i < 2; ++i) {
            st.verts[i] = linearAlloc(sizeof(kj_g3_vertex) * KJ_G3_MAX_VERTS);
            st.indices[i] = linearAlloc(sizeof(uint16_t) * KJ_G3_MAX_INDICES);
            if (st.verts[i] == NULL || st.indices[i] == NULL) {
                kj_log("[gpu] out of linear memory for 3D storage");
                return false;
            }
        }
        kj_g3_init(&st);
        kj_g3_set_wide(true);
    }
    if (!kj_g2d_init() || !kj_g3d_init()) {
        return false;
    }
    s_ready = true;
    kj_log("[gpu] renderer ready (linear free %lu KiB, VRAM free %lu KiB)",
           (unsigned long)(linearSpaceFree() / 1024), (unsigned long)(vramSpaceFree() / 1024));
    return true;
}

void kj_renderer_exit(void)
{
    if (!s_ready) {
        return;
    }
    s_ready = false;
    C3D_FrameBegin(0); /* waits for the GPU */
    C3D_FrameEnd(0);
    kj_g3d_exit();
    kj_g2d_exit();
    kj_targets_exit();
    C3D_Fini();
}

void kj_ctr_publish_frame(const kj_frame_input *in)
{
    if (!s_ready) {
        return;
    }
    kj_perf_publish_begin();

    uint64_t t = kj_perf_now();
    C3D_FrameBegin(0); /* waits for the previous frame's GPU work */
    kj_perf_add(KJ_PERF_GPU_WAIT, kj_perf_now() - t);
    kj_gpu_quads_reset();
    kj_atlas_begin_frame();
    kj_g3d_set_frame(in->g3);
    kj_tex_begin_frame(in);

    /* Engine A (and the 3D layer) is on the screen POWCNT1 says. */
    const int top_engine = in->engine_a_on_top ? 0 : 1;
    kj_screen_draw(KJ_SCREEN_TOP, top_engine, in);
    kj_screen_draw(KJ_SCREEN_BOTTOM, top_engine ^ 1, in);

    t = kj_perf_now();
    C3D_FrameEnd(0);
    kj_perf_add(KJ_PERF_SUBMIT, kj_perf_now() - t);

    ++s_frames;
    kj_perf_publish_end();

    /* Test directives: deterministic per-frame capture. */
    if (kj_script_dump(in->game_frame)) {
        char path[96];
        mkdir("sdmc:/3ds/kohjoh/dumps", 0777);
        snprintf(path, sizeof(path), "sdmc:/3ds/kohjoh/dumps/f%05lu.kjd", (unsigned long)in->game_frame);
        kj_dump_frame(path, in);
    }
    const uint32_t hold = kj_script_freeze(in->game_frame);
    if (hold > 0) {
        kj_log("[script] holding game frame %lu for %lu s", (unsigned long)in->game_frame,
               (unsigned long)hold);
        for (uint32_t v = 0; v < hold * 60u && aptMainLoop(); ++v) {
            gspWaitForVBlank();
        }
    }
}
