/*
 * script.c: see script.h.
 */

#include "app/script.h"

#include "diagnostics/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS 1024
#define MAX_MARKS 64

typedef struct event {
    uint32_t vblank;
    uint16_t keys;
    bool touch;
    uint16_t tx, ty;
} event;

typedef struct mark {
    uint32_t frame;
    uint32_t seconds; /* freeze */
    bool dump;
} mark;

static event s_events[MAX_EVENTS];
static unsigned s_event_count;
static mark s_marks[MAX_MARKS];
static unsigned s_mark_count;
static uint32_t s_every = 10;
static uint32_t s_end;
static bool s_active;

static uint16_t key_bit(const char *k)
{
    static const struct {
        const char *name;
        uint16_t bit;
    } KEYS[] = {
        { "A", 0x001 },     { "B", 0x002 },     { "SELECT", 0x004 }, { "START", 0x008 },
        { "RIGHT", 0x010 }, { "LEFT", 0x020 },  { "UP", 0x040 },     { "DOWN", 0x080 },
        { "R", 0x100 },     { "L", 0x200 },     { "X", 0x400 },      { "Y", 0x800 },
    };
    for (size_t i = 0; i < sizeof(KEYS) / sizeof(KEYS[0]); ++i) {
        if (strcmp(k, KEYS[i].name) == 0) {
            return KEYS[i].bit;
        }
    }
    return 0;
}

static mark *mark_for(uint32_t frame)
{
    for (unsigned i = 0; i < s_mark_count; ++i) {
        if (s_marks[i].frame == frame) {
            return &s_marks[i];
        }
    }
    if (s_mark_count >= MAX_MARKS) {
        return NULL;
    }
    mark *m = &s_marks[s_mark_count++];
    memset(m, 0, sizeof(*m));
    m->frame = frame;
    return m;
}

void kj_script_load(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[256];

    if (f == NULL) {
        return;
    }
    s_active = true;
    while (fgets(line, sizeof(line), f) != NULL) {
        char *hash = strchr(line, '#');
        if (hash != NULL) {
            *hash = '\0';
        }
        char *tok = strtok(line, " \t\r\n");
        if (tok == NULL) {
            continue;
        }
        if (strcmp(tok, "every:") == 0) {
            tok = strtok(NULL, " \t\r\n");
            s_every = tok ? (uint32_t)strtoul(tok, NULL, 10) : s_every;
        } else if (strcmp(tok, "end:") == 0) {
            tok = strtok(NULL, " \t\r\n");
            s_end = tok ? (uint32_t)strtoul(tok, NULL, 10) : 0;
        } else if (strcmp(tok, "freeze") == 0 || strcmp(tok, "dump") == 0) {
            const bool dump = tok[0] == 'd';
            char *a = strtok(NULL, " \t\r\n");
            char *b = strtok(NULL, " \t\r\n");
            mark *m = a ? mark_for((uint32_t)strtoul(a, NULL, 10)) : NULL;
            if (m != NULL) {
                if (dump) {
                    m->dump = true;
                } else {
                    m->seconds = b ? (uint32_t)strtoul(b, NULL, 10) : 5;
                }
            }
        } else if (s_event_count < MAX_EVENTS) {
            event *e = &s_events[s_event_count++];
            memset(e, 0, sizeof(*e));
            e->vblank = (uint32_t)strtoul(tok, NULL, 10);
            while ((tok = strtok(NULL, " \t\r\n")) != NULL) {
                unsigned x, y;
                if (sscanf(tok, "TOUCH:%u,%u", &x, &y) == 2) {
                    e->touch = true;
                    e->tx = (uint16_t)x;
                    e->ty = (uint16_t)y;
                } else {
                    e->keys |= key_bit(tok);
                }
            }
        }
    }
    fclose(f);
    kj_log("[script] %u input events, %u frame marks, every %lu, end %lu", s_event_count,
           s_mark_count, (unsigned long)s_every, (unsigned long)s_end);
}

bool kj_script_active(void)
{
    return s_active;
}

bool kj_script_input(uint32_t vblank, uint16_t *keys, bool *touch, uint16_t *tx, uint16_t *ty)
{
    if (!s_active) {
        return false;
    }
    *keys = 0;
    *touch = false;
    for (unsigned i = 0; i < s_event_count; ++i) {
        const event *e = &s_events[i];
        if (vblank >= e->vblank && vblank < e->vblank + s_every) {
            *keys |= e->keys;
            if (e->touch) {
                *touch = true;
                *tx = e->tx;
                *ty = e->ty;
            }
        }
    }
    return true;
}

uint32_t kj_script_freeze(uint32_t frame)
{
    for (unsigned i = 0; i < s_mark_count; ++i) {
        if (s_marks[i].frame == frame) {
            return s_marks[i].seconds;
        }
    }
    return 0;
}

bool kj_script_dump(uint32_t frame)
{
    for (unsigned i = 0; i < s_mark_count; ++i) {
        if (s_marks[i].frame == frame) {
            return s_marks[i].dump;
        }
    }
    return false;
}

uint32_t kj_script_end(void)
{
    return s_end;
}
