#include "mister_runtime.h"
#include <cstddef>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "keyboard.h"

static bool started;

static void stop(void) {
    if (mister_runtime_release()) std::perror("MiSTer runtime cleanup");
    started = false;
}

int mister_dosbox_start(void) {
    if (started) return 0;
    if (mister_runtime_acquire()) return -1;
    if (std::atexit(stop)) {
        mister_runtime_release();
        errno = ENOMEM;
        return -1;
    }
    started = true;
    return 0;
}

int mister_dosbox_input(void) {
    if (!started) return 0;
    static uint8_t delivered[64] = {};
    static uint64_t next_poll;
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now)) return -1;
    uint64_t time = uint64_t(now.tv_sec) * 1000000000 + now.tv_nsec;
    if (time < next_poll) return 0;
    static const struct { unsigned scan; KBD_KEYS key; } bindings[] = {
        {0x12, KBD_leftshift}, {0x59, KBD_rightshift}, {0x14, KBD_leftctrl},
        {0x114, KBD_rightctrl}, {0x11, KBD_leftalt}, {0x111, KBD_rightalt},
        {0x76, KBD_esc}, {0x0d, KBD_tab}, {0x66, KBD_backspace}, {0x5a, KBD_enter},
        {0x29, KBD_space}, {0x58, KBD_capslock}, {0x77, KBD_numlock}, {0x7e, KBD_scrolllock},
        {0x16, KBD_1}, {0x1e, KBD_2}, {0x26, KBD_3}, {0x25, KBD_4}, {0x2e, KBD_5},
        {0x36, KBD_6}, {0x3d, KBD_7}, {0x3e, KBD_8}, {0x46, KBD_9}, {0x45, KBD_0},
        {0x15, KBD_q}, {0x1d, KBD_w}, {0x24, KBD_e}, {0x2d, KBD_r}, {0x2c, KBD_t},
        {0x35, KBD_y}, {0x3c, KBD_u}, {0x43, KBD_i}, {0x44, KBD_o}, {0x4d, KBD_p},
        {0x1c, KBD_a}, {0x1b, KBD_s}, {0x23, KBD_d}, {0x2b, KBD_f}, {0x34, KBD_g},
        {0x33, KBD_h}, {0x3b, KBD_j}, {0x42, KBD_k}, {0x4b, KBD_l},
        {0x1a, KBD_z}, {0x22, KBD_x}, {0x21, KBD_c}, {0x2a, KBD_v},
        {0x32, KBD_b}, {0x31, KBD_n}, {0x3a, KBD_m},
        {0x0e, KBD_grave}, {0x4e, KBD_minus}, {0x55, KBD_equals}, {0x5d, KBD_backslash},
        {0x54, KBD_leftbracket}, {0x5b, KBD_rightbracket}, {0x4c, KBD_semicolon},
        {0x52, KBD_quote}, {0x49, KBD_period}, {0x41, KBD_comma}, {0x4a, KBD_slash},
        {0x61, KBD_extra_lt_gt}, {0x05, KBD_f1}, {0x06, KBD_f2}, {0x04, KBD_f3},
        {0x0c, KBD_f4}, {0x03, KBD_f5}, {0x0b, KBD_f6}, {0x83, KBD_f7}, {0x0a, KBD_f8},
        {0x01, KBD_f9}, {0x09, KBD_f10}, {0x78, KBD_f11}, {0x07, KBD_f12},
        {0x17c, KBD_printscreen}, {0x177, KBD_pause}, {0x170, KBD_insert}, {0x171, KBD_delete},
        {0x16c, KBD_home}, {0x169, KBD_end}, {0x17d, KBD_pageup}, {0x17a, KBD_pagedown},
        {0x16b, KBD_left}, {0x175, KBD_up}, {0x172, KBD_down}, {0x174, KBD_right},
        {0x69, KBD_kp1}, {0x72, KBD_kp2}, {0x7a, KBD_kp3}, {0x6b, KBD_kp4},
        {0x73, KBD_kp5}, {0x74, KBD_kp6}, {0x6c, KBD_kp7}, {0x75, KBD_kp8},
        {0x7d, KBD_kp9}, {0x70, KBD_kp0}, {0x14a, KBD_kpdivide}, {0x7c, KBD_kpmultiply},
        {0x7b, KBD_kpminus}, {0x79, KBD_kpplus}, {0x15a, KBD_kpenter}, {0x71, KBD_kpperiod},
        {0x0f, KBD_kpequals}, {0x11f, KBD_lwindows}, {0x127, KBD_rwindows}, {0x12f, KBD_rwinmenu}
    };
    const mister_command command = {4, 0, 0, 0};
    mister_keys keys = {};
    if (mister_runtime_exchange(&command, 1, nullptr, &keys)) return -1;
    next_poll = time + 4000000;
    for (const auto &binding : bindings) {
        unsigned byte = binding.scan / 8, mask = 1u << (binding.scan % 8);
        if (!((delivered[byte] ^ keys.bits[byte]) & mask)) continue;
        if (KEYBOARD_BufferSpaceAvail() < 8) break;
        KEYBOARD_AddKey(binding.key, (keys.bits[byte] & mask) != 0);
        delivered[byte] ^= mask;
    }
    return 0;
}
