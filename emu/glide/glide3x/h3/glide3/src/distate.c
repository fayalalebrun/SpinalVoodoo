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
 ** Revision 1.2.4.3  2004/10/04 09:26:30  dborca
 ** DOS/OpenWatcom support
 **
 ** Revision 1.2.4.2  2003/11/07 13:38:38  dborca
 ** unite the clans
 **
 ** Revision 1.2.4.1  2003/06/29 18:45:55  guillemj
 ** Fixed preprocessor invalid token errors.
 **
 ** Revision 1.2  2000/11/24 18:36:48  alanh
 ** Add new grStippleMode and grStipplePattern functions for both Voodoo3 and
 ** Voodoo5 hardware.
 **
 ** Revision 1.1.1.1  1999/11/24 21:44:54  joseph
 ** Initial checkin for SourceForge
 **
** 
** 5     4/06/99 3:54p Dow
** Alt tab again.
** 
** 31    3/24/99 6:17p Peter
** streamlined (made more dangerouse) state validation
** 
** 30    3/22/99 5:37p Peter
** added avenger to grChroma supported list
** 
** 29    3/19/99 12:21p Atai
** update triproc
** 
** 28    3/05/99 10:31p Peter
** coalesced texture state change nop into validate state
** 
** 27    3/02/99 2:03p Peter
** removed no_check variant that led me astray
** 
** 26    1/25/99 6:31p Peter
** removed overly restrictive assertion for depth buffer function vs mask
** 
** 25    1/15/99 10:52a Peter
** cleanup lazy evaluation of fbzMode for grDepthMask and grColorMask
** 
** 24    11/21/98 10:19a Atai
** fixed test37 grChromaRangeModeExt error and rename functions
** 
** 23    11/17/98 4:27p Atai
** fixed gc->triSetupProc for clip coords grDrawTriangle
** 
** 22    10/12/98 9:51a Peter
** dynamic 3DNow!(tm)
** 
** 21    9/08/98 7:19p Atai
** fix debug info. added underline for internal routines and some \n
** 
** 20    8/30/98 10:55p Jdt
** remove invalidate macro ( promoted to fxglide.h )
** 
** 19    8/30/98 1:34p Dow
** State & other optimizations
** 
** 18    7/18/98 12:23a Jdt
** Changes to reflect new shadow register structure.
** 
** 17    7/16/98 8:15a Jdt
** fxcmd.h
** 
** 16    7/13/98 10:43p Jdt
** Removed grSstControl()...
** 
** 
** 15    7/02/98 1:54p Atai
** grDepthBiasLevel argument is FxI32
** 
** 14    6/21/98 11:56a Atai
** fixed fogcoord paramindex
** 
** 13    6/19/98 12:59p Atai
** fixed grDepthBiasLevel
** 
** 12    6/11/98 6:04p Atai
** added aa case for OGL
** 
** 11    6/10/98 12:53p Atai
** replace grSstControl with grEnable/grDisable(GR_PASSTHRU)
** 
** 10    6/03/98 2:43p Atai
** fixed chromarange
** 
** 9     5/29/98 6:39p Atai
** fix chromarange
** 
** 8     5/05/98 2:22p Peter
** banshee is really banshee
** 
** 7     4/30/98 10:34a Peter
** merged w/ cvg again
** 
** 7     4/24/98 2:21p Atai
** call _grUpdateParamIndex() if the state is dirty
** 
** 6     4/23/98 7:33p Atai
** grValidateState: send register in a group
** 
** 5     2/24/98 6:01p Atai
** modify validatestate
** 
** 4     1/30/98 1:19p Atai
** fixed chromarange
** 
** 3     1/28/98 2:26p Atai
** fix grDisable GR_SHAMELESS_PLUG
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 17    1/13/98 12:42p Atai
 * fixed grtexinfo, grVertexLayout, and draw triangle
 * 
 * 16    1/10/98 4:01p Atai
 * inititialize vertex layout, viewport, added defines
 * 
 * 15    1/05/98 6:06p Atai
 * glide extension stuff
 * 
 * 14    12/17/97 4:05p Atai
 * added grChromaRange(), grGammaCorrecionRGB(), grRest(), and grGet()
 * functions
 * 
 * 13    12/16/97 11:38a Atai
 * added grChromaRange()
 * 
 * 12    12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 10    12/12/97 1:30p Atai
 * remove fp z  buffer
 * 
 * 8     12/08/97 10:44a Atai
 * added entry point for grCoordinateSpace(), grDepthRange(), and
 * grViewport()
 * 
 * 7     11/13/97 4:38p Atai
 * invalidate lfbMode and c0c1
 * 
 * 6     11/10/97 5:20p Atai
 * added factor for grAlphaCombine and remove extra  _grChromakeyMode
 * 
 * 5     11/07/97 11:22a Atai
 * remove GR_*_SMOOTH. use GR_SMOOTH
 * 
 * 4     10/15/97 7:33a Dow
 * Made _grValidateState use central routine for writing data
 * 
 * 3     10/14/97 4:18p Atai
 * added grEnable and grDisable
 * 
 * 2     10/10/97 2:57p Dow
 * Minor adjustments
 * 
 * 1     10/09/97 5:19p Dow
 * State Monster file
 */

