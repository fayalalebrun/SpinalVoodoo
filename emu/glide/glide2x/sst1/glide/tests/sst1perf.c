#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <glide.h>
#include <glideutl.h>

#define DEFAULT_TARGET_SECONDS 0.35
#define DEFAULT_CLEAR_TARGET_SECONDS 0.20
#define MAX_BATCH_TRIANGLES 4096
#define DE10_MMIO_BASE 0xC0000000u
#define DE10_MMIO_SPAN 0x1000u

typedef enum {
  MODE_FLAT = 0,
  MODE_GOURAUD_FOG_ALPHA_Z,
  MODE_TEXTURED_FOG,
  MODE_TEXTURED_FOG_ALPHA_Z
} BenchMode;

typedef struct {
  const char* label;
  double area_pixels;
  FxBool random_orientation;
} BenchShape;

typedef struct {
  const char* label;
  BenchMode mode;
  double area_pixels;
  FxBool random_orientation;
  double seconds;
  FxU32 tris_processed;
  FxU32 tris_drawn;
  FxU32 pixels_in;
  FxU32 pixels_out;
  FxU32 chroma_fail;
  FxU32 z_fail;
  FxU32 alpha_fail;
  FxU32 write_path_debug_start;
  FxU32 write_path_debug_end;
} BenchResult;

typedef struct {
  const char* label;
  FxBool color_mask;
  FxBool depth_mask;
  double seconds;
  int clears;
  FxU32 pixels_in;
  FxU32 pixels_out;
  FxU32 write_path_debug_start;
  FxU32 write_path_debug_end;
} ClearResult;

static GrHwConfiguration hwconfig;
static FxU16 texture_lod0[64 * 64];
static FxU16 texture_lod1[32 * 32];
static FxU16 texture_lod2[16 * 16];
static FxU16 texture_lod3[8 * 8];
static FxU16 texture_lod4[4 * 4];
static FxU16 texture_lod5[2 * 2];
static FxU16 texture_lod6[1];
static GrFog_t fog_table[GR_FOG_TABLE_SIZE];
static FxU32 texture_addr;
static GrTexInfo texture_info;
static volatile uint8_t* mmio_regs;
static void* mmio_mapping;
static size_t mmio_map_size;

typedef struct {
  FxU32 pixels_in;
  FxU32 pixels_out;
  FxU32 chroma_fail;
  FxU32 z_fail;
  FxU32 alpha_fail;
  FxU32 write_path_debug;
} CounterSnapshot;

static double now_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static double env_seconds(const char* name, double fallback) {
  const char* value = getenv(name);
  char* end = NULL;
  double parsed;

  if (value == NULL || *value == '\0') {
    return fallback;
  }
  parsed = strtod(value, &end);
  if (end == value || parsed <= 0.0) {
    return fallback;
  }
  return parsed;
}

static int env_int(const char* name, int fallback) {
  const char* value = getenv(name);
  char* end = NULL;
  long parsed;

  if (value == NULL || *value == '\0') {
    return fallback;
  }
  parsed = strtol(value, &end, 10);
  if (end == value || parsed <= 0) {
    return fallback;
  }
  return (int)parsed;
}

static void print_case_begin(const char* group, const char* label) {
  printf("case-begin group=%s shape=%s\n", group, label);
}

static void print_case_end(const char* group, const char* label) {
  printf("case-end group=%s shape=%s\n", group, label);
}

static FxBool init_mmio(void) {
  long page_size = sysconf(_SC_PAGESIZE);
  FxU32 map_base;
  FxU32 page_offset;
  int fd;

  if (page_size <= 0) {
    return FXFALSE;
  }

  map_base = DE10_MMIO_BASE & ~((FxU32)page_size - 1u);
  page_offset = DE10_MMIO_BASE - map_base;
  mmio_map_size = page_offset + DE10_MMIO_SPAN;

  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    return FXFALSE;
  }

  mmio_mapping = mmap(NULL, mmio_map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)map_base);
  close(fd);
  if (mmio_mapping == MAP_FAILED) {
    mmio_mapping = NULL;
    return FXFALSE;
  }

  mmio_regs = (volatile uint8_t*)mmio_mapping + page_offset;
  return FXTRUE;
}

