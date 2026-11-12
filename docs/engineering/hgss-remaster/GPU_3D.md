# GPU_3D — direct native 3D reconstruction of HGSS worlds and battles

**Status:** proposed GPU renderer contract. **Minimum hardware:** launch O3DS, 268 MHz CPU baseline; **presentation:** 30 distinct frames per second. Related: [GPU_2D](GPU_2D.md), [GPU_RESOURCES](GPU_RESOURCES.md), [FRAME_PIPELINE](FRAME_PIPELINE.md), [ASSET_BUILDER](ASSET_BUILDER.md).

## 1. The goal is a DS command translator, not a DS pixel renderer

DS 3D commands express vertex attributes, transforms, textures, polygon modes, lighting and raster ordering. Existing `hgss-3ds` has working game-to-PICA integration but reports substantial CPU geometry construction in its original O3DS Goldenrod profiling. It is a **compatibility reference**, not a recommended new geometry hot path. The related [Platinum port](https://github.com/GabeSud0/pokeplatinum-3ds) demonstrates direct GPU composition but its specific costs and fixes are not automatically transferrable.

**Required path:** game/SDK's G3 stream → stable immutable command decode → retained GPU geometry/texture/material resources → per-frame transforms + dynamic updates → PICA vertex units and rasterizer → native 400×240 GPU target. No full-frame CPU triangle raster, GPU readback or CPU-transformed duplication of static meshes every frame.

The PICA200 has a programmable **vertex** stage, fixed-function fragment operations (TEV, blending, fog, tests and lighting functionality) and no arbitrary programmable fragment shader. Match the actual hardware API; document unavoidable visual remaster approximations individually.

## 2. First perform a command-feature census

Record from the **real game** every G3 command used at boot, title, overworld, battles, cutscenes and transitions. The implementer must produce a machine-readable table of frequency, argument values, mode/state changes, maximum live vertices, texture formats and correctness classification. Include dynamic call-list memory and state inherited at each call boundary.

| Group | Observe | GPU treatment |
| --- | --- | --- |
| Geometry primitives | Vertex formats, primitive begin/end, strips, quad triangulation | Decode to native packed vertices/indices; retain if immutable |
| Matrix modes/stacks | Projection/position/vector/texture matrices, push/pop/store/restore | Resolve CPU-visible DS stack semantics; move vertex transforms into GPU where equivalent |
| Vertex attributes | Color, normal, texture UV, material | Packed attributes plus shader uniforms; preserve original quantization |
| Polygon attrs | Alpha, mode, ID, backface, depth/translucency behavior | Explicit PICA materials / ordered passes; unsupported combinations tracked |
| Lighting | Normals, light vectors/colors, diffuse/specular/toon | Fixed-function/PICA LUT/TEV where provably equivalent |
| Texture state | Formats, palette, wrapping, transforms | Offline or load-time direct-color conversion, cache by content+palette generations |
| Raster effects | Edge marks, fog, alpha test, shadow polygons | GPU-specific pass only when tested; avoid ordinary CPU pixel composition |
| Buffers / clears | Depth/stencil and frame boundaries | Native targets, measured formats, explicit order |
| Visibility | DS viewport clipping/culling, room scene data | Widen horizontal view to 320 logical units; preserve depth/near/far behavior |

Don't assume that reproducing visible triangles proves correctness: game logic may inspect graphics registers, transformation results or flags. Keep DS-visible state even when GPU drawing skips work.

## 3. Retained geometry: cache admission proof

A decoded geometry object may be retained only if the *bytes and interpretation* that determine its vertex data are invariant. Cache key must cover at least: original display-list identity, full source bytes or source generation, matrix/vertex decode mode if baked, material and texture assumptions, and converter version. Dynamic inherited transforms should be **excluded from the immutable mesh** and supplied separately. If a display list changes in-place, invalidate the old mesh before reuse.

**Three runtime paths**:

1. **Static:** models and room scenery decoded at asset build/load time to GPU-ready immutable VBOs; CPU only binds mesh/material and transform.
2. **Transform/animate:** same immutable vertices, per-object matrix or bounded dynamic bone/vertex updates. Measure CPU skinning separately; don't assume the PICA supports an arbitrary desktop-style skeletal shader.
3. **Unusual/dynamic:** streamed vertex data in a guarded per-frame linear-memory ring; no software raster fallback. Explicitly audit memory and max primitive counts.

Potential prototype: test a bounded palette of per-object matrices, with matrix IDs in packed vertex metadata, versus one uniform update and draw per object. The former is a *hypothesis* and must be checked against actual PICA shader instruction/uniform limits, register pressure and material boundaries. A “matrix ring” is not a free unlimited array.

## 4. GPU-side transforms and data formats

Prefer object-space vertices and a transform matrix in a vertex shader instead of CPU multiplying every vertex on every frame. A source DS vertex may be fixed-point; convert once to a compact GPU representation and retain the exact source quantization for comparison. Scale by a documented factor in the shader. PICA shader math uses non-IEEE-literal 24-bit floating formats; **pixel-perfect DS mathematics cannot be assumed**. Explicitly compare screen position, clipping, depth tests and near-plane crossing on representative objects.

Recommended experiments:

- 16-bit packed XYZ+UV against float XYZ+UV for identical map geometry.
- Batched same-material tiles and polygons against one draw call per object.
- Uniform transform update per object against preindexed matrices in shader, with actual `C3D_DrawArrays` count, command-buffer size and GPU duration.
- Compile-time fixed shaders for common materials rather than a large dynamic mega-shader.
- Triangulate quads with ordering that matches DS diagonal rules; compare problematic translucent geometry.

Batch only when overlapping/draw-order semantics are unaffected. A polygon's translucent behavior may depend on primitive order, ID and depth mode. Do **not** sort all transparent polygons by texture for speed.

## 5. Materials: explicit GPU approximation matrix

Every DS material mode requires an implementation disposition:

| Original effect | Candidate | Verification |
| --- | --- | --- |
| Opaque textured | TEV texture×vertex primary color | Texture/color/alpha match |
| Color-only geometry | Vertex color, no texture | Correct interpolation + rounding |
| Alpha test | GPU alpha test | Cutout thresholds and priority |
| Semi-transparent polygons | Hardware blending with ordered passes | Depth-write and ordering tests |
| Toon shading/highlight | PICA lookup/fixed-function pass or prepared vertex color | Face normals, light changes, color matching |
| Shadow polygons | Constrained stencil/depth setup | Occlusion/ID semantics; reject incomplete effects |
| Edge marking | Specialized additional pass only if frame-budgeted | IDs, width, corners, cost |
| Fog | PICA fog controls or equivalent GPU pass | Distance and opacity curves |
| DS W-buffer/depth quirks | Configured target/depth range and transform | Comparison at overlaps and clipping |

These are **candidate mappings**, not statements that the two GPUs have identical fixed-function features. For impossible/expensive exact matches, record an explicit reviewed deviation in [CORRECTNESS_MATRIX](CORRECTNESS_MATRIX.md), rather than silently suppressing the effect or invoking software rasterization. Normal gameplay must not be accepted while an important effect remains absent.

## 6. Native widescreen without asset invention

Original DS world view is 256×192 logical. Target top canvas is 320×192 logical (400×240 physical). Increase effective horizontal field of view by 25% at the same vertical FOV, or adjust the projection equivalently. The choice of preserving original camera vertical framing versus true camera ray semantics must be validated per map type.

This is **not** merely changing a matrix. Update *all* visibility gates upstream: room/object culling, portal bounds, model visibility, screen-space clipping, polygon-reject heuristics and any logic that assumes x in `0..255`. Track cases where original DS intentionally hid unseen geometry; expanded view can reveal unfinished scenery or scene boundaries.

Keep one layout policy per scene: widescreen 3D overworld; battles with reviewed camera/HUD framing; 2D cutscenes pillarboxed or authored; full-screen pictures not stretched. Geometry visible in the new side columns must have actual valid game data.

## 7. Scene building, visibility and streaming

At asset build time, determine whether map geometry can be partitioned by stable spatial bounds. Keep a small spatial index of conservative AABBs or cells; runtime culls by expanded frustum **before issuing materials/commands**, not by deleting game objects. This is an optimization for rendering only; it must not affect object/event simulation or collision.

Candidate layout:

- Static mesh pages per map chunk, loaded near the player and retained with GPU fences.
- Dynamic actors in a separate stream of transforms and animated sprite/mesh frames.
- Opaque material runs grouped only within correct ordering constraints.
- Delayed prefetch of likely next-map textures and polygons to avoid 30 FPS transition spikes.
- Never allocate an entire replacement scene while still keeping duplicate copies of every old resource unless the peak memory ledger permits it.

A 30 FPS frame does **not** require full scene rebuild; after warm-up it should mostly update camera, actor transforms, animation and relevant dynamic state.

## 8. Draw submission and command budget

Coffeepot [drawbench](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/benchmarks/README.md#drawbench) measured approximately 9.6–9.8 µs CPU per separately recorded citro3d draw on its test O3DS. Material, texture and uniform changes add cost. Therefore design coarse draws, avoid redundant `C3D_TexBind` and track command buffer occupancy. The same benchmark's batched 4,096 quads at ~0.21 ms is **CPU recording only**, not a real world GPU load.

The backend must publish:

- `g3_commands_decoded`, `g3_commands_reused`, `g3_vertices_streamed`, `g3_vertices_retained`.
- `gpu_draws`, `gpu_triangles`, `pipeline_rebinds`, `texture_rebinds`, `uniform_updates`, `cmd_buf_max_percent`.
- `cpu_geometry_ms`, `cpu_gpu_record_ms`, `gpu_time_ms`, `gpu_queue_wait_ms`, `render_target_switches`.
- `texture_upload_bytes`, `vertex_upload_bytes`, `resource_cache_hit_miss`, `gpu_generation_lag`.

Counters need a definition (time interval, work included, denominator), stable CSV fields and zero-overhead-off release versions. GPU timing is not equal to `C3D_FrameEnd` wall time.

## 9. G3 acceptance suite

| ID | Scene/test | Gate |
| --- | --- | --- |
| G3D-001 | Captured static field G3 stream | Native 400×240 PICA rendering, no source-fidelity regressions in center crop |
| G3D-002 | Moving camera through Goldenrod | Side visibility correct; stable retained geometry with no unnecessary rebuild |
| G3D-003 | Character animations + followers | Correct transform, palette, depth and game timing |
| G3D-004 | Transparent world objects | Correct order and alpha/depth, no sorting regression |
| G3D-005 | Battles + animations | Essential polygons, materials, UI overlap, near clipping correct |
| G3D-006 | Worst-case scene geometry | Within **allocated** CPU/GPU frame budgets on O3DS; p99 and missed deadlines logged |
| G3D-007 | Source display-list changes in place | Invalidation before GPU use; no stale frames |
| G3D-008 | Depth/fog/edge special modes | Mode-specific visual tests; approved deviations recorded |
| G3D-009 | 30-minute play loop | No upload stall, buffer overwrite, resource leak or progressive frame-time drift |

**Exit from M1:** native-resolution 3D replay working on O3DS, not yet a proof of full-game sustained 30 FPS. **Exit from later integrated milestone:** Goldenrod and battle stress scenes pass with full game execution and audio.

## 10. Relevant sources

- Coffeepot local `ctr\engineering\PICA200\gpu-rendering.md`, `ctr\engineering\PICA200\renderer-architecture.md`, `ctr\engineering\PICA200\pica200-command-pipeline.md`, `ctr\engineering\porting.md` below `C:\Users\terezi\workdir\coffeepot`.
- [Coffeepot GPU rendering](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/gpu-rendering.md), [renderer architecture](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/renderer-architecture.md), [fixed-function API](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/citro3d-fixed-function-api.md).
- [Existing hgss-3ds source and state](https://github.com/estyseesghosts/hgss-3ds), [Platinum port](https://github.com/GabeSud0/pokeplatinum-3ds), [SM64DS Enhanced](https://github.com/ethanseagull/sm64ds-enhanced), [3dbrew GPU](https://www.3dbrew.org/wiki/GPU).
