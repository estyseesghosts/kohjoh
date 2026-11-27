#!/usr/bin/env python3
"""Parse the pinned HGSS character map into a classified JSON table.

Inputs (both from the pinned decompilation, never from a ROM):
  build/upstream/charmap.txt
  build/upstream/include/constants/charcode.h

Output: assets/fonts/generated/charmap.json

The table keeps HGSS character codes distinct from Unicode. A glyph ID is not
a Unicode codepoint, several codes can share one character (variant forms keep
different metrics), and a handful of codes are control codes, macro tokens or
multi-character graphical forms. Those are classified explicitly rather than
fed through ``ord()``.

The pinned charmap predates the pinned charcode.h for the final symbols:
charcode.h defines CHAR_PK=480/CHAR_MN=481 and CHAR_NARROW_SPACE..CHAR_TWO_DOTS
as 482..492, while charmap.txt still names 480/481 as currency signs and stops
at 481. ``charcode.h`` is the compiled authority, so the tail is taken from it
and the discrepancy is recorded in the output.
"""

from __future__ import annotations

import re
import sys

from common import CHARCODE_PATH, CHARMAP_PATH, GENERATED, dump_json, load_json

# Codes whose compiled meaning disagrees with the older charmap text.
CHARCODE_OVERRIDES = {
    480: ("PK", "POKEMON_PK"),
    481: ("MN", "POKEMON_MN"),
    482: ("\u2009", "NARROW_SPACE"),
    483: ("\u2502", "FILL1PX"),
    484: ("\u2502", "FILL2PX"),
    485: ("\u2502", "FILL4PX"),
    486: ("\u2502", "FILL8PX"),
    487: ("\u2502", "FILL16PX"),
    488: ("\u00b0", "DEGREE"),
    489: ("_", "UNDERSCORE"),
    490: ("_", "UNDERSCORE2"),
    491: (".", "ONE_DOT"),
    492: (".", "TWO_DOTS"),
}


def parse_charmap(path):
    entries = {}
    raw = path.read_bytes().decode("utf-8")
    for line in raw.splitlines():
        line = line.rstrip("\r\n")
        if not line or line.startswith("//"):
            continue
        code_hex, sep, payload = line.partition("=")
        if not sep:
            continue
        try:
            code = int(code_hex, 16)
        except ValueError:
            continue
        if payload.startswith("\\"):
            kind = "escape"
            detail = payload
            char = None
        elif payload.startswith("{") and payload.endswith("}"):
            kind = "command"
            detail = payload[1:-1]
            char = None
        elif len(payload) == 1:
            kind = "char"
            detail = None
            char = payload
        else:
            kind = "special"
            detail = payload
            char = None
        entries[code] = {
            "hgss_code": code,
            "kind": kind,
            "char": char,
            "token": detail,
        }
    return entries


def parse_charcode_names(path):
    names = {}
    text = path.read_text(encoding="utf-8", errors="replace")
    for m in re.finditer(r"^#define\s+(CHAR_[A-Za-z0-9_]+)\s+(\d+)\s*$", text, re.M):
        names[int(m.group(2))] = m.group(1)
    return names


def main() -> int:
    charmap = parse_charmap(CHARMAP_PATH)
    names = parse_charcode_names(CHARCODE_PATH)
    overrides_applied = []

    for code, (char, symbolic) in CHARCODE_OVERRIDES.items():
        previous = charmap.get(code)
        if previous and previous["char"] == char and previous["kind"] == "char":
            continue
        overrides_applied.append(
            {
                "hgss_code": code,
                "charmap_said": previous["char"] if previous else None,
                "charcode_h_says": symbolic,
            }
        )
        charmap[code] = {
            "hgss_code": code,
            "kind": "char",
            "char": char,
            "token": symbolic,
        }

    out = []
    for code in sorted(charmap):
        e = charmap[code]
        e["charcode_name"] = names.get(code)
        e["source"] = "charmap.txt" if code not in CHARCODE_OVERRIDES else "charcode.h"
        if e["kind"] == "char":
            if len(e["char"]) == 1:
                e["unicode"] = ord(e["char"])
            else:
                # Multi-character graphical forms (PK, MN) are symbols, not
                # text: they keep their token and get artwork from overrides.
                e["token"] = e["char"]
                e["char"] = None
                e["kind"] = "special"
                e["unicode"] = None
        else:
            e["unicode"] = None
        out.append(e)

    doc = {
        "schema": 1,
        "sources": {
            "charmap": "build/upstream/charmap.txt",
            "charcode_h": "build/upstream/include/constants/charcode.h",
            "decomp_pin": "8cbb4a19ab781611ff1767035dcaea6b47baeded",
        },
        "entry_count": len(out),
        "kind_counts": {
            k: sum(1 for e in out if e["kind"] == k)
            for k in ("char", "command", "escape", "special")
        },
        "charcode_overrides": overrides_applied,
        "entries": out,
    }
    dump_json(GENERATED / "charmap.json", doc)
    print(f"charmap: {len(out)} entries -> {GENERATED / 'charmap.json'}")
    print(
        "  kinds: "
        + ", ".join(f"{k}={v}" for k, v in doc["kind_counts"].items())
        + f"; {len(overrides_applied)} charcode.h overrides"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
