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
** Revision 1.2.2.6  2007/05/19 08:37:25  koolsmoky
** packed rgb fixes
** force 4x4 dither with alpha dither subtraction
**
** Revision 1.2.2.5  2005/01/22 14:52:02  koolsmoky
** enabled packed argb for cmd packet type 3
**
** Revision 1.2.2.4  2004/12/27 20:46:37  koolsmoky
** added dll entry point to call grGlideShutdown when a process is detached
**
** Revision 1.2.2.3  2004/12/23 21:03:14  koolsmoky
** swapinterval
**
** Revision 1.2.2.2  2004/12/23 20:45:56  koolsmoky
** converted to nasm syntax
** added x86 asm, 3dnow! triangle and mmx, 3dnow! texture download optimizations
**
** Revision 1.2.2.1  2004/12/12 15:24:40  koolsmoky
** grDitherMode(): force 2x2 dithering for 4x1 dither
** grBufferSwap(): enable vsync and set swapbufferinterval to 0 for tripple buffering.
**
** Revision 1.2  2000/10/03 18:28:33  mercury
** 003-clean_up_cvg-000, cvg tree cleanup.
**
** Revision 1.1.1.1  1999/12/07 21:49:10  joseph
** Initial checkin into SourceForge.
**
** 
** 168   7/02/98 6:59p Peter
** Fixed merge problem from previous checkin
** 
** 167   6/30/98 6:08p Jeske
** fixed bug where we tried to setup MTRRs on old (<p6) systems which
** didn't have them. 
** 
** 165   6/11/98 3:59p Peter
** sli buffer swap needs to be max of app setting vs forcing sync
** 
** 164   6/03/98 6:31p Peter
** clearing single scanlines
** 
** 163   5/28/98 2:52p Peter
** fixed wrong checking w/ h3
** 
** 162   5/27/98 9:41a Peter
** grBufferClear is constrained by clipping
** 
** 161   5/18/98 12:16p Peter
** culling enabling
** 
** 160   4/16/98 3:54p Peter
** 1x1 lod fix, sorry John
** 
** 159   4/08/98 3:52p Peter
** fixed resetting of unused tmu to small lod
** 
** 158   4/01/98 1:51p Peter
** fixed resetting unused tmu muckage/sli origin thing vs grRenderBuffer
** 
** 157   3/31/98 6:09p Peter
** sli origin everywhere (I think) and grLfbReadRegion/grRenderBuffer vs
** triple buffering
** 
** 156   3/30/98 4:56p Peter
** fog table monotonicity
** 
** 155   3/29/98 1:07p Peter
** removed shutdown cruft and yet another sli origin thing
** 
** 154   3/23/98 4:21p Jdt
** Fixed texture state validation bug in grColorCombine
** 
** 153   3/20/98 11:02a Peter
** inactive tmu reset
** 
** 152   3/04/98 9:10p Peter
** properly restore fbzMode for the sli swapping thing
** 
** 151   3/03/98 9:37p Peter
** more sli origin fun
** 
** 150   3/02/98 7:22p Peter
** more crybaby stuff
** 
** 149   2/21/98 8:33a Peter
** mixed case ini vars
** 
** 148   2/20/98 5:31p Peter
** crybaby glide
** 
** 147   2/20/98 2:16p Peter
** shutting down hw should clear hwInit and open
** 
** 146   2/20/98 11:00a Peter
** removed glide3 from glid2 tree
** 
** 145   2/20/98 9:05a Peter
** removed remnants of comdex grot
** 
** 144   2/17/98 12:40p Peter
** fog table fix
** 
** 143   2/12/98 3:40p Peter
** single buffering for opengl
** 
** 142   2/11/98 5:23p Peter
** fixed grRenderBuffer vs blit clear
** 
** 141   1/30/98 4:19p Peter
** sli/y-origin blit clear
** 
** 140   1/23/98 3:02p Peter
** uswc nightmare
** 
** 139   1/19/98 1:43p Atai
** fixed non-debug mode assignement
** 
** 138   1/19/98 11:00a Atai
** remove assignment before validate the state
 * 
 * 137   1/15/98 1:12p Peter
 * only one culler please
 * 
 * 136   1/13/98 7:48p Atai
 * fixed gu3dfGetInfo, grBufferClear, and GrState size
 * 
 * 135   1/13/98 12:42p Atai
 * fixed grtexinfo, grVertexLayout, and draw triangle
 * 
 * 134   1/09/98 7:29p Atai
 * fixed grBufferSwap for glide3
 * 
 * 133   1/08/98 9:25p Peter
 * infinite recurrsion in debugging assert
 * 
 * 132   1/08/98 9:23p Peter
 * fixed macro muckage
 * 
 * 131   1/08/98 7:09p Peter
 * real hw stuff modulo makefile change
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

#if GLIDE_HW_TRI_SETUP
static void
_grUpdateTriPacketHdr(FxU32 paramMask, 
                      const GrCullMode_t mode);
#endif /* GLIDE_HW_TRI_SETUP */

/*---------------------------------------------------------------------------
** grAlphaBlendFunction
**
** GMT: BUG if grColorMask() turns off alphaplanes then destination alpha
**          blending wont work!
*/

GR_STATE_ENTRY(grAlphaBlendFunction, void, 
               (GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df, 
                GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df))
{
#define FN_NAME "grAlphaBlendFunction"
  FxU32 alphamode;
  GR_BEGIN_NOFIFOCHECK("grAlphaBlendFunction", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d,%d,%d,%d)\n",
                 rgb_sf, rgb_df, alpha_sf, alpha_df);

  alphamode = gc->state.fbi_config.alphaMode;
  if (alpha_sf != GR_BLEND_ONE && alpha_sf != GR_BLEND_ZERO) {
    GR_CHECK_W(myName, 1, "unsupported alpha source blend function");
    alpha_sf = GR_BLEND_ONE;
  }

  if (alpha_df != GR_BLEND_ONE && alpha_df != GR_BLEND_ZERO) {
    GR_CHECK_W(myName, 1, "unsupported alpha destination blend function");
    alpha_df = GR_BLEND_ZERO;
  }

  if (rgb_sf   == GR_BLEND_ONE && rgb_df   == GR_BLEND_ZERO &&
      alpha_sf == GR_BLEND_ONE && alpha_df == GR_BLEND_ZERO)
    alphamode &= ~SST_ENALPHABLEND;
  else
    alphamode |=  SST_ENALPHABLEND;
  
  alphamode &= ~(SST_RGBSRCFACT | SST_RGBDSTFACT | SST_ASRCFACT | SST_ADSTFACT);
  
  alphamode |= ((((FxU32) rgb_sf) << SST_RGBSRCFACT_SHIFT) |
                (((FxU32) rgb_df) << SST_RGBDSTFACT_SHIFT) |
                (((FxU32) alpha_sf) << SST_ASRCFACT_SHIFT) |
                (((FxU32) alpha_df) << SST_ADSTFACT_SHIFT));

  gc->state.fbi_config.alphaMode = alphamode;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, alphaMode, alphamode);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grAlphaBlendFunction */

/*---------------------------------------------------------------------------
** grAlphaCombine
*/

