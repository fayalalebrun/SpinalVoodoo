/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
** FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
** 
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
** THE UNITED STATES.  
** 
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
**
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include <fxos.h>

#include <glide.h>
#include "tlib.h"


int hwconfig;
static const char *version;

static const char name[]    = "sbench";
static const char purpose[] = "benchmark grDrawVertexArray for gouraud shaded triangle";
static const char usage[]   = "-n <frames> -r <res> -d <filename> -l <primitive type> -v <vertex array size> -t <triangel size> -p -a";

void Usage(void)
{
    printf ("       -l <type> => primitive type\n");
    printf ("                 => 0 point\n");
    printf ("                 => 1 line strip\n");
    printf ("                 => 2 line\n");
    printf ("                 => 3 polygon\n");
    printf ("                 => 4 strip\n");
    printf ("                 => 5 fan\n");
    printf ("                 => 6 triangle\n");
    printf ("       -v <size> => vertex array size\n");
    printf ("       -t <size> => triangle size\n");
    printf ("       -p        => packed color\n");
    printf ("       -a        => antialiased\n");
}

void
setVertexXyzRgbaSt( FxFloat *v, FxFloat x, FxFloat y, FxFloat z,
                       FxFloat r, FxFloat g, FxFloat b, FxFloat a,
                       FxFloat s, FxFloat t)
{
    *(v+GR_VERTEX_X_OFFSET) = x;
    *(v+GR_VERTEX_Y_OFFSET) = y;
    *(v+GR_VERTEX_OOZ_OFFSET) =  65000.f/z;
    *(v+GR_VERTEX_OOW_OFFSET) = 1.f;
    *(v+GR_VERTEX_R_OFFSET) = r;
    *(v+GR_VERTEX_G_OFFSET) = g;
    *(v+GR_VERTEX_B_OFFSET) = b;
    *(v+GR_VERTEX_A_OFFSET) = a;
    *(v+GR_VERTEX_OOW_TMU0_OFFSET) = 1.f;
    *(v+GR_VERTEX_SOW_TMU0_OFFSET) = s * 1.f;
    *(v+GR_VERTEX_TOW_TMU0_OFFSET) = t * 1.f;
    *(v+GR_VERTEX_OOW_TMU1_OFFSET) = 1.f;
    *(v+GR_VERTEX_SOW_TMU1_OFFSET) = s * 1.f;
    *(v+GR_VERTEX_TOW_TMU1_OFFSET) = t * 1.f;
}

