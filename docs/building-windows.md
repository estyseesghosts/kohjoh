# Build on Windows

Verified 2026-11-12 on Windows 11 with a **from-scratch** bootstrap and build
(empty `build/upstream`, empty `build/kj`): 91 patches applied, 0 errors,
`.3dsx` packaged in about 6 minutes, and the title screen checked in Azahar.
Output: a HeartGold `.3dsx`. SoulSilver and CIA are not built yet.

## Requirements

| Tool | Where | Notes |
| --- | --- | --- |
| devkitPro | `C:\devkitPro` | Packages: devkitARM, libctru, citro3d, 3dstools, picasso. Versions are pinned in [`upstream.lock`](../upstream.lock). Its MSYS2 (`C:\devkitPro\msys2`) supplies `bash`, `make` and Python 3.14 (`/usr/bin/python`). |
| Git for Windows | `C:\Program Files\Git` | **Required for bootstrap.** devkitPro MSYS2's own `git` cannot fetch over HTTPS (see [pitfalls](#pitfalls)). |
| Azahar | `C:\Program Files\Azahar\azahar.exe` | Testing only (`tools/azahar/run.ps1`). |
| PowerShell 7 (`pwsh`) | | Runs the Azahar harness. |
| ROM | your own verified US dump | Only needed for the data pack. Never commit ROMs, packs, saves or extracted data. |

## Build

Run every build step **inside devkitPro's MSYS2 login shell**
(`C:\devkitPro\msys2\usr\bin\bash.exe -lc "..."`). The login profile
(`/etc/profile.d/devkit-env.sh`) sets `DEVKITPRO=/opt/devkitpro` and
`DEVKITARM=/opt/devkitpro/devkitARM`, which the build reads to find
`arm-none-eabi-gcc`. Use absolute `/c/...` paths throughout.

From Git Bash or PowerShell:

```sh
C:/devkitPro/msys2/usr/bin/bash.exe -lc '
  export PATH="/c/Program Files/Git/cmd:$PATH"    # Git for Windows first (HTTPS)
  K=/c/Users/<you>/workdir/kohjoh
  cd $K
  python tools/game/bootstrap.py                   # pinned decomp + every patch in patches/
  python tools/game/check_patches.py               # fails if build/upstream lacks a patch
  python tools/game/hgss_game.py --root $K/build/upstream --target 3ds --out $K/build/kj -j 32
' > build/kj-build.log 2>&1
```

**`--target 3ds` is required.** The default is `host32`, a PC build for tests.

A build succeeded only if the log ends with
`packaged .../pokeheartgold.3dsx` **and** has no `error:` lines. If the
vendored SDK/NitroSystem library fails to compile, the build prints
`warning: the vendored library build failed; linking with libnitrosystem.a,
libnitro.a ...` and **still packages a `.3dsx` from stale libraries**. Any
SDK or NitroSystem change is then silently absent. Check for it:

```sh
grep -E "error:|vendored library build failed" build/kj-build.log   # must print nothing
```

### Outputs (`--out build/kj`)

| Path | Contents |
| --- | --- |
| `pokeheartgold.3dsx` | the program |
| `pokeheartgold-3ds.elf` | symbols, linked at 0x00100000 (`arm-none-eabi-addr2line -e` for crash addresses) |
| `pokeheartgold.build-info.txt` | commit, dirty flag, sha256, the state of every legacy flag |
| `obj/`, `app/`, `ctr/`, `sdk/`, `gen/` | incremental objects and generated headers |
| `link.log`, `link-<utc>.log` | linker output; unresolved symbols are reported here |

The shared vendored-library trees are `build/hgss-sdk`, `build/hgss-nns`, and
`build/NitroSDK` / `build/NitroSystem` (links into `build/upstream/lib`).

### Timing and incremental builds

- Bootstrap ≈ 1 min (shallow fetch + `git am` of the patch stack).
- First build ≈ 5 min at `-j 32`. A rebuild after a kohjoh-side change takes
  about 2 min. Game objects are skipped when they are newer than their source
  and every `-MMD` dependency and their flags match; the Nitro libraries are
  re-made by `make`.
- Build each variant into its own `--out` (e.g. `--out $K/build/kjz
  --define KJ_G3_ZERO_TRACE`) so variants never mix objects.

## Build options

### `hgss_game.py` arguments

| Argument | Meaning |
| --- | --- |
| `--target 3ds` | 3DS build (default `host32`: PC test build) |
| `--game heartgold\|soulsilver` | edition (default heartgold; SoulSilver untested) |
| `--root PATH` | the patched decomp checkout (use `$K/build/upstream`) |
| `--out PATH` | output directory (required) |
| `-j N` | parallel jobs (default: CPU count) |
| `--define NAME` | add `-DNAME` to the runtime and game (repeatable) |
| `--keep-going` | compile everything, report per-file errors, then link |
| `--skip-prepare` | reuse the generated headers in `--root` |
| `--sanitize` | host32 only: ASan/UBSan |

### kohjoh switches (`--define NAME`)

