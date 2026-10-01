#ifndef SPINALVOODOO_MISTER_MEMORY_H
#define SPINALVOODOO_MISTER_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include "mister_mailbox.h"

#define MISTER_MEMORY_INIT {NULL, -1}
#define MISTER_MEMORY_BYTES UINT32_C(0x02000000)
#define MISTER_FRAME_OFFSET UINT32_C(0x00400000)
#define MISTER_RENDERER_OFFSET UINT32_C(0x01400000)
#define MISTER_TEXTURE_OFFSET UINT32_C(0x01800000)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *base;
    int lock_fd;
} mister_memory;

int mister_memory_open(mister_memory *memory);
int mister_memory_close(mister_memory *memory);
int mister_reservation_valid(const uint8_t *reg, size_t size, unsigned address_cells,
                             unsigned size_cells);

#ifdef __cplusplus
}
#endif

#endif
