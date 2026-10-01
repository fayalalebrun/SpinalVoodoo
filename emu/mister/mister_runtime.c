#include "mister_runtime.h"
#include "mister_memory.h"
#include <errno.h>
#include <string.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static mister_memory memory = MISTER_MEMORY_INIT;
static mister_mailbox mailbox;
static uint32_t clients;
static unsigned next_slot;
static int frames_ready, frames_faulted;
static uint32_t parameters[30], parameters_valid;
static uint32_t palettes[32][256], palette_valid;
static unsigned palette_order[32];
static int palette_banks = -1, palette_bank;

static void palette_observe(mister_command command) {
    if (command.opcode != 1 || command.address >= 0x400000) return;
    unsigned address = command.address & 0x3ff;
    if (command.address == 0x3ec) {
        palette_valid = 0;
        palette_bank = command.mask == 15 && command.value < 32 ? (int)command.value : -1;
    } else if (address >= 0x334 && address <= 0x350 && !(address & 3) &&
               (command.value & UINT32_C(0x80000000))) {
        if (palette_bank < 0) palette_valid = 0;
        else palette_valid &= ~(UINT32_C(1) << palette_bank);
    }
}

static int disable_frame(void) {
    const mister_command command = {3, 0, 0, 0};
    return mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL);
}

static int finish(int error) {
    pthread_mutex_unlock(&lock);
    if (error) errno = error;
    return error ? -1 : 0;
}

int mister_runtime_acquire(void) {
    int error = pthread_mutex_lock(&lock);
    if (error) { errno = error; return -1; }
    if (clients == UINT32_MAX) return finish(EOVERFLOW);
    if (clients) {
        ++clients;
        return finish(0);
    }
    if (memory.base || memory.lock_fd >= 0) return finish(EBUSY);
    if (mister_memory_open(&memory)) {
        error = errno;
        mister_memory_close(&memory);
        return finish(error);
    }
    if (mister_mailbox_attach(&mailbox, memory.base, MISTER_MEMORY_BYTES, 1000)) {
        error = errno;
        mister_memory_close(&memory);
        return finish(error);
    }
    clients = 1;
    next_slot = 0;
    frames_ready = frames_faulted = 0;
    parameters_valid = palette_valid = 0;
    palette_banks = -1;
    palette_bank = 0;
    for (unsigned i = 0; i < 32; ++i) palette_order[i] = i;
    return finish(0);
}

int mister_runtime_release(void) {
    int error = pthread_mutex_lock(&lock);
    if (error) { errno = error; return -1; }
    if (clients && mailbox.pending && mister_mailbox_drain(&mailbox)) return finish(errno);
    if (clients > 1) {
        --clients;
        return finish(0);
    }
    if (clients) {
        if (frames_ready && disable_frame()) return finish(errno);
        mister_mailbox_detach(&mailbox);
        clients = 0;
    }
    error = mister_memory_close(&memory) ? errno : 0;
    return finish(error);
}

int mister_runtime_map_renderer(uint32_t offset, size_t bytes, volatile uint8_t **base) {
    if (!base) { errno = EINVAL; return -1; }
    *base = NULL;
    if ((offset & 4095) || offset >= MISTER_RENDERER_BYTES || !bytes ||
        bytes > MISTER_RENDERER_BYTES - offset) { errno = EINVAL; return -1; }
    int error = pthread_mutex_lock(&lock);
    if (error) { errno = error; return -1; }
    if (!clients) return finish(ENOTCONN);
    if (frames_faulted || mailbox.faulted) return finish(EPIPE);
    if (mister_mailbox_drain(&mailbox)) return finish(errno);
    *base = memory.base + MISTER_RENDERER_OFFSET + offset;
    return finish(0);
}

