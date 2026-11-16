## Initial diagnosis

I inspected Kohjoh at commit `923789b`, including its performance logs, recent optimization work, rendering code, and profiling infrastructure.

The remaining problem looks primarily like a CPU submission and compatibility-layer bottleneck, not simply a PICA200 that cannot render the scenes.

There are three important clues:

1. Route 34 approaches 30 FPS, but Goldenrod collapses. This points toward work that scales with scene complexity: geometry commands, object processing, textures, and rendering submissions.
2. Ho-Oh stutters while other title sequences run smoothly. That suggests a geometry-heavy or animation-specific path rather than a universal graphics-performance problem.
3. The existing hardware profile already identifies expensive code. Goldenrod processes approximately 51,000 DS geometry words, 124 draws, and 8,400 triangles per game frame. Its G3 command decoder was a major CPU consumer.

The project has already made some correct changes: shortening the decoder to fit the ARM11 instruction cache, removing expensive profiling calls, caching texture state, and limiting consecutive skipped presentations.

However, Kohjoh's latest documented Goldenrod timings are still from Azahar. The earlier Old 3DS measurements are from commit `92a9a6d`, before those optimizations. We need to distinguish remaining bottlenecks from ones that may already have been solved.

## Remaining hot paths, in priority order

| Priority | Hot path                                 | Why it matters                                          |
| -------- | ---------------------------------------- | ------------------------------------------------------- |
| 1        | G3 command decoding                      | Large recurring CPU cost, including static scenery      |
| 2        | 2D layer rebuilding and scanline effects | Previously 13 ms per presented picture on O3DS          |
| 3        | H-blank emulation                        | Potentially expensive even when little changes visually |
| 4        | Ho-Oh's matrix and skinning operations   | Distinct workload from ordinary field geometry          |
| 5        | Pokémon data processing                  | Previously around 11% of Goldenrod's sampled execution  |
| 6        | Texture validation and draw submission   | Worth optimizing once the larger costs are confirmed    |

### 1. Stop rebuilding unchanged 3D geometry

Code: `src/runtime/kj_g3.c`, particularly `decode_words()`, `hot_command()` and `NitroPort_GxFifoWords()`.

Goldenrod sends roughly 51,000 geometry words per frame. Kohjoh interprets those commands, reconstructs vertex data and batches triangles for the PICA200.

The major architectural opportunity is that static scenery is being decoded repeatedly even when its underlying model has not changed.

Kohjoh already has a display-list replay implementation. Unfortunately, `KJ_DL_CACHE` is disabled on `master` because of heap corruption. The `wip/savestore` branch contains fixes and a verification mode, but currently has unrelated build regressions.

I would prioritize making that cache safe and usable, with some constraints:

- Cache repeatable vertex and triangle generation, not arbitrary DS GPU commands.
- Continue executing commands that affect game-visible state, matrix readbacks, lighting and geometry tests.
- Measure actual cache admission and replay frequency. Identical model data alone doesn't prove the resulting geometry is reusable.
- Verify output against uncached decoding, including moving and animated objects.

A longer-term improvement is caching decoded geometry at the NitroSystem model level, rather than depending entirely on DMA display-list boundaries.

This is the change most likely to remove substantial repeated work rather than just making the same work marginally faster.

### 2. Investigate the 2D retention cliff

Code: `src/graphics/g2d/g2d.c:860–1090`, `:1223–1285`.

This is the most interesting potential explanation for an abrupt location-dependent slowdown.

Kohjoh retains unchanged backgrounds and sprites. However:

```
const bool single_span = !in->lines.active;
```

When scanline registers differ, the renderer takes the multi-span path and calls:

```
prepare_span(&c, false, &pr);
```

That `false` disables retained-layer reuse.

Consequently, a scene with scanline effects can repeatedly rebuild geometry that is otherwise static. Multiple spans also repeat rendering setup and layer processing.

I haven't established that Goldenrod activates this path. The existing `[gpu] spans` counter can answer it.

