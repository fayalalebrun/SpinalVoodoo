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
** 11    6/24/99 1:46a Stb_mmcclure
** Modifications to fix PRS 6627. Added hwcUnmapMemory9x for revised 9x
** context switching code.
** 
** 10    6/17/99 3:04p Stb_mmcclure
** Fixed bad macro copy in GR_DEBUG_DCL that broke DEBUG builds.
** 
** 9     6/10/99 12:18p Stb_mmcclure
** Modifications for per-process lost context for glide in Win9x
** environments. RT3 Fix PRS 6249.
** 
** 8     5/24/99 2:48p Jamesb
** Added ptrLostContext to exported cmdTransport struct.
** 
** 7     4/25/99 1:02p Atai
** undo hwcMapBoard mods when glide create surface context
** 
** 6     4/23/99 5:15p Atai
** map board when we create surface context
** 
** 5     4/23/99 3:30a Atai
** missing hwcShareContextData and initialize lostContext
** 
** 4     4/06/99 3:54p Dow
** Alt tab again.
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

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)

/* FixMe: dpc - 22 mar 99
 * This little bit of stupidity is so that we can get the
 * various fifo types that the 2d driver knows about.  
 */
#include "../../minihwc/hwcext.h"

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
      if ( !gc->bInfo->osNT )
        hwcUnmapMemory9x ( gc->bInfo );

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
#undef FN_NAME
} /* grSurfaceReleaseContext */

static FxBool
_grSurfaceAttachFifo(GrGC* gc, GrSurface_t sfc)
{
#define FN_NAME "_grSurfaceAttachFifo"
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
    
    /* Starting from the first command buffer */
    gcFifo->curCommandBuf = 0x00UL;
    gcFifo->numQueuedBuf  = 0x01UL;
    gcFifo->numCommandBuf = (gcFifo->hwcFifoInfo.cmdBuf.size / 
                             gcFifo->hwcFifoInfo.cmdBuf.allocUnit);
    
    /* Set default state */
    initGC(gc);
    assertDefaultState();
    gc->state.shadow.fbzMode = ( SST_ENRECTCLIP | SST_ENZBIAS );
    
    gc->state.shadow.colBufferAddr   = 
      gc->state.shadow.colBufferStride = 
        gc->state.shadow.auxBufferAddr   = 
          gc->state.shadow.auxBufferStride = 0x00UL;

    memcpy(gcFifo->stateBuffer, 
           &gc->state.shadow, 
           sizeof(GrStateBuffer));

    retVal = FXTRUE;
  }

 __errFifoAlloc:
  ;

  return retVal;
#undef FN_NAME
} /* _grSurfaceAttachFifo */

static FxBool
_grGetSurfaceInfo(GrGC* gc, 
                  GrSurface_t sfc, 
                  hwcSurfaceInfo* sfcInfo)
{
  FxBool
    surfInfoP;
  
  GR_WINFIFO_BEGIN();
  {
    surfInfoP = hwcGetSurfaceInfo(gc->bInfo, (FxU32*)sfc, sfcInfo);
    _grSurfaceAttachFifo(gc, sfc);
  }
  GR_WINFIFO_END();
  
  return surfInfoP;
}

