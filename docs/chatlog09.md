# Codex conversation

## User

you're working on kohjoh. first, read the project documentation and commit history to get yourself up to speed. then, read chatlog08, read C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3 and continue diagnosing what's still hot. refer to workdir/coffeepot/ctr for 3ds technical knowledge. if the previous session fucked up the logging system and/or took useless diagnostics, do not even bother asking me, just get to fixing.

## Assistant

I’ll read Kohjoh’s docs, recent commits, chatlog08, and the latest 3DS log, then trace what’s still hot against the CTR references and fix any logging issues I find.

## Activity

    $ Get-Location; rg --files -g '*AGENTS*' -g '*README*' -g '*chatlog08*' -g '*.md' kohjoh | Select-Object -First 70
    
    Path
    ----
    C:\Users\terezi\workdir
    kohjoh\PROVENANCE.md
    kohjoh\README.md
    kohjoh\docs\worklog\2026-11-13.md
    kohjoh\docs\worklog\2026-11-12.md
    kohjoh\docs\troubleshooting.md
    kohjoh\docs\testing.md
    kohjoh\docs\status.md
    kohjoh\docs\README.md
    kohjoh\docs\lessons.md
    kohjoh\docs\glossary.md
    kohjoh\docs\engineering\hgss-remaster\README.md
    kohjoh\docs\engineering\hgss-remaster\HARDWARE_OPTIMIZATIONS.md
    kohjoh\docs\engineering\hgss-remaster\GPU_RESOURCES.md
    kohjoh\docs\engineering\hgss-remaster\GPU_3D.md
    kohjoh\docs\engineering\hgss-remaster\GPU_2D.md
    kohjoh\docs\engineering\hgss-remaster\FRAME_PIPELINE.md
    kohjoh\docs\engineering\hgss-remaster\FONT_RENDERER.md
    kohjoh\docs\engineering\hgss-remaster\CORRECTNESS_MATRIX.md
    kohjoh\docs\engineering\hgss-remaster\ASSET_BUILDER.md
    kohjoh\docs\comparison.md
    kohjoh\docs\chatlog08.md
    kohjoh\docs\chatlog07.md
    kohjoh\docs\chatlog06.md
    kohjoh\docs\chatlog05.md
    kohjoh\docs\chatlog.md
    kohjoh\docs\building-windows.md
    kohjoh\docs\building-linux.md
    kohjoh\docs\architecture.md
    kohjoh\AGENTS.md
    kohjoh\kohjoh.md
    kohjoh\src\compat\nitrosystem\README.md
    kohjoh\src\compat\hgss\README.md
    kohjoh\src\compat\nitro\README.md
    
    ✓ • 548ms

## Activity

    $ git status --short; git log -12 --oneline
     M README.md
     M docs/building-windows.md
     M docs/testing.md
     M src/app/main.c
     M src/compat/nitro/src/port_os.c
     M src/compat/nitro/src/port_snd.c
     M src/compat/nitro/src/port_sound_chip.c
     M src/compat/nitro/src/port_timer.c
     M src/diagnostics/crash.c
     M src/diagnostics/log.c
     M src/diagnostics/perf.c
     M src/diagnostics/perf.h
     M src/graphics/g2d/g2d.c
     M src/graphics/g3d/g3d.c
     M src/graphics/g3d/textures.c
     M src/graphics/pica/gpu.c
     M src/graphics/renderer.c
     M src/graphics/resources/atlas.c
     M src/runtime/kj_frame.c
     M src/runtime/kj_g3.c
     M tools/azahar/run.ps1
     M tools/game/hgss_game.py
    ?? docs/chatlog.md
    ?? docs/chatlog02.txt
    ?? docs/chatlog03.txt
    ?? docs/chatlog04.txt
    ?? docs/chatlog05.md
    ?? docs/chatlog06.md
    ?? docs/chatlog07.md
    ?? docs/chatlog08.md
    ?? kohjoh.zip
    ?? kohjoh2.zip
    ?? kohjoh3.zip
    ?? private-gitignore/
    8a92e93 docs: display-list cache default, [gputime], [sndprof]
    6444c8b renderer: [gputime] GPU timing per 600 pictures
    bafd5aa g3d: bind the 3D program once per draw
    c2087f3 kj_g3: display-list cache on by default (KJ_DL_NO_CACHE to disable)
    5026d7b docs: sound mixer findings and results
    dacb07d Sound mixer: SSAT, one-compare loop mark, sub-step ADPCM loop
    da6b36c Sound mixer: render ADPCM in a separate pass from volume and pan
    d433d8a docs: status next steps after session 3
    8a39cea kj_g3: recheck unrecordable lists once per 64 pictures; session 3 docs
    4dc90e6 kj_g3: reuse a display-list entry's storage when it re-records
    5d1a7d2 Game patch 2006: party move scan with one decrypt per Pokemon
    6191966 Geometry DMA in one transfer; display-list cache fixed on master; honest stage times
    ✓ • 211ms

## Activity

    $ Get-Content AGENTS.md; Get-Content README.md; Get-Content docs/status.md; Get-Content docs/worklog/2026-11-13.md; Get-Content docs/architecture.md
    # AGENTS.md
    
    Repository-wide rules for agent sessions working on kohjoh.
    
    ## READ THE WIKI
    
    You're not expected to have deep technical knowledge of the 3DS platform or how to program efficiently for it. This information has been collected for you already. If it isn't already on disk, clone https://github.com/estyseesghosts/coffeepot.git. Always refer to coffeepot's CTR section when you're writing code or planning project changes - read the relevant hardware, firrmware or engineering articles, so you understand what you're working with. If you can't justify a change with real-hardware evidence or a citation from the wiki, you probably should not do it. Never edit coffeepot yourself. If you notice something genuinely incorrect, PR and tell the user. Always follow the wiki's C style guide.
    
    The coffeepot wiki is provided locally on this machine in workdir/coffeepot/.
    ## Delegating to subagents
    
    A lead agent must give every subagent it calls **explicit instructions, explicit deliverables and explicit avoidables**. This is not optional, and it matters most for implementer agents: they edit files and cannot rely on chat context the lead has not written down.
    
    Every subagent brief must state, in the prompt itself:
    
    1. **Objective** — the one bounded slice to complete, and why it exists.
    2. **Allowed paths** — the exact files or directories the subagent may read and edit.
    3. **Deliverables** — the concrete output: code, patch files, tests, docs, and the required report format (files changed, commands run, results, open questions).
    4. **Acceptance checks** — the commands or observations that prove the work is done, and the expected result.
    5. **Avoidables** — what not to touch or do: paths outside the slice, unrelated refactors, the decomp checkout in place, commits/pushes unless explicitly assigned, and any known traps from `kohjoh.md` or `docs/troubleshooting.md`.
    6. **Stopping point** — when to stop and report instead of guessing, for example if the slice grows, the brief conflicts with `kohjoh.md`, or an acceptance check cannot pass.
    
    Do not send a subagent a vague goal and let it infer scope. If the lead cannot fill in all six items, the slice is not ready to delegate. A brief that is broader, vaguer or in conflict with `kohjoh.md` should be revised before the call, not discovered by the subagent mid-edit.
    
    Implementer subagents hold `docs/status.md`, `docs/lessons.md` and `kohjoh.md` as required reading; the brief adds the slice-specific detail those files cannot contain. When this file disagrees with an agent definition under `.opencode/agents/` or `.claude/agents/`, this file wins.
    
    ## Git identity
    
    The only git identity an agent may use in this repository is:
    
    ```text
    terezi <jules@foxtails.me>
    ```
    
    Set it in the repository itself before the first commit, and do not rely on a
    machine-wide default or invent a placeholder:
    
    ```sh
    git config user.name terezi
    git config user.email jules@foxtails.me
    ```
    
    Every agent commit carries both fields. Do not use `A4`,
    `a4@example.invalid`, the host's auto-detected address, or any other
    identity, and do not author on someone else's behalf. If a commit already
    exists under another identity, report it to the lead rather than rewriting
    history: an artifact's `build-info` names the commit it was built from, and
    re-authoring a commit changes that name.
    # kohjoh
    
    a native homebrew port of pokemon heartgold and soulsilver for the original 3DS
    
    not tested or known to be stable yet; runs near full-speed on real hardware, excluding geometry-heavy map areas && the intro ho-oh/lugia
    
    this repository does not contain any unlicensed game data.
    for heartgold, you need a file with sha-1 starting with 4fcded0e2713dc03929845de...
    for soulsilver, f8dc38ea20c17541a43b58c5...
    
    - Build: [docs/building-windows.md](docs/building-windows.md)
    - How it works: [docs/architecture.md](docs/architecture.md)
    - Testing and profiling: [docs/testing.md](docs/testing.md)
    - Lessons and pitfalls: [docs/lessons.md](docs/lessons.md), [docs/troubleshooting.md](docs/troubleshooting.md)
    - Design brief: [kohjoh.md](kohjoh.md); imports and licences: [PROVENANCE.md](PROVENANCE.md)
    
    GPL-3.0 (see [LICENSE](LICENSE)).
    # Project status
    
    As of 2026-11-13. Evidence levels: **built** (compiles and links), **run**
    (boots in Azahar), **verified** (checked in Azahar against an expectation),
    **O3DS** (measured on an Old 3DS). Nothing here has reached O3DS.
    
    ## On `master`
    
    | Area | Level | Evidence |
    | --- | --- | --- |
    | HeartGold `.3dsx` (game 1092 files + runtime) | built | `tools/game/hgss_game.py`; SoulSilver not yet built |
    | Boot, intro, title, menus | verified | intro frames 150-1650 held and captured |
    | GPU 2D: tiles, OBJ, priorities, blending, windows, per-line spans | verified on tested frames | intro frame 1650 matches the reference renderer |
    | GPU 3D: GPU transform, textures, lighting, widescreen field | verified visually | New Bark (intro), Goldenrod field |
    | Title 3D (Ho-Oh, W-buffer, skinned envelopes) | verified | title frames 2300/2500 match melonDS; NitroSystem 0305 / NitroSDK 0606 / game 2004 |
    | Input: buttons and touch | verified | touch BAG opens the Bag; B closes it |
    | Pacing lock (no music drift after late frames) | run | `[pace]` counters; user-reported drift fixed in code, not re-confirmed |
    | Tile-row gaps fix (texcoord inset) | built | awaiting user confirmation |
    | Retained 2D layers, unchanged-screen skip | verified | Goldenrod: bottom screen skipped 557 of 600 frames |
    | Geometry decoder fast paths | verified | Goldenrod renders; logic 37 → 25 ms/frame (Azahar) |
    | Pokémon-data keystream cache (game patch 2002) | built | crypto was ~12% of field instructions |
    | Present skip (never two in a row) | run | `0089b28`; O3DS starvation diagnosed from o3ds-log-0 |
    | Display-list cache | verified, **on by default** since `c2087f3` (`--define KJ_DL_NO_CACHE` to disable) | `KJ_DL_VERIFY` 0 mismatches in Goldenrod idle, intro, title, and Goldenrod → Route 34 → trainer battle → field; battle frames use no lists |
    | GPU timing `[gputime]` (`6444c8b`) | run | C3D drawing time and command-buffer peak; GSP perf log reads 0 in Azahar (not emulated), O3DS untested |
    | GX command DMA in one transfer (SDK patch 2003) | verified | 65% of FIFO words had been split mid-command; logic 21.8 → 21.0 ms |
    | Party move scan, one decrypt per mon (game patch 2006) | verified | Goldenrod idle logic −0.1 ms |
    | Saves | run | interim raw file `saves/<edition>.sav` ([`save_raw.c`](../src/storage/save_raw.c)) |
    | Audio | run | user heard music; Azahar now has DSP firmware, NDSP runs |
    | Sound mixer ADPCM render pass (`da6b36c`, `dacb07d`) | verified | host differential test bit-exact (100k programs); Azahar title 125 → 102 instructions per channel-sample |
    | CIA, RomFS pack, title save archive | not implemented | path lookup only ([`paths.c`](../src/storage/paths.c)) |
    
    ### Performance
    
    **O3DS, profiled build `92a9a6d` (o3ds-log-0):** Route 34 near 30 FPS;
    Goldenrod unplayable. In the Goldenrod window the game ran ~15 game
    frames/s (half speed) but showed only ~4 pictures/s: 1606 of 2206 were
    skipped (uncapped present skip, fixed in `0089b28`), a
    G3 decode cost of ~8.4 ms per game frame (probes subtracted) and g2d
    13 ms p50 per picture. Not yet re-measured on hardware after the fixes.
    
    **Azahar, Goldenrod idle (instruction counts, a floor for O3DS):**
    
    | Stage, ms/frame | `92a9a6d` | `d089ca2` | round 3 | `4dc90e6` + `KJ_DL_CACHE` |
    | --- | --- | --- | --- | --- |
    | logic (game + G3 decode + H-blank walk) | 25.6 | 21.8 | 21.8 | 13.7 (max 17.0) |
    | H-blank walk | 2.1 | 0.5 | 0.5 | 0.5 |
    | g2d (inclusive of g3d before `6191966`) | 4.8 | 4.1 | 3.4 | 0.6 exclusive |
    | g3d | 4.2 | 3.5 | 2.8 | 2.8 |
    
    With the cache, 74% of FIFO words replay. The 2D "retention cliff" under
    scanline effects (chatlog05) does not occur in Goldenrod idle: one span per
    frame, no rebuilt layers. Walking and Route 34 are unmeasured.
    
    Where the remaining time goes (PC samples, Goldenrod): G3 decode loop ~22%,
    idle ~27%, Pokemon data crypto ~11% (`GetIdxOfFirstPartyMonWithMove`
    every field frame: 6 mons x 5 `GetMonData`, each decrypt + checksum +
    re-encrypt), citro3d recording ~10%. Per frame: 51k G3 words, ~124 draws,
    8.4k triangles, 67 box tests (56 culled by the game).
    
    Azahar charges one tick per instruction and does not model the 16 KiB
    I-cache; O3DS pays both. The decode loop was shrunk from 14.6 to 2.0 KiB
    for that reason (not visible in Azahar).
    
    ## On `wip/savestore` (commit `59dbed5`, does not build)
    
    - Two-generation SaveStore ([kohjoh.md §4.4](../kohjoh.md)); needs bool/int
      trampolines in `kj_boot.c`.
    - Display-list cache: recorder fix, bump arena, verify mode (clean).
    - SDK shim uses junctions instead of MSYS copies. **Regression:** with it the
      bench save is not detected. Bisected: not caused by SDK patch 2003. Cause open.
    - SDK patch 2003: GX command buffers in one transfer.
    - Scripted runs ignore live input (stray keyboard input contaminated a test).
    
    **Sound mixer (o3ds-log-1):** 20-30% of the O3DS application core in every
    window, ~170 cycles per channel-sample, almost all channels ADPCM. Cut by
    18% in Azahar instructions (`dacb07d`); O3DS gain unmeasured (the old loop
    also spilled to the stack every frame, which Azahar does not charge).
    
    ## Next
    
    1. Find the save-detection regression on `wip/savestore`; finish SaveStore.
    2. Battle crashes (user-reported): reproduce with `route34-battle.txt`; `logs/crash.txt`.
    3. Route test: Goldenrod → Route 34 grass → wild battle (`tests/inputs/route34-south.txt`).
    4. O3DS re-test of Goldenrod, the title and a battle on a default build
       (cache, mixer and `[gputime]` all in), with ARM11 performance counters
       for I-cache misses. H-blank (0.5 ms) and texture work wait for that data.
    5. Whole-batch frustum culling (chatlog06: 72 of 124 Goldenrod batches lie
       wholly outside one plane).
    6. Title/Ho-Oh: lit lists decode live with CPU lighting (`shade` ~11% of
       samples); vertex lighting on the PICA would cut that and let them record.
    7. CIA packaging, SoulSilver build.
    # 2026-11-13
    
    ## Sound mixer (o3ds-log-1 hot path)
    
    - o3ds-log-1: the mixer took 20-30% of the application core in every
      window, ~170 cycles per channel-sample, ~99% of active channels ADPCM.
      Azahar had never run it (no DSP firmware); `dspfirm.cdc` is now staged.
    - `da6b36c`: ADPCM decoded and resampled into a block buffer (32-bit phase
      fraction, exact under a checked precondition), then volume/pan in a
      second pass. 125.1 → 109.2 instructions per channel-sample.
    - `dacb07d`: SSAT saturation, one-compare loop mark, sub-step loop.
      → 102.1. `mix_block` 3.2 → 4.4 KiB.
    - Both proven bit-exact with a host differential driver (random register
      programs, 100k iterations, output and chip state hashed every call).
    - Azahar A/B: title screen idle (no input script), `[sndprof]` windows,
      identical channel counts in both builds.
    
    Open: O3DS measurement; further levers are ARMv6 SIMD in the volume/pan
    pass and moving the mixer to core 1 (kohjoh.md 5.5 requires an A/B).
    # How kohjoh works
    
    kohjoh runs the original HeartGold/SoulSilver code (the pinned decompilation)
    on the 3DS with a new runtime and GPU renderer. Imported code is listed in
    [`PROVENANCE.md`](../PROVENANCE.md); the design brief is [`kohjoh.md`](../kohjoh.md).
    
    ## Two halves, one bridge
    
    | Half | Compiled with | Contents |
    | --- | --- | --- |
    | DS side | NitroSDK headers, 4-byte enums | game, Nitro compat ([`src/compat`](../src/compat)), [`src/runtime`](../src/runtime) |
    | 3DS side | libctru/citro3d, short enums | [`src/app`](../src/app), [`graphics`](../src/graphics), [`audio`](../src/audio), [`storage`](../src/storage), [`diagnostics`](../src/diagnostics) |
    
    They meet only in [`kj_bridge.h`](../src/runtime/kj_bridge.h): fixed-width
    types, no enums, `kj_ds_*` / `kj_ctr_*` functions.
    
    ## Frame flow
    
    1. The game's own main loop runs on the main thread (`kj_game_main`).
    2. Each DS V-blank wait (`OS_Halt`) calls `kj_ctr_vblank`: APT, input, then a
       wait locked to the physical V-blank count. A late frame does not wait, so
       the game never loses DS time against the real-time audio; a stall over 30
       V-blanks resyncs instead ([`main.c`](../src/app/main.c)).
    3. After the V-blank callback and vwait tasks (game patch 2001),
       [`kj_frame.c`](../src/runtime/kj_frame.c) runs the H-blank walk and
       publishes one coherent snapshot: registers, palettes, OAM, VRAM banks,
       per-line registers and the last swapped 3D scene. Loops that never reach
       the hook (boot, error screens) publish from the V-blank once more than 4
       V-blanks pass without one.
    4. [`renderer.c`](../src/graphics/renderer.c) waits for the previous GPU frame
       (`C3D_FrameBegin(0)`), records both screens and submits. Nothing reads DS
       memory after it returns. If the game is ≥2 refreshes behind, the
       presentation (not the game tick) is skipped and counted.
    
    ## 2D ([`g2d.c`](../src/graphics/g2d/g2d.c))
    
    - DS tiles are palette-expanded into a 1024×1024 RGBA5551 atlas
      ([`atlas.c`](../src/graphics/resources/atlas.c)), one DS tile per PICA 8×8
      tile. A slot is reused only if its tile bytes are byte-identical and its
      palette version unchanged: no write tracking, no hashing.
    - Layers draw in DS priority order, with the 3D scene at BG0's slot. Stencil
      bits carry blend-target membership (alpha blend = two draws) and window
      regions; BLDY and master brightness are full-screen passes.
    - Text BG and OBJ layers are retained when their inputs (registers, palette,
      map/OAM, ext palettes, the character bytes read) are byte-identical; an
      unchanged screen is not drawn or transferred at all.
    - Frames with per-line register changes are drawn as spans of identical lines.
    - Top screen: DS 256×192 at 1.25× centred in 400×240; 2D is scissored to the
      centre, 3D fills the width. Bottom: 1.25× to 320×240.
    
    ## 3D ([`kj_g3.c`](../src/runtime/kj_g3.c), [`g3d.c`](../src/graphics/g3d/g3d.c))
    
    - The DS side interprets the G3 command stream: matrix stacks (20.12 fixed),
      lighting, texgen, box/position/vector tests and result registers on the
      CPU, exactly as the game reads them back.
    - Vertices stay in object space; each batch carries a palette of up to 16 clip
      matrices. The 3DS side multiplies each by one fixup matrix (viewport,
      widescreen x-scale 0.8, screen rotation, depth range) and the vertex shader
      transforms on the GPU.
    - The box test is widened for widescreen so side content is not culled.
    - DS textures convert once per content change ([`textures.c`](../src/graphics/g3d/textures.c)).
    - Not yet supported (counted, never silently dropped): fog, toon/highlight,
      edge marking, shadow volumes, wireframe, display capture, bitmap BGs.
    
    ## Audio, storage, diagnostics
    
    - The vendored ARM7 sequencer and DS sound chip mix 32768 Hz stereo on an
      audio thread (core 0, above the game) fed by NDSP ([`audio.c`](../src/audio/audio.c)).
    - Saves: interim raw 512 KiB file; two-generation SaveStore on `wip/savestore`.
    - Logs go to a memory ring flushed by a low-priority thread; `[perf]`, `[pace]`,
      `[gpu]` and `[ds]` aggregates every 600 publishes ([testing](testing.md)).
    ✓ • 393ms

## Activity

    $ Get-Content AGENTS.md -ErrorAction SilentlyContinue; Get-Item kohjoh/docs/chatlog08.md,kohjoh/private-gitignore/o3ds-log-3; rg --files coffeepot/ctr | Select-Object -First 35
    
        Directory: C:\Users\terezi\workdir\kohjoh\docs
    
    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    -a---          11/13/2026  1:52 PM         367795 chatlog08.md
    
        Directory: C:\Users\terezi\workdir\kohjoh\private-gitignore
    
    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    d----          11/13/2026  2:08 PM                o3ds-log-3
    coffeepot/ctr\software\update-data.md
    coffeepot/ctr\software\titles.md
    coffeepot/ctr\software\title-metadata-and-tickets.md
    coffeepot/ctr\software\title-list.md
    coffeepot/ctr\software\system-flaws.md
    coffeepot/ctr\software\svc-reference.md
    coffeepot/ctr\software\streetpass-cec.md
    coffeepot/ctr\software\services.md
    coffeepot/ctr\software\security-architecture.md
    coffeepot/ctr\software\sd-filesystem.md
    coffeepot/ctr\software\savegames.md
    coffeepot/ctr\software\os.md
    coffeepot/ctr\software\nand-filesystem.md
    coffeepot/ctr\software\kernel.md
    coffeepot/ctr\software\kernel-objects.md
    coffeepot/ctr\software\firmware.md
    coffeepot/ctr\software\file-formats.md
    coffeepot/ctr\software\dev-unit-software.md
    coffeepot/ctr\software\cro-modules.md
    coffeepot/ctr\software\country-region-data.md
    coffeepot/ctr\software\boot-process.md
    coffeepot/ctr\index.md
    coffeepot/ctr\homebrew\research-index.md
    coffeepot/ctr\homebrew\picasso-and-citro3d.md
    coffeepot/ctr\homebrew\luma3ds.md
    coffeepot/ctr\homebrew\libctru.md
    coffeepot/ctr\homebrew\history.md
    coffeepot/ctr\homebrew\exploits.md
    coffeepot/ctr\homebrew\development-workflow.md
    coffeepot/ctr\homebrew\custom-firmware.md
    coffeepot/ctr\homebrew\boot9strap.md
    coffeepot/ctr\homebrew\3ds-examples.md
    coffeepot/ctr\hardware\video-capture.md
    coffeepot/ctr\hardware\tp-nets.md
    coffeepot/ctr\hardware\security-hardware.md
    
    ✓ • 409ms

## Activity

    $ Get-Content kohjoh.md; Get-Content docs/lessons.md; Get-Content docs/troubleshooting.md; Get-Content docs/testing.md; Get-Content docs/building-windows.md; Get-ChildItem private-gitignore/o3ds-log-3 -Recurse | Select-Object FullName,Length; git diff --stat; git log -6 --format=fuller
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
    | Ba
    ... command output truncated for persistence ...
    laimed
      the game ran "near full speed" when that window ran at half speed. Fixed
      in docs; counter semantics are now written in testing.md.
    - **Harness races.** A second `run.ps1` killed the emulator of the first
      (three lost runs). Stop the old harness before starting another.
    - **Lost samples.** gdbprof printed nothing when Azahar closed; it now
      prints what it has.
    - **Sampler bias.** Tight loops (the crypto XOR) looked bigger in samples
      than in `[perf]` time. Measure with stage times.
    
    - **Measuring the wrong thing.** The display-list cache replayed 4% of
      words. The first guess (lists left with BEGIN open) explained 13%; a
      coverage counter (`[dlcov]`) showed 65% of words were split mid-command
      by NitroSDK's 118-word DMA bursts. Count where the work goes before
      fixing a guessed cause.
    - **Bump arenas that flush.** Re-recording into fresh space filled the arena
      and flushed it twice per 600 frames (29.6 ms spikes). Reuse an entry's
      storage when it re-records.
    - **An unprofiled thread.** The DS sound mixer runs on its own thread and
      took 20-30% of the O3DS application core (o3ds-log-1), but Azahar lacked
      DSP firmware, so `ndspInit` failed and the mixer never ran in any
      emulator profile. Make sure every thread the device runs also runs under
      test (`dspfirm.cdc` is now in Azahar's sdmc).
    
    ## Rules
    
    1. Commit each verified step; keep `master` building and verified.
    2. Check the Coffeepot wiki and `docs/engineering/hgss-remaster/` before
       inventing a technique; cite it.
    3. Compare against the oracle or a known-good capture before blaming a subsystem.
    4. Profile with PC sampling; treat Azahar time as instruction count.
    5. Every cache needs an exact validity check and a verify mode.
    6. After any build-system change, rerun intro, Goldenrod (with save) and input.
    7. After touching a hot loop, check its size with `nm -S` (16 KiB I-cache).
    8. One Azahar harness at a time.
    # Troubleshooting
    
    Each entry: symptom → cause → fix. The tools already contain the fixes; this
    page explains them for the next breakage.
    
    ## Windows build chain
    
    | Symptom | Cause | Fix |
    | --- | --- | --- |
    | `Cannot create temporary file in C:\Windows\` | MSYS2 drops `TMP`/`TEMP` from child processes; native GCC falls back to `C:\Windows`. | `common.mk` exports them; `hgss_nitro.py` sets them in Python. |
    | `-DFOO` set via `FOO=1 python ...` has no effect | MSYS2 Python started directly from Git Bash does not see inline environment variables (inside `bash.exe -lc` it does). | `hgss_game.py --define FOO`. |
    | Bootstrap: `git-remote-https.exe: error while loading shared libraries` | devkitPro MSYS2's `git` has a broken HTTPS helper. | `export PATH="/c/Program Files/Git/cmd:$PATH"` inside the `-lc` shell ([build](building-windows.md#build)). |
    | `mkdir: cannot create directory '/home'`; headers "not found" | MSYS2 Python resolves the working directory as `/home/<user>/...`, which bash, make and native tools cannot use; MSYS also does not convert glued arguments such as `-idirafter/c/...`. | Invoke with absolute `/c/...` paths and `--root`; pass `-idirafter` and its path as separate arguments. |
    | `ld: cannot find /c/Users/.../x.o` | Native `ld` reads paths inside `@response` files literally; MSYS converts only command-line arguments. | Response files use `C:/...` (`_tool_path`). |
    | `ar: Argument list too long` | Very deep output paths (e.g. under `%TEMP%`). | Build under the repository's `build/`. |
    | Every build recompiles all 1092 files | The dependency parser split `C:/...` at the drive colon. | Split on `": "`; objects are skipped when newer than source and all `-MMD` dependencies, with identical flags. |
    | SDK/compat edits have no effect | MSYS2 Python's `symlink_to` silently **copies** directories, so `build/hgss-sdk` held stale copies. | On `wip/savestore`: junctions via `cmd.exe //c mklink //J` (MSYS rewrites a bare `/c`). See the open regression in [status](status.md). |
    | `gen_fx_consts` fails (missing libc headers / `Read error`) | MSYS gcc lacks libc headers; the tool reads its CSV in text mode. | Bootstrap generates the header in Python (`prepare_game_headers.py`). |
    | Partial Azahar screenshots | DPI virtualisation. | `run.ps1` calls `SetProcessDPIAware` and resizes the window. |
    
    ## Runtime
    
    | Symptom | Cause | Fix |
    | --- | --- | --- |
    | Rows reversed inside every 8×8 tile | Wrong Morton row order: PICA tiles are stored top row first. | Fixed in `atlas.c`/`textures.c`; found by comparing with the reference renderer. |
    | One-pixel horizontal gaps between tile rows | Nearest sampling near v = 1.0 in float24 rounded into the neighbouring (often transparent) atlas tile. | Texcoords in 1/16 texel, inset by 1/16 per edge. |
    | Music drifts ahead of the picture | A frame overrunning a V-blank lost that V-blank for good. | Emulated V-blanks locked to the real V-blank counter; late frames catch up. |
    | Field at ~17 FPS in Azahar | Geometry decode and `GetMonData` crypto (PC sampling). | Decoder fast paths; keystream cache (game patch 2002). |
    | White map, 1% speed with `KJ_DL_CACHE` | The display-list recorder's counters sat after its arrays and escaped the `memset`; recordings overran and corrupted the heap (100% of samples in `_malloc_r`). | Fixed on `wip/savestore` (explicit reset, bump arena); `KJ_DL_VERIFY` compares replay with live decode. |
    | Saved game ignored (new-game intro) | Open, `wip/savestore` only: appeared with the SDK shim junction change; not caused by patch 2003. | Investigate what differs between the old copied shim and the live sources. |
    | A test run diverges from its script | Live keyboard input on the Azahar window was OR-ed with the script. | `wip/savestore`: scripted runs ignore live input. |
    | Title lower screen black (no Ho-Oh) | The title uses `GX_BUFFERMODE_W` with near = 0, so every vertex has z/w = 1 and failed a Z-mode GREATER test. | W-buffer path in `g3d.c`: depth = w/4096 (PICA W-buffer mode), LESS, depth cleared to 0xFFFFFF. |
    | Chunks of a skinned model missing (title Ho-Oh's wings) | Direct stores to G3 command registers (`reg_G3_MTX_STORE = ...`) land in plain memory on the port. NNS NODEMIX's envelope matrices were never stored, so weighted vertices restored zero matrices and collapsed to clip (0,0,0,0). | Route every direct G3 command-register write through `NitroPort_GxDirectWrite` (NitroSystem 0305, NitroSDK 0606, game 2004). Never "fix" this by changing w or dropping/redrawing w <= 0 triangles: that is what produced the old port's stretched Ho-Oh. |
    | Vendored SDK change has no effect | The vendored library build failed and the link fell back to the prebuilt `libnitro*.a` (build log: "vendored library build failed"). | Grep the build log for `error:` before testing any SDK/NitroSystem change. |
    | O3DS: town "unplayably slow" though the game ticks | Uncapped present skip: while ≥2 refreshes behind, every picture was skipped; a game slightly over budget showed ~4 pictures a second (o3ds-log-0: 1606 of 2206 skipped). | Never skip two in a row (`renderer.c`, `0089b28`). |
    | O3DS: title stays white (normal build), correct but slow (probe build) | Probably the same skip starvation: the last picture before falling behind was the title's white flash. Not reproduced in Azahar (Azahar runs the title within budget). | Skip cap `0089b28`; needs an O3DS check. |
    | `run.ps1`: "azahar exited before tNNN" | Another `run.ps1` still running killed Azahar when it finished. | One harness at a time; stop the old one first. |
    | `run.ps1` run ends after ~2 s | Without `-Holds` or `-Shots` the harness only waits `-Extra` seconds. | Always pass `-Holds` or `-Shots`. |
    | A C source turns "binary" in git, or a string has a stray tab/CR | Python edits passed through an unquoted bash heredoc turned `\0`, `\t`, `\r`, `\n` escapes into real control bytes. | Use the Edit tool, or write the script to a file with raw strings; scan for NUL/CR after scripted edits. |
    # Testing and profiling
    
    All tools here target Azahar. Azahar is for correctness and instruction-count
    profiling; it is not an O3DS timing authority.
    
    ## Harness: `tools/azahar/run.ps1`
    
    | Option | Effect |
    | --- | --- |
    | `-Artifact` | `.3dsx` to run (default `build/kj/pokeheartgold.3dsx`) |
    | `-Pack`, `-Manifest` | stage a data pack (only needed once) |
    | `-Save path` / `-FreshSave` | install a save / delete the test save |
    | `-Script file` | scripted input (below); without it `script.txt` is removed |
    | `-Shots 10,30` | screenshots at seconds after launch |
    | `-Holds 900,5050` | screenshot when the log says game frame N is held |
    | `-Tag`, `-Timeout`, `-Extra` | output folder, hold timeout, seconds kept after the last shot |
    
    Outputs: `build/azahar/<tag>/` (PNGs, program log, Azahar log). The harness
    resizes the window and is DPI-aware so captures are complete.
    
    ## Scripts (`tests/inputs/*.txt`)
    
    ```text
    every: 10             # each press lasts 10 V-blanks
    5000 START            # press at V-blank 5000 (A B X Y L R START SELECT UP DOWN LEFT RIGHT)
    10200 TOUCH:40,130    # touch in DS bottom-screen coordinates
    dump 5000             # write the renderer's DS input for game frame 5000
    freeze 5000 3         # hold game frame 5000 on screen for 3 s
    end: 11800            # close at V-blank 11800
    ```
    
    V-blank indices count from boot; game frames are publishes (≈ V-blanks / 2).
    A scripted run ignores live input on `wip/savestore`; on `master` a live press
    can still leak in, so do not type into the Azahar window during a test.
    
    | Script | Purpose |
    | --- | --- |
    | `intro-step.txt` | intro frames 150-1650, dumps + holds (no save needed) |
    | `goldenrod-step.txt`, `goldenrod-input.txt` | bench save → Goldenrod; touch/B/DOWN checks |
    | `goldenrod-idle.txt` | stand in Goldenrod (profiling) |
    | `route34-south.txt` | walk south toward Route 34 (path not yet calibrated) |
    
    The Goldenrod save is hgss-3ds's `build/bench/heartgold.sav`.
    
    ## Reference renderer
    
    `python tools/capture/ds_reference.py dump.kjd out` renders the DS 2D picture
    of a dump straight from GBATEK (no shared code with the GPU path) to
    `out_A.png`, `out_B.png`, `out_screens.png`. It does not render 3D (BG0 3D is
    left transparent). Dumps are written to `sdmc:/3ds/kohjoh/dumps/`.
    
    `python tools/capture/g3_reference.py dump.kjd out.png` projects the dump's
    decoded 3D scene with plain float math (one flat colour per batch, DS-style
    depth, triangles with a vertex at w <= 0 counted and skipped). Present here but
    missing on the 3DS: the GPU stage (`src/graphics/g3d`). Missing here too: the
    decoder or the game's command stream (`src/runtime/kj_g3.c`, SDK/NNS
    patches). `--define KJ_G3_ZERO_TRACE` logs the first commands that leave a
    zero position/projection matrix (`[g3zero]`).
    
    ## DS oracle
    
    melonDS (`workdir/melonDS.exe`, source in `workdir/melonDS`) runs the retail
    ROM. `tools/melonds/shots.ps1` saves timed screenshots to
    `build/melonds/<tag>/`; the HG title shows Ho-Oh from about 85 s. Check
    `melonDS/src/GPU3D.cpp` before changing 3D semantics.
    
    ## Script extras
    
    - `9600-11400 DOWN B`: hold keys over a V-blank range.
    - `debug: 0x1|0x2|0x4|0x8|0x10`: 3D draw without cull / depth test / alpha
      test / blending / texture (`KJ_DEBUG_*` in `src/app/script.h`).
    - `[frame]` lines report real refresh intervals (0/1/2/3/4+) and hitches with
      per-stage times and counters. Azahar's "App FPS" is not a stutter measure
      (unchanged screens are not presented).
    - A crash writes `logs/crash.txt` (pc, lr, registers) before the system
      error handler; resolve addresses with addr2line on the `.elf`.
    
    ## Profiling
    
    Build the baseline with `HGSS_PROFILE` unset and the diagnostic build with
    `HGSS_PROFILE=on`, using a separate `--out` for each. The build-info sidecar
    records `profiling` and any `--define` switches. The baseline omits the
    frame, GPU, FIFO and mixer profiling probes; ordinary lifecycle and error
    logging remains enabled. Log and crash files append across launches, with a
    `--- new run ---` separator in the main log. Scripted frame dumps still use
    their frame-number filenames and can overwrite earlier dumps.
    The instrumented build includes per-H-blank timing probes, which add
    substantial overhead; use the normal build for performance comparisons.
    
    `[g3sample]` samples one in 64 FIFO calls. Its total, live decode and replay
    values are elapsed ARM11 ticks for those samples, not totals for the whole
    window. The remainder includes cache lookup, comparison, recording and probe
    overhead; preemption is included. The fixed sampling stride can correlate
    with repeated call patterns, so do not multiply by 64 and treat that as an
    exact total. It does not separately time game-side model traversal or replay
    state and copying.
    
    - **Counters**: every 600 *presented* frames the log gets `[perf]` (stage p50/p95/p99/max),
      `[pace]` (late V-blanks, resyncs, skipped presents), `[gpu]` (layers
      reused/rebuilt, screens skipped, unsupported counts) and `[ds]` (geometry
      words, H-blank cost, display-list cache). `[ds] per game frame` divides by
      the H-blank walk count (one walk per game frame) and prints it with the
      picture count, so skipped presents no longer inflate it. `[perf] g2d` is
      exclusive: the g3d time spent inside it is subtracted (before `6191966`
      g2d included g3d). `[dlcov]` (with `KJ_DL_VERIFY`) splits FIFO words into
      bypassed (small, split mid-command, open BEGIN) and new/changed/never/hit/
      keymiss; `[dlwhy]` counts uncacheable reasons and arena flushes.
      `[gpu] ... char KiB compared` is the retained layers' character check.
      `[gputime]`: GSP perf-log microseconds per picture (P3D command lists, PPF
      display transfers, PSC fills; 0 in Azahar, which does not implement the
      log), `C3D_GetDrawingTime` average/max (the latest completed queue, so not
      tied to one picture) and the peak command-buffer use.
      `[sndprof]` (every ~9.4 s of audio, on the audio thread): cumulative mixer
      ticks, frames and active channels; cycles per channel-sample is
      Δmix_ticks / (Δactive_ch / Δmix_calls) / Δmix_frames. Needs
      `dspfirm.cdc` in Azahar's `sdmc/3ds/`, or NDSP fails and the mixer never runs.
    - **PC sampling**: with Azahar's GDB stub enabled (`use_gdbstub=true`, port
      24689), `python tools/azahar/gdbprof.py build/kj/pokeheartgold-3ds.elf
      --samples 1500 --interval 0.02 [--callers fn1,fn2]` prints a function
      histogram (and callers via LR). This found every hotspot so far.
      - `--pcs fn1,fn2`: per-PC histogram inside those functions (map with
        `arm-none-eabi-objdump -d`).
      - `--stack fn:offset`: histogram of the word at SP+offset while sampled in
        `fn`, i.e. a return address further up the stack. Example: the caller of
        `GetMonData` from `_MonEncryptSegment` is `--stack _MonEncryptSegment:36`
        (16 bytes pushed by it, 8 locals + 12 to `lr` in `GetMonData`).
      - Partial results are printed if Azahar closes mid-run. Give the harness a
        late enough last shot, and never start a second harness while one runs:
        each `run.ps1` kills every Azahar when it finishes.
      - Samples are taken when Azahar's JIT stops at a block boundary, so tight
        loops are over-represented. Use samples to find *where*; use `[perf]`
        stage times (instruction counts) to measure *how much*.
    - **Code size**: Azahar does not model the 16 KiB I-cache. After editing a
      hot loop, check `arm-none-eabi-nm -S --size-sort` on the `.elf`
      (`NitroPort_GxFifoWords` must stay well under 16 KiB).
    - **Probes**: `HGSS_PROFILE=on` compiles the HGSS H-blank/2D tick probes in
      (several hundred `svcGetSystemTick` calls a frame). Normal builds leave
      them out; never make a timing claim from a probe build.
    - **Present skipping is disabled while a script runs** (`kj_script_active`).
      To see pacing as the player does (title, field), run without `-Script`.
    - **Calibration**: the `[calib]` log line shows Azahar's ticks per instruction
      (measured 1.0). Treat Azahar milliseconds as instruction counts.
    - `--define KJ_G3_PROFILE` times each geometry opcode. Each probe costs ~390
      ticks; subtract it, and prefer PC sampling.
    # Build on Windows
    
    Verified 2026-11-12 on Windows 11 with a **from-scratch** bootstrap and build
    (empty `build/upstream`, empty `build/kj`): 91 patches applied, 0 errors,
    `.3dsx` packaged in about 6 minutes, and the title screen checked in Azahar.
    Output: a HeartGold `.3dsx`. SoulSilver and CIA are not built yet.
    
    ## Requirements
    
    | Tool | Where | Notes |
    | --- | --- | --- |
    | devkitPro | `C:\devkitPro` | Packages: devkitARM, libctru, citro3d, 3dstools, picasso. Versions are pinned in [`upstream.lock`](../upstream.lock). Its MSYS2 (`C:\devkitPro\msys2`) supplies `bash`, `make` and Python 3.14 (`/usr/bin/python`). |
    | Git for Windows | `C:\Program Files\Git` | **Required for bootstrap.** devkitPro MSYS2's own `git` cannot fetch over HTTPS (see [pitfalls](#pitfalls)). |
    | Azahar | `C:\Program Files\Azahar\azahar.exe` | Testing only (`tools/azahar/run.ps1`). |
    | PowerShell 7 (`pwsh`) | | Runs the Azahar harness. |
    | ROM | your own verified US dump | Only needed for the data pack. Never commit ROMs, packs, saves or extracted data. |
    
    ## Build
    
    Run every build step **inside devkitPro's MSYS2 login shell**
    (`C:\devkitPro\msys2\usr\bin\bash.exe -lc "..."`). The login profile
    (`/etc/profile.d/devkit-env.sh`) sets `DEVKITPRO=/opt/devkitpro` and
    `DEVKITARM=/opt/devkitpro/devkitARM`, which the build reads to find
    `arm-none-eabi-gcc`. Use absolute `/c/...` paths throughout.
    
    From Git Bash or PowerShell:
    
    ```sh
    C:/devkitPro/msys2/usr/bin/bash.exe -lc '
      export PATH="/c/Program Files/Git/cmd:$PATH"    # Git for Windows first (HTTPS)
      K=/c/Users/<you>/workdir/kohjoh
      cd $K
      python tools/game/bootstrap.py                   # pinned decomp + every patch in patches/
      python tools/game/check_patches.py               # fails if build/upstream lacks a patch
      python tools/game/hgss_game.py --root $K/build/upstream --target 3ds --out $K/build/kj -j 32
    ' > build/kj-build.log 2>&1
    ```
    
    **`--target 3ds` is required.** The default is `host32`, a PC build for tests.
    
    A build succeeded only if the log ends with
    `packaged .../pokeheartgold.3dsx` **and** has no `error:` lines. If the
    vendored SDK/NitroSystem library fails to compile, the build prints
    `warning: the vendored library build failed; linking with libnitrosystem.a,
    libnitro.a ...` and **still packages a `.3dsx` from stale libraries**. Any
    SDK or NitroSystem change is then silently absent. Check for it:
    
    ```sh
    grep -E "error:|vendored library build failed" build/kj-build.log   # must print nothing
    ```
    
    ### Outputs (`--out build/kj`)
    
    | Path | Contents |
    | --- | --- |
    | `pokeheartgold.3dsx` | the program |
    | `pokeheartgold-3ds.elf` | symbols, linked at 0x00100000 (`arm-none-eabi-addr2line -e` for crash addresses) |
    | `pokeheartgold.build-info.txt` | commit, dirty flag, sha256, the state of every legacy flag |
    | `obj/`, `app/`, `ctr/`, `sdk/`, `gen/` | incremental objects and generated headers |
    | `link.log`, `link-<utc>.log` | linker output; unresolved symbols are reported here |
    
    The shared vendored-library trees are `build/hgss-sdk`, `build/hgss-nns`, and
    `build/NitroSDK` / `build/NitroSystem` (links into `build/upstream/lib`).
    
    ### Timing and incremental builds
    
    - Bootstrap ≈ 1 min (shallow fetch + `git am` of the patch stack).
    - First build ≈ 5 min at `-j 32`. A rebuild after a kohjoh-side change takes
      about 2 min. Game objects are skipped when they are newer than their source
      and every `-MMD` dependency and their flags match; the Nitro libraries are
      re-made by `make`.
    - Build each variant into its own `--out` (e.g. `--out $K/build/kjz
      --define KJ_G3_ZERO_TRACE`) so variants never mix objects.
    
    ## Build options
    
    ### `hgss_game.py` arguments
    
    | Argument | Meaning |
    | --- | --- |
    | `--target 3ds` | 3DS build (default `host32`: PC test build) |
    | `--game heartgold\|soulsilver` | edition (default heartgold; SoulSilver untested) |
    | `--root PATH` | the patched decomp checkout (use `$K/build/upstream`) |
    | `--out PATH` | output directory (required) |
    | `-j N` | parallel jobs (default: CPU count) |
    | `--define NAME` | add `-DNAME` to the runtime and game (repeatable) |
    | `--keep-going` | compile everything, report per-file errors, then link |
    | `--skip-prepare` | reuse the generated headers in `--root` |
    | `--sanitize` | host32 only: ASan/UBSan |
    
    ### kohjoh switches (`--define NAME`)
    
    | Define | Effect | Status |
    | --- | --- | --- |
    | `KJ_G3_PROFILE` | times each geometry opcode (`[g3prof]`); each probe costs ~390 ticks | diagnostic |
    | `KJ_G3_ZERO_TRACE` | logs the first geometry commands that leave a zero position/projection matrix (`[g3zero]`) | diagnostic |
    | `KJ_DL_NO_CACHE` | turns off the display-list cache (on by default: replays recorded vertex blocks; state commands still run) | A/B and bisects |
    | `KJ_DL_VERIFY` | with the cache on: decode hits live and compare with the recording (`[dlverify] MISMATCH`), plus `[dlwhy]`/`[dlcov]` coverage lines | diagnostic |
    
    Always pass these as `--define`. An inline variable such as `FOO=1 python ...`
    works inside the `bash.exe -lc` shell but **is dropped when MSYS2 Python is
    started directly from Git Bash**. `--define` works either way.
    
    ### Legacy environment variables (from hgss-3ds)
    
    `hgss_game.py` and `src/compat/nitro/Makefile` still read these hgss-3ds
    variables. Set them inside the `-lc` shell, for example `HGSS_PROFILE=on
    python tools/game/hgss_game.py ...`. Each one is recorded in
    `build-info.txt` and appended to the build tag. Leave them unset for normal
    builds.
    
    | Variable | Effect in kohjoh today |
    | --- | --- |
    | `HGSS_PROFILE=on` | enables frame, GPU, FIFO, H-blank, timer and sound profiling; leave unset for a baseline. Use a separate output directory for each variant. |
    | `HGSS_SND_ADPCM=1` | the alternative sample-saturation path in `port_sound_chip.c` |
    | `HGSS_A0_DIAG=1` | H-blank scan cross-check counters in `port_os.c` |
    | `HGSS_BENCH=1` | hgss-3ds benchmark chainload define; no kohjoh source uses it |
    | `HGSS_GX3D_*`, `HGSS_TEXTURE_*`, `HGSS_PALETTE_HASH` | set a define that no kohjoh source reads; no effect |
    | `HGSS_HOST_CC` | host32 compiler (default `gcc-10`) |
    | `DEVKITPRO`, `DEVKITARM` | toolchain location; set by the MSYS2 login profile |
    
    ## Changing the decomp, NitroSDK or NitroSystem
    
    `build/upstream` is generated: the pinned decomp plus `patches/` applied as
    commits. A change to it only counts once it is a patch in `patches/`:
    
    1. Edit inside `build/upstream`, keeping the DS code in `#else` under
       `#ifdef SDK_PORT_3DS`. Build and verify.
    2. Commit in `build/upstream` (identity `terezi <jules@foxtails.me>`).
    3. Export it to the stack that matches the files:
       ```sh
       cd build/upstream
       git format-patch -1 --relative=lib/NitroSystem --start-number 305 -o ../../patches/NitroSystem  # NitroSystem
       git format-patch -1 --relative=lib/NitroSDK    --start-number 606 -o ../../patches/NitroSDK     # NitroSDK
       git format-patch -1                            --start-number 2004 -o ../../patches/pokeheartgold # game
       ```
       Number new patches after the highest one in that directory on every branch.
       `wip/savestore` holds `NitroSDK/2003`.
    4. Commit the patch file in kohjoh.
    
    Bootstrap refuses an existing `build/upstream` if it has local changes, or if
    its applied-patch count differs from `patches/` (for example after switching
    branches). Then run `python tools/game/bootstrap.py --clean`, which deletes and
    re-creates it. Export your commits first; `--clean` discards them.
    `check_patches.py` compares commit subjects, so it catches a tree that is
    missing a patch. It does not catch a tree with **extra** commits from another
    branch: switch branches → `bootstrap.py --clean`.
    
    ## Data pack
    
    Not yet run from this repository (tested via hgss-3ds's identical builder):
    
    ```sh
    python tools/rompack/verify_rom.py HeartGold.nds
    python tools/rompack/extract_rom.py HeartGold.nds -o build/private/hg
    python tools/rompack/pack.py build/private/hg -o build/private/hg.pak --rom HeartGold.nds
    ```
    
    The runtime needs the `.pak` and the extractor's `manifest.json`, installed as
    `sdmc:/3ds/kohjoh/data/heartgold.pak` and `heartgold.manifest.json`. For
    SoulSilver add `--build-id soulsilver-nitrofs-1` to `pack.py`.
    
    ## Run in Azahar
    
    ```powershell
    pwsh tools/azahar/run.ps1 -Pack build\private\hg.pak -Manifest build\private\hg\manifest.json -FreshSave -Shots 10,30 -Tag boot
    ```
    
    This stages the build to Azahar's SD (`%APPDATA%\Azahar\sdmc\3ds\kohjoh\`),
    launches it, and saves screenshots and logs to `build/azahar/<tag>/`. Pass
    `-Holds` or `-Shots`: without them the harness closes Azahar after about 2 s.
    See [testing](testing.md) for scripts, held frames, dumps and profiling.
    
    ## Pitfalls
    
    | Symptom | Cause | Fix |
    | --- | --- | --- |
    | `git-remote-https.exe: error while loading shared libraries` during bootstrap | devkitPro MSYS2's `git` has a broken HTTPS helper. | Put `/c/Program Files/Git/cmd` first on `PATH` inside the `-lc` shell. |
    | `.3dsx` built but an SDK/NNS change has no effect | vendored library compile failed; link used the stale prebuilt libraries | grep the log for `error:` / `vendored library build failed` |
    | `mv`/`rm` of `build/upstream`: "Device or resource busy" | a shell's working directory is inside it | `cd` out first |
    | `build-info.txt` says `dirty: yes` with no source changes | untracked files in the repository count as dirty | harmless; commit or ignore them |
    
    More, with causes, in [troubleshooting](troubleshooting.md#windows-build-chain).
    The build tools already handle each case listed there.
    
     README.md                              |  11 +--
     docs/building-windows.md               |   2 +-
     docs/testing.md                        |  18 +++++
     src/app/main.c                         |   6 ++
     src/compat/nitro/src/port_os.c         |  10 +++
     src/compat/nitro/src/port_snd.c        |  18 ++++-
     src/compat/nitro/src/port_sound_chip.c |   6 ++
     src/compat/nitro/src/port_timer.c      |   4 ++
     src/diagnostics/crash.c                |   2 +-
     src/diagnostics/log.c                  |   6 +-
     src/diagnostics/perf.c                 |   3 +
     src/diagnostics/perf.h                 |  12 ++++
     src/graphics/g2d/g2d.c                 |  38 ++++++++++
     src/graphics/g3d/g3d.c                 |  22 ++++++
     src/graphics/g3d/textures.c            |  18 +++++
     src/graphics/pica/gpu.c                |   2 +
     src/graphics/renderer.c                |  10 +++
     src/graphics/resources/atlas.c         |   8 +++
     src/runtime/kj_frame.c                 |  10 +++
     src/runtime/kj_g3.c                    | 127 +++++++++++++++++++++++++++++++--
     tools/azahar/run.ps1                   |  16 ++++-
     tools/game/hgss_game.py                |  14 ++--
     22 files changed, 342 insertions(+), 21 deletions(-)
    commit 8a92e93e255e07e9c09a036e8a64f7ed71ae5533
    Author:     terezi <jules@foxtails.me>
    AuthorDate: Fri Nov 13 02:31:07 2026 -0500
    Commit:     terezi <jules@foxtails.me>
    CommitDate: Fri Nov 13 02:31:16 2026 -0500
    
        docs: display-list cache default, [gputime], [sndprof]
    
        Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
    
    commit 6444c8bd375b002bf87f28ff8fcc3fde6fb65a64
    Author:     terezi <jules@foxtails.me>
    AuthorDate: Fri Nov 13 02:30:29 2026 -0500
    Commit:     terezi <jules@foxtails.me>
    CommitDate: Fri Nov 13 02:30:48 2026 -0500
    
        renderer: [gputime] GPU timing per 600 pictures
    
        chatlog06 section 5 and coffeepot ctr performance.md (Per-operation GPU
        times; GPU breakdown table): GSP perf-log deltas per picture (P3D command
        lists, PPF display transfers, PSC fills), C3D_GetDrawingTime average and
        maximum (latest completed queue, so averaged rather than tied to the
        current picture) and peak C3D_GetCmdBufUsage. Azahar does not implement
        the GSP perf log (reads 0); there C3D drawing time is 3.1 ms average,
        5.5 ms maximum, in Goldenrod idle, and the command buffer peaks at 8%.
    
        Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
    
    commit bafd5aa2c2ef1fb4b6b13666c5d4c59dab030b5c
    Author:     terezi <jules@foxtails.me>
    AuthorDate: Fri Nov 13 02:30:29 2026 -0500
    Commit:     terezi <jules@foxtails.me>
    CommitDate: Fri Nov 13 02:30:39 2026 -0500
    
        g3d: bind the 3D program once per draw
    
        kj_g3d_draw bound the program, invalidated the cached state, then bound
        it again (chatlog06 section 5). Invalidate first, then bind once; the
        second bind was always a real bind, so state is unchanged. A program
        change reconfigures the shader units and attribute setup (coffeepot ctr
        PICA200 gpu-rendering.md, state-change costs). Goldenrod renders unchanged.
    
        Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
    
    commit c2087f3c2ad8c8a75debc5a312ccca5225c54696
    Author:     terezi <jules@foxtails.me>
    AuthorDate: Fri Nov 13 02:30:28 2026 -0500
    Commit:     terezi <jules@foxtails.me>
    CommitDate: Fri Nov 13 02:30:28 2026 -0500
    
        kj_g3: display-list cache on by default (KJ_DL_NO_CACHE to disable)
    
        chatlog06 order 1: the cache was opt-in, so ordinary builds lacked the
        Goldenrod logic saving (21.8 -> 13.7 ms in Azahar, status.md).
        KJ_DL_VERIFY (live decode compared with every replay) now also covers
        the walk Goldenrod -> Route 34 and a full trainer battle and back to the
        field (tests/inputs/route34-battle.txt, 30000 V-blanks): 0 mismatches,
        no crash; 1016 KiB retained of the 6 MiB arena. Battle frames use no
        display lists (0 replayed, 0 decoded), so the cache is idle there.
        Default build, Goldenrod idle: 42022 lists replayed, 10778 decoded per
        600 frames; the field renders correctly. The O3DS test is still pending
        (status.md Next 4); KJ_DL_NO_CACHE keeps the uncached decoder for A/B.
    
        Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
    
    commit 5026d7b91e760217d3ccb7bfbfcae7e176e3853a
    Author:     terezi <jules@foxtails.me>
    AuthorDate: Fri Nov 13 02:02:29 2026 -0500
    Commit:     terezi <jules@foxtails.me>
    CommitDate: Fri Nov 13 02:02:29 2026 -0500
    
        docs: sound mixer findings and results
    
        Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
    
    commit dacb07d82426cf1207ded0df351ff2d0ff6537bf
    Author:     terezi <jules@foxtails.me>
    AuthorDate: Fri Nov 13 02:02:10 2026 -0500
    Commit:     terezi <jules@foxtails.me>
    CommitDate: Fri Nov 13 02:02:10 2026 -0500
    
        Sound mixer: SSAT, one-compare loop mark, sub-step ADPCM loop
    
        Second pass on the ADPCM render loop from da6b36c, which still loaded
        both saturation limits and the step-table base from the literal pool and
        spilled loop state to the stack:
        - saturate with ARMv6 SSAT (coffeepot ctr cpu-optimization: ARM11
          pipeline saturate stage, ARMv6 instruction set); host keeps saturate().
        - the clean-loop check is one compare against a mark that is 0 when the
          voice is not clean (index is >= 1 where compared), recomputed after
          advance_voice, which can make the voice clean.
        - with interpolation and a step under one sample per frame (stepWhole 0,
          the common case) a specialised loop frees the whole-step registers.
    
        Verification:
        - Host differential test, 100,000 random register programs against the
          pre-da6b36c mixer: identical for interpolation on and off, with and
          without HGSS_SND_ADPCM.
        - Azahar with DSP firmware, title screen, 8 sndprof windows, same channel
          counts: 109.2 -> 102.1 instructions per channel-sample (125.1 before
          da6b36c, -18% in all).
        - mix_block 4.0 -> 4.4 KiB (nm -S).
    
        Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
    FullName                                                                     Length
    --------                                                                     ------
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\heartgold.log    62257
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f00150.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f00450.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f00900.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f01350.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f01650.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f05000.kjd 3169036
    
    ✓ • 457ms

## Activity

    $ Get-Content kohjoh/docs/chatlog08.md -Tail 450; rg --files coffeepot | rg 'style|performance|profil|arm11|optimization'
        $ Get-Content build/instrumented-build.log -Tail 13; rg -n 'error:|vendored library build failed' build/instrumented-build.log | Select-Object -First 10; if (Test-Path build/instrumented/pokeheartgold.build-info.txt) { Get-Content build/instrumented/pokeheartgold.build-info.txt -TotalCount 6 }
        == overlay table
          129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
        == duplicate symbols
           0 symbols defined in more than one object (0 objects weakened)
        == Wi-Fi stand-ins
          257 Wi-Fi SDK functions stood in for (online play is not ported)
        == compiling the DS-side runtime (11 files)
        == compiling the 3DS-side runtime (17 files)
        == link -> /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
           linked /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
           [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
           packaged /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold.3dsx (5542936 bytes)
           build info: pokeheartgold.build-info.txt
        game: heartgold
        target: 3ds
        profiling: profile
        defines: (none)
        texture_profile: no
        texture_hint: no
        ✓ • 1.30s
    
    ## Activity
    
        $ C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe build/instrumented/pokeheartgold-3ds.elf | Select-String '\[perf\]|\[g3sample\]|\[sndprof\]|\[gputime\]' | Select-Object -First 12; C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe -S build/instrumented/pokeheartgold-3ds.elf | Select-String 'NitroPort_GxFifoWords|s_coarse|sProfMixTicks|kj_perf_publish_end'; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 5
    
        [g3sample] every 64th FIFO call: samples %lu total_ticks %llu decode_ticks %llu replay_ticks %llu; remainder includes
        lookup/recording and probe overhead
        [perf] %-8s us p50 %5lu p95 %5lu p99 %5lu max %5lu
        [perf] window %lu: %u publishes, emulated interval (vblanks) p50 %lu max %lu, misses %lu
        [gputime] 600 pictures: GSP us/picture P3D %lu PPF %lu PSC %lu (log %s); C3D drawing ms avg %.2f max %.2f; cmdbuf use
        max %u%%
        [sndprof] render_calls %llu render_frames %llu stretches %llu sndticks %llu mix_ticks %llu tick_ticks %llu mix_calls
        %llu mix_frames %llu active_ch %llu pcm8_ch %llu pcm16_ch %llu adpcm_ch %llu psg_ch %llu cap_inst %llu dep_ch %llu
        0010a9f8 00000278 T kj_perf_publish_end
        001060bc 0000012c T NitroPort_GxFifoWords
        011a389c 00000008 b s_coarse_decode
        011a38b0 00000004 b s_coarse_depth
        011a38b4 00000004 b s_coarse_n
        011a3894 00000008 b s_coarse_replay
        011a38ac 00000001 b s_coarse_sample
        011a38b8 00000004 b s_coarse_seq
        011a38a4 00000008 b s_coarse_total
        011a0ff8 00000008 b sProfMixTicks
    
    
        ✗ (1) • 2.66s
    
    ## Assistant
    
    Both variants have linked successfully, and the instrumented executable contains the expected frame, GPU, audio and FIFO-sampling reports. I found a few more measurement-only counters during the binary audit, so I’m rebuilding once more with those removed from the normal path before the final comparison.
    
    ## Activity
    
        $ $tokens = $null; $parseErrors = $null; [System.Management.Automation.Language.Parser]::ParseFile((Resolve-Path 'tools/azahar/run.ps1'), [ref]$tokens, [ref]$parseErrors) | Out-Null; $parseErrors; python -m py_compile tools/game/hgss_game.py; git diff --check -- src tools docs/testing.md docs/building-windows.md; Get-Content build/normal-build.log -Tail 5
        done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
        == heartgold 3ds: 1036 C, 56 C++, 150 .s
        == compiling C/C++ (1092 files, -j32)
           1092 compiled, 0 failed
        == assembling ARM9 .s (150 files)
        ✓ • 3.34s
    
    ## Activity
    
        $ Get-Content build/normal-build.log -Tail 7; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 5
          ARM7 CC sound_shared_work.c
          ARM7 CC sound_driver.c
          ARM7 CC snd_arm7_data.c
          ARM7 CC ds_sound_tables.c
        built ../../../build/normal/sdk/libnitro.a
        make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitro'
        + /usr/bin/python /c/Users/terezi/workdir/kohjoh/tools/game/hgss_nitro.py make --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/normal/sdk --subproject nitrosystem lib -k -j 32
        ✗ (1) • 1.60s
    
    ## Activity
    
        $ Get-Content build/normal-build.log -Tail 5; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 5
        == overlay table
          129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
        == duplicate symbols
           0 symbols defined in more than one object (0 objects weakened)
        == Wi-Fi stand-ins
        ✗ (1) • 1.55s
    
    ## Activity
    
        $ Get-Content build/normal-build.log -Tail 7; Get-Process python -ErrorAction SilentlyContinue | Select-Object Id,CPU
          1 NitroSystem header inline functions
        make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
        == overlay table
          129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
        == duplicate symbols
           0 symbols defined in more than one object (0 objects weakened)
        == Wi-Fi stand-ins
    
           Id       CPU
           --       ---
        25048 259.09375
        40112  24.34375
    
    
        ✓ • 1.37s
    
    ## Activity
    
        $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/normal -j 32 > /c/Users/terezi/workdir/kohjoh/build/normal-build.log 2>&1'
        ✓ • 2m 18s
    
    ## Activity
    
        $ Get-Content build/normal-build.log -Tail 8; rg -n 'error:|vendored library build failed' build/normal-build.log; C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe build/normal/pokeheartgold-3ds.elf | Select-String '\[perf\]|\[g3sample\]|\[sndprof\]|\[gputime\]'; C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe -S build/normal/pokeheartgold-3ds.elf | Select-String 's_coarse|sProfMix|kj_perf_publish_end|sSyncTicks'; C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe -S build/normal/pokeheartgold-3ds.elf | Select-String 'NitroPort_GxFifoWords'; Get-FileHash build/normal/pokeheartgold.3dsx -Algorithm SHA256
          257 Wi-Fi SDK functions stood in for (online play is not ported)
        == compiling the DS-side runtime (11 files)
        == compiling the 3DS-side runtime (17 files)
        == link -> /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
           linked /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
           [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
           packaged /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold.3dsx (5529328 bytes)
           build info: pokeheartgold.build-info.txt
    
        00105b40 00000004 T NitroPort_GxFifoWords
    
        Algorithm : SHA256
        Hash      : D1BFB01496F88EE80B9B565BE74FE338AF3EEEA27E2946F426300FEB81D16DF0
        Path      : C:\Users\terezi\workdir\kohjoh\build\normal\pokeheartgold.3dsx
    
    
    
        ✓ • 3.89s
    
    ## Assistant
    
    The final normal binary now passes the symbol and report-string checks: the profiling state and the frame, sound, GPU and FIFO report strings are absent. The instrumented binary is being refreshed from the same final source state; both builds have kept the display-list cache enabled.
    
    ## Activity
    
        $ C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe -S --size-sort build/normal/pokeheartgold-3ds.elf | Select-String 'fifo_words_impl|decode_words|NitroPort_GxFifoWords'; git diff --stat; Get-Content build/instrumented-build.log -Tail 5; rg -n 'error:|vendored library build failed' build/instrumented-build.log | Select-Object -First 5
    
        00105b40 00000004 T NitroPort_GxFifoWords
        00104288 000009d0 t decode_words_impl
        00104c58 00000ee8 t fifo_words_impl
         README.md                              |  11 +--
         docs/building-windows.md               |   2 +-
         docs/testing.md                        |  16 +++++
         src/app/main.c                         |   2 +
         src/compat/nitro/src/port_os.c         |  10 +++
         src/compat/nitro/src/port_snd.c        |  18 ++++-
         src/compat/nitro/src/port_sound_chip.c |   6 ++
         src/compat/nitro/src/port_timer.c      |   4 ++
         src/diagnostics/crash.c                |   2 +-
         src/diagnostics/log.c                  |   6 +-
         src/diagnostics/perf.c                 |   3 +
         src/diagnostics/perf.h                 |  12 ++++
         src/graphics/g2d/g2d.c                 |  38 ++++++++++
         src/graphics/g3d/g3d.c                 |  22 ++++++
         src/graphics/g3d/textures.c            |  18 +++++
         src/graphics/pica/gpu.c                |   2 +
         src/graphics/renderer.c                |  10 +++
         src/graphics/resources/atlas.c         |   8 +++
         src/runtime/kj_frame.c                 |  10 +++
         src/runtime/kj_g3.c                    | 127 +++++++++++++++++++++++++++++++--
         tools/azahar/run.ps1                   |  16 ++++-
         tools/game/hgss_game.py                |  14 ++--
         22 files changed, 336 insertions(+), 21 deletions(-)
        overlay ids: /c/Users/terezi/workdir/kohjoh/build/instrumented/gen/overlay_ids.ld (unchanged)
        overlay ids: /c/Users/terezi/workdir/kohjoh/build/instrumented/gen/overlay_ids.h (unchanged)
        done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
        == heartgold 3ds: 1036 C, 56 C++, 150 .s
        == compiling C/C++ (1092 files, -j32)
    
    
        ✗ (1) • 3.04s
    
    ## Activity
    
        $ C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe build/normal/pokeheartgold-3ds.elf | Select-String '\[ds\]|\[gpu\] 600|\[pace\]|\[frame\]|\[dlcov\]|\[dlwhy\]|\[calib\]'; C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe build/instrumented/pokeheartgold-3ds.elf | Select-String '\[ds\]|\[pace\]|\[frame\]|\[dlcov\]|\[dlwhy\]|\[calib\]' | Select-Object -First 12
    
        [dlwhy] batch-in-block %lu shared-verts %lu straddle/drops %lu lit+matrix %lu swap %lu open-prim %lu flushes %lu
        [dlcov] bypass: small %lu split %lu open-begin %lu (begin first %lu)
        [dlcov] words: short %lu new %lu changed %lu never %lu hit %lu keymiss %lu
        [calib] 1M-iteration loop: %llu ticks (%.2f ticks/iteration)
        [frame] real interval (refreshes) 0:%lu 1:%lu 2:%lu 3:%lu 4+:%lu; hitches %lu, presents skipped %lu; frame time us p50
        %lu p99 %lu max %lu
        [frame] hitches, game frame(refreshes logic/g2d/g3d ms):
        [frame]   %lu: tile conv %lu, spans %lu, quads %lu, tex uploads %lu, tex bytes compared %lu
        [ds] per game frame (%lu game frames / 600 pictures): g3 fifo calls %llu (%lu fifo words, %lu direct words, %lu cmds),
        timer sync %.0f us (%llu calls), sound pump %.0f us, hblank %.0f us
        [ds] display lists: %lu replayed, %lu decoded, %lu uncacheable, %lu KiB retained
        [pace] 600 frames: late V-blanks %lu, resyncs %lu, presents skipped %lu
    
    
        ✓ • 4.22s
    
    ## Activity
    
        $ C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe build/normal/pokeheartgold-3ds.elf | Select-String '\[dlcov\]|\[dlwhy\]'; C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe build/normal/pokeheartgold-3ds.elf | Select-String 'kj_g3_dl_stats|kj_ds_take_profile'
        ✓ • 2.49s
    
    ## Activity
    
        $ rg --files tests build | rg 'sound.*(test|diff)|mix.*(test|diff)' | Select-Object -First 30; Get-Content build/instrumented-build.log -Tail 5
          ARM7 CC ds_sound_tables.c
        built ../../../build/instrumented/sdk/libnitro.a
          1 NitroSystem header inline functions
        make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
        == overlay table
        ✓ • 4.37s
    
    ## Activity
    
        $ Get-Process azahar -ErrorAction SilentlyContinue | Select-Object Id; Test-Path C:/Users/terezi/AppData/Roaming/Azahar/sdmc/3ds/kohjoh/script.txt; Test-Path 'C:/Program Files/Azahar/azahar.exe'; Get-Content build/instrumented-build.log -Tail 5
        True
        True
        == overlay table
          129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
        == duplicate symbols
           0 symbols defined in more than one object (0 objects weakened)
        == Wi-Fi stand-ins
        ✓ • 1.54s
    
    ## Activity
    
        file changes: Completed · 1 changes
        Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\docs\testing.md
        @@ -86,2 +86,4 @@
         their frame-number filenames and can overwrite earlier dumps.
        +The instrumented build includes per-H-blank timing probes, which add
        +substantial overhead; use the normal build for performance comparisons.
    
    
    ## Activity
    
        $ rg -n 's_late|s_resync|pacing_stats|s_pacing|late_vblank' src/app src; Get-Content build/instrumented-build.log -Tail 6
        src/app\main.c:48:static u32 s_late_vblanks, s_resyncs;
        src/app\main.c:61:void kj_pacing_stats(u32 *late, u32 *resyncs)
        src/app\main.c:63:    *late = s_late_vblanks;
        src/app\main.c:64:    *resyncs = s_resyncs;
        src/app\main.c:65:    s_late_vblanks = s_resyncs = 0;
        src/app\main.c:116:        ++s_resyncs;
        src/app\main.c:124:        ++s_late_vblanks;
        src/app\app.h:13:void kj_pacing_stats(uint32_t *late, uint32_t *resyncs);
        src\app\app.h:13:void kj_pacing_stats(uint32_t *late, uint32_t *resyncs);
        src\app\main.c:48:static u32 s_late_vblanks, s_resyncs;
        src\app\main.c:61:void kj_pacing_stats(u32 *late, u32 *resyncs)
        src\app\main.c:63:    *late = s_late_vblanks;
        src\app\main.c:64:    *resyncs = s_resyncs;
        src\app\main.c:65:    s_late_vblanks = s_resyncs = 0;
        src\app\main.c:116:        ++s_resyncs;
        src\app\main.c:124:        ++s_late_vblanks;
        src\graphics\renderer.c:196:        kj_pacing_stats(&late, &resyncs);
           0 symbols defined in more than one object (0 objects weakened)
        == Wi-Fi stand-ins
          257 Wi-Fi SDK functions stood in for (online play is not ported)
        == compiling the DS-side runtime (11 files)
        == compiling the 3DS-side runtime (17 files)
        == link -> /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
        ✓ • 1.37s
    
    ## Activity
    
        $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && HGSS_PROFILE=on python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/instrumented -j 32 > /c/Users/terezi/workdir/kohjoh/build/instrumented-build.log 2>&1'
        ✓ • 2m 10s
    
    ## Activity
    
        file changes: Completed · 1 changes
        Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\app\main.c
        @@ -115,3 +115,5 @@
                 s_base += real - s_emulated - 1u;
        +#ifndef HGSS_PROFILE_OFF
                 ++s_resyncs;
        +#endif
                 real = real_vblanks();
        @@ -123,3 +125,5 @@
             } else {
        +#ifndef HGSS_PROFILE_OFF
                 ++s_late_vblanks;
        +#endif
             }
    
    ## Activity
    
        $ rg -n 'stats|prof|counter|ticks|\+\+' src/audio/audio.c src/app/main.c | Select-Object -First 100; Get-Content build/instrumented-build.log -Tail 8; rg -n 'error:|vendored library build failed' build/instrumented-build.log
        src/audio/audio.c:54:        for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
        src/audio/audio.c:58:                ++queued;
        src/audio/audio.c:62:            ++s_underruns;
        src/audio/audio.c:87:    for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
        src/app/main.c:61:void kj_pacing_stats(u32 *late, u32 *resyncs)
        src/app/main.c:117:        ++s_resyncs;
        src/app/main.c:127:        ++s_late_vblanks;
        src/app/main.c:130:    ++s_emulated;
        src/app/main.c:131:    ++s_vblank_count;
        src/app/main.c:203:        for (unsigned i = 0; name[i] != '\0'; ++i) {
        src/app/main.c:212:        /* Calibration: ticks for a loop of known instruction count (4 per
        src/app/main.c:222:        kj_log("[calib] 1M-iteration loop: %llu ticks (%.2f ticks/iteration)",
          257 Wi-Fi SDK functions stood in for (online play is not ported)
        == compiling the DS-side runtime (11 files)
        == compiling the 3DS-side runtime (17 files)
        == link -> /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
           linked /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
           [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
           packaged /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold.3dsx (5542936 bytes)
           build info: pokeheartgold.build-info.txt
        ✗ (1) • 1.95s
    
    ## Assistant
    
    The final audit caught two pacing statistics that were still incrementing in the baseline. They’re now gated as well; the pacing decisions themselves still use the same functional timing state. I’m completing the last normal rebuild and checksum checks.
    
    ## Activity
    
        $ Get-Content build/instrumented/pokeheartgold.build-info.txt -TotalCount 25; Get-FileHash build/instrumented/pokeheartgold.3dsx -Algorithm SHA256; git diff --check -- src tools docs/testing.md docs/building-windows.md; Get-Content build/normal-build.log -Tail 5
        game: heartgold
        target: 3ds
        profiling: profile
        defines: (none)
        texture_profile: no
        texture_hint: no
        palette_hash: no
        gx3d_hotloop: no
        gx3d_indexed: no
        gx3d_listcache: no
        gx3d_statededup: no
        gx3d_fusehash: no
        gx3d_lazyinsert: no
        gx3d_bg3slot: no
        snd_adpcm: no
        bench: no
        a0_diag: no
        build: 8a92e93-dirty-prof
        sha: 8a92e93e255e07e9c09a036e8a64f7ed71ae5533
        dirty: yes
        built_utc: 2026-11-13T18:44:22Z
        file: pokeheartgold.3dsx
        size: 5542936
        sha256: edf2392b6c1b762f26547332a475bddeacd43dafb83c686c6b89966fff869d98
        status:
    
        Algorithm       Hash                                                                   Path
        ---------       ----                                                                   ----
        SHA256          EDF2392B6C1B762F26547332A475BDDEACD43DAFB83C686C6B89966FFF869D98       C:\Users\terezi\workdir\kohjo...
        overlay ids: /c/Users/terezi/workdir/kohjoh/build/normal/gen/overlay_ids.ld (unchanged)
        overlay ids: /c/Users/terezi/workdir/kohjoh/build/normal/gen/overlay_ids.h (unchanged)
        done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
        == heartgold 3ds: 1036 C, 56 C++, 150 .s
        == compiling C/C++ (1092 files, -j32)
    
    
        ✓ • 2.75s
    
    ## Activity
    
        $ Get-Content build/normal-build.log -Tail 5; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 8
          CC  wm_settings.c
          CC  wm_requests.c
          CC  sdk_data.c
          CC  sdk_standins.c
          CC  nns_data.c
        ✗ (1) • 2.37s
    
    ## Activity
    
        $ Get-Content build/normal-build.log -Tail 5
        == overlay table
          129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
        == duplicate symbols
           0 symbols defined in more than one object (0 objects weakened)
        == Wi-Fi stand-ins
        ✓ • 1.26s
    
    ## Assistant
    
    The instrumented build is packaged and its ELF contains the expected reports and profiling state. The normal rebuild is in its final link preparation; I’ll check the finished artifact and its build metadata before handing over the files.
    
    ## Activity
    
        $ $paths = @('private-gitignore/o3ds-log-2/logs/heartgold.log','private-gitignore/o3ds-log-2/logs/heartgold-dbg.log','private-gitignore/n3ds-log-2/logs/heartgold.log','private-gitignore/n3ds-log-2/logs/heartgold-dbg.log'); foreach ($path in $paths) { $lines = Get-Content $path; $logic = $lines | Select-String '\[perf\] logic'; $gpu = $lines | Select-String '\[gputime\]'; Write-Output "$path lines=$($lines.Count) logic-windows=$($logic.Count) gpu-windows=$($gpu.Count)"; $logic | Select-Object -Last 2; $gpu | Select-Object -Last 1 }
        private-gitignore/o3ds-log-2/logs/heartgold.log lines=157 logic-windows=3 gpu-windows=0
    
           132.201 [perf] logic    us p50 143918 p95 148451 p99 150099 max 158545
           192.602 [perf] logic    us p50 96076 p95 147696 p99 151202 max 154876
        private-gitignore/o3ds-log-2/logs/heartgold-dbg.log lines=567 logic-windows=8 gpu-windows=8
           315.186 [perf] logic    us p50 24913 p95 92182 p99 97354 max 128876
           343.253 [perf] logic    us p50  6095 p95 85573 p99 91167 max 171188
           343.269 [gputime] 600 pictures: GSP us/picture P3D 1904 PPF 610 PSC 448 (log on); C3D drawing ms avg 3.81 max 9.07;
        cmdbuf use max 6%
        private-gitignore/n3ds-log-2/logs/heartgold.log lines=157 logic-windows=3 gpu-windows=0
           132.214 [perf] logic    us p50 147860 p95 152330 p99 153436 max 159776
           197.193 [perf] logic    us p50 94253 p95 152199 p99 154924 max 158323
        private-gitignore/n3ds-log-2/logs/heartgold-dbg.log lines=501 logic-windows=7 gpu-windows=7
           240.455 [perf] logic    us p50 26245 p95 96100 p99 104595 max 122212
           272.900 [perf] logic    us p50 26665 p95 94820 p99 98836 max 177522
           272.916 [gputime] 600 pictures: GSP us/picture P3D 1881 PPF 605 PSC 430 (log on); C3D drawing ms avg 3.75 max 9.53;
        cmdbuf use max 7%
    
    
        ✓ • 1.42s
    
    ## Activity
    
        $ Get-Content build/normal-build.log -Tail 6; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 5
        make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
        == overlay table
          129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
        == duplicate symbols
           0 symbols defined in more than one object (0 objects weakened)
        == Wi-Fi stand-ins
        ✗ (1) • 1.29s
    
    ## Activity
    
        $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/normal -j 32 > /c/Users/terezi/workdir/kohjoh/build/normal-build.log 2>&1'
        ✓ • 2m 28s
    
    ## Activity
    
        $ Get-Content build/normal-build.log -Tail 8; rg -n 'error:|vendored library build failed' build/normal-build.log; Get-Content build/normal/pokeheartgold.build-info.txt -TotalCount 5; C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe build/normal/pokeheartgold-3ds.elf | Select-String '\[perf\]|\[frame\]|\[g3sample\]|\[sndprof\]|\[gputime\]|\[dlcov\]|\[pace\]'; C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe build/normal/pokeheartgold-3ds.elf | Select-String 's_coarse|sProfMix|kj_perf_publish_end|sSyncTicks|s_late_vblanks|s_resyncs'; Get-FileHash build/normal/pokeheartgold.3dsx -Algorithm SHA256
          257 Wi-Fi SDK functions stood in for (online play is not ported)
        == compiling the DS-side runtime (11 files)
        == compiling the 3DS-side runtime (17 files)
        == link -> /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
           linked /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
           [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
           packaged /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold.3dsx (5529288 bytes)
           build info: pokeheartgold.build-info.txt
        game: heartgold
        target: 3ds
        profiling: off
        defines: (none)
        texture_profile: no
    
        Algorithm       Hash                                                                   Path
        ---------       ----                                                                   ----
        SHA256          628F5F83A195BB9B5CC236CAED9ABE186C9DE1E458D0CD745D9383E17EEB9FFA       C:\Users\terezi\workdir\kohjo...
    
    
        ✓ • 2.19s
    
    ## Assistant
    
    The two HeartGold builds are ready:
    
    - [Normal build](C:/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold.3dsx) — profiling off; use this for performance comparisons.
    - [Instrumented build](C:/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold.3dsx) — frame, GPU, audio, H-blank, timer and sampled FIFO reporting enabled.
    
    Both builds completed without vendored-library errors, and both have the display-list cache enabled. I checked the linked executables: the normal build lacks the profiling report strings and sampled FIFO, mixer, frame and pacing profiling state; the instrumented build contains the expected reports and counters. The instrumented build has substantial H-blank probe overhead, so its frame times are diagnostic rather than a performance baseline. Neither build has been run on hardware or in Azahar since these changes.
    
    The instrumented build adds `[g3sample]`, which samples one in 64 FIFO calls and separates live decoding from cache replay. It should help narrow the CPU cost, but **does not yet isolate game-side model traversal or split replay state handling from vertex copying**. The existing hardware logs still point to game logic as the dominant cost: the worst Old 3DS windows show roughly 122–144 ms median logic time while GPU drawing is only a few milliseconds per picture.
    
    Main and crash logs now append across runs; the main log marks each launch with `--- new run ---`. The Azahar harness also preserves the accumulated log. Scripted frame dumps still overwrite an existing dump with the same frame number.
    coffeepot\c-style-guide.md
    coffeepot\oz\engineering\graphics-and-performance.md
    coffeepot\ctr\hardware\arm11-mpcore.md
    coffeepot\nx\engineering\profiling-and-benchmarks.md
    coffeepot\nx\engineering\performance-and-porting.md
    coffeepot\ctr\engineering\performance.md
    coffeepot\ctr\engineering\cpu-optimization.md
    ✓ • 287ms

## Activity

    $ Get-ChildItem private-gitignore/o3ds-log-3 -Recurse | Select-Object FullName,Length; Get-Content private-gitignore/o3ds-log-3/logs/*
    
    FullName                                                                     Length
    --------                                                                     ------
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\heartgold.log    62257
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f00150.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f00450.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f00900.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f01350.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f01650.kjd 2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\dumps\f05000.kjd 3169036
    Get-Content:
    Line |
       2 |  … ject FullName,Length; Get-Content private-gitignore/o3ds-log-3/logs/*
         |                          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         | Cannot find path 'C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-3\logs' because it does not exist.
    
    ✗ (1) • 626ms

## Activity

    $ Get-Content coffeepot/ctr/engineering/performance.md; Get-Content coffeepot/ctr/engineering/cpu-optimization.md; Get-Content coffeepot/c-style-guide.md; Get-Content kohjoh/src/diagnostics/perf.c; Get-Content kohjoh/src/diagnostics/log.c
    # Performance engineering for 3DS homebrew
    
    Making a 3DS program fast is mostly about finding out *which* part is slow before changing anything. The console has a slow CPU (268 MHz on an Old 3DS; **New 3DS:** up to 804 MHz), a capable but unusual GPU, and a small amount of fast memory; a frame can be late because the CPU ran out of time, because the GPU did, because one waited for the other, or because something stalled on a file or a system service. This page shows how to measure each of those with the libraries homebrew already uses, how to pace frames, how to use the second CPU core, and which CPU-side habits pay off on this processor.
    
    Related pages: [memory and cache](memory-and-cache.md) (where buffers live and what flushing costs), [GPU rendering](PICA200/gpu-rendering.md) (draw-call and state costs), [testing on hardware](testing-on-hardware.md) (how to run a benchmark you can trust).
    
    ## Before you start
    
    - Checked against libctru 2.7.1, citro3d 1.7.1, devkitARM r68 and Luma3DS v13.5 source.
    - Claims carry the [evidence labels](../../AGENTS.md#evidence-labels) used across the wiki. Nothing on this page is `[measured]` by the wiki itself yet; where a number comes from someone else's measurement, it is labelled with its origin.
    - Decide your target hardware first. An [Old 3DS](../glossary.md#old-new-3ds) has two slow cores and no L2 cache. **New 3DS:** the CPU can run 3× faster with a 2 MB L2 cache, and a third core is open to `.3dsx` homebrew. A program tuned on a New 3DS tells you nothing about an Old 3DS.
    
    ## The budget
    
    At 60 frames per second a frame lasts 16.7 ms. The ARM11 application core on an Old 3DS runs at 268,111,856 Hz ([CPU](../hardware/cpu.md#clocks-and-core-counts)), so one frame is about 4.47 million clock cycles — before the operating system, cache misses, waiting for the GPU and interrupt handling take their share.
    
    > **New 3DS:** with the fast mode enabled the core runs at 804 MHz (3×) with a 2 MB L2 cache, so a frame is about 13.4 million cycles at 60 fps.
    
    | Target | Frame time | Old 3DS core-0 cycles per frame |
    | ------ | ---------- | ------------------------------- |
    | 60 fps | 16.7 ms | ≈ 4.47 M |
    | 30 fps | 33.3 ms | ≈ 8.94 M |
    | 20 fps | 50 ms | ≈ 13.4 M |
    
    Commercial games treated these budgets as design constraints from the start; homebrew ports of games written for faster machines usually have to do the same (reduce work, not just optimise it).
    
    ### New 3DS: turn on the fast mode yourself
    
    > **New 3DS:** this section applies only to a New 3DS; an Old 3DS has no fast mode.
    
    A `.3dsx` started by Luma3DS gets a generated **[extended header](../glossary.md#exheader)** (exheader) in which the 804 MHz and L2-cache flags are **off** [documented: Luma3DS `sysmodules/loader/source/hbldr.c`, `hbldrPatchExHeaderInfo`]. Call `osSetSpeedupEnable(true)` at start-up; it asks the system (`PTMSYSM_ConfigureNew3DSCPU(3)`) for both the higher clock and the L2 cache and does nothing harmful on an Old 3DS ([libctru](../homebrew/libctru.md#clocks-and-the-new-3ds-speed-up)). An installed `.cia` can instead request them in its exheader. The user's Luma3DS setting `app_launch_new_3ds_cpu` can also force them per title.
    
    ## Measuring time
    
    ### CPU time: the system tick
    
    `svcGetSystemTick()` returns a 64-bit counter running at the base ARM11 clock, `SYSCLOCK_ARM11` = 268,111,856 Hz, on all models and in every New 3DS clock mode [documented: libctru `os.h` defines `CPU_TICKS_PER_MSEC` from `SYSCLOCK_ARM11`]. Reading it is a system call, so it is cheap but not free; time blocks of work, not individual pixels.
    
    `TickCounter` wraps it: `osTickCounterStart(&c)` records a reference, `osTickCounterUpdate(&c)` stores the elapsed ticks, `osTickCounterRead(&c)` returns milliseconds as a `double` [documented: `os.h`, `source/os.c`]. Keep raw `u64` ticks in hot code and convert once per report: on the ARM11, 64-bit division and `double` arithmetic are comparatively expensive (see [CPU-side habits](#cpu-side-habits)).
    
    ```c
    #include <3ds.h>
    
    typedef struct { u64 total, worst; u32 count; } Stat;
    
    static inline void stat_add(Stat *s, u64 ticks)
    {
        s->total += ticks; s->count++;
        if (ticks > s->worst) s->worst = ticks;
    }
    
    /* in the frame loop */
    u64 t0 = svcGetSystemTick();
    update_game();
    stat_add(&update_stat, svcGetSystemTick() - t0);
    
    /* once per second, outside the hot path */
    printf("update avg %.2f ms worst %.2f ms\n",
           update_stat.total / (double)update_stat.count / CPU_TICKS_PER_MSEC,
           update_stat.worst / CPU_TICKS_PER_MSEC);
    ```
    
    Report the **worst** frame and percentiles, not only the average: an average of 12 ms can hide a 200 ms hitch every few seconds, and the hitch is what players notice.
    
    ### citro3d's two timers
    
    citro3d keeps two `TickCounter`s and exposes them in milliseconds [documented: citro3d `source/renderqueue.c`]:
    
    | Function | What it measures | What it does *not* include |
    | -------- | ---------------- | -------------------------- |
    | `C3D_GetProcessingTime()` | Elapsed time from the successful `C3D_FrameBegin` after queue wait to the counter update in `C3D_FrameEnd`, including the end hook and command-list split. It is wall time, including any intervening work or preemption | Work before the timer starts, the default whole-linear-heap flush, render-target transfer setup and queue start; those occur after the counter update |
    | `C3D_GetDrawingTime()` | Wall-clock time from `C3D_FrameEnd` starting the GX command queue until the queue-finished callback runs: the GPU executing the command list **plus** the display transfers to the LCD framebuffers queued for that frame, plus the latency of the completion interrupt reaching your process | Anything that happened on a frame that has not finished yet: the value refers to the most recently *completed* frame |
    
    `C3D_FrameEnd` also flushes the CPU data cache over the whole linear heap unless you pass `GX_CMDLIST_FLUSH` (below and in [memory and cache](memory-and-cache.md#the-c3d_frameend-flush)); that flush happens after the processing counter is updated and before the GPU timer starts, so it appears in neither timer. Time the whole `C3D_FrameEnd` call separately; it includes the hook and command-list split as well as later flush and submission work, so do not add all of it to `C3D_GetProcessingTime` without accounting for overlap. After `C3D_FrameEnd`, the processing sample is for that CPU frame; the drawing value is the latest completed queue measurement, which can belong to a different submission. Associate GPU results with queue completion, not merely the current loop index. [documented: citro3d `renderqueue.c` at the pinned commit; libctru `os.h` and `os.c`]
    
    ### Per-operation GPU times: the GSP performance log
    
    Since libctru 2.5.0, `GSPGPU_SetPerfLogMode(true)` and `GSPGPU_GetPerfLog(&log)` read a performance log kept by the system's GPU service (**[GSP](../glossary.md#gsp)**): for each GPU operation type (`GSPGPU_EVENT_P3D` command-list processing, `GSPGPU_EVENT_PPF` display transfer/texture copy, `GSPGPU_EVENT_PSC0`/`PSC1` memory fills, `VBlank0/1`), the duration of the last one and a running total in microseconds [documented: libctru `gspgpu.h`]. This separates "the GPU is slow drawing" (P3D) from "copying to the screen is slow" (PPF) from "clearing is slow" (PSC). The header's notes matter:
    
    - Use differences of `totalDurationUs` between two points you choose; a frame contains several operations of each type, and the total can overflow.
    - For a memory fill that uses both fill units, take the larger of the two "last" values.
    - The first frame after enabling the log has nonsense VBlank entries; very short fills may finish before the system records their start.
    - Enable it while no GPU operation is in progress.
    
    ### A frame-level breakdown
    
    With those three sources you can fill in this table for any frame; it is the starting point for every optimisation decision on this page:
    
    | Column | How |
    | ------ | --- |
    | Logic (CPU) | ticks around your update code |
    | Record (CPU) | `C3D_GetProcessingTime()` |
    | Wait for GPU | ticks around `C3D_FrameBegin` |
    | GPU draw | P3D delta from the perf log, or `C3D_GetDrawingTime()` minus transfers |
    | Transfers / clears | PPF and PSC deltas |
    | Frame interval | ticks between successive `C3D_FrameBegin` returns |
    
    ### Check the measuring code
    
    Write down whether each counter counts attempts, completed work, skipped work or presented frames; inspect every increment site before dividing two counters. Define intervals as disjoint or explicitly nested and do not add a child timer to its parent. A wall-clock interval includes preemption and waits. Publish a complete duration to other threads with appropriate synchronization rather than exposing a transient unsigned subtraction. HGSS found all three pitfalls while diagnosing an Old 3DS port; see the [case study](case-studies/case-study-hgss-port.md#establish-the-bottleneck-and-validate-the-counters) [reported: project worklog, November 2026].
    
    ## Finding the bottleneck
    
    | Observation | Likely bottleneck | Next step |
    | ----------- | ----------------- | --------- |
    | Logic + Record ≈ frame interval, Wait ≈ 0 | **CPU** | Profile the CPU work by subsystem; see [CPU-side habits](#cpu-side-habits) and [draw-call costs](PICA200/gpu-rendering.md#what-a-draw-call-costs) |
    | Wait for GPU large, GPU draw ≈ frame interval | **GPU** | Fewer pixels (smaller targets, less overdraw, no stereo when the slider is off), fewer fragments with expensive features, smaller textures — [GPU rendering](PICA200/gpu-rendering.md) |
    | Both CPU and GPU well under budget, but frames still missed | **Synchronisation** — the two take turns instead of overlapping, or you wait for V-blank at the wrong moment | Restructure the frame loop ([frame pacing](PICA200/frame-pacing.md#overlapping-cpu-and-gpu)) |
    | Occasional single long frames | **Hitches**: file reads, texture uploads, allocation, a full cache flush, a system service call | Log the worst frames with their stage breakdown; move loading off the frame thread |
    | Audio crackles while frames look fine | Audio refill thread starved or blocked | [Audio](#audio-deadlines) |
    
    Do not optimise a stage that is not on the critical path: halving GPU time does nothing for a CPU-bound frame.
    
    ## Frame pacing
    
    Structure the loop so game logic runs *before* `C3D_FrameBegin` (overlapping the GPU's work on the previous frame), lock to 60 or a deliberate 30 with `C3D_FRAME_SYNCDRAW` and `C3D_FrameRate`, and run the simulation on a fixed time step independent of rendering. Details, loop code and missed-frame detection: [frame pacing](PICA200/frame-pacing.md).
    
    ## Using the second core
    
    On an Old 3DS a program can borrow a time slice of the system core for one thread after `APT_SetAppCpuTimeLimit`. **New 3DS:** a third core is fully available to `.3dsx` homebrew. Large core-1 shares slow down the system modules your program calls (a third party measured `DSP_FlushDataCache` at 1–5 ms with an 80% share). Placement, priorities, synchronisation and a worker pattern: [threading](threading.md).
    
    ## Audio deadlines
    
    The DSP plays whatever is queued and stops when the queue runs dry. Keep enough buffered audio to cover your worst frame, refill from a thread that never waits for the GPU, and flush each wave buffer before queueing it: [audio](audio.md).
    
    ## CPU-side habits
    
    The ARM11 is an ARMv6K core with no NEON, no hardware integer divide, half-rate double-precision multiplies and 16 KB L1 caches (no L2 on Old 3DS). Use `float`, avoid division and random memory access in hot loops, check the generated code, and use the hardware performance counters to find cache misses and mispredicted branches: [CPU optimisation](cpu-optimization.md).
    
    ## Real-hardware profiling cookbook
    
    A checklist for getting numbers from an Old 3DS that someone else can trust. Every recipe here is a method. The benchmark programs use them and have hardware results ([benchmark README](../benchmarks/README.md#hardware-runs)), but the helper code on this page has not been run, and no figure on this page is `[measured]` except where it links to those results. Record the results in the format on [testing on hardware](testing-on-hardware.md#benchmark-record-format).
    
    ### Reliable CPU timing
    
    - Time with `svcGetSystemTick()` around *blocks* of work, never around a few instructions (the call is a system call). Keep raw `u64` ticks; convert once at the end.
    - Repeat a measured block enough that it lasts at least ~1 ms (about 270,000 ticks), and take the **median** of 15 or more repeats, plus the maximum. The first repeat is cold (caches, page tables); keep it as its own number.
    - Make the compiler unable to delete the work (consume results through a `volatile` or a checksum).
    - For cycle-level questions (cache misses, mispredicted branches) use the CPU's performance counters ([CPU optimisation](cpu-optimization.md#hardware-performance-counters)); whether they work from a `.3dsx` is an open question, so have a ticks-only fallback. The counters are per core and also count kernel and interrupt time.
    - Pin the thing you measure: one core, one thread, no other homebrew running, debugger detached, game plugins off.
    
    ### Reliable GPU timing
    
    - `C3D_GetDrawingTime()` and the GSP performance log give GPU-side durations ([above](#per-operation-gpu-times-the-gsp-performance-log)). Neither separates GPU execution from the latency of the completion interrupt reaching your process; treat small differences as noise until a baseline run shows the spread.
    - GPU work is asynchronous: timing the CPU call that *submits* a draw tells you about recording, not rendering. To time the GPU, measure frame interval with the CPU deliberately idle (an empty logic loop) so the GPU is the bottleneck, then vary only the GPU workload.
    - Isolate by elimination: render at a smaller target, skip the draw, or draw the same geometry twice, and see which change moves the frame interval ([finding the bottleneck](#finding-the-bottleneck)).
    
    ### Work time versus waiting for V-blank
    
    A frame that "takes 16.7 ms" because it waited 12 ms for V-blank has 12 ms of headroom. Log both: **work** = logic + record + `C3D_FrameEnd` ticks; **wait** = ticks inside `C3D_FrameBegin` (or `gspWaitForVBlank`). Report work time and the frame interval separately; the ratio tells you the headroom. Never conclude anything from a vsynced frame rate alone: 60 fps is the ceiling, so an optimisation shows only in work time until the frame goes over budget ([frame pacing](PICA200/frame-pacing.md#detecting-missed-frames)).
    
    ### Frame-time distribution and missed frames
    
    Record the interval between successive frame starts for a fixed scene (1,800 frames is 30 s at 60 fps), sort once at the end, and report p50, p95, p99, worst and the count of missed refresh slots. This helper compiles with the wiki's `bench.h` (it uses the same `CPU_TICKS_PER_USEC` and `bench_line`):
    
    ```c
    #include <3ds.h>
    #include <stdlib.h>
    #include "bench.h"
    
    #define NFRAMES 1800                 /* 30 s at 60 fps; no allocation in the loop */
    static u64 interval[NFRAMES];        /* ticks between successive frame starts */
    
    static int cmp_u64(const void *a, const void *b)
    {
        u64 x = *(const u64 *)a, y = *(const u64 *)b;
        return (x > y) - (x < y);
    }
    
    static unsigned frames_seen;
    static u64 last_start;
    
    /* Call once per frame at the same point (e.g. right after C3D_FrameBegin returns). */
    static void frame_mark(void)
    {
        u64 now = svcGetSystemTick();
        if (frames_seen > 0 && frames_seen <= NFRAMES) interval[frames_seen - 1] = now - last_start;
        last_start = now;
        frames_seen++;
    }
    
    /* After the run: one machine-readable line. A slot is one nominal refresh period;
       an interval of k slots counts k-1 missed. */
    static void frame_report(const char *program, const char *variant, u64 nominal_ticks)
    {
        unsigned n = frames_seen > NFRAMES ? NFRAMES : (frames_seen ? frames_seen - 1 : 0);
        if (n == 0) return;
        unsigned long missed = 0;
        u64 worst = 0;
        for (unsigned i = 0; i < n; i++) {
            u64 slots = (interval[i] + nominal_ticks / 2) / nominal_ticks;
            if (slots > 1) missed += (unsigned long)(slots - 1);
            if (interval[i] > worst) worst = interval[i];
        }
        qsort(interval, n, sizeof(u64), cmp_u64);
        bench_line("note,pct,%s,%s,%u,%.2f,%.2f,%.2f,%.2f,%lu", program, variant, n,
                   interval[n / 2] / CPU_TICKS_PER_USEC,
                   interval[(n * 95) / 100] / CPU_TICKS_PER_USEC,
                   interval[(n * 99) / 100] / CPU_TICKS_PER_USEC,
                   worst / CPU_TICKS_PER_USEC, missed);
    }
    /* nominal_ticks for 60 fps: (u64)(SYSCLOCK_ARM11 / 60) = 4,468,530. */
    ```
    
    Output line: `note,pct,<program>,<variant>,<frames>,<p50_us>,<p95_us>,<p99_us>,<worst_us>,<missed_slots>`. Verification: builds with devkitARM r68 and libctru 2.7.1 (`-Wall -Wextra -O2`, inside a stub `main`); not run on hardware. The nominal period is an assumption: the LCD refresh rate is often quoted as about 59.83 Hz and has not been confirmed here ([frame pacing: open questions](PICA200/frame-pacing.md#open-questions)), so check how many "missed" slots an idle loop reports before trusting the count.
    
    ### Synchronous SD logging causes hitches
    
    Writing a log to the SD card from the frame thread blocks it for as long as the file system service takes. `[reported]` The G-Diffuser postmortem records synchronous log flushes of 26-270 ms on the main thread of an Old 3DS build (09-30 entries), "lots of awkward hitching", and no hitches after moving to an asynchronous log; a later never-blocking log gave a race median of 16.5 ms with 0 race hitches (10-02) ([source](https://hypnotize.works/gdx-3ds/postmortem/native.html), read directly; its game and conditions only). Rules that follow regardless of the figures: accumulate samples in memory during the run and write after it (as `bench.h` does), or hand lines to a low-priority writer thread through a bounded queue that drops rather than blocks, and never `printf` per frame.
    
    ### Counters, command capture and regression tests
    
    - **Counters:** CPU events per core via `svcControlPerformanceCounter` ([CPU optimisation](cpu-optimization.md#hardware-performance-counters)); GPU durations via the GSP performance log (above). Count events per unit of work, not totals.
    - **Command capture:** to see what a frame sent to the GPU, dump the command list and compare counts between variants. The command-list format, a capture hook and a decoder are on [PICA200 command pipeline](PICA200/pica200-command-pipeline.md); this page does not duplicate them. G-Diffuser reports reducing GX entries per frame from 990 to 771 and completion interrupts from 550 to 331 by submitting both eyes together (1P 3D race, Old 3DS, 10-26 entries) `[reported]`; the same page calls some of its other results "suggestive" (two blocks per arm), which is why this wiki records the counts, not a speed-up, from it.
    - **Regression tests:** keep one fixed scene (input script, camera, seed) and run it on every performance-relevant change. A regression test is a stored `note,pct` line plus a stored checksum of the final framebuffer (or a screenshot); fail the change if p99 worsens by more than the run-to-run spread of the stored baseline, or if the checksum differs without an accepted reason.
    - **Reporting:** attach the raw `coffeepot/s<NNN>/<program>.csv`, not a screenshot of averages; label the claim `[measured]` only with console model, firmware and build ([record format](testing-on-hardware.md#benchmark-record-format)).
    
    ## Lessons from ported games
    
    None of the eleven ports reviewed in the [case studies](case-studies/case-study-daedalus-3ds.md) states a frame rate in its source tree, so these are patterns, each with a cost the case study records, not results.
    
    - **Count before you tune.** Daedalus compiles a code trace after it loops 10 times (`gHotTraceThreshold`), caches up to 9,216 fragments, and resets an 8 MB code buffer when it fills; its authors comment that one game is sensitive to the threshold. Add a counter for resets and cache fills before touching code generation ([Daedalus](case-studies/case-study-daedalus-3ds.md)).
    - **Pace once.** One gate that decides "this frame is finished" and feeds video, audio and frameskip is easier to reason about than three ([Daedalus](case-studies/case-study-daedalus-3ds.md), [frame pacing](PICA200/frame-pacing.md)).
    - **Ask for the clock, then record the answer.** `osSetSpeedupEnable(true)` is called unconditionally by Daedalus, Quake2CTR and Vril and only after a New 3DS check by sm64_3ds and OpenLara `[documented]`; no tree records whether Old 3DS timing changed. Time with it on and off ([testing on hardware](testing-on-hardware.md)).
    - **Move per-pixel work to the GPU when the CPU cannot finish the frame.** The Sonic CD FAQ says the CPU-rendered build needs a New 3DS for full speed and the citro2d build runs at full speed on an Old 3DS but loses real-time palettes `[reported]` ([Sonic CD](case-studies/case-study-sonic-cd-3ds.md)). DXX renders every pixel on the CPU and the older SM64 port does all its vertex work on the CPU; neither states a speed ([DXX](case-studies/case-study-dxx-3ds.md), [older SM64](case-studies/case-study-sm64-3ds.md)).
    - **Where the older SM64 port leaves time on the table (hypotheses):** a three-`mov` vertex shader that moves no work to the GPU, a flush on every state change, a second emission for fog, and a texture upload that initialises and swizzles on the CPU each time ([older SM64](case-studies/case-study-sm64-3ds.md)).
    - **Give expensive options a switch.** SRB2 3DS exposes stereo, wide mode, colour format, precache and bottom-backlight settings, and sm64ds-enhanced exposes 60 fps, anti-aliasing and the frame counter in a SELECT menu; each lets you measure what it costs on your own scene ([SRB2](case-studies/case-study-srb2-3ds.md), [SM64 DS](case-studies/case-study-sm64ds-enhanced.md)).
    
    ## Technical details
    
    - `SYSCLOCK_ARM11` = 268,111,856 Hz; `CPU_TICKS_PER_MSEC` = `SYSCLOCK_ARM11 / 1000.0`; **New 3DS:** its clocks are 2× and 3× that ([libctru](../homebrew/libctru.md#clocks-and-the-new-3ds-speed-up)). The system tick does not change rate with the New 3DS clock (libctru defines it from `SYSCLOCK_ARM11`; the claim is read from the headers, not measured on a New 3DS in 804 MHz mode).
    - `C3D_FrameBegin(flags)`: `C3D_FRAME_SYNCDRAW` = bit 0, `C3D_FRAME_NONBLOCK` = bit 1. Returns `false` if already in a frame or (non-blocking) if the GPU is busy.
    - `C3D_FrameRate(fps)` accepts `0 < fps ≤ 60`; implemented as a per-screen accumulator decremented by `fps` on each V-blank, incrementing the frame counter whenever it crosses zero.
    - `GSPGPU_PerfLogEntry` = `{ u32 lastDurationUs; u32 totalDurationUs; }`, one per `GSPGPU_Event` (`PSC0`, `PSC1`, `VBlank0`, `VBlank1`, `PPF`, `P3D`, `DMA`).
    - Core-1 CPU-time resource limit (kernel type 9) values: 0 forbids application threads on core 1; 1–89 is a percentage and enables preemption; values ≥ 10000 exempt a process (applets); 1000 is used internally for system modules [reverse-engineered: Luma3DS `sysmodules/pm/source/reslimit.c`].
    - `svcFlushProcessDataCache(CUR_PROCESS_HANDLE, addr, size)` takes a *virtual* address and is handled by the kernel directly; `GSPGPU_FlushDataCache` and `DSP_FlushDataCache` are IPC requests to the `gsp` and `dsp` system modules [documented: libctru sources].
    
    ## Pitfalls
    
    - **Measuring in an emulator or on a New 3DS for an Old 3DS target.** Neither predicts Old 3DS timing ([testing on hardware](testing-on-hardware.md)).
    - **Reporting averages only.** Always log the worst frame and what it was doing.
    - **Profiling with `printf` in the loop.** The print costs more than what you measure.
    - **"Faster" with no before/after on the same scene, build and console.** Keep the benchmark fixed and change one thing.
    - **Starving the system on core 1.** Symptoms are audio stutter, slow file access or slow GPU flushes, not an obvious CPU hog.
    
    ## Open questions
    
    - Measured on an Old 3DS (system 11.17.0-50U, Luma3DS 13.0.2): CPU write bandwidth to heap versus linear memory, the cost of `C3D_FrameEnd`'s full linear-heap flush (about 90 µs per frame) and of the kernel and GSP flush calls ([memory and cache](memory-and-cache.md#measured-costs-old-3ds)); the draw-call costs ([GPU rendering](PICA200/gpu-rendering.md#what-a-draw-call-costs)); and the I-cache cliff ([CPU optimisation](cpu-optimization.md#icachebench-on-an-old-3ds)). Measured on a New 2DS XL at the default 268 MHz clock too (higher flush costs, dearer I-cache miss, [suite runs](../benchmarks/README.md#suite-runs-old-3ds-and-new-3ds)). Not measured: anything at 804 MHz with L2, VRAM write bandwidth, `GSPGPU_FlushDataCache` latency versus core-1 time limit, benefit of LTO. See the [benchmark suite](../benchmarks/README.md#hardware-runs).
    - The cookbook's helpers (frame-time percentiles, missed-slot count) compile but have not run on any console; the nominal refresh period used for the missed-slot count (16.7 ms) is close to the loop period measured on an Old 3DS, 16.71 ms or 59.83 Hz, with the tile renderer under `C3D_FRAME_SYNCDRAW` ([benchmark README](../benchmarks/README.md#lessons-for-running-and-reading-these-benchmarks)); a New 2DS XL gives the same 16.71 ms. The `[reported]` G-Diffuser figures quoted here are for its own game on an Old 3DS and are not reproduced.
    - The CP15 cycle counter and the system tick advance 1:1 on an Old 3DS `[measured]` ([CPU optimisation](cpu-optimization.md#counters-versus-the-system-tick)); the 3:1 ratio in New 3DS 804 MHz mode is still unmeasured. The tick is the only one this page relies on.
    - The 25 ms slice model of core-1 scheduling comes from untested notes; a hardware measurement (a core-1 thread recording `svcGetSystemTick` gaps at several percentages) would settle it.
    
    ## Sources
    
    - [citro3d](https://github.com/devkitPro/citro3d) — `source/renderqueue.c` (frame begin/end, timers, V-blank counters, frame-rate limiter, linear-heap flush), `include/c3d/renderqueue.h`.
    - [libctru](https://github.com/devkitPro/libctru) — `os.h` (clocks, `TickCounter`), `services/gspgpu.h` (perf log), `thread.h` (cores, priorities), `services/apt.h`, `source/system/syscalls.c` (FPSCR), `source/services/gspgpu.c`, `svc.h`.
    - [Luma3DS](https://github.com/LumaTeam/Luma3DS) — `sysmodules/loader/source/hbldr.c` (3DSX exheader: clock/L2 flags off, core-1 limit 89, core 2 access), `sysmodules/pm/source/reslimit.c` (scheduler and CPU-time notes).
    - [libctru issue #556: Document performance issues of DSP_FlushDataCache](https://github.com/devkitPro/libctru/issues/556) — third-party flush-time measurements versus core-1 time limit.
    - [ARM11 MPCore Processor Technical Reference Manual, DDI 0360E](https://developer.arm.com/documentation/ddi0360/e) — VFP11 throughput and latency (table 19-16, section 16.4), cache organisation.
    - [3ds-examples](https://github.com/devkitPro/3ds-examples) — template Makefile flags.
    - [G-Diffuser: Road to 60, All Over Again](https://hypnotize.works/gdx-3ds/postmortem/native.html) — `[reported]` SD-logging hitches, GX entry counts, scheduling results (read directly; third-party).
    # CPU optimisation on the ARM11
    
    The 3DS runs games on an **[ARM11](../glossary.md#arm11) [MPCore](../glossary.md#mpcore)** at 268 MHz on an Old 3DS — a processor design from 2005 with small caches, no vector unit and no second-level cache. **New 3DS:** up to 804 MHz and an optional 2 MB L2 cache. Code written for a desktop or phone CPU usually runs several times slower than its authors expect. This page explains how the core executes code, which constructs are expensive, how to see what the compiler produced, and how to read the processor's own **[hardware performance counters](../../glossary.md#performance-counters)** to find cache misses and branch mispredictions instead of guessing.
    
    Hardware reference: [CPU](../hardware/cpu.md). Measuring whole frames: [performance](performance.md). Threads and the second core: [threading](threading.md).
    
    ## Before you start
    
    - Checked against ARM's *ARM11 MPCore Processor Technical Reference Manual* (DDI 0360E), libctru 2.7.1 and devkitARM r68 (GCC 16.1). The performance-counter helper below compiles without warnings with those versions; it has **not** been run as written. The counter calls in [`icachebench`](../benchmarks/README.md#icachebench) have run on an Old 3DS ([below](#availability-under-luma3ds)); New 3DS: not run.
    - Measure before and after every change ([testing on hardware](testing-on-hardware.md#running-a-performance-experiment)). The rules of thumb here say where to look, not what you will find.
    
    ## How the core executes code
    
    [documented: DDI 0360E §1.7, §16.4]
    
    - **Eight-stage integer pipeline** (two fetch stages, decode, issue, then shift/ALU/saturate, a three-stage multiply-accumulate pipe, or address/two data-cache stages), with throughput approaching one instruction per cycle. Results that feed the next instruction directly, especially loads, cause stalls; the compiler schedules around this at `-O2`.
    - **Branch prediction**: a 128-entry branch target address cache with static prediction for branches it misses (forward conditional branches predicted not taken, backward taken), and a 3-entry return stack that predicts `BX lr`, `LDM sp!,{..,pc}` and `LDR pc,[sp..]` returns but not `BLX reg` calls ([CPU](../hardware/cpu.md#the-mp11-core-from-arms-manual); details in [ARM11 MPCore internals](../hardware/arm11-mpcore.md#branch-prediction)). Data-dependent branches that go either way at random mispredict often; a mispredicted branch throws away the instructions already fetched.
    - **Caches** per core: 16 KB instruction + 16 KB data, 4-way set-associative, 32-byte lines; the data cache is write-back, write-allocate and non-blocking (hit-under-miss). The Old 3DS has nothing between the L1 cache and FCRAM; the New 3DS adds an optional 2 MB L2 ([CPU](../hardware/cpu.md#clocks-and-core-counts)).
    - **VFP11 floating point**: single-precision add/multiply/multiply-accumulate at one per cycle (8-cycle latency); double multiply and multiply-accumulate at one per two cycles; divide and square root take about 15 (single) and 29 (double) cycles each. No NEON.
    - **Instruction sets**: ARMv6K ARM and Thumb-1 (no Thumb-2), the ARMv6 SIMD instructions (packed 8-/16-bit arithmetic in normal registers, e.g. `uadd8`, `qadd16`, `usad8`), `LDRD`/`STRD`, `PLD` (preload hint), `LDREX`/`STREX` exclusives, **no hardware integer divide**.
    
    ## Expensive constructs and their replacements
    
    | Construct | Why it is slow | Replacement |
    | --------- | -------------- | ----------- |
    | `/` and `%` by a variable | Library call (`__aeabi_idiv`, `__aeabi_uidivmod`), tens of cycles | Divide once outside the loop and multiply by the reciprocal; use shifts and masks for powers of two; keep running counters instead of computing `i % n` |
    | `double` arithmetic, double literals (`0.5`), `sqrt`, `sin` | Half-rate multiply, slow divide/sqrt, conversions; libm `double` functions | `float`, `0.5f`, `sqrtf`, `sinf`; compile with `-Wdouble-promotion` |
    | 64-bit integer division or modulo | Library call (`__aeabi_ldivmod`) | Avoid in hot code; convert ticks to time once per report |
    | Random access over more than 16 KB | L1 misses go to FCRAM (no L2 on Old 3DS) | Smaller data types, structure-of-arrays for hot fields, sort or bucket work by memory locality |
    | Large lookup tables | Each lookup can miss the cache | Measure against computing the value; keep tables small |
    | Unpredictable branches in inner loops | Mispredictions | Branch-free arithmetic (conditional execution, `min`/`max`, masks), or sort data so branches go the same way in runs |
    | Virtual calls / function pointers per element | Indirect branch, no inlining | Batch elements by type and loop per type |
    | `memcpy`/`memset` of small fixed sizes through a pointer | Call overhead | Let the compiler see constant sizes; copy whole structs |
    | `printf`, `malloc`, file I/O in the frame | Unbounded time | Pre-allocate, format and log once per second |
    
    Unaligned access: the MPCore supports unaligned word and halfword loads and stores in hardware when the system enables it (control register `U` bit; DDI 0360E chapter 4), but they cost extra cycles, and `LDRD`/`STRD`/`LDM`/`STM`, exclusive and coprocessor (VFP) accesses that are not aligned raise an alignment Data Abort once `U` is set ([
    ... command output truncated for persistence ...
    t encoding is generally safer than asserting a struct has the expected size. Don't use C bitfields to model serialized or GPU-visible register layouts without proving compiler, ABI, and access semantics.
    
    ## 10. `const`, `volatile`, `restrict`, and atomics
    
    These qualifiers mean different things:
    
    | Qualifier / mechanism | What it says | What it **does not** say |
    | --- | --- | --- |
    | `const` | This access path will not modify the object | Other aliases cannot modify it |
    | `volatile` | Certain accesses are observable to the abstract machine | Access is atomic, synchronized, or coherent with GPU/DMA |
    | `restrict` | Programmer promises restricted aliasing for accesses through that pointer | Compiler will validate the promise |
    | C atomics / locks | Access follows defined synchronization rules, if supported/used correctly | GPU cache management is handled automatically |
    
    **Never use `volatile` as a thread synchronization mechanism.** It does not establish the ordering and visibility guarantees of a lock or atomic operation. For actual threads, use the synchronization API provided by the target environment, and document which thread owns each mutable object. [documented]
    
    `restrict` can help a measured inner loop, but using it when buffers overlap can trigger undefined behavior. Start without it; add it only when its non-aliasing contract is true and tested. [documented]
    
    ## 11. The preprocessor is not a second programming language
    
    Prefer `enum`, `static const` (where a runtime object is acceptable), ordinary functions, and `static inline` to function-like macros.
    
    **Don't:**
    
    ```c
    #define SQUARE(x) ((x) * (x))
    int y = SQUARE(i++);  /* i is modified twice: undefined behavior. */
    ```
    
    **Do:**
    
    ```c
    static inline int square_int(int value) {
        return value * value;
    }
    ```
    
    The function avoids repeated evaluation but **still requires** that the multiplication not overflow `int`.
    
    Use macros for include guards, compile-time feature switches, and cases genuinely requiring token substitution. If a macro is unavoidable, fully parenthesize operands, avoid multiple evaluations, and document side effects. Use `do { ... } while (0)` for multi-statement macros used as a statement.
    
    **Platform switches:** Prefer selecting one translation unit in the build system instead of wrapping every function in `#if defined(__3DS__)` / `__SWITCH__`. Only one place should decide how to build each platform.
    
    ## 12. State, initialization, and globals
    
    Prefer explicit state objects to hidden mutable globals:
    
    ```c
    typedef struct {
        uint32_t frame_number;
        bool paused;
    } game_state;
    
    void game_update(game_state *state, float delta_seconds);
    ```
    
    This supports tests, multiple instances, and reasoning about call order. Avoid a universal `game_context *` passed to every function when each function needs only two fields.
    
    - Define a valid zero/empty state where practical, but do not claim every API object can be initialized with `memset(0)`.
    - Give initialization, reset, and shutdown different names and semantics.
    - Make partial initialization safe to clean up, or explicitly forbid calling cleanup before success.
    - Keep mutable state in the narrowest scope: local → object → module → global, in that order.
    - Static immutable lookup tables are often appropriate; a single mutable global renderer that silently owns everything usually is not.
    
    ## 13. Data-oriented design without sacrificing clarity
    
    A 3DS or Switch game may justify fixed capacities, compact representations, and allocation-free frame loops. None of these requires unreadable code.
    
    ### Capacity is part of the object invariant
    
    ```c
    typedef struct {
        sprite *items;
        size_t count;
        size_t capacity;
    } sprite_list;
    ```
    
    Maintain `count <= capacity` after **every** operation. A function that appends must check capacity before writing, then increment count only after the write succeeds. Choose whether full capacity is an error, a flush point, or a documented truncation; never silently overwrite memory.
    
    ### Distinguish storage policy from game policy
    
    A frame arena can be good for temporary CPU data; persistent textures have different lifetimes. Do not turn every allocation into a generic allocator framework. Document allocator alignment, lifetime, memory class, and cleanup, especially for buffers shared with graphics or system services.
    
    ### Preserve a readable reference implementation
    
    When replacing a plain loop with SIMD, fixed-point arithmetic, lookup tables, or hand-written assembly, keep a clear statement of the original mathematical contract and tests that compare implementations. Hardware-specific fast paths may coexist with a portable, correct path.
    
    ## 14. Separate the game from the hardware
    
    A platform module owns OS calls, graphics/device allocations, filesystem mounts, input polling, cache maintenance, and lifecycle integration. Game code should consume simple contracts.
    
    ```c
    /* platform/video.h */
    #include <stdbool.h>
    
    typedef struct draw_list draw_list;
    typedef struct platform_video platform_video;
    
    bool platform_video_begin_frame(platform_video *video);
    bool platform_video_submit(platform_video *video, const draw_list *list);
    void platform_video_end_frame(platform_video *video);
    ```
    
    The contract must state whether commands are **copied** or **borrowed**, when submitted memory may be reused, what happens on failed submission, and whether calls must occur on one thread. Merely giving the API three tidy functions does not make it safe.
    
    ### 3DS: Old 3DS is the baseline
    
    [documented] libctru targets 3DS ARM11 user-mode code; the reference devkitARM Makefile uses ARMv6K/`mpcore` target flags. The example build system also offers platform graphics libraries and asset conversion steps; keep those in the platform/asset layer rather than in game logic.
    
    - Budget and test against **Old 3DS first**. **New 3DS:** Additional resources may permit a separate fast path, but do not silently rely on them.
    - Prefer simple bounded work, reusable scratch storage, and clear object ownership, especially in render and asset code. **Do not assert this is faster without a workload-specific measurement.**
    - CPU-accessible memory, linear memory, VRAM, GPU-visible data, and service-owned buffers are not interchangeable. Use allocation and transfer APIs according to their stated requirements.
    - A CPU write followed by a GPU or system transfer can require cache maintenance and explicit completion ordering. Document who writes a buffer, who reads it, and when it may be reused. Do not hide transfer synchronization behind a generic `volatile` pointer.
    - Name ownership and lifetime of GPU command lists, staging buffers, tile/palette data, and persistent textures. Record buffer size and alignment alongside the allocation.
    - For timing-sensitive code, test both gameplay and transitions on hardware; smoothness cannot be established from code appearance.
    
    ### Switch: AArch64 is not desktop x86-64
    
    [documented] The libnx example build targets ARMv8-A and tunes for Cortex-A57. Pointers and object sizes in the Switch build differ from 32-bit 3DS assumptions; audit pointer/integer casts, file layouts, serialization, and allocation arithmetic.
    
    - Use devkitA64/libnx APIs for system interfaces rather than expecting a Linux process, desktop signals, or unrestricted POSIX behavior.
    - Device-facing buffers have their own alignment, cache, and lifetime requirements. In libnx, `armDCacheClean` and `armDCacheFlush` are different operations; the latter includes invalidation. Select the operation required by the **consumer and direction of ownership**, not by a rule to "flush everything." [documented]
    - Separate CPU preparation, GPU submission, presentation, and completion. A successful submission is not necessarily permission to reuse the submitted data.
    - Avoid allocations and blocking file I/O in a time-sensitive loop where feasible, but measure the actual impact on Switch hardware before introducing special cases.
    
    ### Shared rule for both consoles
    
    **A buffer is not ready because its pointer exists.** Record these four facts at every CPU↔device boundary:
    
    1. Who owns the storage and which allocator created it?
    2. Who last wrote it, and in what representation?
    3. What visibility/synchronization action is required before the next reader?
    4. What event proves the previous reader is finished so it may be reused or freed?
    
    Do not take the contents of an asynchronously changing buffer as a stable signature without a defined snapshot point. For example, hashing renderer-visible data **after** drawing it does not necessarily describe the data that was actually drawn.
    
    ## 15. Optimize with evidence, not folklore
    
    - State a **baseline** and one target metric: frame time, CPU time per subsystem, number of allocations, memory usage, or bytes transferred.
    - Reproduce on the same console model, build configuration, scene, and workload. Separate Old 3DS and New 3DS observations.
    - Profile the whole frame before rewriting a small loop. Be suspicious of expensive allocations, repeated conversions, redundant work, and poor data access patterns.
    - Prefer algorithmic savings and eliminating work over hand-written assembly.
    - Do not guess that `inline`, `restrict`, bit tricks, smaller integers, `memcpy`, or branch-free code are inherently faster. Evaluate generated code and performance only after correctness tests pass.
    - Never justify undefined behavior as a performance optimization; higher optimization levels can make the resulting failure harder to reproduce.
    - Leave performance-focused deviations with a comment naming the measured workload, target, comparison, and reason the clearer implementation was insufficient.
    
    **Example of a useful optimization comment:**
    
    ```c
    /* Old 3DS, scene benchmark X, release flags Y:
     * contiguous tile walk reduced update time from A to B (measurement link).
     * Keep traversal order; the renderer assumes deterministic tile ordering.
     */
    ```
    
    Replace placeholders with actual measurements before publishing this as a claim. Never invent numbers for documentation.
    
    ## 16. Comments explain constraints and decisions
    
    Write comments for *why* the code does something non-obvious, especially when an apparent simplification would break a contract.
    
    **Poor:**
    
    ```c
    count++;  /* Increment count. */
    ```
    
    **Useful:**
    
    ```c
    /* Preserve source order: replay checksums depend on deterministic traversal. */
    count++;
    ```
    
    Comment on:
    
    - Borrowed pointer lifetimes and thread ownership.
    - Units, valid ranges, alignment, and endianness.
    - Non-obvious hardware requirements, with a source or reproducible test.
    - Error state and cleanup guarantees.
    - Algorithmic invariants, not line-by-line implementation narration.
    
    Avoid decorative banners, giant history comments, TODOs with no owner or issue, and claims that are contradicted by the code. Public functions should document parameters and failure behavior where the signature cannot express them.
    
    ## 17. Tests, warnings, and tooling
    
    ### Compile warnings
    
    As a reasonable **starting set**, add to the project's existing devkitPro flags:
    
    ```make
    CFLAGS += -std=c11 -Wall -Wextra -Wshadow -Wundef \
              -Wstrict-prototypes -Wmissing-prototypes \
              -Wpointer-arith -Wformat=2
    ```
    
    Adapt to the project's C dialect and verify every flag with the **installed** target GCC. `-Wconversion` and `-Wsign-conversion` are valuable during focused audits but can be noisy in legacy code and third-party headers. Do not blindly paste host flags that override devkitPro architecture or linker settings.
    
    - Keep warnings clean for first-party code. Consider `-Werror` in controlled CI builds after the warning set is stable; do not necessarily impose it on vendor libraries.
    - Include each public header as the first include in its implementation to expose missing dependencies.
    - Run `clang-format --dry-run --Werror` (or an equivalent formatting check) on first-party C files; do not run an unpinned formatter blindly over a repository.
    - If available, build **portable modules** with host Clang/GCC under address/undefined-behavior sanitizers and unit tests. Sanitizer success on a host does **not** verify console cache semantics or device API use.
    - Run actual devkitARM and devkitA64 builds. Review their warnings, artifacts, and link maps. Run representative workloads on hardware.
    
    ### What to test
    
    | Area | Tests that find real bugs |
    | --- | --- |
    | Binary reader | Truncated input, missing fields, wrong endian tag, invalid offsets, overflowed lengths |
    | Container | 0, 1, exactly capacity, capacity + 1; growth failure; repeated reset |
    | Lifecycle | Init failure at every allocation, cleanup twice where promised safe, reload after error |
    | Renderer | Empty draw list, max-size draw list, resize, context loss where applicable, delayed completion |
    | Concurrency | Producer/consumer ordering, stop while waiting, asset replacement in flight |
    | Gameplay | Deterministic updates under recorded input, pause/resume, save/load boundaries |
    | Portability | Same format round-trips on host, 3DS and Switch; explicit 32/64-bit assumptions |
    
    Prefer tests around invariants, not tests that merely mirror implementation lines. When fixing a bug, retain a small regression test that fails for the original reason.
    
    ## 18. Review checklist
    
    Before accepting C code, a reviewer should be able to answer **yes** to these questions:
    
    - [ ] Can I tell what every exported function promises and what happens on failure?
    - [ ] Is each pointer's ownership and valid lifetime clear?
    - [ ] Are every buffer length, element count, stride, and capacity expressed with unambiguous units?
    - [ ] Are arithmetic, indexing, and narrowing casts safe for malformed or maximum inputs?
    - [ ] Are partially acquired resources released on every failure path?
    - [ ] Is each global or public symbol necessary?
    - [ ] Is the work divided by responsibility without pointless one-function wrapper modules?
    - [ ] Are comments about constraints and decisions, rather than restating the syntax?
    - [ ] Are external file and network formats decoded without unproven layout assumptions?
    - [ ] Are thread ownership, synchronization, cache visibility, and device completion explicit where needed?
    - [ ] Are target-specific changes isolated and justified by a real platform requirement?
    - [ ] Does the code build warning-free with the selected target toolchain, and are tests appropriate to the claimed support?
    - [ ] If readability was traded for speed, is there a reproducible benchmark showing why?
    
    **Code is not maintainable just because it is formatted.** Maintainability means the next engineer can determine what is valid, what can fail, and what can safely change.
    
    ## Technical details
    
    ### What this guide deliberately does not standardize
    
    This is a guide for *game application C*, not a fork of the Linux kernel conventions, MISRA C certification process, or a single rigid source layout. These choices are intentionally left to each project:
    
    - Whether exported types use `_t`, with the portability caveat above.
    - Whether a subsystem returns its result by value, out parameter, or a richer status type.
    - Whether an opaque object has a heap allocation or caller-supplied storage.
    - Whether source files are grouped by feature or by platform inside each feature.
    - Whether a proven hot path uses fixed-point math, NEON, assembly, or a software renderer.
    
    Document the choice once per project. Do not combine two conventions within the same subsystem without a concrete reason.
    
    ### Verification and known limits
    
    - The library names and target architecture examples were checked against the publicly available upstream libctru, libnx, and example Makefiles listed below on **2026-11-11**. Compiler and library package versions in an individual devkitPro installation may differ.
    - No code fragment in this guide was verified on console hardware; examples of good practices are not performance measurements.
    - This page does not certify adherence to ISO C, CERT C, or MISRA C. Where a formal requirement exists, consult that standard directly and add appropriate tooling.
    - Whether C11 atomics are supported and appropriate for a particular cross-compiled runtime is an **environment-specific question**; a successful parse of `<stdatomic.h>` alone is not proof that a chosen operation is lock-free or appropriate for a GPU/OS boundary.
    
    ## Pitfalls
    
    | Symptom | Likely design problem | Correction |
    | --- | --- | --- |
    | Crashes only in optimized builds | Out-of-bounds access, invalid lifetime, aliasing or overflow assumption | Reproduce with host sanitizers, inspect contracts, then check target code |
    | Works on Switch but not Old 3DS | Hidden 64-bit assumption, resource budget, alignment or platform-specific behavior | Check widths and platform layer; test Old 3DS explicitly |
    | Rarely corrupted textures | CPU/device visibility or premature buffer reuse | Track producer, cache action, submission and completion |
    | Long cleanup chains with leaks | Ownership spread across callers | Consolidate initialization and destruction contracts |
    | Hundreds of tiny forwarding functions | Abstraction by naming rather than responsibility | Remove wrappers that provide no boundary or invariant |
    | Every game system includes platform headers | Device details leak above the platform layer | Narrow the adapter API and move calls down |
    | Fast but impossible to review | Unmeasured tricks and undocumented invariants | Keep baseline, benchmark and record constraints |
    
    ## Open questions
    
    Before making this a *binding* standard for a particular game, record: compiler versions on both consoles; the chosen C dialect; which warnings remain clean; how host tests run; how allocations/GPU buffers are managed; and how the project captures hardware benchmarks. These are project decisions and must not be invented by the wiki.
    
    ## Sources
    
    - [Coffeepot contributor guidance (`AGENTS.md`)](https://github.com/estyseesghosts/coffeepot/blob/main/AGENTS.md) — document organization, evidence labels, Old 3DS baseline, and verification requirements.
    - [devkitPro libctru](https://github.com/devkitPro/libctru) — 3DS user-mode library and devkitARM support.
    - [devkitPro 3DS application Makefile](https://github.com/devkitPro/3ds-examples/blob/master/templates/application/Makefile) — ARMv6K target and baseline compiler flags.
    - [devkitPro libctru synchronization.h](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/synchronization.h) — light locks and synchronization API.
    - [switchbrew Switch application Makefile](https://github.com/switchbrew/switch-examples/blob/master/templates/application/Makefile) — ARMv8-A/Cortex-A57 target and application build flags.
    - [libnx cache API](https://switchbrew.github.io/libnx/cache_8h.html) — cache clean and flush semantics.
    - [libnx mutex implementation](https://github.com/switchbrew/libnx/blob/master/nx/source/kernel/mutex.c) — platform synchronization implementation.
    - [GCC optimization manual](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html) — strict aliasing and optimization constraints.
    - [C `restrict` reference](https://en.cppreference.com/c/language/restrict) — aliasing contract and undefined behavior risks.
    - [SEI CERT C coding standard](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/) — preprocessor, integer, memory, expression, and other secure-coding rules.
    - [Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html) — reference for naming, scope, function structure, and kernel-oriented tradeoffs (not adopted verbatim).
    - [ClangFormat documentation](https://clang.llvm.org/docs/ClangFormat.html) and [style options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html) — formatting automation and configuration.
    #ifndef HGSS_PROFILE_OFF
    /*
     * perf.c: bounded in-memory frame profiler.
     *
     * Every publish records its stage ticks into a 600-publish window; at the
     * end of each window one aggregate (p50/p95/p99/max per stage, publish
     * interval in V-blanks, deadline misses) goes to the log ring, which is
     * written to SD off the hot path. A deadline miss is a publish more than two
     * refreshes after the previous one (the 30 FPS contract).
     *
     * Two intervals are kept. "emulated" counts DS V-blanks between publishes
     * and is 2 by construction under the pacing lock. "real" counts physical
     * V-blanks (C3D_FrameCounter) between submissions: that is what the player
     * sees. A real interval other than 2 is a hitch (a picture held for 3+
     * refreshes, or one shown for a single refresh while catching up); the
     * first hitches of each window are logged with their game frame so a
     * stutter can be located in a scripted run.
     */
    
    #include "diagnostics/perf.h"
    
    #include "diagnostics/log.h"
    
    #include <3ds.h>
    #include <citro3d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    
    #define WINDOW 600u
    #define TICKS_PER_US (SYSCLOCK_ARM11 / 1000000u)
    
    static uint32_t s_samples[KJ_PERF_STAGE_COUNT][WINDOW];
    static uint32_t s_frame_stage[KJ_PERF_STAGE_COUNT];
    static uint32_t s_interval[WINDOW];
    static unsigned s_n;
    static uint64_t s_vblank_start, s_publish_start, s_logic_start;
    static uint32_t s_vblanks_since_publish, s_misses, s_windows;
    static bool s_any_publish;
    static uint32_t s_real_interval[WINDOW];
    static uint32_t s_last_real;
    static uint64_t s_last_submit;
    static uint32_t s_frame_time[WINDOW];
    #define MAX_HITCH_LOG 12u
    static uint32_t s_hitch_frame[MAX_HITCH_LOG], s_hitch_len[MAX_HITCH_LOG];
    static uint32_t s_hitch_logic[MAX_HITCH_LOG], s_hitch_g2d[MAX_HITCH_LOG], s_hitch_g3d[MAX_HITCH_LOG];
    static uint32_t s_hitches, s_skipped;
    uint32_t kj_perf_counts[KJ_COUNT_COUNT];
    static uint32_t s_hitch_counts[MAX_HITCH_LOG][KJ_COUNT_COUNT];
    
    uint64_t kj_perf_now(void)
    {
        return svcGetSystemTick();
    }
    
    void kj_perf_add(enum kj_perf_stage stage, uint64_t ticks)
    {
        s_frame_stage[stage] += (uint32_t)ticks;
    }
    
    void kj_perf_vblank_begin(void)
    {
        s_vblank_start = svcGetSystemTick();
        if (s_logic_start != 0) {
            kj_perf_add(KJ_PERF_LOGIC, s_vblank_start - s_logic_start);
        }
    }
    
    void kj_perf_vblank_end(void)
    {
        const uint64_t now = svcGetSystemTick();
        kj_perf_add(KJ_PERF_VBLANK_WAIT, now - s_vblank_start);
        s_logic_start = now;
        ++s_vblanks_since_publish;
    }
    
    void kj_perf_publish_begin(void)
    {
        s_publish_start = svcGetSystemTick();
        if (s_logic_start != 0) {
            kj_perf_add(KJ_PERF_LOGIC, s_publish_start - s_logic_start);
        }
    }
    
    static int cmp_u32(const void *a, const void *b)
    {
        const uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
        return x < y ? -1 : x > y;
    }
    
    static void report(void)
    {
        static const char *const names[KJ_PERF_STAGE_COUNT] = {
            "logic", "vbl_wait", "g2d", "g3d", "gpu_wait", "submit", "publish",
        };
        static uint32_t sorted[WINDOW];
    
        for (int s = 0; s < KJ_PERF_STAGE_COUNT; ++s) {
            memcpy(sorted, s_samples[s], sizeof(sorted));
            qsort(sorted, WINDOW, sizeof(sorted[0]), cmp_u32);
            kj_log("[perf] %-8s us p50 %5lu p95 %5lu p99 %5lu max %5lu", names[s],
                   (unsigned long)(sorted[WINDOW / 2] / TICKS_PER_US),
                   (unsigned long)(sorted[WINDOW * 95 / 100] / TICKS_PER_US),
                   (unsigned long)(sorted[WINDOW * 99 / 100] / TICKS_PER_US),
                   (unsigned long)(sorted[WINDOW - 1] / TICKS_PER_US));
        }
        memcpy(sorted, s_interval, sizeof(sorted));
        qsort(sorted, WINDOW, sizeof(sorted[0]), cmp_u32);
        kj_log("[perf] window %lu: %u publishes, emulated interval (vblanks) p50 %lu max %lu, misses %lu",
               (unsigned long)++s_windows, WINDOW, (unsigned long)sorted[WINDOW / 2],
               (unsigned long)sorted[WINDOW - 1], (unsigned long)s_misses);
        s_misses = 0;
        {
            uint32_t hist[5] = {0, 0, 0, 0, 0};
            for (unsigned i = 0; i < WINDOW; ++i) {
                const uint32_t v = s_real_interval[i];
                ++hist[v >= 4 ? 4 : v];
            }
            memcpy(sorted, s_frame_time, sizeof(sorted));
            qsort(sorted, WINDOW, sizeof(sorted[0]), cmp_u32);
            kj_log("[frame] real interval (refreshes) 0:%lu 1:%lu 2:%lu 3:%lu 4+:%lu; hitches %lu, "
                   "presents skipped %lu; frame time us p50 %lu p99 %lu max %lu",
                   (unsigned long)hist[0], (unsigned long)hist[1], (unsigned long)hist[2],
                   (unsigned long)hist[3], (unsigned long)hist[4], (unsigned long)s_hitches,
                   (unsigned long)s_skipped, (unsigned long)(sorted[WINDOW / 2] / TICKS_PER_US),
                   (unsigned long)(sorted[WINDOW * 99 / 100] / TICKS_PER_US),
                   (unsigned long)(sorted[WINDOW - 1] / TICKS_PER_US));
            const uint32_t shown = s_hitches < MAX_HITCH_LOG ? s_hitches : MAX_HITCH_LOG;
            if (shown > 0) {
                char line[640];
                int n = snprintf(line, sizeof(line),
                                 "[frame] hitches, game frame(refreshes logic/g2d/g3d ms):");
                for (uint32_t i = 0; i < shown && n > 0 && n < (int)sizeof(line); ++i) {
                    n += snprintf(line + n, sizeof(line) - (size_t)n, " %lu(%lu %lu/%lu/%lu)",
                                  (unsigned long)s_hitch_frame[i], (unsigned long)s_hitch_len[i],
                                  (unsigned long)(s_hitch_logic[i] / TICKS_PER_US / 1000u),
                                  (unsigned long)(s_hitch_g2d[i] / TICKS_PER_US / 1000u),
                                  (unsigned long)(s_hitch_g3d[i] / TICKS_PER_US / 1000u));
                }
                kj_log("%s", line);
                for (uint32_t i = 0; i < shown; ++i) {
                    const uint32_t *k = s_hitch_counts[i];
                    kj_log("[frame]   %lu: tile conv %lu, spans %lu, quads %lu, tex uploads %lu, "
                           "tex bytes compared %lu", (unsigned long)s_hitch_frame[i],
                           (unsigned long)k[KJ_COUNT_TILE_CONV], (unsigned long)k[KJ_COUNT_SPANS],
                           (unsigned long)k[KJ_COUNT_QUADS], (unsigned long)k[KJ_COUNT_TEX_UPLOAD],
                           (unsigned long)k[KJ_COUNT_TEX_BYTES]);
                }
            }
            s_hitches = 0;
            s_skipped = 0;
        }
    }
    
    static void note_hitch(uint32_t game_frame, uint32_t refreshes)
    {
        if (s_hitches < MAX_HITCH_LOG) {
            s_hitch_frame[s_hitches] = game_frame;
            s_hitch_len[s_hitches] = refreshes;
            s_hitch_logic[s_hitches] = s_frame_stage[KJ_PERF_LOGIC];
            s_hitch_g2d[s_hitches] = s_frame_stage[KJ_PERF_G2D];
            s_hitch_g3d[s_hitches] = s_frame_stage[KJ_PERF_G3D];
            memcpy(s_hitch_counts[s_hitches], kj_perf_counts, sizeof(kj_perf_counts));
        }
        ++s_hitches;
    }
    
    void kj_perf_present_skipped(uint32_t game_frame)
    {
        ++s_skipped;
        note_hitch(game_frame, 0);
    }
    
    void kj_perf_publish_end(uint32_t game_frame)
    {
        const uint64_t now = svcGetSystemTick();
        const uint32_t real = C3D_FrameCounter(0);
        const uint32_t real_interval = s_any_publish ? real - s_last_real : 2u;
        s_real_interval[s_n] = real_interval;
        s_frame_time[s_n] = s_any_publish ? (uint32_t)(now - s_last_submit) : 0u;
        s_last_real = real;
        s_last_submit = now;
        if (s_any_publish && real_interval != 2u) {
            note_hitch(game_frame, real_interval);
        }
        kj_perf_add(KJ_PERF_PUBLISH, now - s_publish_start);
        /* kj_g3d_draw runs inside the screen draw: report g2d exclusive of it,
         * so logic + g2d + g3d + gpu_wait + submit is not double-counted. */
        {
            uint32_t *g2d = &s_frame_stage[KJ_PERF_G2D];
            const uint32_t g3d = s_frame_stage[KJ_PERF_G3D];
            *g2d = *g2d > g3d ? *g2d - g3d : 0u;
        }
        memset(kj_perf_counts, 0, sizeof(kj_perf_counts));
        for (int s = 0; s < KJ_PERF_STAGE_COUNT; ++s) {
            s_samples[s][s_n] = s_frame_stage[s];
            s_frame_stage[s] = 0;
        }
        s_interval[s_n] = s_vblanks_since_publish;
        if (s_any_publish && s_vblanks_since_publish > 2) {
            ++s_misses;
        }
        s_any_publish = true;
        s_vblanks_since_publish = 0;
        s_logic_start = now;
        if (++s_n == WINDOW) {
            s_n = 0;
            report();
        }
    }
    
    #endif
    /*
     * log.c: the program's log (3DS side).
     *
     * Lines go into a fixed in-memory ring under a lock and are written to the
     * SD card by a low-priority thread, never synchronously from a frame
     * (kohjoh.md section 4.2: logs are never synchronous in a frame). A full ring
     * drops lines and counts them; the count is written with the next flush.
     */
    
    #include "diagnostics/log.h"
    
    #include "runtime/kj_bridge.h"
    
    #include <3ds.h>
    #include <stdarg.h>
    #include <stdio.h>
    #include <string.h>
    
    #define RING_BYTES (64 * 1024)
    #define LINE_MAX_BYTES 512
    
    static LightLock s_lock;
    static LightEvent s_wake;
    static char s_ring[RING_BYTES];
    static size_t s_head; /* write position */
    static size_t s_tail; /* flush position */
    static uint32_t s_dropped;
    static FILE *s_file;
    static Thread s_thread;
    static volatile bool s_running;
    static bool s_ready;
    static u64 s_start_tick;
    
    static size_t ring_used(void)
    {
        return (s_head - s_tail) & (RING_BYTES - 1);
    }
    
    static void append(const char *text, size_t len)
    {
        if (ring_used() + len + 1 >= RING_BYTES) {
            ++s_dropped;
            return;
        }
        for (size_t i = 0; i < len; ++i) {
            s_ring[s_head] = text[i];
            s_head = (s_head + 1) & (RING_BYTES - 1);
        }
    }
    
    static void flush_ring(void)
    {
        static char chunk[RING_BYTES];
        size_t n = 0;
        uint32_t dropped;
    
        LightLock_Lock(&s_lock);
        while (s_tail != s_head) {
            chunk[n++] = s_ring[s_tail];
            s_tail = (s_tail + 1) & (RING_BYTES - 1);
        }
        dropped = s_dropped;
        s_dropped = 0;
        LightLock_Unlock(&s_lock);
    
        if (s_file == NULL) {
            return;
        }
        if (n > 0) {
            fwrite(chunk, 1, n, s_file);
        }
        if (dropped > 0) {
            fprintf(s_file, "[log] %lu lines dropped (ring full)\n", (unsigned long)dropped);
        }
        fflush(s_file);
    }
    
    static void log_thread(void *arg)
    {
        (void)arg;
        while (s_running) {
            LightEvent_WaitTimeout(&s_wake, 500000000LL);
            flush_ring();
        }
        flush_ring();
    }
    
    void kj_log_init(const char *path)
    {
        LightLock_Init(&s_lock);
        LightEvent_Init(&s_wake, RESET_ONESHOT);
        s_start_tick = svcGetSystemTick();
        s_file = path != NULL ? fopen(path, "a") : NULL;
        if (s_file != NULL) {
            fputs("\n--- new run ---\n", s_file);
            fflush(s_file);
        }
        s_ready = true;
        s_running = true;
        /* Lowest application priority: it runs while the game waits for V-blank. */
        s_thread = threadCreate(log_thread, NULL, 16 * 1024, 0x3F, 0, false);
    }
    
    void kj_log_shutdown(void)
    {
        if (s_thread != NULL) {
            s_running = false;
            LightEvent_Signal(&s_wake);
            threadJoin(s_thread, U64_MAX);
            threadFree(s_thread);
            s_thread = NULL;
        }
        flush_ring();
        if (s_file != NULL) {
            fclose(s_file);
            s_file = NULL;
        }
    }
    
    void kj_log_flush_now(void)
    {
        flush_ring();
    }
    
    void kj_logv(const char *fmt, va_list args)
    {
        char line[LINE_MAX_BYTES];
        const u64 ms = (svcGetSystemTick() - s_start_tick) / (SYSCLOCK_ARM11 / 1000);
        int n = snprintf(line, sizeof(line), "%6lu.%03lu ", (unsigned long)(ms / 1000),
                         (unsigned long)(ms % 1000));
        if (n < 0) {
            return;
        }
        int m = vsnprintf(line + n, sizeof(line) - (size_t)n - 1, fmt, args);
        if (m < 0) {
            return;
        }
        size_t len = (size_t)n + (size_t)m;
        if (len > sizeof(line) - 2) {
            len = sizeof(line) - 2;
        }
        line[len++] = '\n';
        if (!s_ready) {
            return;
        }
        LightLock_Lock(&s_lock);
        append(line, len);
        LightLock_Unlock(&s_lock);
    }
    
    void kj_log(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        kj_logv(fmt, args);
        va_end(args);
    }
    
    /* The compat layer's logger (strong; overrides its weak stderr default). */
    void NitroPort_Log(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        kj_logv(fmt, args);
        va_end(args);
    }
    
    void kj_ctr_log(const char *line)
    {
        kj_log("%s", line);
    }
    ✓ • 667ms

## Activity

    $ Get-Content private-gitignore/o3ds-log-3/heartgold.log | Select-String 'new run|build|calib|\[perf\] logic|\[ds\]|g3sample|gputime|sndprof|dlcov|dlwhy'; Get-Content src/runtime/kj_frame.c | Select-Object -Last 210; rg -n 'g3sample|sample|sSync|GetSystemTick' src/runtime/kj_g3.c src/compat/nitro/src/port_timer.c src/compat/nitro/src/port_os.c
    
         1.677 [calib] 1M-iteration loop: 3081616 ticks (3.08 ticks/iteration)
        11.051 [sndprof] render_calls 600 render_frames 307200 stretches 2399 sndticks 1799 mix_ticks 296770212 tick_ticks
    26146830 mix_calls 2399 mix_frames 307200 active_ch 18151 pcm8_ch 0 pcm16_ch 0 adpcm_ch 18151 psg_ch 0 cap_inst 0
    dep_ch 0
        20.425 [sndprof] render_calls 1200 render_frames 614400 stretches 4799 sndticks 3599 mix_ticks 762310492
    tick_ticks 59418688 mix_calls 4799 mix_frames 614400 active_ch 45658 pcm8_ch 273 pcm16_ch 0 adpcm_ch 45385 psg_ch 0
    cap_inst 0 dep_ch 0
        29.804 [sndprof] render_calls 1800 render_frames 921600 stretches 7190 sndticks 5398 mix_ticks 1153698818
    tick_ticks 90387628 mix_calls 7190 mix_frames 921600 active_ch 68743 pcm8_ch 273 pcm16_ch 0 adpcm_ch 68470 psg_ch 0
    cap_inst 0 dep_ch 0
        39.176 [sndprof] render_calls 2400 render_frames 1228800 stretches 9590 sndticks 7198 mix_ticks 1625617146
    tick_ticks 125573520 mix_calls 9590 mix_frames 1228800 active_ch 97424 pcm8_ch 273 pcm16_ch 0 adpcm_ch 97151 psg_ch 0
    cap_inst 0 dep_ch 0
        41.980 [perf] logic    us p50   760 p95 146739 p99 149322 max 465824
        41.988 [dlwhy] batch-in-block 0 shared-verts 6 straddle/drops 0 lit+matrix 8 swap 0 open-prim 0 flushes 0
        41.988 [dlcov] bypass: small 694342 split 2412098 open-begin 0 (begin first 0)
        41.988 [dlcov] words: short 3106440 new 50444 changed 128124 never 995280 hit 10688530 keymiss 942163
        41.988 [ds] per game frame (804 game frames / 600 pictures): g3 fifo calls 398 (19789 fifo words, 67 direct words,
    2808 cmds), timer sync 45 us (4 calls), sound pump 5 us, hblank 396 us
        41.989 [ds] display lists: 19662 replayed, 29712 decoded, 1040 uncacheable, 500 KiB retained
        48.553 [sndprof] render_calls 3000 render_frames 1536000 stretches 11980 sndticks 8997 mix_ticks 2086780894
    tick_ticks 159232814 mix_calls 11980 mix_frames 1536000 active_ch 124836 pcm8_ch 273 pcm16_ch 0 adpcm_ch 124563 psg_ch
    0 cap_inst 0 dep_ch 0
        57.929 [sndprof] render_calls 3600 render_frames 1843200 stretches 14380 sndticks 10797 mix_ticks 2531816934
    tick_ticks 192691696 mix_calls 14380 mix_frames 1843200 active_ch 151658 pcm8_ch 273 pcm16_ch 0 adpcm_ch 151385 psg_ch
    0 cap_inst 0 dep_ch 0
        67.301 [sndprof] render_calls 4200 render_frames 2150400 stretches 16780 sndticks 12597 mix_ticks 2963223576
    tick_ticks 225907400 mix_calls 16780 mix_frames 2150400 active_ch 177641 pcm8_ch 273 pcm16_ch 0 adpcm_ch 177368 psg_ch
    0 cap_inst 0 dep_ch 0
        76.677 [sndprof] render_calls 4800 render_frames 2457600 stretches 19171 sndticks 14396 mix_ticks 3441218634
    tick_ticks 261141852 mix_calls 19171 mix_frames 2457600 active_ch 206237 pcm8_ch 273 pcm16_ch 0 adpcm_ch 205964 psg_ch
    0 cap_inst 0 dep_ch 0
        86.054 [sndprof] render_calls 5400 render_frames 2764800 stretches 21571 sndticks 16196 mix_ticks 3898467336
    tick_ticks 294983672 mix_calls 21571 mix_frames 2764800 active_ch 233773 pcm8_ch 273 pcm16_ch 0 adpcm_ch 233500 psg_ch
    0 cap_inst 0 dep_ch 0
        95.425 [sndprof] render_calls 6000 render_frames 3072000 stretches 23961 sndticks 17995 mix_ticks 4337891930
    tick_ticks 328055614 mix_calls 23961 mix_frames 3072000 active_ch 260149 pcm8_ch 273 pcm16_ch 0 adpcm_ch 259876 psg_ch
    0 cap_inst 0 dep_ch 0
       104.802 [sndprof] render_calls 6600 render_frames 3379200 stretches 26361 sndticks 19795 mix_ticks 4776130928
    tick_ticks 361698722 mix_calls 26361 mix_frames 3379200 active_ch 286501 pcm8_ch 273 pcm16_ch 0 adpcm_ch 286228 psg_ch
    0 cap_inst 0 dep_ch 0
       114.179 [sndprof] render_calls 7200 render_frames 3686400 stretches 28761 sndticks 21595 mix_ticks 5251532038
    tick_ticks 396508452 mix_calls 28761 mix_frames 3686400 active_ch 315072 pcm8_ch 273 pcm16_ch 0 adpcm_ch 314799 psg_ch
    0 cap_inst 0 dep_ch 0
       123.550 [sndprof] render_calls 7800 render_frames 3993600 stretches 31152 sndticks 23394 mix_ticks 5707138932
    tick_ticks 430121994 mix_calls 31152 mix_frames 3993600 active_ch 342434 pcm8_ch 273 pcm16_ch 0 adpcm_ch 342161 psg_ch
    0 cap_inst 0 dep_ch 0
       132.201 [perf] logic    us p50 143918 p95 148451 p99 150099 max 158545
       132.212 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
       132.212 [dlcov] bypass: small 2329216 split 9755940 open-begin 0 (begin first 0)
       132.213 [dlcov] words: short 12085156 new 0 changed 232452 never 0 hit 44885961 keymiss 3034371
       132.213 [ds] per game frame (1174 game frames / 600 pictures): g3 fifo calls 979 (51310 fifo words, 55 direct
    words, 6967 cmds), timer sync 41 us (4 calls), sound pump 11 us, hblank 1077 us
       132.213 [ds] display lists: 82226 replayed, 103312 decoded, 0 uncacheable, 500 KiB retained
       132.926 [sndprof] render_calls 8400 render_frames 4300800 stretches 33552 sndticks 25194 mix_ticks 6114692368
    tick_ticks 461770998 mix_calls 33552 mix_frames 4300800 active_ch 366850 pcm8_ch 273 pcm16_ch 0 adpcm_ch 366577 psg_ch
    0 cap_inst 0 dep_ch 0
       142.313 [sndprof] render_calls 9000 render_frames 4608000 stretches 35942 sndticks 26993 mix_ticks 6588314632
    tick_ticks 496761198 mix_calls 35942 mix_frames 4608000 active_ch 395326 pcm8_ch 273 pcm16_ch 0 adpcm_ch 395053 psg_ch
    0 cap_inst 0 dep_ch 0
       151.684 [sndprof] render_calls 9600 render_frames 4915200 stretches 38342 sndticks 28793 mix_ticks 7056070150
    tick_ticks 531298882 mix_calls 38342 mix_frames 4915200 active_ch 423471 pcm8_ch 273 pcm16_ch 0 adpcm_ch 423198 psg_ch
    0 cap_inst 0 dep_ch 0
       161.062 [sndprof] render_calls 10200 render_frames 5222400 stretches 40733 sndticks 30592 mix_ticks 7523534592
    tick_ticks 566154840 mix_calls 40733 mix_frames 5222400 active_ch 451711 pcm8_ch 273 pcm16_ch 0 adpcm_ch 451438 psg_ch
    0 cap_inst 0 dep_ch 0
       170.439 [sndprof] render_calls 10800 render_frames 5529600 stretches 43133 sndticks 32392 mix_ticks 7949979420
    tick_ticks 599914236 mix_calls 43133 mix_frames 5529600 active_ch 477433 pcm8_ch 273 pcm16_ch 0 adpcm_ch 477160 psg_ch
    0 cap_inst 0 dep_ch 0
       179.810 [sndprof] render_calls 11400 render_frames 5836800 stretches 45533 sndticks 34192 mix_ticks 8437294484
    tick_ticks 636687938 mix_calls 45533 mix_frames 5836800 active_ch 507185 pcm8_ch 273 pcm16_ch 0 adpcm_ch 506912 psg_ch
    0 cap_inst 0 dep_ch 0
       189.186 [sndprof] render_calls 12000 render_frames 6144000 stretches 47923 sndticks 35991 mix_ticks 8931858030
    tick_ticks 672550414 mix_calls 47923 mix_frames 6144000 active_ch 536868 pcm8_ch 273 pcm16_ch 0 adpcm_ch 536181 psg_ch
    414 cap_inst 0 dep_ch 0
       192.602 [perf] logic    us p50 96076 p95 147696 p99 151202 max 154876
       192.613 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
       192.613 [dlcov] bypass: small 1544268 split 6357379 open-begin 0 (begin first 0)
       192.613 [dlcov] words: short 7901647 new 28397 changed 200988 never 0 hit 27482964 keymiss 2210517
       192.614 [ds] per game frame (1023 game frames / 600 pictures): g3 fifo calls 707 (36974 fifo words, 55 direct
    words, 5151 cmds), timer sync 45 us (4 calls), sound pump 8 us, hblank 1099 us
       192.614 [ds] display lists: 56509 replayed, 74854 decoded, 0 uncacheable, 816 KiB retained
       198.563 [sndprof] render_calls 12600 render_frames 6451200 stretches 50323 sndticks 37791 mix_ticks 9413654828
    tick_ticks 707723168 mix_calls 50323 mix_frames 6451200 active_ch 565848 pcm8_ch 273 pcm16_ch 0 adpcm_ch 564620 psg_ch
    955 cap_inst 0 dep_ch 0
       207.935 [sndprof] render_calls 13200 render_frames 6758400 stretches 52714 sndticks 39590 mix_ticks 9895781462
    tick_ticks 743285468 mix_calls 52714 mix_frames 6758400 active_ch 594090 pcm8_ch 273 pcm16_ch 0 adpcm_ch 591686 psg_ch
    2131 cap_inst 0 dep_ch 0
       217.312 [sndprof] render_calls 13800 render_frames 7065600 stretches 55114 sndticks 41390 mix_ticks 10428890638
    tick_ticks 784248184 mix_calls 55114 mix_frames 7065600 active_ch 624860 pcm8_ch 512 pcm16_ch 0 adpcm_ch 621104 psg_ch
    3244 cap_inst 0 dep_ch 0
    --- new run ---
    --- new run ---
         1.709 [calib] 1M-iteration loop: 3082060 ticks (3.08 ticks/iteration)
        11.083 [sndprof] render_calls 600 render_frames 307200 stretches 2399 sndticks 1799 mix_ticks 297090070 tick_ticks
    27594074 mix_calls 2399 mix_frames 307200 active_ch 18151 pcm8_ch 0 pcm16_ch 0 adpcm_ch 18151 psg_ch 0 cap_inst 0
    dep_ch 0
        20.460 [sndprof] render_calls 1200 render_frames 614400 stretches 4799 sndticks 3599 mix_ticks 775727886
    tick_ticks 63296718 mix_calls 4799 mix_frames 614400 active_ch 46942 pcm8_ch 0 pcm16_ch 0 adpcm_ch 46942 psg_ch 0
    cap_inst 0 dep_ch 0
        21.913 [perf] logic    us p50   689 p95  2533 p99  4202 max 41685
        21.921 [g3sample] every 64th FIFO call: samples 0 total_ticks 0 decode_ticks 0 replay_ticks 0; remainder includes
    lookup/recording and probe overhead
        21.921 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
        21.922 [dlcov] bypass: small 0 split 0 open-begin 0 (begin first 0)
        21.922 [dlcov] words: short 0 new 0 changed 0 never 0 hit 0 keymiss 0
        21.922 [ds] per game frame (605 game frames / 600 pictures): g3 fifo calls 0 (0 fifo words, 2 direct words, 2
    cmds), timer sync 66 us (4 calls), sound pump 5 us, hblank 10 us
        21.922 [ds] display lists: 0 replayed, 0 decoded, 0 uncacheable, 0 KiB retained
        21.922 [gputime] 600 pictures: GSP us/picture P3D 587755 PPF 160 PSC 130 (log on); C3D drawing ms avg 1.93 max
    6.13; cmdbuf use max 1%
        29.837 [sndprof] render_calls 1800 render_frames 921600 stretches 7190 sndticks 5398 mix_ticks 1250933708
    tick_ticks 100282400 mix_calls 7190 mix_frames 921600 active_ch 75479 pcm8_ch 0 pcm16_ch 0 adpcm_ch 75479 psg_ch 0
    cap_inst 0 dep_ch 0
        39.208 [sndprof] render_calls 2400 render_frames 1228800 stretches 9590 sndticks 7198 mix_ticks 1723711360
    tick_ticks 137177758 mix_calls 9590 mix_frames 1228800 active_ch 103961 pcm8_ch 0 pcm16_ch 0 adpcm_ch 103961 psg_ch 0
    cap_inst 0 dep_ch 0
        42.306 [perf] logic    us p50  1300 p95 12303 p99 24974 max 106282
        42.314 [g3sample] every 64th FIFO call: samples 510 total_ticks 8102410 decode_ticks 2759756 replay_ticks 2447592;
    remainder includes lookup/recording and probe overhead
        42.314 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
        42.314 [dlcov] bypass: small 113027 split 121610 open-begin 0 (begin first 0)
        42.314 [dlcov] words: short 234637 new 30198 changed 7614 never 0 hit 2090088 keymiss 97359
        42.318 [ds] per game frame (610 game frames / 600 pictures): g3 fifo calls 53 (4032 fifo words, 23 direct words,
    402 cmds), timer sync 71 us (4 calls), sound pump 11 us, hblank 226 us
        42.318 [ds] display lists: 6280 replayed, 1161 decoded, 0 uncacheable, 335 KiB retained
        42.318 [gputime] 600 pictures: GSP us/picture P3D 3200 PPF 564 PSC 493 (log on); C3D drawing ms avg 4.79 max
    40.01; cmdbuf use max 48%
        48.585 [sndprof] render_calls 3000 render_frames 1536000 stretches 11980 sndticks 8997 mix_ticks 2196615848
    tick_ticks 173828644 mix_calls 11980 mix_frames 1536000 active_ch 132330 pcm8_ch 0 pcm16_ch 0 adpcm_ch 132330 psg_ch 0
    cap_inst 0 dep_ch 0
        57.962 [sndprof] render_calls 3600 render_frames 1843200 stretches 14380 sndticks 10797 mix_ticks 2671007430
    tick_ticks 211315216 mix_calls 14380 mix_frames 1843200 active_ch 161486 pcm8_ch 66 pcm16_ch 0 adpcm_ch 161420 psg_ch
    0 cap_inst 0 dep_ch 0
        67.333 [sndprof] render_calls 4200 render_frames 2150400 stretches 16780 sndticks 12597 mix_ticks 3062489412
    tick_ticks 242128036 mix_calls 16780 mix_frames 2150400 active_ch 183959 pcm8_ch 273 pcm16_ch 0 adpcm_ch 183686 psg_ch
    0 cap_inst 0 dep_ch 0
        69.597 [perf] logic    us p50   861 p95 78793 p99 82770 max 457302
        69.609 [g3sample] every 64th FIFO call: samples 3242 total_ticks 31228160 decode_ticks 14537444 replay_ticks
    7395806; remainder includes lookup/recording and probe overhead
        69.609 [dlwhy] batch-in-block 0 shared-verts 6 straddle/drops 0 lit+matrix 8 swap 0 open-prim 0 flushes 0
        69.609 [dlcov] bypass: small 453824 split 1537248 open-begin 0 (begin first 0)
        69.609 [dlcov] words: short 1991072 new 50426 changed 88416 never 727320 hit 6760048 keymiss 620094
        69.609 [ds] per game frame (738 game frames / 600 pictures): g3 fifo calls 281 (13871 fifo words, 55 direct words,
    1965 cmds), timer sync 46 us (4 calls), sound pump 6 us, hblank 620 us
        69.609 [ds] display lists: 12446 replayed, 6890 decoded, 760 uncacheable, 835 KiB retained
        69.610 [gputime] 600 pictures: GSP us/picture P3D 3210 PPF 526 PSC 412 (log on); C3D drawing ms avg 4.80 max 9.97;
    cmdbuf use max 10%
        76.710 [sndprof] render_calls 4800 render_frames 2457600 stretches 19171 sndticks 14396 mix_ticks 3510666616
    tick_ticks 276943452 mix_calls 19171 mix_frames 2457600 active_ch 210747 pcm8_ch 273 pcm16_ch 0 adpcm_ch 210474 psg_ch
    0 cap_inst 0 dep_ch 0
        86.086 [sndprof] render_calls 5400 render_frames 2764800 stretches 21571 sndticks 16196 mix_ticks 3982237814
    tick_ticks 312527268 mix_calls 21571 mix_frames 2764800 active_ch 239119 pcm8_ch 273 pcm16_ch 0 adpcm_ch 238846 psg_ch
    0 cap_inst 0 dep_ch 0
        95.458 [sndprof] render_calls 6000 render_frames 3072000 stretches 23961 sndticks 17995 mix_ticks 4439528280
    tick_ticks 347309042 mix_calls 23961 mix_frames 3072000 active_ch 266585 pcm8_ch 273 pcm16_ch 0 adpcm_ch 266312 psg_ch
    0 cap_inst 0 dep_ch 0
       104.835 [sndprof] render_calls 6600 render_frames 3379200 stretches 26361 sndticks 19795 mix_ticks 4847589822
    tick_ticks 380272486 mix_calls 26361 mix_frames 3379200 active_ch 290988 pcm8_ch 273 pcm16_ch 0 adpcm_ch 290715 psg_ch
    0 cap_inst 0 dep_ch 0
       114.211 [sndprof] render_calls 7200 render_frames 3686400 stretches 28761 sndticks 21595 mix_ticks 5321582634
    tick_ticks 416293780 mix_calls 28761 mix_frames 3686400 active_ch 319679 pcm8_ch 273 pcm16_ch 0 adpcm_ch 319406 psg_ch
    0 cap_inst 0 dep_ch 0
       120.954 [perf] logic    us p50 79778 p95 82883 p99 83633 max 94437
       120.966 [g3sample] every 64th FIFO call: samples 17515 total_ticks 162606690 decode_ticks 73568774 replay_ticks
    40985138; remainder includes lookup/recording and probe overhead
       120.966 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
       120.966 [dlcov] bypass: small 2271680 split 9514950 open-begin 0 (begin first 0)
       120.966 [dlcov] words: short 11786630 new 0 changed 226710 never 0 hit 43778393 keymiss 2958217
       120.966 [ds] per game frame (1145 game frames / 600 pictures): g3 fifo calls 979 (51310 fifo words, 55 direct
    words, 6884 cmds), timer sync 41 us (4 calls), sound pump 6 us, hblank 2556 us
       120.966 [ds] display lists: 80196 replayed, 20564 decoded, 0 uncacheable, 835 KiB retained
       120.967 [gputime] 600 pictures: GSP us/picture P3D 2626 PPF 445 PSC 309 (log on); C3D drawing ms avg 3.99 max 6.92;
    cmdbuf use max 8%
       123.582 [sndprof] render_calls 7800 render_frames 3993600 stretches 31152 sndticks 23394 mix_ticks 5784406252
    tick_ticks 451644580 mix_calls 31152 mix_frames 3993600 active_ch 347325 pcm8_ch 273 pcm16_ch 0 adpcm_ch 347052 psg_ch
    0 cap_inst 0 dep_ch 0
       132.959 [sndprof] render_calls 8400 render_frames 4300800 stretches 33552 sndticks 25194 mix_ticks 6245758548
    tick_ticks 486372186 mix_calls 33552 mix_frames 4300800 active_ch 375237 pcm8_ch 273 pcm16_ch 0 adpcm_ch 374964 psg_ch
    0 cap_inst 0 dep_ch 0
       142.336 [sndprof] render_calls 9000 render_frames 4608000 stretches 35942 sndticks 26993 mix_ticks 6659385702
    tick_ticks 519669684 mix_calls 35942 mix_frames 4608000 active_ch 399850 pcm8_ch 273 pcm16_ch 0 adpcm_ch 399577 psg_ch
    0 cap_inst 0 dep_ch 0
       151.708 [sndprof] render_calls 9600 render_frames 4915200 stretches 38342 sndticks 28793 mix_ticks 7131614334
    tick_ticks 556106096 mix_calls 38342 mix_frames 4915200 active_ch 428517 pcm8_ch 273 pcm16_ch 0 adpcm_ch 428244 psg_ch
    0 cap_inst 0 dep_ch 0
       161.084 [sndprof] render_calls 10200 render_frames 5222400 stretches 40733 sndticks 30592 mix_ticks 7592245874
    tick_ticks 590598226 mix_calls 40733 mix_frames 5222400 active_ch 455947 pcm8_ch 273 pcm16_ch 0 adpcm_ch 455674 psg_ch
    0 cap_inst 0 dep_ch 0
       170.460 [sndprof] render_calls 10800 render_frames 5529600 stretches 43133 sndticks 32392 mix_ticks 8037222722
    tick_ticks 625382966 mix_calls 43133 mix_frames 5529600 active_ch 482722 pcm8_ch 273 pcm16_ch 0 adpcm_ch 482449 psg_ch
    0 cap_inst 0 dep_ch 0
       172.279 [perf] logic    us p50 79630 p95 83049 p99 83745 max 93374
       172.291 [g3sample] every 64th FIFO call: samples 17592 total_ticks 164519660 decode_ticks 72465048 replay_ticks
    37517238; remainder includes lookup/recording and probe overhead
       172.291 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
       172.291 [dlcov] bypass: small 2281600 split 9556500 open-begin 0 (begin first 0)
       172.291 [dlcov] words: short 11838100 new 0 changed 227700 never 0 hit 43967242 keymiss 2973458
       172.291 [ds] per game frame (1150 game frames / 600 pictures): g3 fifo calls 979 (51310 fifo words, 55 direct
    words, 6885 cmds), timer sync 44 us (4 calls), sound pump 5 us, hblank 2570 us
       172.291 [ds] display lists: 80544 replayed, 20656 decoded, 0 uncacheable, 835 KiB retained
       172.292 [gputime] 600 pictures: GSP us/picture P3D 2628 PPF 447 PSC 460 (log on); C3D drawing ms avg 4.02 max 6.93;
    cmdbuf use max 8%
       179.837 [sndprof] render_calls 11400 render_frames 5836800 stretches 45533 sndticks 34192 mix_ticks 8469456136
    tick_ticks 659489396 mix_calls 45533 mix_frames 5836800 active_ch 508766 pcm8_ch 273 pcm16_ch 0 adpcm_ch 508493 psg_ch
    0 cap_inst 0 dep_ch 0
       189.209 [sndprof] render_calls 12000 render_frames 6144000 stretches 47923 sndticks 35991 mix_ticks 8961663668
    tick_ticks 697273990 mix_calls 47923 mix_frames 6144000 active_ch 538365 pcm8_ch 273 pcm16_ch 0 adpcm_ch 538092 psg_ch
    0 cap_inst 0 dep_ch 0
       198.586 [sndprof] render_calls 12600 render_frames 6451200 stretches 50323 sndticks 37791 mix_ticks 9438110062
    tick_ticks 734566448 mix_calls 50323 mix_frames 6451200 active_ch 567143 pcm8_ch 273 pcm16_ch 0 adpcm_ch 566773 psg_ch
    97 cap_inst 0 dep_ch 0
       207.963 [sndprof] render_calls 13200 render_frames 6758400 stretches 52714 sndticks 39590 mix_ticks 9981716174
    tick_ticks 775103910 mix_calls 52714 mix_frames 6758400 active_ch 598039 pcm8_ch 273 pcm16_ch 0 adpcm_ch 596934 psg_ch
    832 cap_inst 0 dep_ch 0
       208.095 [perf] logic    us p50 53332 p95 81520 p99 89381 max 174603
       208.107 [g3sample] every 64th FIFO call: samples 9616 total_ticks 96635870 decode_ticks 40635752 replay_ticks
    26059506; remainder includes lookup/recording and probe overhead
       208.107 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
       208.107 [dlcov] bypass: small 1233658 split 5312696 open-begin 0 (begin first 0)
       208.108 [dlcov] words: short 6546354 new 12334 changed 128070 never 0 hit 23959672 keymiss 1864853
       208.108 [ds] per game frame (926 game frames / 600 pictures): g3 fifo calls 664 (35109 fifo words, 74 direct words,
    4847 cmds), timer sync 37 us (4 calls), sound pump 12 us, hblank 2177 us
       208.108 [ds] display lists: 43792 replayed, 12685 decoded, 0 uncacheable, 969 KiB retained
       208.108 [gputime] 600 pictures: GSP us/picture P3D 2161 PPF 511 PSC 371 (log on); C3D drawing ms avg 3.74 max 9.25;
    cmdbuf use max 9%
       217.334 [sndprof] render_calls 13800 render_frames 7065600 stretches 55114 sndticks 41390 mix_ticks 10514320342
    tick_ticks 815306912 mix_calls 55114 mix_frames 7065600 active_ch 628860 pcm8_ch 575 pcm16_ch 0 adpcm_ch 626925 psg_ch
    1360 cap_inst 0 dep_ch 0
       226.711 [sndprof] render_calls 14400 render_frames 7372800 stretches 57514 sndticks 43190 mix_ticks 11057379236
    tick_ticks 857060498 mix_calls 57514 mix_frames 7372800 active_ch 660882 pcm8_ch 575 pcm16_ch 0 adpcm_ch 658329 psg_ch
    1978 cap_inst 0 dep_ch 0
       228.113 [perf] logic    us p50  1884 p95  8341 p99 16840 max 38685
       228.126 [g3sample] every 64th FIFO call: samples 3 total_ticks 35890 decode_ticks 30560 replay_ticks 0; remainder
    includes lookup/recording and probe overhead
       228.126 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
       228.126 [dlcov] bypass: small 0 split 12896 open-begin 0 (begin first 0)
       228.126 [dlcov] words: short 12896 new 0 changed 0 never 0 hit 0 keymiss 0
       228.126 [ds] per game frame (601 game frames / 600 pictures): g3 fifo calls 0 (21 fifo words, 204 direct words, 109
    cmds), timer sync 41 us (4 calls), sound pump 6 us, hblank 22 us
       228.126 [ds] display lists: 0 replayed, 0 decoded, 0 uncacheable, 969 KiB retained
       228.127 [gputime] 600 pictures: GSP us/picture P3D 1755 PPF 677 PSC 529 (log on); C3D drawing ms avg 3.88 max 8.78;
    cmdbuf use max 1%
       236.087 [sndprof] render_calls 15000 render_frames 7680000 stretches 59905 sndticks 44989 mix_ticks 11604489684
    tick_ticks 896710086 mix_calls 59905 mix_frames 7680000 active_ch 691292 pcm8_ch 765 pcm16_ch 0 adpcm_ch 687495 psg_ch
    3032 cap_inst 0 dep_ch 0
       245.458 [sndprof] render_calls 15600 render_frames 7987200 stretches 62305 sndticks 46789 mix_ticks 12079191282
    tick_ticks 929494388 mix_calls 62305 mix_frames 7987200 active_ch 717259 pcm8_ch 765 pcm16_ch 0 adpcm_ch 713462 psg_ch
    3032 cap_inst 0 dep_ch 0
       250.746 [perf] logic    us p50 24119 p95 58095 p99 63554 max 89853
       250.754 [g3sample] every 64th FIFO call: samples 3616 total_ticks 37751210 decode_ticks 16422398 replay_ticks
    9262226; remainder includes lookup/recording and probe overhead
       250.755 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
       250.758 [dlcov] bypass: small 449261 split 2091624 open-begin 0 (begin first 0)
       250.758 [dlcov] words: short 2540885 new 31594 changed 45108 never 0 hit 8993181 keymiss 933119
       250.758 [ds] per game frame (679 game frames / 600 pictures): g3 fifo calls 340 (18474 fifo words, 63 direct words,
    2699 cmds), timer sync 38 us (4 calls), sound pump 6 us, hblank 1694 us
       250.759 [ds] display lists: 15977 replayed, 5831 decoded, 0 uncacheable, 1324 KiB retained
       250.759 [gputime] 600 pictures: GSP us/picture P3D 1914 PPF 542 PSC 414 (log on); C3D drawing ms avg 3.67 max 9.22;
    cmdbuf use max 5%
       254.835 [sndprof] render_calls 16200 render_frames 8294400 stretches 64695 sndticks 48588 mix_ticks 12554749334
    tick_ticks 964086958 mix_calls 64695 mix_frames 8294400 active_ch 745177 pcm8_ch 765 pcm16_ch 0 adpcm_ch 741186 psg_ch
    3226 cap_inst 0 dep_ch 0
       264.212 [sndprof] render_calls 16800 render_frames 8601600 stretches 67095 sndticks 50388 mix_ticks 13032539168
    tick_ticks 1000891400 mix_calls 67095 mix_frames 8601600 active_ch 773960 pcm8_ch 765 pcm16_ch 0 adpcm_ch 769775
    psg_ch 3420 cap_inst 0 dep_ch 0
       273.129 [perf] logic    us p50 20876 p95 60761 p99 62357 max 71820
       273.141 [g3sample] every 64th FIFO call: samples 4135 total_ticks 46226996 decode_ticks 24720182 replay_ticks
    7870744; remainder includes lookup/recording and probe overhead
       273.141 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
       273.141 [dlcov] bypass: small 664587 split 2578898 open-begin 0 (begin first 0)
       273.141 [dlcov] words: short 3243485 new 2906 changed 141048 never 0 hit 9613868 keymiss 1043720
       273.142 [ds] per game frame (675 game frames / 600 pictures): g3 fifo calls 392 (20807 fifo words, 60 direct words,
    2998 cmds), timer sync 45 us (4 calls), sound pump 11 us, hblank 2447 us
       273.142 [ds] display lists: 25828 replayed, 12046 decoded, 0 uncacheable, 1357 KiB retained
       273.142 [gputime] 600 pictures: GSP us/picture P3D 2083 PPF 482 PSC 319 (log on); C3D drawing ms avg 3.42 max 9.35;
    cmdbuf use max 6%
       273.584 [sndprof] render_calls 17400 render_frames 8908800 stretches 69495 sndticks 52188 mix_ticks 13507815116
    tick_ticks 1037632934 mix_calls 69495 mix_frames 8908800 active_ch 802150 pcm8_ch 765 pcm16_ch 0 adpcm_ch 797254
    psg_ch 4131 cap_inst 0 dep_ch 0
       282.961 [sndprof] render_calls 18000 render_frames 9216000 stretches 71886 sndticks 53987 mix_ticks 14049180150
    tick_ticks 1079895020 mix_calls 71886 mix_frames 9216000 active_ch 833025 pcm8_ch 1003 pcm16_ch 0 adpcm_ch 827007
    psg_ch 5015 cap_inst 0 dep_ch 0
    /*
     * kj_frame.c: the scheduling bridge between the game's DS frame and the
     * 3DS presentation (DS side).
     *
     * One emulated DS V-blank is one physical 3DS V-blank: the game's OS_Halt
     * (NitroPort_Halt -> NitroPort_OnFrame) waits for the 3DS display. HGSS's
     * main loop waits twice per game frame, so the game naturally runs at
     * ~29.9 game frames a second and nothing here doubles or halves it.
     *
     * The frame is published once per game frame, at the point patch 2001
     * marks in NitroMain (after the V-blank callback, the sound update and the
     * vwait task queue): the game is stopped at its frame boundary, so the
     * renderer reads one coherent version of registers, palettes, OAM and VRAM
     * on this thread, and nothing races it. Loops that wait for V-blank without
     * reaching that point (soft-reset and error screens, the few boot frames)
     * are published from the V-blank itself after a short grace period.
     */
    
    #include "kj_bridge.h"
    #include "kj_g3.h"
    
    #include <nitro.h>
    #include <nitro_port.h>
    #include <string.h>
    
    extern unsigned char NitroPort_IoReg[];
    extern unsigned char NitroPort_Pltt[];
    extern unsigned char NitroPort_Oam[];
    extern unsigned char NitroPort_BgVram[];
    extern unsigned char NitroPort_DbBgVram[];
    extern unsigned char NitroPort_ObjVram[];
    extern unsigned char NitroPort_DbObjVram[];
    extern unsigned char NitroPort_LcdcVram[];
    
    void NitroPort_SoundPumpReplies(void);
    extern u64 svcGetSystemTick(void);
    
    static u64 s_pump_ticks, s_hblank_ticks;
    static u32 s_pump_calls, s_hblank_walks;
    static u64 s_sync_calls_base, s_sync_ticks_base;
    
    /* V-blanks without a publish after which the V-blank publishes itself. Two
     * is the main loop's normal cadence; four leaves room for a frame that
     * overran once. */
    #define PUBLISH_GRACE_VBLANKS 4u
    
    static kj_ds_memory s_memory;
    static u32 s_vblanks;
    static u32 s_publishes;
    static u32 s_last_publish_vblank;
    
    static const u32 BANK_LCDC_OFFSET[9] = {
        0x00000, 0x20000, 0x40000, 0x60000, 0x80000, 0x90000, 0x94000, 0x98000, 0xA0000,
    };
    static const u32 BANK_SIZE[9] = {
        0x20000, 0x20000, 0x20000, 0x20000, 0x10000, 0x4000, 0x4000, 0x8000, 0x4000,
    };
    /* VRAMCNT_A..G are 0x240..0x246, H and I 0x248/0x249 (0x247 is WRAMCNT). */
    static const u16 BANK_REG[9] = {
        0x240, 0x241, 0x242, 0x243, 0x244, 0x245, 0x246, 0x248, 0x249,
    };
    
    const kj_ds_memory *kj_ds_get_memory(void)
    {
        if (s_memory.io == NULL) {
            s_memory.io = NitroPort_IoReg;
            s_memory.io_size = HW_IOREG_SIZE;
            s_memory.pltt = NitroPort_Pltt;
            s_memory.oam = NitroPort_Oam;
            s_memory.bg_vram_a = NitroPort_BgVram;
            s_memory.bg_vram_b = NitroPort_DbBgVram;
            s_memory.obj_vram_a = NitroPort_ObjVram;
            s_memory.obj_vram_b = NitroPort_DbObjVram;
            s_memory.lcdc_vram = NitroPort_LcdcVram;
        }
        return &s_memory;
    }
    
    static void decode_banks(kj_vram_bank banks[9])
    {
        for (int i = 0; i < 9; ++i) {
            const u8 cnt = NitroPort_IoReg[BANK_REG[i]];
    
            banks[i].lcdc_offset = BANK_LCDC_OFFSET[i];
            banks[i].size = BANK_SIZE[i];
            banks[i].enabled = (cnt & 0x80) != 0;
            banks[i].mst = cnt & 0x07;
            banks[i].ofs = (cnt >> 3) & 0x03;
        }
    }
    
    static void publish(void)
    {
        kj_frame_input in;
    
        /* The per-line registers of this frame: H-blank handlers and DMAs run
         * once per V-blank, after the game's V-blank work armed them. */
        {
    #ifndef HGSS_PROFILE_OFF
            const u64 t0 = svcGetSystemTick();
    #endif
            if (NitroPort_HBlankLinesOnce()) {
    #ifndef HGSS_PROFILE_OFF
                ++s_hblank_walks;
    #endif
            }
    #ifndef HGSS_PROFILE_OFF
            s_hblank_ticks += svcGetSystemTick() - t0;
    #endif
        }
    
        memset(&in, 0, sizeof(in));
        in.mem = kj_ds_get_memory();
        decode_banks(in.banks);
        in.lines.active = NitroPort_LineRegs.active != 0;
        in.lines.regs = in.lines.active
                            ? (const uint8_t (*)[192][KJ_LINE_REG_BYTES])NitroPort_LineRegs.regs
                            : NULL;
        in.g3 = kj_g3_take_frame();
        in.vblank_count = s_vblanks;
        in.game_frame = ++s_publishes;
        in.engine_a_on_top = NitroPort_EngineAOnTop() != FALSE;
        s_last_publish_vblank = s_vblanks;
        kj_ctr_publish_frame(&in);
    }
    
    void NitroPort_VBlankWorkDone(void)
    {
        publish();
    }
    
    void NitroPort_OnFrame(void)
    {
        const kj_input *input;
    
        ++s_vblanks;
        {
    #ifndef HGSS_PROFILE_OFF
            const u64 t0 = svcGetSystemTick();
    #endif
            NitroPort_SoundPumpReplies();
    #ifndef HGSS_PROFILE_OFF
            s_pump_ticks += svcGetSystemTick() - t0;
            ++s_pump_calls;
    #endif
        }
    
        if (!kj_ctr_vblank()) {
            NitroPort_Log("[kj] close requested at V-blank %lu", (unsigned long)s_vblanks);
            NitroPort_CardFlush();
            kj_ctr_terminate();
        }
    
        input = kj_ctr_input();
        NitroPort_SetButtons(input->keys, FALSE);
        NitroPort_SetTouch(input->touch ? TRUE : FALSE, input->touch_x, input->touch_y);
    
        if (s_vblanks - s_last_publish_vblank > PUBLISH_GRACE_VBLANKS) {
            publish();
        }
    }
    
    /* POWCNT1 (0x04000304) bit 15: engine A drives the upper screen. */
    BOOL NitroPort_EngineAOnTop(void)
    {
        return (NitroPort_IoReg[0x305] & 0x80) ? TRUE : FALSE;
    }
    
    void NitroPort_Terminate(void)
    {
        NitroPort_Log("[kj] the game terminated itself (OS_Terminate)");
        NitroPort_CardFlush();
        kj_ctr_terminate();
    }
    
    void NitroPort_ResetSystem(u32 parameter)
    {
        /* A DS soft reset restarts the program from its entry. Static state of
         * the original game cannot be re-initialised in place, so the program
         * closes; relaunch is the user's (tracked: docs/worklog). */
        NitroPort_Log("[kj] soft reset requested (parameter %lu); closing", (unsigned long)parameter);
        NitroPort_CardFlush();
        kj_ctr_terminate();
    }
    
    void kj_g3_log_op_profile(void);
    
    void kj_ds_take_profile(kj_ds_profile *out)
    {
        kj_g3_log_op_profile();
        u64 calls, ticks;
    
        memset(out, 0, sizeof(*out));
        kj_g3_take_profile(&out->g3_fifo_ticks, &out->g3_fifo_words, &out->g3_direct_words,
                           &out->g3_commands);
        NitroPort_SyncTimersStats(&calls, &ticks);
        out->timer_sync_calls = calls - s_sync_calls_base;
        out->timer_sync_ticks = ticks - s_sync_ticks_base;
        s_sync_calls_base = calls;
        s_sync_ticks_base = ticks;
        out->sound_pump_ticks = s_pump_ticks;
        out->sound_pump_calls = s_pump_calls;
        out->hblank_ticks = s_hblank_ticks;
        out->hblank_walks = s_hblank_walks;
        kj_g3_dl_stats(&out->dl_hits, &out->dl_misses, &out->dl_uncacheable, &out->dl_bytes);
        s_pump_ticks = s_hblank_ticks = 0;
        s_pump_calls = s_hblank_walks = 0;
    }
    src/compat/nitro/src/port_os.c:27:extern u64 svcGetSystemTick(void);
    src/compat/nitro/src/port_os.c:28:#define PORT_NOW() svcGetSystemTick()
    src/compat/nitro/src/port_timer.c:35:extern u64 svcGetSystemTick(void);
    src/compat/nitro/src/port_timer.c:36:#define PORT_NOW() svcGetSystemTick()
    src/compat/nitro/src/port_timer.c:68:static u64 sSyncCalls, sSyncTicks;
    src/compat/nitro/src/port_timer.c:189:    sSyncTicks += PORT_NOW() - t0;
    src/compat/nitro/src/port_timer.c:190:    sSyncCalls++;
    src/compat/nitro/src/port_timer.c:198:    *calls = sSyncCalls;
    src/compat/nitro/src/port_timer.c:199:    *ticks = sSyncTicks;
    src/runtime/kj_g3.c:1071:extern u64 svcGetSystemTick(void);
    src/runtime/kj_g3.c:1077:    const u64 t0 = svcGetSystemTick();
    src/runtime/kj_g3.c:1079:    s_op_ticks[op & 255] += svcGetSystemTick() - t0;
    src/runtime/kj_g3.c:1334:extern u64 svcGetSystemTick(void);
    src/runtime/kj_g3.c:2061:/* Sample one FIFO call in 64. Nested portions are timed only in sampled
    src/runtime/kj_g3.c:2065:static bool s_coarse_sample;
    src/runtime/kj_g3.c:2110:    const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
    src/runtime/kj_g3.c:2114:    if (s_coarse_sample) s_coarse_replay += svcGetSystemTick() - t;
    src/runtime/kj_g3.c:2183:    const u64 t0 = svcGetSystemTick();
    src/runtime/kj_g3.c:2224:    s_fifo_ticks += svcGetSystemTick() - t0;
    src/runtime/kj_g3.c:2231:    const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
    src/runtime/kj_g3.c:2235:    if (s_coarse_sample) s_coarse_decode += svcGetSystemTick() - t;
    src/runtime/kj_g3.c:2304:         * the title's lit Ho-Oh lists spent 6.8% of samples comparing */
    src/runtime/kj_g3.c:2494:        s_coarse_sample = (++s_coarse_seq & 63u) == 0;
    src/runtime/kj_g3.c:2495:        if (s_coarse_sample) {
    src/runtime/kj_g3.c:2497:            start = svcGetSystemTick();
    src/runtime/kj_g3.c:2503:    if (outer && s_coarse_sample) s_coarse_total += svcGetSystemTick() - start;
    src/runtime/kj_g3.c:2505:    if (outer) s_coarse_sample = false;
    src/runtime/kj_g3.c:2512:    NitroPort_Log("[g3sample] every 64th FIFO call: samples %lu total_ticks %llu decode_ticks %llu replay_ticks %llu; remainder includes lookup/recording and probe overhead",
    
    ✓ • 625ms

## Assistant

The log contains several appended runs, including older data. In the latest instrumented run, the heavy field window reports about 80 ms median logic time, while GPU completion averages about 4 ms. The sampled FIFO timing also shows substantial CPU cost.

I found an invalid GPU sample—588 ms per picture despite a 1.93 ms completion average—and I’m checking the counter baselines and probe overhead before trusting the breakdown.

## Activity

    $ Get-Content src/graphics/renderer.c; Get-Content src/compat/nitro/src/port_os.c | Select-Object -Skip 550 -First 240; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 2050 -First 210; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 2460 -First 95; Get-Content private-gitignore/o3ds-log-3/heartgold.log -First 30; rg -n 'new run|\[kj\]|\[perf\] logic' private-gitignore/o3ds-log-3/heartgold.log; rg -n 'HBlank|PORT_NOW' src/compat/nitro/src/port_os.c
    /*
     * renderer.c: frame ownership and submission (3DS side).
     *
     * kj_ctr_publish_frame runs on the game thread at the frame boundary. It
     * waits for the previous GPU frame (so every GPU-visible buffer the
     * renderer owns is free to rewrite), translates the coherent DS state into
     * PICA commands for each screen, and submits. The game then continues while
     * the GPU works; nothing reads DS memory after this function returns.
     *
     * Screen mapping (kohjoh.md 0.4): top 400x240 shows a 320x192 logical canvas
     * (DS 256x192 plus 32 logical columns each side) at 1.25x; bottom 320x240
     * shows the DS's 256x192 at 1.25x. Only the screen whose engine output
     * changed is redrawn and transferred.
     */
    
    #include "graphics/renderer.h"
    
    #include "app/app.h"
    #include "app/script.h"
    #include "diagnostics/dump.h"
    #include "diagnostics/log.h"
    #include "diagnostics/perf.h"
    #include "graphics/g2d/g2d.h"
    #include "graphics/g3d/g3d.h"
    #include "graphics/g3d/textures.h"
    #include "graphics/pica/gpu.h"
    #include "graphics/pica/targets.h"
    #include "graphics/resources/atlas.h"
    #include "runtime/kj_bridge.h"
    #include "runtime/kj_g3.h"
    
    #include <3ds.h>
    #include <citro3d.h>
    #include <stdio.h>
    #include <string.h>
    #include <sys/stat.h>
    
    #define CMDBUF_BYTES (512 * 1024)
    
    static bool s_ready;
    static uint32_t s_frames;
    static uint32_t s_skipped_presents;
    static bool s_skipped_last; /* the previous publish skipped its picture */
    
    /* GPU-side time per 600 pictures (coffeepot performance.md, "Per-operation
     * GPU times": P3D is command-list execution, PPF display transfers, PSC
     * memory fills; C3D_GetDrawingTime is the latest completed queue, so it is
     * averaged, never matched to the current picture). */
    static bool s_perflog_on;
    static GSPGPU_PerfLog s_perflog_prev;
    static float s_draw_ms_sum;
    static float s_draw_ms_max;
    static float s_cmdbuf_max;
    
    bool kj_renderer_init(void)
    {
        if (!C3D_Init(CMDBUF_BYTES)) {
            kj_log("[gpu] C3D_Init failed");
            return false;
        }
        if (!kj_targets_init()) {
            return false;
        }
        {
            kj_g3_storage st;
            for (int i = 0; i < 2; ++i) {
                st.verts[i] = linearAlloc(sizeof(kj_g3_vertex) * KJ_G3_MAX_VERTS);
                st.indices[i] = linearAlloc(sizeof(uint16_t) * KJ_G3_MAX_INDICES);
                if (st.verts[i] == NULL || st.indices[i] == NULL) {
                    kj_log("[gpu] out of linear memory for 3D storage");
                    return false;
                }
            }
            kj_g3_init(&st);
            kj_g3_set_wide(true);
        }
        if (!kj_g2d_init() || !kj_g3d_init()) {
            return false;
        }
    #ifndef HGSS_PROFILE_OFF
        s_perflog_on = R_SUCCEEDED(GSPGPU_SetPerfLogMode(true)) &&
                       R_SUCCEEDED(GSPGPU_GetPerfLog(&s_perflog_prev));
    #endif
        s_ready = true;
        kj_log("[gpu] renderer ready (linear free %lu KiB, VRAM free %lu KiB)",
               (unsigned long)(linearSpaceFree() / 1024), (unsigned long)(vramSpaceFree() / 1024));
        return true;
    }
    
    void kj_renderer_exit(void)
    {
        if (!s_ready) {
            return;
        }
        s_ready = false;
        C3D_FrameBegin(0); /* waits for the GPU */
        C3D_FrameEnd(0);
        kj_g3d_exit();
        kj_g2d_exit();
        kj_targets_exit();
        C3D_Fini();
    }
    
    void kj_ctr_publish_frame(const kj_frame_input *in)
    {
        if (!s_ready) {
            return;
        }
        /* Running two or more refreshes behind: this picture would be shown
         * late anyway. Skip its GPU work (the game tick already happened) so
         * the schedule recovers; counted, never silent. Never two in a row
         * (coffeepot PICA200/frame-pacing.md, "Skip the picture, keep the
         * tick"): an uncapped skip starved the screen on O3DS whenever the game
         * ran behind -- 1606 of 2206 Goldenrod pictures skipped, ~4 shown a
         * second while the game ran at half speed (o3ds-log-0, window 6). */
        if (kj_pacing_behind() >= 2u && !s_skipped_last && !kj_script_active()) {
            s_skipped_last = true;
    #ifndef HGSS_PROFILE_OFF
            ++s_skipped_presents;
    #endif
            kj_perf_present_skipped(in->game_frame);
            return;
        }
        s_skipped_last = false;
        kj_perf_publish_begin();
    
        uint64_t t = kj_perf_now();
        C3D_FrameBegin(0); /* waits for the previous frame's GPU work */
        kj_perf_add(KJ_PERF_GPU_WAIT, kj_perf_now() - t);
        kj_gpu_quads_reset();
        kj_atlas_begin_frame();
        kj_g2d_begin_frame();
        kj_g3d_set_frame(in->g3);
        kj_tex_begin_frame(in);
    
        /* Engine A (and the 3D layer) is on the screen POWCNT1 says. */
        const int top_engine = in->engine_a_on_top ? 0 : 1;
        kj_screen_draw(KJ_SCREEN_TOP, top_engine, in);
        kj_screen_draw(KJ_SCREEN_BOTTOM, top_engine ^ 1, in);
    
        t = kj_perf_now();
        C3D_FrameEnd(0);
        kj_perf_add(KJ_PERF_SUBMIT, kj_perf_now() - t);
    #ifndef HGSS_PROFILE_OFF
        {
            const float draw_ms = C3D_GetDrawingTime();
            const float usage = C3D_GetCmdBufUsage();
    
            s_draw_ms_sum += draw_ms;
            if (draw_ms > s_draw_ms_max) {
                s_draw_ms_max = draw_ms;
            }
            if (usage > s_cmdbuf_max) {
                s_cmdbuf_max = usage;
            }
        }
    
    #endif
    #ifndef HGSS_PROFILE_OFF
        ++s_frames;
    #endif
        kj_perf_publish_end(in->game_frame);
    #ifndef HGSS_PROFILE_OFF
        if (s_frames % 600u == 0u) {
            kj_g2d_stats g;
            kj_g3d_stats d;
            kj_atlas_stats a;
            kj_g2d_take_stats(&g);
            kj_g3d_take_stats(&d);
            kj_atlas_take_stats(&a);
            kj_log("[gpu] 600 frames: 2D screens skipped %lu, layers reused %lu rebuilt %lu, "
                   "bg quads %lu obj quads %lu draws %lu spans %lu unsupported %lu dropped %lu, "
                   "char KiB compared %lu",
                   (unsigned long)g.skipped_screens, (unsigned long)g.reused_layers,
                   (unsigned long)g.rebuilt_layers, (unsigned long)g.bg_quads, (unsigned long)g.obj_quads,
                   (unsigned long)g.draws, (unsigned long)g.spans, (unsigned long)g.unsupported_modes,
                   (unsigned long)g.dropped_quads, (unsigned long)(g.char_bytes_compared / 1024u));
            kj_ds_profile dp;
            kj_ds_take_profile(&dp);
            /* Per game frame: one H-blank walk runs per game frame, so walks is
             * the game-frame denominator even when pictures were skipped. */
            const uint32_t gf = dp.hblank_walks != 0u ? dp.hblank_walks : 600u;
            const double us = 1000000.0 / SYSCLOCK_ARM11 / (double)gf;
            kj_log("[ds] per game frame (%lu game frames / 600 pictures): g3 fifo calls %llu "
                   "(%lu fifo words, %lu direct words, %lu cmds), timer sync %.0f us (%llu calls), "
                   "sound pump %.0f us, hblank %.0f us",
                   (unsigned long)gf, (unsigned long long)(dp.g3_fifo_ticks / gf),
                   (unsigned long)(dp.g3_fifo_words / gf), (unsigned long)(dp.g3_direct_words / gf),
                   (unsigned long)(dp.g3_commands / gf), dp.timer_sync_ticks * us,
                   (unsigned long long)(dp.timer_sync_calls / gf), dp.sound_pump_ticks * us,
                   dp.hblank_ticks * us);
            kj_log("[ds] display lists: %lu replayed, %lu decoded, %lu uncacheable, %lu KiB retained",
                   (unsigned long)dp.dl_hits, (unsigned long)dp.dl_misses,
                   (unsigned long)dp.dl_uncacheable, (unsigned long)(dp.dl_bytes / 1024u));
            uint32_t late, resyncs;
            kj_pacing_stats(&late, &resyncs);
            kj_log("[pace] 600 frames: late V-blanks %lu, resyncs %lu, presents skipped %lu",
                   (unsigned long)late, (unsigned long)resyncs, (unsigned long)s_skipped_presents);
            s_skipped_presents = 0;
            kj_log("[gpu] 600 frames: atlas lookups %lu conversions %lu evictions %lu full %lu; "
                   "3D batches %lu draws %lu tris %lu tex uploads %lu tex bytes compared %lu "
                   "tex full %lu unsupported %lu; box tests %lu culled %lu (far only %lu)",
                   (unsigned long)a.lookups, (unsigned long)a.conversions, (unsigned long)a.evictions,
                   (unsigned long)a.full, (unsigned long)d.batches, (unsigned long)d.draws,
                   (unsigned long)d.triangles, (unsigned long)d.tex_uploads,
                   (unsigned long)d.tex_bytes_compared, (unsigned long)d.tex_cache_full,
                   (unsigned long)d.unsupported, (unsigned long)d.box_tests,
                   (unsigned long)d.box_culled, (unsigned long)d.box_culled_far);
            {
                GSPGPU_PerfLog now;
                unsigned long p3d = 0, ppf = 0, psc = 0;
    
                if (s_perflog_on && R_SUCCEEDED(GSPGPU_GetPerfLog(&now))) {
                    /* Differences of the running totals (unsigned, so a wrap
                     * still subtracts correctly), per picture. */
                    p3d = (now.entries[GSPGPU_EVENT_P3D].totalDurationUs -
                           s_perflog_prev.entries[GSPGPU_EVENT_P3D].totalDurationUs) / 600u;
                    ppf = (now.entries[GSPGPU_EVENT_PPF].totalDurationUs -
                           s_perflog_prev.entries[GSPGPU_EVENT_PPF].totalDurationUs) / 600u;
                    psc = (now.entries[GSPGPU_EVENT_PSC0].totalDurationUs -
                           s_perflog_prev.entries[GSPGPU_EVENT_PSC0].totalDurationUs +
                           now.entries[GSPGPU_EVENT_PSC1].totalDurationUs -
                           s_perflog_prev.entries[GSPGPU_EVENT_PSC1].totalDurationUs) / 600u;
                    s_perflog_prev = now;
                }
                kj_log("[gputime] 600 pictures: GSP us/picture P3D %lu PPF %lu PSC %lu (log %s); "
                       "C3D drawing ms avg %.2f max %.2f; cmdbuf use max %u%%",
                       p3d, ppf, psc, s_perflog_on ? "on" : "off", (double)(s_draw_ms_sum / 600.0f),
                       (double)s_draw_ms_max, (unsigned)(s_cmdbuf_max * 100.0f));
                s_draw_ms_sum = 0.0f;
                s_draw_ms_max = 0.0f;
                s_cmdbuf_max = 0.0f;
            }
        }
    
    #endif
        /* Test directives: deterministic per-frame capture. */
        if (kj_script_dump(in->game_frame)) {
            char path[96];
            mkdir("sdmc:/3ds/kohjoh/dumps", 0777);
            snprintf(path, sizeof(path), "sdmc:/3ds/kohjoh/dumps/f%05lu.kjd", (unsigned long)in->game_frame);
            kj_dump_frame(path, in);
        }
        const uint32_t hold = kj_script_freeze(in->game_frame);
        if (hold > 0) {
            kj_log("[script] holding game frame %lu for %lu s", (unsigned long)in->game_frame,
                   (unsigned long)hold);
            for (uint32_t v = 0; v < hold * 60u && aptMainLoop(); ++v) {
                gspWaitForVBlank();
            }
        }
    }
    u32 OS_GetICacheEnableFlagsForProtectionRegion(void) { return 0; }
    void OS_SetIPermissionsForProtectionRegion(u32 setMask, u32 flags) { (void)setMask; (void)flags; }
    u32 OS_GetIPermissionsForProtectionRegion(void) { return 0; }
    void OS_EnableDCacheForProtectionRegion(u32 flags) { (void)flags; }
    void OS_DisableDCacheForProtectionRegion(u32 flags) { (void)flags; }
    u32 OS_GetDCacheEnableFlagsForProtectionRegion(u32 flags) { (void)flags; return 0; }
    void OS_SetDPermissionsForProtectionRegion(u32 setMask, u32 flags) { (void)setMask; (void)flags; }
    u32 OS_GetDPermissionsForProtectionRegion(void) { return 0; }
    void OS_EnableWriteBufferForProtectionRegion(u32 flags) { (void)flags; }
    void OS_DisableWriteBufferForProtectionRegion(u32 flags) { (void)flags; }
    u32 OS_GetWriteBufferEnableFlagsForProtectionRegion(u32 flags) { (void)flags; return 0; }
    void OS_SetProtectionRegion0(u32 param) { (void)param; }
    void OS_SetProtectionRegion1(u32 param) { (void)param; }
    void OS_SetProtectionRegion2(u32 param) { (void)param; }
    void OS_SetProtectionRegion3(u32 param) { (void)param; }
    void OS_SetProtectionRegion4(u32 param) { (void)param; }
    void OS_SetProtectionRegion5(u32 param) { (void)param; }
    void OS_SetProtectionRegion6(u32 param) { (void)param; }
    void OS_SetProtectionRegion7(u32 param) { (void)param; }
    u32 OS_GetProtectionRegion0(void) { return 0; }
    u32 OS_GetProtectionRegion1(void) { return 0; }
    u32 OS_GetProtectionRegion2(void) { return 0; }
    u32 OS_GetProtectionRegion3(void) { return 0; }
    u32 OS_GetProtectionRegion4(void) { return 0; }
    u32 OS_GetProtectionRegion5(void) { return 0; }
    u32 OS_GetProtectionRegion6(void) { return 0; }
    u32 OS_GetProtectionRegion7(void) { return 0; }
            ix[i] = (uint16_t)(si[i] + v0);
        }
        fb->frame.vert_count = v0 + b->nverts;
        fb->frame.index_count += b->nidx;
        s_batch->index_count += b->nidx;
        *v_first_out = v0;
        return true;
    }
    
    #ifndef HGSS_PROFILE_OFF
    /* Sample one FIFO call in 64. Nested portions are timed only in sampled
     * calls; the remainder includes lookup, comparison, recording and probe
     * overhead. Wall time includes preemption. */
    static u32 s_coarse_seq, s_coarse_n, s_coarse_depth;
    static bool s_coarse_sample;
    static u64 s_coarse_total, s_coarse_decode, s_coarse_replay;
    #endif
    
    static void dl_replay_impl(dl_entry *e)
    {
        uint32_t list_v0 = s_buf[s_build].frame.vert_count;
        uint32_t last_block_v0 = list_v0, last_block_rel = 0;
    
        for (uint32_t k = 0; k < e->nevents; ++k) {
            const uint32_t ev = e->events[k];
            if (ev & DL_EV_BLOCK) {
                const dl_block *b = &e->blocks[ev & ~DL_EV_BLOCK];
                uint32_t v0;
                if (!dl_append_block(e, b, &v0)) {
                    ++s_buf[s_build].frame.dropped_verts;
                    continue;
                }
                last_block_v0 = v0;
                last_block_rel = b->first_vert;
            } else {
                const uint32_t off = ev & 0xFFFFFu;
                execute(ev >> 20, e->words + off);
            }
        }
        s_color = e->exit.color;
        memcpy(s_coord, e->exit.coord, sizeof(s_coord));
        s_texcoord[0] = e->exit.texcoord[0];
        s_texcoord[1] = e->exit.texcoord[1];
        s_texcoord_raw[0] = e->exit.texcoord_raw[0];
        s_texcoord_raw[1] = e->exit.texcoord_raw[1];
        s_in_begin = e->exit.in_begin;
        s_primitive = e->exit.primitive;
        s_prim_count = e->exit.prim_count;
        s_strip_parity = e->exit.strip_parity;
        /* open primitives continue from the last block's vertices */
        for (int i = 0; i < 4; ++i) {
            s_prim_verts[i] = e->exit.prim_rel[i] - last_block_rel + last_block_v0;
        }
        update_color_rgba();
    }
    
    static void dl_replay(dl_entry *e)
    {
    #ifndef HGSS_PROFILE_OFF
        const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
    #endif
        dl_replay_impl(e);
    #ifndef HGSS_PROFILE_OFF
        if (s_coarse_sample) s_coarse_replay += svcGetSystemTick() - t;
    #endif
    }
    
    static uint32_t dl_vertex_bound(const dl_entry *e)
    {
        return e->nverts;
    }
    
    #ifdef KJ_DL_VERIFY
    static uint32_t s_verify_logged;
    
    /* A hit decoded live: does the live output equal what replay would copy? */
    static void dl_verify(const dl_entry *e, const frame_buf *fb)
    {
        const uint32_t nv = fb->frame.vert_count - s_rec.v0, ni = fb->frame.index_count - s_rec.i0;
        const char *why = NULL;
        uint32_t at = 0;
        if (s_rec.bad) {
            why = "live decode no longer recordable";
        } else if (nv != e->nverts || ni != e->nidx) {
            why = "vertex/index counts";
        } else if (s_rec.nblocks != e->nblocks || s_rec.nevents != e->nevents) {
            why = "event structure";
        } else if (memcmp(s_rec.events, e->events, e->nevents * 4u) != 0) {
            why = "events";
        } else {
            for (uint32_t i = 0; i < nv && why == NULL; ++i) {
                const uint32_t *a = (const uint32_t *)&fb->verts[s_rec.v0 + i];
                const uint32_t *b = (const uint32_t *)&e->verts[i];
                if (a[0] != b[0] || (a[1] & 0xFFFFu) != (b[1] & 0xFFFFu) || a[2] != b[2] || a[3] != b[3]) {
                    why = "vertex data";
                    at = i;
                }
            }
            for (uint32_t bk = 0; bk < e->nblocks && why == NULL; ++bk) {
                const dl_block *bl = &e->blocks[bk];
                for (uint32_t i = 0; i < bl->nidx; ++i) {
                    const uint32_t live = fb->indices[s_rec.i0 + bl->first_idx + i] - (s_rec.v0 + bl->first_vert);
                    if (live != e->idx[bl->first_idx + i]) {
                        why = "indices";
                        at = bk;
                        break;
                    }
                }
            }
        }
        if (why != NULL) {
            if (s_verify_logged < 40) {
                ++s_verify_logged;
                NitroPort_Log("[dlverify] MISMATCH %s at %lu: list %p (%lu words) nv %lu/%lu ni %lu/%lu "
                              "blocks %lu/%lu events %lu/%lu",
                              why, (unsigned long)at, (const void *)e->ptr, (unsigned long)e->count,
                              (unsigned long)nv, (unsigned long)e->nverts, (unsigned long)ni,
                              (unsigned long)e->nidx, (unsigned long)s_rec.nblocks,
                              (unsigned long)e->nblocks, (unsigned long)s_rec.nevents,
                              (unsigned long)e->nevents);
            }
        } else {
    #ifndef HGSS_PROFILE_OFF
            ++s_dl_hits;
    #endif
        }
    }
    #endif
    
    static void decode_words_impl(const uint32_t *words, size_t count)
    {
    #ifdef KJ_G3_PROFILE
        const u64 t0 = svcGetSystemTick();
    #endif
        size_t i = 0;
    
        while (i < count && s_fifo_need > 0) {
            NitroPort_GxFifoWord(words[i++]);
        }
        while (i < count) {
            uint32_t cmds = words[i];
            size_t need = 0;
            for (int k = 0; k < 4; ++k) {
                need += s_param_count[(cmds >> (8 * k)) & 0xFF];
            }
            if (i + 1 + need > count) {
                s_rec.bad = true; /* (no-op unless recording) */
                while (i < count) {
                    NitroPort_GxFifoWord(words[i++]);
                }
                break;
            }
            ++i;
            for (int k = 0; k < 4 && cmds != 0; ++k, cmds >>= 8) {
                const unsigned op = cmds & 0xFF;
                if (op == 0) {
                    continue;
                }
    #ifdef KJ_DL_CACHE
                if (s_rec.active) {
                    rec_command(op, words + i);
                }
    #endif
                hot_command(op, words + i);
    #ifdef KJ_DL_CACHE
                if (s_rec.active && (op < 0x20 || op > 0x28)) {
                    rec_open_block(); /* the next block starts after this command */
                }
    #endif
                i += s_param_count[op];
            }
        }
    #ifdef KJ_G3_PROFILE
        s_fifo_ticks += svcGetSystemTick() - t0;
    #endif
    }
    
    static void decode_words(const uint32_t *words, size_t count)
    {
    #ifndef HGSS_PROFILE_OFF
        const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
    #endif
        decode_words_impl(words, count);
    #ifndef HGSS_PROFILE_OFF
        if (s_coarse_sample) s_coarse_decode += svcGetSystemTick() - t;
    #endif
    }
    
    static void fifo_words_impl(const uint32_t *words, size_t count)
    {
    #ifndef HGSS_PROFILE_OFF
        ++s_fifo_calls;
    #endif
    #ifndef HGSS_PROFILE_OFF
        s_fifo_words += (u32)count;
    #endif
        if (s_param_count[0x16] == 0) {
            build_param_table();
        }
    #ifndef KJ_DL_CACHE
        /* KJ_DL_NO_CACHE: every list decoded live. */
        decode_words(words, count);
        return;
    #endif
        if (count < DL_MIN_WORDS || count > 0xFFFFFu || s_in_begin || s_fifo_need > 0 || s_rec.active) {
    #ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_SHORT] += (uint32_t)count;
    #endif
            if (count < DL_MIN_WORDS) {
    #ifndef HGSS_PROFILE_OFF
        e->nverts = nv;
        e->nidx = ni;
    
        e->key = key;
        e->lit = lit;
        e->texgen = texgen;
        dl_capture_exit(&e->exit, s_rec.v0);
        if (e->exit.in_begin) {
            /* open primitive at the end: express its vertices relative to the
             * last block so replay can rebase them */
            uint32_t last_rel = 0;
            for (uint32_t b = 0; b < e->nblocks; ++b) {
                last_rel = e->blocks[b].first_vert;
            }
            if (!e->exit.prim_valid) {
    #ifndef HGSS_PROFILE_OFF
                ++s_dl_why[5];
    #endif
                dl_drop_recording(e);
                e->never = true;
                return;
            }
            (void)last_rel;
        }
        e->recorded = true;
    }
    
    void NitroPort_GxFifoWords(const uint32_t *words, size_t count)
    {
    #ifndef HGSS_PROFILE_OFF
        const bool outer = s_coarse_depth++ == 0;
        u64 start = 0;
        if (outer) {
            s_coarse_sample = (++s_coarse_seq & 63u) == 0;
            if (s_coarse_sample) {
                ++s_coarse_n;
                start = svcGetSystemTick();
            }
        }
    #endif
        fifo_words_impl(words, count);
    #ifndef HGSS_PROFILE_OFF
        if (outer && s_coarse_sample) s_coarse_total += svcGetSystemTick() - start;
        --s_coarse_depth;
        if (outer) s_coarse_sample = false;
    #endif
    }
    
    void kj_g3_dl_stats(uint32_t *hits, uint32_t *misses, uint32_t *uncacheable, uint32_t *bytes)
    {
    #ifndef HGSS_PROFILE_OFF
        NitroPort_Log("[g3sample] every 64th FIFO call: samples %lu total_ticks %llu decode_ticks %llu replay_ticks %llu; remainder includes lookup/recording and probe overhead",
                      (unsigned long)s_coarse_n, (unsigned long long)s_coarse_total,
                      (unsigned long long)s_coarse_decode, (unsigned long long)s_coarse_replay);
        s_coarse_n = 0;
        s_coarse_total = s_coarse_decode = s_coarse_replay = 0;
    #endif
        *hits = s_dl_hits;
        *misses = s_dl_misses;
        *uncacheable = s_dl_uncacheable;
        *bytes = s_dl_arena_used;
        NitroPort_Log("[dlwhy] batch-in-block %lu shared-verts %lu straddle/drops %lu lit+matrix %lu "
                      "swap %lu open-prim %lu flushes %lu",
                      (unsigned long)s_dl_why[0], (unsigned long)s_dl_why[1], (unsigned long)s_dl_why[2],
                      (unsigned long)s_dl_why[3], (unsigned long)s_dl_why[4], (unsigned long)s_dl_why[5],
                      (unsigned long)s_dl_flushes);
        NitroPort_Log("[dlcov] bypass: small %lu split %lu open-begin %lu (begin first %lu)",
                      (unsigned long)s_dl_cov[DLC_SMALL], (unsigned long)s_dl_cov[DLC_SPLIT],
                      (unsigned long)s_dl_cov[DLC_BEGIN], (unsigned long)s_dl_cov[DLC_BEGIN_FIRST]);
        NitroPort_Log("[dlcov] words: short %lu new %lu changed %lu never %lu hit %lu keymiss %lu",
                      (unsigned long)s_dl_cov[DLC_SHORT], (unsigned long)s_dl_cov[DLC_NEW],
                      (unsigned long)s_dl_cov[DLC_CHANGED], (unsigned long)s_dl_cov[DLC_NEVER],
                      (unsigned long)s_dl_cov[DLC_HIT], (unsigned long)s_dl_cov[DLC_KEY]);
        memset(s_dl_cov, 0, sizeof(s_dl_cov));
        memset(s_dl_why, 0, sizeof(s_dl_why));
        s_dl_flushes = 0;
        (void)s_dl_bytes;
        s_dl_hits = s_dl_misses = s_dl_uncacheable = 0;
    }
    
    
    void kj_g3_take_profile(uint64_t *fifo_ticks, uint32_t *fifo_words, uint32_t *direct_words,
                            uint32_t *verts)
    {
        *fifo_ticks = (uint64_t)s_fifo_calls; /* calls; ticks only with KJ_G3_PROFILE */
        s_fifo_calls = 0;
        (void)s_fifo_ticks;
        *fifo_words = s_fifo_words;
        *direct_words = s_direct_words;
        *verts = s_stats.commands;
        s_fifo_ticks = 0;
        s_fifo_words = s_direct_words = 0;
        s_stats.commands = 0;
    }
         0.042 [app] kohjoh 5026d7b-dirty (heartgold), homebrew (SD pack), Old 3DS
         1.621 [gpu] renderer ready (linear free 15197 KiB, VRAM free 4794 KiB)
         1.665 [audio] NDSP 32768 Hz stereo, 4 x 512 frames
         1.665 [save] raw save file sdmc:/3ds/kohjoh/saves/heartgold.sav
         1.677 [calib] 1M-iteration loop: 3081616 ticks (3.08 ticks/iteration)
         1.677 [app] starting the game
         1.679 [kj] starting heartgold
         1.936 [nitro] overlay OVY_0 loaded
         1.936 [nitro] overlay OVY_38 loaded
         1.936 [nitro] not implemented: DWC_Init
         1.936 [nitro] overlay intro_title loaded
        11.051 [sndprof] render_calls 600 render_frames 307200 stretches 2399 sndticks 1799 mix_ticks 296770212 tick_ticks 26146830 mix_calls 2399 mix_frames 307200 active_ch 18151 pcm8_ch 0 pcm16_ch 0 adpcm_ch 18151 psg_ch 0 cap_inst 0 dep_ch 0
        12.920 [nitro] overlay intro_title loaded
        18.835 [nitro] overlay OVY_74 loaded
        19.019 [nitro] not implemented: WVR_StartUpAsync
        19.027 [nitro] not implemented: WM_InitializeForListening
        19.027 [nitro] not implemented: WM_GetNextTgid
        19.027 [nitro] not implemented: WM_GetLinkLevel
        19.057 [nitro] not implemented: WM_SetPortCallback
        19.559 [nitro] not implemented: WM_Reset
        19.855 [nitro] overlay OVY_36 loaded
        19.958 [nitro] overlay OVY_124 loaded
        19.958 [nitro] overlay ds_protect loaded
        20.056 [nitro] overlay field loaded
        20.057 [nitro] overlay ds_protect loaded
        20.057 [nitro] overlay OVY_2 loaded
        20.057 [nitro] overlay OVY_3 loaded
        20.425 [sndprof] render_calls 1200 render_frames 614400 stretches 4799 sndticks 3599 mix_ticks 762310492 tick_ticks 59418688 mix_calls 4799 mix_frames 614400 active_ch 45658 pcm8_ch 273 pcm16_ch 0 adpcm_ch 45385 psg_ch 0 cap_inst 0 dep_ch 0
        20.494 [nitro] overlay OVY_27 loaded
        29.804 [sndprof] render_calls 1800 render_frames 921600 stretches 7190 sndticks 5398 mix_ticks 1153698818 tick_ticks 90387628 mix_calls 7190 mix_frames 921600 active_ch 68743 pcm8_ch 273 pcm16_ch 0 adpcm_ch 68470 psg_ch 0 cap_inst 0 dep_ch 0
    7:     1.679 [kj] starting heartgold
    32:    41.980 [perf] logic    us p50   760 p95 146739 p99 149322 max 465824
    71:   132.201 [perf] logic    us p50 143918 p95 148451 p99 150099 max 158545
    108:   192.602 [perf] logic    us p50 96076 p95 147696 p99 151202 max 154876
    155:   230.276 [kj] close requested at V-blank 7533
    159:--- new run ---
    165:     1.708 [kj] starting heartgold
    214:   287.249 [kj] close requested at V-blank 14987
    218:--- new run ---
    225:     1.710 [kj] starting heartgold
    232:    21.913 [perf] logic    us p50   689 p95  2533 p99  4202 max 41685
    265:    42.306 [perf] logic    us p50  1300 p95 12303 p99 24974 max 106282
    316:    69.597 [perf] logic    us p50   861 p95 78793 p99 82770 max 457302
    353:   120.954 [perf] logic    us p50 79778 p95 82883 p99 83633 max 94437
    391:   172.279 [perf] logic    us p50 79630 p95 83049 p99 83745 max 93374
    434:   208.095 [perf] logic    us p50 53332 p95 81520 p99 89381 max 174603
    472:   228.113 [perf] logic    us p50  1884 p95  8341 p99 16840 max 38685
    512:   250.746 [perf] logic    us p50 24119 p95 58095 p99 63554 max 89853
    553:   273.129 [perf] logic    us p50 20876 p95 60761 p99 62357 max 71820
    594:   292.044 [kj] close requested at V-blank 15188
    22:/* libctru's tick counter, for the H-blank split (NitroPort_HBlankSplit). This
    28:#define PORT_NOW() svcGetSystemTick()
    30:#define PORT_NOW() 0
    42:/* Per-interrupt-bit time in the last H-blank walk (NitroPort_HBlankIrqByBit):
    45: * NitroPort_HBlankLines are counted; the one deferred scheduler switch at the
    47:static u64 sHBlankIrqBitTicks[NITRO_PORT_IRQ_BITS];
    48:static u32 sHBlankIrqBitCalls[NITRO_PORT_IRQ_BITS];
    49:static int sHBlankWalking;
    58: * FIFO is refreshed once before the walk instead (NitroPort_HBlankLines).
    92:        if (sHBlankWalking) {
    96:            u64 t = PORT_NOW();
    100:                sHBlankIrqBitTicks[bit] += PORT_NOW() - t;
    101:                sHBlankIrqBitCalls[bit]++;
    122:/* The last walk's three parts, in ARM11 ticks (NitroPort_HBlankSplit). */
    123:static u64 sHBlankKeepTicks, sHBlankDmaTicks, sHBlankIrqTicks;
    127:static u64 sHBlankRefreshTicks, sHBlankSwitchTicks, sHBlankCmpTicks;
    132:static u64 sHBlankScans, sHBlankScanMismatch;
    172:static void HBlank(int line, BOOL irq, BOOL dma)
    177:        u64 t = PORT_NOW();
    179:        NitroPort_DmaHBlank();
    180:        sHBlankDmaTicks += PORT_NOW() - t;
    183:        u64 t = PORT_NOW();
    186:        sHBlankIrqTicks += PORT_NOW() - t;
    192:static BOOL HBlankIrq(void)
    198:void NitroPort_HBlankLines(void)
    200:    BOOL irq = HBlankIrq();
    201:    BOOL dma = NitroPort_DmaHBlankArmed();
    215:    sHBlankKeepTicks = sHBlankDmaTicks = sHBlankIrqTicks = 0;
    217:    sHBlankRefreshTicks = sHBlankSwitchTicks = sHBlankCmpTicks = 0;
    220:    memset(sHBlankIrqBitTicks, 0, sizeof(sHBlankIrqBitTicks));
    221:    memset(sHBlankIrqBitCalls, 0, sizeof(sHBlankIrqBitCalls));
    226:    { u64 tR = PORT_NOW(); NitroPort_GxRefresh(); sHBlankRefreshTicks += PORT_NOW() - tR; }
    238:    sHBlankWalking = 1;
    240:    HBlank(262, irq, FALSE);
    244:        t0 = PORT_NOW();
    258:        t1 = PORT_NOW();
    259:        sHBlankKeepTicks += t1 - t0;
    260:        HBlank(y, irq, dma);   /* (the last one's: for the DMAs' source, as on the DS) */
    263:    sHBlankWalking = 0;
    269:        u64 tS = PORT_NOW();
    274:        sHBlankSwitchTicks += PORT_NOW() - tS;
    291:        sHBlankScans++;
    293:            sHBlankScanMismatch++;
    299:/* The last walk's three parts, in ARM11 ticks (NitroPort_HBlankLines). */
    300:void NitroPort_HBlankSplit(u64 *keep, u64 *dma, u64 *irq)
    302:    *keep = sHBlankKeepTicks;
    303:    *dma = sHBlankDmaTicks;
    304:    *irq = sHBlankIrqTicks;
    309:void NitroPort_HBlankSplit2(u64 *refresh, u64 *sw, u64 *cmp)
    311:    if (refresh != NULL) *refresh = sHBlankRefreshTicks;
    312:    if (sw != NULL) *sw = sHBlankSwitchTicks;
    313:    if (cmp != NULL) *cmp = sHBlankCmpTicks;
    319:void NitroPort_HBlankScanCheck(u64 *walks, u64 *mismatch)
    321:    if (walks != NULL) *walks = sHBlankScans;
    322:    if (mismatch != NULL) *mismatch = sHBlankScanMismatch;
    326:/* The last walk's interrupt handlers, by IRQ bit (NitroPort_HBlankLines). */
    327:void NitroPort_HBlankIrqByBit(NitroPortIrqTime *out)
    332:        out[bit].ticks = sHBlankIrqBitTicks[bit];
    333:        out[bit].calls = sHBlankIrqBitCalls[bit];
    338: * worked out (NitroPort_HBlankLinesOnce). */
    346:BOOL NitroPort_HBlankLinesDue(void)
    348:    return sLinesVBlank != sVBlanks && (HBlankIrq() || NitroPort_DmaHBlankArmed());
    351:BOOL NitroPort_HBlankLinesOnce(void)
    360:    NitroPort_HBlankLines();
    ✓ • 619ms

## Activity

    $ Get-Content src/compat/nitro/src/port_os.c -First 280; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1040 -First 65; Get-Content docs/lessons.md; Get-Content docs/troubleshooting.md; Get-Content docs/building-windows.md; Get-Content private-gitignore/o3ds-log-3/heartgold.log | Select-Object -Skip 350 -First 64; rg -n 'KJ_G3_PROFILE|HGSS_PROFILE|CFLAGS' tools/game/hgss_game.py src/compat/nitro/Makefile
    /*
     * The DS CPU and system, as far as NitroSDK's OS library needs them.
     *
     * - The status register (interrupt enable bits, CPU mode) is a variable.
     * - Interrupts are raised by the port's emulated hardware, which runs
     *   OS_IrqHandler just like the real interrupt vector would, in the CPU's
     *   interrupt mode.
     * - OS_Halt ("sleep until the next interrupt") advances one frame.
     * - Thread switching is in port_thread.c. Reset and OS_Terminate have hooks
     *   for the main program.
     * - Replacements for SDK files that only configure the DS's ARM946 CPU
     *   (caches, tightly-coupled memory, the protection unit): there is nothing
     *   to configure on the 3DS, so they do nothing.
     */
    
    #include <nitro.h>
    #include <nitro_port.h>
    #include <nitro_port_gx3d.h>
    #include <stdlib.h>
    #include <string.h>
    
    /* libctru's tick counter, for the H-blank split (NitroPort_HBlankSplit). This
     * library does not include libctru's headers -- they clash with the SDK's --
     * so the one prototype it needs is declared here. Host builds (tests, tools)
     * have no counter: timings read 0, the same pattern as port_render2d.c. */
    #if defined(__3DS__) && !defined(HGSS_PROFILE_OFF)
    extern u64 svcGetSystemTick(void);
    #define PORT_NOW() svcGetSystemTick()
    #else
    #define PORT_NOW() 0
    #endif
    
    extern unsigned char NitroPort_IoReg[];
    
    /* ---------------------------------------------------------------- the CPU */
    
    /* System mode, interrupts enabled: how the DS CPU is set up before main. */
    u32 OSi_PortCpsr = HW_PSR_SYS_MODE;
    
    /* ------------------------------------------------------------- interrupts */
    
    /* Per-interrupt-bit time in the last H-blank walk (NitroPort_HBlankIrqByBit):
     * which handler the walk's interrupts ran -- bit 1 is the game's H-blank
     * callback -- in ARM11 ticks, and how many times. Only dispatches made inside
     * NitroPort_HBlankLines are counted; the one deferred scheduler switch at the
     * end of the walk is not a handler and stays in the walk's total. */
    static u64 sHBlankIrqBitTicks[NITRO_PORT_IRQ_BITS];
    static u32 sHBlankIrqBitCalls[NITRO_PORT_IRQ_BITS];
    static int sHBlankWalking;
    
    /* `refresh` is off for the 192 raises of an H-blank walk.
     *
     * NitroPort_GxRefresh re-raises the GXFIFO level interrupt whenever the FIFO
     * is under half. Doing that once per scanline means the GXFIFO handler runs
     * 192 times a frame as well as the H-blank one, and on an Old 3DS the walk
     * measures 9.5-13.8 ms with 87% of it inside these raises (hb_irq_us), while
     * the H-blank callback itself is a field wipe task that is usually idle. The
     * FIFO is refreshed once before the walk instead (NitroPort_HBlankLines).
     */
    static void raise_irq(u32 mask, int refresh)
    {
        int guard;
        u32 saved;
    
        reg_OS_IF |= mask;
    
        /* Run handlers while interrupts are enabled and something is pending,
         * one interrupt per call, lowest number first, as on the DS. The guard
         * only stops a handler that never clears its interrupt from hanging the
         * program; level-triggered ones (the 3D FIFO's) legitimately come back
         * many times in a row. */
        for (guard = 0; guard < 65536; guard++) {
            if (refresh) {
                NitroPort_GxRefresh();   /* GXSTAT, and the GXFIFO level interrupt */
            }
            if (OSi_PortCpsr & HW_PSR_IRQ_DISABLE) {
                return;
            }
            if (reg_OS_IME == 0 || (reg_OS_IE & reg_OS_IF) == 0) {
                return;
            }
            /* Taking an interrupt puts the CPU in interrupt mode with further
             * interrupts masked until the handler returns, as the ARM CPU does.
             * Anything raised meanwhile stays pending and runs on the next pass
             * of this loop. In interrupt mode the SDK's scheduler only notes
             * that a switch is wanted; OS_IrqHandler switches at its end
             * (port_thread.c), and this thread carries on from there when it's
             * switched back to. */
            saved = OSi_PortCpsr;
            OSi_PortCpsr = (saved & ~HW_PSR_CPU_MODE_MASK) | HW_PSR_IRQ_MODE | HW_PSR_IRQ_DISABLE;
    #ifndef HGSS_PROFILE_OFF
            if (sHBlankWalking) {
                /* The bit OS_IrqHandler will run: the lowest pending, enabled one. */
                u32 pending = reg_OS_IE & reg_OS_IF;
                int bit = (pending != 0) ? __builtin_ctz(pending) : -1;
                u64 t = PORT_NOW();
    
                OS_IrqHandler();
                if (bit >= 0 && bit < NITRO_PORT_IRQ_BITS) {
                    sHBlankIrqBitTicks[bit] += PORT_NOW() - t;
                    sHBlankIrqBitCalls[bit]++;
                }
            } else {
                OS_IrqHandler();
            }
    #else
            OS_IrqHandler();
    #endif
            OSi_PortCpsr = saved;
        }
    }
    
    void NitroPort_RaiseIrq(u32 mask)
    {
        raise_irq(mask, 1);
    }
    
    /* --------------------------------------------------- H-blank, line by line */
    
    NitroLineRegs NitroPort_LineRegs;
    
    /* The last walk's three parts, in ARM11 ticks (NitroPort_HBlankSplit). */
    static u64 sHBlankKeepTicks, sHBlankDmaTicks, sHBlankIrqTicks;
    #ifndef HGSS_PROFILE_OFF
    /* P1: the parts outside the keep/dma/irq split: the per-walk refresh, the
     * thread-switch step, and the line compare scan. */
    static u64 sHBlankRefreshTicks, sHBlankSwitchTicks, sHBlankCmpTicks;
    #endif
    #ifdef HGSS_A0_DIAG
    /* P6: the walks checked (the fused compare against the old scan) and the
     * ones where the two answers differed. */
    static u64 sHBlankScans, sHBlankScanMismatch;
    #endif
    
    /* A line's registers; not DISPSTAT and VCOUNT (0x04-0x07: they draw
     * nothing, and the V-count is each line's own: every frame with the
     * H-blank interrupt on, as the field has it, would have its lines differ). */
    static void KeepLine(int y)
    {
        memcpy(NitroPort_LineRegs.regs[0][y], NitroPort_IoReg, NITRO_LINE_REGS);
        memcpy(NitroPort_LineRegs.regs[1][y], NitroPort_IoReg + 0x1000, NITRO_LINE_REGS);
        memset(NitroPort_LineRegs.regs[0][y] + 4, 0, 4);
        memset(NitroPort_LineRegs.regs[1][y] + 4, 0, 4);
    }
    
    /* P6: do two lines' register blocks differ? The word at 0x04 (DISPSTAT and
     * VCOUNT) is zeroed in every copy, so it never differs and is skipped; the
     * rest is compared a word at a time, not byte at a time: the same question,
     * a tenth of the instructions (a 112-byte memcmp is about 330 ARM11 cycles,
     * and the scan this replaces did 384 of them per walk). */
    static int LineRegsDiffer(int engine, int y, int y0)
    {
        const u32 *a = (const u32 *)NitroPort_LineRegs.regs[engine][y];
        const u32 *b = (const u32 *)NitroPort_LineRegs.regs[engine][y0];
        int i;
    
        if (a[0] != b[0]) {
            return 1;
        }
        for (i = 2; i < NITRO_LINE_REGS / 4; i++) {
            if (a[i] != b[i]) {
                return 1;
            }
        }
        return 0;
    }
    
    /* One H-blank, of line `line` (the V-count the game reads meanwhile, and
     * DISPSTAT's H-blank flag set: some handlers do nothing without it, e.g.
     * the field's poison wave and the window wipes): its DMAs (only on the 192
     * lines drawn, as on the DS), then its interrupt. */
    static void HBlank(int line, BOOL irq, BOOL dma)
    {
        reg_GX_VCOUNT = (u16)line;
        reg_GX_DISPSTAT |= REG_GX_DISPSTAT_HBLK_MASK;
        if (dma && line < NITRO_PORT_SCREEN_H) {
            u64 t = PORT_NOW();
    
            NitroPort_DmaHBlank();
            sHBlankDmaTicks += PORT_NOW() - t;
        }
        if (irq) {
            u64 t = PORT_NOW();
    
            raise_irq(OS_IE_H_BLANK, 0);
            sHBlankIrqTicks += PORT_NOW() - t;
        }
    }
    
    /* The H-blank interrupt is taken if the display asks for it (DISPSTAT),
     * it's enabled (IE, IME) and the CPU takes interrupts now. */
    static BOOL HBlankIrq(void)
    {
        return (reg_GX_DISPSTAT & REG_GX_DISPSTAT_HBI_MASK) && (reg_OS_IE & OS_IE_H_BLANK) && reg_OS_IME &&
               !(OSi_PortCpsr & HW_PSR_IRQ_DISABLE);
    }
    
    void NitroPort_HBlankLines(void)
    {
        BOOL irq = HBlankIrq();
        BOOL dma = NitroPort_DmaHBlankArmed();
        u16 vcount = reg_GX_VCOUNT;
        u64 t0, t1;
        int y;
    
        NitroPort_LineRegs.serial++;
        if (!irq && !dma) {
            NitroPort_LineRegs.active = 0;
            return;
        }
        /* Where the 9.5-13.8 ms an Old 3DS spends in the hand-off goes. The three
         * candidates are the DMA copies, the interrupt handlers (192 of them,
         * through the SDK's dispatcher) and the per-line register copy; guessing
         * between them has already been wrong once. */
        sHBlankKeepTicks = sHBlankDmaTicks = sHBlankIrqTicks = 0;
    #ifndef HGSS_PROFILE_OFF
        sHBlankRefreshTicks = sHBlankSwitchTicks = sHBlankCmpTicks = 0;
    #endif
    #ifndef HGSS_PROFILE_OFF
        memset(sHBlankIrqBitTicks, 0, sizeof(sHBlankIrqBitTicks));
        memset(sHBlankIrqBitCalls, 0, sizeof(sHBlankIrqBitCalls));
    #endif
        /* (GXSTAT and the GXFIFO level interrupt once, not once a scanline: see
         * raise_irq) */
    #ifndef HGSS_PROFILE_OFF
        { u64 tR = PORT_NOW(); NitroPort_GxRefresh(); sHBlankRefreshTicks += PORT_NOW() - tR; }
    #else
        NitroPort_GxRefresh();
    #endif
        /* One scheduler switch for the whole walk, not 192: see
         * NitroPort_IrqThreadSwitch. */
        NitroPort_IrqSwitchWanted = 0;
        NitroPort_IrqSwitchDeferred = 1;
        /* Line 0 has what the last H-blank before it left (the frame's last
         * line's, in the V-blank: its interrupt only), then each line what
         * the H-blank of the line before did. */
    #ifndef HGSS_PROFILE_OFF
        sHBlankWalking = 1;
    #endif
        HBlank(262, irq, FALSE);
        /* P6: whether the lines differ is decided during the walk, below. */
        NitroPort_LineRegs.active = 0;
        for (y = 0; y < NITRO_PORT_SCREEN_H; y++) {
            t0 = PORT_NOW();
            KeepLine(y);
            /* P6: "do the lines differ?" is answered here, as each line is
             * kept: the line before's copy is still in cache, so this is a
             * cheap compare instead of the separate pass over all 192 lines
             * that followed the walk (43 KB read back from RAM: 0.77 ms of
             * the 2.6 ms the H-blank phase took on an Old 3DS). "Some line
             * differs from the first" and "some line differs from the one
             * before it" are the same question, and the compare stops at the
             * first difference, as the old scan did. */
            if (!NitroPort_LineRegs.active && y > 0) {
                NitroPort_LineRegs.active = LineRegsDiffer(0, y, y - 1) ||
                                            LineRegsDiffer(1, y, y - 1);
            }
            t1 = PORT_NOW();
            sHBlankKeepTicks += t1 - t0;
            HBlank(y, irq, dma);   /* (the last one's: for the DMAs' source, as on the DS) */
        }
    #ifndef HGSS_PROFILE_OFF
        sHBlankWalking = 0;
    #endif
        reg_OS_IF &= ~OS_IE_H_BLANK;   /* (not left pending for later) */
        NitroPort_IrqSwitchDeferred = 0;
        if (NitroPort_IrqSwitchWanted) {
    #ifndef HGSS_PROFILE_OFF
            u64 tS = PORT_NOW();
    #endif
            NitroPort_IrqSwitchWanted = 0;
            NitroPort_IrqThreadSwitch();
    #ifndef HGSS_PROFILE_OFF
            sHBlankSwitchTicks += PORT_NOW() - tS;
    #endif
        }
        reg_GX_DISPSTAT &= (u16)~REG_GX_DISPSTAT_HBLK_MASK;
        reg_GX_VCOUNT = vcount;
    #ifdef HGSS_A0_DIAG
        /* P6: the fused compare and the old scan over all the lines must
    }
    
    /* ------------------------------------------------------------- dispatch */
    
    static int param_count(unsigned op)
    {
        switch (op) {
        case 0x10: case 0x12: case 0x13: case 0x14:
        case 0x20: case 0x21: case 0x22: case 0x24: case 0x25: case 0x26: case 0x27: case 0x28:
        case 0x29: case 0x2A: case 0x2B: case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x40: case 0x50: case 0x60: case 0x72:
            return 1;
        case 0x16: case 0x18:
            return 16;
        case 0x17: case 0x19:
            return 12;
        case 0x1A:
            return 9;
        case 0x1B: case 0x1C: case 0x70:
            return 3;
        case 0x23: case 0x71:
            return 2;
        case 0x34:
            return 32;
        default:
            return 0;
        }
    }
    
    #ifdef KJ_G3_PROFILE
    extern u64 svcGetSystemTick(void);
    static u64 s_op_ticks[256];
    static u32 s_op_calls[256];
    static void execute_inner(unsigned op, const uint32_t *p);
    static void execute(unsigned op, const uint32_t *p)
    {
        const u64 t0 = svcGetSystemTick();
        execute_inner(op, p);
        s_op_ticks[op & 255] += svcGetSystemTick() - t0;
        ++s_op_calls[op & 255];
    }
    
    void kj_g3_log_op_profile(void)
    {
        for (int i = 0; i < 256; ++i) {
            if (s_op_calls[i]) {
                NitroPort_Log("[g3prof] op %02x calls %lu ticks %llu (%llu/call)", i,
                              (unsigned long)s_op_calls[i], (unsigned long long)s_op_ticks[i],
                              (unsigned long long)(s_op_ticks[i] / s_op_calls[i]));
            }
        }
        memset(s_op_ticks, 0, sizeof(s_op_ticks));
        memset(s_op_calls, 0, sizeof(s_op_calls));
    }
    
    static void execute_inner(unsigned op, const uint32_t *p)
    #else
    void kj_g3_log_op_profile(void)
    {
    }
    
    static void execute(unsigned op, const uint32_t *p)
    #endif
    {
    #ifndef HGSS_PROFILE_OFF
    # Lessons
    
    What helped, what cost time, and the rules that follow.
    
    ## What went right
    
    - **Reuse the game-compile layer, rewrite the slow parts.** hgss-3ds's patch
      stack and build driver compile the decomp under GCC (~90 verified fixes).
      Vendoring them (with provenance) gave a linking game in hours, and the time
      went into the runtime and renderer, which is where hgss-3ds lost its speed.
    - **One thread, one publish point.** Rendering at a single coherent point in
      the game loop (patch 2001) removed the drawer/game races that hgss-3ds spent
      weeks on. No locks, no torn frames.
    - **GPU transform with a matrix palette.** Keeping DS matrix, lighting and test
      semantics on the CPU but sending object-space vertices to the PICA made 3D
      work on the first try, widescreen included (one fixup matrix).
    - **An independent oracle.** The GBATEK reference renderer found the tile
      row-order bug in one comparison after guessing had failed.
    - **Real profiling.** Azahar's GDB stub plus a 150-line PC sampler found the
      actual hotspots (geometry decode, Pokémon crypto), including one outside the
      renderer that no reasoning would have predicted.
    - **Calibrating the emulator.** A 4-instruction loop showed Azahar charges one
      tick per instruction, so its timings are instruction counts, not O3DS time.
    - **Verify modes.** `KJ_DL_VERIFY` (decode live, compare with replay) found the
      display-list bug in one run.
    
    ## What went wrong
    
    - **Committing late.** Several verified improvements sat uncommitted while
      riskier work piled on top. Commit every verified step; gate risky features
      behind `--define` switches.
    - **Building before measuring.** The display-list cache was written before its
      hit rate was known; most lists turned out to straddle DMA bursts or vary per
      frame. Measure the opportunity first.
    - **Timing probes that dominate.** Per-call `svcGetSystemTick` pairs cost ~390
      ticks each and nested DMA callbacks inflated the timed region; both misled.
      Prefer sampling.
    - **A struct-layout bug.** `memset(&s, 0, offsetof(s, array))` left counters
      that follow the array unreset. Reset fields explicitly.
    - **An unverified build-system change regressed saves.** Switching the SDK shim
      from copies to junctions changed what compiled; it went untested past a save
      check. Run the save scenario after any build-system change.
    - **Fragile command lines.** Multi-line heredocs with nested quotes were
      mangled by the shell wrapper; complex edits belong in script files.
    - **Contaminated tests.** Live input reached scripted runs. Tests must own
      their input.
    - **Guessing at render state.** Four Ho-Oh render-state experiments (cull,
      depth, alpha test, blend) changed nothing. Dumping the decoded scene and
      projecting it on the CPU (`g3_reference.py`) showed in one step that the
      geometry, not the GPU path, was wrong: 41 all-zero clip matrices per frame.
      Split the pipeline at a dump before toggling GPU state.
    - **"Ported" is not "complete".** Patch 0302 converted the GXFIFO stores in
      NNS but not the direct command-register stores beside them, and nothing
      failed loudly: the writes simply vanished. When a hardware access class is
      ported, scan the whole tree for that class (see game patch 2004).
    - **A shared `build/upstream` checkout carries branch state.** It held
      `wip/savestore`'s SDK patch 2003 while building `master`; `check_patches.py`
      only detects missing patches, not extra ones. Re-created with
      `bootstrap.py --clean` on 2026-11-12; after switching branches, do the same.
    - **An uncapped present skip starves the screen.** "Skip the picture when
      two refreshes behind" never presents while the game runs slightly over
      budget: O3DS Goldenrod ran at half speed but skipped 1606 of 2206
      pictures, ~4 a second (and probably left the title's white flash on screen). Skip at most one
      picture in a row (coffeepot frame-pacing, "skip the picture, keep the tick").
    - **"Uninstrumented" builds still carried probes.** `PORT_NOW` in the H-blank
      walk ignored `HGSS_PROFILE_OFF`, and profiling was the default: ~800
      `svcGetSystemTick` calls a frame. Probes are now opt-in (`HGSS_PROFILE=on`).
    - **Azahar cannot see code size.** Force-inlining the vertex fast path into
      every opcode made the decode loop 14.6 KiB; the O3DS I-cache is 16 KiB and
      CPI reaches 5.6 past 32 KiB (coffeepot icachebench). Check hot-function
      sizes with `nm -S` after touching a hot loop.
    
    ## Session 2 (O3DS Goldenrod performance)
    
    What went right:
    - **Reading the hardware log before the code.** o3ds-log-0's `[pace]` and
      `[frame]` lines showed presentation starvation; the per-opcode G3 profile
      (probe cost subtracted: ~440 ticks, the cost of the empty END opcode)
      ranked the decoder's real costs on O3DS.
    - **The wiki had both answers**: "never skip two in a row" (frame-pacing)
      and the 16 KiB I-cache cliff (cpu-optimization, icachebench).
    - **Exact rewrites, checked by counters**: every decoder change kept the
      same batches, draws and triangles per 600 frames in Azahar.
    
    What went wrong, and the fix:
    - **Misreading a counter.** `[ds] per frame` divides by presented frames;
      with skips it overstated per-game-frame costs, and an early note claimed
      the game ran "near full speed" when that window ran at half speed. Fixed
      in docs; counter semantics are now written in testing.md.
    - **Harness races.** A second `run.ps1` killed the emulator of the first
      (three lost runs). Stop the old harness before starting another.
    - **Lost samples.** gdbprof printed nothing when Azahar closed; it now
      prints what it has.
    - **Sampler bias.** Tight loops (the crypto XOR) looked bigger in samples
      than in `[perf]` time. Measure with stage times.
    
    - **Measuring the wrong thing.** The display-list cache replayed 4% of
      words. The first guess (lists left with BEGIN open) explained 13%; a
      coverage counter (`[dlcov]`) showed 65% of words were split mid-command
      by NitroSDK's 118-word DMA bursts. Count where the work goes before
      fixing a guessed cause.
    - **Bump arenas that flush.** Re-recording into fresh space filled the arena
      and flushed it twice per 600 frames (29.6 ms spikes). Reuse an entry's
      storage when it re-records.
    - **An unprofiled thread.** The DS sound mixer runs on its own thread and
      took 20-30% of the O3DS application core (o3ds-log-1), but Azahar lacked
      DSP firmware, so `ndspInit` failed and the mixer never ran in any
      emulator profile. Make sure every thread the device runs also runs under
      test (`dspfirm.cdc` is now in Azahar's sdmc).
    
    ## Rules
    
    1. Commit each verified step; keep `master` building and verified.
    2. Check the Coffeepot wiki and `docs/engineering/hgss-remaster/` before
       inventing a technique; cite it.
    3. Compare against the oracle or a known-good capture before blaming a subsystem.
    4. Profile with PC sampling; treat Azahar time as instruction count.
    5. Every cache needs an exact validity check and a verify mode.
    6. After any build-system change, rerun intro, Goldenrod (with save) and input.
    7. After touching a hot loop, check its size with `nm -S` (16 KiB I-cache).
    8. One Azahar harness at a time.
    # Troubleshooting
    
    Each entry: symptom → cause → fix. The tools already contain the fixes; this
    page explains them for the next breakage.
    
    ## Windows build chain
    
    | Symptom | Cause | Fix |
    | --- | --- | --- |
    | `Cannot create temporary file in C:\Windows\` | MSYS2 drops `TMP`/`TEMP` from child processes; native GCC falls back to `C:\Windows`. | `common.mk` exports them; `hgss_nitro.py` sets them in Python. |
    | `-DFOO` set via `FOO=1 python ...` has no effect | MSYS2 Python started directly from Git Bash does not see inline environment variables (inside `bash.exe -lc` it does). | `hgss_game.py --define FOO`. |
    | Bootstrap: `git-remote-https.exe: error while loading shared libraries` | devkitPro MSYS2's `git` has a broken HTTPS helper. | `export PATH="/c/Program Files/Git/cmd:$PATH"` inside the `-lc` shell ([build](building-windows.md#build)). |
    | `mkdir: cannot create directory '/home'`; headers "not found" | MSYS2 Python resolves the working directory as `/home/<user>/...`, which bash, make and native tools cannot use; MSYS also does not convert glued arguments such as `-idirafter/c/...`. | Invoke with absolute `/c/...` paths and `--root`; pass `-idirafter` and its path as separate arguments. |
    | `ld: cannot find /c/Users/.../x.o` | Native `ld` reads paths inside `@response` files literally; MSYS converts only command-line arguments. | Response files use `C:/...` (`_tool_path`). |
    | `ar: Argument list too long` | Very deep output paths (e.g. under `%TEMP%`). | Build under the repository's `build/`. |
    | Every build recompiles all 1092 files | The dependency parser split `C:/...` at the drive colon. | Split on `": "`; objects are skipped when newer than source and all `-MMD` dependencies, with identical flags. |
    | SDK/compat edits have no effect | MSYS2 Python's `symlink_to` silently **copies** directories, so `build/hgss-sdk` held stale copies. | On `wip/savestore`: junctions via `cmd.exe //c mklink //J` (MSYS rewrites a bare `/c`). See the open regression in [status](status.md). |
    | `gen_fx_consts` fails (missing libc headers / `Read error`) | MSYS gcc lacks libc headers; the tool reads its CSV in text mode. | Bootstrap generates the header in Python (`prepare_game_headers.py`). |
    | Partial Azahar screenshots | DPI virtualisation. | `run.ps1` calls `SetProcessDPIAware` and resizes the window. |
    
    ## Runtime
    
    | Symptom | Cause | Fix |
    | --- | --- | --- |
    | Rows reversed inside every 8×8 tile | Wrong Morton row order: PICA tiles are stored top row first. | Fixed in `atlas.c`/`textures.c`; found by comparing with the reference renderer. |
    | One-pixel horizontal gaps between tile rows | Nearest sampling near v = 1.0 in float24 rounded into the neighbouring (often transparent) atlas tile. | Texcoords in 1/16 texel, inset by 1/16 per edge. |
    | Music drifts ahead of the picture | A frame overrunning a V-blank lost that V-blank for good. | Emulated V-blanks locked to the real V-blank counter; late frames catch up. |
    | Field at ~17 FPS in Azahar | Geometry decode and `GetMonData` crypto (PC sampling). | Decoder fast paths; keystream cache (game patch 2002). |
    | White map, 1% speed with `KJ_DL_CACHE` | The display-list recorder's counters sat after its arrays and escaped the `memset`; recordings overran and corrupted the heap (100% of samples in `_malloc_r`). | Fixed on `wip/savestore` (explicit reset, bump arena); `KJ_DL_VERIFY` compares replay with live decode. |
    | Saved game ignored (new-game intro) | Open, `wip/savestore` only: appeared with the SDK shim junction change; not caused by patch 2003. | Investigate what differs between the old copied shim and the live sources. |
    | A test run diverges from its script | Live keyboard input on the Azahar window was OR-ed with the script. | `wip/savestore`: scripted runs ignore live input. |
    | Title lower screen black (no Ho-Oh) | The title uses `GX_BUFFERMODE_W` with near = 0, so every vertex has z/w = 1 and failed a Z-mode GREATER test. | W-buffer path in `g3d.c`: depth = w/4096 (PICA W-buffer mode), LESS, depth cleared to 0xFFFFFF. |
    | Chunks of a skinned model missing (title Ho-Oh's wings) | Direct stores to G3 command registers (`reg_G3_MTX_STORE = ...`) land in plain memory on the port. NNS NODEMIX's envelope matrices were never stored, so weighted vertices restored zero matrices and collapsed to clip (0,0,0,0). | Route every direct G3 command-register write through `NitroPort_GxDirectWrite` (NitroSystem 0305, NitroSDK 0606, game 2004). Never "fix" this by changing w or dropping/redrawing w <= 0 triangles: that is what produced the old port's stretched Ho-Oh. |
    | Vendored SDK change has no effect | The vendored library build failed and the link fell back to the prebuilt `libnitro*.a` (build log: "vendored library build failed"). | Grep the build log for `error:` before testing any SDK/NitroSystem change. |
    | O3DS: town "unplayably slow" though the game ticks | Uncapped present skip: while ≥2 refreshes behind, every picture was skipped; a game slightly over budget showed ~4 pictures a second (o3ds-log-0: 1606 of 2206 skipped). | Never skip two in a row (`renderer.c`, `0089b28`). |
    | O3DS: title stays white (normal build), correct but slow (probe build) | Probably the same skip starvation: the last picture before falling behind was the title's white flash. Not reproduced in Azahar (Azahar runs the title within budget). | Skip cap `0089b28`; needs an O3DS check. |
    | `run.ps1`: "azahar exited before tNNN" | Another `run.ps1` still running killed Azahar when it finished. | One harness at a time; stop the old one first. |
    | `run.ps1` run ends after ~2 s | Without `-Holds` or `-Shots` the harness only waits `-Extra` seconds. | Always pass `-Holds` or `-Shots`. |
    | A C source turns "binary" in git, or a string has a stray tab/CR | Python edits passed through an unquoted bash heredoc turned `\0`, `\t`, `\r`, `\n` escapes into real control bytes. | Use the Edit tool, or write the script to a file with raw strings; scan for NUL/CR after scripted edits. |
    # Build on Windows
    
    Verified 2026-11-12 on Windows 11 with a **from-scratch** bootstrap and build
    (empty `build/upstream`, empty `build/kj`): 91 patches applied, 0 errors,
    `.3dsx` packaged in about 6 minutes, and the title screen checked in Azahar.
    Output: a HeartGold `.3dsx`. SoulSilver and CIA are not built yet.
    
    ## Requirements
    
    | Tool | Where | Notes |
    | --- | --- | --- |
    | devkitPro | `C:\devkitPro` | Packages: devkitARM, libctru, citro3d, 3dstools, picasso. Versions are pinned in [`upstream.lock`](../upstream.lock). Its MSYS2 (`C:\devkitPro\msys2`) supplies `bash`, `make` and Python 3.14 (`/usr/bin/python`). |
    | Git for Windows | `C:\Program Files\Git` | **Required for bootstrap.** devkitPro MSYS2's own `git` cannot fetch over HTTPS (see [pitfalls](#pitfalls)). |
    | Azahar | `C:\Program Files\Azahar\azahar.exe` | Testing only (`tools/azahar/run.ps1`). |
    | PowerShell 7 (`pwsh`) | | Runs the Azahar harness. |
    | ROM | your own verified US dump | Only needed for the data pack. Never commit ROMs, packs, saves or extracted data. |
    
    ## Build
    
    Run every build step **inside devkitPro's MSYS2 login shell**
    (`C:\devkitPro\msys2\usr\bin\bash.exe -lc "..."`). The login profile
    (`/etc/profile.d/devkit-env.sh`) sets `DEVKITPRO=/opt/devkitpro` and
    `DEVKITARM=/opt/devkitpro/devkitARM`, which the build reads to find
    `arm-none-eabi-gcc`. Use absolute `/c/...` paths throughout.
    
    From Git Bash or PowerShell:
    
    ```sh
    C:/devkitPro/msys2/usr/bin/bash.exe -lc '
      export PATH="/c/Program Files/Git/cmd:$PATH"    # Git for Windows first (HTTPS)
      K=/c/Users/<you>/workdir/kohjoh
      cd $K
      python tools/game/bootstrap.py                   # pinned decomp + every patch in patches/
      python tools/game/check_patches.py               # fails if build/upstream lacks a patch
      python tools/game/hgss_game.py --root $K/build/upstream --target 3ds --out $K/build/kj -j 32
    ' > build/kj-build.log 2>&1
    ```
    
    **`--target 3ds` is required.** The default is `host32`, a PC build for tests.
    
    A build succeeded only if the log ends with
    `packaged .../pokeheartgold.3dsx` **and** has no `error:` lines. If the
    vendored SDK/NitroSystem library fails to compile, the build prints
    `warning: the vendored library build failed; linking with libnitrosystem.a,
    libnitro.a ...` and **still packages a `.3dsx` from stale libraries**. Any
    SDK or NitroSystem change is then silently absent. Check for it:
    
    ```sh
    grep -E "error:|vendored library build failed" build/kj-build.log   # must print nothing
    ```
    
    ### Outputs (`--out build/kj`)
    
    | Path | Contents |
    | --- | --- |
    | `pokeheartgold.3dsx` | the program |
    | `pokeheartgold-3ds.elf` | symbols, linked at 0x00100000 (`arm-none-eabi-addr2line -e` for crash addresses) |
    | `pokeheartgold.build-info.txt` | commit, dirty flag, sha256, the state of every legacy flag |
    | `obj/`, `app/`, `ctr/`, `sdk/`, `gen/` | incremental objects and generated headers |
    | `link.log`, `link-<utc>.log` | linker output; unresolved symbols are reported here |
    
    The shared vendored-library trees are `build/hgss-sdk`, `build/hgss-nns`, and
    `build/NitroSDK` / `build/NitroSystem` (links into `build/upstream/lib`).
    
    ### Timing and incremental builds
    
    - Bootstrap ≈ 1 min (shallow fetch + `git am` of the patch stack).
    - First build ≈ 5 min at `-j 32`. A rebuild after a kohjoh-side change takes
      about 2 min. Game objects are skipped when they are newer than their source
      and every `-MMD` dependency and their flags match; the Nitro libraries are
      re-made by `make`.
    - Build each variant into its own `--out` (e.g. `--out $K/build/kjz
      --define KJ_G3_ZERO_TRACE`) so variants never mix objects.
    
    ## Build options
    
    ### `hgss_game.py` arguments
    
    | Argument | Meaning |
    | --- | --- |
    | `--target 3ds` | 3DS build (default `host32`: PC test build) |
    | `--game heartgold\|soulsilver` | edition (default heartgold; SoulSilver untested) |
    | `--root PATH` | the patched decomp checkout (use `$K/build/upstream`) |
    | `--out PATH` | output directory (required) |
    | `-j N` | parallel jobs (default: CPU count) |
    | `--define NAME` | add `-DNAME` to the runtime and game (repeatable) |
    | `--keep-going` | compile everything, report per-file errors, then link |
    | `--skip-prepare` | reuse the generated headers in `--root` |
    | `--sanitize` | host32 only: ASan/UBSan |
    
    ### kohjoh switches (`--define NAME`)
    
    | Define | Effect | Status |
    | --- | --- | --- |
    | `KJ_G3_PROFILE` | times each geometry opcode (`[g3prof]`); each probe costs ~390 ticks | diagnostic |
    | `KJ_G3_ZERO_TRACE` | logs the first geometry commands that leave a zero position/projection matrix (`[g3zero]`) | diagnostic |
    | `KJ_DL_NO_CACHE` | turns off the display-list cache (on by default: replays recorded vertex blocks; state commands still run) | A/B and bisects |
    | `KJ_DL_VERIFY` | with the cache on: decode hits live and compare with the recording (`[dlverify] MISMATCH`), plus `[dlwhy]`/`[dlcov]` coverage lines | diagnostic |
    
    Always pass these as `--define`. An inline variable such as `FOO=1 python ...`
    works inside the `bash.exe -lc` shell but **is dropped when MSYS2 Python is
    started directly from Git Bash**. `--define` works either way.
    
    ### Legacy environment variables (from hgss-3ds)
    
    `hgss_game.py` and `src/compat/nitro/Makefile` still read these hgss-3ds
    variables. Set them inside the `-lc` shell, for example `HGSS_PROFILE=on
    python tools/game/hgss_game.py ...`. Each one is recorded in
    `build-info.txt` and appended to the build tag. Leave them unset for normal
    builds.
    
    | Variable | Effect in kohjoh today |
    | --- | --- |
    | `HGSS_PROFILE=on` | enables frame, GPU, FIFO, H-blank, timer and sound profiling; leave unset for a baseline. Use a separate output directory for each variant. |
    | `HGSS_SND_ADPCM=1` | the alternative sample-saturation path in `port_sound_chip.c` |
    | `HGSS_A0_DIAG=1` | H-blank scan cross-check counters in `port_os.c` |
    | `HGSS_BENCH=1` | hgss-3ds benchmark chainload define; no kohjoh source uses it |
    | `HGSS_GX3D_*`, `HGSS_TEXTURE_*`, `HGSS_PALETTE_HASH` | set a define that no kohjoh source reads; no effect |
    | `HGSS_HOST_CC` | host32 compiler (default `gcc-10`) |
    | `DEVKITPRO`, `DEVKITARM` | toolchain location; set by the MSYS2 login profile |
    
    ## Changing the decomp, NitroSDK or NitroSystem
    
    `build/upstream` is generated: the pinned decomp plus `patches/` applied as
    commits. A change to it only counts once it is a patch in `patches/`:
    
    1. Edit inside `build/upstream`, keeping the DS code in `#else` under
       `#ifdef SDK_PORT_3DS`. Build and verify.
    2. Commit in `build/upstream` (identity `terezi <jules@foxtails.me>`).
    3. Export it to the stack that matches the files:
       ```sh
       cd build/upstream
       git format-patch -1 --relative=lib/NitroSystem --start-number 305 -o ../../patches/NitroSystem  # NitroSystem
       git format-patch -1 --relative=lib/NitroSDK    --start-number 606 -o ../../patches/NitroSDK     # NitroSDK
       git format-patch -1                            --start-number 2004 -o ../../patches/pokeheartgold # game
       ```
       Number new patches after the highest one in that directory on every branch.
       `wip/savestore` holds `NitroSDK/2003`.
    4. Commit the patch file in kohjoh.
    
    Bootstrap refuses an existing `build/upstream` if it has local changes, or if
    its applied-patch count differs from `patches/` (for example after switching
    branches). Then run `python tools/game/bootstrap.py --clean`, which deletes and
    re-creates it. Export your commits first; `--clean` discards them.
    `check_patches.py` compares commit subjects, so it catches a tree that is
    missing a patch. It does not catch a tree with **extra** commits from another
    branch: switch branches → `bootstrap.py --clean`.
    
    ## Data pack
    
    Not yet run from this repository (tested via hgss-3ds's identical builder):
    
    ```sh
    python tools/rompack/verify_rom.py HeartGold.nds
    python tools/rompack/extract_rom.py HeartGold.nds -o build/private/hg
    python tools/rompack/pack.py build/private/hg -o build/private/hg.pak --rom HeartGold.nds
    ```
    
    The runtime needs the `.pak` and the extractor's `manifest.json`, installed as
    `sdmc:/3ds/kohjoh/data/heartgold.pak` and `heartgold.manifest.json`. For
    SoulSilver add `--build-id soulsilver-nitrofs-1` to `pack.py`.
    
    ## Run in Azahar
    
    ```powershell
    pwsh tools/azahar/run.ps1 -Pack build\private\hg.pak -Manifest build\private\hg\manifest.json -FreshSave -Shots 10,30 -Tag boot
    ```
    
    This stages the build to Azahar's SD (`%APPDATA%\Azahar\sdmc\3ds\kohjoh\`),
    launches it, and saves screenshots and logs to `build/azahar/<tag>/`. Pass
    `-Holds` or `-Shots`: without them the harness closes Azahar after about 2 s.
    See [testing](testing.md) for scripts, held frames, dumps and profiling.
    
    ## Pitfalls
    
    | Symptom | Cause | Fix |
    | --- | --- | --- |
    | `git-remote-https.exe: error while loading shared libraries` during bootstrap | devkitPro MSYS2's `git` has a broken HTTPS helper. | Put `/c/Program Files/Git/cmd` first on `PATH` inside the `-lc` shell. |
    | `.3dsx` built but an SDK/NNS change has no effect | vendored library compile failed; link used the stale prebuilt libraries | grep the log for `error:` / `vendored library build failed` |
    | `mv`/`rm` of `build/upstream`: "Device or resource busy" | a shell's working directory is inside it | `cd` out first |
    | `build-info.txt` says `dirty: yes` with no source changes | untracked files in the repository count as dirty | harmless; commit or ignore them |
    
    More, with causes, in [troubleshooting](troubleshooting.md#windows-build-chain).
    The build tools already handle each case listed there.
       104.835 [sndprof] render_calls 6600 render_frames 3379200 stretches 26361 sndticks 19795 mix_ticks 4847589822 tick_ticks 380272486 mix_calls 26361 mix_frames 3379200 active_ch 290988 pcm8_ch 273 pcm16_ch 0 adpcm_ch 290715 psg_ch 0 cap_inst 0 dep_ch 0
       114.211 [sndprof] render_calls 7200 render_frames 3686400 stretches 28761 sndticks 21595 mix_ticks 5321582634 tick_ticks 416293780 mix_calls 28761 mix_frames 3686400 active_ch 319679 pcm8_ch 273 pcm16_ch 0 adpcm_ch 319406 psg_ch 0 cap_inst 0 dep_ch 0
       120.954 [perf] logic    us p50 79778 p95 82883 p99 83633 max 94437
       120.958 [perf] vbl_wait us p50    45 p95   169 p99 14690 max 16517
       120.959 [perf] g2d      us p50  1195 p95  4570 p99  4712 max  4799
       120.960 [perf] g3d      us p50  5509 p95  8936 p99  9093 max  9204
       120.961 [perf] gpu_wait us p50     7 p95     9 p99    11 max   125
       120.962 [perf] submit   us p50   181 p95   326 p99  3576 max  3750
       120.963 [perf] publish  us p50 10295 p95 10823 p99 11039 max 11121
       120.963 [perf] window 4: 600 publishes, emulated interval (vblanks) p50 4 max 4, misses 545
       120.964 [frame] real interval (refreshes) 0:0 1:0 2:1 3:47 4+:552; hitches 1144, presents skipped 545; frame time us p50 89639 p99 99944 max 105181
       120.964 [frame] hitches, game frame(refreshes logic/g2d/g3d ms): 1954(0 53/0/0) 1955(6 94/10/5) 1956(0 36/0/0) 1957(5 78/9/8) 1958(0 34/0/0) 1959(6 77/6/5) 1960(0 35/0/0) 1961(5 79/9/5) 1962(0 35/0/0) 1963(5 79/9/8) 1964(0 36/0/0) 1965(6 79/6/5)
       120.964 [frame]   1954: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       120.964 [frame]   1955: tile conv 0, spans 1, quads 1211, tex uploads 0, tex bytes compared 46592
       120.964 [frame]   1956: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       120.965 [frame]   1957: tile conv 0, spans 1, quads 1211, tex uploads 0, tex bytes compared 46592
       120.965 [frame]   1958: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       120.965 [frame]   1959: tile conv 0, spans 1, quads 1211, tex uploads 3, tex bytes compared 46592
       120.965 [frame]   1960: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       120.965 [frame]   1961: tile conv 0, spans 1, quads 1211, tex uploads 0, tex bytes compared 46592
       120.965 [frame]   1962: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       120.965 [frame]   1963: tile conv 0, spans 1, quads 1211, tex uploads 0, tex bytes compared 46592
       120.965 [frame]   1964: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       120.965 [frame]   1965: tile conv 0, spans 1, quads 1211, tex uploads 0, tex bytes compared 46592
       120.965 [gpu] 600 frames: 2D screens skipped 600, layers reused 4200 rebuilt 0, bg quads 514800 obj quads 211800 draws 0 spans 600 unsupported 0 dropped 0, char KiB compared 13012
       120.966 [g3sample] every 64th FIFO call: samples 17515 total_ticks 162606690 decode_ticks 73568774 replay_ticks 40985138; remainder includes lookup/recording and probe overhead
       120.966 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
       120.966 [dlcov] bypass: small 2271680 split 9514950 open-begin 0 (begin first 0)
       120.966 [dlcov] words: short 11786630 new 0 changed 226710 never 0 hit 43778393 keymiss 2958217
       120.966 [ds] per game frame (1145 game frames / 600 pictures): g3 fifo calls 979 (51310 fifo words, 55 direct words, 6884 cmds), timer sync 41 us (4 calls), sound pump 6 us, hblank 2556 us
       120.966 [ds] display lists: 80196 replayed, 20564 decoded, 0 uncacheable, 835 KiB retained
       120.966 [pace] 600 frames: late V-blanks 2264, resyncs 26, presents skipped 545
       120.967 [gpu] 600 frames: atlas lookups 0 conversions 0 evictions 0 full 0; 3D batches 74323 draws 74323 tris 5022000 tex uploads 326 tex bytes compared 27789312 tex full 0 unsupported 600; box tests 40200 culled 33600 (far only 0)
       120.967 [gputime] 600 pictures: GSP us/picture P3D 2626 PPF 445 PSC 309 (log on); C3D drawing ms avg 3.99 max 6.92; cmdbuf use max 8%
       123.582 [sndprof] render_calls 7800 render_frames 3993600 stretches 31152 sndticks 23394 mix_ticks 5784406252 tick_ticks 451644580 mix_calls 31152 mix_frames 3993600 active_ch 347325 pcm8_ch 273 pcm16_ch 0 adpcm_ch 347052 psg_ch 0 cap_inst 0 dep_ch 0
       132.959 [sndprof] render_calls 8400 render_frames 4300800 stretches 33552 sndticks 25194 mix_ticks 6245758548 tick_ticks 486372186 mix_calls 33552 mix_frames 4300800 active_ch 375237 pcm8_ch 273 pcm16_ch 0 adpcm_ch 374964 psg_ch 0 cap_inst 0 dep_ch 0
       142.336 [sndprof] render_calls 9000 render_frames 4608000 stretches 35942 sndticks 26993 mix_ticks 6659385702 tick_ticks 519669684 mix_calls 35942 mix_frames 4608000 active_ch 399850 pcm8_ch 273 pcm16_ch 0 adpcm_ch 399577 psg_ch 0 cap_inst 0 dep_ch 0
       151.708 [sndprof] render_calls 9600 render_frames 4915200 stretches 38342 sndticks 28793 mix_ticks 7131614334 tick_ticks 556106096 mix_calls 38342 mix_frames 4915200 active_ch 428517 pcm8_ch 273 pcm16_ch 0 adpcm_ch 428244 psg_ch 0 cap_inst 0 dep_ch 0
       161.084 [sndprof] render_calls 10200 render_frames 5222400 stretches 40733 sndticks 30592 mix_ticks 7592245874 tick_ticks 590598226 mix_calls 40733 mix_frames 5222400 active_ch 455947 pcm8_ch 273 pcm16_ch 0 adpcm_ch 455674 psg_ch 0 cap_inst 0 dep_ch 0
       170.460 [sndprof] render_calls 10800 render_frames 5529600 stretches 43133 sndticks 32392 mix_ticks 8037222722 tick_ticks 625382966 mix_calls 43133 mix_frames 5529600 active_ch 482722 pcm8_ch 273 pcm16_ch 0 adpcm_ch 482449 psg_ch 0 cap_inst 0 dep_ch 0
       172.279 [perf] logic    us p50 79630 p95 83049 p99 83745 max 93374
       172.280 [perf] vbl_wait us p50    45 p95    85 p99 13517 max 16479
       172.281 [perf] g2d      us p50  1192 p95  4489 p99  4703 max  4821
       172.282 [perf] g3d      us p50  5523 p95  8890 p99  9048 max  9112
       172.283 [perf] gpu_wait us p50     7 p95     9 p99     9 max   132
       172.284 [perf] submit   us p50   181 p95   295 p99  3096 max  3811
       172.285 [perf] publish  us p50 10099 p95 10766 p99 10936 max 11051
       172.285 [perf] window 5: 600 publishes, emulated interval (vblanks) p50 4 max 4, misses 550
       172.289 [frame] real interval (refreshes) 0:0 1:0 2:1 3:47 4+:552; hitches 1149, presents skipped 550; frame time us p50 89359 p99 102486 max 107641
       172.289 [frame] hitches, game frame(refreshes logic/g2d/g3d ms): 3099(0 50/0/0) 3100(6 93/9/8) 3101(0 36/0/0) 3102(5 80/6/5) 3103(0 39/0/0) 3104(5 82/6/5) 3105(0 39/0/0) 3106(6 79/10/8) 3107(0 36/0/0) 3108(5 80/9/8) 3109(0 36/0/0) 3110(6 79/9/5)
       172.289 [frame]   3099: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       172.289 [frame]   3100: tile conv 0, spans 1, quads 1211, tex uploads 0, tex bytes compared 46592
       172.290 [frame]   3101: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       172.290 [frame]   3102: tile conv 0, spans 1, quads 1211, tex uploads 0, tex bytes compared 46592
       172.290 [frame]   3103: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       172.290 [frame]   3104: tile conv 0, spans 1, quads 1211, tex uploads 1, tex bytes compared 46592
       172.290 [frame]   3105: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       172.290 [frame]   3106: tile conv 0, spans 1, quads 1211, tex uploads 0, tex bytes compared 46592
       172.290 [frame]   3107: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       172.290 [frame]   3108: tile conv 0, spans 1, quads 1211, tex uploads 0, tex bytes compared 46592
       172.290 [frame]   3109: tile conv 0, spans 0, quads 0, tex uploads 0, tex bytes compared 0
       172.290 [frame]   3110: tile conv 0, spans 1, quads 1211, tex uploads 3, tex bytes compared 46592
       172.290 [gpu] 600 frames: 2D screens skipped 600, layers reused 4200 rebuilt 0, bg quads 514800 obj quads 211800 draws 0 spans 600 unsupported 0 dropped 0, char KiB compared 13012
       172.291 [g3sample] every 64th FIFO call: samples 17592 total_ticks 164519660 decode_ticks 72465048 replay_ticks 37517238; remainder includes lookup/recording and probe overhead
    src/compat/nitro/Makefile:203:CFLAGS := $(PORT_CFLAGS) $(SDK_INCLUDES) -I../pak/include
    src/compat/nitro/Makefile:205:  CFLAGS += -DHGSS_PALETTE_HASH=1
    src/compat/nitro/Makefile:208:  CFLAGS += -DHGSS_GX3D_HOTLOOP=1
    src/compat/nitro/Makefile:211:  CFLAGS += -DHGSS_GX3D_LISTCACHE=1
    src/compat/nitro/Makefile:214:  CFLAGS += -DHGSS_GX3D_STATEDEDUP=1
    src/compat/nitro/Makefile:217:  CFLAGS += -DHGSS_GX3D_FUSEHASH=1
    src/compat/nitro/Makefile:220:  CFLAGS += -DHGSS_GX3D_LAZYINSERT=1
    src/compat/nitro/Makefile:223:  CFLAGS += -DHGSS_GX3D_BG3SLOT=1
    src/compat/nitro/Makefile:226:  CFLAGS += -DHGSS_SND_ADPCM=1
    src/compat/nitro/Makefile:229:  CFLAGS += -DHGSS_A0_DIAG=1
    src/compat/nitro/Makefile:251:    @$(CC) $(CFLAGS) -c $< -o $@
    src/compat/nitro/Makefile:256:    @$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
    src/compat/nitro/Makefile:267:    @$(CC) $(PORT_CFLAGS) -I$(DEVKITPRO)/libctru/include -c $< -o $@
    src/compat/nitro/Makefile:272:    @$(CC) $(CFLAGS) -c $< -o $@
    src/compat/nitro/Makefile:279:    @$(CC) $(CFLAGS) -USDK_ARM9 -DSDK_ARM7 -include nitro/misc.h -include stdint.h -c $< -o $@
    src/compat/nitro/Makefile:301:    $(CC) $(PORT_CFLAGS) $(LDFLAGS) -Iinclude tests/test_sound_chip.c tests/sound_chip_reference.c src/port_sound_chip.c -o $@
    src/compat/nitro/Makefile:307:    $(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB) -lm -o $@
    src/compat/nitro/Makefile:331:    $(CC) $(CFLAGS) $(NNS_INCLUDES) -DNITRO_SOUND_STATS $(LDFLAGS) $< src/port_sound_chip.c $(SDAT_VOLUME_TABLE) \
    src/compat/nitro/Makefile:338:    $(CC) $(CFLAGS) $(NNS_INCLUDES) $(LDFLAGS) $< $(SDAT_VOLUME_TABLE) \
    src/compat/nitro/Makefile:345:    $(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB) -lm -o $@
    src/compat/nitro/Makefile:354:    $(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB) -lm -o $@
    src/compat/nitro/Makefile:366:    $(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB) -lm -o $@
    src/compat/nitro/Makefile:373:    $(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB) -lm -o $@
    src/compat/nitro/Makefile:379:    $(CC) $(CFLAGS) $(LDFLAGS) $< tests/render2d_reference.c $(LIB) -lm -o $@
    src/compat/nitro/Makefile:387:    $(CC) $(CFLAGS) $(NNS_INCLUDES) $(LDFLAGS) tests/test_gx3d.c $(NNS)/libraries/g3d/src/gecom.c $(LIB) -lm -pthread -o $@
    src/compat/nitro/Makefile:393:    $(CC) $(CFLAGS) $(NNS_INCLUDES) $(LDFLAGS) tests/test_vram_transfer.c $(NNS)/libraries/gfd/src/VramTransferMan/gfd_VramTransferManager.c $(LIB) -lm -o $@
    src/compat/nitro/Makefile:398:    $(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB) -lm -o $@
    src/compat/nitro/Makefile:404:    $(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB) -lm -o $@
    src/compat/nitro/Makefile:410:    $(CC) $(PORT_CFLAGS) $(LDFLAGS) -Iinclude tests/test_options.c src/port_options.c -o $@
    src/compat/nitro/Makefile:416:    $(CC) $(PORT_CFLAGS) $(LDFLAGS) -Iinclude tests/test_upscale.c src/port_upscale.c -o $@
    src/compat/nitro/Makefile:425:    $(CC) $(PORT_CFLAGS) $(if $(filter host32,$(TARGET)),-msse2 -mfpmath=sse) $(LDFLAGS) -Iinclude tests/test_xbrz.c src/port_xbrz.c -lm -o $@
    src/compat/nitro/Makefile:431:    $(CC) $(PORT_CFLAGS) $(LDFLAGS) -Iinclude tests/test_shadow.c src/port_shadow.c -lm -o $@
    src/compat/nitro/Makefile:438:    $(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB) -lm -pthread -o $@
    tools/game/hgss_game.py:214:    # kohjoh default is OFF; HGSS_PROFILE=on compiles the PROFILE build
    tools/game/hgss_game.py:222:        flags += ["-DHGSS_PROFILE_OFF"]
    tools/game/hgss_game.py:373:            *(["-DHGSS_PROFILE_OFF"] if profile_mode() == "off" else []),
    tools/game/hgss_game.py:1534:    return "profile" if os.environ.get("HGSS_PROFILE") == "on" else "off"
    tools/game/hgss_game.py:1645:                    help="extra preprocessor define for the runtime and game (e.g. KJ_G3_PROFILE)")
    ✓ • 588ms

## Assistant

I found a more consequential measurement bug: the “logic” percentile combines skipped and displayed game frames. That 80 ms median often covers two game frames; the steady window contains 1,145 game frames but only 600 pictures. It cannot be read as 80 ms per game frame.

I’m fixing the profiler to report CPU work for every game frame, separating skips from display hitches, and removing the per-scanline timing probes from ordinary profiling. The existing evidence still points to FIFO decode/cache replay and audio as substantial costs.

## Activity

    $ Get-Content src/diagnostics/perf.h; Get-Content C:/devkitPro/libctru/include/3ds/services/gspgpu.h | Select-String 'PerfLog' -Context 15,35; rg -n 'PROFILE|HBlankSplit' src/compat/nitro/src/port_os.c; Get-Content docs/testing.md -First 105; Get-Content tools/game/hgss_game.py | Select-Object -Skip 350 -First 38
    #ifndef KJ_DIAGNOSTICS_PERF_H
    #define KJ_DIAGNOSTICS_PERF_H
    
    #include <stdint.h>
    
    /* Per-frame stage timing in ARM11 ticks. PUBLISH is the parent of G2D, G3D,
     * GPU_WAIT and SUBMIT: never add it to them. G2D is reported exclusive of
     * G3D (the 3D draw is nested in the screen draw). LOGIC excludes V-blank
     * waits. */
    enum kj_perf_stage {
        KJ_PERF_LOGIC,
        KJ_PERF_VBLANK_WAIT,
        KJ_PERF_G2D,
        KJ_PERF_G3D,
        KJ_PERF_GPU_WAIT,
        KJ_PERF_SUBMIT,
        KJ_PERF_PUBLISH,
        KJ_PERF_STAGE_COUNT
    };
    
    /* Per-frame event counts, reported with each logged hitch. */
    enum kj_perf_count {
        KJ_COUNT_TILE_CONV,  /* atlas tile conversions */
        KJ_COUNT_SPANS,      /* 2D spans drawn */
        KJ_COUNT_QUADS,      /* 2D quads built */
        KJ_COUNT_TEX_UPLOAD, /* 3D texture conversions */
        KJ_COUNT_TEX_BYTES,  /* 3D texture bytes compared */
        KJ_COUNT_COUNT
    };
    #ifndef HGSS_PROFILE_OFF
    extern uint32_t kj_perf_counts[KJ_COUNT_COUNT];
    #define kj_perf_count(c, n) (kj_perf_counts[(c)] += (uint32_t)(n))
    
    void kj_perf_vblank_begin(void);
    void kj_perf_vblank_end(void);
    void kj_perf_publish_begin(void);
    /* game_frame: the published game frame (for locating hitches); call after
     * the frame is submitted. */
    void kj_perf_publish_end(uint32_t game_frame);
    /* A publish the renderer skipped (counted as a hitch). */
    void kj_perf_present_skipped(uint32_t game_frame);
    uint64_t kj_perf_now(void);
    void kj_perf_add(enum kj_perf_stage stage, uint64_t ticks);
    
    #else
    #define kj_perf_count(c, n) ((void)0)
    #define kj_perf_vblank_begin() ((void)0)
    #define kj_perf_vblank_end() ((void)0)
    #define kj_perf_publish_begin() ((void)0)
    #define kj_perf_publish_end(frame) ((void)0)
    #define kj_perf_present_skipped(frame) ((void)0)
    #define kj_perf_now() (0ull)
    #define kj_perf_add(stage, ticks) ((void)0)
    #endif
    
    #endif
    
      /**
       * GSPGPU performance log entry.
       *
       * Use the lastDurationUs field when benchmarking single GPU operations, this is usally meant
       * for 3D library writers.
       *
       * Use the difference between two totalDurationUs when using a GPU library (e.g. citro3d), as
       * there can be multiple GPU operations (e.g. P3D, PPF) per render pass, or per frame, and so on.
       * Don't use totalDurationUs as-is (rather, take the difference as just described), because it
       * can overflow.
       */
      typedef struct
      {
          u32 lastDurationUs;    ///< Duration of the last corresponding PICA200 operation (time between op is started and
    IRQ is received).
          u32 totalDurationUs;   ///< Sum of lastDurationUs for the corresponding PICA200 operation. Can overflow.
    > } GSPGPU_PerfLogEntry;
    
      /// GSPGPU performance log
      typedef struct
      {
    >     GSPGPU_PerfLogEntry entries[GSPGPU_EVENT_MAX]; ///< Performance log entries (one per operation/"event").
    > } GSPGPU_PerfLog;
    
      /**
       * @brief Gets the number of bytes per pixel for the specified format.
       * @param format See \ref GSPGPU_FramebufferFormat.
       * @return Bytes per pixel.
       */
      static inline unsigned gspGetBytesPerPixel(GSPGPU_FramebufferFormat format)
      {
          switch (format)
          {
              case GSP_RGBA8_OES:
                  return 4;
              default:
              case GSP_BGR8_OES:
                  return 3;
              case GSP_RGB565_OES:
              case GSP_RGB5_A1_OES:
              case GSP_RGBA4_OES:
                  return 2;
          }
      }
    
      /// Initializes GSPGPU.
      Result gspInit(void);
    
      /// Exits GSPGPU.
      void gspExit(void);
    
      /**
       * @brief Gets a pointer to the current gsp::Gpu session handle.
       * @return A pointer to the current gsp::Gpu session handle.
       */
      Handle *gspGetSessionHandle(void);
    
      /// Returns true if the application currently has GPU rights.
    
      /**
       * @brief Sets 3D_LEDSTATE to the input state value.
       * @param disable False = 3D LED enable, true = 3D LED disable.
       */
      Result GSPGPU_SetLedForceOff(bool disable);
    
      /**
       * @brief Enables or disables the performance log and clear
       *        its state to zero.
       * @param enabled Whether to enable the performance log.
       * @note  It is assumed that no GPU operation is in progress when calling this function.
       * @bug   The official sysmodule forgets to clear the "start tick" states to 0, though
       *        this should not be much of an issue (as per the note above).
       */
    > Result GSPGPU_SetPerfLogMode(bool enabled);
    
      /**
       * @brief Retrieves the performance log.
    >  * @param[out] outPerfLog Pointer to output the performance log to.
       * @note  Use the difference between two totalDurationUs when using a GPU library (e.g. citro3d), as
       *        there can be multiple GPU operations (e.g. P3D, PPF) per render pass, or per frame, and so on.
       *        Don't use totalDurationUs as-is (rather, take the difference as just described), because it
       *        can overflow.
       * @note  For a MemoryFill operation that uses both PSC0 and PSC1, take the maximum
       *        of the two "last duration" entries.
       * @note  For PDC0/PDC1 (VBlank0/1), the "last duration" entry corresponds to the time between
       *        the current PDC (VBlank) IRQ and the previous one. The official GSP sysmodule
       *        assumes both PDC0 and PDC1 IRQ happens at the same rate (this is almost always
       *        the case, but not always if user changes PDC timings), and sets both entries
       *        in the PDC0 handler.
       * @bug   The official sysmodule doesn't handle the PDC0/1 entries correctly after init. On the first
    >  *        frame \ref GSPGPU_SetPerfLogMode is enabled, "last duration" will have a nonsensical
       *        value; and "total duration" stays nonsensical. This isn't much of a problem, except for the
       *        first frame, because "total duration" is not supposed to be used as-is (you are supposed
       *        to take the difference of this field between two time points of your choosing, instead).
       * @bug   Since it is running at approx. 3.25 GiB/s per bank, some small PSC operations might
       *        complete before the official GSP has time to record the start time.
    >  * @bug   The official sysmodule doesn't properly handle data synchronization for the perflog,
       *        in practice this should be fine, however.
       */
    > Result GSPGPU_GetPerfLog(GSPGPU_PerfLog *outPerfLog);
    22:/* libctru's tick counter, for the H-blank split (NitroPort_HBlankSplit). This
    26:#if defined(__3DS__) && !defined(HGSS_PROFILE_OFF)
    91:#ifndef HGSS_PROFILE_OFF
    122:/* The last walk's three parts, in ARM11 ticks (NitroPort_HBlankSplit). */
    124:#ifndef HGSS_PROFILE_OFF
    216:#ifndef HGSS_PROFILE_OFF
    219:#ifndef HGSS_PROFILE_OFF
    225:#ifndef HGSS_PROFILE_OFF
    237:#ifndef HGSS_PROFILE_OFF
    262:#ifndef HGSS_PROFILE_OFF
    268:#ifndef HGSS_PROFILE_OFF
    273:#ifndef HGSS_PROFILE_OFF
    300:void NitroPort_HBlankSplit(u64 *keep, u64 *dma, u64 *irq)
    306:#ifndef HGSS_PROFILE_OFF
    309:void NitroPort_HBlankSplit2(u64 *refresh, u64 *sw, u64 *cmp)
    # Testing and profiling
    
    All tools here target Azahar. Azahar is for correctness and instruction-count
    profiling; it is not an O3DS timing authority.
    
    ## Harness: `tools/azahar/run.ps1`
    
    | Option | Effect |
    | --- | --- |
    | `-Artifact` | `.3dsx` to run (default `build/kj/pokeheartgold.3dsx`) |
    | `-Pack`, `-Manifest` | stage a data pack (only needed once) |
    | `-Save path` / `-FreshSave` | install a save / delete the test save |
    | `-Script file` | scripted input (below); without it `script.txt` is removed |
    | `-Shots 10,30` | screenshots at seconds after launch |
    | `-Holds 900,5050` | screenshot when the log says game frame N is held |
    | `-Tag`, `-Timeout`, `-Extra` | output folder, hold timeout, seconds kept after the last shot |
    
    Outputs: `build/azahar/<tag>/` (PNGs, program log, Azahar log). The harness
    resizes the window and is DPI-aware so captures are complete.
    
    ## Scripts (`tests/inputs/*.txt`)
    
    ```text
    every: 10             # each press lasts 10 V-blanks
    5000 START            # press at V-blank 5000 (A B X Y L R START SELECT UP DOWN LEFT RIGHT)
    10200 TOUCH:40,130    # touch in DS bottom-screen coordinates
    dump 5000             # write the renderer's DS input for game frame 5000
    freeze 5000 3         # hold game frame 5000 on screen for 3 s
    end: 11800            # close at V-blank 11800
    ```
    
    V-blank indices count from boot; game frames are publishes (≈ V-blanks / 2).
    A scripted run ignores live input on `wip/savestore`; on `master` a live press
    can still leak in, so do not type into the Azahar window during a test.
    
    | Script | Purpose |
    | --- | --- |
    | `intro-step.txt` | intro frames 150-1650, dumps + holds (no save needed) |
    | `goldenrod-step.txt`, `goldenrod-input.txt` | bench save → Goldenrod; touch/B/DOWN checks |
    | `goldenrod-idle.txt` | stand in Goldenrod (profiling) |
    | `route34-south.txt` | walk south toward Route 34 (path not yet calibrated) |
    
    The Goldenrod save is hgss-3ds's `build/bench/heartgold.sav`.
    
    ## Reference renderer
    
    `python tools/capture/ds_reference.py dump.kjd out` renders the DS 2D picture
    of a dump straight from GBATEK (no shared code with the GPU path) to
    `out_A.png`, `out_B.png`, `out_screens.png`. It does not render 3D (BG0 3D is
    left transparent). Dumps are written to `sdmc:/3ds/kohjoh/dumps/`.
    
    `python tools/capture/g3_reference.py dump.kjd out.png` projects the dump's
    decoded 3D scene with plain float math (one flat colour per batch, DS-style
    depth, triangles with a vertex at w <= 0 counted and skipped). Present here but
    missing on the 3DS: the GPU stage (`src/graphics/g3d`). Missing here too: the
    decoder or the game's command stream (`src/runtime/kj_g3.c`, SDK/NNS
    patches). `--define KJ_G3_ZERO_TRACE` logs the first commands that leave a
    zero position/projection matrix (`[g3zero]`).
    
    ## DS oracle
    
    melonDS (`workdir/melonDS.exe`, source in `workdir/melonDS`) runs the retail
    ROM. `tools/melonds/shots.ps1` saves timed screenshots to
    `build/melonds/<tag>/`; the HG title shows Ho-Oh from about 85 s. Check
    `melonDS/src/GPU3D.cpp` before changing 3D semantics.
    
    ## Script extras
    
    - `9600-11400 DOWN B`: hold keys over a V-blank range.
    - `debug: 0x1|0x2|0x4|0x8|0x10`: 3D draw without cull / depth test / alpha
      test / blending / texture (`KJ_DEBUG_*` in `src/app/script.h`).
    - `[frame]` lines report real refresh intervals (0/1/2/3/4+) and hitches with
      per-stage times and counters. Azahar's "App FPS" is not a stutter measure
      (unchanged screens are not presented).
    - A crash writes `logs/crash.txt` (pc, lr, registers) before the system
      error handler; resolve addresses with addr2line on the `.elf`.
    
    ## Profiling
    
    Build the baseline with `HGSS_PROFILE` unset and the diagnostic build with
    `HGSS_PROFILE=on`, using a separate `--out` for each. The build-info sidecar
    records `profiling` and any `--define` switches. The baseline omits the
    frame, GPU, FIFO and mixer profiling probes; ordinary lifecycle and error
    logging remains enabled. Log and crash files append across launches, with a
    `--- new run ---` separator in the main log. Scripted frame dumps still use
    their frame-number filenames and can overwrite earlier dumps.
    The instrumented build includes per-H-blank timing probes, which add
    substantial overhead; use the normal build for performance comparisons.
    
    `[g3sample]` samples one in 64 FIFO calls. Its total, live decode and replay
    values are elapsed ARM11 ticks for those samples, not totals for the whole
    window. The remainder includes cache lookup, comparison, recording and probe
    overhead; preemption is included. The fixed sampling stride can correlate
    with repeated call patterns, so do not multiply by 64 and treat that as an
    exact total. It does not separately time game-side model traversal or replay
    state and copying.
    
    - **Counters**: every 600 *presented* frames the log gets `[perf]` (stage p50/p95/p99/max),
      `[pace]` (late V-blanks, resyncs, skipped presents), `[gpu]` (layers
      reused/rebuilt, screens skipped, unsupported counts) and `[ds]` (geometry
      words, H-blank cost, display-list cache). `[ds] per game frame` divides by
      the H-blank walk count (one walk per game frame) and prints it with the
      picture count, so skipped presents no longer inflate it. `[perf] g2d` is
      exclusive: the g3d time spent inside it is subtracted (before `6191966`
      g2d included g3d). `[dlcov]` (with `KJ_DL_VERIFY`) splits FIFO words into
                   compat / "stubs" / "dsprot_stubs.c",
                   compat / "stubs" / "mb_stubs.c"]
        sources += sorted((REPO / "src" / "runtime").glob("*.c"))
        return sources
    
    
    #: 3DS-side directories: compiled with libctru's ABI (short enums), never
    #: with NitroSDK headers. They talk to the game only through
    #: src/runtime/kj_bridge.h (fixed-width C types only).
    CTR_SOURCE_DIRS = ("app", "graphics", "audio", "storage", "content", "diagnostics")
    
    
    def ctr_sources() -> list[Path]:
        out: list[Path] = []
        for d in CTR_SOURCE_DIRS:
            out += sorted((REPO / "src" / d).rglob("*.c"))
        return out
    
    
    def ctr_cflags(out: Path) -> list[str]:
        return ["-std=gnu11", "-O2", "-g", *WARNINGS, *SECTION_FLAGS,
                "-Wno-unused-function", *ARM_FLAGS, *game_defines(),
                *(["-DHGSS_PROFILE_OFF"] if profile_mode() == "off" else []),
                *[f"-D{d}" for d in EXTRA_DEFINES],
                f"-I{devkitpro() / 'libctru' / 'include'}",
                f"-I{REPO / 'src'}", f"-I{REPO / 'src' / 'runtime'}", f"-I{out / 'gen'}"]
    
    
    def shader_sources() -> list[Path]:
        return sorted((REPO / "src" / "graphics" / "pica" / "shaders").glob("*.v.pica"))
    
    
    def shader_objects(out: Path) -> list[Path]:
        """One object per .v.pica: picasso -> .shbin -> bin2s -> as. Each blob is
        exported as <stem>_shbin / <stem>_shbin_size (stem with dots -> '_')."""
        objs = []
        for src in shader_sources():
            objs.append(_one_shader_object(out, src))
    
    ✓ • 510ms

## Activity

    $ python -c "import re,pathlib; s=pathlib.Path('docs/chatlog08.md').read_text(encoding='utf-8'); s=re.sub(r'(?ms)^## Activity.*?(?=^## |\Z)','',s); print(s[-55000:])"
    # Codex conversation
    
    ## User
    
    you're working on kohjoh. first, read the project documentation and commit history to get yourself up to speed. then, read chatlog07, read C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2 (and n3ds-log-2) and continue diagnosing what's still hot. refer to workdir/coffeepot/ctr for 3ds technical knowledge. you should also make our logs nondestructive, so it's easier and faster for me to test multiple builds in sequence. come back to me with two builds, one instrumented and one uninstrumented - make sure the uninstrumented build is genuinely uninstrumented, and vice versa.
    
    ## Assistant
    
    I’ll get up to speed on the project and recent investigation first, then trace the remaining hot paths and make the logging changes before producing and verifying both builds.
    
    ## Assistant
    
    The supplied logs confirm the earlier diagnosis: the severe windows are dominated by time inside game logic, while GPU wait and retained 2D work are small. I’m checking the existing probes carefully because the current “normal” build still includes several timing and counter facilities, so simply omitting the old profiling flags would not produce a truly uninstrumented comparison.
    
    ## Assistant
    
    The logging code currently truncates both the main log and the crash log on startup or crash. I’ve changed those to append, with a run separator in the main log. The larger issue is that profiling is spread across the renderer, frame bridge, geometry decoder, and audio mixer; I’m checking build isolation before treating either binary as a valid baseline.
    
    ## Assistant
    
    The first build attempt hit a sandbox filesystem restriction while rebuilding the vendored libraries, so its output cannot be trusted even if packaging succeeds. I’m requesting the required unrestricted build retry; the build system can otherwise silently link stale libraries.
    
    ## Assistant
    
    The two requested builds are **not ready**. I found that the existing normal build still collects frame, geometry, GPU and audio measurements, so omitting the profiling flags would not produce a genuinely uninstrumented baseline. I have not yet separated those probes or added the measurements needed to identify the remaining CPU hotspot.
    
    The supplied hardware logs support the diagnosis in `chatlog07`: the worst Goldenrod windows spend roughly 122–144 ms in game logic, while GPU drawing is around 3–4 ms per picture and GPU wait is negligible. They do not yet distinguish game-side geometry work from FIFO translation and cache replay.
    
    I changed [log.c](C:/Users/terezi/workdir/kohjoh/src/diagnostics/log.c) and [crash.c](C:/Users/terezi/workdir/kohjoh/src/diagnostics/crash.c) to append instead of truncating existing logs. The main log gets a separator for each run. These changes have **not been verified in a completed build or on hardware**; frame dumps can still overwrite files with the same names.
    
    The first build attempt hit a sandbox filesystem restriction and reported that it could link stale vendored libraries. An unrestricted retry was approved, but I stopped it before completion. No build artifact should be used as the requested test pair.
    
    ## User
    
    well, then get back in there and make sure our normal build isn't collecting those things (and our instrumented build is collecting all of them). i approved your sandbox, you can keep working.
    
    ## Assistant
    
    I’ll finish the separation and verify the actual binaries and build logs. I’ll keep the existing diagnostic output in the instrumented variant and remove its hot-path collection from the normal variant, then add the missing geometry timing needed for the next hardware run.
    
    ## Assistant
    
    The split is now being wired through both the application and vendored sound library. One important detail is that some apparent counters also control renderer behavior, so I’m retaining those functional state variables while removing measurement-only increments and tick reads from the normal build.
    
    ## Assistant
    
    I found and removed another hidden source of baseline overhead: timer synchronization was reading the system clock twice per call even with the old profiling switch off. The instrumented build now has sampled FIFO timing split between live decoding and cache replay, alongside the existing frame, GPU and audio reports; the sample’s remainder includes cache lookup and recording.
    
    ## Assistant
    
    The normal variant has compiled and linked once without vendored-library errors. I’m rebuilding it after the final probe removals, then I’ll build the instrumented variant in its own output tree and inspect both executables for the expected diagnostic code and build metadata.
    
    ## Assistant
    
    The normal rebuild completed cleanly, and the instrumented build is now compiling in a separate tree. I also fixed the Azahar harness so it preserves the accumulated log and reads only the current run when waiting for scripted frames.
    
    ## Assistant
    
    The binary inspection so far shows the normal executable has no frame, sound, GPU-time or sampled-FIFO report strings and no mixer or FIFO sampling counters. Ordinary startup/error logging remains, and timing needed for game pacing remains active. The new FIFO samples are deliberately coarse; they do not yet isolate model traversal or split replay into state handling versus copying.
    
    ## Assistant
    
    Both variants have linked successfully, and the instrumented executable contains the expected frame, GPU, audio and FIFO-sampling reports. I found a few more measurement-only counters during the binary audit, so I’m rebuilding once more with those removed from the normal path before the final comparison.
    
    ## Assistant
    
    The final normal binary now passes the symbol and report-string checks: the profiling state and the frame, sound, GPU and FIFO report strings are absent. The instrumented binary is being refreshed from the same final source state; both builds have kept the display-list cache enabled.
    
    ## Assistant
    
    The final audit caught two pacing statistics that were still incrementing in the baseline. They’re now gated as well; the pacing decisions themselves still use the same functional timing state. I’m completing the last normal rebuild and checksum checks.
    
    ## Assistant
    
    The instrumented build is packaged and its ELF contains the expected reports and profiling state. The normal rebuild is in its final link preparation; I’ll check the finished artifact and its build metadata before handing over the files.
    
    ## Assistant
    
    The two HeartGold builds are ready:
    
    - [Normal build](C:/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold.3dsx) — profiling off; use this for performance comparisons.
    - [Instrumented build](C:/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold.3dsx) — frame, GPU, audio, H-blank, timer and sampled FIFO reporting enabled.
    
    Both builds completed without vendored-library errors, and both have the display-list cache enabled. I checked the linked executables: the normal build lacks the profiling report strings and sampled FIFO, mixer, frame and pacing profiling state; the instrumented build contains the expected reports and counters. The instrumented build has substantial H-blank probe overhead, so its frame times are diagnostic rather than a performance baseline. Neither build has been run on hardware or in Azahar since these changes.
    
    The instrumented build adds `[g3sample]`, which samples one in 64 FIFO calls and separates live decoding from cache replay. It should help narrow the CPU cost, but **does not yet isolate game-side model traversal or split replay state handling from vertex copying**. The existing hardware logs still point to game logic as the dominant cost: the worst Old 3DS windows show roughly 122–144 ms median logic time while GPU drawing is only a few milliseconds per picture.
    
    Main and crash logs now append across runs; the main log marks each launch with `--- new run ---`. The Azahar harness also preserves the accumulated log. Scripted frame dumps still overwrite an existing dump with the same frame number.
    
    ✓ • 407ms

## Activity

    $ rg -n 'HBlankSplit2|HBlankIrqByBit|HBlankSplit\(' src --glob '!port_os.c'; Get-Content docs/building-windows.md -First 50; Get-Content src/app/main.c | Select-Object -Skip 38 -First 35; Get-Content C:/devkitPro/libctru/source/gpu/gspgpu.c -ErrorAction SilentlyContinue | Select-String 'PerfLog' -Context 1,6
    src\compat\nitro\include\nitro_port.h:198:void NitroPort_HBlankSplit(u64 *keep, u64 *dma, u64 *irq);
    src\compat\nitro\include\nitro_port.h:220:void NitroPort_HBlankIrqByBit(NitroPortIrqTime *out);
    # Build on Windows
    
    Verified 2026-11-12 on Windows 11 with a **from-scratch** bootstrap and build
    (empty `build/upstream`, empty `build/kj`): 91 patches applied, 0 errors,
    `.3dsx` packaged in about 6 minutes, and the title screen checked in Azahar.
    Output: a HeartGold `.3dsx`. SoulSilver and CIA are not built yet.
    
    ## Requirements
    
    | Tool | Where | Notes |
    | --- | --- | --- |
    | devkitPro | `C:\devkitPro` | Packages: devkitARM, libctru, citro3d, 3dstools, picasso. Versions are pinned in [`upstream.lock`](../upstream.lock). Its MSYS2 (`C:\devkitPro\msys2`) supplies `bash`, `make` and Python 3.14 (`/usr/bin/python`). |
    | Git for Windows | `C:\Program Files\Git` | **Required for bootstrap.** devkitPro MSYS2's own `git` cannot fetch over HTTPS (see [pitfalls](#pitfalls)). |
    | Azahar | `C:\Program Files\Azahar\azahar.exe` | Testing only (`tools/azahar/run.ps1`). |
    | PowerShell 7 (`pwsh`) | | Runs the Azahar harness. |
    | ROM | your own verified US dump | Only needed for the data pack. Never commit ROMs, packs, saves or extracted data. |
    
    ## Build
    
    Run every build step **inside devkitPro's MSYS2 login shell**
    (`C:\devkitPro\msys2\usr\bin\bash.exe -lc "..."`). The login profile
    (`/etc/profile.d/devkit-env.sh`) sets `DEVKITPRO=/opt/devkitpro` and
    `DEVKITARM=/opt/devkitpro/devkitARM`, which the build reads to find
    `arm-none-eabi-gcc`. Use absolute `/c/...` paths throughout.
    
    From Git Bash or PowerShell:
    
    ```sh
    C:/devkitPro/msys2/usr/bin/bash.exe -lc '
      export PATH="/c/Program Files/Git/cmd:$PATH"    # Git for Windows first (HTTPS)
      K=/c/Users/<you>/workdir/kohjoh
      cd $K
      python tools/game/bootstrap.py                   # pinned decomp + every patch in patches/
      python tools/game/check_patches.py               # fails if build/upstream lacks a patch
      python tools/game/hgss_game.py --root $K/build/upstream --target 3ds --out $K/build/kj -j 32
    ' > build/kj-build.log 2>&1
    ```
    
    **`--target 3ds` is required.** The default is `host32`, a PC build for tests.
    
    A build succeeded only if the log ends with
    `packaged .../pokeheartgold.3dsx` **and** has no `error:` lines. If the
    vendored SDK/NitroSystem library fails to compile, the build prints
    `warning: the vendored library build failed; linking with libnitrosystem.a,
    libnitro.a ...` and **still packages a `.3dsx` from stale libraries**. Any
    SDK or NitroSystem change is then silently absent. Check for it:
    
    ```sh
    grep -E "error:|vendored library build failed" build/kj-build.log   # must print nothing
    ```
     * V-blank N completes at physical V-blank N counted from s_base. A frame
     * that ran long does not wait: the next DS frame starts at once and catches
     * up, so the game never loses DS time against the audio, which runs on the
     * real clock (a lost V-blank is a permanent music/picture desync). A stall
     * longer than RESYNC_VBLANKS (loads, HOME) resynchronises instead of
     * fast-forwarding. */
    #define RESYNC_VBLANKS 30u
    static u32 s_base;
    static u32 s_emulated;
    static u32 s_late_vblanks, s_resyncs;
    
    static u32 real_vblanks(void)
    {
        return C3D_FrameCounter(0) - s_base;
    }
    
    u32 kj_pacing_behind(void)
    {
        const u32 real = real_vblanks();
        return real > s_emulated ? real - s_emulated : 0;
    }
    
    void kj_pacing_stats(u32 *late, u32 *resyncs)
    {
        *late = s_late_vblanks;
        *resyncs = s_resyncs;
        s_late_vblanks = s_resyncs = 0;
    }
    
    static void fatal_screen(const char *title, const char *detail)
    {
        /* No game is running: a text console on the bottom screen is fine. */
        gfxInitDefault();
        consoleInit(GFX_BOTTOM, NULL);
        printf("\x1b[1;1Hkohjoh\n\n%s\n\n%s\n\nPress START to exit.\n", title, detail != NULL ? detail : "");
    ✗ (1) • 311ms

## Activity

    $ @'
        from pathlib import Path
        p=Path('src/diagnostics/perf.c'); s=p.read_text(); s=s.replace('static unsigned s_n;', 'static unsigned s_n;\nstatic uint32_t s_game_logic[WINDOW];\nstatic unsigned s_game_n;')
        pos=s.index('static void report(void)')
        s=s[:pos]+'''/* This window counts every game frame, including skipped pictures. */
        static void record_game_logic(void)
        {
            s_game_logic[s_game_n++] = s_frame_stage[KJ_PERF_LOGIC];
            if (s_game_n == WINDOW) {
                qsort(s_game_logic, WINDOW, sizeof(s_game_logic[0]), cmp_u32);
                kj_log("[gamecpu] 600 game frames, logic us p50 %lu p95 %lu p99 %lu max %lu (includes preemption; excludes V-blank wait)",
                       (unsigned long)(s_game_logic[WINDOW / 2] / TICKS_PER_US),
                       (unsigned long)(s_game_logic[WINDOW * 95 / 100] / TICKS_PER_US),
                       (unsigned long)(s_game_logic[WINDOW * 99 / 100] / TICKS_PER_US),
                       (unsigned long)(s_game_logic[WINDOW - 1] / TICKS_PER_US));
                s_game_n = 0;
            }
        }
        
        '''+s[pos:]
        s=s.replace('    note_hitch(game_frame, 0);','''    (void)game_frame;
            record_game_logic();
            memset(s_frame_stage, 0, sizeof(s_frame_stage));
            memset(kj_perf_counts, 0, sizeof(kj_perf_counts));
            s_logic_start = svcGetSystemTick();''')
        a='''    if (s_any_publish && real_interval != 2u) {
                note_hitch(game_frame, real_interval);
            }
        '''; s=s.replace(a,''); marker='    memset(kj_perf_counts, 0, sizeof(kj_perf_counts));\n    for (int s = 0;'; s=s.replace(marker,'''    if (s_any_publish && real_interval != 2u) {
                note_hitch(game_frame, real_interval);
            }
            record_game_logic();
            memset(kj_perf_counts, 0, sizeof(kj_perf_counts));
            for (int s = 0;'''); s=s.replace('    s_logic_start = now;\n    if (++s_n', '    if (++s_n'); s=s.replace('        report();\n    }\n}', '        report();\n    }\n    s_logic_start = svcGetSystemTick();\n}'); p.write_text(s)
        p=Path('src/graphics/renderer.c'); s=p.read_text(); s=s.replace('    /* Running two or more', '    kj_perf_publish_begin();\n    /* Running two or more',1); s=s.replace('    s_skipped_last = false;\n    kj_perf_publish_begin();','    s_skipped_last = false;'); p.write_text(s)
        p=Path('src/diagnostics/perf.h'); s=p.read_text().replace('/* A publish the renderer skipped (counted as a hitch). */','/* A skipped picture: records its game CPU sample without a submission. */'); p.write_text(s)
        p=Path('src/compat/nitro/src/port_os.c'); s=p.read_text().replace('#if defined(__3DS__) && !defined(HGSS_PROFILE_OFF)', '#if defined(__3DS__) && defined(HGSS_A0_DIAG) && !defined(HGSS_PROFILE_OFF)'); s=s.replace('#ifndef HGSS_PROFILE_OFF', '#if defined(HGSS_A0_DIAG) && !defined(HGSS_PROFILE_OFF)'); p.write_text(s)
        p=Path('src/runtime/kj_g3.c'); s=p.read_text().replace('static u32 s_coarse_seq, s_coarse_n, s_coarse_depth;', 'static u32 s_coarse_seq = 0x6d2b79f5u;\nstatic u32 s_coarse_n, s_coarse_depth;'); s=s.replace('s_coarse_sample = (++s_coarse_seq & 63u) == 0;', '''/* Deterministic PRNG avoids locking the stride to a model's calls. */
                s_coarse_seq ^= s_coarse_seq << 13;
                s_coarse_seq ^= s_coarse_seq >> 17;
                s_coarse_seq ^= s_coarse_seq << 5;
                s_coarse_sample = (s_coarse_seq & 63u) == 0;''').replace('every 64th FIFO call:', 'random 1/64 FIFO calls:'); p.write_text(s)
        '@ | python
    ✓ • 507ms

## User

once you're done what you're doing, continue investigating fifo decode, cache replay and audio - figure out which problem or combination of problems is still cpu heavy on this console, and repair it.

## Assistant

I’ll continue into decode, replay, and audio after the measurement repairs, and test changes against the available correctness checks. I’ll use the hardware log to choose targets and keep emulator instruction counts separate from console timing.

## Activity

    $ Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1930 -First 130; Get-Content src/compat/nitro/src/port_sound_chip.c | Select-String 'mix_block|render_adpcm|vol|pan' -Context 2,6; rg -n 'g3|fifo|profile' tests tools | Select-Object -First 75; Get-Content src/compat/nitro/Makefile -First 65; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1340 -First 75
            }
            k->material[0] = s_diffuse;
            k->material[1] = s_ambient;
            k->material[2] = s_specular;
            k->material[3] = s_emission;
            k->use_shininess = s_use_shininess;
            if (s_use_shininess) {
                memcpy(k->shininess, s_shininess, sizeof(k->shininess));
            }
        }
    }
    
    static dl_entry *dl_slot(const uint32_t *ptr, uint32_t count)
    {
        const uint32_t h = (uint32_t)(((uintptr_t)ptr >> 2) * 2654435761u ^ count * 40503u);
        uint32_t i = h & (DL_SLOTS - 1u);
        dl_entry *victim = NULL;
        for (uint32_t n = 0; n < 8; ++n, i = (i + 1u) & (DL_SLOTS - 1u)) {
            dl_entry *e = &s_dl[i];
            if ((e->ptr == ptr && e->count == count) || e->ptr == NULL) {
                return e;
            }
            if (victim == NULL || e->last_frame < victim->last_frame) {
                victim = e;
            }
        }
        return victim;
    }
    
    /* The recording's storage stays with the entry (capacities in cap_*): a list
     * re-recorded every frame or two (keyed on changing entry state) reuses it
     * instead of filling the arena, whose flush re-records everything in one
     * frame (Goldenrod: 29.6 ms logic against 13.7, twice per 600 frames). */
    static void dl_drop_recording(dl_entry *e)
    {
        e->nevents = e->nblocks = e->nverts = e->nidx = 0;
        e->recorded = false;
    }
    
    /* Room for need elements in *ptr: the existing storage if large enough. */
    static bool dl_reserve(void **ptr, uint32_t *cap, uint32_t need, uint32_t elem)
    {
        if (need <= *cap) {
            return true;
        }
        void *p = dl_alloc(need * elem);
        if (p == NULL) {
            return false;
        }
        *ptr = p;
        *cap = need;
        return true;
    }
    
    static void dl_free(dl_entry *e)
    {
        memset(e, 0, sizeof(*e));
    }
    
    static void dl_capture_exit(dl_exit *x, uint32_t v0)
    {
        x->color = s_color;
        memcpy(x->coord, s_coord, sizeof(x->coord));
        x->texcoord[0] = s_texcoord[0];
        x->texcoord[1] = s_texcoord[1];
        x->texcoord_raw[0] = s_texcoord_raw[0];
        x->texcoord_raw[1] = s_texcoord_raw[1];
        x->in_begin = s_in_begin;
        x->primitive = s_primitive;
        x->prim_count = s_prim_count;
        x->strip_parity = s_strip_parity;
        x->prim_valid = true;
        for (int i = 0; i < 4; ++i) {
            x->prim_rel[i] = s_prim_verts[i] - v0;
            if (x->in_begin && s_prim_verts[i] < v0) {
                x->prim_valid = false;
            }
        }
    }
    
    static void decode_words_impl(const uint32_t *words, size_t count);
    
    static bool dl_append_block(const dl_entry *e, const dl_block *b, uint32_t *v_first_out)
    {
        frame_buf *fb = &s_buf[s_build];
        uint32_t gid;
    
        if (!current_gmtx(&gid)) {
            return false;
        }
        if (!batch_matches_state() && !open_batch()) {
            return false;
        }
        int slot = cached_slot(gid);
        if (slot < 0) {
            close_batch();
            if (!open_batch()) {
                return false;
            }
            slot = cached_slot(gid);
            if (slot < 0) {
                return false;
            }
        }
        const uint32_t v0 = fb->frame.vert_count;
        const uint32_t hi = (uint32_t)slot << 16;
        const uint32_t *src = (const uint32_t *)(e->verts + b->first_vert);
        uint32_t *dst = (uint32_t *)&fb->verts[v0];
        for (uint32_t i = 0; i < b->nverts; ++i) {
            dst[0] = src[0];
            dst[1] = (src[1] & 0xFFFFu) | hi;
            dst[2] = src[2];
            dst[3] = src[3];
            fb->vert_gmtx[v0 + i] = (uint16_t)gid;
            src += 4;
            dst += 4;
        }
        const uint16_t *si = e->idx + b->first_idx;
        uint16_t *ix = fb->indices + fb->frame.index_count;
        for (uint32_t i = 0; i < b->nidx; ++i) {
            ix[i] = (uint16_t)(si[i] + v0);
        }
        fb->frame.vert_count = v0 + b->nverts;
        fb->frame.index_count += b->nidx;
        s_batch->index_count += b->nidx;
        *v_first_out = v0;
        return true;
    }
    
    #ifndef HGSS_PROFILE_OFF
    
      /* Test-only frame hook (sdat_render): the mixer runs one frame per block, so
       * each event lands on its frame. Blocks give what frame-by-frame mixing gives
    >  * (see mix_block); sdat_render's release build checks that on real output. */
    > #define MIX_BLOCK 1
      static NitroSoundFrameFn sFrameFn;
      static void *sFrameArg;
      static NitroSoundFrame sFrame;
      static uint32_t sFrameIndex;
      #define STATS_EVENT(channel, bit) do { if (sFrameFn) sFrame.event[(channel)] |= (uint8_t)(bit); } while (0)
      #define STATS_CHANNEL_OF(base) (((base) - 0x400u) / 16u)
      #define STATS_TAP(c, i, v) do { if ((c)->tap) (c)->tap[(i)] = (v); } while (0)
      #else
    > #define MIX_BLOCK 128
      #define STATS_EVENT(channel, bit) ((void)0)
      #define STATS_CHANNEL_OF(base) 0u
      #define STATS_TAP(c, i, v) ((void)0)
      #endif
    
    > volatile uint8_t NitroPort_SoundRegisters[NITRO_SOUND_REG_SIZE];
    
      static unsigned char reg8(unsigned offset)
      {
          return NitroPort_SoundRegisters[offset];
      }
    
          uint32_t loopAt;
          uint64_t step;
    >     int32_t volume;
    >     int32_t pan;
          unsigned shift;
          uint32_t endByte;      /* the channel reads [sourceAddr, sourceAddr + endByte) */
          int dependent;         /* reads memory a capture writes: mixed frame by frame */
          int interpolate;       /* NitroSoundChip.interpolate, for a sampled sound */
      #ifdef NITRO_SOUND_STATS
          int32_t *tap;          /* the frame hook's samples for this channel, if set */
          c->nSamples = total_samples(c->format, endByte);
          c->loopAt = loop_sample(c->format, startByte);
    >     c->volume = (int32_t)(ctrl & 127u);
    >     c->pan = (int32_t)((ctrl >> 16) & 127u);
          shift = (ctrl >> 8) & 3u;
          c->shift = shift == 3u ? 4u : shift;
          c->interpolate = chip->interpolate && c->format != 3u;
    
          /* The step only changes with the timer: no 64-bit division (slow on
           * the 3DS's CPU, which has no divide instruction) for every sample. */
    
          if (c->interpolate) sample = lerp(v->lastSample, sample, v->phase);
    >     scaled = (sample * c->volume) / 127;
    
          scaled >>= c->shift;
    >     *left += (scaled * (127 - c->pan)) / 127;
    >     *right += (scaled * c->pan) / 127;
    
          v->phase += c->step;
          while (v->phase >= PHASE_ONE && v->running && guard++ < 2048u) {
              v->phase -= PHASE_ONE;
              advance_voice(c);
          }
       * has written its state back already), 1 if the block goes on. This is the
       * phase loop the PCM branch used to hold inline, split out so the silent
    >  * and pan-specialised loops all move the cursor in exactly one way.
       */
      static inline int pcm_advance(Channel *c, uint64_t *phase, uint32_t *index,
                                    int32_t *last, int pcm16bit, int interpolate,
                                    uint32_t nSamples)
      {
          const uint8_t *source = c->source;
       * (coffeepot cpu-optimization: look for sp-relative spills in hot loops).
       * It is split in two passes instead: adpcm_render decodes and resamples
    >  * into a local buffer, mix_scaled applies volume, shift and pan.
       *
       * The phase is kept as its 32-bit fraction. That is exact when the whole
       * part is 0 at the start of the block (each frame's whole steps are
       * consumed before the next frame) and a frame's step is under 2,048
       * samples (so the per-frame cap never applies; the largest step, timer
    >  * period 1 at 32,768 Hz, is 511). mix_block checks both and otherwise
       * mixes frame by frame. Output is bit-identical to the one-pass loop.
       */
    
      /*
       * saturate() for the render loop. ARMv6 SSAT clamps in the pipeline's
       * saturate stage (coffeepot cpu-optimization: ARM11 pipeline, ARMv6
      }
    
    > /* Samples (before volume) for up to frames frames; returns how many frames
       * the channel played (fewer if its sound ended). subStep: the caller knows
       * the step is under one sample per frame (stepWhole 0), so a frame decodes
       * at most one sample; a constant there frees registers in the loop. */
      static inline __attribute__((always_inline)) size_t adpcm_render_frames(
          Channel *c, int32_t *out, size_t frames, const int interpolate,
          const int subStep)
    
      /*
    >  * Adds rendered samples to L and R with the channel's volume, shift and
    >  * pan; values (if not NULL) gets each frame's sample after volume and
    >  * shift. The same arithmetic as voice_mix, so the same result: a hard pan
       * adds (x * 127) / 127 = x to one side and (x * 0) / 127 = 0 to the other.
       */
      static void mix_scaled(Channel *c, const int32_t *samples, int32_t *L, int32_t *R,
                             int32_t *values, size_t frames)
      {
    >     const int32_t volume = c->volume;
    >     const int32_t panL = 127 - c->pan;
    >     const int32_t panR = c->pan;
          const unsigned shift = c->shift;
          size_t i;
    
    >     if (volume == 0) {
              /* Silent: (x * 0) / 127 >> shift is 0 on both sides. */
              for (i = 0; i < frames; ++i) {
                  if (values) {
                      values[i] = 0;
                  }
                  STATS_TAP(c, i, 0);
              return;
          }
    >     if (panR == 0) {
              for (i = 0; i < frames; ++i) {
    >             const int32_t scaled = ((samples[i] * volume) / 127) >> shift;
    
                  if (values) {
                      values[i] = scaled;
                  }
                  STATS_TAP(c, i, scaled);
                  L[i] += scaled;
              }
    >     } else if (panL == 0) {
              for (i = 0; i < frames; ++i) {
    >             const int32_t scaled = ((samples[i] * volume) / 127) >> shift;
    
                  if (values) {
                      values[i] = scaled;
                  }
                  STATS_TAP(c, i, scaled);
                  R[i] += scaled;
          } else {
              for (i = 0; i < frames; ++i) {
    >             const int32_t scaled = ((samples[i] * volume) / 127) >> shift;
    
                  if (values) {
                      values[i] = scaled;
                  }
                  STATS_TAP(c, i, scaled);
    >             L[i] += (scaled * panL) / 127;
    >             R[i] += (scaled * panR) / 127;
              }
          }
      }
    
      /*
       * A channel's samples for a run of frames, added to L and R: what
       * depend on the other channels within a mix.
       */
    > static void mix_block(Channel *c, int32_t *L, int32_t *R, int32_t *values, size_t frames)
      {
          NitroSoundVoice *v = c->v;
    >     const int32_t volume = c->volume, panL = 127 - c->pan, panR = c->pan;
          const unsigned shift = c->shift;
          const uint64_t step = c->step;
          /* step is a block constant. At the port's 32,768 Hz output rate no
           * timer value makes it exactly PHASE_ONE (SOUND_CLOCK is not a
           * multiple of 32,768), so the phase loop always has work to do; a
           * single-sample-per-frame fast path would never be entered. */
          size_t i;
    
    >     if (c->format == 2u && c->source && c->nSamples && frames <= MIX_BLOCK &&
              (v->phase >> 32) == 0u && (step >> 32) < 2048u) {
              /* ADPCM: rendered, then mixed (see adpcm_render). */
    >         int32_t samples[MIX_BLOCK];
              const size_t played = adpcm_render(c, samples, frames);
    
              mix_scaled(c, samples, L, R, values, played);
              return;
          }
          if (c->format <= 1u && c->source && c->nSamples) {
    >         /* PCM8 / PCM16. One loop per pan case, hoisted out of the frame
               * loop; step and shift are already block constants. */
              const uint8_t *source = c->source;
              const uint32_t nSamples = c->nSamples;
              const int pcm16bit = c->format == 1u;
              const int interpolate = c->interpolate;
    >         const int fullVolume = volume == 127;
              uint32_t index = v->sampleIndex;
              int32_t last = v->lastSample;
    
    >         if (volume == 0) {
                  /* Silent: it adds nothing to either side, but its cursor must
                   * still move, or the sample it comes back on when the envelope
                   * lifts would be the wrong one. Only an interpolating channel
                   * reads the source here, to keep lastSample. */
                  for (i = 0; i < frames; ++i) {
                      if (values) values[i] = 0;
                          return;
                  }
    >         } else if (panL == 127) {
    >             /* Hard left: panR is 0, so the right side would add
                   * ((x * 0) / 127) = 0. */
                  for (i = 0; i < frames; ++i) {
                      int32_t sample = pcm16bit ? pcm16(source, index * 2u)
                                                : (int32_t)pcm8(source, index) * 256;
                      int32_t scaled;
    
                      /* (sample * 127) / 127 is sample exactly, and a PCM sample
                       * times 127 cannot overflow int32. */
    >                 if (fullVolume) {
                          scaled = sample >> shift;
                      } else {
    >                     scaled = (sample * volume) / 127;
                          scaled >>= shift;
                      }
                      if (values) values[i] = scaled;
                      STATS_TAP(c, i, scaled);
                      L[i] += scaled;
                      phase += step;
                          return;
                  }
    >         } else if (panL == 0) {
    >             /* Hard right: panL is 0, so the left side would add 0. */
                  for (i = 0; i < frames; ++i) {
                      int32_t sample = pcm16bit ? pcm16(source, index * 2u)
                                                : (int32_t)pcm8(source, index) * 256;
                      int32_t scaled;
    
                      if (interpolate) sample = lerp(last, sample, phase);
    >                 if (fullVolume) {
                          scaled = sample >> shift;
                      } else {
    >                     scaled = (sample * volume) / 127;
                          scaled >>= shift;
                      }
                      if (values) values[i] = scaled;
                      STATS_TAP(c, i, scaled);
                      R[i] += scaled;
                      phase += step;
    
                      if (interpolate) sample = lerp(last, sample, phase);
    >                 if (fullVolume) {
                          scaled = sample >> shift;
                      } else {
    >                     scaled = (sample * volume) / 127;
                          scaled >>= shift;
                      }
                      if (values) values[i] = scaled;
                      STATS_TAP(c, i, scaled);
    >                 L[i] += (scaled * panL) / 127;
    >                 R[i] += (scaled * panR) / 127;
                      phase += step;
                      if (!pcm_advance(c, &phase, &index, &last, pcm16bit,
                                       interpolate, nSamples))
                          return;
                  }
              }
          unsigned count = 0, channel, k;
          unsigned control;
    >     int32_t masterVolume;
          int capturing, bypass1, bypass3;
          unsigned leftFrom, rightFrom;
          size_t i;
      #ifdef NITRO_SOUND_STATS
    >     int32_t tapBuf[NITRO_SOUND_CHANNEL_COUNT][MIX_BLOCK];
      #endif
    
          if (!chip || !stereo || !chip->outputRate || frames == 0) return;
          for (channel = 0; channel < NITRO_SOUND_CHANNEL_COUNT; ++channel) {
              if (start_channel(chip, channel, &channels[count])) ++count;
          }
          for (k = 0; k < count; ++k) channels[k].tap = tapBuf[channels[k].channel];
      #endif
    >     /* SOUNDCNT (0x500): bits 0-6 master volume, 8-9 / 10-11 what the left /
           * right output plays (the mixer, channel 1, channel 3 or both), 12 / 13
           * channel 1 / 3 kept out of the mixer, 15 on. */
          control = reg8(0x501u);
    >     masterVolume = (int32_t)(reg8(0x500u) & 127u);
          leftFrom = control & 3u;
          rightFrom = (control >> 2) & 3u;
          bypass1 = (control & 0x10u) != 0;
          bypass3 = (control & 0x20u) != 0;
          capturing = ((reg8(0x508u) | reg8(0x509u)) & 128u) ||
                      chip->capture[0].running || chip->capture[1].running;
          {
              /* Channel by channel, a block of frames at a time. */
    >         enum { BLOCK = MIX_BLOCK };
              size_t at;
    
              for (at = 0; at < frames; at += BLOCK) {
                  int32_t mixL[BLOCK], mixR[BLOCK], ch1L[BLOCK], ch1R[BLOCK], ch3L[BLOCK], ch3R[BLOCK];
                  int32_t value0[BLOCK], value2[BLOCK];   /* channels 0 and 2, for captures */
                  size_t n = frames - at < BLOCK ? frames - at : BLOCK;
      #ifdef NITRO_SOUND_STATS
                  for (channel = 0; channel < NITRO_SOUND_CHANNEL_COUNT; ++channel)
    >                 for (i = 0; i < MIX_BLOCK; ++i) tapBuf[channel][i] = NITRO_SOUND_TAP_OFF;
      #endif
                  for (k = 0; k < count; ++k) {
                      Channel *c = &channels[k];
    
                      if (!c->v->running || c->dependent) continue;
    >                 if (c->channel == 1) mix_block(c, ch1L, ch1R, NULL, n);
    >                 else if (c->channel == 3) mix_block(c, ch3L, ch3R, NULL, n);
    >                 else mix_block(c, mixL, mixR,
                                     !capturing ? NULL : c->channel == 0 ? value0
                                     : c->channel == 2 ? value2 : NULL, n);
                  }
                  for (i = 0; i < n; ++i) {
                      int32_t outputL = 0, outputR = 0;
      #ifdef NITRO_SOUND_STATS
                                        : rightFrom == 2 ? ch3R[i] : ch1R[i] + ch3R[i];
    
    >                     outputL = (left * masterVolume) / 127;
    >                     outputR = (right * masterVolume) / 127;
      #ifdef NITRO_SOUND_STATS
                          preL = outputL;
                          preR = outputR;
      #endif
                          /* Headroom before the 16-bit saturation, not after it:
                           * attenuating a flat-topped sample just halves the flat
                              sFrame.tap[c->channel] = c->tap[i];
                              sFrame.format[c->channel] = (uint8_t)c->format;
    >                         sFrame.volume[c->channel] = (uint8_t)c->volume;
                              sFrame.shift[c->channel] = (uint8_t)c->shift;
                          }
                          sFrameFn(sFrameArg, &sFrame);
                          memset(sFrame.event, 0, sizeof(sFrame.event));
                      }
      #endif
    tools\azahar\gdbprof.py:6:instruction-count profile: exactly what is needed to find where the ARM11
    tools\capture\g3_reference.py:3:g3_reference.py: project the decoded 3D scene of a .kjd dump on the CPU.
    tools\capture\g3_reference.py:5:The 3DS draws the scene the geometry decoder (src/runtime/kj_g3.c) built:
    tools\capture\g3_reference.py:10:(src/graphics/g3d). Never linked into the 3DS program.
    tools\capture\g3_reference.py:16:usage: g3_reference.py dump.kjd out.png [--batch N] [--scale S]
    tools\capture\g3_reference.py:24:VERT = struct.Struct("<3hBB4B2h")    # kj_g3_vertex, 16 bytes
    tools\capture\g3_reference.py:25:BATCH = struct.Struct("<IIHHIIIII")  # kj_g3_batch, 32 bytes
    tools\capture\g3_reference.py:28:def read_g3(path):
    tools\capture\g3_reference.py:87:    swap, vp, disp3dcnt, verts, indices, mats, batches = read_g3(args.dump)
    tools\game\hgss_game.py:221:    if profile_mode() == "off":
    tools\game\hgss_game.py:373:            *(["-DHGSS_PROFILE_OFF"] if profile_mode() == "off" else []),
    tools\game\hgss_game.py:1053:    if profile_mode() == "profile":
    tools\game\hgss_game.py:1093:            f"profiling: {profile_mode()}\n"
    tools\game\hgss_game.py:1095:            f"texture_profile: {'yes' if target == '3ds' and os.environ.get('HGSS_TEXTURE_PROFILE') == '1' else 'no'}\n"
    tools\game\hgss_game.py:1104:             f"gx3d_bg3slot: {'yes' if target == '3ds' and os.environ.get('HGSS_GX3D_BG3SLOT') == '1' else 'no'}\n"
    tools\game\hgss_game.py:1529:def profile_mode() -> str:
    tools\game\hgss_game.py:1534:    return "profile" if os.environ.get("HGSS_PROFILE") == "on" else "off"
    tools\game\hgss_game.py:1537:def profile_mode_matches(out: Path) -> bool:
    tools\game\hgss_game.py:1538:    marker = out / "sdk" / ".profile-mode"
    tools\game\hgss_game.py:1539:    return marker.is_file() and marker.read_text().strip() == profile_mode()
    tools\game\hgss_game.py:1575:    bg3slot_mode = "1" if os.environ.get("HGSS_GX3D_BG3SLOT") == "1" else "0"
    tools\game\hgss_game.py:1576:    bg3slot_marker = sdk_out / ".bg3slot-mode"
    tools\game\hgss_game.py:1577:    if not bg3slot_marker.exists() or bg3slot_marker.read_text().strip() != bg3slot_mode:
    tools\game\hgss_game.py:1591:    prof_mode = profile_mode()
    tools\game\hgss_game.py:1592:    prof_marker = sdk_out / ".profile-mode"
    tools\game\hgss_game.py:1613:            bg3slot_marker.write_text(bg3slot_mode + "\n")
    tools\game\hgss_game.py:1755:    if not profile_mode_matches(out):
    tools\game\hgss_nitro.py:25:(`g3d/src/gecom.c`, `gfd/src/VramTransferMan/gfd_VramTransferManager.c`). Headers: a shim
    tools\game\hgss_nitro.py:248:    # directly (test_gx3d: g3d/src/gecom.c, test_vram_transfer: gfd/.../gfd_VramTransferManager.c).
    tools\game\hgss_nitro.py:249:    # HGSS's NNS sources are flat (lib/NitroSystem/src/g3d_*.c, ...); the shim exposes them as
    tools\game\hgss_nitro.py:258:    _link(nns / "libraries" / "g3d" / "src" / "gecom.c",
    tools\game\hgss_nitro.py:259:          root / "lib" / "NitroSystem" / "src" / "g3d_gecom.c")
    tools\game\hgss_nitro.py:288:            "-include", f"{repo}/src/compat/hgss/include/hgss_port_math.h"] + profile_defines()
    tools\game\hgss_nitro.py:294:def profile_defines():
    tools\game\make_game_wifi_stubs.py:50:    # No WFC profile / console data on the port: Comm_HasWifiProfile is FALSE
    tools\game\make_game_wifi_stubs.py:51:    # and wifi_connection.c takes its "no profile" branch.
    # ---------------------------------------------------------------------------
    # libnitro: Nintendo's NitroSDK, compiled with GCC for the 3DS (and for PC,
    # so it can be unit tested).
    #
    #   make                 build for PC and run the tests     (TARGET=host)
    #   make TARGET=host32   same, as a 32-bit program (needs gcc-multilib)
    #   make TARGET=3ds      build the library for the 3DS's ARM11 (needs devkitARM)
    #   make clean
    #
    # The SDK sources come from build/NitroSDK (run tools/bootstrap.py first).
    # Our changes to them are in patches/NitroSDK/; code that replaces parts of
    # the SDK entirely is in src/ here.
    # ---------------------------------------------------------------------------
    
    include ../common.mk
    
    OUT      := build/$(TARGET)
    GEN      := $(OUT)/gen
    
    # --- Sources ---------------------------------------------------------------
    
    # NitroSDK files compiled as they are (with our patches applied): every
    # ARM9 source of the libraries the game uses.
    SDK_SRCS := \
        card/src/card_common.c \
        card/src/card_spi.c \
        card/src/card_backup.c \
        card/src/card_rom.c \
        card/src/card_request.c \
        card/src/card_pullOut.c \
        cp/src/cp_context.c \
        fs/src/fs_command.c \
        fs/src/fs_command_default.c \
        fs/src/fs_archive.c \
        fs/src/fs_file.c \
        fs/src/fs_rom.c \
        fs/src/fs_mem.c \
        fs/src/fs_overlay.c \
        fx/src/fx_sincos.c \
        fx/src/fx_mtx22.c \
        fx/src/fx_mtx33.c \
        fx/src/fx_mtx43.c \
        fx/src/fx_mtx44.c \
        fx/src/fx_cp.c \
        fx/src/fx_vec.c \
        fx/src/fx_trig.c \
        fx/src/fx_atan.c \
        fx/src/fx_atanidx.c \
        fx/src/fx.c \
        fx/src/fx_asinacosidx.c \
        gx/src/gx.c \
        gx/src/gxstate.c \
        gx/src/gx_vramcnt.c \
        gx/src/gx_bgcnt.c \
        gx/src/g2.c \
        gx/src/g3b.c \
        gx/src/g3c.c \
        gx/src/g3imm.c \
        gx/src/g3x.c \
        gx/src/g3_util.c \
        gx/src/gx_load2d.c \
        gx/src/gx_load3d.c \
        gx/src/g3.c \
        gx/src/gxasm.c \
        math/src/math.c \
    static int s_direct_have;
    
    void NitroPort_GxDirectWrite(int opcode, uint32_t value)
    {
        const unsigned op = (unsigned)opcode & 0xFF;
        const int need = param_count(op);
    
    #ifndef HGSS_PROFILE_OFF
        ++s_direct_words;
    #endif
        if (need == 0) {
            execute(op, &value);
            return;
        }
        if (s_direct_op != op) {
            s_direct_op = op;
            s_direct_have = 0;
        }
        s_direct_params[s_direct_have++] = value;
        if (s_direct_have >= need) {
            execute(op, s_direct_params);
            s_direct_have = 0;
            s_direct_op = 0;
        }
    }
    
    /* Packed GXFIFO: a word of up to four opcodes (low byte first), then the
     * parameters of each in turn. */
    static uint32_t s_fifo_cmds;     /* remaining packed opcodes */
    static int s_fifo_left;          /* opcodes left in the packed word */
    static unsigned s_fifo_op;
    static int s_fifo_need, s_fifo_have;
    static uint32_t s_fifo_params[32];
    
    static void fifo_next_command(void)
    {
        while (s_fifo_left > 0) {
            s_fifo_op = s_fifo_cmds & 0xFF;
            s_fifo_cmds >>= 8;
            --s_fifo_left;
            s_fifo_need = param_count(s_fifo_op);
            s_fifo_have = 0;
            if (s_fifo_need > 0) {
                return;
            }
            if (s_fifo_op != 0) {
                execute(s_fifo_op, s_fifo_params);
            }
            if ((s_fifo_cmds & 0xFFFFFF) == 0) {
                /* the rest of the word is NOPs */
                s_fifo_left = 0;
            }
        }
        s_fifo_need = 0;
    }
    
    void NitroPort_GxFifoWord(uint32_t value)
    {
        if (s_fifo_need > 0) {
            s_fifo_params[s_fifo_have++] = value;
            if (s_fifo_have >= s_fifo_need) {
                execute(s_fifo_op, s_fifo_params);
                s_fifo_need = 0;
                fifo_next_command();
            }
            return;
        }
        s_fifo_cmds = value;
        s_fifo_left = 4;
        fifo_next_command();
    }
    
    
    static uint8_t s_param_count[256];
    
    
    ✓ • 612ms

## Activity

    $ rg -n 'current_gmtx|cached_slot|batch_matches_state|hot_command|case 0x' src/runtime/kj_g3.c | Select-Object -First 85; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 300 -First 190; Get-Content src/compat/nitro/src/port_sound_chip.c | Select-Object -Skip 1000 -First 125; rg --files build | rg 'diff|sound.*test|g3.*test|gdb' | Select-Object -First 35; Get-Content tools/azahar/gdbprof.py -First 95
    224:        case 0x11:
    231:        case 0x12:
    236:        case 0x13:
    239:        case 0x14:
    248:        case 0x11:
    249:        case 0x13:
    253:        case 0x12:
    254:        case 0x14:
    262:    case 0x11: /* PUSH */
    270:    case 0x12: { /* POP n (signed 6 bit) */
    281:    case 0x13: { /* STORE */
    290:    case 0x14: { /* RESTORE */
    557:static bool current_gmtx(uint32_t *out)
    669:static bool batch_matches_state(void)
    679:static inline int cached_slot(uint32_t gid)
    743:        if (!batch_matches_state() && !open_batch()) {
    769:    if (fb->frame.vert_count >= KJ_G3_MAX_VERTS || !current_gmtx(&gid)) {
    777:    if (!batch_matches_state() && !open_batch()) {
    780:    int slot = cached_slot(gid);
    786:        slot = cached_slot(gid);
    1048:    case 0x10: case 0x12: case 0x13: case 0x14:
    1049:    case 0x20: case 0x21: case 0x22: case 0x24: case 0x25: case 0x26: case 0x27: case 0x28:
    1050:    case 0x29: case 0x2A: case 0x2B: case 0x30: case 0x31: case 0x32: case 0x33:
    1051:    case 0x40: case 0x50: case 0x60: case 0x72:
    1053:    case 0x16: case 0x18:
    1055:    case 0x17: case 0x19:
    1057:    case 0x1A:
    1059:    case 0x1B: case 0x1C: case 0x70:
    1061:    case 0x23: case 0x71:
    1063:    case 0x34:
    1132:    case 0x10:
    1135:    case 0x11: case 0x12: case 0x13: case 0x14:
    1138:    case 0x15: {
    1144:    case 0x16: case 0x17: {
    1150:    case 0x18: case 0x19: case 0x1A: {
    1156:    case 0x1B:
    1159:    case 0x1C:
    1162:    case 0x20:
    1166:    case 0x21:
    1177:    case 0x22:
    1187:    case 0x23:
    1193:    case 0x24:
    1199:    case 0x25:
    1204:    case 0x26:
    1209:    case 0x27:
    1214:    case 0x28:
    1220:    case 0x29:
    1223:    case 0x2A:
    1229:    case 0x2B:
    1235:    case 0x30: {
    1250:    case 0x31: {
    1261:    case 0x32: {
    1280:    case 0x33: {
    1288:    case 0x34:
    1294:    case 0x40:
    1308:    case 0x41:
    1311:    case 0x50:
    1314:    case 0x60:
    1320:    case 0x70:
    1323:    case 0x71:
    1326:    case 0x72:
    1559:static inline __attribute__((always_inline)) void hot_command(unsigned op, const uint32_t *p)
    1563:    case 0x22:
    1570:    case 0x23:
    1575:    case 0x24: /* VTX_10 */
    1580:    case 0x25:
    1584:    case 0x26:
    1588:    case 0x27:
    1592:    case 0x28: /* VTX_DIFF */
    1597:    case 0x21: /* NORMAL without normal texgen */
    1867:            case 0x20:
    1872:            case 0x22:
    1875:            case 0x23: case 0x24:
    1879:            case 0x25: case 0x26: case 0x27: case 0x28:
    1883:            case 0x2A: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
    1884:            case 0x16: case 0x17: case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C:
    2018:    if (!current_gmtx(&gid)) {
    2021:    if (!batch_matches_state() && !open_batch()) {
    2024:    int slot = cached_slot(gid);
    2030:        slot = cached_slot(gid);
    2215:            hot_command(op, words + i);
        }
    }
    
    /* ------------------------------------------------------- vertex state */
    
    static uint16_t s_color = 0x7FFF;
    static int16_t s_coord[3];
    static int32_t s_texcoord_raw[2]; /* last TEXCOORD, 12.4 */
    static int32_t s_texcoord[2];     /* effective, 12.4 */
    
    /* material and lights */
    static uint16_t s_diffuse, s_ambient, s_specular, s_emission;
    static bool s_use_shininess;
    static uint32_t s_shininess[32];
    static uint16_t s_light_color[4];
    static int32_t s_light_dir[4][3];
    static int32_t s_light_half[4][3];
    static uint8_t s_product[32][32];
    static bool s_products_made;
    
    /* polygon state */
    static uint32_t s_attr_pending, s_attr;
    static uint32_t s_tex_param, s_tex_pltt;
    static uint32_t s_viewport = 0xBFFF0000u; /* 0,0,255,191 */
    static int s_primitive;
    static bool s_in_begin;
    
    static void make_products(void)
    {
        for (unsigned a = 0; a < 32; ++a) {
            for (unsigned b = 0; b < 32; ++b) {
                s_product[a][b] = (uint8_t)((a * b + 15) / 31);
            }
        }
        s_products_made = true;
    }
    
    static void transform_vector(const int32_t in[3], const mtx *m, int32_t out[3])
    {
        for (int j = 0; j < 3; ++j) {
            int64_t acc = 0;
            for (int i = 0; i < 3; ++i) {
                acc += (int64_t)in[i] * m->m[i][j];
            }
            out[j] = (int32_t)(acc >> 12);
        }
    }
    
    /* Lighting results by raw normal: map geometry repeats a handful of
     * normals. Generation-keyed: anything shade() depends on (vector matrix,
     * lights, material, the polygon's light enables) bumps the generation
     * (the same sites as Platinum's port_gx3d.c normal cache). */
    #define NORMAL_CACHE 64u
    static uint32_t s_normal_gen = 1;
    static struct {
        uint32_t gen, value;
        uint16_t rgb;
    } s_normal_cache[NORMAL_CACHE];
    
    static void normals_changed(void)
    {
        ++s_normal_gen;
    }
    
    /* Lighting reads only the vector matrix's 3x3 part. NNS restores a matrix
     * per node, and map props are translated, not rotated: comparing content
     * keeps the normal cache warm across objects (O3DS: NORMAL cost 2.2 ms per
     * game frame with the cache flushed at every restore, o3ds-log-0). */
    static int32_t s_shade_vec[3][3];
    
    static void vec_changed(void)
    {
        bool same = true;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (s_shade_vec[i][j] != s_vec.m[i][j]) {
                    s_shade_vec[i][j] = s_vec.m[i][j];
                    same = false;
                }
            }
        }
        if (!same) {
            ++s_normal_gen;
        }
    }
    
    static uint16_t shade_uncached(uint32_t value);
    
    static uint16_t shade(uint32_t value)
    {
        const unsigned slot = (unsigned)((value * 2654435761u) >> 26) & (NORMAL_CACHE - 1u);
        if (s_normal_cache[slot].gen == s_normal_gen && s_normal_cache[slot].value == value) {
            return s_normal_cache[slot].rgb;
        }
        const uint16_t rgb = shade_uncached(value);
        s_normal_cache[slot].gen = s_normal_gen;
        s_normal_cache[slot].value = value;
        s_normal_cache[slot].rgb = rgb;
        return rgb;
    }
    
    static uint16_t shade_uncached(uint32_t value)
    {
        int32_t normal[3], n[3];
        unsigned r = s_emission & 31u, g = (s_emission >> 5) & 31u, b = (s_emission >> 10) & 31u;
    
        for (int j = 0; j < 3; ++j) {
            normal[j] = sign_bits(value >> (j * 10), 10) * 8;
        }
        transform_vector(normal, &s_vec, n);
        for (int i = 0; i < 4; ++i) {
            if (!(s_attr & (1u << i))) {
                continue;
            }
            int64_t dot = 0, half_dot = 0;
            for (int j = 0; j < 3; ++j) {
                dot += (int64_t)n[j] * s_light_dir[i][j];
                half_dot += (int64_t)n[j] * s_light_half[i][j];
            }
            int d = (int)(-dot >> 12);
            const unsigned diffuse = d < 0 ? 0u : d > 4096 ? 4096u : (unsigned)d;
            int h = (int)(-half_dot >> 12);
            unsigned spec = h < 0 ? 0u : h > 4096 ? 4096u : (unsigned)h;
            spec = (spec * spec) >> 12;
            if (s_use_shininess) {
                unsigned index = (spec * 128u) >> 12;
                if (index > 127u) {
                    index = 127u;
                }
                const unsigned entry = (s_shininess[index >> 2] >> ((index & 3u) * 8u)) & 255u;
                spec = entry * 4096u / 255u;
            }
            const uint16_t lc = s_light_color[i];
            for (int c = 0; c < 3; ++c) {
                const unsigned l = (lc >> (5 * c)) & 31u;
                const unsigned amb = s_product[(s_ambient >> (5 * c)) & 31u][l];
                const unsigned dif = s_product[(s_diffuse >> (5 * c)) & 31u][l];
                const unsigned spc = s_product[(s_specular >> (5 * c)) & 31u][l];
                const unsigned add = amb + ((dif * diffuse + 2048u) >> 12) + ((spc * spec + 2048u) >> 12);
                if (c == 0) {
                    r += add;
                } else if (c == 1) {
                    g += add;
                } else {
                    b += add;
                }
            }
        }
        return (uint16_t)((r > 31u ? 31u : r) | ((g > 31u ? 31u : g) << 5) | ((b > 31u ? 31u : b) << 10));
    }
    
    static int32_t saturate16(int64_t v)
    {
        return v < -32768 ? -32768 : v > 32767 ? 32767 : (int32_t)v;
    }
    
    /* Texture-coordinate generation (TEXIMAGE_PARAM bits 30-31). Mode 1 at
     * TEXCOORD, mode 2 at NORMAL, mode 3 at each vertex (GBATEK). */
    static void texgen(const int32_t *source, int mode)
    {
        for (int j = 0; j < 2; ++j) {
            int64_t v;
            if (mode == 1) {
                v = (int64_t)source[0] * s_tex.m[0][j] + (int64_t)source[1] * s_tex.m[1][j] +
                    (int64_t)s_tex.m[2][j] + s_tex.m[3][j];
                s_texcoord[j] = saturate16(v >> 12);
            } else {
                v = (int64_t)source[0] * s_tex.m[0][j] + (int64_t)source[1] * s_tex.m[1][j] +
                    (int64_t)source[2] * s_tex.m[2][j];
                s_texcoord[j] = saturate16((v >> 20) + s_texcoord_raw[j]);
            }
        }
    }
    
    /* ------------------------------------------------------- frame storage */
    
    typedef struct frame_buf {
        kj_g3_frame frame;
        kj_g3_vertex *verts;
        uint16_t *indices;
        kj_g3_matrix matrices[KJ_G3_MAX_MATRICES]; /* per-batch palette slices */
        kj_g3_matrix pool[KJ_G3_MAX_MATRICES];     /* every clip matrix this frame (CPU only) */
        uint32_t pool_count;
        kj_g3_batch batches[KJ_G3_MAX_BATCHES];
        uint16_t vert_gmtx[KJ_G3_MAX_VERTS]; /* global matrix id per vertex (pool < 65536) */
    } frame_buf;
    
    static frame_buf s_buf[2];
    static int s_build;          /* index of the frame being built */
    static bool s_have_display;  /* a swap has happened */
                    if (!bypass1) {
                        mixL[i] += ch1L[i];
                        mixR[i] += ch1R[i];
                    }
                    if (!bypass3) {
                        mixL[i] += ch3L[i];
                        mixR[i] += ch3R[i];
                    }
                    if (capturing) {
                        do_capture(chip, 0, (reg8(0x508u) & 2u) ? value0[i] : mixL[i]);
                        do_capture(chip, 1, (reg8(0x509u) & 2u) ? value2[i] : mixR[i]);
                    }
                    if (control & 128u) {
                        int32_t left = leftFrom == 0 ? mixL[i] : leftFrom == 1 ? ch1L[i]
                                     : leftFrom == 2 ? ch3L[i] : ch1L[i] + ch3L[i];
                        int32_t right = rightFrom == 0 ? mixR[i] : rightFrom == 1 ? ch1R[i]
                                      : rightFrom == 2 ? ch3R[i] : ch1R[i] + ch3R[i];
    
                        outputL = (left * masterVolume) / 127;
                        outputR = (right * masterVolume) / 127;
    #ifdef NITRO_SOUND_STATS
                        preL = outputL;
                        preR = outputR;
    #endif
                        /* Headroom before the 16-bit saturation, not after it:
                         * attenuating a flat-topped sample just halves the flat
                         * top, leaving the clipping distortion in the waveform.
                         * See outputShift in nitro_port_sound_chip.h. */
                        if (chip->outputShift) {
                            unsigned shift = chip->outputShift > 4u ? 4u : chip->outputShift;
                            outputL >>= shift;
                            outputR >>= shift;
                        }
                    }
                    stereo[2u * (at + i)] = saturate(outputL);
                    stereo[2u * (at + i) + 1u] = saturate(outputR);
    #ifdef NITRO_SOUND_STATS
                    if (sFrameFn) {
                        unsigned ch;
                        sFrame.index = sFrameIndex++;
                        sFrame.preL = preL;
                        sFrame.preR = preR;
                        sFrame.postL = outputL;
                        sFrame.postR = outputR;
                        for (ch = 0; ch < NITRO_SOUND_CHANNEL_COUNT; ++ch)
                            sFrame.tap[ch] = NITRO_SOUND_TAP_OFF;
                        for (k = 0; k < count; ++k) {
                            const Channel *c = &channels[k];
                            sFrame.tap[c->channel] = c->tap[i];
                            sFrame.format[c->channel] = (uint8_t)c->format;
                            sFrame.volume[c->channel] = (uint8_t)c->volume;
                            sFrame.shift[c->channel] = (uint8_t)c->shift;
                        }
                        sFrameFn(sFrameArg, &sFrame);
                        memset(sFrame.event, 0, sizeof(sFrame.event));
                    }
    #endif
                }
            }
        }
        /* Channels that ended: as reading their (now off) registers left them. */
        for (k = 0; k < count; ++k) {
            if (!channels[k].v->running) {
                channels[k].v->previousCtrl = reg32(channels[k].base);
            }
        }
    }
    
    /* Cumulative profile counters for port_snd.c's [sndprof] line (declared
     * there, not in the shared header, so no header changes). formatCh takes
     * 4 entries, one per sample format (see sProfFormatCh). */
    void NitroSound_MixProfile(uint64_t *calls, uint64_t *frames,
                               uint64_t *activeCh, uint64_t formatCh[4],
                               uint64_t *capturing, uint64_t *dependentCh)
    {
        unsigned i;
        if (calls) *calls = sProfMixCalls;
        if (frames) *frames = sProfMixFrames;
        if (activeCh) *activeCh = sProfActiveCh;
        if (formatCh) {
            for (i = 0; i < 4u; ++i) formatCh[i] = sProfFormatCh[i];
        }
        if (capturing) *capturing = sProfCapturing;
        if (dependentCh) *dependentCh = sProfDependentCh;
    }
    
    #ifdef NITRO_SOUND_STATS
    void NitroSound_SetFrameHook(NitroSoundFrameFn fn, void *arg)
    {
        sFrameFn = fn;
        sFrameArg = arg;
        sFrameIndex = 0;
        memset(&sFrame, 0, sizeof(sFrame));
    }
    #endif
    build\upstream.pre-clean\tools\asmdiff\ntruncompbw.c
    build\upstream.pre-clean\tools\asmdiff\ntrextractfile.c
    build\upstream.pre-clean\tools\asmdiff\Makefile
    build\upstream.pre-clean\tools\asmdiff\find_module_params.py
    build\upstream\tools\asmdiff\ntruncompbw.c
    build\upstream\tools\asmdiff\ntrextractfile.c
    build\upstream\tools\asmdiff\Makefile
    build\upstream\tools\asmdiff\find_module_params.py
    #!/usr/bin/env python3
    """Statistical PC sampler for a program running in Azahar (GDB stub).
    
    Azahar is not a timing authority, but it charges one tick per guest
    instruction (measured: [calib] in the log), so a PC histogram from it is an
    instruction-count profile: exactly what is needed to find where the ARM11
    work goes. Real O3DS cost is higher wherever code misses caches.
    
        python tools/azahar/gdbprof.py build/kj/pokeheartgold-3ds.elf \
            --samples 2000 --interval 0.01 --delay 140
    
    Speaks the GDB remote serial protocol directly (devkitARM's gdb has no
    Python): interrupt (0x03), read the PC (register 15), continue.
    """
    from __future__ import annotations
    
    import argparse
    import bisect
    import collections
    import socket
    import subprocess
    import sys
    import time
    
    
    class Rsp:
        def __init__(self, host: str, port: int):
            self.s = socket.create_connection((host, port), timeout=10)
            self.buf = b""
    
        def _read_packet(self) -> str:
            while True:
                while b"$" not in self.buf:
                    self.buf += self.s.recv(4096)
                start = self.buf.index(b"$")
                while b"#" not in self.buf[start:] or len(self.buf) < self.buf.index(b"#", start) + 3:
                    self.buf += self.s.recv(4096)
                end = self.buf.index(b"#", start)
                data = self.buf[start + 1:end]
                self.buf = self.buf[end + 3:]
                self.s.sendall(b"+")
                return data.decode("latin-1")
    
        def send(self, data: str) -> None:
            csum = sum(data.encode("latin-1")) & 0xFF
            self.s.sendall(f"${data}#{csum:02x}".encode("latin-1"))
            # wait for ack
            while True:
                if not self.buf:
                    self.buf += self.s.recv(4096)
                c, self.buf = self.buf[:1], self.buf[1:]
                if c == b"+":
                    return
                if c == b"-":
                    self.s.sendall(f"${data}#{csum:02x}".encode("latin-1"))
    
        def request(self, data: str) -> str:
            self.send(data)
            return self._read_packet()
    
        def interrupt(self) -> str:
            self.s.sendall(b"\x03")
            return self._read_packet()
    
    
    def load_symbols(elf: str, nm: str):
        out = subprocess.run([nm, "-n", "-C", elf], capture_output=True, text=True).stdout
        addrs, names = [], []
        for line in out.splitlines():
            parts = line.split(None, 2)
            if len(parts) == 3 and parts[1].lower() in ("t", "w"):
                addrs.append(int(parts[0], 16))
                names.append(parts[2])
        return addrs, names
    
    
    def sample(a, r, hist, callers, watch, sym, pcs):
        for _ in range(a.samples):
            time.sleep(a.interval)
            r.interrupt()
            pc_hex = r.request("p f")
            try:
                pc = int.from_bytes(bytes.fromhex(pc_hex[:8]), "little")
            except ValueError:
                continue
            name = sym(pc)
            hist[name] += 1
            if name in a.pcs_set:
                pcs[pc] += 1
            if name in a.stack_map:
                # a return address saved on the stack (callers further up)
                sp_hex = r.request("p d")
                try:
                    sp = int.from_bytes(bytes.fromhex(sp_hex[:8]), "little")
                    mem = r.request(f"m {sp + a.stack_map[name]:x},4")
    ✓ • 474ms

## Activity

    $ Get-Content src/runtime/kj_g3.c -First 40; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 545 -First 165; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1540 -First 80; Get-Content src/compat/nitro/src/port_sound_chip.c | Select-Object -Skip 510 -First 220; rg --files private-gitignore tests | Select-Object -First 45
    /*
     * kj_g3.c: the DS geometry engine, DS side.
     *
     * Interprets the G3 command stream (direct opcode writes and packed GXFIFO
     * words) with the DS's semantics, and produces a kj_g3_frame per
     * SWAP_BUFFERS for the PICA backend.
     *
     * Division of work (kohjoh design, docs/architecture/3d.md):
     *   CPU (here): matrix stacks and products (20.12 fixed, as the DS), the
     *     game-visible results (clip/vector matrix registers, BOX/POS/VEC
     *     tests, GXSTAT levels), per-vertex lighting and texture-coordinate
     *     generation (exact DS fixed point), primitive assembly to indexed
     *     triangles, material batching.
     *   GPU: position transform. Vertices stay in object space (s16 4.12) and
     *     carry an index into a small palette of clip matrices uploaded per
     *     draw, so no per-vertex matrix math or clipping runs on the ARM11.
     *
     * Fixed-point lighting and texgen follow GBATEK as verified by hgss-3ds
     * (3ds_port/nitro/src/port_gx3d.c @ a26c003, GPL-3.0): shade(),
     * transform_texcoords(), set_geo_position().
     */
    
    #include "kj_g3.h"
    
    #include <nitro.h>
    #include <nitro_port.h>
    #include <nitro_port_gx3d.h>
    #include <stdlib.h>
    #include <stddef.h>
    #include <string.h>
    
    /* The display-list cache is on by default: KJ_DL_VERIFY found 0 mismatches
     * in the intro, title, Goldenrod idle, and Goldenrod -> Route 34 -> trainer
     * battle -> field (2026-11-13). --define KJ_DL_NO_CACHE builds the
     * uncached decoder for A/B runs and bisects. */
    #if !defined(KJ_DL_NO_CACHE) && !defined(KJ_DL_CACHE)
    #define KJ_DL_CACHE
    #endif
    
    extern unsigned char NitroPort_IoReg[];
    {
        return s_have_display ? &s_buf[s_build ^ 1].frame : NULL;
    }
    
    static frame_buf *build(void)
    {
        return &s_buf[s_build];
    }
    
    /* The current clip matrix as a global (frame) matrix id, pushing it into
     * the frame's matrix list when it changed. */
    static bool current_gmtx(uint32_t *out)
    {
        frame_buf *fb = build();
    
        if (s_cur_gmtx_version != s_clip_version || fb->pool_count == 0) {
            if (fb->pool_count >= KJ_G3_MAX_MATRICES) {
                return false;
            }
            const mtx *c = clip_matrix();
            kj_g3_matrix *dst = &fb->pool[fb->pool_count];
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    dst->m[i][j] = (float)c->m[i][j] * (1.0f / 4096.0f);
                }
            }
            s_cur_gmtx = fb->pool_count++;
            s_cur_gmtx_version = s_clip_version;
        }
        *out = s_cur_gmtx;
        return true;
    }
    
    static const uint8_t EXPAND5[32] = {
        0, 8, 16, 24, 33, 41, 49, 57, 66, 74, 82, 90, 99, 107, 115, 123,
        132, 140, 148, 156, 165, 173, 181, 189, 198, 206, 214, 222, 231, 239, 247, 255,
    };
    
    static void update_color_rgba(void)
    {
        const unsigned a = (s_attr >> 16) & 31u;
        s_color_rgba = (uint32_t)EXPAND5[s_color & 31u] | ((uint32_t)EXPAND5[(s_color >> 5) & 31u] << 8) |
                       ((uint32_t)EXPAND5[(s_color >> 10) & 31u] << 16) | ((uint32_t)EXPAND5[a] << 24);
    }
    
    static void material_changed(void)
    {
        s_batch_ok = false;
    }
    
    static void close_batch(void)
    {
        s_batch = NULL;
        s_batch_ok = false;
        s_slot_gid = UINT32_MAX;
    }
    
    static bool open_batch(void)
    {
        frame_buf *fb = build();
    
        if (fb->frame.batch_count >= KJ_G3_MAX_BATCHES) {
            ++fb->frame.dropped_batches;
            return false;
        }
        s_batch = &fb->batches[fb->frame.batch_count++];
        memset(s_batch, 0, sizeof(*s_batch));
        s_batch->first_index = fb->frame.index_count;
        s_batch->first_matrix = 0xFFFF;
        s_batch->poly_attr = s_attr;
        s_batch->tex_param = s_tex_param;
        s_batch->tex_pltt = s_tex_pltt;
        s_batch->viewport = s_viewport;
        {
            const unsigned alpha = (s_attr >> 16) & 31u;
            const unsigned fmt = (s_tex_param >> 26) & 7u;
            const unsigned mode = (s_attr >> 4) & 3u;
            if ((alpha > 0 && alpha < 31) || fmt == 1 || fmt == 6) {
                s_batch->flags |= KJ_G3_BATCH_TRANSLUCENT;
            }
            if (mode == 3) {
                s_batch->flags |= KJ_G3_BATCH_SHADOW;
            }
        }
        s_batch_first_vert = fb->frame.vert_count;
        s_batch_key_attr = s_attr;
        s_batch_key_tex = s_tex_param;
        s_batch_key_pltt = s_tex_pltt;
        s_batch_key_vp = s_viewport;
        s_batch_ok = true;
        s_slot_gid = UINT32_MAX;
        return true;
    }
    
    /* Palette slot of global matrix `gid` in the open batch, adding it if
     * there is room; -1 when the palette is full. */
    static int palette_slot(uint32_t gid)
    {
        const uint16_t count = s_batch->matrix_count;
    
        for (int i = count - 1; i >= 0; --i) {
            if (s_palette_gid[i] == gid) {
                return i;
            }
        }
        if (count >= KJ_G3_PALETTE) {
            return -1;
        }
        /* A batch's palette must be a contiguous slice of the frame's matrix
         * list: copy the matrix to the slice's end. */
        frame_buf *fb = build();
        if (s_batch->first_matrix == 0xFFFF) {
            s_batch->first_matrix = (uint16_t)fb->frame.matrix_count;
        }
        if (fb->frame.matrix_count >= KJ_G3_MAX_MATRICES) {
            return -1;
        }
        fb->matrices[fb->frame.matrix_count++] = fb->pool[gid];
        s_palette_gid[count] = gid;
        s_batch->matrix_count = count + 1;
        return count;
    }
    
    static bool batch_matches_state(void)
    {
        if (s_batch_ok) {
            return true;
        }
        s_batch_ok = s_batch != NULL && s_batch_key_attr == s_attr && s_batch_key_tex == s_tex_param &&
                     s_batch_key_pltt == s_tex_pltt && s_batch_key_vp == s_viewport;
        return s_batch_ok;
    }
    
    static inline int cached_slot(uint32_t gid)
    {
        if (gid == s_slot_gid) {
            return s_slot_idx;
        }
        const int slot = palette_slot(gid);
        if (slot >= 0) {
            s_slot_gid = gid;
            s_slot_idx = slot;
        }
        return slot;
    }
    
    /* A vertex of the open batch for frame vertex `v`: vertices emitted before
     * the batch opened are copied into it with their matrix remapped. */
    static bool local_vertex(uint32_t v, uint32_t *out)
    {
        frame_buf *fb = build();
    
        if (v >= s_batch_first_vert) {
            *out = v;
            return true;
        }
        if (fb->frame.vert_count >= KJ_G3_MAX_VERTS) {
            return false;
        }
        const int slot = palette_slot(fb->vert_gmtx[v]);
        if (slot < 0) {
            return false;
        }
        const uint32_t n = fb->frame.vert_count++;
        fb->verts[n] = fb->verts[v];
            emit_triangle(s_prim_verts[0], s_prim_verts[1], n);
        } else {
            emit_triangle(s_prim_verts[0], s_prim_verts[1], s_prim_verts[2]);
            emit_triangle(s_prim_verts[0], s_prim_verts[2], n);
        }
        return true;
    done:
        s_batch->index_count += ic - fb->frame.index_count;
        fb->frame.index_count = ic;
        return true;
    }
    
    /* Decode one command of the bulk path, with vertex/texcoord fast paths.
     * Every vertex opcode only sets coordinates and shares one vertex tail, so
     * fast_vertex is emitted once: the decode loop has to stay inside the
     * ARM11's 16 KiB I-cache (coffeepot cpu-optimization.md, icachebench: CPI
     * 1.1 up to 16 KiB, 5.6 from 32 KiB). With the fast path inlined into
     * each opcode and both decode branches this function was 14.6 KiB. */
    static inline __attribute__((always_inline)) void hot_command(unsigned op, const uint32_t *p)
    {
        uint32_t v = p[0];
        switch (op) {
        case 0x22:
            if ((s_tex_param >> 30) != 0) {
                break;
            }
            s_texcoord[0] = s_texcoord_raw[0] = (int16_t)v;
            s_texcoord[1] = s_texcoord_raw[1] = (int16_t)(v >> 16);
            return;
        case 0x23:
            s_coord[0] = (int16_t)v;
            s_coord[1] = (int16_t)(v >> 16);
            s_coord[2] = (int16_t)p[1];
            goto vertex;
        case 0x24: /* VTX_10 */
            s_coord[0] = (int16_t)(sign_bits(v, 10) * 64);
            s_coord[1] = (int16_t)(sign_bits(v >> 10, 10) * 64);
            s_coord[2] = (int16_t)(sign_bits(v >> 20, 10) * 64);
            goto vertex;
        case 0x25:
            s_coord[0] = (int16_t)v;
            s_coord[1] = (int16_t)(v >> 16);
            goto vertex;
        case 0x26:
            s_coord[0] = (int16_t)v;
            s_coord[2] = (int16_t)(v >> 16);
            goto vertex;
        case 0x27:
            s_coord[1] = (int16_t)v;
            s_coord[2] = (int16_t)(v >> 16);
            goto vertex;
        case 0x28: /* VTX_DIFF */
            s_coord[0] = (int16_t)(s_coord[0] + sign_bits(v, 10));
            s_coord[1] = (int16_t)(s_coord[1] + sign_bits(v >> 10, 10));
            s_coord[2] = (int16_t)(s_coord[2] + sign_bits(v >> 20, 10));
            goto vertex;
        case 0x21: /* NORMAL without normal texgen */
            if ((s_tex_param >> 30) == 2) {
                break;
            }
            if (s_attr & 0xF) {
                s_color = shade(v);
                update_color_rgba();
            }
            return;
        default:
            break;
        }
        execute(op, p);
        return;
    vertex:
        if (!fast_vertex()) {
            submit_vertex();
        }
    }
    
    static void build_param_table(void)
    {
        for (unsigned op = 0; op < 256; ++op) {
            s_param_count[op] = (uint8_t)param_count(op);
                    v->phase = ((uint64_t)advances << 32) | frac;
                    v->lastSample = last;
                    advance_voice(c);
                    if (!v->running) {
                        return i + 1u;
                    }
                    index = v->sampleIndex;
                    predictor = v->predictor;
                    stepIndex = v->stepIndex;
                    last = v->lastSample;
                    loopMark = v->adpcmClean ? loopAt : 0u;
                    continue;
                }
                {
                    const uint32_t nibble = index - 1u;
                    const uint8_t byte = source[4u + nibble / 2u];
    #ifdef HGSS_SND_ADPCM
                    const unsigned code = (byte >> ((nibble & 1u) << 2)) & 15u;
                    const int32_t st = sStep[stepIndex];
                    const int32_t change = (st >> 3) + (st >> 2) * (int32_t)(code & 1u)
                                         + (st >> 1) * (int32_t)((code >> 1) & 1u)
                                         + st * (int32_t)((code >> 2) & 1u);
    #else
                    const unsigned code = (nibble & 1u) ? byte >> 4 : byte & 15u;
                    const int32_t st = sStep[stepIndex];
                    int32_t change = st >> 3;
    
                    if (code & 1u) {
                        change += st >> 2;
                    }
                    if (code & 2u) {
                        change += st >> 1;
                    }
                    if (code & 4u) {
                        change += st;
                    }
    #endif
                    last = predictor;
                    predictor += (code & 8u) ? -change : change;
                    predictor = sat16(predictor);
                    stepIndex += sIndexDelta[code & 7u];
                    if (stepIndex < 0) {
                        stepIndex = 0;
                    }
                    if (stepIndex > 88) {
                        stepIndex = 88;
                    }
                    if (index == loopMark) {
                        v->loopSaved = 1;
                        v->loopSource = c->sourceAddr;
                        v->loopAt = loopAt;
                        v->loopPredictor = predictor;
                        v->loopStepIndex = stepIndex;
                    }
                }
            }
        }
        v->sampleIndex = index;
        v->predictor = predictor;
        v->stepIndex = stepIndex;
        v->phase = frac;
        v->lastSample = last;
        return frames;
    }
    
    static size_t adpcm_render(Channel *c, int32_t *out, size_t frames)
    {
        if (c->interpolate) {
            if ((c->step >> 32) == 0u) {
                return adpcm_render_frames(c, out, frames, 1, 1);
            }
            return adpcm_render_frames(c, out, frames, 1, 0);
        }
        return adpcm_render_frames(c, out, frames, 0, 0);
    }
    
    /*
     * Adds rendered samples to L and R with the channel's volume, shift and
     * pan; values (if not NULL) gets each frame's sample after volume and
     * shift. The same arithmetic as voice_mix, so the same result: a hard pan
     * adds (x * 127) / 127 = x to one side and (x * 0) / 127 = 0 to the other.
     */
    static void mix_scaled(Channel *c, const int32_t *samples, int32_t *L, int32_t *R,
                           int32_t *values, size_t frames)
    {
        const int32_t volume = c->volume;
        const int32_t panL = 127 - c->pan;
        const int32_t panR = c->pan;
        const unsigned shift = c->shift;
        size_t i;
    
        if (volume == 0) {
            /* Silent: (x * 0) / 127 >> shift is 0 on both sides. */
            for (i = 0; i < frames; ++i) {
                if (values) {
                    values[i] = 0;
                }
                STATS_TAP(c, i, 0);
            }
            return;
        }
        if (panR == 0) {
            for (i = 0; i < frames; ++i) {
                const int32_t scaled = ((samples[i] * volume) / 127) >> shift;
    
                if (values) {
                    values[i] = scaled;
                }
                STATS_TAP(c, i, scaled);
                L[i] += scaled;
            }
        } else if (panL == 0) {
            for (i = 0; i < frames; ++i) {
                const int32_t scaled = ((samples[i] * volume) / 127) >> shift;
    
                if (values) {
                    values[i] = scaled;
                }
                STATS_TAP(c, i, scaled);
                R[i] += scaled;
            }
        } else {
            for (i = 0; i < frames; ++i) {
                const int32_t scaled = ((samples[i] * volume) / 127) >> shift;
    
                if (values) {
                    values[i] = scaled;
                }
                STATS_TAP(c, i, scaled);
                L[i] += (scaled * panL) / 127;
                R[i] += (scaled * panR) / 127;
            }
        }
    }
    
    /*
     * A channel's samples for a run of frames, added to L and R: what
     * voice_mix gives frame by frame, with the channel's settings and its
     * decoder in local variables; values (if not NULL) gets each frame's
     * sample. For channels that don't read what a capture writes: they don't
     * depend on the other channels within a mix.
     */
    static void mix_block(Channel *c, int32_t *L, int32_t *R, int32_t *values, size_t frames)
    {
        NitroSoundVoice *v = c->v;
        const int32_t volume = c->volume, panL = 127 - c->pan, panR = c->pan;
        const unsigned shift = c->shift;
        const uint64_t step = c->step;
        /* step is a block constant. At the port's 32,768 Hz output rate no
         * timer value makes it exactly PHASE_ONE (SOUND_CLOCK is not a
         * multiple of 32,768), so the phase loop always has work to do; a
         * single-sample-per-frame fast path would never be entered. */
        uint64_t phase = v->phase;
        size_t i;
    
        if (c->format == 2u && c->source && c->nSamples && frames <= MIX_BLOCK &&
            (v->phase >> 32) == 0u && (step >> 32) < 2048u) {
            /* ADPCM: rendered, then mixed (see adpcm_render). */
            int32_t samples[MIX_BLOCK];
            const size_t played = adpcm_render(c, samples, frames);
    
            mix_scaled(c, samples, L, R, values, played);
            return;
        }
        if (c->format <= 1u && c->source && c->nSamples) {
            /* PCM8 / PCM16. One loop per pan case, hoisted out of the frame
             * loop; step and shift are already block constants. */
            const uint8_t *source = c->source;
            const uint32_t nSamples = c->nSamples;
            const int pcm16bit = c->format == 1u;
            const int interpolate = c->interpolate;
            const int fullVolume = volume == 127;
            uint32_t index = v->sampleIndex;
            int32_t last = v->lastSample;
    
            if (volume == 0) {
                /* Silent: it adds nothing to either side, but its cursor must
                 * still move, or the sample it comes back on when the envelope
                 * lifts would be the wrong one. Only an interpolating channel
                 * reads the source here, to keep lastSample. */
                for (i = 0; i < frames; ++i) {
                    if (values) values[i] = 0;
                    STATS_TAP(c, i, 0);
                    phase += step;
                    if (!pcm_advance(c, &phase, &index, &last, pcm16bit,
                                     interpolate, nSamples))
                        return;
                }
            } else if (panL == 127) {
                /* Hard left: panR is 0, so the right side would add
                 * ((x * 0) / 127) = 0. */
                for (i = 0; i < frames; ++i) {
                    int32_t sample = pcm16bit ? pcm16(source, index * 2u)
                                              : (int32_t)pcm8(source, index) * 256;
                    int32_t scaled;
    
                    if (interpolate) sample = lerp(last, sample, phase);
                    /* (sample * 127) / 127 is sample exactly, and a PCM sample
                     * times 127 cannot overflow int32. */
                    if (fullVolume) {
                        scaled = sample >> shift;
                    } else {
                        scaled = (sample * volume) / 127;
                        scaled >>= shift;
                    }
                    if (values) values[i] = scaled;
                    STATS_TAP(c, i, scaled);
                    L[i] += scaled;
                    phase += step;
                    if (!pcm_advance(c, &phase, &index, &last, pcm16bit,
                                     interpolate, nSamples))
                        return;
                }
            } else if (panL == 0) {
                /* Hard right: panL is 0, so the left side would add 0. */
                for (i = 0; i < frames; ++i) {
                    int32_t sample = pcm16bit ? pcm16(source, index * 2u)
                                              : (int32_t)pcm8(source, index) * 256;
                    int32_t scaled;
    
    private-gitignore\o3ds-log-3\heartgold.log
    private-gitignore\o3ds-log-3\dumps\f05000.kjd
    private-gitignore\o3ds-log-3\dumps\f01650.kjd
    private-gitignore\o3ds-log-3\dumps\f01350.kjd
    private-gitignore\o3ds-log-3\dumps\f00900.kjd
    private-gitignore\o3ds-log-3\dumps\f00450.kjd
    private-gitignore\o3ds-log-3\dumps\f00150.kjd
    private-gitignore\o3ds-log-2\logs\heartgold.log
    private-gitignore\o3ds-log-2\logs\heartgold-dbg.log
    tests\inputs\title-step.txt
    tests\inputs\title-live.txt
    tests\inputs\route34-south.txt
    tests\inputs\route34-battle.txt
    tests\inputs\intro-step.txt
    tests\inputs\intro-perf.txt
    tests\inputs\hg-continue-field.txt
    tests\inputs\goldenrod-step.txt
    tests\inputs\goldenrod-input.txt
    tests\inputs\goldenrod-idle.txt
    private-gitignore\o3ds-log-2\dumps-dbg\heartgold-dbg.log
    private-gitignore\o3ds-log-2\dumps-dbg\f05000.kjd
    private-gitignore\o3ds-log-2\dumps-dbg\f01650.kjd
    private-gitignore\o3ds-log-2\dumps-dbg\f01350.kjd
    private-gitignore\o3ds-log-2\dumps-dbg\f00900.kjd
    private-gitignore\o3ds-log-2\dumps-dbg\f00450.kjd
    private-gitignore\o3ds-log-2\dumps-dbg\f00150.kjd
    private-gitignore\o3ds-log-0\logs\heartgold.log
    private-gitignore\o3ds-log-0\dumps\f05000.kjd
    private-gitignore\o3ds-log-0\dumps\f01650.kjd
    private-gitignore\o3ds-log-2\dumps\f05000.kjd
    private-gitignore\o3ds-log-0\dumps\f01350.kjd
    private-gitignore\o3ds-log-0\dumps\f00900.kjd
    private-gitignore\o3ds-log-0\dumps\f00450.kjd
    private-gitignore\o3ds-log-0\dumps\f00150.kjd
    private-gitignore\o3ds-log-2\dumps\f01650.kjd
    private-gitignore\o3ds-log-2\dumps\f00450.kjd
    private-gitignore\o3ds-log-2\dumps\f00150.kjd
    private-gitignore\o3ds-log-2\dumps\f01350.kjd
    private-gitignore\o3ds-log-2\dumps\f00900.kjd
    private-gitignore\n3ds-log-2\logs\heartgold.log
    private-gitignore\n3ds-log-2\logs\heartgold-dbg.log
    private-gitignore\n3ds-log-2\dumps\f05000.kjd
    private-gitignore\n3ds-log-2\dumps-dbg\heartgold-dbg.log
    private-gitignore\n3ds-log-2\dumps\f02500.kjd
    private-gitignore\n3ds-log-2\dumps\f02300.kjd
    ✓ • 401ms

## Activity

    $ Get-Content src/compat/nitro/tests/test_sound_chip.c -First 45; rg -n 'sStep|change =|delta =' src/compat/nitro/src/port_sound_chip.c; Get-Content src/compat/nitro/tests/sound_chip_reference.c -First 28; Get-Content src/compat/nitro/Makefile | Select-Object -Skip 290 -First 20; Get-Content src/compat/common.mk -First 60; Get-Content src/compat/nitro/src/port_sound_chip.c | Select-Object -Skip 20 -First 60
    Get-Content:
    Line |
       2 |  Get-Content src/compat/nitro/tests/test_sound_chip.c -First 45; rg -n …
         |  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         | Cannot find path 'C:\Users\terezi\workdir\kohjoh\src\compat\nitro\tests\test_sound_chip.c' because it does not exist.
    100:static const uint16_t sStep[89] = {
    120:    int32_t step = sStep[v->stepIndex];
    126:    int32_t change = (step >> 3) + (step >> 2) * (int32_t)(nibble & 1u)
    130:    int32_t change = step >> 3;
    529:                const int32_t st = sStep[stepIndex];
    530:                const int32_t change = (st >> 3) + (st >> 2) * (int32_t)(code & 1u)
    535:                const int32_t st = sStep[stepIndex];
    536:                int32_t change = st >> 3;
    Get-Content:
    Line |
       2 |  … und_chip.c; Get-Content src/compat/nitro/tests/sound_chip_reference.c …
         |                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         | Cannot find path 'C:\Users\terezi\workdir\kohjoh\src\compat\nitro\tests\sound_chip_reference.c' because it does not exist.
    
    $(LIB): $(SDK_OBJS) $(PORT_OBJS) $(ARM7_OBJS)
        @rm -f $@
        @$(AR) rcs $@ $^
        @echo "built $@"
    
    # Pure-C chip tests link no NitroSDK or platform code.
    SOUND_CHIP_TEST_BIN := $(OUT)/test_sound_chip
    $(SOUND_CHIP_TEST_BIN): tests/test_sound_chip.c tests/sound_chip_reference.c src/port_sound_chip.c include/nitro_port_sound_chip.h
        @mkdir -p $(dir $@)
        $(CC) $(PORT_CFLAGS) $(LDFLAGS) -Iinclude tests/test_sound_chip.c tests/sound_chip_reference.c src/port_sound_chip.c -o $@
    
    # Original ARM9 API to the renamed ARM7 driver through the virtual PXI
    # protocol, with a synthetic SSEQ/SBNK/SWAR fixture (no game content).
    SOUND_SEQ_TEST_BIN := $(OUT)/test_sound_sequence
    $(SOUND_SEQ_TEST_BIN): tests/test_sound_sequence.c $(LIB)
        $(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB) -lm -o $@
    
    # Measurement tool, not a test: plays one sequence of a retail SDAT through the
    # port's audio stack (tests/sdat_render.c) and writes a WAV and mixer diagnostics.
    # ---------------------------------------------------------------------------
    # Settings shared by everything the port compiles from Nintendo's and the
    # decompilation's C: libnitro (NitroSDK), libnitrosystem (NitroSystem) and,
    # later, the game. Included by those Makefiles; not run on its own.
    #
    # Defines, for the including Makefile:
    #   TARGET       host (PC, for tests), host32 (32-bit PC, for tests that need
    #                the 3DS's 4-byte pointers) or 3ds (default: host)
    #   IS_HOST      yes for host and host32
    #   ROOT         repository root
    #   SDK, NNS     build/NitroSDK, build/NitroSystem (fetched by tools/bootstrap.py)
    #   CC, AR, NM, OBJCOPY  the toolchain for TARGET
    #   PORT_CFLAGS  flags every file is compiled with
    #   SDK_INCLUDES include paths for NitroSDK (and the libnitro port headers)
    #   NNS_INCLUDES include paths for NitroSystem
    #   LDFLAGS      link flags for test programs (host only)
    #   NNS_INLINE_SYMS  NitroSystem's header inline functions (built by
    #                3ds_port/nitrosystem); every object that includes NitroSystem
    #                headers must be passed through $(WEAKEN_NNS_INLINE)
    # ---------------------------------------------------------------------------
    
    TARGET ?= host
    ROOT   := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../..)
    SDK    := $(ROOT)/build/NitroSDK
    NNS    := $(ROOT)/build/NitroSystem
    NITRO  := $(ROOT)/src/compat/nitro
    
    ifeq ($(wildcard $(SDK)/include/nitro.h),)
    $(error NitroSDK not found at $(SDK). Run: python tools/bootstrap.py)
    endif
    
    # The same SDK configuration pokeplatinum builds with, minus the Metrowerks
    # compiler (SDK_CW), plus SDK_PORT_3DS which turns on our patches.
    PORT_DEFINES := -DSDK_ARM9 -DSDK_TS -DSDK_4M -DSDK_CODE_ARM -DSDK_FINALROM \
                    -DNNS_FINALROM -DSDK_PORT_3DS
    
    # Matching how Metrowerks compiled the originals:
    #   gnu89 inline rules match how the SDK's .c files use `inline`.
    #   No strict aliasing: the SDK reinterprets memory through casts on purpose.
    #   Enums are always 4 bytes ("-enum int"); ARM GCC would otherwise shrink
    #   them and break structure layouts.
    #   char is signed ("-char signed"); on ARM, GCC's char is unsigned by
    #   default, which silently changes comparisons like `c < 0`.
    #   Overflow, out-of-range indexes and null pointers do what the DS's code
    #   did: signed arithmetic wraps, and GCC doesn't drop loop iterations or
    #   null checks on the grounds that the code can't rely on them. Metrowerks
    #   made no such assumptions, and the game has places that would be
    #   affected (GCC warns about a few loops in battle_lib.c).
    PORT_CFLAGS := -std=gnu99 -fgnu89-inline -fno-strict-aliasing -fno-short-enums \
                   -fsigned-char -fwrapv -fno-aggressive-loop-optimizations \
                   -fno-delete-null-pointer-checks -O2 -g \
                   -Wall -Wno-unused-function -Wno-unused-variable -Wno-missing-braces \
                   -Wno-unknown-pragmas -Wno-unused-but-set-variable -Wno-parentheses \
                   -Wno-switch -Wno-multichar \
                   $(PORT_DEFINES)
    
    # Not wl_arm7's headers: that's the ARM7's wireless library, which no ARM9
    # code uses, and its System.h and Font.h would stand in for the game's
    # system.h and font.h on Windows, where file names ignore case.
    SDK_INCLUDES := -I$(NITRO)/include -I$(NITRO)/build/$(TARGET)/gen -I$(SDK)/include \
    #define REG_LOOP    10u
    #define REG_LENGTH  12u
    
    #ifdef NITRO_SOUND_STATS
    /* Test-only frame hook (sdat_render): the mixer runs one frame per block, so
     * each event lands on its frame. Blocks give what frame-by-frame mixing gives
     * (see mix_block); sdat_render's release build checks that on real output. */
    #define MIX_BLOCK 1
    static NitroSoundFrameFn sFrameFn;
    static void *sFrameArg;
    static NitroSoundFrame sFrame;
    static uint32_t sFrameIndex;
    #define STATS_EVENT(channel, bit) do { if (sFrameFn) sFrame.event[(channel)] |= (uint8_t)(bit); } while (0)
    #define STATS_CHANNEL_OF(base) (((base) - 0x400u) / 16u)
    #define STATS_TAP(c, i, v) do { if ((c)->tap) (c)->tap[(i)] = (v); } while (0)
    #else
    #define MIX_BLOCK 128
    #define STATS_EVENT(channel, bit) ((void)0)
    #define STATS_CHANNEL_OF(base) 0u
    #define STATS_TAP(c, i, v) ((void)0)
    #endif
    
    volatile uint8_t NitroPort_SoundRegisters[NITRO_SOUND_REG_SIZE];
    
    static unsigned char reg8(unsigned offset)
    {
        return NitroPort_SoundRegisters[offset];
    }
    
    static uint16_t reg16(unsigned offset)
    {
        return (uint16_t)(reg8(offset) | ((uint16_t)reg8(offset + 1) << 8));
    }
    
    static uint32_t reg32(unsigned offset)
    {
        return (uint32_t)reg16(offset) | ((uint32_t)reg16(offset + 2) << 16);
    }
    
    static void put8(unsigned offset, uint8_t value)
    {
        NitroPort_SoundRegisters[offset] = value;
    }
    
    static void put32(unsigned offset, uint32_t value)
    {
        put8(offset, (uint8_t)value);
        put8(offset + 1, (uint8_t)(value >> 8));
        put8(offset + 2, (uint8_t)(value >> 16));
        put8(offset + 3, (uint8_t)(value >> 24));
    }
    
    static int16_t saturate(int32_t value)
    {
    #ifdef HGSS_SND_ADPCM
        /* Ternary form so ARM builds emit predicated movs (verified: SSAT,
         * no branches, in the device object); bit-identical to the branched
         * form (host differential proof). Rides the ADPCM diet flag so the
         * hardware A/B measures one change. */
        int32_t v = value < INT16_MIN ? INT16_MIN : value;
    ✓ • 582ms

## Activity

    $ New-Item -ItemType Directory -Force build/diagnosis | Out-Null; Copy-Item src/compat/nitro/src/port_sound_chip.c build/diagnosis/sound_before.c; Copy-Item src/runtime/kj_g3.c build/diagnosis/g3_before.c; Get-Content src/compat/nitro/src/port_sound_chip.c | Select-Object -Skip 98 -First 18; Get-Content src/compat/nitro/include/nitro_port_sound_chip.h -First 130; Get-Content tools/azahar/run.ps1 -First 65
    
    static const uint16_t sStep[89] = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
        130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371,
        408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166,
        1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
        3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845,
        8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
        18500, 20350, 22385, 24623, 27086, 29794, 32767
    };
    static const int8_t sIndexDelta[8] = {-1,-1,-1,-1,2,4,6,8};
    
    static void adpcm_init(NitroSoundVoice *v, const uint8_t *source)
    {
        v->predictor = pcm16(source, 0);
        v->stepIndex = source[2] <= 88 ? source[2] : 88;
    }
    /* Independent DS sound register engine (plain C99, no libctru/NitroSDK).
     * ARM7 SDK sound-register writes target NitroPort_SoundRegisters; the mixer
     * consumes their state at 32,768 samples/s, producing interleaved signed PCM.
     */
    #ifndef NITRO_PORT_SOUND_CHIP_H
    #define NITRO_PORT_SOUND_CHIP_H
    #include <stddef.h>
    #include <stdint.h>
    #define NITRO_SOUND_RATE 32768u
    #define NITRO_SOUND_REG_SIZE 0x600u
    #define NITRO_SOUND_CHANNEL_COUNT 16u
    extern volatile uint8_t NitroPort_SoundRegisters[NITRO_SOUND_REG_SIZE];
    
    typedef struct NitroSoundVoice {
        uint32_t previousCtrl;
        uint32_t sampleIndex;
        uint64_t phase;
        int32_t predictor;
        int32_t lastSample;
        int32_t stepIndex;
        uint16_t noise;
        uint8_t adpcmByte;
        uint8_t adpcmNibble;
        uint8_t running;
        /* Kept by the mixer so it doesn't redo work every sample: the step for
         * the last timer value, and the ADPCM decoder's state at the loop
         * start (as the DS keeps it), for the loop start and data it was
         * saved for. */
        uint8_t loopSaved;
        uint8_t adpcmClean;    /* the ADPCM state is adpcmSource's, decoded from its start */
        uint32_t adpcmSource;
        uint32_t stepPeriod;
        uint32_t stepRate;
        uint64_t step;
        uint32_t loopSource;
        uint32_t loopAt;
        int32_t loopPredictor;
        int32_t loopStepIndex;
    } NitroSoundVoice;
    
    typedef struct NitroSoundCapture {
        uint32_t index;
        uint8_t running;
        /* A capture takes samples at the rate of channel 1's (capture 0) or
         * channel 3's (capture 1) timer, as the DS's does. */
        uint64_t phase;
        uint32_t stepPeriod;
        uint32_t stepRate;
        uint64_t step;
    } NitroSoundCapture;
    
    typedef struct NitroSoundChip {
        NitroSoundVoice voice[NITRO_SOUND_CHANNEL_COUNT];
        NitroSoundCapture capture[2];
        uint32_t outputRate;
        /* Sampled sounds (PCM8, PCM16, ADPCM) go from one sample to the next
         * in a straight line, a sample later, instead of holding each sample
         * (the DS's own way, NitroSound_Reset's default): no harsh images of
         * the sound above it, as on the DS's own output and in emulators. */
        uint8_t interpolate;
        /* Right shift applied to the final left/right mix *before* the 16-bit
         * saturation (0 = no shift). The mixer's sum can leave the 16-bit range;
         * the DS's own 20.8-bit mixer attenuates at the output stage and clips
         * only at its final 10-bit DAC, so applying the headroom here keeps loud
         * peaks smooth instead of flat-topping them and then halving the flat
         * top. The port's output stage sets 1 (6 dB), matching the measured DS
         * title level. */
        uint8_t outputShift;
    } NitroSoundChip;
    
    void NitroSound_Reset(NitroSoundChip *chip, uint32_t outputRate);
    void NitroSound_Mix(NitroSoundChip *chip, int16_t *stereo, size_t frames);
    /* The driver is about to set channel ch's enable bit: if it's 0 now (the
     * channel stopped, perhaps just before, and set up again), the channel
     * starts over from its beginning, as on the DS. */
    void NitroSound_KeyOn(NitroSoundChip *chip, unsigned channel);
    
    #ifdef NITRO_SOUND_STATS
    /* Test-only (3ds_port/nitro/tests/sdat_render.c): the mixer's state for one
     * output frame, handed to the frame hook once the frame is made. Only built
     * with NITRO_SOUND_STATS, so the port's own build has none of it. */
    #define NITRO_SOUND_TAP_OFF INT32_MIN   /* the channel made no sample in the frame */
    #define NITRO_SOUND_EV_START 1u         /* the channel started (its first frame) */
    #define NITRO_SOUND_EV_KILL 2u          /* the driver cleared its enable bit while it played */
    #define NITRO_SOUND_EV_END 4u           /* the sound ended (no repeat) */
    #define NITRO_SOUND_EV_LOOP 8u          /* the sound wrapped to its loop start after this frame */
    
    typedef struct NitroSoundFrame {
        uint32_t index;                     /* output frames since the hook was set */
        int32_t preL, preR;                 /* after master volume, before outputShift */
        int32_t postL, postR;               /* after outputShift, before saturate() */
        /* Per channel: its sample after volume and shift, before pan (what it
         * adds to the mix), or NITRO_SOUND_TAP_OFF. */
        int32_t tap[NITRO_SOUND_CHANNEL_COUNT];
        uint8_t format[NITRO_SOUND_CHANNEL_COUNT];   /* ctrl bits 29-30 */
        uint8_t volume[NITRO_SOUND_CHANNEL_COUNT];   /* ctrl bits 0-6 */
        uint8_t shift[NITRO_SOUND_CHANNEL_COUNT];    /* the shift applied to the sample: 0, 1, 2 or 4 */
        uint8_t event[NITRO_SOUND_CHANNEL_COUNT];    /* NITRO_SOUND_EV_*, for this frame */
    } NitroSoundFrame;
    
    typedef void (*NitroSoundFrameFn)(void *arg, const NitroSoundFrame *frame);
    void NitroSound_SetFrameHook(NitroSoundFrameFn fn, void *arg);
    #endif
    #endif
    # run.ps1: stage kohjoh into Azahar's virtual SD card, launch it, take window
    # screenshots at fixed times and collect the program's log.
    #
    # Azahar is an integration debugger, not a timing authority (kohjoh.md 1):
    # every result written here is AZAHAR-only evidence.
    #
    #   pwsh tools/azahar/run.ps1 -Shots 8,15,25 -Tag boot
    #   pwsh tools/azahar/run.ps1 -Save path\to\heartgold.sav -Script tests\inputs\x.txt
    param(
      [string]$Edition = "heartgold",
      [string]$Artifact = "build/kj/pokeheartgold.3dsx",
      [string]$Pack = "",
      [string]$Manifest = "",
      [string]$Save = "",
      [switch]$FreshSave,
      [string]$Script = "",
      [string]$Shots = "",
      [string]$Holds = "",
      [int]$Timeout = 300,
      [string]$Tag = "run",
      [string]$OutDir = "build/azahar",
      [int]$Extra = 2
    )
    $ErrorActionPreference = "Stop"
    $repo = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path))
    Set-Location $repo
    Add-Type -AssemblyName System.Drawing
    $sig = @'
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hWnd, int x, int y, int w, int h, bool repaint);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    public struct RECT { public int Left, Top, Right, Bottom; }
    '@
    if (-not ("N.W" -as [type])) { Add-Type -MemberDefinition $sig -Name "W" -Namespace "N" | Out-Null }
    [N.W]::SetProcessDPIAware() | Out-Null
    
    $sd = Join-Path $env:APPDATA "Azahar\sdmc\3ds"
    $root = Join-Path $sd "kohjoh"
    foreach ($d in @($root, "$root\data", "$root\saves", "$root\logs")) { New-Item -ItemType Directory -Force -Path $d | Out-Null }
    if ($Pack -ne "") { Copy-Item $Pack (Join-Path $root "data\$Edition.pak") -Force }
    if ($Manifest -ne "") { Copy-Item $Manifest (Join-Path $root "data\$Edition.manifest.json") -Force }
    if ($Save -ne "") { Copy-Item $Save (Join-Path $root "saves\$Edition.sav") -Force }
    if ($FreshSave) { Remove-Item (Join-Path $root "saves\$Edition.sav") -ErrorAction SilentlyContinue }
    $scriptDst = Join-Path $root "script.txt"
    if ($Script -ne "") { Copy-Item $Script $scriptDst -Force } else { Remove-Item $scriptDst -ErrorAction SilentlyContinue }
    $log = Join-Path $root "logs\$Edition.log"
    
    $out = Join-Path $OutDir $Tag
    New-Item -ItemType Directory -Force -Path $out | Out-Null
    $dsx = Join-Path $sd "kohjoh.3dsx"
    Copy-Item $Artifact $dsx -Force
    
    Get-Process azahar -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 800
    $logOffset = if (Test-Path $log) { (Get-Item $log).Length } else { 0 }
    function Read-NewLog {
      if (-not (Test-Path $log)) { return "" }
      $stream = [System.IO.File]::Open($log, [System.IO.FileMode]::Open,
        [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
      try {
        [void]$stream.Seek([Math]::Min($logOffset, $stream.Length), [System.IO.SeekOrigin]::Begin)
        $reader = [System.IO.StreamReader]::new($stream)
        try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
      } finally { $stream.Dispose() }
    ✓ • 448ms

## Activity

    $ rg --files --no-ignore build | rg '(diff.*sound|sound.*diff|test_sound_chip|g3.*test|g3.*prof)' | Select-Object -First 40; Get-Content src/compat/nitro/src/port_sound_chip.c -First 20; Get-Content src/compat/nitro/src/port_sound_chip.c | Select-String 'extern' -Context 0,1; Get-ChildItem C:/devkitPro/msys2/mingw64/bin/gcc.exe,C:/devkitPro/msys2/usr/bin/gcc.exe -ErrorAction SilentlyContinue; Get-Content build/diagnosis/instrumented-build.log -Tail 5; git diff -- src/diagnostics/perf.c
    /*
     * Software model of the DS sound hardware.
     * Only this file owns the playback cursor; the original ARM7 driver writes
     * its control/timer/source registers. All public operations are plain C99.
     *
     * The ARM7 audio service serializes register access with mixing. Registers
     * retain the DS layout so SDK ARM7 snd_channel.c/snd_capture.c can run
     * without modification to their command logic.
     */
    #include <nitro_port_sound_chip.h>
    #include <limits.h>
    #include <stdint.h>
    #include <string.h>
    
    #define SOUND_CLOCK 16756991ull
    #define PHASE_ONE (1ull << 32)
    #define CHANNEL_BASE(ch) (0x400u + (unsigned)(ch) * 16u)
    #define REG_CNT     0u
    #define REG_SAD     4u
    #define REG_TIMER   8u
    
        Directory: C:\devkitPro\msys2\usr\bin
    
    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    -a---          10/17/2026  4:29 AM        2309179 gcc.exe
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/instrumented/gen/overlay_ids.ld (unchanged)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/instrumented/gen/overlay_ids.h (unchanged)
    done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
    diff --git a/src/diagnostics/perf.c b/src/diagnostics/perf.c
    index 4eae1ff..0f88a85 100644
    --- a/src/diagnostics/perf.c
    +++ b/src/diagnostics/perf.c
    @@ -1,3 +1,4 @@
    +#ifndef HGSS_PROFILE_OFF
     /*
      * perf.c: bounded in-memory frame profiler.
      *
    @@ -33,6 +34,8 @@ static uint32_t s_samples[KJ_PERF_STAGE_COUNT][WINDOW];
     static uint32_t s_frame_stage[KJ_PERF_STAGE_COUNT];
     static uint32_t s_interval[WINDOW];
     static unsigned s_n;
    +static uint32_t s_game_logic[WINDOW];
    +static unsigned s_game_n;
     static uint64_t s_vblank_start, s_publish_start, s_logic_start;
     static uint32_t s_vblanks_since_publish, s_misses, s_windows;
     static bool s_any_publish;
    @@ -87,6 +90,21 @@ static int cmp_u32(const void *a, const void *b)
         return x < y ? -1 : x > y;
     }
    
    +/* This window counts every game frame, including skipped pictures. */
    +static void record_game_logic(void)
    +{
    +    s_game_logic[s_game_n++] = s_frame_stage[KJ_PERF_LOGIC];
    +    if (s_game_n == WINDOW) {
    +        qsort(s_game_logic, WINDOW, sizeof(s_game_logic[0]), cmp_u32);
    +        kj_log("[gamecpu] 600 game frames, logic us p50 %lu p95 %lu p99 %lu max %lu (includes preemption; excludes V-blank wait)",
    +               (unsigned long)(s_game_logic[WINDOW / 2] / TICKS_PER_US),
    +               (unsigned long)(s_game_logic[WINDOW * 95 / 100] / TICKS_PER_US),
    +               (unsigned long)(s_game_logic[WINDOW * 99 / 100] / TICKS_PER_US),
    +               (unsigned long)(s_game_logic[WINDOW - 1] / TICKS_PER_US));
    +        s_game_n = 0;
    +    }
    +}
    +
     static void report(void)
     {
         static const char *const names[KJ_PERF_STAGE_COUNT] = {
    @@ -167,7 +185,11 @@ static void note_hitch(uint32_t game_frame, uint32_t refreshes)
     void kj_perf_present_skipped(uint32_t game_frame)
     {
         ++s_skipped;
    -    note_hitch(game_frame, 0);
    +    (void)game_frame;
    +    record_game_logic();
    +    memset(s_frame_stage, 0, sizeof(s_frame_stage));
    +    memset(kj_perf_counts, 0, sizeof(kj_perf_counts));
    +    s_logic_start = svcGetSystemTick();
     }
    
     void kj_perf_publish_end(uint32_t game_frame)
    @@ -179,9 +201,6 @@ void kj_perf_publish_end(uint32_t game_frame)
         s_frame_time[s_n] = s_any_publish ? (uint32_t)(now - s_last_submit) : 0u;
         s_last_real = real;
         s_last_submit = now;
    -    if (s_any_publish && real_interval != 2u) {
    -        note_hitch(game_frame, real_interval);
    -    }
         kj_perf_add(KJ_PERF_PUBLISH, now - s_publish_start);
         /* kj_g3d_draw runs inside the screen draw: report g2d exclusive of it,
          * so logic + g2d + g3d + gpu_wait + submit is not double-counted. */
    @@ -190,6 +209,10 @@ void kj_perf_publish_end(uint32_t game_frame)
             const uint32_t g3d = s_frame_stage[KJ_PERF_G3D];
             *g2d = *g2d > g3d ? *g2d - g3d : 0u;
         }
    +    if (s_any_publish && real_interval != 2u) {
    +        note_hitch(game_frame, real_interval);
    +    }
    +    record_game_logic();
         memset(kj_perf_counts, 0, sizeof(kj_perf_counts));
         for (int s = 0; s < KJ_PERF_STAGE_COUNT; ++s) {
             s_samples[s][s_n] = s_frame_stage[s];
    @@ -201,9 +224,11 @@ void kj_perf_publish_end(uint32_t game_frame)
         }
         s_any_publish = true;
         s_vblanks_since_publish = 0;
    -    s_logic_start = now;
         if (++s_n == WINDOW) {
             s_n = 0;
             report();
         }
    +    s_logic_start = svcGetSystemTick();
     }
    +
    +#endif
    
    ✓ • 906ms

## Activity

    $ Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1650 -First 160; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 2255 -First 130; Get-Content src/compat/nitro/src/port_sound_chip.c | Select-String 'Resolve|Read|Write|extern|Map' -Context 1,1; rg --files --no-ignore C:/Users/terezi/workdir | rg 'sound.*(diff|test)|diff.*sound' | Select-Object -First 25
     * blocks (skinned strips).
     */
    
    #define DL_SLOTS 1024u
    #define DL_MIN_WORDS 16u
    #define DL_BYTES_MAX (6u * 1024u * 1024u)
    #define DL_MAX_EVENTS 4096u
    
    typedef struct dl_key {
        uint32_t attr_pending, attr, tex_param, tex_pltt, viewport;
        uint32_t color, coord01, coord2;
        int32_t texcoord[2], texcoord_raw[2];
        uint32_t mtx_mode;
        int32_t texm[4][4];
        int32_t vec[3][3];
        int32_t light_dir[4][3], light_half[4][3];
        uint32_t light_color[4];
        uint32_t material[4];
        uint32_t use_shininess;
        uint32_t shininess[32];
    } dl_key;
    
    typedef struct dl_exit {
        uint16_t color;
        int16_t coord[3];
        int32_t texcoord[2], texcoord_raw[2];
        bool in_begin;
        int primitive;
        uint32_t prim_count, strip_parity;
        uint32_t prim_rel[4];
        bool prim_valid;
    } dl_exit;
    
    /* event: high bit set = BLOCK (index into blocks), else STATE (word offset) */
    #define DL_EV_BLOCK 0x80000000u
    
    typedef struct dl_block {
        uint32_t first_vert, nverts; /* into the entry's vertex array */
        uint32_t first_idx, nidx;    /* relative to the block's first vertex */
    } dl_block;
    
    /* Unrecordable lists recheck their content once per this many pictures */
    #define DL_NEVER_RECHECK_MASK 63u
    
    typedef struct dl_entry {
        const uint32_t *ptr;
        uint32_t count;
        bool recorded;
        bool never;        /* proven unrecordable for this content */
        bool needs_light;  /* seen lighting its normals: key on lighting state */
        uint32_t reads;    /* DL_READS_*: entry inputs the list consumes */
        uint32_t *words;
        dl_key key;
        bool lit, texgen;
        dl_exit exit;
        uint32_t *events;
        uint32_t nevents;
        dl_block *blocks;
        uint32_t nblocks;
        kj_g3_vertex *verts;
        uint32_t nverts;
        uint16_t *idx;
        uint32_t nidx;
        uint32_t cap_events, cap_blocks, cap_verts, cap_idx; /* storage kept across re-recordings */
        uint32_t bytes;
        uint32_t last_frame;
    } dl_entry;
    
    /* Recording state (one list at a time; lists do not nest). */
    typedef struct dl_rec {
        bool active, bad, has_matrix, vecdep;
        const uint32_t *base;
        uint32_t events[DL_MAX_EVENTS];
        uint32_t nevents;
        dl_block blocks[DL_MAX_EVENTS];
        uint32_t nblocks;
        uint32_t block_v0, block_i0, block_b0; /* frame positions at block start */
        uint32_t v0, i0;                       /* frame positions at list start */
    } dl_rec;
    
    static dl_entry s_dl[DL_SLOTS];
    
    /* All recordings live in one bump arena, flushed wholesale when full: no
     * per-list heap traffic (thousands of small malloc/free pairs a frame), and
     * no per-entry ownership to get wrong. */
    static uint8_t *s_dl_arena;
    static uint32_t s_dl_arena_used;
    static uint32_t s_dl_flushes;
    
    static void *dl_alloc(uint32_t n)
    {
        if (s_dl_arena == NULL) {
            s_dl_arena = malloc(DL_BYTES_MAX);
            if (s_dl_arena == NULL) {
                return NULL;
            }
        }
        n = (n + 3u) & ~3u;
        if (n == 0 || s_dl_arena_used + n > DL_BYTES_MAX) {
            return NULL;
        }
        void *ptr = s_dl_arena + s_dl_arena_used;
        s_dl_arena_used += n;
        return ptr;
    }
    
    static void dl_flush_all(void)
    {
        memset(s_dl, 0, sizeof(s_dl));
        s_dl_arena_used = 0;
    #ifndef HGSS_PROFILE_OFF
        ++s_dl_flushes;
    #endif
    }
    static dl_rec s_rec;
    static uint32_t s_dl_bytes;
    static uint32_t s_dl_hits, s_dl_misses, s_dl_uncacheable;
    /* Word coverage of the cache: short lists, a new slot, changed contents,
     * proven uncacheable, replayed, recorded but missed on the entry key. */
    enum { DLC_SHORT, DLC_SMALL, DLC_BEGIN, DLC_BEGIN_FIRST, DLC_SPLIT, DLC_NEW, DLC_CHANGED, DLC_NEVER, DLC_HIT, DLC_KEY, DLC_COUNT };
    static uint32_t s_dl_cov[DLC_COUNT];
    static uint32_t s_dl_why[6]; /* batch-in-block, shared-verts, straddle/drops, lit+matrix, swap, open-prim */
    
    /* Close the vertex block in progress (if it produced anything). */
    static void rec_close_block(void)
    {
        frame_buf *fb = &s_buf[s_build];
        const uint32_t v1 = fb->frame.vert_count, i1 = fb->frame.index_count;
    
        if (v1 == s_rec.block_v0 && i1 == s_rec.block_i0) {
            return;
        }
        const uint32_t opened = fb->frame.batch_count - s_rec.block_b0;
        if (opened > 1u || (opened == 1u && fb->batches[fb->frame.batch_count - 1].first_index != s_rec.block_i0) ||
            s_rec.nevents >= DL_MAX_EVENTS || s_rec.nblocks >= DL_MAX_EVENTS) {
            s_rec.bad = true; /* batches changed inside the block, or too many events */
    #ifndef HGSS_PROFILE_OFF
            ++s_dl_why[0];
    #endif
        }
        for (uint32_t i = s_rec.block_i0; i < i1 && !s_rec.bad; ++i) {
            if (fb->indices[i] < s_rec.block_v0) {
                s_rec.bad = true; /* shares vertices with an earlier block */
    #ifndef HGSS_PROFILE_OFF
                ++s_dl_why[1];
    #endif
            }
        }
        if (!s_rec.bad) {
            dl_block *b = &s_rec.blocks[s_rec.nblocks];
            b->first_vert = s_rec.block_v0 - s_rec.v0;
            b->nverts = v1 - s_rec.block_v0;
            b->first_idx = s_rec.block_i0 - s_rec.i0;
            b->nidx = i1 - s_rec.block_i0;
            s_rec.events[s_rec.nevents++] = DL_EV_BLOCK | s_rec.nblocks++;
        }
    }
    
    static void rec_open_block(void)
    {
        if (count < DL_MIN_WORDS || count > 0xFFFFFu || s_in_begin || s_fifo_need > 0 || s_rec.active) {
    #ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_SHORT] += (uint32_t)count;
    #endif
            if (count < DL_MIN_WORDS) {
    #ifndef HGSS_PROFILE_OFF
                s_dl_cov[DLC_SMALL] += (uint32_t)count;
    #endif
            } else if (s_fifo_need > 0) {
    #ifndef HGSS_PROFILE_OFF
                s_dl_cov[DLC_SPLIT] += (uint32_t)count;
    #endif
            } else if (s_in_begin) {
    #ifndef HGSS_PROFILE_OFF
                s_dl_cov[DLC_BEGIN] += (uint32_t)count;
    #endif
                if ((words[0] & 0xFFu) == 0x40u) {
    #ifndef HGSS_PROFILE_OFF
                    s_dl_cov[DLC_BEGIN_FIRST] += (uint32_t)count;
    #endif
                }
            }
            decode_words(words, count);
            return;
        }
        dl_entry *e = dl_slot(words, (uint32_t)count);
        if (e == NULL) {
            decode_words(words, count);
            return;
        }
        if (e->ptr != words || e->count != count) {
    #ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_NEW] += (uint32_t)count;
    #endif
            dl_free(e);
            uint32_t *copy = dl_alloc((uint32_t)count * 4u);
            if (copy == NULL) {
                dl_flush_all();
                decode_words(words, count);
                return;
            }
            e->words = copy;
            e->ptr = words;
            e->count = (uint32_t)count;
            memcpy(e->words, words, count * 4u);
            e->reads = dl_entry_reads(words, count);
        } else if (e->never && (s_stats.swaps & DL_NEVER_RECHECK_MASK) != 0u) {
            /* An unrecordable list decodes live whatever its content, so the
             * content check only matters for noticing it became recordable;
             * the title's lit Ho-Oh lists spent 6.8% of samples comparing */
            e->last_frame = s_stats.swaps;
    #ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_NEVER] += (uint32_t)count;
    #endif
    #ifndef HGSS_PROFILE_OFF
            ++s_dl_uncacheable;
    #endif
            decode_words(words, count);
            return;
        } else if (memcmp(e->words, words, count * 4u) != 0) {
    #ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_CHANGED] += (uint32_t)count;
    #endif
            memcpy(e->words, words, count * 4u);
            dl_drop_recording(e);
            e->never = false;
            e->needs_light = false;
            e->reads = dl_entry_reads(words, count);
        }
        e->last_frame = s_stats.swaps;
        if (e->never) {
    #ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_NEVER] += (uint32_t)count;
    #endif
    #ifndef HGSS_PROFILE_OFF
            ++s_dl_uncacheable;
    #endif
            decode_words(words, count);
            return;
        }
        const bool texgen = (s_tex_param >> 30) != 0;
        const bool lit = e->needs_light || ((s_attr | s_attr_pending) & 0xFu) != 0;
        dl_key key;
        bool verify = false;
        dl_make_key(&key, lit, texgen, e->reads);
        if (e->recorded && e->lit == lit && e->texgen == texgen &&
            memcmp(&key, &e->key, sizeof(key)) == 0) {
            frame_buf *fb = &s_buf[s_build];
            if (fb->frame.vert_count + dl_vertex_bound(e) <= KJ_G3_MAX_VERTS &&
                fb->frame.index_count + e->nidx <= KJ_G3_MAX_INDICES &&
                fb->frame.batch_count + e->nblocks + 2 <= KJ_G3_MAX_BATCHES) {
    #ifdef KJ_DL_VERIFY
                verify = true;
    #else
                dl_replay(e);
    #ifndef HGSS_PROFILE_OFF
                ++s_dl_hits;
    #endif
    #ifndef HGSS_PROFILE_OFF
                s_dl_cov[DLC_HIT] += (uint32_t)count;
    #endif
                return;
    #endif
            }
        }
    
    #ifdef KJ_DL_VERIFY
        if (e->recorded && !verify) {
            static uint32_t logged;
            if (logged < 30 && s_stats.swaps > 4600u) {
                ++logged;
                const uint32_t *a = (const uint32_t *)&key, *b = (const uint32_t *)&e->key;
                unsigned first = 0;
                while (first < sizeof(key) / 4 && a[first] == b[first]) {
                    ++first;
                }
                NitroPort_Log("[dlmiss] list %p lit %d/%d texgen %d/%d key word %u: %08lx vs %08lx",
                              (const void *)words, lit, e->lit, texgen, e->texgen, first,
                              first < sizeof(key) / 4 ? (unsigned long)a[first] : 0ul,
                              first < sizeof(key) / 4 ? (unsigned long)b[first] : 0ul);
            }
        }
    #endif
        /* record */
        if (verify) {
    #ifndef HGSS_PROFILE_OFF
            s_dl_cov[DLC_HIT] += (uint32_t)count;
    #endif
        } else if (e->recorded) {
    #ifndef HGSS_PROFILE_OFF
    
       * Software model of the DS sound hardware.
    >  * Only this file owns the playback cursor; the original ARM7 driver writes
       * its control/timer/source registers. All public operations are plain C99.
       * a channel's registers while it mixes (it switches a channel off at the end
    >  * of its sound), so they're read once per call, not once per sample, and the
       * sound's length, loop point and step are worked out once too. What comes
    >  * out is exactly what reading them every sample gave.
       */
          unsigned shift;
    >     uint32_t endByte;      /* the channel reads [sourceAddr, sourceAddr + endByte) */
    >     int dependent;         /* reads memory a capture writes: mixed frame by frame */
          int interpolate;       /* NitroSoundChip.interpolate, for a sampled sound */
    
    > /* Read a channel's registers at the start of a mix; 0 if it's off. */
      static int start_channel(NitroSoundChip *chip, unsigned channel, Channel *c)
       * the sample it left behind. Returns 0 if the sound ended (advance_voice
    >  * has written its state back already), 1 if the block goes on. This is the
       * phase loop the PCM branch used to hold inline, split out so the silent
       * decoder in local variables; values (if not NULL) gets each frame's
    >  * sample. For channels that don't read what a capture writes: they don't
       * depend on the other channels within a mix.
              /* PCM8 / PCM16. One loop per pan case, hoisted out of the frame
    >          * loop; step and shift are already block constants. */
              const uint8_t *source = c->source;
                   * lifts would be the wrong one. Only an interpolating channel
    >              * reads the source here, to keep lastSample. */
                  for (i = 0; i < frames; ++i) {
       * the sequencer split off in port_snd.c). Counters only; no mixing math
    >  * touched. The mixer runs under the audio lock on one thread (tests are
    >  * single-threaded), so plain integers, no atomics. port_snd.c reads these
       * for its [sndprof] line via NitroSound_MixProfile below. */
      static uint64_t sProfCapturing;     /* capture instances running, summed over calls (0-2 each) */
    > static uint64_t sProfDependentCh;   /* channels reading a capture buffer, summed over calls */
    
          if (capturing) {
    >         /* A channel that reads a capture's buffer (reverb plays the
               * recorded mix back on channels 1 and 3) depends on what the
    >          * capture writes frame by frame: those are mixed frame by frame,
               * in their place before the capture. The others don't depend on
          }
    >     /* Channels that ended: as reading their (now off) registers left them. */
          for (k = 0; k < count; ++k) {
    C:/Users/terezi/workdir\coffeepot\private_please_gitignore\switch\SwitchWakerHD\runtime\tools\menu_sounds_test.cpp
    C:/Users/terezi/workdir\hgss-3ds\tests\sdk\arm7_sound\test_arm9_flush.c
    C:/Users/terezi/workdir\hgss-3ds\tests\sdk\arm7_sound\test_arm7_sound.c
    
    ✓ • 3.46s

## Activity

    $ Get-Content src/compat/nitro/src/port_sound_chip.c | Select-Object -Skip 260 -First 110; Get-Content build/diagnosis/instrumented-build.log -Tail 6; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1810 -First 40; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1900 -First 37; Get-ChildItem C:/devkitPro/msys2/usr/bin/*clang* -ErrorAction SilentlyContinue
    static void disable_channel(unsigned base, NitroSoundVoice *voice)
    {
        STATS_EVENT(STATS_CHANNEL_OF(base), NITRO_SOUND_EV_END);
        put32(base + REG_CNT, reg32(base + REG_CNT) & 0x7fffffffu);
        voice->running = 0;
        voice->previousCtrl &= 0x7fffffffu;
    }
    
    static void restart_channel(NitroSoundVoice *voice, uint32_t ctrl,
                                unsigned base)
    {
        const uint8_t *source = (const uint8_t *)(uintptr_t)reg32(base + REG_SAD);
        unsigned format = (ctrl >> 29) & 3u;
        STATS_EVENT(STATS_CHANNEL_OF(base), NITRO_SOUND_EV_START);
        voice->running = 1;
        voice->sampleIndex = 0;
        voice->phase = 0;
        voice->noise = 0x7fffu;
        voice->predictor = 0;
        voice->stepIndex = 0;
        voice->lastSample = 0;
        voice->loopSaved = 0;
        voice->adpcmClean = 0;
        if (format == 2u && source) {
            adpcm_init(voice, source);
            voice->adpcmClean = 1;
            voice->adpcmSource = (uint32_t)(uintptr_t)source;
        }
    }
    
    /*
     * One channel during one NitroSound_Mix call. Only the mixer itself changes
     * a channel's registers while it mixes (it switches a channel off at the end
     * of its sound), so they're read once per call, not once per sample, and the
     * sound's length, loop point and step are worked out once too. What comes
     * out is exactly what reading them every sample gave.
     */
    typedef struct Channel {
        NitroSoundVoice *v;
        unsigned channel;
        unsigned base;
        unsigned format;
        unsigned repeat;
        uint32_t ctrl;
        uint32_t sourceAddr;
        const uint8_t *source;
        uint32_t nSamples;
        uint32_t loopAt;
        uint64_t step;
        int32_t volume;
        int32_t pan;
        unsigned shift;
        uint32_t endByte;      /* the channel reads [sourceAddr, sourceAddr + endByte) */
        int dependent;         /* reads memory a capture writes: mixed frame by frame */
        int interpolate;       /* NitroSoundChip.interpolate, for a sampled sound */
    #ifdef NITRO_SOUND_STATS
        int32_t *tap;          /* the frame hook's samples for this channel, if set */
    #endif
    } Channel;
    
    /* Between the sample before (a) and the sample now (b), as far along as
     * the playback cursor is (phase's fraction). */
    static inline int32_t lerp(int32_t a, int32_t b, uint64_t phase)
    {
        return a + (int32_t)(((int64_t)(b - a) * (int64_t)(uint32_t)phase) >> 32);
    }
    
    static int32_t wave_sample(const Channel *c)
    {
        const NitroSoundVoice *v = c->v;
    
        switch (c->format) {
        case 0:
            return c->source ? (int32_t)pcm8(c->source, v->sampleIndex) * 256 : 0;
        case 1:
            return c->source ? pcm16(c->source, v->sampleIndex * 2u) : 0;
        case 2:
            return c->source ? v->predictor : 0;
        default:
            if (c->channel >= 14) return (v->noise & 1u) ? -32767 : 32767;
            if (c->channel < 8) return 0;
            /* Duty 7 is 0% high (GBATEK): silence, not a full-scale DC level. */
            if (((c->ctrl >> 24) & 7u) == 7u) return 0;
            return (v->sampleIndex & 7u) <= ((c->ctrl >> 24) & 7u) ? 32767 : -32767;
        }
    }
    
    /* The ADPCM decoder's state at the loop start, as it was when the sound
     * first got there; without one saved (or for other data), decoded again
     * from the start. */
    static void adpcm_to_loop(NitroSoundVoice *v, const Channel *c)
    {
        uint32_t i;
    
        if (c->loopAt != 0 && v->loopSaved && v->loopSource == c->sourceAddr &&
            v->loopAt == c->loopAt) {
            v->predictor = v->loopPredictor;
            v->stepIndex = v->loopStepIndex;
            return;
        }
        adpcm_init(v, c->source);
        for (i = 1u; i <= c->loopAt; ++i) (void)adpcm_at(v, c->source, i);
        v->adpcmClean = 1;
        v->adpcmSource = c->sourceAddr;
        if (c->loopAt != 0) {
            v->loopSaved = 1;
            v->loopSource = c->sourceAddr;
            v->loopAt = c->loopAt;
            v->loopPredictor = v->predictor;
            v->loopStepIndex = v->stepIndex;
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/instrumented/gen/overlay_ids.h (unchanged)
    done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
       1092 compiled, 0 failed
    == assembling ARM9 .s (150 files)
        frame_buf *fb = &s_buf[s_build];
        s_rec.block_v0 = fb->frame.vert_count;
        s_rec.block_i0 = fb->frame.index_count;
        s_rec.block_b0 = fb->frame.batch_count;
    }
    
    /* Called by decode_words before each command while recording. */
    static void rec_command(unsigned op, const uint32_t *p)
    {
        if (op >= 0x20 && op <= 0x28) {
            if (op == 0x21 && (s_attr & 0xFu)) {
                s_rec.vecdep = true;
            }
            if ((s_tex_param >> 30) >= 2u) {
                s_rec.vecdep = true;
            }
            if (s_rec.nblocks == 0 && s_rec.nevents == 0) {
                /* first vertex-data command opens the first block lazily */
            }
            return;
        }
        if (op == 0x50) {
            s_rec.bad = true;
    #ifndef HGSS_PROFILE_OFF
            ++s_dl_why[4];
    #endif
        }
        if (op >= 0x10 && op <= 0x1C) {
            s_rec.has_matrix = true;
        }
        /* a state command ends the current block; the block after it starts
         * once this command has run (its effect on batches belongs to it) */
        rec_close_block();
        if (s_rec.nevents >= DL_MAX_EVENTS) {
            s_rec.bad = true;
            return;
        }
        s_rec.events[s_rec.nevents++] = (uint32_t)(p - s_rec.base) | ((uint32_t)op << 20);
    }
    
        k->tex_param = s_tex_param;
        k->tex_pltt = s_tex_pltt;
        k->viewport = s_viewport;
        if (reads & DL_READS_COLOR) {
            k->color = s_color;
        }
        if (reads & DL_READS_COORD) {
            k->coord01 = (uint16_t)s_coord[0] | ((uint32_t)(uint16_t)s_coord[1] << 16);
            k->coord2 = (uint16_t)s_coord[2];
        }
        if (reads & DL_READS_TEXCOORD) {
            k->texcoord[0] = s_texcoord[0];
            k->texcoord[1] = s_texcoord[1];
            k->texcoord_raw[0] = s_texcoord_raw[0];
            k->texcoord_raw[1] = s_texcoord_raw[1];
        }
        k->mtx_mode = (uint32_t)s_mtx_mode;
        if (texgen || (reads & DL_READS_TEXM)) {
            memcpy(k->texm, s_tex.m, sizeof(k->texm));
        }
        if (lit) {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    k->vec[i][j] = s_vec.m[i][j];
                }
            }
            memcpy(k->light_dir, s_light_dir, sizeof(k->light_dir));
            memcpy(k->light_half, s_light_half, sizeof(k->light_half));
            for (int i = 0; i < 4; ++i) {
                k->light_color[i] = s_light_color[i];
            }
            k->material[0] = s_diffuse;
            k->material[1] = s_ambient;
            k->material[2] = s_specular;
            k->material[3] = s_emission;
            k->use_shininess = s_use_shininess;
            if (s_use_shininess) {
    ✓ • 505ms

## Activity

    $ Get-Content build/diagnosis/instrumented-build.log -Tail 6; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1400 -First 140; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1888 -First 15; Get-ChildItem C:/devkitPro/msys2/usr/bin/*gdb* -ErrorAction SilentlyContinue; rg -n 'gcc|diff|sound' docs/chatlog07.md | Select-Object -Last 12; Get-Content src/runtime/kj_g3.h -First 100
      CC  os_alarm_before_handler.c
      CC  os_alloc.c
      CC  os_arena.c
      CC  os_context.c
      CC  os_spin_lock_card.c
      CC  os_spinLock.c
            if (s_fifo_have >= s_fifo_need) {
                execute(s_fifo_op, s_fifo_params);
                s_fifo_need = 0;
                fifo_next_command();
            }
            return;
        }
        s_fifo_cmds = value;
        s_fifo_left = 4;
        fifo_next_command();
    }
    
    
    static uint8_t s_param_count[256];
    
    /* Vertex fast path for the bulk decoder: everything the general path
     * (submit_vertex) checks is already settled -- inside BEGIN, the open batch
     * matches the material, the current clip matrix has a palette slot in it,
     * no per-vertex texgen, room for the vertex and a quad's indices. Writes
     * the vertex and assembles the primitive inline. Returns false to make the
     * caller take the general path. Kept small: it runs ~14k times a frame in
     * Goldenrod (ARM11 L1 I-cache: coffeepot icachebench). */
    static bool fast_vertex(void)
    {
        if (!s_in_begin || !s_batch_ok || s_cur_gmtx_version != s_clip_version ||
            s_slot_gid != s_cur_gmtx || (s_tex_param >> 30) == 3) {
            return false;
        }
        frame_buf *fb = &s_buf[s_build];
        const uint32_t n = fb->frame.vert_count;
        uint32_t ic = fb->frame.index_count;
        if (n >= KJ_G3_MAX_VERTS || ic + 6 > KJ_G3_MAX_INDICES || ((s_attr >> 16) & 31u) == 0) {
            return false;
        }
        uint32_t *w = (uint32_t *)&fb->verts[n];
        w[0] = (uint16_t)s_coord[0] | ((uint32_t)(uint16_t)s_coord[1] << 16);
        w[1] = (uint16_t)s_coord[2] | ((uint32_t)s_slot_idx << 16);
        w[2] = s_color_rgba;
        w[3] = (uint16_t)s_texcoord[0] | ((uint32_t)(uint16_t)s_texcoord[1] << 16);
        fb->vert_gmtx[n] = (uint16_t)s_cur_gmtx;
        fb->frame.vert_count = n + 1;
    
        const uint32_t k = s_prim_count++;
        const uint32_t first = s_batch_first_vert;
        uint16_t *idx = fb->indices + ic;
        uint32_t a, b, c, d;
        switch (s_primitive) {
        case 0:
            s_prim_verts[k % 3] = n;
            if (k % 3 != 2) {
                return true;
            }
            a = s_prim_verts[0];
            b = s_prim_verts[1];
            if (a < first || b < first) {
                break;
            }
            idx[0] = (uint16_t)a;
            idx[1] = (uint16_t)b;
            idx[2] = (uint16_t)n;
            ic += 3;
            goto done;
        case 1:
            s_prim_verts[k & 3] = n;
            if ((k & 3) != 3) {
                return true;
            }
            a = s_prim_verts[0];
            b = s_prim_verts[1];
            c = s_prim_verts[2];
            if (a < first || b < first || c < first) {
                break;
            }
            idx[0] = (uint16_t)a;
            idx[1] = (uint16_t)b;
            idx[2] = (uint16_t)c;
            idx[3] = (uint16_t)a;
            idx[4] = (uint16_t)c;
            idx[5] = (uint16_t)n;
            ic += 6;
            goto done;
        case 2:
            if (k < 2) {
                s_prim_verts[k] = n;
                return true;
            }
            a = s_prim_verts[0];
            b = s_prim_verts[1];
            s_prim_verts[0] = b;
            s_prim_verts[1] = n;
            if (a < first || b < first) {
                if (s_strip_parity == 0) {
                    emit_triangle(a, b, n);
                } else {
                    emit_triangle(b, a, n);
                }
                s_strip_parity ^= 1;
                return true;
            }
            if (s_strip_parity == 0) {
                idx[0] = (uint16_t)a;
                idx[1] = (uint16_t)b;
            } else {
                idx[0] = (uint16_t)b;
                idx[1] = (uint16_t)a;
            }
            idx[2] = (uint16_t)n;
            s_strip_parity ^= 1;
            ic += 3;
            goto done;
        default:
            if (k < 2) {
                s_prim_verts[k] = n;
                return true;
            }
            if ((k & 1) == 0) {
                s_prim_verts[2] = n;
                return true;
            }
            a = s_prim_verts[0];
            b = s_prim_verts[1];
            d = s_prim_verts[2];
            s_prim_verts[0] = d;
            s_prim_verts[1] = n;
            if (a < first || b < first || d < first) {
                emit_triangle(a, b, n);
                emit_triangle(a, n, d);
                return true;
            }
            idx[0] = (uint16_t)a;
            idx[1] = (uint16_t)b;
            idx[2] = (uint16_t)n;
            idx[3] = (uint16_t)a;
            idx[4] = (uint16_t)n;
            idx[5] = (uint16_t)d;
            ic += 6;
            goto done;
        }
        /* triangles/quads touching vertices from before the batch: general path */
        if (s_primitive == 0) {
                }
                i += s_param_count[op];
            }
        }
        return reads;
    }
    
    static void dl_make_key(dl_key *k, bool lit, bool texgen, uint32_t reads)
    {
        memset(k, 0, sizeof(*k));
        k->attr_pending = s_attr_pending;
        k->attr = s_attr;
        k->tex_param = s_tex_param;
        k->tex_pltt = s_tex_pltt;
        k->viewport = s_viewport;
    
        Directory: C:\devkitPro\msys2\usr\bin
    
    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    -a---           4/17/2025  3:44 AM          26226 gdbm_dump.exe
    -a---           4/17/2025  3:44 AM          29125 gdbm_load.exe
    -a---           4/17/2025  3:44 AM         131976 gdbmtool.exe
    -a---           4/17/2025  3:44 AM          13806 msys-gdbm_compat-4.dll
    -a---           4/17/2025  3:44 AM          59090 msys-gdbm-6.dll
    15:These measurements are from different passes through the scene, not perfectly matched A/B runs.
    30:Both report millions of display-list cache hits. So these are not cache-disabled versus cache-enabled runs, despite the different commit dates.
    47:The number of active channels is slightly different, but normalized cost also improves, from roughly 169 to 130 ticks per channel-sample.
    /*
     * kj_g3.h: the DS geometry engine (DS side). See kj_g3.c.
     */
    
    #ifndef KJ_G3_H
    #define KJ_G3_H
    
    #include "kj_bridge.h"
    
    
    /* The most recently swapped scene (the DS keeps drawing it until the next
     * SWAP_BUFFERS), or NULL before the first swap. Valid until the swap after
     * next; the renderer only reads it inside kj_ctr_publish_frame. */
    const kj_g3_frame *kj_g3_take_frame(void);
    
    /* Widescreen: the renderer shows 25% more horizontal clip space than the
     * DS. The box test (the game's visibility culling) is widened to match so
     * objects in the extension are not culled. */
    void kj_g3_set_wide(bool wide);
    
    /* Vertex/index storage lives in GPU-readable memory; the 3DS side provides
     * it at startup (before the game runs). Two sets: build and display. */
    typedef struct kj_g3_storage {
        kj_g3_vertex *verts[2];
        uint16_t *indices[2];
    } kj_g3_storage;
    
    void kj_g3_init(const kj_g3_storage *storage);
    
    /* Counters for diagnostics. */
    typedef struct kj_g3_stats {
        uint32_t swaps;
        uint32_t commands;
        uint32_t box_tests;
        uint32_t box_culled;
    } kj_g3_stats;
    
    void kj_g3_get_stats(kj_g3_stats *out);
    void kj_g3_dl_stats(uint32_t *hits, uint32_t *misses, uint32_t *uncacheable, uint32_t *bytes);
    void kj_g3_take_profile(uint64_t *fifo_ticks, uint32_t *fifo_words, uint32_t *direct_words,
                            uint32_t *commands);
    
    #endif
    
    ✓ • 762ms

## Activity

    $ @'
        from pathlib import Path
        p=Path('src/runtime/kj_g3.c'); s=p.read_text(); pos=s.index('static void dl_make_key(')
        s=s[:pos]+'''/* The unused tail is not part of the key. Most map lists consume neither
         * texgen matrices nor shininess; clearing/comparing those bytes on every
         * cache lookup needlessly streams through the ARM11's small data cache.
         * All bytes in the selected prefix are initialized, including padding. */
        static size_t dl_key_size(bool lit, bool texgen, uint32_t reads)
        {
            if (lit) {
                return s_use_shininess ? sizeof(dl_key) : offsetof(dl_key, shininess);
            }
            return texgen || (reads & DL_READS_TEXM) ? offsetof(dl_key, vec)
                                                  : offsetof(dl_key, texm);
        }
        
        '''+s[pos:]; s=s.replace('memset(k, 0, sizeof(*k));','memset(k, 0, dl_key_size(lit, texgen, reads));',1); s=s.replace('    dl_make_key(&key, lit, texgen, e->reads);','    const size_t key_bytes = dl_key_size(lit, texgen, e->reads);\n    dl_make_key(&key, lit, texgen, e->reads);'); s=s.replace('memcmp(&key, &e->key, sizeof(key))','memcmp(&key, &e->key, key_bytes)'); s=s.replace('first < sizeof(key) / 4','first < key_bytes / 4'); s=s.replace('    e->key = key;', '    memcpy(&e->key, &key, key_bytes);'); p.write_text(s)
        '@ | python
        Get-Content build/diagnosis/instrumented-build.log -Tail 5
      1 NitroSystem header inline functions
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
      129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
    == duplicate symbols
    ✓ • 244ms

## Assistant

The CPU load is a combination. In the steady field window, audio mixing consumes roughly 18% of the application core. The FIFO samples indicate substantial decode and replay work too, though the fixed sampling stride makes an exact split unreliable.

I found two concrete costs to reduce: cache lookups clear and compare key fields that many lists never use, and ADPCM decoding repeats the same bit tests and arithmetic for each nibble. I’ve changed the key handling to touch only relevant fields and added a small table of exact ADPCM deltas. I’m checking correctness and generated ARM code before keeping either optimization.

## Activity

    $ @'
        from pathlib import Path
        p=Path('src/graphics/renderer.c'); s=p.read_text().replace('static bool s_perflog_on;', 'static bool s_perflog_on;\nstatic bool s_perflog_valid;'); s=s.replace('if (s_perflog_on && R_SUCCEEDED(GSPGPU_GetPerfLog(&now))) {','''bool valid = false;
                    if (s_perflog_on && R_SUCCEEDED(GSPGPU_GetPerfLog(&now))) {
                        valid = s_perflog_valid;'''); s=s.replace('                s_perflog_prev = now;','''                s_perflog_prev = now;
                        s_perflog_valid = true;
                    } else {
                        s_perflog_valid = false;'''); s=s.replace('            kj_log("[gputime]', '''            /* Discard the first window: GSP start timestamps survive enable,
                     * and the first observed P3D total was nonsensical on O3DS log 3.
                     * A failed read also requires a fresh baseline. */
                    if (!valid) p3d = ppf = psc = 0;
                    kj_log("[gputime]'''); s=s.replace('s_perflog_on ? "on" : "off"', 'valid ? "valid" : "baseline/unavailable"'); p.write_text(s)
        # Build a small exact delta table: 89 step indices x 8 magnitude codes.
        p=Path('src/compat/nitro/src/port_sound_chip.c'); s=p.read_text(); import re
        steps=list(map(int,re.search(r'sStep\[89\] = \{(.*?)\};',s,re.S)[1].replace('\n','').split(',')))
        table='''/* Exact sums of the separately truncated ADPCM shifts. 1,424 bytes
         * replaces three bit tests and additions per decoded sample. See Coffeepot
         * ctr/engineering/cpu-optimization.md (small tables and branch costs).
         * Do not replace with step*(2*code+1)/8: its rounding differs. */
        static const uint16_t sAdpcmDelta[89][8] = {
        '''
        for st in steps:
            table+='    {'+', '.join(str((st>>3)+((st>>2) if c&1 else 0)+((st>>1) if c&2 else 0)+(st if c&4 else 0)) for c in range(8))+'},\n'
        table+='};\n'
        s=s.replace('static const int8_t sIndexDelta',table+'static const int8_t sIndexDelta',1)
        a=s.index('#ifdef HGSS_SND_ADPCM',s.index('static inline __attribute__((always_inline)) size_t adpcm_render_frames'))
        b=s.index('                last = predictor;',a)
        s=s[:a]+'''                const unsigned code = (nibble & 1u) ? byte >> 4 : byte & 15u;
                        const int32_t change = sAdpcmDelta[stepIndex][code & 7u];
        '''+s[b:]; p.write_text(s)
        '@ | python
        C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && HGSS_PROFILE=on python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/instrumented -j 32 > build/diagnosis/instrumented-build.log 2>&1'
    ✓ • 1m 47s

## Activity

    $ @'
        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>
        #include SOURCE
        static uint32_t rng = 42;
        static uint32_t next(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
        static uint64_t hash = 1469598103934665603ull;
        static void digest(const void *p, size_t n) { const unsigned char *b=p; while(n--) hash=(hash ^ *b++)*1099511628211ull; }
        int main(void) {
            uint8_t source[2052];
            for (unsigned test=0; test<100000; ++test) {
                NitroSoundVoice v = {0}; Channel c = {0};
                int32_t L[128]={0}, R[128]={0}, values[128]={0};
                for (unsigned i=0;i<sizeof(source);++i) source[i]=next();
                source[2] %= 89;
                c.v=&v; c.source=source; c.sourceAddr=0x1000; c.format=2;
                c.nSamples=4097; c.endByte=2052; c.loopAt=1+(next()%128);
                c.repeat=next()%3; c.interpolate=next()&1;
                c.volume=next()%128; c.pan=next()%128; c.shift=next()%5;
                c.step=((uint64_t)(next()%5)<<32)|next();
                if (test&1) c.step=(uint32_t)c.step;
                v.running=1; v.phase=next(); v.sampleIndex=0;
                adpcm_init(&v,source); v.adpcmClean=1; v.adpcmSource=c.sourceAddr;
                for (unsigned block=0;block<8;++block) {
                    mix_block(&c,L,R,values,128);
                    digest(L,sizeof(L)); digest(R,sizeof(R)); digest(values,sizeof(values)); digest(&v,sizeof(v));
                }
            }
            printf("%016llx\n",(unsigned long long)hash);
            return 0;
        }
        '@ | Set-Content build/diagnosis/sound_diff.c
        C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && gcc -O2 -fwrapv -I src/compat/nitro/include -DSOURCE=\"sound_before.c\" build/diagnosis/sound_diff.c -o build/diagnosis/sound_before.exe && gcc -O2 -fwrapv -I src/compat/nitro/include -DSOURCE=\"../../src/compat/nitro/src/port_sound_chip.c\" build/diagnosis/sound_diff.c -o build/diagnosis/sound_after.exe && build/diagnosis/sound_before.exe > build/diagnosis/sound-before.hash && build/diagnosis/sound_after.exe > build/diagnosis/sound-after.hash && diff build/diagnosis/sound-before.hash build/diagnosis/sound-after.hash'
    ✓ • 12.44s

## Activity

    $ Get-Content tests/inputs/title-live.txt; Get-Content tests/inputs/goldenrod-idle.txt; Get-Content build/diagnosis/instrumented-build.log -Tail 8; Get-Content src/compat/nitro/src/port_sound_chip.c | Select-String 'static uint32_t total_samples' -Context 0,15; Get-Content C:/Users/terezi/AppData/Roaming/Azahar/config/qt-config.ini | Select-String 'gdb|stub|use_cpu_jit'
    # Title screen at natural pacing (no freezes or dumps): checks the title
    # draws when presentation timing is not held by the script.
    end: 6600
    # Input check (Goldenrod save): touch BAG on the bottom menu, B back out,
    # then hold DOWN to walk. Holds capture the effect of each input.
    every: 10
    0 -
    5000 START
    5010 START
    5020 START
    5030 START
    5040 START
    5050 START
    5100 START
    5110 START
    5120 START
    5130 START
    5140 START
    5150 START
    8000 A
    8020 A
    8040 A
    end: 14000
      257 Wi-Fi SDK functions stood in for (online play is not ported)
    == compiling the DS-side runtime (11 files)
    == compiling the 3DS-side runtime (17 files)
    == link -> /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
       linked /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
       [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
       packaged /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold.3dsx (5544300 bytes)
       build info: pokeheartgold.build-info.txt
    
    > static uint32_t total_samples(unsigned format, uint32_t bytes)
      {
          if (format == 0) return bytes;
          if (format == 1) return bytes / 2u;
          if (format == 2) return bytes >= 4u ? 1u + (bytes - 4u) * 2u : 0u;
          return 0;
      }
    
      static uint32_t loop_sample(unsigned format, uint32_t loopBytes)
      {
          if (format == 0) return loopBytes;
          if (format == 1) return loopBytes / 2u;
          return loopBytes >= 4u ? 1u + (loopBytes - 4u) * 2u : 0u;
      }
    
      static void disable_channel(unsigned base, NitroSoundVoice *voice)
    use_gdbstub\default=false
    use_gdbstub=true
    gdbstub_port\default=true
    gdbstub_port=24689
    use_cpu_jit\default=true
    use_cpu_jit=true
    
    ✓ • 429ms

## Activity

    $ New-Item -ItemType Directory -Force build/diagnosis/stubs | Out-Null
        @'
        #include <stdint.h>
        #define SYSCLOCK_ARM11 268111856u
        uint64_t svcGetSystemTick(void);
        '@ | Set-Content build/diagnosis/stubs/3ds.h
        @'
        #include <stdint.h>
        uint32_t C3D_FrameCounter(int);
        '@ | Set-Content build/diagnosis/stubs/citro3d.h
        @'
        #include <assert.h>
        #include <stdarg.h>
        #include <stdio.h>
        #include "../../src/diagnostics/perf.c"
        static uint64_t tick;
        static uint32_t refresh;
        uint64_t svcGetSystemTick(void) { return tick; }
        uint32_t C3D_FrameCounter(int n) { (void)n; return refresh; }
        void kj_log(const char *fmt, ...) { (void)fmt; }
        int main(void) {
            tick=1; kj_perf_vblank_end();
            tick=101; kj_perf_publish_begin(); kj_perf_present_skipped(1);
            assert(s_game_n==1 && s_game_logic[0]==100 && s_hitches==0);
            tick=301; kj_perf_publish_begin();
            kj_perf_add(KJ_PERF_G2D,90); kj_perf_add(KJ_PERF_G3D,60);
            tick=401; refresh=2; kj_perf_publish_end(2);
            assert(s_game_n==2 && s_game_logic[1]==200);
            assert(s_samples[KJ_PERF_LOGIC][0]==200);
            assert(s_samples[KJ_PERF_G2D][0]==30);
            tick=501; kj_perf_publish_begin();
            kj_perf_add(KJ_PERF_G2D,80); kj_perf_add(KJ_PERF_G3D,50);
            tick=601; refresh=6; kj_perf_publish_end(3);
            assert(s_hitches==1 && s_hitch_g2d[0]==30 && s_hitch_g3d[0]==50);
            puts("PASS: skipped game CPU samples, display hitch count, exclusive hitch timing");
        }
        '@ | Set-Content build/diagnosis/perf_test.c
        C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && gcc -O2 -I build/diagnosis/stubs -I src build/diagnosis/perf_test.c -o build/diagnosis/perf_test.exe && build/diagnosis/perf_test.exe'
        Get-Content build/diagnosis/sound-*.hash -ErrorAction SilentlyContinue; Get-Content build/diagnosis/instrumented-build.log -Tail 4
    PASS: skipped game CPU samples, display hitch count, exclusive hitch timing
    e8bfdd766ba03a03
    e8bfdd766ba03a03
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
       1092 compiled, 0 failed
    == assembling ARM9 .s (150 files)
    ✓ • 2.00s

## Activity

    $ python -c "import importlib.util; print('unicorn', bool(importlib.util.find_spec('unicorn')))"; Get-Content build/diagnosis/instrumented-build.log -Tail 5; Get-Content tools/game/hgss_game.py | Select-Object -Skip 270 -First 35; Get-Content src/compat/nitro/include/nitro_port_gx3d.h -First 30; Get-Content src/compat/nitro/include/nitro_port.h -First 30; git diff --check
    unicorn False
      431 |     return result;
          |            ^~~~~~
    C:/Users/terezi/workdir/kohjoh/build/hgss-sdk/libraries/hgss_sub/sound_seq_player.c:393:9: note: 'result' was declared here
      393 |     int result;
          |         ^~~~~~
            #
            # -fno-ipa-icf: -O2 merges functions that compile to identical bodies,
            # which changes function-pointer identity. This port passes functions
            # around constantly (IRQ table entries, HBlank tasks, SysTask
            # callbacks), so two byte-identical callbacks becoming one is not
            # something the game's code can be assumed to survive. It costs well
            # under 1% of code size.
            flags += ["-O2"]
        return flags
    
    
    # --sanitize: instrument the game and app objects (and the link) for the
    # Phase 4 replay. The vendored archives are built normally; ASan still
    # interposes malloc and checks the game's accesses into them.
    EXTRA_DEFINES: list[str] = []
    SANITIZE = False
    SANITIZE_FLAGS = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
    
    
    def hgss_includes(root: Path, sdk: Path, gen: Path) -> list[str]:
        """Include paths for game sources.
    
        tools/hgss_nitro.py's list (nitro_port + HGSS's flat SDK headers) plus
        the game's own include directory, its library headers, the generated
        naix/msg headers under files/, and the generated overlay-id header.
        """
        nitro_dir = REPO / "src" / "compat" / "nitro"
        return nitro.sdk_includes(root, sdk, nitro_dir, gen) + [
            f"-I{REPO / 'src' / 'compat' / 'pak' / 'include'}",
            # After the system headers: HGSS's lib/include has the game's own
            # <dwc/...> and other library headers, but also Metrowerks
            # stdarg.h/stddef.h/string.h. -idirafter lets the real system headers
            # win while still finding the game's library headers.
            "-idirafter", f"{root}/lib/include",
            f"-I{root}/files",
    /*
     * nitro_port_gx3d.h: the geometry-engine hooks the patched NitroSDK and the
     * compat layer call. kohjoh implements them in src/runtime/kj_g3.c (a new
     * engine: matrices, lighting and tests on the CPU, positions transformed on
     * the PICA). This replaces the vendored header, whose engine (CPU transform,
     * clipping and rasterisation) is not part of kohjoh.
     */
    
    #ifndef NITRO_PORT_GX3D_H_
    #define NITRO_PORT_GX3D_H_
    
    #include <stddef.h>
    #include <stdint.h>
    
    #ifdef __cplusplus
    extern "C" {
    #endif
    
    /* Direct opcode writes (one parameter word per call), and packed GXFIFO
     * words (a command word, then the parameters of its up-to-four commands). */
    void NitroPort_GxDirectWrite(int opcode, uint32_t value);
    void NitroPort_GxFifoWord(uint32_t value);
    void NitroPort_GxFifoWords(const uint32_t *words, size_t count);
    
    /* Bring GXSTAT's stack levels, test bits and RAM counters up to date. */
    void NitroPort_Gx3dRefreshStatus(void);
    
    /* Bring the clip and vector matrix result registers (0x640, 0x680) up to
     * date; the SDK's G3X_GetClipMtx/G3X_GetVectorMtx call it first. */
    void NitroPort_Gx3dSyncResults(void);
    /*
     * nitro_port.h: the 3DS port's side of libnitro.
     *
     * NitroSDK talks to DS hardware. On the 3DS that hardware doesn't exist, so
     * the port provides it in software. This header is the interface between
     * the two: logging, "not implemented yet" reporting, and the hooks the rest
     * of the port (the 3DS main loop, the renderer, the virtual ARM7) plugs into.
     */
    
    #ifndef NITRO_PORT_H_
    #define NITRO_PORT_H_
    
    #include <nitro/types.h>
    #include <stddef.h>
    #include <stdint.h>
    /* kohjoh: the vendored software 2D renderer (nitro_port_render2d.h) is not part of this port. */
    
    #ifdef __cplusplus
    extern "C" {
    #endif
    
    /* ------------------------------------------------------------------ logging */
    
    /* printf-style log line. Goes to stderr on PC; on 3DS, wherever the port's
     * main program sends it (weak, so the program can replace it). */
    void NitroPort_Log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
    
    /* Report that `what` was called but isn't implemented in the port yet.
     * Logged once per name, so a running game produces a short to-do list
     * rather than a flood. Returns how many distinct names have been reported. */
    README.md:5: trailing whitespace.
    +not tested or known to be stable yet; runs near full-speed on real hardware, excluding geometry-heavy map areas && the intro ho-oh/lugia
    README.md:7: trailing whitespace.
    +this repository does not contain any unlicensed game data.
    ✗ (1) • 437ms

## Activity

    $ rg -o '\b[A-Z][A-Z0-9_]{2,}\b' src/runtime/kj_g3.c | Sort-Object -Unique; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 40 -First 40; rg -n 'NitroPort_|reg_|FX_|GX_' src/runtime/kj_g3.c | Select-Object -Last 40; Get-Content build/diagnosis/instrumented-build.log -Tail 5
    ARM11
    BEGIN
    BLOCK
    BOX
    COLOR
    CPI
    CPU
    DL_BYTES_MAX
    DL_EV_BLOCK
    DL_MAX_EVENTS
    DL_MIN_WORDS
    DL_NEVER_RECHECK_MASK
    DL_READS_
    DL_READS_COLOR
    DL_READS_COORD
    DL_READS_TEXCOORD
    DL_READS_TEXM
    DL_SLOTS
    DLC_BEGIN
    DLC_BEGIN_FIRST
    DLC_CHANGED
    DLC_COUNT
    DLC_HIT
    DLC_KEY
    DLC_NEVER
    DLC_NEW
    DLC_SHORT
    DLC_SMALL
    DLC_SPLIT
    DMA
    END
    EXPAND5
    FIFO
    FX_ONE
    GBATEK
    GPL
    GPU
    GXFIFO
    GXSTAT
    HGSS_PROFILE_OFF
    KJ_DL_CACHE
    KJ_DL_NO_CACHE
    KJ_DL_VERIFY
    KJ_G3_BATCH_SHADOW
    KJ_G3_BATCH_TRANSLUCENT
    KJ_G3_MAX_BATCHES
    KJ_G3_MAX_INDICES
    KJ_G3_MAX_MATRICES
    KJ_G3_MAX_VERTS
    KJ_G3_PALETTE
    KJ_G3_PROFILE
    KJ_G3_ZERO_TRACE
    MISMATCH
    MTX_LOAD
    MTX_SCALE
    MTX_TRANS
    MULT
    NNS
    NORMAL
    NORMAL_CACHE
    NULL
    O3DS
    OPT
    PICA
    POP
    POS
    POSVEC_STACK
    PRNG
    PUSH
    RESTORE
    SCALE
    STATE
    STORE
    SWAP_BUFFERS
    TEXCOORD
    TEXIMAGE_PARAM
    TRANS
    UINT32_MAX
    VEC
    VTX_
    VTX_10
    VTX_DIFF
    
    /* ------------------------------------------------------------- matrices */
    
    typedef struct mtx {
        int32_t m[4][4]; /* 20.12, row-vector convention: v' = v * M */
    } mtx;
    
    #define POSVEC_STACK 32
    #define FX_ONE 4096
    
    static mtx s_proj, s_pos, s_vec, s_tex;
    static mtx s_proj_stack[1], s_tex_stack[1];
    static mtx s_pos_stack[POSVEC_STACK], s_vec_stack[POSVEC_STACK];
    static int s_proj_sp, s_tex_sp, s_posvec_sp;
    static int s_mtx_mode;
    static bool s_stack_error;
    static mtx s_clip;
    static bool s_clip_dirty = true;
    static uint32_t s_clip_version = 1; /* bumps on every clip change */
    
    static void mtx_identity(mtx *m)
    {
        memset(m, 0, sizeof(*m));
        m->m[0][0] = m->m[1][1] = m->m[2][2] = m->m[3][3] = FX_ONE;
    }
    
    /* out = a * b (row-vector convention: apply a, then b). */
    static void mtx_mul(mtx *out, const mtx *a, const mtx *b)
    {
        mtx r;
    
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                int64_t acc = 0;
                for (int k = 0; k < 4; ++k) {
                    acc += (int64_t)a->m[i][k] * b->m[k][j];
                }
                r.m[i][j] = (int32_t)(acc >> 12);
            }
        }
    40:extern unsigned char NitroPort_IoReg[];
    49:#define FX_ONE 4096
    64:    m->m[0][0] = m->m[1][1] = m->m[2][2] = m->m[3][3] = FX_ONE;
    853:    memcpy(NitroPort_IoReg + offset, &value, 4);
    859:    memcpy(&v, NitroPort_IoReg + offset, 4);
    863:void NitroPort_Gx3dSyncResults(void)
    875:void NitroPort_Gx3dRefreshStatus(void)
    894:void NitroPort_Gx3dClearStackError(void)
    898:    NitroPort_Gx3dRefreshStatus();
    901:int NitroPort_Gx3dEarlierSceneUnread(void)
    996:        memcpy(NitroPort_IoReg + 0x630 + 2 * j, &v, 2);
    1019:        memcpy(&d3, NitroPort_IoReg + 0x60, 2);
    1022:    f->alpha_test_ref = NitroPort_IoReg[0x340] & 31u;
    1026:    memcpy(&f->fog_offset, NitroPort_IoReg + 0x35C, 2);
    1027:    memcpy(f->fog_table, NitroPort_IoReg + 0x360, 32);
    1028:    memcpy(f->edge_color, NitroPort_IoReg + 0x330, 16);
    1029:    memcpy(f->toon_table, NitroPort_IoReg + 0x380, 64);
    1087:            NitroPort_Log("[g3prof] op %02x calls %lu ticks %llu (%llu/call)", i,
    1120:            NitroPort_Log("[g3zero] op %02x p0 %08lx mode %d sp %d -> pos %d proj %d (swap %lu)", op,
    1343:void NitroPort_GxDirectWrite(int opcode, uint32_t value)
    1397:void NitroPort_GxFifoWord(uint32_t value)
    2178:            NitroPort_Log("[dlverify] MISMATCH %s at %lu: list %p (%lu words) nv %lu/%lu ni %lu/%lu "
    2202:        NitroPort_GxFifoWord(words[i++]);
    2213:                NitroPort_GxFifoWord(words[i++]);
    2386:            NitroPort_Log("[dlmiss] list %p lit %d/%d texgen %d/%d key word %u: %08lx vs %08lx",
    2503:void NitroPort_GxFifoWords(const uint32_t *words, size_t count)
    2531:    NitroPort_Log("[g3sample] random 1/64 FIFO calls: samples %lu total_ticks %llu decode_ticks %llu replay_ticks %llu; remainder includes lookup/recording and probe overhead",
    2541:    NitroPort_Log("[dlwhy] batch-in-block %lu shared-verts %lu straddle/drops %lu lit+matrix %lu "
    2546:    NitroPort_Log("[dlcov] bypass: small %lu split %lu open-begin %lu (begin first %lu)",
    2549:    NitroPort_Log("[dlcov] words: short %lu new %lu changed %lu never %lu hit %lu keymiss %lu",
      ARM7 CC ds_sound_tables.c
    built ../../../build/instrumented/sdk/libnitro.a
      1 NitroSystem header inline functions
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
    ✓ • 238ms

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && HGSS_PROFILE=on python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/instrumented -j 32 > build/diagnosis/instrumented-build.log 2>&1'
    ✓ • 1m 44s

## Activity

    $ @'
        from pathlib import Path
        p=Path('src/runtime/kj_g3.c'); s=p.read_text(); s=s.replace('static void fifo_next_command(void)', 'static void fifo_command(unsigned op, const uint32_t *p);\n\nstatic void fifo_next_command(void)',1); s=s.replace('execute(s_fifo_op, s_fifo_params);','fifo_command(s_fifo_op, s_fifo_params);'); pos=s.index('static void build_param_table(void)'); s=s[:pos]+'''/* A command completed across FIFO calls still has exactly the same vertex
         * state as a bulk command. Reuse its fast path instead of forcing every
         * split vertex through submit_vertex and the general dispatcher. */
        static void fifo_command(unsigned op, const uint32_t *p)
        {
            hot_command(op, p);
        }
        
        '''+s[pos:]; p.write_text(s)
        '@ | python
        @'
        #include <stdint.h>
        typedef uint64_t u64;
        typedef uint32_t u32;
        '@ | Set-Content build/diagnosis/stubs/nitro.h
        @'
        #include <stddef.h>
        #include <stdint.h>
        void NitroPort_Log(const char *, ...);
        '@ | Set-Content build/diagnosis/stubs/nitro_port.h
        Copy-Item src/compat/nitro/include/nitro_port_gx3d.h build/diagnosis/stubs/nitro_port_gx3d.h
        @'
        #include <stdio.h>
        #include <stdarg.h>
        #include <stdlib.h>
        #include <string.h>
        #include SOURCE
        unsigned char NitroPort_IoReg[0x1000];
        void NitroPort_Log(const char *fmt,...) { (void)fmt; }
        static uint32_t rng=76543;
        static uint32_t next(void) { rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng; }
        static uint64_t hash=1469598103934665603ull;
        static void digest(const void *p,size_t n) { const unsigned char *s=p; while(n--)hash=(hash^*s++)*1099511628211ull; }
        int main(void) {
            kj_g3_storage st;
            for(int i=0;i<2;i++) { st.verts[i]=calloc(KJ_G3_MAX_VERTS,sizeof(kj_g3_vertex)); st.indices[i]=calloc(KJ_G3_MAX_INDICES,2); }
            kj_g3_init(&st);
            uint32_t lists[16][128]; size_t sizes[16];
            for(int l=0;l<16;l++) {
                size_t n=0; lists[l][n++]=0x40; lists[l][n++]=l%4;
                for(int k=0;k<10;k++) { lists[l][n++]=0x23232220; lists[l][n++]=next()&32767; lists[l][n++]=next(); for(int j=0;j<4;j++) lists[l][n++]=next(); }
                lists[l][n++]=0x41; sizes[l]=n;
            }
            for(int f=0;f<2000;f++) {
                NitroPort_GxDirectWrite(0x29,0x001f00c0u);
                for(int l=0;l<16;l++) {
                    if(f%7==0) lists[l][4]^=0x1234;
                    if(l%3==0) { size_t n=0; while(n<sizes[l]) { size_t c=1+next()%11; if(c>sizes[l]-n)c=sizes[l]-n; NitroPort_GxFifoWords(lists[l]+n,c); n+=c; } }
                    else NitroPort_GxFifoWords(lists[l],sizes[l]);
                }
                NitroPort_GxDirectWrite(0x50,0);
                const kj_g3_frame *g=kj_g3_take_frame();
                digest(g->verts,g->vert_count*sizeof(*g->verts)); digest(g->indices,g->index_count*2);
                digest(g->matrices,g->matrix_count*sizeof(*g->matrices)); digest(g->batches,g->batch_count*sizeof(*g->batches));
                digest(NitroPort_IoReg,sizeof(NitroPort_IoReg));
            }
            printf("%016llx\n",(unsigned long long)hash);
        }
        '@ | Set-Content build/diagnosis/g3_diff.c
        C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && gcc -O2 -fwrapv -fno-strict-aliasing -DHGSS_PROFILE_OFF -I build/diagnosis/stubs -I src/runtime -DSOURCE=\"g3_before.c\" build/diagnosis/g3_diff.c -o build/diagnosis/g3_before.exe && gcc -O2 -fwrapv -fno-strict-aliasing -DHGSS_PROFILE_OFF -I build/diagnosis/stubs -I src/runtime -DSOURCE=\"../../src/runtime/kj_g3.c\" build/diagnosis/g3_diff.c -o build/diagnosis/g3_after.exe && build/diagnosis/g3_before.exe > build/diagnosis/g3-before.hash && build/diagnosis/g3_after.exe > build/diagnosis/g3-after.hash && diff build/diagnosis/g3-before.hash build/diagnosis/g3-after.hash'
    ✓ • 4.07s

## Activity

    $ python -m pip install --quiet unicorn pyelftools; Get-Content build/diagnosis/instrumented-build.log -Tail 8
    error: externally-managed-environment
    
    × This environment is externally managed
    ╰─> To install Python packages system-wide, try 'pacman -S
        python-xyz', where xyz is the package you are trying to
        install.
    
        If you wish to install a non-MSYS2-packaged Python package,
        create a virtual environment using 'python -m venv path/to/venv'.
        Then use path/to/venv/bin/python and path/to/venv/bin/pip.
    
    note: If you believe this is a mistake, please contact your Python installation or OS distribution provider. You can override this, at the risk of breaking your Python installation or OS, by passing --break-system-packages.
    hint: See PEP 668 for the detailed specification.
    == build id: 8a92e93-dirty-prof
    + /usr/bin/python /c/Users/terezi/workdir/kohjoh/tools/game/prepare_game_headers.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --ids-output /c/Users/terezi/workdir/kohjoh/build/instrumented/gen/overlay_ids.ld --header /c/Users/terezi/workdir/kohjoh/build/instrumented/gen/overlay_ids.h
    ✓ • 11.87s

## Activity

    $ @'
        from pathlib import Path
        p=Path('build/diagnosis/g3_diff.c'); s=p.read_text().replace('NitroPort_GxDirectWrite(0x29,0x001f00c0u);','''NitroPort_GxDirectWrite(0x29,0x001f00c0u | ((f/64)&1 ? 15u : 0u));
                NitroPort_GxDirectWrite(0x2a,(uint32_t)((f/128)%4)<<30);
                NitroPort_GxDirectWrite(0x30,0x12345678);
                NitroPort_GxDirectWrite(0x31,(f/256)&1 ? 0x9234abcd : 0x1234abcd);'''); p.write_text(s)
        '@ | python
        C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && gcc -O2 -fwrapv -fno-strict-aliasing -DHGSS_PROFILE_OFF -I build/diagnosis/stubs -I src/runtime -DSOURCE=\"g3_before.c\" build/diagnosis/g3_diff.c -o build/diagnosis/g3_before.exe && gcc -O2 -fwrapv -fno-strict-aliasing -DHGSS_PROFILE_OFF -I build/diagnosis/stubs -I src/runtime -DSOURCE=\"../../src/runtime/kj_g3.c\" build/diagnosis/g3_diff.c -o build/diagnosis/g3_after.exe && build/diagnosis/g3_before.exe > build/diagnosis/g3-before.hash && build/diagnosis/g3_after.exe > build/diagnosis/g3-after.hash && diff build/diagnosis/g3-before.hash build/diagnosis/g3-after.hash'
        Get-Content build/diagnosis/*-*.hash
    2aaf6f99e63dabab
    2aaf6f99e63dabab
    e8bfdd766ba03a03
    e8bfdd766ba03a03
    ✓ • 5.17s

## Activity

    $ Get-Command python,py -All -ErrorAction SilentlyContinue | Select-Object Source; Get-Content build/diagnosis/instrumented-build.log -Tail 4; Get-Content tools/azahar/run.ps1 | Select-Object -Skip 65 -First 25; Get-ChildItem build/normal/pokeheartgold.3dsx,build/instrumented/pokeheartgold.3dsx | Select-Object FullName,LastWriteTime
    
    Source
    ------
    c:\devkitPro\msys2\usr\bin\python.exe
    C:\Program Files\Python315\python.exe
    C:\Users\terezi\AppData\Local\Microsoft\WindowsApps\python.exe
    C:\Windows\py.exe
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
       1092 compiled, 0 failed
    == assembling ARM9 .s (150 files)
    }
    $p = Start-Process -FilePath "C:\Program Files\Azahar\azahar.exe" -ArgumentList "`"$dsx`"" -PassThru
    $start = Get-Date
    # A fixed, large window so the two screens are readable in the shots.
    for ($i = 0; $i -lt 50; $i++) {
      Start-Sleep -Milliseconds 100
      $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
      if ($proc -and $proc.MainWindowHandle -ne 0) { [N.W]::MoveWindow($proc.MainWindowHandle, 0, 0, 840, 1100, $true) | Out-Null; break }
    }
    
    function Shot([string]$name) {
      $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
      if ($null -eq $proc) { Write-Output "azahar exited before $name"; return }
      $h = $proc.MainWindowHandle
      $r = New-Object N.W+RECT
      [N.W]::GetWindowRect($h, [ref]$r) | Out-Null
      $w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
      if ($w -le 0 -or $hh -le 0) { Write-Output "no window for $name"; return }
      $bmp = New-Object System.Drawing.Bitmap $w, $hh
      $g = [System.Drawing.Graphics]::FromImage($bmp)
      $dc = $g.GetHdc()
      [N.W]::PrintWindow($h, $dc, 2) | Out-Null
      $g.ReleaseHdc($dc); $g.Dispose()
      $file = Join-Path $out "$name.png"
      $bmp.Save($file, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
    
    
    
    ✓ • 392ms

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && for variant in before after; do src=build/diagnosis/sound_before.c; if test "$variant" = after; then src=src/compat/nitro/src/port_sound_chip.c; fi; /opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc -O2 -marm -march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp -DHGSS_PROFILE_OFF -ffunction-sections -fdata-sections -I src/compat/nitro/include -nostartfiles -Wl,--gc-sections,-e,NitroSound_Mix -Wl,-Ttext,0x100000 "$src" -o build/diagnosis/sound-$variant.elf; done'
    ✓ • 1.83s

## Activity

    $ & 'C:/Program Files/Python315/python.exe' -m venv build/diagnosis/venv; & build/diagnosis/venv/Scripts/python.exe -m pip install --quiet unicorn pyelftools
    
    [notice] A new release of pip is available: 26.2.2 -> 26.3
    [notice] To update, run: C:\Users\terezi\workdir\kohjoh\build\diagnosis\venv\Scripts\python.exe -m pip install --upgrade pip
    ✓ • 15.48s

## Activity

    $ Get-Content build/diagnosis/instrumented-build.log -Tail 5; Get-Content build/diagnosis/venv/pyvenv.cfg; Get-Content build/diagnosis/sound_diff.c | Select-Object -Last 8; Get-Content docs/testing.md | Select-Object -Skip 100 -First 65
    == overlay table
      129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
    == duplicate symbols
       0 symbols defined in more than one object (0 objects weakened)
    == Wi-Fi stand-ins
    home = C:\Program Files\Python315
    include-system-site-packages = false
    version = 3.15.2
    executable = C:\Program Files\Python315\python.exe
    command = C:\Program Files\Python315\python.exe -m venv C:\Users\terezi\workdir\kohjoh\build\diagnosis\venv
            for (unsigned block=0;block<8;++block) {
                mix_block(&c,L,R,values,128);
                digest(L,sizeof(L)); digest(R,sizeof(R)); digest(values,sizeof(values)); digest(&v,sizeof(v));
            }
        }
        printf("%016llx\n",(unsigned long long)hash);
        return 0;
    }
      words, H-blank cost, display-list cache). `[ds] per game frame` divides by
      the H-blank walk count (one walk per game frame) and prints it with the
      picture count, so skipped presents no longer inflate it. `[perf] g2d` is
      exclusive: the g3d time spent inside it is subtracted (before `6191966`
      g2d included g3d). `[dlcov]` (with `KJ_DL_VERIFY`) splits FIFO words into
      bypassed (small, split mid-command, open BEGIN) and new/changed/never/hit/
      keymiss; `[dlwhy]` counts uncacheable reasons and arena flushes.
      `[gpu] ... char KiB compared` is the retained layers' character check.
      `[gputime]`: GSP perf-log microseconds per picture (P3D command lists, PPF
      display transfers, PSC fills; 0 in Azahar, which does not implement the
      log), `C3D_GetDrawingTime` average/max (the latest completed queue, so not
      tied to one picture) and the peak command-buffer use.
      `[sndprof]` (every ~9.4 s of audio, on the audio thread): cumulative mixer
      ticks, frames and active channels; cycles per channel-sample is
      Δmix_ticks / (Δactive_ch / Δmix_calls) / Δmix_frames. Needs
      `dspfirm.cdc` in Azahar's `sdmc/3ds/`, or NDSP fails and the mixer never runs.
    - **PC sampling**: with Azahar's GDB stub enabled (`use_gdbstub=true`, port
      24689), `python tools/azahar/gdbprof.py build/kj/pokeheartgold-3ds.elf
      --samples 1500 --interval 0.02 [--callers fn1,fn2]` prints a function
      histogram (and callers via LR). This found every hotspot so far.
      - `--pcs fn1,fn2`: per-PC histogram inside those functions (map with
        `arm-none-eabi-objdump -d`).
      - `--stack fn:offset`: histogram of the word at SP+offset while sampled in
        `fn`, i.e. a return address further up the stack. Example: the caller of
        `GetMonData` from `_MonEncryptSegment` is `--stack _MonEncryptSegment:36`
        (16 bytes pushed by it, 8 locals + 12 to `lr` in `GetMonData`).
      - Partial results are printed if Azahar closes mid-run. Give the harness a
        late enough last shot, and never start a second harness while one runs:
        each `run.ps1` kills every Azahar when it finishes.
      - Samples are taken when Azahar's JIT stops at a block boundary, so tight
        loops are over-represented. Use samples to find *where*; use `[perf]`
        stage times (instruction counts) to measure *how much*.
    - **Code size**: Azahar does not model the 16 KiB I-cache. After editing a
      hot loop, check `arm-none-eabi-nm -S --size-sort` on the `.elf`
      (`NitroPort_GxFifoWords` must stay well under 16 KiB).
    - **Probes**: `HGSS_PROFILE=on` compiles the HGSS H-blank/2D tick probes in
      (several hundred `svcGetSystemTick` calls a frame). Normal builds leave
      them out; never make a timing claim from a probe build.
    - **Present skipping is disabled while a script runs** (`kj_script_active`).
      To see pacing as the player does (title, field), run without `-Script`.
    - **Calibration**: the `[calib]` log line shows Azahar's ticks per instruction
      (measured 1.0). Treat Azahar milliseconds as instruction counts.
    - `--define KJ_G3_PROFILE` times each geometry opcode. Each probe costs ~390
      ticks; subtract it, and prefer PC sampling.
    ✓ • 192ms

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && HGSS_PROFILE=on python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/instrumented -j 32 > build/diagnosis/instrumented-build.log 2>&1'
    ✓ • 1m 46s

## Activity

    $ @'
        from pathlib import Path
        s=Path('build/diagnosis/sound_diff.c').read_text().replace('100000','100').replace('static uint64_t hash','volatile uint64_t result;\nstatic uint64_t hash').replace('    printf("%016llx\\n",(unsigned long long)hash);','    result=hash;'); Path('build/diagnosis/sound_arm.c').write_text(s)
        s=Path('build/diagnosis/g3_diff.c').read_text().replace('#include SOURCE','''static unsigned char heap[24*1024*1024];
        static size_t used;
        void *malloc(size_t n) { void *p=heap+used; used=(used+n+15)&~15u; return p; }
        void *calloc(size_t n,size_t z) { void *p=malloc(n*z); memset(p,0,n*z); return p; }
        #include SOURCE''').replace('f<2000','f<128').replace('static uint64_t hash','volatile uint64_t result;\nstatic uint64_t hash').replace('    printf("%016llx\\n",(unsigned long long)hash);','    result=hash;'); Path('build/diagnosis/g3_arm.c').write_text(s)
        '@ | python
        C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && for task in sound g3; do for variant in before after; do src=$task"_before.c"; if test "$variant" = after; then if test "$task" = sound; then src=../../src/compat/nitro/src/port_sound_chip.c; else src=../../src/runtime/kj_g3.c; fi; fi; /opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc -O2 -marm -march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp -fno-strict-aliasing -fwrapv -DHGSS_PROFILE_OFF -ffunction-sections -fdata-sections -I src/compat/nitro/include -I build/diagnosis/stubs -I src/runtime -DSOURCE=\"$src\" -nostartfiles -specs=nosys.specs -Wl,--gc-sections,-e,main -Wl,-Ttext,0x100000 build/diagnosis/"$task"_arm.c -o build/diagnosis/$task-$variant.elf; done; done'
    In file included from build/diagnosis/g3_before.c:26,
                     from build/diagnosis/g3_arm.c:9:
    src/compat/nitro/include/nitro_port.h:13:10: fatal error: nitro/types.h: No such file or directory
       13 | #include <nitro/types.h>
          |          ^~~~~~~~~~~~~~~
    compilation terminated.
    In file included from build/diagnosis/../../src/runtime/kj_g3.c:26,
                     from build/diagnosis/g3_arm.c:9:
    src/compat/nitro/include/nitro_port.h:13:10: fatal error: nitro/types.h: No such file or directory
       13 | #include <nitro/types.h>
          |          ^~~~~~~~~~~~~~~
    compilation terminated.
    ✗ (1) • 1.61s

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && for variant in before after; do src=g3_before.c; if test "$variant" = after; then src=../../src/runtime/kj_g3.c; fi; /opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc -O2 -marm -march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp -fno-strict-aliasing -fwrapv -DHGSS_PROFILE_OFF -ffunction-sections -fdata-sections -I build/diagnosis/stubs -I src/runtime -DSOURCE=\"$src\" -nostartfiles -specs=nosys.specs -Wl,--gc-sections,-e,main -Wl,-Ttext,0x100000 build/diagnosis/g3_arm.c -o build/diagnosis/g3-$variant.elf; done'
        @'
        from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_BLOCK
        from unicorn.arm_const import *
        from elftools.elf.elffile import ELFFile
        import struct,sys,json
        for filename in sys.argv[1:]:
            with open(filename,'rb') as f:
                elf=ELFFile(f); u=Uc(UC_ARCH_ARM,UC_MODE_ARM)
                segs=[(s['p_vaddr'],s['p_memsz'],s.data()) for s in elf.iter_segments() if s['p_type']=='PT_LOAD']
                lo=min(a for a,n,d in segs)&~4095; hi=max(a+n for a,n,d in segs)
                u.mem_map(lo, (hi-lo+4095)&~4095)
                for a,n,d in segs:u.mem_write(a,d)
                syms={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
            u.mem_map(0x8000000,0x100000); u.reg_write(UC_ARM_REG_SP,0x80ffff0)
            u.mem_map(0x9000000,4096); u.reg_write(UC_ARM_REG_LR,0x9000000)
            u.reg_write(UC_ARM_REG_C1_C0_2,0xf<<20); u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
            counts=[0]
            def block(uc,addr,size,data): counts[0]+=size//4
            u.hook_add(UC_HOOK_BLOCK,block)
            u.emu_start(syms['main'],0x9000000)
            result=struct.unpack('<Q',u.mem_read(syms['result'],8))[0]
            print(json.dumps(dict(file=filename,instructions=counts[0],hash=f'{result:016x}')),flush=True)
        '@ | Set-Content build/diagnosis/arm_bench.py
        & build/diagnosis/venv/Scripts/python.exe build/diagnosis/arm_bench.py build/diagnosis/sound-before.elf build/diagnosis/sound-after.elf build/diagnosis/g3-before.elf build/diagnosis/g3-after.elf > build/diagnosis/arm-results.jsonl
    ✓ • 7.08s

## Activity

    $ Get-Content build/diagnosis/arm-results.jsonl; Get-Content build/diagnosis/instrumented-build.log -Tail 8; rg -n 'error:|vendored library build failed' build/diagnosis/instrumented-build.log; C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe -S --size-sort build/instrumented/pokeheartgold-3ds.elf | Select-String 'fifo_command|decode_words_impl|fifo_words_impl|mix_block|adpcm|dl_replay'; git diff --numstat src/runtime/kj_g3.c
    {"file": "build/diagnosis/sound-before.elf", "instructions": 22237237, "hash": "cffe8aeaa99e5708"}
    {"file": "build/diagnosis/sound-after.elf", "instructions": 21375151, "hash": "cffe8aeaa99e5708"}
    {"file": "build/diagnosis/g3-before.elf", "instructions": 22935870, "hash": "f20273876a89a46b"}
    {"file": "build/diagnosis/g3-after.elf", "instructions": 21758088, "hash": "f20273876a89a46b"}
      257 Wi-Fi SDK functions stood in for (online play is not ported)
    == compiling the DS-side runtime (11 files)
    == compiling the 3DS-side runtime (17 files)
    == link -> /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
       linked /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
       [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
       packaged /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold.3dsx (5544780 bytes)
       build info: pokeheartgold.build-info.txt
    
    00581bbc 00000090 t adpcm_at.isra.0
    001043d0 000001b4 t fifo_command.constprop.0
    005f0cbc 00000590 r sAdpcmDelta
    0058201c 0000117c t mix_block
    00105100 000011d0 t fifo_words_impl
    160    12    src/runtime/kj_g3.c
    
    ✓ • 473ms

## Activity

    $ pwsh -NoProfile -File tools/azahar/run.ps1 -Artifact build/instrumented/pokeheartgold.3dsx -Shots 15,35 -Tag diagnosis-smoke
    no window for t015
    shot build\azahar\diagnosis-smoke\t035.png
    log copied
    ✓ • 40.88s

## Assistant

The changes pass the first differential checks: 100,000 randomized audio programs and 2,000 synthetic geometry frames produce identical output and state hashes before and after. The geometry test includes split FIFO calls and cache hits.

I also found that commands completed across FIFO calls bypassed the existing vertex fast path. They now use it. ARM instruction emulation shows about 5.1% fewer instructions in the geometry fixture and 3.9% fewer in the audio fixture. Those are synthetic instruction reductions; hardware timing remains unmeasured. The rebuilt instrumented game is now in an Azahar smoke test.

## Activity

    $ Get-Content build/diagnosis/normal-build.log -Tail 4; Get-Content build/azahar/diagnosis-smoke/heartgold.log -Tail 15 -ErrorAction SilentlyContinue; Get-ChildItem build/azahar/diagnosis-smoke; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1900 -First 52; Get-Content src/compat/nitro/src/port_os.c | Select-Object -Skip 300 -First 40
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/normal/gen/overlay_ids.h (unchanged)
    done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
    
    --- new run ---
         0.032 [app] kohjoh 8a92e93-dirty-prof (heartgold), homebrew (SD pack), Old 3DS
         0.549 [gpu] renderer ready (linear free 26393 KiB, VRAM free 4794 KiB)
         0.620 [audio] NDSP 32768 Hz stereo, 4 x 512 frames
         0.620 [save] raw save file sdmc:/3ds/kohjoh/saves/heartgold.sav
         0.635 [calib] 1M-iteration loop: 4108987 ticks (4.11 ticks/iteration)
         0.635 [app] starting the game
         0.636 [kj] starting heartgold
         0.948 [nitro] overlay OVY_0 loaded
         0.948 [nitro] overlay OVY_38 loaded
         0.948 [nitro] not implemented: DWC_Init
         0.948 [nitro] overlay intro_title loaded
        10.006 [sndprof] render_calls 600 render_frames 307200 stretches 2399 sndticks 1799 mix_ticks 223486762 tick_ticks 11468948 mix_calls 2399 mix_frames 307200 active_ch 17959 pcm8_ch 0 pcm16_ch 0 adpcm_ch 17959 psg_ch 0 cap_inst 0 dep_ch 0
    
        Directory: C:\Users\terezi\workdir\kohjoh\build\azahar\diagnosis-smoke
    
    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    -a---          11/13/2026  2:20 PM          10622 azahar_log.txt
    -a---          11/13/2026  2:20 PM            861 heartgold.log
    -a---          11/13/2026  2:20 PM         120283 t035.png
            }
        }
        return reads;
    }
    
    /* The unused tail is not part of the key. Most map lists consume neither
     * texgen matrices nor shininess; clearing/comparing those bytes on every
     * cache lookup needlessly streams through the ARM11's small data cache.
     * All bytes in the selected prefix are initialized, including padding. */
    static size_t dl_key_size(bool lit, bool texgen, uint32_t reads)
    {
        if (lit) {
            return s_use_shininess ? sizeof(dl_key) : offsetof(dl_key, shininess);
        }
        return texgen || (reads & DL_READS_TEXM) ? offsetof(dl_key, vec)
                                              : offsetof(dl_key, texm);
    }
    
    static void dl_make_key(dl_key *k, bool lit, bool texgen, uint32_t reads)
    {
        memset(k, 0, dl_key_size(lit, texgen, reads));
        k->attr_pending = s_attr_pending;
        k->attr = s_attr;
        k->tex_param = s_tex_param;
        k->tex_pltt = s_tex_pltt;
        k->viewport = s_viewport;
        if (reads & DL_READS_COLOR) {
            k->color = s_color;
        }
        if (reads & DL_READS_COORD) {
            k->coord01 = (uint16_t)s_coord[0] | ((uint32_t)(uint16_t)s_coord[1] << 16);
            k->coord2 = (uint16_t)s_coord[2];
        }
        if (reads & DL_READS_TEXCOORD) {
            k->texcoord[0] = s_texcoord[0];
            k->texcoord[1] = s_texcoord[1];
            k->texcoord_raw[0] = s_texcoord_raw[0];
            k->texcoord_raw[1] = s_texcoord_raw[1];
        }
        k->mtx_mode = (uint32_t)s_mtx_mode;
        if (texgen || (reads & DL_READS_TEXM)) {
            memcpy(k->texm, s_tex.m, sizeof(k->texm));
        }
        if (lit) {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    k->vec[i][j] = s_vec.m[i][j];
                }
            }
            memcpy(k->light_dir, s_light_dir, sizeof(k->light_dir));
            memcpy(k->light_half, s_light_half, sizeof(k->light_half));
            for (int i = 0; i < 4; ++i) {
    {
        *keep = sHBlankKeepTicks;
        *dma = sHBlankDmaTicks;
        *irq = sHBlankIrqTicks;
    }
    #if defined(HGSS_A0_DIAG) && !defined(HGSS_PROFILE_OFF)
    /* P1: the parts outside the keep/dma/irq split: the per-walk refresh, the
     * thread-switch step, and the line compare scan. */
    void NitroPort_HBlankSplit2(u64 *refresh, u64 *sw, u64 *cmp)
    {
        if (refresh != NULL) *refresh = sHBlankRefreshTicks;
        if (sw != NULL) *sw = sHBlankSwitchTicks;
        if (cmp != NULL) *cmp = sHBlankCmpTicks;
    }
    #endif
    #ifdef HGSS_A0_DIAG
    /* P6: the walks where the fused compare and the old scan were compared,
     * and the ones where they disagreed (must be none). */
    void NitroPort_HBlankScanCheck(u64 *walks, u64 *mismatch)
    {
        if (walks != NULL) *walks = sHBlankScans;
        if (mismatch != NULL) *mismatch = sHBlankScanMismatch;
    }
    #endif
    
    /* The last walk's interrupt handlers, by IRQ bit (NitroPort_HBlankLines). */
    void NitroPort_HBlankIrqByBit(NitroPortIrqTime *out)
    {
        int bit;
    
        for (bit = 0; bit < NITRO_PORT_IRQ_BITS; bit++) {
            out[bit].ticks = sHBlankIrqBitTicks[bit];
            out[bit].calls = sHBlankIrqBitCalls[bit];
        }
    }
    
    /* The V-blanks so far (StartVBlank), and the one whose lines were last
     * worked out (NitroPort_HBlankLinesOnce). */
    static u32 sVBlanks, sLinesVBlank = 0xFFFFFFFFu;
    
    
    ✓ • 520ms

## Activity

    $ Get-Content tests/inputs/hg-continue-field.txt; Get-ChildItem build -Recurse -Filter '*sav' | Select-Object -First 8 FullName; Get-Content build/diagnosis/normal-build.log -Tail 4; git diff --check -- src tools docs/testing.md docs/status.md docs/worklog/2026-11-13.md
    # HeartGold: cold boot with an existing save -> title -> main menu ->
    # CONTINUE -> Goldenrod field at rest. Frame-indexed from power-on.
    #
    # Calibrated 2026-11-09 (Azahar, Old-3DS mode, save: player AAR, 2 badges):
    # the title is up by ~4200, START opens the main menu, A on the first entry
    # (CONTINUE) loads the field by ~9000. The presses are repeated over short
    # windows because Azahar's frame pacing jitters; they stop before the field
    # settles so no menu opens there.
    every: 10
    0 -
    5000 START
    5010 START
    5020 START
    5030 START
    5040 START
    5050 START
    5100 START
    5110 START
    5120 START
    5130 START
    5140 START
    5150 START
    8000 A
    8020 A
    8040 A
    end: 16000
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
       1092 compiled, 0 failed
    == assembling ARM9 .s (150 files)
    ✓ • 6.50s

## Activity

    Viewed image C:\Users\terezi\workdir\kohjoh\build\azahar\diagnosis-smoke\t035.png

## Activity

    $ New-Item -ItemType Directory -Force tests/native | Out-Null
        Copy-Item build/diagnosis/sound_diff.c tests/native/sound_diff.c
        Copy-Item build/diagnosis/g3_diff.c tests/native/g3_diff.c
        Copy-Item build/diagnosis/perf_test.c tests/native/perf_test.c
        @'
        """Host differential checks against a git revision; no game assets required.
        Run: python tests/native/check_hotpaths.py [--reference HEAD] [--cc gcc]
        """
        import argparse
        from pathlib import Path
        import shutil
        import subprocess
        import tempfile
        
        root = Path(__file__).resolve().parents[2]
        a = argparse.ArgumentParser()
        a.add_argument('--reference', default='HEAD')
        a.add_argument('--cc', default=shutil.which('gcc') or 'C:/devkitPro/msys2/usr/bin/gcc.exe')
        a = a.parse_args()
        
        def run(args):
            return subprocess.check_output([str(x) for x in args], cwd=root, text=True).strip()
        
        with tempfile.TemporaryDirectory(prefix='kohjoh-test-') as tmp:
            tmp = Path(tmp)
            (tmp/'3ds.h').write_text('#include <stdint.h>\n#define SYSCLOCK_ARM11 268111856u\nuint64_t svcGetSystemTick(void);\n')
            (tmp/'citro3d.h').write_text('#include <stdint.h>\nuint32_t C3D_FrameCounter(int);\n')
            (tmp/'nitro.h').write_text('#include <stdint.h>\ntypedef uint64_t u64; typedef uint32_t u32;\n')
            (tmp/'nitro_port.h').write_text('#include <stddef.h>\n#include <stdint.h>\nvoid NitroPort_Log(const char *, ...);\n')
            shutil.copy(root/'src/compat/nitro/include/nitro_port_gx3d.h',tmp)
            common=[a.cc,'-O2','-fwrapv','-fno-strict-aliasing','-I',tmp,'-I',root/'src/runtime','-I',root/'src/compat/nitro/include']
            for name,path in [('sound','src/compat/nitro/src/port_sound_chip.c'),('g3','src/runtime/kj_g3.c')]:
                before=tmp/(name+'_before.c')
                before.write_text(run(['git','show',f'{a.reference}:{path}'])+'\n')
                results=[]
                for variant,source in [('before',before),('after',root/path)]:
                    exe=tmp/(name+'_'+variant+'.exe')
                    run([*common,'-DHGSS_PROFILE_OFF',f'-DSOURCE="{source.as_posix()}"',root/f'tests/native/{name}_diff.c','-o',exe])
                    results.append(run([exe]))
                assert results[0]==results[1], (name,results)
                print(f'PASS {name}: {results[0]}',flush=True)
            exe=tmp/'perf_test.exe'
            run([*common,'-I',root/'src',root/'tests/native/perf_test.c','-o',exe])
            print(run([exe]))
        '@ | Set-Content tests/native/check_hotpaths.py
        & 'C:/Program Files/Python315/python.exe' tests/native/check_hotpaths.py > build/diagnosis/native-checks.txt 2>&1
    ✗ (1) • 632ms

## Activity

    $ Get-Content build/diagnosis/native-checks.txt -Tail 20; Get-Content build/diagnosis/normal-build.log -Tail 5
    gcc: fatal error: no input files
    compilation terminated.
    Traceback (most recent call last):
      File "C:\Users\terezi\workdir\kohjoh\tests\native\check_hotpaths.py", line 33, in <module>
        run([*common,'-DHGSS_PROFILE_OFF',f'-DSOURCE="{source.as_posix()}"',root/f'tests/native/{name}_diff.c','-o',exe])
        ~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
      File "C:\Users\terezi\workdir\kohjoh\tests\native\check_hotpaths.py", line 17, in run
        return subprocess.check_output([str(x) for x in args], cwd=root, text=True).strip()
               ~~~~~~~~~~~~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
      File "C:\Program Files\Python315\Lib\subprocess.py", line 587, in check_output
        return run(*popenargs, stdout=PIPE, timeout=timeout, check=True,
               ~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                   **kwargs).stdout
                   ^^^^^^^^^
      File "C:\Program Files\Python315\Lib\subprocess.py", line 692, in run
        raise CalledProcessError(retcode, process.args,
                                 output=stdout, stderr=stderr)
    subprocess.CalledProcessError: Command ['c:\\devkitPro\\msys2\\usr\\bin\\gcc.EXE', '-O2', '-fwrapv', '-fno-strict-aliasing', '-I', 'C:\\Users\\terezi\\AppData\\Local\\Temp\\kohjoh-test-z0tuml94', '-I', 'C:\\Users\\terezi\\workdir\\kohjoh\\src\\runtime', '-I', 'C:\\Users\\terezi\\workdir\\kohjoh\\src\\compat\\nitro\\include', '-DHGSS_PROFILE_OFF', '-DSOURCE="C:/Users/terezi/AppData/Local/Temp/kohjoh-test-z0tuml94/sound_before.c"', 'C:\\Users\\terezi\\workdir\\kohjoh\\tests\\native\\sound_diff.c', '-o', 'C:\\Users\\terezi\\AppData\\Local\\Temp\\kohjoh-test-z0tuml94\\sound_before.exe'] returned non-zero exit status 1.
    built ../../../build/normal/sdk/libnitro.a
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitro'
    + /usr/bin/python /c/Users/terezi/workdir/kohjoh/tools/game/hgss_nitro.py make --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/normal/sdk --subproject nitrosystem lib -k -j 32
    + make -C /c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem TARGET=3ds DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM OUT=../../../build/normal/sdk GEN=/c/Users/terezi/workdir/kohjoh/build/normal/sdk/gen SDK=/c/Users/terezi/workdir/kohjoh/build/hgss-sdk SDK_SRCS=hgss/card/CARD_backup.c hgss/card/CARD_common.c hgss/card/CARD_pullOut.c hgss/card/CARD_request.c hgss/card/CARD_rom.c hgss/card/card_cache_flush_threshold.c hgss/card/card_identify_backup.c hgss/card/card_read_card.c hgss/card/card_request_stream_command.c hgss/card/card_spi_wait.c hgss/card/card_spi_wait_cycles.c hgss/card/card_spi_wait_init.c hgss/card/card_terminate_pulled_out.c hgss/card/card_try_read_dma.c hgss/cp/CP_context.c hgss/fs/FS_archive_prefix.c hgss/fs/FS_archive_suffix.c hgss/fs/FS_command.c hgss/fs/FS_command_default.c hgss/fs/FS_file.c hgss/fs/FS_overlay.c hgss/fs/FS_rom.c hgss/fs/fs_send_command.c hgss/fx/fx.c hgss/fx/fx_atanidx.c hgss/fx/fx_cp.c hgss/fx/fx_mtx22.c hgss/fx/fx_mtx33_identity.c hgss/fx/fx_mtx33_inverse_concat.c hgss/fx/fx_mtx33_rot_yz.c hgss/fx/fx_mtx33_scale_rot_x.c hgss/fx/fx_mtx43_identity.c hgss/fx/fx_mtx43_inverse_concat.c hgss/fx/fx_mtx43_rot.c hgss/fx/fx_mtx43_scale.c hgss/fx/fx_mtx43_scale_apply.c hgss/fx/fx_mtx43_trans_apply.c hgss/fx/fx_mtx44_concat.c hgss/fx/fx_mtx44_identity.c hgss/fx/fx_mtx44_ortho.c hgss/fx/fx_mtx44_perspective.c hgss/fx/fx_mtx44_rot.c hgss/fx/fx_mtx44_trans_apply.c hgss/fx/fx_trig.c hgss/fx/fx_trig_internal.c hgss/fx/fx_vec.c hgss/fx/fx_vec_mult_add.c hgss/fx/fx_vec_normalize.c hgss/gx/g2.c hgss/gx/g3.c hgss/gx/g3_util.c hgss/gx/g3b.c hgss/gx/g3imm.c hgss/gx/g3x.c hgss/gx/g3x_nop_clear_fifo.c hgss/gx/gx.c hgss/gx/gx_bgcnt.c hgss/gx/gx_load2d.c hgss/gx/gx_load3d.c hgss/gx/gx_vramcnt.c hgss/gx/gxasm.c hgss/gx/gxstate.c hgss/math/MATH_crc.c hgss/math/MATH_dgt.c hgss/math/MATH_pop.c hgss/math/math_qsort.c hgss/mi/mi_compress.c hgss/mi/mi_compress_search.c hgss/mi/mi_dma.c hgss/mi/mi_dma_card.c hgss/mi/mi_dma_gxcommand.c hgss/mi/mi_dma_hblank.c hgss/mi/mi_init.c hgss/mi/mi_wram.c hgss/mic.c hgss/os/os_alarm_after_handler.c hgss/os/os_alarm_before_handler.c hgss/os/os_alarm_irq_handler.c hgss/os/os_alloc.c hgss/os/os_arena.c hgss/os/os_context.c hgss/os/os_emulator.c hgss/os/os_entropy.c hgss/os/os_init.c hgss/os/os_init_system.c hgss/os/os_interrupt.c hgss/os/os_irqHandler.c hgss/os/os_irqTable.c hgss/os/os_message.c hgss/os/os_mutex.c hgss/os/os_ownerInfo.c hgss/os/os_printf.c hgss/os/os_spinLock.c hgss/os/os_spin_lock_card.c hgss/os/os_spin_lock_cartridge.c hgss/os/os_spin_lock_id.c hgss/os/os_spin_lock_word.c hgss/os/os_system.c hgss/os/os_terminate_proc.c hgss/os/os_thread.c hgss/os/os_thread_available.c hgss/os/os_tick.c hgss/os/os_timer.c hgss/os/os_valarm.c hgss/os/os_vramExclusive.c hgss/os/os_vram_exclusive_clz.c hgss/os/os_wait_irq.c hgss/os/os_wait_vblank.c hgss/pm.c hgss/rtc/rtc_api.c hgss/rtc/rtc_convert_date_time_to_second.c hgss/rtc/rtc_convert_post.c hgss/rtc/rtc_convert_pre.c hgss/rtc/rtc_internal.c hgss/snd/SND_alarm.c hgss/snd/SND_bank.c hgss/snd/SND_command.c hgss/snd/SND_main.c hgss/snd/SND_util.c hgss/snd/SND_work.c hgss/snd/snd_destroy_bank.c hgss/snd/snd_flush_command.c hgss/snd/snd_interface.c hgss/snd/snd_master_volume.c hgss/snd/snd_next_inst_data.c hgss/snd/snd_player_local_variable.c hgss/snd/snd_player_tempo_ratio.c hgss/snd/snd_read_channel_info.c hgss/snd/snd_read_driver_info.c hgss/snd/snd_read_track_info.c hgss/std_sprintf.c hgss/std_sprintf_writer.c hgss/std_string.c hgss/tp.c hgss/wm_core.c hgss/wm_data_sharing.c hgss/wm_requests.c hgss/wm_settings.c hgss_compat/sdk_data.c hgss_compat/sdk_standins.c hgss_compat/nns_data.c hgss_compat/nns_tables_gfd.c ARM7_SOUND_SRCS=hgss_sub/sound_main.c hgss_sub/sound_command.c hgss_sub/sound_ex_channel.c hgss_sub/sound_seq_player.c hgss_sub/sound_alarm.c hgss_sub/sound_bank.c hgss_sub/sound_channel.c hgss_sub/sound_driver.c hgss_sub/sound_shared_work.c hgss_compat/snd_arm7_data.c hgss_compat/ds_sound_tables.c SDK_INCLUDES=-I/c/Users/terezi/workdir/kohjoh/src/compat/nitro/include -I/c/Users/terezi/workdir/kohjoh/build/normal/sdk/gen -I/c/Users/terezi/workdir/kohjoh/src/compat/hgss/include -I/c/Users/terezi/workdir/kohjoh/build/hgss-sdk/include -I/c/Users/terezi/workdir/kohjoh/build/upstream/include -I/c/Users/terezi/workdir/kohjoh/build/upstream/include/library -I/c/Users/terezi/workdir/kohjoh/build/upstream/lib/NitroSDK/include -I/c/Users/terezi/workdir/kohjoh/build/upstream/lib/NitroSDK/include-mw -I/c/Users/terezi/workdir/kohjoh/build/upstream/lib/NitroSDK/include-pokediamond -I/c/Users/terezi/workdir/kohjoh/build/upstream/sub/include -I/c/Users/terezi/workdir/kohjoh/build/upstream/sub/lib/include -include /c/Users/terezi/workdir/kohjoh/build/upstream/lib/include/assert.h -include /c/Users/terezi/workdir/kohjoh/src/compat/hgss/include/hgss_port_math.h -DHGSS_PROFILE_OFF NNS=/c/Users/terezi/workdir/kohjoh/build/hgss-nns NNS_INCLUDES=-I/c/Users/terezi/workdir/kohjoh/build/hgss-nns/include NNS_SRCS=hgss_nns/fnd_allocator.c hgss_nns/fnd_expheap.c hgss_nns/fnd_frameheap.c hgss_nns/fnd_heapcommon.c hgss_nns/fnd_list.c hgss_nns/fnd_unitheap.c hgss_nns/g2d_animation.c hgss_nns/g2d_canvas_arrange.c hgss_nns/g2d_cell_animation.c hgss_nns/g2d_cell_transfer_manager.c hgss_nns/g2d_char_canvas.c hgss_nns/g2d_char_canvas_letter.c hgss_nns/g2d_char_canvas_util.c hgss_nns/g2d_char_util.c hgss_nns/g2d_font.c hgss_nns/g2d_font_load.c hgss_nns/g2d_image.c hgss_nns/g2d_load.c hgss_nns/g2d_mc_animation.c hgss_nns/g2d_node.c hgss_nns/g2d_oam.c hgss_nns/g2d_oam_soft_emu.c hgss_nns/g2d_palette_table.c hgss_nns/g2d_renderer.c hgss_nns/g2d_renderer_core.c hgss_nns/g2d_renderer_identity.c hgss_nns/g2d_renderer_mtx_cache.c hgss_nns/g2d_renderer_mtx_stack.c hgss_nns/g2d_renderer_transform.c hgss_nns/g2d_softsprite.c hgss_nns/g2d_srt_control.c hgss_nns/g2d_text_canvas.c hgss_nns/g3d_1mat1shp.c hgss_nns/g3d_anm_bca.c hgss_nns/g3d_anm_bca_data.c hgss_nns/g3d_anm_bca_scale.c hgss_nns/g3d_anm_bca_trans.c hgss_nns/g3d_anm_bca_trans_ex.c hgss_nns/g3d_anm_blend.c hgss_nns/g3d_anm_bma.c hgss_nns/g3d_anm_bta.c hgss_nns/g3d_anm_bta_sincos.c hgss_nns/g3d_anm_bta_vector.c hgss_nns/g3d_anm_btp.c hgss_nns/g3d_anm_btp_init.c hgss_nns/g3d_anm_bva.c hgss_nns/g3d_cgtool_3dsmax.c hgss_nns/g3d_cgtool_3dsmax_texmtx.c hgss_nns/g3d_cgtool_basic.c hgss_nns/g3d_cgtool_maya_joint.c hgss_nns/g3d_cgtool_maya_tex.c hgss_nns/g3d_cgtool_maya_texmtx.c hgss_nns/g3d_cgtool_maya_texmtx_r.c hgss_nns/g3d_cgtool_maya_texmtx_s.c hgss_nns/g3d_cgtool_si3d.c hgss_nns/g3d_cgtool_xsi.c hgss_nns/g3d_cgtool_xsi_texmtx.c hgss_nns/g3d_cgtool_xsi_texmtx_sr.c hgss_nns/g3d_cgtool_xsi_texmtx_t.c hgss_nns/g3d_draw.c hgss_nns/g3d_gecom.c hgss_nns/g3d_glb.c hgss_nns/g3d_glb_state.c hgss_nns/g3d_kernel.c hgss_nns/g3d_mdl_bind.c hgss_nns/g3d_model.c hgss_nns/g3d_res_struct_accessor.c hgss_nns/g3d_res_struct_accessor_anm.c hgss_nns/g3d_sbc.c hgss_nns/g3d_util.c hgss_nns/gfd_frame_pltt_vram_man.c hgss_nns/gfd_frame_tex_vram_man.c hgss_nns/gfd_linked_list_pltt_vram_man.c hgss_nns/gfd_linked_list_pltt_vram_man_reset.c hgss_nns/gfd_linked_list_tex_vram_man.c hgss_nns/gfd_linked_list_tex_vram_man_reset.c hgss_nns/gfd_linked_list_vram_man.c hgss_nns/gfd_pltt_vram_man.c hgss_nns/gfd_tex_vram_man.c hgss_nns/gfd_vram_transfer_manager.c hgss_nns/snd_arc.c hgss_nns/snd_arc_loader.c hgss_nns/snd_arc_loader_wave.c hgss_nns/snd_arc_player.c hgss_nns/snd_arc_strm.c hgss_nns/snd_capture.c hgss_nns/snd_fader.c hgss_nns/snd_heap.c hgss_nns/snd_player.c hgss_nns/snd_resource_mgr.c hgss_nns/snd_seqarc.c hgss_nns/snd_stream.c hgss_nns/snd_system.c hgss_nns/snd_waveout.c hgss_nns_compat/nns_tables_nns.c NNS_INLINE_SYMS=/c/Users/terezi/workdir/kohjoh/build/normal/sdk/gen/nns_inline.syms LIBNITRO=/c/Users/terezi/workdir/kohjoh/build/normal/sdk/libnitro.a NM=/c/Users/terezi/workdir/kohjoh/build/normal/sdk/nm-for-weaken.sh lib -k -j 32
    make: Entering directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    ✓ • 220ms

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tests/native/check_hotpaths.py > build/diagnosis/native-checks.txt 2>&1'
    ✓ • 15.11s

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/normal -j 32 > build/diagnosis/normal-build.log 2>&1'
    ✓ • 2m 09s

## Activity

    $ @'
        from pathlib import Path
        p=Path('docs/testing.md'); s=p.read_text().replace('The instrumented build includes per-H-blank timing probes, which add\nsubstantial overhead; use the normal build for performance comparisons.', '''The instrumented build times the whole H-blank walk. Per-scanline and IRQ
        probes require `HGSS_A0_DIAG`; they are too intrusive for normal profiling.
        Use the normal build for hardware performance comparisons.'''); a=s.index('`[g3sample]` samples'); b=s.index('\n- **Counters**',a); s=s[:a]+'''`[gamecpu]` reports logic percentiles over 600 game frames, including frames
        whose picture was skipped. `[perf] logic` now covers only the single game
        frame producing each displayed picture. Older logs accumulated skipped
        frames into the next picture: **80 ms per picture could cover two game
        frames**, and is not an 80 ms game update. Display intervals still cover
        all intervening skipped pictures. Skips have their own count and no longer
        inflate the display hitch count; hitch g2d is exclusive of g3d.
        
        `[g3sample]` randomly samples FIFO calls with probability 1/64 using a
        reproducible xorshift sequence, avoiding correlation with repeated model
        call patterns. Total, live decode and replay are raw ARM11 ticks for the
        samples; preemption and probe overhead remain included. The remainder is
        lookup/comparison/recording. These are estimates, not exact window totals.
        The first GSP reporting window establishes a baseline and is explicitly
        marked `baseline/unavailable`; a failed read also invalidates its baseline.
        '''+s[b:]; s=s.replace('- **Probes**: `HGSS_PROFILE=on` compiles the HGSS H-blank/2D tick probes in\n  (several hundred `svcGetSystemTick` calls a frame). Normal builds leave\n  them out; never make a timing claim from a probe build.', '''- **Probes**: `HGSS_PROFILE=on` enables frame, GPU, audio and sampled FIFO
          reporting. `HGSS_A0_DIAG` additionally enables the intrusive per-line
          H-blank probes. Neither is a hardware performance baseline.'''); s+='''
        ## Native hot-path regression checks
        
        Inside the devkitPro MSYS2 shell, run `python tests/native/check_hotpaths.py`.
        It compiles the current sound/geometry code and the `HEAD` versions with
        host GCC, compares output/state hashes for 100,000 randomized ADPCM programs
        and 2,000 synthetic geometry frames (split FIFO, cache hits, all primitives,
        lighting and texgen modes), and checks skipped-frame CPU and hitch accounting
        with a mocked clock. `--reference REV` selects another baseline. No game
        assets are needed. This is differential coverage, not a complete DS oracle.
        '''; p.write_text(s)
        p=Path('docs/worklog/2026-11-13.md'); s=p.read_text()+'''
        ## o3ds-log-3: repair measurements and CPU hot paths
        
        The appended `heartgold.log` contains three runs: old `5026d7b-dirty`, normal
        `8a92e93-dirty-off`, and instrumented `8a92e93-dirty-prof`. Do not mix them.
        In the final run, windows ending at 120.954 and 172.279 s contain 1,145 and
        1,150 game frames per 600 pictures. Their ~80 ms median logic samples are
        accumulations between pictures, often **two game updates**, not one.
        GPU queue completion averages ~4 ms and GPU wait is ~7 us. All 600 bottom
        screens and 4,200 retained layers reuse their content, with zero atlas
        conversions: rebuilding 2D is not the steady field CPU problem.
        
        Audio mix ticks rise from 5,321,582,634 at 114.211 s to 8,037,222,722 at
        170.460 s: about 18% of a 268 MHz core (wall intervals include preemption).
        FIFO's fixed-stride samples indicate substantial decode, replay and cache
        bookkeeping; their stride can correlate with model calls, so do not treat
        scaled values as exact totals. Audio runs on the same core and its
        preemption is already included in logic/FIFO wall times; never add them.
        
        Repairs:
        - `[gamecpu]`: every game frame sampled independently, including skipped
          pictures; displayed-picture logic no longer accumulates skipped updates.
        - Skips and display hitches counted separately; hitch g2d excludes g3d.
        - Ordinary profiling keeps whole-walk H-blank timing but removes hundreds
          of per-line SVC probes (`HGSS_A0_DIAG` retains detailed probes).
        - FIFO sampling uses reproducible pseudorandom 1/64 selection.
        - First GSP window is baseline-only (the observed first P3D delta was
          587,755 us/picture against 1.93 ms queue completion). Failed reads reset
          the baseline. This discards startup contamination without claiming its
          precise firmware cause.
        
        CPU repairs, justified by the sampled FIFO/audio load and Coffeepot CTR
        `engineering/performance.md` and `engineering/cpu-optimization.md`:
        - Split FIFO commands now use the existing vertex fast path instead of
          always entering the general command dispatcher/vertex submitter.
        - Display-list keys clear/copy/compare only the relevant initialized
          prefix; unused texture/light/shininess tails need no memory traffic.
        - ADPCM uses a 1,424-byte table of exact separately truncated step deltas,
          replacing repeated magnitude bit tests/additions. Sign, clamping, loop
          state and resampling are unchanged.
        
        Validation: native differential tests compare PCM/state across 100,000
        randomized ADPCM programs and geometry/matrices/batches/registers across
        2,000 synthetic frames. Before/after hashes match. Mock-clock regression
        checks cover independent skipped samples and exclusive hitch timing.
        Standalone ARMv6K binaries executed with Unicorn (instruction counting,
        **not hardware timing**) yield identical hashes: audio 22,237,237 to
        21,375,151 instructions (-3.88%); geometry 22,935,870 to 21,758,088
        (-5.14%). Fixtures and ARM benchmark outputs are under `build/diagnosis`.
        The geometry fixture mixes split and cached streams, not a retail scene;
        these percentages do not establish a game speedup. Actual console gains
        and remaining per-game CPU percentiles require the repaired build on O3DS.
        '''; p.write_text(s)
        p=Path('docs/status.md'); s=p.read_text(); i=s.index('\n## On `master`'); s=s[:i]+'''
        ## Latest investigation (uncommitted, 2026-11-13)
        
        `o3ds-log-3` confirms CPU pressure from geometry and audio, with GPU queues
        around 4 ms in the steady field window. Previous ~80 ms "logic" figures
        often accumulated **two game frames per picture**. The profiler now records
        independent game-frame CPU percentiles and removes ordinary per-line probes.
        Split FIFO decoding, display-list key traffic and ADPCM magnitude arithmetic
        have been reduced, with matching differential output/state hashes and
        synthetic ARM instruction reductions. O3DS speedup remains unmeasured.
        See [today's worklog](worklog/2026-11-13.md#o3ds-log-3-repair-measurements-and-cpu-hot-paths).
        '''+s[i:]; p.write_text(s)
        '@ | python
        Get-Content build/diagnosis/native-checks.txt -Tail 6; Get-Content build/diagnosis/normal-build.log -Tail 6
    PASS sound: e8bfdd766ba03a03
    PASS g3: 2aaf6f99e63dabab
    PASS: skipped game CPU samples, display hitch count, exclusive hitch timing
    == compiling the 3DS-side runtime (17 files)
    == link -> /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
       linked /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
       [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
       packaged /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold.3dsx (5531168 bytes)
       build info: pokeheartgold.build-info.txt
    ✓ • 342ms

## Activity

    $ @'
        from pathlib import Path
        import subprocess, json,hashlib
        nm='C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe'
        strings='C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe'
        for mode in ['normal','instrumented']:
            base=Path('build')/mode
            syms=subprocess.check_output([nm,str(base/'pokeheartgold-3ds.elf')],text=True)
            texts=subprocess.check_output([strings,str(base/'pokeheartgold-3ds.elf')],text=True)
            reports=['[gamecpu]','[g3sample]','[sndprof]','[gputime]','[perf]']
            for report in reports: assert (report in texts)==(mode=='instrumented'),(mode,report)
            for symbol in ['s_coarse_seq','s_game_logic','sProfMixCalls']:
                assert (symbol in syms)==(mode=='instrumented'),(mode,symbol)
            data=(base/'pokeheartgold.3dsx').read_bytes()
            print(mode,len(data),hashlib.sha256(data).hexdigest())
        '@ | python
        rg -n 'error:|vendored library build failed' build/diagnosis/normal-build.log build/diagnosis/instrumented-build.log; git diff --check -- src tools docs/testing.md docs/status.md docs/worklog/2026-11-13.md tests/native; git status --short
    normal 5531168 651c1cddb7b0787a673a1e3b20242b928903a4809ce429ef154eb6ca50323af9
    instrumented 5544780 758217204a4ace6f797acfc59dd9dfcbe3ccebf23a1af3bb22a58cf152e63a3c
     M README.md
     M docs/building-windows.md
     M docs/status.md
     M docs/testing.md
     M docs/worklog/2026-11-13.md
     M src/app/main.c
     M src/compat/nitro/src/port_os.c
     M src/compat/nitro/src/port_snd.c
     M src/compat/nitro/src/port_sound_chip.c
     M src/compat/nitro/src/port_timer.c
     M src/diagnostics/crash.c
     M src/diagnostics/log.c
     M src/diagnostics/perf.c
     M src/diagnostics/perf.h
     M src/graphics/g2d/g2d.c
     M src/graphics/g3d/g3d.c
     M src/graphics/g3d/textures.c
     M src/graphics/pica/gpu.c
     M src/graphics/renderer.c
     M src/graphics/resources/atlas.c
     M src/runtime/kj_frame.c
     M src/runtime/kj_g3.c
     M tools/azahar/run.ps1
     M tools/game/hgss_game.py
    ?? docs/chatlog.md
    ?? docs/chatlog02.txt
    ?? docs/chatlog03.txt
    ?? docs/chatlog04.txt
    ?? docs/chatlog05.md
    ?? docs/chatlog06.md
    ?? docs/chatlog07.md
    ?? docs/chatlog08.md
    ?? kohjoh.zip
    ?? kohjoh2.zip
    ?? kohjoh3.zip
    ?? private-gitignore/
    ?? tests/native/
    ✓ • 2.47s

## Assistant

Both builds are complete, and the binary audit confirms that the normal build lacks the profiling reports and state while the instrumented build contains them. The new regression suite passes against `HEAD`.

The remaining uncertainty is console timing: the old picture-based logic samples and fixed-stride FIFO samples cannot establish an exact CPU split. I’m checking that the repaired reports run in Azahar; the new instrumented build will provide per-game-frame measurements on the next console run.

## Activity

    $ Get-Content "$env:APPDATA/Azahar/sdmc/3ds/kohjoh/logs/heartgold.log" -Tail 20; Get-Content build/diagnosis/native-checks.txt; Get-Content build/normal/pokeheartgold.build-info.txt -First 5; Get-Content build/instrumented/pokeheartgold.build-info.txt -First 5
        40.880 [frame]   877: tile conv 0, spans 129, quads 9431, tex uploads 0, tex bytes compared 0
        40.880 [frame]   878: tile conv 0, spans 2, quads 2327, tex uploads 0, tex bytes compared 0
        40.880 [frame]   879: tile conv 0, spans 2, quads 2327, tex uploads 0, tex bytes compared 0
        40.880 [frame]   882: tile conv 456, spans 3, quads 1632, tex uploads 33, tex bytes compared 0
        40.880 [frame]   883: tile conv 0, spans 56, quads 1632, tex uploads 0, tex bytes compared 23680
        40.881 [frame]   987: tile conv 0, spans 56, quads 1632, tex uploads 49, tex bytes compared 0
        40.881 [frame]   988: tile conv 0, spans 130, quads 1632, tex uploads 0, tex bytes compared 44368
        40.881 [frame]   1095: tile conv 0, spans 56, quads 4704, tex uploads 0, tex bytes compared 44368
        40.881 [frame]   1096: tile conv 0, spans 130, quads 11314, tex uploads 0, tex bytes compared 44368
        40.881 [gpu] 600 frames: 2D screens skipped 293, layers reused 1598 rebuilt 4312, bg quads 1135221 obj quads 70118 draws 4983 spans 4635 unsupported 0 dropped 0, char KiB compared 8396
        40.881 [g3sample] random 1/64 FIFO calls: samples 520 total_ticks 4165277 decode_ticks 927790 replay_ticks 1369506; remainder includes lookup/recording and probe overhead
        40.881 [dlwhy] batch-in-block 0 shared-verts 0 straddle/drops 0 lit+matrix 0 swap 0 open-prim 0 flushes 0
        40.881 [dlcov] bypass: small 113027 split 121610 open-begin 0 (begin first 0)
        40.881 [dlcov] words: short 234637 new 30198 changed 7614 never 0 hit 2090088 keymiss 97359
        40.881 [ds] per game frame (601 game frames / 600 pictures): g3 fifo calls 54 (4093 fifo words, 23 direct words, 390 cmds), timer sync 22 us (4 calls), sound pump 3 us, hblank 40 us
        40.881 [ds] display lists: 6280 replayed, 1161 decoded, 0 uncacheable, 335 KiB retained
        40.881 [pace] 600 frames: late V-blanks 29, resyncs 0, presents skipped 1
        40.881 [gpu] 600 frames: atlas lookups 710976 conversions 2293 evictions 0 full 0; 3D batches 16435 draws 16435 tris 701655 tex uploads 119 tex bytes compared 12321520 tex full 0 unsupported 325; box tests 0 culled 0 (far only 0)
        40.882 [gputime] 600 pictures: GSP us/picture P3D 0 PPF 7157589 PSC 0 (log valid); C3D drawing ms avg 1.80 max 11.92; cmdbuf use max 48%
        47.458 [sndprof] render_calls 3000 render_frames 1536000 stretches 11980 sndticks 8997 mix_ticks 1656027883 tick_ticks 73672112 mix_calls 11980 mix_frames 1536000 active_ch 132282 pcm8_ch 0 pcm16_ch 0 adpcm_ch 132282 psg_ch 0 cap_inst 0 dep_ch 0
    PASS sound: e8bfdd766ba03a03
    PASS g3: 2aaf6f99e63dabab
    PASS: skipped game CPU samples, display hitch count, exclusive hitch timing
    game: heartgold
    target: 3ds
    profiling: off
    defines: (none)
    texture_profile: no
    game: heartgold
    target: 3ds
    profiling: profile
    defines: (none)
    texture_profile: no
    ✓ • 265ms

## Activity

    $ pwsh -NoProfile -File tools/azahar/run.ps1 -Artifact build/instrumented/pokeheartgold.3dsx -Shots 55 -Extra 2 -Tag diagnosis-profile
    shot build\azahar\diagnosis-profile\t055.png
    log copied
    ✓ • 1m 00s

## Assistant

The longer smoke test caught another GPU-counter problem: Azahar returned a decreasing transfer total, which unsigned subtraction turned into a bogus 7.16-second average. I’m adding a wall-time bound so impossible deltas are marked unavailable and establish a fresh baseline. The new game-frame CPU and random FIFO reports are running successfully.

## Activity

    $ @'
        from pathlib import Path
        p=Path('tests/native/g3_diff.c'); s=p.read_text().replace('#include <stdio.h>', '#include <stdio.h>\n#include <assert.h>'); s=s.replace('void NitroPort_Log(const char *fmt,...) { (void)fmt; }','void NitroPort_Log(const char *fmt,...) { assert(strstr(fmt,"MISMATCH")==NULL); }'); s=s.replace('        digest(g->verts', '        assert(g != NULL && g->vert_count != 0);\n        digest(g->verts'); p.write_text(s)
        p=Path('tests/native/check_hotpaths.py'); s=p.read_text(); pos=s.index('    exe=tmp/\'perf_test.exe\''); s=s[:pos]+'''    exe=tmp/'g3_verify.exe'
            source=root/'src/runtime/kj_g3.c'
            run([*common,'-DHGSS_PROFILE_OFF','-DKJ_DL_VERIFY',f'-DSOURCE="{source.as_posix()}"',root/'tests/native/g3_diff.c','-o',exe])
            assert run([exe])==results[1], 'cached/live geometry mismatch'
            print('PASS g3: live decode verification',flush=True)
        '''+s[pos:]; p.write_text(s)
        '@ | python
        C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tests/native/check_hotpaths.py > build/diagnosis/native-checks.txt 2>&1'
    ✓ • 24.89s

## Activity

    $ Get-Content build/diagnosis/native-checks.txt -Tail 8; Get-Content build/diagnosis/instrumented-build.log -Tail 4; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1106 -First 16; Get-Content tests/native/check_hotpaths.py -Tail 17; git diff --check -- src tools docs/status.md docs/testing.md docs/worklog/2026-11-13.md tests/native
    PASS sound: e8bfdd766ba03a03
    PASS g3: 2aaf6f99e63dabab
    PASS g3: live decode verification
    PASS: skipped game CPU samples, display hitch count, exclusive hitch timing
    == assembling ARM9 .s (150 files)
       150 assembled, 0 failed
    == port libraries
    + /usr/bin/python /c/Users/terezi/workdir/kohjoh/tools/game/hgss_nitro.py make --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/instrumented/sdk --subproject nitro lib -k -j 32
    #endif
    #ifdef KJ_G3_ZERO_TRACE
        {
            /* Diagnostics: report the first commands that leave a zero matrix. */
            static unsigned reported;
            static const mtx zero;
            const bool pos_was = memcmp(&s_pos, &zero, sizeof(zero)) == 0;
            const bool proj_was = memcmp(&s_proj, &zero, sizeof(zero)) == 0;
            execute_body(op, p);
            const bool pos_is = memcmp(&s_pos, &zero, sizeof(zero)) == 0;
            const bool proj_is = memcmp(&s_proj, &zero, sizeof(zero)) == 0;
            if (((pos_is && !pos_was) || (proj_is && !proj_was)) && reported < 64) {
                ++reported;
                NitroPort_Log("[g3zero] op %02x p0 %08lx mode %d sp %d -> pos %d proj %d (swap %lu)", op,
                              (unsigned long)p[0], s_mtx_mode, s_posvec_sp, pos_is, proj_is,
                              (unsigned long)s_stats.swaps);
            before=tmp/(name+'_before.c')
            before.write_text(run(['git','show',f'{a.reference}:{path}'])+'\n')
            results=[]
            for variant,source in [('before',before),('after',root/path)]:
                exe=tmp/(name+'_'+variant+'.exe')
                run([*common,'-DHGSS_PROFILE_OFF',f'-DSOURCE="{source.as_posix()}"',root/f'tests/native/{name}_diff.c','-o',exe])
                results.append(run([exe]))
            assert results[0]==results[1], (name,results)
            print(f'PASS {name}: {results[0]}',flush=True)
        exe=tmp/'g3_verify.exe'
        source=root/'src/runtime/kj_g3.c'
        run([*common,'-DHGSS_PROFILE_OFF','-DKJ_DL_VERIFY',f'-DSOURCE="{source.as_posix()}"',root/'tests/native/g3_diff.c','-o',exe])
        assert run([exe])==results[1], 'cached/live geometry mismatch'
        print('PASS g3: live decode verification',flush=True)
        exe=tmp/'perf_test.exe'
        run([*common,'-I',root/'src',root/'tests/native/perf_test.c','-o',exe])
        print(run([exe]))
    ✓ • 431ms

## Activity

    $ @'
        from pathlib import Path
        p=Path('src/runtime/kj_g3.c'); s=p.read_text(); a=s.index('#ifndef HGSS_PROFILE_OFF\n/* Sample one FIFO'); b=s.index('\nstatic void dl_replay_impl',a); block=s[a:b].replace('s_coarse_decode, s_coarse_replay;', 's_coarse_decode, s_coarse_replay, s_coarse_copy;'); s=s[:a]+s[b:]; pos=s.index('static bool dl_append_block('); s=s[:pos]+block+'\n\n'+s[pos:]; s=s.replace('    const uint32_t v0 = fb->frame.vert_count;\n    const uint32_t hi', '''#ifndef HGSS_PROFILE_OFF
            const u64 copy_start = s_coarse_sample ? svcGetSystemTick() : 0;
        #endif
            const uint32_t v0 = fb->frame.vert_count;
            const uint32_t hi''',1); s=s.replace('    *v_first_out = v0;\n    return true;', '''    *v_first_out = v0;
        #ifndef HGSS_PROFILE_OFF
            if (s_coarse_sample) s_coarse_copy += svcGetSystemTick() - copy_start;
        #endif
            return true;''',1); s=s.replace('replay_ticks %llu; remainder includes lookup/recording and probe overhead', 'replay_ticks %llu (block_copy_ticks %llu nested); remainder includes lookup/recording and probe overhead'); s=s.replace('(unsigned long long)s_coarse_decode, (unsigned long long)s_coarse_replay);','(unsigned long long)s_coarse_decode, (unsigned long long)s_coarse_replay,\n                  (unsigned long long)s_coarse_copy);'); s=s.replace('s_coarse_total = s_coarse_decode = s_coarse_replay = 0;', 's_coarse_total = s_coarse_decode = s_coarse_replay = s_coarse_copy = 0;'); p.write_text(s)
        '@ | python
        Get-Content build/diagnosis/instrumented-build.log -Tail 3
      ARM7 CC sound_shared_work.c
      ARM7 CC ds_sound_tables.c
      ARM7 CC snd_arm7_data.c
    ✓ • 488ms

## Activity

    $ @'
        from pathlib import Path
        p=Path('docs/testing.md'); s=p.read_text().replace('The remainder is\nlookup/comparison/recording.', '`block_copy_ticks` is nested inside replay and covers vertex/index copying\nand rebasing; replay minus that child includes state events, matrix/palette\npreparation and exit-state restoration. Do not add the child to replay.\nThe outer remainder is lookup/comparison/recording.'); p.write_text(s)
        p=Path('docs/worklog/2026-11-13.md'); s=p.read_text()+'''
        The final instrumented FIFO report also nests block-copy ticks inside replay,
        so a new hardware sample can separate vertex/index copy/rebase from replay
        state/matrix/palette handling; the old report could not. These additional
        block probes run only inside the randomly selected calls, never in baseline.
        Native cache verification also compares cached recordings with live decode
        through `KJ_DL_VERIFY` across the synthetic corpus (zero mismatches).
        '''; p.write_text(s)
        '@ | python
        Get-Content build/diagnosis/instrumented-build.log -Tail 6; git diff --check -- src tools tests/native docs/testing.md docs/status.md docs/worklog/2026-11-13.md
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
      129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
    == duplicate symbols
       0 symbols defined in more than one object (0 objects weakened)
    == Wi-Fi stand-ins
    ✓ • 345ms

## Activity

    $ @'
        from pathlib import Path
        p=Path('tests/native/g3_diff.c'); s=p.read_text().replace('lists[l][n++]=0x23232220;', 'lists[l][n++]=(k&1) ? 0x23222120 : 0x23232220;').replace('for(int j=0;j<4;j++)', 'for(int j=0;j<((k&1)?3:4);j++)'); p.write_text(s)
        '@ | python
        C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tests/native/check_hotpaths.py > build/diagnosis/native-checks.txt 2>&1'
        Get-Content build/diagnosis/instrumented-build.log -Tail 8
    built ../../../build/instrumented/sdk/libnitro.a
      1 NitroSystem header inline functions
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
      129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
    == duplicate symbols
       0 symbols defined in more than one object (0 objects weakened)
    == Wi-Fi stand-ins
    ✓ • 13.41s

## Activity

    $ Get-Content build/diagnosis/instrumented-build.log -Tail 8; Get-Content build/diagnosis/native-checks.txt -Tail 5; C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe build/instrumented/pokeheartgold-3ds.elf | Select-String 'block_copy_ticks|baseline/unavailable'
    built ../../../build/instrumented/sdk/libnitro.a
      1 NitroSystem header inline functions
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
      129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
    == duplicate symbols
       0 symbols defined in more than one object (0 objects weakened)
    == Wi-Fi stand-ins
    PASS sound: e8bfdd766ba03a03
    PASS g3: d511eb58d36aceeb
    PASS g3: live decode verification
    PASS: skipped game CPU samples, display hitch count, exclusive hitch timing
    
    baseline/unavailable
    
    ✓ • 489ms

## Activity

    $ @'
        from pathlib import Path
        p=Path('src/graphics/renderer.c'); s=p.read_text().replace('static bool s_perflog_valid;', 'static bool s_perflog_valid;\nstatic uint64_t s_perflog_tick;'); s=s.replace('            bool valid = false;', '''            const uint64_t perf_tick = svcGetSystemTick();
                    const uint64_t wall_us = (perf_tick - s_perflog_tick) * 1000000u / SYSCLOCK_ARM11;
                    bool valid = false;'''); s=s.replace('                s_perflog_prev = now;', '''                /* Each operation engine is serial. Its total cannot exceed
                         * the wall interval (PSC is the sum of two fill engines).
                         * Reject counter resets/torn reads instead of presenting a
                         * nearly-2^32 unsigned difference as GPU work. */
                        if (p3d > wall_us / 600u + 1u || ppf > wall_us / 600u + 1u ||
                            psc > 2u * (wall_us / 600u + 1u)) {
                            valid = false;
                        }
                        s_perflog_tick = perf_tick;
                        s_perflog_prev = now;'''); p.write_text(s)
        p=Path('docs/worklog/2026-11-13.md'); s=p.read_text()+'''
        Azahar 2126.2 smoke: boot/intro rendered, NDSP and repaired `[gamecpu]`,
        random FIFO, frame and sound reports ran. A second window exposed a
        backwards PPF counter producing a near-2^32 delta. GSP reports now reject
        an operation engine's total exceeding its elapsed wall interval (twice
        that interval for the summed fill engines), label it unavailable, and
        rebaseline. This also guards resets/torn counter reads on hardware.
        '''; p.write_text(s)
        p=Path('docs/testing.md'); s=p.read_text().replace('marked `baseline/unavailable`; a failed read also invalidates its baseline.', 'marked `baseline/unavailable`; failed reads invalidate its baseline. Deltas\nexceeding elapsed wall time for an operation engine are rejected and\nrebaselined (two engines for the summed fill counters).'); s=s.replace('`[dlcov]` (with `KJ_DL_VERIFY`)', '`[dlcov]`'); p.write_text(s)
        '@ | python
        C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && HGSS_PROFILE=on python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/instrumented -j 32 > build/diagnosis/instrumented-build.log 2>&1'
    ✓ • 2m 35s

## Activity

    $ @'
        from pathlib import Path
        p=Path('docs/status.md'); s=p.read_text().replace('Nothing here has reached O3DS.', 'Hardware logs below supply O3DS observations; sustained deadline certification remains open.'); p.write_text(s)
        p=Path('docs/testing.md'); s=p.read_text().replace('  words, H-blank cost, display-list cache).', '  words, H-blank cost, display-list cache). The legacy `cmds` counter counts\n  general-dispatch commands and excludes bulk fast commands; use FIFO words\n  for workload comparisons.'); p.write_text(s)
        '@ | python
        Get-Content build/diagnosis/native-checks.txt; git diff --check -- src tools tests/native docs/testing.md docs/status.md docs/worklog/2026-11-13.md; rg -n 'error:|vendored library build failed' build/diagnosis/normal-build.log build/diagnosis/instrumented-build.log; Get-FileHash build/normal/pokeheartgold.3dsx,build/instrumented/pokeheartgold.3dsx -Algorithm SHA256 | Select-Object Path,Hash
    PASS sound: e8bfdd766ba03a03
    PASS g3: d511eb58d36aceeb
    PASS g3: live decode verification
    PASS: skipped game CPU samples, display hitch count, exclusive hitch timing
    
    Path                                                                 Hash
    ----                                                                 ----
    C:\Users\terezi\workdir\kohjoh\build\normal\pokeheartgold.3dsx       651C1CDDB7B0787A673A1E3B20242B928903A4809CE429EF1…
    C:\Users\terezi\workdir\kohjoh\build\instrumented\pokeheartgold.3dsx DA940562007D082876A5630813F0C886F40709104E7ACD5EF…
    
    ✓ • 649ms

## Assistant

The final instrumented build also separates replay’s vertex/index copy cost from its state-handling cost, so the next hardware log can narrow the remaining replay bottleneck.

Decode, cache-key, and ADPCM repairs are built. Differential tests and cached-versus-live geometry verification pass. I’m finishing the final runtime check; I have not established a console FPS improvement.

## Activity

    $ @'
        from pathlib import Path
        import subprocess,hashlib
        for mode in ('normal','instrumented'):
            p=Path('build')/mode
            elf=str(p/'pokeheartgold-3ds.elf')
            strings=subprocess.check_output(['C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe',elf],text=True)
            nm=subprocess.check_output(['C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe',elf],text=True)
            for x in ('[gamecpu]','[g3sample]','[sndprof]','[gputime]','block_copy_ticks'):
                assert (x in strings)==(mode=='instrumented'),(mode,x)
            for x in ('s_coarse_seq','s_coarse_copy','s_game_logic','sProfMixCalls'):
                assert (x in nm)==(mode=='instrumented'),(mode,x)
            print(mode,hashlib.sha256((p/'pokeheartgold.3dsx').read_bytes()).hexdigest())
        '@ | python
        Get-Content "$env:APPDATA/Azahar/sdmc/3ds/kohjoh/logs/heartgold.log" -Tail 4
    normal 651c1cddb7b0787a673a1e3b20242b928903a4809ce429ef154eb6ca50323af9
    instrumented da940562007d082876a5630813f0c886f40709104e7acd5ef14df34e0d1915b4
        40.881 [ds] display lists: 6280 replayed, 1161 decoded, 0 uncacheable, 335 KiB retained
        40.881 [pace] 600 frames: late V-blanks 29, resyncs 0, presents skipped 1
        40.881 [gpu] 600 frames: atlas lookups 710976 conversions 2293 evictions 0 full 0; 3D batches 16435 draws 16435 tris 701655 tex uploads 119 tex bytes compared 12321520 tex full 0 unsupported 325; box tests 0 culled 0 (far only 0)
        40.882 [gputime] 600 pictures: GSP us/picture P3D 0 PPF 0 PSC 0 (log baseline/unavailable); C3D drawing ms avg 1.80 max 11.92; cmdbuf use max 48%
    ✓ • 2.35s

## Activity

    $ Get-Content build/diagnosis/instrumented-build.log -Tail 8; C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe build/instrumented/pokeheartgold-3ds.elf | Select-String 'block_copy_ticks'; pwsh -NoProfile -File tools/azahar/run.ps1 -Artifact build/instrumented/pokeheartgold.3dsx -Shots 48 -Tag diagnosis-final
      257 Wi-Fi SDK functions stood in for (online play is not ported)
    == compiling the DS-side runtime (11 files)
    == compiling the 3DS-side runtime (17 files)
    == link -> /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
       linked /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold-3ds.elf
       [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
       packaged /c/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold.3dsx (5545216 bytes)
       build info: pokeheartgold.build-info.txt
    
    [g3sample] random 1/64 FIFO calls: samples %lu total_ticks %llu decode_ticks %llu replay_ticks %llu (block_copy_ticks
    %llu nested); remainder includes lookup/recording and probe overhead
    shot build\azahar\diagnosis-final\t048.png
    log copied
    
    ✓ • 53.10s

## Activity

    $ Get-Content build/azahar/diagnosis-final/heartgold.log | Select-String 'gamecpu|g3sample|gputime'; @'
        from pathlib import Path
        p=Path('docs/worklog/2026-11-13.md'); s=p.read_text()+'''
        Final Azahar smoke (`build/azahar/diagnosis-final`) reached two reporting
        windows: game-frame CPU, random FIFO with nested copy ticks, sound and frame
        reports emitted; the impossible PPF delta was correctly rejected as
        `baseline/unavailable`. Both `.3dsx` artifacts packaged without vendored
        library errors. ELF audits confirm all profiling report strings/state absent
        from normal and present in instrumented. Neither final binary has O3DS timing
        validation. SHA-256 normal: `651c1cddb7b0787a673a1e3b20242b928903a4809ce429ef154eb6ca50323af9`;
        instrumented: `da940562007d082876a5630813f0c886f40709104e7acd5ef14df34e0d1915b4`.
        '''; p.write_text(s)
        '@ | python
    
        20.617 [gamecpu] 600 game frames, logic us p50 365 p95 452 p99 2868 max 40008 (includes preemption; excludes
    V-blank wait)
        20.790 [g3sample] random 1/64 FIFO calls: samples 0 total_ticks 0 decode_ticks 0 replay_ticks 0 (block_copy_ticks
    0 nested); remainder includes lookup/recording and probe overhead
        20.790 [gputime] 600 pictures: GSP us/picture P3D 0 PPF 0 PSC 0 (log baseline/unavailable); C3D drawing ms avg
    1.09 max 3.45; cmdbuf use max 1%
        40.674 [gamecpu] 600 game frames, logic us p50 637 p95 5418 p99 8093 max 77568 (includes preemption; excludes
    V-blank wait)
        40.881 [g3sample] random 1/64 FIFO calls: samples 520 total_ticks 4759618 decode_ticks 951149 replay_ticks 1952112
    (block_copy_ticks 730302 nested); remainder includes lookup/recording and probe overhead
        40.882 [gputime] 600 pictures: GSP us/picture P3D 0 PPF 0 PSC 0 (log baseline/unavailable); C3D drawing ms avg
    1.80 max 11.92; cmdbuf use max 48%
    
    ✓ • 507ms

## Assistant

Found and repaired several concrete issues:

- Logic timings combined skipped game frames; the reported ~80 ms often covered two updates.
- Split FIFO commands bypassed the vertex fast path.
- Cache lookups processed unused key fields.
- ADPCM repeated arithmetic now replaced by an exact lookup table.
- Profiling now avoids per-scanline probes, randomizes FIFO sampling, separates replay copying, and rejects impossible GPU counters.

Differential tests pass, including cached-versus-live geometry verification. Synthetic ARM instruction counts fell **5.1% for geometry** and **3.9% for audio**. Azahar smoke tests pass; actual O3DS gains remain unmeasured.

Updated builds:

- [Normal](C:/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold.3dsx)
- [Instrumented](C:/Users/terezi/workdir/kohjoh/build/instrumented/pokeheartgold.3dsx)

[Detailed findings and validation](C:/Users/terezi/workdir/kohjoh/docs/worklog/2026-11-13.md).
