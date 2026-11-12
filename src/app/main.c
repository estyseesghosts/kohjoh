/*
 * main.c: kohjoh's 3DS entry, lifecycle and the 3DS side of the frame bridge.
 *
 * The original game runs on this (main) thread: kj_game_main never returns.
 * Every emulated DS V-blank comes back here through kj_ctr_vblank (pacing,
 * input, APT); once per game frame kj_ctr_publish_frame hands the coherent DS
 * state to the renderer. There is exactly one frame loop: the game's.
 *
 * O3DS baseline (kohjoh.md section 0): the program never enables the New 3DS
 * clock/L2 and runs in the ordinary 64 MB application memory mode.
 */

#include "app/app.h"
#include "app/input.h"
#include "audio/audio.h"
#include "diagnostics/log.h"
#include "diagnostics/perf.h"
#include "graphics/renderer.h"
#include "runtime/kj_bridge.h"
#include "storage/paths.h"

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* The game thread is this one; DS stacks were sized for 16 KB of DTCM but
 * GCC code needs far more (hgss-3ds found 512 KB insufficient once). */
u32 __stacksize__ = 1024 * 1024;

static bool s_closing;
static u32 s_vblank_count;

static void fatal_screen(const char *title, const char *detail)
{
    /* No game is running: a text console on the bottom screen is fine. */
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    printf("\x1b[1;1Hkohjoh\n\n%s\n\n%s\n\nPress START to exit.\n", title, detail != NULL ? detail : "");
    kj_log("[fatal] %s: %s", title, detail != NULL ? detail : "");
    kj_log_flush_now();
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) {
            break;
        }
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    gfxExit();
    exit(1);
}

bool kj_ctr_vblank(void)
{
    kj_perf_vblank_begin();
    /* One owner of APT, once per V-blank (hgss-3ds pitfall: "APT has one
     * owner"). aptMainLoop handles HOME, sleep and the close request. */
    if (!aptMainLoop()) {
        s_closing = true;
        return false;
    }
    kj_input_sample();
    kj_audio_service();
    /* One emulated DS V-blank per physical top-screen V-blank. */
    gspWaitForVBlank();
    ++s_vblank_count;
    kj_perf_vblank_end();
    return true;
}

void kj_ctr_terminate(void)
{
    kj_log("[app] terminating after %lu V-blanks%s", (unsigned long)s_vblank_count,
           s_closing ? " (close requested)" : "");
    kj_audio_exit();
    kj_renderer_exit();
    kj_log_shutdown();
    romfsExit();
    gfxExit();
    exit(0);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /* O3DS baseline: run at 268 MHz without the L2 even on a New 3DS. */
    osSetSpeedupEnable(false);

    const kj_paths *paths = kj_paths_init();
    kj_log_init(paths->log_file);
    bool is_new = false;
    APT_CheckNew3DS(&is_new);
    kj_log("[app] kohjoh %s (%s), %s, %s", KJ_BUILD_ID, kj_ds_edition_name(),
           paths->bundled ? "installed title (RomFS pack)" : "homebrew (SD pack)",
           is_new ? "New 3DS hardware held at O3DS clock" : "Old 3DS");

    gfxInitDefault();
    gfxSet3D(false);

    const char *pack_error = kj_ds_open_pack(paths->pack_file, paths->manifest_file);
    if (pack_error != NULL) {
        char detail[512];
        snprintf(detail, sizeof(detail), "%s\n\nExpected:\n%s\n%s", pack_error, paths->pack_file,
                 paths->manifest_file);
        gfxExit();
        fatal_screen("The game data pack could not be opened.", detail);
    }

    if (!kj_renderer_init()) {
        gfxExit();
        fatal_screen("The GPU renderer could not start.", "See the log for details.");
    }
    kj_input_init();
    if (!kj_audio_init()) {
        kj_log("[app] audio unavailable (is the DSP firmware dumped?); continuing silent");
    }
    if (!kj_save_backend_init(paths)) {
        gfxExit();
        fatal_screen("The save storage could not be opened.", paths->save_file);
    }

    kj_game_settings settings;
    memset(&settings, 0, sizeof(settings));
    {
        static const char name[] = "Player";
        for (unsigned i = 0; name[i] != '\0'; ++i) {
            settings.nickname[i] = (uint16_t)name[i];
        }
    }
    settings.language = 1; /* English */
    settings.birth_month = 1;
    settings.birth_day = 1;
    kj_log("[app] starting the game");
    kj_game_main(&settings);
    return 0;
}
