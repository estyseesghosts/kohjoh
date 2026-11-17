/*
 * nitro_port_app.h: the application (OverlayManager) hooks the patched game
 * calls. kohjoh implements them in src/runtime/kj_screen.c, which turns the
 * running application into a stable screen ID for the renderer (issue #15).
 * Display only: nothing here touches game state or the save.
 */

#ifndef NITRO_PORT_APP_H_
#define NITRO_PORT_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* OverlayManager_Run of the application whose template init is `init` is
 * starting / has returned. Calls nest: a child application runs inside its
 * parent's Main, so the deepest run of a frame is the screen on display. */
void NitroPort_AppEnter(const void *init);
void NitroPort_AppLeave(void);

#ifdef __cplusplus
}
#endif

#endif
