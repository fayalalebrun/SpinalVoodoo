#include "de10_adv7513.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define DE10_ADV7513_DEFAULT_I2C_BASE 0xffc06000u
#define DE10_ADV7513_I2C_SPAN 0x1000u
#define DE10_ADV7513_ADDR 0x39u

#define IC_CON              0x00
#define IC_TAR              0x04
#define IC_DATA_CMD         0x10
#define IC_SS_SCL_HCNT      0x14
#define IC_SS_SCL_LCNT      0x18
#define IC_FS_SCL_HCNT      0x1c
#define IC_FS_SCL_LCNT      0x20
#define IC_RAW_INTR_STAT    0x34
#define IC_CLR_INTR         0x40
#define IC_CLR_TX_ABRT      0x54
#define IC_ENABLE           0x6c
#define IC_STATUS           0x70
#define IC_TXFLR            0x74
#define IC_RXFLR            0x78
#define IC_TX_ABRT_SOURCE   0x80
#define IC_ENABLE_STATUS    0x9c
#define IC_COMP_TYPE        0xfc

#define IC_DATA_CMD_READ    (1u << 8)
#define IC_DATA_CMD_STOP    (1u << 9)
#define IC_DATA_CMD_RESTART (1u << 10)

typedef struct {
  uint8_t reg;
  uint8_t val;
} De10Adv7513RegValue;

static const De10Adv7513RegValue advInit[] = {
  {0x98, 0x03}, {0xd6, 0xd0}, {0x41, 0x10}, {0x9a, 0x70},
  {0x9c, 0x30}, {0x9d, 0x61}, {0xa2, 0xa4}, {0xa3, 0xa4},
  {0xe0, 0xd0}, {0x35, 0x40}, {0x36, 0xd9}, {0x37, 0x0a},
  {0x38, 0x00}, {0x39, 0x2d}, {0x3a, 0x00}, {0x16, 0x38},
  {0x17, 0x62}, {0x3b, 0x40}, {0x3c, 0x02}, {0x43, 0x7e},
  {0x45, 0x70}, {0x48, 0x08}, {0x49, 0xa8}, {0x40, 0x00},
  {0x4a, 0x80}, {0x4c, 0x00}, {0x55, 0x10}, {0x56, 0x08},
  {0x57, 0x08}, {0x59, 0x00}, {0x73, 0x01}, {0x94, 0x80},
  {0x99, 0x02}, {0x9b, 0x18}, {0x9f, 0x00}, {0xa1, 0x00},
  {0xa4, 0x08}, {0xa5, 0x04}, {0xa6, 0x00}, {0xa7, 0x00},
  {0xa8, 0x00}, {0xa9, 0x00}, {0xaa, 0x00}, {0xab, 0x40},
  {0xaf, 0x06}, {0xb9, 0x00}, {0xba, 0x60}, {0xbb, 0x00},
  {0xde, 0x9c}, {0xe2, 0x01}, {0xe4, 0x60}, {0xfa, 0x7d},
  {0x0a, 0x00}, {0x0b, 0x0e}, {0x0c, 0x04}, {0x0d, 0x10},
  {0x14, 0x02}, {0x15, 0x20}, {0x01, 0x00}, {0x02, 0x18},
  {0x03, 0x00}, {0x07, 0x01}, {0x08, 0x22}, {0x09, 0x0a},
};

static volatile uint32_t *i2cRegs;

static uint32_t rd(uint32_t off) { return i2cRegs[off / 4]; }
static void wr(uint32_t off, uint32_t val) { i2cRegs[off / 4] = val; }

static int envEnabled(const char *name, int fallback) {
  const char *value = getenv(name);
  if (value == NULL || *value == '\0') return fallback;
  return strcmp(value, "0") != 0;
}

static uint32_t parseEnvU32(const char *name, uint32_t fallback) {
  const char *value = getenv(name);
  char *end = NULL;
  unsigned long parsed;

  if (value == NULL || *value == '\0') return fallback;
  errno = 0;
  parsed = strtoul(value, &end, 0);
  if (errno != 0 || end == value || *end != '\0') return fallback;
  return (uint32_t)parsed;
}

