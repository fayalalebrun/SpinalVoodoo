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
** $Log: 
**  20   3dfx      1.17.1.0.1.010/11/00 Brent           Forced check in to enforce
**       branching.
**  19   3dfx      1.17.1.0    06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  18   3dfx      1.17        04/25/00 Adam Briggs     Ack: don't clear the lost
**       context dword in set rendering surface--it is cleared when the context is
**       allocated.
**  17   3dfx      1.16        03/31/00 Adam Briggs     Agh! don't EXIT when we
**       can't get info on a surface! just set the lost context dword.
**  16   3dfx      1.15        03/31/00 Kenneth Dyke    Make sure gc->chipCount is
**       set to 1.
**  15   3dfx      1.14        03/24/00 Kenneth Dyke    Make sure gc->chipmask has
**       all chips enabled.
**  14   3dfx      1.13        03/08/00 Kenneth Dyke    Don't cache hardware
**       register pointers.
**  13   3dfx      1.12        03/08/00 Kenneth Dyke    New use isMapped boardInfo
**       flag instead of broken gc flag.
**  12   3dfx      1.11        03/07/00 Don Mullis      Generalize glide3 to handle
**       sub-byte-size texels.
** 
**  11   3dfx      1.10        01/31/00 Adam Briggs     changed the IS_NAPALM macro
**       to cooperate with the display driver version of the same
**  10   3dfx      1.9         01/31/00 Adam Briggs     Changed all device ID magic
**       numbers to use those defined in fxhal.h & added IS_NAPALM macro to test
**       against device ID range
**  9    3dfx      1.8         01/28/00 Kenneth Dyke    Totoally revamped TMU
**       register update mechanism to make 2PPC modes work right regardless of the
**       order of Glide calls.   Also fixed a few register config bugs found when
**       switching between new and old style combine modes.
**  8    3dfx      1.7         01/27/00 Adam Briggs     if one variable is good,
**       two variables are better (as long as they are both initialized)
**  7    3dfx      1.6         11/10/99 Adam Briggs     Made grBufferClear(Ext)
**       cooperate with linear surfaces & texture buffers
**  6    3dfx      1.5         10/29/99 Adam Briggs     set h3pixelSize in
**       grSurfaceSetRenderingSurface instead of _grGetSurfaceInfo (to prevent
**       problems w/ 16bit textures while 32bit rendering)
**  5    3dfx      1.4         10/08/99 Anthony tai     prevent v3 doing
**       _grChipMask
**  4    3dfx      1.3         10/07/99 Larry  warner   Fix
**       grSurfaceCalcTextureWHD() so that it supports 32 and 4 bit texture
**       formats.
**  3    3dfx      1.2         09/17/99 Anthony tai     fixed h3 build
**  2    3dfx      1.1         09/16/99 Anthony tai     added 32 bpp rendermode
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 53    8/23/99 3:14p Kcd
** New MacOS 8 surface stuff.
** 
** 52    8/20/99 4:56p Atai
** fixed packet4 register bit mask for window glide
** 
** 51    8/18/99 4:30p Martin
** Commented out a small bit of broken code.
** 
** 50    8/17/99 5:10p Kcd
** Updated Mac surface stuff to newer API.
** 
** 49    8/16/99 11:18a Adamb
** Merged in V3_OEM_100 fixes
** 
** 48    7/29/99 7:07p Larryw
** Pave the way for FXT1 (but not quite there yet).
** 
** 47    7/22/99 8:14p Larryw
** Texture format byte-depth improvements
** 
** 46    6/24/99 7:10p Atai
** new grSurfaceSetRenderingSurface interface and use 2 buffers per chip
** 
** 45    5/24/99 2:49p Jamesb
** Added ptrLostContext field to exported command transport struct.
** 
** 44    4/25/99 1:07p Atai
** undo hwcMapBoard mods when glide create surface context
** 
** 43    4/23/99 5:09p Atai
** map board when we create surface context
** 
** 42    4/16/99 2:56p Kcd
** MacOS Surface Extensions (temporary).
** 
** 41    4/10/99 1:24p Atai
** fixed code to compile in packet fifo mode 
** 
** 40    4/09/99 4:22p Dow
** Fixes for lost surfaces
** 
** 39    3/22/99 9:12a Peter
** cleanup include paths
** 
** 38    3/14/99 1:46p Peter
** temp implemntation of my surface extension extension
** 
** 37    3/11/99 6:42p Dow
** Resolution help
** 
** 36    3/05/99 10:38p Peter
** fixed the surface fifo state race condition (thanks to Ken Dyke)
** 
** 35    2/18/99 5:55p Peter
** testing glide_platform has to be after glidesys.h has been included
** 
** 34    2/18/99 3:45p Kcd
** Disabled Surface Extensions for non-Win32 platforms.
** 
** 33    2/02/99 4:41p Peter
** computation of passed/whd texture surface size, initial/cleanup state
** cleanup, null rendering fix
** 
** 32    1/25/99 6:35p Peter
** tiled texture cleanup, don't flush if nothing was written on the
** context release
** 
** 31    1/14/99 7:48p Peter
** grSurfaceCaclTextureWHD
** 
** 30    1/06/99 11:30a Peter
** cleanup win fifo locking
** 
** 29    1/04/99 12:03p Peter
** use window contexts for sending state
** 
** 28    12/23/98 2:01p Peter
** nt currently has mutexing problems via ddraw and extescape
** 
** 27    12/21/98 3:01p Peter
** cleaned up shutdow semantics
** 
** 26    12/14/98 6:19p Dow
** Fixed for current surface extension spec
** 
** 25    12/11/98 1:36p Peter
** flush and free fifo when releasing
** 
** 24    12/09/98 11:50a Jeske
** fix for static compile... note that static building glide3 still dosn't
** work..
** 
** 23    12/05/98 2:50p Dow
** Added one more case for attach
** 
** 22    11/30/98 6:57p Peter
** video memory fifo's
** 
** 21    11/17/98 7:06p Atai
** get num tmu from _GlideRoot
** 
** 20    11/15/98 3:21a Atai
** first attempt to make 2 tmus work in H4 glide3x full screen mode, just
** in time check-in for comdex demo. warning: the code is not completed
** yet.
** 
** 19    11/09/98 5:07p Dow
** Texturing from tiled rendered surfaces works now
** 
** 18    10/30/98 3:45p Dow
** Fixed Tiled/Linear color/aux bug
** 
** 17    10/20/98 8:28p Peter
** shared files and fixed whackage
** 
** 16    8/30/98 9:20p Wheeler
** Set # color bufs to 2 in windowed mode (makes depth clears work).
** 
** 15    8/30/98 9:54a Jdt
** Remove spurious buffer clear from grSurfaceSetRenderingSurface
** 
** 14    8/29/98 4:55p Jdt
** Fixed implementation of grSurfaceReleaseSurface
** 
** 13    8/06/98 7:50p Dow
** BInfo stuff for SDRAM detection
** 
** 12    8/03/98 6:40a Jdt
** multi-thread changes
** 
** 11    8/02/98 5:01p Dow
** Glide Surface Extension
** 
** 10    7/21/98 10:53a Jdt
** Added retail-build error checking for grSurfaceSetTextureSurface
** 
** 9     7/18/98 1:45p Jdt
** Removed TACO_MEMORY_FIFO_HACK
** 
** 8     7/18/98 10:40a Dow
** TMU Mem
** 
** 7     7/18/98 12:28a Jdt
** Fixes for both tiled and linear surface address computation.
** 
** 6     7/16/98 10:26p Dow
** moved some stuff down to minihwc & added aux & tex
** 
** 5     7/16/98 8:18a Jdt
** fxcmd.h fixed bug in gc heap allocator
** 
** 4     7/14/98 10:14a Dow
** indentation
** 
** 3     7/13/98 9:57p Jdt
** Added to build.  First implementation of SetRenderSurface and
** CreateContext
** 
** 2     7/09/98 6:47p Dow
** 
** 1     7/09/98 11:37a Dow
** Initial Checkin
**
*/

