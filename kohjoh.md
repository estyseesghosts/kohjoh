# HeartGold / SoulSilver Native 3DS Remaster
## Final implementation plan — O3DS-first, GPU-native, two installation formats

**Status:** Engineering specification and execution plan, 12 November 2026  
**Minimum hardware:** Original 2011 Nintendo 3DS (and original 2DS), at ordinary O3DS clocks and memory allocation  
**Target:** One new game-derived frame per two display refreshes (~29.9 FPS on the original refresh clock), sustained, with original HGSS simulation timing  
**Primary validation during implementation:** Azahar, with hardware-specific claims explicitly deferred  
**Output:** Shared native game/runtime; SD-backed `.3dsx`; user-built self-contained game-data `.cia` with 3DS title save archive

---

## 0. Instructions to the implementer

Implement this as a **new project**, not a rewrite of `hgss-3ds` in place. Inspect and pin the three reference projects, the original HGSS decompilation, Coffeepot CTR documentation, and the additional documents supplied by the project owner. Reuse **verified, minimal** compatibility logic and test fixtures, not the previous renderer's architecture. Maintain source provenance and licenses.

Proceed in **one sustained implementation pass**, carrying work as far as the available build environment and Azahar allow. Do not stop after writing architecture documents or an isolated triangle demo. Implement, compile, test, diagnose and continue through successive integration milestones. Do not call emulator performance an O3DS benchmark. Never replace a requested feature with a CPU software renderer to make a demo appear finished. Do not wait for an additional approval between milestones; leave a precise evidence-based blocker when a necessary input or hardware test is unavailable.

At every stage, report **implemented / built / run / verified**, separately. A source file, successful compilation, emulator boot and O3DS performance certification are four different facts.

### Fixed decisions

1. **O3DS is the design floor**. No N3DS-only threads, speedup, L2 or extended-memory assumptions in the baseline.
2. **30 FPS, not 60 FPS.** No interpolation work, extra presentation frames or duplicated simulation updates. Preserve original DS VBlank/interrupt and game-tick semantics; synchronize presentation deliberately.
3. **GPU-native runtime.** No CPU software rasterizer, full-frame CPU compositor, routine GPU readback, or transparent software fallback in the shipping 3DS application. Offline reference rendering on the host is permitted and encouraged.
4. **Real remaster framing.** Render 3D directly at 400×240 top and GPU-render the 320×240 bottom. The initial world-space design is a **320×192 logical widescreen window** (256×192 original plus 32 logical pixels on each side) mapped to 400×240 physical. That preserves vertical framing while showing 25% more horizontal world. Verify both camera frustum and game-level object culling. Do not use 800×240 wide mode as the baseline.
5. **Crisp native text.** Keep original text/control-code behavior; emit retained GPU glyphs from original HGSS font resources. Do not upscale a completed 256×192 text picture.
6. **One shared executable codebase and one deterministic user-owned data pack**. The packaging target selects asset and storage mounts; it must not fork gameplay or rendering behavior.
7. **Both editions:** HeartGold and SoulSilver through one engine, edition-specific assets and independent save identities.
8. **Scope exclusions for baseline:** stereo, 60 FPS interpolation, autosave, expensive postprocessing, enhanced edge marking, network/wireless and N3DS-only cosmetic effects. They may have design hooks; do not delay the core port for them.

## 1. Correctness and performance acceptance

### Objective performance contract

- On a real 2011 O3DS at baseline clock and memory configuration, the game completes each ordinary gameplay frame before its next two-refresh deadline (roughly 33.4 ms). Target **≤25 ms active critical-path work** where practical, leaving headroom. CPU and GPU times can overlap: do not sum overlapping durations as if serial.
- A qualifying 10-minute steady-state scene run shows **zero missed 30 FPS presentation deadlines after warm-up**, with separately reported p50, p95, p99, maximum, GPU busy, CPU work, CPU wait and service stalls. Repeat across the worst-case test corpus, then expand to long play sessions. Transitions and loading are accounted for separately; they must not corrupt simulation, audio or saves.
- Original gameplay pace, scripts, battle rules, random behavior, audio sequencing and saves remain correct; only intentional graphics/layout remaster changes may differ.
- Normal-frame graphics require **zero CPU pixel rasterization, zero full-frame CPU uploads and zero GPU-to-CPU picture readbacks**. Assert and count violations in debug; enforce absent symbols/backends in release builds.
- Hardware memory peaks, command-buffer use and texture residency have defined ceilings. No normal-frame heap allocation, unbounded queue or silent object-drop behavior.

