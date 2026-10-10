#include "mister_runtime.h"
#include <SDL.h>
#include <cerrno>

int mister_dosbox_present(SDL_Surface *surface, const SDL_Rect *clip) {
    if (!surface || !clip || !surface->pixels || clip->x < 0 || clip->y < 0 ||
        clip->w <= 0 || clip->h <= 0 || clip->x > surface->w || clip->y > surface->h ||
        clip->w > surface->w - clip->x || clip->h > surface->h - clip->y) {
        errno = EINVAL;
        return -1;
    }
    SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB565, 0);
    if (!converted) { errno = ENOMEM; return -1; }
    uint16_t pixels[640 * 480];
    for (unsigned y = 0; y < 480; ++y) {
        const auto *row = reinterpret_cast<const uint16_t *>(static_cast<const uint8_t *>(converted->pixels) +
                          (clip->y + uint64_t(y) * clip->h / 480) * converted->pitch);
        for (unsigned x = 0; x < 640; ++x)
            pixels[y * 640 + x] = row[clip->x + uint64_t(x) * clip->w / 640];
    }
    SDL_FreeSurface(converted);
    return mister_runtime_present(pixels, sizeof(pixels), 1280);
}
