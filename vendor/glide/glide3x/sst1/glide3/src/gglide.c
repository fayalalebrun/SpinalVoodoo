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
** Revision 1.1.2.6  2006/01/16 21:22:45  jwrdegoede
** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
**
** Revision 1.1.2.5  2005/05/25 08:51:52  jwrdegoede
** Add #ifdef GL_X86 around x86 specific code
**
** Revision 1.1.2.4  2004/10/04 09:35:59  dborca
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
** Revision 1.1.1.1  1999/12/07 21:48:52  joseph
** Initial checkin into SourceForge.
**
** 
** 78    1/21/98 2:25p Peter
** fixed assertion muckage
 * 
 * 77    1/12/98 10:25p Dow
 * H3D Stereo Support
 * 
 * 76    1/07/98 9:58a Jdt
 * Include Z in datalist for GR_COMBINE_LOCAL_DEPTH...Fixes Bug 1084
 * 
 * 75    12/19/97 8:09a Peter
 * fog table propogation
 * 
 * 74    9/24/97 7:02p Atai
 * 1. use initStatus() instead of grSstStatus() in grBufferNumPending
 * 2. add one p6 nudge code in grBufferSwap()
 * 
 * 73    9/07/97 12:14p Pgj
 * fix bug 660
 * 
 * 72    9/05/97 4:46p Peter
 * fixed fifo silliness, sorry
 * 
 * 71    8/19/97 8:55p Peter
 * lots of stuff, hopefully no muckage
 * 
 * 70    8/17/97 10:04p Pgj
 * Fix bug 735
 * 
 * 69    8/01/97 11:48a Dow
 * Added a size check for debugging
 * 
 * 68    7/20/97 2:05p Dow
 * Hopefully worked around VSYNC bug
 * 
 * 67    7/18/97 6:48p Jdt
 * grGlideShutdown now unsets _GlideRoot.initialized, further open/close
 * symmetry fix
 * 
 * 66    7/09/97 10:18a Dow
 * Further Nudge Of Love adjustments
 * 
 * 65    7/04/97 12:08p Dow
 * Separated SST96 & SST1 versions of grRebuildDataList, fixed chip field
 * muckage
 * 
 * 64    6/30/97 6:43p Dow
 * Added more blank lines
 * 
 * 63    6/29/97 11:47p Jdt
 * Adjustments for triangle optimization
 * 
 * 62    6/29/97 1:58p Dow
 * Fixed busted ifdef
 * 
 * 61    6/26/97 3:09p Dow
 * protected P6 stuff
 * 
 * 60    6/20/97 5:52p Dow
 * Changes to make chip field work
 * 
 * 59    6/19/97 7:35p Dow
 * More P6 Stuff
 * 
 * 58    6/18/97 6:08p Dow
 * Fixed bogus call to gdbg_error
 * 
 * 57    6/18/97 5:54p Dow
 * P6 adjustments
 * 
 * 56    6/12/97 5:19p Pgj
 * Fix bug 578
 * 
 * 55    5/27/97 11:37p Pgj
 * Fix for Bug report 545
 * 
 * 54    5/22/97 2:18p Pgj
 * maxx --> maxy
 * 
 * 53    5/11/97 11:27p Pgj
 * bug report 1152
 * 
 * 52    5/04/97 12:47p Dow
 * Moved ClipWindow sanity check to internal routine as grSstControl needs
 * it.
 * 
 * 51    5/02/97 2:09p Pgj
 * grClipWindow sanity checks coords (intersect w/screen rect)
 * 
 * 50    3/24/97 2:00p Dow
 * Fixed some chip field problems
 * 
 * 49    3/22/97 7:04p Dow
 * Fixed chip field
 * 
 * 48    3/21/97 12:58p Dow
 * Fixed error for SST1
 * 
 * 47    3/21/97 12:42p Dow
 * Made STWHints not send the Bend Over Baby Packet to FBI Jr.
 * 
 * 46    3/19/97 10:42p Dow
 * Dither subtraction sense, windowsInit
 * 
 * 45    3/18/97 9:08p Dow
 * Added FX_GLIDE_NO_DITHER_SUB environment variable
 * 
 * 44    3/17/97 6:26a Jdt
 * Re-worked grGlideShutdown()
 * 
 * 43    3/07/97 2:51p Jdt
 * Fix for SST96 register offsets.
 * 
 * 42    3/07/97 2:29p Dow
 * Maybe fixed lines
 * 
 * 41    3/05/97 1:07p Dow
 * Clipping and origin changes for SST98
 * 
 * 40    3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 39    3/01/97 2:48p Dow
 * Moved buffer swap count increment so that it works for both VG & VR
 * 
 * 38    2/26/97 11:56a Jdt
 * Updated lfb api, fixed swapping
 * 
 * 37    2/18/97 9:51a Jdt
 * Removed a bunch of warnings, no functional changes.
 * 
 * 36    2/12/97 10:39p Hanson
 * 
 * 35    2/12/97 2:09p Hanson
 * Hopefully removed the rest of my muckage. 
 * 
 * 34    1/14/97 10:45a Dow
 * Added check for pending swaps to grBufferSwap for VG96
 * Modified grBufferNumPending to work with VG96
 * 
 * 33    12/23/96 1:37p Dow
 * chagnes for multiplatform glide
 * 
 * 32    11/17/96 11:16p Garymct
 * Updated grSplash code to set all of the necessary state itself rather
 * than expecting the caller to do so.
 * 
 * 31    11/18/96 1:37a Tarolli
 * fixed grAlphaBlendFunction warning bug
 * 
 * 29    11/15/96 3:40p Jdt
 * Fixed SST-96 build.
 * 
 * 28    11/15/96 3:24p Tarolli
 * renamed version.h to rcver.h ,  added some alpha blend function
 * checking
**
*/

#include <string.h>
#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"
#include "fxinline.h"

#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
#include <init.h>
#endif

/*---------------------------------------------------------------------------
** grAlphaBlendFunction
**
** GMT: BUG if grColorMask() turns off alphaplanes then destination alpha
**          blending wont work!
*/

GR_STATE_ENTRY(grAlphaBlendFunction, void, ( GrAlphaBlendFnc_t rgb_sf,   GrAlphaBlendFnc_t rgb_df, GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df ))
{
  FxU32 alphamode;

  GR_BEGIN_NOFIFOCHECK("grAlphaBlendFunction",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d,%d)\n",rgb_sf,rgb_df,alpha_sf,alpha_df));

  alphamode = gc->state.fbi_config.alphaMode;
  if (alpha_sf != GR_BLEND_ONE && alpha_sf != GR_BLEND_ZERO) {
    // GR_CHECK_W(myName, 1, "unsupported alpha source blend function");
    alpha_sf = GR_BLEND_ONE;
  }
  if (alpha_df != GR_BLEND_ONE && alpha_df != GR_BLEND_ZERO) {
    // GR_CHECK_W(myName, 1, "unsupported alpha destination blend function");
    alpha_df = GR_BLEND_ZERO;
  }
  if ( rgb_sf   == GR_BLEND_ONE && rgb_df   == GR_BLEND_ZERO &&
      alpha_sf == GR_BLEND_ONE && alpha_df == GR_BLEND_ZERO )
    alphamode &= ~SST_ENALPHABLEND;
  else
    alphamode |=  SST_ENALPHABLEND;
  
  alphamode &= ~( SST_RGBSRCFACT | SST_RGBDSTFACT | SST_ASRCFACT | SST_ADSTFACT );
  
  alphamode |= ( ( ( FxU32 ) rgb_sf ) << SST_RGBSRCFACT_SHIFT ) |
    ( ( ( FxU32 ) rgb_df ) << SST_RGBDSTFACT_SHIFT ) |
      ( ( ( FxU32 ) alpha_sf ) << SST_ASRCFACT_SHIFT ) |
        ( ( ( FxU32 ) alpha_df ) << SST_ADSTFACT_SHIFT );
  
  gc->state.fbi_config.alphaMode = alphamode;
} /* grAlphaBlendFunction */

