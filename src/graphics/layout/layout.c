/*
 * layout.c: wide-layout rows -> scissored segments (issue #16).
 *
 * Semantics follow Unown's buildRow() (soulsilver3ds/platform/widecomp.c):
 * per BG and line, wide column s (-32..287) shows picture column r[s];
 * columns 0..255 start as the DS shows them, the sides start empty, MOVE
 * vacates its source columns, then rows apply in order (later rows win).
 * Instead of compositing pixels on the CPU, the result is cut into runs of
 * constant offset or constant column, which g2d.c draws as the layer's own
 * retained quads translated (or stretched) under a scissor: "compact runs
 * or scissored strips with per-run state" (coffeepot private_please_gitignore/
 * HGSS_3DS_ENGINEERING_GUIDE.md, DS-to-PICA mapping table).
 */

#include "graphics/layout/layout.h"

#define COLS (KJ_LAYOUT_RIGHT - KJ_LAYOUT_LEFT)
#define NONE INT16_MIN

static bool bg_row(const kj_layout_row *t)
{
    return t->kind == KJ_LAYOUT_MOVE || t->kind == KJ_LAYOUT_FILL || t->kind == KJ_LAYOUT_COPY ||
           t->kind == KJ_LAYOUT_WRAP;
}

/* Unown's h0: the scroll as a small signed picture offset. */
static int h0_of(const kj_layout_bg *bg)
{
    int h = bg->hofs & (bg->w - 1);
    if (h > bg->w - 64) {
        h -= bg->w;
    }
    return h;
}

uint32_t kj_layout_active(const kj_layout_row *rows, uint32_t n, const kj_layout_bg *bg, int y)
{
    const int py = (y + bg->vofs) & (bg->h - 1);
    uint32_t mask = 0;
    if (n > KJ_LAYOUT_MAX_ROWS) {
        n = KJ_LAYOUT_MAX_ROWS;
    }
    for (uint32_t k = 0; k < n; ++k) {
        const kj_layout_row *t = &rows[k];
        if (!bg_row(t) || !(t->bgs & (1 << bg->bg))) {
            continue;
        }
        const int ln = (t->bgs & KJ_LAYOUT_SCREEN) ? y : py;
        if (ln >= t->top && ln < t->bottom) {
            mask |= 1u << k;
        }
    }
    return mask;
}

/* Picture column -> the source screen column nearest wide column s (the
 * map repeats every w px), in [SRC_LO, SRC_HI), or NONE. */
static int16_t source_col(int pic, int h0, int w, int s)
{
    int d = (pic - h0 - s) & (w - 1);
    if (d > w / 2) {
        d -= w;
    }
    int sx = s + d;
    if (sx >= KJ_LAYOUT_SRC_HI) {
        sx -= w;
    } else if (sx < KJ_LAYOUT_SRC_LO) {
        sx += w;
    }
    return sx >= KJ_LAYOUT_SRC_LO && sx < KJ_LAYOUT_SRC_HI ? (int16_t)sx : NONE;
}

