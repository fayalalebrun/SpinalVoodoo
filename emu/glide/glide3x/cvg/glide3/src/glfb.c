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
** Revision 1.1.1.1.8.3  2007/05/19 11:06:28  koolsmoky
** win32 build workaround
**
** Revision 1.1.1.1.8.2  2005/08/13 21:06:57  jwrdegoede
** Last needed 64 bit fixes for h5/h3, complete 64 bit support for cvg
**
** Revision 1.1.1.1.8.1  2005/06/09 18:32:08  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.1.1  1999/12/07 21:42:32  joseph
** Initial checkin into SourceForge.
**
** 
** 1     10/08/98 11:30a Brent
** 
** 14    10/07/98 9:42p Peter
** ignore color format in grLfbLock
** 
** 13    9/30/98 2:45p Peter
** fail raw grLfbLock for triple buffer
** 
** 12    9/15/98 5:40p Atai
** fbzMode for pixelPipeline enable
** 
** 11    9/08/98 6:56p Atai
** fix debug info. added underline for internal routines and some \n
** 
** 10    7/24/98 1:41p Hohn
** 
** 9     7/21/98 3:47p Peter
** fixed grLfbLock stuff
** 
** 8     7/21/98 3:14p Peter
** fixed grLfbLock stuff
** 
** 7     4/22/98 4:57p Peter
** glide2x merge
** 
** 6     4/21/98 1:34p Atai
** make 32 bit clean
** 
** 5     2/01/98 7:52p Peter
** grLfbWriteRegion byte count problems
** 
** 4     1/28/98 2:20p Atai
** fixed lfb state validation
** 
** 3     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 2     1/16/98 6:44p Atai
 * fixed for glide 3 build
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 58    1/16/98 4:18p Atai
 * fixed lfb and grLoadGammaTable
 * 
 * 57    1/06/98 3:53p Atai
 * remove grHint, modify grLfbWriteRegion and grGet
 * 
 * 56    12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 55    12/15/97 6:04p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 54    12/15/97 5:54p Peter
 * swizzle reads too
 * 
 * 53    12/11/97 4:15p Peter
 * mac lfb write region
 * 
 * 52    12/01/97 5:18p Peter
 * 
 * 51    11/25/97 12:09p Peter
 * nested calls to grLfbLock vs init code locking on v2
 * 
 * 50    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 49    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 48    11/06/97 3:38p Dow
 * More banshee stuff
 * 
 * 47    11/04/97 5:04p Peter
 * cataclysm part deux
 * 
 * 46    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 45    10/27/97 11:10a Peter
 * starting cleanup
 * 
 * 44    10/09/97 8:02p Dow
 * State Monster 1st Cut
 * 
 * 43    10/08/97 5:18p Peter
 * fixed grLfbLock wrt writemode_any
 * 
 * 42    10/08/97 11:33a Peter
 * hmmmm....
 * 
 * 41    9/30/97 1:03p Peter
 * more debugging code
 * 
 * 40    9/25/97 1:35p Peter
 * more fixes for csim
 * 
 * 39    9/24/97 4:09p Peter
 * lfb/idle fixes
 * 
 * 38    9/24/97 1:31p Peter
 * assert if grXX call inside lock/unlock
 * 
 * 37    9/20/97 10:54a Peter
 * naked lfb writes
 * 
 * 36    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 35    9/10/97 10:13p Peter
 * fifo logic from GaryT, non-normalized fp first cut
 * 
 * 34    9/05/97 5:29p Peter
 * changes for direct hw
 * 
 * 33    9/04/97 3:32p Peter
 * starting grouping serial reg writes
 * 
 * 32    8/18/97 3:52p Peter
 * pre-hw arrival fixes/cleanup
 * 
 * 31    7/30/97 2:42p Peter
 * more cleanup
 * 
 * 30    7/28/97 2:41p Peter
 * turned sli code back on for cvg, but waiting for hal
 * 
 * 29    7/26/97 3:04p Peter
 * gratuitous plug is translucent
 * 
 * 28    7/02/97 12:28p Peter
 * removed spurious NOP, tex dl
 * 
 * 27    6/26/97 10:14a Peter
 * cmd fifo for lfb
 * 
 * 26    6/06/97 10:47a Peter
 * texture downloading, fixed 640x480 dimension, changed cvg dep to be the
 * same as sst1
 * 
**
*/

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

/*---------------------------------------------------------------------------
** grLfbConstantAlpha
*/
GR_ENTRY(grLfbConstantAlpha, void, (GrAlpha_t alpha))
{
  GR_BEGIN_NOFIFOCHECK("grLfbConstantAlpha",82);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",alpha);

  gc->state.lfb_constant_alpha = alpha;
  GR_END();
} /* grLfbConstantAlpha */

/*---------------------------------------------------------------------------
** grLfbConstantDepth
*/
GR_ENTRY(grLfbConstantDepth, void, (FxU32 depth))
{
  GR_BEGIN_NOFIFOCHECK("grLfbConstantDepth",82);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",depth);

  gc->state.lfb_constant_depth = depth;
  GR_END();
} /* grLfbConstantDepth */


/*-------------------------------------------------------------------
  Function: grLfbLock
  Date: 2/19
  Implementor(s): jdt
  Library: Glide
  Description:
  Secures a pointer to a requested frame buffer and guarantees ordered
  access to that buffer.

  Arguments:
  type - type of lock, one of:
  GR_LOCK_WRITE_ONLY
  GR_LOCK_READ_ONLY
  buffer - which buffer to lock, one of:
  GR_BUFFER_FRONTBUFFER
  GR_BUFFER_BACKBUFFER
  GR_BUFFER_AUXBUFFER
  writeMode - desired destination color format
  origin - desired lfb origin
  pixelPipeline - flag whether to process through pixpipe
  info   - pointer to info structure
  Return:
  FXTRUE  - lock is successful
  FXFALSE - lock fails
  -------------------------------------------------------------------*/

