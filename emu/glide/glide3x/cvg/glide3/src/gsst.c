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
** Revision 1.1.1.1.8.8  2005/08/13 21:06:57  jwrdegoede
** Last needed 64 bit fixes for h5/h3, complete 64 bit support for cvg
**
** Revision 1.1.1.1.8.7  2005/06/09 18:32:08  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.1.1.8.6  2005/01/13 16:02:32  koolsmoky
** Restict calls to pciOpen() pciClose() when compiled with DIRECTX option. this fixes problems with the win32 miniport opened in exclusive mode.
**
** Revision 1.1.1.1.8.5  2004/12/13 08:40:18  dborca
** fixed splash DLL code (but partially left disabled)
**
** Revision 1.1.1.1.8.4  2004/11/25 19:07:31  koolsmoky
** Prepare for newer splash screens
**
** Revision 1.1.1.1.8.3  2004/10/08 06:28:35  dborca
** small matters concerning "ActiveWindow" on non-Windows
**
** Revision 1.1.1.1.8.2  2004/02/16 07:42:15  dborca
** grSetNumPendingBuffers visible with grGetProcAddress
**
** Revision 1.1.1.1.8.1  2003/11/03 13:34:29  dborca
** Voodoo2 happiness (DJGPP & Linux)
**
** Revision 1.1.1.1  1999/12/07 21:42:33  joseph
** Initial checkin into SourceForge.
**
** 
** 1     10/08/98 11:30a Brent
** 
** 40    10/07/98 9:43p Peter
** triangle procs for 3DNow!(tm)
** 
** 39    10/06/98 8:23p Peter
** 3DNow!(tm) texture downloads
** 
** 38    9/28/98 4:37p Peter
** removed cruft
** 
** 37    9/18/98 5:16p Atai
** check sli swap interval in grBufferSwap
** 
** 36    9/10/98 3:39p Atai
** callback the error routine if window handle is not valid in
** grSstWinOpen
** 
** 35    9/08/98 6:56p Atai
** fix debug info. added underline for internal routines and some \n
** 
** 34    8/10/98 12:25p Peter
** fixed debug build
** 
** 33    8/03/98 10:48a Atai
** rename 3dfxsplash3.dll to 3dfxspl3.dll
** 
** 32    7/24/98 1:41p Hohn
** 
** 31    7/16/98 5:09p Atai
** fix grTexNCCTable textureMode and clip space st coords
** 
** 30    7/01/98 11:31a Atai
** make grDepthBiasLevel argument FxI32
** 
** 29    6/25/98 10:40a Peter
** more cb checks
** 
** 28    6/24/98 1:47p Atai
** code clean up; rename texute line routine
** 
** 27    6/15/98 2:39p Atai
** undo previous check-in
** 
** 25    6/11/98 12:53p Atai
** remove grGammaCorrectionValue
** 
** 24    6/11/98 12:48p Atai
** remove grGammaCorrectionValue
** 
** 23    6/09/98 5:33p Atai
** replace grSstControl with grEnable/grDisable
** 
** 22    6/04/98 12:12p Peter
** splash dll rename
** 
** 21    5/19/98 1:27p Peter
** buffer init
** 
** 20    4/22/98 4:57p Peter
** glide2x merge
** 
** 19    3/30/98 3:28p Atai
** set gamma to 1.3
** 
** 18    3/13/98 4:03p Peter
** start glide3 merge
** 
** 17    3/02/98 7:26p Peter
** clear slop on sli systems when changing y origin
** 
** 16    2/23/98 11:44a Peter
** merged monitor detection and fixed compilation error from recent
** videobuffer changes
** 
** 15    2/12/98 3:40p Peter
** single buffering for opengl
** 
** 14    2/10/98 7:04p Atai
** fix grvertexlayout for migration dll
** 
** 13    2/05/98 3:07p Atai
** fix migration dll
** 
** 12    1/30/98 4:58p Atai
** fix gamma table parameters
** 
** 11    1/30/98 4:29p Peter
** no uswc for sli slave
** 
** 10    1/26/98 12:20p Atai
** fix grVertexLayout in grSplash()
** 
** 9     1/26/98 11:30a Atai
** update to new glide.h
** 
** 8     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
** 
** 7     1/20/98 11:03a Peter
** env var to force triple buffering
** 
** 6     1/18/98 12:03p Atai
** sync to rev 17 spec
 * 
 * 5     1/17/98 2:26p Atai
 * fix grvertexlayout
 * 
 * 4     1/17/98 1:12p Atai
 * set default as back buffer
 * 
 * 3     1/16/98 6:54p Atai
 * disable gamma table for now
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 116   1/16/98 4:18p Atai
 * fixed lfb and grLoadGammaTable
 * 
 * 115   1/16/98 10:47a Peter
 * 
 * 114   1/16/98 10:16a Atai
 * fixed grSstIldle
 * 
 * 113   1/10/98 4:01p Atai
 * inititialize vertex layout, viewport, added defines
 * 
 * 110   1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 109   1/06/98 3:53p Atai
 * remove grHint, modify grLfbWriteRegion and grGet
 * 
 * 107   12/18/97 2:12p Peter
 * grSstControl on v2
 * 
 * 106   12/17/97 4:48p Peter
 * groundwork for CrybabyGlide
 * 
 * 105   12/17/97 4:06p Atai
 * added grChromaRange(), grGammaCorrecionRGB(), grRest(), and grGet()
 * functions
 * 
 * 104   12/16/97 1:33p Atai
 * added grGammaCorrectionRGB()
 * 
 * 103   12/16/97 10:03a Atai
 * fixed gutexmemreset for glide2
 * 
 * 101   12/09/97 12:20p Peter
 * mac glide port
 * 
 * 100   12/05/97 4:26p Peter
 * watcom warnings
 * 
 * 99    12/03/97 11:35a Peter
 * is busy thing
 * 
 * 98    11/25/97 12:09p Peter
 * nested calls to grLfbLock vs init code locking on v2
 * 
 * 97    11/21/97 1:02p Peter
 * v^2 supported resolutions
 * 
 * 96    11/21/97 11:19a Dow
 * Added RESOLUTION_NONE hack for Banshee
 * 
 * 95    11/19/97 2:49p Peter
 * env vars in registry for win32
 * 
 * 94    11/19/97 2:22p Dow
 * gsst.c
 * 
 * 93    11/18/97 4:50p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 92    11/18/97 4:00p Atai
 * fixed the GR_BEGIN and GR_END error in previous check-in
 * 
 * 91    11/18/97 3:27p Atai
 * update vData 
 * optimize state monster
 * 
 * 90    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 89    11/16/97 2:20p Peter
 * cleanup
 * 
 * 88    11/15/97 7:43p Peter
 * more comdex silliness
 * 
 * 87    11/14/97 11:10p Peter
 * open vs hw init confusion
 * 
 * 86    11/14/97 5:02p Peter
 * more comdex stuff
 * 
 * 85    11/14/97 4:47p Dow
 * New splash screen
 * 
 * 84    11/14/97 12:09a Peter
 * comdex thing and some other stuff
 * 
 * 83    11/12/97 9:37p Dow
 * Banshee crap
 * 
 * 82    11/12/97 2:27p Peter
 * 
 * 81    11/12/97 1:09p Dow
 * H3 Stuf
 * 
 * 80    11/12/97 9:22a Dow
 * H3 Mods
 * 
 * 79    11/06/97 3:46p Peter
 * sli shutdown problem
 * 
 * 78    11/06/97 3:38p Dow
 * More banshee stuff
 * 
 * 77    11/04/97 5:04p Peter
 * cataclysm part deux
 * 
 * 76    11/04/97 3:58p Dow
 * Banshee stuff
 * 
 * 75    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 74    10/29/97 4:59p Peter
 * fixed csim/non-debug stupidity
 * 
 * 73    10/29/97 2:45p Peter
 * C version of Taco's packing code
 * 
 * 72    10/23/97 5:28p Peter
 * sli fifo thing
 * 
 * 71    10/17/97 3:15p Peter
 * grSstVidMode thingee
 * 
 * 70    10/14/97 2:44p Peter
 * moved close flag set
 * 
 * 69    10/09/97 8:02p Dow
 * State Monster 1st Cut
 * 
**
*/

#include <stdio.h>
#include <string.h>
#include <3dfx.h>

#include <glidesys.h>

#if (GLIDE_PLATFORM & GLIDE_HW_SST96) 
#include <init.h>
#endif

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxsplash.h"

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


/*-------------------------------------------------------------------
  Function: grSstWinOpen
  Date: 3/16
  Implementor(s): dow, gmt, murali, jdt
  Mutator: dpc
  Library: Glide
  Description:

  Initialize the selected SST

  Initialization has 4 Steps 

  Video Init - 
     In the full screen case this includes setting the requested
     resolution/refresh state and allocating any necessary OS 
     resource for the GC.  

     In the windowed case, this involves acquiring all necessary
     surfaces for rendering, back buffer and memory fifo

     Also perform any work necessary to enable access to 3D 
     registers

  Command Transport Init:
     Assuming video registers are in a reset state, initialize
     the appropriate command transport mechanism.  All writes
     to hardware prior to this action are direct writes, 
     afterwards, most commands will go throught the command
     transport

  GC Init - 
     Initialize the current GC based on the user requested
     open parameters and command transport data reported
     during initialization.

  3D State Init - 
     Push an initial state onto all of the 3D state registers.   
     
  Arguments:
  hwnd - pointer to a window handle or null.  If NULL, then 
         the application window handle will be inferred though
         the GetActiveWindow() api.
  resolution - either one of the pre-defined glide resolutions,
               or GR_RESOLUTION_NONE, in which case the window
               size is inferred from the size application window
  refresh - requested fullscreen refresh rate, ignored in a window
  format  - requested ccolor format for glide packed color values
  origin  - location of coordinate origin either upper left or
                    lower right
  nColBuffers - number of color buffers to attempt to allocate
                0 - meaningless
                1 - allocate a front buffer only
                2 - allocate a front and back buffer
                3 - allocate a front, back, aux buffer for tripple buffering
  nAuxBuffers - number of aux buffers to attempt to allocate
                0 - no alpha or z buffers
                1 - allocate one aux buffer for alpha/depth buffering
                2 - allocate on depth and one alpha buffer (unsup)
  Return:
  FXTRUE - glide successfully acquired the necessary resources and a
           is ready for rendering
  FXFALSE - glide was unsuccessful in getting the necessary resources, 
            or the requested configuration is unavailble on the host
            hardware - any calls to glide rendering routines will result
            in undefined behavior.
  -------------------------------------------------------------------*/
