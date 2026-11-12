/*
 * dump.c: write the exact DS state one publish rendered from (.kjd), for the
 * offline reference renderer (tools/capture/ds_reference.py). Debug/test
 * only: synchronous SD write, triggered by a script "dump" directive.
 *
 * Layout (little endian): "KJD1", u32 frame, u32 engine_a_on_top,
 * 9 x { u32 lcdc_offset, u32 size, u8 enabled, u8 mst, u8 ofs, u8 0 },
 * u32 lines_active, then blobs each prefixed by u32 length:
 * io, pltt, oam, bg_a, bg_b, obj_a, obj_b, lcdc, lines[2][192][0x70].
 * Then the decoded 3D scene, if one was swapped: u32 present (0/1); when 1,
 * u32 swap_params, u32 viewport, u32 disp3dcnt and the blobs verts
 * (kj_g3_vertex), indices (u16), matrices (kj_g3_matrix), batches
 * (kj_g3_batch). tools/capture/g3_reference.py projects them.
 */

#include "diagnostics/dump.h"

#include "diagnostics/log.h"

#include <stdio.h>
#include <string.h>

static bool put(FILE *f, const void *p, size_t n)
{
    return fwrite(p, 1, n, f) == n;
}

static bool blob(FILE *f, const void *p, uint32_t n)
{
    static const uint8_t zeros[256];
    if (!put(f, &n, 4)) {
        return false;
    }
    if (p != NULL) {
        return put(f, p, n);
    }
    while (n > 0) {
        const uint32_t k = n > sizeof(zeros) ? (uint32_t)sizeof(zeros) : n;
        if (!put(f, zeros, k)) {
            return false;
        }
        n -= k;
    }
    return true;
}

bool kj_dump_frame(const char *path, const kj_frame_input *in)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        kj_log("[dump] cannot open %s", path);
        return false;
    }
    const kj_ds_memory *m = in->mem;
    const uint32_t frame = in->game_frame, top = in->engine_a_on_top ? 1u : 0u;
    const uint32_t lines = in->lines.active ? 1u : 0u;
    bool ok = put(f, "KJD1", 4) && put(f, &frame, 4) && put(f, &top, 4);
    for (int i = 0; i < 9 && ok; ++i) {
        const uint8_t b[4] = { in->banks[i].enabled, in->banks[i].mst, in->banks[i].ofs, 0 };
        ok = put(f, &in->banks[i].lcdc_offset, 4) && put(f, &in->banks[i].size, 4) && put(f, b, 4);
    }
    ok = ok && put(f, &lines, 4) && blob(f, m->io, (uint32_t)m->io_size) && blob(f, m->pltt, 0x800) &&
         blob(f, m->oam, 0x800) && blob(f, m->bg_vram_a, 0x80000) && blob(f, m->bg_vram_b, 0x20000) &&
         blob(f, m->obj_vram_a, 0x40000) && blob(f, m->obj_vram_b, 0x20000) &&
         blob(f, m->lcdc_vram, 0xA4000) &&
         blob(f, in->lines.active ? (const void *)in->lines.regs : NULL, 2 * 192 * KJ_LINE_REG_BYTES);
    const kj_g3_frame *g = in->g3;
    const uint32_t present = g != NULL ? 1u : 0u;
    ok = ok && put(f, &present, 4);
    if (ok && g != NULL) {
        ok = put(f, &g->swap_params, 4) && put(f, &g->viewport, 4) && put(f, &g->disp3dcnt, 4) &&
             blob(f, g->verts, g->vert_count * (uint32_t)sizeof(kj_g3_vertex)) &&
             blob(f, g->indices, g->index_count * 2u) &&
             blob(f, g->matrices, g->matrix_count * (uint32_t)sizeof(kj_g3_matrix)) &&
             blob(f, g->batches, g->batch_count * (uint32_t)sizeof(kj_g3_batch));
    }
    if (fclose(f) != 0) {
        ok = false;
    }
    kj_log("[dump] frame %lu -> %s %s", (unsigned long)frame, path, ok ? "ok" : "FAILED");
    return ok;
}
