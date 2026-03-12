/*
 ** Insert new header here
 **
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <glide.h>
#include "tlib.h"


#define SNAP_COORD(v)   ( ( v ) = (( float )( ( FxI32 )( ( v ) * 16 ) )) / 16.0f )

#define WREFWIDTH 640.f
#define WREFHEIGHT 480.f

#define WINSCALEX(x) ((x/WREFWIDTH) * wWidth)
#define WINSCALEY(y) ((y/WREFHEIGHT) * wHeight)  

#ifndef M_PI
#define M_PI    3.14159265358979323846f
#endif
#define DEG2RAD( x ) ( (x) * M_PI / 180.0f )

#define VERT_COUNT 3

int hwconfig;
static const char *version;

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
    localVerts[VERT_COUNT],     /* Vertices in world coordinates */
    texVerts[4];                /* Texture vertices for background */
    
  float
    alpha = 192.0f,             /* Alpha for blending tringle over background */
    y_angle = 0.0f;             /* rotation amount */
    
  int
    firstTime = 1;              /* Used for performance calculations */

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
    
  FxU32 startAddress = 0, bgDecalAddress = 0, triDecalAddress = 0;

  GrTexInfo
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

  FxU32 wrange[2];
  FxI32 state_size;
  void *oldState;

  /* Initialize Glide */
  grGlideInit();
  assert( hwconfig = tlVoodooType() );

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

  version = grGetString( GR_VERSION );
    
  printf( "%s:\n%s\n", name, purpose );
  printf( "%s\n", version );
  printf( "Resolution: %s\n", tlGetResolutionString( resolution ) );
  if ( frames == -1 ) {
    printf( "Press A Key To Begin Test.\n" );
    tlGetCH();
  }
    
  grSstSelect( 0 );
  assert( grSstWinOpen(tlGethWnd(),
                       resolution,
                       GR_REFRESH_60Hz,
                       GR_COLORFORMAT_ABGR,
                       origin,
                       2, 1 ) );

  grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_A,   GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);

  grGet(GR_WDEPTH_MIN_MAX, 8, wrange);  
  grGet(GR_GLIDE_STATE_SIZE, 4, &state_size);
  oldState = malloc(state_size);        

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

    bgDecal.smallLodLog2    = bgInfo.header.small_lod;
    bgDecal.largeLodLog2    = bgInfo.header.large_lod;
    bgDecal.aspectRatioLog2 = bgInfo.header.aspect_ratio;
    bgDecal.data            = bgInfo.data;
    bgDecal.format          = bgInfo.header.format;
    grTexDownloadMipMap( GR_TMU0, startAddress, GR_MIPMAPLEVELMASK_BOTH, &bgDecal);
    grTexMipMapMode(GR_TMU0, GR_MIPMAP_NEAREST, FXTRUE);
    grTexClampMode( GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
    grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR );
    bgDecalAddress = startAddress;
    startAddress += grTexCalcMemRequired ( bgDecal.smallLodLog2, bgDecal.largeLodLog2, 
                                           bgDecal.aspectRatioLog2, bgDecal.format );
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

    triDecal.smallLodLog2    = triInfo.header.small_lod;
    triDecal.largeLodLog2    = triInfo.header.large_lod;
    triDecal.aspectRatioLog2 = triInfo.header.aspect_ratio;
    triDecal.data            = triInfo.data;
    triDecal.format          = triInfo.header.format;
    grTexDownloadMipMap( GR_TMU0, startAddress, GR_MIPMAPLEVELMASK_BOTH, &triDecal);
    grTexMipMapMode(GR_TMU0, GR_MIPMAP_NEAREST, FXTRUE);
    grTexClampMode( GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
    grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR );
    triDecalAddress = startAddress;
    free( triInfo.data );
  } else {
    fprintf( stderr, "could not get info on %s\n", triFileName );
    grGlideShutdown();
    exit( -1 );
  }

  grTexCombine( GR_TMU0, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
                GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
  grRenderBuffer(backbuffer == FXTRUE ? GR_BUFFER_BACKBUFFER : GR_BUFFER_FRONTBUFFER);

  /* Set up alpha blending for AA and compositing... */
  grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL, 
                  GR_COMBINE_FACTOR_NONE, 
                  GR_COMBINE_LOCAL_ITERATED, 
                  GR_COMBINE_OTHER_NONE, 
                  FXFALSE );
  grAlphaBlendFunction( GR_BLEND_SRC_ALPHA,
                       GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ONE,
                       GR_BLEND_ZERO );
  grAlphaTestFunction( GR_CMP_ALWAYS );


  while ( 1 ) {
    Matrix rotm;
    GrVertex xformedVerts[VERT_COUNT];
    int i;

    
    MatMakeYRot( rotm, DEG2RAD( y_angle ) );
    
    if (resolution == GR_RESOLUTION_NONE) 
      tlGetResolutionConstant("0",
                              &scrWidth, 
                              &scrHeight );


    for( i = 0; i < VERT_COUNT; i++ ) {
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
    
    switch ( ccFnc )
      {
      case GR_COLORCOMBINE_ITRGB:
        grColorCombine( GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_NONE, FXFALSE );
        break;
        
      case GR_COLORCOMBINE_DECAL_TEXTURE:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;
        
      case GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;
      }
    
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

            grGet(GR_STATS_PIXELS_IN,             4, &pStats.pixelsIn);
            grGet(GR_STATS_PIXELS_CHROMA_FAIL,    4, &pStats.chromaFail);
            grGet(GR_STATS_PIXELS_DEPTHFUNC_FAIL, 4, &pStats.zFuncFail);
            grGet(GR_STATS_PIXELS_AFUNC_FAIL,     4, &pStats.aFuncFail);
            grGet(GR_STATS_PIXELS_OUT,            4, &pStats.pixelsOut);
            grGet(GR_STATS_TRIANGLES_IN,          4, &trisProcessed);
            grGet(GR_STATS_TRIANGLES_OUT,         4, &trisDrawn);
            
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

            grReset(GR_STATS_PIXELS);
          } else {
            lastFrame = frameNum;
            grReset(GR_STATS_PIXELS);
            firstTime = 0;
          }
        }
      }

      grBufferClear( 0xffffffff, 0, (FxU16)wrange[1] );
    
      if (background == FXTRUE) {

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

        grGlideGetState(oldState);

        grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
        grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);

        grAlphaBlendFunction(
                             GR_BLEND_ONE, GR_BLEND_ZERO,
                             GR_BLEND_ONE, GR_BLEND_ZERO);

        grColorCombine(
                       GR_COMBINE_FUNCTION_SCALE_OTHER,
                       GR_COMBINE_FACTOR_ONE,
                       GR_COMBINE_LOCAL_NONE,
                       GR_COMBINE_OTHER_TEXTURE, FXFALSE
                       );

        grTexSource( GR_TMU0, bgDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &bgDecal );
        
        for (i = 0; i < NTRIS; i++) {
          grDrawTriangle(&texVerts[0], &texVerts[1], &texVerts[2]);
          grDrawTriangle(&texVerts[2], &texVerts[3], &texVerts[0]);
        }
        grGlideSetState(oldState);
        grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);

      }

      if (texturing == FXTRUE)
        grTexSource( GR_TMU0, bgDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &bgDecal );

      if (texturing)
        grTexSource( GR_TMU0, triDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &triDecal );

      if (antialias == FXTRUE) {
        grEnable(GR_AA_ORDERED);
        if (lines == FXTRUE) {
          grDrawLine(&xformedVerts[0], &xformedVerts[1]);
          grDrawLine(&xformedVerts[1], &xformedVerts[2]);
          grDrawLine(&xformedVerts[2], &xformedVerts[0]);
        } else {
          grAADrawTriangle(
                           &xformedVerts[0],  &xformedVerts[1],
                           &xformedVerts[2], FXTRUE, FXTRUE, FXTRUE
                           );
        }
      } else {
        grDisable(GR_AA_ORDERED);
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
          grTexFilterMode ( GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR );
        } else {
          bilinear = FXFALSE;
          tlConOutput("Turning OFF BiLinear blending\n");
          grTexFilterMode ( GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, 
                          GR_TEXTUREFILTER_POINT_SAMPLED );
        }
        break;

      case 'c':
      case 'C':
        if (blend == FXTRUE) {
                  int i;
          blend = FXFALSE;
                  for(i=0;i<VERT_COUNT;i++) localVerts[i].a = 255.0f;
        } else {
                  int i;
          blend = FXTRUE;
          for(i=0;i<VERT_COUNT;i++) localVerts[i].a = alpha;
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
          grReset(GR_STATS_PIXELS);
          grReset(GR_STATS_TRIANGLES);
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
          grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
          grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);
        } else {
          tlConOutput("Turning OFF texturing\n");
          ccFnc = GR_COLORCOMBINE_ITRGB;
          texturing = FXFALSE;
          grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_DISABLE);
          grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_DISABLE);
        }
        break;
      case 'u':
      case 'U':
        if (plugging == FXTRUE) {
          plugging = FXFALSE;
          grDisable(GR_SHAMELESS_PLUG);
        }
        else {
          plugging = FXTRUE;
          grEnable(GR_SHAMELESS_PLUG);
        }
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
