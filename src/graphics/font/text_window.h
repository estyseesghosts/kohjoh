/*
 * text_window.h: the text presentation registry and eligibility policy.
 *
 * The game still owns every window, pixel buffer and VRAM transfer. This
 * registry keeps a generation-aware copy of the *resolved* text operations
 * for windows that may be presented natively, decides atomically whether a
 * complete text object can be represented, and exposes the result to the
 * compositor.
 *
 * The legacy path is always authoritative. A record either presents natively
 * or leaves the original pixels alone; it never hides both.
 */

#ifndef KJ_GRAPHICS_FONT_TEXT_WINDOW_H
#define KJ_GRAPHICS_FONT_TEXT_WINDOW_H

#include <stdbool.h>
#include <stdint.h>

#include "glyph_atlas.h"
#include "../../runtime/kj_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KJ_TEXT_LEGACY = 0,
    KJ_TEXT_NATIVE_BG,
    KJ_TEXT_NATIVE_OBJ,
    KJ_TEXT_NATIVE_HPBOX,
} kj_text_policy;

/* Why a record could not be represented natively. Stable values for logs. */
typedef enum {
    KJ_TEXT_OK = 0,
    KJ_TEXT_FALLBACK_DISABLED,
    KJ_TEXT_FALLBACK_NO_ATLAS,
    KJ_TEXT_FALLBACK_TEMPORARY_WINDOW,
    KJ_TEXT_FALLBACK_UNSUPPORTED_LAYER,
    KJ_TEXT_FALLBACK_UNSUPPORTED_GLYPH,
    KJ_TEXT_FALLBACK_ALTERNATE_TABLE,
    KJ_TEXT_FALLBACK_NO_BACKING,
    KJ_TEXT_FALLBACK_CAPACITY,
    KJ_TEXT_FALLBACK_OVERFLOW,
    KJ_TEXT_FALLBACK_COUNT,
} kj_text_fallback;

typedef struct kj_text_stats {
    uint32_t glyphs_captured;
    uint32_t glyphs_native;
    uint32_t windows_native;
    uint32_t objects_native;
    uint32_t windows_fallback;
    uint32_t fallback_reason[KJ_TEXT_FALLBACK_COUNT];
    uint32_t unsupported_codes;
    uint32_t allocation_failures;
    uint32_t draws;
    uint32_t events_seen;
    uint32_t events_dropped;
} kj_text_stats;

typedef struct kj_text_glyph {
    uint16_t code;
    int16_t x, y;
    uint8_t font_id;
    uint8_t fg, shadow, bg;
} kj_text_glyph;

#define KJ_TEXT_MAX_WINDOWS 24
#define KJ_TEXT_MAX_GLYPHS_PER_WINDOW 256

typedef struct kj_text_presentation {
    bool valid;
    bool published;
    bool native;
    uint8_t policy;
    uint8_t fallback;
    uint32_t window;
    uint32_t generation;
    uint8_t engine, bg_id, palette_num, color_mode;
    uint8_t fill;
    uint8_t have_fill;
    uint8_t alternate;
    uint8_t poisoned;
    uint8_t temporary;
    uint16_t tilemap_x, tilemap_y, width, height;
    uint16_t glyph_count;
    kj_text_glyph glyphs[KJ_TEXT_MAX_GLYPHS_PER_WINDOW];
} kj_text_presentation;

void kj_text_init(void);
void kj_text_exit(void);

/* Configuration (Task 6). `native` is the opt-in switch; the default is off. */
void kj_text_set_enabled(bool native);
bool kj_text_enabled(void);

/* Apply one published frame's event list. Must run before the compositor. */
void kj_text_frame(const kj_frame_input *in);

/* Presentation lookup for the compositor. `engine` is 0/1, `bg` 0..3.
 * Returns the next valid record at or after `cursor`, or NULL. */
const kj_text_presentation *kj_text_next(int engine, int bg, int *cursor);

/* Convert a DS 4bpp/8bpp palette index into RGBA8 using the live BG palette. */
uint32_t kj_text_ds_color(const uint16_t *palette, uint8_t color_mode, uint8_t palette_num,
                          uint8_t index);

/* True when a DS tile at screen (x, y) on this plane belongs to a window that
 * this frame presents natively: the compositor must not draw its legacy
 * pixels. */
bool kj_text_owns_tile(int engine, int bg, int hofs, int vofs, int x, int y);

const kj_text_stats *kj_text_stats_read(void);
void kj_text_stats_reset(void);
void kj_text_note_draw(void);

/* Log a rate-limited fallback line (development builds). */
void kj_text_log_fallback(kj_text_fallback reason, const kj_text_event *event);

#ifdef __cplusplus
}
#endif

#endif
