/*
 * nitro_port_gx3d.h: the geometry-engine hooks the patched NitroSDK and the
 * compat layer call. kohjoh implements them in src/runtime/kj_g3.c (a new
 * engine: matrices, lighting and tests on the CPU, positions transformed on
 * the PICA). This replaces the vendored header, whose engine (CPU transform,
 * clipping and rasterisation) is not part of kohjoh.
 */

#ifndef NITRO_PORT_GX3D_H_
#define NITRO_PORT_GX3D_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Direct opcode writes (one parameter word per call), and packed GXFIFO
 * words (a command word, then the parameters of its up-to-four commands). */
void NitroPort_GxDirectWrite(int opcode, uint32_t value);
void NitroPort_GxFifoWord(uint32_t value);
void NitroPort_GxFifoWords(const uint32_t *words, size_t count);

/* Bring GXSTAT's stack levels, test bits and RAM counters up to date. */
void NitroPort_Gx3dRefreshStatus(void);

/* Bring the clip and vector matrix result registers (0x640, 0x680) up to
 * date; the SDK's G3X_GetClipMtx/G3X_GetVectorMtx call it first. */
void NitroPort_Gx3dSyncResults(void);

/* GXSTAT bit 15 is write-one-to-clear and also resets the projection stack
 * pointer. */
void NitroPort_Gx3dClearStackError(void);

/* Whether a swapped scene is still waiting to be drawn when the V-blank
 * does the game's queued 3D texture transfers (kohjoh: never). */
int NitroPort_Gx3dEarlierSceneUnread(void);

#ifdef __cplusplus
}
#endif

#endif
