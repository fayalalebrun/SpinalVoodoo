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
#include <string.h>
#include <assert.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"


static const char name[]    = "test40";
static const char purpose[] = "mipmap download/source test";
static const char usage[]   = "-f <3df filename> -n <frames> -r <res> -d <filename>";

static struct texInfo {
  const char* 
    baseName;
  TlTexture 
    texData;
  FxU32 
    paletteStart, paletteEnd;
  GrLOD_t
    largeLodLog2, smallLodLog2;
} baseTexList[3], *curBaseTexture;
static GrTexInfo* lodMap = NULL;

static const char kDefaultTexFileName[] = "miro.3df";
static const char* texFileName = kDefaultTexFileName;

static FxBool
setupBaseTextures(void);
static FxBool
rebuildTextureMap(struct texInfo* texMap,
                  GrAspectRatio_t aspectRatioLog2,
                  GrTextureFormat_t format);
static FxBool
rebuildLodMap(struct texInfo* texMap);

int 
main(int argc, char **argv) 
{
  GrScreenResolution_t 
    resolution = GR_RESOLUTION_640x480;
  float
    scrWidth   = 640.0f,
    scrHeight  = 480.0f;
  int 
    frames                      = -1;
  FxBool
    scrgrab = FXFALSE;
  char
    filename[256];

  /* Process Command Line Arguments */
  {
    char match; 
    char **remArgs;
    int  rv;

    while((rv = tlGetOpt(argc, argv, "dfnr", &match, &remArgs)) != 0) {
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
        frames = 5;
        strcpy(filename, remArgs[0]);
        break;

      case 'f':
        texFileName = remArgs[0];
        break;

      case 'n':
        frames = atoi(remArgs[0]);
        break;

      case 'r':
        resolution = tlGetResolutionConstant(remArgs[0], 
                                             &scrWidth, 
                                             &scrHeight);
        break;
      }
    }
  }

  /* Initialize Glide */
  grGlideInit();
    
  {
    FxU32
      numBoards = 0;

    if (!grGet(GR_NUM_BOARDS, sizeof(numBoards), (FxI32 *)&numBoards) ||
        (numBoards == 0)) {
      fprintf(stderr, "no boards in system\n");
      goto __errExit;
    }
  }
  grSstSelect(0);

  printf("%s:\n%s\n", name, purpose);
  printf("%s : %s\n", grGetString(GR_HARDWARE), grGetString(GR_VERSION));
  printf("Resolution: %s\n", tlGetResolutionString(resolution));
  if (frames == -1) {
    printf("Press A Key To Begin Test.\n");
    tlGetCH();
  }

  if (scrWidth < 512.0f) {
    fprintf(stderr, "Cannot run w/ a screen width < 512\n");
    goto __errExit;
  }
  tlSetScreen(scrWidth, scrHeight);

  if (!grSstWinOpen(tlGethWnd(),
                    resolution,
                    GR_REFRESH_60Hz,
                    GR_COLORFORMAT_ABGR,
                    GR_ORIGIN_UPPER_LEFT,
                    2, 1)) {
    fprintf(stderr, "grSstWinOpen failed (0x%X)\n", resolution);
    goto __errExit;
  }

  tlVertexLayout();    
  tlConSet(0.0f, 0.0f, 1.0f, 0.5f, 
           60, 15, 0xffffff);

  if (!setupBaseTextures()) goto __errExit;
    
  /* Set up Render State - Decal Texture */
  grTexCombine(GR_TMU0,
               GR_COMBINE_FUNCTION_LOCAL,
               GR_COMBINE_FACTOR_NONE,
               GR_COMBINE_FUNCTION_NONE,
               GR_COMBINE_FACTOR_NONE,
               FXFALSE, FXFALSE);
  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_NONE,
                 GR_COMBINE_OTHER_TEXTURE,
                 FXFALSE);

  grTexMipMapMode(GR_TMU0,
                  GR_MIPMAP_NEAREST,
                  FXFALSE);

  /* No filtering for the texture blowup so that we
   * might be able to see texel mistakes in the download.
   */
  grTexFilterMode(GR_TMU0,
                  GR_TEXTUREFILTER_POINT_SAMPLED,
                  GR_TEXTUREFILTER_POINT_SAMPLED);

  
  {
    GrLOD_t
      curLod = curBaseTexture->texData.info.largeLodLog2;
    float
      lodBias = 0.0f;
    FxBool
      updateP = FXTRUE;
    FxU32
      zDepthRange[2];

    if (!grGet(GR_ZDEPTH_MIN_MAX, sizeof(zDepthRange), (FxI32 *)zDepthRange)) {
      fprintf(stderr, "Query for z-depth min/max failed");
      goto __errExit;
    }
    
    while(frames-- && tlOkToRender()) {
      GrVertex 
        vtxA, vtxB, vtxC, vtxD;

      grBufferClear(0x444444UL, 0, zDepthRange[1]);

      tlConClear();
      tlConOutput("b: base texture (%s)\n", curBaseTexture->baseName);
      tlConOutput("l: largeLodLog2 (0x%X)\n", (0x01UL << curLod));
      tlConOutput("s: smallLodLog2 (0x%X)\n", (0x01UL << curBaseTexture->smallLodLog2));
      {
        FxU32
          w = 0x01UL, 
          h = 0x01UL;

        if (curBaseTexture->texData.info.aspectRatioLog2 < GR_ASPECT_LOG2_1x1) {
          w <<= -curBaseTexture->texData.info.aspectRatioLog2;
        } else if (curBaseTexture->texData.info.aspectRatioLog2 > GR_ASPECT_LOG2_1x1) {
          h <<= curBaseTexture->texData.info.aspectRatioLog2;
        }
        tlConOutput("a: aspectRatioLog2 (%ld x %ld)\n", w, h);
      }
      tlConOutput("d: displayed (0x%X)\n", (0x01UL << (FxU32)(curLod + lodBias)));
      tlConOutput("Press any other key to quit\n\n");

      if (updateP) {
        GrTexInfo*
          curTexInfo = lodMap + (curBaseTexture->texData.info.largeLodLog2 - curLod);

        grTexDownloadMipMap(GR_TMU0,
                            0x00UL,
                            GR_MIPMAPLEVELMASK_BOTH,
                            curTexInfo);
        grTexSource(GR_TMU0,
                    0x00UL,
                    GR_MIPMAPLEVELMASK_BOTH,
                    curTexInfo);

        if (curBaseTexture->texData.tableType != NO_TABLE) {
          grTexDownloadTablePartial(curBaseTexture->texData.tableType,
                                    (void*)&curBaseTexture->texData.tableData,
                                    curBaseTexture->paletteStart, curBaseTexture->paletteEnd);
        }

        grTexLodBiasValue(GR_TMU0, lodBias);

        updateP = FXFALSE;
      }
      
      /* A-B
       * |\|
       * C-D
       */
      vtxA.oow =
      vtxB.oow = 
      vtxC.oow =
      vtxD.oow = 1.0f;

      vtxA.r = 255.0f, vtxA.g =   0.0f, vtxA.b =   0.0f, vtxA.a = 255.0f;
      vtxB.r =   0.0f, vtxB.g = 255.0f, vtxB.b =   0.0f, vtxB.a = 128.0f;
      vtxC.r =   0.0f, vtxC.g =   0.0f, vtxC.b = 255.0f, vtxC.a = 128.0f;
      vtxD.r =   0.0f, vtxD.g =   0.0f, vtxD.b =   0.0f, vtxD.a =   0.0f;
      
      {
        const FxU32
          aspectFactorWidth = ((curBaseTexture->texData.info.aspectRatioLog2 > GR_ASPECT_LOG2_1x1)
                               ? (0x01UL << curBaseTexture->texData.info.aspectRatioLog2)
                               : 0x001UL),
          aspectFactorHeight = ((curBaseTexture->texData.info.aspectRatioLog2 < GR_ASPECT_LOG2_1x1)
                                ? (0x01UL << -curBaseTexture->texData.info.aspectRatioLog2)
                                : 0x01UL);
        FxU32
          texWidth = ((0x01UL << curLod) / aspectFactorWidth),
          texHeight = ((0x01UL << curLod) / aspectFactorHeight);
        const float
          imageWidth = (255.0f / aspectFactorWidth),
          imageHeight = (255.0f / aspectFactorHeight),
          gap = (scrWidth - texWidth - imageWidth) / 3.0f;

        if (texWidth == 0x00UL) texWidth = 1UL;
        if (texHeight == 0x00UL) texHeight = 1UL;

        vtxA.x = vtxC.x = gap;
        vtxB.x = vtxD.x = gap + texWidth;
        vtxA.y = vtxB.y = tlScaleY(0.2f);
        vtxC.y = vtxD.y = tlScaleY(0.2f) + texHeight;
                            
        vtxA.tmuvtx[0].sow = vtxC.tmuvtx[0].sow = 0.0f;
        vtxB.tmuvtx[0].sow = vtxD.tmuvtx[0].sow = 255.0f / aspectFactorWidth;
        vtxA.tmuvtx[0].tow = vtxB.tmuvtx[0].tow = 0.0f;
        vtxC.tmuvtx[0].tow = vtxD.tmuvtx[0].tow = 255.0f / aspectFactorHeight;

        grDrawTriangle(&vtxA, &vtxD, &vtxC);
        grDrawTriangle(&vtxA, &vtxB, &vtxD);

        vtxA.x = vtxC.x = vtxD.x + gap;
        vtxB.x = vtxD.x = vtxA.x + imageWidth;
        vtxA.y = vtxB.y = tlScaleY(0.2f);
        vtxC.y = vtxD.y = vtxA.y + imageHeight;

        grDrawTriangle(&vtxA, &vtxD, &vtxC);
        grDrawTriangle(&vtxA, &vtxB, &vtxD);
      }      

      grConstantColorValue(0x00ff0000);

      tlConRender();
      grBufferSwap(1);
      grFinish();

      /* grab the frame buffer */
      if (scrgrab) {
        if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight)) {
          fprintf(stderr, "Cannot open %s\n", filename);
          goto __errExit;
        }
      }
        
      while(tlKbHit()) {
        updateP = FXTRUE;

        switch(tlGetCH()) {
        case 'b':
          curBaseTexture++;
          if (curBaseTexture >= baseTexList + (sizeof(baseTexList) / sizeof(baseTexList[0]))) 
            curBaseTexture = baseTexList;
          rebuildLodMap(curBaseTexture);

          curLod = curBaseTexture->texData.info.largeLodLog2;
          curBaseTexture->smallLodLog2 = curBaseTexture->texData.info.smallLodLog2;
          lodBias = 0.0f;
          break;

        case 'l':
          curLod--;
          if (curLod < curBaseTexture->texData.info.smallLodLog2) curLod = curBaseTexture->texData.info.largeLodLog2;
          if (curLod < curBaseTexture->smallLodLog2) {
            curBaseTexture->smallLodLog2 = curLod;
            goto __smallLodLog2Propogate;
          }
          break;
          
        case 's':
          {
            FxI32
              i;

            curBaseTexture->smallLodLog2++;
            if (curBaseTexture->smallLodLog2 > curBaseTexture->texData.info.largeLodLog2) 
              curBaseTexture->smallLodLog2 = curBaseTexture->texData.info.smallLodLog2;
            
        __smallLodLog2Propogate:
            for(i = 0; 
                i < curBaseTexture->texData.info.largeLodLog2 - curBaseTexture->texData.info.smallLodLog2 + 1; 
                i++) 
              lodMap[i].smallLodLog2 = curBaseTexture->smallLodLog2;
          }
          break;

        case 'a':
          if (curBaseTexture != baseTexList) {
            curBaseTexture->texData.info.aspectRatioLog2++;
            if (curBaseTexture->texData.info.aspectRatioLog2 > GR_ASPECT_LOG2_8x1) 
              curBaseTexture->texData.info.aspectRatioLog2 = GR_ASPECT_LOG2_1x8;
            rebuildTextureMap(curBaseTexture,
                              curBaseTexture->texData.info.aspectRatioLog2,
                              curBaseTexture->texData.info.format);
          }
          break;

        case 'd':
          lodBias -= 1.0f;
          if (lodBias < (float)(curLod - curBaseTexture->smallLodLog2)) lodBias = 0.0f;
          break;
          
        default:
          frames = 0;
          break;
        }
      }
    }
  }

 __errExit:    
  grGlideShutdown();
  return 0;
}

