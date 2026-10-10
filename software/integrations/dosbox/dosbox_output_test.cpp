#include "mister_runtime.h"
#include <SDL.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <initializer_list>

int mister_dosbox_present(SDL_Surface *, const SDL_Rect *);
static unsigned frames;
static int failure;

extern "C" int __wrap_mister_runtime_present(const void *data, size_t bytes, size_t pitch) {
    assert(bytes == 640 * 480 * 2 && pitch == 1280);
    const auto *pixels = static_cast<const uint16_t *>(data);
    const uint16_t colors[] = {0xf800, 0x07e0, 0x001f, 0xffff};
    for (unsigned y = 0; y < 480; ++y)
        for (unsigned x = 0; x < 640; ++x) {
            if (pixels[y * 640 + x] != colors[(y / 240) * 2 + x / 320])
                std::fprintf(stderr, "frame %u pixel %u,%u: %04x expected %04x\n", frames, x, y,
                             pixels[y * 640 + x], colors[(y / 240) * 2 + x / 320]);
            assert(pixels[y * 640 + x] == colors[(y / 240) * 2 + x / 320]);
        }
    ++frames;
    if (failure) { errno = failure; return -1; }
    return 0;
}

int main() {
    assert(SDL_Init(0) == 0);
    for (const Uint32 format : {SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_RGB888, SDL_PIXELFORMAT_ARGB8888}) {
        SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 4, 4, SDL_BITSPERPIXEL(format), format);
        assert(surface);
        const Uint8 colors[][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}};
        for (int i = 0; i < 4; ++i) {
            SDL_Rect pixel = {1 + i % 2, 1 + i / 2, 1, 1};
            assert(!SDL_FillRect(surface, &pixel, SDL_MapRGB(surface->format, colors[i][0], colors[i][1], colors[i][2])));
        }
        SDL_Rect clip = {1, 1, 2, 2};
        assert(!mister_dosbox_present(surface, &clip));
        failure = ETIMEDOUT;
        assert(mister_dosbox_present(surface, &clip) == -1 && errno == ETIMEDOUT);
        failure = 0;
        assert(mister_dosbox_present(nullptr, &clip) == -1 && errno == EINVAL);
        assert(mister_dosbox_present(surface, nullptr) == -1 && errno == EINVAL);
        for (const SDL_Rect invalid : {SDL_Rect{-1, 0, 1, 1}, SDL_Rect{0, 0, 0, 1},
                                      SDL_Rect{4, 0, 1, 1}, SDL_Rect{0, 3, 1, 2}})
            assert(mister_dosbox_present(surface, &invalid) == -1 && errno == EINVAL);
        SDL_FreeSurface(surface);
    }
    assert(frames == 6);
    SDL_Quit();
    std::puts("dosbox_output_test: PASS");
}
