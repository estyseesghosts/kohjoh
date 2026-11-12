/*
 * savestore.c: the game's 512 KiB save chip as two transactional
 * generations (kohjoh.md 4.4).
 *
 * Each generation file is a 64-byte header followed by the unmodified HGSS
 * save image (the original format, so a raw .sav exports by stripping the
 * header). Load picks the newest generation whose header and payload CRCs
 * check out. A commit writes the inactive generation completely, flushes and
 * closes it (and commits the archive for an installed title) before it
 * counts: a write interrupted at any point leaves the other generation whole.
 * Rename is not used (not power-fail atomic on FAT).
 *
 * .3dsx: sdmc:/3ds/kohjoh/saves/<edition>_a.bin, <edition>_b.bin
 * .cia : save:/game_a.bin, save:/game_b.bin in the title's save archive
 * A legacy raw <edition>.sav in the saves folder is imported once when no
 * generation exists yet (it is left in place, never overwritten).
 */

#include "storage/savestore.h"

#include "diagnostics/log.h"
#include "runtime/kj_bridge.h"

#include <3ds.h>
#include <stdio.h>
#include <string.h>

#define SAVE_BYTES (512u * 1024u)
#define HEADER_BYTES 64u
#define SAVE_MAGIC 0x5653504Bu /* "KJSV" little endian */
#define SAVE_VERSION 1u

typedef struct save_header {
    uint32_t magic;
    uint32_t version;
    char edition[16];
    uint32_t generation;
    uint32_t length;
    uint32_t payload_crc;
    uint32_t header_crc; /* over the bytes before this field */
    uint8_t reserved[HEADER_BYTES - 40];
} save_header;

_Static_assert(sizeof(save_header) == HEADER_BYTES, "save header is 64 bytes");

static char s_gen_path[2][128];
static char s_legacy_path[128];
static bool s_archive;
static uint32_t s_generation;
static int s_next_slot;
static uint32_t s_commits, s_failures;
static uint32_t s_crc_table[256];

static void crc_init(void)
{
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int k = 0; k < 8; ++k) {
            c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        }
        s_crc_table[i] = c;
    }
}

static uint32_t crc32(const void *data, size_t len)
{
    const uint8_t *p = data;
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        c = s_crc_table[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFu;
}

static bool read_generation(const char *path, void *image, uint32_t *generation)
{
    save_header h;
    FILE *f = fopen(path, "rb");
    bool ok = false;

    if (f == NULL) {
        return false;
    }
    if (fread(&h, 1, sizeof(h), f) == sizeof(h) && h.magic == SAVE_MAGIC &&
        h.version == SAVE_VERSION && h.length == SAVE_BYTES &&
        h.header_crc == crc32(&h, offsetof(save_header, header_crc)) &&
        strncmp(h.edition, kj_ds_edition_name(), sizeof(h.edition)) == 0 &&
        fread(image, 1, SAVE_BYTES, f) == SAVE_BYTES && crc32(image, SAVE_BYTES) == h.payload_crc) {
        *generation = h.generation;
        ok = true;
    }
    fclose(f);
    return ok;
}

static bool load(void *image, size_t size)
{
    static uint8_t other[SAVE_BYTES];
    uint32_t g[2] = { 0, 0 };
    bool v[2];

    if (size != SAVE_BYTES) {
        return false;
    }
    v[0] = read_generation(s_gen_path[0], image, &g[0]);
    v[1] = read_generation(s_gen_path[1], other, &g[1]);
    if (v[1] && (!v[0] || g[1] > g[0])) {
        memcpy(image, other, SAVE_BYTES);
        s_generation = g[1];
        s_next_slot = 0;
        kj_log("[save] loaded generation %lu (copy B)", (unsigned long)g[1]);
        return true;
    }
    if (v[0]) {
        s_generation = g[0];
        s_next_slot = 1;
        kj_log("[save] loaded generation %lu (copy A)%s", (unsigned long)g[0],
               v[1] ? "" : "; copy B missing or damaged");
        return true;
    }
    /* No generation yet: import a raw DS save if one is there. */
    if (s_legacy_path[0] != '\0') {
        FILE *f = fopen(s_legacy_path, "rb");
        if (f != NULL) {
            const bool ok = fread(image, 1, SAVE_BYTES, f) == SAVE_BYTES;
            fclose(f);
            if (ok) {
                s_generation = 0;
                s_next_slot = 0;
                kj_log("[save] imported raw save %s (written as a generation on the next save)",
                       s_legacy_path);
                return true;
            }
        }
    }
    memset(image, 0xFF, SAVE_BYTES);
    s_generation = 0;
    s_next_slot = 0;
    kj_log("[save] no save: starting blank");
    return true;
}

static bool commit(const void *image, size_t size)
{
    save_header h;
    const uint64_t t0 = svcGetSystemTick();
    const char *path = s_gen_path[s_next_slot];
    bool ok;

    if (size != SAVE_BYTES) {
        return false;
    }
    memset(&h, 0, sizeof(h));
    h.magic = SAVE_MAGIC;
    h.version = SAVE_VERSION;
    strncpy(h.edition, kj_ds_edition_name(), sizeof(h.edition) - 1);
    h.generation = s_generation + 1u;
    h.length = SAVE_BYTES;
    h.payload_crc = crc32(image, SAVE_BYTES);
    h.header_crc = crc32(&h, offsetof(save_header, header_crc));

    FILE *f = fopen(path, "wb");
    ok = f != NULL;
    if (ok) {
        ok = fwrite(&h, 1, sizeof(h), f) == sizeof(h) &&
             fwrite(image, 1, SAVE_BYTES, f) == SAVE_BYTES && fflush(f) == 0;
        if (fclose(f) != 0) {
            ok = false;
        }
    }
    if (ok && s_archive) {
        const Result rc = archiveCommitSaveData("save");
        if (R_FAILED(rc)) {
            kj_log("[save] archiveCommitSaveData failed: 0x%08lx", (unsigned long)rc);
            ok = false;
        }
    }
    if (!ok) {
        ++s_failures;
        kj_log("[save] COMMIT FAILED for %s (generation %lu kept)", path,
               (unsigned long)s_generation);
        return false;
    }
    s_generation = h.generation;
    s_next_slot ^= 1;
    ++s_commits;
    kj_log("[save] generation %lu committed to %s in %lu ms", (unsigned long)s_generation, path,
           (unsigned long)((svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000u)));
    return true;
}

static const kj_save_backend BACKEND = { load, commit };

bool kj_savestore_init(const kj_paths *paths)
{
    crc_init();
    if (paths->bundled) {
        Result rc = archiveMount(ARCHIVE_SAVEDATA, fsMakePath(PATH_EMPTY, ""), "save");
        if (R_FAILED(rc)) {
            kj_log("[save] save archive mount failed (0x%08lx): formatting", (unsigned long)rc);
            rc = FSUSER_FormatSaveData(ARCHIVE_SAVEDATA, fsMakePath(PATH_EMPTY, ""), 0x200, 10, 10,
                                       11, 11, true);
            if (R_SUCCEEDED(rc)) {
                rc = archiveMount(ARCHIVE_SAVEDATA, fsMakePath(PATH_EMPTY, ""), "save");
            }
            if (R_FAILED(rc)) {
                kj_log("[save] save archive unavailable: 0x%08lx", (unsigned long)rc);
                return false;
            }
        }
        s_archive = true;
        snprintf(s_gen_path[0], sizeof(s_gen_path[0]), "save:/game_a.bin");
        snprintf(s_gen_path[1], sizeof(s_gen_path[1]), "save:/game_b.bin");
        s_legacy_path[0] = '\0';
    } else {
        snprintf(s_gen_path[0], sizeof(s_gen_path[0]), "%s/saves/%s_a.bin", paths->root,
                 kj_ds_edition_name());
        snprintf(s_gen_path[1], sizeof(s_gen_path[1]), "%s/saves/%s_b.bin", paths->root,
                 kj_ds_edition_name());
        snprintf(s_legacy_path, sizeof(s_legacy_path), "%s", paths->save_file);
    }
    kj_ds_set_save_backend(&BACKEND);
    kj_log("[save] two-generation store: %s | %s", s_gen_path[0], s_gen_path[1]);
    return true;
}

void kj_savestore_exit(void)
{
    if (s_archive) {
        archiveUnmount("save");
        s_archive = false;
    }
}
