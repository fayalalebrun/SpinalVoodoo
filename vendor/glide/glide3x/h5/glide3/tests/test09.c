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
#include <assert.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"


int hwconfig;
static const char *version;
static const char *extension;

static const char name[]    = "test09";
static const char purpose[] = "chromakey and chromarange - render a red and blue"
                              " triangle but chromakey one out";
static const char usage[]   = "-n <frames> -r <res> -d <filename>";

int main( int argc, char **argv) {
    char match; 
    char **remArgs;
    int  rv;
    char *extstr;
    GrProc grChromaRangeMode = NULL;
    GrProc grChromaRange = NULL;
    GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
    float                scrWidth   = 640.0f;
    float                scrHeight  = 480.0f;
    int frames                      = -1;
    FxBool               scrgrab = FXFALSE;
    char                 filename[256];
    FxU8                 subframe = 0;
    GrContext_t          gc = 0;

    unsigned long chromaColor;

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
            frames = 2;
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
    
    tlConSet( 0.0f, 0.0f, 1.0f, 1.0f,
              60, 30, 0xffffff );

    /* Set up Render State - gouraud shading + enable chromakey on blue */
    grVertexLayout(GR_PARAM_XY, 0, GR_PARAM_ENABLE);

    grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                    GR_COMBINE_FACTOR_NONE,
                    GR_COMBINE_LOCAL_CONSTANT,
                    GR_COMBINE_OTHER_CONSTANT,
                    FXFALSE );

    extension = grGetString(GR_EXTENSION);

    extstr = strstr(extension, "CHROMARANGE");
    if (extstr) {
      /*
      ** use chromarange 
      ** otherwise, use chromakey
      */
      if (!strncmp(extstr, "CHROMARANGE", 11)) {
        grChromaRangeMode = grGetProcAddress("grChromaRangeModeExt");
        grChromaRange = grGetProcAddress("grChromaRangeExt");
      }
    }

    if (grChromaRangeMode)
      grChromaRangeMode( GR_CHROMARANGE_ENABLE_EXT );
    else
      grChromakeyMode( GR_CHROMAKEY_ENABLE );

#define RED  0x000000ff
#define BLUE 0x00ff0000

    chromaColor = BLUE;
    if (grChromaRange)
      grChromaRange( chromaColor, chromaColor , GR_CHROMARANGE_RGB_ALL_EXT);
    else
      grChromakeyValue( chromaColor );


    tlConOutput( "Press <SPACE> to toggle chromakey color\n" );
    tlConOutput( "Press any other key to quit\n" );
    while( frames-- && tlOkToRender()) {
        GrVertex vtxA, vtxB, vtxC;
        char str[32];

        if (hwconfig == TL_VOODOORUSH) {
          tlGetDimsByConst(resolution,
                           &scrWidth, 
                           &scrHeight );
        
          grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
        }

        if (grChromaRange)
          strcpy(str, "Chromarange ");
        else
          strcpy(str, "Chromakey ");

        switch( chromaColor ) {
        case RED:
          strcat(str, "RED  (this color should be absent)\r");
            break;
        case BLUE:
          strcat(str, "BLUE (this color should be absent)\r");
            break;
        }
        tlConOutput( str );

        grBufferClear( 0x00, 0, 0 );

        vtxA.x = tlScaleX( 0.5f ), vtxA.y = tlScaleY( 0.1f );
        vtxB.x = tlScaleX( 0.8f ), vtxB.y = tlScaleY( 0.9f );
        vtxC.x = tlScaleX( 0.2f ), vtxC.y = tlScaleY( 0.9f );
        
        grConstantColorValue( RED );
        grDrawTriangle( &vtxA, &vtxB, &vtxC );

        grSstOrigin( GR_ORIGIN_LOWER_LEFT );

        grConstantColorValue( BLUE );
        grDrawTriangle( &vtxA, &vtxB, &vtxC );

        grSstOrigin( GR_ORIGIN_UPPER_LEFT );

        tlConRender();
        grBufferSwap( 1 );

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
          if ( chromaColor == RED )
            chromaColor = BLUE;
          else
            chromaColor = RED;
          if (grChromaRange)
            grChromaRange( chromaColor, chromaColor , GR_CHROMARANGE_RGB_ALL_EXT);
          else
            grChromakeyValue( chromaColor );
          
          subframe++;
          /* scrgrab = FXFALSE; */
        }

        while( tlKbHit() ) {
            switch( tlGetCH() ) {
            case ' ':
                if ( chromaColor == RED )
                    chromaColor = BLUE;
                else
                    chromaColor = RED;
                if (grChromaRange)
                  grChromaRange( chromaColor, chromaColor , GR_CHROMARANGE_RGB_ALL_EXT);
                else
                  grChromakeyValue( chromaColor );
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






