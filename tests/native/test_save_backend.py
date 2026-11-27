#!/usr/bin/env python3
"""Host regression for the raw-save backend path contract (cleanup task 3).

Compiles the production port_card.c and port_save_import.c for the host with
small NitroSDK stubs, then exercises NitroPort_SetSavePath against temporary
files: new save creation, an existing valid image, an importable container,
a missing parent, an unusable path, a failed reopen, and repeated calls.

Run from the repository root: python tests/native/test_save_backend.py
"""

import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]

NITRO_H = """
#ifndef NITRO_H_
#define NITRO_H_
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
typedef enum {
    CARD_REQ_INIT = 0,
    CARD_REQ_ACK = 1,
    CARD_REQ_IDENTIFY = 2,
    CARD_REQ_READ_BACKUP = 3,
    CARD_REQ_WRITE_BACKUP = 4,
    CARD_REQ_PROGRAM_BACKUP = 5,
    CARD_REQ_VERIFY_BACKUP = 6,
    CARD_REQ_ERASE_PAGE_BACKUP = 7,
    CARD_REQ_ERASE_SECTOR_BACKUP = 8,
    CARD_REQ_ERASE_CHIP_BACKUP = 12,
    CARD_REQ_READ_STATUS = 13,
    CARD_REQ_WRITE_STATUS = 14,
    CARD_REQ_ERASE_SUBSECTOR_BACKUP = 15
} CARDRequest;
typedef enum {
    CARD_RESULT_SUCCESS = 0,
    CARD_RESULT_FAILURE = 1,
    CARD_RESULT_UNSUPPORTED = 2,
    CARD_RESULT_NO_RESPONSE = 3
} CARDResult;
#define CARD_BACKUP_TYPE_FLASH_4MBITS 3
#define PXI_FIFO_TAG_FS 4
typedef struct CARDiCommandArg {
    u32 type;
    u32 src;
    u32 dst;
    u32 len;
    CARDResult result;
} CARDiCommandArg;
typedef struct {
    CARDiCommandArg *cmd;
    u32 len;
    u32 dst;
} CardCommon;
extern CardCommon cardi_common;
#endif
"""

NITRO_PORT_H = """
#ifndef NITRO_PORT_H_
#define NITRO_PORT_H_
#include <nitro.h>
void NitroPort_Log(const char *format, ...);
void NitroPort_Arm7Reply(int tag, u32 data, BOOL err);
BOOL NitroPort_SetSavePath(const char *path);
#endif
"""

CARD_COMMON_H = """
#ifndef HGSS_COMPAT_CARD_COMMON_H
#define HGSS_COMPAT_CARD_COMMON_H
#define CARD_REQ_READ_STATUS 13
#define CARD_REQ_WRITE_STATUS 14
#define CARD_REQ_ERASE_SUBSECTOR_BACKUP 15
#endif
"""

TEST_C = r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <nitro.h>
#include "port_platform.h"

BOOL NitroPort_SetSavePath(const char *path);

static const size_t SAVE_BYTES = 512u * 1024u;

CardCommon cardi_common;

uint64_t NitroPortPlatform_Nanoseconds(void)
{
    return 0;
}

void NitroPortPlatform_Sleep(uint32_t microseconds)
{
    (void)microseconds;
}

void NitroPort_Log(const char *format, ...)
{
    (void)format;
}

void NitroPort_Arm7Reply(int tag, u32 data, BOOL err)
{
    (void)tag;
    (void)data;
    (void)err;
}

static void path_join(char *out, size_t size, const char *dir, const char *leaf)
{
    int written = snprintf(out, size, "%s/%s", dir, leaf);
    assert(written > 0 && (size_t)written < size);
}

static void write_file(const char *path, const unsigned char *bytes, size_t size)
{
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    assert(fwrite(bytes, 1, size, file) == size);
    assert(fclose(file) == 0);
}

static long file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    return (long)st.st_size;
}

static unsigned char *read_file(const char *path, size_t size)
{
    unsigned char *bytes = malloc(size);
    FILE *file = fopen(path, "rb");
    assert(bytes != NULL);
    assert(file != NULL);
    assert(fread(bytes, 1, size, file) == size);
    assert(fclose(file) == 0);
    return bytes;
}

static void test_new_save(const char *root)
{
    char saves[1024], path[1024];
    path_join(saves, sizeof(saves), root, "saves");
    mkdir(saves, 0777);
    path_join(path, sizeof(path), saves, "heartgold.sav");

    assert(NitroPort_SetSavePath(path) == TRUE);
    assert(file_size(path) == (long)SAVE_BYTES);
    unsigned char *bytes = read_file(path, SAVE_BYTES);
    assert(bytes[0] == 0xFF && bytes[SAVE_BYTES - 1] == 0xFF);
    free(bytes);
    printf("PASS new save created and filled with 0xFF\n");
}

static void test_existing_valid(const char *root)
{
    char path[1024];
    path_join(path, sizeof(path), root, "existing.sav");
    unsigned char *image = malloc(SAVE_BYTES);
    assert(image != NULL);
    for (size_t i = 0; i < SAVE_BYTES; ++i) {
        image[i] = (unsigned char)(i * 7u);
    }
    write_file(path, image, SAVE_BYTES);

    assert(NitroPort_SetSavePath(path) == TRUE);
    unsigned char *back = read_file(path, SAVE_BYTES);
    assert(memcmp(image, back, SAVE_BYTES) == 0);
    free(back);
    free(image);
    printf("PASS existing valid save opened without modification\n");
}

