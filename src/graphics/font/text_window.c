#include "text_window.h"

#include <stdio.h>
#include <string.h>

#include "glyph_batch.h"
#include "diagnostics/log.h"

static kj_text_presentation s_windows[KJ_TEXT_MAX_WINDOWS];
/* Least-recently-seen age per record, private to this file. A live window's
 * age is refreshed by every event or publication it receives; on capacity
 * exhaustion the smallest age is evicted. Never expose this through the
 * public presentation struct. */
static uint32_t s_age[KJ_TEXT_MAX_WINDOWS];
static uint32_t s_serial;
static kj_text_stats s_stats;
static bool s_enabled;
static bool s_atlas_ready;

static unsigned char s_logged[KJ_TEXT_FALLBACK_COUNT];
static bool s_logged_native;

void kj_text_init(void) {
    memset(s_windows, 0, sizeof(s_windows));
    memset(s_age, 0, sizeof(s_age));
    s_serial = 0;
    memset(&s_stats, 0, sizeof(s_stats));
    memset(s_logged, 0, sizeof(s_logged));
    s_enabled = false;
    s_atlas_ready = kj_font_atlas_ready();
}

void kj_text_exit(void) {
    memset(s_windows, 0, sizeof(s_windows));
    memset(s_age, 0, sizeof(s_age));
    s_serial = 0;
}

void kj_text_set_enabled(bool native) {
    s_enabled = native;
}

bool kj_text_enabled(void) {
    return s_enabled && s_atlas_ready;
}

void kj_text_stats_reset(void) {
    memset(&s_stats, 0, sizeof(s_stats));
}

const kj_text_stats *kj_text_stats_read(void) {
    return &s_stats;
}

void kj_text_note_draw(void) {
    ++s_stats.draws;
}

static void touch(int index) {
    s_age[index] = ++s_serial;
}

static kj_text_presentation *find_live(uint32_t window) {
    for (int i = 0; i < KJ_TEXT_MAX_WINDOWS; ++i) {
        if (s_windows[i].valid && s_windows[i].window == window) {
            touch(i);
            return &s_windows[i];
        }
    }
    return NULL;
}

static int least_recently_seen(void) {
    int best = -1;
    uint32_t oldest = UINT32_MAX;
    for (int i = 0; i < KJ_TEXT_MAX_WINDOWS; ++i) {
        if (s_windows[i].valid && s_age[i] < oldest) {
            oldest = s_age[i];
            best = i;
        }
    }
    return best;
}

static kj_text_presentation *claim(uint32_t window, uint32_t generation) {
    int slot = -1;

    for (int i = 0; i < KJ_TEXT_MAX_WINDOWS; ++i) {
        if (s_windows[i].valid && s_windows[i].window == window) {
            if (s_windows[i].generation == generation) {
                touch(i);
                return &s_windows[i];
            }
            /* A new allocation at the same address must not inherit. */
            slot = i;
            break;
        }
        if (!s_windows[i].valid && slot < 0) {
            slot = i;
        }
    }
    if (slot < 0) {
        /* The table is a fixed capacity: evict the least recently seen
         * record (never an arbitrary slot), and count the drop. */
        slot = least_recently_seen();
        ++s_stats.events_dropped;
        if (slot < 0) {
            slot = 0; /* no valid record: unreachable with a non-zero capacity */
        }
    }
    memset(&s_windows[slot], 0, sizeof(s_windows[slot]));
    s_windows[slot].valid = true;
    s_windows[slot].window = window;
    s_windows[slot].generation = generation;
    touch(slot);
    return &s_windows[slot];
}