GR_ENTRY(grLfbLock, FxBool,(GrLock_t type, GrBuffer_t buffer, 
                            GrLfbWriteMode_t writeMode, GrOriginLocation_t origin, 
                            FxBool pixelPipeline, GrLfbInfo_t *info)) 
{
#define FN_NAME "grLfbLock"
  FxBool rv = FXTRUE;
  FxU32 lfbMode, zaColor = 0, fbzMode = 0;
  const FxBool idleLockP = ((type & GR_LFB_NOIDLE) == 0);
#if (GLIDE_PLATFORM & GLIDE_OS_MACOS) && SET_BSWAP
  FxBool swizzleByteP = FXFALSE;
#endif /* (GLIDE_PLATFORM & GLIDE_OS_MACOS) && SET_BSWAP */
  
  GR_BEGIN_NOFIFOCHECK("grLfbLock", 87);
  GDBG_INFO_MORE(gc->myLevel,"(%d, %d, %d, %d, %d)\n", 
                 type, buffer, writeMode, origin, pixelPipeline);

  GR_CHECK_COMPATABILITY(FN_NAME, !info,
             "Null info structure passed.");
  /* there is only one revision extant */
  GR_CHECK_COMPATABILITY(FN_NAME, info->size != sizeof(GrLfbInfo_t),
                         "uninitialized info structure passed.");

  type = type & ~(GR_LFB_NOIDLE);
#if defined(GLIDE3)
  _grValidateState();
#endif

  switch(type) {
  case GR_LFB_READ_ONLY:
  {
    /* Voodoo2 has a problem reading from the frame buffer if we are
     * triple buffering and the triple buffer is the currently
     * active lfb buffer. The problem is that the 32 pixel screen
     * tiles are reversed such that the logical tiling 0101 comes
     * back as 1010.
     *
     * If this call to grLfbLock is coming through grLfbReadRegion
     * then it knows how to work around this problem, but raw lfb
     * reads from the application won't work correctly so we fail the
     * lock and woe to the application that does not look at the
     * return value.
     */
    rv = !((gc->grColBuf >= 3) &&
           !gc->cmdTransportInfo.fifoLfbP &&
           (((buffer == GR_BUFFER_BACKBUFFER) && (gc->hwDep.cvgDep.backBuf == 2)) ||
            ((buffer == GR_BUFFER_FRONTBUFFER) && (gc->hwDep.cvgDep.frontBuf == 2))));
    if (!rv) {
      GDBG_INFO(83, "Read lock failure due to raw triple buffer bug");
    } else if (gc->lockPtrs[type] != (FxU32)-1) {
      GDBG_INFO(83, "Read lock failure due to existing lock");
      rv = FXFALSE;
    } else {
      lfbMode = gc->state.fbi_config.lfbMode;
      lfbMode &= ~(SST_LFB_READBUFSELECT | 
                   SST_LFB_YORIGIN);
                    
      switch(buffer) {
      case GR_BUFFER_FRONTBUFFER:
        lfbMode |= SST_LFB_READFRONTBUFFER;
        break;

      case GR_BUFFER_BACKBUFFER:
        lfbMode |= SST_LFB_READBACKBUFFER;
        break;

      case GR_BUFFER_AUXBUFFER:
        lfbMode |= SST_LFB_READAUXBUFFER;
        break;

      default:
        GR_CHECK_F(myName, 
                   1, 
                   "illegal buffer parameter passed");
        rv = FXFALSE;
        break;
      }

      lfbMode |= (origin ? SST_LFB_YORIGIN : 0);
      gc->state.fbi_config.lfbMode = lfbMode;

#if (GLIDE_PLATFORM & GLIDE_OS_MACOS) && SET_BSWAP
      if (!gc->cmdTransportInfo.fifoLfbP) lfbMode |= (SST_LFB_READ_SWAP16 |
                                                      SST_LFB_READ_BYTESWAP);
#endif /* (GLIDE_PLATFORM & GLIDE_OS_MACOS) */

      info->strideInBytes = gc->fbStride * 2;
      info->origin        = (origin ? GR_ORIGIN_LOWER_LEFT : GR_ORIGIN_UPPER_LEFT);
      info->lfbPtr        = gc->lfb_ptr;
    }
  }
  break;

  case GR_LFB_WRITE_ONLY:
  {           
    if (gc->lockPtrs[type] != (FxU32)-1) {
      GDBG_INFO(83, "Write failure due to existing lock");
      rv = FXFALSE;
    } else {
      /* Set up the constant depth register because it may have
       * been trashed by a call to grDepthBiasLevel
       * (depthbiaslevel and constant depth use the same register) 
       */
      zaColor = gc->state.fbi_config.zaColor;
      zaColor = (((FxU32) gc->state.lfb_constant_depth) << SST_ZACOLOR_DEPTH_SHIFT);
      zaColor |= (gc->state.lfb_constant_alpha << SST_ZACOLOR_ALPHA_SHIFT);

      /* disable depth biasing */
      fbzMode = gc->state.fbi_config.fbzMode;
      fbzMode &= ~(SST_ENZBIAS);
          
      lfbMode = gc->state.fbi_config.lfbMode;
      lfbMode &= ~(SST_LFB_WRITEBUFSELECT |
                   SST_LFB_YORIGIN        |
                   SST_LFB_FORMAT         |
                   SST_LFB_ENPIXPIPE);
          
      switch(writeMode) {
      case GR_LFBWRITEMODE_RESERVED1:
      case GR_LFBWRITEMODE_RESERVED2:
      case GR_LFBWRITEMODE_RESERVED3:
      case GR_LFBWRITEMODE_RESERVED4:
      case GR_LFBWRITEMODE_RESERVED5:
      case GR_LFBWRITEMODE_RESERVED6:
      case GR_LFBWRITEMODE_RESERVED7:
        rv = FXFALSE;
      }

      /* Default to hw */
      if (writeMode == GR_LFBWRITEMODE_ANY) {
        writeMode = ((buffer == GR_BUFFER_AUXBUFFER)
                     ? GR_LFBWRITEMODE_ZA16
                     : GR_LFBWRITEMODE_565);
      }
          
      switch(buffer) {
      case GR_BUFFER_FRONTBUFFER:
        if (writeMode == GR_LFBWRITEMODE_ZA16) {
          rv = FXFALSE;
        }
        lfbMode |= SST_LFB_WRITEFRONTBUFFER;
        break;

      case GR_BUFFER_BACKBUFFER:
        if (writeMode == GR_LFBWRITEMODE_ZA16) {
          rv = FXFALSE;
        }
        lfbMode |= SST_LFB_WRITEBACKBUFFER;
        break;

      case GR_BUFFER_AUXBUFFER:
        if (writeMode != GR_LFBWRITEMODE_ZA16) {
          rv = FXFALSE;
        }
        lfbMode |= SST_LFB_WRITEFRONTBUFFER;
        break;

      default:
        GR_CHECK_F(myName, 1, "illegal buffer parameter passed");
        break;
      }

      lfbMode |= (writeMode << SST_LFB_FORMAT_SHIFT);
      lfbMode |= (origin ? SST_LFB_YORIGIN : 0);

      if (pixelPipeline) {
        lfbMode |= SST_LFB_ENPIXPIPE;

        fbzMode &= ~SST_YORIGIN;
        fbzMode |= (origin ? SST_YORIGIN : 0);
        fbzMode |= ((buffer == GR_BUFFER_FRONTBUFFER) 
                    ? SST_DRAWBUFFER_FRONT 
                    : SST_DRAWBUFFER_BACK);
      }
      gc->state.fbi_config.lfbMode = lfbMode;
                        
      info->origin    = (origin ? GR_ORIGIN_LOWER_LEFT : GR_ORIGIN_UPPER_LEFT);
      info->writeMode = writeMode;

#if (GLIDE_PLATFORM & GLIDE_OS_MACOS) && SET_BSWAP
      /* Turn on word swizzling by default.
       *
       * NB: This state does not get shadowed because it is only
       * relevant if the app is doing its own lfb writes. When coming
       * through grLfbWriteRegion we take care of doing this when
       * putting the data in the fifo so the actual rendering state
       * in lfbMode is all we care about.
       */
      if (!gc->cmdTransportInfo.fifoLfbP) lfbMode |= SST_LFB_WRITE_SWAP16;
#endif /* (GLIDE_PLATFORM & GLIDE_OS_MACOS) */
                    
      switch(lfbMode & SST_LFB_FORMAT) {
      case SST_LFB_565:
      case SST_LFB_555:
      case SST_LFB_1555:
      case SST_LFB_ZZ:
        info->strideInBytes = gc->fbStride << 1;

#if (GLIDE_PLATFORM & GLIDE_OS_MACOS) && SET_BSWAP
        if (!gc->cmdTransportInfo.fifoLfbP) lfbMode |= SST_LFB_WRITE_BYTESWAP;
#endif /* (GLIDE_PLATFORM & GLIDE_OS_MACOS) */
        break;

      case SST_LFB_888:
      case SST_LFB_8888:
      case SST_LFB_Z565:
      case SST_LFB_Z555:
      case SST_LFB_Z1555:
        info->strideInBytes = gc->fbStride << 2;
        break;
      }

      info->lfbPtr    = gc->lfb_ptr;
    }
  }
  break;
  default:
    rv = FXFALSE;
    GDBG_INFO(gc->myLevel,  "Lock failed because of invalid lock type.");
    break;
  }

  if (rv) {
    const FxU32 lfbLockCount = gc->cmdTransportInfo.lfbLockCount;

    gc->lockPtrs[type] = buffer;

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) && !GLIDE_INIT_HAL
    /* If the cmd fifo has been turned off before then make sure its
     * on again before trying to dork w/ the modes etc.  
     */
    if (lfbLockCount > 0) {
      GR_ASSERT(!gc->cmdTransportInfo.fifoLfbP);

#if GLIDE_DEBUG || GLIDE_CHECK_COMPATABILITY
      pciLinearRangeSetPermission((const FxU32)((const FxU8*)gc->base_ptr + 0x200000UL),
                                  0x200000UL,
                                  FXTRUE);
#endif /* GLIDE_DEBUG || GLIDE_CHECK_COMPATABILITY */

      /* Hack to allow the register writes below to be happy.
       *
       * NB: This *MUST* come before the unlock call since that
       * is going to write a nopCmd via the init code callback.
       */
      gc->cmdTransportInfo.lfbLockCount = 0;
      sst1InitLfbUnlock((FxU32*)hw);

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
      grFinish();
#else
      grSstIdle();
#endif
    }
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && !GLIDE_INIT_HAL */

    /* Setup the hw w/ the settings computed above. */
    switch(type) {
    case GR_LFB_READ_ONLY:
      GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
      GR_SET(BROADCAST_ID, hw, lfbMode, lfbMode);
      GR_CHECK_SIZE();
      break;

    case GR_LFB_WRITE_ONLY:
      REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 3, 0x103);
      {          
        REG_GROUP_SET(hw, fbzMode, fbzMode);
        REG_GROUP_SET(hw, lfbMode, lfbMode);
        REG_GROUP_SET(hw, zaColor, zaColor);
      }
      REG_GROUP_END();

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
      break;
    }

    if (!gc->cmdTransportInfo.fifoLfbP && idleLockP) {
      /* This is required to flush the write buffers before the
       * actual LFB accesses.
       */
      P6FENCE;

      /* Must idle to get things in the right order. */
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
      grFinish();
#else
      grSstIdle();
#endif
    }

    /* If we're not coming through grLfbWriteRegion then we need to
     * temporarily turn off the command fifo so that the accesses are
     * processed correctly by the hw.
     */
    if (!gc->cmdTransportInfo.fifoLfbP) {
#if (GLIDE_PLATFORM & GLIDE_HW_CVG) && !GLIDE_INIT_HAL
      sst1InitLfbLock((FxU32*)hw);
      
#if GLIDE_DEBUG || GLIDE_CHECK_COMPATABILITY
      {
        const FxU32 lfbRange = (((info->strideInBytes * gc->state.screen_height) + 0x1000UL) & 
                                ~(0x1000UL - 1));
        
        pciLinearRangeSetPermission((const FxU32)((const FxU8*)gc->base_ptr + 0x200000UL),
                                    0x200000UL,
                                    FXFALSE);
        pciLinearRangeSetPermission((const FxU32)gc->lfb_ptr,
                                    lfbRange,
                                    FXTRUE);
        pciLinearRangeSetPermission((const FxU32)((const FxU8*)gc->lfb_ptr + lfbRange),
                                    0x400000UL - lfbRange,
                                    FXFALSE);
      }
#endif /* GLIDE_DEBUG || GLIDE_CHECK_COMPATABILITY */
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && !GLIDE_INIT_HAL */
      
      /* Increment lock count */
      gc->cmdTransportInfo.lfbLockCount = lfbLockCount + 1;
    }
  }

  GR_RETURN(rv);
