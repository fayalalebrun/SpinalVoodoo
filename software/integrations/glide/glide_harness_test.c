#include "../../simulation/xls/harness/sim_harness.h"
#include "mister_runtime.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void mister_glide_write_masked(uint32_t, uint32_t, uint32_t);
static mister_command received[1024];
static size_t received_count, batches;
static unsigned opens, closes, stops;
static int open_error, exchange_error, stop_error;
static unsigned palette_calls;

int __wrap_mister_runtime_upload_palette(const uint32_t *data, unsigned start, unsigned end) {
    assert(data && start == 0 && end == 255);
    assert(received_count && received[received_count - 1].address == 0x80);
    if (exchange_error) { errno = EIO; return -1; }
    ++palette_calls;
    return 0;
}

int __wrap_mister_runtime_stop_presenting(void) {
    ++stops;
    if (stop_error) { errno = ETIMEDOUT; return -1; }
    return 0;
}

int __wrap_mister_runtime_acquire(void) {
    ++opens;
    if (open_error) { errno = EBUSY; return -1; }
    return 0;
}

int __wrap_mister_runtime_release(void) {
    ++closes;
    return 0;
}

int __wrap_mister_runtime_exchange(const mister_command *commands, size_t count,
                                    uint32_t *results, mister_keys *keys) {
    ++batches;
    assert(!keys && count && count <= MISTER_MAILBOX_MAX_COMMANDS);
    if (exchange_error) { errno = ETIMEDOUT; return -1; }
    assert(received_count + count <= sizeof(received) / sizeof(received[0]));
    for (size_t i = 0; i < count; ++i) {
        received[received_count++] = commands[i];
        if (results) results[i] = commands[i].opcode == 2 ? 0x12345678 : 0;
    }
    return 0;
}

int __wrap_mister_runtime_submit_writes(const mister_command *commands, size_t count) {
    for (size_t i = 0; i < count; ++i) assert(commands[i].opcode == 1);
    return __wrap_mister_runtime_exchange(commands, count, NULL, NULL);
}

static void invalid(unsigned scenario) {
    switch (scenario) {
    case 0: sim_write(3, 0); break;
    case 1: sim_write16(0x400001, 0); break;
    case 2: sim_read(0x1000000); break;
    case 3: sim_write_tex_bulk(0x7ffffc, &received[0].value, 2); break;
    case 4: sim_write_tex_bulk(0, NULL, 1); break;
    case 5: exchange_error = 1; sim_read(0); break;
    case 6: sim_shutdown(); sim_write(0, 0); break;
    case 7: mister_glide_write_masked(0x3ffffc, 1, 15); break;
    case 8: mister_glide_write_masked(0x800000, 1, 15); break;
    case 9: mister_glide_write_masked(0x400001, 1, 15); break;
    case 10: mister_glide_write_masked(0x400000, 1, 16); break;
    }
}

int main(void) {
    assert(sim_stalled());
    open_error = 1;
    assert(sim_init() == -1 && errno == EBUSY);
    open_error = 0;
    assert(!sim_init() && !sim_init() && opens == 2);
    sim_write(0x300, 0x42);
    sim_write16(0x400000, 0x1234);
    sim_write16(0x400002, 0xabcd);
    assert(!batches && sim_read(0) == 0x12345678);
    assert(batches == 1 && received_count == 4);
    assert(received[0].address == 0x300 && received[0].value == 0x42 && received[0].mask == 15);
    assert(received[1].value == 0x1234 && received[1].mask == 3);
    assert(received[2].address == 0x400000 && received[2].value == 0xabcd0000 && received[2].mask == 12);
    uint32_t texture[257];
    for (unsigned i = 0; i < 257; ++i) texture[i] = i ^ 0x13579bdf;
    sim_write_tex_bulk(0x400, texture, 257);
    assert(batches == 2 && received_count == 260);
    sim_write(0x80, 1);
    assert(sim_idle_wait() == 0x12345678);
    assert(batches == 3 && received_count == 263);
    for (unsigned i = 0; i < 257; ++i)
        assert(received[i + 4].address == 0x800400 + i * 4 && received[i + 4].value == texture[i]);
    assert(received[261].address == 0x80 && received[262].opcode == 2);
    mister_glide_write_masked(0x400000, 0x5600, 2);
    sim_idle_wait();
    assert(received[263].address == 0x400000 && received[263].value == 0x5600 && received[263].mask == 2);
    for (unsigned i = 0; i < 11; ++i) {
        pid_t child = fork();
        assert(child >= 0);
        if (!child) { invalid(i); _exit(0); }
        int status;
        assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE);
    }
    sim_shutdown();
    sim_shutdown();
    assert(closes == 1 && sim_stalled());
    assert(received[received_count - 1].opcode == 2 && received[received_count - 1].address == 0);
    assert(stops == 1);
    stop_error = 1;
    assert(sim_init() == -1 && errno == ETIMEDOUT && sim_stalled() && closes == 2);
    stop_error = 0;
    assert(!sim_init() && stops == 3);
    sim_shutdown();
    assert(closes == 3);
    assert(!sim_init());
    uint32_t palette[256] = {0};
    sim_write(0x80, 7);
    sim_write_palette(palette, 0, 255);
    assert(palette_calls == 1);
    for (unsigned scenario = 0; scenario < 4; ++scenario) {
        pid_t child = fork();
        assert(child >= 0);
        if (!child) {
            if (scenario == 0) sim_write_palette(NULL, 0, 255);
            if (scenario == 1) sim_write_palette(palette, 0, 256);
            if (scenario == 2) sim_write_palette(palette, 5, 4);
            if (scenario == 3) { exchange_error = 1; sim_write_palette(palette, 0, 255); }
            _exit(0);
        }
        int status;
        assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE);
    }
    sim_shutdown();
    puts("glide_harness_test: PASS");
    return 0;
}
