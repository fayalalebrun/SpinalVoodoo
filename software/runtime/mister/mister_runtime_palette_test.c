#include "mister_runtime.h"
#include "mister_memory.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static uint8_t memory[MISTER_MAILBOX_BYTES];
static uint32_t hardware[32][256], selected, capability = 0x50414c20;
static unsigned writes, submits, queries, fail_submit;
static int drain_error, query_error;
static mister_mailbox *attached;

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
    attached = mailbox;
    selected = 0;
    return 0;
}
void __wrap_mister_mailbox_detach(mister_mailbox *mailbox) {
    assert(!mailbox->pending);
    attached = NULL;
}
int __wrap_mister_mailbox_drain(mister_mailbox *mailbox) {
    if (drain_error) { errno = drain_error; return -1; }
    mailbox->pending = 0;
    return 0;
}
int __wrap_mister_mailbox_submit_writes(mister_mailbox *mailbox, const mister_command *commands,
                                       size_t count) {
    assert(commands && count && count <= MISTER_MAILBOX_MAX_COMMANDS);
    ++submits;
    if (submits == fail_submit) { errno = EIO; return -1; }
    if (__wrap_mister_mailbox_drain(mailbox)) return -1;
    for (size_t i = 0; i < count; ++i) {
        mister_command command = commands[i];
        assert(mister_command_valid(&command) && command.opcode == 1);
        unsigned address = command.address & 0x3ff;
        if (command.address == 0x3ec) {
            assert(capability == 0x50414c20 || capability == 0x50414c10);
            assert(command.value < (capability & 255));
            if (command.mask == 15) selected = command.value;
        } else if (command.address < 0x400000 && address >= 0x334 && address <= 0x350 &&
                   (command.value & 0x80000000)) {
            unsigned index = ((command.value >> 23) & 254) | (((address - 0x334) / 4) & 1);
            hardware[selected][index] = command.value & 0xffffff;
            ++writes;
        }
    }
    mailbox->pending = 1;
    return 0;
}
int __wrap_mister_mailbox_exchange(mister_mailbox *mailbox, const mister_command *commands,
                                   size_t count, uint32_t *results, mister_keys *keys) {
    assert(!keys && count == 1 && mister_command_valid(commands));
    if (__wrap_mister_mailbox_drain(mailbox)) return -1;
    if (commands->opcode == 1) return __wrap_mister_mailbox_submit_writes(mailbox, commands, count);
    if (commands->address == 0x3ec) {
        ++queries;
        if (query_error) { errno = query_error; return -1; }
        assert(results);
        *results = capability;
    } else if (results) *results = 0;
    return 0;
}

int main(void) {
    uint32_t colors[256];
    for (unsigned i = 0; i < 256; ++i) colors[i] = 0xab000000 | i;
    assert(mister_runtime_upload_palette(colors, 0, 255) == -1 && errno == ENOTCONN);
    assert(!mister_runtime_acquire());
    assert(mister_runtime_upload_palette(NULL, 0, 255) == -1 && errno == EINVAL);
    assert(mister_runtime_upload_palette(colors, 1, 0) == -1 && errno == EINVAL);
    assert(mister_runtime_upload_palette(colors, 0, 256) == -1 && errno == EINVAL);
    query_error = EIO;
    assert(mister_runtime_upload_palette(colors, 0, 255) == -1 && errno == EIO && !writes);
    query_error = 0;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == 256 && queries == 2);
    for (unsigned i = 0; i < 256; ++i) assert(hardware[selected][i] == i);
    for (unsigned i = 0; i < 256; ++i) colors[i] ^= 0xff000000;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == 256 && queries == 2);
    for (unsigned table = 1; table < 32; ++table) {
        colors[0] = table + 256;
        assert(!mister_runtime_upload_palette(colors, 0, 255));
    }
    assert(writes == 8192);
    colors[0] = 0;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == 8192);
    colors[0] = 288;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == 8448);
    colors[0] = 0;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == 8448);
    colors[0] = 257;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == 8704);
    unsigned before = writes;
    colors[1] = 0x123456;
    assert(!mister_runtime_upload_palette(colors, 1, 1) && writes == before + 1);
    assert(hardware[selected][0] == 257 && hardware[selected][1] == 0x123456);
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 257);
    const mister_command alias = {1, 0x734, 0x80000088, 3};
    assert(!mister_runtime_submit_writes(&alias, 1));
    before = writes;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 256);
    assert(!mister_runtime_exchange(&alias, 1, NULL, NULL));
    before = writes;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 256);
    const mister_command select = {1, 0x3ec, 31, 15};
    assert(!mister_runtime_submit_writes(&select, 1));
    before = writes;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 256);
    colors[2]++;
    fail_submit = submits + 2;
    assert(mister_runtime_upload_palette(colors, 0, 255) == -1 && errno == EIO);
    before = writes;
    fail_submit = 0;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 256);
    drain_error = ETIMEDOUT;
    assert(mister_runtime_upload_palette(colors, 0, 255) == -1 && errno == ETIMEDOUT);
    drain_error = 0;
    before = writes;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 256);
    attached->faulted = 1;
    assert(mister_runtime_upload_palette(colors, 0, 255) == -1 && errno == EPIPE);
    attached->faulted = 0;
    assert(!mister_runtime_release() && !mister_runtime_acquire());
    before = writes;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 256);
    assert(!mister_runtime_release());
    capability = 0x50414c10;
    assert(!mister_runtime_acquire());
    before = writes;
    for (unsigned table = 0; table < 16; ++table) {
        colors[0] = 1000 + table;
        assert(!mister_runtime_upload_palette(colors, 0, 255));
    }
    assert(writes == before + 4096);
    colors[0] = 1000;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 4096);
    colors[0] = 1016;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 4352);
    colors[0] = 1001;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 4608);
    assert(!mister_runtime_submit_writes(&alias, 1));
    before = writes;
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 256);
    assert(!mister_runtime_release());
    capability = 0;
    assert(!mister_runtime_acquire());
    before = writes;
    assert(!mister_runtime_upload_palette(colors, 0, 255));
    assert(!mister_runtime_upload_palette(colors, 0, 255) && writes == before + 512);
    assert(!mister_runtime_release());
    puts("MiSTer banked palette cache: PASS");
    return 0;
}