static void shutdown_mmio(void) {
  if (mmio_mapping != NULL) {
    munmap(mmio_mapping, mmio_map_size);
    mmio_mapping = NULL;
    mmio_regs = NULL;
    mmio_map_size = 0;
  }
}

static FxU32 mmio_read32(FxU32 offset) {
  return *(volatile FxU32*)(mmio_regs + offset);
}

static CounterSnapshot snapshot_counters(void) {
  CounterSnapshot snap;

  memset(&snap, 0, sizeof(snap));
  if (mmio_regs != NULL) {
    snap.pixels_in = mmio_read32(0x14cu);
    snap.chroma_fail = mmio_read32(0x150u);
    snap.z_fail = mmio_read32(0x154u);
    snap.alpha_fail = mmio_read32(0x158u);
    snap.pixels_out = mmio_read32(0x15cu);
    snap.write_path_debug = mmio_read32(0x250u);
  }
  return snap;
}

static FxU32 counter_delta(FxU32 start, FxU32 end) {
  return end - start;
}

static float frand_unit(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return (float)((*state >> 8) & 0x00ffffffu) / 16777215.0f;
}

static FxU16 pack_565(int r, int g, int b) {
  return (FxU16)(((r & 0x1f) << 11) | ((g & 0x3f) << 5) | (b & 0x1f));
}

static void build_texture_level(FxU16* dst, int size, int shift) {
  int y;
  for (y = 0; y < size; ++y) {
    int x;
    for (x = 0; x < size; ++x) {
      int checker = ((x >> (shift > 0 ? shift : 0)) ^ (y >> (shift > 0 ? shift : 0))) & 1;
      int r = checker ? 31 : 3;
      int g = (x * 63) / (size - 1 ? size - 1 : 1);
      int b = (y * 31) / (size - 1 ? size - 1 : 1);
      if (((x + y) & 7) == 0) {
        r = 31;
        g = 63;
        b = 31;
      }
      dst[y * size + x] = pack_565(r, g, b);
    }
  }
}

static void build_textures(void) {
  build_texture_level(texture_lod0, 64, 3);
  build_texture_level(texture_lod1, 32, 2);
  build_texture_level(texture_lod2, 16, 1);
  build_texture_level(texture_lod3, 8, 1);
  build_texture_level(texture_lod4, 4, 0);
  build_texture_level(texture_lod5, 2, 0);
  texture_lod6[0] = pack_565(31, 63, 31);
}

static void upload_texture(void) {
  memset(&texture_info, 0, sizeof(texture_info));
  texture_info.smallLod = GR_LOD_1;
  texture_info.largeLod = GR_LOD_64;
  texture_info.aspectRatio = GR_ASPECT_1x1;
  texture_info.format = GR_TEXFMT_RGB_565;
  texture_info.data = texture_lod0;

  texture_addr = grTexMinAddress(GR_TMU0);
  grTexDownloadMipMapLevel(GR_TMU0, texture_addr, GR_LOD_64, GR_LOD_64,
                           GR_ASPECT_1x1, GR_TEXFMT_RGB_565,
                           GR_MIPMAPLEVELMASK_BOTH, texture_lod0);
  grTexDownloadMipMapLevel(GR_TMU0, texture_addr, GR_LOD_32, GR_LOD_64,
                           GR_ASPECT_1x1, GR_TEXFMT_RGB_565,
                           GR_MIPMAPLEVELMASK_BOTH, texture_lod1);
  grTexDownloadMipMapLevel(GR_TMU0, texture_addr, GR_LOD_16, GR_LOD_64,
                           GR_ASPECT_1x1, GR_TEXFMT_RGB_565,
                           GR_MIPMAPLEVELMASK_BOTH, texture_lod2);
  grTexDownloadMipMapLevel(GR_TMU0, texture_addr, GR_LOD_8, GR_LOD_64,
                           GR_ASPECT_1x1, GR_TEXFMT_RGB_565,
                           GR_MIPMAPLEVELMASK_BOTH, texture_lod3);
  grTexDownloadMipMapLevel(GR_TMU0, texture_addr, GR_LOD_4, GR_LOD_64,
                           GR_ASPECT_1x1, GR_TEXFMT_RGB_565,
                           GR_MIPMAPLEVELMASK_BOTH, texture_lod4);
  grTexDownloadMipMapLevel(GR_TMU0, texture_addr, GR_LOD_2, GR_LOD_64,
                           GR_ASPECT_1x1, GR_TEXFMT_RGB_565,
                           GR_MIPMAPLEVELMASK_BOTH, texture_lod5);
  grTexDownloadMipMapLevel(GR_TMU0, texture_addr, GR_LOD_1, GR_LOD_64,
                           GR_ASPECT_1x1, GR_TEXFMT_RGB_565,
                           GR_MIPMAPLEVELMASK_BOTH, texture_lod6);
}

static void setup_common_state(void) {
  grClipWindow(0, 0, 640, 480);
  grRenderBuffer(GR_BUFFER_BACKBUFFER);
  grColorMask(FXTRUE, FXFALSE);
  grDepthMask(FXFALSE);
  grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO);
  grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_NONE, FXFALSE);
  grAlphaTestFunction(GR_CMP_ALWAYS);
  grDepthBufferFunction(GR_CMP_ALWAYS);
  grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
  grCullMode(GR_CULL_DISABLE);
  grFogMode(GR_FOG_DISABLE);
  grChromakeyMode(GR_CHROMAKEY_DISABLE);
  grConstantColorValue(0xffffffffu);
  grTexMipMapMode(GR_TMU0, GR_MIPMAP_NEAREST, FXFALSE);
  grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
  grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
  grTexSource(GR_TMU0, texture_addr, GR_MIPMAPLEVELMASK_BOTH, &texture_info);
}

static void setup_mode(BenchMode mode) {
  setup_common_state();

  switch (mode) {
    case MODE_FLAT:
      grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
                     GR_COMBINE_FACTOR_NONE,
                     GR_COMBINE_LOCAL_CONSTANT,
                     GR_COMBINE_OTHER_NONE,
                     FXFALSE);
      break;
    case MODE_GOURAUD_FOG_ALPHA_Z:
      grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
                     GR_COMBINE_FACTOR_NONE,
                     GR_COMBINE_LOCAL_ITERATED,
                     GR_COMBINE_OTHER_NONE,
                     FXFALSE);
      grFogMode(GR_FOG_WITH_TABLE);
      grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
                           GR_BLEND_ONE, GR_BLEND_ZERO);
      grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
      grDepthBufferFunction(GR_CMP_LESS);
      grDepthMask(FXTRUE);
      break;
    case MODE_TEXTURED_FOG:
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_ITERATED,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
      grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
      grTexMipMapMode(GR_TMU0, GR_MIPMAP_NEAREST, FXFALSE);
      grFogMode(GR_FOG_WITH_TABLE);
      break;
    case MODE_TEXTURED_FOG_ALPHA_Z:
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_ITERATED,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
      grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
      grTexMipMapMode(GR_TMU0, GR_MIPMAP_NEAREST, FXFALSE);
      grFogMode(GR_FOG_WITH_TABLE);
      grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
                           GR_BLEND_ONE, GR_BLEND_ZERO);
      grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
      grDepthBufferFunction(GR_CMP_LESS);
      grDepthMask(FXTRUE);
      break;
  }
}

static void fill_vertex_color(GrVertex* v, float r, float g, float b, float a) {
  v->r = r;
  v->g = g;
  v->b = b;
  v->a = a;
}

static void fill_vertex_tex(GrVertex* v, float s, float t, float oow) {
  v->oow = oow;
  v->tmuvtx[0].oow = oow;
  v->tmuvtx[0].sow = s * oow;
  v->tmuvtx[0].tow = t * oow;
}

