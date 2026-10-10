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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"

/* Video timing data structure 
 *
 * NB: This structure definition should be used w/ care since most
 * monitors will not correctly be able to handle arbitrary settings of
 * the fields in this structure.
 */
typedef struct {
    FxU32 hSyncOn;
    FxU32 hSyncOff;
    FxU32 vSyncOn;
    FxU32 vSyncOff;
    FxU32 hBackPorch;
    FxU32 vBackPorch;
    FxU32 xDimension;
    FxU32 yDimension;
    FxU32 refreshRate;
    FxU32 miscCtrl;
    FxU32 memOffset;
    FxU32 tilesInX;
    FxU32 vFifoThreshold;
    FxBool video16BPPIsOK;
    FxBool video24BPPIsOK;
    float clkFreq16bpp;
    float clkFreq24bpp;
} FxVideoTimingInfo;

/* this is semi-undocumented external for arcade developers */
FX_ENTRY void FX_CALL
grSstVidMode(FxU32 whichSst, FxVideoTimingInfo* vidTimings);

/* VESA Standard - 640x480 60Hz */
static FxVideoTimingInfo
gDefaultTimingVal = {
    96,        /* hSyncOn */
    704,       /* hSyncOff */
    2,         /* vSyncOn */
    523,       /* vSyncOff */
    38,        /* hBackPorch */
    25,        /* vBackPorch */
    640,       /* xDimension */
    480,       /* yDimension */
    60,        /* refreshRate */
    0,         /* miscCtrl */
    150,       /* memOffset */
    20,        /* tilesInX */
    25,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    25.175F,   /* clkFreq16bpp */
    50.350F    /* clkFreq24bpp */
};

int hwconfig;
static const char *version;

static const char name[]    = "test30";
static const char purpose[] = "Glide resolution/refresh rate/buffer test";
static const char usage[]   = "-n <frames> -d <filename> -v (use vid timing)";

static void
XLateTri(GrVertex* a, GrVertex* b, GrVertex* c,
         const float theta, const float scale,
         const float x, const float y, const float z);

static void  
OutputPromptMessage(GrScreenResolution_t conRes,
                    GrScreenResolution_t res, GrScreenRefresh_t refresh,
                    int colorBuf, int auxBuf,
                    const FxVideoTimingInfo* vidTiming,
                    FxBool errorP);

static FxBool
ProcessPromptKey(int* frames, 
                 GrScreenResolution_t* res,
                 GrScreenRefresh_t* refresh,
                 int* colorBuf,
                 int* auxBuf,
                 FxVideoTimingInfo* vidTiming);