static int verboseEnabled(void) {
  return envEnabled("DE10_ADV7513_VERBOSE", 0) || envEnabled("DE10_TRACE", 0);
}

static int waitUntil(uint32_t off, uint32_t mask, uint32_t val, int timeoutUs) {
  while (timeoutUs-- > 0) {
    if ((rd(off) & mask) == val) return 0;
    usleep(1);
  }
  return -1;
}

static int waitTxNotFull(void) {
  return waitUntil(IC_STATUS, 1u << 1, 1u << 1, 5000);
}

static int waitRxNotEmpty(void) {
  return waitUntil(IC_STATUS, 1u << 3, 1u << 3, 5000);
}

static int waitTxEmpty(void) {
  return waitUntil(IC_TXFLR, 0xffffffffu, 0, 5000);
}

static int waitIdle(void) {
  return waitUntil(IC_STATUS, 1u, 0, 5000);
}

static int waitTransferDone(void) {
  if (waitTxEmpty() < 0) return -1;
  return waitIdle();
}

static void clearAbort(void) {
  (void)rd(IC_CLR_TX_ABRT);
  (void)rd(IC_CLR_INTR);
}

static int hasAbort(void) {
  return (rd(IC_RAW_INTR_STAT) & (1u << 6)) != 0;
}

static int controllerInit(uint8_t addr) {
  wr(IC_ENABLE, 0);
  if (waitUntil(IC_ENABLE_STATUS, 1u, 0, 5000) < 0) return -1;

  wr(IC_CON, (1u << 0) | (1u << 1) | (1u << 5) | (1u << 6));
  wr(IC_TAR, addr);
  wr(IC_SS_SCL_HCNT, 500);
  wr(IC_SS_SCL_LCNT, 600);
  wr(IC_FS_SCL_HCNT, 100);
  wr(IC_FS_SCL_LCNT, 130);
  clearAbort();
  wr(IC_ENABLE, 1);
  return waitUntil(IC_ENABLE_STATUS, 1u, 1u, 5000);
}

static int i2cWriteReg(uint8_t addr, uint8_t reg, uint8_t val) {
  if (controllerInit(addr) < 0) return -1;
  if (waitTxNotFull() < 0) return -1;
  wr(IC_DATA_CMD, reg | IC_DATA_CMD_RESTART);
  if (waitTxNotFull() < 0) return -1;
  wr(IC_DATA_CMD, val | IC_DATA_CMD_STOP);
  if (waitTransferDone() < 0) return -1;
  if (hasAbort()) {
    uint32_t src = rd(IC_TX_ABRT_SOURCE);
    clearAbort();
    errno = EIO;
    return -(int)(src ? src : 1);
  }
  return 0;
}

static int i2cReadReg(uint8_t addr, uint8_t reg) {
  if (controllerInit(addr) < 0) return -1;
  if (waitTxNotFull() < 0) return -1;
  wr(IC_DATA_CMD, reg | IC_DATA_CMD_RESTART);
  if (waitTxNotFull() < 0) return -1;
  wr(IC_DATA_CMD, IC_DATA_CMD_READ | IC_DATA_CMD_RESTART | IC_DATA_CMD_STOP);
  if (waitRxNotEmpty() < 0) return -1;
  {
    int val = (int)(rd(IC_DATA_CMD) & 0xff);
    if (waitIdle() < 0) return -1;
    if (hasAbort()) {
      uint32_t src = rd(IC_TX_ABRT_SOURCE);
      clearAbort();
      errno = EIO;
      return -(int)(src ? src : 1);
    }
    return val;
  }
}

static int mapI2c(uint32_t base) {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) return -1;
  i2cRegs = (volatile uint32_t *)mmap(NULL,
                                      DE10_ADV7513_I2C_SPAN,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED,
                                      fd,
                                      base);
  close(fd);
  if (i2cRegs == MAP_FAILED) {
    i2cRegs = NULL;
    return -1;
  }
  return 0;
}

static void unmapI2c(void) {
  if (i2cRegs != NULL) munmap((void *)i2cRegs, DE10_ADV7513_I2C_SPAN);
  i2cRegs = NULL;
}