#ifdef GLIDE3

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

/*=============================================================================
**  Replacement state routines.
**
**  These routines store away their arguments, and mark a piece of glide state
**  as invalid.  The next time a rendering primitive is called, the state will
**  be invalid, and grValidateState will be called.  See that routine for more
**  info.
**===========================================================================*/

/*
   Some macros for use in this file only
 */

#define STOREARG(function, arg) \
gc->state.stateArgs.function ## Args.arg = arg

#define LOADARG(function, arg) \
gc->state.stateArgs.function ## Args.arg


#define NOTVALID(regset) \
(gc->state.invalid & regset ## BIT)

#define SETVALID(regset) \
(gc->state.invalid &= ~(regset ## BIT))

#define ENABLEMODE(mode) \
gc->state.grEnableArgs.mode = GR_MODE_ENABLE;

#define DISABLEMODE(mode) \
gc->state.grEnableArgs.mode = GR_MODE_DISABLE;

/*-------------------------------------------------------------------
  Function: grAlphaBlendFunction
  Date: 06-Oct-97
  Implementor(s): dow
  Description:
  Inform Glide that the Alpha Blend Function has been modified.
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grAlphaBlendFunction, void , (GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df, GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df) )
{
 #define FN_NAME "grAlphaBlendFunction"
  GR_BEGIN_NOFIFOCHECK("grAlphaBlendFunction\n", 87);

  /* Invalidate AlphaMode */
  INVALIDATE(alphaMode);

  STOREARG(grAlphaBlendFunction, rgb_sf);
  STOREARG(grAlphaBlendFunction, rgb_df);
  STOREARG(grAlphaBlendFunction, alpha_sf);
  STOREARG(grAlphaBlendFunction, alpha_df);

 #undef FN_NAME
} /* grAlphaBlendFunction */

/*-------------------------------------------------------------------
  Function: grAlphaTestFunction
  Date: 06-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grAlphaTestFunction, void , (GrCmpFnc_t fnc) )
{
 #define FN_NAME "grAlphaTestFunction"
  GR_BEGIN_NOFIFOCHECK("grAlphaTestFunction\n", 87);

  /* Invalidate AlphaMode */
  INVALIDATE(alphaMode);

  STOREARG(grAlphaTestFunction, fnc);

 #undef FN_NAME
} /* grAlphaTestFunction */

/*-------------------------------------------------------------------
  Function: grAlphaTestReferenceValue
  Date: 06-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grAlphaTestReferenceValue, void , (GrAlpha_t value) )
{
 #define FN_NAME "grAlphaTestReferenceValue"
  GR_BEGIN_NOFIFOCHECK("grAlphaTestReferenceValue\n", 87);

  INVALIDATE(alphaMode);

  STOREARG(grAlphaTestReferenceValue,value);

 #undef FN_NAME
} /* grAlphaTestReferenceValue */

/*-------------------------------------------------------------------
  Function: grAlphaCombine
  Date: 06-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grAlphaCombine, void ,
         (GrCombineFunction_t function, GrCombineFactor_t factor, 
          GrCombineLocal_t local, GrCombineOther_t other, FxBool invert) )
{
 #define FN_NAME "grAlphaCombine"
  GR_BEGIN_NOFIFOCHECK("grAlphaCombine\n", 85);

  INVALIDATE(fbzColorPath);

  STOREARG(grAlphaCombine, function);
  STOREARG(grAlphaCombine, factor);
  STOREARG(grAlphaCombine, local);
  STOREARG(grAlphaCombine, other);
  STOREARG(grAlphaCombine, invert);

 #undef FN_NAME
} /* grAlphaCombine */

/*-------------------------------------------------------------------
  Function: grAlphaControlsITRGBLighting
  Date: 07-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grAlphaControlsITRGBLighting, void , (FxBool enable) )
{
#define FN_NAME "grAlphaControlsITRGBLighting"
  GR_BEGIN_NOFIFOCHECK("grAlphaControlsITRGBLighting\n",85);

  INVALIDATE(fbzColorPath);

  STOREARG(grAlphaControlsITRGBLighting, enable);

#undef FN_NAME
} /* grAlphaControlsITRGBLighting */

/*-------------------------------------------------------------------
  Function: grColorCombine
  Date: 07-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grColorCombine, void , (GrCombineFunction_t function,
                                   GrCombineFactor_t factor,
                                   GrCombineLocal_t local,
                                   GrCombineOther_t other, FxBool invert) )
{
#define FN_NAME "grColorCombine"
  GR_BEGIN_NOFIFOCHECK("grColorCombine\n",85);

  INVALIDATE(fbzColorPath);

  STOREARG(grColorCombine, function);
  STOREARG(grColorCombine, factor);
  STOREARG(grColorCombine, local);
  STOREARG(grColorCombine, other);
  STOREARG(grColorCombine, invert);

#undef FN_NAME
} /* grColorCombine */


/*-------------------------------------------------------------------
  Function: grChromakeyMode
  Date: 07-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grChromakeyMode, void , (GrChromakeyMode_t mode) )
{
#define FN_NAME "grChromakeyMode"
  GR_BEGIN_NOFIFOCHECK("grChromakeyMode\n",85);

  INVALIDATE(fbzMode);

  STOREARG(grChromakeyMode, mode);

#undef FN_NAME
} /* grChromakeyMode */

/*-------------------------------------------------------------------
  Function: grChromaRangeMode
  Date: 05-Jan-98
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grChromaRangeMode, void , (GrChromakeyMode_t mode) )
{
#define FN_NAME "grChromaRangeMode"
  GR_BEGIN_NOFIFOCHECK("grChromaRangeMode\n",85);

  /*
  ** We need to enable both fbzMode chromakeymode and chrmarange mdoe
  */ 
  INVALIDATE(fbzMode);
  INVALIDATE(chromaRange);

  STOREARG(grChromaRange, mode);

#undef FN_NAME
} /* grChromaRangeMode */

