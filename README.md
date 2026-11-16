# kohjoh

a native homebrew port of pokemon heartgold and soulsilver for the original 3DS.
this is more of a proof-of-concept than a playable game. expect issues.

runs nearly full speed on real old-3DS hardware, bar the intro ho-oh/lugia & busier game areas 

this repository does not contain any unlicensed game data. 
for heartgold, you need a file with sha-1 starting with 4fcded0e2713dc03929845de...
for soulsilver, f8dc38ea20c17541a43b58c5...

- Build: [docs/building-windows.md](docs/building-windows.md)
- How it works: [docs/architecture.md](docs/architecture.md)
- Testing and profiling: [docs/testing.md](docs/testing.md), [docs/lotus.md](docs/lotus.md)
- Lessons and pitfalls: [docs/lessons.md](docs/lessons.md), [docs/troubleshooting.md](docs/troubleshooting.md)
- Design brief: [kohjoh.md](kohjoh.md); imports and licences: [PROVENANCE.md](PROVENANCE.md)
- Complain about this project: [on discord](https://discord.gg/wJNprjYCun)

GPL-3.0 (see [LICENSE](LICENSE)).

## Installation

You need a 3DS with custom firmware (Luma3DS), the Homebrew Launcher, and your own unmodified USA dump of the game (`.nds`). Python 3.10+ is needed on your PC for one step.

1. Download from the [latest release](https://github.com/estyseesghosts/kohjoh/releases/latest): `kohjoh-gold.3dsx` (HeartGold) and/or `kohjoh-silver.3dsx` (SoulSilver), and `kohjoh-asset-tools.zip`.
2. Copy the `.3dsx` into the `/3ds/` folder on your SD card, keeping its name: `/3ds/kohjoh-gold.3dsx`.
3. Unzip `kohjoh-asset-tools.zip`. On Windows, drag your `.nds` onto `prepare.bat`. Elsewhere run `python prepare.py your-game.nds`. It checks the dump, then writes a `kohjoh-data/` folder (a few seconds; the ROM is only read, and nothing is uploaded). Add `--sd E:\` (your SD card's drive or mount point) to write straight to `3ds/kohjoh/data/` on the card instead.
4. Copy the contents of `kohjoh-data/` (`heartgold.pak` and `heartgold.manifest.json`, or the `soulsilver` pair) into `/3ds/kohjoh/data/` on the SD card. Create the folders if they don't exist.
5. Launch kohjoh from the Homebrew Launcher.

Never share the `.pak` files: they contain your game's data. Saves are kept on the SD card.

The optional CIA is a Luma3DS HOME Menu shortcut; it launches the 3DSX from that exact path and keeps saves on SD. The forwarder requires Luma3DS and has not yet been tested on hardware.
