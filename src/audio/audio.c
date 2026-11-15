/*
 * audio.c: DS sound output through NDSP.
 *
 * The compat layer runs the original ARM7 sequencer and the DS sound chip
 * model; NitroPort_SoundRender mixes 32768 Hz stereo and is safe to call
 * from another thread (it takes its own lock; alarm replies are pumped on
 * the game thread). A dedicated audio thread on the application core, at a
 * higher priority than the game, refills NDSP wave buffers when the DSP
 * signals a finished frame, so audio never waits on the GPU or on loads.
 * Core 1 is deliberately unused (kohjoh.md 5.5: it is a shared system core).
 */

#include "audio/audio.h"

#include "diagnostics/log.h"

#include <3ds.h>
#include <stdint.h>
#include <string.h>

void NitroPort_SoundRender(int16_t *stereo, size_t frames);
void NitroPort_SoundSetInterpolation(int enabled);

/* Held samples, as the DS plays them, unless the build asks for linear
 * interpolation (--define KJ_SND_INTERPOLATE; see port_snd.c). */
#ifdef KJ_SND_INTERPOLATE
#define SOUND_INTERPOLATE 1
#else
#define SOUND_INTERPOLATE 0
#endif

#define SAMPLE_RATE 32768.0f
#define BUFFER_FRAMES 512u /* 15.6 ms per wave buffer */
#define BUFFER_COUNT 4u
#define BUFFER_BYTES (BUFFER_FRAMES * 4u)

static ndspWaveBuf s_wave[BUFFER_COUNT];
static int16_t *s_data;
static Thread s_thread;
static LightEvent s_event;
static volatile bool s_running;
static bool s_ready;
/* Wake-ups that found every buffer played: the DSP had nothing left to
 * play. Not counted before the first buffers are queued (the old counter
 * reported that start-up wake-up as an underrun in every log). */
static uint32_t s_underruns;
static bool s_primed;

#ifndef HGSS_PROFILE_OFF
/*
 * Profile builds: the refill deadline (coffeepot ctr/engineering/audio.md,
 * "Mixing deadlines": log the time from refill needed to buffer queued, the
 * queue's low-water mark and the DSP's dropped frames) and, where the kernel
 * allows it, the ARM11 performance counters around the mixer
 * (ctr/engineering/cpu-optimization.md, "Hardware performance counters":
 * they work from a .3dsx under Luma3DS on an Old 3DS; they count the whole
 * core, so they are read around the render call on this thread only).
 */
#define REPORT_FILLS 600u
static uint32_t s_fills;
static uint32_t s_wakes;
static uint32_t s_low_water_frames;
static uint64_t s_refill_ticks;
static uint64_t s_refill_max;
static uint64_t s_render_ticks;
static uint64_t s_render_max;
static uint32_t s_dropped_base;
static bool s_pmu;
static uint64_t s_pmu_cycles;
static uint64_t s_pmu_icache_miss;
static uint64_t s_pmu_dcache_read_miss;

#define PMU_REGS (BIT(PERFCOUNTERREG_CORE_COUNT_REG_0) | BIT(PERFCOUNTERREG_CORE_COUNT_REG_1) | \
                  BIT(PERFCOUNTERREG_CORE_CYCLE_COUNTER))

static void pmu_begin(void)
{
    u64 unused;

    if (R_FAILED(svcControlPerformanceCounter(&unused, PERFCOUNTEROP_ENABLE, 0, 0))) {
        kj_log("[sndout] ARM11 performance counters unavailable");
        return;
    }
    svcControlPerformanceCounter(&unused, PERFCOUNTEROP_SET_VIRTUAL_COUNTER_ENABLED, true, 0);
    svcControlPerformanceCounter(&unused, PERFCOUNTEROP_SET_EVENT, PERFCOUNTERREG_CORE_COUNT_REG_0,
                                 PERFCOUNTEREVT_CORE_INST_CACHE_MISS);
    svcControlPerformanceCounter(&unused, PERFCOUNTEROP_SET_EVENT, PERFCOUNTERREG_CORE_COUNT_REG_1,
                                 PERFCOUNTEREVT_CORE_DATA_CACHE_READ_MISS);
    svcControlPerformanceCounter(&unused, PERFCOUNTEROP_SET_EVENT,
                                 PERFCOUNTERREG_CORE_CYCLE_COUNTER,
                                 PERFCOUNTEREVT_CORE_CYCLE_COUNT);
    svcControlPerformanceCounter(&unused, PERFCOUNTEROP_RESET, PMU_REGS, PMU_REGS);
    s_pmu = true;
}

