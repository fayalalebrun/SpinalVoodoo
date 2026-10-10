#define _POSIX_C_SOURCE 200809L
#include "glide.h"
#include "mister_runtime.h"
#include "../../simulation/xls/harness/sim_harness.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double seconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now)) { perror("clock_gettime"); exit(EXIT_FAILURE); }
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
}

static void draw(unsigned frame) {
    float x = 40.0f + (float)((frame * 7) % 360);
    GrVertex a = {0}, b = {0}, c = {0};
    a.x = x; a.y = 100; a.oow = 1;
    b.x = x + 180; b.y = 100; b.oow = 1;
    c.x = x; c.y = 350; c.oow = 1;
    grBufferClear(0xff0000ff, 255, 0xffff);
    grConstantColorValue(0xff00ff00);
    grDrawTriangle(&a, &b, &c);
    grBufferSwap(0);
}

static int verify(unsigned frame) {
    GrLfbInfo_t info = {.size = sizeof(info)};
    if (!grLfbLock(GR_LFB_READ_ONLY, GR_BUFFER_FRONTBUFFER, GR_LFBWRITEMODE_ANY,
                   GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) return -1;
    sim_idle_wait();
    unsigned x = 60 + (frame * 7) % 360;
    mister_command commands[] = {{2, 0x400000 + 120 * 2048 + (x / 2) * 4, 0, 15},
                                  {2, 0x400000 + 450 * 2048 + 600 * 2, 0, 15}};
    uint32_t results[2];
    int result = mister_runtime_exchange(commands, 2, results, NULL);
    if (!grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_FRONTBUFFER)) return -1;
    sim_idle_wait();
    if (result) return -1;
    unsigned triangle = (results[0] >> ((x & 1) * 16)) & 0xffff;
    unsigned background = results[1] & 0xffff;
    printf("GPU_READBACK frame=%u triangle=0x%04x background=0x%04x\n", frame, triangle, background);
    return triangle == 0x07e0 && background == 0x001f ? 0 : -1;
}

int main(int argc, char **argv) {
    unsigned long frames = 120;
    if (argc > 2) { fputs("usage: glide_demo [frames:1..10000]\n", stderr); return 2; }
    if (argc == 2) {
        char *end;
        errno = 0;
        frames = strtoul(argv[1], &end, 10);
        if (errno || *end || end == argv[1] || frames < 1 || frames > 10000) {
            fputs("frames must be 1..10000\n", stderr);
            return 2;
        }
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    puts("SpinalVoodoo hardware demo: 640x480 blue clear + moving green triangle");
    grGlideInit();
    GrHwConfiguration hardware;
    if (!grSstQueryHardware(&hardware)) { fputs("No Glide hardware\n", stderr); return 1; }
    grSstSelect(0);
    if (!grSstWinOpen(0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz, GR_COLORFORMAT_ARGB,
                     GR_ORIGIN_UPPER_LEFT, 2, 1)) { fputs("Glide open failed\n", stderr); return 1; }
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_CONSTANT,
                   GR_COMBINE_OTHER_NONE, FXFALSE);
    draw(0);
    int result = verify(0);
    if (!result) {
        double start = seconds();
        for (unsigned frame = 1; frame <= frames; ++frame) {
            draw(frame);
            if (frame % 10 == 0 || frame == frames) {
                double elapsed = seconds() - start;
                printf("PRESENTED frames=%u seconds=%.6f fps=%.3f\n", frame, elapsed, frame / elapsed);
            }
        }
        double elapsed = seconds() - start;
        printf("BENCHMARK frames=%lu seconds=%.6f fps=%.3f resolution=640x480 workload=clear_triangle_swap_present\n",
               frames, elapsed, frames / elapsed);
        result = verify((unsigned)frames);
    }
    grSstWinClose();
    grGlideShutdown();
    puts(result ? "HARDWARE_DEMO_FAIL" : "HARDWARE_DEMO_PASS");
    return result ? 1 : 0;
}
