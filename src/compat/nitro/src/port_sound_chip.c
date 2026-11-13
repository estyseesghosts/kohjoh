/*
 * Software model of the DS sound hardware.
 * Only this file owns the playback cursor; the original ARM7 driver writes
 * its control/timer/source registers. All public operations are plain C99.
 *
 * The ARM7 audio service serializes register access with mixing. Registers
 * retain the DS layout so SDK ARM7 snd_channel.c/snd_capture.c can run
 * without modification to their command logic.
 */
#include <nitro_port_sound_chip.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#define SOUND_CLOCK 16756991ull
#define PHASE_ONE (1ull << 32)
#define CHANNEL_BASE(ch) (0x400u + (unsigned)(ch) * 16u)
#define REG_CNT     0u
#define REG_SAD     4u
#define REG_TIMER   8u
#define REG_LOOP    10u
#define REG_LENGTH  12u

#ifdef NITRO_SOUND_STATS
/* Test-only frame hook (sdat_render): the mixer runs one frame per block, so
 * each event lands on its frame. Blocks give what frame-by-frame mixing gives
 * (see mix_block); sdat_render's release build checks that on real output. */
#define MIX_BLOCK 1
static NitroSoundFrameFn sFrameFn;
static void *sFrameArg;
static NitroSoundFrame sFrame;
static uint32_t sFrameIndex;
#define STATS_EVENT(channel, bit) do { if (sFrameFn) sFrame.event[(channel)] |= (uint8_t)(bit); } while (0)
#define STATS_CHANNEL_OF(base) (((base) - 0x400u) / 16u)
#define STATS_TAP(c, i, v) do { if ((c)->tap) (c)->tap[(i)] = (v); } while (0)
#else
#define MIX_BLOCK 128
#define STATS_EVENT(channel, bit) ((void)0)
#define STATS_CHANNEL_OF(base) 0u
#define STATS_TAP(c, i, v) ((void)0)
#endif

volatile uint8_t NitroPort_SoundRegisters[NITRO_SOUND_REG_SIZE];

static unsigned char reg8(unsigned offset)
{
    return NitroPort_SoundRegisters[offset];
}

static uint16_t reg16(unsigned offset)
{
    return (uint16_t)(reg8(offset) | ((uint16_t)reg8(offset + 1) << 8));
}

static uint32_t reg32(unsigned offset)
{
    return (uint32_t)reg16(offset) | ((uint32_t)reg16(offset + 2) << 16);
}

static void put8(unsigned offset, uint8_t value)
{
    NitroPort_SoundRegisters[offset] = value;
}

static void put32(unsigned offset, uint32_t value)
{
    put8(offset, (uint8_t)value);
    put8(offset + 1, (uint8_t)(value >> 8));
    put8(offset + 2, (uint8_t)(value >> 16));
    put8(offset + 3, (uint8_t)(value >> 24));
}

static int16_t saturate(int32_t value)
{
#ifdef HGSS_SND_ADPCM
    /* Ternary form so ARM builds emit predicated movs (verified: SSAT,
     * no branches, in the device object); bit-identical to the branched
     * form (host differential proof). Rides the ADPCM diet flag so the
     * hardware A/B measures one change. */
    int32_t v = value < INT16_MIN ? INT16_MIN : value;
    v = v > INT16_MAX ? INT16_MAX : v;
    return (int16_t)v;
#else
    if (value < INT16_MIN) return INT16_MIN;
    if (value > INT16_MAX) return INT16_MAX;
    return (int16_t)value;
#endif
}

static int8_t pcm8(const uint8_t *data, uint32_t offset)
{
    return (int8_t)data[offset];
}

static int16_t pcm16(const uint8_t *data, uint32_t offset)
{
    return (int16_t)((unsigned)data[offset] | ((unsigned)data[offset + 1] << 8));
}

static const uint16_t sStep[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371,
    408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166,
    1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845,
    8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
    18500, 20350, 22385, 24623, 27086, 29794, 32767
};
/* Exact sums of the separately truncated ADPCM shifts. 1,424 bytes
 * replaces three bit tests and additions per decoded sample. See Coffeepot
 * ctr/engineering/cpu-optimization.md (small tables and branch costs).
 * Do not replace with step*(2*code+1)/8: its rounding differs. */
