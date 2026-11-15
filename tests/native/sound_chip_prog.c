/*
 * Full-chip sound program for an emulated ARMv6K core (tests/native/sound_arm.py).
 *
 * It includes one revision of port_sound_chip.c and drives NitroSound_Mix the
 * way port_snd.c and the ARM7 driver do: register writes and key-ons between
 * mix calls, stretches of about 171 frames. Two workloads:
 *
 *   SOUND_PROGRAM_COVERAGE  random register programs over every format,
 *                           routing, bypass, capture and loop case, sample
 *                           memory rewritten under the channels now and then.
 *   SOUND_PROGRAM_BENCH     a fixed HGSS-like workload: about 11 looping
 *                           ADPCM voices with envelopes, pans and note starts,
 *                           now and then a PSG voice. Instruction counts.
 *
 * The digest covers what the rest of the program can observe: the PCM output,
 * the sound registers, sample memory (capture writes there) and the voice
 * state that decides future output. Mixer-private caches are not hashed, so a
 * revision may keep different bookkeeping as long as everything it produces is
 * identical. lastSample is hashed only while interpolating: nothing reads it
 * otherwise.
 *
 * No libc: the program runs bare on Unicorn (memset/memcpy are below).
 */
#include <stddef.h>
#include <stdint.h>

void *memset(void *dst, int value, size_t count);
void *memcpy(void *dst, const void *src, size_t count);

#include SOURCE

#define SAMPLE_MEMORY_SIZE 0x10000u
#ifndef SOUND_BENCH_INTERPOLATE
#define SOUND_BENCH_INTERPOLATE 0
#endif

volatile uint64_t result;
volatile uint32_t result_frames;
volatile uint32_t result_channel_frames;

static uint8_t sample_memory[SAMPLE_MEMORY_SIZE] __attribute__((aligned(32)));
static int16_t output[2u * 1024u] __attribute__((aligned(32)));
static NitroSoundChip chip;
static uint32_t rng_state = 42u;
static uint64_t digest_state = 1469598103934665603ull;

/* GCC would turn these loops into calls to themselves. */
#define HARNESS_NO_LIBCALL __attribute__((optimize("no-tree-loop-distribute-patterns")))

/* Word stores, as newlib's ARM memset does for the mixer's word-aligned
 * arrays (a byte loop would overstate the clears several times over). */
HARNESS_NO_LIBCALL void *memset(void *dst, int value, size_t count)
{
    uint8_t *bytes = dst;
    const uint32_t word = 0x01010101u * (uint8_t)value;

    while (count != 0u && ((uintptr_t)bytes & 3u) != 0u) {
        *bytes++ = (uint8_t)value;
        --count;
    }
    while (count >= 16u) {
        uint32_t *words = (uint32_t *)(void *)bytes;

        words[0] = word;
        words[1] = word;
        words[2] = word;
        words[3] = word;
        bytes += 16;
        count -= 16u;
    }
    while (count--) {
        *bytes++ = (uint8_t)value;
    }
    return dst;
}

HARNESS_NO_LIBCALL void *memcpy(void *dst, const void *src, size_t count)
{
    uint8_t *to = dst;
    const uint8_t *from = src;

    while (count--) {
        *to++ = *from++;
    }
    return dst;
}

