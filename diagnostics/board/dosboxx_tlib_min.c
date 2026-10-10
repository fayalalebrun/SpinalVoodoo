#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glide.h"
#include "tlib.h"

extern FxBool gu3dfGetInfo(const char *filename, Gu3dfInfo *info);
extern FxBool gu3dfLoad(const char *filename, Gu3dfInfo *info);

static float screen_width = 640.0f;
static float screen_height = 480.0f;

void tlSetScreen(float width, float height) {
  screen_width = width;
  screen_height = height;
}

float tlScaleX(float coord) { return coord * screen_width; }
float tlScaleY(float coord) { return coord * screen_height; }

const char *tlGetResolutionList(void) { return "640x480"; }
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

FxBool tlOkToRender(void) { return FXTRUE; }
FxBool tlKbHit(void) { return FXFALSE; }
char tlGetCH(void) { return 0; }
void tlConSet(float minX, float minY, float maxX, float maxY, int columns, int rows, int color) {
  (void)minX; (void)minY; (void)maxX; (void)maxY; (void)columns; (void)rows; (void)color;
}
int tlConOutput(const char *fmt, ...) { (void)fmt; return 0; }
void tlConRender(void) { }
void tlGetDimsByConst(const int res, float *width, float *height) {
  (void)res;
  *width = 640.0f;
  *height = 480.0f;
}

int tlGetOpt(int argc, char *argv[], const char *tags, char *match, char ***remArgs) {
  static int idx = 1;
  int i;
  if (idx >= argc) {
    idx = 1;
    return 0;
  }
  if (argv[idx][0] != '-' || argv[idx][1] == '\0') return -1;
  *match = argv[idx][1];
  if (!strchr(tags, *match)) return -1;
  for (i = idx + 1; i < argc; i++) {
    if (argv[i][0] == '-') break;
  }
  *remArgs = &argv[idx + 1];
  idx = i;
  return 1;
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
  pixels = malloc(pixel_bytes);
  if (!pixels) return FXFALSE;
  if (!grLfbReadRegion(GR_BUFFER_FRONTBUFFER, 0, 0, width, height, width * 2, pixels)) {
    free(pixels);
    return FXFALSE;
  }
  rle = malloc(pixel_bytes + pixel_count + 16);
  if (!rle) {
    free(pixels);
    return FXFALSE;
  }
  fp = fopen(filename, "wb");
  if (!fp) {
    free(rle);
    free(pixels);
    return FXFALSE;
  }
  rle_pos = 0;
  for (i = 0; i < pixel_count; ) {
    size_t remain = pixel_count - i;
    size_t chunk = remain > 128 ? 128 : remain;
    rle[rle_pos++] = (FxU8)(chunk - 1);
    memcpy(&rle[rle_pos], &pixels[i], chunk * sizeof(FxU16));
    rle_pos += chunk * sizeof(FxU16);
    i += chunk;
  }

  {
    FxU32 signature = 0x53524C45u;
    fwrite(&signature, sizeof(signature), 1, fp);
  }
  fwrite(&width, sizeof(width), 1, fp);
  fwrite(&height, sizeof(height), 1, fp);
  {
    FxU8 depth = 16, type = 1;
    fwrite(&depth, 1, 1, fp);
    fwrite(&type, 1, 1, fp);
  }
  fwrite(rle, 1, rle_pos, fp);
  fclose(fp);
  free(rle);
  free(pixels);
  return FXTRUE;
}

static GrTexTable_t texTableType(GrTextureFormat_t format) {
  switch (format) {
    case GR_TEXFMT_P_8:
    case GR_TEXFMT_AP_88:
      return GR_TEXTABLE_PALETTE;
    case GR_TEXFMT_YIQ_422:
      return GR_TEXTABLE_NCC0;
    case GR_TEXFMT_AYIQ_8422:
      return GR_TEXTABLE_NCC1;
    default:
      return NO_TABLE;
  }
}

int tlLoadTexture(const char *filename, GrTexInfo *info, GrTexTable_t *tableType, void *table) {
  Gu3dfInfo tdfInfo;
  if (!filename || !info || !tableType || !table) return 0;
  if (!gu3dfGetInfo(filename, &tdfInfo)) return 0;
  tdfInfo.data = malloc(tdfInfo.mem_required);
  if (!tdfInfo.data) return 0;
  if (!gu3dfLoad(filename, &tdfInfo)) {
    free(tdfInfo.data);
    return 0;
  }
  info->smallLod = tdfInfo.header.small_lod;
  info->largeLod = tdfInfo.header.large_lod;
  info->aspectRatio = tdfInfo.header.aspect_ratio;
  info->format = tdfInfo.header.format;
  info->data = tdfInfo.data;
  *tableType = texTableType(info->format);
  switch (*tableType) {
    case GR_TEXTABLE_NCC0:
    case GR_TEXTABLE_NCC1:
    case GR_TEXTABLE_PALETTE:
      memcpy(table, &tdfInfo.table, sizeof(TlTextureTable));
      break;
    default:
      break;
  }
  return 1;
}
