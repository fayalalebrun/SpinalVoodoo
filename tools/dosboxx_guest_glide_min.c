#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <dpmi.h>
#include <go32.h>
#include <pc.h>

#include "glide.h"

enum {
  F_grBufferClear = 10,
  F_grBufferSwap = 12,
  F_grClipWindow = 16,
  F_grColorCombine = 17,
  F_grConstantColorValue = 20,
  F_grDrawPoint = 31,
  F_grGlideGetVersion = 41,
  F_grGlideInit = 42,
  F_grGlideShutdown = 45,
  F_grLfbReadRegion = 50,
  F_grSstQueryHardware = 65,
  F_grSstIdle = 60,
  F_grSstSelect = 69,
  F_grSstWinOpen = 75,
  F_grTexCalcMemRequired = 76,
  F_grTexCombine = 78,
  F_grTexDownloadMipMap = 81,
  F_grTexDownloadMipMapLevel = 82,
  F_grTexDownloadMipMapLevelPartial = 83,
  F_grTexDownloadTable = 84,
  F_grTexFilterMode = 86,
  F_grTexLodBiasValue = 87,
  F_grTexMaxAddress = 88,
  F_grTexMinAddress = 89,
  F_grTexMipMapMode = 90,
  F_grTexMultibase = 91,
  F_grTexMultibaseAddress = 92,
  F_grTexSource = 94,
  F_grTexTextureMemRequired = 95,
  F_grAlphaBlendFunction = 5,
  F_grAlphaCombine = 6,
  F_grAlphaTestFunction = 8,
  F_grAlphaTestReferenceValue = 9,
  F_grCullMode = 21,
  F_grDepthBufferFunction = 23,
  F_grDrawTriangle = 34,
  F_grGlideGetState = 40,
  F_grGlideSetState = 43,
  F_grHints = 46,
  F_grSstOrigin = 62,
};

typedef struct {
  int32_t smallLod;
  int32_t largeLod;
  int32_t aspectRatio;
  int32_t format;
  unsigned long data;
} DBGrTexInfo;

static unsigned short glide_port;
static unsigned short cmd_seg;
static unsigned long cmd_phys;
static int glide_active;

static _go32_dpmi_seginfo handmem;
static unsigned long handaddr;
static int handready;

static _go32_dpmi_seginfo hwmem;
static unsigned long hwaddr;
static int hwready;

static _go32_dpmi_seginfo lfbmem;
static unsigned long lfbaddr;
static int lfbready;

static _go32_dpmi_seginfo vertexmem;
static unsigned long vertexaddr;
static int vertexready;
static _go32_dpmi_seginfo triVertexmem;
static unsigned long triVertexaddr;
static int triVertexready;
static _go32_dpmi_seginfo texinfo_mem;
static unsigned long texinfo_addr;
static int texinfo_ready;
static _go32_dpmi_seginfo state_mem;
static unsigned long state_addr;
static int state_ready;
static _go32_dpmi_seginfo tex_data_mem;
static unsigned long tex_data_addr;
static size_t tex_data_bytes;
static _go32_dpmi_seginfo table_mem;
static unsigned long table_addr;
static size_t table_bytes;

static _go32_dpmi_seginfo linebuf;
static unsigned long lineaddr;
static size_t linebytes;

static int alloc_dos(size_t bytes, _go32_dpmi_seginfo *info) {
  memset(info, 0, sizeof(*info));
  info->size = (bytes + 15u) >> 4;
  return _go32_dpmi_allocate_dos_memory(info);
}

static unsigned long dos_addr(const _go32_dpmi_seginfo *info) {
  return ((unsigned long)info->rm_segment) << 4;
}

static void dos_put(unsigned long addr, const void *src, size_t len) {
  dosmemput(src, (int)len, addr);
}

static void dos_get(unsigned long addr, void *dst, size_t len) {
  dosmemget(addr, (int)len, dst);
}

static int ensure_dos_buffer(_go32_dpmi_seginfo *info, unsigned long *addr, int *ready, size_t bytes) {
  if (*ready) return 0;
  if (alloc_dos(bytes, info)) return -1;
  *addr = dos_addr(info);
  *ready = 1;
  return 0;
}

