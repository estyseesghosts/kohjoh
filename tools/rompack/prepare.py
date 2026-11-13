#!/usr/bin/env python3
"""Turn your own retail USA HeartGold / SoulSilver dump into kohjoh's game data.

    python prepare.py                      # finds a .nds in this folder
    python prepare.py your-game.nds        # or name it (drag and drop works)
    python prepare.py your-game.nds --sd E:\\    # write straight to your SD card

The ROM is only read, never modified. Output: <edition>.pak and
<edition>.manifest.json, which belong in /3ds/kohjoh/data/ on the SD card.
Nothing is uploaded or sent anywhere; the files contain your game's data, so
do not share them.
"""
import argparse
import shutil
import struct
import sys
import tempfile
from pathlib import Path

import extract_rom
import pack
import verify_rom

SD_DATA_DIR = Path('3ds') / 'kohjoh' / 'data'
EDITIONS = {  # SHA-1 -> edition name; build IDs are what the game checks at boot
    'f8dc38ea20c17541a43b58c5e6d18c1732c7e582': 'soulsilver',
    '4fcded0e2713dc03929845de631d0932ea2b5a37': 'heartgold',
}


def find_roms():
    """.nds files next to this script or in the current folder."""
    seen = {}
    for folder in (Path.cwd(), Path(__file__).resolve().parent):
        for rom in sorted(folder.glob('*.nds')):
            seen[rom.resolve()] = rom
    return list(seen)


def check_pak(path, edition, digest):
    """Re-read the finished pack's header and make sure it matches what the game will ask for."""
    with path.open('rb') as f:
        magic, version, build_id, sha1, count = pack.HEADER.unpack(f.read(pack.HEADER.size))
    build_id = build_id.rstrip(b'\0').decode()
    if (magic != pack.MAGIC or version != pack.VERSION or sha1.hex() != digest
            or build_id != edition + '-nitrofs-1' or not count):
        raise ValueError(f'{path.name} did not come out right; please try again')


def prepare(rom, outdir):
    print(f'Checking {rom.name} ...')
    digest = verify_rom.sha1_of(rom)
    name = verify_rom.verify(rom, digest=digest)
    edition = EDITIONS[digest]
    print(f'  {name}: OK')
    outdir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='kohjoh-') as tmp:
        data = Path(tmp)
        print('Reading the game files ...')
        entries = extract_rom.extract(rom, data, verify=False)  # already verified above
        counts = extract_rom.summarize(entries)
        print(f'  {counts["files"]} files, {counts["bytes"] // (1 << 20)} MiB')
        print('Building the data pack ...')
        pak = outdir / (edition + '.pak')
        pack.pack(data, pak, build_id=edition + '-nitrofs-1', rom_sha1=digest,
                  supported=verify_rom.SUPPORTED)
        check_pak(pak, edition, digest)
        shutil.copyfile(data / 'manifest.json', outdir / (edition + '.manifest.json'))
    return edition


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        stream.reconfigure(errors='replace')  # legacy Windows consoles
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('rom', type=Path, nargs='*',
                        help='your unmodified USA .nds dump(s); default: any .nds in this folder')
    where = parser.add_mutually_exclusive_group()
    where.add_argument('--sd', type=Path, metavar='SD_ROOT',
                       help='SD card root (e.g. E:\\); writes to 3ds/kohjoh/data/ on it')
    where.add_argument('-o', '--output', type=Path, default=Path('kohjoh-data'),
                       help='output folder (default: kohjoh-data)')
    args = parser.parse_args(argv)

    roms = args.rom or find_roms()
    if not roms:
        print('No ROM given and no .nds file found in this folder.\n'
              'Pass your dump: python prepare.py your-game.nds\n')
        verify_rom.print_supported()
        return 2
    if args.sd is not None:
        if not args.sd.is_dir():
            print(f'SD card folder not found: {args.sd}')
            return 2
        outdir = args.sd / SD_DATA_DIR
    else:
        outdir = args.output

    done = []
    for rom in roms:
        try:
            if not rom.is_file():
                raise OSError(f'not a file: {rom}')
            done.append(prepare(rom, outdir))
        except verify_rom.RomError as exc:
            print(f'\n{rom.name}: not usable.\n  {exc}\n')
            verify_rom.print_supported()
        except (OSError, ValueError) as exc:
            print(f'\n{rom.name}: failed: {exc}')
    if not done:
        return 1
    print(f'\nDone: {", ".join(done)} -> {outdir.resolve()}')
    if args.sd is None:
        print('Copy those files into /3ds/kohjoh/data/ on your SD card '
              '(next to kohjoh-gold.3dsx / kohjoh-silver.3dsx in /3ds/).')
    return 0 if len(done) == len(roms) else 1


if __name__ == '__main__':
    sys.exit(main())
