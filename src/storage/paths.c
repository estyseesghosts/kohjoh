/*
 * paths.c: storage roots for the two installation formats.
 *
 * .3dsx: everything under sdmc:/3ds/kohjoh/ (data/<edition>.pak and its
 *        manifest, saves/, logs/).
 * .cia:  the pack is romfs:/data/game.pak (+ game.manifest.json); the save
 *        still uses the raw .sav under the SD root (no title save archive
 *        backend exists yet).
 *
 * data/ and logs/ may be best-effort; saves/ must be established or startup
 * fails, because a save path that looks valid after mkdir failed would only
 * be discovered at the first card request.
 */

#include "storage/paths.h"

#include "diagnostics/log.h"
#include "runtime/kj_bridge.h"

#include <3ds.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static kj_paths s_paths;

static bool file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static bool ensure_dir(const char *path) {
    if (mkdir(path, 0777) == 0 || errno == EEXIST) {
        return true;
    }
    kj_log("[paths] cannot create %s (errno %d)", path, errno);
    return false;
}

const kj_paths *kj_paths_init(void) {
    const char *edition = kj_ds_edition_name();

    snprintf(s_paths.root, sizeof(s_paths.root), "sdmc:/3ds/kohjoh");
    (void)ensure_dir("sdmc:/3ds");
    (void)ensure_dir(s_paths.root);
    {
        char dir[128];
        snprintf(dir, sizeof(dir), "%s/data", s_paths.root);
        (void)ensure_dir(dir);
        snprintf(dir, sizeof(dir), "%s/saves", s_paths.root);
        s_paths.save_dir_ready = ensure_dir(dir);
        snprintf(dir, sizeof(dir), "%s/logs", s_paths.root);
        (void)ensure_dir(dir);
    }
    snprintf(s_paths.log_file, sizeof(s_paths.log_file), "%s/logs/%s.log", s_paths.root, edition);

    if (R_SUCCEEDED(romfsInit()) && file_exists("romfs:/data/game.pak")) {
        s_paths.bundled = true;
        snprintf(s_paths.pack_file, sizeof(s_paths.pack_file), "romfs:/data/game.pak");
        snprintf(s_paths.manifest_file, sizeof(s_paths.manifest_file),
                 "romfs:/data/game.manifest.json");
    } else {
        s_paths.bundled = false;
        snprintf(s_paths.pack_file, sizeof(s_paths.pack_file), "%s/data/%s.pak", s_paths.root,
                 edition);
        snprintf(s_paths.manifest_file, sizeof(s_paths.manifest_file), "%s/data/%s.manifest.json",
                 s_paths.root, edition);
    }
    snprintf(s_paths.save_file, sizeof(s_paths.save_file), "%s/saves/%s.sav", s_paths.root,
             edition);
    return &s_paths;
}
