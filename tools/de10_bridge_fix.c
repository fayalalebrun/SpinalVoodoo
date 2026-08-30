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
#define L3REGS_BASE 0xFF800000u
#define SDRCTRL_BASE 0xFFC25000u
#define SYSMGR_BASE 0xFFD08000u
#define BRGMODRST_OFF 0x1Cu
#define MISCI_OFF 0x18u
#define REMAP_OFF 0x00u
#define FPGAPORTRST_OFF 0x80u
#define HANDOFF3_OFF 0x8Cu
#define BRIDGE_RESET_MASK 0x7u
#define FPGA2SDRAM_PORT_MASK 0x3FFFu

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

int main(int argc, char **argv) {
  int fd0, fd1, fd2, fd3, fd4;
  void *m0, *m1, *m2, *m3, *m4;
  size_t l0, l1, l2, l3, l4;
  volatile uint32_t *brgmodrst = map_reg(RSTMGR_BASE + BRGMODRST_OFF, &fd0, &m0, &l0);
  volatile uint32_t *misci = map_reg(RSTMGR_BASE + MISCI_OFF, &fd1, &m1, &l1);
  volatile uint32_t *remap = map_reg(L3REGS_BASE + REMAP_OFF, &fd2, &m2, &l2);
  volatile uint32_t *fpgaportrst = map_reg(SDRCTRL_BASE + FPGAPORTRST_OFF, &fd3, &m3, &l3);
  volatile uint32_t *handoff3 = map_reg(SYSMGR_BASE + HANDOFF3_OFF, &fd4, &m4, &l4);
  uint32_t v;
  int pulse_bridge_reset = 0;
  if (argc == 2 && strcmp(argv[1], "bridge-reset") == 0) {
    pulse_bridge_reset = 1;
  } else if (argc != 1) {
    fprintf(stderr, "usage: %s [bridge-reset]\n", argv[0]);
    return 2;
  }
  if (!brgmodrst || !misci || !remap || !fpgaportrst || !handoff3) {
    fprintf(stderr, "map failed errno=%d (%s)\n", errno, strerror(errno));
    return 1;
  }

  printf("before brgmodrst=0x%08x remap=0x%08x misci=0x%08x fpgaportrst=0x%08x handoff3=0x%08x\n",
         *brgmodrst, *remap, *misci, *fpgaportrst, *handoff3);

  if (pulse_bridge_reset) {
    v = *brgmodrst;
    *brgmodrst = v | BRIDGE_RESET_MASK;
    __sync_synchronize();
    sleep(1);
  }

  v = *brgmodrst;
  v &= ~BRIDGE_RESET_MASK;
  *brgmodrst = v;
  __sync_synchronize();

  v = *remap;
  v |= (1u << 4) | (1u << 3);
  *remap = v;
  __sync_synchronize();

  *handoff3 = FPGA2SDRAM_PORT_MASK;
  *fpgaportrst = FPGA2SDRAM_PORT_MASK;
  __sync_synchronize();

  if (pulse_bridge_reset) {
    // De10PlatformTop holds the core reset for a few seconds after h2f_reset_n
    // returns. Wait here so the next MMIO access does not hit the reset window.
    sleep(4);
  }

  printf("after  brgmodrst=0x%08x remap=0x%08x misci=0x%08x fpgaportrst=0x%08x handoff3=0x%08x\n",
         *brgmodrst, *remap, *misci, *fpgaportrst, *handoff3);

  munmap(m0, l0); close(fd0);
  munmap(m1, l1); close(fd1);
  munmap(m2, l2); close(fd2);
  munmap(m3, l3); close(fd3);
  munmap(m4, l4); close(fd4);
  return 0;
}
