#include "mister_runtime.h"
#include "mister_memory.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static uint8_t memory[MISTER_MAILBOX_BYTES];
static mister_command received[2048];
static size_t count;
static unsigned drains;
static int submit_error, drain_error;

int __wrap_mister_memory_open(mister_memory *mapping) {
    mapping->base = memory;
    mapping->lock_fd = 42;
    return 0;
}

int __wrap_mister_memory_close(mister_memory *mapping) {
    mapping->base = NULL;
    mapping->lock_fd = -1;
    return 0;
}

int __wrap_mister_mailbox_attach(mister_mailbox *mailbox, volatile void *base,
                                 size_t bytes, unsigned timeout) {
    assert(base == memory && bytes == MISTER_MEMORY_BYTES && timeout == 1000);
    memset(mailbox, 0, sizeof(*mailbox));
    mailbox->words = base;
    return 0;
}

void __wrap_mister_mailbox_detach(mister_mailbox *mailbox) {
    assert(!mailbox->pending);
    mailbox->words = NULL;
}

int __wrap_mister_mailbox_drain(mister_mailbox *mailbox) {
    ++drains;
    if (drain_error) { errno = drain_error; return -1; }
    mailbox->pending = 0;
    return 0;
}

int __wrap_mister_mailbox_submit_writes(mister_mailbox *mailbox, const mister_command *commands,
                                       size_t size) {
    if (!commands || !size || size > MISTER_MAILBOX_MAX_COMMANDS) { errno = EINVAL; return -1; }
    for (size_t i = 0; i < size; ++i)
        if (!mister_command_valid(commands + i) || commands[i].opcode != 1) { errno = EINVAL; return -1; }
    if (submit_error) { errno = submit_error; return -1; }
    assert(count + size <= sizeof(received) / sizeof(received[0]));
    memcpy(received + count, commands, size * sizeof(*commands));
    count += size;
    mailbox->pending = 1;
    return 0;
}

int __wrap_mister_mailbox_exchange(mister_mailbox *mailbox, const mister_command *commands,
                                   size_t size, uint32_t *results, mister_keys *keys) {
    assert(!keys && size == 1 && mister_command_valid(commands));
    assert(!__wrap_mister_mailbox_drain(mailbox));
    received[count++] = *commands;
    if (results) results[0] = 0x12345678;
    return 0;
}