int mister_runtime_present(const void *rgb565, size_t bytes, size_t pitch) {
    if (!rgb565 || pitch < 1280 || pitch > (SIZE_MAX - 1280) / 479 ||
        bytes < pitch * 479 + 1280) { errno = EINVAL; return -1; }
    int error = pthread_mutex_lock(&lock);
    if (error) { errno = error; return -1; }
    if (!clients) return finish(ENOTCONN);
    if (frames_faulted || mailbox.faulted) return finish(EPIPE);
    if (!frames_ready) {
        if (disable_frame()) { frames_faulted = 1; return finish(errno); }
        frames_ready = 1;
    }
    volatile uint16_t *destination = (volatile uint16_t *)(memory.base + MISTER_FRAME_OFFSET +
                                                        next_slot * MISTER_FRAME_SLOT_BYTES);
    const uint8_t *source = rgb565;
    for (unsigned y = 0; y < 480; ++y) {
        for (unsigned x = 0; x < 640; ++x) {
            uint16_t pixel;
            memcpy(&pixel, source + y * pitch + x * 2, sizeof(pixel));
            destination[y * 640 + x] = pixel;
        }
    }
    const mister_command command = {3, MISTER_FRAME_BASE + next_slot * MISTER_FRAME_SLOT_BYTES,
                                    640 | (480 << 12) | (4 << 24) | (1u << 29), 1280};
    if (mister_mailbox_exchange(&mailbox, &command, 1, NULL, NULL)) {
        frames_faulted = 1;
        return finish(errno);
    }
    next_slot = (next_slot + 1) % 3;
    return finish(0);
}

int mister_runtime_present_renderer(void) {
    int error = pthread_mutex_lock(&lock);
    if (error) { errno = error; return -1; }
    if (!clients) return finish(ENOTCONN);
    if (frames_faulted || mailbox.faulted) return finish(EPIPE);
    const mister_command registers[] = {{2, 0, 0, 15}, {2, 0x214, 0, 15}, {2, 0x218, 0, 15}};
    uint32_t values[3];
    if (mister_mailbox_exchange(&mailbox, registers, 3, values, NULL)) return finish(errno);
    if (values[0] & UINT32_C(0xf0000000)) return finish(EBUSY);
    unsigned tiles = (values[1] >> 4) & 15;
    unsigned stride = (tiles ? tiles : 10) * 128;
    unsigned offset = ((values[2] >> 11) & 511) * 4096;
    unsigned extent = 479 * stride + 1280;
    if (stride < 1280 || offset < extent || offset * 3 > MISTER_RENDERER_BYTES) return finish(EINVAL);
    if (!frames_ready) {
        if (disable_frame()) { frames_faulted = 1; return finish(errno); }
        frames_ready = 1;
    }
    const mister_command frame = {3, MISTER_RENDERER_BASE + ((values[0] & 1024) ? offset : 0),
                                  640 | (480 << 12) | (4 << 24) | (1u << 29), stride};
    if (mister_mailbox_exchange(&mailbox, &frame, 1, NULL, NULL)) {
        frames_faulted = 1;
        return finish(errno);
    }
    return finish(0);
}

int mister_runtime_stop_presenting(void) {
    int error = pthread_mutex_lock(&lock);
    if (error) { errno = error; return -1; }
    if (!clients) return finish(ENOTCONN);
    frames_ready = 1;
    if (disable_frame()) { frames_faulted = 1; return finish(errno); }
    frames_ready = 0;
    return finish(0);
}

int mister_runtime_submit_writes(const mister_command *commands, size_t count) {
    int error = pthread_mutex_lock(&lock);
    if (error) { errno = error; return -1; }
    if (!clients) return finish(ENOTCONN);
    if (!commands || !count || count > MISTER_MAILBOX_MAX_COMMANDS) return finish(EINVAL);
    mister_command compact[MISTER_MAILBOX_MAX_COMMANDS];
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        mister_command command = commands[i];
        palette_observe(command);
        if (command.opcode == 1 && command.mask == 15 && !(command.address & 3) &&
            command.address >= 0x88 && command.address <= 0xfc) {
            unsigned index = (command.address - 0x88) / 4;
            uint32_t bit = UINT32_C(1) << index;
            if ((parameters_valid & bit) && parameters[index] == command.value) continue;
            parameters[index] = command.value;
            parameters_valid |= bit;
        } else if (command.opcode != 1 || command.mask != 15 ||
                   (command.address < 0x400000 && command.address != 0x80 && command.address != 0x100)) {
            parameters_valid = 0;
        }
        compact[used++] = command;
    }
    error = (used ? mister_mailbox_submit_writes(&mailbox, compact, used) :
                    mister_mailbox_drain(&mailbox)) ? errno : 0;
    if (error) parameters_valid = palette_valid = 0;
    return finish(error);
}

