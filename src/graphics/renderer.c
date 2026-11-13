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

#include "app/app.h"
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
static uint32_t s_skipped_presents;
static bool s_skipped_last; /* the previous publish skipped its picture */

/* GPU-side time per 600 pictures (coffeepot performance.md, "Per-operation
 * GPU times": P3D is command-list execution, PPF display transfers, PSC
 * memory fills; C3D_GetDrawingTime is the latest completed queue, so it is
 * averaged, never matched to the current picture). */
static bool s_perflog_on;
static GSPGPU_PerfLog s_perflog_prev;
static float s_draw_ms_sum;
static float s_draw_ms_max;
static float s_cmdbuf_max;

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
    s_perflog_on = R_SUCCEEDED(GSPGPU_SetPerfLogMode(true)) &&
                   R_SUCCEEDED(GSPGPU_GetPerfLog(&s_perflog_prev));
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
    /* Running two or more refreshes behind: this picture would be shown
     * late anyway. Skip its GPU work (the game tick already happened) so
     * the schedule recovers; counted, never silent. Never two in a row
     * (coffeepot PICA200/frame-pacing.md, "Skip the picture, keep the
     * tick"): an uncapped skip starved the screen on O3DS whenever the game
     * ran behind -- 1606 of 2206 Goldenrod pictures skipped, ~4 shown a
     * second while the game ran at half speed (o3ds-log-0, window 6). */
    if (kj_pacing_behind() >= 2u && !s_skipped_last && !kj_script_active()) {
        s_skipped_last = true;
        ++s_skipped_presents;
        kj_perf_present_skipped(in->game_frame);
        return;
    }
    s_skipped_last = false;
    kj_perf_publish_begin();

    uint64_t t = kj_perf_now();
    C3D_FrameBegin(0); /* waits for the previous frame's GPU work */
    kj_perf_add(KJ_PERF_GPU_WAIT, kj_perf_now() - t);
    kj_gpu_quads_reset();
    kj_atlas_begin_frame();
    kj_g2d_begin_frame();
    kj_g3d_set_frame(in->g3);
    kj_tex_begin_frame(in);

    /* Engine A (and the 3D layer) is on the screen POWCNT1 says. */
    const int top_engine = in->engine_a_on_top ? 0 : 1;
    kj_screen_draw(KJ_SCREEN_TOP, top_engine, in);
    kj_screen_draw(KJ_SCREEN_BOTTOM, top_engine ^ 1, in);

    t = kj_perf_now();
    C3D_FrameEnd(0);
    kj_perf_add(KJ_PERF_SUBMIT, kj_perf_now() - t);
    {
        const float draw_ms = C3D_GetDrawingTime();
        const float usage = C3D_GetCmdBufUsage();

        s_draw_ms_sum += draw_ms;
        if (draw_ms > s_draw_ms_max) {
            s_draw_ms_max = draw_ms;
        }
        if (usage > s_cmdbuf_max) {
            s_cmdbuf_max = usage;
        }
    }

    ++s_frames;
    kj_perf_publish_end(in->game_frame);
    if (s_frames % 600u == 0u) {
        kj_g2d_stats g;
        kj_g3d_stats d;
        kj_atlas_stats a;
        kj_g2d_take_stats(&g);
        kj_g3d_take_stats(&d);
        kj_atlas_take_stats(&a);
        kj_log("[gpu] 600 frames: 2D screens skipped %lu, layers reused %lu rebuilt %lu, "
               "bg quads %lu obj quads %lu draws %lu spans %lu unsupported %lu dropped %lu, "
               "char KiB compared %lu",
               (unsigned long)g.skipped_screens, (unsigned long)g.reused_layers,
               (unsigned long)g.rebuilt_layers, (unsigned long)g.bg_quads, (unsigned long)g.obj_quads,
               (unsigned long)g.draws, (unsigned long)g.spans, (unsigned long)g.unsupported_modes,
               (unsigned long)g.dropped_quads, (unsigned long)(g.char_bytes_compared / 1024u));
        kj_ds_profile dp;
        kj_ds_take_profile(&dp);
        /* Per game frame: one H-blank walk runs per game frame, so walks is
         * the game-frame denominator even when pictures were skipped. */
        const uint32_t gf = dp.hblank_walks != 0u ? dp.hblank_walks : 600u;
        const double us = 1000000.0 / SYSCLOCK_ARM11 / (double)gf;
        kj_log("[ds] per game frame (%lu game frames / 600 pictures): g3 fifo calls %llu "
               "(%lu fifo words, %lu direct words, %lu cmds), timer sync %.0f us (%llu calls), "
               "sound pump %.0f us, hblank %.0f us",
               (unsigned long)gf, (unsigned long long)(dp.g3_fifo_ticks / gf),
               (unsigned long)(dp.g3_fifo_words / gf), (unsigned long)(dp.g3_direct_words / gf),
               (unsigned long)(dp.g3_commands / gf), dp.timer_sync_ticks * us,
               (unsigned long long)(dp.timer_sync_calls / gf), dp.sound_pump_ticks * us,
               dp.hblank_ticks * us);
        kj_log("[ds] display lists: %lu replayed, %lu decoded, %lu uncacheable, %lu KiB retained",
               (unsigned long)dp.dl_hits, (unsigned long)dp.dl_misses,
               (unsigned long)dp.dl_uncacheable, (unsigned long)(dp.dl_bytes / 1024u));
        uint32_t late, resyncs;
        kj_pacing_stats(&late, &resyncs);
        kj_log("[pace] 600 frames: late V-blanks %lu, resyncs %lu, presents skipped %lu",
               (unsigned long)late, (unsigned long)resyncs, (unsigned long)s_skipped_presents);
        s_skipped_presents = 0;
        kj_log("[gpu] 600 frames: atlas lookups %lu conversions %lu evictions %lu full %lu; "
               "3D batches %lu draws %lu tris %lu tex uploads %lu tex bytes compared %lu "
               "tex full %lu unsupported %lu; box tests %lu culled %lu (far only %lu)",
               (unsigned long)a.lookups, (unsigned long)a.conversions, (unsigned long)a.evictions,
               (unsigned long)a.full, (unsigned long)d.batches, (unsigned long)d.draws,
               (unsigned long)d.triangles, (unsigned long)d.tex_uploads,
               (unsigned long)d.tex_bytes_compared, (unsigned long)d.tex_cache_full,
               (unsigned long)d.unsupported, (unsigned long)d.box_tests,
               (unsigned long)d.box_culled, (unsigned long)d.box_culled_far);
        {
            GSPGPU_PerfLog now;
            unsigned long p3d = 0, ppf = 0, psc = 0;

            if (s_perflog_on && R_SUCCEEDED(GSPGPU_GetPerfLog(&now))) {
                /* Differences of the running totals (unsigned, so a wrap
                 * still subtracts correctly), per picture. */
                p3d = (now.entries[GSPGPU_EVENT_P3D].totalDurationUs -
                       s_perflog_prev.entries[GSPGPU_EVENT_P3D].totalDurationUs) / 600u;
                ppf = (now.entries[GSPGPU_EVENT_PPF].totalDurationUs -
                       s_perflog_prev.entries[GSPGPU_EVENT_PPF].totalDurationUs) / 600u;
                psc = (now.entries[GSPGPU_EVENT_PSC0].totalDurationUs -
                       s_perflog_prev.entries[GSPGPU_EVENT_PSC0].totalDurationUs +
                       now.entries[GSPGPU_EVENT_PSC1].totalDurationUs -
                       s_perflog_prev.entries[GSPGPU_EVENT_PSC1].totalDurationUs) / 600u;
                s_perflog_prev = now;
            }
            kj_log("[gputime] 600 pictures: GSP us/picture P3D %lu PPF %lu PSC %lu (log %s); "
                   "C3D drawing ms avg %.2f max %.2f; cmdbuf use max %u%%",
                   p3d, ppf, psc, s_perflog_on ? "on" : "off", (double)(s_draw_ms_sum / 600.0f),
                   (double)s_draw_ms_max, (unsigned)(s_cmdbuf_max * 100.0f));
            s_draw_ms_sum = 0.0f;
            s_draw_ms_max = 0.0f;
            s_cmdbuf_max = 0.0f;
        }
    }

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
