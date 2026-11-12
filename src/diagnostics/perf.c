/*
 * perf.c: bounded in-memory frame profiler.
 *
 * Every publish records its stage ticks into a 600-publish window; at the
 * end of each window one aggregate (p50/p95/p99/max per stage, publish
 * interval in V-blanks, deadline misses) goes to the log ring, which is
 * written to SD off the hot path. A deadline miss is a publish more than two
 * refreshes after the previous one (the 30 FPS contract).
 */

#include "diagnostics/perf.h"

#include "diagnostics/log.h"

#include <3ds.h>
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
    kj_log("[perf] window %lu: %u publishes, interval (vblanks) p50 %lu max %lu, misses %lu",
           (unsigned long)++s_windows, WINDOW, (unsigned long)sorted[WINDOW / 2],
           (unsigned long)sorted[WINDOW - 1], (unsigned long)s_misses);
    s_misses = 0;
}

void kj_perf_publish_end(void)
{
    const uint64_t now = svcGetSystemTick();
    kj_perf_add(KJ_PERF_PUBLISH, now - s_publish_start);
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