/*---------------------------------------------------------------------------
** grAlphaCombine
*/
GR_STATE_ENTRY(grAlphaCombine, void, (GrCombineFunction_t function, GrCombineFactor_t factor, GrCombineLocal_t local, GrCombineOther_t other, FxBool invert ))
{
  FxU32 fbzColorPath;
  FxU32 oldTextureEnabled;

  GR_BEGIN_NOFIFOCHECK("grAlphaCombine",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d,%d,%d)\n",function,factor,local,other,invert));
  GR_CHECK_W(myName,
              function < GR_COMBINE_FUNCTION_ZERO ||
              function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA,
              "unsupported alpha combine function");
  GR_CHECK_W(myName,
              (factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
              (factor & 0x7) > GR_COMBINE_FACTOR_TEXTURE_ALPHA ||
              factor > GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA,
              "unsupported alpha combine scale factor");

  GR_CHECK_W(myName,
              local < GR_COMBINE_LOCAL_ITERATED || local > GR_COMBINE_LOCAL_DEPTH,
              "unsupported alpha combine local color");
  GR_CHECK_W(myName,
              other < GR_COMBINE_OTHER_ITERATED || other > GR_COMBINE_OTHER_CONSTANT,
              "unsupported alpha combine other color");

  fbzColorPath = gc->state.fbi_config.fbzColorPath;
  oldTextureEnabled = fbzColorPath & SST_ENTEXTUREMAP;
  fbzColorPath &= ~( SST_ENTEXTUREMAP |
                    SST_ASELECT |
                    SST_ALOCALSELECT |
                    SST_CCA_ZERO_OTHER |
                    SST_CCA_SUB_CLOCAL |
                    SST_CCA_MSELECT |
                    SST_CCA_REVERSE_BLEND |
                    SST_CCA_ADD_CLOCAL |
                    SST_CCA_ADD_ALOCAL |
                    SST_CCA_INVERT_OUTPUT );
  
  /* setup reverse blending first, then strip off the the high bit */
  if ( (factor & 0x8) == 0 )
    fbzColorPath |= SST_CCA_REVERSE_BLEND;
  factor &= 0x7;

  /* NOTE: we use boolean OR instead of logical to avoid branches */
  gc->state.ac_requires_texture = ( factor == GR_COMBINE_FACTOR_TEXTURE_ALPHA ) |
                                  ( other == GR_COMBINE_OTHER_TEXTURE );
  gc->state.ac_requires_it_alpha = ( local == GR_COMBINE_LOCAL_ITERATED ) |
                                   ( other == GR_COMBINE_OTHER_ITERATED );
  
  /* setup scale factor bits */
  fbzColorPath |= factor << SST_CCA_MSELECT_SHIFT;

  /* setup local color bits */
  fbzColorPath |= local << SST_ALOCALSELECT_SHIFT;
  
  /* setup other color bits */
  fbzColorPath |= other << SST_ASELECT_SHIFT;
  
  
  /* setup invert output bits */
  if ( invert )
    fbzColorPath |= SST_CCA_INVERT_OUTPUT;
  
  /* setup core color combine unit bits */
  switch ( function ) {
  case GR_COMBINE_FUNCTION_ZERO:
    fbzColorPath |= SST_CCA_ZERO_OTHER;
    break;
    
  case GR_COMBINE_FUNCTION_LOCAL:
  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    fbzColorPath |= SST_CCA_ZERO_OTHER | SST_CCA_ADD_ALOCAL;
    break;
    
  case GR_COMBINE_FUNCTION_SCALE_OTHER:
    break;
    
  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
    fbzColorPath |= SST_CCA_ADD_ALOCAL;
    break;
    
  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
    fbzColorPath |= SST_CCA_SUB_CLOCAL;
    break;
    
  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    fbzColorPath |= SST_CCA_SUB_CLOCAL | SST_CCA_ADD_ALOCAL;
    break;
    
  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    fbzColorPath |= SST_CCA_ZERO_OTHER | SST_CCA_SUB_CLOCAL | SST_CCA_ADD_ALOCAL;
    break;
  }

  /* if either color or alpha combine requires texture then enable it */
  if ( gc->state.cc_requires_texture || gc->state.ac_requires_texture )
    fbzColorPath |= SST_ENTEXTUREMAP;

  /* transition into/out of texturing ... add nopCMD */

  if(oldTextureEnabled != (fbzColorPath & SST_ENTEXTUREMAP)) {
    GR_SET_EXPECTED_SIZE(sizeof(FxU32));
    P6FENCE_CMD( GR_SET(hw->nopCMD,0) );
    GR_CHECK_SIZE();
  }

  gc->state.fbi_config.fbzColorPath = fbzColorPath;

} /* grAlphaCombine */

/*---------------------------------------------------------------------------
** grAlphaControlsITRGBLighting
** 
** Determines whether the LSB of alpha controls what lighting is used--
** Specifically whether grConstantColorValu or the interated RGB values are used
** during TEXTURE_TIMES_itrgb & TEXTURE_TIMES_ITRGB_DELTA0 color combine modes.
**
*/  

GR_STATE_ENTRY(grAlphaControlsITRGBLighting, void, (FxBool enable))
{
  FxU32 fbzColorPath;

  GR_BEGIN_NOFIFOCHECK("grAlphaControlsITRGBLighting",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",enable));

  fbzColorPath = gc->state.fbi_config.fbzColorPath;
  if ( enable ) {
    fbzColorPath |= SST_LOCALSELECT_OVERRIDE_WITH_ATEX;
  } else {
    fbzColorPath &= ~SST_LOCALSELECT_OVERRIDE_WITH_ATEX;
  }

  gc->state.fbi_config.fbzColorPath = fbzColorPath;
} /* grAlphaControlsITRGBLighting() */

/*---------------------------------------------------------------------------
** grAlphaTestFunction
*/

GR_STATE_ENTRY(grAlphaTestFunction, void, ( GrCmpFnc_t fnc ))
{
  FxU32 alphamode;

  GR_BEGIN_NOFIFOCHECK("grAlphaTestFunction",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",fnc));

  alphamode = gc->state.fbi_config.alphaMode;
  alphamode &= ~(SST_ALPHAFUNC | SST_ENALPHAFUNC);
  if ( fnc != GR_CMP_ALWAYS )
    alphamode |= ( ( fnc << SST_ALPHAFUNC_SHIFT ) | SST_ENALPHAFUNC );

  gc->state.fbi_config.alphaMode = alphamode;
} /* grAlphaTestFunction */

/*---------------------------------------------------------------------------
** grAlphaTestReferenceValue
*/

GR_STATE_ENTRY(grAlphaTestReferenceValue, void, ( GrAlpha_t value ))
{
  FxU32 alphamode;

  GR_BEGIN_NOFIFOCHECK("grAlphaTestReferenceValue",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",value));

  alphamode = gc->state.fbi_config.alphaMode;
  alphamode &= ~SST_ALPHAREF;
  alphamode |= ( ( ( FxU32 ) value ) << SST_ALPHAREF_SHIFT );

  gc->state.fbi_config.alphaMode = alphamode;
} /* grAlphaTestReferenceValue */

/*---------------------------------------------------------------------------
** grBufferClear
*/

GR_ENTRY(grBufferClear, void, ( GrColor_t color, GrAlpha_t alpha, FxU32 depth ))
{
  GrColor_t oldc1;
  FxU32 oldzacolor, zacolor;

  GR_DCL_GC;
  GR_DCL_HW;

  if (gc->state.invalid)
    _grValidateState();

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
  GR_SET_EXPECTED_SIZE(44);
#else
  GR_SET_EXPECTED_SIZE(24);
#endif

  oldc1   = gc->state.fbi_config.color1;
  zacolor = oldzacolor = gc->state.fbi_config.zaColor;

  /*
  ** Setup source registers
  */
  if ( gc->state.fbi_config.fbzMode & SST_RGBWRMASK )
  {
    _grSwizzleColor( &color );
    GR_SET( hw->c1, color );
  }
  if ( ( gc->state.fbi_config.fbzMode & ( SST_ENALPHABUFFER | SST_ZAWRMASK ) ) == ( SST_ENALPHABUFFER | SST_ZAWRMASK ) )
  {
    zacolor &= ~SST_ZACOLOR_ALPHA;
    zacolor |= ( ( ( FxU32 ) alpha ) << SST_ZACOLOR_ALPHA_SHIFT );
    GR_SET( hw->zaColor, zacolor );
  }
  if ( ( gc->state.fbi_config.fbzMode & ( SST_ENDEPTHBUFFER | SST_ZAWRMASK ) ) == ( SST_ENDEPTHBUFFER | SST_ZAWRMASK ) )  {
    zacolor &= ~SST_ZACOLOR_DEPTH;
    zacolor |= ( ( ( FxU32 ) depth ) << SST_ZACOLOR_DEPTH_SHIFT );
    GR_SET( hw->zaColor, zacolor );
  }
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
  /*
  **  On SST96, it's faster if clear the buffers separately if we need
  **  to clear both of them.
  */
  if (
      (gc->state.fbi_config.fbzMode & (SST_RGBWRMASK | SST_ZAWRMASK))
      == (SST_RGBWRMASK | SST_ZAWRMASK)
      ) {
    FxU32 fbzMode;
    /* Disable ZA write and fill */
    fbzMode = gc->state.fbi_config.fbzMode & ~SST_ZAWRMASK;
    GR_SET(hw->fbzMode, fbzMode);
    GR_SET(hw->fastfillCMD,1);

    /* Disable RGB write mask and fill */
    fbzMode = gc->state.fbi_config.fbzMode & ~SST_RGBWRMASK;
    GR_SET(hw->fbzMode, fbzMode);
    GR_SET(hw->fastfillCMD,1);

    /* Restore fbzMode to previous state */
    GR_SET(hw->fbzMode, gc->state.fbi_config.fbzMode);
        
  } else
#endif
  /*
  ** Execute the FASTFILL command
  */
  P6FENCE_CMD( GR_SET(hw->fastfillCMD,1) );


  /*
  ** Restore C1 and ZACOLOR
  */
  GR_SET( hw->c1, oldc1 );
  GR_SET( hw->zaColor, oldzacolor );
  GR_CHECK_SIZE_SLOPPY();  
} /* grBufferClear */


/*---------------------------------------------------------------------------
** grBufferSwap
**
** NOTE:        This routine should be COMPLETELY device-independant,
**              but it isn't right now because we need to move all the
**              code for the splash screen into the init library.
*/

GR_ENTRY(grBufferSwap, void, ( FxU32 swapInterval ))
{
#define FN_NAME "grBuferSwap"
  int vSync;

#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
#ifdef GDBG_INFO_ON
  int
    nTries;
#endif

  int pending;
#endif

  GR_BEGIN_NOFIFOCHECK(FN_NAME,80);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",swapInterval));

#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 ) 
  FXUNUSED( vSync );

#ifdef GDBG_INFO_ON
  nTries = 0;
#endif

  /*
   * To speed up glquake, we take the following steps
   * 1. remove the status checking
   * 2. modify grBufferNumPending so it does not need to do P6 nudge code
   * 3. add one p6 nudge code
   * Here is step 3.
   * If we introduce new bugs in 2.43, please undo the above steps
   */
  P6_NUDGE_OF_LOVE;

  pending = _grBufferNumPending();

  if (pending < 1 && gc->grSstRez != GR_RESOLUTION_NONE) {
    GR_SET_EXPECTED_SIZE((9 << 2));
    GR_SET(hw->fbzMode, (gc->state.fbi_config.fbzMode & (~(SST_RGBWRMASK | SST_ZAWRMASK))));
    GR_SETF(hw->FvA.x, 0.f);
    GR_SETF(hw->FvA.y, 0.f);
    GR_SETF(hw->FvB.x, 640.f);
    GR_SETF(hw->FvB.y, 0.f);
    GR_SETF(hw->FvC.x, 640.f);
    GR_SETF(hw->FvC.y, 480.f);
    GR_SETF(hw->FtriangleCMD, 1.f);
    GR_SET(hw->fbzMode, gc->state.fbi_config.fbzMode);
    GR_CHECK_SIZE();
  }

  while (pending > _GlideRoot.environment.swapPendingCount) {
#ifdef GDBG_INFO_ON
    GDBG_INFO((86, "%s:  Checked pending %d times\n", FN_NAME,
               ++nTries));
    if (nTries > 10000)
      GDBG_ERROR(FN_NAME , "Exiting after %d tries for pending\n", nTries);
#endif /* GDBG_INFO_ON */
    pending = _grBufferNumPending();
  }
  
#ifdef GLIDE_PLUG
  /* optionally display the 3Dfx powerfield logo overlay */
  if (_GlideRoot.environment.shamelessPlug)
    _grShamelessPlug();
#endif

  initSwapBuffers( swapInterval );

#ifdef H3D
  if (GR_RESOLUTION_IS_AUTOFLIPPED(gc->grSstRez)) {
    extern void pageflippingSwapWait(void);
    do {
      pending = grBufferNumPending();
    } while (pending > 0);
    pageflippingSwapWait();
  }
#endif

  P6_NUDGE_OF_LOVE;

#elif ( GLIDE_PLATFORM & GLIDE_HW_SST1 ) 
  #ifdef GLIDE_PLUG
  /* optionally display the 3Dfx powerfield logo overlay */
  if (_GlideRoot.environment.shamelessPlug)
    _grShamelessPlug();
  #endif

  /* check for environmental override */
  if (_GlideRoot.environment.swapInterval >= 0) {
    swapInterval = _GlideRoot.environment.swapInterval;
  }

  GR_CHECK_F(myName,
              ( swapInterval > 255 ) || ( swapInterval < 0 ),
              "swap_interval out of range");

  /* wait until there's 6 or fewer buffer swaps pending */
  /* the hardware counter is only 3 bits so we don't want it to overflow */
  /* also the latency gets too long */
  while (_grBufferNumPending() > _GlideRoot.environment.swapPendingCount)
   ;

  /* if the interval is non-zero turn on VSYNC waiting */
  vSync = swapInterval > 0 ? 1 : 0;

  if ((swapInterval > 0) &&
      (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.fbiRev == 1))
  {
    /* Adjust for .6 micron FBI */
    if (swapInterval == 2)
      swapInterval = 3;
    else if (swapInterval != 1)
      swapInterval <<= 1;
  } else {
    if (swapInterval > 0) swapInterval--;
  }

  /* NOTE: we need a PCI read before and after the swap command */
  /* but since we already called grBufferNumPending() we've done a read */
  GR_SET_EXPECTED_SIZE(4);
  P6FENCE_CMD( GR_SET(hw->swapbufferCMD, (swapInterval<<1) | vSync) );

#ifdef GLIDE_DEBUG
  if (_GlideRoot.environment.snapshot > 0) {
    static char saveDBG[GDBG_MAX_LEVELS];
    int i;

    /* turn off tracing after frame 0 and the snapshot frame */
    if (_GlideRoot.stats.bufferSwaps == 1 || 
        _GlideRoot.stats.bufferSwaps == _GlideRoot.environment.snapshot+1)
    {
      for (i=1; i<GDBG_MAX_LEVELS; i++) {
        saveDBG[i] = (char)GDBG_GET_DEBUGLEVEL(i);
        GDBG_SET_DEBUGLEVEL(i,0);
      }
    }
    /* turn on tracing after the snapshot frame */
    if (_GlideRoot.stats.bufferSwaps == _GlideRoot.environment.snapshot) {
      for (i=1; i<GDBG_MAX_LEVELS; i++) {
        GDBG_SET_DEBUGLEVEL(i,saveDBG[i]);
      }
    }
  }
#endif /* GLIDE_DEBUG */
  _grSstStatus();        /* special bug workaround       */
#else
#    error "Need to implement swap." 
#endif /* GLIDE_PLATFORM & GLIDE_HW_SST1 */

  _GlideRoot.stats.bufferSwaps++;

  GR_END();
#undef FN_NAME  
} /* grBufferSwap */

/*---------------------------------------------------------------------------
** grBufferNumPending
*/

int FX_CSTYLE
_grBufferNumPending(void)
{
  int
    pend;                       /* Num Swaps pending */ 
  /*
   * To speed up glquake, we take the following steps
   * 1. remove the status checking
   * 2. modify grBufferNumPending so it does not need to do P6 nudge code
   * 3. add one p6 nudge code
   * Here is step 2. Goto grBufferSwap() in gglide.c to continue.
   */
  FxU32
    status = initStatus();

#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
  pend = (status & SST_SWAPBUFPENDING) >> SST_SWAPBUFPENDING_SHIFT;
#elif (GLIDE_PLATFORM & GLIDE_HW_SST96)
#define SST96_SWAPPEND_SHIFT    0x4
#define SST96_SWAPPEND_BITS     (0x7 << SST96_SWAPPEND_SHIFT)
  pend = ((status & SST96_SWAPPEND_BITS) >> SST96_SWAPPEND_SHIFT);
#else
#  error "Implement BufferNumPending for this hardware"
#endif

  GDBG_INFO((86,"grBufferNumPending() => %d\n",pend));

  return pend;

} /* grBufferNumPending */

/*---------------------------------------------------------------------------
** grChromakeyMode
*/

GR_STATE_ENTRY(grChromakeyMode, void, ( GrChromakeyMode_t mode ))
{
  FxU32 fbzMode;

  GR_BEGIN_NOFIFOCHECK("grChromakeyMode",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mode));

  fbzMode = gc->state.fbi_config.fbzMode;
  if ( mode == GR_CHROMAKEY_ENABLE )
    fbzMode |= SST_ENCHROMAKEY;
  else
    fbzMode &= ~SST_ENCHROMAKEY;

  gc->state.fbi_config.fbzMode = fbzMode;
} /* grChromaKeyMode */

/*---------------------------------------------------------------------------
** grChromaRange
*/

GR_STATE_ENTRY(grChromaRange, void, (GrColor_t min, GrColor_t max, GrChromaRangeMode_t mode))
{
  GR_BEGIN_NOFIFOCHECK("grChromakeyValue", 85);
  GDBG_INFO_MORE((gc->myLevel, "(0x%x)\n", min));
  GDBG_INFO_MORE((gc->myLevel, "(0x%x)\n", max));
  GDBG_INFO_MORE((gc->myLevel, "(0x%x)\n", mode));

  _grSwizzleColor(&min);
  _grSwizzleColor(&max);
  max = max & 0x00ffffff;
#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
  if (gc->state.stateArgs.grChromakeyModeArgs.mode == GR_CHROMAKEY_ENABLE)
    max |= SST_ENCHROMARANGE;
#endif
  mode = mode << 24;
  gc->state.fbi_config.chromaKey = min;
  gc->state.fbi_config.chromaRange = max | mode;

} /* grChromaRange */

/*---------------------------------------------------------------------------
** _grClipNormalizeAndGenerateRegValues
**
** This routine normalizes clip variables to screen_width and
** screen_height, then generates the values to be written to the
** registers clipLeftRight and clipBottomTop.   The reason for its
** existence is that BOTH grClipWindow() and grSstControl() need to
** write these registers.
*/
void
_grClipNormalizeAndGenerateRegValues(FxU32 minx, FxU32 miny, FxU32 maxx,
                                     FxU32 maxy, FxU32 *clipLeftRight,
                                     FxU32 *clipBottomTop)
{
#define FN_NAME "_grClipNormalizeAndGenerateRegValues"
  GR_DCL_GC;

  GDBG_INFO((41, "%s:  minx = %d, maxx = %d, miny = %d, maxy = %d\n",
           FN_NAME, minx, maxx, miny, maxy));

  /* don't allow bogus clip coords!!! */
  if (maxx > gc->state.screen_width) maxx = gc->state.screen_width;
  if (maxy > gc->state.screen_height) maxy = gc->state.screen_height;
  if (minx > maxx) minx = maxx;
  if (miny > maxy) miny = maxy;

  GDBG_INFO((41, "%s: normalized  minx = %d, maxx = %d, miny = %d, maxy = %d\n",
             FN_NAME, minx, maxx, miny, maxy));

  *clipLeftRight =
    ( minx << SST_CLIPLEFT_SHIFT ) | ( maxx << SST_CLIPRIGHT_SHIFT ); 

  *clipBottomTop =
    ( miny << SST_CLIPBOTTOM_SHIFT ) | ( maxy << SST_CLIPTOP_SHIFT );

  GDBG_INFO((41, "%s:  clipLeftRight = 0x%x, clipBottomTop = 0x%x\n",
             FN_NAME, clipLeftRight, clipBottomTop));

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
  /* Enable clipping */
  *clipLeftRight |= 0x80000000;
#endif  
#undef FN_NAME 
} /* _grClipNormalizeAndGenerateRegValues */

/*---------------------------------------------------------------------------
** grClipWindow
*/
GR_STATE_ENTRY(grClipWindow, void, ( FxU32 minx, FxU32 miny, FxU32 maxx,
                              FxU32 maxy )) 
{
  FxU32
    clipLeftRight,              /* SST Clipping Registers */
    clipBottomTop;

  GR_BEGIN_NOFIFOCHECK("grClipWindow",41);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d %d,%d)\n",minx,miny,maxx,maxy));

  _grClipNormalizeAndGenerateRegValues(minx, miny, maxx, maxy,
                                       &clipLeftRight, &clipBottomTop);

  gc->state.fbi_config.clipLeftRight = clipLeftRight;
  gc->state.fbi_config.clipBottomTop = clipBottomTop;

  gc->state.clipwindowf_xmin = ( float ) minx;
  gc->state.clipwindowf_xmax = ( float ) maxx;
  gc->state.clipwindowf_ymin = ( float ) miny;
  gc->state.clipwindowf_ymax = ( float ) maxy;

} /* grClipWindow */

/*---------------------------------------------------------------------------
** grColorCombine
*/

GR_STATE_ENTRY(grColorCombine, void, ( GrCombineFunction_t function, GrCombineFactor_t factor, GrCombineLocal_t local, GrCombineOther_t other, FxBool invert ))
{
  FxU32 fbzColorPath;
  FxU32 oldTextureEnabled;

  GR_BEGIN_NOFIFOCHECK("grColorCombine",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d,%d,%d)\n",function,factor,local,other,invert));

  GR_CHECK_W( myName,
               function < GR_COMBINE_FUNCTION_ZERO ||
               function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA,
               "unsupported color combine function" );

  GR_CHECK_W( myName,
              (factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
              (factor & 0x7) > GR_COMBINE_FACTOR_TEXTURE_ALPHA ||
              factor > GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA,
              "unsupported color combine scale factor");
  GR_CHECK_W( myName,
              local < GR_COMBINE_LOCAL_ITERATED || local > GR_COMBINE_LOCAL_DEPTH,
              "unsupported color combine local color");
  GR_CHECK_W( myName,
              other < GR_COMBINE_OTHER_ITERATED || other > GR_COMBINE_OTHER_CONSTANT,
              "unsupported color combine other color");

  fbzColorPath = gc->state.fbi_config.fbzColorPath;
  oldTextureEnabled = fbzColorPath & SST_ENTEXTUREMAP;
  fbzColorPath &= ~( SST_ENTEXTUREMAP |
                     SST_RGBSELECT |
                     SST_LOCALSELECT |
                     SST_CC_ZERO_OTHER |
                     SST_CC_SUB_CLOCAL |
                     SST_CC_MSELECT |
                     SST_CC_REVERSE_BLEND |
                     SST_CC_ADD_CLOCAL |
                     SST_CC_ADD_ALOCAL |
                     SST_CC_INVERT_OUTPUT );

  /* this is bogus, it should be done once, somewhere. */
  fbzColorPath |= SST_PARMADJUST;

  /* setup reverse blending first, then strip off the the high bit */
  if ( (factor & 0x8) == 0 )
    fbzColorPath |= SST_CC_REVERSE_BLEND;
  factor &= 0x7;

  /* NOTE: we use boolean OR instead of logical to avoid branches */
  gc->state.cc_requires_texture = ( factor == GR_COMBINE_FACTOR_TEXTURE_ALPHA ) |
                                  ( other == GR_COMBINE_OTHER_TEXTURE );
  gc->state.cc_requires_it_rgb = ( local == GR_COMBINE_LOCAL_ITERATED ) |
                                 ( other == GR_COMBINE_OTHER_ITERATED );

  /* setup scale factor bits */
  fbzColorPath |= factor << SST_CC_MSELECT_SHIFT;

  /* setup local color bits */
  fbzColorPath |= local << SST_LOCALSELECT_SHIFT;

  /* setup other color bits */
  fbzColorPath |= other << SST_RGBSELECT_SHIFT;

  /* setup invert output bits */
  if ( invert )
    fbzColorPath |= SST_CC_INVERT_OUTPUT;

  /* setup core color combine unit bits */
  switch ( function )
  {
  case GR_COMBINE_FUNCTION_ZERO:
    fbzColorPath |= SST_CC_ZERO_OTHER;
    break;

  case GR_COMBINE_FUNCTION_LOCAL:
    fbzColorPath |= SST_CC_ZERO_OTHER | SST_CC_ADD_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    fbzColorPath |= SST_CC_ZERO_OTHER | SST_CC_ADD_ALOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER:
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
    fbzColorPath |= SST_CC_ADD_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
    fbzColorPath |= SST_CC_ADD_ALOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
    fbzColorPath |= SST_CC_SUB_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
    fbzColorPath |= SST_CC_SUB_CLOCAL | SST_CC_ADD_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    fbzColorPath |= SST_CC_SUB_CLOCAL | SST_CC_ADD_ALOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
    fbzColorPath |= SST_CC_ZERO_OTHER | SST_CC_SUB_CLOCAL | SST_CC_ADD_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    fbzColorPath |= SST_CC_ZERO_OTHER | SST_CC_SUB_CLOCAL | SST_CC_ADD_ALOCAL;
    break;
  }

  /* if either color or alpha combine requires texture then enable it */
  if ( gc->state.cc_requires_texture || gc->state.ac_requires_texture )
    fbzColorPath |= SST_ENTEXTUREMAP;

  /* if we transition into/out of texturing ... add nopCMD */
  if(oldTextureEnabled != (fbzColorPath & SST_ENTEXTUREMAP)) {
    GR_SET_EXPECTED_SIZE(sizeof(FxU32));
    P6FENCE_CMD( GR_SET(hw->nopCMD,0) );
    GR_CHECK_SIZE();
  }

  gc->state.fbi_config.fbzColorPath = fbzColorPath;
} /* grColorCombine */

/*---------------------------------------------------------------------------
** grColorMask
*/

GR_STATE_ENTRY(grColorMask, void, ( FxBool rgb, FxBool alpha ))
{
  FxU32 fbzMode;

  GR_BEGIN_NOFIFOCHECK("grColorMask",85);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x,0x%x)\n",rgb,alpha));

  fbzMode = gc->state.fbi_config.fbzMode;
  GR_CHECK_W( myName,
              ( fbzMode & SST_ENDEPTHBUFFER ) && alpha,
              "alpha writes enabled even though depth buffering" );
  GR_CHECK_W( myName,
              (gc->state.num_buffers == 3) && alpha,
              "alpha writes enabled while triple buffering" );

  if ( rgb )
    fbzMode |= SST_RGBWRMASK;
  else
    fbzMode &= ~SST_RGBWRMASK;

  /* GMT: BUG leave SST_ENALPHABUFFER on if dest. alpha being used */
  /* Don't do anything if depth buffering on */
  /* XXX Should check for triple buffering too */

  if ( !(fbzMode & SST_ENDEPTHBUFFER) && (gc->state.num_buffers != 3) ) {
    if ( alpha )
      fbzMode |= SST_ENALPHABUFFER | SST_ZAWRMASK;
    else
      fbzMode &= ~(SST_ENALPHABUFFER | SST_ZAWRMASK);
  }

  gc->state.fbi_config.fbzMode = fbzMode;
} /* grColorMask */

/*---------------------------------------------------------------------------
** grConstantColorValue
*/

GR_STATE_ENTRY(grConstantColorValue, void, ( GrColor_t color ))
{
  GR_BEGIN_NOFIFOCHECK("grConstantColorValue",85);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",color));

  _grSwizzleColor( &color );

  gc->state.fbi_config.color0 = color;
  gc->state.fbi_config.color1 = color;
} /* grConstanColorValue */

/*---------------------------------------------------------------------------
** grConstantColorValue4
**           GMT: obsolete routine
**           GMT: send values to hardware immediately
*/

GR_STATE_ENTRY(grConstantColorValue4, void, ( float a, float r, float g, float b ))
{
  GR_BEGIN("grConstantColorValue4",85,12);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d,%d)\n",a,r,g,b));
  gc->state.a = a;
  gc->state.r = r;
  gc->state.g = g;
  gc->state.b = b;
  if ( gc->state.cc_delta0mode ) {
    GR_SETF( hw->Fr, r );
    GR_SETF( hw->Fg, g );
    GR_SETF( hw->Fb, b );
  }
  GR_END_SLOPPY();
} /* grConstantColorValue4 */

/*---------------------------------------------------------------------------
** grCullMode
**
** GMT: warning - gaa.c has the guts of this in-line
*/

GR_ENTRY(grCullMode, void, ( GrCullMode_t mode ))
{
  GR_BEGIN_NOFIFOCHECK("grCullMode",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mode));
  gc->state.cull_mode = mode;
  GR_END();
} /* grCullMode */

