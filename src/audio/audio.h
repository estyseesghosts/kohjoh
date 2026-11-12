#ifndef KJ_AUDIO_AUDIO_H
#define KJ_AUDIO_AUDIO_H

#include <stdbool.h>

/* Start NDSP output fed by the DS sound chip model (compat port_snd.c). */
bool kj_audio_init(void);
/* Per-V-blank bookkeeping on the game thread (cheap). */
void kj_audio_service(void);
void kj_audio_exit(void);

#endif