static uint32_t harness_random(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static uint32_t harness_below(uint32_t limit)
{
    return limit ? harness_random() % limit : 0u;
}

static void harness_digest(const void *data, size_t count)
{
    const uint8_t *bytes = data;

    while (count--) {
        digest_state = (digest_state ^ *bytes++) * 1099511628211ull;
    }
}

static void harness_digest_word(uint64_t value)
{
    harness_digest(&value, sizeof(value));
}

static void harness_put8(unsigned offset, uint8_t value)
{
    NitroPort_SoundRegisters[offset] = value;
}

static void harness_put16(unsigned offset, uint16_t value)
{
    harness_put8(offset, (uint8_t)value);
    harness_put8(offset + 1u, (uint8_t)(value >> 8));
}

static void harness_put32(unsigned offset, uint32_t value)
{
    harness_put16(offset, (uint16_t)value);
    harness_put16(offset + 2u, (uint16_t)(value >> 16));
}

static uint32_t harness_get32(unsigned offset)
{
    return (uint32_t)NitroPort_SoundRegisters[offset] |
           ((uint32_t)NitroPort_SoundRegisters[offset + 1u] << 8) |
           ((uint32_t)NitroPort_SoundRegisters[offset + 2u] << 16) |
           ((uint32_t)NitroPort_SoundRegisters[offset + 3u] << 24);
}

static void harness_digest_state(const int16_t *stereo, size_t frames)
{
    unsigned channel;

    harness_digest(stereo, frames * 4u);
    harness_digest((const void *)&NitroPort_SoundRegisters[0x400], 0x120u);
    for (channel = 0; channel < NITRO_SOUND_CHANNEL_COUNT; ++channel) {
        const NitroSoundVoice *v = &chip.voice[channel];

        harness_digest_word(v->running);
        if (!v->running) {
            continue;
        }
        harness_digest_word(v->sampleIndex);
        harness_digest_word(v->phase);
        harness_digest_word((uint32_t)v->predictor);
        harness_digest_word((uint32_t)v->stepIndex);
        harness_digest_word(v->noise);
        harness_digest_word(v->previousCtrl);
        if (chip.interpolate) {
            harness_digest_word((uint32_t)v->lastSample);
        }
    }
    for (channel = 0; channel < 2u; ++channel) {
        const NitroSoundCapture *cap = &chip.capture[channel];

        harness_digest_word(cap->running);
        harness_digest_word(cap->index);
        harness_digest_word(cap->phase);
    }
}

/* Mix a stretch and count it. */
static void harness_mix(size_t frames)
{
    unsigned channel;

    for (channel = 0; channel < NITRO_SOUND_CHANNEL_COUNT; ++channel) {
        if (NitroPort_SoundRegisters[CHANNEL_BASE(channel) + 3u] & 0x80u) {
            result_channel_frames += (uint32_t)frames;
        }
    }
    NitroSound_Mix(&chip, output, frames);
    result_frames += (uint32_t)frames;
}

/* An ADPCM sample: a 4-byte header (predictor, step index), then nibbles. */
static void harness_write_adpcm(uint32_t offset, uint32_t bytes)
{
    uint32_t i;

    for (i = 0; i < bytes; ++i) {
        sample_memory[offset + i] = (uint8_t)harness_random();
    }
    sample_memory[offset + 2u] = (uint8_t)harness_below(89u);
    sample_memory[offset + 3u] = 0u;
}

static void harness_fill_memory(void)
{
    uint32_t i;

    for (i = 0; i < SAMPLE_MEMORY_SIZE; ++i) {
        sample_memory[i] = (uint8_t)harness_random();
    }
}

/*
 * Start a note on a channel the way SND_StartChannel does: channel off,
 * KeyOn, registers, enable bit last.
 */
static void harness_start(unsigned channel, uint32_t ctrl, uint32_t source,
                          uint16_t timer, uint16_t loop_words, uint32_t length_words)
{
    const unsigned base = CHANNEL_BASE(channel);

    harness_put32(base + REG_CNT, ctrl & 0x7fffffffu);
    NitroSound_KeyOn(&chip, channel);
    harness_put32(base + REG_SAD, source);
    harness_put16(base + REG_TIMER, timer);
    harness_put16(base + REG_LOOP, loop_words);
    harness_put32(base + REG_LENGTH, length_words);
    harness_put32(base + REG_CNT, ctrl | 0x80000000u);
}

static uint32_t harness_random_ctrl(unsigned format)
{
    uint32_t ctrl = harness_below(128u);                  /* volume */
    uint32_t pan;

    switch (harness_below(4u)) {
    case 0:
        pan = 0u;
        break;
    case 1:
        pan = 127u;
        break;
    case 2:
        pan = 64u;
        break;
    default:
        pan = harness_below(128u);
        break;
    }
    ctrl |= harness_below(4u) << 8;                       /* shift */
    ctrl |= pan << 16;
    ctrl |= harness_below(8u) << 24;                      /* PSG duty */
    ctrl |= harness_below(4u) << 27;                      /* repeat */
    ctrl |= (uint32_t)format << 29;
    if (harness_below(5u) == 0u) {
        ctrl |= 127u;                                     /* full volume */
    }
    return ctrl;
}

static uint16_t harness_random_timer(void)
{
    switch (harness_below(6u)) {
    case 0:
        return (uint16_t)(0x10000u - 1u - harness_below(64u));    /* very high pitch */
    case 1:
        return (uint16_t)(0x10000u - 300u - harness_below(800u)); /* step around 1 */
    case 2:
        return (uint16_t)harness_below(0x10000u);
    default:
        return (uint16_t)(0x10000u - 400u - harness_below(1600u)); /* typical notes */
    }
}

static void harness_coverage_note(unsigned channel)
{
    unsigned format = harness_below(10u) < 6u ? 2u : harness_below(4u);
    uint32_t length_words = 1u + harness_below(600u);
    uint32_t loop_words = harness_below(length_words + 4u);
    uint32_t offset = harness_below(SAMPLE_MEMORY_SIZE / 4u - 1300u) * 4u;
    uint32_t ctrl = harness_random_ctrl(format);
    uint32_t source = (uint32_t)(uintptr_t)&sample_memory[offset];

    if (harness_below(40u) == 0u) {
        length_words = harness_below(3u);                 /* empty and tiny sounds */
    }
    if (harness_below(60u) == 0u) {
        source = 0u;                                      /* no data */
    }
    if (format == 2u && source != 0u) {
        sample_memory[offset + 2u] = (uint8_t)harness_below(100u);
    }
    harness_start(channel, ctrl, source, harness_random_timer(), (uint16_t)loop_words,
                  length_words);
}

static void harness_coverage_capture(void)
{
    unsigned slot = harness_below(2u);
    unsigned control = 0x80u;
    uint32_t offset = harness_below(SAMPLE_MEMORY_SIZE / 4u - 1300u) * 4u;
    uint32_t words = 1u + harness_below(400u);
    unsigned play_channel = slot ? 3u : 1u;

    control |= harness_below(2u) << 3;                    /* PCM8 */
    control |= harness_below(3u) == 0u ? 4u : 0u;         /* one-shot */
    control |= harness_below(2u) << 1;                    /* channel 0/2 source */
    harness_put8(0x508u + slot, 0u);
    harness_put32(0x510u + slot * 8u, (uint32_t)(uintptr_t)&sample_memory[offset]);
    harness_put16(0x514u + slot * 8u, (uint16_t)words);
    if (harness_below(3u) != 0u) {
        /* Reverb: the buffer played back on channel 1/3. */
        uint32_t ctrl = harness_random_ctrl(harness_below(2u));

        harness_start(play_channel, ctrl, (uint32_t)(uintptr_t)&sample_memory[offset],
                      harness_random_timer(), 0u, words);
    }
    if (harness_below(8u) != 0u) {
        harness_put8(0x508u + slot, (uint8_t)control);
    }
}

static void harness_coverage_step(void)
{
    unsigned channel = harness_below(NITRO_SOUND_CHANNEL_COUNT);
    unsigned base = CHANNEL_BASE(channel);
    uint32_t ctrl = harness_get32(base + REG_CNT);

    switch (harness_below(12u)) {
    case 0:
    case 1:
    case 2:
        harness_coverage_note(channel);
        break;
    case 3:
    case 4:
        /* Envelope: volume, shift, pan; the enable bit as it is. */
        ctrl = (ctrl & ~0x007f037fu) | harness_below(128u) | (harness_below(4u) << 8) |
               (harness_below(128u) << 16);
        harness_put32(base + REG_CNT, ctrl);
        break;
    case 5:
        harness_put16(base + REG_TIMER, harness_random_timer());
        break;
    case 6:
        harness_put32(base + REG_CNT, ctrl & 0x7fffffffu);
        break;
    case 7:
        harness_put8(0x500u, (uint8_t)harness_below(128u));
        harness_put8(0x501u, (uint8_t)(harness_below(4u) == 0u ? harness_random() : 0x80u));
        break;
    case 8:
        harness_coverage_capture();
        break;
    case 9:
        harness_put8(0x508u + harness_below(2u), 0u);
        break;
    case 10:
        if (harness_below(4u) == 0u) {
            /* Data rewritten under whatever plays it. */
            uint32_t at = harness_below(SAMPLE_MEMORY_SIZE - 64u);
            unsigned i;

            for (i = 0; i < 64u; ++i) {
                sample_memory[at + i] = (uint8_t)harness_random();
            }
        }
        break;
    default:
        /* Same note again (KeyOn of a playing channel restarts it). */
        if (ctrl & 0x80000000u) {
            harness_put32(base + REG_CNT, ctrl & 0x7fffffffu);
            NitroSound_KeyOn(&chip, channel);
            harness_put32(base + REG_CNT, ctrl);
        }
        break;
    }
}

static size_t harness_coverage_frames(void)
{
    switch (harness_below(8u)) {
    case 0:
        return 1u + harness_below(8u);
    case 1:
        return 1u + harness_below(1024u);
    case 2:
        return 512u;
    case 3:
        return 128u + harness_below(3u) - 1u;
    case 4:
        return 192u + harness_below(3u) - 1u;
    default:
        return 170u + harness_below(2u);
    }
}

static void harness_coverage(unsigned programs)
{
    unsigned program;

    for (program = 0; program < programs; ++program) {
        unsigned call;
        unsigned calls = 4u + harness_below(40u);

        NitroSound_Reset(&chip, NITRO_SOUND_RATE);
        chip.interpolate = (uint8_t)(program & 1u);
        chip.outputShift = (uint8_t)harness_below(3u);
        harness_put8(0x501u, 0x80u);
        if ((program & 7u) == 0u) {
            harness_fill_memory();
        }
        for (call = 0; call < calls; ++call) {
            unsigned steps = harness_below(6u);
            size_t frames = harness_coverage_frames();

            while (steps--) {
                harness_coverage_step();
            }
            harness_mix(frames);
            harness_digest_state(output, frames);
        }
        harness_digest(sample_memory, SAMPLE_MEMORY_SIZE);
    }
}

/*
 * HGSS-like: o3ds logs show about 11 active channels, ~99% ADPCM, no
 * capture, sometimes a PSG voice. Instruments loop; a sequencer tick
 * (every 170-171 frames) moves envelopes and starts notes.
 */
#define BENCH_INSTRUMENTS 12u

static uint32_t bench_offset[BENCH_INSTRUMENTS];
static uint32_t bench_words[BENCH_INSTRUMENTS];
static uint32_t bench_loop[BENCH_INSTRUMENTS];

static void harness_bench_note(unsigned channel)
{
    unsigned instrument = harness_below(BENCH_INSTRUMENTS);
    uint32_t ctrl = 40u + harness_below(88u);
    uint32_t pan = channel & 1u ? 64u : 20u + harness_below(88u);
    /* Timers for 0.3-1.0 source samples per output frame. */
    uint16_t timer = (uint16_t)(0x10000u - 512u - harness_below(1200u));

    ctrl |= pan << 16;
    ctrl |= 1u << 27;                                     /* loop */
    ctrl |= 2u << 29;                                     /* ADPCM */
    harness_start(channel, ctrl, (uint32_t)(uintptr_t)&sample_memory[bench_offset[instrument]],
                  timer, (uint16_t)bench_loop[instrument], bench_words[instrument]);
}

static void harness_bench(unsigned ticks)
{
    unsigned i;
    unsigned tick;
    uint32_t offset = 0;
    uint64_t phase = 0;

    NitroSound_Reset(&chip, NITRO_SOUND_RATE);
    chip.interpolate = SOUND_BENCH_INTERPOLATE;
    chip.outputShift = 1u;
    harness_put8(0x500u, 127u);
    harness_put8(0x501u, 0x80u);
    for (i = 0; i < BENCH_INSTRUMENTS; ++i) {
        uint32_t words = 250u + harness_below(1500u);

        bench_offset[i] = offset;
        bench_words[i] = words;
        bench_loop[i] = words / 4u + harness_below(words / 2u);
        harness_write_adpcm(offset, words * 4u);
        offset += words * 4u;
    }
    for (i = 0; i < 11u; ++i) {
        harness_bench_note(i);
    }
    for (tick = 0; tick < ticks; ++tick) {
        /* 2728 * 64 * 32768 / 33513982 frames per tick, as port_snd.c. */
        const uint64_t tick_phase = (uint64_t)2728u * 64u * 32768u;
        size_t frames = (size_t)((tick_phase - phase + 33513982u - 1u) / 33513982u);
        unsigned channel;

        phase = phase + (uint64_t)frames * 33513982u - tick_phase;
        harness_mix(frames);
        harness_digest(output, frames * 4u);
        for (channel = 0; channel < 11u; ++channel) {
            unsigned base = CHANNEL_BASE(channel);
            uint32_t ctrl = harness_get32(base + REG_CNT);

            if (harness_below(48u) == 0u) {
                harness_bench_note(channel);
            } else if (ctrl & 0x80000000u) {
                /* Envelope step. */
                uint32_t volume = ctrl & 127u;

                volume = volume > 2u ? volume - harness_below(3u) : 90u;
                harness_put32(base + REG_CNT, (ctrl & ~127u) | volume);
            }
        }
        if (tick % 400u == 0u) {
            /* A short square-wave voice now and then. */
            harness_start(14u - 6u, (3u << 29) | (4u << 24) | (64u << 16) | 60u, 0u,
                          (uint16_t)(0x10000u - 800u), 0u, 0u);
        } else if (tick % 400u == 40u) {
            harness_put32(CHANNEL_BASE(8u) + REG_CNT, 0u);
        }
    }
}

int main(void)
{
#ifdef SOUND_PROGRAM_BENCH
    harness_bench(SOUND_PROGRAM_BENCH);
#else
    harness_coverage(SOUND_PROGRAM_COVERAGE);
#endif
    result = digest_state;
    return 0;
}
