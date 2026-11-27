/*
 * save_raw.c: interim save backend: the game's 512 KiB save chip backed by
 * one raw file through the compat layer (port_card.c). To be replaced by the
 * two-generation SaveStore (kohjoh.md 4.4).
 */

#include "storage/paths.h"

#include "diagnostics/log.h"
#include "runtime/kj_bridge.h"

bool kj_save_backend_init(const kj_paths *paths)
{
    kj_log("[save] raw save file %s", paths->save_file);
    if (!paths->save_dir_ready) {
        kj_log("[save] the save directory could not be created; not starting the game");
        return false;
    }
    if (!kj_ds_set_save_path(paths->save_file)) {
        kj_log("[save] cannot open, create or import %s; see the log above", paths->save_file);
        return false;
    }
    return true;
}
