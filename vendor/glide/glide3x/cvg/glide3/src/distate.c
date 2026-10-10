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
 ** Revision 1.1.1.1.8.4  2007/09/29 13:59:34  koolsmoky
 ** completed grStippleMode and grStipplePattern
 **
 ** Revision 1.1.1.1.8.3  2004/03/08 07:42:21  dborca
 ** Voodoo Rush fixes
 **
 ** Revision 1.1.1.1.8.2  2003/11/03 13:34:29  dborca
 ** Voodoo2 happiness (DJGPP & Linux)
 **
 ** Revision 1.1.1.1.8.1  2003/06/29 18:45:55  guillemj
 ** Fixed preprocessor invalid token errors.
 **
 ** Revision 1.1.1.1  1999/12/07 21:42:31  joseph
 ** Initial checkin into SourceForge.
 **
** 
** 1     10/08/98 11:30a Brent
** 
** 22    10/07/98 9:43p Peter
** triangle procs for 3DNow!(tm)
** 
** 21    10/06/98 8:23p Peter
** 3DNow!(tm) texture downloads
** 
** 20    10/06/98 7:18p Atai
** added triangle and drawlist asm routine for clip coords
** 
** 19    9/08/98 7:13p Atai
** same
** 
** 18    9/08/98 6:56p Atai
** fix debug info. added underline for internal routines and some \n
** 
** 17    7/01/98 11:44a Atai
** cast _grDepthBiasLevel call
** 
** 16    7/01/98 11:31a Atai
** make grDepthBiasLevel argument FxI32
** 
** 15    6/24/98 1:47p Atai
** code clean up; rename texute line routine
** 
** 14    6/21/98 11:58a Atai
** fixed fogcoord paramindex
** 
** 13    6/17/98 10:39a Atai
** fix grDepthBiasLevel
** 
** 12    6/11/98 5:12p Atai
** added aa case for OGL
** 
** 11    6/09/98 5:33p Atai
** replace grSstControl with grEnable/grDisable
** 
** 10    6/03/98 2:34p Atai
** fix chromarange
** 
** 9     5/29/98 6:39p Atai
** fix chromarange
** 
** 8     5/28/98 3:27p Atai
** check the invalid flag in grBufferClear
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

