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
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <glide.h>
#include <g3ext.h>
#include "tlib.h"


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
const char *version;
const char *extension;

const char name[]    = "test41";
const char purpose[] = "Texture Buffer Extension (Render into a texture) test";
const char usage[]   = "-n <frames> -r <res> -p <pixel format> -b <bgfile.3df> -t <trifile.3df>";
FxBool gWireframe = FXFALSE;

GrProc grChromaRangeModeExt = NULL;
GrProc grChromaRangeExt = NULL;
GrProc grTextureBufferExt = NULL;
GrProc grTextureAuxBufferExt = NULL;
GrProc grAuxBufferExt = NULL;


GrVertex
  localVerts[VERT_COUNT],     /* Vertices in world coordinates */
  texVerts[4];                /* Texture vertices for background */
GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
float                scrWidth   = 640.0f;
float                scrHeight  = 480.0f;
int frames                      = -1;

float
  minColor = 10.f,            /* Vertex min color */
  maxColor = 245.f;           /* Vertex max color */
  
  
float
  y_angle = 0.0f,             /* rotation amount for inner triangles */
  y_angle_main = 0.0f;        /* rotation amount */
  
int
  firstTime;                  /* Used for performance calculations */

FxBool
  plugging = FXFALSE,         /* Show shameless plug */
  printPerf = FXFALSE,        /* Print performance numbers */
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
  lastFrame,                  /* Number of last frame we did perf stats */
  frameNum = 0L;              /* id of each frame drawn */
  
GrCullMode_t
  cullMode = GR_CULL_DISABLE; /* backface culling */
GrContext_t
  curContext;
  
FxU32 startAddress = 0, bgDecalAddress = 0, triDecalAddress = 0, trdDecalAddress = 0,
      fourthDecalAddress = 0, fifthDecalAddress = 0, sixthDecalAddress = 0;

GrTexInfo
  triDecal,                   /* Triangle decal texture */
  bgDecal,                    /* Background decal texture */
  trdDecal,
  fourthDecal,                /* first dynamic texture in which we'll be rendering to */
  fifthDecal,                 /* second dynamic texture in which we'll be rendering to */
  sixthDecal;                 /* texture aux. buffer. that we'll use when rendering into */
                              /* the 2 texture buffers (instead of using the default aux buffers */
  
Gu3dfInfo
  bgInfo,                     /* Info on background texture */
  triInfo,                    /* Info on triangle texture */
  trdInfo;

GrColorCombineFnc_t
  ccFnc = GR_COLORCOMBINE_ITRGB; /* Start of w/ Gouraud shading */
  
char
  *bgFileName = NULL,         /* Name of background texture file */
  *triFileName = NULL,        /* Name of triangle texture file */
  *trdFileName = NULL;
  
int
  frameCount = 0;

FxU32 wrange[2];
FxU32 pixelformat = GR_PIXFMT_RGB_565 ;
static tlGlideExtension glideext;

int lfbOffset = 50;
float lodBias = 0.0f;
void *lfbData = NULL;
int lfbWidth = 100, lfbHeight = 82;
char *extstr;

GrVertex xformedVerts[VERT_COUNT];
GrVertex xformedVertsMain[VERT_COUNT];




#define DRAW_TRIANGLE( A, B, C ) \
if ( gWireframe ) { \
  grDrawLine( A, B ) ; \
  grDrawLine( B, C ) ; \
  grDrawLine( C, A ) ; \
} \
else {\
  grDrawTriangle( A, B, C ); \
};

void byebye()
{
  exit( -1 );
}

void *makeChecker565(int width, int height)
{
  FxU16 *data = malloc( width * height * sizeof( FxU16 ) );
  int i, j;
  if ( ! data )
    byebye();    
  for ( j = 0 ; j < height ; j++ )
  {
    for ( i = 0 ; i < width ; i++ )
    {
      *(data+width*j+i) = ( i%2 && j%2 ) ? 0x0: ( j < 32 ) ? 0x001f : 0x07e0;
    } /* endfor i < width */
  } /* endfor j < height */
  return data;
}