static FxBool
setupBaseTextures(void)
{
  FxBool
    retVal = FXFALSE;
  FxU32
    i;

  /* Load 3df file for testing */
  while (!retVal) {
    retVal = tlLoadTexture(texFileName, 
                           &baseTexList[0].texData.info, 
                           &baseTexList[0].texData.tableType, 
                           &baseTexList[0].texData.tableData);
    if (retVal) {
      baseTexList[0].baseName = texFileName;

      baseTexList[0].smallLodLog2 = baseTexList[0].texData.info.smallLodLog2;
      baseTexList[0].largeLodLog2 = baseTexList[0].texData.info.largeLodLog2;
    } else {
      fprintf(stderr, "Could not load texture file: %s\n", texFileName);
      if (texFileName != kDefaultTexFileName) {
        texFileName = kDefaultTexFileName;
        fprintf(stderr, "Trying default texture file: %s\n", texFileName);
      } else {
        break;
      }
    }
  }
  if (!retVal) goto __errExit;

  for(i = 1; i < 3; i++) {
    struct {
      GrTextureFormat_t format;
      const char* baseName;
    } texFormatList[] = {
      { GR_TEXFMT_P_8, "Paletted 8" },
      { GR_TEXFMT_RGB_565, "RGB 565" }
    };

    baseTexList[i].baseName = texFormatList[i - 1].baseName;
    retVal = rebuildTextureMap(baseTexList + i,
                               GR_ASPECT_LOG2_1x1,
                               texFormatList[i - 1].format);
    if (!retVal) break;
  }
  if (!retVal) goto __errExit;

  curBaseTexture = baseTexList + 0;
  if (!rebuildLodMap(curBaseTexture)) goto __errExit;

 __errExit:
  if (!retVal) {

  }

  return retVal;
}

