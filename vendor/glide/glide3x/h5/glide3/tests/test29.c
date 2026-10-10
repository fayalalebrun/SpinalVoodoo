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

static const char name[]    = "test29";
static const char purpose[] = "Simple palette texture test";
static const char usage[]   = "\t-d <screen dump filename>\n"
                              "\t-l <small lod log2>,<large lod log2>\n"
                              "\t-m [check for mirror extension]\n"
                              "\t-n <frames>\n"
                              "\t-p [check for 6666 palette extension]\n"
                              "\t-r <res>\n"
                              "\t-s [force single texturing]\n";
                              

int
main(int argc, char **argv) 
{
  char match; 
  char **remArgs;
  int  rv = -1;

  GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
  float                scrWidth   = 640.0f;
  float                scrHeight  = 480.0f;
  int frames                      = -1;
  FxBool               scrgrab = FXFALSE;
  char                 filename[256];
  FxU32                wrange[2];

  GrLOD_t              largeLod   = GR_LOD_LOG2_256;
  GrLOD_t              smallLod   = GR_LOD_LOG2_1;
  GrTexTable_t         curPalType = GR_TEXTABLE_PALETTE;
  FxBool               singleTexP = FXFALSE;
  FxBool               extMirrorP = FXFALSE;
  FxBool               extPalP    = FXFALSE;
  float                minTexCoor = 0.0f;
  float                maxTexCoor = 256.0f;
  GrContext_t          gc = 0;

  /* Initialize Glide */
  grGlideInit();
  assert(hwconfig = tlVoodooType());

  /* Process Command Line Arguments */
  while ((rv = tlGetOpt(argc, argv, "dlmnprs", &match, &remArgs)) != 0) {
    if (rv == -1) {
      printf("Unrecognized command line argument\n");
      printf("%s %s\n", name, usage);
      printf("Available resolutions:\n%s\n",
              tlGetResolutionList());
      return -1;
    }
    switch(match) {
    case 'd':
      scrgrab = FXTRUE;
      frames = 1;
      strcpy(filename, remArgs[0]);
      break;
    case 'l':
      if (sscanf(remArgs[0], "%d,%d", &smallLod, &largeLod) != 2) {
        largeLod = GR_LOD_LOG2_256;
        smallLod = GR_LOD_LOG2_1;
      } else {
        GrLOD_t
          maxLod,
          lodShift;

        if (grGet(GR_MAX_TEXTURE_SIZE, sizeof(maxLod), &maxLod) == 0) {
          largeLod = GR_LOD_LOG2_256;
          smallLod = GR_LOD_LOG2_1;
        } else {
          lodShift = 0;
          while((0x01 << lodShift) != maxLod) lodShift++;
          if (largeLod > lodShift) largeLod = lodShift;
        }
      }
      break;
    case 'm':
      extMirrorP = FXTRUE;
      break;
    case 'n':
      frames = atoi(remArgs[0]);
      break;
    case 'p':
      extPalP = FXTRUE;
      break;
    case 'r':
      resolution = tlGetResolutionConstant(remArgs[0], 
                                           &scrWidth, 
                                           &scrHeight);
      break;
    case 's':
      singleTexP = FXTRUE;
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
    
  grSstSelect(0);
  gc = grSstWinOpen(tlGethWnd(),
                    resolution,
                    GR_REFRESH_60Hz,
                    GR_COLORFORMAT_ABGR,
                    GR_ORIGIN_UPPER_LEFT,
                    2, 1);
  if (!gc) {
    printf("Could not allocate glide fullscreen context.\n");
    goto __errExit;
  }
    
  tlConSet(0.0f, 0.0f, 1.0f, 1.0f, 
            60, 30, 0xffffff);

  /* Set up Render State */
  grGet(GR_WDEPTH_MIN_MAX, 8, (FxI32 *)wrange);  
  grVertexLayout(GR_PARAM_XY,  0, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);

  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_CONSTANT,
                 GR_COMBINE_OTHER_TEXTURE,
                 FXFALSE);
  grTexCombine(GR_TMU0,
               GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ONE,
               GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ONE,
               FXFALSE, FXFALSE);

  /* Load texture data into system ram */
  if (!singleTexP) {
    FxI32 numTmu = 0;

    if (grGet(GR_NUM_TMU, sizeof(numTmu), &numTmu) && (numTmu > 1)) {
      TlTexture  texture;
      
      assert(tlLoadTexture("decal1.3df", 
                           &texture.info, 
                           &texture.tableType, 
                           &texture.tableData));
      /* Download texture data to TMU */
      grTexDownloadMipMap(GR_TMU1,
                          grTexMinAddress(GR_TMU1),
                          GR_MIPMAPLEVELMASK_BOTH,
                          &texture.info);
      if (texture.tableType != NO_TABLE) {
        grTexDownloadTable(texture.tableType,
                           &texture.tableData);
      }
      grTexSource(GR_TMU1,
                  grTexMinAddress(GR_TMU1),
                  GR_MIPMAPLEVELMASK_BOTH,
                  &texture.info);

      grVertexLayout(GR_PARAM_ST1, GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_ENABLE);

      grTexCombine(GR_TMU1,
                   GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ONE,
                   GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ONE,
                   FXFALSE, FXFALSE);
      grTexCombine(GR_TMU0,
                   GR_COMBINE_FUNCTION_BLEND_OTHER, GR_COMBINE_FACTOR_LOCAL,
                   GR_COMBINE_FUNCTION_BLEND_OTHER, GR_COMBINE_FACTOR_LOCAL,
                   FXFALSE, FXFALSE);
    }    
  }

  if (extMirrorP || extPalP) {
    const char* extString = grGetString(GR_EXTENSION);
    const char* tempStr = extString;
    
    while(tempStr != NULL) {
      const char* keyList[] = {
        "TEXMIRROR",
        "PALETTE6666"
      };
      
      if (extMirrorP && 
          !singleTexP && 
          (strncmp(tempStr, keyList[0], strlen(keyList[0])) == 0)) {
        grTexClampMode(GR_TMU0, 
                       GR_TEXTURECLAMP_MIRROR_EXT, 
                       GR_TEXTURECLAMP_MIRROR_EXT);
        minTexCoor = -256.0f;
        maxTexCoor =  512.0f;
      }
      
      if (extPalP && 
          (strncmp(tempStr, keyList[1], strlen(keyList[1])) == 0)) {
        curPalType = GR_TEXTABLE_PALETTE_6666_EXT;
      }
      
      tempStr = strchr(tempStr, ' ');
      if (tempStr != NULL) tempStr++;
    }
  }
  
  {
    GuTexPalette texPal;
    GrTexInfo texInfo;
    unsigned char* texData = NULL;
    int i, j, k;

    /* Build mindlessly simple ramp palette */
    i = 0;
    while(i < 256) {
      texPal.data[i] = ((i << (8 * 3)) |
                        (i << (8 * 2)) |
                        (i << (8 * 1)) |
                        (i << (8 * 0)));
      i++;
    }

    /* Download this in a sequential way to test the palette download */
    i = 0;
    while(i < 256) {
      const int max = i + (rand() % 10);
      const int end = MIN(max, 255);

      grTexDownloadTablePartial(curPalType, &texPal, i, end);
      i = end + 1;
    }

    i = 0;
    for(k = largeLod; k >= smallLod; k--) {
      i += ((0x01 << k) * (0x01 << k));
    }
    texData = (unsigned char*)malloc(i * 2);
    if (texData == NULL) {
      printf("Unable to allocate texture data.\n");
      goto __errExit;
    }

    /* Download dumb palettized texture */
    texInfo.smallLodLog2 = smallLod;
    texInfo.largeLodLog2 = largeLod;
    texInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
    texInfo.format = GR_TEXFMT_P_8;
    texInfo.data = (void*)texData;
    
    for(k = texInfo.largeLodLog2; k >= texInfo.smallLodLog2; k--) {
      const int texDim = (0x01 << k);
      const float levelStep = (256.0f / texDim);

      for(i = 0; i < texDim; i++) {
        for(j = 0; j < texDim; j++) {
          *texData++ = (FxU8)(levelStep * i); 
        }
      }
    }

    grTexDownloadMipMap(GR_TMU0, 0, GR_MIPMAPLEVELMASK_BOTH, &texInfo);
    grTexSource(GR_TMU0, 0, GR_MIPMAPLEVELMASK_BOTH, &texInfo);

    free(texInfo.data);
  }
  
  tlConOutput("Press a key to quit\n");
  while(frames-- && tlOkToRender()) {
    GrVertex vtxA, vtxB, vtxC;

    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight);
        
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }

    grBufferClear(0xFF00FF00, 0xFF, wrange[1]);

    /* Triangle w/ texture ramp going across and down. */
    vtxA.oow = 
    vtxB.oow =
    vtxC.oow = 1.0f;

    {
      vtxA.x = tlScaleX(0.2f), vtxA.y = tlScaleY(0.2f);
      vtxA.tmuvtx[0].sow = minTexCoor;
      vtxA.tmuvtx[0].tow = minTexCoor;
      vtxA.tmuvtx[1].sow = 0.0f;
      vtxA.tmuvtx[1].tow = 0.0f;
      
      vtxB.x = tlScaleX(0.8f), vtxB.y = tlScaleY(0.2f);
      vtxB.tmuvtx[0].sow = maxTexCoor;
      vtxB.tmuvtx[0].tow = minTexCoor;
      vtxB.tmuvtx[1].sow = 255.0f;
      vtxB.tmuvtx[1].tow = 0.0f;
      
      vtxC.x = tlScaleX(0.8f), vtxC.y = tlScaleY(0.8f);
      vtxC.tmuvtx[0].sow = maxTexCoor;
      vtxC.tmuvtx[0].tow = maxTexCoor;
      vtxC.tmuvtx[1].sow = 255.0f;
      vtxC.tmuvtx[1].tow = 255.0f;

      grDrawTriangle(&vtxA, &vtxB, &vtxC);
    }

    {
      vtxA.x = tlScaleX(0.2f), vtxA.y = tlScaleY(0.2f);
      vtxA.tmuvtx[0].sow = minTexCoor;
      vtxA.tmuvtx[0].tow = minTexCoor;
      vtxA.tmuvtx[1].sow = 0.0f;
      vtxA.tmuvtx[1].tow = 0.0f;
      
      vtxB.x = tlScaleX(0.8f), vtxB.y = tlScaleY(0.8f);
      vtxB.tmuvtx[0].sow = maxTexCoor;
      vtxB.tmuvtx[0].tow = maxTexCoor;
      vtxB.tmuvtx[1].sow = 255.0f;
      vtxB.tmuvtx[1].tow = 255.0f;
      
      vtxC.x = tlScaleX(0.2f), vtxC.y = tlScaleY(0.8f);
      vtxC.tmuvtx[0].sow = minTexCoor;
      vtxC.tmuvtx[0].tow = maxTexCoor;
      vtxC.tmuvtx[1].sow = 0.0f;
      vtxC.tmuvtx[1].tow = 255.0f;
      
      grDrawTriangle(&vtxA, &vtxB, &vtxC);
    }

    tlConRender();
    grBufferSwap(1);

    /* grab the frame buffer */
    if (scrgrab) {
      if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight))
        printf("Cannot open %s\n", filename);
      scrgrab = FXFALSE;
    }

    if (tlKbHit()) frames = 0;
  }
  rv = 0;

__errExit:    
  grGlideShutdown();

  return rv;
}
