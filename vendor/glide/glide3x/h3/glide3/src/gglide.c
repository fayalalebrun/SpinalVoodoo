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
** Revision 1.1.2.4  2005/06/09 18:32:28  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.2.3  2005/05/25 08:56:23  jwrdegoede
** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
**
** Revision 1.1.2.2  2004/02/16 07:42:16  dborca
** grSetNumPendingBuffers visible with grGetProcAddress
**
** Revision 1.1.2.1  2003/11/07 13:38:38  dborca
** unite the clans
**
** Revision 1.5  2000/11/24 18:36:48  alanh
** Add new grStippleMode and grStipplePattern functions for both Voodoo3 and
** Voodoo5 hardware.
**
** Revision 1.4  2000/11/16 20:25:38  alanh
** fix a typo
**
** Revision 1.3  2000/11/16 18:58:27  alanh
** add linux specific grRenderBuffer to fix problems switching between FRONT and
** BACK buffers.
**
** Revision 1.2  2000/10/27 07:54:51  alanh
** fix readPtr0/1 -> depth0/1 check
**
** Revision 1.1.1.1  1999/11/24 21:45:01  joseph
** Initial checkin for SourceForge
**
** 
** 6     5/12/99 10:39p Stb_gkincade
** Adding user support for turning on/off dither substraction
** 
** 5     4/29/99 10:24a Atai
** fixed grDitherMode
** 
** 4     4/06/99 3:54p Dow
** Alt tab again.
** 
** 87    3/24/99 6:17p Peter
** streamlined (made more dangerouse) state validation
** 
** 86    3/10/99 10:42a Peter
** bump-n-grind workaround for katmai until the bug is better
** characterized
** 
** 85    3/05/99 10:31p Peter
** coalesced texture state change nop into validate state
** 
** 84    3/02/99 2:03p Peter
** removed no_check variant that led me astray
** 
** 83    2/18/99 3:27p Kcd
** Fixed direct register access, removed surface extension call for
** non-Win32.
** 
** 82    2/11/99 1:38p Atai
** sync buffer swap pending code, the right way.
** 
** 81    2/09/99 5:08p Atai
** fixed fog ext win coords that I broke in my previous check-in
** 
** 80    2/02/99 4:39p Peter
** cleaned up wax commands
** 
** 79    1/28/99 3:04p Atai
** fixed clip coord fog extension (c and asm version)
** 
** 78    1/25/99 6:34p Peter
** _grDepthBufferFunction should not write to hw as an intermediary step
** 
** 77    1/15/99 10:52a Peter
** cleanup lazy evaluation of fbzMode for grDepthMask and grColorMask
** 
** 76    12/21/98 3:01p Peter
** cleaned up shutdow semantics
** 
** 75    12/09/98 6:25p Atai
** grTexCombine did the right thing for the un-used TMU.  Initialize the
** 2nd TMU value to take care of "set FX_GLIDE_NUM_TMU=1"
** 
** 74    12/09/98 5:10p Atai
** set MAXLOD = MINLOD = 8 in _grUpdateParamIndex if ST1 is not used
** 
** 73    12/09/98 3:59p Peter
** test20 state validation bug
** 
** 72    12/07/98 4:42p Atai
** enable w is good enough for qx/q
** 
** 71    12/05/98 2:51p Dow
** Fixed swapinterval
** 
** 70    12/03/98 11:26p Dow
** Code 'cleanup' heheh
** 
** 69    12/02/98 11:30a Dow
** Fixed effed-up size checking in gglide.c
** 
** 68    11/30/98 6:57p Peter
** windowed texture muckage
** 
** 67    11/18/98 6:34p Dow
** Fixed clear problem on v3/banshee
** 
** 66    11/17/98 4:27p Atai
** fixed gc->triSetupProc for clip coords grDrawTriangle
** 
** 65    11/15/98 3:21a Atai
** first attempt to make 2 tmus work in H4 glide3x full screen mode, just
** in time check-in for comdex demo. warning: the code is not completed
** yet.
** 
** 64    11/02/98 5:34p Peter
** tls per thread for fullscreen contexts
** 
** 63    10/30/98 3:45p Dow
** Fixed Tiled/Linear color/aux bug
** 
** 62    10/08/98 10:29a Dow
** Fixes triple buffering
** 
** 61    9/30/98 12:59p Atai
** use texchromakey and texchromarange shadow state
** 
** 60    9/08/98 7:19p Atai
** fix debug info. added underline for internal routines and some \n
** 
** 59    9/02/98 3:34p Atai
** fixed texturing transition bug
** 
** 58    8/31/98 7:11p Atai
** fix clip coord tmu q param bug. send 1/w or Qn/w
** 
** 57    8/30/98 1:34p Dow
** State & other optimizations
** 
** 56    8/29/98 10:04p Peter
** sdram clear fixes
** 
** 55    8/29/98 8:12p Dow
** Clip optimization
** 
** 54    8/29/98 2:29p Peter
** call grSstWinClose w/ correct context parameter
** 
** 53    8/27/98 9:54p Peter
** clear aux buffer w/ rgb not rbg
** 
** 52    8/27/98 5:52p Jdt
** removed maxx maxy check from grClipwindow
** 
** 51    8/27/98 1:55p Peter
** use converted color on sdram boards
** 
** 50    8/26/98 3:01p Jdt
** tsuIndices may not be used to index p_str < debugging bug only >
** 
** 49    8/18/98 1:44p Atai
** fixed "w param is turned off in clip space"
** 
** 48    8/14/98 1:56p Atai
** fix fog coord bug
** 
** 47    8/14/98 10:21a Dow
** Fixed fog for B silicon
** 
** 46    8/06/98 8:04p Dow
** Fixed SDRAM for full-screen
** 
** 45    8/06/98 7:50p Dow
** Fixed SDRAM stuff
** 
** 43    8/05/98 11:46p Atai
** 1. grBufferSwap argument FxU32
** 2. grGet return FxU32
** 
** 42    8/03/98 6:40a Jdt
** moved stats, curvertexsize, curtrisize into gc from global
** 
** 41    7/18/98 1:45p Jdt
** Removed TACO_MEMORY_FIFO_HACK
** 
** 40    7/18/98 12:27a Jdt
** Added shadowing of fog tables.  Chagnes to reflect new shadow register
** structure.
** 
** 39    7/17/98 2:04p Jdt
** Remove pointless SST_DRAWBUFFER bits.
** 
** 38    7/16/98 8:18a Jdt
** Removed conditional screen clear for A1
** 
** fxcmd.h
** 
** 37    7/02/98 1:55p Atai
** grDepthBiasLevel argument is FxI32
** 
** 36    7/01/98 12:40p Jdt
** Protected hacks for Glide/Win ( FX_TACO_MEMORY_FIFO_HACK )
** 
** 35    6/21/98 11:56a Atai
** fixed fogcoord paramindex
** 
** 34    6/10/98 12:53p Atai
** replace grSstControl with grEnable/grDisable(GR_PASSTHRU)
** 
** 33    6/09/98 11:59a Atai
** 1. update glide header
** 2. fix cull mode
** 3. fix tri stats
** 
** 32    5/29/98 6:39p Atai
** fix chromarange
** 
** 31    5/29/98 11:45a Atai
** 1.added _EXT for extension #defines. 
** 2. change GR_TEXBASE_* values
** 3. Remove GR_TEXCHROMA_ENABLE_SUBSTITUTE_RGB
** 
** 30    5/28/98 2:07p Peter
** banshee merge
** 
** 29    5/27/98 9:52a Peter
** grBufferClear is constrained by clipping
** 
** 28    5/21/98 7:15p Atai
** fix q0 and q1 for clip coords
** 
** 27    5/19/98 2:00p Atai
** use GR_PARAM_Q for fbi w
** 
** 26    5/15/98 4:02p Atai
** fogCoord and texchroma extension for Banshee
** 
** 25    4/30/98 5:01p Peter
** first pass glide3 merge
** 
** 23    4/22/98 4:57p Peter
** glide2x merge
** 
** 22    4/21/98 1:34p Atai
** make 32 bit clean
** 
** 21    3/23/98 6:34p Atai
** Fixed texture state validation bug in grColorCombine
** 
** 20    3/21/98 11:31a Atai
** added GR_TRIANGLE_STRIP_CONTINUE and GR_TRIANGLE_FAN_CONTINUE
** 
** 19    2/20/98 2:17p Peter
** shutting down hw should clear hwInit and open
** 
** 18    2/17/98 5:07p Atai
** fixed packed a
** 
** 17    2/17/98 12:40p Peter
** fog table fix
** 
** 16    2/12/98 4:08p Atai
** casting value
** 
** 15    2/12/98 3:40p Peter
** single buffering for opengl
** 
** 14    2/12/98 3:05p Peter
** fixed naming change for glid3
** 
** 13    2/11/98 7:31p Peter
** blit clear vs grRenderBuffer
** 
** 12    2/05/98 6:52p Atai
** fixed pargb vertex size
** 
** 11    2/05/98 6:19p Atai
** lazy evaluate for grVertexLayout
** 
** 10    1/30/98 4:21p Peter
** fixed old glide-ism
** 
** 9     1/30/98 4:18p Peter
** sli/y-origin blit clear
** 
** 8     1/30/98 1:19p Atai
** fixed chromarange
** 
** 7     1/28/98 2:20p Atai
** fixed lfb state validation
** 
** 6     1/23/98 3:07p Peter
** uswc nightmare
** 
** 5     1/19/98 1:38p Atai
** fixed _grDrawLineStrip zero length lline
** 
** 4     1/19/98 11:03a Atai
** remove assignment before validate the state
** 
** 3     1/18/98 12:03p Atai
** sync to rev 17 spec
 * 
 * 2     1/16/98 6:44p Atai
 * fixed for glide 3 build
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
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
 * fixed macro effage
 * 
 * 131   1/08/98 7:09p Peter
 * real hw stuff modulo makefile change
 * 
 * 130   1/08/98 4:58p Atai
 * tex table broadcast, grVertexLayout enable/disable, stq, and some
 * defines
 * 
 * 129   1/07/98 5:22p Atai
 * fixed grGet compiler error
 * 
 * 128   1/07/98 10:22a Peter
 * merged John's ooz fix
 * 
 * 127   1/06/98 3:53p Atai
 * remove grHint, modify grLfbWriteRegion and grGet
 * 
 * 126   1/05/98 6:06p Atai
 * glide extension stuff
 * 
 * 125   12/22/97 12:40p Peter
 * added new grColorCombine factor for OpenGL
 * 
 * 124   12/18/97 2:13p Peter
 * fogTable cataclysm
 * 
 * 123   12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 122   12/17/97 4:06p Atai
 * added grChromaRange(), grGammaCorrecionRGB(), grRest(), and grGet()
 * functions
 * 
 * 120   12/16/97 11:38a Atai
 * added grChromaRange()
 * 
 * 118   12/08/97 10:49a Atai
 * rename some state variables
 * 
 * 117   12/03/97 11:35a Peter
 * reset for swapping
 * 
 * 116   11/21/97 6:05p Atai
 * use one datalist (tsuDataList) in glide3
 * 
 * 115   11/21/97 3:20p Peter
 * direct writes tsu registers
 * 
 * 114   11/18/97 6:11p Peter
 * fixed glide3 muckage
 * 
 * 113   11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 112   11/18/97 3:27p Atai
 * update vData 
 * optimize state monster
 * 
 * 111   11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 110   11/16/97 2:20p Peter
 * cleanup
 * 
 * 109   11/15/97 7:43p Peter
 * more comdex silliness
 * 
 * 108   11/14/97 5:02p Peter
 * more comdex stuff
 * 
 * 107   11/14/97 12:09a Peter
 * comdex thing and some other stuff
 * 
 * 106   11/13/97 4:39p Atai
 * enable _grUpdateParamIndex for grGlideSetState
 * 
 * 105   11/12/97 9:54p Peter
 * fixed all the muckage from new config
 * 
 * 104   11/12/97 2:27p Peter
 * 
 * 103   11/12/97 11:16a Peter
 * cleaned up assertions
 * 
 * 102   11/06/97 3:38p Dow
 * More banshee stuff
 * 
 * 101   11/04/97 5:59p Peter
 * more of the same
 * 
 * 100   11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 99    11/01/97 10:01a Peter
 * tri dispatch stuff
 * 
