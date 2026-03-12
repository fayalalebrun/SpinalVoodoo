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
#include <math.h>

#include <glide.h>
#include "tlib.h"

#define VERTEX_ARRAY_SIZE     16
#define PI                    3.1415f

int hwconfig;
static const char *version;

static const char name[]    = "test33";
static const char purpose[] = "draws gouraud shaded triangle strip and fan with grDrawVertexArray(GR_TRIANGLE_STRIP(FAN)_CONTINUE";
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
    FxU32                wrange[2];
    GrVertex             stripdata[VERTEX_ARRAY_SIZE];
    GrVertex             fandata[VERTEX_ARRAY_SIZE];
    FxFloat              *stripptr[VERTEX_ARRAY_SIZE];
    FxFloat              *fanptr[VERTEX_ARRAY_SIZE];
    FxU32                i, j, k, v;
    float                tSize = 50.f;
    GrContext_t          gc = 0;

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

    for (v = 0; v < VERTEX_ARRAY_SIZE; v++) {
      stripptr[v] = &stripdata[v].x;
      stripdata[v].x = (v >> 1) * tSize;
      stripdata[v].y = (v & 1) ? 100.f + tSize : 100.f;
      stripdata[v].r = v * (255.f / VERTEX_ARRAY_SIZE);
      stripdata[v].g = 0.f;
      stripdata[v].b = 255.f - (v * 255.f / VERTEX_ARRAY_SIZE);
    }
    v = 0;
    fanptr[v] = &fandata[v].x;
    fandata[v].x = tlScaleX(0.5f);
    fandata[v].y = tlScaleY(0.5f);
    fandata[v].r = 255.f;
    fandata[v].g = 0.f;
    fandata[v].b = 255.f;
    
    for (v = 1; v < VERTEX_ARRAY_SIZE; v++) {
      float theta;
      float x, y;
      float area = tSize * tSize / 2;
      float r = (float)sqrt((VERTEX_ARRAY_SIZE - 2) * area / PI);
      theta = PI - ((((float) (v-1) / (float) (VERTEX_ARRAY_SIZE - 2))) * 2.f * PI);
      x = (float) cos(theta) * r;
      y = (float) sin(theta) * r;
      fanptr[v] = &fandata[v].x;
      fandata[v].x = tlScaleX(0.5f) + x;
      fandata[v].y = tlScaleY(0.5f) + y;
      fandata[v].r = v * (255.f / VERTEX_ARRAY_SIZE);
      fandata[v].g = 0.f;
      fandata[v].b = 255.f - (v * 255.f / VERTEX_ARRAY_SIZE);
    }
    
    tlConSet( 0.0f, 0.0f, 1.0f, 1.0f, 
              60, 30, 0xffffff );

    /* Set up Render State - gouraud shading */

    grGet(GR_WDEPTH_MIN_MAX, 8, (FxI32 *)wrange);  

    grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);

    grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                    GR_COMBINE_FACTOR_NONE,
                    GR_COMBINE_LOCAL_ITERATED,
                    GR_COMBINE_OTHER_NONE,
                    FXFALSE );
    
    tlConOutput( "Press a key to quit\n" );

    for (i = 1; i <= 6; i++) {
      for (j = 1; j <= 5; j++) {
        for (k = 1; k <= 5; k++) {
          if (hwconfig == TL_VOODOORUSH) {
            tlGetDimsByConst(resolution,
                             &scrWidth, 
                             &scrHeight );
            
            grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
          }
          tlConOutput( "i  %d, j  %d, k  %d \r", i, j, k );
          
          grBufferClear(0x00000000, 0, wrange[1]);

          grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
          grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);

          grDrawVertexArray(GR_TRIANGLE_STRIP, i, stripptr);
          grDrawVertexArray(GR_TRIANGLE_STRIP_CONTINUE, j, &stripptr[i]);
          grDrawVertexArray(GR_TRIANGLE_STRIP_CONTINUE, k, &stripptr[i+j]);
          
          grDrawVertexArray(GR_TRIANGLE_FAN, i, fanptr);
          grDrawVertexArray(GR_TRIANGLE_FAN_CONTINUE, j, fanptr+i);
          grDrawVertexArray(GR_TRIANGLE_FAN_CONTINUE, k, fanptr+i+j);
          if ( frames == -1 )
            tlSleep( 1 );

          tlConRender();
          grBufferSwap( 1 );
          
          /* grab the frame buffer */
          if (scrgrab) {
            if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight))
              printf( "Cannot open %s\n", filename);
            scrgrab = FXFALSE;
          }
          if ( tlKbHit() ) goto __errExit;
        }
      }
    }
    
    tlSleep( 1 );
 __errExit:    
    grGlideShutdown();
    return 0;
}





