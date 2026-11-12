#ifndef KJ_DIAGNOSTICS_PERF_H
#define KJ_DIAGNOSTICS_PERF_H

#include <stdint.h>

/* Per-frame stage timing in ARM11 ticks. PUBLISH is the parent of G2D, G3D,
 * GPU_WAIT and SUBMIT: never add it to them. LOGIC excludes V-blank waits. */
enum kj_perf_stage {
    KJ_PERF_LOGIC,
    KJ_PERF_VBLANK_WAIT,
    KJ_PERF_G2D,
    KJ_PERF_G3D,
    KJ_PERF_GPU_WAIT,
    KJ_PERF_SUBMIT,
    KJ_PERF_PUBLISH,
    KJ_PERF_STAGE_COUNT
};

void kj_perf_vblank_begin(void);
void kj_perf_vblank_end(void);
void kj_perf_publish_begin(void);
void kj_perf_publish_end(void);
uint64_t kj_perf_now(void);
void kj_perf_add(enum kj_perf_stage stage, uint64_t ticks);

#endif
