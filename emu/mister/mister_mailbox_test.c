#define _POSIX_C_SOURCE 200809L
#include "mister_mailbox.h"
#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint32_t words[MISTER_MAILBOX_BYTES / 4] __attribute__((aligned(8)));
static atomic_int stop;
static atomic_int mode;
static atomic_uint processed;
static atomic_uint checked_writes[2];

static uint32_t load(unsigned offset) {
    return __atomic_load_n(words + offset / 4, __ATOMIC_ACQUIRE);
}

static void store(unsigned offset, uint32_t value) {
    __atomic_store_n(words + offset / 4, value, __ATOMIC_RELEASE);
}

static void *device(void *unused) {
    (void)unused;
    uint32_t sequence = load(MISTER_MAILBOX_ACK);
    while (!atomic_load(&stop)) {
        int current_mode = atomic_load(&mode);
        if (current_mode != 1 && load(0) == MISTER_MAILBOX_MAGIC && load(4) != sequence) {
            sequence = load(4);
            unsigned count = load(8);
            assert(count <= MISTER_MAILBOX_MAX_COMMANDS && load(12) == 0);
            unsigned completed = current_mode == 3 ? 0 : count;
            for (unsigned i = 0; i < completed; ++i) {
                unsigned offset = MISTER_MAILBOX_COMMANDS + i * 16;
                unsigned opcode = load(offset);
                if (opcode == 1) {
                    unsigned id = (load(offset + 4) - 0x400) / 4;
                    assert(id < 2 && load(offset + 12) == 15);
                    assert(load(offset + 8) == atomic_fetch_add(&checked_writes[id], 1));
                }
                if (opcode == 2) store(MISTER_MAILBOX_RESULTS + i * 4, load(offset + 4) ^ 0xabcdef12);
                if (opcode == 4) {
                    for (unsigned word = 0; word < 16; ++word) store(MISTER_MAILBOX_KEYS + word * 4, word + 1);
                    store(MISTER_MAILBOX_KEYS + 64, 41);
                }
            }
            atomic_fetch_add(&processed, 1);
            store(MISTER_MAILBOX_ACK + 4, (completed << 8) |
                  (current_mode == 2 ? 3 : current_mode == 3 ? 4 : 0));
            store(MISTER_MAILBOX_ACK, sequence);
        }
        struct timespec pause = {0, 10000};
        nanosleep(&pause, NULL);
    }
    return NULL;
}

static void *client(void *opaque) {
    mister_mailbox *mailbox = opaque;
    mister_command read = {2, 0x100, 0, 15};
    for (unsigned i = 0; i < 20; ++i) {
        uint32_t result = 0;
        assert(mister_mailbox_exchange(mailbox, &read, 1, &result, NULL) == 0);
        assert(result == (0x100 ^ 0xabcdef12));
    }
    return NULL;
}

struct writer_context { mister_mailbox *mailbox; unsigned id; };

static void *writer(void *opaque) {
    struct writer_context *context = opaque;
    mister_command batch[MISTER_MAILBOX_MAX_COMMANDS];
    for (unsigned i = 0; i < 20; ++i) {
        for (unsigned j = 0; j < MISTER_MAILBOX_MAX_COMMANDS; ++j)
            batch[j] = (mister_command){1, 0x400 + context->id * 4,
                                       i * MISTER_MAILBOX_MAX_COMMANDS + j, 15};
        assert(!mister_mailbox_submit_writes(context->mailbox, batch, MISTER_MAILBOX_MAX_COMMANDS));
    }
    assert(!mister_mailbox_drain(context->mailbox));
    return NULL;
}