If Goldenrod has significantly more spans and rebuilt layers than Route 34, I would separate cached layer geometry from per-span render state. Reuse the same BG and OBJ geometry across spans, applying different scissor, window and blending state as necessary.

There's a second issue: `chars_same()` compares the entire contiguous character-memory range used by a layer. A sparse set of tiles can therefore cause a much larger comparison than strictly necessary.

Possible improvement: exact dirty-page tracking for DS VRAM, with conservative invalidation for unmapped writes and DMA. Don't replace comparisons with hashes unless the hash can be maintained cheaply.

This aligns with Coffeepot's [retained 2D tile-renderer experiments](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/2d-tile-renderer.md?utm_source=chatgpt.com), although Kohjoh's blending and scanline requirements are more complex.

### 3. H-blank is its own CPU workload

Code: `src/compat/nitro/src/port_os.c:194–291`.

The implementation simulates up to 192 H-blank events, including register snapshots, DMA and interrupt handlers.

An existing source comment reports an earlier O3DS hand-off cost of approximately 9.5–13.8 ms, although that is not a measurement of the current build. The recent changes greatly reduced Azahar's H-blank cost, from 2.1 to 0.5 ms.

This needs another real-hardware measurement.

In particular, distinguish:

- Register snapshot and comparison cost.
- H-blank DMA copies.
- Game interrupt-handler execution.
- Interrupt dispatch and thread-switch overhead.

Don't eliminate H-blank events merely because they don't affect the picture. Game logic may depend on those interrupts. Optimize the emulation machinery while preserving observable behavior.

### 4. Treat Ho-Oh as a separate performance case

Code: NitroSystem patch `0305`, especially `NNSi_G3dFuncSbc_NODEMIX`; `src/runtime/kj_g3.c`.

Ho-Oh exercises weighted-envelope animation and matrix operations. Kohjoh's patch correctly forwards direct geometry-register writes, fixing previously missing matrix state.

However, this creates substantial work involving matrix restoration, multiplication, readback and weighted blending.

Ordinary static-model caching may help little here because the animation changes the input state.

I'd separately profile:

- `NNSi_G3dFuncSbc_NODEMIX`
- `NNS_G3dGetCurrentMtx`
- `G3_MultMtx43` and `G3_MultMtx33`
- `MTX_RESTORE`, `MTX_MULT_4x3` and geometry readbacks

If these dominate, consider caching invariant inverse/envelope matrices and reducing repeated CPU calculations. Preserve the DS's fixed-point results where they're observable.

The smooth title sequences are useful controls: compare Ho-Oh against another 3D title scene using exactly the same counters. That will tell us whether the difference is geometry volume, skinning or GPU rendering.

### 5. Revisit the game-side costs

Goldenrod's earlier PC samples attributed approximately 11% of execution to Pokémon data encryption and decryption.

The game repeatedly calls `GetIdxOfFirstPartyMonWithMove`, scanning party Pokémon and calling `GetMonData`.

Kohjoh now has keystream caching and word-wide XOR optimizations. Those may have removed much of the cost.

If fresh samples still show this path prominently, look at eliminating repeated queries or reusing validated decrypted data within the field update. Optimizing the function itself may yield less than avoiding unnecessary calls.

### 6. Draw calls and textures come after those

Code: `src/graphics/g3d/g3d.c:170–260`, `src/graphics/g3d/textures.c:374–450`.

The 3D renderer already avoids some redundant texture, TEV and matrix state updates.

Coffeepot's [O3DS drawbench](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/gpu-rendering.md?utm_source=chatgpt.com) measured roughly 9.7 microseconds per separate draw. At 124 draws, that's about 1.2 ms as a rough reference, before workload-specific state changes.

That's meaningful, but less compelling than removing repeated decoding or layer rebuilds.

The texture cache also compares original DS VRAM contents to cached copies. The existing `tex bytes compared` and `tex uploads` counters should reveal whether this is significant.

