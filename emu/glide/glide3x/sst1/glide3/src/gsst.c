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
** Revision 1.1.2.6  2005/06/09 18:32:35  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.2.5  2005/05/25 08:51:52  jwrdegoede
** Add #ifdef GL_X86 around x86 specific code
**
** Revision 1.1.2.4  2004/10/05 14:47:18  dborca
** conditional compilation a bit more sane
**
** Revision 1.1.2.3  2004/10/04 09:36:00  dborca
** second cut at Glide3x for Voodoo1/Rush (massive update):
** delayed validation, vertex snapping, clip coordinates, strip/fan_continue, bugfixes.
** and probably a bunch of other crap that I forgot
**
** Revision 1.1.2.2  2004/03/08 07:42:21  dborca
** Voodoo Rush fixes
**
** Revision 1.1.2.1  2004/03/02 07:55:30  dborca
** Bastardised Glide3x for SST1
**
** Revision 1.1.1.1  1999/12/07 21:48:53  joseph
** Initial checkin into SourceForge.
**
** 
** 79    7/06/98 3:05p Peter
** grSstWinOpen fails triple buffering
** 
** 78    6/04/98 12:13p Peter
** splash dll rename
** 
** 77    3/16/98 3:20p Atai
** fixed glide in a window bug. OEM DLL code bails out if res can not be
** found
** 
** 76    3/02/98 10:42a Atai
** remove oemvidtiming for DOS
** 
** 75    2/24/98 10:59a Atai
** modify for oem dll
** 
** 74    2/23/98 11:03a Peter
** 
** 73    2/12/98 8:21p Pgj
** 
** 72    2/12/98 6:48p Atai
** fix typo
** 
** 71    2/12/98 4:01p Atai
** change refresh rate if oemdll updated for tv out
 * 
 * 70    1/12/98 10:25p Dow
 * H3D Stereo Support
 * 
 * 69    1/06/98 3:44p Atai
 * fixed tv out related stuff for dos build.
 * 
 * 68    12/19/97 11:04a Atai
 * oeminit dll stuff
 * 
 * 67    12/19/97 10:23a Dow
 * Splash Screen stuff
 * 
 * 66    12/03/97 9:36a Dow
 * Fixed bug in grSstIsBusy()
 * 
 * 65    12/02/97 7:20p Dow
 * Added code for new splash screen
 * 
 * 64    10/21/97 8:20p Atai
 * added parameter for vid timing
 * 
 * 63    10/06/97 1:14p Atai
 * fix bug 824. added sli resolution checking
 * 
 * 62    9/19/97 1:24p Peter
 * more test27 fun
 * 
 * 61    9/11/97 1:33p Atai
 * In grSstWinOpen(), checking (nColBuffers+nAuxBuffers) for
 * GR_RESOLUTION_800x600 and GR_RESOLUTION_856x480
 * 
 * 60    9/10/97 10:13p Peter
 * fifo logic from GaryT, non-normalized fp first cut
 * 
 * 59    9/07/97 12:25p Pgj
 * 
 * 58    9/07/97 12:14p Pgj
 * fix bug 697
 * 
 * 57    8/19/97 8:55p Peter
 * lots of stuff, hopefully no muckage
 * 
 * 56    7/09/97 10:18a Dow
 * Further Nudge Of Love adjustments
 * 
 * 55    7/07/97 3:05p Dow
 * Moved some state BEFORE splash screen
 * 
 * 54    6/26/97 3:09p Dow
 * removed warning
 * 
 * 53    6/24/97 3:40p Dow
 * Protected Nude of Love
 * 
 * 52    6/22/97 10:19a Dow
 * Set cpuType filed inf fifoInfo before calling initEnableTransport
 * 
 * 51    6/21/97 1:05p Dow
 * Made it use the NUDGE_OF_LOVE macro
 * 
 * 50    6/21/97 12:15p Dow
 * Moved 128 DWORD nudge of love into grSstStatus()
 * 
 * 49    6/21/97 11:39a Dow
 * Fixed debug assertion
 * 
 * 48    6/19/97 7:35p Dow
 * More P6 Stuff
 * 
 * 47    6/11/97 5:30p Pgj
 * fix bug 594 (grSstPerfStats() returns inaccurate data on SLI systems)
 * 
 * 46    6/05/97 11:15p Pgj
 * 
 * 45    5/30/97 8:53a Dow
 * Protected int10Called set in grSstWinOpen
 * 
 * 44    5/28/97 2:11p Dow
 * int10h handler
 * 
 * 43    5/27/97 2:03p Dow
 * added check for new env variable
 * 
 * 42    5/22/97 11:25a Dow
 * Fixed bogus polling for idle.
 * 
 * 41    5/21/97 9:52a Jdt
 * Added grSstControl() implementation for SST-1.
 * 
 * 40    5/05/97 2:22p Dow
 * Protected previous checkin with SST96 to avoid errors when compiling
 * SST1
 * 
 * 39    5/05/97 1:35p Dow
 * Changed grSstControl to modify clip1 instead of clip0
 * 
 * 38    5/04/97 12:48p Dow
 * made grSstControl change clipLeftRight and clipBottomTop via direct
 * writes.
 * 
 * 37    5/02/97 2:11p Pgj
 * screen_width/height now FxU32
 * 
 * 36    4/15/97 12:59p Dow
 * Fixed clipping problem
 * 
 * 35    4/13/97 8:11p Jdt
 * Change in init.h
 * 
 * 34    4/13/97 2:06p Pgj
 * eliminate all anonymous unions (use hwDep)
 * 
 * 33    3/20/97 2:18a Dow
 * added a fixme
 * 
 * 32    3/19/97 10:43p Dow
 * windowsInit stuff
 * 
 * 31    3/19/97 1:37a Jdt
 * Added some stride computation
 * 
 * 30    3/18/97 9:55p Jdt
 * Moved resolution guessing into init lib.
 * 
 * 29    3/17/97 6:28a Jdt
 * Rewrite of grSstWinOpen with new init code in mind.
 * 
 * 28    3/16/97 12:39a Jdt
 * Moved over to serialstatus stored in hwDep...
 * 
 * 27    3/14/97 2:36a Murali
 * Removed printfs(), and GDBG_INFO won't work at this stage.
 * 
 * 26    3/13/97 11:02p Jdt
 * Added new splash in.
 * 
 * 25    3/13/97 5:54a Murali
 * Second set of changes for WinGlide.
 * 
 * 24    3/13/97 2:51a Jdt
 * First step towards windowed splash
 * 
 * 23    3/05/97 2:18p Murali
 * Preliminary changes for Glide in a window.
 * 
 * 22    3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 21    2/26/97 11:58a Jdt
 * Fixed grsstopen and grSstorigin
 * 
 * 20    2/19/97 4:41p Dow
 * Changed status read to use serial status on SST96
 * 
 * 19    2/14/97 12:55p Dow
 * moved vg96 fifo wrap into init code
 * 
 * 18    1/14/97 10:48a Dow
 * Modified grSstStatus to work w/ VG96
 * Made grSstIdle work with either VG or VG96  (via Init HAL)
 * 
 * 17    12/23/96 1:37p Dow
 * chagnes for multiplatform glide
 * 
 * 16    11/24/96 9:35a Tarolli
 * it now calls sst1InitNumBoardsInSystem(); instead of doing the work
 * itself
 * 
 * 15    11/15/96 4:05p Jdt
 * Dllable for vg96
