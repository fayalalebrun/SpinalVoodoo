/*
** Copyright (c) 1996, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
** rights reserved under the Copyright Laws of the United States.
**
*/
#define INIT_AT3D_LIB
#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */

#include <3dfx.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __linux__
#include <conio.h>
#endif
#include <assert.h>
#include <time.h>

#ifdef __DOS32__
#ifdef __DJGPP__
#include <dos.h>
#else
#include <i86.h>
#endif
#endif

#ifdef __WIN32__
#include <windows.h>
#ifdef __MINGW32__
static inline unsigned char _inp_asm (unsigned short _port) {
  unsigned char rc;
  __asm__ __volatile__ ("inb %w1,%b0" : "=a" (rc) : "Nd" (_port));
  return rc;
}
static inline void _outp_asm (unsigned short _port, unsigned char _data) {
  __asm__ __volatile__ ("outb %b0,%w1" : : "a" (_data), "Nd" (_port));
}
#define _inp  _inp_asm
#define _outp  _outp_asm
#endif
#endif /* __WIN32__ */

#if defined(__WATCOMC__)
#define _inp inp
#define _outp outp
#endif

#ifdef __DJGPP__
#include <fxdpmi.h>
#endif

#if defined(__linux__)
#define _inp(port) pioInByte(port)
#define _outp(port, data) pioOutByte(port, data)
#endif

#define AT3D_BASE 0xffec00

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <gdebug.h>
#include <init.h>

#include "fxinit96.h"

#include "initat3d.h"
#include "atvga.h"
#include "initvga.h"
#include "physical.h"

/*
**  Static Globals   
*/

/* Stuff for programming the memory clock */
typedef struct _CLOCK_TABLE {
  int iMclk;
  unsigned char wN;
  unsigned char wM;
  unsigned char wL;
  unsigned char wFR;
} CLOCK_TABLE;


/* This table is for AT3D, other parts need different tables. */ 
static const CLOCK_TABLE ClockTableAT3D[] =
{
  { 25, 0x1b, 0x1, 0x3, 0x6 },
  { 40, 0x2c, 0x1, 0x3, 0x3 },
  { 45, 0x32, 0x1, 0x3, 0x1 },
  { 50, 0x1b, 0x1, 0x2, 0x6 },
  { 55, 0x2d, 0x2, 0x2, 0x6 },
  { 60, 0x21, 0x1, 0x2, 0x5 },
  { 62, 0x44, 0x3, 0x2, 0x5 },
  { 65, 0x5a, 0x4, 0x2, 0x4 },
  { 67, 0x37, 0x2, 0x2, 0x4 },
  { 70, 0x26, 0x1, 0x2, 0x4 },
  { 72, 0x27, 0x1, 0x2, 0x4 },
  { 75, 0x29, 0x1, 0x2, 0x3 },
  { 80, 0x2c, 0x1, 0x2, 0x3 },
  { 85, 0x2f, 0x1, 0x2, 0x2 },
  { 90, 0x31, 0x1, 0x2, 0x1 },
  { 95, 0x34, 0x1, 0x2, 0x1 },
  { 100, 0x1b, 0x1, 0x1, 0x6},
  { 0,  0,    0,   0,   0 } /* used for end of table mark */
};

typedef struct _REGVALS {
  unsigned char E8;
  unsigned char E9;
  unsigned char EA;
} REGVALS, *LPREGVALS;

typedef struct _REGINFO {
  REGVALS RegVals;
  int   bFreqFound;
} REGINFO, *LPREGINFO;

/* MCLK Constants */
static const unsigned char bBypass = 0;
static const unsigned char bHighSpeed = 1;
static const unsigned char bPowerOff = 0;


/* HALData required for each partner */
static Init96HALData at3dHALData = {
  0x1000000,
  initAT3DAperture,
  initAT3DSetVideo,    
  initAT3DEnableRegs,
  initAT3DDisableRegs,
  NULL,
  initAT3DUseTiles,
  initAT3DGetInfo,
  NULL, /* SwapTiles */
  NULL
};

/* Random Kruft */
static FxBool tiling;           /* Well, are we? */
static FxU32 workaround;        /* Some registers have to be read
                                   after they're written, and we store
                                   that read here*/
static LPCOP at3dRegPtr = NULL;

/* Stuff we squirrel away and put back when we've finished */
static FxU16 tileRegSave[12*8];
FxU16
  regWindowSequenceControlSave,
  regWindow0BaseAddressSave,
  regWindow0StretchFactorHor1Save,
  regWindow0StretchFactorHor2Save,
  regWindow0StretchFactorVer1Save,
  regWindow0StretchFactorVer2Save,
  regWindow0ControlSave;

/* Some Constants */
#define GINTERVAL 100


/*
 **   Assignment Macros/Debugging Support Routines
 **
 **   OK, So these just print out the byte offset of the register, but
 **   before you come bitch at me, look at physical.h to see how much
 **   work it is to generate register names, then look at this file and
 **   see how few of them we actually use.
 **
 **   CHD
 **
 **   Fixme!!! need workaround = d, not workaround = s!  Do this after 2.4
 **
 */
#ifdef GDBG_INFO_ON
#define SETAT3D(d,s) \
{\
GDBG_INFO((80, "%s:  Setting AT3D Register 0x%x with value 0x%x\n", FN_NAME, (FxU32) &d - (FxU32) at3dRegPtr,  s));\
d = s; workaround = (FxU32) s ;}
#define GETAT3D(s) readDebugAT3D(&s, FN_NAME)
#define GET8AT3D(s) read8DebugAT3D(&s, FN_NAME)
#define GET16AT3D(s) read16DebugAT3D(&s, FN_NAME)
#else
#define SETAT3D(d,s) d = s; workaround = (FxU32) s
#define GETAT3D(s) s
#define GET8AT3D(s) s
#define GET16AT3D(s) s
#endif

#ifdef GDBG_INFO_ON
static FxU32
readDebugAT3D(volatile FxU32 *s, char *fName) {
  FxU32 ret;

  ret = *s;

  GDBG_INFO((80, "%s:  Read 0x%x from register 0x%x\n", fName, ret,
             (FxU32) s - (FxU32) at3dRegPtr));

  return ret;

} /* readDebugAT3D */

static FxU8
read8DebugAT3D(volatile FxU8 *s, char *fName) {

  FxU8
    ret;

  ret = *s;

  GDBG_INFO((80, "%s:  Read 0x%x from register 0x%x\n", fName, ret,
             (FxU32) s - (FxU32) at3dRegPtr));

  return ret;
  
} /* read8DebugAT3D */

static FxU16
read16DebugAT3D(volatile FxU16 *s, char *fName) {

  FxU16
    ret;

  ret = *s;

  GDBG_INFO((80, "%s:  Read 0x%x from register 0x%x\n", fName, ret,
             (FxU32) s - (FxU32) at3dRegPtr));

  return ret;
  
} /* read16DebugAT3D */
#endif

/*
 **  AT3D Utility Routines
 **
 **  These routines blah blah blah
 */

#ifdef H3D
void pageflippingSwapWait(void) {
#define FN_NAME "pageflippingSwapWait"
  int bailCount = 150000, pending = 1;
  FxU32 status;

  if (!at3dRegPtr) return;

  status = (FxU32)(GETAT3D(at3dRegPtr->Extended_Status) & 0x00400000L);
  if (!status) {
    do {
      status = (FxU32)(GETAT3D(at3dRegPtr->Extended_Status) & 0x00400000L);
    } while (!status && --bailCount);  // wait for it to go low
    bailCount = 150000;
  }
  if (status) { // is swap pending?
    do {
      status = (FxU32)(GETAT3D(at3dRegPtr->Extended_Status) & 0x00400000L);
    } while (status && --bailCount);  // wait for it to go low
  }
#undef FN_NAME
}
#endif /* H3D */

