#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char **argv) {
  uint32_t phys;
  long page;
  uint32_t base, off;
  int fd;
  void *map;
  volatile uint32_t *reg;
  if (argc != 2) {
    fprintf(stderr, "usage: %s <phys-addr>\n", argv[0]);
    return 2;
  }
  phys = (uint32_t)strtoul(argv[1], NULL, 0);
  page = sysconf(_SC_PAGESIZE);
  base = phys & ~((uint32_t)page - 1u);
  off = phys - base;
  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    fprintf(stderr, "open: %s\n", strerror(errno));
    return 1;
  }
  map = mmap(NULL, (size_t)page, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
  if (map == MAP_FAILED) {
    fprintf(stderr, "mmap: %s\n", strerror(errno));
    close(fd);
    return 1;
  }
  reg = (volatile uint32_t *)((volatile uint8_t *)map + off);
  printf("0x%08x\n", *reg);
  munmap(map, (size_t)page);
  close(fd);
  return 0;
}
