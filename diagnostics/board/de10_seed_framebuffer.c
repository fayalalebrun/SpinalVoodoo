#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FB_PHYS_BASE 0x3f000000u
#define FB_WIDTH 720u
#define FB_HEIGHT 480u
#define FB_STRIDE_PIXELS 768u

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((uint16_t)(r >> 3) << 11) |
                      ((uint16_t)(g >> 2) << 5) |
                      ((uint16_t)(b >> 3)));
}

int main(int argc, char **argv) {
    uint32_t base = FB_PHYS_BASE;
    unsigned mode = 0;
    uint32_t width = FB_WIDTH;
    uint32_t stride_pixels = FB_STRIDE_PIXELS;
    if (argc > 5) {
        fprintf(stderr, "usage: %s [physical_base_hex] [mode] [active_width] [stride_pixels]\n", argv[0]);
        return 2;
    }
    if (argc >= 2) {
        char *end = NULL;
        unsigned long value = strtoul(argv[1], &end, 0);
        if (!end || *end != '\0' || value > UINT32_MAX) {
            fprintf(stderr, "invalid physical base: %s\n", argv[1]);
            return 2;
        }
        base = (uint32_t)value;
    }
    if (argc >= 3) {
        char *end = NULL;
        unsigned long value = strtoul(argv[2], &end, 0);
        if (!end || *end != '\0' || value > 2) {
            fprintf(stderr, "invalid mode: %s\n", argv[2]);
            return 2;
        }
        mode = (unsigned)value;
    }
    if (argc >= 4) {
        char *end = NULL;
        unsigned long value = strtoul(argv[3], &end, 0);
        if (!end || *end != '\0' || value == 0 || value > 2048) {
            fprintf(stderr, "invalid active width: %s\n", argv[3]);
            return 2;
        }
        width = (uint32_t)value;
    }
    if (argc >= 5) {
        char *end = NULL;
        unsigned long value = strtoul(argv[4], &end, 0);
        if (!end || *end != '\0' || value == 0 || value > 4096 || value < width) {
            fprintf(stderr, "invalid stride pixels: %s\n", argv[4]);
            return 2;
        }
        stride_pixels = (uint32_t)value;
    }

    const size_t bytes = stride_pixels * FB_HEIGHT * sizeof(uint16_t);
    const long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) {
        perror("sysconf(_SC_PAGESIZE)");
        return 1;
    }

    const off_t page_mask = (off_t)page - 1;
    const off_t map_base = (off_t)base & ~page_mask;
    const off_t page_offset = (off_t)base - map_base;
    const size_t map_bytes = (page_offset + bytes + (size_t)page - 1) & ~((size_t)page - 1);

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open(/dev/mem)");
        return 1;
    }

    void *map = mmap(NULL, map_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_base);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    volatile uint16_t *fb = (volatile uint16_t *)((uint8_t *)map + page_offset);
    for (uint32_t y = 0; y < FB_HEIGHT; ++y) {
        for (uint32_t x = 0; x < stride_pixels; ++x) {
            uint8_t r = 0, g = 0, b = 0;
            if (x < width) {
                const uint32_t tile = ((x >> 4) ^ (y >> 4)) & 1u;
                if (mode == 2) {
                    /* Geometry probe: unmistakable top/bottom/left/right markers plus row gradient. */
                    r = (uint8_t)((y * 255u) / (FB_HEIGHT - 1u));
                    g = (uint8_t)((x * 255u) / (width - 1u));
                    b = (uint8_t)(((x >> 4) ^ (y >> 4)) ? 255u : 0u);
                    if (y < 32) { r = 255; g = 0; b = 0; }
                    if (y >= FB_HEIGHT - 32) { r = 0; g = 255; b = 0; }
                    if (x < 32) { r = 0; g = 0; b = 255; }
                    if (x >= width - 32) { r = 255; g = 255; b = 0; }
                } else if (mode == 1) {
                    const uint32_t ring = ((x + y) >> 3) & 3u;
                    r = (uint8_t)(x * 11u + y * 5u + ring * 37u);
                    g = (uint8_t)(255u - x * 3u + y * 13u);
                    b = (uint8_t)((x ^ (y << 1)) + ring * 61u);
                } else if (tile) {
                    r = (uint8_t)(x * 5u + y * 3u);
                    g = (uint8_t)(x ^ (y * 7u));
                    b = (uint8_t)(255u - y * 5u + x);
                } else {
                    r = (uint8_t)(255u - x * 7u + y);
                    g = (uint8_t)(y * 9u + x * 2u);
                    b = (uint8_t)(x * 3u - y * 11u);
                }
            }
            fb[y * stride_pixels + x] = rgb565(r, g, b);
        }
    }

    /* O_SYNC /dev/mem mappings on this kernel make the writes visible without msync. */
    munmap(map, map_bytes);
    close(fd);

    printf("seeded %ux%u RGB565 framebuffer mode %u at 0x%08x with stride %u pixels (%zu bytes)\n",
           width, FB_HEIGHT, mode, base, stride_pixels, bytes);
    return 0;
}
