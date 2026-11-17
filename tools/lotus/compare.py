"""Byte-compare a LOTUS capture folder's PNGs against reference PNGs.

Usage: python tools/lotus/compare.py <new_dir> <ref_dir>
Compares files whose names match fNNNNN-top.png / fNNNNN-bottom.png present
in both folders; prints sha256 and PASS/DIFF per file.
"""
import hashlib
import sys
from pathlib import Path


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()[:16]


def main() -> int:
    new, ref = Path(sys.argv[1]), Path(sys.argv[2])
    failures = 0
    for f in sorted(new.glob("f*-*.png")):
        g = ref / f.name
        if not g.is_file():
            print(f"{f.name}: no reference")
            continue
        same = sha(f) == sha(g)
        print(f"{f.name}: {'PASS' if same else 'DIFF'} ({sha(f)} vs {sha(g)})")
        failures += 0 if same else 1
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
