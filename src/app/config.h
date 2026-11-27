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

typedef enum {
    KJ_CONFIG_OK = 0,       /* a valid field_scale value was found */
    KJ_CONFIG_MISSING,      /* no configuration file (default applies) */
    KJ_CONFIG_UNRECOGNIZED, /* file readable, but no valid field_scale value */
    KJ_CONFIG_READ_ERROR,   /* file present but could not be read */
} kj_config_status;

/* Pure parser, usable by native tests. `text` need not be NUL terminated.
 * `requested` is the last valid mode seen; `effective` is the mode that will
 * actually be used (legacy unless a valid value was found). */
kj_config_status kj_config_parse(const char *text, size_t length,
                                 kj_field_scale *requested, kj_field_scale *effective);
const char *kj_config_scale_name(kj_field_scale mode);

/* Read sdmc:/3ds/kohjoh/config.txt once during startup and publish the
 * effective mode. Never touches the SD card again. */
kj_config_status kj_config_load(const char *path, kj_field_scale *requested,
                                kj_field_scale *effective);
kj_field_scale kj_config_field_scale(void);
bool kj_config_requires_offscreen(void);

#endif