static int glide_activate(void) {
  const char *env;

  if (glide_active) return 1;
  env = getenv("GLIDE");
  if (!env) {
    return 0;
  }

  glide_port = (unsigned short)strtoul(env, NULL, 16);
  outportb(glide_port, 0xFF);
  cmd_seg = (unsigned short)inportb(glide_port);
  cmd_seg |= ((unsigned short)inportb(glide_port)) << 8;
  if (cmd_seg == 0) return 0;

  cmd_phys = ((unsigned long)cmd_seg) << 4;
  glide_active = 1;
  return 1;
}

static void glide_issue(unsigned char fn, unsigned long retptr, const unsigned long *args, int count) {
  unsigned long params[20];
  int i;

  memset(params, 0, sizeof(params));
  params[0] = retptr;
  for (i = 0; i < count && i < 19; i++) params[i + 1] = args[i];
  dos_put(cmd_phys, params, sizeof(params));
  outportb(glide_port, fn);
  (void)inportb(glide_port);
}

FX_ENTRY void FX_CALL grGlideGetVersion(char version[80]) {
  static const char text[] = "DOSBox-X passthrough";
  strncpy(version, text, 80);
  version[79] = '\0';
}

FX_ENTRY void FX_CALL grGlideInit(void) {
  unsigned long args[2];
  unsigned long magic = 0xFFFFF1FBu;
  unsigned long pages = 0;

  if (!glide_activate()) return;
  if (ensure_dos_buffer(&handmem, &handaddr, &handready, 8)) return;
  dos_put(handaddr, &magic, sizeof(magic));
  dos_put(handaddr + 4, &pages, sizeof(pages));
  args[0] = handaddr;
  args[1] = handaddr + 4;
  glide_issue(F_grGlideInit, 0, args, 2);
}

FX_ENTRY void FX_CALL grGlideShutdown(void) {
  if (!glide_activate()) return;
  glide_issue(F_grGlideShutdown, 0, NULL, 0);
}

FX_ENTRY FxBool FX_CALL grSstQueryHardware(GrHwConfiguration *hwconfig) {
  unsigned long args[1];
  unsigned long retv = 0;

  if (!glide_activate()) return FXFALSE;
  if (ensure_dos_buffer(&hwmem, &hwaddr, &hwready, sizeof(*hwconfig))) return FXFALSE;
  memset(hwconfig, 0, sizeof(*hwconfig));
  dos_put(hwaddr, hwconfig, sizeof(*hwconfig));
  args[0] = hwaddr;
  glide_issue(F_grSstQueryHardware, cmd_phys, args, 1);
  dos_get(hwaddr, hwconfig, sizeof(*hwconfig));
  dos_get(cmd_phys, &retv, sizeof(retv));
  return retv ? FXTRUE : FXFALSE;
}

FX_ENTRY void FX_CALL grSstSelect(int which_sst) {
  unsigned long args[1];
  if (!glide_activate()) return;
  args[0] = (unsigned long)which_sst;
  glide_issue(F_grSstSelect, 0, args, 1);
}

FX_ENTRY void FX_CALL grSstIdle(void) {
  if (!glide_activate()) return;
  glide_issue(F_grSstIdle, 0, NULL, 0);
}

FX_ENTRY FxBool FX_CALL grSstWinOpen(FxU32 hWnd, GrScreenResolution_t res, GrScreenRefresh_t refresh,
                                     GrColorFormat_t format, GrOriginLocation_t origin,
                                     int nColorBuffers, int nAuxBuffers) {
  unsigned long args[11];
  unsigned long retv = 0;
  unsigned long lfb_linear = 0;

  if (!glide_activate()) return FXFALSE;
  if (ensure_dos_buffer(&lfbmem, &lfbaddr, &lfbready, sizeof(lfb_linear))) return FXFALSE;
  dos_put(lfbaddr, &lfb_linear, sizeof(lfb_linear));
  args[0] = hWnd;
  args[1] = res;
  args[2] = refresh;
  args[3] = format;
  args[4] = origin;
  args[5] = (unsigned long)nColorBuffers;
  args[6] = (unsigned long)nAuxBuffers;
  args[7] = 640;
  args[8] = 480;
  args[9] = 0;
  args[10] = lfbaddr;
  glide_issue(F_grSstWinOpen, cmd_phys, args, 11);
  dos_get(cmd_phys, &retv, sizeof(retv));
  return retv ? FXTRUE : FXFALSE;
}