void myLfbWriteRegion( 
                  GrOriginLocation_t origin,
                  GrBuffer_t dst_buffer, 
                  FxU32 dst_x, FxU32 dst_y, 
                  GrLfbSrcFmt_t src_format, 
                  FxU32 src_width, FxU32 src_height, 
                  FxBool pixelPipeline,
                  FxI32 src_stride, void *src_data )
{
  GrLfbInfo_t      info;
  info.size = sizeof(info);

  // Turning off texture keeps the sim from bitching
  // not necessary w/ hardware, though.
  grColorCombine(
                 GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_NONE,
                 GR_COMBINE_OTHER_NONE, FXFALSE
                 );

  if (grLfbLock(GR_LFB_WRITE_ONLY, 
                 dst_buffer, 
                 src_format,
                 origin,
                 pixelPipeline,
                 &info)) 
  {
    FxU32 i, j;
    FxU16 *sdata = (FxU16*)src_data;
    for ( j = 0 ; j < src_height; j++ )
    {
      for ( i = 0 ; i < src_width ; i++ )
      {
        *((FxU16*)info.lfbPtr + (info.strideInBytes / 2 ) * (dst_y+j) + dst_x + i ) 
          = *(sdata + src_width * j + i);
      }/* endfor i < src_width */
    }/* endfor j < src_height */
    grLfbUnlock(GR_LFB_WRITE_ONLY, dst_buffer);
  } /* endif grLfbLock */  

  // Turn texture back on... we're past the point where
  // the sim would bitch.
  grColorCombine(
                 GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_NONE,
                 GR_COMBINE_OTHER_TEXTURE, FXFALSE
                 );
}


static void doHelp( void ) {
  tlConOutput("Keymap:\n");
  tlConOutput("        H:         Help\n");
  tlConOutput("        N:         print performance Numbers\n");
  tlConOutput("        P:         Pause rendering\n");
  tlConOutput("        R:         Print screen Resolution\n");
  tlConOutput("        Q or esc:  Quit\n");
  tlConOutput("        U:         toggle shameless Plug\n");
  tlConOutput("        1:         decrease LFB Y offset\n");
  tlConOutput("        2:         increase LFB Y offset\n");
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

// These are only used to calc performance numbers
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
  856*480,
  512*256,
  1024*768,
  1280*1024,
  1600*1200,
  400*300,
  1152*864,
  1280*960,
  1600*1024,
  1792*1344,
  1856*1392,
  1920*1440,
  2048*1536,
  2048*2048
};

#define NTRIS 1
#define NFRAMES 20
#define NVERTS 3

