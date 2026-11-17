# Opening-movie starter particles — scene located, drop not yet pinned

Date: 2026-11-17. Report: the three-starter slide-in scene ("starter Pokémon
2D sprites come horizontally across the screen") is missing its grass/fire/
water particle effect versus the NDS game (melonDS 1.1 reference, now present
at `workdir/melonDS.exe` with `heartgold.nds`/`.sav`).

## Scene map (publish index; 1 publish ≈ 2 vblanks, dumps are publish-indexed)

- 400–800: Kotone running (2D). 950: 3D town. 1100–1300: Silver face
  (DISPCNT 3D bit legitimately off). 1500: Team Rocket group (2D OBJs).
- 1650–1670: Ethan + Lyra with Poké Ball.
- Starter showcase, one starter per ~100 publishes, each slides in from the
  right with its effect puff and settles centred on a gradient (BG3):
  Chikorita lands ~1690 (grass), Cyndaquil slides ~1740–1780 (fire,
  `f01770.kjd` shows the fire puff mid-slide), Totodile lands ~1800
  (water). 1900: Ho-Oh over the ocean. 2100: HGSS logo splash.

## Evidence so far

- `.kjd` dumps at every 10 publishes across 1650–1850 (`starter_scan.py` for
  the census, `ds_reference.py` for the renders): the particle puffs ARE in
  the DS state as mode-0 OBJs; the offline oracle renders them.
- Freeze-and-capture at publish 1770 in LOTUS: kohjoh renders the Cyndaquil
  slide + fire puff identically to the oracle (`frz1770/top.png` vs
  `orc2d1770_screens.png`). Publish 1700 frozen also matched (Chikorita).
- So the drop is not a static state issue at these instants; remaining
  hypothesis is a live-playback-only effect (retained-layer reuse missing a
  fast-changing OBJ burst, the same class as the call-text bug fixed in
  `5601e06`) or a moment outside the sampled windows.
- Live-capture sweep tool: `run 45` + capture steps across the slide. Two
  attempts failed on LOTUS capture quota ("FAIL" for every capture); the
  bridge restart resets the artifact quota — rerun the sweep after a bridge
  restart before code changes.

## Tools added

- `tools/capture/starter_scan.py`: per-dump census (DISPCNT, visible OBJ
  counts by mode, OBJ bounding boxes) to locate sprite bursts.
- `tools/lotus/battle_arrows.script`: vblank-indexed drive template for the
  battle stat-change-arrow repro (title → CONTINUE → Route 29 grass →
  encounter → DOWN+A), timings to be tuned from captures.

## Next

1. Restart the LOTUS bridge (quota), rerun the live sweep across publishes
   1690–1800, diff every capture against the oracle of the nearest dump.
2. Battle stat-arrow repro with `battle_arrows.script`; check WININ/WINOUT
   and OBJ-window semantics against `g2d.c` (mode-2/3 OBJs are skipped in
   `objs()`; the OBJ window is treated as the WINOUT region).

## Root cause and fix (follow-up session)

The puffs are SPL 3D particles, not 2D OBJs, so the 2D-only oracle could not see them.
`lib/src/spl_draw_ptcl.c` stored its geometry straight to `reg_G3_*`, which on the port
is plain memory; patch 2004 had only routed `src/`. Dumps showed 0 G3 batches during the
particle phases while breakpoints confirmed `spl_draw`/`spl_draw_bb` ran every frame.

Patch 2008 routes all 61 stores through `NitroPort_GxDirectWrite` (same model as 0604/0605/2004).
Verified in LOTUS (kj-prof, frame 3450): leaves and green sparkles render around Chikorita.
This should also restore SPL effects in battle and on the field.
