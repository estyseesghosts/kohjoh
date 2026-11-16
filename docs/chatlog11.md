## **Why Kohjoh's audio is so expensive**

The main problem is architectural: **Kohjoh emulates almost the entire Nintendo DS sound system in software, even though the 3DS has a dedicated audio DSP.**

The current implementation preserves the original HGSS ARM7 sound driver, including its sequencer, envelopes, channel allocation and sound commands. It then emulates the DS sound hardware on the ARM11.

The 3DS DSP receives only the finished stereo PCM stream. It isn't doing the expensive per-voice work.

Current audio pipeline

**Original HGSS sound commands**

Game thread — ARM11 core 0

**Original ARM7 sequencer and envelopes**

**16-channel DS sound chip emulation**

**ADPCM decoding, resampling, mixing and capture**

Audio thread — ARM11 core 0

**3DS DSP — final stereo playback**

Dedicated audio hardware

Kohjoh's earlier profiling documented audio mixing taking **23–30% of core 0**. On a 268 MHz Old 3DS, that corresponds to roughly 62–80 million CPU cycles per second.

That's an earlier measurement, not a fresh benchmark of the latest build.

The primary cost comes from four things:

- **ADPCM decoding:** Each active compressed voice requires predictor calculations and sample reconstruction.
- **Resampling:** Every voice advances its own playback cursor according to the original DS sound timer.
- **Mixing:** Per-channel volume, panning, accumulation, clipping and stereo output.
- **Capture and reverb:** The DS hardware can record its own mix back into buffers that other channels play. This creates dependencies that prevent simple batching.

