/*
** DE10-Nano platform backend for SpinalVoodoo hardware.
**
** Exposes a single fake 3Dfx PCI device backed by the HPS-to-FPGA MMIO
** aperture. This avoids the legacy /dev/3dfx + x86 port-I/O path and lets
** Glide talk to the live core through /dev/mem on ARM Linux.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <3dfx.h>
#include <fxmemmap.h>

#include "fxde10.h"
#include "fxpci.h"
#include "pcilib.h"

#define DE10_DEFAULT_BASE 0xC0000000u
#define DE10_DEFAULT_SPAN 0x01000000u
#define DE10_BATCH_DOORBELL 0x003FF000u
#define DE10_BATCH_CAPABILITY 0x003FF004u
#define DE10_BATCH_MAGIC 0x42415431u
#define DE10_BATCH_DDR_BASE 0x3F080000u
#define DE10_BATCH_SLOT_WORDS 128u
#define DE10_BATCH_SLOTS 4096u
#define DE10_BATCH_DDR_SPAN (DE10_BATCH_SLOT_WORDS * DE10_BATCH_SLOTS * sizeof(FxU32))

De10BatchState de10BatchState = {0, NULL, NULL};

static int de10Fd = -1;
static void *de10MappedRaw = MAP_FAILED;
static uintptr_t de10MappedBase = 0;
static size_t de10MappedLength = 0;
static FxU32 de10PhysBase = DE10_DEFAULT_BASE;
static FxU32 de10PhysSpan = DE10_DEFAULT_SPAN;
static FxU32 de10PciConfig[64];
static void *de10BatchRaw = MAP_FAILED;
static volatile FxU32 *de10BatchBase = NULL;
static volatile FxU32 *de10BatchStart = NULL;
static FxU32 de10BatchSlot = 0;
static int de10BatchAvailable = -1;
static int de10BatchSession = 0;
static int de10PhysicalMemory = 0;

static FxBool de10TraceEnabled(void) {
  const char *value = getenv("DE10_TRACE");
  return (value != NULL && *value != '\0');
}

static void de10Trace(const char *fmt, ...) {
  va_list args;

  if (!de10TraceEnabled()) {
    return;
  }

  fputs("[de10] ", stderr);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fputc('\n', stderr);
}

static FxU32 de10ParseEnvU32(const char *name, FxU32 fallback) {
  const char *value = getenv(name);
  char *end = NULL;
  unsigned long parsed;

  if (value == NULL || *value == '\0') {
    return fallback;
  }

  errno = 0;
  parsed = strtoul(value, &end, 0);
  if (errno != 0 || end == value || *end != '\0') {
    return fallback;
  }

  return (FxU32)parsed;
}

static void de10InitPciConfig(void) {
  memset(de10PciConfig, 0, sizeof(de10PciConfig));
  de10PciConfig[0] = 0x0001121A;
  de10PciConfig[1] = 0x00000002;
  de10PciConfig[2] = 0x04000002;
  de10PciConfig[4] = de10PhysBase;
  de10PciConfig[16] = 0x00000000;
  de10PciConfig[17] = 0x00000000;
  de10PciConfig[18] = 0x00000000;
}

static FxBool de10EnsureMapped(void) {
  long pageSize;
  uintptr_t alignedBase;
  size_t pageOffset;
  size_t mapLength;

  if (de10MappedRaw != MAP_FAILED) {
    de10Trace("reusing mapped aperture base=0x%08x span=0x%08x", de10PhysBase, de10PhysSpan);
    return FXTRUE;
  }

  pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize <= 0) {
    pciErrorCode = PCI_ERR_MEMMAP;
    de10Trace("sysconf(_SC_PAGESIZE) failed");
    return FXFALSE;
  }

  alignedBase = de10PhysBase & ~((uintptr_t)pageSize - 1u);
  pageOffset = (size_t)(de10PhysBase - alignedBase);
  mapLength = pageOffset + de10PhysSpan;

  de10MappedRaw = mmap(NULL, mapLength, PROT_READ | PROT_WRITE, MAP_SHARED, de10Fd, (off_t)alignedBase);
  if (de10MappedRaw == MAP_FAILED) {
    pciErrorCode = PCI_ERR_MEMMAP;
    de10Trace("mmap failed base=0x%08lx length=0x%08lx errno=%d (%s)",
              (unsigned long)alignedBase,
              (unsigned long)mapLength,
              errno,
              strerror(errno));
    return FXFALSE;
  }

  de10MappedBase = (uintptr_t)de10MappedRaw + pageOffset;
  de10MappedLength = mapLength;
  de10Trace("mapped aperture phys=0x%08x span=0x%08x virt=0x%08lx",
            de10PhysBase,
            de10PhysSpan,
            (unsigned long)de10MappedBase);
  return FXTRUE;
}

static FxBool de10EnsureBatchMapped(void) {
  const char *enabled = getenv("DE10_BATCH");
  volatile FxU32 *capability;

  if (de10BatchAvailable >= 0) return de10BatchAvailable ? FXTRUE : FXFALSE;
  de10BatchAvailable = 0;
  if (!de10PhysicalMemory || enabled == NULL || strcmp(enabled, "1") != 0 ||
      de10MappedRaw == MAP_FAILED || de10PhysSpan <= DE10_BATCH_CAPABILITY)
    return FXFALSE;

  capability = (volatile FxU32 *)(de10MappedBase + DE10_BATCH_CAPABILITY);
  if (*capability != DE10_BATCH_MAGIC) return FXFALSE;

  de10BatchRaw = mmap(NULL, DE10_BATCH_DDR_SPAN, PROT_READ | PROT_WRITE,
                      MAP_SHARED, de10Fd, (off_t)DE10_BATCH_DDR_BASE);
  if (de10BatchRaw == MAP_FAILED) return FXFALSE;

  de10BatchBase = (volatile FxU32 *)de10BatchRaw;
  de10BatchState.registerBase = de10MappedBase;
  de10BatchAvailable = 1;
  de10Trace("batch transport ready slots=%u", DE10_BATCH_SLOTS);
  return FXTRUE;
}

static void de10StartBatchSlot(void) {
  de10BatchStart = de10BatchBase + de10BatchSlot * DE10_BATCH_SLOT_WORDS;
  de10BatchState.write = de10BatchStart;
  de10BatchState.end = de10BatchStart + DE10_BATCH_SLOT_WORDS;
}

static void de10SubmitBatch(void) {
  volatile FxU32 *doorbell;
  FxU32 count;

  if (de10BatchStart == NULL || de10BatchState.write == de10BatchStart) return;
  count = (FxU32)(de10BatchState.write - de10BatchStart) >> 1;
  __sync_synchronize();
  doorbell = (volatile FxU32 *)(de10MappedBase + DE10_BATCH_DOORBELL);
  *doorbell = (de10BatchSlot << 20) | (count - 1u);
  de10BatchSlot++;
  if (de10BatchSlot == DE10_BATCH_SLOTS) {
    volatile FxU32 *barrier = (volatile FxU32 *)(de10MappedBase + DE10_BATCH_CAPABILITY);
    __sync_synchronize();
    (void)*barrier;
    de10BatchSlot = 0;
  }
}

void de10BatchBegin(void) {
  if (de10BatchSession || !de10EnsureBatchMapped()) return;
  de10BatchSession = 1;
  de10StartBatchSlot();
}

void de10BatchEnd(void) {
}

void de10BatchFlush(void) {
  if (!de10BatchSession) return;
  de10SubmitBatch();
  de10BatchSession = 0;
  de10BatchStart = NULL;
  de10BatchState.write = NULL;
  de10BatchState.end = NULL;
}

void de10BatchAbort(void) {
  de10BatchFlush();
}

static void de10DisableBatch(void) {
  de10BatchSession = 0;
  de10BatchStart = NULL;
  de10BatchState.write = NULL;
  de10BatchState.end = NULL;
}

void de10BatchWriteSlow(volatile void *address, FxU32 data) {
  uintptr_t offset = (uintptr_t)address - de10BatchState.registerBase;

  if (!de10BatchSession) {
    *(volatile FxU32 *)address = data;
    return;
  }
  de10SubmitBatch();
  if (offset >= 0x00400000u || (offset & 3u) != 0) {
    de10DisableBatch();
    *(volatile FxU32 *)address = data;
    return;
  }
  de10StartBatchSlot();
  *(volatile uint64_t *)de10BatchState.write = (uint64_t)data |
      ((uint64_t)(((FxU32)(offset >> 2) << 6) | 0x1fu) << 32);
  de10BatchState.write += 2;
}

FxU32 de10Load32(volatile void *address) {
  de10BatchFlush();
  return *(volatile FxU32 *)address;
}

void de10Store16(volatile void *address, FxU16 data) {
  de10BatchFlush();
  *(volatile FxU16 *)address = data;
}

static const char *pciIdentifyDe10(void) {
  return "fxPCI for SpinalVoodoo DE10-Nano";
}

static FxBool pciInitializeDe10(void) {
  const char *devicePath = getenv("DE10_VOODOO_MEM");

  if (devicePath == NULL || *devicePath == '\0') {
    devicePath = "/dev/mem";
  }

  de10PhysBase = de10ParseEnvU32("DE10_VOODOO_BASE", DE10_DEFAULT_BASE);
  de10PhysSpan = de10ParseEnvU32("DE10_VOODOO_SPAN", DE10_DEFAULT_SPAN);
  if (de10PhysSpan == 0) {
    de10PhysSpan = DE10_DEFAULT_SPAN;
  }

  de10InitPciConfig();
  de10Trace("initializing devicePath=%s base=0x%08x span=0x%08x", devicePath, de10PhysBase, de10PhysSpan);

  de10PhysicalMemory = strcmp(devicePath, "/dev/mem") == 0;
  de10BatchAvailable = -1;
  de10Fd = open(devicePath, O_RDWR | O_SYNC);
  if (de10Fd < 0) {
    pciErrorCode = PCI_ERR_NO_MEM_PERM;
    de10Trace("open failed path=%s errno=%d (%s)", devicePath, errno, strerror(errno));
    return FXFALSE;
  }

  de10Trace("open succeeded fd=%d", de10Fd);
  return FXTRUE;
}

static FxBool pciShutdownDe10(void) {
  de10BatchAbort();
  if (de10BatchRaw != MAP_FAILED) {
    munmap(de10BatchRaw, DE10_BATCH_DDR_SPAN);
    de10BatchRaw = MAP_FAILED;
    de10BatchBase = NULL;
  }
  de10BatchAvailable = -1;
  if (de10MappedRaw != MAP_FAILED) {
    de10Trace("unmapping aperture virt=0x%08lx length=0x%08lx",
              (unsigned long)de10MappedBase,
              (unsigned long)de10MappedLength);
    munmap(de10MappedRaw, de10MappedLength);
    de10MappedRaw = MAP_FAILED;
    de10MappedBase = 0;
    de10MappedLength = 0;
  }

  if (de10Fd >= 0) {
    de10Trace("closing fd=%d", de10Fd);
    close(de10Fd);
    de10Fd = -1;
  }

  return FXTRUE;
}

static FxU32 pciPortInLongDe10(FxU16 port) {
  return 0xFFFFFFFFu;
}

static FxU16 pciPortInWordDe10(FxU16 port) {
  return 0xFFFFu;
}

static FxU8 pciPortInByteDe10(FxU16 port) {
  return 0xFFu;
}

static FxBool pciPortOutLongDe10(FxU16 port, FxU32 data) {
  return FXTRUE;
}

static FxBool pciPortOutWordDe10(FxU16 port, FxU16 data) {
  return FXTRUE;
}

static FxBool pciPortOutByteDe10(FxU16 port, FxU8 data) {
  return FXTRUE;
}

static FxBool pciMapLinearDe10(FxU32 busNumber, FxU32 physAddr, unsigned long *linearAddr, FxU32 *length) {
  FxU32 offset;

  de10Trace("map request bus=%u phys=0x%08x length=0x%08x", busNumber, physAddr, *length);

  if (physAddr < de10PhysBase || physAddr >= (de10PhysBase + de10PhysSpan)) {
    pciErrorCode = PCI_ERR_MEMMAP;
    de10Trace("map rejected phys=0x%08x outside aperture [0x%08x,0x%08x)",
              physAddr,
              de10PhysBase,
              de10PhysBase + de10PhysSpan);
    return FXFALSE;
  }

  if (!de10EnsureMapped()) {
    return FXFALSE;
  }

  offset = physAddr - de10PhysBase;
  if (*length == 0 || (*length + offset) > de10PhysSpan) {
    *length = de10PhysSpan - offset;
  }

  *linearAddr = (unsigned long)(de10MappedBase + offset);
  de10Trace("map success phys=0x%08x virt=0x%08lx length=0x%08x", physAddr, *linearAddr, *length);
  return FXTRUE;
}

static FxBool pciUnmapLinearDe10(unsigned long linearAddr, FxU32 length) {
  return FXTRUE;
}

static FxBool pciSetPermissionDe10(const unsigned long addrBase, const FxU32 addrLen, const FxBool writePermP) {
  return FXTRUE;
}

static FxBool pciMsrGetDe10(MSRInfo *in, MSRInfo *out) {
  return FXFALSE;
}

static FxBool pciMsrSetDe10(MSRInfo *in, MSRInfo *out) {
  return FXFALSE;
}

static FxBool pciOutputStringDe10(const char *msg) {
  fputs(msg, stderr);
  return FXTRUE;
}

static FxBool pciSetPassThroughBaseDe10(FxU32 *baseAddr, FxU32 baseAddrLen) {
  return FXTRUE;
}

static const FxPlatformIOProcs ioProcsDe10 = {
  pciInitializeDe10,
  pciShutdownDe10,
  pciIdentifyDe10,
  pciPortInByteDe10,
  pciPortInWordDe10,
  pciPortInLongDe10,
  pciPortOutByteDe10,
  pciPortOutWordDe10,
  pciPortOutLongDe10,
  pciMapLinearDe10,
  pciUnmapLinearDe10,
  pciSetPermissionDe10,
  pciMsrGetDe10,
  pciMsrSetDe10,
  pciOutputStringDe10,
  pciSetPassThroughBaseDe10
};

FxBool pciPlatformInit(void) {
  gCurPlatformIO = &ioProcsDe10;
  de10Trace("platform init complete");
  return FXTRUE;
}

FxBool hasDev3DfxLinux(void) {
  de10Trace("reporting fake /dev/3dfx presence");
  return FXTRUE;
}

int getNumDevicesLinux(void) {
  de10Trace("reporting 1 fake PCI device");
  return 1;
}

FxU32 pciFetchRegisterLinux(FxU32 cmd, FxU32 size, FxU32 device) {
  FxU32 index;
  FxU32 shift;
  FxU32 value;

  if ((device & 0xFFFu) != 0) {
    return 0xFFFFFFFFu;
  }

  index = (cmd & 0xFCu) >> 2;
  if (index >= (sizeof(de10PciConfig) / sizeof(de10PciConfig[0]))) {
    return 0xFFFFFFFFu;
  }

  shift = (cmd & 0x3u) * 8u;
  value = de10PciConfig[index] >> shift;
  switch (size) {
    case 1:
      value &= 0xFFu;
      break;
    case 2:
      value &= 0xFFFFu;
      break;
    default:
      break;
  }

  de10Trace("cfg read device=0x%x cmd=0x%x size=%u -> 0x%08x", device, cmd, size, value);
  return value;
}

int pciUpdateRegisterLinux(FxU32 cmd, FxU32 data, FxU32 size, FxU32 device) {
  FxU32 index;
  FxU32 shift;
  FxU32 mask;

  if ((device & 0xFFFu) != 0) {
    return FXFALSE;
  }

  index = (cmd & 0xFCu) >> 2;
  if (index >= (sizeof(de10PciConfig) / sizeof(de10PciConfig[0]))) {
    return FXFALSE;
  }

  shift = (cmd & 0x3u) * 8u;
  switch (size) {
    case 1:
      mask = 0xFFu;
      break;
    case 2:
      mask = 0xFFFFu;
      break;
    default:
      mask = 0xFFFFFFFFu;
      break;
  }

  de10PciConfig[index] &= ~(mask << shift);
  de10PciConfig[index] |= (data & mask) << shift;

  if ((cmd & 0xFCu) == 0x10u) {
    de10PciConfig[index] = (de10PciConfig[index] & ~0xFu) | (de10PhysBase & ~0xFu);
  }

  de10Trace("cfg write device=0x%x cmd=0x%x size=%u data=0x%08x", device, cmd, size, data);
  return FXTRUE;
}
