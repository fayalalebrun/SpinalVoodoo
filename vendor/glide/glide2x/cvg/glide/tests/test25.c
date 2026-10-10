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
#include <string.h>
#include <assert.h>
#include <math.h>

#include <glide.h>
#include "tlib.h"


#define SNAP_COORD(v)   ( ( v ) = (( float )( ( long )( ( v ) * 16 ) )) / 16.0f )

#define WREFWIDTH 640.f
#define WREFHEIGHT 480.f

#define WINSCALEX(x) ((x/WREFWIDTH) * wWidth)
#define WINSCALEY(y) ((y/WREFHEIGHT) * wHeight)  

#ifndef M_PI
#define M_PI    3.14159265358979323846f
#endif
#define DEG2RAD( x ) ( (x) * M_PI / 180.0f )


GrHwConfiguration hwconfig;
static char version[80];

static const char name[]    = "test25";
static const char purpose[] = "anti-aliased triangle test";
static const char usage[]   = "-n <frames> -r <res> -b <bgfile.3df> -t<trifile.3df> -aalphavalue";


static void doHelp( void ) {
  grBufferClear( 0, 0, 0 );
  tlConOutput("Keymap:\n");
  tlConOutput("        A or a:         toggle Antialiasing\n");
  tlConOutput("        C or c:         toggle Compositing\n");
  tlConOutput("        B or b:         toggle Bilinear\n");
  tlConOutput("        F or f:         toggle Front buffer \n");
  tlConOutput("        H, h, or ?:     Help\n");
  tlConOutput("        I or i:         toggle background Image\n");
  tlConOutput("        L or l:         Draw lines\n");
  tlConOutput("        N or n:         print performance Numbers\n");
  tlConOutput("        P or p:         Pause rendering\n");
  tlConOutput("        R or r:         Print screen Resolution\n");
  tlConOutput("        Q or q or esc:  Quit\n");
  tlConOutput("        S or s:         toggle hidden Surface removal (backface culling)\n");
  tlConOutput("        T or t:         toggle Texturing\n");
  tlConOutput("        U or u:         toggle shameless Plug\n");
  tlConOutput("        V or v:         toggle VGA/Voodoo\n");
  tlConOutput("Press a key to continue...\n");
  tlConRender();
  grBufferSwap( 1 );
  tlGetCH();
  tlConClear();
}

typedef float Matrix[4][4];
typedef float Point[3];

void MatMakeXRot( Matrix m, float radians )
{
  float c = (float)cos( radians );
  float s = (float)sin( radians );

  m[0][0] = 1.0f;  m[0][1] = 0.0f;    m[0][2] = 0.0f;    m[0][3] = 0.0f;
  m[1][0] = 0.0f;  m[1][1] = c;       m[1][2] = s;       m[1][3] = 0.0f;
  m[2][0] = 0.0f;  m[2][1] = -s;      m[2][2] = c;       m[2][3] = 0.0f;
  m[3][0] = 0.0f;  m[3][1] = 0.0f;    m[3][2] = 0.0f;    m[3][3] = 1.0f;
}

void MatMakeYRot( Matrix m, float radians )
{
  float c = (float)cos( radians );
  float s = (float)sin( radians );

  m[0][0] = c;     m[0][1] = 0.0f;    m[0][2] = -s;      m[0][3] = 0.0f;
  m[1][0] = 0.0f;  m[1][1] = 1.0f;    m[1][2] = 0.0f;    m[1][3] = 0.0f;
  m[2][0] = s;     m[2][1] = 0.0f;    m[2][2] = c;       m[2][3] = 0.0f;
  m[3][0] = 0.0f;  m[3][1] = 0.0f;    m[3][2] = 0.0f;    m[3][3] = 1.0f;
}

void MatMakeZRot( Matrix m, float radians )
{
  float c = (float)cos( radians );
  float s = (float)sin( radians );

  m[0][0] = c;     m[0][1] = s;       m[0][2] = 0.0f;    m[0][3] = 0.0f;
  m[1][0] = -s;    m[1][1] = c;       m[1][2] = 0.0f;    m[1][3] = 0.0f;
  m[2][0] = 0.0f;  m[2][1] = 0.0f;    m[2][2] = 1.0f;    m[2][3] = 0.0f;
  m[3][0] = 0.0f;  m[3][1] = 0.0f;    m[3][2] = 0.0f;    m[3][3] = 1.0f;
}

