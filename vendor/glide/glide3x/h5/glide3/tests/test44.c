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

#include <glide.h>
#include "tlib.h"


int hwconfig;
static const char *version;
static tlGlideExtension glideext;

static const char name[]    = "test44";
static const char purpose[] = "mipmap modes with TXS file";
static const char usage[]   = "-n <frames> -r <res> -t <texturename> -p <pixel format>";

const char *txname  = "glide.txs";

typedef enum { DISABLE, NEAREST, TRILINEAR } MipMapMode;

const char *texfmt[]={
         "rgb332",                              // TEXFMT_RGB_332 
         "yiq",                                 // TEXFMT_YIQ_422
         "a8",                                  // TEXFMT_A_8
         "i8",                                  // TEXFMT_I_8
         "ai44",                                // TEXFMT_A_44
         "p8",                                  // TEXFMT_P_8
         "p6666",                               // TEXFMT_P_8_6666
         "rsvd2",                               // TEXFMT_RSVD2
         "argb8332",                            // TEXFMT_ARGB_8332
         "ayiq8422",                            // TEXFMT_AYIQ_8422
         "rgb565",                              // TEXFMT_RGB_565
         "argb1555",                            // TEXFMT_ARGB_1555
         "argb4444",                            // TEXFMT_ARGB_4444
         "ai88",                                // TEXFMT_AI_88
         "ap88",                                // TEXFMT_AP_88
         "rsvd4",                               // TEXFMT_RSVD4
         "unknown",                             // skipped 0x10 for some reason in texus.h
         "fxt1",                                // TEXFMT_FXT1
         "argb8888",                            // TEXFMT_ARGB_8888
         "yuyv422",                             // TEXFMT_YUYV_422
         "uyvy422",                             // TEXFMT_UYVY_422
         "ayuv444",                             // TEXFMT_AYUV_444
         "dxt1",                                // TEXFMT_DXT1
         "dxt2",                                // TEXFMT_DXT2
         "dxt3",                                // TEXFMT_DXT3
         "dxt4",                                // TEXFMT_DXT4
         "dxt5",                                // TEXFMT_DXT5
         NULL
};

const char* palfmt[]={
  "ncc0",
  "ncc1",
  "palette",
  "palette 6666",
  NULL
};

