#ifndef KJ_GRAPHICS_FONT_FONTBENCH_H
#define KJ_GRAPHICS_FONT_FONTBENCH_H

#include "../pica/targets.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Development-only specimen overlay; the caller checks for a `fontbench`
 * line in script.txt. Draws over the finished top frame at 1:1 physical
 * pixels. A no-op when the atlas is not ready. */
void kj_fontbench_draw(const kj_target *top);

#ifdef __cplusplus
}
#endif

#endif
