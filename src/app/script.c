/*
 * script.c: see script.h.
 */

#include "app/script.h"

#include "diagnostics/log.h"
#include "runtime/kj_field_view.h"

#include "compat/nitro/include/nitro_port_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS 1024
#define MAX_MARKS 64

typedef struct event {
    uint32_t vblank;
    uint32_t length; /* V-blanks the press lasts (0: `every`) */
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
static uint32_t s_debug;
static uint32_t s_now;
static uint64_t s_input_end; /* exclusive; wider than a V-blank counter */
static uint32_t s_battle_frame; /* 0: no `battle:` request */
static unsigned s_battle_t1, s_battle_t2;
static int s_battle_bg = -1, s_battle_terrain = -1; /* -1: the map's */

/* Parse decimal fields without accepting signs, junk or uint32_t overflow. */
static bool parse_decimal(const char *text, const char **end, uint32_t *value)
{
    uint32_t n = 0;
    const char *p = text;
    if (*p < '0' || *p > '9') {
        return false;
    }
    while (*p >= '0' && *p <= '9') {
        const uint32_t digit = (uint32_t)(*p - '0');
        if (n > (UINT32_MAX - digit) / 10u) {
            return false;
        }
        n = n * 10u + digit;
        ++p;
    }
    *value = n;
    *end = p;
    return true;
}

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
            const char *end;
            uint32_t value;
            if (tok != NULL && parse_decimal(tok, &end, &value) && *end == '\0' && value != 0) {
                s_every = value;
            } else {
                kj_log("[script] ignored invalid every duration");
            }
        } else if (strcmp(tok, "debug:") == 0) {
            while ((tok = strtok(NULL, " \t\r\n")) != NULL) {
                s_debug |= (uint32_t)strtoul(tok, NULL, 0);
            }
            /* The visibility A/B switch lives with the field-view policy so
             * the DS-side geometry engine does not depend on the SD parser. */
            kj_field_view_set_vis_cull_enabled((s_debug & KJ_DEBUG_NO_VIS_CULL) == 0);
            kj_field_view_set_box_cull_enabled((s_debug & KJ_DEBUG_NO_BOX_CULL) == 0);
        } else if (strcmp(tok, "end:") == 0) {
            tok = strtok(NULL, " \t\r\n");
            s_end = tok ? (uint32_t)strtoul(tok, NULL, 10) : 0;
        } else if (strcmp(tok, "battle:") == 0) {
            char *f = strtok(NULL, " \t\r\n");
            char *a = strtok(NULL, " \t\r\n");
            char *b = strtok(NULL, " \t\r\n");
            char *bg = strtok(NULL, " \t\r\n");
            char *ter = strtok(NULL, " \t\r\n");
            if (bg != NULL && ter != NULL) {
                s_battle_bg = (int)strtol(bg, NULL, 10);
                s_battle_terrain = (int)strtol(ter, NULL, 10);
            }
            if (f != NULL && a != NULL) {
                s_battle_frame = (uint32_t)strtoul(f, NULL, 10);
                s_battle_t1 = (unsigned)strtoul(a, NULL, 10);
                s_battle_t2 = b ? (unsigned)strtoul(b, NULL, 10) : 0;
            }
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
            event parsed = { 0 };
            const char *end;
            if (!parse_decimal(tok, &end, &parsed.vblank)) {
                kj_log("[script] ignored invalid input event");
                continue;
            }
            if (*end == '-') {
                uint32_t last;
                if (!parse_decimal(end + 1, &end, &last) || *end != '\0' ||
                    last <= parsed.vblank) {
                    kj_log("[script] ignored invalid input interval");
                    continue;
                }
                parsed.length = last - parsed.vblank;
            } else if (*end != '\0') {
                kj_log("[script] ignored invalid input event");
                continue;
            }
            bool has_input = false;
            while ((tok = strtok(NULL, " \t\r\n")) != NULL) {
                unsigned x, y;
                if (sscanf(tok, "TOUCH:%u,%u", &x, &y) == 2) {
                    parsed.touch = true;
                    parsed.tx = (uint16_t)x;
                    parsed.ty = (uint16_t)y;
                    has_input = true;
                } else {
                    const uint16_t bit = key_bit(tok);
                    parsed.keys |= bit;
                    has_input |= bit != 0 || strcmp(tok, "-") == 0;
                }
            }
            if (has_input) {
                s_events[s_event_count++] = parsed;
            } else {
                kj_log("[script] ignored input event without keys or touch");
            }
        }
    }
    fclose(f);
    /* `every:` applies to all default-length events, even if it came later. */
    s_input_end = 0;
    for (unsigned i = 0; i < s_event_count; ++i) {
        const event *e = &s_events[i];
        const uint32_t len = e->length != 0 ? e->length : s_every;
        const uint64_t end = (uint64_t)e->vblank + len;
        if (end > s_input_end) {
            s_input_end = end;
        }
    }
    kj_log("[script] %u input events, %u frame marks, every %lu, end %lu, debug 0x%lx",
           s_event_count, s_mark_count, (unsigned long)s_every, (unsigned long)s_end,
           (unsigned long)s_debug);
}

