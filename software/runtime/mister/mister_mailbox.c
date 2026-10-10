#define _POSIX_C_SOURCE 200809L
#include "mister_mailbox.h"
#include <errno.h>
#include <string.h>
#include <time.h>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error MiSTer mailbox requires a little-endian host
#endif

static void barrier(void) {
#if defined(__arm__) || defined(__aarch64__)
    __asm__ volatile("dmb sy" ::: "memory");
#else
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
#endif
}

static uint32_t load_word(volatile uint32_t *words, unsigned offset) {
    return __atomic_load_n(words + offset / 4, __ATOMIC_RELAXED);
}

static void store_word(volatile uint32_t *words, unsigned offset, uint32_t value) {
    __atomic_store_n(words + offset / 4, value, __ATOMIC_RELAXED);
}

static int now_ns(uint64_t *value) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now)) return -1;
    *value = (uint64_t)now.tv_sec * 1000000000 + now.tv_nsec;
    return 0;
}

int mister_command_valid(const mister_command *command) {
    if (!command) return 0;
    switch (command->opcode) {
    case 0:
    case 4:
        return !command->address && !command->value && !command->mask;
    case 1:
    case 2:
        return command->address < 0x1000000 && !(command->address & 3) &&
               command->mask && command->mask <= 15;
    case 3: {
        uint32_t width = command->value & 4095;
        uint32_t height = (command->value >> 12) & 4095;
        uint32_t format = (command->value >> 24) & 31;
        uint32_t stride = command->mask;
        uint32_t row = width * (format == 4 ? 2 : 4);
        if ((command->value >> 30) || stride >= 16384) return 0;
        if (!(command->value & (1u << 29))) return 1;
        uint64_t extent = (uint64_t)(height ? height - 1 : 0) * stride + row;
        int host = command->address >= MISTER_FRAME_BASE &&
                   command->address < MISTER_FRAME_BASE + 3 * MISTER_FRAME_SLOT_BYTES &&
                   (command->address - MISTER_FRAME_BASE) % MISTER_FRAME_SLOT_BYTES == 0 &&
                   extent <= MISTER_FRAME_SLOT_BYTES;
        int renderer = command->address >= MISTER_RENDERER_BASE && !(command->address & 4095) &&
                       command->address < MISTER_RENDERER_BASE + MISTER_RENDERER_BYTES &&
                       command->address + extent <= MISTER_RENDERER_BASE + MISTER_RENDERER_BYTES &&
                       format == 4 && width == 640 && height == 480;
        return (host || renderer) && (format == 4 || format == 6) && width && width <= 1920 &&
               height && height <= 1200 && stride >= row && !(stride & 3);
    }
    default:
        return 0;
    }
}

int mister_mailbox_attach(mister_mailbox *mailbox, volatile void *memory, size_t bytes,
                          unsigned timeout_ms) {
    if (!mailbox || !memory || ((uintptr_t)memory & 7) || bytes < MISTER_MAILBOX_BYTES ||
        !timeout_ms || timeout_ms > 60000) {
        errno = EINVAL;
        return -1;
    }
    memset(mailbox, 0, sizeof(*mailbox));
    mailbox->words = memory;
    mailbox->timeout_ms = timeout_ms;
    mailbox->sequence = load_word(mailbox->words, MISTER_MAILBOX_ACK);
    barrier();
    if (load_word(mailbox->words, 4) != mailbox->sequence) {
        mailbox->words = NULL;
        errno = EBUSY;
        return -1;
    }
    int error = pthread_mutex_init(&mailbox->lock, NULL);
    if (error) {
        mailbox->words = NULL;
        errno = error;
        return -1;
    }
    if (mister_mailbox_exchange(mailbox, NULL, 0, NULL, NULL)) {
        int saved = errno;
        mister_mailbox_detach(mailbox);
        errno = saved;
        return -1;
    }
    return 0;
}

void mister_mailbox_detach(mister_mailbox *mailbox) {
    if (!mailbox || !mailbox->words) return;
    pthread_mutex_destroy(&mailbox->lock);
    mailbox->words = NULL;
}

