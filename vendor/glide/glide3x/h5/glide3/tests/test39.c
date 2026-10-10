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

static const char name[]    = "test39";
static const char purpose[] = "Multi-base address test";
static const char usage[]   = "\t-d <screen dump filename>\n"
                              "\t-m [Check grGet(GR_MAX_TEXTURE_SIZE) for max map size]\n"
                              "\t-n <frames>\n"
                              "\t-r <res>\n";

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

  FxU32
    minTexSize = 1,
    maxTexSize = 256;

  const TlVertex3D 
    srcVerts[4] = 
  {
    { -0.5f, 0.0f,  0.5f, 1.0f, 0.0f, 0.0f },
    {  0.5f, 0.0f,  0.5f, 1.0f, 1.0f, 0.0f },
    { -0.5f, 0.0f, -0.5f, 1.0f, 0.0f, 1.0f },
    {  0.5f, 0.0f, -0.5f, 1.0f, 1.0f, 1.0f }
  };

  /* Initialize Glide */
  grGlideInit();
  if ((hwconfig = tlVoodooType()) == 0) {
    printf("Error getting 3Dfx hw type.\n");
    exit(-1);
  }
  /* Process Command Line Arguments */
  while((rv = tlGetOpt(argc, argv, "dmnrx", &match, &remArgs)) != 0) {
    if (rv == -1) {
      printf("Unrecognized command line argument\n");
      printf("%s %s\n", name, usage);
      printf("Available resolutions:\n%s\n",
              tlGetResolutionList());
      exit(-1);
    }

    switch(match) {
    case 'd':
      scrgrab = FXTRUE;
      if (scrgrab) {
        frames = 1;
        strcpy(filename, remArgs[0]);
      }
      break;
    case 'm':
      /* Do we want to test larger sizes if the hw supports it? */
      if (!grGet(GR_MAX_TEXTURE_SIZE, sizeof(maxTexSize), (FxI32 *)&maxTexSize)) {
        printf("grGet(GR_MAX_TEXTURE_SIZE) failed.\n");
        exit(-1);
      }
      break;
    case 'n':
      if (remArgs[0] != NULL) frames = atoi(remArgs[0]);
      break;
    case 'r':
      if (remArgs[0] != NULL) resolution = tlGetResolutionConstant(remArgs[0], 
                                                                   &scrWidth, 
                                                                   &scrHeight);
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
  if (grSstWinOpen(tlGethWnd(),
                   resolution,
                   GR_REFRESH_60Hz,
                   GR_COLORFORMAT_ABGR,
                   GR_ORIGIN_UPPER_LEFT,
                   2, 1) == 0) {
    printf("grSstWinOpen failed.\n");
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

  /* Generate texture */
  {
    GrLOD_t 
      largeLod, 
      smallLod;
    FxU16* 
      texData = NULL;
    FxU32 
      totalMapSize,
      curMapSize;

    /* There's some closed form of this series thing, but I forget
     * what it is and am way to lazy to actually go look it up. This
     * is only a glide test after all.  
     */
    totalMapSize = 0;
    curMapSize = maxTexSize;
    while(curMapSize > 0) {
      totalMapSize += (curMapSize * curMapSize) * sizeof(FxU16);
      curMapSize >>= 1;
    }
    texData = (FxU16*)malloc(totalMapSize * sizeof(*texData));
    if (texData == NULL) {
      printf("Unable to allocate texture data.\n");
      exit(-1);
    }

    /* Figure out the supported log2(lod size) */
    largeLod = GR_LOD_LOG2_1;
    while((0x01UL << largeLod) != maxTexSize) largeLod++;

    smallLod = GR_LOD_LOG2_1;
    while((0x01UL << smallLod) != minTexSize) smallLod++;
    
    /* Build simple texture w/ a different color for each map to help
     * distinguish the different maps when mipmapping. Additionally,
     * add a 'border' so that we can see if the start of the texture
     * gets mucked up due to an incorrect start address.
     */
    {
      FxU16 
        colorVal = 0xF800,
        *curTexPtr = texData;
      FxU32 
        i, j,
        colorShift = (16 / largeLod);

      curMapSize = maxTexSize;
      while(curMapSize > 0) {
        for(j = 0; j < curMapSize; j++) *curTexPtr++ = 0xFFFF;
        for(i = 1; i < curMapSize - 1; i++) {
          *curTexPtr++ = 0xFFFF;
          for(j = 1; j < curMapSize - 1; j++) {
            *curTexPtr++ = colorVal;
          }
          *curTexPtr++ = 0xFFFF;
        }
        for(j = 0; j < curMapSize; j++) *curTexPtr++ = 0xFFFF;
        
        curMapSize >>= 1;
        colorVal >>= colorShift;
      }
    }

    {
      GrTexInfo texInfo = {
        GR_LOD_LOG2_1,
        0,
        GR_ASPECT_LOG2_1x1,
        GR_TEXFMT_RGB_565,
        0
      };
      FxU32 
        curTexAddr = 0x00UL,
        texMultiBaseOffset;
      
      texInfo.largeLodLog2 = largeLod;
      texInfo.data = texData;
      
      /* Determine the hw texture alignment and generate some random
       * offset for the texture base addressing.  
       */
      {
        FxU32 
          texAlign,
          maxTexMem = grTexMaxAddress(GR_TMU0),
          maxTexSize = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, 
                                               &texInfo);

        if (!grGet(GR_TEXTURE_ALIGN, sizeof(texAlign), (FxI32 *)&texAlign)) {
          printf("grGet(GR_TEXTURE_ALIGN): Failed!\n");
          exit(-1);
        }
        texAlign <<= 3UL;

        /* Get some offset that has to be smaller than the amount of
         * texxture space we have left divided by the # of times
         * that we're going to add it in.
         */
        do {
          texMultiBaseOffset = rand();
        } while ((texMultiBaseOffset < 0x1000UL) ||
                 (texMultiBaseOffset > ((maxTexMem - maxTexSize) / 4)));

        texMultiBaseOffset = (texMultiBaseOffset + texAlign) & ~(texAlign - 1UL);
      }

      /* Download the texture to the multibase address specified by
       * the current mode. We play a few games w/ offsetting the
       * texture start address by texMultiBaseOffset to try to make
       * sure that multi-base actually works inside of glide.  
       */

        /* Mmmm... multibase */
        grTexMultibase(GR_TMU0, FXTRUE);

        /* Download and set the base addresses in descending map size
         * order offsetting by some dorky amount as we go.
         */
        texInfo.smallLodLog2 = GR_LOD_LOG2_256;
        texInfo.largeLodLog2 = largeLod;
        
        grTexDownloadMipMap(GR_TMU0, 
                            curTexAddr, 
                            GR_MIPMAPLEVELMASK_BOTH, 
                            &texInfo);
        grTexMultibaseAddress(GR_TMU0, 
                              GR_TEXBASE_256, 
                              curTexAddr, 
                              GR_MIPMAPLEVELMASK_BOTH, 
                              &texInfo);

        curTexAddr += (texMultiBaseOffset +
                       grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, 
                                               &texInfo));
        texInfo.data = (void*)((FxU8*)texInfo.data + 
                               grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, 
                                                       &texInfo));

        texInfo.smallLodLog2 = GR_LOD_LOG2_128;
        texInfo.largeLodLog2 = GR_LOD_LOG2_128;

        grTexDownloadMipMap(GR_TMU0, 
                            curTexAddr, 
                            GR_MIPMAPLEVELMASK_BOTH, 
                            &texInfo);
        grTexMultibaseAddress(GR_TMU0, 
                              GR_TEXBASE_128, 
                              curTexAddr, 
                              GR_MIPMAPLEVELMASK_BOTH, 
                              &texInfo);

        curTexAddr += (texMultiBaseOffset +
                       grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, 
                                               &texInfo));
        texInfo.data = (void*)((FxU8*)texInfo.data + 
                               grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, 
                                                       &texInfo));

        texInfo.smallLodLog2 = GR_LOD_LOG2_64;
        texInfo.largeLodLog2 = GR_LOD_LOG2_64;
        
        grTexDownloadMipMap(GR_TMU0, 
                            curTexAddr, 
                            GR_MIPMAPLEVELMASK_BOTH, 
                            &texInfo);
        grTexMultibaseAddress(GR_TMU0, 
                              GR_TEXBASE_64, 
                              curTexAddr, 
                              GR_MIPMAPLEVELMASK_BOTH, 
                              &texInfo);

        curTexAddr += (texMultiBaseOffset +
                       grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, 
                                               &texInfo));
        texInfo.data = (void*)((FxU8*)texInfo.data + 
                               grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, 
                                                       &texInfo));

        texInfo.smallLodLog2 = GR_LOD_LOG2_1;
        texInfo.largeLodLog2 = GR_LOD_LOG2_32;
        
        grTexDownloadMipMap(GR_TMU0, 
                            curTexAddr, 
                            GR_MIPMAPLEVELMASK_BOTH, 
                            &texInfo);
        grTexMultibaseAddress(GR_TMU0, 
                              GR_TEXBASE_32_TO_1, 
                              curTexAddr, 
                              GR_MIPMAPLEVELMASK_BOTH, 
                              &texInfo);

      /* Source the whole texture. The large map start will reset
       * baseAddr0, but that should be fine since we did not adjust
       * its address because it came first.  
       */
      texInfo.smallLodLog2 = GR_LOD_LOG2_1;
      texInfo.largeLodLog2 = largeLod;
      
      grTexSource(GR_TMU0, 
                  0x00UL, 
                  GR_MIPMAPLEVELMASK_BOTH, 
                  &texInfo);
    }

    free((void*)texData);
  }

  grTexMipMapMode(GR_TMU0,
                  GR_MIPMAP_NEAREST,
                  FXFALSE);

  tlConOutput("Press a key to quit\n");
  while(frames-- && tlOkToRender()) {
    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight);
        
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }

    grBufferClear(0x808080, 0xFF, wrange[1]);

    {
      static float
        distance = 1.0f,
        dDelta   = 0.1f;
      TlVertex3D 
        xfVerts[4],
        prjVerts[4];
      GrVertex 
        vtxA, 
        vtxB, 
        vtxC,
        vtxD;

      /*---- 
        A-B
        |\|
        C-D
        -----*/
      vtxA.oow = 1.0f;
      vtxB = vtxC = vtxD = vtxA;

#define MAX_DIST 30.5f
#define MIN_DIST 1.0f

      distance += dDelta;
      if (distance > MAX_DIST || 
           distance < MIN_DIST) { 
        dDelta *= -1.0f;
        distance += dDelta;
      }

      tlSetMatrix(tlIdentity());
      tlMultMatrix(tlXRotation(-20.0f));
      tlMultMatrix(tlTranslation(0.0f, -0.3f, distance));

      tlTransformVertices(xfVerts, srcVerts, 4);
      tlProjectVertices(prjVerts, xfVerts, 4);

      vtxA.x = tlScaleX(prjVerts[0].x);
      vtxA.y = tlScaleY(prjVerts[0].y);
      vtxA.oow = 1.0f / prjVerts[0].w;
      vtxA.tmuvtx[0].sow = prjVerts[0].s * 255.0f * vtxA.oow;
      vtxA.tmuvtx[0].tow = prjVerts[0].t * 255.0f * vtxA.oow;

      vtxB.x = tlScaleX(prjVerts[1].x);
      vtxB.y = tlScaleY(prjVerts[1].y); 
      vtxB.oow = 1.0f / prjVerts[1].w;
      vtxB.tmuvtx[0].sow = prjVerts[1].s * 255.0f * vtxB.oow;
      vtxB.tmuvtx[0].tow = prjVerts[1].t * 255.0f * vtxB.oow;
        
      vtxC.x = tlScaleX(prjVerts[2].x);
      vtxC.y = tlScaleY(prjVerts[2].y);
      vtxC.oow = 1.0f / prjVerts[2].w;
      vtxC.tmuvtx[0].sow = prjVerts[2].s * 255.0f * vtxC.oow;
      vtxC.tmuvtx[0].tow = prjVerts[2].t * 255.0f * vtxC.oow;

      vtxD.x = tlScaleX(prjVerts[3].x); 
      vtxD.y = tlScaleY(prjVerts[3].y);
      vtxD.oow = 1.0f / prjVerts[3].w;
      vtxD.tmuvtx[0].sow = prjVerts[3].s * 255.0f * vtxD.oow;
      vtxD.tmuvtx[0].tow = prjVerts[3].t * 255.0f * vtxD.oow;

      grDrawTriangle(&vtxA, &vtxB, &vtxD);
      grDrawTriangle(&vtxA, &vtxD, &vtxC);
    }

    tlConRender();
    grBufferSwap(1);

    /* grab the frame buffer */
    if (scrgrab) {
      if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight))
        printf("Cannot open %s\n", filename);
      scrgrab = FXFALSE;
    }

    if (tlKbHit()) {
      char curKey = tlGetCH();

      switch(curKey) {
      default:
        frames = 0;
        break;
      }
    }
  }
  rv = 0;

 __errExit:    
  grGlideShutdown();

  return rv;
}
