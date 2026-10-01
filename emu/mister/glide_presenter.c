#include "mister_runtime.h"
#include "glide.h"
#include "../sim/harness/sim_harness.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned frame_width, frame_height;
static FxU16 source[640 * 480], scaled[640 * 480];

static void fail(void) {
    perror("MiSTer Glide presentation");
    exit(EXIT_FAILURE);
}

void _grX11PresenterInit(FxU32 window, FxU32 width, FxU32 height) {
    (void)window;
    if (!width || width > 640 || !height || height > 480) { errno = EINVAL; fail(); }
    frame_width = width;
    frame_height = height;
}

void _grX11PresenterSwap(void) {
    if (!frame_width) return;
    if (frame_width == 640 && frame_height == 480) {
        sim_idle_wait();
        if (mister_runtime_present_renderer()) fail();
        return;
    }
    GrLfbInfo_t info = {.size = sizeof(info)};
    if (!grLfbLock(GR_LFB_READ_ONLY, GR_BUFFER_FRONTBUFFER, GR_LFBWRITEMODE_ANY,
                   GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) { errno = EIO; fail(); }
    sim_idle_wait();
    const unsigned row_words = (frame_width + 1) / 2;
    const unsigned total = row_words * frame_height;
    for (unsigned first = 0; first < total; first += MISTER_MAILBOX_MAX_COMMANDS) {
        mister_command commands[MISTER_MAILBOX_MAX_COMMANDS];
        uint32_t results[MISTER_MAILBOX_MAX_COMMANDS];
        unsigned count = total - first;
        if (count > MISTER_MAILBOX_MAX_COMMANDS) count = MISTER_MAILBOX_MAX_COMMANDS;
        for (unsigned i = 0; i < count; ++i) {
            unsigned word = first + i;
            commands[i] = (mister_command){2, 0x400000 + (word / row_words) * 2048 + (word % row_words) * 4, 0, 15};
        }
        if (mister_runtime_exchange(commands, count, results, NULL)) fail();
        for (unsigned i = 0; i < count; ++i) {
            unsigned y = (first + i) / row_words, x = ((first + i) % row_words) * 2;
            source[y * frame_width + x] = (FxU16)results[i];
            if (x + 1 < frame_width) source[y * frame_width + x + 1] = (FxU16)(results[i] >> 16);
        }
    }
    if (!grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_FRONTBUFFER)) { errno = EIO; fail(); }
    sim_idle_wait();
    for (unsigned y = 0; y < 480; ++y)
        for (unsigned x = 0; x < 640; ++x)
            scaled[y * 640 + x] = source[(y * frame_height / 480) * frame_width + x * frame_width / 640];
    if (mister_runtime_present(scaled, sizeof(scaled), 1280)) fail();
}

void _grX11PresenterShutdown(void) {
    if (frame_width == 640 && frame_height == 480 && mister_runtime_stop_presenting()) fail();
    frame_width = frame_height = 0;
}
