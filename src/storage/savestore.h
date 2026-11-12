#ifndef KJ_STORAGE_SAVESTORE_H
#define KJ_STORAGE_SAVESTORE_H

#include "storage/paths.h"

#include <stdbool.h>

/* Attach the two-generation save store to the game's save chip. */
bool kj_savestore_init(const kj_paths *paths);
void kj_savestore_exit(void);

#endif