static const uint16_t sAdpcmDelta[89][8] = {
    {0, 1, 3, 4, 7, 8, 10, 11},
    {1, 3, 5, 7, 9, 11, 13, 15},
    {1, 3, 5, 7, 10, 12, 14, 16},
    {1, 3, 6, 8, 11, 13, 16, 18},
    {1, 3, 6, 8, 12, 14, 17, 19},
    {1, 4, 7, 10, 13, 16, 19, 22},
    {1, 4, 7, 10, 14, 17, 20, 23},
    {1, 4, 8, 11, 15, 18, 22, 25},
    {2, 6, 10, 14, 18, 22, 26, 30},
    {2, 6, 10, 14, 19, 23, 27, 31},
    {2, 6, 11, 15, 21, 25, 30, 34},
    {2, 7, 12, 17, 23, 28, 33, 38},
    {2, 7, 13, 18, 25, 30, 36, 41},
    {3, 9, 15, 21, 28, 34, 40, 46},
    {3, 10, 17, 24, 31, 38, 45, 52},
    {3, 10, 18, 25, 34, 41, 49, 56},
    {4, 12, 21, 29, 38, 46, 55, 63},
    {4, 13, 22, 31, 41, 50, 59, 68},
    {5, 15, 25, 35, 46, 56, 66, 76},
    {5, 16, 27, 38, 50, 61, 72, 83},
    {6, 18, 31, 43, 56, 68, 81, 93},
    {6, 19, 33, 46, 61, 74, 88, 101},
    {7, 22, 37, 52, 67, 82, 97, 112},
    {8, 24, 41, 57, 74, 90, 107, 123},
    {9, 27, 45, 63, 82, 100, 118, 136},
    {10, 30, 50, 70, 90, 110, 130, 150},
    {11, 33, 55, 77, 99, 121, 143, 165},
    {12, 36, 60, 84, 109, 133, 157, 181},
    {13, 39, 66, 92, 120, 146, 173, 199},
    {14, 43, 73, 102, 132, 161, 191, 220},
    {16, 48, 81, 113, 146, 178, 211, 243},
    {17, 52, 88, 123, 160, 195, 231, 266},
    {19, 58, 97, 136, 176, 215, 254, 293},
    {21, 64, 107, 150, 194, 237, 280, 323},
    {23, 70, 118, 165, 213, 260, 308, 355},
    {26, 78, 130, 182, 235, 287, 339, 391},
    {28, 85, 143, 200, 258, 315, 373, 430},
    {31, 94, 157, 220, 284, 347, 410, 473},
    {34, 103, 173, 242, 313, 382, 452, 521},
    {38, 114, 191, 267, 345, 421, 498, 574},
    {42, 126, 210, 294, 379, 463, 547, 631},
    {46, 138, 231, 323, 417, 509, 602, 694},
    {51, 153, 255, 357, 459, 561, 663, 765},
    {56, 168, 280, 392, 505, 617, 729, 841},
    {61, 184, 308, 431, 555, 678, 802, 925},
    {68, 204, 340, 476, 612, 748, 884, 1020},
    {74, 223, 373, 522, 672, 821, 971, 1120},
    {82, 246, 411, 575, 740, 904, 1069, 1233},
    {90, 271, 452, 633, 814, 995, 1176, 1357},
    {99, 298, 497, 696, 895, 1094, 1293, 1492},
    {109, 328, 547, 766, 985, 1204, 1423, 1642},
    {120, 360, 601, 841, 1083, 1323, 1564, 1804},
    {132, 397, 662, 927, 1192, 1457, 1722, 1987},
    {145, 436, 728, 1019, 1311, 1602, 1894, 2185},
    {160, 480, 801, 1121, 1442, 1762, 2083, 2403},
    {176, 528, 881, 1233, 1587, 1939, 2292, 2644},
    {194, 582, 970, 1358, 1746, 2134, 2522, 2910},
    {213, 639, 1066, 1492, 1920, 2346, 2773, 3199},
    {234, 703, 1173, 1642, 2112, 2581, 3051, 3520},
    {258, 774, 1291, 1807, 2324, 2840, 3357, 3873},
    {284, 852, 1420, 1988, 2556, 3124, 3692, 4260},
    {312, 936, 1561, 2185, 2811, 3435, 4060, 4684},
    {343, 1030, 1717, 2404, 3092, 3779, 4466, 5153},
    {378, 1134, 1890, 2646, 3402, 4158, 4914, 5670},
    {415, 1246, 2078, 2909, 3742, 4573, 5405, 6236},
    {457, 1372, 2287, 3202, 4117, 5032, 5947, 6862},
    {503, 1509, 2516, 3522, 4529, 5535, 6542, 7548},
    {553, 1660, 2767, 3874, 4981, 6088, 7195, 8302},
    {608, 1825, 3043, 4260, 5479, 6696, 7914, 9131},
    {669, 2008, 3348, 4687, 6027, 7366, 8706, 10045},
    {736, 2209, 3683, 5156, 6630, 8103, 9577, 11050},
    {810, 2431, 4052, 5673, 7294, 8915, 10536, 12157},
    {891, 2674, 4457, 6240, 8023, 9806, 11589, 13372},
    {980, 2941, 4902, 6863, 8825, 10786, 12747, 14708},
    {1078, 3235, 5393, 7550, 9708, 11865, 14023, 16180},
    {1186, 3559, 5932, 8305, 10679, 13052, 15425, 17798},
    {1305, 3915, 6526, 9136, 11747, 14357, 16968, 19578},
    {1435, 4306, 7178, 10049, 12922, 15793, 18665, 21536},
    {1579, 4737, 7896, 11054, 14214, 17372, 20531, 23689},
    {1737, 5211, 8686, 12160, 15636, 19110, 22585, 26059},
    {1911, 5733, 9555, 13377, 17200, 21022, 24844, 28666},
    {2102, 6306, 10511, 14715, 18920, 23124, 27329, 31533},
    {2312, 6937, 11562, 16187, 20812, 25437, 30062, 34687},
    {2543, 7630, 12718, 17805, 22893, 27980, 33068, 38155},
    {2798, 8394, 13990, 19586, 25183, 30779, 36375, 41971},
    {3077, 9232, 15388, 21543, 27700, 33855, 40011, 46166},
    {3385, 10156, 16928, 23699, 30471, 37242, 44014, 50785},
    {3724, 11172, 18621, 26069, 33518, 40966, 48415, 55863},
    {4095, 12286, 20478, 28669, 36862, 45053, 53245, 61436},
};
static const int8_t sIndexDelta[8] = {-1,-1,-1,-1,2,4,6,8};

static void adpcm_init(NitroSoundVoice *v, const uint8_t *source)
{
    v->predictor = pcm16(source, 0);
    v->stepIndex = source[2] <= 88 ? source[2] : 88;
}

static void adpcm_step(NitroSoundVoice *v, uint8_t nibble)
{
    int32_t step = sStep[v->stepIndex];
#ifdef HGSS_SND_ADPCM
    /* Branchless nibble decode: exact (shifts exact, multiply by 0/1
     * exact, additions associative mod 2^32 under -fwrapv). Nibble bits
     * are music data (~50/50, unpredicted as branches). Bit-identical
     * to the branched form (host differential proof). */
    int32_t change = (step >> 3) + (step >> 2) * (int32_t)(nibble & 1u)
                   + (step >> 1) * (int32_t)((nibble >> 1) & 1u)
                   + step * (int32_t)((nibble >> 2) & 1u);
#else
    int32_t change = step >> 3;
    if (nibble & 1u) change += step >> 2;
    if (nibble & 2u) change += step >> 1;
    if (nibble & 4u) change += step;
#endif
    v->predictor += (nibble & 8u) ? -change : change;
    v->predictor = saturate(v->predictor);
    v->stepIndex += sIndexDelta[nibble & 7u];
    if (v->stepIndex < 0) v->stepIndex = 0;
    if (v->stepIndex > 88) v->stepIndex = 88;
}

static int16_t adpcm_at(NitroSoundVoice *v, const uint8_t *source,
                        uint32_t index)
{
    uint32_t nibble = index - 1;
    uint8_t byte = source[4u + nibble / 2u];
    adpcm_step(v, (nibble & 1u) ? byte >> 4 : byte & 15u);
    return (int16_t)v->predictor;
}

