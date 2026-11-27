"""Check that a 2x field capture is exactly the 1x central field at 2x scale.

For the canonical mappings:
    1x: phys_x = ds_x + 72, phys_y = ds_y + 24
    2x: phys_x = 2*(ds_x - 28), phys_y = 2*(ds_y - 36)
so 1x pixel (X/2 + 100, Y/2 + 60) must equal the 2x 2x2 block at (X, Y).
Both captures must be of the same game frame (same save/script/frame count).

    python tools/qa/check_2x_matches_1x.py field_1x.bgra field_2x.bgra
"""
from __future__ import annotations

import argparse
import sys

WIDTH = 400
HEIGHT = 240


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("one_x")
    ap.add_argument("two_x")
    args = ap.parse_args()

    a = open(args.one_x, "rb").read()
    b = open(args.two_x, "rb").read()
    if len(a) < WIDTH * HEIGHT * 4 or len(b) < WIDTH * HEIGHT * 4:
        print("FAIL: capture is not a full 400x240 BGRA frame")
        return 1

    same = total = 0
    first_bad = None
    for y in range(0, HEIGHT, 2):
        for x in range(0, WIDTH, 2):
            ax, ay = x // 2 + 100, y // 2 + 60
            if not (0 <= ax < WIDTH and 0 <= ay < HEIGHT):
                continue
            pa = a[(ay * WIDTH + ax) * 4:(ay * WIDTH + ax) * 4 + 3]
            for dx in (0, 1):
                for dy in (0, 1):
                    pb = b[((y + dy) * WIDTH + x + dx) * 4:((y + dy) * WIDTH + x + dx) * 4 + 3]
                    total += 1
                    if pa == pb:
                        same += 1
                    elif first_bad is None:
                        first_bad = (x + dx, y + dy, tuple(pa), tuple(pb))
    print(f"2x pixels equal to the 1x central crop: {same}/{total} ({same / total:.3%})")
    if first_bad is not None:
        print(f"first mismatch: {first_bad}")
    return 0 if total and same == total else 1


if __name__ == "__main__":
    sys.exit(main())