**
*/

#include <string.h>
#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"
#include "fxinline.h"

#if DRI_BUILD
#include <lindri.h>
#endif

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
#define FN_NAME "_grAlphaBlendFunction"
  FxU32 alphamode;
  GR_BEGIN_NOFIFOCHECK("_grAlphaBlendFunction", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d,%d,%d,%d)\n",
                 rgb_sf, rgb_df, alpha_sf, alpha_df);

  alphamode = gc->state.shadow.alphaMode;
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

  gc->state.shadow.alphaMode = alphamode;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, alphaMode, alphamode);
  GR_CHECK_SIZE();
#endif
#undef FN_NAME
} /* grAlphaBlendFunction */

/*---------------------------------------------------------------------------
** grAlphaCombine
*/

GR_STATE_ENTRY(grAlphaCombine, void, 
               (GrCombineFunction_t function, GrCombineFactor_t factor, 
                GrCombineLocal_t local, GrCombineOther_t other, FxBool invert))
{
#define FN_NAME "_grAlphaCombine"
  FxU32 fbzColorPath;

  GR_BEGIN_NOFIFOCHECK("_grAlphaCombine",85);
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

  fbzColorPath = gc->state.shadow.fbzColorPath;
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
  gc->state.shadow.fbzColorPath = fbzColorPath;

  GR_END();
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
#define FN_NAME "_grAlphaControlsITRGBLighting"
  FxU32 fbzColorPath;
  GR_BEGIN_NOFIFOCHECK("_grAlphaControlsITRGBLighting", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", enable);

  fbzColorPath = gc->state.shadow.fbzColorPath;
  if (enable) {
    fbzColorPath |= SST_LOCALSELECT_OVERRIDE_WITH_ATEX;
  } else {
    fbzColorPath &= ~SST_LOCALSELECT_OVERRIDE_WITH_ATEX;
  }

  gc->state.shadow.fbzColorPath = fbzColorPath;
#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzColorPath, fbzColorPath);
  GR_CHECK_SIZE();
#endif /* !GLIDE3 */
#undef FN_NAME
} /* grAlphaControlsITRGBLighting() */

/*---------------------------------------------------------------------------
** grAlphaTestFunction
*/

GR_STATE_ENTRY(grAlphaTestFunction, void, (GrCmpFnc_t fnc))
{
#define FN_NAME "_grAlphaTestFunction"
  FxU32 alphamode;
  GR_BEGIN_NOFIFOCHECK("_grAlphaTestFunction", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",fnc);

  alphamode = gc->state.shadow.alphaMode;
  alphamode &= ~(SST_ALPHAFUNC | SST_ENALPHAFUNC);
  if (fnc != GR_CMP_ALWAYS)
    alphamode |= ((fnc << SST_ALPHAFUNC_SHIFT) | SST_ENALPHAFUNC);

  gc->state.shadow.alphaMode = alphamode;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, alphaMode, alphamode);
  GR_CHECK_SIZE();
#endif /* !GLIDE3 */
#undef FN_NAME
} /* grAlphaTestFunction */

/*---------------------------------------------------------------------------
** grAlphaTestReferenceValue
*/

GR_STATE_ENTRY(grAlphaTestReferenceValue, void, (GrAlpha_t value))
{
#define FN_NAME "_grAlphaTestReferenceValue"
  FxU32 alphamode;
  GR_BEGIN_NOFIFOCHECK("_grAlphaTestReferenceValue", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",value);

  alphamode = gc->state.shadow.alphaMode;
  alphamode &= ~SST_ALPHAREF;
  alphamode |= (((FxU32) value) << SST_ALPHAREF_SHIFT);

  gc->state.shadow.alphaMode = alphamode;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, alphaMode,  alphamode);
  GR_CHECK_SIZE();
#endif /* !GLIDE3 */
#undef FN_NAME
} /* grAlphaTestReferenceValue */


#define ADDWAXMASK(mask, reg, base) mask |= (1 << ((offsetof(SstGRegs, reg) - offsetof(SstGRegs, base)) >> 2))

#if 0 /* unused */
static void
_grBufferClear2D(const FxU32 buffOffset, 
                 const FxU32 clipLeft, const FxU32 clipTop,
                 const FxU32 width, const FxU32 height, 
                 const FxU32 color) 
{
#define FN_NAME "_grBufferClear2D"
  FxU32                   /* Registers we use */
    clip0min, clip0max, dstBaseAddr, dstFormat, commandEx, colorFore, command;
  FxU32
    regMask = 0L;

  GR_BEGIN_NOFIFOCHECK("_grBufferClear2D", 86);

  ADDWAXMASK(regMask, clip0min, clip0min);
  ADDWAXMASK(regMask, clip0max, clip0min);
  ADDWAXMASK(regMask, dstBaseAddr, clip0min);        
  ADDWAXMASK(regMask, dstFormat, clip0min);
  ADDWAXMASK(regMask, commandEx, clip0min);

  REG_GROUP_BEGIN_WAX(clip0min, 5, regMask);

  /* XXX USE CONSTANTS! */

  /* Setup clipping rectangle based on top left origin */
  clip0min = ((clipTop << 16) | clipLeft);
  clip0max = ((clipLeft + width) | ((clipTop + height) << 16));
  REG_GROUP_SET_WAX(gc->gRegs, clip0min, clip0min);
  REG_GROUP_SET_WAX(gc->gRegs, clip0max, clip0max);

  /* Set buffer address and or in the tiled bit */
  dstBaseAddr = buffOffset | (1 << 31);
  REG_GROUP_SET_WAX(gc->gRegs, dstBaseAddr, dstBaseAddr);
  
  /* Build up dstFormat (Should this be done globally?) */
  dstFormat = gc->strideInTiles;
  dstFormat |= (0x3 << 16); 
  REG_GROUP_SET_WAX(gc->gRegs, dstFormat, dstFormat);
  
  /* Setup Command extra */
  commandEx = 0;
  REG_GROUP_SET_WAX(gc->gRegs, commandEx, commandEx);
        
  REG_GROUP_END();


  regMask = 0L;

  ADDWAXMASK(regMask, colorFore, colorFore);
  ADDWAXMASK(regMask, dstSize, colorFore);
  ADDWAXMASK(regMask, dstXY, colorFore);
  ADDWAXMASK(regMask, command, colorFore);

  REG_GROUP_BEGIN_WAX(colorFore, 4, regMask);

  /* Setup colorFore */
  colorFore = color;
  REG_GROUP_SET_WAX(gc->gRegs, colorFore, colorFore);

  /* Setup dstSize - height/width */
  REG_GROUP_SET_WAX(gc->gRegs, dstSize, clip0max);

  /* Setup dstXY - starting coordinate */
  REG_GROUP_SET_WAX(gc->gRegs, dstXY, clip0min);

  /* Setup command */
  command = SSTG_RECTFILL;    /* rectangle fill */
  command |= SSTG_GO;         /* Initiate immediately */
  command |= 0xcc000000;      /* Src ROP */

  REG_GROUP_SET_WAX(gc->gRegs, command, command);
  REG_GROUP_END();
#undef FN_NAME
} /* _grBufferClear2D */
#endif /* unused */

/*---------------------------------------------------------------------------
** grBufferClear
*/

GR_ENTRY(grBufferClear, void, (GrColor_t color, GrAlpha_t alpha, FxU32 depth))
{
#define FN_NAME "grBufferClear"
  GR_BEGIN_NOFIFOCHECK("grBufferClear", 86);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x,0x%x,0x%x)\n", color, alpha, depth);

  /* validate the state */
  if (gc->state.invalid)
    _grValidateState();

  {
    const GrColor_t oldc1 = gc->state.shadow.color1;
    const FxU32 oldzacolor = gc->state.shadow.zaColor;
    const FxU32 fbzMode = gc->state.shadow.fbzMode;
    FxU32 zacolor = oldzacolor;

    FxBool doneP = FXFALSE;
    FxBool doColorP, doAuxP = FXFALSE;

    /* validate the state */
    /* Setup source registers */
    doColorP = ((fbzMode & SST_RGBWRMASK) != 0);
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

    if (!doneP) {
      if (!gc->bInfo->sdRAM && !gc->windowed) {
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
      } else {
        const FxU32 colorBufMode = ((fbzMode & ~(SST_ZAWRMASK | SST_ENDEPTHBUFFER)) |
                                    SST_RGBWRMASK |
                                    SST_ENRECTCLIP);

        /* Turn off writes to the aux buffer */
        REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 1, 1);
        REG_GROUP_SET(hw, fbzMode, colorBufMode);
        REG_GROUP_END();        

        if (doColorP) {
          /* Clear Color Buffer */
          REG_GROUP_BEGIN(BROADCAST_ID, c1, 1, 0x1);
          REG_GROUP_SET(hw, c1, color);
          REG_GROUP_END();
            
          /* Execute the FASTFILL command */
          REG_GROUP_BEGIN(BROADCAST_ID, fastfillCMD, 1, 1);
          REG_GROUP_SET(hw, fastfillCMD, 1);
          REG_GROUP_END();
        }

        if (doAuxP) {
          FxU32
            red, green, blue, convertedDepth;
#define GETRED(a) ((a >> 11) & 0x1f)
#define GETGREEN(a) ((a >> 5) & 0x3f)
#define GETBLUE(a)  (a & 0x1f)

          /* Convert 16-bit depth to 24-bit, ready for truncation:
             20        10        0  
             321098765432109876543210
             RRRRR000GGGGGG00BBBBB000      
             
             So, we get the 565 out of 16-bit depth, then operate
             like this:
             
             RED'   = red << 3
             GREEN' = green << 2
             BLUE'  = blue << 3
             
             This way, when the fastFill hardware truncates, we
             still have all the bits we were given.
             
             We then simply recombin RED', BLUE', and GREEN' to make
             a 24-bit color value.
             
             capisce?
             
             */

          red = GETRED(depth) << 3;
          green = GETGREEN(depth) << 2;
          blue = GETBLUE(depth) << 3;

          convertedDepth = ((red << 16) | (green << 8) | blue);

          /* Clear Aux Buffer */
          REG_GROUP_BEGIN(BROADCAST_ID, c1, 1, 0x1);
          REG_GROUP_SET(hw, c1, convertedDepth);
          REG_GROUP_END();
          
          REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3);
          REG_GROUP_SET(hw, colBufferAddr,gc->buffers[gc->grColBuf]);
#if DRI_BUILD
          REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer)? (FxU32)driInfo.stride :
			gc->state.shadow.auxBufferStride);
#else
          REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.auxBufferStride);