/* Surface extension currently only really works on Windows anyway */
#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"
#include "fxcmd.h"
#include "rcver.h"
#include "gsfc.h"

#if (GLIDE_PLATFORM & (GLIDE_OS_WIN32|GLIDE_OS_MACOS))

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)

/* FixMe: dpc - 22 mar 99
 * This little bit of stupidity is so that we can get the
 * various fifo types that the 2d driver knows about.  
 */
#include "../../minihwc/hwcext.h"

#elif (GLIDE_PLATFORM & GLIDE_OS_MACOS)

#include <GraphicsPrivHwc.h>
#include <mac_sfc.h>
#endif

/* 
**  This array is us still twisting around the knives to not allocate
**  memory.  Surfaces are stored in here if they are presented to
**  grSurfaceSetRenderingSurface() with the argument that they are not
**  going to be used for texturing surfaces.  If they are later used
**  for that purpose, we hock up an error.
*/

#define GSFC_MAX_RENDER_SURFACES        0x400
static FxU32 renderSurfaces[GSFC_MAX_RENDER_SURFACES];


/*-------------------------------------------------------------------
  Function: grSurfaceCreateContext
  Date: 09-Jul-98
  Implementor(s): dow, taco
  Description:

  Create a Glide State Vector and Bind it to a ContextID which 
  is passed back to the application.  A glide state vector consists
  of all persistent hardware state except for the contents of 
  client surfaces ( aux, color, texture ).
  
  Arguments:

  type - surface type - fullscreen or windowed.  Windowed is currently
         the only valid argument
  
  Return:
  
  An integer naming the new state vector.  The context name is used
  for grSelectContext()
  -------------------------------------------------------------------*/