static uint32_t total_samples(unsigned format, uint32_t bytes)
{
    if (format == 0) return bytes;
    if (format == 1) return bytes / 2u;
    if (format == 2) return bytes >= 4u ? 1u + (bytes - 4u) * 2u : 0u;
    return 0;
}

static uint32_t loop_sample(unsigned format, uint32_t loopBytes)
{
    if (format == 0) return loopBytes;
    if (format == 1) return loopBytes / 2u;
    return loopBytes >= 4u ? 1u + (loopBytes - 4u) * 2u : 0u;
}

static void disable_channel(unsigned base, NitroSoundVoice *voice)
{
    STATS_EVENT(STATS_CHANNEL_OF(base), NITRO_SOUND_EV_END);
    put32(base + REG_CNT, reg32(base + REG_CNT) & 0x7fffffffu);
    voice->running = 0;
    voice->previousCtrl &= 0x7fffffffu;
}

static void restart_channel(NitroSoundVoice *voice, uint32_t ctrl,
                            unsigned base)
{
    const uint8_t *source = (const uint8_t *)(uintptr_t)reg32(base + REG_SAD);
    unsigned format = (ctrl >> 29) & 3u;
    STATS_EVENT(STATS_CHANNEL_OF(base), NITRO_SOUND_EV_START);
    voice->running = 1;
    voice->sampleIndex = 0;
    voice->phase = 0;
    voice->noise = 0x7fffu;
    voice->predictor = 0;
    voice->stepIndex = 0;
    voice->lastSample = 0;
    voice->loopSaved = 0;
    voice->adpcmClean = 0;
    if (format == 2u && source) {
        adpcm_init(voice, source);
        voice->adpcmClean = 1;
        voice->adpcmSource = (uint32_t)(uintptr_t)source;
    }
}

/*
 * One channel during one NitroSound_Mix call. Only the mixer itself changes
 * a channel's registers while it mixes (it switches a channel off at the end
 * of its sound), so they're read once per call, not once per sample, and the
 * sound's length, loop point and step are worked out once too. What comes
 * out is exactly what reading them every sample gave.
 */
typedef struct Channel {
    NitroSoundVoice *v;
    unsigned channel;
    unsigned base;
    unsigned format;
    unsigned repeat;
    uint32_t ctrl;
    uint32_t sourceAddr;
    const uint8_t *source;
    uint32_t nSamples;
    uint32_t loopAt;
    uint64_t step;
    int32_t volume;
    int32_t pan;
    unsigned shift;
    uint32_t endByte;      /* the channel reads [sourceAddr, sourceAddr + endByte) */
    int dependent;         /* reads memory a capture writes: mixed frame by frame */
    int interpolate;       /* NitroSoundChip.interpolate, for a sampled sound */
#ifdef NITRO_SOUND_STATS
    int32_t *tap;          /* the frame hook's samples for this channel, if set */
#endif
} Channel;

/* Between the sample before (a) and the sample now (b), as far along as
 * the playback cursor is (phase's fraction). */
static inline int32_t lerp(int32_t a, int32_t b, uint64_t phase)
{
    return a + (int32_t)(((int64_t)(b - a) * (int64_t)(uint32_t)phase) >> 32);
}

static int32_t wave_sample(const Channel *c)
{
    const NitroSoundVoice *v = c->v;

    switch (c->format) {
    case 0:
        return c->source ? (int32_t)pcm8(c->source, v->sampleIndex) * 256 : 0;
    case 1:
        return c->source ? pcm16(c->source, v->sampleIndex * 2u) : 0;
    case 2:
        return c->source ? v->predictor : 0;
    default:
        if (c->channel >= 14) return (v->noise & 1u) ? -32767 : 32767;
        if (c->channel < 8) return 0;
        /* Duty 7 is 0% high (GBATEK): silence, not a full-scale DC level. */
        if (((c->ctrl >> 24) & 7u) == 7u) return 0;
        return (v->sampleIndex & 7u) <= ((c->ctrl >> 24) & 7u) ? 32767 : -32767;
    }
}

/* The ADPCM decoder's state at the loop start, as it was when the sound
 * first got there; without one saved (or for other data), decoded again
 * from the start. */
static void adpcm_to_loop(NitroSoundVoice *v, const Channel *c)
{
    uint32_t i;

    if (c->loopAt != 0 && v->loopSaved && v->loopSource == c->sourceAddr &&
        v->loopAt == c->loopAt) {
        v->predictor = v->loopPredictor;
        v->stepIndex = v->loopStepIndex;
        return;
    }
    adpcm_init(v, c->source);
    for (i = 1u; i <= c->loopAt; ++i) (void)adpcm_at(v, c->source, i);
    v->adpcmClean = 1;
    v->adpcmSource = c->sourceAddr;
    if (c->loopAt != 0) {
        v->loopSaved = 1;
        v->loopSource = c->sourceAddr;
        v->loopAt = c->loopAt;
        v->loopPredictor = v->predictor;
        v->loopStepIndex = v->stepIndex;
    }
}

static void advance_voice(const Channel *c)
{
    NitroSoundVoice *v = c->v;

    if (c->format == 3u) {
        v->sampleIndex = (v->sampleIndex + 1u) & 7u;
        if (c->channel >= 14u) {
            unsigned carry = (v->noise ^ (v->noise >> 1u)) & 1u;
            v->noise = (uint16_t)((v->noise >> 1u) | (carry << 14u));
        }
        return;
    }
    if (!c->source || !c->nSamples) {
        disable_channel(c->base, v);
        return;
    }

    if (c->interpolate) v->lastSample = wave_sample(c);
    v->sampleIndex++;
    if (v->sampleIndex >= c->nSamples) {
        if (c->repeat != 1u || c->loopAt >= c->nSamples) {
            disable_channel(c->base, v);
            return;
        }
        STATS_EVENT(c->channel, NITRO_SOUND_EV_LOOP);
        v->sampleIndex = c->loopAt;
        if (c->format == 2u) adpcm_to_loop(v, c);
    } else if (c->format == 2u) {
        (void)adpcm_at(v, c->source, v->sampleIndex);
        if (v->sampleIndex == c->loopAt && v->adpcmClean) {
            v->loopSaved = 1;
            v->loopSource = c->sourceAddr;
            v->loopAt = c->loopAt;
            v->loopPredictor = v->predictor;
            v->loopStepIndex = v->stepIndex;
        }
    }
}

