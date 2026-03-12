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
#include <g3ext.h>
#include "tlib.h"


/*-------------------------------------------------
 * Function: GetStencilFunction
 * Responsible Party: adamb 
 * Arguments: An index value corresponding to the 
 *            string printed on the screen in this
 *            test which happens to coincide exactly
 *            with the order in which these constants
 *            are defined in glide.h.
 * In short:  This function does nothing but
 *            abstract the constants in case they
 *            ever change.
 *------------------------------------------------*/
int
GetStencilFunction(int number)
{
  int retVal = GR_CMP_NEVER ; /* value to return */

  switch(number)
  {
    case  0:
      retVal = GR_CMP_NEVER ;
    break ; 

    case  1:
      retVal = GR_CMP_LESS ;
    break ; 

    case  2:
      retVal = GR_CMP_EQUAL ;
    break ; 

    case  3:
      retVal = GR_CMP_LEQUAL ;
    break ; 

    case  4:
      retVal = GR_CMP_GREATER ;
    break ; 

    case  5:
      retVal = GR_CMP_NOTEQUAL ;
    break ; 

    case  6:
      retVal = GR_CMP_GEQUAL ;
    break ; 

    case  7:
      retVal = GR_CMP_ALWAYS ;
    break ; 

    default:
      break ;
  }

  return retVal ;
}


/*-------------------------------------------------
 * Function: GetStencilOp
 * Responsible Party: adamb 
 * Arguments: An index value corresponding to the 
 *            string printed on the screen in this
 *            test which happens to coincide exactly
 *            with the order in which these constants
 *            are defined in glide.h.
 * In short:  This function does nothing but
 *            abstract the constants in case they
 *            ever change.
 *------------------------------------------------*/
int
GetStencilOp(int number)
{
  int retVal = GR_CMP_NEVER ; /* value to return */

  switch(number)
  {
    case  0:
      retVal = GR_STENCILOP_KEEP ;
    break ; 

    case  1:
      retVal = GR_STENCILOP_ZERO ;
    break ; 

    case  2:
      retVal = GR_STENCILOP_REPLACE ;
    break ; 

    case  3:
      retVal = GR_STENCILOP_INCR_CLAMP ;
    break ; 

    case  4:
      retVal = GR_STENCILOP_DECR_CLAMP ;
    break ; 

    case  5:
      retVal = GR_STENCILOP_INVERT ;
    break ; 

    case  6:
      retVal = GR_STENCILOP_INCR_WRAP ;
    break ; 

    case  7:
      retVal = GR_STENCILOP_DECR_WRAP ;
    break ; 

    default:
      break ;
  }

  return retVal ;
}



int hwconfig;
static const char *version;
static tlGlideExtension glideext;

static const char name[]    = "test49";
static const char purpose[] = "renders a series of triangles with different stenciling";
static const char usage[]   = "-n <frames> -r <res> -d <filename> -p <pixel format>";

