#!/usr/bin/env python3
"""Validate the generated native font assets and write the coverage report.

Checks, in order:
  1. source pin: BDF hash matches the one recorded by extract_bdf;
  2. completeness: every required Latin (font, code) pair resolves to artwork;
  3. metrics: bearings keep every mask inside a sane cell window and every
     advance is positive;
  4. packing: no overlap, no out-of-page art, every glyph's art exists;
  5. binary: magic/version/size agree, records decode, and every art mask read
     back through the PICA tiling matches the source mask bit for bit.

Exit status is non-zero if a required glyph is missing or the binary does not
round-trip. Unsupported non-Latin characters are accounted for as explicit
fallback records and do not fail the build.
"""

from __future__ import annotations

import struct
import sys

from common import (
    ATLAS_MAGIC,
    ATLAS_VERSION,
    BDF_PATH,
    GENERATED,
    dump_json,
    load_json,
    scope_of,
    parse_bdf,
    sha256_file,
)

HEADER_FMT = "<4sHHBBHHHHIIIIHBBHII"
ART_FMT = "<HHHBBBBII"
GLYPH_FMT = "<HHHHhhBBI"

REQUIRED_FONTS = [0, 1, 2, 3, 4, 5]


def untile_rgba5551(blob, page_w, page_h):
    texels = [0] * (page_w * page_h)
    tiles_x = page_w // 8

    def morton(x, y):
        return ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) |
                ((x & 4) << 2) | ((y & 4) << 3))

    for ty in range(page_h // 8):
        for tx in range(tiles_x):
            base = (ty * tiles_x + tx) * 64 * 2
            for py in range(8):
                for px in range(8):
                    off = base + morton(px, py) * 2
                    v = blob[off] | (blob[off + 1] << 8)
                    texels[(ty * 8 + py) * page_w + tx * 8 + px] = v
    return texels


def main() -> int:
    errors = []
    warnings = []

    bdf = parse_bdf(BDF_PATH)
    glyphs_doc = load_json(GENERATED / "glyphs.json")
    atlas_doc = load_json(GENERATED / "atlas.json")
    charmap = load_json(GENERATED / "charmap.json")

    # 1. Source pin.
    if glyphs_doc["source_font"]["sha256"] != bdf.sha256:
        errors.append(
            f"BDF hash changed: pipeline has {glyphs_doc['source_font']['sha256']}, "
            f"file is {bdf.sha256}"
        )

    # 2. Completeness of the required repertoire.
    present = {(g["font_id"], g["hgss_code"], g["variant"]) for g in glyphs_doc["glyphs"]}
    required_codes = []
    for e in charmap["entries"]:
        if e["kind"] != "char" or scope_of(e) is not None:
            continue
        required_codes.append(e["hgss_code"])
    missing = []
    for font_id in REQUIRED_FONTS:
        for code in required_codes:
            if (font_id, code, "standard") not in present:
                missing.append((font_id, code))
    if missing:
        errors.append(f"{len(missing)} required (font,code) pairs missing, e.g. {missing[:8]}")

    # 3. Metrics.
    art_by_id = {a["id"]: a for a in glyphs_doc["artwork"]}
    for g in glyphs_doc["glyphs"]:
        a = art_by_id[g["art"]]
        if g["advance"] <= 0:
            errors.append(f"glyph {g['font_id']}:{g['hgss_code']} has advance {g['advance']}")
        if g["bearing_y"] + a["height"] > 22:
            warnings.append(
                f"glyph {g['font_id']}:{g['hgss_code']} descends to row "
                f"{g['bearing_y'] + a['height']} (cell is 16, 22 allowed with accent/descender)"
            )
        if g["bearing_y"] < -4:
            warnings.append(
                f"glyph {g['font_id']}:{g['hgss_code']} starts at row {g['bearing_y']} "
                "(above the cell; must be reviewed at Task 4)"
            )

    # 4. Packing.
    seen = {}
    for a in atlas_doc["art"]:
        for y in range(a["y"], a["y"] + a["h"]):
            for x in range(a["x"], a["x"] + a["w"]):
                key = (a["page"], x, y)
                if key in seen and seen[key] != a["id"]:
                    errors.append(f"overlap at page {a['page']} {x},{y}: {seen[key]} / {a['id']}")
                seen[key] = a["id"]
        if a["x"] + a["w"] > atlas_doc["format"]["page_width"]:
            errors.append(f"art {a['id']} exceeds page width")
        if a["y"] + a["h"] > atlas_doc["format"]["page_height"]:
            errors.append(f"art {a['id']} exceeds page height")

    # 5. Binary round-trip.
    blob = (GENERATED / "kohjoh_text_atlas.bin").read_bytes()
    hdr = struct.unpack_from(HEADER_FMT, blob, 0)
    (magic, version, header_size, page_count, flags, page_w, page_h, art_count,
     glyph_count, art_off, glyph_off, pages_off, total, baseline, numeric_cell,
     variants, reserved, font_sha, pad) = hdr
    if magic != ATLAS_MAGIC:
        errors.append(f"bad magic {magic!r}")
    if version != ATLAS_VERSION:
        errors.append(f"bad version {version}")
    if header_size != struct.calcsize(HEADER_FMT):
        errors.append(f"bad header size {header_size}")
    if total != len(blob):
        errors.append(f"declared size {total} != file size {len(blob)}")
    if font_sha != int.from_bytes(bytes.fromhex(bdf.sha256[:8]), "little"):
        errors.append("binary font hash disagrees with the pinned BDF")

    art_records = [
        struct.unpack_from(ART_FMT, blob, art_off + i * struct.calcsize(ART_FMT))
        for i in range(art_count)
    ]
    glyph_records = [
        struct.unpack_from(GLYPH_FMT, blob, glyph_off + i * struct.calcsize(GLYPH_FMT))
        for i in range(glyph_count)
    ]

    pages = []
    for p in range(page_count):
        off = pages_off + p * page_w * page_h * 2
        pages.append(untile_rgba5551(blob[off : off + page_w * page_h * 2], page_w, page_h))

    # The runtime binary-searches glyph records by (font, code, variant).
    keys = [(f, c, v) for (f, c, _a, _adv, _bx, _by, v, _fl, _u) in glyph_records]
    if keys != sorted(keys):
        errors.append("glyph records are not sorted by (font, code, variant)")
    if len(set(keys)) != len(keys):
        errors.append("duplicate (font, code, variant) glyph records")

    art_ids = [a["id"] for a in sorted(glyphs_doc["artwork"], key=lambda a: a["id"])]
    mismatched = 0
    for idx, (page, x, y, w, h, aflags, ares, fp, apad) in enumerate(art_records):
        source = art_by_id[art_ids[idx]]
        for yy, row in enumerate(source["rows"]):
            for xx, ch in enumerate(row):
                got = pages[page][(y + yy) * page_w + (x + xx)] & 1
                want = 1 if ch == "1" else 0
                if got != want:
                    mismatched += 1
    if mismatched:
        errors.append(f"{mismatched} atlas texels do not match their source mask")

    # Coverage report.
    place_by_id = {a["id"]: a for a in atlas_doc["art"]}
    rows = []
    fallback_by_code = {f["hgss_code"]: f for f in glyphs_doc["fallbacks"]}
    for e in charmap["entries"]:
        code = e["hgss_code"]
        if e["kind"] != "char":
            rows.append(
                {
                    "hgss_code": f"0x{code:04X}",
                    "character": e.get("token"),
                    "status": e["kind"],
                    "source": None,
                }
            )
            continue
        u = e["unicode"]
        g = next((g for g in glyphs_doc["glyphs"]
                  if g["font_id"] == 0 and g["hgss_code"] == code and g["variant"] == "standard"), None)
        if e["kind"] == "char" and scope_of(e) is not None:
            rows.append(
                {
                    "hgss_code": f"0x{code:04X}",
                    "character": chr(u),
                    "status": "fallback",
                    "reason": scope_of(e),
                    "source": None,
                }
            )
        elif g is None:
            fb = fallback_by_code.get(code)
            rows.append(
                {
                    "hgss_code": f"0x{code:04X}",
                    "character": chr(u) if u else "",
                    "status": "fallback",
                    "reason": fb["reason"] if fb else "unknown",
                    "source": None,
                }
            )
        else:
            a = art_by_id[g["art"]]
            place = place_by_id[g["art"]]
            rows.append(
                {
                    "hgss_code": f"0x{code:04X}",
                    "character": chr(u) if u else "",
                    "status": "native",
                    "source": g["source"],
                    "advance": g["advance"],
                    "ink": [a["width"], a["height"]],
                    "bearing": [g["bearing_x"], g["bearing_y"]],
                    "atlas": [place["page"], place["x"], place["y"]],
                    "override": g["source"].startswith("override:"),
                }
            )

    native = sum(1 for r in rows if r["status"] == "native")
    fallback = sum(1 for r in rows if r["status"] == "fallback")
    dump_json(
        GENERATED / "coverage.json",
        {
            "schema": 1,
            "source_font_sha256": bdf.sha256,
            "atlas_sha256": sha256_file(GENERATED / "kohjoh_text_atlas.bin"),
            "native": native,
            "fallback": fallback,
            "errors": errors,
            "warnings": warnings,
            "rows": rows,
        },
    )

    md = [
        "# Native glyph coverage",
        "",
        "Generated by `tools/fonts/validate_atlas.py`. Do not edit.",
        "",
        f"- Source font SHA-256: `{bdf.sha256}`",
        f"- Atlas SHA-256: `{sha256_file(GENERATED / 'kohjoh_text_atlas.bin')}`",
        f"- Atlas pages: {page_count} x {page_w}x{page_h} RGBA5551 "
        f"({len(blob)} bytes)",
        f"- Unique masks: {art_count}; runtime glyph records: {glyph_count}",
        f"- Character codes with native artwork: {native}",
        f"- Character codes on explicit fallback: {fallback}",
        "",
        "## Fallbacks",
        "",
        "Every non-Latin code in the pinned map is an explicit fallback: those",
        "editions and character sets stay on the original renderer by scope.",
        "Latin codes that the font does not carry are listed individually.",
        "",
        "| Range | Reason | Codes |",
        "| --- | --- | --- |",
    ]
    ranges = {}
    for r in rows:
        if r["status"] == "fallback" and r.get("reason") != "outside_latin_repertoire":
            code = int(r["hgss_code"], 16)
            ranges[(code // 0x100, r["reason"])] = ranges.get((code // 0x100, r["reason"]), 0) + 1
    for (block, reason), count in sorted(ranges.items()):
        md.append(f"| 0x{block:02X}00-0x{block:02X}FF | {reason} | {count} |")
    md += [
        "",
        "| Code | Char | Reason |",
        "| --- | --- | --- |",
    ]
    for r in rows:
        if r["status"] == "fallback" and r.get("reason") == "outside_latin_repertoire":
            ch = r["character"]
            if ch and (ord(ch) < 0x20 or ord(ch) == 0x7F):
                ch = "control"
            md.append(f"| {r['hgss_code']} | {ch} | {r['reason']} |")
    md += [
        "",
        "## Native repertoire",
        "",
        "| Code | Char | Advance | Ink | Bearing | Atlas | Override |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ]
    for r in rows:
        if r["status"] != "native":
            continue
        ch = r["character"]
        md.append(
            f"| {r['hgss_code']} | {ch} | {r['advance']} | "
            f"{r['ink'][0]}x{r['ink'][1]} | {r['bearing'][0]},{r['bearing'][1]} | "
            f"{r['atlas'][0]}:{r['atlas'][1]},{r['atlas'][2]} | "
            f"{'yes' if r['override'] else ''} |"
        )
    (GENERATED / "coverage.md").write_text("\n".join(md) + "\n", encoding="utf-8", newline="\n")

    print(
        f"validate: {art_count} masks, {glyph_count} records, {native} native codes, "
        f"{fallback} fallbacks, {len(warnings)} warnings, {len(errors)} errors"
    )
    for e in errors:
        print(f"  ERROR: {e}")
    for w in warnings[:10]:
        print(f"  warning: {w}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())