/* NOTE: THIS FUNCTION CAN BE CALLED WHEN THERE IS NO VALID GC */        
GR_EXT_ENTRY(grSurfaceCreateContext, GrContext_t, ( GrSurfaceContextType_t type ) ) 
{
#define FN_NAME "grSurfaceCreateContext"
#ifdef GLIDE_INIT_HWC
  GrContext_t rv;
  GrGC *ctx;
  int i;
  GDBG_INFO( 80, "%s(0x%X)\n", FN_NAME, type);
  
  rv = 0;
  
  /* validate */
  if ( type != GR_SURFACECONTEXT_WINDOWED ) {
    GDBG_INFO( 80, "GR_SURFACECONTEXT_FULLSCREEN not yet implemented\n" );
  } else {
    /* Search for a free context */
    /* XXX Taco - BEGIN OS SEMAPHORE LOCK */
    beginCriticalSection();
    
    for( i = 0; i < MAX_NUM_CONTEXTS; i++ ) {
      if ( _GlideRoot.surfaceGCHeap[i] == 0 ) {
        const FxU32
          hwcContextId = hwcAllocWinContext(_GlideRoot.GCs[_GlideRoot.current_sst].bInfo);

        if (hwcContextId == 0x00UL) continue;

        ctx = &_GlideRoot.surfaceGCs[i];
        _GlideRoot.surfaceGCHeap[i] = ctx;

        ctx->num_tmu = _GlideRoot.GCs[_GlideRoot.current_sst].num_tmu;
        ctx->bInfo = _GlideRoot.GCs[_GlideRoot.current_sst].bInfo;
        ctx->chipCount = 1; /* Can only use one chip in windowed mode. */
        /* Make sure board has been mapped! */
        if(!ctx->bInfo->isMapped) {
          if (!hwcMapBoard(ctx->bInfo, HWC_BASE_ADDR_MASK)) {
              GrErrorCallback( FN_NAME": Failed to re-map the hw.", FXFALSE );
              return 0;
          }
          if (!hwcInitRegisters(ctx->bInfo)) {
              GrErrorCallback( FN_NAME": Failed to re-initialize the hw.", FXFALSE );
              return 0;
          }
        }

        /* Don't assume that because the board was mapped that these didn't change! */
        ctx->sstRegs = (SstRegs*)ctx->bInfo->regInfo.sstBase;
        ctx->ioRegs  = (SstIORegs*)ctx->bInfo->regInfo.ioMemBase;
        ctx->cRegs   = (SstCRegs*)ctx->bInfo->regInfo.cmdAGPBase;
        ctx->lfb_ptr = (FxU32*)ctx->bInfo->regInfo.lfbBase;
        ctx->rawLfb  = (FxU32*)ctx->bInfo->regInfo.rawLfbBase;
        ctx->tex_ptr = (FxU32*)SST_TEX_ADDRESS(ctx->bInfo->regInfo.sstBase);

        ctx->winContextId = hwcContextId;

        /* NULL out context fifo ptr info which gets set on the first
         * grSurfaceSetXXXSurface call. 
         */
        memset(&ctx->cmdTransportInfo, 0, sizeof(ctx->cmdTransportInfo));

        ctx->windowed = FXTRUE;
        rv = (GrContext_t)ctx;

        hwcShareContextData(ctx->bInfo,
                            &(ctx->lostContext));
        ctx->cmdTransportInfo.ptrLostContext = &(ctx->lostContext);

        *ctx->lostContext = FXFALSE;

        setThreadValue(rv);
        break;
      }
    }
    /* XXX Taco - END OS SEMAPHORE LOCK */
    endCriticalSection();
  }

  GDBG_INFO(80, "%s() => 0x%x---------------------\n", FN_NAME, rv );
  return rv;
#else /* !GLIDE_INIT_HWC */
  return 0;
#endif
#undef FN_NAME
} /* grSurfaceCreateContext */


/*-------------------------------------------------------------------
  Function: grSurfaceReleaseContext
  Date: 09-Jul-98
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/

GR_EXT_ENTRY(grSurfaceReleaseContext, void , (GrContext_t ctx) )
{
#define FN_NAME "grSurfaceReleaseContext"
#ifdef GLIDE_INIT_HWC
  int i;

  GDBG_INFO( 80, "%s(0x%X)\n", FN_NAME, ctx);

  /* XXX Taco - BEGIN OS SEMAPHORE LOCK */
  beginCriticalSection();
  for( i = 0; i < MAX_NUM_CONTEXTS; i++ ) {
    if (ctx == (GrContext_t)(_GlideRoot.surfaceGCs + i)) {
      GrGC* gc = (GrGC*)ctx;
      const struct cmdTransportInfo*
        fifoInfo = &gc->cmdTransportInfo;
      
      /* MDM-Richardson - OpenGL does not call winclose so we will
       * have to deal with the context close here.  
       */
      /* 
       * But on Win98 unmapping the memory doesn't do anything, and
       * we may have multiple context's open on the same board!  If Win98
       * actually worked right, this would break every other open
       * context by yanking out the hardware mapping!  %%KCD
       */
#if (GLIDE_OS & GLIDE_OS_WIN32)
      if ( hwcIsOSWin9x() )
        hwcUnmapMemory9x ( gc->bInfo );
#endif

      /* Free any windowed fifo associated w/ the context */
      if (fifoInfo->fifoPtr != NULL) {
        hwcUnlockWinFifo(gc->bInfo, &gc->cmdTransportInfo.hwcFifoInfo);
        hwcFreeWinFifo(gc->bInfo, &gc->cmdTransportInfo.hwcFifoInfo);
      }

      /* Clear the current context info, including selection */
      hwcFreeWinContext(gc->bInfo, gc->winContextId);
      setThreadValue(0x00UL);
      _GlideRoot.surfaceGCHeap[i] = 0;

      gc->open = FXFALSE;
      gc->windowed = FXFALSE;

      gc->curSurface = NULL;
      gc->state.shadow.colBufferAddr =
        gc->state.shadow.colBufferStride = 0x00UL;

      gc->auxSurface = NULL;
      gc->state.shadow.auxBufferAddr = 
        gc->state.shadow.auxBufferStride = 0x00UL;

      for(i = 0; i < GLIDE_NUM_TMU; i++) {
        struct GrTmuMemInfo*
          memInfo = gc->tmuMemInfo + i;

        gc->texSurface[i] = NULL;

        memInfo->tramLfbAddr =
          memInfo->tramOffset =
            memInfo->tramSize = 0x00UL;
      }

      break;
    }
  }
  /* XXX Taco - END OS SEMAPHORE LOCK */
  endCriticalSection();

  /* This means an invalid context was passed */
  GR_ASSERT(i != MAX_NUM_CONTEXTS);
