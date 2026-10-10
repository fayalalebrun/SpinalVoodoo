#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static volatile sig_atomic_t got_sigbus;

static void on_sigbus(int sig) {
  (void)sig;
  got_sigbus = 1;
}

static int read_reg(off_t phys, uint32_t *value) {
  const long page = sysconf(_SC_PAGESIZE);
  const off_t base = phys & ~((off_t)page - 1);
  const off_t off = phys - base;
  int fd;
  void *map;

  fd = open("/dev/mem", O_RDONLY | O_SYNC);
  if (fd < 0) {
    return -1;
  }
  map = mmap(NULL, (size_t)page, PROT_READ, MAP_SHARED, fd, base);
  close(fd);
  if (map == MAP_FAILED) {
    return -2;
  }

  got_sigbus = 0;
  *value = *(volatile uint32_t *)((volatile uint8_t *)map + off);
  munmap(map, (size_t)page);
  if (got_sigbus) {
    return -3;
  }
  return 0;
}

int main(void) {
  const struct {
    const char *name;
    off_t phys;
  } regs[] = {
    {"rstmgr.brgmodrst", 0xFFD0501C},
    {"rstmgr.misci", 0xFFD05018},
    {"sysmgr.remap", 0xFFD08058},
    {"sysmgr.fpgaportrst", 0xFFD05024},
  };
  size_t i;
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = on_sigbus;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGBUS, &sa, NULL);

  for (i = 0; i < sizeof(regs) / sizeof(regs[0]); ++i) {
    uint32_t value = 0;
    int rc = read_reg(regs[i].phys, &value);
    if (rc == 0) {
      printf("%s 0x%08x\n", regs[i].name, value);
    } else {
      printf("%s error=%d errno=%d (%s)\n", regs[i].name, rc, errno, strerror(errno));
    }
  }

  return 0;
}
