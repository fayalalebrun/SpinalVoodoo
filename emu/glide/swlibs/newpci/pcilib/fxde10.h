#ifndef _FXDE10_H_
#define _FXDE10_H_

#include <stdint.h>
#include <3dfx.h>

typedef struct {
  uintptr_t registerBase;
  volatile FxU32 *write;
  volatile FxU32 *end;
} De10BatchState;

extern De10BatchState de10BatchState;

void de10BatchBegin(void);
void de10BatchEnd(void);
void de10BatchAbort(void);
void de10BatchFlush(void);
void de10BatchWriteSlow(volatile void *address, FxU32 data);
FxU32 de10Load32(volatile void *address);
void de10Store16(volatile void *address, FxU16 data);

static inline __attribute__((always_inline)) void de10Store32(volatile void *address, FxU32 data) {
  volatile FxU32 *write = de10BatchState.write;
  uintptr_t offset;

  if (write != NULL && write < de10BatchState.end) {
    offset = (uintptr_t)address - de10BatchState.registerBase;
    if (offset < 0x00400000u && (offset & 3u) == 0) {
      *(volatile uint64_t *)write = (uint64_t)data |
          ((uint64_t)(((FxU32)(offset >> 2) << 6) | 0x1fu) << 32);
      de10BatchState.write = write + 2;
      return;
    }
  }
  if (write != NULL) {
    de10BatchWriteSlow(address, data);
  } else {
    *(volatile FxU32 *)address = data;
  }
}

static inline __attribute__((always_inline)) void de10StoreFloat(volatile void *address, float data) {
  union {
    float f;
    FxU32 u;
  } bits;
  bits.f = data;
  de10Store32(address, bits.u);
}

static inline __attribute__((always_inline)) void de10StoreFloatPair(
    volatile void *address, float first, float second) {
  union {
    float f;
    FxU32 u;
  } low, high;

  if (de10BatchState.write != NULL) {
    de10StoreFloat(address, first);
    de10StoreFloat((volatile FxU32 *)address + 1, second);
    return;
  }
  low.f = first;
  high.f = second;
  *(volatile uint64_t *)address = (uint64_t)low.u | ((uint64_t)high.u << 32);
}

FxBool pciPlatformInit(void);
FxBool hasDev3DfxLinux(void);
int getNumDevicesLinux(void);
FxU32 pciFetchRegisterLinux(FxU32 cmd, FxU32 size, FxU32 device);
int pciUpdateRegisterLinux(FxU32 cmd, FxU32 data, FxU32 size, FxU32 device);

#endif
