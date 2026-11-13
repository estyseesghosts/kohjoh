#ifndef KJ_DIAGNOSTICS_PERF_H
#define KJ_DIAGNOSTICS_PERF_H

#include <stdint.h>

/* Per-frame stage timing in ARM11 ticks. PUBLISH is the parent of G2D, G3D,
 * GPU_WAIT and SUBMIT: never add it to them. G2D is reported exclusive of
 * G3D (the 3D draw is nested in the screen draw). LOGIC excludes V-blank
 * waits. */
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

/* Per-frame event counts, reported with each logged hitch. */
enum kj_perf_count {
    KJ_COUNT_TILE_CONV,  /* atlas tile conversions */
    KJ_COUNT_SPANS,      /* 2D spans drawn */
    KJ_COUNT_QUADS,      /* 2D quads built */
    KJ_COUNT_TEX_UPLOAD, /* 3D texture conversions */
    KJ_COUNT_TEX_BYTES,  /* 3D texture bytes compared */
    KJ_COUNT_COUNT
};
extern uint32_t kj_perf_counts[KJ_COUNT_COUNT];
#define kj_perf_count(c, n) (kj_perf_counts[(c)] += (uint32_t)(n))

void kj_perf_vblank_begin(void);
void kj_perf_vblank_end(void);
void kj_perf_publish_begin(void);
/* game_frame: the published game frame (for locating hitches); call after
 * the frame is submitted. */
void kj_perf_publish_end(uint32_t game_frame);
/* A publish the renderer skipped (counted as a hitch). */
void kj_perf_present_skipped(uint32_t game_frame);
uint64_t kj_perf_now(void);
void kj_perf_add(enum kj_perf_stage stage, uint64_t ticks);

#endif
