#include "mister_runtime.h"
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>
#include <ctime>
#include "keyboard.h"

int mister_dosbox_input(void);
int mister_dosbox_start(void);
static bool open_error;
static unsigned opens;

extern "C" int __wrap_mister_runtime_acquire(void) {
    ++opens;
    if (open_error) { errno = EACCES; return -1; }
    return 0;
}

extern "C" int __wrap_mister_runtime_release(void) {
    assert(opens == 2);
    return 0;
}
static mister_keys snapshot;
static size_t space = 1024;
static bool exchange_error, clock_error;
static unsigned exchanges;
static uint64_t clock_ns = 4000000, clock_step = 4000000;
static std::vector<std::pair<KBD_KEYS, bool>> events;

extern "C" int __wrap_clock_gettime(clockid_t id, struct timespec *value) {
    assert(id == CLOCK_MONOTONIC);
    if (clock_error) { errno = EIO; return -1; }
    value->tv_sec = clock_ns / 1000000000;
    value->tv_nsec = clock_ns % 1000000000;
    clock_ns += clock_step;
    return 0;
}

extern "C" int __wrap_mister_runtime_exchange(const mister_command *commands, size_t count,
                                               uint32_t *results, mister_keys *keys) {
    assert(count == 1 && commands[0].opcode == 4 && !commands[0].address &&
           !commands[0].value && !commands[0].mask && !results && keys);
    ++exchanges;
    if (exchange_error) { errno = EPIPE; return -1; }
    *keys = snapshot;
    return 0;
}

size_t KEYBOARD_BufferSpaceAvail() { return space; }
void KEYBOARD_AddKey(KBD_KEYS key, bool pressed) {
    assert(space >= 8);
    space -= 8;
    events.emplace_back(key, pressed);
}

static void set(unsigned scan) { snapshot.bits[scan / 8] |= 1u << (scan % 8); }

int main() {
    assert(!mister_dosbox_input() && events.empty());
    open_error = true;
    assert(mister_dosbox_start() == -1 && errno == EACCES);
    open_error = false;
    assert(!mister_dosbox_start() && !mister_dosbox_start() && opens == 2);
    assert(!mister_dosbox_input() && events.empty());
    set(0x1c);
    set(0x114);
    set(0x175);
    set(0x75);
    set(0x1ff);
    space = 7;
    assert(!mister_dosbox_input() && events.empty());
    space = 8;
    assert(!mister_dosbox_input() && events.size() == 1);
    assert(events[0] == std::make_pair(KBD_rightctrl, true));
    space = 1024;
    assert(!mister_dosbox_input() && events.size() == 4);
    assert(events[1] == std::make_pair(KBD_a, true));
    assert(events[2] == std::make_pair(KBD_up, true));
    assert(events[3] == std::make_pair(KBD_kp8, true));
    ++snapshot.sequence;
    assert(!mister_dosbox_input() && events.size() == 4);
    std::memset(snapshot.bits, 0, sizeof(snapshot.bits));
    exchange_error = true;
    assert(mister_dosbox_input() == -1 && errno == EPIPE && events.size() == 4);
    exchange_error = false;
    assert(!mister_dosbox_input() && events.size() == 8);
    for (size_t i = 0; i < 4; ++i)
        assert(events[i].first == events[i + 4].first && !events[i + 4].second);
    assert(!mister_dosbox_input() && events.size() == 8);
    clock_step = 0;
    assert(!mister_dosbox_input());
    unsigned before = exchanges;
    assert(!mister_dosbox_input() && exchanges == before);
    clock_ns += 3999999;
    assert(!mister_dosbox_input() && exchanges == before);
    ++clock_ns;
    assert(!mister_dosbox_input() && exchanges == before + 1);
    clock_error = true;
    assert(mister_dosbox_input() == -1 && errno == EIO && exchanges == before + 1);
    clock_error = false;
    clock_ns += 4000000;
    exchange_error = true;
    assert(mister_dosbox_input() == -1 && errno == EPIPE);
    exchange_error = false;
    assert(!mister_dosbox_input() && exchanges == before + 3);
    std::puts("dosbox_input_test: PASS");
    return 0;
}
