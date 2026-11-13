# kohjoh

a native homebrew port of pokemon heartgold and soulsilver for the original 3DS.
this is more of a proof-of-concept than a playable game. expect issues.

runs nearly full speed on real old-3DS hardware, bar the intro ho-oh/lugia & busier game areas 

this repository does not contain any unlicensed game data. 
for heartgold, you need a file with sha-1 starting with 4fcded0e2713dc03929845de...
for soulsilver, f8dc38ea20c17541a43b58c5...

- Build: [docs/building-windows.md](docs/building-windows.md)
- How it works: [docs/architecture.md](docs/architecture.md)
- Testing and profiling: [docs/testing.md](docs/testing.md)
- Lessons and pitfalls: [docs/lessons.md](docs/lessons.md), [docs/troubleshooting.md](docs/troubleshooting.md)
- Design brief: [kohjoh.md](kohjoh.md); imports and licences: [PROVENANCE.md](PROVENANCE.md)

GPL-3.0 (see [LICENSE](LICENSE)).

## Installation

1. Download the matching `kohjoh-gold.3dsx` or `kohjoh-silver.3dsx` into `/3ds/` on your SD card.
2. Prepare your game data from your own unmodified USA dump (needs Python 3.10+): on Windows, drag the `.nds` onto `tools/rompack/prepare.bat`; elsewhere run `python tools/rompack/prepare.py your-game.nds --sd /path/to/sd`. Without `--sd` it writes `kohjoh-data/`; copy its files into `/3ds/kohjoh/data/`. Takes a few seconds; the ROM is only read.

The optional CIA is a Luma3DS HOME Menu shortcut; it launches the 3DSX from that exact path and keeps saves on SD. The forwarder requires Luma3DS and has not yet been tested on hardware.