/*---------------------------------------------------------------------------
** grDepthBiasLevel
**
** Sets the depth bias level.
*/

GR_STATE_ENTRY(grDepthBiasLevel, void, ( FxI32 level ))
{
  FxU32 zacolor;

  GR_BEGIN_NOFIFOCHECK("grDepthBiasLevel",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",level));

  zacolor = gc->state.fbi_config.zaColor;
  zacolor = ( zacolor & ~SST_ZACOLOR_DEPTH ) | ((FxI16)level & SST_ZACOLOR_DEPTH);

  gc->state.fbi_config.zaColor = zacolor;
} /* grDepthBiasLevel */

/*---------------------------------------------------------------------------
** grDepthBufferFunction
*/

GR_STATE_ENTRY(grDepthBufferFunction, void, ( GrCmpFnc_t fnc ))
{
  FxU32 fbzMode;

  GR_BEGIN_NOFIFOCHECK("grDepthBufferFunction",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",fnc));

  fbzMode = gc->state.fbi_config.fbzMode;
  fbzMode &= ~SST_ZFUNC;
  fbzMode |= ( fnc << SST_ZFUNC_SHIFT );

  gc->state.fbi_config.fbzMode = fbzMode;
} /* grDepthBufferFunction */

/*---------------------------------------------------------------------------
** grDepthBufferMode
*/

GR_STATE_ENTRY(grDepthBufferMode, void, ( GrDepthBufferMode_t mode ))
{
  FxU32 fbzMode;

  GR_BEGIN_NOFIFOCHECK("grDepthBufferMode",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mode));

   /*
   ** depth buffering cannot be enabled at the same time as triple buffering
   */
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
   GR_CHECK_F( myName,
                mode != GR_DEPTHBUFFER_DISABLE && gc->state.num_buffers == 3 && gc->fbuf_size <= 2,
                "cannot enable depth buffer when triple buffering" );
#endif
   /*
   ** depth buffering cannot be enabled on systems running at:
   **      800x600 w/ 2MB
   **      640x480 w/ 1MB
   */
   GR_CHECK_F( myName,
                mode != GR_DEPTHBUFFER_DISABLE && !_grCanSupportDepthBuffer(),
                "cannot enable depthbuffer with configuration" );

  /* turn off all the bits and then turn them back on selectively */
  fbzMode = gc->state.fbi_config.fbzMode;
  fbzMode &= ~(SST_ENDEPTHBUFFER | SST_WBUFFER | SST_ENZBIAS | SST_ZCOMPARE_TO_ZACOLOR);

  switch (mode) {
  case GR_DEPTHBUFFER_DISABLE:
    break;
    
  case GR_DEPTHBUFFER_ZBUFFER:
    fbzMode |= SST_ENDEPTHBUFFER | SST_ENZBIAS;
    break;

  case GR_DEPTHBUFFER_WBUFFER:
    fbzMode |= SST_ENDEPTHBUFFER | SST_WBUFFER | SST_ENZBIAS;
    break;

  case GR_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS:
    fbzMode |= SST_ENDEPTHBUFFER | SST_ZCOMPARE_TO_ZACOLOR;
    break;

  case GR_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS:
    fbzMode |= SST_ENDEPTHBUFFER | SST_WBUFFER | SST_ZCOMPARE_TO_ZACOLOR;
    break;
  }

  /*
  ** Update hardware and Glide state
  */
  gc->state.fbi_config.fbzMode = fbzMode;
} /* grDepthBufferMode */

/*---------------------------------------------------------------------------
** grDepthMask
*/
GR_STATE_ENTRY(grDepthMask, void, ( FxBool enable ))
{
  FxU32 fbzMode;

  GR_BEGIN_NOFIFOCHECK("grDepthMask",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",enable));

  fbzMode = gc->state.fbi_config.fbzMode;
  GR_CHECK_F( myName,
                enable && !_grCanSupportDepthBuffer(),
                "called in a non-depthbufferable configuration" );

  if ( enable )
    fbzMode |= SST_ZAWRMASK;
  else
    fbzMode &= ~SST_ZAWRMASK;

  gc->state.fbi_config.fbzMode = fbzMode;
} /* grDepthMask */

/*---------------------------------------------------------------------------
** grDisableAllEffects
*/

GR_ENTRY(grDisableAllEffects, void, ( void ))
{
  GDBG_INFO((87,"grDisableAllEffects()\n"));
  grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO );
  grAlphaTestFunction( GR_CMP_ALWAYS );
  grChromakeyMode( GR_CHROMAKEY_DISABLE );
  grDepthBiasLevel( 0 );
  grDepthBufferMode( GR_DEPTHBUFFER_DISABLE );
  grFogMode( GR_FOG_DISABLE );
} /* grDisableAllEffects */

/*---------------------------------------------------------------------------
** grDitherMode
*/

GR_STATE_ENTRY(grDitherMode, void, ( GrDitherMode_t mode ))
{
  FxU32 fbzMode;

  GR_BEGIN_NOFIFOCHECK("grDitherMode",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mode));

  fbzMode = gc->state.fbi_config.fbzMode &
            ~( SST_ENDITHER | SST_DITHER2x2 | SST_ENDITHERSUBTRACT );

  switch ( mode ) {
     case GR_DITHER_DISABLE:
        break;
     case GR_DITHER_2x2:
        fbzMode |= ( SST_ENDITHER | SST_DITHER2x2 );
        break;
     case GR_DITHER_4x4:
        /* test per bug 660 */
        if (_GlideRoot.environment.disableDitherSub == FXTRUE) {
          fbzMode |= SST_ENDITHER;
        } else {
          fbzMode |= (SST_ENDITHER | SST_ENDITHERSUBTRACT);
        }
        break;
  }

  gc->state.fbi_config.fbzMode = fbzMode;
} /* grDitherMode */

