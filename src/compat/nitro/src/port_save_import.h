#ifndef PORT_SAVE_IMPORT_H
#define PORT_SAVE_IMPORT_H

#include <stdbool.h>
#include <stddef.h>

#define PORT_SAVE_BYTES (512u * 1024u)

/* On success replaces path with a verified raw image and retains the original.
 * On failure path may be missing after a failed rename; the .orig copy remains.
 * `kind` is a static diagnostic string or NULL. */
bool port_save_import(const char *path, unsigned char *image, const char **kind);
/* Static diagnostic stage from the most recent import; NULL on success. */
const char *port_save_import_failure(void);
/* Captured errno for the failure, or 0 if no system error was reported. */
int port_save_import_errno(void);
/* Backup or temporary files prevent a fresh save after interrupted replacement.
 * Inability to inspect the directory also fails closed. */
bool port_save_has_backup(const char *path);
bool port_save_decode(const unsigned char *input, size_t size, unsigned char *output,
                      const char **kind);

#endif
