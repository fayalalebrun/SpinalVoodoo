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

#define MAX_CC  6

int hwconfig;
static const char *version;
static tlGlideExtension glideext;

static const char name[]    = "test45";
static const char purpose[] = "combine extension example";
static const char usage[]   = "-n <frames> -r <res> -d <filename> -p <pixel format>";

const char *cmodeName[] = {
  "OFF     ",
  "ON      "
};

const char *cmbxName[] = {
  "ZERO ",    /* GR_CMBX_ZERO                      0x00 */
  "TA   ",    /* GR_CMBX_TEXTURE_ALPHA             0x01 */
  "ALCL ",    /* GR_CMBX_ALOCAL                    0x02 */
  "AOTR ",    /* GR_CMBX_AOTHER                    0x03 */
  "B    ",    /* GR_CMBX_B                         0x04 */
  "CA   ",    /* GR_CMBX_CONSTANT_ALPHA            0x05 */
  "CC   ",    /* GR_CMBX_CONSTANT_COLOR            0x06 */
  "DFCT ",    /* GR_CMBX_DETAIL_FACTOR             0x07 */
  "IA   ",    /* GR_CMBX_ITALPHA                   0x08 */
  "IRGB ",    /* GR_CMBX_ITRGB                     0x09 */
  "LTA  ",    /* GR_CMBX_LOCAL_TEXTURE_ALPHA       0x0a */
  "LTRG ",    /* GR_CMBX_LOCAL_TEXTURE_RGB         0x0b */
  "LFRC ",    /* GR_CMBX_LOD_FRAC                  0x0c */
  "OTA  ",    /* GR_CMBX_OTHER_TEXTURE_ALPHA       0x0d */
  "OTRG ",    /* GR_CMBX_OTHER_TEXTURE_RGB         0x0e */
  "TRGB ",    /* GR_CMBX_TEXTURE_RGB               0x0f */
  "TCA  ",    /* GR_CMBX_TMU_CALPHA                0x10 */
  "TCC  "     /* GR_CMBX_TMU_CCOLOR                0x11 */
};

const char *funcName[] = {
  "ZERO ",    /* GR_FUNC_MODE_ZERO                 0x00 */
  "X    ",    /* GR_FUNC_MODE_X                    0x01 */
  "1-X  ",    /* GR_FUNC_MODE_ONE_MINUS_X          0x02 */
  "-X   ",    /* GR_FUNC_MODE_NEGATIVE_X           0x03 */
  "X-.5 ",    /* GR_FUNC_MODE_X_MINUS_HALF         0x04 */
  "1    ",    /* GR_FUNC_MODE_ONE                  0x05 */
  "-.5  "     /* GR_FUNC_MODE_MINUS_HALF           0x06 */
};

const char *shiftName[] = {
  "1X   ",
  "2X   ",
  "4X   "
};

const char *boolName[] = {
  "0    ",
  "1    "
};

const char *markName[] = {
  " ",
  "*"
};

FxU32 ca_list[] = { GR_CMBX_ITRGB, 
                    GR_CMBX_TEXTURE_RGB, 
                    GR_CMBX_CONSTANT_COLOR,
                    GR_CMBX_ITALPHA, 
                    GR_CMBX_TEXTURE_ALPHA,
                    GR_CMBX_CONSTANT_ALPHA, 
                    GR_CMBX_ZERO };

FxU32 cb_list[] = { GR_CMBX_ITRGB, 
                    GR_CMBX_CONSTANT_COLOR, 
                    GR_CMBX_TEXTURE_RGB,
                    GR_CMBX_ITALPHA, 
                    GR_CMBX_CONSTANT_ALPHA, 
                    GR_CMBX_TEXTURE_ALPHA,
                    GR_CMBX_ZERO };

FxU32 cc_list[] = { GR_CMBX_ZERO, 
                    GR_CMBX_B, 
                    GR_CMBX_AOTHER,
                    GR_CMBX_ALOCAL, 
                    GR_CMBX_TEXTURE_ALPHA, 
                    GR_CMBX_TEXTURE_RGB,
                    GR_CMBX_ZERO, 
                    GR_CMBX_ITRGB, 
                    GR_CMBX_CONSTANT_COLOR,
                    GR_CMBX_ITALPHA, 
                    GR_CMBX_CONSTANT_ALPHA };

