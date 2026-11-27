#!/usr/bin/env python3
"""Pack the resolved glyph masks into the runtime atlas.

Inputs:  assets/fonts/generated/glyphs.json
Outputs: assets/fonts/generated/kohjoh_text_atlas.bin   (runtime)
         assets/fonts/generated/atlas.json             (human + tests)
         assets/fonts/generated/atlas_preview_1x.png
         assets/fonts/generated/atlas_preview_4x.png

Format: RGBA5551, white RGB with one-bit coverage alpha, PICA200 8x8 tiled,
one or more 256x256 pages. White-plus-alpha is the doc's first working
material: it tints correctly with vertex colour and needs no TEV experiment
before there is a correct picture. The binary is deliberately dumb -- fixed
records, no compression, no pointers -- so the loader is a bounds check and a
memcpy.

Determinism: artwork is sorted by fingerprint before packing, the shelf packer
is ordered, and no timestamps or absolute paths enter the output.
"""

from __future__ import annotations

import struct
import sys

from common import (
    ATLAS_MAGIC,
    ATLAS_PADDING,
    ATLAS_PAGE_H,
    ATLAS_PAGE_W,
    ATLAS_VERSION,
    BASELINE_IN_CELL,
    GENERATED,
    dump_json,
    load_json,
    pica_tile_rgba5551,
    pack_shelves,
    rgba5551,
    sha256_file,
)

HEADER_FMT = "<4sHHBBHHHHIIIIHBBHII"
HEADER_SIZE = struct.calcsize(HEADER_FMT)
ART_FMT = "<HHHBBBBII"
ART_SIZE = struct.calcsize(ART_FMT)
GLYPH_FMT = "<HHHHhhBBI"
GLYPH_SIZE = struct.calcsize(GLYPH_FMT)

FLAG_TILED = 0x1
FLAG_RGBA5551 = 0x2


def render_preview(pages, page_w, page_h, scale, path, bg=(18, 18, 24, 255)):
    from PIL import Image

    cols = len(pages)
    img = Image.new("RGBA", (page_w * cols, page_h), bg)
    for pi, texels in enumerate(pages):
        for y in range(page_h):
            for x in range(page_w):
                if texels[y * page_w + x] & 1:
                    img.putpixel((pi * page_w + x, y), (245, 245, 245, 255))
    if scale != 1:
        img = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path)


def main() -> int:
    doc = load_json(GENERATED / "glyphs.json")
    art = sorted(doc["artwork"], key=lambda a: a["id"])

    items = [(a["id"], a["width"], a["height"]) for a in art]
    placements, page_count = pack_shelves(items, ATLAS_PAGE_W, ATLAS_PAGE_H, ATLAS_PADDING)

    pages = [[0] * (ATLAS_PAGE_W * ATLAS_PAGE_H) for _ in range(page_count)]
    art_index = {}
    art_records = []
    for a, p in zip(art, placements):
        art_index[a["id"]] = len(art_records)
        for y, row in enumerate(a["rows"]):
            for x, ch in enumerate(row):
                if ch == "1":
                    px = p.x + x
                    py = p.y + y
                    if px >= ATLAS_PAGE_W or py >= ATLAS_PAGE_H:
                        raise ValueError(f"art {a['id']} escaped its page")
                    idx = py * ATLAS_PAGE_W + px
                    if pages[p.page][idx]:
                        raise ValueError(f"overlapping allocation at {px},{py}")
                    pages[p.page][idx] = rgba5551(1)
        art_records.append(
            (
                p.page,
                p.x,
                p.y,
                a["width"],
                a["height"],
                0,
                0,
                int(a["fingerprint"][:8], 16),
                0,
            )
        )

    glyph_records = []
    for g in doc["glyphs"]:
        idx = art_index.get(g["art"])
        if idx is None:
            raise ValueError(f"glyph {g['font_id']}:{g['hgss_code']} has no artwork")
        glyph_records.append(
            (
                g["font_id"],
                g["hgss_code"],
                idx,
                g["advance"],
                g["bearing_x"],
                g["bearing_y"],
                0 if g["variant"] == "standard" else 1,
                0,
                g["unicode"],
            )
        )

    page_bytes = b"".join(
        pica_tile_rgba5551(p, ATLAS_PAGE_W, ATLAS_PAGE_H) for p in pages
    )

    art_off = HEADER_SIZE
    glyph_off = art_off + len(art_records) * ART_SIZE
    pages_off = glyph_off + len(glyph_records) * GLYPH_SIZE
    total = pages_off + len(page_bytes)

    font_sha = bytes.fromhex(doc["source_font"]["sha256"][:8])
    header = struct.pack(
        HEADER_FMT,
        ATLAS_MAGIC,
        ATLAS_VERSION,
        HEADER_SIZE,
        page_count,
        FLAG_TILED | FLAG_RGBA5551,
        ATLAS_PAGE_W,
        ATLAS_PAGE_H,
        len(art_records),
        len(glyph_records),
        art_off,
        glyph_off,
        pages_off,
        total,
        BASELINE_IN_CELL,
        doc["numeric_cell"],
        2,
        0,
        int.from_bytes(font_sha, "little"),
        0,
    )
    assert len(header) == HEADER_SIZE

    blob = bytearray()
    blob += header
    for r in art_records:
        blob += struct.pack(ART_FMT, *r)
    for r in glyph_records:
        blob += struct.pack(GLYPH_FMT, *r)
    blob += page_bytes
    assert len(blob) == total

    out_bin = GENERATED / "kohjoh_text_atlas.bin"
    out_bin.write_bytes(blob)

    dump_json(
        GENERATED / "atlas.json",
        {
            "schema": 1,
            "format": {
                "magic": ATLAS_MAGIC.decode(),
                "version": ATLAS_VERSION,
                "header_size": HEADER_SIZE,
                "art_record_size": ART_SIZE,
                "glyph_record_size": GLYPH_SIZE,
                "pixel_format": "RGBA5551",
                "layout": "pica200 8x8 tiles, row-major within tile, tiles row-major",
                "page_width": ATLAS_PAGE_W,
                "page_height": ATLAS_PAGE_H,
                "pages": page_count,
                "bytes": total,
                "sha256": sha256_file(out_bin),
            },
            "baseline_in_cell": BASELINE_IN_CELL,
            "numeric_cell": doc["numeric_cell"],
            "art": [
                {
                    "index": i,
                    "id": a["id"],
                    "fingerprint": a["fingerprint"],
                    "page": p.page,
                    "x": p.x,
                    "y": p.y,
                    "w": a["width"],
                    "h": a["height"],
                }
                for i, (a, p) in enumerate(zip(art, placements))
            ],
            "glyphs": [
                {**{k: g[k] for k in ("font_id", "hgss_code", "unicode", "variant",
                                      "advance", "bearing_x", "bearing_y")},
                 "art_index": art_index[g["art"]]}
                for g in doc["glyphs"]
            ],
            "counts": {
                "art": len(art_records),
                "glyphs": len(glyph_records),
                "pages": page_count,
            },
        },
    )

    render_preview(pages, ATLAS_PAGE_W, ATLAS_PAGE_H, 1, GENERATED / "atlas_preview_1x.png")
    render_preview(pages, ATLAS_PAGE_W, ATLAS_PAGE_H, 4, GENERATED / "atlas_preview_4x.png")

    print(
        f"atlas: {len(art_records)} masks, {len(glyph_records)} glyph records, "
        f"{page_count} page(s), {total} bytes -> {out_bin.name}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
