#!/usr/bin/env python3
"""
g3_reference.py: project the decoded 3D scene of a .kjd dump on the CPU.

The 3DS draws the scene the geometry decoder (src/runtime/kj_g3.c) built:
object-space vertices, a clip matrix palette and material batches. This
tool rasterises the same data with plain float math and no GPU state, so a
part of a model that is missing here was lost in decoding, and a part that
is present here but missing on the 3DS was lost in the GPU draw stage
(src/graphics/g3d). Never linked into the 3DS program.

Each batch gets its own flat colour (shaded by the vertex colour), depth is
DS-style (w with W-buffering, z/w otherwise, smaller wins) and triangles with
a vertex at w <= 0 are counted and skipped.

usage: g3_reference.py dump.kjd out.png [--batch N] [--scale S]
"""

import argparse
import struct
import sys
import zlib

VERT = struct.Struct("<3hBB4B2h")    # kj_g3_vertex, 16 bytes
BATCH = struct.Struct("<IIHHIIIII")  # kj_g3_batch, 32 bytes


def read_g3(path):
    data = open(path, "rb").read()
    if data[:4] != b"KJD1":
        sys.exit("not a KJD1 dump")
    off = 12 + 9 * 12 + 4
    for _ in range(9):  # io .. lines
        (n,) = struct.unpack_from("<I", data, off)
        off += 4 + n
    if off + 4 > len(data):
        sys.exit("dump has no 3D section (older build)")
    (present,) = struct.unpack_from("<I", data, off)
    off += 4
    if not present:
        sys.exit("no 3D scene in this frame")
    swap, viewport, disp3dcnt = struct.unpack_from("<III", data, off)
    off += 12
    blobs = []
    for _ in range(4):
        (n,) = struct.unpack_from("<I", data, off)
        blobs.append(data[off + 4:off + 4 + n])
        off += 4 + n
    verts = [VERT.unpack_from(blobs[0], i) for i in range(0, len(blobs[0]), VERT.size)]
    indices = struct.unpack("<%dH" % (len(blobs[1]) // 2), blobs[1])
    mats = []
    for i in range(0, len(blobs[2]), 64):
        f = struct.unpack_from("<16f", blobs[2], i)
        mats.append([f[0:4], f[4:8], f[8:12], f[12:16]])
    batches = [BATCH.unpack_from(blobs[3], i) for i in range(0, len(blobs[3]), BATCH.size)]
    return swap, viewport, disp3dcnt, verts, indices, mats, batches


def png(path, w, h, rgb):
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw += rgb[y * w * 3:(y + 1) * w * 3]

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    out = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    out += chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b"")
    open(path, "wb").write(out)


def batch_colour(i):
    h = (i * 2654435761) & 0xFFFFFF
    return (64 + (h & 0x7F), 64 + ((h >> 8) & 0x7F), 64 + ((h >> 16) & 0x7F))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump")
    ap.add_argument("out")
    ap.add_argument("--batch", type=int, default=-1, help="draw only this batch")
    ap.add_argument("--scale", type=int, default=2)
    args = ap.parse_args()

    swap, vp, disp3dcnt, verts, indices, mats, batches = read_g3(args.dump)
    wbuffer = (swap & 2) != 0
    x1, y1, x2, y2 = vp & 0xFF, (vp >> 8) & 0xFF, (vp >> 16) & 0xFF, (vp >> 24) & 0xFF
    vw, vh = x2 - x1 + 1, y2 - y1 + 1
    S = args.scale
    W, H = 256 * S, 192 * S
    rgb = bytearray(b"\x20\x20\x30" * (W * H))
    depth = [float("inf")] * (W * H)
    stats = {"tris": 0, "w<=0": 0, "zero area": 0, "drawn": 0}

    def project(v, first):
        x, y, z, mtx = v[0] / 4096.0, v[1] / 4096.0, v[2] / 4096.0, v[3]
        m = mats[first + mtx]
        c = [m[0][j] * x + m[1][j] * y + m[2][j] * z + m[3][j] for j in range(4)]  # DS: p x M
        if c[3] <= 0.0:
            return None
        sx = (c[0] / c[3] + 1.0) * vw / 2.0 + x1
        sy = 191.0 - ((c[1] / c[3] + 1.0) * vh / 2.0 + y1)
        d = c[3] if wbuffer else c[2] / c[3]
        return sx * S, sy * S, d

    for bi, b in enumerate(batches):
        first_index, index_count, first_matrix, _mc, attr, _tp, _pl, _vp, flags = b
        if args.batch >= 0 and bi != args.batch:
            continue
        base = batch_colour(bi)
        for t in range(first_index, first_index + index_count, 3):
            stats["tris"] += 1
            tv = [verts[indices[t + k]] for k in range(3)]
            p = [project(v, first_matrix) for v in tv]
            if None in p:
                stats["w<=0"] += 1
                continue
            (ax, ay, ad), (bx, by, bd), (cx, cy, cd) = p
            area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)
            if abs(area) < 1e-9:
                stats["zero area"] += 1
                continue
            stats["drawn"] += 1
            shade = (tv[0][5] + tv[0][6] + tv[0][7]) / (3.0 * 255.0)
            col = bytes(int(c * (0.5 + 0.5 * shade)) for c in base)
            minx, maxx = max(int(min(ax, bx, cx)), 0), min(int(max(ax, bx, cx)) + 1, W - 1)
            miny, maxy = max(int(min(ay, by, cy)), 0), min(int(max(ay, by, cy)) + 1, H - 1)
            for py in range(miny, maxy + 1):
                fy = py + 0.5
                for px in range(minx, maxx + 1):
                    fx = px + 0.5
                    w0 = ((bx - fx) * (cy - fy) - (by - fy) * (cx - fx)) / area
                    w1 = ((cx - fx) * (ay - fy) - (cy - fy) * (ax - fx)) / area
                    w2 = 1.0 - w0 - w1
                    if w0 < 0 or w1 < 0 or w2 < 0:
                        continue
                    d = w0 * ad + w1 * bd + w2 * cd
                    i = py * W + px
                    if d < depth[i]:
                        depth[i] = d
                        rgb[i * 3:i * 3 + 3] = col
    png(args.out, W, H, rgb)
    print("w-buffer %s viewport %d,%d-%d,%d batches %d verts %d matrices %d" %
          (wbuffer, x1, y1, x2, y2, len(batches), len(verts), len(mats)))
    print(" ".join("%s %d" % kv for kv in stats.items()))


if __name__ == "__main__":
    main()
