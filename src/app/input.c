/*
 * input.c: 3DS buttons and touch -> DS pad bits and DS touch coordinates.
 *
 * The bottom screen shows the DS's 256x192 lower screen at 1.25x on the
 * 320x240 panel, so a touch maps back by 0.8 (exact: 320 * 0.8 = 256).
 */

#include "app/input.h"

#include "runtime/kj_bridge.h"

#include <3ds.h>

/* NitroSDK PAD bits (nitro/spi/ARM9/pad.h). */
enum {
    DS_A = 0x0001,
    DS_B = 0x0002,
    DS_SELECT = 0x0004,
    DS_START = 0x0008,
    DS_RIGHT = 0x0010,
    DS_LEFT = 0x0020,
    DS_UP = 0x0040,
    DS_DOWN = 0x0080,
    DS_R = 0x0100,
    DS_L = 0x0200,
    DS_X = 0x0400,
    DS_Y = 0x0800,
};

static kj_input s_input;

void kj_input_init(void)
{
    s_input.keys = 0;
    s_input.touch = false;
}

void kj_input_sample(void)
{
    hidScanInput();
    const u32 held = hidKeysHeld();
    uint16_t keys = 0;

    keys |= (held & KEY_A) ? DS_A : 0;
    keys |= (held & KEY_B) ? DS_B : 0;
    keys |= (held & KEY_SELECT) ? DS_SELECT : 0;
    keys |= (held & KEY_START) ? DS_START : 0;
    keys |= (held & (KEY_DRIGHT | KEY_CPAD_RIGHT)) ? DS_RIGHT : 0;
    keys |= (held & (KEY_DLEFT | KEY_CPAD_LEFT)) ? DS_LEFT : 0;
    keys |= (held & (KEY_DUP | KEY_CPAD_UP)) ? DS_UP : 0;
    keys |= (held & (KEY_DDOWN | KEY_CPAD_DOWN)) ? DS_DOWN : 0;
    keys |= (held & KEY_R) ? DS_R : 0;
    keys |= (held & KEY_L) ? DS_L : 0;
    keys |= (held & KEY_X) ? DS_X : 0;
    keys |= (held & KEY_Y) ? DS_Y : 0;
    s_input.keys = keys;

    if (held & KEY_TOUCH) {
        touchPosition t;
        hidTouchRead(&t);
        unsigned x = (unsigned)t.px * 4u / 5u;
        unsigned y = (unsigned)t.py * 4u / 5u;
        s_input.touch = true;
        s_input.touch_x = (uint16_t)(x > 255u ? 255u : x);
        s_input.touch_y = (uint16_t)(y > 191u ? 191u : y);
    } else {
        s_input.touch = false;
    }
}

const kj_input *kj_ctr_input(void)
{
    return &s_input;
}
