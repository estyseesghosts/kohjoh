# Project status

As of 2026-11-12. Evidence levels: **built** (compiles and links), **run**
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
| Display-list cache | verified, **off by default** (`--define KJ_DL_CACHE`) | heap fix ported; `KJ_DL_VERIFY` 0 mismatches in Goldenrod idle, intro, title; battle and walking untested |
| GX command DMA in one transfer (SDK patch 2003) | verified | 65% of FIFO words had been split mid-command; logic 21.8 → 21.0 ms |
| Party move scan, one decrypt per mon (game patch 2006) | verified | Goldenrod idle logic −0.1 ms |
| Saves | run | interim raw file `saves/<edition>.sav` ([`save_raw.c`](../src/storage/save_raw.c)) |
| Audio | run | user heard music; Azahar here lacks DSP firmware (`ndspInit` fails) |
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

## Next

1. Find the save-detection regression on `wip/savestore`; finish SaveStore.
2. Battle crashes (user-reported): reproduce with `route34-battle.txt`; `logs/crash.txt`.
3. Route test: Goldenrod → Route 34 grass → wild battle (`tests/inputs/route34-south.txt`).
4. O3DS re-test of Goldenrod and the title with the skip cap (the white
   title is unconfirmed). Then cut geometry cost further: display-list cache
   (fix on `wip/savestore`) and ARM11 performance counters for I-cache
   misses on hardware.
5. CIA packaging, SoulSilver build.
