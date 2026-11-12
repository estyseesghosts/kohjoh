# kohjoh documentation

kohjoh is a 3DS port of Pokémon HeartGold/SoulSilver: the original game code
on a new GPU-native runtime. Current state: the game boots and plays its intro
and Goldenrod field in Azahar; Old 3DS speed is not yet established.

| Page | Contents |
| --- | --- |
| [Status](status.md) | What works, at what evidence level; open work |
| [Architecture](architecture.md) | Frame flow, 2D and 3D renderers, the bridge |
| [Build on Windows](building-windows.md) | Bootstrap, build, data pack, run |
| [Testing and profiling](testing.md) | Azahar harness, scripts, oracle, PC sampler |
| [Troubleshooting](troubleshooting.md) | Build-chain and runtime failures with causes |
| [Lessons](lessons.md) | What went right, what went wrong, working rules |
| [Comparison](comparison.md) | What came from hgss-3ds / pokeplatinum-3ds |
| [Glossary](glossary.md) | Terms |
| [Build on Linux](building-linux.md) | Not yet verified |
| [Worklog](worklog/) | Dated engineering notes |

Design references: [`kohjoh.md`](../kohjoh.md) (brief) and
[`engineering/hgss-remaster/`](engineering/hgss-remaster/README.md)
(proposed designs, not current behaviour). Hardware background: the pinned
[Coffeepot wiki](https://github.com/estyseesghosts/coffeepot/tree/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr).
Emulator results are never O3DS results.
