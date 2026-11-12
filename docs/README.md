# kohjoh documentation

kohjoh is an in-progress Nintendo 3DS port of Pokémon HeartGold and SoulSilver.
It builds the game's decompiled source and connects it to a new 3DS runtime and
GPU renderer. The runtime and renderer are implemented; full-game compatibility,
physical-console performance and release packaging are still works in
progress. Start with [How kohjoh works](architecture.md), then use the build
guides and [project status](status.md) for current details.

## Guides

- [How kohjoh works](architecture.md) — how the game, runtime and renderer fit together.
- [Build on Windows](building-windows.md) — the workflow recorded by the project.
- [Build on Linux](building-linux.md) — a stub awaiting a tested Linux run.
- [Project status](status.md) — completed work, evidence and open work.
- [Port comparison](comparison.md) — what kohjoh takes from HGSS and Platinum ports, and what it replaces.
- [Troubleshooting](troubleshooting.md) — observed build and renderer failures and their fixes.
- [Glossary](glossary.md) — short explanations of project and 3DS terms.

## Engineering references

[`engineering/hgss-remaster/`](engineering/hgss-remaster/README.md) contains a
larger technical design set for the planned remaster. Those articles define
proposed interfaces, coverage and hardware acceptance criteria; their presence
does not mean each feature is implemented. Check the source links and
[project status](status.md) before treating a design requirement as current
behavior.

## How to read claims

Links to source code support statements about what the program currently does.
[`kohjoh.md`](../kohjoh.md) describes intended scope; it is not proof that every
planned feature exists. The [chatlog](chatlog.md) records development history,
including experiments and intermediate failures. [Project status](status.md)
separates those from the committed, checked-in state. In particular, Azahar
results are emulator evidence, not measurements of a physical 3DS.

For hardware background, this documentation links to the pinned Coffeepot
wiki, including its [porting guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/porting.md),
[PICA200 renderer guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/renderer-architecture.md)
and [frame-pacing guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/frame-pacing.md).
Imported code and licenses are listed in [`PROVENANCE.md`](../PROVENANCE.md);
exact source revisions and tool versions are in [`upstream.lock`](../upstream.lock).
