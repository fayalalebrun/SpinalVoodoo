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
** Initialization code for initializing canpus cards w/ assymetric
** inter-chip buses.
**
*/

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

#include "canopus.h"

/*
 * Syntax:                                                  LarsF (8/2/97)
 *        sst1InitComputeClkParamsATT_Int(Frequ, *clkTiming);
 *  
 *----------------------------------------------------------------------------
 * Description:
 *              Calculates the PLL Parameter for the given Frequency in Hz
 *
 * Input : Frequ        - Frequency in Hz
 *         *clkTiming   - pointer to timing structure
 * Output: *clkTiming   - pointer to timing structure which would be filled
 * Return: 1 - Sucess, 0 - Error
 */
FxBool 
sst1InitComputeClkParamsATT_Int(FFLOAT dwFreq, sst1ClkTimingStruct *clkTiming)
{
#define MMAX    (126+2)     /* physical limit of the M parameter */
#define NMID    18          /* PLL: middle N test range          */
#define NDELTA  15          /* PLL: +/- N test range             */
#define FI      (14318180l) /* PLL: PLL frequenz in HZ           */

  FxU32   lPDiv, lRatio;
  FxU16   sN, sM, sP;
  FxU16   sNBest, sMBest;
  FxU32   lError, lActual;
  FxU32   lBestErr, lBestFreq;

  // first check range of Frequ
  if ((dwFreq < 15000000) || (dwFreq > 240000000)) return FXFALSE;

  // get the best P divider for the given Frequency 
  if (dwFreq < 30000000) {
    sP = 3; /* divide by 16 */
    lPDiv = 8;
  } else if ( dwFreq < 60000000 ) {
    sP = 2; /* divide by 8 */
    lPDiv = 4;
  } else if ( dwFreq < 120000000 ) {
    sP = 1; /* divide by 4 */
    lPDiv = 2;
  } else {
    sP = 0; /* divide by 1 */
    lPDiv = 1;
  }

  // next find the best N/M combination
  lBestErr = 99999999;
  sNBest = 0;
  sMBest = 0;
  lRatio = (FxU32)((dwFreq*10l)/(FI/100l)) * lPDiv; // lRatio in [1/1000]
  for ( sN= (NMID-NDELTA); sN <= (NMID+NDELTA); sN++ ) {
    sM = (unsigned short)((lRatio * sN + 500) / 1000l);
    if (sM > MMAX) sM = MMAX;
    
    lActual = (FxU32)((FI * sM) / (sN * lPDiv));
    lError = (lActual > dwFreq) ? (lActual - dwFreq) : (dwFreq - lActual);
    if ( lError < lBestErr ) {
      sNBest    = sN;
      sMBest    = sM;
      lBestErr  = lError;
      lBestFreq = lActual;
    } /* if best */

    /* increment M also */
    sM++;
    if (sM > MMAX) sM = MMAX;

    lActual = (FxU32)((FI * sM) / (sN * lPDiv));
    lError = (lActual > dwFreq) ? (lActual - dwFreq) : (dwFreq - lActual);
    if ( lError < lBestErr) {
      sNBest    = sN;
      sMBest    = sM;
      lBestErr  = lError;
      lBestFreq = lActual;
    } /* if best */
  } /* for sN */

  if(sNBest == 0) return(FXFALSE);

  sM    = sMBest-2; 
  sN    = sNBest-2;
  
  clkTiming->freq = (float)(dwFreq / 1000000.0f);
  clkTiming->clkTiming_M = sM;
  clkTiming->clkTiming_P = sP;
  clkTiming->clkTiming_N = sN;
  if(dwFreq <  37000000) {
    clkTiming->clkTiming_L = 0xa;
    clkTiming->clkTiming_IB = 0x6;
  } else if(dwFreq < 45000000) {
    clkTiming->clkTiming_L = 0xc;
    clkTiming->clkTiming_IB = 0x4;
  } else if(dwFreq < 58000000) {
    clkTiming->clkTiming_L = 0x8;
    clkTiming->clkTiming_IB = 0x4;
  } else if(dwFreq < 66000000) {
    clkTiming->clkTiming_L = 0xa;
    clkTiming->clkTiming_IB = 0x6;
  } else {
    clkTiming->clkTiming_L = 0xa;
    clkTiming->clkTiming_IB = 0x8;
  }

  return FXTRUE;
} /* sst1InitComputeClkParamsATT_Int() */

/*
** sst1UsrSetGrxClk():
**  Initialize FBI and TREX Memory clocks to a user value
**  NOTE: sst1InitCalcGrxClk() must be called prior to sst1InitGrxClk()
**  NOTE: sst1InitGrxClk() resets the PCI fifo and the graphics subsystem of FBI
**
*/
FxBool 
sst1SetGrxClk_Canopus(FxU32* sstbase, FFLOAT grxclk)
{
  FxBool  RetVal;
  sst1ClkTimingStruct sstGrxClk;
  
  if(!sst1InitCheckBoard(sstbase))
    return(FXFALSE);
  
  if(!sst1InitComputeClkParamsATT_Int(grxclk * CLOCK_MULTIPLIER, &sstGrxClk))
    return(FXFALSE);
  
  sst1CurrentBoard->initGrxClkDone = 0;  // Allow clk to be reset to user value.
  
  RetVal = sst1InitSetGrxClk(sstbase, &sstGrxClk);
  
  sst1CurrentBoard->initGrxClkDone = 0;  // Allow clk to be set to the choosen value again
  
  return(RetVal);
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
