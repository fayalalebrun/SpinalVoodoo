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
 **
 ** $Log:
 **  33   3dfx      1.25.1.2.1.311/14/00 Jonny Cochrane  Implement multisample LOD
 **       Dithering for 2x and 4x FSAA modes 
 **  32   3dfx      1.25.1.2.1.210/11/00 Brent           Forced check in to enforce
 **       branching.
 **  31   3dfx      1.25.1.2.1.109/26/00 Andy Hanson     Add environment variable of
 **       glide group so they can do wacky things safely.
 **  30   3dfx      1.25.1.2.1.009/19/00 Matt McClure    Added a call to win_mode.c
 **       to enable OpenGL.
 **  29   3dfx      1.25.1.2    06/20/00 Joseph Kain     Fixed errors introduced by
 **       my previous merge.
 **  28   3dfx      1.25.1.1    06/20/00 Joseph Kain     Changes to support the
 **       Napalm Glide open source release.  Changes include cleaned up offensive
 **       comments and new legal headers.
 **  27   3dfx      1.25.1.0    06/15/00 Bill White      Merged changes to support
 **       Linux.
 ** 
 **  26   3dfx      1.25        04/25/00 Kenneth Dyke    Fixed non-compressed ->
 **       compressed texture chip bug workaround code.
 **  25   3dfx      1.24        04/24/00 Kenneth Dyke    Make sure we don't modify
 **       the stencil buffer when stencil mode is disabled.
 **  24   3dfx      1.23        04/13/00 Kenneth Dyke    Added support for new style
 **       2-sample AA mode.
 **  23   3dfx      1.22        03/30/00 Kenneth Dyke    Added private grEnable()
 **       for OpenGL.
 **  22   3dfx      1.21        03/23/00 Kenneth Dyke    Keep state maintained
 **       across all chips.
 **  21   3dfx      1.20        03/19/00 Kenneth Dyke    Keep track of which TMU
 **       'owns' the real TMU register state when in 2ppc mode.
 **  20   3dfx      1.19        03/07/00 Kenneth Dyke    Workaround for weird
 **       compressed texture quirk.
 **  19   3dfx      1.18        02/28/00 Kenneth Dyke    Fixed dither rotation stuff
 **       for 4-sample AA.  We also no longer clobber the fog mode setting.
 ** 
 **  18   3dfx      1.17        02/22/00 Kenneth Dyke    Disable 2PPC modes when
 **       palettized textures are in use until I find a way to make it fast.
 **  17   3dfx      1.16        02/02/00 Kenneth Dyke    Fixed per-TMU constant
 **       color values to not interfere with texture chroma keying.
 **  16   3dfx      1.15        02/02/00 Kenneth Dyke    Fixed triangle parameter
 **       problems related to 2PPC mode.
 **  15   3dfx      1.14        02/02/00 Adam Briggs     they say that if two people
 **       with braces kiss their braces can get stuck together
 **  14   3dfx      1.13        02/01/00 Kenneth Dyke    Added code to detect when
 **       TMU0 is in passthrough mode and to enabled 2PPC in that case. (Remap TMU1
 **       to TMU0).
 **  13   3dfx      1.12        01/31/00 Adam Briggs     changed the IS_NAPALM macro
 **       to cooperate with the display driver version of the same
 **  12   3dfx      1.11        01/31/00 Adam Briggs     Changed all device ID magic
 **       numbers to use those defined in fxhal.h & added IS_NAPALM macro to test
 **       against device ID range
 **  11   3dfx      1.10        01/28/00 Kenneth Dyke    Totoally revamped TMU
 **       register update mechanism to make 2PPC modes work right regardless of the
 **       order of Glide calls.   Also fixed a few register config bugs found when
 **       switching between new and old style combine modes.
 **  10   3dfx      1.9         01/23/00 Adam Briggs     set & recognize the SSTTYPE
 **       var for Voodoo4
 **  9    3dfx      1.8         01/18/00 Kenneth Dyke    Pull AA jitter values from
 **       Glide environment.
 **  8    3dfx      1.7         01/16/00 Kenneth Dyke    Changes to enforce proper
 **       combine extension usage.
 **  7    3dfx      1.6         10/25/99 Anthony tai     mask off alpha stuff for
 **       combineMode
 **  6    3dfx      1.5         10/25/99 Anthony tai     only clean up the
 **       un-necessary
 **  5    3dfx      1.4         10/22/99 Anthony tai     clean up combineMode
 **       register if ext is not used
 **  4    3dfx      1.3         10/21/99 Anthony tai     fixed iterated rgb checking
 **  3    3dfx      1.2         10/15/99 Anthony tai     move 2ppc setting to state
 **       validation
 **  2    3dfx      1.1         09/17/99 Adam Briggs     Supported TEXTUREBUFFEREXT
 **       for Napalm 32bpp and AA modes.
 **  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
 ** $
** 
** 57    8/16/99 11:18a Adamb
** Merged in V3_OEM_100 fixes
** 
** 56    7/27/99 2:23p Larryw
** Allow UMA to do its thing.
** 
** 55    7/22/99 1:18p Atai
** added grTBufferMaskExt
** 
** 54    7/21/99 4:06p Atai
** direct write hack and GR_AA_MULTI_SAMPLE
** 
** 53    7/18/99 1:59p Atai
** added grAlphaBlendFunctionExt
** 
** 52    7/14/99 5:07p Atai
** fixed stencil interface and some cc/ac stuff
** 
** 51    7/14/99 9:39a Atai
** direct register write for glide3x
** test04 can do 4 sample aa (2 chips)
** 
** 50    7/12/99 4:01p Atai
** fixed warnings
** 
** 49    7/08/99 8:48p Atai
** stencil interface update
** 
** 48    7/06/99 2:45p Atai
** added gbc and 32 bpp color mask
** 
** 47    6/29/99 7:19p Atai
** remove argument for enabling SST_CM_USE_COMBINE_MODE
** 
** 45    6/27/99 12:44p Atai
** fixed CC and TCC
** 
** 44    6/24/99 7:12p Atai
** added STENCIL and COMBINE extension
** 
** 43    6/13/99 4:55p Atai
** fixed register mask
** 
** 41    6/10/99 5:12p Atai
** fist pass CC and AC ext
** 
** 40    6/09/99 2:35p Atai
** added combine mode bit and args defines
** 
** 39    6/04/99 3:18p Atai
** stencilMode and stencilOp
** 
** 38    6/04/99 11:00a Atai
** added stencil functions
** 
** 37    5/28/99 10:19a Atai
** the triproc setup should use the invalid flavor
** 
** 36    5/26/99 4:43p Kcd
** byte and word swap bits were reversed.
** 
** 35    5/19/99 3:55p Denis
** 
** 34    5/19/99 12:45p Denis
** First check in of the TEXTUREBUFFER extension.
** Contains both the texture color buffer and texture aux. buffer
** extensions
** that allows to specify a piece of texture memory as a rendering target
** and/or a piece of texture memory as the aux. buffer.
** 
** Probably a non conventional check in, in the sense that the API
** isn't entirely frozen / specified yet. To ease whoever's job it will be
** to complete the extension, I've added a "tbext" comment
** everywhere I made a modification. These should go away
** once the API is frozen.
**  
** 
** 33    4/10/99 1:23p Atai
** fixed code to compile in packet fifo mode 
** 
** 32    4/07/99 7:18p Atai
** added uma extension
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

#define STOREARG_TMU(function, tmu, arg) \
gc->state.stateArgs.function ## Args.arg[tmu] = arg

#define LOADARG_TMU(function, tmu, arg) \
gc->state.stateArgs.function ## Args.arg[tmu]

#define STOREARG(function, arg) \
gc->state.stateArgs.function ## Args.arg = arg

#define LOADARG(function, arg) \
gc->state.stateArgs.function ## Args.arg

#define NOTVALID(regset) \
(gc->state.invalid & regset ## BIT)

#define NOTVALID_TMU(tmu, regset) \
(gc->state.tmuInvalid[tmu] & regset ## BIT)

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
  INVALIDATE(alphaMode)

  STOREARG(grAlphaBlendFunction, rgb_sf);
  STOREARG(grAlphaBlendFunction, rgb_df);
  STOREARG(grAlphaBlendFunction, alpha_sf);
  STOREARG(grAlphaBlendFunction, alpha_df);

  
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    GrAlphaBlendOp_t rgb_op = GR_BLEND_OP_ADD;
    GrAlphaBlendOp_t alpha_op = GR_BLEND_OP_ADD;
    INVALIDATE(fogMode);
    STOREARG(grAlphaBlendFunction, rgb_op);
    STOREARG(grAlphaBlendFunction, alpha_op);
  }

 #undef FN_NAME
} /* grAlphaBlendFunction */

/*-------------------------------------------------------------------
  Function: grAlphaBlendFunctionExt
  Date: 18-July-99
  Implementor(s): atai
  Description:
  Inform Glide that the Alpha Blend Function has been modified.
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grAlphaBlendFunctionExt, void , 
             (GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df, GrAlphaBlendOp_t rgb_op,
              GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df, GrAlphaBlendOp_t alpha_op) )
{
#define FN_NAME "grAlphaBlendFunctionExt"
  GR_BEGIN_NOFIFOCHECK("grAlphaBlendFunctionExt\n", 87);
  
  /* Invalidate AlphaMode */
  INVALIDATE(alphaMode);
  
  STOREARG(grAlphaBlendFunction, rgb_sf);
  STOREARG(grAlphaBlendFunction, rgb_df);
  STOREARG(grAlphaBlendFunction, alpha_sf);
  STOREARG(grAlphaBlendFunction, alpha_df);
  
  INVALIDATE(fogMode);
  STOREARG(grAlphaBlendFunction, rgb_op);
  STOREARG(grAlphaBlendFunction, alpha_op);
  
#undef FN_NAME
} /* grAlphaBlendFunctionExt */

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
  INVALIDATE(tmuConfig);