**
*/

#include <stdio.h>
#include <string.h>
#include <3dfx.h>

#include <glidesys.h>
#include <sst1vid.h>

#if (GLIDE_PLATFORM & GLIDE_SST_SIM)
#include <gsim.h>
#endif

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
#include <init.h>
#endif

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include "oeminit.h"    /* NEVER used by DOS */
#include <windows.h>
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_DOS32) && (GLIDE_PLATFORM & GLIDE_HW_SST96) && GLIDE_DEBUG10
#include <dos.h>
  /* int 10h interrupt handler */
void __interrupt __far 
_grInt10Handler() {
#define FN_NAME "_grInt10Handler"
  GR_DCL_GC;

  gc->hwDep.sst96Dep.int10Called = FXTRUE;

#undef FN_NAME
} /* _grInt10Handler */
#endif


#ifdef H3D
#include <windows.h>
#define F_PATTERN 0   /* flippy pattern (sst96) */
#define T_PATTERN 1   /* tippy pattern (sst1)   */

typedef struct patColor_t { unsigned char r,g,b; } patColor;

#define PAT_BLACK   {0,0,0}
#define PAT_RED     {255,0,0}
#define PAT_GREEN   {0,255,0}
#define PAT_YELLOW  {255,255,0}

static h3dRez[]={240,300};
#define H3DHEIGHT_FROM_REZ(r) (h3dRez[(r)-GR_RESOLUTION_AUTOFLIPPED_MIN])

static GrState defstate; // default winopen state

static patColor sigPattern[] = {        // signature color pattern
  PAT_RED,PAT_RED,PAT_RED,PAT_RED,PAT_GREEN,PAT_YELLOW,PAT_YELLOW
};

static patColor disableColor[2] = {PAT_BLACK,PAT_BLACK}; // turn off adapter

#if   ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
static patColor enableColor[2] = {PAT_YELLOW,PAT_BLACK}; // use top and bottom mode 1
#elif ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
static patColor enableColor[2] = {PAT_GREEN,PAT_BLACK};  // use pageflip mode
#endif
static patColor enableColor2[2] = {PAT_YELLOW,PAT_GREEN}; // use top and bottom mode 2

// TODO : get key value for tb mode 1/2
static int getGetIntKeyValue(char * key) {
//  RegCreateKeyEx();
  return 1; // use mode 1
};

// TODO: should use _grSwizzleColor(...)
static unsigned long patColorToGR(patColor clr, GrColorFormat_t fmt) {
  switch (fmt) {
  case GR_COLORFORMAT_ARGB:
    return ((unsigned long)(255<<24)|(unsigned long)(clr.r<<16)|((unsigned long)clr.g<<8)|(clr.b));
  case GR_COLORFORMAT_ABGR:
    return ((unsigned long)(255<<24)|(unsigned long)(clr.b<<16)|((unsigned long)clr.g<<8)|(clr.r));
  case GR_COLORFORMAT_RGBA:
    return ((unsigned long)(clr.r<<24)|(unsigned long)(clr.g<<16)|((unsigned long)clr.b<<8)|(255));
  case GR_COLORFORMAT_BGRA:
    return ((unsigned long)(clr.b<<24)|(unsigned long)(clr.g<<16)|((unsigned long)clr.r<<8)|(255));
  }
}

static void drawlinefb(GrVertex * lineLeft, GrVertex * lineRight) {
  grRenderBuffer(GR_BUFFER_FRONTBUFFER);
  grDrawLine(lineLeft, lineRight);
  grRenderBuffer(GR_BUFFER_BACKBUFFER);
  grDrawLine(lineLeft, lineRight);
}

static void initState(void) {
  grAlphaBlendFunction( GR_BLEND_ONE , GR_BLEND_ZERO, 
                        GR_BLEND_ONE, GR_BLEND_ZERO );
  grAlphaTestFunction( GR_CMP_ALWAYS );
  grAlphaTestReferenceValue( 0 );
  grChromakeyMode( GR_CHROMAKEY_DISABLE );
  grConstantColorValue( ( FxU32 ) ~0 );
  _grColorCombineDelta0Mode( FXFALSE );
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
  grColorMask( FXTRUE, FXFALSE );
  grCullMode( GR_CULL_DISABLE );
  grDepthBiasLevel( 0 );
  grDepthMask( FXFALSE );
  grDepthBufferMode( GR_DEPTHBUFFER_DISABLE );
  grDepthBufferFunction( GR_CMP_ALWAYS );
  grDepthBiasLevel( 0 );
  grDitherMode( GR_DITHER_4x4 );
  grFogMode( GR_FOG_DISABLE );
  grFogColorValue( 0x00000000 );
  grDisableAllEffects();
}

