# ASSET_BUILDER — deterministic, user-local ROM conversion for the 3DS

**Status:** tool specification, not code. **Primary rule:** ship source/build tools, not Nintendo game content. Related: [FONT_RENDERER](FONT_RENDERER.md), [GPU_2D](GPU_2D.md), [GPU_3D](GPU_3D.md), [GPU_RESOURCES](GPU_RESOURCES.md).

## 1. Why a converter is a performance component

The 3DS should not decompress and reinterpret an invariant texture, font or static model every frame. The build process should use a user-supplied HG/SS ROM to produce PICA200-ready pages and metadata, with only dynamic game state reconstructed at runtime.

The converter is a **host-side program** (Windows/Linux/macOS); use reproducible tools and strict validation instead of performing expensive discovery on the 268 MHz ARM11. The format should be versioned, with integrity checks, explicit offsets/lengths, alignment and an asset dependency graph. For 3DS runtime memory, prefer fewer indexed packed blocks rather than many individual SD files.

Important references: [Coffeepot asset pipeline](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/asset-pipeline.md), [porting](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/porting.md#7-build-an-asset-pipeline-for-the-destination-not-for-the-desktop), [GPU texture formats](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/PICA200/rendering-2d.md#textures-and-memory), [original HGSS source](https://github.com/pret/pokeheartgold).

## 2. Two distinct build products

1. **Development reference capture pack:** original asset bytes, decoded state and expected reference frames; may be too large and only exists on the developer's machine. Used by automated pixel/state tests, never linked into the release game.
2. **Runtime content pack:** GPU-compatible texture pages, glyph tables, meshes, indexed assets and minimal metadata required by the actual game. Converted from the user's ROM, no original ROM content committed to source control.

Separate host converter behavior from runtime assumptions. A generated pack must declare exactly which ROM build/region, source version and converter version it supports. Fail on unsupported content rather than guessing archive member layout by offsets.

## 3. Required manifest and verification

Proposed manifest fields (schema not frozen):

```text
pack_format_version, converter_git_sha, build_timestamp_policy,
source_rom_sha256, region, game_kind(HG|SS), supported_revision,
engine_source_sha, shader_format_version, atlas_layout_version,
asset_record_count, asset_table_digest, content_digest,
resource_alignments, compression_mode, memory_footprint_estimate
```

For reproducibility, timestamps must be omitted or normalized; sort records by stable content ID, not filesystem enumeration order. Keep a converter self-test that builds the same ROM twice and compares output SHA256 exactly.

Do not embed personal paths, game saves or arbitrary files inside the final pack. Record intentional variants as manifest flags (e.g. font render variant), not as unnamed special cases.

## 4. Extraction/translation inventory

| Source class | Host processing | Runtime representation |
| --- | --- | --- |
| Text and font archives | Parse NARC, original variable widths, glyph mask, control-code mapping | Native 5/4 glyph atlases + original logical metrics, per-region tables |
| BG tile character/map/palettes | Decode DS tile modes; construct dependency lists; pack stable pages | Swizzled direct-color atlas pages, palette-version dependent dynamic page rules |
| OBJ sprite sheets | Extract frames, pivots, OAM metadata and palettes | Batched sprite texture pages, clip/affine/flip metadata |
| Static 3D textures | Interpret DS formats/palettes; preconvert losslessly as practical | GPU tiled textures, optionally ETC1 only if visually approved |
| Static geometry/display lists | Parse only when interpretation is invariant; preserve original command relationships | Retained packed vertices/indices, material tables, transform bindings |
| Dynamic geometry and palettes | Build schema and known bounds; **do not bake dynamic state as static** | On-device specialized command/atlas updates |
| UI panels and frames | Inspect nine-slice/tiling semantics, window rects, original palette behavior | Native dimension layouts/slices and reusable GPU panels |
| Audio resources | Preserve DS sequences/instrument references; candidate sample conversion | Runtime DSP/mixer inputs with reference-compatible timing |
| Script/overlay metadata | Resolve code↔asset mappings without changing game meaning | Indexed compatible asset resolver and source identifiers |

A preconverted asset cannot erase runtime changes the game legitimately makes. Original VRAM write paths remain authoritative for dynamic data and palette changes; asset metadata must let the renderer invalidate and rebuild only what changed.

## 5. Font asset conversion: required fidelity

For every font ID and supported region, export glyph IDs, source masks, original glyph width/height, advance width, baseline/line-height, and any palette/shadow semantics that are actually present. The game-facing width stays original; native scaled quad bounds are separate metadata.

Produce at least: `faithful_nearest`, `gridfit_125` and optional `smooth_125` atlases. Before selecting the default, compare actual game text strings on the physical 3DS; do not trust a desktop screenshot alone. Precompose common shadow/foreground variants only when atlas size makes sense. Keep a complete glyph coverage report and fallback policy; no blank characters in rare languages/fonts.

See [FONT_RENDERER.md](FONT_RENDERER.md) for the runtime window-mutation contract. The converter must not decide game string wrapping or rewrite text control codes.

## 6. Static vs dynamic geometry audit

A retained G3 mesh is only safe if source bytes, inherited command state and interpretation have been proven stable. Record for every candidate:

- Original code/display-list caller and archive member ID.
- Whether vertices reference mutable RAM, runtime animation, light/material/texture state.
- Which matrix state is inherited or pushed and must stay dynamic.
- Explicit cache key/dependency ranges and invalidation events.
- Replaying the same original DS command sequence with the cached object against an unoptimized decoder.

Only then turn it into immutable GPU geometry. Unclear objects stay in the dynamic command decoder (still GPU-rendered) rather than being guessed static.

## 7. Pack layout and streaming

A suggested pack architecture:

```text
HGSSPACK header (version, hashes, source ID, alignment rules)
  asset index (sorted, bounded records, checksums)
  dependency table (palette/VRAM/source hashes)
  palette/material metadata
  font metric/glyph metadata
  swizzled texture pages (aligned, optionally compressed for storage)
  static mesh pages
  audio metadata/optional converted samples
  optional scene-prefetch index
```

Use 64-bit host offsets while building; validate on-target offset width and integer overflow for every array before use. Store immutable pages contiguously for fewer seeks, and use explicit decompression scratch limits. If format compression is used, test cold loads and worst-case decompression on O3DS rather than choosing a desktop-friendly codec. Consider background streaming only when threading tests show benefit over synchronous chunk loads before scene entry.

**Avoid a 3DSX package that requires large ROM copies in `/3ds/<app>/` unless that is an intentional installation route.** If the project later supports both 3DSX and user-built CIA, keep **one asset content format** with a pluggable storage backend. A self-contained CIA can embed converted user assets, whereas 3DSX can read app-local content; actual read/write/save path APIs and SD permissions must be tested on hardware.

## 8. User-local build flow and repository hygiene

Suggested host commands and output names are **proposals**:

```text
hgss-pack verify --rom <user-file.nds>
hgss-pack inspect --rom <user-file.nds>
hgss-pack build --rom <user-file.nds> --game hg --out <private-build-dir>
hgss-pack audit --pack <private-build-dir>/game.pack
hgss-pack compare --left <build-A> --right <build-B>
```

No tool may silently download a copyrighted ROM. Keep generated art, fonts, complete extracted NARCs, compiled game executables, saves, keys and proprietary binaries out of the public repository and CI artifacts. Tests may use synthetic public-domain fixtures and hashed metadata; end-to-end ROM tests run only in the user's private environment.

If a packaging step uses devkitPro utilities (`tex3ds`, shader assembler, etc.), pin the tool versions and record command lines and exit status for reproducibility. A custom pack file format must be fuzzed and validated before parsing files from storage.

## 9. Cold/warm resource behavior

The builder can provide hints for next-map prefetch and persistent UI/font pages. Runtime must still be correct when prefetch misses, the SD is slow, or the user immediately changes direction at a map boundary. Measure:

- Cold boot and cold map load time.
- Warm scene revisit with unchanged cached assets.
- Total bytes read, number of I/O calls, decompressed bytes, decode CPU time.
- GPU upload bytes/frame and total resident VRAM.
- Peak temporary memory while unloading old and loading new scenes.
- Correct resource version after edited palette/VRAM/register state.

Rendering deadlines apply *while actual gameplay continues*. A declared loading screen may pause game presentation, but a hidden 100 ms texture upload in a walking frame violates the 30 FPS goal.

## 10. Acceptance and test IDs

| ID | Test | Gate |
| --- | --- | --- |
| AST-001 | Convert identical ROM twice | Byte-identical output, normalized metadata |
| AST-002 | HG and SS variants, region/revision mismatches | Correct supported packs; explicit errors for unsupported |
| AST-003 | NARC/text/font round trip | All glyph IDs/widths valid; no missing glyphs |
| AST-004 | Random tile/palette/OBJ samples | Exact decoded colors/alpha/flip after conversion |
| AST-005 | Static mesh replay | Equivalent decoded G3 vertex/material state |
| AST-006 | Dynamic map/palette changes | Cached assets invalidated; no frozen source state |
| AST-007 | Pack parser fuzz/malformed input | Bounds-checked failure, no crash/overflow |
| AST-008 | Full transition O3DS stress | Within memory peak, no in-play deadline spikes |
| AST-009 | Build hygiene | No ROM/game art/user identifiers in public artifacts |
| AST-010 | Storage backend parity | Same content semantics for planned 3DSX/CIA backends |

## 11. Sources and references

- Coffeepot local `C:\Users\terezi\workdir\coffeepot\ctr\engineering\PICA200\asset-pipeline.md`, `ctr\engineering\porting.md`, `ctr\engineering\memory-and-cache.md`.
- [devkitPro tex3ds tooling/3DS texture references](https://github.com/devkitPro/3ds-examples), [original Pokémon HG/SS decomp](https://github.com/pret/pokeheartgold), [existing hgss-3ds](https://github.com/estyseesghosts/hgss-3ds).
