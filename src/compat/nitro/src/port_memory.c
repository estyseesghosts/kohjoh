/*
 * The DS's fixed memory areas, as ordinary memory.
 *
 * With SDK_PORT_3DS, the SDK's HW_MAIN_MEM, HW_ITCM, HW_WRAM, palette, OAM and
 * VRAM addresses point here instead of at DS hardware addresses (see
 * patches/NitroSDK, mmap_global.h). Sizes match the DS.
 *
 * About 14 MB in total. The New 3DS has plenty; the PC test build doesn't care.
 */

#include <nitro/types.h>
#include <nitro_port.h>
#include <string.h>

#define ALIGNED __attribute__((aligned(32)))

/* 8 MB: the SDK's shared/system area lives at offset 0x7FF000-0x7FFFFF.
 * On a 64-bit PC the system-work structure at its very end (OSSystemWork)
 * holds 8-byte pointers and runs a few bytes past it, so leave room there. */
#if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ != 4
unsigned char NitroPort_MainMem[0x800000 + 0x100] ALIGNED;
#else
unsigned char NitroPort_MainMem[0x800000] ALIGNED;
#endif

unsigned char NitroPort_Itcm[0x8000] ALIGNED;
unsigned char NitroPort_Wram[0x8000] ALIGNED;

/* The linker symbol the SDK uses for the start of DTCM (16 KB). */
u32 SDK_AUTOLOAD_DTCM_START[0x4000 / sizeof(u32)] ALIGNED;

/* Palettes: BG A, OBJ A, BG B, OBJ B, 0x200 bytes each. */
unsigned char NitroPort_Pltt[0x800] ALIGNED;

/* OAM: engine A then engine B, 0x400 bytes each. */
unsigned char NitroPort_Oam[0x800] ALIGNED;

static int8_t s_oam_pending[2][128];
static int8_t s_oam_active[256];
static u8 s_oam_attrs[2][128][6];
static int s_oam_draw_dx;

void NitroPort_WideObjScope(int dx)
{
    s_oam_draw_dx = dx;
}

void NitroPort_WideObjEntry(unsigned engine, unsigned slot)
{
    if (engine < 2 && slot < 128) {
        s_oam_pending[engine][slot] = (int8_t)s_oam_draw_dx;
    }
}

void NitroPort_WideObjCommit(unsigned engine, unsigned first, unsigned last)
{
    if (engine >= 2 || first > last || last >= 128) {
        return;
    }
    for (unsigned i = first; i <= last; ++i) {
        s_oam_active[engine * 128 + i] = s_oam_pending[engine][i];
        s_oam_pending[engine][i] = 0;
        memcpy(s_oam_attrs[engine][i], NitroPort_Oam + engine * 0x400 + i * 8, 6);
    }
}

void NitroPort_WideObjReset(unsigned engine, unsigned first, unsigned last)
{
    if (engine < 2 && first <= last && last < 128) {
        memset(&s_oam_pending[engine][first], 0, last - first + 1);
    }
}

void NitroPort_WideObjInvalidate(const void *dest, unsigned size)
{
    uintptr_t begin = (uintptr_t)dest;
    uintptr_t base = (uintptr_t)NitroPort_Oam;
    uintptr_t end = begin + size;
    if (size == 0 || end < begin || end <= base || begin >= base + 0x800) {
        return;
    }
    if (begin < base) {
        begin = base;
    }
    if (end > base + 0x800) {
        end = base + 0x800;
    }
    for (uintptr_t p = (begin - base) / 8; p <= (end - base - 1) / 8; ++p) {
        if ((begin - base) < p * 8 + 6 && end - base > p * 8) {
            s_oam_active[p] = 0;
        }
    }
}

const int8_t *NitroPort_WideObjOffsets(void)
{
    for (unsigned engine = 0; engine < 2; ++engine) {
        for (unsigned slot = 0; slot < 128; ++slot) {
            unsigned index = engine * 128 + slot;
            if (s_oam_active[index] &&
                memcmp(s_oam_attrs[engine][slot], NitroPort_Oam + engine * 0x400 + slot * 8, 6)) {
                s_oam_active[index] = 0;
            }
        }
    }
    return s_oam_active;
}

/* CPU views of VRAM (see the note in mmap_global.h). */
unsigned char NitroPort_BgVram[0x80000] ALIGNED;
unsigned char NitroPort_DbBgVram[0x20000] ALIGNED;
unsigned char NitroPort_ObjVram[0x40000] ALIGNED;
unsigned char NitroPort_DbObjVram[0x20000] ALIGNED;
/* 4 KB aligned: the texture loader stores LCDC addresses as 4 KB page numbers. */
unsigned char NitroPort_LcdcVram[0xA4000] __attribute__((aligned(4096)));
