#include "mister_runtime.h"
#include "glide.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void _grX11PresenterInit(FxU32, FxU32, FxU32);
void _grX11PresenterSwap(void);
void _grX11PresenterShutdown(void);
static unsigned width, height, reads, frames;
static int read_error, present_error, batch_error, unlock_error;
static int direct_error, stop_error;
static unsigned direct_frames, stops;

int __wrap_mister_runtime_present_renderer(void) {
    assert(width == 640 && height == 480);
    ++direct_frames;
    if (direct_error) { errno = ETIMEDOUT; return -1; }
    return 0;
}

int __wrap_mister_runtime_stop_presenting(void) {
    ++stops;
    if (stop_error) { errno = ETIMEDOUT; return -1; }
    return 0;
}

FxBool grLfbLock(GrLock_t type, GrBuffer_t buffer, GrLfbWriteMode_t mode,
                 GrOriginLocation_t origin, FxBool pipeline, GrLfbInfo_t *info) {
    assert(type == GR_LFB_READ_ONLY && buffer == GR_BUFFER_FRONTBUFFER && mode == GR_LFBWRITEMODE_ANY);
    assert(origin == GR_ORIGIN_UPPER_LEFT && !pipeline && info->size == sizeof(*info));
    ++reads;
    return !read_error;
}

FxBool grLfbUnlock(GrLock_t type, GrBuffer_t buffer) {
    assert(type == GR_LFB_READ_ONLY && buffer == GR_BUFFER_FRONTBUFFER);
    return !unlock_error;
}

uint32_t sim_idle_wait(void) { return 0; }

int __wrap_mister_runtime_exchange(const mister_command *commands, size_t count,
                                    uint32_t *results, mister_keys *keys) {
    assert(count && count <= MISTER_MAILBOX_MAX_COMMANDS && results && !keys);
    if (batch_error) { errno = ETIMEDOUT; return -1; }
    for (size_t i = 0; i < count; ++i) {
        const mister_command *command = commands + i;
        assert(command->opcode == 2 && command->mask == 15 && !command->value);
        assert(command->address >= 0x400000 && !(command->address & 3));
        unsigned offset = command->address - 0x400000;
        unsigned y = offset / 2048, x = (offset % 2048) / 2;
        assert(y < height && x < width);
        results[i] = (uint16_t)(y * width + x) | (uint32_t)(uint16_t)(y * width + x + 1) << 16;
    }
    return 0;
}

int __wrap_mister_runtime_present(const void *data, size_t bytes, size_t pitch) {
    assert(bytes == 640 * 480 * 2 && pitch == 1280);
    const FxU16 *pixels = data;
    for (unsigned y = 0; y < 480; ++y)
        for (unsigned x = 0; x < 640; ++x)
            assert(pixels[y * 640 + x] == (FxU16)((y * height / 480) * width + x * width / 640));
    ++frames;
    if (present_error) { errno = ETIMEDOUT; return -1; }
    return 0;
}

int main(void) {
    _grX11PresenterSwap();
    assert(!reads && !frames);
    for (unsigned i = 1; i <= 2; ++i) {
        width = 320 * i;
        height = 240 * i;
        unsigned before = direct_frames;
        _grX11PresenterInit(0, 640, 480);
        width = 640;
        height = 480;
        _grX11PresenterSwap();
        assert(direct_frames == before + 1);
        _grX11PresenterShutdown();
        width = 320 * i;
        height = 240 * i;
        _grX11PresenterInit(0, width == 640 ? 639 : width, height);
        if (width == 640) width = 639;
        _grX11PresenterSwap();
    }
    assert(reads == 2 && frames == 2);
    assert(direct_frames == 2 && stops == 2);
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        pid_t child = fork();
        assert(child >= 0);
        if (!child) {
            if (scenario == 0) _grX11PresenterInit(0, 0, 480);
            if (scenario == 1) _grX11PresenterInit(0, 641, 480);
            if (scenario == 2) read_error = 1;
            if (scenario == 3) present_error = 1;
            if (scenario == 4) batch_error = 1;
            if (scenario == 5) unlock_error = 1;
            if (scenario == 6) {
                width = 640; height = 480;
                _grX11PresenterInit(0, width, height);
                direct_error = 1;
            }
            if (scenario == 7) {
                _grX11PresenterInit(0, 640, 480);
                stop_error = 1;
                _grX11PresenterShutdown();
            }
            _grX11PresenterSwap();
            _exit(0);
        }
        int status;
        assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE);
    }
    _grX11PresenterShutdown();
    _grX11PresenterSwap();
    assert(reads == 2 && frames == 2);
    puts("glide_presenter_test: PASS");
    return 0;
}
