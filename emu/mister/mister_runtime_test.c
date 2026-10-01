#include "mister_runtime.h"
#include "mister_memory.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static uint8_t mapping[MISTER_MEMORY_BYTES];
static uint8_t source[480 * 1284 + 1];
static uint8_t active_copy[480 * 1280];
static unsigned frames, disables;
static int active = -1;
static unsigned opens, closes, attaches, detaches, exchanges;
static int open_error, attach_error, close_error, exchange_error;
static uint32_t renderer_status, renderer_stride = 10 << 4, renderer_offset = 150 << 11;
static unsigned renderer_frames;
static int renderer_publish_error, drain_error, submit_error;
static unsigned submissions, drains;
static mister_mailbox *async_mailbox;

int __wrap_mister_mailbox_submit_writes(mister_mailbox *mailbox, const mister_command *commands,
                                       size_t count) {
    assert(commands && count == 1 && commands[0].opcode == 1);
    ++submissions;
    if (submit_error) { errno = submit_error; return -1; }
    mailbox->pending = 1;
    async_mailbox = mailbox;
    return 0;
}

int __wrap_mister_mailbox_drain(mister_mailbox *mailbox) {
    ++drains;
    if (drain_error) { errno = drain_error; return -1; }
    mailbox->pending = 0;
    return 0;
}

int __wrap_mister_memory_open(mister_memory *memory) {
    ++opens;
    assert(!memory->base && memory->lock_fd == -1);
    memory->base = mapping;
    memory->lock_fd = 42;
    if (open_error) { errno = open_error; return -1; }
    return 0;
}

int __wrap_mister_memory_close(mister_memory *memory) {
    ++closes;
    if (close_error) { errno = close_error; return -1; }
    memory->base = NULL;
    memory->lock_fd = -1;
    return 0;
}

int __wrap_mister_mailbox_attach(mister_mailbox *mailbox, volatile void *base,
                                 size_t bytes, unsigned timeout_ms) {
    (void)mailbox;
    ++attaches;
    assert(base == mapping && bytes == MISTER_MEMORY_BYTES && timeout_ms == 1000);
    if (attach_error) { errno = attach_error; return -1; }
    return 0;
}

void __wrap_mister_mailbox_detach(mister_mailbox *mailbox) {
    (void)mailbox;
    ++detaches;
}

int __wrap_mister_mailbox_exchange(mister_mailbox *mailbox, const mister_command *commands,
                                   size_t count, uint32_t *results, mister_keys *keys) {
    if (__wrap_mister_mailbox_drain(mailbox)) return -1;
    ++exchanges;
    assert(commands && !keys);
    if (count == 3) {
        assert(results && commands[0].opcode == 2 && commands[0].address == 0);
        assert(commands[1].opcode == 2 && commands[1].address == 0x214);
        assert(commands[2].opcode == 2 && commands[2].address == 0x218);
        if (exchange_error) { errno = exchange_error; return -1; }
        results[0] = renderer_status;
        results[1] = renderer_stride;
        results[2] = renderer_offset;
        return 0;
    }
    assert(count == 1);
    if (commands[0].opcode == 3) {
        if (active >= 0) assert(!memcmp(active_copy, mapping + MISTER_FRAME_OFFSET +
                                       active * MISTER_FRAME_SLOT_BYTES, sizeof(active_copy)));
        if (exchange_error) { errno = exchange_error; return -1; }
        if (!commands[0].value) { ++disables; active = -1; return 0; }
        assert(commands[0].value == (640 | (480 << 12) | (4 << 24) | (1u << 29)));
        if (commands[0].address >= MISTER_RENDERER_BASE) {
            unsigned tiles = (renderer_stride >> 4) & 15;
            assert(commands[0].mask == (tiles ? tiles : 10) * 128);
            uint32_t offset = ((renderer_offset >> 11) & 511) << 12;
            assert(commands[0].address == MISTER_RENDERER_BASE + ((renderer_status & 1024) ? offset : 0));
            if (renderer_publish_error) { errno = ETIMEDOUT; return -1; }
            active = -1;
            ++renderer_frames;
            return 0;
        }
        assert(commands[0].mask == 1280);
        unsigned slot = (commands[0].address - MISTER_FRAME_BASE) / MISTER_FRAME_SLOT_BYTES;
        assert(slot == frames % 3 && active != (int)slot);
        active = (int)slot;
        memcpy(active_copy, mapping + MISTER_FRAME_OFFSET + slot * MISTER_FRAME_SLOT_BYTES,
               sizeof(active_copy));
        for (unsigned y = 0; y < 480; ++y)
            assert(!memcmp(active_copy + y * 1280, source + 1 + y * 1284, 1280));
        ++frames;
        return 0;
    }
    assert(commands[0].opcode == 2);
    if (exchange_error) { errno = exchange_error; return -1; }
    results[0] = 0x42415431;
    return 0;
}