/*-------------------------------------------------------------------
  Function: initAT3DGetRegVals
  Date: 20-May-97
  Implementor(s): someone from ALSC, ganked by dow
  Library: Init Code for AT3D
  Description:
  This routine was shamelessly ganked out of the DirectDraw driver
  for AT3D/Voodoo Rush.  
  
  Basically, it takes a frequency (in MHz), and returns some
  mystical values needed for programming the PLL.
  
  Arguments:
  
  Return:
  FXTRUE if successful.
  -------------------------------------------------------------------*/
REGINFO
initAT3DGetRegVals(int iFreq)
{
#define FN_NAME "initAT3DGetRegVals"
  /* Get the E8,E9, and EA values for given frequency. */
  const CLOCK_TABLE *lpClockTable = (const CLOCK_TABLE *) ClockTableAT3D;
  int i;
  REGINFO RegInfo; 
  BYTE bN, bM, bL, bFreqRange;
  
  /* Scan for MCLK value from array. */
  i = 0;
  while( lpClockTable[i].iMclk != 0 ) {
    if( lpClockTable[i].iMclk == iFreq )
      break;
    i++;
  }
  
  /* Check if there is no MCLK matched. */
  if( lpClockTable[i].iMclk == 0 )  {
    RegInfo.bFreqFound = FXFALSE;
    i=0;
  } else {
    RegInfo.bFreqFound = FXTRUE;
  }
  
  bN = lpClockTable[i].wN;
  bM = lpClockTable[i].wM;
  bL = lpClockTable[i].wL;
  bFreqRange = lpClockTable[i].wFR;
  
  /* Assign values to E8-EA... */
  RegInfo.RegVals.E8 = bBypass +
    (bPowerOff << 1) +
      (bL << 2) + 
        (bFreqRange << 4) +
          (bHighSpeed << 7);
  RegInfo.RegVals.E9 = bM;
  RegInfo.RegVals.EA = bN;
  
  return(RegInfo);

#undef FN_NAME
} /* GetRegVals */

/*-------------------------------------------------------------------
  Function: setmclk32
  Date: 20-May-97
  Implementor(s): someone from ALSC, ganked by dow
  Library: Init Code for AT3D
  Description:
  This routine sets the memory clock for the AT3D chip.  It was
  shamelessly ganked out of the DirectDraw driver for
  AT3D/Voodoo Rush.  
  
  Arguments:
  
  Return:
  FXTRUE if successful.
  -------------------------------------------------------------------*/
static FxBool
initAT3DSetMClock(int iMclk, LPCOP at3dRegs)
{
#define FN_NAME "initAT3DSetMClock"
  REGINFO RegInfo;
  FxU8 oE8, oE9, oEA;
  FxU8 nE8, nE9, nEA;
  
  GDBG_INFO((80, "%s:\n", FN_NAME));

  /* Get values for E8, E9, EA etc. based on iMclk. */
  RegInfo = initAT3DGetRegVals(iMclk);
  
  if(!RegInfo.bFreqFound)
    return(FXFALSE);
  
  /* Read in the current 3  Bytes starting at 0E8 */
  oE8 = GET8AT3D(at3dRegs->MCLK_Ctrl);
  oE9 = GET8AT3D(at3dRegs->MCLK_M);
  oEA = GET8AT3D(at3dRegs->MCLK_N);
  
  /*
   ** Set new values....
   **
   ** Assign new E8-EA... (with L=3, and high speed bit set)
   ** This is done to avoid a temporary illegal setting of the mclk!
   */
  nE8 = (oE8 | 0x8C);
  nE9 = oE9;
  nEA = oEA;
  SETAT3D(at3dRegs->MCLK_Ctrl, nE8);
  SETAT3D(at3dRegs->MCLK_M, nE9);
  SETAT3D(at3dRegs->MCLK_N, nEA);
  
  /*
   **   Assign new E8-EA... (with desired M and N values)
   ** This is done to avoid a temporary illegal setting of the mclk!
   */
  nE8 = nE8;
  nE9 = RegInfo.RegVals.E9;
  nEA = RegInfo.RegVals.EA;
  SETAT3D(at3dRegs->MCLK_Ctrl, nE8);
  SETAT3D(at3dRegs->MCLK_M, nE9);
  SETAT3D(at3dRegs->MCLK_N, nEA);
  
  /* Assign new E8-EA... (with correct L value) */
  nE8 = RegInfo.RegVals.E8;
  nE9 = nE9;
  nEA = nEA;
  SETAT3D(at3dRegs->MCLK_Ctrl,nE8);
  SETAT3D(at3dRegs->MCLK_M, nE9);
  SETAT3D(at3dRegs->MCLK_N, nEA);
  
  GDBG_INFO((80, "%s:  Returning FXTRUE\n", FN_NAME));

  return(FXTRUE);
  
#undef FN_NAME
} /* initAT3DSetMClock */

/*-------------------------------------------------------------------
  Function: initAT3DWait
  Date: 21-May-97
  Implementor(s): dow
  Library: Init Code for AT3D
  Description:
  Waits for the specified number of milliseconds.  This works
  because CLOCKS_PER_SEC is equal to 1000 in time.h;
  Arguments:
  
  Return:
  Nothing ever.
  -------------------------------------------------------------------*/
static void
initAT3DWait( FxI32 mSec)
{
#define FN_NAME "initAT3DWait"
  clock_t t0, t1;

  t0 = clock();

  do {
    t1 = clock();
  } while ((t1 - t0) < mSec);
#undef FN_NAME
} /* initAT3DWait */

/*-------------------------------------------------------------------
  Function: initAT3D2DIdle
  Date: 21-May-97
  Implementor(s): someone from ALSC, dow
  Library: Init Code for AT3D
  Description:
  Waits for the AT3D chip to be idle;
  Arguments:
  
  Return:
  Nothing ever.
  -------------------------------------------------------------------*/
static void
initAT3D2DIdle(LPCOP at3dRegs) 
{
#define FN_NAME "initAT3D2DIdle"
  volatile FxU32 val;

  while ((val = GETAT3D(at3dRegs->Extended_Status)) & EXT_STATUS_DWG_ENG_BUSY);

#undef FN_NAME
} /* initAT3D2DIdle */

/*-------------------------------------------------------------------
  Function: initAT3DDisableMemoryRefresh
  Date: 09-May-97
  Implementor(s): someone from ALSC, dow
  Library: Init Code for AT3D
  Description:
  Disables memory refresh on AT3D.
  
  Notes:
  Shamelessly ganked from the AT3D DDRAW code.
  
  Arguments:
  
  Return:
  pointer to at3dHALData
  -------------------------------------------------------------------*/
static void
initAT3DDisableMemoryRefresh(LPCOP at3dRegs) 
{
#define FN_NAME "initAT3DDisableMemoryRefresh"
  FxU32 vgaSeq, data;
  FxU16 vgaOverride;

  /* Make sure the 2D is not making any requests for memory */
  initAT3D2DIdle(at3dRegs);
  
  GDBG_INFO((80, "%s:  Turning off video refresh\n", FN_NAME));
  INP(SC_INDEX, vgaSeq);

  OUTP(SC_INDEX, SC_CLOCKING_MODE);
  INP(SC_INDEX, data);

  data |= VGA_SEQUENCER_SCREEN_OFF;
  OUTP(SC_DATA, data);
  OUTP(SC_INDEX, vgaSeq);

  GDBG_INFO((80, "%s:  Turning off DRAM refresh\n", FN_NAME));
  GDBG_INFO((80, "%s:  VGAOverride\n", FN_NAME));
  vgaOverride = GET16AT3D(at3dRegs->VGAOverride);
  vgaOverride |= VGA_OVERRIDE_DISABLE_DRAM_REFRESH;
  SETAT3D(at3dRegs->VGAOverride, vgaOverride);

#undef FN_NAME
} /* initAT3DDisableMemoryRefresh */

/*-------------------------------------------------------------------
  Function: initAT3DEnableMemoryRefresh
  Date: 09-May-97
  Implementor(s): Someone from ALSC, dow
  Library: Init Code for AT3D
  Description:
  
  Arguments:
  
  Return:
  pointer to at3dHALData
  -------------------------------------------------------------------*/
