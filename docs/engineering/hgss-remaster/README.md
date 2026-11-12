# HGSS 3DS remaster — engineering document set

**Status:** implementation design; not a claim that the proposed renderer is implemented or fast yet.  
**Target:** original launch-model 3DS / original 2DS, 30 game updates and approximately 30 distinct presented frames per second, 400×240 upper screen and 320×240 lower screen, GPU-native rendering with **zero runtime software rasterization fallback**.  
**Prepared:** 2026-11-12.

## Where these files go

This ZIP is rooted at the repository root. Extract it **into the new HGSS remaster repository**, retaining `docs/engineering/hgss-remaster/`.

The implementer has a separate, local copy of Coffeepot at:

`C:\Users\terezi\workdir\coffeepot`

When a document refers to `coffeepot/ctr/...`, first look for that path below the local root above. The Markdown links also point to the public repository so the documents remain useful elsewhere. **Do not copy a Coffeepot snippet without checking its local revision, build assumptions, and verification status.**

## Reading order and contracts

| File | Responsibility | Primary verification |
| --- | --- | --- |
| [FONT_RENDERER.md](FONT_RENDERER.md) | Preserve HGSS text semantics while rendering native crisp glyphs | `FNT-*` |
| [GPU_2D.md](GPU_2D.md) | Tile BGs, OAM, windows, blend, scanlines and captures on GPU | `G2D-*` |
| [GPU_3D.md](GPU_3D.md) | Decode G3 streams, retain meshes, map materials and use widescreen | `G3D-*` |
| [GPU_RESOURCES.md](GPU_RESOURCES.md) | Resource ownership, coherency, generation and memory budgets | `RES-*` |
| [FRAME_PIPELINE.md](FRAME_PIPELINE.md) | Game clock, VBlank, frame submission, scheduling, sound | `FRM-*` |
| [ASSET_BUILDER.md](ASSET_BUILDER.md) | Reproducible extraction and offline conversion from user-owned ROM | `AST-*` |
| [HARDWARE_OPTIMIZATIONS.md](HARDWARE_OPTIMIZATIONS.md) | Evidence-ranked CPU/GPU optimization hypotheses and tests | `OPT-*` |
| [CORRECTNESS_MATRIX.md](CORRECTNESS_MATRIX.md) | End-to-end scenes, assertions, regression gates, acceptance | `COR-*` |

## Unnegotiable engineering rules

1. **O3DS is the baseline; N3DS is never a milestone or the assumed performance environment.** Only measured O3DS results permit a milestone to pass. Azahar is a correctness/debugging aid, not a timing authority.
2. **No CPU final-pixel renderer in the release executable.** CPU decoding, format conversion at load time, command translation and game-state updates are permissible; normal per-frame CPU scene rasterization, whole-frame readback and hidden fallback are not.
3. **No feature is accepted based on average FPS.** Test frame pacing, 1%/0.1% tails, long runs, memory peaks and audio underruns. Report limits and scene duration, not just a headline number.
4. **Preserve the original game loop and DS-visible behavior.** 30 FPS means approximately one complete game update and one new picture for every two 3DS display refreshes; do not double simulation or add interpolation in this iteration. Use a measured pacing strategy rather than layering competing 30 Hz sleeps.
5. **Frame input is immutable after the frame boundary.** Reads from emulated VRAM, tilemaps, OAM and graphics registers must be coherent with the version the game published. No hash may be taken from a later live generation than the frame being rendered.
6. **Correctness before caching.** Every fast path needs an admission test, output check and invalidation proof; unsupported effects trigger conspicuous development errors, never a runtime CPU-rendering escape hatch.
7. **Unclear claims are experiments, not design facts.** Hardware data from Coffeepot is identified as measured *for that benchmark*, not transplanted into an HGSS performance promise.
8. **Keep the working original project as a reference.** Import small proven compatibility components and tests only after examining ownership, licensing and hidden renderer dependencies.

## Expected repository interfaces

