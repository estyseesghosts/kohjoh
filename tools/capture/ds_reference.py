#!/usr/bin/env python3
"""Offline DS 2D reference renderer for kohjoh state dumps (.kjd).

Host-only test oracle (kohjoh.md 7, milestone B): renders what the DS would
show for the DS state the 3DS renderer read for one game frame, written
straight from GBATEK per scanline. It shares no code with the GPU path, so a
disagreement points at one of them. Never linked into the 3DS program.

The 3D layer is not rendered (drawn as transparent BG0); compare 3D scenes
with that in mind.

    python tools/capture/ds_reference.py dump.kjd out_prefix
      -> out_prefix_A.png, out_prefix_B.png (256x192 each), and
         out_prefix_screens.png (top over bottom, as the 3DS shows them)
"""
from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path

W, H = 256, 192
OBJ_SIZES = {
    (0, 0): (8, 8), (0, 1): (16, 16), (0, 2): (32, 32), (0, 3): (64, 64),
    (1, 0): (16, 8), (1, 1): (32, 8), (1, 2): (32, 16), (1, 3): (64, 32),
    (2, 0): (8, 16), (2, 1): (8, 32), (2, 2): (16, 32), (2, 3): (32, 64),
}


class Dump:
    def __init__(self, data: bytes):
        if data[:4] != b"KJD1":
            raise ValueError("not a kohjoh dump")
        self.frame, self.top_a = struct.unpack_from("<II", data, 4)
        off = 12
        self.banks = []
        for _ in range(9):
            lo, size, en, mst, ofs, _p = struct.unpack_from("<IIBBBB", data, off)
            self.banks.append((lo, size, en, mst, ofs))
            off += 12
        (self.lines_active,) = struct.unpack_from("<I", data, off)
        off += 4
        blobs = []
        for _ in range(9):
            (n,) = struct.unpack_from("<I", data, off)
            off += 4
            blobs.append(data[off:off + n])
            off += n
        (self.io, self.pltt, self.oam, self.bg_a, self.bg_b, self.obj_a, self.obj_b,
         self.lcdc, self.lines) = blobs

    def line_regs(self, engine: int, y: int) -> bytes:
        if self.lines_active:
            base = (engine * 192 + y) * 0x70
            return self.lines[base:base + 0x70]
        return self.io[engine * 0x1000:engine * 0x1000 + 0x70]


def u16(b, o):
    return b[o] | (b[o + 1] << 8)


def u32(b, o):
    return b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | (b[o + 3] << 24)


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


class Engine:
    def __init__(self, d: Dump, e: int):
        self.d, self.e = d, e
        self.bg = d.bg_a if e == 0 else d.bg_b
        self.obj = d.obj_a if e == 0 else d.obj_b
        self.pal_bg = d.pltt[e * 0x400:e * 0x400 + 0x200]
        self.pal_obj = d.pltt[e * 0x400 + 0x200:e * 0x400 + 0x400]
        self.oam = d.oam[e * 0x400:e * 0x400 + 0x400]

    def ext_bg(self, slot):
        b, lc = self.d.banks, self.d.lcdc
        if self.e == 0:
            if b[4][2] and b[4][3] == 4:
                return lc[b[4][0] + slot * 0x2000:]
            for i in (5, 6):
                if b[i][2] and b[i][3] == 4:
                    first = 2 if b[i][4] & 1 else 0
                    if first <= slot < first + 2:
                        return lc[b[i][0] + (slot - first) * 0x2000:]
            return None
        if b[7][2] and b[7][3] == 2:
            return lc[b[7][0] + slot * 0x2000:]
        return None

    def ext_obj(self):
        b, lc = self.d.banks, self.d.lcdc
        if self.e == 0:
            for i in (5, 6):
                if b[i][2] and b[i][3] == 5:
                    return lc[b[i][0]:]
            return None
        if b[8][2] and b[8][3] == 3:
            return lc[b[8][0]:]
        return None