#endif
#undef FN_NAME
} /* grSurfaceReleaseContext */

static FxBool
_grSurfaceAttachFifo(GrGC* gc, GrSurface_t sfc)
{
#define FN_NAME "_grSurfaceAttachFifo"
#ifdef GLIDE_INIT_HWC
  struct cmdTransportInfo* 
    gcFifo = &gc->cmdTransportInfo;
  FxBool 
    retVal = FXFALSE;

  GDBG_INFO(80, FN_NAME "(%x, %x)\n", gc, sfc);
  
  if (gcFifo->hwcFifoInfo.fifoType == HWCEXT_FIFO_INVALID) {
    /* Fallback fifo setup. */
    gcFifo->hwcFifoInfo.cmdBuf.baseAddr  = (FxU32)&gcFifo->windowedFifo;
    gcFifo->hwcFifoInfo.cmdBuf.allocUnit = sizeof(gcFifo->windowedFifo);
    
    gcFifo->hwcFifoInfo.stateBuf.baseAddr  = (FxU32)&gcFifo->windowedState;
    gcFifo->hwcFifoInfo.stateBuf.allocUnit = sizeof(gcFifo->windowedState);

    GR_WINFIFO_BEGIN();
    {
      retVal = hwcAllocWinFifo(gc->bInfo,
                               &gcFifo->hwcFifoInfo,
                               sfc);
      if (retVal) hwcLockWinFifo(gc->bInfo, &gc->cmdTransportInfo.hwcFifoInfo); 
    }
    GR_WINFIFO_END();
    if (!retVal) goto __errFifoAlloc;
    
    /* Intial fifo state stuff */
    gcFifo->fifoStart = (FxU32*)gcFifo->hwcFifoInfo.cmdBuf.baseAddr;
    gcFifo->fifoSize  = gc->cmdTransportInfo.hwcFifoInfo.cmdBuf.size;
    gcFifo->fifoEnd   = (FxU32*)((FxU8*)gcFifo->fifoStart + gcFifo->fifoSize);
    
    /* Set the pointer for the state buffer */
    gcFifo->stateBuffer = (GrStateBuffer*)gc->cmdTransportInfo.hwcFifoInfo.stateBuf.baseAddr;
    
    /* Checking frequencies are based on the allocation unit of the fifo */
    gcFifo->fifoPtr  = gcFifo->fifoStart;
    gcFifo->fifoRoom = gcFifo->hwcFifoInfo.cmdBuf.allocUnit - FIFO_END_ADJUST;
    
    /* Make sure we don't ever do bump & grind stuff with surface FIFOs. */
    gcFifo->autoBump = FXTRUE;

    /* Starting from the first command buffer */
    gcFifo->curCommandBuf = 0x00UL;
    gcFifo->numQueuedBuf  = 0x01UL;
    gcFifo->numCommandBuf = (gcFifo->hwcFifoInfo.cmdBuf.size / 
                             gcFifo->hwcFifoInfo.cmdBuf.allocUnit);
    
    /* Set default state */
#ifdef FX_GLIDE_NAPALM
    if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
      _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
      gc->chipmask = SST_CHIP_MASK_ALL_CHIPS;
    }
#endif
    initGC(gc);
    assertDefaultState();

#ifdef FX_GLIDE_NAPALM
    if (gc->bInfo->h3pixelSize == 4)
      _grRenderMode(GR_PIXFMT_ARGB_8888);
#endif

    gc->state.shadow.fbzMode = ( SST_ENRECTCLIP | SST_ENZBIAS );
    
    gc->state.shadow.colBufferAddr   = 
      gc->state.shadow.colBufferStride = 
        gc->state.shadow.auxBufferAddr   = 
          gc->state.shadow.auxBufferStride = 0x00UL;

    /* For performance reasons, the hardware-visible state buffer must
       be stored in little endian format.  Ask KCD for details. */
#if __POWERPC__
    {
       FxU32 *src, *dst, i;
       src = (FxU32 *)&gc->state.shadow;
       dst = (FxU32 *)gcFifo->stateBuffer;
       for(i = 0; i < (sizeof(GrStateBuffer)/sizeof(FxU32)); i++) {
         SET(*dst++,*src++);
       }
    }
#else /* !__POWERPC__ */
    memcpy(gcFifo->stateBuffer, 
           &gc->state.shadow, 
           sizeof(GrStateBuffer));
#endif

    retVal = FXTRUE;
  }

 __errFifoAlloc:
  ;

  return retVal;
#else
  return FXFALSE;
#endif
#undef FN_NAME
} /* _grSurfaceAttachFifo */

