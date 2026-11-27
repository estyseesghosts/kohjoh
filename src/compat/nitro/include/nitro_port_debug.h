/*
 * nitro_port_debug.h: test hooks the patched game polls. kohjoh implements
 * them in src/app/script.c from the LOTUS input script; with no script (or
 * no matching directive) they never fire, so a normal run is unchanged.
 */

#ifndef NITRO_PORT_DEBUG_H_
#define NITRO_PORT_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 once, on the first field frame at or after the script's
 * `battle: <v-blank> <trainer1> [trainer2]` line, with the trainer IDs to
 * fight (trainer2 == trainer1: double battle). Otherwise returns 0. */
int NitroPort_DebugBattle(unsigned *trainer1, unsigned *trainer2);

/* Returns 1 once, as that battle is set up, when its line also names a
 * backdrop (`... <trainer2> <BattleBg> <Terrain>`, decomp enum values: 9
 * and 5 a cave, 1 and 7 water), with them. Otherwise returns 0. */
int NitroPort_DebugBattleBackdrop(int *bg, int *terrain);

/* Called once per active map prop's render-visibility BOX_TEST. Returns
 * nonzero to bypass that one rejection (development-only, script.txt
 * `debug: 0x2000`). With `debug: 0x4000` every decision is logged with a
 * stable prop-slot identifier and its world translation, so a pop-in can be
 * attributed to producer-side rejection rather than renderer culling. */
int NitroPort_DebugPropVisibility(unsigned propIndex, int x, int y, int z, int boxResult);

/* Called for each visible field billboard before its texture is bound. The
 * model pointer, animation sequence/frame and texture key are stable across
 * consecutive frames, so Task 3A can follow one character and compare its
 * projected position/scale in legacy, 1x and 2x. Enabled by
 * script.txt `debug: 0x8000`. */
void NitroPort_DebugBillboard(unsigned model, unsigned type, unsigned sequence, unsigned frame,
                              unsigned texKey, int x, int y, int z, int sx, int sy, int sz);

#ifdef __cplusplus
}
#endif

#endif
