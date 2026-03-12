/*
** Insert new header here
**
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"

typedef struct
{
  float x, y;         /* X and Y in screen space */
  float ooz;          /* 65535/Z (used for Z-buffering) */
  float oow;          /* 1/W (used for W-buffering, texturing) */
  float r, g, b, a;   /* R, G, B, A [0..255.0] */
  float z;            /* Z is ignored */
  GrTmuVertex  tmuvtx[GLIDE_NUM_TMU];
  float fog;          /* fog coordinate */
} FogVertex;

int hwconfig;
static const char *version;
static const char *extension;

static const char name[]    = "test34";
static const char purpose[] = "fogging with table/fogcoord";
static const char usage[]   = "-n <frames> -r <res> -d <filename>";

int main( int argc, char **argv) {
    char match; 
    char **remArgs;
    int  rv;

    GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
    float                scrWidth   = 640.0f;
    float                scrHeight  = 480.0f;
    int frames                      = -1;
    FxBool               scrgrab = FXFALSE;
    char                 filename[256];
    FxI32                ftsize = 0;
    GrFog_t              *fogtable = NULL;

    TlTexture  baseTexture;
    FxU32 baseTextureAddr;
    FogVertex vtxA, vtxB, vtxC, vtxD;
    TlVertex3D srcVerts[4];
    TlVertex3D xfVerts[4];
    TlVertex3D prjVerts[4];
    FxBool fogCoord = FXFALSE, fogext = FXFALSE;
    char *extstr;
    float dist = 0.f;
    float delta = 0.5f;

    /* Initialize Glide */
    grGlideInit();
    assert( hwconfig = tlVoodooType() );

    /* Process Command Line Arguments */
    while((rv = tlGetOpt(argc, argv, "nrd", &match, &remArgs)) != 0) {
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
        case 'd':
            scrgrab = FXTRUE;
            frames = 1;
            strcpy(filename, remArgs[0]);
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
                         GR_ORIGIN_UPPER_LEFT,
                         2, 1 ) );
    
    tlConSet( 0.0f, 0.0f, 1.0f, 1.0f, 
              60, 30, 0xffffff );

    /* Set up Render State - gouraud shading + fog */

    extension = grGetString(GR_EXTENSION);
    extstr = strstr(extension, "FOGCOORD");
    if (!extstr) {
      printf( "FOGCOORD is not supported in %s\n", grGetString(GR_HARDWARE) );
      grGlideShutdown();
      return -1;
    }
    if (!strncmp(extstr, "FOGCOORD", 8)) {
      fogext = FXTRUE;
    }

    grGet(GR_FOG_TABLE_ENTRIES, 4, &ftsize);
    fogtable = malloc(sizeof(GrFog_t)*ftsize);
    assert(fogtable);

    grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);

    grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                    GR_COMBINE_FACTOR_ONE,
                    GR_COMBINE_LOCAL_NONE,
                    GR_COMBINE_OTHER_TEXTURE,
                    FXFALSE );
    grTexMipMapMode( GR_TMU0,
                     GR_MIPMAP_NEAREST,
                     FXFALSE );
    grTexFilterMode( GR_TMU0,
                     GR_TEXTUREFILTER_BILINEAR,
                     GR_TEXTUREFILTER_BILINEAR );

    grFogColorValue( 0xff00ff00 );
    guFogGenerateExp( fogtable, .01f );
    grFogTable( fogtable );

    assert( tlLoadTexture( "miro.3df", 
                           &baseTexture.info, 
                           &baseTexture.tableType, 
                           &baseTexture.tableData ) );
    /* Download texture data to TMU */
    baseTextureAddr = grTexMinAddress( GR_TMU0 );
    grTexDownloadMipMap( GR_TMU0,
                         baseTextureAddr,
                         GR_MIPMAPLEVELMASK_BOTH,
                         &baseTexture.info );
    if ( baseTexture.tableType != NO_TABLE ) {
        grTexDownloadTable( baseTexture.tableType,
                            &baseTexture.tableData );
    }
    grTexCombine( GR_TMU0,
                  GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  FXFALSE,
                  FXFALSE );
    grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ZERO,
                          GR_BLEND_ONE, GR_BLEND_ZERO );
    grTexSource( GR_TMU0,
                 baseTextureAddr,
                 GR_MIPMAPLEVELMASK_BOTH,
                 &baseTexture.info );
    grTexClampMode( GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
    
    tlConOutput( "Press a key to quit\n" );
    tlConOutput( "Press F to toggle fog mode\n" );
    while( frames-- && tlOkToRender()) {
      if (fogCoord && fogext) {
        grFogMode( GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT );
        grVertexLayout(GR_PARAM_FOG_EXT, sizeof(GrVertex), GR_PARAM_ENABLE);
        tlConOutput( "GR_FOG_WITH_TABLE_ON_FOGCOORD\r" );
      }
      else {
        grFogMode( GR_FOG_WITH_TABLE_ON_W );
        grVertexLayout(GR_PARAM_FOG_EXT, sizeof(GrVertex), GR_PARAM_DISABLE);
        if (fogCoord && !fogext)
          tlConOutput( "FOG_COORD IS NOT SUPPORTED    \r");
        else
          tlConOutput( "GR_FOG_WITH_TABLE_ON_W        \r" );
      }
      
      {
        srcVerts[0].x = -10.0f, srcVerts[0].y = 0.0f, srcVerts[0].z =  200.0f, srcVerts[0].w = 1.0f;
        srcVerts[1].x =  10.0f, srcVerts[1].y = 0.0f, srcVerts[1].z =  200.0f, srcVerts[1].w = 1.0f;
        srcVerts[2].x = -10.0f, srcVerts[2].y = 0.0f, srcVerts[2].z = -40.0f, srcVerts[2].w = 1.0f;
        srcVerts[3].x =  10.0f, srcVerts[3].y = 0.0f, srcVerts[3].z = -40.0f, srcVerts[3].w = 1.0f;
        
        srcVerts[0].s = 0.0f, srcVerts[0].t = 0.0f;
        srcVerts[1].s = 1.0f, srcVerts[1].t = 0.0f;
        srcVerts[2].s = 0.0f, srcVerts[2].t = 1.0f;
        srcVerts[3].s = 1.0f, srcVerts[3].t = 1.0f;
        
        tlSetMatrix( tlIdentity() );
        
        tlMultMatrix( tlXRotation( 15.f ) );          
        
        if (dist <= 0.f)
          delta = 0.5f;
        if (dist >= 30.f)
          delta = -0.5f;
        dist += delta;

        tlMultMatrix( tlTranslation( 0.0f, 0.0f, 50.f + dist ) );
        
        tlTransformVertices( xfVerts, srcVerts, 4 );
        tlProjectVertices( prjVerts, xfVerts, 4 );
        
        vtxA.x = tlScaleX( prjVerts[0].x );
        vtxA.y = tlScaleY( prjVerts[0].y );
        vtxA.oow = 1.0f / prjVerts[0].w;
        
        vtxB.x = tlScaleX( prjVerts[1].x );
        vtxB.y = tlScaleY( prjVerts[1].y ); 
        vtxB.oow = 1.0f / prjVerts[1].w;
        
        vtxC.x = tlScaleX( prjVerts[2].x );
        vtxC.y = tlScaleY( prjVerts[2].y );
        vtxC.oow = 1.0f / prjVerts[2].w;
        
        vtxD.x = tlScaleX( prjVerts[3].x ); 
        vtxD.y = tlScaleY( prjVerts[3].y );
        vtxD.oow = 1.0f / prjVerts[3].w;
        
        vtxA.tmuvtx[0].sow = prjVerts[0].s * 255.0f * vtxA.oow;
        vtxA.tmuvtx[0].tow = prjVerts[0].t * 255.0f * vtxA.oow;
        vtxA.fog = 1.f / 30.f;
        
        vtxB.tmuvtx[0].sow = prjVerts[1].s * 255.0f * vtxB.oow;
        vtxB.tmuvtx[0].tow = prjVerts[1].t * 255.0f * vtxB.oow;
        vtxB.fog = 1.f / 15.f;
        
        vtxC.tmuvtx[0].sow = prjVerts[2].s * 255.0f * vtxC.oow;
        vtxC.tmuvtx[0].tow = prjVerts[2].t * 255.0f * vtxC.oow;
        vtxC.fog = 1.f / 300.f;
        
        vtxD.tmuvtx[0].sow = prjVerts[3].s * 255.0f * vtxD.oow;
        vtxD.tmuvtx[0].tow = prjVerts[3].t * 255.0f * vtxD.oow;
        vtxD.fog = 1.f / 15.f;
      }

      if (hwconfig == TL_VOODOORUSH) {
        tlGetDimsByConst(resolution,
                         &scrWidth, 
                         &scrHeight );
        
        grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
      }

      grBufferClear( 0x00, 0, 0 );
      
      grDrawTriangle( &vtxA, &vtxB, &vtxD );
      grDrawTriangle( &vtxA, &vtxD, &vtxC );
      
      if (fogCoord && fogext) {
        grVertexLayout(GR_PARAM_FOG_EXT, sizeof(GrVertex), GR_PARAM_DISABLE);
      }

      tlConRender();
      grBufferSwap( 1 );
      
      /* grab the frame buffer */
      if (scrgrab) {
        if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight))
          printf( "Cannot open %s\n", filename);
        scrgrab = FXFALSE;
      }
      
      while( tlKbHit() ) {
        switch( tlGetCH() ) {
        case 'f':
        case 'F':
          fogCoord = !fogCoord;
          break;
        default:
          frames = 0;
          break;
        }
      }
    }

    grGlideShutdown();
    free(fogtable);
    return 0;
}





