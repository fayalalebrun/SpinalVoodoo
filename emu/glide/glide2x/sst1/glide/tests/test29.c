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


GrHwConfiguration hwconfig;
static char version[80];

static const char name[]    = "test29";
static const char purpose[] = "oow diff hint";
static const char usage[]   = "-n <frames> -r <res>";

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
                      GR_ORIGIN_LOWER_LEFT,
                      2, 1 ) );
    
    tlConSet( 0.0f, 0.0f, 1.0f, 0.5f, 
              60, 15, 0xffffff );
    
    /* Set up Render State - Decal Texture - bilinear */
    grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                    GR_COMBINE_FACTOR_ONE,
                    GR_COMBINE_LOCAL_NONE,
                    GR_COMBINE_OTHER_TEXTURE,
                    FXFALSE );
    grTexFilterMode( GR_TMU0,
                     GR_TEXTUREFILTER_POINT_SAMPLED,
                     GR_TEXTUREFILTER_POINT_SAMPLED );
    
    /* Load texture data into system ram */
    assert( tlLoadTexture( "decal1.3df", 
                           &texture.info, 
                           &texture.tableType, 
                           &texture.tableData ) );
    /* Download texture data to TMU */
    grTexDownloadMipMap( GR_TMU0,
                         grTexMinAddress( GR_TMU0 ),
                         GR_MIPMAPLEVELMASK_BOTH,
                         &texture.info );
    if ( texture.tableType != NO_TABLE ) {
        grTexDownloadTable( GR_TMU0,
                            texture.tableType,
                            &texture.tableData );
    }

    /* Select Texture As Source of all texturing operations */
    grTexSource( GR_TMU0,
                 grTexMinAddress( GR_TMU0 ),
                 GR_MIPMAPLEVELMASK_BOTH,
                 &texture.info );

    grTexMipMapMode( GR_TMU0,
                     GR_MIPMAP_NEAREST,
                     FXFALSE );
    grTexCombine( GR_TMU0,
                  GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  FXFALSE, FXFALSE );

    /* enable oow depth buffering */
    grDepthBufferMode( GR_DEPTHBUFFER_WBUFFER );
    grDepthBufferFunction( GR_CMP_LESS );
    grDepthMask( FXTRUE );

    /* Set hint to separate w for depth and tex */
    grHints( GR_HINT_STWHINT, GR_STWHINT_W_DIFF_TMU0 );
    /* oow for the texture coords will be set to one
       in effect disabling perspective correction */

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

    tlConOutput( "Press any key to quit\n\n" );
    while( frames-- && tlOkToRender()) {
        GrVertex vtxA, vtxB, vtxC, vtxD;
        TlVertex3D xfVerts[4];
        TlVertex3D prjVerts[4];

        tlGetDimsByConst(resolution,
                         &scrWidth, 
                         &scrHeight );
        
        grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);

        grBufferClear( 0x00404040, 0, GR_WDEPTHVALUE_FARTHEST );


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

        vtxA.tmuvtx[0].oow = 1.0f;
        vtxA.tmuvtx[0].sow = prjVerts[0].s * 255.0f * vtxA.tmuvtx[0].oow;
        vtxA.tmuvtx[0].tow = prjVerts[0].t * 255.0f * vtxA.tmuvtx[0].oow;

        vtxB.tmuvtx[0].oow = 1.0f;
        vtxB.tmuvtx[0].sow = prjVerts[1].s * 255.0f * vtxB.tmuvtx[0].oow;
        vtxB.tmuvtx[0].tow = prjVerts[1].t * 255.0f * vtxB.tmuvtx[0].oow;

        vtxC.tmuvtx[0].oow = 1.0f;
        vtxC.tmuvtx[0].sow = prjVerts[2].s * 255.0f * vtxC.tmuvtx[0].oow;
        vtxC.tmuvtx[0].tow = prjVerts[2].t * 255.0f * vtxC.tmuvtx[0].oow;

        vtxD.tmuvtx[0].oow = 1.0f;
        vtxD.tmuvtx[0].sow = prjVerts[3].s * 255.0f * vtxD.tmuvtx[0].oow;
        vtxD.tmuvtx[0].tow = prjVerts[3].t * 255.0f * vtxD.tmuvtx[0].oow;

        grDrawTriangle( &vtxA, &vtxB, &vtxD );
        grDrawTriangle( &vtxA, &vtxD, &vtxC );

        tlConRender();
        grBufferSwap( 1 );
        grSstIdle();
        
        while( tlKbHit() ) {
            switch( tlGetCH() ) {
            default:
                frames = 0;
                break;
            }
        }
    }
    
    grGlideShutdown();
    return 0;
}