static void make_triangle(GrVertex* a,
                          GrVertex* b,
                          GrVertex* c,
                          double area_pixels,
                          FxBool random_orientation,
                          uint32_t* rng_state) {
  float side = (float)sqrt(area_pixels * 2.0);
  float half = side * 0.5f;
  float cx;
  float cy;
  float angle;
  float cos_a;
  float sin_a;
  float local_x[3] = {0.0f, side, 0.0f};
  float local_y[3] = {0.0f, 0.0f, side};
  int i;
  GrVertex* verts[3] = {a, b, c};

  if (random_orientation) {
    angle = frand_unit(rng_state) * 6.28318530718f;
    cx = half + frand_unit(rng_state) * (640.0f - side - 2.0f);
    cy = half + frand_unit(rng_state) * (480.0f - side - 2.0f);
  } else {
    angle = 0.0f;
    cx = half + frand_unit(rng_state) * (640.0f - side - 2.0f);
    cy = half + frand_unit(rng_state) * (480.0f - side - 2.0f);
  }

  cos_a = cosf(angle);
  sin_a = sinf(angle);

  for (i = 0; i < 3; ++i) {
    float lx = local_x[i] - half;
    float ly = local_y[i] - half;
    float x = cx + (lx * cos_a) - (ly * sin_a);
    float y = cy + (lx * sin_a) + (ly * cos_a);
    verts[i]->x = x;
    verts[i]->y = y;
    verts[i]->z = 0.0f;
  }

  a->ooz = 45000.0f;
  b->ooz = 25000.0f;
  c->ooz = 10000.0f;

  fill_vertex_color(a, 255.0f, 64.0f, 48.0f, 192.0f);
  fill_vertex_color(b, 48.0f, 255.0f, 96.0f, 160.0f);
  fill_vertex_color(c, 48.0f, 96.0f, 255.0f, 128.0f);

  fill_vertex_tex(a, 0.0f, 0.0f, 1.0f);
  fill_vertex_tex(b, 63.0f, 0.0f, 0.75f);
  fill_vertex_tex(c, 0.0f, 63.0f, 0.5f);
}

static BenchResult run_triangle_bench(const char* label,
                                      BenchMode mode,
                                      double area_pixels,
                                      FxBool random_orientation,
                                      double target_seconds) {
  BenchResult result;
  double start;
  double elapsed;
  int batch_triangles = 64;
  uint32_t rng_state = 0x12345678u ^ (uint32_t)(area_pixels * 100.0);
  CounterSnapshot start_counters;
  CounterSnapshot end_counters;

  memset(&result, 0, sizeof(result));
  result.label = label;
  result.mode = mode;
  result.area_pixels = area_pixels;
  result.random_orientation = random_orientation;

  setup_mode(mode);
  grBufferClear(0x00000000u, 0, GR_WDEPTHVALUE_FARTHEST);
  grSstIdle();
  grSstResetPerfStats();
  grResetTriStats();
  start_counters = snapshot_counters();

  start = now_seconds();
  elapsed = 0.0;
  while (elapsed < target_seconds) {
    int i;
    for (i = 0; i < batch_triangles; ++i) {
      GrVertex a;
      GrVertex b;
      GrVertex c;
      make_triangle(&a, &b, &c, area_pixels, random_orientation, &rng_state);
      grDrawTriangle(&a, &b, &c);
    }
    if (batch_triangles < MAX_BATCH_TRIANGLES) {
      batch_triangles *= 2;
      if (batch_triangles > MAX_BATCH_TRIANGLES) {
        batch_triangles = MAX_BATCH_TRIANGLES;
      }
    }
    elapsed = now_seconds() - start;
  }

  grSstIdle();
  elapsed = now_seconds() - start;

  {
    GrSstPerfStats_t stats;
    grSstPerfStats(&stats);
    grTriStats(&result.tris_processed, &result.tris_drawn);
    end_counters = snapshot_counters();
    result.pixels_in = mmio_regs ? counter_delta(start_counters.pixels_in, end_counters.pixels_in) : stats.pixelsIn;
    result.pixels_out = mmio_regs ? counter_delta(start_counters.pixels_out, end_counters.pixels_out) : stats.pixelsOut;
    result.chroma_fail = mmio_regs ? counter_delta(start_counters.chroma_fail, end_counters.chroma_fail) : stats.chromaFail;
    result.z_fail = mmio_regs ? counter_delta(start_counters.z_fail, end_counters.z_fail) : stats.zFuncFail;
    result.alpha_fail = mmio_regs ? counter_delta(start_counters.alpha_fail, end_counters.alpha_fail) : stats.aFuncFail;
  }

  result.seconds = elapsed;
  result.write_path_debug_start = start_counters.write_path_debug;
  result.write_path_debug_end = end_counters.write_path_debug;
  return result;
}