I would not prioritize cache-flush tricks yet: Coffeepot's [hardware measurements](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/memory-and-cache.md?utm_source=chatgpt.com) put the normal whole-linear-heap flush overhead around 90 microseconds.

## The next O3DS measurement

Run the current uninstrumented release-style build, with the lightweight counters enabled, in these four situations:

| Scene                                   | What to compare                        |
| --------------------------------------- | -------------------------------------- |
| Route 34, away from Goldenrod           | Baseline near 30 FPS                   |
| Route 34 immediately south of Goldenrod | Exact point where performance declines |
| Central Goldenrod                       | Worst-case steady workload             |
| Ho-Oh versus a smooth title 3D scene    | Animation-specific workload            |

Capture existing `[perf]`, `[frame]`, `[pace]`, `[gpu]` and `[ds]` records. In particular, compare `logic`, `g2d`, `g3d`, `gpu_wait`, `spans`, reused/rebuilt layers, geometry words, texture bytes compared and uploads.

One profiler correction is important: the current 600-frame reporting window counts presented pictures, not necessarily game ticks. Under frame skipping, some reported per-frame averages are misleading. Record both denominators separately.

Avoid `KJ_G3_PROFILE` and `HGSS_PROFILE=on` for the initial measurements; their per-command probes substantially perturb O3DS timings.

For any suspected optimization, run an A/B comparison on the same hardware and save state, recording p50, p95, p99 and missed 30 FPS deadlines. Coffeepot's [hardware testing guide](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/testing-on-hardware.md?utm_source=chatgpt.com) provides the methodology.

## What I would do first

My preferred sequence is O3DS reprofile → prove or disprove the 2D scanline-retention cliff → stabilize geometry replay → profile Ho-Oh independently.

The 30 FPS target is approximately 33.4 ms per game frame. The latest Azahar measurements already account for about 28 ms in logic, 2D and 3D submission combined, before accurately accounting for O3DS instruction-cache penalties and other hardware costs.

Coffeepot's [CPU benchmarks](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/cpu-optimization.md?utm_source=chatgpt.com) explain why this matters: a hot instruction footprint exceeding the ARM11's 16 KiB instruction cache can become extremely expensive. Shrinking the decoder was appropriate, but retaining unchanged work would be better still.

The strongest direction is to stop doing work the game doesn't require, especially repeated geometry translation and repeated 2D layer construction. Those are more promising than further micro-optimizing individual PICA200 calls.

All current conclusions about the latest build remain static-analysis findings until that build is profiled on the real Old 3DS.

## 1. Goldenrod's 8,400 triangles deserve investigation

The original Nintendo DS has a hardware limit of 2,048 stored polygons and 6,144 vertices per frame. Nintendo's 2004 developer documentation confirms those limits, but also specifies that polygons rejected by culling don't count toward them.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://twlsdk.randommeaninglesscharacters.com\&sz=32)

twlsdk.randommeaninglesscharacters.com



Kohjoh reports approximately 8,400 triangles per presented Goldenrod frame. That is substantially higher than the DS's visible geometry budget, but it doesn't automatically mean Kohjoh is drawing incorrect geometry.

The distinction matters:

- The DS accepts geometry commands, transforms polygons, rejects invisible geometry, and stores eligible polygons up to its hardware limits.
- Kohjoh translates the incoming DS geometry into PICA200 triangle lists, splitting every quad into two triangles.
- The PICA200 then performs vertex transformation, clipping and face culling.

Kohjoh currently counts triangles submitted to the PICA200, not triangles that survive clipping and actually render.

I checked `src/runtime/kj_g3.c` and `src/graphics/g3d/g3d.c`. The number is derived from `index_count / 3` at submission. It is not a GPU rasterization statistic.

I also checked whether the compositor draws the same 3D scene repeatedly for its 2D layers. `draw_span()` uses a `drew_3d` guard, so it normally submits the 3D scene only once per screen, even when there are multiple scanline spans.

