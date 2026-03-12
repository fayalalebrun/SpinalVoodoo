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

#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */

#include <3dfx.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __linux__
#ifdef __DJGPP__
#include <fxdpmi.h>
#else
#include <conio.h>
#endif
#else
#include <fxpci.h>
#define _inp(port) pioInByte(port);
#define _outp(port,data) pioOutByte(port, data);
#define _outpw(port,data) pioOutWord(port, data);
#endif
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
static inline void _outpw_asm (unsigned short _port, unsigned short _data) {
  __asm__ __volatile__ ("outw %w0,%w1" : : "a" (_data), "Nd" (_port));
}
#define _outpw  _outpw_asm
#define _inp  _inp_asm
#define _outp  _outp_asm
#endif
#endif /* __WIN32__ */

#define INIT_MCRX_LIB
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <gdebug.h>
#include <init.h>


#include <initvga.h>
#include "fxinit96.h"
#include "initmcrx.h"

#ifndef BIT
#define BIT(a)       (1 << a)
#endif

/* Global Data */
static Init96HALData mcrxHALData = {
  0x2000000,
  initMCRXAperture,
  initMCRXSetVideo,
  initMCRXEnableRegs,
  initMCRXDisableRegs,
  initMCRXWrapFIFO,
  initMCRXUseTiles,
  initMCRXGetInfo,
  NULL, /* initMCRXSwapTiles */
  NULL
};


#if 0 /* ??? needed??? XXX */
static FxBool tiling;
#endif

/*-------------------------------------------------------------------
  Function: intiMCRXGetHALData
  Date: 09-May-97
  Implementor(s): dow
  Library: Init Code for MCRX
  Description:
  
  Arguments:
  
  Return:
        address of mcrxHALData
  -------------------------------------------------------------------*/
INITMCRXENTRY(initMCRXGetHALData, Init96HALData *, (void) )
{
#define FN_NAME "intiMCRXGetHALData"
  return &mcrxHALData;
#undef FN_NAME
} /* intiMCRXGetHALData */

/*-------------------------------------------------------------------
  Function: initMCRXAperture
  Date: 3-May-97
  Implementor(s): dow
  Library: Init Code for MCRX
  Description:
  Initialize rd pointers to access apertures
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
INITMCRXENTRY(initMCRXAperture, FxBool , (InitRegisterDesc *rd) )
{
#define FN_NAME "initMCRXAperture"
  FxBool
    rVal = FXTRUE;

  GDBG_INFO((80, "%s:\n", FN_NAME));

  rd->hwDep.VG96RegDesc.partnerRegPtr = (FxU32 *)
    ((FxU32) rd->hwDep.VG96RegDesc.baseAddress +
     0x2000000 - 0x20000);

  GDBG_INFO((80, "%s:  Setting partnerRegPtr to 0x%x\n", 
             FN_NAME, rd->hwDep.VG96RegDesc.partnerRegPtr));
  
#if 1
  rd->hwDep.VG96RegDesc.serialStatus = (FxU32 *)
    ((FxU32) rd->hwDep.VG96RegDesc.partnerRegPtr + 0x700);
#else
  GDBG_INFO((80, "%s:  Working around invalid serial status\n", FN_NAME));
  rd->hwDep.VG96RegDesc.serialStatus =
    &rd->hwDep.VG96RegDesc.baseAddress[0x100000];
#endif
  GDBG_INFO((80, "%s:  Setting seial status to 0x%x\n", FN_NAME, rd->hwDep.VG96RegDesc.serialStatus));

  /* BOGUS */
  rd->hwDep.VG96RegDesc.fifoApertureBase =
    (FxU32 *) (((FxU32)rd->hwDep.VG96RegDesc.baseAddress) + 0x400000);
  
  GDBG_INFO((80, "%s:  returning TRUE\n", FN_NAME));
  
  return rVal;

#undef FN_NAME
} /* initMCRXAperture */

