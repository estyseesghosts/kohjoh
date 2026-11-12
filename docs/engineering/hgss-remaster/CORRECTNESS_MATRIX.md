# CORRECTNESS_MATRIX — test scenes, invariants and release gates

**Status:** living acceptance specification; initial evidence status is **NOT YET VERIFIED**. This document does not claim the new renderer already passes any tests. Related: [FONT_RENDERER](FONT_RENDERER.md), [GPU_2D](GPU_2D.md), [GPU_3D](GPU_3D.md), [GPU_RESOURCES](GPU_RESOURCES.md), [FRAME_PIPELINE](FRAME_PIPELINE.md), [ASSET_BUILDER](ASSET_BUILDER.md).

## 1. Four distinct kinds of correctness

A remaster intentionally changes output dimensions and may sharpen glyphs. Literal RGB pixel identity to an original DS screenshot is therefore not the only correctness metric. Enforce **all four layers**:

1. **Behavioral:** game tick count, scripts, battles, collision, input, dialogue events, save data and audio sequencing agree with original game.
2. **Semantic graphics:** same tile/sprite/background identities, window mask/priority, material and alpha state, scanline event effects, transform and layout semantics.
3. **Visual fidelity:** native-resolution screenshot compared against an approved remaster reference, with center crop checked separately against original DS view and explicitly tolerated differences.
4. **Hardware/performance:** real launch-model O3DS returns 30 distinct new pictures per intended period under full game+audio load, with bounded latency, no silent fallback and no memory corruption.

A scene may pass native pixel/visual tests but fail the game's text timing or material semantics. Log each layer separately.

## 2. Reference capture and reproducibility

Every scenario must have a **reference bundle** containing:

```text
scenario_id, game(HG|SS), ROM revision/region/hash, source SHA,
reference emulator/hardware build and firmware, map/overlay,
initial save checksum and location, input script with tick numbers,
RNG/RTC setup, intended frame count, captured DS graphics events,
expected game-state hashes, optional audio checksum,
approved output image(s) and region masks,
known remaster-specific differences, comparison rule, tolerance
```

Use the old port and Azahar as *auxiliary* capture/debug paths; the canonical semantic comparison should trace to original HGSS/NDS behavior where available. If full DS hardware state cannot be captured on a real DS, label the emulator-derived reference and its uncertainty. Lock emulator version and configuration. Keep original copyrighted assets and saves private; the public repository stores tests using synthetic assets and hashes/metadata only.

For native visual comparison, partition output into: (A) center 256×192 logical original region; (B) newly visible upper-screen side columns; (C) HUD/dialogue artwork; (D) touch-screen region; (E) approved remaster font area. Use pixel or edge/shape tolerances per region, not a single global hash.

## 3. Scene corpus — required baseline

