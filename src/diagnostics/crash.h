#ifndef KJ_DIAGNOSTICS_CRASH_H
#define KJ_DIAGNOSTICS_CRASH_H

/* Install the exception handler on the calling (main) thread; crash_path
 * receives the register dump (NULL: log only). */
void kj_crash_init(const char *crash_path);
/* Install it on the calling thread (each thread needs its own). */
void kj_crash_install(void);

#ifdef KJ_CRASH_TEST
/* Development-only fault in ordinary code, after the logger is active. It
 * exercises the exception handler and the crash-file write path; never in a
 * shipping build. */
void kj_crash_test_trigger(void);
#endif

#endif
