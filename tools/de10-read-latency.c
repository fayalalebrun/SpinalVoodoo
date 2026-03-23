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
  uint32_t offset = 0x0u;
  uint32_t warmup = 10000u;
  uint32_t iters = 1000000u;
  long page_size;
  uint32_t page_base;
  uint32_t page_off;
  void *raw;
  volatile uint32_t *reg;
  volatile uint32_t sink = 0;
  volatile uint32_t fake = 0x12345678u;
  uint64_t t0, t1, t2, t3;
  uint64_t mmio_ns;
  uint64_t base_ns;

  if (argc >= 2) phys = (uint32_t)strtoul(argv[1], NULL, 0);
  if (argc >= 3) offset = (uint32_t)strtoul(argv[2], NULL, 0);
  if (argc >= 4) iters = (uint32_t)strtoul(argv[3], NULL, 0);
  if (argc >= 5) warmup = (uint32_t)strtoul(argv[4], NULL, 0);

  page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    perror("sysconf");
    return 1;
  }
  page_base = phys & ~((uint32_t)page_size - 1u);
  page_off = (phys - page_base) + offset;

  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    fprintf(stderr, "open(/dev/mem) failed: %s\n", strerror(errno));
    return 1;
  }

  raw = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)page_base);
  if (raw == MAP_FAILED) {
    fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    close(fd);
    return 1;
  }

  reg = (volatile uint32_t *)((volatile uint8_t *)raw + page_off);

  for (uint32_t i = 0; i < warmup; ++i) sink ^= *reg;

  t0 = now_ns();
  for (uint32_t i = 0; i < iters; ++i) sink ^= *reg;
  t1 = now_ns();

  t2 = now_ns();
  for (uint32_t i = 0; i < iters; ++i) sink ^= fake;
  t3 = now_ns();

  mmio_ns = t1 - t0;
  base_ns = t3 - t2;

  printf("phys=0x%08x offset=0x%08x iters=%u warmup=%u\n", phys, offset, iters, warmup);
  printf("sample=0x%08x sink=0x%08x\n", *reg, sink);
  printf("mmio_total_ns=%" PRIu64 "\n", mmio_ns);
  printf("baseline_total_ns=%" PRIu64 "\n", base_ns);
  printf("mmio_avg_ns=%.2f\n", (double)mmio_ns / (double)iters);
  printf("baseline_avg_ns=%.2f\n", (double)base_ns / (double)iters);
  if (mmio_ns > base_ns) {
    printf("adjusted_avg_ns=%.2f\n", (double)(mmio_ns - base_ns) / (double)iters);
  }

  munmap(raw, (size_t)page_size);
  close(fd);
  return 0;
}
