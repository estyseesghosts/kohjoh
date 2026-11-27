/*
 * override.c: sdmc:/3ds/kohjoh/wide_layouts.txt, the development override
 * of the built-in wide layouts (issue #17). Same format as Unown's
 * wide_layouts.txt, so tuned rows move between the ports unchanged:
 *     screen party
 *     B move 0x2 0 192 0 20 -32      (engine, kind, bgs top bottom x0 x1 dx)
 *     B fill 0x2 0 192 -12 52 21     (fill / copy: ... x0 x1 column)
 *     A objs 0 192 128 256 32        (top bottom x0 x1 dx)
 *     A wrap 0x18 0 192              (bgs top bottom)
 *     A shift3d 32
 * A screen listed here uses only these rows (an empty section draws it as
 * the DS does). Read once at start; release builds ship without the file.
 */

#include "graphics/layout/override.h"

#include "diagnostics/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SCREENS 16

typedef struct screen_rows {
    uint32_t id;
    uint32_t count[2];
    kj_layout_row rows[2][KJ_LAYOUT_MAX_ROWS];
} screen_rows;

static screen_rows s_screens[MAX_SCREENS];
static uint32_t s_screen_count;

static int screen_id(const char *name) {
    static const char *const names[KJ_SCREEN_COUNT] = KJ_SCREEN_NAMES_INIT;
    for (uint32_t i = 0; i < KJ_SCREEN_COUNT; ++i) {
        if (strcmp(names[i], name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static int16_t kind_of(const char *k) {
    static const struct {
        const char *name;
        int16_t kind;
    } KINDS[] = {
        {"move", KJ_LAYOUT_MOVE},       {"fill", KJ_LAYOUT_FILL}, {"objs", KJ_LAYOUT_OBJS},
        {"shift3d", KJ_LAYOUT_SHIFT3D}, {"copy", KJ_LAYOUT_COPY}, {"wrap", KJ_LAYOUT_WRAP},
    };
    for (size_t i = 0; i < sizeof(KINDS) / sizeof(KINDS[0]); ++i) {
        if (strcmp(KINDS[i].name, k) == 0) {
            return KINDS[i].kind;
        }
    }
    return 0;
}

/* Values in the row's own field order: bgs top bottom x0 x1 (dx | col). */
static bool parse_row(int16_t kind, const long *v, int n, kj_layout_row *r) {
    memset(r, 0, sizeof(*r));
    r->kind = kind;
    switch (kind) {
    case KJ_LAYOUT_MOVE:
    case KJ_LAYOUT_FILL:
    case KJ_LAYOUT_COPY:
        if (n < 6) {
            return false;
        }
        r->bgs = (int16_t)v[0];
        r->top = (int16_t)v[1];
        r->bottom = (int16_t)v[2];
        r->x0 = (int16_t)v[3];
        r->x1 = (int16_t)v[4];
        if (kind == KJ_LAYOUT_MOVE) {
            r->dx = (int16_t)v[5];
        } else {
            r->arg = (int16_t)v[5];
        }
        return true;
    case KJ_LAYOUT_OBJS:
        if (n < 5) {
            return false;
        }
        r->top = (int16_t)v[0];
        r->bottom = (int16_t)v[1];
        r->x0 = (int16_t)v[2];
        r->x1 = (int16_t)v[3];
        r->dx = (int16_t)v[4];
        return true;
    case KJ_LAYOUT_WRAP:
        if (n < 3) {
            return false;
        }
        r->bgs = (int16_t)v[0];
        r->top = (int16_t)v[1];
        r->bottom = (int16_t)v[2];
        return true;
    case KJ_LAYOUT_SHIFT3D:
        if (n < 1) {
            return false;
        }
        r->dx = (int16_t)v[0];
        return true;
    default:
        return false;
    }
}

void kj_layout_override_load(const char *path) {
    FILE *f = fopen(path, "r");
    char line[160];
    screen_rows *cur = NULL;
    unsigned bad = 0, rows = 0;

    if (f == NULL) {
        return;
    }
    while (fgets(line, sizeof(line), f) != NULL) {
        char *hash = strchr(line, '#');
        if (hash != NULL) {
            *hash = '\0';
        }
        char *tok = strtok(line, " \t\r\n");
        if (tok == NULL) {
            continue;
        }
        if (strcmp(tok, "screen") == 0) {
            const char *name = strtok(NULL, " \t\r\n");
            const int id = name ? screen_id(name) : -1;
            cur = NULL;
            if (id < 0 || s_screen_count >= MAX_SCREENS) {
                ++bad;
                continue;
            }
            cur = &s_screens[s_screen_count++];
            memset(cur, 0, sizeof(*cur));
            cur->id = (uint32_t)id;
            continue;
        }
        const int engine = tok[0] == 'A' ? 0 : tok[0] == 'B' ? 1 : -1;
        const char *k = strtok(NULL, " \t\r\n");
        const int16_t kind = k ? kind_of(k) : 0;
        long v[6];
        int n = 0;
        while (n < 6 && (tok = strtok(NULL, " \t\r\n")) != NULL) {
            v[n++] = strtol(tok, NULL, 0);
        }
        kj_layout_row r;
        if (cur == NULL || engine < 0 || cur->count[engine] >= KJ_LAYOUT_MAX_ROWS ||
            !parse_row(kind, v, n, &r)) {
            ++bad;
            continue;
        }
        cur->rows[engine][cur->count[engine]++] = r;
        ++rows;
    }
    fclose(f);
    kj_log("[layout] %s: %lu screens, %lu rows, %lu lines ignored", path,
           (unsigned long)s_screen_count, (unsigned long)rows, (unsigned long)bad);
}

bool kj_layout_override_get(uint32_t id, int engine, const kj_layout_row **rows, uint32_t *count) {
    for (uint32_t i = 0; i < s_screen_count; ++i) {
        if (s_screens[i].id == id) {
            *rows = s_screens[i].rows[engine];
            *count = s_screens[i].count[engine];
            return true;
        }
    }
    return false;
}