static void
initAT3DEnableMemoryRefresh(LPCOP at3dRegs) 
{
#define FN_NAME "initAT3DEnableMemoryRefresh"

  FxU32 vgaSeq, data;
  FxU16 vgaOverride;

  GDBG_INFO((80, "%s:  Turning on video refresh\n", FN_NAME));
  INP(SC_INDEX, vgaSeq);

  OUTP(SC_INDEX, SC_CLOCKING_MODE);
  INP(SC_DATA, data);

  data &= ~VGA_SEQUENCER_SCREEN_OFF;

  OUTP(SC_DATA, data);
  OUTP(SC_INDEX, vgaSeq);

  GDBG_INFO((80, "%s:  Turning on DRAM refresh\n", FN_NAME));
  GDBG_INFO((80, "%s:  VGAOverride\n", FN_NAME));
  vgaOverride = GET16AT3D(at3dRegs->VGAOverride);
  vgaOverride &= !VGA_OVERRIDE_DISABLE_DRAM_REFRESH;
  SETAT3D(at3dRegs->VGAOverride, vgaOverride);

#undef FN_NAME
} /* initAT3DEnableMemoryRefresh */


/*
 **  Entry Points
 */

/*-------------------------------------------------------------------
  Function: initAT3DGetHALData
  Date: 09-May-97
  Implementor(s): dow
  Library: Init Code for AT3D
  Description:
  
  Arguments:
  
  Return:
  pointer to at3dHALData
  -------------------------------------------------------------------*/
INITAT3DENTRY(initAT3DGetHALData, Init96HALData *, (void) )
{
#define FN_NAME "initAT3DGetHALData"
  return &at3dHALData;
#undef FN_NAME
} /* initAT3DGetHALData */

/*-------------------------------------------------------------------
  Function: initAT3DAperture
  Date: 19-Feb-97
  Implementor(s): dow
  Library: Init Code for AT3D
  Description:
  Initialize rd pointers to access apertures
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
INITAT3DENTRY(initAT3DAperture, FxBool , (InitRegisterDesc *rd) )
{
#define FN_NAME "initAT3DAperture"
  FxBool
    rVal = FXTRUE;

  GDBG_INFO((80, "%s:\n", FN_NAME));

  rd->hwDep.VG96RegDesc.partnerRegPtr =
    &rd->hwDep.VG96RegDesc.baseAddress[AT3D_BASE >> 2];
  GDBG_INFO((80, "%s:  Setting partnerRegPtr to 0x%x\n", FN_NAME, rd->hwDep.VG96RegDesc.partnerRegPtr));

  at3dRegPtr = (LPCOP) rd->hwDep.VG96RegDesc.partnerRegPtr;

  rd->hwDep.VG96RegDesc.serialStatus = (FxU32 *) &at3dRegPtr->SerialInput;
  GDBG_INFO((80, "%s:  Setting serialStatus to 0x%x\n", FN_NAME, rd->hwDep.VG96RegDesc.serialStatus));

  /* This is not important, but it might be tested for NULL somewhere */
  rd->hwDep.VG96RegDesc.fifoApertureBase = rd->hwDep.VG96RegDesc.baseAddress;

  GDBG_INFO((80, "%s:  returning FXTRUE\n", FN_NAME));

  return rVal;

#undef FN_NAME
} /* initAT3DAperture */


INITAT3DENTRY(initAT3DSetVideo, FxBool, (GrScreenResolution_t res, GrScreenRefresh_t refresh, FxU32 *xr, FxU32 *yr )) 
{
#define FN_NAME "initAT3DSetVideo"
#if defined(__DOS32__)
  union REGS r, rOut;
#endif
  const char *envp;
  FxBool
    fixVsync = FXFALSE,
    rv = FXTRUE;
  
  GDBG_INFO((80, "%s: \n", FN_NAME));

#if defined(INIT_ACCESS_DIRECT) && defined( __DOS32__ )
  /* Wait half a sec */
  initAT3DWait( 100);
#endif

  switch( res ) {
  case GR_RESOLUTION_320x200:
#if defined(__DOS32__)
    /* Set up VESA Mode 0x136 - 320x200x16 */
    GDBG_INFO((80, "initAT3DSetVideo:  Setting up VESA mode 320x200x16\n"));
    r.w.ax = 0x4f02;
    r.w.bx = 0x136;
    int386( 0x10, &r, &rOut );
#endif
    *xr = 320;
    *yr = 200;
    rv = FXTRUE;
    break;
  case GR_RESOLUTION_320x240:
#if defined(__DOS32__)
    /* Set up VESA Mode 0x137 - 320x240x16 */
    GDBG_INFO((80, "initAT3DSetVideo:  Setting up VESA mode 320x240x16\n"));
    r.w.ax = 0x4f02;
    r.w.bx = 0x137;
    int386( 0x10, &r, &rOut );
#endif
    *xr = 320;
    *yr = 240;
    rv = FXTRUE;
    break;
  case GR_RESOLUTION_512x384:
    fixVsync = FXTRUE;
#if defined(__DOS32__)
    /* Set up VESA Mode 0x139 - 512x384x16 */
    GDBG_INFO((80, "initAT3DSetVideo:  Setting up VESA mode 512x384x16\n"));
    r.w.ax = 0x4f02;
    r.w.bx = 0x139;
    int386( 0x10, &r, &rOut );
#endif
    *xr = 512;
    *yr = 384;
    rv = FXTRUE;
    break;
  case GR_RESOLUTION_640x400:
    fixVsync = FXTRUE;
    rv = FXTRUE;
    break;

  case GR_RESOLUTION_640x480:
#if defined(__DOS32__)
    /* Set up VESA Mode 0x111 - 640x480x16 */
    GDBG_INFO((80, "initAT3DSetVideo:  Setting up VESA mode 640x480x16\n"));
    r.w.ax = 0x4f02;
    r.w.bx = 0x111;
    int386( 0x10, &r, &rOut );
#endif
    *xr = 640;
    *yr = 480;
    rv = FXTRUE;
    break;

#ifdef H3D
  case GR_RESOLUTION_640x240_AUTOFLIPPED:
    *xr = 640;
    *yr = 480;
    rv = FXTRUE;
    break;
  case GR_RESOLUTION_800x300_AUTOFLIPPED:
    *xr = 800;
    *yr = 600;
    rv = FXTRUE;
    break;
#endif

  case GR_RESOLUTION_800x600:
    fixVsync = FXTRUE;
#if defined(__DOS32__)
    /* Set up VESA Mode 0x114 - 800x600x16 */
    GDBG_INFO((80, "initAT3DSetVideo:  Setting up VESA mode 800x600x16\n"));
    r.w.ax = 0x4f02;
    r.w.bx = 0x114;
    int386( 0x10, &r, &rOut );
#endif
    *xr = 800;
    *yr = 600;
    rv = FXTRUE;
    break;
  default:
    rv = FXFALSE;
    break;
  }

  if (rv) {
    /* For some reason, AT3D reverses the polarity of  VSYNC for
    512x384 & 800x600, so we need to fix that (NOTE: this code also
    sets the polarity of HSYNC, hence c0 instead of 80)

    See page 342 of _Programmers_Guide_To_The_EGA_And_VGA_Cards, 2nd
    Ed. by Richard F. Ferraro for details on the Miscellaneous Output
    Register. 
    */
    if (fixVsync) {
      FxU16 vgaMiscOutput;

      GDBG_INFO((80, "initAT3DSetVideo:  Setting VSYNC Polarity\n"));

      INP(VGA_MISC_OUTPUT_READ, vgaMiscOutput);
      vgaMiscOutput |= 0xc0;

      OUTP(VGA_MISC_OUTPUT_WRITE, vgaMiscOutput);
    }
  }

  envp = myGetenv("INIT96_AT3D_VSYNC_END");
  if (envp) {
    FxU16 vSyncEnd = 3;
    GDBG_INFO((80, "%s: Setting VSync End\n", FN_NAME));
    vSyncEnd = atoi(envp);
    CRTC_SET(CC_VSYNC_END, vSyncEnd);
  }
  
  envp = myGetenv("INIT96_AT3D_VSYNC_START");
  if (envp) {
    FxU16 vSyncStart = 0;
    GDBG_INFO((80, "%s: Setting VSync Start\n", FN_NAME));
    vSyncStart = atoi(envp);
    CRTC_SET(CC_VSYNC_START, vSyncStart);
  }
  
  GDBG_INFO((80, "%s: returning %s\n", FN_NAME, rv?"FXTRUE":"FXFALSE"));  
  return rv;
#undef FN_NAME  
} /* initAT3DSetVideo */