#ifndef GLIDE3_ALPHA
GR_ENTRY(grSstWinOpen, FxBool, (
  FxU32                   hWnd,
  GrScreenResolution_t    resolution, 
  GrScreenRefresh_t       refresh, 
  GrColorFormat_t         format, 
  GrOriginLocation_t      origin, 
  int                     nColBuffers,
  int                     nAuxBuffers))
#else
GR_ENTRY(grSstWinOpen, GrContext_t, (
  FxU32                   hWnd,
  GrScreenResolution_t    resolution, 
  GrScreenRefresh_t       refresh, 
  GrColorFormat_t         format, 
  GrOriginLocation_t      origin, 
  int                     nColBuffers,
  int                     nAuxBuffers))
#endif
{
#define FN_NAME "grSstWinOpen"
  FxBool rv = FXTRUE;
  int tmu;
  int xres, yres, fbStride;
  FxDeviceInfo devInfo;
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  sst1VideoTimingStruct *sstVideoRez = NULL, tvVidtiming;
  FxBool tv_swap_board = FXFALSE;
#endif
  
  GR_BEGIN_NOFIFOCHECK("grSstWinOpen",80);
  GDBG_INFO_MORE(gc->myLevel,
                 "(rez=%d,ref=%d,cformat=%d,origin=%s,#bufs=%d, #abufs=%d)\n",
                 resolution,refresh,format,
                 origin ? "LL" : "UL",
                 nColBuffers, nAuxBuffers);
  GR_CHECK_F(FN_NAME, !gc, "no SST selected as current (gc==NULL)");
  
#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
  /* CVG cannot just do one color buffer (Well, I guess it could, but
   * not if you have the aux buffer too) so just force the allocation
   * of two buffers. This does have consequences for swap since we
   * need to ignore them too.
   */
  gc->hwDep.cvgDep.singleBufferP = (nColBuffers == 1);
  if (gc->hwDep.cvgDep.singleBufferP) nColBuffers = 2;
  
  /* Voodoo^2 has a bug in sli mode where the video cannot really
   * support resolutions less than 512x384 on pc monitors. Rather than
   * saying sli is not fully compatible w/ single board systems the
   * 'solution' is to just not allow these resolutions.
   * 
   * NB: The current assumption here is taht if the user passes
   * their own video timing structure then they know what the hell
   * they are doing for some weird-ass arcade monitor.
   */
  rv = !((gc->vidTimings == NULL) &&
         ((resolution < GR_RESOLUTION_512x384) ||
          (resolution == GR_RESOLUTION_512x256) ||
          (resolution == GR_RESOLUTION_400x300) ||
          (resolution == GR_RESOLUTION_NONE)));
  if (!rv) {
    GDBG_INFO(gc->myLevel, "  Incompatible Voodoo^2 resolution.\n");
    goto BAILOUT;
  }

#if !defined(__linux__) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
  if (!hWnd)
    GrErrorCallback("grSstWinOpen: need to use a valid window handle",
                    FXTRUE);
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  GR_CHECK_COMPATABILITY(FN_NAME, 
                         ((hWnd == 0x00UL) && (GetActiveWindow() == NULL)),
                         "A valid active window or hWnd parameter is required.");
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */
#endif /* (GLIDE_PLATFORM & GLODE_HW_CVG) */
  
#if !GLIDE_INIT_HAL
__tryReOpen:
  /* If we've been closed via grSstWinClose then we need to re-init
   * the hw registers etc before init-ing video etc or bad ju-ju is
   * in our future.
   *
   * NB: These are in the same order that they are done in the same
   * order as in _grDetectResources. They should match, so don't mess
   * it up.
   */
  if (!gc->hwInitP) {
    FxU32* sstRegs = NULL;

#if !DIRECTX
    rv = pciOpen();
    if (!rv) {
      GDBG_INFO(gc->myLevel, "%s: pci bus could not be opened\n", FN_NAME);
      goto BAILOUT;
    }
#endif
    
    sstRegs = sst1InitMapBoard(_GlideRoot.current_sst);  
    rv = (sstRegs != NULL);
    if (!rv) {
      GDBG_INFO(gc->myLevel, "%s: re-map of board failed (0x%X : 0x%x)\n",
                FN_NAME, sstRegs, gc->reg_ptr);
      goto BAILOUT;
    }
    
    /* Re-enabled write combining */
    sst1InitCaching(sstRegs, FXTRUE);

    rv = sst1InitRegisters(sstRegs);
    if (!rv) {
      GDBG_INFO(gc->myLevel, "%s: Could not re-init hw registers\n", FN_NAME);
      goto BAILOUT;
    }

    /* Reset up pointers to the various address spaces within the hw
     * since the hw base address can be different from the original
     * map board.  
     */
    gc->base_ptr  = (FxU32*)HW_BASE_PTR(sstRegs);
    gc->reg_ptr   = (FxU32*)HW_REG_PTR(sstRegs);
    gc->lfb_ptr   = (FxU32*)HW_LFB_PTR(sstRegs);
    gc->tex_ptr   = (FxU32*)HW_TEX_PTR(sstRegs);
    
    /* Make sure that the sli-slave gets re-initted too.
     *
     * NB: This makes the same assumption as the sli-pairing code
     * that the sli slave is the next one discovered in the
     * set of GC's.
     */
    if (gc->scanline_interleaved) {
      sstRegs = sst1InitMapBoard(_GlideRoot.current_sst + 1);
      rv = (sstRegs != NULL);
      if (!rv) {
        GDBG_INFO(gc->myLevel, "%s: Could not re-map slave.\n", FN_NAME);
        goto BAILOUT;
      }
      
      rv = sst1InitRegisters(sstRegs);
      if (!rv) {
        GDBG_INFO(gc->myLevel, "%s: Could not re-init slave registers\n", FN_NAME);
        goto BAILOUT;
      }
      
      /* Reset up pointers to the various address spaces within the hw
       * since the hw base address can be different from the original
       * map board.  
       */
      (gc + 1)->base_ptr  = (FxU32*)HW_BASE_PTR(sstRegs);
      (gc + 1)->reg_ptr   = (FxU32*)HW_REG_PTR(sstRegs);
      (gc + 1)->lfb_ptr   = (FxU32*)HW_LFB_PTR(sstRegs);
      (gc + 1)->tex_ptr   = (FxU32*)HW_TEX_PTR(sstRegs);

      gc->slave_ptr = sstRegs;
    }
    
    gc->hwInitP = FXTRUE;
  } else if (gc->open) {
    /* If this device is already open then it is probably an error to
     * call grSstWinOpen again. Rather than letting all hell break
     * loose as we re-init the video and (especially) command fifo
     * shutdown the hw and do a full re-init.
     */
    GR_CHECK_COMPATABILITY(FN_NAME, gc->open, "This context is already open");
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
    grSstWinClose(0);
#else
    grSstWinClose();
#endif
    goto __tryReOpen;
  }
#endif /* !GLIDE_INIT_HAL */
  
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  /*
  ** load fxoem2x.dll and map board
  */
  { 
    gc->oemInit = NULL;
    rv = sst1InitGetDeviceInfo(gc->reg_ptr, &devInfo);
    if (rv) {
      char* boardIDStr;
      FxU32 boardID;
      char* requireOEMDLLStr;
      FxU32 requireOEMDLL;

      gc->oemi.version = OEMINIT_VERSION;
      gc->oemi.vendorID = devInfo.vendorID;
      gc->oemi.subvendorID = OEMINIT_INVALID_BOARD_ID;
      gc->oemi.deviceID = devInfo.deviceID;
      gc->oemi.boardID  = devInfo.fbiBoardID | (devInfo.fbiConfig << FX_OEM_FBI_CONFIG_SHIFT);
      gc->oemi.linearAddress = gc->base_ptr;
      gc->oemi.slaveAddress = gc->slave_ptr;
      gc->oemi.fxoemPciWriteConfig = (PciConfigProc)pciSetConfigData;
      gc->oemi.fxoemPciReadConfig = (PciConfigProc)pciGetConfigData;
      

      boardIDStr = GETENV("FX_GLIDE_BOARDID");
      boardID = (boardIDStr == NULL) ? 0 : atol(boardIDStr);
      if ((boardID) && (devInfo.fbiBoardID != boardID)) {
        /* if boardID is defined and does not match */
        GrErrorCallback("Undefined boardID", FXTRUE);
        grSstWinClose(0);
        exit(0);
      }

      requireOEMDLLStr = GETENV("FX_GLIDE_REQUIREOEMDLL");
      requireOEMDLL = (requireOEMDLLStr == NULL) ? 0 : atol(requireOEMDLLStr);
      if (requireOEMDLL == GR_SKIP_OEMDLL) {
        /* skip oem dll */
        gc->oemInit = NULL;
      }
      else if (requireOEMDLL == GR_NO_OEMDLL) {
        /* no oemdll is allowed */
        gc->oemInit = LoadLibrary("fxoem2x.dll");
        if (gc->oemInit) {
          GrErrorCallback("Please remove fxoem2x.dll", FXTRUE);
          grSstWinClose(0);
          exit(0);
        }
      }
      else if ( requireOEMDLL == 0) {
        /* FX_GLIDE_REQUIREOEMDLL is not defined, load oemdll if it exist */
        gc->oemInit = LoadLibrary("fxoem2x.dll");
      }
      else {
        /* oem dll is required */
        gc->oemInit = LoadLibrary("fxoem2x.dll");
        if (gc->oemInit == NULL) {
          GrErrorCallback("Missing fxoem2x.dll", FXTRUE);
          grSstWinClose(0);
          exit(0);
        }
      }
      
      /*
      ** oem map board 
      */
      if (gc->oemInit) {
        FARPROC oemInitMapBoard = GetProcAddress(gc->oemInit, "_fxoemInitMapBoard@4");
        if (oemInitMapBoard != NULL)
          oemInitMapBoard(&gc->oemi);
        else
          gc->oemInit = NULL;
      }
    }
    else {
      GDBG_INFO(gc->myLevel, "  XXXGetDeviceInfo failed. (0x%X)\n", gc->reg_ptr);
      goto BAILOUT;
    }
  }
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */

  /*------------------------------------------------------
    Video Init
    ------------------------------------------------------*/  
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  /*
  ** initialize video timing data for oemdll
  ** if tv out component exist, the oem dll need to modify the vidtiming
  ** data in oemi. Glide will take the changes and pass it to 
  ** sst1InitVideoBuffers()
  */
  {
    if((gc->oemInit) && (sstVideoRez = sst1InitFindVideoTimingStruct(resolution, refresh))) {
      tvVidtiming               = *sstVideoRez;
      gc->oemi.vid.res          = resolution;
      gc->oemi.vid.refresh      = refresh;
      gc->oemi.vid.depth        = 16;
      gc->oemi.vid.hSyncOn      = tvVidtiming.hSyncOn;
      gc->oemi.vid.hSyncOff     = tvVidtiming.hSyncOff;
      gc->oemi.vid.vSyncOn      = tvVidtiming.vSyncOn;
      gc->oemi.vid.vSyncOff     = tvVidtiming.vSyncOff;
      gc->oemi.vid.hBackPorch   = tvVidtiming.hBackPorch;
      gc->oemi.vid.vBackPorch   = tvVidtiming.vBackPorch;
      gc->oemi.vid.xDimension   = tvVidtiming.xDimension;
      gc->oemi.vid.yDimension   = tvVidtiming.yDimension;
      gc->oemi.vid.clkFreq16bpp = tvVidtiming.clkFreq16bpp;
      gc->oemi.vid.clkFreq24bpp = tvVidtiming.clkFreq24bpp;
      
      if (gc->oemInit) {
        FARPROC oemInitVideoTiming = GetProcAddress(gc->oemInit, "_fxoemInitVideoTiming@4");
        if (oemInitVideoTiming) {
          if (oemInitVideoTiming(&gc->oemi.vid)) {
            /*
            ** video timing is updated by oem dll
            */
            tvVidtiming.hSyncOn      = gc->oemi.vid.hSyncOn;
            tvVidtiming.hSyncOff     = gc->oemi.vid.hSyncOff;
            tvVidtiming.vSyncOn      = gc->oemi.vid.vSyncOn;
            tvVidtiming.vSyncOff     = gc->oemi.vid.vSyncOff;
            tvVidtiming.hBackPorch   = gc->oemi.vid.hBackPorch;
            tvVidtiming.vBackPorch   = gc->oemi.vid.vBackPorch;
            tvVidtiming.xDimension   = gc->oemi.vid.xDimension;
            tvVidtiming.yDimension   = gc->oemi.vid.yDimension;
            tvVidtiming.clkFreq16bpp = gc->oemi.vid.clkFreq16bpp;
            tvVidtiming.clkFreq24bpp = gc->oemi.vid.clkFreq24bpp;
            refresh = tvVidtiming.refreshRate  = gc->oemi.vid.refresh; 
            grSstVidMode(_GlideRoot.current_sst, &tvVidtiming);
          }
        }
      }
    }
  }
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */

#if GLIDE_INIT_HAL
  {
    GDBG_INFO(gc->myLevel, "  HAL Video Init\n");
    
    rv = fxHalInitVideo((SstRegs*)gc->reg_ptr,
                        ((resolution == GR_RESOLUTION_NONE) 
                         ? GR_RESOLUTION_640x480
                         : resolution), 
                        refresh, 
                        NULL);
    if (!rv) goto BAILOUT;
  }

#else
  GDBG_INFO(gc->myLevel, "  Video Init\n");

  /* If the user has buffer allocation set in their environment. Try
   * those settings before trying the actual program settings.
   *
   * NB: We cannot fail the allocation on the env settings since the
   * user could have messed up and set something ridiculous.  
   */
  {
    FxI32 curColorBuffers = ((_GlideRoot.environment.nColorBuffer == -1L)
                             ? nColBuffers
                             : _GlideRoot.environment.nColorBuffer);
    FxI32 curAuxBuffers   = ((_GlideRoot.environment.nAuxBuffer == -1L)
                             ? nAuxBuffers
                             : _GlideRoot.environment.nAuxBuffer);
    FxBool triedParamsP;

    do {
      triedParamsP = ((curColorBuffers == nColBuffers) &&
                      (curAuxBuffers == nAuxBuffers));

      rv = sst1InitVideoBuffers(gc->reg_ptr, 
                                resolution, refresh, 
                                curColorBuffers,
                                curAuxBuffers,
                                gc->vidTimings,
                                FXTRUE);
      if (!rv && !triedParamsP) {
        curColorBuffers = nColBuffers;
        curAuxBuffers   = nAuxBuffers;
      }
    } while(!rv && !triedParamsP);

    /* Reset the parameters for the possible sli-slave video init and
     * internal bookkeeping.  
     */
    nColBuffers = curColorBuffers;
    nAuxBuffers  = curAuxBuffers;
  }

  /* If sli is on then we need to init the slave board's video */
  if (rv && gc->scanline_interleaved) {
    rv = (gc->slave_ptr != NULL);
    if (!rv) {
      GDBG_INFO(gc->myLevel, "  Invalid slave base address.\n");
      goto __errSliExit;
    }
    
    rv = sst1InitVideoBuffers(gc->slave_ptr,
                              resolution, refresh,
                              nColBuffers, nAuxBuffers, 
                              gc->vidTimings,
                              FXTRUE);
    if (!rv) {
      GDBG_INFO(gc->myLevel, "  sst1InitVideoBuffers failed for sli slave.\n");
      goto __errSliExit;
    }
  }

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  /*
  ** initialize oem vedio after master and slave board
  */
  if (gc->oemInit) {
    FARPROC oemInitSetVideo = NULL;
    FARPROC oemGet;
    FxI32 tv_connected = 0;
    FxI32 slimaster[2], slislave[2];
    oemInitSetVideo = GetProcAddress(gc->oemInit, "_fxoemInitSetVideo@4");
    if (oemInitSetVideo)
      oemInitSetVideo(&gc->oemi);
    
    oemGet = GetProcAddress(gc->oemInit, "_fxoemGet@12");
    if (oemGet) {
      oemGet(FX_OEM_TVOUT, 4, &tv_connected);
      /* Is tv connected to the board? */
      if (tv_connected) {
        oemGet(FX_OEM_SLIMASTER, 8, slimaster);
        oemGet(FX_OEM_SLISLAVE, 8, slislave);
        if (((FxU32 *)slimaster[1] == gc->slave_ptr) && 
            ((FxU32 *)slislave[1] == gc->base_ptr))
          tv_swap_board = FXTRUE;
        else
          tv_swap_board = FXFALSE;
      }
    }
  }
#endif

  if (rv) {
    /* Check to see that we have a monitor attached to the board taht we
     * expect to be driving the video. There are two cases here: sli and
     * non-sli.
     *
     * In the sli case we expect the master to be driving the video so if
     * the slave has the monitor plugged into it then we swap the sense of
     * the master and slave.
     *
     * In the non-sli case there are two sub-cases: the user does not
     * have a monitor attached at all to the card (read: moron) or that
     * they have explicitly disabled sli with the monitor plugged into
     * the slave. In the moron case there's nothing we can do, except
     * finish up the init and let the user deal w/ the fact that they
     * can't see anything. In the sli-disable case we need to initialize
     * the video of the would be slave (this was not done before since
     * sli was not auto-detected by the init code), swap the sense of
     * the boards and continue w/ the rest of the initialization.
     *
     * NB: The calls to sst1InitGetDeviceInfo *MUST* come after
     * sst1InitVideoBuffers for each card since that is where the
     * monitor detection is done.
     */
    rv = sst1InitGetDeviceInfo(gc->reg_ptr, &devInfo);
    if (!rv) {
      GDBG_INFO(gc->myLevel, 
                "  sst1InitGetDeviceInfo failed to determine master monitor connectivness.\n");
      goto __errSliExit;
    }
    if (!devInfo.monitorDetected) {
      if (gc->scanline_interleaved) {
        rv = sst1InitGetDeviceInfo(gc->slave_ptr, &devInfo);
        if (!rv) {
          GDBG_INFO(gc->myLevel, 
                    "  sst1InitGetDeviceInfo failed to determine slave monitor connectivness.\n");
          goto __errSliExit;
        }

        rv = FXTRUE;
        if (devInfo.monitorDetected) {
          GDBG_INFO(gc->myLevel, "  Swapping sli slave and master");          
        } else {
          /* We're in moron mode w/ no monitors attached.
           * Just do the default init w/o any swapping.
           */
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
          /* 
          ** We are almost in the moron mode. We need to check the if tv is connected.
          ** If not, then we are in the moron mode.
          */
          if (tv_swap_board) 
            GDBG_INFO(gc->myLevel, "  TV is connected. Swapping sli slave and master");          
          else 
#endif
          {
            GDBG_INFO(gc->myLevel, "  No monitor attached to slave or master.\n");
            goto __errSliExit;
          }
        }
      } else {
        if (!gc->sliPairP) {
          /* If there isn't a next board then we're in moron mode. */
          rv = FXTRUE;
          goto __errSliExit;
        } else 
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
          if (!tv_swap_board)
#endif
        {
          /* Shutdown the logical master so that the new master will
           * really own the video.
           *
           * NB: We don't have to shutdown the command fifo here since
           * it is not on yet.
           */
          {
            sst1InitIdle(gc->base_ptr);

            /* Make sure that the caching is toggled.
             *
             * NB: The order of the caching changes is relevant here since
             * we don't want to waste mtrr's, even briefly.  
             */
            sst1InitCaching(gc->base_ptr, FXFALSE);
            sst1InitCaching((gc + 1)->base_ptr, FXTRUE);

            sst1InitShutdown(gc->base_ptr);

            /* We want to let the oem that video has shutdown here so
             * that they can do the 'right' thing when we re-init
             * video w/ the current slave as the master.
             *
             * NB: However, we don't do this currently because it requires
             * odd semantics to the calling conventions of the oem dll.
             */            
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32) && 0
            if (gc->oemInit) {
              FARPROC oemRestoreVideo = GetProcAddress(gc->oemInit, "_fxoemRestoreVideo@4"); 
              if (oemRestoreVideo != NULL) oemRestoreVideo(&gc->oemi);
            }
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */
            
            gc->hwInitP = FXFALSE;
          }

#if !DIRECTX
          /* dpc - 14 feb 1997 - HackAlert!!!!
           * The sst1InitShutdown above will close the pci library which
           * then prevents other pci library calls which we need to init
           * video next.
           */
          pciOpen();
#endif
          
          /* Re-init the new master */
          {
            FxU32* sstRegs = sst1InitMapBoard(_GlideRoot.current_sst + 1);
            rv = (sstRegs != NULL);
            if (!rv) {
              GDBG_INFO(gc->myLevel, "%s: Could not re-map 'monitor swap slave'.\n", FN_NAME);
              goto __errSliExit;
            }

            rv = sst1InitRegisters(sstRegs);
            if (!rv) {
              GDBG_INFO(gc->myLevel, "%s: Could not re-init 'monitor swap slave' registers\n", FN_NAME);
              goto __errSliExit;
            }

            /* Reset up pointers to the various address spaces within the hw
             * since the hw base address can be different from the original
             * map board.  
             */
            (gc + 1)->base_ptr  = (FxU32*)HW_BASE_PTR(sstRegs);
            (gc + 1)->reg_ptr   = (FxU32*)HW_REG_PTR(sstRegs);
            (gc + 1)->lfb_ptr   = (FxU32*)HW_LFB_PTR(sstRegs);
            (gc + 1)->tex_ptr   = (FxU32*)HW_TEX_PTR(sstRegs);
            
            /* We're done setting up the new master's hw */
            (gc + 1)->hwInitP = FXTRUE;
          }

          rv = sst1InitVideoBuffers((gc + 1)->base_ptr,
                                    resolution, refresh,
                                    nColBuffers, nAuxBuffers,
                                    gc->vidTimings,
                                    FXTRUE);
          if (!rv) {
            GDBG_INFO(gc->myLevel, "  sst1InitVideoBuffers failed for 'monitor swap slave'.\n");
            goto __errSliExit;
          }
        }
      }
    
      /* Switch the senses of the master and slave in the gc's. 
       * 
       * NB: This only has to swap the hw addresses since the rest of
       * the information has to be the same in order for the boards to
       * be sli-able.  
       */
#define SwapPtrVal(__a, __b) do { FxU32* temp = (__a); (__a) = (__b); (__b) = temp; } while(0);
      SwapPtrVal(gc->base_ptr,  (gc + 1)->base_ptr);
      SwapPtrVal(gc->reg_ptr,   (gc + 1)->reg_ptr);
      SwapPtrVal(gc->tex_ptr,   (gc + 1)->tex_ptr);
      SwapPtrVal(gc->lfb_ptr,   (gc + 1)->lfb_ptr);
#undef SwapPtrVal

      if (gc->scanline_interleaved) {
        /* The slave pointer, however, needs to be set by hand again. */
        gc->slave_ptr = (gc + 1)->base_ptr;
        (gc + 1)->slave_ptr = NULL;

        /* Swap the caching characteristics too. The old master is now
         * the slave, and the old slave is now the master. The caching
         * on the old master was set when the board was mapped the
         * first time.
         *
         * NB: The order of the caching changes is relevant here since
         * we don't want to waste mtrr's, even briefly.  
         */
        sst1InitCaching(gc->slave_ptr, FXFALSE);
        sst1InitCaching(gc->base_ptr, FXTRUE);
      }
    
      /* Save that we swapped teh master sense so that we can
       * restore these settings when we shut down.
       */
      gc->swapMasterSenseP = FXTRUE;

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
      /* NB: We need to make sure that the oem structure reflects the
       * 'new' sense of the master/slave relationship.  
       */
      gc->oemi.linearAddress = gc->base_ptr;
      gc->oemi.slaveAddress = gc->slave_ptr;      

      /* re-initialize oem vedio for the board */
      if (gc->oemInit) {
        FARPROC oemInitSetVideo = GetProcAddress(gc->oemInit, "_fxoemInitSetVideo@4");
        if (oemInitSetVideo != NULL) oemInitSetVideo(&gc->oemi);
      }
#endif
    }
  }

  /* If we have an sli configuration then start it up.
   *
   * NB: There are two ways to get here. In the case that a monitor
   * was detected we will get here straightline. If no monitors are
   * detected (sli or no) we will get here via an error goto to
   * the label __errSliExit.  
   */