/* Read a channel's registers at the start of a mix; 0 if it's off. */
static int start_channel(NitroSoundChip *chip, unsigned channel, Channel *c)
{
    NitroSoundVoice *v = &chip->voice[channel];
    unsigned base = CHANNEL_BASE(channel);
    uint32_t ctrl = reg32(base);
    uint32_t period, startByte, endByte, shift;

    if (!(ctrl & 0x80000000u)) {
        if (v->running) STATS_EVENT(channel, NITRO_SOUND_EV_KILL);
        v->running = 0;
        v->previousCtrl = ctrl;
        return 0;
    }
    if (!v->running || !(v->previousCtrl & 0x80000000u))
        restart_channel(v, ctrl, base);
    v->previousCtrl = ctrl;

    c->v = v;
    c->channel = channel;
    c->base = base;
    c->ctrl = ctrl;
    c->format = (ctrl >> 29) & 3u;
    c->repeat = (ctrl >> 27) & 3u;
    c->sourceAddr = reg32(base + REG_SAD);
    c->source = (const uint8_t *)(uintptr_t)c->sourceAddr;
    if (v->adpcmClean && v->adpcmSource != c->sourceAddr) {
        /* The data changed under a sound playing on: its decoder state no
         * longer comes from this data alone; don't keep it for the loop. */
        v->adpcmClean = 0;
    }
    startByte = (uint32_t)reg16(base + REG_LOOP) * 4u;
    endByte = startByte + (reg32(base + REG_LENGTH) & 0x3fffffu) * 4u;
    c->endByte = endByte;
    c->dependent = 0;
    c->nSamples = total_samples(c->format, endByte);
    c->loopAt = loop_sample(c->format, startByte);
    c->volume = (int32_t)(ctrl & 127u);
    c->pan = (int32_t)((ctrl >> 16) & 127u);
    shift = (ctrl >> 8) & 3u;
    c->shift = shift == 3u ? 4u : shift;
    c->interpolate = chip->interpolate && c->format != 3u;

    /* The step only changes with the timer: no 64-bit division (slow on
     * the 3DS's CPU, which has no divide instruction) for every sample. */
    period = 0x10000u - reg16(base + REG_TIMER);
    if (v->stepPeriod != period || v->stepRate != chip->outputRate) {
        v->stepPeriod = period;
        v->stepRate = chip->outputRate;
        v->step = (SOUND_CLOCK << 32) / ((uint64_t)period * chip->outputRate);
    }
    c->step = v->step;
    return 1;
}

/* One sample of a running channel: adds it to the mix, returns it. */
static int32_t voice_mix(Channel *c, int32_t *left, int32_t *right)
{
    NitroSoundVoice *v = c->v;
    int32_t sample = wave_sample(c);
    int32_t scaled;
    unsigned guard = 0;

    if (c->interpolate) sample = lerp(v->lastSample, sample, v->phase);
    scaled = (sample * c->volume) / 127;

    scaled >>= c->shift;
    *left += (scaled * (127 - c->pan)) / 127;
    *right += (scaled * c->pan) / 127;

    v->phase += c->step;
    while (v->phase >= PHASE_ONE && v->running && guard++ < 2048u) {
        v->phase -= PHASE_ONE;
        advance_voice(c);
    }
    return scaled;
}

/*
 * One output frame's advance for PCM8/PCM16: the source index moves on by
 * every whole step the phase crossed and, when interpolating, last becomes
 * the sample it left behind. Returns 0 if the sound ended (advance_voice
 * has written its state back already), 1 if the block goes on. This is the
 * phase loop the PCM branch used to hold inline, split out so the silent
 * and pan-specialised loops all move the cursor in exactly one way.
 */
static inline int pcm_advance(Channel *c, uint64_t *phase, uint32_t *index,
                              int32_t *last, int pcm16bit, int interpolate,
                              uint32_t nSamples)
{
    const uint8_t *source = c->source;
    uint32_t advances = (uint32_t)(*phase >> 32);

    /* As voice_mix's loop: one source sample per whole step, at most 2048
     * per frame. */
    if (advances > 2048u) advances = 2048u;
    while (advances--) {
        *phase -= PHASE_ONE;
        if (*index + 1u < nSamples) {
            if (interpolate)
                *last = pcm16bit ? pcm16(source, *index * 2u)
                                 : (int32_t)pcm8(source, *index) * 256;
            ++*index;
            continue;
        }
        /* The end: what advance_voice does, with the state written back
         * first. */
        c->v->sampleIndex = *index;
        c->v->phase = *phase;
        advance_voice(c);
        if (!c->v->running) return 0;
        *index = c->v->sampleIndex;
        *last = c->v->lastSample;
    }
    return 1;
}

/*
 * ADPCM is the mixer's hot path on the Old 3DS: in o3ds-log-1 the mixer
 * took 23-30% of the application core in every window, about 170 cycles
 * per channel and output frame, and 99% of the active channels were ADPCM.
 * The one-pass loop kept more values live than the ARM11 has registers and
 * spilled its output pointers and 64-bit phase to the stack every frame
 * (coffeepot cpu-optimization: look for sp-relative spills in hot loops).
 * It is split in two passes instead: adpcm_render decodes and resamples
 * into a local buffer, mix_scaled applies volume, shift and pan.
 *
 * The phase is kept as its 32-bit fraction. That is exact when the whole
 * part is 0 at the start of the block (each frame's whole steps are
 * consumed before the next frame) and a frame's step is under 2,048
 * samples (so the per-frame cap never applies; the largest step, timer
 * period 1 at 32,768 Hz, is 511). mix_block checks both and otherwise
 * mixes frame by frame. Output is bit-identical to the one-pass loop.
 */

/*
 * saturate() for the render loop. ARMv6 SSAT clamps in the pipeline's
 * saturate stage (coffeepot cpu-optimization: ARM11 pipeline, ARMv6
 * instructions); GCC's compare form needed both limits from the literal
 * pool on every sample. Same result: signed 16-bit saturation.
 */
static inline int32_t sat16(int32_t value)
{
#if defined(__arm__) && defined(__ARM_ARCH) && __ARM_ARCH >= 6
    int32_t result;

    __asm__("ssat %0, #16, %1" : "=r"(result) : "r"(value));
    return result;
#else
    return saturate(value);
#endif
}

