#include "../../simulation/xls/harness/sim_harness.h"
#include "mister_runtime.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static mister_command pending[MISTER_MAILBOX_MAX_COMMANDS];
static size_t count;
static int active;

static void fatal(void) {
    perror("MiSTer Glide transport");
    exit(EXIT_FAILURE);
}

static void flush(uint32_t *results) {
    if (!active) { errno = ENOTCONN; fatal(); }
    if (count && (results ? mister_runtime_exchange(pending, count, results, NULL) :
                           mister_runtime_submit_writes(pending, count))) fatal();
    count = 0;
}

static void append(mister_command command) {
    if (!active) { errno = ENOTCONN; fatal(); }
    if (!mister_command_valid(&command)) { errno = EINVAL; fatal(); }
    if (count == MISTER_MAILBOX_MAX_COMMANDS) flush(NULL);
    pending[count++] = command;
}

int sim_init(void) {
    if (active) return 0;
    if (mister_runtime_acquire()) return -1;
    if (mister_runtime_stop_presenting()) {
        int error = errno;
        mister_runtime_release();
        errno = error;
        return -1;
    }
    active = 1;
    count = 0;
    return 0;
}

void sim_shutdown(void) {
    if (!active) return;
    sim_idle_wait();
    active = 0;
    if (mister_runtime_release()) fatal();
}

void mister_glide_write_masked(uint32_t address, uint32_t data, uint32_t mask) {
    if (address < 0x400000 || address >= 0x800000) { errno = EINVAL; fatal(); }
    append((mister_command){1, address, data, mask});
}

void sim_write(uint32_t address, uint32_t data) {
    append((mister_command){1, address, data, 15});
}

void sim_write16(uint32_t address, uint16_t data) {
    if (address & 1) { errno = EINVAL; fatal(); }
    unsigned shift = (address & 2) * 8;
    append((mister_command){1, address & ~3u, (uint32_t)data << shift, 3u << (shift / 8)});
}

uint32_t sim_read(uint32_t address) {
    uint32_t results[MISTER_MAILBOX_MAX_COMMANDS];
    append((mister_command){2, address, 0, 15});
    size_t index = count - 1;
    flush(results);
    return results[index];
}

uint32_t sim_idle_wait(void) {
    return sim_read(0);
}

void sim_write_palette(const uint32_t *data, unsigned start, unsigned end) {
    if (!data || start > end || end > 255) { errno = EINVAL; fatal(); }
    flush(NULL);
    if (mister_runtime_upload_palette(data, start, end)) fatal();
}

void sim_write_tex_bulk(uint32_t offset, const uint32_t *source, uint32_t words) {
    if ((offset & 3) || offset > 0x800000 || words > (0x800000 - offset) / 4 ||
        (words && !source)) { errno = EINVAL; fatal(); }
    for (uint32_t i = 0; i < words; ++i)
        sim_write(0x800000 + offset + i * 4, source[i]);
}

int sim_stalled(void) {
    return !active;
}
