/*
 * kj_frame.c: the scheduling bridge between the game's DS frame and the
 * 3DS presentation (DS side).
 *
 * One emulated DS V-blank is one physical 3DS V-blank: the game's OS_Halt
 * (NitroPort_Halt -> NitroPort_OnFrame) waits for the 3DS display. HGSS's
 * main loop waits twice per game frame, so the game naturally runs at
 * ~29.9 game frames a second and nothing here doubles or halves it.
 *
 * The frame is published once per game frame, at the point patch 2001
 * marks in NitroMain (after the V-blank callback, the sound update and the
 * vwait task queue): the game is stopped at its frame boundary, so the
 * renderer reads one coherent version of registers, palettes, OAM and VRAM
 * on this thread, and nothing races it. Loops that wait for V-blank without
 * reaching that point (soft-reset and error screens, the few boot frames)
 * are published from the V-blank itself after a short grace period.
 */

#include "kj_bridge.h"
#include "kj_g3.h"

#include <nitro.h>
#include <nitro_port.h>
#include <string.h>

extern unsigned char NitroPort_IoReg[];
extern unsigned char NitroPort_Pltt[];
extern unsigned char NitroPort_Oam[];
extern unsigned char NitroPort_BgVram[];
extern unsigned char NitroPort_DbBgVram[];
extern unsigned char NitroPort_ObjVram[];
extern unsigned char NitroPort_DbObjVram[];
extern unsigned char NitroPort_LcdcVram[];

void NitroPort_SoundPumpReplies(void);
extern u64 svcGetSystemTick(void);

static u64 s_pump_ticks, s_hblank_ticks;
static u32 s_pump_calls, s_hblank_walks;
static u64 s_sync_calls_base, s_sync_ticks_base;

/* V-blanks without a publish after which the V-blank publishes itself. Two
 * is the main loop's normal cadence; four leaves room for a frame that
 * overran once. */
#define PUBLISH_GRACE_VBLANKS 4u

static kj_ds_memory s_memory;
static u32 s_vblanks;
static u32 s_publishes;
static u32 s_last_publish_vblank;

static const u32 BANK_LCDC_OFFSET[9] = {
    0x00000, 0x20000, 0x40000, 0x60000, 0x80000, 0x90000, 0x94000, 0x98000, 0xA0000,
};
static const u32 BANK_SIZE[9] = {
    0x20000, 0x20000, 0x20000, 0x20000, 0x10000, 0x4000, 0x4000, 0x8000, 0x4000,
};
/* VRAMCNT_A..G are 0x240..0x246, H and I 0x248/0x249 (0x247 is WRAMCNT). */
static const u16 BANK_REG[9] = {
    0x240, 0x241, 0x242, 0x243, 0x244, 0x245, 0x246, 0x248, 0x249,
};

const kj_ds_memory *kj_ds_get_memory(void)
{
    if (s_memory.io == NULL) {
        s_memory.io = NitroPort_IoReg;
        s_memory.io_size = HW_IOREG_SIZE;
        s_memory.pltt = NitroPort_Pltt;
        s_memory.oam = NitroPort_Oam;
        s_memory.bg_vram_a = NitroPort_BgVram;
        s_memory.bg_vram_b = NitroPort_DbBgVram;
        s_memory.obj_vram_a = NitroPort_ObjVram;
        s_memory.obj_vram_b = NitroPort_DbObjVram;
        s_memory.lcdc_vram = NitroPort_LcdcVram;
    }
    return &s_memory;
}

static void decode_banks(kj_vram_bank banks[9])
{
    for (int i = 0; i < 9; ++i) {
        const u8 cnt = NitroPort_IoReg[BANK_REG[i]];

        banks[i].lcdc_offset = BANK_LCDC_OFFSET[i];
        banks[i].size = BANK_SIZE[i];
        banks[i].enabled = (cnt & 0x80) != 0;
        banks[i].mst = cnt & 0x07;
        banks[i].ofs = (cnt >> 3) & 0x03;
    }
}

