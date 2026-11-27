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
   `kj_ctr_publish_frame` is a coordinator over named phases: skip decision,
   queue completion, frame preparation, screen draws, submission recording,
   the 600-frame report and scripted capture/freeze directives.

## Lifecycle and ownership

- Renderer stages (`C3D`, targets, G3 storage, G2D, G3D) each record their
  own readiness. `kj_renderer_init` unwinds through `kj_renderer_exit` on any
  failure, and `kj_renderer_exit` waits for outstanding GPU work before
  freeing buffers the PICA may still read. The renderer owns the four G3
  vertex/index buffers; `kj_g3_shutdown` frees only the G3 display-list arena
  and clears the borrowed pointers.
- Child modules own their resources: `targets.c` deletes a partial target
  pair, `gpu.c` tracks DVLB/program/arena stages, `atlas.c` has explicit
  readiness, `offscreen.c` cleans its texture, `text_render.c` unwinds its
  run buffer and atlas, `fontbench.c` and the G2D retained-layer caches have
  exit paths.
- Threads use defined atomics, not `volatile`: the audio and log stop flags
  are published with `__atomic_store_n(..., __ATOMIC_RELEASE)` and read with
  `__atomic_load_n(..., __ATOMIC_ACQUIRE)`; `LightEvent` carries the wakeup.
  A faulting thread never calls the normal ring logger from the exception
  handler.
- Save startup is transactional: `NitroPort_SetSavePath` opens, creates or
  imports the 512 KiB image before `kj_game_main`, and failure reaches the
  application's fatal screen. `paths.c` reports whether `saves/` exists.

## 2D ([`g2d.c`](../src/graphics/g2d/g2d.c))

- `g2d.c` is the screen compositor: it walks spans in DS priority order,
  applies windows/blend/brightness and owns retained-layer reuse policy.
  BG/OBJ palette and tile decoding lives in
  [`g2d_layers.c`](../src/graphics/g2d/g2d_layers.c); the two share only the
  private [`g2d_internal.h`](../src/graphics/g2d/g2d_internal.h) context and
  run types. That header is included only inside `graphics/g2d`.
- Retained-layer reuse stays next to the draw order because the cache records
  exactly what the span pass consumed; the store/comparison helpers remain in
  `g2d.c` rather than being split into a second state owner.

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

- Pure fixed-point matrix products and sign/vector helpers live in the
  private [`kj_g3_matrix.c`](../src/runtime/kj_g3_matrix.c) /
  [`kj_g3_matrix.h`](../src/runtime/kj_g3_matrix.h). Command handlers, matrix
  stacks and display-list replay stay in `kj_g3.c`: those are the measured
  hot path and share live DS state.

- The DS side interprets matrix stacks (20.12 fixed), texgen and readable
  box/position/vector tests on the CPU. NORMAL latches its transformed normal
  and immutable lighting inputs; the PICA evaluates DS diffuse/specular,
  shininess, ambient and emission with exact fixed-point rounding.
- Lit display lists cache expanded object-space positions and retain live
  state events. Source validation is exact; animated matrices and lights do
  not prevent reuse. Literal-colour meshes retain the baked block cache.
- Vertices stay in object space; each batch carries a palette of up to 16 clip
  matrices. The 3DS side multiplies each by one fixup matrix (viewport,
  widescreen x-scale 0.8, screen rotation, depth range) and the vertex shader
  transforms on the GPU. Lit draws use six clip matrices to leave uniform
  space for four lights and the shininess table; the vertex ABI stays 16 bytes.
- The prior publish selects capture before the next scene starts. Field and
  title producers skip discarded geometry traversal and FIFO submission,
  preserving game updates, animation and resource retirement. Other producers
  retain the state-preserving decoder fallback.
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
