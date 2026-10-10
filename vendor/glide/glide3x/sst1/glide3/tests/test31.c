/*
** Insert new header here
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

static const char name[]    = "test31";
static const char purpose[] = "draws gouraud shaded triangle using grDrawVertexArray";
static const char usage[]   = "-n <frames> -r <res> -d <filename>";

static void doHelp( void ) {
  grBufferClear( 0, 0, 0 );
  tlConOutput("Keymap:\n");
  tlConOutput("        C :         toggle grDrawVertexArray/grDrawVertexArrayContiguous\n");
  tlConOutput("        A :         toggle Antialiasing\n");
  tlConOutput("        P :         toggle packed color\n");
  tlConOutput("        T :         draw triangle\n");
  tlConOutput("        0 :         draw points\n");
  tlConOutput("        1 :         draw line strip\n");
  tlConOutput("        2 :         draw lines\n");
  tlConOutput("        3 :         draw polygon\n");
  tlConOutput("        4 :         draw triangle strip\n");
  tlConOutput("        5 :         draw triangle fan\n");
  tlConOutput("        6 :         draw triangles\n");
  tlConOutput("Press a key to continue...\n");
  tlConRender();
  grBufferSwap( 1 );
  tlGetCH();
  tlConClear();
}

int
main( int argc, char **argv)
{
  char
    match, 
    **remArgs,
    *args = "nrd",
    filename[256];
  int
    frames = -1,
    rv;
  float
    scrWidth   = 640.f,
    scrHeight  = 480.f;
  FxBool
    scrgrab = FXFALSE;

  GrScreenResolution_t
    resolution = GR_RESOLUTION_640x480;

  FxI32    listType = -1;
  FxBool   aaenable = FXFALSE;
  FxBool   packedrgb = FXFALSE;
  FxBool   varraycont = FXFALSE;
  GrVertex vtxList[4];
  void     *vtxListArray[6];
  GrVertex vtxListCont[6];
  FxU32    wrange[2];
  
  /* Initialize Glide */
  grGlideInit();
  assert( hwconfig = tlVoodooType() );

  /* Process Command Line Arguments */
  while((rv = tlGetOpt(argc, argv, args, &match, &remArgs)) != 0) {
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
  assert( grSstWinOpen(tlGethWnd(),
                       resolution,
                       GR_REFRESH_60Hz,
                       GR_COLORFORMAT_ABGR,
                       GR_ORIGIN_UPPER_LEFT,
                       2, 1 ) );  

  tlConSet( 0.f, 0.f, 1.f, 1.f, 
           60, 30, 0xffffff );
  
  /* Set up Render State - gouraud shading */
  grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
  grGet(GR_WDEPTH_MIN_MAX, 8, wrange);  

  grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                 GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_LOCAL_ITERATED,
                 GR_COMBINE_OTHER_NONE,
                 FXFALSE );

  grCullMode(GR_CULL_DISABLE);

  if(frames == -1) {
    doHelp();
  }


  /*
  ** data 
  */
  {
    vtxList[0].x = tlScaleX( 0.3f ), vtxList[0].y = tlScaleY( 0.3f );
    vtxList[0].r = 255.f, vtxList[0].g = 0.f, vtxList[0].b = 0.f, vtxList[0].a = 255.f;
    
    vtxList[1].x = tlScaleX( 0.8f ), vtxList[1].y = tlScaleY( 0.4f );
    vtxList[1].r = 0.f, vtxList[1].g = 255.f, vtxList[1].b = 0.f, vtxList[1].a = 255.f;
    
    vtxList[2].x = tlScaleX( 0.5f ), vtxList[2].y = tlScaleY( 0.8f );
    vtxList[2].r = 0.f, vtxList[2].g = 0.f, vtxList[2].b = 255.f, vtxList[2].a = 255.f;
    
    vtxList[3].x = tlScaleX( 0.9f ), vtxList[3].y = tlScaleY( 0.8f );
    vtxList[3].r = 0.f, vtxList[3].g = 0.f, vtxList[3].b = 255.f, vtxList[3].a = 255.f;
    
    vtxListCont[0] = vtxList[0];
    vtxListCont[1] = vtxList[1];
    vtxListCont[2] = vtxList[2];
    vtxListCont[3] = vtxList[3];
  }
  
  while( frames-- && tlOkToRender()) {
      
    tlConClear();
    tlConOutput( "Press a key to quit\n" );
    if (aaenable)
      tlConOutput("Antialiasing ON\n");
    else
      tlConOutput("Antialiasing OFF\n");
    if (packedrgb)
      tlConOutput("Packed Color ON\n");
    else
      tlConOutput("Packed Color OFF\n");
    if (listType >= 0) {
      if (varraycont)
        tlConOutput("grDrawVertexArrayContiguous\n");
      else
        tlConOutput("grDrawVertexArray\n");
      switch (listType) {
      case GR_POINTS:
        tlConOutput("POINTS\n");
        break;
      case GR_LINE_STRIP:
        tlConOutput("LINE STRIP\n");
        break;
      case GR_LINES:
        tlConOutput("LINES\n");
        break;
      case GR_POLYGON:
        tlConOutput("POLYGON\n");
        break;
      case GR_TRIANGLE_STRIP:
        tlConOutput("TRIANGLE STRIP\n");
        break;
      case GR_TRIANGLE_FAN:
        tlConOutput("TRIANGLE FAN\n");
        break;
      case GR_TRIANGLES:
        tlConOutput("TRIANGLES\n");
        break;
      }
    }
    else
      tlConOutput("grDrawTriangle\n");

    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight );
        
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }
    grRenderBuffer( GR_BUFFER_BACKBUFFER );
    
    grBufferClear( 0x00, 0, wrange[1] );

    switch (listType) {
    case GR_POINTS:
    case GR_LINE_STRIP:
    case GR_LINES:
    case GR_POLYGON:
    case GR_TRIANGLE_STRIP:
    case GR_TRIANGLE_FAN:
      if (varraycont)
        grDrawVertexArrayContiguous(listType, 4, vtxListCont, sizeof(GrVertex));
      else
        grDrawVertexArray(listType, 4, vtxListArray);
      break;
    case GR_TRIANGLES:
      if (varraycont)
        grDrawVertexArrayContiguous(listType, 6, vtxListCont, sizeof(GrVertex));
      else
        grDrawVertexArray(listType, 6, vtxListArray);
      break;
    default:
      if (aaenable)
        grAADrawTriangle(vtxList, vtxList+1, vtxList+2, FXTRUE, FXTRUE, FXTRUE);
      else
        grDrawTriangle(vtxList, vtxList+1, vtxList+2);
    }

    tlConRender();
    grBufferSwap( 1 );
    
    /* grab the frame buffer */
    if (scrgrab) {
      if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight))
        printf( "Cannot open %s\n", filename);
      scrgrab = FXFALSE;
    }

    while( tlKbHit() ) {
      FxU32 key;
      switch( key = tlGetCH() ) {
      case 'c':
      case 'C':
        varraycont = !varraycont;
        break;
      case 't':
      case 'T':
        listType = -1;
        break;
      case 'a':
      case 'A':
        aaenable = !aaenable;
        if (aaenable) {
          grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL,
                          GR_COMBINE_FACTOR_NONE,
                          GR_COMBINE_LOCAL_ITERATED,
                          GR_COMBINE_OTHER_NONE,
                          FXFALSE );
          grAlphaBlendFunction( GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
                                GR_BLEND_ZERO, GR_BLEND_ZERO );
          grEnable(GR_AA_ORDERED);
          if (!packedrgb)
            grVertexLayout(GR_PARAM_A,
                           GR_VERTEX_A_OFFSET << 2,
                           GR_PARAM_ENABLE);
        }
        else {
          grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                         GR_COMBINE_FACTOR_ONE,
                         GR_COMBINE_LOCAL_NONE,
                         GR_COMBINE_OTHER_CONSTANT,
                         FXFALSE);
          grAlphaBlendFunction(GR_BLEND_ONE , GR_BLEND_ZERO, 
                               GR_BLEND_ONE, GR_BLEND_ZERO);
          grDisable(GR_AA_ORDERED);
          if (!packedrgb)
            grVertexLayout(GR_PARAM_A,
                           GR_VERTEX_A_OFFSET << 2,
                           GR_PARAM_DISABLE);
        }
        break;
      case 'p':
      case 'P':
        packedrgb = !packedrgb;
        if (packedrgb) {
          grVertexLayout(GR_PARAM_PARGB,   GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
          *(FxU32*)&vtxList[0].r = 0xff0000ff;
          *(FxU32*)&vtxList[1].r = 0xff00ff00;
          *(FxU32*)&vtxList[2].r = 0xffff0000;
          *(FxU32*)&vtxList[3].r = 0xffff0000;
          *(FxU32*)&vtxList[4].r = 0xff00ff00;
          *(FxU32*)&vtxList[5].r = 0xff0000ff;
        }
        else {
          grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
          if (aaenable)
            grVertexLayout(GR_PARAM_A,   GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);
          vtxList[0].r = 255.f, vtxList[0].g = 0.f, vtxList[0].b = 0.f, vtxList[0].a = 255.f;
          vtxList[1].r = 0.f, vtxList[1].g = 255.f, vtxList[1].b = 0.f, vtxList[1].a = 255.f;
          vtxList[2].r = 0.f, vtxList[2].g = 0.f, vtxList[2].b = 255.f, vtxList[2].a = 255.f;
          vtxList[3].r = 0.f, vtxList[3].g = 0.f, vtxList[3].b = 255.f, vtxList[3].a = 255.f;
          vtxList[4].r = 0.f, vtxList[4].g = 255.f, vtxList[4].b = 0.f, vtxList[4].a = 255.f;
          vtxList[5].r = 0.f, vtxList[5].g = 0.f, vtxList[5].b = 255.f, vtxList[5].a = 255.f;
        }
        switch (listType) {
        case 0:
        case 1:
        case 2:
        case 4:
          vtxListCont[0] = vtxList[0]; vtxListCont[1] = vtxList[1];
          vtxListCont[2] = vtxList[2]; vtxListCont[3] = vtxList[3];
          break;
        case 3:
        case 5:
          vtxListCont[0] = vtxList[1]; vtxListCont[1] = vtxList[0];
          vtxListCont[2] = vtxList[2]; vtxListCont[3] = vtxList[3];
          break;
        case 6:
          vtxListCont[0] = vtxList[1]; vtxListCont[1] = vtxList[0]; vtxListCont[2] = vtxList[2]; 
          vtxListCont[3] = vtxList[1]; vtxListCont[4] = vtxList[2]; vtxListCont[5] = vtxList[3];
          break;
        }
        break;
      case '0':
      case '1':
      case '2':
      case '4':
        listType =  key - '0';
        vtxListArray[0] = (void *)&vtxList[0]; vtxListArray[1] = (void *)&vtxList[1];
        vtxListArray[2] = (void *)&vtxList[2]; vtxListArray[3] = (void *)&vtxList[3];
        vtxListCont[0] = vtxList[0]; vtxListCont[1] = vtxList[1];
        vtxListCont[2] = vtxList[2]; vtxListCont[3] = vtxList[3];
        break;
      case '3':
      case '5':
        listType =  key - '0';
        vtxListArray[0] = (void *)&vtxList[1]; vtxListArray[1] = (void *)&vtxList[0];
        vtxListArray[2] = (void *)&vtxList[2]; vtxListArray[3] = (void *)&vtxList[3];
        vtxListCont[0] = vtxList[1]; vtxListCont[1] = vtxList[0];
        vtxListCont[2] = vtxList[2]; vtxListCont[3] = vtxList[3];
        break;
      case '6':
        listType =  key - '0';
        vtxListArray[0] = (void *)&vtxList[1]; vtxListArray[1] = (void *)&vtxList[0];
        vtxListArray[2] = (void *)&vtxList[2];
        vtxListArray[3] = (void *)&vtxList[1]; vtxListArray[4] = (void *)&vtxList[2];
        vtxListArray[5] = (void *)&vtxList[3];
        vtxListCont[0] = vtxList[1]; vtxListCont[1] = vtxList[0]; vtxListCont[2] = vtxList[2]; 
        vtxListCont[3] = vtxList[1]; vtxListCont[4] = vtxList[2]; vtxListCont[5] = vtxList[3];
        break;
      default:
        frames = 0;
        break;
      }
    }
  }
  
  grGlideShutdown();
  return 0;
} /* main */