__errSliExit:
  if (rv && gc->scanline_interleaved) {  
    rv = sst1InitSli(gc->reg_ptr, gc->slave_ptr);
    if (!rv) {

      GDBG_INFO(gc->myLevel, "  sst1InitSli failed.\n");
      goto __errSliExit;
    }
  }

  /* Explicitly set the pass through address that the vxd will be
   * using to toggle the passthrough. In sli mode this is the master,
   * and otherwise it is the card driving the monitor's vidoe.  
   * Currently, we set this to be a 16mb address space.
   */
  if (rv) pciSetPassThroughBase(gc->base_ptr, 0x1000000);
#endif

  if (!rv) goto BAILOUT;

  /* Doh! Don't forget to set the resolution conversions. */
  if (gc->vidTimings == NULL) {
    switch(resolution) {
#if (GLIDE_PLATFORM & GLIDE_HW_H3)
    case GR_RESOLUTION_NONE:
      xres = 640; yres = 480;
      break;
#endif

#if !(GLIDE_PLATFORM & GLIDE_HW_CVG)
    case GR_RESOLUTION_320x200:
      xres = 320; yres = 200;
      break;
    
    case GR_RESOLUTION_320x240:
      xres = 320; yres = 240;
      break;
    
    case GR_RESOLUTION_400x256:
      xres = 400; yres = 256;
      break;
#endif /* !(GLIDE_PLATFORM & GLIDE_HW_CVG) */

    case GR_RESOLUTION_512x384:
      xres = 512; yres = 384;
      break;

    case GR_RESOLUTION_640x200:
      xres = 640; yres = 200;
      break;

    case GR_RESOLUTION_640x350:
      xres = 640; yres = 350;
      break;

    case GR_RESOLUTION_640x400:
      xres = 640; yres = 400;
      break;

    case GR_RESOLUTION_640x480:
      xres = 640; yres = 480;
      break;

    case GR_RESOLUTION_800x600:
      xres = 800; yres = 600;
      break;

    case GR_RESOLUTION_960x720:
      xres = 960; yres = 720;
      break;

    case GR_RESOLUTION_856x480:
      xres = 856; yres = 480;
      break;

    case GR_RESOLUTION_512x256:
      xres = 512; yres = 256;
      break;

    case GR_RESOLUTION_1024x768: 
      xres = 1024; yres = 768;
      break;

    case GR_RESOLUTION_1280x1024:
      xres = 1280; yres = 1024;
      break;

    case GR_RESOLUTION_1600x1200:
      xres = 1600; yres = 1200;
      break;

    default:
      /* GR_ASSERT should never return, just to shut up warnings */
      xres = yres = 0;
      GR_ASSERT(0);
    }
  } else {
    xres = gc->vidTimings->xDimension;
    yres = gc->vidTimings->yDimension;
  }

  /* Stash these, mostly for error checking type things */
  gc->grColBuf = nColBuffers;
  gc->grAuxBuf = nAuxBuffers;

  /* The current init code always sets this to 1024. */
  fbStride = 1024;

  GDBG_INFO(gc->myLevel, 
            "  Video init succeeded. xRes = %.04d, yRes = %.04d\n",
            xres, yres);

  /*------------------------------------------------------
    Command Transport Init
    ------------------------------------------------------*/
  {
#if !((GLIDE_PLATFORM & GLIDE_HW_CVG))
    InitFIFOData fifoInfo;
     
    GDBG_INFO(gc->myLevel, "  Command Transport Init\n");
    rv = initEnableTransport(&fifoInfo);
    if (!rv) goto BAILOUT;     
#endif
     
#if   (((GLIDE_PLATFORM & GLIDE_HW_H3) && (!USE_PACKET_FIFO)) || ((GLIDE_PLATFORM & GLIDE_HW_CVG) && !USE_PACKET_FIFO))

#if 0
    /* dpc - 12 nov 1997 - FixMe
     * Do we still need this for the interim?
     */
    grHints(GR_HINT_FIFOCHECKHINT, 
            fifoInfo.hwDep.vgFIFOData.memFifoStatusLwm + 0x100);
#endif

    _grReCacheFifo(0);
#elif ((GLIDE_PLATFORM & GLIDE_HW_H3) || (GLIDE_PLATFORM & GLIDE_HW_CVG)) && USE_PACKET_FIFO
    /* Place cmd fifo into fbi memory.  There are actually two
     * 'placements' here, hw memory and virtual address space. In hw
     * memory, the fifo is placed after any buffers that the user has
     * requested (color and aux). If there is any memory left after
     * these buffer allocations, we grab it all, otherwise this is an
     * error. In virtual address space, the fifo is always placed at
     * HW_FIFO_OFFSET.
     */
    {
      FxU32 fifoSize = 0x00UL;
      FxU32 xTileNum = 0;

#if GLIDE_INIT_HAL
      rv = fxHalGetDeviceInfo((SstRegs*)gc->reg_ptr, &devInfo);
#else
      rv = sst1InitGetDeviceInfo(gc->base_ptr, &devInfo);
#endif
      if (!rv) {
        GDBG_INFO(gc->myLevel, "  XXXGetDeviceInfo failed. (0x%X)\n", gc->reg_ptr);
        goto BAILOUT;
      }

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
      xTileNum = devInfo.fbiVideoTilesInX;
#endif
       
#if (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR
      {
        /* Cache the # of pages for video tiles in the x direction
         * for use in grBufferClear (gglide.c).
         */
        gc->hwDep.cvgDep.tileSlopP = ((xTileNum & 0x01) != 0x00);
        gc->hwDep.cvgDep.xTilePages = (xTileNum >> 1);
        
        gc->hwDep.cvgDep.yTileShift = (gc->scanline_interleaved ? 6 : 5);

        /* Get the # of pages per buffer. */
        gc->hwDep.cvgDep.numBufferPages = devInfo.fbiVideoMemOffset;

        /* Set logical buffer -> physical buffer translations */
        gc->hwDep.cvgDep.frontBuf = 0;
        gc->hwDep.cvgDep.backBuf  = (gc->scanline_interleaved ? gc->grColBuf - 1 : 1);

        /* Initially current buffer == back buffer. */
        gc->hwDep.cvgDep.renderBuf = gc->hwDep.cvgDep.backBuf;
      }
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR */

      /* Place fifo in hw memory */
      {
#define kPageBoundarySlop 0x1000UL
#define kPageBoundaryMask (kPageBoundarySlop - 1)

        /* Init hw */
#define MB2B(__b) (((FxU32)(__b)) << 20)
#if GLIDE_INIT_HAL
        const FxU32 memEnd = MB2B(gc->fbuf_size);                                 /* size of hw memory */
        FxU32 maxOffset = ((xTileNum * yres) << 6) * (nColBuffers + nAuxBuffers); /* Bytes for buffers */
        
        /* Align on a page boundary */
        if ((maxOffset & kPageBoundaryMask) != 0x00) {
          maxOffset = ((maxOffset + kPageBoundarySlop) & ~kPageBoundaryMask);
        }
        
#if GLIDE_USE_DEBUG_FIFO
        fifoSize = 0x1000UL;
#else
        /* Too much memory claimed for other crap? */     
        fifoSize = MIN(memEnd - maxOffset, (0x01 << 18));
        rv = (maxOffset < memEnd);
        if (!rv) {
          GDBG_INFO(0, "Error: Tried to place cmd fifo (0x%X) past end of fbi mem (0x%X).\n",
                    maxOffset, memEnd);
          goto BAILOUT;
        }          
#endif
        
        /* Place fifo in hw. Taking all of the remainging memory up to the
         * byte swizzling bit in the cmd fifo address.
         */
        gc->cmdTransportInfo.fifoOffset = memEnd - fifoSize;

#if (GLIDE_PLATFORM & GLIDE_HW_H3)
        rv = fxHalInitCmdFifo((SstRegs *) gc->reg_ptr,
                              0, /* which fifo - 0 for 3d cmd fifo */
                              /* v fifoStart - offset from hw base v */
                              gc->cmdTransportInfo.fifoOffset, 
                              fifoSize, /* size - in bytes */ 
                              FXFALSE, /* directExec */
                              FXFALSE, /* disableHoles */
                              FXFALSE); /* agpEnable */
#elif (GLIDE_PLATFORM & GLIDE_HW_CVG)
        rv = fxHalInitCmdFifo(hw, /* sst */
                              0, /* which fifo - always 0 for cvg */
                              /* V fifoStart - offset from hw base V */
                              gc->cmdTransportInfo.fifoOffset,  
                              fifoSize, /* size - in bytes */
                              FXFALSE, /* directExec */
                              FXFALSE, /* disableHoles */
                              _grSet32);
#else
#error "Need HAL command fifo init routine for this hw"
#endif
#else /* !GLIDE_INIT_HAL */
        {
          /* NB: This is a dummy since we always use sstbase +
             cmdTransportInfo.fifoOffset, so we don't need a direct
             virtual address for the cmdfifo. */
          unsigned long fifoVirtAddr = 0;
          
          rv = sst1InitCmdFifo(gc->reg_ptr, FXTRUE,
                               &fifoVirtAddr,
                               &gc->cmdTransportInfo.fifoOffset,
                               &fifoSize,
                               _grSet32);
        }
#if GLIDE_USE_DEBUG_FIFO
        {
          const char* envVar = GETENV("FX_GLIDE_DEBUG_FIFO");
          const FxU32 debugFifoSize = ((envVar == NULL)
                                       ? kDebugFifoSize
                                       : atoi(envVar));
          if (fifoSize > debugFifoSize) fifoSize = debugFifoSize;
        }
#endif /* GLIDE_USE_DEBUG_FIFO */
        
#if GLIDE_USE_SHADOW_FIFO
        /* If we're debugging its nice to know what we really wrote
         * into the fifo space, just in case tracing is off.  
         */
        {
          FxU32* shadowPtr = (FxU32*)calloc(fifoSize + kPageBoundarySlop, sizeof(FxU32));

          if (shadowPtr == NULL) {
            gdbg_printf("Could not allocate shadow fifo\n");
          } else if (((FxU32)shadowPtr & kPageBoundaryMask) != 0x00) {
            shadowPtr = (FxU32*)(((FxU32)shadowPtr + kPageBoundarySlop) & ~kPageBoundaryMask);
          }
          gc->cmdTransportInfo.fifoShadowBase =
          gc->cmdTransportInfo.fifoShadowPtr  = shadowPtr;
        }
#endif /* GLIDE_USE_SHADOW_FIFO */
#endif /* !GLIDE_INIT_HAL */
        if (!rv) {
          GDBG_INFO(0, "Error: fxHalInitCmdFifo failed.\n");
          goto BAILOUT;
        }
        GR_ASSERT(fifoSize > 0);

        /* Pre-compute the packet to return us back to the start.
         *
         * NB: This is an actual hw address.
         */
        gc->cmdTransportInfo.fifoJmpHdr = (SSTCP_PKT0_JMP_LOCAL |
                                       (gc->cmdTransportInfo.fifoOffset << (SSTCP_PKT0_ADDR_SHIFT - 2)));
      }

      /* Setup virtual fifo addressing */
      {
#if (GLIDE_PLATFORM & GLIDE_HW_H3)
        /* dpc - 12 nov 1997 - WTF!?!?!? */
        gc->cmdTransportInfo.fifoStart = (FxU32 *) ((unsigned long) gc->base_ptr + 0x3c0000);
#else
        gc->cmdTransportInfo.fifoStart = (FxU32*)((unsigned long)gc->base_ptr + HW_FIFO_OFFSET);
#endif
        gc->cmdTransportInfo.fifoEnd   = 
          (FxU32*)((unsigned long)gc->cmdTransportInfo.fifoStart + fifoSize);
        gc->cmdTransportInfo.fifoSize  = fifoSize;

        /* Adjust room values. 
         * RoomToEnd needs enough room for the jmp packet since we
         * never allow the hw to auto-wrap. RoomToRead needs to be
         * adjusted so that we never acutally write onto the read ptr.
         *
         * fifoRoom is generally the min of roomToEnd and roomToRead,
         * but we 'know' here that roomToRead < roomToEnd.  
         */
        gc->cmdTransportInfo.roomToEnd       = fifoSize - FIFO_END_ADJUST;
        gc->cmdTransportInfo.fifoRoom        =
        gc->cmdTransportInfo.roomToReadPtr   =
          gc->cmdTransportInfo.roomToEnd - sizeof(FxU32); 

        /* Set initial fifo state. hw read and sw write pointers at
         * start of the fifo.
         */
        gc->cmdTransportInfo.fifoPtr  = gc->cmdTransportInfo.fifoStart;
        gc->cmdTransportInfo.fifoRead = HW_FIFO_PTR(FXTRUE);
      }

      /* We expect that lfb writes will *NOT* go through the fifo, and that the
       * fifo is ready for commands.
       */
      gc->cmdTransportInfo.fifoLfbP     = FXFALSE;
      gc->cmdTransportInfo.lfbLockCount = 0;
          
      GDBG_INFO(1, "Fifo Parameters:\n"
                "\tStart: 0x%lX\n"
                "\tHW Read: 0x%lX\n"
                "\tHW Offset: 0x%X\n"
                "\tSW Write: 0x%lX\n",
                (unsigned long)gc->cmdTransportInfo.fifoStart,
                (unsigned long)gc->cmdTransportInfo.fifoRead,
                gc->cmdTransportInfo.fifoOffset,
                (unsigned long)gc->cmdTransportInfo.fifoPtr);

#undef MB2B
    }

#if !GLIDE_INIT_HAL && 0
    /* dpc - 24 jan 1998 - FixMe!  
     * This is currently being done in _grDetectResources so that we
     * can match the nt driver semantics in win95.  
     */
    sst1InitCaching(gc->reg_ptr, FXTRUE);
#endif /* !GLIDE_INIT_HAL */
#else
#error "Need to write command transport init for glide for this hw"
#endif
  }
  
  /* We're effectively open now */
  gc->open = FXTRUE;

  /* Setup the procs that we can do w/o any mode knowledge */
  gc->curArchProcs.texDownloadProcs  = _GlideRoot.deviceArchProcs.curTexProcs;
  gc->curArchProcs.drawTrianglesProc = _GlideRoot.deviceArchProcs.curDrawTrisProc;

  /*------------------------------------------------------
    GC Init
    ------------------------------------------------------*/
  GDBG_INFO(gc->myLevel, "  GC Init\n");
  gc->state.screen_width  = xres;
  gc->state.screen_height = yres;
  gc->state.num_buffers   = nColBuffers;
  gc->state.color_format  = format;
  gc->grSstRez            = resolution;
  gc->grSstRefresh        = refresh;

  gc->lockPtrs[GR_LFB_READ_ONLY]  = (FxU32)-1;
  gc->lockPtrs[GR_LFB_WRITE_ONLY] = (FxU32)-1;
  gc->fbStride = fbStride;
  
  /* Initialize the read/write registers to all 0 */
  gc->state.fbi_config.fbzColorPath  = 0;
  gc->state.fbi_config.fogMode       = 0;
  gc->state.fbi_config.alphaMode     = 0;
  gc->state.fbi_config.fbzMode       = 0;
  gc->state.fbi_config.lfbMode       = 0;
  gc->state.fbi_config.clipLeftRight = 0;
  gc->state.fbi_config.clipBottomTop = 0;
  gc->state.fbi_config.fogColor      = 0;
  gc->state.fbi_config.zaColor       = 0;
  gc->state.fbi_config.chromaKey     = 0;
  gc->state.fbi_config.stipple       = 0;
  gc->state.fbi_config.color0        = 0;
  gc->state.fbi_config.color1        = 0;

  for (tmu = 0; tmu < gc->num_tmu; tmu += 1) {
    const GrHwConfiguration* hwConfig = &_GlideRoot.hwConfig;
    FxU32 textureMode = (FxU32)SST_SEQ_8_DOWNLD;

    /*
    if ((hwConfig->SSTs[_GlideRoot.current_sst].type == GR_SSTTYPE_VOODOO) && 
        (hwConfig->SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[tmu].tmuRev == 0)) {
      textureMode = 0;
    }
    */
    gc->state.tmu_config[tmu].textureMode     = textureMode;
    gc->state.tmu_config[tmu].tLOD            = 0x00000000;
    gc->state.tmu_config[tmu].tDetail         = 0x00000000;
    gc->state.tmu_config[tmu].texBaseAddr     = 0x00000000;
    gc->state.tmu_config[tmu].texBaseAddr_1   = 0x00000000;
    gc->state.tmu_config[tmu].texBaseAddr_2   = 0x00000000;
    gc->state.tmu_config[tmu].texBaseAddr_3_8 = 0x00000000;
    gc->state.tmu_config[tmu].mmMode          = GR_MIPMAP_NEAREST;
    gc->state.tmu_config[tmu].s_scale         = 256.f;
    gc->state.tmu_config[tmu].t_scale         = 256.f;
#ifndef GLIDE3_ALPHA
    gc->state.tmu_config[tmu].smallLod        = G3_LOD_TRANSLATE(GR_LOD_1);
    gc->state.tmu_config[tmu].largeLod        = G3_LOD_TRANSLATE(GR_LOD_1);
#else
    gc->state.tmu_config[tmu].smallLod        = G3_LOD_TRANSLATE(GR_LOD_LOG2_1);
    gc->state.tmu_config[tmu].largeLod        = G3_LOD_TRANSLATE(GR_LOD_LOG2_1);
#endif
    gc->state.tmu_config[tmu].evenOdd         = GR_MIPMAPLEVELMASK_BOTH;
    gc->state.tmu_config[tmu].nccTable        = GR_NCCTABLE_NCC0;
  } 

  /*------------------------------------------------------
    3D State Init 
    ------------------------------------------------------*/
  GDBG_INFO(gc->myLevel, "  3D State Init\n");
  gc->state.fbi_config.fbzMode = (SST_ENRECTCLIP | 
                                  SST_ENZBIAS    | 
                                  SST_DRAWBUFFER_BACK);
  
  grDisable(GR_ALLOW_MIPMAP_DITHER);
  grSstOrigin(origin);
  grAlphaBlendFunction(GR_BLEND_ONE , GR_BLEND_ZERO, 
                       GR_BLEND_ONE, GR_BLEND_ZERO);
  grAlphaTestFunction(GR_CMP_ALWAYS);
  grAlphaTestReferenceValue(0);
  grChromakeyMode(GR_CHROMAKEY_DISABLE);
  grConstantColorValue((FxU32) ~0);
  grClipWindow(0, 0, gc->state.screen_width, 
               gc->state.screen_height);
  _grColorCombineDelta0Mode(FXFALSE);
  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_ITERATED,
                 GR_COMBINE_OTHER_ITERATED,
                 FXFALSE);
  grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_NONE,
                 GR_COMBINE_OTHER_CONSTANT,
                 FXFALSE);
  grColorMask(FXTRUE, FXFALSE);
  grDepthBiasLevel(0);
  grDepthMask(FXFALSE);
  grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
  grDepthBufferFunction(GR_CMP_LESS);
  grDepthBiasLevel(0);
  grDitherMode(GR_DITHER_4x4);
  grFogMode(GR_FOG_DISABLE);
  grFogColorValue(0x00000000);
  guGammaCorrectionRGB(1.3f, 1.3f, 1.3f);
  _guTexMemReset();
