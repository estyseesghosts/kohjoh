# Fonts

Source assets for Kohjoh's native-resolution text renderer
([plan](../../docs/text-renderer.txt), [contract](../../docs/engineering/hgss-remaster/FONT_RENDERER.md)).

```text
assets/fonts/
    README.md
    galmuri14/
        Galmuri14.bdf     upstream, unmodified
        LICENSE.txt       upstream SIL OFL 1.1, verbatim
        SOURCE.md         release pin, hashes, verified strike metadata
    overrides/
        glyphs.json       hand-authored HGSS-only symbols
        masks/*.png       one-bit masks for those symbols
    generated/            produced by tools/fonts/, committed for the build
        charmap.json      parsed HGSS character map
        glyphs.json       resolved glyph metrics and provenance
        atlas.json        packing result (per-glyph UVs and bearings)
        kohjoh_text_atlas.bin      RGBA5551, 8x8-tiled, white + coverage alpha
        atlas_preview_1x.png      human-readable preview
        atlas_preview_4x.png      nearest-neighbour enlarged preview
        coverage.md / coverage.json
        specimens/        typography review sheets (Task 4)
```

## Rules

- Nothing under `galmuri14/` is edited. Re-import from the pinned release and
  verify the hash in `SOURCE.md`.
- Nothing under `generated/` is edited by hand. Regenerate it with the
  pipeline; `tools/fonts/validate_atlas.py` refuses an out-of-date tree.
- The 3DS never parses BDF, TTF or OpenType. Only the packed atlas binary is
  compiled into the executable; there is no runtime font library.
- Font assets are OFL 1.1, not GPL-3.0. They stay under OFL 1.1 when
  distributed with Kohjoh builds. See [`PROVENANCE.md`](../../PROVENANCE.md).

## Attribution

> Kohjoh's native-resolution text renderer uses Galmuri14 by Lee Minseo
> (quiple), licensed under the SIL Open Font License 1.1. Native glyph
> adaptations and supplementary symbols are generated for Kohjoh.