void setAutoflip(int enable) {
  GrVertex lineLeft, lineRight;
  GrState state;
  int scrWidth = grSstScreenWidth();
  int line;
  MSG msg;
  GrColorFormat_t format;
  static firstupdate = 1;
  extern void initAT3DSetTiles(FxU32 PageFlipping, FxU32 nBuffers);
  GR_DCL_GC;

  // wait for the window messages to flush
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  grGlideGetState(&state); // save the app. state

  initState();

#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
  if (firstupdate) {
    initAT3DSetTiles(0,gc->state.num_buffers);
    firstupdate = 0;
  } /* TODO:: setup regs for non-pageflipping seems to fix fb alignment... */
  if (enable) {
    grSstIdle();
    initAT3DSetTiles(1,gc->state.num_buffers);
  } /* setup regs for pageflipping */
#endif

  grDepthBufferFunction( GR_CMP_ALWAYS );
  grDepthMask( FXFALSE );

  grColorCombine( GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_NONE,
                  FXFALSE );

  format = state.color_format;
  lineLeft.x = 0.f;
  lineRight.x = (float)scrWidth-1;
  lineLeft.z = lineRight.z = 0.f;
  lineLeft.ooz = lineRight.ooz = 65535.f;
#if   ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  lineLeft.y = lineRight.y = 2.f;
#elif ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
  lineLeft.y = lineRight.y = grSstScreenHeight() - H3DHEIGHT_FROM_REZ(gc->grSstRez) + 2.f;
#endif

  grColorMask(FXTRUE,0);
  grDepthBufferFunction( GR_CMP_ALWAYS );
  grClipWindow(0,0,grSstScreenWidth(),grSstScreenHeight());
  grRenderBuffer(GR_BUFFER_FRONTBUFFER);
  grBufferClear( 0x00, 0, GR_ZDEPTHVALUE_FARTHEST );

  grRenderBuffer(GR_BUFFER_BACKBUFFER);
  grBufferClear( 0x00, 0, GR_ZDEPTHVALUE_FARTHEST );

  grBufferSwap(1);
  grBufferClear( 0x00, 0, GR_ZDEPTHVALUE_FARTHEST );
  grSstOrigin(GR_ORIGIN_UPPER_LEFT);

  for (line = 0; line < 7; line++) {
    grConstantColorValue( patColorToGR(sigPattern[line],format) );
    drawlinefb(&lineLeft, &lineRight);
    lineLeft.y = ++lineRight.y;

    grConstantColorValue( 0 );
    drawlinefb(&lineLeft, &lineRight);
    lineLeft.y = ++lineRight.y;
  }

  for (line = 0; line < 2; line++) {
    grConstantColorValue( patColorToGR(enable?(getGetIntKeyValue("\\SOFTWARE\\H3D\\TBMODE")==1?enableColor[line]:enableColor2[line]):disableColor[line],format) );
    drawlinefb(&lineLeft, &lineRight);
    lineLeft.y = ++lineRight.y;

    grConstantColorValue( 0 );
    drawlinefb(&lineLeft, &lineRight);
    lineLeft.y = ++lineRight.y;
  }

  // wait for adapter sync-up
#if   ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  for(line = 40; line>0; line--) {
#elif ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
  for(line = 20; line>0; line--) {
#endif
    grBufferSwap(1);
  }

#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
  if (!enable) {
    // grSstIdle();
    initAT3DSetTiles(0,gc->state.num_buffers);
  } // setup regs for non-pageflipping
#endif

  line = 50000;
  while ((grBufferNumPending() > 2) && (--line));

  grGlideSetState(&state);  // restore app. state

  // restore
#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
  if (GR_RESOLUTION_IS_AUTOFLIPPED(gc->grSstRez)) {
    if (enable==1) {
      switch (gc->grSstRez) {
      case GR_RESOLUTION_640x240_AUTOFLIPPED:
        gc->state.screen_height = 502;
        break;
      case GR_RESOLUTION_800x300_AUTOFLIPPED:
        gc->state.screen_height = 620;
        break;
      }
    } else {
      switch (gc->grSstRez) {
      case GR_RESOLUTION_640x240_AUTOFLIPPED:
        gc->state.screen_height = 480;
      case GR_RESOLUTION_800x300_AUTOFLIPPED:
        gc->state.screen_height = 600;
      }
    }
    grClipWindow(0,0,grSstScreenWidth(),grSstScreenHeight());
  }
#endif
}

#endif

/*-------------------------------------------------------------------
  Function: grSstWinOpen
  Date: 3/16
  Implementor(s): dow, gmt, murali, jdt
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
                2 - allocate on depth and one alpha buffer ( unsup )
  Return:
  FXTRUE - glide successfully acquired the necessary resources and a
           is ready for rendering
  FXFALSE - glide was unsuccessful in getting the necessary resources, 
            or the requested configuration is unavailble on the host
            hardware - any calls to glide rendering routines will result
            in undefined behavior.
  -------------------------------------------------------------------*/
GR_ENTRY(grSstWinOpen, GrContext_t, (FxU32                   hWnd,
                                     GrScreenResolution_t    resolution,
                                     GrScreenRefresh_t       refresh,
                                     GrColorFormat_t         format,
                                     GrOriginLocation_t      origin,
                                     int                     nColBuffers,
                                     int                     nAuxBuffers))
{
  GrContext_t 
    rv = 0x00UL;
  int 
    tmu,
    xres, yres,
    fbStride;
  InitFIFOData 
    fifoInfo;
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  FARPROC 
    oemInitMapBoard = NULL,
    oemInitSetVideo = NULL,
    oemInitVideoTiming = NULL;
  OemInitInfo oemi;
  sst1VideoTimingStruct *sstVideoRez = NULL, tvVidtiming;
  FxBool oemvidtiming = FXFALSE;
  FxU32 tvVidtiming_refreshRate = refresh;
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */

  GR_BEGIN_NOFIFOCHECK("grSstWinOpen",80);
  GDBG_INFO_MORE((gc->myLevel,
                  "(rez=%d,ref=%d,cformat=%d,origin=%s,#bufs=%d, #abufs=%d)\n",
                  resolution,refresh,format,
                  origin ? "LL" : "UL",
                  nColBuffers, nAuxBuffers));
  GR_CHECK_F("grSstWinOpen", !gc, "no SST selected as current (gc==NULL)");

#if !defined(__linux__) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
  if (!hWnd)
    GrErrorCallback("grSstWinOpen: need to use a valid window handle",
                    FXTRUE);
#endif

  if (!_GlideRoot.environment.ignoreReopen) {
    GR_CHECK_F("grSstWinOpen", gc->open, "gc opened twice!" );
  }

  /* If we've been closed via grSstWinClose then we need to re-init
   * the hw registers etc before init-ing video etc or bad ju-ju is
   * in our future.
   */
  if (gc->closedP) {
    if (!(pciOpen() &&
          (initMapBoard(_GlideRoot.current_sst) != NULL))) return FXFALSE;
    gc->closedP = FXFALSE;
  }

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32) && (!defined(GLIDE_DEBUG) || (GLIDE_DEBUG == 0))
  /*
  ** load fxoem2x.dll and map board
  */
  oemi.version = OEMINIT_VERSION;