/*-------------------------------------------------------------------
  Function: grChromaRange
  Date: 15-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grChromaRange, void , (GrColor_t color, GrColor_t range, GrChromaRangeMode_t match_mode) )
{
#define FN_NAME "grChromaRange"
  GR_BEGIN_NOFIFOCHECK("grChromaRange\n",85);

  INVALIDATE(chromaKey);
  INVALIDATE(chromaRange);

  STOREARG(grChromakeyValue, color);
  STOREARG(grChromaRange, range);
  STOREARG(grChromaRange, match_mode);

#undef FN_NAME
} /* grChromaRange */

/*-------------------------------------------------------------------
  Function: grChromakeyValue
  Date: 09-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_ENTRY(grChromakeyValue, void , (GrColor_t color) )
{
#define FN_NAME "grChromakeyValue"
  GR_BEGIN_NOFIFOCHECK("grChromakeyMode\n",85);

  INVALIDATE(chromaKey);

  STOREARG(grChromakeyValue, color);

#undef FN_NAME
} /* grChromakeyValue */

/*-------------------------------------------------------------------
  Function: grDeptMask
  Date: 07-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grDepthMask, void , (FxBool enable) )
{
 #define FN_NAME "grDepthMask"

  GR_BEGIN_NOFIFOCHECK("grDepthMask\n", 85);

  INVALIDATE(fbzMode);

  STOREARG(grDepthMask, enable);

 #undef FN_NAME
} /* grDeptMask */


/*-------------------------------------------------------------------
  Function: grDepthBufferFunction
  Date: 07-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grDepthBufferFunction, void , (GrCmpFnc_t fnc) )
{
 #define FN_NAME "grDepthBufferFunction"
  GR_BEGIN_NOFIFOCHECK("grDepthBufferFunction\n", 85);

  INVALIDATE(fbzMode);

  STOREARG(grDepthBufferFunction, fnc);

 #undef FN_NAME
} /* grDepthBufferFunction */


/*-------------------------------------------------------------------
  Function: grDepthBufferMode
  Date: 07-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grDepthBufferMode, void , (GrDepthBufferMode_t mode) )
{
 #define FN_NAME "grDepthBufferMode"

  GR_BEGIN_NOFIFOCHECK("grDepthBufferMode\n", 85);

  INVALIDATE(fbzMode);

  STOREARG(grDepthBufferMode, mode);

 #undef FN_NAME
} /* grDepthBufferMode */

/*-------------------------------------------------------------------
  Function: grStipplePattern
  Date: 23-Nov-2000
  Implementor(s): alanh
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grStipplePattern, void , (GrStipplePattern_t stipple))
{
 #define FN_NAME "grStipplePattern"

  GR_BEGIN_NOFIFOCHECK("grStippleMode\n", 85);

  INVALIDATE(stipple);

  STOREARG(grStipplePattern, stipple);

 #undef FN_NAME
} /* grStipplePattern */

 /*-------------------------------------------------------------------
  Function: grStippleMode
  Date: 23-Nov-2000
  Implementor(s): alanh
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grStippleMode, void , (GrStippleMode_t mode) )
{
 #define FN_NAME "grStippleMode"

  GR_BEGIN_NOFIFOCHECK("grStippleMode\n", 85);

  INVALIDATE(fbzMode);

  STOREARG(grStippleMode, mode);

 #undef FN_NAME
} /* grStippleMode */

/*-------------------------------------------------------------------
  Function: grDitherMode
  Date: 07-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grDitherMode, void , (GrDitherMode_t mode) )
{
 #define FN_NAME "grDitherMode"

  GR_BEGIN_NOFIFOCHECK("grDitherMode\n", 85);

  INVALIDATE(fbzMode);

  STOREARG(grDitherMode, mode);

 #undef FN_NAME
} /* grDitherMode */

