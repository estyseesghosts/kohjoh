#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include SOURCE
unsigned char NitroPort_IoReg[0x1000];
#ifndef HGSS_PROFILE_OFF
static uint64_t mock_tick;
uint64_t svcGetSystemTick(void) {
    return ++mock_tick;
}
#endif
void NitroPort_Log(const char *fmt, ...) {
    assert(strstr(fmt, "MISMATCH") == NULL);
}
static uint32_t rng = 76543;
static uint32_t next(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}
static uint64_t hash = 1469598103934665603ull;
static void digest(const void *p, size_t n) {
    const unsigned char *s = p;
    while (n--)
        hash = (hash ^ *s++) * 1099511628211ull;
}
int main(void) {
    kj_g3_storage st;
    for (int i = 0; i < 2; i++) {
        st.verts[i] = calloc(KJ_G3_MAX_VERTS, sizeof(kj_g3_vertex));
        st.indices[i] = calloc(KJ_G3_MAX_INDICES, 2);
    }
    kj_g3_init(&st);
    uint32_t lists[16][128];
    size_t sizes[16];
    for (int l = 0; l < 16; l++) {
        size_t n = 0;
        lists[l][n++] = 0x40;
        lists[l][n++] = l % 4;
        for (int k = 0; k < 10; k++) {
            lists[l][n++] = (k & 1) ? 0x23222120 : 0x23232220;
            lists[l][n++] = next() & 32767;
            lists[l][n++] = next();
            for (int j = 0; j < ((k & 1) ? 3 : 4); j++)
                lists[l][n++] = next();
        }
        lists[l][n++] = 0x41;
        sizes[l] = n;
    }
    for (int f = 0; f < 2000; f++) {
        NitroPort_GxDirectWrite(0x29, 0x001f00c0u | ((f / 64) & 1 ? 15u : 0u));
        NitroPort_GxDirectWrite(0x2a, (uint32_t)((f / 128) % 4) << 30);
        NitroPort_GxDirectWrite(0x30, 0x12345678);
        NitroPort_GxDirectWrite(0x31, (f / 256) & 1 ? 0x9234abcd : 0x1234abcd);
        for (int l = 0; l < 16; l++) {
            if (f % 7 == 0)
                lists[l][4] ^= 0x1234;
            if (l % 3 == 0) {
                size_t n = 0;
                while (n < sizes[l]) {
                    size_t c = 1 + next() % 11;
                    if (c > sizes[l] - n)
                        c = sizes[l] - n;
                    NitroPort_GxFifoWords(lists[l] + n, c);
                    n += c;
                }
            } else
                NitroPort_GxFifoWords(lists[l], sizes[l]);
        }
        NitroPort_GxDirectWrite(0x50, 0);
        const kj_g3_frame *g = kj_g3_take_frame();
        assert(g != NULL && g->vert_count != 0);
        digest(g->verts, g->vert_count * sizeof(*g->verts));
        digest(g->indices, g->index_count * 2);
        digest(g->matrices, g->matrix_count * sizeof(*g->matrices));
        digest(g->batches, g->batch_count * sizeof(*g->batches));
        digest(NitroPort_IoReg, sizeof(NitroPort_IoReg));
    }
#ifndef HGSS_PROFILE_OFF
    uint64_t total = 0, copied = 0;
    uint32_t samples = 0, normals = 0;
    for (unsigned i = 0; i < DL_PROFILE_SLOTS; ++i) {
        const dl_profile *p = &s_dl_profile[i];
        total += p->total;
        samples += p->samples;
        copied += p->copy_bytes;
        normals += p->normals;
        assert(p->copy <= p->replay && p->replay <= p->total);
        assert(p->decode <= p->total && p->shade_misses <= p->normals);
    }
    assert(samples + s_dl_profile_overflow == s_coarse_n);
    assert(total + s_dl_profile_omitted_ticks == s_coarse_total);
    assert(copied != 0 && normals != 0);
    dl_profile_report();
    for (unsigned i = 0; i < DL_PROFILE_SLOTS; ++i) {
        assert(s_dl_profile[i].samples == 0);
    }
    /* Full tables report omission explicitly and never replace a tracked key. */
    for (unsigned i = 0; i < DL_PROFILE_SLOTS; ++i) {
        s_dl_profile[i].samples = 1;
        s_dl_profile[i].source = NULL;
    }
    dl_profile_add(lists[0], sizes[0], 0, 1, 0, 0, 0);
    assert(s_dl_profile_overflow == 1);
    dl_profile_report();
    /* A known rejection must survive report resets and fast never bypasses. */
    s_in_begin = false;
    s_fifo_need = 0;
    dl_entry *entry = dl_slot(lists[1], (uint32_t)sizes[1]);
    assert(entry != NULL);
    dl_free(entry);
    entry->ptr = entry->words = lists[1];
    entry->count = (uint32_t)sizes[1];
    entry->never = true;
    entry->reject_mask = 8;
    s_stats.swaps = 1;
    s_sample_reject = s_sample_rechecks = 0;
    fifo_words_impl(lists[1], sizes[1]);
    assert(s_sample_reject == 8 && s_sample_rechecks == 0);
    s_in_begin = false;
    s_stats.swaps = 64;
    fifo_words_impl(lists[1], sizes[1]);
    assert(s_sample_reject == 8 && s_sample_rechecks == 1);
#endif
    printf("%016llx\n", (unsigned long long)hash);
}
