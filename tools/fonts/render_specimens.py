#!/usr/bin/env python3
"""Render Task 4 typography specimens at actual destination size.

Inputs:  assets/fonts/generated/{atlas.json,glyphs.json}
Outputs: assets/fonts/generated/specimens/*.png

Everything is drawn with the same integer placement rules the runtime uses:
the 5/4 interface mapping applies to the *cell origin*, the glyph mask is
blitted 1:1 at native size, and the baseline is a fixed physical row inside
the mapped cell. That keeps strokes on whole physical pixels and avoids the
cumulative drift a per-advance rounding would introduce.

Each case is written twice: actual size (100%) and nearest-neighbour 4x.
Nothing here rescales, antialiases or synthesises a glyph.
"""

from __future__ import annotations

import sys

from common import GENERATED, dump_json, load_json

MAP_NUM = 5
MAP_DEN = 4
TOP_INSET_PHYS = 40  # upper screen: 400 wide, 320 logical mapped + 40 margin


def map_x(logical_x, upper):
    return (TOP_INSET_PHYS if upper else 0) + (logical_x * MAP_NUM + MAP_DEN // 2) // MAP_DEN


def map_y(logical_y):
    return (logical_y * MAP_NUM + MAP_DEN // 2) // MAP_DEN


class Atlas:
    def __init__(self):
        self.glyphs = load_json(GENERATED / "glyphs.json")
        self.atlas = load_json(GENERATED / "atlas.json")
        self.art = {a["id"]: a for a in sorted(self.glyphs["artwork"], key=lambda a: a["id"])}
        # atlas.json art entries are in the same sorted order as artwork.
        self.art_by_index = self.atlas["art"]
        self.by_key = {}
        self.by_unicode = {}
        for g in self.atlas["glyphs"]:
            self.by_key[(g["font_id"], g["hgss_code"], g["variant"])] = g
        for g in self.glyphs["glyphs"]:
            self.by_unicode.setdefault((g["font_id"], g["unicode"]), g)

    def glyph(self, ch, font_id=0, variant="standard"):
        g = self.by_unicode.get((font_id, ord(ch)))
        if g is None:
            g = self.by_unicode.get((0, ord(ch)))
        if g is None:
            return None
        return self.by_key.get((g["font_id"], g["hgss_code"], variant)) or self.by_key.get(
            (g["font_id"], g["hgss_code"], "standard")
        )

    def mask(self, glyph_rec):
        a = self.art_by_index[glyph_rec["art_index"]]
        src = self.art[self.glyphs["artwork"][glyph_rec["art_index"]]["id"]]
        return a, src


def draw_line(img, atlas, x, y, text, font_id=0, variant="standard", fg=(245, 245, 245, 255),
              shadow=(80, 80, 88, 255), upper=False, show_shadow=True):
    pen_logical = x
    ox = map_x(pen_logical, upper)
    for ch in text:
        rec = atlas.glyph(ch, font_id, variant)
        if rec is None:
            continue
        a, src = atlas.mask(rec)
        gx = ox + rec["bearing_x"]
        gy = map_y(y) + rec["bearing_y"]
        if show_shadow:
            for yy, row in enumerate(src["rows"]):
                for xx, chbit in enumerate(row):
                    if chbit == "1":
                        px, py = gx + xx + 1, gy + yy + 1
                        if 0 <= px < img.width and 0 <= py < img.height:
                            img.putpixel((px, py), shadow)
        for yy, row in enumerate(src["rows"]):
            for xx, chbit in enumerate(row):
                if chbit == "1":
                    px, py = gx + xx, gy + yy
                    if 0 <= px < img.width and 0 <= py < img.height:
                        img.putpixel((px, py), fg)
        pen_logical += rec["advance"]
        ox = map_x(pen_logical, upper)


CASES = [
    ("uppercase", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", False, 0),
    ("lowercase", "abcdefghijklmnopqrstuvwxyz", False, 1),
    ("digits", "0123456789", False, 2),
    ("ambiguity", "Ill1 | O0 | rn | m | vv | w", False, 3),
    ("pokemon", "PokÃ©mon", False, 4),
    ("battle-commands", "FIGHT  BAG  POKÃ©MON  RUN", False, 5),
    ("battle-line", "Pikachu used Thunderbolt!", False, 6),
    ("hp-box", "Typhlosion   Lv. 100", False, 7),
    ("hp-values", "HP  299/299", False, 8),
    ("pp-values", "PP  15/15", False, 9),
    ("item", "PokÃ© Ball Ã—999", False, 10),
    ("item-upper", "MAX ELIXIR", False, 11),
    ("yesno", "Yes    No", False, 12),
    ("saving", "Saving... Don't turn off the power.", False, 13),
    ("accents-upper", "Ã€ÃÃ‚ÃƒÃ„Ã…Ã†Ã‡ÃˆÃ‰ÃŠÃ‹ÃŒÃÃŽÃÃÃ‘Ã’Ã“Ã”Ã•Ã–Ã—Ã˜Ã™ÃšÃ›ÃœÃÃžÃŸ", False, 14),
    ("accents-lower", "Ã Ã¡Ã¢Ã£Ã¤Ã¥Ã¦Ã§Ã¨Ã©ÃªÃ«Ã¬Ã­Ã®Ã¯Ã°Ã±Ã²Ã³Ã´ÃµÃ¶Ã·Ã¸Ã¹ÃºÃ»Ã¼Ã½Ã¾Ã¿", False, 15),
    ("symbols", "â™‚ â™€ â€¦ â€˜ â€™ â€œ â€ â€ž ã€Š ã€‹ â™  â™£ â™¥ â™¦ â˜… â—‰ â— â–  â–² â—† â™ª â˜€ â˜ â˜‚ â˜ƒ â˜º â™š â™› â˜¹ â†— â†˜ â˜½", False, 16),
    ("narrow-menu", "BAG  PKMN  GEAR  SAVE  OPTION", False, 17),
    ("two-line", "The wild Pidgey appeared!\nGo! Typhlosion!", True, 0),
    ("long-name", "Typhlosion the Magnificent  Lv. 100", False, 18),
    ("three-digit", "HP 714/714   PP 999/999", False, 19),
]

# The four horizontal phases produced by 5/4 mapping: logical offsets 0..3.
PHASE_CASES = [
    ("phase-0", 0),
    ("phase-1", 1),
    ("phase-2", 2),
    ("phase-3", 3),
]


def compose(atlas, cases, scale, title, background, fg, shadow, width=400, height=240):
    from PIL import Image

    img = Image.new("RGBA", (width, height), background)
    y = 6
    for name, text, two_line, _idx in cases:
        for line in text.split("\n"):
            if y + 26 > height:
                break
            draw_line(img, atlas, 6, y, line, 0, "standard", fg, shadow)
            y += 18
    if scale != 1:
        img = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
    return img


def main() -> int:
    atlas = Atlas()
    out = GENERATED / "specimens"
    out.mkdir(parents=True, exist_ok=True)
    from PIL import Image

    dark = (26, 28, 38, 255)
    light = (232, 232, 224, 255)
    fg_light = (248, 248, 244, 255)
    sh_dark = (64, 66, 80, 255)
    fg_dark = (32, 34, 44, 255)
    sh_light = (176, 178, 184, 255)

    written = []
    for scale, suffix in ((1, "1x"), (4, "4x")):
        for theme, bg, fg, sh in (
            ("dark", dark, fg_light, sh_dark),
            ("light", light, fg_dark, sh_light),
        ):
            img = compose(atlas, CASES, scale, "", bg, fg, sh)
            path = out / f"sheet_{theme}_{suffix}.png"
            img.save(path)
            written.append(path.name)

        # Individual cases, so a reviewer can inspect one at a time.
        for name, text, two_line, _idx in CASES:
            from PIL import Image as _I

            img = _I.new("RGBA", (400, 60), dark)
            yy = 6
            for line in text.split("\n"):
                draw_line(img, atlas, 6, yy, line, 0, "standard", fg_light, sh_dark)
                yy += 18
            if scale != 1:
                img = img.resize((img.width * scale, img.height * scale), _I.NEAREST)
            path = out / f"case_{name}_{suffix}.png"
            img.save(path)
            written.append(path.name)

    # Alignment and phase checks.
    for scale, suffix in ((1, "1x"), (4, "4x")):
        img = Image.new("RGBA", (400, 180), dark)
        draw_line(img, atlas, 6, 2, "Left aligned label", 0, "standard", fg_light, sh_dark)
        def text_width(s):
            return sum((atlas.glyph(ch) or {}).get("advance", 0) for ch in s)

        text = "Centred"
        draw_line(img, atlas, (160 - text_width(text)) // 2 + 40, 22, text, 0, "standard",
                  fg_light, sh_dark)
        text = "Right"
        draw_line(img, atlas, 160 - text_width(text), 42, text, 0, "standard",
                  fg_light, sh_dark)
        for i, phase in enumerate(PHASE_CASES):
            draw_line(img, atlas, 6, 56 + i * 18, f"HH phase {i} +{phase[1]}px", 0, "standard",
                      fg_light, sh_dark)
        if scale != 1:
            img = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
        path = out / f"alignment_and_phase_{suffix}.png"
        img.save(path)
        written.append(path.name)

    dump_json(
        out / "index.json",
        {
            "schema": 1,
            "mapping": "physical_x = 40 + round(logical_x*5/4) (upper), round(logical_x*5/4) (lower)",
            "baseline": "physical baseline = round(logical_y*5/4) + 15; glyph mask blitted 1:1",
            "shadow": "same mask at +1,+1 under the foreground, foreground opaque on top",
            "images": written,
        },
    )
    print(f"specimens: {len(written)} images -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

