# Galmuri14 source pin

This directory holds the unmodified upstream Galmuri14 bitmap font that
Kohjoh's native text renderer is derived from. Nothing in this directory is
generated or edited by Kohjoh; every adapted bitmap lives under
`../generated/` and is produced by `tools/fonts/`.

| Field | Value |
| --- | --- |
| Family | Galmuri14 (Regular) |
| Release tag | [`v2.40.4`](https://github.com/quiple/galmuri/releases/tag/v2.40.4) |
| Release asset | `Galmuri-v2.40.4.zip` |
| Release asset SHA-256 | `c8b3d9861a62ae73c8b1178091401cd79994812437ef386413f6dd54856e60e7` |
| Download URL | <https://github.com/quiple/galmuri/releases/download/v2.40.4/Galmuri-v2.40.4.zip> |
| File imported | `Galmuri14.bdf` |
| Imported file SHA-256 | `316afc1e7b48abf3bbd261b572148610a5fcad9da8e96a7c9de7e910af973bf4` |
| Imported file size | 2,938,984 bytes |
| Copyright | Copyright (c) 2019–2025 Lee Minseo (quiple@quiple.dev) |
| Licence | SIL Open Font License 1.1 (`LICENSE.txt`, copied verbatim) |
| Upstream revision | `v2.40.4` release (workflow-built from `main`; the release page is the pin) |

The release archive SHA-256 above is the digest GitHub publishes for the
asset, and it was checked against the downloaded archive before extraction.
The BDF itself was then hashed so a re-import can be proven identical.

## Verified strike metadata

The brief describes Galmuri14's strike as "15 pixels (11 points)". The BDF's
own header is authoritative and does not say that: its `POINT_SIZE` is 15
(hence the "15" in some upstream prose), while the physical strike is a
**21-pixel line box**:

```text
SIZE 21 75 75
FONTBOUNDINGBOX 16 22 -1 -4
FONT_ASCENT 18
FONT_DESCENT 3
POINT_SIZE 15
X_HEIGHT 10
CAP_HEIGHT 14
CHARS 19222
```

So the metrics Kohjoh's renderer must honour are: cap height 14 px, x-height
10 px, ascent 18 px, descent 3 px, 21 px line box. "Galmuri14" names the cap
height, not the em size. This is recorded here because the brief's phrasing
would otherwise cause the importer to look for a non-existent 15 px strike.

## Modifications made by Kohjoh

None to the files in this directory. Downstream pipeline artefacts (a
one-bit mask atlas, fixed-width numeric digits and a few hand-authored
supplementary symbols) are **derived** works, generated under
`../generated/`, and are distributed under the same OFL 1.1. They are named
the *Kohjoh Native Text Atlas* so no derivative claims the Galmuri name.