/*-------------------------------------------------------------------
  Function: grSurfaceSetRenderingSurface
  Date: 09-Jul-98
  Implementor(s): dow
  Description:
  Attaches a rendering surface to the context.  

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grSurfaceSetRenderingSurface, void , (GrSurface_t sfc) )
{
#define FN_NAME "grSurfaceSetRenderingSurface"
  DWORD
    _colBufferAddr = 0x00UL,
    _colBufferStride = 0x00UL;
  hwcSurfaceInfo
    sfcInfo;
  
  GR_DCL_GC;
  GR_DCL_HW;
  GR_DEBUG_DCL(FN_NAME, 80);
  FXUNUSED(hw);
  if (!gc)
    return;
  if (gc->lostContext) {
    *gc->lostContext = FALSE;
  }
  
  /*GR_BEGIN_NOFIFOCHECK(FN_NAME, 80);*/
  GDBG_INFO_MORE(gc->myLevel, "(0x%X)\n", sfc);
  
  if (!gc->windowed) return;
  if (sfc == 0x00UL) goto __errNullSurface;
  if (!_grGetSurfaceInfo(gc, sfc, &sfcInfo)) {
    GDBG_INFO(80, "%s: Could not get surface info.\n", FN_NAME);
    GDBG_INFO(80, "\t%s\n", hwcGetErrorString());
    return;
  }
  
  GDBG_INFO(80, "%s:  sfcInfo:\n", FN_NAME);
  GDBG_INFO(80, "       tileBase:       0x%x\n", sfcInfo.tileBase);
  GDBG_INFO(80, "       lpLFB:          0x%x\n", sfcInfo.lpLFB);
  GDBG_INFO(80, "       lpSurface:      0x%x\n", sfcInfo.lpSurface);
  GDBG_INFO(80, "       pciStride:      0x%x\n", sfcInfo.pciStride);
  GDBG_INFO(80, "       hwStride:       0x%x\n", sfcInfo.hwStride);
  GDBG_INFO(80, "       isTiled:        0x%x\n", sfcInfo.isTiled);  
  GDBG_INFO(80, "       width:          0x%x\n", sfcInfo.width);
  GDBG_INFO(80, "       height:         0x%x\n", sfcInfo.height);
  GDBG_INFO(80, "       fbOffset:       0x%x\n", sfcInfo.fbOffset);

  if ( !sfcInfo.isTiled ) {
    /* NB: Banshee (and derivatives) have a 16-byte alignment
     * restriction on the stride and offset for color/aux buffers, but
     * it is left to the client to correctly adjust for this when
     * setting the values.  
     */
    GR_ASSERT((sfcInfo.fbOffset & 0x0FUL) == 0x00UL);
    GR_ASSERT((sfcInfo.fbStride & 0x0FUL) == 0x00UL);
  }

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

  gc->state.screen_width = sfcInfo.width;
  gc->state.screen_height = sfcInfo.height;

  grClipWindow(0, 0, 
               gc->state.screen_width, 
               gc->state.screen_height);
  grViewport(0, 0, 
             gc->state.screen_width, 
             gc->state.screen_height);

 __errNullSurface:
  GDBG_INFO(80, "%s:  _colBufferAddr = 0x%x\n", FN_NAME, _colBufferAddr);
  GDBG_INFO(80, "%s:  _colBufferStride = 0x%x\n", FN_NAME, _colBufferStride);

  gc->grColBuf = ((sfc != 0x00UL) << 0x01UL);
  gc->curSurface = sfc;

  gc->buffers[0] = gc->buffers[1] = _colBufferAddr;
  gc->state.shadow.colBufferAddr = _colBufferAddr;
  gc->state.shadow.colBufferStride = _colBufferStride;

  GR_END();
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
  DWORD
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

  GDBG_INFO(80, "%s:  sfcInfo:\n", FN_NAME);
  GDBG_INFO(80, "       tileBase:       0x%x\n", sfcInfo.tileBase);
  GDBG_INFO(80, "       lpLFB:          0x%x\n", sfcInfo.lpLFB);
  GDBG_INFO(80, "       lpSurface:      0x%x\n", sfcInfo.lpSurface);
  GDBG_INFO(80, "       pciStride:      0x%x\n", sfcInfo.pciStride);
  GDBG_INFO(80, "       hwStride:       0x%x\n", sfcInfo.hwStride);
  GDBG_INFO(80, "       isTiled:        0x%x\n", sfcInfo.isTiled);  
  GDBG_INFO(80, "       width:          0x%x\n", sfcInfo.width);
  GDBG_INFO(80, "       height:         0x%x\n", sfcInfo.height);

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

 __errNullSurface:
  GDBG_INFO(80, "%s:  _auxBufferAddr = 0x%x\n", FN_NAME, _auxBufferAddr);
  GDBG_INFO(80, "%s:  _auxBufferStride = 0x%x\n", FN_NAME, _auxBufferStride);

  gc->auxSurface = sfc;
  gc->grAuxBuf = (sfc != 0x00UL);
  
  /* NB: If the surface is not valid then the stride will be 'cleared'
   * which should be enough to prevent rendering conflicts w/ other
   * surfaces/apps.  
   */
  gc->buffers[2] = _auxBufferAddr;
  gc->state.shadow.auxBufferAddr = _auxBufferAddr;
  gc->state.shadow.auxBufferStride = _auxBufferStride;

  GR_END();
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
      if (!_grGetSurfaceInfo(gc, sfc, &sfcInfo)) {
        GDBG_INFO(80, "%s: Could not get surface info.\n", FN_NAME);
        GDBG_INFO(80, "\t%s\n", hwcGetErrorString());
        goto __errExit;
      }

      GDBG_INFO(80, "%s:  sfcInfo:\n", FN_NAME);
      GDBG_INFO(80, "       tileBase:       0x%x\n", sfcInfo.tileBase);
      GDBG_INFO(80, "       lpLFB:          0x%x\n", sfcInfo.lpLFB);
      GDBG_INFO(80, "       lpSurface:      0x%x\n", sfcInfo.lpSurface);
      GDBG_INFO(80, "       pciStride:      0x%x\n", sfcInfo.pciStride);
      GDBG_INFO(80, "       hwStride:       0x%x\n", sfcInfo.hwStride);
      GDBG_INFO(80, "       isTiled:        0x%x\n", sfcInfo.isTiled);  
      GDBG_INFO(80, "       width:          0x%x\n", sfcInfo.width);
      GDBG_INFO(80, "       height:         0x%x\n", sfcInfo.height);
      GDBG_INFO(80, "       depth:          0x%x\n", sfcInfo.depth);

      memInfo->texTiled = sfcInfo.isTiled;
      if (memInfo->texTiled) {
        memInfo->texStrideTiles = sfcInfo.hwStride;
        memInfo->texStrideBytes = sfcInfo.pciStride;
      }

      memInfo->tramLfbAddr = (FxU32)sfcInfo.lpSurface - (FxU32)sfcInfo.lpLFB;
      memInfo->tramOffset = sfcInfo.fbOffset;
      memInfo->tramSize = sfcInfo.width * sfcInfo.height * sfcInfo.depth;

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

  *d = _grTexBytesPerTexel(tInfo->format);
  retVal = (*d != 0);
  if (retVal) {
    /* These tables are indexed [largeLodLog2][smallLodLog2], and are
     * in units of texels because we pass back the texel depth.
     */
    static const FxU32
      widthTable[9][9] = {
        {  16,   0,   0,   0,   0,   0,   0,   0,   0 }, /* GR_LOD_LOG2_1 */
        {  16,  16,   0,   0,   0,   0,   0,   0,   0 }, /* GR_LOD_LOG2_2 */
        {  16,  16,  16,   0,   0,   0,   0,   0,   0 }, /* GR_LOD_LOG2_4 */
        {  16,  16,  16,  16,   0,   0,   0,   0,   0 }, /* GR_LOD_LOG2_8 */
        {  16,  16,  16,  16,  16,   0,   0,   0,   0 }, /* GR_LOD_LOG2_16 */
        {  48,  48,  48,  48,  48,  32,   0,   0,   0 }, /* GR_LOD_LOG2_32 */
        { 112, 112, 112, 112, 112,  96,  64,   0,   0 }, /* GR_LOD_LOG2_64 */ 
        { 128, 128, 128, 128, 128, 128, 128, 128,   0 }, /* GR_LOD_LOG2_128 */
        { 384, 384, 384, 384, 384, 384, 384, 384, 256 }, /* GR_LOD_LOG2_256 */
      };
    static const FxU32
      heightTable[9][9] = {
        {   1,   0,   0,   0,   0,   0,   0,   0,   0 }, /* GR_LOD_LOG2_1 */
        {   3,   2,   0,   0,   0,   0,   0,   0,   0 }, /* GR_LOD_LOG2_2 */
        {   7,   6,   4,   0,   0,   0,   0,   0,   0 }, /* GR_LOD_LOG2_4 */
        {  15,  14,  12,   8,   0,   0,   0,   0,   0 }, /* GR_LOD_LOG2_8 */
        {  31,  30,  28,  24,  16,   0,   0,   0,   0 }, /* GR_LOD_LOG2_16 */
        {  32,  32,  32,  32,  32,  32,   0,   0,   0 }, /* GR_LOD_LOG2_32 */
        {  64,  64,  64,  64,  64,  64,  64,   0,   0 }, /* GR_LOD_LOG2_64 */
        { 192, 192, 192, 192, 192, 192, 192, 128,   0 }, /* GR_LOD_LOG2_128 */
        { 256, 256, 256, 256, 256, 256, 256, 256, 256 }, /* GR_LOD_LOG2_256 */
      };
    FxU32
      internalWidth = widthTable[tInfo->largeLodLog2][tInfo->smallLodLog2],
      internalHeight = heightTable[tInfo->largeLodLog2][tInfo->smallLodLog2];

    if (tInfo->aspectRatioLog2 > GR_ASPECT_LOG2_1x1) {
      internalHeight >>= tInfo->aspectRatioLog2;
    } else if (tInfo->aspectRatioLog2 < GR_ASPECT_LOG2_1x1) {
      internalWidth >>= -tInfo->aspectRatioLog2;
    }

    if (internalWidth < 0x01UL) internalWidth = 0x01UL;
    if (internalHeight < 0x01UL) internalHeight = 0x01UL;

    *w = internalWidth;
    *h = internalHeight;
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
  FxU32
    retVal;

  /* Pass over the actual device list rather than the current
   * device. This does not need to be protected since we're not
   * claiming any resources here.  
   */
  for(retVal = 0; retVal < (FxU32)_GlideRoot.hwConfig.num_sst; retVal++) {
    if ((devList != NULL) && (retVal < (FxU32)listCount)) {
      devList[retVal].glideDeviceId  = retVal;
      devList[retVal].systemDeviceId = _GlideRoot.GCs[retVal].bInfo->hMon;
    }
  }

  return retVal;
#undef FN_NAME
} /* grDeviceQuery */

#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */
