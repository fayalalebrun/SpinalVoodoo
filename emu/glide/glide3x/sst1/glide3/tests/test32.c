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

static const char name[]    = "test32";
static const char purpose[] = "draws gouraud shaded triangle using grDrawVertexArray";
static const char usage[]   = "-n <frames> -r <res> -d <filename>";

static void doHelp( void ) {
  grBufferClear( 0, 0, 0 );
  tlConOutput("Keymap:\n");
  tlConOutput("        C :         toggle WINDOW/CLIP COORDINATE\n");
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
    scrX       = 0.f,
    scrY       = 0.f,
    scrWidth   = 640.f,
    scrHeight  = 480.f;
  FxBool
    scrgrab = FXFALSE;

  GrScreenResolution_t
    resolution = GR_RESOLUTION_640x480;

  FxI32      listType = -1;
  FxBool     aaenable = FXFALSE;
  FxBool     packedrgb = FXFALSE;
  FxU32      coords_space = 0;
  GrVertex   vtxList[4];
  TlVertex3D srcVerts[4], xfVerts[4], prjVerts[4];
  void       *vtxListArray[6];
  FxFloat    distance = 1.f, dDelta = 0.05f;
  FxI32      i;
  FxFloat    vnear = 0.f, vfar = 1.f;
  FxU32      zrange[2];
  TlTexture            texture;
  
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
  grGet(GR_ZDEPTH_MIN_MAX, 8, zrange);  
  grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);

  /* Set up Render State */

  grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                  GR_COMBINE_FACTOR_LOCAL,
                  GR_COMBINE_LOCAL_ITERATED,
                  GR_COMBINE_OTHER_TEXTURE,
                  FXFALSE );

  grTexCombine( GR_TMU0,
                GR_COMBINE_FUNCTION_LOCAL,
                GR_COMBINE_FACTOR_NONE,
                GR_COMBINE_FUNCTION_NONE,
                GR_COMBINE_FACTOR_NONE,
                FXFALSE, FXFALSE );
  
  /* Load texture data into system ram */
  assert( tlLoadTexture( "matt1.3df", 
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
  /*
  grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                 GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_LOCAL_ITERATED,
                 GR_COMBINE_OTHER_NONE,
                 FXFALSE );
                 */

  grCullMode(GR_CULL_DISABLE);

  if(frames == -1) {
    doHelp();
  }

  /*
  ** data 
  */
  {
    /*
    ** 0--1
    ** |  |
    ** 2--3
    */
    srcVerts[0].x = -0.5f, srcVerts[0].y = 0.5f,  srcVerts[0].z = 0.f, srcVerts[0].w = 1.f;
    srcVerts[0].r = 1.f,   srcVerts[0].g = 0.f,   srcVerts[0].b = 0.f, srcVerts[0].a   = 1.f;
    srcVerts[0].s = 0.f,   srcVerts[0].t = 1.f;

    srcVerts[1].x = 0.5f,  srcVerts[1].y = 0.5f,  srcVerts[1].z = 0.f, srcVerts[1].w = 1.f;
    srcVerts[1].r = 0.f,   srcVerts[1].g = 1.f,   srcVerts[1].b = 0.f, srcVerts[1].a   = 1.f;
    srcVerts[1].s = 1.f,   srcVerts[1].t = 1.f;

    srcVerts[2].x = -0.5f, srcVerts[2].y = -0.5f, srcVerts[2].z = 0.f, srcVerts[2].w = 1.f;
    srcVerts[2].r = 0.f,   srcVerts[2].g = 0.f,   srcVerts[2].b = 1.f, srcVerts[2].a   = 1.f;
    srcVerts[2].s = 0.f,   srcVerts[2].t = 0.f;

    srcVerts[3].x = 0.5f,  srcVerts[3].y = -0.5f, srcVerts[3].z = 0.f, srcVerts[3].w = 1.f;
    srcVerts[3].r = 1.f,   srcVerts[3].g = 0.f,   srcVerts[3].b = 0.f, srcVerts[3].a   = 1.f;
    srcVerts[3].s = 1.f,   srcVerts[3].t = 0.f;

    vtxListArray[0] = (void *)&vtxList[0]; vtxListArray[1] = (void *)&vtxList[1];
    vtxListArray[2] = (void *)&vtxList[2]; vtxListArray[3] = (void *)&vtxList[3];

  }
  
  while( frames-- && tlOkToRender()) {
      
    tlConClear();
    tlConOutput( "Press a key to quit\n" );
    if (coords_space == GR_WINDOW_COORDS)
      tlConOutput("WINDOW COORDINATE\n");
    else
      tlConOutput("CLIP COORDINATE\n");
    if (aaenable)
      tlConOutput("Antialiasing ON\n");
    else
      tlConOutput("Antialiasing OFF\n");
    if (packedrgb)
      tlConOutput("Packed Color ON\n");
    else
      tlConOutput("Packed Color OFF\n");
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
    case -1:
      tlConOutput("grDrawTriangle\n");
      break;
    }

    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight );
        
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }
    grRenderBuffer( GR_BUFFER_BACKBUFFER );
    
    grBufferClear( 0x00, 0, zrange[1] );

    {
#define MAX_DIST 10.0f
#define MIN_DIST 1.0f

      distance += dDelta;
      if ( distance > MAX_DIST || 
           distance < MIN_DIST ) { 
        dDelta *= -1.0f;
        distance += dDelta;
      }

      tlSetMatrix( tlIdentity() );
      tlMultMatrix( tlTranslation( 0.0f, 0.0f, distance ) );      
      
      tlTransformVertices( xfVerts, srcVerts, 4 );
      tlCProjectVertices( prjVerts, xfVerts, 4 );

      if (coords_space == GR_WINDOW_COORDS) {
        grCoordinateSpace(GR_WINDOW_COORDS);
        for (i = 0; i < 4; i++) {
          vtxList[i].oow = 1.f / prjVerts[i].w;
          vtxList[i].x = prjVerts[i].x * vtxList[i].oow * scrWidth * 0.5f + scrWidth * 0.5f;
          vtxList[i].y = prjVerts[i].y * vtxList[i].oow * scrHeight * 0.5f + scrHeight * 0.5f;
          vtxList[i].z = prjVerts[i].z * vtxList[i].oow * (vfar - vnear) * 0.5f * 65535.f
            + (vfar + vnear) * 0.5f * 65535.f;
          
          if (!packedrgb) {
            vtxList[i].r = srcVerts[i].r * 255.f;
            vtxList[i].g = srcVerts[i].g * 255.f;
            vtxList[i].b = srcVerts[i].b * 255.f;
            vtxList[i].a = srcVerts[i].a * 255.f;
          }
          else
            vtxList[i].r = srcVerts[i].r;
          vtxList[i].tmuvtx[0].sow = srcVerts[i].s * 256.f * vtxList[i].oow;
          vtxList[i].tmuvtx[0].tow = srcVerts[i].t * 256.f * vtxList[i].oow;
        }
      }
      else {
        grCoordinateSpace(GR_CLIP_COORDS);
        grDepthRange( vnear, vfar );
        grViewport((FxU32)scrX, (FxU32)scrY, (FxU32)scrWidth, (FxU32)scrHeight);
        for (i = 0; i < 4; i++) {
          vtxList[i].oow = prjVerts[i].w;
          vtxList[i].x = prjVerts[i].x;
          vtxList[i].y = prjVerts[i].y;
          vtxList[i].z = prjVerts[i].z;
          vtxList[i].r = srcVerts[i].r;
          vtxList[i].g = srcVerts[i].g;
          vtxList[i].b = srcVerts[i].b;
          vtxList[i].a = srcVerts[i].a;
          vtxList[i].tmuvtx[0].sow = srcVerts[i].s;
          vtxList[i].tmuvtx[0].tow = srcVerts[i].t;
        }
      }      
    }

    switch (listType) {
    case GR_POINTS:
    case GR_LINE_STRIP:
    case GR_LINES:
    case GR_POLYGON:
    case GR_TRIANGLE_STRIP:
    case GR_TRIANGLE_FAN:
      grDrawVertexArray(listType, 4, vtxListArray);
      break;
    case GR_TRIANGLES:
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
      case 'V':
        if (coords_space) {
          if (scrX > 0.f) {
            scrX -= 5.f; scrY -= 5.f;
            scrWidth += 10.f; scrHeight += 10.f;
          }
        }
        else {
          scrX = 0.f; scrY = 0.f;
          scrWidth = 640.f; scrHeight = 480.f;
        }
        break;
      case 'v':
        if (coords_space) {
          if (scrX <= 200.f) {
            scrX += 5.f; scrY += 5.f;
            scrWidth -= 10.f; scrHeight -= 10.f;
          }
        }
        else {
          scrX = 0.f; scrY = 0.f;
          scrWidth = 640.f; scrHeight = 480.f;
        }
        break;
      case 'c':
      case 'C':
        coords_space = !coords_space;
        if (coords_space) {
          grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_DISABLE);
          grVertexLayout(GR_PARAM_W,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
        }
        else {
          grVertexLayout(GR_PARAM_W,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_DISABLE);
          grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
        }
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
          *(FxU32 *)&srcVerts[0].r = 0xff0000ff;
          *(FxU32 *)&srcVerts[1].r = 0xff00ff00;
          *(FxU32 *)&srcVerts[2].r = 0xffff0000;
          *(FxU32 *)&srcVerts[3].r = 0xffff0000;
        }
        else {
          grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
          if (aaenable)
            grVertexLayout(GR_PARAM_A,   GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);
          srcVerts[0].r = 1.f, srcVerts[0].g = 0.f, srcVerts[0].b = 0.f, srcVerts[0].a = 1.f;
          srcVerts[1].r = 0.f, srcVerts[1].g = 1.f, srcVerts[1].b = 0.f, srcVerts[1].a = 1.f;
          srcVerts[2].r = 0.f, srcVerts[2].g = 0.f, srcVerts[2].b = 1.f, srcVerts[2].a = 1.f;
          srcVerts[3].r = 0.f, srcVerts[3].g = 0.f, srcVerts[3].b = 1.f, srcVerts[3].a = 1.f;
        }
        break;
      case '0':
      case '1':
      case '2':
      case '4':
        listType =  key - '0';
        vtxListArray[0] = (void *)&vtxList[0]; vtxListArray[1] = (void *)&vtxList[1];
        vtxListArray[2] = (void *)&vtxList[2]; vtxListArray[3] = (void *)&vtxList[3];
        break;
      case '3':
      case '5':
        listType =  key - '0';
        vtxListArray[0] = (void *)&vtxList[1]; vtxListArray[1] = (void *)&vtxList[0];
        vtxListArray[2] = (void *)&vtxList[2]; vtxListArray[3] = (void *)&vtxList[3];
        break;
      case '6':
        listType =  key - '0';
        vtxListArray[0] = (void *)&vtxList[1]; vtxListArray[1] = (void *)&vtxList[0];
        vtxListArray[2] = (void *)&vtxList[2];
        vtxListArray[3] = (void *)&vtxList[1]; vtxListArray[4] = (void *)&vtxList[2];
        vtxListArray[5] = (void *)&vtxList[3];
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