#if defined(GLIDE3)
  /*
  ** initialize default state for viewport and grVertexLayout
  */
  grCoordinateSpace(GR_WINDOW_COORDS);
  grViewport(0, 0, xres, yres);
  grRenderBuffer( GR_BUFFER_BACKBUFFER );
#endif /* defined(GLIDE3) */
  grCullMode(GR_CULL_DISABLE);
  switch (gc->num_tmu) {
  case 3:
    grTexClampMode(GR_TMU2, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
    grTexDetailControl(GR_TMU2, 0, 1, 1.0F);
    grTexFilterMode(GR_TMU2, GR_TEXTUREFILTER_POINT_SAMPLED, 
                    GR_TEXTUREFILTER_POINT_SAMPLED);
    grTexLodBiasValue(GR_TMU2, 0.0F);
    grTexMipMapMode(GR_TMU2, GR_MIPMAP_DISABLE, FXFALSE);
#ifndef GLIDE3_ALPHA
    grTexCombineFunction(GR_TMU2, GR_TEXTURECOMBINE_ZERO);
#endif
    grTexCombine(GR_TMU2, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 FXFALSE, FXFALSE);
  case 2:
    grTexClampMode(GR_TMU1, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
    grTexDetailControl(GR_TMU1, 0, 1, 1.0F);
    grTexFilterMode(GR_TMU1, GR_TEXTUREFILTER_POINT_SAMPLED, 
                    GR_TEXTUREFILTER_POINT_SAMPLED);
    grTexLodBiasValue(GR_TMU1, 0.0F);
    grTexMipMapMode(GR_TMU1, GR_MIPMAP_DISABLE, FXFALSE);
    grTexCombine(GR_TMU1, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 FXFALSE, FXFALSE);
  case 1:
    grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
    grTexDetailControl(GR_TMU0, 0, 1, 1.0F);
    grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, 
                    GR_TEXTUREFILTER_POINT_SAMPLED);
    grTexLodBiasValue(GR_TMU0, 0.0F);
    grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
    grTexCombine(GR_TMU0, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 FXFALSE, FXFALSE);
  }
  grLfbConstantAlpha(0xFF);
  grLfbConstantDepth(0);

  /* --------------------------------------------------------
     Splash Screen
     --------------------------------------------------------*/
#ifdef GLIDE_SPLASH
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  if (!_GlideRoot.environment.noSplash) {
    HMODULE newSplash = LoadLibrary("3dfxspl3.dll");

    if (newSplash) {
      GrState glideState;
      FxBool didLoad;
      GrSplashProc fxSplash;
      GrSplashInitProc fxSplashInit;
      GrSplashPlugProc fxSplashPlug;
      GrSplashShutdownProc fxSplashShutdown;

      fxSplash = (GrSplashProc)GetProcAddress(newSplash, "_fxSplash@20");
      fxSplashInit = (GrSplashInitProc)GetProcAddress(newSplash, "_fxSplashInit@24");
      fxSplashPlug = (GrSplashPlugProc)GetProcAddress(newSplash, "_fxSplashPlug@16");
      fxSplashShutdown = (GrSplashShutdownProc)GetProcAddress(newSplash, "_fxSplashShutdown@0");

      didLoad = ((fxSplash != NULL) &&
                 (fxSplashInit != NULL) &&
                 (fxSplashPlug != NULL) &&
                 (fxSplashShutdown != NULL));

      if (didLoad & 0/* [dBorca] i am evil! harr-harr */) {
        /* new style DLL */
        grGlideGetState(&glideState);
        didLoad = fxSplashInit(hWnd,
                               gc->state.screen_width, gc->state.screen_height,
                               nColBuffers, nAuxBuffers,
                               format);
        if (didLoad) {
          fxSplash(0.0f, 0.0f, 
                   (float)gc->state.screen_width,
                   (float)gc->state.screen_height,
                   0);
          fxSplashShutdown();
          _GlideRoot.environment.noSplash = 1;
        }
        grGlideSetState((const void*)&glideState);
      } else {
        /* old style DLL */
        typedef int (FX_CALL *GrSplashOld) (FxU32 hWind, FxU32 scrWidth, FxU32 scrHeight, FxU32 nAuxBuffers);
        GrSplashOld fxSplashOld = (GrSplashOld)GetProcAddress(newSplash, "_fxSplash@16");
        if (fxSplashOld) {
            grGlideGetState(&glideState);
            fxSplashOld(hWnd, gc->state.screen_width, gc->state.screen_height, nAuxBuffers);
            _GlideRoot.environment.noSplash = 1;
	    grGlideSetState((const void*)&glideState);
	}
      }

      FreeLibrary(newSplash);
    }
  }
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */

        /* If it's still 0, then do the old one */
  if (!_GlideRoot.environment.noSplash) {
    grSplash(0.0f, 0.0f, 
             (float) gc->state.screen_width,
             (float) gc->state.screen_height,
             0);
    _GlideRoot.environment.noSplash = 1;
  }
#endif

  _GlideRoot.windowsInit = FXTRUE; /* to avoid race with grSstControl() */

BAILOUT:
   GR_RETURN(rv);
#undef FN_NAME
} /* grSstWinOpen */