| Define | Effect | Status |
| --- | --- | --- |
| `KJ_G3_PROFILE` | times each geometry opcode (`[g3prof]`); each probe costs ~390 ticks | diagnostic |
| `KJ_G3_ZERO_TRACE` | logs the first geometry commands that leave a zero position/projection matrix (`[g3zero]`) | diagnostic |
| `KJ_DL_NO_CACHE` | turns off the display-list cache (on by default: replays recorded vertex blocks; state commands still run) | A/B and bisects |
| `KJ_DL_VERIFY` | with the cache on: decode hits live and compare with the recording (`[dlverify] MISMATCH`), plus `[dlwhy]`/`[dlcov]` coverage lines | diagnostic |

Always pass these as `--define`. An inline variable such as `FOO=1 python ...`
works inside the `bash.exe -lc` shell but **is dropped when MSYS2 Python is
started directly from Git Bash**. `--define` works either way.

### Legacy environment variables (from hgss-3ds)

`hgss_game.py` and `src/compat/nitro/Makefile` still read these hgss-3ds
variables. Set them inside the `-lc` shell, for example `HGSS_PROFILE=on
python tools/game/hgss_game.py ...`. Each one is recorded in
`build-info.txt` and appended to the build tag. Leave them unset for normal
builds.

| Variable | Effect in kohjoh today |
| --- | --- |
| `HGSS_PROFILE=on` | enables frame, GPU, FIFO, H-blank, timer and sound profiling; leave unset for a baseline. Use a separate output directory for each variant. |
| `HGSS_SND_ADPCM=1` | the alternative sample-saturation path in `port_sound_chip.c` |
| `HGSS_A0_DIAG=1` | H-blank scan cross-check counters in `port_os.c` |
| `HGSS_BENCH=1` | hgss-3ds benchmark chainload define; no kohjoh source uses it |
| `HGSS_GX3D_*`, `HGSS_TEXTURE_*`, `HGSS_PALETTE_HASH` | set a define that no kohjoh source reads; no effect |
| `HGSS_HOST_CC` | host32 compiler (default `gcc-10`) |
| `DEVKITPRO`, `DEVKITARM` | toolchain location; set by the MSYS2 login profile |

## Changing the decomp, NitroSDK or NitroSystem

`build/upstream` is generated: the pinned decomp plus `patches/` applied as
commits. A change to it only counts once it is a patch in `patches/`:

1. Edit inside `build/upstream`, keeping the DS code in `#else` under
   `#ifdef SDK_PORT_3DS`. Build and verify.
2. Commit in `build/upstream` (identity `terezi <jules@foxtails.me>`).
3. Export it to the stack that matches the files:
   ```sh
   cd build/upstream
   git format-patch -1 --relative=lib/NitroSystem --start-number 305 -o ../../patches/NitroSystem  # NitroSystem
   git format-patch -1 --relative=lib/NitroSDK    --start-number 606 -o ../../patches/NitroSDK     # NitroSDK
   git format-patch -1                            --start-number 2004 -o ../../patches/pokeheartgold # game
   ```
   Number new patches after the highest one in that directory on every branch.
   `wip/savestore` holds `NitroSDK/2003`.
4. Commit the patch file in kohjoh.

Bootstrap refuses an existing `build/upstream` if it has local changes, or if
its applied-patch count differs from `patches/` (for example after switching
branches). Then run `python tools/game/bootstrap.py --clean`, which deletes and
re-creates it. Export your commits first; `--clean` discards them.
`check_patches.py` compares commit subjects, so it catches a tree that is
missing a patch. It does not catch a tree with **extra** commits from another
branch: switch branches → `bootstrap.py --clean`.

## Data pack

Not yet run from this repository (tested via hgss-3ds's identical builder):

```sh
python tools/rompack/verify_rom.py HeartGold.nds
python tools/rompack/extract_rom.py HeartGold.nds -o build/private/hg
python tools/rompack/pack.py build/private/hg -o build/private/hg.pak --rom HeartGold.nds
```

The runtime needs the `.pak` and the extractor's `manifest.json`, installed as
`sdmc:/3ds/kohjoh/data/heartgold.pak` and `heartgold.manifest.json`. For
SoulSilver add `--build-id soulsilver-nitrofs-1` to `pack.py`.

## Run in Azahar

```powershell
pwsh tools/azahar/run.ps1 -Pack build\private\hg.pak -Manifest build\private\hg\manifest.json -FreshSave -Shots 10,30 -Tag boot
```

This stages the build to Azahar's SD (`%APPDATA%\Azahar\sdmc\3ds\kohjoh\`),
launches it, and saves screenshots and logs to `build/azahar/<tag>/`. Pass
`-Holds` or `-Shots`: without them the harness closes Azahar after about 2 s.
See [testing](testing.md) for scripts, held frames, dumps and profiling.

## Pitfalls

| Symptom | Cause | Fix |
| --- | --- | --- |
| `git-remote-https.exe: error while loading shared libraries` during bootstrap | devkitPro MSYS2's `git` has a broken HTTPS helper. | Put `/c/Program Files/Git/cmd` first on `PATH` inside the `-lc` shell. |
| `.3dsx` built but an SDK/NNS change has no effect | vendored library compile failed; link used the stale prebuilt libraries | grep the log for `error:` / `vendored library build failed` |
| `mv`/`rm` of `build/upstream`: "Device or resource busy" | a shell's working directory is inside it | `cd` out first |
| `build-info.txt` says `dirty: yes` with no source changes | untracked files in the repository count as dirty | harmless; commit or ignore them |

More, with causes, in [troubleshooting](troubleshooting.md#windows-build-chain).
The build tools already handle each case listed there.