#if DEBUG_2PPC
  GDBG_PRINTF("grAlphaCombine(func=%x, factor=%x, local=%x, other=%x, invert=%d)\n",
    function,factor,local,other,invert);
#endif
  
  gc->state.combineExtsInUse &= ~STATE_USING_CA;
  
  grDisable(GR_COMBINEEXT_MODE);

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
  INVALIDATE(tmuConfig);
  
#if DEBUG_2PPC  
  GDBG_PRINTF("grColorCombine(func=%x, factor=%x, local=%x, other=%x, invert=%d)\n",
    function,factor,local,other,invert);
#endif
  
  gc->state.combineExtsInUse &= ~STATE_USING_CC;
  
  grDisable(GR_COMBINEEXT_MODE);

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
  Function: grStencilFunc
  Date: 03-June-99
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grStencilFunc, void , (GrCmpFnc_t fnc, GrStencil_t ref, GrStencil_t mask) )
{
 #define FN_NAME "grStencilFunc"

  GR_BEGIN_NOFIFOCHECK("grStencilFunc\n", 85);

  INVALIDATE(stencilMode);

  STOREARG(grStencilFunc, fnc);
  STOREARG(grStencilFunc, ref);
  STOREARG(grStencilFunc, mask);

 #undef FN_NAME
} /* grStencilFunc */

/*-------------------------------------------------------------------
  Function: grStencilMask
  Date: 03-June-99
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grStencilMask, void , (GrStencil_t value) )
{
 #define FN_NAME "grStencilMask"

  GR_BEGIN_NOFIFOCHECK("grStencilMask\n", 85);

  INVALIDATE(stencilMode);

  STOREARG(grStencilMask, value);

 #undef FN_NAME
} /* grStencilMask */

/*-------------------------------------------------------------------
  Function: grLfbConstantStencil
  Date: 03-June-99
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grLfbConstantStencil, void , (GrStencil_t value) )
{
 #define FN_NAME "grLfbConstantStencil"

  GR_BEGIN_NOFIFOCHECK("grLfbConstantStencil\n", 85);

  INVALIDATE(stencilMode);

  STOREARG(grLfbConstantStencil, value);

 #undef FN_NAME
} /* grLfbConstantStencil */

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

  GR_BEGIN_NOFIFOCHECK("grStipplePattern\n", 85);

  INVALIDATE(stipple);

  STOREARG(grStipplePattern, stipple);

 #undef FN_NAME
} /* grStipplePattern */


/*-------------------------------------------------------------------
  Function: grStencilOp
  Date: 03-June-99
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grStencilOp, void , (GrStencilOp_t stencil_fail,
                                GrStencilOp_t depth_fail,
                                GrStencilOp_t depth_pass) )
{
 #define FN_NAME "grStencilWriteMask"

  GR_BEGIN_NOFIFOCHECK("grStencilMode\n", 85);

  INVALIDATE(stencilOp);

  STOREARG(grStencilOp, stencil_fail);
  STOREARG(grStencilOp, depth_fail);
  STOREARG(grStencilOp, depth_pass);

 #undef FN_NAME
} /* grStencilOp */

#if FX_GLIDE_NAPALM
/*-------------------------------------------------------------------
  Function: grColorCombineExt
  Date: 08-June-99
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grColorCombineExt, void , (GrCCUColor_t     a,
                                        GrCombineMode_t  a_mode,
                                        GrCCUColor_t     b,
                                        GrCombineMode_t  b_mode,
                                        GrCCUColor_t     c,
                                        FxBool           c_invert,
                                        GrCCUColor_t     d,
                                        FxBool           d_invert,
                                        FxU32            shift,
                                        FxBool           invert) )
{
#define FN_NAME "grColorCombineExt"
  GR_BEGIN_NOFIFOCHECK("grColorCombineExt\n",85);

#if DEBUG_2PPC
  GDBG_PRINTF("grColorCombineExt(a=%x,am=%x,b=%x,bm=%x,c=%x,ci=%d,d=%x,di=%x,s=%d,i=%d)\n",
    a,a_mode,b,b_mode,c,c_invert,d,d_invert,shift,invert);
#endif
    
  /* update 
  ** cc_mselect (part of c), cc_add_clocal (part of d), cc_add_alocal (part of d)
  ** cc_invert_output (invert) 
  */
  INVALIDATE(fbzColorPath);
  /* update
  ** cc_otherselect (a), cc_localselect (b), cc_mselect_7 (part of c),
  ** cc_invert_other (a_mode), cc_invert_local (b_mode), cc_outshift (shift)
  ** bit(31)
  */
  INVALIDATE(combineMode);
  INVALIDATE(tmuConfig);

  gc->state.combineExtsInUse |= STATE_USING_CC;

  grEnable(GR_COMBINEEXT_MODE);

  STOREARG(grColorCombineExt, a);
  STOREARG(grColorCombineExt, a_mode);
  STOREARG(grColorCombineExt, b);
  STOREARG(grColorCombineExt, b_mode);
  STOREARG(grColorCombineExt, c);
  STOREARG(grColorCombineExt, c_invert);
  STOREARG(grColorCombineExt, d);
  STOREARG(grColorCombineExt, d_invert);
  STOREARG(grColorCombineExt, shift);
  STOREARG(grColorCombineExt, invert);

#undef FN_NAME
} /* grColorCombineExt */

/*-------------------------------------------------------------------
  Function: grAlphaCombineExt
  Date: 08-June-99
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grAlphaCombineExt, void , (GrACUColor_t     a,
                                        GrCombineMode_t  a_mode,
                                        GrACUColor_t     b,
                                        GrCombineMode_t  b_mode,
                                        GrACUColor_t     c,
                                        FxBool           c_invert,
                                        GrACUColor_t     d,
                                        FxBool           d_invert,
                                        FxU32            shift,
                                        FxBool           invert) )
{
#define FN_NAME "grAlphaCombineExt"
  GR_BEGIN_NOFIFOCHECK("grAlphaCombineExt\n",85);

#if DEBUG_2PPC
  GDBG_PRINTF("grAlphaCombineExt(a=%x,am=%x,b=%x,bm=%x,c=%x,ci=%d,d=%x,di=%x,s=%d,i=%d)\n",
    a,a_mode,b,b_mode,c,c_invert,d,d_invert,shift,invert);
#endif

  /* update
  ** cca_mselect (c), cca_add_clocal (part of d), cca_add_alocal (part of d)
  ** cca_invert_output (invert)
  */
  INVALIDATE(fbzColorPath);
  /* update
  ** cca_otherselect (a), cca_localselect (b), cca_invert_other (a_mode),
  ** cca_invert_local (b_mode), cca_outshift (shift)
  ** bit (31)
  */
  INVALIDATE(combineMode);
  INVALIDATE(tmuConfig);
  
  gc->state.combineExtsInUse |= STATE_USING_CA;

  grEnable(GR_COMBINEEXT_MODE);

  STOREARG(grAlphaCombineExt, a);
  STOREARG(grAlphaCombineExt, a_mode);
  STOREARG(grAlphaCombineExt, b);
  STOREARG(grAlphaCombineExt, b_mode);
  STOREARG(grAlphaCombineExt, c);
  STOREARG(grAlphaCombineExt, c_invert);
  STOREARG(grAlphaCombineExt, d);
  STOREARG(grAlphaCombineExt, d_invert);
  STOREARG(grAlphaCombineExt, shift);
  STOREARG(grAlphaCombineExt, invert);

#undef FN_NAME
} /* grAlphaCombineExt */

#endif

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


/* TextureBuffer extension utility.
** Copied relevant part from _grValidateState. Kind of bad, but I figured
** it was better than calling _grValidateState itself.
** tbext
*/
void _grValidateClipState( FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy )
{
#define FN_NAME "_grValidateClipState"
#define STATE_REG_BASE             (offsetof(SstRegs, fbzColorPath) >> 2UL)
#define STATE_REG_END              (offsetof(SstRegs, chromaRange) >> 2UL)
#define STATE_REG_MASK(__regName)  (0x01UL << ((offsetof(SstRegs, __regName) >> 2UL) - STATE_REG_BASE))
        FxU32 
          mask = 0,
          writeShadow[STATE_REG_END - STATE_REG_BASE + 1];

        GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

        _grClipWindow( minx, miny, maxx, maxy );
        writeShadow[0] = gc->state.shadow.clipLeftRight;
        writeShadow[1] = gc->state.shadow.clipBottomTop;

        mask |= (STATE_REG_MASK(clipLeftRight) | STATE_REG_MASK(clipBottomTop));
        #if USE_PACKET_FIFO
            REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, 2, mask);
            REG_GROUP_INDEX_SET(writeShadow[0]);
            REG_GROUP_INDEX_SET(writeShadow[1]);
            REG_GROUP_END();
        #else /* !USE_PACKET_FIFO */
            /* I'm not too sure about this one. */
            /* GR_SET_EXPECTED_SIZE(reg_cnt, reg_cnt); */
            GR_SET_EXPECTED_SIZE(2, 2);
            if (NOTVALID(clipRegs)) {
              REG_GROUP_BEGIN(BROADCAST_ID, clipLeftRight, 2, 0x3); 
              REG_GROUP_SET(hw, clipLeftRight, gc->state.shadow.clipLeftRight);
              REG_GROUP_SET(hw, clipBottomTop, gc->state.shadow.clipBottomTop);
              REG_GROUP_END();
            }
            GR_CHECK_SIZE();
        #endif /* !USE_PACKET_FIFO */
