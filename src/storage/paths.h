#ifndef KJ_STORAGE_PATHS_H
#define KJ_STORAGE_PATHS_H

#include <stdbool.h>

/* Where this program's data, save and log live. One root, never the
 * current working directory (kohjoh.md 4.2). */
typedef struct kj_paths {
    bool bundled;              /* installed title: pack in RomFS, save in the title archive */
    char root[96];             /* sdmc:/3ds/kohjoh */
    char pack_file[128];
    char manifest_file[128];
    char save_file[128];
    char log_file[128];
} kj_paths;

const kj_paths *kj_paths_init(void);

/* Open the save backend and attach it to the game's save chip. */
bool kj_save_backend_init(const kj_paths *paths);

#endif
