/*
 * log.c: the program's log (3DS side).
 *
 * Lines go into a fixed in-memory ring under a lock and are written to the
 * SD card by a low-priority thread, never synchronously from a frame
 * (kohjoh.md section 4.2: logs are never synchronous in a frame). A full ring
 * drops lines and counts them; the count is written with the next flush.
 */

#include "diagnostics/log.h"

#include "runtime/kj_bridge.h"

#include <3ds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define RING_BYTES (64 * 1024)
#define LINE_MAX_BYTES 512

static LightLock s_lock;
static LightLock s_flush_lock; /* exactly one drain+write operation at a time */
static LightEvent s_wake;
static char s_ring[RING_BYTES];
static size_t s_head; /* write position */
static size_t s_tail; /* flush position */
static uint32_t s_dropped;
static FILE *s_file;
static Thread s_thread;
/* Stop flag shared with the writer thread (atomic publication, not
 * volatile). When threadCreate fails the ring stays usable and explicit
 * flush/shutdown still write; only the background writer is absent. */
static bool s_running;
static bool s_ready;
static u64 s_start_tick;

static size_t ring_used(void)
{
    return (s_head - s_tail) & (RING_BYTES - 1);
}

static void append(const char *text, size_t len)
{
    if (ring_used() + len + 1 >= RING_BYTES) {
        ++s_dropped;
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        s_ring[s_head] = text[i];
        s_head = (s_head + 1) & (RING_BYTES - 1);
    }
}

/* Drain the producer ring under its lock, then write to the file under the
 * flush lock. The two locks are separate on purpose: the SD write must not
 * block producers, and kj_log_flush_now() from another thread must not
 * interleave chunks with the writer thread. */
static void flush_ring_locked(void)
{
    static char chunk[RING_BYTES];
    size_t n = 0;
    uint32_t dropped;

    LightLock_Lock(&s_lock);
    while (s_tail != s_head) {
        chunk[n++] = s_ring[s_tail];
        s_tail = (s_tail + 1) & (RING_BYTES - 1);
    }
    dropped = s_dropped;
    s_dropped = 0;
    LightLock_Unlock(&s_lock);

    if (s_file == NULL) {
        return;
    }
    if (n > 0) {
        fwrite(chunk, 1, n, s_file);
    }
    if (dropped > 0) {
        fprintf(s_file, "[log] %lu lines dropped (ring full)\n", (unsigned long)dropped);
    }
    fflush(s_file);
}

static void flush_ring(void)
{
    LightLock_Lock(&s_flush_lock);
    flush_ring_locked();
    LightLock_Unlock(&s_flush_lock);
}

static void log_thread(void *arg)
{
    (void)arg;
    while (__atomic_load_n(&s_running, __ATOMIC_ACQUIRE)) {
        LightEvent_WaitTimeout(&s_wake, 500000000LL);
        flush_ring();
    }
    flush_ring();
}

void kj_log_init(const char *path)
{
    LightLock_Init(&s_lock);
    LightLock_Init(&s_flush_lock);
    LightEvent_Init(&s_wake, RESET_ONESHOT);
    s_start_tick = svcGetSystemTick();
    s_file = path != NULL ? fopen(path, "a") : NULL;
    if (s_file != NULL) {
        fputs("\n--- new run ---\n", s_file);
        fflush(s_file);
    }
    s_ready = true;
    __atomic_store_n(&s_running, true, __ATOMIC_RELEASE);
    /* Lowest application priority: it runs while the game waits for V-blank. */
    s_thread = threadCreate(log_thread, NULL, 16 * 1024, 0x3F, 0, false);
    if (s_thread == NULL) {
        /* Degraded but defined: the ring stays usable and explicit flush or
         * shutdown still writes. Never fall back to synchronous SD writes
         * from the frame thread. */
        __atomic_store_n(&s_running, false, __ATOMIC_RELEASE);
        kj_log("[log] background writer unavailable; lines are written on explicit flush");
    }
}

void kj_log_shutdown(void)
{
    if (s_thread != NULL) {
        __atomic_store_n(&s_running, false, __ATOMIC_RELEASE);
        LightEvent_Signal(&s_wake);
        threadJoin(s_thread, U64_MAX);
        threadFree(s_thread);
        s_thread = NULL;
    } else {
        __atomic_store_n(&s_running, false, __ATOMIC_RELEASE);
    }
    /* Flush and close under the flush lock so a concurrent
     * kj_log_flush_now() cannot use a closed file. */
    LightLock_Lock(&s_flush_lock);
    flush_ring_locked();
    if (s_file != NULL) {
        fclose(s_file);
        s_file = NULL;
    }
    LightLock_Unlock(&s_flush_lock);
}

void kj_log_flush_now(void)
{
    flush_ring();
}

void kj_logv(const char *fmt, va_list args)
{
    char line[LINE_MAX_BYTES];
    const u64 ms = (svcGetSystemTick() - s_start_tick) / (SYSCLOCK_ARM11 / 1000);
    int n = snprintf(line, sizeof(line), "%6lu.%03lu ", (unsigned long)(ms / 1000),
                     (unsigned long)(ms % 1000));
    if (n < 0) {
        return;
    }
    int m = vsnprintf(line + n, sizeof(line) - (size_t)n - 1, fmt, args);
    if (m < 0) {
        return;
    }
    size_t len = (size_t)n + (size_t)m;
    if (len > sizeof(line) - 2) {
        len = sizeof(line) - 2;
    }
    line[len++] = '\n';
    if (!s_ready) {
        return;
    }
    LightLock_Lock(&s_lock);
    append(line, len);
    LightLock_Unlock(&s_lock);
}

void kj_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    kj_logv(fmt, args);
    va_end(args);
}

/* The compat layer's logger (strong; overrides its weak stderr default).
 * Declared here because the NitroSDK-side header cannot be included from the
 * libctru build. */
void NitroPort_Log(const char *fmt, ...);

void NitroPort_Log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    kj_logv(fmt, args);
    va_end(args);
}

void kj_ctr_log(const char *line)
{
    kj_log("%s", line);
}
