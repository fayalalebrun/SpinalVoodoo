/* Stub: platform utilities */
#ifndef _86BOX_PLAT_H
#define _86BOX_PLAT_H

#include <stdint.h>

static inline uint64_t plat_timer_read(void) { return 0; }

/* JIT codegen stubs — codegen functions compile but are never called */
static inline void *plat_mmap(size_t size, uint8_t exec)
    { (void)exec; return malloc(size); }
static inline void plat_munmap(void *ptr, size_t size)
    { (void)size; free(ptr); }

/* GCC attribute to mark unused parameters */
#define UNUSED(arg) __attribute__((unused)) arg

#endif /* _86BOX_PLAT_H */
