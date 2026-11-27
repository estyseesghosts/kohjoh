"""Check that a 2x field capture is made of uniform 2x2 output blocks.

Usage:
    python tools/qa/check_2x_blocks.py capture.bgra [--width 400] [--height 240]
    python tools/qa/check_2x_blocks.py capture.bgra --x0 40 --x1 360

Input is a raw BGRA8 top-screen capture: LOTUS `lotus_capture` writes one as
`lotus://artifact/<id>.bgra`, and `tools/lotus/run.py` can convert the same
bytes to PNG. The assertion excludes native-resolution interface overlays by
restricting the checked region (Task 6 acceptance / Task 14 comparison).
"""
from __future__ import annotations

import argparse
import sys

WIDTH = 400
HEIGHT = 240


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("capture")
    ap.add_argument("--width", type=int, default=WIDTH)
    ap.add_argument("--height", type=int, default=HEIGHT)
    ap.add_argument("--x0", type=int, default=0)
    ap.add_argument("--x1", type=int, default=WIDTH)
    ap.add_argument("--y0", type=int, default=0)
    ap.add_argument("--y1", type=int, default=HEIGHT)
    args = ap.parse_args()

    data = open(args.capture, "rb").read()
    need = args.width * args.height * 4
    if len(data) < need:
        print(f"FAIL: {args.capture} has {len(data)} bytes, expected {need}")
        return 1

    same = total = 0
    first_bad = None
    for y in range(args.y0 + (args.y0 % 2), args.y1 - 1, 2):
        for x in range(args.x0 + (args.x0 % 2), args.x1 - 1, 2):
            i0 = (y * args.width + x) * 4
            i1 = (y * args.width + x + 1) * 4
            i2 = ((y + 1) * args.width + x) * 4
            i3 = ((y + 1) * args.width + x + 1) * 4
            total += 1
            if data[i0:i0 + 4] == data[i1:i1 + 4] == data[i2:i2 + 4] == data[i3:i3 + 4]:
                same += 1
            elif first_bad is None:
                first_bad = (x, y)
    print(f"2x aligned blocks: {same}/{total} ({same / total:.3%})")
    if first_bad is not None:
        print(f"first non-uniform block at {first_bad}")
    return 0 if total and same == total else 1


if __name__ == "__main__":
    sys.exit(main())