#undef FN_NAME
} /* grLfbLock */
    
/*-------------------------------------------------------------------
  Function: grLfbUnlock
  Date: 2/21
  Implementor(s): jdt
  Library: Glide
  Description:
  Unlock a previously locked buffer
  Arguments:
  type - type of lock (read only/write only)
  buffer - which buffer to unlock 
  Return:
  FXTRUE  - success
  FXFALSE - failure
  -------------------------------------------------------------------*/

GR_ENTRY(grLfbUnlock, FxBool, (GrLock_t type, GrBuffer_t buffer))
{
#define FN_NAME "grLfbUnlock"
  FxBool rval = FXFALSE;
  
  GR_BEGIN_NOFIFOCHECK("grLfbUnLock",83);
  GDBG_INFO_MORE(gc->myLevel,"(%d, %d)\n", type, buffer);
  
  type = type & ~(GR_LFB_NOIDLE);
  
  GR_CHECK_COMPATABILITY(FN_NAME, 
                         type != GR_LFB_WRITE_ONLY &&
                         type != GR_LFB_READ_ONLY,
                         "Bad type");
  
  GR_CHECK_COMPATABILITY(FN_NAME, 
                         buffer != GR_BUFFER_FRONTBUFFER &&
                         buffer != GR_BUFFER_BACKBUFFER  &&
                         buffer != GR_BUFFER_AUXBUFFER,
                         "Bad buffer");

#if ((GLIDE_PLATFORM & GLIDE_HW_SST1) || (GLIDE_PLATFORM & GLIDE_HW_CVG)) 
  if (gc->lockPtrs[type] == (FxU32)buffer) {
    rval = FXTRUE;
    gc->lockPtrs[type] = (FxU32)-1;

    /* Is this a direct lfb access? */
    if (!gc->cmdTransportInfo.fifoLfbP) {
      /* Flush the write buffers */
      if (type != GR_LFB_READ_ONLY) P6FENCE;

      /* If there are no more outstanding lfb locks, then turn the
       * fifo back on before we do anything else otherwise just
       * return w/ the new lock count.  
       */
      gc->cmdTransportInfo.lfbLockCount--;
      if (gc->cmdTransportInfo.lfbLockCount == 0) {
#if (GLIDE_PLATFORM & GLIDE_HW_CVG) && !GLIDE_INIT_HAL
#if GLIDE_DEBUG || GLIDE_CHECK_COMPATABILITY
        {
          pciLinearRangeSetPermission((const FxU32)((const FxU8*)gc->base_ptr + 0x200000UL),
                                      0x200000UL,
                                      FXTRUE);
          pciLinearRangeSetPermission((const FxU32)gc->lfb_ptr,
                                      0x400000,
                                      FXFALSE);
        }
#endif /* GLIDE_DEBUG || GLIDE_CHECK_COMPATABILITY */

        sst1InitLfbUnlock((FxU32*)hw);
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && !GLIDE_INIT_HAL */
      } else {
        return FXTRUE;
      }
    }
    
    GR_SET_EXPECTED_SIZE(8 + MaskSelect(gc->scanline_interleaved, sizeof(FxU32)),
                         2 + gc->scanline_interleaved);
    {
      /* Restore depth bias level */
      GR_SET(BROADCAST_ID, hw, zaColor, gc->state.fbi_config.zaColor);
      
      /* turn back on depth biasing */
      GR_SET(BROADCAST_ID, hw, fbzMode, gc->state.fbi_config.fbzMode);
      
      if (gc->scanline_interleaved) GR_SET(BROADCAST_ID, hw, nopCMD, 0x0);
    }
    GR_CHECK_SIZE();
  } else {
    GDBG_INFO(gc->myLevel, "Unlock failed because there was no matching lock");
    rval = FXFALSE;
  }
#endif

  GR_RETURN(rval);
#undef FN_NAME
} /* grLfbUnlock */