/*---------------------------------------------------------------------------
** grFogMode
*/

GR_STATE_ENTRY(grFogMode, void, ( GrFogMode_t mode ))
{
  FxU32 fogmode;

  GR_BEGIN_NOFIFOCHECK("grFogMode",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mode));

  fogmode = gc->state.fbi_config.fogMode;
  fogmode &= ~( SST_ENFOGGING | SST_FOGADD | SST_FOGMULT | SST_FOG_ALPHA | SST_FOG_Z | SST_FOG_CONSTANT );

  switch ( mode & 0xFF ) {    /* switch based on lower 8 bits */
    case GR_FOG_DISABLE:
      break;
    case GR_FOG_WITH_ITERATED_ALPHA_EXT:
      fogmode |= ( SST_ENFOGGING | SST_FOG_ALPHA );
      break;
    case GR_FOG_WITH_ITERATED_Z:        /* Bug 735 */
      fogmode |= ( SST_ENFOGGING | SST_FOG_Z );
      break;
    case GR_FOG_WITH_TABLE_ON_W:
      fogmode |= ( SST_ENFOGGING );
  }
  if ( mode &  GR_FOG_MULT2 ) fogmode |= SST_FOGMULT;
  if ( mode &  GR_FOG_ADD2 ) fogmode |= SST_FOGADD;

  /*
  ** Update the hardware and Glide state
  */
  gc->state.fbi_config.fogMode = fogmode;
} /* grFogMode */

/*---------------------------------------------------------------------------
** grFogColorValue
*/

GR_STATE_ENTRY(grFogColorValue, void, ( GrColor_t color ))
{
  GR_BEGIN_NOFIFOCHECK("grFogColorValue",85);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",color));

  _grSwizzleColor( &color );

  gc->state.fbi_config.fogColor = color;
} /* grFogColorValue */

/*---------------------------------------------------------------------------
** grFogTable
*/

