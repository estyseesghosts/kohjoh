# Build on Windows

This is the recorded Windows/MSYS2-oriented workflow. The verified first
packaged artifact was `build/kj/pokeheartgold.3dsx`; the Azahar harness stages
that artifact on the emulator's virtual SD. The build scripts can also emit an
ELF before packaging. No CIA build procedure is implemented here.

## Prerequisites

- Git and Python 3 on `PATH`.
- The pinned devkitPro toolchain and 3DS tools described by
  [`upstream.lock`](../upstream.lock): devkitARM, libctru, citro3d, citro2d,
  3dstools and picasso.
- Enough free disk for the bootstrapped decomp checkout and build outputs.
- An original compatible game image only for your local asset/data workflow;
  do not put ROMs, extracted data, packs or saves in source control.

The actual toolchain versions and upstream commit pins live in
[`upstream.lock`](../upstream.lock). The setup uses a pinned checkout and
applies the repository patch stack; see [`bootstrap.py`](../tools/game/bootstrap.py)
and [`PROVENANCE.md`](../PROVENANCE.md).

## Bootstrap and build

From PowerShell at the repository root:

```powershell
python tools/game/bootstrap.py
python tools/game/hgss_game.py --target 3ds --out build/kj -j 8
```

Use a job count appropriate to your machine. `--game soulsilver` selects
SoulSilver and needs its own output directory, for example:

```powershell
python tools/game/hgss_game.py --target 3ds --game soulsilver --out build/ss -j 8
```

The build driver prepares generated game headers, checks the patch stack,
compiles the game and runtime, links, and packages a `.3dsx` when its pinned
`3dsxtool` is available. It writes logs and generated files under the output
directory and the bootstrap checkout under `build/upstream`. Options are
defined in [`hgss_game.py`](../tools/game/hgss_game.py); patch application and
header generation are in [`bootstrap.py`](../tools/game/bootstrap.py).

On Windows, use a short workspace path if the archive/link stages report a
command-line-too-long or response-file path issue. The scripts include path
normalization for native Windows tools called from MSYS, but short paths avoid
some Windows tool limits. Do not set `TMP` or `TEMP` to a protected Windows
system directory; the build helpers set temporary paths for compiler children
([`common.mk`](../src/compat/common.mk), [`bootstrap.py`](../tools/game/bootstrap.py)).

## Run in Azahar

With Azahar installed at the path expected by the harness, run:

```powershell
pwsh tools/azahar/run.ps1 -Shots 8,15,25 -Tag boot
```

The harness accepts optional pack, manifest, save and input-script paths. It
copies the selected artifact to Azahar's virtual SD, captures screenshots and
copies the title and emulator logs under `build/azahar/<tag>`.
[`run.ps1`](../tools/azahar/run.ps1) explicitly labels these as emulator-only
evidence. Emulator screenshots or frame cadence do not establish Old 3DS
performance; see Coffeepot's [hardware test guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/testing-on-hardware.md).

## Common Windows-specific pitfalls

See [Troubleshooting](troubleshooting.md) for the failures actually observed:
compiler temporary paths, MSYS path conversion, generated dependency files,
response-file paths, Python path roots, SDK link emulation and long archive
paths. These are workarounds learned from this project, not universal claims
about every Windows setup.