/*---------------------------------------------------------------------------
** grLfbWriteColorFormat
**
*/

GR_STATE_ENTRY(grLfbWriteColorFormat, void, (GrColorFormat_t colorFormat))
{
#define FN_NAME "_grLfbWriteColorFormat"
  FxU32 lfbMode;

  GR_BEGIN_NOFIFOCHECK("_grLfbWriteColorFormat",82);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",colorFormat);

  GR_CHECK_F(myName,
             colorFormat < 0 || colorFormat > 0x3,
             "invalid color format");

#if defined(GLIDE3)
  //_grValidateState();
#endif

  lfbMode = gc->state.fbi_config.lfbMode;
  lfbMode &= ~SST_LFB_RGBALANES;
  lfbMode |= (colorFormat << SST_LFB_RGBALANES_SHIFT);

  {
    const FxBool sliP = gc->scanline_interleaved;

    GR_SET_EXPECTED_SIZE(sizeof(FxU32) + MaskSelect(sliP, sizeof(FxU32)),
                         1 + sliP);
    {
      GR_SET(BROADCAST_ID, hw, lfbMode, lfbMode);
      gc->state.fbi_config.lfbMode = lfbMode;
      
      if (sliP) GR_SET(BROADCAST_ID, hw, nopCMD, 0x0);
    }
    GR_CHECK_SIZE();
  }

  GR_END();
#undef FN_NAME
} /* grLfbWriteColorFormat */


