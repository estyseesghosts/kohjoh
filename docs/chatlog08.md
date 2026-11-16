# Codex conversation

## User

you're working on kohjoh. first, read the project documentation and commit history to get yourself up to speed. then, read chatlog07, read C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2 (and n3ds-log-2) and continue diagnosing what's still hot. refer to workdir/coffeepot/ctr for 3ds technical knowledge. you should also make our logs nondestructive, so it's easier and faster for me to test multiple builds in sequence. come back to me with two builds, one instrumented and one uninstrumented - make sure the uninstrumented build is genuinely uninstrumented, and vice versa.

## Assistant

I’ll get up to speed on the project and recent investigation first, then trace the remaining hot paths and make the logging changes before producing and verifying both builds.

## Activity

    $ Get-ChildItem -Force; Get-ChildItem -Force kohjoh; git -C kohjoh log -12 --oneline; rg --files kohjoh | Select-Object -First 100
    
    
        Directory: C:\Users\terezi\workdir
    
    
    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    d-----        11/10/2026   2:00 PM                .claude
    d-----        11/11/2026   1:58 AM                .venv
    d-----        11/11/2026   6:47 PM                3ds-bench-staged
    d-----        11/11/2026  12:49 PM                3ds-bench-staging
    d-----        11/11/2026   3:33 PM                3ds-bench-staging-n3ds
    d-----        11/11/2026   1:35 PM                3ds-bench-staging-v2
    d-----        11/11/2026   6:35 AM                bw-private
    d-----        11/11/2026   5:25 AM                bw-switch
    d-----        11/12/2026   2:05 AM                coffeepot
    d-----        11/11/2026   9:46 PM                fedifedifedi
    d-----        11/11/2026   1:21 PM                hgss-3ds
    d-----        11/12/2026   5:11 PM                hgss-private
    d-----        11/12/2026  12:05 AM                ivywall
    d-----        11/10/2026  11:05 AM                keystone
    d-----         11/9/2026   4:37 PM                kissme
    d-----        11/13/2026  11:26 AM                kohjoh
    d-----        11/12/2026   5:29 PM                melonDS
    d-----        11/13/2026  12:13 PM                pica-lab
    d-----        11/13/2026   5:15 AM                Pokemon-Soulsilver-New3ds
    d-----         11/9/2026   6:05 PM                wintty
    d-----         11/9/2026   4:30 PM                yggdrasil
    d-----        11/11/2026   2:31 AM                zensical-check
    -a----         3/14/2024  12:55 PM          49756 dspfirm.cdc
    -a----         11/9/2026   5:39 PM      134217728 heartgold.nds
    -a----        11/12/2026   5:08 PM         524288 heartgold.sav
    -a----        11/12/2026   5:08 PM       43520000 melonDS.exe
    -a----        11/12/2026   5:11 PM           3101 melonDS.toml
    -a----        11/12/2026   7:13 PM          74948 nds-to-3ds-pica200-porting-engineering-report.md
    -a----         11/9/2026   7:09 PM          71560 PICA200.md
    -a----        11/12/2026  11:05 PM          10172 recover_cw2004.py
    -a----        11/12/2026   5:11 PM           2304 wfcsettings.bin
    
    
        Directory: C:\Users\terezi\workdir\kohjoh
    
    
    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    da-h--        11/13/2026  11:34 AM                .git
    d-----        11/13/2026  11:25 AM                build
    d-----        11/12/2026   5:35 AM                dist
    d-----        11/13/2026  11:34 AM                docs
    d-----        11/12/2026   5:38 AM                patches
    d-----        11/13/2026  12:52 PM                private-gitignore
    d-----        11/12/2026   5:44 AM                src
    d-----        11/12/2026  12:21 PM                tests
    d-----        11/12/2026   5:10 PM                tools
    -a----        11/12/2026   5:35 AM            196 .gitignore
    -a----        11/12/2026   3:46 PM           3358 AGENTS.md
    -a----        11/12/2026   5:18 AM          39920 kohjoh.md
    -a----        11/12/2026   9:51 PM        2390134 kohjoh.zip
    -a----        11/13/2026  12:35 AM        3205458 kohjoh2.zip
    -a----        11/13/2026  11:26 AM        4383476 kohjoh3.zip
    -a----        11/12/2026  12:01 PM          35149 LICENSE
    -a----        11/12/2026  12:01 PM           3106 PROVENANCE.md
    -a----        11/13/2026   1:01 PM            841 README.md
    -a----        11/12/2026   5:35 AM           1291 upstream.lock
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
    kohjoh\upstream.lock
    kohjoh\kohjoh3.zip
    kohjoh\kohjoh2.zip
    kohjoh\kohjoh.zip
    kohjoh\kohjoh.md
    kohjoh\tools\rompack\verify_rom.py
    kohjoh\tools\rompack\pack.py
    kohjoh\tools\rompack\extract_rom.py
    kohjoh\tools\melonds\shots.ps1
    kohjoh\docs\worklog\2026-11-13.md
    kohjoh\docs\worklog\2026-11-12.md
    kohjoh\docs\troubleshooting.md
    kohjoh\docs\testing.md
    kohjoh\docs\status.md
    kohjoh\docs\README.md
    kohjoh\docs\lessons.md
    kohjoh\docs\glossary.md
    kohjoh\README.md
    kohjoh\docs\chatlog03.txt
    kohjoh\docs\chatlog02.txt
    kohjoh\PROVENANCE.md
    kohjoh\docs\chatlog.md
    kohjoh\docs\building-windows.md
    kohjoh\docs\building-linux.md
    kohjoh\docs\architecture.md
    kohjoh\LICENSE
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
    kohjoh\docs\chatlog07.md
    kohjoh\docs\chatlog06.md
    kohjoh\docs\chatlog05.md
    kohjoh\docs\chatlog04.txt
    kohjoh\AGENTS.md
    kohjoh\tools\game\bootstrap.py
    kohjoh\tools\game\check_patches.py
    kohjoh\tools\game\game_overlays.py
    kohjoh\tools\game\game_sources.py
    kohjoh\src\storage\save_raw.c
    kohjoh\src\storage\paths.h
    kohjoh\src\storage\paths.c
    kohjoh\tools\azahar\run.ps1
    kohjoh\tools\azahar\gdbprof.py
    kohjoh\tools\capture\ds_reference.py
    kohjoh\tools\game\s_to_gas.py
    kohjoh\tools\game\prepare_game_headers.py
    kohjoh\tools\game\nns_inline_symbols.py
    kohjoh\tools\game\make_game_wifi_stubs.py
    kohjoh\tools\game\hgss_nitro.py
    kohjoh\tools\game\hgss_game.py
    kohjoh\tools\capture\g3_reference.py
    kohjoh\src\runtime\kj_g3.h
    kohjoh\src\runtime\kj_g3.c
    kohjoh\src\runtime\kj_frame.c
    kohjoh\src\audio\audio.h
    kohjoh\src\audio\audio.c
    kohjoh\src\runtime\kj_boot.c
    kohjoh\src\app\script.h
    kohjoh\src\app\script.c
    kohjoh\src\app\main.c
    kohjoh\src\app\input.h
    kohjoh\src\app\input.c
    kohjoh\src\app\app.h
    kohjoh\src\runtime\kj_bridge.h
    kohjoh\src\compat\common.mk
    kohjoh\src\compat\stubs\wm_stubs.c
    kohjoh\src\compat\stubs\mb_stubs.c
    kohjoh\src\compat\stubs\dsprot_stubs.c
    kohjoh\src\compat\stubs\cpp_runtime.c
    kohjoh\src\diagnostics\perf.h
    kohjoh\src\diagnostics\perf.c
    kohjoh\src\diagnostics\log.h
    kohjoh\src\diagnostics\log.c
    kohjoh\src\diagnostics\dump.h
    kohjoh\src\diagnostics\dump.c
    kohjoh\src\diagnostics\crash.h
    kohjoh\src\diagnostics\crash.c
    kohjoh\src\compat\nitro\Makefile
    kohjoh\src\graphics\resources\atlas.h
    kohjoh\src\graphics\resources\atlas.c
    kohjoh\src\graphics\renderer.h
    kohjoh\src\graphics\renderer.c
    kohjoh\src\compat\hgss\src\snd_arm7_data.c
    kohjoh\src\compat\hgss\src\sdk_standins.c
    kohjoh\src\compat\hgss\src\sdk_data.c
    kohjoh\src\compat\hgss\src\nns_volume_table.c
    kohjoh\src\compat\hgss\src\nns_tables_nns.c
    kohjoh\src\compat\hgss\src\nns_tables_gfd.c
    kohjoh\src\compat\hgss\src\nns_data.c
    kohjoh\src\compat\hgss\src\msl_runtime.c
    kohjoh\src\compat\hgss\src\ds_sound_tables.c
    kohjoh\src\compat\hgss\src\arm7_sound_compat.c
    kohjoh\src\compat\hgss\README.md
    
    
    ✗ (1) • 1.56s

## Activity

    $ Get-Content AGENTS.md; Get-Content README.md; Get-Content docs/status.md; Get-Content docs/building-windows.md; Get-Content docs/worklog/2026-11-13.md; Get-Content docs/chatlog07.md; Get-Content private-gitignore/o3ds-log-2; Get-Content private-gitignore/n3ds-log-2; git status --short; git log -30 --oneline
    # AGENTS.md
    
    Repository-wide rules for agent sessions working on kohjoh.
    
    ## READ THE WIKI
    
    You're not expected to have deep technical knowledge of the 3DS platform or how to program efficiently for it. This information has been collected for you already. If it isn't already on disk, clone https://github.com/estyseesghosts/coffeepot.git. Always refer to coffeepot's CTR section when you're writing code or planning project changes - read the relevant hardware, firrmware or engineering articles, so you understand what you're working with. If you can't justify a change with real-hardware evidence or a citation from the wiki, you probably should not do it. Never edit coffeepot yourself. If you notice something genuinely incorrect, PR and tell the user. Always follow the wiki's C style guide.
    
    The coffeepot wiki is provided locally on this machine in workdir/coffeepot/.
    ## Delegating to subagents
    
    A lead agent must give every subagent it calls **explicit instructions, explicit deliverables and explicit avoidables**. This is not optional, and it matters most for implementer agents: they edit files and cannot rely on chat context the lead has not written down.
    
    Every subagent brief must state, in the prompt itself:
    
    1. **Objective** â€” the one bounded slice to complete, and why it exists.
    2. **Allowed paths** â€” the exact files or directories the subagent may read and edit.
    3. **Deliverables** â€” the concrete output: code, patch files, tests, docs, and the required report format (files changed, commands run, results, open questions).
    4. **Acceptance checks** â€” the commands or observations that prove the work is done, and the expected result.
    5. **Avoidables** â€” what not to touch or do: paths outside the slice, unrelated refactors, the decomp checkout in place, commits/pushes unless explicitly assigned, and any known traps from `kohjoh.md` or `docs/troubleshooting.md`.
    6. **Stopping point** â€” when to stop and report instead of guessing, for example if the slice grows, the brief conflicts with `kohjoh.md`, or an acceptance check cannot pass.
    
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
    | Geometry decoder fast paths | verified | Goldenrod renders; logic 37 â†’ 25 ms/frame (Azahar) |
    | PokÃ©mon-data keystream cache (game patch 2002) | built | crypto was ~12% of field instructions |
    | Present skip (never two in a row) | run | `0089b28`; O3DS starvation diagnosed from o3ds-log-0 |
    | Display-list cache | verified, **on by default** since `c2087f3` (`--define KJ_DL_NO_CACHE` to disable) | `KJ_DL_VERIFY` 0 mismatches in Goldenrod idle, intro, title, and Goldenrod â†’ Route 34 â†’ trainer battle â†’ field; battle frames use no lists |
    | GPU timing `[gputime]` (`6444c8b`) | run | C3D drawing time and command-buffer peak; GSP perf log reads 0 in Azahar (not emulated), O3DS untested |
    | GX command DMA in one transfer (SDK patch 2003) | verified | 65% of FIFO words had been split mid-command; logic 21.8 â†’ 21.0 ms |
    | Party move scan, one decrypt per mon (game patch 2006) | verified | Goldenrod idle logic âˆ’0.1 ms |
    | Saves | run | interim raw file `saves/<edition>.sav` ([`save_raw.c`](../src/storage/save_raw.c)) |
    | Audio | run | user heard music; Azahar now has DSP firmware, NDSP runs |
    | Sound mixer ADPCM render pass (`da6b36c`, `dacb07d`) | verified | host differential test bit-exact (100k programs); Azahar title 125 â†’ 102 instructions per channel-sample |
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
    
    - Two-generation SaveStore ([kohjoh.md Â§4.4](../kohjoh.md)); needs bool/int
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
    3. Route test: Goldenrod â†’ Route 34 grass â†’ wild battle (`tests/inputs/route34-south.txt`).
    4. O3DS re-test of Goldenrod, the title and a battle on a default build
       (cache, mixer and `[gputime]` all in), with ARM11 performance counters
       for I-cache misses. H-blank (0.5 ms) and texture work wait for that data.
    5. Whole-batch frustum culling (chatlog06: 72 of 124 Goldenrod batches lie
       wholly outside one plane).
    6. Title/Ho-Oh: lit lists decode live with CPU lighting (`shade` ~11% of
       samples); vertex lighting on the PICA would cut that and let them record.
    7. CIA packaging, SoulSilver build.
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
    
    - Bootstrap â‰ˆ 1 min (shallow fetch + `git am` of the patch stack).
    - First build â‰ˆ 5 min at `-j 32`. A rebuild after a kohjoh-side change takes
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
    | `HGSS_PROFILE=on` | compiles in the HGSS tick probes (H-blank split timings in `port_os.c`). Off by default since `0089b28`; before that every build carried them |
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
    branch: switch branches â†’ `bootstrap.py --clean`.
    
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
    # 2026-11-13
    
    ## Sound mixer (o3ds-log-1 hot path)
    
    - o3ds-log-1: the mixer took 20-30% of the application core in every
      window, ~170 cycles per channel-sample, ~99% of active channels ADPCM.
      Azahar had never run it (no DSP firmware); `dspfirm.cdc` is now staged.
    - `da6b36c`: ADPCM decoded and resampled into a block buffer (32-bit phase
      fraction, exact under a checked precondition), then volume/pan in a
      second pass. 125.1 â†’ 109.2 instructions per channel-sample.
    - `dacb07d`: SSAT saturation, one-compare loop mark, sub-step loop.
      â†’ 102.1. `mix_block` 3.2 â†’ 4.4 KiB.
    - Both proven bit-exact with a host differential driver (random register
      programs, 100k iterations, output and chip state hashed every call).
    - Azahar A/B: title screen idle (no input script), `[sndprof]` windows,
      identical channel counts in both builds.
    
    Open: O3DS measurement; further levers are ARMv6 SIMD in the volume/pan
    pass and moving the mixer to core 1 (kohjoh.md 5.5 requires an A/B).
    ## The hardware logs narrow this down considerably
    
    The strongest finding is that the severe Goldenrod slowdown is CPU-side, not a PICA200 triangle-throughput problem.
    
    In the new Old 3DS logs, the worst sustained windows show:
    
    | Metric                           | O3DS debug build | O3DS normal build |
    | -------------------------------- | ---------------- | ----------------- |
    | Game logic, median               | 121.9 ms         | 143.9 ms          |
    | 3D draw preparation, median      | 5.5 ms           | 5.1 ms            |
    | 2D rendering, median             | 1.2 ms           | 1.2 ms            |
    | GPU wait, median                 | 0.007 ms         | 0.007 ms          |
    | Displayed-frame interval, median | 130 ms           | 154 ms            |
    
    These measurements are from different passes through the scene, not perfectly matched A/B runs.
    
    Crucially, the renderer reports GPU drawing averages around 3â€“4 ms per picture during the worst geometry-heavy sections. That leaves an enormous discrepancy: the GPU finishes its work quickly, while the game thread spends roughly 120â€“145 ms producing the next frame.
    
    The slowdown also reproduces on a New 3DS held at O3DS clock speeds. This is not an isolated fault with one physical console.
    
    We should stop prioritizing GPU throughput optimizations until this CPU-side cost is explained.
    
    ## The cache is not the obvious regression culprit
    
    The logs contain two builds:
    
    - `5026d7b-dirty` â€” ordinary build
    - `8a92e93-dirty-prof` â€” later profiled build
    
    Both report millions of display-list cache hits. So these are not cache-disabled versus cache-enabled runs, despite the different commit dates.
    
    In the sustained 51,310-FIFO-word scene, the later O3DS build reports approximately 122 ms median logic time versus 144 ms in the earlier build. The runs aren't strictly controlled, but there's no evidence here that the cache is causing that catastrophic slowdown.
    
    More importantly, even the lighter field sections are close to the deadline: one O3DS window at roughly 20,700 FIFO words per game frame has a 24.9 ms median logic stage, before 2D work, 3D submission and interruptions from audio.
    
    That explains why a few extra milliseconds can now destabilize scenes that used to hold 30 FPS. We need to measure where those milliseconds went, rather than treating the entire renderer as equally suspect.
    
    ## Audio is less suspicious than it first appeared
    
    The new `sndprof` counters let us compare mixer work directly.
    
    At approximately 39 seconds into the O3DS runs:
    
    - Earlier mixer: about 2.26 billion mixing ticks
    - Optimized mixer: about 1.63 billion mixing ticks
    
    The number of active channels is slightly different, but normalized cost also improves, from roughly 169 to 130 ticks per channel-sample.
    
    That's a substantial hardware improvement. Audio still competes with the game on core 0, but the revised mixer is not the leading explanation for the new slowdown.
    
    This leaves one disproportionately large, insufficiently measured stage: the DS geometry command processing that happens inside the `logic` timer, before Kohjoh enters its separately timed PICA200 draw path.
    
    ## What the remaining counters tell us
    
    The sustained Goldenrod workload contains approximately:
    
    | Per game frame or presented picture        | Measurement |
    | ------------------------------------------ | ----------- |
    | DS geometry FIFO calls/game frame          | 979         |
    | FIFO words/game frame                      | 51,310      |
    | DS geometry commands/game frame            | 6,885       |
    | Triangles/presented picture                | 8,370       |
    | Display-list words replayed from cache     | \~74%       |
    | Words bypassing cache                      | \~20%       |
    | Words needing re-decoding after key misses | \~5%        |
    
    That is a substantial amount of geometry processing on the ARM11, even though the cached lists no longer require full decoding.
    
    The source explains why: `dl_replay()` still executes state events, computes matrix references and copies vertices and indices into the current frame. Cache hits aren't free.
    
    There's another useful negative result: during the worst O3DS windows, the renderer reports zero rebuilt 2D layers, with median 2D time around 1.2 ms. So Goldenrod's sustained slowdown is not the 2D retention problem we previously suspected.
    
    ## The two measurements we are missing
    
    I would add only two coarse timing groups before doing any further optimization.
    
    1\. Game-side geometry production
    
    Measure the complete NitroSystem model/draw calls, excluding the time they spend inside `NitroPort_GxFifoWords`.
    
    This determines whether PokÃ©mon's own model traversal, animation, visibility handling and geometry generation are expensive.
    
    2\. Kohjoh's geometry translation
    
    Measure total time inside `NitroPort_GxFifoWords`, broken into cache lookup/comparison, replayed state commands, replayed vertex/index copying, and live decoding.
    
    That gives us a decision tree:
    
    | Result                              | Next optimization                                                        |
    | ----------------------------------- | ------------------------------------------------------------------------ |
    | Game-side model traversal dominates | Cache model traversal, transforms and geometry generation                |
    | Live decoding dominates             | Improve DMA batching and eliminate uncached short/split lists            |
    | Replay state commands dominate      | Retain model-level state and avoid executing redundant commands          |
    | Vertex/index copying dominates      | Retain immutable GPU geometry                                            |
    | Neither dominates                   | Profile game functions outside rendering, especially PokÃ©mon data access |
    
    Use sampled or aggregated timing, not one clock read per opcode. The `-prof` build already demonstrates millions of timed opcode calls and is unsuitable as the performance baseline.
    
    ## My revised priority
    
    1. Do one clean O3DS cache-on/cache-off A/B, using the same commit and location. Both supplied builds had caching active, so this comparison is still missing.
    2. Separate game-side geometry generation from Kohjoh's FIFO translation. This is the single most informative new measurement.
    3. Investigate the approximately 20% uncached FIFO traffic, especially command streams split across calls.
    4. Target whichever CPU stage actually dominates. Don't implement another GPU culling pass before measuring whether it can eliminate meaningful upstream CPU work.
    
    One final distinction: the logs confirm a severe performance problem, but do not yet prove a specific recent code regression. The older stable field windows processed around 17,300 FIFO words per game frame; a newer problematic window processed around 20,700. Those aren't identical workloads.
    
    The diagnosis is now much narrower: the PICA200 and retained 2D renderer appear to have sufficient headroom. Audio has improved. The remaining question is how much of the expensive DS geometry work occurs in the game itself versus Kohjoh's decoder and cache. One carefully instrumented O3DS run should answer that.
    Get-Content : Access to the path 'C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2' is denied.
    At line:2 char:184
    + ... t docs/chatlog07.md; Get-Content private-gitignore/o3ds-log-2; Get-Co ...
    +                          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        + CategoryInfo          : PermissionDenied: (C:\Users\terezi...nore\o3ds-log-2:String) [Get-Content], Unauthorized
       AccessException
        + FullyQualifiedErrorId : GetContentReaderUnauthorizedAccessError,Microsoft.PowerShell.Commands.GetContentCommand
    
    Get-Content : Access to the path 'C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2' is denied.
    At line:2 char:226
    + ... itignore/o3ds-log-2; Get-Content private-gitignore/n3ds-log-2; git st ...
    +                          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        + CategoryInfo          : PermissionDenied: (C:\Users\terezi...nore\n3ds-log-2:String) [Get-Content], Unauthorized
       AccessException
        + FullyQualifiedErrorId : GetContentReaderUnauthorizedAccessError,Microsoft.PowerShell.Commands.GetContentCommand
    
     M README.md
    ?? docs/chatlog.md
    ?? docs/chatlog02.txt
    ?? docs/chatlog03.txt
    ?? docs/chatlog04.txt
    ?? docs/chatlog05.md
    ?? docs/chatlog06.md
    ?? docs/chatlog07.md
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
    923789b Texture cache hints, g2d compare counter; document the O3DS session
    d089ca2 Shrink the G3 decode loop under the I-cache; word-wide mon crypto
    0089b28 Goldenrod hot paths: present-skip cap, decoder fast paths, probes off
    92a9a6d docs: complete, verified Windows build guide
    aadc864 docs: Ho-Oh skinning root cause, g3 reference projector, melonDS oracle, pitfalls
    617a681 Diagnostics: 3D scene in dumps, offline projector, zero-matrix trace, counters
    f769ae3 Title Ho-Oh: route direct G3 command-register writes to the geometry engine
    4661af7 Title screen 3D: DS W-buffer depth; real frame-interval hitch log; crash handler; script hold ranges
    38f395b docs: day-one status, lessons, testing guide and worklog
    7961266 Fix stale SDK dependencies breaking save loading
    0f8caac docs: clarify kohjoh guides and add glossary
    0f48f4d docs: document kohjoh architecture and workflow
    40c6984 Pacing lock, tile-edge inset, geometry fast paths, retained 2D layers, profiler
    4063c5e Goldenrod field renders in Azahar from the prior hgss-3ds bench save
    95cd1ff Frame stepping, state dumps and an offline DS reference renderer; fix PICA tile row order
    75ae75c kohjoh: new HGSS 3DS port: GPU-native runtime on the vendored game-compile layer
    ✓ • 1.59s