/*-------------------------------------------------------------------
  Function: grSstWinClose
  Date: 3/16
  Implementor(s): jdt
  Library: Glide
  Description:
  Shut down the selected SST

  Shutdown has 4 steps

  3D Idle 
    the 3D engine must be idled to make sure that there are no
    commands executing in the transport when the registers are
    reset

  GC Reset
    the GC is flagged as unitialized - (nosup)

  Command Transport Disable
    the command transport to the 3D device is put in a state
    of reset.  No further commands may be issued throught the
    command transport

  Video Restore
    video is restored to its pre-open state.

  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/
#ifndef GLIDE3_ALPHA
GR_ENTRY(grSstWinClose, void, (void))
#else
GR_ENTRY(grSstWinClose, FxBool, (GrContext_t context))
#endif
{
#define FN_NAME "grSstWinClose"
  FxBool retVal;
  GR_BEGIN_NOFIFOCHECK("grSstWinClose", 80);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  retVal = ((gc != NULL) && gc->open);
  if (retVal) {
#if GLIDE_INIT_HAL
    /* dpc - 22 may 1997 - FixMe!
     * We need the equivilant stuff in the hal layer too.
     */
#else /* !GLIDE_INIT_HAL */
#if GLIDE_DEBUG || GLIDE_CHECK_COMPATABILITY
    /* Make sure that linear memory is back to the 'happy' state
     * before shutting down so that any re-inits are not going to die
     * on access violations.
     */
    pciLinearRangeSetPermission((const FxU32)gc->base_ptr,
                                0x1000000UL,
                                FXTRUE);