/* Deal w/ the aspect ratio whackiness which becomes linear if the
 * lod is smaller than the aspect ratio shift and also clamps to a
 * minimum size of 1 texel to avoid aliasing artifacts.
 */
static void
getTexLevelDims(GrLOD_t lod, GrAspectRatio_t aspectRatio, 
                FxU32* texWidthPtr, FxU32* texHeightPtr)
{
  const FxU32
    texEdge = (0x01UL << lod);
  FxU32
    texWidth = texEdge, 
    texHeight = texEdge;
  
  if (aspectRatio > GR_ASPECT_LOG2_1x1) {
    if (aspectRatio > lod) {
      texWidth >>= 1UL;
    } else {
      texWidth >>= aspectRatio;
    }
    
    if (texWidth == 0x00UL) texWidth = 0x01UL;
  } else if (aspectRatio < GR_ASPECT_LOG2_1x1) {
    aspectRatio *= -1;

    if (aspectRatio > lod) {
      texHeight >>= 1UL;
    } else {
      texHeight >>= aspectRatio;
    }
    
    if (texHeight == 0x00UL) texHeight = 0x01UL;
  }

  *texWidthPtr = texWidth;
  *texHeightPtr = texHeight;
}

static FxBool
rebuildTextureMap(struct texInfo* texMap,
                  GrAspectRatio_t aspectRatio,
                  GrTextureFormat_t format)
{
  FxBool
    retVal = FXTRUE;
  GrLOD_t 
    curLod;
  FxU8*
    texPtr;

  texMap->texData.info.aspectRatioLog2 = aspectRatio;
  texMap->texData.info.format = format;
  
  /* Allocate memory for an entire mipmap w/ a 1x1 aspect ratio */
  if (texMap->texData.info.data == NULL) {
    FxU32 
      texSize = 0x00UL;
    
    texMap->texData.info.largeLodLog2 = GR_LOD_LOG2_256;
    texMap->texData.info.smallLodLog2 = GR_LOD_LOG2_1;
    
    /* Allocates too much memory for non 1x1 textures, but that's safe */
    for(curLod = texMap->texData.info.smallLodLog2; curLod <= texMap->texData.info.largeLodLog2; curLod++) {
      const FxU32
        texEdge = (0x01UL << curLod);
      
      texSize += ((texEdge * texEdge) << (format >= GR_TEXFMT_16BIT));
    }
    texMap->texData.info.data = malloc(texSize);
    retVal = (texMap->texData.info.data != NULL);
    if (!retVal) {
      fprintf(stderr, "Could not allocate texture memory (0x%X : 0x%X) : 0x%X\n",
              texMap->texData.info.smallLodLog2, texMap->texData.info.largeLodLog2, texSize);
      goto __errExit;
    }

    /* Setup the palette once */
    if (format == GR_TEXFMT_P_8) {
      static const FxU32 texPalette[] = {
        0x3F3F3FUL, /* 256 : 0 */
        0x7F7F7FUL, /* 128 : 1 */
        0xFFFFFFUL, /* 64  : 2 */
        0xFFFF00UL, /* 32  : 3 */
        0xFF00FFUL, /* 16  : 4 */
        0xFF0000UL, /* 8   : 5 */
        0x00FFFFUL, /* 4   : 6 */
        0x00FF00UL, /* 2   : 7 */
        0x0000FFUL  /* 1   : 8 */
      };
      FxU32
        i;
      
      texMap->texData.tableType = GR_TEXTABLE_PALETTE;
      texMap->paletteStart = 0;
      texMap->paletteEnd = (sizeof(texPalette) / sizeof(texPalette[0])) - 1;
      
      for(i = texMap->paletteStart; i <= texMap->paletteEnd; i++) {
        texMap->texData.tableData.palette[i] = texPalette[i];
      }
    } else {
      texMap->texData.tableType = NO_TABLE;
    }
  }
  texPtr = (FxU8*)texMap->texData.info.data;

  /* Initially download and source the entire chain */
  texMap->largeLodLog2 = texMap->texData.info.largeLodLog2;
  texMap->smallLodLog2 = texMap->texData.info.smallLodLog2;
  
  for(curLod = texMap->texData.info.largeLodLog2; curLod >= texMap->texData.info.smallLodLog2; curLod--) {
    FxU32
      texHeight, texWidth,
      i, j;

    getTexLevelDims(curLod, aspectRatio,
                    &texWidth, &texHeight);
    
    for(i = 0; i < texHeight; i++) {
      for(j = 0; j < texWidth; j++) {
        const FxU32 
          texIndex = i * texWidth + j,
          texelValIndex = texMap->texData.info.largeLodLog2 - curLod;

        if (format == GR_TEXFMT_P_8) {
          texPtr[texIndex] =  (FxU8)texelValIndex;
        } else {
          static const FxU16 
            texels[]  = {
              0x39E7, /* 256 : 0 */
              0x7BEF, /* 128 : 1 */
              0xFFFF, /* 64  : 2 */
              0xFF80, /* 32  : 3 */
              0xF81F, /* 16  : 4 */
              0xF800, /* 8   : 5 */
              0x07FF, /* 4   : 6 */
              0x07E0, /* 2   : 7 */
              0x001F  /* 1   : 8 */
            };
          
          ((FxU16*)texPtr)[texIndex] = texels[texelValIndex];
        }
      }
    }

    texPtr += ((texWidth * texHeight) << (format >= GR_TEXFMT_16BIT));
  }

  rebuildLodMap(texMap);

 __errExit:
  return retVal;
}

