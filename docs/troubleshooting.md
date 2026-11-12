# Troubleshooting

Each entry: symptom → cause → fix. The tools already contain the fixes; this
page explains them for the next breakage.

## Windows build chain

| Symptom | Cause | Fix |
| --- | --- | --- |
| `Cannot create temporary file in C:\Windows\` | MSYS2 drops `TMP`/`TEMP` from child processes; native GCC falls back to `C:\Windows`. | `common.mk` exports them; `hgss_nitro.py` sets them in Python. |
| `-DFOO` set via `FOO=1 python ...` has no effect | MSYS2 Python does not see inline environment variables. | `hgss_game.py --define FOO`. |
| `mkdir: cannot create directory '/home'`; headers "not found" | MSYS2 Python resolves the working directory as `/home/<user>/...`, which bash, make and native tools cannot use; MSYS also does not convert glued arguments such as `-idirafter/c/...`. | Invoke with absolute `/c/...` paths and `--root`; pass `-idirafter` and its path as separate arguments. |
| `ld: cannot find /c/Users/.../x.o` | Native `ld` reads paths inside `@response` files literally; MSYS converts only command-line arguments. | Response files use `C:/...` (`_tool_path`). |
| `ar: Argument list too long` | Very deep output paths (e.g. under `%TEMP%`). | Build under the repository's `build/`. |
| Every build recompiles all 1092 files | The dependency parser split `C:/...` at the drive colon. | Split on `": "`; objects are skipped when newer than source and all `-MMD` dependencies, with identical flags. |
| SDK/compat edits have no effect | MSYS2 Python's `symlink_to` silently **copies** directories, so `build/hgss-sdk` held stale copies. | On `wip/savestore`: junctions via `cmd.exe //c mklink //J` (MSYS rewrites a bare `/c`). See the open regression in [status](status.md). |
| `gen_fx_consts` fails (missing libc headers / `Read error`) | MSYS gcc lacks libc headers; the tool reads its CSV in text mode. | Bootstrap generates the header in Python (`prepare_game_headers.py`). |
| Partial Azahar screenshots | DPI virtualisation. | `run.ps1` calls `SetProcessDPIAware` and resizes the window. |

## Runtime

| Symptom | Cause | Fix |
| --- | --- | --- |
| Rows reversed inside every 8×8 tile | Wrong Morton row order: PICA tiles are stored top row first. | Fixed in `atlas.c`/`textures.c`; found by comparing with the reference renderer. |
| One-pixel horizontal gaps between tile rows | Nearest sampling near v = 1.0 in float24 rounded into the neighbouring (often transparent) atlas tile. | Texcoords in 1/16 texel, inset by 1/16 per edge. |
| Music drifts ahead of the picture | A frame overrunning a V-blank lost that V-blank for good. | Emulated V-blanks locked to the real V-blank counter; late frames catch up. |
| Field at ~17 FPS in Azahar | Geometry decode and `GetMonData` crypto (PC sampling). | Decoder fast paths; keystream cache (game patch 2002). |
| White map, 1% speed with `KJ_DL_CACHE` | The display-list recorder's counters sat after its arrays and escaped the `memset`; recordings overran and corrupted the heap (100% of samples in `_malloc_r`). | Fixed on `wip/savestore` (explicit reset, bump arena); `KJ_DL_VERIFY` compares replay with live decode. |
| Saved game ignored (new-game intro) | Open, `wip/savestore` only: appeared with the SDK shim junction change; not caused by patch 2003. | Investigate what differs between the old copied shim and the live sources. |
| A test run diverges from its script | Live keyboard input on the Azahar window was OR-ed with the script. | `wip/savestore`: scripted runs ignore live input. |