int 
main( int argc, char **argv) 
{
  char match; 
  char **remArgs;
  int  rv;
  
  GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
  float                scrWidth   = 640.0f;
  float                scrHeight  = 480.0f;
  int frames                      = -1;
  int                  which_test = 0;
  int                  x, y ;
  FxBool               scrgrab = FXFALSE;
  char                 filename[256];
  FxU32                zrange[2];
  GrContext_t          gc = 0;
  FxU32                pixelformat = 0x0003;

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
    printf("Could not use STENCIL extension.\n");
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
  
  tlVertexLayout();
  grGet(GR_ZDEPTH_MIN_MAX, 8, zrange);  

  tlConSet( 0.0f, 0.0f, 1.0f, 1.0f, 
           60, 30, 0xffffff );
  
  /* Set up Render State - flat shading + Z-buffering */
  grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                 GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_LOCAL_CONSTANT,
                 GR_COMBINE_OTHER_NONE,
                 FXFALSE );
  grDepthBufferMode( GR_DEPTHBUFFER_ZBUFFER );
  grDepthBufferFunction( GR_CMP_ALWAYS );
  grDepthMask( FXTRUE );

  glideext.grStencilMask(0xFF);


  while( frames-- && tlOkToRender()) {
    GrVertex vtxA, vtxB, vtxC;

    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight );
      
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }

    grEnable(GR_STENCIL_MODE_EXT);

    glideext.grBufferClearExt(0x00, 0, zrange[1], 0);

    if (which_test)
    {
      /* exercise stencil ops */
      tlConClear() ;
      tlConOutput("Press <SPACE> to see stencil function test\n") ;
      tlConOutput("Press any other key to quit\n" ) ;
      tlConOutput( "\n                   Stencil Op Test\n\n" ) ;
      tlConOutput( "Keep    Zero   Replace ++Clmp --Clmp  Invert ++Wrap  --Wrap\n") ;
      tlConOutput( "\n\n\nStencil Fail\n\n\n\n\n\nZ Fail\n\n\n\n\n\nZ Pass\n\n\n") ;
      tlConOutput( "Never   Less   Equal   LEqual Greater NEqual GEqual  Always\n") ;

      /* TODO: Make this test have decypherable results
       *       my only excuse is that this test provides
       *       coverage...
       */

      for (y = 0 ; /* y is used to select the stencil fail, z fail or z pass stencil op */ 
           y < 3 ;
           y++)
      {
        for (x = 0 ;
             x < 8 ; /* x is used to select the stencil op */ 
             x++)
        {
          switch (y)
	  {
            case 0:
              glideext.grStencilOp(GetStencilOp(x),
                                   GR_STENCILOP_KEEP,
                                   GR_STENCILOP_KEEP);
	      break ;
	    case 1:
              glideext.grStencilOp(GR_STENCILOP_KEEP,
                                   GetStencilOp(x),
                                   GR_STENCILOP_KEEP);
	      break ;
	    case 2:
              glideext.grStencilOp(GR_STENCILOP_KEEP,
                                   GR_STENCILOP_KEEP,
                                   GetStencilOp(x));
	      break ;
	    default:
	      break ;
	  }

          /* draw stencil triangle */
          vtxA.x = tlScaleX( (float)(x / 8.0f) ),          vtxA.y = tlScaleY( (float)((y + 1.00f) / 5.0f) );
          vtxB.x = tlScaleX( (float)((x + 1) / 8.0f) ),    vtxB.y = tlScaleY( (float)((y + 1.00f) / 5.0f) );
          vtxC.x = tlScaleX( (float)((x + 0.5f) / 8.0f) ), vtxC.y = tlScaleY( (float)((y + 2.00f) / 5.0f) );
          vtxA.ooz = vtxB.ooz = vtxC.ooz = ( 65535.0f / 10.0f );
          grConstantColorValue( 0x00808080 );

          glideext.grStencilFunc(GR_CMP_ALWAYS, 1, 0xFF);
          grDrawTriangle( &vtxA, &vtxB, &vtxC );
 
          /* draw first overlapping triangle -- for completeness this should fail the Z test */
          vtxA.x = tlScaleX( (float)(x / 8.0f) ),          vtxA.y = tlScaleY( (float)((y + 1.00f) / 5.0f) );
          vtxB.x = tlScaleX( (float)((x + 1) / 8.0f) ),    vtxB.y = tlScaleY( (float)((y + 1.00f) / 5.0f) );
          vtxC.x = tlScaleX( (float)(x / 8.0f) ),          vtxC.y = tlScaleY( (float)((y + 2.00f) / 5.0f) );
          vtxA.ooz = vtxB.ooz = vtxC.ooz = ( 65535.0f / 10.5f );
          grConstantColorValue( 0x00FF0000 );

          glideext.grStencilFunc(GetStencilFunction(x), 1, 0xFF);
          grDrawTriangle( &vtxA, &vtxB, &vtxC );

	  /* draw second overlapping triangle */
          vtxA.x = tlScaleX( (float)(x / 8.0f) ),          vtxA.y = tlScaleY( (float)((y + 1.00f) / 5.0f) );
          vtxB.x = tlScaleX( (float)((x + 1) / 8.0f) ),    vtxB.y = tlScaleY( (float)((y + 1.00f) / 5.0f) );
          vtxC.x = tlScaleX( (float)((x + 1) / 8.0f) ),    vtxC.y = tlScaleY( (float)((y + 2.00f) / 5.0f) );
          vtxA.ooz = vtxB.ooz = vtxC.ooz = ( 65535.0f / 7.5f );
          grConstantColorValue( 0x000000FF );

          glideext.grStencilFunc(GetStencilFunction(x), 2, 0xFF);
          grDrawTriangle( &vtxA, &vtxB, &vtxC );
        } /* for x... */
      } /* for y... */

    } 
     else /* if (which_test) */
    {
      /* test stencil functions with varied reference values */

      glideext.grStencilOp(GR_STENCILOP_KEEP,
                           GR_STENCILOP_KEEP,
                           GR_STENCILOP_REPLACE);
 
      tlConClear() ;
      tlConOutput( "Press <SPACE> to see stencil op test\n" );
      tlConOutput( "Press any other key to quit\n" );
      tlConOutput( "\n                   Stencil Function Test\n" ) ;
      tlConOutput( "Never   Less   Equal   LEqual Greater NEqual GEqual  Always\n") ;
      tlConOutput( "\n\n\n\nGreater\n\n\n\n\n\nEqual\n\n\n\n\n\nLess\n") ;

      for (y = 0 ; /* y is used to iterate the reference value for the grey triangle */
           y < 3 ;
           y++)
      {
        for (x = 0 ;
             x < 8 ; /* x is used to iterate the stencil function for the colored triangle */
             x++)
        {
          /* draw stencil triangle */
          vtxA.x = tlScaleX( (float)(x / 8.0f) ),          vtxA.y = tlScaleY( (float)((y + 1.25f) / 5.0f) );
          vtxB.x = tlScaleX( (float)((x + 1) / 8.0f) ),    vtxB.y = tlScaleY( (float)((y + 1.25f) / 5.0f) );
          vtxC.x = tlScaleX( (float)((x + 0.5f) / 8.0f) ), vtxC.y = tlScaleY( (float)((y + 2.00f) / 5.0f) );
          vtxA.ooz = vtxB.ooz = vtxC.ooz = ( 65535.0f / 10.0f );
          grConstantColorValue( 0x00808080 );

          glideext.grStencilFunc(GR_CMP_ALWAYS, y, 0xFF);
          grDrawTriangle( &vtxA, &vtxB, &vtxC );
 
          /* draw overlapping triangle */
          vtxA.x = tlScaleX( (float)((x / 8.0f)) ),        vtxA.y = tlScaleY( (float)((y + 1.75f) / 5.0f) );
          vtxB.x = tlScaleX( (float)((x + 0.5f) / 8.0f) ), vtxB.y = tlScaleY( (float)((y + 1.00f) / 5.0f) );
          vtxC.x = tlScaleX( (float)((x + 1) / 8.0f) ),    vtxC.y = tlScaleY( (float)((y + 1.75f) / 5.0f) );
          vtxA.ooz = vtxB.ooz = vtxC.ooz = ( 65535.0f / 7.5f );
          grConstantColorValue( 0x000000FF << (8 * (x % 3))); /* make the colors different so it's pretty */

          glideext.grStencilFunc(GetStencilFunction(x), 1, 0xFF);
          grDrawTriangle( &vtxA, &vtxB, &vtxC );
        } /* for x... */
      } /* for y... */
    } /* else which_test */

    glideext.grStencilFunc(GR_CMP_ALWAYS, 0, 0xFF);

    grDisable(GR_STENCIL_MODE_EXT);

    tlConRender();
    grBufferSwap( 1 );

    /* grab the frame buffer */
    if (scrgrab) {
      if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight))
        printf( "Cannot open %s\n", filename);
      scrgrab = FXFALSE;
    }

    while (tlKbHit() ) {
      switch( tlGetCH() ) {
        case ' ':
          which_test = (which_test + 1) & 1;
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
