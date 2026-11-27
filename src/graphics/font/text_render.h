/*
 * text_render.h: draw registered native text inside the existing compositor.
 *
 * This is the only font module that knows about PICA/citro3d. It owns the
 * font texture and turns presentation records into quad runs. It does not
 * decide eligibility (text_window does) and does not submit a new frame
 * (g2d does).
 */

#ifndef KJ_GRAPHICS_FONT_TEXT_RENDER_H
#define KJ_GRAPHICS_FONT_TEXT_RENDER_H

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

#include "../pica/targets.h"

#ifdef __cplusplus
extern "C" {
#endif

bool kj_text_render_init(void);
void kj_text_render_exit(void);
C3D_Tex *kj_text_texture(void);
bool kj_text_texture_ready(void);

/* Draw every native presentation for (engine, bg). `palette` is the live
 * 256-colour BG palette for the engine; `hofs`/`vofs` are the BG scroll. The
 * caller has already set the layer's scissor and priority slot. */
void kj_text_draw_bg(const kj_target *target, int engine, int bg, const uint16_t *palette,
                     int hofs, int vofs);

#ifdef __cplusplus
}
#endif

#endif