#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
  oemi.vendorID = sst1BoardInfo[_GlideRoot.current_sst].vendorID;
  oemi.deviceID = sst1BoardInfo[_GlideRoot.current_sst].deviceID;
#else /* SST96 */
  { InitDeviceInfo info;
    initGetDeviceInfo(_GlideRoot.current_sst, &info);
    oemi.vendorID = info.vendorID;
    oemi.deviceID = info.deviceID;
  }
#endif
  oemi.boardID = OEMINIT_INVALID_BOARD_ID;
  oemi.subvendorID = OEMINIT_INVALID_BOARD_ID;
  oemi.linearAddress = gc->base_ptr;
  oemi.slaveAddress = NULL;
  if ((gc->oemInit = LoadLibrary("fxoem2x.dll")) != NULL) {
    oemInitMapBoard = GetProcAddress(gc->oemInit, "_fxoemInitMapBoard@4");
    if (oemInitMapBoard) {
      oemInitMapBoard(&oemi);
    }
  }
#if defined(FX_OEM_BUILD) && !defined(FX_STATIC_BUILD)
  else {
    GrErrorCallback("Missing fxoem2x.dll", FXTRUE);
    grSstWinClose();
    exit(0);
  }
#endif /* FX_STATIC_BUILD */
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) && (!defined(GLIDE_DEBUG) || (GLIDE_DEBUG == 0)) */

  /*------------------------------------------------------
    Validate and Parse Arguments
    ------------------------------------------------------*/
  /*
     FIXME!! jdt

     There needs to be one large table of resolutions vs. 
     nColBuffers vs. nAuxBuffers that specifies required
     ram and vald/invalid configurations
   */
#define SLI_DETECT _GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.sliDetect
/* this is for SST1 only: for an SST96 board, the above overlays with SST96Config.tmuRev == 1 ... */
#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
  if ((!SLI_DETECT) &&
      ((gc->fbuf_size <= 2 &&
        resolution == GR_RESOLUTION_800x600 &&
        (nColBuffers+nAuxBuffers) >= 3 ) ||
       (gc->fbuf_size <= 2 &&
        resolution == GR_RESOLUTION_856x480 &&
        (nColBuffers+nAuxBuffers) >= 3 ) ||
       (gc->fbuf_size <= 2 &&
        resolution == GR_RESOLUTION_960x720 ) ))
    {
      GDBG_INFO(( gc->myLevel, "grSstWinOpen failed "
                  "for illegal screen resolution or "
                  "insufficient memory\n" ));
      goto BAILOUT;
    }
#endif /* (GLIDE_PLATFORM & GLIDE_HW_SST1) */

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
  if (resolution == GR_RESOLUTION_NONE) {
    goto BAILOUT;
  }
#endif

#ifdef H3D
  if (!GR_RESOLUTION_IS_AUTOFLIPPED(resolution)) {
#endif
  if ((resolution < GR_RESOLUTION_MIN) ||
      ((resolution > GR_RESOLUTION_MAX) &&
       (resolution != GR_RESOLUTION_NONE )) ) {
      GDBG_INFO(( gc->myLevel, "grSstWinOpen failed "
                  "for illegal screen resolution\n" ));
      goto BAILOUT;
    }
#ifdef H3D
  } else {      // is autoflipped resolution
  if ((resolution < GR_RESOLUTION_AUTOFLIPPED_MIN) ||
      (resolution > GR_RESOLUTION_AUTOFLIPPED_MAX) ) {
      GDBG_INFO(( gc->myLevel, "grSstWinOpen failed "
                  "for illegal autoflipped screen resolution\n" ));
      goto BAILOUT;
    }
  }
#endif

#if ( !( GLIDE_PLATFORM & GLIDE_SST_SIM ) )
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
#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
    if(!(sstVideoRez = sst1InitFindVideoTimingStruct(resolution,refresh)))
      goto BAILOUT;
    tvVidtiming = *sstVideoRez;
    oemi.vid.res = resolution;
    oemi.vid.refresh = refresh;
    oemi.vid.depth = 16;
    oemi.vid.hSyncOn = tvVidtiming.hSyncOn;
    oemi.vid.hSyncOff = tvVidtiming.hSyncOff;
    oemi.vid.vSyncOn = tvVidtiming.vSyncOn;
    oemi.vid.vSyncOff = tvVidtiming.vSyncOff;
    oemi.vid.hBackPorch = tvVidtiming.hBackPorch;
    oemi.vid.vBackPorch = tvVidtiming.vBackPorch;
    oemi.vid.xDimension = tvVidtiming.xDimension;
    oemi.vid.yDimension = tvVidtiming.yDimension;
    oemi.vid.clkFreq16bpp = tvVidtiming.clkFreq16bpp;
    oemi.vid.clkFreq24bpp = tvVidtiming.clkFreq24bpp;

    if (gc->oemInit) {
      oemInitVideoTiming = GetProcAddress(gc->oemInit, "_fxoemInitVideoTiming@4");
      if (oemInitVideoTiming && oemInitMapBoard)
        oemvidtiming = oemInitVideoTiming(&oemi);
      /*
      ** video timing is updated by oem dll
      */
      if (oemvidtiming) {
        tvVidtiming.hSyncOn = oemi.vid.hSyncOn;
        tvVidtiming.hSyncOff = oemi.vid.hSyncOff;
        tvVidtiming.vSyncOn = oemi.vid.vSyncOn;
        tvVidtiming.vSyncOff = oemi.vid.vSyncOff;
        tvVidtiming.hBackPorch = oemi.vid.hBackPorch;
        tvVidtiming.vBackPorch = oemi.vid.vBackPorch;
        tvVidtiming.xDimension = oemi.vid.xDimension;
        tvVidtiming.yDimension = oemi.vid.yDimension;
        tvVidtiming.clkFreq16bpp = oemi.vid.clkFreq16bpp;
        tvVidtiming.clkFreq24bpp = oemi.vid.clkFreq24bpp;
      }
    }
#else
    oemvidtiming = 0;
    oemi.vid.refresh = 0;
#endif
  }
#endif
  GDBG_INFO((gc->myLevel, "  Video Init\n" ));
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  if (oemvidtiming)
    rv = initSetVideo( hWnd, resolution, 
                       oemi.vid.refresh, format, origin, 
                       nColBuffers, nAuxBuffers,
                       &xres, &yres, &fbStride, &tvVidtiming );
  else
