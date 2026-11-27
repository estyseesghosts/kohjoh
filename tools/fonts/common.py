"""Shared primitives for Kohjoh's offline font pipeline.

One BDF parser, one mask model, one packer and one PICA tiler live here so the
six scripts do not each grow their own. Everything is deterministic: given the
same inputs the same bytes are produced, and the atlas is byte-stable across
runs and Python versions (no dict-order or float input reaches the output).

Standard library only for parsing, JSON and binary; Pillow only for PNG
previews and specimens, imported lazily so the generator still runs where
Pillow is absent and only previews are requested.
"""

from __future__ import annotations

import hashlib
import json
import os
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Sequence

REPO = Path(__file__).resolve().parents[2]
FONT_DIR = REPO / "assets" / "fonts"
BDF_PATH = FONT_DIR / "galmuri14" / "Galmuri14.bdf"
OVERRIDE_DIR = FONT_DIR / "overrides"
GENERATED = FONT_DIR / "generated"
CHARMAP_PATH = REPO / "build" / "upstream" / "charmap.txt"
CHARCODE_PATH = REPO / "build" / "upstream" / "include" / "constants" / "charcode.h"

# The DS text cell is 16 logical pixels tall; Galmuri14's baseline sits on row
# 15 of that cell (0-based), which leaves one pixel of cap padding and three
# for descenders before the 5/4 interface mapping. Recorded, reviewable, and
# used only for bearing maths; the game's own pen still decides placement.
BASELINE_IN_CELL = 15

# Atlas format constants. Bump ATLAS_VERSION whenever the binary layout or the
# meaning of a record changes; the runtime refuses a mismatched version.
ATLAS_MAGIC = b"KJAT"
ATLAS_VERSION = 1
ATLAS_PAGE_W = 256
ATLAS_PAGE_H = 256
ATLAS_PADDING = 1


# --------------------------------------------------------------------------- bdf


@dataclass(frozen=True)
class BdfGlyph:
    encoding: int
    name: str
    dwidth: int
    width: int
    height: int
    xoff: int
    yoff: int
    rows: tuple[tuple[int, ...], ...]

    @property
    def top_above_baseline(self) -> int:
        """BDF y-offset is baseline-to-bottom; top = -(yoff + h)."""
        return -(self.yoff + self.height)

    def ink_rows(self) -> tuple[tuple[int, ...], ...]:
        return self.rows


@dataclass(frozen=True)
class Bdf:
    path: Path
    sha256: str
    ascent: int
    descent: int
    point_size: int
    x_height: int
    cap_height: int
    bbox: tuple[int, int, int, int]
    glyphs: dict[int, BdfGlyph]
    copyright: str


def decode_bitmap_row(hex_row: str, width: int) -> tuple[int, ...]:
    """Leftmost bit is the leftmost pixel; trailing byte padding is ignored."""
    value = int(hex_row, 16) if hex_row else 0
    bit_count = len(hex_row) * 4
    return tuple((value >> (bit_count - 1 - x)) & 1 for x in range(width))


def parse_bdf(path: Path = BDF_PATH) -> Bdf:
    raw = path.read_bytes()
    text = raw.decode("utf-8")
    props: dict[str, str] = {}
    glyphs: dict[int, BdfGlyph] = {}
    bbox = (0, 0, 0, 0)
    ascent = descent = point_size = x_height = cap_height = 0

    lines = text.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith("FONTBOUNDINGBOX "):
            _, w, h, xo, yo = line.split()
            bbox = (int(w), int(h), int(xo), int(yo))
        elif line.startswith("FONT_ASCENT "):
            ascent = int(line.split()[1])
        elif line.startswith("FONT_DESCENT "):
            descent = int(line.split()[1])
        elif line.startswith("POINT_SIZE "):
            point_size = int(line.split()[1])
        elif line.startswith("X_HEIGHT "):
            x_height = int(line.split()[1])
        elif line.startswith("CAP_HEIGHT "):
            cap_height = int(line.split()[1])
        elif line.startswith("COPYRIGHT "):
            props["copyright"] = line[len("COPYRIGHT ") :].strip('"')
        elif line.startswith("STARTCHAR "):
            name = line[len("STARTCHAR ") :]
            enc = None
            dwidth = 0
            gw = gh = gx = gy = 0
            rows: list[tuple[int, ...]] = []
            i += 1
            while i < len(lines) and not lines[i].startswith("ENDCHAR"):
                cur = lines[i]
                if cur.startswith("ENCODING "):
                    enc = int(cur.split()[1])
                elif cur.startswith("DWIDTH "):
                    dwidth = int(cur.split()[1])
                elif cur.startswith("BBX "):
                    _, w, h, xo, yo = cur.split()
                    gw, gh, gx, gy = int(w), int(h), int(xo), int(yo)
                elif cur.startswith("BITMAP"):
                    i += 1
                    while i < len(lines) and not lines[i].startswith("ENDCHAR"):
                        rows.append(decode_bitmap_row(lines[i].strip(), gw))
                        i += 1
                    break
                i += 1
            if enc is not None and enc >= 0:
                glyphs[enc] = BdfGlyph(enc, name, dwidth, gw, gh, gx, gy, tuple(rows))
        i += 1

    return Bdf(
        path=path,
        sha256=hashlib.sha256(raw).hexdigest(),
        ascent=ascent,
        descent=descent,
        point_size=point_size,
        x_height=x_height,
        cap_height=cap_height,
        bbox=bbox,
        glyphs=glyphs,
        copyright=props.get("copyright", ""),
    )


