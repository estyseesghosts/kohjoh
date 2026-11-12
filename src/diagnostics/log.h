#ifndef KJ_DIAGNOSTICS_LOG_H
#define KJ_DIAGNOSTICS_LOG_H

#include <stdarg.h>

/* Open the log file (NULL: memory only) and start the flush thread. */
void kj_log_init(const char *path);
/* Stop the flush thread and write everything still buffered. */
void kj_log_shutdown(void);
/* Write the ring now, on the calling thread (crash paths, exit). */
void kj_log_flush_now(void);

void kj_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void kj_logv(const char *fmt, va_list args);

#endif
