## **What we're missing: we're reproducing the DS graphics pipeline too expensively**

The original DS could render Goldenrod because it had dedicated hardware for geometry processing, lighting, clipping and rasterization. Kohjoh currently recreates much of that pipeline on the Old 3DS ARM11 before handing the results to PICA200.

Ho-Oh and Lugia running smoothly proves the renderer can handle demanding animated models. It doesn't establish that the engine can efficiently process Goldenrod's much larger number of objects, state changes and draw submissions.

Three findings stand out.

### **1. We're still submitting enormous amounts of invisible geometry**

Kohjoh already supports the original game's `BOX_TEST` visibility checks. In steady Goldenrod:

- 67 box tests per captured scene.
- 56 boxes rejected by the game's own visibility logic.
- Yet Kohjoh still constructs approximately 124 batches containing 8,370 triangles.

The repository's offline visibility analysis found:

**Triangles entirely outside the view**

## **89%**

7,451 offscreen

919 remaining

Furthermore, 72 of the 124 batches are wholly outside a common clipping plane, representing 3,720 triangles.

**Currently, Kohjoh doesn't perform that additional batch-level culling.** [`g3d.c`](https://github.com/estyseesghosts/kohjoh/blob/main/src/graphics/g3d/g3d.c?utm_source=chatgpt.com) simply iterates through the batches and issues draws. PICA ultimately handles clipping.

However, adding a draw-time culling check alone won't solve Goldenrod: the ARM11 will already have spent time decoding the geometry. We need to avoid producing invisible geometry earlier wherever doing so preserves the game's behaviour.

### **2. The original DS discarded invisible polygons before rasterization**

This is a more important architectural difference than the existing box tests.

Nintendo's DS geometry engine performed vertex transforms, lighting, backface culling and clipping in dedicated hardware. Fully clipped polygons did not consume polygon-list storage. Its normal scene limits were 2,048 polygons and 6,144 vertices, excluding rejected geometry.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub+2



Kohjoh currently builds triangles on the ARM11 and passes them to PICA, which performs the final clipping later.

That means Goldenrod's approximately 8,370 submitted triangles aren't evidence the original DS rendered 8,370 visible triangles. The port is carrying substantial invisible work through its CPU renderer and GPU submission stages.

**This is likely one of the biggest architectural gaps remaining.**

### **3. GPU lighting is expensive because our shader does too much per vertex**

I inspected [`g3d_lit.v.pica`](https://github.com/estyseesghosts/kohjoh/blob/main/src/graphics/pica/shaders/g3d_lit.v.pica?utm_source=chatgpt.com).

It's a 409-instruction vertex shader that reproduces DS lighting arithmetic, including exact fixed-point rounding and specular calculations.

The expensive parts are:

- **All four light calculations are present**, even when fewer lights are enabled.
- Fixed-point calculations require extra instructions to preserve the DS's results on PICA's limited floating-point precision.
- Shininess-table lookup and specular arithmetic add substantial work.
- Lit batches support only six matrices, potentially producing more draw calls.
- Uploading changed lighting contexts and shininess tables also costs ARM11 time.

The hardware evidence is substantial:

| GPU processing    | Previous | Latest   |
| ----------------- | -------- | -------- |
| P3D time/picture  | 2.63 ms  | 11.31 ms |
| Draws/picture     | \~124    | \~134    |
| Triangles/picture | 8,370    | 8,370    |

That's approximately **4.3 times the GPU execution time** for the same triangle count. The shader is a strong explanation, although the comparison also includes other changes.

But here's the distinction: **the GPU isn't currently preventing 30 FPS.** It completes its work in roughly 12.4 ms, and the CPU almost never waits for it.

The CPU is the immediate problem.

### **4. Our geometry cache doesn't avoid enough geometry processing**

Look at [`vertex_plan()`](https://github.com/estyseesghosts/kohjoh/blob/main/src/runtime/kj_g3.c?utm_source=chatgpt.com#L2998-L3059).

It caches expanded vertex coordinates, but replays the command stream, executes state changes and reconstructs the draw geometry.

The latest logs show geometry replay costs increasing markedly, despite eliminating almost all vertex-copying time.

This is likely why moving lighting to the GPU hasn't translated into better Goldenrod performance: **we transferred lighting arithmetic but introduced a more expensive geometry replay path.**

For static scenery, we should ideally decode geometry once, retain GPU-ready vertices and indices, and subsequently update only the relevant transforms, materials and lighting. Animated objects can use a separate path when necessary.

## **Why are we skipping frames?**

The code intentionally skips a picture when the game is at least two refreshes behind, and prevents two consecutive skips.

This is a fallback to keep game simulation and audio progressing when rendering exceeds the deadline. It isn't a substitute for optimization.

The latest Goldenrod profile illustrates the problem:

Captured picture CPU work versus 30 FPS budget

Game logic, median

**57.4 ms**

30 FPS deadline

**33.4 ms**

The captured frame also requires roughly 10 ms of publish work. These are profiled wall times, not an additive hardware execution trace.

Simply disabling skips wouldn't achieve 30 FPS. It would make the game fall further behind or slow down.

**The goal should absolutely be sustained 30 FPS without routine skipping.** Keep skipping as emergency protection for occasional spikes, not normal Goldenrod gameplay.

## **What I would change about our optimization strategy**

I would stop treating this primarily as a question of making the existing decoder incrementally faster.

**First, establish why the new lighting/geometry path increased captured-frame logic time from approximately 36 ms to 57 ms.** Compare the same revision with CPU lighting enabled and disabled, keeping everything else fixed. Profile command replay, vertex reconstruction and normal processing separately.

**Second, add conservative culling before geometry construction.** Use cached per-model or per-submesh bounds at the producer level, before traversing display lists. Keep the existing `BOX_TEST` behaviour intact; don't skip game-visible state changes. The 72 entirely offscreen batches are an obvious starting point, although the largest gains require avoiding their CPU processing, not merely their draw calls.

**Third, retain complete geometry instead of repeatedly expanding it.** Static Goldenrod scenery should be a particularly good candidate. We can preserve the DS command semantics while avoiding redundant vertex and index generation.

**Fourth, specialize GPU lighting.** Measure how many lights each Goldenrod batch actually uses. Build cheaper shader variants for the common cases, retaining the exact four-light implementation as a fallback.

Then benchmark Goldenrod with routine frame skipping disabled. The acceptance criterion is every captured game update completing within the 33.4 ms deadline, including rendering, with enough headroom for audio and occasional spikes.

One additional correctness concern worth testing: the DS's 2,048-polygon limit counts hardware polygons after clipping and culling, whereas Kohjoh counts PICA triangles. Those totals aren't directly comparable. We should verify that our retained scenes match the DS's visible geometry rather than merely rendering every decoded primitive.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://ndsdoc.gbadev.net\&sz=32)

ndsdoc+1



**My conclusion:** Ho-Oh and Lugia show that the PICA rendering approach works. Goldenrod exposes where the port still behaves like a software-emulated DS geometry engine instead of a native 3DS renderer. The next gains should come from eliminating unnecessary geometry processing, not more aggressive frame skipping.