My conclusion is that 8,400 submitted triangles is plausible, but it's not yet demonstrated to be necessary. We need to establish how much geometry survives culling, and whether the CPU is spending milliseconds generating geometry the GPU immediately rejects.

The polygon count itself is unlikely to be the whole explanation for Goldenrod's poor performance. The more troubling figure is the roughly 51,000 DS geometry words decoded every game frame, especially on a 268 MHz ARM11.

## 2. The CPU is doing work the original DS delegated to hardware

Kohjoh's division of work is fundamentally sensible: the ARM11 emulates DS graphics state, and the PICA200 performs vertex transformation and rasterization. But the compatibility layer still reconstructs an entire stream of geometry commands every frame.

There are four particularly important costs.

| CPU operation     | Current implementation                                                             | Better long-term approach                                     |
| ----------------- | ---------------------------------------------------------------------------------- | ------------------------------------------------------------- |
| Geometry decoding | Reinterprets thousands of FIFO commands                                            | Retain decoded models and display lists                       |
| Matrix processing | Fixed-point matrix operations, clip-matrix reconstruction and palette construction | Cache reusable transforms; update only changed matrices       |
| Vertex lighting   | `shade()` computes DS lighting on ARM11                                            | Cache static lighting or migrate suitable calculations to GPU |
| Vertex assembly   | Rebuilds vertex and index arrays each frame                                        | Persistent meshes with dynamic transform palettes             |

The renderer already uses object-space vertices and GPU matrix palettes. That's a good foundation. What's missing is persistent geometry across frames.

### Goldenrod: the right architecture

For static buildings and map geometry, I would aim for this:

Decode map/model geometry once

Preserve the DS-visible state changes and tests

Keep immutable mesh data in GPU-readable memory

Vertices, indices, material runs and bounds

Each game frame

Update camera and changed transforms → perform object-level visibility checks → submit surviving meshes

This avoids both repeated command decoding and repeatedly copying unchanged geometry into the frame's vertex buffer.

The existing `KJ_DL_CACHE` is a useful intermediate implementation, but even its replay path copies vertices and indices into the current frame. It is not yet true persistent GPU geometry.

The wide camera complicates culling, but Kohjoh already handles the expanded view in `box_test()`. We should preserve that rule when moving static meshes to retained resources.

## 3. Ho-Oh/Lugia: there is a specific CPU-heavy path

The strongest suspect is NitroSystem's weighted-envelope animation, not the raw triangle count.

In `patches/NitroSystem/0305`, the `NNSi_G3dFuncSbc_NODEMIX` path:

1. Restores skeletal node matrices.
2. Multiplies inverse bind matrices.
3. Reads back position and direction matrices.
4. Computes weighted combinations of matrices.
5. Loads the resulting matrices back into the emulated DS graphics engine.

Those operations were originally assisted by the DS geometry hardware. Kohjoh now performs the matrix arithmetic and graphics-register emulation on the ARM11.

The corresponding code in `kj_g3.c` uses signed 64-bit intermediate arithmetic to reproduce the DS fixed-point behavior. That is correct for compatibility, but potentially expensive when repeated across animated nodes.

I have not found a Ho-Oh-specific CPU timing measurement in the supplied archive. So this is a strong architectural suspect, not a confirmed measured hotspot.

For Ho-Oh and Lugia, I would target persistent model vertices, indices and materials. Each animation tick should principally update the skeletal pose, compute the necessary final matrices, upload the changed matrix palette and submit the same mesh.

We should not regenerate a bird's entire mesh just because its wings moved.

There is one important restriction: changing that path must not break NitroSystem's matrix readbacks or weighted-envelope results. The existing Ho-Oh correctness fixes specifically depend on those operations.

## 4. Two more findings that change how I'd profile this

First, 2D layering isn't inflating the reported 3D triangle count. The `3D tris` statistic comes exclusively from `kj_g3d_draw()`. Background tiles, sprites and blend passes have separate counters. Goldenrod's 8,400 triangles therefore represent the 3D model pipeline alone.