#ifdef GLIDE_INIT_HWC
static FxBool
_grGetSurfaceInfo(GrGC* gc, 
                  GrSurface_t sfc, 
                  hwcSurfaceInfo* sfcInfo)
{
#define FN_NAME "grGetSurfaceInfo"
  FxBool
    surfInfoP;
  
  GR_WINFIFO_BEGIN();
  {
    surfInfoP = hwcGetSurfaceInfo(gc->bInfo, (FxU32*)sfc, sfcInfo);
    if (!surfInfoP) {
      GDBG_INFO(0, FN_NAME ":  hwcGetSurfaceInfo Failed.\n");
      *gc->lostContext = TRUE ;
    }

    _grSurfaceAttachFifo(gc, sfc);
  }
  GR_WINFIFO_END();
  
  return surfInfoP;
}
#undef FN_NAME
#endif

/*-------------------------------------------------------------------
  Function: grSurfaceSetRenderingSurface
  Date: 09-Jul-98
  Implementor(s): dow
  Description:
  Attaches a rendering surface to the context.  

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grSurfaceSetRenderingSurface, void , (GrSurface_t sfc,
  FxBool textureP) )
{
#define FN_NAME "grSurfaceSetRenderingSurface"
#ifdef GLIDE_INIT_HWC
  FxU32
    _colBufferAddr = 0x00UL,
    _colBufferStride = 0x00UL;
  hwcSurfaceInfo
    sfcInfo;
  
  GR_DCL_GC;
  GR_DCL_HW;
  GR_DEBUG_DCL(FN_NAME, 180);
  FXUNUSED(hw);
  if (!gc)
    return;
  
  /*GR_BEGIN_NOFIFOCHECK(FN_NAME, 80);*/
  GDBG_INFO_MORE(gc->myLevel, "(0x%X)\n", sfc);
  
  if (!gc->windowed) return;
  if (sfc == 0x00UL) goto __errNullSurface;
  if (!_grGetSurfaceInfo(gc, sfc, &sfcInfo)) {
    GDBG_INFO(80, "%s: Could not get surface info.\n", FN_NAME);
    GDBG_INFO(80, "\t%s\n", hwcGetErrorString());
    return;
  }
  
  GDBG_INFO(180, "%s:  sfcInfo:\n", FN_NAME);
  GDBG_INFO(180, "       tileBase:       0x%x\n", sfcInfo.tileBase);
  GDBG_INFO(180, "       lpLFB:          0x%x\n", sfcInfo.lpLFB);
  GDBG_INFO(180, "       lpSurface:      0x%x\n", sfcInfo.lpSurface);
  GDBG_INFO(180, "       pciStride:      0x%x\n", sfcInfo.pciStride);
  GDBG_INFO(180, "       hwStride:       0x%x\n", sfcInfo.hwStride);
  GDBG_INFO(180, "       isTiled:        0x%x\n", sfcInfo.isTiled);  
  GDBG_INFO(180, "       width:          0x%x\n", sfcInfo.width);
  GDBG_INFO(180, "       height:         0x%x\n", sfcInfo.height);
  GDBG_INFO(180, "       fbOffset:       0x%x\n", sfcInfo.fbOffset);

  gc->grPixelSize = gc->bInfo->h3pixelSize = sfcInfo.bitdepth / 8; 
  gc->grPixelSample = 1 ;

#ifdef FX_GLIDE_NAPALM
    if (gc->bInfo->h3pixelSize == 4)
      _grRenderMode(GR_PIXFMT_ARGB_8888);
#if (GLIDE_OS & GLIDE_OS_MACOS)
    else
      _grRenderMode(GR_PIXFMT_ARGB_1555);
#endif
#endif

#ifdef GDBG_INFO_ON
  hwcCheckTarget(sfcInfo.fbOffset, sfcInfo.width,  sfcInfo.height,
                 sfcInfo.bitdepth, 0x1000000, 0
                 );
#endif

  gc->colTiled = sfcInfo.isTiled ; // AJB- grBufferClear needs to know this

  if ( !sfcInfo.isTiled ) {
    /* NB: Banshee (and derivatives) have a 16-byte alignment
     * restriction on the stride and offset for color/aux buffers, but
     * it is left to the client to correctly adjust for this when
     * setting the values.  
     */
    GR_ASSERT((sfcInfo.fbOffset & 0x0FUL) == 0x00UL);
    GR_ASSERT((sfcInfo.fbStride & 0x0FUL) == 0x00UL);
  }

#if 0
  if (!textureP) {
    int i = 0;
    FxBool slotFound = FXFALSE;

    do {
      if (renderSurfaces[i] == 0) {
        renderSurfaces[i] = (FxU32) sfc;
        slotFound = FXTRUE;
      }
      i++;
    } while (i < GSFC_MAX_RENDER_SURFACES && !slotFound);

    if (!slotFound)
      _doGrErrorCallback("SURFACE", "Max Surfaces exceeded!", FXTRUE);
  }