#endif
          REG_GROUP_END();
          
          REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 2, 0x21);
          {
            /* Write the depth buffer as if it were a color buffer,
             * but w/ actual color buffer features
             * (dithering/chroma/stipple) cleared so that the
             * converted depth value does not get dorked along the
             * way down the eerie pathways of banshee.
             */
            REG_GROUP_SET(hw, fbzMode, colorBufMode & ~(SST_ENCHROMAKEY |
                                                        SST_ENSTIPPLE |
                                                        SST_ENDITHER));

            /* Execute the FASTFILL command */
            REG_GROUP_SET(hw, fastfillCMD, 1);
          }
          REG_GROUP_END();            
        }

        /* Restore trashed things */
        REG_GROUP_BEGIN(BROADCAST_ID, c1, 1, 0x1);
        REG_GROUP_SET(hw, c1, oldc1);
        REG_GROUP_END();

        REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3);
        REG_GROUP_SET(hw, colBufferAddr, gc->buffers[gc->windowed ? 0 : gc->curBuffer]);

#if DRI_BUILD
        REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride :
		      gc->state.shadow.colBufferStride);
#else
        REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride);
#endif
        REG_GROUP_END();

        REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 1, 1);
        REG_GROUP_SET(hw, fbzMode, fbzMode);
        REG_GROUP_END();        
      }
    }
  }
#undef FN_NAME
} /* grBufferClear */

#if 0
/*---------------------------------------------------------------------------
** grBufferClear
*/

GR_ENTRY(grBufferClear, void, (GrColor_t color, GrAlpha_t alpha, FxU32 depth))
{
#define FN_NAME "grBufferClear"
  GR_BEGIN_NOFIFOCHECK("grBufferClear", 86);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x,0x%x,0x%x)\n", color, alpha, depth);

  /* validate the state */
  if (gc->state.invalid)
    _grValidateState();

  {
    const GrColor_t 
      oldc1 = gc->state.shadow.color1;
    const FxU32 
      oldzacolor = gc->state.shadow.zaColor;
    const FxU32 
      fbzMode = gc->state.shadow.fbzMode;
    FxU32 
      zacolor = oldzacolor;
    FxBool 
      doColorP = ((fbzMode & SST_RGBWRMASK) != 0), 
      doneP = FXFALSE,
      doAuxP = FXFALSE;

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

    /* A2 and above have functioning fastfill */
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
  }
#undef FN_NAME
} /* grBufferClear */
#endif

#if !DRI_BUILD
/*---------------------------------------------------------------------------
** grBufferSwap
**
** NOTE:        This routine should be COMPLETELY device-independant,
**              but it isn't right now because we need to move all the
**              code for the splash screen into the init library.
*/

GR_ENTRY(grBufferSwap, void, (FxU32 swapInterval))
{
  static int hack=0;
#define FN_NAME "grBufferSwap"
  GR_BEGIN_NOFIFOCHECK(FN_NAME,86);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",swapInterval);

#ifdef GLIDE_PLUG
  /* optionally display the 3Dfx powerfield logo overlay */
  if (_GlideRoot.environment.shamelessPlug) _grShamelessPlug();
#endif

  /* check for environmental override */
  if (_GlideRoot.environment.swapInterval >= 0) {
    swapInterval = _GlideRoot.environment.swapInterval;
  }
  
  if (swapInterval) {
    if (swapInterval > 1) 
      swapInterval = ((swapInterval - 1) << 1) | 1; /* Format for hw */
  }
  while(_grBufferNumPending() > _GlideRoot.environment.swapPendingCount)
   ;

  /* Cycle the buffer indices */
  {
    const FxU32 numBufs = gc->state.num_buffers;
    FxU32* bufPtrs[3];
    FxU32 i;

    bufPtrs[0] = &gc->curBuffer;
    bufPtrs[1] = &gc->frontBuffer;
    bufPtrs[2] = &gc->backBuffer;

    for(i = 0; i < sizeof(bufPtrs) / sizeof(*bufPtrs); i++) {
      FxU32 curBufIndex = *bufPtrs[i] + 1;
      if (curBufIndex == numBufs) curBufIndex = 0;
      *bufPtrs[i] = curBufIndex;
    }
  }

#if USE_PACKET_FIFO
  {
    int i, j = -1;

    for ( i = 0; i < MAX_BUFF_PENDING && j == -1; i++) {
      if (gc->bufferSwaps[i] == 0xffffffff) {
        gc->bufferSwaps[i] =
          (unsigned long) gc->cmdTransportInfo.fifoPtr -
          (unsigned long) gc->cmdTransportInfo.fifoStart; 
        j = i;
      }
    }
    GR_ASSERT(j != -1);

    gc->swapsPending++;

  }
#endif

  REG_GROUP_BEGIN(BROADCAST_ID, leftOverlayBuf, 1, 0x1);
  REG_GROUP_SET(hw, leftOverlayBuf, gc->buffers[gc->frontBuffer]);
  REG_GROUP_END();

  /* Just 0x1 for mask is OK here since we're writing one register */
  REG_GROUP_BEGIN(BROADCAST_ID, swapbufferCMD, 1, 0x1); 
  REG_GROUP_SET(hw, swapbufferCMD, swapInterval);
  REG_GROUP_END();

#if defined( TACO_MEMORY_FIFO_HACK )
  _FifoFlush();
#endif

  REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
  REG_GROUP_SET(hw, colBufferAddr, gc->buffers[gc->curBuffer]);
  REG_GROUP_END();

#ifdef GLIDE_DEBUG
  {
    if ((FxI32)_GlideRoot.environment.snapshot > 0) {
      static char saveDBG[GDBG_MAX_LEVELS];
      int i;
      
      /* turn off tracing after frame 0 and the snapshot frame */
      if ((gc->stats.bufferSwaps == 0) || 
          (gc->stats.bufferSwaps == _GlideRoot.environment.snapshot + 3)) {
        GDBG_PRINTF(FN_NAME": FX_SNAPSHOT (%ld)\n", gc->stats.bufferSwaps);

        for (i = 1; i < GDBG_MAX_LEVELS; i++) {
          if (gc->stats.bufferSwaps == 0) saveDBG[i] = (char)GDBG_GET_DEBUGLEVEL(i);
          GDBG_SET_DEBUGLEVEL(i, 0);
        }
      }
      /* turn on tracing after the snapshot frame */
      if (gc->stats.bufferSwaps == _GlideRoot.environment.snapshot) {
        GDBG_PRINTF(FN_NAME": FX_SNAPSHOT (%ld)\n", gc->stats.bufferSwaps);

        for (i = 1; i < GDBG_MAX_LEVELS; i++) {
          GDBG_SET_DEBUGLEVEL(i, saveDBG[i]);
        }
      }
    }
  }
#endif /* GLIDE_DEBUG */

  gc->stats.bufferSwaps++;
  
  GR_END();
#undef FN_NAME  
} /* grBufferSwap */
#else
/*---------------------------------------------------------------------------
** grBufferSwap
**
** NOTE:        This routine should be COMPLETELY device-independant,
**              but it isn't right now because we need to move all the
**              code for the splash screen into the init library.
*/

