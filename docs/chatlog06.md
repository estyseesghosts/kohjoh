## Initial findings from the new snapshot

I inspected Kohjoh at commit `d433d8a`, including its recent optimization commits, geometry renderer, display-list cache, development notes, and the included Old 3DS log.

The geometry bottleneck has changed significantly since the earlier snapshot. Several expensive paths have been improved, but the newer code still has substantial repeated CPU work.

| Goldenrod, Azahar            | Before new caching | With display-list cache |
| ---------------------------- | ------------------ | ----------------------- |
| Game logic, median           | 21.8 ms            | 13.7 ms                 |
| 2D rendering, exclusive      | 3.4 ms             | 0.6 ms                  |
| 3D draw preparation          | 2.8 ms             | 2.8 ms                  |
| Geometry FIFO words replayed | 0%                 | 74%                     |

These are the project's recorded emulator measurements, not confirmed Old 3DS improvements. The display-list cache is still disabled by default, and the supplied hardware log predates the new work.

Three findings stand out:

1. Geometry caching is working, but incomplete. Most Goldenrod command words can now be replayed instead of decoded. The remaining 26% deserves closer investigation.
2. Ho-Oh's bottleneck has been narrowed down. The project's latest PC samples attribute about 10.6% to software vertex lighting, while skeletal `NODEMIX` accounts for almost nothing. That corrects my earlier suspicion that skinning itself was expensive.
3. The rendering backend hasn't become cheaper. Despite much faster logic, 3D preparation remains around 2.8 ms in Azahar. The CPU still assembles and submits draw calls, validates textures and uploads matrix palettes.

The next question is whether the remaining lag comes from uncached geometry, CPU-side lighting, draw submission, or actual GPU execution. The code exposes several ways to distinguish them.

## 2. The supplied Goldenrod frame reveals a larger problem: invisible geometry

I analyzed the actual decoded 3D scene in:

`private-gitignore/o3ds-log-0/dumps/f05000.kjd`

It contains:

Submitted triangles

# 8,370

Submitted vertices

# 14,030

3D batches

# 124

I transformed the captured vertices using their recorded clip matrices and tested complete triangles against the viewing frustum, including Kohjoh's widened horizontal view.

Conservative frustum classification

8,370 triangles

7,451 trivially outside (89.0%)

919 potentially visible (11.0%)

This is a conservative geometric test, not a hardware rasterization measurement. It does not account for occlusion or all face-culling rules. But approximately nine-tenths of the submitted triangles can be rejected outright using the captured transforms.

Several large batches contain no potentially visible triangles at all.

That changes my assessment of the triangle count: the PICA200 may be able to handle 8,370 triangles, but submitting so much demonstrably offscreen geometry is wasteful, particularly when the ARM11 also spends time constructing it.

The original DS rejects culled polygons before consuming its limited polygon and vertex storage. Kohjoh's current implementation instead prepares the full triangle stream and leaves most rejection to the PICA200.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://twlsdk.randommeaninglesscharacters.com\&sz=32)

twlsdk.randommeaninglesscharacters.com



I'd put conservative geometry rejection near the top of the optimization plan, alongside completing the display-list cache. The key is to reject geometry before costly decoding or submission where possible—not introduce thousands of new per-triangle CPU tests.

### How much could be discarded cheaply?

The capture also contains 122 single-matrix batches out of 124, which makes conservative batch-level visibility testing practical.

My second pass found:

| Goldenrod batch test                                   | Result |
| ------------------------------------------------------ | ------ |
| Total 3D batches                                       | 124    |
| Entire batches outside one frustum plane               | 72     |
| Triangles within those 72 batches                      | 3,720  |
| Batches with at least one potentially visible triangle | 49     |

The three additional batches with no potentially visible triangles are rejected triangle-by-triangle, but aren't proven removable by one simple whole-batch plane test.

A cached bounding box per mesh, transformed against the camera, could avoid 72 draw submissions and 3,720 triangles in this captured frame without testing every triangle individually.