/*-------------------------------------------------------------------
  Function: initAT3DEnableRegs
  Date: 07-Feb-97
  Implementor(s): dow
  Library: Init Code for AT3D
  Description:
  
  Do all setup necessary to enable direct writes to JR Space
  
  Arguments:
  rd - pointer to register description
  
  Return:
  FXTRUE if success, FXFALSE if Failure
  -------------------------------------------------------------------*/
INITAT3DENTRY(initAT3DEnableRegs,FxBool,(InitRegisterDesc *rd))  
{
#define FN_NAME "initAT3DEnableRegs"
#if !defined( __WIN32__) || defined(INIT_ACCESS_DIRECT)
  LPCOP
    at3dRegs = (LPCOP) rd->hwDep.VG96RegDesc.partnerRegPtr;

  FxU8 tmp;

  GDBG_INFO((80, "%s:\n", FN_NAME));

  GDBG_INFO((80, "%s:  Enabling writes to AT3D registers\n", FN_NAME)); 
  GDBG_INFO((80, "%s:  Setting VGA sequencer to point at flat model control\n", FN_NAME)); 
  GDBG_INFO((80, "%s:  6MB Memory Aperture\n", FN_NAME));
  SEQU_SET(0x1c, 0x27);

  GDBG_INFO((80, "%s:  VGA sequencer to remap control\n", FN_NAME));
  GDBG_INFO((80, "%s:  Map Control Registers into end of Flat Space\n", FN_NAME));
  SEQU_SET(0x1b, 0x24);

  GDBG_INFO((80, "%s:  Waiting for 2D Idle", FN_NAME));
  initAT3D2DIdle(at3dRegs);

  /*
     VG96_MCLKFrequency=50
     DualCycleThreshold=65
     */

  GDBG_INFO((80, "%s:  Setting Graphics clock to 50MHz\n", FN_NAME));
  /* FIXME!!  Add a myGetenv for SST96_MEMCLOCK */
  initAT3DSetMClock(50, at3dRegs);

  /* Wait two VSYNCs in order to give a few milliseconds for the
     on-board MCLK VCO to settle. (LouP)  */
  while(AT3D_IN_VSYNC);
  while(!AT3D_IN_VSYNC); 
  while(AT3D_IN_VSYNC);
  while(!AT3D_IN_VSYNC); 
  
  GDBG_INFO((80, "%s:  Turning on PUMA\n", FN_NAME));
  GDBG_INFO((80, "%s:  UMAControl\n", FN_NAME));
  SETAT3D(at3dRegs->UMAControl, PUMA_CONTROL_ON);

  GDBG_INFO((80, "%s:  GPIOControl\n", FN_NAME));  
  tmp = GET8AT3D(at3dRegs->GPIOControl);
  tmp |= GPIO_PUMA_SRESET_N;
  SETAT3D(at3dRegs->GPIOControl, 0xd0); /* tmp */

  GDBG_INFO((80, "%s:  GPIODirection\n", FN_NAME));
  tmp = GET8AT3D(at3dRegs->GPIODirection);
  tmp |= GPIO_PUMA_SRESET_N;
  SETAT3D(at3dRegs->GPIODirection, 0x10);

  GDBG_INFO((80, "%s:  GPIOLevel\n", FN_NAME));
  tmp = GET8AT3D(at3dRegs->GPIOLevel);
  tmp |= GPIO_PUMA_SRESET_N;    /* active low signal */
  SETAT3D(at3dRegs->GPIOLevel, tmp);

  GDBG_INFO((80, "%s:  FeatureCnctrDisable\n", FN_NAME));
  SETAT3D(at3dRegs->FeatureCnctrDisable, 0x80);

  GDBG_INFO((80, "%s:  GPIOLevel\n", FN_NAME));
  SETAT3D(at3dRegs->GPIOLevel, 0x10);

  GDBG_INFO((80, "%s:  GenFeatureCnctrCtl\n", FN_NAME));
  SETAT3D(at3dRegs->GenFeatureCnctrCtl, 0x10);

  GDBG_INFO((80, "%s:  Setting up AT3D Pixel FIFO Request Points\n", FN_NAME));

  GDBG_INFO((80, "%s:  HiReqMiss\n", FN_NAME));
  SETAT3D(at3dRegs->HiReqMiss, 0x04);

  GDBG_INFO((80, "%s:  HiReqNoMiss\n", FN_NAME));
  SETAT3D(at3dRegs->HiReqNoMiss, 0x04);

  GDBG_INFO((80, "%s:  LoReq\n", FN_NAME));
  SETAT3D(at3dRegs->LoReq, 0x1b);
  
  GDBG_INFO((80, "%s:  Enabling FBI Jr. Space\n", FN_NAME));

  GDBG_INFO((80, "%s:  ExtendedMemoryEnable\n", FN_NAME));
  SETAT3D(at3dRegs->ExtendedMemoryEnable, 0x04);

  GDBG_INFO((80, "%s:  GPIOControl", FN_NAME));
  SETAT3D(at3dRegs->GPIOControl, 0x03);

  GDBG_INFO((80, "%s:  returning FXTRUE\n", FN_NAME));

#endif /* __WIN32__ */

  return FXTRUE;

#undef FN_NAME
} /* initAT3DEnableRegs */

/* FIXME!!! jdt - puma/ram timing fixes need to go here */
INITAT3DENTRY(initAT3DDisableRegs,FxBool,(InitRegisterDesc *rd)) 
{
#define FN_NAME "initAT3DDisableRegs"
  int i,j;
#if !defined( __WIN32__) || defined(INIT_ACCESS_DIRECT)
  FxU32 status;
  FxU8
    gpioControl, gpioDirection, gpioLevel;
#endif  
  
#if 0 /* Fixme! delete after 2.4 */
  LPCOP
    lAT3DRegPtr = (LPCOP) rd->hwDep.VG96RegDesc.partnerRegPtr;
#endif 
  
  GDBG_INFO((80, "%s\n", FN_NAME));
  
#if !defined( __WIN32__) || defined(INIT_ACCESS_DIRECT)

  GDBG_INFO((80, "%s:  Waiting for 3D Idle", FN_NAME));
  do {
    status = GETAT3D(*rd->hwDep.VG96RegDesc.serialStatus);
  } while (status & 1);

  GDBG_INFO((80, "%s:  Disabling PUMA\n", FN_NAME));
  
  GDBG_INFO((80, "%s:  Disabling Memory Refresh\n", FN_NAME));
  initAT3DDisableMemoryRefresh(at3dRegPtr);
  
  GDBG_INFO((80, "%s:  Waiting for %d msec\n", FN_NAME, GINTERVAL));
  initAT3DWait(GINTERVAL);
  
  SETAT3D(at3dRegPtr->UMAControl, PUMA_CONTROL_OFF);
  
  GDBG_INFO((80, "%s:  GPIOControl\n", FN_NAME));
  gpioControl = GET8AT3D(at3dRegPtr->GPIOControl);
  gpioControl |= GPIO_PUMA_SRESET_N;
  SETAT3D(at3dRegPtr->GPIOControl, gpioControl);

  GDBG_INFO((80, "%s:  GPIODirection\n", FN_NAME));
  gpioDirection = GET8AT3D(at3dRegPtr->GPIODirection);
  gpioDirection |= GPIO_PUMA_SRESET_N;
  SETAT3D(at3dRegPtr->GPIODirection, gpioDirection);

  GDBG_INFO((80, "%s:  GPIOLevel\n", FN_NAME));
  gpioLevel = GET8AT3D(at3dRegPtr->GPIOLevel);
  gpioLevel &= ~ GPIO_PUMA_SRESET_N;
  SETAT3D(at3dRegPtr->GPIOLevel, gpioLevel);
  
  GDBG_INFO((80, "%s:  Enabling Memory refresh\n", FN_NAME));
  initAT3DEnableMemoryRefresh(at3dRegPtr);
  
#endif
  /* Undo tile programming regardless */
  if ( tiling ) {
    GDBG_INFO((80, "%s:  Restoring tile registers\n", FN_NAME));
    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl,
                      regWindowSequenceControlSave);

    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidDataOffset, 
                      regWindow0BaseAddressSave);
    
    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorHor, 
                      regWindow0StretchFactorHor1Save);

    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetHor, 
                      regWindow0StretchFactorHor2Save);

    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorVer, 
                      regWindow0StretchFactorVer1Save);
    
    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetVer, 
                      regWindow0StretchFactorVer2Save);
    
    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidControl, 
                      regWindow0ControlSave);

    GDBG_INFO((80, "Restoring all tiles\n"));
    for (i=0, j=0; i<MAX_TILES; i++) {
      int k;
      volatile FxU16 *tileRegPtr = (FxU16 *) &at3dRegPtr->TileRegsSet[i];
      for (k=0; k<8; k++, j++) {
        SETAT3D(tileRegPtr[k], tileRegSave[j]);
      }
    } /* end for i */
    tiling = 0;
  }

  GDBG_INFO((80, "%s:  returning FXTRUE\n", FN_NAME));
  
  return FXTRUE;