/*-------------------------------------------------------------------
  Function: grRenderBuffer
  Date: 07-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grRenderBuffer, void , (GrBuffer_t buffer) )
{
 #define FN_NAME "grRenderBuffer"

  GR_BEGIN_NOFIFOCHECK("grRenderBuffer\n", 85);

  INVALIDATE(fbzMode);

  STOREARG(grRenderBuffer, buffer);

 #undef FN_NAME
} /* grRenderBuffer */

/*-------------------------------------------------------------------
  Function: grColorMask
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_ENTRY(grColorMask, void , (FxBool rgb, FxBool alpha) )
{
#define FN_NAME "grColorMask"

  GR_BEGIN_NOFIFOCHECK("grColorMask\n", 85);

  INVALIDATE(fbzMode);

  STOREARG(grColorMask, rgb);

  /* NB: The api is defined such that if depth buffering is enabled
   * the alpha mask parameter is ignored. So we need to track the call
   * order dependency between grColorMask and grDepthMask here.
   */
  gc->state.stateArgs.grColorMaskArgs.alpha = ((LOADARG(grDepthMask, enable) && alpha)
                                               ? -1
                                               : alpha);
#undef FN_NAME
} /* grColorMask */

/*-------------------------------------------------------------------
  Function: grSstOrigin
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grSstOrigin, void , (GrOriginLocation_t origin) )
{
 #define FN_NAME "grSstOrigin"
  GR_BEGIN_NOFIFOCHECK("grSstOrigin\n", 83);

  INVALIDATE(fbzMode);

  STOREARG(grSstOrigin, origin);

#undef FN_NAME
} /* grSstOrigin */

/*-------------------------------------------------------------------
  Function: grClipWindow
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grClipWindow, void , (FxU32 minx, FxU32 miny, FxU32 maxx,
                               FxU32 maxy) ) 
{
#define FN_NAME "grClipWindow"

  GR_BEGIN_NOFIFOCHECK("grClipWindow\n", 83);

  INVALIDATE(clipRegs);

  STOREARG(grClipWindow, minx);
  STOREARG(grClipWindow, miny);
  STOREARG(grClipWindow, maxx);
  STOREARG(grClipWindow, maxy);

#undef FN_NAME
} /* grClipWindow */

/*-------------------------------------------------------------------
  Function: grDepthBiasLevel
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grDepthBiasLevel, void , (FxI32 level) )
{
#define FN_NAME "grDepthBiasLevel"

  GR_BEGIN_NOFIFOCHECK("grDepthBiasLevel\n", 83);

  INVALIDATE(zaColor);

  STOREARG(grDepthBiasLevel, level);

#undef FN_NAME
} /* grDepthBiasLevel */


/*-------------------------------------------------------------------
  Function: grFogMode
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grFogMode, void , (GrFogMode_t mode) )
{
#define FN_NAME "grFogMode"

  GR_BEGIN_NOFIFOCHECK("grFogMode\n", 83);

  INVALIDATE(fogMode);

  STOREARG(grFogMode, mode);

  if (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE)
    INVALIDATE(fbzMode);

#undef FN_NAME
} /* grFogMode */


/*-------------------------------------------------------------------
  Function: grFogColorValue
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grFogColorValue, void , (GrColor_t color) )
{
#define FN_NAME "grFogColorValue"
  GR_BEGIN_NOFIFOCHECK("grFogColorValue\n", 83);

  INVALIDATE(fogColor);

  STOREARG(grFogColorValue, color);

#undef FN_NAME
} /* grFogColorValue */

/*-------------------------------------------------------------------
  Function: grLfbWriteColorFormat
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grLfbWriteColorFormat, void , (GrColorFormat_t colorFormat) )
{
#define FN_NAME "grLfbWriteColorFormat"

  GR_BEGIN_NOFIFOCHECK("grLfbWriteColorFormat\n", 82);

  INVALIDATE(lfbMode);

  STOREARG(grLfbWriteColorFormat, colorFormat);

#undef FN_NAME
} /* grLfbWriteColorFormat */

/*-------------------------------------------------------------------
  Function: grLfbWriteColorSwizzle
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grLfbWriteColorSwizzle, void , (FxBool swizzleBytes, FxBool
                                         swapWords) ) 
{
#define FN_NAME "grLfbWriteColorSwizzle"
  GR_BEGIN_NOFIFOCHECK("grLfbWriteColorSwizzle\n", 82);

  INVALIDATE(lfbMode);

  STOREARG(grLfbWriteColorSwizzle, swizzleBytes);
  STOREARG(grLfbWriteColorSwizzle, swapWords);

#undef FN_NAME
} /* grLfbWriteColorSwizzle */


/*-------------------------------------------------------------------
  Function: grConstantColorValue
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grConstantColorValue, void , (GrColor_t color) )
{
#define FN_NAME "grConstantColorValue"

  GR_BEGIN_NOFIFOCHECK("grConstantColorValue\n", 85);

  INVALIDATE(c0c1);

  STOREARG(grConstantColorValue, color);

#undef FN_NAME
} /* grConstantColorValue */


/*==========================================================================*/