#endif

  _colBufferAddr = sfcInfo.fbOffset;
  _colBufferStride = sfcInfo.fbStride;
  
  REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3);
  {
    REG_GROUP_SET(hw, colBufferAddr, _colBufferAddr );
    REG_GROUP_SET(hw, colBufferStride, _colBufferStride );
  }
  REG_GROUP_END();
  
  GDBG_INFO(80, "%s:  width  = %d\n", FN_NAME, sfcInfo.width); 
  GDBG_INFO(80, "%s:  height = %d\n", FN_NAME, sfcInfo.height);

  gc->state.wClipping.colClip.width = sfcInfo.width;
  gc->state.wClipping.colClip.height = sfcInfo.height;
  gc->state.wClipping.colBufferSet = FXTRUE;

  /* NB:  This will modify the clipped width */
  grClipWindow(0, 0, sfcInfo.width, sfcInfo.height);

  grViewport(0, 0, 
             gc->state.wClipping.winClip.width, 
             gc->state.wClipping.winClip.height);

  gc->grColBuf = ((sfc != 0x00UL) << 0x01UL);
  gc->curSurface = sfc;

  gc->buffers0[0] = gc->buffers0[1] = _colBufferAddr;
  gc->state.shadow.colBufferAddr = _colBufferAddr;
  gc->state.shadow.colBufferStride = _colBufferStride;

 __errNullSurface:
  if (sfc == 0x00UL)
    gc->state.wClipping.colBufferSet = FXFALSE;
  GDBG_INFO(180, "%s:  _colBufferAddr = 0x%x\n", FN_NAME, _colBufferAddr);
  GDBG_INFO(180, "%s:  _colBufferStride = 0x%x\n", FN_NAME, _colBufferStride);

  GR_END();
#endif
#undef FN_NAME
} /* grSurfaceSetRenderingSurface */

/*-------------------------------------------------------------------
  Function: grSurfaceSetAuxSurface
  Date: 09-Jul-98
  Implementor(s): dow
  Description:
  Attaches an aux buffer surface to the context.  Note that this
  surface is a DirectDraw surface in the windowed case, and a Glide
  surface in the full-screen case. 
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grSurfaceSetAuxSurface, void , (GrSurface_t sfc) )
{
#define FN_NAME "grSurfaceSetAuxSurface"
#ifdef GLIDE_INIT_HWC
  FxU32
    _auxBufferAddr = 0x00UL,
    _auxBufferStride = 0x00UL;
  hwcSurfaceInfo
    sfcInfo;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 80);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", sfc);

  if (!gc->windowed) return;
  if (sfc == 0x00UL) goto __errNullSurface;
  if (!_grGetSurfaceInfo(gc, sfc, &sfcInfo)) {
    GDBG_INFO(80, "%s: Could not get surface info.\n", FN_NAME);
    GDBG_INFO(80, "\t%s\n", hwcGetErrorString());
    return;
  }

  GDBG_INFO(180, "%s:  sfcInfo:\n", FN_NAME);
  GDBG_INFO(180, "       tileBase:       0x%x\n", sfcInfo.tileBase);
  GDBG_INFO(180, "       lpLFB:          0x%x\n", sfcInfo.lpLFB);
  GDBG_INFO(180, "       lpSurface:      0x%x\n", sfcInfo.lpSurface);
  GDBG_INFO(180, "       pciStride:      0x%x\n", sfcInfo.pciStride);
  GDBG_INFO(180, "       hwStride:       0x%x\n", sfcInfo.hwStride);
  GDBG_INFO(180, "       isTiled:        0x%x\n", sfcInfo.isTiled);  
  GDBG_INFO(180, "       width:          0x%x\n", sfcInfo.width);
  GDBG_INFO(180, "       height:         0x%x\n", sfcInfo.height);

#ifdef GDBG_INFO_ON
  hwcCheckTarget(sfcInfo.fbOffset, sfcInfo.width,  sfcInfo.height,
                 sfcInfo.bitdepth, 0x1000000, 0
                 );
#endif
  gc->auxTiled = sfcInfo.isTiled ; // AJB- grBufferClear needs to know this

  if ( !sfcInfo.isTiled ) {
    /* NB: Banshee (and derivatives) have a 16-byte alignment
     * restriction on the stride and offset for color/aux buffers, but
     * it is left to the client to correctly adjust for this when
     * setting the values.  
     */
    GR_ASSERT((sfcInfo.fbOffset & 0x0FUL) == 0x00UL);
    GR_ASSERT((sfcInfo.fbStride & 0x0FUL) == 0x00UL);
  }
  
  _auxBufferAddr = sfcInfo.fbOffset;
  _auxBufferStride = sfcInfo.fbStride;
  
  REG_GROUP_BEGIN(BROADCAST_ID, auxBufferAddr, 2, 0x3);
  REG_GROUP_SET(hw, auxBufferAddr, _auxBufferAddr );
  REG_GROUP_SET(hw, auxBufferStride, _auxBufferStride );
  REG_GROUP_END();
  
  GDBG_INFO(80, "%s:  width  = %d\n", FN_NAME, sfcInfo.width); 
  GDBG_INFO(80, "%s:  height = %d\n", FN_NAME, sfcInfo.height);
  
  gc->state.screen_width = sfcInfo.width;
  gc->state.screen_height = sfcInfo.height;

  gc->state.wClipping.auxClip.width = sfcInfo.width;
  gc->state.wClipping.auxClip.height = sfcInfo.height;
  gc->state.wClipping.auxBufferSet = FXTRUE;

  /* NB:  This will modify the clipped width */
  grClipWindow(0, 0, sfcInfo.width, sfcInfo.height);

  grViewport(0, 0, 
             gc->state.wClipping.auxClip.width, 
             gc->state.wClipping.auxClip.height);

 __errNullSurface:
  if (sfc == 0L)
    gc->state.wClipping.colBufferSet = FXFALSE;

  GDBG_INFO(180, "%s:  _auxBufferAddr = 0x%x\n", FN_NAME, _auxBufferAddr);
  GDBG_INFO(180, "%s:  _auxBufferStride = 0x%x\n", FN_NAME, _auxBufferStride);

  gc->auxSurface = sfc;
  gc->grAuxBuf = (sfc != 0x00UL);
  
  /* NB: If the surface is not valid then the stride will be 'cleared'
   * which should be enough to prevent rendering conflicts w/ other
   * surfaces/apps.  
   */
  gc->buffers0[2] = _auxBufferAddr;
  gc->state.shadow.auxBufferAddr = _auxBufferAddr;
  gc->state.shadow.auxBufferStride = _auxBufferStride;

  GR_END();
