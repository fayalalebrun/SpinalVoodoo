#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
static uint32_t test_load(volatile uint32_t *word, int order);
#define __atomic_load_n test_load
#define clock_gettime test_clock_gettime
#define nanosleep test_nanosleep
#include "mister_mailbox.c"
#undef __atomic_load_n
#include <assert.h>
#include <stdio.h>

static uint32_t memory[MISTER_MAILBOX_BYTES / 4] __attribute__((aligned(8)));
static uint64_t time_ns, reply_ns, preparation_ns;
static unsigned sleeps, clocks, clock_failure;
static int sleep_error, protocol_error;
static unsigned ack_reads, ack_after;

static uint32_t test_load(volatile uint32_t *word, int order) {
    if (word == memory + MISTER_MAILBOX_ACK / 4 && ++ack_reads == ack_after) {
        memory[MISTER_MAILBOX_ACK / 4 + 1] = memory[2] << 8;
        memory[MISTER_MAILBOX_ACK / 4] = memory[1];
    }
    return __atomic_load_n(word, order);
}

int test_clock_gettime(clockid_t clock, struct timespec *value) {
    assert(clock == CLOCK_MONOTONIC);
    if (++clocks == clock_failure) { errno = EIO; return -1; }
    time_ns += 1000;
    if (clocks == 2) time_ns += preparation_ns;
    if (time_ns >= reply_ns && memory[1] != memory[MISTER_MAILBOX_ACK / 4]) {
        memory[MISTER_MAILBOX_ACK / 4 + 1] = protocol_error ? 3 : memory[2] << 8;
        memory[MISTER_MAILBOX_ACK / 4] = memory[1];
    }
    value->tv_sec = time_ns / 1000000000;
    value->tv_nsec = time_ns % 1000000000;
    return 0;
}

int test_nanosleep(const struct timespec *request, struct timespec *remaining) {
    assert(!remaining && request->tv_sec == 0 && request->tv_nsec == 50000);
    ++sleeps;
    time_ns += request->tv_nsec;
    if (sleep_error) { errno = sleep_error; return -1; }
    return 0;
}

static void setup(mister_mailbox *mailbox, uint64_t delay) {
    memset(memory, 0, sizeof(memory));
    memset(mailbox, 0, sizeof(*mailbox));
    mailbox->words = memory;
    mailbox->timeout_ms = 1;
    assert(!pthread_mutex_init(&mailbox->lock, NULL));
    time_ns = preparation_ns = 0;
    reply_ns = delay;
    sleeps = clocks = clock_failure = 0;
    sleep_error = protocol_error = 0;
    ack_reads = ack_after = 0;
}

int main(void) {
    mister_mailbox mailbox;
    const mister_command command = {1, 0x128, 0, 15};
    for (unsigned reply = 1; reply <= 17; ++reply) {
        setup(&mailbox, UINT64_MAX);
        ack_after = reply;
        assert(!mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL));
        assert(clocks == 2 + (reply - 1) / 8 && ack_reads == reply);
        assert(!sleeps && !mailbox.faulted);
        mister_mailbox_detach(&mailbox);
    }
    setup(&mailbox, 30000);
    assert(!mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL));
    assert(!sleeps && mailbox.sequence == 1 && !mailbox.faulted);
    mister_mailbox_detach(&mailbox);
    setup(&mailbox, 130000);
    preparation_ns = 100000;
    assert(!mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL));
    assert(!sleeps && !mailbox.faulted);
    mister_mailbox_detach(&mailbox);
    for (unsigned interrupted = 0; interrupted < 2; ++interrupted) {
        setup(&mailbox, 200000);
        sleep_error = interrupted ? EINTR : 0;
        assert(!mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL));
        assert(sleeps && !mailbox.faulted);
        mister_mailbox_detach(&mailbox);
    }
    setup(&mailbox, UINT64_MAX);
    assert(mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL) == -1 && errno == ETIMEDOUT);
    assert(sleeps && mailbox.faulted && time_ns >= 1000000 && time_ns < 1100000);
    assert(mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL) == -1 && errno == EPIPE);
    mister_mailbox_detach(&mailbox);
    for (unsigned failure = 1; failure <= 2; ++failure) {
        setup(&mailbox, UINT64_MAX);
        clock_failure = failure;
        assert(mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL) == -1 && errno == EIO);
        assert(mailbox.faulted == (failure == 2) && mailbox.sequence == failure - 1);
        mister_mailbox_detach(&mailbox);
    }
    setup(&mailbox, UINT64_MAX);
    clock_failure = 3;
    assert(mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL) == -1 && errno == EIO);
    assert(mailbox.faulted && mailbox.sequence == 1);
    mister_mailbox_detach(&mailbox);
    setup(&mailbox, UINT64_MAX);
    preparation_ns = 1000000;
    assert(mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL) == -1 && errno == ETIMEDOUT);
    assert(mailbox.faulted && !sleeps && time_ns < 1100000);
    mister_mailbox_detach(&mailbox);
    setup(&mailbox, UINT64_MAX);
    sleep_error = EIO;
    assert(mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL) == -1 && errno == EIO);
    assert(mailbox.faulted && sleeps == 1);
    mister_mailbox_detach(&mailbox);
    setup(&mailbox, 30000);
    protocol_error = 1;
    assert(mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL) == -1 && errno == EPROTO);
    assert(mailbox.faulted && mailbox.status == 3);
    mister_mailbox_detach(&mailbox);
    puts("MiSTer bounded mailbox polling: PASS");
    return 0;
}