/*-------------------------------------------------------------------
  Function: grValidateState
  Date: 08-Oct-97
  Implementor(s): dow
  Description:
  State Validation:
  Once a rendering primitive has determined that the state is invalid,
  it calls this routine.  grValidateState then goes through valid
  markers and flushes all invalid state.
  -------------------------------------------------------------------*/
void GR_CDECL
_grValidateState()
{
#define FN_NAME "_grValidateState"
#define STATE_REG_BASE             (offsetof(SstRegs, fbzColorPath) >> 2UL)
#define STATE_REG_END              (offsetof(SstRegs, chromaRange) >> 2UL)
#define STATE_REG_MASK(__regName)  (0x01UL << ((offsetof(SstRegs, __regName) >> 2UL) - STATE_REG_BASE))
  FxU32 
    mask = 0,
    reg_cnt = 0,
    writeShadow[STATE_REG_END - STATE_REG_BASE + 1];

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%X)\n", gc->state.invalid);

  /* NB: The values in the write shadow must be set in register order
   * for the unsafe reg_group write.  
   */
  if (NOTVALID(fbzColorPath)) {
    FxU32 
      oldTextureEnabled = gc->state.shadow.fbzColorPath & SST_ENTEXTUREMAP;

    _grAlphaCombine(LOADARG(grAlphaCombine, function),
                    LOADARG(grAlphaCombine, factor),
                    LOADARG(grAlphaCombine, local),
                    LOADARG(grAlphaCombine, other),
                    LOADARG(grAlphaCombine, invert));
    _grAlphaControlsITRGBLighting(LOADARG(grAlphaControlsITRGBLighting, enable)); 
    _grColorCombine(LOADARG(grColorCombine, function), 
                    LOADARG(grColorCombine, factor),
                    LOADARG(grColorCombine, local),
                    LOADARG(grColorCombine, other),
                    LOADARG(grColorCombine, invert));

    /* transition into/out of texturing ... add nopCMD */
    if ((oldTextureEnabled ^ (gc->state.shadow.fbzColorPath & SST_ENTEXTUREMAP)) != 0x00UL) {
      GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
      GR_SET(BROADCAST_ID, hw, nopCMD, 0);
      GR_CHECK_SIZE();
    }

    writeShadow[reg_cnt] = gc->state.shadow.fbzColorPath;

    mask |= STATE_REG_MASK(fbzColorPath);
    reg_cnt++;
  }

  if (NOTVALID(fogMode)) {
    _grFogMode(LOADARG(grFogMode, mode));

    writeShadow[reg_cnt] = gc->state.shadow.fogMode;

    mask |= STATE_REG_MASK(fogMode);
    reg_cnt++;
  }

  if (NOTVALID(alphaMode)) {
    _grAlphaBlendFunction(LOADARG(grAlphaBlendFunction, rgb_sf),
                          LOADARG(grAlphaBlendFunction, rgb_df),
                          LOADARG(grAlphaBlendFunction, alpha_sf),
                          LOADARG(grAlphaBlendFunction, alpha_df));
    _grAlphaTestFunction(LOADARG(grAlphaTestFunction, fnc));
    _grAlphaTestReferenceValue(LOADARG(grAlphaTestReferenceValue, value));

    writeShadow[reg_cnt] = gc->state.shadow.alphaMode;

    mask |= STATE_REG_MASK(alphaMode);
    reg_cnt++;
  }

  if (NOTVALID(fbzMode)) {
    _grChromakeyMode(LOADARG(grChromakeyMode, mode));
    _grChromaMode(LOADARG(grChromaRange, mode));
    _grDepthBufferFunction(LOADARG(grDepthBufferFunction, fnc));
    _grDepthBufferMode(LOADARG(grDepthBufferMode, mode));
    _grDitherMode(LOADARG(grDitherMode, mode));
    _grStippleMode(LOADARG(grStippleMode, mode));
    _grSstOrigin(LOADARG(grSstOrigin, origin));
    _grRenderBuffer(LOADARG(grRenderBuffer, buffer));

    /* NB: The alpha mask enable parameter to grColorMask is *DEFINED*
     * to be ignored if depth buffering is enabled. So we need to keep
     * track of the call order even when doing lazy state
     * evaluation. However, both alpha and depth buffering cannot be
     * enabled in fbzMode otherwise alpha buffering 'wins' and depth
     * writes don't happen correctly in the clear.  We need to make
     * sure taht only one of them is active at once which is what the
     * old code actually wanted to do, but failed to do.
     * 
     * Who defined the api to be this way? Ick!  
     */
    {
      const FxBool
        enableDepthMask = LOADARG(grDepthMask, enable),
        enableColorMask = LOADARG(grColorMask, rgb);
      const FxI32
        enableAlphaMask = LOADARG(grColorMask, alpha);
      FxU32
        fbzMode = (gc->state.shadow.fbzMode & 
                   ~(SST_RGBWRMASK | SST_ZAWRMASK | SST_ENALPHABUFFER));

#if GLIDE_DEBUG
      _grDepthMask(enableDepthMask);
      _grColorMask(enableColorMask, enableAlphaMask);
#endif /* GLIDE_DEBUG */
      
      if (enableColorMask) fbzMode |= SST_RGBWRMASK;
      if (enableDepthMask) {
        GR_CHECK_COMPATABILITY(FN_NAME,
                               ((enableAlphaMask != 0) && (gc->state.shadow.fbzMode & SST_ENALPHABUFFER)),
                               "alpha writes enabled even though depth buffering");

        fbzMode |= SST_ZAWRMASK;
      } else if (enableAlphaMask > 0) {
        GR_CHECK_COMPATABILITY(FN_NAME,
                               (fbzMode & SST_ENDEPTHBUFFER),
                               "alpha writes enabled even though depth buffering");

        GR_CHECK_COMPATABILITY(FN_NAME,
                               (gc->grAuxBuf == 0),
                               "cannot enable alpha buffering w/o allocating one");

        fbzMode |= SST_ENALPHABUFFER | SST_ZAWRMASK;
      }

      /* Write back shadow */
      gc->state.shadow.fbzMode = fbzMode;
    }

    writeShadow[reg_cnt] = gc->state.shadow.fbzMode;

    mask |= STATE_REG_MASK(fbzMode);
    reg_cnt++;
  }

  if (NOTVALID(stipple)) {
    gc->state.shadow.stipple = LOADARG(grStipplePattern, stipple);
    REG_GROUP_BEGIN(BROADCAST_ID, stipple, 1, 0x01);
    {
      REG_GROUP_SET(hw, stipple, gc->state.shadow.stipple);
    }
    REG_GROUP_END();
  }

  if (NOTVALID(lfbMode)) {
    FxU32
      lfbMode = gc->state.shadow.lfbMode;

    lfbMode &= ~(SST_LFB_RGBALANES | 
                 SST_LFB_WRITE_SWAP16 | 
                 SST_LFB_WRITE_BYTESWAP);

    lfbMode |= ((LOADARG(grLfbWriteColorFormat, colorFormat) << SST_LFB_RGBALANES_SHIFT) |
                (LOADARG(grLfbWriteColorSwizzle, swizzleBytes) << 11UL) |
                (LOADARG(grLfbWriteColorSwizzle, swapWords) << 12UL));

#if GLIDE_DEBUG
    _grLfbWriteColorFormat(LOADARG(grLfbWriteColorFormat, colorFormat));
    _grLfbWriteColorSwizzle(LOADARG(grLfbWriteColorSwizzle, swizzleBytes),
                            LOADARG(grLfbWriteColorSwizzle, swapWords));
#endif /* GLIDE_DEBUG */
    GR_ASSERT(lfbMode == gc->state.shadow.lfbMode);

    gc->state.shadow.lfbMode = lfbMode;
    writeShadow[reg_cnt] = lfbMode;

    mask |= STATE_REG_MASK(lfbMode);
    reg_cnt++;
  }

  if (NOTVALID(clipRegs)) {
    _grClipWindow(LOADARG(grClipWindow, minx),
                  LOADARG(grClipWindow, miny),
                  LOADARG(grClipWindow, maxx),
                  LOADARG(grClipWindow, maxy));

    writeShadow[reg_cnt + 0] = gc->state.shadow.clipLeftRight;
    writeShadow[reg_cnt + 1] = gc->state.shadow.clipBottomTop;

    mask |= (STATE_REG_MASK(clipLeftRight) | STATE_REG_MASK(clipBottomTop));
    reg_cnt += 2;
  }

  if (NOTVALID(fogColor)) {
#if GLIDE_DEBUG
    _grFogColorValue(LOADARG(grFogColorValue, color));
#endif /* GLIDE_DEBUG */

    gc->state.shadow.fogColor = LOADARG(grFogColorValue, color);
    _grSwizzleColor(&gc->state.shadow.fogColor);

    writeShadow[reg_cnt] = gc->state.shadow.fogColor;

    mask |= STATE_REG_MASK(fogColor);
    reg_cnt++;
  }

  if (NOTVALID(zaColor)) {
    FxU32
      zaColor = gc->state.shadow.zaColor;

    zaColor &= ~SST_ZACOLOR_DEPTH;
    zaColor |= (LOADARG(grDepthBiasLevel, level) & SST_ZACOLOR_DEPTH);

#if GLIDE_DEBUG
    _grDepthBiasLevel(LOADARG(grDepthBiasLevel, level));
#endif /* GLIDE_DEBUG */
    GR_ASSERT(zaColor == gc->state.shadow.zaColor);

    gc->state.shadow.zaColor = zaColor;
    writeShadow[reg_cnt] = zaColor;

    mask |= STATE_REG_MASK(zaColor);
    reg_cnt++;
  }

  if (NOTVALID(chromaKey)) {
#if GLIDE_DEBUG
    _grChromakeyValue(LOADARG(grChromakeyValue, color));
#endif /* GLIDE_DEBUG */

    gc->state.shadow.chromaKey = LOADARG(grChromakeyValue, color);
    _grSwizzleColor(&gc->state.shadow.chromaKey);

    writeShadow[reg_cnt] = gc->state.shadow.chromaKey;

    mask |= STATE_REG_MASK(chromaKey);
    reg_cnt++;
  }

  if (NOTVALID(chromaRange)) {
    FxU32
      chromaRange = gc->state.shadow.chromaRange;

    chromaRange &= SST_ENCHROMARANGE;

    _grSwizzleColor(&LOADARG(grChromaRange, range));
    chromaRange |= ((LOADARG(grChromaRange, range) & 0x00FFFFFFUL) |
                    (LOADARG(grChromaRange, match_mode) << 24UL));
#if GLIDE_DEBUG
    _grChromaRange(LOADARG(grChromaRange, range),
                   LOADARG(grChromaRange, match_mode));
#endif /* GLIDE_DEBUG */
    GR_ASSERT(chromaRange == gc->state.shadow.chromaRange);

    gc->state.shadow.chromaRange = chromaRange;
    writeShadow[reg_cnt] = chromaRange;

    mask |= STATE_REG_MASK(chromaRange);
    reg_cnt++;
  }

  if (reg_cnt) {
#if USE_PACKET_FIFO
    REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, reg_cnt, mask);
    {
      FxU32 i;

      for(i = 0; i < reg_cnt; i++) {
        REG_GROUP_INDEX_SET(writeShadow[i]);
      }
    }
    REG_GROUP_END();
