#include "mister_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned clients, frames, polls, nonzero;

int mister_runtime_acquire(void) {
    ++clients;
    return 0;
}

int mister_runtime_release(void) {
    assert(clients);
    if (!--clients) {
        printf("MISTER_DOSBOX_SMOKE frames=%u polls=%u nonzero=%u\n", frames, polls, nonzero);
        if (!frames || !polls || !nonzero) _Exit(3);
    }
    return 0;
}

int mister_runtime_exchange(const mister_command *commands, size_t count,
                            uint32_t *results, mister_keys *keys) {
    assert(clients && commands && count == 1 && commands[0].opcode == 4);
    assert(!commands[0].address && !commands[0].value && !commands[0].mask && !results && keys);
    memset(keys, 0, sizeof(*keys));
    ++polls;
    return 0;
}

int mister_runtime_present(const void *data, size_t bytes, size_t pitch) {
    assert(clients && data && bytes == 640 * 480 * 2 && pitch == 1280);
    const uint16_t *pixels = data;
    for (unsigned i = 0; i < 640 * 480; ++i) nonzero |= pixels[i];
    ++frames;
    return 0;
}
