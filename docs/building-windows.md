# Build on Windows

This guide describes the Windows workflow recorded for kohjoh. The project
uses Windows with MSYS2 tooling for parts of the build. Build steps and outputs
may change; the scripts are the final authority for available options.

The verified target is a Homebrew Launcher `.3dsx`. The build can also leave an
ELF executable for debugging. A CIA build is not implemented. See the
[glossary](glossary.md) for `.3dsx`, ELF and CIA.

## You will need

- Git and Python 3.
- The devkitPro tools and libraries listed in [`upstream.lock`](../upstream.lock):
  devkitARM, libctru, citro3d, citro2d, 3dstools and picasso, at the pinned
  versions.
- A Windows environment where those tools are available to the build scripts.
  Some development steps in the worklog used MSYS2 Python and MSYS paths such
  as `/c/...`; the Azahar runner uses PowerShell.
- A compatible game source image only for your own local content-pack workflow.
  The image, extracted assets, pack and save are private data and must not be
  committed.

`upstream.lock` records the source commits and tool versions. The bootstrap
script fetches the pinned decompilation and applies the project patches; see
[`bootstrap.py`](../tools/game/bootstrap.py) and
[`PROVENANCE.md`](../PROVENANCE.md). You do not need to fetch hgss-3ds,
pokeplatinum-3ds or Coffeepot separately to build the pinned project sources.

## Bootstrap and compile

Run these commands from the repository root in the Python environment used by
the Windows build:

```powershell
python tools/game/bootstrap.py
python tools/game/hgss_game.py --target 3ds --out build/kj -j 8
```

The first command prepares `build/upstream` and generated headers. The second
compiles HeartGold, the compatibility libraries and the 3DS runtime, then
links the program. If `3dsxtool` is available, the build driver also packages
the `.3dsx` beside the linked output. Logs and generated files are under
`build/kj`. See [`hgss_game.py`](../tools/game/hgss_game.py) for exact
arguments, output names and failure codes.

Use a separate output directory for SoulSilver; do not share object files
between editions:

```powershell
python tools/game/hgss_game.py --target 3ds --game soulsilver --out build/ss -j 8
```

The `-j` value sets parallel compile jobs. Choose a value your machine can
handle. The driver treats the ELF link as a successful build even if
`3dsxtool` is missing; in that case it reports that the program was left
unpackaged. To launch through Homebrew Launcher, confirm that the `.3dsx` was
produced beside the ELF.

## Prepare your local game-data pack

This is separate from compiling the executable. It reads your own verified US
HeartGold or SoulSilver image and writes private extracted data under the
output folder. The repository's ROM tools verify the supported image, extract
its files, then pack them into the format the game expects
([`verify_rom.py`](../tools/rompack/verify_rom.py),
[`extract_rom.py`](../tools/rompack/extract_rom.py),
[`pack.py`](../tools/rompack/pack.py)). For example:

```powershell
$rom = 'C:\private\HeartGold.nds'
python tools/rompack/verify_rom.py $rom
python tools/rompack/extract_rom.py $rom -o build\private\heartgold
python tools/rompack/pack.py build\private\heartgold `
  -o build\private\heartgold.pak --rom $rom
```

Keep the extracted directory and pack out of source control. The extractor
writes a `manifest.json` sidecar into the extracted directory; the runtime
needs both that manifest and the `.pak`. For the Azahar harness, pass both:

```powershell
pwsh tools/azahar/run.ps1 -Pack build\private\heartgold.pak -Manifest build\private\heartgold\manifest.json -Tag heartgold
```

The harness stages them under the HeartGold data names. The in-app save is a
separate raw SD file. This guide does not describe migrating a retail save or
installing a CIA.

For SoulSilver, extract into a separate directory and pass
`--build-id soulsilver-nitrofs-1` to `pack.py`; the runtime checks the edition's
pack ID ([`kj_boot.c`](../src/runtime/kj_boot.c)). Keep HeartGold and
SoulSilver packs, manifests, saves and build output separate.

## Run in Azahar

With Azahar installed at `C:\Program Files\Azahar\azahar.exe`, run from the
repository root:

```powershell
pwsh tools/azahar/run.ps1 -Shots 8,15,25 -Tag boot
```

This harness copies the selected `.3dsx` to Azahar's virtual SD card, starts
the emulator, captures window screenshots at the requested times, and copies
the program and emulator logs into `build/azahar/boot`. Times in `-Shots` are
seconds after launch. Useful optional arguments include `-Edition`,
`-Artifact`, `-Pack`, `-Manifest`, `-Save`, `-FreshSave`, `-Script`, `-Holds`
and `-OutDir`; see the parameter block in [`run.ps1`](../tools/azahar/run.ps1).
Use `-FreshSave` only when you intend to discard that test save.

Azahar is an integration and visual-debugging tool here, not a timing
authority. A smooth emulator run does not demonstrate Old 3DS performance;
that requires the physical-hardware testing described by Coffeepot's
[hardware test guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/testing-on-hardware.md).

## Windows and MSYS2 notes

The bring-up exposed several path and environment differences between MSYS2
programs and native Windows tools. Keep the workspace path short if archiving
or linking reports a path-length or argument-size failure. Use absolute paths
when the Python process resolves its working directory differently from the
shell. Native tools read Windows paths from response files, while MSYS tools
expect MSYS paths. The scripts contain path handling for these cases; details
and observed fixes are in [Troubleshooting](troubleshooting.md).

If a compiler reports it cannot create a temporary file, check that `TMP` and
`TEMP` point to a writable directory. The build helpers set them for child
processes when needed ([`common.mk`](../src/compat/common.mk),
[`hgss_nitro.py`](../tools/game/hgss_nitro.py)).
