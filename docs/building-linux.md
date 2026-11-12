# Build on Linux

> **Article stub — not yet validated.** The current build history documents a
> Windows/MSYS2 development environment. No Linux build has been recorded, so
> commands, package names and expected output below are intentionally not
> guessed.

## To document

- Supported distribution and versions of Python, Git, devkitARM, libctru,
  citro3d, citro2d, picasso and 3dstools.
- Clean bootstrap and HeartGold/SoulSilver build commands.
- Required environment variables and path assumptions.
- Whether the Windows-oriented build driver and Azahar harness need a Linux
  implementation or changes.
- A clean-build result, incremental-build result and artifact names.
- A reproducible Azahar smoke run and its logs.

Record the exact commands and tool versions in the article after running them.
The current pins are in [`upstream.lock`](../upstream.lock), while Windows
workarounds are documented in [building-windows.md](building-windows.md) and
[troubleshooting.md](troubleshooting.md). For hardware-independent portability
considerations, see Coffeepot's [development workflow](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/homebrew/development-workflow.md)
and [porting guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/porting.md).
