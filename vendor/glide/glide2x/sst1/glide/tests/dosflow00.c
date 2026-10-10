#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"

GrHwConfiguration hwconfig;
static char version[80];

static const char name[] = "dosflow00";
static const char purpose[] = "multi-frame clear plus moving flat bar";
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
  int frames = 60;
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
    float left;
    float right;
    FxU32 bg;
    GrVertex a, b, c, d;
    int phase = frame % 64;

    bg = ((phase * 3) << 16) | ((phase * 2) << 8) | (63 - phase);
    grBufferClear(bg, 0, GR_WDEPTHVALUE_FARTHEST);

    left = (float)((frame * 17) % 520) / 640.0f;
    right = left + 96.0f / 640.0f;

    a.x = tlScaleX(left);
    a.y = tlScaleY(0.18f);
    b.x = tlScaleX(right);
    b.y = tlScaleY(0.18f);
    c.x = tlScaleX(right);
    c.y = tlScaleY(0.82f);
    d.x = tlScaleX(left);
    d.y = tlScaleY(0.82f);

    grConstantColorValue(0x00f0f0f0);
    grDrawTriangle(&a, &b, &c);
    grDrawTriangle(&a, &c, &d);
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
