# Project status

2026-11-27: cleanup pass 2 landed on the `cleanup-2` worktree branch (`b28165c`
through `cb9f534`), started from `de26b3c` so the uncommitted work in the main
checkout stayed untouched. It makes the format/native CI gates trustworthy,
builds the differential "before" side from the reference tree, makes wide-art
init transactional, bounds the profile probe line and the arena arithmetic,
prefixes G2D and matrix private names, and adds palette-snapshot measurement
only. Every commit passed `check_format.py` and `run_host.py`; hot-path hashes
and hot-function sizes are unchanged; both editions and `HGSS_PROFILE=on`
build. CI confirmation needs a push, and the O3DS palette measurement is
**pending: no O3DS run**, so performance neutrality is not claimed on hardware.
See [code-cleanup-acceptance.md](code-cleanup-acceptance.md#cleanup-pass-2-branch-cleanup-2-from-de26b3c).

2026-11-20: the virtual touch panel now reports a released stylus as the
ARM7 does (`touch=OFF`, `validity=INVALID_XY`), not as a valid `(0,0)`
sample. HGSS's `TouchOutput_CollectValid` selects by validity, so the old
pen-up sample entered the Trainer Card signature stroke as an origin point
and drew the diagonal strokes of
[#28](https://github.com/estyseesghosts/kohjoh/issues/28). A host test
compiles the real `port_tp.c` and checks release semantics, calibration,
clamping and stroke continuity. Gold normal is rebuilt (5,777,648 bytes).
See the [touch worklog](worklog/2026-11-20-touch-signature.md).
**O3DS_UNTESTED** for this pass.

2026-11-18: HeartGold's debug build includes the blend-order and save-import
repairs; the LOTUS live-button mapping is corrected. Native checks pass and
the current HeartGold debug build booted in LOTUS. A cross-project LOTUS retest
converted and saved a locally wrapped packed save with an existing `.orig`;
HGSS import validation remains pending. Fly and dark cave widescreen issues are
still open. See the
[repair worklog](worklog/2026-11-18-resumed-repairs.md) for scope and evidence.
**O3DS_UNTESTED** for this pass.

As of 2026-11-13; audio updated 2026-11-15 ([audio worklog](worklog/2026-11-15-audio.md)). Evidence levels: **built** (compiles and links), **run**
(boots in Azahar), **verified** (checked in Azahar against an expectation),
**O3DS** (measured on an Old 3DS). Hardware logs below supply O3DS observations; sustained deadline certification remains open.

2026-11-14: scene profiler audit adds sampled per-list attribution, persistent
cache rejection reasons, renderer-inclusive game-frame budgets and properly
associated completed-queue timings. Submission intervals are now explicitly
distinct from unobserved LCD presentation. An offline visibility census
reproduces 72 wholly offscreen Goldenrod batches. See the
[investigation and hardware run plan](worklog/2026-11-14.md).

The subsequent [hardware-log follow-up](worklog/2026-11-14-hotpaths.md)
confirms ~37 ms logic / ~43 ms active game-frame medians in Goldenrod on O3DS.
Commit `a5f2d14` targets split global-state submissions, repeated state-list
recording, false lit-normal colour dependencies and affine matrix arithmetic.
Native correctness checks and HeartGold Azahar intro/title/Goldenrod checks
pass. Matching Goldenrod A/B reduces emulator logic median 7.3% and active
logic+publish median 6.0%, with unchanged triangles; Ho-Oh changes little.
Both editions' normal/profiled artifacts are built. Hardware speedup still
requires a matching re-test; SoulSilver runtime validation remains open.

2026-11-15 geometry work, scoped PR stack #6/#7/#8: FIFO hits compare live
inputs without temporary keys; field/title producers skip discarded scene
traversal and generate zero FIFO words on skipped ticks; DS lighting runs
on PICA and animated lit lists cache expanded positions. Native CPU/GPU
semantic comparisons and shader float24 arithmetic checks pass. Held Ho-Oh
and Goldenrod frames match the old CPU reference pixel for pixel, including
forced alternating producer skips. Normal and diagnostic HeartGold artifacts
are built at `f5f2e1e`. Hardware status remains **O3DS_UNTESTED**; audio-agent
changes are excluded. See the [lighting worklog](worklog/2026-11-15-lit-plans.md)
and [producer-gate evidence](worklog/2026-11-15-capture.md).

2026-11-15 hardware cuts (chatlog12 follow-up): stable lit lists bake to
literal colours (`84d46dc`), retained blocks outside the picture are culled
before copying (`413d0cc`), and the NNS GLB state list restores from a
prefix snapshot (`e587d4c`). The lit shader skips disabled and
non-specular lights (101 instead of 409 instructions per one-light vertex).
Azahar Goldenrod: logic p50 25.35 → 13.38 ms, triangles 5.02M → 1.94M,
draws 80.3k → 61.7k per 600 pictures. Held frames are pixel-identical.
**O3DS_UNTESTED.** See the [worklog](worklog/2026-11-15-hardware-cuts.md).

## Latest investigation (uncommitted, 2026-11-13)

`o3ds-log-3` confirms CPU pressure from geometry and audio, with GPU queues
around 4 ms in the steady field window. Previous ~80 ms "logic" figures
often accumulated **two game frames per picture**. The profiler now records
independent game-frame CPU percentiles and removes ordinary per-line probes.
Split FIFO decoding, display-list key traffic and ADPCM magnitude arithmetic
have been reduced, with matching differential output/state hashes and
synthetic ARM instruction reductions. O3DS speedup remains unmeasured.
See [today's worklog](worklog/2026-11-13.md#o3ds-log-3-repair-measurements-and-cpu-hot-paths).

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
| Audio cost plan (chatlog11, `4756c78`..`52f78b6`) | verified (Azahar) | Unicorn full-chip differential bit-exact; Azahar Goldenrod mixer 107.6 → 48.0 ticks per channel-sample, 14.8% → 6.6% of the core; decoded-ADPCM cache verify build 0 mismatches; held samples (DS) by default. O3DS unmeasured. [worklog](worklog/2026-11-15-audio.md) |
| Producer skip (PR #7, `ee0e7ac`) and PICA lighting (PR #8, `3076bb6`) | verified (Azahar) | merged with the audio work; native checks pass, title and Goldenrod correct in Azahar; [audio worklog integration](worklog/2026-11-15-audio.md#integration-with-prs-6-7-and-8) |
| FIFO cache hits compare keys in place (PR #6, merged `8fb6f54`) | verified (host) | `check_hotpaths.py` differentials and 20k-key corpus; O3DS unmeasured. [worklog](worklog/2026-11-15-fifo.md) |
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
18% in Azahar instructions (`dacb07d`), then by the chatlog11 plan to 48
Azahar ticks per channel-sample (6.6% of the core in Goldenrod;
[audio worklog](worklog/2026-11-15-audio.md)). O3DS gain unmeasured.

## Next

0. O3DS run of the 2026-11-15 builds in `build/hw-2026-11-15/` (both
   editions, normal and profile; audio plus PRs #6-#8): compare
   `[sndprof]`, `[sndpmu]`, `[sndout]` and `[gamecpu]` with `docs/logs/1`,
   and listen to held against interpolated samples (`KJ_SND_INTERPOLATE`).

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
