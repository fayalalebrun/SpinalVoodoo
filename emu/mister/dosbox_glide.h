#include "glide_guest.h"

static bool mister_guest_paired[3], mister_guest_mapped[3], mister_guest_implicit[2];
static uint32_t mister_guest_locked[2] = {UINT32_MAX, UINT32_MAX};
static uint32_t mister_guest_origins[3];

template<class T> static T mister_guest_symbol(const char *name) {
    T symbol = reinterpret_cast<T>(dlsym(hdll, name));
    if (!symbol) E_Exit("MiSTer Glide extension missing: %s", name);
    return symbol;
}

static void mister_guest_release(unsigned access) {
    if (!mister_guest_implicit[access]) return;
    if (!mister_guest_symbol<decltype(&mister_glide_lfb_unlock)>("mister_glide_lfb_unlock")(
            access, mister_guest_locked[access])) E_Exit("MiSTer implicit LFB unlock failed");
    mister_guest_implicit[access] = false;
    mister_guest_locked[access] = UINT32_MAX;
}

static void mister_guest_flush(void) {
    mister_guest_release(1);
    mister_guest_release(0);
}

static void mister_guest_access(unsigned access, uint32_t buffer) {
    if (buffer > 2 || !mister_guest_mapped[buffer]) E_Exit("MiSTer unmapped guest LFB access");
    if (mister_guest_locked[access] == buffer) return;
    mister_guest_release(access);
    mister_guest_lfb_info info = {sizeof(info), 0, 0, 0, 0};
    if (!mister_guest_symbol<decltype(&mister_glide_lfb_lock)>("mister_glide_lfb_lock")(
            access, buffer, buffer == 2 ? 15 : 0, mister_guest_origins[buffer], 0, &info))
        E_Exit("MiSTer implicit LFB lock failed");
    mister_guest_locked[access] = buffer;
    mister_guest_implicit[access] = true;
}

static uint32_t mister_guest_read(uint32_t buffer, uint32_t offset, uint32_t size) {
    if (!(lfbacc & 1) || (buffer == 2 && !(lfbacc & 4))) E_Exit("MiSTer guest LFB reads disabled");
    mister_guest_access(0, buffer);
    static auto function = mister_guest_symbol<decltype(&mister_glide_lfb_read)>("mister_glide_lfb_read");
    return function(buffer, offset, size);
}

static void mister_guest_write(uint32_t buffer, uint32_t offset, uint32_t value, uint32_t size) {
    if (!(lfbacc & 2) || (buffer == 2 && !(lfbacc & 4))) E_Exit("MiSTer guest LFB writes disabled");
    mister_guest_access(1, buffer);
    static auto function = mister_guest_symbol<decltype(&mister_glide_lfb_write)>("mister_glide_lfb_write");
    function(buffer, offset, value, size);
}

static int mister_guest_lock(uint32_t type, uint32_t buffer, uint32_t mode, uint32_t origin,
                            uint32_t pipeline, mister_guest_lfb_info *info) {
    mister_guest_flush();
    static auto function = mister_guest_symbol<decltype(&mister_glide_lfb_lock)>("mister_glide_lfb_lock");
    if (!function(type, buffer, mode, origin, pipeline, info)) return 0;
    if (!(type & 1) && (lfbacc & 2)) {
        mister_guest_lfb_info writable = {sizeof(writable), 0, 0, 0, 0};
        if (!function(1, buffer, buffer == 2 ? 15 : 0, info->origin, pipeline, &writable)) {
            if (!mister_guest_symbol<decltype(&mister_glide_lfb_unlock)>("mister_glide_lfb_unlock")(type, buffer))
                E_Exit("MiSTer guest LFB lock rollback failed");
            return 0;
        }
        mister_guest_paired[buffer] = true;
        mister_guest_locked[1] = buffer;
    }
    mister_guest_locked[type & 1] = buffer;
    mister_guest_mapped[buffer] = true;
    mister_guest_origins[buffer] = info->origin;
    return 1;
}

static int mister_guest_unlock(uint32_t type, uint32_t buffer) {
    mister_guest_flush();
    static auto function = mister_guest_symbol<decltype(&mister_glide_lfb_unlock)>("mister_glide_lfb_unlock");
    if (buffer < 3 && mister_guest_paired[buffer]) {
        if ((type & ~0x10u) || !function(1, buffer)) return 0;
        mister_guest_paired[buffer] = false;
        mister_guest_locked[1] = UINT32_MAX;
    }
    int result = function(type, buffer);
    if (result) mister_guest_locked[type & 1] = UINT32_MAX;
    return result;
}

static void mister_guest_reset(void) {
    mister_guest_flush();
    for (auto &paired : mister_guest_paired) paired = false;
    for (auto &mapped : mister_guest_mapped) mapped = false;
    mister_guest_locked[0] = mister_guest_locked[1] = UINT32_MAX;
    if (hdll) mister_guest_symbol<decltype(&mister_glide_lfb_reset)>("mister_glide_lfb_reset")();
}