GR_ENTRY(grFogTable, void, ( const GrFog_t fogtable[kInternalFogTableEntryCount] ))
{
  int i, iend = kInternalFogTableEntryCount / 2 - 1;
  const GrFog_t *locTable = fogtable;

  GR_BEGIN("grFogTable",85,kInternalFogTableEntryCount*2);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",fogtable));
  GR_ASSERT(fogtable != NULL);
  GR_ASSERT(kInternalFogTableEntryCount == 64);

  for ( i = 0; i <= iend; i++ ) {
    GrFog_t e0,e1,d0,d1;

    e0 = locTable[0];                   /* lower entry */
    e1 = locTable[1];                   /* upper entry */
    d0 = (e1-e0)<<2;                    /* del0 in .2 format */
    d1 = (i==iend) ? e1 : locTable[2];  /* don't access beyond end of table */
    d1 = (d1-e1)<<2;                    /* del1 in .2 format */

    GR_SET( hw->fogTable[i], (e1<<24) | (d1<<16) | (e0<<8) | d0);
    locTable += 2;
  }
  GR_END();
} /* grFogTable */



/*-------------------------------------------------------------------
  Function: grGlideShutdown
  Date: 3/16
  Implementor(s): dow, gmt, jdt
  Library: Glide
  Description:
  Shutdown the Glide Library.  Iterate through all hardware and 
  call grSstWinClose().  Call InitShutdown() which unmaps all
  hardware from linear memory.
  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/

GR_ENTRY(grGlideShutdown, void, ( void ))
{
  _GlideRoot.windowsInit = FXFALSE;

  if ( _GlideRoot.initialized ) {
    { /* GMT: reset the counter so we can proceed without assertions */
      GR_DCL_GC;
      gc->counter = gc->expected_counter = 0;
    }
    {
      int i;    
      GR_BEGIN_NOFIFOCHECK("grGlideShutdown",80);
      GDBG_INFO_MORE((gc->myLevel,"()\n"));
      
#if ( GLIDE_PLATFORM & GLIDE_SST_SIM )
      {
        extern void gsim_winclose( void );
        gsim_winclose();
      }
#endif
      
      for( i = 0; i < _GlideRoot.hwConfig.num_sst; i++ ) {
        grSstSelect( i );
        grSstWinClose((GrContext_t)(_GlideRoot.GCs + i));
      }
      
      initClose();
      
      _grDisplayStats();
      gc->grSstRez           = GR_RESOLUTION_NONE;
      gc->grSstRefresh       = GR_REFRESH_NONE;
      _GlideRoot.initialized = FXFALSE;
      GR_END();
    }
  }
} /* grGlideShutdown */

/*---------------------------------------------------------------------------
** grGlideSetState
*/

GR_ENTRY(grGlideSetState, void, ( const void *state ))
{
  int tmu;
  Sstregs *tmuregs;

  GR_BEGIN("grGlideSetState",87,(1+13+7*gc->num_tmu)*4+(gc->num_tmu+1)*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",state));
  GR_ASSERT(state != NULL);

  /* if texture mapping changed then we need to issue a NOP command*/
  if ( (gc->state.fbi_config.fbzColorPath ^ ((GrState *)state)->fbi_config.fbzColorPath) &
        SST_ENTEXTUREMAP)
  {
    P6FENCE_CMD( GR_SET(hw->nopCMD,0) );
  }

  /* copy all the state */
  gc->state = *((GrState *)state);

  /* Update the hardware state from the saved state */
  GR_SET( hw->fbzColorPath, gc->state.fbi_config.fbzColorPath );
  GR_SET( hw->fogMode, gc->state.fbi_config.fogMode );
  GR_SET( hw->alphaMode, gc->state.fbi_config.alphaMode );
  GR_SET( hw->fbzMode, gc->state.fbi_config.fbzMode );
#if !(GLIDE_PLATFORM & GLIDE_HW_SST96)
  GR_SET( hw->lfbMode, gc->state.fbi_config.lfbMode );
#endif
  GR_SET( hw->clipLeftRight, gc->state.fbi_config.clipLeftRight );
  GR_SET( hw->clipBottomTop, gc->state.fbi_config.clipBottomTop );
  GR_SET( hw->fogColor, gc->state.fbi_config.fogColor );
  GR_SET( hw->zaColor, gc->state.fbi_config.zaColor );
  GR_SET( hw->chromaKey, gc->state.fbi_config.chromaKey );
  GR_SET( hw->stipple, gc->state.fbi_config.stipple );
  GR_SET( hw->c0, gc->state.fbi_config.color0 );
  GR_SET( hw->c1, gc->state.fbi_config.color1 );

  for ( tmu = 0; tmu < gc->num_tmu; tmu++ ) {
    tmuregs = SST_TMU(hw,tmu);
    PACKER_WORKAROUND;
    GR_SET( tmuregs->textureMode, gc->state.tmu_config[tmu].textureMode );
    GR_SET( tmuregs->tLOD, gc->state.tmu_config[tmu].tLOD );
    GR_SET( tmuregs->tDetail, gc->state.tmu_config[tmu].tDetail );
    GR_SET( tmuregs->texBaseAddr, gc->state.tmu_config[tmu].texBaseAddr );
    GR_SET( tmuregs->texBaseAddr1, gc->state.tmu_config[tmu].texBaseAddr_1 );
    GR_SET( tmuregs->texBaseAddr2, gc->state.tmu_config[tmu].texBaseAddr_2 );
    GR_SET( tmuregs->texBaseAddr38, gc->state.tmu_config[tmu].texBaseAddr_3_8 );
  }
  PACKER_WORKAROUND;

  /* NOTE: since glide state includes things like hints and
     all cached variables like paramIndex we needn't recompute these,
     BUT: we do need to rebuild stuff that depends on them!!! */

  _grUpdateParamIndex();

  GR_END_SLOPPY();
  
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
  if (gc->state.fbi_config.fbzMode & SST_YORIGIN)
    grSstOrigin(GR_ORIGIN_LOWER_LEFT);
  else
    grSstOrigin(GR_ORIGIN_UPPER_LEFT);
#endif

} /* grGlideSetState */

/*---------------------------------------------------------------------------
** grRenderBuffer
**
**  Although SST1 supports triple buffering, it's a hack in the hardware,
**  and the only drawbuffer modes supported by the fbzMode register are 0
**  (back) and 1 (front)
*/

GR_STATE_ENTRY(grRenderBuffer, void, ( GrBuffer_t buffer ))
{

  GR_BEGIN_NOFIFOCHECK("grRenderBuffer",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",buffer));
  GR_CHECK_F( myName, buffer >= GR_BUFFER_AUXBUFFER, "invalid buffer" );

#if ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  {
      FxU32 fbzMode;
      
      fbzMode = gc->state.fbi_config.fbzMode;
      fbzMode &= ~( SST_DRAWBUFFER );
      fbzMode |= buffer == GR_BUFFER_FRONTBUFFER ? SST_DRAWBUFFER_FRONT : SST_DRAWBUFFER_BACK;
      
      gc->state.fbi_config.fbzMode = fbzMode;
  }
#elif ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
  initRenderBuffer( buffer );
#else
#  error "No renderbuffer implementation"
#endif

} /* grRenderBuffer */

GR_ENTRY(grCheckForRoom, void, (FxI32 n))
{
   GR_DCL_GC;
   GR_CHECK_FOR_ROOM(n);
}

/*---------------------------------------------------------------------------
** _grUpdateParamIndex
**
** Updates the paramIndex bits based on Glide state and the hints.
**
*/
GR_DDFUNC(_grUpdateParamIndex, void, ( void ))
{
  GR_DCL_GC;
  FxU32 paramIndex = 0;
  FxU32 fbzColorPath = gc->state.fbi_config.fbzColorPath;
  FxU32 fogMode = gc->state.fbi_config.fogMode;
  FxU32 fbzMode = gc->state.fbi_config.fbzMode;

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
  FxU32 gwHeaderNum = 0;
  /* Set up bits for vtx a,b,c x & y */
  gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] = GWH_VTX_BITS;
#endif

  /*
  ** First, turn on every bit that we think we need. We can prune them
  ** back later.
  */

  /* Turn on the texture bits based on what grTexCombine set */
  if (fbzColorPath & SST_ENTEXTUREMAP) {
    /* No matter what, we need oow from the main grvertex structure */
    static FxU32 paramI_array[8] = {
      STATE_REQUIRES_OOW_FBI,
      STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0,
      STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1,
      STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0 |
      STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1,
      STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2,
      STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2 |
      STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0,
      STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2 |
      STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1,
      STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2 |
      STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0 |
      STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1
    };
    GR_ASSERT(gc->state.tmuMask < sizeof(paramI_array)/sizeof(paramI_array[0]));
    paramIndex |= paramI_array[gc->state.tmuMask];
  }  

  /* See if we need iterated RGB */
  if ( gc->state.cc_requires_it_rgb && !gc->state.cc_delta0mode ) {
    paramIndex |= STATE_REQUIRES_IT_DRGB;
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
    gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= (GWH_RGB_BITS);
#endif
  }

  /* See if we need to iterate alpha based on the value of
     ac_requires_it_alpha */ 
  if (gc->state.ac_requires_it_alpha) {
    paramIndex |= STATE_REQUIRES_IT_ALPHA;
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
    gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_A_BITS;
#endif
  }

  /* See what fbzColorPath contributes - BUG 1084*/
  if ( ( fbzColorPath & SST_ALOCALSELECT ) == 
       ( SST_ALOCAL_Z ) ) {
      paramIndex |= STATE_REQUIRES_OOZ;
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_Z_BITS;
#endif
  }

  /* See what fbzMode contributes */
  if (fbzMode & SST_ENDEPTHBUFFER) {
    if (fbzMode & SST_WBUFFER) {
      paramIndex |= STATE_REQUIRES_OOW_FBI;
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_W_BITS;
#endif
    } else {
      paramIndex |= STATE_REQUIRES_OOZ;
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_Z_BITS;
#endif
    }
  }

  /* See what fogMode contributes */
  if (fogMode & SST_ENFOGGING) {
    if (fogMode & SST_FOG_Z) {
      paramIndex |= STATE_REQUIRES_OOZ;
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_Z_BITS;
#endif
    } else {
      if (fogMode & SST_FOG_ALPHA) {
        paramIndex |= STATE_REQUIRES_IT_ALPHA;
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_A_BITS;
#endif
      } else {
        paramIndex |= STATE_REQUIRES_OOW_FBI;
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_W_BITS;
#endif
      }
    }
  }

  /*
  **  Now we know everything that needs to be iterated.  Prune back
  **  the stuff that isn't explicitly different
  **
  **  NOTE: by GMT, STATE_REQUIRES_OOW_FBI is set whenever texture mapping
  **        is enabled
  */
  /* Turn off W for TMU0 if we don't have a hint */
  if (paramIndex & STATE_REQUIRES_W_TMU0) {
    GR_ASSERT(paramIndex & STATE_REQUIRES_OOW_FBI);
    if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
      if (gc->state.vData.q0Info.mode == GR_PARAM_DISABLE)
        paramIndex &= ~STATE_REQUIRES_W_TMU0;
    }
    else {
      if ((gc->state.vData.q0Info.mode == GR_PARAM_DISABLE) &&
          (gc->state.vData.qInfo.mode == GR_PARAM_DISABLE))
        paramIndex &= ~STATE_REQUIRES_W_TMU0;
    }
  }
  
  /* Turn off ST for TMU1 if TMU0 is active and TMU1 is not different */
  if (((paramIndex & (STATE_REQUIRES_ST_TMU0 | STATE_REQUIRES_ST_TMU1)) ==
       (STATE_REQUIRES_ST_TMU0 | STATE_REQUIRES_ST_TMU1)) &&
      gc->state.vData.st1Info.mode == GR_PARAM_DISABLE)
    paramIndex &= ~STATE_REQUIRES_ST_TMU1;
  
  /* Turn off W for TMU1 if we have a previous W, and don't have a hint */ 
  if (paramIndex & STATE_REQUIRES_W_TMU1) {
    if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
      if (gc->state.vData.q1Info.mode == GR_PARAM_DISABLE)
        paramIndex &= ~STATE_REQUIRES_W_TMU0;
    }
    else {
      if ((gc->state.vData.q1Info.mode == GR_PARAM_DISABLE) &&
          (gc->state.vData.qInfo.mode == GR_PARAM_DISABLE))
        paramIndex &= ~STATE_REQUIRES_W_TMU0;
    }
  }