#else /* !USE_PACKET_FIFO */
    GR_SET_EXPECTED_SIZE(reg_cnt, reg_cnt);
    {
      if (NOTVALID(fbzColorPath))
        REG_GROUP_SET(hw, fbzColorPath, gc->state.shadow.fbzColorPath);
      if (NOTVALID(fogMode))
        REG_GROUP_SET(hw, fogMode, gc->state.shadow.fogMode);
      if (NOTVALID(alphaMode))
        REG_GROUP_SET(hw, alphaMode, gc->state.shadow.alphaMode);
      if (NOTVALID(fbzMode))
        REG_GROUP_SET(hw, fbzMode, gc->state.shadow.fbzMode);
      
      if (NOTVALID(lfbMode))
        REG_GROUP_SET(hw, lfbMode, gc->state.shadow.lfbMode);
      if (NOTVALID(clipRegs)) {
        REG_GROUP_SET(hw, clipLeftRight, gc->state.shadow.clipLeftRight);
        REG_GROUP_SET(hw, clipBottomTop, gc->state.shadow.clipBottomTop);
      }
      
      if (NOTVALID(fogColor))
        REG_GROUP_SET(hw, fogColor, gc->state.shadow.fogColor);
      if (NOTVALID(zaColor))
        REG_GROUP_SET(hw, zaColor, gc->state.shadow.zaColor);

      if (NOTVALID(chromaKey)) 
        REG_GROUP_SET(hw, chromaKey, gc->state.shadow.chromaKey);
      if (NOTVALID(chromaRange)) 
        REG_GROUP_SET(hw, chromaRange, gc->state.shadow.chromaRange);
    }
    GR_CHECK_SIZE();
