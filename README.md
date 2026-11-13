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

Download the matching `kohjoh-gold.3dsx` or `kohjoh-silver.3dsx` into `/3ds/` on your SD card. Run `python prepare.py your-game.nds` from the asset tools bundle and copy the resulting files into `/3ds/kohjoh/data/`. The tool supports unmodified USA dumps of the corresponding retail game. The optional CIA is a Luma3DS HOME Menu shortcut; it launches the 3DSX from that exact path and keeps saves on SD. The forwarder requires Luma3DS and has not yet been tested on hardware.