int mister_runtime_upload_palette(const uint32_t *data, unsigned start, unsigned end) {
    if (!data || start > end || end > 255) { errno = EINVAL; return -1; }
    int error = pthread_mutex_lock(&lock);
    if (error) { errno = error; return -1; }
    if (!clients) return finish(ENOTCONN);
    if (frames_faulted || mailbox.faulted) return finish(EPIPE);
    parameters_valid = 0;
    if (palette_banks < 0) {
        const mister_command query = {2, 0x3ec, 0, 15};
        uint32_t capability;
        if (mister_mailbox_exchange(&mailbox, &query, 1, &capability, NULL)) return finish(errno);
        palette_banks = capability == UINT32_C(0x50414c20) ? 32 :
                        capability == UINT32_C(0x50414c10) ? 16 : 0;
    }
    uint32_t colors[256];
    for (unsigned i = start; i <= end; ++i) colors[i] = data[i] & 0xffffff;
    unsigned distance = 32, slot = 0;
    int cached = palette_banks && start == 0 && end == 255;
    if (cached) {
        for (unsigned i = 0; i < (unsigned)palette_banks; ++i) {
            unsigned bank = palette_order[i];
            if ((palette_valid & (UINT32_C(1) << bank)) &&
                !memcmp(colors, palettes[bank], sizeof(colors))) { distance = i; break; }
        }
    }
    int hit = distance < 32;
    if (cached) {
        if (!hit) {
            for (distance = 0; distance + 1 < (unsigned)palette_banks; ++distance)
                if (!(palette_valid & (UINT32_C(1) << palette_order[distance]))) break;
        }
        slot = palette_order[distance];
        const mister_command select = {1, 0x3ec, slot, 15};
        if (mister_mailbox_submit_writes(&mailbox, &select, 1)) {
            palette_valid = 0;
            return finish(errno);
        }
        palette_bank = (int)slot;
    }
    if (!hit) {
        mister_command commands[256];
        for (unsigned i = start; i <= end; ++i) {
            commands[i - start] = (mister_command){1, 0x334 + (i & 7) * 4,
                UINT32_C(0x80000000) | ((i & 254) << 23) | colors[i], 15};
            palette_observe(commands[i - start]);
        }
        if (mister_mailbox_submit_writes(&mailbox, commands, end - start + 1)) {
            palette_valid = parameters_valid = 0;
            return finish(errno);
        }
    }
    if (cached) {
        memcpy(palettes[slot], colors, sizeof(colors));
        palette_valid |= UINT32_C(1) << slot;
        memmove(palette_order + 1, palette_order, distance * sizeof(*palette_order));
        palette_order[0] = slot;
    }
    parameters_valid = 0;
    return finish(0);
}

int mister_runtime_exchange(const mister_command *commands, size_t count,
                           uint32_t *results, mister_keys *keys) {
    int error = pthread_mutex_lock(&lock);
    if (error) { errno = error; return -1; }
    if (!clients) return finish(ENOTCONN);
    if (commands && count <= MISTER_MAILBOX_MAX_COMMANDS)
        for (size_t i = 0; i < count; ++i) {
            if (commands[i].opcode == 3) return finish(EPERM);
            if (commands[i].opcode == 1) parameters_valid = 0;
            palette_observe(commands[i]);
        }
    error = mister_mailbox_exchange(&mailbox, commands, count, results, keys) ? errno : 0;
    if (error) palette_valid = 0;
    return finish(error);
}
