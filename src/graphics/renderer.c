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
static bool s_skip_capture; /* committed before this picture's game work */

bool kj_ctr_capture_next_frame(void)
{
    /* Coffeepot frame-pacing.md: skip building an unstarted picture, keep
     * the tick, never twice in a row. Scripts require every captured frame. */
    s_skip_capture = kj_pacing_behind() >= 2u && !s_skipped_last && !kj_script_active();
#ifdef KJ_TEST_SKIP_CAPTURE
    /* Alternating dry ticks for scripted producer correctness QA. */
    s_skip_capture = !s_skipped_last;
#endif
    return !s_skip_capture;
}

/* GPU-side time per 600 pictures (coffeepot performance.md, "Per-operation
 * GPU times": P3D is command-list execution, PPF display transfers, PSC
 * memory fills; C3D_GetDrawingTime is sampled after the next FrameBegin
 * wait and associated with the preceding non-empty submission). */
static bool s_perflog_on;
static bool s_perflog_valid;
static uint64_t s_perflog_tick;
static GSPGPU_PerfLog s_perflog_prev;
static float s_draw_ms_sum;
static float s_draw_ms_max;
static float s_cmdbuf_max;
#ifndef HGSS_PROFILE_OFF
static bool s_queue_pending;
static uint32_t s_queue_game, s_queue_scene;
static uint32_t s_completed_game, s_completed_scene, s_completed_queues;
static uint32_t s_empty_submissions, s_same_scene, s_previous_scene;
static uint32_t s_window_first_game;
#endif

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
#ifndef HGSS_PROFILE_OFF
    s_perflog_on = R_SUCCEEDED(GSPGPU_SetPerfLogMode(true)) &&
                   R_SUCCEEDED(GSPGPU_GetPerfLog(&s_perflog_prev));
#endif
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
    kj_perf_publish_begin(in->g3 != NULL ? in->g3->serial : 0);
    /* The previous boundary committed this picture's capture decision
     * before game/geometry work. Apply that same decision here: a partial
     * dry scene must never be submitted after a late policy change.
     * Skip at most one picture in a row
     * (coffeepot PICA200/frame-pacing.md, "Skip the picture, keep the
     * tick"): an uncapped skip starved the screen on O3DS whenever the game
     * ran behind -- 1606 of 2206 Goldenrod pictures skipped, ~4 shown a
     * second while the game ran at half speed (o3ds-log-0, window 6). */
    if (s_skip_capture) {
        s_skipped_last = true;
#ifndef HGSS_PROFILE_OFF
        ++s_skipped_presents;
#endif
        kj_perf_present_skipped(in->game_frame);
        return;
    }
    s_skipped_last = false;

    uint64_t t = kj_perf_now();
    C3D_FrameBegin(0); /* waits for the previous frame's GPU work */
    kj_perf_add(KJ_PERF_GPU_WAIT, kj_perf_now() - t);
#ifndef HGSS_PROFILE_OFF
    /* Coffeepot performance.md: only after FrameBegin's queue wait does
     * drawing time belong to the previous non-empty submission. Empty
     * frames leave C3D's old timer unchanged; never count it again. */
    if (s_queue_pending) {
        const float draw_ms = C3D_GetDrawingTime();
        s_draw_ms_sum += draw_ms;
        if (draw_ms > s_draw_ms_max) {
            s_draw_ms_max = draw_ms;
        }
        ++s_completed_queues;
        s_completed_game = s_queue_game;
        s_completed_scene = s_queue_scene;
        s_queue_pending = false;
    }
#endif
    kj_gpu_quads_reset();
    kj_atlas_begin_frame();
    kj_g2d_begin_frame();
    kj_g3d_set_frame(in->g3);
    kj_tex_begin_frame(in);

    /* Engine A (and the 3D layer) is on the screen POWCNT1 says. */
    const int top_engine = in->engine_a_on_top ? 0 : 1;
    kj_screen_draw(KJ_SCREEN_TOP, top_engine, in);
    kj_screen_draw(KJ_SCREEN_BOTTOM, top_engine ^ 1, in);

#ifndef HGSS_PROFILE_OFF
    s_queue_pending = kj_target_get(KJ_SCREEN_TOP)->rt->used ||
                      kj_target_get(KJ_SCREEN_BOTTOM)->rt->used;
    s_empty_submissions += !s_queue_pending;
    s_queue_game = in->game_frame;
    s_queue_scene = in->g3 != NULL ? in->g3->serial : 0;
    if (s_frames % 600u == 0u) {
        s_window_first_game = in->game_frame;
    }
    s_same_scene += s_frames != 0u && s_queue_scene == s_previous_scene;
    s_previous_scene = s_queue_scene;
#endif
    t = kj_perf_now();
    C3D_FrameEnd(0);
    kj_perf_add(KJ_PERF_SUBMIT, kj_perf_now() - t);
#ifndef HGSS_PROFILE_OFF
    {
        const float usage = C3D_GetCmdBufUsage();

        if (usage > s_cmdbuf_max) {
            s_cmdbuf_max = usage;
        }
    }

#endif
#ifndef HGSS_PROFILE_OFF
    ++s_frames;
#endif
    kj_perf_publish_end(in->game_frame);
