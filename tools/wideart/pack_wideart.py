#!/usr/bin/env python3
"""Precompute the wide art: the hand-drawn bands for the wider screen.

The remaster's upper screen is 32 DS pixels wider on each side than the DS's
picture. The game draws nothing there, so the port fills the bands itself:
with a plain edge colour, or with a picture drawn by hand for that layer
(`docs/PORTING.md`, the WideArt feature; the pictures are pokeplatinum-3ds's,
GPL-3.0, in `wideart/`).

A picture belongs to one layer of the game and is found by that layer's art
key (a hash of the layer's own art, not of where the game put it). The
runtime has to work the key out from the game's VRAM; that is only possible
if the hash is the same one the pictures were named with, so this tool does
not touch the pictures' names: it reads the key out of the file name and
writes it into the table it builds.

What this tool does *not* do is leave any work for the console:

  - the PNG is decoded here, not on the 3DS;
  - the bands are cut out and converted to the console's texture format
    (RGBA5551, the DS's own 5-bit colours) here;
  - the texture is laid out in the PICA's own tile order (8x8 tiles, Morton
    order inside a tile) here, so the console only uploads it;
  - a pixel that was the game's own (the shared pictures' marker, alpha 64)
    becomes transparent: the console draws the hand-drawn part of a band and
    leaves the rest to whatever is behind it.

Usage:
    python tools/wideart/pack_wideart.py --source wideart --output build/wideart/wideart.bin

The output is a binary blob: a header, a table of pictures, and the texture.
`engine/graphics/g2d/wideart.c` reads it.
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
import zlib
from pathlib import Path

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"

# The wide canvas: the DS's 256 pixels, 32 more on each side.
CANVAS_W = 320
CANVAS_H = 192
BAND_W = 32
BAND_H = 192

# The texture the bands live in. The console's texture space is the PICA's:
# a texture's width and height must be powers of two (citro3d's
# checkTexSize), so the texture is 512x1024. The bands are 32 wide and 192
# tall, so 16 fit across and three rows (48 bands) fit in the texture's
# height. Only the rows the bands use are in the blob; the rest of the
# texture is cleared at startup.
TEX_W = 512
TEX_H = 1024

MAGIC = b"SJWA"
VERSION = 1

# A picture's file name: <key>_<hofs>_<vofs>[_full|_loop].png
NAME_RE = re.compile(r"^([0-9a-f]{16})_(-?\d+)_(-?\d+)(_full|_loop)?\.png$")

# The shared pictures' marker: the pixel was the game's own.
MARKER_ALPHA = 64


class PackError(Exception):
    pass


def read_png(data: bytes) -> tuple[int, int, list[bytes]]:
    """(width, height, rows of RGBA bytes) of an 8-bit RGBA PNG."""
    if not data.startswith(PNG_SIGNATURE):
        raise PackError("not a PNG file")
    pos = len(PNG_SIGNATURE)
    header = None
    idat = bytearray()
    while pos + 8 <= len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break
    if header is None:
        raise PackError("no IHDR chunk")
    width, height, depth, color, _comp, _filt, interlace = header
    if depth != 8 or color != 6:
        raise PackError(f"expected an 8-bit RGBA PNG, got depth {depth} colour {color}")
    if interlace != 0:
        raise PackError("interlaced PNGs are not supported")
    raw = zlib.decompress(bytes(idat))
    stride = width * 4
    rows: list[bytes] = []
    prev = bytearray(stride)
    pos = 0
    for _ in range(height):
        kind = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for i in range(stride):
            a = line[i - 4] if i >= 4 else 0
            b = prev[i]
            c = prev[i - 4] if i >= 4 else 0
            if kind == 0:
                continue
            if kind == 1:
                line[i] = (line[i] + a) & 0xFF
            elif kind == 2:
                line[i] = (line[i] + b) & 0xFF
            elif kind == 3:
                line[i] = (line[i] + ((a + b) >> 1)) & 0xFF
            elif kind == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 0xFF
            else:
                raise PackError(f"bad filter type {kind}")
        rows.append(bytes(line))
        prev = line
    return width, height, rows


def morton(x: int, y: int) -> int:
    """The texel's place in a PICA tile (8x8, Morton order, rows from the top)."""
    return ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) |
            ((x & 4) << 2) | ((y & 4) << 3))


