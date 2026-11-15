#!/usr/bin/env python3
"""Conservative offscreen census of decoded .kjd geometry, never run on 3DS.

Uses the backend's viewport and widened top-screen mapping. A triangle is
trivially outside only if all vertices fail the same clip plane; occlusion,
face culling and rasterization are not measured. W mode deliberately omits
Z planes, matching g3d.c. Reads each vertex's matrix-palette slot.
"""

import argparse
import csv
import json
import math

from g3_reference import read_g3


def outcode(vertex, matrix, viewport, wide, wbuffer):
    position = [v / 4096.0 for v in vertex[:3]] + [1.0]
    x, y, z, w = [sum(position[i] * matrix[i][j] for i in range(4))
                  for j in range(4)]
    if not all(math.isfinite(v) for v in (x, y, z, w)):
        return 0  # unknown: cannot prove invisible
    x1, y1 = viewport & 255, (viewport >> 8) & 255
    vw = ((viewport >> 16) & 255) - x1 + 1
    vh = ((viewport >> 24) & 255) - y1 + 1
    left, right = (-32, 288) if wide else (0, 256)
    planes = [vw * x + (vw + 2 * x1 - 2 * left) * w,
              -vw * x + (2 * right - 2 * x1 - vw) * w,
              vh * y + (vh + 2 * y1) * w,
              -vh * y + (384 - 2 * y1 - vh) * w,
              w]
    if not wbuffer:
        planes += [z + w, w - z]
    return sum(1 << i for i, distance in enumerate(planes) if distance < 0)


def census(scene, wide=True):
    swap, _, _, verts, indices, matrices, batches = scene
    rows = []
    for batch_id, batch in enumerate(batches):
        start, count, first_matrix, matrix_count, _, _, _, viewport, flags = batch
        if count % 3 or start > len(indices) or count > len(indices) - start:
            raise ValueError(f"invalid indices in batch {batch_id}")
        codes = []
        for index in indices[start:start + count]:
            vertex = verts[index]
            slot = vertex[3]
            if slot >= matrix_count:
                raise ValueError(f"invalid palette slot in batch {batch_id}")
            codes.append(outcode(vertex, matrices[first_matrix + slot], viewport,
                                 wide, bool(swap & 2)))
        common = 127
        for code in codes:
            common &= code
        outside = sum(bool(codes[i] & codes[i + 1] & codes[i + 2])
                      for i in range(0, count, 3))
        rows.append(dict(batch=batch_id, triangles=count // 3,
                         outside_triangles=outside,
                         whole_batch_outside=bool(codes) and bool(common),
                         common_planes=common if codes else 0,
                         matrices=matrix_count, flags=flags))
    triangles = sum(row['triangles'] for row in rows)
    outside = sum(row['outside_triangles'] for row in rows)
    summary = dict(batches=len(rows), triangles=triangles,
                   trivially_outside_triangles=outside,
                   potentially_visible_triangles=triangles - outside,
                   whole_batches_outside=sum(row['whole_batch_outside'] for row in rows),
                   triangles_in_whole_outside_batches=sum(
                       row['triangles'] for row in rows if row['whole_batch_outside']),
                   wide=wide, wbuffer=bool(swap & 2))
    return summary, rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dump')
    parser.add_argument('--csv', help='write all batch classifications')
    parser.add_argument('--ds-width', action='store_true',
                        help='256 logical pixels (bottom screen), instead of widened top')
    args = parser.parse_args()
    summary, rows = census(read_g3(args.dump), wide=not args.ds_width)
    print(json.dumps(summary, indent=2))
    if args.csv and rows:
        with open(args.csv, 'w', newline='') as output:
            writer = csv.DictWriter(output, fieldnames=rows[0].keys())
            writer.writeheader()
            writer.writerows(rows)


if __name__ == '__main__':
    main()
