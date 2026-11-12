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

#define SAMPLE_RATE 32768.0f
#define BUFFER_FRAMES 512u /* 15.6 ms per wave buffer */
#define BUFFER_COUNT 4u

static ndspWaveBuf s_wave[BUFFER_COUNT];
static int16_t *s_data;
static Thread s_thread;
static LightEvent s_event;
static volatile bool s_running;
static bool s_ready;
static uint32_t s_underruns;

static void ndsp_frame_callback(void *arg)
{
    (void)arg;
    LightEvent_Signal(&s_event);
}

static void fill(ndspWaveBuf *wb)
{
    NitroPort_SoundRender(wb->data_pcm16, BUFFER_FRAMES);
    DSP_FlushDataCache(wb->data_pcm16, BUFFER_FRAMES * 4u);
    ndspChnWaveBufAdd(0, wb);
}

static void audio_thread(void *arg)
{
    (void)arg;
    while (s_running) {
        LightEvent_Wait(&s_event);
        unsigned queued = 0;
        for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
            if (s_wave[i].status == NDSP_WBUF_DONE || s_wave[i].status == NDSP_WBUF_FREE) {
                fill(&s_wave[i]);
            } else {
                ++queued;
            }
        }
        if (queued == 0) {
            ++s_underruns;
        }
    }
}

bool kj_audio_init(void)
{
    if (R_FAILED(ndspInit())) {
        return false;
    }
    s_data = linearAlloc(BUFFER_FRAMES * 4u * BUFFER_COUNT);
    if (s_data == NULL) {
        ndspExit();
        return false;
    }
    memset(s_data, 0, BUFFER_FRAMES * 4u * BUFFER_COUNT);
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
    LightEvent_Init(&s_event, RESET_ONESHOT);
    s_running = true;
    s_thread = threadCreate(audio_thread, NULL, 32 * 1024, 0x18, 0, false);
    if (s_thread == NULL) {
        kj_log("[audio] cannot start the audio thread");
        s_running = false;
        ndspExit();
        return false;
    }
    ndspSetCallback(ndsp_frame_callback, NULL);
    LightEvent_Signal(&s_event);
    s_ready = true;
    kj_log("[audio] NDSP 32768 Hz stereo, %u x %u frames", BUFFER_COUNT, BUFFER_FRAMES);
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
    ndspSetCallback(NULL, NULL);
    ndspChnReset(0);
    ndspExit();
    linearFree(s_data);
    kj_log("[audio] stopped, %lu underruns", (unsigned long)s_underruns);
    s_ready = false;
}