GR_STATE_ENTRY(grAlphaCombine, void, 
               (GrCombineFunction_t function, GrCombineFactor_t factor, 
                GrCombineLocal_t local, GrCombineOther_t other, FxBool invert))
{
#define FN_NAME "grAlphaCombine"
  FxU32 fbzColorPath;
  FxU32 oldTextureEnabled;

  GR_BEGIN_NOFIFOCHECK("grAlphaCombine",85);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d,%d,%d)\n",function,factor,local,other,invert);
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
  fbzColorPath &= ~(SST_ENTEXTUREMAP |
                    SST_ASELECT |
                    SST_ALOCALSELECT |
                    SST_CCA_ZERO_OTHER |
                    SST_CCA_SUB_CLOCAL |
                    SST_CCA_MSELECT |
                    SST_CCA_REVERSE_BLEND |
                    SST_CCA_ADD_CLOCAL |
                    SST_CCA_ADD_ALOCAL |
                    SST_CCA_INVERT_OUTPUT);
  
  /* setup reverse blending first, then strip off the the high bit */
  if ((factor & 0x8) == 0)
    fbzColorPath |= SST_CCA_REVERSE_BLEND;
  factor &= 0x7;

  /* NOTE: we use boolean OR instead of logical to avoid branches */
  gc->state.ac_requires_texture = ((factor == GR_COMBINE_FACTOR_TEXTURE_ALPHA) |
                                   (other == GR_COMBINE_OTHER_TEXTURE));
  gc->state.ac_requires_it_alpha = ((local == GR_COMBINE_LOCAL_ITERATED) |
                                    (other == GR_COMBINE_OTHER_ITERATED));
  
  /* setup scale factor bits */
  fbzColorPath |= factor << SST_CCA_MSELECT_SHIFT;

  /* setup local color bits */
  fbzColorPath |= local << SST_ALOCALSELECT_SHIFT;
  
  /* setup other color bits */
  fbzColorPath |= other << SST_ASELECT_SHIFT;
  
  
  /* setup invert output bits */
  if (invert)
    fbzColorPath |= SST_CCA_INVERT_OUTPUT;
  
  /* setup core color combine unit bits */
  switch (function) {
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
  if (gc->state.cc_requires_texture || gc->state.ac_requires_texture)
    fbzColorPath |= SST_ENTEXTUREMAP;
  gc->state.fbi_config.fbzColorPath = fbzColorPath;

  {
    const FxBool texTransP = (oldTextureEnabled != (fbzColorPath & SST_ENTEXTUREMAP));

    GR_SET_EXPECTED_SIZE(sizeof(FxU32) + (texTransP ? sizeof(FxU32) : 0),
                         1 + texTransP);

    /* transition into/out of texturing ... add nopCMD */
    if (texTransP) GR_SET(BROADCAST_ID, hw, nopCMD, 0);
    GR_SET(BROADCAST_ID, hw, fbzColorPath,  fbzColorPath);
    GR_CHECK_SIZE();
  }

  /* setup paramIndex bits */
  _grUpdateParamIndex();
#undef FN_NAME
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
#define FN_NAME "grAlphaControlsITRGBLighting"
  FxU32 fbzColorPath;
  GR_BEGIN_NOFIFOCHECK("grAlphaControlsITRGBLighting", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", enable);

  fbzColorPath = gc->state.fbi_config.fbzColorPath;
  if (enable) {
    fbzColorPath |= SST_LOCALSELECT_OVERRIDE_WITH_ATEX;
  } else {
    fbzColorPath &= ~SST_LOCALSELECT_OVERRIDE_WITH_ATEX;
  }

  gc->state.fbi_config.fbzColorPath = fbzColorPath;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzColorPath, fbzColorPath);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grAlphaControlsITRGBLighting() */

/*---------------------------------------------------------------------------
** grAlphaTestFunction
*/

GR_STATE_ENTRY(grAlphaTestFunction, void, (GrCmpFnc_t fnc))
{
#define FN_NAME "grAlphaTestFunction"
  FxU32 alphamode;
  GR_BEGIN_NOFIFOCHECK("grAlphaTestFunction", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",fnc);

  alphamode = gc->state.fbi_config.alphaMode;
  alphamode &= ~(SST_ALPHAFUNC | SST_ENALPHAFUNC);
  if (fnc != GR_CMP_ALWAYS)
    alphamode |= ((fnc << SST_ALPHAFUNC_SHIFT) | SST_ENALPHAFUNC);

  gc->state.fbi_config.alphaMode = alphamode;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, alphaMode, alphamode);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grAlphaTestFunction */

/*---------------------------------------------------------------------------
** grAlphaTestReferenceValue
*/

GR_STATE_ENTRY(grAlphaTestReferenceValue, void, (GrAlpha_t value))
{
#define FN_NAME "grAlphaTestReferenceValue"
  FxU32 alphamode;
  GR_BEGIN_NOFIFOCHECK("grAlphaTestReferenceValue", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",value);

  alphamode = gc->state.fbi_config.alphaMode;
  alphamode &= ~SST_ALPHAREF;
  alphamode |= (((FxU32) value) << SST_ALPHAREF_SHIFT);

  gc->state.fbi_config.alphaMode = alphamode;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, alphaMode,  alphamode);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grAlphaTestReferenceValue */

/*---------------------------------------------------------------------------
** grBufferClear
*/

GR_ENTRY(grBufferClear, void, (GrColor_t color, GrAlpha_t alpha, FxU16 depth))
{
#define FN_NAME "grBufferClear"
  GR_BEGIN_NOFIFOCHECK("grBufferClear", 86);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x,0x%x,0x%x)\n", color, alpha, depth);

  {
    const GrColor_t oldc1 = gc->state.fbi_config.color1;
    const FxU32 oldzacolor = gc->state.fbi_config.zaColor;
    const FxU32 fbzMode = gc->state.fbi_config.fbzMode;
    FxU32 zacolor = oldzacolor;
    FxBool doneP = FXFALSE;
    FxBool doColorP = ((fbzMode & SST_RGBWRMASK) != 0);
    FxBool doAuxP = FXFALSE;

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR
    const FxU32 clipBottomTop = gc->state.fbi_config.clipBottomTop;
    const FxU32 bottomOriginP = ((fbzMode & SST_YORIGIN) == SST_YORIGIN);
    FxBool didClipP = FXFALSE;
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR */

    /* Setup source registers */
    if (doColorP) _grSwizzleColor(&color);
    if ((fbzMode & SST_ZAWRMASK) != 0) {
      if ((fbzMode & SST_ENALPHABUFFER) != 0) {
        doAuxP = FXTRUE;

        zacolor &= ~SST_ZACOLOR_ALPHA;
        zacolor |= (((FxU32) alpha) << SST_ZACOLOR_ALPHA_SHIFT);
      } else if ((fbzMode & SST_ENDEPTHBUFFER) != 0) {
        doAuxP = FXTRUE;

        zacolor &= ~SST_ZACOLOR_DEPTH;
        zacolor |= (((FxU32) depth) << SST_ZACOLOR_DEPTH_SHIFT);
      }
    }

    /* Why were we called? */
    if (!doColorP && !doAuxP) return;

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR
    /* If the width clipping is the same as the screen resolution
     * then we can use the sgram fill rect to clear the page aligned
     * region, and then use the fastFillCmd to clear the remainder.
     * 
     * NB: The test really does check against (clipLeft == 0) &&
     * (clipright == screenWidth) since screenWidth should have 0
     * in its upper bits.
     *
     * First we convert the vertical clipping from pixel coordinates
     * to tile coordinates. (The top coordinate is rounded up to the
     * next page, and the bottom coordinate is rounded down.) A page
     * (4k) is composed of two tiles layed out horizontally. We then
     * map this into fbi memory based on the current screen
     * resolution, set in grSstWinOpen, scaled into tile space as
     * well. Ick!
     *
     * To top it all of we have this 'special' case thanks to sli and
     * y origin flipping. The problem is that each card is responsible
     * for rendering its set of scanlines, regardless or the location
     * of the y origin. However, if the origin is 'flipped' then it
     * can be the case that the 'first' scanline on the screen is
     * actually odd. To get around this the hw implicitly adds two
     * invisible (they're always black) scanlines to the renderable
     * area. If the y origin is at the top of the screen then
     * scanlines screenHeight and (screenHeight + 1) are not
     * rendered. If the origin is 'flipped' then scanlines 0 and
     * (screenHeight + 1) are not rendered.
     */
    if (gc->state.fbi_config.clipLeftRight == gc->state.screen_width) {
      const FxU32 screenHeightAdjust = (gc->scanline_interleaved && bottomOriginP ? 1 : 0);
      const FxU32 screenHeight = gc->state.screen_height + screenHeightAdjust;
      const FxU32 yTileShift = gc->hwDep.cvgDep.yTileShift;
      const FxU32 yTileMask = ((0x01 << yTileShift) - 1);
      const FxU32 regBottom = ((clipBottomTop >> SST_CLIPBOTTOM_SHIFT) & 0x0FFF);
      const FxU32 regTop = ((clipBottomTop >> SST_CLIPTOP_SHIFT) & 0x0FFF);
      const FxU32 clipLow = (bottomOriginP ? screenHeight - regTop : regBottom);
      const FxU32 clipHi = (bottomOriginP ? screenHeight - regBottom : regTop);
      const FxU32 tileLow = ((clipLow >> yTileShift) + (((clipLow & yTileMask) == 0) ? 0 : 1));
      const FxU32 tileHi = (clipHi >> yTileShift);

      /* If they're both in the same tile then just clear using a
       * fastFillCmd 
       */
      if (tileHi <= tileLow) goto __skipBlitClear;

      /* Column size is always a full page size divided by the #
       * of points in a tile row (2 tiles). 
       *
       * NB: This set is 'lifted' out of the groupings below because
       * it is common and stays set accross blit invocations.
       */
      {
        GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
        GR_SET(BROADCAST_ID, hw, bltSize,  
               (((((tileHi - tileLow) * gc->hwDep.cvgDep.xTilePages) - 1) << 16) | 511));/* 511 == (0x1000 >> 3) - 1 */
        GR_CHECK_SIZE();
      }

      /* Clear the current buffer */
      if (doColorP) {
        const FxU32 tileBuffer = (gc->hwDep.cvgDep.numBufferPages * gc->hwDep.cvgDep.renderBuf);

        REG_GROUP_BEGIN(BROADCAST_ID, bltDstXY, 3, 0x29);
        {
          /* Starting point of the blit. We always do a full page in x. */
          REG_GROUP_SET(hw, bltDstXY, 
                        (((tileBuffer + (tileLow * gc->hwDep.cvgDep.xTilePages)) << 16) | 0x00UL));

          /* dpc - 27 aug 1997 - FixMe!
           * What is the right way of converting the 888 color that we're
           * passed into a 565 color?
           */
          REG_GROUP_SET(hw, bltColor, (((color & 0xF8UL) >> (3 + 0)) |      /* blue [4:0] */
                                       ((color & 0xFC00UL) >> (2 + 3)) |    /* green [10:5] */
                                       ((color & 0xF80000UL) >> (3 + 5)))); /* red [15:11] */

          REG_GROUP_SET(hw, bltCommand, (SSTG_FRECTFILL | /* SGRAM Fill command */
                                         0x80000000));    /* Start blit signal */
        }
        REG_GROUP_END();
      }

      /* Clear the aux buffer */
      if (doAuxP) {
        const FxU32 tileBuffer = (gc->hwDep.cvgDep.numBufferPages * gc->state.num_buffers);
           
        REG_GROUP_BEGIN(BROADCAST_ID, bltDstXY, 3, 0x29);
        {
          REG_GROUP_SET(hw, bltDstXY, (((tileBuffer + (tileLow * gc->hwDep.cvgDep.xTilePages)) << 16) |
                                       0x00UL));
          REG_GROUP_SET(hw, bltColor, zacolor);
          REG_GROUP_SET(hw, bltCommand, (SSTG_FRECTFILL | /* SGRAM Fill command */
                                         0x80000000));    /* Start blit signal */
        }
        REG_GROUP_END();
      }

  __skipBlitClear:
      /* Did we round off crap to page align the top part of
       * clipping? If so then reset the hw clipping and use
       * the normal fastFillCmd on this region. 
       */
      didClipP = ((tileLow << yTileShift) > clipLow);
      if (didClipP) {
        REG_GROUP_BEGIN(BROADCAST_ID, zaColor, 2, 0x41UL);
        {
          /* Set the colors */
          REG_GROUP_SET(hw, zaColor, zacolor);
          REG_GROUP_SET(hw, c1, color);
        }
        REG_GROUP_END();

        REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 
                        2 + bottomOriginP, (bottomOriginP | 0x28UL));
        {
          /* Put the origin in our canonical clipping form */
          if (bottomOriginP) REG_GROUP_SET(hw, fbzMode, fbzMode & ~SST_YORIGIN);

          /* Reset the clipping to have the same top clipping and
           * our rounded clip value as the bottom.
           */
          REG_GROUP_SET(hw, clipBottomTop, ((clipLow << SST_CLIPBOTTOM_SHIFT) |
                                            MIN(((tileLow << yTileShift) << SST_CLIPTOP_SHIFT), clipHi)));
          
          /* Execute the FASTFILL command */
          REG_GROUP_SET(hw, fastfillCMD, 1);
        }
        REG_GROUP_END();
      }       
         
      /* If we're done then we have to restore the color registers.
       * Otherwise, leave them set for the next fastFillCmd, and
       * reset the clipping for the bottom rounding error.
       */
      doneP = ((tileLow > tileHi) |
               ((tileHi << yTileShift) == clipHi));
      if (doneP && didClipP) {
        REG_GROUP_BEGIN(BROADCAST_ID, zaColor, 2, 0x41);
        {
          REG_GROUP_SET(hw, zaColor, oldzacolor);
          REG_GROUP_SET(hw, c1, oldc1);
        }
        REG_GROUP_END();
      } else if (!doneP) {
        const FxBool origSetP = (bottomOriginP & !didClipP);

        REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 1 + origSetP, (origSetP | 0x8UL));
        {
          if (origSetP) REG_GROUP_SET(hw, fbzMode, fbzMode & ~SST_YORIGIN);

          /* Reset the clipping to have the same bottom clipping and
           * our rounded clip value as the top.
           */
          REG_GROUP_SET(hw, clipBottomTop, (((tileHi << yTileShift) << SST_CLIPBOTTOM_SHIFT) |
                                            (clipHi << SST_CLIPTOP_SHIFT)));
        }
        REG_GROUP_END();

        /* Clear the color sense flags so that we don't waste
         * time sending them again, and always set the clip flag.
         */
        doColorP = 
        doAuxP   = FXFALSE;
        didClipP = FXTRUE;
      }
    }
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR */
    
    if (!doneP) {
      REG_GROUP_BEGIN(BROADCAST_ID, zaColor, 2, 0x41);
      {
        REG_GROUP_SET(hw, zaColor, zacolor);
        REG_GROUP_SET(hw, c1, color);
      }
      REG_GROUP_END();
      
      REG_GROUP_BEGIN(BROADCAST_ID, fastfillCMD, 3, 0x209);
      {
        /* Execute the FASTFILL command */
        REG_GROUP_SET(hw, fastfillCMD, 1);
        
        /* Restore C1 and ZACOLOR */
        REG_GROUP_SET(hw, zaColor, oldzacolor);
        REG_GROUP_SET(hw, c1, oldc1);
      }
      REG_GROUP_END();
    }

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR
    /* Restore clipping */
    if (didClipP) {
      REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 
                      bottomOriginP + 1, (bottomOriginP | 0x8UL));
      {
        if (bottomOriginP) REG_GROUP_SET(hw, fbzMode, fbzMode);
        REG_GROUP_SET(hw, clipBottomTop, clipBottomTop);
      }
      REG_GROUP_END();
    }
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR */
  }

