# Code-cleanup acceptance (2026-11-27)

Evidence for [`cleanup.txt`](cleanup.txt), against the frozen baseline in
[`code-cleanup-baseline.md`](code-cleanup-baseline.md). Emulator results are
correctness evidence only; this machine had no Old 3DS attached, so no new
O3DS timing claim is made.

## Builds

| Build | Result |
| --- | --- |
| HeartGold 3DS, strict first-party warnings (`-Werror`) | passes, 5,958,172-byte `.3dsx` |
| SoulSilver 3DS, same tree | passes, 5,958,164-byte `.3dsx`, SHA-256 `ab9cc82df2ffdc9de24f5bbd46d08b9c2a89b0888aea719af4b65a27c4936328` |
| Profile-only reporter timing (`perf.c`) | compiles; run on O3DS pending |

The strict policy is `-Wall -Wextra -Wshadow -Wundef -Wstrict-prototypes
-Wmissing-prototypes -Wpointer-arith -Wformat=2 -Werror` for
`src/app`, `graphics`, `audio`, `storage`, `content` and `diagnostics`.
`src/runtime` uses the same set minus the C-prototype rules and
`-Wignored-qualifiers` (a NitroSDK-header exception documented next to the
flag list in `tools/game/hgss_game.py`).

## Native regression suite

`python tests/native/run_host.py` covers config, field view, layout, world
view, native-text capacity, perf accounting, texture cache, save backend and
import, touch, script input, visibility, PICA lighting, the new matrix test
and the cached/uncached G3 and sound differential harness. Logs are kept in
`build/cleanup-native-*.log`. The suite also ran through the formatting-only
commit; its differential hashes (`e8bfdd766ba03a03` sound,
`d511eb58d36aceeb` g3) did not change.

## Correctness changes exercised

| Task | Evidence |
| --- | --- |
| 2 texture cache | palette-only and format-5 invalidation, transactional refresh, recovery after a failed refresh (`tests/native/test_textures.py`) |
| 3 save startup | create/import/missing-dir/unwritable/invalid follow-up (`tests/native/test_save_backend.py`, `test_save_import.py`) |
| 4 crash logging | handler no longer calls the ring logger; crash-file write is best effort; `KJ_CRASH_TEST` hook |
| 5 stop flags | atomic acquire/release stop protocols, failed-thread cleanup |
| 6-8 lifecycle | renderer/child modules unwind partial init and free retained caches; `shaderProgramInit` is checked as a `Result` (0 = success), so the renderer no longer aborts silently on a valid program |
| 9 text capacity | exact-capacity and capacity+1 LRU eviction (`tests/native/text_test.c`) |
| 10 warnings | clean strict build above |
| 11 CI | native job, PR-base reference, texture/save/capacity tests |
| 12 formatting | pinned clang-format 18.1.8, first-party files only, zero output change |
| 13 frame phases | `kj_ctr_publish_frame` named phases, operation order unchanged |
| 14 G2D | decoding moved to `g2d_layers.c`; retained caches/allocation/byte comparison moved to `g2d_retained.c`; `g2d_internal.h` is private to the subsystem; window/blend/brightness stay with the span orchestrator because they interleave its priority pass |
| 15 G3 | pure matrix support moved to `kj_g3_matrix.c/.h`; hot decode, stacks and retained lists stay in one translation unit |
| 16 boundaries | every 3DS and runtime header compiles standalone; new exports are in subsystem-private headers |
| 17 reporter | `[perfrep]` window timing is profile-build only and not attributed to game/render |
| 18 docs | current raw-save path, lifecycle ownership and split boundaries documented |

## Emulator correctness

LOTUS (0.1.4, AZAHAR backend, Old 3DS configuration, software renderer) runs
the cleanup artifact through the intro. The recorded capture at 1,798 guest
frames shows the widened New Bark scene and trainer, matching the baseline
artifact run in the same session setup. The screenshot is
`build/lotus/cleanup-intro/final-top.png`; the baseline comparison is
`build/lotus/cleanup-base-lotus/final-top.png`. LOTUS is an emulator: this
verifies that the pack, overlays, renderer and GPU path boot and draw, not
O3DS pacing.

## Performance position

The normal build remains the timing authority. Final normal-build symbol sizes
(HeartGold, `arm-none-eabi-nm -S --size-sort`) are: `fifo_words_impl` 0x21d0
(baseline 0x21cc), `kj_screen_draw` 0x89c (0x860), `kj_g3d_draw` 0x2a4
(unchanged), `kj_tex_get` 0x8d0 (0x950), `kj_text_frame` 0x9ac (0x930),
`kj_ctr_publish_frame` 0x1a8 (0x198), `kj_renderer_init` 0x1c4 (0x138),
`kj_atlas_tile` 0x450 (unchanged). The 60-byte `kj_screen_draw` growth is the
cross-module call glue for the extracted layer/retained helpers; the work
itself moved to `g2d_layers.c`/`g2d_retained.c`. No O3DS hardware run was
available, so pacing, texture upload counts and command-buffer pressure are
not claimed as measured after the cleanup. A follow-up run of the same
Goldenrod workload on the same O3DS remains required before treating the
structural splits as performance-neutral on hardware.