| ID | Scene/repro | Main risk | Required observations |
| --- | --- | --- | --- |
| COR-001 | Boot/logo/title/opening | Modes, fades, special pictures | Correct state sequence, images, pacing |
| COR-002 | New game intro/name entry | Fonts, input, script | Correct glyph IDs, cursor, selection, save init |
| COR-003 | Static Goldenrod outdoors | Multiple BG/3D, alpha, window | Native upper width, texture stability, exact central view |
| COR-004 | Goldenrod scripted 600-tick walk | Camera, culling, reuse | No missing side scenery, 30 new FPS, dynamic geometry |
| COR-005 | Ecruteak or comparable dense town | Object density/material changes | No geometry holes, bounded draws/stall |
| COR-006 | Route with water and animation | Tile animation, palette cycling | Correct animation timing and dirty-atlas updates |
| COR-007 | Grass/forest scene | Transparency and depth | Correct ordering, no doubled alpha |
| COR-008 | Indoor→outdoor transition | Resource lifetime and targets | Correct scene load and peak RAM; no stale texture |
| COR-009 | Cave/night weather | Fog/brightness/tints | Exact intended semantic effects |
| COR-010 | Basic wild battle | 2D/3D, text, animation | Correct encounter, battle and dialog |
| COR-011 | Trainer battle with many effects | Blending/depth/particles | All mandatory materials and timing |
| COR-012 | Battle entering/exiting, fades | Capture, scanline events | Correct per-line state, no feedback corruption |
| COR-013 | Text box typewriter/wait/scroll | Font+window mutation | Same logical glyph events; crisp native output |
| COR-014 | Bag, party summary, PC | Text, icon atlas, touch | Correct UI, cursor, icon priority |
| COR-015 | Pokégear / map / phone | Dynamic lower screen | Correct touch mapping; animation invalidation |
| COR-016 | Character follower in motion | Actor transform, sprite depth | Follower not clipped/behind wrong layer |
| COR-017 | Save/load/soft reset | Filesystem, persistent state | DS-compatible save, no corruption |
| COR-018 | Long idle state | Spurious redraws/cache | No stationary font/texture rebuild, audio stable |
| COR-019 | Rapid palette/window invalidation | Cache coherency | No stale colors or reused window glyphs |
| COR-020 | Lid/suspend/resume, HOME if available | GPU/audio lifecycle | No deadlock, stale screen, file descriptor leak |
| COR-021 | Continuous 30-minute route play | Worst-case pacing/memory | No sustained drops, leaks, queue backlog |
| COR-022 | HG vs SS differences | Assets/script/game choice | Correct pack and no version cross-contamination |
| COR-023 | Rare 2D modes and object windows | Unimplemented effects | No hidden fallback or silently omitted effects |
| COR-024 | Stress HBlank 1, 8, 32, 192 spans | CPU scheduling, GPU draw explosion | Correct scanlines and measured deadline behavior |
| COR-025 | Repeated map loop/cold map | I/O spikes, resource peaks | Bounded load stutter under declared scene policy |
| COR-026 | Multi-language/special font cases | Glyph coverage and layout | No missing glyphs; correct text behavior |

This is a minimum seeded corpus, not a claim that all special game scenes are covered. Extend it as a feature census discovers new DS modes. For game completion, include both HG and SS story-critical events and scripted features beyond the performance corpus.

## 4. Subsystem assertions and trace links

Tests in each engineering guide are normative. Keep this cross-check ledger:

| Subsystem | Test IDs | Correctness trigger |
| --- | --- | --- |
| Text | `FNT-001..008` | Glyph text events and window mutation visibility |
| 2D | `G2D-001..010` | BG, OBJ, palette, masks, HBlank, capture |
| 3D | `G3D-001..009` | Scene decode/retain, transform, material, widescreen |
| Resources | `RES-001..009` | Source snapshot, GPU lifetime, eviction/flush |
| Timing/audio | `FRM-001..010` | Game tick vs presented frame and no DSP underrun |
| Conversion | `AST-001..010` | Source ROM identity and correct pack decode |
| Performance tuning | `OPT-001..021` | A/B experiments and rollback decisions |

The implementation issue tracker should link every observation to at least one of these IDs. Not every OPT experiment must be implemented, but every accepted optimization needs the correctness assertions relevant to its changed path.

## 5. Two separate visual references

**DS behavior reference:** original 256×192 image/state and semantics. Check center original logical frame, priority, effect state and game events. The 3DS remaster may intentionally improve text, resolution and horizontal visibility without changing behavior.

**Remaster output reference:** native 400×240 top and 320×240 bottom, with approved HUD placement, native crisp font, projection and side content. Compare screenshots from the **same tick and render generation**, not same wall-clock time. Smooth font pixels are allowed only in font masks; missing outlines and altered line breaks are not.

Every approved visual deviation gets an explicit entry:

```text
Deviation: <scene + area + original behavior>
Reason: <resolution, widescreen, legibility or other improvement>
Implementation: <GPU material/layout/build-time conversion>
Approved reference: <image ID, review, test>
Prohibited regression: <what must still be true>
O3DS GPU/CPU budget: <measured cost or bound>
```

Never approve a blanket “visual differences acceptable” rule.

## 6. Strict runtime invariants

- **No linked software graphics fallback** in the shipped executable. Offline reference rasterizer allowed solely in host test tools.
- **No CPU final-color framebuffer** or routine GPU-to-CPU image readback in release frame path.
- **No stale image signatures:** a frame's source generation/signature is recorded before or at its actual data consumption boundary, not from later live VRAM.
- **No untracked atlas mutation, rendering from freed/overwritten GPU buffers, or draw-plan queue growing unboundedly.**
- **No required draw silently dropped because an unsupported mode was encountered.** Development builds must count and stop on unsupported effect IDs.
- **No game-simulation doubling, 60 Hz interpolation, hidden 15 FPS pacing or artificial audio slowdown.**
- **No wrong-region assets or text output due to assumed fixed NARC offsets.**
- **No false O3DS performance claim based on N3DS or emulator profiling.**