#undef FN_NAME
}


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

  /* tbext */
  if ( buffer == GR_BUFFER_TEXTUREBUFFER_EXT )
  {
    GR_CHECK_F(myName,  !gc->textureBuffer.init, "GR_BUFFER_TEXTUREBUFFER_EXT mode not initialised (You need to call grTextureBuffer() at least once)");
    if ( !gc->textureBuffer.prevState.valid )  {
      gc->textureBuffer.prevState.cwMinx = gc->state.clipwindowf_xmin;
      gc->textureBuffer.prevState.cwMaxx = gc->state.clipwindowf_xmax;
      gc->textureBuffer.prevState.cwMiny = gc->state.clipwindowf_ymin;
      gc->textureBuffer.prevState.cwMaxy = gc->state.clipwindowf_ymax;
      gc->textureBuffer.prevState.clipLeftRight = gc->state.shadow.clipLeftRight;
      gc->textureBuffer.prevState.clipBottomTop = gc->state.shadow.clipBottomTop;
      grClipWindow(0, 0, gc->textureBuffer.width, gc->textureBuffer.height);
      _grValidateClipState(0, 0, gc->textureBuffer.width, gc->textureBuffer.height);
      gc->textureBuffer.prevState.valid = FXTRUE;
    } /* endif !buffer->textureBuffer.prevState.valid */

    //
    // AJB: Turn off AA for textureBuffers
    //
#ifdef FX_GLIDE_NAPALM 
    if (gc->grPixelSample > 1) 
    {
      _grAAOffsetValue(_GlideRoot.environment.aaXOffset[0],
                       _GlideRoot.environment.aaYOffset[0],
                       0, gc->chipCount - 1, FXTRUE, FXFALSE);
    }
#endif
    gc->textureBuffer.on = FXTRUE;
    gc->curBuffer = 0xffffffff;
  } else /* buffer != GR_BUFFER_TEXTUREBUFFER_EXT */
  {
    if ( gc->textureBuffer.on ) {
      gc->textureBuffer.on = FXFALSE;
      if ( gc->textureBuffer.prevState.valid )  {
        grClipWindow( (FxU32)gc->textureBuffer.prevState.cwMinx,
                      (FxU32)gc->textureBuffer.prevState.cwMiny,
                      (FxU32)gc->textureBuffer.prevState.cwMaxx,
                      (FxU32)gc->textureBuffer.prevState.cwMaxy );
        gc->curBuffer = 0xffffffff;
        _grValidateClipState( 
                      (FxU32)gc->textureBuffer.prevState.cwMinx,
                      (FxU32)gc->textureBuffer.prevState.cwMiny,
                      (FxU32)gc->textureBuffer.prevState.cwMaxx,
                      (FxU32)gc->textureBuffer.prevState.cwMaxy );
        /* Restore the stride */
        REG_GROUP_BEGIN(BROADCAST_ID, colBufferStride, 1, 0x1); 
        REG_GROUP_SET(hw, colBufferStride,gc->state.shadow.colBufferStride);
        REG_GROUP_END();

        //
        // AJB: Turn AA back on if necessary
        //
#ifdef FX_GLIDE_NAPALM
        if (gc->grPixelSample > 1) {
          if (!gc->state.grEnableArgs.aaMultisampleDisableCount) {
            _grAAOffsetValue(_GlideRoot.environment.aaXOffset[gc->sampleOffsetIndex],
                             _GlideRoot.environment.aaYOffset[gc->sampleOffsetIndex],
                             0, gc->chipCount - 1, FXTRUE, gc->enableSecondaryBuffer) ;
          }
        }
#endif

        gc->textureBuffer.prevState.valid = FXFALSE;
      } /* endif gc->textureBuffer.prevState.valid */
    } /* endif gc->textureBuffer.on */
  } /* endif buffer == GR_BUFFER_TEXTUREBUFFER_EXT */

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
  Function: grColorMaskExt
  Date: 03-July-99
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grColorMaskExt, void , (FxBool r, FxBool g, FxBool b, FxBool a) )
{
#define FN_NAME "grColorMaskExt"

  GR_BEGIN_NOFIFOCHECK("grColorMaskExt\n", 85);

  INVALIDATE(renderMode);

  STOREARG(grColorMaskExt, r);
  STOREARG(grColorMaskExt, g);
  STOREARG(grColorMaskExt, b);
  STOREARG(grColorMaskExt, a);

#undef FN_NAME
} /* grColorMaskExt */

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

  if (gc->state.forced32BPP) level*=256;

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
  Function: grValidateTMUState
  Date: 25-January-2000
  Implementor(s): kcd
  Description:
  State Validation:
  Once a rendering primitive has determined that the state is invalid,
  it calls this routine.  grValidateState then goes through valid
  markers and flushes all invalid state.  This routine handles the TMU
  specific register state.
  -------------------------------------------------------------------*/
  
#define COPY_TMU_STATE(_src, _dst, _var) \
      gc->state.shadow.tmuState[_dst]._var = gc->state.tmuShadow[_src]._var;

#define DEBUG_2PPC 0