GR_ENTRY(grDRIBufferSwap, void, (FxU32 swapInterval))
{
  FxU32 regMask;
  int cnt, x, y, w, h;

#define FN_NAME "grBufferSwap"
  GR_BEGIN_NOFIFOCHECK(FN_NAME,86);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",swapInterval);

#ifdef GLIDE_PLUG
  /* optionally display the 3Dfx powerfield logo overlay */
  if (_GlideRoot.environment.shamelessPlug) _grShamelessPlug();
#endif

  /* check for environmental override */
  if (_GlideRoot.environment.swapInterval >= 0) {
    swapInterval = _GlideRoot.environment.swapInterval;
  }

  if (swapInterval) {
    if (swapInterval > 1) 
      swapInterval = ((swapInterval - 1) << 1) | 1; /* Format for hw */
  }
  while(_grBufferNumPending() > _GlideRoot.environment.swapPendingCount)
   ;

#if USE_PACKET_FIFO
  {
    int i, j = -1;

    for ( i = 0; i < MAX_BUFF_PENDING && j == -1; i++) {
      if (gc->bufferSwaps[i] == 0xffffffff) {
        gc->bufferSwaps[i] =
          (unsigned long) gc->cmdTransportInfo.fifoPtr -
          (unsigned long) gc->cmdTransportInfo.fifoStart; 
        j = i;
      }
    }
    GR_ASSERT(j != -1);

    gc->swapsPending++;

  }
#endif

  /* Just 0x1 for mask is OK here since we're writing one register */
  REG_GROUP_BEGIN(BROADCAST_ID, swapbufferCMD, 1, 0x1); 
  REG_GROUP_SET(hw, swapbufferCMD, swapInterval);
  REG_GROUP_END();

  cnt=driInfo.numClip;
  if (cnt) {
    /* Copy */
    regMask=0;
    ADDWAXMASK(regMask, srcBaseAddr, srcBaseAddr);
    ADDWAXMASK(regMask, srcFormat, srcBaseAddr);
    REG_GROUP_BEGIN_WAX(srcBaseAddr, 2, regMask);
    REG_GROUP_SET_WAX(hw, srcBaseAddr, gc->buffers[1]|BIT(31));
    REG_GROUP_SET_WAX(hw, srcFormat, gc->strideInTiles | (3<<16));
    REG_GROUP_END();

    do {
      cnt--;
      x=driInfo.pClip[cnt].x1;
      y=driInfo.pClip[cnt].y1;
      w=driInfo.pClip[cnt].x2-x;
      h=driInfo.pClip[cnt].y2-y;
      regMask=0;
      ADDWAXMASK(regMask, srcXY, srcXY);
      ADDWAXMASK(regMask, dstSize, srcXY);
      ADDWAXMASK(regMask, dstXY, srcXY);
      ADDWAXMASK(regMask, command, srcXY);
      REG_GROUP_BEGIN_WAX(srcXY, 4, regMask);
      REG_GROUP_SET_WAX(hw, srcXY, x | ((driInfo.y+(y-driInfo.y))<<16));
      REG_GROUP_SET_WAX(hw, dstSize, (w&0x1FFF)|((h&0x1FFF)<<16));
      REG_GROUP_SET_WAX(hw, dstXY, (x&0x1FFF) | ((y&0x1FFF)<<16));
      REG_GROUP_SET_WAX(hw, command, (0xCCu<<24) | 0x1 | BIT(8));
      REG_GROUP_END();
    } while (cnt);

    /* Put things back as X expects them */
    regMask=0;
    ADDWAXMASK(regMask, srcBaseAddr, srcBaseAddr);
    ADDWAXMASK(regMask, srcFormat, srcBaseAddr);
    REG_GROUP_BEGIN_WAX(srcBaseAddr, 2, regMask);
    REG_GROUP_SET_WAX(hw, srcBaseAddr, gc->buffers[0]);
    REG_GROUP_SET_WAX(hw, srcFormat, driInfo.stride | (3<<16));
    REG_GROUP_END();
  }

#ifdef GLIDE_DEBUG
  {
    if ((FxI32)_GlideRoot.environment.snapshot > 0) {
      static char saveDBG[GDBG_MAX_LEVELS];
      int i;
      
      /* turn off tracing after frame 0 and the snapshot frame */
      if ((gc->stats.bufferSwaps == 0) || 
          (gc->stats.bufferSwaps == _GlideRoot.environment.snapshot + 3)) {
        GDBG_PRINTF(FN_NAME": FX_SNAPSHOT (%ld)\n", gc->stats.bufferSwaps);

        for (i = 1; i < GDBG_MAX_LEVELS; i++) {
          if (gc->stats.bufferSwaps == 0) saveDBG[i] = (char)GDBG_GET_DEBUGLEVEL(i);
          GDBG_SET_DEBUGLEVEL(i, 0);
        }
      }
      /* turn on tracing after the snapshot frame */
      if (gc->stats.bufferSwaps == _GlideRoot.environment.snapshot) {
        GDBG_PRINTF(FN_NAME": FX_SNAPSHOT (%ld)\n", gc->stats.bufferSwaps);

        for (i = 1; i < GDBG_MAX_LEVELS; i++) {
          GDBG_SET_DEBUGLEVEL(i, saveDBG[i]);
        }
      }
    }
  }
#endif /* GLIDE_DEBUG */

  gc->stats.bufferSwaps++;
  
  GR_END();
#undef FN_NAME  
} /* grBufferSwap */

void grBufferSwap(FxU32 i) {
  grDRIBufferSwap(i);
}

#endif

/*---------------------------------------------------------------------------
** grBufferNumPending
*/

int FX_CSTYLE
_grBufferNumPending(void)
{
#if USE_PACKET_FIFO 
  volatile FxU32
    dummy,
    depth0, depth1,
    readPtr0, readPtr1, readPtr;
  volatile int
    i;
  int
    pend;                       /* Num Swaps pending */

  GR_DCL_GC;

  /* If we're not using hole counting then we need to make sure that
   * any queued commands are actually flushed before checking the fifo
   * ptr's location.
   */
  if (!gc->cmdTransportInfo.autoBump)
    GR_BUMP_N_GRIND;

  /* HACK HACK HACK */
  do {
    readPtr0 = GET(gc->cRegs->cmdFifo0.readPtrL) - gc->cmdTransportInfo.fifoOffset;
    dummy = GET(gc->ioRegs->status);
    readPtr1 = GET(gc->cRegs->cmdFifo0.readPtrL) - gc->cmdTransportInfo.fifoOffset;
  } while (readPtr0 != readPtr1);

  readPtr = readPtr1;

  if (readPtr == gc->lastSwapCheck) {
    do {
      depth0 = GET(gc->cRegs->cmdFifo0.depth);
      depth1 = GET(gc->cRegs->cmdFifo0.depth);
    } while (depth0 != depth1);

    if (depth0 == 0) {
      for (i = 0; i < MAX_BUFF_PENDING; i++)
        gc->bufferSwaps[i] = 0xffffffff;
      gc->swapsPending = 0;
      goto NPDONE;
    }
  }

  /*
  **  There are two cases here:  One where the read pointer has wrapped around
  **  behind us, and one where it's ahead of us.
  */
  if (readPtr < gc->lastSwapCheck) { /* We've wrapped */
    for (i = 0; i < MAX_BUFF_PENDING; i++) {
      /* If it's between the last check and the end of the FIFO or between the
       beginning of the FIFO and the current Read pointer, then it's gone
       */
      if ( (gc->bufferSwaps[i] != 0xffffffff) && (
        (gc->bufferSwaps[i] >= gc->lastSwapCheck) ||
          (gc->bufferSwaps[i] <= readPtr)) ) {
        --gc->swapsPending;
        gc->bufferSwaps[i] = 0xffffffff; /* Free swap slot */
      }
    }
  } else {                      /* It's behind us */
    for (i = 0; i < MAX_BUFF_PENDING; i++) {
      if ((gc->bufferSwaps[i] != 0xffffffff) && (
        (gc->bufferSwaps[i] >= gc->lastSwapCheck) && (gc->bufferSwaps[i] <=
        readPtr))) {
        --gc->swapsPending;
        gc->bufferSwaps[i] = 0xffffffff; /* Free swap slot */
      }
    }
  }

  gc->lastSwapCheck = readPtr;

NPDONE:
  pend = gc->swapsPending;
  GDBG_INFO(86,"grBufferNumPending() = %d\n", pend);

  return pend;
#else
  return 0;
#endif
} /* grBufferNumPending */

/*---------------------------------------------------------------------------
** grChromakeyMode
*/

GR_STATE_ENTRY(grChromakeyMode, void, (GrChromakeyMode_t mode))
{
#define FN_NAME "_grChromakeyMode"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("_grChromakeyMode", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", mode);

  fbzMode = gc->state.shadow.fbzMode;
  if (mode == GR_CHROMAKEY_ENABLE)
    fbzMode |= SST_ENCHROMAKEY;
  else
    fbzMode &= ~SST_ENCHROMAKEY;

  gc->state.shadow.fbzMode = fbzMode;
#undef FN_NAME
} /* grChromaKeyMode */

