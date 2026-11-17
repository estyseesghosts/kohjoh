#ifndef KJ_RUNTIME_KJ_SCREEN_H
#define KJ_RUNTIME_KJ_SCREEN_H

#include "kj_bridge.h"

/* The screen on display and its layout, for kj_frame.c's publish. */
void kj_screen_snapshot(kj_screen_state *out);

#endif
