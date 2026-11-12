# Build on Windows

Tested on Windows 11 with devkitPro (MSYS2 under `C:\devkitPro\msys2`) and the
tool versions in [`upstream.lock`](../upstream.lock). Output: a HeartGold
`.3dsx`. SoulSilver and CIA are not built yet.

## Requirements

- devkitPro with the 3DS packages (devkitARM, libctru, citro3d, 3dstools,
  picasso), Git, and the devkitPro MSYS2 Python (`/c/devkitPro/msys2/usr/bin/python`).
- Azahar for testing (`C:\Program Files\Azahar\azahar.exe`).
- Your own verified US HeartGold/SoulSilver ROM for the data pack. Never commit
  ROMs, packs, saves or extracted data.

## Build

Run from a Git Bash/MSYS shell. **Use absolute `/c/...` paths**: MSYS Python
resolves its working directory to `/home/<user>/...`, which native tools and
`mkdir -p` cannot use.

```sh
K=/c/Users/<you>/workdir/kohjoh
python $K/tools/game/bootstrap.py                    # pinned decomp + patches
python $K/tools/game/hgss_game.py --root $K/build/upstream \
    --target 3ds --out $K/build/kj -j 32             # -> build/kj/pokeheartgold.3dsx
```

- First build ~3.5 min; rebuilds ~2 min (game objects are incremental via
  `-MMD` and flag signatures; the Nitro libraries are relinked by make).
- Diagnostic switches are `--define NAME` (e.g. `KJ_G3_PROFILE`,
  `KJ_DL_VERIFY`). Environment variables set inline do **not** reach the
  build: MSYS2 Python drops them.
- Build a variant into its own `--out` directory.

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

Stages the build to Azahar's SD (`%APPDATA%\Azahar\sdmc\3ds\kohjoh\`),
launches it, saves screenshots and logs to `build/azahar/<tag>/`. See
[testing](testing.md) for scripts, held frames, dumps and profiling.

## Windows/MSYS2 pitfalls

See [troubleshooting](troubleshooting.md#windows-build-chain); the build tools
already handle each case.