def rgba5551(px: bytes) -> int:
    """One RGBA8888 pixel as RGBA5551 (5 bits a channel, 1 bit of alpha)."""
    r, g, b, a = px[0], px[1], px[2], px[3]
    if a < 128:
        return 0  # transparent (and the game's own, the shared pictures' marker)
    return ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | 1


def band_pixels(rows: list[bytes], x0: int) -> list[list[int]]:
    """The band's columns (x0..x0+BAND_W) as RGBA5551 texels, row by row."""
    out = []
    for y in range(BAND_H):
        row = rows[y]
        out.append([rgba5551(row[(x0 + x) * 4:(x0 + x) * 4 + 4]) for x in range(BAND_W)])
    return out


def write_band(tex: list[int], tex_w: int, at_x: int, at_y: int, pixels: list[list[int]]) -> None:
    """Put a band into the texture, in the PICA's tile order."""
    for ty in range(BAND_H // 8):
        for tx in range(BAND_W // 8):
            tile = (at_y // 8 + ty) * (tex_w // 8) + (at_x // 8 + tx)
            base = tile * 64
            for y in range(8):
                for x in range(8):
                    tex[base + morton(x, y)] = pixels[ty * 8 + y][tx * 8 + x]


def build(source: Path, output: Path) -> int:
    pictures = []
    for png in sorted(source.glob("*.png")):
        m = NAME_RE.match(png.name)
        if m is None:
            continue  # a picture a screen's layout draws, not a band
        key = int(m.group(1), 16)
        hofs = int(m.group(2))
        vofs = int(m.group(3))
        flags = 0
        if m.group(4) == "_full":
            flags |= 1
        elif m.group(4) == "_loop":
            flags |= 2
        width, height, rows = read_png(png.read_bytes())
        if (width, height) != (CANVAS_W, CANVAS_H):
            raise PackError(f"{png.name}: expected {CANVAS_W}x{CANVAS_H}, got {width}x{height}")
        pictures.append((key, hofs, vofs, flags, band_pixels(rows, 0), band_pixels(rows, CANVAS_W - BAND_W)))

    per_row = TEX_W // BAND_W
    rows_needed = (len(pictures) * 2 + per_row - 1) // per_row
    data_h = rows_needed * BAND_H
    if data_h > TEX_H:
        raise PackError(f"{len(pictures)} pictures do not fit in a {TEX_W}x{TEX_H} texture")

    tex = [0] * (TEX_W * data_h)
    records = []
    for i, (key, hofs, vofs, flags, left, right) in enumerate(pictures):
        band = i * 2
        lx = (band % per_row) * BAND_W
        ly = (band // per_row) * BAND_H
        write_band(tex, TEX_W, lx, ly, left)
        rx = ((band + 1) % per_row) * BAND_W
        ry = ((band + 1) // per_row) * BAND_H
        write_band(tex, TEX_W, rx, ry, right)
        records.append((key, hofs, vofs, flags, lx, ly, rx, ry))

    blob = bytearray()
    blob += MAGIC
    blob += struct.pack("<HHHHHH", VERSION, len(records), TEX_W, TEX_H, data_h, 0)
    for key, hofs, vofs, flags, lx, ly, rx, ry in records:
        blob += struct.pack("<QhhHHHHHHH6x", key, hofs, vofs, flags, lx, ly, rx, ry,
                            BAND_W, BAND_H)
    for texel in tex:
        blob += struct.pack("<H", texel)

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(bytes(blob))
    return len(records)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--source", type=Path, default=Path("wideart"))
    ap.add_argument("--output", type=Path, default=Path("build/wideart/wideart.bin"))
    args = ap.parse_args()
    try:
        count = build(args.source, args.output)
    except PackError as e:
        sys.exit(f"pack_wideart: {e}")
    print(f"wideart: {count} pictures -> {args.output} ({args.output.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