int main(void) {
    mister_mailbox mailbox;
    assert(mister_mailbox_attach(NULL, words, sizeof(words), 100) == -1 && errno == EINVAL);
    assert(mister_mailbox_attach(&mailbox, words + 1, sizeof(words) - 4, 100) == -1 && errno == EINVAL);
    assert(mister_mailbox_attach(&mailbox, words, 64, 100) == -1 && errno == EINVAL);
    assert(mister_mailbox_attach(&mailbox, words, sizeof(words), 0) == -1 && errno == EINVAL);
    store(4, UINT32_MAX - 1);
    assert(mister_mailbox_attach(&mailbox, words, sizeof(words), 100) == -1 && errno == EBUSY);
    store(MISTER_MAILBOX_ACK, UINT32_MAX - 1);
    pthread_t thread;
    assert(pthread_create(&thread, NULL, device, NULL) == 0);
    assert(mister_mailbox_attach(&mailbox, words, sizeof(words), 1000) == 0);
    assert(mailbox.sequence == UINT32_MAX);
    mister_command batch[256];
    uint32_t results[256];
    for (unsigned i = 0; i < 256; ++i) batch[i] = (mister_command){2, i * 4, 0, 15};
    assert(mister_mailbox_exchange(&mailbox, batch, 256, results, NULL) == 0);
    assert(mailbox.sequence == 0 && load(MISTER_MAILBOX_ACK) == 0);
    for (unsigned i = 0; i < 256; ++i) assert(results[i] == (i * 4 ^ 0xabcdef12));
    uint32_t sequence = load(4);
    mister_command invalid[] = {{1, 3, 0, 15}, {2, 0x1000000, 0, 15}, {1, 0, 0, 0},
        {1, 0, 0, 16}, {99, 0, 0, 0}, {4, 1, 0, 0}, {3, MISTER_FRAME_BASE, 0xc0000000, 0},
        {3, MISTER_FRAME_BASE + 8, 640 | (480 << 12) | (4 << 24) | (1 << 29), 1280},
        {3, MISTER_FRAME_BASE, 1920 | (1200 << 12) | (6 << 24) | (1 << 29), 7680}};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
        assert(mister_mailbox_exchange(&mailbox, &invalid[i], 1, NULL, NULL) == -1 && errno == EINVAL);
        assert(load(4) == sequence);
    }
    assert(mister_mailbox_exchange(&mailbox, batch, 257, NULL, NULL) == -1 && errno == EINVAL);
    assert(mister_mailbox_exchange(&mailbox, NULL, 1, NULL, NULL) == -1 && errno == EINVAL);
    mister_command frame = {3, MISTER_FRAME_BASE, 640 | (480 << 12) | (4 << 24) | (1 << 29), 1280};
    assert(mister_mailbox_exchange(&mailbox, &frame, 1, NULL, NULL) == 0);
    mister_command renderer = {3, 0x3f400000, 640 | (480 << 12) | (4 << 24) | (1u << 29), 1280};
    assert(mister_command_valid(&renderer));
    renderer.address += 150 * 4096;
    assert(mister_command_valid(&renderer));
    renderer.address = 0x3f800000 - 150 * 4096;
    assert(mister_command_valid(&renderer));
    renderer.address += 4096;
    assert(!mister_command_valid(&renderer));
    renderer.address = 0x3f400004;
    assert(!mister_command_valid(&renderer));
    renderer.address = 0x3f400000;
    renderer.value = 640 | (480 << 12) | (6 << 24) | (1u << 29);
    renderer.mask = 2560;
    assert(!mister_command_valid(&renderer));
    mister_keys keys = {0};
    mister_command snapshot = {4, 0, 0, 0};
    assert(mister_mailbox_exchange(&mailbox, &snapshot, 1, NULL, &keys) == 0);
    assert(keys.bits[0] == 1 && keys.bits[60] == 16 && keys.sequence == 41);
    pthread_t a, b;
    assert(pthread_create(&a, NULL, client, &mailbox) == 0);
    assert(pthread_create(&b, NULL, client, &mailbox) == 0);
    assert(pthread_join(a, NULL) == 0 && pthread_join(b, NULL) == 0);
    struct writer_context writers[] = {{&mailbox, 0}, {&mailbox, 1}};
    pthread_t reader;
    assert(!pthread_create(&a, NULL, writer, &writers[0]));
    assert(!pthread_create(&b, NULL, writer, &writers[1]));
    assert(!pthread_create(&reader, NULL, client, &mailbox));
    assert(!pthread_join(a, NULL) && !pthread_join(b, NULL) && !pthread_join(reader, NULL));
    assert(atomic_load(&checked_writes[0]) == 20 * MISTER_MAILBOX_MAX_COMMANDS);
    assert(atomic_load(&checked_writes[1]) == 20 * MISTER_MAILBOX_MAX_COMMANDS);
    atomic_store(&mode, 2);
    assert(mister_mailbox_exchange(&mailbox, NULL, 0, NULL, NULL) == -1 && errno == EPROTO);
    sequence = load(4);
    assert(mailbox.status == 3);
    assert(mister_mailbox_exchange(&mailbox, NULL, 0, NULL, NULL) == -1 && errno == EPIPE);
    assert(load(4) == sequence);
    mister_mailbox_detach(&mailbox);
    atomic_store(&mode, 0);
    assert(mister_mailbox_attach(&mailbox, words, sizeof(words), 1000) == 0);
    atomic_store(&mode, 3);
    assert(mister_mailbox_exchange(&mailbox, &frame, 1, NULL, NULL) == -1 && errno == EPROTO);
    assert(mailbox.status == 4);
    sequence = load(4);
    assert(mister_mailbox_exchange(&mailbox, &frame, 1, NULL, NULL) == -1 && errno == EPIPE);
    assert(load(4) == sequence);
    mister_mailbox_detach(&mailbox);
    assert(mister_mailbox_attach(&mailbox, words, sizeof(words), 1000) == -1 && errno == EPROTO);
    assert(mailbox.words == NULL);
    atomic_store(&mode, 0);
    assert(mister_mailbox_attach(&mailbox, words, sizeof(words), 1000) == 0);
    atomic_store(&mode, 1);
    mailbox.timeout_ms = 5;
    assert(mister_mailbox_exchange(&mailbox, NULL, 0, NULL, NULL) == -1 && errno == ETIMEDOUT);
    sequence = load(4);
    assert(mister_mailbox_exchange(&mailbox, NULL, 0, NULL, NULL) == -1 && errno == EPIPE);
    assert(load(4) == sequence);
    mister_mailbox_detach(&mailbox);
    assert(mister_mailbox_attach(&mailbox, words, sizeof(words), 1000) == -1 && errno == EBUSY);
    assert(load(4) == sequence);
    atomic_store(&stop, 1);
    assert(pthread_join(thread, NULL) == 0);
    printf("MiSTer host transport PASS (%u batches)\n", atomic_load(&processed));
    return 0;
}