#undef FN_NAME
} /* grBufferClear */

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
void
_grSliOriginClear(void)
{
#define FN_NAME "_grSliOriginClear"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 86);

  GR_ASSERT(gc->scanline_interleaved);
  GR_ASSERT(gc->hwDep.cvgDep.sliOriginBufCount != 0);

  /* Setup the two pixel high area to clear */
  REG_GROUP_BEGIN(BROADCAST_ID, clipLeftRight, 3, 0x1003UL);
  {
    REG_GROUP_SET(hw, clipLeftRight, gc->state.screen_width);
    REG_GROUP_SET(hw, clipBottomTop, 0x01UL);
    REG_GROUP_SET(hw, c1, 0x00UL);
  }
  REG_GROUP_END();
  
  /* Do the clear */
  REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 2, 0x21UL);
  {
    REG_GROUP_SET(hw, fbzMode, 
                  ((gc->state.fbi_config.fbzMode & ~SST_YORIGIN) | 
                   (~gc->state.fbi_config.fbzMode & SST_YORIGIN) |
                   SST_RGBWRMASK));
    REG_GROUP_SET(hw, fastfillCMD, 0x01UL);
  }
  REG_GROUP_END();

  REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 2, 0x21UL);
  {
    REG_GROUP_SET(hw, fbzMode, 
                  (gc->state.fbi_config.fbzMode | SST_RGBWRMASK));
    REG_GROUP_SET(hw, fastfillCMD, 0x01UL);
  }
  REG_GROUP_END();
  
  /* Restore the initial glide state.
   * NB: Cannot be done in one packet. Wah!
   */
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode, gc->state.fbi_config.fbzMode);
  GR_CHECK_SIZE();

  REG_GROUP_BEGIN(BROADCAST_ID, clipLeftRight, 3, 0x1003UL);
  {
    REG_GROUP_SET(hw, clipLeftRight, gc->state.fbi_config.clipLeftRight);
    REG_GROUP_SET(hw, clipBottomTop, gc->state.fbi_config.clipBottomTop);
    REG_GROUP_SET(hw, c1, gc->state.fbi_config.color1);
  }
  REG_GROUP_END();

  gc->hwDep.cvgDep.sliOriginBufCount--;
#undef FN_NAME
}
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) */

/*---------------------------------------------------------------------------
** grBufferSwap
**
** NOTE:        This routine should be COMPLETELY device-independant,
**              but it isn't right now because we need to move all the
**              code for the splash screen into the init library.
*/
GR_ENTRY(grBufferSwap, void, (int swapInterval))
{
#define FN_NAME "grBufferSwap"
  int vSync;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 88);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",swapInterval);

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
  if (gc->hwDep.cvgDep.singleBufferP) return;
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) */

#if ((GLIDE_PLATFORM & GLIDE_HW_CVG) || (GLIDE_PLATFORM & GLIDE_HW_H3))
  #ifdef GLIDE_PLUG
  /* optionally display the 3Dfx powerfield logo overlay */
  if (_GlideRoot.environment.shamelessPlug) _grShamelessPlug();
  #endif

  /* check for environmental override.
   * 
   * NB: If we are sli, the application passes in 0, and the user has
   * elected to not sync to retrace then we honor the user's
   * choice. Otherwise we force syncing because most apps don't know
   * any better. If, however, the user has not chosen, but the app
   * wants something other than 0 then we need to honor their choice.  
   */
  swapInterval = ((_GlideRoot.environment.swapInterval >= 0)
                  ? _GlideRoot.environment.swapInterval
                  : (gc->scanline_interleaved ? MAX(swapInterval, 1) : swapInterval));
  
  GR_CHECK_F(myName,
             (swapInterval > 255) || (swapInterval < 0),
             "swap_interval out of range");

  /* Wait until there's 6 or fewer buffer swaps pending.  the hardware
   * counter is only 3 bits so we don't want it to overflow also the
   * latency gets too long.
   */
  while (grBufferNumPending() > 6)
    ;
  
  /* if the interval is non-zero turn on VSYNC waiting */
  vSync = (swapInterval > 0);

  /* when triple buffering, vsync must be enabled and swapbuffer interval must be 0 */
  /*if (gc->grColBuf >= 3) {
    vSync = FXTRUE;
    swapInterval = 0;
  }*/
  
  if (swapInterval > 0) swapInterval--;
  
    /* NOTE: we need a PCI read before and after the swap command */
    /* but since we already called grBufferNumPending() we've done a read */
  {
    const FxU32 swapCmdVal = (swapInterval << 1) | vSync;

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) && USE_PACKET_FIFO
    /* When the command fifo is enabled we need to do a naked
     * set to the command register too so that the num swaps pending
     * field in status is kept in sync w/ reality.
     *
     * NB: This must be done before the fifo write to avoid a race
     * condition if the hw is on our ass.
     *
     * NB: We fence here just in case since we are hopefully going to
     * figure out the damn uswc problem soon.
     */
    SET(hw->swapbufferCMD, swapCmdVal);
    P6FENCE;
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && USE_PACKET_FIFO */

    GR_SET_EXPECTED_SIZE(4, 1);
    GR_SET(BROADCAST_ID, hw, swapbufferCMD,  swapCmdVal);
    GR_CHECK_SIZE();
  }
  
#ifdef GLIDE_DEBUG
  if ((FxI32)_GlideRoot.environment.snapshot > 0) {
    static char saveDBG[GDBG_MAX_LEVELS];
    int i;
    
    /* turn off tracing after frame 0 and the snapshot frame */
    if ((_GlideRoot.stats.bufferSwaps == 0) || 
        (_GlideRoot.stats.bufferSwaps == _GlideRoot.environment.snapshot + 3)) {
      GDBG_PRINTF(FN_NAME": FX_SNAPSHOT (%ld)\n", _GlideRoot.stats.bufferSwaps);
      for (i = 1; i < GDBG_MAX_LEVELS; i++) {
        if (_GlideRoot.stats.bufferSwaps == 0) saveDBG[i] = (char)GDBG_GET_DEBUGLEVEL(i);
        GDBG_SET_DEBUGLEVEL(i, 0);
      }
    }

    /* turn on tracing after the snapshot frame */
    if (_GlideRoot.stats.bufferSwaps == _GlideRoot.environment.snapshot) {
      GDBG_PRINTF(FN_NAME": FX_SNAPSHOT (%ld)\n", _GlideRoot.stats.bufferSwaps);
      for (i = 1; i < GDBG_MAX_LEVELS; i++) {
        GDBG_SET_DEBUGLEVEL(i, saveDBG[i]);
      }
    }
  }
#endif /* GLIDE_DEBUG */

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
  if (gc->hwDep.cvgDep.sliOriginBufCount != 0) _grSliOriginClear();

#if GLIDE_BLIT_CLEAR
  {
    const FxU32 numBufs = gc->state.num_buffers;
    FxU32* bufPtrs[3];
    FxU32 i;

    bufPtrs[0] = &gc->hwDep.cvgDep.renderBuf;
    bufPtrs[1] = &gc->hwDep.cvgDep.frontBuf;
    bufPtrs[2] = &gc->hwDep.cvgDep.backBuf;

    for(i = 0; i < sizeof(bufPtrs) / sizeof(*bufPtrs); i++) {
      *bufPtrs[i] = (*bufPtrs[i] + 1) % numBufs;
    }

    GDBG_INFO(gc->myLevel, 
              "\trenderBuf: 0x%X\n",
              gc->hwDep.cvgDep.renderBuf);
  }
#endif /* GLIDE_BLIT_CLEAR */
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) */

  grSstStatus();        /* special bug workaround       */
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

GR_ENTRY(grBufferNumPending, int, (void))
{
  int pend;                       /* Num Swaps pending */

  {
    FxU32 status = grSstStatus();
    
#if ((GLIDE_PLATFORM & GLIDE_HW_CVG) || (GLIDE_PLATFORM & GLIDE_HW_H3))
    pend = ((status & SST_SWAPBUFPENDING) >> SST_SWAPBUFPENDING_SHIFT);
#else
#error "Implement BufferNumPending for this hardware"
#endif
  }

  GDBG_INFO(86,"grBufferNumPending() => %d\n", pend);

  return pend;
} /* grBufferNumPending */

