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
| Input: buttons and touch | verified | touch BAG opens the Bag; B closes it |
| Pacing lock (no music drift after late frames) | run | `[pace]` counters; user-reported drift fixed in code, not re-confirmed |
| Tile-row gaps fix (texcoord inset) | built | awaiting user confirmation |
| Retained 2D layers, unchanged-screen skip | verified | Goldenrod: bottom screen skipped 557 of 600 frames |
| Geometry decoder fast paths | verified | Goldenrod renders; logic 37 → 25 ms/frame (Azahar) |
| Pokémon-data keystream cache (game patch 2002) | built | crypto was ~12% of field instructions |
| Display-list cache | **off** (`--define KJ_DL_CACHE`) | **do not enable on master**: it corrupts the heap; fixed on `wip/savestore` |
| Saves | run | interim raw file `saves/<edition>.sav` ([`save_raw.c`](../src/storage/save_raw.c)) |
| Audio | run | user heard music; Azahar here lacks DSP firmware (`ndspInit` fails) |
| CIA, RomFS pack, title save archive | not implemented | path lookup only ([`paths.c`](../src/storage/paths.c)) |

### Performance (Azahar instruction counts, not O3DS)

Azahar charges one tick per guest instruction (`[calib]` log line), so its
times are instruction counts — a floor for O3DS, which also pays cache misses.
Goldenrod at rest: game thread ~25 ms/frame (geometry decode is the largest
part), publish ~5 ms, ~29% idle. Full speed on O3DS is **not** established and
is unlikely at these numbers; see [next work](#next).

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
2. Title screen: Ho-Oh/Lugia 3D missing on the lower screen.
3. Route test: Goldenrod → Route 34 grass → wild battle (`tests/inputs/route34-south.txt`).
4. Cut geometry cost: display-list cache hit rate (texgen-1 lists), then O3DS
   test kit with ARM11 performance counters.
5. CIA packaging, SoulSilver build.