/*---------------------------------------------------------------------------
**  grLfbWriteColorSwizzle  - set up SST for byte swizzling and word swapping
**
**  Registers/Bits Affected:
**  lfbMode:  bit(11), bit(12)
**
**  WARNING:  GMT: SST_LFB_WRITE_SWAP16 changes pixel addressing!!!
*/
GR_STATE_ENTRY(grLfbWriteColorSwizzle, void, (FxBool swizzleBytes, FxBool swapWords))
{
#define FN_NAME "_grLfbWriteColorSwizzle"
  FxU32 lfbMode;

  GR_BEGIN_NOFIFOCHECK("_grLfbWriteColorSwizzle",82);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d)\n",swizzleBytes,swapWords);

#if defined(GLIDE3)
  //_grValidateState();
#endif

  /* Clear out the bits we'll set back if appropriate */
  lfbMode = gc->state.fbi_config.lfbMode;
  lfbMode &= ~(SST_LFB_WRITE_SWAP16 | SST_LFB_WRITE_BYTESWAP);

  if (swizzleBytes)
    lfbMode |= SST_LFB_WRITE_BYTESWAP;

  if (swapWords)
    lfbMode |= SST_LFB_WRITE_SWAP16;

  {
    const FxBool sliP = gc->scanline_interleaved;

    GR_SET_EXPECTED_SIZE(sizeof(FxU32) + MaskSelect(sliP, sizeof(FxU32)),
                         1 + sliP);
    {
      GR_SET(BROADCAST_ID, hw, lfbMode, lfbMode);
      gc->state.fbi_config.lfbMode = lfbMode;
      
      if (sliP) GR_SET(BROADCAST_ID, hw, nopCMD, 0x0);
    }
    GR_CHECK_SIZE();
  }

  GR_END();
#undef FN_NAME
} /* grLfbWriteColorSwizzle */