## Activity

    $ Get-ChildItem private-gitignore/o3ds-log-2,private-gitignore/n3ds-log-2 -Recurse | Select-Object FullName,Length; Get-Content docs/lessons.md; Get-Content docs/testing.md; rg -n 'fopen|logs/|sdmc:|KJ_.*PROF|sndprof|gputime' src tools/game/hgss_game.py | Select-Object -First 180; rg --files C:/Users/terezi/workdir/coffeepot/ctr | Select-Object -First 80
    
    FullName                                                                                Length
    --------                                                                                ------
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps-dbg
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\logs
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps\f00150.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps\f00450.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps\f00900.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps\f01350.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps\f01650.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps\f05000.kjd            3169036
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps-dbg\f00150.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps-dbg\f00450.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps-dbg\f00900.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps-dbg\f01350.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps-dbg\f01650.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps-dbg\f05000.kjd        3169036
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\dumps-dbg\heartgold-dbg.log 53213
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\logs\heartgold-dbg.log      53213
    C:\Users\terezi\workdir\kohjoh\private-gitignore\o3ds-log-2\logs\heartgold.log          17533
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\logs
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps\f00150.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps\f00450.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps\f00900.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps\f01350.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps\f01650.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps\f02300.kjd            2965856
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps\f02500.kjd            2965888
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps\f05000.kjd            2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg\f00150.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg\f00450.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg\f00900.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg\f01350.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg\f01650.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg\f02300.kjd        2965856
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg\f02500.kjd        2965888
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg\f05000.kjd        2881696
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\dumps-dbg\heartgold-dbg.log 46187
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\logs\heartgold-dbg.log      46187
    C:\Users\terezi\workdir\kohjoh\private-gitignore\n3ds-log-2\logs\heartgold.log          17567
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
      actual hotspots (geometry decode, PokÃ©mon crypto), including one outside the
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
    
    V-blank indices count from boot; game frames are publishes (â‰ˆ V-blanks / 2).
    A scripted run ignores live input on `wip/savestore`; on `master` a live press
    can still leak in, so do not type into the Azahar window during a test.
    
    | Script | Purpose |
    | --- | --- |
    | `intro-step.txt` | intro frames 150-1650, dumps + holds (no save needed) |
    | `goldenrod-step.txt`, `goldenrod-input.txt` | bench save â†’ Goldenrod; touch/B/DOWN checks |
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
      Î”mix_ticks / (Î”active_ch / Î”mix_calls) / Î”mix_frames. Needs
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
    tools/game/hgss_game.py:1641:                    help="extra preprocessor define for the runtime and game (e.g. KJ_G3_PROFILE)")
    src\storage\paths.h:10:    char root[96];             /* sdmc:/3ds/kohjoh */
    src\app\script.h:9: * File: sdmc:/3ds/kohjoh/script.txt. Lines:
    src\app\script.h:17: *                         <frame> to sdmc:/3ds/kohjoh/dumps/f<frame>.kjd
    src\storage\paths.c:4: * .3dsx: everything under sdmc:/3ds/kohjoh/ (data/<edition>.pak and its
    src\storage\paths.c:5: *        manifest, saves/, logs/).
    src\storage\paths.c:40:    snprintf(s_paths.root, sizeof(s_paths.root), "sdmc:/3ds/kohjoh");
    src\storage\paths.c:41:    ensure_dir("sdmc:/3ds");
    src\storage\paths.c:52:    snprintf(s_paths.log_file, sizeof(s_paths.log_file), "%s/logs/%s.log", s_paths.root, edition);
    src\graphics\renderer.c:217:            kj_log("[gputime] 600 pictures: GSP us/picture P3D %lu PPF %lu PSC %lu (log %s); "
    src\graphics\renderer.c:230:        mkdir("sdmc:/3ds/kohjoh/dumps", 0777);
    src\graphics\renderer.c:231:        snprintf(path, sizeof(path), "sdmc:/3ds/kohjoh/dumps/f%05lu.kjd", (unsigned long)in->game_frame);
    src\app\script.c:75:    FILE *f = fopen(path, "r");
    src\app\main.c:156:        snprintf(crash, sizeof(crash), "%s/logs/crash.txt", paths->root);
    src\runtime\kj_g3.c:1066:#ifdef KJ_G3_PROFILE
    src\runtime\kj_g3.c:2144:#ifdef KJ_G3_PROFILE
    src\runtime\kj_g3.c:2185:#ifdef KJ_G3_PROFILE
    src\runtime\kj_g3.c:2426:    *fifo_ticks = (uint64_t)s_fifo_calls; /* calls; ticks only with KJ_G3_PROFILE */
    src\diagnostics\dump.c:49:    FILE *f = fopen(path, "wb");
    src\diagnostics\crash.c:73:            FILE *f = fopen(s_crash_path, "w");
    src\runtime\kj_bridge.h:36: * sdmc:/...). Returns NULL on success or a human-readable reason. */
    src\diagnostics\log.c:93:    s_file = path != NULL ? fopen(path, "w") : NULL;
    src\compat\pak\tests\test_pak.c:34:    FILE *f = fopen(path, "wb");
    src\compat\pak\tests\test_pak.c:59:    FILE *f = fopen(path, "r+b");
    src\compat\pak\tests\test_pak.c:131:    f = fopen(path, "rb");
    src\compat\nitro\README.md:212:NDSP buffers. If `sdmc:/3ds/dspfirm.cdc` is missing, the application
    src\compat\nitro\README.md:233:NitroPort_SetSavePath("sdmc:/3ds/platinum/platinum.sav");
    src\compat\nitro\README.md:243:`sdmc:` path. The host uses an ordinary filesystem path.
    src\compat\nitro\README.md:287:if (NitroPort_OpenPak("sdmc:/3ds/pokeplatinum/platinum3ds.pak",
    src\compat\nitro\README.md:288:                      "sdmc:/3ds/pokeplatinum/manifest.json",
    src\compat\nitro\README.md:298:Mount/initialize the SD filesystem first on the 3DS, so its `sdmc:`
    src\compat\pak\src\pak.c:133:    file = fopen(path, "rb");
    src\compat\nitrosystem\tests\test_nitrosystem.c:73:    FILE *f = fopen(path, "rb");
    src\compat\nitro\src\port_card.c:112:    sSave = fopen(sSavePath, "r+b");
    src\compat\nitro\src\port_card.c:114:        sSave = fopen(sSavePath, "w+b");
    src\compat\nitro\src\port_fs.c:215:    file = fopen(manifest_path, "rb");
    src\compat\nitro\src\port_snd.c:292:        NitroPort_Log("[sndprof] render_calls %llu render_frames %llu stretches %llu sndticks %llu mix_ticks %llu tick_ticks %llu mix_calls %llu mix_frames %llu active_ch %llu pcm8_ch %llu pcm16_ch %llu adpcm_ch %llu psg_ch %llu cap_inst %llu dep_ch %llu",
    src\compat\nitro\src\port_sound_chip.c:866: * for its [sndprof] line via NitroSound_MixProfile below. */
    src\compat\nitro\src\port_sound_chip.c:1063:/* Cumulative profile counters for port_snd.c's [sndprof] line (declared
    C:/Users/terezi/workdir/coffeepot/ctr\software\update-data.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\titles.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\title-metadata-and-tickets.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\title-list.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\system-flaws.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\svc-reference.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\streetpass-cec.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\services.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\security-architecture.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\sd-filesystem.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\savegames.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\os.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\nand-filesystem.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\kernel.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\kernel-objects.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\firmware.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\file-formats.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\dev-unit-software.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\cro-modules.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\country-region-data.md
    C:/Users/terezi/workdir/coffeepot/ctr\software\boot-process.md
    C:/Users/terezi/workdir/coffeepot/ctr\index.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\research-index.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\picasso-and-citro3d.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\luma3ds.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\libctru.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\history.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\exploits.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\development-workflow.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\custom-firmware.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\boot9strap.md
    C:/Users/terezi/workdir/coffeepot/ctr\homebrew\3ds-examples.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\video-capture.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\tp-nets.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\security-hardware.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\peripheral-registers.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\models.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\memory.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\io-and-peripherals.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\gpu.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\gpu-shading-features.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\gpu-internal-registers.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\gpu-external-registers.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\gamecard.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\dsp.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\display.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\cpu.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\audio.md
    C:/Users/terezi/workdir/coffeepot/ctr\hardware\arm11-mpcore.md
    C:/Users/terezi/workdir/coffeepot/ctr\glossary.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\threading.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\testing-on-hardware.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\system-info-and-power.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\source-versions.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\service-groups.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\porting.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\stereo-engineering.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\rendering-2d.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\renderer-architecture.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\pica200-shader-programming.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\pica200-command-pipeline.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\gpu-rendering.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\frame-pacing.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\asset-pipeline.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\3d-scene-optimisation.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\PICA200\2d-tile-renderer.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\performance.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\nfc-and-infrared.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\new-3ds-qtm-and-mvd.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\networking.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\microphone.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\memory-and-cache.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\local-wireless.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\ipc-and-services.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\input-and-sensors.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\filesystem-and-saves.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\debugging.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\cpu-optimization.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\citro3d-fixed-function-api.md
    C:/Users/terezi/workdir/coffeepot/ctr\engineering\application-lifecycle.md
    
    
    ✓ • 1.49s

## Assistant

The supplied logs confirm the earlier diagnosis: the severe windows are dominated by time inside game logic, while GPU wait and retained 2D work are small. I’m checking the existing probes carefully because the current “normal” build still includes several timing and counter facilities, so simply omitting the old profiling flags would not produce a truly uninstrumented comparison.

## Activity

    $ Get-Content src/diagnostics/log.c; Get-Content src/diagnostics/perf.h; Get-Content src/storage/paths.c; Get-Content src/diagnostics/crash.c; rg -n 'svcGetSystemTick|kj_perf|sndprof|gputime|GxFifoWords|dl_replay' src/runtime/kj_g3.c src/graphics/renderer.c src/compat/nitro/src/port_snd.c src/compat/nitro/src/port_sound_chip.c src/app src/runtime/kj_frame.c; Get-Content C:/Users/terezi/workdir/coffeepot/ctr/engineering/performance.md -TotalCount 100; Get-Content C:/Users/terezi/workdir/coffeepot/ctr/engineering/filesystem-and-saves.md -TotalCount 70; Get-Content private-gitignore/o3ds-log-2/logs/heartgold.log -Tail 20; Get-Content private-gitignore/n3ds-log-2/logs/heartgold.log -Tail 10
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
        s_file = path != NULL ? fopen(path, "w") : NULL;
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
    
    #endif
    /*
     * paths.c: storage roots for the two installation formats.
     *
     * .3dsx: everything under sdmc:/3ds/kohjoh/ (data/<edition>.pak and its
     *        manifest, saves/, logs/).
     * .cia:  the pack is romfs:/data/game.pak (+ game.manifest.json); saves
     *        live in the title's save archive (storage/save_archive.c).
     */
    
    #include "storage/paths.h"
    
    #include "diagnostics/log.h"
    #include "runtime/kj_bridge.h"
    
    #include <3ds.h>
    #include <errno.h>
    #include <stdio.h>
    #include <string.h>
    #include <sys/stat.h>
    
    static kj_paths s_paths;
    
    static bool file_exists(const char *path)
    {
        struct stat st;
        return stat(path, &st) == 0 && S_ISREG(st.st_mode);
    }
    
    static void ensure_dir(const char *path)
    {
        if (mkdir(path, 0777) != 0 && errno != EEXIST) {
            kj_log("[paths] cannot create %s (errno %d)", path, errno);
        }
    }
    
    const kj_paths *kj_paths_init(void)
    {
        const char *edition = kj_ds_edition_name();
    
        snprintf(s_paths.root, sizeof(s_paths.root), "sdmc:/3ds/kohjoh");
        ensure_dir("sdmc:/3ds");
        ensure_dir(s_paths.root);
        {
            char dir[128];
            snprintf(dir, sizeof(dir), "%s/data", s_paths.root);
            ensure_dir(dir);
            snprintf(dir, sizeof(dir), "%s/saves", s_paths.root);
            ensure_dir(dir);
            snprintf(dir, sizeof(dir), "%s/logs", s_paths.root);
            ensure_dir(dir);
        }
        snprintf(s_paths.log_file, sizeof(s_paths.log_file), "%s/logs/%s.log", s_paths.root, edition);
    
        if (R_SUCCEEDED(romfsInit()) && file_exists("romfs:/data/game.pak")) {
            s_paths.bundled = true;
            snprintf(s_paths.pack_file, sizeof(s_paths.pack_file), "romfs:/data/game.pak");
            snprintf(s_paths.manifest_file, sizeof(s_paths.manifest_file),
                     "romfs:/data/game.manifest.json");
        } else {
            s_paths.bundled = false;
            snprintf(s_paths.pack_file, sizeof(s_paths.pack_file), "%s/data/%s.pak", s_paths.root,
                     edition);
            snprintf(s_paths.manifest_file, sizeof(s_paths.manifest_file),
                     "%s/data/%s.manifest.json", s_paths.root, edition);
        }
        snprintf(s_paths.save_file, sizeof(s_paths.save_file), "%s/saves/%s.sav", s_paths.root,
                 edition);
        return &s_paths;
    }
    /*
     * crash.c: CPU exception reporting (3DS side).
     *
     * Coffeepot ctr/engineering/debugging.md ("Catching exceptions in your own
     * program"): each thread installs a handler with threadOnException; on a
     * fault the kernel jumps to it with the exception information and all CPU
     * registers. The handler runs on its own static stack (the fault may be a
     * stack overflow), records the registers in the log file and a separate
     * crash file, then hands over to ERRF_ExceptionHandler so the console shows
     * the usual error screen. It does not return.
     *
     * Feed the reported pc and lr to
     *   arm-none-eabi-addr2line -f -C -e build/kj/pokeheartgold-3ds.elf 0x...
     * (3dsx.ld links at 0x00100000; no adjustment needed).
     *
     * Azahar note: Azahar does not raise data aborts for unmapped guest memory;
     * it logs "Unmapped Read/Write" with the PC in azahar_log.txt and carries on.
     * Look there first when a run hangs or misbehaves in the emulator.
     */
    
    #include "diagnostics/crash.h"
    
    #include "diagnostics/log.h"
    
    #include <3ds.h>
    #include <stdio.h>
    #include <string.h>
    
    #define HANDLER_STACK_BYTES 0x4000
    #define MAX_THREADS 8
    
    static u8 s_stacks[MAX_THREADS][HANDLER_STACK_BYTES] __attribute__((aligned(8)));
    static ERRF_ExceptionData s_data[MAX_THREADS];
    static volatile u32 s_next;
    static char s_crash_path[128];
    static volatile u32 s_in_handler;
    
    static const char *type_name(ERRF_ExceptionType t)
    {
        switch (t) {
        case ERRF_EXCEPTION_PREFETCH_ABORT: return "prefetch abort";
        case ERRF_EXCEPTION_DATA_ABORT: return "data abort";
        case ERRF_EXCEPTION_UNDEFINED: return "undefined instruction";
        case ERRF_EXCEPTION_VFP: return "VFP exception";
        default: return "unknown exception";
        }
    }
    
    static void handler(ERRF_ExceptionInfo *excep, CpuRegisters *regs)
    {
        /* A second fault (in this handler or another thread): go straight to
         * the system screen. */
        if (__atomic_fetch_add(&s_in_handler, 1, __ATOMIC_SEQ_CST) == 0) {
            char text[640];
            int n = snprintf(text, sizeof(text),
                             "[crash] %s on thread %08lx: pc %08lx lr %08lx sp %08lx cpsr %08lx "
                             "far %08lx fsr %08lx\n[crash] r0 %08lx r1 %08lx r2 %08lx r3 %08lx "
                             "r4 %08lx r5 %08lx r6 %08lx r7 %08lx\n[crash] r8 %08lx r9 %08lx "
                             "r10 %08lx r11 %08lx r12 %08lx\n",
                             type_name(excep->type), (unsigned long)(u32)(uintptr_t)getThreadLocalStorage(),
                             (unsigned long)regs->pc, (unsigned long)regs->lr, (unsigned long)regs->sp,
                             (unsigned long)regs->cpsr, (unsigned long)excep->far,
                             (unsigned long)excep->fsr, (unsigned long)regs->r[0],
                             (unsigned long)regs->r[1], (unsigned long)regs->r[2],
                             (unsigned long)regs->r[3], (unsigned long)regs->r[4],
                             (unsigned long)regs->r[5], (unsigned long)regs->r[6],
                             (unsigned long)regs->r[7], (unsigned long)regs->r[8],
                             (unsigned long)regs->r[9], (unsigned long)regs->r[10],
                             (unsigned long)regs->r[11], (unsigned long)regs->r[12]);
            /* The crash file first: it needs no lock the faulting thread could
             * hold (the log ring does). */
            if (s_crash_path[0] != '\0') {
                FILE *f = fopen(s_crash_path, "w");
                if (f != NULL) {
                    fwrite(text, 1, n > 0 ? (size_t)n : 0, f);
                    fclose(f);
                }
            }
            kj_log("%s", text);
            kj_log_flush_now();
        }
        ERRF_ExceptionHandler(excep, regs);
    }
    
    void kj_crash_init(const char *crash_path)
    {
        if (crash_path != NULL) {
            snprintf(s_crash_path, sizeof(s_crash_path), "%s", crash_path);
        }
        kj_crash_install();
    }
    
    void kj_crash_install(void)
    {
        const u32 i = __atomic_fetch_add(&s_next, 1, __ATOMIC_SEQ_CST);
        if (i >= MAX_THREADS) {
            return; /* the system screen still reports it */
        }
        threadOnException(handler, s_stacks[i] + HANDLER_STACK_BYTES, &s_data[i]);
    }
    src/compat/nitro/src/port_sound_chip.c:866: * for its [sndprof] line via NitroSound_MixProfile below. */
    src/compat/nitro/src/port_sound_chip.c:1063:/* Cumulative profile counters for port_snd.c's [sndprof] line (declared
    src/runtime/kj_frame.c:36:extern u64 svcGetSystemTick(void);
    src/runtime/kj_frame.c:99:        const u64 t0 = svcGetSystemTick();
    src/runtime/kj_frame.c:103:        s_hblank_ticks += svcGetSystemTick() - t0;
    src/runtime/kj_frame.c:132:        const u64 t0 = svcGetSystemTick();
    src/runtime/kj_frame.c:134:        s_pump_ticks += svcGetSystemTick() - t0;
    src/compat/nitro/src/port_snd.c:30:extern u64 svcGetSystemTick(void);
    src/compat/nitro/src/port_snd.c:31:#define SNDPROF_NOW() svcGetSystemTick()
    src/compat/nitro/src/port_snd.c:292:        NitroPort_Log("[sndprof] render_calls %llu render_frames %llu stretches %llu sndticks %llu mix_ticks %llu tick_ticks %llu mix_calls %llu mix_frames %llu active_ch %llu pcm8_ch %llu pcm16_ch %llu adpcm_ch %llu psg_ch %llu cap_inst %llu dep_ch %llu",
    src/graphics/renderer.c:117:        kj_perf_present_skipped(in->game_frame);
    src/graphics/renderer.c:121:    kj_perf_publish_begin();
    src/graphics/renderer.c:123:    uint64_t t = kj_perf_now();
    src/graphics/renderer.c:125:    kj_perf_add(KJ_PERF_GPU_WAIT, kj_perf_now() - t);
    src/graphics/renderer.c:137:    t = kj_perf_now();
    src/graphics/renderer.c:139:    kj_perf_add(KJ_PERF_SUBMIT, kj_perf_now() - t);
    src/graphics/renderer.c:154:    kj_perf_publish_end(in->game_frame);
    src/graphics/renderer.c:217:            kj_log("[gputime] 600 pictures: GSP us/picture P3D %lu PPF %lu PSC %lu (log %s); "
    src/runtime/kj_g3.c:1067:extern u64 svcGetSystemTick(void);
    src/runtime/kj_g3.c:1073:    const u64 t0 = svcGetSystemTick();
    src/runtime/kj_g3.c:1075:    s_op_ticks[op & 255] += svcGetSystemTick() - t0;
    src/runtime/kj_g3.c:1328:extern u64 svcGetSystemTick(void);
    src/runtime/kj_g3.c:2044:static void dl_replay(dl_entry *e)
    src/runtime/kj_g3.c:2145:    const u64 t0 = svcGetSystemTick();
    src/runtime/kj_g3.c:2186:    s_fifo_ticks += svcGetSystemTick() - t0;
    src/runtime/kj_g3.c:2190:void NitroPort_GxFifoWords(const uint32_t *words, size_t count)
    src/runtime/kj_g3.c:2274:            dl_replay(e);
    src/app\main.c:97:    kj_perf_vblank_begin();
    src/app\main.c:128:    kj_perf_vblank_end();
    src/app\main.c:211:        const u64 t0 = svcGetSystemTick();
    src/app\main.c:216:        const u64 t1 = svcGetSystemTick();
    # Performance engineering for 3DS homebrew
    
    Making a 3DS program fast is mostly about finding out *which* part is slow before changing anything. The console has a slow CPU (268 MHz on an Old 3DS; **New 3DS:** up to 804 MHz), a capable but unusual GPU, and a small amount of fast memory; a frame can be late because the CPU ran out of time, because the GPU did, because one waited for the other, or because something stalled on a file or a system service. This page shows how to measure each of those with the libraries homebrew already uses, how to pace frames, how to use the second CPU core, and which CPU-side habits pay off on this processor.
    
    Related pages: [memory and cache](memory-and-cache.md) (where buffers live and what flushing costs), [GPU rendering](PICA200/gpu-rendering.md) (draw-call and state costs), [testing on hardware](testing-on-hardware.md) (how to run a benchmark you can trust).
    
    ## Before you start
    
    - Checked against libctru 2.7.1, citro3d 1.7.1, devkitARM r68 and Luma3DS v13.5 source.
    - Claims carry the [evidence labels](../../AGENTS.md#evidence-labels) used across the wiki. Nothing on this page is `[measured]` by the wiki itself yet; where a number comes from someone else's measurement, it is labelled with its origin.
    - Decide your target hardware first. An [Old 3DS](../glossary.md#old-new-3ds) has two slow cores and no L2 cache. **New 3DS:** the CPU can run 3Ã— faster with a 2 MB L2 cache, and a third core is open to `.3dsx` homebrew. A program tuned on a New 3DS tells you nothing about an Old 3DS.
    
    ## The budget
    
    At 60 frames per second a frame lasts 16.7 ms. The ARM11 application core on an Old 3DS runs at 268,111,856 Hz ([CPU](../hardware/cpu.md#clocks-and-core-counts)), so one frame is about 4.47 million clock cycles â€” before the operating system, cache misses, waiting for the GPU and interrupt handling take their share.
    
    > **New 3DS:** with the fast mode enabled the core runs at 804 MHz (3Ã—) with a 2 MB L2 cache, so a frame is about 13.4 million cycles at 60 fps.
    
    | Target | Frame time | Old 3DS core-0 cycles per frame |
    | ------ | ---------- | ------------------------------- |
    | 60 fps | 16.7 ms | â‰ˆ 4.47 M |
    | 30 fps | 33.3 ms | â‰ˆ 8.94 M |
    | 20 fps | 50 ms | â‰ˆ 13.4 M |
    
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
    # Files and saves in 3DS homebrew
    
    Where can a 3DS program read assets and keep its progress, and how do you write a save file that survives power loss, a full SD card and a bad write? This page covers the four storage areas libctru exposes (SD card, bundled read-only files, save data, extended data), the file APIs for each, the commit rule for save data, how failures show up, and a complete two-copy save manager with a version header and checksums.
    
    The short answer: homebrew `.3dsx` programs use the SD card with ordinary `fopen`; installed `.cia` titles can also have real save data, which needs an explicit commit. Never overwrite your only good save in place; write the other copy, check every return value, and let a checksum decide which copy is valid at load time.
    
    ## The whole thing in 10 lines
    
    1. A `.3dsx` program should keep its data on the SD card under `sdmc:/3ds/<yourapp>/` with plain `fopen`.
    2. Bundled read-only assets go in RomFS (`romfsInit()`, then `romfs:/...`).
    3. Real save data and extdata belong to installed titles (`.cia`); save data needs `archiveCommitSaveData()`.
    4. Extdata cannot be written through `fopen` (`ENOTSUP`); use `FSUSER_*`/`FSFILE_*`.
    5. Never overwrite your only good save: keep two slots, write the older one.
    6. Put a magic number, a format version, a generation counter, the size and CRC-32s in a header.
    7. Check `fopen`, `fwrite`, `fflush`, `fsync` and `fclose`; any failure means "this save failed".
    8. At load, read both slots, drop invalid ones, keep the highest generation.
    9. Do not rely on `rename` being crash-safe; nothing in libctru promises it.
    10. Save from the main thread, before the loop ends; never in an exit hook ([lifecycle](application-lifecycle.md#exit)).
    
    ## Before you start
    
    - Checked against libctru 2.7.1 at the commit pinned on [source versions](source-versions.md) (`62b7c42c88b1ee837f513358409d2e5c8f9001a9`), devkitARM r68. Source links are at that commit.
    - Nothing here was run on hardware or an emulator; see [Verification status](#verification-status).
    - Background: the on-card layout and encryption are on [SD filesystem](../software/sd-filesystem.md), the save container format on [savegames](../software/savegames.md), the file service on [services](../software/services.md), the library on [libctru](../homebrew/libctru.md). Lifecycle interaction (when to save) is on [application lifecycle](application-lifecycle.md).
    - Terms: an **[archive](../glossary.md#fs-archive)** is one mounted storage area opened through the file service; **[RomFS](../glossary.md#romfs)** is the read-only file tree inside a program; **[save data](../glossary.md#save-data)** is the per-title private storage; **[extdata](../glossary.md#extdata)** is the larger shared storage a title can create on the SD card.
    
    ## Picking a storage area
    
    | Area | libctru device | Writable from `fopen`? | Needs a title (`.cia`)? | Use for |
    | ---- | -------------- | ---------------------- | ----------------------- | ------- |
    | SD card | `sdmc:` (mounted automatically at startup) | Yes | No | Homebrew saves, configs, logs, user-supplied files |
    | RomFS | `romfs:` after `romfsInit()` | No (read-only) | No: a `.3dsx` can embed one | Bundled assets |
    | Save data | any name you give `archiveMount`, e.g. `save:` | Yes, plus a commit | Yes: a title with a save, not a `.3dsx` `[hypothesis]` | Progress of an installed title |
    | Extdata | a name you mount | **No through stdio**; use `FSUSER_*` / `FSFILE_*` | A title creates it | Large shared data, DLC-like content |
    
    Facts behind the table [documented: `libctru/source/system/appInit.c` (SD mount), `libctru/include/3ds/romfs.h`, `libctru/source/archive_dev.c`]:
    
    - `__appInit` calls `fsInit()` then `archiveMountSdmc()`, so `sdmc:/...` works from `main` with no setup. `__appExit` calls `archiveUnmountAll()` and `fsExit()` before `aptExit()`.
    - `romfsMountSelf(name)` mounts your own RomFS: for a `.3dsx` it is the RomFS section embedded in the file; for a title it is the title's RomFS. `romfsInit()` is the wrapper using the name `romfs`.
    - `archiveMount(archiveID, archivePath, deviceName)` opens any archive and registers a stdio device. When the archive id is `ARCHIVE_EXTDATA`, `ARCHIVE_SHARED_EXTDATA`, `ARCHIVE_BOSS_EXTDATA` or `ARCHIVE_EXTDATA_AND_BOSS_EXTDATA` the device is flagged `is_extdata` and **opening for write, creating, renaming and the other mutating calls return `ENOTSUP`** (the source comment calls extdata "weird"). Write extdata through `FSUSER_OpenFile`/`FSFILE_Write`.
    - A `.3dsx` has no title of its own, so whether `ARCHIVE_SAVEDATA` opens for it is not established by the files read `[hypothesis]`: test before relying on it. The SD card is the portable choice.
    
    A homebrew-friendly SD layout is `sdmc:/3ds/<yourapp>/`. Create the folders with `mkdir` (it fails with `EEXIST` if they exist; that is fine).
    
    ## Reading and writing with stdio
    
    `fopen`, `fread`, `fwrite`, `fclose`, `fsync`, `rename`, `mkdir`, `stat`, `opendir` all work on mounted devices through libctru's archive device layer [documented: `archive_dev.c`]. What each call does underneath:
    
    | Call | Underlying FS call | Notes |
    | ---- | ------------------ | ----- |
    | `fopen(p, "wb")` | open with write+create, then `FSFILE_SetSize(fd, 0)` for `O_TRUNC` | The old contents are gone as soon as the open returns; a crash between truncation and the end of your write leaves a short or empty file |
    | `fwrite` | `FSFILE_Write(handle, &n, offset, buf, len, flags)` | `flags` is `FS_WRITE_FLUSH \| FS_WRITE_UPDATE_TIME` only for files opened `O_SYNC`; otherwise `0`. The returned count is the bytes the service reported written |
    | `fsync(fileno(f))` | `FSFILE_Flush` | Header text: "Flushes a file's contents" (`fs.h`) |
    | `fclose` | `FSFILE_Close` | Can fail; check it |
    | `rename(a, b)` | `FSUSER_RenameFile`; if the target exists it deletes the target and renames again; if that fails it tries directory rename | The delete-then-rename fallback means **two operations with a gap between them** |
    | `mkdir`, `unlink`, `stat` | `FSUSER_CreateDirectory`, `FSUSER_DeleteFile`, â€¦ | |
    
    Note that stdio's own buffer sits in front of this: call `fflush` before `fsync`, as in the example.
    
    ### Checking errors
    
    FS calls return a libctru `Result`; stdio converts it to `errno`. The table in `archive_dev.c` maps only a few codes (`0x082044BE`â†’`EEXIST`, `0x086044D2`â†’`ENOSPC`, `0xC8804478` and `0xC92044FA`â†’`ENOENT`, `0xE0E046BE`â†’`EINVAL`, `0xE0E046BF`â†’`ENAMETOOLONG`) [documented: `archive_dev.c` `error_table`]. Anything else becomes a generic errno, so for diagnostics log the raw `Result` when you call `FSUSER_*` directly. Disk full (`ENOSPC`) shows up on a write or on `fclose`/`fflush`; treat any failed step as a failed save, not only a short `fwrite`.
    
    ### Short writes
    
    `fwrite` returning fewer items than requested means the write failed part-way. The example below treats `put != len` as failure and also checks `fflush`, `fsync` and `fclose`. Do not retry a short write blindly at the same offset without re-seeking; simplest is to abandon this slot and report failure, because the other slot is still intact.
    
    ## Committing save data
    
    Save-data archives buffer changes until you commit them. libctru's `archiveCommitSaveData(deviceName)` runs `FSUSER_ControlArchive(archive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0)`, and the header states: "Not done automatically at unmount" [documented: `archive.h`, `archive_dev.c`, `fs.h` (`ARCHIVE_ACTION_COMMIT_SAVE_DATA = 0`, "Commits save data changes")]. The header documents that unmounting does not commit; libctru's `fclose` path only calls `FSFILE_Close`, which does not mention a commit either [documented: `archive.h`, `archive_dev.c`]. So:
       198.563 [sndprof] render_calls 12600 render_frames 6451200 stretches 50323 sndticks 37791 mix_ticks 9413654828 tick_ticks 707723168 mix_calls 50323 mix_frames 6451200 active_ch 565848 pcm8_ch 273 pcm16_ch 0 adpcm_ch 564620 psg_ch 955 cap_inst 0 dep_ch 0
       206.504 [nitro] overlay OVY_120 loaded
       207.935 [sndprof] render_calls 13200 render_frames 6758400 stretches 52714 sndticks 39590 mix_ticks 9895781462 tick_ticks 743285468 mix_calls 52714 mix_frames 6758400 active_ch 594090 pcm8_ch 273 pcm16_ch 0 adpcm_ch 591686 psg_ch 2131 cap_inst 0 dep_ch 0
       209.518 [nitro] overlay ds_protect loaded
       209.522 [nitro] overlay ds_protect loaded
       209.532 [nitro] overlay OVY_12 loaded
       209.532 [nitro] overlay OVY_18 loaded
       209.551 [nitro] overlay OVY_6 loaded
       209.551 [nitro] overlay OVY_7 loaded
       215.470 [nitro] overlay OVY_10 loaded
       216.221 [nitro] overlay OVY_7 loaded
       217.312 [sndprof] render_calls 13800 render_frames 7065600 stretches 55114 sndticks 41390 mix_ticks 10428890638 tick_ticks 784248184 mix_calls 55114 mix_frames 7065600 active_ch 624860 pcm8_ch 512 pcm16_ch 0 adpcm_ch 621104 psg_ch 3244 cap_inst 0 dep_ch 0
       219.721 [nitro] overlay field loaded
       219.721 [nitro] overlay ds_protect loaded
       219.721 [nitro] overlay OVY_2 loaded
       219.721 [nitro] overlay OVY_3 loaded
       219.811 [nitro] overlay OVY_27 loaded
       230.276 [kj] close requested at V-blank 7533
       230.276 [app] terminating after 7532 V-blanks (close requested)
       230.277 [audio] stopped, 1 underruns
       218.227 [nitro] overlay OVY_7 loaded
       219.173 [sndprof] render_calls 13800 render_frames 7065600 stretches 55114 sndticks 41390 mix_ticks 10433895230 tick_ticks 806407110 mix_calls 55114 mix_frames 7065600 active_ch 624216 pcm8_ch 511 pcm16_ch 0 adpcm_ch 621030 psg_ch 2675 cap_inst 0 dep_ch 0
       220.624 [nitro] overlay field loaded
       220.624 [nitro] overlay ds_protect loaded
       220.624 [nitro] overlay OVY_2 loaded
       220.624 [nitro] overlay OVY_3 loaded
       220.713 [nitro] overlay OVY_27 loaded
       233.343 [kj] close requested at V-blank 7430
       233.343 [app] terminating after 7429 V-blanks (close requested)
       233.350 [audio] stopped, 1 underruns
    ✓ • 1.41s

## Activity

    $ Get-Content src/runtime/kj_g3.c | Select-Object -Skip 2030 -First 280; Get-Content src/runtime/kj_frame.c | Select-Object -First 160; Get-Content src/compat/nitro/src/port_snd.c | Select-Object -First 75; Get-Content src/compat/nitro/src/port_sound_chip.c | Select-Object -Skip 855 -First 55; Get-Content src/graphics/renderer.c | Select-Object -Skip 150 -First 85; rg -n 'HGSS_PROFILE|define|Makefile|build-info' tools/game/hgss_game.py src/compat/nitro/Makefile | Select-Object -Last 50
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
    
    static void dl_replay(dl_entry *e)
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
            ++s_dl_hits;
        }
    }
    #endif
    
    static void decode_words(const uint32_t *words, size_t count)
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
    
    void NitroPort_GxFifoWords(const uint32_t *words, size_t count)
    {
        ++s_fifo_calls;
        s_fifo_words += (u32)count;
        if (s_param_count[0x16] == 0) {
            build_param_table();
        }
    #ifndef KJ_DL_CACHE
        /* KJ_DL_NO_CACHE: every list decoded live. */
        decode_words(words, count);
        return;
    #endif
        if (count < DL_MIN_WORDS || count > 0xFFFFFu || s_in_begin || s_fifo_need > 0 || s_rec.active) {
            s_dl_cov[DLC_SHORT] += (uint32_t)count;
            if (count < DL_MIN_WORDS) {
                s_dl_cov[DLC_SMALL] += (uint32_t)count;
            } else if (s_fifo_need > 0) {
                s_dl_cov[DLC_SPLIT] += (uint32_t)count;
            } else if (s_in_begin) {
                s_dl_cov[DLC_BEGIN] += (uint32_t)count;
                if ((words[0] & 0xFFu) == 0x40u) {
                    s_dl_cov[DLC_BEGIN_FIRST] += (uint32_t)count;
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
            s_dl_cov[DLC_NEW] += (uint32_t)count;
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
            s_dl_cov[DLC_NEVER] += (uint32_t)count;
            ++s_dl_uncacheable;
            decode_words(words, count);
            return;
        } else if (memcmp(e->words, words, count * 4u) != 0) {
            s_dl_cov[DLC_CHANGED] += (uint32_t)count;
            memcpy(e->words, words, count * 4u);
            dl_drop_recording(e);
            e->never = false;
            e->needs_light = false;
            e->reads = dl_entry_reads(words, count);
        }
        e->last_frame = s_stats.swaps;
        if (e->never) {
            s_dl_cov[DLC_NEVER] += (uint32_t)count;
            ++s_dl_uncacheable;
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
                ++s_dl_hits;
                s_dl_cov[DLC_HIT] += (uint32_t)count;
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
            s_dl_cov[DLC_HIT] += (uint32_t)count;
        } else if (e->recorded) {
            s_dl_cov[DLC_KEY] += (uint32_t)count;
        }
        ++s_dl_misses;
        if (e->recorded && !verify) {
            dl_drop_recording(e);
        }
        frame_buf *fb = &s_buf[s_build];
        /* reset every scalar field (the arrays need no clearing; nevents and
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
            const u64 t0 = svcGetSystemTick();
            if (NitroPort_HBlankLinesOnce()) {
                ++s_hblank_walks;
            }
            s_hblank_ticks += svcGetSystemTick() - t0;
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
            const u64 t0 = svcGetSystemTick();
            NitroPort_SoundPumpReplies();
            s_pump_ticks += svcGetSystemTick() - t0;
            ++s_pump_calls;
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
    /*
     * Real NitroSDK ARM7 sequencer, running on an independent audio clock.
     *
     * The ARM9 game's PXI sender only enqueues immutable command-list pointers;
     * the audio producer executes the ORIGINAL SDK ARM7 command processor,
     * sequencer, envelopes and channel allocation. These symbols are in the
     * N7_ namespace, generated by objcopy --redefine-syms in the Makefile.
     *
     * No ARM9 OS calls, scheduler, PXI interrupts or libctru are made from
     * NitroPort_SoundRender. Alarm replies are queued for the game/platform
     * thread to deliver by NitroPort_SoundPumpReplies.
     */
    #include <nitro.h>
    #include <nitro_port.h>
    #include <nitro_port_sound_chip.h>
    #include "port_platform.h"
    #include <nitro/snd/common/command.h>
    #include <nitro/snd/common/work.h>
    #include <nitro/snd/common/main.h>
    
    #include <stdint.h>
    #include <string.h>
    
    /* COUNT-HEAVY profile ticks: the 3DS's tick counter (port_os.c's pattern:
     * the port never includes libctru's headers, so the one prototype it needs
     * is declared here). Host builds have no counter: the Mix-vs-tick split
     * reads 0 there and only the counts are exact. Emulator ticks are mushy
     * (JIT/preemption inflate them); hardware ticks are true. */
    #ifdef __3DS__
    extern u64 svcGetSystemTick(void);
    #define SNDPROF_NOW() svcGetSystemTick()
    #else
    #define SNDPROF_NOW() ((u64)0)
    #endif
    
    #define SOUND_QUEUE_CAPACITY 32u
    #define SOUND_ALARM_CAPACITY 128u
    /* The ARM7 driver's tick: every SND_PROC_INTERVAL (2728) OS ticks, an OS
     * tick being 64 cycles of the 33.513982 MHz bus clock: 5.2095 ms, about 192
     * a second. In samples: 2728 * 64 * 32768 / 33513982 = 170.7. The phase
     * below counts in 1/33513982ths of a sample. (It was taken for 2728
     * microseconds once, and music played 1.9 times too fast.) */
    #define SOUND_BUS_CLOCK 33513982ull
    #define SOUND_TICK_SAMPLES ((uint64_t)SND_PROC_INTERVAL * 64u * NITRO_SOUND_RATE)
    
    /* The ARM7 copy of the SDK's own functions and shared globals. */
    extern void N7_SND_Init(u32 priority);
    extern void N7_SND_CommandProc(void);
    extern void N7_SND_UpdateExChannel(void);
    extern void N7_SND_SeqMain(BOOL periodic);
    extern void N7_SND_ExChannelMain(BOOL periodic);
    extern void N7_SND_UpdateSharedWork(void);
    extern u16 N7_SND_CalcRandom(void);
    /* Cumulative mixer counters owned by port_sound_chip.c (file-static there);
     * declared here, not in the shared header, so no header changes. formatCh
     * takes 4 entries, one per sample format. */
    extern void NitroSound_MixProfile(uint64_t *calls, uint64_t *frames,
                                      uint64_t *activeCh, uint64_t formatCh[4],
                                      uint64_t *capturing, uint64_t *dependentCh);
    
    static NitroSoundChip sChip;
    static volatile unsigned int sLock;
    static volatile int sReady;
    static const SNDCommand *sPending[SOUND_QUEUE_CAPACITY];
    static unsigned sHead, sCount;
    static u32 sAlarmReplies[SOUND_ALARM_CAPACITY];
    static unsigned sAlarmHead, sAlarmCount;
    static uint64_t sClock, sTickPhase;
    
    /* COUNT-HEAVY profile: cumulative counters so one device run can name the
     * ~25% core-0 audio cost (format decode vs capture/reverb vs sequencer
     * tick). Counters plus one periodic log line only; no mixing math touched.
     * Written and read on the audio thread only (NitroPort_SoundRender below),
     * so plain integers, no atomics. */
    static uint64_t sProfRenderCalls;   /* SoundRender calls that reached the mix loop */
            /* The next mix starts it again (start_channel). */
            chip->voice[channel].running = 0;
        }
    }
    
    /* COUNT-HEAVY profile: cumulative counters so one device run can name the
     * mixer's cost (per-sample format decode vs the capture/reverb path, with
     * the sequencer split off in port_snd.c). Counters only; no mixing math
     * touched. The mixer runs under the audio lock on one thread (tests are
     * single-threaded), so plain integers, no atomics. port_snd.c reads these
     * for its [sndprof] line via NitroSound_MixProfile below. */
    static uint64_t sProfMixCalls;      /* NitroSound_Mix calls that mixed (past the guard) */
    static uint64_t sProfMixFrames;     /* output frames, summed over those calls */
    static uint64_t sProfActiveCh;      /* active channels, summed over calls (mean: / calls) */
    static uint64_t sProfFormatCh[4];   /* active channels by sample format: 0 PCM8, 1 PCM16, 2 ADPCM, 3 PSG/noise */
    static uint64_t sProfCapturing;     /* capture instances running, summed over calls (0-2 each) */
    static uint64_t sProfDependentCh;   /* channels reading a capture buffer, summed over calls */
    
    void NitroSound_Mix(NitroSoundChip *chip, int16_t *stereo, size_t frames)
    {
        Channel channels[NITRO_SOUND_CHANNEL_COUNT];
        unsigned count = 0, channel, k;
        unsigned control;
        int32_t masterVolume;
        int capturing, bypass1, bypass3;
        unsigned leftFrom, rightFrom;
        size_t i;
    #ifdef NITRO_SOUND_STATS
        int32_t tapBuf[NITRO_SOUND_CHANNEL_COUNT][MIX_BLOCK];
    #endif
    
        if (!chip || !stereo || !chip->outputRate || frames == 0) return;
        for (channel = 0; channel < NITRO_SOUND_CHANNEL_COUNT; ++channel) {
            if (start_channel(chip, channel, &channels[count])) ++count;
        }
        sProfMixCalls += 1u;
        sProfMixFrames += (uint64_t)frames;
        sProfActiveCh += (uint64_t)count;
        for (k = 0; k < count; ++k) sProfFormatCh[channels[k].format] += 1u;
    #ifdef NITRO_SOUND_STATS
        for (k = 0; k < count; ++k) channels[k].tap = tapBuf[channels[k].channel];
    #endif
        /* SOUNDCNT (0x500): bits 0-6 master volume, 8-9 / 10-11 what the left /
         * right output plays (the mixer, channel 1, channel 3 or both), 12 / 13
         * channel 1 / 3 kept out of the mixer, 15 on. */
        control = reg8(0x501u);
        masterVolume = (int32_t)(reg8(0x500u) & 127u);
        leftFrom = control & 3u;
        rightFrom = (control >> 2) & 3u;
        bypass1 = (control & 0x10u) != 0;
        bypass3 = (control & 0x20u) != 0;
        capturing = ((reg8(0x508u) | reg8(0x509u)) & 128u) ||
                    chip->capture[0].running || chip->capture[1].running;
        for (channel = 0; channel < 2u; ++channel) {
            if ((reg8(0x508u + channel) & 128u) || chip->capture[channel].running)
        }
    
        ++s_frames;
        kj_perf_publish_end(in->game_frame);
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
    
        /* Test directives: deterministic per-frame capture. */
        if (kj_script_dump(in->game_frame)) {
            char path[96];
            mkdir("sdmc:/3ds/kohjoh/dumps", 0777);
            snprintf(path, sizeof(path), "sdmc:/3ds/kohjoh/dumps/f%05lu.kjd", (unsigned long)in->game_frame);
            kj_dump_frame(path, in);
        }
        const uint32_t hold = kj_script_freeze(in->game_frame);
        if (hold > 0) {
    src/compat/nitro/Makefile:162:# and cross-object references are renamed with objcopy --redefine-syms. This
    src/compat/nitro/Makefile:180:ARM7_MAP := $(GEN)/arm7_sound_redefine.syms
    src/compat/nitro/Makefile:285:    @$(NM) -g --defined-only $(ARM7_RAW) | awk 'NF == 3 && $$2 ~ /^[TDBRCSG]$$/ { print $$3 " N7_" $$3 }' | sort -u > $@
    src/compat/nitro/Makefile:290:    @$(OBJCOPY) --redefine-syms=$(ARM7_MAP) $@
    tools/game/hgss_game.py:10:`--game` selects the decomp's HEARTGOLD (the default) or SOULSILVER define and
    tools/game/hgss_game.py:128:#: The two games this tree builds. `define` is what the decomp's config.h
    tools/game/hgss_game.py:133:        "define": "HEARTGOLD",
    tools/game/hgss_game.py:139:        "define": "SOULSILVER",
    tools/game/hgss_game.py:148:def game_defines() -> list[str]:
    tools/game/hgss_game.py:150:    return ["-D" + GAME["define"]]
    tools/game/hgss_game.py:160:UNDEFINED_RE = re.compile(r"undefined reference to [`']([^']+)'")
    tools/game/hgss_game.py:214:    # kohjoh default is OFF; HGSS_PROFILE=on compiles the PROFILE build
    tools/game/hgss_game.py:218:    # define; everything else (flags, commit, inputs) stays identical.
    tools/game/hgss_game.py:219:    # The same define must reach libnitro.a/libnitrosystem.a, which is why
    tools/game/hgss_game.py:222:        flags += ["-DHGSS_PROFILE_OFF"]
    tools/game/hgss_game.py:225:    # Normal builds never define it, so the chainloading code cannot end
    tools/game/hgss_game.py:248:    # kohjoh diagnostic switches (--define NAME): MSYS2 Python does not see
    tools/game/hgss_game.py:253:    flags += game_defines()
    tools/game/hgss_game.py:287:SANITIZE_FLAGS = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
    tools/game/hgss_game.py:372:            "-Wno-unused-function", *ARM_FLAGS, *game_defines(),
    tools/game/hgss_game.py:557:    return machine + PORT_DEFINES + game_defines() + include_flags(root, out)
    tools/game/hgss_game.py:576:# cpp_runtime.c defines these two destructors itself. The decomp's maps alias
    tools/game/hgss_game.py:642:def defined_symbol_size(nm: str, obj: Path, name: str) -> int | None:
    tools/game/hgss_game.py:643:    """The ELF size of one global defined symbol, or None if absent."""
    tools/game/hgss_game.py:644:    proc = run([nm, "-S", "-g", "--defined-only", str(obj)])
    tools/game/hgss_game.py:662:    """(section name, section-relative offset, size) of one defined symbol.
    tools/game/hgss_game.py:698:    - Entries whose alias cpp_runtime.c already defines are dropped (see
    tools/game/hgss_game.py:941:    relocating it (Luma: undefined instruction in process "loader"). The
    tools/game/hgss_game.py:1067:            f'#define HGSS_BUILD_ID "{build_id}"\n'
    tools/game/hgss_game.py:1068:            f'#define HGSS_BUILD_SHA "{full}"\n'
    tools/game/hgss_game.py:1069:            f'#define HGSS_BUILD_DIRTY {1 if dirty else 0}\n')
    tools/game/hgss_game.py:1079:    """Write the .build-info.txt sidecar next to a packaged .3dsx.
    tools/game/hgss_game.py:1086:    manifest = dsx.with_name(dsx.stem + ".build-info.txt")
    tools/game/hgss_game.py:1203:                 "-Wl,--undefined=kj_game_main", "-Wl,--undefined=NitroMain",
    tools/game/hgss_game.py:1223:    """Unique undefined symbols in a linker's output, in first-seen order."""
    tools/game/hgss_game.py:1268:        proc = run([nm, "-S", "--defined-only", str(binary)])
    tools/game/hgss_game.py:1294:    return False, f"{NNS_VOLUME_SYMBOL} is not defined"
    tools/game/hgss_game.py:1469:    """Make every symbol defined in more than one object weak, once.
    tools/game/hgss_game.py:1471:    The original game is one program, so a global defined twice can only be
    tools/game/hgss_game.py:1486:        proc = subprocess.run([nm, "-g", "--defined-only",
    tools/game/hgss_game.py:1530:    return "profile" if os.environ.get("HGSS_PROFILE") == "on" else "off"
    tools/game/hgss_game.py:1590:        # HGSS_PROFILE toggles the 2D tick reads (port_render2d.c) and the
    tools/game/hgss_game.py:1630:                    "the decomp's HEARTGOLD/SOULSILVER define and the program name")
    tools/game/hgss_game.py:1640:    ap.add_argument("--define", action="append", default=[],
    tools/game/hgss_game.py:1641:                    help="extra preprocessor define for the runtime and game (e.g. KJ_G3_PROFILE)")
    tools/game/hgss_game.py:1643:                    help="host32 only: build with -fsanitize=address,undefined")
    tools/game/hgss_game.py:1648:    EXTRA_DEFINES.extend(args.define)
    tools/game/hgss_game.py:1680:    # Commit identity for the boot banner + build-info sidecar (log.txt
    tools/game/hgss_game.py:1782:    print(f"   {len(duplicates)} symbols defined in more than one object "
    ✓ • 1.36s

## Activity

    $ Get-Content src/diagnostics/perf.c -TotalCount 100; Get-Content src/app/main.c | Select-Object -Skip 195 -First 35; Get-Content src/graphics/renderer.c -TotalCount 115; rg -n 'sProf|SNDPROF_NOW' src/compat/nitro/src/port_snd.c src/compat/nitro/src/port_sound_chip.c
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
        memset(&settings, 0, sizeof(settings));
        {
            static const char name[] = "Player";
            for (unsigned i = 0; name[i] != '\0'; ++i) {
                settings.nickname[i] = (uint16_t)name[i];
            }
        }
        settings.language = 1; /* English */
        settings.birth_month = 1;
        settings.birth_day = 1;
        {
            /* Calibration: ticks for a loop of known instruction count (4 per
             * iteration: add, subs, bne + one store), to read emulator timing. */
            volatile u32 sink = 0;
            u32 acc = 0;
            const u64 t0 = svcGetSystemTick();
            for (u32 i = 1000000; i != 0; --i) {
                acc += i;
                sink = acc;
            }
            const u64 t1 = svcGetSystemTick();
            kj_log("[calib] 1M-iteration loop: %llu ticks (%.2f ticks/iteration)",
                   (unsigned long long)(t1 - t0), (double)(t1 - t0) / 1000000.0);
            (void)sink;
        }
        s_base = C3D_FrameCounter(0);
        kj_log("[app] starting the game");
        kj_game_main(&settings);
        return 0;
    }
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
        s_perflog_on = R_SUCCEEDED(GSPGPU_SetPerfLogMode(true)) &&
                       R_SUCCEEDED(GSPGPU_GetPerfLog(&s_perflog_prev));
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
    src/compat/nitro/src/port_sound_chip.c:867:static uint64_t sProfMixCalls;      /* NitroSound_Mix calls that mixed (past the guard) */
    src/compat/nitro/src/port_sound_chip.c:868:static uint64_t sProfMixFrames;     /* output frames, summed over those calls */
    src/compat/nitro/src/port_sound_chip.c:869:static uint64_t sProfActiveCh;      /* active channels, summed over calls (mean: / calls) */
    src/compat/nitro/src/port_sound_chip.c:870:static uint64_t sProfFormatCh[4];   /* active channels by sample format: 0 PCM8, 1 PCM16, 2 ADPCM, 3 PSG/noise */
    src/compat/nitro/src/port_sound_chip.c:871:static uint64_t sProfCapturing;     /* capture instances running, summed over calls (0-2 each) */
    src/compat/nitro/src/port_sound_chip.c:872:static uint64_t sProfDependentCh;   /* channels reading a capture buffer, summed over calls */
    src/compat/nitro/src/port_sound_chip.c:891:    sProfMixCalls += 1u;
    src/compat/nitro/src/port_sound_chip.c:892:    sProfMixFrames += (uint64_t)frames;
    src/compat/nitro/src/port_sound_chip.c:893:    sProfActiveCh += (uint64_t)count;
    src/compat/nitro/src/port_sound_chip.c:894:    for (k = 0; k < count; ++k) sProfFormatCh[channels[k].format] += 1u;
    src/compat/nitro/src/port_sound_chip.c:911:            sProfCapturing += 1u;
    src/compat/nitro/src/port_sound_chip.c:930:                    sProfDependentCh += 1u;
    src/compat/nitro/src/port_sound_chip.c:1065: * 4 entries, one per sample format (see sProfFormatCh). */
    src/compat/nitro/src/port_sound_chip.c:1071:    if (calls) *calls = sProfMixCalls;
    src/compat/nitro/src/port_sound_chip.c:1072:    if (frames) *frames = sProfMixFrames;
    src/compat/nitro/src/port_sound_chip.c:1073:    if (activeCh) *activeCh = sProfActiveCh;
    src/compat/nitro/src/port_sound_chip.c:1075:        for (i = 0; i < 4u; ++i) formatCh[i] = sProfFormatCh[i];
    src/compat/nitro/src/port_sound_chip.c:1077:    if (capturing) *capturing = sProfCapturing;
    src/compat/nitro/src/port_sound_chip.c:1078:    if (dependentCh) *dependentCh = sProfDependentCh;
    src/compat/nitro/src/port_snd.c:31:#define SNDPROF_NOW() svcGetSystemTick()
    src/compat/nitro/src/port_snd.c:33:#define SNDPROF_NOW() ((u64)0)
    src/compat/nitro/src/port_snd.c:75:static uint64_t sProfRenderCalls;   /* SoundRender calls that reached the mix loop */
    src/compat/nitro/src/port_snd.c:76:static uint64_t sProfRenderFrames;  /* output frames requested, summed over those calls */
    src/compat/nitro/src/port_snd.c:77:static uint64_t sProfStretches;     /* mix-loop iterations (one lock hold + one Mix each) */
    src/compat/nitro/src/port_snd.c:78:static uint64_t sProfSoundTicks;    /* sequencer sound_tick executions */
    src/compat/nitro/src/port_snd.c:79:static uint64_t sProfMixTicks;      /* ARM11 ticks inside NitroSound_Mix (__3DS__ only; 0 on host) */
    src/compat/nitro/src/port_snd.c:80:static uint64_t sProfTickTicks;     /* ARM11 ticks inside sound_tick (__3DS__ only; 0 on host) */
    src/compat/nitro/src/port_snd.c:250:    sProfRenderCalls += 1u;
    src/compat/nitro/src/port_snd.c:251:    sProfRenderFrames += (uint64_t)frames;
    src/compat/nitro/src/port_snd.c:266:            u64 mixStart = SNDPROF_NOW();
    src/compat/nitro/src/port_snd.c:268:            sProfMixTicks += (uint64_t)(SNDPROF_NOW() - mixStart);
    src/compat/nitro/src/port_snd.c:274:                u64 tickStart = SNDPROF_NOW();
    src/compat/nitro/src/port_snd.c:276:                sProfTickTicks += (uint64_t)(SNDPROF_NOW() - tickStart);
    src/compat/nitro/src/port_snd.c:278:            sProfSoundTicks += 1u;
    src/compat/nitro/src/port_snd.c:282:        sProfStretches += 1u;
    src/compat/nitro/src/port_snd.c:287:    if (sProfRenderCalls % 600u == 0u) {
    src/compat/nitro/src/port_snd.c:293:                      (unsigned long long)sProfRenderCalls,
    src/compat/nitro/src/port_snd.c:294:                      (unsigned long long)sProfRenderFrames,
    src/compat/nitro/src/port_snd.c:295:                      (unsigned long long)sProfStretches,
    src/compat/nitro/src/port_snd.c:296:                      (unsigned long long)sProfSoundTicks,
    src/compat/nitro/src/port_snd.c:297:                      (unsigned long long)sProfMixTicks,
    src/compat/nitro/src/port_snd.c:298:                      (unsigned long long)sProfTickTicks,
    ✓ • 1.36s

## Activity

    $ (Get-Content src/diagnostics/log.c -Raw).Replace('fopen(path, "w")','fopen(path, "a")') | Set-Content src/diagnostics/log.c; (Get-Content src/diagnostics/crash.c -Raw).Replace('fopen(s_crash_path, "w")','fopen(s_crash_path, "a")') | Set-Content src/diagnostics/crash.c; git diff --stat; git diff -- src/diagnostics/log.c src/diagnostics/crash.c
     README.md               | 11 ++++++-----
     src/diagnostics/crash.c |  3 ++-
     src/diagnostics/log.c   |  3 ++-
     3 files changed, 10 insertions(+), 7 deletions(-)
    diff --git a/src/diagnostics/crash.c b/src/diagnostics/crash.c
    index b6cd8fc..c2b97c4 100644
    --- a/src/diagnostics/crash.c
    +++ b/src/diagnostics/crash.c
    @@ -70,7 +70,7 @@ static void handler(ERRF_ExceptionInfo *excep, CpuRegisters *regs)
             /* The crash file first: it needs no lock the faulting thread could
              * hold (the log ring does). */
             if (s_crash_path[0] != '\0') {
    -            FILE *f = fopen(s_crash_path, "w");
    +            FILE *f = fopen(s_crash_path, "a");
                 if (f != NULL) {
                     fwrite(text, 1, n > 0 ? (size_t)n : 0, f);
                     fclose(f);
    @@ -98,3 +98,4 @@ void kj_crash_install(void)
         }
         threadOnException(handler, s_stacks[i] + HANDLER_STACK_BYTES, &s_data[i]);
     }
    +
    diff --git a/src/diagnostics/log.c b/src/diagnostics/log.c
    index f8b4369..728697b 100644
    --- a/src/diagnostics/log.c
    +++ b/src/diagnostics/log.c
    @@ -90,7 +90,7 @@ void kj_log_init(const char *path)
         LightLock_Init(&s_lock);
         LightEvent_Init(&s_wake, RESET_ONESHOT);
         s_start_tick = svcGetSystemTick();
    -    s_file = path != NULL ? fopen(path, "w") : NULL;
    +    s_file = path != NULL ? fopen(path, "a") : NULL;
         s_ready = true;
         s_running = true;
         /* Lowest application priority: it runs while the game waits for V-blank. */
    @@ -165,3 +165,4 @@ void kj_ctr_log(const char *line)
     {
         kj_log("%s", line);
     }
    +
    ✓ • 1.63s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\diagnostics\log.c
    @@ -93,2 +93,6 @@
         s_file = path != NULL ? fopen(path, "a") : NULL;
    +    if (s_file != NULL) {
    +        fputs("\n--- new run ---\n", s_file);
    +        fflush(s_file);
    +    }
         s_ready = true;

## Assistant

The logging code currently truncates both the main log and the crash log on startup or crash. I’ve changed those to append, with a run separator in the main log. The larger issue is that profiling is spread across the renderer, frame bridge, geometry decoder, and audio mixer; I’m checking build isolation before treating either binary as a valid baseline.

## Activity

    $ git diff --check; python tools/game/check_patches.py
    README.md:5: trailing whitespace.
    +not tested or known to be stable yet; runs near full-speed on real hardware, excluding geometry-heavy map areas && the intro ho-oh/lugia
    README.md:7: trailing whitespace.
    +this repository does not contain any unlicensed game data.
    src/diagnostics/crash.c:101: trailing whitespace.
    +
    src/diagnostics/crash.c:101: new blank line at EOF.
    src/diagnostics/log.c:172: trailing whitespace.
    +
    src/diagnostics/log.c:172: new blank line at EOF.
    94/94 patches applied, 0 missing
    ✓ • 2.14s

## Activity

    $ Get-Content build/log-append-build.log -Tail 8; git diff --check -- src/diagnostics/log.c src/diagnostics/crash.c
    o2narc naix: 14 (0 updated)
    csv2bin naix: 3 (0 updated)
    alias naix: 2 (0 updated)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/kj/gen/overlay_ids.ld (unchanged)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/kj/gen/overlay_ids.h (unchanged)
    done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
    ✓ • 1.63s

## Activity

    $ Get-Content build/log-append-build.log -Tail 8
    mkdir: cannot create directory â€˜/c/Users/tereziâ€™: Permission denied
    make[1]: *** [Makefile:245: /c/Users/terezi/workdir/kohjoh/build/kj/sdk/gen/nitro/fx/fx_const.h] Error 1
    make[1]: Target 'lib' not remade because of errors.
    make: *** [Makefile:97: libnitro] Error 2
    make: Target 'lib' not remade because of errors.
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
       warning: the vendored library build failed; linking with libnitrosystem.a, libnitro.a and reporting what is missing
    == overlay table
    ✓ • 1.37s

## Assistant

The first build attempt hit a sandbox filesystem restriction while rebuilding the vendored libraries, so its output cannot be trusted even if packaging succeeds. I’m requesting the required unrestricted build retry; the build system can otherwise silently link stale libraries.

## Activity

    $ python -c "from pathlib import Path; files=[Path('src/diagnostics/log.c'),Path('src/diagnostics/crash.c')]; [(p.write_bytes(p.read_bytes().rstrip()+b'\n')) for p in files]"; C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/kj -j 32 > /c/Users/terezi/workdir/kohjoh/build/log-append-build.log 2>&1'
    MSYS2 is starting for the first time. Executing the initial setup.
    mkdir: cannot create directory ‘/home/CodexSandboxOffline’: Permission denied
    /home/CodexSandboxOffline could not be created.
    Setting HOME to /tmp.
    Initial setup complete. MSYS2 is now ready to use.
    ✗ (1) • 2m 08s

## Activity

    $ Get-Content build/log-append-build.log -Tail 8
    o2narc naix: 14 (0 updated)
    csv2bin naix: 3 (0 updated)
    alias naix: 2 (0 updated)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/kj/gen/overlay_ids.ld (unchanged)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/kj/gen/overlay_ids.h (unchanged)
    done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
    ✓ • 1.86s

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/kj -j 32 > /c/Users/terezi/workdir/kohjoh/build/log-append-build.log 2>&1'
    ✗ (1) • 30.72s

## Assistant

The two requested builds are **not ready**. I found that the existing normal build still collects frame, geometry, GPU and audio measurements, so omitting the profiling flags would not produce a genuinely uninstrumented baseline. I have not yet separated those probes or added the measurements needed to identify the remaining CPU hotspot.

The supplied hardware logs support the diagnosis in `chatlog07`: the worst Goldenrod windows spend roughly 122–144 ms in game logic, while GPU drawing is around 3–4 ms per picture and GPU wait is negligible. They do not yet distinguish game-side geometry work from FIFO translation and cache replay.

I changed [log.c](C:/Users/terezi/workdir/kohjoh/src/diagnostics/log.c) and [crash.c](C:/Users/terezi/workdir/kohjoh/src/diagnostics/crash.c) to append instead of truncating existing logs. The main log gets a separator for each run. These changes have **not been verified in a completed build or on hardware**; frame dumps can still overwrite files with the same names.

The first build attempt hit a sandbox filesystem restriction and reported that it could link stale vendored libraries. An unrestricted retry was approved, but I stopped it before completion. No build artifact should be used as the requested test pair.

## User

well, then get back in there and make sure our normal build isn't collecting those things (and our instrumented build is collecting all of them). i approved your sandbox, you can keep working.

## Assistant

I’ll finish the separation and verify the actual binaries and build logs. I’ll keep the existing diagnostic output in the instrumented variant and remove its hot-path collection from the normal variant, then add the missing geometry timing needed for the next hardware run.

## Activity

    $ Get-Content src/compat/nitro/Makefile -TotalCount 150; Get-Content tools/game/hgss_game.py | Select-Object -Skip 200 -First 65; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 2260 -First 130; rg -n 'kj_perf_count|kj_perf_now|s_.*(stat|count|ticks)|svcGetSystemTick' src/graphics src/runtime/kj_g3.c src/runtime/kj_frame.c | Measure-Object; rg -n 'sProf|SNDPROF|NitroSound_MixProfile' src/compat/nitro/src/port_snd.c; rg -n 'EXTRA_DEFINES|make.*env|HGSS_PROFILE' tools/game/hgss_game.py
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
        math/src/dgt.c \
        math/src/crc.c \
        math/src/checksum.c \
        math/src/fft.c \
        mi/src/mi_wram.c \
        mi/src/mi_dma.c \
        mi/src/mi_dma_hblank.c \
        mi/src/mi_dma_vblank.c \
        mi/src/mi_dma_mainmem.c \
        mi/src/mi_dma_gxcommand.c \
        mi/src/mi_byteAccess.c \
        mi/src/mi_stream.c \
        mi/src/mi_card.c \
        mi/src/mi_dma_card.c \
        mi/src/mi_compress.c \
        mi/src/mi_exMemory.c \
        mi/src/mi_init.c \
        mi/src/mi_cache.c \
        os/src/os_irqHandler.c \
        os/src/os_irqTable.c \
        os/src/os_interrupt.c \
        os/src/os_spinLock.c \
        os/src/os_printf.c \
        os/src/os_thread.c \
        os/src/os_context.c \
        os/src/os_emulator.c \
        os/src/os_message.c \
        os/src/os_mutex.c \
        os/src/os_init.c \
        os/src/os_arena.c \
        os/src/os_alloc.c \
        os/src/os_timer.c \
        os/src/os_tick.c \
        os/src/os_alarm.c \
        os/src/os_valarm.c \
        os/src/os_system.c \
        os/src/os_profile.c \
        os/src/os_ownerInfo.c \
        os/src/os_vramExclusive.c \
        os/src/os_argument.c \
        os/src/os_entropy.c \
        os/src/os_terminate_proc.c \
        rtc/src/external.c \
        rtc/src/internal.c \
        rtc/src/convert.c \
        snd/src/snd_interface.c \
        snd/src/snd_main.c \
        snd/src/snd_command.c \
        snd/src/snd_alarm.c \
        snd/src/snd_work.c \
        snd/src/snd_util.c \
        snd/src/snd_bank.c \
        snd/src/snd_data.c \
        spi/src/tp.c \
        spi/src/mic.c \
        spi/src/pm.c \
        std/src/std_string.c \
        std/src/std_sprintf.c \
        std/src/std_unicode.c \
        wbt/src/wbt_context.c \
        wbt/src/wbt_api.c \
        wfs/src/wfs_format.c \
        wfs/src/wfs_server.c \
        wfs/src/wfs_client.c \
        wfs/src/wfs_archive.c \
        wfs/src/wfs_thread.c \
        utest/src/utest_assert.c \
        utest/src/utest_cui.c \
        utest/src/utest_main.c \
        wm/src/wm_system.c \
        wm/src/wm_sync.c \
        wm/src/wm_standard.c \
        wm/src/wm_mp.c \
        wm/src/wm_dcf.c \
        wm/src/wm_ds.c \
        wm/src/wm_ks.c \
        wm/src/wm_etc.c
    
    # Not built, because the game doesn't use them: mb (download play),
    # ctrdg (GBA slot, answered by src/ctrdg.c instead), prc (gesture recognition),
    # wvr, env, ext, os/src/os_china.c (China-edition boot screen) and
    # math/src/qsort.c. pad and exi are ARM7-only. utest is built because the
    # SDK's own self-tests (in os_thread.c) refer to it.
    #
    # Replaced by our own C in src/:
        everything else is what every port object is compiled with. The C++
        game files (src/lobby) are built without exceptions and RTTI, so their
        static constructors stay ordinary .init_array entries.
        """
        if target not in ("host32", "3ds"):
            raise ValueError(f"unknown target {target!r}")
        flags = ["-std=gnu++98" if cxx else "-std=gnu99",
                 "-fgnu89-inline", "-fno-strict-aliasing", "-fno-short-enums",
                 "-fsigned-char", "-fwrapv",
                 "-fno-aggressive-loop-optimizations",
                 "-fno-delete-null-pointer-checks"]
        flags += PORT_DEFINES
        # Instrumentation build (Packet A / PICA200-INSTRUMENTATION Â§B): the
        # kohjoh default is OFF; HGSS_PROFILE=on compiles the PROFILE build
        # (every counter and tick probe). The OFF build: the 2D
        # renderer's per-draw tick reads (R2D_NOW) become zero and the geometry
        # diagnostics stop logging. The two builds must differ only in this
        # define; everything else (flags, commit, inputs) stays identical.
        # The same define must reach libnitro.a/libnitrosystem.a, which is why
        # tools/hgss_nitro.py also injects it (see its sdk_includes callers).
        if profile_mode() == "off":
            flags += ["-DHGSS_PROFILE_OFF"]
        # Automated benchmark builds only (docs/autobench.md): HGSS_BENCH=1
        # compiles the hb:ldr chainload path in hgss/game/app/main_3ds.c.
        # Normal builds never define it, so the chainloading code cannot end
        # up in a release artifact. 3DS-only: the path uses libctru/hb:ldr
        # and does not compile against the host libc.
        if target == "3ds" and os.environ.get("HGSS_BENCH") == "1":
            flags += ["-DHGSS_BENCH"]
        # Diagnostic build only: never enable texture timing in normal releases.
        if target == "3ds" and os.environ.get("HGSS_TEXTURE_PROFILE") == "1":
            flags += ["-DHGSS_TEXTURE_PROFILE=1"]
        if target == "3ds" and os.environ.get("HGSS_TEXTURE_HINT") == "1":
            flags += ["-DHGSS_TEXTURE_HINT=1"]
        if target == "3ds" and os.environ.get("HGSS_GX3D_HOTLOOP") == "1":
            flags += ["-DHGSS_GX3D_HOTLOOP=1"]
        if target == "3ds" and os.environ.get("HGSS_GX3D_INDEXED") == "1":
            flags += ["-DHGSS_GX3D_INDEXED=1"]
        if target == "3ds" and os.environ.get("HGSS_GX3D_LISTCACHE") == "1":
            flags += ["-DHGSS_GX3D_LISTCACHE=1"]
        if target == "3ds" and os.environ.get("HGSS_GX3D_STATEDEDUP") == "1":
            flags += ["-DHGSS_GX3D_STATEDEDUP=1"]
        # BG3SLOT gates NITRO_GPU_BG_SLOTS in the shared render2d header, so
        # every port object (lib and app) must see it: a split otherwise
        # corrupts the split/layer structs across the drawer call.
        if target == "3ds" and os.environ.get("HGSS_GX3D_BG3SLOT") == "1":
            flags += ["-DHGSS_GX3D_BG3SLOT=1"]
        # kohjoh diagnostic switches (--define NAME): MSYS2 Python does not see
        # variables set inline on the command line, so these are arguments.
        flags += [f"-D{d}" for d in EXTRA_DEFINES]
        if target == "3ds" and os.environ.get("HGSS_A0_DIAG") == "1":
            flags += ["-DHGSS_A0_DIAG=1"]
        flags += game_defines()
        flags += WARNINGS + SECTION_FLAGS
        if cxx:
            flags += ["-fno-exceptions", "-fno-rtti"]
        flags += HOST_FLAGS if target == "host32" else ARM_FLAGS
        if SANITIZE and target == "host32":
            flags += SANITIZE_FLAGS
        else:
            # The game is 1,092 objects and used to carry no -O at all, so GCC's
            # default (-O0) applied to all of them: no inlining, no frame-pointer
            # elision, no IPA. Measured on devkitARM 16.1.0, -O2 shrinks the game's
            # .text from 8,146,868 to 4,512,808 B (-44.6%) and the packaged .3dsx
            # from 9,403,804 to 5,762,900 B -- and on an Old 3DS that freed memory
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
                ++s_dl_hits;
                s_dl_cov[DLC_HIT] += (uint32_t)count;
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
            s_dl_cov[DLC_HIT] += (uint32_t)count;
        } else if (e->recorded) {
            s_dl_cov[DLC_KEY] += (uint32_t)count;
        }
        ++s_dl_misses;
        if (e->recorded && !verify) {
            dl_drop_recording(e);
        }
        frame_buf *fb = &s_buf[s_build];
        /* reset every scalar field (the arrays need no clearing; nevents and
         * nblocks follow them in the struct, so one memset cannot cover them) */
        s_rec.active = s_rec.bad = s_rec.has_matrix = s_rec.vecdep = false;
        s_rec.nevents = 0;
        s_rec.nblocks = 0;
        s_rec.active = true;
        s_rec.base = words;
        s_rec.v0 = fb->frame.vert_count;
        s_rec.i0 = fb->frame.index_count;
        rec_open_block();
        const uint32_t drops0 = fb->frame.dropped_verts + fb->frame.dropped_batches + fb->frame.unsupported;
        decode_words(words, count);
        rec_close_block();
        s_rec.active = false;
    
        if (fb->frame.dropped_verts + fb->frame.dropped_batches + fb->frame.unsupported != drops0 ||
            s_fifo_need > 0) {
            s_rec.bad = true;
            ++s_dl_why[2];
        }
        if (s_rec.vecdep && s_rec.has_matrix) {
            s_rec.bad = true;
            ++s_dl_why[3];
        }
        if (s_rec.vecdep && !lit && !s_rec.bad) {
            /* keyed without the lighting state it turned out to read: key on it
             * from now on and record next time */
            e->needs_light = true;
            return;
        }
    #ifdef KJ_DL_VERIFY
        if (verify) {
            dl_verify(e, fb);
            return;
        }
    #endif
        if (s_rec.bad) {
            e->never = true;
            return;
        }
        const uint32_t nv = fb->frame.vert_count - s_rec.v0, ni = fb->frame.index_count - s_rec.i0;
        if (!dl_reserve((void **)&e->events, &e->cap_events, s_rec.nevents + 1u, 4u) ||
            !dl_reserve((void **)&e->blocks, &e->cap_blocks, s_rec.nblocks + 1u, (uint32_t)sizeof(dl_block)) ||
            !dl_reserve((void **)&e->verts, &e->cap_verts, nv, (uint32_t)sizeof(kj_g3_vertex)) ||
            !dl_reserve((void **)&e->idx, &e->cap_idx, ni, 2u)) {
            dl_flush_all(); /* arena full: start over (re-recorded on later calls) */
            return;
        }
        memcpy(e->events, s_rec.events, s_rec.nevents * 4u);
        memcpy(e->blocks, s_rec.blocks, s_rec.nblocks * sizeof(dl_block));
        e->nevents = s_rec.nevents;
        e->nblocks = s_rec.nblocks;
        if (nv) {
            memcpy(e->verts, &fb->verts[s_rec.v0], nv * sizeof(kj_g3_vertex));
        }
        for (uint32_t b = 0; b < s_rec.nblocks; ++b) {
            const dl_block *bl = &s_rec.blocks[b];
            for (uint32_t i = 0; i < bl->nidx; ++i) {
                const uint32_t abs = fb->indices[s_rec.i0 + bl->first_idx + i];
                e->idx[bl->first_idx + i] = (uint16_t)(abs - (s_rec.v0 + bl->first_vert));
            }
        }
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
                ++s_dl_why[5];
                dl_drop_recording(e);
                e->never = true;
                return;
    
    
    Count    : 186
    Average  :
    Sum      :
    Maximum  :
    Minimum  :
    Property :
    
    31:#define SNDPROF_NOW() svcGetSystemTick()
    33:#define SNDPROF_NOW() ((u64)0)
    57:extern void NitroSound_MixProfile(uint64_t *calls, uint64_t *frames,
    75:static uint64_t sProfRenderCalls;   /* SoundRender calls that reached the mix loop */
    76:static uint64_t sProfRenderFrames;  /* output frames requested, summed over those calls */
    77:static uint64_t sProfStretches;     /* mix-loop iterations (one lock hold + one Mix each) */
    78:static uint64_t sProfSoundTicks;    /* sequencer sound_tick executions */
    79:static uint64_t sProfMixTicks;      /* ARM11 ticks inside NitroSound_Mix (__3DS__ only; 0 on host) */
    80:static uint64_t sProfTickTicks;     /* ARM11 ticks inside sound_tick (__3DS__ only; 0 on host) */
    250:    sProfRenderCalls += 1u;
    251:    sProfRenderFrames += (uint64_t)frames;
    266:            u64 mixStart = SNDPROF_NOW();
    268:            sProfMixTicks += (uint64_t)(SNDPROF_NOW() - mixStart);
    274:                u64 tickStart = SNDPROF_NOW();
    276:                sProfTickTicks += (uint64_t)(SNDPROF_NOW() - tickStart);
    278:            sProfSoundTicks += 1u;
    282:        sProfStretches += 1u;
    287:    if (sProfRenderCalls % 600u == 0u) {
    290:        NitroSound_MixProfile(&mixCalls, &mixFrames, &activeCh, formatCh,
    293:                      (unsigned long long)sProfRenderCalls,
    294:                      (unsigned long long)sProfRenderFrames,
    295:                      (unsigned long long)sProfStretches,
    296:                      (unsigned long long)sProfSoundTicks,
    297:                      (unsigned long long)sProfMixTicks,
    298:                      (unsigned long long)sProfTickTicks,
    214:    # kohjoh default is OFF; HGSS_PROFILE=on compiles the PROFILE build
    222:        flags += ["-DHGSS_PROFILE_OFF"]
    250:    flags += [f"-D{d}" for d in EXTRA_DEFINES]
    285:EXTRA_DEFINES: list[str] = []
    1530:    return "profile" if os.environ.get("HGSS_PROFILE") == "on" else "off"
    1590:        # HGSS_PROFILE toggles the 2D tick reads (port_render2d.c) and the
    1648:    EXTRA_DEFINES.extend(args.define)
    
    
    ✓ • 1.49s

## Activity

    $ rg -n 's_stats|s_dl_cov|s_dl_why|s_fifo_calls|s_fifo_words|s_dl_hits|s_dl_misses|s_dl_uncacheable' src/runtime/kj_g3.c; rg -n 'stats\.|s_stats|kj_perf_count|kj_perf_now' src/graphics; Get-Content src/compat/common.mk -TotalCount 130; Get-Content tools/game/hgss_nitro.py | Select-Object -First 150; Get-Content src/compat/nitro/src/port_snd.c | Select-Object -Skip 235 -First 90
    492:static kj_g3_stats s_stats;
    542:    *out = s_stats;
    925:    ++s_stats.box_tests;
    960:        ++s_stats.box_culled;
    1026:    f->serial = ++s_stats.swaps;
    1101:    ++s_stats.commands;
    1116:                          (unsigned long)s_stats.swaps);
    1330:static u32 s_fifo_words, s_direct_words;
    1621:static u32 s_fifo_calls;
    1757:static uint32_t s_dl_hits, s_dl_misses, s_dl_uncacheable;
    1761:static uint32_t s_dl_cov[DLC_COUNT];
    1762:static uint32_t s_dl_why[6]; /* batch-in-block, shared-verts, straddle/drops, lit+matrix, swap, open-prim */
    1777:        ++s_dl_why[0];
    1782:            ++s_dl_why[1];
    1820:        ++s_dl_why[4];
    2137:        ++s_dl_hits;
    2192:    ++s_fifo_calls;
    2193:    s_fifo_words += (u32)count;
    2203:        s_dl_cov[DLC_SHORT] += (uint32_t)count;
    2205:            s_dl_cov[DLC_SMALL] += (uint32_t)count;
    2207:            s_dl_cov[DLC_SPLIT] += (uint32_t)count;
    2209:            s_dl_cov[DLC_BEGIN] += (uint32_t)count;
    2211:                s_dl_cov[DLC_BEGIN_FIRST] += (uint32_t)count;
    2223:        s_dl_cov[DLC_NEW] += (uint32_t)count;
    2236:    } else if (e->never && (s_stats.swaps & DL_NEVER_RECHECK_MASK) != 0u) {
    2240:        e->last_frame = s_stats.swaps;
    2241:        s_dl_cov[DLC_NEVER] += (uint32_t)count;
    2242:        ++s_dl_uncacheable;
    2246:        s_dl_cov[DLC_CHANGED] += (uint32_t)count;
    2253:    e->last_frame = s_stats.swaps;
    2255:        s_dl_cov[DLC_NEVER] += (uint32_t)count;
    2256:        ++s_dl_uncacheable;
    2275:            ++s_dl_hits;
    2276:            s_dl_cov[DLC_HIT] += (uint32_t)count;
    2285:        if (logged < 30 && s_stats.swaps > 4600u) {
    2301:        s_dl_cov[DLC_HIT] += (uint32_t)count;
    2303:        s_dl_cov[DLC_KEY] += (uint32_t)count;
    2305:    ++s_dl_misses;
    2328:        ++s_dl_why[2];
    2332:        ++s_dl_why[3];
    2387:            ++s_dl_why[5];
    2399:    *hits = s_dl_hits;
    2400:    *misses = s_dl_misses;
    2401:    *uncacheable = s_dl_uncacheable;
    2405:                  (unsigned long)s_dl_why[0], (unsigned long)s_dl_why[1], (unsigned long)s_dl_why[2],
    2406:                  (unsigned long)s_dl_why[3], (unsigned long)s_dl_why[4], (unsigned long)s_dl_why[5],
    2409:                  (unsigned long)s_dl_cov[DLC_SMALL], (unsigned long)s_dl_cov[DLC_SPLIT],
    2410:                  (unsigned long)s_dl_cov[DLC_BEGIN], (unsigned long)s_dl_cov[DLC_BEGIN_FIRST]);
    2412:                  (unsigned long)s_dl_cov[DLC_SHORT], (unsigned long)s_dl_cov[DLC_NEW],
    2413:                  (unsigned long)s_dl_cov[DLC_CHANGED], (unsigned long)s_dl_cov[DLC_NEVER],
    2414:                  (unsigned long)s_dl_cov[DLC_HIT], (unsigned long)s_dl_cov[DLC_KEY]);
    2415:    memset(s_dl_cov, 0, sizeof(s_dl_cov));
    2416:    memset(s_dl_why, 0, sizeof(s_dl_why));
    2419:    s_dl_hits = s_dl_misses = s_dl_uncacheable = 0;
    2426:    *fifo_ticks = (uint64_t)s_fifo_calls; /* calls; ticks only with KJ_G3_PROFILE */
    2427:    s_fifo_calls = 0;
    2429:    *fifo_words = s_fifo_words;
    2431:    *verts = s_stats.commands;
    2433:    s_fifo_words = s_direct_words = 0;
    2434:    s_stats.commands = 0;
    src/graphics\resources\atlas.h:80:typedef struct kj_atlas_stats {
    src/graphics\resources\atlas.h:82:} kj_atlas_stats;
    src/graphics\resources\atlas.h:84:void kj_atlas_take_stats(kj_atlas_stats *out);
    src/graphics\resources\atlas.c:43:static kj_atlas_stats s_stats;
    src/graphics\resources\atlas.c:97:void kj_atlas_take_stats(kj_atlas_stats *out)
    src/graphics\resources\atlas.c:99:    *out = s_stats;
    src/graphics\resources\atlas.c:100:    memset(&s_stats, 0, sizeof(s_stats));
    src/graphics\resources\atlas.c:168:            ++s_stats.evictions;
    src/graphics\resources\atlas.c:200:    ++s_stats.conversions;
    src/graphics\resources\atlas.c:201:    kj_perf_count(KJ_COUNT_TILE_CONV, 1);
    src/graphics\resources\atlas.c:213:    ++s_stats.lookups;
    src/graphics\resources\atlas.c:245:        ++s_stats.full;
    src/graphics\renderer.c:123:    uint64_t t = kj_perf_now();
    src/graphics\renderer.c:125:    kj_perf_add(KJ_PERF_GPU_WAIT, kj_perf_now() - t);
    src/graphics\renderer.c:137:    t = kj_perf_now();
    src/graphics\renderer.c:139:    kj_perf_add(KJ_PERF_SUBMIT, kj_perf_now() - t);
    src/graphics\renderer.c:158:        kj_atlas_stats a;
    src/graphics\g3d\textures.c:46:static kj_tex_stats s_stats;
    src/graphics\g3d\textures.c:264:            ++s_stats.missing_bank;
    src/graphics\g3d\textures.c:327:                    ++s_stats.missing_bank;
    src/graphics\g3d\textures.c:386:    s_stats.compared_bytes += total;
    src/graphics\g3d\textures.c:387:    kj_perf_count(KJ_COUNT_TEX_BYTES, total);
    src/graphics\g3d\textures.c:443:        ++s_stats.uploads;
    src/graphics\g3d\textures.c:444:        kj_perf_count(KJ_COUNT_TEX_UPLOAD, 1);
    src/graphics\g3d\textures.c:450:        ++s_stats.full;
    src/graphics\g3d\textures.c:460:            ++s_stats.full;
    src/graphics\g3d\textures.c:466:        ++s_stats.full;
    src/graphics\g3d\textures.c:482:        ++s_stats.missing_bank;
    src/graphics\g3d\textures.c:496:    ++s_stats.uploads;
    src/graphics\g3d\textures.c:497:        kj_perf_count(KJ_COUNT_TEX_UPLOAD, 1);
    src/graphics\g3d\textures.c:503:    *out = s_stats;
    src/graphics\g3d\textures.c:504:    memset(&s_stats, 0, sizeof(s_stats));
    src/graphics\g3d\g3d.c:32:static kj_g3d_stats s_stats;
    src/graphics\g3d\g3d.c:66:    *out = s_stats;
    src/graphics\g3d\g3d.c:70:    memset(&s_stats, 0, sizeof(s_stats));
    src/graphics\g3d\g3d.c:155:            ++s_stats.unsupported; /* toon/highlight drawn as modulate */
    src/graphics\g3d\g3d.c:188:                ++s_stats.unsupported; /* shadow mask volume: not drawn */
    src/graphics\g3d\g3d.c:257:        ++s_stats.draws;
    src/graphics\g3d\g3d.c:258:        s_stats.triangles += b->index_count / 3u;
    src/graphics\g3d\g3d.c:269:    uint64_t t0 = kj_perf_now();
    src/graphics\g3d\g3d.c:270:    s_stats.batches += f->batch_count;
    src/graphics\g3d\g3d.c:271:    s_stats.unsupported += f->unsupported + f->dropped_batches + f->dropped_verts;
    src/graphics\g3d\g3d.c:272:    s_stats.box_tests += f->box_tests;
    src/graphics\g3d\g3d.c:273:    s_stats.box_culled += f->box_culled;
    src/graphics\g3d\g3d.c:274:    s_stats.box_culled_far += f->box_culled_far;
    src/graphics\g3d\g3d.c:276:        ++s_stats.unsupported; /* fog */
    src/graphics\g3d\g3d.c:279:        ++s_stats.unsupported; /* edge marking (baseline excluded) */
    src/graphics\g3d\g3d.c:306:    kj_perf_add(KJ_PERF_G3D, kj_perf_now() - t0);
    src/graphics\g2d\g2d.c:51:static kj_g2d_stats s_stats;
    src/graphics\g2d\g2d.c:303:                ++s_stats.dropped_quads;
    src/graphics\g2d\g2d.c:375:        ++s_stats.unsupported_modes;
    src/graphics\g2d\g2d.c:464:            ++s_stats.unsupported_modes; /* OBJ window / bitmap OBJ */
    src/graphics\g2d\g2d.c:518:            ++s_stats.dropped_quads;
    src/graphics\g2d\g2d.c:599:        ++s_stats.unsupported_modes; /* OBJ window: drawn as outside */
    src/graphics\g2d\g2d.c:667:            ++s_stats.draws;
    src/graphics\g2d\g2d.c:680:        s_stats.draws += 2;
    src/graphics\g2d\g2d.c:757:    ++s_stats.draws;
    src/graphics\g2d\g2d.c:784:    ++s_stats.draws;
    src/graphics\g2d\g2d.c:1085:            ++s_stats.unsupported_modes;
    src/graphics\g2d\g2d.c:1107:        s_stats.bg_quads += pr->bg[bg].count;
    src/graphics\g2d\g2d.c:1108:        kj_perf_count(KJ_COUNT_QUADS, pr->bg[bg].count);
    src/graphics\g2d\g2d.c:1129:            s_stats.obj_quads += pr->obj[i].count;
    src/graphics\g2d\g2d.c:1130:            kj_perf_count(KJ_COUNT_QUADS, pr->obj[i].count);
    src/graphics\g2d\g2d.c:1230:    uint64_t t0 = kj_perf_now();
    src/graphics\g2d\g2d.c:1241:            kj_perf_add(KJ_PERF_G2D, kj_perf_now() - t0);
    src/graphics\g2d\g2d.c:1255:        kj_perf_add(KJ_PERF_G2D, kj_perf_now() - t0);
    src/graphics\g2d\g2d.c:1259:        ++s_stats.unsupported_modes; /* VRAM / main-memory display: tracked blocker */
    src/graphics\g2d\g2d.c:1261:        kj_perf_add(KJ_PERF_G2D, kj_perf_now() - t0);
    src/graphics\g2d\g2d.c:1273:        ++s_stats.spans;
    src/graphics\g2d\g2d.c:1274:        kj_perf_count(KJ_COUNT_SPANS, 1);
    src/graphics\g2d\g2d.c:1284:            ++s_stats.spans;
    src/graphics\g2d\g2d.c:1285:        kj_perf_count(KJ_COUNT_SPANS, 1);
    src/graphics\g2d\g2d.c:1298:    kj_perf_add(KJ_PERF_G2D, kj_perf_now() - t0);
    src/graphics\g2d\g2d.c:1317:    s_stats.skipped_screens = s_skipped_screens;
    src/graphics\g2d\g2d.c:1318:    s_stats.reused_layers = s_reused_layers;
    src/graphics\g2d\g2d.c:1319:    s_stats.rebuilt_layers = s_rebuilt_layers;
    src/graphics\g2d\g2d.c:1320:    s_stats.char_bytes_compared = s_char_bytes_compared;
    src/graphics\g2d\g2d.c:1323:    *out = s_stats;
    src/graphics\g2d\g2d.c:1325:    memset(&s_stats, 0, sizeof(s_stats));
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
                    $(foreach d,$(filter-out %/wl_arm7/include,$(wildcard $(SDK)/libraries/*/include)),-I$(d))
    NNS_INCLUDES := -I$(NNS)/include \
                    $(foreach d,$(wildcard $(NNS)/libraries/*/include),-I$(d))
    
    ifeq ($(TARGET),3ds)
      ifeq ($(strip $(DEVKITARM)),)
        $(error Set DEVKITARM (install devkitPro with 3ds-dev))
      endif
      PREFIX := $(DEVKITARM)/bin/arm-none-eabi-
      CC     := $(PREFIX)gcc
      AR     := $(PREFIX)gcc-ar
      NM     := $(PREFIX)nm
      OBJCOPY := $(PREFIX)objcopy
      # The 3DS's ARM11 MPCore, with its VFP floating-point unit.
      PORT_CFLAGS += -march=armv6k -mtune=mpcore -mfpu=vfp -mfloat-abi=hard -mtp=soft \
                     -mword-relocations -ffunction-sections -D__3DS__
    else ifneq ($(filter host host32,$(TARGET)),)
      IS_HOST := yes
      CC     ?= cc
      AR     ?= ar
      NM     ?= nm
      OBJCOPY ?= objcopy
      # The SDK stores addresses in 32-bit integers. On a 64-bit PC that only
      # works if our data sits below 4 GB, which a non-PIE executable guarantees.
      PORT_CFLAGS += -fno-pie -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
      # -pthread: the port's threads are POSIX threads on a PC (nitro/src/platform/host.c).
      LDFLAGS += -no-pie -pthread
      ifeq ($(TARGET),host32)
        # Nintendo's file formats hold 4-byte pointers that the libraries fix up
        # in place, so those parts only work with 32-bit pointers, as on the 3DS.
        # -malign-double aligns 64-bit values to 8 bytes like ARM (x86 uses 4).
        # Needs 32-bit C libraries (Ubuntu: gcc-multilib).
        PORT_CFLAGS += -m32 -malign-double
        LDFLAGS += -m32
      endif
    else
      $(error TARGET must be host, host32 or 3ds)
    endif
    
    # NitroSystem's headers define small functions as plain `inline`. Metrowerks
    # kept one copy of each; GCC exports one from every file that includes the
    # header, and those files then can't be linked together. So after compiling,
    # mark them weak: the linker keeps one copy, as Metrowerks did. The list is
    # made by compiling every NitroSystem header (tools/nns_inline_symbols.py).
    NNS_INLINE_SYMS   := $(ROOT)/3ds_port/nitrosystem/build/$(TARGET)/gen/nns_inline.syms
    WEAKEN_NNS_INLINE  = $(OBJCOPY) --weaken-symbols=$(NNS_INLINE_SYMS) $@
    
    # kohjoh: MSYS2 strips TMP/TEMP from the environment its children see, and a
    # native-Windows GCC without them tries to write temporaries to C:\Windows.
    ifneq ($(OS),)
    export TMP  := $(shell cygpath -w /tmp 2>/dev/null)
    export TEMP := $(TMP)
    endif
    #!/usr/bin/env python3
    """Build the vendored 3ds_port libraries against HGSS's SDK without editing the vendored Makefiles.
    
    Usage (inside the dev container, after tools/bootstrap.py):
      hgss_nitro.py shim                 create the shim trees and print the nitro make overrides
      hgss_nitro.py make --target host32 --out /work/out/p1 lib [-k -j8]
                                         run `make -C 3ds_port/nitro` with the overrides
      hgss_nitro.py make --target host32 --out /work/out/p1 test
                                         run `test` across pak, nitro and nitrosystem (see --subproject)
      hgss_nitro.py make --target host32 --out /work/out/p1 test --subproject nitro
                                         run `test` for one subproject only
      hgss_nitro.py list                 print the HGSS source lists that are compiled, and the excluded files
      hgss_nitro.py sdat-render --out /work/audio-probe/build
                                         build tests/sdat_render (the offline sequence renderer, host32) and
                                         its two archives: libnitro.a, libnitrosystem.a, then the renderers
    
    How it works (docs/port-wiring.md): the vendored nitro Makefile compiles `$(SDK)/libraries/<x>.c`
    for each entry of SDK_SRCS. `build/hgss-sdk/libraries/hgss` is a symlink to
    build/upstream/lib/NitroSDK/src, so the list can name HGSS's real files (`hgss/os/os_thread.c`).
    `hgss_compat` (a symlink to hgss/compat/src) adds the port's SDK stand-ins, SDK data and NNS
    data; `hgss_sub` (a symlink to sub/lib/src) feeds the genuine ARM7 sound driver to the
    Makefile's ARM7 rule, which renames its symbols to N7_*. The nitrosystem Makefile compiles
    `$(NNS)/libraries/<x>.c` for each entry of NNS_SRCS: `build/hgss-nns` shims HGSS's flat
    lib/NitroSystem/src (as `hgss_nns/`) plus the two files the nitro tests compile directly
    (`g3d/src/gecom.c`, `gfd/src/VramTransferMan/gfd_VramTransferManager.c`). Headers: a shim
    include dir with only nitro*/nnsys*/cw* from lib/include (its stdarg.h/stddef.h/string.h would
    shadow the system ones), HGSS's flat SDK include dirs, and hgss/compat/include (tracked:
    headers the port includes that HGSS names differently or lacks). Command-line variables
    override the Makefiles' own assignments, so nothing vendored changes.
    """
    import argparse
    import os
    import re
    import shlex
    import shutil
    import subprocess
    import sys
    from pathlib import Path
    
    REPO = Path(__file__).resolve().parent.parent.parent
    
    # kohjoh: MSYS2 drops TMP/TEMP from the environment a child process inherits;
    # a native-Windows GCC without them tries C:\Windows for its temporaries and
    # fails ("Cannot create temporary file"). Give every child a usable one.
    if os.name == "nt" or os.environ.get("MSYSTEM") or sys.platform in ("msys", "cygwin"):
        if not os.environ.get("TMP"):
            _tmp = subprocess.run(["cygpath", "-w", "/tmp"], capture_output=True, text=True).stdout.strip()
            if _tmp:
                os.environ["TMP"] = os.environ["TEMP"] = _tmp
    BUILD = REPO / "build"
    
    # HGSS SDK sources the port replaces with its own code (3ds_port/nitro/src), or that make no sense on the
    # port. Matches docs/progress/phase-1.md ("port-replaced") plus the libraries the vendored Makefile documents
    # as not built. os_terminate_arm9.c defines OS_Terminate, which the port owns (port_os.c).
    EXCLUDE_FILES = {
        "crt0.c", "dgt_hash.c", "dgt_hash2_arm4_small.c",
        "mi/mi_memory.c", "mi/mi_swap.c", "mi/mi_uncompress.c",
        "os/os_cache.c", "os/os_tcm.c", "os/os_alarm.c",
        "os/os_terminate_arm9.c",
        "wvr.c",
    }
    EXCLUDE_PREFIXES = ("os/os_exception", "os/os_protection", "os/os_reset_", "pxi/", "ctrdg/", "mb/")
    
    # HGSS's genuine ARM7 sound driver (sub/lib/src/). The vendored nitro Makefile compiles these with
    # -DSDK_ARM7 and renames every global to N7_*, which is what port_snd.c calls (N7_SND_Init,
    # NNS_G3dGlb-style renaming for sound). Needs SNDi_DecibelTable from the ARM9 SND_util.c object.
    # snd_arm7_data.c is hgss/compat/src's sound-only subset of sub/asm/sound_card_pad_data.s (the
    # driver's own BSS: SNDi_Work, the sequence cache window, thread/alarm state, tables); it is
    # compiled by the same ARM7 rule so its globals are renamed too.
    ARM7_SOUND_FILES = (
        "sound_main.c", "sound_command.c", "sound_ex_channel.c", "sound_seq_player.c",
        "sound_alarm.c", "sound_bank.c", "sound_channel.c", "sound_driver.c",
        "sound_shared_work.c",
    )
    # ds_sound_tables.c: the two BIOS sound tables (SWIs) and the ARM7 PM control pair
    # the driver calls; the port has neither BIOS nor ARM7 SPI.
    ARM7_COMPAT_FILES = ("snd_arm7_data.c", "ds_sound_tables.c")
    
    # hgss/compat/src files compiled into libnitro.a (weak symbols: a real SDK definition wins).
    # nns_data.c is NNS BSS (no references); nns_tables_gfd.c is the VRAM transfer dispatch table
    # the nitro VRAM-transfer test dispatches through. The remaining NNS tables go to libnitrosystem.a.
    COMPAT_SRCS = ("sdk_data.c", "sdk_standins.c", "nns_data.c", "nns_tables_gfd.c")
    # hgss/compat/src files compiled into libnitrosystem.a (as hgss_nns_compat/).
    NNS_COMPAT_SRCS = ("nns_tables_nns.c",)
    
    SUBPROJECTS = ("pak", "nitro", "nitrosystem")
    
    # test_nitrosystem walks a directory of game files (see 3ds_port/nitrosystem/tests/test_nitrosystem.c):
    # lowercase .nsbmd models (it requires more than 600) and uppercase .NCGR/.NCLR/.NCER/.NANR/.NSCR
    # 2D files. The decompilation's files/ holds a few dozen models, so the fixture pads the count with
    # hard links to one real model. Links, not copies: no extra disk, no game bytes in the repo (build/
    # is git-ignored). nftw with FTW_PHYS skips symlinks, hence hard links.
    NNS_RES_PAD_MODELS = 600
    NNS_RES_2D_EXTS = frozenset({".NCGR", ".NCLR", ".NCER", ".NANR", ".NSCR"})
    
    
    def src_dir(root):
        return root / "lib" / "NitroSDK" / "src"
    
    
    def sdk_sources(root):
        base = src_dir(root)
        keep, drop = [], []
        for p in sorted(base.rglob("*.c")):
            rel = p.relative_to(base).as_posix()
            if rel in EXCLUDE_FILES or rel.startswith(EXCLUDE_PREFIXES):
                drop.append(rel)
            else:
                keep.append(rel)
        return keep, drop
    
    
    def nns_sources(root):
        base = root / "lib" / "NitroSystem" / "src"
        return sorted(p.name for p in base.glob("*.c"))
    
    
    def _abspath(link, target):
        """The absolute path `target` refers to from `link`'s directory."""
        link, target = Path(link), Path(target)
        return target if target.is_absolute() else link.parent / target
    
    
    def posix(path) -> str:
        """Forward-slash spelling of a path (`C:/...`), for native Windows tools.
    
        Native devkitARM/3dsxtool binaries accept forward slashes, while no shell
        sits between this process and them, so backslashes would also work; the
        forward-slash form is used for consistency. It must NOT cross into msys
        sh/python (see msys()): their runtime treats `C:/...` as relative.
        """
        return Path(path).as_posix()
    
    
    def msys(path) -> str:
        """msys-absolute spelling of a path (`/c/...`), for values passed to make.
    
        Every make variable and `-C` directory crosses msys sh, whose runtime
        eats backslashes as escapes and fails `C:/...` drive paths as relative;
        only `/c/...` reaches both msys programs (python3, sh scripts) and native
        ones (msys converts back to `C:\\...` for non-msys executables).
        """
        text = Path(path).as_posix()
        match = re.match(r"^([A-Za-z]):/(.*)$", text)
        return f"/{match.group(1).lower()}/{match.group(2)}" if match else text
    
    
    def link_points_to(link, target) -> bool:
        """True when `link` already reaches `target`.
    
     * fractional sample timing.
     *
     * Mixed a stretch at a time: up to the next tick (about 171 samples), with
     * the lock held for one stretch only, so the game never waits for long.
     * Commands the game sends are taken at the start of the next stretch, at
     * most 5.2 ms later (the DS's ARM7 took them on its next tick, too). */
    void NitroPort_SoundRender(int16_t *stereo, size_t frames)
    {
        size_t done = 0;
        if (!stereo) return;
        if (!__atomic_load_n(&sReady, __ATOMIC_ACQUIRE)) {
            memset(stereo, 0, frames * 4u);
            return;
        }
        sProfRenderCalls += 1u;
        sProfRenderFrames += (uint64_t)frames;
        while (done < frames) {
            uint64_t toTick;
            size_t n;
    
            sound_lock();
            if (sCount) {
                N7_SND_CommandProc();
                N7_SND_UpdateSharedWork();
            }
            /* Samples until the one that reaches the next tick, included. */
            toTick = (SOUND_TICK_SAMPLES - sTickPhase + SOUND_BUS_CLOCK - 1u) / SOUND_BUS_CLOCK;
            n = frames - done;
            if (toTick < n) n = (size_t)toTick;
            {
                u64 mixStart = SNDPROF_NOW();
                NitroSound_Mix(&sChip, stereo + 2u * done, n);
                sProfMixTicks += (uint64_t)(SNDPROF_NOW() - mixStart);
            }
            sTickPhase += SOUND_BUS_CLOCK * (uint64_t)n;
            if (sTickPhase >= SOUND_TICK_SAMPLES) {
                sTickPhase -= SOUND_TICK_SAMPLES;
                {
                    u64 tickStart = SNDPROF_NOW();
                    sound_tick();
                    sProfTickTicks += (uint64_t)(SNDPROF_NOW() - tickStart);
                }
                sProfSoundTicks += 1u;
            }
            sound_unlock();
            done += n;
            sProfStretches += 1u;
        }
        /* One cumulative line every 600 renders (~9.4 s of 512-frame blocks):
         * the lead's run reads the hog off it. Outside the lock above, on the
         * audio thread like the other audio logging. */
        if (sProfRenderCalls % 600u == 0u) {
            uint64_t mixCalls, mixFrames, activeCh, formatCh[4], capturing, dependentCh;
    
            NitroSound_MixProfile(&mixCalls, &mixFrames, &activeCh, formatCh,
                                  &capturing, &dependentCh);
            NitroPort_Log("[sndprof] render_calls %llu render_frames %llu stretches %llu sndticks %llu mix_ticks %llu tick_ticks %llu mix_calls %llu mix_frames %llu active_ch %llu pcm8_ch %llu pcm16_ch %llu adpcm_ch %llu psg_ch %llu cap_inst %llu dep_ch %llu",
                          (unsigned long long)sProfRenderCalls,
                          (unsigned long long)sProfRenderFrames,
                          (unsigned long long)sProfStretches,
                          (unsigned long long)sProfSoundTicks,
                          (unsigned long long)sProfMixTicks,
                          (unsigned long long)sProfTickTicks,
                          (unsigned long long)mixCalls,
                          (unsigned long long)mixFrames,
                          (unsigned long long)activeCh,
                          (unsigned long long)formatCh[0],
                          (unsigned long long)formatCh[1],
                          (unsigned long long)formatCh[2],
                          (unsigned long long)formatCh[3],
                          (unsigned long long)capturing,
                          (unsigned long long)dependentCh);
        }
    }
    
    /* Called by the game thread (e.g. its OnFrame hook), NOT the audio thread.
     * NitroPort_Arm7Reply invokes the virtual ARM9 FIFO interrupt/scheduler. */
    void NitroPort_SoundPumpReplies(void)
    {
        for (;;) {
            u32 msg;
            sound_lock();
            if (!sAlarmCount) {
                sound_unlock();
                break;
            }
            msg = sAlarmReplies[sAlarmHead];
            sAlarmHead = (sAlarmHead + 1u) % SOUND_ALARM_CAPACITY;
            --sAlarmCount;
            sound_unlock();
    ✓ • 1.31s

## Activity

    $ rg -n 'HGSS_PROFILE_OFF|sdk_includes|PORT_CFLAGS' tools/game/hgss_nitro.py; rg -n 's_(skipped_screens|reused_layers|rebuilt_layers|char_bytes_compared)|s_direct_words|s_stats.box' src/graphics/g2d/g2d.c src/runtime/kj_g3.c; Get-Content src/runtime/kj_frame.c | Select-Object -Skip 160 -First 100; Get-Content src/graphics/renderer.c | Select-Object -Skip 115 -First 45; rg -n 's_draw_ms|s_cmdbuf|s_perflog' src/graphics/renderer.c
    267:def sdk_includes(root, sdk, nitro, gen, msys_paths=False):
    291:PROFILE_OFF_DEFINE = "-DHGSS_PROFILE_OFF"
    301:    Makefiles append after PORT_CFLAGS); tools/hgss_game.py adds the same
    331:            "SDK_INCLUDES=" + " ".join(sdk_includes(root, sdk, nitro, gen, msys_paths=True)),
    src/graphics/g2d/g2d.c:848:static uint32_t s_skipped_screens, s_reused_layers, s_rebuilt_layers;
    src/graphics/g2d/g2d.c:858:static uint32_t s_char_bytes_compared;
    src/graphics/g2d/g2d.c:862:    s_char_bytes_compared += lc->char_len;
    src/graphics/g2d/g2d.c:941:        ++s_reused_layers;
    src/graphics/g2d/g2d.c:968:    ++s_rebuilt_layers;
    src/graphics/g2d/g2d.c:992:        ++s_reused_layers;
    src/graphics/g2d/g2d.c:1029:    ++s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1064:    const uint32_t rebuilt_before = s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1105:            ++s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1126:            ++s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1133:    pr->all_reused = retain && s_rebuilt_layers == rebuilt_before;
    src/graphics/g2d/g2d.c:1240:            ++s_skipped_screens;
    src/graphics/g2d/g2d.c:1317:    s_stats.skipped_screens = s_skipped_screens;
    src/graphics/g2d/g2d.c:1318:    s_stats.reused_layers = s_reused_layers;
    src/graphics/g2d/g2d.c:1319:    s_stats.rebuilt_layers = s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1320:    s_stats.char_bytes_compared = s_char_bytes_compared;
    src/graphics/g2d/g2d.c:1321:    s_skipped_screens = s_reused_layers = s_rebuilt_layers = 0;
    src/graphics/g2d/g2d.c:1322:    s_char_bytes_compared = 0;
    src/runtime/kj_g3.c:925:    ++s_stats.box_tests;
    src/runtime/kj_g3.c:960:        ++s_stats.box_culled;
    src/runtime/kj_g3.c:1330:static u32 s_fifo_words, s_direct_words;
    src/runtime/kj_g3.c:1342:    ++s_direct_words;
    src/runtime/kj_g3.c:2430:    *direct_words = s_direct_words;
    src/runtime/kj_g3.c:2433:    s_fifo_words = s_direct_words = 0;
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
            ++s_skipped_presents;
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
    
        ++s_frames;
        kj_perf_publish_end(in->game_frame);
        if (s_frames % 600u == 0u) {
            kj_g2d_stats g;
            kj_g3d_stats d;
            kj_atlas_stats a;
            kj_g2d_take_stats(&g);
            kj_g3d_take_stats(&d);
    49:static bool s_perflog_on;
    50:static GSPGPU_PerfLog s_perflog_prev;
    51:static float s_draw_ms_sum;
    52:static float s_draw_ms_max;
    53:static float s_cmdbuf_max;
    80:    s_perflog_on = R_SUCCEEDED(GSPGPU_SetPerfLogMode(true)) &&
    81:                   R_SUCCEEDED(GSPGPU_GetPerfLog(&s_perflog_prev));
    144:        s_draw_ms_sum += draw_ms;
    145:        if (draw_ms > s_draw_ms_max) {
    146:            s_draw_ms_max = draw_ms;
    148:        if (usage > s_cmdbuf_max) {
    149:            s_cmdbuf_max = usage;
    204:            if (s_perflog_on && R_SUCCEEDED(GSPGPU_GetPerfLog(&now))) {
    208:                       s_perflog_prev.entries[GSPGPU_EVENT_P3D].totalDurationUs) / 600u;
    210:                       s_perflog_prev.entries[GSPGPU_EVENT_PPF].totalDurationUs) / 600u;
    212:                       s_perflog_prev.entries[GSPGPU_EVENT_PSC0].totalDurationUs +
    214:                       s_perflog_prev.entries[GSPGPU_EVENT_PSC1].totalDurationUs) / 600u;
    215:                s_perflog_prev = now;
    219:                   p3d, ppf, psc, s_perflog_on ? "on" : "off", (double)(s_draw_ms_sum / 600.0f),
    220:                   (double)s_draw_ms_max, (unsigned)(s_cmdbuf_max * 100.0f));
    221:            s_draw_ms_sum = 0.0f;
    222:            s_draw_ms_max = 0.0f;
    223:            s_cmdbuf_max = 0.0f;
    ✓ • 1.46s

## Activity

    $ Get-Content tools/game/hgss_nitro.py | Select-Object -Skip 285 -First 38; rg -n 's_fifo_ticks|s_direct_words|s_stats.commands|kj_g3_take_profile' src/runtime/kj_g3.c; Get-Content src/diagnostics/perf.h | Select-Object -Last 22; Get-Content src/graphics/renderer.c | Select-Object -Skip 220 -First 20
                # undefined at link. hgss/compat/include/hgss_port_math.h declares the surface
                # with the standard prototypes (the system header is shadowed by the SDK's).
                "-include", f"{repo}/src/compat/hgss/include/hgss_port_math.h"] + profile_defines()
    
    
    PROFILE_OFF_DEFINE = "-DHGSS_PROFILE_OFF"
    
    
    def profile_defines():
        """Build switches that must reach the vendored libraries as well as the app.
    
        HGSS_PROFILE=off compiles the OFF build for timing claims (see
        PICA200-INSTRUMENTATION.md Â§B): the 2D tick reads and the geometry
        diagnostics disappear. The vendored Makefiles take their defines from
        common.mk, so the switch travels with the include flags (which the
        Makefiles append after PORT_CFLAGS); tools/hgss_game.py adds the same
        define to the app objects. Both builds otherwise share every flag.
        """
        return [] if os.environ.get("HGSS_PROFILE") == "on" else [PROFILE_OFF_DEFINE]
    
    
    def _make_out(subproject, out):
        """OUT= spelling for `make -C 3ds_port/<subproject>`.
    
        The vendored test recipes run binaries as `./$(TEST_BIN)`, so an absolute OUT=
        makes every test invocation `.//work/out/...`, which never executes. Pass OUT
        relative to the subproject directory instead; it names the same directory.
        GEN/LIBNITRO keep their absolute spellings (used only as paths, never after `./`).
        """
        if out is None:
            return None
        return Path(os.path.relpath(out, REPO / "src" / "compat" / subproject)).as_posix()
    
    
    def nitro_overrides(root, target, out=None, subdir="nitro"):
        sdk, nns = make_shim(root)
        keep, _ = sdk_sources(root)
        nitro = REPO / "src" / "compat" / "nitro"
    1101:    ++s_stats.commands;
    1329:static u64 s_fifo_ticks;
    1330:static u32 s_fifo_words, s_direct_words;
    1342:    ++s_direct_words;
    2186:    s_fifo_ticks += svcGetSystemTick() - t0;
    2423:void kj_g3_take_profile(uint64_t *fifo_ticks, uint32_t *fifo_words, uint32_t *direct_words,
    2428:    (void)s_fifo_ticks;
    2430:    *direct_words = s_direct_words;
    2431:    *verts = s_stats.commands;
    2432:    s_fifo_ticks = 0;
    2433:    s_fifo_words = s_direct_words = 0;
    2434:    s_stats.commands = 0;
        KJ_COUNT_TILE_CONV,  /* atlas tile conversions */
        KJ_COUNT_SPANS,      /* 2D spans drawn */
        KJ_COUNT_QUADS,      /* 2D quads built */
        KJ_COUNT_TEX_UPLOAD, /* 3D texture conversions */
        KJ_COUNT_TEX_BYTES,  /* 3D texture bytes compared */
        KJ_COUNT_COUNT
    };
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
    
    #endif
                s_draw_ms_sum = 0.0f;
                s_draw_ms_max = 0.0f;
                s_cmdbuf_max = 0.0f;
            }
        }
    
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
    ✓ • 1.34s

## Activity

    $ @'
        from pathlib import Path
        p=Path('src/diagnostics/perf.h'); s=p.read_text(); s=s.replace('extern uint32_t kj_perf_counts', '#ifndef HGSS_PROFILE_OFF\nextern uint32_t kj_perf_counts'); s=s.replace('\n#endif\n', '''
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
        '''); p.write_text(s)
        p=Path('src/diagnostics/perf.c'); s=p.read_text(); s='#ifndef HGSS_PROFILE_OFF\n'+s+'\n#endif\n'; p.write_text(s)
        p=Path('src/graphics/renderer.c'); s=p.read_text(); s=s.replace('    s_perflog_on = R_SUCCEEDED', '#ifndef HGSS_PROFILE_OFF\n    s_perflog_on = R_SUCCEEDED').replace('R_SUCCEEDED(GSPGPU_GetPerfLog(&s_perflog_prev));','R_SUCCEEDED(GSPGPU_GetPerfLog(&s_perflog_prev));\n#endif'); s=s.replace('    {\n        const float draw_ms', '#ifndef HGSS_PROFILE_OFF\n    {\n        const float draw_ms'); s=s.replace('    ++s_frames;\n    kj_perf_publish_end', '#endif\n    ++s_frames;\n    kj_perf_publish_end'); s=s.replace('    if (s_frames % 600u == 0u) {','#ifndef HGSS_PROFILE_OFF\n    if (s_frames % 600u == 0u) {'); s=s.replace('    /* Test directives: deterministic per-frame capture. */','#endif\n    /* Test directives: deterministic per-frame capture. */'); p.write_text(s)
        p=Path('src/runtime/kj_frame.c'); s=p.read_text(); s=s.replace('        const u64 t0 = svcGetSystemTick();\n        if (NitroPort_HBlankLinesOnce())', '#ifndef HGSS_PROFILE_OFF\n        const u64 t0 = svcGetSystemTick();\n#endif\n        if (NitroPort_HBlankLinesOnce())'); s=s.replace('            ++s_hblank_walks;','#ifndef HGSS_PROFILE_OFF\n            ++s_hblank_walks;\n#endif'); s=s.replace('        s_hblank_ticks += svcGetSystemTick() - t0;','#ifndef HGSS_PROFILE_OFF\n        s_hblank_ticks += svcGetSystemTick() - t0;\n#endif'); s=s.replace('        const u64 t0 = svcGetSystemTick();\n        NitroPort_SoundPumpReplies();', '#ifndef HGSS_PROFILE_OFF\n        const u64 t0 = svcGetSystemTick();\n#endif\n        NitroPort_SoundPumpReplies();'); s=s.replace('        s_pump_ticks += svcGetSystemTick() - t0;\n        ++s_pump_calls;', '#ifndef HGSS_PROFILE_OFF\n        s_pump_ticks += svcGetSystemTick() - t0;\n        ++s_pump_calls;\n#endif'); p.write_text(s)
        p=Path('src/compat/nitro/src/port_snd.c'); s=p.read_text(); s=s.replace('#ifdef __3DS__\nextern u64 svcGetSystemTick', '#if defined(__3DS__) && !defined(HGSS_PROFILE_OFF)\nextern u64 svcGetSystemTick');
        for line in ['    sProfRenderCalls += 1u;\n    sProfRenderFrames += (uint64_t)frames;', '            u64 mixStart = SNDPROF_NOW();', '            sProfMixTicks += (uint64_t)(SNDPROF_NOW() - mixStart);', '                u64 tickStart = SNDPROF_NOW();', '                sProfTickTicks += (uint64_t)(SNDPROF_NOW() - tickStart);', '            sProfSoundTicks += 1u;', '        sProfStretches += 1u;']:
         s=s.replace(line,'#ifndef HGSS_PROFILE_OFF\n'+line+'\n#endif')
        s=s.replace('    if (sProfRenderCalls % 600u == 0u) {','#ifndef HGSS_PROFILE_OFF\n    if (sProfRenderCalls % 600u == 0u) {'); s=s.replace('\n}\n\n/* Called by the game thread', '\n#endif\n}\n\n/* Called by the game thread'); p.write_text(s)
        p=Path('src/compat/nitro/src/port_sound_chip.c'); s=p.read_text();
        for line in ['    sProfMixCalls += 1u;\n    sProfMixFrames += (uint64_t)frames;\n    sProfActiveCh += (uint64_t)count;\n    for (k = 0; k < count; ++k) sProfFormatCh[channels[k].format] += 1u;', '            sProfCapturing += 1u;', '                    sProfDependentCh += 1u;']:
         s=s.replace(line,'#ifndef HGSS_PROFILE_OFF\n'+line+'\n#endif')
        p.write_text(s)
        # Pure reporting counters only; swaps and rebuilt-layer counts are functional.
        for name in ['src/graphics/resources/atlas.c','src/graphics/g3d/textures.c','src/graphics/g3d/g3d.c','src/graphics/g2d/g2d.c','src/runtime/kj_g3.c']:
         p=Path(name); s=p.read_text(); lines=s.splitlines(True); out=[]
         import re
         for line in lines:
          pure = re.match(r'^\s*(?:\+\+s_stats\.(?!swaps)|s_stats\.(?:triangles|batches|unsupported|box_tests|box_culled|box_culled_far|bg_quads|obj_quads|draws|spans|char_bytes_compared)\s*\+=|\+\+s_dl_(?:why\[\d\]|hits|misses|uncacheable)|s_dl_cov\[[^]]+\]\s*\+=|\+\+s_fifo_calls|s_fifo_words\s*\+=|\+\+s_direct_words)',line)
          if pure: out.extend(['#ifndef HGSS_PROFILE_OFF\n',line,'#endif\n'])
          else: out.append(line)
         p.write_text(''.join(out))
        '@ | Set-Content build/apply-profile-split.py
        python build/apply-profile-split.py
         git diff --stat
     README.md                              | 11 ++++---
     src/compat/nitro/src/port_snd.c        | 18 ++++++++++-
     src/compat/nitro/src/port_sound_chip.c |  6 ++++
     src/diagnostics/crash.c                |  2 +-
     src/diagnostics/log.c                  |  6 +++-
     src/diagnostics/perf.c                 |  3 ++
     src/diagnostics/perf.h                 | 12 +++++++
     src/graphics/g2d/g2d.c                 | 30 ++++++++++++++++++
     src/graphics/g3d/g3d.c                 | 22 +++++++++++++
     src/graphics/g3d/textures.c            | 16 ++++++++++
     src/graphics/renderer.c                |  6 ++++
     src/graphics/resources/atlas.c         |  8 +++++
     src/runtime/kj_frame.c                 | 10 ++++++
     src/runtime/kj_g3.c                    | 58 ++++++++++++++++++++++++++++++++++
     14 files changed, 200 insertions(+), 8 deletions(-)
    ✓ • 1.66s

## Assistant

The split is now being wired through both the application and vendored sound library. One important detail is that some apparent counters also control renderer behavior, so I’m retaining those functional state variables while removing measurement-only increments and tick reads from the normal build.

## Activity

    $ @'
        from pathlib import Path
        p=Path('src/runtime/kj_g3.c'); s=p.read_text(); anchor='static void dl_replay(dl_entry *e)'; s=s.replace(anchor,'''#ifndef HGSS_PROFILE_OFF
        /* Sample one FIFO call in 64. Nested portions are timed only in sampled
         * calls; the remainder includes lookup, comparison, recording and probe
         * overhead. Wall time includes preemption. */
        static u32 s_coarse_seq, s_coarse_n, s_coarse_depth;
        static bool s_coarse_sample;
        static u64 s_coarse_total, s_coarse_decode, s_coarse_replay;
        #endif
        
        static void dl_replay_impl(dl_entry *e)'''); anchor='static uint32_t dl_vertex_bound(const dl_entry *e)'; s=s.replace(anchor,'''static void dl_replay(dl_entry *e)
        {
        #ifndef HGSS_PROFILE_OFF
            const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
        #endif
            dl_replay_impl(e);
        #ifndef HGSS_PROFILE_OFF
            if (s_coarse_sample) s_coarse_replay += svcGetSystemTick() - t;
        #endif
        }
        
        '''+anchor); s=s.replace('static void decode_words(const uint32_t *words, size_t count)','static void decode_words_impl(const uint32_t *words, size_t count)'); anchor='void NitroPort_GxFifoWords(const uint32_t *words, size_t count)'; s=s.replace(anchor,'''static void decode_words(const uint32_t *words, size_t count)
        {
        #ifndef HGSS_PROFILE_OFF
            const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
        #endif
            decode_words_impl(words, count);
        #ifndef HGSS_PROFILE_OFF
            if (s_coarse_sample) s_coarse_decode += svcGetSystemTick() - t;
        #endif
        }
        
        static void fifo_words_impl(const uint32_t *words, size_t count)'''); anchor='void kj_g3_dl_stats('; pos=s.find(anchor); assert pos>=0; s=s[:pos]+'''void NitroPort_GxFifoWords(const uint32_t *words, size_t count)
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
        
        '''+s[pos:]; marker='    *hits = s_dl_hits;'; s=s.replace(marker,'''#ifndef HGSS_PROFILE_OFF
            NitroPort_Log("[g3sample] every 64th FIFO call: samples %lu total_ticks %llu decode_ticks %llu replay_ticks %llu; remainder includes lookup/recording and probe overhead",
                          (unsigned long)s_coarse_n, (unsigned long long)s_coarse_total,
                          (unsigned long long)s_coarse_decode, (unsigned long long)s_coarse_replay);
            s_coarse_n = 0;
            s_coarse_total = s_coarse_decode = s_coarse_replay = 0;
        #endif
        '''+marker); p.write_text(s)
        '@ | Set-Content build/add-coarse-profile.py
        python build/add-coarse-profile.py
        rg -n 'error:|vendored library build failed|packaged' build/normal-build.log | Select-Object -Last 15
    ✗ (1) • 2.75s

## Activity

    $ Get-Content build/normal-build.log -Tail 12; git diff --check -- src; rg -n 's_stats\.|s_dl_cov|s_dl_why|s_fifo' src/runtime/kj_g3.c | Select-Object -Last 40; rg -n 'profile_mode|build.info|flags' tools/game/hgss_game.py | Select-Object -Last 25
    fx_const: lib/include/nitro/fx/fx_const.h (unchanged)
    trname: files/msgdata/msg/msg_0729.gmm (unchanged)
    msg headers: 829 (0 updated)
    nitroarc naix: 38 (0 updated)
    o2narc naix: 14 (0 updated)
    csv2bin naix: 3 (0 updated)
    alias naix: 2 (0 updated)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/normal/gen/overlay_ids.ld (updated)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/normal/gen/overlay_ids.h (updated)
    done: 890 files, 2 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
    2243:    s_fifo_words += (u32)count;
    2253:    if (count < DL_MIN_WORDS || count > 0xFFFFFu || s_in_begin || s_fifo_need > 0 || s_rec.active) {
    2255:        s_dl_cov[DLC_SHORT] += (uint32_t)count;
    2259:            s_dl_cov[DLC_SMALL] += (uint32_t)count;
    2261:        } else if (s_fifo_need > 0) {
    2263:            s_dl_cov[DLC_SPLIT] += (uint32_t)count;
    2267:            s_dl_cov[DLC_BEGIN] += (uint32_t)count;
    2271:                s_dl_cov[DLC_BEGIN_FIRST] += (uint32_t)count;
    2285:        s_dl_cov[DLC_NEW] += (uint32_t)count;
    2299:    } else if (e->never && (s_stats.swaps & DL_NEVER_RECHECK_MASK) != 0u) {
    2303:        e->last_frame = s_stats.swaps;
    2305:        s_dl_cov[DLC_NEVER] += (uint32_t)count;
    2314:        s_dl_cov[DLC_CHANGED] += (uint32_t)count;
    2322:    e->last_frame = s_stats.swaps;
    2325:        s_dl_cov[DLC_NEVER] += (uint32_t)count;
    2352:            s_dl_cov[DLC_HIT] += (uint32_t)count;
    2362:        if (logged < 30 && s_stats.swaps > 4600u) {
    2379:        s_dl_cov[DLC_HIT] += (uint32_t)count;
    2383:        s_dl_cov[DLC_KEY] += (uint32_t)count;
    2409:        s_fifo_need > 0) {
    2412:        ++s_dl_why[2];
    2418:        ++s_dl_why[3];
    2475:            ++s_dl_why[5];
    2522:                  (unsigned long)s_dl_why[0], (unsigned long)s_dl_why[1], (unsigned long)s_dl_why[2],
    2523:                  (unsigned long)s_dl_why[3], (unsigned long)s_dl_why[4], (unsigned long)s_dl_why[5],
    2526:                  (unsigned long)s_dl_cov[DLC_SMALL], (unsigned long)s_dl_cov[DLC_SPLIT],
    2527:                  (unsigned long)s_dl_cov[DLC_BEGIN], (unsigned long)s_dl_cov[DLC_BEGIN_FIRST]);
    2529:                  (unsigned long)s_dl_cov[DLC_SHORT], (unsigned long)s_dl_cov[DLC_NEW],
    2530:                  (unsigned long)s_dl_cov[DLC_CHANGED], (unsigned long)s_dl_cov[DLC_NEVER],
    2531:                  (unsigned long)s_dl_cov[DLC_HIT], (unsigned long)s_dl_cov[DLC_KEY]);
    2532:    memset(s_dl_cov, 0, sizeof(s_dl_cov));
    2533:    memset(s_dl_why, 0, sizeof(s_dl_why));
    2543:    *fifo_ticks = (uint64_t)s_fifo_calls; /* calls; ticks only with KJ_G3_PROFILE */
    2544:    s_fifo_calls = 0;
    2545:    (void)s_fifo_ticks;
    2546:    *fifo_words = s_fifo_words;
    2548:    *verts = s_stats.commands;
    2549:    s_fifo_ticks = 0;
    2550:    s_fifo_words = s_direct_words = 0;
    2551:    s_stats.commands = 0;
    1518:    (port_gx3d.c). Make does not track environment flags; the marker below
    1525:def profile_mode() -> str:
    1533:def profile_mode_matches(out: Path) -> bool:
    1535:    return marker.is_file() and marker.read_text().strip() == profile_mode()
    1560:    # Make does not track changed environment flags. Invalidate the one
    1587:    prof_mode = profile_mode()
    1680:    # Commit identity for the boot banner + build-info sidecar (log.txt
    1683:    build_info = git_build_info()
    1684:    build_id = write_build_id_header(gen, build_info)
    1700:    cflags = port_cflags(args.target)
    1701:    cxxflags = port_cflags(args.target, cxx=True)
    1702:    includes = include_flags(root, out)
    1704:    cflags += includes + ["-include", str(overlay_header)]
    1705:    cxxflags += includes + ["-include", str(overlay_header)]
    1713:    c_objects, c_failed = compile_objects(tools["cc"], cflags, c_jobs, args.jobs,
    1717:    cxx_objects, cxx_failed = compile_objects(tools["cc"], cxxflags, cxx_jobs,
    1731:    aflags = asm_flags(args.target, root, out)
    1736:        root, tools["cc"], aflags, out, sources["asm"], args.jobs, log_dir,
    1751:    if not profile_mode_matches(out):
    1806:    app_cflags = list(cflags) + [f"-I{REPO / 'src' / 'compat' / 'pak' / 'include'}",
    1809:        tools["cc"], app_cflags, app_jobs, args.jobs, log_dir, args.keep_going)
    1815:            tools["cc"], ctr_cflags(out), ctr_jobs, args.jobs, log_dir, args.keep_going)
    1833:        tools["cc"], port_cflags(args.target) + includes,
    1873:            manifest = write_build_info(dsx, args.game, args.target, build_info)
    1874:            print(f"   build info: {manifest.name}")
    ✓ • 2.36s

## Activity

    $ rg -n -A35 'def ctr_cflags|def profile_mode_matches|def write_build_info' tools/game/hgss_game.py; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 2500 -First 45; Get-Content src/graphics/g3d/g3d.c | Select-Object -Skip 260 -First 55
    370:def ctr_cflags(out: Path) -> list[str]:
    371-    return ["-std=gnu11", "-O2", "-g", *WARNINGS, *SECTION_FLAGS,
    372-            "-Wno-unused-function", *ARM_FLAGS, *game_defines(),
    373-            f"-I{devkitpro() / 'libctru' / 'include'}",
    374-            f"-I{REPO / 'src'}", f"-I{REPO / 'src' / 'runtime'}", f"-I{out / 'gen'}"]
    375-
    376-
    377-def shader_sources() -> list[Path]:
    378-    return sorted((REPO / "src" / "graphics" / "pica" / "shaders").glob("*.v.pica"))
    379-
    380-
    381-def shader_objects(out: Path) -> list[Path]:
    382-    """One object per .v.pica: picasso -> .shbin -> bin2s -> as. Each blob is
    383-    exported as <stem>_shbin / <stem>_shbin_size (stem with dots -> '_')."""
    384-    objs = []
    385-    for src in shader_sources():
    386-        objs.append(_one_shader_object(out, src))
    387-    return objs
    388-
    389-
    390-def object_path(out: Path, rel: str) -> Path:
    391-    """out/obj/<checkout-relative path with .o> (main.lsf's object spelling)."""
    392-    return out / "obj" / Path(rel).with_suffix(".o")
    393-
    394-
    395-# The PICA vertex programs port_gx3d_pica.c expects, in DVLE order: the
    396-# polygons, the sun shadows' map, the sun shadows' scene. They are Platinum's
    397-# (3ds_port/ is byte-identical), assembled by picasso and embedded by bin2s.
    398-
    399-
    400-def devkit_tool(*parts: str) -> Path:
    401-    """A devkitPro tool path, with .exe resolved on native Windows.
    402-
    403-    The bare names (`picasso`, `3dsxtool`) never exist as files there, so the
    404-    `is_file()` gates would misreport every tool as missing.
    405-    """
    --
    1077:def write_build_info(dsx: Path, game: str, target: str,
    1078-                     info: tuple[str, str, bool, str]) -> Path:
    1079-    """Write the .build-info.txt sidecar next to a packaged .3dsx.
    1080-
    1081-    Pairs the artifact with its commit, dirty state and sha256 while still
    1082-    in --out (this module never writes outside --out); tools/stage_3ds.py
    1083-    carries it to the staged copies.
    1084-    """
    1085-    short, full, dirty, status = info
    1086-    manifest = dsx.with_name(dsx.stem + ".build-info.txt")
    1087-    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    1088-    digest = hashlib.sha256(dsx.read_bytes()).hexdigest()
    1089-    body = (f"game: {game}\n"
    1090-            f"target: {target}\n"
    1091-            f"texture_profile: {'yes' if target == '3ds' and os.environ.get('HGSS_TEXTURE_PROFILE') == '1' else 'no'}\n"
    1092-             f"texture_hint: {'yes' if target == '3ds' and os.environ.get('HGSS_TEXTURE_HINT') == '1' else 'no'}\n"
    1093-             f"palette_hash: {'yes' if os.environ.get('HGSS_PALETTE_HASH') == '1' else 'no'}\n"
    1094-             f"gx3d_hotloop: {'yes' if target == '3ds' and os.environ.get('HGSS_GX3D_HOTLOOP') == '1' else 'no'}\n"
    1095-             f"gx3d_indexed: {'yes' if target == '3ds' and os.environ.get('HGSS_GX3D_INDEXED') == '1' else 'no'}\n"
    1096-             f"gx3d_listcache: {'yes' if target == '3ds' and os.environ.get('HGSS_GX3D_LISTCACHE') == '1' else 'no'}\n"
    1097-             f"gx3d_statededup: {'yes' if target == '3ds' and os.environ.get('HGSS_GX3D_STATEDEDUP') == '1' else 'no'}\n"
    1098-             f"gx3d_fusehash: {'yes' if target == '3ds' and os.environ.get('HGSS_GX3D_FUSEHASH') == '1' else 'no'}\n"
    1099-             f"gx3d_lazyinsert: {'yes' if target == '3ds' and os.environ.get('HGSS_GX3D_LAZYINSERT') == '1' else 'no'}\n"
    1100-             f"gx3d_bg3slot: {'yes' if target == '3ds' and os.environ.get('HGSS_GX3D_BG3SLOT') == '1' else 'no'}\n"
    1101-             f"snd_adpcm: {'yes' if target == '3ds' and os.environ.get('HGSS_SND_ADPCM') == '1' else 'no'}\n"
    1102-             f"bench: {'yes' if target == '3ds' and os.environ.get('HGSS_BENCH') == '1' else 'no'}\n"
    1103-             f"a0_diag: {'yes' if target == '3ds' and os.environ.get('HGSS_A0_DIAG') == '1' else 'no'}\n"
    1104-            f"build: {experiment_build_id(short, dirty)}\n"
    1105-            f"sha: {full}\n"
    1106-            f"dirty: {'yes' if dirty else 'no'}\n"
    1107-            f"built_utc: {now}\n"
    1108-            f"file: {dsx.name}\n"
    1109-            f"size: {dsx.stat().st_size}\n"
    1110-            f"sha256: {digest}\n"
    1111-            f"status:\n{status}")
    1112-    if not body.endswith("\n"):
    --
    1533:def profile_mode_matches(out: Path) -> bool:
    1534-    marker = out / "sdk" / ".profile-mode"
    1535-    return marker.is_file() and marker.read_text().strip() == profile_mode()
    1536-
    1537-
    1538-def bench_mode() -> str:
    1539-    """The automated-benchmark mode compiled into the 3DS shell.
    1540-
    1541-    HGSS_BENCH=1 builds a test artifact with the hb:ldr chainload path
    1542-    (docs/autobench.md). Anything else builds the normal shell with no
    1543-    chainloading code at all."""
    1544-    return "bench" if os.environ.get("HGSS_BENCH") == "1" else "normal"
    1545-
    1546-
    1547-def bench_mode_matches(out: Path) -> bool:
    1548-    marker = out / ".bench-mode"
    1549-    return marker.is_file() and marker.read_text().strip() == bench_mode()
    1550-
    1551-
    1552-def build_libraries(root: Path, target: str, out: Path, jobs: int) -> tuple[bool, list[Path]]:
    1553-    """libnitro.a and libnitrosystem.a through tools/hgss_nitro.py.
    1554-
    1555-    Returns whether both built and the archives that exist. A failed archive
    1556-    does not stop the harness: the link then reports the symbols it would
    1557-    have provided, which is part of the Phase 2 work queue.
    1558-    """
    1559-    sdk_out = out / "sdk"
    1560-    # Make does not track changed environment flags. Invalidate the one
    1561-    # affected generated object whenever this experiment is toggled, including
    1562-    # an existing output that predates the marker, so the sidecar is truthful.
    1563-    palette_mode = "1" if os.environ.get("HGSS_PALETTE_HASH") == "1" else "0"
    1564-    palette_marker = sdk_out / ".palette-hash-mode"
    1565-    if not palette_marker.exists() or palette_marker.read_text().strip() != palette_mode:
    1566-        (sdk_out / "port" / "port_gx3d_texture.o").unlink(missing_ok=True)
    1567-    gx3d_mode = gx3d_sdk_mode()
    1568-    gx3d_marker = sdk_out / ".gx3d-mode"
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
    #ifndef HGSS_PROFILE_OFF
            ++s_stats.draws;
    #endif
    #ifndef HGSS_PROFILE_OFF
            s_stats.triangles += b->index_count / 3u;
    #endif
        }
    }
    
    void kj_g3d_draw(const kj_target *target, bool wide, uint8_t stencil_flags)
    {
        const kj_g3_frame *f = s_frame;
        (void)wide;
        if (f == NULL || f->batch_count == 0) {
            return;
        }
        uint64_t t0 = kj_perf_now();
    #ifndef HGSS_PROFILE_OFF
        s_stats.batches += f->batch_count;
    #endif
    #ifndef HGSS_PROFILE_OFF
        s_stats.unsupported += f->unsupported + f->dropped_batches + f->dropped_verts;
    #endif
    #ifndef HGSS_PROFILE_OFF
        s_stats.box_tests += f->box_tests;
    #endif
    #ifndef HGSS_PROFILE_OFF
        s_stats.box_culled += f->box_culled;
    #endif
    #ifndef HGSS_PROFILE_OFF
        s_stats.box_culled_far += f->box_culled_far;
    #endif
        if (f->disp3dcnt & (1u << 7)) {
    #ifndef HGSS_PROFILE_OFF
            ++s_stats.unsupported; /* fog */
    #endif
        }
        if (f->disp3dcnt & (1u << 5)) {
    #ifndef HGSS_PROFILE_OFF
            ++s_stats.unsupported; /* edge marking (baseline excluded) */
    #endif
        }
    
        /* Invalidate first (textures are bound directly below), then bind the
         * 3D program once; binding before the invalidate bound it twice. */
        kj_gpu_invalidate();
        kj_gpu_use_program(KJ_PROGRAM_3D);
        s_last_tev = -1;
        s_last_tex = NULL;
        s_last_pal = NULL;
        C3D_BufInfo *bi = C3D_GetBufInfo();
        BufInfo_Init(bi);
        BufInfo_Add(bi, f->verts, sizeof(kj_g3_vertex), 4, 0x2130);
    
        C3D_DepthMap(!kj_g3d_wbuffer(), -1.0f, 0.0f);
    ✓ • 2.48s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\tools\game\hgss_game.py
    @@ -372,2 +372,4 @@
                 "-Wno-unused-function", *ARM_FLAGS, *game_defines(),
    +            *(["-DHGSS_PROFILE_OFF"] if profile_mode() == "off" else []),
    +            *[f"-D{d}" for d in EXTRA_DEFINES],
                 f"-I{devkitpro() / 'libctru' / 'include'}",
    @@ -1090,2 +1092,4 @@
                 f"target: {target}\n"
    +            f"profiling: {profile_mode()}\n"
    +            f"defines: {', '.join(EXTRA_DEFINES) or '(none)'}\n"
                 f"texture_profile: {'yes' if target == '3ds' and os.environ.get('HGSS_TEXTURE_PROFILE') == '1' else 'no'}\n"

## Activity

    $ Get-Content build/normal-build.log -Tail 12; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 15; rg -n 's_dl_flushes|s_char_bytes_compared|s_skipped_screens|s_reused_layers' src/runtime/kj_g3.c src/graphics/g2d/g2d.c; rg -n 'svcGetSystemTick|sProf|[Pp]rofil|[Ss]tat' src/compat/nitro/src/port_os.c | Select-Object -First 80
      ARM7 CC snd_arm7_data.c
      ARM7 CC sound_shared_work.c
    C:/Users/terezi/workdir/kohjoh/build/hgss-sdk/libraries/hgss_sub/sound_seq_player.c: In function 'ReadArg':
    C:/Users/terezi/workdir/kohjoh/build/hgss-sdk/libraries/hgss_sub/sound_seq_player.c:431:12: warning: 'result' may be used uninitialized [-Wmaybe-uninitialized]
      431 |     return result;
          |            ^~~~~~
    C:/Users/terezi/workdir/kohjoh/build/hgss-sdk/libraries/hgss_sub/sound_seq_player.c:393:9: note: 'result' was declared here
      393 |     int result;
          |         ^~~~~~
    make: Target 'lib' not remade because of errors.
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitro'
    + /usr/bin/python /c/Users/terezi/workdir/kohjoh/tools/game/hgss_nitro.py make --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/normal/sdk --subproject nitrosystem lib -k -j 32
    534:src/port_sound_chip.c:916:5: error: expected expression before '}' token
    src/graphics/g2d/g2d.c:866:static uint32_t s_skipped_screens, s_reused_layers, s_rebuilt_layers;
    src/graphics/g2d/g2d.c:876:static uint32_t s_char_bytes_compared;
    src/graphics/g2d/g2d.c:880:    s_char_bytes_compared += lc->char_len;
    src/graphics/g2d/g2d.c:959:        ++s_reused_layers;
    src/graphics/g2d/g2d.c:1010:        ++s_reused_layers;
    src/graphics/g2d/g2d.c:1264:            ++s_skipped_screens;
    src/graphics/g2d/g2d.c:1347:    s_stats.skipped_screens = s_skipped_screens;
    src/graphics/g2d/g2d.c:1348:    s_stats.reused_layers = s_reused_layers;
    src/graphics/g2d/g2d.c:1350:    s_stats.char_bytes_compared = s_char_bytes_compared;
    src/graphics/g2d/g2d.c:1351:    s_skipped_screens = s_reused_layers = s_rebuilt_layers = 0;
    src/graphics/g2d/g2d.c:1352:    s_char_bytes_compared = 0;
    src/runtime/kj_g3.c:1738:static uint32_t s_dl_flushes;
    src/runtime/kj_g3.c:1761:    ++s_dl_flushes;
    src/runtime/kj_g3.c:2524:                  (unsigned long)s_dl_flushes);
    src/runtime/kj_g3.c:2534:    s_dl_flushes = 0;
    4: * - The status register (interrupt enable bits, CPU mode) is a variable.
    27:extern u64 svcGetSystemTick(void);
    28:#define PORT_NOW() svcGetSystemTick()
    47:static u64 sHBlankIrqBitTicks[NITRO_PORT_IRQ_BITS];
    48:static u32 sHBlankIrqBitCalls[NITRO_PORT_IRQ_BITS];
    49:static int sHBlankWalking;
    60:static void raise_irq(u32 mask, int refresh)
    119:static u64 sHBlankKeepTicks, sHBlankDmaTicks, sHBlankIrqTicks;
    123:static u64 sHBlankRefreshTicks, sHBlankSwitchTicks, sHBlankCmpTicks;
    128:static u64 sHBlankScans, sHBlankScanMismatch;
    134:static void KeepLine(int y)
    147:static int LineRegsDiffer(int engine, int y, int y0)
    168:static void HBlank(int line, BOOL irq, BOOL dma)
    188:static BOOL HBlankIrq(void)
    329:static u32 sVBlanks, sLinesVBlank = 0xFFFFFFFFu;
    364:static void StartVBlank(void)
    380:static u64 sWorkCycles;
    440:static BOOL sResetInitialized;
    469:static OSExceptionHandler sUserExceptionHandler;
    470:static void *sUserExceptionArg;
    ✓ • 1.75s

## Activity

    $ Get-Content src/compat/nitro/src/port_sound_chip.c | Select-Object -Skip 900 -First 40; Get-Content build/normal-build.log | Select-Object -Skip 527 -First 14; rg -n 'PORT_NOW|sHBlank.*\+\+|sHBlank.*\+=' src/compat/nitro/src/port_os.c
         * right output plays (the mixer, channel 1, channel 3 or both), 12 / 13
         * channel 1 / 3 kept out of the mixer, 15 on. */
        control = reg8(0x501u);
        masterVolume = (int32_t)(reg8(0x500u) & 127u);
        leftFrom = control & 3u;
        rightFrom = (control >> 2) & 3u;
        bypass1 = (control & 0x10u) != 0;
        bypass3 = (control & 0x20u) != 0;
        capturing = ((reg8(0x508u) | reg8(0x509u)) & 128u) ||
                    chip->capture[0].running || chip->capture[1].running;
        for (channel = 0; channel < 2u; ++channel) {
            if ((reg8(0x508u + channel) & 128u) || chip->capture[channel].running)
    #ifndef HGSS_PROFILE_OFF
                sProfCapturing += 1u;
    #endif
        }
    
        if (capturing) {
            /* A channel that reads a capture's buffer (reverb plays the
             * recorded mix back on channels 1 and 3) depends on what the
             * capture writes frame by frame: those are mixed frame by frame,
             * in their place before the capture. The others don't depend on
             * anything in the mix and go a block at a time like the rest. */
            unsigned slot;
            for (slot = 0; slot < 2; ++slot) {
                uint32_t dst = reg32(0x510u + slot * 8u);
                uint32_t size = (uint32_t)reg16(0x514u + slot * 8u) * 4u;
                if (!dst || !size) continue;
                for (k = 0; k < count; ++k) {
                    Channel *c = &channels[k];
                    if (c->format == 3u || !c->source) continue;
                    if (c->sourceAddr < dst + size && dst < c->sourceAddr + c->endByte) {
                        c->dependent = 1;
    #ifndef HGSS_PROFILE_OFF
                        sProfDependentCh += 1u;
    #endif
                    }
                }
            }
        }
      CC  port_pm.c
      CC  port_overlay.c
      CC  port_snd.c
      CC  port_thread.c
      CC  port_sound_chip.c
    src/port_sound_chip.c: In function 'NitroSound_Mix':
    src/port_sound_chip.c:916:5: error: expected expression before '}' token
      916 |     }
          |     ^
      CC  port_timer.c
      CC  port_tp.c
    make: *** [Makefile:256: ../../../build/normal/sdk/port/port_sound_chip.o] Error 1
      CC  pxi.c
      ARM7 CC sound_command.c
    28:#define PORT_NOW() svcGetSystemTick()
    30:#define PORT_NOW() 0
    95:            u64 t = PORT_NOW();
    99:                sHBlankIrqBitTicks[bit] += PORT_NOW() - t;
    100:                sHBlankIrqBitCalls[bit]++;
    173:        u64 t = PORT_NOW();
    176:        sHBlankDmaTicks += PORT_NOW() - t;
    179:        u64 t = PORT_NOW();
    182:        sHBlankIrqTicks += PORT_NOW() - t;
    220:    { u64 tR = PORT_NOW(); NitroPort_GxRefresh(); sHBlankRefreshTicks += PORT_NOW() - tR; }
    236:        t0 = PORT_NOW();
    250:        t1 = PORT_NOW();
    251:        sHBlankKeepTicks += t1 - t0;
    259:        u64 tS = PORT_NOW();
    264:        sHBlankSwitchTicks += PORT_NOW() - tS;
    281:        sHBlankScans++;
    283:            sHBlankScanMismatch++;
    ✓ • 2.50s

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/normal -j 32 > /c/Users/terezi/workdir/kohjoh/build/normal-build.log 2>&1'
    ✗ (1) • 1m 31s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\compat\nitro\src\port_sound_chip.c
    @@ -910,8 +910,8 @@
                     chip->capture[0].running || chip->capture[1].running;
    +#ifndef HGSS_PROFILE_OFF
         for (channel = 0; channel < 2u; ++channel) {
             if ((reg8(0x508u + channel) & 128u) || chip->capture[channel].running)
    -#ifndef HGSS_PROFILE_OFF
                 sProfCapturing += 1u;
    +    }
     #endif
    -    }
     

## Activity

    $ Get-Content src/compat/nitro/src/port_os.c | Select-Object -First 330; rg -n 'svcGetSystemTick|[Pp]rofil' src/compat/nitro/src/port_timer.c
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
        memset(sHBlankIrqBitTicks, 0, sizeof(sHBlankIrqBitTicks));
        memset(sHBlankIrqBitCalls, 0, sizeof(sHBlankIrqBitCalls));
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
        sHBlankWalking = 1;
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
        sHBlankWalking = 0;
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
         * agree, on every walk: the old way is computed here and the two
         * answers compared. (The line-register array itself is unchanged by
         * the change: the same KeepLine calls, in the same order.) */
        {
            int differ = 0;
    
            for (y = 1; y < NITRO_PORT_SCREEN_H && !differ; y++) {
                differ = memcmp(NitroPort_LineRegs.regs[0][y], NitroPort_LineRegs.regs[0][0], NITRO_LINE_REGS) != 0 ||
                         memcmp(NitroPort_LineRegs.regs[1][y], NitroPort_LineRegs.regs[1][0], NITRO_LINE_REGS) != 0;
            }
            sHBlankScans++;
            if (differ != NitroPort_LineRegs.active) {
                sHBlankScanMismatch++;
            }
        }
    #endif
    }
    
    /* The last walk's three parts, in ARM11 ticks (NitroPort_HBlankLines). */
    void NitroPort_HBlankSplit(u64 *keep, u64 *dma, u64 *irq)
    {
        *keep = sHBlankKeepTicks;
        *dma = sHBlankDmaTicks;
        *irq = sHBlankIrqTicks;
    }
    #ifndef HGSS_PROFILE_OFF
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
    
    35:extern u64 svcGetSystemTick(void);
    36:#define PORT_NOW() svcGetSystemTick()
    ✓ • 1.53s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\compat\nitro\src\port_os.c
    @@ -90,2 +90,3 @@
             OSi_PortCpsr = (saved & ~HW_PSR_CPU_MODE_MASK) | HW_PSR_IRQ_MODE | HW_PSR_IRQ_DISABLE;
    +#ifndef HGSS_PROFILE_OFF
             if (sHBlankWalking) {
    @@ -104,2 +105,5 @@
             }
    +#else
    +        OS_IrqHandler();
    +#endif
             OSi_PortCpsr = saved;

## Activity

    $ Get-Content build/normal-build.log -Tail 8; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 10; git diff --check -- src tools/game/hgss_game.py; rg -n -C 2 's_dl_cov\[DLC|s_stats.commands|s_dl_flushes' src/runtime/kj_g3.c | Select-Object -First 100; Get-Content src/compat/nitro/src/port_timer.c -TotalCount 65
    o2narc naix: 14 (0 updated)
    csv2bin naix: 3 (0 updated)
    alias naix: 2 (0 updated)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/normal/gen/overlay_ids.ld (unchanged)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/normal/gen/overlay_ids.h (unchanged)
    done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
    1104-{
    1105-#ifndef HGSS_PROFILE_OFF
    1106:    ++s_stats.commands;
    1107-#endif
    1108-#ifdef KJ_G3_ZERO_TRACE
    --
    1736-static uint8_t *s_dl_arena;
    1737-static uint32_t s_dl_arena_used;
    1738:static uint32_t s_dl_flushes;
    1739-
    1740-static void *dl_alloc(uint32_t n)
    --
    1759-    memset(s_dl, 0, sizeof(s_dl));
    1760-    s_dl_arena_used = 0;
    1761:    ++s_dl_flushes;
    1762-}
    1763-static dl_rec s_rec;
    --
    1767- * proven uncacheable, replayed, recorded but missed on the entry key. */
    1768-enum { DLC_SHORT, DLC_SMALL, DLC_BEGIN, DLC_BEGIN_FIRST, DLC_SPLIT, DLC_NEW, DLC_CHANGED, DLC_NEVER, DLC_HIT, DLC_KEY, DLC_COUNT };
    1769:static uint32_t s_dl_cov[DLC_COUNT];
    1770-static uint32_t s_dl_why[6]; /* batch-in-block, shared-verts, straddle/drops, lit+matrix, swap, open-prim */
    1771-
    --
    2253-    if (count < DL_MIN_WORDS || count > 0xFFFFFu || s_in_begin || s_fifo_need > 0 || s_rec.active) {
    2254-#ifndef HGSS_PROFILE_OFF
    2255:        s_dl_cov[DLC_SHORT] += (uint32_t)count;
    2256-#endif
    2257-        if (count < DL_MIN_WORDS) {
    2258-#ifndef HGSS_PROFILE_OFF
    2259:            s_dl_cov[DLC_SMALL] += (uint32_t)count;
    2260-#endif
    2261-        } else if (s_fifo_need > 0) {
    2262-#ifndef HGSS_PROFILE_OFF
    2263:            s_dl_cov[DLC_SPLIT] += (uint32_t)count;
    2264-#endif
    2265-        } else if (s_in_begin) {
    2266-#ifndef HGSS_PROFILE_OFF
    2267:            s_dl_cov[DLC_BEGIN] += (uint32_t)count;
    2268-#endif
    2269-            if ((words[0] & 0xFFu) == 0x40u) {
    2270-#ifndef HGSS_PROFILE_OFF
    2271:                s_dl_cov[DLC_BEGIN_FIRST] += (uint32_t)count;
    2272-#endif
    2273-            }
    --
    2283-    if (e->ptr != words || e->count != count) {
    2284-#ifndef HGSS_PROFILE_OFF
    2285:        s_dl_cov[DLC_NEW] += (uint32_t)count;
    2286-#endif
    2287-        dl_free(e);
    --
    2303-        e->last_frame = s_stats.swaps;
    2304-#ifndef HGSS_PROFILE_OFF
    2305:        s_dl_cov[DLC_NEVER] += (uint32_t)count;
    2306-#endif
    2307-#ifndef HGSS_PROFILE_OFF
    --
    2312-    } else if (memcmp(e->words, words, count * 4u) != 0) {
    2313-#ifndef HGSS_PROFILE_OFF
    2314:        s_dl_cov[DLC_CHANGED] += (uint32_t)count;
    2315-#endif
    2316-        memcpy(e->words, words, count * 4u);
    --
    2323-    if (e->never) {
    2324-#ifndef HGSS_PROFILE_OFF
    2325:        s_dl_cov[DLC_NEVER] += (uint32_t)count;
    2326-#endif
    2327-#ifndef HGSS_PROFILE_OFF
    --
    2350-#endif
    2351-#ifndef HGSS_PROFILE_OFF
    2352:            s_dl_cov[DLC_HIT] += (uint32_t)count;
    2353-#endif
    2354-            return;
    --
    2377-    if (verify) {
    2378-#ifndef HGSS_PROFILE_OFF
    2379:        s_dl_cov[DLC_HIT] += (uint32_t)count;
    2380-#endif
    2381-    } else if (e->recorded) {
    2382-#ifndef HGSS_PROFILE_OFF
    2383:        s_dl_cov[DLC_KEY] += (uint32_t)count;
    2384-#endif
    2385-    }
    --
    2522-                  (unsigned long)s_dl_why[0], (unsigned long)s_dl_why[1], (unsigned long)s_dl_why[2],
    2523-                  (unsigned long)s_dl_why[3], (unsigned long)s_dl_why[4], (unsigned long)s_dl_why[5],
    2524:                  (unsigned long)s_dl_flushes);
    2525-    NitroPort_Log("[dlcov] bypass: small %lu split %lu open-begin %lu (begin first %lu)",
    2526:                  (unsigned long)s_dl_cov[DLC_SMALL], (unsigned long)s_dl_cov[DLC_SPLIT],
    2527:                  (unsigned long)s_dl_cov[DLC_BEGIN], (unsigned long)s_dl_cov[DLC_BEGIN_FIRST]);
    2528-    NitroPort_Log("[dlcov] words: short %lu new %lu changed %lu never %lu hit %lu keymiss %lu",
    2529:                  (unsigned long)s_dl_cov[DLC_SHORT], (unsigned long)s_dl_cov[DLC_NEW],
    2530:                  (unsigned long)s_dl_cov[DLC_CHANGED], (unsigned long)s_dl_cov[DLC_NEVER],
    2531:                  (unsigned long)s_dl_cov[DLC_HIT], (unsigned long)s_dl_cov[DLC_KEY]);
    2532-    memset(s_dl_cov, 0, sizeof(s_dl_cov));
    2533-    memset(s_dl_why, 0, sizeof(s_dl_why));
    2534:    s_dl_flushes = 0;
    2535-    (void)s_dl_bytes;
    /*
     * The DS's four hardware timers, and the V-count, driven by a real clock.
     *
     * Each DS timer is a 16-bit counter that counts up at the bus clock
     * (33,513,982 Hz) divided by 1, 64, 256 or 1024, reloads a set value when
     * it overflows, and can request an interrupt then (or make the next timer
     * count one up: "cascade"). NitroSDK's tick (OS_GetTick: timer 0), alarms
     * (timer 1) and the game's play-time clock (timer 3) are all built on them.
     *
     * The port can't see reads of the counter registers, so it brings the
     * counters up to date ("syncs") at the moments that matter: when the SDK
     * writes a timer (the patched OS_SetTimerCount/OS_SetTimerControl call
     * NitroPort_TimerSetReload/NitroPort_TimerSetControl), when it reads the
     * tick (the patched OS_GetTick calls NitroPort_SyncTimers), and every frame.
     * Between syncs a counter reads its last value; with a sync at least every
     * frame that's within 1/60 s.
     *
     * Overflow interrupts are never lost: if a timer would overflow again while
     * the interrupt for its previous overflow is still waiting to be taken, it
     * stops just short and counts the rest at the next sync (the 64-bit tick is
     * the counter plus the number of those interrupts). That only happens if
     * the game keeps interrupts off for longer than an overflow period (125 ms
     * at the tick's speed), or the program was suspended.
     */
    
    #include <nitro.h>
    #include <nitro_port.h>
    
    #include "port_platform.h"
    
    /* libctru's tick counter, for the timer-sync cost. Not included as a header:
     * its types clash with the SDK's (port_render2d.c does the same). Host builds
     * have no counter, so the cost reads 0. */
    #ifdef __3DS__
    extern u64 svcGetSystemTick(void);
    #define PORT_NOW() svcGetSystemTick()
    #else
    #define PORT_NOW() 0
    #endif
    
    #define DS_BUS_HZ        33513982ULL
    #define CYCLES_PER_LINE  2130ULL       /* 355 dots of 6 cycles */
    #define LINES_PER_FRAME  263
    
    #define TMCNT_PRESCALER  0x0003
    #define TMCNT_CASCADE    0x0004
    #define TMCNT_IRQ        0x0040
    #define TMCNT_ENABLE     0x0080
    
    #define REG_COUNT(i)     (*(REGType16v *)((u32)REG_TM0CNT_L_ADDR + (u32)(i) * 4))
    #define REG_CONTROL(i)   (*(REGType16v *)((u32)REG_TM0CNT_H_ADDR + (u32)(i) * 4))
    
    typedef struct {
        u16 reload;
        u16 control;
        u32 counter;
        u64 last;      /* clock (in bus cycles) the counter is up to date with */
    } Timer;
    
    static const u32 sPrescale[4] = { 1, 64, 256, 1024 };
    
    static Timer sTimers[4];
    static u64 (*sClockSource)(void);
    static BOOL sClockStarted;
    static u64 sClockStart;
    ✓ • 2.67s

## Activity

    $ rg -n -C 5 'PORT_NOW|sSync' src/compat/nitro/src/port_timer.c; rg -n 's_[a-z_]+(\+\+| \+=)|\+\+s_' src/graphics/g2d/g2d.c src/runtime/kj_g3.c | Select-Object -First 140
    31-/* libctru's tick counter, for the timer-sync cost. Not included as a header:
    32- * its types clash with the SDK's (port_render2d.c does the same). Host builds
    33- * have no counter, so the cost reads 0. */
    34-#ifdef __3DS__
    35-extern u64 svcGetSystemTick(void);
    36:#define PORT_NOW() svcGetSystemTick()
    37-#else
    38:#define PORT_NOW() 0
    39-#endif
    40-
    41-#define DS_BUS_HZ        33513982ULL
    42-#define CYCLES_PER_LINE  2130ULL       /* 355 dots of 6 cycles */
    43-#define LINES_PER_FRAME  263
    --
    63-static u64 (*sClockSource)(void);
    64-static BOOL sClockStarted;
    65-static u64 sClockStart;
    66-static BOOL sVBlankSeen;
    67-static u64 sVBlankStart;
    68:static u64 sSyncCalls, sSyncTicks;
    69-static u64 sSoundPumpTicks;
    70-static u32 sSoundPumpCalls;
    71-
    72-/* ------------------------------------------------------------------ clock */
    73-
    --
    170-}
    171-
    172-void NitroPort_SyncTimers(void)
    173-{
    174-    u64 now = NitroPort_Cycles();
    175:    u64 t0 = PORT_NOW();
    176-    int i;
    177-
    178-    for (i = 0; i < 4; i++) {
    179-        Advance(i, now);
    180-    }
    181-    if (sVBlankSeen) {
    182-        u64 line = (now - sVBlankStart) / CYCLES_PER_LINE;
    183-
    184-        reg_GX_VCOUNT = (u16)((192 + line) % LINES_PER_FRAME);
    185-    }
    186:    sSyncTicks += PORT_NOW() - t0;
    187:    sSyncCalls++;
    188-}
    189-
    190-/* The syncs so far and their cost, in ARM11 ticks (NitroPort_SyncTimers).
    191- * OS_GetTick is the hot caller; the V-blank and timer writes sync too. */
    192-void NitroPort_SyncTimersStats(u64 *calls, u64 *ticks)
    193-{
    194:    *calls = sSyncCalls;
    195:    *ticks = sSyncTicks;
    196-}
    197-
    198-/* ------------------------------------------------------- sound-pump time */
    199-
    200-/* The main program reports how long a NitroPort_SoundPumpReplies call took
    src/graphics/g2d/g2d.c:304:                ++s_stats.dropped_quads;
    src/graphics/g2d/g2d.c:378:        ++s_stats.unsupported_modes;
    src/graphics/g2d/g2d.c:469:            ++s_stats.unsupported_modes; /* OBJ window / bitmap OBJ */
    src/graphics/g2d/g2d.c:525:            ++s_stats.dropped_quads;
    src/graphics/g2d/g2d.c:608:        ++s_stats.unsupported_modes; /* OBJ window: drawn as outside */
    src/graphics/g2d/g2d.c:678:            ++s_stats.draws;
    src/graphics/g2d/g2d.c:772:    ++s_stats.draws;
    src/graphics/g2d/g2d.c:801:    ++s_stats.draws;
    src/graphics/g2d/g2d.c:880:    s_char_bytes_compared += lc->char_len;
    src/graphics/g2d/g2d.c:959:        ++s_reused_layers;
    src/graphics/g2d/g2d.c:986:    ++s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1010:        ++s_reused_layers;
    src/graphics/g2d/g2d.c:1047:    ++s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1104:            ++s_stats.unsupported_modes;
    src/graphics/g2d/g2d.c:1125:            ++s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1148:            ++s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1264:            ++s_skipped_screens;
    src/graphics/g2d/g2d.c:1284:        ++s_stats.unsupported_modes; /* VRAM / main-memory display: tracked blocker */
    src/graphics/g2d/g2d.c:1300:        ++s_stats.spans;
    src/graphics/g2d/g2d.c:1313:            ++s_stats.spans;
    src/runtime/kj_g3.c:97:    ++s_clip_version;
    src/runtime/kj_g3.c:362:    ++s_normal_gen;
    src/runtime/kj_g3.c:383:        ++s_normal_gen;
    src/runtime/kj_g3.c:805:    const uint32_t k = s_prim_count++;
    src/runtime/kj_g3.c:926:    ++s_stats.box_tests;
    src/runtime/kj_g3.c:963:        ++s_stats.box_culled;
    src/runtime/kj_g3.c:1030:    f->serial = ++s_stats.swaps;
    src/runtime/kj_g3.c:1080:    ++s_op_calls[op & 255];
    src/runtime/kj_g3.c:1106:    ++s_stats.commands;
    src/runtime/kj_g3.c:1349:    ++s_direct_words;
    src/runtime/kj_g3.c:1359:    s_direct_params[s_direct_have++] = value;
    src/runtime/kj_g3.c:1400:        s_fifo_params[s_fifo_have++] = value;
    src/runtime/kj_g3.c:1443:    const uint32_t k = s_prim_count++;
    src/runtime/kj_g3.c:1753:    s_dl_arena_used += n;
    src/runtime/kj_g3.c:1761:    ++s_dl_flushes;
    src/runtime/kj_g3.c:1786:        ++s_dl_why[0];
    src/runtime/kj_g3.c:1793:            ++s_dl_why[1];
    src/runtime/kj_g3.c:1833:        ++s_dl_why[4];
    src/runtime/kj_g3.c:2078:                ++s_buf[s_build].frame.dropped_verts;
    src/runtime/kj_g3.c:2112:    if (s_coarse_sample) s_coarse_replay += svcGetSystemTick() - t;
    src/runtime/kj_g3.c:2161:            ++s_verify_logged;
    src/runtime/kj_g3.c:2172:        ++s_dl_hits;
    src/runtime/kj_g3.c:2222:    s_fifo_ticks += svcGetSystemTick() - t0;
    src/runtime/kj_g3.c:2233:    if (s_coarse_sample) s_coarse_decode += svcGetSystemTick() - t;
    src/runtime/kj_g3.c:2240:    ++s_fifo_calls;
    src/runtime/kj_g3.c:2243:    s_fifo_words += (u32)count;
    src/runtime/kj_g3.c:2308:        ++s_dl_uncacheable;
    src/runtime/kj_g3.c:2328:        ++s_dl_uncacheable;
    src/runtime/kj_g3.c:2349:            ++s_dl_hits;
    src/runtime/kj_g3.c:2387:    ++s_dl_misses;
    src/runtime/kj_g3.c:2412:        ++s_dl_why[2];
    src/runtime/kj_g3.c:2418:        ++s_dl_why[3];
    src/runtime/kj_g3.c:2475:            ++s_dl_why[5];
    src/runtime/kj_g3.c:2489:    const bool outer = s_coarse_depth++ == 0;
    src/runtime/kj_g3.c:2492:        s_coarse_sample = (++s_coarse_seq & 63u) == 0;
    src/runtime/kj_g3.c:2494:            ++s_coarse_n;
    src/runtime/kj_g3.c:2501:    if (outer && s_coarse_sample) s_coarse_total += svcGetSystemTick() - start;
    ✓ • 1.41s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\compat\nitro\src\port_timer.c
    @@ -174,3 +174,5 @@
         u64 now = NitroPort_Cycles();
    +#ifndef HGSS_PROFILE_OFF
         u64 t0 = PORT_NOW();
    +#endif
         int i;
    @@ -185,4 +187,6 @@
         }
    +#ifndef HGSS_PROFILE_OFF
         sSyncTicks += PORT_NOW() - t0;
         sSyncCalls++;
    +#endif
     }

## Activity

    $ @'
        from pathlib import Path
        for name, fragments in [('src/runtime/kj_g3.c',['    ++s_dl_flushes;']),('src/graphics/g2d/g2d.c',['    s_char_bytes_compared += lc->char_len;','        ++s_reused_layers;','            ++s_skipped_screens;'])]:
         p=Path(name); s=p.read_text()
         for f in fragments:s=s.replace(f,'#ifndef HGSS_PROFILE_OFF\n'+f+'\n#endif')
         p.write_text(s)
        '@ | Set-Content build/finish-profile-split.py
        python build/finish-profile-split.py
        rg -n 'error:|vendored library build failed|packaged' build/normal-build.log | Select-Object -Last 10
    ✗ (1) • 2.15s

## Assistant

I found and removed another hidden source of baseline overhead: timer synchronization was reading the system clock twice per call even with the old profiling switch off. The instrumented build now has sampled FIFO timing split between live decoding and cache replay, alongside the existing frame, GPU and audio reports; the sample’s remainder includes cache lookup and recording.

## Activity

    $ Get-Content build/normal-build.log -Tail 10; rg -n 'error:|vendored library build failed|packaged' build/normal-build.log | Select-Object -Last 8; rg -n 's_[a-zA-Z_]*(stats|ticks|calls)|[Pp]rofile' src/compat/nitro/src/port_timer.c | Select-Object -First 40
      CC  port_misc.c
      CC  port_overlay.c
      CC  port_os.c
      CC  port_thread.c
      CC  port_sound_chip.c
      CC  port_snd.c
      CC  port_rtc.c
      CC  port_pm.c
      CC  port_tp.c
      CC  port_timer.c
      CC  svc.c
      CC  pxi.c
    ✗ (1) • 2.69s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\docs\testing.md
    @@ -79,2 +79,18 @@
     
    +Build the baseline with `HGSS_PROFILE` unset and the diagnostic build with
    +`HGSS_PROFILE=on`, using a separate `--out` for each. The build-info sidecar
    +records `profiling` and any `--define` switches. The baseline omits the
    +frame, GPU, FIFO and mixer profiling probes; ordinary lifecycle and error
    +logging remains enabled. Log and crash files append across launches, with a
    +`--- new run ---` separator in the main log. Scripted frame dumps still use
    +their frame-number filenames and can overwrite earlier dumps.
    +
    +`[g3sample]` samples one in 64 FIFO calls. Its total, live decode and replay
    +values are elapsed ARM11 ticks for those samples, not totals for the whole
    +window. The remainder includes cache lookup, comparison, recording and probe
    +overhead; preemption is included. The fixed sampling stride can correlate
    +with repeated call patterns, so do not multiply by 64 and treat that as an
    +exact total. It does not separately time game-side model traversal or replay
    +state and copying.
    +
     - **Counters**: every 600 *presented* frames the log gets `[perf]` (stage p50/p95/p99/max),

## Activity

    $ Get-Content build/normal-build.log -Tail 8; rg -n 'error:|vendored library build failed|packaged' build/normal-build.log | Select-Object -Last 8
       45 |     SND_SetupChannelPcm(handle->chNo, format, data, loopFlag ? SND_CHANNEL_LOOP_REPEAT : SND_CHANNEL_LOOP_1SHOT, loopStart, loopLen, volume, 0, (u32)timer, pan);
          |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    C:/Users/terezi/workdir/kohjoh/build/hgss-nns/libraries/hgss_nns/snd_waveout.c:28:20: note: 'loopLen' was declared here
       28 |     int loopStart, loopLen;
          |                    ^~~~~~~
    built ../../../build/normal/sdk/libnitrosystem.a
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
    ✗ (1) • 1.44s

## Activity

    $ Get-Content build/normal-build.log -Tail 8; rg -n 'error:|vendored library build failed|packaged' build/normal-build.log | Select-Object -Last 8
       45 |     SND_SetupChannelPcm(handle->chNo, format, data, loopFlag ? SND_CHANNEL_LOOP_REPEAT : SND_CHANNEL_LOOP_1SHOT, loopStart, loopLen, volume, 0, (u32)timer, pan);
          |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    C:/Users/terezi/workdir/kohjoh/build/hgss-nns/libraries/hgss_nns/snd_waveout.c:28:20: note: 'loopLen' was declared here
       28 |     int loopStart, loopLen;
          |                    ^~~~~~~
    built ../../../build/normal/sdk/libnitrosystem.a
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
    ✗ (1) • 1.41s

## Activity

    $ rg -n 's[A-Za-z_]*(Calls|Ticks|Count|Frames|Profile)|svcGetSystemTick\(' src/compat/nitro/src src/runtime src/graphics src/app | Select-Object -First 180
    src/app\main.c:211:        const u64 t0 = svcGetSystemTick();
    src/app\main.c:216:        const u64 t1 = svcGetSystemTick();
    src/runtime\kj_frame.c:36:extern u64 svcGetSystemTick(void);
    src/runtime\kj_frame.c:100:        const u64 t0 = svcGetSystemTick();
    src/runtime\kj_frame.c:108:        s_hblank_ticks += svcGetSystemTick() - t0;
    src/runtime\kj_frame.c:139:        const u64 t0 = svcGetSystemTick();
    src/runtime\kj_frame.c:143:        s_pump_ticks += svcGetSystemTick() - t0;
    src/runtime\kj_g3.c:1071:extern u64 svcGetSystemTick(void);
    src/runtime\kj_g3.c:1077:    const u64 t0 = svcGetSystemTick();
    src/runtime\kj_g3.c:1079:    s_op_ticks[op & 255] += svcGetSystemTick() - t0;
    src/runtime\kj_g3.c:1334:extern u64 svcGetSystemTick(void);
    src/runtime\kj_g3.c:2110:    const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
    src/runtime\kj_g3.c:2114:    if (s_coarse_sample) s_coarse_replay += svcGetSystemTick() - t;
    src/runtime\kj_g3.c:2183:    const u64 t0 = svcGetSystemTick();
    src/runtime\kj_g3.c:2224:    s_fifo_ticks += svcGetSystemTick() - t0;
    src/runtime\kj_g3.c:2231:    const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
    src/runtime\kj_g3.c:2235:    if (s_coarse_sample) s_coarse_decode += svcGetSystemTick() - t;
    src/runtime\kj_g3.c:2497:            start = svcGetSystemTick();
    src/runtime\kj_g3.c:2503:    if (outer && s_coarse_sample) s_coarse_total += svcGetSystemTick() - start;
    src/compat/nitro/src\platform\3ds.c:97:    u64 ticks = svcGetSystemTick();
    src/compat/nitro/src\port_fs.c:35:static size_t sPathCount;
    src/compat/nitro/src\port_fs.c:49:    free_paths(sPaths, sPathCount);
    src/compat/nitro/src\port_fs.c:51:    sPathCount = 0;
    src/compat/nitro/src\port_fs.c:313:    sPathCount = count;
    src/compat/nitro/src\port_fs.c:364:        for (i = 0; i < sPathCount; ++i) {
    src/compat/nitro/src\port_os.c:27:extern u64 svcGetSystemTick(void);
    src/compat/nitro/src\port_os.c:28:#define PORT_NOW() svcGetSystemTick()
    src/compat/nitro/src\port_os.c:47:static u64 sHBlankIrqBitTicks[NITRO_PORT_IRQ_BITS];
    src/compat/nitro/src\port_os.c:48:static u32 sHBlankIrqBitCalls[NITRO_PORT_IRQ_BITS];
    src/compat/nitro/src\port_os.c:100:                sHBlankIrqBitTicks[bit] += PORT_NOW() - t;
    src/compat/nitro/src\port_os.c:101:                sHBlankIrqBitCalls[bit]++;
    src/compat/nitro/src\port_os.c:123:static u64 sHBlankKeepTicks, sHBlankDmaTicks, sHBlankIrqTicks;
    src/compat/nitro/src\port_os.c:127:static u64 sHBlankRefreshTicks, sHBlankSwitchTicks, sHBlankCmpTicks;
    src/compat/nitro/src\port_os.c:180:        sHBlankDmaTicks += PORT_NOW() - t;
    src/compat/nitro/src\port_os.c:186:        sHBlankIrqTicks += PORT_NOW() - t;
    src/compat/nitro/src\port_os.c:215:    sHBlankKeepTicks = sHBlankDmaTicks = sHBlankIrqTicks = 0;
    src/compat/nitro/src\port_os.c:217:    sHBlankRefreshTicks = sHBlankSwitchTicks = sHBlankCmpTicks = 0;
    src/compat/nitro/src\port_os.c:219:    memset(sHBlankIrqBitTicks, 0, sizeof(sHBlankIrqBitTicks));
    src/compat/nitro/src\port_os.c:220:    memset(sHBlankIrqBitCalls, 0, sizeof(sHBlankIrqBitCalls));
    src/compat/nitro/src\port_os.c:224:    { u64 tR = PORT_NOW(); NitroPort_GxRefresh(); sHBlankRefreshTicks += PORT_NOW() - tR; }
    src/compat/nitro/src\port_os.c:255:        sHBlankKeepTicks += t1 - t0;
    src/compat/nitro/src\port_os.c:268:        sHBlankSwitchTicks += PORT_NOW() - tS;
    src/compat/nitro/src\port_os.c:296:    *keep = sHBlankKeepTicks;
    src/compat/nitro/src\port_os.c:297:    *dma = sHBlankDmaTicks;
    src/compat/nitro/src\port_os.c:298:    *irq = sHBlankIrqTicks;
    src/compat/nitro/src\port_os.c:305:    if (refresh != NULL) *refresh = sHBlankRefreshTicks;
    src/compat/nitro/src\port_os.c:306:    if (sw != NULL) *sw = sHBlankSwitchTicks;
    src/compat/nitro/src\port_os.c:307:    if (cmp != NULL) *cmp = sHBlankCmpTicks;
    src/compat/nitro/src\port_os.c:326:        out[bit].ticks = sHBlankIrqBitTicks[bit];
    src/compat/nitro/src\port_os.c:327:        out[bit].calls = sHBlankIrqBitCalls[bit];
    src/compat/nitro/src\port_log.c:13:static int sReportedCount;
    src/compat/nitro/src\port_log.c:29:    for (i = 0; i < sReportedCount; i++) {
    src/compat/nitro/src\port_log.c:39:    if (!NitroPort_WasReported(what) && sReportedCount < MAX_REPORTED) {
    src/compat/nitro/src\port_log.c:40:        sReported[sReportedCount++] = what; /* callers pass string literals */
    src/compat/nitro/src\port_log.c:43:    return sReportedCount;
    src/compat/nitro/src\port_overlay.c:38:static u32 sCount;
    src/compat/nitro/src\port_overlay.c:53:    sCount = count;
    src/compat/nitro/src\port_overlay.c:83:    return sCount;
    src/compat/nitro/src\port_overlay.c:88:    return id < sCount && sLoaded[id];
    src/compat/nitro/src\port_overlay.c:95:    for (i = 0; i < sCount; i++) {
    src/compat/nitro/src\port_overlay.c:119:    if (target != MI_PROCESSOR_ARM9 || id >= sCount) {
    src/compat/nitro/src\port_overlay.c:140:    if (p_ovi->header.id >= sCount) {
    src/compat/nitro/src\port_overlay.c:154:    if (id >= sCount) {
    src/compat/nitro/src\port_overlay.c:172:    if (id < sCount) {
    src/compat/nitro/src\port_snd.c:30:extern u64 svcGetSystemTick(void);
    src/compat/nitro/src\port_snd.c:31:#define SNDPROF_NOW() svcGetSystemTick()
    src/compat/nitro/src\port_snd.c:65:static unsigned sHead, sCount;
    src/compat/nitro/src\port_snd.c:67:static unsigned sAlarmHead, sAlarmCount;
    src/compat/nitro/src\port_snd.c:75:static uint64_t sProfRenderCalls;   /* SoundRender calls that reached the mix loop */
    src/compat/nitro/src\port_snd.c:76:static uint64_t sProfRenderFrames;  /* output frames requested, summed over those calls */
    src/compat/nitro/src\port_snd.c:78:static uint64_t sProfSoundTicks;    /* sequencer sound_tick executions */
    src/compat/nitro/src\port_snd.c:79:static uint64_t sProfMixTicks;      /* ARM11 ticks inside NitroSound_Mix (__3DS__ only; 0 on host) */
    src/compat/nitro/src\port_snd.c:80:static uint64_t sProfTickTicks;     /* ARM11 ticks inside sound_tick (__3DS__ only; 0 on host) */
    src/compat/nitro/src\port_snd.c:127:    sHead = sCount = 0;
    src/compat/nitro/src\port_snd.c:128:    sAlarmHead = sAlarmCount = 0;
    src/compat/nitro/src\port_snd.c:153:    if (!sCount) return NULL;
    src/compat/nitro/src\port_snd.c:156:    --sCount;
    src/compat/nitro/src\port_snd.c:167:    if (sCount == SOUND_QUEUE_CAPACITY) {
    src/compat/nitro/src\port_snd.c:172:    sPending[(sHead + sCount) % SOUND_QUEUE_CAPACITY] =
    src/compat/nitro/src\port_snd.c:174:    ++sCount;
    src/compat/nitro/src\port_snd.c:223:        if (sAlarmCount < SOUND_ALARM_CAPACITY) {
    src/compat/nitro/src\port_snd.c:224:            sAlarmReplies[(sAlarmHead + sAlarmCount) % SOUND_ALARM_CAPACITY]
    src/compat/nitro/src\port_snd.c:226:            ++sAlarmCount;
    src/compat/nitro/src\port_snd.c:251:    sProfRenderCalls += 1u;
    src/compat/nitro/src\port_snd.c:252:    sProfRenderFrames += (uint64_t)frames;
    src/compat/nitro/src\port_snd.c:259:        if (sCount) {
    src/compat/nitro/src\port_snd.c:273:            sProfMixTicks += (uint64_t)(SNDPROF_NOW() - mixStart);
    src/compat/nitro/src\port_snd.c:285:                sProfTickTicks += (uint64_t)(SNDPROF_NOW() - tickStart);
    src/compat/nitro/src\port_snd.c:289:            sProfSoundTicks += 1u;
    src/compat/nitro/src\port_snd.c:302:    if (sProfRenderCalls % 600u == 0u) {
    src/compat/nitro/src\port_snd.c:308:                      (unsigned long long)sProfRenderCalls,
    src/compat/nitro/src\port_snd.c:309:                      (unsigned long long)sProfRenderFrames,
    src/compat/nitro/src\port_snd.c:311:                      (unsigned long long)sProfSoundTicks,
    src/compat/nitro/src\port_snd.c:312:                      (unsigned long long)sProfMixTicks,
    src/compat/nitro/src\port_snd.c:313:                      (unsigned long long)sProfTickTicks,
    src/compat/nitro/src\port_snd.c:334:        if (!sAlarmCount) {
    src/compat/nitro/src\port_snd.c:340:        --sAlarmCount;
    src/compat/nitro/src\port_sound_chip.c:867:static uint64_t sProfMixCalls;      /* NitroSound_Mix calls that mixed (past the guard) */
    src/compat/nitro/src\port_sound_chip.c:868:static uint64_t sProfMixFrames;     /* output frames, summed over those calls */
    src/compat/nitro/src\port_sound_chip.c:892:    sProfMixCalls += 1u;
    src/compat/nitro/src\port_sound_chip.c:893:    sProfMixFrames += (uint64_t)frames;
    src/compat/nitro/src\port_sound_chip.c:1077:    if (calls) *calls = sProfMixCalls;
    src/compat/nitro/src\port_sound_chip.c:1078:    if (frames) *frames = sProfMixFrames;
    src/compat/nitro/src\port_timer.c:35:extern u64 svcGetSystemTick(void);
    src/compat/nitro/src\port_timer.c:36:#define PORT_NOW() svcGetSystemTick()
    src/compat/nitro/src\port_timer.c:68:static u64 sSyncCalls, sSyncTicks;
    src/compat/nitro/src\port_timer.c:69:static u64 sSoundPumpTicks;
    src/compat/nitro/src\port_timer.c:70:static u32 sSoundPumpCalls;
    src/compat/nitro/src\port_timer.c:189:    sSyncTicks += PORT_NOW() - t0;
    src/compat/nitro/src\port_timer.c:190:    sSyncCalls++;
    src/compat/nitro/src\port_timer.c:198:    *calls = sSyncCalls;
    src/compat/nitro/src\port_timer.c:199:    *ticks = sSyncTicks;
    src/compat/nitro/src\port_timer.c:209:    sSoundPumpTicks += ticks;
    src/compat/nitro/src\port_timer.c:210:    sSoundPumpCalls++;
    src/compat/nitro/src\port_timer.c:215:    *ticks = sSoundPumpTicks;
    src/compat/nitro/src\port_timer.c:216:    *calls = sSoundPumpCalls;
    src/compat/nitro/src\pxi.c:38:static u32 sRecvCount;
    src/compat/nitro/src\pxi.c:83:        sRecvCount = 0;
    src/compat/nitro/src\pxi.c:141:        if (sRecvCount == 0) {
    src/compat/nitro/src\pxi.c:147:        sRecvCount--;
    src/compat/nitro/src\pxi.c:180:    if (sRecvCount == RECV_FIFO_SIZE) {
    src/compat/nitro/src\pxi.c:185:    sRecvFifo[(sRecvHead + sRecvCount) % RECV_FIFO_SIZE] = msg;
    src/compat/nitro/src\pxi.c:186:    sRecvCount++;
    ✓ • 1.29s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\app\main.c
    @@ -205,2 +205,3 @@
         settings.birth_day = 1;
    +#ifndef HGSS_PROFILE_OFF
         {
    @@ -220,2 +221,3 @@
         }
    +#endif
         s_base = C3D_FrameCounter(0);

## Activity

    $ Get-Content build/normal-build.log -Tail 12; Get-Process python,make,arm-none-eabi-gcc -ErrorAction SilentlyContinue | Select-Object ProcessName,CPU,Id | Select-Object -First 10
          |                    ^~~~~~~
    built ../../../build/normal/sdk/libnitrosystem.a
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
      129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
    == duplicate symbols
       0 symbols defined in more than one object (0 objects weakened)
    == Wi-Fi stand-ins
      257 Wi-Fi SDK functions stood in for (online play is not ported)
    == compiling the DS-side runtime (11 files)
    == compiling the 3DS-side runtime (17 files)
    == link -> /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
    
    ProcessName              CPU    Id
    -----------              ---    --
    arm-none-eabi-gcc    0.03125 48704
    python            166.515625 47128
    python              0.015625 48012
    
    
    ✗ (1) • 1.56s

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/normal -j 32 > /c/Users/terezi/workdir/kohjoh/build/normal-build.log 2>&1'
    ✓ • 2m 28s

## Activity

    $ Get-Content build/normal-build.log -Tail 18; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 20
    C:/Users/terezi/workdir/kohjoh/build/hgss-nns/libraries/hgss_nns/snd_waveout.c:28:20: note: 'loopLen' was declared here
       28 |     int loopStart, loopLen;
          |                    ^~~~~~~
    built ../../../build/normal/sdk/libnitrosystem.a
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
      129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
    == duplicate symbols
       0 symbols defined in more than one object (0 objects weakened)
    == Wi-Fi stand-ins
      257 Wi-Fi SDK functions stood in for (online play is not ported)
    == compiling the DS-side runtime (11 files)
    == compiling the 3DS-side runtime (17 files)
    == link -> /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
       linked /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
       [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
       packaged /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold.3dsx (5529432 bytes)
       build info: pokeheartgold.build-info.txt
    ✗ (1) • 1.35s

## Assistant

The normal variant has compiled and linked once without vendored-library errors. I’m rebuilding it after the final probe removals, then I’ll build the instrumented variant in its own output tree and inspect both executables for the expected diagnostic code and build metadata.

## Activity

    $ rg -n 'mix|differential' tests src/compat/nitro/tests | Select-Object -First 35; Get-Content tools/game/hgss_game.py | Select-Object -Skip 1578 -First 48; rg -n -C 2 's_coarse' src/runtime/kj_g3.c; Get-Content build/normal-build.log -Tail 4
    rg: src/compat/nitro/tests: The system cannot find the file specified. (os error 2)
            # rebuilds the 2D engine (gx3d.o rides the gx3d-mode marker).
            (sdk_out / "port" / "port_render2d.o").unlink(missing_ok=True)
        sndadpcm_mode = "1" if os.environ.get("HGSS_SND_ADPCM") == "1" else "0"
        sndadpcm_marker = sdk_out / ".sndadpcm-mode"
        if not sndadpcm_marker.exists() or sndadpcm_marker.read_text().strip() != sndadpcm_mode:
            # Branchless ADPCM decode lives in the sound chip object only.
            (sdk_out / "port" / "port_sound_chip.o").unlink(missing_ok=True)
        a0_mode = "1" if os.environ.get("HGSS_A0_DIAG") == "1" else "0"
        a0_marker = sdk_out / ".a0-mode"
        if not a0_marker.exists() or a0_marker.read_text().strip() != a0_mode:
            (sdk_out / "port" / "port_gx3d.o").unlink(missing_ok=True)
            (sdk_out / "port" / "port_render2d.o").unlink(missing_ok=True)
        prof_mode = profile_mode()
        prof_marker = sdk_out / ".profile-mode"
        if not prof_marker.exists() or prof_marker.read_text().strip() != prof_mode:
            # HGSS_PROFILE toggles the 2D tick reads (port_render2d.c) and the
            # geometry diagnostics (port_gx3d.c); both objects must rebuild so a
            # PROFILE and an OFF artifact never share one archive.
            (sdk_out / "port" / "port_gx3d.o").unlink(missing_ok=True)
            (sdk_out / "port" / "port_render2d.o").unlink(missing_ok=True)
        nitro_cmd = [sys.executable, str(TOOLS / "hgss_nitro.py"), "make",
                     "--target", target, "--out", str(sdk_out), "--subproject", "nitro",
                     "lib", "-k", "-j", str(jobs)]
        nns_cmd = [sys.executable, str(TOOLS / "hgss_nitro.py"), "make",
                   "--target", target, "--out", str(sdk_out), "--subproject", "nitrosystem",
                   "lib", "-k", "-j", str(jobs)]
        ok = True
        for index, cmd in enumerate((nitro_cmd, nns_cmd)):
            print("+ " + " ".join(cmd), flush=True)
            proc = subprocess.run(cmd)
            if index == 0 and proc.returncode == 0:
                palette_marker.parent.mkdir(parents=True, exist_ok=True)
                palette_marker.write_text(palette_mode + "\n")
                gx3d_marker.write_text(gx3d_mode + "\n")
                bg3slot_marker.write_text(bg3slot_mode + "\n")
                sndadpcm_marker.write_text(sndadpcm_mode + "\n")
                a0_marker.write_text(a0_mode + "\n")
                prof_marker.write_text(prof_mode + "\n")
            ok = ok and proc.returncode == 0
        libs = [p for p in (sdk_out / "libnitrosystem.a", sdk_out / "libnitro.a")
                if p.is_file()]
        if not ok:
            print("   warning: the vendored library build failed; linking with "
                  f"{', '.join(p.name for p in libs) or 'no archives'} and reporting "
                  "what is missing", flush=True)
        return ok and len(libs) == 2, libs
    
    
    2062- * calls; the remainder includes lookup, comparison, recording and probe
    2063- * overhead. Wall time includes preemption. */
    2064:static u32 s_coarse_seq, s_coarse_n, s_coarse_depth;
    2065:static bool s_coarse_sample;
    2066:static u64 s_coarse_total, s_coarse_decode, s_coarse_replay;
    2067-#endif
    2068-
    --
    2108-{
    2109-#ifndef HGSS_PROFILE_OFF
    2110:    const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
    2111-#endif
    2112-    dl_replay_impl(e);
    2113-#ifndef HGSS_PROFILE_OFF
    2114:    if (s_coarse_sample) s_coarse_replay += svcGetSystemTick() - t;
    2115-#endif
    2116-}
    --
    2229-{
    2230-#ifndef HGSS_PROFILE_OFF
    2231:    const u64 t = s_coarse_sample ? svcGetSystemTick() : 0;
    2232-#endif
    2233-    decode_words_impl(words, count);
    2234-#ifndef HGSS_PROFILE_OFF
    2235:    if (s_coarse_sample) s_coarse_decode += svcGetSystemTick() - t;
    2236-#endif
    2237-}
    --
    2489-{
    2490-#ifndef HGSS_PROFILE_OFF
    2491:    const bool outer = s_coarse_depth++ == 0;
    2492-    u64 start = 0;
    2493-    if (outer) {
    2494:        s_coarse_sample = (++s_coarse_seq & 63u) == 0;
    2495:        if (s_coarse_sample) {
    2496:            ++s_coarse_n;
    2497-            start = svcGetSystemTick();
    2498-        }
    --
    2501-    fifo_words_impl(words, count);
    2502-#ifndef HGSS_PROFILE_OFF
    2503:    if (outer && s_coarse_sample) s_coarse_total += svcGetSystemTick() - start;
    2504:    --s_coarse_depth;
    2505:    if (outer) s_coarse_sample = false;
    2506-#endif
    2507-}
    --
    2511-#ifndef HGSS_PROFILE_OFF
    2512-    NitroPort_Log("[g3sample] every 64th FIFO call: samples %lu total_ticks %llu decode_ticks %llu replay_ticks %llu; remainder includes lookup/recording and probe overhead",
    2513:                  (unsigned long)s_coarse_n, (unsigned long long)s_coarse_total,
    2514:                  (unsigned long long)s_coarse_decode, (unsigned long long)s_coarse_replay);
    2515:    s_coarse_n = 0;
    2516:    s_coarse_total = s_coarse_decode = s_coarse_replay = 0;
    2517-#endif
    2518-    *hits = s_dl_hits;
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/normal/gen/overlay_ids.h (unchanged)
    done: 890 files, 0 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
    ✓ • 2.19s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\tools\game\hgss_game.py
    @@ -1593,7 +1593,7 @@
         if not prof_marker.exists() or prof_marker.read_text().strip() != prof_mode:
    -        # HGSS_PROFILE toggles the 2D tick reads (port_render2d.c) and the
    -        # geometry diagnostics (port_gx3d.c); both objects must rebuild so a
    -        # PROFILE and an OFF artifact never share one archive.
    -        (sdk_out / "port" / "port_gx3d.o").unlink(missing_ok=True)
    -        (sdk_out / "port" / "port_render2d.o").unlink(missing_ok=True)
    +        # Make does not track changed command-line defines. Rebuild every
    +        # port object that uses this switch when an output tree is reused.
    +        for name in ("port_gx3d", "port_render2d", "port_os", "port_timer",
    +                     "port_snd", "port_sound_chip"):
    +            (sdk_out / "port" / f"{name}.o").unlink(missing_ok=True)
         nitro_cmd = [sys.executable, str(TOOLS / "hgss_nitro.py"), "make",

## Activity

    $ Get-Content build/normal-build.log -Tail 7; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 8
      CC  SND_bank.c
      CC  SND_main.c
      CC  SND_work.c
      CC  SND_util.c
      CC  snd_flush_command.c
      CC  snd_destroy_bank.c
      CC  snd_interface.c
      CC  snd_master_volume.c
    ✗ (1) • 2.23s

## Activity

    $ rg -n 'log|Remove-Item|Copy-Item' tools/azahar/run.ps1 | Select-Object -Last 45; rg -n 's_.*(profile|stats|counter)' src/graphics src/runtime | Measure-Object; rg -n 's_frames' src/graphics/renderer.c
    2:# screenshots at fixed times and collect the program's log.
    40:foreach ($d in @($root, "$root\data", "$root\saves", "$root\logs")) { New-Item -ItemType Directory -Force -Path $d | Out-Null }
    41:if ($Pack -ne "") { Copy-Item $Pack (Join-Path $root "data\$Edition.pak") -Force }
    42:if ($Manifest -ne "") { Copy-Item $Manifest (Join-Path $root "data\$Edition.manifest.json") -Force }
    43:if ($Save -ne "") { Copy-Item $Save (Join-Path $root "saves\$Edition.sav") -Force }
    44:if ($FreshSave) { Remove-Item (Join-Path $root "saves\$Edition.sav") -ErrorAction SilentlyContinue }
    46:if ($Script -ne "") { Copy-Item $Script $scriptDst -Force } else { Remove-Item $scriptDst -ErrorAction SilentlyContinue }
    47:$log = Join-Path $root "logs\$Edition.log"
    48:Remove-Item $log -ErrorAction SilentlyContinue
    53:Copy-Item $Artifact $dsx -Force
    91:      if ((Test-Path $log) -and (Select-String -Path $log -SimpleMatch $pat -Quiet)) { $found = $true; break }
    108:if (Test-Path $log) { Copy-Item $log (Join-Path $out "$Edition.log") -Force; Write-Output "log copied" } else { Write-Output "no program log" }
    109:$azlog = Join-Path $env:APPDATA "Azahar\log\azahar_log.txt"
    110:if (Test-Path $azlog) { Copy-Item $azlog (Join-Path $out "azahar_log.txt") -Force }
    
    
    Count    : 80
    Average  :
    Sum      :
    Maximum  :
    Minimum  :
    Property :
    
    41:static uint32_t s_frames;
    157:    ++s_frames;
    160:    if (s_frames % 600u == 0u) {
    
    
    ✓ • 1.94s

## Activity

    $ Get-Content tools/azahar/run.ps1 | Select-Object -First 115
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
    Remove-Item $log -ErrorAction SilentlyContinue
    
    $out = Join-Path $OutDir $Tag
    New-Item -ItemType Directory -Force -Path $out | Out-Null
    $dsx = Join-Path $sd "kohjoh.3dsx"
    Copy-Item $Artifact $dsx -Force
    
    Get-Process azahar -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 800
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
      Write-Output "shot $file"
    }
    
    # Log-synchronised shots: the script freezes game frame N; shoot while held.
    if ($Holds -ne "") {
      foreach ($hf in $Holds.Split(',')) {
        $pat = "holding game frame $hf "
        $deadline = (Get-Date).AddSeconds($Timeout)
        $found = $false
        while ((Get-Date) -lt $deadline) {
          if ((Test-Path $log) -and (Select-String -Path $log -SimpleMatch $pat -Quiet)) { $found = $true; break }
          Start-Sleep -Milliseconds 250
        }
        if (-not $found) { Write-Output "frame $hf never held"; break }
        Start-Sleep -Milliseconds 900
        Shot ("f{0:00000}" -f [int]$hf)
      }
    }
    if ($Shots -ne "") { foreach ($s in $Shots.Split(',')) {
      $t = [double]$s
      $wait = $t - ((Get-Date) - $start).TotalSeconds
      if ($wait -gt 0) { Start-Sleep -Milliseconds ([int]($wait * 1000)) }
      Shot ("t{0:000}" -f [int]$t)
    } }
    Start-Sleep -Seconds $Extra
    Get-Process azahar -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 500
    if (Test-Path $log) { Copy-Item $log (Join-Path $out "$Edition.log") -Force; Write-Output "log copied" } else { Write-Output "no program log" }
    $azlog = Join-Path $env:APPDATA "Azahar\log\azahar_log.txt"
    if (Test-Path $azlog) { Copy-Item $azlog (Join-Path $out "azahar_log.txt") -Force }
    ✓ • 1.35s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\tools\azahar\run.ps1
    @@ -47,3 +47,2 @@
     $log = Join-Path $root "logs\$Edition.log"
    -Remove-Item $log -ErrorAction SilentlyContinue
     
    @@ -56,2 +55,13 @@
     Start-Sleep -Milliseconds 800
    +$logOffset = if (Test-Path $log) { (Get-Item $log).Length } else { 0 }
    +function Read-NewLog {
    +  if (-not (Test-Path $log)) { return "" }
    +  $stream = [System.IO.File]::Open($log, [System.IO.FileMode]::Open,
    +    [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    +  try {
    +    [void]$stream.Seek([Math]::Min($logOffset, $stream.Length), [System.IO.SeekOrigin]::Begin)
    +    $reader = [System.IO.StreamReader]::new($stream)
    +    try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
    +  } finally { $stream.Dispose() }
    +}
     $p = Start-Process -FilePath "C:\Program Files\Azahar\azahar.exe" -ArgumentList "`"$dsx`"" -PassThru
    @@ -90,3 +100,3 @@
         while ((Get-Date) -lt $deadline) {
    -      if ((Test-Path $log) -and (Select-String -Path $log -SimpleMatch $pat -Quiet)) { $found = $true; break }
    +      if ((Read-NewLog).Contains($pat)) { $found = $true; break }
           Start-Sleep -Milliseconds 250
    @@ -107,3 +117,3 @@
     Start-Sleep -Milliseconds 500
    -if (Test-Path $log) { Copy-Item $log (Join-Path $out "$Edition.log") -Force; Write-Output "log copied" } else { Write-Output "no program log" }
    +if (Test-Path $log) { [System.IO.File]::WriteAllText((Join-Path (Resolve-Path $out) "$Edition.log"), (Read-NewLog)); Write-Output "log copied" } else { Write-Output "no program log" }
     $azlog = Join-Path $env:APPDATA "Azahar\log\azahar_log.txt"

## Activity

    $ Get-Content build/normal-build.log -Tail 6; rg -n 'error:|vendored library build failed' build/normal-build.log | Select-Object -First 5
       0 symbols defined in more than one object (0 objects weakened)
    == Wi-Fi stand-ins
      257 Wi-Fi SDK functions stood in for (online play is not ported)
    == compiling the DS-side runtime (11 files)
    == compiling the 3DS-side runtime (17 files)
    == link -> /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
    ✗ (1) • 1.49s

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/normal -j 32 > /c/Users/terezi/workdir/kohjoh/build/normal-build.log 2>&1'
    ✓ • 1m 41s

## Assistant

The normal rebuild completed cleanly, and the instrumented build is now compiling in a separate tree. I also fixed the Azahar harness so it preserves the accumulated log and reads only the current run when waiting for scripted frames.

## Activity

    $ Get-Content build/normal-build.log -Tail 8; Get-Content build/normal/pokeheartgold.build-info.txt -TotalCount 23; C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe -S build/normal/pokeheartgold-3ds.elf | Select-String 'kj_perf|sProf|s_coarse|sSyncTicks|sHBlankIrqBit'; C:/devkitPro/devkitARM/bin/arm-none-eabi-strings.exe build/normal/pokeheartgold-3ds.elf | Select-String '\[perf\]|\[g3sample\]|\[sndprof\]|\[gputime\]'
      257 Wi-Fi SDK functions stood in for (online play is not ported)
    == compiling the DS-side runtime (11 files)
    == compiling the 3DS-side runtime (17 files)
    == link -> /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
       linked /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold-3ds.elf
       [sound] _0210D750 -> nns_volume_table.c (512 bytes, [128] = 0)
       packaged /c/Users/terezi/workdir/kohjoh/build/normal/pokeheartgold.3dsx (5529432 bytes)
       build info: pokeheartgold.build-info.txt
    game: heartgold
    target: 3ds
    profiling: off
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
    build: 8a92e93-dirty
    sha: 8a92e93e255e07e9c09a036e8a64f7ed71ae5533
    dirty: yes
    built_utc: 2026-11-13T18:35:49Z
    file: pokeheartgold.3dsx
    size: 5529432
    
    002f7fb8 0000002c T Comm_GetOwnGsProfileId
    001070b4 00000018 T DWC_CheckHasProfile
    0010732c 00000018 T DWC_GetGsProfileId
    005c8620 R gPalPadDetailsProfileWindowTemplates
    003e8078 00000010 T PPWLobbyPlayer_HasProfiles
    01192078 00000020 b sHBlankIrqBitCalls
    01192038 00000040 b sHBlankIrqBitTicks
    00622600 0000000c r sProfileButtonHitboxes
    00622634 00000004 r sProfileButtonIdleAnims
    00622640 00000004 r sProfileButtonPressAnims
    
    
    ✓ • 7.57s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\compat\nitro\src\port_os.c
    @@ -218,4 +218,6 @@
     #endif
    +#ifndef HGSS_PROFILE_OFF
         memset(sHBlankIrqBitTicks, 0, sizeof(sHBlankIrqBitTicks));
         memset(sHBlankIrqBitCalls, 0, sizeof(sHBlankIrqBitCalls));
    +#endif
         /* (GXSTAT and the GXFIFO level interrupt once, not once a scanline: see
    @@ -234,3 +236,5 @@
          * the H-blank of the line before did. */
    +#ifndef HGSS_PROFILE_OFF
         sHBlankWalking = 1;
    +#endif
         HBlank(262, irq, FALSE);
    @@ -257,3 +261,5 @@
         }
    +#ifndef HGSS_PROFILE_OFF
         sHBlankWalking = 0;
    +#endif
         reg_OS_IF &= ~OS_IE_H_BLANK;   /* (not left pending for later) */

## Activity

    $ rg -n '\+\+s_|s_[a-z_]+ \+=' src/graphics src/runtime/kj_g3.c | Select-Object -First 180; rg -n -C 2 's_stats|[Pp]rofile' src/graphics/pica src/graphics/g3d | Select-Object -First 25; Get-Content build/instrumented-build.log -Tail 6
    src/runtime/kj_g3.c:97:    ++s_clip_version;
    src/runtime/kj_g3.c:362:    ++s_normal_gen;
    src/runtime/kj_g3.c:383:        ++s_normal_gen;
    src/runtime/kj_g3.c:926:    ++s_stats.box_tests;
    src/runtime/kj_g3.c:963:        ++s_stats.box_culled;
    src/runtime/kj_g3.c:1030:    f->serial = ++s_stats.swaps;
    src/runtime/kj_g3.c:1080:    ++s_op_calls[op & 255];
    src/runtime/kj_g3.c:1106:    ++s_stats.commands;
    src/runtime/kj_g3.c:1349:    ++s_direct_words;
    src/runtime/kj_g3.c:1753:    s_dl_arena_used += n;
    src/runtime/kj_g3.c:1762:    ++s_dl_flushes;
    src/runtime/kj_g3.c:1788:        ++s_dl_why[0];
    src/runtime/kj_g3.c:1795:            ++s_dl_why[1];
    src/runtime/kj_g3.c:1835:        ++s_dl_why[4];
    src/runtime/kj_g3.c:2080:                ++s_buf[s_build].frame.dropped_verts;
    src/runtime/kj_g3.c:2114:    if (s_coarse_sample) s_coarse_replay += svcGetSystemTick() - t;
    src/runtime/kj_g3.c:2163:            ++s_verify_logged;
    src/runtime/kj_g3.c:2174:        ++s_dl_hits;
    src/runtime/kj_g3.c:2224:    s_fifo_ticks += svcGetSystemTick() - t0;
    src/runtime/kj_g3.c:2235:    if (s_coarse_sample) s_coarse_decode += svcGetSystemTick() - t;
    src/runtime/kj_g3.c:2242:    ++s_fifo_calls;
    src/runtime/kj_g3.c:2245:    s_fifo_words += (u32)count;
    src/runtime/kj_g3.c:2310:        ++s_dl_uncacheable;
    src/runtime/kj_g3.c:2330:        ++s_dl_uncacheable;
    src/runtime/kj_g3.c:2351:            ++s_dl_hits;
    src/runtime/kj_g3.c:2389:    ++s_dl_misses;
    src/runtime/kj_g3.c:2414:        ++s_dl_why[2];
    src/runtime/kj_g3.c:2420:        ++s_dl_why[3];
    src/runtime/kj_g3.c:2477:            ++s_dl_why[5];
    src/runtime/kj_g3.c:2494:        s_coarse_sample = (++s_coarse_seq & 63u) == 0;
    src/runtime/kj_g3.c:2496:            ++s_coarse_n;
    src/runtime/kj_g3.c:2503:    if (outer && s_coarse_sample) s_coarse_total += svcGetSystemTick() - start;
    src/graphics\pica\gpu.c:135:        s_arena_dropped += quads;
    src/graphics\pica\gpu.c:140:    s_arena_used += quads;
    src/graphics\renderer.c:118:        ++s_skipped_presents;
    src/graphics\renderer.c:147:        s_draw_ms_sum += draw_ms;
    src/graphics\renderer.c:157:    ++s_frames;
    src/graphics\resources\atlas.c:89:    ++s_frame;
    src/graphics\resources\atlas.c:169:            ++s_stats.evictions;
    src/graphics\resources\atlas.c:203:    ++s_stats.conversions;
    src/graphics\resources\atlas.c:218:    ++s_stats.lookups;
    src/graphics\resources\atlas.c:252:        ++s_stats.full;
    src/graphics\g3d\g3d.c:156:            ++s_stats.unsupported; /* toon/highlight drawn as modulate */
    src/graphics\g3d\g3d.c:191:                ++s_stats.unsupported; /* shadow mask volume: not drawn */
    src/graphics\g3d\g3d.c:262:        ++s_stats.draws;
    src/graphics\g3d\g3d.c:295:        ++s_stats.unsupported; /* fog */
    src/graphics\g3d\g3d.c:300:        ++s_stats.unsupported; /* edge marking (baseline excluded) */
    src/graphics\g2d\g2d.c:304:                ++s_stats.dropped_quads;
    src/graphics\g2d\g2d.c:378:        ++s_stats.unsupported_modes;
    src/graphics\g2d\g2d.c:469:            ++s_stats.unsupported_modes; /* OBJ window / bitmap OBJ */
    src/graphics\g2d\g2d.c:525:            ++s_stats.dropped_quads;
    src/graphics\g2d\g2d.c:608:        ++s_stats.unsupported_modes; /* OBJ window: drawn as outside */
    src/graphics\g2d\g2d.c:678:            ++s_stats.draws;
    src/graphics\g2d\g2d.c:772:    ++s_stats.draws;
    src/graphics\g2d\g2d.c:801:    ++s_stats.draws;
    src/graphics\g2d\g2d.c:881:    s_char_bytes_compared += lc->char_len;
    src/graphics\g2d\g2d.c:962:        ++s_reused_layers;
    src/graphics\g2d\g2d.c:990:    ++s_rebuilt_layers;
    src/graphics\g2d\g2d.c:1015:        ++s_reused_layers;
    src/graphics\g2d\g2d.c:1053:    ++s_rebuilt_layers;
    src/graphics\g2d\g2d.c:1110:            ++s_stats.unsupported_modes;
    src/graphics\g2d\g2d.c:1131:            ++s_rebuilt_layers;
    src/graphics\g2d\g2d.c:1154:            ++s_rebuilt_layers;
    src/graphics\g2d\g2d.c:1271:            ++s_skipped_screens;
    src/graphics\g2d\g2d.c:1292:        ++s_stats.unsupported_modes; /* VRAM / main-memory display: tracked blocker */
    src/graphics\g2d\g2d.c:1308:        ++s_stats.spans;
    src/graphics\g2d\g2d.c:1321:            ++s_stats.spans;
    src/graphics\g3d\textures.c:101:    ++s_frame;
    src/graphics\g3d\textures.c:265:            ++s_stats.missing_bank;
    src/graphics\g3d\textures.c:330:                    ++s_stats.missing_bank;
    src/graphics\g3d\textures.c:448:        ++s_stats.uploads;
    src/graphics\g3d\textures.c:457:        ++s_stats.full;
    src/graphics\g3d\textures.c:469:            ++s_stats.full;
    src/graphics\g3d\textures.c:477:        ++s_stats.full;
    src/graphics\g3d\textures.c:495:        ++s_stats.missing_bank;
    src/graphics\g3d\textures.c:507:    s_cache_bytes += bytes;
    src/graphics\g3d\textures.c:511:    ++s_stats.uploads;
    src/graphics/g3d\textures.c-44-static const uint8_t *s_pal_slot[6];
    src/graphics/g3d\textures.c-45-static uint8_t s_morton[64];
    src/graphics/g3d\textures.c:46:static kj_tex_stats s_stats;
    src/graphics/g3d\textures.c-47-static int s_last_hit = -1;
    src/graphics/g3d\textures.c-48-/* Entry hints by key hash: a draw used to scan all 384 entries whenever it
    --
    src/graphics/g3d\textures.c-263-        if (!gather_pal(palbuf, d->pal_addr, d->pal_len)) {
    src/graphics/g3d\textures.c-264-#ifndef HGSS_PROFILE_OFF
    src/graphics/g3d\textures.c:265:            ++s_stats.missing_bank;
    src/graphics/g3d\textures.c-266-#endif
    src/graphics/g3d\textures.c-267-            return false;
    --
    src/graphics/g3d\textures.c-328-                if (!gather_pal(pb, d->pal_addr + (ix & 0x3FFFu) * 4u, 8)) {
    src/graphics/g3d\textures.c-329-#ifndef HGSS_PROFILE_OFF
    src/graphics/g3d\textures.c:330:                    ++s_stats.missing_bank;
    src/graphics/g3d\textures.c-331-#endif
    src/graphics/g3d\textures.c-332-                    return false;
    --
    src/graphics/g3d\textures.c-388-        return true; /* bank unmapped this frame: keep what we have */
    src/graphics/g3d\textures.c-389-    }
    src/graphics/g3d\textures.c:390:    s_stats.compared_bytes += total;
    src/graphics/g3d\textures.c-391-    kj_perf_count(KJ_COUNT_TEX_BYTES, total);
    src/graphics/g3d\textures.c-392-    if (data || index) {
    --
    src/graphics/g3d\textures.c-446-        decode(&d, e->shadow, e->shadow + d.data_len, &e->tex);
    alias naix: 2 (0 updated)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/instrumented/gen/overlay_ids.ld (updated)
    overlay ids: /c/Users/terezi/workdir/kohjoh/build/instrumented/gen/overlay_ids.h (updated)
    done: 890 files, 2 updated under /c/Users/terezi/workdir/kohjoh/build/upstream
    == heartgold 3ds: 1036 C, 56 C++, 150 .s
    == compiling C/C++ (1092 files, -j32)
    ✓ • 3.21s

## Activity

    $ rg -n 's_arena_dropped|s_stats\.[a-z_]+\s*\+=' src/graphics; rg -n 's_skipped_presents|s_frames' src/graphics/renderer.c
    src/graphics\pica\gpu.c:37:static uint32_t s_arena_used, s_arena_dropped;
    src/graphics\pica\gpu.c:135:        s_arena_dropped += quads;
    src/graphics\pica\gpu.c:158:    const uint32_t d = s_arena_dropped;
    src/graphics\pica\gpu.c:159:    s_arena_dropped = 0;
    src/graphics\g3d\textures.c:390:    s_stats.compared_bytes += total;
    src/graphics\g2d\g2d.c:693:        s_stats.draws += 2;
    src/graphics\g2d\g2d.c:1134:        s_stats.bg_quads += pr->bg[bg].count;
    src/graphics\g2d\g2d.c:1158:            s_stats.obj_quads += pr->obj[i].count;
    src/graphics\g3d\g3d.c:265:        s_stats.triangles += b->index_count / 3u;
    src/graphics\g3d\g3d.c:279:    s_stats.batches += f->batch_count;
    src/graphics\g3d\g3d.c:282:    s_stats.unsupported += f->unsupported + f->dropped_batches + f->dropped_verts;
    src/graphics\g3d\g3d.c:285:    s_stats.box_tests += f->box_tests;
    src/graphics\g3d\g3d.c:288:    s_stats.box_culled += f->box_culled;
    src/graphics\g3d\g3d.c:291:    s_stats.box_culled_far += f->box_culled_far;
    41:static uint32_t s_frames;
    42:static uint32_t s_skipped_presents;
    118:        ++s_skipped_presents;
    157:    ++s_frames;
    160:    if (s_frames % 600u == 0u) {
    194:               (unsigned long)late, (unsigned long)resyncs, (unsigned long)s_skipped_presents);
    195:        s_skipped_presents = 0;
    ✓ • 1.15s

## Activity

    file changes: Completed · 2 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\graphics\g3d\textures.c
    @@ -389,3 +389,5 @@
         }
    +#ifndef HGSS_PROFILE_OFF
         s_stats.compared_bytes += total;
    +#endif
         kj_perf_count(KJ_COUNT_TEX_BYTES, total);
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\graphics\renderer.c
    @@ -117,3 +117,5 @@
             s_skipped_last = true;
    +#ifndef HGSS_PROFILE_OFF
             ++s_skipped_presents;
    +#endif
             kj_perf_present_skipped(in->game_frame);
    @@ -156,3 +158,5 @@
     #endif
    +#ifndef HGSS_PROFILE_OFF
         ++s_frames;
    +#endif
         kj_perf_publish_end(in->game_frame);

## Activity

    $ Get-ChildItem build/private -ErrorAction SilentlyContinue; Get-ChildItem C:/Users/terezi/AppData/Roaming/Azahar/sdmc/3ds/kohjoh/data -ErrorAction SilentlyContinue; Get-Content build/instrumented-build.log -Tail 6; rg -n 'error:|vendored library build failed' build/instrumented-build.log | Select-Object -First 10
    
    
        Directory: C:\Users\terezi\AppData\Roaming\Azahar\sdmc\3ds\kohjoh\data
    
    
    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    -a----         11/9/2026   6:02 PM          44485 heartgold.manifest.json
    -a----         11/9/2026   6:02 PM      125556080 heartgold.pak
      CC  os_alarm_irq_handler.c
      CC  os_alloc.c
      CC  os_arena.c
      CC  os_context.c
      CC  os_entropy.c
      CC  os_emulator.c
    
    
    ✗ (1) • 2.49s

## Activity

    $ Get-Content src/runtime/kj_g3.c | Select-Object -Skip 1600 -First 40; Get-Content src/runtime/kj_g3.c | Select-Object -Skip 2460 -First 50; rg -n '#if.*KJ_DL_CACHE|#endif.*KJ_DL' src/runtime/kj_g3.c; Get-Content build/instrumented-build.log -Tail 4
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
        }
        s_param_count[0] = 0;
    }
    
    /* Bulk packed-FIFO decode (DMA'd display lists): parameters are passed to
     * execute() in place, without per-word calls or copies. A command whose
     * parameters run past the end of the buffer goes through the word-at-a-time
     * path, which also finishes any command left open by an earlier call. */
    static u32 s_fifo_calls;
    
    
    /* ------------------------------------------------- retained display lists */
    
    /*
     * Retained display lists (OPT-003), event replay.
     *
     * NNS DMAs each model's command lists every frame; static scenery sends the
     * same words with the same entry state. A list is recorded once as a
     * sequence of events:
     *   STATE  a command that is not vertex data (matrix, material, BEGIN/END,
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
    36:#if !defined(KJ_DL_NO_CACHE) && !defined(KJ_DL_CACHE)
    2209:#ifdef KJ_DL_CACHE
    2215:#ifdef KJ_DL_CACHE
    2250:#ifndef KJ_DL_CACHE
      CC  g2d_node.c
      CC  g2d_oam.c
      CC  g2d_oam_soft_emu.c
      CC  g2d_palette_table.c
    ✓ • 2.25s

## Assistant

The binary inspection so far shows the normal executable has no frame, sound, GPU-time or sampled-FIFO report strings and no mixer or FIFO sampling counters. Ordinary startup/error logging remains, and timing needed for game pacing remains active. The new FIFO samples are deliberately coarse; they do not yet isolate model traversal or split replay into state handling versus copying.

## Activity

    $ Get-Content build/instrumented-build.log -Tail 8; rg -n 'error:|vendored library build failed' build/instrumented-build.log | Select-Object -First 10; Get-Content src/compat/nitro/src/port_timer.c | Select-Object -Skip 200 -First 23; rg -n 'NitroPort_RecordSoundPump|SoundPumpStats|SoundPumpTime' src
       45 |     SND_SetupChannelPcm(handle->chNo, format, data, loopFlag ? SND_CHANNEL_LOOP_REPEAT : SND_CHANNEL_LOOP_1SHOT, loopStart, loopLen, volume, 0, (u32)timer, pan);
          |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    C:/Users/terezi/workdir/kohjoh/build/hgss-nns/libraries/hgss_nns/snd_waveout.c:28:20: note: 'loopLen' was declared here
       28 |     int loopStart, loopLen;
          |                    ^~~~~~~
    built ../../../build/instrumented/sdk/libnitrosystem.a
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
    
    /* ------------------------------------------------------- sound-pump time */
    
    /* The main program reports how long a NitroPort_SoundPumpReplies call took
     * (the API is here because this file owns the tick counters; the call site
     * is hgss/game/app/main_3ds.c). */
    void NitroPort_NoteSoundPumpTicks(u64 ticks)
    {
        sSoundPumpTicks += ticks;
        sSoundPumpCalls++;
    }
    
    void NitroPort_SoundPumpTickStats(u64 *ticks, u32 *calls)
    {
        *ticks = sSoundPumpTicks;
        *calls = sSoundPumpCalls;
    }
    
    void NitroPort_TimerSetReload(int id, u16 count)
    {
        if (id < 0 || id > 3) {
            return;
        }
    ✗ (1) • 1.45s

## Activity

    $ Get-Content build/instrumented-build.log -Tail 8; rg -n 'error:|vendored library build failed' build/instrumented-build.log | Select-Object -First 10
          |                    ^~~~~~~
    built ../../../build/instrumented/sdk/libnitrosystem.a
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
      129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
    == duplicate symbols
       0 symbols defined in more than one object (0 objects weakened)
    == Wi-Fi stand-ins
    ✗ (1) • 1.38s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\docs\building-windows.md
    @@ -113,3 +113,3 @@
     | --- | --- |
    -| `HGSS_PROFILE=on` | compiles in the HGSS tick probes (H-blank split timings in `port_os.c`). Off by default since `0089b28`; before that every build carried them |
    +| `HGSS_PROFILE=on` | enables frame, GPU, FIFO, H-blank, timer and sound profiling; leave unset for a baseline. Use a separate output directory for each variant. |
     | `HGSS_SND_ADPCM=1` | the alternative sample-saturation path in `port_sound_chip.c` |

## Activity

    $ rg -n 'kj_gpu_quads_dropped|s_arena_dropped|s_skipped_screens|s_stats\.' src/graphics/pica/gpu.c src/graphics/g2d/g2d.c | Select-Object -Last 15; Get-Content build/instrumented-build.log -Tail 8
    src/graphics/g2d/g2d.c:801:    ++s_stats.draws;
    src/graphics/g2d/g2d.c:866:static uint32_t s_skipped_screens, s_reused_layers, s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1110:            ++s_stats.unsupported_modes;
    src/graphics/g2d/g2d.c:1134:        s_stats.bg_quads += pr->bg[bg].count;
    src/graphics/g2d/g2d.c:1158:            s_stats.obj_quads += pr->obj[i].count;
    src/graphics/g2d/g2d.c:1271:            ++s_skipped_screens;
    src/graphics/g2d/g2d.c:1292:        ++s_stats.unsupported_modes; /* VRAM / main-memory display: tracked blocker */
    src/graphics/g2d/g2d.c:1308:        ++s_stats.spans;
    src/graphics/g2d/g2d.c:1321:            ++s_stats.spans;
    src/graphics/g2d/g2d.c:1355:    s_stats.skipped_screens = s_skipped_screens;
    src/graphics/g2d/g2d.c:1356:    s_stats.reused_layers = s_reused_layers;
    src/graphics/g2d/g2d.c:1357:    s_stats.rebuilt_layers = s_rebuilt_layers;
    src/graphics/g2d/g2d.c:1358:    s_stats.char_bytes_compared = s_char_bytes_compared;
    src/graphics/g2d/g2d.c:1359:    s_skipped_screens = s_reused_layers = s_rebuilt_layers = 0;
    src/graphics/g2d/g2d.c:1362:    out->dropped_quads += kj_gpu_quads_dropped();
          |                    ^~~~~~~
    built ../../../build/instrumented/sdk/libnitrosystem.a
    make: Leaving directory '/c/Users/terezi/workdir/kohjoh/src/compat/nitrosystem'
    == overlay table
      129 overlays (37 with data, 2 with static initializers), 828 objects (85 not in this build): sections grouped per overlay
    == duplicate symbols
       0 symbols defined in more than one object (0 objects weakened)
    == Wi-Fi stand-ins
    ✓ • 1.37s

## Activity

    file changes: Completed · 1 changes
    Update { move_path: None }: C:\Users\terezi\workdir\kohjoh\src\graphics\pica\gpu.c
    @@ -134,3 +134,5 @@
         if (s_arena_used + quads > ARENA_QUADS) {
    +#ifndef HGSS_PROFILE_OFF
             s_arena_dropped += quads;
    +#endif
             return NULL;

## Activity

    $ C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/Users/terezi/workdir/kohjoh && HGSS_PROFILE=on python tools/game/hgss_game.py --root /c/Users/terezi/workdir/kohjoh/build/upstream --target 3ds --out /c/Users/terezi/workdir/kohjoh/build/instrumented -j 32 > /c/Users/terezi/workdir/kohjoh/build/instrumented-build.log 2>&1'
    ✓ • 3m 06s

## Activity

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
