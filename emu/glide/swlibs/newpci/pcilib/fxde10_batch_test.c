#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fxde10.h"

De10BatchState de10BatchState = {0, NULL, NULL};
static int slowCalls;

void de10BatchWriteSlow(volatile void *address, FxU32 data) {
  slowCalls++;
  *(volatile FxU32 *)address = data;
}

int main(void) {
  static FxU32 registers[16];
  static uint64_t recordStorage[4];
  FxU32 *records = (FxU32 *)recordStorage;
  union {
    float f;
    FxU32 u;
  } bits, highBits;
  uint64_t directPair = 0;

  memset(registers, 0, sizeof(registers));
  memset(records, 0, sizeof(recordStorage));
  de10BatchState.registerBase = (uintptr_t)registers;
  de10BatchState.write = records;
  de10BatchState.end = records + 8;

  de10Store32(&registers[3], 0x12345678u);
  bits.f = 1.5f;
  highBits.f = -2.0f;
  de10StoreFloatPair(&registers[4], bits.f, highBits.f);
  if (recordStorage[0] != (((uint64_t)((3u << 6) | 0x1fu) << 32) | 0x12345678u) ||
      recordStorage[1] != (((uint64_t)((4u << 6) | 0x1fu) << 32) | bits.u) ||
      recordStorage[2] != (((uint64_t)((5u << 6) | 0x1fu) << 32) | highBits.u) ||
      slowCalls != 0)
    return 1;

  de10BatchState.write = de10BatchState.end;
  de10Store32(&registers[5], 0xa5a5a5a5u);
  if (slowCalls != 1 || registers[5] != 0xa5a5a5a5u) return 1;

  de10BatchState.write = NULL;
  de10Store32(&registers[6], 0x55aa55aau);
  de10StoreFloatPair(&directPair, bits.f, highBits.f);
  if (registers[6] != 0x55aa55aau ||
      directPair != ((uint64_t)highBits.u << 32 | bits.u)) return 1;

  puts("fxde10_batch_test PASS");
  return 0;
}
