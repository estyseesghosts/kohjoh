# kohjoh documentation

kohjoh is an in-progress native Nintendo 3DS port of the decompilation of
Pokémon HeartGold and SoulSilver. Its central technical choice is to keep the
game's simulation and compile layer while replacing the old port's rendering
and platform runtime with a GPU-oriented 3DS runtime. See the [design brief](../kohjoh.md),
[architecture](architecture.md), and [status](status.md) for the current scope
and evidence.

## Articles

- [How kohjoh works](architecture.md): build boundary, frame flow, memory and render paths.
- [Build on Windows](building-windows.md): current supported developer workflow.
- [Build on Linux](building-linux.md): article stub; no Linux recipe has been validated.
- [Project progress](status.md): completed milestones, verified scenes and open work.
- [Differences from hgss-3ds and platinum-3ds](comparison.md): what was retained and rewritten.
- [Troubleshooting and lessons](troubleshooting.md): observed failures and their fixes.

## Evidence and status language

These articles separate repository facts from plans and experiments. “Verified”
means the chatlog or checked-in source records a successful result; emulator
results are Azahar-only and say nothing conclusive about Old 3DS speed. A design
brief is not proof of an implemented feature. See the [Coffeepot porting
guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/porting.md)
and [frame-pacing guide](https://github.com/estyseesghosts/coffeepot/blob/35b59f178e638506dcdfdf649651e20abf9fa8d4/ctr/engineering/PICA200/frame-pacing.md)
for the hardware concepts behind the design. The source and patch lineage is
recorded in [PROVENANCE.md](../PROVENANCE.md); work history is in
[chatlog.md](chatlog.md).