The suggested boundaries are **design interfaces**, not existing C symbols:

- `GameFrame`: immutable tick identifier, mapped graphics state, VRAM/OAM/palette generations, audio timestamp.
- `RenderPlan`: ordered logical layers and required effects without drawing them.
- `GpuResources`: resident tiles, meshes, glyph atlases, targets, staging rings and retire fences.
- `GraphicsBackend`: record/submit PICA commands and publish a completed target.
- `PerfRecord`: per-stage CPU/GPU cost, freshness, upload bytes, draw/state changes, queue waits and deadline results.

Do not introduce a generic interpreter into every hot draw call just because these contracts are defined in one place; the hot paths should be small and specialised.

## Ground truth and source priority

1. Original HGSS source/decompilation and verified game captures.
2. libctru/citro2d/citro3d headers and source at the **version actually linked**.
3. Coffeepot code + benchmark CSV with platform metadata.
4. 3dbrew, GBATEK and independently corroborated hardware architecture sources.
5. Other 3DS ports as examples; measurements from other ports are not HGSS guarantees.

**Source repositories:**

- [pret/pokeheartgold](https://github.com/pret/pokeheartgold) — original text/game flow (branch and revision must be locked).
- [estyseesghosts/hgss-3ds](https://github.com/estyseesghosts/hgss-3ds) — working compatibility, instrumentation and failure history.
- [GabeSud0/pokeplatinum-3ds](https://github.com/GabeSud0/pokeplatinum-3ds) — composition/remaster reference.
- [unowncollectables/Pokemon-Soulsilver-New3ds](https://github.com/unowncollectables/Pokemon-Soulsilver-New3ds) — alternative HGSS port and graphics pitfalls.
- [estyseesghosts/coffeepot](https://github.com/estyseesghosts/coffeepot/tree/main/ctr) — locally available CTR engineering handbook, hardware notes and experiments.
- [devkitPro/citro3d](https://github.com/devkitPro/citro3d), [devkitPro/citro2d](https://github.com/devkitPro/citro2d), [devkitPro/libctru](https://github.com/devkitPro/libctru).
- [3dbrew](https://www.3dbrew.org/wiki/Main_Page) and [GBATEK](https://problemkaputt.de/gbatek.htm).

### Benchmark evidence: read literally

Coffeepot's [microbenchmark suite](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/benchmarks/README.md) reports, on its test launch-model O3DS, ~9.6–9.8 µs **CPU recording cost** per separate citro3d draw and ~0.21 ms for a **batched** 4,096-quad recording workload. Its synthetic tile renderer measured distinct strategies. These are not promises about HGSS GPU raster time. Read the setup and CSVs before using any number in an engineering decision.

## Required first commit in the new repository

1. Record upstream git SHAs and devkitPro/lib versions in `upstream.lock` or an equivalent version manifest. The above links currently follow mutable branches.
2. Implement an **offline, deterministic reference-frame and input-capture format**. Each capture must identify ROM region/version, engine source SHA, map/scene, emulated graphics state, graphics resource generations and expected output.
3. Set up O3DS-specific build and benchmark targets, with explicitly marked Azahar smoke tests.
4. Build the stand-alone font and GPU 2D/3D replay prototypes before integrating the full game runtime.
5. Create a non-linkable offline software reference renderer for comparisons; the retail/homebrew 3DS runtime must not include its object files.

## Delivery and licensing

The build tools must take assets from the user's legally obtained ROM/dump. Do not ship Nintendo ROM assets, reconstructed fonts, game graphics, proprietary SDK files, bundled firmware data or saves in the repository. User-built outputs, including 3DSX/CIA routes if later implemented, should be packaged locally. Review upstream code/data licenses separately before moving any source.

## Document maintenance

Each major claim is labeled **documented**, **measured**, **proposed** or **hypothesis**. When implementers discover a contradiction, change the text and link the failing test; do not keep an outdated explanation simply because the build seems to run.
