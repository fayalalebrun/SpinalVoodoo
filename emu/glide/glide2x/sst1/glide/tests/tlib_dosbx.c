#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"

static float screen_width = 640.0f;
static float screen_height = 480.0f;

void tlSetScreen(float width, float height) {
  screen_width = width;
  screen_height = height;
}

float tlScaleX(float coord) {
  return coord * screen_width;
}

float tlScaleY(float coord) {
  return coord * screen_height;
}

const char *tlGetResolutionList(void) {
  return "640x480";
}

const char *tlGetResolutionString(int res) {
  (void)res;
  return "640x480";
}

int tlGetResolutionConstant(const char *arg, float *width, float *height) {
  (void)arg;
  *width = 640.0f;
  *height = 480.0f;
  return GR_RESOLUTION_640x480;
}

void tlGetDimsByConst(const int res, float *width, float *height) {
  (void)res;
  *width = 640.0f;
  *height = 480.0f;
}

int tlGetOpt(int argc, char *argv[], const char *tags, char *match, char **remArgs[]) {
  static int idx = 1;
  int i;

  if (idx >= argc) {
    idx = 1;
    return 0;
  }

  if (argv[idx][0] != '-' || argv[idx][1] == '\0') {
    return -1;
  }

  *match = argv[idx][1];
  if (!strchr(tags, *match)) {
    return -1;
  }

  for (i = idx + 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      break;
    }
  }

  *remArgs = &argv[idx + 1];
  idx = i;
  return 1;
}

FxBool tlOkToRender(void) {
  return FXTRUE;
}

int tlKbHit(void) {
  return 0;
}

char tlGetCH(void) {
  return 0;
}

void tlConSet(float minX, float minY, float maxX, float maxY, int columns, int rows, int color) {
  (void)minX;
  (void)minY;
  (void)maxX;
  (void)maxY;
  (void)columns;
  (void)rows;
  (void)color;
}

int tlConOutput(const char *fmt, ...) {
  (void)fmt;
  return 0;
}

void tlConClear(void) {
}

void tlConRender(void) {
}

void tlSleep(int seconds) {
  (void)seconds;
}

FxBool tlErrorMessage(const char *err) {
  if (err != NULL) {
    fputs(err, stderr);
    fputc('\n', stderr);
  }
  return FXFALSE;
}

void tlGrabRect(void *memory, FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy) {
  FxU32 width;
  FxU32 height;

  if (memory == NULL || maxx < minx || maxy < miny) {
    return;
  }

  width = maxx - minx + 1;
  height = maxy - miny + 1;
  grLfbReadRegion(GR_BUFFER_FRONTBUFFER,
                  minx,
                  miny,
                  width,
                  height,
                  width * sizeof(FxU16),
                  memory);
}

FxBool SimpleRleDecode(FxU16 width, FxU16 height, FxU8 pixelsize, FxU8 *mem, FxU8 *buff) {
  size_t total;
  size_t copied;

  if (mem == NULL || buff == NULL) {
    return FXFALSE;
  }

  total = (size_t)width * (size_t)height * (size_t)pixelsize;
  copied = 0;
  while (copied < total) {
    FxU8 count = (FxU8)(*mem++) + 1;
    size_t chunk = (size_t)count * (size_t)pixelsize;
    memcpy(buff + copied, mem, chunk);
    mem += chunk;
    copied += chunk;
  }

  return FXTRUE;
}

FxBool tlScreenDump(const char *filename, FxU16 width, FxU16 height) {
  FILE *fp;
  FxU16 *pixels;
  FxU8 *rle;
  size_t pixel_bytes;
  size_t pixel_count;
  size_t rle_pos;
  size_t i;

  pixel_bytes = (size_t)width * (size_t)height * sizeof(FxU16);
  pixel_count = (size_t)width * (size_t)height;
  pixels = (FxU16 *)malloc(pixel_bytes);
  if (pixels == NULL) {
    return FXFALSE;
  }

  if (!grLfbReadRegion(GR_BUFFER_FRONTBUFFER, 0, 0, width, height, width * 2, pixels)) {
    free(pixels);
    return FXFALSE;
  }

  rle = (FxU8 *)malloc(pixel_bytes + pixel_count + 16);
  if (rle == NULL) {
    free(pixels);
    return FXFALSE;
  }

  fp = fopen(filename, "wb");
  if (fp == NULL) {
    free(rle);
    free(pixels);
    return FXFALSE;
  }

  rle_pos = 0;
  for (i = 0; i < pixel_count;) {
    size_t remain = pixel_count - i;
    size_t chunk = remain > 128 ? 128 : remain;
    rle[rle_pos++] = (FxU8)(chunk - 1);
    memcpy(&rle[rle_pos], &pixels[i], chunk * sizeof(FxU16));
    rle_pos += chunk * sizeof(FxU16);
    i += chunk;
  }

  {
    FxU32 signature = 0x53524C45UL;
    fwrite(&signature, sizeof(signature), 1, fp);
  }
  fwrite(&width, sizeof(width), 1, fp);
  fwrite(&height, sizeof(height), 1, fp);
  {
    FxU8 depth = 16;
    FxU8 type = 1;
    fwrite(&depth, 1, 1, fp);
    fwrite(&type, 1, 1, fp);
  }
  fwrite(rle, 1, rle_pos, fp);

  fclose(fp);
  free(rle);
  free(pixels);
  return FXTRUE;
}

int tlLoadTexture(const char *filename, GrTexInfo *info, GrTexTable_t *tableType, void *table) {
  (void)filename;
  (void)info;
  (void)tableType;
  (void)table;
  return 0;
}