FX_ENTRY void FX_CALL grBufferClear(GrColor_t color, GrAlpha_t alpha, FxU16 depth) {
  unsigned long args[3];
  if (!glide_activate()) return;
  args[0] = color;
  args[1] = alpha;
  args[2] = depth;
  glide_issue(F_grBufferClear, 0, args, 3);
}

FX_ENTRY void FX_CALL grBufferSwap(int interval) {
  unsigned long args[1];
  if (!glide_activate()) return;
  args[0] = (unsigned long)interval;
  glide_issue(F_grBufferSwap, 0, args, 1);
}

FX_ENTRY void FX_CALL grClipWindow(FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy) {
  unsigned long args[4];
  if (!glide_activate()) return;
  args[0] = minx;
  args[1] = miny;
  args[2] = maxx;
  args[3] = maxy;
  glide_issue(F_grClipWindow, 0, args, 4);
}

FX_ENTRY void FX_CALL grConstantColorValue(GrColor_t color) {
  unsigned long args[1];
  if (!glide_activate()) return;
  args[0] = color;
  glide_issue(F_grConstantColorValue, 0, args, 1);
}

FX_ENTRY void FX_CALL grColorCombine(GrCombineFunction_t function, GrCombineFactor_t factor,
                                     GrCombineLocal_t local, GrCombineOther_t other,
                                     FxBool invert) {
  unsigned long args[5];
  if (!glide_activate()) return;
  args[0] = function;
  args[1] = factor;
  args[2] = local;
  args[3] = other;
  args[4] = invert;
  glide_issue(F_grColorCombine, 0, args, 5);
}

FX_ENTRY void FX_CALL grAlphaCombine(GrCombineFunction_t function, GrCombineFactor_t factor,
                                     GrCombineLocal_t local, GrCombineOther_t other,
                                     FxBool invert) {
  unsigned long args[5];
  if (!glide_activate()) return;
  args[0] = function;
  args[1] = factor;
  args[2] = local;
  args[3] = other;
  args[4] = invert;
  glide_issue(F_grAlphaCombine, 0, args, 5);
}

FX_ENTRY void FX_CALL grAlphaBlendFunction(GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df,
                                           GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df) {
  unsigned long args[4];
  if (!glide_activate()) return;
  args[0] = rgb_sf;
  args[1] = rgb_df;
  args[2] = alpha_sf;
  args[3] = alpha_df;
  glide_issue(F_grAlphaBlendFunction, 0, args, 4);
}

FX_ENTRY void FX_CALL grAlphaTestFunction(GrCmpFnc_t function) {
  unsigned long args[1];
  if (!glide_activate()) return;
  args[0] = function;
  glide_issue(F_grAlphaTestFunction, 0, args, 1);
}

FX_ENTRY void FX_CALL grAlphaTestReferenceValue(GrAlpha_t value) {
  unsigned long args[1];
  if (!glide_activate()) return;
  args[0] = value;
  glide_issue(F_grAlphaTestReferenceValue, 0, args, 1);
}

FX_ENTRY void FX_CALL grCullMode(GrCullMode_t mode) {
  unsigned long args[1];
  if (!glide_activate()) return;
  args[0] = mode;
  glide_issue(F_grCullMode, 0, args, 1);
}

FX_ENTRY void FX_CALL grDepthBufferFunction(GrCmpFnc_t func) {
  unsigned long args[1];
  if (!glide_activate()) return;
  args[0] = func;
  glide_issue(F_grDepthBufferFunction, 0, args, 1);
}

FX_ENTRY void FX_CALL grHints(GrHint_t type, FxU32 hintMask) {
  unsigned long args[2];
  if (!glide_activate()) return;
  args[0] = type;
  args[1] = hintMask;
  glide_issue(F_grHints, 0, args, 2);
}

FX_ENTRY void FX_CALL grSstOrigin(GrOriginLocation_t origin) {
  unsigned long args[1];
  if (!glide_activate()) return;
  args[0] = origin;
  glide_issue(F_grSstOrigin, 0, args, 1);
}

FX_ENTRY void FX_CALL grDrawPoint(const GrVertex *v) {
  unsigned long args[1];
  if (!glide_activate()) return;
  if (ensure_dos_buffer(&vertexmem, &vertexaddr, &vertexready, sizeof(*v))) return;
  dos_put(vertexaddr, v, sizeof(*v));
  args[0] = vertexaddr;
  glide_issue(F_grDrawPoint, 0, args, 1);
}

