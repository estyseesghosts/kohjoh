## **Kohjoh v0.0.1 — performance investigation**

I found the [v0.0.1 release](https://github.com/estyseesghosts/kohjoh/releases/tag/v0.0.1?utm_source=chatgpt.com), published November 13, and reviewed its architecture, profiling notes, and rendering code.

**My initial assessment: the two slowdowns likely have different immediate causes, but share an expensive CPU-side geometry pipeline.** I would investigate that pipeline before attempting another broad optimization pass.

The release's existing measurements provide useful evidence:

| Scene       | Existing evidence                                                                       | Main suspect                                                                        |
| ----------- | --------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------- |
| Ho-Oh title | CPU lighting accounts for \~10.6% of sampled execution; geometry decoding another \~13% | Dynamic, illuminated geometry cannot use the current display-list cache effectively |
| Goldenrod   | \~51,000 geometry FIFO words, 124 draws and 8,400 triangles per game frame              | Geometry processing, draw submission and repeated work                              |
| Route 34    | Previously approached 30 FPS on original hardware                                       | Much lighter workload, confirming a scene-dependent bottleneck                      |

Importantly, **the latest v0.0.1 CPU optimizations haven't been conclusively benchmarked on original 3DS hardware**. Some of the strongest measurements come from Azahar, which isn't a reliable model of actual ARM11 cache behavior or hardware execution time.

There are two questions to resolve first: whether the title and Goldenrod are missing their 33.4 ms game-frame deadline, and exactly which work consumes that deadline.

## **1. The main architectural bottleneck**

The most significant finding is in [`kj_g3.c`](https://github.com/estyseesghosts/kohjoh/blob/v0.0.1/src/runtime/kj_g3.c?utm_source=chatgpt.com#L2047-L2148).

Your display-list cache avoids repeatedly decoding much of Goldenrod's static geometry. It has achieved approximately **74% FIFO-word replay coverage**, which is a substantial improvement.

However, a cache hit still does considerable work:

- Replays the display list's state-changing commands.
- Copies cached vertices into the current frame's vertex buffer.
- Rewrites matrix indices.
- Rebuilds triangle indices.
- Reconstructs the batches that Citro3D will draw.

This is **cached decoding, not fully retained GPU geometry**.

That distinction matters enormously for a scene like Goldenrod, because the environment stays loaded and continuously submits geometry even when much of it is outside the camera.

I would investigate two complementary changes:

**A. Persistent geometry:** Keep immutable model vertices and indices in GPU-accessible memory, updating transforms and material state without copying their geometry every frame.

**B. Earlier visibility rejection:** Determine which models or batches can be excluded before doing expensive geometry processing, rather than leaving all rejection to the GPU.

The existing worklog reports that 72 of 124 Goldenrod batches lie entirely outside at least one frustum plane. That's an excellent experiment candidate, but skipping those draws only at the Citro3D stage won't eliminate the earlier CPU decoding and copying costs.

## **2. Ho-Oh exposes a separate cache limitation**

[`shade()`](https://github.com/estyseesghosts/kohjoh/blob/v0.0.1/src/runtime/kj_g3.c?utm_source=chatgpt.com#L389-L450) performs DS-style vertex lighting on the ARM11: normal transformation, light direction calculations, diffuse/specular contributions and colour conversion.

The display-list recorder then rejects lists where vertex output depends on changing matrices. Ho-Oh's lit lists hit precisely this limitation.

I'd prototype a different representation:

- Retain Ho-Oh's source geometry and normals independently of its animation.
- Update the changing bone/clip matrices as separate data.
- Calculate lighting in the PICA vertex shader where feasible, rather than baking every vertex's colour into the CPU-generated stream.

Your existing shader uses 64 of the PICA's 96 float-vector uniforms for its 16-matrix palette. That leaves limited space for lighting state, and full DS lighting—including shininess—will need careful validation.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://3dbrew.org\&sz=32)

3dbrew



A simpler intermediate experiment is also worthwhile: retain the geometry while continuing CPU lighting, but update only the colours that actually change. This could test the benefit of separating geometry from illumination without immediately implementing a complete GPU lighting system.

## **3. What I'd measure next**

Before changing either renderer, I'd run the same v0.0.1 revision through a controlled hardware comparison.

| Test scene                        | What we're looking for                                       |
| --------------------------------- | ------------------------------------------------------------ |
| Ho-Oh, title screen               | CPU lighting cost, uncacheable lists, irregular presentation |
| Goldenrod, stationary             | Baseline geometry decoding and replay costs                  |
| Goldenrod, walking                | Additional camera, animation, 2D and streaming costs         |
| Goldenrod edge, city still loaded | Whether offscreen city geometry remains expensive            |
| Just beyond city unload           | Which CPU costs disappear when the city unloads              |
| Route 34                          | Full-speed control case                                      |

Use the uninstrumented build for actual smoothness, then `HGSS_PROFILE=on` for attribution. The existing profiler already supplies most of the measurements.

Read these together

| Counter              | What it establishes                                        |
| -------------------- | ---------------------------------------------------------- |
| `[gamecpu]`          | Actual per-game-frame CPU work, including skipped pictures |
| `[g3sample]`         | Geometry decode vs replay vs block copying                 |
| `[dlcov]`, `[dlwhy]` | Why individual display lists miss the cache                |
| `[perf]`             | 2D, 3D, submission and GPU-wait costs                      |
| `[gputime]`          | GPU execution and command-buffer pressure                  |
| `[pace]`, `[frame]`  | Late frames, skipped pictures and irregular intervals      |
| `[sndprof]`          | Background audio mixer cost                                |

The previous hardware logs recorded roughly **4 ms GPU queue completion and 7 µs GPU waiting** in steady field conditions. That strongly suggests CPU pressure is the dominant issue there, although it needs confirmation on the release build.

I also found a measurement gap: [`perf.c`](https://github.com/estyseesghosts/kohjoh/blob/v0.0.1/src/diagnostics/perf.c?utm_source=chatgpt.com) counts physical refreshes between submissions, not confirmed changes on the display. Citro3D submits GPU work asynchronously and swaps buffers when the queue completes.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub+1

&#x20;For visibly choppy animation, I would add explicit submitted/completed/presented frame IDs.



## **4. Four experiments I'd run before implementing major changes**

01

**Remove 3D draw submission experimentally**

Keep game simulation, FIFO decoding and DS-visible geometry results intact, but bypass Citro3D 3D drawing. If Goldenrod remains slow, the main bottleneck is earlier in the pipeline. If it recovers, concentrate on draw recording and GPU work.

02

**Measure the top 20 most expensive display lists**

Group by source pointer, call site, FIFO words, decode/replay cost, copied bytes and cache rejection reason. This should reveal whether a handful of large models dominate Goldenrod or whether the overhead is distributed across hundreds of small submissions.

03

**Cull offscreen batches as a diagnostic**

First reject fully invisible batches immediately before `C3D_DrawElements`, preserving all DS execution. Measure the gain. Then move rejection earlier for safe, immutable geometry. Compare both versions to quantify how much wasted work occurs before drawing.

04

**Isolate lighting and audio costs**

Temporarily substitute constant lighting on the title screen to establish an upper bound for eliminating `shade()`. Separately benchmark audio mixing against a timing-preserving silent diagnostic path. The existing logs attribute roughly 18% of the application core to audio; that cost overlaps the measured game-thread wall time, so don't add it twice.

These are diagnostic switches, not proposed shipping compromises.

## **5. Two things I'd avoid prioritizing**

**2D optimization:** Goldenrod idle already showed a retained bottom screen, no atlas conversions and approximately 0.6 ms exclusive G2D work in Azahar. Walking still needs testing, but I wouldn't redesign the tile renderer based on the current evidence.

**Pokémon-data decryption:** Earlier samples attributed substantial execution to party-data accesses, but patch 2006 produced only about a 0.1 ms improvement in Goldenrod. More speculative optimization here is unlikely to address the main problem.

## **6. One extra question about Ho-Oh**

I'd compare the original game in melonDS frame by frame against Kohjoh, measuring actual changes in Ho-Oh's model matrices.

There are two possible problems: the game updates the animation too slowly, or it updates correctly but Kohjoh fails to display some frames.

The first would implicate scheduling, animation or geometry processing. The second implicates rendering deadlines and the presentation-skip policy in [`renderer.c`](https://github.com/estyseesghosts/kohjoh/blob/v0.0.1/src/graphics/renderer.c?utm_source=chatgpt.com#L106-L145).

There's also a possible periodic hitch: unrecordable lists are normally spared repeated content comparisons, but they're rechecked every 64 scene swaps. Check whether these rechecks correlate with title-screen frame-time spikes.

## **Recommended direction for v0.0.2**

I'd organize the next performance pass around **three deliverables**:

1. **A trustworthy O3DS scene profiler** that distinguishes game updates, GPU submission, GPU completion and actually displayed frames, with per-display-list cost attribution.
2. **Retained, visibility-aware overworld geometry**, aimed at eliminating Goldenrod's repeated static-model copying and offscreen work.
3. **A separate Ho-Oh lighting/animation path**, retaining source geometry independently of its changing lighting and transforms.

I would implement them in that order, with an A/B benchmark after each change.

The larger opportunity isn't another 5% improvement to the existing FIFO loop. It's making Goldenrod cheap enough that **having the city loaded no longer means reconstructing and processing its geometry every game frame**, even when most of it isn't visible.
