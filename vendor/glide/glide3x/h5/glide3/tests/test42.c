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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glide.h>
#include <g3ext.h>
#include "tlib.h"

#define TEX_MAX 3 /* max size of bigTex variable */
/*
 * Meaning of bigTex:
 * 0: Stay in "small texture" mode (max texture size is 256)
 * 1: Use a 512x512 texture
 * 2: Use a 1024x1024 texture
 * 3: Use a 2048x2048 texture
 * Further, if TEXFMT extension is not available, bigTex is always 0.
 */

int hwconfig;
static const char *version;
static tlGlideExtension glideext;

static const char name[]    = "test42";
static const char purpose[] = "32-bit and 2k texturing example, decal, rgb lit, white lit, flat lit";
static const char usage[]   = "-n <frames> -r <res> -d <filename> -p <pixel format>";

typedef enum { DECAL, FLATLIT, RGBLIT, WHITELIT, SPECALPHA } TextureMode;
const char *textureModeNames[] = {
  "DECAL TEXTURE                   ",
  "FLAT SHADING * TEXTURE          ",
  "ITERATED RGB * TEXTURE          ", 
  "INTENSITY LIGHTING * TEXTURE    ",
  "(ITRGB * TEXTURE)+WHITE SPECULAR"
};

const char *texWidthString[] = {
  "Max. Texture size is 256\n",
  "Max. Texture size is 512\n",
  "Max. Texture size is 1024\n",
  "Max. Texture size is 2048\n",
};

const char *texFileNames[] = {
  "t256x32.3df",   /* 32-bit;  256x256  LOD only */
  "t512x32.3df",   /* 32-bit;  512x512  LOD only */
  "t1Kx32.3df",    /* 32-bit; 1024x1024 LOD only */
  "t2Kx32.3df",    /* 32-bit; 2048x2048 LOD only */
};

const char *cmodeName[] = {
  "OFF     ",
  "ON      "
};