#endif
    rv = initSetVideo( hWnd, resolution, 
                       refresh, format, origin, 
                       nColBuffers, nAuxBuffers,
                       &xres, &yres, &fbStride, _GlideRoot.GCs[_GlideRoot.current_sst].vidTimings );
  if ( !rv ) goto BAILOUT;
  GDBG_INFO((gc->myLevel, 
             "  Video init succeeded. xRes = %.04d, yRes = %.04d\n",
             xres, yres ));

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  if (gc->oemInit) {
    oemInitSetVideo = GetProcAddress(gc->oemInit, "_fxoemInitSetVideo@4");
    if (oemInitSetVideo && oemInitMapBoard)
      oemInitSetVideo(&oemi);
  }
#endif

  /*------------------------------------------------------
    Command Transport Init
    ------------------------------------------------------*/
  GDBG_INFO((gc->myLevel, "  Command Transport Init\n" ));

  /* Set up FifoInfo with CPU Type so that init code knows if it needs
     to advance the write pointer beyond the read pointer for P6 */

#if GL_X86
  fifoInfo.cpuType = _GlideRoot.CPUType;
#endif

  rv = initEnableTransport( &fifoInfo );
  if ( !rv ) goto BAILOUT;

  gc->nopCMD = FXFALSE;

#  if   ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  _grHints( GR_HINT_FIFOCHECKHINT, 
           fifoInfo.hwDep.vgFIFOData.memFifoStatusLwm + 0x100 );
  _grReCacheFifo( 0 );
#  elif ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
  gc->hwDep.sst96Dep.writesSinceFence = 0;
#if (GLIDE_PLATFORM & GLIDE_OS_DOS32) && GLIDE_DEBUG10
  /* Set int 10h interrupt handler */
  gc->hwDep.sst96Dep.int10Called = FXFALSE;
  gc->hwDep.sst96Dep.prevInt10 = NULL;
  GDBG_INFO((80, "Storing old int 10h interrupt handler (0x%x)\n", gc->hwDep.sst96Dep.prevInt10));  

  GDBG_INFO((80, "Overriding int 10h interrupt handler\n"));
  _dos_setvect(0x10, _grInt10Handler);
#endif

  gc->fifoData = fifoInfo;

  GDBG_INFO((120, "After Init:  fifoPtr  = 0x%x\n", 
             fifoInfo.hwDep.vg96FIFOData.fifoPtr ));
  GDBG_INFO((120, "After Init:  fifoSize = 0x%x\n",
             fifoInfo.hwDep.vg96FIFOData.fifoSize ));
  GDBG_INFO((120, "After Init:  fifoVirt = 0x%x\n",
             fifoInfo.hwDep.vg96FIFOData.fifoVirt ));
  GDBG_INFO((120, "After Init:  fifoPhys = 0x%x\n",
             fifoInfo.hwDep.vg96FIFOData.fifoPhys ));
  GDBG_INFO((120, "After Init:  fifoMax  = 0x%x\n",
             fifoInfo.hwDep.vg96FIFOData.fifoMax ));
  GDBG_INFO((120, "After Init:  fifoCur  = 0x%x\n",
             fifoInfo.hwDep.vg96FIFOData.fifoCur ));
  GDBG_INFO((120, "After Init:  fifoLfb  = 0x%x\n",
             fifoInfo.hwDep.vg96FIFOData.fifoLfb ));
#  else
#    error "Need to write command transport init for glide for this hw"
#  endif

#else
  gsim_setFBsize(gc->state.screen_width, 
                 gc->state.screen_height );
