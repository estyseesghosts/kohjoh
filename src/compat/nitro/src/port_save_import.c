/* Emulator save containers: DeSmuME mc.cpp readNoGbaSaveInfo and ensure,
 * https://github.com/TASEmulators/desmume/blob/master/desmume/src/mc.cpp.
 * The earlier Kohjoh importer was tested with real exports. Filesystem
 * durability constraints: coffeepot/ctr/engineering/filesystem-and-saves.md. */
#include "port_save_import.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_IMPORT_BYTES (2u * 1024u * 1024u)
#define MAX_BACKUPS 1000u

static const char *s_import_failure;
static int s_import_errno;

const char *port_save_import_failure(void)
{
    return s_import_failure;
}

int port_save_import_errno(void)
{
    return s_import_errno;
}

static uint32_t read_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
           (uint32_t)p[3] << 24;
}

bool port_save_decode(const unsigned char *input, size_t size, unsigned char *output,
                      const char **kind)
{
    static const char snip[] =
        "|<--Snip above here to create a raw sav by excluding this DeSmuME savedata footer:";
    static const char end[] = "|-DESMUME SAVE-|";
    static const char magic[] = "NocashGbaBackupMediaSavDataFile\x1a";
    size_t src = 0x50, dst = 0;

    if (kind != NULL) {
        *kind = NULL;
    }
    if (input == NULL || output == NULL || kind == NULL || size > MAX_IMPORT_BYTES) {
        return false;
    }
    if (size == PORT_SAVE_BYTES + sizeof(snip) - 1 + 24 + sizeof(end) - 1 &&
        memcmp(input + PORT_SAVE_BYTES, snip, sizeof(snip) - 1) == 0 &&
        memcmp(input + size - (sizeof(end) - 1), end, sizeof(end) - 1) == 0) {
        const unsigned char *footer = input + PORT_SAVE_BYTES + sizeof(snip) - 1;
        if (read_u32(footer) > PORT_SAVE_BYTES || read_u32(footer + 4) != PORT_SAVE_BYTES ||
            read_u32(footer + 20) != 0) {
            return false;
        }
        memcpy(output, input, PORT_SAVE_BYTES);
        *kind = "DeSmuME .dsv";
        return true;
    }
    if (size < 0x4c || memcmp(input, magic, sizeof(magic) - 1) != 0 ||
        memcmp(input + 0x40, "SRAM", 4) != 0) {
        return false;
    }
    /* Real No$GBA exports carry further chunks after the SRAM block (a
     * "LIFF" block and a "STOP" marker in the Platinum and HeartGold
     * exports tested): the SRAM block's own length bounds its data, not
     * the file's end. Bytes after it are not part of the save. */
    if (read_u32(input + 0x44) == 0) {
        if (read_u32(input + 0x48) != PORT_SAVE_BYTES || size - 0x4c < PORT_SAVE_BYTES) {
            return false;
        }
        memcpy(output, input + 0x4c, PORT_SAVE_BYTES);
        *kind = "No$GBA uncompressed";
        return true;
    }
    if (size < 0x50 || read_u32(input + 0x44) != 1 ||
        read_u32(input + 0x4c) != PORT_SAVE_BYTES ||
        read_u32(input + 0x48) > size - 0x4c) {
        return false;
    }
    /* The SRAM block length at 0x48 counts from 0x4c: it covers the
     * uncompressed-size word as well as the packed data (both authentic
     * packed exports tested end their terminator exactly there). */
    const size_t limit = 0x4c + (size_t)read_u32(input + 0x48);
    while (src < limit) {
        unsigned code = input[src++];
        size_t run;
        if (code == 0) {
            if (dst != PORT_SAVE_BYTES || src != limit) {
                return false;
            }
            *kind = "No$GBA packed";
            return true;
        }
        if (code < 0x80) {
            run = code;
            if (run > limit - src || run > PORT_SAVE_BYTES - dst) {
                return false;
            }
            memcpy(output + dst, input + src, run);
            src += run;
        } else {
            if (src >= limit) {
                return false;
            }
            unsigned char value = input[src++];
            if (code == 0x80) {
                if (limit - src < 2) {
                    return false;
                }
                run = (size_t)input[src] | (size_t)input[src + 1] << 8;
                src += 2;
            } else {
                run = code - 0x80;
            }
            if (run == 0 || run > PORT_SAVE_BYTES - dst) {
                return false;
            }
            memset(output + dst, value, run);
        }
        dst += run;
    }
    return false;
}

