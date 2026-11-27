/*
 * kj_text.h: DS-side capture of resolved text operations.
 *
 * The game keeps owning the text state machine, the window pixel buffers and
 * every VRAM transfer. This module only records what the game resolved:
 * which glyph, where, in which colours, into which window, and when that
 * window's pixels were published. The 3DS side reproduces the text natively;
 * the original path still runs unchanged underneath.
 */

#ifndef KJ_TEXT_H
#define KJ_TEXT_H

#include "kj_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Window;

/* Called from render_text.c immediately after CopyGlyphToWindow. */
void NitroPort_TextGlyph(struct Window *window, unsigned int code, unsigned int fontId, int x,
                         int y, unsigned int fg, unsigned int shadow, unsigned int bg,
                         unsigned int glyphTable);

/* Window lifecycle / mutations, called from bg_window.c. */
void NitroPort_TextWindowAdd(struct Window *window);
void NitroPort_TextWindowAddTemporary(struct Window *window);
void NitroPort_TextWindowFillRect(struct Window *window, unsigned int fill, int x, int y, int width,
                                  int height);
void NitroPort_TextWindowBlit(struct Window *window);
void NitroPort_TextWindowRemove(struct Window *window);
void NitroPort_TextWindowPublish(struct Window *window);
void NitroPort_TextWindowFill(struct Window *window, unsigned int fill);
void NitroPort_TextWindowScroll(struct Window *window, int dx, int dy, unsigned int fill);

/* The coherent event list for the frame being published. `*count` is 0 when
 * nothing was captured or the buffer overflowed (the router then keeps the
 * legacy presentation for the whole frame). */
const kj_text_event *NitroPort_TextEvents(uint32_t *count);

#ifdef __cplusplus
}
#endif

#endif
