#ifndef KJ_GRAPHICS_G2D_WIDEART_H
#define KJ_GRAPHICS_G2D_WIDEART_H

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * The wide art: the hand-drawn bands for the wider screen.
 * (From Sinjoh at 418dcf4, GPL-3.0; kohjoh issue #14.)
 *
 * The remaster's upper screen is 32 DS pixels wider on each side than the
 * DS's picture, and the game draws nothing there. The port fills the bands
 * itself; where a picture was drawn by hand for a layer, the picture's bands
 * are used instead.
 *
 * A picture belongs to one layer of the game and is found by that layer's
 * art key: a hash of the layer's own art (a BG's map, the tiles the map
 * uses, their palettes), not of where the game put it or how it is
 * scrolled. The hash is the same one the pictures are named with
 * (pokeplatinum-3ds's, so the pictures in wideart/ are found), and the
 * pictures are cut, converted and laid out at package time
 * (tools/wideart/pack_wideart.py): the console uploads them and draws them.
 *
 * The picture's bands are 32 x 192 DS pixels each (the sides of the wide
 * canvas), in one texture, in the PICA's own tile order.
 */

#define KJ_WIDE_ART_FULL 1u /* the whole layer, not only its bands */
#define KJ_WIDE_ART_LOOP 2u /* the bands go round with the layer */

typedef struct kj_wideart_pic {
    uint64_t key;
    int16_t hofs, vofs;
    uint16_t flags;
    uint16_t left_x, left_y; /* the left band's corner in the texture */
    uint16_t right_x, right_y;
    uint16_t band_w, band_h;
} kj_wideart_pic;

bool kj_wideart_init(void);
void kj_wideart_exit(void);

/* The picture for a layer's art key, or NULL. */
const kj_wideart_pic *kj_wideart_find(uint64_t key);

/* The picture for a layer's art key nearest the layer's scroll: a layer can
 * have several pictures (one map can hold two scenes), and the one made
 * nearest where the layer is now is the one of what is on screen.
 *   cur_h, cur_v: the layer's scroll now
 *   period_h, period_v: the map's size (the scroll goes round it)
 *   max_h, max_v: how far away a picture may be (0: any)
 *   dx_out: how far the layer has scrolled since the picture was made */
const kj_wideart_pic *kj_wideart_find_near(uint64_t key, int cur_h, int cur_v, int period_h,
                                            int period_v, int max_h, int max_v, int *dx_out);

C3D_Tex *kj_wideart_texture(void);
uint32_t kj_wideart_texture_width(void);
uint32_t kj_wideart_texture_height(void);
uint32_t kj_wideart_count(void);

#endif
