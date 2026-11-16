# Cutting hardware work in Goldenrod (chatlog12 follow-up)

Goal: a sustained 30 FPS in Goldenrod on an Old 3DS, with every captured
game frame (logic + publish) under 33.4 ms. The O3DS run at `2524e88`
measured ~57 ms captured frames, up from ~36 ms. Azahar kept hitting 30 FPS
throughout, because it charges about one tick per instruction and does not
model the PICA's vertex cost. The comparisons below therefore count work
(instructions, triangles, draws, shader instructions per vertex). Azahar
milliseconds are only a proxy for O3DS time: O3DS has run about 2.3x slower
for logic and publish.

## Why 36 ms became 57 ms

PR #8 sent every display list containing a NORMAL down the per-vertex plan.
Each vertex re-ran the decoder tail and each normal its matrix transform,
including static map scenery. The PICA then lit every map vertex with the
409-instruction lit shader. The two 5,200-word map lists cost ~1.3M ticks per
call, and P3D reached 11.3 ms per picture. The work grew on both the CPU
and the GPU, and Azahar showed neither.

## Commits

| Commit | Change | Azahar Goldenrod, 600 captured frames |
| --- | --- | --- |
| `84d46dc` | Bake stable lit lists once with exact CPU lighting; replay as literal-colour blocks drawn with the unlit shader. Only lists whose lighting key misses on consecutive calls stay PICA-lit (retry every 64 pictures). | logic p50 25.35 → 14.06 ms |
| `413d0cc` | Cull retained blocks wholly outside the side planes of the clip volume before copying them (cached plane vectors, exact centre/extent test, list-level test for matrix-free lists). | triangles 5.02M → 1.94M, draws 80.3k → 61.7k, logic 14.06 → 13.67 ms, publish 4.49 → 3.75 ms |
| `e587d4c` | NNS GLB state list (63 words, ~105 calls per picture): restore the pure-setter prefix from a snapshot and execute only the last packed word (base matrix, scale, texture). | logic 13.67 → 13.38 ms |
| `eeb4b37` | Lit shader skips disabled lights and the half-vector path of lights with no specular colour. The flags sit in the unused `.w` of `light_dir_hi` / `light_half_hi`. | 101 shader instructions per one-light vertex instead of 409 (GPU only; Azahar CPU unchanged) |

Every step's held title, Goldenrod and walking frames match the baseline
pixel for pixel. Each step has a host test that fails when its key
invalidation is removed (checked by mutation): `g3_lighting_test`
(static-scenery phase), `g3_cull_test` and `g3_state_test`.

## Remaining profile (Azahar, sampled 1/64 FIFO calls, after `e587d4c`)

- GLB state list: ~4K ticks per call (snapshot apply + live tail).
- Two ~1,050-word lit lists stay on the per-vertex PICA-lit plan at ~128K
  ticks per call, about 2 ms of logic per picture. A diagnostic
  (`build/opt-tools/diag_gpulit.py`) is ready to show why they are recorded
  as bad. The suspected cause is lighting that depends on vector matrices
  restored inside the list. If so, keying on the 3x3 of the restored stack
  slots would let them bake.
- BOX_TEST (66 per picture, ~3.8K ticks each) is already reduced to 16
  products per test, and the game reads its result.
- Short NNS calls (`MAT_InternalDefault`, 1- and 3-word calls) are decoded
  live.
- About 102 draws per picture at ~9.7 µs of CPU each (coffeepot drawbench).

## What went wrong

- **A contaminated baseline.** The first "baseline" build compiled after the
  edits (identical md5). It was rebuilt from a `git stash`.
- **Drifting references.** After a few hours, a wandering NPC in the held
  field and walk frames differed even for an unchanged binary. The emulated
  clock moves on and the NPC's movement depends on it. Three bisection
  builds (verify mode, init revert, HEAD without culling) all showed the same
  diff before an old binary was rerun. Recapture the baseline
  (`build/opt-tools/rebase.sh`) before blaming a change for character
  movement.
- **Weak tests.** Mutations first went undetected for several reasons: a
  rotation that only affected y, ambient lighting saturating to white, masked
  light keys, normals facing away from the light, and neighbouring
  disturbances doing the same invalidation. `g3_state_test` now puts a quiet
  model after each disturbance. The clip, light-colour and tail mutations are
  caught. The viewport `material_changed` is redundant (the batch key compares
  the viewport) and the `vec_changed` is masked by the tail's MULT_4x3. Both
  stay as defensive invalidations.
- **A full-content snapshot that never hit.** The GLB's last word changes
  per model, so the whole-list snapshot was slower. Splitting at the last
  command word fixed it.
- **PICA bool uniforms read as false in Azahar.** Picasso placed the
  `.bool` uniforms at b0–b8, libctru returned `startReg - 0x10`, and citro3d
  uploaded `0x7FFF0000 | bools`. Even so, every light was skipped and Ho-Oh
  rendered black. The cause was not found. Flags in the unused `.w` of float
  uniforms, tested with `cmp`/`jmpc` like the existing shininess branch,
  work. A separate `light_flags[4]` array would not fit: the shader already
  uses 91 float uniforms plus 7 constants of the 96.
- **The 8-corner cull test was too slow.** It was replaced with plane vectors
  and a centre/extent test, plus one test per matrix-free list.

## Next

1. Run `diag_gpulit.py` in Azahar and bake the two lit lists if they are
   static under the camera.
2. Build normal and profiled hardware artifacts and measure on O3DS. P3D
   should fall with the shader skips and baked scenery. Logic should follow
   the instruction counts above.
3. Per-draw renderer cost: palette float maths and redundant state calls.
