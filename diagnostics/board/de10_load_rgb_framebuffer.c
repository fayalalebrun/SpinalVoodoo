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
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s raw_rgb_720x480 [physical_base_hex]\n", argv[0]);
        return 2;
    }

    uint32_t base = FB_PHYS_BASE;
    if (argc == 3) {
        char *end = NULL;
        unsigned long value = strtoul(argv[2], &end, 0);
        if (!end || *end != '\0' || value > UINT32_MAX) {
            fprintf(stderr, "invalid physical base: %s\n", argv[2]);
            return 2;
        }
        base = (uint32_t)value;
    }

    const size_t input_bytes = FB_WIDTH * FB_HEIGHT * 3u;
    const size_t fb_bytes = FB_STRIDE_PIXELS * FB_HEIGHT * sizeof(uint16_t);

    int img_fd = open(argv[1], O_RDONLY);
    if (img_fd < 0) {
        perror("open(image)");
        return 1;
    }

    uint8_t *image = malloc(input_bytes);
    if (!image) {
        perror("malloc");
        close(img_fd);
        return 1;
    }

    size_t offset = 0;
    while (offset < input_bytes) {
        ssize_t got = read(img_fd, image + offset, input_bytes - offset);
        if (got < 0) {
            if (errno == EINTR) continue;
            perror("read(image)");
            free(image);
            close(img_fd);
            return 1;
        }
        if (got == 0) {
            fprintf(stderr, "short image: expected %zu bytes, got %zu\n", input_bytes, offset);
            free(image);
            close(img_fd);
            return 1;
        }
        offset += (size_t)got;
    }
    close(img_fd);

    const long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) {
        perror("sysconf(_SC_PAGESIZE)");
        free(image);
        return 1;
    }

    const off_t page_mask = (off_t)page - 1;
    const off_t map_base = (off_t)base & ~page_mask;
    const off_t page_offset = (off_t)base - map_base;
    const size_t map_bytes = (page_offset + fb_bytes + (size_t)page - 1) & ~((size_t)page - 1);

    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("open(/dev/mem)");
        free(image);
        return 1;
    }

    void *map = mmap(NULL, map_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, map_base);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(mem_fd);
        free(image);
        return 1;
    }

    volatile uint16_t *fb = (volatile uint16_t *)((uint8_t *)map + page_offset);
    for (uint32_t y = 0; y < FB_HEIGHT; ++y) {
        for (uint32_t x = 0; x < FB_STRIDE_PIXELS; ++x) {
            uint16_t pixel = 0;
            if (x < FB_WIDTH) {
                const size_t i = ((size_t)y * FB_WIDTH + x) * 3u;
                pixel = rgb565(image[i], image[i + 1], image[i + 2]);
            }
            fb[y * FB_STRIDE_PIXELS + x] = pixel;
        }
    }

    munmap(map, map_bytes);
    close(mem_fd);
    free(image);

    printf("loaded %ux%u raw RGB image into RGB565 framebuffer at 0x%08x with stride %u pixels (%zu bytes)\n",
           FB_WIDTH, FB_HEIGHT, base, FB_STRIDE_PIXELS, fb_bytes);
    return 0;
}
