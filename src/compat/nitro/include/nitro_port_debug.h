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

#ifdef __cplusplus
}
#endif

#endif