static void pmu_read(u64 *cycles, u64 *icache, u64 *dcache)
{
    svcControlPerformanceCounter(cycles, PERFCOUNTEROP_GET_VALUE,
                                 PERFCOUNTERREG_CORE_CYCLE_COUNTER, 0);
    svcControlPerformanceCounter(icache, PERFCOUNTEROP_GET_VALUE,
                                 PERFCOUNTERREG_CORE_COUNT_REG_0, 0);
    svcControlPerformanceCounter(dcache, PERFCOUNTEROP_GET_VALUE,
                                 PERFCOUNTERREG_CORE_COUNT_REG_1, 0);
}

static void pmu_end(void)
{
    u64 unused;

    if (s_pmu) {
        svcControlPerformanceCounter(&unused, PERFCOUNTEROP_DISABLE, 0, 0);
        s_pmu = false;
    }
}

/* Frames still to play: queued buffers, less what the DSP has played of the
 * one it is on. */
static uint32_t queued_frames(void)
{
    uint32_t frames = 0;

    for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
        if (s_wave[i].status == NDSP_WBUF_QUEUED || s_wave[i].status == NDSP_WBUF_PLAYING) {
            frames += BUFFER_FRAMES;
        }
    }
    if (frames != 0) {
        const uint32_t played = ndspChnGetSamplePos(0);

        frames -= played < frames ? played : frames;
    }
    return frames;
}

static void report(void)
{
    const uint32_t dropped = ndspGetDroppedFrames();

    kj_log("[sndout] fills %lu wakes %lu underruns %lu low_water_frames %lu dsp_dropped %lu "
           "refill_us avg %llu max %llu render_us avg %llu max %llu",
           (unsigned long)s_fills, (unsigned long)s_wakes, (unsigned long)s_underruns,
           (unsigned long)s_low_water_frames, (unsigned long)(dropped - s_dropped_base),
           (unsigned long long)(s_refill_ticks / (s_wakes ? s_wakes : 1u) / 268u),
           (unsigned long long)(s_refill_max / 268u),
           (unsigned long long)(s_render_ticks / (s_fills ? s_fills : 1u) / 268u),
           (unsigned long long)(s_render_max / 268u));
    if (s_pmu) {
        kj_log("[sndpmu] mixer cycles %llu icache_miss %llu dcache_read_miss %llu",
               (unsigned long long)s_pmu_cycles, (unsigned long long)s_pmu_icache_miss,
               (unsigned long long)s_pmu_dcache_read_miss);
    }
    /* Per-window figures restart; the PMU totals stay cumulative, as
     * [sndprof]'s mixer ticks are. */
    s_low_water_frames = UINT32_MAX;
    s_refill_max = 0;
    s_render_max = 0;
}
#endif

static void ndsp_frame_callback(void *arg)
{
    (void)arg;
    LightEvent_Signal(&s_event);
}

static void fill(ndspWaveBuf *wb)
{
#ifndef HGSS_PROFILE_OFF
    u64 c0 = 0, i0 = 0, d0 = 0;
    const u64 start = svcGetSystemTick();

    if (s_pmu) {
        pmu_read(&c0, &i0, &d0);
    }
#endif
    NitroPort_SoundRender(wb->data_pcm16, BUFFER_FRAMES);
#ifndef HGSS_PROFILE_OFF
    {
        const u64 ticks = svcGetSystemTick() - start;

        if (s_pmu) {
            u64 c1, i1, d1;

            pmu_read(&c1, &i1, &d1);
            s_pmu_cycles += c1 - c0;
            s_pmu_icache_miss += i1 - i0;
            s_pmu_dcache_read_miss += d1 - d0;
        }
        s_render_ticks += ticks;
        if (ticks > s_render_max) {
            s_render_max = ticks;
        }
        ++s_fills;
    }
#endif
    DSP_FlushDataCache(wb->data_pcm16, BUFFER_BYTES);
    ndspChnWaveBufAdd(0, wb);
}