static bool all_glyphs_resolvable(const kj_text_presentation *p, kj_text_fallback *why) {
    for (uint16_t i = 0; i < p->glyph_count; ++i) {
        const kj_text_glyph *g = &p->glyphs[i];
        kj_glyph rec;
        if (g->code == 0xFFFFu || g->code == 0xE000u || g->code == 0x0000u) {
            /* End of string, line feed, or the blank filler HGSS leaves in
             * windows that also carry artwork (the map-name banner). Those
             * windows cannot be backed by a fill, so keep them on DS pixels. */
            if (g->code == 0x0000u) {
                *why = KJ_TEXT_FALLBACK_UNSUPPORTED_GLYPH;
                return false;
            }
            continue;
        }
        if (!kj_font_lookup(g->font_id, g->code, KJ_GLYPH_VARIANT_STANDARD, &rec)) {
            static uint32_t logged_code;
            if (logged_code != ((uint32_t)g->font_id << 16 | g->code)) {
                logged_code = (uint32_t)g->font_id << 16 | g->code;
                kj_log("[text] no glyph: font %u code 0x%04X win %08lx", (unsigned)g->font_id,
                       (unsigned)g->code, (unsigned long)p->window);
            }
            *why = KJ_TEXT_FALLBACK_UNSUPPORTED_GLYPH;
            ++s_stats.unsupported_codes;
            return false;
        }
    }
    return true;
}

static void decide(kj_text_presentation *p) {
    kj_text_fallback why = KJ_TEXT_OK;

    p->native = false;
    p->policy = KJ_TEXT_LEGACY;
    if (!s_enabled) {
        why = KJ_TEXT_FALLBACK_DISABLED;
    } else if (!s_atlas_ready) {
        why = KJ_TEXT_FALLBACK_NO_ATLAS;
    } else if (p->temporary) {
        why = KJ_TEXT_FALLBACK_TEMPORARY_WINDOW;
    } else if (p->poisoned) {
        why =
            p->fallback != KJ_TEXT_OK ? (kj_text_fallback)p->fallback : KJ_TEXT_FALLBACK_NO_BACKING;
    } else if (p->bg_id >= 8) {
        why = KJ_TEXT_FALLBACK_UNSUPPORTED_LAYER;
    } else if (p->glyph_count == 0 || p->glyph_count > KJ_TEXT_MAX_GLYPHS_PER_WINDOW) {
        why = KJ_TEXT_FALLBACK_CAPACITY;
    } else if (p->alternate) {
        why = KJ_TEXT_FALLBACK_ALTERNATE_TABLE;
    } else if (!all_glyphs_resolvable(p, &why)) {
        /* why already set */
    }

    if (why == KJ_TEXT_OK) {
        p->native = true;
        p->policy = KJ_TEXT_NATIVE_BG;
        ++s_stats.windows_native;
        if (!s_logged_native) {
            s_logged_native = true;
            kj_log("[text] first native window: %u glyphs, bg %u tiles %ux%u",
                   (unsigned)p->glyph_count, p->bg_id, p->width, p->height);
        }
    } else {
        p->fallback = (uint8_t)why;
        ++s_stats.windows_fallback;
        if (why < KJ_TEXT_FALLBACK_COUNT) {
            ++s_stats.fallback_reason[why];
        }
        if (p->logged != (uint8_t)why) {
            p->logged = (uint8_t)why;
            kj_log("[text] fallback %d win %08lx gen %lu bg %u %ux%u at %u,%u glyphs %u", (int)why,
                   (unsigned long)p->window, (unsigned long)p->generation, (unsigned)p->bg_id,
                   (unsigned)p->width, (unsigned)p->height, (unsigned)p->tilemap_x,
                   (unsigned)p->tilemap_y, (unsigned)p->glyph_count);
        }
        kj_text_log_fallback(why, NULL);
    }
    p->fallback = (uint8_t)why;
}