static void publish(void)
{
    kj_frame_input in;

    /* The per-line registers of this frame: H-blank handlers and DMAs run
     * once per V-blank, after the game's V-blank work armed them. */
    {
#ifndef HGSS_PROFILE_OFF
        const u64 t0 = svcGetSystemTick();
#endif
        if (NitroPort_HBlankLinesOnce()) {
#ifndef HGSS_PROFILE_OFF
            ++s_hblank_walks;
#endif
        }
#ifndef HGSS_PROFILE_OFF
        s_hblank_ticks += svcGetSystemTick() - t0;
#endif
    }

    memset(&in, 0, sizeof(in));
    in.mem = kj_ds_get_memory();
    decode_banks(in.banks);
    in.lines.active = NitroPort_LineRegs.active != 0;
    in.lines.regs = in.lines.active
                        ? (const uint8_t (*)[192][KJ_LINE_REG_BYTES])NitroPort_LineRegs.regs
                        : NULL;
    in.g3 = kj_g3_take_frame();
    in.vblank_count = s_vblanks;
    in.game_frame = ++s_publishes;
    in.engine_a_on_top = NitroPort_EngineAOnTop() != FALSE;
    s_last_publish_vblank = s_vblanks;
    kj_ctr_publish_frame(&in);
}

void NitroPort_VBlankWorkDone(void)
{
    publish();
}

void NitroPort_OnFrame(void)
{
    const kj_input *input;

    ++s_vblanks;
    {
#ifndef HGSS_PROFILE_OFF
        const u64 t0 = svcGetSystemTick();
#endif
        NitroPort_SoundPumpReplies();
#ifndef HGSS_PROFILE_OFF
        s_pump_ticks += svcGetSystemTick() - t0;
        ++s_pump_calls;
#endif
    }

    if (!kj_ctr_vblank()) {
        NitroPort_Log("[kj] close requested at V-blank %lu", (unsigned long)s_vblanks);
        NitroPort_CardFlush();
        kj_ctr_terminate();
    }

    input = kj_ctr_input();
    NitroPort_SetButtons(input->keys, FALSE);
    NitroPort_SetTouch(input->touch ? TRUE : FALSE, input->touch_x, input->touch_y);

    if (s_vblanks - s_last_publish_vblank > PUBLISH_GRACE_VBLANKS) {
        publish();
    }
}

/* POWCNT1 (0x04000304) bit 15: engine A drives the upper screen. */
BOOL NitroPort_EngineAOnTop(void)
{
    return (NitroPort_IoReg[0x305] & 0x80) ? TRUE : FALSE;
}

void NitroPort_Terminate(void)
{
    NitroPort_Log("[kj] the game terminated itself (OS_Terminate)");
    NitroPort_CardFlush();
    kj_ctr_terminate();
}

void NitroPort_ResetSystem(u32 parameter)
{
    /* A DS soft reset restarts the program from its entry. Static state of
     * the original game cannot be re-initialised in place, so the program
     * closes; relaunch is the user's (tracked: docs/worklog). */
    NitroPort_Log("[kj] soft reset requested (parameter %lu); closing", (unsigned long)parameter);
    NitroPort_CardFlush();
    kj_ctr_terminate();
}

void kj_g3_log_op_profile(void);

void kj_ds_take_profile(kj_ds_profile *out)
{
    kj_g3_log_op_profile();
    u64 calls, ticks;

    memset(out, 0, sizeof(*out));
    kj_g3_take_profile(&out->g3_fifo_ticks, &out->g3_fifo_words, &out->g3_direct_words,
                       &out->g3_commands);
    NitroPort_SyncTimersStats(&calls, &ticks);
    out->timer_sync_calls = calls - s_sync_calls_base;
    out->timer_sync_ticks = ticks - s_sync_ticks_base;
    s_sync_calls_base = calls;
    s_sync_ticks_base = ticks;
    out->sound_pump_ticks = s_pump_ticks;
    out->sound_pump_calls = s_pump_calls;
    out->hblank_ticks = s_hblank_ticks;
    out->hblank_walks = s_hblank_walks;
    kj_g3_dl_stats(&out->dl_hits, &out->dl_misses, &out->dl_uncacheable, &out->dl_bytes);
    s_pump_ticks = s_hblank_ticks = 0;
    s_pump_calls = s_hblank_walks = 0;
}