GR_STATE_ENTRY(grChromaMode, void, (GrChromaRangeMode_t mode))
{
#define FN_NAME "_grChromaMode"
  FxU32 fbzMode, chromaRange;
  GR_BEGIN_NOFIFOCHECK("_grChromaMode", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", mode);

  fbzMode = gc->state.shadow.fbzMode;
  chromaRange = gc->state.shadow.chromaRange;
  if (mode == GR_CHROMARANGE_ENABLE_EXT) {
    fbzMode |= SST_ENCHROMAKEY;
    chromaRange |= SST_ENCHROMARANGE;
  }
  else if ((mode == GR_CHROMARANGE_DISABLE_EXT) &&
           (gc->state.stateArgs.grChromakeyModeArgs.mode == GR_CHROMAKEY_DISABLE)) {
    fbzMode &= ~SST_ENCHROMAKEY;
    chromaRange |= ~SST_ENCHROMARANGE;
  }
  else
    chromaRange |= ~SST_ENCHROMARANGE;
    
  gc->state.shadow.fbzMode = fbzMode;
  gc->state.shadow.chromaRange = chromaRange;
#undef FN_NAME
} /* grChromaMode */

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
#define FN_NAME "_grClipWindow"
  FxU32
    clipLeftRight,              /* SST Clipping Registers */
    clipBottomTop;

  GR_BEGIN_NOFIFOCHECK("_grClipWindow",83);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d %d,%d)\n",minx,miny,maxx,maxy);

  _grClipNormalizeAndGenerateRegValues(minx, miny, maxx, maxy,
                                       &clipLeftRight, &clipBottomTop);

#if !GLIDE3
  REG_GROUP_BEGIN(BROADCAST_ID, clipLeftRight, 2, 0x03);
  {
    REG_GROUP_SET(hw, clipLeftRight, clipLeftRight);
    REG_GROUP_SET(hw, clipBottomTop, clipBottomTop);
  }
  REG_GROUP_END();
#endif /* !GLIDE3 */

  gc->state.shadow.clipLeftRight = clipLeftRight;
  gc->state.shadow.clipBottomTop = clipBottomTop;

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
#define FN_NAME "_grColorCombine"
  FxU32 fbzColorPath;

  GR_BEGIN_NOFIFOCHECK("_grColorCombine",85);
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

  fbzColorPath = gc->state.shadow.fbzColorPath;
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
  gc->state.shadow.fbzColorPath = fbzColorPath;

  GR_END();
#undef FN_NAME
} /* grColorCombine */

/* NB: These are only called when GLIDE_DEBUG is set for tracing */
#if GLIDE_DEBUG
/*---------------------------------------------------------------------------
** grDepthMask
*/
GR_STATE_ENTRY(grDepthMask, void, (FxBool enable))
{
#define FN_NAME "_grDepthMask"
  GR_BEGIN_NOFIFOCHECK("_grDepthMask", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", enable);

  GR_CHECK_COMPATABILITY(FN_NAME,
                         (enable && (gc->grAuxBuf == 0)),
                         "Cannot enable depth buffer w/o allocating one");
#undef FN_NAME
} /* grDepthMask */

/*---------------------------------------------------------------------------
** grColorMask
*/
GR_STATE_ENTRY(grColorMask, void, (FxBool rgb, FxBool alpha))
{
#define FN_NAME "_grColorMask"
  GR_BEGIN_NOFIFOCHECK("_grColorMask", 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x,0x%x)\n", rgb, alpha);
#undef FN_NAME
} /* grColorMask */

/*---------------------------------------------------------------------------
** grFogColorValue
*/
GR_STATE_ENTRY(grFogColorValue, void, (GrColor_t color))
{
#define FN_NAME "_grFogColorValue"
  GR_BEGIN_NOFIFOCHECK("_grFogColorValue", 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", color);

  _grSwizzleColor(&color);

  gc->state.shadow.fogColor = color;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fogColor,  color);
  GR_CHECK_SIZE();
#endif /* !GLIDE3 */
#undef FN_NAME
} /* grFogColorValue */

/*---------------------------------------------------------------------------
** grDepthBiasLevel
**
** Sets the depth bias level.
*/
GR_STATE_ENTRY(grDepthBiasLevel, void, (FxI32 level))
{
#define FN_NAME "_grDepthBiasLevel"
  FxU32 zacolor;
  GR_BEGIN_NOFIFOCHECK("_grDepthBiasLevel", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",level);

  zacolor = gc->state.shadow.zaColor;
  zacolor = (zacolor & ~SST_ZACOLOR_DEPTH) | ((FxI16)level & SST_ZACOLOR_DEPTH);

  gc->state.shadow.zaColor = zacolor;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, zaColor,  zacolor);
  GR_CHECK_SIZE();
#endif /* !GLIDE3 */
#undef FN_NAME
} /* grDepthBiasLevel */

/*---------------------------------------------------------------------------
** grChromaRange
*/

GR_STATE_ENTRY(grChromaRange, void, (GrColor_t max, GrChromaRangeMode_t mode))
{
  FxU32 chromaRange;
  GR_BEGIN_NOFIFOCHECK("_grChromakeyValue", 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", max);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", mode);
  
  chromaRange = gc->state.shadow.chromaRange & SST_ENCHROMARANGE;
  _grSwizzleColor(&max);
  gc->state.shadow.chromaRange = chromaRange | (max & 0x00ffffff) | (mode << 24);
} /* grChromaRange */

/*---------------------------------------------------------------------------
** grChromakeyValue
*/
GR_STATE_ENTRY(grChromakeyValue, void, (GrColor_t color))
{
#define FN_NAME "_grChromakeyValue"
  GR_BEGIN_NOFIFOCHECK("_grChromakeyValue", 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", color);

  _grSwizzleColor(&color);

  gc->state.shadow.chromaKey = color;
#undef FN_NAME
} /* grChromaKeyValue */

#endif /* GLIDE_DEBUG */

/*---------------------------------------------------------------------------
** grConstantColorValue
*/

GR_STATE_ENTRY(grConstantColorValue, void, (GrColor_t color))
{
#define FN_NAME "_grConstantColorValue"
  GR_BEGIN_NOFIFOCHECK("_grConstantColorValue",85);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",color);

  _grSwizzleColor(&color);

#if !GLIDE3
  REG_GROUP_BEGIN(BROADCAST_ID, c0, 2, 0x03);
  {
    REG_GROUP_SET(hw, c0, color);
    REG_GROUP_SET(hw, c1, color);
  }
  REG_GROUP_END();
#endif /* !GLIDE3 */

  gc->state.shadow.color0 = color;
  gc->state.shadow.color1 = color;

  GR_END();
#undef FN_NAME
} /* grConstanColorValue */

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
** grDepthBufferFunction
*/

GR_STATE_ENTRY(grDepthBufferFunction, void, (GrCmpFnc_t fnc))
{
#define FN_NAME "_grDepthBufferFunction"
  FxU32 fbzMode;

  GR_BEGIN_NOFIFOCHECK("_grDepthBufferFunction",85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",fnc);

  fbzMode = gc->state.shadow.fbzMode;
  fbzMode &= ~SST_ZFUNC;
  fbzMode |= (fnc << SST_ZFUNC_SHIFT);

  gc->state.shadow.fbzMode = fbzMode;
  GR_END();
#undef FN_NAME
} /* grDepthBufferFunction */

/*---------------------------------------------------------------------------
** grDepthBufferMode
*/

GR_STATE_ENTRY(grDepthBufferMode, void, (GrDepthBufferMode_t mode))
{
#define FN_NAME "_grDepthBufferMode"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("_grDepthBufferMode", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", mode);

  /* This is fatal because they could stomp on the cmd fifo. */
  GR_CHECK_COMPATABILITY(FN_NAME,
                         (mode != GR_DEPTHBUFFER_DISABLE) && (gc->grAuxBuf == 0),
                         "cannot enable depth buffer w/o allocating one");

  /* turn off all the bits and then turn them back on selectively */
  fbzMode = gc->state.shadow.fbzMode;
  fbzMode &= ~(SST_ENDEPTHBUFFER | 
               SST_WBUFFER | 
               SST_ENZBIAS | 
               SST_ZCOMPARE_TO_ZACOLOR |
               SST_DEPTH_FLOAT_SEL);

  switch (mode) {
  case GR_DEPTHBUFFER_DISABLE:
    break;
    
  case GR_DEPTHBUFFER_ZBUFFER:
    fbzMode |= SST_ENDEPTHBUFFER | SST_ENZBIAS;
    break;

  case GR_DEPTHBUFFER_WBUFFER:
    /*
    ** if fog coordinate is enabled, we need to enable floating point z buffer for w buffering
    */
    fbzMode |= SST_ENDEPTHBUFFER | SST_WBUFFER | SST_ENZBIAS;
    if (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE)    
      fbzMode |= SST_DEPTH_FLOAT_SEL;
    break;

  case GR_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS:
    fbzMode |= SST_ENDEPTHBUFFER | SST_ZCOMPARE_TO_ZACOLOR;
    break;

  case GR_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS:
    fbzMode |= SST_ENDEPTHBUFFER | SST_WBUFFER | SST_ZCOMPARE_TO_ZACOLOR;
    if (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE)    
      fbzMode |= SST_DEPTH_FLOAT_SEL;
    break;
  }

  /*
  ** Update hardware and Glide state
  */
  gc->state.shadow.fbzMode = fbzMode;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode,  fbzMode);
  GR_CHECK_SIZE();

  _grUpdateParamIndex();
#endif /* !GLIDE3 */
#undef FN_NAME
} /* grDepthBufferMode */

/*---------------------------------------------------------------------------
** No Comment
*/
#ifdef GLIDE_DEBUG
static FxBool
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
** grStippleMode
*/

GR_STATE_ENTRY(grStippleMode, void, (GrStippleMode_t mode))
{
#define FN_NAME "_grStippleMode"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("_grStippleMode", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", mode);

  fbzMode = gc->state.shadow.fbzMode; 

  fbzMode &= ~(SST_ENSTIPPLE | SST_ENSTIPPLEPATTERN);

  switch (mode) {
  case GR_STIPPLE_DISABLE:      
    break;           
                    
  case GR_STIPPLE_PATTERN:
    fbzMode |= (SST_ENSTIPPLE | SST_ENSTIPPLEPATTERN);
    break;

  case GR_STIPPLE_ROTATE:
    fbzMode |= SST_ENSTIPPLE;
    break;
  }
   
  gc->state.shadow.fbzMode = fbzMode;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode,  fbzMode);
  GR_CHECK_SIZE();
#endif /* !GLIDE3 */
#undef FN_NAME
} /* grStippleMode */

/*---------------------------------------------------------------------------
** grDitherMode
*/

GR_STATE_ENTRY(grDitherMode, void, (GrDitherMode_t mode))
{
#define FN_NAME "_grDitherMode"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("_grDitherMode", 85);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", mode);

  fbzMode = gc->state.shadow.fbzMode; 
  
  /****************************************************************** 
   * 4/30/99 gregk
   * Forcing dither rendering matrix to 2x2
   * Adding user support for turning on/off dither substraction
   * According to Alpha Blending Quality Tab of control panel choices   
   * Optimal/Sharper -> disable dither subtraction    
   * Smoother        -> enable  dither subtraction
   * (see also _GlideInitEnvironment() in gpci.c )
   ******************************************************************/
   
  fbzMode &= ~(SST_ENDITHER | SST_DITHER2x2 | SST_ENDITHERSUBTRACT);
  
  switch (mode) {
  case GR_DITHER_DISABLE:      
    break;         
  case GR_DITHER_2x2:
  case GR_DITHER_4x4:
    fbzMode |= (SST_ENDITHER | SST_DITHER2x2 | SST_ENDITHERSUBTRACT);
    
    if (_GlideRoot.environment.disableDitherSub == FXTRUE) 
        fbzMode &= ~(SST_ENDITHERSUBTRACT);  
    break;
  }
   
  gc->state.shadow.fbzMode = fbzMode;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fbzMode,  fbzMode);
  GR_CHECK_SIZE();
#endif /* !GLIDE3 */
#undef FN_NAME
} /* grDitherMode */

