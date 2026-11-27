# Code-cleanup acceptance (2026-11-27)

Evidence for [`cleanup.txt`](cleanup.txt), against the frozen baseline in
[`code-cleanup-baseline.md`](code-cleanup-baseline.md). Emulator results are
correctness evidence only; this machine had no Old 3DS attached, so no new
O3DS timing claim is made.

## Builds

| Build | Result |
| --- | --- |
| HeartGold 3DS, strict first-party warnings (`-Werror`) | passes, 5,957,904-byte `.3dsx` |
| SoulSilver 3DS, same tree | passes, 5,957,896-byte `.3dsx`, SHA-256 `0872853ffbee9261e81a47f12fed45589c0ecf42eb0eaf0c254a5ecd833baa3d` |
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
| 14 G2D | decoding in `g2d_layers.c`, retained caches in `g2d_retained.c`, windows/blend/brightness/coordinate mapping in `g2d_effects.c`; `g2d_internal.h` private; wide-art key/band stays with the span that owns its tile snapshot |
| 15 G3 | pure matrix support in `kj_g3_matrix.c/.h`, display-list bump arena in `kj_g3_arena.c/.h`; hot decode, stacks and retained list replay stay in one translation unit with the shared live state |
| 16 boundaries | every 3DS and runtime header compiles standalone; new exports are in subsystem-private headers |
| 17 reporter | `[perfrep]` window timing is profile-build only and not attributed to game/render |
| 18 docs | current raw-save path, lifecycle ownership and split boundaries documented |

## Emulator correctness

LOTUS (0.1.4, AZAHAR backend, Old 3DS configuration, software renderer) runs
the cleanup artifact through the intro. The 1,798-frame capture shows the
widened New Bark scene and trainer (`build/lotus/cleanup-intro/final-top.png`),
matching the baseline artifact (`build/lotus/cleanup-base-lotus/final-top.png`).
The final module layout was re-run to the Game Freak splash:
`build/lotus/cleanup-modules/final-bottom.png`. LOTUS is an emulator: this
verifies that the pack, overlays, renderer and GPU path boot and draw, not
O3DS pacing.

## Performance position

The normal build remains the timing authority. Final normal-build symbol sizes
(HeartGold, `arm-none-eabi-nm -S --size-sort`) are: `fifo_words_impl` 0x2174
(baseline 0x21cc), `kj_screen_draw` 0x7e8 (0x860), `kj_g3d_draw` 0x2a4
(unchanged), `kj_tex_get` 0x8d0 (0x950), `kj_text_frame` 0x9ac (0x930),
`kj_ctr_publish_frame` 0x1a8 (0x198), `kj_renderer_init` 0x1c4 (0x138),
`kj_atlas_tile` 0x450 (unchanged). The extracted modules reduce
`kj_screen_draw` and the FIFO body slightly; per-span calls replace inlined
layer/effect code. No O3DS hardware run was available, so pacing, texture
upload counts and command-buffer pressure are not claimed as measured after
the cleanup. A follow-up run of the same Goldenrod workload on the same O3DS
remains required before treating the structural splits as performance-neutral
on hardware.

## Cleanup pass 2 (branch `cleanup-2`, from `de26b3c`)

One commit per task. Every commit passed `python tools/check_format.py` with
clang-format 18.1.8 and `python tests/native/run_host.py`. Work was done in a
separate worktree so the uncommitted changes in the main checkout were never
touched.

