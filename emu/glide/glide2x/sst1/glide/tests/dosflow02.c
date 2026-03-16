#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"

GrHwConfiguration hwconfig;
static char version[80];

static const char name[] = "dosflow02";
static const char purpose[] = "multi-frame procedural texture quad";
static const char usage[] = "-n <frames> -r <res> -d <filename>";

static FxU16 checker565[64 * 64];

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

static void build_checker(void) {
  int y;
  for (y = 0; y < 64; y++) {
    int x;
    for (x = 0; x < 64; x++) {
      int block = ((x >> 3) ^ (y >> 3)) & 1;
      FxU16 color;
      color = block ? 0xf800 : 0x07ff;
      if ((x > 24 && x < 40) || (y > 24 && y < 40)) {
        color = 0xffff;
      }
      checker565[y * 64 + x] = color;
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
  GrTexInfo texInfo;
  FxU32 texAddr;
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

  build_checker();
  memset(&texInfo, 0, sizeof(texInfo));
  texInfo.smallLod = GR_LOD_64;
  texInfo.largeLod = GR_LOD_64;
  texInfo.aspectRatio = GR_ASPECT_1x1;
  texInfo.format = GR_TEXFMT_RGB_565;
  texInfo.data = checker565;

  texAddr = grTexMinAddress(GR_TMU0);
  grTexDownloadMipMap(GR_TMU0, texAddr, GR_MIPMAPLEVELMASK_BOTH, &texInfo);
  grTexSource(GR_TMU0, texAddr, GR_MIPMAPLEVELMASK_BOTH, &texInfo);
  grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
  grTexMipMapMode(GR_TMU0, GR_MIPMAP_NEAREST, FXFALSE);
  grTexCombine(GR_TMU0,
               GR_COMBINE_FUNCTION_LOCAL,
               GR_COMBINE_FACTOR_NONE,
               GR_COMBINE_FUNCTION_LOCAL,
               GR_COMBINE_FACTOR_NONE,
               FXFALSE, FXFALSE);
  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_NONE,
                 GR_COMBINE_OTHER_TEXTURE,
                 FXFALSE);

  for (frame = 0; frame < frames; frame++) {
    float phase = (float)frame * 0.08f;
    float left = 0.20f + 0.20f * (float)sin((double)phase);
    float top = 0.22f + 0.14f * (float)cos((double)(phase * 0.7f));
    float size = 0.38f;
    GrVertex a, b, c, d;

    grBufferClear(0x00202020, 0, GR_WDEPTHVALUE_FARTHEST);

    a.x = tlScaleX(left);
    a.y = tlScaleY(top);
    b.x = tlScaleX(left + size);
    b.y = tlScaleY(top);
    c.x = tlScaleX(left + size);
    c.y = tlScaleY(top + size);
    d.x = tlScaleX(left);
    d.y = tlScaleY(top + size);

    a.oow = 1.0f;
    b.oow = 1.0f;
    c.oow = 1.0f;
    d.oow = 1.0f;

    a.tmuvtx[0].sow = 0.0f;
    a.tmuvtx[0].tow = 0.0f;
    b.tmuvtx[0].sow = 63.0f;
    b.tmuvtx[0].tow = 0.0f;
    c.tmuvtx[0].sow = 63.0f;
    c.tmuvtx[0].tow = 63.0f;
    d.tmuvtx[0].sow = 0.0f;
    d.tmuvtx[0].tow = 63.0f;

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