static void audio_thread(void *arg)
{
    (void)arg;
    while (s_running) {
        LightEvent_Wait(&s_event);
        unsigned queued = 0;
#ifndef HGSS_PROFILE_OFF
        const u64 wake = svcGetSystemTick();
        const uint32_t left = queued_frames();

        if (s_primed && left < s_low_water_frames) {
            s_low_water_frames = left;
        }
        ++s_wakes;
#endif
        for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
            if (s_wave[i].status == NDSP_WBUF_DONE || s_wave[i].status == NDSP_WBUF_FREE) {
                fill(&s_wave[i]);
            } else {
                ++queued;
            }
        }
        if (queued == 0 && s_primed) {
            ++s_underruns;
        }
        s_primed = true;
#ifndef HGSS_PROFILE_OFF
        {
            const u64 ticks = svcGetSystemTick() - wake;

            s_refill_ticks += ticks;
            if (ticks > s_refill_max) {
                s_refill_max = ticks;
            }
            if (s_fills >= REPORT_FILLS) {
                report();
                s_fills = 0;
                s_wakes = 0;
                s_refill_ticks = 0;
                s_render_ticks = 0;
            }
        }
#endif
    }
}

bool kj_audio_init(void)
{
    NitroPort_SoundSetInterpolation(SOUND_INTERPOLATE);
    if (R_FAILED(ndspInit())) {
        return false;
    }
    s_data = linearAlloc(BUFFER_BYTES * BUFFER_COUNT);
    if (s_data == NULL) {
        ndspExit();
        return false;
    }
    memset(s_data, 0, BUFFER_BYTES * BUFFER_COUNT);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(0);
    ndspChnSetInterp(0, NDSP_INTERP_NONE);
    ndspChnSetRate(0, SAMPLE_RATE);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    {
        float mix[12] = { 1.0f, 1.0f };
        ndspChnSetMix(0, mix);
    }
    for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
        memset(&s_wave[i], 0, sizeof(s_wave[i]));
        s_wave[i].data_pcm16 = s_data + i * BUFFER_FRAMES * 2u;
        s_wave[i].nsamples = BUFFER_FRAMES;
        s_wave[i].status = NDSP_WBUF_FREE;
    }
#ifndef HGSS_PROFILE_OFF
    s_low_water_frames = UINT32_MAX;
    s_dropped_base = ndspGetDroppedFrames();
#endif
    LightEvent_Init(&s_event, RESET_ONESHOT);
    s_running = true;
    s_thread = threadCreate(audio_thread, NULL, 32 * 1024, 0x18, 0, false);
    if (s_thread == NULL) {
        kj_log("[audio] cannot start the audio thread");
        s_running = false;
        ndspExit();
        return false;
    }
#ifndef HGSS_PROFILE_OFF
    pmu_begin();
#endif
    ndspSetCallback(ndsp_frame_callback, NULL);
    LightEvent_Signal(&s_event);
    s_ready = true;
    kj_log("[audio] NDSP 32768 Hz stereo, %u x %u frames, %s", BUFFER_COUNT, BUFFER_FRAMES,
           SOUND_INTERPOLATE ? "interpolated samples" : "held samples (DS)");
    return true;
}

void kj_audio_service(void)
{
}

void kj_audio_exit(void)
{
    if (!s_ready) {
        return;
    }
    s_running = false;
    LightEvent_Signal(&s_event);
    threadJoin(s_thread, U64_MAX);
    threadFree(s_thread);
#ifndef HGSS_PROFILE_OFF
    pmu_end();
#endif
    ndspSetCallback(NULL, NULL);
    ndspChnReset(0);
    ndspExit();
    linearFree(s_data);
    kj_log("[audio] stopped, %lu underruns", (unsigned long)s_underruns);
    s_ready = false;
}
