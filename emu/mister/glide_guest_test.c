#include "glide_guest.h"
#include "glide.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static unsigned locks, unlocks, fences, stops, presents, bus_writes;
static uint32_t lfb_mode, map_offset;
static uint32_t fb_status, fb_stride = 10 << 4, fb_offset = 150 << 11, fb_origin = 479u << 22;
static int map_error;
static unsigned height = 480, width = 640, stride = 2048;
static int lock_error, unlock_error, stop_error;
static uint32_t ram[2048 * 480 / 4];

FxU32 grSstScreenHeight(void) { return height; }
FxU32 grSstScreenWidth(void) { return width; }
FxBool grLfbLock(GrLock_t type, GrBuffer_t buffer, GrLfbWriteMode_t mode,
                 GrOriginLocation_t origin, FxBool pipeline, GrLfbInfo_t *info) {
    assert(type <= 1 && buffer <= 2 && !pipeline);
    ++locks;
    if (lock_error) return FXFALSE;
    info->strideInBytes = stride;
    info->writeMode = mode == GR_LFBWRITEMODE_ANY ? GR_LFBWRITEMODE_565 : mode;
    info->origin = origin;
    lfb_mode = (lfb_mode & ~0x2000u) | (origin << 13);
    if (type) lfb_mode = (lfb_mode & ~0x3fu) | (buffer << 4) | (info->writeMode & 15);
    else lfb_mode = (lfb_mode & ~0xc0u) | (buffer << 6);
    return FXTRUE;
}
FxBool grLfbUnlock(GrLock_t type, GrBuffer_t buffer) {
    assert(type <= 1 && buffer <= 2);
    ++unlocks;
    return !unlock_error;
}
uint32_t sim_idle_wait(void) { ++fences; return 0; }
int mister_runtime_stop_presenting(void) { ++stops; errno = EIO; return stop_error ? -1 : 0; }
void _grX11PresenterSwap(void) { ++presents; }
int mister_runtime_map_renderer(uint32_t offset, size_t bytes, volatile uint8_t **base) {
    assert(bytes == 480 * 1280);
    map_offset = offset;
    if (map_error) { errno = EIO; return -1; }
    *base = (volatile uint8_t *)ram;
    return 0;
}
uint32_t sim_read(uint32_t address) {
    if (address == 0) return fb_status;
    if (address == 0x114) return lfb_mode;
    if (address == 0x214) return fb_stride;
    if (address == 0x218) return fb_offset;
    if (address == 0x21c) return fb_origin;
    assert(!(address & 3) && address >= 0x400000 && address < 0x400000 + sizeof(ram));
    uint32_t offset = address - 0x400000, row = offset >> 11;
    if (lfb_mode & 8192) row = 479 - row;
    return ram[(row * 1280 + (offset & 2047)) / 4];
}
void mister_glide_write_masked(uint32_t address, uint32_t value, uint32_t mask) {
    ++bus_writes;
    assert(!(address & 3) && address >= 0x400000 && address < 0x400000 + sizeof(ram));
    uint32_t offset = address - 0x400000, row = offset >> 11;
    if (lfb_mode & 8192) row = 479 - row;
    uint32_t *word = &ram[(row * 1280 + (offset & 2047)) / 4];
    for (unsigned i = 0; i < 4; ++i)
        if (mask & (1u << i)) *word = (*word & ~(255u << (8 * i))) | (value & (255u << (8 * i)));
}