### What Azahar can and cannot establish

Azahar is the primary fast integration debugger: startup, dual screens, scene appearance, game correctness, GPU state, save mount logic, CIA install/boot, basic pacing, and reproducible scripted sequences. Keep an exact Azahar version/build identifier, renderer backend, settings and input script with every result; use native output scale and disable optional graphics hacks for reference tests.

**Azahar cannot certify O3DS timing, instruction-cache behavior, PICA/GSP synchronization, service-core contention, real SD stalls or VRAM hazards.** Its O3DS profile is not a substitute for a 2011 console. Final report must explicitly separate `AZAHAR_PASS`, `O3DS_UNTESTED` and `O3DS_PASS`. If hardware is unavailable during the one-shot pass, deliver the strongest emulator-verified build and a ready-to-run O3DS validation kit; do not label it locked 30 FPS on hardware.

## 2. Repositories, dependencies and provenance

Inspect and pin exact revisions before adapting anything:

- `https://github.com/estyseesghosts/hgss-3ds` — working HGSS/Nitro integration; patches for saves, sound, overlays, timers, startup; scene captures and performance failures. **Do not import its software 2D/composition pipeline as the runtime renderer.**
- `https://github.com/unowncollectables/Pokemon-Soulsilver-New3ds` — HGSS behavior, assets, PSP-derived port work, GPU readback/composition compromises and scene tests. **Do not inherit the original GPU→CPU image resolve.**
- `https://github.com/GabeSud0/pokeplatinum-3ds` — direct game-source port, PICA composition, camera/widescreen/UI and GPU materials. **Do not import interpolation or routine CPU 2D.**
- `https://github.com/pret/pokeheartgold` — game logic and text/font/window semantics. Pin revision, record supported ROM revisions and both edition layouts.
- `https://github.com/estyseesghosts/coffeepot` — CTR hardware, PICA200, asset pipeline, frame pacing, save archives, cache ownership and hardware benchmark methods.
- `https://github.com/devkitPro/libctru`, `https://github.com/devkitPro/citro3d`, `https://github.com/devkitPro/citro2d`, `https://github.com/devkitPro/3ds-examples` — current API and implementation references.

Use upstream code only where licenses and required attributions permit. Keep local changes as reviewable patches. Record toolchain and dependency hashes in a lockfile. Any report about prior performance must name its exact scene/build/model; do not combine unmatched runs.

## 3. Repository layout and module contracts

```text
hgss-remaster/
  AGENTS.md
  README.md
  upstream.lock
  CMakeLists.txt / Makefile / make rules (one pinned build path)
  game/                    # pinned game source/submodule or reproducible patches
  patches/{game,nitro}/
  src/
    app/                   # APT lifecycle, input and bootstrap
    compat/                # DS SDK, overlays, VRAM, DMA, clocks, interrupts
    graphics/
      state/               # coherent GPU-facing snapshot and change generations
      g2d/                 # BG, OBJ, windows, masks, scanline effects
      g3d/                 # DS command translation, geometry, materials, culling
      font/                # HGSS font atlas and glyph command backend
      pica/                # citro3d, vertex shaders, TEV, submission, targets
      resources/           # GPU allocations, caches, residency and fences
      layout/              # wide camera, UI anchoring and touch transforms
    audio/                 # DS sound translation, ndsp/CSND service
    content/               # indexed readonly game data interface
    storage/               # shared save protocol, .3dsx and CIA adapters
    diagnostics/           # counters, worst-frame ring, dumps
  tools/
    rompack/               # user-ROM verifier, extract/convert/pack
    capture/               # record/replay DS state and input
    image-diff/            # reference image comparisons
    build/                 # 3dsx and CIA orchestrator
    azahar/                # launch, scripted smoke tests, result collection
  tests/{unit,reference,render,save,integration,hardware}/
  docs/{architecture,fonts,2d,3d,assets,save,packaging,performance,worklog}/
  dist/                    # ignored user build artifacts only
```

**Mandatory boundaries**:

- `GameRuntime` runs the original HGSS state machine and calls Nitro-compatible services. It does not own PICA GPU handles or know about 3DS save paths.
- `GraphicsCapture` consumes original G2/G3 operations and produces coherent immutable **frame/scene commands**. All CPU/DMA VRAM writes, palette/OBJ changes, bank mapping, register effects and display-capture dependencies must be tracked. Do not hash live mutable memory and draw later from another version.
- `RenderBackend` translates those commands into PICA200 resources and ordered draws. It owns GPU synchronization and can detect unsupported operations, but **cannot select a software rasterizer**.
- `AssetProvider` supplies stable logical game file IDs over a readonly indexed pack. Neither game code nor renderer knows whether this pack is on SD or inside RomFS.
- `SaveStore` reads/writes **original-format HGSS save payloads**, with a transaction wrapper and backend-specific commitment. No mode switch inside game save serialization.
- `Presentation` handles frame pacing, top/bottom invalidation and APT restore, without advancing game ticks or holding a render lock through IO.

No monolithic GPU compatibility file, ad hoc global state or multiple competing frame loops. Reserve explicit ownership per subsystem in `AGENTS.md`.

## 4. Dual installation and storage contract — implement early

### 4.1 Common user-ROM asset pack

A host-only `rompack` tool takes the user's original HG/SS `.nds` and produces a versioned, indexed **`game.pak`**. This contains the game data required by the native engine (original resources and preconverted GPU assets), not an NDS emulator or second executable. Determine support by verified ROM hashes, game code, region, edition and extracted content layout. Unknown ROM revisions fail with an actionable message instead of silently interpreting incorrect offsets.

The pack contains a manifest: format version, edition, region, source ROM SHA-256, asset builder commit, conversion version, entry-table checksums, source-to-native IDs and compatibility requirements. Use deterministic ordering, reproducible output and per-entry integrity checks. Avoid hundreds of tiny filesystem opens. Store compressed or raw entries based on measured decode+IO cost, with lazy loading and bounded cache. Never assume the full original ROM or all converted textures can fit in the O3DS heap. No Nintendo ROM data, copyrighted fonts, artwork or fully prepared packs in public source distributions or CI logs/artifacts.

Two products use the **same generated pack bytes**; compatibility and renderer output should not depend on installation mode.

### 4.2 Homebrew Launcher `.3dsx`

Distribution/installation directory:

```text
sdmc:/3ds/hgss-remaster/
  hgss-remaster.3dsx
  hgss-remaster.smdh
  data/
    heartgold.pak           # produced by the user, or soulsilver.pak
    soulsilver.pak          # optional second edition
  config.ini
  saves/
    heartgold_a.bin
    heartgold_b.bin
    soulsilver_a.bin
    soulsilver_b.bin
  logs/                     # optional; never synchronous in a frame
```

- The `.3dsx` itself contains no user ROM data; install the user's pack alongside it. If both packs exist, show a lightweight edition selector before boot. A missing pack produces a clear diagnostic, not an empty or fake game.
- Read game data from `sdmc:/3ds/hgss-remaster/data/`; save and config also live **only** in this application folder. Use a single path root, not the current working directory, to prevent Homebrew Launcher launch-path differences.
- Do not assume installed-title save services are available to `.3dsx`. Use ordinary `sdmc:` files with checked reads, writes, flushes and close results.
- Test launching from Homebrew Menu, returning to it, suspending/resuming and missing/malformed directories. Ensure directory creation doesn't silently fail.

### 4.3 Self-contained user-built `.cia`

- The build tool takes **one edition at a time initially**, stages the same `game.pak` inside the CIA's RomFS and builds a **native installed title from the ELF and a validated RSF/exheader**, using a pinned `makerom` tool. Include icon/SMDH/banner metadata as supported. The CIA must launch without an external `.nds`, `.pak` or loose app config file on SD.
- `romfsInit()` / `romfs:/...` provides readonly game data: `romfs:/data/game.pak`. `romfsMountSelf` supports the app's own RomFS for an installed NCCH and embedded RomFS for a `.3dsx`, but this project deliberately uses **external SD packs for `.3dsx`** and bundled RomFS for CIA.
- Specify ordinary **64 MB O3DS application memory mode**, O3DS clock, required service permissions and a **nonzero title SaveDataSize** in the RSF/exheader. Reserve sufficient save blocks for two original-format payload copies plus config and metadata; verify allocation and formatting on real hardware. Do not request N3DS-only memory/features as baseline.
- Use a **stable, unique homebrew Title ID per edition**, selected/recorded during personal packaging (do not generate a fresh ID on every update). Do not collide with retail titles or another installed homebrew. An upgrade must keep the ID so the user's save archive persists. HeartGold and SoulSilver CIA builds should not accidentally replace or share saves with each other.
- Mount `ARCHIVE_SAVEDATA` using `archiveMount(ARCHIVE_SAVEDATA, fsMakePath(PATH_EMPTY, ""), "save")` for the title; after a complete logical save write, flush/close and check **`archiveCommitSaveData("save")`**. Commit is not implicit at unmount. Handle the unformatted/first-run save archive with the supported `FSUSER_FormatSaveData` flow only if actually required by the title/install configuration; verify behavior rather than guessing the initial archive state.
- Store `save:/game_a.bin`, `save:/game_b.bin` and `save:/config.ini` within that archive. **No ordinary SD writes for gameplay data/config** in CIA mode. Optional diagnostic export only when explicitly requested and when SD access is permitted.
- Treat the CIA as self-contained with respect to **game content**. System services and any separately required DSP firmware are platform prerequisites, not files to bundle without rights.
- Verify the installed CIA's archive mount, commit, reboot/relaunch persistence, title update behavior and uninstall/save-loss behavior in Azahar and on O3DS. Preserve the Title ID and document backup/export before uninstalling.

