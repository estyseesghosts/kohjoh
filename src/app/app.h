#ifndef KJ_APP_APP_H
#define KJ_APP_APP_H

#include "build_id.h" /* generated: HGSS_BUILD_ID, HGSS_BUILD_SHA, HGSS_BUILD_DIRTY */

#define KJ_BUILD_ID HGSS_BUILD_ID

#include <stdint.h>

/* Physical V-blanks the game is behind its schedule (0: on time). */
uint32_t kj_pacing_behind(void);
/* V-blanks that started late, and resyncs, since the last call. */
void kj_pacing_stats(uint32_t *late, uint32_t *resyncs);

#endif
