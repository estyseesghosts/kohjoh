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
 *                         (after its last input event the script releases
 *                         the pad to live input)
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
#define KJ_DEBUG_NO_JOIN 0x20u       /* issue #13: no batch joins at all */
#define KJ_DEBUG_NO_JOIN_LIT 0x40u   /* issue #13: no joins for lit batches */
#define KJ_DEBUG_ART_KEYS 0x80u      /* issue #14: log wide-art keys with no pictures */
/* issue #21 A/B switches: each widescreen stage off on its own */
#define KJ_DEBUG_NO_WIDEART 0x100u   /* no wide-art bands */
#define KJ_DEBUG_NO_LAYOUT 0x200u    /* no layout rows (centred DS columns) */
#define KJ_DEBUG_NO_SIDE_FILL 0x400u /* side columns black, not the backdrop */
#define KJ_DEBUG_NO_VIS_CULL 0x800u
#define KJ_DEBUG_NO_BOX_CULL 0x1000u /* development-only BOX_TEST bypass */
#define KJ_DEBUG_PROPVIS_BYPASS 0x2000u /* map-prop render BOX_TEST only */
#define KJ_DEBUG_PROPVIS_LOG 0x4000u    /* log stable prop visibility decisions */
#define KJ_DEBUG_BILLBOARD_LOG 0x8000u  /* log billboard identity/position/scale */  /* offscreen-object culling only, not face culling */
uint32_t kj_script_debug(void);
/* V-blank at which to close (0: never). */
uint32_t kj_script_end(void);

#endif