#endif
#undef FN_NAME
} /* grSurfaceSetAuxSurface */

/*-------------------------------------------------------------------
  Function: grSurfaceSetTextureSurface
  Date: 09-Jul-98
  Implementor(s): dow
  Description:
  Attaches a texture buffer surface to the context.  Until another
  texture buffer is specified, grTexMinAddress, grTexMaxAddress,
  grTexDownloadMipmap*, and grTexSource act  in relation to the buffer
  specified by grSurfaceTexture. 

  Notes:
  grSurfaceTexture is to be used only with windowed Glide.

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grSurfaceSetTextureSurface, void , (GrChipID_t tmu, GrSurface_t sfc) )
{
#define FN_NAME "grSurfaceSetTextureSurface"
#ifdef GLIDE_INIT_HWC
  hwcSurfaceInfo
    sfcInfo;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 80);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", sfc);

  GR_CHECK_TMU(FN_NAME, tmu);

  {
    struct GrTmuMemInfo*
      memInfo = gc->tmuMemInfo + tmu;

    if (sfc == NULL) goto __errExit;
    if (gc->windowed) {
#if 0
      int i = 0;
      do {
        if (renderSurfaces[i] == (FxU32) sfc)
          _doGrErrorCallback("SURFACE", "Render surface illegally used as texture!", FXTRUE);
        i++;
      } while (i < GSFC_MAX_RENDER_SURFACES);
#endif
      if (!_grGetSurfaceInfo(gc, sfc, &sfcInfo)) {
        GDBG_INFO(80, "%s: Could not get surface info.\n", FN_NAME);
        GDBG_INFO(80, "\t%s\n", hwcGetErrorString());
        goto __errExit;
      }

      GDBG_INFO(180, "%s:  sfcInfo:\n", FN_NAME);
      GDBG_INFO(180, "       tileBase:       0x%x\n", sfcInfo.tileBase);
      GDBG_INFO(180, "       lpLFB:          0x%x\n", sfcInfo.lpLFB);
      GDBG_INFO(180, "       lpSurface:      0x%x\n", sfcInfo.lpSurface);
      GDBG_INFO(180, "       pciStride:      0x%x\n", sfcInfo.pciStride);
      GDBG_INFO(180, "       hwStride:       0x%x\n", sfcInfo.hwStride);
      GDBG_INFO(180, "       isTiled:        0x%x\n", sfcInfo.isTiled);  
      GDBG_INFO(180, "       width:          0x%x\n", sfcInfo.width);
      GDBG_INFO(180, "       height:         0x%x\n", sfcInfo.height);
      GDBG_INFO(180, "       depth:          0x%x\n", sfcInfo.bitdepth);

#ifdef GDBG_INFO_ON
      hwcCheckTarget(sfcInfo.fbOffset, sfcInfo.width,  sfcInfo.height,
                     sfcInfo.bitdepth, 0x1000000, 0
                     );
#endif
      memInfo->texTiled = sfcInfo.isTiled;
      if (memInfo->texTiled) {
        memInfo->texStrideTiles = sfcInfo.hwStride;
        memInfo->texStrideBytes = sfcInfo.pciStride;
      }

      memInfo->tramLfbAddr = (FxU32)sfcInfo.lpSurface - (FxU32)sfcInfo.lpLFB;
      memInfo->tramOffset = sfcInfo.fbOffset;
      memInfo->tramSize = sfcInfo.width * sfcInfo.height * sfcInfo.bitdepth / 8;

      GDBG_INFO(80, FN_NAME ":  tramOffset[%d] = 0x%x\n",
                tmu, memInfo->tramOffset);
      GDBG_INFO(80, FN_NAME ":  tramSize[%d] = 0x%x\n", 
                tmu, memInfo->tramSize);
      GDBG_INFO(80, FN_NAME ":  tramLfbAddr[%d] = 0x%x\n",
                tmu, memInfo->tramLfbAddr);
    } else {
      hwcBufferDesc *bd = (hwcBufferDesc *) sfc;
      memInfo->tramOffset = bd->bufOffset;
      memInfo->tramSize = bd->bufSize;
    }

    if (0) {
  __errExit:
      /* Clear glide texturing */
      memInfo->tramLfbAddr =
        memInfo->tramOffset  =
          memInfo->tramSize    = 0x00UL;
    }

    gc->tmu_state[tmu].total_mem = memInfo->tramSize;
  }

  gc->texSurface[tmu] = sfc;

  GR_END();
