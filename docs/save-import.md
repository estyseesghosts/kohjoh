# Importing emulator saves

Place the save at `sdmc:/3ds/kohjoh/saves/heartgold.sav` or
`sdmc:/3ds/kohjoh/saves/soulsilver.sav`, matching the edition. Raw 512 KiB
cartridge images load directly. A DeSmuME `.dsv` with a 512 KiB payload and a
version-zero footer, or a No$GBA `.SAV` with a 512 KiB uncompressed or packed
payload, is recognized by its content and converted on first load. Rename a
copy to the edition's `.sav` filename; alternate filenames are not searched.

A packed No$GBA file can itself be exactly 512 KiB. Kohjoh treats every file of
that length as a raw image, so export such a file to raw before use. Interpreting
matching bytes in a raw image as a container would change existing raw-save
behavior.

The importer checks the container header and sizes and, for packed No$GBA,
every run and the terminator. Unsupported, malformed or truncated inputs are
rejected rather than interpreted as blank saves. Before replacing the active
file, it writes and verifies a copy of the complete original as `<save>.orig`
(or `.orig.1`, `.orig.2`, etc. if occupied), then writes, flushes and verifies
a temporary raw image. Existing backups are not overwritten. Conversion happens
once; the resulting active file is raw and the original remains alongside it.

If storage is full, read-only, or an operation fails, conversion stops and the
log reports the failed stage and errno (zero if no system error was reported).
The original remains at the active path or in a verified
`.orig` backup. An incomplete backup or temporary file may also remain; do not
assume every backup is complete. If interruption leaves the active file missing
but a backup or temporary file exists, Kohjoh refuses to create a blank save.
Restore a known complete original to the edition's `.sav` path and retry. A
3DS filesystem rename is not guaranteed crash-safe; the backup is the recovery
copy, not a guarantee that replacement survives power loss.

DeSmuME's [`mc.cpp`](https://github.com/TASEmulators/desmume/blob/master/desmume/src/mc.cpp)
documents its footer and the No$GBA header and packed data fields. Coffeepot's
`ctr/engineering/filesystem-and-saves.md` describes 3DS flush and rename limits.
The previous Kohjoh importer was exercised with real exports; the revised
recovery path still needs edition-specific Continue, save and reload checks in
Lotus and on hardware.
