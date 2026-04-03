#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define RSTMGR_BASE 0xFFD05000u
#define SYSMGR_BASE 0xFFD08000u
#define BRGMODRST_OFF 0x1Cu
#define MISCI_OFF 0x18u
#define REMAP_OFF 0x58u

static volatile uint32_t *map_reg(uint32_t phys, int *fd_out, void **map_out, size_t *len_out) {
  long page = sysconf(_SC_PAGESIZE);
  uint32_t page_base = phys & ~((uint32_t)page - 1u);
  uint32_t page_off = phys - page_base;
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  void *map;
  if (fd < 0) return NULL;
  map = mmap(NULL, (size_t)page, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
  if (map == MAP_FAILED) {
    close(fd);
    return NULL;
  }
  *fd_out = fd;
  *map_out = map;
  *len_out = (size_t)page;
  return (volatile uint32_t *)((volatile uint8_t *)map + page_off);
}

int main(void) {
  int fd0, fd1, fd2;
  void *m0, *m1, *m2;
  size_t l0, l1, l2;
  volatile uint32_t *brgmodrst = map_reg(RSTMGR_BASE + BRGMODRST_OFF, &fd0, &m0, &l0);
  volatile uint32_t *misci = map_reg(RSTMGR_BASE + MISCI_OFF, &fd1, &m1, &l1);
  volatile uint32_t *remap = map_reg(SYSMGR_BASE + REMAP_OFF, &fd2, &m2, &l2);
  uint32_t v;
  if (!brgmodrst || !misci || !remap) {
    fprintf(stderr, "map failed errno=%d (%s)\n", errno, strerror(errno));
    return 1;
  }

  printf("before brgmodrst=0x%08x remap=0x%08x misci=0x%08x\n", *brgmodrst, *remap, *misci);

  v = *brgmodrst;
  v &= ~0x7u;
  *brgmodrst = v;
  __sync_synchronize();

  v = *remap;
  v |= (1u << 4) | (1u << 3);
  *remap = v;
  __sync_synchronize();

  printf("after  brgmodrst=0x%08x remap=0x%08x misci=0x%08x\n", *brgmodrst, *remap, *misci);

  munmap(m0, l0); close(fd0);
  munmap(m1, l1); close(fd1);
  munmap(m2, l2); close(fd2);
  return 0;
}
