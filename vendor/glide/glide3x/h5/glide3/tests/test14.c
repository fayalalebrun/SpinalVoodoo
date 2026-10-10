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

static const char name[]    = "test14";
static const char purpose[] = "depth bias test: vary depth bias \n"
                               "over time with two interpenetrating triangles";
static const char usage[]   = "-n <frames> -r <res>";

int main( int argc, char **argv) {
    char match; 
    char **remArgs;
    int  rv;

    GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
    float                scrWidth   = 640.0f;
    float                scrHeight  = 480.0f;
    int frames                      = -1;

    FxI32 zBias;
    FxI32 zDelta;
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
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1 );
    if (!gc) {
      printf("Could not allocate glide fullscreen context.\n");
      goto __errExit;
    }
    
    tlConSet( 0.0f, 0.0f, 1.0f, 0.5f, 
              60, 15, 0xffffff );

    /* Set up Render State - iterated alpha + z buffering */
    grGet(GR_ZDEPTH_MIN_MAX, 8, (FxI32 *)zrange);  
    grVertexLayout(GR_PARAM_Z,   GR_VERTEX_OOZ_OFFSET << 2, GR_PARAM_ENABLE);
    
    grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                    GR_COMBINE_FACTOR_NONE,
                    GR_COMBINE_LOCAL_CONSTANT,
                    GR_COMBINE_OTHER_NONE,
                    FXFALSE );
    grDepthBufferMode( GR_DEPTHBUFFER_ZBUFFER );
    grDepthBufferFunction( GR_CMP_GREATER );
    grDepthMask( FXTRUE );

    zBias  = 0;
    zDelta = 10;

#define RED   0x000000ff
#define GREEN 0x0000ff00
#define BLUE  0x00ff0000
#define GRAY  0x00808080

#define MAX_ZBIAS  500
#define MIN_ZBIAS -500

    tlConOutput( "Press any key to quit\n" );
    while( frames-- && tlOkToRender()) {
        GrVertex vtxA, vtxB, vtxC;
        float zDist;

        if (hwconfig == TL_VOODOORUSH) {
          tlGetDimsByConst(resolution,
                           &scrWidth, 
                           &scrHeight );
        
          grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
        }

        if ( ( zBias > MAX_ZBIAS ) || 
             ( zBias < MIN_ZBIAS ) ) {
            zDelta = -zDelta;
        }
        zBias += zDelta;

        tlConOutput( "DepthBiasLevel: %+.03d  \r", zBias );

        grBufferClear(0x00, 0, zrange[1]);

        vtxA.x = tlScaleX( 0.25f ), vtxA.y = tlScaleY( 0.21f );
        vtxB.x = tlScaleX( 0.75f ), vtxB.y = tlScaleY( 0.21f );
        vtxC.x = tlScaleX( 0.5f  ), vtxC.y = tlScaleY( 0.79f );
        zDist = 10.0f;
        vtxA.ooz = vtxB.ooz = vtxC.ooz = ( 65535.0f / zDist );

        grConstantColorValue( GRAY );

        grDepthBiasLevel( zBias );
        grDrawTriangle( &vtxA, &vtxB, &vtxC );

        vtxA.x = tlScaleX( 0.86f ), vtxA.y = tlScaleY( 0.21f );
        vtxB.x = tlScaleX( 0.86f ), vtxB.y = tlScaleY( 0.79f );
        vtxC.x = tlScaleX( 0.14f ), vtxC.y = tlScaleY( 0.5f );
        zDist = 12.5f;
        vtxA.ooz = vtxB.ooz = ( 65535.0f / zDist );
        zDist = 7.5f;
        vtxC.ooz = ( 65535.0f / zDist );

        grConstantColorValue( GREEN );

        grDepthBiasLevel( 0 );
        grDrawTriangle( &vtxA, &vtxB, &vtxC );

        tlConRender();
        grBufferSwap( 1 );
        
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