void kj_text_frame(const kj_frame_input *in) {
    if (in == NULL) {
        return;
    }
    if (in->text_events == NULL || in->text_event_count == 0) {
        return;
    }
    s_stats.events_seen += in->text_event_count;
    if (s_stats.events_seen == in->text_event_count) {
        kj_log("[text] first capture: %u events", (unsigned)in->text_event_count);
    }

    for (uint32_t i = 0; i < in->text_event_count; ++i) {
        const kj_text_event *e = &in->text_events[i];
        kj_text_presentation *p;

        switch (e->kind) {
        case KJ_TEXT_EV_ADD:
            p = claim(e->window, e->generation);
            p->glyph_count = 0;
            p->published = false;
            p->native = false;
            p->policy = KJ_TEXT_LEGACY;
            p->have_fill = 0;
            p->alternate = 0;
            p->poisoned = 0;
            p->fallback = KJ_TEXT_OK;
            p->engine = e->engine;
            p->bg_id = e->bg_id;
            p->palette_num = e->palette_num;
            p->color_mode = e->color_mode;
            p->tilemap_x = e->tilemap_x;
            p->tilemap_y = e->tilemap_y;
            p->width = e->width;
            p->height = e->height;
            p->temporary = (e->flags & KJ_TEXT_WINDOW_TEMPORARY) != 0;
            break;
        case KJ_TEXT_EV_REMOVE:
            p = find_live(e->window);
            if (p != NULL) {
                /* The DS tilemap keeps whatever was last copied, so dropping
                 * the native record falls back to those pixels rather than to
                 * a blank rectangle. Never draw over a reused window slot. */
                memset(p, 0, sizeof(*p));
            }
            break;
        case KJ_TEXT_EV_GLYPH:
            p = claim(e->window, e->generation);
            if (p->temporary) {
                break;
            }
            if (p->glyph_count >= KJ_TEXT_MAX_GLYPHS_PER_WINDOW) {
                p->poisoned = 1;
                p->fallback = KJ_TEXT_FALLBACK_CAPACITY;
                p->native = false;
                p->policy = KJ_TEXT_LEGACY;
                ++s_stats.allocation_failures;
                break;
            }
            if (e->x < 0 || e->y < 0) {
                /* A control-code reposition outside the window: keep legacy. */
                p->poisoned = 1;
                p->fallback = KJ_TEXT_FALLBACK_NO_BACKING;
                p->native = false;
                break;
            }
            {
                kj_text_glyph *g = &p->glyphs[p->glyph_count++];
                g->code = e->code;
                g->x = e->x;
                g->y = e->y;
                g->font_id = e->font_id;
                g->fg = e->fg;
                g->shadow = e->shadow;
                g->bg = e->bg;
            }
            if (e->glyph_table != 0) {
                p->alternate = 1;
            }
            ++s_stats.glyphs_captured;
            break;
        case KJ_TEXT_EV_ART:
            p = find_live(e->window);
            if (p != NULL) {
                /* Art was blitted into the window buffer; its background is
                 * no longer a solid fill, so keep the DS pixels. */
                p->poisoned = 1;
                p->native = false;
                p->policy = KJ_TEXT_LEGACY;
                p->fallback = KJ_TEXT_FALLBACK_NO_BACKING;
            }
            break;
        case KJ_TEXT_EV_FILL:
            p = claim(e->window, e->generation);
            p->fill = (uint8_t)e->value;
            p->have_fill = 1;
            if (e->x == 0 && e->y == 0 && e->arg0 >= (uint16_t)(p->width * 8) &&
                e->arg1 >= (uint16_t)(p->height * 8)) {
                /* Full clear: the clean backing is a solid fill. */
                p->glyph_count = 0;
                p->published = false;
                p->poisoned = 0;
                p->alternate = 0;
            } else {
                /* A partial erase clears that rectangle only. Drop the glyphs
                 * it covers and keep the rest native; the window's backing
                 * fill still covers the cleared area. */
                const int x0 = e->x, y0 = e->y;
                const int x1 = e->x + (int)e->arg0, y1 = e->y + (int)e->arg1;
                uint16_t keep = 0;
                for (uint16_t gi = 0; gi < p->glyph_count; ++gi) {
                    const kj_text_glyph *g = &p->glyphs[gi];
                    const bool hit = g->x < x1 && g->x + 8 > x0 && g->y < y1 && g->y + 16 > y0;
                    if (!hit) {
                        p->glyphs[keep++] = *g;
                    }
                }
                p->glyph_count = keep;
            }
            break;
        case KJ_TEXT_EV_SCROLL:
            p = find_live(e->window);
            if (p != NULL) {
                /* The DS shifts the window's pixels; shift the records with
                 * them and drop whatever scrolled out. This is what lets the
                 * second page of a dialogue stay native. */
                uint16_t keep = 0;
                const int dw = (int)p->width * 8, dh = (int)p->height * 8;
                for (uint16_t gi = 0; gi < p->glyph_count; ++gi) {
                    kj_text_glyph g = p->glyphs[gi];
                    g.x = (int16_t)(g.x + e->x);
                    g.y = (int16_t)(g.y + e->y);
                    if (g.x + 8 > 0 && g.x < dw && g.y + 16 > 0 && g.y < dh) {
                        p->glyphs[keep++] = g;
                    }
                }
                p->glyph_count = keep;
                ++s_stats.scrolls;
            }
            break;
        case KJ_TEXT_EV_PUBLISH:
            p = find_live(e->window);
            if (p == NULL) {
                p = claim(e->window, e->generation);
            }
            p->engine = e->engine;
            p->bg_id = e->bg_id;
            p->palette_num = e->palette_num;
            p->color_mode = e->color_mode;
            p->tilemap_x = e->tilemap_x;
            p->tilemap_y = e->tilemap_y;
            p->width = e->width;
            p->height = e->height;
            p->published = true;
            decide(p);
            if (p->native) {
                s_stats.glyphs_native += p->glyph_count;
            }
            break;
        default:
            ++s_stats.events_dropped;
            break;
        }
    }
}

