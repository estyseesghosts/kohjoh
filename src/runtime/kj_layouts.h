#ifndef KJ_RUNTIME_KJ_LAYOUTS_H
#define KJ_RUNTIME_KJ_LAYOUTS_H

#include "kj_bridge.h"

/* The built-in wide layout rows of a screen, into state->rows/row_count
 * (none: the screen draws as the DS does). */
void kj_layouts_for(uint32_t screen_id, kj_screen_state *state);

#endif