# ------------------------------------------------------------------------ masks


@dataclass(frozen=True)
class Mask:
    """A one-bit glyph mask in top-down, left-to-right order."""

    width: int
    height: int
    rows: tuple[tuple[int, ...], ...]

    @classmethod
    def from_bdf(cls, g: BdfGlyph) -> "Mask":
        if g.height == 0 or g.width == 0:
            return cls(0, 0, ())
        return cls(g.width, g.height, g.rows)

    @classmethod
    def from_text(cls, text: str) -> "Mask":
        rows = []
        for line in text.splitlines():
            line = line.rstrip("\n")
            if not line or line.startswith(";"):
                continue
            rows.append(tuple(1 if ch in "#X1" else 0 for ch in line))
        width = max((len(r) for r in rows), default=0)
        return cls(width, len(rows), tuple(r + (0,) * (width - len(r)) for r in rows))

    def ink_bounds(self) -> tuple[int, int, int, int] | None:
        xs = [x for y, row in enumerate(self.rows) for x, v in enumerate(row) if v]
        if not xs:
            return None
        ys = [y for y, row in enumerate(self.rows) for v in row if v]
        return min(xs), min(ys), max(xs) + 1, max(ys) + 1

    def cropped(self) -> "Mask":
        b = self.ink_bounds()
        if b is None:
            return Mask(0, 0, ())
        x0, y0, x1, y1 = b
        return Mask(x1 - x0, y1 - y0, tuple(row[x0:x1] for row in self.rows[y0:y1]))

    def shifted(self, dx: int, dy: int) -> "Mask":
        w = self.width + abs(dx)
        h = self.height + abs(dy)
        sx = max(dx, 0)
        sy = max(dy, 0)
        rows = [[0] * w for _ in range(h)]
        for y in range(self.height):
            for x in range(self.width):
                if self.rows[y][x]:
                    rows[y + sy][x + sx] = 1
        return Mask(w, h, tuple(tuple(r) for r in rows))

    def union(self, other: "Mask") -> "Mask":
        w = max(self.width, other.width)
        h = max(self.height, other.height)
        rows = []
        for y in range(h):
            row = []
            for x in range(w):
                a = self.rows[y][x] if y < self.height and x < self.width else 0
                b = other.rows[y][x] if y < other.height and x < other.width else 0
                row.append(a | b)
            rows.append(tuple(row))
        return Mask(w, h, tuple(rows))

    def minus(self, other: "Mask") -> "Mask":
        w = max(self.width, other.width)
        h = max(self.height, other.height)
        rows = []
        for y in range(h):
            row = []
            for x in range(w):
                a = self.rows[y][x] if y < self.height and x < self.width else 0
                b = other.rows[y][x] if y < other.height and x < other.width else 0
                row.append(1 if a and not b else 0)
            rows.append(tuple(row))
        return Mask(w, h, tuple(rows))

    def fingerprint(self) -> str:
        h = hashlib.sha256()
        h.update(f"{self.width}x{self.height}\n".encode())
        for row in self.rows:
            h.update(bytes((1 if v else 0) for v in row))
        return h.hexdigest()


def mask_to_png(mask: Mask, path: Path, scale: int = 1, fg=(255, 255, 255, 255),
                bg=(0, 0, 0, 0)) -> None:
    from PIL import Image

    img = Image.new("RGBA", (max(mask.width, 1), max(mask.height, 1)), bg)
    px = img.load()
    for y, row in enumerate(mask.rows):
        for x, v in enumerate(row):
            if v:
                px[x, y] = fg
    if scale != 1:
        img = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path)


# ---------------------------------------------------------------------- scope

# Symmetric symbols and punctuation that the targeted screens use and that the
# Latin repertoire needs even though they live outside the Latin blocks.
EXTRA_REQUIRED = {
    0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A7, 0x00A9, 0x00AB, 0x00AE,
    0x00B0, 0x00B1, 0x00B5, 0x00B6, 0x00B7, 0x00BB, 0x00BF,
    0x00B9, 0x00B2, 0x00B3, 0x00AA, 0x00BA, 0x00D7, 0x00F7,
    0x2010, 0x2013, 0x2014, 0x2018, 0x2019, 0x201A, 0x201C, 0x201D,
    0x201E, 0x2020, 0x2021, 0x2022, 0x2026, 0x2030, 0x2039, 0x203A,
    0x2032, 0x2033, 0x20A6, 0x20A7, 0x20AC, 0x20B1,
    0x2116, 0x2122, 0x2113,
    0x2190, 0x2191, 0x2192, 0x2193, 0x2196, 0x2197, 0x2198, 0x2199,
    0x21D0, 0x21D1, 0x21D2, 0x21D3,
    0x25A0, 0x25A1, 0x25B2, 0x25B3, 0x25BC, 0x25BD, 0x25C6, 0x25C7,
    0x25C9, 0x25CB, 0x25CF, 0x25E6,
    0x2600, 0x2601, 0x2602, 0x2603, 0x2605, 0x2606, 0x2639, 0x263A,
    0x263D, 0x263E, 0x2640, 0x2642, 0x265A, 0x265B, 0x2660, 0x2663,
    0x2665, 0x2666, 0x266A, 0x266B, 0x2705, 0x2713, 0x2717,
    0x300A, 0x300B,
}


