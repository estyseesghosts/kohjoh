#!/usr/bin/env python3
"""Resolve the required HGSS repertoire into native glyph records.

Inputs:
  assets/fonts/generated/charmap.json
  assets/fonts/galmuri14/Galmuri14.bdf
  assets/fonts/overrides/glyphs.json + masks/*.txt

Output:
  assets/fonts/generated/glyphs.json

For every required (font, HGSS code) pair this records the source of the
artwork, the original logical advance where the font provides one, the native
ink bounds and the top-down bearing relative to the DS text cell. Missing or
unsupported characters become explicit fallback records; they are never
silently dropped and never replaced with an ASCII lookalike.

Priority (task 3E): upstream Galmuri14 mask, then reviewed override mask,
then an explicit fallback. There is no distortion or antialiased resampling
anywhere in this step.
"""

from __future__ import annotations

import sys

from common import (
    BASELINE_IN_CELL,
    Mask,
    scope_of,
    BDF_PATH,
    GENERATED,
    OVERRIDE_DIR,
    dump_json,
    load_json,
    parse_bdf,
)

# Fonts 0..4 are the 16-row variable-width HGSS faces; font 5 is the fixed
# width face. 255 is the pseudo-font used for the specialised numeric bitmap
# printers (message_printer.c / battle HP boxes) which bypass the text font.
VARIABLE_FONTS = (0, 1, 2, 3, 4)
FIXED_FONT = 5
NUMERIC_FONT = 255
NUMERIC_CODES = set(range(0x30, 0x3A)) | {0x2F, 0x2E, 0x2D, 0x2C, 0x25, 0x20}
NUMERIC_CELL = 10

# Anything at or above this is outside the targeted Latin editions
# (CJK, kana, full-width forms). Those stay on the legacy renderer by scope.
NON_LATIN_START = 0x3000


def is_required(entry):
    return entry["kind"] == "char" and scope_of(entry) is None


def build_artwork(bdf, charmap, overrides):
    """Unique one-bit masks keyed by fingerprint, plus code -> artwork id."""
    artwork = {}
    by_code = {}

    def add(mask, source, advance, bearing_x, yoff, glyph_meta):
        key = mask.fingerprint()
        if key not in artwork:
            artwork[key] = {
                "id": key[:16],
                "fingerprint": key,
                "width": mask.width,
                "height": mask.height,
                "rows": ["".join(str(v) for v in row) for row in mask.rows],
                "sources": [],
            }
        if source not in artwork[key]["sources"]:
            artwork[key]["sources"].append(source)
        by_code[glyph_meta["hgss_code"]] = {
            "art": key[:16],
            "advance": advance,
            "bearing_x": bearing_x,
            "yoff": yoff,
            "source": source,
        }

    for entry in charmap["entries"]:
        code = entry["hgss_code"]
        if not is_required(entry):
            continue
        u = entry["unicode"]
        if u is None or scope_of(entry) is not None:
            continue
        if code in overrides["by_code"]:
            continue
        g = bdf.glyphs.get(u)
        if g is None:
            continue
        mask = Mask.from_bdf(g)
        add(mask, f"galmuri14:U+{u:04X}", g.dwidth, g.xoff, g.yoff, entry)

    for code, ov in sorted(overrides["by_code"].items()):
        mask = ov["mask"]
        entry = next(e for e in charmap["entries"] if e["hgss_code"] == code)
        add(
            mask,
            f"override:{ov['source']}",
            ov["advance"],
            ov["bearing_x"],
            ov["baseline_offset"],
            entry,
        )

    return artwork, by_code


