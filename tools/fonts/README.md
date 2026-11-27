# `tools/fonts/` — offline native-font pipeline

Development-machine tooling only. The 3DS never parses BDF/TTF/OpenType and
never links a font rasterizer; it loads the packed atlas this pipeline emits.

## Run

```sh
# Pillow is needed for previews and specimens. Use a native Python 3 with it:
python -m pip install -r tools/fonts/requirements.txt

python tools/fonts/build_charmap.py     # charmap.txt + charcode.h -> charmap.json
python tools/fonts/extract_bdf.py       # Galmuri14.bdf -> bdf.json (repertoire)
python tools/fonts/generate_glyphs.py   # resolve (font, code) -> glyphs.json
python tools/fonts/build_atlas.py       # pack -> atlas .bin + previews
python tools/fonts/validate_atlas.py    # fail on missing required glyphs
python tools/fonts/render_specimens.py  # Task 4 review sheets
```

All six share `common.py` (one BDF parser, one mask model, one packer, one
PICA tiler). Order matters: `build_charmap.py` first, because the extract and
generator use its scope classification.

## Guarantees

- **Deterministic.** Artwork is packed in fingerprint order; no timestamps,
  floats, dict-order or absolute paths reach the binary. Re-running produces
  byte-identical output (`validate_atlas.py` prints the hash).
- **Complete or loud.** A required Latin glyph with no artwork fails
  `validate_atlas.py`. Non-Latin and Japanese/Korean-edition codes are written
  as explicit fallback records, never blank glyphs.
- **No resampling.** Masks are one-bit, blitted 1:1. Nothing is stretched,
  antialiased or synthetically bolded.
- **Attributed.** The source hash, release pin and licence live beside the
  font; see `assets/fonts/galmuri14/SOURCE.md`.

## Files

| File | Responsibility |
| --- | --- |
| `common.py` | BDF parsing, mask algebra, JSON, shelf packing, PICA tiling, scope classification |
| `extract_bdf.py` | strike metrics + repertoire masks from the pinned BDF |
| `build_charmap.py` | classified HGSS code → Unicode table, including `charcode.h` overrides |
| `generate_glyphs.py` | per-(font, code, variant) records, bearings, fallbacks |
| `build_atlas.py` | packing, RGBA5551 8×8-tiled binary, previews |
| `validate_atlas.py` | coverage, bounds, overlap, binary round-trip, coverage report |
| `render_specimens.py` | actual-size and 4× typography review sheets |