def scope_of(entry) -> str | None:
    """Classify a charmap entry's target scope.

    Returns None when the code belongs to the required Latin repertoire, or a
    short reason string when it is an explicit fallback. The Japanese and
    Korean editions are separate character sets and stay on the legacy
    renderer by the brief's scope; their codes must not make the Latin
    coverage report look incomplete.
    """
    name = entry.get("charcode_name") or ""
    if name.startswith("CHAR_JP_"):
        return "japanese_edition_only"
    u = entry.get("unicode")
    if u is None:
        return "not_a_character"
    if 0x1100 <= u <= 0x11FF:
        return "korean_edition_only"
    if u >= 0x3000 and u not in (0x300A, 0x300B):
        return "non_latin_script"
    if 0x20 <= u <= 0x24F:
        return None
    if u in EXTRA_REQUIRED:
        return None
    if 0x2000 <= u <= 0x206F or 0x20A0 <= u <= 0x20CF:
        return None
    if 0x2190 <= u <= 0x21FF:
        return None
    if 0x25A0 <= u <= 0x26FF:
        return None
    return "outside_latin_repertoire"


# ------------------------------------------------------------------------ JSON


def load_json(path: Path):
    with path.open("r", encoding="utf-8") as fh:
        return json.load(fh)


def dump_json(path: Path, obj) -> None:
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    text = json.dumps(obj, indent=2, ensure_ascii=False, sort_keys=False) + "\n"
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(text, encoding="utf-8", newline="\n")
    os.replace(tmp, path)


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# ---------------------------------------------------------------------- packing


@dataclass
class Packed:
    key: str
    page: int
    x: int
    y: int
    w: int
    h: int


def pack_shelves(items: Sequence[tuple[str, int, int]], page_w: int, page_h: int,
                 padding: int) -> tuple[list[Packed], int]:
    """Deterministic shelf packing. Items keep the caller's order.

    Returns the placements and the page count. Raises if an item cannot fit a
    fresh page, which the caller reports as a generator failure rather than
    silently dropping a glyph.
    """
    out: list[Packed] = []
    page = 0
    x = padding
    y = padding
    shelf_h = 0
    for key, w, h in items:
        if w + 2 * padding > page_w or h + 2 * padding > page_h:
            raise ValueError(f"glyph {key} ({w}x{h}) does not fit a {page_w}x{page_h} page")
        if x + w + padding > page_w:
            x = padding
            y += shelf_h + padding
            shelf_h = 0
        if y + h + padding > page_h:
            page += 1
            x = padding
            y = padding
            shelf_h = 0
        out.append(Packed(key, page, x, y, w, h))
        x += w + padding
        shelf_h = max(shelf_h, h)
    return out, page + 1 if out else 0


# ------------------------------------------------------------------- pica tiling


def pica_tile_rgba5551(texels: Sequence[int], width: int, height: int) -> bytes:
    """Reorder a row-major 16-bit texture into PICA200's 8x8 tile layout.

    Tiles are stored row-major across the page; inside a tile the texels are
    in Morton order with x in the even bits and y in the odd bits (the same
    rule as src/graphics/resources/atlas.c)."""
    if width % 8 or height % 8:
        raise ValueError("PICA texture dimensions must be multiples of 8")
    out = bytearray(width * height * 2)
    tiles_x = width // 8

    def morton(x: int, y: int) -> int:
        return ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) |
                ((x & 4) << 2) | ((y & 4) << 3))

    for ty in range(height // 8):
        for tx in range(tiles_x):
            tile_index = ty * tiles_x + tx
            base = tile_index * 64 * 2
            for py in range(8):
                for px in range(8):
                    sx = tx * 8 + px
                    sy = ty * 8 + py
                    v = texels[sy * width + sx]
                    off = base + morton(px, py) * 2
                    out[off] = v & 0xFF
                    out[off + 1] = (v >> 8) & 0xFF
    return bytes(out)


def rgba5551(alpha: int, white: bool = True) -> int:
    """White texel with the given 1-bit alpha (0 or 1).

    The PICA's RGBA5551 word stores the 1-bit alpha in the LSB and the five
    colour bits above it, so opaque white is 0xFFFF (not 0x7FFF). Getting
    this wrong tints every glyph cyan."""
    base = 0xFFFE if white else 0
    return base | (1 if alpha else 0)