#define INVALIDATE(regset) \
gc->state.invalid |= regset ## BIT

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
  Function: grChromaModeExt
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
GR_DIENTRY(grStipplePattern, void , (GrStipplePattern_t stipple))
{
 #define FN_NAME "grStipplePattern"

  GR_BEGIN_NOFIFOCHECK("grStipplePattern\n", 85);

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
  STOREARG(grColorMask, alpha);


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
void
_grValidateState()
{
#define FN_NAME "_grValidateState"
  FxU32 mask = 0;
  FxU32 reg_cnt = 0;
  GR_BEGIN_NOFIFOCHECK("_grValidateState\n", 85);

  if (NOTVALID(alphaMode)) {
    _grAlphaBlendFunction(LOADARG(grAlphaBlendFunction, rgb_sf),
                          LOADARG(grAlphaBlendFunction, rgb_df),
                          LOADARG(grAlphaBlendFunction, alpha_sf),
                          LOADARG(grAlphaBlendFunction, alpha_df));
    _grAlphaTestFunction(LOADARG(grAlphaTestFunction, fnc));
    _grAlphaTestReferenceValue(LOADARG(grAlphaTestReferenceValue, value));

    mask |= 0x0004;
    reg_cnt++;
  }

  if (NOTVALID(fbzColorPath)) {
    _grAlphaCombine(LOADARG(grAlphaCombine, function),
                    LOADARG(grAlphaCombine, factor),
                    LOADARG(grAlphaCombine, local),
                    LOADARG(grAlphaCombine, other),
                    LOADARG(grAlphaCombine, invert));
    _grAlphaControlsITRGBLighting(LOADARG(grAlphaControlsITRGBLighting,
                                          enable)); 
    _grColorCombine(LOADARG(grColorCombine, function), 
                    LOADARG(grColorCombine, factor),
                    LOADARG(grColorCombine, local),
                    LOADARG(grColorCombine, other),
                    LOADARG(grColorCombine, invert));

    mask |= 0x0001;
    reg_cnt++;
  }

  if (NOTVALID(fbzMode)) {
    _grChromakeyMode(LOADARG(grChromakeyMode, mode));
    _grChromaMode(LOADARG(grChromaRange, mode));
    _grDepthMask(LOADARG(grDepthMask, enable));
    _grDepthBufferFunction(LOADARG(grDepthBufferFunction, fnc));
    _grDepthBufferMode(LOADARG(grDepthBufferMode, mode));
    _grDitherMode(LOADARG(grDitherMode, mode));
    _grStippleMode(LOADARG(grStippleMode, mode));
    _grRenderBuffer(LOADARG(grRenderBuffer, buffer));
    _grColorMask(LOADARG(grColorMask, rgb), LOADARG(grColorMask, alpha));
    _grSstOrigin(LOADARG(grSstOrigin, origin));

    mask |= 0x0008;
    reg_cnt++;
  }

  if (NOTVALID(stipple)) {
    gc->state.fbi_config.stipple = LOADARG(grStipplePattern, stipple);
    REG_GROUP_BEGIN(BROADCAST_ID, stipple, 1, 0x01);
    {
      REG_GROUP_SET(hw, stipple, gc->state.fbi_config.stipple);
    }
    REG_GROUP_END();
  }

  if (NOTVALID(chromaKey)) {
    _grChromakeyValue(LOADARG(grChromakeyValue, color));
    mask |= 0x1000;
    reg_cnt+=1;
  }

  if (NOTVALID(chromaRange)) {
    _grChromaRange(LOADARG(grChromaRange, range), LOADARG(grChromaRange, match_mode));
    mask |= 0x2000;
    reg_cnt+=1;
  }

  if (NOTVALID(clipRegs)) {
    _grClipWindow(
                  LOADARG(grClipWindow, minx),
                  LOADARG(grClipWindow, miny),
                  LOADARG(grClipWindow, maxx),
                  LOADARG(grClipWindow, maxy));
    mask |= 0x0060;
    reg_cnt+=2;
  }

  if (NOTVALID(zaColor)) {
    _grDepthBiasLevel(LOADARG(grDepthBiasLevel, level));
    mask |= 0x0800;
    reg_cnt++;
  }

  if (NOTVALID(fogMode)) {
    _grFogMode(LOADARG(grFogMode, mode));
    mask |= 0x0002;
    reg_cnt++;
  }

  if (NOTVALID(fogColor)) {
    _grFogColorValue(LOADARG(grFogColorValue, color));
    mask |= 0x0400;
    reg_cnt++;
  }

  if (NOTVALID(lfbMode)) {
    _grLfbWriteColorFormat(LOADARG(grLfbWriteColorFormat, colorFormat));
    _grLfbWriteColorSwizzle(LOADARG(grLfbWriteColorSwizzle, swizzleBytes),
                            LOADARG(grLfbWriteColorSwizzle, swapWords));
    mask |= 0x0010;
    reg_cnt++;
  }

  if (reg_cnt) {

    GR_SET_EXPECTED_SIZE(sizeof(FxU32)*reg_cnt, 1);

    REG_GROUP_NO_CHECK_BEGIN(BROADCAST_ID, fbzColorPath, reg_cnt, mask);
    {
      if (NOTVALID(fbzColorPath))
        REG_GROUP_SET(hw, fbzColorPath, gc->state.fbi_config.fbzColorPath);
      if (NOTVALID(fogMode))
        REG_GROUP_SET(hw, fogMode, gc->state.fbi_config.fogMode);
      if (NOTVALID(alphaMode))
        REG_GROUP_SET(hw, alphaMode, gc->state.fbi_config.alphaMode);
      if (NOTVALID(fbzMode))
        REG_GROUP_SET(hw, fbzMode, gc->state.fbi_config.fbzMode);
      
      if (NOTVALID(lfbMode))
        REG_GROUP_SET(hw, lfbMode, gc->state.fbi_config.lfbMode);
      if (NOTVALID(clipRegs)) {
        REG_GROUP_SET(hw, clipLeftRight, gc->state.fbi_config.clipLeftRight);
        REG_GROUP_SET(hw, clipBottomTop, gc->state.fbi_config.clipBottomTop);
      }
      
      if (NOTVALID(fogColor))
        REG_GROUP_SET(hw, fogColor, gc->state.fbi_config.fogColor);
      if (NOTVALID(zaColor))
        REG_GROUP_SET(hw, zaColor, gc->state.fbi_config.zaColor);
      if (NOTVALID(chromaKey)) {
        REG_GROUP_SET(hw, chromaKey, gc->state.fbi_config.chromaKey);
      }
      if (NOTVALID(chromaRange)) {
        REG_GROUP_SET(hw, chromaRange, gc->state.fbi_config.chromaRange);
      }
    }
    REG_GROUP_NO_CHECK_END();
    GR_END();
  }

  if (NOTVALID(c0c1)) {

    //    _grConstantColorValue(LOADARG(grConstantColorValue, color));
    _grSwizzleColor(&gc->state.stateArgs.grConstantColorValueArgs.color);
    gc->state.fbi_config.color0 = gc->state.stateArgs.grConstantColorValueArgs.color;
    gc->state.fbi_config.color1 = gc->state.stateArgs.grConstantColorValueArgs.color;
    
    GR_SET_EXPECTED_SIZE(sizeof(FxU32)*2, 1);
    REG_GROUP_NO_CHECK_BEGIN(BROADCAST_ID, c0, 2, 0x03);
    {
      REG_GROUP_SET(hw, c0, gc->state.fbi_config.color0);
      REG_GROUP_SET(hw, c1, gc->state.fbi_config.color1);
    }
    REG_GROUP_NO_CHECK_END();
    GR_END();
  }

  _grUpdateParamIndex();

  gc->state.invalid = 0;  

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
    _grSstControl(GR_CONTROL_ACTIVATE);
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
    _grSstControl(GR_CONTROL_DEACTIVATE);
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

  gc->curArchProcs.coorTriSetupVector = (*_GlideRoot.deviceArchProcs.curTriProcs) + mode;
  gc->curArchProcs.drawVertexList     = _GlideRoot.deviceArchProcs.curVertexListProcs[mode];

  /* Update triangle proc based on the current culling mode */
  grCullMode(gc->state.cull_mode);

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

#endif /* GLIDE3 */