static FxBool
rebuildLodMap(struct texInfo* texMap)
{
  FxBool
    retVal = FXFALSE;

  /* Allocate the maximally sized lod map */
  if (lodMap == NULL) {
    lodMap = (GrTexInfo*)malloc((GR_LOD_LOG2_256 - GR_LOD_LOG2_1 + 1) * sizeof(GrTexInfo));
    retVal = (lodMap != NULL);
    if (!retVal) {
      fprintf(stderr, "Could not allocate lod map");

      goto __errExit;
    }
  }

  /* Convert the loaded texture into a series of independent maps */
  {
    const FxU8*
      curTexPtr = (const FxU8*)texMap->texData.info.data;
    GrLOD_t 
      curLod;
    int 
      i;

    for(i = 0, curLod = texMap->texData.info.largeLodLog2; 
        curLod >= texMap->texData.info.smallLodLog2; 
        i++, curLod--) {
      FxU32
        texWidth, texHeight;

      lodMap[i].largeLodLog2 = curLod;
      lodMap[i].smallLodLog2 = texMap->smallLodLog2;

      lodMap[i].aspectRatioLog2 = texMap->texData.info.aspectRatioLog2;
      lodMap[i].format = texMap->texData.info.format;

      lodMap[i].data = (void*)curTexPtr;

      getTexLevelDims(curLod, texMap->texData.info.aspectRatioLog2,
                      &texWidth, &texHeight);

      curTexPtr += ((texHeight * texWidth) << (texMap->texData.info.format >= GR_TEXFMT_16BIT));
    }
  }

 __errExit:
  return retVal;
}