#undef FN_NAME
} /* initAT3DDisableRegs */

/*-------------------------------------------------------------------
  Function: initAT3DUseTiles
  Date: 07-Feb-97 (Moved into initat3d from init96)
  Implementor(s): jdt, paik, murali, dow
  Library: Init Code for AT3D
  Description:
  
  Arguments:
  frontBuffer:          FB offset of front buffer
  backBuffer:           FB offset of back buffer
  strideInBytes:        Tile width in bytes

  Return:
  1 if successful
  -------------------------------------------------------------------*/
#ifdef H3D
  // save framebufferinfo for enabling/disabling h3d
static InitRegisterDesc *h3drd=0;
static FxU32 h3dbuffers[3], h3dnBuffers, h3dstrideInBytes, h3dWidth, h3dHeight, h3dleft, h3dright, h3dbottom;

INITAT3DENTRY(initAT3DUseTiles, int, (InitRegisterDesc *rd,
                                      FxU32 *buffers,
                                      FxU32 nBuffers,
                                                                                            FxU32 strideInBytes,
                                                                                            FxU32 Width, FxU32 Height,
                                                                                            FxU32 PageFlipping))
#else
INITAT3DENTRY(initAT3DUseTiles, int, (InitRegisterDesc *rd,
                                      FxU32 *buffers,
                                      FxU32 nBuffers,
                                      FxU32 strideInBytes,
                                      FxU32 Width, FxU32 Height)) 
#endif
{
#define FN_NAME "initAT3DUseTiles"

  FxU32 left   = 0; 
  FxU32 right  = Width;
  FxU32 bottom = Height-1;
  unsigned int   i, j, ntimes;

  GDBG_INFO((80, "%s:\n", FN_NAME)); 
  GDBG_INFO((80, "%s:  Setting up overlays!\n", FN_NAME));

#ifdef H3D
  h3drd = rd;
  for(i=0;i<nBuffers;i++)
    h3dbuffers[i]=buffers[i];
  h3dnBuffers=nBuffers, h3dstrideInBytes=strideInBytes, h3dWidth=Width, h3dHeight=Height;
  h3dleft = left, h3dright = right, h3dbottom = bottom;
#endif
  tiling = 1;

#ifdef GDBG_INFO_ON
  for (i = 0; i < nBuffers; i++)
    GDBG_INFO((80, "%s:  Buffer %d = 0x%x\n", FN_NAME, i, buffers[i]));
#endif

  GDBG_INFO((80, "%s:  Saving all tiles\n", FN_NAME));
  for (i=0, j=0; i<MAX_TILES; i++) {
    int k;
    volatile FxU16 *tileRegPtr = (FxU16 *) &at3dRegPtr->TileRegsSet[i];
    for (k=0; k<8; k++) {
      tileRegSave[j++] = GET16AT3D(tileRegPtr[k]);
    }
  } /* end for i */

  GDBG_INFO((80, "%s:  Zeroing out all tiles\n", FN_NAME));
    for (i=0; i<MAX_TILES; i++) {
      int k;
      volatile FxU16 *tileRegPtr = (FxU16 *) &at3dRegPtr->TileRegsSet[i];
      for (k=0; k<8; k++) {
        SETAT3D(tileRegPtr[k], 0);
      }
    } /* end for i */
  
  /*---------------------------------
    VIDEO WINDOW
    ---------------------------------*/
  
  
  /* Set up tile sequence control register 
     [12]    - stereo enable                 - 0x0 - disable
     [11:10] - buffercount                   - 0x1 - single buffer
     [9]     - swap buffer on 3D swap signal - 0x1 - enable
     [8]     - buffer swap using TV input    - 0x0 - disable
     [7:4]   - tile sequence length          - 0x1 - magic
     [3:0]   - tile sequence base            - 0x0 - magic
     */
  GDBG_INFO((80, "%s:  SEtting up tile sequence control register\n", FN_NAME));
  {
    int a = 0, b = 0;
    ntimes = 0;
    while( !( a && !b ) ) {     /* pass on falling edge of VRETRACE_ACTIVE */
      a = b;
      b = _inp( VGA_INPUT_STATUS_1C ) & VIS1C_VERTICAL_RETRACE_ACTIVE;
      if (ntimes++ > 1000) {
        GDBG_INFO((80, "%s:  Bailing out after %d checks of VRETRACE check\n", FN_NAME, ntimes));
        break;
      } 
    }
  }
  
  while (GET16AT3D(at3dRegPtr->CurrentVerticalPosition) > 20) {
    if (ntimes++ > 1000) {
      GDBG_INFO((80, "%s:  Bailing out after %d checks of register 0x1fa\n", FN_NAME, ntimes));
      break;
    }
  }
  
  GDBG_INFO((80, "%s, Setting Window Sequence Control Register to %x\n", FN_NAME, 0x0610)); 
  regWindow0ControlSave =
        GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl);
#ifdef H3D
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl, (PageFlipping?(FxU16)(0x1610):(FxU16)(0x0610)));
#else
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl,
                (FxU16)(0x0610));
#endif
  
  /* 1: video data stride 
   * Set stride of data buffer in DWORDS
   */
  GDBG_INFO((80, "%s:  Setting Window 0 Video Data Stride to 0x%x\n", FN_NAME, strideInBytes >> 2));
  regWindow0BaseAddressSave =
        GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidDataOffset);
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidDataOffset,
                (FxU16)(strideInBytes>>2));
  
  /* 2: video scale factor horizontal
   * A value of zero represents a factor of 1.0
   */
  GDBG_INFO((80, "%s:  Setting Window 0 Horizontal Video Scale Factor to 0x%x \n", FN_NAME, 0x0000));
  regWindow0StretchFactorHor1Save =
    GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorHor);
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorHor, (FxU16)0x0000);
  
  /* 3: video offset factor horizontal 
   * 0xFFF - 0x0 * (X) & 0xFFFh = 0xFFF - set for no scale
   */
  GDBG_INFO((80, "%s:  Setting Window 0 Horizontal Video Offset Factor to 0x%x\n", FN_NAME, 0xfff));

  regWindow0StretchFactorHor2Save = 
    GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetHor);
    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetHor, (FxU16)0xFFF);
  
  /* 4: video scale factor vertical
   * A value of zero represents a factor of 1.0
   */
  GDBG_INFO((80, "%s:  Setting Window 0  Vertical Video Scale Factor to 0x%x\n", FN_NAME, 0x0000));