def text_bg_pixel(eng, regs, bg, x, y, dispcnt):
    cnt = u16(regs, 0x08 + 2 * bg)
    hofs = u16(regs, 0x10 + 4 * bg) & 0x1FF
    vofs = u16(regs, 0x12 + 4 * bg) & 0x1FF
    size = cnt >> 14
    w = 512 if size & 1 else 256
    h = 512 if size & 2 else 256
    bpp8 = bool(cnt & 0x80)
    char_base = ((cnt >> 2) & 15) * 0x4000
    scr_base = ((cnt >> 8) & 31) * 0x800
    if eng.e == 0:
        char_base += ((dispcnt >> 24) & 7) * 0x10000
        scr_base += ((dispcnt >> 27) & 7) * 0x10000
    mx = (x + hofs) & (w - 1)
    my = (y + vofs) & (h - 1)
    sb = (my >> 8) * (w >> 8) + (mx >> 8)
    mask = len(eng.bg) - 1
    a = (scr_base + sb * 0x800 + ((my & 255) >> 3) * 64 + ((mx & 255) >> 3) * 2) & mask
    entry = u16(eng.bg, a)
    tx, ty = mx & 7, my & 7
    if entry & 0x400:
        tx = 7 - tx
    if entry & 0x800:
        ty = 7 - ty
    tile = entry & 0x3FF
    pb = entry >> 12
    if bpp8:
        idx = eng.bg[(char_base + tile * 64 + ty * 8 + tx) & mask]
        if idx == 0:
            return None
        if dispcnt & (1 << 30):
            slot = bg + (2 if (bg < 2 and (cnt >> 13) & 1) else 0) if bg < 2 else bg
            ext = eng.ext_bg(slot)
            if ext is not None:
                return u16(ext, pb * 512 + idx * 2)
        return u16(eng.pal_bg, idx * 2)
    b = eng.bg[(char_base + tile * 32 + ty * 4 + (tx >> 1)) & mask]
    idx = (b >> 4) if tx & 1 else (b & 15)
    if idx == 0:
        return None
    return u16(eng.pal_bg, (pb * 16 + idx) * 2)


def affine_bg_pixel(eng, regs, bg, x, y, dispcnt, ext16, refx, refy):
    base = 0x20 if bg == 2 else 0x30
    cnt = u16(regs, 0x08 + 2 * bg)
    pa, pc = s16(u16(regs, base)), s16(u16(regs, base + 4))
    tx_ = (refx + pa * x) >> 8
    ty_ = (refy + pc * x) >> 8
    size = 128 << (cnt >> 14)
    if cnt & 0x2000:
        tx_ &= size - 1
        ty_ &= size - 1
    elif not (0 <= tx_ < size and 0 <= ty_ < size):
        return None
    char_base = ((cnt >> 2) & 15) * 0x4000
    scr_base = ((cnt >> 8) & 31) * 0x800
    if eng.e == 0:
        char_base += ((dispcnt >> 24) & 7) * 0x10000
        scr_base += ((dispcnt >> 27) & 7) * 0x10000
    mask = len(eng.bg) - 1
    tiles = size // 8
    if ext16:
        if cnt & 0x80:
            return None  # bitmap modes: not modelled
        e = u16(eng.bg, (scr_base + ((ty_ >> 3) * tiles + (tx_ >> 3)) * 2) & mask)
        px, py = tx_ & 7, ty_ & 7
        if e & 0x400:
            px = 7 - px
        if e & 0x800:
            py = 7 - py
        idx = eng.bg[(char_base + (e & 0x3FF) * 64 + py * 8 + px) & mask]
        if idx == 0:
            return None
        if dispcnt & (1 << 30):
            ext = eng.ext_bg(bg)
            if ext is not None:
                return u16(ext, (e >> 12) * 512 + idx * 2)
        return u16(eng.pal_bg, idx * 2)
    tile = eng.bg[(scr_base + (ty_ >> 3) * tiles + (tx_ >> 3)) & mask]
    idx = eng.bg[(char_base + tile * 64 + (ty_ & 7) * 8 + (tx_ & 7)) & mask]
    return None if idx == 0 else u16(eng.pal_bg, idx * 2)