FxBool
_grLfbWriteRegion(FxBool pixPipelineP,
                  GrBuffer_t dst_buffer, FxU32 dst_x, FxU32 dst_y, 
                  GrLfbSrcFmt_t src_format, 
                  FxU32 src_width, FxU32 src_height, 
                  FxI32 src_stride, void *src_data)
{
#define FN_NAME "_grLfbWriteRegion"
  FxBool           rv = FXTRUE;
  GrLfbInfo_t      info;
  GrLfbWriteMode_t writeMode;
  
  GR_BEGIN_NOFIFOCHECK("_grLfbWriteRegion", 82);
  GDBG_INFO_MORE(gc->myLevel,
                 "(0x%x, %s, %d, %d, %d, %d, %d, %d, 0x%x)\n", 
                 dst_buffer,
                 (pixPipelineP ? "Enable" : "Disable"),
                 dst_x, dst_y, 
                 src_format, src_width, src_height,
                 src_stride, src_data);
  
  if (src_format == GR_LFB_SRC_FMT_RLE16) 
    writeMode = GR_LFBWRITEMODE_565;
  else 
    writeMode = src_format;
  
  gc->cmdTransportInfo.fifoLfbP = FXTRUE;
  info.size = sizeof(info);
  
  if (grLfbLock(GR_LFB_WRITE_ONLY | GR_LFB_NOIDLE, 
                dst_buffer, 
                writeMode,
                GR_ORIGIN_UPPER_LEFT,
                pixPipelineP,
                &info)) {
    FxU32 *srcData;             /* Tracking Source Pointer */
    FxU32 *dstData;             /* Tracking Destination Pointer */
    FxU32 *end;                 /* Demarks End of each Scanline */
    FxI32 srcJump;              /* bytes to next scanline */
    FxU32 dstJump;              /* bytes to next scanline */
    FxU32 length;               /* bytes to copy in scanline */
    FxU32 scanline;             /* scanline number */
    int   aligned;              /* word aligned? */

    /* When writing to the fifo we give an offset from the
     * start of the lfb so we don't need to add it in here.
     */
#if USE_PACKET_FIFO
    dstData = (FxU32*)((unsigned long)(dst_y * info.strideInBytes));
#else
    dstData = (FxU32*)(((FxU8*)info.lfbPtr) + (dst_y * info.strideInBytes));
#endif
      
    srcData = (FxU32*)src_data;
    scanline = src_height;
    
    switch(src_format) {
      /* 16-bit aligned */
    case GR_LFB_SRC_FMT_565:
    case GR_LFB_SRC_FMT_555:
    case GR_LFB_SRC_FMT_1555:
    case GR_LFB_SRC_FMT_ZA16:
      dstData = (FxU32*)(((FxU16*)dstData) + dst_x);
      length  = src_width * 2;
      aligned = !((unsigned long)dstData & 0x2);
      srcJump = src_stride - length;
      dstJump = info.strideInBytes - length;

      if (aligned) {
        while(scanline--) {
          GR_ASSERT(((unsigned long)dstData & 0x03UL) == 0);
          end = (FxU32*)((char*)srcData + length - 2);
               
          if (srcData < end) {
            LINEAR_WRITE_BEGIN(length >> 2, kLinearWriteLFB, dstData, 0x00, 0x00);
            while(srcData < end) {
              LINEAR_WRITE_SET_16(dstData, srcData[0]);
              dstData++;
              srcData++;
            }
            LINEAR_WRITE_END();
          }
                  
          if ((length & 0x3) != 0x00) {
            LINEAR_WRITE_EDGE(kLinearWriteLFB, dstData, *(FxU16*)srcData, sizeof(FxU16));
            dstData = (FxU32*)(((FxU16*)dstData) + 1);
            srcData = (FxU32*)(((FxU16*)srcData) + 1);
          }
                  
          dstData = (FxU32*)(((char*)dstData) + dstJump);
          srcData = (FxU32*)(((char*)srcData) + srcJump);
        }
      } else {
        while(scanline--) {
          GR_ASSERT(((unsigned long)dstData & 0x03UL) != 0);
          end = (FxU32*)((char*)srcData + length);

          LINEAR_WRITE_EDGE(kLinearWriteLFB, dstData, 
                            *(FxU16*)srcData, sizeof(FxU16));
          dstData = (FxU32*)(((FxU16*)dstData) + 1);
          srcData = (FxU32*)(((FxU16*)srcData) + 1);
                  
          if (srcData < end) {
            const FxU32 numWords = (((unsigned long)end - (unsigned long)srcData) >> 2);
            FxU32 i;

            LINEAR_WRITE_BEGIN(numWords, kLinearWriteLFB, dstData, 0x00, 0x00);
            for(i = 0; i < numWords; i++) {
              LINEAR_WRITE_SET_16(dstData, srcData[0]);
              dstData++;
              srcData++;
            }
            LINEAR_WRITE_END();
          }
                  
          if ((length & 0x03) == 0) {
            LINEAR_WRITE_EDGE(kLinearWriteLFB, dstData, 
                              *(FxU16*)srcData, sizeof(FxU16));
            dstData = (FxU32*)(((FxU16*)dstData) + 1);
            srcData = (FxU32*)(((FxU16*)srcData) + 1);
          }
                  
          dstData = (FxU32*)(((char*)dstData) + dstJump);
          srcData = (FxU32*)(((char*)srcData) + srcJump);
        }
      }
      break;

      /* 32-bit aligned */
    case GR_LFB_SRC_FMT_888:
    case GR_LFB_SRC_FMT_8888:
    case GR_LFB_SRC_FMT_565_DEPTH:
    case GR_LFB_SRC_FMT_555_DEPTH:
    case GR_LFB_SRC_FMT_1555_DEPTH:
      dstData = ((FxU32*)dstData) + dst_x;
      length  = src_width * 4;
      srcJump = src_stride - length;
      dstJump = info.strideInBytes - length;
      while(scanline--) {
        end = (FxU32*)((char*)srcData + length);
            
        LINEAR_WRITE_BEGIN(src_width,
                           kLinearWriteLFB,
                           dstData,
                           0x00, 0x00);
        while(srcData < end) {
          LINEAR_WRITE_SET(dstData, srcData[0]);
          dstData++;
          srcData++;
        }
        LINEAR_WRITE_END();
            
        dstData = (FxU32*)(((char*)dstData)+dstJump);
        srcData = (FxU32*)(((char*)srcData)+srcJump);
      }
      break;
    case GR_LFB_SRC_FMT_RLE16:
      /* needs to be implemented */
      rv = FXFALSE;
      break;
    }
    grLfbUnlock(GR_LFB_WRITE_ONLY, dst_buffer);
  } else {
    rv = FXFALSE;
  }
  gc->cmdTransportInfo.fifoLfbP = FXFALSE;
  GR_RETURN(rv);
#undef FN_NAME
} /* grLfbWriteRegion */

/*-------------------------------------------------------------------
  Function: grLfbWriteRegion
  Date: 3/5
  Implementor(s): jdt
  Library: Glide
  Description:
  Write a pixel rectangle to the frame buffer as efficiently as possible
  Arguments:
  dst_buffer  - buffer to which to copy data
  dst_x,dst_y - destination image start coordinates
  src_format  - data format of source image
  src_width, src_height 
              - dimensions of source image
  src_stride  - stride of source image in bytes, not meaningful
                for RLE images
  src_data    - pointer to source data memory           
  Return:
  FXTRUE  succeed
  FXFALSE fail
  -------------------------------------------------------------------*/
GR_ENTRY(grLfbWriteRegion, FxBool, ( GrBuffer_t dst_buffer, 
                                     FxU32 dst_x, FxU32 dst_y, 
                                     GrLfbSrcFmt_t src_format, 
                                     FxU32 src_width, FxU32 src_height, 
                                     FxBool pixelPipeline,
                                     FxI32 src_stride, void *src_data ))
{
  FxBool rv;
  
  GR_BEGIN_NOFIFOCHECK("grLfbWriteRegion",82);
  GDBG_INFO_MORE(gc->myLevel,
                 "(0x%x,%d,%d,%d,%d,%d,%d,0x%x)\n", 
                 dst_buffer, dst_x, dst_y, 
                 src_format, src_width, src_height,
                 src_stride, src_data);

  rv = _grLfbWriteRegion(pixelPipeline, dst_buffer, dst_x, dst_y,
                         src_format, src_width, src_height,
                         src_stride, src_data);
  
  GR_RETURN(rv);
} /* grLfbWriteRegion */