#if (GLIDE_NUM_TMU > 2)
  /* Turn off ST for TMU1 if it's not different & any other is set up.  */ 
  if ((paramIndex & (STATE_REQUIRES_ST_TMU0 | STATE_REQURES_ST_TMU1)) &&
      (paramIndex & STATE_REQUIRES_ST_TMU2) &&
      !(hints & GR_STWHINT_ST_DIFF_TMU2))
    paramIndex &= ~STATE_REQUIRES_ST_TMU2;
  
  /* Turn off W for TMU2 if we have a previous W, and don't have a hint */ 
  if ((paramIndex & STATE_REQUIRES_W_TMU2) && !(hints & GR_STWHINT_W_DIFF_TMU2))
    paramIndex &= ~STATE_REQUIRES_W_TMU2;
#endif
  
  /*
  ** last check
  */
  if (gc->state.vData.colorType == GR_U8) {
    if (gc->state.vData.pargbInfo.mode == GR_PARAM_DISABLE) {
      paramIndex &= ~STATE_REQUIRES_IT_ALPHA;
      paramIndex &= ~STATE_REQUIRES_IT_DRGB;
    }
  }
  else {
    if (gc->state.vData.aInfo.mode == GR_PARAM_DISABLE) {
      paramIndex &= ~STATE_REQUIRES_IT_ALPHA;
    }
    if (gc->state.vData.rgbInfo.mode == GR_PARAM_DISABLE) {
      paramIndex &= ~STATE_REQUIRES_IT_DRGB;
    }
  }
  if (gc->state.vData.zInfo.mode == GR_PARAM_DISABLE)
    paramIndex &= ~STATE_REQUIRES_OOZ;
  if (gc->state.vData.qInfo.mode == GR_PARAM_DISABLE)
    paramIndex &= ~STATE_REQUIRES_OOW_FBI;
  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_CLIP_COORDS) {
    if (gc->state.vData.wInfo.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_OOW_FBI;
  }
  if (gc->state.vData.st0Info.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_ST_TMU0;
  if (gc->state.vData.st1Info.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_ST_TMU1;
  if (gc->state.vData.q0Info.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_W_TMU0;
  if (gc->state.vData.q1Info.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_W_TMU1;

  gc->state.paramIndex = paramIndex;

  _grRebuildDataList();

} /* _grUpdateParamIndex */

#if defined(GLIDE_USE_ALT_REGMAP)
  #define RED   Fr_ALT
  #define DRDX  Fdrdx_ALT
  #define DRDY  Fdrdy_ALT
  #define GRN   Fg_ALT
  #define DGDX  Fdgdx_ALT
  #define DGDY  Fdgdy_ALT
  #define BLU   Fb_ALT
  #define DBDX  Fdbdx_ALT
  #define DBDY  Fdbdy_ALT
  #define ALF   Fa_ALT
  #define DADX  Fdadx_ALT
  #define DADY  Fdady_ALT
  #define Z     Fz_ALT
  #define DZDX  Fdzdx_ALT
  #define DZDY  Fdzdy_ALT
  #define S     Fs_ALT
  #define DSDX  Fdsdx_ALT
  #define DSDY  Fdsdy_ALT
  #define T     Ft_ALT
  #define DTDX  Fdtdx_ALT
  #define DTDY  Fdtdy_ALT

  #define W     Fw_ALT
  #define DWDX  Fdwdx_ALT
  #define DWDY  Fdwdy_ALT
#else

  #define RED   Fr
  #define DRDX  Fdrdx
  #define DRDY  Fdrdy
  #define GRN   Fg
  #define DGDX  Fdgdx
  #define DGDY  Fdgdy
  #define BLU   Fb
  #define DBDX  Fdbdx
  #define DBDY  Fdbdy
  #define ALF   Fa
  #define DADX  Fdadx
  #define DADY  Fdady
  #define Z     Fz
  #define DZDX  Fdzdx
  #define DZDY  Fdzdy
  #define S     Fs
  #define DSDX  Fdsdx
  #define DSDY  Fdsdy
  #define T     Ft
  #define DTDX  Fdtdx
  #define DTDY  Fdtdy
  #define W     Fw
  #define DWDX  Fdwdx
  #define DWDY  Fdwdy
#endif

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)

/*---------------------------------------------------------------------------
** _grRebuildDataList
*/
GR_DDFUNC(_grRebuildDataList, void, ( void ))
{
  GR_DCL_GC;
  GR_DCL_HW;
  int curTriSize, params;
  FxU32 gwHeaderNum, gwpSize;            /* header + vertices */
  FxU32 i, packMask=1;
  Sstregs *tmu0;
  Sstregs *tmu1;
    
#ifdef GLIDE_DEBUG
  static char *p_str[] = {"x","y","z","r","g","b","ooz","a","oow",
                            "s0","t0","w0","s1","t1","w1","s2","t2","w2"};
#endif
  
  GDBG_INFO((80,"_grRebuildDataList() paramIndex=0x%x\n", gc->state.paramIndex));
  
  curTriSize = params = 0;
  i = gc->state.paramIndex;

#if GL_X86
  if (_GlideRoot.CPUType == 6) packMask |= 2;
#endif
  
  tmu0 = SST_CHIP(hw,0xE); /* tmu 0,1,2 */
  tmu1 = SST_CHIP(hw,0xC); /* tmu 1,2 */
  
  /* init group write data */
  gwHeaderNum = 0;
  gwpSize     = 8;   /* header + vertices */
  /* !!! fixme - jdt
     this won't ever change, actually, but this is the safest
     place I can think of to init this for now */
  gc->hwDep.sst96Dep.gwCommand = GWH_GEN_ADDRESS(&hw->FvA.x);
  
  /*
     ((((FxU32)((FxU32) &hw->FvA.x) - (FxU32) gc->lfb_ptr) >> 2) | 0x100000 | ( 1L << (31) ));
     */
  
  gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] = 0;
  gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_VTX_BITS;
  
  if (i & STATE_REQUIRES_IT_DRGB) {
    if (gc->state.vData.colorType == GR_FLOAT) {
      /* non packed color */
      /* add 9 to size array for r,drdx,drdy,g,dgdx,dgdy,b,dbdx,dbdy */
      gc->dataList[curTriSize + 0].i     = gc->state.vData.rgbInfo.offset;
      gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->RED;
      gc->dataList[curTriSize + 0].bddr  = 0;
      gc->dataList[curTriSize + 1].i     = gc->state.vData.rgbInfo.offset+GR_COLOR_OFFSET_GREEN;
      gc->dataList[curTriSize + 1].addr  = (float *)(void *)&hw->GRN;
      gc->dataList[curTriSize + 1].bddr  = 0;
      gc->dataList[curTriSize + 2].i     = gc->state.vData.rgbInfo.offset+GR_COLOR_OFFSET_BLUE;
      gc->dataList[curTriSize + 2].addr  = (float *)(void *)&hw->BLU;
      gc->dataList[curTriSize + 2].bddr  = 0;
      curTriSize += 3;
      params += 3;
      
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_RGB_BITS;
      gwpSize += 9;
    }
    else {
      /* packed color, argb */
      /* add 9 to size array for r,drdx,drdy,g,dgdx,dgdy,b,dbdx,dbdy */
      gc->dataList[curTriSize + 0].i     = gc->state.vData.pargbInfo.offset;
      gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->RED;
      gc->dataList[curTriSize + 0].bddr  = gc->state.vData.pargbInfo.offset + GR_PARGB_R;
      gc->dataList[curTriSize + 1].i     = gc->state.vData.pargbInfo.offset;
      gc->dataList[curTriSize + 1].addr  = (float *)(void *)&hw->GRN;
      gc->dataList[curTriSize + 1].bddr  = gc->state.vData.pargbInfo.offset + GR_PARGB_G;
      gc->dataList[curTriSize + 2].i     = gc->state.vData.pargbInfo.offset;
      gc->dataList[curTriSize + 2].addr  = (float *)(void *)&hw->BLU;
      gc->dataList[curTriSize + 2].bddr  = gc->state.vData.pargbInfo.offset + GR_PARGB_B;
      curTriSize += 3;
      params += 3;
      
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_RGB_BITS;
      gwpSize += 9;
    }
  }
  
  if (i & STATE_REQUIRES_OOZ) {
    gc->dataList[curTriSize + 0].i     = gc->state.vData.zInfo.offset;
    gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->Z;
    gc->dataList[curTriSize + 0].bddr  = 0;
    curTriSize += 1;
    params += 1;

    gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_Z_BITS;
    gwpSize += 3;
  }
  
  if (i & STATE_REQUIRES_IT_ALPHA) {
    if (gc->state.vData.colorType == GR_FLOAT) {
      gc->dataList[curTriSize + 0].i     = gc->state.vData.aInfo.offset;
      gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->ALF;
      gc->dataList[curTriSize + 0].bddr  = 0;
      curTriSize += 1;
      params += 1;
      
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_A_BITS;
      gwpSize += 3;
    }
    else {
      /* packed color, argb */
      gc->dataList[curTriSize + 0].i     = gc->state.vData.pargbInfo.offset;
      gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->ALF;
      gc->dataList[curTriSize + 0].bddr  = gc->state.vData.pargbInfo.offset + GR_PARGB_A;
      curTriSize += 1;
      params += 1;
      
      gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_A_BITS;
      gwpSize += 3;
    }
  }
  
  /* TMU1 --------------------------------- */
  /* always output to ALL chips, saves from having to change CHIP field */
  if (i & STATE_REQUIRES_ST_TMU0) {
    gc->dataList[curTriSize + 0].i     = gc->state.vData.st0Info.offset;
    gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->S;
    gc->dataList[curTriSize + 0].bddr  = 0;
    gc->dataList[curTriSize + 1].i     = gc->state.vData.st0Info.offset + GR_TEXTURE_OFFSET_T;
    gc->dataList[curTriSize + 1].addr  = (float *)(void *)&hw->T;
    gc->dataList[curTriSize + 1].bddr  = 0;
    curTriSize += 2;
    params += 2;

    gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_ST_BITS;
    gwpSize += 6;
  }
  
  /* we squeeze FBI.OOW in here for sequential writes in the simple case */
  if (i & STATE_REQUIRES_OOW_FBI) {
    gc->dataList[curTriSize + 0].i     = gc->state.vData.qInfo.offset;
    gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->W;
    gc->dataList[curTriSize + 0].bddr  = 0;
    curTriSize += 1;
    params += 1;

    gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_W_BITS;
    gwpSize += 3;
  }

  /* NOTE: this is the first */
  if (i & STATE_REQUIRES_W_TMU0) {
    /* We do this a bit differently for grouped writes */
    gc->dataList[curTriSize + 0].i     = packMask;
    gc->dataList[curTriSize + 0].addr  =
      (float *)(void *) GWH_GEN_ADDRESS(&tmu0->FvA.x);
    gc->dataList[curTriSize + 0].bddr  = 0;
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE)    
      gc->dataList[curTriSize + 1].i     = gc->state.vData.q0Info.offset;
    else {
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        gc->dataList[curTriSize + 1].i     = gc->state.vData.qInfo.offset;
      else
        gc->dataList[curTriSize + 1].i     = gc->state.vData.wInfo.offset;
    }
    gc->dataList[curTriSize + 1].addr  = NULL;
    gc->dataList[curTriSize + 1].bddr  = 0;
    
    if (gwpSize & 1)
      ++gwpSize;              /* Justify your love */
    gwpSize += 5;             /* 2 header + grads */
    
    ++gwHeaderNum;
    gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_W_BITS;
    curTriSize += 2;
    params += 1;
  }
  
  gc->dataList[curTriSize++].i = 0;     /* terminate the list with 0,*      */
  /* followed by the FtriangleCMD reg */
  gc->dataList[curTriSize].i = packMask;/* encode P6 flag here for asm code */
  gc->dataList[curTriSize].addr = (float *)(void *)&hw->FtriangleCMD;
  
  /* 6 X,Y values plus AREA = 7, plus parameters */
  if (gwHeaderNum) {
    if (gwpSize & 1)
      ++gwpSize;                /* justify your love */

    /* If we've had a chip field change above then we need to start a
     ** new packet for the triangle command, so we don't put it in the
     ** original */
    gwpSize +=4;        /* The triangleCMD GWP constitutes 4 DWORDS */
  } else {
    gc->hwDep.sst96Dep.gwHeaders[gwHeaderNum] |= GWH_FTRIANGLECMD_BIT;
    gwpSize++;                    /* Triangle Command */

    if (gwpSize & 1)
      ++gwpSize;                /* justify the fifo */
  }    

  _GlideRoot.curTriSize = gwpSize << 2; /* GR.curTriSize in bytes */

  /* Need to know tri size without gradients for planar polygons */
  _GlideRoot.curTriSizeNoGradient = _GlideRoot.curTriSize - (params<<3);
  
#ifdef GDBG_INFO_ON
  GDBG_INFO((282, "DataList\n"));
  for (i = 0; gc->dataList[i].i; i++) {
    GDBG_INFO((282,"    dataList[%d] = %2d 0x%x [%s]\n",
               i,gc->dataList[i].i>>2,gc->dataList[i].addr,
               (gc->dataList[i].i & 1) ? "packer" : p_str[gc->dataList[i].i>>2]));
  }
#endif

} /* _grRebuildDataList */

#else  /* It's SST1 */

/*---------------------------------------------------------------------------
** _grRebuildDataList
*/
GR_DDFUNC(_grRebuildDataList, void, ( void ))
{
  GR_DCL_GC;
  GR_DCL_HW;
  int curTriSize, params, packerFlag;
  FxU32 i, packMask=1;
  Sstregs *tmu0;
  Sstregs *tmu1;
    
#ifdef GLIDE_DEBUG
  static char *p_str[] = {"x","y","z","r","g","b","ooz","a","oow",
                            "s0","t0","w0","s1","t1","w1","s2","t2","w2"};
#endif
  
  GDBG_INFO((80,"_grRebuildDataList() paramIndex=0x%x\n",
             gc->state.paramIndex));
  
  curTriSize = params = 0;
  i = gc->state.paramIndex;
#if GL_X86
  if (_GlideRoot.CPUType == 6) packMask |= 2;
#endif
#ifdef GLIDE_USE_ALT_REGMAP
  hw = SST_WRAP(hw,128);        /* wrap 128-257 are alternate
                                   register mappings */
#endif /* GLIDE_USE_ALT_REGMAP */
  
  tmu0 = SST_CHIP(hw,0xE); /* tmu 0,1,2 */
  tmu1 = SST_CHIP(hw,0xC); /* tmu 1,2 */
  
  if (i & STATE_REQUIRES_IT_DRGB) {
    if (gc->state.vData.colorType == GR_FLOAT) {
      /* non packed color */
      /* add 9 to size array for r,drdx,drdy,g,dgdx,dgdy,b,dbdx,dbdy */
      gc->dataList[curTriSize + 0].i     = gc->state.vData.rgbInfo.offset;
      gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->RED;
      gc->dataList[curTriSize + 0].bddr  = 0;
      gc->dataList[curTriSize + 1].i     = gc->state.vData.rgbInfo.offset+GR_COLOR_OFFSET_GREEN;
      gc->dataList[curTriSize + 1].addr  = (float *)(void *)&hw->GRN;
      gc->dataList[curTriSize + 1].bddr  = 0;
      gc->dataList[curTriSize + 2].i     = gc->state.vData.rgbInfo.offset+GR_COLOR_OFFSET_BLUE;
      gc->dataList[curTriSize + 2].addr  = (float *)(void *)&hw->BLU;
      gc->dataList[curTriSize + 2].bddr  = 0;
      curTriSize += 3;
      params += 3;
    }
    else {
      /* packed color, argb */
      /* add 9 to size array for r,drdx,drdy,g,dgdx,dgdy,b,dbdx,dbdy */
      gc->dataList[curTriSize + 0].i     = gc->state.vData.pargbInfo.offset;
      gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->RED;
      gc->dataList[curTriSize + 0].bddr  = gc->state.vData.pargbInfo.offset + GR_PARGB_R;
      gc->dataList[curTriSize + 1].i     = gc->state.vData.pargbInfo.offset;
      gc->dataList[curTriSize + 1].addr  = (float *)(void *)&hw->GRN;
      gc->dataList[curTriSize + 1].bddr  = gc->state.vData.pargbInfo.offset + GR_PARGB_G;
      gc->dataList[curTriSize + 2].i     = gc->state.vData.pargbInfo.offset;
      gc->dataList[curTriSize + 2].addr  = (float *)(void *)&hw->BLU;
      gc->dataList[curTriSize + 2].bddr  = gc->state.vData.pargbInfo.offset + GR_PARGB_B;
      curTriSize += 3;
      params += 3;
    }
  }
  
  if (i & STATE_REQUIRES_OOZ) {
    gc->dataList[curTriSize + 0].i     = gc->state.vData.zInfo.offset;
    gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->Z;
    gc->dataList[curTriSize + 0].bddr  = 0;
    curTriSize += 1;
    params += 1;
  }
  
  if (i & STATE_REQUIRES_IT_ALPHA) {
    if (gc->state.vData.colorType == GR_FLOAT) {
      gc->dataList[curTriSize + 0].i     = gc->state.vData.aInfo.offset;
      gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->ALF;
      gc->dataList[curTriSize + 0].bddr  = 0;
      curTriSize += 1;
      params += 1;
    }
    else {
      /* packed color, argb */
      gc->dataList[curTriSize + 0].i     = gc->state.vData.pargbInfo.offset;
      gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->ALF;
      gc->dataList[curTriSize + 0].bddr  = gc->state.vData.pargbInfo.offset + GR_PARGB_A;
      curTriSize += 1;
      params += 1;
    }
  }
  
  /* TMU1 --------------------------------- */
  /* always output to ALL chips, saves from having to change CHIP field */
  if (i & STATE_REQUIRES_ST_TMU0) {
    gc->dataList[curTriSize + 0].i     = gc->state.vData.st0Info.offset;
    gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->S;
    gc->dataList[curTriSize + 0].bddr  = 0;
    gc->dataList[curTriSize + 1].i     = gc->state.vData.st0Info.offset 
      + GR_TEXTURE_OFFSET_T;
    gc->dataList[curTriSize + 1].addr  = (float *)(void *)&hw->T;
    gc->dataList[curTriSize + 1].bddr  = 0;
    curTriSize += 2;
    params += 2;
  }
  
  /* we squeeze FBI.OOW in here for sequential writes in the simple case */
  if (i & STATE_REQUIRES_OOW_FBI) {
    gc->dataList[curTriSize + 0].i     = gc->state.vData.qInfo.offset;
    gc->dataList[curTriSize + 0].addr  = (float *)(void *)&hw->W;
    gc->dataList[curTriSize + 0].bddr  = 0;
    curTriSize += 1;
    params += 1;
  }
  /* NOTE: this is the first */
  if (i & STATE_REQUIRES_W_TMU0) {
    gc->dataList[curTriSize + 0].i     =  packMask;
    gc->dataList[curTriSize + 0].addr  = (float *)_GlideRoot.packerFixAddress;
    gc->dataList[curTriSize + 0].bddr  = 0;
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE)    
      gc->dataList[curTriSize + 1].i     = gc->state.vData.q0Info.offset;
    else {
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        gc->dataList[curTriSize + 1].i     = gc->state.vData.qInfo.offset;
      else
        gc->dataList[curTriSize + 1].i     = gc->state.vData.wInfo.offset;
    }
    gc->dataList[curTriSize + 1].addr  = (float *)(void *)&tmu0->W;
    gc->dataList[curTriSize + 1].bddr  = 0;
    curTriSize += 2;
    params += 1;
  }
  
  /* TMU1 --------------------------------- */
  if (i & STATE_REQUIRES_ST_TMU1) {
    gc->dataList[curTriSize + 0].i     = packMask;
    gc->dataList[curTriSize + 0].addr  = (float *)_GlideRoot.packerFixAddress;
    packerFlag = 0;
    gc->dataList[curTriSize + 0].bddr  = 0;
    gc->dataList[curTriSize + 1].i     = gc->state.vData.st1Info.offset;
    gc->dataList[curTriSize + 1].addr  = (float *)(void *)&tmu1->S;
    gc->dataList[curTriSize + 1].bddr  = 0;
    gc->dataList[curTriSize + 2].i     = gc->state.vData.st1Info.offset + GR_TEXTURE_OFFSET_T;
    gc->dataList[curTriSize + 2].addr  = (float *)(void *)&tmu1->T;
    gc->dataList[curTriSize + 2].bddr  = 0;
    curTriSize += 3;
    params += 2;
  }
  else packerFlag = 1;
  
  if (i & STATE_REQUIRES_W_TMU1) {
    if (packerFlag) {
      gc->dataList[curTriSize + 0].i     = packMask;
      gc->dataList[curTriSize + 0].addr  = (float *)_GlideRoot.packerFixAddress;
      gc->dataList[curTriSize + 0].bddr  = 0;
      curTriSize += 1;
    }
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE)    
      gc->dataList[curTriSize + 0].i     = gc->state.vData.q1Info.offset;
    else {
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        gc->dataList[curTriSize + 0].i     = gc->state.vData.qInfo.offset;
      else
        gc->dataList[curTriSize + 0].i     = gc->state.vData.wInfo.offset;
    }
    gc->dataList[curTriSize + 0].addr  = (float *)(void *)&tmu1->W;
    gc->dataList[curTriSize + 0].bddr  = 0;
    curTriSize += 1;
    params += 1;
  }
