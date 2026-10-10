#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"

GrHwConfiguration hwconfig;
static char version[80];

static const char name[] = "dosflow01";
static const char purpose[] = "multi-frame moving flat triangle";
static const char usage[] = "-n <frames> -r <res> -d <filename>";

static void parse_args(int argc, char **argv,
                       int *frames,
                       GrScreenResolution_t *resolution,
                       float *scrWidth,
                       float *scrHeight,
                       FxBool *scrgrab,
                       char *filename) {
  char match;
  char **remArgs;
  int rv;

  while ((rv = tlGetOpt(argc, argv, "nrd", &match, &remArgs)) != 0) {
    if (rv == -1) {
      printf("Unrecognized command line argument\n");
      printf("%s %s\n", name, usage);
      printf("Available resolutions:\n%s\n", tlGetResolutionList());
      exit(1);
    }
    switch (match) {
      case 'n':
        *frames = atoi(remArgs[0]);
        break;
      case 'r':
        *resolution = tlGetResolutionConstant(remArgs[0], scrWidth, scrHeight);
        break;
      case 'd':
        *scrgrab = FXTRUE;
        strcpy(filename, remArgs[0]);
        break;
    }
  }
}

int main(int argc, char **argv) {
  GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
  float scrWidth = 640.0f;
  float scrHeight = 480.0f;
  int frames = 90;
  FxBool scrgrab = FXFALSE;
  char filename[256] = {0};
  int frame;

  parse_args(argc, argv, &frames, &resolution, &scrWidth, &scrHeight, &scrgrab, filename);
  tlSetScreen(scrWidth, scrHeight);

  grGlideGetVersion(version);
  printf("%s:\n%s\n", name, purpose);
  printf("%s\n", version);
  printf("Resolution: %s\n", tlGetResolutionString(resolution));

  grGlideInit();
  assert(grSstQueryHardware(&hwconfig));
  grSstSelect(0);
  assert(grSstWinOpen(0,
                      resolution,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1));

  grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
                 GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_LOCAL_CONSTANT,
                 GR_COMBINE_OTHER_NONE,
                 FXFALSE);

  for (frame = 0; frame < frames; frame++) {
    float cx = 0.5f + 0.28f * (float)sin((double)frame * 0.11);
    float cy = 0.5f + 0.18f * (float)cos((double)frame * 0.07);
    float size = 0.18f;
    FxU32 color = ((frame * 5) & 0xff) | (((frame * 3) & 0xff) << 8) | (((255 - frame * 2) & 0xff) << 16);
    GrVertex a, b, c;

    grBufferClear(0x00101018, 0, GR_WDEPTHVALUE_FARTHEST);

    a.x = tlScaleX(cx);
    a.y = tlScaleY(cy - size);
    b.x = tlScaleX(cx - size);
    b.y = tlScaleY(cy + size);
    c.x = tlScaleX(cx + size);
    c.y = tlScaleY(cy + size * 0.7f);

    grConstantColorValue(color);
    grDrawTriangle(&a, &b, &c);
    grBufferSwap(1);
    grSstIdle();
  }

  if (scrgrab) {
    if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight)) {
      printf("Cannot open %s\n", filename);
    }
  }

  grGlideShutdown();
  return 0;
}