int main(void) {
    mister_guest_lfb_info info = {.size = sizeof(info)};
    assert(!mister_glide_lfb_lock(0, 0, 0, 0, 0, NULL));
    for (unsigned i = 0; i < 6; ++i) {
        assert(!mister_glide_lfb_lock(i == 0 ? 2 : 0, i == 1 ? 3 : 0, i == 2 ? 3 : 0,
                                    i == 3 ? 2 : 0, i == 4 ? 2 : 0, &(mister_guest_lfb_info){.size = i == 5 ? 0 : sizeof(info)}));
    }
    assert(!mister_glide_lfb_lock(1, 0, 0, 0, 1, &info));
    assert(!locks);
    lock_error = 1;
    assert(!mister_glide_lfb_lock(0, 0, 0, 0, 0, &info));
    lock_error = 0;
    stride = 4096;
    assert(!mister_glide_lfb_lock(0, 0, 0, 0, 0, &info) && unlocks == 1);
    stride = 2048;
    assert(mister_glide_lfb_lock(0x11, 1, 255, 255, 0, &info));
    assert(info.stride == 2048 && info.origin == 0 && !info.pointer && !info.mode);
    assert(!mister_glide_lfb_lock(1, 1, 0, 0, 0, &info));
    assert(!mister_glide_lfb_lock(0, 0, 0, 1, 0, &info));
    assert(mister_glide_lfb_lock(0, 0, 0, 0, 0, &info));
    assert(mister_glide_lfb_unlock(0, 0));
    for (unsigned offset = 0; offset < 4; ++offset)
        for (unsigned size = 1; size <= 4; size *= 2) {
            for (unsigned j = 0; j < 4; ++j) ram[j] = 0xaaaaaaaa;
            mister_glide_lfb_write(1, offset, 0x12345678, size);
            for (unsigned j = 0; j < 12; ++j) {
                unsigned expected = j >= offset && j < offset + size ? (0x12345678 >> (8 * (j - offset))) & 255 : 170;
                assert(((ram[j / 4] >> (8 * (j % 4))) & 255) == expected);
            }
        }
    assert(bus_writes == 0 && map_offset == 150 * 4096);
    assert(!mister_glide_lfb_unlock(1, 0));
    unlock_error = 1;
    assert(!mister_glide_lfb_unlock(1, 1));
    unlock_error = 0;
    assert(mister_glide_lfb_unlock(1, 1));
    assert(mister_glide_lfb_lock(0, 0, 0, 0, 0, &info));
    ram[0] = 0x76543210; ram[1] = 0xfedcba98;
    for (unsigned offset = 0; offset < 4; ++offset)
        for (unsigned size = 1; size <= 4; size *= 2) {
            uint32_t expected = (UINT64_C(0xfedcba9876543210) >> (8 * offset)) & (UINT32_MAX >> (8 * (4 - size)));
            assert(mister_glide_lfb_read(0, offset, size) == expected);
        }
    for (unsigned scenario = 0; scenario < 9; ++scenario) {
        pid_t child = fork(); assert(child >= 0);
        if (!child) {
            if (scenario == 0) mister_glide_lfb_read(3, 0, 1);
            if (scenario == 1) mister_glide_lfb_read(0, sizeof(ram) - 1, 2);
            if (scenario == 2) mister_glide_lfb_read(0, UINT32_MAX, 1);
            if (scenario == 3) mister_glide_lfb_read(0, 0, 3);
            if (scenario == 4) mister_glide_lfb_write(0, 0, 1, 1);
            if (scenario == 5) { mister_glide_lfb_reset(); mister_glide_lfb_read(0, 0, 1); }
            if (scenario == 6) { stop_error = 1; mister_glide_lfb_lock(1, 0, 0, 0, 0, &info); }
            if (scenario == 7) { assert(mister_glide_lfb_lock(1, 1, 0, 0, 0, &info)); mister_glide_lfb_write(1, 479 * 2048 + 1280, 0, 4); }
            if (scenario == 8) mister_glide_lfb_read(0, 1279, 2);
            _exit(0);
        }
        int status; assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE);
    }
    assert(mister_glide_lfb_unlock(0, 0));
    assert(mister_glide_lfb_lock(1, 0, 0, 0, 0, &info) && stops == 1);
    assert(mister_glide_lfb_unlock(1, 0) && presents == 1 && fences);
    height = 0;
    assert(!mister_glide_lfb_lock(1, 0, 0, 0, 0, &info));
    height = 480;
    for (width = 0; width <= 1025; width += 1025)
        assert(!mister_glide_lfb_lock(1, 0, 0, 0, 0, &info));
    width = 640;
    map_error = 1;
    assert(!mister_glide_lfb_lock(1, 1, 0, 0, 0, &info));
    map_error = 0;
    fb_status = 1u << 28;
    assert(!mister_glide_lfb_lock(1, 1, 0, 0, 0, &info));
    fb_status = 0;
    fb_stride = 9 << 4;
    assert(!mister_glide_lfb_lock(1, 1, 0, 0, 0, &info));
    fb_stride = 10 << 4;
    for (unsigned i = 0; i < 2; ++i) {
        fb_offset = (i ? 400 : 149) << 11;
        assert(!mister_glide_lfb_lock(1, 1, 0, 0, 0, &info));
    }
    fb_offset = 150 << 11;
    fb_origin = 478u << 22;
    assert(!mister_glide_lfb_lock(1, 1, 0, 1, 0, &info));
    fb_origin = 479u << 22;
    for (unsigned swap = 0; swap < 2; ++swap)
        for (unsigned buffer = 0; buffer < 3; ++buffer) {
            fb_status = swap << 10;
            assert(mister_glide_lfb_lock(1, buffer, buffer == 2 ? 15 : 0, 1, 0, &info));
            assert(map_offset == (buffer == 2 ? 2 : swap ^ buffer) * 150 * 4096);
            mister_glide_lfb_write(buffer, 4, 0x12345678, 4);
            assert(ram[(479 * 1280 + 4) / 4] == 0x12345678);
            unsigned before = bus_writes;
            assert(mister_glide_lfb_unlock(1, buffer) && bus_writes == before + 1);
        }
    puts("glide_guest_test: PASS");
    return 0;
}