FX_ENTRY void FX_CALL grDrawTriangle(const GrVertex *a, const GrVertex *b, const GrVertex *c) {
  unsigned long args[3];
  if (!glide_activate()) return;
  if (ensure_dos_buffer(&triVertexmem, &triVertexaddr, &triVertexready, sizeof(GrVertex) * 3u)) return;
  dos_put(triVertexaddr, a, sizeof(GrVertex));
  dos_put(triVertexaddr + sizeof(GrVertex), b, sizeof(GrVertex));
  dos_put(triVertexaddr + sizeof(GrVertex) * 2u, c, sizeof(GrVertex));
  args[0] = triVertexaddr;
  args[1] = triVertexaddr + sizeof(GrVertex);
  args[2] = triVertexaddr + sizeof(GrVertex) * 2u;
  glide_issue(F_grDrawTriangle, 0, args, 3);
}

static void fill_dbtexinfo(DBGrTexInfo *db, const GrTexInfo *info) {
  db->smallLod = info->smallLod;
  db->largeLod = info->largeLod;
  db->aspectRatio = info->aspectRatio;
  db->format = info->format;
  db->data = (unsigned long)info->data;
}

static unsigned lod_size_value(GrLOD_t lod) {
  switch (lod) {
    case GR_LOD_256: return 256;
    case GR_LOD_128: return 128;
    case GR_LOD_64: return 64;
    case GR_LOD_32: return 32;
    case GR_LOD_16: return 16;
    case GR_LOD_8: return 8;
    case GR_LOD_4: return 4;
    case GR_LOD_2: return 2;
    default: return 1;
  }
}

static void lod_dims(GrLOD_t lod, GrAspectRatio_t aspect, unsigned *w, unsigned *h) {
  unsigned base = lod_size_value(lod);
  switch (aspect) {
    case GR_ASPECT_8x1: *w = base; *h = base / 8; break;
    case GR_ASPECT_4x1: *w = base; *h = base / 4; break;
    case GR_ASPECT_2x1: *w = base; *h = base / 2; break;
    case GR_ASPECT_1x1: *w = base; *h = base; break;
    case GR_ASPECT_1x2: *w = base / 2; *h = base; break;
    case GR_ASPECT_1x4: *w = base / 4; *h = base; break;
    case GR_ASPECT_1x8: *w = base / 8; *h = base; break;
    default: *w = base; *h = base; break;
  }
}

static unsigned texel_bytes(GrTextureFormat_t format) {
  switch (format) {
    case GR_TEXFMT_RGB_332:
    case GR_TEXFMT_YIQ_422:
    case GR_TEXFMT_ALPHA_8:
    case GR_TEXFMT_INTENSITY_8:
    case GR_TEXFMT_ALPHA_INTENSITY_44:
    case GR_TEXFMT_P_8:
      return 1;
    default:
      return 2;
  }
}

static size_t level_bytes(GrLOD_t lod, GrAspectRatio_t aspect, GrTextureFormat_t format) {
  unsigned w, h;
  lod_dims(lod, aspect, &w, &h);
  return (size_t)w * (size_t)h * texel_bytes(format);
}

static size_t texture_bytes(const GrTexInfo *info) {
  size_t total = 0;
  GrLOD_t lod;
  for (lod = info->largeLod; lod <= info->smallLod; lod++) {
    total += level_bytes(lod, info->aspectRatio, info->format);
    if (lod == info->smallLod) break;
  }
  return total;
}

static unsigned char *stage_tex_data(const void *src, size_t bytes) {
  if (!tex_data_bytes || tex_data_bytes < bytes) {
    if (tex_data_bytes) {
      _go32_dpmi_free_dos_memory(&tex_data_mem);
      tex_data_addr = 0;
      tex_data_bytes = 0;
    }
    if (alloc_dos(bytes, &tex_data_mem)) return NULL;
    tex_data_addr = dos_addr(&tex_data_mem);
    tex_data_bytes = bytes;
  }
  dos_put(tex_data_addr, src, bytes);
  return (unsigned char *)tex_data_addr;
}