void
_grValidateTMUState()
{
#define FN_NAME "_grValidateTMUState"
  FxI32 tmu, tmuSource;
  FxU32 tmuMask;
  FxU32 tmuRequiredByDownstreamUnit, texturingRequiredByFBI;
  FxU32 doTMU0Passthrough, enable2PPC, palletizedTexture;
  FifoChipField chipField;
  SstRegs* tmuHw;
  
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%X)\n", gc->state.invalid);
  
  /* Okay, if we are here then we know we have some amount of work to do.
     The first order of business is to figure out which TMUs are actually
     in use.  First we check to see if the alpha or color combine units
     are set up in such a way that texturing is disabled.  If texturing is
     in use, then we check to see whether or not the upstream TMU exists,
     and if so whether or not it is being used by the downstream TMU.
     
     This leaves us with a few cases to consider for different hardware
     targets:
     
     Banshee:
     
       TMU0 disabled:  "Disable" TMU0 by forcing LOD to 1x1.
       TMU0 enabled:   Update full TMU0 state.
       
     Voodoo3:
     
       TMU0 disabled/TMU1 disabled:  "Disable" both TMUS with LOD trick.
       TMU0 enabled /TMU1 disabled:  Update TMU0 state, "Disable" TMU1
       TMU0 disabled/TMU1  enabled:  "Disable" TMU1, Update TMU1 state
       TMU0 enabled /TMU1  enabled:  Update TMU0 and TMU1 state
       
     Napalm:
       TMU0 disabled/TMU1 disabled:  "Disable" both TMUS with LOD trick, 
                                     sync TMU state and enable 2PPC.
       TMU0 enabled /TMU1 disabled:  Clone TMU1 state to TMU0 and enable
                                     2PPC mode.
       TMU0 disabled/TMU1  enabled:  "Disable" TMU1, Update TMU1 state[1]                                        
       TMU0 enabled /TMU1  enabled:  Update TMU0 and TMU1 state.
                                     
     Note 1: It would be nice if we could do some magic in this case and
             rempap TMU1's state down to TMU0.  We should be able to do this
             if UMA mode is enabled and TMU0 is only in passthrough mode. 
             
     Implementation details:
      
       We let grTexCombine & friends do as much work as possible for computing
       TMU register values and for determining locally when a particular TMU
       needs to have it's LOD stuff forced down because that TMU is going to
       have texturing disabled.   We process the TMUs in order from downstream
       to upstream.   That way we can come back here and check the gc state
       flags to see how we should deal with each upstream TMU as we go.                                            
   */
  
  /* Copy over shadowed TMU state */
  gc->state.tmuMask = gc->state.tmuMaskShadow;
  
  texturingRequiredByFBI = (gc->state.cc_requires_texture || gc->state.ac_requires_texture);
  tmuRequiredByDownstreamUnit = FXTRUE; /* Keep compiler from complaining */
  doTMU0Passthrough = FXFALSE;
  enable2PPC = FXFALSE;
  palletizedTexture = FXFALSE;

  /* If TMU0 is in passthrough mode and TMU1 is doing texturing, then we remap TMU1's
   * register state to TMU0 and do the hardware specific TMU1 shutdown logic. */
  if(IS_NAPALM(gc->bInfo->pciInfo.deviceID) &&
     (gc->do2ppc) &&
     (GR_TMUMASK_TMU0 & gc->state.tmuColorPassthrough & gc->state.tmuAlphaPassthrough) &&
     (gc->state.tmuMask & GR_TMUMASK_TMU1)) {
    doTMU0Passthrough = FXTRUE;
#if DEBUG_2PPC
    GDBG_PRINTF("Enabling TMU0 passthrough mode. cp: %08lx ap: %08lx tm: %08lx\n",
      gc->state.tmuColorPassthrough,
      gc->state.tmuAlphaPassthrough,
      gc->state.tmuMask);
#endif
  }

  palletizedTexture = gc->state.palletizedTexture[0] | gc->state.palletizedTexture[1];

  
  
  for(tmu = 0; tmu < gc->num_tmu; tmu++) {
    FxU32 texSubLodDitherTMU = tmu;
    tmuSource = tmu;

    chipField = (FifoChipField)(0x02UL << tmu);
    tmuHw = SST_TMU(hw, tmu);
    
    /* Generate mask for this TMU. */
    tmuMask = GR_TMUMASK_TMU0 << tmu;
              
    /* If we're an upstream TMU, then look at the downstream TMU to see if it's 
       using us or not... */    
    if(tmu > 0) {
      tmuRequiredByDownstreamUnit = (gc->state.tcc_requires_prev_texture[tmu - 1] ||
                                     gc->state.tac_requires_prev_texture[tmu - 1]) && tmuRequiredByDownstreamUnit;
    } else {
      tmuRequiredByDownstreamUnit = texturingRequiredByFBI;
    }
                     
    /* If texturing is enabled both globally and on this TMU, then update it's state. */
    /* Note: We always do the more granular checks on TMU so that we force ourselves  */
    /* to update things such as the tLOD register if the texCombine code has deemed   */
    /* texturing to not be in use.   This also guarantees that TMU0's state is updated */
    /* in such a way that we can clone it's register values for TMU1 if we are doing   */
    /* 2PPC mode on Napalm. */        

#if DEBUG_2PPC
    GDBG_PRINTF("TMU%d required by downstream unit: %d c: %d a: %d\n",tmu,tmuRequiredByDownstreamUnit,
        tmu > 0 ? gc->state.tcc_requires_prev_texture[tmu - 1] : gc->state.cc_requires_texture,
        tmu > 0 ? gc->state.tcc_requires_prev_texture[tmu - 1] : gc->state.ac_requires_texture);
#endif
       
    if((tmuRequiredByDownstreamUnit && !doTMU0Passthrough) || palletizedTexture) {
#if DEBUG_2PPC
      GDBG_PRINTF("Setup TMU%d\n",tmu);
#endif
#if FX_GLIDE_NAPALM      
      if(IS_NAPALM(gc->bInfo->pciInfo.deviceID) && gc->do2ppc) {
        /* Make sure 2PPC mode is disabled. */
          _grTex2ppc(FXFALSE);
#if DEBUG_2PPC
          GDBG_PRINTF("2PPC mode off\n");
#endif
      }
#endif      
      /* Update TMU register state from backup shadows, then update registers for real. */
      /* For theoretical code performance reasons, I'm grouping the registers together */
      /* In the groups that seem most likely to change at the same time. */      
      if(NOTVALID_TMU(tmu, textureMode)) {
        
        /* non-compressed -> compressed chip bug workaround. */
        if((gc->state.tmuShadow[tmu].textureMode ^ 
            gc->state.shadow.tmuState[tmu].textureMode) & SST_COMPRESSED_TEXTURES & 
            gc->state.tmuShadow[tmu].textureMode) {
          GR_SET_EXPECTED_SIZE(sizeof(FxU32)*1, 1);
          GR_SET(eChipTMU0 | eChipTMU1, hw, nopCMD,  0);
          GR_CHECK_SIZE();
        }
        
        COPY_TMU_STATE(tmu, tmu, textureMode);
        /* If this TMU is not using texturing then force it's LOD to 1x1 */
        if((gc->state.tmuMask & tmuMask) == 0) {
#if DEBUG_2PPC
          GDBG_PRINTF("TMU%d LOD forced to 1x1\n");
#endif          
          gc->state.shadow.tmuState[tmu].tLOD = SST_TLOD_MINMAX_INT(8, 8);
        } else {
          COPY_TMU_STATE(tmu, tmu, tLOD);
        }
        COPY_TMU_STATE(tmu, tmu, tDetail);
        REG_GROUP_BEGIN(chipField, textureMode, 3, 0x07);
		{
          REG_GROUP_SET(tmuHw, textureMode, gc->state.shadow.tmuState[tmu].textureMode);
          REG_GROUP_SET(tmuHw, tLOD, gc->state.shadow.tmuState[tmu].tLOD);
          REG_GROUP_SET(tmuHw, tDetail, gc->state.shadow.tmuState[tmu].tDetail);          
#if DEBUG_2PPC && 1
          GDBG_PRINTF("TMU%d textureMode: %08lx\n",tmu,gc->state.shadow.tmuState[tmu].textureMode);
          GDBG_PRINTF("TMU%d tLod:        %08lx\n",tmu,gc->state.shadow.tmuState[tmu].tLOD);
          GDBG_PRINTF("TMU%d tDetail:     %08lx\n",tmu,gc->state.shadow.tmuState[tmu].tDetail);          
#endif
        }
        REG_GROUP_END();

        if(gc->state.per_tmu[tmu].texSubLodDither)
            g3LodBiasPerChip(tmu, gc->state.shadow.tmuState[tmu].tLOD);
        
        if(IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
          COPY_TMU_STATE(tmu, tmu, combineMode);
          REG_GROUP_BEGIN(chipField, combineMode, 1, 0x01);
          {
            REG_GROUP_SET(tmuHw, combineMode, gc->state.shadow.tmuState[tmu].combineMode);
#if DEBUG_2PPC && 1
            GDBG_PRINTF("TMU%d combineMode: %08lx\n",tmu,gc->state.shadow.tmuState[tmu].combineMode);
#endif
          }
          REG_GROUP_END();
        }
        
      }
      
      if(NOTVALID_TMU(tmu, texBaseAddr)) {
        COPY_TMU_STATE(tmu, tmu, texBaseAddr);
        COPY_TMU_STATE(tmu, tmu, texBaseAddr_1);
        COPY_TMU_STATE(tmu, tmu, texBaseAddr_2);
        COPY_TMU_STATE(tmu, tmu, texBaseAddr_3_8);
        
        REG_GROUP_BEGIN(chipField, texBaseAddr, 4, 0x0f);
        {
          REG_GROUP_SET(tmuHw, texBaseAddr, gc->state.shadow.tmuState[tmu].texBaseAddr);
          REG_GROUP_SET(tmuHw, texBaseAddr1, gc->state.shadow.tmuState[tmu].texBaseAddr_1);
          REG_GROUP_SET(tmuHw, texBaseAddr2, gc->state.shadow.tmuState[tmu].texBaseAddr_2);
          REG_GROUP_SET(tmuHw, texBaseAddr38, gc->state.shadow.tmuState[tmu].texBaseAddr_3_8);
#if DEBUG_2PPC && 1
          GDBG_PRINTF("TMU%d texBaseAddr: %08lx\n",tmu,gc->state.shadow.tmuState[tmu].texBaseAddr);
#endif
        }
        REG_GROUP_END();
      }
      
      if(NOTVALID_TMU(tmu, texchroma)) {
        if(gc->state.shadow.tmuState[tmu].combineMode & SST_CM_DISABLE_CHROMA_SUBSTITUTION) {
          gc->state.shadow.tmuState[tmu].texchromaKey   = gc->state.tmuColor[tmu];
          gc->state.shadow.tmuState[tmu].texchromaRange = gc->state.tmuColor[tmu];
        } else {
          COPY_TMU_STATE(tmu, tmu, texchromaKey);
          COPY_TMU_STATE(tmu, tmu, texchromaRange);
        }
        REG_GROUP_BEGIN(chipField, chromaKey, 2, 0x03);
        {
          REG_GROUP_SET(tmuHw, chromaKey, gc->state.shadow.tmuState[tmu].texchromaKey);
          REG_GROUP_SET(tmuHw, chromaRange, gc->state.shadow.tmuState[tmu].texchromaRange);
#if DEBUG_2PPC && 1
          GDBG_PRINTF("TMU%d texchromakey %08lx\n",tmu,gc->state.shadow.tmuState[tmu].texchromaKey);
          GDBG_PRINTF("TMU%d texchromarng %08lx\n",tmu,gc->state.shadow.tmuState[tmu].texchromaRange);
#endif
        }
        REG_GROUP_END();
      }
      
      /* Mark TMU state as valid. */
      gc->state.tmuInvalid[tmu] = 0;
      
  
      } else {
#if DEBUG_2PPC
      GDBG_PRINTF("TMU%d not required by downstream units or passthrough enabled\n",tmu);
#endif
      chipField = (FifoChipField)(0x02UL << tmu);
      tmuHw = SST_TMU(hw, tmu);
#ifdef FX_GLIDE_NAPALM
      if ((IS_NAPALM(gc->bInfo->pciInfo.deviceID)) && (gc->do2ppc)) {
        if(tmu == 0 && doTMU0Passthrough) {
#if DEBUG_2PPC
          GDBG_PRINTF("TMU1:s -> TMU0:r\n");
#endif          
          /* non-compressed -> compressed chip bug workaround. */
          if((gc->state.tmuShadow[GR_TMU1].textureMode ^ 
              gc->state.shadow.tmuState[GR_TMU0].textureMode) & SST_COMPRESSED_TEXTURES & 
              gc->state.tmuShadow[GR_TMU1].textureMode) {
            GR_SET_EXPECTED_SIZE(sizeof(FxU32)*1, 1);
            GR_SET(eChipTMU0 | eChipTMU1, hw, nopCMD,  0);
            GR_CHECK_SIZE();
          }
          /* Copy current TMU1 shadow register state to TMU0 real register state. */
          gc->state.shadow.tmuState[GR_TMU0].textureMode     = gc->state.tmuShadow[GR_TMU1].textureMode;
          gc->state.shadow.tmuState[GR_TMU0].tLOD            = gc->state.tmuShadow[GR_TMU1].tLOD;
          gc->state.shadow.tmuState[GR_TMU0].tDetail         = gc->state.tmuShadow[GR_TMU1].tDetail;
          gc->state.shadow.tmuState[GR_TMU0].texBaseAddr     = gc->state.tmuShadow[GR_TMU1].texBaseAddr;
          gc->state.shadow.tmuState[GR_TMU0].texBaseAddr_1   = gc->state.tmuShadow[GR_TMU1].texBaseAddr_1;
          gc->state.shadow.tmuState[GR_TMU0].texBaseAddr_2   = gc->state.tmuShadow[GR_TMU1].texBaseAddr_2;
          gc->state.shadow.tmuState[GR_TMU0].texBaseAddr_3_8 = gc->state.tmuShadow[GR_TMU1].texBaseAddr_3_8;
          gc->state.shadow.tmuState[GR_TMU0].combineMode     = gc->state.tmuShadow[GR_TMU1].combineMode;   
          if(gc->state.shadow.tmuState[GR_TMU0].combineMode & SST_CM_DISABLE_CHROMA_SUBSTITUTION) {
            gc->state.shadow.tmuState[GR_TMU0].texchromaKey   = gc->state.tmuColor[GR_TMU1];
            gc->state.shadow.tmuState[GR_TMU0].texchromaRange = gc->state.tmuColor[GR_TMU1];
          } else {
            gc->state.shadow.tmuState[GR_TMU0].texchromaKey    = gc->state.tmuShadow[GR_TMU1].texchromaKey;
            gc->state.shadow.tmuState[GR_TMU0].texchromaRange  = gc->state.tmuShadow[GR_TMU1].texchromaRange;          
          }
                           
          /* Flag all TMU0 registers as invalid */
          gc->state.tmuInvalid[GR_TMU0] = 0xffffffff;

          texSubLodDitherTMU = GR_TMU1;

        } else if(tmu > 0) {
#if DEBUG_2PPC        
          GDBG_PRINTF("TMU0:r -> TMU1:r\n");
#endif          
          /* non-compressed -> compressed chip bug workaround. */
          if((gc->state.tmuShadow[GR_TMU0].textureMode ^ 
              gc->state.shadow.tmuState[GR_TMU1].textureMode) & SST_COMPRESSED_TEXTURES & 
              gc->state.tmuShadow[GR_TMU0].textureMode) {
            GR_SET_EXPECTED_SIZE(sizeof(FxU32)*1, 1);
            GR_SET(eChipTMU0 | eChipTMU1, hw, nopCMD,  0);
            GR_CHECK_SIZE();
          }
          
          /* Copy current TMU0 real register state to TMU1 real register state and enable 2PPC mode. */
          gc->state.shadow.tmuState[GR_TMU1].textureMode     = gc->state.shadow.tmuState[GR_TMU0].textureMode;
          gc->state.shadow.tmuState[GR_TMU1].tLOD            = gc->state.shadow.tmuState[GR_TMU0].tLOD;
          gc->state.shadow.tmuState[GR_TMU1].tDetail         = gc->state.shadow.tmuState[GR_TMU0].tDetail;
          gc->state.shadow.tmuState[GR_TMU1].texBaseAddr     = gc->state.shadow.tmuState[GR_TMU0].texBaseAddr;
          gc->state.shadow.tmuState[GR_TMU1].texBaseAddr_1   = gc->state.shadow.tmuState[GR_TMU0].texBaseAddr_1;
          gc->state.shadow.tmuState[GR_TMU1].texBaseAddr_2   = gc->state.shadow.tmuState[GR_TMU0].texBaseAddr_2;
          gc->state.shadow.tmuState[GR_TMU1].texBaseAddr_3_8 = gc->state.shadow.tmuState[GR_TMU0].texBaseAddr_3_8;
          gc->state.shadow.tmuState[GR_TMU1].texchromaKey    = gc->state.shadow.tmuState[GR_TMU0].texchromaKey;
          gc->state.shadow.tmuState[GR_TMU1].texchromaRange  = gc->state.shadow.tmuState[GR_TMU0].texchromaRange;          
          gc->state.shadow.tmuState[GR_TMU1].combineMode     = gc->state.shadow.tmuState[GR_TMU0].combineMode;          
          /* Flag all TMU1 registers as invalid */
          gc->state.tmuInvalid[tmu] = 0xffffffff;
          
          /* Now that we know that TMU1 is not being used and it's state matches TMU0, we can enable 2PPC mode. */
          enable2PPC = FXTRUE;

          texSubLodDitherTMU = GR_TMU0;
          
        } else {    
#if DEBUG_2PPC        
          GDBG_PRINTF("TMU0:s -> TMU0:r\n");
#endif          
          /* non-compressed -> compressed chip bug workaround. */
          if((gc->state.tmuShadow[GR_TMU0].textureMode ^ 
              gc->state.shadow.tmuState[GR_TMU0].textureMode) & SST_COMPRESSED_TEXTURES & 
              gc->state.tmuShadow[GR_TMU0].textureMode) {
            GR_SET_EXPECTED_SIZE(sizeof(FxU32)*1, 1);
            GR_SET(eChipTMU0 | eChipTMU1, hw, nopCMD,  0);
            GR_CHECK_SIZE();
          }
          
          /* Just update TMU0's complete register state for now. */
          gc->state.tmuInvalid[tmu] = 0;
          
          COPY_TMU_STATE(tmu, tmu, textureMode);
          /* If this TMU is not using texturing then set force it's LOD to 1x1 */
          if((gc->state.tmuMask & tmuMask) == 0) {
            gc->state.shadow.tmuState[tmu].tLOD = SST_TLOD_MINMAX_INT(8, 8);
            INVALIDATE_TMU(tmu, textureMode);
          } else {
            COPY_TMU_STATE(tmu, tmu, tLOD);
          }
          COPY_TMU_STATE(tmu, tmu, tDetail);
          COPY_TMU_STATE(tmu, tmu, combineMode);
          COPY_TMU_STATE(tmu, tmu, texBaseAddr);
          COPY_TMU_STATE(tmu, tmu, texBaseAddr_1);
          COPY_TMU_STATE(tmu, tmu, texBaseAddr_2);
          COPY_TMU_STATE(tmu, tmu, texBaseAddr_3_8);
          if(gc->state.shadow.tmuState[tmu].combineMode & SST_CM_DISABLE_CHROMA_SUBSTITUTION) {
            gc->state.shadow.tmuState[tmu].texchromaKey   = gc->state.tmuColor[tmu];
            gc->state.shadow.tmuState[tmu].texchromaRange = gc->state.tmuColor[tmu];
          } else {
            COPY_TMU_STATE(tmu, tmu, texchromaKey);
            COPY_TMU_STATE(tmu, tmu, texchromaRange);
          }
        }
        
        /* This code is not yet optimal, as it will always update all of the TMU registers any time */
        /* the combine state chages and the TMU is not in use. */
        REG_GROUP_BEGIN(chipField, textureMode, 7, 0x7F);
        {
          REG_GROUP_SET(tmuHw, textureMode, gc->state.shadow.tmuState[tmu].textureMode);
          REG_GROUP_SET(tmuHw, tLOD, gc->state.shadow.tmuState[tmu].tLOD);
          REG_GROUP_SET(tmuHw, tDetail, gc->state.shadow.tmuState[tmu].tDetail);
          REG_GROUP_SET(tmuHw, texBaseAddr, gc->state.shadow.tmuState[tmu].texBaseAddr);
    
          REG_GROUP_SET(tmuHw, texBaseAddr1, gc->state.shadow.tmuState[tmu].texBaseAddr_1);
          REG_GROUP_SET(tmuHw, texBaseAddr2, gc->state.shadow.tmuState[tmu].texBaseAddr_2);
          REG_GROUP_SET(tmuHw, texBaseAddr38, gc->state.shadow.tmuState[tmu].texBaseAddr_3_8);
#if DEBUG_2PPC && 1
          GDBG_PRINTF("TMU%d textureMode: %08lx\n",tmu,gc->state.shadow.tmuState[tmu].textureMode);
          GDBG_PRINTF("TMU%d tLod:        %08lx\n",tmu,gc->state.shadow.tmuState[tmu].tLOD);
          GDBG_PRINTF("TMU%d tDetail:     %08lx\n",tmu,gc->state.shadow.tmuState[tmu].tDetail);
          GDBG_PRINTF("TMU%d texBaseAddr: %08lx\n",tmu,gc->state.shadow.tmuState[tmu].texBaseAddr);
#endif           
        }
        REG_GROUP_END();

        if(gc->state.per_tmu[texSubLodDitherTMU].texSubLodDither)
            g3LodBiasPerChip(tmu, gc->state.shadow.tmuState[tmu].tLOD);
  
        REG_GROUP_BEGIN(chipField, combineMode, 1, 0x01);
        {
          REG_GROUP_SET(tmuHw, combineMode, gc->state.shadow.tmuState[tmu].combineMode);
#if DEBUG_2PPC && 1
          GDBG_PRINTF("TMU%d combineMode: %08lx\n",tmu,gc->state.shadow.tmuState[tmu].combineMode);
#endif          
        }
        REG_GROUP_END();
  
        REG_GROUP_BEGIN(chipField, chromaKey, 2, 0x03);
        {
          REG_GROUP_SET(tmuHw, chromaKey, gc->state.shadow.tmuState[tmu].texchromaKey);
          REG_GROUP_SET(tmuHw, chromaRange, gc->state.shadow.tmuState[tmu].texchromaRange);
#if DEBUG_2PPC && 1
          GDBG_PRINTF("TMU%d texchromakey %08lx\n",tmu,gc->state.shadow.tmuState[tmu].texchromaKey);
          GDBG_PRINTF("TMU%d texchromarng %08lx\n",tmu,gc->state.shadow.tmuState[tmu].texchromaRange);
#endif          
        }
        REG_GROUP_END();

        if(enable2PPC) {
          /* There are two cases, either we're using TMU0's state on both TMUs or we are using
             TMU1's state on both.  We need to disabled the W,S,T parameters for the TMU that 
             is not being used, otherwise bad things happen because when 2PPC mode is enabled,
             any writes that go to one TMU will get sent to both. */
          if(doTMU0Passthrough) {
            gc->state.tmuMask &= ~GR_TMUMASK_TMU0;
            gc->state.mode2ppcTMU = GR_TMU1;
          } else {
            gc->state.tmuMask &= ~GR_TMUMASK_TMU1;
            gc->state.mode2ppcTMU = GR_TMU0;
          }
          _grTex2ppc(FXTRUE);
        }
        
      } else {
#endif      

        /* Set LOD on TMU to 1x1. */
        SstRegs* tmuHw = SST_TMU(hw, tmu);

        /* GDBG_PRINTF("tmu[%d]: forced off because it's not in use by downstream unit.\n",tmu); */
        GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
        /* %%% NOTE: We use 8 here which is the hardware value for 1x1 LOD, rather than using a #define which
                     isn't consistant between Glide2 and Glide3. */ 
        GR_SET((0x02UL << tmu), tmuHw, tLOD, SST_TLOD_MINMAX_INT(8, 8));
        GR_CHECK_SIZE();

        /* Mark tLOD register as invalid so it gets set to the correct value if/when this TMU is re-enabled. */
        INVALIDATE_TMU(tmu, textureMode);
                        
        /* Update "real" register shadow too. */
        gc->state.shadow.tmuState[tmu].tLOD = SST_TLOD_MINMAX_INT(8, 8);
#ifdef FX_GLIDE_NAPALM              
      }
#endif    
    }     

}
#undef FN_NAME  
}

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

  FxU32
    alphaTestOptimization = FXFALSE,
    updateAlphaMode = FXFALSE;
    
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%X)\n", gc->state.invalid);

  GR_ASSERT((gc->state.combineExtsInUse == 0) ||
            (gc->state.combineExtsInUse == 
            (STATE_USING_CC|STATE_USING_CA|
            STATE_USING_TCC|STATE_USING_TAC)));

  /* Make sure that state is maintained across all chips! */
  if(gc->state.invalid) {
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
  }  

  /* NB: The values in the write shadow must be set in register order
   * for the unsafe reg_group write.  
   */
  if (NOTVALID(fbzColorPath)) {
    FxU32 
      oldTextureEnabled = gc->state.shadow.fbzColorPath & SST_ENTEXTUREMAP;

#ifdef FX_GLIDE_NAPALM
    if (gc->state.grEnableArgs.combine_ext_mode == GR_MODE_ENABLE) {

      _grACExtfbzColorPath(LOADARG(grAlphaCombineExt, a),
                           LOADARG(grAlphaCombineExt, a_mode),
                           LOADARG(grAlphaCombineExt, b),
                           LOADARG(grAlphaCombineExt, b_mode),
                           LOADARG(grAlphaCombineExt, c),
                           LOADARG(grAlphaCombineExt, c_invert),
                           LOADARG(grAlphaCombineExt, d),
                           LOADARG(grAlphaCombineExt, invert));
      _grAlphaControlsITRGBLighting(LOADARG(grAlphaControlsITRGBLighting, enable)); 
      _grCCExtfbzColorPath(LOADARG(grColorCombineExt, a),
                           LOADARG(grColorCombineExt, a_mode),
                           LOADARG(grColorCombineExt, b),
                           LOADARG(grColorCombineExt, b_mode),
                           LOADARG(grColorCombineExt, c),
                           LOADARG(grColorCombineExt, c_invert),
                           LOADARG(grColorCombineExt, d),
                           LOADARG(grColorCombineExt, invert));
    }
    else {
      FxU32 combineMode = gc->state.shadow.combineMode;
      FxU32 isCombineExt = (combineMode & SST_CM_USE_COMBINE_MODE);
      if (isCombineExt) {
        combineMode &= ~( 
                         SST_CM_CC_OTHERSELECT | 
                         SST_CM_CC_LOCALSELECT |
                         SST_CM_CC_MSELECT_7 | 
                         SST_CM_CC_INVERT_OTHER |
                         SST_CM_CC_INVERT_LOCAL |
                         SST_CM_CC_OUTSHIFT |
                         SST_CM_USE_COMBINE_MODE |
                         SST_CM_CC_INVERT_ADD_LOCAL |
                         SST_CM_CCA_OTHERSELECT | 
                         SST_CM_CCA_LOCALSELECT |
                         SST_CM_CCA_INVERT_OTHER |
                         SST_CM_CCA_INVERT_LOCAL |
                         SST_CM_CCA_OUTSHIFT |
                         SST_CM_USE_COMBINE_MODE |
                         SST_CM_CCA_INVERT_ADD_LOCAL
                         );
        REG_GROUP_BEGIN(eChipFBI, combineMode, 1, 0x01);
        {
          REG_GROUP_SET(hw, combineMode, combineMode);
        }
        REG_GROUP_END();      
        gc->state.shadow.combineMode = combineMode;
#if !USE_PACKET_FIFO
        { /* hack alert! for csim only */
          int tmu;
          
          for(tmu = 0; tmu < gc->num_tmu; tmu++) {
            SstRegs* tmuregs = SST_TMU(hw, tmu);
            const FifoChipField chipField = (FifoChipField)(0x02UL << tmu);
            REG_GROUP_BEGIN(chipField, combineMode, 1, 0x01);
            {
              REG_GROUP_SET(tmuregs, combineMode, gc->state.shadow.tmuState[tmu].combineMode);
            }
            REG_GROUP_END();
            if (gc->do2ppc)
              tmu = gc->num_tmu;
          }
        }
#endif
      }
    }
#endif
    if (gc->state.grEnableArgs.combine_ext_mode == GR_MODE_DISABLE) {
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
    }
        
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

  /* %%KCD - We no longer include fogMode in the bulk register update
   * because we may have to send different values to different chips. */
  #if 0
  if (NOTVALID(fogMode)) {
    _grFogMode(LOADARG(grFogMode, mode));

    writeShadow[reg_cnt] = gc->state.shadow.fogMode;

    mask |= STATE_REG_MASK(fogMode);
    reg_cnt++;
  }
  #endif

  /* Check for alpha test optimization */
  if (NOTVALID(alphaMode) || NOTVALID(fbzMode) || NOTVALID(stencilMode)) {
    updateAlphaMode = FXTRUE;
    if((LOADARG(grAlphaBlendFunction, rgb_sf) == GR_BLEND_SRC_ALPHA) &&
       (LOADARG(grAlphaBlendFunction, rgb_df) == GR_BLEND_ONE_MINUS_SRC_ALPHA) &&
       (LOADARG(grAlphaBlendFunction, rgb_op) == GR_BLEND_OP_ADD) &&
       (LOADARG(grDepthMask, enable) == FXFALSE) &&
       ((LOADARG(grStencilMask, value) == 0x00) ||
        (gc->state.grEnableArgs.stencil_mode == FXFALSE))) {
      //GDBG_PRINTF("Alpha test optimization enabled.\n");
      alphaTestOptimization = FXTRUE;
    } else {
      //GDBG_PRINTF("Alpha test optimization disabled.\n");
    }  
  }

  if (NOTVALID(alphaMode) || updateAlphaMode) {
    _grAlphaBlendFunction(LOADARG(grAlphaBlendFunction, rgb_sf),
                          LOADARG(grAlphaBlendFunction, rgb_df),
                          LOADARG(grAlphaBlendFunction, alpha_sf),
                          LOADARG(grAlphaBlendFunction, alpha_df));
    _grAlphaTestFunction(LOADARG(grAlphaTestFunction, fnc));
    _grAlphaTestReferenceValue(LOADARG(grAlphaTestReferenceValue, value));

    /* If conditions are right, then we override alpha testing if it's
     * not already enabled. */
    if(alphaTestOptimization && !(gc->state.shadow.alphaMode & SST_ENALPHAFUNC)) {
      gc->state.shadow.alphaMode &= ~(SST_ALPHAFUNC | SST_ENALPHAFUNC | SST_ALPHAREF);
      gc->state.shadow.alphaMode |= (SST_ENALPHAFUNC | SST_ALPHAFUNC_GT);
    }
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
        /* tbext */
    if ( gc->textureBuffer.on ) 
    {
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3);
      REG_GROUP_SET(hw, colBufferAddr, gc->textureBuffer.addr );
      REG_GROUP_SET(hw,colBufferStride,gc->textureBuffer.stride );
      REG_GROUP_END();
    }

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
                               ((enableAlphaMask != 0) && (gc->state.shadow.fbzMode & SST_ENALPHABUFFER)
                                && (gc->bInfo->h3pixelSize < 4)),
                               "alpha writes enabled even though depth buffering");

        if (gc->grAuxBuf != 0)
          fbzMode |= SST_ZAWRMASK;
      } else if (enableAlphaMask > 0) {
        GR_CHECK_COMPATABILITY(FN_NAME,
                               ((fbzMode & SST_ENDEPTHBUFFER) && (gc->bInfo->h3pixelSize < 4)),
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

  if (NOTVALID(lfbMode)) {
    FxU32
      lfbMode = gc->state.shadow.lfbMode;

    lfbMode &= ~(SST_LFB_RGBALANES | 
                 SST_LFB_WRITE_SWAP16 | 
                 SST_LFB_WRITE_BYTESWAP);

    lfbMode |= ((LOADARG(grLfbWriteColorFormat, colorFormat) << SST_LFB_RGBALANES_SHIFT) |
                (LOADARG(grLfbWriteColorSwizzle, swizzleBytes) << 12UL) |
                (LOADARG(grLfbWriteColorSwizzle, swapWords) << 11UL));

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

    if (_GlideRoot.environment.guardbandclipping) {
#ifdef FX_GLIDE_NAPALM
      REG_GROUP_BEGIN(BROADCAST_ID, clipLeftRight1, 2, 0x03);
      {
        REG_GROUP_SET(hw, clipLeftRight1, gc->state.shadow.clipLeftRight1);
        REG_GROUP_SET(hw, clipBottomTop1, gc->state.shadow.clipBottomTop1);
  }
      REG_GROUP_END();
#endif
    }
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
    //GR_ASSERT(zaColor == gc->state.shadow.zaColor);

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
#if GLIDE_DEBUG
                GrColor_t saveColorRange = LOADARG(grChromaRange, range);
#endif          
    chromaRange &= SST_ENCHROMARANGE;

    _grSwizzleColor(&LOADARG(grChromaRange, range));
    chromaRange |= ((LOADARG(grChromaRange, range) & 0x00FFFFFFUL) |
                    (LOADARG(grChromaRange, match_mode) << 24UL));
#if GLIDE_DEBUG
    _grChromaRange(saveColorRange,
                   LOADARG(grChromaRange, match_mode));
#endif /* GLIDE_DEBUG */
    GR_ASSERT(chromaRange == gc->state.shadow.chromaRange);

    gc->state.shadow.chromaRange = chromaRange;
    writeShadow[reg_cnt] = chromaRange;

    mask |= STATE_REG_MASK(chromaRange);
    reg_cnt++;
  }

#ifdef FX_GLIDE_NAPALM
  if (NOTVALID(stencilMode)) {
    FxU32 stencilMode = gc->state.shadow.stencilMode;
    stencilMode &= ~(SST_STENCIL_FUNC | SST_STENCIL_REF | SST_STENCIL_MASK | 
                     SST_STENCIL_WMASK | SST_STENCIL_ENABLE);
    if(gc->state.grEnableArgs.stencil_mode) {
      stencilMode |= ( LOADARG(grStencilFunc, fnc) << SST_STENCIL_FUNC_SHIFT ) | 
                     ( LOADARG(grStencilFunc, ref) << SST_STENCIL_REF_SHIFT ) | 
                     ( LOADARG(grStencilFunc, mask) << SST_STENCIL_MASK_SHIFT ) | 
                     ( LOADARG(grStencilMask, value) << SST_STENCIL_WMASK_SHIFT ) | 
                     ( SST_STENCIL_ENABLE);
    } else {
      stencilMode = SST_STENCIL_MODE_DISABLE;
    }    

    gc->state.shadow.stencilMode = stencilMode;
    REG_GROUP_BEGIN(BROADCAST_ID, stencilMode, 1, 0x01);
    {
      REG_GROUP_SET(hw, stencilMode, stencilMode);
    }
    REG_GROUP_END();
  }

  if (NOTVALID(stipple)) {
    gc->state.shadow.stipple = LOADARG(grStipplePattern, stipple);
    REG_GROUP_BEGIN(BROADCAST_ID, stipple, 1, 0x01);
    {
      REG_GROUP_SET(hw, stipple, gc->state.shadow.stipple);
    }
    REG_GROUP_END();
  }

  if (NOTVALID(stencilOp)) {
    FxU32 stencilOp = gc->state.shadow.stencilOp;
    stencilOp &= ~(SST_STENCIL_SFAIL_OP | SST_STENCIL_ZFAIL_OP | SST_STENCIL_ZPASS_OP);
    stencilOp |= 
      (LOADARG(grStencilOp, stencil_fail) << SST_STENCIL_SFAIL_OP_SHIFT) | 
      (LOADARG(grStencilOp, depth_fail) << SST_STENCIL_ZFAIL_OP_SHIFT) | 
      (LOADARG(grStencilOp, depth_pass) << SST_STENCIL_ZPASS_OP_SHIFT);
    
    gc->state.shadow.stencilOp = stencilOp;
    REG_GROUP_BEGIN(BROADCAST_ID, stencilOp, 1, 0x01);
    {
      REG_GROUP_SET(hw, stencilOp, stencilOp);
    }
    REG_GROUP_END();
  }

  if (NOTVALID(combineMode)) {
    _grCCExtcombineMode( LOADARG(grColorCombineExt, a),
                         LOADARG(grColorCombineExt, a_mode),
                         LOADARG(grColorCombineExt, b),
                         LOADARG(grColorCombineExt, b_mode),
                         LOADARG(grColorCombineExt, c),
                         LOADARG(grColorCombineExt, d_invert),
                         LOADARG(grColorCombineExt, shift) );
    _grACExtcombineMode( LOADARG(grAlphaCombineExt, a),
                         LOADARG(grAlphaCombineExt, a_mode),
                         LOADARG(grAlphaCombineExt, b),
                         LOADARG(grAlphaCombineExt, b_mode),
                         LOADARG(grAlphaCombineExt, d_invert),
                         LOADARG(grAlphaCombineExt, shift) );
    gc->state.cc_requires_it_rgb = ( 
                                    (LOADARG(grColorCombineExt, a) == GR_CMBX_ITRGB) |
                                    (LOADARG(grColorCombineExt, b) == GR_CMBX_ITRGB) |
                                    (LOADARG(grColorCombineExt, c) == GR_CMBX_ITRGB) |
                                    (LOADARG(grColorCombineExt, d) == GR_CMBX_ITRGB)
                                    );

    gc->state.ac_requires_it_alpha = ( 
                                      (LOADARG(grColorCombineExt, a) == GR_CMBX_ITALPHA) |
                                      (LOADARG(grColorCombineExt, b) == GR_CMBX_ITALPHA) |
                                      (LOADARG(grColorCombineExt, c) == GR_CMBX_ITALPHA) |
                                      (LOADARG(grAlphaCombineExt, a) == GR_CMBX_ITALPHA) |
                                      (LOADARG(grAlphaCombineExt, b) == GR_CMBX_ITALPHA) |
                                      (LOADARG(grAlphaCombineExt, c) == GR_CMBX_ITALPHA) |
                                      (LOADARG(grAlphaCombineExt, d) == GR_CMBX_ITALPHA)
                                      );

    REG_GROUP_BEGIN(eChipFBI, combineMode, 1, 0x01);
    {
      REG_GROUP_SET(hw, combineMode, gc->state.shadow.combineMode);
    }
    REG_GROUP_END();      
#if !USE_PACKET_FIFO
    { /* hack alert! for csim only */
      int tmu;
      
      for(tmu = 0; tmu < gc->num_tmu; tmu++) {
        SstRegs* tmuregs = SST_TMU(hw, tmu);
        const FifoChipField chipField = (FifoChipField)(0x02UL << tmu);
        REG_GROUP_BEGIN(chipField, combineMode, 1, 0x01);
        {
          REG_GROUP_SET(tmuregs, combineMode, gc->state.shadow.tmuState[tmu].combineMode);
        }
        REG_GROUP_END();
        if (gc->do2ppc)
          tmu = gc->num_tmu;
      }
    }
#endif
  }

  if (NOTVALID(renderMode)) {
    FxU32 renderMode = gc->state.shadow.renderMode;
    renderMode &= ~(
                    SST_RM_RED_WMASK |
                    SST_RM_GREEN_WMASK |
                    SST_RM_BLUE_WMASK |
                    SST_RM_ALPHA_WMASK
                    );
    renderMode |= (LOADARG(grColorMaskExt, r)) ? SST_RM_RED_WMASK : 0x0;
    renderMode |= (LOADARG(grColorMaskExt, g)) ? SST_RM_GREEN_WMASK : 0x0;
    renderMode |= (LOADARG(grColorMaskExt, b)) ? SST_RM_BLUE_WMASK : 0x0;
    renderMode |= (LOADARG(grColorMaskExt, a)) ? SST_RM_ALPHA_WMASK : 0x0;
    gc->state.shadow.renderMode = renderMode;
    REG_GROUP_BEGIN(eChipFBI, renderMode, 1, 0x01);
    {
      REG_GROUP_SET(hw, renderMode, renderMode);
    }
    REG_GROUP_END();      
  }

#endif

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
    {
      if (NOTVALID(fbzColorPath)) {
        REG_GROUP_BEGIN(eChipFBI, fbzColorPath, 1, 0x01);
        REG_GROUP_SET(hw, fbzColorPath, gc->state.shadow.fbzColorPath);
        REG_GROUP_END();      
      }
      if (NOTVALID(fogMode)) {
        REG_GROUP_BEGIN(eChipFBI, fogMode, 1, 0x01);
        REG_GROUP_SET(hw, fogMode, gc->state.shadow.fogMode);
        REG_GROUP_END();      
      }
      if (NOTVALID(alphaMode)) {
        REG_GROUP_BEGIN(eChipFBI, alphaMode, 1, 0x01);
        REG_GROUP_SET(hw, alphaMode, gc->state.shadow.alphaMode);
        REG_GROUP_END();      
      }
      if (NOTVALID(fbzMode)) {
        REG_GROUP_BEGIN(eChipFBI, fbzMode, 1, 0x01);
        REG_GROUP_SET(hw, fbzMode, gc->state.shadow.fbzMode);
        REG_GROUP_END();      
      }
      
      if (NOTVALID(lfbMode)) {
        REG_GROUP_BEGIN(eChipFBI, lfbMode, 1, 0x01);
        REG_GROUP_SET(hw, lfbMode, gc->state.shadow.lfbMode);
        REG_GROUP_END();      
      }
      if (NOTVALID(clipRegs)) {
        REG_GROUP_BEGIN(eChipFBI, clipLeftRight, 2, 0x03);
        REG_GROUP_SET(hw, clipLeftRight, gc->state.shadow.clipLeftRight);
        REG_GROUP_SET(hw, clipBottomTop, gc->state.shadow.clipBottomTop);
        REG_GROUP_END();      
      }
      
      if (NOTVALID(fogColor)) {
        REG_GROUP_BEGIN(eChipFBI, fogColor, 1, 0x01);
        REG_GROUP_SET(hw, fogColor, gc->state.shadow.fogColor);
        REG_GROUP_END();      
      }
      if (NOTVALID(zaColor)) {
        REG_GROUP_BEGIN(eChipFBI, zaColor, 1, 0x01);
        REG_GROUP_SET(hw, zaColor, gc->state.shadow.zaColor);
        REG_GROUP_END();      
      }

      if (NOTVALID(chromaKey)) {
        REG_GROUP_BEGIN(eChipFBI, chromaKey, 1, 0x01);
        REG_GROUP_SET(hw, chromaKey, gc->state.shadow.chromaKey);
        REG_GROUP_END();      
      }
      if (NOTVALID(chromaRange)) {
        REG_GROUP_BEGIN(eChipFBI, chromaRange, 1, 0x01);
        REG_GROUP_SET(hw, chromaRange, gc->state.shadow.chromaRange);
        REG_GROUP_END();      
    }
    }
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

  /* Now handle fogMode.  Sigh. */
  if (NOTVALID(fogMode)) {
     
    _grFogMode(LOADARG(grFogMode, mode));
    if(gc->grPixelSample == 2) {
      if(gc->grSamplesPerChip == 2) {
        /* All chips get the same fogMode value */
        FxU32 fogMode = gc->state.shadow.fogMode;
        fogMode &= ~(SST_DITHER_ROTATE | SST_DITHER_ROTATE_BLEND | 
                     SST_DITHER_ROTATE_AA | SST_DITHER_ROTATE_BLEND_AA) ;
        fogMode |= (2 << SST_DITHER_ROTATE_AA_SHIFT) |
                   (2 << SST_DITHER_ROTATE_BLEND_AA_SHIFT) ;
        REG_GROUP_BEGIN(eChipFBI, fogMode, 1, 0x01);
        REG_GROUP_SET(hw, fogMode, fogMode);
        REG_GROUP_END();
      } else {
        /* Even chips use one rotation, odd chips use the other one */
        FxU32 chipIndex, fogMode;
        fogMode = gc->state.shadow.fogMode;
        for(chipIndex = 0; chipIndex < gc->chipCount; chipIndex++) {
            if(chipIndex & 1) { /* Sample 1 */
            fogMode &= ~(SST_DITHER_ROTATE | SST_DITHER_ROTATE_BLEND | 
                        SST_DITHER_ROTATE_AA | SST_DITHER_ROTATE_BLEND_AA) ;
            fogMode |= (2 << SST_DITHER_ROTATE_SHIFT) |
                        (2 << SST_DITHER_ROTATE_BLEND_SHIFT) |
                        (2 << SST_DITHER_ROTATE_AA_SHIFT) |
                        (2 << SST_DITHER_ROTATE_BLEND_AA_SHIFT) ;
            } else { /* Sample 0 */
            fogMode &= ~(SST_DITHER_ROTATE | SST_DITHER_ROTATE_BLEND | 
                        SST_DITHER_ROTATE_AA | SST_DITHER_ROTATE_BLEND_AA) ;
            fogMode |= (0 << SST_DITHER_ROTATE_SHIFT) |
                        (0 << SST_DITHER_ROTATE_BLEND_SHIFT) |
                        (0 << SST_DITHER_ROTATE_AA_SHIFT) |
                        (0 << SST_DITHER_ROTATE_BLEND_AA_SHIFT) ;
            }
            _grChipMask( 1L << chipIndex );
            REG_GROUP_BEGIN(eChipFBI, fogMode, 1, 0x01);
            REG_GROUP_SET(hw, fogMode, fogMode);
            REG_GROUP_END();
        }
        _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
      }
    } else if(gc->grPixelSample >= 4) {
      FxU32 chipIndex, fogMode;
      fogMode = gc->state.shadow.fogMode;
      for(chipIndex = 0; chipIndex < gc->chipCount; chipIndex++) {
#if 1 /* KoolSmoky */
        if(gc->grSamplesPerChip == 2) {
          if(chipIndex & 1) { /* Samples 2 and 3 */
            fogMode &= ~(SST_DITHER_ROTATE | SST_DITHER_ROTATE_BLEND | 
                         SST_DITHER_ROTATE_AA | SST_DITHER_ROTATE_BLEND_AA) ;
            fogMode |= (2 << SST_DITHER_ROTATE_SHIFT) |
                       (2 << SST_DITHER_ROTATE_BLEND_SHIFT) |
                       (3 << SST_DITHER_ROTATE_AA_SHIFT) |
                       (3 << SST_DITHER_ROTATE_BLEND_AA_SHIFT) ;
          } else { /* Samples 0 and 1 */
            fogMode &= ~(SST_DITHER_ROTATE | SST_DITHER_ROTATE_BLEND | 
                         SST_DITHER_ROTATE_AA | SST_DITHER_ROTATE_BLEND_AA) ;
            fogMode |= (0 << SST_DITHER_ROTATE_SHIFT) |
                       (0 << SST_DITHER_ROTATE_BLEND_SHIFT) |
                       (1 << SST_DITHER_ROTATE_AA_SHIFT) |
                       (1 << SST_DITHER_ROTATE_BLEND_AA_SHIFT) ;
          }
        } else {
          int rotNum = chipIndex & 3;

          fogMode &= ~(SST_DITHER_ROTATE | SST_DITHER_ROTATE_BLEND |
                       SST_DITHER_ROTATE_AA | SST_DITHER_ROTATE_BLEND_AA) ;

          fogMode |= (rotNum << SST_DITHER_ROTATE_SHIFT) |
                     (rotNum << SST_DITHER_ROTATE_BLEND_SHIFT);
        }
#else /* Colourless */
        /* Chip0Buf0=0, Chip0Buf1=2, Chip1Buf0=1, Chip1Buf1=3 */
        /* Chip2Buf0=2, Chip2Buf1=0, Chip3Buf0=3, Chip3Buf1=1 */
        fogMode &= ~(SST_DITHER_ROTATE | SST_DITHER_ROTATE_BLEND | 
                     SST_DITHER_ROTATE_AA | SST_DITHER_ROTATE_BLEND_AA) ;
        fogMode |= ((chipIndex&3) << SST_DITHER_ROTATE_SHIFT) |
                   ((chipIndex&3) << SST_DITHER_ROTATE_BLEND_SHIFT) |
                   (((chipIndex+2)&3) << SST_DITHER_ROTATE_AA_SHIFT) |
                   (((chipIndex+2)&3) << SST_DITHER_ROTATE_BLEND_AA_SHIFT) ;
#endif
        _grChipMask( 1L << chipIndex );
        REG_GROUP_BEGIN(eChipFBI, fogMode, 1, 0x01);
        REG_GROUP_SET(hw, fogMode, fogMode);
        REG_GROUP_END();      
      }      
      _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    } else {
      REG_GROUP_BEGIN(eChipFBI, fogMode, 1, 0x01);
      REG_GROUP_SET(hw, fogMode, gc->state.shadow.fogMode);
      REG_GROUP_END();
    }
  }

  /* In this case, textureCombine is a "virtual" register that means something important
     changed that may affect TMU configuration.  Since that code is pretty complicated, it
     has it's own routine up above. */
  if(NOTVALID(tmuConfig)) {
    _grValidateTMUState();
  }

#if 0
  if (gc->mode2ppc)
    _grTex2ppc(GR_TMU0, FXTRUE);
#endif
  
  if (gc->state.invalid) {
    _grUpdateParamIndex();
    _grChipMask(gc->chipmask);
  }

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
  GR_BEGIN_NOFIFOCHECK_NORET("grEnable\n", 85);

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
  case GR_TEXTURE_UMA_EXT:
    gc->state.grEnableArgs.texture_uma_mode = GR_MODE_ENABLE;
    if ((gc->num_tmu == 2) && (gc->open)) {
      gc->tmuMemInfo[0].tramOffset = gc->tmuMemInfo[1].tramOffset = gc->bInfo->tramOffset;
      gc->tmuMemInfo[0].tramSize = gc->tmuMemInfo[1].tramSize = gc->bInfo->tramSize;
      gc->tmu_state[0].total_mem = gc->tmu_state[1].total_mem = gc->tmuMemInfo[0].tramSize;
    }
    break;
  case GR_COMBINEEXT_MODE:
    gc->state.grEnableArgs.combine_ext_mode = GR_MODE_ENABLE;
    break;
  case GR_STENCIL_MODE_EXT:
    gc->state.grEnableArgs.stencil_mode = GR_MODE_ENABLE;
    INVALIDATE(stencilMode);
    break;
#ifdef FX_GLIDE_NAPALM
  case GR_AA_MULTI_SAMPLE:
    {
      if (gc->state.grEnableArgs.aaMultisampleDisableCount) {
        gc->state.grEnableArgs.aaMultisampleDisableCount--;
      }
      
      if (!gc->state.grEnableArgs.aaMultisampleDisableCount) {
        _grAAOffsetValue(_GlideRoot.environment.aaXOffset[gc->sampleOffsetIndex],
                         _GlideRoot.environment.aaYOffset[gc->sampleOffsetIndex],
                         0, gc->chipCount - 1, FXTRUE, gc->enableSecondaryBuffer) ;
      }
    }
    break;
  case GR_OPENGL_MODE_EXT:
    {
#ifdef WIN32
      /* EnableOpenGL - Win_Mode.c 
      ** Allow minihwc to know about OpenGL 
      */
      /*void EnableOpenGL ();
      EnableOpenGL();*/
     /* setup env to determine whether we are an OGL app */
     _GlideRoot.environment.is_opengl=FXTRUE;
#endif
      /* Set up some stuff that's affected by OpenGL's behaviour. */
      _GlideRoot.environment.sliBandHeightForce = FXTRUE;
    }
    break;
#endif
#ifdef MULTIRENDERING
  case GR_MULTIRENDERING_EXT:
    {
      /* EnableMultiRendering - Win_Mode.c
      ** Allow minihwc to know about MultiRendering mode
      */
      void EnableMultiRendering();
      EnableMultiRendering();
      /* disable splash screen */
      _GlideRoot.environment.noSplash = FXTRUE;
    }
    break;
#endif
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
  case GR_TEXTURE_UMA_EXT:
    gc->state.grEnableArgs.texture_uma_mode = GR_MODE_DISABLE;
    if ((gc->num_tmu == 2) && (gc->open)) {
      gc->tmuMemInfo[0].tramOffset = gc->bInfo->tramOffset;
      gc->tmuMemInfo[0].tramSize = gc->bInfo->tramSize >> 1;
      gc->tmu_state[0].total_mem = gc->tmuMemInfo[0].tramSize;
      
      gc->tmuMemInfo[1].tramOffset = gc->tmuMemInfo[0].tramOffset + gc->tmuMemInfo[0].tramSize;
      gc->tmuMemInfo[1].tramSize   = (gc->bInfo->tramSize >> 1);
      gc->tmu_state[1].total_mem   = gc->tmuMemInfo[1].tramSize;
    }
    break;
  case GR_COMBINEEXT_MODE:
    gc->state.grEnableArgs.combine_ext_mode = GR_MODE_DISABLE;
    break;
  case GR_STENCIL_MODE_EXT:
    gc->state.grEnableArgs.stencil_mode = GR_MODE_DISABLE;
    INVALIDATE(stencilMode);
    break;
  case GR_OPENGL_MODE_EXT:
    _GlideRoot.environment.is_opengl=FXFALSE;
    break;
#ifdef FX_GLIDE_NAPALM
  case GR_AA_MULTI_SAMPLE:
    {
      if (!gc->state.grEnableArgs.aaMultisampleDisableCount) {
        _grAAOffsetValue(_GlideRoot.environment.aaXOffset[0],
                         _GlideRoot.environment.aaYOffset[0],
                         0, gc->chipCount - 1, FXTRUE, gc->enableSecondaryBuffer) ;
      }
      gc->state.grEnableArgs.aaMultisampleDisableCount++;
    }
    break;
#endif
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
  gc->triSetupProc = CUR_TRI_PROC(FXTRUE, (gc->state.cull_mode != GR_CULL_DISABLE));

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

#ifdef DRI_BUILD
void
_grInvalidateAll()
{
  GR_DCL_GC;

  grGlideSetState(&gc->state);
}
#endif  /* defined(DRI_BUILD) */

#endif /* GLIDE3 */