/*---------------------------------------------------------------------------
** grChromakeyMode
*/

GR_STATE_ENTRY(grChromakeyMode, void, (GrChromakeyMode_t mode))
{
#define FN_NAME "grChromakeyMode"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("grChromakeyMode", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", mode);

  fbzMode = gc->state.fbi_config.fbzMode;
  if (mode == GR_CHROMAKEY_ENABLE)
    fbzMode |= SST_ENCHROMAKEY;
  else
    fbzMode &= ~SST_ENCHROMAKEY;

  gc->state.fbi_config.fbzMode = fbzMode;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(eChipFBI, hw, fbzMode,  fbzMode);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grChromaKeyMode */

/*---------------------------------------------------------------------------
** grChromakeyValue
*/
GR_STATE_ENTRY(grChromakeyValue, void, (GrColor_t color))
{
#define FN_NAME "grChromakeyValue"
  GR_BEGIN_NOFIFOCHECK("grChromakeyValue", 10);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", color);

  _grSwizzleColor(&color);
  gc->state.fbi_config.chromaKey = color;

#if 1
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(eChipFBI, hw, chromaKey,  color);
  GR_CHECK_SIZE();
#else
  /* Experimental fix to see if the increased precison of v^2 bilinear
   * blending (.4 to .8) was causing more grief to developers who did
   * the hacky by turning on chroma w/ bilinear.
   */
#define kChromaOffset 2
  {
    const FxI32 compR = (color >> 16) & 0xFF;
    const FxI32 compG = (color >>  8) & 0xFF;
    const FxI32 compB = (color >>  0) & 0xFF;
    const FxU32 chromaMin = ((MAX(compR - kChromaOffset, 0x00) << 16) |
                             (MAX(compG - kChromaOffset, 0x00) <<  8) |
                             (MAX(compB - kChromaOffset, 0x00) <<  0));
    const FxU32 chromaMax = ((MIN(compR + kChromaOffset, 0xFF) << 16) |
                             (MIN(compG + kChromaOffset, 0xFF) <<  8) |
                             (MIN(compB + kChromaOffset, 0xFF) <<  0));

    REG_GROUP_BEGIN(eChipFBI, chromaKey, 2, 0x03UL);
    {
      REG_GROUP_SET(hw, chromaKey, chromaMin);
      REG_GROUP_SET(hw, chromaRange, chromaMax | SST_ENCHROMARANGE);
    }
    REG_GROUP_END();
  }
#undef kChromaOffset
#endif
#undef FN_NAME
} /* grChromaKeyValue */

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

  GDBG_INFO(85, "%s:  minx = %d, maxx = %d, miny = %d, maxy = %d\n",
            FN_NAME, minx, maxx, miny, maxy);

  GR_CHECK_COMPATABILITY(FN_NAME, 
                         ((maxx > gc->state.screen_width) || (maxy > gc->state.screen_height)),
                         "Max clip coordinate > screen size");
  GR_CHECK_COMPATABILITY(FN_NAME,
                         ((minx > LONG_MAX) || (miny > LONG_MAX)),
                         "Negative min clip coordinate");

  /* don't allow bogus clip coords!!! */
  if (maxx > gc->state.screen_width) maxx = gc->state.screen_width;
  if (maxy > gc->state.screen_height) maxy = gc->state.screen_height;
  if (minx > maxx) minx = maxx;
  if (miny > maxy) miny = maxy;

  GDBG_INFO(85, "%s: normalized  minx = %d, maxx = %d, miny = %d, maxy = %d\n",
            FN_NAME, minx, maxx, miny, maxy);

  *clipLeftRight = (minx << SST_CLIPLEFT_SHIFT) | (maxx << SST_CLIPRIGHT_SHIFT); 
  *clipBottomTop = (miny << SST_CLIPBOTTOM_SHIFT) | (maxy << SST_CLIPTOP_SHIFT);

  GDBG_INFO(85, "%s:  clipLeftRight = 0x%x, clipBottomTop = 0x%x\n",
            FN_NAME, clipLeftRight, clipBottomTop);

#undef FN_NAME 
} /* _grClipNormalizeAndGenerateRegValues */

/*---------------------------------------------------------------------------
** grClipWindow
*/
GR_STATE_ENTRY(grClipWindow, void, (FxU32 minx, FxU32 miny, 
                                    FxU32 maxx, FxU32 maxy)) 
{
#define FN_NAME "grClipWindow"
  FxU32
    clipLeftRight,              /* SST Clipping Registers */
    clipBottomTop;

  GR_BEGIN_NOFIFOCHECK("grClipWindow",83);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d %d,%d)\n",minx,miny,maxx,maxy);

  _grClipNormalizeAndGenerateRegValues(minx, miny, maxx, maxy,
                                       &clipLeftRight, &clipBottomTop);

  REG_GROUP_BEGIN(BROADCAST_ID, clipLeftRight, 2, 0x03);
  {
    REG_GROUP_SET(hw, clipLeftRight, clipLeftRight);
    REG_GROUP_SET(hw, clipBottomTop, clipBottomTop);
  }
  REG_GROUP_END();

  gc->state.fbi_config.clipLeftRight = clipLeftRight;
  gc->state.fbi_config.clipBottomTop = clipBottomTop;

  gc->state.clipwindowf_xmin = (float) minx;
  gc->state.clipwindowf_xmax = (float) maxx;
  gc->state.clipwindowf_ymin = (float) miny;
  gc->state.clipwindowf_ymax = (float) maxy;

  GR_END();
#undef FN_NAME
} /* grClipWindow */

/*---------------------------------------------------------------------------
** grColorCombine
*/

GR_STATE_ENTRY(grColorCombine, void, 
               (GrCombineFunction_t function, GrCombineFactor_t factor,
                GrCombineLocal_t local, GrCombineOther_t other, FxBool invert))
{
#define FN_NAME "grColorCombine"
  FxU32 fbzColorPath;
  FxU32 oldTextureEnabled;

  GR_BEGIN_NOFIFOCHECK("grColorCombine",85);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d,%d,%d)\n",function,factor,local,other,invert);

  GR_CHECK_W(myName,
             function < GR_COMBINE_FUNCTION_ZERO ||
             function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA ,
             "unsupported color combine function");

  /* Starting w/ Voodoo^2 the ccu has texture RGB mode as well. */
  GR_CHECK_W(myName,
             (factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
             (factor & 0x7) > GR_COMBINE_FACTOR_TEXTURE_RGB ||
             factor > GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA,
             "unsupported color combine scale factor");
  GR_CHECK_W(myName,
             local < GR_COMBINE_LOCAL_ITERATED || local > GR_COMBINE_LOCAL_DEPTH,
             "unsupported color combine local color");
  GR_CHECK_W(myName,
             other < GR_COMBINE_OTHER_ITERATED || other > GR_COMBINE_OTHER_CONSTANT,
             "unsupported color combine other color");

  fbzColorPath = gc->state.fbi_config.fbzColorPath;
  oldTextureEnabled = fbzColorPath & SST_ENTEXTUREMAP;
  fbzColorPath &= ~(SST_ENTEXTUREMAP |
                    SST_RGBSELECT |
                    SST_LOCALSELECT |
                    SST_CC_ZERO_OTHER |
                    SST_CC_SUB_CLOCAL |
                    SST_CC_MSELECT |
                    SST_CC_REVERSE_BLEND |
                    SST_CC_ADD_CLOCAL |
                    SST_CC_ADD_ALOCAL |
                    SST_CC_INVERT_OUTPUT);

  /* this is bogus, it should be done once, somewhere. */
  fbzColorPath |= SST_PARMADJUST;

  /* setup reverse blending first, then strip off the the high bit */
  if ((factor & 0x8) == 0)
    fbzColorPath |= SST_CC_REVERSE_BLEND;
  factor &= 0x7;

  /* NOTE: we use boolean OR instead of logical to avoid branches */
  gc->state.cc_requires_texture = ((factor == GR_COMBINE_FACTOR_TEXTURE_ALPHA) |
                                   (factor == GR_COMBINE_FACTOR_TEXTURE_RGB) |
                                   (other == GR_COMBINE_OTHER_TEXTURE));
  gc->state.cc_requires_it_rgb = ((local == GR_COMBINE_LOCAL_ITERATED) |
                                  (other == GR_COMBINE_OTHER_ITERATED));

  /* setup scale factor bits */
  fbzColorPath |= factor << SST_CC_MSELECT_SHIFT;

  /* setup local color bits */
  fbzColorPath |= local << SST_LOCALSELECT_SHIFT;

  /* setup other color bits */
  fbzColorPath |= other << SST_RGBSELECT_SHIFT;

  /* setup invert output bits */
  if (invert)
    fbzColorPath |= SST_CC_INVERT_OUTPUT;

  /* setup core color combine unit bits */
  switch (function) {
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
  if (gc->state.cc_requires_texture || gc->state.ac_requires_texture)
    fbzColorPath |= SST_ENTEXTUREMAP;
  gc->state.fbi_config.fbzColorPath = fbzColorPath;

  {
    const FxBool texTransP = (oldTextureEnabled != (fbzColorPath & SST_ENTEXTUREMAP));

    GR_SET_EXPECTED_SIZE(sizeof(FxU32) + (texTransP ? sizeof(FxU32) : 0),
                         1 + texTransP);
    
    /* transition into/out of texturing ... add nopCMD */
    if (texTransP) GR_SET(BROADCAST_ID, hw, nopCMD, 0);
    GR_SET(BROADCAST_ID, hw, fbzColorPath,  fbzColorPath);
    GR_CHECK_SIZE();
  }

  /* setup paramIndex bits */
  _grUpdateParamIndex();

  GR_END();
#undef FN_NAME
} /* grColorCombine */

/*---------------------------------------------------------------------------
** grColorMask
*/

GR_STATE_ENTRY(grColorMask, void, (FxBool rgb, FxBool alpha))
{
#define FN_NAME "grColorMask"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("grColorMask", 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x,0x%x)\n", rgb, alpha);

  fbzMode = gc->state.fbi_config.fbzMode;

#if 0
  GR_CHECK_W(myName,
             (fbzMode & SST_ENDEPTHBUFFER) && alpha,
             "alpha writes enabled even though depth buffering");
#endif

  /* This is fatal because they could stomp on the cmd fifo. */
  GR_CHECK_COMPATABILITY(FN_NAME,
                         (alpha && (gc->grAuxBuf == 0)),
                         "cannot enable alpha buffering w/o allocating one");

  if (rgb)
    fbzMode |= SST_RGBWRMASK;
  else
    fbzMode &= ~SST_RGBWRMASK;

  /* GMT: BUG leave SST_ENALPHABUFFER on if dest. alpha being used */
  /* Don't do anything if depth buffering on */
  /* XXX Should check for triple buffering too */

  if (!(fbzMode & SST_ENDEPTHBUFFER) && (gc->state.num_buffers != 3)) {
    if (alpha)
      fbzMode |= SST_ENALPHABUFFER | SST_ZAWRMASK;
    else
      fbzMode &= ~(SST_ENALPHABUFFER | SST_ZAWRMASK);
  }

  gc->state.fbi_config.fbzMode = fbzMode;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode,  fbzMode);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grColorMask */

/*---------------------------------------------------------------------------
** grConstantColorValue
*/

GR_STATE_ENTRY(grConstantColorValue, void, (GrColor_t color))
{
#define FN_NAME "grConstantColorValue"
  GR_BEGIN_NOFIFOCHECK("grConstantColorValue",85);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",color);

  _grSwizzleColor(&color);

  REG_GROUP_BEGIN(BROADCAST_ID, c0, 2, 0x03);
  {
    REG_GROUP_SET(hw, c0, color);
    REG_GROUP_SET(hw, c1, color);
  }
  REG_GROUP_END();

  gc->state.fbi_config.color0 = color;
  gc->state.fbi_config.color1 = color;

  GR_END();
#undef FN_NAME
} /* grConstanColorValue */

/*---------------------------------------------------------------------------
** grConstantColorValue4
**           GMT: obsolete routine
**           GMT: send values to hardware immediately
*/

GR_ENTRY(grConstantColorValue4, void, (float a, float r, float g, float b))
{
#define FN_NAME "grConstantColorValue4"
  GR_BEGIN_NOFIFOCHECK("grConstantColorValue4",85);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d,%d)\n",a,r,g,b);
  gc->state.a = a;
  gc->state.r = r;
  gc->state.g = g;
  gc->state.b = b;

  if (gc->state.cc_delta0mode) {
    REG_GROUP_BEGIN(BROADCAST_ID, Fr, 3, 0x07);
    {
      REG_GROUP_SETF(hw, Fr, r);
      REG_GROUP_SETF(hw, Fg, g);
      REG_GROUP_SETF(hw, Fb, b);
    }
    REG_GROUP_END();
  }

  GR_END();
#undef FN_NAME
} /* grConstantColorValue4 */

