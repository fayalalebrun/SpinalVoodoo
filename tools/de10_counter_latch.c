#include <stdio.h>
#include <unistd.h>

#include "glide.h"

int main(void) {
  GrHwConfiguration hw;

  grGlideInit();
  if (!grSstQueryHardware(&hw)) return 2;
  grSstSelect(0);
  if (!grSstWinOpen(0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz,
                    GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, 2, 1)) {
    grGlideShutdown();
    return 3;
  }

  grBufferClear(0x00ff0000u, 0, 0xffff);
  grBufferSwap(1);
  sleep(5);
  grGlideShutdown();
  return 0;
}
