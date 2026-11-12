/*
 * kj_boot.c: what the DS firmware and cartridge loader did before the
 * game's NitroMain, on the DS side of the bridge.
 *
 * Writes the cartridge header and firmware settings the SDK reads, hands the
 * compat layer the overlay table, registers the virtual ARM7 services and
 * runs the original NitroMain. Cartridge-header values and the fixed MAC
 * address follow hgss-3ds (hgss/game/app/heartgold.c @ a26c003, GPL-3.0).
 */

#include "kj_bridge.h"

#include <nitro.h>
#include <nitro_port.h>
#include <nitro_port_fs.h>
#include <string.h>

extern const NitroPortOverlay gHeartgoldOverlays[];   /* gen/overlay_table.c */
extern const u32 gHeartgoldOverlayCount;

void NitroMain(void);

#if defined(SOULSILVER)
#define KJ_EDITION "soulsilver"
#define KJ_PAK_BUILD_ID "soulsilver-nitrofs-1"
#define KJ_ROM_GAME_CODE "IPGE"
#define KJ_ROM_TITLE "POKEMON SS"
#else
#define KJ_EDITION "heartgold"
#define KJ_PAK_BUILD_ID "heartgold-nitrofs-1"
#define KJ_ROM_GAME_CODE "IPKE"
#define KJ_ROM_TITLE "POKEMON HG"
#endif

/* The game stores the console's MAC address in the save and compares it on
 * load; one fixed address keeps every save "from this DS". */
static const u8 MAC_ADDRESS[6] = { 0x00, 0x09, 0xBF, 0x48, 0x47, 0x53 };

#define ROM_HEADER_SIZE 0x200
#define ROM_TITLE_OFFSET 0x00
#define ROM_GAME_CODE_OFFSET 0x0C
#define ROM_MAKER_CODE_OFFSET 0x10
#define ROM_DEVICE_SIZE_OFFSET 0x14
#define ROM_VERSION_OFFSET 0x1E

const char *kj_ds_edition_name(void)
{
    return KJ_EDITION;
}

const char *kj_ds_pack_build_id(void)
{
    return KJ_PAK_BUILD_ID;
}

void kj_ds_set_save_path(const char *path)
{
    NitroPort_SetSavePath(path);
}

const char *kj_ds_open_pack(const char *pak_path, const char *manifest_path)
{
    switch (NitroPort_OpenPak(pak_path, manifest_path, KJ_PAK_BUILD_ID)) {
    case PAK_OK:
        return NULL;
    case PAK_ERR_IO:
        return "cannot read the data pack or its manifest";
    case PAK_ERR_FORMAT:
        return "the data pack or its manifest is damaged, or they do not belong together";
    case PAK_ERR_VERSION:
        return "the data pack was made by a different version of the pack builder";
    case PAK_ERR_BUILD_ID:
        return "the data pack is for the other edition (wrong build ID)";
    case PAK_ERR_ROM_SHA1:
        return "the data pack was made from a ROM this version does not support";
    case PAK_ERR_MEMORY:
        return "out of memory opening the data pack";
    default:
        return "cannot open the data pack";
    }
}

void kj_game_main(const kj_game_settings *settings)
{
    static u8 header[ROM_HEADER_SIZE];
    NitroPortFirmware firmware;

    memset(header, 0, sizeof(header));
    memcpy(header + ROM_TITLE_OFFSET, KJ_ROM_TITLE, sizeof(KJ_ROM_TITLE) - 1);
    memcpy(header + ROM_GAME_CODE_OFFSET, KJ_ROM_GAME_CODE, sizeof(KJ_ROM_GAME_CODE) - 1);
    memcpy(header + ROM_MAKER_CODE_OFFSET, "01", 2);
    header[ROM_DEVICE_SIZE_OFFSET] = 10; /* 128 MB card */
    header[ROM_VERSION_OFFSET] = 0;

    memset(&firmware, 0, sizeof(firmware));
    firmware.romHeader = header;
    firmware.romHeaderSize = sizeof(header);
    firmware.nickname = settings->nickname;
    firmware.language = settings->language;
    firmware.birthMonth = settings->birth_month;
    firmware.birthDay = settings->birth_day;
    firmware.favoriteColor = settings->favorite_color;
    memcpy(firmware.macAddress, MAC_ADDRESS, sizeof(MAC_ADDRESS));
    firmware.rtcOffset = 0;
    NitroPort_BootFirmware(&firmware);

    NitroPort_SetOverlays(gHeartgoldOverlays, gHeartgoldOverlayCount);
    NitroPort_InitArm7Services();

    NitroPort_Log("[kj] starting %s", KJ_EDITION);
    NitroMain();
    NitroPort_Log("[kj] NitroMain returned");
    OS_Terminate();
    for (;;) {
    }
}
