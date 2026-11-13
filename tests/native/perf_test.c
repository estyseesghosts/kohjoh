#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "../../src/diagnostics/perf.c"
static uint64_t tick;
static uint32_t refresh;
uint64_t svcGetSystemTick(void) { return tick; }
uint32_t C3D_FrameCounter(int n) { (void)n; return refresh; }
void kj_log(const char *fmt, ...) { (void)fmt; }
int main(void) {
    tick=1; kj_perf_vblank_end();
    tick=101; kj_perf_publish_begin(); kj_perf_present_skipped(1);
    assert(s_game_n==1 && s_game_logic[0]==100 && s_hitches==0);
    tick=301; kj_perf_publish_begin();
    kj_perf_add(KJ_PERF_G2D,90); kj_perf_add(KJ_PERF_G3D,60);
    tick=401; refresh=2; kj_perf_publish_end(2);
    assert(s_game_n==2 && s_game_logic[1]==200);
    assert(s_samples[KJ_PERF_LOGIC][0]==200);
    assert(s_samples[KJ_PERF_G2D][0]==30);
    tick=501; kj_perf_publish_begin();
    kj_perf_add(KJ_PERF_G2D,80); kj_perf_add(KJ_PERF_G3D,50);
    tick=601; refresh=6; kj_perf_publish_end(3);
    assert(s_hitches==1 && s_hitch_g2d[0]==30 && s_hitch_g3d[0]==50);
    puts("PASS: skipped game CPU samples, display hitch count, exclusive hitch timing");
}