bool kj_script_active(void)
{
    return s_active;
}

bool kj_script_input(uint32_t vblank, uint16_t *keys, bool *touch, uint16_t *tx, uint16_t *ty)
{
    s_now = vblank;
    /* Only a script with input events owns the pad: one that sets only
     * debug flags, marks or pacing leaves live input working. */
    if (!s_active || s_event_count == 0) {
        return false;
    }
    /* After its last event the script hands the pad back, so a scripted
     * drive to a scene can be continued by hand or over LOTUS MCP. */
    if ((uint64_t)vblank >= s_input_end) {
        return false;
    }
    *keys = 0;
    *touch = false;
    for (unsigned i = 0; i < s_event_count; ++i) {
        const event *e = &s_events[i];
        const uint32_t len = e->length != 0 ? e->length : s_every;
        if (vblank >= e->vblank && (uint64_t)vblank < (uint64_t)e->vblank + len) {
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

uint32_t kj_script_debug(void)
{
    return s_debug;
}

uint32_t kj_script_end(void)
{
    return s_end;
}

int NitroPort_DebugBattle(unsigned *trainer1, unsigned *trainer2)
{
    if (s_battle_frame == 0 || s_now < s_battle_frame) {
        return 0;
    }
    *trainer1 = s_battle_t1;
    *trainer2 = s_battle_t2;
    s_battle_frame = 0;
    kj_log("[script] battle: trainer %u/%u at v-blank %lu", s_battle_t1, s_battle_t2,
           (unsigned long)s_now);
    return 1;
}

int NitroPort_DebugBattleBackdrop(int *bg, int *terrain)
{
    if (s_battle_bg < 0 || s_battle_terrain < 0) {
        return 0;
    }
    *bg = s_battle_bg;
    *terrain = s_battle_terrain;
    s_battle_bg = s_battle_terrain = -1;
    return 1;
}

int NitroPort_DebugPropVisibility(unsigned propIndex, int x, int y, int z, int boxResult)
{
    if (s_debug & KJ_DEBUG_PROPVIS_LOG) {
        kj_log("[propvis] id=%u x=%d y=%d z=%d box=%d", (unsigned)propIndex, x, y, z, boxResult);
    }
    return (s_debug & KJ_DEBUG_PROPVIS_BYPASS) != 0;
}

void NitroPort_DebugBillboard(unsigned model, unsigned type, unsigned sequence, unsigned frame,
                              unsigned texKey, int x, int y, int z, int sx, int sy, int sz)
{
    if (s_debug & KJ_DEBUG_BILLBOARD_LOG) {
        kj_log("[billboard] model=%u type=%u seq=%u frame=%u tex=%u "
               "pos=%d,%d,%d scale=%d,%d,%d",
               model, type, sequence, frame, texKey, x, y, z, sx, sy, sz);
    }
}

void NitroPort_DebugMapResidency(unsigned matrixIndex, unsigned tile, unsigned quadrant,
                                  int slot, int slotMatrix)
{
    if (s_debug & KJ_DEBUG_MAPRES_LOG) {
        kj_log("[mapres] tracked=%u tile=%u quad=%u slot=%d matrix=%d",
               matrixIndex, tile, quadrant, slot, slotMatrix);
    }
}
