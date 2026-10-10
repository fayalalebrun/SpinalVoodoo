#include "glide_guest.h"
#include "mister_runtime.h"
#include "../../simulation/xls/harness/sim_harness.h"
#include "glide.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t buffers[2] = {UINT32_MAX, UINT32_MAX};
static uint32_t extents[2], origins[2], row_bytes[2];
static int resume_front;
static volatile uint8_t *write_memory;
static uint32_t write_stride;
static mister_command last_write;
void mister_glide_write_masked(uint32_t, uint32_t, uint32_t);
void _grX11PresenterSwap(void);

static void invalid(void) {
    fputs("MiSTer: invalid or unlocked guest LFB access\n", stderr);
    exit(EXIT_FAILURE);
}

static void flush_direct(void) {
    if (last_write.mask) {
        mister_glide_write_masked(last_write.address, last_write.value, last_write.mask);
        last_write.mask = 0;
    }
}

static int map_write(uint32_t width, uint32_t height) {
    uint32_t status = sim_read(0), init1 = sim_read(0x214), init2 = sim_read(0x218);
    uint32_t mode = sim_read(0x114), y_origin = sim_read(0x21c) >> 22;
    uint32_t tiles = (init1 >> 4) & 15;
    uint32_t stride = (tiles ? tiles : 10) * 128, offset = ((init2 >> 11) & 511) * 4096;
    uint32_t selected = (mode & 15) == 15 ? 2 : (mode >> 4) & 3;
    if ((status & 0xf0000000) || selected > 2 || stride < width * 2 ||
        offset < height * stride || offset * 3 > MISTER_RENDERER_BYTES ||
        ((mode & 8192) && y_origin != height - 1)) return 0;
    uint32_t base = selected == 2 ? 2 * offset : (((status >> 10) & 1) ^ selected) * offset;
    sim_read(0x400000);
    if (mister_runtime_map_renderer(base, height * stride, &write_memory)) return 0;
    write_stride = stride;
    return 1;
}

void mister_glide_lfb_reset(void) {
    buffers[0] = buffers[1] = UINT32_MAX;
    extents[0] = extents[1] = 0;
    resume_front = 0;
    write_memory = NULL;
    last_write.mask = 0;
}

int mister_glide_lfb_lock(uint32_t type, uint32_t buffer, uint32_t mode,
                         uint32_t origin, uint32_t pipeline, mister_guest_lfb_info *info) {
    unsigned access = type & 1;
    if (!info || info->size != sizeof(*info) || (type & ~UINT32_C(0x11)) ||
        buffer > 2 || buffers[access] != UINT32_MAX || pipeline != 0 ||
        (origin > 1 && origin != GR_ORIGIN_ANY) ||
        (mode != GR_LFBWRITEMODE_565 && mode != GR_LFBWRITEMODE_ZA16 && mode != GR_LFBWRITEMODE_ANY)) return 0;
    uint32_t height = grSstScreenHeight(), width = grSstScreenWidth();
    if (!height || height > 1024 || !width || width > 1024) return 0;
    if (origin == GR_ORIGIN_ANY) origin = GR_ORIGIN_UPPER_LEFT;
    if (buffers[access ^ 1] != UINT32_MAX && origins[access ^ 1] != origin) return 0;
    GrLfbInfo_t host = {.size = sizeof(host)};
    sim_idle_wait();
    if (!grLfbLock(type & ~GR_LFB_NOIDLE, buffer, mode, origin, pipeline, &host)) return 0;
    if (host.strideInBytes != 2048) {
        if (!grLfbUnlock(access, buffer)) invalid();
        sim_idle_wait();
        return 0;
    }
    if (access && !map_write(width, height)) {
        if (!grLfbUnlock(access, buffer)) invalid();
        sim_idle_wait();
        return 0;
    }
    if (access && buffer == GR_BUFFER_FRONTBUFFER) {
        if (mister_runtime_stop_presenting()) { perror("MiSTer front LFB retirement"); exit(EXIT_FAILURE); }
        resume_front = 1;
    }
    sim_idle_wait();
    buffers[access] = buffer;
    extents[access] = host.strideInBytes * height;
    origins[access] = origin;
    row_bytes[access] = width * 2;
    info->pointer = 0;
    info->stride = host.strideInBytes;
    info->mode = host.writeMode;
    info->origin = host.origin;
    return 1;
}

int mister_glide_lfb_unlock(uint32_t type, uint32_t buffer) {
    unsigned access = type & 1;
    if ((type & ~UINT32_C(0x11)) || buffer > 2 || buffers[access] != buffer) return 0;
    flush_direct();
    sim_idle_wait();
    if (!grLfbUnlock(access, buffer)) return 0;
    sim_idle_wait();
    buffers[access] = UINT32_MAX;
    extents[access] = 0;
    if (access) write_memory = NULL;
    if (access && resume_front) { resume_front = 0; _grX11PresenterSwap(); }
    return 1;
}

static void validate(unsigned access, uint32_t buffer, uint32_t offset, uint32_t size) {
    uint32_t column = offset & 2047;
    if (buffer > 2 || buffers[access] != buffer || (size != 1 && size != 2 && size != 4) ||
        offset >= extents[access] || size > extents[access] - offset ||
        column >= row_bytes[access] || size > row_bytes[access] - column) {
        fprintf(stderr, "MiSTer: guest LFB access=%u buffer=%u locked=%u offset=0x%x size=%u extent=%u row_bytes=%u\n",
                access, buffer, buffers[access], offset, size, extents[access], row_bytes[access]);
        invalid();
    }
}

uint32_t mister_glide_lfb_read(uint32_t buffer, uint32_t offset, uint32_t size) {
    validate(0, buffer, offset, size);
    flush_direct();
    uint32_t value = 0, done = 0;
    while (done < size) {
        unsigned shift = offset & 3, count = size - done;
        if (count > 4 - shift) count = 4 - shift;
        uint32_t mask = UINT32_MAX >> ((4 - count) * 8);
        value |= ((sim_read(0x400000 + (offset & ~3u)) >> (shift * 8)) & mask) << (done * 8);
        offset += count;
        done += count;
    }
    return value;
}

void mister_glide_lfb_write(uint32_t buffer, uint32_t offset, uint32_t value, uint32_t size) {
    validate(1, buffer, offset, size);
    if (!write_memory) invalid();
    uint32_t row = offset >> 11;
    if (origins[1]) row = (extents[1] >> 11) - 1 - row;
    volatile uint8_t *destination = write_memory + row * write_stride + (offset & 2047);
    if (size == 4 && !(offset & 3)) *(volatile uint32_t *)destination = value;
    else if (size == 2 && !(offset & 1)) *(volatile uint16_t *)destination = value;
    else for (unsigned i = 0; i < size; ++i) destination[i] = value >> (8 * i);
    uint32_t done = 0;
    while (done < size) {
        unsigned shift = offset & 3, count = size - done;
        if (count > 4 - shift) count = 4 - shift;
        uint32_t mask = ((1u << count) - 1) << shift;
        last_write = (mister_command){1, 0x400000 + (offset & ~3u),
                                      (value >> (done * 8)) << (shift * 8), mask};
        offset += count;
        done += count;
    }
}