#endif /* GLIDE_DEBUG || GLIDE_CHECK_COMPATABILITY */

    /*--------------------------
      3D Idle
      --------------------------*/
    GDBG_INFO(gc->myLevel, "  3D Idle");
    /* dpc - 5 sep 1997
     * The old way.
     *
     * initIdle();
     */
    sst1InitIdle(gc->reg_ptr);
    
    /*--------------------------
      Command Transport Disable
      --------------------------*/
    GDBG_INFO(gc->myLevel, "  Command Transport Disable");
    /* dpc - 5 sep 1997
     * The old way.
     *
     * initDisableTransport();
     */
    /* When disabling all of the parameters other than sstBase and the
     * enable flag are ignored.  
     */
    sst1InitCmdFifo(gc->reg_ptr, FXFALSE, NULL, NULL, NULL, NULL);
    
    /*--------------------------
      Video Restore 
      --------------------------*/
    GDBG_INFO(gc->myLevel, "  Restore Video");
    /* dpc - 5 sep 1997 - FixMe!
     * Uhhh.... what is the new call to do this?
     *
     * initRestoreVideo();
     */
    /* The shutdown takes care of restoring the video etc.  The
     * shutdown also takes care of cleaning up sli now just in case
     * someone (test27) tries to re-open the sli pair later because
     * the init sequence has trouble sometimes re-initting if the
     * boards are still paired.
     *
     * NB: We do have to cleanup any processor caching characteristics
     * here because sst1InitShutdown will unlink the connection
     * between our hw address and its internal structures which keep
     * track of the mtrr's.
     */
    sst1InitIdle(gc->reg_ptr);
    sst1InitCaching(gc->base_ptr, FXFALSE);
    sst1InitShutdown(gc->reg_ptr);
