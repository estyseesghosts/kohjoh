/* Compare the live predicate with the original serialized-key contract. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include SOURCE

unsigned char NitroPort_IoReg[0x1000];
void NitroPort_Log(const char *fmt, ...) { (void)fmt; }
static uint32_t rng = 0x913297u;

static uint32_t next(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static void random_bytes(void *ptr, size_t count)
{
    unsigned char *bytes = ptr;
    for (size_t i = 0; i < count; ++i) {
        bytes[i] = (unsigned char)next();
    }
}

int main(void)
{
    for (unsigned trial = 0; trial < 20000; ++trial) {
        s_attr_pending = next();
        s_attr = next();
        s_tex_param = next();
        s_tex_pltt = next();
        s_viewport = next();
        s_mtx_mode = next() & 3u;
        s_color = next();
        random_bytes(s_coord, sizeof(s_coord));
        random_bytes(s_texcoord, sizeof(s_texcoord));
        random_bytes(s_texcoord_raw, sizeof(s_texcoord_raw));
        random_bytes(&s_tex, sizeof(s_tex));
        random_bytes(&s_vec, sizeof(s_vec));
        random_bytes(s_light_dir, sizeof(s_light_dir));
        random_bytes(s_light_half, sizeof(s_light_half));
        random_bytes(s_light_color, sizeof(s_light_color));
        random_bytes(s_shininess, sizeof(s_shininess));
        s_diffuse = next();
        s_ambient = next();
        s_specular = next();
        s_emission = next();
        s_use_shininess = (trial & 1u) != 0;
        const bool lit = (trial & 2u) != 0;
        const bool texgen = (trial & 4u) != 0;
        const uint32_t reads = next() & 15u;
        dl_key stored, live;
        dl_make_key(&stored, lit, texgen, reads);
        assert(dl_key_matches(&stored, lit, texgen, reads));
        /* Perturb every byte of the stored snapshot in turn, including
         * unused/padding bytes. Unused bytes are zero in the old key. */
        for (size_t i = 0; i < dl_key_size(lit, texgen, reads); ++i) {
            unsigned char *bytes = (unsigned char *)&stored;
            bytes[i] ^= 0x80u;
            dl_make_key(&live, lit, texgen, reads);
            /* The direct predicate intentionally ignores unused fields.
             * Compare mutations only in fields consumed by this lookup. */
            dl_key canonical = stored;
            if (!(reads & DL_READS_COLOR)) {
                canonical.color = 0;
            }
            if (!(reads & DL_READS_COORD)) {
                canonical.coord01 = canonical.coord2 = 0;
            }
            if (!(reads & DL_READS_TEXCOORD)) {
                memset(canonical.texcoord, 0, sizeof(canonical.texcoord));
                memset(canonical.texcoord_raw, 0, sizeof(canonical.texcoord_raw));
            }
            if (!texgen && !(reads & DL_READS_TEXM)) {
                memset(canonical.texm, 0, sizeof(canonical.texm));
            }
            assert(dl_key_matches(&stored, lit, texgen, reads) ==
                   (memcmp(&canonical, &live, dl_key_size(lit, texgen, reads)) == 0));
            bytes[i] ^= 0x80u;
        }
    }
    puts("PASS: 20k live cache keys, every consumed byte, lit/texgen/shininess dependencies");
    return 0;
}