def main() -> int:
    bdf = parse_bdf(BDF_PATH)
    charmap = load_json(GENERATED / "charmap.json")
    override_doc = load_json(OVERRIDE_DIR / "glyphs.json")

    overrides = {"by_code": {}}
    for g in override_doc["glyphs"]:
        source = OVERRIDE_DIR / g["source"]
        text = source.read_text(encoding="utf-8")
        # Reuse the BDF-style text mask parser.
        from common import Mask

        mask = Mask.from_text(text)
        overrides["by_code"][g["hgss_code"]] = {
            "mask": mask,
            "source": g["source"],
            "advance": g["advance"],
            "bearing_x": g["bearing_x"],
            "baseline_offset": g["baseline_offset"],
            "reason": g["reason"],
        }

    artwork, by_code = build_artwork(bdf, charmap, overrides)

    glyphs = []
    fallbacks = []
    variants_available = ["standard", "numeric"]

    def emit(font_id, code, variant, art_info, cell=None):
        entry = next(e for e in charmap["entries"] if e["hgss_code"] == code)
        mask = next(a for a in artwork.values() if a["id"] == art_info["art"])
        w, h = mask["width"], mask["height"]
        bearing_x = art_info["bearing_x"]
        advance = art_info["advance"]
        if cell is not None:
            bearing_x += (cell - advance) // 2
            advance = cell
        top = -(art_info["yoff"] + h) if h else 0
        glyphs.append(
            {
                "font_id": font_id,
                "hgss_code": code,
                "unicode": entry["unicode"],
                "charcode_name": entry.get("charcode_name"),
                "variant": variant,
                "art": art_info["art"],
                "source": art_info["source"],
                "advance": advance,
                "cell": advance,
                "bearing_x": bearing_x,
                "bearing_y": BASELINE_IN_CELL + top,
                "ink_w": w,
                "ink_h": h,
                "top_above_baseline": top,
            }
        )

    for entry in charmap["entries"]:
        code = entry["hgss_code"]
        u = entry["unicode"]
        if entry["kind"] != "char" or u is None:
            continue
        scope = scope_of(entry)
        if scope is not None:
            fallbacks.append(
                {
                    "hgss_code": code,
                    "unicode": u,
                    "reason": scope,
                    "detail": "outside the targeted Latin repertoire; stays on the legacy renderer",
                }
            )
            continue
        if code not in by_code:
            fallbacks.append(
                {
                    "hgss_code": code,
                    "unicode": u,
                    "reason": "missing_in_galmuri14",
                    "detail": f"U+{u:04X} has no glyph and no override",
                }
            )
            continue
        info = by_code[code]
        for font_id in VARIABLE_FONTS:
            emit(font_id, code, "standard", info)
        emit(FIXED_FONT, code, "standard", info, cell=NUMERIC_CELL)
        if code in NUMERIC_CODES:
            emit(0, code, "numeric", info, cell=NUMERIC_CELL)
            emit(NUMERIC_FONT, code, "numeric", info, cell=NUMERIC_CELL)

    doc = {
        "schema": 1,
        "pipeline": 1,
        "source_font": {
            "path": "assets/fonts/galmuri14/Galmuri14.bdf",
            "sha256": bdf.sha256,
            "copyright": bdf.copyright,
            "metrics": {
                "ascent": bdf.ascent,
                "descent": bdf.descent,
                "cap_height": bdf.cap_height,
                "x_height": bdf.x_height,
                "line_box": bdf.ascent + bdf.descent,
            },
        },
        "baseline_in_cell": BASELINE_IN_CELL,
        "numeric_cell": NUMERIC_CELL,
        "fonts": {
            "variable": list(VARIABLE_FONTS),
            "fixed": FIXED_FONT,
            "numeric_pseudo": NUMERIC_FONT,
        },
        "variants_available": variants_available,
        "compact_variant": {
            "status": "pending_review",
            "reason": "A compact face is only justified by measured HGSS advances that "
            "Galmuri14's ink exceeds. Those advances come from the running game, so "
            "compact masks are generated after Task 4 measurement and maintainer review; "
            "no unapproved squeezed glyph is shipped.",
        },
        "artwork": sorted(artwork.values(), key=lambda a: a["id"]),
        "glyphs": glyphs,
        "fallbacks": fallbacks,
        "counts": {
            "artwork": len(artwork),
            "glyphs": len(glyphs),
            "fallbacks": len(fallbacks),
        },
    }
    dump_json(GENERATED / "glyphs.json", doc)
    print(
        f"glyphs: {len(artwork)} unique masks, {len(glyphs)} (font,code,variant) records, "
        f"{len(fallbacks)} explicit fallbacks -> {GENERATED / 'glyphs.json'}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())