static int initAdv7513(uint32_t base) {
  int id0;
  int pwr;
  int stat;
  int verbose = verboseEnabled();

  if (mapI2c(base) < 0) {
    fprintf(stderr, "[de10] adv7513: map 0x%08x failed: %s\n", base, strerror(errno));
    return 0;
  }

  if (verbose) {
    uint32_t comp = rd(IC_COMP_TYPE);
    fprintf(stderr, "[de10] adv7513: init base=0x%08x comp=0x%08x\n", base, comp);
  }

  id0 = i2cReadReg(DE10_ADV7513_ADDR, 0x00);
  pwr = i2cReadReg(DE10_ADV7513_ADDR, 0x41);
  stat = i2cReadReg(DE10_ADV7513_ADDR, 0x42);
  if (id0 < 0 && pwr < 0 && stat < 0) {
    fprintf(stderr, "[de10] adv7513: not readable at 0x%02x on base 0x%08x\n",
            DE10_ADV7513_ADDR,
            base);
    unmapI2c();
    return 0;
  }

  if (verbose) {
    fprintf(stderr, "[de10] adv7513: before reg00=%02x reg41=%02x reg42=%02x\n",
            id0 & 0xff,
            pwr & 0xff,
            stat & 0xff);
  }

  for (size_t i = 0; i < sizeof(advInit) / sizeof(advInit[0]); ++i) {
    int r = i2cWriteReg(DE10_ADV7513_ADDR, advInit[i].reg, advInit[i].val);
    if (r < 0) {
      fprintf(stderr, "[de10] adv7513: write %02x=%02x failed: abort=0x%x\n",
              advInit[i].reg,
              advInit[i].val,
              -r);
      unmapI2c();
      return 0;
    }
  }

  if (envEnabled("DE10_ADV7513_DVI", 0)) {
    int r = i2cWriteReg(DE10_ADV7513_ADDR, 0xaf, 0x04);
    if (r < 0) {
      fprintf(stderr, "[de10] adv7513: write af=04 failed: abort=0x%x\n", -r);
      unmapI2c();
      return 0;
    }
  }

  if (i2cWriteReg(DE10_ADV7513_ADDR, 0x41, 0x00) < 0 ||
      i2cWriteReg(DE10_ADV7513_ADDR, 0xd6, 0xd0) < 0) {
    fprintf(stderr, "[de10] adv7513: final power/hpd writes failed\n");
    unmapI2c();
    return 0;
  }

  if (verbose) {
    int fmt = i2cReadReg(DE10_ADV7513_ADDR, 0x16);
    int mode = i2cReadReg(DE10_ADV7513_ADDR, 0x3b);
    int vic = i2cReadReg(DE10_ADV7513_ADDR, 0x3c);
    int pll = i2cReadReg(DE10_ADV7513_ADDR, 0x9e);
    id0 = i2cReadReg(DE10_ADV7513_ADDR, 0x00);
    pwr = i2cReadReg(DE10_ADV7513_ADDR, 0x41);
    stat = i2cReadReg(DE10_ADV7513_ADDR, 0x42);
    fprintf(stderr,
            "[de10] adv7513: after reg00=%02x reg16=%02x reg3b=%02x reg3c=%02x reg41=%02x reg42=%02x reg9e=%02x\n",
            id0 & 0xff,
            fmt & 0xff,
            mode & 0xff,
            vic & 0xff,
            pwr & 0xff,
            stat & 0xff,
            pll & 0xff);
  }

  unmapI2c();
  return 1;
}

int de10Adv7513InitOnce(void) {
  static int attempted;
  static int ok;
  uint32_t base;

  if (!envEnabled("DE10_ADV7513_INIT", 1)) return 1;
  if (attempted) return ok;

  attempted = 1;
  base = parseEnvU32("DE10_ADV7513_I2C_BASE", DE10_ADV7513_DEFAULT_I2C_BASE);
  ok = initAdv7513(base);
  if (!ok) {
    fprintf(stderr, "[de10] adv7513: init failed; set DE10_ADV7513_INIT=0 to skip\n");
  }
  return ok;
}