int
main( int argc, char **argv)
{
  char
    match, 
    **remArgs,
    *args =
      "nrdlsfapvt",
    filename[256];
  int
    pass,
    nVerts = 640,
    frames = -1,
    rv;
  float
    t, tps,
    scrWidth   = 640.f,
    scrHeight  = 480.f;
  FxBool
    scrgrab = FXFALSE;

  GrScreenResolution_t
    resolution = GR_RESOLUTION_640x480;

  FxU32
    listType = GR_TRIANGLE_STRIP;
  FxBool
    useList = FXFALSE;
  FxBool
    aaenable = FXFALSE;
  FxBool
    packedrgb = FXFALSE;
  FxI32
    vsize = 640;
  float tSize = 10.f;
  int kNumPasses;
  FxU32 coords = GR_WINDOW_COORDS;
  FxU32 wrange[2];
  
  /* Process Command Line Arguments */
  while((rv = tlGetOpt(argc, argv, args, &match, &remArgs)) != 0) {
    if ( rv == -1 ) {
      printf( "Unrecognized command line argument\n" );
      printf( "%s %s\n", name, usage );
      printf( "Available resolutions:\n%s\n",
             tlGetResolutionList() );
      Usage();
      return -1;
    }
    switch( match ) {
    case 'n':
      frames = atoi( remArgs[0] );
      break;
    case 'r':
      resolution = tlGetResolutionConstant( remArgs[0], 
                                           &scrWidth, 
                                           &scrHeight );
      break;
    case 'd':
      scrgrab = FXTRUE;
      frames = 1;
      strcpy(filename, remArgs[0]);
      break;

    case 'l':
      useList = FXTRUE;
      listType = atoi( remArgs[0] );
      break;

    case 'f':
      listType = GR_TRIANGLE_FAN;
      break;

    case 's':
      listType = GR_TRIANGLE_STRIP;
      break;

    case 'a':
      aaenable = FXTRUE;
      break;

    case 'p':
      packedrgb = FXTRUE;
      break;

    case 'v':
      vsize = atoi( remArgs[0] );
      break;

    case 't':
      tSize = (float)atoi( remArgs[0] );
      break;

    case 'c':
      coords = atoi( remArgs[0] );
      break;

    }
  }
  
  tlSetScreen( scrWidth, scrHeight );
  
  version = grGetString( GR_VERSION );
  
  printf( "%s:\n%s\n", name, purpose );
  printf( "%s\n", version );
  printf( "Resolution: %s\n", tlGetResolutionString( resolution ) );
  if ( frames == -1 ) {
    printf( "Press A Key To Begin Test.\n" );
    tlGetCH();
  }
  
  /* Initialize Glide */
  grGlideInit();
  assert( hwconfig = tlVoodooType() );
  grGet(GR_WDEPTH_MIN_MAX, 8, wrange);  
  grSstSelect( 0 );
  assert( grSstWinOpen(tlGethWnd(),
                       resolution,
                       GR_REFRESH_60Hz,
                       GR_COLORFORMAT_ABGR,
                       GR_ORIGIN_UPPER_LEFT,
                       2, 1 ) );
  
  tlConSet( 0.f, 0.f, 1.f, 1.f, 
           60, 30, 0xffffff );
  
  /* Set up Render State - gouraud shading */
  grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_LOCAL_ITERATED,
                  GR_COMBINE_OTHER_NONE,
                  FXFALSE );

  if (aaenable) {
    grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL,
                    GR_COMBINE_FACTOR_NONE,
                    GR_COMBINE_LOCAL_ITERATED,
                    GR_COMBINE_OTHER_NONE,
                    FXFALSE );
    grAlphaBlendFunction( GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
                          GR_BLEND_ZERO, GR_BLEND_ZERO );
  }

  grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_A,   GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Z,   GR_VERTEX_OOZ_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_W,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q0,  GR_VERTEX_OOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_ST1, GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q1,  GR_VERTEX_OOW_TMU1_OFFSET << 2, GR_PARAM_ENABLE);

  grCullMode(GR_CULL_DISABLE);
  
  grSstOrigin(GR_ORIGIN_LOWER_LEFT);

  tlConOutput( "Press a key to quit\n" );
  while( frames-- && tlOkToRender()) {
    GrVertex vtxA, vtxB, vtxC;
    
    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight );
        
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }
    
    if (useList == FXFALSE) {

      grBufferClear( 0x00, 0, (FxU16)wrange[1] );
      
      vtxA.x = tlScaleX( 0.3f ), vtxA.y = tlScaleY( 0.3f );
      vtxA.r = 255.f, vtxA.g = 0.f, vtxA.b = 0.f;
      
      vtxB.x = tlScaleX( 0.8f ), vtxB.y = tlScaleY( 0.4f );
      vtxB.r = 0.f, vtxB.g = 255.f, vtxB.b = 0.f;
      
      vtxC.x = tlScaleX( 0.5f ), vtxC.y = tlScaleY( 0.8f );
      vtxC.r = 0.f, vtxC.g = 0.f, vtxC.b = 255.f;
      
      grDrawTriangle( &vtxA, &vtxB, &vtxC );
    } else {

#define NVELEM  (sizeof(GrVertex) >> 2)
      
      int v;
      float *vArray;
      float **vPtrArray;


      grRenderBuffer(GR_BUFFER_FRONTBUFFER);

      vArray = (float *) calloc(nVerts * NVELEM, sizeof(float));
      vPtrArray = (float **) calloc(nVerts, sizeof(float *));

      /* Setup the pointer array */
      for (v = 0; v < nVerts; v++)
        vPtrArray[v] = &vArray[NVELEM * v];

      if ((listType == GR_TRIANGLE_STRIP) || 
          (listType == GR_TRIANGLES) || 
          (listType == GR_POINTS) || 
          (listType == GR_LINE_STRIP) || 
          (listType == GR_LINES)){
        /* Set up the vertices */
        for (v = 0; v < vsize; v++) {
          setVertexXyzRgbaSt( &vPtrArray[v][GR_VERTEX_X_OFFSET], 
                              (float) (v >> 1) * tSize, (v & 1) ? tSize : 0.f, 100.f,
                              v * (255.f / vsize), 0.f, 255.f - (v * 255.f / vsize),
                              255.f,
                              0.5f, 0.5f);

          if (packedrgb) {
            *(FxU32*) &vPtrArray[v][GR_VERTEX_R_OFFSET] =
              ((unsigned char) (vPtrArray[v][GR_VERTEX_R_OFFSET]) << 16) |
              ((unsigned char) (vPtrArray[v][GR_VERTEX_G_OFFSET]) << 8) |
              ((unsigned char) (vPtrArray[v][GR_VERTEX_B_OFFSET]));
            *(FxU32*) &vPtrArray[v][GR_VERTEX_R_OFFSET] |= 0xff000000;
          }
        }
      } else { /* listType == GR_TRIANGLE_FAN */
#define FX_PI 3.1415f

#if 0

        for (v = 0; v < nVerts; v++) {

          float x, y;

          if (v == 0) {
            x = 0.5f;
            y = 0.0f;
            vPtrArray[v][VTX_R] = 0.f;
            vPtrArray[v][VTX_G] = 255.f;
            vPtrArray[v][VTX_R] = 0.f;
          } else {
            float
              theta;

            theta = FX_PI - ((((float) v / (float) nVerts)) * FX_PI);

            x = ((float) cos(theta) + 1.f) / 2.f;
            y = (float) sin(theta);

            vPtrArray[v][VTX_R] = v * (255.f / nVerts);
            vPtrArray[v][VTX_G] = 0.f;
            vPtrArray[v][VTX_B] = 255.f - (v * 255.f / nVerts);
          }
          vPtrArray[v][VTX_X] = tlScaleX(x);
          vPtrArray[v][VTX_Y] = tlScaleY(y);
        }
#endif

#if 1
        for (v = 0; v < vsize; v++) {

          float x, y;
          float area = tSize * tSize / 2;
          float r = (float)sqrt((vsize - 2) * area / FX_PI);

          if (v == 0) {
            x = 0.5f;
            y = 0.5f;

            vPtrArray[v][GR_VERTEX_X_OFFSET] = tlScaleX(x);
            vPtrArray[v][GR_VERTEX_Y_OFFSET] = tlScaleY(y);
            vPtrArray[v][GR_VERTEX_R_OFFSET] = 0.f;
            vPtrArray[v][GR_VERTEX_G_OFFSET] = 255.f;
            vPtrArray[v][GR_VERTEX_B_OFFSET] = 0.f;
            vPtrArray[v][GR_VERTEX_A_OFFSET] = 255.f;
          } else {
            float
              theta;

            theta = FX_PI - ((((float) (v-1) / (float) (vsize - 2))) * 2.f * FX_PI);

            x = (float) cos(theta) * r;
            y = (float) sin(theta) * r;
 
            vPtrArray[v][GR_VERTEX_X_OFFSET] = tlScaleX(0.5f) + x;
            vPtrArray[v][GR_VERTEX_Y_OFFSET] = tlScaleY(0.5f) + y;
            
            vPtrArray[v][GR_VERTEX_R_OFFSET] = v * (255.f / vsize);
            vPtrArray[v][GR_VERTEX_G_OFFSET] = 0.f;
            vPtrArray[v][GR_VERTEX_B_OFFSET] = 255.f - (v * 255.f / vsize);
            vPtrArray[v][GR_VERTEX_A_OFFSET] = 255.f;
          }
          if (packedrgb) {
            *(FxU32*) &vPtrArray[v][GR_VERTEX_R_OFFSET] =
              ((unsigned char) (vPtrArray[v][GR_VERTEX_R_OFFSET]) << 16) |
              ((unsigned char) (vPtrArray[v][GR_VERTEX_G_OFFSET]) << 8) |
              ((unsigned char) (vPtrArray[v][GR_VERTEX_B_OFFSET]));
            *(FxU32*) &vPtrArray[v][GR_VERTEX_R_OFFSET] |= 0xff000000;
          }
        }
#endif
      }

      /* Describe the vertices to Glide */

      if (packedrgb)
        grVertexLayout(GR_PARAM_PARGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
      if (aaenable)
        grEnable(GR_AA_ORDERED);
      
      if (coords == GR_WINDOW_COORDS) {
        grCoordinateSpace(GR_WINDOW_COORDS);
      }
      else {
        grCoordinateSpace(GR_CLIP_COORDS);
        grViewport(0, 0, 640, 480);
        grDepthRange(0.f, 1.f);
      }

/* #define kNumPasses 10000000 */
      kNumPasses = 10000000 / vsize;

      grFinish();                                /* wait for idle        */
      timer(0);
      for (pass = 0; pass < kNumPasses; pass++)
        grDrawVertexArray(listType, vsize, vPtrArray);
      grFinish();                                /* wait for idle        */
      t = timer(1);

      switch (listType) {
      case GR_POINTS:
        tps = (vsize * kNumPasses) / t;
        printf("%.2f pps\n", tps);
        break;

      case GR_LINE_STRIP:
        tps = ((vsize - 1) * kNumPasses) / t;
        printf("%.2f lps\n", tps);
        break;

      case GR_LINES:
        tps = ((vsize/2) * kNumPasses) / t;
        printf("%.2f lps\n", tps);
        break;

      case GR_POLYGON:
      case GR_TRIANGLE_STRIP:
      case GR_TRIANGLE_FAN:
        tps = ((vsize - 2) * kNumPasses) / t;
        printf("%.2f tps\n", tps);
        break;

      case GR_TRIANGLES:
        tps = ((vsize / 3) * kNumPasses) / t;
        printf("%.2f tps\n", tps);
        break;
      }

      
    }
    frames = 0;
  }
  
  grGlideShutdown();
  return 0;
} /* main */