FxU32 cd_list[] = { GR_CMBX_ZERO, 
                    GR_CMBX_ALOCAL, 
                    GR_CMBX_TEXTURE_RGB, 
                    GR_CMBX_B };

FxU32 aa_list[] = { GR_CMBX_ITALPHA, 
                    GR_CMBX_TEXTURE_ALPHA, 
                    GR_CMBX_CONSTANT_ALPHA };

FxU32 ab_list[] = { GR_CMBX_ITALPHA, 
                    GR_CMBX_CONSTANT_ALPHA, 
                    GR_CMBX_TEXTURE_ALPHA,
                    GR_CMBX_ZERO };

FxU32 ac_list[] = { GR_CMBX_ZERO, 
                    GR_CMBX_B, 
                    GR_CMBX_AOTHER,
                    GR_CMBX_ALOCAL, 
                    GR_CMBX_TEXTURE_ALPHA, 
                    GR_CMBX_ITALPHA,
                    GR_CMBX_CONSTANT_ALPHA,
                    GR_CMBX_ZERO };

FxU32 ad_list[] = { GR_CMBX_ZERO, 
                    GR_CMBX_ALOCAL, 
                    GR_CMBX_TEXTURE_ALPHA, 
                    GR_CMBX_B };

FxU32 tca_list[] = { GR_CMBX_OTHER_TEXTURE_RGB ,
                     GR_CMBX_OTHER_TEXTURE_ALPHA, 
                     GR_CMBX_LOCAL_TEXTURE_RGB,
                     GR_CMBX_LOCAL_TEXTURE_ALPHA, 
                     GR_CMBX_ITRGB, 
                     GR_CMBX_ITALPHA, 
                     GR_CMBX_TMU_CCOLOR, 
                     GR_CMBX_TMU_CALPHA };

FxU32 tcb_list[] = { GR_CMBX_LOCAL_TEXTURE_RGB, 
                     GR_CMBX_LOCAL_TEXTURE_ALPHA, 
                     GR_CMBX_OTHER_TEXTURE_RGB, 
                     GR_CMBX_OTHER_TEXTURE_ALPHA, 
                     GR_CMBX_ITRGB, 
                     GR_CMBX_ITALPHA, 
                     GR_CMBX_TMU_CCOLOR, 
                     GR_CMBX_TMU_CALPHA };

FxU32 tcc_list[] = { GR_CMBX_ZERO, 
                     GR_CMBX_B, 
                     GR_CMBX_OTHER_TEXTURE_ALPHA, 
                     GR_CMBX_LOCAL_TEXTURE_ALPHA, 
                     GR_CMBX_DETAIL_FACTOR, 
                     GR_CMBX_LOD_FRAC,
                     GR_CMBX_LOCAL_TEXTURE_RGB, 
                     GR_CMBX_OTHER_TEXTURE_RGB, 
                     GR_CMBX_ITRGB,
                     GR_CMBX_ITALPHA, 
                     GR_CMBX_TMU_CCOLOR, 
                     GR_CMBX_TMU_CALPHA };

FxU32 tcd_list[] = { GR_CMBX_ZERO, GR_CMBX_LOCAL_TEXTURE_ALPHA, GR_CMBX_ITRGB,
                     GR_CMBX_B };

FxU32 taa_list[] = { GR_CMBX_OTHER_TEXTURE_ALPHA, 
                     GR_CMBX_LOCAL_TEXTURE_ALPHA, 
                     GR_CMBX_ITALPHA, 
                     GR_CMBX_TMU_CALPHA };

FxU32 tab_list[] = { GR_CMBX_LOCAL_TEXTURE_ALPHA,
                     GR_CMBX_OTHER_TEXTURE_ALPHA,
                     GR_CMBX_ITALPHA, 
                     GR_CMBX_TMU_CALPHA };

FxU32 tac_list[] = { GR_CMBX_ZERO, 
                     GR_CMBX_B, 
                     GR_CMBX_OTHER_TEXTURE_ALPHA, 
                     GR_CMBX_LOCAL_TEXTURE_ALPHA, 
                     GR_CMBX_DETAIL_FACTOR, 
                     GR_CMBX_LOD_FRAC,
                     GR_CMBX_ITALPHA, 
                     GR_CMBX_TMU_CALPHA };

FxU32 tad_list[] = { GR_CMBX_ZERO, 
                     GR_CMBX_LOCAL_TEXTURE_ALPHA, 
                     GR_CMBX_ITRGB,
                     GR_CMBX_B };