#endif /* !GLIDE_INIT_HAL */

    /*--------------------------
      GC Reset
      --------------------------*/
    GDBG_INFO(gc->myLevel, "  GC Reset");

    /* open and hwInitP are really two different things.  
     *
     * hwInitP indicates whether the init code mapping/init sequence
     * is active for this hw. 
     *
     * open includes setting up video, command transport, and the
     * initial glide state.
     */
    gc->hwInitP = FXFALSE;
    if (gc->scanline_interleaved) (gc + 1)->hwInitP = FXFALSE;
      
    /* If we swapped the monitor sense on open then switch these
     * back so that any later re-mappings of the board are in sync
     * w/ the init code's mappings.  
     */
    if (gc->swapMasterSenseP) {
#define SwapPtrVal(__a, __b) do { FxU32* temp = (__a); (__a) = (__b); (__b) = temp; } while(0);
      SwapPtrVal(gc->base_ptr,  (gc + 1)->base_ptr);
      SwapPtrVal(gc->reg_ptr,   (gc + 1)->reg_ptr);
      SwapPtrVal(gc->tex_ptr,   (gc + 1)->tex_ptr);
      SwapPtrVal(gc->lfb_ptr,   (gc + 1)->lfb_ptr);
#undef SwapPtrVal

      /* Again, reset the slave pointers by hand */
      if (gc->scanline_interleaved) {
        gc->slave_ptr = (gc + 1)->base_ptr;
        (gc + 1)->slave_ptr = NULL;
      }
      
      gc->swapMasterSenseP = FXFALSE;
    }

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
    if (gc->oemInit) {
      FARPROC oemRestoreVideo = GetProcAddress(gc->oemInit, "_fxoemRestoreVideo@4");

      if (oemRestoreVideo != NULL) oemRestoreVideo(&gc->oemi);
      FreeLibrary(gc->oemInit);
    }
#endif
  }
  gc->open = FXFALSE;
    
  GR_END();
  return retVal;
#undef FN_NAME
} /* grSstWinClose */

/*-------------------------------------------------------------------
  Function: grSetNumPendingBuffers
  Date: 13-Oct-2000
  Implementor(s): mmcclure
  Description:
  
  Allow the application to supply the number of pending buffers

  Arguments:

  NumPendingBuffers - Sent to force number of pending buffers
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grSetNumPendingBuffers, void, (FxI32 NumPendingBuffers))
{
  _GlideRoot.environment.swapPendingCount = NumPendingBuffers;
}

/*-------------------------------------------------------------------
  Function: grSelectContext
  Date: 18-Jan-98
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grSelectContext, FxBool , (GrContext_t context) )
{
#define FN_NAME "grSelectContext"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 80);
  
  /*
  ** FIXME!! Not implemented in glide3 alpha
  */
  return(FXTRUE);
#undef FN_NAME
} /* grSelectConetext */

FxBool FX_CSTYLE
_grSstControl(GrControl_t code)
{
  GR_DCL_GC;
#define FN_NAME "grSstControl"  
  GDBG_INFO(41, "%s:  code = 0x%x, windowsInit = %d\n", FN_NAME,
            code, _GlideRoot.windowsInit);
  
  if (_GlideRoot.windowsInit && gc->open) {
    {
      FxBool isValidP = FXTRUE;
      FxBool passFlag;

      switch ( code ) {
      case GR_CONTROL_DEACTIVATE:
        passFlag = FXTRUE;
        break;

      case GR_CONTROL_ACTIVATE:
        passFlag = FXFALSE;
        break;

      default:
        isValidP = FXFALSE;
      }

      if (isValidP) sst1InitVgaPassCtrl(gc->base_ptr, passFlag);
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
      {
        if (gc->oemInit) {
          FARPROC oemControl = GetProcAddress(gc->oemInit, "_fxoemControl@4");
          if (oemControl)
            oemControl(code);
        }
      }
#endif

    }
  }

  GDBG_INFO(41, "%s:  Returning TRUE\n", FN_NAME);
  return FXTRUE;  
#undef FN_NAME
} /* grSstControl */

