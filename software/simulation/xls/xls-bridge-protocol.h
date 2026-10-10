#ifndef XLS_BRIDGE_PROTOCOL_H
#define XLS_BRIDGE_PROTOCOL_H

#include <stdint.h>

#define XLS_BRIDGE_MAGIC 0x584c5342u

enum {
    XLS_BRIDGE_WRITE = 1,
    XLS_BRIDGE_READ = 2,
    XLS_BRIDGE_READ_FB = 3,
    XLS_BRIDGE_WRITE_FB = 4,
    XLS_BRIDGE_READ_TEX = 5,
    XLS_BRIDGE_WRITE_TEX = 6,
    XLS_BRIDGE_WRITE_BUS_BULK = 7,
    XLS_BRIDGE_READ_FRONTBUFFER = 8,
    XLS_BRIDGE_IDLE = 9,
    XLS_BRIDGE_TICK = 10,
    XLS_BRIDGE_SET_SWAP = 11,
    XLS_BRIDGE_GET_SWAP = 12,
    XLS_BRIDGE_CLOSE = 13
};

typedef struct {
    uint32_t magic;
    uint32_t operation;
    uint32_t address;
    uint32_t data;
    uint32_t count;
    uint32_t mask;
} xls_bridge_request_t;

typedef struct {
    uint32_t magic;
    uint32_t status;
    uint32_t data;
    uint32_t count;
    uint64_t ticks;
} xls_bridge_response_t;

#endif
