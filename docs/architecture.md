# How kohjoh works

kohjoh runs the original HeartGold/SoulSilver code (the pinned decompilation)
on the 3DS with a new runtime and GPU renderer. Imported code is listed in
[`PROVENANCE.md`](../PROVENANCE.md); the design brief is [`kohjoh.md`](../kohjoh.md).

## Two halves, one bridge

| Half | Compiled with | Contents |
| --- | --- | --- |
| DS side | NitroSDK headers, 4-byte enums | game, Nitro compat ([`src/compat`](../src/compat)), [`src/runtime`](../src/runtime) |
| 3DS side | libctru/citro3d, short enums | [`src/app`](../src/app), [`graphics`](../src/graphics), [`audio`](../src/audio), [`storage`](../src/storage), [`diagnostics`](../src/diagnostics) |

They meet only in [`kj_bridge.h`](../src/runtime/kj_bridge.h): fixed-width
types, no enums, `kj_ds_*` / `kj_ctr_*` functions.

## Frame flow

1. The game's own main loop runs on the main thread (`kj_game_main`).
2. Each DS V-blank wait (`OS_Halt`) calls `kj_ctr_vblank`: APT, input, then a
   wait locked to the physical V-blank count. A late frame does not wait, so
   the game never loses DS time against the real-time audio; a stall over 30
   V-blanks resyncs instead ([`main.c`](../src/app/main.c)).
3. After the V-blank callback and vwait tasks (game patch 2001),
   [`kj_frame.c`](../src/runtime/kj_frame.c) runs the H-blank walk and
   publishes one coherent snapshot: registers, palettes, OAM, VRAM banks,
   per-line registers and the last swapped 3D scene. Loops that never reach
   the hook (boot, error screens) publish from the V-blank once more than 4
   V-blanks pass without one.
4. [`renderer.c`](../src/graphics/renderer.c) waits for the previous GPU frame
   (`C3D_FrameBegin(0)`), records both screens and submits. Nothing reads DS
   memory after it returns. If the game is ≥2 refreshes behind, the
   presentation (not the game tick) is skipped and counted.

## 2D ([`g2d.c`](../src/graphics/g2d/g2d.c))

- DS tiles are palette-expanded into a 1024×1024 RGBA5551 atlas
  ([`atlas.c`](../src/graphics/resources/atlas.c)), one DS tile per PICA 8×8
  tile. A slot is reused only if its tile bytes are byte-identical and its
  palette version unchanged: no write tracking, no hashing.
- Layers draw in DS priority order, with the 3D scene at BG0's slot. Stencil
  bits carry blend-target membership (alpha blend = two draws) and window
  regions; BLDY and master brightness are full-screen passes.
- Text BG and OBJ layers are retained when their inputs (registers, palette,
  map/OAM, ext palettes, the character bytes read) are byte-identical; an
  unchanged screen is not drawn or transferred at all.
- Frames with per-line register changes are drawn as spans of identical lines.
- Top screen: DS 256×192 at 1.25× centred in 400×240; 2D is scissored to the
  centre, 3D fills the width. Bottom: 1.25× to 320×240.

## 3D ([`kj_g3.c`](../src/runtime/kj_g3.c), [`g3d.c`](../src/graphics/g3d/g3d.c))

- The DS side interprets the G3 command stream: matrix stacks (20.12 fixed),
  lighting, texgen, box/position/vector tests and result registers on the
  CPU, exactly as the game reads them back.
- Vertices stay in object space; each batch carries a palette of up to 16 clip
  matrices. The 3DS side multiplies each by one fixup matrix (viewport,
  widescreen x-scale 0.8, screen rotation, depth range) and the vertex shader
  transforms on the GPU.
- The box test is widened for widescreen so side content is not culled.
- DS textures convert once per content change ([`textures.c`](../src/graphics/g3d/textures.c)).
- Not yet supported (counted, never silently dropped): fog, toon/highlight,
  edge marking, shadow volumes, wireframe, display capture, bitmap BGs.

## Audio, storage, diagnostics

- The vendored ARM7 sequencer and DS sound chip mix 32768 Hz stereo on an
  audio thread (core 0, above the game) fed by NDSP ([`audio.c`](../src/audio/audio.c)).
- Saves: interim raw 512 KiB file; two-generation SaveStore on `wip/savestore`.
- Logs go to a memory ring flushed by a low-priority thread; `[perf]`, `[pace]`,
  `[gpu]` and `[ds]` aggregates every 600 publishes ([testing](testing.md)).