#if 0 /* Fixme I think this line is bogus, remove after 2.4 !!! XXX ??? */
  at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorVer = (FxU16)0x0000;
#endif
  regWindow0StretchFactorVer1Save =
    GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorVer);
#ifdef H3D
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorVer, (PageFlipping?(FxU16)0x07FA:(FxU16)0x0000));
#else
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorVer, (FxU16)0x0000);
#endif

  /* 5: video offset factor vertical 
   * 0xFFF - 0x0 * (X) & 0xFFFh = 0xFFF - set for no scale
   */
  GDBG_INFO((80, "%s:  Setting  Window 0 Vertical Video Offset Factor \n", FN_NAME, 0xfff));
  regWindow0StretchFactorVer2Save =
    GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetVer);
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetVer, (FxU16)0xFFF);

  
  /* 6: control reg, pixel depth, window format, enable bits 
     [14]  - Chromakey     - 0x0 - Disable
     [13]  - Reservered    - 0x0
     [12]  - Smoothing     - 0x0 - disable
     [11]  - VStretch lerp - 0x0 - disable
     [10]  - HStretch lerp - 0x0 - disable
     [9]   - Stretch       - 0x0 - disable
     [8]   - YUV-RGB       - 0x0 - disable
     [7]   - Reservered    - 0x0
     [6:4] - color format  - 0x2 - RGB
     [3:1] - Pixel Depth   - 0x5 - 16 BPP
     [0]   - enable window - 0x1 - enable
   */
  GDBG_INFO((80, "%s:  Window 0 Control to 0x%x\n", FN_NAME, 0x20 | 0xa | 0x1));

  regWindow0ControlSave =
        GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidControl);
#ifdef H3D
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidControl, (PageFlipping ? (0x0200|0x0020|0x000A|0x0001) : (0x0020|0x000A|0x0001)));
#else
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidControl,
                 0x0020 | 0x000A | 0x0001);
#endif
  
#ifdef H3D  
  /*---------------------------------
    CONFIGURE TILES 0, 1, 2 & 3
    ---------------------------------*/
  for (i=0; i < nBuffers*2; i++) {
    FxU32 buf;
    if (PageFlipping) {
                buf = buffers[i>>1];
                if (i & 1)      // bottom half of page
                        buf += (strideInBytes*((Height)>>1));
    } else {
      buf = buffers[i];
      if (i == nBuffers) break;
    }
#else
  /*---------------------------------
    CONFIGURE TILES
    ---------------------------------*/
  for (i=0; i < nBuffers; i++) {
    FxU32
      buf = buffers[i];
#endif

    GDBG_INFO((80, "%s:  Configuring tile %d ======================\n", FN_NAME, i));
  
    /* display position left register 
     * Sets the leftmost column of pixels in the tile
     * this column lies inside the tile
     * [10:0] - integer column - 0
     */
    GDBG_INFO((80, "%s:  Setting Tile %d Left Register to 0x%x\n", FN_NAME, i, left));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DisplayPositionLeft, (FxU16)left);
  
    /* display position right register
     * sets the rightmost column of pixels in the tile
     * this column lies outside the tile
     * [10:0] - integer column - 640
     */
    GDBG_INFO((80, "%s:  Setting Tile %d Right Register to 0x%x\n", FN_NAME, i, right));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DisplayPositionRight, (FxU16)right);
  
    /* display position bottom register
     * sets the bottom row of pixels in the tile
     * this row of pixels lies inside the tile
     * [10:0] - bottomost tile - 480-1;
     */    
    GDBG_INFO((80, "%s:  Setting Tile %d Bottom Register to 0x%x\n", FN_NAME, i, bottom));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DisplayPositionBottom, (FxU16)bottom);
  
    /* tile data width register
     * sets the width of the source tile data, not the display
     * [10:0] - width in pixels( not bytes ) - 640
     */
    GDBG_INFO((80, "%s:  Setting Tile %d Data Width register to 0x%x\n", FN_NAME, i, strideInBytes >> 1));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DataWidth, (FxU16)(strideInBytes>>1));
  
    /* tile data location 
     * sets the address in bytes of the top-left corner
     * of the video tile data where it is stored, not the
     * display window.
     * [21:0] - byte address - offset - 0
     */
    GDBG_INFO((20, "%s:  Setting Tile %d Data location to 0x%x\n", FN_NAME, i, buf));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DataLocationLo, (FxU16)(buf & 0xFFFF));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DataLocationHi,
                                                 (FxU8)((buf >> 16) & 0xFF));
  
    /* tile control register 
     * [4]   - rightmost tile  - 0x1 - only
     * [3]   - reserved        - 0 
     * [2:0] - vWindow select  - 0x1 - window 1
     */
    GDBG_INFO((80, "%s: Setting Tile %d Control Register to 0x%x\n", FN_NAME, i, 0x11));
    SETAT3D(at3dRegPtr->TileRegsSet[i].Control, 0x11);
  } /* end for i */
  
  /*  set window sequence control to disable tile swap on 3d request 
      [12]    - stereo enable                 - 0x0 - disable
      [11:10] - buffercount                   - 0x1 - single buffer
      [9]     - swap buffer on 3D swap signal - 0x0 - enable
      [8]     - buffer swap using TV input    - 0x0 - disable
      [7:4]   - tile sequence length          - 0x1 - magic
      [3:0]   - tile sequence base            - 0x0 - magic
   */
#ifdef H3D
  GDBG_INFO((80, "%s:  Disabling swap on 3d request\n", FN_NAME));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl, (PageFlipping ? (FxU16)(0x1410) : (FxU16)(0x0410)));
#else
  GDBG_INFO((80, "%s:  Disabling swap on 3d request\n", FN_NAME));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl,
                                         (FxU16)(0x0410));
#endif
  
  /*  set window sequence control to single tile 
      [12]    - stereo enable                 - 0x0 - disable
      [11:10] - buffercount                   - 0x1 - single buffer
      [9]     - swap buffer on 3D swap signal - 0x0 - enable
      [8]     - buffer swap using TV input    - 0x0 - disable
      [7:4]   - tile sequence length          - 0x1 - magic
      [3:0]   - tile sequence base            - 0x0 - magic
   */
#ifdef H3D
  GDBG_INFO((80, "%s:  Setting window sequence control to single tile\n", FN_NAME));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl, (PageFlipping ? (FxU16)(0x1010) : (FxU16)(0x0010)));
#else
  GDBG_INFO((80, "%s:  Setting Tile Sequence Control to single tile\n", FN_NAME));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl,
                                (FxU16)(0x0010));
#endif
  
  /*  set window sequence control to enable tile swap on 3d request 
      [12]    - stereo enable                 - 0x0 - disable
      [11:10] - buffercount                   - 0x0 - single buffer
      [9]     - swap buffer on 3D swap signal - 0x1 - enable
      [8]     - buffer swap using TV input    - 0x0 - disable
      [7:4]   - tile sequence length          - 0x1 - magic
      [3:0]   - tile sequence base            - 0x0 - magic
   */
#ifdef H3D
  GDBG_INFO((80, "%s:  Enabling swap on 3d request\n", FN_NAME));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl, (PageFlipping ? (FxU16)(0x1210) : (FxU16)(0x0210)));
#else
  GDBG_INFO((80, "%s:  Enabling swap on 3d request\n", FN_NAME));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl,
                                 (FxU16)(0x0210));
#endif
 
  GDBG_INFO((80, "%s:  Enabling %s buffering\n", FN_NAME,
             nBuffers > 2 ? "triple" : "double"));

    /* set window sequence control to enable double buffering 
       [12]    - stereo enable                 - 0x0 - disable
       [11:10] - buffercount                   - 0x1 or 0x2
                          (double or triple buffer, respectively)
       [9]     - swap buffer on 3D swap signal - 0x1 - enable
       [8]     - buffer swap using TV input    - 0x0 - disable
       [7:4]   - tile sequence length          - 0x1 - magic
       [3:0]   - tile sequence base            - 0x0 - magic
       */
  GDBG_INFO((80, "******* nBuffers = %d\n", nBuffers));
