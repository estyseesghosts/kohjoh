# 2026-11-20 Trainer Card pen-up strokes (issue #28)

[#28](https://github.com/estyseesghosts/kohjoh/issues/28): drawing a signature
on the physical 3DS produced long diagonal strokes toward the canvas corner
between pen-down samples. The report correctly identified the virtual touch
panel's pen-up sample as the cause; the earlier access failure in
[#25](https://github.com/estyseesghosts/kohjoh/issues/25) was a stale-build
test and is unrelated.

## Root cause

`publish_touch()` in `src/compat/nitro/src/port_tp.c` reported a released
panel as `touch=OFF, x=0, y=0` but still set `validity=SPI_TP_VALIDITY_VALID`.
That is not what the ARM7 reports. In the pinned driver
`build/upstream/sub/lib/src/touch_panel_driver.c`, `TP_ExecSampling`'s
no-touch path publishes `x=0, y=0, touch=OFF` and
`SPI_TP_VALIDITY_INVALID_XY`.

The distinction matters because HGSS does not use `touch` to build the
signature stroke. `build/upstream/src/touchpad.c`'s
`TouchOutput_CollectValid()` selects a sample on `validity == 0` alone, so
each pen-up sample entered the stroke as a usable `(0, 0)` point.
`overlay_trainer_card_signature.c` then connected consecutive collected
points with `TrainerCardSignature_DrawLine()`, producing the diagonal from
the last real sample to the origin. The NitroSDK side matches the driver:
`TP_GetCalibratedPoint` keeps `validity` while zeroing `x`/`y` for
`touch=OFF`, and `TPData`'s own documentation says `validity` describes the
chattering measurement, not the touch status.

## Fix

`publish_touch()` now sets `validity` from `down`: `SPI_TP_VALIDITY_VALID`
while pressed, `SPI_TP_VALIDITY_INVALID_XY` on release. The coordinates were
already zeroed on release, so the published sample is now byte-identical in
meaning to the ARM7's no-touch sample.

The fix stays in the virtual driver rather than patching HGSS. The game's
release detection already uses `touch`
(`ReadKeypadAndTouchpad` in `build/upstream/src/system.c`), so no game-side
filter is needed; correcting the driver restores the original
game/hitbox semantics for every touchpad user, not just the signature
overlay.

## Test

`tests/native/touch_tp_test.c` compiles the real `port_tp.c` against minimal
SDK stubs and replays a pen-down / drag / pen-up / re-down trace through the
same calibration and validity filter the game uses. Checks:

- a release publishes `touch=off, validity=invalid, x=0, y=0`;
- a press publishes `raw = screen * 16` and calibration restores the screen
  position with `validity=valid`;
- pen-up frames contribute no drawable point, and the following pen-down
  does not join the previous stroke;
- coordinate clamping is unchanged (`255, 191`);
- the mutation check feeds the old valid-`(0,0)` pen-up encoding through the
  same trace check and confirms it is rejected, so the test catches a
  regression;
- auto-sampling still answers once per published sample.

Run: `python tests/native/test_touch_tp.py`. All checks pass.

## Build

`python tools/game/check_patches.py`: `106/106 patches applied, 0 missing`.

Gold normal (uninstrumented) HeartGold:
`python tools/game/build_variants.py --only gold-normal -j 32`.

- Artifact: `build/kj-gold-normal/pokeheartgold.3dsx`, 5,777,648 bytes,
  SHA-256 `e3a30119f00d8937338632340ffa62f43960b1e73f69765436b52f35b47e2e51`.
- Log: `build/kj-gold-normal-build.log`, 0 lines matching `error:` or
  `vendored library build failed`.
- `pokeheartgold.build-info.txt` records `profiling: off`, `defines: (none)`,
  `build: d5bc0f2-dirty` and `dirty: yes`. The tracked tree is `d5bc0f2`; the
  only untracked entries are the pre-existing `kohjoh*.zip` archives in the
  repository root, which are not part of the build. Two builds of the same
  sources produced the same size and different SHA-256 values, so the
  `.3dsx` packaging is not byte-reproducible.

The change was not exercised on a physical Old 3DS in this pass:
**O3DS_UNTESTED**. The emulator/host evidence covers the sample encoding and
the stroke-continuity consequence, not the signature UI itself.