#endif
#undef FN_NAME
} /* grSurfaceSetTextureSurface */

/*-------------------------------------------------------------------
  Function: grSurfaceCalcTextureWHD
  Date: 14-Dec-98
  Implementor(s): dow
  Description:
    See the very nice ascii art w/ _grTexCalcMipmapLevelOffsetTiled in
    gtex.c for a description of the memory layout for tiled textures.
    Textures in linear memory take up the same amount of memory so
    thinking of everything as tiled makes the implementation simpler.

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grSurfaceCalcTextureWHD, FxBool , (GrTexInfo *tInfo, FxU32 *w,
                                                FxU32 *h, FxU32 *d) )
{
#define FN_NAME "grSurfaceCalcTextureWHD"
  FxU32 
    offset = 0;
  FxBool
    retVal = FXFALSE;

  GR_CHECK_F(FN_NAME, tInfo->format < 0, "invalid texture format");
  *d = _grBitsPerTexel[tInfo->format];
  GR_CHECK_F(FN_NAME, *d == 0, "invalid texture format");

  retVal = (*d != 0);
  if (retVal) {
    /* gsfctabl.h contains the boundingBoxWH array, which we include here: */
#include "gsfctabl.h"
    FxU32 fmtType, internalWidth, internalHeight;

    switch(tInfo->format) {
    case GR_TEXFMT_ARGB_CMP_FXT1:
      fmtType = 1;
      break;
    case GR_TEXFMT_ARGB_CMP_DXT1:
    case GR_TEXFMT_ARGB_CMP_DXT2:
    case GR_TEXFMT_ARGB_CMP_DXT3:
    case GR_TEXFMT_ARGB_CMP_DXT4:
    case GR_TEXFMT_ARGB_CMP_DXT5:
      fmtType = 2;
      break;
    default:
      fmtType = 0;
      break;
    }

    internalWidth =
      (FxU32)(boundingBoxWH[fmtType]
              [G3_ASPECT_TRANSLATE(tInfo->aspectRatioLog2)]
              [tInfo->largeLodLog2][tInfo->smallLodLog2][0]);

    internalHeight =
      (FxU32)(boundingBoxWH[fmtType]
              [G3_ASPECT_TRANSLATE(tInfo->aspectRatioLog2)]
              [tInfo->largeLodLog2][tInfo->smallLodLog2][1]);

    if (w != NULL) *w = internalWidth;
    if (h != NULL) *h = internalHeight;
  }

  return retVal;
#undef FN_NAME
} /* grSurfaceCalcTextureWHD */

/* Returns the # of glide compatible devices in the system. If devList
 * is non-NULL and listCount is non-zero, devList is filled in w/
 * information about the current glide device to system device
 * mapping. If there is not enough space in the client passed list to
 * fill in all of the device information a partial list is returned to
 * the client along w/ the full count.  
 */
GR_EXT_ENTRY(grDeviceQuery,
             FxU32,
             (GrDeviceInfo_t devList[], FxU32 listCount))
{
#define FN_NAME "grDeviceQuery"
#ifdef GLIDE_INIT_HWC
  FxU32
    retVal;

  /* Pass over the actual device list rather than the current
   * device. This does not need to be protected since we're not
   * claiming any resources here.  
   */
  for(retVal = 0; retVal < (FxU32)_GlideRoot.hwConfig.num_sst; retVal++) {
    if ((devList != NULL) && (retVal < (FxU32)listCount)) {
      devList[retVal].glideDeviceId  = retVal;
#if (GLIDE_PLATFORM & GLIDE_OS_MACOS)
      devList[retVal].systemDeviceId = _GlideRoot.GCs[retVal].bInfo->hdc;
#else
      devList[retVal].systemDeviceId = _GlideRoot.GCs[retVal].bInfo->hMon;
#endif
    }
  }

  return retVal;
#else /* !GLIDE_INIT_HWC */
  return 0;
#endif 
#undef FN_NAME
} /* grDeviceQuery */

#if (GLIDE_PLATFORM & GLIDE_OS_MACOS)
GR_EXT_ENTRY(grSurfaceCreate, GrSurface_t , (GrSurfaceDesc_t *sfcDesc))
{
#define FN_NAME "grSurfaceCreate"
  GR_DCL_GC;
  return gdxSurfaceAlloc(gc->bInfo, gc->bInfo->hMon, sfcDesc);
#undef FN_NAME
}

GR_EXT_ENTRY(grSurfaceRelease, void , (GrSurface_t sfc) )
{
#define FN_NAME "grSurfaceRelease"
  gdxSurfaceFree(sfc);
#undef FN_NAME
}

GR_EXT_ENTRY(grSurfaceGetDesc, void , (GrSurface_t sfc, GrSurfaceDesc_t *sfcDesc))
{
#define FN_NAME "grSurfaceGetDesc"
  gdxSurfaceGetDesc(sfc, sfcDesc);
#undef FN_NAME
}

#endif /* (GLIDE_PLATFORM & GLIDE_OS_MACOS) */
#endif /* (GLIDE_PLATFORM & (GLIDE_OS_WIN32 | GLIDE_OS_MACOS)) */