int main(void) {
    volatile uint8_t *renderer = NULL;
    assert(mister_runtime_map_renderer(0, 1, &renderer) == -1 && errno == ENOTCONN && !renderer);
    assert(!mister_runtime_acquire());
    assert(!mister_runtime_map_renderer(4096, 1280 * 480, &renderer));
    assert(renderer == mapping + MISTER_RENDERER_OFFSET + 4096);
    assert(mister_runtime_map_renderer(0, 0, &renderer) == -1 && errno == EINVAL && !renderer);
    assert(mister_runtime_map_renderer(0, SIZE_MAX, &renderer) == -1 && errno == EINVAL);
    assert(mister_runtime_map_renderer(1, 4, &renderer) == -1 && errno == EINVAL);
    assert(mister_runtime_map_renderer(MISTER_RENDERER_BYTES, 1, &renderer) == -1 && errno == EINVAL);
    assert(mister_runtime_map_renderer(MISTER_RENDERER_BYTES - 4096, 4097, &renderer) == -1 && errno == EINVAL);
    assert(mister_runtime_map_renderer(0, 1, NULL) == -1 && errno == EINVAL);
    assert(!mister_runtime_map_renderer(MISTER_RENDERER_BYTES - 4096, 4096, &renderer));
    assert(renderer == mapping + MISTER_RENDERER_OFFSET + MISTER_RENDERER_BYTES - 4096);
    exchange_error = EIO;
    assert(mister_runtime_stop_presenting() == -1 && errno == EIO);
    assert(mister_runtime_map_renderer(0, 1, &renderer) == -1 && errno == EPIPE && !renderer);
    exchange_error = 0;
    assert(!mister_runtime_release());
    opens = closes = attaches = detaches = exchanges = disables = 0;
    const mister_command command = {2, 0x3ff004, 0, 15};
    uint32_t value = 0;
    assert(mister_runtime_exchange(&command, 1, &value, NULL) == -1 && errno == ENOTCONN);
    assert(!mister_runtime_acquire() && !mister_runtime_acquire());
    assert(opens == 1 && attaches == 1);
    assert(!mister_runtime_exchange(&command, 1, &value, NULL) && value == 0x42415431);
    assert(!mister_runtime_release() && !closes && !detaches);
    exchange_error = EPIPE;
    assert(mister_runtime_exchange(&command, 1, &value, NULL) == -1 && errno == EPIPE);
    assert(exchanges == 2);
    close_error = EIO;
    assert(mister_runtime_release() == -1 && errno == EIO && detaches == 1);
    assert(mister_runtime_acquire() == -1 && errno == EBUSY && opens == 1);
    assert(mister_runtime_exchange(&command, 1, &value, NULL) == -1 && errno == ENOTCONN);
    close_error = 0;
    assert(!mister_runtime_release() && detaches == 1);
    open_error = EACCES;
    assert(mister_runtime_acquire() == -1 && errno == EACCES && attaches == 1);
    open_error = 0;
    attach_error = EPROTO;
    close_error = EIO;
    assert(mister_runtime_acquire() == -1 && errno == EPROTO);
    assert(mister_runtime_acquire() == -1 && errno == EBUSY);
    close_error = 0;
    assert(!mister_runtime_release());
    attach_error = 0;
    assert(!mister_runtime_acquire() && !mister_runtime_release());
    assert(opens == 4 && attaches == 3 && detaches == 2 && exchanges == 2);
    assert(mister_runtime_present(source, sizeof(source), 1284) == -1 && errno == ENOTCONN);
    exchange_error = EIO;
    assert(!mister_runtime_acquire());
    assert(mister_runtime_present(source + 1, sizeof(source) - 1, 1284) == -1 && errno == EIO);
    assert(mapping[MISTER_FRAME_OFFSET] == 0);
    exchange_error = 0;
    assert(mister_runtime_present(source + 1, sizeof(source) - 1, 1284) == -1 && errno == EPIPE);
    assert(!mister_runtime_release());
    assert(!mister_runtime_acquire() && !mister_runtime_acquire());
    assert(mister_runtime_present(NULL, sizeof(source), 1284) == -1 && errno == EINVAL);
    assert(mister_runtime_present(source, sizeof(source), 1279) == -1 && errno == EINVAL);
    assert(mister_runtime_present(source, 1280, 1284) == -1 && errno == EINVAL);
    assert(mister_runtime_present(source, SIZE_MAX, SIZE_MAX) == -1 && errno == EINVAL);
    const mister_command forbidden = {3, 0, 0, 0};
    assert(mister_runtime_exchange(&forbidden, 1, NULL, NULL) == -1 && errno == EPERM);
    for (unsigned i = 0; i < 7; ++i) {
        for (size_t j = 0; j < sizeof(source); ++j) source[j] = (uint8_t)(i * 17 + j);
        assert(!mister_runtime_present(source + 1, sizeof(source) - 1, 1284));
    }
    assert(frames == 7 && disables == 1);
    assert(!mister_runtime_release() && disables == 1);
    exchange_error = ETIMEDOUT;
    assert(mister_runtime_present(source + 1, sizeof(source) - 1, 1284) == -1 && errno == ETIMEDOUT);
    const unsigned failed_exchanges = exchanges;
    memset(source, 0, sizeof(source));
    assert(mister_runtime_present(source + 1, sizeof(source) - 1, 1284) == -1 && errno == EPIPE);
    assert(exchanges == failed_exchanges);
    assert(mister_runtime_release() == -1 && errno == ETIMEDOUT);
    exchange_error = 0;
    assert(!mister_runtime_release() && disables == 2);
    assert(mister_runtime_present_renderer() == -1 && errno == ENOTCONN);
    assert(!mister_runtime_acquire());
    for (unsigned i = 0; i < 4; ++i) {
        renderer_status = (i & 1) << 10;
        assert(!mister_runtime_present_renderer());
    }
    assert(renderer_frames == 4);
    frames = 0;
    assert(!mister_runtime_present(source + 1, sizeof(source) - 1, 1284));
    assert(!mister_runtime_present_renderer());
    assert(!mister_runtime_present(source + 1, sizeof(source) - 1, 1284));
    assert(!mister_runtime_stop_presenting());
    assert(!mister_runtime_stop_presenting());
    renderer_stride = 12 << 4;
    renderer_offset = 180 << 11;
    assert(!mister_runtime_present_renderer());
    renderer_offset = 150 << 11;
    renderer_stride = 0;
    assert(!mister_runtime_present_renderer());
    renderer_status = 1u << 28;
    assert(mister_runtime_present_renderer() == -1 && errno == EBUSY);
    renderer_status = 0;
    renderer_stride = 9 << 4;
    assert(mister_runtime_present_renderer() == -1 && errno == EINVAL);
    renderer_stride = 10 << 4;
    renderer_offset = 149 << 11;
    assert(mister_runtime_present_renderer() == -1 && errno == EINVAL);
    renderer_offset = 400 << 11;
    assert(mister_runtime_present_renderer() == -1 && errno == EINVAL);
    renderer_offset = 150 << 11;
    renderer_publish_error = 1;
    assert(mister_runtime_present_renderer() == -1 && errno == ETIMEDOUT);
    assert(mister_runtime_present_renderer() == -1 && errno == EPIPE);
    renderer_publish_error = 0;
    assert(!mister_runtime_release());
    assert(mister_runtime_stop_presenting() == -1 && errno == ENOTCONN);
    assert(!mister_runtime_acquire());
    exchange_error = EIO;
    assert(mister_runtime_present_renderer() == -1 && errno == EIO);
    exchange_error = 0;
    assert(!mister_runtime_present_renderer());
    exchange_error = ETIMEDOUT;
    assert(mister_runtime_stop_presenting() == -1 && errno == ETIMEDOUT);
    assert(mister_runtime_present_renderer() == -1 && errno == EPIPE);
    assert(mister_runtime_release() == -1 && errno == ETIMEDOUT);
    exchange_error = 0;
    assert(!mister_runtime_stop_presenting());
    assert(!mister_runtime_release());
    assert(!mister_runtime_acquire());
    unsigned previous_disables = disables;
    assert(!mister_runtime_stop_presenting() && disables == previous_disables + 1);
    assert(!mister_runtime_release());
    assert(!mister_runtime_acquire());
    exchange_error = ETIMEDOUT;
    assert(mister_runtime_stop_presenting() == -1 && errno == ETIMEDOUT);
    unsigned previous_closes = closes;
    assert(mister_runtime_release() == -1 && errno == ETIMEDOUT && closes == previous_closes);
    exchange_error = 0;
    assert(!mister_runtime_release());
    const mister_command write = {1, 0x80, 1, 15};
    assert(mister_runtime_submit_writes(&write, 1) == -1 && errno == ENOTCONN && !submissions);
    assert(!mister_runtime_acquire() && !mister_runtime_acquire());
    assert(!mister_runtime_submit_writes(&write, 1) && async_mailbox->pending);
    assert(!mister_runtime_exchange(&command, 1, &value, NULL) && !async_mailbox->pending);
    assert(!mister_runtime_submit_writes(&write, 1));
    assert(!mister_runtime_map_renderer(0, 1, &renderer) && !async_mailbox->pending);
    assert(!mister_runtime_submit_writes(&write, 1));
    assert(!mister_runtime_present_renderer() && !async_mailbox->pending);
    assert(!mister_runtime_submit_writes(&write, 1));
    assert(!mister_runtime_stop_presenting() && !async_mailbox->pending);
    assert(!mister_runtime_submit_writes(&write, 1));
    unsigned before_drains = drains;
    assert(!mister_runtime_release() && drains == before_drains + 1 && !async_mailbox->pending);
    submit_error = EIO;
    assert(mister_runtime_submit_writes(&write, 1) == -1 && errno == EIO);
    submit_error = 0;
    assert(!mister_runtime_submit_writes(&write, 1));
    drain_error = ETIMEDOUT;
    assert(mister_runtime_map_renderer(0, 1, &renderer) == -1 && errno == ETIMEDOUT && !renderer);
    previous_closes = closes;
    unsigned previous_detaches = detaches;
    assert(mister_runtime_release() == -1 && errno == ETIMEDOUT);
    assert(closes == previous_closes && detaches == previous_detaches && async_mailbox->pending);
    drain_error = 0;
    assert(!mister_runtime_release() && closes == previous_closes + 1 && !async_mailbox->pending);
    puts("mister_runtime_test: PASS");
    return 0;
}