The relevant source is [port_sound_chip.c](https://github.com/estyseesghosts/kohjoh/blob/main/src/compat/nitro/src/port_sound_chip.c?utm_source=chatgpt.com).

Importantly, Kohjoh has **already optimized the ADPCM path**: it uses lookup tables, branchless decoding and a two-pass mixer to reduce register spilling. We shouldn't redo that work without profiling evidence.

## **What the newer measurements tell us**

There is a useful correction to the older 23–30% figure.

Kohjoh's [November 13 engineering log](https://github.com/estyseesghosts/kohjoh/blob/main/docs/worklog/2026-11-13.md?utm_source=chatgpt.com) calculates approximately **18% of one 268 MHz core** spent inside the mixer over a later measured interval. The log also reports:

- Around 99% of active channels in the original sample were ADPCM.
- The two-pass ADPCM optimization reduced Azahar's instruction count from 125.1 to 102.1 instructions per channel-sample.
- A subsequent table-based optimization reduced synthetic audio instructions by another 3.88%.

The last improvement's real-hardware benefit remains unmeasured.

That suggests the software optimization work is already paying off, and means a further improvement is plausible without moving cores.

## **The best software-side opportunities**

Cross-referencing [Coffeepot's ARM11 optimization guide](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/cpu-optimization.md?utm_source=chatgpt.com) and the current code, I'd prioritize these:

| Change                                          | Potential                     | Main concern                 |
| ----------------------------------------------- | ----------------------------- | ---------------------------- |
| Optimize interpolation's 64-bit multiplication  | Medium–high                   | Keep sample values exact     |
| Cache decoded ADPCM samples                     | High when sounds repeat       | RAM usage and invalidation   |
| Streamline capture/reverb processing            | Medium when capture is active | Feedback ordering            |
| ARMv6 packed SIMD for stereo accumulation       | Medium                        | Exact rounding and code size |
| Reduce repeated channel setup and lock overhead | Low–medium                    | Register-change timing       |

Two particularly concrete targets stand out.

**First,&#x20;**`lerp()`**&#x20;currently multiplies a signed sample difference by a 32-bit fractional playback position using 64-bit arithmetic.** That runs for interpolated samples on every active channel. An ARM11-specific implementation using long-multiply instructions could retain exact output while reducing instruction overhead. Whether GCC already generates sufficiently good code needs checking in the compiled assembly.

**Second, decoding the same ADPCM waveform repeatedly is unnecessary when the underlying data is immutable.** A bounded cache of decoded PCM16 samples would let repeated notes skip predictor and step-index calculations. The decoder still matters for dynamic or capture-dependent sounds.

However, decoding a 4-bit ADPCM sample into PCM16 roughly quadruples its storage requirement. A small cache with verified source identity is preferable to expanding the whole sound bank.

Coffeepot's hardware benchmarks also demonstrate a major instruction-cache penalty above the Old 3DS's 16 KiB I-cache. Kohjoh's ADPCM optimization already increased `mix_block` from 3.2 to 4.4 KiB, so making the mixer larger through aggressive inlining or SIMD could reverse some gains.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://hypnotize.works\&sz=32)

hypnotize.works



## **One simple experiment I would run first: disable interpolation**

Kohjoh explicitly enables interpolation:

```
sChip.interpolate = 1;
```

That is an audio-quality enhancement, not a requirement of the original hardware.

The [melonDS developer's explanation](https://melonds.kuribo64.net/comments.php?id=147\&utm_source=chatgpt.com) confirms that the original DS sound mixer does **not** perform interpolation. melonDS implements it as an optional enhancement.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://melonds.kuribo64.net\&sz=32)

melonDS



This gives us a useful choice:

- **Original DS mode:** Nearest-neighbor sampling, considerably simpler.
- **Enhanced mode:** Keep Kohjoh's current linear interpolation.

Turning it off removes the interpolation multiplication from every sampled voice. Since Kohjoh's active channels are overwhelmingly ADPCM, it's worth measuring on real hardware.

It may sound rougher on some instruments, so I would preserve the enhanced mode as an option. But original-hardware behavior should be a legitimate performance baseline, not treated as broken audio.

## **The bigger opportunity: use CSND instead of software mixing**

This is potentially more important than any ARM11 micro-optimization.

Coffeepot describes two 3DS audio systems: NDSP and CSND. The latter is particularly interesting because it retains hardware features closely related to the original DS sound system.

According to [3dbrew's CSND documentation](https://www.3dbrew.org/wiki/CSND_Shared_Memory?utm_source=chatgpt.com) and [libctru's CSND API](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/services/csnd.h?utm_source=chatgpt.com), it supports:

| Feature                     | DS sound chip | 3DS CSND                        |
| --------------------------- | ------------- | ------------------------------- |
| PCM8 / PCM16                | Yes           | Yes                             |
| IMA-ADPCM                   | Yes           | Yes                             |
| Independent hardware voices | 16            | Up to 32, subject to allocation |
| PSG square waves / noise    | Yes           | Supported interfaces            |
| Per-channel volume / pan    | Yes           | Yes                             |
| Playback timers / looping   | Yes           | Yes                             |
| Sound capture               | 2 units       | 2 units                         |

[image](https://www.google.com/s2/favicons?domain=https://www.3dbrew.org\&sz=32)

3dbrew+2

Even better, libctru's `csndPlaySound()` initializes IMA-ADPCM playback using the predictor and index stored immediately before the compressed sample data. That's very close to the format Kohjoh already processes.

**We may be able to keep the original ARM7 sequencer but replace most software sound-chip emulation with actual 3DS sound hardware.**

There are substantial compatibility questions, however. CSND is not guaranteed to match the DS's exact mixing arithmetic, playback timing, loop transitions or capture feedback. It also requires suitable physical sample memory and correctly allocated channels. These need hardware tests rather than assumptions.

Another important consideration: `csndPlaySound()` submits commands synchronously. We should not call it separately for every channel update. A real backend would batch dirty voice-register changes through CSND's command interface.

This would be a larger engineering change, but unlike moving the audio thread to core 1, it could eliminate a considerable amount of audio CPU work altogether.

## **What other ports teach us**

The [G-Diffuser F-Zero X postmortem](https://hypnotize.works/gdx-3ds/postmortem/native.html?utm_source=chatgpt.com) reports a 2.2× improvement to its audio synthesis using ARMv6 packed SIMD instructions, alongside a major reduction in audio-lock waiting.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://hypnotize.works\&sz=32)

hypnotize.works



The techniques are relevant, but the speedup is not directly transferable. Kohjoh's ADPCM predictors are sequential, making SIMD less straightforward than for independent PCM samples. Stereo accumulation and volume/panning are better initial candidates.

## **The implementation order I'd choose**

1. **Establish a fresh hardware baseline.** Separate mixer, sequencer, interpolation, ADPCM decoding, capture, lock waiting and DSP submission costs. Extend `[sndprof]` without leaving expensive probes in release builds.
2. **Test original-DS interpolation mode.** Compare PCM output, audible quality, mixer cycles and game-frame p99 against enhanced interpolation.
3. **Optimize the existing mixer.** Inspect ARM assembly for the interpolation multiply, divisions and stack spills; optimize capture's repeated register handling; test SIMD selectively. Preserve the existing randomized differential tests.
4. **Prototype bounded ADPCM caching.** Use immutable samples only, test cache hits and invalidation, and reject designs that increase loading hitches or exceed memory budgets.
5. **Build an isolated CSND proof of concept.** Test one ADPCM voice, then multiple voices, pitch changes, loop state, pan and capture/reverb against the software reference. Verify the channel-allocation mask and both `.3dsx` and CIA service permissions.

For each change, I'd use Goldenrod, the Ho-Oh title screen, a normal battle and a sound-heavy transition as hardware benchmarks. Require no new audio gaps, and validate software changes by PCM/state comparisons. CSND would additionally need hardware recording and perceptual comparisons.

**My conclusion:** Keep audio on core 0 for now. First measure the cheaper, more faithful software mixer, then investigate CSND as a native backend. If CSND can reproduce HGSS's sound behavior closely enough, moving audio to core 1 may become unnecessary.

# **Kohjoh: Software Audio Optimization Plan**

Target: original 2011 Nintendo 3DS · ARM11 at 268 MHz · audio on core 0 · full-speed 30 FPS

## **1. The engineering goal**

I would keep Kohjoh's original NitroSDK ARM7 sound driver and its software implementation of the Nintendo DS sound hardware. The goal is to make that implementation substantially cheaper without sacrificing sound accuracy, timing or compatibility.

There are three priorities:

1. **Preserve DS behavior.** Sequencing, pitch, channel allocation, envelopes, looping, stereo mixing, capture/reverb and sound-command responses must remain correct.
2. **Reduce total CPU consumption.** Optimize the actual work performed for every audio sample, not merely shift it between threads.
3. **Reduce game-thread interference.** Audio should not cause substantial stalls in game logic, even when all sound channels are active.

I would explicitly exclude moving audio to core 1, replacing the mixer with CSND, dropping active voices, or reducing the 32,768 Hz simulation rate.

### **What improvement is realistic?**

Kohjoh's November 13 worklog provides two useful measurements:

| Measurement                             | Result                                    |
| --------------------------------------- | ----------------------------------------- |
| Initial Old 3DS audio cost              | 20–30% of core 0                          |
| Later measured audio mixing cost        | Approximately 18% of a core               |
| ADPCM optimization in Azahar            | 125.1 → 102.1 instructions/channel-sample |
| Subsequent synthetic ADPCM optimization | 3.88% fewer instructions                  |

The later 18% figure comes from a different measurement window and code revision. It is not proof that all improvements have translated into equivalent hardware gains.

Suppose the current mixer really occupies 18% of a core. Reducing that work by 25% would recover approximately **1.5 ms of CPU time per 30 FPS game frame**.

That's meaningful on the Old 3DS, especially in Goldenrod, where the game already competes with geometry processing and rendering preparation.

I would treat a 20–30% mixer reduction as an initial engineering objective, not a forecast. Larger savings might be possible through caching, but we need to establish how much work repeats.

## **2. What we should preserve**

The current audio architecture is fundamentally reasonable.

**Game thread — original HGSS ARM9 code**

Sound commands, status queries, game events

Command handoff + synchronization

**Audio worker — ARM11 core 0**

Original ARM7 sequencer

16-channel DS sound model

Software decode, resample, mix and capture

**NDSP → 3DS audio hardware**

Plays the finished PCM16 stream

The existing implementation already makes several good decisions:

- It keeps the original ARM7 sequencer rather than approximating Pokémon's music logic.
- Sound timing follows an independent clock instead of the game's frame rate.
- ADPCM processing uses cached playback steps, a precomputed magnitude table and a specialized decode loop.
- Audio samples are processed in blocks instead of dispatching an entire sound-chip interpreter for every output sample.
- Sound-alarm replies are delivered on the game thread, avoiding direct calls into the game's scheduler from the audio worker.

Those are foundations worth retaining.

**One accuracy distinction matters:** preserving the current mixer's output exactly is not always the same as matching original DS hardware. Some Kohjoh behavior, particularly linear interpolation, is deliberately enhanced. We should retain the choice between original-hardware accuracy and enhanced sound rather than mixing those goals together.

Sources: [Sound sequencer](https://github.com/estyseesghosts/kohjoh/blob/main/src/compat/nitro/src/port_snd.c?utm_source=chatgpt.com) · [Software mixer](https://github.com/estyseesghosts/kohjoh/blob/main/src/compat/nitro/src/port_sound_chip.c?utm_source=chatgpt.com) · [GBATEK DS sound hardware](https://mgba-emu.github.io/gbatek/?utm_source=chatgpt.com).

## **3. The first changes I would implement**

### **A. Match the mix block size to the sequencer interval**

Low-risk first experiment

This is a small but specific inefficiency in the current implementation.

In `port_snd.c`, the audio sequencer advances approximately every 5.21 ms, corresponding to about 171 output samples.

However, `port_sound_chip.c` uses:

```
#define MIX_BLOCK 128
```

That means a typical 171-sample stretch gets divided into two processing blocks: 128 samples, then approximately 43.

For each block, the mixer initializes scratch arrays and loops through the active channels.

I would test:

```
#define MIX_BLOCK 192
```

A 192-sample block can hold an entire normal sequencer interval.

This could eliminate redundant setup and channel-loop work without modifying the actual sound calculations.

The tradeoff is a larger working set: eight 32-bit scratch arrays grow from approximately 4 KiB to 6 KiB. That matters on the Old 3DS's 16 KiB data cache.

**Validation:** Compare 96, 128, 176, 192 and 256 samples using identical audio workloads. Check CPU time, data-cache misses, output hashes and stack usage. Keep 128 if the larger blocks hurt cache behavior.

### **B. Add a specialized ordinary-mixing path**

This is probably the most interesting straightforward CPU optimization.

Inside `NitroSound_Mix`, the current implementation maintains several intermediate arrays:

```
mixL[], mixR[]
ch1L[], ch1R[]
ch3L[], ch3R[]
value0[], value2[]
```

They exist because DS sound channels 1 and 3 have special routing and the capture hardware can feed recorded audio back into other voices.

But there are situations where none of that complexity is necessary.

When capture is disabled, channel 1/3 bypass is disabled, and both outputs select the normal mixer, we can accumulate all active voices directly into two stereo arrays.

**General path**

Up to 8 temporary arrays

Voice groups

Separate mix / capture buffers

Combine + output

**Ordinary fast path**

2 temporary arrays

All active voices

Stereo accumulation

Final output

This avoids unnecessary intermediate writes, reads, clearing and merging.

I would implement one narrow conditional fast path while retaining the complete existing mixer for all other routing configurations.

Before writing it, instrument how frequently HGSS actually qualifies. If capture/reverb is active nearly all the time, the potential gain is small.

The output must remain bit-identical to the existing implementation, including channel order, volume truncation and clipping.

### **C. Make original DS interpolation behavior the default**

The original DS uses nearest-neighbor playback, not linear interpolation. This is corroborated by [melonDS's interpolation research](https://melonds.kuribo64.net/comments.php?id=147\&utm_source=chatgpt.com) and the DS hardware references.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://melonds.kuribo64.net\&sz=32)

melonDS+1



Kohjoh currently initializes the software chip with:

```
sChip.interpolate = 1;
```

I would expose two modes:

| Mode     | Behavior                         | Purpose                       |
| -------- | -------------------------------- | ----------------------------- |
| Accurate | Nearest-neighbor sample playback | Default, original DS behavior |
| Enhanced | Current linear interpolation     | Optional smoother sound       |

This is a particularly worthwhile experiment because `lerp()` runs for every active interpolated voice and currently uses a 64-bit multiplication.

The accurate path avoids that interpolation calculation.

It must still advance every voice's playback cursor correctly, including silent voices. Disabling interpolation must never mean disabling their playback-state updates.

I would not change the global output sample rate, pitch calculations or master-volume calibration as part of this optimization.

## **4. Optimize the remaining ADPCM hot path**

The present ADPCM work is already reasonably sophisticated. It uses a 1,424-byte delta table and a dedicated fast path for cases where the playback cursor advances less than one source sample per output sample.

The next improvements should target instructions the ARM11 executes unnecessarily.

### **A. Inspect interpolation assembly**

The current interpolator is effectively:

```
return a + (int32_t)(
    ((int64_t)(b - a) * (uint32_t)phase) >> 32
);
```

That calculation requires the upper half of a signed 64-bit product.

ARM11 has `SMULL`, which produces both halves of a signed 32×32-bit multiplication. A carefully constructed implementation can perform this interpolation without general 64-bit arithmetic sequences, including a correction for the unsigned phase's sign bit.

However, GCC may already generate something close to the ideal sequence.

I would first inspect the disassembly. Only add ARM-specific assembly if it removes real instructions or stalls.

The test must include negative sample differences, phase values around `0x80000000`, and exact truncation behavior. A mathematically similar interpolation that differs by one PCM unit is not a bit-exact replacement.

### **B. Inspect division in volume and panning**

`mix_scaled()` repeatedly performs operations like:

```
scaled = (sample * volume) / 127;
left += (scaled * panL) / 127;
right += (scaled * panR) / 127;
```

The ARM11 lacks an integer divide instruction, but GCC can replace division by a compile-time constant with multiplication and shifts.

So we should **not assume&#x20;**`/ 127`**&#x20;calls a slow division routine**.

Instead, inspect the compiled inner loops for actual division helpers, unnecessary sign corrections, repeated multiplications and stack spills.

If the compiler's code is already compact, leave it alone.

One correctness rule is non-negotiable: don't combine the volume and pan divisions into one operation. The original expression truncates intermediate values; changing the order can change the output.

### **C. Try ARMv6 packed arithmetic selectively**

Coffeepot documents ARMv6 instructions that operate on multiple 8-bit or 16-bit values within ordinary 32-bit registers. ARM's MPCore manual confirms these facilities. This is **not NEON**.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://documentation-service.arm.com\&sz=32)

documentation-service.arm.com



I would investigate packed operations for:

- Applying volume to pairs of PCM16 samples.
- Efficient loading and sign-extension of sample data.
- Parts of stereo accumulation where numeric ranges are proven safe.
- Saturation and packing at the final PCM16 output.

I would not immediately attempt to vectorize the entire ADPCM decoder. Each decoded ADPCM sample updates the predictor and step index required for the next sample, so the decoder is fundamentally sequential within one voice.

Furthermore, the current mixer accumulates values in 32-bit integers. Packing the accumulators into 16-bit lanes would risk overflow before the final output clamp.

The useful comparison is therefore **scalar specialized loop versus compact ARMv6 implementation**, with identical output and state.

The [G-Diffuser engineering postmortem](https://hypnotize.works/gdx-3ds/postmortem/native.html?utm_source=chatgpt.com) reports a 2.2× audio synthesis improvement from packed arithmetic in its own engine. That supports investigating the technique, not expecting the same result in Kohjoh.

## **5. Optimize sound capture and reverb**

This is the next area I would inspect after the regular mixing path.

The DS has two sound-capture units. HeartGold/SoulSilver uses the original NitroSDK sound engine's capture facilities, including effects that can record a mix and play it back through other channels.

Kohjoh implements this in `do_capture()`.

Currently, each active capture unit repeatedly:

- Reads its emulated hardware registers.
- Determines the recording format and buffer address.
- Checks the capture timer and playback position.
- Advances the capture cursor.
- Writes output samples into emulated memory.
- Handles buffer wrapping and one-shot completion.

Some of this work occurs for every output sample, even when the register values haven't changed.

### **Proposed change: persistent capture state**

I would extend `NitroSoundCapture` with a prepared, compact representation of its current configuration.

For example:

```
typedef struct {
    uint8_t *dst;
    uint32_t buffer_size;
    uint32_t index;
    uint64_t phase;
    uint64_t step;
    uint32_t flags;
} PreparedCapture;
```

At the beginning of each audio stretch, prepare the configuration once. Then execute the capture loop against that local state, writing back the persistent cursor and status.

This should eliminate repeated register reads, address calculations and control decoding.

**The important restriction is preserving feedback order.**

A capture buffer may be the source of another playing voice. That voice must see the correct recorded samples at the correct time.

We cannot simply defer all recording until after an audio block finishes, because that would change reverb behavior.

Instead, preserve the existing sample-by-sample feedback ordering while making each iteration cheaper.

I would also specialize capture-disabled blocks so they never enter capture-processing code unnecessarily.

## **6. Investigate decoded ADPCM caching**

This has the greatest potential to eliminate work, but it is more complicated than the previous changes.

ADPCM is useful because it stores audio in approximately one-quarter the space of PCM16. The cost is that each sample depends on the preceding decoder state.

Currently, Kohjoh reconstructs samples whenever they are played.

But Pokémon sound banks reuse the same instruments and samples across many notes.

If a sample is immutable, we could decode it once and reuse the decoded waveform for subsequent playback.

### **Proposed approach**

Introduce a bounded software cache with three components:

| Component           | Purpose                                                     |
| ------------------- | ----------------------------------------------------------- |
| Sample identity     | Identify an immutable source waveform                       |
| Decoded PCM16       | Reuse reconstructed audio samples                           |
| Decoder checkpoints | Recover exact predictor and step-index state when necessary |

A decoded waveform cannot replace the whole DS sound-channel state. The playback phase, envelope, looping behavior, channel registers and decoder state must still behave correctly.

The cache therefore accelerates sample reconstruction; it does not replace the sequencer or the channel emulator.

### **Memory tradeoff**

Example: 64 KiB of ADPCM sample data

Compressed source

**64 KiB**

Decoded PCM16

**\~256 KiB**

Approximate storage only; decoder checkpoints and metadata add overhead.

On a 64 MB Old 3DS application, unrestricted sample caching would be a mistake.

I would begin with a 128–256 KiB decoded-data budget, restricted to small, frequently reused, known-immutable samples.

The cache must not rely on source pointers alone: memory can be reused, source contents may change, and capture buffers are inherently mutable.

A stable asset identity and generation, or equivalent verified content identity, is required. Capture-dependent audio should remain on the original uncached path.

**Before implementing this**, measure how often the same sample is replayed, how many decoder operations are avoided, and the memory required for a useful hit rate.

If reuse is poor, reject the feature rather than keep an expensive cache that seldom helps.

## **7. Address audio-thread interference**

This is important because reducing mixer instructions doesn't automatically eliminate every graphics hitch.

Kohjoh's audio thread runs at priority `0x18` on core 0. The game's thread has lower priority.

The audio worker can therefore interrupt the game whenever it needs to render another buffer.

### **The shared-lock problem**

In `port_snd.c`, `sound_lock()` protects the sequencer, sound registers, decoder state, and mixing operations.

The audio thread acquires that lock and runs `NitroSound_Mix()` while holding it.

The game thread also uses the lock when issuing sound commands or reading shared sound state.

There are two different costs here:

**CPU preemption:** Even without lock contention, audio consumes time that game logic could otherwise use.

**Synchronization stalls:** A game sound command can wait for an existing mix operation to finish, producing a visible spike in frame time.

The current code already releases the lock between approximately 5.2 ms stretches of audio, which is much better than holding it across an entire 512-sample refill.

But a single stretch can still be expensive.

### **What I would change**

First, measure how long the game thread actually waits for the sound lock, and the longest individual audio lock hold.

Then investigate a stricter ownership model where the audio worker owns live mixer state and the game submits commands through a bounded queue.

The game could continue to wait for acknowledgments where the original NitroSDK requires synchronous completion.

This could allow shorter locks around command transfer instead of holding them throughout audio generation.

However, it requires carefully handling source-data lifetimes, command ordering, sound status queries and capture feedback. Copying the channel registers and unlocking is **not** sufficient; the underlying sample memory may still change.

I would treat this as a second-stage architecture improvement, not a prerequisite for the first optimization pass.

Coffeepot's [threading guidance](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/threading.md?utm_source=chatgpt.com) documents a relevant failure in the G-Diffuser port: narrowing its audio lock reduced a reported 25.8 ms wait to 0.2 ms on Old 3DS hardware. That demonstrates why lock duration deserves its own measurement.

## **8. Reduce audio-output overhead**

Kohjoh currently submits four rotating buffers, each containing 512 stereo frames.

Each buffer is 2 KiB, and the CPU flushes it through `DSP_FlushDataCache()` before submission to NDSP.

Coffeepot's [cache and memory benchmarks](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/memory-and-cache.md?utm_source=chatgpt.com) show that kernel-managed cache operations can be materially cheaper than service-mediated flushes.

I would benchmark replacing the explicit DSP cache flush with `svcFlushProcessDataCache()` for the same linear-memory range.

This is a relatively isolated experiment. It must preserve correct DSP visibility and never reuse a buffer until playback has finished.

I would not expect this to recover a large fraction of the mixer's 18% CPU cost. It mainly reduces service overhead and possibly worst-case refill latency.

One more issue should be fixed during instrumentation: Kohjoh's existing `s_underruns` counter does not reliably identify actual playback starvation. Add DSP dropped-frame counters, queued-sample low-water marks and refill deadlines before making claims about underrun improvements.

## **9. Build a proper software-audio benchmark**

I'd extend Kohjoh's existing profiling rather than introduce another independent performance framework.

The current `[sndprof]` counters already distinguish mixer work from sequencer ticks and count active channels.

However, the implementation still needs a clearer breakdown of where the mixing time goes.

### **Required measurements**

| Metric                        | What it tells us                                       |
| ----------------------------- | ------------------------------------------------------ |
| ADPCM decode cycles           | Cost of predictor reconstruction                       |
| Resampling cycles             | Cost of fractional playback positions                  |
| Volume/pan cycles             | Cost of arithmetic and stereo accumulation             |
| Capture/reverb cycles         | Cost of feedback and recording                         |
| Sequencer cycles              | Cost of original ARM7 sound processing                 |
| Sound-command processing      | Work occurring synchronously on the game thread        |
| Audio lock wait, p99/max      | Whether synchronization causes game hitches            |
| Refill-to-submit time         | Whether audio generation meets DSP deadlines           |
| ADPCM cache hit rate          | Whether precaching is worthwhile                       |
| Instruction/data-cache misses | Whether larger optimized loops hurt hardware execution |

Use short, selectively sampled instrumentation rather than adding timer calls to every sample. Record counters in memory and emit aggregate results periodically.

Keep separate instrumented and non-instrumented builds. The latter is the actual frame-rate benchmark.

### **Correctness testing**

Kohjoh's `tests/native/sound_diff.c` already runs 100,000 randomized ADPCM programs and hashes output and voice state. This is a useful foundation.

But that particular test doesn't cover the entire sound system.

I'd expand testing across the following cases:

| Case                             | Required verification                       |
| -------------------------------- | ------------------------------------------- |
| PCM8, PCM16, ADPCM               | Exact output and channel state              |
| High/low playback rates          | Correct fractional advancement              |
| Hard left/right and centered pan | Identical truncation and accumulation       |
| Volume 0 → audible               | Silent voices continue advancing            |
| ADPCM loop boundaries            | Exact predictor and index restoration       |
| Sound stopping mid-block         | Correct final samples and register state    |
| Multiple simultaneous channels   | Correct mixing order                        |
| Capture feedback                 | Correct sample-by-sample dependencies       |
| Reverb buffer wrap               | No stale samples or incorrect loop state    |
| Sound commands during playback   | No missed or reordered commands             |
| Sound alarms                     | Correct replies and scheduling              |
| HOME, sleep and resume           | No hangs, corrupted state or excessive gaps |

For ordinary optimizations, the baseline should be the original Kohjoh implementation, compared through both PCM output hashes and complete emulated sound-chip state.

For intentional accuracy improvements, compare against melonDS's hardware-oriented path and, where possible, actual Nintendo DS recordings or digital sound-state traces.

There are existing accuracy details worth testing separately. For example, GBATEK describes hardware-dependent playback-start delays for PCM, ADPCM and PSG. Kohjoh's `restart_channel()` does not obviously model those delays. Fixing that would be a fidelity improvement, but it should not be mixed into a performance optimization commit.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://mgba-emu.github.io\&sz=32)

GBATEK Markdown Fork



## **10. Recommended implementation sequence**

I would divide this into five passes, with each producing a measurable deliverable.

**Pass 1 — Measure and establish a reference**

Extend `[sndprof]`, fix underrun accounting, collect hardware traces and preserve a pinned output reference. Capture the current cost on the title screen, Goldenrod, a battle and a sound-heavy transition.

**Pass 2 — Cheap structural changes**

Test 192-sample blocks, original-DS interpolation, ordinary stereo fast path, and optimized capture configuration. Benchmark each change independently before combining them.

**Pass 3 — ARM11 instruction optimization**

Inspect `adpcm_render_frames`, `lerp` and `mix_scaled` using `arm-none-eabi-objdump`. Test long-multiply improvements, compact hot loops and ARMv6 packed operations. Monitor executable size and I-cache misses.

**Pass 4 — Reduce repeated work**

Add ADPCM replay statistics. Only build the bounded decoded-sample cache if source reuse justifies its cost. Measure total CPU savings after accounting for cache management.

**Pass 5 — Synchronization and integration**

Address proven sound-lock stalls, test NDSP flushing, run the complete game-scene regression suite and establish the final core-0 audio cost.

### **Acceptance criteria**

Every optimization must pass three independent checks:

**Sound correctness:** Identical PCM and emulated sound state against the appropriate reference, except for an explicitly selected audio-quality mode or separately justified accuracy fix.

**Audio stability:** No additional dropped DSP frames, buffer underruns, lost sound commands or incorrect sequencer timing.

**Actual performance:** A repeatable reduction in Old 3DS CPU consumption, without increased game-frame p99 or missed 30 FPS presentation deadlines.

I'd also require each change to record the compiler configuration, function size and hardware measurement. Azahar instruction counts are useful for screening optimizations, but Coffeepot's measured 16 KiB I-cache cliff makes hardware validation essential.

## **Final assessment**

The strongest near-term opportunities are **removing unnecessary resampling work, specializing common mixing configurations, matching the block size to the sequencer interval, and eliminating repeated capture setup**.

I would prioritize these over elaborate SIMD or sample-caching work because they are easier to verify and don't substantially change the audio architecture.

The longer-term opportunity is removing redundant ADPCM decoding for frequently reused samples. That could be valuable, but only if the working-set measurements support it.

If we can reduce the current approximately 18% mixer cost by a quarter, we'd recover around 1.5 ms per game frame. That is useful headroom on a machine struggling to meet its 30 FPS target.

More importantly, the result would remain portable across every Old 3DS without special CPU permissions or unusual core scheduling.

**I would pursue this software optimization plan first and keep the current core-0 audio architecture until profiling demonstrates a reason to change it.**