/*---------------------------------------------------------------------------
** grCullMode
**
** GMT: warning - gaa.c has the guts of this in-line
*/

GR_ENTRY(grCullMode, void, (GrCullMode_t mode))
{
#define FN_NAME "grCullMode"
  GR_BEGIN_NOFIFOCHECK("grCullMode",85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",mode);
  gc->state.cull_mode = mode;

#if GLIDE_HW_TRI_SETUP
  _grUpdateTriPacketHdr(gc->cmdTransportInfo.paramMask, mode);
#endif /* GLIDE_HW_TRI_SETUP */

  GR_END();
#undef FN_NAME
} /* grCullMode */

/*---------------------------------------------------------------------------
** grDepthBiasLevel
**
** Sets the depth bias level.
*/

GR_STATE_ENTRY(grDepthBiasLevel, void, (FxI16 level))
{
#define FN_NAME "grDepthBiasLevel"
  FxU32 zacolor;
  GR_BEGIN_NOFIFOCHECK("grDepthBiasLevel", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",level);

  zacolor = gc->state.fbi_config.zaColor;
  zacolor = (zacolor & ~SST_ZACOLOR_DEPTH) | (level & SST_ZACOLOR_DEPTH);

  gc->state.fbi_config.zaColor = zacolor;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, zaColor,  zacolor);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grDepthBiasLevel */

/*---------------------------------------------------------------------------
** grDepthBufferFunction
*/

GR_STATE_ENTRY(grDepthBufferFunction, void, (GrCmpFnc_t fnc))
{
#define FN_NAME "grDepthBufferFunction"
  FxU32 fbzMode;

  GR_BEGIN_NOFIFOCHECK("grDepthBufferFunction",85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",fnc);

  fbzMode = gc->state.fbi_config.fbzMode;
  fbzMode &= ~SST_ZFUNC;
  fbzMode |= (fnc << SST_ZFUNC_SHIFT);

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode,  fbzMode);
  GR_CHECK_SIZE();

  gc->state.fbi_config.fbzMode = fbzMode;
  GR_END();
#undef FN_NAME
} /* grDepthBufferFunction */

/*---------------------------------------------------------------------------
** grDepthBufferMode
*/

GR_STATE_ENTRY(grDepthBufferMode, void, (GrDepthBufferMode_t mode))
{
#define FN_NAME "grDepthBufferMode"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("grDepthBufferMode", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", mode);

  /* This is fatal because they could stomp on the cmd fifo. */
  GR_CHECK_COMPATABILITY(FN_NAME,
                         (mode != GR_DEPTHBUFFER_DISABLE) && (gc->grAuxBuf == 0),
                         "cannot enable depth buffer w/o allocating one");

  /* turn off all the bits and then turn them back on selectively */
  fbzMode = gc->state.fbi_config.fbzMode;
  fbzMode &= ~(SST_ENDEPTHBUFFER | 
               SST_WBUFFER | 
               SST_ENZBIAS | 
               SST_ZCOMPARE_TO_ZACOLOR);

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

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode,  fbzMode);
  GR_CHECK_SIZE();

  _grUpdateParamIndex();
#undef FN_NAME
} /* grDepthBufferMode */

/*---------------------------------------------------------------------------
** No Comment
*/
#ifdef GLIDE_DEBUG
FxBool
_grCanSupportDepthBuffer(void)
{
  GR_DCL_GC;
  if (gc->state.screen_height == 640) {
    if (gc->fbuf_size == 1) {
      return FXFALSE;
    }
  } else if (gc->state.screen_width == 800) {
    if ((gc->fbuf_size == 1) ||
        (gc->fbuf_size == 2)) {
      return FXFALSE;
    }
  }
  return FXTRUE;
} /* _grCanSupportDepthBuffer */
#endif

/*---------------------------------------------------------------------------
** grDepthMask
*/
GR_STATE_ENTRY(grDepthMask, void, (FxBool enable))
{
#define FN_NAME "grDepthMask"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("grDepthMask", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", enable);

  fbzMode = gc->state.fbi_config.fbzMode;
  GR_CHECK_F(myName,
             enable && !_grCanSupportDepthBuffer(),
             "called in a non-depthbufferable configuration");

  if (enable)
    fbzMode |= SST_ZAWRMASK;
  else
    fbzMode &= ~SST_ZAWRMASK;

  gc->state.fbi_config.fbzMode = fbzMode;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode,  fbzMode);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grDepthMask */

/*---------------------------------------------------------------------------
** grDisableAllEffects
*/

GR_ENTRY(grDisableAllEffects, void, (void))
{
  GDBG_INFO(87,"grDisableAllEffects()\n");
  grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO);
  grAlphaTestFunction(GR_CMP_ALWAYS);
  grChromakeyMode(GR_CHROMAKEY_DISABLE);
  grDepthBiasLevel(0);
  grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
  grFogMode(GR_FOG_DISABLE);
} /* grDisableAllEffects */

/*---------------------------------------------------------------------------
** grDitherMode
*/

GR_STATE_ENTRY(grDitherMode, void, (GrDitherMode_t mode))
{
#define FN_NAME "grDitherMode"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("grDitherMode", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", mode);

  fbzMode = gc->state.fbi_config.fbzMode;
  fbzMode &= ~(SST_ENDITHER | SST_DITHER2x2 | SST_ENDITHERSUBTRACT);

  switch (mode) {
  case GR_DITHER_DISABLE:
    /* alpha dither subtract should be disabled */
    break;

  case GR_DITHER_2x2:
  case GR_DITHER_4x4:
    /* force 4x4 dither with alpha dither subtraction */
    fbzMode |= (SST_ENDITHER | SST_ENDITHERSUBTRACT);

    /* disable alpha blending dither subtraction according to user request */
    if (_GlideRoot.environment.disableDitherSub == FXTRUE) {
      /* without alpha dither subtraction, 2x2 dither looks better */
      fbzMode |= SST_DITHER2x2;
      fbzMode &= ~(SST_ENDITHERSUBTRACT);
    }
    break;
  }

  gc->state.fbi_config.fbzMode = fbzMode;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode,  fbzMode);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grDitherMode */

/*---------------------------------------------------------------------------
** grFogMode
*/

GR_STATE_ENTRY(grFogMode, void, (GrFogMode_t mode))
{
#define FN_NAME "grFogMode"
  FxU32 fogmode;
  GR_BEGIN_NOFIFOCHECK("grFogMode", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",mode);

  fogmode = gc->state.fbi_config.fogMode;
  fogmode &= ~(SST_ENFOGGING | 
               SST_FOGADD | 
               SST_FOGMULT | 
               SST_FOG_ALPHA | 
               SST_FOG_Z | 
               SST_FOG_CONSTANT);

  switch (mode & 0xFF) {    /* switch based on lower 8 bits */
  case GR_FOG_DISABLE:
    break;

  case GR_FOG_WITH_ITERATED_ALPHA:
    fogmode |= (SST_ENFOGGING | SST_FOG_ALPHA);
    break;

  case GR_FOG_WITH_ITERATED_Z:        /* Bug 735 */
    fogmode |= (SST_ENFOGGING | SST_FOG_Z);
    break;

  case GR_FOG_WITH_TABLE:
    fogmode |= (SST_ENFOGGING);
    break;
  }
  if (mode &  GR_FOG_MULT2) fogmode |= SST_FOGMULT;
  if (mode &  GR_FOG_ADD2) fogmode |= SST_FOGADD;

  /* GMT says that this should be enabled for CVG.  It is always safe
   * to enable these even when fogging is not enabled.  
   */
  fogmode |= (SST_FOG_DITHER | SST_FOG_ZONES);

  /*
  ** Update the hardware and Glide state
  */
  gc->state.fbi_config.fogMode = fogmode;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fogMode,  fogmode);
  GR_CHECK_SIZE();

  _grUpdateParamIndex();
#undef FN_NAME
} /* grFogMode */

