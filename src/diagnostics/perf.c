/*
 * perf.c: bounded in-memory frame profiler.
 *
 * Every publish records its stage ticks into a 600-publish window; at the
 * end of each window one aggregate (p50/p95/p99/max per stage, publish
 * interval in V-blanks, deadline misses) goes to the log ring, which is
 * written to SD off the hot path. A deadline miss is a publish more than two
 * refreshes after the previous one (the 30 FPS contract).
 *
 * Two intervals are kept. "emulated" counts DS V-blanks between publishes
 * and is 2 by construction under the pacing lock. "real" counts physical
 * V-blanks (C3D_FrameCounter) between submissions: that is what the player
 * sees. A real interval other than 2 is a hitch (a picture held for 3+
 * refreshes, or one shown for a single refresh while catching up); the
 * first hitches of each window are logged with their game frame so a
 * stutter can be located in a scripted run.
 */

#include "diagnostics/perf.h"

#include "diagnostics/log.h"

#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW 600u
#define TICKS_PER_US (SYSCLOCK_ARM11 / 1000000u)

static uint32_t s_samples[KJ_PERF_STAGE_COUNT][WINDOW];
static uint32_t s_frame_stage[KJ_PERF_STAGE_COUNT];
static uint32_t s_interval[WINDOW];
static unsigned s_n;
static uint64_t s_vblank_start, s_publish_start, s_logic_start;
static uint32_t s_vblanks_since_publish, s_misses, s_windows;
static bool s_any_publish;
static uint32_t s_real_interval[WINDOW];
static uint32_t s_last_real;
static uint64_t s_last_submit;
static uint32_t s_frame_time[WINDOW];
#define MAX_HITCH_LOG 12u
static uint32_t s_hitch_frame[MAX_HITCH_LOG], s_hitch_len[MAX_HITCH_LOG];
static uint32_t s_hitch_logic[MAX_HITCH_LOG], s_hitch_g2d[MAX_HITCH_LOG], s_hitch_g3d[MAX_HITCH_LOG];
static uint32_t s_hitches, s_skipped;
uint32_t kj_perf_counts[KJ_COUNT_COUNT];
static uint32_t s_hitch_counts[MAX_HITCH_LOG][KJ_COUNT_COUNT];

uint64_t kj_perf_now(void)
{
    return svcGetSystemTick();
}

void kj_perf_add(enum kj_perf_stage stage, uint64_t ticks)
{
    s_frame_stage[stage] += (uint32_t)ticks;
}

void kj_perf_vblank_begin(void)
{
    s_vblank_start = svcGetSystemTick();
    if (s_logic_start != 0) {
        kj_perf_add(KJ_PERF_LOGIC, s_vblank_start - s_logic_start);
    }
}

void kj_perf_vblank_end(void)
{
    const uint64_t now = svcGetSystemTick();
    kj_perf_add(KJ_PERF_VBLANK_WAIT, now - s_vblank_start);
    s_logic_start = now;
    ++s_vblanks_since_publish;
}

void kj_perf_publish_begin(void)
{
    s_publish_start = svcGetSystemTick();
    if (s_logic_start != 0) {
        kj_perf_add(KJ_PERF_LOGIC, s_publish_start - s_logic_start);
    }
}

static int cmp_u32(const void *a, const void *b)
{
    const uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return x < y ? -1 : x > y;
}