FxU32 mode_list[] = { GR_FUNC_MODE_ZERO, 
                      GR_FUNC_MODE_X, 
                      GR_FUNC_MODE_ONE_MINUS_X,
                      GR_FUNC_MODE_NEGATIVE_X, 
                      GR_FUNC_MODE_X_MINUS_HALF };

FxU32 shift_list[] = { 0, 1, 2 };
FxU32 inv_list[] = { 0, 1 };
FxU32 a_size[] = { sizeof(ca_list) / sizeof(FxU32), 
                   sizeof(aa_list) / sizeof(FxU32),
                   sizeof(tca_list) / sizeof(FxU32),
                   sizeof(taa_list) / sizeof(FxU32) };
FxU32 b_size[] = { sizeof(cb_list) / sizeof(FxU32), 
                   sizeof(ab_list) / sizeof(FxU32),
                   sizeof(tcb_list) / sizeof(FxU32),
                   sizeof(tab_list) / sizeof(FxU32) };
FxU32 c_size[] = { sizeof(cc_list) / sizeof(FxU32), 
                   sizeof(ac_list) / sizeof(FxU32),
                   sizeof(tcc_list) / sizeof(FxU32),
                   sizeof(tac_list) / sizeof(FxU32) };
FxU32 d_size[] = { sizeof(cd_list) / sizeof(FxU32), 
                   sizeof(ad_list) / sizeof(FxU32),
                   sizeof(tcd_list) / sizeof(FxU32),
                   sizeof(tad_list) / sizeof(FxU32) };