int 
main(int argc, char **argv) 
{
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

  TlTexture            texture;
  FxU32                zrange[2];
  GrContext_t          gc = 0;
  FxU32                pixelformat = 0x0003; /* 16 bit 565 */
  static FxBool        useCCExt = 1;
  char                 textureFile[256];
  int                  bigTex = 0;
  FxBool               needToLoadNewTexture = FXFALSE;
  FxBool               umaIsEnabled = FXFALSE;

  float 
    xLeft,
    xRight,
    yTop,
    yBottom;

  /* Process Command Line Arguments */
  while((rv = tlGetOpt(argc, argv, "nrdxp", &match, &remArgs)) != 0) {
    if (rv == -1) {
      printf("Unrecognized command line argument\n");
      printf("%s %s\n", name, usage);
      printf("Available resolutions:\n%s\n",
             tlGetResolutionList());
      return -1;
    }
    switch(match) {
    case 'n':
      frames = atoi(remArgs[0]);
      break;
    case 'r':
      resolution = tlGetResolutionConstant(remArgs[0], 
                                           &scrWidth, 
                                           &scrHeight);
      break;
    case 'd':
      scrgrab = FXTRUE;
      frames = 5;      
      strcpy(filename, remArgs[0]);
      break;

    case 'p':
      pixelformat = tlGetPixelFormat( remArgs[0] );
      break;
    }
  }

  tlSetScreen(scrWidth, scrHeight);

  version = grGetString(GR_VERSION);

  printf("%s:\n%s\n", name, purpose);
  printf("%s\n", version);
  printf("Resolution: %s\n", tlGetResolutionString(resolution));
  if (frames == -1) {
    printf("Press A Key To Begin Test.\n");
    tlGetCH();
  }
    
  /* Initialize Glide */
  grGlideInit();
  assert(hwconfig = tlVoodooType());

  grGet(GR_ZDEPTH_MIN_MAX, 8, (FxI32 *)zrange);

  grSstSelect(0);

  tlInitGlideExt(&glideext);
  
  if (!glideext.combineext) {
    printf("Could not use COMBINE extension.\n");
    useCCExt = 0;
  }

  if (glideext.canDo32BitTexture) {
    printf("Using TEXFMT 32-bit extension.\n");
    strcpy(textureFile, "t256x32.3df"); /* 32 bits deep */
  } else {
    printf("Could not use TEXFMT 32-bit extension.\n");
    strcpy(textureFile, "miro.3df");    /* 16 bits deep */
  }

  if (glideext.umaExt) {
    printf("Using TEXUMA extension.\n");
    grEnable(GR_TEXTURE_UMA_EXT);
    umaIsEnabled = FXTRUE;
  } else {
    printf("Could not use TEXUMA extension.\n");
    grDisable(GR_TEXTURE_UMA_EXT);
    umaIsEnabled = FXFALSE;
  }

  if (glideext.grSstWinOpen) {
    gc = glideext.grSstWinOpen(tlGethWnd(),
                               resolution,
                               GR_REFRESH_60Hz,
                               GR_COLORFORMAT_ABGR,
                               GR_ORIGIN_UPPER_LEFT,
                               pixelformat,
                               2, 1);
  } else {
    gc = grSstWinOpen(tlGethWnd(),
                      resolution,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1);
  }
  if (!gc) {
    printf("Could not allocate glide fullscreen context.\n");
    goto __errExit;
  }
  
  tlVertexLayout();
  tlConSet(0.0f, 0.0f, 1.0f, 0.5f, 
           60, 15, 0xffffff);
    
  /* Load texture data into system ram */
  assert(tlLoadTexture(textureFile,
                       &texture.info, 
                       &texture.tableType, 
                       &texture.tableData));
  /* Download texture data to TMU */
  grTexDownloadMipMap(GR_TMU0,
                      grTexMinAddress(GR_TMU0),
                      GR_MIPMAPLEVELMASK_BOTH,
                      &texture.info);
  if (texture.tableType != NO_TABLE) {
    grTexDownloadTable(texture.tableType,
                       &texture.tableData);
  }

  /* Select Texture As Source of all texturing operations */
  grTexSource(GR_TMU0,
              grTexMinAddress(GR_TMU0),
              GR_MIPMAPLEVELMASK_BOTH,
              &texture.info);


  /* Enable Bilinear Filtering + Mipmapping */
  grTexFilterMode(GR_TMU0,
                  GR_TEXTUREFILTER_BILINEAR,
                  GR_TEXTUREFILTER_BILINEAR);
  grTexMipMapMode(GR_TMU0,
                  GR_MIPMAP_NEAREST,
                  FXFALSE);

  xLeft   = tlScaleX( 0.2f );
  xRight  = tlScaleX( 0.8f );
  yTop    = tlScaleY( 0.2f );
  yBottom = tlScaleY( 0.8f );

  while(frames-- && tlOkToRender()) {
    GrVertex vtxA, vtxB, vtxC, vtxD;
    static TextureMode textureMode;

    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight);
        
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }

    grBufferClear(0x00, 0, zrange[1]);

    tlConOutput(  "m   : change lighting mode\n");
    tlConOutput(  "e   : toggle combine extension\n");
    tlConOutput(  "b   : big textures (if TEXFMT extension is available)\n");
    tlConOutput(  "u   : toggle UMA (if TEXUMA extension is available)\n");
    tlConOutput("Press any other key to quit\n");
    tlConOutput("Current Texture Mode: %s\n", textureModeNames[textureMode]);
    tlConOutput("COMBINE extension %s\n", cmodeName[useCCExt]);

    if (needToLoadNewTexture) {
      /*
       * This is only true if TEXFMT extension is available, which
       * implies 2k texture capability.  Also, this is only true if
       * the user has just hit 'b' for big texture testing.
       */
      assert(bigTex <= TEX_MAX);
      strcpy(textureFile, texFileNames[bigTex]);

      /* Redo all the texture loading stuff. */
      /* Load texture data into system ram */
      assert(tlLoadTexture(textureFile, &texture.info, &texture.tableType, 
                           &texture.tableData));
      /* Download texture data to TMU */
      grTexDownloadMipMap(GR_TMU0, grTexMinAddress(GR_TMU0),
                          GR_MIPMAPLEVELMASK_BOTH, &texture.info);
      if (texture.tableType != NO_TABLE) {
        grTexDownloadTable(texture.tableType, &texture.tableData);
      }
      
      /* Select Texture As Source of all texturing operations */
      grTexSource(GR_TMU0, grTexMinAddress(GR_TMU0), 
                  GR_MIPMAPLEVELMASK_BOTH, &texture.info);

      needToLoadNewTexture = FXFALSE;
    }

    if (useCCExt) {
#ifdef FX_GLIDE_NAPALM
      glideext.grTexColorCombineExt(GR_TMU0,
                                    GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                                    GR_CMBX_LOCAL_TEXTURE_RGB, GR_FUNC_MODE_X, 
                                    GR_CMBX_ZERO, FXFALSE,
                                    GR_CMBX_B, FXFALSE,
                                    0, FXFALSE);
      switch(textureMode) {
      case DECAL:
        glideext.grColorCombineExt(GR_CMBX_TEXTURE_RGB, GR_FUNC_MODE_X, 
                                   GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                                   GR_CMBX_ZERO, FXTRUE, 
                                   GR_CMBX_ZERO, FXFALSE,
                                   0, FXFALSE);
        break;
      case FLATLIT:
        glideext.grColorCombineExt(GR_CMBX_TEXTURE_RGB, GR_FUNC_MODE_X, 
                                   GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                                   GR_CMBX_CONSTANT_COLOR, FXFALSE,
                                   GR_CMBX_ZERO, FXFALSE,
                                   0, FXFALSE);
        break;
      case RGBLIT:
        glideext.grColorCombineExt(GR_CMBX_TEXTURE_RGB, GR_FUNC_MODE_X, 
                                   GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                                   GR_CMBX_ITRGB, FXFALSE,
                                   GR_CMBX_ZERO, FXFALSE,
                                   0, FXFALSE);
        break;
      case WHITELIT:
        glideext.grColorCombineExt(GR_CMBX_TEXTURE_RGB, GR_FUNC_MODE_X, 
                                   GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO, 
                                   GR_CMBX_B, FXFALSE,
                                   GR_CMBX_ZERO, FXFALSE,
                                   0, FXFALSE);
        glideext.grAlphaCombineExt(GR_CMBX_ITALPHA, GR_FUNC_MODE_X, 
                                   GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                                   GR_CMBX_ZERO, FXTRUE,
                                   GR_CMBX_ALOCAL, FXFALSE,
                                   0, FXFALSE);
        break;
      case SPECALPHA:
        glideext.grColorCombineExt(GR_CMBX_TEXTURE_RGB, GR_FUNC_MODE_X, 
                                   GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                   GR_CMBX_ITRGB, FXFALSE,
                                   GR_CMBX_B, FXFALSE,
                                   0, FXFALSE);
        glideext.grAlphaCombineExt(GR_CMBX_ITALPHA, GR_FUNC_MODE_X, 
                                   GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                                   GR_CMBX_ZERO, FXTRUE,
                                   GR_CMBX_ALOCAL, FXFALSE,
                                   0, FXFALSE);
        break;
      }
#endif
    }
    else {
      /* Set up Render State - Decal Texture - color combine set in render loop */
      grTexCombine(GR_TMU0,
                   GR_COMBINE_FUNCTION_LOCAL,
                   GR_COMBINE_FACTOR_NONE,
                   GR_COMBINE_FUNCTION_NONE,
                   GR_COMBINE_FACTOR_NONE,
                   FXFALSE, FXFALSE);

      switch(textureMode) {
      case DECAL:
        grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                       GR_COMBINE_FACTOR_ONE,
                       GR_COMBINE_LOCAL_NONE,
                       GR_COMBINE_OTHER_TEXTURE,
                       FXFALSE);
        break;
      case FLATLIT:
        grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                       GR_COMBINE_FACTOR_LOCAL,
                       GR_COMBINE_LOCAL_CONSTANT,
                       GR_COMBINE_OTHER_TEXTURE,
                       FXFALSE);
        break;
      case RGBLIT:
        grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                       GR_COMBINE_FACTOR_LOCAL,
                       GR_COMBINE_LOCAL_ITERATED,
                       GR_COMBINE_OTHER_TEXTURE,
                       FXFALSE);
        break;
      case WHITELIT:
        grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                       GR_COMBINE_FACTOR_LOCAL_ALPHA,
                       GR_COMBINE_LOCAL_NONE,
                       GR_COMBINE_OTHER_TEXTURE,
                       FXFALSE);
        grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
                       GR_COMBINE_FACTOR_NONE,
                       GR_COMBINE_LOCAL_ITERATED,
                       GR_COMBINE_OTHER_TEXTURE,
                       FXFALSE);
        break;
      case SPECALPHA:
        grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA,
                       GR_COMBINE_FACTOR_LOCAL,
                       GR_COMBINE_LOCAL_ITERATED,
                       GR_COMBINE_OTHER_TEXTURE,
                       FXFALSE);
        grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
                       GR_COMBINE_FACTOR_NONE,
                       GR_COMBINE_LOCAL_ITERATED,
                       GR_COMBINE_OTHER_TEXTURE,
                       FXFALSE);
        break;
      }
    }
        
    /*---- 
      A-B
      |\|
      C-D
      -----*/
    vtxA.oow = 1.0f;
    vtxB = vtxC = vtxD = vtxA;

    vtxA.x = vtxC.x = xLeft;
    vtxB.x = vtxD.x = xRight;
    vtxA.y = vtxB.y = yTop;
    vtxC.y = vtxD.y = yBottom;

    vtxA.tmuvtx[0].sow = vtxC.tmuvtx[0].sow = 0.0f;
    vtxB.tmuvtx[0].sow = vtxD.tmuvtx[0].sow = 255.0f;
    vtxA.tmuvtx[0].tow = vtxB.tmuvtx[0].tow = 0.0f;
    vtxC.tmuvtx[0].tow = vtxD.tmuvtx[0].tow = 255.0f;

    vtxA.r = 255.0f, vtxA.g =   0.0f, vtxA.b =   0.0f, vtxA.a = 255.0f;
    vtxB.r =   0.0f, vtxB.g = 255.0f, vtxB.b =   0.0f, vtxB.a = 128.0f;
    vtxC.r =   0.0f, vtxC.g =   0.0f, vtxC.b = 255.0f, vtxC.a = 128.0f;
    vtxD.r =   0.0f, vtxD.g =   0.0f, vtxD.b =   0.0f, vtxD.a =   0.0f;

    grConstantColorValue(0xFF0000FFUL);
    grTexSource(GR_TMU0,
                grTexMinAddress(GR_TMU0),
                GR_MIPMAPLEVELMASK_BOTH,
                &texture.info);

    grDrawTriangle(&vtxA, &vtxD, &vtxC);
    grDrawTriangle(&vtxA, &vtxB, &vtxD);

    tlConRender();
    tlConClear();
    grBufferSwap(1);
    grFinish();

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
        printf("Cannot open %s\n", filename);

      /* cycle through all mode */
      textureMode++;
      if (textureMode > SPECALPHA) textureMode = DECAL;

      subframe++;
    }
        
    while(tlKbHit()) {
      const char curKey = tlGetCH();

      switch(curKey) {
      case 'e':
      case 'E':
        useCCExt = (!useCCExt);
        if (!glideext.combineext)
          useCCExt = 0;
        break;
      case 'm':
      case 'M':
        textureMode++;
        if (textureMode > SPECALPHA) textureMode = DECAL;
        break;
      case 'b':
      case 'B':
        /* Cycle through some texture sizes */
        if (glideext.canDo2kTexture) {
          needToLoadNewTexture = FXTRUE;
          bigTex = (bigTex + 1) % (TEX_MAX + 1); /* [0,...,TEX_MAX] */
          printf ("%s", texWidthString[bigTex]);
        }
        break;
      case 'u':
      case 'U':
        if (glideext.umaExt) {
          /* Toggle UMA on/off */
          if (umaIsEnabled) {
            grDisable(GR_TEXTURE_UMA_EXT);
            printf ("UMA extension is disabled.\n");
            umaIsEnabled = FXFALSE;
          } else {
            grEnable(GR_TEXTURE_UMA_EXT);
            printf ("UMA extension is enabled.\n");
            umaIsEnabled = FXTRUE;
          }
        }
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
