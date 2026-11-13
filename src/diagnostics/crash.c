/*
 * crash.c: CPU exception reporting (3DS side).
 *
 * Coffeepot ctr/engineering/debugging.md ("Catching exceptions in your own
 * program"): each thread installs a handler with threadOnException; on a
 * fault the kernel jumps to it with the exception information and all CPU
 * registers. The handler runs on its own static stack (the fault may be a
 * stack overflow), records the registers in the log file and a separate
 * crash file, then hands over to ERRF_ExceptionHandler so the console shows
 * the usual error screen. It does not return.
 *
 * Feed the reported pc and lr to
 *   arm-none-eabi-addr2line -f -C -e build/kj/pokeheartgold-3ds.elf 0x...
 * (3dsx.ld links at 0x00100000; no adjustment needed).
 *
 * Azahar note: Azahar does not raise data aborts for unmapped guest memory;
 * it logs "Unmapped Read/Write" with the PC in azahar_log.txt and carries on.
 * Look there first when a run hangs or misbehaves in the emulator.
 */

#include "diagnostics/crash.h"

#include "diagnostics/log.h"

#include <3ds.h>
#include <stdio.h>
#include <string.h>

#define HANDLER_STACK_BYTES 0x4000
#define MAX_THREADS 8

static u8 s_stacks[MAX_THREADS][HANDLER_STACK_BYTES] __attribute__((aligned(8)));
static ERRF_ExceptionData s_data[MAX_THREADS];
static volatile u32 s_next;
static char s_crash_path[128];
static volatile u32 s_in_handler;

static const char *type_name(ERRF_ExceptionType t)
{
    switch (t) {
    case ERRF_EXCEPTION_PREFETCH_ABORT: return "prefetch abort";
    case ERRF_EXCEPTION_DATA_ABORT: return "data abort";
    case ERRF_EXCEPTION_UNDEFINED: return "undefined instruction";
    case ERRF_EXCEPTION_VFP: return "VFP exception";
    default: return "unknown exception";
    }
}

static void handler(ERRF_ExceptionInfo *excep, CpuRegisters *regs)
{
    /* A second fault (in this handler or another thread): go straight to
     * the system screen. */
    if (__atomic_fetch_add(&s_in_handler, 1, __ATOMIC_SEQ_CST) == 0) {
        char text[640];
        int n = snprintf(text, sizeof(text),
                         "[crash] %s on thread %08lx: pc %08lx lr %08lx sp %08lx cpsr %08lx "
                         "far %08lx fsr %08lx\n[crash] r0 %08lx r1 %08lx r2 %08lx r3 %08lx "
                         "r4 %08lx r5 %08lx r6 %08lx r7 %08lx\n[crash] r8 %08lx r9 %08lx "
                         "r10 %08lx r11 %08lx r12 %08lx\n",
                         type_name(excep->type), (unsigned long)(u32)(uintptr_t)getThreadLocalStorage(),
                         (unsigned long)regs->pc, (unsigned long)regs->lr, (unsigned long)regs->sp,
                         (unsigned long)regs->cpsr, (unsigned long)excep->far,
                         (unsigned long)excep->fsr, (unsigned long)regs->r[0],
                         (unsigned long)regs->r[1], (unsigned long)regs->r[2],
                         (unsigned long)regs->r[3], (unsigned long)regs->r[4],
                         (unsigned long)regs->r[5], (unsigned long)regs->r[6],
                         (unsigned long)regs->r[7], (unsigned long)regs->r[8],
                         (unsigned long)regs->r[9], (unsigned long)regs->r[10],
                         (unsigned long)regs->r[11], (unsigned long)regs->r[12]);
        /* The crash file first: it needs no lock the faulting thread could
         * hold (the log ring does). */
        if (s_crash_path[0] != '\0') {
            FILE *f = fopen(s_crash_path, "a");
            if (f != NULL) {
                fwrite(text, 1, n > 0 ? (size_t)n : 0, f);
                fclose(f);
            }
        }
        kj_log("%s", text);
        kj_log_flush_now();
    }
    ERRF_ExceptionHandler(excep, regs);
}

void kj_crash_init(const char *crash_path)
{
    if (crash_path != NULL) {
        snprintf(s_crash_path, sizeof(s_crash_path), "%s", crash_path);
    }
    kj_crash_install();
}

void kj_crash_install(void)
{
    const u32 i = __atomic_fetch_add(&s_next, 1, __ATOMIC_SEQ_CST);
    if (i >= MAX_THREADS) {
        return; /* the system screen still reports it */
    }
    threadOnException(handler, s_stacks[i] + HANDLER_STACK_BYTES, &s_data[i]);
}
