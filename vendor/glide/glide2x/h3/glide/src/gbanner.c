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
** 
** 19    2/19/99 5:54p Peter
** new splash screen
 * 
 * 17    1/05/98 6:06p Atai
 * glide extension stuff
 * 
 * 16    12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 15    11/18/97 4:36p Peter
 * make thing more translucent
 * 
 * 14    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 13    7/26/97 3:04p Peter
 * gratuitous plug is translucent
 * 
 * 12    7/08/97 2:48p Peter
 * shameless plug just uses lfb routines
 * 
 * 11    5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 10    5/21/97 6:05a Peter
 * 
 * 9     5/02/97 2:08p Pgj
 * screen_width/height now FxU32
 * 
 * 8     3/16/97 2:24a Jdt
 * Fixed bug.  Didn't initialize info.
 * 
 * 7     3/12/97 11:51p Jdt
 * Watcom warning.
 * 
 * 6     3/12/97 4:20p Jdt
 * Fixed for VG96 and optimized SST-1
 * 
 * 5     2/26/97 11:55a Jdt
 * Updated banner for new lfb api
 * 
 * 4     12/23/96 1:37p Dow
 * chagnes for multiplatform glide
**
*/
#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#ifdef GLIDE_PLUG

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
static const void *
getPlugdata (FxU32* w, FxU32* h, FxI32* strideInBytes,
             GrLfbWriteMode_t* format)
{
  GR_DCL_GC;
  if (gc->pluginInfo.plugProc != NULL)
      return (*gc->pluginInfo.plugProc)(w,h,strideInBytes,format);
  return NULL;
}
#else
static __inline const void *
getPlugdata (FxU32* w, FxU32* h, FxI32* strideInBytes,
             GrLfbWriteMode_t* format)
{
  return NULL;
}
#endif


void
_grShamelessPlug(void)
{
  GrState state;
  FxU32 plugWidth, plugHeight;
  FxI32 plugStride;
  GrLfbWriteMode_t plugFormat;
  const void* plugData;

  GR_BEGIN_NOFIFOCHECK("_grShamelessPlug", 80);
  GDBG_INFO_MORE(gc->myLevel, "()\n");

  plugData =
    getPlugdata(&plugWidth, &plugHeight, &plugStride, &plugFormat);

  if (!plugData) return;

      grGlideGetState(&state);
      grDisableAllEffects();
      
      grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_CONSTANT,
                     GR_COMBINE_OTHER_TEXTURE, FXFALSE);
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_NONE,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
      grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,
                           GR_BLEND_ONE_MINUS_SRC_ALPHA,
                           GR_BLEND_ZERO, GR_BLEND_ZERO);
      grClipWindow(0, 0,
                   gc->state.screen_width - 1,
                   gc->state.screen_height - 1); 
      grDepthMask(FXFALSE);
      grDepthBufferFunction(GR_CMP_ALWAYS);
      grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
      
      grChromakeyValue(0x0000);

      grChromakeyMode(GR_CHROMAKEY_ENABLE);
      grLfbConstantAlpha((FxU8) 0x40);
      grLfbWriteColorFormat(GR_COLORFORMAT_ARGB);
      
      _grLfbWriteRegion(FXTRUE, GR_BUFFER_BACKBUFFER,
                        gc->state.screen_width - (plugWidth + 1), 
                        gc->state.screen_height - (plugHeight + 1),
                        GR_LFBWRITEMODE_565,
                        plugWidth, plugHeight,
                        plugStride,
                        plugData);
      
      grGlideSetState(&state);

  GR_END();
} /* _grShamelessPlug */

#endif  /* GLIDE_PLUG */
