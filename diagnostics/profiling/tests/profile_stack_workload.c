#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <time.h>

__attribute__((noinline)) static void sample_busy(void) {
    volatile uint32_t value = 1;
    for (unsigned i = 0; i < 10000000; ++i) value = value * 1664525u + 1013904223u;
}
__attribute__((noinline)) static void sample_sleep(void) {
    struct timespec delay = {0, 20000000};
    nanosleep(&delay, 0);
}
__attribute__((noinline)) static void sample_work(void) {
    sample_busy();
    sample_sleep();
}
int main(void) {
    for (unsigned i = 0; i < 100; ++i) sample_work();
    return 0;
}
