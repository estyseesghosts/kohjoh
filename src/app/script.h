#ifndef KJ_APP_SCRIPT_H
#define KJ_APP_SCRIPT_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Scripted input and capture directives (test builds and Azahar runs).
 * File: sdmc:/3ds/kohjoh/script.txt. Lines:
 *   every: N              each press lasts N V-blanks (default 10)
 *   <vblank> KEYS...      press KEYS (A B X Y L R START SELECT UP DOWN LEFT RIGHT,
 *                         or TOUCH:x,y in DS coordinates) at that V-blank; "-" = none
 *   <from>-<to> KEYS...   hold KEYS from V-blank <from> up to (not including) <to>
 *   freeze <frame> <sec>  after publishing game frame <frame>, hold it on
 *                         screen for <sec> seconds without running the game
 *   dump <frame>          write the DS state the renderer read for game frame
 *                         <frame> to sdmc:/3ds/kohjoh/dumps/f<frame>.kjd
 *   end: N                close at V-blank N
 *   debug: FLAGS          renderer experiment switches (KJ_DEBUG_* below)
 * Frame-indexed scripts make Azahar captures reproducible; V-blank indexing
 * matches hgss-3ds's tests/inputs format.
 */

void kj_script_load(const char *path);
bool kj_script_active(void);
/* Keys (DS PAD bits) and touch for V-blank `vblank`; returns false if the
 * script has nothing to say (live input is used). */
bool kj_script_input(uint32_t vblank, uint16_t *keys, bool *touch, uint16_t *tx, uint16_t *ty);
/* Seconds to freeze after publishing game frame `frame` (0: none). */
uint32_t kj_script_freeze(uint32_t frame);
bool kj_script_dump(uint32_t frame);
/* Renderer experiment switches for A/B runs without a rebuild. */
#define KJ_DEBUG_NO_CULL 0x1u
#define KJ_DEBUG_NO_DEPTH 0x2u
#define KJ_DEBUG_NO_ALPHA_TEST 0x4u
#define KJ_DEBUG_NO_BLEND 0x8u
#define KJ_DEBUG_NO_TEXTURE 0x10u
uint32_t kj_script_debug(void);
/* V-blank at which to close (0: never). */
uint32_t kj_script_end(void);

#endif