/*---------------------------------------------------------------------------
** grFogMode
*/

GR_STATE_ENTRY(grFogMode, void, (GrFogMode_t mode))
{
#define FN_NAME "_grFogMode"
  FxU32 fogmode;
  GR_BEGIN_NOFIFOCHECK("_grFogMode", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",mode);

  fogmode = gc->state.shadow.fogMode;
  fogmode &= ~(SST_ENFOGGING | 
               SST_FOGADD | 
               SST_FOGMULT | 
               SST_FOG_Z | 
               SST_FOG_CONSTANT);

  switch (mode & 0xFF) {    /* switch based on lower 8 bits */
  case GR_FOG_DISABLE:
    break;

    /*
    ** disable in Glide 3. 
    */
#if 0
  case GR_FOG_WITH_ITERATED_ALPHA:
    fogmode |= (SST_ENFOGGING | SST_FOG_ALPHA);
    break;
#endif

    /*
    ** iterate perspective correct fog
    */
  case GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT:
    fogmode |= (SST_ENFOGGING);
    break;

  case GR_FOG_WITH_ITERATED_Z:        /* Bug 735 */
    fogmode |= (SST_ENFOGGING | SST_FOG_Z);
    break;

  case GR_FOG_WITH_TABLE_ON_W:
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
  gc->state.shadow.fogMode = fogmode;

#if !GLIDE3
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, fogMode,  fogmode);
  GR_CHECK_SIZE();

  _grUpdateParamIndex();
#endif /* !GLIDE3 */
#undef FN_NAME
} /* grFogMode */

/*
** grFogTable
*/
GR_ENTRY(grFogTable, void, (const GrFog_t fogtable[]))
{
#define FN_NAME "grFogTable"
  int i = 0;
  const int iend = (kInternalFogTableEntryCount >> 1);
  const GrFog_t *locTable = fogtable;

  GR_BEGIN_NOFIFOCHECK("grFogTable",85);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",fogtable);
  GR_ASSERT(fogtable != NULL);

  while(i < iend) {
    REG_GROUP_LONG_BEGIN(BROADCAST_ID, fogTable[i], 32);
    {
      int j;
      
      for (j = 0; j < 32; j++) {
        GrFog_t e0, e1, d0, d1;
        FxU32   entry;
        
        e0 = locTable[0];                     /* lower entry */
        e1 = locTable[1];                     /* upper entry */
        d0 = ((e1 - e0) << 2);                /* del0 in .2 format */
        d1 = (((i + j) == (iend - 1))         /* don't access beyond end of table */
              ? e1 
              : locTable[2]);  
        d1 = (d1 - e1) << 2;                  /* del1 in .2 format */
      
        entry = ((e1 << 24) | (d1 << 16) | (e0 << 8) | d0);

        gc->state.shadow.fogTable[i+j] = entry;
        REG_GROUP_SET(hw, fogTable[i + j], entry );
        locTable += 2;
      }
    }
    REG_GROUP_END();
    
    i += 32;
  }
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
  if (!_GlideRoot.initialized) return;  /* never made it thru startup */

  /* GMT: reset the counter so we can proceed without assertions */
  {
    GR_DCL_GC;
    if (gc != NULL) gc->counter = gc->expected_counter = 0;
  }

  {
    int i;

    GDBG_INFO(80, "grGlideShutdown()\n");

    for(i = 0; i < _GlideRoot.hwConfig.num_sst; i++) {
      GrGC* gc = _GlideRoot.GCs + i;

      /* We can only do the fullscreen shutdown here if we actually did an open */
      if (gc->open) {
        grSstSelect(i);
        grSstWinClose((GrContext_t)gc);
      }
    }
    
    /* Force all fullscreen contexts to be closed */
    _GlideRoot.windowsInit = 0;
    
    /* If there are any surface contexts close them up now too */
    {
      for(i = 0; i < MAX_NUM_CONTEXTS; i++) {
        GrGC* gc = _GlideRoot.surfaceGCHeap[i];

        if (gc != NULL) {
          /* NB: We need to set the current thread's context before
           * continuing so that any internal glide calls have a valid
           * gc from tls via GR_DCL_GC. F*ck this up at your own peril.
           */
          setThreadValue((unsigned long)gc);
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
          /* Flush any remaining commands and cleanup any per gc state */
          grSurfaceReleaseContext((GrContext_t)gc);
#endif          
        }
      }
    }
  }
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
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 97);

  /* NB: This logical write must be split into two writes since the
   * registers are non-contiguous (not good for packet 1) and are the
   * register span is larget (greater than the 14 register mask size
   * for packet 4).
   */
  REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, 11, 0x3C7F);
  {
    REG_GROUP_SET(hw, fbzColorPath, gc->state.shadow.fbzColorPath);
    REG_GROUP_SET(hw, fogMode, gc->state.shadow.fogMode);
    REG_GROUP_SET(hw, alphaMode, gc->state.shadow.alphaMode);
    REG_GROUP_SET(hw, fbzMode, gc->state.shadow.fbzMode);
    
    REG_GROUP_SET(hw, lfbMode, gc->state.shadow.lfbMode);
    REG_GROUP_SET(hw, clipLeftRight, gc->state.shadow.clipLeftRight);
    REG_GROUP_SET(hw, clipBottomTop, gc->state.shadow.clipBottomTop);
    /* nopCMD */

    /* fastFillCmd */
    /* swapBufferCmd */
    REG_GROUP_SET(hw, fogColor, gc->state.shadow.fogColor);
    REG_GROUP_SET(hw, zaColor, gc->state.shadow.zaColor);

    REG_GROUP_SET(hw, chromaKey, gc->state.shadow.chromaKey);
    REG_GROUP_SET(hw, chromaRange, gc->state.shadow.chromaRange);
  }
  REG_GROUP_END();
  
  REG_GROUP_BEGIN(BROADCAST_ID, stipple, 3, 0x07);
  {
    REG_GROUP_SET(hw, stipple, gc->state.shadow.stipple);
    REG_GROUP_SET(hw, c0, gc->state.shadow.color0);
    REG_GROUP_SET(hw, c1, gc->state.shadow.color1);
  }
  REG_GROUP_END();

#if GLIDE3
  gc->state.invalid = 0;  
#endif /* GLIDE3 */

  GR_END();
#undef FN_NAME
} /* _grFlushCommonStateRegs */

