/*
 * config.c: startup-only SD-card configuration for the 3DS renderer.
 *
 *   sdmc:/3ds/kohjoh/config.txt
 *       # Overworld rendering resolution.
 *       # Valid values: legacy, 1x, 2x
 *       field_scale = 1x
 *
 *       # Native Galmuri14 text. The DS renderer remains the default and the
 *       # fallback for every surface it cannot reproduce.
 *       # Valid values: legacy, native
 *       text_renderer = native
 *
 * The parser is deliberately independent of libctru so tests/native can
 * compile it directly. kj_config_load() is called exactly once from main()
 * before kj_renderer_init(); rendering never reads the SD card.
 *
 * The two keys are parsed and validated independently: a malformed
 * text_renderer value never changes the field scale and vice versa.
 */

#include "app/config.h"

#include "diagnostics/log.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define KJ_CONFIG_MAX_FILE 4096

static kj_field_scale s_active = KJ_FIELD_SCALE_LEGACY;
static kj_text_renderer s_text_active = KJ_TEXT_RENDERER_LEGACY;

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

/* One lexical entry of the file: `key = value`, with comments and blank
 * lines skipped. Keys that contain no '=' are skipped as unknown. */
typedef struct {
    const char *text;
    size_t length;
    size_t pos;
} config_reader;

static void reader_init(config_reader *r, const char *text, size_t length)
{
    r->text = text;
    r->length = length;
    r->pos = 0;
}

static bool reader_next(config_reader *r, const char **key, size_t *key_n,
                        const char **value, size_t *value_n)
{
    while (r->pos < r->length) {
        size_t end = r->pos;
        while (end < r->length && r->text[end] != '\n') {
            ++end;
        }
        size_t beg = r->pos;
        size_t n = trim_right(r->text + beg, end - beg); /* drops CR of CRLF */
        while (n != 0 && isspace((unsigned char)r->text[beg])) {
            ++beg;
            --n;
        }
        n = trim_right(r->text + beg, n);
        r->pos = end + 1; /* also correct for an unterminated final line */

        if (n == 0 || r->text[beg] == '#') {
            continue;
        }
        /* Inline comments are accepted as well as whole-line comments. */
        for (size_t i = 0; i < n; ++i) {
            if (r->text[beg + i] == '#') {
                n = trim_right(r->text + beg, i);
                break;
            }
        }
        if (n == 0) {
            continue;
        }
        size_t eq = 0;
        while (eq < n && r->text[beg + eq] != '=') {
            ++eq;
        }
        if (eq == n) {
            continue; /* not a key/value line */
        }
        const char *k = r->text + beg;
        size_t kn = trim_right(k, eq);
        while (kn != 0 && isspace((unsigned char)*k)) {
            ++k;
            --kn;
        }
        const char *v = r->text + beg + eq + 1;
        size_t vn = n - eq - 1;
        while (vn != 0 && isspace((unsigned char)*v)) {
            ++v;
            --vn;
        }
        vn = trim_right(v, vn);
        *key = k;
        *key_n = kn;
        *value = v;
        *value_n = vn;
        return true;
    }
    return false;
}

kj_config_status kj_config_parse(const char *text, size_t length,
                                 kj_field_scale *requested, kj_field_scale *effective)
{
    bool found = false;
    kj_field_scale chosen = KJ_FIELD_SCALE_LEGACY;
    config_reader r;
    const char *key, *value;
    size_t key_n, value_n;

    if (requested != NULL) {
        *requested = KJ_FIELD_SCALE_LEGACY;
    }
    if (effective != NULL) {
        *effective = KJ_FIELD_SCALE_LEGACY;
    }
    if (text == NULL) {
        return KJ_CONFIG_UNRECOGNIZED;
    }

    reader_init(&r, text, length);
    while (reader_next(&r, &key, &key_n, &value, &value_n)) {
        if (!equals_ascii(key, key_n, "field_scale")) {
            continue;
        }
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

kj_config_status kj_config_parse_text(const char *text, size_t length,
                                      kj_text_renderer *requested,
                                      kj_text_renderer *effective)
{
    bool found = false;
    kj_text_renderer chosen = KJ_TEXT_RENDERER_LEGACY;
    config_reader r;
    const char *key, *value;
    size_t key_n, value_n;

    if (requested != NULL) {
        *requested = KJ_TEXT_RENDERER_LEGACY;
    }
    if (effective != NULL) {
        *effective = KJ_TEXT_RENDERER_LEGACY;
    }
    if (text == NULL) {
        return KJ_CONFIG_UNRECOGNIZED;
    }

    reader_init(&r, text, length);
    while (reader_next(&r, &key, &key_n, &value, &value_n)) {
        if (!equals_ascii(key, key_n, "text_renderer")) {
            continue;
        }
        if (equals_ascii(value, value_n, "legacy")) {
            chosen = KJ_TEXT_RENDERER_LEGACY;
        } else if (equals_ascii(value, value_n, "native")) {
            chosen = KJ_TEXT_RENDERER_NATIVE;
        } else {
            continue;
        }
        found = true;
    }

    if (requested != NULL) {
        *requested = found ? chosen : KJ_TEXT_RENDERER_LEGACY;
    }
    if (effective != NULL) {
        *effective = found ? chosen : KJ_TEXT_RENDERER_LEGACY;
    }
    return found ? KJ_CONFIG_OK : KJ_CONFIG_UNRECOGNIZED;
}

const char *kj_config_text_name(kj_text_renderer mode)
{
    switch (mode) {
    case KJ_TEXT_RENDERER_NATIVE:
        return "native";
    case KJ_TEXT_RENDERER_LEGACY:
    default:
        return "legacy";
    }
}

kj_config_status kj_config_load(const char *path, kj_field_scale *requested,
                                kj_field_scale *effective)
{
    kj_field_scale want = KJ_FIELD_SCALE_LEGACY, use = KJ_FIELD_SCALE_LEGACY;
    kj_text_renderer twant = KJ_TEXT_RENDERER_LEGACY, tuse = KJ_TEXT_RENDERER_LEGACY;
    kj_config_status status;
    kj_config_status text_status = KJ_CONFIG_MISSING;
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
        text_status = kj_config_parse_text(buf, got, &twant, &tuse);
    } else {
        want = use = KJ_FIELD_SCALE_LEGACY;
        twant = tuse = KJ_TEXT_RENDERER_LEGACY;
    }

    s_active = use;
    s_text_active = tuse;
    if (requested != NULL) {
        *requested = want;
    }
    if (effective != NULL) {
        *effective = use;
    }
    kj_log("[config] %s: requested %s, initialized %s (%s)", path != NULL ? path : "(no path)",
           kj_config_scale_name(want), kj_config_scale_name(use),
           status == KJ_CONFIG_MISSING ? "no file" :
           status == KJ_CONFIG_OK ? "ok" :
           status == KJ_CONFIG_UNRECOGNIZED ? "no valid field_scale" : "read error");
    kj_log("[config] text_renderer: requested %s, initialized %s (%s)",
           kj_config_text_name(twant), kj_config_text_name(tuse),
           text_status == KJ_CONFIG_MISSING ? "no file" :
           text_status == KJ_CONFIG_OK ? "ok" :
           text_status == KJ_CONFIG_UNRECOGNIZED ? "no valid text_renderer" :
                                                  "read error");
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

kj_text_renderer kj_config_text_renderer(void)
{
    return s_text_active;
}

bool kj_config_native_text_requested(void)
{
    return s_text_active == KJ_TEXT_RENDERER_NATIVE;
}