#endif

  /* We're all inited now. No more failures from this ppoint on. */
  gc->open = FXTRUE;
  _GlideRoot.windowsInit = FXTRUE; /* to avoid race with grSstControl() */

  rv = (GrContext_t)gc;

  /*------------------------------------------------------
    GC Init
    ------------------------------------------------------*/
  GDBG_INFO((gc->myLevel, "  GC Init\n" ));
  gc->state.screen_width  = xres;
  gc->state.screen_height = yres;
  gc->state.num_buffers   = nColBuffers;
  gc->state.color_format  = format;
  gc->grSstRez            = resolution;
  gc->grSstRefresh        = refresh;

  gc->lockPtrs[GR_LFB_READ_ONLY]  = (FxU32)-1;
  gc->lockPtrs[GR_LFB_WRITE_ONLY] = (FxU32)-1;
  gc->lfbSliOk = 0;
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
  for (tmu = 0; tmu < gc->num_tmu; tmu += 1)
    {
      FxU32 textureMode = (FxU32)SST_SEQ_8_DOWNLD;
      if ((_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type == GR_SSTTYPE_VOODOO) && 
          (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[tmu].tmuRev == 0)) {
          textureMode = 0;
      }
      gc->state.tmu_config[tmu].textureMode     = textureMode;
      gc->state.tmu_config[tmu].tLOD            = 0x00000000;
      gc->state.tmu_config[tmu].tDetail         = 0x00000000;
      gc->state.tmu_config[tmu].texBaseAddr     = 0x00000000;
      gc->state.tmu_config[tmu].texBaseAddr_1   = 0x00000000;
      gc->state.tmu_config[tmu].texBaseAddr_2   = 0x00000000;
      gc->state.tmu_config[tmu].texBaseAddr_3_8 = 0x00000000;
      gc->state.tmu_config[tmu].mmMode          = GR_MIPMAP_NEAREST;
      gc->state.tmu_config[tmu].st_scale[0]     = 256.f;
      gc->state.tmu_config[tmu].st_scale[1]     = 256.f;
      gc->state.tmu_config[tmu].smallLod        = G3_LOD_TRANSLATE(GR_LOD_LOG2_1);
      gc->state.tmu_config[tmu].largeLod        = G3_LOD_TRANSLATE(GR_LOD_LOG2_1);
      gc->state.tmu_config[tmu].evenOdd         = GR_MIPMAPLEVELMASK_BOTH;
      gc->state.tmu_config[tmu].nccTable        = GR_NCCTABLE_NCC0;
    }

  /*------------------------------------------------------
    3D State Init
    ------------------------------------------------------*/
  GDBG_INFO((gc->myLevel, "  3D State Init\n" ));
  gc->state.fbi_config.fbzMode = 
    SST_ENRECTCLIP | 
    SST_ENZBIAS    | 
    SST_DRAWBUFFER_BACK;

  grDisable(GR_ALLOW_MIPMAP_DITHER);
  grSstOrigin( origin );
  grAlphaBlendFunction( GR_BLEND_ONE , GR_BLEND_ZERO, 
                        GR_BLEND_ONE, GR_BLEND_ZERO );
  grAlphaTestFunction( GR_CMP_ALWAYS );
  grAlphaTestReferenceValue( 0 );
  grChromakeyMode( GR_CHROMAKEY_DISABLE );
  grConstantColorValue( ( FxU32 ) ~0 );
  grClipWindow( 0, 0, gc->state.screen_width, 
                gc->state.screen_height );
  _grColorCombineDelta0Mode( FXFALSE );
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
  grColorMask( FXTRUE, FXFALSE );
  grCullMode( GR_CULL_DISABLE );
  grDepthBiasLevel( 0 );
  grDepthMask( FXFALSE );
  grDepthBufferMode( GR_DEPTHBUFFER_DISABLE );
  grDepthBufferFunction( GR_CMP_LESS );
  grDepthBiasLevel( 0 );
  grDitherMode( GR_DITHER_4x4 );
  grFogMode( GR_FOG_DISABLE );
  grFogColorValue( 0x00000000 );
  guGammaCorrectionRGB(1.7f, 1.7f, 1.7f);
  /*
  ** initialize default state for viewport and grVertexLayout
  */
  grCoordinateSpace(GR_WINDOW_COORDS);
  grViewport(0, 0, xres, yres);
  grRenderBuffer( GR_BUFFER_BACKBUFFER );
  switch (gc->num_tmu) {
  case 3:
    grTexClampMode( GR_TMU2, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP );
    grTexDetailControl( GR_TMU2, 0, 1, 1.0F );
    grTexFilterMode( GR_TMU2, GR_TEXTUREFILTER_POINT_SAMPLED, 
                     GR_TEXTUREFILTER_POINT_SAMPLED );
    grTexLodBiasValue( GR_TMU2, 0.0F);
    grTexMipMapMode( GR_TMU2, GR_MIPMAP_DISABLE, FXFALSE );
    grTexCombine( GR_TMU2, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  FXFALSE, FXFALSE );
  case 2:
    grTexClampMode( GR_TMU1, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP );
    grTexDetailControl( GR_TMU1, 0, 1, 1.0F );
    grTexFilterMode( GR_TMU1, GR_TEXTUREFILTER_POINT_SAMPLED, 
                     GR_TEXTUREFILTER_POINT_SAMPLED );
    grTexLodBiasValue( GR_TMU1, 0.0F);
    grTexMipMapMode( GR_TMU1, GR_MIPMAP_DISABLE, FXFALSE );
    grTexCombine( GR_TMU1, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  FXFALSE, FXFALSE );
  case 1:
    grTexClampMode( GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP );
    grTexDetailControl( GR_TMU0, 0, 1, 1.0F );
    grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, 
                     GR_TEXTUREFILTER_POINT_SAMPLED );
    grTexLodBiasValue( GR_TMU0, 0.0F);
    grTexMipMapMode( GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE );
    grTexCombine( GR_TMU0, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  FXFALSE, FXFALSE );
  }
  grLfbConstantAlpha( 0xFF );
  grLfbConstantDepth( 0 );

#ifdef H3D
  if (GR_RESOLUTION_IS_AUTOFLIPPED(resolution)) {
    setAutoflip(1);
  } else
#endif

  {
    GrState
      state;

    grGlideGetState(&state);

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)

    grSstOrigin( GR_ORIGIN_LOWER_LEFT );

#ifdef GLIDE_SPLASH
    if (!_GlideRoot.environment.noSplash) {
      HMODULE newSplash = LoadLibrary("3dfxspl3.dll");

      if (newSplash) {
        FARPROC fxSplash = GetProcAddress(newSplash, "_fxSplash@16");

        if (fxSplash) {
          fxSplash(hWnd, gc->state.screen_width,
                   gc->state.screen_height, nAuxBuffers);
          _GlideRoot.environment.noSplash = 1;
        }
        FreeLibrary(newSplash);
      }
    }
#endif
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */

#ifdef GLIDE_SPLASH
    /* If it's still 0, then do the old one */
    if (!_GlideRoot.environment.noSplash) {
      grSplash(0.0f, 0.0f, 
               (float) gc->state.screen_width,
               (float) gc->state.screen_height,
               0);
      _GlideRoot.environment.noSplash = 1;
    }
#endif

    grGlideSetState(&state);
  }

  BAILOUT:
  GR_RETURN( rv );
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
    the GC is flagged as unitialized - ( nosup )

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
GR_ENTRY(grSstWinClose, FxBool, (GrContext_t context))
{
#define FN_NAME "grSstWinClose"
  GR_BEGIN_NOFIFOCHECK("grSstWinClose",80);
  GDBG_INFO_MORE((gc->myLevel,"()\n"));

  GR_ASSERT((GrContext_t)gc == context);

  if ( (gc != NULL) && gc->open ) {
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
    FARPROC oemRestoreVideo = NULL;
#endif
    /*--------------------------
      3D Idle
      --------------------------*/
    GDBG_INFO(( gc->myLevel, "  3D Idle\n" ));
    initIdle();

    /*--------------------------
      GC Reset
      --------------------------*/
    GDBG_INFO(( gc->myLevel, "  GC Reset\n" ));
    gc->open = FXFALSE;
    gc->closedP = FXTRUE;

    /*--------------------------
      Command Transport Disable
      --------------------------*/
    GDBG_INFO(( gc->myLevel, "  Command Transport Disable\n" ));
    initDisableTransport();

#if (GLIDE_PLATFORM & GLIDE_OS_DOS32) && (GLIDE_PLATFORM & GLIDE_HW_SST96) && GLIDE_DEBUG10
    GDBG_INFO((80, "Restoring int 10h interrupt handler (0x%x)\n", gc->hwDep.sst96Dep.prevInt10));
    _dos_setvect(0x10, NULL);
#endif

    /*--------------------------
      Video Restore
      --------------------------*/
    GDBG_INFO(( gc->myLevel, "  Restore Video\n" ));
    initRestoreVideo();
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
    if (gc->oemInit) {
      oemRestoreVideo = GetProcAddress(gc->oemInit, "_fxoemRestoreVideo@0");
      if (oemRestoreVideo)
        oemRestoreVideo();
      FreeLibrary(gc->oemInit);
    }
#endif
  }

  GR_END();
  return FXTRUE;
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

  return(FXTRUE);
#undef FN_NAME
} /* grSelectConetext */