/* FIXME !! jdt - need to add more resolutions to dos */
INITMCRXENTRY(initMCRXSetVideo, FxBool, (GrScreenResolution_t res, GrScreenRefresh_t refresh, FxU32 *xr, FxU32 *yr )) 
{
#define FN_NAME "initMCRXSetVideo"
  FxBool rv = FXTRUE;
#if defined(INIT_ACCESS_DIRECT) && defined(__DOS32__)
  union REGS r0, r1;
#endif
  
  GDBG_INFO((80, "%s: \n", FN_NAME));

#if defined(INIT_ACCESS_DIRECT) && defined(__DOS32__)
  switch( res ) {
  case GR_RESOLUTION_320x200:
    /* Set up VESA Mode 0x136 - 320x200x16 */
    r1.w.ax = 0x4f02;
    r1.w.bx = 0x0136;
    *xr = 320;
    *yr = 200;
    rv = FXTRUE;
    break;
  case GR_RESOLUTION_320x240:
    /* Set up VESA Mode 0x137 - 320x240x16 */
    r1.w.ax = 0x4f02;
    r1.w.bx = 0x0137;
    *xr = 320;
    *yr = 240;
    rv = FXTRUE;
    break;
  case GR_RESOLUTION_512x384:
    /* Set up VESA Mode 0x139 - 512x384x16 */
    r1.w.ax = 0x4f02;
    r1.w.bx = 0x0139;
    *xr = 512;
    *yr = 384;
    rv = FXTRUE;
    break;
  case GR_RESOLUTION_640x480:
    /* Set up VESA Mode 0x111 - 640x480x16 */
    r1.w.ax = 0x4f02;
    r1.w.bx = 0x0111;
    *xr = 640;
    *yr = 480;
    rv = FXTRUE;
    break;
  case GR_RESOLUTION_800x600:
    /* Set up VESA Mode 0x114 - 800x600x16 */
    r1.w.ax = 0x4f02;
    r1.w.bx = 0x0114;
    *xr = 800;
    *yr = 600;
    rv = FXTRUE;
    break;
  default:
    rv = FXFALSE;
    break;
  }

  if (rv == FXTRUE) {
    GDBG_INFO((80, "initMCRXSetVideo: Enabling VESA modes\n"));
    r0.w.ax = 0x6483;
    r0.w.bx = 0x0060;
    int386( 0x10, &r0, &r0 );

    GDBG_INFO((80, "initMCRXSetVideo: Setting video mode %dx%dx16\n",
                                                        *xr, *yr));
    int386( 0x10, &r1, &r1 );
  }

#endif
  GDBG_INFO((80, "%s: returning %s\n", FN_NAME, rv?"TRUE":"FALSE"));  
  return rv;
#undef FN_NAME  
} /* initMCRXSetVideo */

/*-------------------------------------------------------------------
  Function: initMCRXEnableRegs
  Date: 07-Feb-97
  Implementor(s): dow
  Library: Init Code for MCRX
  Description:

  Do all setup necessary to enable direct writes to JR Space
  
  Arguments:
  rd - pointer to register description
  
  Return:
  True if success, False if Failure
  -------------------------------------------------------------------*/