Second, the profiler has overlapping timings. `KJ_PERF_G2D` measures the complete screen draw, including calls to `kj_g3d_draw()`, which are separately charged to `KJ_PERF_G3D`.

That means adding `logic + g2d + g3d` overstates total CPU time. The approximate Azahar baseline is 21.8 ms of logic plus 3.4 ms of screen rendering, not 21.8 + 3.4 + 2.8 ms. GPU wait and submission are additional.

This correction matters when calculating how much of the 33.4 ms budget remains.

### There's also a DS geometry-limit correctness gap

`NitroPort_Gx3dRefreshStatus()` clamps the polygon count reported to the game at 2,048, but the renderer doesn't actually enforce the original DS's post-culling polygon and vertex-storage limits.

That could cause Kohjoh to draw geometry that the DS would have dropped.

I would investigate this with reference captures rather than simply truncating the triangle list: the original limits apply after culling, and blindly dropping triangles would remove legitimate geometry.

## 5. The decisive measurements

The next profiling pass should add a small number of counters that answer specific questions.

| Counter                                     | Question it answers                             |
| ------------------------------------------- | ----------------------------------------------- |
| Source triangles/quads vs PICA triangles    | How much geometry does translation expand?      |
| Triangles rejected by clipping/culling      | Is most generated geometry invisible?           |
| G3 decode CPU time                          | Is Goldenrod limited by command interpretation? |
| Matrix operations and cache hits            | How much work is being repeated?                |
| NODEMIX time and invocation count           | Is Ho-Oh's skinning the culprit?                |
| G3D submission vs actual GPU execution time | CPU-bound or GPU-bound?                         |
| 2D spans, rebuilt quads, reused layers      | Does Goldenrod's 2D workload cause the cliff?   |

For the first GPU-versus-CPU isolation test, use a temporary build that keeps DS game logic and G3 decoding intact but skips the final PICA 3D draw. Compare frame time against the normal build in Goldenrod and Ho-Oh. The resulting picture will be incomplete, but the timing difference will isolate the rendering cost. Use GPU completion timing alongside it to distinguish GPU execution from CPU draw recording.

Then profile the geometry decoder and Ho-Oh's matrix functions separately. Don't instrument every individual geometry command with clock reads; Kohjoh already demonstrated how expensive that becomes on O3DS.

## 6. Implementation order I'd recommend

1\. Remove redundant geometry decoding

Repair and validate the display-list cache. Measure hits, work saved and memory copies. Then extend retention toward immutable model-level meshes so static scenery does not have to be rebuilt.

2\. Eliminate unnecessary matrix work

Start with `clip_matrix()`, `current_gmtx()` and the repeated restore/readback path. Reuse matrix products when their inputs are identical, including across repeated model submissions.

3\. Give Ho-Oh/Lugia a retained animated-mesh path

Profile NODEMIX first. Keep the mesh static and update only the skeletal pose, required lighting and GPU matrix palettes. Preserve exact game-visible matrix results.

4\. Fix 2D retention under scanline effects

The existing multi-span path disables reuse. Verify whether Goldenrod exercises it, then retain geometry independently of per-span display state wherever correct.

The important architectural lesson from Coffeepot's [CPU measurements](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/cpu-optimization.md?utm_source=chatgpt.com) and [3D renderer guidance](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/renderer-architecture.md?utm_source=chatgpt.com) is that reducing work matters more than increasing the complexity of already-hot loops.

My assessment: I wouldn't panic over 8,400 submitted triangles. That's not inherently unreasonable for a PICA200, and the available evidence doesn't establish GPU triangle throughput as the limiting factor.

I would be considerably more concerned that Kohjoh still reconstructs much of the scene on its weak ARM11 every frame. Goldenrod should not require retranslating unchanged buildings, and Ho-Oh should not require regenerating an unchanged mesh for each animation pose.

That's where I would concentrate the next engineering pass.
