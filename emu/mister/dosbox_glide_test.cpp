#include "glide_guest.h"
#include <cassert>
#include <cstring>
#include <stdexcept>

static void *hdll = reinterpret_cast<void *>(1);
static unsigned lfbacc = 3, reads, writes;
static uint32_t held[2] = {UINT32_MAX, UINT32_MAX};
static bool fail_write, fail_unlock;
[[noreturn]] static void E_Exit(const char *, ...) { throw std::runtime_error("LFB"); }
static int lock_lfb(uint32_t type, uint32_t buffer, uint32_t mode, uint32_t origin,
                    uint32_t pipeline, mister_guest_lfb_info *info) {
    unsigned access = type & 1;
    if (type & ~0x11u || buffer > 2 || pipeline || !info || held[access] != UINT32_MAX || (access && fail_write)) return 0;
    held[access] = buffer;
    info->stride = 2048; info->mode = mode; info->origin = origin;
    return 1;
}
static int unlock_lfb(uint32_t type, uint32_t buffer) {
    unsigned access = type & 1;
    if (type & ~0x11u || buffer > 2 || held[access] != buffer || fail_unlock) return 0;
    held[access] = UINT32_MAX;
    return 1;
}
static uint32_t read_lfb(uint32_t buffer, uint32_t, uint32_t) {
    assert(held[0] == buffer); ++reads; return 0x1234;
}
static void write_lfb(uint32_t buffer, uint32_t, uint32_t, uint32_t) {
    assert(held[1] == buffer); ++writes;
}
static void reset_lfb(void) { held[0] = held[1] = UINT32_MAX; }
static void *dlsym(void *, const char *name) {
    if (!std::strcmp(name, "mister_glide_lfb_lock")) return reinterpret_cast<void *>(&lock_lfb);
    if (!std::strcmp(name, "mister_glide_lfb_unlock")) return reinterpret_cast<void *>(&unlock_lfb);
    if (!std::strcmp(name, "mister_glide_lfb_read")) return reinterpret_cast<void *>(&read_lfb);
    if (!std::strcmp(name, "mister_glide_lfb_write")) return reinterpret_cast<void *>(&write_lfb);
    if (!std::strcmp(name, "mister_glide_lfb_reset")) return reinterpret_cast<void *>(&reset_lfb);
    return nullptr;
}
#include "dosbox_glide.h"

int main() {
    mister_guest_lfb_info info = {sizeof(info), 0, 0, 0, 0};
    assert(!mister_guest_lock(0, 3, 0, 0, 0, &info));
    assert(!mister_guest_lock(2, 1, 0, 0, 0, &info));
    assert(!mister_guest_lock(0, 1, 0, 0, 0, nullptr));
    assert(mister_guest_lock(0, 1, 0, 0, 0, &info));
    assert(held[0] == 1 && held[1] == 1);
    assert(mister_guest_read(1, 0, 2) == 0x1234);
    mister_guest_write(1, 479 * 2048, 0xabcd, 4);
    assert(reads == 1 && writes == 1);
    assert(!mister_guest_unlock(2, 1) && held[0] == 1 && held[1] == 1);
    assert(!mister_guest_unlock(1, 1) && held[0] == 1 && held[1] == 1);
    assert(mister_guest_unlock(0x10, 1) && held[0] == UINT32_MAX && held[1] == UINT32_MAX);
    fail_write = true;
    assert(!mister_guest_lock(0, 1, 0, 0, 0, &info));
    assert(held[0] == UINT32_MAX && held[1] == UINT32_MAX);
    fail_write = false;
    lfbacc = 1;
    assert(mister_guest_lock(0, 0, 0, 0, 0, &info) && held[1] == UINT32_MAX);
    bool rejected = false;
    try { mister_guest_write(0, 0, 0, 4); } catch (const std::runtime_error &) { rejected = true; }
    assert(rejected && writes == 1);
    assert(mister_guest_unlock(0, 0));
    lfbacc = 2;
    assert(mister_guest_lock(1, 1, 0, 0, 0, &info));
    rejected = false;
    try { mister_guest_read(1, 0, 4); } catch (const std::runtime_error &) { rejected = true; }
    assert(rejected && reads == 1);
    assert(mister_guest_unlock(1, 1));
    mister_guest_write(1, 0x6390, 0xabcd, 2);
    assert(held[1] == 1);
    lfbacc = 7;
    assert(mister_guest_lock(0, 2, 15, 1, 0, &info));
    assert(info.mode == 15 && held[0] == 2 && held[1] == 2);
    lfbacc = 3;
    rejected = false;
    try { mister_guest_read(2, 0, 4); } catch (const std::runtime_error &) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { mister_guest_write(2, 0, 0, 4); } catch (const std::runtime_error &) { rejected = true; }
    assert(rejected);
    lfbacc = 7;
    mister_guest_reset();
    assert(held[0] == UINT32_MAX && held[1] == UINT32_MAX);
    assert(mister_guest_lock(1, 2, 15, 1, 0, &info));
    assert(mister_guest_unlock(1, 2));
    assert(mister_guest_read(2, 0, 4) == 0x1234);
    mister_guest_write(2, 0, 0, 4);
    assert(held[0] == 2 && held[1] == 2);
    mister_guest_flush();
    assert(held[0] == UINT32_MAX && held[1] == UINT32_MAX);
    fail_write = true;
    rejected = false;
    try { mister_guest_write(2, 0, 0, 4); } catch (const std::runtime_error &) { rejected = true; }
    assert(rejected && held[1] == UINT32_MAX);
    fail_write = false;
    mister_guest_write(2, 0, 0, 4);
    fail_unlock = true;
    rejected = false;
    try { mister_guest_flush(); } catch (const std::runtime_error &) { rejected = true; }
    assert(rejected && held[1] == 2);
    fail_unlock = false;
    mister_guest_reset();
    for (uint32_t buffer : {0u, 1u, 2u, 3u, UINT32_MAX}) {
        rejected = false;
        try { mister_guest_write(buffer, 0, 0, 4); } catch (const std::runtime_error &) { rejected = true; }
        assert(rejected);
    }
    return 0;
}