/*---------------------------------------------------------------------------
** grGlideSetState
*/
GR_ENTRY(grGlideSetState, void, (const void *state))
{
#define FN_NAME "grGlideSetState"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 87);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",state);

  GR_ASSERT(state != NULL);
  
  /* if texture mapping changed then we need to issue a NOP command*/
  {
    const FxBool texChangeP = (((gc->state.shadow.fbzColorPath ^ 
                                 ((GrState*)state)->shadow.fbzColorPath) & SST_ENTEXTUREMAP) != 0);
    if (texChangeP) {
      GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
      GR_SET(BROADCAST_ID, hw, nopCMD,  0);
      GR_CHECK_SIZE();
    }
  }

  /* copy all the state */
  gc->state = *((GrState *)state);

  /* Update the hardware state from the saved state. */
  _grFlushCommonStateRegs();
  
  {
    int tmu;

    for(tmu = 0; tmu < gc->num_tmu; tmu++) {
      SstRegs* tmuregs = SST_TMU(hw, tmu);
      const FifoChipField chipField = (FifoChipField)(0x02UL << tmu);

      /* Synch flush data w/ new state */
      gc->tmuMemInfo[tmu].postPacket[1] = ~gc->state.shadow.tmuState[tmu].texBaseAddr;
      gc->tmuMemInfo[tmu].postPacket[5] = gc->state.shadow.tmuState[tmu].texBaseAddr;

      REG_GROUP_BEGIN(chipField, textureMode, 7, 0x7F);
      {
        REG_GROUP_SET(tmuregs, textureMode, gc->state.shadow.tmuState[tmu].textureMode);
        REG_GROUP_SET(tmuregs, tLOD, gc->state.shadow.tmuState[tmu].tLOD);
        REG_GROUP_SET(tmuregs, tDetail, gc->state.shadow.tmuState[tmu].tDetail);
        REG_GROUP_SET(tmuregs, texBaseAddr, gc->state.shadow.tmuState[tmu].texBaseAddr);
        
        REG_GROUP_SET(tmuregs, texBaseAddr1, gc->state.shadow.tmuState[tmu].texBaseAddr_1);
        REG_GROUP_SET(tmuregs, texBaseAddr2, gc->state.shadow.tmuState[tmu].texBaseAddr_2);
        REG_GROUP_SET(tmuregs, texBaseAddr38, gc->state.shadow.tmuState[tmu].texBaseAddr_3_8);
      }
      REG_GROUP_END();

      REG_GROUP_BEGIN(chipField, chromaKey, 2, 0x03);
      {
        REG_GROUP_SET(tmuregs, chromaKey, gc->state.shadow.tmuState[tmu].texchromaKey);
        REG_GROUP_SET(tmuregs, chromaRange, gc->state.shadow.tmuState[tmu].texchromaRange);
      }
      REG_GROUP_END();
    }
  }


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
#if !DRI_BUILD
GR_STATE_ENTRY(grRenderBuffer, void, (GrBuffer_t buffer))
{
#define FN_NAME "_grRenderBuffer"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",buffer);
  GR_CHECK_F(myName, buffer >= GR_BUFFER_AUXBUFFER, "invalid buffer");

  {
    const FxU32 
      oldBuffer = gc->curBuffer;

    gc->curBuffer = ((buffer == GR_BUFFER_FRONTBUFFER)
                     ? gc->frontBuffer
                     : gc->backBuffer);
    if (oldBuffer != gc->curBuffer) {
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
      REG_GROUP_SET(hw, colBufferAddr, gc->buffers[gc->curBuffer]);
      REG_GROUP_END();

      gc->state.shadow.colBufferAddr = gc->buffers[gc->curBuffer];
    }
  }

  GR_END();
#undef FN_NAME
} /* grRenderBuffer */
#else /* DRI_BUILD */
GR_STATE_ENTRY(grRenderBuffer, void, (GrBuffer_t buffer))
{
#define FN_NAME "_grRenderBuffer"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",buffer);
  GR_CHECK_F(myName, buffer >= GR_BUFFER_AUXBUFFER, "invalid buffer");

  {
    gc->curBuffer = ((buffer == GR_BUFFER_FRONTBUFFER)
                     ? gc->frontBuffer
                     : gc->backBuffer);
    REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3); 
    REG_GROUP_SET(hw, colBufferAddr, gc->buffers[gc->curBuffer]);
    REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride :
		    gc->state.shadow.colBufferStride);
    REG_GROUP_END();

    gc->state.shadow.colBufferAddr = gc->buffers[gc->curBuffer];
  }

  GR_END();
#undef FN_NAME
} /* grRenderBuffer */
#endif

GR_ENTRY(grCheckForRoom, void, (FxI32 n))
{
#define FN_NAME "grCheckForRoom"
  GR_DCL_GC;

  /* dpc - 13 sep 1997 - FixMe!
   * Setting one packet for now.
   */
  GR_CHECK_FOR_ROOM(n, 1);
#undef FN_NAME
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
  const FxU32 fbzColorPath = gc->state.shadow.fbzColorPath;
  const FxU32 fogMode = gc->state.shadow.fogMode;
  const FxU32 fbzMode = gc->state.shadow.fbzMode;

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

    GR_ASSERT(gc->state.tmuMask < sizeof(paramI_array) / sizeof(paramI_array[0]));
    paramIndex |= paramI_array[gc->state.tmuMask];
  }  

  /* See if we need iterated RGB */
  if (gc->state.cc_requires_it_rgb ) {
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
      /*
      ** enable z iterator if we are using fog coordinate and w buffering
      */
      if ((fogMode & SST_ENFOGGING) && (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE))
        paramIndex |= STATE_REQUIRES_OOZ;
    } else {
      paramIndex |= STATE_REQUIRES_OOZ;
    }
  }

  /* See what fogMode contributes */
  if (fogMode & SST_ENFOGGING) {
    if (fogMode & SST_FOG_Z) {
      paramIndex |= STATE_REQUIRES_OOZ;
    } else {
      paramIndex |= STATE_REQUIRES_OOW_FBI;
    }
  }

  /*
  **  Now we know everything that needs to be iterated.  Prune back
  **  the stuff that isn't explicitly different
  **
  **  NOTE: by GMT, STATE_REQUIRES_OOW_FBI is set whenever texture mapping
  **        is enabled
  */
#if defined(GLIDE3) && (GLIDE3_ALPHA)
  /* Turn off W for TMU0 if we don't have a hint */
  if (paramIndex & STATE_REQUIRES_W_TMU0) {
    GR_ASSERT(paramIndex & STATE_REQUIRES_OOW_FBI);
    if (gc->state.vData.fogInfo.mode == GR_PARAM_DISABLE) {
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
        if (gc->state.vData.q0Info.mode == GR_PARAM_DISABLE)
          paramIndex &= ~STATE_REQUIRES_W_TMU0;
      }
      else {
        if ((gc->state.vData.q0Info.mode == GR_PARAM_DISABLE) ||
            (gc->state.vData.wInfo.mode == GR_PARAM_DISABLE))
          paramIndex &= ~STATE_REQUIRES_W_TMU0;
      }
    }
    else {
      /*
      ** win coords and no texturing
      */
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
        if ((gc->state.vData.q0Info.mode == GR_PARAM_DISABLE) && 
            (gc->state.vData.qInfo.mode == GR_PARAM_DISABLE))
          paramIndex &= ~STATE_REQUIRES_W_TMU0;
      }
      /*
      ** clip coords and no texturing
      */
      else {
        if ((gc->state.vData.q0Info.mode == GR_PARAM_DISABLE) && 
            (gc->state.vData.wInfo.mode == GR_PARAM_DISABLE))
          paramIndex &= ~STATE_REQUIRES_W_TMU0;
      }
    }     
  }
  
  /* Turn off ST for TMU1 if TMU0 is active and TMU1 is not different */
  if (((paramIndex & (STATE_REQUIRES_ST_TMU0 | STATE_REQUIRES_ST_TMU1)) ==
       (STATE_REQUIRES_ST_TMU0 | STATE_REQUIRES_ST_TMU1)) &&
      gc->state.vData.st1Info.mode == GR_PARAM_DISABLE)
    paramIndex &= ~STATE_REQUIRES_ST_TMU1;
  
  /* Turn off W for TMU1 if we have a previous W, and don't have a hint */ 
  if (paramIndex & STATE_REQUIRES_W_TMU1) {
    if (gc->state.vData.fogInfo.mode == GR_PARAM_DISABLE) {
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
        if (gc->state.vData.q1Info.mode == GR_PARAM_DISABLE)
          paramIndex &= ~STATE_REQUIRES_W_TMU1;
      }
      else {
        if ((gc->state.vData.q1Info.mode == GR_PARAM_DISABLE) || 
            (gc->state.vData.wInfo.mode == GR_PARAM_DISABLE))
          paramIndex &= ~STATE_REQUIRES_W_TMU1;
      }
    }
    else {
      /*
      ** win coords and no texturing
      */
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
        if ((gc->state.vData.q1Info.mode == GR_PARAM_DISABLE) && 
            (gc->state.vData.qInfo.mode == GR_PARAM_DISABLE))
          paramIndex &= ~STATE_REQUIRES_W_TMU1;
      }
      /*
      ** clip coords and no texturing
      */
      else {
        if ((gc->state.vData.q1Info.mode == GR_PARAM_DISABLE) && 
            (gc->state.vData.wInfo.mode == GR_PARAM_DISABLE))
          paramIndex &= ~STATE_REQUIRES_W_TMU1;
      }
    }     
  }