#if (GLIDE_NUM_TMU > 2)
  error WRITE THIS CODE;
#endif
  
  /* if last write was not to chip 0 */
  if ( SST_CHIP_MASK & (FxU32)gc->dataList[curTriSize-1].addr ) {
    /* flag write as a packer bug fix  */
    gc->dataList[curTriSize].i     = packMask; 
    gc->dataList[curTriSize].addr  = (float *)_GlideRoot.packerFixAddress;
    gc->dataList[curTriSize].bddr  = 0;
    curTriSize++;
  }

  
  gc->dataList[curTriSize++].i = 0; /* terminate the list with 0,*      */
                                /* followed by the FtriangleCMD reg */
  gc->dataList[curTriSize].i = packMask;/* encode P6 flag here for asm code */
  gc->dataList[curTriSize].addr = (float *)(void *)&hw->FtriangleCMD;
  
  /* 6 X,Y values plus AREA = 7, plus parameters */
  _GlideRoot.curTriSize = (6 + curTriSize + (params<<1)) <<2;

  /* Need to know tri size without gradients for planar polygons */
  _GlideRoot.curTriSizeNoGradient = _GlideRoot.curTriSize - (params<<3);
  
#ifdef GDBG_INFO_ON
  GDBG_INFO((282, "DataList\n"));
  for (i = 0; gc->dataList[i].i; i++) {
    GDBG_INFO((282,"    dataList[%d] = %2d 0x%x [%s]\n",
               i,gc->dataList[i].i>>2,gc->dataList[i].addr,
               (gc->dataList[i].i & 1) ? "packer" :
               p_str[gc->dataList[i].i>>2])); 
  }