static unsigned char *stage_tex_data_padded(const void *src, size_t src_bytes, size_t total_bytes) {
  if (!tex_data_bytes || tex_data_bytes < total_bytes) {
    if (tex_data_bytes) {
      _go32_dpmi_free_dos_memory(&tex_data_mem);
      tex_data_addr = 0;
      tex_data_bytes = 0;
    }
    if (alloc_dos(total_bytes, &tex_data_mem)) return NULL;
    tex_data_addr = dos_addr(&tex_data_mem);
    tex_data_bytes = total_bytes;
  }
  dos_put(tex_data_addr, src, src_bytes);
  if (total_bytes > src_bytes) {
    static unsigned char zero_block[256];
    size_t off = src_bytes;
    while (off < total_bytes) {
      size_t chunk = (total_bytes - off) > sizeof(zero_block) ? sizeof(zero_block) : (total_bytes - off);
      dos_put(tex_data_addr + off, zero_block, chunk);
      off += chunk;
    }
  }
  return (unsigned char *)tex_data_addr;
}

static void *stage_table_data(const void *src, size_t bytes) {
  if (!table_bytes || table_bytes < bytes) {
    if (table_bytes) {
      _go32_dpmi_free_dos_memory(&table_mem);
      table_addr = 0;
      table_bytes = 0;
    }
    if (alloc_dos(bytes, &table_mem)) return NULL;
    table_addr = dos_addr(&table_mem);
    table_bytes = bytes;
  }
  dos_put(table_addr, src, bytes);
  return (void *)table_addr;
}

FX_ENTRY FxU32 FX_CALL grTexCalcMemRequired(GrLOD_t smallLod, GrLOD_t largeLod,
                                            GrAspectRatio_t aspect, GrTextureFormat_t format) {
  unsigned long args[4];
  unsigned long retv = 0;
  if (!glide_activate()) return 0;
  args[0] = smallLod;
  args[1] = largeLod;
  args[2] = aspect;
  args[3] = format;
  glide_issue(F_grTexCalcMemRequired, cmd_phys, args, 4);
  dos_get(cmd_phys, &retv, sizeof(retv));
  return retv;
}

FX_ENTRY FxU32 FX_CALL grTexTextureMemRequired(FxU32 evenOdd, GrTexInfo *info) {
  unsigned long args[2];
  unsigned long retv = 0;
  DBGrTexInfo db;
  if (!glide_activate()) return 0;
  if (ensure_dos_buffer(&texinfo_mem, &texinfo_addr, &texinfo_ready, sizeof(DBGrTexInfo))) return 0;
  fill_dbtexinfo(&db, info);
  dos_put(texinfo_addr, &db, sizeof(db));
  args[0] = evenOdd;
  args[1] = texinfo_addr;
  glide_issue(F_grTexTextureMemRequired, cmd_phys, args, 2);
  dos_get(cmd_phys, &retv, sizeof(retv));
  return retv;
}

FX_ENTRY FxU32 FX_CALL grTexMinAddress(GrChipID_t tmu) {
  unsigned long args[1], retv = 0;
  if (!glide_activate()) return 0;
  args[0] = tmu;
  glide_issue(F_grTexMinAddress, cmd_phys, args, 1);
  dos_get(cmd_phys, &retv, sizeof(retv));
  return retv;
}

FX_ENTRY FxU32 FX_CALL grTexMaxAddress(GrChipID_t tmu) {
  unsigned long args[1], retv = 0;
  if (!glide_activate()) return 0;
  args[0] = tmu;
  glide_issue(F_grTexMaxAddress, cmd_phys, args, 1);
  dos_get(cmd_phys, &retv, sizeof(retv));
  return retv;
}

FX_ENTRY void FX_CALL grTexMipMapMode(GrChipID_t tmu, GrMipMapMode_t mode, FxBool lodBlend) {
  unsigned long args[3];
  if (!glide_activate()) return;
  args[0] = tmu;
  args[1] = mode;
  args[2] = lodBlend;
  glide_issue(F_grTexMipMapMode, 0, args, 3);
}

FX_ENTRY void FX_CALL grTexFilterMode(GrChipID_t tmu, GrTextureFilterMode_t minFilterMode,
                                      GrTextureFilterMode_t magFilterMode) {
  unsigned long args[3];
  if (!glide_activate()) return;
  args[0] = tmu;
  args[1] = minFilterMode;
  args[2] = magFilterMode;
  glide_issue(F_grTexFilterMode, 0, args, 3);
}

