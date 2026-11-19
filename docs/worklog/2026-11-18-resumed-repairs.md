# 2026-11-18 resumed repairs

This work resumed after the LOTUS MCP connection closed during the previous
session. The connection now exposes both projects' build directories. The
commits below were pushed to `main`.

## Changes and checks

- `df5cc64` corrects live `lotus_input` button masks in `tools/lotus/run.py`.
  LOTUS uses NativeButton bits, so its START is `0x400`; the old DS bit `0x8`
  sent Y. Two Python mapping/help tests pass.
- `63f3e8d` chooses the blend pass order according to which pass changes the
  second-target stencil bit. The native stencil model passes for production
  and rejects both unconditional pass-order mutations. This is a synthetic
  test, not a PICA precision or texture-sampling test.
- `1879bfa` replaces in-place emulator-container conversion with a verified
  backup and temporary raw image; see [save import](../save-import.md).
  `12e0465` adds collision handling in Kohjoh, mirrored by Sinjoh `52b693f`:
  after an exclusive create fails, it checks whether that candidate is occupied
  and retries an exclusive create with the next suffix. Nine native importer
  tests pass in each port, including occupied backups and injected failures.
  HGSS target import validation remains pending.
- `2095a56` aligns script parsing and input-end arithmetic with Sinjoh. The native input-ownership test passes; the current HeartGold
  debug build boots in LOTUS, but the changed behavior has not been exercised
  end-to-end there.

The HeartGold debug target built with the current script and importer changes
and booted through 600 native LOTUS frames; the `.3dsx` was 5,801,812 bytes.
The build used a dirty source tree and is not a clean build of any one commit.
No SoulSilver runtime check or physical O3DS run was completed in this pass.

## Cross-project LOTUS save test

LOTUS used CITnLO worker source `c3422b0a712b14cdeabc638693a55d733b43e524`,
worker SHA-256 `c77d459de1a143d88ef168462f0a74295896562b1b5fe1fbcfdef19094921568`,
renderer 2 (Vulkan), and no renderer fallback. In a disposable Sinjoh session,
a copy of the user's raw Platinum save wrapped locally in a DeSmuME-format
container converted to byte-identical raw data and retained a byte-identical
`.orig`. Continue, journal, Mt. Coronet field, in-game Save, reset and Continue
back to the field succeeded. This used a locally constructed wrapper, not an
independently exported emulator file, and does not validate either HGSS edition.

A locally constructed packed No$GBA container from that save initially failed
to import with `.orig` occupied; the packed input and earlier `.orig` were
unchanged. After the collision-handling change, a Sinjoh debug LOTUS retest
preserved the existing 524,410-byte `.orig`, created a 528,498-byte `.orig.1`,
and produced a 524,288-byte raw save matching the expected original exactly.
The player continued to Mt. Coronet, saved, reset, and continued back to the
field; the raw file changed after saving. Both backups still matched their local
fixtures byte-for-byte. The errno from the original failure was not measured.
HGSS tests with real emulator exports remain pending. The packed
container whose total length equals 512 KiB is currently treated as raw; see
the import guide for that ambiguity. The 3DS filesystem's rename is not a
crash-safe guarantee; [Coffeepot's save guidance](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/filesystem-and-saves.md)
describes the relevant flush and rename limits.

## Open investigations

- [#22](https://github.com/estyseesghosts/kohjoh/issues/22): the Flash cave
  darkness is a 3D model, resource `0x6A`, drawn by
  `build/upstream/src/field/field_flash_darkness.c`. Its reported clipping
  still needs a reproducible cave capture and viewport/matrix inspection.
- [#23](https://github.com/estyseesghosts/kohjoh/issues/23): Fly's cut-in
  uses WIN0 with a right bound of 255 in
  `build/upstream/src/field/field_move_cut_in.c`; the renderer's wide-edge
  check looks for 256. This is a candidate mismatch, not a verified complete
  fix for the cloud/sprite effect. A Fly fixture and center/margin comparison
  are needed before changing presentation behavior.
- [#9](https://github.com/estyseesghosts/kohjoh/issues/9) (Lugia hardware
  crash) and [#2](https://github.com/estyseesghosts/kohjoh/issues/2)
  (cartridge-save crash) remain open. The wider scene oracle and physical
  O3DS timing gates also remain open. LOTUS timings do not establish hardware
  performance.