static bool make_name(char *out, size_t cap, const char *path, const char *suffix,
                      unsigned index)
{
    int n = index == 0 ? snprintf(out, cap, "%s%s", path, suffix) :
                         snprintf(out, cap, "%s%s.%u", path, suffix, index);
    return n >= 0 && (size_t)n < cap;
}

bool port_save_has_backup(const char *path)
{
    const char *base = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    if (backslash != NULL && (base == NULL || backslash > base)) {
        base = backslash;
    }
    const size_t dir_len = base == NULL ? 1 : (size_t)(base - path) + 1;
    size_t name_len;
    char *directory = malloc(dir_len + 1);
    if (directory == NULL) {
        return true;
    }
    if (base == NULL) {
        memcpy(directory, ".", 2);
        base = path;
    } else {
        memcpy(directory, path, dir_len);
        directory[dir_len] = '\0';
        ++base;
    }
    name_len = strlen(base);
    DIR *dir = opendir(directory);
    free(directory);
    if (dir == NULL) {
        return true;
    }
    bool found = false;
    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        const char *suffix;
        if (strncmp(entry->d_name, base, name_len) != 0) {
            continue;
        }
        suffix = entry->d_name + name_len;
        if (strncmp(suffix, ".orig", 5) != 0 && strncmp(suffix, ".tmp", 4) != 0) {
            continue;
        }
        suffix += suffix[1] == 'o' ? 5 : 4;
        if (*suffix == '\0') {
            found = true;
            break;
        }
        if (*suffix++ != '.' || *suffix == '\0') {
            continue;
        }
        while (*suffix >= '0' && *suffix <= '9') {
            ++suffix;
        }
        if (*suffix == '\0') {
            found = true;
            break;
        }
    }
    if (errno != 0) {
        found = true;
    }
    if (closedir(dir) != 0) {
        found = true;
    }
    return found;
}

/* Create without truncating an existing backup or temp file. */
static FILE *create_unique(char *name, size_t cap, const char *path, const char *suffix)
{
    for (unsigned i = 0; i < MAX_BACKUPS; ++i) {
        if (!make_name(name, cap, path, suffix, i)) {
            return NULL;
        }
        int fd = open(name, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            FILE *file = fdopen(fd, "wb");
            if (file == NULL) {
                int saved_errno = errno;
                close(fd);
                unlink(name);
                errno = saved_errno;
            }
            return file;
        }
        /* Some filesystem implementations translate a collision to a
         * generic errno.  Only skip a candidate confirmed to exist; never
         * reopen or truncate it, and retain exclusive creation for the next. */
        int open_error = errno;
#ifdef PORT_SAVE_IMPORT_TEST
        const char *fault = getenv("PORT_SAVE_IMPORT_TEST_FAULT");
        if (open_error == EEXIST && fault != NULL && strcmp(fault, "collision_errno") == 0) {
            open_error = EIO;
        }
#endif
        if (open_error != EEXIST) {
            struct stat info;
            if (stat(name, &info) != 0) {
                errno = open_error;
                return NULL;
            }
        }
    }
    return NULL;
}

static bool write_checked(FILE *file, const unsigned char *data, size_t size, const char *phase)
{
    size_t write_size = size;
#ifdef PORT_SAVE_IMPORT_TEST
    const char *fault = getenv("PORT_SAVE_IMPORT_TEST_FAULT");
    if (fault != NULL && strcmp(fault, phase) == 0) {
        write_size = size / 2;
    }
#else
    (void)phase;
#endif
    bool ok = fwrite(data, 1, write_size, file) == size;
    if (fflush(file) != 0 || fsync(fileno(file)) != 0) {
        ok = false;
    }
    if (fclose(file) != 0) {
        ok = false;
    }
    return ok;
}