/*---------------------------------------------------------------------------
**  grSstPerfStats
*/ 
#ifndef GLIDE3_ALPHA
GR_ENTRY(grSstPerfStats, void, (GrSstPerfStats_t *pStats))
{
#define FN_NAME "grSstPerfStats"
  GR_BEGIN_NOFIFOCHECK("grSstPerfStats",83);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",pStats);
  GR_CHECK_F(FN_NAME, !pStats, "NULL pointer");

  pStats->pixelsIn   = GR_GET(hw->stats.fbiPixelsIn);
  pStats->chromaFail = GR_GET(hw->stats.fbiChromaFail);
  pStats->zFuncFail  = GR_GET(hw->stats.fbiZfuncFail);
  pStats->aFuncFail  = GR_GET(hw->stats.fbiAfuncFail);
  pStats->pixelsOut  = GR_GET(hw->stats.fbiPixelsOut);

  GR_END();
#undef FN_NAME
} /* grSstPerfStats */
#endif

/*---------------------------------------------------------------------------
**  grStatsResetPerfStats
*/

#if defined(GLIDE3) && (GLIDE3_ALPHA)
void FX_CSTYLE
_grSstResetPerfStats(void)
#else
GR_ENTRY(grSstResetPerfStats, void, (void))
#endif
{
#define FN_NAME "grSstResetPerfStats"
  GR_BEGIN("grSstResetPerfStats",83,4, 1);
  GDBG_INFO_MORE(gc->myLevel,"()\n");
  GR_SET(BROADCAST_ID, hw, nopCMD, 1);
  GR_END();
#undef FN_NAME
} /* grSstResetPerfStats */


/*---------------------------------------------------------------------------
**  grSstStatus - return contents of status register
*/

#if defined(GLIDE3) && (GLIDE3_ALPHA)
FxU32 FX_CSTYLE
_grSstStatus(void)
#else
GR_ENTRY(grSstStatus, FxU32, (void))
#endif
{
  GR_BEGIN_NOFIFOCHECK("grSstStatus", 85);

  {
    const FxU32 stat = GR_GET(hw->status);
    
    GDBG_INFO(84,"grSstStatus() => 0x%x\n", stat);
    
    return stat;
  }
}/* grSstStatus */

/*---------------------------------------------------------------------------
**  grSstVideoLine - return current video line number
*/

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
FxU32 FX_CSTYLE
_grSstVideoLine(void)
#else
GR_ENTRY(grSstVideoLine, FxU32, (void))
#endif
{
  FxU32 vline = 1;
#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
  GR_DCL_GC;
  GR_DCL_HW;

  vline = GR_GET(hw->vRetrace);
  GDBG_INFO(84,"grSstVideoLine() => 0x%x (%d)\n",vline,vline);
#endif
  return vline;
}/* grSstVideoLine */

/*---------------------------------------------------------------------------
**  grSstVRetrace - return contents of SST_VRETRACE bit of status register;
*/

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
FxBool FX_CSTYLE
_grSstVRetraceOn(void)
#else
GR_ENTRY(grSstVRetraceOn, FxBool, (void))
#endif
{
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
  return ((_grSstStatus() & SST_VRETRACE) == 0);
#else
  return ((grSstStatus() & SST_VRETRACE) == 0);
#endif
}/* grSstVRetrace */

/*---------------------------------------------------------------------------
** grSstIdle/grFinish
*/
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
GR_ENTRY(grFinish, void, (void))
#define FN_NAME "grFinish"
#else
GR_ENTRY(grSstIdle, void, (void))
#define FN_NAME "grSstIdle"
#endif
{
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  IDLE_HW(hw);

  GR_END();
#undef FN_NAME
} /* grSstIdle */

/*-------------------------------------------------------------------
  Function: grFlush
  Date: 09-Jan-98
  Implementor(s): atai
  Description:

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
GR_ENTRY(grFlush, void, (void))
{
#define FN_NAME "grFlush"
  GR_BEGIN_NOFIFOCHECK("grFlush", 83);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, nopCMD, 0x0);
  GR_CHECK_SIZE();

  GR_END();
#undef FN_NAME
} /* grSstIdle */
#endif

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
/*---------------------------------------------------------------------------
**  guGammaCorrectionRGB - set the gamma correction value
*/

GR_ENTRY(guGammaCorrectionRGB, void, (float r, float g, float b))
{
  GR_BEGIN_NOFIFOCHECK("guGammaCorrectionValue",80);
  GDBG_INFO_MORE(gc->myLevel,"(%g %g %g)\n",r, g, b);

#if GLIDE_INIT_HAL
  fxHalInitGamma(hw, gamma);
#else /* !GLIDE_INIT_HAL */
  /* dpc - 5 sep 1997 - FixMe!
   * The old way
   *
   * initGamma(gam);
   */
  sst1InitGammaRGB(gc->reg_ptr, r, g, b);
#endif /* !GLIDE_INIT_HAL */

  GR_END();
} /* guGammaCorrectionRGB */

/*-------------------------------------------------------------------
  Function: grLoadGammaTable
  Date: 05-Jan-97
  Implementor(s): atai
  Description:

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grLoadGammaTable, void, (FxU32 nentries, FxU32 *red, FxU32 *green, FxU32 *blue))
{
#define FN_NAME "grLoadGammaTable"
  FxI32 max;

  GR_BEGIN_NOFIFOCHECK("grLoadGammaTable",80);
  
  grGet(GR_GAMMA_TABLE_ENTRIES, 4, &max);
  if (nentries > (FxU32)max)
    nentries = max;
  sst1InitGammaTable(gc->reg_ptr, nentries, red, green, blue);

  GR_END();
#undef FN_NAME
}
#endif

#ifndef GLIDE3
/*---------------------------------------------------------------------------
**  grGammaCorrectionValue - set the gamma correction value
*/

GR_ENTRY(grGammaCorrectionValue, void, (float gamma))
{
  GR_BEGIN_NOFIFOCHECK("grGammaCorrectionValue",80);
  GDBG_INFO_MORE(gc->myLevel,"(%g)\n",gamma);

#if GLIDE_INIT_HAL
  fxHalInitGamma(hw, gamma);
#else /* !GLIDE_INIT_HAL */
  /* dpc - 5 sep 1997 - FixMe!
   * The old way
   *
   * initGamma(gam);
   */
  sst1InitGamma(gc->reg_ptr, gamma);
#endif /* !GLIDE_INIT_HAL */

  GR_END();
} /* grGammaCorrectionValue */
#endif

/*---------------------------------------------------------------------------
**  grSstOrigin - Set the orgin orientation of the screen.
**
**  Returns:
**
**  Notes:
**
*/

GR_STATE_ENTRY(grSstOrigin, void, (GrOriginLocation_t origin))
{
#define FN_NAME "_grSstOrigin"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("_grSstOrigin", 83);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", origin);

  /* Initialize FBZMODE register */
  fbzMode = gc->state.fbi_config.fbzMode;
  if (origin == GR_ORIGIN_LOWER_LEFT)
    fbzMode |= SST_YORIGIN;
  else
    fbzMode &= ~(SST_YORIGIN);

  /* dpc - 22 may 1997 - FixMe! 
   * Do we need to do anything here for the HAL?
   */
#if !GLIDE_INIT_HAL
  /* dpc - 5 sep 1997 - FixMe!
   * This is the old way. Is there anything else we
   * need to do here?
   *
   * initOrigin(origin); 
   */
#endif

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
  /* Sli has an 'interesting' feature where the physical scanlines
   * that are being rendered is dependent on the location of the y
   * origin. We always clear the overlap bit to black here so that
   * no one ever notices. This needs to be done for all of the allocated
   * color buffers that could have possibly have been rendered to since
   * the last origin swap. Rather than beeing really tricky I just do
   * it for every allocated color buffer.
   *
   * Origin_Lower_Left: 0:Black [1 .. screenRezY]:Rendered screenRez+1:Black
   * Origin_Upper_Left: [0 .. screenRezY - 1]:Rendered [screenRez-screenRez+1]:Black
   */
  if (gc->scanline_interleaved) gc->hwDep.cvgDep.sliOriginBufCount = gc->grColBuf;
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) */

  gc->state.fbi_config.fbzMode = fbzMode;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode, fbzMode);
  GR_CHECK_SIZE();
#endif
#undef FN_NAME
} /* grSstOrigin */

/* GMT: do we really have users for this???
 * CHD: No.
 * JDT: Huh?  If you're talking about grSstOrigin, you're smoking crack.
 *      if you are talking about SstConfigPipeline, it is evil and must
 *      be destroyed. :)
 * dpc: There is one user that I know of. This 'Nature' demo that Scott just
 *      gave me.
 * chd: It's a stub now.
 */


extern FX_ENTRY void FX_CALL
grSstConfigPipeline(GrChipID_t chip, FxU32 reg, FxU32 value);

/*---------------------------------------------------------------------------
** grSstConfigPipeline
*/

GR_ENTRY(grSstConfigPipeline, void, (GrChipID_t chip, FxU32 reg, FxU32 value))
{
} /* grSstConfigPipeline */