int main(void) {
    mister_command values[30];
    for (unsigned i = 0; i < 30; ++i) values[i] = (mister_command){1, 0x88 + i * 4, i, 15};
    assert(!mister_runtime_acquire());
    assert(!mister_runtime_submit_writes(values, 30) && count == 30);
    assert(!memcmp(values, received, sizeof(values)));
    unsigned before_drains = drains;
    assert(!mister_runtime_submit_writes(values, 30) && count == 30 && drains == before_drains + 1);
    mister_command triangles[] = {values[0], {1, 0x80, 0, 15}, {1, 0x80, 0, 15},
                                  values[0], {1, 0x100, 0, 15}, {1, 0x100, 0, 15}};
    assert(!mister_runtime_submit_writes(triangles, 6) && count == 34);
    assert(received[30].address == 0x80 && received[31].address == 0x80);
    assert(received[32].address == 0x100 && received[33].address == 0x100);
    values[0].value = 0x80000000;
    assert(!mister_runtime_submit_writes(values, 1) && count == 35);
    values[0].value = 0;
    assert(!mister_runtime_submit_writes(values, 1) && count == 36);
    values[0].value = 0x7fc00001;
    assert(!mister_runtime_submit_writes(values, 1) && count == 37);
    values[0].value = 0x7fc00002;
    assert(!mister_runtime_submit_writes(values, 1) && count == 38);
    mister_command read = {2, 0, 0, 15};
    uint32_t result;
    assert(!mister_runtime_exchange(&read, 1, &result, NULL) && result == 0x12345678);
    size_t previous = count;
    assert(!mister_runtime_submit_writes(values, 30) && count == previous);
    const uint32_t invalidators[] = {0x8, 0x7c, 0x84, 0x104, 0x888, 0x10088};
    for (unsigned i = 0; i < sizeof(invalidators) / sizeof(invalidators[0]); ++i) {
        mister_command other = {1, invalidators[i], 0, 15};
        assert(!mister_runtime_submit_writes(&other, 1));
        previous = count;
        assert(!mister_runtime_submit_writes(values, 30) && count == previous + 30);
    }
    const uint32_t memory_addresses[] = {0x400000, 0x7ffffc, 0x800000, 0xe20000, 0xfffffc};
    for (unsigned i = 0; i < sizeof(memory_addresses) / sizeof(memory_addresses[0]); ++i) {
        mister_command write = {1, memory_addresses[i], i, 15};
        previous = count;
        assert(!mister_runtime_submit_writes(&write, 1));
        assert(!mister_runtime_submit_writes(&write, 1) && count == previous + 2);
        assert(!mister_runtime_submit_writes(values, 30) && count == previous + 2);
        assert(received[previous].address == write.address && received[previous].value == i);
        write.mask = 3;
        assert(!mister_runtime_submit_writes(&write, 1));
        previous = count;
        assert(!mister_runtime_submit_writes(values, 30) && count == previous + 30);
    }
    mister_command partial = values[0];
    partial.mask = 3;
    assert(!mister_runtime_submit_writes(&partial, 1));
    previous = count;
    assert(!mister_runtime_submit_writes(values, 30) && count == previous + 30);
    mister_command alias = {1, 0x8, 5, 15};
    assert(!mister_runtime_exchange(&alias, 1, NULL, NULL));
    previous = count;
    assert(!mister_runtime_submit_writes(values, 30) && count == previous + 30);
    mister_command invalid[] = {values[0], {1, 0x89, 0, 15}};
    previous = count;
    assert(mister_runtime_submit_writes(invalid, 2) == -1 && errno == EINVAL && count == previous);
    assert(!mister_runtime_submit_writes(values, 30) && count == previous + 30);
    assert(mister_runtime_submit_writes(values, 257) == -1 && errno == EINVAL);
    assert(mister_runtime_submit_writes(NULL, 1) == -1 && errno == EINVAL);
    assert(mister_runtime_submit_writes(values, 0) == -1 && errno == EINVAL);
    for (unsigned opcode = 0; opcode <= 4; ++opcode) {
        if (opcode == 1) continue;
        mister_command other = values[0];
        other.opcode = opcode;
        assert(mister_runtime_submit_writes(&other, 1) == -1 && errno == EINVAL);
    }
    mister_command bad_mask = values[0];
    bad_mask.mask = 0;
    assert(mister_runtime_submit_writes(&bad_mask, 1) == -1 && errno == EINVAL);
    bad_mask.mask = 16;
    assert(mister_runtime_submit_writes(&bad_mask, 1) == -1 && errno == EINVAL);
    values[0].value ^= 1;
    submit_error = EIO;
    previous = count;
    assert(mister_runtime_submit_writes(values, 30) == -1 && errno == EIO && count == previous);
    submit_error = 0;
    assert(!mister_runtime_submit_writes(values, 30) && count == previous + 30);
    drain_error = ETIMEDOUT;
    previous = count;
    assert(mister_runtime_submit_writes(values, 30) == -1 && errno == ETIMEDOUT && count == previous);
    drain_error = 0;
    assert(!mister_runtime_submit_writes(values, 30) && count == previous + 30);
    assert(!mister_runtime_acquire());
    assert(!mister_runtime_exchange(&alias, 1, NULL, NULL));
    previous = count;
    assert(!mister_runtime_submit_writes(values, 30) && count == previous + 30);
    assert(!mister_runtime_release());
    previous = count;
    assert(!mister_runtime_submit_writes(values, 30) && count == previous);
    assert(!mister_runtime_release() && !mister_runtime_acquire());
    previous = count;
    assert(!mister_runtime_submit_writes(values, 30) && count == previous + 30);
    assert(!mister_runtime_release());
    puts("MiSTer float-parameter write elision: PASS");
    return 0;
}