int
main(int argc, char **argv) {
  float                scrWidth   = 640.0f;
  float                scrHeight  = 480.0f;
  int frames                      = -1;
  FxBool               scrgrab    = FXFALSE;
  char                 filename[256];
  FxVideoTimingInfo*   vidTiming  = NULL;
  FxVideoTimingInfo    curVidTiming = gDefaultTimingVal;
  GrContext_t context;

  /* Initialize Glide */
  grGlideInit();
  assert( hwconfig = tlVoodooType() );

  /* Process Command Line Arguments */
  {
    char match; 
    char **remArgs;
    int  rv;

    while((rv = tlGetOpt(argc, argv, "ndv", &match, &remArgs)) != 0) {
      if(rv == -1) {
        printf("Unrecognized command line argument\n");
        printf("%s %s\n", name, usage);

        return -1;
      }

      switch(match) {
      case 'n':
        frames = atoi(remArgs[0]);
        break;

      case 'd':
        scrgrab = FXTRUE;
        frames = 1;
        strcpy(filename, remArgs[0]);
        break;

      case 'v':
        if (hwconfig == TL_VOODOORUSH) {
          printf("%s: cannot use grSstVidMode on Voodoo Rush systems.\n", name);
        } else {
          vidTiming = &curVidTiming;
          grSstVidMode(0, vidTiming);
        }
        break;
      }
    }
  }

  tlSetScreen(scrWidth, scrHeight);
  version = grGetString( GR_VERSION );

  printf("%s:\n%s\n", name, purpose);
  printf("%s\n", version);
    
  grSstSelect(0);
  context = grSstWinOpen(tlGethWnd(),
                      GR_RESOLUTION_640x480,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1);
  if (!context) {
    printf("Could not allocate glide fullscreen context.\n");
    goto __errExit;
  }

  tlConSet(0.0f, 0.0f, 1.0f, 1.0f, 
           60, 30, 0xffffff);
  grSstWinClose(context);
    
  {
    GrScreenRefresh_t curRefresh = GR_REFRESH_60Hz;
    GrScreenResolution_t curRes = GR_RESOLUTION_640x480;
    int numColorBuf = 2, numAuxBuf = 1;
    
    while((frames != 0) && tlOkToRender()) {
      /* Try to open at the current user selection */
      if (!(context = grSstWinOpen(tlGethWnd(),
                        curRes, curRefresh,
                        GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT,
                        numColorBuf, numAuxBuf))) {          

        /* If we cannot use the user selection the set to one that
         * should work so that we have reasonable error output.
         *
         * NB: We clear the user's video data here so that we don't
         * any failures due to bad data.
         */
        grSstVidMode(0, NULL);
        if (! ( context = grSstWinOpen(tlGethWnd(), GR_RESOLUTION_640x480, GR_REFRESH_60Hz,
                          GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT,
                          2, 0))) {
          fprintf(stderr, "Could not set error resolution.\n");
          goto __errExit;
        }
        grSstVidMode(0, vidTiming);
        
        /*
        ** reset font base address
        */ 
        tlConSet(0.0f, 0.0f, 1.0f, 1.0f, 
                 60, 30, 0xffffff);
        /* Let the user know something went wrong, and poll for some new input */
        OutputPromptMessage(GR_RESOLUTION_640x480,
                            curRes, curRefresh, numColorBuf, numAuxBuf, 
                            vidTiming,
                            FXTRUE);
        tlConRender();
        grBufferSwap(1);
        
        while(!tlKbHit() && !ProcessPromptKey(&frames, &curRes, &curRefresh, 
                                              &numColorBuf, &numAuxBuf,
                                              vidTiming))
          /* Do Nothing */;
        
        /* Cleanup the error reporting and try again. */
        grSstWinClose(context);
        continue;
      }
      
      tlGetDimsByConst(curRes, &scrWidth, &scrHeight);
      tlConSet(0.0f, 0.0f, 1.0f, 1.0f, 
               60, 30, 0xffffff);

      grClipWindow(0, 0,(FxU32)scrWidth,(FxU32)scrHeight);

      grReset(GR_VERTEX_PARAMETER);
      grVertexLayout(GR_PARAM_XY, 0, GR_PARAM_ENABLE);
      
      /* Turn on z-buffering if we have an aux buffer */
      if (numAuxBuf != 0) {
        grVertexLayout(GR_PARAM_Z,   GR_VERTEX_OOZ_OFFSET << 2, GR_PARAM_ENABLE);
        grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
        grDepthBufferFunction(GR_CMP_LESS);
        grDepthMask(FXTRUE);
      }
      
      /* Mmmmm.... Gouraud triangles */
      grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
      grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
                     GR_COMBINE_FACTOR_NONE,
                     GR_COMBINE_LOCAL_ITERATED,
                     GR_COMBINE_OTHER_NONE,
                     FXFALSE);

      {
        GrVertex vtxA, vtxB, vtxC;
        const float triEdge = MIN(scrWidth, scrHeight) / 2.0f;
        const float sin60 = 0.8660254f;
        const float baseXCoor = (triEdge / 2.0f);
        const float baseYCoor = ((sin60 * triEdge) / 2.0f);
        GrVertex templateA = {
          0.f,    /* x: - (triEdge / 2.0f) */
          0.f,    /* y: - ((sin60 * triEdge) / 2.0f) */
          0.0f,   /* z */
          255.0f, /* r */
          0.0f,   /* g */
          0.0f    /* b */
        };
        GrVertex templateB = {
          0.0f,   /* x: (triEdge / 2.0f) */
          0.0f,   /* y: - ((sin60 * triEdge) / 2.0f) */
          0.0f,   /* z */
          0.0f,   /* r */
          255.0f, /* g */
          0.0f    /* b */
        };
        GrVertex templateC = {
          0.0f,  /* x: 0.0f */
          0.0f,  /* y: ((sin60 * triEdge) / 2.0f) */
          0.0f,  /* z */
          0.0f,  /* r */
          0.0f,  /* g */
          255.0f /* b */
        };

        templateA.x = -baseXCoor;
        templateA.y = -baseYCoor;

        templateB.x = baseXCoor;
        templateB.y = -baseYCoor;

        templateC.x = 0.0f;
        templateC.y = baseYCoor;
        
        while (frames != 0) {
#define kPi 3.14159f
          static float t = 0.0f;
          
          grBufferClear(0x00, 0x00, 0xFFFF);

          {
            static float zVal = 0.0f;
            static float zDir = 1.0f;

            vtxA = templateA;
            vtxB = templateB;
            vtxC = templateC;
            
            if (zVal > 100.0f) {
              zDir = -1.0f;
            } else if (zVal < 1.0f) {
              zDir = 1.0f;
            }
            zVal += zDir;

            vtxA.z = 
            vtxB.z =
            vtxC.z = zVal;

            XLateTri(&vtxA, &vtxB, &vtxC, 
                     -1.0f * t, 2.0f,
                     (scrWidth / 2.0f), (scrHeight / 2.0f), 0.0f);
            grDrawTriangle(&vtxA, &vtxB, &vtxC);
          }
          
          {
            vtxA = templateA;
            vtxB = templateB;
            vtxC = templateC;
            
            vtxA.z = 1.0f;
            vtxB.z = 100.0f;
            vtxC.z = 100.0f;
            
            XLateTri(&vtxA, &vtxB, &vtxC, 
                     2.0f * t, 1.0f,
                     (scrWidth / 4.0f), 3.0f * (scrHeight / 4.0f), 0.0f);
            grDrawTriangle(&vtxA, &vtxB, &vtxC);
          }
          
          {
            vtxA = templateA;
            vtxB = templateB;
            vtxC = templateC;
            
            vtxA.z = 100.0f;
            vtxB.z = 1.0f;
            vtxC.z = 100.0f;
            
            XLateTri(&vtxA, &vtxB, &vtxC, 
                     3.0f * t, 1.0f,
                     2.0f * (scrWidth / 4.0f), (scrHeight / 4.0f), 0.0f);
            grDrawTriangle(&vtxA, &vtxB, &vtxC);
          }
          
          {
            vtxA = templateA;
            vtxB = templateB;
            vtxC = templateC;
            
            vtxA.z = 100.0f;
            vtxB.z = 100.0f;
            vtxC.z = 1.0f;

            XLateTri(&vtxA, &vtxB, &vtxC, 
                     4.0f * t, 1.0f,
                     3.0f * (scrWidth / 4.0f), 3.0f * (scrHeight / 4.0f), 0.0f);
            grDrawTriangle(&vtxA, &vtxB, &vtxC);
          }

          /* Advance triangle rotation */
          t += (kPi / 180.0f);
          if (t >= (2.0f * kPi)) t = 0.0f;

          OutputPromptMessage(curRes, curRes, curRefresh, 
                              numColorBuf, numAuxBuf, 
                              vidTiming,
                              FXFALSE);
          tlConRender();
          grBufferSwap(1);
      
          /* grab the frame buffer */
          if(scrgrab) {
            if(!tlScreenDump(filename,(FxU16)scrWidth,(FxU16)scrHeight)) {
              printf("Cannot open %s\n", filename);
            }

            scrgrab = FXFALSE;
          }
      
          if(tlKbHit() && ProcessPromptKey(&frames, &curRes, &curRefresh, 
                                           &numColorBuf, &numAuxBuf,
                                           vidTiming)) break;
          if (frames > 0) frames--;
        }
      }
      
      /* Start new refresh settings */
      grSstWinClose(context);
    }
  }
    
 __errExit:    
  grGlideShutdown();

  return 0;
}