/*
** This function does the rendering into texture thing.
** It can be called anywhere within the 'main' frame, since
** it uses its own aux. buffer (and obviously its own color buffers
** which are the texture buffers), and because it resets the correct
** glide sets once it is done.
*/
void renderIntoTextures()
{
  int i;
  grChromaRangeModeExt(GR_CHROMARANGE_ENABLE_EXT);
  grChromaRangeExt( 0x0000ff00, 0x0000ff00, GR_CHROMARANGE_RGB_ALL_EXT);
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

  /* Tell glide we are going to use the texture specified by grTextureBuffer() as the rendering buffer */
  grRenderBuffer( GR_BUFFER_TEXTUREBUFFER_EXT );
  /* Tell glide we are going to use the texture specified by grTextureAuxBuffer() as the aux buffer */
  grAuxBufferExt( GR_BUFFER_TEXTUREAUXBUFFER_EXT );

  /* First render into the fourth texture */
  grTextureBufferExt( GR_TMU0, fourthDecalAddress , fourthDecal.largeLodLog2, fourthDecal.largeLodLog2, 
                      fourthDecal.aspectRatioLog2, fourthDecal.format, GR_MIPMAPLEVELMASK_BOTH ); 

  /* Note: the following clear clears both the fourth texture (the color buffer)
  ** and the sixth texture (the aux buffer).f
  */
  grBufferClear( 0x000000ff, 0, wrange[1] );

  /* Render the 2 big background triangles witht the bg texture (miro). */
  grTexSource( GR_TMU0, bgDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &bgDecal );
  for (i = 0; i < NTRIS; i++) 
  {
    DRAW_TRIANGLE(&texVerts[0], &texVerts[1], &texVerts[2]);
    DRAW_TRIANGLE(&texVerts[2], &texVerts[3], &texVerts[0]);
  }

  /* render 1 spinning triangles with the triDecal texture (blurb.tga) */
  grTexSource( GR_TMU0, triDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &triDecal );
  {          
    GrVertex verts[VERT_COUNT];
    verts[0] = xformedVerts[0];
    verts[1] = xformedVerts[1];
    verts[2] = xformedVerts[2];
    verts[0].x -= 250.0f;
    verts[1].x -= 250.0f;
    verts[2].x -= 250.0f;
    verts[0].y += 20.0f;
    verts[1].y += 20.0f;
    verts[2].y += 20.0f;
    verts[0].x *= .6f;
    verts[1].x *= .6f;
    verts[2].x *= .6f;
    verts[0].y *= .6f;
    verts[1].y *= .6f;
    verts[2].y *= .6f;
    DRAW_TRIANGLE(
      &verts[0], &verts[1],
      &verts[2]
    );
  }
  /* Lfb 
  ** Lfb stuff is here just to demonstrate that you can do regular stuff
  ** inside the texturebuffer, like a 3D lfb for example (the lfb be is
  ** is applied thru the pipeline, using the chromakeying settings).
  ** The lfb is a checker board pattern: the first part is a green & black pattern
  ** and the second part is a blue and black pattern.
  ** Since we are chromakeying on the green, the chunk of the lfb that contained
  ** the green pixels will be transparent (except for where the black pixels were).
  ** End result: a chunk of opaque blue&black checker pattern, and a grid of black texels.
  */
  grDepthBufferMode( GR_DEPTHBUFFER_DISABLE );
  myLfbWriteRegion( origin,
    GR_BUFFER_TEXTUREBUFFER_EXT,
    50, lfbOffset, GR_LFB_SRC_FMT_565,
    lfbWidth, lfbHeight,
    FXTRUE, lfbWidth*2, lfbData );
  grDepthBufferMode( GR_DEPTHBUFFER_WBUFFER );

  /* Second dynamic texture.
  ** Render into the fifth texture
  */
  grTextureBufferExt( GR_TMU0, fifthDecalAddress , fifthDecal.largeLodLog2, fifthDecal.largeLodLog2, 
                      fifthDecal.aspectRatioLog2, fifthDecal.format, GR_MIPMAPLEVELMASK_BOTH ); 

  /* Note: the following clear clears both the fourth texture (the color buffer)
  ** and the sixth texture (the aux buffer).f
  */
  grBufferClear( 0x000000ff, 0, wrange[1] );

  /* Render the 2 big background triangles witht the bg texture (miro). */
  grTexSource( GR_TMU0, bgDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &bgDecal );
  for (i = 0; i < NTRIS; i++) 
  {
    DRAW_TRIANGLE(&texVerts[0], &texVerts[1], &texVerts[2]);
    DRAW_TRIANGLE(&texVerts[2], &texVerts[3], &texVerts[0]);
  }
  /* render 1 spinning triangle with the trdDecal texture */
  grTexSource( GR_TMU0, trdDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &trdDecal );
  {          
    GrVertex verts[VERT_COUNT];
    verts[0] = xformedVerts[0];
    verts[1] = xformedVerts[1];
    verts[2] = xformedVerts[2];
    verts[0].x -= 250.0f;
    verts[1].x -= 250.0f;
    verts[2].x -= 250.0f;
    verts[0].y += 20.0f;
    verts[1].y += 20.0f;
    verts[2].y += 20.0f;
    verts[0].x *= .55f;
    verts[1].x *= .55f;
    verts[2].x *= .55f;
    verts[0].y *= .55f;
    verts[1].y *= .55f;
    verts[2].y *= .55f;
    DRAW_TRIANGLE(
      &verts[0], &verts[1],
      &verts[2]
    );
  }

  /* Lfb 
  ** Lfb stuff is here just to demonstrate that you can do regular stuff
  ** inside the texturebuffer, like a 3D lfb for example (the lfb be is
  ** is applied thru the pipeline, using the chromakeying settings).
  ** The lfb is a checker board pattern: the first part is a green & black pattern
  ** and the second part is a blue and black pattern.
  ** Since we are chromakeying on the green, the chunk of the lfb that contained
  ** the green pixels will be transparent (except for where the black pixels were).
  ** End result: a chunk of opaque blue&black checker pattern, and a grid of black texels.
  */
  grDepthBufferMode( GR_DEPTHBUFFER_DISABLE );
  myLfbWriteRegion( origin,
    GR_BUFFER_TEXTUREBUFFER_EXT,
    0, lfbOffset, GR_LFB_SRC_FMT_565,
    lfbWidth, lfbHeight,
    FXTRUE, lfbWidth*2, lfbData );
  grDepthBufferMode( GR_DEPTHBUFFER_WBUFFER );


  /* Restore State */
  grChromaRangeModeExt( GR_CHROMARANGE_DISABLE_EXT );

  /* tell glide to use regular bacbuffer and auxbuffer */
  grRenderBuffer( GR_BUFFER_BACKBUFFER );
  grAuxBufferExt( GR_BUFFER_AUXBUFFER );

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
}