static ClearResult run_clear_bench(const char* label, FxBool color_mask, FxBool depth_mask) {
  ClearResult result;
  double start;
  double elapsed;
  int clears = 0;
  const double target_seconds = env_seconds("SST1PERF_CLEAR_SECONDS", DEFAULT_CLEAR_TARGET_SECONDS);
  CounterSnapshot start_counters;
  CounterSnapshot end_counters;

  memset(&result, 0, sizeof(result));
  result.label = label;
  result.color_mask = color_mask;
  result.depth_mask = depth_mask;

  setup_common_state();
  grColorMask(color_mask, FXFALSE);
  grDepthMask(depth_mask);
  grDepthBufferMode(depth_mask ? GR_DEPTHBUFFER_WBUFFER : GR_DEPTHBUFFER_DISABLE);
  grDepthBufferFunction(GR_CMP_ALWAYS);
  grSstIdle();
  grSstResetPerfStats();
  start_counters = snapshot_counters();

  start = now_seconds();
  elapsed = 0.0;
  while (elapsed < target_seconds) {
    grBufferClear(0x00202020u, 0, GR_WDEPTHVALUE_FARTHEST);
    ++clears;
    elapsed = now_seconds() - start;
  }

  grSstIdle();
  end_counters = snapshot_counters();
  elapsed = now_seconds() - start;
  result.seconds = elapsed;
  result.clears = clears;
  result.pixels_in = counter_delta(start_counters.pixels_in, end_counters.pixels_in);
  result.pixels_out = counter_delta(start_counters.pixels_out, end_counters.pixels_out);
  result.write_path_debug_start = start_counters.write_path_debug;
  result.write_path_debug_end = end_counters.write_path_debug;
  return result;
}

static void print_triangle_result(const BenchResult* result) {
  double ktri_sec = (result->seconds > 0.0)
                      ? ((double)result->tris_drawn / result->seconds) / 1000.0
                      : 0.0;
  double mpix_sec = (result->seconds > 0.0)
                      ? ((double)result->pixels_out / result->seconds) / 1000000.0
                      : 0.0;

  printf("%-88s | %8.1f Ktri/s | %7.2f Mpix/s | tris=%-8u pixels=%-9u time=%0.3fs\n",
         result->label,
         ktri_sec,
         mpix_sec,
         (unsigned)result->tris_drawn,
         (unsigned)result->pixels_out,
         result->seconds);
  printf("  raw counters: in=%u out=%u chroma=%u z=%u a=%u debug=0x%08x->0x%08x\n",
         (unsigned)result->pixels_in,
         (unsigned)result->pixels_out,
         (unsigned)result->chroma_fail,
         (unsigned)result->z_fail,
         (unsigned)result->alpha_fail,
         (unsigned)result->write_path_debug_start,
         (unsigned)result->write_path_debug_end);
}

static void print_clear_result(const ClearResult* result) {
  double ms_per_clear = (result->clears > 0)
                          ? (result->seconds * 1000.0) / (double)result->clears
                          : 0.0;
  double mpix_sec = (ms_per_clear > 0.0)
                      ? (640.0 * 480.0 * ((result->color_mask ? 1.0 : 0.0) + (result->depth_mask ? 1.0 : 0.0)))
                          / (ms_per_clear / 1000.0) / 1000000.0
                      : 0.0;

  printf("%-32s | %7.3f ms | %7.2f Mpix/s | clears=%d\n",
         result->label,
         ms_per_clear,
         mpix_sec,
         result->clears);
  printf("  raw counters: in=%u out=%u debug=0x%08x->0x%08x\n",
         (unsigned)result->pixels_in,
         (unsigned)result->pixels_out,
         (unsigned)result->write_path_debug_start,
         (unsigned)result->write_path_debug_end);
}