def obj_line(eng, regs, y, dispcnt):
    """Per pixel: (prio, color, semi) of the top OBJ, or None."""
    out = [None] * W
    if not dispcnt & 0x1000:
        return out
    map1d = bool(dispcnt & 0x10)
    boundary = 32 << ((dispcnt >> 20) & 3)
    ext = eng.ext_obj() if dispcnt & (1 << 31) else None
    mask = len(eng.obj) - 1
    for i in range(127, -1, -1):
        a0, a1, a2 = u16(eng.oam, i * 8), u16(eng.oam, i * 8 + 2), u16(eng.oam, i * 8 + 4)
        affine = bool(a0 & 0x100)
        if not affine and a0 & 0x200:
            continue
        mode = (a0 >> 10) & 3
        if mode >= 2:
            continue
        shape, sz = a0 >> 14, a1 >> 14
        if shape == 3:
            continue
        w, h = OBJ_SIZES[(shape, sz)]
        dbl = affine and bool(a0 & 0x200)
        bw, bh = (w * 2, h * 2) if dbl else (w, h)
        oy = a0 & 0xFF
        if oy + bh > 256:
            oy -= 256
        if not (oy <= y < oy + bh):
            continue
        ox = a1 & 0x1FF
        if ox >= 256:
            ox -= 512
        bpp8 = bool(a0 & 0x2000)
        tnum = a2 & 0x3FF
        prio = (a2 >> 10) & 3
        pb = a2 >> 12
        if affine:
            g = (a1 >> 9) & 31
            pa = s16(u16(eng.oam, g * 32 + 6))
            pbb = s16(u16(eng.oam, g * 32 + 14))
            pc = s16(u16(eng.oam, g * 32 + 22))
            pd = s16(u16(eng.oam, g * 32 + 30))
        for sx in range(bw):
            X = ox + sx
            if not (0 <= X < W):
                continue
            if affine:
                dx, dy = sx - bw // 2, (y - oy) - bh // 2
                px = ((pa * dx + pbb * dy) >> 8) + w // 2
                py = ((pc * dx + pd * dy) >> 8) + h // 2
                if not (0 <= px < w and 0 <= py < h):
                    continue
            else:
                px, py = sx, y - oy
                if a1 & 0x1000:
                    px = w - 1 - px
                if a1 & 0x2000:
                    py = h - 1 - py
            tx, ty = px >> 3, py >> 3
            if map1d:
                off = tnum * boundary + (ty * (w // 8) + tx) * (64 if bpp8 else 32)
            else:
                off = tnum * 32 + ty * 1024 + tx * (64 if bpp8 else 32)
            if bpp8:
                idx = eng.obj[(off + (py & 7) * 8 + (px & 7)) & mask]
                if idx == 0:
                    continue
                col = u16(ext, pb * 512 + idx * 2) if ext is not None else u16(eng.pal_obj, idx * 2)
            else:
                b = eng.obj[(off + (py & 7) * 4 + ((px & 7) >> 1)) & mask]
                idx = (b >> 4) if px & 1 else (b & 15)
                if idx == 0:
                    continue
                col = u16(eng.pal_obj, (pb * 16 + idx) * 2)
            cur = out[X]
            if cur is None or prio <= cur[0]:
                out[X] = (prio, col, mode == 1)
    return out


def render_engine(d: Dump, e: int):
    eng = Engine(d, e)
    img = []
    refs = {}
    for y in range(H):
        regs = d.line_regs(e, y)
        dispcnt = u32(regs, 0)
        if (dispcnt >> 16) & 3 != 1 or dispcnt & 0x80:
            img.append([0x7FFF] * W)
            continue
        mode = dispcnt & 7
        on = (dispcnt >> 8) & 0x1F
        bldcnt = u16(regs, 0x50)
        bmode = (bldcnt >> 6) & 3
        eva = min(16, u16(regs, 0x52) & 31)
        evb = min(16, (u16(regs, 0x52) >> 8) & 31)
        evy = min(16, u16(regs, 0x54) & 31)
        backdrop = u16(eng.pal_bg, 0)
        objs = obj_line(eng, regs, y, dispcnt)
        # affine reference points: the latched value is the register's own
        # at line 0 / when rewritten, then += PB / PD per line
        aff = {}
        for bg in (2, 3):
            base = 0x20 if bg == 2 else 0x30
            rx = (u32(regs, base + 8) << 4) & 0xFFFFFFFF
            ry = (u32(regs, base + 12) << 4) & 0xFFFFFFFF
            rx = (rx - (1 << 32) if rx & 0x80000000 else rx) >> 4
            ry = (ry - (1 << 32) if ry & 0x80000000 else ry) >> 4
            key = (bg, rx, ry)
            prev = refs.get(bg)
            if prev is None or prev[0] != (rx, ry):
                cur = (rx, ry)
            else:
                cur = (prev[1][0] + s16(u16(regs, base + 2)), prev[1][1] + s16(u16(regs, base + 6)))
            refs[bg] = ((rx, ry), cur)
            aff[bg] = cur
        line = []
        for x in range(W):
            layers = []  # (prio, order, color, layer_id, semi)
            for bg in range(4):
                if not on & (1 << bg):
                    continue
                if bg == 0 and e == 0 and dispcnt & 8:
                    continue  # 3D: not modelled
                cnt = u16(regs, 0x08 + 2 * bg)
                kind = "text"
                if (mode == 1 and bg == 3) or (mode == 2 and bg >= 2) or (mode == 4 and bg == 2):
                    kind = "affine"
                elif (mode == 3 and bg == 3) or (mode == 4 and bg == 3) or (mode == 5 and bg >= 2):
                    kind = "ext"
                elif mode == 6 and bg >= 2:
                    continue
                if kind == "text":
                    c = text_bg_pixel(eng, regs, bg, x, y, dispcnt)
                else:
                    c = affine_bg_pixel(eng, regs, bg, x, y, dispcnt, kind == "ext", aff[bg][0], aff[bg][1])
                if c is not None:
                    layers.append((cnt & 3, 1 + bg, c, bg, False))
            o = objs[x]
            if o is not None and on & 0x10:
                layers.append((o[0], 0, o[1], 4, o[2]))
            layers.sort(key=lambda t: (t[0], t[1]))
            layers.append((4, 9, backdrop, 5, False))
            layers.append((5, 9, backdrop, 6, False))  # nothing below the backdrop
            top, under = layers[0], layers[1]
            c = top[2]
            t1 = bool((bldcnt >> top[3]) & 1)
            t2u = bool((bldcnt >> (8 + under[3])) & 1)
            if (top[4] or (t1 and bmode == 1)) and t2u:
                c = blend(c, under[2], eva, evb)
            elif t1 and bmode == 2:
                c = bright(c, evy, True)
            elif t1 and bmode == 3:
                c = bright(c, evy, False)
            line.append(c & 0x7FFF)
        mb = u16(regs, 0x6C)
        if (mb >> 14) in (1, 2) and mb & 31:
            line = [bright(c, min(16, mb & 31), (mb >> 14) == 1) for c in line]
        img.append(line)
    return img


def chan(c, i):
    return (c >> (5 * i)) & 31


def blend(a, b, eva, evb):
    return sum(min(31, (chan(a, i) * eva + chan(b, i) * evb) >> 4) << (5 * i) for i in range(3))


def bright(c, evy, up):
    out = 0
    for i in range(3):
        v = chan(c, i)
        v = v + (((31 - v) * evy) >> 4) if up else v - ((v * evy) >> 4)
        out |= v << (5 * i)
    return out


def write_png(path: Path, rows) -> None:
    h, w = len(rows), len(rows[0])
    raw = bytearray()
    for r in rows:
        raw.append(0)
        for c in r:
            raw += bytes(((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b"")
    path.write_bytes(png)


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    d = Dump(Path(argv[1]).read_bytes())
    a = render_engine(d, 0)
    b = render_engine(d, 1)
    write_png(Path(argv[2] + "_A.png"), a)
    write_png(Path(argv[2] + "_B.png"), b)
    top, bot = (a, b) if d.top_a else (b, a)
    write_png(Path(argv[2] + "_screens.png"), top + bot)
    print(f"frame {d.frame}: lines_active={d.lines_active} top={'A' if d.top_a else 'B'}")
    for e in range(2):
        r = d.line_regs(e, 0)
        print(f"  engine {'AB'[e]}: DISPCNT={u32(r, 0):08x} BGCNT={[hex(u16(r, 8 + 2 * i)) for i in range(4)]}"
              f" BLDCNT={u16(r, 0x50):04x} MB={u16(r, 0x6C):04x}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