void MatMakeIdent( Matrix m )
{
  int i, j;

  for( i = 0; i < 4; i++ )
    {
      for( j = 0; j < 4; j++ )
        {
          if( i == j )
            m[i][j] = 1.0f;
          else
            m[i][j] = 0.0f;
        }
    }
}

void PointMatMult( GrVertex *result, GrVertex *v, Matrix m )
{
  float w;
  GrVertex ptmp;

  ptmp = *v;
  ptmp.x = ( v->x * m[0][0] ) + ( v->y * m[1][0] ) + 
    ( v->z * m[2][0] ) + m[3][0];
  ptmp.y = ( v->x * m[0][1] ) + ( v->y * m[1][1] ) + 
    ( v->z * m[2][1] ) + m[3][1];
  ptmp.z = ( v->x * m[0][2] ) + ( v->y * m[1][2] ) + 
    ( v->z * m[2][2] ) + m[3][2];
  w = ( v->x * m[0][3] ) + ( v->y * m[1][3] ) + 
    ( v->z * m[2][3] ) + m[3][3];
  if( w != 0.0f ) { ptmp.x /= w;  ptmp.y /= w;  ptmp.z /= w; }
  *result = ptmp;
}

static int screenFulls[] = {
  320*200,   
  320*240,   
  400*256,   
  512*384,   
  640*200,   
  640*350,   
  640*400,   
  640*480,   
  800*600,   
  960*720,   
  856*480
};

#define NTRIS 1
#define NFRAMES 20
#define NVERTS 3