int main(void) {
  static const BenchShape shapes[4] = {
    {"10-pixel, right-angled, horizontally oriented", 10.0, FXFALSE},
    {"25-pixel, right-angled, horizontally oriented", 25.0, FXFALSE},
    {"50-pixel, right-angled, horizontally oriented", 50.0, FXFALSE},
    {"1000-pixel, right-angled, horizontally oriented", 1000.0, FXFALSE}
  };
  static const BenchShape random_shapes[4] = {
    {"10-pixel, right-angled, randomly oriented", 10.0, FXTRUE},
    {"25-pixel, right-angled, randomly oriented", 25.0, FXTRUE},
    {"50-pixel, right-angled, randomly oriented", 50.0, FXTRUE},
    {"1000-pixel, right-angled, randomly oriented", 1000.0, FXTRUE}
  };
  int i;
  ClearResult clear_result;
  const double target_seconds = env_seconds("SST1PERF_TARGET_SECONDS", DEFAULT_TARGET_SECONDS);
  const double clear_seconds = env_seconds("SST1PERF_CLEAR_SECONDS", DEFAULT_CLEAR_TARGET_SECONDS);
  const int max_cases = env_int("SST1PERF_MAX_CASES", 4);
  const int mode_mask = env_int("SST1PERF_MODE_MASK", 0x0f);
  const int shape_count = (max_cases < 4) ? max_cases : 4;

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  printf("sst1perf: SST-1 style Glide benchmark\n");
  grGlideInit();
  printf("after grGlideInit\n");
  if (!grSstQueryHardware(&hwconfig)) {
    fprintf(stderr, "grSstQueryHardware failed\n");
    return 2;
  }
  grSstSelect(0);
  printf("after grSstSelect\n");
  if (!grSstWinOpen(0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz,
                    GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, 2, 1)) {
    fprintf(stderr, "grSstWinOpen failed\n");
    grGlideShutdown();
    return 3;
  }
  printf("after grSstWinOpen\n");
  printf("mmio=%s\n", init_mmio() ? "ok" : "unavailable");

  build_textures();
  printf("after build_textures\n");
  guFogGenerateExp(fog_table, 0.25f);
  grFogColorValue(0x404040u);
  grFogTable(fog_table);
  upload_texture();
  printf("after upload_texture\n");

  printf("board_count=%d type=%d\n", hwconfig.num_sst, hwconfig.SSTs[0].type);
  printf("resolution=640x480 refresh=60Hz target_seconds=%.2f clear_seconds=%.2f\n",
         target_seconds, clear_seconds);
  printf("mode_mask=0x%x shape_count=%d\n", mode_mask, shape_count);
  printf("\nTriangles\n");
  if (mode_mask & 0x1) {
    for (i = 0; i < shape_count; ++i) {
      BenchResult result;
      print_case_begin("flat", shapes[i].label);
      result = run_triangle_bench(shapes[i].label,
                                  MODE_FLAT,
                                  shapes[i].area_pixels,
                                  shapes[i].random_orientation,
                                  target_seconds);
      print_triangle_result(&result);
      print_case_end("flat", shapes[i].label);
    }
  }
  if (mode_mask & 0x2) {
    for (i = 0; i < shape_count; ++i) {
      BenchResult result;
      print_case_begin("gouraud_fog_alpha_z", random_shapes[i].label);
      result = run_triangle_bench(random_shapes[i].label,
                                  MODE_GOURAUD_FOG_ALPHA_Z,
                                  random_shapes[i].area_pixels,
                                  random_shapes[i].random_orientation,
                                  target_seconds);
      print_triangle_result(&result);
      print_case_end("gouraud_fog_alpha_z", random_shapes[i].label);
    }
  }
  if (mode_mask & 0x4) {
    for (i = 0; i < shape_count; ++i) {
      BenchResult result;
      print_case_begin("textured_fog", random_shapes[i].label);
      result = run_triangle_bench(random_shapes[i].label,
                                  MODE_TEXTURED_FOG,
                                  random_shapes[i].area_pixels,
                                  random_shapes[i].random_orientation,
                                  target_seconds);
      print_triangle_result(&result);
      print_case_end("textured_fog", random_shapes[i].label);
    }
  }
  if (mode_mask & 0x8) {
    for (i = 0; i < shape_count; ++i) {
      BenchResult result;
      print_case_begin("textured_fog_alpha_z", random_shapes[i].label);
      result = run_triangle_bench(random_shapes[i].label,
                                  MODE_TEXTURED_FOG_ALPHA_Z,
                                  random_shapes[i].area_pixels,
                                  random_shapes[i].random_orientation,
                                  target_seconds);
      print_triangle_result(&result);
      print_case_end("textured_fog_alpha_z", random_shapes[i].label);
    }
  }

  printf("\nScreen Clears\n");
  clear_result = run_clear_bench("RGB buffer", FXTRUE, FXFALSE);
  print_clear_result(&clear_result);
  clear_result = run_clear_bench("Depth buffer", FXFALSE, FXTRUE);
  print_clear_result(&clear_result);
  clear_result = run_clear_bench("RGB and depth buffer", FXTRUE, FXTRUE);
  print_clear_result(&clear_result);

  grGlideShutdown();
  shutdown_mmio();
  return 0;
}
