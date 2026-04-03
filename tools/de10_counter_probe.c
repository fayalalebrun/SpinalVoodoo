#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "glide.h"

#define MMIO_BASE 0xC0000000u
#define MMIO_SPAN 0x1000u

static uint32_t mmio_read32(volatile uint8_t *mmio, uint32_t off) {
  return *(volatile uint32_t *)(mmio + off);
}

static void log_regs(FILE *f, const char *tag, volatile uint8_t *mmio) {
  fprintf(f,
          "%s status=0x%08x pixelsIn=%u pixelsOut=%u debugBusy=0x%08x writePath=0x%08x\n",
          tag,
          mmio_read32(mmio, 0x000),
          mmio_read32(mmio, 0x14c),
          mmio_read32(mmio, 0x15c),
          mmio_read32(mmio, 0x240),
          mmio_read32(mmio, 0x250));
  fflush(f);
}

int main(void) {
  FILE *logf = fopen("/home/fpga/sst1tools/probe.out", "w");
  int fd;
  void *raw;
  volatile uint8_t *mmio;
  GrHwConfiguration hw;

  if (logf == NULL) {
    return 2;
  }

  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    fprintf(logf, "open /dev/mem failed: %s\n", strerror(errno));
    fclose(logf);
    return 3;
  }

  raw = mmap(NULL, MMIO_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)MMIO_BASE);
  close(fd);
  if (raw == MAP_FAILED) {
    fprintf(logf, "mmap failed: %s\n", strerror(errno));
    fclose(logf);
    return 4;
  }
  mmio = (volatile uint8_t *)raw;

  log_regs(logf, "before_init", mmio);
  grGlideInit();
  log_regs(logf, "after_init", mmio);

  if (!grSstQueryHardware(&hw)) {
    fprintf(logf, "grSstQueryHardware failed\n");
    munmap(raw, MMIO_SPAN);
    fclose(logf);
    return 5;
  }
  fprintf(logf, "boards=%d type=%d\n", hw.num_sst, hw.SSTs[0].type);
  grSstSelect(0);
  if (!grSstWinOpen(0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz,
                    GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, 2, 1)) {
    fprintf(logf, "grSstWinOpen failed\n");
    grGlideShutdown();
    munmap(raw, MMIO_SPAN);
    fclose(logf);
    return 6;
  }
  log_regs(logf, "after_open", mmio);

  grBufferClear(0x00ff0000u, 0, 0xffff);
  log_regs(logf, "after_clear", mmio);
  grBufferSwap(1);
  log_regs(logf, "after_swap", mmio);
  grSstIdle();
  log_regs(logf, "after_idle", mmio);

  grGlideShutdown();
  log_regs(logf, "after_shutdown", mmio);
  munmap(raw, MMIO_SPAN);
  fclose(logf);
  return 0;
}
