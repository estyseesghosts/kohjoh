#ifndef KJ_DIAGNOSTICS_CRASH_H
#define KJ_DIAGNOSTICS_CRASH_H

/* Install the exception handler on the calling (main) thread; crash_path
 * receives the register dump (NULL: log only). */
void kj_crash_init(const char *crash_path);
/* Install it on the calling thread (each thread needs its own). */
void kj_crash_install(void);

#endif