static void  
OutputPromptMessage(GrScreenResolution_t conRes,
                    GrScreenResolution_t res, GrScreenRefresh_t refresh,
                    int colorBuf, int auxBuf,
                    const FxVideoTimingInfo* vidTiming,
                    FxBool errorP)
{
  /* Reset the console attributes so the text
   * is always at the top of the screen.
   */
  {
    float scrWidth, scrHeight;
    
    tlGetDimsByConst(conRes, &scrWidth, &scrHeight);
    tlSetScreen(scrWidth, scrHeight);
    tlConClear();
  }
  
  if (errorP) tlConOutput("Could not use Resolution/Refresh/Buffer combination.\n");
  tlConOutput("ColorBuf:%d AuxBuf:%d\n",
              colorBuf, auxBuf);
  if (vidTiming == NULL) {
    static const char* refreshStrings[] = {
      "60", "70", "72", "75", "80", "90", "100", "85", "120"
    };
    tlConOutput("Res:(%s : 0x%X) Refresh:%s\n",
                tlGetResolutionString(res), res, refreshStrings[refresh]);
    tlConOutput("z       - Resolution\n");
  } else {
    tlConOutput("Res:(%d x %d) Refresh:%d\n",
                vidTiming->xDimension, vidTiming->yDimension,
                vidTiming->refreshRate);
    tlConOutput("x y     - Resolution\n");
  }
  tlConOutput("f       - Refresh rate\n");
  tlConOutput("c       - Color buffer\n");
  tlConOutput("a       - Aux Buffer\n");
  tlConOutput("q       - Quit\n");
}

