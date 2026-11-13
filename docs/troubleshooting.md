# Troubleshooting

Each entry: symptom → cause → fix. The tools already contain the fixes; this
page explains them for the next breakage.

## Windows build chain

| Symptom | Cause | Fix |
| --- | --- | --- |
| `Cannot create temporary file in C:\Windows\` | MSYS2 drops `TMP`/`TEMP` from child processes; native GCC falls back to `C:\Windows`. | `common.mk` exports them; `hgss_nitro.py` sets them in Python. |
| `-DFOO` set via `FOO=1 python ...` has no effect | MSYS2 Python started directly from Git Bash does not see inline environment variables (inside `bash.exe -lc` it does). | `hgss_game.py --define FOO`. |
| Bootstrap: `git-remote-https.exe: error while loading shared libraries` | devkitPro MSYS2's `git` has a broken HTTPS helper. | `export PATH="/c/Program Files/Git/cmd:$PATH"` inside the `-lc` shell ([build](building-windows.md#build)). |
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
| Title lower screen black (no Ho-Oh) | The title uses `GX_BUFFERMODE_W` with near = 0, so every vertex has z/w = 1 and failed a Z-mode GREATER test. | W-buffer path in `g3d.c`: depth = w/4096 (PICA W-buffer mode), LESS, depth cleared to 0xFFFFFF. |
| Chunks of a skinned model missing (title Ho-Oh's wings) | Direct stores to G3 command registers (`reg_G3_MTX_STORE = ...`) land in plain memory on the port. NNS NODEMIX's envelope matrices were never stored, so weighted vertices restored zero matrices and collapsed to clip (0,0,0,0). | Route every direct G3 command-register write through `NitroPort_GxDirectWrite` (NitroSystem 0305, NitroSDK 0606, game 2004). Never "fix" this by changing w or dropping/redrawing w <= 0 triangles: that is what produced the old port's stretched Ho-Oh. |
| Vendored SDK change has no effect | The vendored library build failed and the link fell back to the prebuilt `libnitro*.a` (build log: "vendored library build failed"). | Grep the build log for `error:` before testing any SDK/NitroSystem change. |
| O3DS: town "unplayably slow" though the game ticks | Uncapped present skip: while ≥2 refreshes behind, every picture was skipped; a game slightly over budget showed ~4 pictures a second (o3ds-log-0: 1606 of 2206 skipped). | Never skip two in a row (`renderer.c`, `0089b28`). |
| O3DS: title stays white (normal build), correct but slow (probe build) | Probably the same skip starvation: the last picture before falling behind was the title's white flash. Not reproduced in Azahar (Azahar runs the title within budget). | Skip cap `0089b28`; needs an O3DS check. |
| `run.ps1`: "azahar exited before tNNN" | Another `run.ps1` still running killed Azahar when it finished. | One harness at a time; stop the old one first. |
| `run.ps1` run ends after ~2 s | Without `-Holds` or `-Shots` the harness only waits `-Extra` seconds. | Always pass `-Holds` or `-Shots`. |
| A C source turns "binary" in git, or a string has a stray tab/CR | Python edits passed through an unquoted bash heredoc turned `\0`, `\t`, `\r`, `\n` escapes into real control bytes. | Use the Edit tool, or write the script to a file with raw strings; scan for NUL/CR after scripted edits. |