/*---------------------------------------------------------------------------
** grFogColorValue
*/

GR_STATE_ENTRY(grFogColorValue, void, (GrColor_t color))
{
#define FN_NAME "grFogColorValue"
  GR_BEGIN_NOFIFOCHECK("grFogColorValue", 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", color);

  _grSwizzleColor(&color);

  gc->state.fbi_config.fogColor = color;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fogColor,  color);
  GR_CHECK_SIZE();
#undef FN_NAME
} /* grFogColorValue */

/*---------------------------------------------------------------------------
** grFogTable
*/

GR_ENTRY(grFogTable, void, (const GrFog_t fogtable[]))
{
#define FN_NAME "grFogTable"
  const GrFog_t *locTable = fogtable;
  const int iend = (kInternalFogTableEntryCount >> 1);
  GR_BEGIN_NOFIFOCHECK("grFogTable",85);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",fogtable);
  GR_ASSERT(fogtable != NULL);
  GR_ASSERT(kInternalFogTableEntryCount == 64);

  REG_GROUP_LONG_BEGIN(BROADCAST_ID, fogTable[0], 32);
  {
    int j;
    
    for (j = 0; j < iend; j++) {
      FxU32 e0, e1, d0, d1;
      
      e0 = locTable[0];                     /* lower entry */
      e1 = locTable[1];                     /* upper entry */

      GR_CHECK_COMPATABILITY(FN_NAME, 
                             (e1 < e0), 
                             "Invalid fog table entry. (Must be monotonically increasing)");
      GR_CHECK_COMPATABILITY(FN_NAME, 
                             ((j > 0) ? (e0 < locTable[-1]) : 0),
                             "Invalid fog table entry. (Must be monotonically increasing)");
      
      /* del0 in .2 format */
      d0 = ((e1 - e0) << 2); 

      /* del1 in .2 format - don't access beyond end of table */
      d1 = ((j == (iend - 1)) ? e1 : locTable[2]);
      d1 = (d1 - e1) << 2;
      
      REG_GROUP_SET(hw, fogTable[j], 
                    ((e1 << 24) | (d1 << 16) | (e0 << 8) | d0));
      GDBG_INFO(gc->myLevel + 200, "fogTable[%ld]: 0x%X\n",
                j, ((e1 << 24) | (d1 << 16) | (e0 << 8) | d0));
      locTable += 2;
    }
  }
  REG_GROUP_END();

  GR_END();
#undef FN_NAME
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

GR_ENTRY(grGlideShutdown, void, (void))
{
  _GlideRoot.windowsInit = FXFALSE;

  if (!_GlideRoot.initialized) return;  /* never made it thru startup */

#if GLIDE_SANITY_SIZE
  /* GMT: reset the counter so we can proceed without assertions */
  {
    GR_DCL_GC;
    gc->counter = gc->expected_counter = 0;
  }
#endif /* GLIDE_SANITY_SIZE */

  {
    int i;
    GR_BEGIN_NOFIFOCHECK("grGlideShutdown\n", 80);

    for(i = 0; i < _GlideRoot.hwConfig.num_sst; i++) {
      grSstSelect(i);
      grSstWinClose();
    }

#if GLIDE_INIT_HAL
    fxHalShutdownAll();
#else /* !GLIDE_INIT_HAL */
    /* dpc - 5 sep 1997 - FixMe!
     * Move this to the new initCode world some time.
     * 
     * initClose();
     *
     * NB: This is only necessary to properly shtudown the hw and vxd
     * if the application never called grSstWinOpen because they are
     * doing some sort of hw/dll detection before trying to run w/
     * glide.  
     */
    for(i = 0; i < _GlideRoot.hwConfig.num_sst; i++) {
      if (_GlideRoot.GCs[i].hwInitP) {
        /*if (_GlideRoot.CPUType.family >= 6) {*/
        sst1InitCaching(_GlideRoot.GCs[i].base_ptr, FXFALSE);
        /*}*/
        sst1InitShutdown(_GlideRoot.GCs[i].base_ptr);

        _GlideRoot.GCs[i].hwInitP = FXFALSE;
      }
    }
#endif /* !GLIDE_INIT_HAL */

    _grDisplayStats();
    gc->grSstRez = GR_RESOLUTION_NONE;
    gc->grSstRefresh = GR_REFRESH_NONE;
  }

  _GlideRoot.initialized = FXFALSE;
} /* grGlideShutdown */


/*-------------------------------------------------------------------
  Function: _grFlushCommonStateRegs
  Date: 14-Oct-97
  Implementor(s): dpc
  Description:
        Flushes all State Monster regs.  If we're not doing Glide 3,
        then it's still used by grGlideSetState()
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void
_grFlushCommonStateRegs()
{
#define FN_NAME "_grFlushCommonStateRegs"
  GR_BEGIN("_grFlushCommonStateRegs", 97, 13 * sizeof(FxU32), 2);

  /* NB: This logical write must be split into two writes since the
   * registers are non-contiguous (not good for packet 1) and are the
   * register span is larget (greater than the 14 register mask size
   * for packet 4).
   */
  REG_GROUP_NO_CHECK_BEGIN(BROADCAST_ID, fbzColorPath, 10, 0x1C7F);
  {
    REG_GROUP_SET(hw, fbzColorPath, gc->state.fbi_config.fbzColorPath);
    REG_GROUP_SET(hw, fogMode, gc->state.fbi_config.fogMode);
    REG_GROUP_SET(hw, alphaMode, gc->state.fbi_config.alphaMode);
    REG_GROUP_SET(hw, fbzMode, gc->state.fbi_config.fbzMode);
    
    REG_GROUP_SET(hw, lfbMode, gc->state.fbi_config.lfbMode);
    REG_GROUP_SET(hw, clipLeftRight, gc->state.fbi_config.clipLeftRight);
    REG_GROUP_SET(hw, clipBottomTop, gc->state.fbi_config.clipBottomTop);
    
    REG_GROUP_SET(hw, fogColor, gc->state.fbi_config.fogColor);
    REG_GROUP_SET(hw, zaColor, gc->state.fbi_config.zaColor);
    REG_GROUP_SET(hw, chromaKey, gc->state.fbi_config.chromaKey);
  }
  REG_GROUP_NO_CHECK_END();
  
  REG_GROUP_NO_CHECK_BEGIN(BROADCAST_ID, stipple, 3, 0x07);
  {
    REG_GROUP_SET(hw, stipple, gc->state.fbi_config.stipple);
    REG_GROUP_SET(hw, c0, gc->state.fbi_config.color0);
    REG_GROUP_SET(hw, c1, gc->state.fbi_config.color1);
  }
  REG_GROUP_NO_CHECK_END();


  GR_END();

#undef FN_NAME
} /* _grFlushCommonStateRegs */

/*---------------------------------------------------------------------------
** grGlideSetState
*/
GR_ENTRY(grGlideSetState, void, (const GrState *state))
{
#define FN_NAME "grGlideSetState"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 87);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",state);

  GR_ASSERT(state != NULL);
  
  /* if texture mapping changed then we need to issue a NOP command*/
  {
    const FxBool texChangeP = (((gc->state.fbi_config.fbzColorPath ^ state->fbi_config.fbzColorPath) & 
                                SST_ENTEXTUREMAP) != 0);
    if (texChangeP) {
      GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
      GR_SET(BROADCAST_ID, hw, nopCMD,  0);
      GR_CHECK_SIZE();
    }
  }

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
  /* If we're on sli and the new state includes changing the origin
   * that we clear the slop lines at the bottom of the screen.  
   */
  if (gc->scanline_interleaved) gc->hwDep.cvgDep.sliOriginBufCount = gc->grColBuf;
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) */

  /* copy all the state */
  gc->state = *state;

  /* Update the hardware state from the saved state. */
  _grFlushCommonStateRegs();
  
  GR_SET_EXPECTED_SIZE((7 * sizeof(FxU32)) * gc->num_tmu, gc->num_tmu);
  {
    int tmu;
    
    for (tmu = 0; tmu < gc->num_tmu; tmu++) {
      SstRegs* tmuregs = SST_TMU(hw, tmu);
      const FifoChipField chipField = (FifoChipField)(0x02UL << tmu);
      
      REG_GROUP_NO_CHECK_BEGIN(chipField, textureMode, 7, 0x7F);
      {
        REG_GROUP_SET(tmuregs, textureMode, gc->state.tmu_config[tmu].textureMode);
        REG_GROUP_SET(tmuregs, tLOD, gc->state.tmu_config[tmu].tLOD);
        REG_GROUP_SET(tmuregs, tDetail, gc->state.tmu_config[tmu].tDetail);
        REG_GROUP_SET(tmuregs, texBaseAddr, gc->state.tmu_config[tmu].texBaseAddr);
        
        REG_GROUP_SET(tmuregs, texBaseAddr1, gc->state.tmu_config[tmu].texBaseAddr_1);
        REG_GROUP_SET(tmuregs, texBaseAddr2, gc->state.tmu_config[tmu].texBaseAddr_2);
        REG_GROUP_SET(tmuregs, texBaseAddr38, gc->state.tmu_config[tmu].texBaseAddr_3_8);
      }
      REG_GROUP_NO_CHECK_END();
    }
  }
  GR_CHECK_SIZE();

  /* NOTE: since glide state includes things like hints and all cached
   * variables like paramIndex we needn't recompute these, BUT: we do
   * need to rebuild stuff that depends on them!!! 
   */
  _grUpdateParamIndex();

  GR_END();
#undef FN_NAME
} /* grGlideSetState */

/*---------------------------------------------------------------------------
** grRenderBuffer
**
**  Although SST1 supports triple buffering, it's a hack in the hardware,
**  and the only drawbuffer modes supported by the fbzMode register are 0
**  (back) and 1 (front)
*/