int main( int argc, char **argv) {
  char match; 
  char **remArgs;
  int  rv;

  /* Initialize Glide */
  grGlideInit();
  assert( hwconfig = tlVoodooType() );

  /* Process Command Line Arguments */
  while((rv = tlGetOpt(argc, argv, "nrpbtea", &match, &remArgs)) != 0) {
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
    case 'p':
      pixelformat = tlGetPixelFormat( remArgs[0] );
      break;
    }
  }

  tlSetScreen( scrWidth, scrHeight );

  version = grGetString( GR_VERSION );
    
  printf( "%s:\n%s\n", name, purpose );
  printf( "%s\n", version );
  printf( "Resolution: %s\n", tlGetResolutionString( resolution ) );

  if ( frames == -1 ) 
  {
    printf( "Press A Key To Begin Test.\n" );
    tlGetCH();
  }

  grSstSelect(0) ;

  tlInitGlideExt(&glideext);

  if (glideext.grSstWinOpen) 
  {
    curContext = glideext.grSstWinOpen(tlGethWnd(),
                                       resolution,
                                       GR_REFRESH_60Hz,
                                       GR_COLORFORMAT_ABGR,
                                       origin,
                                       pixelformat,
                                       2, 1);
   }
    else 
   {
    curContext = grSstWinOpen(tlGethWnd(),
                              resolution,
                              GR_REFRESH_60Hz,
                              GR_COLORFORMAT_ABGR,
                              origin,
                              2, 1 );
   }

  if (!curContext) {
    printf("Could not allocate glide fullscreen context.\n");
    exit(-1);
  }

  extension = grGetString(GR_EXTENSION);

  if ((extstr = strstr(extension, "TEXCHROMA")) != NULL) {
    if (!strncmp(extstr, "TEXCHROMA", 9)) {
      grChromaRangeModeExt = grGetProcAddress("grChromaRangeModeExt");
      grChromaRangeExt = grGetProcAddress("grChromaRangeExt");
    }
  }

  if ((extstr = strstr(extension, "TEXTUREBUFFER")) != NULL) {
    if (!strncmp(extstr, "TEXTUREBUFFER", 13)) {
      grTextureBufferExt = grGetProcAddress("grTextureBufferExt");
      grTextureAuxBufferExt = grGetProcAddress("grTextureAuxBufferExt");
      grAuxBufferExt = grGetProcAddress("grAuxBufferExt");
    }
  }

  if ( !grChromaRangeModeExt | !grChromaRangeExt | !grTextureBufferExt 
       | !grTextureAuxBufferExt | !grAuxBufferExt )
    byebye();


  grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_A,   GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);

  grGet(GR_WDEPTH_MIN_MAX, 8, (FxI32 *)wrange);  

  tlConSet( 0.0f, 0.0f, 1.0f, 1.0f,
            80, 40, 0xffffff );

  grRenderBuffer(GR_BUFFER_BACKBUFFER);
  grBufferClear( 0x0, 0, wrange[1] );

  if(frames == -1) 
  {
    doHelp();
  }

  localVerts[0].x = 0.f;
  localVerts[0].y = 0.75f;
  localVerts[0].z = 0.0f;
  localVerts[0].tmuvtx[0].sow = 0.f;
  localVerts[0].tmuvtx[0].tow = 0.f;
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
  localVerts[2].tmuvtx[0].tow = 255.f;
  localVerts[2].oow = 1.f;  
  localVerts[2].r = minColor;
  localVerts[2].g = minColor;
  localVerts[2].b = maxColor;
  localVerts[2].a = 255.f;

  texVerts[0].x = 0.f;
  texVerts[0].y = 0.f;
  texVerts[0].a = 255.f;
