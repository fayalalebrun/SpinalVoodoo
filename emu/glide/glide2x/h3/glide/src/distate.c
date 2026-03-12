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
** 3     3/17/99 6:16p Dow
** Phantom Menace and other fixes.
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
gc->state.stateArgs.##function##Args.arg = arg

#define LOADARG(function, arg) \
gc->state.stateArgs.##function##Args.arg

#define INVALIDATE(regset) \
gc->state.invalid |= ##regset##BIT

#define NOTVALID(regset) \
(gc->state.invalid & ##regset##BIT)

#define SETVALID(regset) \
(gc->state.invalid &= ~(##regset##BIT))

#define ENABLEMODE(mode) \
gc->state.grEnableArgs.##mode## = GR_MODE_ENABLE;

#define DISABLEMODE(mode) \
gc->state.grEnableArgs.##mode## = GR_MODE_DISABLE;

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 87);

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 87);

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 87);

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

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
  GR_BEGIN_NOFIFOCHECK("grAlphaControlsITRGBLighting",85);

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
  GR_BEGIN_NOFIFOCHECK("grColorCombine",85);

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
  GR_BEGIN_NOFIFOCHECK("grChromakeyMode",85);

  INVALIDATE(fbzMode);

  STOREARG(grChromakeyMode, mode);

#undef FN_NAME
} /* grChromakeyMode */

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
/*-------------------------------------------------------------------
  Function: grChromaModeExt
  Date: 05-Jan-98
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void _grChromaModeExt(GrChromakeyMode_t mode)
{
#define FN_NAME "_grChromaModeExt"
  GR_BEGIN_NOFIFOCHECK("_grChromaModeExt",85);

  INVALIDATE(fbzMode);

  STOREARG(grChromakeyMode, mode);

#undef FN_NAME
} /* _grChromaModeExt */

/*-------------------------------------------------------------------
  Function: _grChromaRangeExt
  Date: 15-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void _grChromaRangeExt(GrColor_t color, GrColor_t range, GrChromaRangeMode_t mode)
{
#define FN_NAME "_grChromaRangeExt"
  GR_BEGIN_NOFIFOCHECK("_grChromaRangeExt",85);

  GR_CHECK_F(myName,
             (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Voodoo2),
             "grChromaRange not supported.");

  INVALIDATE(chromaKey);

  STOREARG(grChromakeyValue, color);
  STOREARG(grChromaRange, range);
  STOREARG(grChromaRange, mode);

#undef FN_NAME
} /* _grChromaRangeExt */
#endif

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
  GR_BEGIN_NOFIFOCHECK("grChromakeyMode",85);

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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

  INVALIDATE(fbzMode);

  STOREARG(grDepthBufferMode, mode);

 #undef FN_NAME
} /* grDepthBufferMode */


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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);

  INVALIDATE(chromaKey);

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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);

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
GR_DIENTRY(grDepthBiasLevel, void , (FxI16 level) )
{
#define FN_NAME "grDepthBiasLevel"

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);

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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);

  INVALIDATE(fogMode);

  STOREARG(grFogMode, mode);

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);

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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 82);

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 82);

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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

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

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

  if (NOTVALID(alphaMode)) {
    _grAlphaBlendFunction(LOADARG(grAlphaBlendFunction, rgb_sf),
                          LOADARG(grAlphaBlendFunction, rgb_df),
                          LOADARG(grAlphaBlendFunction, alpha_sf),
                          LOADARG(grAlphaBlendFunction, alpha_df));
    _grAlphaTestFunction(LOADARG(grAlphaTestFunction, fnc));
    _grAlphaTestReferenceValue(LOADARG(grAlphaTestReferenceValue, value));

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

  }

  if (NOTVALID(fbzMode)) {
    _grChromakeyMode(LOADARG(grChromakeyMode, mode));
    _grDepthMask(LOADARG(grDepthMask, enable));
    _grDepthBufferFunction(LOADARG(grDepthBufferFunction, fnc));
    _grDepthBufferMode(LOADARG(grDepthBufferMode, mode));
    _grDitherMode(LOADARG(grDitherMode, mode));
    _grRenderBuffer(LOADARG(grRenderBuffer, buffer));
    _grColorMask(LOADARG(grColorMask, rgb), LOADARG(grColorMask, alpha));
    _grSstOrigin(LOADARG(grSstOrigin, origin));

  }

  if (NOTVALID(chromaKey)) {
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
    _grChromaRangeExt(LOADARG(grChromakeyValue, color),LOADARG(grChromaRange, range)
                      , LOADARG(grChromaRange, mode));

#else
    _grChromakeyValue(LOADARG(grChromakeyValue, color));
#endif
  }

  if (NOTVALID(clipRegs)) {
    _grClipWindow(
                  LOADARG(grClipWindow, minx),
                  LOADARG(grClipWindow, miny),
                  LOADARG(grClipWindow, maxx),
                  LOADARG(grClipWindow, maxy));
  }

  if (NOTVALID(zaColor)) {
    _grDepthBiasLevel((FxI16) LOADARG(grDepthBiasLevel, level));
  }

  if (NOTVALID(fogMode)) {
    _grFogMode(LOADARG(grFogMode, mode));
  }

  if (NOTVALID(fogColor)) {
    _grFogColorValue(LOADARG(grFogColorValue, color));
  }

  if (NOTVALID(lfbMode)) {
    _grLfbWriteColorFormat(LOADARG(grLfbWriteColorFormat, colorFormat));
    _grLfbWriteColorSwizzle(LOADARG(grLfbWriteColorSwizzle, swizzleBytes),
                            LOADARG(grLfbWriteColorSwizzle, swapWords));

  }

  if (NOTVALID(c0c1)) {
    _grConstantColorValue(LOADARG(grConstantColorValue, color));
  }

  _grUpdateParamIndex();

  _grFlushCommonStateRegs();

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

  switch (mode) {
  case GR_AA_ORDERED:
    ENABLEMODE(primitive_smooth_mode);
    break;
  case GR_SHAMELESS_PLUG:
    ENABLEMODE(shameless_plug_mode);
    _GlideRoot.environment.shamelessPlug = mode;
    break;
  case GR_VIDEO_SMOOTHING:
    ENABLEMODE(video_smooth_mode);
    break;
  case GR_ALLOW_MIPMAP_DITHER:
    gc->state.allowLODdither = GR_MODE_ENABLE;
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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

  switch (mode) {
  case GR_AA_ORDERED:
    DISABLEMODE(primitive_smooth_mode);
    break;
  case GR_SHAMELESS_PLUG:
    DISABLEMODE(shameless_plug_mode);
    break;
  case GR_VIDEO_SMOOTHING:
    DISABLEMODE(video_smooth_mode);
    break;
  case GR_ALLOW_MIPMAP_DITHER:
    gc->state.allowLODdither = GR_MODE_DISABLE;
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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

  switch (mode) {
  case GR_WINDOW_COORDS:
    gc->state.grCoordinateSpaceArgs.coordinate_space_mode = GR_WINDOW_COORDS;
    break;
  case GR_CLIP_COORDS:
    gc->state.grCoordinateSpaceArgs.coordinate_space_mode = GR_CLIP_COORDS;
    break;
  }

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

    gc->state.Viewport.ox      = (FxFloat)(x + width * 0.5f);
    gc->state.Viewport.oy      = (FxFloat)(y + height *0.5f);
    gc->state.Viewport.hwidth  = width * 0.5f;
    gc->state.Viewport.hheight = height * 0.5f;

#undef FN_NAME
} /* grViewport */

#endif /* GLIDE3 */
