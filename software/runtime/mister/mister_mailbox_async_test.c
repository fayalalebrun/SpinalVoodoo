#define main synchronous_tests
#include "mister_mailbox_wait_test.c"
#undef main

int main(void) {
    mister_mailbox mailbox;
    mister_command commands[] = {{1, 0x80, 0x1234, 15}, {1, 0x84, 0x5678, 15}};
    assert(mister_mailbox_submit_writes(NULL, commands, 2) == -1 && errno == EINVAL);
    assert(mister_mailbox_drain(NULL) == -1 && errno == EINVAL);
    setup(&mailbox, 30000);
    assert(!mister_mailbox_drain(&mailbox) && !clocks);
    assert(mister_mailbox_submit_writes(&mailbox, NULL, 1) == -1 && errno == EINVAL);
    assert(mister_mailbox_submit_writes(&mailbox, commands, 0) == -1 && errno == EINVAL);
    assert(mister_mailbox_submit_writes(&mailbox, commands, 257) == -1 && errno == EINVAL);
    for (unsigned opcode = 0; opcode <= 4; ++opcode) {
        if (opcode == 1) continue;
        mister_command invalid = {opcode, 0, 0, opcode == 2 ? 15 : 0};
        assert(mister_mailbox_submit_writes(&mailbox, &invalid, 1) == -1 && errno == EINVAL);
    }
    mister_command invalid = {1, 3, 0, 15};
    assert(mister_mailbox_submit_writes(&mailbox, &invalid, 1) == -1 && errno == EINVAL);
    assert(!clocks && !mailbox.sequence);
    assert(!mister_mailbox_submit_writes(&mailbox, commands, 2));
    assert(clocks == 2 && !sleeps && memory[MISTER_MAILBOX_ACK / 4] == 0);
    assert(mailbox.sequence == 1 && memory[2] == 2);
    assert(memory[MISTER_MAILBOX_COMMANDS / 4 + 2] == 0x1234);
    assert(mister_mailbox_submit_writes(&mailbox, &invalid, 1) == -1 && errno == EINVAL);
    assert(memory[1] == 1 && memory[2] == 2 && clocks == 2);
    assert(!mister_mailbox_submit_writes(&mailbox, commands + 1, 1));
    assert(memory[MISTER_MAILBOX_ACK / 4] == 2 && mailbox.sequence == 2);
    assert(memory[MISTER_MAILBOX_COMMANDS / 4 + 2] == 0x5678);
    assert(!mister_mailbox_drain(&mailbox) && mailbox.status == 256);
    unsigned before = clocks;
    assert(!mister_mailbox_drain(&mailbox) && clocks == before);
    mister_mailbox_detach(&mailbox);
    setup(&mailbox, 30000);
    assert(!mister_mailbox_submit_writes(&mailbox, commands, 2));
    mister_command read = {2, 0, 0, 15};
    uint32_t result = 0;
    memory[MISTER_MAILBOX_RESULTS / 4] = 0xabcdef01;
    assert(!mister_mailbox_exchange(&mailbox, &read, 1, &result, NULL));
    assert(mailbox.sequence == 2 && result == 0xabcdef01 && !mailbox.faulted);
    mister_mailbox_detach(&mailbox);
    for (unsigned error = 0; error < 4; ++error) {
        setup(&mailbox, error == 1 ? 30000 : UINT64_MAX);
        assert(!mister_mailbox_submit_writes(&mailbox, commands, 2));
        if (error == 1) protocol_error = 1;
        if (error == 2) clock_failure = 3;
        if (error == 3) sleep_error = EIO;
        assert(mister_mailbox_drain(&mailbox) == -1);
        assert(errno == (error == 0 ? ETIMEDOUT : error == 1 ? EPROTO : EIO));
        assert(mailbox.faulted && mailbox.sequence == 1);
        assert(mister_mailbox_submit_writes(&mailbox, commands, 2) == -1 && errno == EPIPE);
        assert(mister_mailbox_drain(&mailbox) == -1 && errno == EPIPE);
        assert(mister_mailbox_exchange(&mailbox, &read, 1, &result, NULL) == -1 && errno == EPIPE);
        mister_mailbox_detach(&mailbox);
    }
    setup(&mailbox, UINT64_MAX);
    assert(!mister_mailbox_submit_writes(&mailbox, commands, 2));
    time_ns += 1000000;
    assert(mister_mailbox_submit_writes(&mailbox, commands + 1, 1) == -1 && errno == ETIMEDOUT);
    assert(!sleeps && mailbox.sequence == 1 && memory[2] == 2);
    assert(memory[MISTER_MAILBOX_COMMANDS / 4 + 2] == 0x1234);
    mister_mailbox_detach(&mailbox);
    setup(&mailbox, UINT64_MAX);
    assert(!mister_mailbox_submit_writes(&mailbox, commands, 2));
    time_ns += 2000000;
    memory[MISTER_MAILBOX_ACK / 4] = 1;
    memory[MISTER_MAILBOX_ACK / 4 + 1] = 512;
    assert(!mister_mailbox_drain(&mailbox) && !sleeps);
    mister_mailbox_detach(&mailbox);
    setup(&mailbox, UINT64_MAX);
    clock_failure = 1;
    assert(mister_mailbox_submit_writes(&mailbox, commands, 2) == -1 && errno == EIO);
    assert(!mailbox.faulted && !mailbox.sequence);
    mister_mailbox_detach(&mailbox);
    setup(&mailbox, UINT64_MAX);
    clock_failure = 2;
    assert(mister_mailbox_submit_writes(&mailbox, commands, 2) == -1 && errno == EIO);
    assert(mailbox.faulted && mailbox.sequence == 1);
    mister_mailbox_detach(&mailbox);
    puts("MiSTer ordered asynchronous mailbox writes: PASS");
    return 0;
}
