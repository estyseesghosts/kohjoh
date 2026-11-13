#!/usr/bin/env python3
"""Prepare a retail USA HeartGold or SoulSilver dump for kohjoh."""
import argparse
import shutil
import tempfile
from pathlib import Path
import extract_rom
import pack
import verify_rom


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path, help='your unmodified USA .nds dump')
    parser.add_argument('-o', '--output', type=Path, default=Path('kohjoh-data'), help='output directory')
    args = parser.parse_args()
    digest = verify_rom.sha1_of(args.rom)
    name = verify_rom.verify(args.rom, digest=digest)
    edition = 'soulsilver' if digest.startswith('f8dc38') else 'heartgold'
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='kohjoh-') as tmp:
        data = Path(tmp)
        extract_rom.extract(args.rom, data)
        pack.pack(data, args.output / (edition + '.pak'), build_id=edition + '-nitrofs-1',
                  rom_sha1=digest, supported=verify_rom.SUPPORTED)
        shutil.copyfile(data / 'manifest.json', args.output / (edition + '.manifest.json'))
    print('Copy the output files to /3ds/kohjoh/data/ on your SD card.')


if __name__ == '__main__':
    main()
