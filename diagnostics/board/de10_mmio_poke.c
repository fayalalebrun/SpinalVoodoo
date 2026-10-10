#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define DEFAULT_BASE 0xC0000000u
#define DEFAULT_SPAN 0x1000u

static uint32_t parse_u32(const char *s) {
  char *end = NULL;
  unsigned long v = strtoul(s, &end, 0);
  if (!s[0] || (end && *end)) {
    fprintf(stderr, "invalid integer: %s\n", s);
    exit(2);
  }
  return (uint32_t)v;
}

int main(int argc, char **argv) {
  uint32_t base = DEFAULT_BASE;
  uint32_t span = DEFAULT_SPAN;
  uint32_t offset;
  uint32_t value;
  int write_only = 0;
  int verbose = 0;
  int fd;
  void *map;
  volatile uint32_t *reg;

  while (argc >= 2) {
    if (strcmp(argv[1], "--write-only") == 0) {
      write_only = 1;
    } else if (strcmp(argv[1], "--verbose") == 0) {
      verbose = 1;
    } else {
      break;
    }
    argv++;
    argc--;
  }

  if (argc != 2 && argc != 3 && argc != 4 && argc != 5) {
    fprintf(stderr, "usage: %s [--write-only] [--verbose] offset [value] [base span]\n", argv[0]);
    return 2;
  }
  if (write_only && argc != 3 && argc != 5) {
    fprintf(stderr, "--write-only requires a value\n");
    return 2;
  }

  offset = parse_u32(argv[1]);
  value = 0;
  if (argc == 3 || argc == 5) {
    value = parse_u32(argv[2]);
  }
  if (argc == 5) {
    base = parse_u32(argv[3]);
    span = parse_u32(argv[4]);
  } else if (argc == 4) {
    base = parse_u32(argv[2]);
    span = parse_u32(argv[3]);
  }

  if ((offset & 3u) != 0 || offset + 4u > span) {
    fprintf(stderr, "offset 0x%08x outside mapped span 0x%08x or unaligned\n", offset, span);
    return 2;
  }

  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    perror("open(/dev/mem)");
    return 1;
  }

  map = mmap(NULL, span, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
  if (map == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return 1;
  }

  reg = (volatile uint32_t *)((volatile uint8_t *)map + offset);
  if (verbose) {
    printf("mapped base=0x%08x span=0x%08x virt=%p reg=%p\n", base, span, map, (const void *)reg);
    fflush(stdout);
  }
  if (!write_only) {
    if (verbose) {
      printf("reading [0x%08x + 0x%03x]\n", base, offset);
      fflush(stdout);
    }
    printf("before[0x%08x + 0x%03x] = 0x%08x\n", base, offset, *reg);
  }
  if (argc == 3 || argc == 5) {
    if (verbose) {
      printf("writing [0x%08x + 0x%03x] = 0x%08x\n", base, offset, value);
      fflush(stdout);
    }
    *reg = value;
    if (write_only) {
      printf("wrote [0x%08x + 0x%03x] = 0x%08x\n", base, offset, value);
    } else {
      printf("after [0x%08x + 0x%03x] = 0x%08x\n", base, offset, *reg);
    }
  }

  munmap(map, span);
  close(fd);
  return 0;
}
