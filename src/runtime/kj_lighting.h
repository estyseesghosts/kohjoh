#ifndef KJ_LIGHTING_H
#define KJ_LIGHTING_H

#include <stdint.h>

/* Immutable NORMAL-time lighting context. Direction/normal units are 20.12.
 * Colour products have already used the DS's exact (a*b+15)/31 rule. */
typedef struct kj_lighting {
    int32_t direction[4][3], half[4][3];
    uint32_t shininess[32];
    uint8_t ambient[4][3], diffuse[4][3], specular[4][3];
    uint8_t emission[3], use_shininess;
    uint32_t enabled;
} kj_lighting;

/* CPU oracle and boundary-vertex conversion, not the normal vertex path. */
static inline uint16_t kj_lighting_color(const int32_t n[3], const kj_lighting *s) {
    unsigned rgb[3] = {s->emission[0], s->emission[1], s->emission[2]};
    for (unsigned i = 0; i < 4; ++i) {
        if (!(s->enabled & (1u << i))) {
            continue;
        }
        int64_t dot = 0, half_dot = 0;
        for (unsigned c = 0; c < 3; ++c) {
            dot += (int64_t)n[c] * s->direction[i][c];
            half_dot += (int64_t)n[c] * s->half[i][c];
        }
        int d = (int)(-dot >> 12), h = (int)(-half_dot >> 12);
        unsigned diffuse = d < 0 ? 0u : d > 4096 ? 4096u : (unsigned)d;
        unsigned spec = h < 0 ? 0u : h > 4096 ? 4096u : (unsigned)h;
        spec = spec * spec >> 12;
        if (s->use_shininess) {
            unsigned index = spec * 128u >> 12;
            if (index > 127u) {
                index = 127u;
            }
            unsigned entry = s->shininess[index >> 2] >> ((index & 3u) * 8u) & 255u;
            spec = entry * 4096u / 255u;
        }
        for (unsigned c = 0; c < 3; ++c) {
            rgb[c] += s->ambient[i][c] + ((s->diffuse[i][c] * diffuse + 2048u) >> 12) +
                      ((s->specular[i][c] * spec + 2048u) >> 12);
        }
    }
    return (uint16_t)((rgb[0] > 31 ? 31 : rgb[0]) | ((rgb[1] > 31 ? 31 : rgb[1]) << 5) |
                      ((rgb[2] > 31 ? 31 : rgb[2]) << 10));
}

/* Lit vertices retain their 16-byte ABI. The three signed 14-bit transformed
 * normal components occupy rgba plus the unused high bits of mtx/pad.
 * The low three mtx bits select one of six position matrices. Alpha is
 * constant for a batch; cross-context strip vertices become literal RGBA. */
static inline void kj_lighting_unpack_normal(uint32_t rgba, uint8_t mtx, uint8_t pad,
                                             int32_t n[3]) {
    uint64_t bits = rgba | ((uint64_t)(mtx >> 3) << 32) | ((uint64_t)(pad & 31u) << 37);
    for (unsigned c = 0; c < 3; ++c) {
        uint32_t value = (uint32_t)(bits >> (14 * c)) & 16383u;
        n[c] = (int32_t)(value ^ 8192u) - 8192;
    }
}

#endif
