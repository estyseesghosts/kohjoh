/*
 * paths.c: storage roots for the two installation formats.
 *
 * .3dsx: everything under sdmc:/3ds/kohjoh/ (data/<edition>.pak and its
 *        manifest, saves/, logs/).
 * .cia:  the pack is romfs:/data/game.pak (+ game.manifest.json); saves
 *        live in the title's save archive (storage/save_archive.c).
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

static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static void ensure_dir(const char *path)
{
    if (mkdir(path, 0777) != 0 && errno != EEXIST) {
        kj_log("[paths] cannot create %s (errno %d)", path, errno);
    }
}

const kj_paths *kj_paths_init(void)
{
    const char *edition = kj_ds_edition_name();

    snprintf(s_paths.root, sizeof(s_paths.root), "sdmc:/3ds/kohjoh");
    ensure_dir("sdmc:/3ds");
    ensure_dir(s_paths.root);
    {
        char dir[128];
        snprintf(dir, sizeof(dir), "%s/data", s_paths.root);
        ensure_dir(dir);
        snprintf(dir, sizeof(dir), "%s/saves", s_paths.root);
        ensure_dir(dir);
        snprintf(dir, sizeof(dir), "%s/logs", s_paths.root);
        ensure_dir(dir);
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
        snprintf(s_paths.manifest_file, sizeof(s_paths.manifest_file),
                 "%s/data/%s.manifest.json", s_paths.root, edition);
    }
    snprintf(s_paths.save_file, sizeof(s_paths.save_file), "%s/saves/%s.sav", s_paths.root,
             edition);
    return &s_paths;
}