## 7. Performance acceptance records

For every measured scene run, store:

| Field | Interpretation |
| --- | --- |
| Console/model + sys firmware + Luma + devkitPro libs + binary SHA | Reproducibility |
| Screen mode, CPU clock, homebrew loader | Comparable runtime conditions |
| Scene/ROM/region/save hash, tick script | Reproduced work, not hand-picked idle |
| Warm-up frames, measurement frames, sample count | Statistical validity |
| Tick, render, submit and present counts | Distinct new frame confirmation |
| Inter-present p50/p95/p99/p99.9/max, deadline misses | Actual user-visible pacing |
| Game, HBlank, snapshot, 2D, 3D, record, flush CPU stage times | Bottleneck attribution |
| GPU time, queue wait, target transfer count | GPU bottleneck attribution |
| Draws/material switches/texture binds/vertices/cmd buffer use | Structural cost |
| CPU heap/linear/VRAM peak, alloc failures | O3DS feasibility |
| Texture uploaded bytes, dirty pages, cache hit/miss | Resource stability |
| Audio queue/underrun and input latency | Overall gameplay quality |
| Screenshot, game-state checksum, event trace | Correctness evidence |

**Reject metrics that average over duplicate pictures.** `game ticks/s`, `submitted/s`, `unique presented/s` and raw LCD swap frequency must be separate columns.

## 8. Milestone gates and no-compromise policy

| Phase | Exit gate on real O3DS |
| --- | --- |
| M0 reference tooling | Reproducible captures, frame hashes, event traces; source and library revisions locked |
| M1 standalone GPU replay | Correct representative top/bottom 2D/3D/font frames at native resolution; no fallback; 30 FPS in its replay context |
| M2 playable field | Original game integration, movement/input/audio, correct text and 30 new FPS in benchmark field scene |
| M3 overworld coverage | All ordinary field effects/windows/HBlank/culling, continuous scenario passes |
| M4 battles and UI | Battle and touch scenes pass semantic, native visual and 30 FPS gates |
| M5 integrated release | Long-run stability, HG and SS coverage, save/lifecycle, effect completeness and no missed gameplay deadlines in defined corpus |

If M1 is fast but M2 drops frames, do **not** announce success or target N3DS instead. Compare deterministic replay and full-game metrics to locate added work. If a required effect cannot be implemented under the deadline, the relevant milestone fails until the GPU design is changed. Optional visual improvements can be turned off; required game behavior cannot.

## 9. Failure log template and handling

```text
Issue ID: COR-XXX / subsystem test ID
Repro: game/region/revision, initial state and tick script
Observed: screenshot + state trace + numeric performance evidence
Expected: original semantic + approved remaster reference
First bad generation: game tick, VRAM page, resource generation, command index
Hypothesis: source/pipeline stage, not generic 'GPU slow'
Fix: code commit, affected contract
Proof: same deterministic input A/B + O3DS retest + regression IDs
Status: open / verified / rejected / documented deviation
```

Keep **reproducible** investigation artifacts instead of long agent-only narrative logs. A broken visual should be bisectable to the first incorrect command/state, not discovered only by watching the game manually.

## 10. Traceable references

- Original game: [pret/pokeheartgold](https://github.com/pret/pokeheartgold).
- Prior port worklogs: [hgss-3ds docs/progress/STATE.md](https://github.com/estyseesghosts/hgss-3ds/blob/main/docs/progress/STATE.md), [PICA instrumentation](https://github.com/estyseesghosts/hgss-3ds/blob/main/PICA200-INSTRUMENTATION.md).
- Coffeepot local: `C:\Users\terezi\workdir\coffeepot\ctr\engineering\porting.md`, `ctr\engineering\testing-on-hardware.md`, `ctr\engineering\memory-and-cache.md`, `ctr\benchmarks\README.md`.
- [Coffeepot porting](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/porting.md), [benchmarks](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/benchmarks/README.md), [Coffeepot HGSS postmortem](https://github.com/estyseesghosts/coffeepot/blob/main/ctr/engineering/case-studies/case-study-hgss-port.md).
