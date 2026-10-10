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
#include <assert.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"


int hwconfig;
static const char *version;
static tlGlideExtension glideext;

static const char name[]    = "test43";
static const char purpose[] = "color mask test";
static const char usage[]   = "-n <frames> -r <res> -d <filename> -p <pixel format>";

const char *mName[] = {
  "OFF",
  "ON "
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
  FxU32                wrange[2];
  GrContext_t          gc = 0;
  FxU32                pixelformat = 0x0003;
  FxBool               r_mask = FXTRUE;
  FxBool               g_mask = FXTRUE;
  FxBool               b_mask = FXTRUE;
  FxBool               a_mask = FXTRUE;

  /* Initialize Glide */
  grGlideInit();
  assert( hwconfig = tlVoodooType() );
  
  /* Process Command Line Arguments */
  while((rv = tlGetOpt(argc, argv, "nrdp", &match, &remArgs)) != 0) {
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
  
  if (!glideext.pixext) {
    printf("Could not use PIXEXT extension.\n");
    goto __errExit;
  }

  gc = glideext.grSstWinOpen(tlGethWnd(),
                             resolution,
                             GR_REFRESH_60Hz,
                             GR_COLORFORMAT_ABGR,
                             GR_ORIGIN_UPPER_LEFT,
                             pixelformat,
                             2, 1);

  if (!gc) {
    printf("Could not allocate glide fullscreen context.\n");
    goto __errExit;
  }
  
  grGet(GR_WDEPTH_MIN_MAX, 8, (FxI32 *)wrange);  
  grVertexLayout(GR_PARAM_XY,  0, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
  
  tlConSet( 0.0f, 0.0f, 1.0f, 1.0f, 
            60, 30, 0xffffff );
  
  /* Set up Render State - gouraud shading */
  grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_LOCAL_ITERATED,
                  GR_COMBINE_OTHER_NONE,
                  FXFALSE );
  
  while( frames-- && tlOkToRender()) {
    GrVertex vtxA, vtxB, vtxC;
    
    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight );
      
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }
    
    tlConOutput( "Press a key to quit\n" );
    tlConOutput( "Press R,G,B,A to toggle red,green,blue,alpha write mask\n" );
    tlConOutput( "R: %s, G: %s, B: %s, A: %s \n", 
                 mName[r_mask], mName[g_mask], mName[b_mask], mName[a_mask] );

    glideext.grColorMask(FXTRUE, FXTRUE, FXTRUE, FXTRUE);

    grBufferClear(0x00000000, 0, wrange[1]);
    
    glideext.grColorMask(r_mask, g_mask, b_mask, a_mask);

    grBufferClear(0x7f7f7f7f, 0, wrange[1]);
    

    vtxA.x = tlScaleX( 0.3f ), vtxA.y = tlScaleY( 0.3f );
    vtxA.r = 255.0f, vtxA.g = 0.0f, vtxA.b = 0.0f;
    
    vtxB.x = tlScaleX( 0.8f ), vtxB.y = tlScaleY( 0.4f );
    vtxB.r = 0.0f, vtxB.g = 255.0f, vtxB.b = 0.0f;
    
    vtxC.x = tlScaleX( 0.5f ), vtxC.y = tlScaleY( 0.8f );
    vtxC.r = 0.0f, vtxC.g = 0.0f, vtxC.b = 255.0f;
    
    grDrawTriangle( &vtxA, &vtxB, &vtxC );
    
    tlConRender();
    tlConClear();
    grBufferSwap( 1 );
    
    /* grab the frame buffer */
    if (scrgrab) {
      if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight))
        printf( "Cannot open %s\n", filename);
      scrgrab = FXFALSE;
    }

    while(tlKbHit()) {
      const char curKey = tlGetCH();
      
      switch(curKey) {
      case 'r':
      case 'R':
        r_mask = (!r_mask);
        break;
      case 'g':
      case 'G':
        g_mask = (!g_mask);
        break;
      case 'b':
      case 'B':
        b_mask = (!b_mask);
        break;
      case 'a':
      case 'A':
        a_mask = (!a_mask);
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

