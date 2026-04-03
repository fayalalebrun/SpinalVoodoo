#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MMIO_BASE 0xc0000000u
#define MMIO_SPAN 0x1000u

#define REG_CONTROL 0x00cu
#define REG_STATUS 0x010u

#define PORT_STRIDE 0x80u
#define FB_PORT_BASE 0x100u
#define TEX_PORT_BASE (FB_PORT_BASE + PORT_STRIDE)

#define PORT_CONTROL 0x00u
#define PORT_BASE_ADDR 0x04u
#define PORT_ADDRESS_MASK 0x08u
#define PORT_STRIDE_BYTES 0x0cu
#define PORT_TRANSFER_COUNT 0x10u
#define PORT_SEED 0x14u
#define PORT_LAST_READ_DATA 0x54u

#define CTRL_START (1u << 0)
#define CTRL_STOP (1u << 1)
#define CTRL_CLEAR (1u << 2)

#define PORT_CTRL_ENABLE (1u << 0)
#define PORT_CTRL_READ (1u << 1)

#define FB_DEFAULT_BASE 0x3f000000u
#define TEX_DEFAULT_BASE 0x3f400000u

static volatile uint32_t *map_region(int fd, uint32_t phys_base, uint32_t span, void **raw_out, size_t *len_out) {
  long page_size = sysconf(_SC_PAGESIZE);
  uint64_t page_mask = (uint64_t)page_size - 1u;
  uint64_t map_base = phys_base & ~page_mask;
  uint64_t page_offset = phys_base - map_base;
  uint64_t map_size = page_offset + span;
  void *mapped = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)map_base);
  if (mapped == MAP_FAILED) return NULL;
  *raw_out = mapped;
  *len_out = (size_t)map_size;
  return (volatile uint32_t *)((volatile uint8_t *)mapped + page_offset);
}

static inline uint32_t reg_read32(volatile uint32_t *regs, uint32_t offset) {
  return regs[offset >> 2];
}

static inline void reg_write32(volatile uint32_t *regs, uint32_t offset, uint32_t value) {
  regs[offset >> 2] = value;
}

int main(int argc, char **argv) {
  const char *port = "tex";
  uint32_t value = 0x11223344u;
  uint32_t offset = 0u;
  uint32_t sweep_words = 0u;
  int interleaved = 0;
  uint32_t target_base = TEX_DEFAULT_BASE;
  int fd = -1;
  void *mmio_raw = NULL, *ddr_raw = NULL;
  size_t mmio_len = 0, ddr_len = 0;
  volatile uint32_t *regs = NULL, *ddr = NULL;
  uint32_t port_base;
  uint32_t control;
  uint32_t seen = 0;

  if (argc >= 2) port = argv[1];
  if (argc >= 3) value = (uint32_t)strtoul(argv[2], NULL, 0);
  if (argc >= 4) offset = (uint32_t)strtoul(argv[3], NULL, 0);
  if (argc >= 5) sweep_words = (uint32_t)strtoul(argv[4], NULL, 0);
  if (argc >= 6) interleaved = atoi(argv[5]);

  if (strcmp(port, "fb") == 0) {
    port_base = FB_PORT_BASE;
    target_base = FB_DEFAULT_BASE;
  } else if (strcmp(port, "tex") == 0) {
    port_base = TEX_PORT_BASE;
    target_base = TEX_DEFAULT_BASE;
  } else {
    fprintf(stderr, "usage: %s [fb|tex] [value] [offset] [sweep_words] [interleaved]\n", argv[0]);
    return 2;
  }

  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    perror("open(/dev/mem)");
    return 1;
  }

  regs = map_region(fd, MMIO_BASE, MMIO_SPAN, &mmio_raw, &mmio_len);
  ddr = map_region(fd, target_base, 0x1000u, &ddr_raw, &ddr_len);
  if (!regs || !ddr) {
    perror("mmap");
    return 1;
  }

  if (sweep_words == 0) {
    ddr[0] = 0x11223344u;
    ddr[1] = 0x55667788u;
  } else {
    if (!interleaved) for (uint32_t i = 0; i < sweep_words; ++i) ddr[i] = 0x13570000u ^ (i * 0x10203u);
  }

  if (sweep_words == 0) {
    reg_write32(regs, REG_CONTROL, CTRL_STOP);
    reg_write32(regs, REG_CONTROL, CTRL_CLEAR);
    reg_write32(regs, port_base + PORT_CONTROL, 0u);
    reg_write32(regs, port_base + PORT_BASE_ADDR, target_base + offset);
    reg_write32(regs, port_base + PORT_ADDRESS_MASK, 0u);
    reg_write32(regs, port_base + PORT_STRIDE_BYTES, 4u);
    reg_write32(regs, port_base + PORT_TRANSFER_COUNT, 1u);
    reg_write32(regs, port_base + PORT_SEED, 0u);
    reg_write32(regs, port_base + PORT_CONTROL, PORT_CTRL_ENABLE | PORT_CTRL_READ);
    reg_write32(regs, REG_CONTROL, CTRL_START);

    for (int i = 0; i < 100000; ++i) {
      uint32_t status = reg_read32(regs, REG_STATUS);
      if (status & 0x2u) break;
    }

    seen = reg_read32(regs, port_base + PORT_LAST_READ_DATA);
    printf("port=%s offset=0x%08" PRIx32 " wrote=[0]=0x%08x [1]=0x%08x saw=0x%08" PRIx32 " status=0x%08" PRIx32 "\n",
           port, offset, 0x11223344u, 0x55667788u, seen, reg_read32(regs, REG_STATUS));
  } else {
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < sweep_words; ++i) {
      uint32_t expected = 0x13570000u ^ (i * 0x10203u);
      if (interleaved) ddr[i] = expected;
      reg_write32(regs, REG_CONTROL, CTRL_STOP);
      reg_write32(regs, REG_CONTROL, CTRL_CLEAR);
      reg_write32(regs, port_base + PORT_CONTROL, 0u);
      reg_write32(regs, port_base + PORT_BASE_ADDR, target_base + offset + i * 4u);
      reg_write32(regs, port_base + PORT_ADDRESS_MASK, 0u);
      reg_write32(regs, port_base + PORT_STRIDE_BYTES, 4u);
      reg_write32(regs, port_base + PORT_TRANSFER_COUNT, 1u);
      reg_write32(regs, port_base + PORT_SEED, 0u);
      reg_write32(regs, port_base + PORT_CONTROL, PORT_CTRL_ENABLE | PORT_CTRL_READ);
      reg_write32(regs, REG_CONTROL, CTRL_START);
      for (int j = 0; j < 100000; ++j) {
        uint32_t status = reg_read32(regs, REG_STATUS);
        if (status & 0x2u) break;
      }
      seen = reg_read32(regs, port_base + PORT_LAST_READ_DATA);
      if (seen != expected) {
        if (mismatches < 8)
          printf("mismatch[%u] off=0x%08x expected=0x%08x saw=0x%08x\n", mismatches, offset + i * 4u, expected, seen);
        mismatches++;
      }
    }
    printf("port=%s sweep offset=0x%08" PRIx32 " words=%u interleaved=%d mismatches=%u\n", port, offset, sweep_words, interleaved, mismatches);
  }

  munmap(mmio_raw, mmio_len);
  munmap(ddr_raw, ddr_len);
  close(fd);
  return (seen == value) ? 0 : 3;
}