#endif

} /* _grRebuildDataList */

#endif

#if GLIDE_MULTIPLATFORM
/*
   _grInitializeGCFuncs - initialize the gcFuncs Structure for this
        device.
*/
GR_DDFUNC(_grInitializeGCFuncs, void, (GrGC *gc))
{
  
  gc->gcFuncs._grColorCombineDelta0Mode = (void *)
    GR_DDNAME(_grColorCombineDelta0Mode); 
  gc->gcFuncs._grRebuildDataList = (void *) GR_DDNAME(_grRebuildDataList);
  gc->gcFuncs._grTexDetailControl = (void *) GR_DDNAME(_grTexDetailControl);
  gc->gcFuncs._grTexDownloadNccTable = (void *)
    GR_DDNAME(_grTexDownloadNccTable); 
  gc->gcFuncs._grTexDownloadPalette = (void *)
    GR_DDNAME(_grTexDownloadPalette); 
  gc->gcFuncs._gumpTexCombineFunction = (void *)
    GR_DDNAME(_gumpTexCombineFunction); 
  gc->gcFuncs._grUpdateParamIndex = (void *)
    GR_DDNAME(_grUpdateParamIndex); 
  gc->gcFuncs._trisetup = (void *) GR_DDNAME(_trisetup); 
  gc->gcFuncs._trisetup_nogradients = (void *)
    GR_DDNAME(_trisetup_nogradients); 
  gc->gcFuncs.grAADrawLine = (void *) GR_DDNAME(grAADrawLine); 
  gc->gcFuncs.grAADrawPoint = (void *) GR_DDNAME(grAADrawPoint); 
  gc->gcFuncs.grAADrawPolygon = (void *) GR_DDNAME(grAADrawPolygon); 
  gc->gcFuncs.grAADrawPolygonVertexList = (void *)
    GR_DDNAME(grAADrawPolygonVertexList); 
  gc->gcFuncs.grAADrawTriangle = (void *) GR_DDNAME(grAADrawTriangle); 
  gc->gcFuncs.grAlphaBlendFunction = (void *)
    GR_DDNAME(grAlphaBlendFunction); 
  gc->gcFuncs.grAlphaCombine = (void *) GR_DDNAME(grAlphaCombine); 
  gc->gcFuncs.grAlphaControlsITRGBLighting = (void *)
    GR_DDNAME(grAlphaControlsITRGBLighting); 
  gc->gcFuncs.grAlphaTestFunction = (void *)
    GR_DDNAME(grAlphaTestFunction); 
  gc->gcFuncs.grAlphaTestReferenceValue = (void *)
    GR_DDNAME(grAlphaTestReferenceValue); 
  gc->gcFuncs.grBufferClear = (void *) GR_DDNAME(grBufferClear); 
  gc->gcFuncs.grBufferNumPending = (void *)
    GR_DDNAME(grBufferNumPending); 
  gc->gcFuncs.grBufferSwap = (void *) GR_DDNAME(grBufferSwap); 
  gc->gcFuncs.grChromakeyMode = (void *) GR_DDNAME(grChromakeyMode); 
  gc->gcFuncs.grChromakeyValue = (void *) GR_DDNAME(grChromakeyValue); 
  gc->gcFuncs.grClipWindow = (void *) GR_DDNAME(grClipWindow); 
  gc->gcFuncs.grColorCombine = (void *) GR_DDNAME(grColorCombine); 
  gc->gcFuncs.grColorMask = (void *) GR_DDNAME(grColorMask); 
  gc->gcFuncs.grConstantColorValue = (void *)
    GR_DDNAME(grConstantColorValue); 
  gc->gcFuncs.grConstantColorValue4 = (void *)
    GR_DDNAME(grConstantColorValue4); 
  gc->gcFuncs.grCullMode = (void *) GR_DDNAME(grCullMode); 
  gc->gcFuncs.grDepthBiasLevel = (void *) GR_DDNAME(grDepthBiasLevel); 
  gc->gcFuncs.grDepthBufferFunction = (void *)
    GR_DDNAME(grDepthBufferFunction); 
  gc->gcFuncs.grDepthBufferMode = (void *) GR_DDNAME(grDepthBufferMode);
  
  gc->gcFuncs.grDepthMask = (void *) GR_DDNAME(grDepthMask); 
  gc->gcFuncs.grDisableAllEffects = (void *)
    GR_DDNAME(grDisableAllEffects); 
  gc->gcFuncs.grDitherMode = (void *) GR_DDNAME(grDitherMode); 
  gc->gcFuncs.grDrawLine = (void *) GR_DDNAME(grDrawLine); 
  gc->gcFuncs.grDrawPlanarPolygon = (void *)
    GR_DDNAME(grDrawPlanarPolygon); 
  gc->gcFuncs.grDrawPlanarPolygonVertexList = (void *)
    GR_DDNAME(grDrawPlanarPolygonVertexList); 
  gc->gcFuncs.grDrawPoint = (void *) GR_DDNAME(grDrawPoint); 
  gc->gcFuncs.grDrawPolygon = (void *) GR_DDNAME(grDrawPolygon); 
  gc->gcFuncs.grDrawPolygonVertexList = (void *)
    GR_DDNAME(grDrawPolygonVertexList); 
  gc->gcFuncs.grDrawTriangle = (void *) GR_DDNAME(grDrawTriangle); 
  gc->gcFuncs.grFogColorValue = (void *) GR_DDNAME(grFogColorValue); 
  gc->gcFuncs.grFogMode = (void *) GR_DDNAME(grFogMode); 
  gc->gcFuncs.grFogTable = (void *) GR_DDNAME(grFogTable); 
  gc->gcFuncs.grGammaCorrectionValue = (void *)
    GR_DDNAME(grGammaCorrectionValue); 
  gc->gcFuncs.grGlideSetState = (void *) GR_DDNAME(grGlideSetState); 
  gc->gcFuncs.grGlideShutdown = (void *) GR_DDNAME(grGlideShutdown); 
  gc->gcFuncs.grLfbConstantAlpha = (void *)
    GR_DDNAME(grLfbConstantAlpha); 
  gc->gcFuncs.grLfbConstantDepth = (void *)
    GR_DDNAME(grLfbConstantDepth); 
  gc->gcFuncs.grLfbLock = (void *)GR_DDNAME(grLfbLock);
  gc->gcFuncs.grLfbUnlock = (void*)GR_DDNAME(grLfbUnlock);
  gc->gcFuncs.grLfbWriteColorFormat = (void *)
    GR_DDNAME(grLfbWriteColorFormat); 
  gc->gcFuncs.grLfbWriteColorSwizzle = (void *)
    GR_DDNAME(grLfbWriteColorSwizzle); 
  gc->gcFuncs.grRenderBuffer = (void *) GR_DDNAME(grRenderBuffer); 
  gc->gcFuncs.grSstConfigPipeline = (void *)
    GR_DDNAME(grSstConfigPipeline); 
  gc->gcFuncs.grSstIdle = (void *) GR_DDNAME(grSstIdle); 
  gc->gcFuncs.grSstIsBusy = (void *) GR_DDNAME(grSstIsBusy); 
  gc->gcFuncs.grSstOpen = (void *) GR_DDNAME(grSstOpen); 
  gc->gcFuncs.grSstOrigin = (void *) GR_DDNAME(grSstOrigin); 
  gc->gcFuncs.grSstPassthruMode = (void *) GR_DDNAME(grSstPassthruMode);
  
  gc->gcFuncs.grSstPerfStats = (void *) GR_DDNAME(grSstPerfStats); 
  gc->gcFuncs.grSstResetPerfStats = (void *) GR_DDNAME(grSstResetPerfStats);
  gc->gcFuncs.grSstStatus = (void *) GR_DDNAME(grSstStatus); 
  gc->gcFuncs.grSstVRetraceOn = (void *) GR_DDNAME(grSstVRetraceOn); 
  gc->gcFuncs.grSstVideoLine = (void *) GR_DDNAME(grSstVideoLine); 
  gc->gcFuncs.grTexClampMode = (void *) GR_DDNAME(grTexClampMode); 
  gc->gcFuncs.grTexCombine = (void *) GR_DDNAME(grTexCombine); 
  gc->gcFuncs.grTexDownloadMipMapLevelPartial = (void *)
    GR_DDNAME(grTexDownloadMipMapLevelPartial); 
  gc->gcFuncs.grTexFilterMode = (void *) GR_DDNAME(grTexFilterMode); 
  gc->gcFuncs.grTexLodBiasValue = (void *) GR_DDNAME(grTexLodBiasValue);
  gc->gcFuncs.grTexMipMapMode = (void *) GR_DDNAME(grTexMipMapMode);
  gc->gcFuncs.grTexMultibase = (void *) GR_DDNAME(grTexMultibase); 
  gc->gcFuncs.grTexMultibaseAddress = (void *)
    GR_DDNAME(grTexMultibaseAddress); 
  gc->gcFuncs.grTexNCCTable = (void *) GR_DDNAME(grTexNCCTable); 
  gc->gcFuncs.grTexSource = (void *) GR_DDNAME(grTexSource); 
  gc->gcFuncs.guMPDrawTriangle = (void *) GR_DDNAME(guMPDrawTriangle); 
  gc->gcFuncs.guTexSource = (void *) GR_DDNAME(guTexSource);
  gc->gcFuncs.ConvertAndDownloadRle=(void *) GR_DDNAME(ConvertAndDownloadRle); 
  gc->gcFuncs.grCheckForRoom=(void *) GR_DDNAME(grCheckForRoom); 
} /* _grInitializeGCFuncs */
#endif