static void report(void)
{
    static const char *const names[KJ_PERF_STAGE_COUNT] = {
        "logic", "vbl_wait", "g2d", "g3d", "gpu_wait", "submit", "publish",
    };
    static uint32_t sorted[WINDOW];

    for (int s = 0; s < KJ_PERF_STAGE_COUNT; ++s) {
        memcpy(sorted, s_samples[s], sizeof(sorted));
        qsort(sorted, WINDOW, sizeof(sorted[0]), cmp_u32);
        kj_log("[perf] %-8s us p50 %5lu p95 %5lu p99 %5lu max %5lu", names[s],
               (unsigned long)(sorted[WINDOW / 2] / TICKS_PER_US),
               (unsigned long)(sorted[WINDOW * 95 / 100] / TICKS_PER_US),
               (unsigned long)(sorted[WINDOW * 99 / 100] / TICKS_PER_US),
               (unsigned long)(sorted[WINDOW - 1] / TICKS_PER_US));
    }
    memcpy(sorted, s_interval, sizeof(sorted));
    qsort(sorted, WINDOW, sizeof(sorted[0]), cmp_u32);
    kj_log("[perf] window %lu: %u publishes, emulated interval (vblanks) p50 %lu max %lu, misses %lu",
           (unsigned long)++s_windows, WINDOW, (unsigned long)sorted[WINDOW / 2],
           (unsigned long)sorted[WINDOW - 1], (unsigned long)s_misses);
    s_misses = 0;
    {
        uint32_t hist[5] = {0, 0, 0, 0, 0};
        for (unsigned i = 0; i < WINDOW; ++i) {
            const uint32_t v = s_real_interval[i];
            ++hist[v >= 4 ? 4 : v];
        }
        memcpy(sorted, s_frame_time, sizeof(sorted));
        qsort(sorted, WINDOW, sizeof(sorted[0]), cmp_u32);
        kj_log("[frame] real interval (refreshes) 0:%lu 1:%lu 2:%lu 3:%lu 4+:%lu; hitches %lu, "
               "presents skipped %lu; frame time us p50 %lu p99 %lu max %lu",
               (unsigned long)hist[0], (unsigned long)hist[1], (unsigned long)hist[2],
               (unsigned long)hist[3], (unsigned long)hist[4], (unsigned long)s_hitches,
               (unsigned long)s_skipped, (unsigned long)(sorted[WINDOW / 2] / TICKS_PER_US),
               (unsigned long)(sorted[WINDOW * 99 / 100] / TICKS_PER_US),
               (unsigned long)(sorted[WINDOW - 1] / TICKS_PER_US));
        const uint32_t shown = s_hitches < MAX_HITCH_LOG ? s_hitches : MAX_HITCH_LOG;
        if (shown > 0) {
            char line[640];
            int n = snprintf(line, sizeof(line),
                             "[frame] hitches, game frame(refreshes logic/g2d/g3d ms):");
            for (uint32_t i = 0; i < shown && n > 0 && n < (int)sizeof(line); ++i) {
                n += snprintf(line + n, sizeof(line) - (size_t)n, " %lu(%lu %lu/%lu/%lu)",
                              (unsigned long)s_hitch_frame[i], (unsigned long)s_hitch_len[i],
                              (unsigned long)(s_hitch_logic[i] / TICKS_PER_US / 1000u),
                              (unsigned long)(s_hitch_g2d[i] / TICKS_PER_US / 1000u),
                              (unsigned long)(s_hitch_g3d[i] / TICKS_PER_US / 1000u));
            }
            kj_log("%s", line);
            for (uint32_t i = 0; i < shown; ++i) {
                const uint32_t *k = s_hitch_counts[i];
                kj_log("[frame]   %lu: tile conv %lu, spans %lu, quads %lu, tex uploads %lu, "
                       "tex bytes compared %lu", (unsigned long)s_hitch_frame[i],
                       (unsigned long)k[KJ_COUNT_TILE_CONV], (unsigned long)k[KJ_COUNT_SPANS],
                       (unsigned long)k[KJ_COUNT_QUADS], (unsigned long)k[KJ_COUNT_TEX_UPLOAD],
                       (unsigned long)k[KJ_COUNT_TEX_BYTES]);
            }
        }
        s_hitches = 0;
        s_skipped = 0;
    }
}

static void note_hitch(uint32_t game_frame, uint32_t refreshes)
{
    if (s_hitches < MAX_HITCH_LOG) {
        s_hitch_frame[s_hitches] = game_frame;
        s_hitch_len[s_hitches] = refreshes;
        s_hitch_logic[s_hitches] = s_frame_stage[KJ_PERF_LOGIC];
        s_hitch_g2d[s_hitches] = s_frame_stage[KJ_PERF_G2D];
        s_hitch_g3d[s_hitches] = s_frame_stage[KJ_PERF_G3D];
        memcpy(s_hitch_counts[s_hitches], kj_perf_counts, sizeof(kj_perf_counts));
    }
    ++s_hitches;
}

void kj_perf_present_skipped(uint32_t game_frame)
{
    ++s_skipped;
    note_hitch(game_frame, 0);
}

void kj_perf_publish_end(uint32_t game_frame)
{
    const uint64_t now = svcGetSystemTick();
    const uint32_t real = C3D_FrameCounter(0);
    const uint32_t real_interval = s_any_publish ? real - s_last_real : 2u;
    s_real_interval[s_n] = real_interval;
    s_frame_time[s_n] = s_any_publish ? (uint32_t)(now - s_last_submit) : 0u;
    s_last_real = real;
    s_last_submit = now;
    if (s_any_publish && real_interval != 2u) {
        note_hitch(game_frame, real_interval);
    }
    kj_perf_add(KJ_PERF_PUBLISH, now - s_publish_start);
    memset(kj_perf_counts, 0, sizeof(kj_perf_counts));
    for (int s = 0; s < KJ_PERF_STAGE_COUNT; ++s) {
        s_samples[s][s_n] = s_frame_stage[s];
        s_frame_stage[s] = 0;
    }
    s_interval[s_n] = s_vblanks_since_publish;
    if (s_any_publish && s_vblanks_since_publish > 2) {
        ++s_misses;
    }
    s_any_publish = true;
    s_vblanks_since_publish = 0;
    s_logic_start = now;
    if (++s_n == WINDOW) {
        s_n = 0;
        report();
    }
}
