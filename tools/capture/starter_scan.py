#!/usr/bin/env python3
"""Scan movie dumps for the starter slide-in scene and its particle sprites.

For every .kjd dump in a range, prints a one-line summary per frame:
engine A/B DISPCNT, visible OBJ count and modes, and the bounding area of
non-trivial OBJs (the slide-in starters + grass/fire/water particles are
OBJs; the gradient/white-ramp are BG3/BG2).
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

SIZES = {(0, 0): (8, 8), (0, 1): (16, 16), (0, 2): (32, 32), (0, 3): (64, 64),
         (1, 0): (16, 8), (1, 1): (32, 8), (1, 2): (32, 16), (1, 3): (64, 32),
         (2, 0): (8, 16), (2, 1): (8, 32), (2, 2): (16, 32), (2, 3): (32, 64)}

DUMPS = Path(sys.argv[1] if len(sys.argv) > 1 else
             r"C:\Users\terezi\workdir\LOTUS-v0.0.3-windows-x64\data\citnlo\sessions"
             r"\cards\kohjoh-prof\sdmc\3ds\kohjoh\dumps")


def scan(path: Path) -> str:
    data = path.read_bytes()
    if data[:4] != b"KJD1":
        return f"{path.name}: not KJD1"
    frame = struct.unpack_from("<I", data, 4)[0]
    off = 12 + 9 * 12 + 4
    b = {}
    for n in ("io", "pltt", "oam", "bg_a", "bg_b", "obj_a", "obj_b", "lcdc", "lines"):
        (l,) = struct.unpack_from("<I", data, off)
        off += 4
        b[n] = data[off:off + l]
        off += l
    oam = b["oam"]
    dispcnt = struct.unpack_from("<I", b["lcdc"], 0)[0], struct.unpack_from("<I", b["lcdc"], 4)[0]
    out = [f"f{frame:05d} dispcnt A={dispcnt[0]:08x} B={dispcnt[1]:08x}"]
    for eng, base in (("A", 0), ("B", 1024)):
        if base + 1024 > len(oam):
            break
        vis = 0
        modes = {}
        xs, ys = [], []
        for i in range(128):
            o = base + i * 8
            a0, a1, a2 = (struct.unpack_from("<H", oam, o + k)[0] for k in (0, 2, 4))
            if a0 & 0x200 and not a0 & 0x100:
                continue
            mode = (a0 >> 10) & 3
            w, h = SIZES.get(((a0 >> 14), (a1 >> 14) & 3), (0, 0))
            if not w:
                continue
            vis += 1
            modes[mode] = modes.get(mode, 0) + 1
            if mode < 2:
                x = (a1 & 0x1FF) - (512 if (a1 & 0x1FF) >= 256 else 0)
                y = a0 & 0xFF
                xs.append(x)
                ys.append(y)
        ext = f" objs={vis} modes={modes}"
        if xs:
            ext += f" x=[{min(xs)}..{max(xs)}] y=[{min(ys)}..{max(ys)}]"
        out.append(eng + ext)
    return "  ".join(out)


def main() -> None:
    for p in sorted(DUMPS.glob("f0*.kjd")):
        frame = int(p.stem[1:])
        if 900 <= frame <= 2200:
            print(scan(p))


if __name__ == "__main__":
    main()
