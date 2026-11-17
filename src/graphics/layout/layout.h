#ifndef KJ_GRAPHICS_LAYOUT_LAYOUT_H
#define KJ_GRAPHICS_LAYOUT_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

#include "runtime/kj_bridge.h"

/* Wide-layout planning for text BGs (issue #16): which screen column of the
 * DS picture each wide column -32..287 shows, as a few segments the
 * renderer draws by re-placing the layer's retained quads under a scissor.
 * Pure C: no GPU, no game state (tests/native/layout_test.c). */

#define KJ_LAYOUT_LEFT  (-32)
#define KJ_LAYOUT_RIGHT 288
/* Text BG quads are emitted for screen columns [-64, 320) while a layout
 * is active, so every source column a segment names exists. */
#define KJ_LAYOUT_SRC_LO (-64)
#define KJ_LAYOUT_SRC_HI 320
#define KJ_LAYOUT_MAX_SEGS 32

/* Wide columns [s0, s1) show source screen columns: stretch = 0, sx + (s -
 * s0); stretch = 1, column sx repeated. Source columns are where the
 * layer's quads lie (picture column minus scroll). */
typedef struct kj_layout_seg {
    int16_t s0, s1, sx;
    uint8_t stretch;
} kj_layout_seg;

/* The BG geometry a plan depends on. */
typedef struct kj_layout_bg {
    int bg;        /* 0..3 */
    int hofs, vofs;
    int w, h;      /* map size in pixels */
} kj_layout_bg;

/* Bit k set: rows[k] applies to this BG on screen line y. */
uint32_t kj_layout_active(const kj_layout_row *rows, uint32_t n, const kj_layout_bg *bg, int y);

/* The segments of a line whose active row set is `active`. Returns the
 * count; *unsupported is set when a source column has no quads (512-wide
 * maps scrolled far) or the segments overflowed. */
int kj_layout_segments(const kj_layout_row *rows, uint32_t n, uint32_t active,
                       const kj_layout_bg *bg, kj_layout_seg *out, bool *unsupported);

/* The x move of an OBJ whose middle is (cx, cy), from the OBJS rows. */
int kj_layout_obj_dx(const kj_layout_row *rows, uint32_t n, int cx, int cy);

/* The SHIFT3D move (0 without one). */
int kj_layout_shift3d(const kj_layout_row *rows, uint32_t n);

#endif