### 4.4 Shared save design and migration

- Preserve the original HGSS binary save payload and checksum behavior; reuse audited fixes from `hgss-3ds`, including the CRC/reflection issue. Do not alter the game's in-memory save semantics to match a 3DS file API.
- Implement **two transaction generations** around raw save payloads: each slot has a small external header (magic/version/edition/ROM identity/generation/length/checksums) followed by unmodified HGSS save bytes. On load choose the newest valid generation; on write update the inactive generation, flush/close, then commit if applicable. Do not assume rename is power-fail atomic.
- Test first-run, two consecutive saves, corrupted newest copy, short write, full SD/archive, interrupted write, checksum mismatch, edition mismatch and relaunch after process termination. A failed save must be reported, not ignored.
- Provide an **offline import/export tool** for standard raw HGSS `.sav` files. Exports strip the transaction wrapper and preserve native DS save data. Imports validate original data structure/CRC and avoid overwriting a good save before explicit acceptance. Include a clear migration path between `.3dsx` and CIA through an export/import function; never silently copy between their storage areas.
- Configuration has a versioned schema with defaults and validation. Set nonessential remaster options only after base rendering correctness. Disable autosave in baseline to reduce extra IO and unintended save semantics.

**Primary libctru sources:** [romfs.h](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/romfs.h), [archive.h](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/archive.h), [fs.h](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/services/fs.h), [Coffeepot filesystem-and-saves](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/filesystem-and-saves.md), [makerom RSF example](https://github.com/Steveice10/buildtools/blob/master/3ds/template.rsf).

## 5. Rendering architecture: GPU-only by construction

### 5.1 Frame ownership

The DS game may change emulated graphics memory/registers during callbacks. Build a coherent immutable GPU-facing command/state view for each scene, with content generations for VRAM tiles, palettes, OAM, DMA writes and register-dependent effects. Track live memory aliasing and VRAM bank remaps. Keep source buffers and GPU resources immutable until GPU completion; scanout availability is a separate ownership condition. Use bounded double buffers or a ring if hardware measures a benefit.

**Never** use CPU pixel output as a substitute for a missing PICA200 mode. A missing effect is a tracked renderer blocker; the host oracle can render it to support debugging, but the 3DS executable cannot quietly do so.

### 5.2 Native 2D

- **BG tiles**: retain 4bpp/8bpp tiles as palette-expanded GPU atlas entries, keyed by tile data + effective palette + bank mapping + generations; only dirty entries are regenerated. Keep visible map geometry/chunks and scroll via UV/transform; don't rebuild full images on camera movement.
- **OBJ sprites**: retain atlases and ordered quads; respect OAM index/priority, affine transform, object window, mosaic, flip, transparency and tie behavior. Do not reorder overlapping translucent objects just to reduce texture binds.
- **Priority**: faithfully position engine-A 3D as one BG-priority participant and compose it against BG/OBJ. Preserve equal-priority and blend behavior using ordered passes and tested depth/stencil rules, not a single approximate depth sort.
- **Windows, clipping, fades and special blends**: scissor, stencil, alpha hardware, TEV and targeted render passes, with per-feature fixtures and an explicit supported-mode table. Handle unusual bitmap/capture modes with tested GPU-specific techniques.
- **HBlank effects**: execute required callbacks in original order, collect effective per-scanline state and merge only adjacent identical spans. Do not arbitrarily group different lines. Specialize recurring effects only after equivalence tests.
- **Display capture**: use two GPU-owned targets, never sample the target currently being written or download it to a CPU buffer. Validate feedback, prior-frame contents and ordering.
- **Lower screen**: draw independently; do not clear/rebuild/transfer unchanged lower display layers. Invalidate cursor, touch states, animation, palette and window changes correctly.

### 5.3 Native HGSS text and UI

Keep `TextPrinter`/`RenderText` interpretation, callbacks, delays, control codes, centering and original glyph advances. Hook the point where `RenderText` has decided a glyph and the original path calls `CopyGlyphToWindow`. Emit **font ID, glyph ID, logical x/y, palette/color, shadow, clip/window ID and reveal generation** to a retained GPU text model.

Build the font data offline from **the user's original font NARC**. Produce native-resolution 400/320-display glyph atlases using carefully grid-fitted Pokémon-style bitmap shapes, plus original DS advance/width metrics and per-variant mapping. Do not use a generic 3DS system font for game dialogue; use citro2d custom BCFNT as an implementation benchmark, and prefer a compact dedicated citro3d glyph-quad backend where original text semantics make it simpler or faster. Batch compatible glyphs into few atlas/material runs; no per-character texture bind or full-window upload. Precompute common foreground/shadow variants and compare mask-based colouring on hardware before committing.

**Scaling rule**: cumulative logical character positions map to native output coordinates, then snap consistently; never round each advance separately and accumulate visual drift. Compare nearest, carefully pre-scaled, and hand-tuned native glyphs on actual 3DS LCDs, preserving readable strokes. Reflow/anchor windows intentionally in the new 320×192 logical top-screen canvas. Test multiline, battle messages, Bag, Pokégear, Japanese/other supported language glyph coverage, inline colour, arrows, text erasure, scrolling and backgrounds with non-text pixel writes.

A general HGSS `Window` can receive non-text pixel mutations. Intercept those too. For a pure text window render retained glyphs; for mixed-content windows implement a GPU-owned surface and ordered drawing operations. Audit `CopyWindowToVram`, `FillWindowPixelBuffer`, `ScrollWindow`, `BlitBitmapRect` and readback-like callers. Do not globally no-op original window writes without proving all consumers.

One previously reported O3DS battle dialogue GPU path mirrored text horizontally. Make that a mandatory regression capture, not a known acceptable defect.

### 5.4 Native 3D

- Decode G3 display lists into object-space **retained meshes** when immutable, with versioned dependencies and inherited matrix/material state. Use specialized fast paths for common geometry while retaining a **GPU-based** general command translator for unusual supported geometry.
- Put ordinary transforms in PICA200 vertex shaders; pack vertices and retain textures. Avoid repeated CPU transform, per-triangle clipping and reconstruction wherever an equivalent PICA path is verified. DS fixed-point/lighting/clipping differences require explicit test cases, not blind fast-math.
- Culling starts with object/chunk bounds and the actual widened 320×192 logical camera, including shadows/particles extending beyond their owners; never drop newly visible side content.
- Treat alpha-test, translucent, polygon-ID, depth, fog, shadow and edge cases as distinct material contracts; state batching may cross only proven-safe boundaries. Audit the old duplicated-NPC and halo regressions.
- Render directly at 400×240; 2D effects/text are positioned for native output. Preserve original scaling/layout of image-only cutscenes by deliberate framing or remaster side artwork, not stretching or mirroring.

### 5.5 GPU resource and memory strategy

- Use O3DS VRAM (6 MiB) and normal application memory budgets from the start; enumerate every target and pool, including double buffers, depth and pending uploads. Start with a single 400×240 top plus 320×240 bottom and no compulsory stereo/right-eye target.
- Investigate RGB565/RGBA5551/A8/LA8 vs RGBA8 where fidelity permits. PICA200 texture layout is tiled; texture swizzles are done offline or on **dirty** updates only. Use ETC1 only for assets whose image quality is acceptable.
- Batch geometry by compatible material/texture and draw order. Cache only resources whose full dependencies are tracked. Keep hot ARM11 loops compact (Coffeepot measured a severe instruction-fetch cliff beyond ~16 KiB of hot code).
- Use targeted GPU transfers/fills for supported copies, clears and output conversions. Never do a routine GPU→CPU→GPU compose cycle.
- Start with conservative citro3d cache flushing, then measure explicit dirty-range flushing and `GX_CMDLIST_FLUSH` **only when every modified GPU-visible buffer is flushed correctly**. Treat `C3D_FrameEnd`/queue timings as asynchronous; beware in-flight buffer overwrites.
- Treat core 1 as a *shared system core*. Don't rely on worker completion within the active frame. Add one bounded throughput worker (assets/audio) only when A/B testing improves total frame deadlines and doesn't slow services. N3DS-specific core 2 is outside the baseline.

**Core references:** [Coffeepot renderer architecture](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/renderer-architecture.md), [tile renderer and O3DS measurements](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/2d-tile-renderer.md), [PICA200 2D semantics](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/rendering-2d.md), [HGSS original font](https://github.com/pret/pokeheartgold/blob/master/src/font_data.c), [original printer](https://github.com/pret/pokeheartgold/blob/master/src/render_text.c), [GPU command pipeline](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/pica200-command-pipeline.md).

## 6. Game timing, audio and app lifecycle

The game source's main loop processes simulation tasks and waits for VBlank in a way that normally yields a 30 Hz field. Maintain those semantics for timing-sensitive code, callbacks, interrupts, RNG and audio; don't simply call update twice per frame. `C3D_FrameRate(30.0f)` with `C3D_FRAME_SYNCDRAW` is an available presentation mechanism, but **must not be layered on top of an additional independent pair of DS VBlank waits** that halves the rate. Implement one explicit scheduling bridge mapping emulated DS events to physical presentation events, and count **game ticks, produced frames, queued frames, GPU-complete frames and actually presented distinct frames** separately. Validate the DS's ~59.826 Hz native refresh discrepancy and drift.

Audio: port the corrected baseline mixer/SDAT logic, queue output through `ndsp`, keep DSP buffer ownership and flushing correct, and make audio independent of GPU waits. Only then benchmark ARMv6 packed-SIMD mixing or compatible direct DSP channel operations; compare actual samples and event timing before adopting hardware-channel mapping. No frame-synchronous SD logging or audio decode. Use bounded prefetch and queues.

Integrate APT suspend/resume, sleep/wake, HOME, low-memory failures, input/touch, RTC and controlled exit. Stop new work, finish ongoing GPU use, drain queues in dependency order, persist committed saves before services shut down, then free resources. Invalidate potentially lost GPU resources after restore. Test 3DSX return-to-Homebrew-Menu separately from CIA HOME behavior.

## 7. Execution sequence — one implementer pass

**Milestone A — Inventory and reproducible build (first deliverable).** Pin sources and dependencies; study the owner's supplied docs. Produce a patch inventory from all three ports, a file/API map for HGSS fonts/windows/2D/3D/interrupts, a feature-coverage matrix, and a buildable empty O3DS `.3dsx` + installed test CIA from one ELF. Verify dual asset mounts and both storage backends with a dummy pack and save write/reload. Start CI and manifest generation. Do not spend the run only writing these docs.

**Milestone B — Asset tool and test harness.** Implement `rompack` input validation, deterministic content pack, edition/region metadata, no copyrighted files in source, extraction of original font/tiles/models and representative captured scenes. Build an offline reference image generator or compare recorded NDS outputs; record enough register/VRAM/previous-frame state to catch window/blend/capture differences. Add a host unit test runner and Azahar launch/smoke harness.

**Milestone C — Native font + 2D PICA.** Build both screens' GPU targets and a coherent 2D state translator. Ship GPU tile/sprite layers, scissor/stencil/blend, native font atlases, text commands, dirty tracking and static-bottom reuse. Exercise typewriter/battle dialogues, text erase/scroll, palettes, OBJ priorities and dense menus via replay. Keep all unresolved graphics modes marked **unsupported in debug**, not silently rendered by a CPU fallback.

**Milestone D — Native 3D + widened scene player.** Translate G3 commands into retained mesh/material resources, PICA vertex transforms, visible-list culling, resource cache and correct 400×240 view. Replay fixed Goldenrod movement and battles; compare to NDS reference inside the original 256×192 region and verify intentional side extensions. Profile draw/record counts and GPU work on Azahar only as diagnostic information.

**Milestone E — Integrate actual HGSS early.** Bring forward the verified game/Nitro pieces—overlays, input, clocks, DMA/register state, cartridge file resolver, sound and original saves. Boot a supported HeartGold and SoulSilver title into normal field gameplay, then battles, dialogs, menus and transitions. No separate 'works on N3DS' fork or stage. Preserve the source's original gameplay logic.

**Milestone F — Uncommon graphics and correctness.** Complete remaining DS visual contracts (capture/feedback, HBlank effects, windows, mosaic, fades, battle intro, cutscenes, animated fonts, special sprite palettes). Eliminate every known unsupported mode in the test corpus with GPU implementations. Maintain hardware/reference regression tests and release-time guards against software rasterization or GPU image readback.

**Milestone G — Both full installation products.** Build `dist/3dsx/` containing executable/metadata and example empty directory structure; require user-built `.pak`. Build a HeartGold CIA and SoulSilver CIA from supplied personal packs in the same invocation. Validate Title IDs, RomFS, first-run archive, commits, reboots, edition separation, CIA update and file migration/export/import. The public artifacts contain no ROM-derived content; user-local builds do.

**Milestone H — Integrated profiling and Azahar hardening.** Use deterministic scripted scenes on a pinned Azahar build. Capture comparison screenshots, framebuffer differences, queue accounting, p50/p95/p99 stage times, memory peaks, resource counts and failures. Profile cold and warm loads. Build a hardware-ready profiler that can run without a debugger and write aggregates asynchronously. Stop only after attempting all feasible steps, with documented remaining obstacles and exact reproduction instructions.

**Milestone I — Physical O3DS acceptance (owner hardware gate).** On 2011 O3DS, at baseline speed/memory: run both packaging modes, all prepared scripts, 10-minute sustained tests, suspend/restart, save fault tests, gameplay correctness, and worst-case resource stress. Compare against the 25 ms headroom objective and ~33.4 ms deadline. An O3DS miss is a hard regression to diagnose; no N3DS speedup or software fallback workaround. Iterate architecture until hardware passes. This gate cannot be completed by Azahar-only testing.

## 8. Scene corpus and mandatory regressions

Create scene IDs, original reference captures, deterministic input scripts, fixed save files and expected graphics-state coverage:

| Scene | Primary stress |
| --- | --- |
| Goldenrod standing/walking, nighttime and crowded | 3D reuse, camera, culling, UV animation, texture residency |
| Dense town/interior/route, area boundary | wide culling, asset streaming, transitions |
| Following Pokémon plus many NPCs | billboard/mesh and priority, animated resources |
| Wild/trainer battle + message box | 3D/2D priority, battle text horizontal-mirror regression |
| Status effects, weather, encounter transition | HBlank, blending, stencil, capture/feedback |
| Bag, party, Pokégear, PC and Save menu | font metrics, windows, touch, bottom-screen invalidation |
| Title, cinematic and credits | bitmap/special modes, artwork framing and long-duration playback |
| Large-font/Unown/non-Latin text samples | full glyph atlas and control-code coverage |
| Save/load, full card, interrupted write, suspend | both storage backends, no data loss |

For every graphics capture, include all effective dependencies: VRAM and palette contents/generations, OAM, layer ordering, bank mapping, HBlank state, relevant prior frame and render-target contents. Record unexpected render modes as blocking omissions, not zeros.

## 9. Profiler, tests and build gates

Capture per-frame `logic_ms`, `g2_decode_ms`, `g2_batch_ms`, `g3_decode_ms`, `g3_build_ms`, `gpu_record_ms`, `C3D_FrameEnd_ms`, `gpu_time_ms`, GPU clear/transfer timings when available, frame-begin wait, queued/completed/presented counts, VBlank periods, audio refills, SD read/decode, atlas misses/updates/uploads, geometry cache hits, command/vertex bytes, VRAM/FCRAM high-water, unsupported operation count and invalid GPU ownership events. All nested counters must state whether they overlap; never double-add parent and child spans.

Instrument work in bounded in-memory structures, export aggregated logs outside the hot frame, and create an explicit low-overhead production profiling mode. Do not assume emulator timing resembles real O3DS. Compare optimized and reference versions from matching inputs and pinned binaries.

**CI/automation gates:** clean builds of both targets; host tests for pack parsing, original-save checksums, corruption recovery, font metric layout, VRAM dirty dependencies and geometry-cache invalidation; no forbidden graphics backend symbols in shipping outputs; no accidental ROM-derived files in version control/artifacts; reproducible pack hash; Azahar smoke tests when executable environment supports them; file size and memory-budget reports. Preserve known-good binary after every passed integration gate.

## 10. Builder interface and install UX

Implement a simple reproducible command-line builder (syntax illustrative; the implementer owns the final tested interface):

```sh
# User-supplied original ROM; create homebrew SD pack
python tools/build/pack.py --rom /path/to/HeartGold.nds --edition hg --out build/user/hg.pak
python tools/build/build.py --target 3dsx --pack build/user/hg.pak --out dist/3dsx

# Personal full CIA containing assets in its RomFS
python tools/build/build.py --target cia --pack build/user/hg.pak \
  --title-id <persistent-user-chosen-id> --out dist/private/hg.cia
```

The builder must validate dependencies and title metadata, print source edition/region and hash, calculate pack/installed size, reject duplicate/colliding Title IDs where detectable, record the save identity in a reusable manifest and show installation instructions. No GUI, online account or ROM-download feature is needed. CLI errors should explain how to fix an unsupported ROM or missing tool.

`3dsx` instructions: copy executable and pack to the app folder under `/3ds/`; run through Homebrew Launcher.  
`cia` instructions: provide own source ROM; build/install the CIA on a homebrew-enabled 3DS; use HOME to launch; game data comes from RomFS and save/config from the title archive. Back up/export original progress before uninstalling or changing Title ID.

## 11. Deliverables at end of the one-shot implementation

1. **Source tree** with reproducible build, pinned upstream dependencies, patch ledger and clear license/provenance records.
2. **ROM packer** supporting verified HeartGold/SoulSilver input revisions, deterministic game assets and user instructions.
3. **Bootable `.3dsx` and user-buildable `.cia`** with correct storage adapter behavior, save imports/exports and no ROM contents in public distributables.
4. **Integrated playable native game as far as the implementation reaches**, without a disguised CPU renderer; clearly identify incomplete modes.
5. **GPU renderer and native font** with capture/image-diff regression tests, resource ceilings and diagnostic counters.
6. **Automated Azahar tests and real-O3DS test kit**, including scripts and saved scenario states.
7. **Status report**: concrete files/commits, commands run, scenes passed, screenshots/diffs, timing counters, failure codes, known blockers and a ranked next-action list. Claims must be tied to exact evidence.

### Hard prohibitions

No claiming full 30 FPS on O3DS from Azahar; no N3DS-only 'temporary' optimization path; no CPU renderer fallback, GPU framebuffer download or CPU full-screen conversion as a runtime crutch; no relabelled/interpolated frames presented as native updates; no silent object drops; no stale-input hash cache; no unauthorized bundled ROM/font/assets in public output; no save writes without error handling and, in CIA mode, commit; no changing Title ID on upgrade; no repeated low-value 'test-only' loops or architecture rewrites without measured evidence.

### Final acceptance statement

**“O3DS full-speed remaster” is earned only when the actual 2011 console runs the verified release, both 3DSX and CIA builds work, both editions' tested content works, saves persist across restart, GPU output is correct, and the measured 30 FPS deadline is met across the acceptance corpus.** An Azahar-only implementation can be a strong release candidate but is not a certified O3DS performance result.

---

## Primary reference index

- [HGSS `main.c`: VBlank/game loop](https://github.com/pret/pokeheartgold/blob/master/src/main.c)
- [HGSS `render_text.c`: original printer/control codes](https://github.com/pret/pokeheartgold/blob/master/src/render_text.c)
- [HGSS `font_data.c`: original glyph decoder](https://github.com/pret/pokeheartgold/blob/master/src/font_data.c)
- [HGSS `bg_window.c`: text window pixels and VRAM copies](https://github.com/pret/pokeheartgold/blob/master/src/bg_window.c)
- [citro2d text API and source](https://github.com/devkitPro/citro2d/blob/master/source/text.c)
- [libctru RomFS](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/romfs.h)
- [libctru archive mount/commit](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/archive.h)
- [libctru FS services and save formatting](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/services/fs.h)
- [devkitPro RomFS `.3dsx` example](https://github.com/devkitPro/3ds-examples/blob/master/romfs/Makefile)
- [RSF `SaveDataSize` example](https://github.com/Steveice10/buildtools/blob/master/3ds/template.rsf)
- [Coffeepot files/saves](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/filesystem-and-saves.md)
- [Coffeepot frame pacing](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/frame-pacing.md)
- [Coffeepot GPU renderer architecture](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/renderer-architecture.md)
- [Coffeepot tile-renderer hardware results](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/2d-tile-renderer.md)
- [Coffeepot HGSS port case study](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/case-studies/case-study-hgss-port.md)
- [Azahar source and releases](https://github.com/azahar-emu/azahar)
- [3dbrew filesystem archive documentation](https://3dbrew.org/wiki/Filesystem_services)