static bool verify_file(const char *path, const unsigned char *data, size_t size)
{
    unsigned char buffer[4096];
    FILE *file = fopen(path, "rb");
    bool ok = file != NULL;
    for (size_t pos = 0; ok && pos < size;) {
        size_t count = size - pos < sizeof(buffer) ? size - pos : sizeof(buffer);
        ok = fread(buffer, 1, count, file) == count && memcmp(buffer, data + pos, count) == 0;
        pos += count;
    }
    if (file != NULL) {
        ok = ok && fgetc(file) == EOF && !ferror(file);
        if (fclose(file) != 0) {
            ok = false;
        }
    }
    return ok;
}

bool port_save_import(const char *path, unsigned char *image, const char **kind)
{
    FILE *input = NULL, *file = NULL;
    unsigned char *bytes = NULL;
    size_t cap = strlen(path) + 32;
    char *backup = malloc(cap), *temp = malloc(cap);
    bool ok = false;
    long length;

    s_import_failure = "read or decode input";
    s_import_errno = 0;
    errno = 0;
    *kind = NULL;
    if (backup == NULL || temp == NULL || (input = fopen(path, "rb")) == NULL ||
        fseek(input, 0, SEEK_END) != 0 || (length = ftell(input)) <= 0 ||
        (unsigned long)length > MAX_IMPORT_BYTES || fseek(input, 0, SEEK_SET) != 0 ||
        (bytes = malloc((size_t)length)) == NULL ||
        fread(bytes, 1, (size_t)length, input) != (size_t)length ||
        !port_save_decode(bytes, (size_t)length, image, kind)) {
        goto done;
    }
    s_import_failure = "close input";
    errno = 0;
    if (fclose(input) != 0) {
        input = NULL;
        goto done;
    }
    input = NULL;
    s_import_failure = "create backup";
    errno = 0;
    file = create_unique(backup, cap, path, ".orig");
    if (file == NULL) {
        goto done;
    }
    s_import_failure = "write or verify backup";
    errno = 0;
    if (!write_checked(file, bytes, (size_t)length, "backup") ||
        !verify_file(backup, bytes, (size_t)length)) {
        goto done;
    }
    s_import_failure = "create temporary image";
    errno = 0;
    file = create_unique(temp, cap, path, ".tmp");
    if (file == NULL) {
        goto done;
    }
    s_import_failure = "write or verify temporary image";
    errno = 0;
    if (!write_checked(file, image, PORT_SAVE_BYTES, "temp") ||
        !verify_file(temp, image, PORT_SAVE_BYTES)) {
        int saved_errno = errno;
        unlink(temp);
        errno = saved_errno;
        goto done;
    }
    /* libctru may remove the destination before renaming. The independently
     * verified backup is retained even if this operation fails or power is lost. */
    s_import_failure = "replace active save";
    errno = 0;
    int rename_result;
#ifdef PORT_SAVE_IMPORT_TEST
    const char *fault = getenv("PORT_SAVE_IMPORT_TEST_FAULT");
    if (fault != NULL && strcmp(fault, "rename") == 0) {
        /* Simulate libctru's delete-then-rename fallback losing the target. */
        unlink(path);
        rename_result = -1;
    } else
#endif
    {
        rename_result = rename(temp, path);
    }
    if (rename_result != 0) {
        int saved_errno = errno;
        unlink(temp);
        errno = saved_errno;
        goto done;
    }
    ok = true;
    s_import_failure = NULL;
done:
    if (!ok) {
        s_import_errno = errno;
    }
    if (input != NULL) {
        fclose(input);
    }
    free(bytes);
    free(backup);
    free(temp);
    return ok;
}
