# Troubleshooting and lessons from bring-up

This page covers failures recorded while bringing up the Windows/MSYS2 build
and renderer. It is a practical summary, not a list of universal Windows
rules. The [chatlog](chatlog.md) has the sequence; links below point to the
current workaround where one exists.

## Build failures

| Symptom | What happened | Fix or next step |
| --- | --- | --- |
| Compiler says it cannot create a temporary file. | In the recorded run, `TMP` pointed to `C:\Windows`, which was not writable by the compiler process. | Point `TMP` and `TEMP` at a writable temporary directory. The MSYS build helpers export usable values to child processes when they are unset ([`common.mk`](../src/compat/common.mk), [`hgss_nitro.py`](../tools/game/hgss_nitro.py)). |
| Host header generator fails because system headers are missing. | Bootstrap tried to compile a host utility only to generate game headers; the MSYS compiler did not have the needed libc headers. | The project generates those headers with Python instead ([`prepare_game_headers.py`](../tools/game/prepare_game_headers.py), [`bootstrap.py`](../tools/game/bootstrap.py)). |
| SDK directory link is stale or is actually a copy. | The observed MSYS Python setup copied directories where a symbolic link had been expected. | Bootstrap checks whether the link reaches the real source and safely refreshes a stale copy ([`bootstrap.py`](../tools/game/bootstrap.py), `link_sdks`). |
| Archive tool reports “argument list too long.” | The scratch/build path was too deep for that tool invocation. | Move the checkout/build to a shorter path and rerun. The log records this workaround, not a universal Windows path limit. |
| A script looks under `/home/...` and cannot find Windows files. | MSYS Python resolved its working directory differently from the Windows-mounted script path. | Use absolute paths in the form expected by the running tool. The build driver contains conversions between MSYS and Windows paths ([`hgss_game.py`](../tools/game/hgss_game.py)). |
| Linker says compiled object files do not exist. | Native `ld` read `/c/...` paths from a response file, but MSYS path conversion had only applied to command-line arguments. | Use Windows-style paths such as `C:/...` inside files read by native tools. The driver has separate path handling for these cases ([`hgss_game.py`](../tools/game/hgss_game.py)). |
| Incremental build recompiles everything. | Generated headers changed timestamps on each run, and a dependency parser initially split a path at the drive-letter colon. | Keep generated outputs stable, parse the dependency separator after the target, and check source/object times plus build flags and upstream revision. See the incremental compile logic in [`hgss_game.py`](../tools/game/hgss_game.py). |
| Link fails on `NitroPort_EngineAOnTop`. | Removing the old 2D renderer also removed its definition of this small SDK hook. | Implement the hook in the new frame bridge by reading the DS display-control bit in `POWCNT1` ([`kj_frame.c`](../src/runtime/kj_frame.c)). |
| App code no longer sees a standard integer type. | A deleted renderer header had been pulling in `<stdint.h>` indirectly. | Include required standard headers directly in the code that uses them; do not rely on a removed renderer's incidental includes. |
| Azahar screenshots show only part of the emulator window. | Windows DPI virtualization made the captured window dimensions differ from the requested dimensions. | The runner now marks itself DPI-aware and resizes the emulator window before capture ([`run.ps1`](../tools/azahar/run.ps1)). |

If the failure differs from these cases, inspect the first compiler/linker error
and the per-file logs under the chosen build output. Do not infer the cause
from the last summary line alone.

## Renderer investigation: display-list cache

An experimental cache records DS 3D command blocks and reuses decoded results.
In one Azahar run with the cache enabled, the map became white while sprites
remained visible, and the run slowed dramatically. The sprites used the
uncached path, so the cache was a strong suspect, but the exact texture-state
bug was not proven. A later verification run stopped progressing before the
requested frame. Profiling showed the main thread waiting for VBlank amid heavy
activity; it did not establish a deadlock or a final root cause. See the cache
code in [`kj_g3.c`](../src/runtime/kj_g3.c) and the chronology in
[`chatlog.md`](chatlog.md).

The safe recorded result is that turning the cache off restored the intro and
Goldenrod scene; that state was committed as `40c6984`. The cache remains
behind `HGSS_GX3D_LISTCACHE=1` and is not a verified optimization. Keep it off
for ordinary builds while that work is unresolved. See [status](status.md).

## Graphics defects caught during implementation

The worklog records fixes for a 2D shader colour that was not normalized, a
window pass that left colour writes disabled, missing helper functions, and a
storage layout that could split palette data needed contiguously. A tile-edge
inset was later added for visible horizontal gaps. Those details are tied to
the graphics code and change history, not claims that every visual defect is
fixed. Relevant sources are [`g2d.c`](../src/graphics/g2d/g2d.c),
[`atlas.c`](../src/graphics/resources/atlas.c) and
[`kj_g3.c`](../src/runtime/kj_g3.c).

The first intro capture also showed horizontal bands. Later work added the
tile-edge inset and committed it with the verified cache-off renderer state;
the initial screenshot alone did not establish whether the cause was a
transition effect or span handling. Compare the same held frame before
attributing similar lines to one subsystem.

When diagnosing a future graphics issue, reproduce the same scene and compare
one change at a time. Keep a known-good binary, log the active build options,
and check that the optimization actually ran. Coffeepot's [debugging guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/debugging.md)
and [PICA command-pipeline notes](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/pica200-command-pipeline.md)
give useful background; the kohjoh-specific findings above are cited to its
own sources and worklog.
