#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char **argv) {
  uint64_t phys = 0xc0000000ULL;
  uint64_t span = 0x1000ULL;
  uint32_t read_off = 0;
  int do_read = 0;
  uint32_t write_off = 0;
  uint32_t write_val = 0;
  int do_write = 0;

  if (argc >= 2) phys = strtoull(argv[1], NULL, 0);
  if (argc >= 3) span = strtoull(argv[2], NULL, 0);
  if (argc >= 4) {
    read_off = (uint32_t)strtoul(argv[3], NULL, 0);
    do_read = (read_off != 0xffffffffu);
  }
  if (argc >= 6) {
    write_off = (uint32_t)strtoul(argv[4], NULL, 0);
    write_val = (uint32_t)strtoul(argv[5], NULL, 0);
    do_write = 1;
  }

  fprintf(stderr,
          "probe: phys=0x%08" PRIx64 " span=0x%08" PRIx64 " do_read=%d roff=0x%08" PRIx32
          " do_write=%d woff=0x%08" PRIx32 " wval=0x%08" PRIx32 "\n",
          phys, span, do_read, read_off, do_write, write_off, write_val);

  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  fprintf(stderr, "probe: open fd=%d errno=%d (%s)\n", fd, errno, strerror(errno));
  if (fd < 0) return 1;

  long page_size = sysconf(_SC_PAGESIZE);
  uint64_t page_mask = (uint64_t)page_size - 1u;
  uint64_t map_base = phys & ~page_mask;
  uint64_t page_off = phys - map_base;
  uint64_t map_size = page_off + span;
  fprintf(stderr, "probe: before mmap base=0x%08" PRIx64 " size=0x%08" PRIx64 "\n", map_base, map_size);

  void *raw = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)map_base);
  fprintf(stderr, "probe: after mmap raw=%p errno=%d (%s)\n", raw, errno, strerror(errno));
  if (raw == MAP_FAILED) {
    close(fd);
    return 2;
  }

  volatile uint8_t *base = (volatile uint8_t *)raw + page_off;
  fprintf(stderr, "probe: mapped base=%p\n", (const void *)base);

  if (do_read) {
    fprintf(stderr, "probe: before read32\n");
    uint32_t value = *(volatile uint32_t *)(base + read_off);
    fprintf(stderr, "probe: after read32 value=0x%08" PRIx32 "\n", value);
  }

  if (do_write) {
    fprintf(stderr, "probe: before write32\n");
    *(volatile uint32_t *)(base + write_off) = write_val;
    fprintf(stderr, "probe: after write32\n");
  }

  fprintf(stderr, "probe: before munmap\n");
  munmap(raw, map_size);
  close(fd);
  fprintf(stderr, "probe: done\n");
  return 0;
}
