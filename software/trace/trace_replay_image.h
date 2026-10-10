#ifndef TRACE_REPLAY_IMAGE_H
#define TRACE_REPLAY_IMAGE_H

#include <cmath>
#include <cstdint>
#include <vector>

static inline void traceReplayInitSrgbLut(uint8_t lut[256]) {
    for (int i = 0; i < 256; ++i) {
        double v = i / 255.0;
        double s = (v <= 0.0031308)
            ? v * 12.92
            : 1.055 * pow(v, 1.0 / 2.4) - 0.055;
        lut[i] = static_cast<uint8_t>(s * 255.0 + 0.5);
    }
}

static inline void traceReplayRgb565ToRgb24(const uint16_t *src,
                                            uint8_t *dst,
                                            int count,
                                            const uint8_t *srgbLut) {
    for (int i = 0; i < count; ++i) {
        uint16_t px = src[i];
        uint8_t r = (px >> 8) & 0xf8;
        uint8_t g = (px >> 3) & 0xfc;
        uint8_t b = (px << 3) & 0xf8;
        r |= r >> 5;
        g |= g >> 6;
        b |= b >> 5;
        if (srgbLut) {
            dst[i * 3 + 0] = srgbLut[r];
            dst[i * 3 + 1] = srgbLut[g];
            dst[i * 3 + 2] = srgbLut[b];
        } else {
            dst[i * 3 + 0] = r;
            dst[i * 3 + 1] = g;
            dst[i * 3 + 2] = b;
        }
    }
}

static inline bool traceReplayWriteRgb565Png(const char *path,
                                             const uint16_t *src,
                                             int width,
                                             int height,
                                             int rowStridePixels,
                                             const uint8_t *srgbLut,
                                             int (*writePng)(const char *, int, int, int, const void *, int)) {
    std::vector<uint8_t> rgb(width * height * 3);
    for (int y = 0; y < height; ++y) {
        traceReplayRgb565ToRgb24(src + y * rowStridePixels, rgb.data() + y * width * 3, width, srgbLut);
    }
    return writePng(path, width, height, 3, rgb.data(), width * 3) != 0;
}

#endif
