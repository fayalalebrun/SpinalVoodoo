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

#include <glide.h>
#include "tlib.h"


int hwconfig;
static const char *version;

static const char name[]    = "test21";
static const char purpose[] = "texture compositing";
static const char usage[]   = "-n <frames> -r <res>";

typedef enum { LIGHTMAP, SPECULAR, DETAIL } Mode;

int main( int argc, char **argv) {
    char match; 
    char **remArgs;
    int  rv;

    GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
    float                scrWidth   = 640.0f;
    float                scrHeight  = 480.0f;
    int frames                      = -1;

    TlTexture  baseTexture;
    unsigned long baseTextureAddr;
    TlTexture  lightTexture;
    unsigned long lightTextureAddr;
    TlTexture  detailTexture;
    unsigned long detailTextureAddr;

    TlVertex3D srcVerts[4];
    float      distance, dDelta;

    Mode mode;
    FxU32 zrange[2];
    GrContext_t          gc = 0;

    /* Initialize Glide */
    grGlideInit();
    assert( hwconfig = tlVoodooType() );

    /* Process Command Line Arguments */
    while ((rv = tlGetOpt(argc, argv, "nr", &match, &remArgs)) != 0) {
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
    gc = grSstWinOpen(tlGethWnd(),
                      resolution,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_LOWER_LEFT,
                      2, 1 );
    if (!gc) {
      printf("Could not allocate glide fullscreen context.\n");
      goto __errExit;
    }
    
    tlConSet( 0.0f, 0.0f, 1.0f, 0.5f, 
              60, 15, 0xffffff );
    
    /* Set up Render State - decal - bilinear - nearest mipmapping  */
    grGet(GR_ZDEPTH_MIN_MAX, 8, (FxI32 *)zrange);  
    grVertexLayout(GR_PARAM_XY,  0, GR_PARAM_ENABLE);
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
    
    /* Load texture data into system ram */
    assert( tlLoadTexture( "decal1.3df", 
                           &baseTexture.info, 
                           &baseTexture.tableType, 
                           &baseTexture.tableData ) );
    assert( tlLoadTexture( "light.3df", 
                           &lightTexture.info, 
                           &lightTexture.tableType, 
                           &lightTexture.tableData ) );
    assert( tlLoadTexture( "lava.3df", 
                           &detailTexture.info, 
                           &detailTexture.tableType, 
                           &detailTexture.tableData ) );

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

    lightTextureAddr = 
        baseTextureAddr +
        grTexTextureMemRequired( GR_MIPMAPLEVELMASK_BOTH,
                                 &baseTexture.info );
    grTexDownloadMipMap( GR_TMU0,
                         lightTextureAddr,
                         GR_MIPMAPLEVELMASK_BOTH,
                         &lightTexture.info );
    if ( lightTexture.tableType != NO_TABLE ) {
        grTexDownloadTable( lightTexture.tableType,
                            &lightTexture.tableData );
    }

    detailTextureAddr = 
        lightTextureAddr +
        grTexTextureMemRequired( GR_MIPMAPLEVELMASK_BOTH,
                                 &lightTexture.info );
    grTexDownloadMipMap( GR_TMU0,
                         detailTextureAddr,
                         GR_MIPMAPLEVELMASK_BOTH,
                         &detailTexture.info );
    if ( detailTexture.tableType != NO_TABLE ) {
        grTexDownloadTable( detailTexture.tableType,
                            &detailTexture.tableData );
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

#define MAX_DIST 2.5f
#define MIN_DIST 1.0f

    distance = 1.0f;
    dDelta   = 0.01f;

    mode = LIGHTMAP;

    tlConOutput( "m - change texture compositing mode\n" );
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

        /* 3D Transformations */
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

        /* Render First Pass */
        switch( mode ) {
        case LIGHTMAP:
        case SPECULAR:
            grTexCombine( GR_TMU0,
                          GR_COMBINE_FUNCTION_LOCAL,
                          GR_COMBINE_FACTOR_NONE,
                          GR_COMBINE_FUNCTION_LOCAL,
                          GR_COMBINE_FACTOR_NONE,
                          FXFALSE,
                          FXFALSE );
            break;
        case DETAIL:
            grTexCombine( GR_TMU0,
                          GR_COMBINE_FUNCTION_BLEND_LOCAL,
                          GR_COMBINE_FACTOR_DETAIL_FACTOR,
                          GR_COMBINE_FUNCTION_LOCAL,
                          GR_COMBINE_FACTOR_NONE,
                          FXFALSE,
                          FXFALSE );
            grTexDetailControl( GR_TMU0, 2, 7, 1.0f );
            break;
        }
        grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ZERO,
                              GR_BLEND_ONE, GR_BLEND_ZERO );
        grTexSource( GR_TMU0,
                     baseTextureAddr,
                     GR_MIPMAPLEVELMASK_BOTH,
                     &baseTexture.info );

        grDrawTriangle( &vtxA, &vtxB, &vtxD );
        grDrawTriangle( &vtxA, &vtxD, &vtxC );

        /* Render Second Pass */
        switch( mode ) {
        case LIGHTMAP:
            grAlphaBlendFunction( GR_BLEND_DST_COLOR, GR_BLEND_ZERO,
                                  GR_BLEND_ZERO, GR_BLEND_ZERO );
            grTexSource( GR_TMU0,
                         lightTextureAddr,
                         GR_MIPMAPLEVELMASK_BOTH,
                         &lightTexture.info );
            tlConOutput( "LIGHTMAP - TEXTURE MODULATE  \r" );
            break;
        case DETAIL:
            grTexCombine( GR_TMU0,
                          GR_COMBINE_FUNCTION_BLEND_LOCAL,
                          GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR,
                          GR_COMBINE_FUNCTION_LOCAL,
                          GR_COMBINE_FACTOR_NONE,
                          FXFALSE,
                          FXFALSE );
            grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ONE,
                                  GR_BLEND_ZERO, GR_BLEND_ZERO );
            grTexSource( GR_TMU0,
                         detailTextureAddr,
                         GR_MIPMAPLEVELMASK_BOTH,
                         &detailTexture.info );
            tlConOutput( "DETAIL   - BLEND ON LOD      \r" );
            break;
        case SPECULAR:
            grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ONE,
                                  GR_BLEND_ZERO, GR_BLEND_ZERO );
            grTexSource( GR_TMU0,
                         lightTextureAddr,
                         GR_MIPMAPLEVELMASK_BOTH,
                         &lightTexture.info );
            tlConOutput( "SPECULAR - TEXTURE ACCUMULATE\r" );
            break;
        }

        grDrawTriangle( &vtxA, &vtxB, &vtxD );
        grDrawTriangle( &vtxA, &vtxD, &vtxC );

        tlConRender();
        grBufferSwap( 1 );
        grFinish();
        
        while( tlKbHit() ) {
            switch( tlGetCH() ) {
            case 'm':
            case 'M':
                mode++;
                mode%=3;
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