/* Samples (before volume) for up to frames frames; returns how many frames
 * the channel played (fewer if its sound ended). subStep: the caller knows
 * the step is under one sample per frame (stepWhole 0), so a frame decodes
 * at most one sample; a constant there frees registers in the loop. */
static inline __attribute__((always_inline)) size_t adpcm_render_frames(
    Channel *c, int32_t *out, size_t frames, const int interpolate,
    const int subStep)
{
    NitroSoundVoice *v = c->v;
    const uint8_t *source = c->source;
    const uint32_t nSamples = c->nSamples;
    const uint32_t loopAt = c->loopAt;
    /* index is at least 1 where it is compared, so 0 never matches: the
     * clean-loop check in one compare. */
    uint32_t loopMark = v->adpcmClean ? loopAt : 0u;
    const uint32_t stepWhole = subStep ? 0u : (uint32_t)(c->step >> 32);
    const uint32_t stepFrac = (uint32_t)c->step;
    uint32_t frac = (uint32_t)v->phase;
    uint32_t index = v->sampleIndex;
    int32_t predictor = v->predictor;
    int32_t stepIndex = v->stepIndex;
    int32_t last = v->lastSample;
    size_t i;

    for (i = 0; i < frames; ++i) {
        const uint32_t next = frac + stepFrac;
        uint32_t advances = stepWhole + (next < frac ? 1u : 0u);

        out[i] = interpolate ? lerp(last, predictor, frac) : predictor;
        frac = next;
        while (advances != 0u) {
            --advances;
            index++;
            if (index >= nSamples) {
                /* The end: what advance_voice does, with the state written
                 * back first (the phase still holding the whole steps not
                 * yet taken, as the 64-bit loop left it). */
                v->sampleIndex = index - 1u;
                v->predictor = predictor;
                v->stepIndex = stepIndex;
                v->phase = ((uint64_t)advances << 32) | frac;
                v->lastSample = last;
                advance_voice(c);
                if (!v->running) {
                    return i + 1u;
                }
                index = v->sampleIndex;
                predictor = v->predictor;
                stepIndex = v->stepIndex;
                last = v->lastSample;
                loopMark = v->adpcmClean ? loopAt : 0u;
                continue;
            }
            {
                const uint32_t nibble = index - 1u;
                const uint8_t byte = source[4u + nibble / 2u];
                const unsigned code = (nibble & 1u) ? byte >> 4 : byte & 15u;
                const int32_t change = sAdpcmDelta[stepIndex][code & 7u];
                last = predictor;
                predictor += (code & 8u) ? -change : change;
                predictor = sat16(predictor);
                stepIndex += sIndexDelta[code & 7u];
                if (stepIndex < 0) {
                    stepIndex = 0;
                }
                if (stepIndex > 88) {
                    stepIndex = 88;
                }
                if (index == loopMark) {
                    v->loopSaved = 1;
                    v->loopSource = c->sourceAddr;
                    v->loopAt = loopAt;
                    v->loopPredictor = predictor;
                    v->loopStepIndex = stepIndex;
                }
            }
        }
    }
    v->sampleIndex = index;
    v->predictor = predictor;
    v->stepIndex = stepIndex;
    v->phase = frac;
    v->lastSample = last;
    return frames;
}

static size_t adpcm_render(Channel *c, int32_t *out, size_t frames)
{
    if (c->interpolate) {
        if ((c->step >> 32) == 0u) {
            return adpcm_render_frames(c, out, frames, 1, 1);
        }
        return adpcm_render_frames(c, out, frames, 1, 0);
    }
    return adpcm_render_frames(c, out, frames, 0, 0);
}

/*
 * Adds rendered samples to L and R with the channel's volume, shift and
 * pan; values (if not NULL) gets each frame's sample after volume and
 * shift. The same arithmetic as voice_mix, so the same result: a hard pan
 * adds (x * 127) / 127 = x to one side and (x * 0) / 127 = 0 to the other.
 */
static void mix_scaled(Channel *c, const int32_t *samples, int32_t *L, int32_t *R,
                       int32_t *values, size_t frames)
{
    const int32_t volume = c->volume;
    const int32_t panL = 127 - c->pan;
    const int32_t panR = c->pan;
    const unsigned shift = c->shift;
    size_t i;

    if (volume == 0) {
        /* Silent: (x * 0) / 127 >> shift is 0 on both sides. */
        for (i = 0; i < frames; ++i) {
            if (values) {
                values[i] = 0;
            }
            STATS_TAP(c, i, 0);
        }
        return;
    }
    if (panR == 0) {
        for (i = 0; i < frames; ++i) {
            const int32_t scaled = ((samples[i] * volume) / 127) >> shift;

            if (values) {
                values[i] = scaled;
            }
            STATS_TAP(c, i, scaled);
            L[i] += scaled;
        }
    } else if (panL == 0) {
        for (i = 0; i < frames; ++i) {
            const int32_t scaled = ((samples[i] * volume) / 127) >> shift;

            if (values) {
                values[i] = scaled;
            }
            STATS_TAP(c, i, scaled);
            R[i] += scaled;
        }
    } else {
        for (i = 0; i < frames; ++i) {
            const int32_t scaled = ((samples[i] * volume) / 127) >> shift;

            if (values) {
                values[i] = scaled;
            }
            STATS_TAP(c, i, scaled);
            L[i] += (scaled * panL) / 127;
            R[i] += (scaled * panR) / 127;
        }
    }
}

/*
 * A channel's samples for a run of frames, added to L and R: what
 * voice_mix gives frame by frame, with the channel's settings and its
 * decoder in local variables; values (if not NULL) gets each frame's
 * sample. For channels that don't read what a capture writes: they don't
 * depend on the other channels within a mix.
 */
