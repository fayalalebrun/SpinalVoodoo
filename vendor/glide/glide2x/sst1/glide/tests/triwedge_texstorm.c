#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glide.h>

#define TEX_DIM 64

static FxU16 texture_a[TEX_DIM * TEX_DIM];
static FxU16 texture_b[TEX_DIM * TEX_DIM];

static void fail(const char* stage) {
  printf("triwedge_texstorm: FAIL %s\n", stage);
  grGlideShutdown();
  exit(1);
}

static FxU16 pack565(int r, int g, int b) {
  return (FxU16)(((r & 0x1f) << 11) | ((g & 0x3f) << 5) | (b & 0x1f));
}

static void build_texture(FxU16* dst, int seed) {
  int x;
  int y;
  for (y = 0; y < TEX_DIM; ++y) {
    for (x = 0; x < TEX_DIM; ++x) {
      int r = (seed + x + (y * 3)) & 31;
      int g = ((seed * 7) + (x * 5) + (y * 11)) & 63;
      int b = ((seed * 13) + (x * 9) + (y * 17)) & 31;
      if (((x ^ y ^ seed) & 7) == 0) {
        r = 31;
        g = (seed + x) & 63;
        b = 31 - ((seed + y) & 31);
      }
      dst[y * TEX_DIM + x] = pack565(r, g, b);
    }
  }
}

static void upload_texture(FxU32 base_addr, FxU16* data, int seed) {
  build_texture(data, seed);
  grTexDownloadMipMapLevel(GR_TMU0,
                           base_addr,
                           GR_LOD_64,
                           GR_LOD_64,
                           GR_ASPECT_1x1,
                           GR_TEXFMT_RGB_565,
                           GR_MIPMAPLEVELMASK_BOTH,
                           data);
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

static void draw_triangle_burst(float width, float height, int iter, int burst) {
  int tri;
  for (tri = 0; tri < 96; ++tri) {
    GrVertex a;
    GrVertex b;
    GrVertex c;
    float cx = 24.0f + (float)((tri * 37 + iter * 11 + burst * 19) % ((int)width - 48));
    float cy = 24.0f + (float)((tri * 23 + iter * 7 + burst * 29) % ((int)height - 48));
    float span = 18.0f + (float)((tri * 13 + iter) & 63);
    float tall = 24.0f + (float)((tri * 9 + burst * 5) & 95);
    float oow = 0.5f + 0.0625f * (float)((tri + iter + burst) & 7);
    float alpha = 32.0f + (float)(((tri * 17) + (iter * 23)) & 191);
    float s0 = (float)((tri * 5 + iter * 3) & 63);
    float t0 = (float)((tri * 7 + burst * 9) & 63);

    set_vertex(&a,
               cx - span,
               cy - tall,
               oow,
               s0,
               t0,
               255.0f,
               96.0f + (float)((tri * 5) & 127),
               64.0f + (float)((iter * 3) & 127),
               alpha);
    set_vertex(&b,
               cx + span,
               cy - 2.0f + (float)(tri & 3),
               oow,
               s0 + 31.5f,
               t0 + 7.5f,
               64.0f + (float)((burst * 9) & 127),
               255.0f,
               96.0f + (float)((tri * 11) & 127),
               alpha);
    set_vertex(&c,
               cx + 2.0f - (float)(tri & 3),
               cy + tall,
               oow,
               s0 + 7.5f,
               t0 + 47.5f,
               96.0f + (float)((iter * 13) & 127),
               64.0f + (float)((burst * 7) & 127),
               255.0f,
               alpha);
    grDrawTriangle(&a, &b, &c);
  }
}

int main(int argc, char** argv) {
  GrHwConfiguration hwconfig;
  GrTexInfo tex_info;
  FxU32 tex_base;
  FxU32 tex_bytes;
  int iterations = 200;
  int iter;
  float width = 640.0f;
  float height = 480.0f;

  if (argc > 1) {
    iterations = atoi(argv[1]);
    if (iterations <= 0) iterations = 200;
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

  tex_bytes = grTexCalcMemRequired(GR_LOD_64,
                                   GR_LOD_64,
                                   GR_ASPECT_1x1,
                                   GR_TEXFMT_RGB_565);
  tex_base = grTexMinAddress(GR_TMU0);

  tex_info.smallLod = GR_LOD_64;
  tex_info.largeLod = GR_LOD_64;
  tex_info.aspectRatio = GR_ASPECT_1x1;
  tex_info.format = GR_TEXFMT_RGB_565;
  tex_info.data = texture_a;

  grClipWindow(0, 0, (FxU32)width, (FxU32)height);
  grCullMode(GR_CULL_DISABLE);
  grDepthMask(FXTRUE);
  grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
  grDepthBufferFunction(GR_CMP_ALWAYS);
  grColorMask(FXTRUE, FXFALSE);
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

  printf("triwedge_texstorm: begin iterations=%d\n", iterations);
  for (iter = 0; iter < iterations; ++iter) {
    upload_texture(tex_base, texture_a, iter * 31 + 1);
    upload_texture(tex_base + tex_bytes, texture_b, iter * 31 + 17);

    grBufferClear(0x00101010u ^ (FxU32)(iter * 0x00010101u), 0, 0xffff);

    grRenderBuffer((iter & 1) ? GR_BUFFER_FRONTBUFFER : GR_BUFFER_BACKBUFFER);
    grTexSource(GR_TMU0,
                (iter & 1) ? (tex_base + tex_bytes) : tex_base,
                GR_MIPMAPLEVELMASK_BOTH,
                &tex_info);
    draw_triangle_burst(width, height, iter, 0);

    grRenderBuffer((iter & 1) ? GR_BUFFER_BACKBUFFER : GR_BUFFER_FRONTBUFFER);
    grTexSource(GR_TMU0,
                (iter & 1) ? tex_base : (tex_base + tex_bytes),
                GR_MIPMAPLEVELMASK_BOTH,
                &tex_info);
    draw_triangle_burst(width, height, iter, 1);

    grBufferSwap(1);

    grRenderBuffer(GR_BUFFER_BACKBUFFER);
    grTexSource(GR_TMU0,
                (iter & 1) ? (tex_base + tex_bytes) : tex_base,
                GR_MIPMAPLEVELMASK_BOTH,
                &tex_info);
    draw_triangle_burst(width, height, iter, 2);

    if ((iter & 7) == 7) {
      printf("triwedge_texstorm: iter=%d\n", iter + 1);
      fflush(stdout);
    }
  }

  grSstIdle();
  printf("triwedge_texstorm: PASS completed=%d\n", iterations);
  grGlideShutdown();
  return 0;
}