const kj_text_presentation *kj_text_next(int engine, int bg, int *cursor) {
    if (cursor == NULL) {
        return NULL;
    }
    for (int i = *cursor; i < KJ_TEXT_MAX_WINDOWS; ++i) {
        const kj_text_presentation *p = &s_windows[i];
        const int plane = engine * 4 + bg;
        if (p->valid && p->published && p->native && p->engine == engine && p->bg_id == plane) {
            *cursor = i + 1;
            return p;
        }
    }
    *cursor = KJ_TEXT_MAX_WINDOWS;
    return NULL;
}

/* A tile inside a window that this frame presents natively is replaced by the
 * native pass, so the legacy glyph pixels must not be drawn underneath it.
 * Coordinates are DS screen pixels (the same space text_render maps). */
bool kj_text_owns_tile(int engine, int bg, int hofs, int vofs, int x, int y) {
    const int plane = engine * 4 + bg;
    for (int i = 0; i < KJ_TEXT_MAX_WINDOWS; ++i) {
        const kj_text_presentation *p = &s_windows[i];
        if (!p->valid || !p->published || !p->native || p->engine != engine || p->bg_id != plane) {
            continue;
        }
        const int wx = (int)p->tilemap_x * 8 - hofs;
        const int wy = (int)p->tilemap_y * 8 - vofs;
        if (x >= wx && x < wx + (int)p->width * 8 && y >= wy && y < wy + (int)p->height * 8) {
            return true;
        }
    }
    return false;
}

uint32_t kj_text_ds_color(const uint16_t *palette, uint8_t color_mode, uint8_t palette_num,
                          uint8_t index) {
    uint16_t c;
    uint32_t r, g, b;

    /* Index 0 is transparent on every DS BG layer; drawing it opaque is what
     * produced black bars behind menu labels whose window has no fill. */
    if (index == 0) {
        return 0;
    }
    if (palette == NULL) {
        return 0xFFFFFFFFu;
    }
    c = color_mode ? palette[index] : palette[(palette_num * 16u + index) & 0xFFu];
    r = c & 31u;
    g = (c >> 5) & 31u;
    b = (c >> 10) & 31u;
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return (r << 24) | (g << 16) | (b << 8) | 0xFFu;
}

void kj_text_log_fallback(kj_text_fallback reason, const kj_text_event *event) {
    char line[120];

    if (reason >= KJ_TEXT_FALLBACK_COUNT || s_logged[reason]) {
        return;
    }
    s_logged[reason] = 1;
    snprintf(line, sizeof(line), "[text] native fallback %d (font %u code 0x%04X bg %u)",
             (int)reason, event != NULL ? event->font_id : 0u, event != NULL ? event->code : 0u,
             event != NULL ? event->bg_id : 0u);
    kj_log("%s", line);
}
