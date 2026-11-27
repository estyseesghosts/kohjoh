# Code-cleanup baseline (2026-11-27)

Reproducible facts for the cleanup plan in `docs/cleanup.txt`. No source was
changed while this baseline was collected.

## Commit and configuration

| Item | Value |
| --- | --- |
| Kohjoh commit | `10444381a55f05cb4a42fcee8c52292943f3361a` (clean tree) |
| Coffeepot reference | `209c8ed42406453f8cff8c7f2df86c2723bc759d` |
| Target | `--target 3ds`, normal build (`HGSS_PROFILE` unset, `--define` none) |
| Compiler | devkitARM `arm-none-eabi-gcc` 16.1.0, devkitPro at `/opt/devkitpro` |
| Host | Windows 11, devkitPro MSYS2 login shell |
| Build command | `python tools/game/hgss_game.py --root build/upstream --target 3ds --game <edition> --out build/cleanup-base-<edition> -j 32` |
| Renderer configuration | `field_scale = legacy` (default), native text legacy (default); every `HGSS_*` / `--define` diagnostic off (see each `.build-info.txt`) |

The baseline was built from a detached worktree of the commit above so later
edits could not contaminate it. Raw build log:
[`cleanup-baseline-build.log`](logs/cleanup-baseline/cleanup-baseline-build.log).

## Binaries

| Edition | `.3dsx` bytes | ELF bytes | `.3dsx` SHA-256 |
| --- | ---: | ---: | --- |
| HeartGold | 5,954,504 | 13,667,900 | `c15553ee8c3d1ce383f386de09370d6c9d8e5d97f9830638d4a232d871c6d6a3` |
| SoulSilver | 5,954,496 | 13,667,852 | `539b9a19577289134e9d19d0c0dc873408acda62c1cad2e57421e3f58ed0a0b1` |

Both `.3dsx`/ELF pairs are preserved under `build/cleanup-base-gold/` and
`build/cleanup-base-silver/` for later comparison.

## Selected function sizes (`arm-none-eabi-nm -S --size-sort`, HeartGold)

| Symbol | Bytes |
| --- | ---: |
| `fifo_words_impl` (the GX FIFO decode body behind `NitroPort_GxFifoWords`) | 8,652 |
| `kj_screen_draw` | 2,144 |
| `kj_g3d_draw` | 676 |
| `kj_ctr_publish_frame` | 408 |
| `kj_renderer_init` | 312 |
| `kj_targets_init` | 376 |
| `kj_tex_get` | 2,384 |
| `kj_text_frame` | 2,352 |
| `kj_atlas_tile` | 1,104 |

`fifo_words_impl` stays well under the 16 KiB instruction-cache budget named in
`docs/testing.md`.

## Native regression result

`python tests/native/run_host.py` at this commit: see
[`cleanup-native-run.log`](logs/cleanup-baseline/cleanup-native-run.log). It
covers the differential hot-path harness (`check_hotpaths.py`), config,
field-view, layout, world-view, native-text, perf accounting, texture cache,
save backend/import, touch, script input, visibility and PICA lighting
arithmetic. The tree's `tests/native/layout_test.c` still asserted that the
field screen has no wide-layout rows, while `src/runtime/kj_layouts.c` has two
field rows (#19); that pre-existing assertion is corrected as a test-only fix
before this run so the recorded suite result reflects production behaviour.
Renderer changes and the CI task use the corrected test.

## Hardware frame/pacing baseline

No Old 3DS was attached for this collection, so no new O3DS timing is claimed.
The most recent recorded hardware observations in `docs/status.md` are:

* profiled Goldenrod field: `active logic+publish` p50 16.9 ms / p95 18.3 ms
  on both v0.0.5 and the layout-profile build;
* a 2-VBlank frame interval (33.44 ms p50), 0 skipped presents, 0 hitches;
* layout validation 182 us/frame on the emulator.

These are historical hardware numbers for the same workload class, not a
measurement of this baseline artifact. Emulator (Azahar/LOTUS) instruction
counts are correctness evidence only and are never compared with O3DS
milliseconds.

## What later tasks must keep visible

* the normal build, not a profile build, is the timing authority;
* the display-list cache/arena, retained G2D byte comparison, GPU lighting,
  culling/join paths, texture hinting and present pacing are measured fast
  paths, not cleanup targets;
* every later comparison must use the same edition, configuration, workload
  and hardware.
