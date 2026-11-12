# Build on Linux

> **Not yet verified.** The recorded build work was done on Windows/MSYS2. No
> Linux build has been completed for this documentation, so this page does not
> give commands that merely look plausible.

The source pins are shared across platforms in [`upstream.lock`](../upstream.lock),
but that alone does not prove the scripts work on Linux. Before calling Linux
supported, record an actual clean build and the exact tool versions.

## What a complete guide needs

- A supported distribution and versions of Python, Git and the devkitPro tools.
- The commands to bootstrap the pinned source, compile HeartGold, and compile
  SoulSilver into its own output directory.
- Required environment variables, permissions and disk-space needs.
- Whether the build scripts need Linux-specific path or subprocess handling.
- Expected ELF and `.3dsx` artifact names, plus a successful Azahar smoke run.
- Clean-build and incremental-build results, including any failures and fixes.

The Windows workflow and its observed MSYS path issues are in
[building-windows.md](building-windows.md) and
[troubleshooting.md](troubleshooting.md). Coffeepot's [development workflow](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/homebrew/development-workflow.md)
and [porting guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/porting.md)
give general guidance, not a validated kohjoh Linux recipe.