| Task | Commit | Change and checks |
| --- | --- | --- |
| 1 | `b28165c` | Pinned formatter applied to `g3_state_test.c`; format check exits 0; includes unchanged. |
| 2 | `c2cae55` | CI trusts the container checkout and `default_reference()` fails loudly with `CI=true`; local harness passes, `CI=true --reference does-not-exist` exits 1. CI confirmation pending push. |
| 3 | `1cdc4e4` | Hot-path "before" builds use the reference revision's own `src/runtime` (`git archive` + `tarfile`); default and `HEAD~3` hashes unchanged. A throwaway `mtx_identity` rename failed only in `kj_g3_matrix_test`, never in a "before" build; reverted. |
| 4 | `0934189` | Transactional `kj_wideart_init()` plus `tests/native/test_wideart.py`; the test SIGSEGVs on the old code and passes on the new code; both editions build. |
| 5 | `041cc37` | Bounded `[probe]` line; normal and `HGSS_PROFILE=on` builds clean; `kj_ctr_publish_frame` unchanged. |
| 6 | `a5d4265` | Overflow-safe arena sizing plus `kj_g3_arena_test.c`; the test fails at `UINT32_MAX - 3` before the fix and passes after; hot-path hashes unchanged. |
| 7 | `bd89edf` | Blend-order mutation test runs in `run_host.py`: `production: passed`, both mutations `rejected`. |
| 8 | `3b93657` | G2D private cross-file names prefixed `kj_g2d_`; all listed hot-function sizes identical; both editions build. |
| 9 | `b620d3e` | Matrix helpers prefixed `kj_g3_`; all hot-path hashes and sizes identical. See deviation below. |
| 10 | `94b7341` | FIFO **calls** named by what they count; dead `s_fifo_ticks` timing removed; `kj_g3_dl_stats` renamed to say it logs and resets; normal and profile builds clean; sizes identical. |
| 11 | `4aad007` | Palette-snapshot measurement only: `palette_bytes_compared`, `palette_generation_bumps`, `format5_entries`; `test_textures.py` passes; both builds clean; `kj_tex_begin_frame` unchanged. |
| 12 | `cb9f534` | Both CI jobs pinned to `devkitpro/devkitarm@sha256:116afba8df8453961de2936ffab20dd441edf4d682856c1ec8b0e53d7ed0bbf5`; CI confirmation pending push. |

Hot-path hashes were unchanged by Tasks 3-11: sound `e8bfdd766ba03a03`, g3
`d511eb58d36aceeb`, matrix_diff `894b2296a9950b41`, dl_dependencies_test
`04d5a8250aaaefff`, capture `09dd42eae3a89dfb`, lit plans `c79962fa0fd30bf7`,
GPU lighting inputs `1f3d9db83ecf0833`, cull `f88f16d21d934509`, pure state
lists `dffafa0256256687`.

Normal-build HeartGold symbol sizes after pass 2 (`arm-none-eabi-nm -S
--size-sort`): `NitroPort_GxFifoWords` 0x4, `fast_vertex` 0x55c,
`fifo_words_impl` 0x2174, `execute` 0x162c (`execute_body` has no standalone
symbol in this build), `kj_screen_draw` 0x7e8, `kj_ctr_publish_frame` 0x1a8,
`kj_tex_begin_frame` 0x21c. Every one matches the pre-pass baseline.

CI status: **not pushed**, as the plan requires, so there is no run link yet.
The last known red run was 45991210630 (format and dubious ownership in
`native`); the last green build-only run was 45971280335, whose image digest
is the pin above. After a push, `format` must be green and `native` must print
`PASS g3:` lines.

O3DS result for the Task 11 counters: **pending: no O3DS run**. No number is
estimated. Performance neutrality of the decomposition is therefore still not
claimed on hardware.

Deviations and open questions:

- Task 9: renaming `FX_ONE` in `matrix_diff.c` made the reference-side build
  fail with `'KJ_G3_FX_ONE' undeclared`, because that fixture is compiled
  against both revisions' private headers. With no compatibility `#define`
  allowed, the fixture now uses its own named constant
  `MATRIX_DIFF_FX_ONE 4096u` (the same 20.12 value); hashes are unchanged.
- Task 11: `renderer.c` receives `kj_g3d_stats`, and the forwarding fields live
  in `g3d.h`/`g3d.c`, which are outside the task's allowed paths. The three
  counters are read through a non-resetting `kj_tex_peek_stats()` declared in
  `textures.h`, called before `kj_g3d_take_stats()` resets them.
- Task 10: `[dlwhy]`/`[dlcov]` are still logged in normal builds. Whether to
  guard them with `#ifndef HGSS_PROFILE_OFF` is a user decision, left open.