FX_ENTRY void FX_CALL grTexLodBiasValue(GrChipID_t tmu, float bias) {
  unsigned long args[2];
  if (!glide_activate()) return;
  memcpy(&args[1], &bias, sizeof(bias));
  args[0] = tmu;
  glide_issue(F_grTexLodBiasValue, 0, args, 2);
}

FX_ENTRY void FX_CALL grTexCombine(GrChipID_t tmu, GrCombineFunction_t rgb_function,
                                   GrCombineFactor_t rgb_factor, GrCombineFunction_t alpha_function,
                                   GrCombineFactor_t alpha_factor, FxBool rgb_invert,
                                   FxBool alpha_invert) {
  unsigned long args[7];
  if (!glide_activate()) return;
  args[0] = tmu;
  args[1] = rgb_function;
  args[2] = rgb_factor;
  args[3] = alpha_function;
  args[4] = alpha_factor;
  args[5] = rgb_invert;
  args[6] = alpha_invert;
  glide_issue(F_grTexCombine, 0, args, 7);
}

FX_ENTRY void FX_CALL grTexDownloadTable(GrChipID_t tmu, GrTexTable_t type, void *data) {
  unsigned long args[3];
  size_t bytes;
  void *staged;
  if (!glide_activate()) return;
  bytes = (type == GR_TEXTABLE_PALETTE) ? sizeof(GuTexPalette) : sizeof(GuNccTable);
  staged = stage_table_data(data, bytes);
  if (!staged) return;
  args[0] = tmu;
  args[1] = type;
  args[2] = (unsigned long)staged;
  glide_issue(F_grTexDownloadTable, 0, args, 3);
}

FX_ENTRY void FX_CALL grTexDownloadMipMap(GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info) {
  unsigned long args[4];
  DBGrTexInfo db;
  size_t bytes;
  void *staged;
  if (!glide_activate()) return;
  if (ensure_dos_buffer(&texinfo_mem, &texinfo_addr, &texinfo_ready, sizeof(DBGrTexInfo))) return;
  fill_dbtexinfo(&db, info);
  bytes = grTexTextureMemRequired(evenOdd, info);
  staged = stage_tex_data_padded(info->data, texture_bytes(info), bytes);
  if (!staged) return;
  db.data = (unsigned long)staged;
  dos_put(texinfo_addr, &db, sizeof(db));
  args[0] = tmu;
  args[1] = startAddress;
  args[2] = evenOdd;
  args[3] = texinfo_addr;
  glide_issue(F_grTexDownloadMipMap, 0, args, 4);
}

FX_ENTRY void FX_CALL grTexDownloadMipMapLevel(GrChipID_t tmu, FxU32 startAddress,
                                               GrLOD_t thisLod, GrLOD_t largeLod,
                                               GrAspectRatio_t aspectRatio, GrTextureFormat_t format,
                                               FxU32 evenOdd, void *data) {
  unsigned long args[8];
  size_t bytes;
  unsigned char *staged;
  if (!glide_activate()) return;
  bytes = level_bytes(thisLod, aspectRatio, format);
  staged = stage_tex_data(data, bytes);
  if (!staged) return;
  args[0] = tmu;
  args[1] = startAddress;
  args[2] = thisLod;
  args[3] = largeLod;
  args[4] = aspectRatio;
  args[5] = format;
  args[6] = evenOdd;
  args[7] = (unsigned long)staged;
  glide_issue(F_grTexDownloadMipMapLevel, 0, args, 8);
}

FX_ENTRY void FX_CALL grTexDownloadMipMapLevelPartial(GrChipID_t tmu, FxU32 startAddress,
                                                      GrLOD_t thisLod, GrLOD_t largeLod,
                                                      GrAspectRatio_t aspectRatio, GrTextureFormat_t format,
                                                      FxU32 evenOdd, void *data, int start, int end) {
  unsigned long args[10];
  size_t bytes;
  unsigned char *staged;
  if (!glide_activate()) return;
  bytes = level_bytes(thisLod, aspectRatio, format);
  staged = stage_tex_data(data, bytes);
  if (!staged) return;
  args[0] = tmu;
  args[1] = startAddress;
  args[2] = thisLod;
  args[3] = largeLod;
  args[4] = aspectRatio;
  args[5] = format;
  args[6] = evenOdd;
  args[7] = (unsigned long)staged;
  args[8] = (unsigned long)start;
  args[9] = (unsigned long)end;
  glide_issue(F_grTexDownloadMipMapLevelPartial, 0, args, 10);
}