#endif /* !USE_PACKET_FIFO */
  }

  if (NOTVALID(c0c1)) {
    //    _grConstantColorValue(LOADARG(grConstantColorValue, color));
    _grSwizzleColor(&LOADARG(grConstantColorValue, color));
    
    gc->state.shadow.color0 = LOADARG(grConstantColorValue, color);
    gc->state.shadow.color1 = LOADARG(grConstantColorValue, color);
    
    REG_GROUP_BEGIN(BROADCAST_ID, c0, 2, 0x03);
    {
      REG_GROUP_SET(hw, c0, gc->state.shadow.color0);
      REG_GROUP_SET(hw, c1, gc->state.shadow.color1);
    }
    REG_GROUP_END();
  }

  if (gc->state.invalid)
    _grUpdateParamIndex();

  gc->state.invalid = 0;  

  /*
  ** NB:
  ** This code *always* clips on the host.  This is because I beleive
  ** we've reached the point that any performance-concious person
  ** using Banshee will have a machine that is fast enough such that
  ** host-based culling is faster.
  *
  * dpc: What if the app has turned off culling? Why bother checking
  *      and doing the branch away from the culling code if we don't
  *      need to bother? I've put the cull/nocull variants back.
  */
  gc->triSetupProc = CUR_TRI_PROC(FXFALSE, (gc->state.cull_mode != GR_CULL_DISABLE));

#undef FN_NAME
} /* _grValidateState */

#define IARRAY(p,i)    (*((FxU32 *)(p)+(i)))

