#ifndef KJ_APP_CONFIG_H
#define KJ_APP_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

/* Overworld rendering mode. Legacy is the default and the safe fallback for
 * every failure mode; see docs/engineering/hgss-remaster/FIELD_SCALING.md. */
typedef enum {
    KJ_FIELD_SCALE_LEGACY,
    KJ_FIELD_SCALE_1X,
    KJ_FIELD_SCALE_2X,
} kj_field_scale;

/* Native text rendering. `legacy` is the DS glyph pipeline and the default:
 * it is complete, authoritative and never goes away. `native` asks the 3DS
 * runtime to redraw audited text windows from the embedded Galmuri14 atlas;
 * any window it cannot reproduce falls back to the DS pixels beneath it.
 * See docs/engineering/hgss-remaster/NATIVE_TEXT.md. */
typedef enum {
    KJ_TEXT_RENDERER_LEGACY,
    KJ_TEXT_RENDERER_NATIVE,
} kj_text_renderer;

typedef enum {
    KJ_CONFIG_OK = 0,       /* a valid field_scale value was found */
    KJ_CONFIG_MISSING,      /* no configuration file (default applies) */
    KJ_CONFIG_UNRECOGNIZED, /* file readable, but no valid field_scale value */
    KJ_CONFIG_READ_ERROR,   /* file present but could not be read */
} kj_config_status;

/* Pure parser, usable by native tests. `text` need not be NUL terminated.
 * `requested` is the last valid mode seen; `effective` is the mode that will
 * actually be used (legacy unless a valid value was found). */
kj_config_status kj_config_parse(const char *text, size_t length, kj_field_scale *requested,
                                 kj_field_scale *effective);
const char *kj_config_scale_name(kj_field_scale mode);

/* Same file and same grammar, independent key and independent validation:
 * a bad `text_renderer` value never changes the field scale and vice versa.
 * `*requested` is what the file asked for; `*effective` is what will be used
 * (legacy unless `native` was seen and the build has an atlas). The atlas
 * check happens at renderer init, not here. */
kj_config_status kj_config_parse_text(const char *text, size_t length, kj_text_renderer *requested,
                                      kj_text_renderer *effective);
const char *kj_config_text_name(kj_text_renderer mode);

/* Read sdmc:/3ds/kohjoh/config.txt once during startup and publish the
 * effective modes. Never touches the SD card again during rendering. */
kj_config_status kj_config_load(const char *path, kj_field_scale *requested,
                                kj_field_scale *effective);
kj_field_scale kj_config_field_scale(void);
bool kj_config_requires_offscreen(void);

kj_text_renderer kj_config_text_renderer(void);
/* True only when the file asked for native *and* the build can honour it.
 * The renderer still applies its per-window fallback policy. */
bool kj_config_native_text_requested(void);

#endif