#ifdef H3D
    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl,
      (
      ((FxU16)(0x0010)) |
      (PageFlipping ? (FxU16)(0x1000) : (FxU16)(0x0000)) |
      ((nBuffers>2) ? (FxU16)(0x0a00) : (FxU16)(0x0600))
      ));
#else
  if (nBuffers > 2) {
    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl,
            (FxU16)(0x0a10));
  } else 
    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl,
            (FxU16)(0x0610));
#endif

  /* wait a few vsyncs */
  /*           VVVVV--WTF? (CHD) */
  for ( i = 0; i < 1; i++ ) {
    int a = 0, b = 0;
    ntimes = 0;
    while( !( a && !b ) ) {     /* pass on falling edge of VRETRACE_ACTIVE */
      if (ntimes >= 1000) {
        GDBG_INFO((80, "Bailing out of vsync check after %d tries\n", FN_NAME, ntimes));
        break;
      }
      a = b;
      b = _inp( VGA_INPUT_STATUS_1C ) & VIS1C_VERTICAL_RETRACE_ACTIVE;
      
    }
  }
  
  GDBG_INFO((80, "%s:  returning %d\n", FN_NAME, 1));

  return 1;

#undef FN_NAME
} /* initAT3DUseTiles */
 

/*-------------------------------------------------------------------
  Function: initAT3DGetInfo
  Date: 28-Apr-97
  Implementor(s): pgj
  Library: Init Code for AT3D
  Description:

  get information about at3d
  
  Arguments:
  info - pointer to info struct
  
  Return:
  FXTRUE if success, FXFALSE if Failure
  -------------------------------------------------------------------*/
INITAT3DENTRY(initAT3DGetInfo,FxBool,(VG96Info *info))  
{
#define FN_NAME "initAT3DGetInfo"
#ifdef __DOS32__
    union REGS inR, outR;
#endif /* __DOS32__ */
    FxU32 bytes;

    GDBG_INFO((80, "%s:\n", FN_NAME));

    info->vgaChip = 0;
    info->vg96Rev = 0;

#ifdef __DOS32__
    inR.w.ax = 0x3002;
    int386(0x10, &inR, &outR);
    bytes = outR.h.al;
#else
#ifdef __linux__
    bytes = myGetenv("SST_VRAM_4MB") ? 0x40 : 0x20;
#else
#define PROMOTIONESCAPE                 0x80DE
    {
        HDC hdc = GetDC(GetActiveWindow());
        bytes = ExtEscape(hdc, PROMOTIONESCAPE, sizeof("MS"), "MS", 0, NULL);
    }
#endif
#endif /* __DOS32__ */

    bytes <<= 16;
    info->vg96Ram = bytes;

    info->nTFX    = 1;
    info->tfxRev  = 1;
    info->tfxRam  = 2;
    GDBG_INFO((135, "%s: %8.8x bytes\n", FN_NAME, bytes));

    return FXTRUE;

#undef FN_NAME
} /* initAT3DGetInfo */

#ifdef H3D
/*-------------------------------------------------------------------
  Function: initAT3DSetTiles
  Date: 28-Apr-97
  Implementor(s):
  Library: Init Code for AT3D
  Description:

  get information about at3d
  
  Arguments:
  info - pointer to info struct
  
  Return:
  FXTRUE if success, FXFALSE if Failure
  -------------------------------------------------------------------*/