INITMCRXENTRY(initMCRXEnableRegs, FxBool, (InitRegisterDesc *rd))  
{
#define FN_NAME "initMCRXEnableRegs"
  FxU32 
    tmp, u;
  FxU8
    crtcIndex;
  FxU16
    protState;

  const char *envVal;

  GDBG_INFO((80, "%s:\n", FN_NAME));

  GDBG_INFO((80, "%s:  Unprotect\n", FN_NAME));

  GDBG_INFO((80, "%s:    Save CRTC Index\n", FN_NAME));
  INP(0x3c4, crtcIndex);

  GDBG_INFO((80, "%s:    Save Extended Registers Protection State\n",
             FN_NAME));
  
  OUTP(0x3d4, 0x19);
  INP(0x3d5, protState);

  protState <<= 8;
  protState |= 0x19;

  GDBG_INFO((80, "%s:  Enabling Access to Extended Registers\n", FN_NAME));
  OUTPW(0x3d4, 0x8819);

  /* Getting initial values of the mclock for later restore */
  GDBG_INFO((80, "%s: Getting macronix mclock registers\n", FN_NAME));
  OUTP(0x3C4, 0x1A);
  INP(0x3C5, rd->hwDep.VG96RegDesc.mclockHi);

  OUTP(0x3C4, 0x1B);
  INP(0x3C5, rd->hwDep.VG96RegDesc.mclockLo);
  GDBG_INFO((80, "%s: mclock registers: (0x%X : 0x%X)\n", FN_NAME, 
             (FxU32)rd->hwDep.VG96RegDesc.mclockHi, (FxU32)rd->hwDep.VG96RegDesc.mclockHi));
  
  GDBG_INFO((80, "%s:  Enabling FBI Jr.\n", FN_NAME));
  OUTPW(0x3C4, 0x001F);

  GDBG_INFO((80, "%s:  Setting clock to 50MHz\n", FN_NAME));
  OUTPW(0x3c4, 0x011a);
  OUTPW(0x3c4, 0x061b);

  /* Macronix reset propogation */
  GDBG_INFO((80, "%s:  Macronix reset\n", FN_NAME));
  OUTPW(0x3c4, 0x081c);
  OUTPW(0x3c4, 0x001c);

  GDBG_INFO((80, "%s:  Hard Reset of FBI Jr.\n", FN_NAME));
  OUTPW(0x3d4, 0x0029);
  OUTPW(0x3d4, 0x0129);

  GDBG_INFO((80, "%s:  Enabling PUMA\n", FN_NAME));
  CRTC_GET(0x28, tmp);
  tmp |= 1;
  if ((envVal = myGetenv("MCRX_28")) &&
      (sscanf(envVal, "%x", &u) == 1))
    tmp = u;
  CRTC_SET(0x28, tmp);

  GDBG_INFO((80, "%s:  Restoring Protection\n", FN_NAME));
  OUTP(0x3d4, protState);

  GDBG_INFO((80, "%s:  Restoring Sequencer Index\n", FN_NAME));
  OUTP(0x3c4, crtcIndex);

  return FXTRUE;

#undef FN_NAME
} /* initMCRXEnableRegs */


/* FIXME!!! jdt - puma/ram timing fixes need to go here */
INITMCRXENTRY(initMCRXDisableRegs,FxBool,(InitRegisterDesc *rd)) 
{
#define FN_NAME "initMCRXDisableRegs"
  FxU8
    seqSave, crtcSave;
  
  FxU16
    protState;

  FxU32
    tmp;

  GDBG_INFO((80, "%s:\n", FN_NAME));

  GDBG_INFO((80, "%s:  Saving Sequencer Index\n", FN_NAME));
  INP(0x3c4, seqSave);

  GDBG_INFO((80, "%s:  Saving CRTC Index\n", FN_NAME));
  INP(0x3d4, crtcSave);

  GDBG_INFO((80, "%s:  Saving Protection State\n", FN_NAME));
  OUTP(0x3d4, 0x19);
  INP(0x3d5, protState);

  protState <<= 9;
  protState |= 0x19;

  GDBG_INFO((80, "%s:  Enabling access to extended registers\n", FN_NAME));
  OUTPW(0x3d4, 0x8819);

  /* Reset 3Dfx chip via macronix */
  GDBG_INFO((80, "%s:  Reset 3Dfx via macronix\n", FN_NAME));
  OUTPW(0x3d4, 0x0029);
  OUTPW(0x3d4, 0x0129);

  GDBG_INFO((80, "%s:  Disabling PUMA\n", FN_NAME));
  CRTC_GET(0x28, tmp);
  GDBG_INFO((80, "%s:  crtc 0x28: 0x%X\n", FN_NAME, tmp));
  tmp &= ~1; 
  CRTC_SET(0x28, tmp);

  GDBG_INFO((80, "%s:  Turning off FBI Jr.\n", FN_NAME));
  OUTPW(0x3c4, 0x061f);

  /* Restore the mclock */
  {
    FxU16 tempMclock;
    
    GDBG_INFO((80, "%s:  Restoring mclock\n", FN_NAME));
    tempMclock = (0x1A | (rd->hwDep.VG96RegDesc.mclockHi << 8));
    OUTPW(0x3C4, tempMclock);
    
    tempMclock = (0x1B | (rd->hwDep.VG96RegDesc.mclockLo << 8));
    OUTPW(0x3C4, tempMclock);

    /* Macronix MCLK reset propogation */
    OUTPW(0x3C4, 0x081C);
    OUTPW(0x3C4, 0x001C);
  }
  
  GDBG_INFO((80, "%s:  Restoring Protection\n", FN_NAME));
  OUTPW(0x3df, protState);

  GDBG_INFO((80, "%s: Restoring CRTC Index\n", FN_NAME));
  OUTP(0x3d4, crtcSave);

  GDBG_INFO((80, "%s: Restoring Sequencer Index\n", FN_NAME));
  OUTP(0x3c4, seqSave);

  return FXTRUE;
  
#undef FN_NAME
} /* initMCRXDisableRegs */