static void test_importable(const char *root)
{
    char path[1024], orig[1032];
    path_join(path, sizeof(path), root, "imported.sav");
    snprintf(orig, sizeof(orig), "%s.orig", path);
    (void)remove(orig);

    unsigned char *container = calloc(1, 0x4C + SAVE_BYTES);
    assert(container != NULL);
    memcpy(container, "NocashGbaBackupMediaSavDataFile\x1a", 33);
    memcpy(container + 0x40, "SRAM", 4);
    /* 0x44 = 0 (uncompressed), 0x48 = length */
    container[0x48] = (unsigned char)(SAVE_BYTES & 0xFF);
    container[0x49] = (unsigned char)((SAVE_BYTES >> 8) & 0xFF);
    container[0x4A] = (unsigned char)((SAVE_BYTES >> 16) & 0xFF);
    container[0x4B] = (unsigned char)((SAVE_BYTES >> 24) & 0xFF);
    for (size_t i = 0; i < SAVE_BYTES; ++i) {
        container[0x4C + i] = (unsigned char)(255u - (i & 0xFFu));
    }
    write_file(path, container, 0x4C + SAVE_BYTES);

    assert(NitroPort_SetSavePath(path) == TRUE);
    assert(file_size(path) == (long)SAVE_BYTES);
    assert(file_size(orig) == (long)(0x4C + SAVE_BYTES));
    unsigned char *raw = read_file(path, SAVE_BYTES);
    assert(raw[0] == 255 && raw[255] == 0 && raw[256] == 255);
    free(raw);
    free(container);
    printf("PASS importable save converted and original retained as .orig\n");
}

static void test_missing_parent(const char *root)
{
    char path[1024];
    path_join(path, sizeof(path), root, "no-such-dir/heartgold.sav");
    assert(NitroPort_SetSavePath(path) == FALSE);
    printf("PASS missing save directory reports failure\n");
}

static void test_unusable_path(const char *root)
{
    char path[1024];
    path_join(path, sizeof(path), root, "a-directory.sav");
    mkdir(path, 0777);
    assert(NitroPort_SetSavePath(path) == FALSE);
    printf("PASS a directory in place of the save file reports failure\n");

    /* A non-save file must not be truncated or replaced by a blank save. */
    char bad[1024];
    path_join(bad, sizeof(bad), root, "invalid.sav");
    const unsigned char junk[64] = { 1, 2, 3, 4 };
    write_file(bad, junk, sizeof(junk));
    assert(NitroPort_SetSavePath(bad) == FALSE);
    assert(file_size(bad) == (long)sizeof(junk));
    unsigned char *back = read_file(bad, sizeof(junk));
    assert(memcmp(back, junk, sizeof(junk)) == 0);
    free(back);
    printf("PASS invalid existing save is refused and left untouched\n");
}

static void test_reopen_after_failure(const char *root)
{
    char path[1024], good[1024];
    path_join(path, sizeof(path), root, "still-missing/heartgold.sav");
    path_join(good, sizeof(good), root, "recovered.sav");
    assert(NitroPort_SetSavePath(path) == FALSE);
    assert(NitroPort_SetSavePath(good) == TRUE);
    assert(file_size(good) == (long)SAVE_BYTES);
    assert(NitroPort_SetSavePath(NULL) == TRUE);
    assert(NitroPort_SetSavePath(good) == TRUE);
    printf("PASS a later SetSavePath recovers and NULL clears\n");
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    test_new_save(argv[1]);
    test_existing_valid(argv[1]);
    test_importable(argv[1]);
    test_missing_parent(argv[1]);
    test_unusable_path(argv[1]);
    test_reopen_after_failure(argv[1]);
    printf("PASS save backend: all cases\n");
    return 0;
}
"""


def main():
    cc = os.environ.get("CC", "gcc")
    with tempfile.TemporaryDirectory(prefix="kohjoh-save-") as tmp:
        tmp = Path(tmp)
        (tmp / "nitro.h").write_text(NITRO_H)
        (tmp / "nitro_port.h").write_text(NITRO_PORT_H)
        (tmp / "card_common.h").write_text(CARD_COMMON_H)
        (tmp / "test.c").write_text(TEST_C)
        exe = tmp / "save_backend_test.exe"
        args = [cc, "-std=gnu11", "-O1", "-g", "-Wall", "-Wextra",
                "-I", tmp.as_posix(),
                "-I", (ROOT / "src/compat/nitro/src").as_posix(),
                (ROOT / "src/compat/nitro/src/port_card.c").as_posix(),
                (ROOT / "src/compat/nitro/src/port_save_import.c").as_posix(),
                (tmp / "test.c").as_posix(),
                "-o", exe.as_posix()]
        subprocess.run(args, cwd=ROOT, check=True)
        with tempfile.TemporaryDirectory(prefix="kohjoh-save-data-") as data:
            subprocess.run([exe.as_posix(), data], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