GR_STATE_ENTRY(grRenderBuffer, void, (GrBuffer_t buffer))
{
#define FN_NAME "grRenderBuffer"
  GR_BEGIN_NOFIFOCHECK("grRenderBuffer", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",buffer);
  GR_CHECK_F(myName, buffer >= GR_BUFFER_AUXBUFFER, "invalid buffer");

  {
    FxU32 fbzMode;
    
    fbzMode = gc->state.fbi_config.fbzMode;
    fbzMode &= ~(SST_DRAWBUFFER);
    fbzMode |= ((buffer == GR_BUFFER_FRONTBUFFER)
                ? SST_DRAWBUFFER_FRONT 
                : SST_DRAWBUFFER_BACK);
    
    GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
    GR_SET(BROADCAST_ID, hw, fbzMode, fbzMode);
    GR_CHECK_SIZE();

    gc->state.fbi_config.fbzMode = fbzMode;
  }

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR 
    /* Setting the render buffer means we need to adjust
     * our current pointer to the render buffer's physical
     * address if we're doing blit clears.
     */
    {
      const FxU32 oldRenderBuf = gc->hwDep.cvgDep.renderBuf;

      gc->hwDep.cvgDep.renderBuf = ((buffer == GR_BUFFER_FRONTBUFFER)
                                    ? gc->hwDep.cvgDep.frontBuf
                                    : gc->hwDep.cvgDep.backBuf);
      if ((gc->hwDep.cvgDep.renderBuf != oldRenderBuf) &&
          (gc->hwDep.cvgDep.sliOriginBufCount != 0)) _grSliOriginClear();
    }
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && GLIDE_BLIT_CLEAR */

  GR_END();
#undef FN_NAME
} /* grRenderBuffer */

GR_ENTRY(grCheckForRoom, void, (FxI32 n))
{
  GR_DCL_GC;

  /* dpc - 13 sep 1997 - FixMe!
   * Setting one packet for now.
   */
  GR_CHECK_FOR_ROOM(n, 1);
}

/*---------------------------------------------------------------------------
** _grUpdateParamIndex
**
** Updates the paramIndex bits based on Glide state and the hints.
**
*/
GR_DDFUNC(_grUpdateParamIndex, void, (void))
{
#define FN_NAME "_grUpdateParamIndex"
  GR_DCL_GC;
  FxU32 paramIndex = 0;
  const FxU32 hints = gc->state.paramHints;
  const FxU32 fbzColorPath = gc->state.fbi_config.fbzColorPath;
  const FxU32 fogMode = gc->state.fbi_config.fogMode;
  const FxU32 fbzMode = gc->state.fbi_config.fbzMode;

  /*
  ** First, turn on every bit that we think we need. We can prune them
  ** back later.
  */

  /* Turn on the texture bits based on what grTexCombine set */
  if (fbzColorPath & SST_ENTEXTUREMAP) {
    /* No matter what, we need oow from the main grvertex structure */
    static FxU32 paramI_array[] = {
      /* 0 */
      STATE_REQUIRES_OOW_FBI,
      
      /* 1 */
      STATE_REQUIRES_OOW_FBI | 
      STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0,

      /* 2 */
      STATE_REQUIRES_OOW_FBI | 
      STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1,

      /* 3 */
      STATE_REQUIRES_OOW_FBI | 
      STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0 | 
      STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1,

      /* 4 */
      STATE_REQUIRES_OOW_FBI | 
      STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2,

      /* 5 */
      STATE_REQUIRES_OOW_FBI | 
      STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0 |
      STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2,

      /* 6 */
      STATE_REQUIRES_OOW_FBI | 
      STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1 |
      STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2,

      /* 7 */
      STATE_REQUIRES_OOW_FBI | 
      STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0 |
      STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1 |
      STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2,
    };

    GR_ASSERT(gc->state.tmuMask < sizeof(paramI_array)/sizeof(paramI_array[0]));
    paramIndex |= paramI_array[gc->state.tmuMask];
  }  

  /* See if we need iterated RGB */
  if (gc->state.cc_requires_it_rgb && !gc->state.cc_delta0mode) {
    paramIndex |= STATE_REQUIRES_IT_DRGB;
  }

  /* See if we need to iterate alpha based on the value of
     ac_requires_it_alpha */ 
  if (gc->state.ac_requires_it_alpha) {
    paramIndex |= STATE_REQUIRES_IT_ALPHA;
  }

  /* See what fbzColorPath contributes - BUG 1084*/
  if (( fbzColorPath & SST_ALOCALSELECT ) == SST_ALOCAL_Z ) {
      paramIndex |= STATE_REQUIRES_OOZ;
  }

  /* See what fbzMode contributes */
  if (fbzMode & SST_ENDEPTHBUFFER) {
    if (fbzMode & SST_WBUFFER) {
      paramIndex |= STATE_REQUIRES_OOW_FBI;
    } else {
      paramIndex |= STATE_REQUIRES_OOZ;
    }
  }

  /* See what fogMode contributes */
  if (fogMode & SST_ENFOGGING) {
    if (fogMode & SST_FOG_Z) {
      paramIndex |= STATE_REQUIRES_OOZ;
    } else {
      if (fogMode & SST_FOG_ALPHA) {
        paramIndex |= STATE_REQUIRES_IT_ALPHA;
      } else {
        paramIndex |= STATE_REQUIRES_OOW_FBI;
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
    if (!(hints & GR_STWHINT_W_DIFF_TMU0))
      paramIndex &= ~STATE_REQUIRES_W_TMU0;
  }
  
  /* Turn off ST for TMU1 if TMU0 is active and TMU1 is not different */
  if (((paramIndex & (STATE_REQUIRES_ST_TMU0 | STATE_REQUIRES_ST_TMU1)) ==
       (STATE_REQUIRES_ST_TMU0 | STATE_REQUIRES_ST_TMU1)) &&
      !(hints & GR_STWHINT_ST_DIFF_TMU1))
    paramIndex &= ~STATE_REQUIRES_ST_TMU1;
  
  /* Turn off W for TMU1 if we have a previous W, and don't have a hint */ 
  if ((paramIndex & STATE_REQUIRES_W_TMU1) && !(hints & GR_STWHINT_W_DIFF_TMU1))
    paramIndex &= ~STATE_REQUIRES_W_TMU1;

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
  
  gc->state.paramIndex = paramIndex;

  _grRebuildDataList();
#undef FN_NAME
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

#if GLIDE_HW_TRI_SETUP
static void
_grUpdateTriPacketHdr(FxU32 paramMask, 
                      const GrCullMode_t cullMode)
{
  GR_DCL_GC;
  FxU32 sMode = ((cullMode != GR_CULL_DISABLE) 
                 ? kSetupCullEnable 
                 : kSetupPingPongDisable);
  if (sMode != kSetupPingPongDisable) sMode |= ((cullMode == GR_CULL_POSITIVE) 
                                                ? kSetupCullPositive 
                                                : kSetupCullNegative);

#define COLOR_COMP_ARGB ((SST_SETUP_RGB | SST_SETUP_A) << SSTCP_PKT3_PMASK_SHIFT)
#define COLOR_COMP_RGB  (SST_SETUP_RGB << SSTCP_PKT3_PMASK_SHIFT)
#define COLOR_COMP_MASK COLOR_COMP_ARGB 

#if GLIDE_DISPATCH_SETUP
  /* Setup custom triangle/strip procs.
   *
   * NB: Currently this selection is based entirely on if
   * we're sending color information. We should possibly
   * select on cpu type as well to determine if we should
   * do sw culling or not.
   */
  {
    GrTriSetupProcVector* curTriProcVector = TRISETUP_NORGB;

#if GLIDE_PACKED_RGB
    if ((paramMask & SSTCP_PKT3_PACKEDCOLOR) == SSTCP_PKT3_PACKEDCOLOR) {
      const FxU32 colorComp = paramMask & COLOR_COMP_MASK;

      switch(colorComp) {
      case COLOR_COMP_ARGB:
        curTriProcVector = TRISETUP_ARGB;
        break;

      case COLOR_COMP_RGB:
        curTriProcVector = TRISETUP_RGB;
        break;

        /* If no rgb data then it is not worthwhile to pack
         * just alpha so just mask off the packed color bit
         * and just use the looping proc.
         */
      default:
        curTriProcVector = TRISETUP_NORGB;
        paramMask &= ~SSTCP_PKT3_PACKEDCOLOR;
        break;
      }
    }
#endif

    gc->curArchProcs.triSetupProc = PROC_SELECT_TRISETUP(*curTriProcVector, cullMode);
  }
#else
#if GLIDE_PACKED_RGB
    if ((paramMask & SSTCP_PKT3_PACKEDCOLOR) == SSTCP_PKT3_PACKEDCOLOR) {
      const FxU32 colorComp = paramMask & COLOR_COMP_MASK;

      switch(colorComp) {
      case COLOR_COMP_ARGB:
      case COLOR_COMP_RGB:
        break;
      default:
        paramMask &= ~SSTCP_PKT3_PACKEDCOLOR;
        break;
      }
    }
#endif
#endif /* GLIDE_DISPATCH_SETUP */

  gc->cmdTransportInfo.paramMask = paramMask;

  /* Compute the common case packet 3 header which just needs
   * the vertex count and strip/command type to be completed.
   */
  gc->cmdTransportInfo.cullStripHdr = ((sMode << SSTCP_PKT3_SMODE_SHIFT) |
                                       paramMask |
                                       SSTCP_PKT3);
  
  /* Independent triangle hdr for grDrawTriangle */
  gc->cmdTransportInfo.triPacketHdr = (gc->cmdTransportInfo.cullStripHdr |
                                       (0x3UL << SSTCP_PKT3_NUMVERTEX_SHIFT) |
                                       SSTCP_PKT3_BDDBDD);

#if GLIDE_TRI_CULLING
  /* If we're doing sw culling for independent triangles then turn off
   * the hw culling so we're consistent. HW culling, however, remains
   * enabled for things using strips/fans.
   */
  if (cullMode != GR_CULL_DISABLE) {
    gc->cmdTransportInfo.triPacketHdr &= ~(kSetupCullEnable << SSTCP_PKT3_SMODE_SHIFT);
  }
#endif /* GLIDE_TRI_CULLING */
}
#endif /* GLIDE_HW_TRI_SETUP */

/*---------------------------------------------------------------------------
 * _grRebuildDataList
 *
 * NB: For CVG is *IMPERATIVE* that the writes to the parameter
 * dataList remain in the order below otherwise the parameters
 * will get written to the command packet in the wrong order.
 */
GR_DDFUNC(_grRebuildDataList, void, (void))
{
#define FN_NAME "_grRebuildDataList"
  GR_DCL_GC;
  GR_DCL_HW;
  int curTriSize, params;
  FxU32 i;
  SstRegs *tmu0;
  SstRegs *tmu1;

#if GLIDE_PACKED_RGB
  FxBool packedRGB = FXFALSE;
#endif /* GLIDE_PACKED_RGB */
    
#ifdef GLIDE_DEBUG
  static char *p_str[] = {"x","y","z","r","g","b","ooz","a","oow",
                          "s0","t0","w0","s1","t1","w1","s2","t2","w2"};
#endif /* GLIDE_DEBUG */
  
  GDBG_INFO(85,"(s) paramHints=0x%x paramIndex=0x%x\n", FN_NAME,
            gc->state.paramHints,gc->state.paramIndex);
  
  curTriSize = params = 0;
  i = gc->state.paramIndex;
#ifdef GLIDE_USE_ALT_REGMAP
  hw = SST_WRAP(hw,128);        /* wrap 128-257 are ALTernate register mappings */
#endif /* GLIDE_USE_ALT_REGMAP */

  tmu0 = SST_CHIP(hw,0xE); /* tmu 0,1,2 */
  tmu1 = SST_CHIP(hw,0xC); /* tmu 1,2 */

#if GLIDE_HW_TRI_SETUP
  gc->cmdTransportInfo.paramMask = 0x00;
#endif /* GLIDE_HW_TRI_SETUP */

  if (i & STATE_REQUIRES_IT_DRGB) {
#if GLIDE_HW_TRI_SETUP
    gc->cmdTransportInfo.paramMask  |= SST_SETUP_RGB;
    gc->tsuDataList[curTriSize + 0] = GR_VERTEX_R_OFFSET << 2;

    /* When using packed color we only add *ONE* item to the data list
     * and this signals the entire color set since it is not possible
     * to specify a single color component in any packet.  
     */
#if !GLIDE_PACKED_RGB
    gc->tsuDataList[curTriSize + 1] = GR_VERTEX_G_OFFSET << 2;
    gc->tsuDataList[curTriSize + 2] = GR_VERTEX_B_OFFSET << 2;
#endif /* !GLIDE_PACKED_RGB */
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_R_OFFSET << 2;
    gc->regDataList[curTriSize + 1].i    = GR_VERTEX_G_OFFSET << 2;
    gc->regDataList[curTriSize + 2].i    = GR_VERTEX_B_OFFSET << 2;

    gc->regDataList[curTriSize + 0].addr = (float*)&hw->RED;
    gc->regDataList[curTriSize + 1].addr = (float*)&hw->GRN;
    gc->regDataList[curTriSize + 2].addr = (float*)&hw->BLU;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

#if GLIDE_PACKED_RGB
    curTriSize += 1;
    params += 1;

    packedRGB = FXTRUE;
#else /* !GLIDE_PACKED_RGB */
    curTriSize += 3;
    params += 3;
#endif /* !GLIDE_PACKED_RGB */
  }

  if (i & STATE_REQUIRES_IT_ALPHA) {
#if GLIDE_HW_TRI_SETUP
    gc->cmdTransportInfo.paramMask   |= SST_SETUP_A;
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_A_OFFSET << 2;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_A_OFFSET << 2;
    gc->regDataList[curTriSize + 0].addr = (float*)&hw->ALF;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

    curTriSize += 1;

#if GLIDE_PACKED_RGB
    {
      /* Only increment the parameter packet size if we have not already
       * added the rgb fields.  
       */
      if ((i & STATE_REQUIRES_IT_DRGB) == 0) params += 1;
      packedRGB = FXTRUE;
    }
#else /* !GLIDE_PACKED_RGB */
    params += 1;
#endif /* !GLIDE_PACKED_RGB */
  }
  
  if (i & STATE_REQUIRES_OOZ) {
#if GLIDE_HW_TRI_SETUP
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_OOZ_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_Z;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_OOZ_OFFSET << 2;
    gc->regDataList[curTriSize + 0].addr = (float*)&hw->Z;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

    curTriSize += 1;
    params += 1;
  }

  /* we squeeze FBI.OOW in here for sequential writes in the simple case */
  if (i & STATE_REQUIRES_OOW_FBI) {
#if GLIDE_HW_TRI_SETUP
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_OOW_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_Wfbi;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_OOW_OFFSET << 2;
    gc->regDataList[curTriSize + 0].addr = (float*)&hw->W;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

    curTriSize += 1;
    params += 1;
  }

#if GLIDE_FP_CLAMP_TEX
#if !GLIDE_FP_CLAMP || !GLIDE_HW_TRI_SETUP
#error "Does not make sense to have GLIDE_FP_CLAMP_TEX w/o GLIDE_FP_CLAMP or GLIDE_HW_TRI_SETUP"
#endif /* !GLIDE_FP_CLAMP || !GLIDE_HW_TRI_SETUP */

  /* Simplify the test for clamping only the texture parameters by
   * sticking an extra empty slot into the data list.
   *
   * NB: This means that the code that runs through the parameter list
   * needs to increment the datalist pointer before moving onto the
   * texture parameters.
   */
  gc->tsuDataList[curTriSize++] = 0;
#endif /* GLIDE_FP_CLAMP_TEX */
  
  /* NOTE: this is the first */
  if (i & STATE_REQUIRES_W_TMU0) {
#if GLIDE_HW_TRI_SETUP
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_OOW_TMU0_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_W0;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_OOW_TMU0_OFFSET << 2;
    gc->regDataList[curTriSize + 0].addr = (float*)&tmu0->W;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

    curTriSize += 1;
    params += 1;
  }
  
  /* TMU0 --------------------------------- */
  /* always output to ALL chips, saves from having to change CHIP field */
  if (i & STATE_REQUIRES_ST_TMU0) {
#if GLIDE_HW_TRI_SETUP
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_SOW_TMU0_OFFSET << 2;
    gc->tsuDataList[curTriSize + 1]      = GR_VERTEX_TOW_TMU0_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_ST0;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_SOW_TMU0_OFFSET << 2;
    gc->regDataList[curTriSize + 1].i    = GR_VERTEX_TOW_TMU0_OFFSET << 2;
    gc->regDataList[curTriSize + 0].addr = (float*)&tmu0->S;
    gc->regDataList[curTriSize + 1].addr = (float*)&tmu0->T;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

    curTriSize += 2;
    params += 2;
  }
    
  /* TMU1 --------------------------------- */
  if (i & STATE_REQUIRES_W_TMU1) {
#if GLIDE_HW_TRI_SETUP
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_OOW_TMU1_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_W1;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_OOW_TMU1_OFFSET << 2;
    gc->regDataList[curTriSize + 0].addr = (float*)&tmu1->W;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

    curTriSize += 1;
    params += 1;

  }

  if (i & STATE_REQUIRES_ST_TMU1) {
#if GLIDE_HW_TRI_SETUP
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_SOW_TMU1_OFFSET << 2;
    gc->tsuDataList[curTriSize + 1]      = GR_VERTEX_TOW_TMU1_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_ST1;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_SOW_TMU1_OFFSET << 2;
    gc->regDataList[curTriSize + 0].addr = (float*)&tmu1->S;
    gc->regDataList[curTriSize + 1].i    = GR_VERTEX_TOW_TMU1_OFFSET << 2;
    gc->regDataList[curTriSize + 1].addr = (float*)&tmu1->T;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

    curTriSize += 2;
    params += 2;
  }
  
#if (GLIDE_NUM_TMU > 2)
#error "GLIDE_NUM_TMU > 2: Write this code."
#endif

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
  gc->regDataList[curTriSize].i = 0;     /* terminate the list with 0,*      */
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

#if GLIDE_HW_TRI_SETUP
  gc->tsuDataList[curTriSize]   = 0;
#endif /* GLIDE_HW_TRI_SETUP */

  curTriSize++;

#if GLIDE_HW_TRI_SETUP
  /* Per vertex size: (xy [required] + parameters) * param size (32 bit word) */
  _GlideRoot.curVertexSize = ((2 + params) << 2);

  /* Packet size: 3 * vertex size */
  _GlideRoot.curTriSize = (_GlideRoot.curVertexSize << 1) + _GlideRoot.curVertexSize;

  /* Shift the parameter mask to just write into the packet header. */
  GDBG_INFO(191, "CVG ParamMask: 0x%X\n", gc->cmdTransportInfo.paramMask);
  gc->cmdTransportInfo.paramMask <<= SSTCP_PKT3_PMASK_SHIFT;

#if GLIDE_PACKED_RGB
  if (packedRGB) gc->cmdTransportInfo.paramMask |= SSTCP_PKT3_PACKEDCOLOR;
#endif /* GLIDE_PACKED_RGB */

  /* Update common packet 3 headers */
  _grUpdateTriPacketHdr(gc->cmdTransportInfo.paramMask, gc->state.cull_mode);
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
  gc->regDataList[curTriSize].i    = 1;
  gc->regDataList[curTriSize].addr = (float*)&hw->FtriangleCMD;

  /* 6 X,Y values plus AREA = 7, plus parameters */
  _GlideRoot.paramCount = params;
  _GlideRoot.curTriSize = (6 + curTriSize + (params << 1)) << 2;

  /* Need to know tri size without gradients for planar polygons */
  _GlideRoot.curTriSizeNoGradient = _GlideRoot.curTriSize - (params << 3);
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

#if GLIDE_FP_CLAMP_TEX
  /* Stick one more 0 in the dataList so that the texture clamping
   * loop does not go one more iteration.
   */
  gc->tsuDataList[++curTriSize]   = 0;
#endif /* GLIDE_FP_CLAMP_TEX */
  
#ifdef GDBG_INFO_ON
#if GLIDE_HW_TRI_SETUP
  for (i = 0; gc->tsuDataList[i]; i++) {
    GDBG_INFO(282,"    tsuDataList[%d] = %2d [%s]\n",
              i, gc->tsuDataList[i] >> 2, p_str[gc->tsuDataList[i] >> 2]);
  }
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
  for (i = 0; gc->regDataList[i].i; i++) {
    GDBG_INFO(282,"    dataList[%d] = %2d 0x%x [%s]\n",
              i, gc->regDataList[i].i >> 2, gc->regDataList[i].addr,
              p_str[gc->regDataList[i].i >> 2]);
  }
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */
#endif /* GDBG_INFO_ON */

#undef FN_NAME
} /* _grRebuildDataList */

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