/*-------------------------------------------------------------------
  Function: grEnable
  Date: 10-Oct-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grEnable, void , (GrEnableMode_t mode) )
{
#define FN_NAME "grEnable"
  GR_BEGIN_NOFIFOCHECK("grEnable\n", 85);

  switch (mode) {
  case GR_AA_ORDERED:
    gc->state.grEnableArgs.primitive_smooth_mode = 
      GR_AA_ORDERED_POINTS_MASK |
      GR_AA_ORDERED_LINES_MASK |
      GR_AA_ORDERED_TRIANGLES_MASK;
    break;
  case GR_AA_ORDERED_POINTS_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode |= GR_AA_ORDERED_POINTS_MASK;
    break;
  case GR_AA_ORDERED_LINES_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode |= GR_AA_ORDERED_LINES_MASK;
    break;
  case GR_AA_ORDERED_TRIANGLES_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode |= GR_AA_ORDERED_TRIANGLES_MASK;
    break;
  case GR_SHAMELESS_PLUG:
    ENABLEMODE(shameless_plug_mode);
    _GlideRoot.environment.shamelessPlug = GR_MODE_ENABLE;
    break;
  case GR_VIDEO_SMOOTHING:
    ENABLEMODE(video_smooth_mode);
    break;
  case GR_ALLOW_MIPMAP_DITHER:
    gc->state.allowLODdither = GR_MODE_ENABLE;
    break;
  case GR_PASSTHRU:
    break;
  }

#undef FN_NAME
} /* grEnable */

/*-------------------------------------------------------------------
  Function: grDisable
  Date: 10-Oct-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grDisable, void , (GrEnableMode_t mode) )
{
#define FN_NAME "grDisable"
  GR_BEGIN_NOFIFOCHECK("grDisable\n", 85);

  switch (mode) {
  case GR_AA_ORDERED:
    gc->state.grEnableArgs.primitive_smooth_mode = 0;
    break;
  case GR_AA_ORDERED_POINTS_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode &= ~GR_AA_ORDERED_POINTS_MASK;
    break;
  case GR_AA_ORDERED_LINES_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode &= ~GR_AA_ORDERED_LINES_MASK;
    break;
  case GR_AA_ORDERED_TRIANGLES_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode &= ~GR_AA_ORDERED_TRIANGLES_MASK;
    break;
  case GR_SHAMELESS_PLUG:
    DISABLEMODE(shameless_plug_mode);
    _GlideRoot.environment.shamelessPlug = GR_MODE_DISABLE;
    break;
  case GR_VIDEO_SMOOTHING:
    DISABLEMODE(video_smooth_mode);
    break;
  case GR_ALLOW_MIPMAP_DITHER:
    gc->state.allowLODdither = GR_MODE_DISABLE;
    break;
  case GR_PASSTHRU:
    break;
  }

#undef FN_NAME
} /* grDisable */

/*-------------------------------------------------------------------
  Function: grCoordinateSpace
  Date: 01-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grCoordinateSpace, void , (GrCoordinateSpaceMode_t mode) )
{
#define FN_NAME "grCoordinateSpace"
  GR_BEGIN_NOFIFOCHECK("grCoordinateSpace\n", 85);

  switch (mode) {
  case GR_WINDOW_COORDS:
    gc->state.grCoordinateSpaceArgs.coordinate_space_mode = GR_WINDOW_COORDS;
    break;
  case GR_CLIP_COORDS:
    gc->state.grCoordinateSpaceArgs.coordinate_space_mode = GR_CLIP_COORDS;
    break;
  }

  /* Select function vectors based on current coordinate system. These
   * can be further specialized for cull/no-cull.  
   */
  gc->archDispatchProcs.coorModeTriVector = (*_GlideRoot.deviceArchProcs.curTriProcs) + mode;
  gc->archDispatchProcs.drawVertexList    = _GlideRoot.deviceArchProcs.curVertexListProcs[mode];

  /* Update triangle proc based on the current culling mode */
  grCullMode(gc->state.cull_mode);
  
  /* cull mode is not validated so we need to update triproc here */
  gc->triSetupProc = CUR_TRI_PROC(FXFALSE, (gc->state.cull_mode != GR_CULL_DISABLE));

#undef FN_NAME
} /* grCoordinateSpace */

/*-------------------------------------------------------------------
  Function: grDepthRange
  Date: 01-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grDepthRange, void , (FxFloat n, FxFloat f) )
{
#define FN_NAME "grDepthRange"
  GR_BEGIN_NOFIFOCHECK("grDepthRange\n", 85);

  gc->state.Viewport.n = n;
  gc->state.Viewport.f  = f;
  gc->state.Viewport.hdepth = (f - n) * 0.5f * 65535.f;
  gc->state.Viewport.oz = (f + n) * 0.5f * 65535.f;

#undef FN_NAME
} /* grDepthRange */

/*-------------------------------------------------------------------
  Function: grViewport
  Date: 01-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grViewport, void , (FxI32 x, FxI32 y, FxI32 width, FxI32 height) )
{
#define FN_NAME "grViewport"
  GR_BEGIN_NOFIFOCHECK("grViewport\n", 85);

    gc->state.Viewport.ox      = (FxFloat)(x + width * 0.5f);
    gc->state.Viewport.oy      = (FxFloat)(y + height *0.5f);
    gc->state.Viewport.hwidth  = width * 0.5f;
    gc->state.Viewport.hheight = height * 0.5f;

#undef FN_NAME
} /* grViewport */

#if DRI_BUILD
void
_grInvalidateAll()
{
  GR_DCL_GC;

  gc->state.invalid=~0;
}
#endif


#endif /* GLIDE3 */