static int drain_locked(mister_mailbox *mailbox) {
    uint64_t now;
    unsigned polls = 0;
    if (mailbox->faulted) { errno = EPIPE; return -1; }
    if (!mailbox->pending) return 0;
    while (load_word(mailbox->words, MISTER_MAILBOX_ACK) != mailbox->sequence) {
        if (++polls < 8) continue;
        polls = 0;
        if (now_ns(&now)) goto fault;
        if (now - mailbox->started >= (uint64_t)mailbox->timeout_ms * 1000000) {
            errno = ETIMEDOUT;
            goto fault;
        }
        if (now - mailbox->submitted < 50000) continue;
        struct timespec pause = {0, 50000};
        if (nanosleep(&pause, NULL) && errno != EINTR) goto fault;
    }
    barrier();
    mailbox->status = load_word(mailbox->words, MISTER_MAILBOX_ACK + 4);
    if (mailbox->status != (uint32_t)mailbox->pending_count << 8) {
        errno = EPROTO;
        goto fault;
    }
    mailbox->pending = 0;
    return 0;
fault:
    mailbox->faulted = 1;
    return -1;
}

int mister_mailbox_drain(mister_mailbox *mailbox) {
    if (!mailbox || !mailbox->words) { errno = EINVAL; return -1; }
    int error = pthread_mutex_lock(&mailbox->lock);
    if (error) { errno = error; return -1; }
    error = drain_locked(mailbox) ? errno : 0;
    pthread_mutex_unlock(&mailbox->lock);
    if (error) errno = error;
    return error ? -1 : 0;
}

static int exchange(mister_mailbox *mailbox, const mister_command *commands,
                    size_t count, uint32_t *results, mister_keys *keys, int asynchronous) {
    if (!mailbox || !mailbox->words || count > MISTER_MAILBOX_MAX_COMMANDS ||
        (count && !commands)) {
        errno = EINVAL;
        return -1;
    }
    int snapshot = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!mister_command_valid(commands + i) || (asynchronous && commands[i].opcode != 1)) {
            errno = EINVAL;
            return -1;
        }
        snapshot |= commands[i].opcode == 4;
    }
    int error = pthread_mutex_lock(&mailbox->lock);
    if (error) { errno = error; return -1; }
    if (drain_locked(mailbox)) { error = errno; goto done; }
    if (now_ns(&mailbox->started)) { error = errno; goto done; }
    _Static_assert(sizeof(mister_command) == 16, "mailbox command layout");
    for (size_t i = 0; i < count; ++i)
        *(volatile mister_command *)(mailbox->words + MISTER_MAILBOX_COMMANDS / 4 + i * 4) = commands[i];
    store_word(mailbox->words, 8, (uint32_t)count);
    store_word(mailbox->words, 12, 0);
    store_word(mailbox->words, 0, MISTER_MAILBOX_MAGIC);
    barrier();
    ++mailbox->sequence;
    store_word(mailbox->words, 4, mailbox->sequence);
    barrier();
    mailbox->pending = 1;
    mailbox->pending_count = count;
    if (now_ns(&mailbox->submitted)) { error = errno; goto fault; }
    if (asynchronous) goto done;
    if (drain_locked(mailbox)) { error = errno; goto done; }
    if (results) for (size_t i = 0; i < count; ++i)
        results[i] = commands[i].opcode == 2 ?
            load_word(mailbox->words, MISTER_MAILBOX_RESULTS + (unsigned)i * 4) : 0;
    if (snapshot && keys) {
        for (unsigned i = 0; i < 16; ++i) {
            uint32_t word = load_word(mailbox->words, MISTER_MAILBOX_KEYS + i * 4);
            memcpy(keys->bits + i * 4, &word, 4);
        }
        keys->sequence = load_word(mailbox->words, MISTER_MAILBOX_KEYS + 64);
    }
    goto done;
fault:
    mailbox->faulted = 1;
done:
    pthread_mutex_unlock(&mailbox->lock);
    if (error) errno = error;
    return error ? -1 : 0;
}

int mister_mailbox_exchange(mister_mailbox *mailbox, const mister_command *commands,
                            size_t count, uint32_t *results, mister_keys *keys) {
    return exchange(mailbox, commands, count, results, keys, 0);
}

int mister_mailbox_submit_writes(mister_mailbox *mailbox, const mister_command *commands,
                                 size_t count) {
    if (!count) { errno = EINVAL; return -1; }
    return exchange(mailbox, commands, count, NULL, NULL, 1);
}