int main( int argc, char **argv) {
  char match; 
  char **remArgs;
  int  rv;

  GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
  float                scrWidth   = 640.0f;
  float                scrHeight  = 480.0f;
  int frames                      = -1;

  float
    minColor = 10.f,            /* Vertex min color */
    maxColor = 245.f;           /* Vertex max color */
    
  GrVertex
    localVerts[3],              /* Vertices in world coordinates */
    texVerts[4];                /* Texture vertices for background */
    
  float
    alpha = 192.0f,             /* Alpha for blending tringle over background */
    y_angle = 0.0f;             /* rotation amount */
    
  int
    firstTime = 1;              /* Used for performance calculations */

  GrPassthruMode_t
    passthruMode = GR_PASSTHRU_SHOW_SST1; /* For toggling passthru */

  FxBool
    plugging = FXFALSE,         /* Show shameless plug */
    printPerf = FXFALSE,        /* Print performance numbers */
    lines = FXFALSE,            /* Draw lines instead of triangles */
    blend = FXFALSE,            /* Blend the triangle over the background */
    texturing = FXFALSE,        /* Texture the tiangle */
    antialias = FXTRUE,         /* Antialias the triangle? */
    bilinear = FXTRUE,          /* Perform bilinear filtering on the texture? */
    render = FXTRUE,            /* Draw? */
    backbuffer = FXTRUE,        /* Draw to backbuffer? */
    background = FXTRUE;        /* Draw background? */

  GrOriginLocation_t
    origin = GR_ORIGIN_LOWER_LEFT; /* Origin */

  FxU32
    swapDelay = 1,              /* Arg to grBufferSwap */
    trisDrawn,                  /* # triangles drawn */
    trisProcessed,              /* # triangles through pipeline */
    lastFrame = 0,              /* Number of last frame we did perf stats */
    frameNum = 0L;              /* id of each frame drawn */
    
  GrCullMode_t
    cullMode = GR_CULL_DISABLE; /* backface culling */
    

  GrMipMapId_t
    triDecal,                   /* Triangle decal texture */
    bgDecal;                    /* Background decal texture */
    
  Gu3dfInfo
    bgInfo,                     /* Info on background texture */
    triInfo;                    /* Info on triangle texture */
    
  GrColorCombineFnc_t
    ccFnc = GR_COLORCOMBINE_ITRGB; /* Start of w/ Gouraud shading */
    
  char
    *bgFileName = NULL,         /* Name of background texture file */
    *triFileName = NULL;        /* Name of triangle texture file */
    
  int
    frameCount = 0;


  /* Process Command Line Arguments */
  while ((rv = tlGetOpt(argc, argv, "nrbtea", &match, &remArgs)) != 0) {
    if ( rv == -1 ) {
      printf( "Unrecognized command line argument\n" );
      printf( "%s %s\n", name, usage );
      printf( "Available resolutions:\n%s\n",
             tlGetResolutionList() );
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
    case 'b':
      bgFileName = strdup( remArgs[0] );
      break;
    case 't':
      triFileName = strdup( remArgs[0] );
      break;
    case 'a':
      alpha = (float)atof( remArgs[0] );
      break;
    }
  }

  tlSetScreen( scrWidth, scrHeight );

  grGlideGetVersion( version );
    
  printf( "%s:\n%s\n", name, purpose );
  printf( "%s\n", version );
  printf( "Resolution: %s\n", tlGetResolutionString( resolution ) );
  if ( frames == -1 ) {
    printf( "Press A Key To Begin Test.\n" );
    tlGetCH();
  }
    
  /* Initialize Glide */
  grGlideInit();
  assert( grSstQueryHardware( &hwconfig ) );
  grSstSelect( 0 );
  assert( grSstWinOpen( 0,
                       resolution,
                       GR_REFRESH_60Hz,
                       GR_COLORFORMAT_ABGR,
                       origin,
                       2, 1 ) );
    
  tlConSet( 0.0f, 0.0f, 1.0f, 1.0f,
           80, 40, 0xffffff );

  if(frames == -1) {
    doHelp();
  }

  localVerts[0].x = 0.f;
  localVerts[0].y = 0.75f;
  localVerts[0].z = 0.0f;
  localVerts[0].tmuvtx[0].sow = 255.f;
  localVerts[0].tmuvtx[0].tow = 255.f;
  localVerts[0].oow = 1.f;  
  localVerts[0].r = maxColor;
  localVerts[0].g = minColor;
  localVerts[0].b = minColor;
  localVerts[0].a = 255.f;

  localVerts[1].x = -0.75f;
  localVerts[1].y = -0.75f;
  localVerts[1].z = 0.0f;
  localVerts[1].tmuvtx[0].sow = 0.f;
  localVerts[1].tmuvtx[0].tow = 255.f;
  localVerts[1].oow = 1.f;  
  localVerts[1].r = minColor;
  localVerts[1].g = maxColor;
  localVerts[1].b = minColor;
  localVerts[1].a = 255.f;

  localVerts[2].x = 0.75f;
  localVerts[2].y = -0.75f;
  localVerts[2].z = 0.0f;
  localVerts[2].tmuvtx[0].sow = 255.f;
  localVerts[2].tmuvtx[0].tow = 0.f;
  localVerts[2].oow = 1.f;  
  localVerts[2].r = minColor;
  localVerts[2].g = minColor;
  localVerts[2].b = maxColor;
  localVerts[2].a = 255.f;

  texVerts[0].x = 0.f;
  texVerts[0].y = 0.f;
  texVerts[0].a = 255.f;
  texVerts[0].oow = 1.f;
  texVerts[0].tmuvtx[0].sow = 0.f * texVerts[0].oow;
  texVerts[0].tmuvtx[0].tow = 255.f * texVerts[0].oow;

  texVerts[1].x = scrWidth;
  texVerts[1].y = 0.f;
  texVerts[1].a = 255.f;
  texVerts[1].oow = 1.f;
  texVerts[1].tmuvtx[0].sow = 255.f * texVerts[1].oow;
  texVerts[1].tmuvtx[0].tow = 255.f * texVerts[1].oow;

  texVerts[2].x = scrWidth;
  texVerts[2].y = scrHeight;
  texVerts[2].a = 255.f;
  texVerts[2].oow = 1.f;
  texVerts[2].tmuvtx[0].sow = 255.f * texVerts[2].oow;
  texVerts[2].tmuvtx[0].tow = 0.f * texVerts[2].oow;
  
  texVerts[3].x = 0.f;
  texVerts[3].y = scrHeight;
  texVerts[3].a = 255.f;
  texVerts[3].oow = 1.f;
  texVerts[3].tmuvtx[0].sow = 0.f * texVerts[3].oow;
  texVerts[3].tmuvtx[0].tow = 0.f * texVerts[3].oow;
  
  if (bgFileName == NULL)
    bgFileName = "miro.3df";
  if (triFileName == NULL)
    triFileName = "matt1.3df";
  
  /* Read in background texture file  */
  if ( gu3dfGetInfo( bgFileName, &bgInfo ) )  {
    bgInfo.data = malloc( bgInfo.mem_required );

    if ( bgInfo.data == 0 ) {
      fprintf( stderr, "out of memory for texture file %s\n", bgFileName );
      grGlideShutdown();
      exit( -1 );
    }

    if ( !gu3dfLoad( bgFileName, &bgInfo ) ) {
      fprintf( stderr, "could not load texture file %s\n", bgFileName );
      grGlideShutdown();
      exit( -1 );
    }

    bgDecal = guTexAllocateMemory( 0, GR_MIPMAPLEVELMASK_BOTH,
                                  bgInfo.header.width, bgInfo.header.height,
                                  bgInfo.header.format,
                                  GR_MIPMAP_NEAREST,
                                  bgInfo.header.small_lod, bgInfo.header.large_lod,
                                  bgInfo.header.aspect_ratio,
                                  GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP,
                                  GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR,
                                  0.0F,
                                  FXFALSE );
    if ( bgDecal == GR_NULL_MIPMAP_HANDLE ) {
      fprintf( stderr, "could not allocate memory for texture file %s\n", bgFileName );
      grGlideShutdown();
      exit( -1 );
    }
    guTexDownloadMipMap( bgDecal, bgInfo.data, &bgInfo.table.nccTable );
    free( bgInfo.data );
  } else {
    fprintf( stderr, "could not get info on %s\n", bgFileName );
    grGlideShutdown();
    exit( -1 );
  }
  
  if ( gu3dfGetInfo( triFileName, &triInfo ) )  {
    triInfo.data = malloc( triInfo.mem_required );
    
    if ( triInfo.data == 0 ) {
      fprintf( stderr, "out of memory for texture file  %s\n", triFileName );
      grGlideShutdown();
      exit( -1 );
    }
    
    if ( !gu3dfLoad( triFileName, &triInfo ) ) {
      fprintf( stderr, "could not load texture file %s\n", triFileName );
      grGlideShutdown();
      exit( -1 );
    }

    triDecal = guTexAllocateMemory( 0, GR_MIPMAPLEVELMASK_BOTH,
                                   triInfo.header.width,  
                                   triInfo.header.height, 
                                   triInfo.header.format,
                                   GR_MIPMAP_NEAREST,
                                   triInfo.header.small_lod,
                                   triInfo.header.large_lod, 
                                   triInfo.header.aspect_ratio,
                                   GR_TEXTURECLAMP_WRAP,
                                   GR_TEXTURECLAMP_WRAP, 
                                   GR_TEXTUREFILTER_BILINEAR,
                                   GR_TEXTUREFILTER_BILINEAR, 
                                   0.0F,
                                   FXFALSE );
    if ( triDecal == GR_NULL_MIPMAP_HANDLE ) {
      fprintf( stderr, "could not allocate memory for %s\n", triFileName );
      grGlideShutdown();
      exit( -1 );
    }
    guTexDownloadMipMap( triDecal, triInfo.data, &triInfo.table.nccTable );
    free( triInfo.data );
  } else {
    fprintf( stderr, "could not get info on %s\n", triFileName );
    grGlideShutdown();
    exit( -1 );
  }

  grTexCombineFunction(GR_TMU0, GR_TEXTURECOMBINE_DECAL);
  grRenderBuffer(backbuffer == FXTRUE ? GR_BUFFER_BACKBUFFER : GR_BUFFER_FRONTBUFFER);

  /* Set up alpha blending for AA and compositing... */
  guAlphaSource( GR_ALPHASOURCE_ITERATED_ALPHA );
  grAlphaBlendFunction( GR_BLEND_SRC_ALPHA,
                       GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ONE,
                       GR_BLEND_ZERO );
  grAlphaTestFunction( GR_CMP_ALWAYS );


  while ( 1 ) {
    Matrix rotm;
    GrVertex xformedVerts[3];
    int i;

    
    MatMakeYRot( rotm, DEG2RAD( y_angle ) );
    
    if (resolution == GR_RESOLUTION_NONE) 
      tlGetResolutionConstant("0",
                              &scrWidth, 
                              &scrHeight );


    for( i = 0; i < 3; i++ ) {
      PointMatMult( &xformedVerts[i], &localVerts[i], rotm );
      xformedVerts[i].x = xformedVerts[i].x / ( xformedVerts[i].z + 2.0f );
      xformedVerts[i].y = xformedVerts[i].y / ( xformedVerts[i].z + 2.0f );
      xformedVerts[i].x *= scrWidth / 2.0f;
      xformedVerts[i].y *= scrHeight / 2.0f;
      xformedVerts[i].x += scrWidth / 2.0f;
      xformedVerts[i].y += scrHeight / 2.0f;
      xformedVerts[i].oow = 1.f / ((xformedVerts[i].z + 2) * scrHeight);
      xformedVerts[i].tmuvtx[0].sow *= xformedVerts[i].oow;
      xformedVerts[i].tmuvtx[0].tow *= xformedVerts[i].oow;
      SNAP_COORD( xformedVerts[i].x );
      SNAP_COORD( xformedVerts[i].y );
    }
    
    guColorCombineFunction( ccFnc );
    
    /*    grLfbBypassMode(GR_LFBBYPASS_ENABLE); */

    if (render == FXTRUE) {
      ++frameNum;
      if ((frameNum % NFRAMES) == 0) {
        if (printPerf) {
          if (!firstTime)  {
            GrSstPerfStats_t
              pStats;
            FxU32
              lfbWritePixels,
              nFrames = frameNum - lastFrame,
              fillPixels = nFrames * screenFulls[resolution],
              totFail;
            
            lastFrame = frameNum;

            grSstPerfStats(&pStats);
            grTriStats(&trisProcessed, &trisDrawn);
            
            totFail = pStats.chromaFail + pStats.zFuncFail +
              pStats.aFuncFail;  
            
            lfbWritePixels = pStats.pixelsOut - pStats.pixelsIn - fillPixels;

            tlConOutput("In the last %d frames:\n", nFrames);
            tlConOutput("      Pixels Processed:               %d\n",
                        pStats.pixelsIn);
            tlConOutput("      Chroma Failures:                %d\n",
                        pStats.chromaFail);
            tlConOutput("      Z Compare Failures:             %d\n",
                        pStats.zFuncFail);
            tlConOutput("      Alpha Compare Failures:         %d\n",
                        pStats.aFuncFail);
            tlConOutput("      Fast Fill Pixels:               %d\n",
                        fillPixels);
            tlConOutput("      LFB Write Pixels:               %d\n",
                        lfbWritePixels);
            tlConOutput("      Total Pixels Drawn:             %d\n",
                        pStats.pixelsOut);
            tlConOutput("      Triangles Processed             %d\n",
                        trisProcessed);
            tlConOutput("      Triangles Drawn                 %d\n",
                        trisDrawn);

            if (
                (pStats.pixelsOut - lfbWritePixels - fillPixels - pStats.pixelsIn) != totFail)
              tlConOutput("Error:  %d != %d\n",
                          pStats.pixelsOut - lfbWritePixels - fillPixels,
                          totFail);

            grSstResetPerfStats();
          } else {
            lastFrame = frameNum;
            grSstResetPerfStats();
            firstTime = 0;
          }
        }
      }
      grBufferClear( 0xffffffff, 0, GR_WDEPTHVALUE_FARTHEST );
    
      if (background == FXTRUE) {
        GrState
          oldState;

        texVerts[0].x = 0.f;
        texVerts[0].y = 0.f;

        texVerts[1].x = scrWidth;
        texVerts[1].y = 0.f;

        texVerts[1].x = scrWidth;
        texVerts[1].y = 0.f;

        texVerts[2].x = scrWidth;
        texVerts[2].y = scrHeight;

        texVerts[3].x = 0.f;
        texVerts[3].y = scrHeight;

        grGlideGetState(&oldState);

        grAlphaBlendFunction(
                             GR_BLEND_ONE, GR_BLEND_ZERO,
                             GR_BLEND_ONE, GR_BLEND_ZERO);

        grColorCombine(
                       GR_COMBINE_FUNCTION_SCALE_OTHER,
                       GR_COMBINE_FACTOR_ONE,
                       GR_COMBINE_LOCAL_NONE,
                       GR_COMBINE_OTHER_TEXTURE, FXFALSE
                       );

        guTexSource(bgDecal);
        
        for (i = 0; i < NTRIS; i++) {
          grDrawTriangle(&texVerts[0], &texVerts[1], &texVerts[2]);
          grDrawTriangle(&texVerts[2], &texVerts[3], &texVerts[0]);
        }
        grGlideSetState(&oldState);
        grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);

      }

      if (texturing == FXTRUE)
        guTexSource(bgDecal);

      if (texturing)
        guTexSource(triDecal);

      if (antialias == FXTRUE) {
        if (lines == FXTRUE) {
          grAADrawLine(&xformedVerts[0], &xformedVerts[1]);
          grAADrawLine(&xformedVerts[1], &xformedVerts[2]);
          grAADrawLine(&xformedVerts[2], &xformedVerts[0]);
        } else {
          grAADrawTriangle(
                           &xformedVerts[0],  &xformedVerts[1],
                           &xformedVerts[2], FXTRUE, FXTRUE, FXTRUE
                           );
        }
      } else {
        if (lines == FXTRUE) {
          grDrawLine(&xformedVerts[0], &xformedVerts[1]);
          grDrawLine(&xformedVerts[1], &xformedVerts[2]);
          grDrawLine(&xformedVerts[2], &xformedVerts[0]);
        } else {          
          grDrawTriangle(
                         &xformedVerts[0], &xformedVerts[1],
                         &xformedVerts[2]
                         );
        }
      }

      if (plugging) 
        grSplash(0.f, 0.f, scrWidth / 5.f, scrHeight / 5.f, frameNum);
      
      if (backbuffer) {
        tlConRender();
        grBufferSwap( swapDelay );
      }
    }
    
    if (tlKbHit()) {
      char c = (char) tlGetCH();
      
      switch (c) {
      case 'a':
      case 'A':
        if (antialias == FXFALSE) {
          tlConOutput("Turning ON Antialiasing\n");
          antialias = FXTRUE;
        } else {
          tlConOutput("Turning OFF Antialiasing\n");
          antialias = FXFALSE;
        }
        break;
      case 'B':
      case 'b':
        if (bilinear == FXFALSE) {
          bilinear = FXTRUE;
          tlConOutput("Turning ON BiLinear blending\n");
          guTexChangeAttributes(
                                triDecal, bgInfo.header.width,
                                bgInfo.header.height, 
                                bgInfo.header.format, GR_MIPMAP_NEAREST,
                                bgInfo.header.small_lod,
                                bgInfo.header.large_lod, 
                                bgInfo.header.aspect_ratio,
                                GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP,
                                GR_TEXTUREFILTER_BILINEAR,
                                GR_TEXTUREFILTER_BILINEAR);
        } else {
          bilinear = FXFALSE;
          tlConOutput("Turning OFF BiLinear blending\n");
          guTexChangeAttributes(
                                triDecal, bgInfo.header.width,
                                bgInfo.header.height, 
                                bgInfo.header.format, GR_MIPMAP_NEAREST,
                                bgInfo.header.small_lod,
                                bgInfo.header.large_lod,
                                bgInfo.header.aspect_ratio, 
                                GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP,
                                GR_TEXTUREFILTER_POINT_SAMPLED,
                                GR_TEXTUREFILTER_POINT_SAMPLED);
        }
        break;

      case 'c':
      case 'C':
        if (blend == FXTRUE) {
          blend = FXFALSE;
          localVerts[0].a = 255.0f;
          localVerts[1].a = 255.0f;
          localVerts[2].a = 255.0f;
        } else {
          blend = FXTRUE;
          localVerts[0].a = alpha;
          localVerts[1].a = alpha;
          localVerts[2].a = alpha;
        }
        break;

      case 'd':
      case 'D':
        tlConOutput("vtxa = (%.2f, %.2f), vtxb = (%.2f, %.2f), vtxc = (%.2f, %.2f)\n",
                    xformedVerts[0].x, xformedVerts[0].y,
                    xformedVerts[1].x, xformedVerts[1].y,
                    xformedVerts[2].x, xformedVerts[2].y
                    );
        break;

      case 'f':
      case 'F':
        if (backbuffer == FXTRUE) {
          backbuffer = FXFALSE;
          grRenderBuffer(GR_BUFFER_FRONTBUFFER);
        } else {
          backbuffer = FXTRUE;
          grRenderBuffer(GR_BUFFER_BACKBUFFER);
        }
        break;

      case 'g':
      case 'G':
#if 0
        grLfbBegin();

        grLfbWriteMode(GR_LFBWRITEMODE_565);
        grLfbOrigin(GR_ORIGIN_UPPER_LEFT);
        grLfbGetReadPtr(GR_BUFFER_FRONTBUFFER);
        tlConOutput("Press a key to get front buffer\n");
        while (!tlKbHit());
        c = (char) tlGetCH();
        guFbReadRegion(0,0,(int)wWidth,(int)scrHeight,scrnImage,(int)wWidth * sizeof(FxU16));
        tlConOutput("Press a key to put image in back buffer and swap\n");
        while (!tlKbHit());
        tlGetCH();

        grLfbGetWritePtr(GR_BUFFER_BACKBUFFER);
        guFbWriteRegion(0,0,(int)wWidth,(int)scrHeight,scrnImage,(int)wWidth * sizeof(FxU16));
        grBufferSwap(swapDelay);

        tlConOutput("Press a key to continue...\n");
        while (!tlKbHit());
        tlGetCH();

        grLfbEnd();
#endif
        break;

      case 'h':
      case 'H':
      case '?':
        doHelp();
        break;

      case 'i':
      case 'I':
        if (background == FXTRUE) {
          background = FXFALSE;
          tlConOutput("Turning off background\n");
        } else {
          tlConOutput("Turning on background\n");
          background = FXTRUE;
        }
        break;

      case 'l':
      case 'L':
        if (lines == FXTRUE) {
          lines = FXFALSE;
          tlConOutput("Turning OFF lines\n");
        } else {
          lines = FXTRUE;
          tlConOutput("Turning ON lines\n");
        }
        break;
      case 'm':
      case 'M':
        ccFnc = GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB;
        break;

      case 'n':
      case 'N':
        if (printPerf == FXFALSE) {
          printPerf = FXTRUE;
          firstTime = 1;
          grSstResetPerfStats();
          grResetTriStats();
        } else {
          printPerf= FXFALSE;
        }
        break;

      case 'o':
      case 'O':
        if (origin == GR_ORIGIN_LOWER_LEFT)
          origin = GR_ORIGIN_UPPER_LEFT;
        else
          origin = GR_ORIGIN_LOWER_LEFT; 

        grSstOrigin(origin);
        break;

      case 'p':
      case 'P':
        if (render == FXTRUE)
          render = FXFALSE;
        else
          render = FXTRUE;
        break;
        
      case 'q':
      case 'Q':
      case 27:
        grGlideShutdown();
        exit(0);
        break;

      case 'r':
      case 'R':
        tlConOutput("Screen Resolution is %s\n", tlGetResolutionString( resolution ) );
        break;

      case 'S':
      case 's':
        if (cullMode == GR_CULL_DISABLE) {
          cullMode = GR_CULL_NEGATIVE;
          tlConOutput("Turning ON backface culling (hidden Surface removal)\n");
        } else  {
          cullMode = GR_CULL_DISABLE;
          tlConOutput("Turning OFF backface culling (hidden Surface removal)\n");
        }
        grCullMode(cullMode);
        break;
      case 'T':
      case 't':
        if (texturing == FXFALSE) {
          tlConOutput("Turning ON texturing\n");
          ccFnc = GR_COLORCOMBINE_DECAL_TEXTURE;
          texturing = FXTRUE;
        } else {
          tlConOutput("Turning OFF texturing\n");
          ccFnc = GR_COLORCOMBINE_ITRGB;
          texturing = FXFALSE;
        }
        break;
      case 'u':
      case 'U':
        if (plugging == FXTRUE) 
          plugging = FXFALSE;
        else
          plugging = FXTRUE;

        grGlideShamelessPlug(plugging);
        break;
        
      case 'v':
      case 'V':
        if (passthruMode == GR_PASSTHRU_SHOW_VGA) {
          passthruMode = GR_PASSTHRU_SHOW_SST1;
          tlConOutput("Changing Passthru to show Voodoo\n");
        }
        else {
          passthruMode = GR_PASSTHRU_SHOW_VGA;
          tlConOutput("Changing Passthru to show VGA\n");
        }
        /* grSstPassthruMode(passthruMode); */
        break;

      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '0':
        {
          char str[256];
          swapDelay = (int) c - 0x30;
          sprintf(str, "Swapdelay = %d\n", swapDelay);
          tlConOutput(str);
        }
        break;
      }
    }
    
    if (render) {
      y_angle += 2.f;
      if( y_angle > 360.0f )
        y_angle -= 360.0f;
    }
    frameCount++;
    if(frameCount < 0)
      frameCount = 0;
    if(frames == frameCount)
      break;
  }
  grGlideShutdown();
  return 0;
}