/*-------------------------------------------------------------------
  Function: initMCRXWrapFIFO
  Date: 20-Feb-97
  Implementor(s): tarolli
  Library: Init Code for MCRX
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
INITMCRXENTRY(initMCRXWrapFIFO, FxBool, (InitRegisterDesc *rd, InitFIFOData *fd) )
{
#define FN_NAME "initMCRXWrapFIFO"
  return FXFALSE;
#undef FN_NAME
} /* initMCRXWrapFIFO */

#define MAX_BUF 3

static unsigned char magic[MAX_BUF][3] = {
  { 0x0d, 0x0c, 0x21 },
  { 0x71, 0x72, 0x73 },
  /* chd - Fixme! 
   * Bogus numbers.  Does triple buffering work on Macronix?
   * 
   * dpc - 23 jan 1998 - 
   * Donny just told me this so hopefully this will work.
   */
  { 0x74, 0x75, 0x76 }
};

INITMCRXENTRY(initMCRXSwapTiles, FxBool, (InitRegisterDesc* rd))
{
#define FN_NAME "initMCRXSwapTiles"
  FxU8 crtcSwapStatus;
  FxU8 crtcSwapControl;

  /* Get the # of buffers swapping */
  OUTP(0x3D4, 0x70);
  INP(0x3D5, crtcSwapControl);

  /* Get the current active buffer setting */
  OUTP(0x3D4, 0x7A);
  INP(0x3D5, crtcSwapStatus);

  /* Bits 0-1 indicate the current active buffer.  Check to see if we
   * need to wrap this and go to the next buffer.
   */
  {
    const FxU8 bufferBits = (crtcSwapStatus & 0x03);
    const FxU8 maxBufferBits = (((crtcSwapControl & 0x08) == 0) ? 1 : 2);

    crtcSwapStatus = ((crtcSwapStatus & ~0x03) | 
                      ((bufferBits == maxBufferBits) ? 0x00 : bufferBits + 1));
    CRTC_SET(0x7A, crtcSwapStatus);
  }
  
  /* We have no way of knowing that this worked so of course we always
   * return success.
   */
  return FXTRUE;
#undef FN_NAME
}

/*-------------------------------------------------------------------
  Function: initMCRXUseTiles
  Date: 07-Feb-97 (Moved into initMCRX from init96)
  Implementor(s): pgj, dow
  Library: Init Code for MCRX
  Description:
  
  Arguments:
  atmm:                 Pointer to base of MCRX
  buffers:              Pointer to array of offsets to buffers
  nBuffers:             Total number of buffers
  backBuffer:           FB offset of back buffer
  strideInBytes:        Tile width in bytes

  Return:
  1 if successful, 2 if renderbuffers need to be swapped.
  -------------------------------------------------------------------*/
#ifdef H3D
INITMCRXENTRY(initMCRXUseTiles, int, (InitRegisterDesc *rd,
                                      FxU32 *buffers,
                                      FxU32 nBuffers,
                                      FxU32 strideInBytes,
                                      FxU32 Width, FxU32 Height, FxU32 PageFlipping)) 
#else
INITMCRXENTRY(initMCRXUseTiles, int, (InitRegisterDesc *rd,
                                      FxU32 *buffers,
                                      FxU32 nBuffers,
                                      FxU32 strideInBytes,
                                      FxU32 Width, FxU32 Height)) 
