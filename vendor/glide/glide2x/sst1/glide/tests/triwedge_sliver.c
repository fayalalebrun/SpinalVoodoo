#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glide.h>

#define TEX_DIM 64

static FxU16 texture[TEX_DIM * TEX_DIM];

static void fail(const char* stage) {
  printf("triwedge_sliver: FAIL %s\n", stage);
  grGlideShutdown();
  exit(1);
}

static FxU16 pack565(int r, int g, int b) {
  return (FxU16)(((r & 0x1f) << 11) | ((g & 0x3f) << 5) | (b & 0x1f));
}

static void build_texture(int seed) {
  int x;
  int y;
  for (y = 0; y < TEX_DIM; ++y) {
    for (x = 0; x < TEX_DIM; ++x) {
      int r = ((seed * 3) + x + y) & 31;
      int g = ((seed * 5) + x * 3 + y * 7) & 63;
      int b = ((seed * 11) + x * 13 + y * 5) & 31;
      if (((x + y + seed) & 15) == 0) {
        r = 31;
        g = 63;
        b = 31;
      }
      texture[y * TEX_DIM + x] = pack565(r, g, b);
    }
  }
}

static void upload_texture(FxU32 base_addr, int seed) {
  build_texture(seed);
  grTexDownloadMipMapLevel(GR_TMU0,
                           base_addr,
                           GR_LOD_64,
                           GR_LOD_64,
                           GR_ASPECT_1x1,
                           GR_TEXFMT_RGB_565,
                           GR_MIPMAPLEVELMASK_BOTH,
                           texture);
}

static void set_vertex(GrVertex* v,
                       float x,
                       float y,
                       float oow,
                       float s,
                       float t,
                       float r,
                       float g,
                       float b,
                       float a) {
  v->x = x;
  v->y = y;
  v->z = 0.0f;
  v->ooz = 65535.0f * oow;
  v->oow = oow;
  v->r = r;
  v->g = g;
  v->b = b;
  v->a = a;
  v->tmuvtx[0].oow = oow;
  v->tmuvtx[0].sow = s * oow;
  v->tmuvtx[0].tow = t * oow;
}

static void draw_sliver_storm(float width, float height, int iter, int burst) {
  int tri;
  for (tri = 0; tri < 224; ++tri) {
    GrVertex a;
    GrVertex b;
    GrVertex c;
    float base_y = (float)(((tri * 17) + (iter * 29) + (burst * 13)) % ((int)height + 160)) - 80.0f;
    float span_h = 18.0f + (float)((tri * 9 + burst * 7) & 127);
    float sliver = 0.75f + 0.125f * (float)((tri + iter) & 7);
    float oow = 0.625f + 0.0625f * (float)((tri + burst) & 7);
    float alpha = 48.0f + (float)(((tri * 23) + (iter * 5)) & 127);
    float s0 = (float)((tri * 5 + iter * 7) & 63);
    float t0 = (float)((tri * 11 + burst * 3) & 63);
    float inner_x;
    float edge_x;

    if ((tri & 1) == 0) {
      edge_x = -24.0f - (float)((tri * 3) & 15);
      inner_x = 2.0f + (float)((tri + burst) & 3);
      set_vertex(&a, edge_x, base_y, oow, s0, t0, 255.0f, 64.0f, 64.0f, alpha);
      set_vertex(&b, inner_x, base_y + span_h, oow, s0 + 31.5f, t0 + 7.5f, 64.0f, 255.0f, 96.0f, alpha);
      set_vertex(&c, inner_x + sliver, base_y + 0.5f * span_h, oow, s0 + 7.5f, t0 + 47.5f, 96.0f, 96.0f, 255.0f, alpha);
    } else {
      edge_x = width + 24.0f + (float)((tri * 5) & 15);
      inner_x = width - 3.0f - (float)((tri + iter) & 3);
      set_vertex(&a, edge_x, base_y, oow, s0, t0, 255.0f, 64.0f, 64.0f, alpha);
      set_vertex(&b, inner_x, base_y + span_h, oow, s0 + 31.5f, t0 + 7.5f, 64.0f, 255.0f, 96.0f, alpha);
      set_vertex(&c, inner_x - sliver, base_y + 0.5f * span_h, oow, s0 + 7.5f, t0 + 47.5f, 96.0f, 96.0f, 255.0f, alpha);
    }

    grDrawTriangle(&a, &b, &c);
  }
}