FX_ENTRY void FX_CALL grTexSource(GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info) {
  unsigned long args[4];
  DBGrTexInfo db;
  if (!glide_activate()) return;
  if (ensure_dos_buffer(&texinfo_mem, &texinfo_addr, &texinfo_ready, sizeof(DBGrTexInfo))) return;
  fill_dbtexinfo(&db, info);
  dos_put(texinfo_addr, &db, sizeof(db));
  args[0] = tmu;
  args[1] = startAddress;
  args[2] = evenOdd;
  args[3] = texinfo_addr;
  glide_issue(F_grTexSource, 0, args, 4);
}

FX_ENTRY void FX_CALL grTexMultibase(GrChipID_t tmu, FxBool enable) {
  unsigned long args[2];
  if (!glide_activate()) return;
  args[0] = tmu;
  args[1] = enable;
  glide_issue(F_grTexMultibase, 0, args, 2);
}

FX_ENTRY void FX_CALL grTexMultibaseAddress(GrChipID_t tmu, GrTexBaseRange_t range,
                                            FxU32 startAddress, FxU32 evenOdd,
                                            GrTexInfo *info) {
  unsigned long args[5];
  DBGrTexInfo db;
  if (!glide_activate()) return;
  if (ensure_dos_buffer(&texinfo_mem, &texinfo_addr, &texinfo_ready, sizeof(DBGrTexInfo))) return;
  fill_dbtexinfo(&db, info);
  dos_put(texinfo_addr, &db, sizeof(db));
  args[0] = tmu;
  args[1] = range;
  args[2] = startAddress;
  args[3] = evenOdd;
  args[4] = texinfo_addr;
  glide_issue(F_grTexMultibaseAddress, 0, args, 5);
}

FX_ENTRY void FX_CALL grGlideGetState(GrState *state) {
  unsigned long args[1];
  if (!glide_activate()) return;
  if (ensure_dos_buffer(&state_mem, &state_addr, &state_ready, sizeof(*state))) return;
  dos_put(state_addr, state, sizeof(*state));
  args[0] = state_addr;
  glide_issue(F_grGlideGetState, 0, args, 1);
  dos_get(state_addr, state, sizeof(*state));
}

FX_ENTRY void FX_CALL grGlideSetState(const GrState *state) {
  unsigned long args[1];
  if (!glide_activate()) return;
  if (ensure_dos_buffer(&state_mem, &state_addr, &state_ready, sizeof(*state))) return;
  dos_put(state_addr, state, sizeof(*state));
  args[0] = state_addr;
  glide_issue(F_grGlideSetState, 0, args, 1);
}

FX_ENTRY FxBool FX_CALL grLfbReadRegion(GrBuffer_t src_buffer, FxU32 src_x, FxU32 src_y,
                                        FxU32 src_width, FxU32 src_height,
                                        FxU32 dst_stride, void *dst_data) {
  unsigned long args[7];
  unsigned long retv = 0;
  size_t line_copy = (size_t)dst_stride;
  FxU32 y;

  if (!glide_activate()) return FXFALSE;

  if (!linebytes || linebytes < line_copy) {
    if (linebytes) {
      _go32_dpmi_free_dos_memory(&linebuf);
      linebytes = 0;
      lineaddr = 0;
    }
    if (alloc_dos(line_copy, &linebuf)) {
        return FXFALSE;
    }
    lineaddr = dos_addr(&linebuf);
    linebytes = line_copy;
  }

  for (y = 0; y < src_height; ++y) {
    args[0] = src_buffer;
    args[1] = src_x;
    args[2] = src_y + y;
    args[3] = src_width;
    args[4] = 1;
    args[5] = dst_stride;
    args[6] = lineaddr;
    glide_issue(F_grLfbReadRegion, cmd_phys, args, 7);
    dos_get(cmd_phys, &retv, sizeof(retv));
    if (!retv) {
        return FXFALSE;
    }
    dos_get(lineaddr, ((unsigned char *)dst_data) + ((size_t)y * dst_stride), line_copy);
  }

  return FXTRUE;
}