static FxBool
ProcessPromptKey(int* frames, 
                 GrScreenResolution_t* res,
                 GrScreenRefresh_t* refresh,
                 int* colorBuf,
                 int* auxBuf,
                 FxVideoTimingInfo* vidTiming)
{
  int origKey = tlGetCH();
  int keyHit = tolower(origKey);
  FxI32 dir = ((origKey == keyHit) ? 1 : -1);
  FxBool retVal = FXTRUE;
  FxBool doVidTiming = FXFALSE;
  
  switch(keyHit) {
  case 'a':
    *auxBuf += dir;
    if (*auxBuf > 1) *auxBuf = 0;
    if (*auxBuf < 0) *auxBuf = 1;
    break;

  case 'c':
    *colorBuf += dir;
    if (*colorBuf > 3) *colorBuf = 2;
    if (*colorBuf < 2) *colorBuf = 3;
    break;

  case 'f':
    if (vidTiming == NULL) {
      *refresh += dir;
      if (*refresh > GR_REFRESH_120Hz) *refresh = GR_REFRESH_60Hz;
      if (*refresh < GR_REFRESH_60Hz)  *refresh = GR_REFRESH_120Hz;
    } else {
      vidTiming->refreshRate += dir;
      doVidTiming = FXTRUE;
    }
    break;

  case 'q':
    *frames = 0;
    break;

  case 'x':
  case 'y':
  case 'z':
    if (vidTiming == NULL) {
      *res += dir;
      if (*res > GR_RESOLUTION_400x300) *res = GR_RESOLUTION_320x200;
      if (*res < GR_RESOLUTION_320x200) *res = GR_RESOLUTION_400x300;
    } else if (keyHit != 'z') {
      FxU32* valChange = NULL;

      switch(keyHit) {
      case 'x':
        valChange = &vidTiming->xDimension;
        doVidTiming = FXTRUE;
        break;

      case 'y':
        {
          FxI32 num_boards;
          valChange = &vidTiming->yDimension;
          grGet(GR_NUM_FB, 4, &num_boards);
          if (num_boards == 2) dir *= 2;
          doVidTiming = FXTRUE;
          break;
        }
      }

      if (valChange != NULL) *valChange += dir;
    }
    break;

  default:
    retVal = FXFALSE;
  }

  /* Re-compute the rest of the video timing structure.  Don't blame
   * me, this is a direct translation of the vesa timing spreadsheet
   * that Scott gave me w/o all of the unused fields.
   */
  if ((vidTiming != NULL) && doVidTiming) {
    const FxU32 hGradient  = 600;
    const FxU32 hOffset    = 40;
    const FxU32 hTimeScale = 128;
    const FxU32 hScale     = 20;
    const float floatTimeScale = ((hTimeScale == 0) ? 0.001f : (float)hTimeScale);
    const float floatOffset = (((hOffset - hScale) * floatTimeScale / 256.0f) + hScale);
    const float floatGradient = (floatTimeScale / 256.0f * hGradient);
    const float estHPeriod = (((1.0f / vidTiming->refreshRate) - 550.0f / 1000000.0f) / 
                              (vidTiming->yDimension + (2.0f * 0.0f) + 1.0f + 0.0f) *
                              1000000.0f);
    const float linesSyncPorch = (float)floor(0.5f + (550.0f / estHPeriod));
    const float linesBackPorch = linesSyncPorch - 3.0f;
    const float linesField = (vidTiming->yDimension + linesSyncPorch + 1.0f);
    const float estVFieldRate = (1.0f / estHPeriod  / linesField * 1000000.0f);
    const float actHPeriod = (estHPeriod / (vidTiming->refreshRate / estVFieldRate));
    const float idealDutyCycle = floatOffset - (floatGradient * actHPeriod / 1000.0f);
    const float blankingTimePixels = ((2.0f * 8.0f) * 
                                      (float)floor(0.5f + (vidTiming->xDimension * 
                                                           idealDutyCycle /
                                                           (100.0f - idealDutyCycle) /
                                                           (2.0f * 8.0f))));
    const float totalPixels = (vidTiming->xDimension + blankingTimePixels);
    const float pixelFreq = (totalPixels / actHPeriod);
    const float totalHTime = (float)floor(totalPixels / 8.0f);
    const float syncWidthPixel = (float)floor(0.5f + (8.0f / 100.0f * totalPixels / 8.0f) * 8.0f);
    const float frontPorch = (blankingTimePixels / 2.0f) - syncWidthPixel;
    const float backPorch = frontPorch + syncWidthPixel;
    const float syncWidthChar = syncWidthPixel / 8.0f;
    const float backPorchChar = backPorch / 8.0f;
    
    vidTiming->clkFreq16bpp = pixelFreq;
    vidTiming->clkFreq24bpp = pixelFreq * 2.0f;
    vidTiming->memOffset = (FxU32)(floor(0.5f + ((vidTiming->xDimension / 64.0f) + 0.99)) *
                                   floor(0.5f + ((vidTiming->yDimension / 32.0f) + 0.99)));
    vidTiming->tilesInX = (FxU32)(floor(0.5f + ((vidTiming->xDimension / 64.0f) + 0.99) * 2.0f));
    
    vidTiming->hSyncOff = (FxU32)(((totalHTime - syncWidthChar) * 8.0f) - 1.0f);
    vidTiming->hSyncOn = (FxU32)((syncWidthChar * 8.0f) - 1.0f);
    vidTiming->hBackPorch = (FxU32)((backPorchChar * 8.0f) - 2.0f);
    
    vidTiming->vSyncOff = (FxU32)(linesField - 3.0f);
    vidTiming->vSyncOn = (FxU32)3.0f;
    vidTiming->vBackPorch = (FxU32)linesBackPorch;
  }

  return retVal;
}

static void
XLateTri(GrVertex* a, GrVertex* b, GrVertex* c,
         const float theta, const float scale,
         const float x, const float y, const float z)
{
  GrVertex* vertList[3];
  const float cosTheta = (const float)cos(theta);
  const float sinTheta = (const float)sin(theta);
  int i;

  vertList[0] = a;
  vertList[1] = b;
  vertList[2] = c;

  for(i = 0; i < sizeof(vertList) / sizeof(vertList[0]); i++) {
    const GrVertex* curVertex = vertList[i];
    GrVertex tempVert = *curVertex;

    tempVert.x = ((scale * cosTheta * curVertex->x) +
                  (-sinTheta * curVertex->y) +
                  (0.0f * curVertex->z) +
                  (x * 1.0f));

    tempVert.y = ((sinTheta * curVertex->x) +
                  (scale * cosTheta * curVertex->y) +
                  (0.0f * curVertex->z) +
                  (y * 1.0f));

    tempVert.z = ((0.0f * curVertex->x) +
                  (0.0f * curVertex->y) +
                  (scale * 1.0f * curVertex->z) +
                  (0.0f * 1.0f));
    tempVert.ooz = (1.0f * tempVert.z);

    *vertList[i] = tempVert;
  }
}