FxBool FX_CSTYLE
_grSstControl(GrControl_t code)
{
#define FN_NAME "grSstControl"

  GDBG_INFO((41, "%s:  code = 0x%x, windowsInit = %d\n", FN_NAME,
             code, _GlideRoot.windowsInit));

  if (_GlideRoot.windowsInit) {
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
    /* For VG96, windows haven't been created, so there's no point
     * trying to control it.
     */
    /* Believe it or not, the following code really should be bracketed
       like this.  The reason is that GR_BEGIN_NOFIFOCHECK seg faults
       when grSstControl is called before the Glide window is truly
       initialized.  This is a real concern, as grSstControl is called
       from event loops, which are asynchronous.
       */
    FxU32
      status,
      xRes, yRes,
      clipLeftRight, clipBottomTop;
    FxU32 nTries;

    GR_BEGIN_NOFIFOCHECK(FN_NAME,80);

    /* We're either cleaning up or closed so forget about this one. */
    if (gc->closedP) goto __done;
    xRes = initControl(code);
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
    {
      if (gc->oemInit) {
        FARPROC oemControl = GetProcAddress(gc->oemInit, "_fxoemControl@4");
        if (oemControl)
          oemControl(code);
      }
    }
#endif

    GDBG_INFO((80, "%s:  initControl returned 0x%x\n", FN_NAME, xRes));

    if (!xRes) return FXFALSE;

    yRes = xRes >> 16;
    xRes = xRes & 0xffff;

    /* !! FIXME +++ this call should properly update the gc->fbStride,
       this information is known only to the init layer and needs to be
       exposed, it is only really necessary to lfblock right now,
       and therefore is returned by initGetBufferPtr */

    gc->state.screen_width = xRes;
    gc->state.screen_height = yRes;

    /* Wait for Idle. */

    nTries = 0;
    do {
      if (nTries++ > 999) {
        GDBG_INFO((80, "%s:  returning FALSE after %d checks for idle\n", 
                   FN_NAME, nTries));
        return FXTRUE;
      }

      status = GET(hw->status);

    } while (status & 0x1);

    /* Set ClipRect Via direct writes */
    _grClipNormalizeAndGenerateRegValues(0, 0, xRes, yRes,
                                         &clipLeftRight,
                                         &clipBottomTop);
    SET_DIRECT(clipLeftRight1, clipLeftRight);
    SET_DIRECT(clipBottomTop1, clipBottomTop);

    __done:
    /* Do Nothing */;
#elif (GLIDE_PLATFORM&GLIDE_HW_SST1)
    FxBool ctrlflag;
    ctrlflag = initControl(code);
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
    {
      GR_DCL_GC;
      if (gc->oemInit) {
        FARPROC oemControl = GetProcAddress(gc->oemInit, "_fxoemControl@4");
        if (oemControl)
          oemControl(code);
      }
    }
#endif
    return ctrlflag;
#endif
  }
  GDBG_INFO((41, "%s:  Returning TRUE\n", FN_NAME));
  return FXTRUE;

#undef FN_NAME
} /* grSstControl */

/*---------------------------------------------------------------------------
**  grSstPerfStats
*/

GR_ENTRY(grSstPerfStats, void, (GrSstPerfStats_t *pStats))
{
#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
  FxU32 jr;
#endif

  GR_BEGIN_NOFIFOCHECK("grSstPerfStats",83);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",pStats));
  GR_CHECK_F(myName, !pStats, "NULL pointer");

#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
/*
 * fixme! this is a fix for bug 697.  The right thing to do is
 * define GR_GET properly in fxglide.h, but this breaks lots of other
 * things, and this is the only place in glide where we actually read
 * sst-1 registers, and this pools is mature, so... Also, note that
 * these reads are not synchronized with the FIFO, so results are
 * approximate.  I'm not going to change this behavior for fear of
 * introducing performance degradation in existing apps that may
 * be instrumented with this call.  The read of jr is a sanity check.
 */
#undef GETINDEX
#undef GETREADADDR
#undef GR_GET
#define GR_GET(s) *(GETREADADDR(s))
#define GETINDEX(a) ((((FxU32) &a) - (((FxU32) hw) + 0x400000)) >> 2)
#define GETREADADDR(s) ((FxU32 *) (((FxU32) hw) + (GETINDEX(s) <<12)))
  jr = GR_GET(hw->fbijrVersion);
#endif /* SST96 */
  pStats->pixelsIn   = GR_GET(hw->stats.fbiPixelsIn);
  pStats->chromaFail = GR_GET(hw->stats.fbiChromaFail);
  pStats->zFuncFail  = GR_GET(hw->stats.fbiZfuncFail);
  pStats->aFuncFail  = GR_GET(hw->stats.fbiAfuncFail);
  pStats->pixelsOut  = GR_GET(hw->stats.fbiPixelsOut);

#if ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  if (gc->scanline_interleaved == FXTRUE) {
    sst1InitSliPciOwner(gc->base_ptr, SST_SLI_SLAVE_OWNPCI);

    pStats->pixelsIn   += GR_GET(hw->stats.fbiPixelsIn);
    pStats->chromaFail += GR_GET(hw->stats.fbiChromaFail);
    pStats->zFuncFail  += GR_GET(hw->stats.fbiZfuncFail);
    pStats->aFuncFail  += GR_GET(hw->stats.fbiAfuncFail);
    pStats->pixelsOut  += GR_GET(hw->stats.fbiPixelsOut);

    sst1InitSliPciOwner(gc->base_ptr, SST_SLI_MASTER_OWNPCI);
  }
#endif
    
  GR_END();
} /* grSstPerfStats */

/*---------------------------------------------------------------------------
**  grStatsResetPerfStats
*/

void FX_CSTYLE
_grSstResetPerfStats(void)
{
  GR_BEGIN("grSstResetPerfStats",83,4);
  GDBG_INFO_MORE((gc->myLevel,"()\n"));
  P6FENCE_CMD( GR_SET(hw->nopCMD,1) );
  GR_END();
} /* grSstResetPerfStats */


/*---------------------------------------------------------------------------
**  grSstStatus - return contents of status register
*/