int main( int argc, char **argv) {
    char match; 
    char **remArgs;
    int  rv;

    GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
    float                scrWidth   = 640.0f;
    float                scrHeight  = 480.0f;
    int frames                      = -1;

    TlTexture  texture;
    TlVertex3D srcVerts[4];
    float      distance, dDelta;

    MipMapMode mipMapMode;
    FxU32      zrange[2];
    GrContext_t          gc = 0;
    FxU32                pixelformat = 0x0003;

    /* Initialize Glide */
    grGlideInit();
    if(!( hwconfig = tlVoodooType()) )
      {
        grGlideShutdown();
        printf("unknown hardware\n");
        exit(1);
      }

    /* Process Command Line Arguments */
    while((rv = tlGetOpt(argc, argv, "nrtp", &match, &remArgs)) != 0) {
      if ( rv == -1 ) {
        printf( "Unrecognized command line argument\n" );
        printf( "%s %s\n", name, usage );
        printf( "Available resolutions:\n%s\n",
                tlGetResolutionList() );
        return -1;
      }
      switch( match ) 
        {
        case 'n':
          frames = atoi( remArgs[0] );
          break;
        case 'r':
          resolution = tlGetResolutionConstant( remArgs[0], 
                                                &scrWidth, 
                                                &scrHeight );
          break;
        case 't':
          txname=remArgs[0];
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
    if ( frames == -1 ) {
        printf( "Press A Key To Begin Test.\n" );
        tlGetCH();
    }
    
    grSstSelect( 0 );

    tlInitGlideExt(&glideext);

    if (glideext.grSstWinOpen) {
      gc = glideext.grSstWinOpen(tlGethWnd(),
                                 resolution,
                                 GR_REFRESH_60Hz,
                                 GR_COLORFORMAT_ABGR,
                                 GR_ORIGIN_LOWER_LEFT,
                                 pixelformat,
                                 2, 1);
    } else {
      gc = grSstWinOpen(tlGethWnd(),
                        resolution,
                        GR_REFRESH_60Hz,
                        GR_COLORFORMAT_ABGR,
                        GR_ORIGIN_LOWER_LEFT,
                        2, 1 );
    }
    if (!gc) {
      printf("Could not allocate glide fullscreen context.\n");
      goto __errExit;
    }
    
    tlConSet( 0.0f, 0.0f, 1.0f, 0.5f, 
              60, 15, 0xffffff );
    
    /* Set up Render State - Decal Texture - bilinear */
    grGet(GR_ZDEPTH_MIN_MAX, 8, (FxI32 *)zrange);  
    grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);

    grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                    GR_COMBINE_FACTOR_ONE,
                    GR_COMBINE_LOCAL_NONE,
                    GR_COMBINE_OTHER_TEXTURE,
                    FXFALSE );
    grTexFilterMode( GR_TMU0,
                     GR_TEXTUREFILTER_BILINEAR,
                     GR_TEXTUREFILTER_BILINEAR );

    grTexClampMode(GR_TMU0,
                   GR_TEXTURECLAMP_WRAP,
                   GR_TEXTURECLAMP_WRAP);
    
    /* Load texture data into system ram */
    if( tlLoadTexture( txname, 
                       &texture.info, 
                       &texture.tableType, 
                       &texture.tableData ) )
      {
        FxU32 w,h;
        GrAspectRatio_t a;
        a=texture.info.aspectRatioLog2;
        if(a<0)
          {
            h=(1<<texture.info.largeLodLog2);
            w=(1<<(texture.info.largeLodLog2+a));
          }
        else
          {
            w=(1<<texture.info.largeLodLog2);
            h=(1<<(texture.info.largeLodLog2-a));
          }
    
        tlConOutput("Loaded %s\n       %s\n       %s\n       %d mips %dx%d\n\n",
                    txname,
                    texfmt[texture.info.format],
                    (texture.tableType==NO_TABLE)?"not paletted":palfmt[texture.tableType],
                    texture.info.largeLodLog2-texture.info.smallLodLog2+1,
                    w,h);

        /* Download texture data to TMU */
        grTexDownloadMipMap( GR_TMU0,
                             grTexMinAddress( GR_TMU0 ),
                             GR_MIPMAPLEVELMASK_BOTH,
                             &texture.info );
        if ( texture.tableType != NO_TABLE ) {
          grTexDownloadTable( texture.tableType,
                              &texture.tableData );
        }
        
        /* Select Texture As Source of all texturing operations */
        grTexSource( GR_TMU0,
                     grTexMinAddress( GR_TMU0 ),
                     GR_MIPMAPLEVELMASK_BOTH,
                     &texture.info );
        
      }
    else
      {
        printf("Unable to load %s\n",txname);
        tlConOutput("Unable to load %s\n",txname);
      }


    /* Initialize Source 3D data - Rectangle on X/Z Plane 
       Centered about Y Axis

       0--1  Z+
       |  |  |
       2--3   - X+

     */
    srcVerts[0].x = -0.5f, srcVerts[0].y = 0.0f, srcVerts[0].z =  0.5f, srcVerts[0].w = 1.0f;
    srcVerts[1].x =  0.5f, srcVerts[1].y = 0.0f, srcVerts[1].z =  0.5f, srcVerts[1].w = 1.0f;
    srcVerts[2].x = -0.5f, srcVerts[2].y = 0.0f, srcVerts[2].z = -0.5f, srcVerts[2].w = 1.0f;
    srcVerts[3].x =  0.5f, srcVerts[3].y = 0.0f, srcVerts[3].z = -0.5f, srcVerts[3].w = 1.0f;

    srcVerts[0].s = 0.0f, srcVerts[0].t = 0.0f;
    srcVerts[1].s = 1.0f, srcVerts[1].t = 0.0f;
    srcVerts[2].s = 0.0f, srcVerts[2].t = 1.0f;
    srcVerts[3].s = 1.0f, srcVerts[3].t = 1.0f;

#define RED  0x000000ff
#define BLUE 0x00ff0000

#define MAX_DIST 6.5f
#define MIN_DIST 1.0f

    distance = 1.0f;
    dDelta   = 0.01f;

    mipMapMode = DISABLE;

    tlConOutput( "m - change mipmapping mode\n" );
    tlConOutput( "Press any other key to quit\n\n" );
    while( frames-- && tlOkToRender()) {
        GrVertex vtxA, vtxB, vtxC, vtxD;
        TlVertex3D xfVerts[4];
        TlVertex3D prjVerts[4];

        if (hwconfig == TL_VOODOORUSH) {
          tlGetDimsByConst(resolution,
                           &scrWidth, 
                           &scrHeight );
        
          grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
        }

        grBufferClear( 0x00404040, 0, zrange[1] );

        switch( mipMapMode ) {
        case DISABLE:
            grTexMipMapMode( GR_TMU0,
                             GR_MIPMAP_DISABLE,
                             FXFALSE );
            grTexCombine( GR_TMU0,
                          GR_COMBINE_FUNCTION_LOCAL,
                          GR_COMBINE_FACTOR_NONE,
                          GR_COMBINE_FUNCTION_LOCAL,
                          GR_COMBINE_FACTOR_NONE,
                          FXFALSE, FXFALSE );
            tlConOutput( "GR_MIPMAP_DISABLE  \r" );
            break;
        case NEAREST:
            grTexMipMapMode( GR_TMU0,
                             GR_MIPMAP_NEAREST,
                             FXFALSE );
            grTexCombine( GR_TMU0,
                          GR_COMBINE_FUNCTION_LOCAL,
                          GR_COMBINE_FACTOR_NONE,
                          GR_COMBINE_FUNCTION_LOCAL,
                          GR_COMBINE_FACTOR_NONE,
                          FXFALSE, FXFALSE );
            tlConOutput( "GR_MIPMAP_NEAREST  \r" );
            break;
        case TRILINEAR:
            tlConOutput( "GR_MIPMAP_TRILINEAR\r" );
            grTexMipMapMode( GR_TMU0,
                             GR_MIPMAP_NEAREST,
                             FXTRUE );
            grTexCombine( GR_TMU0,
                          GR_COMBINE_FUNCTION_BLEND_LOCAL,
                          GR_COMBINE_FACTOR_LOD_FRACTION,
                          GR_COMBINE_FUNCTION_BLEND_LOCAL,
                          GR_COMBINE_FACTOR_LOD_FRACTION,
                          FXFALSE,
                          FXFALSE );
            break;
        }
            

        /*---- 
          A-B
          |\|
          C-D
          -----*/
        vtxA.oow = 1.0f;
        vtxB = vtxC = vtxD = vtxA;

        distance += dDelta;
        if ( distance > MAX_DIST || 
             distance < MIN_DIST ) { 
            dDelta *= -1.0f;
            distance += dDelta;
        }

        tlSetMatrix( tlIdentity() );
        tlMultMatrix( tlXRotation( -20.0f ) );
        tlMultMatrix( tlTranslation( 0.0f, -0.3f, distance ) );

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

        vtxB.tmuvtx[0].sow = prjVerts[1].s * 255.0f * vtxB.oow;
        vtxB.tmuvtx[0].tow = prjVerts[1].t * 255.0f * vtxB.oow;

        vtxC.tmuvtx[0].sow = prjVerts[2].s * 255.0f * vtxC.oow;
        vtxC.tmuvtx[0].tow = prjVerts[2].t * 255.0f * vtxC.oow;

        vtxD.tmuvtx[0].sow = prjVerts[3].s * 255.0f * vtxD.oow;
        vtxD.tmuvtx[0].tow = prjVerts[3].t * 255.0f * vtxD.oow;


        grDrawTriangle( &vtxA, &vtxB, &vtxD );
        grDrawTriangle( &vtxA, &vtxD, &vtxC );

        if ( mipMapMode == TRILINEAR ) {
            grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ONE,
                                  GR_BLEND_ZERO, GR_BLEND_ZERO );
            grTexCombine( GR_TMU0,
                          GR_COMBINE_FUNCTION_BLEND_LOCAL,
                          GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
                          GR_COMBINE_FUNCTION_BLEND_LOCAL,
                          GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
                          FXFALSE,
                          FXFALSE );
            grDrawTriangle( &vtxA, &vtxB, &vtxD );
            grDrawTriangle( &vtxA, &vtxD, &vtxC );
            grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ZERO,
                                  GR_BLEND_ZERO, GR_BLEND_ZERO );
        }

        tlConRender();
        grBufferSwap( 1 );
        grFinish();
        
        while( tlKbHit() ) {
            switch( tlGetCH() ) {
            case 'm':
            case 'M':
                mipMapMode++;
                mipMapMode%=3;
                break;
            default:
                frames = 0;
                break;
            }
        }
    }
    
 __errExit:    
    grGlideShutdown();
    return 0;
}

