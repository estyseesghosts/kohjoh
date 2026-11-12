# Troubleshooting and lessons from bring-up

This page records failures observed in the project transcript and the
corresponding fixes. Code links show where the maintained workaround lives;
[`chatlog.md`](chatlog.md) has the chronological details. These are project
observations, not universal Windows or MSYS behavior.

| Symptom | Cause found | Fix or current handling | Evidence |
| --- | --- | --- | --- |
| Compiler cannot create temporary files. | `TMP` pointed at `C:\Windows`, which was not writable in that run. | Build under a writable scratch/work directory; the compatibility make layer exports `TMP`/`TEMP` for child processes. | [`common.mk`](../src/compat/common.mk), [`hgss_nitro.py`](../tools/game/hgss_nitro.py) |
| Host generator fails because MSYS GCC lacks libc headers. | Bootstrap tried to compile a decomp host tool only to produce generated constants. | Generate game headers with the Python implementation in [`prepare_game_headers.py`](../tools/game/prepare_game_headers.py), and omit the redundant host-tool step in [`bootstrap.py`](../tools/game/bootstrap.py). | [`bootstrap.py`](../tools/game/bootstrap.py) |
| SDK link paths are stale or contain copies. | In this setup MSYS Python could create directory copies where symlinks were intended. | Bootstrap checks whether the path really points at the upstream library and refreshes a safe copy when needed. | [`bootstrap.py`](../tools/game/bootstrap.py), `link_sdks` |
| Archive fails with “argument list too long.” | Deep scratch path made archive command arguments exceed the Windows tool's limit. | Use a shorter in-repository build path. | [`chatlog.md`](chatlog.md) |
| Python cannot find files under `/home/...` while running from Windows. | MSYS Python's current-directory resolution differed from the Windows-mounted script path. | Pass absolute `/c/...` paths to the driver. | [`chatlog.md`](chatlog.md), [`hgss_game.py`](../tools/game/hgss_game.py) |
| Native linker reports object files missing even though compile succeeded. | `/c/...` paths inside a response file were not converted by MSYS for the native linker. | Emit native `C:/...` paths in files consumed by native Windows tools; keep MSYS paths for MSYS Python. | [`hgss_game.py`](../tools/game/hgss_game.py) |
| Incremental build recompiled every game source. | Generated headers were rewritten each run and dependency parsing split paths at the drive-letter colon. | Keep stable generated headers, parse the dependency target separator correctly, and compare object/source/flags/upstream signatures. | [`hgss_game.py`](../tools/game/hgss_game.py) |
| Cache-enabled scene showed white map geometry and ran at roughly 1% speed. | Not conclusively diagnosed. Evidence pointed at display-list cache recording/replay; sprites still used the uncached path. | Disable cache by default; verify the cache-off commit restored intro and Goldenrod. Cache remains behind `HGSS_GX3D_LISTCACHE=1`. | [`hgss_game.py`](../tools/game/hgss_game.py), [`kj_g3.c`](../src/runtime/kj_g3.c), [`status.md`](status.md) |
| Verify run appeared to hang in a wait loop. | It did not reach the requested frame; stack samples showed the main loop waiting for VBlank amid heavy activity. Root cause remains unresolved. | Do not call this a confirmed deadlock. Compare cache-disabled and cache-enabled builds, then profile the recording path and replayed state. | [`chatlog.md`](chatlog.md), [`status.md`](status.md) |

## Graphics correctness fixes recorded

During implementation, review caught a non-normalized 2D shader colour,
colour writes left disabled after window rendering, missing helper functions,
atlas/palette-list storage that could break contiguous palette slices, and
tile-edge gaps addressed with an inset. These were corrected in successive
commits; consult the renderer sources and commit history before treating any
one transcript snapshot as current. Relevant code: [`src/graphics/g2d`](../src/graphics/g2d),
[`atlas.c`](../src/graphics/resources/atlas.c), and [`kj_g3.c`](../src/runtime/kj_g3.c).

Coffeepot's [debugging guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/debugging.md)
and [GPU command pipeline notes](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/pica200-command-pipeline.md)
are useful background, but the fixes above are attributed to kohjoh's own
transcript and source rather than to the wiki.