static void mix_block(Channel *c, int32_t *L, int32_t *R, int32_t *values, size_t frames)
{
    NitroSoundVoice *v = c->v;
    const int32_t volume = c->volume, panL = 127 - c->pan, panR = c->pan;
    const unsigned shift = c->shift;
    const uint64_t step = c->step;
    /* step is a block constant. At the port's 32,768 Hz output rate no
     * timer value makes it exactly PHASE_ONE (SOUND_CLOCK is not a
     * multiple of 32,768), so the phase loop always has work to do; a
     * single-sample-per-frame fast path would never be entered. */
    uint64_t phase = v->phase;
    size_t i;

    if (c->format == 2u && c->source && c->nSamples && frames <= MIX_BLOCK &&
        (v->phase >> 32) == 0u && (step >> 32) < 2048u) {
        /* ADPCM: rendered, then mixed (see adpcm_render). */
        int32_t samples[MIX_BLOCK];
        const size_t played = adpcm_render(c, samples, frames);

        mix_scaled(c, samples, L, R, values, played);
        return;
    }
    if (c->format <= 1u && c->source && c->nSamples) {
        /* PCM8 / PCM16. One loop per pan case, hoisted out of the frame
         * loop; step and shift are already block constants. */
        const uint8_t *source = c->source;
        const uint32_t nSamples = c->nSamples;
        const int pcm16bit = c->format == 1u;
        const int interpolate = c->interpolate;
        const int fullVolume = volume == 127;
        uint32_t index = v->sampleIndex;
        int32_t last = v->lastSample;

        if (volume == 0) {
            /* Silent: it adds nothing to either side, but its cursor must
             * still move, or the sample it comes back on when the envelope
             * lifts would be the wrong one. Only an interpolating channel
             * reads the source here, to keep lastSample. */
            for (i = 0; i < frames; ++i) {
                if (values) values[i] = 0;
                STATS_TAP(c, i, 0);
                phase += step;
                if (!pcm_advance(c, &phase, &index, &last, pcm16bit,
                                 interpolate, nSamples))
                    return;
            }
        } else if (panL == 127) {
            /* Hard left: panR is 0, so the right side would add
             * ((x * 0) / 127) = 0. */
            for (i = 0; i < frames; ++i) {
                int32_t sample = pcm16bit ? pcm16(source, index * 2u)
                                          : (int32_t)pcm8(source, index) * 256;
                int32_t scaled;

                if (interpolate) sample = lerp(last, sample, phase);
                /* (sample * 127) / 127 is sample exactly, and a PCM sample
                 * times 127 cannot overflow int32. */
                if (fullVolume) {
                    scaled = sample >> shift;
                } else {
                    scaled = (sample * volume) / 127;
                    scaled >>= shift;
                }
                if (values) values[i] = scaled;
                STATS_TAP(c, i, scaled);
                L[i] += scaled;
                phase += step;
                if (!pcm_advance(c, &phase, &index, &last, pcm16bit,
                                 interpolate, nSamples))
                    return;
            }
        } else if (panL == 0) {
            /* Hard right: panL is 0, so the left side would add 0. */
            for (i = 0; i < frames; ++i) {
                int32_t sample = pcm16bit ? pcm16(source, index * 2u)
                                          : (int32_t)pcm8(source, index) * 256;
                int32_t scaled;

                if (interpolate) sample = lerp(last, sample, phase);
                if (fullVolume) {
                    scaled = sample >> shift;
                } else {
                    scaled = (sample * volume) / 127;
                    scaled >>= shift;
                }
                if (values) values[i] = scaled;
                STATS_TAP(c, i, scaled);
                R[i] += scaled;
                phase += step;
                if (!pcm_advance(c, &phase, &index, &last, pcm16bit,
                                 interpolate, nSamples))
                    return;
            }
        } else {
            for (i = 0; i < frames; ++i) {
                int32_t sample = pcm16bit ? pcm16(source, index * 2u)
                                          : (int32_t)pcm8(source, index) * 256;
                int32_t scaled;

                if (interpolate) sample = lerp(last, sample, phase);
                if (fullVolume) {
                    scaled = sample >> shift;
                } else {
                    scaled = (sample * volume) / 127;
                    scaled >>= shift;
                }
                if (values) values[i] = scaled;
                STATS_TAP(c, i, scaled);
                L[i] += (scaled * panL) / 127;
                R[i] += (scaled * panR) / 127;
                phase += step;
                if (!pcm_advance(c, &phase, &index, &last, pcm16bit,
                                 interpolate, nSamples))
                    return;
            }
        }
        v->sampleIndex = index;
        v->phase = phase;
        v->lastSample = last;
        return;
    }
    /* Anything else (PSG, noise, no data): frame by frame. */
    for (i = 0; i < frames && v->running; ++i) {
        int32_t scaled = voice_mix(c, &L[i], &R[i]);
        if (values) values[i] = scaled;
        STATS_TAP(c, i, scaled);
    }
}

/*
 * Sound capture (SNDCAP0CNT/SNDCAP1CNT, 0x508/0x509): bit 7 on, bit 3 PCM8
 * (else PCM16), bit 2 one-shot (else it loops over its buffer: the SDK's
 * SND_CAPTURE_REPEAT_YES is 0), bit 1 records channel 0/2 (else the left /
 * right mixer). It records at the rate of channel 1's / channel 3's timer,
 * the channel that plays its buffer back: NitroSystem's reverb records the
 * mix into a looping buffer and plays it back quietly on those channels.
 * (Bit 0, adding channel 1/3 into channel 0/2, isn't done: nothing in the
 * game uses it.)
 */
static void do_capture(NitroSoundChip *chip, unsigned slot, int32_t signal)
{
    NitroSoundCapture *cap = &chip->capture[slot];
    unsigned controlReg = 0x508u + slot;
    unsigned base = 0x510u + slot * 8u;
    uint8_t ctl = reg8(controlReg);
    unsigned bytesPerSample = (ctl & 8u) ? 1u : 2u;
    uint32_t bufferSize = (uint32_t)reg16(base + 4u) * 4u;
    uint8_t *dst = (uint8_t *)(uintptr_t)reg32(base);
    uint32_t period;

    if (!(ctl & 128u)) {
        cap->running = 0;
        return;
    }
    if (!cap->running) {
        cap->index = 0;
        cap->phase = 0;
        cap->running = 1;
    }
    if (!dst || bufferSize < bytesPerSample) return;

    period = 0x10000u - reg16(CHANNEL_BASE(slot ? 3u : 1u) + REG_TIMER);
    if (cap->stepPeriod != period || cap->stepRate != chip->outputRate) {
        cap->stepPeriod = period;
        cap->stepRate = chip->outputRate;
        cap->step = (SOUND_CLOCK << 32) / ((uint64_t)period * chip->outputRate);
    }
    cap->phase += cap->step;
    while (cap->phase >= PHASE_ONE) {
        cap->phase -= PHASE_ONE;
        if (cap->index + bytesPerSample > bufferSize) {
            if (ctl & 4u) {             /* one-shot: done */
                put8(controlReg, (uint8_t)(ctl & 127u));
                cap->running = 0;
                return;
            }
            cap->index = 0;
        }
        if (bytesPerSample == 1u) {
            dst[cap->index] = (uint8_t)((int8_t)(saturate(signal) >> 8));
        } else {
            int16_t value = saturate(signal);
            dst[cap->index] = (uint8_t)value;
            dst[cap->index + 1u] = (uint8_t)((uint16_t)value >> 8);
        }
        cap->index += bytesPerSample;
    }
}