#endif
{
#define FN_NAME "initMCRXUseTiles"

  int ret = 1;
  unsigned int i;

  FxU32
    dwordOffset;

  FxU8
    regVal;

  FxU32
    tmp;

  const char *envVal;
  
  GDBG_INFO((80, "%s:\n", FN_NAME));

  regVal = _inp(VGA_MISC_OUTPUT_READ);
  /* Mask off bit 7 */
  regVal |= (1<<7);
  _outp(VGA_MISC_OUTPUT_WRITE, regVal);

  /*---------------------------------
    CONFIGURE TILES
    ---------------------------------*/
  for (i=0; i < nBuffers; i++) {
    FxU32
      buf = buffers[i];

    GDBG_INFO((80, "%s:  Setting up buffer %d\n", FN_NAME, i));

    dwordOffset = buf >> 2;

    GDBG_INFO((80, "%s:  Buffer[%d] = 0x%x, dwordOffset = 0x%x\n",
             FN_NAME, i, buf, dwordOffset));

    GDBG_INFO((80, "%s:  Writing bits 0-7 of buffer 0 offset\n", FN_NAME));
    tmp = (FxU8) (dwordOffset & 0xff);

    CRTC_SET(magic[i][0], tmp);

    GDBG_INFO((80, "%s:  Writing bits 8-15 of buffer 0 offset\n", FN_NAME));
    tmp = (FxU8) ((dwordOffset >> 8) & 0xff);
    CRTC_SET(magic[i][1], tmp);

    GDBG_INFO((80, "%s:  Writing bits 16-19 of buffer 0 offset\n", FN_NAME));
    tmp = (FxU8) ((dwordOffset >> 16) & 0xf);
    CRTC_SET(magic[i][2], tmp);
  }

  GDBG_INFO((80, "%s:  Setting up Swap Control\n", FN_NAME));
  tmp = 0x01;

  /* dpc - 26 han 1998
   * If we are triple buffering we need to enable this on the actual
   * macronix part as well. This is set by enabling bit 3 of the crtc
   * register.  
   */
  if (nBuffers == 3) tmp |= 0x08;
  mcrxHALData.initSwapTiles = initMCRXSwapTiles;

  if ((envVal = myGetenv("MRCX_71")) &&
      (sscanf(envVal, "%x", &i) == 1))
    tmp = i;
  CRTC_SET(0x70, tmp);

  /* Reset the current display buffer bits (0-1) */
  {
    FxU8 crtcSwapBuffer;

    OUTP(0x3D4, 0x7A);
    INP(0x3D5, crtcSwapBuffer);

    crtcSwapBuffer &= ~0x03;
    CRTC_SET(0x7A, crtcSwapBuffer);
  }

  return ret;

#undef FN_NAME
} /* initMCRXUseTiles */
 

/*-------------------------------------------------------------------
  Function: initMCRXGetInfo
  Date: 28-Apr-97
  Implementor(s): pgj, dow
  Library: Init Code for MCRX
  Description:

  get information about MCRX
  
  Arguments:
  info - pointer to info struct
  
  Return:
  True if success, False if Failure
  -------------------------------------------------------------------*/
INITMCRXENTRY(initMCRXGetInfo,FxBool,(VG96Info *info))  
{
#define FN_NAME "initMCRXGetInfo"
  FxU32 mBytesPow;
  FxU8 res;

  GDBG_INFO((80, "%s:\n", FN_NAME));

  CRTC_GET(0x1a, res);

  /* We care about bit 5 & 6 */
  mBytesPow = ((res >> 5) & 3);
  
  info->vgaChip = 0;
  info->vg96Rev = 0;
  info->nTFX    = 1;
  info->tfxRev  = 1;
  info->tfxRam  = 2;
  
  info->vg96Ram = (0x100000 << mBytesPow);

  GDBG_INFO((80, "%s: frame buffer has 0x%x bytes\n", FN_NAME, info->vg96Ram));
  
  return FXTRUE;
  
#undef FN_NAME
} /* initMCRXGetInfo */


