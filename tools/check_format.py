#!/usr/bin/env python3
"""Check first-party formatting against the repository .clang-format.

Cleanup task 12: formatting is mechanical and separate from semantic
changes. This covers Kohjoh's own C/H only. The decompilation, NitroSDK,
compat emulation layers and generated assets are deliberately excluded.
CI pins clang-format 18.1.8 (`pip install clang-format==18.1.8`).

    python tools/check_format.py           # check (default)
    python tools/check_format.py --write   # apply
"""

import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
DIRS = ("src/app", "src/graphics", "src/audio", "src/storage", "src/content",
        "src/diagnostics", "src/runtime", "tests/native")


def files():
    selected = []
    for directory in DIRS:
        base = ROOT / directory
        if base.is_dir():
            selected += [p for p in base.rglob("*") if p.suffix in (".c", ".h") and p.is_file()]
    return sorted(selected)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    tool = shutil.which("clang-format")
    if tool is None:
        sys.exit("clang-format not found; install the pinned version (18.1.8)")
    selected = files()
    if not selected:
        sys.exit("no first-party C/H files found")
    if args.write:
        subprocess.run([tool, "-i", *[str(p) for p in selected]], cwd=ROOT, check=True)
        return 0
    result = subprocess.run([tool, "--dry-run", "--Werror", *[str(p) for p in selected]],
                            cwd=ROOT)
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
