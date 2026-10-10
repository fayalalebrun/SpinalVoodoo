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
** Initialization code for loading SST-1 gamma tables
**
*/
#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */
#ifdef _MSC_VER
#pragma optimize ("",off)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef BUILD_FOR_SST1
#include <sst.h>
#else
#include <3dfx.h>
#include <cvgregs.h>
#include <cvgdefs.h>
#endif
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <sst1vid.h>
#include <sst1init.h>

/* OK, so this should be 1.7, but sometime during the
 * late stages of the original v2 release hell we changed
 * it to 1.3 to make id (or someone happy).
 */
#define kDefaultVoodoo2Gamma 1.3

/*
** sst1InitGamma():
**  Load the video color-lookup tables with the specified gamma function
**
**    Returns:
**      FXTRUE if successfully initializes SST-1 gamma tables
**      FXFALSE if cannot initialize SST-1 gamma tables
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitGamma(FxU32 *sstbase, double gamma)
{
  if(sstbase == NULL) return(FXFALSE);
  if(!sst1InitCheckBoard(sstbase)) return(FXFALSE);

  return sst1InitGammaRGB(sstbase, gamma, gamma, gamma);
}

FX_EXPORT FxBool FX_CSTYLE sst1InitGammaRGB(FxU32 *sstbase, 
                                            double gammaR,
                                            double gammaG, 
                                            double gammaB)
{
  FxU32 
    x,
    gammaTableR[256],
    gammaTableG[256],
    gammaTableB[256];
  FxBool 
    sstVideoIsReset;
  SstRegs *
    sst = (SstRegs *) sstbase;
  static FxBool 
    calledBefore = FXFALSE;
  static double 
    userGammaR = kDefaultVoodoo2Gamma, 
    userGammaG = kDefaultVoodoo2Gamma, 
    userGammaB = kDefaultVoodoo2Gamma;

  if(sstbase == NULL) return(FXFALSE);
  if(!sst1InitCheckBoard(sstbase)) return(FXFALSE);
  
  if(!sst1CurrentBoard->fbiInitGammaDone)
    INIT_PRINTF(("sst1InitGammaRGB(): Setting GammaRGB = (%.2f,%.2f,%.2f)\n",
                 gammaR, gammaG, gammaB));

  /* Get the user set definitions (cp or environment) */
  if(!calledBefore) {
    const char *envp;
    calledBefore = FXTRUE;
    envp = GETENV(("SSTV2_RGAMMA"));
    if(envp) {
      userGammaR = (double) ATOF(envp);
    }
    envp = GETENV(("SSTV2_GGAMMA"));
    if(envp) {
      userGammaG = (double) ATOF(envp);
    }
    envp = GETENV(("SSTV2_BGAMMA"));
    if(envp) {
      userGammaB = (double) ATOF(envp);
    }
    envp = GETENV(("SSTV2_GAMMA"));
    if(envp) {
      userGammaR = (double) ATOF(envp);
      userGammaG = userGammaR;
      userGammaB = userGammaR;
    }
  }

  gammaR *= (userGammaR / kDefaultVoodoo2Gamma);
  gammaG *= (userGammaG / kDefaultVoodoo2Gamma);
  gammaB *= (userGammaB / kDefaultVoodoo2Gamma);
  
  // Initialize the gamma table
  for(x=0; x<256; x++) {
    gammaTableR[x] = FTOL (POW(x/255.0F, 1.0F/gammaR) * 255.0F + 0.5F);
    gammaTableG[x] = FTOL (POW(x/255.0F, 1.0F/gammaG) * 255.0F + 0.5F);
    gammaTableB[x] = FTOL (POW(x/255.0F, 1.0F/gammaB) * 255.0F + 0.5F);
  }
  
  // Store gamma values in board info structure
  sst1CurrentBoard->fbiGammaRed = gammaR;
  sst1CurrentBoard->fbiGammaGreen = gammaG;
  sst1CurrentBoard->fbiGammaBlue = gammaB;

  // SST-1 video reset must be inactive to load gamma tables
  if(IGET(sst->fbiInit1) & SST_VIDEO_RESET) {
    sstVideoIsReset = FXTRUE;
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) & ~SST_VIDEO_RESET);
    // wait for video reset to be deasserted
    sst1InitIdleFBINoNOP(sstbase);
  } else {
    sstVideoIsReset = FXFALSE;
  }

  // SST-1 requires every eighth entry of the gamma table to be loaded,
  // so only 32 basic writes are required.  A 33rd write is used to load
  // the top entry of the gamma table.  The 33rd entry is necessary because
  // SST-1 performs linear interpolation between each gamma table entry to
  // generate 256 unique gamma-corrected values.
  for(x=0; x<32; x++) {
    FxU32 gcR = gammaTableR[(x<<3)];
    FxU32 gcG = gammaTableG[(x<<3)];
    FxU32 gcB = gammaTableB[(x<<3)];
    ISET(sst->clutData, ((x<<SST_CLUTDATA_INDEX_SHIFT) | 
                         (gcR<<SST_CLUTDATA_RED_SHIFT) |
                         (gcG<<SST_CLUTDATA_GREEN_SHIFT) |
                         (gcB<<SST_CLUTDATA_BLUE_SHIFT)));
  }

  // Last entry in the gamma table is stored as 0x0 to perform proper
  // linear interpolation of the last 8 entries
  //
  // BUG Fix: Last entry in table needs to be 0xffffff for proper linear
  //  interpolation
  //
  // YABF: Unconditionally using 0xFFFFFF to max out the interpolation
  // causes a problem if the component gamma value is 0 (some
  // developer wants this).   Anyway, we now special case 0 (or close to 0) 
  // so that   // this looks right.