That's a conservative first optimization. More granular spatial chunks could remove considerably more.

There are two separate savings to pursue:

- Submission-stage culling: avoid setting up materials, uploading matrix uniforms and issuing draw calls for entirely invisible batches.
- Pre-decoding culling: avoid reconstructing their vertices and indices in the first place, while still executing DS commands that affect later state or game-visible results.

The second is harder, but potentially far more valuable on the ARM11. It also matches the retained-geometry and spatial-bounds architecture already specified in Kohjoh's own `GPU_3D.md`.

## 3. Why the display-list cache isn't enough

The new cache in `src/runtime/kj_g3.c` is a meaningful improvement, but it mainly avoids interpreting the same commands again.

`dl_append_block()` still copies every retained vertex into the current frame, rewrites its matrix index, copies and rebases the triangle indices, and updates batch state.

That means 74% command replay does not equal 74% less geometry processing.

There are two architectural limitations:

A. Geometry isn't persistent on the GPU. The cache retains decoded vertex data in ordinary memory, then reconstructs the frame's GPU-readable buffers. For an unchanged building, the same geometry can be copied into the buffer every game frame.

B. Replay doesn't understand visibility. It copies geometry whether the building is on screen or not.

I'd evolve this into retained model or mesh resources, with a separate dynamic fallback. Immutable geometry should remain GPU-readable until the model unloads. For the 122 single-matrix batches in the capture, updating a transform should not require rewriting the vertices.

That would realize the architecture described in `docs/engineering/hgss-remaster/GPU_3D.md`.

### One immediate issue: the cache is still opt-in

At `src/runtime/kj_g3.c:2189`, the uncached decoder is still compiled unless `KJ_DL_CACHE` is defined.

So an ordinary `master` build doesn't benefit from the 13.7 ms Azahar result.

Before adding another large optimization, I would finish verifying replay while walking and battling, test it on O3DS, and make it the default if correct. There is already a 6 MiB cache arena; its peak use and eviction behavior also need testing across scene transitions.

## 4. Ho-Oh needs a different solution

The latest title-screen profile is particularly useful:

| CPU hotspot                  | Azahar PC samples              |
| ---------------------------- | ------------------------------ |
| Geometry decoding            | \~13%                          |
| `shade()` software lighting  | \~10.6%                        |
| Display-list cache front end | \~8.2%                         |
| Source comparison (`memcmp`) | \~6.8% before reduced rechecks |
| NODEMIX skinning             | \~0.07%                        |

This is why simply optimizing skeletal matrices would have accomplished very little.

The current cache refuses to retain lists that both modify matrices and generate matrix-dependent vertex attributes. Animated, lit models therefore continue down the expensive decoder path.

There's also a 64-entry direct-mapped normal-lighting cache in `kj_g3.c`. It invalidates results when the vector matrix changes, which is reasonable for correctness but limits reuse across animated poses.

I would investigate separating immutable mesh geometry from changing lighting. Keep the same mesh and update pose and lighting independently.

Moving some lighting to the PICA vertex shader is one candidate, but not automatically the best. Kohjoh's existing vertex shader already performs indexed matrix transforms; additional lighting instructions and uniforms could shift the bottleneck to the GPU. Exact DS fixed-point lighting also isn't guaranteed to match floating-point shader results.

First measure lighting cache misses, unique normals, matrix changes and time spent in `shade_uncached()`. Then compare a retained CPU-lit approach against a GPU-lit prototype on O3DS.

## 5. We still don't have enough GPU measurements

The historical O3DS Goldenrod log recorded `gpu_wait` around 6 microseconds at the median, even while overall performance was terrible.

That suggests the CPU was usually keeping the GPU waiting, rather than vice versa. But that log used the older instrumented build, and it doesn't independently establish GPU execution time.

Kohjoh should now measure:

| Metric                                         | Source                 | Purpose                                                           |
| ---------------------------------------------- | ---------------------- | ----------------------------------------------------------------- |
| GPU queue completion time                      | `C3D_GetDrawingTime()` | Establish GPU workload, attributed to the correct submitted frame |
| CPU draw recording                             | `KJ_PERF_G3D`          | Submission and preparation overhead                               |
| Command-buffer usage                           | `C3D_GetCmdBufUsage()` | Detect command pressure and frame splits                          |
| ARM11 I-cache misses                           | Performance counters   | Find instruction-fetch stalls                                     |
| Decoded/replayed vertices, not just FIFO words | New counters           | Quantify work actually avoided                                    |
| Culled batches/triangles before submission     | New counters           | Verify the visibility optimization                                |

The citro3d [render queue implementation](https://github.com/devkitPro/citro3d/blob/master/source/renderqueue.c?utm_source=chatgpt.com) provides the relevant GPU timers. Its completion measurement is asynchronous, so the profiler must not incorrectly assign the previous frame's GPU time to the current frame.

Coffeepot's [draw-call benchmarks](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/gpu-rendering.md?utm_source=chatgpt.com) measured approximately 9.7 microseconds per separate draw on O3DS. Eliminating 72 draws is therefore potentially worthwhile even before accounting for matrix uploads or GPU savings, but that's a reference estimate, not a Kohjoh measurement.

One small additional inefficiency: `kj_g3d_draw()` binds the 3D program, invalidates the cached program state, and immediately binds it again. That redundant first bind should be removed, although it isn't a major bottleneck.

## 6. The implementation order I'd use

| Order | Change                                                                 | Expected benefit                                              |
| ----- | ---------------------------------------------------------------------- | ------------------------------------------------------------- |
| 1     | Test `KJ_DL_CACHE` enabled on real O3DS, Goldenrod walking and battles | Establish the actual new baseline                             |
| 2     | Conservative whole-batch frustum culling                               | Remove unnecessary draws and GPU transforms                   |
| 3     | Attach immutable geometry and bounds to retained models                | Eliminate repeated vertex/index construction                  |
| 4     | Move visibility decisions before retained geometry is copied           | Remove substantial CPU work on offscreen objects              |
| 5     | Split changing lighting from immutable geometry                        | Make animated lit models cheaper                              |
| 6     | Benchmark a simpler single-matrix shader                               | Potential GPU saving for common field models                  |
| 7     | Reprofile 2D during movement                                           | Check whether the static-layer improvements survive scrolling |

For the culling work, I'd start with a deliberately conservative implementation in `g3d.c`, using cached mesh bounds and the existing clip matrices. This should be comparatively straightforward to validate against reference captures.

Once proven, move that rejection earlier into `kj_g3.c` or the model-submission layer. This requires preserving all DS-visible state changes and geometry-test results even when no vertices are emitted. The original DS's geometry processing and culling semantics are useful references; melonDS implements them in [GPU3D.cpp](https://github.com/melonDS-emu/melonDS/blob/master/src/GPU3D.cpp?utm_source=chatgpt.com).&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



Each change should preserve images and gameplay state, and be measured in Goldenrod at rest, walking south, entering Route 34, Ho-Oh/Lugia and a battle.

## Assessment

I think Kohjoh's progress is substantial, but we're still missing two major pieces of the original performance plan: persistent GPU geometry and early visibility rejection.

The evidence is stronger now than it was in the previous review. The actual Goldenrod capture demonstrates that a huge amount of invisible geometry reaches the rendering backend. The display-list cache substantially reduces decoding but still copies that geometry, and the cache isn't enabled by default.

I'd prioritize those issues over further tweaks to individual geometry opcodes. They should benefit Goldenrod and other crowded scenes without increasing the PICA200's shader complexity.

The remaining uncertainty is how much of the improved CPU time translates to actual O3DS frame time. Until the current cached build runs on hardware with CPU and GPU timing separated, we cannot quantify how close Goldenrod is to locked 30 FPS.

The encouraging part is that the new evidence identifies removable work, not an unavoidable hardware limit.
