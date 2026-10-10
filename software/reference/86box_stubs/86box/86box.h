/*
 * Minimal stub for 86Box framework headers.
 * Satisfies includes from vid_voodoo_render.c / vid_voodoo_texture.c
 * when compiling standalone for the trace_test reference model.
 */
#ifndef _86BOX_86BOX_H
#define _86BOX_86BOX_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Atomics — single-threaded ref model, plain volatile suffices */
#define ATOMIC_INT  volatile int
#define ATOMIC_UINT volatile uint32_t

/* Logging — no-op */
static inline void pclog_ex(const char *fmt, ...) { (void)fmt; }
static inline void pclog(const char *fmt, ...) { (void)fmt; }

/* Global cycle counter used by trace code. */
extern uint64_t tsc;
extern int voodoo_trace_max_frames;

/* Fatal error */
static inline void fatal(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    abort();
}

#ifndef ABS
#define ABS(x) ((x) > 0 ? (x) : -(x))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Branch hints */
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

/* Switch fallthrough annotation */
#ifndef fallthrough
#define fallthrough __attribute__((fallthrough))
#endif

/* Force no codegen — we don't want JIT in the ref model */
#define NO_CODEGEN

#endif /* _86BOX_86BOX_H */
