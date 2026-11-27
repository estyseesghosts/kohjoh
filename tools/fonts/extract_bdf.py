#!/usr/bin/env python3
"""Dump the pinned Galmuri14 BDF to JSON with metrics and one-bit masks.

Output: assets/fonts/generated/bdf.json

This is the extract step of the pipeline: it proves what the upstream font
actually contains (strike, bearings, advance widths) and gives later stages a
stable, reviewable intermediate. No runtime component reads BDF.

By default only glyphs in the required Latin repertoire are dumped (a few
hundred, with rows); ``--all`` dumps all 19k glyphs for inspection but is not
committed. The font-level metrics and the file hash are always recorded.
"""

from __future__ import annotations

import argparse
import sys

from common import BDF_PATH, GENERATED, dump_json, parse_bdf, scope_of


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true", help="dump every glyph, not just the repertoire")
    ap.add_argument("--out", default=str(GENERATED / "bdf.json"))
    args = ap.parse_args()

    bdf = parse_bdf(BDF_PATH)

    wanted = None
    if not args.all:
        wanted = set()
        charmap_path = GENERATED / "charmap.json"
        if charmap_path.exists():
            from common import load_json

            for e in load_json(charmap_path)["entries"]:
                if e["kind"] == "char" and scope_of(e) is None:
                    wanted.add(e["unicode"])
        # Common symbols are also useful when the charmap is absent.
        wanted.update({0x00B0, 0x00D7, 0x00F7, 0x2026, 0x2018, 0x2019, 0x201C, 0x201D})
        wanted.update(range(0x20, 0x7F))

    glyphs = []
    for enc in sorted(bdf.glyphs):
        if wanted is not None and enc not in wanted:
            continue
        g = bdf.glyphs[enc]
        glyphs.append(
            {
                "encoding": enc,
                "name": g.name,
                "advance": g.dwidth,
                "bbx": [g.width, g.height, g.xoff, g.yoff],
                "top_above_baseline": g.top_above_baseline,
                "rows": ["".join(str(v) for v in row) for row in g.rows],
            }
        )

    doc = {
        "schema": 1,
        "source": "assets/fonts/galmuri14/Galmuri14.bdf",
        "sha256": bdf.sha256,
        "copyright": bdf.copyright,
        "metrics": {
            "ascent": bdf.ascent,
            "descent": bdf.descent,
            "point_size": bdf.point_size,
            "x_height": bdf.x_height,
            "cap_height": bdf.cap_height,
            "bounding_box": list(bdf.bbox),
            "line_box": bdf.ascent + bdf.descent,
        },
        "total_glyphs": len(bdf.glyphs),
        "dumped_glyphs": len(glyphs),
        "filter": "all" if args.all else "latin_repertoire",
        "glyphs": glyphs,
    }
    dump_json(args.out, doc)
    print(
        f"bdf: {len(glyphs)}/{len(bdf.glyphs)} glyphs, ascent {bdf.ascent} "
        f"descent {bdf.descent} cap {bdf.cap_height} x {bdf.x_height} -> {args.out}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
