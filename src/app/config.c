/*
 * config.c: startup-only SD-card configuration for the overworld renderer.
 *
 *   sdmc:/3ds/kohjoh/config.txt
 *       # Overworld rendering resolution.
 *       # Valid values: legacy, 1x, 2x
 *       field_scale = 1x
 *
 * The parser is deliberately independent of libctru so tests/native can
 * compile it directly. kj_config_load() is called exactly once from main()
 * before kj_renderer_init(); rendering never reads the SD card.
 */

#include "app/config.h"

#include "diagnostics/log.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define KJ_CONFIG_MAX_FILE 4096

static kj_field_scale s_active = KJ_FIELD_SCALE_LEGACY;

static char lower_ascii(char c)
{
    return (char)tolower((unsigned char)c);
}

static size_t trim_right(const char *s, size_t n)
{
    while (n != 0 && isspace((unsigned char)s[n - 1])) {
        --n;
    }
    return n;
}

static bool equals_ascii(const char *a, size_t an, const char *b)
{
    size_t i = 0;
    for (; i < an && b[i] != '\0'; ++i) {
        if (lower_ascii(a[i]) != lower_ascii(b[i])) {
            return false;
        }
    }
    return i == an && b[i] == '\0';
}

kj_config_status kj_config_parse(const char *text, size_t length,
                                 kj_field_scale *requested, kj_field_scale *effective)
{
    bool found = false;
    size_t pos = 0;
    kj_field_scale chosen = KJ_FIELD_SCALE_LEGACY;

    if (requested != NULL) {
        *requested = KJ_FIELD_SCALE_LEGACY;
    }
    if (effective != NULL) {
        *effective = KJ_FIELD_SCALE_LEGACY;
    }
    if (text == NULL) {
        return KJ_CONFIG_UNRECOGNIZED;
    }

    while (pos < length) {
        size_t end = pos;
        while (end < length && text[end] != '\n') {
            ++end;
        }
        size_t beg = pos;
        size_t n = trim_right(text + beg, end - beg); /* drops '\r' of CRLF */
        while (n != 0 && isspace((unsigned char)text[beg])) {
            ++beg;
            --n;
        }
        n = trim_right(text + beg, n);
        pos = end + 1; /* also correct for the final, unterminated line */

        if (n == 0 || text[beg] == '#') {
            continue;
        }
        /* Inline comments are accepted as well as whole-line comments. */
        for (size_t i = 0; i < n; ++i) {
            if (text[beg + i] == '#') {
                n = trim_right(text + beg, i);
                break;
            }
        }
        if (n == 0) {
            continue;
        }
        size_t eq = 0;
        while (eq < n && text[beg + eq] != '=') {
            ++eq;
        }
        if (eq == n) {
            continue; /* unknown, non-key line: ignore, never fail startup */
        }
        const char *key = text + beg;
        size_t key_n = trim_right(key, eq);
        while (key_n != 0 && isspace((unsigned char)*key)) {
            ++key;
            --key_n;
        }
        if (!equals_ascii(key, key_n, "field_scale")) {
            continue;
        }
        const char *value = text + beg + eq + 1;
        size_t value_n = n - eq - 1;
        while (value_n != 0 && isspace((unsigned char)*value)) {
            ++value;
            --value_n;
        }
        value_n = trim_right(value, value_n);
        if (equals_ascii(value, value_n, "legacy")) {
            chosen = KJ_FIELD_SCALE_LEGACY;
        } else if (equals_ascii(value, value_n, "1x")) {
            chosen = KJ_FIELD_SCALE_1X;
        } else if (equals_ascii(value, value_n, "2x")) {
            chosen = KJ_FIELD_SCALE_2X;
        } else {
            /* An unrecognised value is reported, but never aborts startup. */
            continue;
        }
        found = true; /* last valid entry wins */
    }

    if (requested != NULL) {
        *requested = found ? chosen : KJ_FIELD_SCALE_LEGACY;
    }
    if (effective != NULL) {
        *effective = found ? chosen : KJ_FIELD_SCALE_LEGACY;
    }
    return found ? KJ_CONFIG_OK : KJ_CONFIG_UNRECOGNIZED;
}

const char *kj_config_scale_name(kj_field_scale mode)
{
    switch (mode) {
    case KJ_FIELD_SCALE_1X:
        return "1x";
    case KJ_FIELD_SCALE_2X:
        return "2x";
    case KJ_FIELD_SCALE_LEGACY:
    default:
        return "legacy";
    }
}

kj_config_status kj_config_load(const char *path, kj_field_scale *requested,
                                kj_field_scale *effective)
{
    kj_field_scale want = KJ_FIELD_SCALE_LEGACY, use = KJ_FIELD_SCALE_LEGACY;
    kj_config_status status;
    char buf[KJ_CONFIG_MAX_FILE + 1];
    size_t got = 0;

    if (requested != NULL) {
        *requested = KJ_FIELD_SCALE_LEGACY;
    }
    if (effective != NULL) {
        *effective = KJ_FIELD_SCALE_LEGACY;
    }
    if (path == NULL) {
        status = KJ_CONFIG_MISSING;
    } else {
        FILE *f = fopen(path, "rb");
        if (f == NULL) {
            status = KJ_CONFIG_MISSING;
        } else {
            got = fread(buf, 1, sizeof(buf) - 1, f);
            if (ferror(f)) {
                status = KJ_CONFIG_READ_ERROR;
            } else {
                size_t extra = 0;
                char c;
                if (fread(&c, 1, 1, f) == 1) {
                    extra = 1;
                }
                status = extra ? KJ_CONFIG_READ_ERROR : KJ_CONFIG_OK;
            }
            fclose(f);
        }
    }

    if (status == KJ_CONFIG_OK) {
        status = kj_config_parse(buf, got, &want, &use);
    } else {
        want = use = KJ_FIELD_SCALE_LEGACY;
    }

    s_active = use;
    if (requested != NULL) {
        *requested = want;
    }
    if (effective != NULL) {
        *effective = use;
    }
    kj_log("[config] %s: requested %s, initialized %s (%s)",
           path != NULL ? path : "(no path)", kj_config_scale_name(want),
           kj_config_scale_name(use),
           status == KJ_CONFIG_MISSING ? "no file" :
           status == KJ_CONFIG_OK ? "ok" :
           status == KJ_CONFIG_UNRECOGNIZED ? "no valid field_scale" : "read error");
    return status;
}

kj_field_scale kj_config_field_scale(void)
{
    return s_active;
}

bool kj_config_requires_offscreen(void)
{
    return s_active == KJ_FIELD_SCALE_2X;
}
