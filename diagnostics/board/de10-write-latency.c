#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

static uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

int main(int argc, char **argv) {
  uint32_t phys = 0xc0000000u;
  uint32_t write_offset = 0x0u;
  uint32_t fence_offset = 0x0u;
  uint32_t iters = 100000u;
  uint32_t warmup = 1000u;
  long page_size;
  uint32_t map_size;
  void *raw;
  volatile uint32_t *write_reg;
  volatile uint32_t *fence_reg;
  volatile uint32_t fake_reg = 0;
  volatile uint32_t sink = 0;
  uint32_t write_value;
  uint64_t t0, t1, t2, t3, t4, t5, t6, t7;

  if (argc >= 2) phys = (uint32_t)strtoul(argv[1], NULL, 0);
  if (argc >= 3) write_offset = (uint32_t)strtoul(argv[2], NULL, 0);
  if (argc >= 4) iters = (uint32_t)strtoul(argv[3], NULL, 0);
  if (argc >= 5) warmup = (uint32_t)strtoul(argv[4], NULL, 0);
  if (argc >= 6) fence_offset = (uint32_t)strtoul(argv[5], NULL, 0);

  page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    perror("sysconf");
    return 1;
  }

  map_size = (write_offset > fence_offset ? write_offset : fence_offset) + sizeof(uint32_t);
  map_size = (map_size + (uint32_t)page_size - 1u) & ~((uint32_t)page_size - 1u);

  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    fprintf(stderr, "open(/dev/mem) failed: %s\n", strerror(errno));
    return 1;
  }

  raw = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)phys);
  if (raw == MAP_FAILED) {
    fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    close(fd);
    return 1;
  }

  write_reg = (volatile uint32_t *)((volatile uint8_t *)raw + write_offset);
  fence_reg = (volatile uint32_t *)((volatile uint8_t *)raw + fence_offset);
  write_value = *write_reg;

  for (uint32_t i = 0; i < warmup; ++i) {
    *write_reg = write_value;
    sink ^= *fence_reg;
  }

  t0 = now_ns();
  for (uint32_t i = 0; i < iters; ++i) *write_reg = write_value;
  t1 = now_ns();

  t2 = now_ns();
  for (uint32_t i = 0; i < iters; ++i) fake_reg = i;
  t3 = now_ns();

  t4 = now_ns();
  for (uint32_t i = 0; i < iters; ++i) {
    *write_reg = write_value;
    sink ^= *fence_reg;
  }
  t5 = now_ns();

  t6 = now_ns();
  for (uint32_t i = 0; i < iters; ++i) {
    fake_reg = i;
    sink ^= fake_reg;
  }
  t7 = now_ns();

  printf("phys=0x%08x write_offset=0x%08x fence_offset=0x%08x iters=%u warmup=%u\n",
         phys, write_offset, fence_offset, iters, warmup);
  printf("write_value=0x%08x fence_sample=0x%08x sink=0x%08x\n", write_value, *fence_reg, sink);
  printf("posted_total_ns=%" PRIu64 "\n", t1 - t0);
  printf("posted_baseline_total_ns=%" PRIu64 "\n", t3 - t2);
  printf("posted_avg_ns=%.2f\n", (double)(t1 - t0) / (double)iters);
  printf("posted_adjusted_avg_ns=%.2f\n", (double)((t1 - t0) - (t3 - t2)) / (double)iters);
  printf("fenced_total_ns=%" PRIu64 "\n", t5 - t4);
  printf("fenced_baseline_total_ns=%" PRIu64 "\n", t7 - t6);
  printf("fenced_avg_ns=%.2f\n", (double)(t5 - t4) / (double)iters);
  printf("fenced_adjusted_avg_ns=%.2f\n", (double)((t5 - t4) - (t7 - t6)) / (double)iters);

  munmap(raw, map_size);
  close(fd);
  return 0;
}