FxU32 FX_CSTYLE
_grSstStatus(void)
{
  FxU32 stat;

  GR_BEGIN_NOFIFOCHECK( "grSstStatus", 85 );

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)  
  P6_NUDGE_OF_LOVE;
#endif

  stat = initStatus();
  GDBG_INFO((84,"grSstStatus() => 0x%x\n", stat));
  GR_END();

  return stat;
}/* grSstStatus */

/*---------------------------------------------------------------------------
**  grSstVideoLine - return current video line number
*/

GR_ENTRY(grSstVideoLine, FxU32, (void))
{
  FxU32 vline = 1;
#if ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  GR_DCL_GC;
  GR_DCL_HW;

  vline = GR_GET(hw->vRetrace);
  GDBG_INFO((84,"grSstVideoLine() => 0x%x (%d)\n",vline,vline));
#endif
  return vline;

}/* grSstVideoLine */

/*---------------------------------------------------------------------------
**  grSstVRetrace - return contents of SST_VRETRACE bit of status register;
*/

GR_ENTRY(grSstVRetraceOn, FxBool, (void))
{
  if (_grSstStatus() & SST_VRETRACE)
    return FXFALSE;
  else
    return FXTRUE;
}/* grSstVRetrace */

/*---------------------------------------------------------------------------
**  grSstIsBusy - find out if the SST is busy or not
*/

GR_ENTRY(grSstIsBusy, FxBool, (void))
{
  FxBool busy;
  GR_BEGIN_NOFIFOCHECK( "grSstIsBusy", 85 );

  if (gc->nopCMD == FXFALSE) {
    GR_SET_EXPECTED_SIZE(sizeof(FxU32));
    P6FENCE_CMD( GR_SET(hw->nopCMD,1) );
    gc->nopCMD = FXTRUE;
  }
  if ((busy = initBusy()) == FXFALSE)
    gc->nopCMD = FXFALSE;

  GDBG_INFO((84,"grSstIsBusy() => 0x%x\n",busy));

  GR_RETURN( busy );

}/* grSstIsBusy */

/*---------------------------------------------------------------------------
**  guGammaCorrectionRGB - set the gamma correction value
*/

GR_ENTRY(guGammaCorrectionRGB, void, (float r, float g, float b))
{
  /*
  ** FIXME!!!
  */
  GR_BEGIN_NOFIFOCHECK("guGammaCorrectionRGB",80);
  GDBG_INFO_MORE((gc->myLevel,"(%g,%g,%g)\n",r, g, b));
  if (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type == GR_SSTTYPE_VOODOO)
    initGammaRGB((double)r, (double)g, (double)b);
  else
    initGamma(r);
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
  /*
  ** FIXME!!
  */
  GR_BEGIN_NOFIFOCHECK("guGammaCorrectionRGB",80);
  GDBG_INFO_MORE((gc->myLevel,"(%g,%g,%g)\n",red, green, blue));
  if (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type == GR_SSTTYPE_VOODOO)
    initGammaTable(nentries, red, green, blue);
  GR_END();
#undef FN_NAME
}

/*---------------------------------------------------------------------------
** grSstIdle/grFinish
*/

GR_ENTRY(grFinish, void, (void))
{
  GR_BEGIN_NOFIFOCHECK("grSstIdle",83);
  GDBG_INFO_MORE((gc->myLevel,"()\n"));

#if (GLIDE_PLATFORM & GLIDE_SST_SIM)
  GR_SET(hw->nopCMD,0);         /* need to issue NOP command to clear busy flag */
  while (GET(hw->status) & SST_BUSY)
    ;
#else

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
  P6_NUDGE_OF_LOVE;
#endif

  initIdle();
#endif

  GR_END_SLOPPY();
} /* grSstIdle */

/*-------------------------------------------------------------------
  Function: grFlush
  Date: 09-Jan-98
  Implementor(s): atai
  Description:

  Arguments:

  Return:
  -------------------------------------------------------------------*/
GR_ENTRY(grFlush, void, (void))
{
#define FN_NAME "grFlush"
  GR_BEGIN_NOFIFOCHECK("grFlush", 83);
  GDBG_INFO_MORE((gc->myLevel,"()\n"));

  P6FENCE_CMD( GR_SET(hw->nopCMD,0) );

  GR_END();
#undef FN_NAME
} /* grSstIdle */

/*---------------------------------------------------------------------------
**  grSstOrigin - Set the orgin orientation of the screen.
*/

GR_STATE_ENTRY(grSstOrigin, void, (GrOriginLocation_t origin ))
{
  FxU32 fbzMode;

  GR_BEGIN_NOFIFOCHECK("grSstOrigin",83);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",origin));

  /* Initialize FBZMODE register */
  fbzMode = gc->state.fbi_config.fbzMode;
  if (origin == GR_ORIGIN_LOWER_LEFT)
    fbzMode |= SST_YORIGIN;
  else
    fbzMode &= ~(SST_YORIGIN);

  initOrigin( origin );

  gc->state.fbi_config.fbzMode = fbzMode;
} /* grSstOrigin */

/* GMT: do we really have users for this???
   CHD: No.
   JDT: Huh?  If you're talking about grSstOrigin, you're smoking crack.
        if you are talking about SstConfigPipeline, it is evil and must
        be destroyed. :)
 */
#if 0 /* [dBorca] it is... now! :) */

#include "gsstdef.h"

/*---------------------------------------------------------------------------
** grSstConfigPipeline
*/

GR_ENTRY(grSstConfigPipeline, void, (GrChipID_t chip, GrSstRegister reg, FxU32 value))
{
  GR_BEGIN("grSstConfigPipeline",83,4);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x,0x%x)\n",chip,reg,value));
  PACKER_WORKAROUND;

  switch (chip) {
  case GR_TMU0:
  case GR_TMU1:
  case GR_TMU2:
    hw = SST_TMU(hw,chip);
    /* FALL THRU */
  case GR_FBI:
    GR_SET( ((FxU32 *)hw)[reg], value );
    break;
#ifdef GLIDE_DEBUG
  default:
  {
    char outString[256];

    sprintf(outString, "grSstConfigPipeline: invalid chip ID specified (%d)\n", chip);
    GrErrorCallback(outString, FXTRUE);
  }
#endif
    break;
  }
  PACKER_WORKAROUND;
  GR_END();
} /* grSstConfigPipeline */
#endif
