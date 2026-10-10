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


GrHwConfiguration hwconfig;
static char version[80];

static const char name[]    = "test17";
static const char purpose[] = "texturing example, decal, rgb lit, white lit, flat lit";
static const char usage[]   = "-n <frames> -r <res> -d <filename>";

typedef enum { DECAL, FLATLIT, RGBLIT, WHITELIT, SPECALPHA } TextureMode;
const char *textureModeNames[] = {
    "DECAL TEXTURE                   ",
    "FLAT SHADING * TEXTURE          ",
    "ITERATED RGB * TEXTURE          ", 
    "INTENSITY LIGHTING * TEXTURE    ",
    "(ITRGB * TEXTURE)+WHITE SPECULAR"
};

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
    FxU8                 subframe = 0;

    TlTexture texture;

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
            frames = 5;
            strcpy(filename, remArgs[0]);
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
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1 ) );
    
    tlConSet( 0.0f, 0.0f, 1.0f, 0.5f, 
              60, 15, 0xffffff );
    
    /* Set up Render State - Decal Texture - color combine set in render loop */
    grTexCombine( GR_TMU0,
                  GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_NONE,
                  GR_COMBINE_FACTOR_NONE,
                  FXFALSE, FXFALSE );

    /* Load texture data into system ram */
    assert( tlLoadTexture( "miro.3df", 
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

    /* Enable Bilinear Filtering + Mipmapping */
    grTexFilterMode( GR_TMU0,
                     GR_TEXTUREFILTER_BILINEAR,
                     GR_TEXTUREFILTER_BILINEAR );
    grTexMipMapMode( GR_TMU0,
                     GR_MIPMAP_NEAREST,
                     FXFALSE );

    tlConOutput( "m - change lighting mode\n" );
    tlConOutput( "Press any other key to quit\n\n" );
    while( frames-- && tlOkToRender()) {
        GrVertex vtxA, vtxB, vtxC, vtxD;
        static TextureMode textureMode;

        if (hwconfig.SSTs[0].type == GR_SSTTYPE_SST96) {
          tlGetDimsByConst(resolution,
                           &scrWidth, 
                           &scrHeight );
        
          grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
        }

        grBufferClear( 0, 0, GR_ZDEPTHVALUE_FARTHEST );

        tlConOutput( "Current Texture Mode: %s\r",
                     textureModeNames[textureMode] );

        switch( textureMode ) {
        case DECAL:
            grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                            GR_COMBINE_FACTOR_ONE,
                            GR_COMBINE_LOCAL_NONE,
                            GR_COMBINE_OTHER_TEXTURE,
                            FXFALSE );
            break;
        case FLATLIT:
            grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                            GR_COMBINE_FACTOR_LOCAL,
                            GR_COMBINE_LOCAL_CONSTANT,
                            GR_COMBINE_OTHER_TEXTURE,
                            FXFALSE );
            break;
        case RGBLIT:
            grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                            GR_COMBINE_FACTOR_LOCAL,
                            GR_COMBINE_LOCAL_ITERATED,
                            GR_COMBINE_OTHER_TEXTURE,
                            FXFALSE );
            break;
        case WHITELIT:
            grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                            GR_COMBINE_FACTOR_LOCAL_ALPHA,
                            GR_COMBINE_LOCAL_NONE,
                            GR_COMBINE_OTHER_TEXTURE,
                            FXFALSE );
            grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL,
                            GR_COMBINE_FACTOR_NONE,
                            GR_COMBINE_LOCAL_ITERATED,
                            GR_COMBINE_OTHER_TEXTURE,
                            FXFALSE );
            break;
        case SPECALPHA:
            grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA,
                            GR_COMBINE_FACTOR_LOCAL,
                            GR_COMBINE_LOCAL_ITERATED,
                            GR_COMBINE_OTHER_TEXTURE,
                            FXFALSE );
            grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL,
                            GR_COMBINE_FACTOR_NONE,
                            GR_COMBINE_LOCAL_ITERATED,
                            GR_COMBINE_OTHER_TEXTURE,
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

        vtxA.x = vtxC.x = tlScaleX( 0.2f );
        vtxB.x = vtxD.x = tlScaleX( 0.8f );
        vtxA.y = vtxB.y = tlScaleY( 0.2f );
        vtxC.y = vtxD.y = tlScaleY( 0.8f );

        vtxA.tmuvtx[0].sow = vtxC.tmuvtx[0].sow = 0.0f;
        vtxB.tmuvtx[0].sow = vtxD.tmuvtx[0].sow = 255.0f;
        vtxA.tmuvtx[0].tow = vtxB.tmuvtx[0].tow = 0.0f;
        vtxC.tmuvtx[0].tow = vtxD.tmuvtx[0].tow = 255.0f;

        vtxA.r = 255.0f, vtxA.g =   0.0f, vtxA.b =   0.0f, vtxA.a = 255.0f;
        vtxB.r =   0.0f, vtxB.g = 255.0f, vtxB.b =   0.0f, vtxB.a = 128.0f;
        vtxC.r =   0.0f, vtxC.g =   0.0f, vtxC.b = 255.0f, vtxC.a = 128.0f;
        vtxD.r =   0.0f, vtxD.g =   0.0f, vtxD.b =   0.0f, vtxD.a =   0.0f;

        grDrawTriangle( &vtxA, &vtxD, &vtxC );
        grDrawTriangle( &vtxA, &vtxB, &vtxD );

        grConstantColorValue( 0x00ff0000 );

        tlConRender();
        grBufferSwap( 1 );
        grSstIdle();

        /* grab the frame buffer */
        if (scrgrab) {
          char fname[256], tmp[32];
          FxU16 cnt;

          cnt = strcspn(filename, ".");
          strncpy(fname, filename, cnt);
          fname[cnt] = 0;
          sprintf(tmp,"_%d", subframe);
          strcat(fname, tmp);
          strcat(fname, filename+cnt);
          if (!tlScreenDump(fname, (FxU16)scrWidth, (FxU16)scrHeight))
            printf( "Cannot open %s\n", filename);

          /* cycle through all mode */
          textureMode++;
          if ( textureMode > SPECALPHA ) textureMode = DECAL;

          subframe++;
        }
        
        while( tlKbHit() ) {
            switch( tlGetCH() ) {
            case 'm':
            case 'M':
                textureMode++;
                if ( textureMode > SPECALPHA ) textureMode = DECAL;
                break;
            default:
                frames = 0;
                break;
            }
        }
    }
    
    grGlideShutdown();
    return 0;
}
