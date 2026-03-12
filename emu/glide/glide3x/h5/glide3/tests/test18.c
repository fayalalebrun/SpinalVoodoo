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
#include <string.h>

#include <glide.h>
#include "tlib.h"


int hwconfig;
static const char *version;

static const char name[]    = "test18";
static const char purpose[] = "alpha texture test";
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

    TlTexture            texture;
    FxU32                zrange[2];
    GrContext_t          gc = 0;

    /* Initialize Glide */
    grGlideInit();
    assert( hwconfig = tlVoodooType() );

    /* Process Command Line Arguments */
    while ((rv = tlGetOpt(argc, argv, "nrd", &match, &remArgs)) != 0) {
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
    gc = grSstWinOpen(tlGethWnd(),
                      resolution,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1 );
    if (!gc) {
      printf("Could not allocate glide fullscreen context.\n");
      goto __errExit;
    }

    tlConSet( 0.0f, 0.0f, 1.0f, 0.5f, 
              60, 15, 0xffffff );
    
    /* Set up Render State - Decal Texture - alpha blend */
    grGet(GR_ZDEPTH_MIN_MAX, 8, (FxI32 *)zrange);  
    grVertexLayout(GR_PARAM_XY,  0, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);

    grTexCombine( GR_TMU0,
                  GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  FXFALSE, FXFALSE );

    /* Load texture data into system ram */
    assert( tlLoadTexture( "alpha.3df", 
                           &texture.info, 
                           &texture.tableType, 
                           &texture.tableData ) );
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

    /* Enable Bilinear Filtering + Mipmapping */
    grTexFilterMode( GR_TMU0,
                     GR_TEXTUREFILTER_BILINEAR,
                     GR_TEXTUREFILTER_BILINEAR );
    grTexMipMapMode( GR_TMU0,
                     GR_MIPMAP_NEAREST,
                     FXFALSE );

    tlConOutput( "Press any key to quit\n\n" );

#define RED  0x000000ff
#define BLUE 0x00ff0000

    while( frames-- && tlOkToRender()) {
        GrVertex vtxA, vtxB, vtxC, vtxD;
        int x,y;

        if (hwconfig == TL_VOODOORUSH) {
          tlGetDimsByConst(resolution,
                           &scrWidth, 
                           &scrHeight );
        
          grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
        }

        grBufferClear( 0, 0, zrange[1] );
        
        /* Draw 10x10 grid of triangles */

        grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                        GR_COMBINE_FACTOR_NONE,
                        GR_COMBINE_LOCAL_CONSTANT,
                        GR_COMBINE_OTHER_NONE,
                        FXFALSE );
        grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL,
                        GR_COMBINE_FACTOR_NONE,
                        GR_COMBINE_LOCAL_CONSTANT,
                        GR_COMBINE_OTHER_NONE,
                        FXFALSE );
        grAlphaBlendFunction( GR_BLEND_ONE,
                              GR_BLEND_ZERO,
                              GR_BLEND_ZERO,
                              GR_BLEND_ZERO );
        for( y = 0; y < 10; y++ ) {
            for( x = 0; x < 10; x++ ) {
                /* 
                   A-D
                   |\|
                   B-C
                 */
                vtxA.x = vtxB.x = tlScaleX( ((float)x)/10.0f );
                vtxA.y = vtxD.y = tlScaleY( ((float)y)/10.0f );
                vtxB.y = vtxC.y = tlScaleY( (((float)y)/10.0f) + 0.1f );
                vtxC.x = vtxD.x = tlScaleX( (((float)x)/10.0f) + 0.1f );
                
                grConstantColorValue( RED );
                grDrawTriangle( &vtxA, &vtxB, &vtxC );
                grConstantColorValue( BLUE );
                grDrawTriangle( &vtxA, &vtxC, &vtxD );
            }
        }

        /*---- 
          A-B
          |\|
          C-D
          -----*/
        vtxA.oow = 1.0f;
        vtxB = vtxC = vtxD = vtxA;

        vtxA.x = vtxC.x = tlScaleX( 0.2f );
        vtxB.x = vtxD.x = tlScaleX( 0.8f );
        vtxA.y = vtxB.y = tlScaleY( 0.2f );
        vtxC.y = vtxD.y = tlScaleY( 0.8f );

        vtxA.tmuvtx[0].sow = vtxC.tmuvtx[0].sow = 0.0f;
        vtxB.tmuvtx[0].sow = vtxD.tmuvtx[0].sow = 255.0f;
        vtxA.tmuvtx[0].tow = vtxB.tmuvtx[0].tow = 0.0f;
        vtxC.tmuvtx[0].tow = vtxD.tmuvtx[0].tow = 255.0f;

        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                        GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE,
                        GR_COMBINE_OTHER_TEXTURE,
                        FXFALSE );
        grAlphaCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                        GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE,
                        GR_COMBINE_OTHER_TEXTURE,
                        FXFALSE );
        grAlphaBlendFunction( GR_BLEND_SRC_ALPHA,
                              GR_BLEND_ONE_MINUS_SRC_ALPHA,
                              GR_BLEND_ZERO,
                              GR_BLEND_ZERO );

        grDrawTriangle( &vtxA, &vtxD, &vtxC );
        grDrawTriangle( &vtxA, &vtxB, &vtxD );

        tlConRender();
        grBufferSwap( 1 );
        grFinish();

        /* grab the frame buffer */
        if (scrgrab) {
          if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight))
            printf( "Cannot open %s\n", filename);
          scrgrab = FXFALSE;
        }
        
        while( tlKbHit() ) {
            switch( tlGetCH() ) {
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
