"""Check a 2x field capture against the Task 2D acceptance conditions.

Usage:
    python tools/qa/check_2x_blocks.py capture.bgra [--width 400] [--height 240]
    python tools/qa/check_2x_blocks.py capture.bgra --x0 40 --x1 360
    python tools/qa/check_2x_blocks.py capture.bgra --source field_1x.bgra

Input is a raw BGRA8 top-screen capture: LOTUS `lotus_capture` writes one as
`lotus://artifact/<id>.bgra`, and `tools/lotus/run.py` can convert the same
bytes to PNG.

The block-uniformity assertion alone is insufficient (a black or mirrored
frame would score 100%). With `--source`, the tool also compares the 2x output
against an independently generated nearest-neighbour enlargement of the same
frame captured in 1x mode. The two captures must be of the same game frame,
save and input script. The canonical mapping between them is

    source_x = dest_x / 2 + 100
    source_y = dest_y / 2 + 60

so every 2x 2x2 block must equal one 1x pixel. This is exactly
`np.repeat(np.repeat(source_200x120, 2, 0), 2, 1)`, but uses the 1x capture as
an independent rendering of the same source rather than a CPU enlargement of
the 2x texture itself.
"""
from __future__ import annotations

import argparse
import sys

WIDTH = 400
HEIGHT = 240


def check_blocks(data: bytes, args) -> bool:
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
    return bool(total) and same == total


def check_nearest_neighbour(two_x: bytes, one_x: bytes) -> bool:
    if len(one_x) < WIDTH * HEIGHT * 4:
        print(f"FAIL: source capture has {len(one_x)} bytes, expected {WIDTH * HEIGHT * 4}")
        return False
    same = total = 0
    first_bad = None
    for y in range(0, HEIGHT, 2):
        for x in range(0, WIDTH, 2):
            sx, sy = x // 2 + 100, y // 2 + 60
            if not (0 <= sx < WIDTH and 0 <= sy < HEIGHT):
                continue
            src = one_x[(sy * WIDTH + sx) * 4:(sy * WIDTH + sx) * 4 + 3]
            for dy in (0, 1):
                for dx in (0, 1):
                    px = (y + dy) * WIDTH + x + dx
                    dst = two_x[px * 4:px * 4 + 3]
                    total += 1
                    if src == dst:
                        same += 1
                    elif first_bad is None:
                        first_bad = (x + dx, y + dy, tuple(src), tuple(dst))
    print(f"2x pixels equal to independent 1x source: {same}/{total} ({same / total:.3%})")
    if first_bad is not None:
        print(f"first mismatch: {first_bad}")
    return bool(total) and same == total


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("capture")
    ap.add_argument("--width", type=int, default=WIDTH)
    ap.add_argument("--height", type=int, default=HEIGHT)
    ap.add_argument("--x0", type=int, default=0)
    ap.add_argument("--x1", type=int, default=WIDTH)
    ap.add_argument("--y0", type=int, default=0)
    ap.add_argument("--y1", type=int, default=HEIGHT)
    ap.add_argument("--source", help="same-frame 1x raw BGRA capture for exact NN comparison")
    args = ap.parse_args()

    data = open(args.capture, "rb").read()
    need = args.width * args.height * 4
    if len(data) < need:
        print(f"FAIL: {args.capture} has {len(data)} bytes, expected {need}")
        return 1

    ok = check_blocks(data, args)
    if args.source is not None:
        ok = check_nearest_neighbour(data, open(args.source, "rb").read()) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