#else
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
#endif

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
  if (gc->state.vData.fogInfo.mode == GR_PARAM_DISABLE) {
    if (gc->state.vData.zInfo.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_OOZ;
    if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
      if (gc->state.vData.qInfo.mode == GR_PARAM_DISABLE)
        paramIndex &= ~STATE_REQUIRES_OOW_FBI;
    }
    else {
      if (gc->state.vData.wInfo.mode == GR_PARAM_DISABLE)
        paramIndex &= ~STATE_REQUIRES_OOW_FBI;
    }
  }
  else {
    if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
      if (gc->state.vData.qInfo.mode == GR_PARAM_DISABLE)
        paramIndex &= ~STATE_REQUIRES_OOZ;
    }
    else {
      if (gc->state.vData.wInfo.mode == GR_PARAM_DISABLE)
        paramIndex &= ~STATE_REQUIRES_OOZ;
    }
  }
  if (gc->state.vData.st0Info.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_ST_TMU0;
  if (gc->state.vData.st1Info.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_ST_TMU1;
  if (gc->state.vData.fogInfo.mode == GR_PARAM_DISABLE) {
    if (gc->state.vData.q0Info.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_W_TMU0;
    if (gc->state.vData.q1Info.mode == GR_PARAM_DISABLE)
      paramIndex &= ~STATE_REQUIRES_W_TMU1;
  }

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

#if GLIDE3 || GLIDE_PACKED_RGB
  FxBool packedRGB = FXFALSE;
#endif /* GLIDE3 || GLIDE_PACKED_RGB */
    
#ifdef GLIDE_DEBUG
  /*
    tsuIndices aren't fixed, they are programmable, ..taco
  static char *p_str[] = {"x","y","z","r","g","b","ooz","a","oow",
                          "s0","t0","w0","s1","t1","w1","s2","t2","w2"};
                          */
#endif /* GLIDE_DEBUG */
  
  GDBG_INFO(85,"(s) paramIndex=0x%x\n", FN_NAME, gc->state.paramIndex);
  
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

#ifdef GLIDE3
  gc->state.vData.vSize = 8;         /* XY * 3 */
  /*
  ** make x, y default to 0 and 1
  */
  /*
  gc->sDataList[sVtxSize++] = gc->state.vData.vertexInfo.offset + GR_VERTEX_OFFSET_X;
  gc->sDataList[sVtxSize++] = gc->state.vData.vertexInfo.offset + GR_VERTEX_OFFSET_Y;
  */
#endif /* GLIDE3 */

  if (i & STATE_REQUIRES_IT_DRGB) {
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
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
#elif defined(GLIDE3)
    gc->cmdTransportInfo.paramMask  |= SST_SETUP_RGB;
    if (gc->state.vData.colorType == GR_FLOAT) {
      gc->tsuDataList[curTriSize + 0] = gc->state.vData.rgbInfo.offset;
      gc->tsuDataList[curTriSize + 1] = gc->state.vData.rgbInfo.offset + GR_COLOR_OFFSET_GREEN;
      gc->tsuDataList[curTriSize + 2] = gc->state.vData.rgbInfo.offset + GR_COLOR_OFFSET_BLUE;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize + 0] = 2;
      gc->tsuDataListScaler[curTriSize + 1] = 2;
      gc->tsuDataListScaler[curTriSize + 2] = 2;
#endif
      gc->state.vData.vSize += 12; /* RGB */
    }
    else {
      gc->tsuDataList[curTriSize + 0] = gc->state.vData.pargbInfo.offset;
      packedRGB = FXTRUE;
      gc->state.vData.vSize += 4;
    }
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
#if GLIDE3
#if !GLIDE3_VERTEX_LAYOUT
    if (gc->state.vData.colorType == GR_FLOAT) {
#else /* GLIDE3_VERTEX_LAYOUT */
    if (gc->state.vData.colorType == GR_FLOAT) {
#endif /* GLIDE3_VERTEX_LAYOUT */
      curTriSize += 3;
      params += 3;
    } else {
      curTriSize += 1;
      params += 1;

      packedRGB = FXTRUE;
    }
#else /* !GLIDE3 */
    curTriSize += 1;
    params += 1;

    packedRGB = FXTRUE;
#endif /* GLIDE3 */
#else /* !GLIDE_PACKED_RGB */
    if (gc->state.vData.colorType == GR_FLOAT) {
      curTriSize += 3;
      params += 3;
    }
    else {
      curTriSize += 1;
      params += 1;
    }
#endif /* !GLIDE_PACKED_RGB */
  }

  if (i & STATE_REQUIRES_IT_ALPHA) {
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    gc->cmdTransportInfo.paramMask   |= SST_SETUP_A;
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_A_OFFSET << 2;
#elif defined(GLIDE3)
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_A;
    if (gc->state.vData.colorType == GR_FLOAT) {
      gc->tsuDataList[curTriSize] = gc->state.vData.aInfo.offset;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 2;
#endif
      gc->state.vData.vSize += 4;
    }
    else if (!(i & STATE_REQUIRES_IT_DRGB)) {
      packedRGB = FXTRUE;
      gc->tsuDataList[curTriSize] = gc->state.vData.pargbInfo.offset;
      gc->state.vData.vSize += 4;
    }
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_A_OFFSET << 2;
    gc->regDataList[curTriSize + 0].addr = (float*)&hw->ALF;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    curTriSize += 1;
#elif defined(GLIDE3)
    if ((gc->state.vData.colorType == GR_FLOAT) || (!(i & STATE_REQUIRES_IT_DRGB)))
      curTriSize += 1;
#endif /* GLIDE_HW_TRI_SETUP */

#if GLIDE_PACKED_RGB
#if GLIDE3
#if !GLIDE3_VERTEX_LAYOUT
    if (gc->state.vData.colorInfo.type == GR_FLOAT) {
#else /* GLIDE3_VERTEX_LAYOUT */
    if (gc->state.vData.colorType == GR_FLOAT) {
#endif /* GLIDE3_VERTEX_LAYOUT */
      params += 1;
    }
    else
#endif /* GLIDE3 */ 
    {
      /* Only increment the parameter packet size if we have not already
       * added the rgb fields.  
       */
      if ((i & STATE_REQUIRES_IT_DRGB) == 0) params += 1;
      packedRGB = FXTRUE;
    }
#else /* !GLIDE_PACKED_RGB */
    if ((gc->state.vData.colorType == GR_FLOAT) || !(i & STATE_REQUIRES_IT_DRGB)) 
      params += 1;
#endif /* !GLIDE_PACKED_RGB */
  }
  
  if (i & STATE_REQUIRES_OOZ) {
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_OOZ_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_Z;
#elif defined(GLIDE3)
    /*
    ** use z iterator for w if w buffering and fog coordinate enable
    */
    if ((gc->state.shadow.fbzMode & SST_DEPTH_FLOAT_SEL) && 
        (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)) {
      gc->tsuDataList[curTriSize] = gc->state.vData.qInfo.offset;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 0;
#endif
    }
    else {
      gc->tsuDataList[curTriSize] = gc->state.vData.zInfo.offset;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 1;
#endif
    }
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_Z;

    gc->state.vData.vSize += 4;
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
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_OOW_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_Wfbi;
#elif defined(GLIDE3)
    /*
    ** if fog coordinate is enabled, use w for fog coord
    */
    if ((gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE) && 
        (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)) {
      gc->tsuDataList[curTriSize] = gc->state.vData.fogInfo.offset;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 4;
#endif
    }
    else {
      gc->tsuDataList[curTriSize] = gc->state.vData.qInfo.offset;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 0;
#endif
    }
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_Wfbi;

    gc->state.vData.vSize += 4;
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
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_OOW_TMU0_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_W0;
#elif defined(GLIDE3)
    /*
    ** if q0 is disabled and fogCoord/Q(in clip coords) is enabled, we need to move oow to w0
    */
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
      gc->tsuDataList[curTriSize] = gc->state.vData.q0Info.offset;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 4;
#endif
    }
    else {
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)    
        gc->tsuDataList[curTriSize] = gc->state.vData.qInfo.offset;
      else
        gc->tsuDataList[curTriSize] = gc->state.vData.wInfo.offset;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 0;
#endif
    }

    gc->cmdTransportInfo.paramMask       |= SST_SETUP_W0;
    gc->state.vData.vSize += 4;
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
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_SOW_TMU0_OFFSET << 2;
    gc->tsuDataList[curTriSize + 1]      = GR_VERTEX_TOW_TMU0_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_ST0;
#elif defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = gc->state.vData.st0Info.offset;
    gc->tsuDataList[curTriSize + 1]      = gc->state.vData.st0Info.offset 
      + GR_TEXTURE_OFFSET_T;
#ifdef GLIDE3_SCALER
    gc->tsuDataListScaler[curTriSize] = 3;
    gc->tsuDataListScaler[curTriSize+1] = 3;
#endif
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_ST0;
    gc->state.vData.vSize += 8;
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
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_OOW_TMU1_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_W1;
#elif defined(GLIDE3)
    /*
    ** if q1 is disabled and fogCoord/Q(in clip coords) is enabled, we need to move oow to w1
    */
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) {
      gc->tsuDataList[curTriSize] = gc->state.vData.q1Info.offset;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 4;
#endif
    }
    else {
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)    
        gc->tsuDataList[curTriSize] = gc->state.vData.qInfo.offset;
      else
        gc->tsuDataList[curTriSize] = gc->state.vData.wInfo.offset;
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 0;
#endif
    }
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_W1;
    gc->state.vData.vSize += 4;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = GR_VERTEX_OOW_TMU1_OFFSET << 2;
    gc->regDataList[curTriSize + 0].addr = (float*)&tmu1->W;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

    curTriSize += 1;
    params += 1;

  }

  if (i & STATE_REQUIRES_ST_TMU1) {
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_SOW_TMU1_OFFSET << 2;
    gc->tsuDataList[curTriSize + 1]      = GR_VERTEX_TOW_TMU1_OFFSET << 2;
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_ST1;
#elif defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0] = gc->state.vData.st1Info.offset;
    gc->tsuDataList[curTriSize + 1] = gc->state.vData.st1Info.offset + GR_TEXTURE_OFFSET_T;
#ifdef GLIDE3_SCALER
    gc->tsuDataListScaler[curTriSize] = 3;
    gc->tsuDataListScaler[curTriSize+1] = 3;
#endif
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_ST1;
    gc->state.vData.vSize += 8;
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
  
#ifdef GLIDE_VERTEX_TABLE
  {
    FxU32 vsize = 0;
    for (i = 0; i < 16; i++) {
      gc->state.vTable[i] = vsize;
      vsize += gc->state.vData.vSize;
    }
  }
#endif

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
  gc->curVertexSize = ((2 + params) << 2);

  /* Packet size: 3 * vertex size */
  gc->curTriSize = (gc->curVertexSize << 1) + gc->curVertexSize;

  /* Shift the parameter mask to just write into the packet header. */
  GDBG_INFO(191, "CVG ParamMask: 0x%X\n", gc->cmdTransportInfo.paramMask);
  gc->cmdTransportInfo.paramMask <<= SSTCP_PKT3_PMASK_SHIFT;

#if GLIDE_PACKED_RGB || GLIDE3
  if (packedRGB) gc->cmdTransportInfo.paramMask |= SSTCP_PKT3_PACKEDCOLOR;
#endif /* GLIDE_PACKED_RGB || GLIDE3 */

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
    GDBG_INFO(282,"    tsuDataList[%d] = %2d \n",
              i, gc->tsuDataList[i] >> 2 );
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
