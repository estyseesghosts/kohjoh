#ifndef KJ_APP_APP_H
#define KJ_APP_APP_H

#include "build_id.h" /* generated: HGSS_BUILD_ID, HGSS_BUILD_SHA, HGSS_BUILD_DIRTY */

#define KJ_BUILD_ID HGSS_BUILD_ID

#include <stdint.h>

/* Weak hook called at the start of every compat-created game thread; the
 * application overrides it to install its exception handler. Declared here
 * because the compat platform header carries NitroSDK types this side must
 * not include. */
void NitroPort_PlatformThreadStarted(void);

/* Physical V-blanks the game is behind its schedule (0: on time). */
uint32_t kj_pacing_behind(void);
/* V-blanks that started late, and resyncs, since the last call. */
void kj_pacing_stats(uint32_t *late, uint32_t *resyncs);

#endif