int main(int argc, char** argv) {
  GrHwConfiguration hwconfig;
  GrTexInfo tex_info;
  FxU32 tex_base;
  int iterations = 160;
  int iter;
  float width = 640.0f;
  float height = 480.0f;

  if (argc > 1) {
    iterations = atoi(argv[1]);
    if (iterations <= 0) iterations = 160;
  }

  grGlideInit();
  if (!grSstQueryHardware(&hwconfig)) fail("grSstQueryHardware");
  grSstSelect(0);
  if (!grSstWinOpen(0,
                    GR_RESOLUTION_640x480,
                    GR_REFRESH_60Hz,
                    GR_COLORFORMAT_ABGR,
                    GR_ORIGIN_UPPER_LEFT,
                    2,
                    1)) {
    fail("grSstWinOpen");
  }

  tex_base = grTexMinAddress(GR_TMU0);
  tex_info.smallLod = GR_LOD_64;
  tex_info.largeLod = GR_LOD_64;
  tex_info.aspectRatio = GR_ASPECT_1x1;
  tex_info.format = GR_TEXFMT_RGB_565;
  tex_info.data = texture;

  grClipWindow(0, 0, (FxU32)width, (FxU32)height);
  grCullMode(GR_CULL_DISABLE);
  grColorMask(FXTRUE, FXFALSE);
  grDepthMask(FXTRUE);
  grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
  grDepthBufferFunction(GR_CMP_ALWAYS);
  grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,
                       GR_BLEND_ONE_MINUS_SRC_ALPHA,
                       GR_BLEND_ONE,
                       GR_BLEND_ZERO);
  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_ITERATED,
                 GR_COMBINE_OTHER_TEXTURE,
                 FXFALSE);
  grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
                 GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_LOCAL_ITERATED,
                 GR_COMBINE_OTHER_NONE,
                 FXFALSE);
  grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
  grTexMipMapMode(GR_TMU0, GR_MIPMAP_NEAREST, FXFALSE);
  grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);

  printf("triwedge_sliver: begin iterations=%d\n", iterations);
  for (iter = 0; iter < iterations; ++iter) {
    upload_texture(tex_base, iter * 19 + 5);
    grTexSource(GR_TMU0, tex_base, GR_MIPMAPLEVELMASK_BOTH, &tex_info);

    grRenderBuffer(GR_BUFFER_BACKBUFFER);
    grBufferClear(0x00000000u ^ (FxU32)(iter * 0x00010101u), 0, 0xffff);
    draw_sliver_storm(width, height, iter, 0);

    grRenderBuffer(GR_BUFFER_FRONTBUFFER);
    draw_sliver_storm(width, height, iter, 1);

    if ((iter & 1) == 1) {
      upload_texture(tex_base, iter * 19 + 11);
      grTexSource(GR_TMU0, tex_base, GR_MIPMAPLEVELMASK_BOTH, &tex_info);
      grRenderBuffer(GR_BUFFER_BACKBUFFER);
      draw_sliver_storm(width, height, iter, 2);
    }

    grBufferSwap(1);

    if ((iter & 7) == 7) {
      printf("triwedge_sliver: iter=%d\n", iter + 1);
      fflush(stdout);
    }
  }

  grSstIdle();
  printf("triwedge_sliver: PASS completed=%d\n", iterations);
  grGlideShutdown();
  return 0;
}
