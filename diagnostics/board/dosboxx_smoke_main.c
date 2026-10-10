#include <stdio.h>

#include "glide.h"

int main(void) {
  GrHwConfiguration hw;
  FxU16 pix = 0;

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  printf("smoke start\n");
  grGlideInit();
  printf("after grGlideInit\n");
  if (!grSstQueryHardware(&hw)) {
    printf("query failed\n");
    return 2;
  }
  printf("ssts=%d type=%d\n", hw.num_sst, hw.SSTs[0].type);
  grSstSelect(0);
  if (!grSstWinOpen(0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz,
                    GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, 2, 1)) {
    printf("winopen failed\n");
    grGlideShutdown();
    return 3;
  }
  printf("after grSstWinOpen\n");
  grBufferClear(0x00ff0000u, 0, 0xffff);
  printf("after grBufferClear\n");
  grBufferSwap(1);
  printf("after grBufferSwap\n");
  if (!grLfbReadRegion(GR_BUFFER_FRONTBUFFER, 0, 0, 1, 1, 2, &pix)) {
    printf("lfbread failed\n");
    grGlideShutdown();
    return 4;
  }
  printf("pix=0x%04x\n", pix);
  grGlideShutdown();
  return 0;
}