void initAT3DSetTiles(FxU32 PageFlipping, FxU32 nBuffers) 
{
#define FN_NAME "initAT3DSetTiles"
  FxU32   i, ntimes;

  if (!h3drd) return; // not inited yet

#ifdef GDBG_INFO_ON
  for (i = 0; i < nBuffers; i++)
    GDBG_INFO((80, "%s:  Buffer %d = 0x%x\n", FN_NAME, i, h3dbuffers[i]));
#endif

  GDBG_INFO((80, "%s:  Zeroing out all tiles\n", FN_NAME));
    for (i=0; i<MAX_TILES; i++) {
      int k;
      volatile FxU16 *tileRegPtr = (FxU16 *) &at3dRegPtr->TileRegsSet[i];
      for (k=0; k<8; k++) {
        SETAT3D(tileRegPtr[k], 0);
      }
    } /* end for i */
  
  /*---------------------------------
    VIDEO WINDOW
    ---------------------------------*/
  
  
  /* Set up tile sequence control register 
     [12]    - stereo enable                 - 0x0 - disable
     [11:10] - buffercount                   - 0x1 - single buffer
     [9]     - swap buffer on 3D swap signal - 0x1 - enable
     [8]     - buffer swap using TV input    - 0x0 - disable
     [7:4]   - tile sequence length          - 0x1 - magic
     [3:0]   - tile sequence base            - 0x0 - magic
     */
  GDBG_INFO((80, "%s:  SEtting up tile sequence control register\n", FN_NAME));
  {
    int a = 0, b = 0;
    ntimes = 0;
    while( !( a && !b ) ) {     /* pass on falling edge of VRETRACE_ACTIVE */
      a = b;
      b = _inp( VGA_INPUT_STATUS_1C ) & VIS1C_VERTICAL_RETRACE_ACTIVE;
      if (ntimes++ > 1000) {
        GDBG_INFO((80, "%s:  Bailing out after %d checks of VRETRACE check\n", FN_NAME, ntimes));
        break;
      } 
    }
  }
  
  while (GET16AT3D(at3dRegPtr->CurrentVerticalPosition) > 20) {
    if (ntimes++ > 1000) {
      GDBG_INFO((80, "%s:  Bailing out after %d checks of register 0x1fa\n", FN_NAME, ntimes));
      break;
    }
  }
  
  GDBG_INFO((80, "%s, Setting Window Sequence Control Register to %x\n", FN_NAME, 0x0610)); 
  regWindow0ControlSave =
        GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl);
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl, (PageFlipping?(FxU16)(0x1610):(FxU16)(0x0610)));
  
  /* 1: video data stride 
   * Set stride of data buffer in DWORDS
   */
  GDBG_INFO((80, "%s:  Setting Window 0 Video Data Stride to 0x%x\n", FN_NAME, h3dstrideInBytes >> 2));
  regWindow0BaseAddressSave =
        GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidDataOffset);
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidDataOffset,
                (FxU16)(h3dstrideInBytes>>2));
  
  /* 2: video scale factor horizontal
   * A value of zero represents a factor of 1.0
   */
  GDBG_INFO((80, "%s:  Setting Window 0 Horizontal Video Scale Factor to 0x%x \n", FN_NAME, 0x0000));
  regWindow0StretchFactorHor1Save =
    GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorHor);
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorHor, (FxU16)0x0000);
  
  /* 3: video offset factor horizontal 
   * 0xFFF - 0x0 * (X) & 0xFFFh = 0xFFF - set for no scale
   */
  GDBG_INFO((80, "%s:  Setting Window 0 Horizontal Video Offset Factor to 0x%x\n", FN_NAME, 0xfff));

  regWindow0StretchFactorHor2Save = 
    GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetHor);
    SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetHor, (FxU16)0xFFF);
  
  /* 4: video scale factor vertical
   * A value of zero represents a factor of 1.0
   */
  GDBG_INFO((80, "%s:  Setting Window 0  Vertical Video Scale Factor to 0x%x\n", FN_NAME, 0x0000));
  regWindow0StretchFactorVer1Save =
    GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorVer);
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleFactorVer, (PageFlipping?(FxU16)0x07FA:(FxU16)0x0000));

  /* 5: video offset factor vertical 
   * 0xFFF - 0x0 * (X) & 0xFFFh = 0xFFF - set for no scale
   */
  GDBG_INFO((80, "%s:  Setting  Window 0 Vertical Video Offset Factor \n", FN_NAME, 0xfff));
  regWindow0StretchFactorVer2Save =
    GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetVer);
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidScaleOffsetVer, (FxU16)0xFFF);

  
  /* 6: control reg, pixel depth, window format, enable bits 
     [14]  - Chromakey     - 0x0 - Disable
     [13]  - Reservered    - 0x0
     [12]  - Smoothing     - 0x0 - disable
     [11]  - VStretch lerp - 0x0 - disable
     [10]  - HStretch lerp - 0x0 - disable
     [9]   - Stretch       - 0x0 - disable
     [8]   - YUV-RGB       - 0x0 - disable
     [7]   - Reservered    - 0x0
     [6:4] - color format  - 0x2 - RGB
     [3:1] - Pixel Depth   - 0x5 - 16 BPP
     [0]   - enable window - 0x1 - enable
   */
  GDBG_INFO((80, "%s:  Window 0 Control to 0x%x\n", FN_NAME, 0x20 | 0xa | 0x1));

  regWindow0ControlSave =
        GET16AT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidControl);
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].VidControl, (PageFlipping ? (0x0200|0x0020|0x000A|0x0001) : (0x0020|0x000A|0x0001)));
  /*---------------------------------
    CONFIGURE TILES 0, 1, 2 & 3
    ---------------------------------*/
  for (i=0; i < nBuffers*2; i++) {
    FxU32 buf;
    if (PageFlipping) {
                buf = h3dbuffers[i>>1];
                if (i & 1)      // bottom half of page
                        buf += (h3dstrideInBytes*((h3dHeight)>>1));
    } else {
      buf = h3dbuffers[i];
      if (i == nBuffers) break;
    }

    GDBG_INFO((80, "%s:  Configuring tile %d ======================\n", FN_NAME, i));
  
    /* display position left register 
     * Sets the leftmost column of pixels in the tile
     * this column lies inside the tile
     * [10:0] - integer column - 0
     */
    GDBG_INFO((80, "%s:  Setting Tile %d Left Register to 0x%x\n", FN_NAME, i, h3dleft));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DisplayPositionLeft, (FxU16)h3dleft);
  
    /* display position right register
     * sets the rightmost column of pixels in the tile
     * this column lies outside the tile
     * [10:0] - integer column - 640
     */
    GDBG_INFO((80, "%s:  Setting Tile %d Right Register to 0x%x\n", FN_NAME, i, h3dright));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DisplayPositionRight, (FxU16)h3dright);
  
    /* display position bottom register
     * sets the bottom row of pixels in the tile
     * this row of pixels lies inside the tile
     * [10:0] - bottomost tile - 480-1;
     */    
    GDBG_INFO((80, "%s:  Setting Tile %d Bottom Register to 0x%x\n", FN_NAME, i, h3dbottom));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DisplayPositionBottom, (FxU16)h3dbottom);
  
    /* tile data width register
     * sets the width of the source tile data, not the display
     * [10:0] - width in pixels( not bytes ) - 640
     */
    GDBG_INFO((80, "%s:  Setting Tile %d Data Width register to 0x%x\n", FN_NAME, i, h3dstrideInBytes >> 1));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DataWidth, (FxU16)(h3dstrideInBytes>>1));
  
    /* tile data location 
     * sets the address in bytes of the top-left corner
     * of the video tile data where it is stored, not the
     * display window.
     * [21:0] - byte address - offset - 0
     */
    GDBG_INFO((20, "%s:  Setting Tile %d Data location to 0x%x\n", FN_NAME, i, buf));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DataLocationLo, (FxU16)(buf & 0xFFFF));
    SETAT3D(at3dRegPtr->TileRegsSet[i].DataLocationHi,
                                                 (FxU8)((buf >> 16) & 0xFF));
  
    /* tile control register 
     * [4]   - rightmost tile  - 0x1 - only
     * [3]   - reserved        - 0 
     * [2:0] - vWindow select  - 0x1 - window 1
     */
    GDBG_INFO((80, "%s: Setting Tile %d Control Register to 0x%x\n", FN_NAME, i, 0x11));
    SETAT3D(at3dRegPtr->TileRegsSet[i].Control, 0x11);
  } /* end for i */
  
  /*  set window sequence control to disable tile swap on 3d request 
      [12]    - stereo enable                 - 0x0 - disable
      [11:10] - buffercount                   - 0x1 - single buffer
      [9]     - swap buffer on 3D swap signal - 0x0 - enable
      [8]     - buffer swap using TV input    - 0x0 - disable
      [7:4]   - tile sequence length          - 0x1 - magic
      [3:0]   - tile sequence base            - 0x0 - magic
   */
  GDBG_INFO((80, "%s:  Disabling swap on 3d request\n", FN_NAME));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl, (PageFlipping ? (FxU16)(0x1410) : (FxU16)(0x0410)));
  
  /*  set window sequence control to single tile 
      [12]    - stereo enable                 - 0x0 - disable
      [11:10] - buffercount                   - 0x1 - single buffer
      [9]     - swap buffer on 3D swap signal - 0x0 - enable
      [8]     - buffer swap using TV input    - 0x0 - disable
      [7:4]   - tile sequence length          - 0x1 - magic
      [3:0]   - tile sequence base            - 0x0 - magic
   */
  GDBG_INFO((80, "%s:  Setting window sequence control to single tile\n", FN_NAME));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl, (PageFlipping ? (FxU16)(0x1010) : (FxU16)(0x0010)));
  
  /*  set window sequence control to enable tile swap on 3d request 
      [12]    - stereo enable                 - 0x0 - disable
      [11:10] - buffercount                   - 0x0 - single buffer
      [9]     - swap buffer on 3D swap signal - 0x1 - enable
      [8]     - buffer swap using TV input    - 0x0 - disable
      [7:4]   - tile sequence length          - 0x1 - magic
      [3:0]   - tile sequence base            - 0x0 - magic
   */
  GDBG_INFO((80, "%s:  Enabling swap on 3d request\n", FN_NAME));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl, (PageFlipping ? (FxU16)(0x1210) : (FxU16)(0x0210)));
 
  GDBG_INFO((80, "%s:  Enabling %s buffering\n", FN_NAME,
             nBuffers > 2 ? "triple" : "double"));

    /* set window sequence control to enable double buffering 
       [12]    - stereo enable                 - 0x0 - disable
       [11:10] - buffercount                   - 0x1 or 0x2
                          (double or triple buffer, respectively)
       [9]     - swap buffer on 3D swap signal - 0x1 - enable
       [8]     - buffer swap using TV input    - 0x0 - disable
       [7:4]   - tile sequence length          - 0x1 - magic
       [3:0]   - tile sequence base            - 0x0 - magic
       */
  GDBG_INFO((80, "******* nBuffers = %d\n", nBuffers));
  SETAT3D(at3dRegPtr->V2.VidWindowV2Set[0].TileSequenceControl,
    (
    ((FxU16)(0x0010)) |
    (PageFlipping ? (FxU16)(0x1000) : (FxU16)(0x0000)) |
    ((nBuffers>2) ? (FxU16)(0x0a00) : (FxU16)(0x0600))
    ));
  
  /* wait a few vsyncs */
  /*           VVVVV--WTF? (CHD) */
  for ( i = 0; i < 1; i++ ) {
    int a = 0, b = 0;
    ntimes = 0;
    while( !( a && !b ) ) {     /* pass on falling edge of VRETRACE_ACTIVE */
      if (ntimes >= 1000) {
        GDBG_INFO((80, "Bailing out of vsync check after %d tries\n", FN_NAME, ntimes));
        break;
      }
      a = b;
      b = _inp( VGA_INPUT_STATUS_1C ) & VIS1C_VERTICAL_RETRACE_ACTIVE;
      
    }
  }
  
  GDBG_INFO((80, "%s:  returning %d\n", FN_NAME, 1));

#undef FN_NAME
} /* initAT3DSetTiles */

#endif /* H3D */
