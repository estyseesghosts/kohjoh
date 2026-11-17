#ifndef KJ_GRAPHICS_LAYOUT_OVERRIDE_H
#define KJ_GRAPHICS_LAYOUT_OVERRIDE_H

#include <stdbool.h>
#include <stdint.h>

#include "runtime/kj_bridge.h"

/* Read the development wide-layout override (absent: nothing changes). */
void kj_layout_override_load(const char *path);

/* The override rows of a screen and engine; false if the screen is not in
 * the override (the built-in rows apply). */
bool kj_layout_override_get(uint32_t id, int engine, const kj_layout_row **rows, uint32_t *count);

#endif