static void doHelp( void ) {
  grBufferClear( 0, 0, 0 );
  tlConClear();
  tlConOutput("Keymap:\n");
  tlConOutput(" 1:           use next a value\n");
  tlConOutput(" 2:           use next a mode\n");
  tlConOutput(" 3:           use next b value\n");
  tlConOutput(" 4:           use next b mode\n");
  tlConOutput(" 5:           use next c value\n");
  tlConOutput(" 6:           toggle c invert\n");
  tlConOutput(" 7:           use next d value\n");
  tlConOutput(" 8:           toggle d invert\n");
  tlConOutput(" 9:           use next shift vlaue\n");
  tlConOutput(" 0:           toggle invert value\n");
  tlConOutput(" H:           get help\n");
  tlConOutput(" up arrow:    select previous combine function\n");
  tlConOutput(" down arrow:  select next combine function\n");
  tlConOutput("Press a key to continue...\n");
  tlConRender();
  grBufferSwap( 1 );
  tlGetCH();
  tlConClear();
}

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

  TlTexture            texture0;
  TlTexture            texture1;
  FxU32                zrange[2];
  GrContext_t          gc = 0;
  FxU32                pixelformat = 0x0003; /* 16 bit 565 */

  float 
    xLeft,
    xRight,
    yTop,
    yBottom;
  FxU32 ca[MAX_CC], cam[MAX_CC], cb[MAX_CC], cbm[MAX_CC], 
    cc[MAX_CC], ccm[MAX_CC], cd[MAX_CC], cdm[MAX_CC], cshift[MAX_CC], cinv[MAX_CC];    
  FxI32 mode = 0;

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
    goto __errExit;
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
  assert(tlLoadTexture("miro.3df", 
                       &texture0.info, 
                       &texture0.tableType, 
                       &texture0.tableData));
  /* Download texture data to TMU */
  grTexDownloadMipMap(GR_TMU0,
                      grTexMinAddress(GR_TMU0),
                      GR_MIPMAPLEVELMASK_BOTH,
                      &texture0.info);
  if (texture0.tableType != NO_TABLE) {
    grTexDownloadTable(texture0.tableType,
                       &texture0.tableData);
  }
  
  assert(tlLoadTexture("lava.3df", 
                       &texture1.info, 
                       &texture1.tableType, 
                       &texture1.tableData));
  grTexDownloadMipMap(GR_TMU1,
                      grTexMinAddress(GR_TMU1),
                      GR_MIPMAPLEVELMASK_BOTH,
                      &texture1.info);
  if (texture1.tableType != NO_TABLE) {
    grTexDownloadTable(texture1.tableType,
                       &texture1.tableData);
  }

  /* Select Texture As Source of all texturing operations */
  grTexSource(GR_TMU0,
              grTexMinAddress(GR_TMU0),
              GR_MIPMAPLEVELMASK_BOTH,
              &texture0.info);
  grTexSource(GR_TMU1,
              grTexMinAddress(GR_TMU1),
              GR_MIPMAPLEVELMASK_BOTH,
              &texture1.info);


  /* Enable Bilinear Filtering + Mipmapping */
  grTexFilterMode(GR_TMU0,
                  GR_TEXTUREFILTER_BILINEAR,
                  GR_TEXTUREFILTER_BILINEAR);
  grTexMipMapMode(GR_TMU0,
                  GR_MIPMAP_NEAREST,
                  FXFALSE);

  xLeft   = tlScaleX( 0.2f );
  xRight  = tlScaleX( 0.8f );
  yTop    = tlScaleY( 0.3f );
  yBottom = tlScaleY( 0.9f );

  { /* default value */
    FxI32 i = 0;
    ca[i] = 1; cam[i] = 1; cb[i] = 6; cbm[i] = 1; 
    cc[i] = 0; ccm[i] = 1; cd[i] = 0; cdm[i] = 0;
    cshift[i] = 0; cinv[i] = 0;    
    i = 1;
    ca[i] = 0; cam[i] = 0; cb[i] = 0; cbm[i] = 0; 
    cc[i] = 0; ccm[i] = 0; cd[i] = 0; cdm[i] = 0;
    cshift[i] = 0; cinv[i] = 0;    
    i = 2;
    ca[i] = 0; cam[i] = 0; cb[i] = 0; cbm[i] = 1; 
    cc[i] = 0; ccm[i] = 0; cd[i] = 3; cdm[i] = 0;
    cshift[i] = 0; cinv[i] = 0;    
    i = 3;
    ca[i] = 0; cam[i] = 0; cb[i] = 0; cbm[i] = 0; 
    cc[i] = 0; ccm[i] = 0; cd[i] = 0; cdm[i] = 0;
    cshift[i] = 0; cinv[i] = 0;    
    i = 4;
    ca[i] = 0; cam[i] = 0; cb[i] = 0; cbm[i] = 1; 
    cc[i] = 0; ccm[i] = 0; cd[i] = 3; cdm[i] = 0;
    cshift[i] = 0; cinv[i] = 0;    
    i = 5;
    ca[i] = 0; cam[i] = 0; cb[i] = 0; cbm[i] = 0; 
    cc[i] = 0; ccm[i] = 0; cd[i] = 0; cdm[i] = 0;
    cshift[i] = 0; cinv[i] = 0;    
  }

  if(frames == -1) {
    doHelp();
  }

  while(frames-- && tlOkToRender()) {
    GrVertex vtxA, vtxB, vtxC, vtxD;
    FxI32 i, cont = 1;

    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight);
        
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }

    grBufferClear(0x00, 0, zrange[1]);

    tlConOutput("Press Q key to quit\n");
    tlConOutput("      a    mode b    mode c    mode d    mode shif inv  \n");
    i = 0;
    tlConOutput("%s  CC %s%s%s%s%s%s%s%s%s%s\n", 
                markName[(i == mode)],
                cmbxName[ca_list[ca[i]]],
                funcName[mode_list[cam[i]]],
                cmbxName[cb_list[cb[i]]],
                funcName[mode_list[cbm[i]]],
                cmbxName[cc_list[cc[i]]],
                boolName[inv_list[ccm[i]]],
                cmbxName[cd_list[cd[i]]],
                boolName[inv_list[cdm[i]]],
                shiftName[shift_list[cshift[i]]],
                boolName[inv_list[cinv[i]]]);
    i = 1;
    tlConOutput("%s  AC %s%s%s%s%s%s%s%s%s%s\n", 
                markName[(i == mode)],
                cmbxName[aa_list[ca[i]]],
                funcName[mode_list[cam[i]]],
                cmbxName[ab_list[cb[i]]],
                funcName[mode_list[cbm[i]]],
                cmbxName[ac_list[cc[i]]],
                boolName[inv_list[ccm[i]]],
                cmbxName[ad_list[cd[i]]],
                boolName[inv_list[cdm[i]]],
                shiftName[shift_list[cshift[i]]],
                boolName[inv_list[cinv[i]]]);
    i = 2;
    tlConOutput("%s TC0 %s%s%s%s%s%s%s%s%s%s\n", 
                markName[(i == mode)],
                cmbxName[tca_list[ca[i]]],
                funcName[mode_list[cam[i]]],
                cmbxName[tcb_list[cb[i]]],
                funcName[mode_list[cbm[i]]],
                cmbxName[tcc_list[cc[i]]],
                boolName[inv_list[ccm[i]]],
                cmbxName[tcd_list[cd[i]]],
                boolName[inv_list[cdm[i]]],
                shiftName[shift_list[cshift[i]]],
                boolName[inv_list[cinv[i]]]);
    i = 3;
    tlConOutput("%s TA0 %s%s%s%s%s%s%s%s%s%s\n", 
                markName[(i == mode)],
                cmbxName[taa_list[ca[i]]],
                funcName[mode_list[cam[i]]],
                cmbxName[tab_list[cb[i]]],
                funcName[mode_list[cbm[i]]],
                cmbxName[tac_list[cc[i]]],
                boolName[inv_list[ccm[i]]],
                cmbxName[tad_list[cd[i]]],
                boolName[inv_list[cdm[i]]],
                shiftName[shift_list[cshift[i]]],
                boolName[inv_list[cinv[i]]]);

    i = 4;
    tlConOutput("%s TC1 %s%s%s%s%s%s%s%s%s%s\n", 
                markName[(i == mode)],
                cmbxName[tca_list[ca[i]]],
                funcName[mode_list[cam[i]]],
                cmbxName[tcb_list[cb[i]]],
                funcName[mode_list[cbm[i]]],
                cmbxName[tcc_list[cc[i]]],
                boolName[inv_list[ccm[i]]],
                cmbxName[tcd_list[cd[i]]],
                boolName[inv_list[cdm[i]]],
                shiftName[shift_list[cshift[i]]],
                boolName[inv_list[cinv[i]]]);
    i = 5;
    tlConOutput("%s TA1 %s%s%s%s%s%s%s%s%s%s\n", 
                markName[(i == mode)],
                cmbxName[taa_list[ca[i]]],
                funcName[mode_list[cam[i]]],
                cmbxName[tab_list[cb[i]]],
                funcName[mode_list[cbm[i]]],
                cmbxName[tac_list[cc[i]]],
                boolName[inv_list[ccm[i]]],
                cmbxName[tad_list[cd[i]]],
                boolName[inv_list[cdm[i]]],
                shiftName[shift_list[cshift[i]]],
                boolName[inv_list[cinv[i]]]);

    i = 0;
    glideext.grColorCombineExt(ca_list[ca[i]], mode_list[cam[i]], 
                               cb_list[cb[i]], mode_list[cbm[i]], 
                               cc_list[cc[i]], inv_list[ccm[i]],
                               cd_list[cd[i]], inv_list[cdm[i]],
                               shift_list[cshift[i]], inv_list[cinv[i]]);
    i = 1;
    glideext.grAlphaCombineExt(aa_list[ca[i]], mode_list[cam[i]], 
                               ab_list[cb[i]], mode_list[cbm[i]], 
                               ac_list[cc[i]], inv_list[ccm[i]],
                               ad_list[cd[i]], inv_list[cdm[i]],
                               shift_list[cshift[i]], inv_list[cinv[i]]);
    i = 2;
    glideext.grTexColorCombineExt(GR_TMU0, 
                                  tca_list[ca[i]], mode_list[cam[i]], 
                                  tcb_list[cb[i]], mode_list[cbm[i]], 
                                  tcc_list[cc[i]], inv_list[ccm[i]],
                                  tcd_list[cd[i]], inv_list[cdm[i]],
                                  shift_list[cshift[i]], inv_list[cinv[i]]);
    i = 3;
    glideext.grTexAlphaCombineExt(GR_TMU0, 
                                  taa_list[ca[i]], mode_list[cam[i]], 
                                  tab_list[cb[i]], mode_list[cbm[i]], 
                                  tac_list[cc[i]], inv_list[ccm[i]],
                                  tad_list[cd[i]], inv_list[cdm[i]],
                                  shift_list[cshift[i]], inv_list[cinv[i]]);
    i = 4;
    glideext.grTexColorCombineExt(GR_TMU1, 
                                  tca_list[ca[i]], mode_list[cam[i]], 
                                  tcb_list[cb[i]], mode_list[cbm[i]], 
                                  tcc_list[cc[i]], inv_list[ccm[i]],
                                  tcd_list[cd[i]], inv_list[cdm[i]],
                                  shift_list[cshift[i]], inv_list[cinv[i]]);
    i = 5;
    glideext.grTexAlphaCombineExt(GR_TMU1, 
                                  taa_list[ca[i]], mode_list[cam[i]], 
                                  tab_list[cb[i]], mode_list[cbm[i]], 
                                  tac_list[cc[i]], inv_list[ccm[i]],
                                  tad_list[cd[i]], inv_list[cdm[i]],
                                  shift_list[cshift[i]], inv_list[cinv[i]]);
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
    vtxA.tmuvtx[1].sow = vtxC.tmuvtx[1].sow = 0.0f;
    vtxB.tmuvtx[1].sow = vtxD.tmuvtx[1].sow = 255.0f;
    vtxA.tmuvtx[1].tow = vtxB.tmuvtx[1].tow = 0.0f;
    vtxC.tmuvtx[1].tow = vtxD.tmuvtx[1].tow = 255.0f;

    vtxA.r = 255.0f, vtxA.g =   0.0f, vtxA.b =   0.0f, vtxA.a = 255.0f;
    vtxB.r =   0.0f, vtxB.g = 255.0f, vtxB.b =   0.0f, vtxB.a = 128.0f;
    vtxC.r =   0.0f, vtxC.g =   0.0f, vtxC.b = 255.0f, vtxC.a = 128.0f;
    vtxD.r =   0.0f, vtxD.g =   0.0f, vtxD.b =   0.0f, vtxD.a =   0.0f;

    grConstantColorValue(0xFF0000FFUL);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ONE, GR_BLEND_ZERO);
    grTexSource(GR_TMU0,
                grTexMinAddress(GR_TMU0),
                GR_MIPMAPLEVELMASK_BOTH,
                &texture0.info);
    grTexSource(GR_TMU1,
                grTexMinAddress(GR_TMU1),
                GR_MIPMAPLEVELMASK_BOTH,
                &texture1.info);

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

      subframe++;
    }
        
    do { /* tlKbHit()) { */
      const char curKey = tlGetCH();

      switch(curKey) {
      case '1':
        cont = 0;
        ca[mode]++;
        if (ca[mode] >= a_size[mode]) ca[mode] = 0;
        break;
      case '2':
        cont = 0;
        cam[mode]++;
        if (cam[mode] >= (sizeof(mode_list) / sizeof(FxU32))) cam[mode] = 0;
        break;
      case '3':
        cont = 0;
        cb[mode]++;
        if (cb[mode] >= b_size[mode]) cb[mode] = 0;
        break;
      case '4':
        cont = 0;
        cbm[mode]++;
        if (cbm[mode] >= (sizeof(mode_list) / sizeof(FxU32))) cbm[mode] =0;
        break;
      case '5':
        cont = 0;
        cc[mode]++;
        if (cc[mode] >= c_size[mode]) cc[mode] = 0;
        break;
      case '6':
        cont = 0;
        ccm[mode]++;
        if (ccm[mode] >= (sizeof(inv_list) / sizeof(FxU32))) ccm[mode] = 0;
        break;
      case '7':
        cont = 0;
        cd[mode]++;
        if (cd[mode] >= d_size[mode]) cd[mode] = 0;
        break;
      case '8':
        cont = 0;
        cdm[mode]++;
        if (cdm[mode] >= (sizeof(inv_list) / sizeof(FxU32))) cdm[mode] = 0;
        break;
      case '9':
        cont = 0;
        cshift[mode]++;
        if (cshift[mode] >= (sizeof(shift_list) / sizeof(FxU32))) cshift[mode] = 0;
        break;
      case '0':
        cont = 0;
        cinv[mode]++;
        if (cinv[mode] >= (sizeof(inv_list) / sizeof(FxU32)))  cinv[mode] = 0;
        break;
      case 'h':
      case 'H':
        cont = 0;
        doHelp();
        break;
      case ' ':
        cont = 0;
        break;
      case -122:
      case 30:
        cont = 0;
        mode -= 1;
        if (mode < 0) mode = MAX_CC - 1;
        break;
      case -120:
      case 31:
        cont = 0;
        mode += 1;
        if (mode >= MAX_CC) mode = 0;
        break;
      case 'q':
      case 27:
        cont = 0;
        frames = 0;
        break;
      }
    } while ( cont );
  }
    
 __errExit:    
  grGlideShutdown();
  return 0;
}