/*-------------------------------------------------------------------
  Function: grLfbReadRegion
  Date: 3/12
  Implementor(s): jdt
  Library: Glide
  Description:
  Grab a rectangle from the frame buffer into user supplied memory
  Arguments:
  src_buffer - buffer to read from 
  src_x      - x coordinate of upper left corner rectangle to read
  src_y      - y coordinate of upper left corner of rectangle to read
  src_width  - width of rectangle to read
  src_height - height of rectangle to read
  dst_stride - distance between scanlines in destination buffer
  dst_data   - pointer to user memory in which to place image
  Return:
  FXTRUE - success
  FXFALSE - failure
  -------------------------------------------------------------------*/
GR_ENTRY(grLfbReadRegion, FxBool, (GrBuffer_t src_buffer, 
                                   FxU32 src_x, FxU32 src_y, 
                                   FxU32 src_width, FxU32 src_height, 
                                   FxU32 dst_stride, void *dst_data))
{
#define FN_NAME "grLfbReadRegion"
  FxBool rv = FXTRUE;
  GrLfbInfo_t info;
  
  GR_BEGIN_NOFIFOCHECK("grLfbReadRegion", 82);
  GDBG_INFO_MORE(gc->myLevel,
                 "(0x%x,%d,%d,%d,%d,%d,0x%x)\n",
                 src_buffer, src_x, src_y,
                 src_width, src_height, dst_stride, dst_data);

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) 
#define kTileSize (32 * sizeof(FxU16))
#define kTileMask (kTileSize - 1)
#define kPageMask ((kTileSize << 1) - 1)

  {
    const FxBool swapP = ((gc->grColBuf >= 3) &&
                          (((src_buffer == GR_BUFFER_BACKBUFFER) && (gc->hwDep.cvgDep.backBuf == 2)) ||
                           ((src_buffer == GR_BUFFER_FRONTBUFFER) && (gc->hwDep.cvgDep.frontBuf == 2))));

    if (swapP && (gc->grAuxBuf > 0)) sst1InitAllocBuffers(gc->base_ptr, 
                                                          gc->grColBuf, 
                                                          0);
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) */

    gc->cmdTransportInfo.fifoLfbP = FXTRUE;  
    info.size = sizeof(info);
    if (grLfbLock(GR_LFB_READ_ONLY,
                  src_buffer,
                  GR_LFBWRITEMODE_ANY,
                  GR_ORIGIN_UPPER_LEFT,
                  FXFALSE,
                  &info)) {
      const FxU32 *srcData;       /* Tracking Source Pointer */
      FxU32 *dstData;             /* Tracking Destination Pointer */
      FxU32 srcJump;              /* bytes to next scanline */
      FxU32 dstJump;              /* bytes to next scanline */
      FxU32 length;               /* bytes to copy in scanline */
      FxU32 scanline;             /* scanline number */
     
      dstData = (FxU32*)dst_data;
      srcData = (const FxU32*)((FxU8*)info.lfbPtr +
                               (src_y * info.strideInBytes) +
                               (src_x << 1));
      scanline = src_height;
      length   = src_width * 2;
      dstJump  = dst_stride - length;
      srcJump  = info.strideInBytes - length;

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) 
      /* Voodoo2 has a problem reading from the frame buffer if we are
       * triple buffering and the triple buffer is the currently
       * active lfb buffer. The problem is that the 32 pixel screen
       * tiles are reversed such that the logical tiling 0101 comes
       * back as 1010. We do the 'swapping' here for the app, but
       * if they use direct lfb accesses they are screwed.
       */
      if (swapP) {
        while(scanline--) {
          FxU32* const dstEnd = (FxU32*)((FxU8*)dstData + length);
          const FxU32* end = (const FxU32*)((FxU8*)srcData + length);
          const FxU32* tilePtr;
          const FxU32* tileEnd;
          const FxU32* tileEndAlign;

          /* Leading slop up to the start of a logical 0 tile */
          if (((unsigned long)srcData & kPageMask) != 0) {
            unsigned long tileSlopMask = ~0UL; // XXX was -1UL.
            FxU32 tileSlopAdjust = kTileSize;

            /* Do we have a partial 0 tile? */
            if (((unsigned long)srcData & kTileSize) == 0) {
              tilePtr      = (const FxU32*)((unsigned long)srcData + kTileSize);
              tileEnd      = (const FxU32*)(((unsigned long)end <= ((unsigned long)tilePtr & ~kTileMask))
                                            ? ((unsigned long)end + kTileSize)
                                            : (((unsigned long)tilePtr + kTileSize) & ~kTileMask));
              tileEndAlign = (const FxU32*)((unsigned long)tileEnd & ~0x03UL);

              /* Are we aligned in the hw lfb? */
              if (((unsigned long)tilePtr & 0x03UL) != 0) {
                *(FxU16*)dstData = GR_GET16(*tilePtr);
                dstData = (FxU32*)((FxU8*)dstData + sizeof(FxU16));
                tilePtr = (const FxU32*)((FxU8*)tilePtr + sizeof(FxU16));
              }

              /* Copy the remainder of the 0 tile */
              while(tilePtr < tileEndAlign) *dstData++ = GR_GET(*tilePtr++);
              if (tileEnd != tileEndAlign) *(FxU16*)dstData = GR_GET16(*tilePtr);

              tileSlopMask = ~kTileMask;
              tileSlopAdjust = 0;
            }

            /* Copying the logical 1 tile. This may require us to go
             * 'backwards' in physical memory if there was no logical
             * 0 tile in the current read.  
             */
            tilePtr      = (const FxU32*)(((unsigned long)srcData - tileSlopAdjust) & tileSlopMask);
            tileEnd      = (const FxU32*)MIN((((unsigned long)tilePtr + kTileSize) & ~kTileMask), 
                                             (unsigned long)end - kTileSize - sizeof(FxU16));
            tileEndAlign = (const FxU32*)((unsigned long)tileEnd & ~0x03UL);

            if (tilePtr < tileEnd) {
              /* Are we aligned in the hw lfb? */
              if (((unsigned long)tilePtr & 0x03UL) != 0) {
                *(FxU16*)dstData = GR_GET16(*tilePtr);
                dstData = (FxU32*)((FxU8*)dstData + sizeof(FxU16));
                tilePtr = (const FxU32*)((FxU8*)tilePtr + sizeof(FxU16));
              }
              
              /* Copy the remainder of the logical 1 tile */
              while(tilePtr < tileEndAlign) *dstData++ = GR_GET(*tilePtr++);
              if (tileEnd != tileEndAlign) 
                *(FxU16*)dstData = GR_GET16(*tilePtr);
            }
              
            srcData = (const FxU32*)(((unsigned long)srcData + (kTileSize << 1)) & ~kPageMask);
          }

          /* Loop over complete logical 01 tile groups */
          {
            const FxU32* endTileAddr = (const FxU32*)((unsigned long)end & ~kPageMask);

            while(srcData < endTileAddr) {
              tilePtr     = (const FxU32*)((unsigned long)srcData + kTileSize);
              tileEnd     = (const FxU32*)((unsigned long)tilePtr + kTileSize);
              while(tilePtr < tileEnd) *dstData++ = GR_GET(*tilePtr++);
              
              tilePtr = srcData;
              tileEnd = (const FxU32*)((unsigned long)tilePtr + kTileSize);
              while(tilePtr < tileEnd) *dstData++ = GR_GET(*tilePtr++);

              srcData = (const FxU32*)((FxU8*)srcData + (kTileSize << 1));
            }
          }

          /* Slop 01 tile group */
          if (srcData < end) {
            const FxU32* startTileAddr = (const FxU32*)((unsigned long)srcData + kTileSize);
            
            tilePtr      = startTileAddr;
            tileEnd      = (const FxU32*)((unsigned long)startTileAddr + 
                                          MIN(kTileSize, ((unsigned long)end - (unsigned long)srcData)));
            tileEndAlign = (const FxU32*)((unsigned long)tileEnd & ~0x3);

            while(tilePtr < tileEndAlign) *dstData++ = GR_GET(*tilePtr++);
            if (tileEnd != tileEndAlign) 
              *(FxU16*)dstData = GR_GET16(*tilePtr);

            if (startTileAddr < end) {
              tilePtr      = srcData;
              tileEnd      = (const FxU32*)((FxU8*)tilePtr + kTileSize - 
                                            ((unsigned long)tileEndAlign - (unsigned long)end));
              tileEndAlign = (const FxU32*)((unsigned long)tileEnd & ~0x3);

              while(tilePtr < tileEndAlign) *dstData++ = GR_GET(*tilePtr++);
              if (tileEnd != tileEndAlign) 
                *(FxU16*)dstData = GR_GET16(*tilePtr);
            }
          }

          /* Adjust pointers */
          dstData = (FxU32*)((FxU8*)dstEnd + dstJump);
          srcData = (const FxU32*)((FxU8*)end + srcJump);
        }
      } else
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) */     
      /* If the source data is aligned for 4 byte pci reads */
      if (((unsigned long)srcData & 0x02UL) == 0) {
        while(scanline--) {
          const FxU32* end = (const FxU32*)((char*)srcData + length - 2);
          
          while(srcData < end) {
            *dstData++ = GR_GET(*srcData++);
          }
          
          if (((int)length) & 0x2) {
            (*(FxU16*)dstData) = GR_GET16(*srcData);
            dstData = (FxU32*)(((FxU16*)dstData) + 1);
            srcData = (FxU32*)(((FxU16*)srcData) + 1);
          }
          
          dstData = (FxU32*)(((char*)dstData)+dstJump);
          srcData = (FxU32*)(((char*)srcData)+srcJump);
        }
      } else {
        while(scanline--) {
          const FxU32* end = (const FxU32*)((char*)srcData + length - 2);

          (*(FxU16*)dstData) = (FxU16)GR_GET16(*srcData);
          dstData = (FxU32*)(((FxU16*)dstData) + 1);
          srcData = (FxU32*)(((FxU16*)srcData) + 1);

          while(srcData < end) {
            *dstData++ = GR_GET(*srcData++);
          }

          if (!(((int)length) & 0x2)) {
            (*(FxU16*)dstData) = GR_GET16(*srcData);
            dstData = (FxU32*)(((FxU16*)dstData) + 1);
            srcData = (FxU32*)(((FxU16*)srcData) + 1);
          }
                
          dstData = (FxU32*)(((char*)dstData)+dstJump);
          srcData = (FxU32*)(((char*)srcData)+srcJump);
        }
      }

      grLfbUnlock(GR_LFB_READ_ONLY, src_buffer);
    } else {
      rv = FXFALSE;
    }
    gc->cmdTransportInfo.fifoLfbP = FXFALSE;

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) 
    if (swapP && (gc->grAuxBuf > 0)) sst1InitAllocBuffers(gc->base_ptr, 
                                                          gc->grColBuf, 
                                                          gc->grAuxBuf);
  }
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG)  */

  GR_RETURN(rv);
#undef FN_NAME
}/* grLfbReadRegion */

