# GPU_RESOURCES — memory ownership, coherency, generations and uploads

**Status:** foundational contract; implement before asynchronous rendering. Related: [GPU_2D](GPU_2D.md), [GPU_3D](GPU_3D.md), [FRAME_PIPELINE](FRAME_PIPELINE.md), [ASSET_BUILDER](ASSET_BUILDER.md).

## 1. Architecture and constraints

Original 3DS application memory is approximately 64 MiB in common homebrew configurations; VRAM is 6 MiB total in two banks, with targets and textures sharing the budget. Check actual process memory mode/build configuration at runtime and fail explicitly if a request exceeds the measured budget. **The new 3DS's larger memory must not mask O3DS failures.**

Reference: Coffeepot [memory and cache](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/memory-and-cache.md), [hardware memory](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/hardware/memory.md), [GPU renderer architecture](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/renderer-architecture.md).

**Suggested memory pools**:

| Pool | Purpose | Critical restriction |
| --- | --- | --- |
| Ordinary CPU heap | Game state, audio metadata, small decoded CPU-only assets, dependency tables | Do not pass ordinary pointers directly to GPU unless proven GPU-addressable |
| Linear heap | GPU-visible vertex/index buffers, dynamic texture staging, command buffers, DSP audio buffers | Explicit flush before CPU-produced bytes are consumed by devices |
| VRAM | Render targets, depth and selected heavily used resident textures | Homebrew CPU usually cannot write these as normal writable RAM; upload by supported GPU transfers |
| SD/ROMFS | Compressed user-generated asset packages and low-frequency bulk data | Avoid synchronous tiny-file operations in active frame |

## 2. Start with a complete ledger

**Nominal raw capacities** (no padding/alignment/extra transfers):

- 400×240 RGBA8 color = **384,000 B** (~375 KiB).
- 400×240 RGB565 color = **192,000 B** (~187.5 KiB).
- 400×240 D24S8 = **384,000 B**; D16 = **192,000 B**.
- 320×240 RGBA8 color = **307,200 B** (~300 KiB); RGB565 = **153,600 B**.
- 512² RGBA8 = 1 MiB; 16-bit texture = 512 KiB; A8 = 256 KiB; ETC1 = 128 KiB (not including compression alpha or mip levels).

Create a **VRAM bank-level budget**, not just a sum of image payloads. Include buffer alignment, driver-owned targets, depth/stencil, per-eye assumptions, render-to-texture surfaces, scratch transfers and whether textures can truly occupy the space left over.

Create a **CPU memory budget** with code/data/BSS, game heaps, original overlay transient cost, source ROM/cache blocks, audio, atlas metadata, linear heaps, staging rings, diagnostic storage and peak map transitions. Budget both `steady` and `old scene + new scene + decode scratch` peaks. Surface a memory ledger in the startup log.

## 3. Explicit resource state machine

Every GPU resource must have a generation and a known owner. Proposed states:

```text
Unloaded → IO_PENDING → CPU_DECODED → STAGING_WRITABLE →
GPU_UPLOAD_QUEUED → GPU_RESIDENT → IN_USE_BY_FRAME → RETIRE_PENDING → FREE
```

Allowed shortcuts are deliberate and documented (e.g., GPU resident target reused across frames). For an update, produce a **new generation** if the current one is still sampled or referenced by an in-flight command buffer.

Each allocation record should contain: resource ID, content/dependency hash, format, width/height/stride, owner, pool, alignment, bytes reserved, generation, first/last frame IDs, GPU queue/fence token, and reverse dependencies. Resource IDs are never identical to raw pointers; IDs may be reused only after generation change and retirement.

No per-frame uncontrolled malloc/free. Prefer bounded allocators for command lists and per-frame dynamic geometry, with capacity counters and an explicit error if capacity is exceeded.

## 4. Immutable source snapshots

A renderer frame must not sample a different VRAM generation at two unrelated points. The old `hgss-3ds` found a signature race where a hash taken after rendering described newer live VRAM than the picture actually drawn. A strong rule is **capture the signature no later than the draw**, but an even better rule is to bind each command to an immutable source snapshot or generation.

For DS-visible graphics state capture:

1. Collect DS register/OAM/palette/VRAM page generation IDs during the game frame under well-defined synchronization.
2. Finalize a `FrameGraphicsSnapshot` with immutable owned bytes or explicitly pinned pages; no live source pointers may outlive the capture boundary.
3. Build dirty/invalidation keys from **data that the renderer actually consumes**, not volatile timing registers. VCOUNT/DISPSTAT affect emulation but are not image signatures unless a rendering operation depends on them.
4. Update pages only after the earlier pinned version's consumers retire; use copy-on-write or page staging when necessary.
5. Verify an optimization with reference first/last hashes **within the same generation**, plus rendered output comparisons.

A dirty bit alone is insufficient when palette dependencies or VRAM bank remapping change the interpretation of unchanged tile bytes.

## 5. ARM11 data-cache coherence

On O3DS the CPU cache is write-back. CPU writes to linear buffers may not be visible to GPU/DSP until flushed; devices writing memory require appropriate invalidation before CPU reads. The exact libctru/citro3d semantics must be checked for the version linked.

Coffeepot's measurements (launch O3DS, November 2026) report `C3D_FrameEnd(0)` ~147–149 µs versus `C3D_FrameEnd(GX_CMDLIST_FLUSH)` ~57–60 µs for its empty scene; the difference reflects the default whole-linear-heap flush. That is only ~0.09 ms, so **start with the safe default**, even if targeted flush sounds more efficient.

Optimized flushes are allowed only when all GPU-visible writes are recorded. `C3D_FrameEnd(GX_CMDLIST_FLUSH)` bypasses citro3d's automatic whole-linear-heap flush, and then every dirty vertex/index/texture/staging range must have been made visible *before submission*. Missing one range produces intermittent old geometry/texels. Validate using deliberately dirtied nonadjacent buffers and randomized frame timing. Do not treat shaders/uniform writes in the command stream as ordinary externally allocated vertex-buffer writes.

DSP audio buffers require their own documented device-coherency lifecycle.

## 6. GPU asynchronous ownership and rings

Recommended (not mandatory) baseline is a small fixed number of GPU frame contexts with per-context command and dynamic upload storage. Before reusing one, prove completion of all commands that reference it. Do not infer completion merely because `C3D_FrameEnd` returned; that queues GPU work, not guaranteed end-of-render.

A ring entry has:

- Frame ID + generation, start/end allocator positions and GPU completion token.
- Immutable references to every texture, glyph atlas, mesh and render target used by that frame.
- Data-cache flush state and byte ranges.
- Explicit consumer count for shared resources and display capture outputs.

If the GPU falls behind, record a deadline/queue wait. Do not overwrite in-flight memory to preserve apparent FPS. An emergency allocator that silently grows on O3DS is not an acceptable failure mode.

## 7. Atlas invalidation and uploads

For each atlas page retain `(asset pack generation, tile/glyph source generation, palette generation, upload generation)`. A page can be reused only when the full signature matches and its GPU contents are coherent.

- Coalesce adjacent or repeated dirty tile updates within an atlas page; preserve the exact visible order of palette and scene changes.
- Favor offline 3DS swizzled texture pages for static assets; use runtime swizzling only for truly dynamic graphics.
- VRAM upload via GPU transfer is not an unconditional win: measure driver/setup, queue latency and scratch memory against a linear-resident texture.
- Most content is not allowed to mutate while resident/in-flight: stage a new page and atomically replace the handle at a frame boundary.
- Cache textures by actual bytes/format/palette, not just file ID. File IDs can be reused across scenes or ROM versions.
- Prefer nearest sampling for sharp art; verify alpha edges and padding. For minified 3D textures, compare mipmaps, memory and visual result.

## 8. Memory pressure strategy (no software fallback)

When memory pressure appears:

1. Evict GPU resources that are unreferenced and have completed all users.
2. Lower cache capacity and prefetch distance for **future** scenes, without changing visible rendering fidelity.
3. Reduce unnecessary double residency at scene transitions through ordered unload/load.
4. Reduce only optional postprocessing, stereo or N3DS extras first (never a required core visual effect).
5. Fail the development gate if required O3DS graphics cannot fit. Do not switch to CPU rasterization, stream screenshots, silently omit polygons or enable compression that damages required art without approval.

## 9. Memory and lifetime gates

| ID | Test | Pass |
| --- | --- | --- |
| RES-001 | Memory ledger boot + steady + transitions | No pool exceeds actual O3DS limits, including temporary peaks |
| RES-002 | CPU writes before GPU reads | No stale data with randomized flush/queue timing |
| RES-003 | Reuse atlas while previous frame in flight | No overwritten glyph/tile or flicker |
| RES-004 | Palette and VRAM remapping | All dependent resources invalidated; unrelated ones retained |
| RES-005 | Rapid map and menu transitions | Stable bounded allocation, no leaks or deferred-free backlog |
| RES-006 | Whole-heap vs tracked flush A/B | Correctness identical; optimization retained only for end-to-end benefit |
| RES-007 | Static scene 10 minutes | No texture reupload/mesh regeneration beyond legitimate animation |
| RES-008 | VCOUNT/DISPSTAT graphics-signature regression | Volatile timing changes do not invalidate a flat unchanged picture |
| RES-009 | GPU stalls | Never reuse or free in-flight resources; queue wait is counted |

Use one CSV row per frame and separate allocation event logs. Capture at least max, p95, p99, failed allocs, total bytes uploaded and GPU queue lag.

## 10. Sources

- Local: `C:\Users\terezi\workdir\coffeepot\ctr\engineering\memory-and-cache.md`, `ctr\engineering\PICA200\renderer-architecture.md`, `ctr\benchmarks\README.md`.
- [Coffeepot measured cache/flush costs](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/memory-and-cache.md#measured-costs-old-3ds), [benchmark suite](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/benchmarks/README.md), [citro3d renderqueue.c](https://github.com/devkitPro/citro3d/blob/master/source/renderqueue.c).
- [Existing HGSS source State/known hazards](https://github.com/estyseesghosts/hgss-3ds/blob/main/docs/progress/STATE.md), [3dbrew GPU](https://www.3dbrew.org/wiki/GPU).