int kj_layout_segments(const kj_layout_row *rows, uint32_t n, uint32_t active,
                       const kj_layout_bg *bg, kj_layout_seg *out, bool *unsupported)
{
    int16_t r[COLS];
    const int h0 = h0_of(bg), m = bg->w - 1;
    bool missing = false;

    *unsupported = false;
    if (n > KJ_LAYOUT_MAX_ROWS) {
        n = KJ_LAYOUT_MAX_ROWS;
    }
    /* r[] holds picture columns first. */
    for (int s = KJ_LAYOUT_LEFT; s < KJ_LAYOUT_RIGHT; ++s) {
        r[s - KJ_LAYOUT_LEFT] = (s >= 0 && s < 256) ? (int16_t)((s + h0) & m) : NONE;
    }
    for (uint32_t k = 0; k < n; ++k) {
        const kj_layout_row *t = &rows[k];
        if (!(active & (1u << k)) || t->kind != KJ_LAYOUT_MOVE) {
            continue;
        }
        const bool scr = (t->bgs & KJ_LAYOUT_SCREEN) != 0;
        const int a0 = scr ? t->x0 : t->x0 - h0, a1 = scr ? t->x1 : t->x1 - h0;
        for (int s = a0 < 0 ? 0 : a0; s < a1 && s < 256; ++s) {
            r[s - KJ_LAYOUT_LEFT] = NONE;
        }
    }
    for (uint32_t k = 0; k < n; ++k) {
        const kj_layout_row *t = &rows[k];
        if (!(active & (1u << k))) {
            continue;
        }
        const bool scr = (t->bgs & KJ_LAYOUT_SCREEN) != 0;
        switch (t->kind) {
        case KJ_LAYOUT_MOVE:
            for (int c = t->x0; c < t->x1; ++c) {
                const int s = (scr ? c : c - h0) + t->dx;
                if (s >= KJ_LAYOUT_LEFT && s < KJ_LAYOUT_RIGHT) {
                    r[s - KJ_LAYOUT_LEFT] = (int16_t)((scr ? c + h0 : c) & m);
                }
            }
            break;
        case KJ_LAYOUT_COPY:
        case KJ_LAYOUT_FILL:
            for (int c = t->x0; c < t->x1; ++c) {
                const int s = scr ? c : c - h0;
                if (s >= KJ_LAYOUT_LEFT && s < KJ_LAYOUT_RIGHT) {
                    int pic = t->kind == KJ_LAYOUT_COPY ? t->arg + (c - t->x0) : t->arg;
                    if (scr) {
                        pic += h0; /* arg is a screen column */
                    }
                    r[s - KJ_LAYOUT_LEFT] = (int16_t)(pic & m);
                }
            }
            break;
        case KJ_LAYOUT_WRAP:
            for (int s = KJ_LAYOUT_LEFT; s < KJ_LAYOUT_RIGHT; ++s) {
                if (s < 0 || s >= 256) {
                    r[s - KJ_LAYOUT_LEFT] = (int16_t)((s + h0) & m);
                }
            }
            break;
        default:
            break;
        }
    }
    /* Picture columns -> source screen columns. */
    for (int i = 0; i < COLS; ++i) {
        if (r[i] != NONE) {
            r[i] = source_col(r[i], h0, bg->w, i + KJ_LAYOUT_LEFT);
            missing |= r[i] == NONE;
        }
    }
    /* Runs of constant offset, or of one repeated column. */
    int count = 0;
    for (int i = 0; i < COLS;) {
        if (r[i] == NONE) {
            ++i;
            continue;
        }
        int j = i + 1;
        const bool stretch = j < COLS && r[j] == r[i];
        if (stretch) {
            while (j < COLS && r[j] == r[i]) {
                ++j;
            }
        } else {
            while (j < COLS && r[j] != NONE && r[j] == r[j - 1] + 1) {
                ++j;
            }
        }
        if (count == KJ_LAYOUT_MAX_SEGS) {
            *unsupported = true;
            break;
        }
        out[count++] = (kj_layout_seg){ (int16_t)(i + KJ_LAYOUT_LEFT), (int16_t)(j + KJ_LAYOUT_LEFT),
                                        r[i], stretch ? 1 : 0 };
        i = j;
    }
    *unsupported |= missing;
    return count;
}

int kj_layout_obj_dx(const kj_layout_row *rows, uint32_t n, int cx, int cy)
{
    for (uint32_t k = 0; k < n; ++k) {
        const kj_layout_row *t = &rows[k];
        if (t->kind == KJ_LAYOUT_OBJS && cx >= t->x0 && cx < t->x1 && cy >= t->top &&
            cy < t->bottom) {
            return t->dx;
        }
    }
    return 0;
}

int kj_layout_shift3d(const kj_layout_row *rows, uint32_t n)
{
    int dx = 0;
    for (uint32_t k = 0; k < n; ++k) {
        if (rows[k].kind == KJ_LAYOUT_SHIFT3D) {
            dx = rows[k].dx;
        }
    }
    return dx;
}