#ifndef HGSS_PROFILE_OFF
    if (s_frames % 600u == 0u) {
        kj_log("[scene] submissions %lu-%lu game %lu-%lu latest scene %lu; same scene %lu; "
               "empty submissions %lu; GPU queue complete game %lu scene %lu; "
               "LCD presentation unobserved (submission intervals are a proxy)",
               (unsigned long)(s_frames - 599u), (unsigned long)s_frames,
               (unsigned long)s_window_first_game, (unsigned long)in->game_frame,
               (unsigned long)s_queue_scene, (unsigned long)s_same_scene,
               (unsigned long)s_empty_submissions, (unsigned long)s_completed_game,
               (unsigned long)s_completed_scene);
        s_same_scene = s_empty_submissions = 0;
        kj_g2d_stats g;
        kj_g3d_stats d;
        kj_atlas_stats a;
        kj_g2d_take_stats(&g);
        kj_g3d_take_stats(&d);
        kj_atlas_take_stats(&a);
        kj_log("[gpu] 600 frames: 2D screens skipped %lu, layers reused %lu rebuilt %lu, "
               "bg quads %lu obj quads %lu draws %lu spans %lu unsupported %lu dropped %lu, "
               "char KiB compared %lu, layout segments %lu",
               (unsigned long)g.skipped_screens, (unsigned long)g.reused_layers,
               (unsigned long)g.rebuilt_layers, (unsigned long)g.bg_quads, (unsigned long)g.obj_quads,
               (unsigned long)g.draws, (unsigned long)g.spans, (unsigned long)g.unsupported_modes,
               (unsigned long)g.dropped_quads, (unsigned long)(g.char_bytes_compared / 1024u), (unsigned long)g.layout_segments);
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
        {
            /* Game-side probes (issue #11): per game frame, microseconds.
             * Slots are siblings of slot 0, so slot 0 minus the children is
             * the unattributed remainder. A window of zeros is not evidence
             * that nothing ran; check probe_calls and the scene content. */
            char line[160];
            int n = 0;
            n += snprintf(line, sizeof(line), "[probe] per game frame:");
            for (unsigned i = 0; i < KJ_PROBE_SLOTS; ++i) {
                n += snprintf(line + n, sizeof(line) - n, " %u: %.0f/%luus",
                              i, dp.probe_ticks[i] * us, (unsigned long)(dp.probe_calls[i] / gf));
            }
            kj_log("%s", line);
        }
        uint32_t late, resyncs;
        kj_pacing_stats(&late, &resyncs);
        kj_log("[pace] 600 frames: late V-blanks %lu, resyncs %lu, presents skipped %lu",
               (unsigned long)late, (unsigned long)resyncs, (unsigned long)s_skipped_presents);
        s_skipped_presents = 0;
        kj_log("[gpu] 600 frames: atlas lookups %lu conversions %lu evictions %lu full %lu; "
               "3D batches %lu draws %lu tris %lu state skips %lu "
               "joins %lu tried %lu joined %lu batches %lu declined %lu "
               "tex uploads %lu tex bytes compared %lu "
               "tex full %lu unsupported %lu; box tests %lu culled %lu (far only %lu) window passes %lu",
               (unsigned long)a.lookups, (unsigned long)a.conversions, (unsigned long)a.evictions,
               (unsigned long)a.full, (unsigned long)d.batches, (unsigned long)d.draws,
               (unsigned long)d.triangles, (unsigned long)d.state_skips,
                (unsigned long)d.join_groups, (unsigned long)d.joined_groups,
                (unsigned long)d.joined_batches, (unsigned long)d.join_declined,
                (unsigned long)d.tex_uploads,
               (unsigned long)d.tex_bytes_compared, (unsigned long)d.tex_cache_full,
               (unsigned long)d.unsupported, (unsigned long)d.box_tests,
               (unsigned long)d.box_culled, (unsigned long)d.box_culled_far,
               (unsigned long)d.window_passes);
        {
            GSPGPU_PerfLog now;
            unsigned long p3d = 0, ppf = 0, psc = 0;

            const uint64_t perf_tick = svcGetSystemTick();
            const uint64_t wall_us = (perf_tick - s_perflog_tick) * 1000000u / SYSCLOCK_ARM11;
            bool valid = false;
            if (s_perflog_on && R_SUCCEEDED(GSPGPU_GetPerfLog(&now))) {
                valid = s_perflog_valid;
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
                /* Each operation engine is serial. Its total cannot exceed
                 * the wall interval (PSC is the sum of two fill engines).
                 * Reject counter resets/torn reads instead of presenting a
                 * nearly-2^32 unsigned difference as GPU work. */
                if (p3d > wall_us / 600u + 1u || ppf > wall_us / 600u + 1u ||
                    psc > 2u * (wall_us / 600u + 1u)) {
                    valid = false;
                }
                s_perflog_tick = perf_tick;
                s_perflog_prev = now;
                s_perflog_valid = true;
            } else {
                s_perflog_valid = false;
            }
            /* Discard the first window: GSP start timestamps survive enable,
             * and the first observed P3D total was nonsensical on O3DS log 3.
             * A failed read also requires a fresh baseline. */
            if (!valid) p3d = ppf = psc = 0;
            kj_log("[gputime] 600 pictures: GSP us/picture P3D %lu PPF %lu PSC %lu (log %s); "
                   "C3D completed queues %lu drawing ms avg %.2f max %.2f; cmdbuf use max %u%%",
                   p3d, ppf, psc, valid ? "valid" : "baseline/unavailable",
                   (unsigned long)s_completed_queues,
                   (double)(s_completed_queues != 0u ? s_draw_ms_sum / s_completed_queues : 0.0f),
                   (double)s_draw_ms_max, (unsigned)(s_cmdbuf_max * 100.0f));
            s_draw_ms_sum = 0.0f;
            s_draw_ms_max = 0.0f;
            s_cmdbuf_max = 0.0f;
            s_completed_queues = 0;
        }
    }

#endif
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