#define FAR wrange[1]
  texVerts[0].oow = 1.f/FAR;
  texVerts[0].tmuvtx[0].sow = 0.f * texVerts[0].oow;
  texVerts[0].tmuvtx[0].tow = 255.f * texVerts[0].oow;

  texVerts[1].x = scrWidth;
  texVerts[1].y = 0.f;
  texVerts[1].a = 255.f;
  texVerts[1].oow = 1.f/FAR;
  texVerts[1].tmuvtx[0].sow = 255.f * texVerts[1].oow;
  texVerts[1].tmuvtx[0].tow = 255.f * texVerts[1].oow;

  texVerts[2].x = scrWidth;
  texVerts[2].y = scrHeight;
  texVerts[2].a = 255.f;
  texVerts[2].oow = 1.f/FAR;
  texVerts[2].tmuvtx[0].sow = 255.f * texVerts[2].oow;
  texVerts[2].tmuvtx[0].tow = 0.f * texVerts[2].oow;
  
  texVerts[3].x = 0.f;
  texVerts[3].y = scrHeight;
  texVerts[3].a = 255.f;
  texVerts[3].oow = 1.f/FAR;
  texVerts[3].tmuvtx[0].sow = 0.f * texVerts[3].oow;
  texVerts[3].tmuvtx[0].tow = 0.f * texVerts[3].oow;
  
  if (bgFileName == NULL)
    bgFileName = "miro.3df";
  if (triFileName == NULL)
    triFileName = "alpha.3df";
  trdFileName = "decal1.3df";

 
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

    bgDecalAddress = startAddress;
    startAddress += grTexCalcMemRequired ( bgDecal.smallLodLog2, bgDecal.largeLodLog2, 
                                           bgDecal.aspectRatioLog2, bgDecal.format );
    grTexDownloadMipMap( GR_TMU0, bgDecalAddress,  GR_MIPMAPLEVELMASK_BOTH, &bgDecal);
  } else {
    fprintf( stderr, "could not get info on %s\n", bgFileName );
    grGlideShutdown();
    exit( -1 );
  }
  

  if ( gu3dfGetInfo( triFileName, &triInfo ) )  
  {
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

    triDecalAddress = startAddress;
    startAddress += grTexCalcMemRequired ( triDecal.smallLodLog2, triDecal.largeLodLog2, 
                                           triDecal.aspectRatioLog2, triDecal.format );
    grTexDownloadMipMap( GR_TMU0, triDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &triDecal);
  } else {
    fprintf( stderr, "could not get info on %s\n", triFileName );
    grGlideShutdown();
    exit( -1 );
  }



  if ( gu3dfGetInfo( trdFileName, &trdInfo ) )  
  {
    trdInfo.data = malloc( trdInfo.mem_required );
    
    if ( trdInfo.data == 0 ) {
      fprintf( stderr, "out of memory for texture file  %s\n", trdFileName );
      grGlideShutdown();
      exit( -1 );
    }
    
    if ( !gu3dfLoad( trdFileName, &trdInfo ) ) {
      fprintf( stderr, "could not load texture file %s\n", trdFileName);
      grGlideShutdown();
      exit( -1 );
    }

    trdDecal.smallLodLog2    = trdInfo.header.small_lod;
    trdDecal.largeLodLog2    = trdInfo.header.large_lod;
    trdDecal.aspectRatioLog2 = trdInfo.header.aspect_ratio;
    trdDecal.data            = trdInfo.data;
    trdDecal.format          = trdInfo.header.format;

    trdDecalAddress = startAddress;
    startAddress += grTexCalcMemRequired ( trdDecal.smallLodLog2, trdDecal.largeLodLog2, 
                                           trdDecal.aspectRatioLog2, trdDecal.format );
    grTexDownloadMipMap( GR_TMU0, trdDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &trdDecal);
  } else {
    fprintf( stderr, "could not get info on %s\n", trdFileName );
    grGlideShutdown();
    exit( -1 );
  }


  //
  // fourthDecal and fifthDecal are going to be the texture render buffers
  // Since we clear them then render into them, there's no point in loading
  // a texture... Plus, at least for now, they need to be in a texture format
  // compatible with the current rendering format.
  //
  // sixthDecal is just an auxbuffer for fourthDecal and fifthDecal... It needs
  // to be in an equivalent format for now also.
  //
  // Sure this could be written nicer... If you care, go ahead.
  //
  {
    fourthDecal.smallLodLog2    = fourthDecal.largeLodLog2    = GR_LOD_LOG2_256 ;
    fourthDecal.aspectRatioLog2 = GR_ASPECT_LOG2_1x1 ;

    switch (pixelformat)
    {
      case GR_PIXFMT_ARGB_8888:
      case GR_PIXFMT_AA_2_ARGB_8888:
      case GR_PIXFMT_AA_4_ARGB_8888:
        fourthDecal.format = GR_TEXFMT_ARGB_8888 ;
        break ;

      case GR_PIXFMT_ARGB_1555:
        fourthDecal.format = GR_TEXFMT_ARGB_1555 ;
        break ;

      default:
        fourthDecal.format = GR_TEXFMT_RGB_565 ;
        break ;
    } ;

    fourthDecalAddress = startAddress;

    startAddress += grTexCalcMemRequired(fourthDecal.smallLodLog2, fourthDecal.largeLodLog2, 
                                         fourthDecal.aspectRatioLog2, fourthDecal.format) ;


    fifthDecal.smallLodLog2    = fifthDecal.largeLodLog2    = GR_LOD_LOG2_256 ;
    fifthDecal.aspectRatioLog2 = GR_ASPECT_LOG2_1x1 ;

    switch (pixelformat)
    {
      case GR_PIXFMT_ARGB_8888:
      case GR_PIXFMT_AA_2_ARGB_8888:
      case GR_PIXFMT_AA_4_ARGB_8888:
        fifthDecal.format = GR_TEXFMT_ARGB_8888 ;
        break ;

      case GR_PIXFMT_ARGB_1555:
        fifthDecal.format = GR_TEXFMT_ARGB_1555 ;
        break ;

      default:
        fifthDecal.format = GR_TEXFMT_RGB_565 ;
        break ;
    } ;

    fifthDecalAddress = startAddress;

    startAddress += grTexCalcMemRequired(fifthDecal.smallLodLog2, fifthDecal.largeLodLog2, 
                                         fifthDecal.aspectRatioLog2, fifthDecal.format) ;

    sixthDecal.smallLodLog2    = sixthDecal.largeLodLog2    = GR_LOD_LOG2_256 ;
    sixthDecal.aspectRatioLog2 = GR_ASPECT_LOG2_1x1 ;

    switch (pixelformat)
    {
      case GR_PIXFMT_ARGB_8888:
      case GR_PIXFMT_AA_2_ARGB_8888:
      case GR_PIXFMT_AA_4_ARGB_8888:
        sixthDecal.format = GR_TEXFMT_ARGB_8888 ;
        break ;

      case GR_PIXFMT_ARGB_1555:
        sixthDecal.format = GR_TEXFMT_ARGB_1555 ;
        break ;

      default:
        sixthDecal.format = GR_TEXFMT_RGB_565 ;
        break ;
    } ;

    sixthDecalAddress = startAddress;

    startAddress += grTexCalcMemRequired(sixthDecal.smallLodLog2, sixthDecal.largeLodLog2, 
                                         sixthDecal.aspectRatioLog2, sixthDecal.format) ;
  }



 
  /* Lfb */
  lfbData = makeChecker565( lfbWidth, lfbHeight );

  ccFnc = GR_COLORCOMBINE_DECAL_TEXTURE;
  grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);

  /* Important. 
  ** Need to call grTextureBufferExt once before calling grRenderBuffer( GR_BUFFER_TEXTUREBUFFER_EXT ) 
  ** Here it doesn't really matter what we specify, as we'll be changing it inside the loop
  */
  grTextureBufferExt(  GR_TMU0, fourthDecalAddress , fourthDecal.largeLodLog2, fourthDecal.largeLodLog2, 
                    fourthDecal.aspectRatioLog2, fourthDecal.format, GR_MIPMAPLEVELMASK_BOTH ); 

  /* Call grTextureAuxBufferExt only once. We'll be using our own aux buffer, but this will stay the same.
  ** We'll simply toggle its usage on/off by calling grAuxBufferExt()
  */
  grTextureAuxBufferExt(  GR_TMU0, sixthDecalAddress , sixthDecal.largeLodLog2, sixthDecal.largeLodLog2, 
                    sixthDecal.aspectRatioLog2, sixthDecal.format, GR_MIPMAPLEVELMASK_BOTH ); 

  /* Disable mipmapping. */
  grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
  grTexClampMode( GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
  grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR );        
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

  grDepthBufferFunction( GR_CMP_LEQUAL );
  grDepthBufferMode( GR_DEPTHBUFFER_WBUFFER );
  grDepthMask( FXTRUE );

 
  while ( 1 ) 
  {
    Matrix rotm;
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
    }

    MatMakeZRot( rotm, DEG2RAD( y_angle_main ) );
    for( i = 0; i < VERT_COUNT; i++ ) {
      PointMatMult( &xformedVertsMain[i], &localVerts[i], rotm );
      xformedVertsMain[i].x = xformedVertsMain[i].x / ( xformedVertsMain[i].z + 2.0f );
      xformedVertsMain[i].y = xformedVertsMain[i].y / ( xformedVertsMain[i].z + 2.0f );
      xformedVertsMain[i].x *= scrWidth / 2.0f;
      xformedVertsMain[i].y *= scrHeight / 2.0f;
      xformedVertsMain[i].x += scrWidth / 2.0f;
      xformedVertsMain[i].y += scrHeight / 2.0f;
      xformedVertsMain[i].oow = 1.f / ((xformedVertsMain[i].z + 2) * scrHeight);
      xformedVertsMain[i].tmuvtx[0].sow *= xformedVertsMain[i].oow;
      xformedVertsMain[i].tmuvtx[0].tow *= xformedVertsMain[i].oow;
    }


    {
      switch ( ccFnc ) 
      {
      case GR_COLORCOMBINE_ITRGB:
        grColorCombine( GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, 
                        GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_NONE, 
                        FXFALSE );
        break;
        
      case GR_COLORCOMBINE_DECAL_TEXTURE:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, 
                        GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, 
                        FXFALSE );
        break;
        
      case GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, 
                        GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, 
                        FXFALSE );
        break;
      }
    
      if (render == FXTRUE) 
      {
        ++frameNum;
        /* Performance stuff */
        if ((frameNum % NFRAMES) == 0) 
        {
          if (printPerf) 
          {
            if (!firstTime)  
            {
              GrSstPerfStats_t
                pStats;
              FxU32
                lfbWritePixels,
                nFrames = frameNum - lastFrame,
                fillPixels = nFrames * screenFulls[resolution],
                totFail;
            
              lastFrame = frameNum;

              grGet(GR_STATS_PIXELS_IN,             4, (FxI32 *)&pStats.pixelsIn);
              grGet(GR_STATS_PIXELS_CHROMA_FAIL,    4, (FxI32 *)&pStats.chromaFail);
              grGet(GR_STATS_PIXELS_DEPTHFUNC_FAIL, 4, (FxI32 *)&pStats.zFuncFail);
              grGet(GR_STATS_PIXELS_AFUNC_FAIL,     4, (FxI32 *)&pStats.aFuncFail);
              grGet(GR_STATS_PIXELS_OUT,            4, (FxI32 *)&pStats.pixelsOut);
              grGet(GR_STATS_TRIANGLES_IN,          4, (FxI32 *)&trisProcessed);
              grGet(GR_STATS_TRIANGLES_OUT,         4, (FxI32 *)&trisDrawn);
            
              totFail = pStats.chromaFail + pStats.zFuncFail + pStats.aFuncFail;  
            
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
            }
            else /* first time */
            {
              lastFrame = frameNum;
              grReset(GR_STATS_PIXELS);
              firstTime = 0;
            } /* endif !first time */
          }
        }
        /* end performance stuff */

        grTexLodBiasValue(GR_TMU0, lodBias );

        grRenderBuffer(GR_BUFFER_BACKBUFFER);
        grBufferClear( 0x00ffff00, 0, wrange[1] );

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

        /* Render the bacgkround (2 big triangles) with the bgDecal */
        grTexSource( GR_TMU0, bgDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &bgDecal );
        for (i = 0; i < NTRIS; i++) 
        {
          DRAW_TRIANGLE(&texVerts[0], &texVerts[1], &texVerts[2]);
          DRAW_TRIANGLE(&texVerts[2], &texVerts[3], &texVerts[0]);
        }

        /* Call function that renders into texture memory.
        ** Note that we are right in the middle of rendering a frame.
        ** But we've created our own Aux buffer and are using it
        ** for the rendering into texture memory. This way, the current
        ** aux buffer remains untouched and the main rendering can resume
        ** into it later on.
        ** For a pipeline doing its special effect / rendering into a texture effect
        ** at the very begining or end of a frame, there'd be no need to create
        ** its own aux buffer. It could reuse the default aux buffer.
        */
        renderIntoTextures();

        /* Resume rendering the main scene that includes 2 big tris for the background
        ** (already drawn) and 2 spinning triangles.
        */

        {          
          GrVertex verts[VERT_COUNT];
          /* 1st spining triangle with the first texture we rendered into (fourthDecal) */
          grTexSource( GR_TMU0, fourthDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &fourthDecal );
          DRAW_TRIANGLE(
                         &xformedVertsMain[0], &xformedVertsMain[1],
                         &xformedVertsMain[2]
                         );
          /* 2nd spining triangle with the second texture we rendered into (fifthDecal) */
          grTexSource( GR_TMU0, fifthDecalAddress, GR_MIPMAPLEVELMASK_BOTH, &fifthDecal );
          verts[0] = xformedVertsMain[0];
          verts[1] = xformedVertsMain[1];
          verts[2] = xformedVertsMain[2];
          verts[0].y -= 100.0f;
          verts[1].y -= 100.0f;
          verts[2].y -= 100.0f;
          verts[0].x += 200.0f;
          verts[1].x += 200.0f;
          verts[2].x += 200.0f;
          DRAW_TRIANGLE(
                         &verts[0], &verts[1],
                         &verts[2]
                         );
        }                  
        
        if (backbuffer) 
        {
          tlConRender();
          grBufferSwap( swapDelay );
        }
      }
    }

    if (tlKbHit()) {
      char c = (char) tlGetCH();
      
      switch (c) {
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

      case 'h':
      case 'H':
      case '?':
        doHelp();
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
        
      case 'w':
      case 'W':
        gWireframe = !gWireframe;
        break;
        
      case '1':
        lfbOffset -= 1;
        lodBias -=0.25f;
        if ( lodBias <= -8.0f )
          lodBias = -8.0f;
        break;
      case '2':
        lfbOffset += 1;
        lodBias +=0.25f;
        if ( lodBias >= 7.75)
          lodBias = 7.75f;
        break;
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
      y_angle += 1.0f;
      if( y_angle > 360.0f )
        y_angle -= 360.0f;
      y_angle_main += .2f;
      if( y_angle_main > 360.0f )
        y_angle_main -= 360.0f;
    }
    frameCount++;
    if(frameCount < 0)
      frameCount = 0;
    if(frames == frameCount)
      break;

  } /* endwhile (1) */
  grGlideShutdown();
  return 0;
}