void NitroSound_Reset(NitroSoundChip *chip, uint32_t outputRate)
{
    if (!chip) return;
    memset(chip, 0, sizeof(*chip));
    memset((void *)NitroPort_SoundRegisters, 0, sizeof(NitroPort_SoundRegisters));
    chip->outputRate = outputRate ? outputRate : NITRO_SOUND_RATE;
    put8(0x500u, 127u);
    put8(0x501u, 128u);
}

void NitroSound_KeyOn(NitroSoundChip *chip, unsigned channel)
{
    if (!chip || channel >= NITRO_SOUND_CHANNEL_COUNT) return;
    if (!(reg8(CHANNEL_BASE(channel) + 3u) & 0x80u)) {
        /* The next mix starts it again (start_channel). */
        chip->voice[channel].running = 0;
    }
}

/* COUNT-HEAVY profile: cumulative counters so one device run can name the
 * mixer's cost (per-sample format decode vs the capture/reverb path, with
 * the sequencer split off in port_snd.c). Counters only; no mixing math
 * touched. The mixer runs under the audio lock on one thread (tests are
 * single-threaded), so plain integers, no atomics. port_snd.c reads these
 * for its [sndprof] line via NitroSound_MixProfile below. */
static uint64_t sProfMixCalls;      /* NitroSound_Mix calls that mixed (past the guard) */
static uint64_t sProfMixFrames;     /* output frames, summed over those calls */
static uint64_t sProfActiveCh;      /* active channels, summed over calls (mean: / calls) */
static uint64_t sProfFormatCh[4];   /* active channels by sample format: 0 PCM8, 1 PCM16, 2 ADPCM, 3 PSG/noise */
static uint64_t sProfCapturing;     /* capture instances running, summed over calls (0-2 each) */
static uint64_t sProfDependentCh;   /* channels reading a capture buffer, summed over calls */