#define GAMMA_COMP_FLOOR(__val) (~(((__val) == 0x00UL) - 0x01UL) & 0xFFUL)
  ISET(sst->clutData, ((32 << SST_CLUTDATA_INDEX_SHIFT) | 
                       (GAMMA_COMP_FLOOR(gammaTableR[255]) << SST_CLUTDATA_RED_SHIFT) |
                       (GAMMA_COMP_FLOOR(gammaTableG[255]) << SST_CLUTDATA_RED_SHIFT) |
                       (GAMMA_COMP_FLOOR(gammaTableB[255]) << SST_CLUTDATA_RED_SHIFT)));
#undef GAMMA_COMP_FLOOR
  
  if(sstVideoIsReset) {
    // wait for gamma table writes to complete
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    sst1InitIdleFBINoNOP(sstbase);
  }

  if(!sst1CurrentBoard->fbiInitGammaDone) {
    sst1CurrentBoard->fbiInitGammaDone = 1;
    INIT_PRINTF(("sst1InitGammaRGB() exiting with status %d...\n", FXTRUE));
  }
  
  return FXTRUE;
}

FX_EXPORT FxBool FX_CSTYLE sst1InitGammaTable(FxU32 *sstbase, FxU32 nentries, FxU32 *r, FxU32 *g, FxU32 *b)
{
    FxU32 x;
    FxU32 gammaTableR[256];
    FxU32 gammaTableG[256];
    FxU32 gammaTableB[256];
    FxBool sstVideoIsReset;
    SstRegs *sst = (SstRegs *) sstbase;

    if(!sstbase)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    // Initialize the gamma table
    for(x=0; x < nentries; x++) {
      gammaTableR[x] = *r;
      gammaTableG[x] = *g;
      gammaTableB[x] = *b;
      r++; g++; b++;
    }

    // SST-1 video reset must be inactive to load gamma tables
    if(IGET(sst->fbiInit1) & SST_VIDEO_RESET) {
        sstVideoIsReset = FXTRUE;
        sst1InitIdleFBINoNOP(sstbase);
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) & ~SST_VIDEO_RESET);
        // wait for video reset to be deasserted
        sst1InitIdleFBINoNOP(sstbase);
    } else
        sstVideoIsReset = FXFALSE;

    // SST-1 requires every eighth entry of the gamma table to be loaded,
    // so only 32 basic writes are required.  A 33rd write is used to load
    // the top entry of the gamma table.  The 33rd entry is necessary because
    // SST-1 performs linear interpolation between each gamma table entry to
    // generate 256 unique gamma-corrected values.
    for(x=0; x < nentries; x++) {
        FxU32 gcR = gammaTableR[(x)];
        FxU32 gcG = gammaTableG[(x)];
        FxU32 gcB = gammaTableB[(x)];
        ISET(sst->clutData, ((x<<SST_CLUTDATA_INDEX_SHIFT) | 
                         (gcR<<SST_CLUTDATA_RED_SHIFT) |
                         (gcG<<SST_CLUTDATA_GREEN_SHIFT) |
                         (gcB<<SST_CLUTDATA_BLUE_SHIFT)));
    }
    // Last entry in the gamma table is stored as 0x0 to perform proper
    // linear interpolation of the last 8 entries
    // BUG Fix: Last entry in table needs to be 0xffffff for proper linear
    //  interpolation
    ISET(sst->clutData, (32<<SST_CLUTDATA_INDEX_SHIFT) | 0xffffff);

    if(sstVideoIsReset == FXTRUE) {
        // wait for gamma table writes to complete
        sst1InitIdleFBINoNOP(sstbase);
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
        sst1InitIdleFBINoNOP(sstbase);
    }

    return(FXTRUE);
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
