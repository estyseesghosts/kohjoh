#include "kj_text.h"

#include <nitro.h>
#include <string.h>

#include "bg_window.h"

/* Bounded capture. A dialogue page is well under 200 glyphs; menus that
 * rebuild many windows can exceed this, and then the frame falls back rather
 * than showing half a text object. */
static kj_text_event s_events[KJ_TEXT_MAX_EVENTS];
static unsigned int s_event_count;
static BOOL s_overflow;

typedef struct {
    struct Window *window;
    unsigned int generation;
    BOOL live;
} window_identity;

static window_identity s_windows[KJ_TEXT_MAX_TEXT_WINDOWS];

static window_identity *identity_for(struct Window *window, BOOL create)
{
    unsigned int i;
    window_identity *free_slot = NULL;

    for (i = 0; i < KJ_TEXT_MAX_TEXT_WINDOWS; ++i) {
        if (s_windows[i].live && s_windows[i].window == window) {
            return &s_windows[i];
        }
        if (!s_windows[i].live && free_slot == NULL) {
            free_slot = &s_windows[i];
        }
    }
    if (!create) {
        return NULL;
    }
    if (free_slot == NULL) {
        s_overflow = TRUE;
        return NULL;
    }
    /* A new allocation at a reused address must not inherit old glyphs. */
    free_slot->window = window;
    free_slot->generation++;
    free_slot->live = TRUE;
    return free_slot;
}

static kj_text_event *next_event(unsigned int kind)
{
    kj_text_event *e;

    if (s_event_count >= KJ_TEXT_MAX_EVENTS) {
        s_overflow = TRUE;
        return NULL;
    }
    e = &s_events[s_event_count++];
    memset(e, 0, sizeof(*e));
    e->kind = (unsigned char)kind;
    return e;
}

static void fill_geometry(kj_text_event *e, struct Window *window, const window_identity *id)
{
    e->window = (uint32_t)(uintptr_t)window;
    e->generation = id != NULL ? id->generation : 0;
    e->bg_id = GetWindowBgId(window);
    e->engine = (unsigned char)(e->bg_id >= 4 ? 1 : 0);
    e->palette_num = window->paletteNum;
    e->color_mode = (unsigned char)(window->colorMode ? 1 : 0);
    e->tilemap_x = GetWindowX(window);
    e->tilemap_y = GetWindowY(window);
    e->width = GetWindowWidth(window);
    e->height = GetWindowHeight(window);
}

void NitroPort_TextWindowAdd(struct Window *window)
{
    window_identity *id = identity_for(window, TRUE);
    kj_text_event *e;

    if (id == NULL) {
        return;
    }
    e = next_event(KJ_TEXT_EV_ADD);
    if (e != NULL) {
        fill_geometry(e, window, id);
    }
}

void NitroPort_TextWindowAddTemporary(struct Window *window)
{
    window_identity *id = identity_for(window, TRUE);
    kj_text_event *e;

    if (id == NULL) {
        return;
    }
    e = next_event(KJ_TEXT_EV_ADD);
    if (e != NULL) {
        fill_geometry(e, window, id);
        e->flags = KJ_TEXT_WINDOW_TEMPORARY;
    }
}

void NitroPort_TextWindowFillRect(struct Window *window, unsigned int fill, int x, int y,
                                  int width, int height)
{
    window_identity *id = identity_for(window, TRUE);
    kj_text_event *e;

    if (id == NULL) {
        return;
    }
    e = next_event(KJ_TEXT_EV_FILL);
    if (e != NULL) {
        fill_geometry(e, window, id);
        e->value = (uint16_t)fill;
        e->x = (int16_t)x;
        e->y = (int16_t)y;
        e->arg0 = (uint16_t)width;
        e->arg1 = (uint16_t)height;
    }
}

void NitroPort_TextWindowBlit(struct Window *window)
{
    window_identity *id = identity_for(window, FALSE);
    kj_text_event *e;

    if (id == NULL) {
        return;
    }
    e = next_event(KJ_TEXT_EV_ART);
    if (e != NULL) {
        fill_geometry(e, window, id);
    }
}

void NitroPort_TextWindowRemove(struct Window *window)
{
    window_identity *id = identity_for(window, FALSE);
    kj_text_event *e;

    if (id == NULL) {
        return;
    }
    e = next_event(KJ_TEXT_EV_REMOVE);
    if (e != NULL) {
        fill_geometry(e, window, id);
    }
    id->live = FALSE;
    id->window = NULL;
}

void NitroPort_TextWindowPublish(struct Window *window)
{
    window_identity *id = identity_for(window, TRUE);
    kj_text_event *e;

    if (id == NULL) {
        return;
    }
    e = next_event(KJ_TEXT_EV_PUBLISH);
    if (e != NULL) {
        fill_geometry(e, window, id);
    }
}

void NitroPort_TextWindowFill(struct Window *window, unsigned int fill)
{
    window_identity *id = identity_for(window, TRUE);
    kj_text_event *e;

    if (id == NULL) {
        return;
    }
    e = next_event(KJ_TEXT_EV_FILL);
    if (e != NULL) {
        fill_geometry(e, window, id);
        e->value = (uint16_t)fill;
    }
}

void NitroPort_TextWindowScroll(struct Window *window, int dx, int dy, unsigned int fill)
{
    window_identity *id = identity_for(window, TRUE);
    kj_text_event *e;

    if (id == NULL) {
        return;
    }
    e = next_event(KJ_TEXT_EV_SCROLL);
    if (e != NULL) {
        fill_geometry(e, window, id);
        e->x = (int16_t)dx;
        e->y = (int16_t)dy;
        e->value = (uint16_t)fill;
    }
}

void NitroPort_TextGlyph(struct Window *window, unsigned int code, unsigned int fontId,
                         int x, int y, unsigned int fg, unsigned int shadow, unsigned int bg,
                         unsigned int glyphTable)
{
    window_identity *id = identity_for(window, TRUE);
    kj_text_event *e;

    if (id == NULL) {
        return;
    }
    e = next_event(KJ_TEXT_EV_GLYPH);
    if (e != NULL) {
        fill_geometry(e, window, id);
        e->code = (uint16_t)code;
        e->font_id = (unsigned char)fontId;
        e->x = (int16_t)x;
        e->y = (int16_t)y;
        e->fg = (unsigned char)fg;
        e->shadow = (unsigned char)shadow;
        e->bg = (unsigned char)bg;
        e->glyph_table = (unsigned char)(glyphTable == 0xFFFC ? 1 : 0);
    }
}

const kj_text_event *NitroPort_TextEvents(uint32_t *count)
{
    const kj_text_event *out;

    if (s_overflow) {
        /* Never hand over a truncated text object. */
        s_event_count = 0;
        s_overflow = FALSE;
        *count = 0;
        return NULL;
    }
    *count = s_event_count;
    out = s_event_count != 0 ? s_events : NULL;
    s_event_count = 0;
    return out;
}
