#!/usr/bin/env python3
"""Decode each BG/OBJ layer of an engine in a .kjd dump to its own PNG."""
import struct, sys, zlib
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "capture"))

W, H = 256, 192

def load(p):
    b = Path(p).read_bytes()
    off = 4
    def u32():
        nonlocal off
        v = struct.unpack_from('<I', b, off)[0]; off += 4; return v
    u32(); u32()
    for _ in range(9):
        u32(); u32(); off += 4
    u32()
    blobs = []
    for _ in range(8):
        l = u32(); blobs.append(b[off:off+l]); off += l
    io, pltt, oam, bg_a, bg_b, obj_a, obj_b, lcdc = blobs
    return io, pltt, oam, bg_a, bg_b, obj_a, obj_b, lcdc

def r16(buf, base, o): return struct.unpack_from('<H', buf, base + o)[0]
def r32(buf, base, o): return struct.unpack_from('<I', buf, base + o)[0]

def write_png(path, w, h, pixels):
    rows = b''.join(b'\x00' + bytes(v for px in row for v in px) for row in pixels)
    def chunk(tag, data):
        c = tag + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c))
    png = (b'\x89PNG\r\n\x1a\n'
           + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
           + chunk(b'IDAT', zlib.compress(rows))
           + chunk(b'IEND', b''))
    Path(path).write_bytes(png)

def decode_tile4(vram, toff, pltt, pal_off, rowflip):
    px = []
    for y in range(8):
        yy = (7 - y) if rowflip else y
        px.append([])
        for x in range(8):
            by = vram[toff + yy * 4 + (x >> 1)]
            idx = (by & 15) if (x & 1) else (by >> 4)
            c = struct.unpack_from('<H', pltt, pal_off + idx * 2)[0]
            r, g, bl = (c & 31) * 8, ((c >> 5) & 31) * 8, ((c >> 10) & 31) * 8
            if idx == 0:
                px[-1].extend((0, 0, 0))
            else:
                px[-1].extend((r, g, bl))
    return px

def main(p, prefix):
    io, pltt, oam, bg_a, bg_b, obj_a, obj_b, lcdc = load(p)
    for eng, name, base, bgram in ((0, 'A', 0, bg_a), (1, 'B', 0x1000, bg_b)):
        dc = r32(io, base, 0)
        for bg in range(4):
            cnt = r16(io, base, 8 + bg * 2)
            if not (dc & (1 << (8 + bg))):
                continue
            cbb = (cnt >> 2) & 3; sbb = (cnt >> 8) & 31; sz = cnt >> 14
            pal_base = ((cnt >> 12) & 15) * 32
            img = [[(0, 0, 0)] * W for _ in range(H)]
            for my in range(32):
                for mx in range(32):
                    ent = struct.unpack_from('<H', bgram, sbb * 0x800 + (my * 32 + mx) * 2)[0]
                    tile = ent & 0x3FF
                    if tile == 0:
                        continue
                    hf = (ent >> 10) & 1; vf = (ent >> 11) & 1
                    pl = (ent >> 12) & 15
                    toff = cbb * 0x4000 + tile * 32
                    t = decode_tile4(bgram, toff, pltt, pal_base + pl * 32, vf)
                    for y in range(8):
                        for x in range(8):
                            sx = mx * 8 + (7 - x) if hf else mx * 8 + x
                            sy = my * 8 + y
                            if sx < W and sy < H:
                                img[sy][sx] = tuple(t[y][x * 3:x * 3 + 3])
            write_png(f'{prefix}_{name}_bg{bg}.png', W, H, img)
            print(f'{prefix}_{name}_bg{bg}.png cnt={cnt:04x} cbb={cbb} sbb={sbb}')

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