void NitroSound_Mix(NitroSoundChip *chip, int16_t *stereo, size_t frames)
{
    Channel channels[NITRO_SOUND_CHANNEL_COUNT];
    unsigned count = 0, channel, k;
    unsigned control;
    int32_t masterVolume;
    int capturing, bypass1, bypass3;
    unsigned leftFrom, rightFrom;
    size_t i;
#ifdef NITRO_SOUND_STATS
    int32_t tapBuf[NITRO_SOUND_CHANNEL_COUNT][MIX_BLOCK];
#endif

    if (!chip || !stereo || !chip->outputRate || frames == 0) return;
    for (channel = 0; channel < NITRO_SOUND_CHANNEL_COUNT; ++channel) {
        if (start_channel(chip, channel, &channels[count])) ++count;
    }
#ifndef HGSS_PROFILE_OFF
    sProfMixCalls += 1u;
    sProfMixFrames += (uint64_t)frames;
    sProfActiveCh += (uint64_t)count;
    for (k = 0; k < count; ++k) sProfFormatCh[channels[k].format] += 1u;
#endif
#ifdef NITRO_SOUND_STATS
    for (k = 0; k < count; ++k) channels[k].tap = tapBuf[channels[k].channel];
#endif
    /* SOUNDCNT (0x500): bits 0-6 master volume, 8-9 / 10-11 what the left /
     * right output plays (the mixer, channel 1, channel 3 or both), 12 / 13
     * channel 1 / 3 kept out of the mixer, 15 on. */
    control = reg8(0x501u);
    masterVolume = (int32_t)(reg8(0x500u) & 127u);
    leftFrom = control & 3u;
    rightFrom = (control >> 2) & 3u;
    bypass1 = (control & 0x10u) != 0;
    bypass3 = (control & 0x20u) != 0;
    capturing = ((reg8(0x508u) | reg8(0x509u)) & 128u) ||
                chip->capture[0].running || chip->capture[1].running;
#ifndef HGSS_PROFILE_OFF
    for (channel = 0; channel < 2u; ++channel) {
        if ((reg8(0x508u + channel) & 128u) || chip->capture[channel].running)
            sProfCapturing += 1u;
    }
#endif

    if (capturing) {
        /* A channel that reads a capture's buffer (reverb plays the
         * recorded mix back on channels 1 and 3) depends on what the
         * capture writes frame by frame: those are mixed frame by frame,
         * in their place before the capture. The others don't depend on
         * anything in the mix and go a block at a time like the rest. */
        unsigned slot;
        for (slot = 0; slot < 2; ++slot) {
            uint32_t dst = reg32(0x510u + slot * 8u);
            uint32_t size = (uint32_t)reg16(0x514u + slot * 8u) * 4u;
            if (!dst || !size) continue;
            for (k = 0; k < count; ++k) {
                Channel *c = &channels[k];
                if (c->format == 3u || !c->source) continue;
                if (c->sourceAddr < dst + size && dst < c->sourceAddr + c->endByte) {
                    c->dependent = 1;
#ifndef HGSS_PROFILE_OFF
                    sProfDependentCh += 1u;
#endif
                }
            }
        }
    }
    {
        /* Channel by channel, a block of frames at a time. */
        enum { BLOCK = MIX_BLOCK };
        size_t at;

        for (at = 0; at < frames; at += BLOCK) {
            int32_t mixL[BLOCK], mixR[BLOCK], ch1L[BLOCK], ch1R[BLOCK], ch3L[BLOCK], ch3R[BLOCK];
            int32_t value0[BLOCK], value2[BLOCK];   /* channels 0 and 2, for captures */
            size_t n = frames - at < BLOCK ? frames - at : BLOCK;

            memset(mixL, 0, n * sizeof(int32_t));
            memset(mixR, 0, n * sizeof(int32_t));
            memset(ch1L, 0, n * sizeof(int32_t));
            memset(ch1R, 0, n * sizeof(int32_t));
            memset(ch3L, 0, n * sizeof(int32_t));
            memset(ch3R, 0, n * sizeof(int32_t));
            if (capturing) {
                memset(value0, 0, n * sizeof(int32_t));
                memset(value2, 0, n * sizeof(int32_t));
            }
#ifdef NITRO_SOUND_STATS
            for (channel = 0; channel < NITRO_SOUND_CHANNEL_COUNT; ++channel)
                for (i = 0; i < MIX_BLOCK; ++i) tapBuf[channel][i] = NITRO_SOUND_TAP_OFF;
#endif
            for (k = 0; k < count; ++k) {
                Channel *c = &channels[k];

                if (!c->v->running || c->dependent) continue;
                if (c->channel == 1) mix_block(c, ch1L, ch1R, NULL, n);
                else if (c->channel == 3) mix_block(c, ch3L, ch3R, NULL, n);
                else mix_block(c, mixL, mixR,
                               !capturing ? NULL : c->channel == 0 ? value0
                               : c->channel == 2 ? value2 : NULL, n);
            }
            for (i = 0; i < n; ++i) {
                int32_t outputL = 0, outputR = 0;
#ifdef NITRO_SOUND_STATS
                int32_t preL = 0, preR = 0;
#endif

                if (capturing) {
                    for (k = 0; k < count; ++k) {
                        Channel *c = &channels[k];
                        int32_t value;

                        if (!c->dependent || !c->v->running) continue;
                        if (c->channel == 1) {
                            value = voice_mix(c, &ch1L[i], &ch1R[i]);
                            STATS_TAP(c, i, value);
                        } else if (c->channel == 3) {
                            value = voice_mix(c, &ch3L[i], &ch3R[i]);
                            STATS_TAP(c, i, value);
                        } else {
                            value = voice_mix(c, &mixL[i], &mixR[i]);
                            STATS_TAP(c, i, value);
                            if (c->channel == 0) value0[i] = value;
                            if (c->channel == 2) value2[i] = value;
                        }
                    }
                }
                if (!bypass1) {
                    mixL[i] += ch1L[i];
                    mixR[i] += ch1R[i];
                }
                if (!bypass3) {
                    mixL[i] += ch3L[i];
                    mixR[i] += ch3R[i];
                }
                if (capturing) {
                    do_capture(chip, 0, (reg8(0x508u) & 2u) ? value0[i] : mixL[i]);
                    do_capture(chip, 1, (reg8(0x509u) & 2u) ? value2[i] : mixR[i]);
                }
                if (control & 128u) {
                    int32_t left = leftFrom == 0 ? mixL[i] : leftFrom == 1 ? ch1L[i]
                                 : leftFrom == 2 ? ch3L[i] : ch1L[i] + ch3L[i];
                    int32_t right = rightFrom == 0 ? mixR[i] : rightFrom == 1 ? ch1R[i]
                                  : rightFrom == 2 ? ch3R[i] : ch1R[i] + ch3R[i];

                    outputL = (left * masterVolume) / 127;
                    outputR = (right * masterVolume) / 127;
#ifdef NITRO_SOUND_STATS
                    preL = outputL;
                    preR = outputR;
#endif
                    /* Headroom before the 16-bit saturation, not after it:
                     * attenuating a flat-topped sample just halves the flat
                     * top, leaving the clipping distortion in the waveform.
                     * See outputShift in nitro_port_sound_chip.h. */
                    if (chip->outputShift) {
                        unsigned shift = chip->outputShift > 4u ? 4u : chip->outputShift;
                        outputL >>= shift;
                        outputR >>= shift;
                    }
                }
                stereo[2u * (at + i)] = saturate(outputL);
                stereo[2u * (at + i) + 1u] = saturate(outputR);
#ifdef NITRO_SOUND_STATS
                if (sFrameFn) {
                    unsigned ch;
                    sFrame.index = sFrameIndex++;
                    sFrame.preL = preL;
                    sFrame.preR = preR;
                    sFrame.postL = outputL;
                    sFrame.postR = outputR;
                    for (ch = 0; ch < NITRO_SOUND_CHANNEL_COUNT; ++ch)
                        sFrame.tap[ch] = NITRO_SOUND_TAP_OFF;
                    for (k = 0; k < count; ++k) {
                        const Channel *c = &channels[k];
                        sFrame.tap[c->channel] = c->tap[i];
                        sFrame.format[c->channel] = (uint8_t)c->format;
                        sFrame.volume[c->channel] = (uint8_t)c->volume;
                        sFrame.shift[c->channel] = (uint8_t)c->shift;
                    }
                    sFrameFn(sFrameArg, &sFrame);
                    memset(sFrame.event, 0, sizeof(sFrame.event));
                }
#endif
            }
        }
    }
    /* Channels that ended: as reading their (now off) registers left them. */
    for (k = 0; k < count; ++k) {
        if (!channels[k].v->running) {
            channels[k].v->previousCtrl = reg32(channels[k].base);
        }
    }
}

/* Cumulative profile counters for port_snd.c's [sndprof] line (declared
 * there, not in the shared header, so no header changes). formatCh takes
 * 4 entries, one per sample format (see sProfFormatCh). */
void NitroSound_MixProfile(uint64_t *calls, uint64_t *frames,
                           uint64_t *activeCh, uint64_t formatCh[4],
                           uint64_t *capturing, uint64_t *dependentCh)
{
    unsigned i;
    if (calls) *calls = sProfMixCalls;
    if (frames) *frames = sProfMixFrames;
    if (activeCh) *activeCh = sProfActiveCh;
    if (formatCh) {
        for (i = 0; i < 4u; ++i) formatCh[i] = sProfFormatCh[i];
    }
    if (capturing) *capturing = sProfCapturing;
    if (dependentCh) *dependentCh = sProfDependentCh;
}

#ifdef NITRO_SOUND_STATS
void NitroSound_SetFrameHook(NitroSoundFrameFn fn, void *arg)
{
    sFrameFn = fn;
    sFrameArg = arg;
    sFrameIndex = 0;
    memset(&sFrame, 0, sizeof(sFrame));
}
#endif
