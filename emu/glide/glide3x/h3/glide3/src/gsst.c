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
** Revision 1.1.2.6  2005/06/09 18:32:29  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.2.5  2005/05/25 08:56:23  jwrdegoede
** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
**
** Revision 1.1.2.4  2004/10/05 14:47:15  dborca
** conditional compilation a bit more sane
**
** Revision 1.1.2.3  2004/10/04 09:26:31  dborca
** DOS/OpenWatcom support
**
** Revision 1.1.2.2  2004/02/16 07:42:16  dborca
** grSetNumPendingBuffers visible with grGetProcAddress
**
** Revision 1.1.2.1  2003/11/07 13:38:38  dborca
** unite the clans
**
** Revision 1.1.1.1.6.1  2003/06/29 18:43:27  guillemj
** Fix compilation warnings.
**
** Revision 1.1.1.1  1999/11/24 21:45:01  joseph
** Initial checkin for SourceForge
**
** 
** 11    6/24/99 1:28a Stb_mmcclure
** Modifications to fix PRS 6627. Added hwcUnmapMemory9x for revised 9x
** context switching code.
** 
** 10    6/10/99 12:19p Stb_mmcclure
** Modifications for per-process lost context for glide in Win9x
** environments. RT3 Fix PRS 6249.
** 
** 9     5/28/99 12:58p Atai
** move color format above doSplash. The new glide/splash api is smart
** enough to take the in-coming color format
** 
** 8     5/24/99 2:48p Jamesb
** Added ptrLostContext to exported cmdTransport struct.
** 
** 7     4/15/99 5:38p Dow
** protected winclose
** 
** 110   4/15/99 5:34p Dow
** Protected WinClose
** 
** 109   4/10/99 2:21p Atai
** set contexP to 1
** 
** 108   4/10/99 1:24p Atai
** fixed code to compile in packet fifo mode 
** 
** 107   4/09/99 4:52p Dow
** Get lostcontext each time
** 
** 106   4/07/99 7:18p Atai
** added uma extension
** 
** 105   4/05/99 8:25p Dow
** Alt tab mostly happy
** 
** 104   4/05/99 4:05p Atai
** I removed hwcShareContextData code in my previous check-in
** 
** 103   4/04/99 8:51p Atai
** Partial check-in for alt-tab issue. set FX_GLIDE_ALT_TAB=1 to build
** glide3x with hwcQueryContext built into GR_BEGIN_NOFIFOCHECK. It works
** with DEBUG glide only. In the non-debug glide, we can still see the
** desktop corruption.
** 
** 100   4/02/99 11:51a Peter
** only monitor state changes not every time
** 
** 99    4/01/99 7:55p Peter
** change to allow fsem recovery
** 
** 98    3/31/99 9:02p Dow
** context loosing means no writing to hw
** 
** 97    3/24/99 6:17p Peter
** reduce nop flush for chain downloads
** 
** 96    3/22/99 5:38p Peter
** single load of splash
** 
** 95    3/19/99 11:30a Peter
** protect against splash screen not restoring app state
** 
** 94    3/17/99 5:08p Peter
** removed whacky stuff now that the command fifo threshold stuff appears
** to make all happy (including the k7)
** 
** 93    3/14/99 1:48p Peter
** cmd's bng optimization
** 
** 92    3/12/99 2:27p Dow
** Turn off hole counting for PIII and K7
** 
** 91    3/11/99 6:42p Dow
** Resolution help
** 
** 90    3/10/99 10:42a Peter
** bump-n-grind workaround for katmai until the bug is better
** characterized
** 
** 89    3/05/99 10:33p Peter
** fixed the surface fifo state race condition (thanks to Ken Dyke)
** 
** 88    3/02/99 2:04p Peter
** removed tmu check that should be done in grTexCombine
** 
** 87    2/27/99 12:25p Dow
** gsst.c
** 
** 86    2/26/99 10:27a Peter
** Mmmm.... 8.3
** 
** 85    2/19/99 8:03p Peter
** new splash crapola
** 
** 84    2/18/99 3:51p Kcd
** Fixed dumb Codewarrior warning.
** 
** 83    2/18/99 3:05p Atai
** allow calls guGammaCorrectionRGB after
** grGlideShutdown. Check if we have a null gc.
** 
** 82    2/13/99 2:01p Dow
** Added code for new resolutions
** 
** 81    2/11/99 1:38p Atai
** sync buffer swap pending code, the right way.
** 
** 80    2/05/99 9:38a Atai
** fixed grSelectContext return value. If the gc is open, return FXTRUE.
** 
** 79    2/02/99 4:41p Peter
** debugging info to grFlush
** 
** 78    1/25/99 6:35p Peter
** tiled texture cleanup, default lfb buffer when single buffering
** 
** 77    1/06/99 11:30a Peter
** cleanup win fifo locking
** 
** 76    12/23/98 2:01p Peter
** nt currently has mutexing problems via ddraw and extescape
** 
** 75    12/11/98 1:36p Peter
** made grFlush's operation more obvious
** 
** 74    12/09/98 6:25p Atai
** grTexCombine did the right thing for the un-used TMU.  Initialize the
** 2nd TMU value to take care of "set FX_GLIDE_NUM_TMU=1"
** 
** 73    12/09/98 5:10p Atai
** set MAXLOD = MINLOD = 8 in _grUpdateParamIndex if ST1 is not used
** 
** 72    12/09/98 2:02p Atai
** Added _grTexForceLod back. Set tLOD min = max = 8 for the 2nd TMU by
** default for Avenger to increase single texturing tri fillrate.
** 
** 71    12/09/98 10:22a Dow
** Fixed infinite recursion on shutdown
** 
** 70    12/08/98 2:23p Dow
** Fixed effage in grSstWinClose when invalid context is passed
** 
** 69    12/03/98 11:26p Dow
** Code 'cleanup' heheh
** 
** 68    12/02/98 2:07p Dow
** removed spurious call to guGammaCorrectionRGB
** 
** 67    11/30/98 6:57p Peter
** video memory fifo's
** 
** 66    11/25/98 12:10p Atai
** fixed sdram buffer clear (gc->state.shadow.auxBufferStride not
** initialized)
** 
** 65    11/21/98 10:19a Atai
** fixed test37 grChromaRangeModeExt error and rename functions
** 
** 64    11/15/98 3:21a Atai
** first attempt to make 2 tmus work in H4 glide3x full screen mode, just
** in time check-in for comdex demo. warning: the code is not completed
** yet.
** 
** 63    11/12/98 3:12p Mikec
** In winOpen, moved setting color format in gc to after splash. Fixed the
** red screen bug after splash (RGBA format).
** 
** 62    11/02/98 5:34p Peter
** tls per thread for fullscreen contexts
** 
** 61    10/14/98 3:38p Dow
** Gamma stuff
** 
** 60    10/14/98 1:47p Jdt
** Fix state restore buffer and add archDispatchProcs setup to
** selectContext
** 
** 59    10/13/98 8:47p Dow
** Works with 4MB boards
** 
** 58    10/12/98 9:51a Peter
** dynamic 3DNow!(tm)
** 
** 57    10/08/98 10:29a Dow
** Fixes triple buffering
** 
** 56    9/18/98 10:52a Dow
** VidMode Stuff
** 
** 55    9/14/98 9:57a Jdt
** 
** 54    9/11/98 10:45p Jdt
** switch over to statically allocated in-memory fifo.
** 
** 53    9/09/98 1:33p Atai
** callback the error routine if window handle is not valid in
** grSstWinOpen
** 
** 52    9/04/98 11:35a Peter
** re-open fix for nt (thanks to taco/rob/nt bob)
** 
** 51    8/31/98 4:03p Dow
** Contol Panel
** 
** 50    8/30/98 1:34p Dow
** State & other optimizations
** 
** 49    8/29/98 2:29p Peter
** set context value in case grGlideInit was called from a different
** thread
** 
** 48    8/28/98 4:37p Atai
** 1. added MIN_FIFO_SIZE for memory checking
** 2. hack for resolution checking if we have 8M board and triple
** buffering on
** 
** 47    8/27/98 9:27p Atai
** fix env variable for glide3x
** 
** 46    8/27/98 5:57p Dow
** Indentation
** 
** 45    8/26/98 9:59p Jdt
** Compute and store pointer to aux buffer.
** 
** 44    8/21/98 3:48p Jdt
** Fixed flush bug that was causing exit.
** 
** 43    8/03/98 10:40a Atai
** rename slpash dll to "3dfxspl3.dll"
** 
** 42    8/03/98 6:41a Jdt
** removed gc argument from assertDefaultState, multi-thread changes
** 
** 41    8/02/98 5:01p Dow
** Glide Surface Extension
** 
** 39    7/20/98 10:49p Jdt
** Don't send chromarange to all chips on H3....
** 
** 38    7/18/98 1:45p Jdt
** Removed TACO_MEMORY_FIFO_HACK
** 
** 37    7/18/98 10:41a Dow
** Num TMU
** 
** 36    7/18/98 12:30a Jdt
** Remove reference to colorcombinedelta0mode
** Added initialzation for state restoreation buffer to initGC.
** Changes to reflect new shadow register structure.
** 
** 35    7/17/98 10:44a Atai
** fixed grTexNCCTable and clip coords st with aspect ratio
** 
** 34    7/16/98 8:19a Jdt
** TACO_MEMORY_FIFO_HACK now enables 1 window glide in window.
** 
** more direct read protection.
** 
** 33    7/13/98 10:43p Jdt
** Various Surface Implementation Changes
** 
** 32    7/13/98 9:57p Jdt
** Removed guTexMemReset()
** 
** Made initGC public.
** 
** 31    7/01/98 12:41p Jdt
** Reorganized grSstWinOpen, factoring out initialization functionality
**            that will be shared between fullscreen and windowed
** operation.
**            Protected hacks for Glide/Win ( FX_TACO_MEMORY_FIFO_HACK )
** 
** 28    6/12/98 11:35a Atai
** temporary disable guGammaCorrectionRGB
** 
** 27    6/11/98 12:53p Atai
** remove grGammaCorrectionValue
** 
** 26    6/10/98 12:53p Atai
** replace grSstControl with grEnable/grDisable(GR_PASSTHRU)
** 
** 25    6/10/98 11:58a Peter
** lfb tiled addressing
** 
** 24    6/04/98 12:12p Peter
** splash dll rename
** 
** 23    4/30/98 5:01p Peter
** first pass glide3 merge
** 
** 22    4/30/98 10:34a Peter
** merged w/ cvg again
** 
** 20    4/22/98 4:57p Peter
** glide2x merge
** 
** 19    3/30/98 3:28p Atai
** set gamma to 1.3
** 
** 18    3/13/98 4:03p Peter
** start glide3 merge
** 
** 17    3/02/98 7:26p Peter
** clear slop on sli systems when changing y origin
** 
** 16    2/23/98 11:44a Peter
** merged monitor detection and fixed compilation error from recent
** videobuffer changes
** 
** 15    2/12/98 3:40p Peter
** single buffering for opengl
** 
** 14    2/10/98 7:04p Atai
** fix grvertexlayout for migration dll
** 
** 13    2/05/98 3:07p Atai
** fix migration dll
** 
** 12    1/30/98 4:58p Atai
** fix gamma table parameters
** 
** 11    1/30/98 4:29p Peter
** no uswc for sli slave
** 
** 10    1/26/98 12:20p Atai
** fix grVertexLayout in grSplash()
** 
** 9     1/26/98 11:30a Atai
** update to new glide.h
** 
** 8     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
** 
** 7     1/20/98 11:03a Peter
** env var to force triple buffering
** 
** 6     1/18/98 12:03p Atai
** sync to rev 17 spec
 * 
 * 5     1/17/98 2:26p Atai
 * fix grvertexlayout
 * 
 * 4     1/17/98 1:12p Atai
 * set default as back buffer
 * 
 * 3     1/16/98 6:54p Atai
 * disable gamma table for now
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 116   1/16/98 4:18p Atai
 * fixed lfb and grLoadGammaTable
 * 
 * 115   1/16/98 10:47a Peter
 * fixed idle effage
 * 
 * 114   1/16/98 10:16a Atai
 * fixed grSstIldle
 * 
 * 113   1/10/98 4:01p Atai
 * inititialize vertex layout, viewport, added defines
 * 
 * 110   1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 109   1/06/98 3:53p Atai
 * remove grHint, modify grLfbWriteRegion and grGet
 * 
 * 107   12/18/97 2:12p Peter
 * grSstControl on v2
 * 
 * 106   12/17/97 4:48p Peter
 * groundwork for CrybabyGlide
 * 
 * 105   12/17/97 4:06p Atai
 * added grChromaRange(), grGammaCorrecionRGB(), grRest(), and grGet()
 * functions
 * 
 * 104   12/16/97 1:33p Atai
 * added grGammaCorrectionRGB()
 * 
 * 103   12/16/97 10:03a Atai
 * fixed gutexmemreset for glide2
 * 
 * 101   12/09/97 12:20p Peter
 * mac glide port
 * 
 * 100   12/05/97 4:26p Peter
 * watcom warnings
 * 
 * 99    12/03/97 11:35a Peter
 * is busy thing
 * 
 * 98    11/25/97 12:09p Peter
 * nested calls to grLfbLock vs init code locking on v2
 * 
 * 97    11/21/97 1:02p Peter
 * v^2 supported resolutions
 * 
 * 96    11/21/97 11:19a Dow
 * Added RESOLUTION_NONE hack for Banshee
 * 
 * 95    11/19/97 2:49p Peter
 * env vars in registry for win32
 * 
 * 94    11/19/97 2:22p Dow
 * gsst.c
 * 
 * 93    11/18/97 4:50p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 92    11/18/97 4:00p Atai
 * fixed the GR_BEGIN and GR_END error in previous check-in
 * 
 * 91    11/18/97 3:27p Atai
 * update vData 
 * optimize state monster
 * 
 * 90    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 89    11/16/97 2:20p Peter
 * cleanup
 * 
 * 88    11/15/97 7:43p Peter
 * more comdex silliness
 * 
 * 87    11/14/97 11:10p Peter
 * open vs hw init confusion
 * 
 * 86    11/14/97 5:02p Peter
 * more comdex stuff
 * 
 * 85    11/14/97 4:47p Dow
 * New splash screen
 * 
 * 84    11/14/97 12:09a Peter
 * comdex thing and some other stuff
 * 
 * 83    11/12/97 9:37p Dow
 * Banshee crap
 * 
 * 82    11/12/97 2:27p Peter
 * 
 * 81    11/12/97 1:09p Dow
 * H3 Stuf
 * 
 * 80    11/12/97 9:22a Dow
 * H3 Mods
 * 
 * 79    11/06/97 3:46p Peter
 * sli shutdown problem
 * 
 * 78    11/06/97 3:38p Dow
 * More banshee stuff
 * 
 * 77    11/04/97 5:04p Peter
 * cataclysm part deux
 * 
 * 76    11/04/97 3:58p Dow
 * Banshee stuff
 * 
 * 75    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 74    10/29/97 4:59p Peter
 * 
 * 73    10/29/97 2:45p Peter
 * C version of Taco's packing code
 * 
 * 72    10/23/97 5:28p Peter
 * sli fifo thing
 * 
 * 71    10/17/97 3:15p Peter
 * grSstVidMode thingee
 * 
 * 70    10/14/97 2:44p Peter
 * moved close flag set
 * 
 * 69    10/09/97 8:02p Dow
 * State Monster 1st Cut
 * 
**
*/

#include <stdio.h>
#include <string.h>
#include <3dfx.h>

#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if DRI_BUILD
#include <lindri.h>
#endif

#define kPageBoundarySlop 0x1000UL
#define kPageBoundaryMask (kPageBoundarySlop - 1)

/* Some forward declarations */
void _grImportFifo (int, int);

/* Init hw */

ResEntry
_resTable[] = {
  {GR_RESOLUTION_320x200, 320, 200}, /* 0x0 */
  {GR_RESOLUTION_320x240, 320,  240}, /*   0x1 */
  {GR_RESOLUTION_400x256, 400,  256}, /*   0x2 */
  {GR_RESOLUTION_512x384, 512,  384}, /*   0x3 */
  {GR_RESOLUTION_640x200, 640,  200}, /*   0x4 */
  {GR_RESOLUTION_640x350, 640,  350}, /*   0x5 */
  {GR_RESOLUTION_640x400, 640,  400}, /*   0x6 */
  {GR_RESOLUTION_640x480, 640,  480}, /*   0x7 */
  {GR_RESOLUTION_800x600, 800,  600}, /*   0x8 */
  {GR_RESOLUTION_960x720, 960,  720}, /*   0x9 */
  {GR_RESOLUTION_856x480, 856,  480}, /*   0xa */
  {GR_RESOLUTION_512x256, 512,  256}, /*   0xb */
  {GR_RESOLUTION_1024x768, 1024,  768}, /*  0xC */
  {GR_RESOLUTION_1280x1024, 1280,  1024}, /* 0xD */
  {GR_RESOLUTION_1600x1200, 1600,  1200}, /* 0xE */
  {GR_RESOLUTION_400x300, 400,  300}, /*   0xF */
  {GR_RESOLUTION_1152x864, 1152,  864}, /*  0x10 */
  {GR_RESOLUTION_1280x960, 1280,  960}, /*  0x11 */
  {GR_RESOLUTION_1600x1024, 1600,  1024}, /* 0x12 */
  {GR_RESOLUTION_1792x1344, 1792,  1344}, /* 0x13 */
  {GR_RESOLUTION_1856x1392, 1856,  1392}, /* 0x14 */
  {GR_RESOLUTION_1920x1440, 1920,  1440}, /* 0x15 */
  {GR_RESOLUTION_2048x1536, 2048,  1536}, /* 0x16 */
  {GR_RESOLUTION_2048x2048, 2048,  2048} /* 0x17 */
};

/* ---------------------------------------------
   This function both sets and documents the 
   expected default state for any rendering
   context

   ..taco - separated out in preparation for
   multiple contexts
   ---------------------------------------------*/
void 
assertDefaultState( void ) 
{
  GR_DCL_GC;
  
  grDisable(GR_ALLOW_MIPMAP_DITHER);
  grSstOrigin(gc->state.origin);
  grAlphaBlendFunction(GR_BLEND_ONE , GR_BLEND_ZERO, 
                       GR_BLEND_ONE, GR_BLEND_ZERO);
  grAlphaTestFunction(GR_CMP_ALWAYS);
  grAlphaTestReferenceValue(0);
  grChromakeyMode(GR_CHROMAKEY_DISABLE);
  grChromaRangeMode( GR_CHROMAKEY_DISABLE );
  grTexChromaMode( GR_TMU0, GR_TEXCHROMA_DISABLE_EXT );
  grConstantColorValue((FxU32) ~0);
  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_ITERATED,
                 GR_COMBINE_OTHER_ITERATED,
                 FXFALSE);
  grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_NONE,
                 GR_COMBINE_OTHER_CONSTANT,
                 FXFALSE);
  grColorMask(FXTRUE, FXFALSE);
  grCullMode(GR_CULL_DISABLE);
  grDepthBiasLevel(0);
  grDepthMask(FXFALSE);
  grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
  grDepthBufferFunction(GR_CMP_LESS);
  grDepthBiasLevel(0);
  grDitherMode(GR_DITHER_4x4);
  grFogMode(GR_FOG_DISABLE);
  grFogColorValue(0x00000000);
  
  /*
   ** initialize default state for viewport and grVertexLayout
   */
  grCoordinateSpace(GR_WINDOW_COORDS);
  grViewport(0, 0, gc->state.screen_width, gc->state.screen_height );
  
  switch (gc->num_tmu) {
  case 2:
    grTexClampMode(GR_TMU1, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
    grTexDetailControl(GR_TMU1, 0, 1, 1.0F);
    grTexFilterMode(GR_TMU1, GR_TEXTUREFILTER_POINT_SAMPLED, 
                    GR_TEXTUREFILTER_POINT_SAMPLED);
    grTexLodBiasValue(GR_TMU1, 0.0F);
    grTexMipMapMode(GR_TMU1, GR_MIPMAP_DISABLE, FXFALSE);
    grTexCombine(GR_TMU1, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 FXFALSE, FXFALSE);
    /* Intentional fallthrough */

  case 1:
    grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
    grTexDetailControl(GR_TMU0, 0, 1, 1.0F);
    grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, 
                    GR_TEXTUREFILTER_POINT_SAMPLED);
    grTexLodBiasValue(GR_TMU0, 0.0F);
    grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
    grTexCombine(GR_TMU0, 
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 FXFALSE, FXFALSE);
  }
  grLfbConstantAlpha(0xFF);
  grLfbConstantDepth(0);

  gc->triSetupProc = CUR_TRI_PROC(FXTRUE, (gc->state.cull_mode != GR_CULL_DISABLE));
} /* assertDefaultState */

#ifndef DRI_BUILD
static void 
clearBuffers( GrGC *gc ) 
{
  /* Get rid of crap in the buffers. */
  grClipWindow(0, 0, gc->state.screen_width, 
               gc->state.screen_height);
  if ( gc->state.num_buffers > 1 ) {
    grBufferClear( 0, 0, 0UL );
    grBufferSwap( 1 );
    grBufferClear( 0, 0, 0xFFFFFFFFUL );
    grBufferSwap( 1 );
    grBufferClear( 0, 0, 0xFFFFFFFFUL );
    grBufferSwap( 1 );
    grRenderBuffer( GR_BUFFER_BACKBUFFER );
  } else {
    grBufferClear( 0, 0, 0xFFFFFFFFUL );
    grRenderBuffer( GR_BUFFER_FRONTBUFFER );
  }
} /* clearBuffers */
#endif

static void 
doSplash( void ) 
{
#if defined(GLIDE_SPLASH) || ((GLIDE_PLATFORM & GLIDE_OS_WIN32) && defined(GLIDE_PLUG))
  GR_DCL_GC;
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32) && (defined(GLIDE_SPLASH)||defined(GLIDE_PLUG))
  {
    FxBool
      didLoad;

    if (gc->pluginInfo.moduleHandle == NULL) gc->pluginInfo.moduleHandle = LoadLibrary("3dfxspl3.dll");
    didLoad = (gc->pluginInfo.moduleHandle != NULL);
    if (didLoad) {
      gc->pluginInfo.initProc = (GrSplashInitProc)GetProcAddress(gc->pluginInfo.moduleHandle, 
                                                                 "_fxSplashInit@24");
      gc->pluginInfo.shutdownProc = (GrSplashShutdownProc)GetProcAddress(gc->pluginInfo.moduleHandle, 
                                                                         "_fxSplashShutdown@0");
      gc->pluginInfo.splashProc = (GrSplashProc)GetProcAddress(gc->pluginInfo.moduleHandle, 
                                                               "_fxSplash@20");
      gc->pluginInfo.plugProc = (GrSplashPlugProc)GetProcAddress(gc->pluginInfo.moduleHandle, 
                                                                 "_fxSplashPlug@16");

      didLoad = ((gc->pluginInfo.initProc != NULL) &&
                 (gc->pluginInfo.splashProc != NULL) &&
                 (gc->pluginInfo.plugProc != NULL) &&
                 (gc->pluginInfo.shutdownProc != NULL));
      if (didLoad) {
        GrState glideState;

        /* Protect ourselves from the splash screen */
        grGlideGetState(&glideState);
        {
          didLoad = (*gc->pluginInfo.initProc)(gc->grHwnd,
                                               gc->state.screen_width, gc->state.screen_height,
                                               gc->grColBuf, gc->grAuxBuf,
                                               gc->state.color_format);
          if (!didLoad) (*gc->pluginInfo.shutdownProc)();
        }
        grGlideSetState((const void*)&glideState);
      }
      
      if (!didLoad) FreeLibrary(gc->pluginInfo.moduleHandle);
    }

    /* Clear all the info if we could not load for some reason */
    if (!didLoad) memset(&gc->pluginInfo, 0, sizeof(gc->pluginInfo));
  }
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */

#ifdef GLIDE_SPLASH
  if (_GlideRoot.environment.noSplash == 0) {
    grSplash(0.0f, 0.0f, 
             (float)gc->state.screen_width,
             (float)gc->state.screen_height,
             0);
  }
  _GlideRoot.environment.noSplash = 1;
#endif
} /* doSplash */


/*----------------------------------------------------
  Return a GC to reset state
  
  ...taco - separated out as a first pass since this will
  be common function per context
  ----------------------------------------------------*/
void 
initGC ( GrGC *gc ) 
{
#define FN_NAME "initGC"
  FxI32 t = 0;

  GDBG_INFO(95, FN_NAME"(0x%X)\n", gc);
  
  /* Setup the indices of the logical buffers */
#if DRI_BUILD
  gc->curBuffer   = (gc->grColBuf > 1) ? 1 : 0;
  gc->frontBuffer = 0;
#else
  gc->curBuffer   = 0;
  gc->frontBuffer = ((gc->grColBuf > 1) ? 1 : 0);
#endif
  gc->backBuffer  = (gc->grColBuf > 2) ? 2 : gc->curBuffer;
  
  for (t = 0; t < 7; t++) {
    gc->bufferSwaps[t] = 0xffffffff;
  }
  
  gc->bufferSwaps[0] = (unsigned long) gc->cmdTransportInfo.fifoPtr -
                       (unsigned long) gc->cmdTransportInfo.fifoStart;
  
  gc->swapsPending = 1;
  
  gc->lockPtrs[GR_LFB_READ_ONLY]  = (FxU32)-1;
  gc->lockPtrs[GR_LFB_WRITE_ONLY] = (FxU32)-1;
  
  /* initialize command packet headers for state 
     restore buffer */
  gc->state.shadow.pkt4Hdr_0 = 
    0x1 << 11 | /* note, don't send chromarange to tmu... */
    ( SR_MASK_0 << SSTCP_PKT4_MASK_SHIFT ) |
    SSTCP_REGBASE_FROM_ADDR( SR_ADDR_0 )   |
    SSTCP_PKT4;
  
  gc->state.shadow.pkt4Hdr_1 =
    ( SR_MASK_1 << SSTCP_PKT4_MASK_SHIFT ) |
    SSTCP_REGBASE_FROM_ADDR( SR_ADDR_1 )   |
    SSTCP_PKT4;
  
  gc->state.shadow.pkt1Hdr_2 = 
    ( SR_WORDS_2 << SSTCP_PKT1_NWORDS_SHIFT ) |
    SSTCP_INC |
    SSTCP_REGBASE_FROM_ADDR( SR_ADDR_2 )   |
    SSTCP_PKT1;
  
  gc->state.shadow.pkt4Hdr_3 =
    ( SR_MASK_3 << SSTCP_PKT4_MASK_SHIFT ) |
    SSTCP_REGBASE_FROM_ADDR( SR_ADDR_3 )   |
    SSTCP_PKT4;  
  
  for( t = 0; t < 32; t++ ) {
    gc->state.shadow.paletteRow[t].pkt1Hdr_P = 
      ( SR_WORDS_P << SSTCP_PKT1_NWORDS_SHIFT ) |
      SSTCP_INC |
      SSTCP_REGBASE_FROM_ADDR( SR_ADDR_P )   |
      SSTCP_PKT1;
  }
  
  /* Initialize the read/write registers to all 0 */
  gc->state.shadow.fbzColorPath  = 0;
  gc->state.shadow.fogMode       = 0;
  gc->state.shadow.alphaMode     = 0;
  gc->state.shadow.fbzMode       = 0;
  gc->state.shadow.lfbMode       = 0;
  gc->state.shadow.clipLeftRight = 0;
  gc->state.shadow.clipBottomTop = 0;
  gc->state.shadow.fogColor      = 0;
  gc->state.shadow.zaColor       = 0;
  gc->state.shadow.chromaKey     = 0;
  gc->state.shadow.stipple       = 0;
  gc->state.shadow.color0        = 0;
  gc->state.shadow.color1        = 0;

  /* NB: This loop has to setup the packet headers for *ALL* of the
   * tmu persistent state. Mess this up at your own peril because
   * someone who cares about running glide in a window will come and
   * kill you as you sit in your cube.
   */
  for (t = 0; t < GLIDE_NUM_TMU; t ++) {
    gc->state.shadow.tmuState[t].texPkt4Hdr_0 = (( SR_MASK_4 << SSTCP_PKT4_MASK_SHIFT ) |
                                                 FIFO_REG((0x02UL << t), textureMode) |
                                                 SSTCP_PKT4);

    gc->state.shadow.tmuState[t].textureMode     = 0x00000000;
    gc->state.shadow.tmuState[t].tLOD            = 0x00000000;
    gc->state.shadow.tmuState[t].tDetail         = 0x00000000;
    gc->state.shadow.tmuState[t].texBaseAddr     = 0x00000000;
    gc->state.shadow.tmuState[t].texBaseAddr_1   = 0x00000000;
    gc->state.shadow.tmuState[t].texBaseAddr_2   = 0x00000000;
    gc->state.shadow.tmuState[t].texBaseAddr_3_8 = 0x00000000;
    
    gc->state.shadow.tmuState[t].texPkt4Hdr_1 =
      ( SR_MASK_5 << SSTCP_PKT4_MASK_SHIFT ) |
      FIFO_REG((0x02UL << t), chromaKey) |
      SSTCP_PKT4;
    
    gc->state.shadow.tmuState[t].texPkt1Hdr_2 = 
      ( SR_WORDS_6 << SSTCP_PKT1_NWORDS_SHIFT ) |
      SSTCP_INC |
      FIFO_REG((0x02UL << t), nccTable0) |
      SSTCP_PKT1;

    gc->tmuMemInfo[t].prePacket[0] = ((0x01UL << SSTCP_PKT1_NWORDS_SHIFT) |
                                      FIFO_REG_WAX(command) |
                                      SSTCP_PKT1);
    gc->tmuMemInfo[t].prePacket[1] = (SSTG_NOP + SSTG_GO);

    gc->tmuMemInfo[t].postPacket[0] = ((0x01UL << SSTCP_PKT1_NWORDS_SHIFT) |
                                       FIFO_REG((0x02UL << t), texBaseAddr) |
                                       SSTCP_PKT1);
    gc->tmuMemInfo[t].postPacket[1] = ~gc->state.shadow.tmuState[t].texBaseAddr;
    gc->tmuMemInfo[t].postPacket[2] = ((0x01UL << SSTCP_PKT1_NWORDS_SHIFT) |
                                       FIFO_REG(BROADCAST_ID, nopCMD) |
                                       SSTCP_PKT1);
    gc->tmuMemInfo[t].postPacket[3] = 0x00UL;
    gc->tmuMemInfo[t].postPacket[4] = ((0x01UL << SSTCP_PKT1_NWORDS_SHIFT) |
                                       FIFO_REG((0x02UL << t), texBaseAddr) |
                                       SSTCP_PKT1);
    gc->tmuMemInfo[t].postPacket[5] = gc->state.shadow.tmuState[t].texBaseAddr;
    gc->tmuMemInfo[t].postPacket[6] = ((0x01UL << SSTCP_PKT1_NWORDS_SHIFT) |
                                       FIFO_REG_WAX(command) |
                                       SSTCP_PKT1);
    gc->tmuMemInfo[t].postPacket[7] = (SSTG_NOP + SSTG_GO);

    /* This is the signal to flush */
    gc->tmuMemInfo[t].flushCount = 1;

    gc->state.per_tmu[t].mmMode   = GR_MIPMAP_NEAREST;
    gc->state.per_tmu[t].smallLod = G3_LOD_TRANSLATE(GR_LOD_LOG2_1);
    gc->state.per_tmu[t].largeLod = G3_LOD_TRANSLATE(GR_LOD_LOG2_1);
    gc->state.per_tmu[t].s_scale  = 256.f;
    gc->state.per_tmu[t].t_scale  = 256.f;
    gc->state.per_tmu[t].evenOdd  = GR_MIPMAPLEVELMASK_BOTH;
    gc->state.per_tmu[t].nccTable = GR_NCCTABLE_NCC0;
  } 
#undef FN_NAME
} /* initGC */

/*-------------------------------------------------------------------
  Function: grSstWinOpen
  Date: 3/16
  Implementor(s): dow, gmt, murali, jdt
  Mutator: dpc
  Library: Glide
  Description:

  Initialize the selected SST
     
  Arguments:
  hwnd - pointer to a window handle or null.  If NULL, then 
         the application window handle will be inferred though
         the GetActiveWindow() api.
  resolution - either one of the pre-defined glide resolutions,
               or GR_RESOLUTION_NONE, in which case the window
               size is inferred from the size application window
  refresh - requested fullscreen refresh rate, ignored in a window
  format  - requested ccolor format for glide packed color values
  origin  - location of coordinate origin either upper left or
                    lower right
  nColBuffers - number of color buffers to attempt to allocate
                0 - meaningless
                1 - allocate a front buffer only
                2 - allocate a front and back buffer
                3 - allocate a front, back, aux buffer for tripple buffering
  nAuxBuffers - number of aux buffers to attempt to allocate
                0 - no alpha or z buffers
                1 - allocate one aux buffer for alpha/depth buffering
                2 - allocate on depth and one alpha buffer (unsup)
  Return:
  NULL - glide was unable to create a fullscreen context
  context handle - glide was able to create a context with handle H
  -------------------------------------------------------------------*/
GR_ENTRY(grSstWinOpen, GrContext_t, ( FxU32                   hWnd,
                                      GrScreenResolution_t    resolution, 
                                      GrScreenRefresh_t       refresh, 
                                      GrColorFormat_t         format, 
                                      GrOriginLocation_t      origin, 
                                      int                     nColBuffers,
                                      int                     nAuxBuffers) )
{
#define FN_NAME "grSstWinOpen"
#define TILE_WIDTH_PXLS   64
#define TILE_HEIGHT_PXLS  32
#define BYTES_PER_PIXEL   2
#define MIN_TEXTURE_STORE 0x200000
#define MIN_FIFO_SIZE     0x10000
#if defined( GLIDE_INIT_HWC )
  hwcBoardInfo  *bInfo   = 0;
  hwcVidInfo    *vInfo   = 0;
  hwcBufferInfo *bufInfo = 0;
  hwcFifoInfo   *fInfo   = 0;
#elif defined( GLIDE_INIT_HAL )
  FxDeviceInfo   devInfo;
#endif /* defined ( GLIDE_INIT_HAL ) */
  int            buffer;
  struct cmdTransportInfo *gcFifo = 0;
  GrContext_t retVal = 0;

#if !defined(__linux__) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
  if (!hWnd)
    GrErrorCallback("grSstWinOpen: need to use a valid window handle",
                    FXTRUE);
#endif

  /* NB: TLS must be setup before the 'declaration' which grabs the
   * current gc. This gc is valid for all threads in the fullscreen
   * context.
   */
  setThreadValue( (unsigned long)&_GlideRoot.GCs[_GlideRoot.current_sst] );
  
  {
    /* Partial Argument Validation */
    GR_BEGIN_NOFIFOCHECK_NORET("grSstWinOpen",80);
    GDBG_INFO_MORE(gc->myLevel,
                   "(rez=%d,ref=%d,cformat=%d,origin=%s,#bufs=%d, #abufs=%d)\n",
                   resolution,refresh,format,
                   origin ? "LL" : "UL",
                   nColBuffers, nAuxBuffers);
    GR_CHECK_F(FN_NAME, !gc, "no SST selected as current (gc==NULL)");
  
    /*
    ** check if the environment variable for triple buffering is on
    */
    if (_GlideRoot.environment.nColorBuffer != -1) {
      if ((_GlideRoot.environment.nColorBuffer > 0) &&
          (_GlideRoot.environment.nColorBuffer <= 3)
          ){
        nColBuffers = _GlideRoot.environment.nColorBuffer;
      }
    }
    
    resolution =
      (((FxU32)resolution) > (sizeof(_resTable) / sizeof(ResEntry)))
        ? GR_RESOLUTION_640x480 
          : resolution;

#if DRI_BUILD
    gc->state.screen_width = driInfo.screenWidth;
    gc->state.screen_height = driInfo.screenHeight;
#else
    gc->state.screen_width  = _resTable[resolution].xres;
    gc->state.screen_height = _resTable[resolution].yres;

    GR_CHECK_F( FN_NAME, 
                resolution != _resTable[resolution].resolution, 
                "resolution table compilation incorrect" );
    if ( gc->vidTimings ) {
      gc->state.screen_width  = gc->vidTimings->xDimension;
      gc->state.screen_height = gc->vidTimings->yDimension;
    }
#endif
  
    gc->state.origin             = origin;
    gc->grSstRez                 = resolution;
    gc->grSstRefresh             = refresh;
    gc->grColBuf                 = gc->state.num_buffers = nColBuffers;
    gc->grAuxBuf                 = nAuxBuffers;
    gc->fbStride                 = gc->state.screen_width * BYTES_PER_PIXEL;
    gc->grHwnd                   = (int) hWnd;
  
    /* compute tile dimensions */
    gc->strideInTiles  = ( gc->state.screen_width  + ( TILE_WIDTH_PXLS - 1 ) ) / TILE_WIDTH_PXLS;
    GDBG_INFO(80, "%s: strideInTiles = 0X%x\n", FN_NAME, gc->strideInTiles);
    gc->heightInTiles  = ( gc->state.screen_height + ( TILE_HEIGHT_PXLS - 1 ) ) / TILE_HEIGHT_PXLS;
    GDBG_INFO(80, "%s: heightInTiles = 0x%x\n", FN_NAME, gc->heightInTiles);
    gc->bufferStride   = gc->strideInTiles * TILE_WIDTH_PXLS * BYTES_PER_PIXEL;
    GDBG_INFO(80, "%s: bufferStride = 0x%x\n", FN_NAME, gc->bufferStride);
    gc->bufSizeInTiles = gc->strideInTiles * gc->heightInTiles;
    GDBG_INFO(80, "%s: bufSizeInTiles = 0x%x\n", FN_NAME, gc->bufSizeInTiles);
    gc->bufSize = gc->bufSizeInTiles * TILE_WIDTH_PXLS * TILE_HEIGHT_PXLS * BYTES_PER_PIXEL;
    GDBG_INFO(80, "%s: bufSize = 0x%x\n", FN_NAME, gc->bufSize);
  
    /* Check for enough memory */
#ifdef GLIDE_INIT_HWC
    if ( ( gc->bufSize * ( gc->grColBuf + gc->grAuxBuf ) + MIN_TEXTURE_STORE + MIN_FIFO_SIZE) >
         ( gc->bInfo->h3Mem << 20 ) ) {
      GDBG_INFO( gc->myLevel, "Failed to open for insufficient memory\n" );
      GrErrorCallback( "grSstWinOpen: not enough memory for requested buffers", FXFALSE );
      return 0;
    }
#endif
  
    /* Allocate Color/Aux Buffers, Set Memory Layout */
    gcFifo = &gc->cmdTransportInfo;
  
#if defined( USE_PACKET_FIFO )
#if defined( GLIDE_INIT_HWC )
    bInfo   = gc->bInfo;
    vInfo   = &bInfo->vidInfo;
    bufInfo = &bInfo->buffInfo;

    /* If we closed down then the hw may have been un-mapped (on
     * systems that actually support this) so we need to re-map the
     * board and re-cache our hw pointers.  
     */
    if (!gc->hwInitP) {
      if (!hwcMapBoard(bInfo, HWC_BASE_ADDR_MASK)) {
        GDBG_INFO( gc->myLevel, "Failed to re-map the hw.\n" );
        GrErrorCallback( FN_NAME": Failed to re-map the hw.", FXFALSE );
        GR_RETURN( FXFALSE );
      }

      if (!hwcInitRegisters(bInfo)) {
        GDBG_INFO( gc->myLevel, "Failed to re-initialize the hw.\n" );
        GrErrorCallback( FN_NAME": Failed to re-initialize the hw.", FXFALSE );
        GR_RETURN( FXFALSE );
      }

      gc->sstRegs = (SstRegs*)bInfo->regInfo.sstBase;
      gc->ioRegs  = (SstIORegs*)bInfo->regInfo.ioMemBase;
      gc->cRegs   = (SstCRegs*)bInfo->regInfo.cmdAGPBase;
      gc->lfb_ptr = (FxU32*)bInfo->regInfo.lfbBase;
      gc->rawLfb  = (FxU32*)bInfo->regInfo.rawLfbBase;
      gc->tex_ptr = (FxU32*)SST_TEX_ADDRESS(bInfo->regInfo.sstBase);

      gc->hwInitP = FXTRUE;
    }

#if DRI_BUILD
    vInfo->xRes	       = driInfo.w;
    vInfo->yRes	       = driInfo.h;
#else
    vInfo->xRes        = gc->state.screen_width;
    vInfo->yRes        = gc->state.screen_height;
#endif
    vInfo->refresh     = gc->grSstRefresh;
    vInfo->tiled       = FXTRUE;
    vInfo->initialized = FXTRUE;
  
    if ( hwcAllocBuffers( bInfo, nColBuffers, nAuxBuffers ) == FXFALSE ) {
      GDBG_INFO( gc->myLevel, "hwcAllocBuffers failed\n" );
      GrErrorCallback(hwcGetErrorString(), FXFALSE);
      return 0;
    }
  
    for (buffer = 0; buffer < nColBuffers; buffer++) {
      gc->buffers[buffer] = bufInfo->colBuffStart[buffer];
      GDBG_INFO(80, "Buffer %d:  Start: 0x%x\n", buffer, gc->buffers[buffer]);
      gc->lfbBuffers[buffer] = (unsigned long)gc->rawLfb + bufInfo->lfbBuffAddr[buffer];
    }
    if (nAuxBuffers != 0) {
      gc->buffers[buffer] = bufInfo->auxBuffStart;
      GDBG_INFO(80, "Aux Buffer:  Start: 0x%x\n", gc->buffers[buffer]);
      gc->lfbBuffers[buffer] = (unsigned long)gc->rawLfb + bufInfo->lfbBuffAddr[buffer];
    }
  
    vInfo->hWnd     = gc->grHwnd;
    vInfo->sRes     = gc->grSstRez;
    vInfo->vRefresh = gc->grSstRefresh;
  
    if ( hwcInitVideo( bInfo, FXTRUE, gc->vidTimings, FXTRUE ) == FXFALSE ) {
      GrErrorCallback(hwcGetErrorString(), FXFALSE);
      GDBG_INFO( gc->myLevel, "hwcInitVideo failed\n" );
      return 0;
    }

    /* Restore the function specializations if the user is trying to
     * recover. This only resets the non-null environment. The actual
     * function specializations are recovered later in the mainline
     * code path for the open.
     */
    if (gc->open && !gc->contextP) {
      GrTriSetupProcArchVector* 
        curTriProcs = _GlideRoot.deviceArchProcs.curTriProcs;
      GrVertexListProc*
        curVertexListProcs = _GlideRoot.deviceArchProcs.curVertexListProcs;
      
      _GlideRoot.deviceArchProcs.curTriProcs        = _GlideRoot.deviceArchProcs.nullTriProcs;
      _GlideRoot.deviceArchProcs.curVertexListProcs = _GlideRoot.deviceArchProcs.nullVertexListProcs;
      
      _GlideRoot.deviceArchProcs.nullTriProcs        = curTriProcs;
      _GlideRoot.deviceArchProcs.nullVertexListProcs = curVertexListProcs;
    }

    /* This actually gets taken in hwcInitVideo */
    gc->contextP = FXTRUE;

#if GLIDE_CHECK_CONTEXT
    /* CSR - Set up flag for display driver to tell us that context was lost */
	if ( !gc->open )  /* If we already have a context open, then lets not
					     re-initialize the pointers                          */						
	{
	  hwcShareContextData(gc->bInfo, &(gc->lostContext));
	  gc->cmdTransportInfo.ptrLostContext = &(gc->lostContext);
	}

    *gc->lostContext = FXFALSE;
#endif

    if (_GlideRoot.environment.gammaR != -1.f &&
        _GlideRoot.environment.gammaG != -1.f &&
        _GlideRoot.environment.gammaB != -1.f) {
      hwcGammaRGB(gc->bInfo, 
                  _GlideRoot.environment.gammaR, 
                  _GlideRoot.environment.gammaG,
                  _GlideRoot.environment.gammaB);
    } else {
      hwcGammaRGB(gc->bInfo, 1.3f, 1.3f, 1.3f);
    }

    /* Setup memory configuration */
    gc->fbOffset = bInfo->fbOffset;
    switch (gc->num_tmu) {
    case 2:
      if (gc->state.grEnableArgs.texture_uma_mode == GR_MODE_ENABLE) {
        gc->tmuMemInfo[0].tramOffset = gc->tmuMemInfo[1].tramOffset = bInfo->tramOffset;
        gc->tmuMemInfo[0].tramSize = gc->tmuMemInfo[1].tramSize = bInfo->tramSize;
        gc->tmu_state[0].total_mem = gc->tmu_state[1].total_mem = gc->tmuMemInfo[0].tramSize;
      }
      else {
        gc->tmuMemInfo[0].tramOffset = bInfo->tramOffset;
        
        /*
        ** if the environment variable is on, use the its texture memory size * 2
        */
        if ((_GlideRoot.environment.tmuMemory != -1) && (gc->fbOffset >= 0x200000))
          bInfo->tramSize = _GlideRoot.environment.tmuMemory << 21;
        
        /*
        ** when tmu size is 2, split the texture memory for each tmu
        */
        gc->tmuMemInfo[0].tramSize = bInfo->tramSize >> 1;
        gc->tmu_state[0].total_mem = gc->tmuMemInfo[0].tramSize;
        
        gc->tmuMemInfo[1].tramOffset = gc->tmuMemInfo[0].tramOffset + gc->tmuMemInfo[0].tramSize;
        gc->tmuMemInfo[1].tramSize   = (bInfo->tramSize >> 1);
        gc->tmu_state[1].total_mem   = gc->tmuMemInfo[1].tramSize;
      }
      break;

    case 1:
    default:
      gc->tmuMemInfo[0].tramOffset = bInfo->tramOffset;

      /*
      ** if the environment variable is on, use the its texture memory size
      */
      if ((_GlideRoot.environment.tmuMemory != -1) && (gc->fbOffset >= 0x200000))
        bInfo->tramSize = _GlideRoot.environment.tmuMemory << 20;

      gc->tmuMemInfo[0].tramSize = bInfo->tramSize;
      gc->tmu_state[0].total_mem = gc->tmuMemInfo[0].tramSize;
      break;
    }
    
    gc->tBuffer.bufType = HWC_BUFFER_TEXTUREBUFFER;
    gc->tBuffer.bufOffset = bInfo->tramOffset;
    gc->tBuffer.bufSize = bInfo->tramSize;
    gc->tBuffer.tiled = FXFALSE;
    gc->tBuffer.bufBPP = 0xffffffff; /* Don't matter to me */

    GDBG_INFO(1, "autoBump: 0x%x\n", _GlideRoot.environment.autoBump);
    if ((gc->cmdTransportInfo.autoBump = _GlideRoot.environment.autoBump)) {
      if (!hwcInitFifo( bInfo, gc->cmdTransportInfo.autoBump)) {
        hwcRestoreVideo(bInfo);
        GrErrorCallback(hwcGetErrorString(), FXFALSE);
        GDBG_INFO(gc->myLevel, "hwcInitFifo failed\n");
        GR_RETURN(FXFALSE);
      }
    } else {
      if (!hwcInitAGPFifo(bInfo, FXTRUE)) {
        hwcRestoreVideo(bInfo);
        GrErrorCallback(hwcGetErrorString(), FXFALSE);
        GDBG_INFO(gc->myLevel, "hwcInitFifo failed\n");
        GR_RETURN(FXFALSE);
      }
      
      /* Check to see where the command fifo was placed since the agp
       * allocation might have failed for some reason, and fallen back
       * to using the normal video command fifo.
       */
      gc->cmdTransportInfo.autoBump = ((GR_CAGP_GET(baseSize) & SST_CMDFIFO_DISABLE_HOLES) == 0);
    }
  
    /* COMMAND FIFO SETUP */
    fInfo  = &gc->bInfo->fifoInfo;
  
    /* Establish physical bounds of cmd fifo from HWC calculation */
    gcFifo->fifoOffset = fInfo->fifoStart;
    gcFifo->fifoSize   = fInfo->fifoLength;
#elif defined( GLIDE_INIT_HAL ) 
#if 0
    gc->fbOffset               = 0x200000;
    gc->tramOffset             = 0x0;
    gc->tramSize             = gc->fbOffset;
    gc->tmu_state[0].total_mem = gc->tramSize;
#else
    /* gc->fbOffset               = (FxU32)fxHalFbiGetMemory((SstRegs*)gc->reg_ptr); */
    gc->fbOffset                  = (unsigned long)gc->rawLfb;
    gc->tmuMemInfo[0].tramOffset  = 0x200000;
    gc->tmuMemInfo[0].tramSize    = 0x200000;
    gc->tmuMemInfo[1].tramOffset  = gc->tmuMemInfo[0].tramSize + gc->tmuMemInfo[0].tramOffset;
    gc->tmuMemInfo[1].tramSize    = 0x200000;
    gc->tmu_state[0].total_mem    = gc->tmuMemInfo[0].tramSize;
    gc->tmu_state[1].total_mem    = gc->tmuMemInfo[1].tramSize;
#endif
  
    for ( buffer = 0; buffer < nColBuffers; buffer++ ) {
      gc->buffers[buffer]    = gc->fbOffset + buffer * gc->bufSize;
      /* XXXjdt: this is never initialized in the old code */
      gc->lfbBuffers[buffer] = 0; 
      GDBG_INFO(80, "%s: Buffer %d: 0x%x\n",
                FN_NAME, buffer, gc->buffers[buffer]);
    }
    if( !fxHalGetDeviceInfo((SstRegs*)gc->reg_ptr, &devInfo) ) {
      GrErrorCallback("  XXXGetDeviceInfo failed.\n", FXFALSE);
      GDBG_INFO( gc->myLevel, 
                 "   XXXGetDeviceInfo failed. (0x%x)\n", 
                 gc->reg_ptr );
      return 0;
    }
    
    /* COMMAND FIFO SETUP */
#if 0
    gcFifo->fifoOffset = gc->fbOffset + 
        ( gc->bufSize * ( gc->grColBuf + gc->grAuxBuf ) );
    gcFifo->fifoSize = 0x400000 - gcFifo->fifoOffset + gc->fbOffset;
#else
    gcFifo->fifoOffset = ( gc->bufSize * ( gc->grColBuf + gc->grAuxBuf ) );
    gcFifo->fifoSize = 0x20000;
#endif
    
    if ( !fxHalInitCmdFifo((SstRegs *) gc->reg_ptr,
                           0,     /* which fifo - 0 for 3d cmd fifo */
                           /* v fifoStart - offset from hw base v */
                           gcFifo->fifoOffset, 
                           gcFifo->fifoSize, /* size - in bytes */ 
                           FXTRUE, /* directExec */
                           FXFALSE, /* disableHoles */
                           FXFALSE) /* agpEnable */ ) {
#ifdef GLIDE_INIT_HWC
      GrErrorCallBack( "fxHalInitCmdFifo failed.\n", FxFALSE );
#endif
      GDBG_INFO( 0, "Error: fxHalInitCmdFifo failed\n" );
      return 0;
    }
    if ( !fxHalInitVideo( (SstRegs*) gc->reg_ptr,
                          (resolution == GR_RESOLUTION_NONE) ? GR_RESOLUTION_640x480
                          : (resolution), 
                          refresh, 
                          NULL ) ) {
#ifdef GLIDE_INIT_HWC
        GrErrorCallBack( "fxHalInitVideo failed.\n", FxFALSE );
#endif
        GDBG_INFO( 0, "Error: fxHalInitVideo failed\n" );
        GR_RETURN( FXFALSE );
    }
    fxHalInitVideoOverlaySurface( (SstRegs*) gc->reg_ptr,     /* SstRegs */
                                  FXTRUE,                     /* 1=enable Overlay surface*/
                                  FXFALSE,                    /* 1=enable OS stereo, 0=disable*/
                                  FXFALSE,                    /* 1=enable horizontal*/
                                  0,                          /* horizontal scale factor (ignored if*/
                                  FXFALSE,                    /* 1=enable vertical scaling,*/
                                  0,                          /* vertical scale factor (ignored if not*/
                                  0,                          /* duh*/
                                  1,                          /* 0=OS linear, 1=tiled*/
                                  SST_OVERLAY_PIXEL_RGB565U,  /* pixel format of OS*/
                                  FXFALSE,                    /* bypass clut for OS?*/
                                  FXFALSE,                    /* 0=lower 256 CLUT entries,*/
                                  gc->buffers[gc->curBuffer], /* board address of beginning of OS  */
                                  gc->strideInTiles );        /* distance between scanlines of the OS, in*/

    gc->contextP = FXTRUE;

#endif /*  defined( GLIDE_INIT_HAL )  */
#else  /* !defined( USE_PACKET_FIFO ) */
    gc->fbOffset               = 0x0;
    gc->tramOffset[0]          = 0x200000;
    gc->tramSize[0]            = 0x200000;
    gc->tramOffset[1]          = gc->tramSize[0] + gc->tramOffset[0];
    gc->tramSize[1]            = 0x200000;
#if 0
    gc->tmu_state[0].total_mem = gc->tramSize;
    gc->fbOffset               = 0x200000;
    gc->tramOffset             = 0x0;
    gc->tramSize               = gc->fbOffset;
    gc->tmu_state[0].total_mem = gc->tramSize;

    for ( buffer = 0; buffer < nColBuffers; buffer++ ) {
      gc->buffers[buffer]    = gc->fbOffset + buffer * gc->bufSize;
      /* XXXjdt: this is never initialized in the old code */
      gc->lfbBuffers[buffer] = 0; 
      GDBG_INFO(80, "%s: Buffer %d: 0x%x\n",
                FN_NAME, buffer, gc->buffers[buffer]);
    }
#endif
    if( !fxHalGetDeviceInfo((SstRegs*)gc->reg_ptr, &devInfo) ) {
        GrErrorCallback("  XXXGetDeviceInfo failed.\n", FXFALSE);
        GDBG_INFO( gc->myLevel, 
                   "   XXXGetDeviceInfo failed. (0x%x)\n", 
                   gc->reg_ptr );
        GR_RETURN( 0 );
    }
#if 0
    /* COMMAND FIFO SETUP */
    gcFifo->fifoOffset = gc->fbOffset + 
        ( gc->bufSize * ( gc->grColBuf + gc->grAuxBuf ) );
    gcFifo->fifoSize = 0x400000 - gcFifo->fifoOffset + gc->fbOffset;

    if ( !fxHalInitCmdFifo((SstRegs *) gc->reg_ptr,
                           0, /* which fifo - 0 for 3d cmd fifo */
                           /* v fifoStart - offset from hw base v */
                           gcFifo->fifoOffset, 
                           gcFifo->fifoSize, /* size - in bytes */ 
                           FXTRUE, /* directExec */
                           FXFALSE, /* disableHoles */
                           FXFALSE) /* agpEnable */ ) {
#ifdef FX_FAIL_HWC
        GrErrorCallBack( "fxHalInitCmdFifo failed.\n", FxFALSE );
#endif
        GDBG_INFO( 0, "Error: fxHalInitCmdFifo failed\n" );
        GR_RETURN( 0 );
    }
#endif
    if ( !fxHalInitVideo( (SstRegs*) gc->reg_ptr,
                          (resolution == GR_RESOLUTION_NONE) ? GR_RESOLUTION_640x480
                          : (resolution), 
                          refresh, 
                          NULL ) ) {
        GR_RETURN( 0 );
    }
    fxHalInitVideoOverlaySurface( (SstRegs*) gc->reg_ptr,     /* SstRegs */
                                  FXTRUE,                     /* 1=enable Overlay surface*/
                                  FXFALSE,                    /* 1=enable OS stereo, 0=disable*/
                                  FXFALSE,                    /* 1=enable horizontal*/
                                  0,                          /* horizontal scale factor (ignored if*/
                                  FXFALSE,                    /* 1=enable vertical scaling,*/
                                  0,                          /* vertical scale factor (ignored if not*/
                                  0,                          /* duh*/
                                  1,                          /* 0=OS linear, 1=tiled*/
                                  SST_OVERLAY_PIXEL_RGB565U,  /* pixel format of OS*/
                                  FXFALSE,                    /* bypass clut for OS?*/
                                  FXFALSE,                    /* 0=lower 256 CLUT entries,*/
                                  gc->buffers[gc->curBuffer], /* board address of beginning of OS  */
                                  gc->strideInTiles );        /* distance between scanlines of the OS, in*/
    _grReCacheFifo(0);
#endif /* !defined( USE_PACKET_FIFO ) */
  
    /* Compute Virtual FIFO address extents */
    gcFifo->fifoStart = gc->rawLfb + ( gcFifo->fifoOffset >> 2 );
    gcFifo->fifoEnd   = gcFifo->fifoStart + ( gcFifo->fifoSize >> 2 );
  
    /* Adjust room values. 
    ** RoomToEnd needs enough room for the jmp packet since we never
    ** allow the hw to auto-wrap. RoomToRead needs to be adjusted so that
    ** we never acutally write onto the read ptr.
    **
    ** fifoRoom is generally the min of roomToEnd and roomToRead, but we
    ** 'know' here that roomToRead < roomToEnd.   
    */
    gcFifo->roomToEnd = gcFifo->fifoSize - FIFO_END_ADJUST;
    gcFifo->fifoRoom  = gcFifo->roomToReadPtr = gcFifo->roomToEnd - sizeof( FxU32 );
    
    /* Set initial fifo state. hw read and sw write pointers at
     * start of the fifo.
     */
    gcFifo->fifoPtr  = gcFifo->fifoStart;
    gcFifo->fifoRead = HW_FIFO_PTR( FXTRUE );

#if !DRI_BUILD
    if ( (void*)gcFifo->fifoPtr != (void*)gcFifo->fifoRead ) {
#ifdef GLIDE_INIT_HWC
      hwcRestoreVideo( bInfo );
#endif
      GDBG_INFO( gc->myLevel, "Initial fifo state is incorrect\n" );
      return 0;
    }
#endif
    
    if (!gc->cmdTransportInfo.autoBump) {
      gcFifo->bumpSize = _GlideRoot.environment.bumpSize;

      gcFifo->lastBump = gcFifo->fifoPtr;
      gcFifo->bumpPos = gcFifo->fifoPtr + gcFifo->bumpSize;

      gcFifo->fifoJmpHdr[0] = 
        ( SSTCP_PKT0_JMP_AGP |
          ( gcFifo->fifoOffset << ( SSTCP_PKT0_ADDR_SHIFT - 2 )));
      gcFifo->fifoJmpHdr[1] = (gcFifo->fifoOffset >> 25);


    } else {
      gcFifo->fifoJmpHdr[0] = 
        ( SSTCP_PKT0_JMP_LOCAL |
          ( gcFifo->fifoOffset << ( SSTCP_PKT0_ADDR_SHIFT - 2 )));


    }

    GDBG_INFO(80, "Command Fifo:\n"
              "\tfifoStart:       0x%x\n"
              "\tfifoEnd:         0x%x\n"
              "\tfifoOffset:      0x%x\n"
              "\tfifoSize:        0x%x\n"
              "\tfifoPtr:         0x%x\n",
              gcFifo->fifoStart, 
              gcFifo->fifoEnd,
              gcFifo->fifoOffset, 
              gcFifo->fifoSize,
              gcFifo->fifoPtr ); 

#if DRI_BUILD
    _grImportFifo(*driInfo.fifoPtr, *driInfo.fifoRead);
#endif
    
    /* The hw is now in a usable state from the fifo macros.
     * 
     * NB: See the comment in fxglide.h for the difference between
     * these flags.
     */
    gc->open = FXTRUE;

    /* Setup the procs that we can do w/o any mode knowledge */
    gc->archDispatchProcs.texDownloadProcs  = _GlideRoot.deviceArchProcs.curTexProcs;
    gc->archDispatchProcs.drawTrianglesProc = _GlideRoot.deviceArchProcs.curDrawTrisProc;

    /* Default render procs to window space */
    gc->archDispatchProcs.coorModeTriVector = (*_GlideRoot.deviceArchProcs.curTriProcs) + GR_WINDOW_COORDS;
    gc->archDispatchProcs.drawVertexList    = _GlideRoot.deviceArchProcs.curVertexListProcs[GR_WINDOW_COORDS];
    
    /*------------------------------------------------------
      GC Init
      ------------------------------------------------------*/
    GDBG_INFO(gc->myLevel, "  GC Init\n");
    initGC( gc );
    
    gc->orgSW = gc->state.screen_width;
    gc->orgSH = gc->state.screen_height;
    
    /*------------------------------------------------------
      3D State Init 
      ------------------------------------------------------*/
    GDBG_INFO( gc->myLevel, "  3D State Init\n");
    
    GDBG_INFO( gc->myLevel, "   Setting default register states\n" );
    gc->state.shadow.fbzMode = ( SST_ENRECTCLIP | SST_ENZBIAS );
  
    GDBG_INFO( gc->myLevel, "   Setting up initial draw buffer state\n" );
    REG_GROUP_BEGIN(BROADCAST_ID, leftOverlayBuf, 1, 0x1);
    REG_GROUP_SET(hw, leftOverlayBuf, gc->buffers[1]);
    REG_GROUP_END();
  
    REG_GROUP_BEGIN(BROADCAST_ID, swapbufferCMD, 1, 0x1); 
    REG_GROUP_SET(hw, swapbufferCMD, 0x0);
    REG_GROUP_END();

    gc->state.shadow.colBufferAddr = gc->buffers[gc->curBuffer];
    gc->state.shadow.colBufferStride = gc->strideInTiles | SST_BUFFER_MEMORY_TILED;
    gc->state.shadow.auxBufferAddr = gc->buffers[nColBuffers];
    gc->state.shadow.auxBufferStride = gc->strideInTiles | SST_BUFFER_MEMORY_TILED;
  
#ifdef GLIDE_INIT_HWC    
    REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
    {
      REG_GROUP_SET(hw, colBufferAddr, gc->state.shadow.colBufferAddr);
#if DRI_BUILD
      REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride :
		    gc->state.shadow.colBufferStride);
#else
      REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride);
#endif
      REG_GROUP_SET(hw, auxBufferAddr, gc->state.shadow.auxBufferAddr);
      REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride); 
    }
    REG_GROUP_END();
#endif
    
    GDBG_INFO( gc->myLevel, "  Setting all Glide state\n" );
    assertDefaultState();
#if DRI_BUILD
    if (nColBuffers>1)
	grRenderBuffer(GR_BUFFER_BACKBUFFER);
    else
	grRenderBuffer(GR_BUFFER_FRONTBUFFER);
    grClipWindow(0, 0, gc->state.screen_width, gc->state.screen_height);
#else
    clearBuffers( gc );
#endif
    
    /* The new splash know the color format so we move it above splash screen */
    gc->state.color_format = format;

    /* --------------------------------------------------------
       Splash Screen
       --------------------------------------------------------*/
    doSplash();


    gc->windowed = FXFALSE;
    _GlideRoot.windowsInit++; /* to avoid race with grSstControl() */

    retVal = (GrContext_t)gc;

    GR_END();
  }

  return retVal;
#undef FN_NAME
} /* grSstWinOpen */

/*-------------------------------------------------------------------
  Function: grSstWinClose
  Date: 3/16
  Implementor(s): jdt
  Library: Glide
  Description:
  Shut down the selected SST

  Shutdown has 4 steps

  3D Idle 
    the 3D engine must be idled to make sure that there are no
    commands executing in the transport when the registers are
    reset

  GC Reset
    the GC is flagged as unitialized - (nosup)

  Command Transport Disable
    the command transport to the 3D device is put in a state
    of reset.  No further commands may be issued throught the
    command transport

  Video Restore
    video is restored to its pre-open state.

  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/
GR_ENTRY(grSstWinClose, FxBool, (GrContext_t context))
{
#define FN_NAME "grSstWinClose"  
  GrGC* gc = (GrGC*)context;
  GDBG_INFO(80, FN_NAME"(0x%X)\n", context);

#if GLIDE_CHECK_CONTEXT
  if (gc->lostContext) {
    if (*gc->lostContext)
      return 0;
  }
#endif

  /* NB: The gc that is being closed is the passed gc not the
   * currently selected gc. This must be setup before the
   * 'declaration' which grabs the current gc in grFlush.  In
   * addition, it is possible for us to have 'missed' a thread attach
   * if the current thread came into existance before glide was
   * explicitly loaded by an application. In this case we have to set
   * the tls gc explicitly otherwise other whacky-ness (read 'random
   * crashes' will ensue). 
   */
  setThreadValue((unsigned long)gc);
  if ((gc != NULL) && gc->open) grFlush();

  /* Make sure that the user specified gc is not whacked */
  if ((gc != NULL) &&
      (gc >= _GlideRoot.GCs) &&
      (gc <= _GlideRoot.GCs + MAX_NUM_SST)) {
    if (gc->open) {
#if GLIDE_INIT_HAL
      /* dpc - 22 may 1997 - FixMe!
       * We need the equivilant stuff in the hal layer too.
       */
#else /* !GLIDE_INIT_HAL */
      /*--------------------------
        3D Idle
        --------------------------*/
      GDBG_INFO(gc->myLevel, "  3D Idle");

      /*--------------------------
        Command Transport Disable
        --------------------------*/
      GDBG_INFO(gc->myLevel, "  Command Transport Disable");
    
      /* Video Restore 
       *
       * NB: The hwcRestoreVideo in addition to restoring the video also
       * turns off the command fifo and then releases the hw context
       * which can unmap the board at the driver level.  The next time
       * we use grSstWinOpen we need to re-map the board etc just to be
       * safe everywhere.
       */
      GDBG_INFO(gc->myLevel, "  Restore Video");
#if GLIDE_CHECK_CONTEXT
      if (!*gc->lostContext)
#endif
        hwcRestoreVideo(gc->bInfo);
#endif /* !GLIDE_INIT_HAL */

      /*--------------------------
        GC Reset
        --------------------------*/
      GDBG_INFO(gc->myLevel, "  GC Reset");

      /* These are really two different things.  
       *
       * hwInitP indicates whether the init code mapping/init sequence
       * is active for this hw. 
       *
       * open includes setting up video, command transport, and the
       * initial glide state.
       */
      gc->hwInitP = FXFALSE;

      _grDisplayStats();
    }
    gc->open = FXFALSE;
    gc->grSstRez = GR_RESOLUTION_NONE;
    gc->grSstRefresh = GR_REFRESH_NONE;

    
  }
  _GlideRoot.windowsInit--;

#if !defined(__linux__) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
  if ( gc->bInfo->osNT )
    hwcUnmapMemory();
  else
    hwcUnmapMemory9x ( gc->bInfo );
#endif

  return FXTRUE;
#undef FN_NAME
} /* grSstWinClose */

/*-------------------------------------------------------------------
  Function: grSetNumPendingBuffers
  Date: 13-Oct-2000
  Implementor(s): mmcclure
  Description:
  
  Allow the application to supply the number of pending buffers

  Arguments:

  NumPendingBuffers - Sent to force number of pending buffers
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grSetNumPendingBuffers, void, (FxI32 NumPendingBuffers))
{
  _GlideRoot.environment.swapPendingCount = NumPendingBuffers;
}

/*-------------------------------------------------------------------
  Function: grSelectContext
  Date: 18-Jan-98
  Implementor(s): atai, taco
  Description:
  
  Designates a context as current.  This selects a named state vector
  with it's own command transport as the current target for all glide
  commands.

  Arguments:

  context - name of context
  
  Return:

  TRUE - valid context 
  FALSE - invalid context
  -------------------------------------------------------------------*/
/* NOTE: THIS FUNCTION CAN BE CALLED WHEN THERE IS NO VALID GC */        
GR_DIENTRY(grSelectContext, FxBool , (GrContext_t context) )
{
#define FN_NAME "grSelectContext"
  GrGC* 
    gc = (GrGC*)context;
  FxBool 
    rv = FXFALSE;

  GDBG_INFO( 80, FN_NAME"(0x%X)\n", context );
  
  /* WTF?!?!?!?!!? - dpc - 31 mar 1999 - I was not aware that apps
   * could select invalid contexts. They're not supposed to call
   * grSstWinClose w/ some whacked value, why's this function any
   * different?  
   */
  if ( context == 0 ) {
    GDBG_INFO( 80, "NULL Context passed\n" );
    rv = FXFALSE;
  } else {
    setThreadValue((unsigned long)context);
    if (gc != NULL) {
      if (gc->windowed) {
        /* Setup the procs that we can do w/o any mode knowledge */
        gc->archDispatchProcs.texDownloadProcs  = _GlideRoot.deviceArchProcs.curTexProcs;
        gc->archDispatchProcs.drawTrianglesProc = _GlideRoot.deviceArchProcs.curDrawTrisProc;
        
        /* Default render procs to window space */
        gc->archDispatchProcs.coorModeTriVector = (*_GlideRoot.deviceArchProcs.curTriProcs) + GR_WINDOW_COORDS;
        gc->archDispatchProcs.drawVertexList    = _GlideRoot.deviceArchProcs.curVertexListProcs[GR_WINDOW_COORDS];

        gc->contextP = FXTRUE;
        gc->open = FXTRUE;

        rv = FXTRUE;
      } else {
        const FxBool
          oldContextState = gc->contextP;

        GR_ASSERT((gc >= _GlideRoot.GCs) &&
                  (gc <= _GlideRoot.GCs + MAX_NUM_SST));

#if defined( GLIDE_INIT_HWC ) && GLIDE_CHECK_CONTEXT
        gc->contextP = hwcQueryContext(gc->bInfo);
#else
        gc->contextP = 1;
#endif
        rv = gc->contextP;

        /* We may now need to fiddle w/ the current specialization
         * vectors. The fifo writing macros are 'smart' (in a dumb
         * sort of way) about not writing to the fifo, but the
         * specialized routines in assembly are not. We replace the
         * base specialization vectors here w/ null (empty that is)
         * vectors that don't do anything.  
         */
        if (!gc->contextP && oldContextState) {
          GrTriSetupProcArchVector* 
            curTriProcs = _GlideRoot.deviceArchProcs.curTriProcs;
          GrVertexListProc*
            curVertexListProcs = _GlideRoot.deviceArchProcs.curVertexListProcs;

          _GlideRoot.deviceArchProcs.curTriProcs        = _GlideRoot.deviceArchProcs.nullTriProcs;
          _GlideRoot.deviceArchProcs.curVertexListProcs = _GlideRoot.deviceArchProcs.nullVertexListProcs;

          _GlideRoot.deviceArchProcs.nullTriProcs        = curTriProcs;
          _GlideRoot.deviceArchProcs.nullVertexListProcs = curVertexListProcs;

          gc->archDispatchProcs.texDownloadProcs  = _GlideRoot.deviceArchProcs.nullTexProcs;
          gc->archDispatchProcs.drawTrianglesProc = _GlideRoot.deviceArchProcs.nullDrawTrisProc;
          gc->archDispatchProcs.coorModeTriVector = (*_GlideRoot.deviceArchProcs.curTriProcs) + GR_WINDOW_COORDS;
          gc->archDispatchProcs.drawVertexList    = _GlideRoot.deviceArchProcs.curVertexListProcs[GR_WINDOW_COORDS];
        }
      }
    }
  }
  
  GDBG_INFO(80, "%s() => 0x%x---------------------\n", FN_NAME, rv );
  return rv;
#undef FN_NAME
} /* grSelectConetext */


/*---------------------------------------------------------------------------
**  grStatsResetPerfStats
*/

void FX_CSTYLE
_grSstResetPerfStats(void)
{
#define FN_NAME "grSstResetPerfStats"
  GR_BEGIN("grSstResetPerfStats",83,4, 1);
  GDBG_INFO_MORE(gc->myLevel,"()\n");
  GR_SET(BROADCAST_ID, hw, nopCMD, 1);
  GR_END();
#undef FN_NAME
} /* grSstResetPerfStats */


/*---------------------------------------------------------------------------
**  grSstStatus - return contents of status register
*/

FxU32 FX_CSTYLE
_grSstStatus(void)
{
#define FN_NAME "grSstStatus"
  FxU32 status;

  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME, 83);

  status = GR_GET(hw->status);

  return(status);

  GR_END();
#undef FN_NAME
}/* grSstStatus */

/*---------------------------------------------------------------------------
**  grSstVideoLine - return current video line number
*/

FxU32 FX_CSTYLE
_grSstVideoLine(void)
{
  FxU32 vline = 1;

  return vline;
}/* grSstVideoLine */

/*---------------------------------------------------------------------------
**  grSstVRetrace - return contents of SST_VRETRACE bit of status register;
*/

FxBool FX_CSTYLE
_grSstVRetraceOn(void)
{
  FxU32 status;
  GR_BEGIN_NOFIFOCHECK_RET("grSstVRetraceOn",83);

  status = GR_GET(hw->status);

  return ((status & SST_VRETRACE) == 0);

}/* grSstVRetrace */

/*-------------------------------------------------------------------
  Function: grFlush
  Date: 09-Jan-98
  Implementor(s): atai
  Description:

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_ENTRY(grFlush, void, (void))
{
#define FN_NAME "grFlush"
  GR_BEGIN_NOFIFOCHECK( "grFlush", 80 );
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, nopCMD, 0x0);
  GR_CHECK_SIZE();

  if ( gc->windowed ) {
#ifdef GLIDE_INIT_HWC
    GDBG_INFO(gc->myLevel + 200, FN_NAME": cmdSize(0x%X)\n",
              ((FxU32)gc->cmdTransportInfo.fifoPtr - 
               (FxU32)gc->cmdTransportInfo.hwcFifoInfo.cmdBuf.baseAddr));
#endif
    _FifoFlush();
  } else if (!gc->cmdTransportInfo.autoBump) {
    GR_BUMP_N_GRIND;
  }
  
  GR_END();
#undef FN_NAME
} /* grFlush */

/*---------------------------------------------------------------------------
** grSstIdle/grFinish
*/
GR_ENTRY(grFinish, void, (void))
#define FN_NAME "grFinish"
{
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  grFlush();
  if ( gc->windowed ) {
#if defined( GLIDE_INIT_HWC ) && !defined( __linux__ ) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
    struct cmdTransportInfo*
      gcFifo = &gc->cmdTransportInfo;
    
    /* Make sure that the issued commands have been executed. */
    while(hwcExecuteStatusWinFifo(gc->bInfo, 
                                  &gcFifo->hwcFifoInfo, 
                                  gcFifo->issuedSerialNumber) != gcFifo->issuedSerialNumber) {
      /* Do Nothing? */;
    }
#endif
  } else {
    while((_grSstStatus() & SST_BUSY) != 0)
      /* Do Nothing */; 
  }

  GR_END();
#undef FN_NAME
} /* grSstIdle */

/*---------------------------------------------------------------------------
**  grSstIsBusy - find out if the SST is busy or not
*/

FxBool FX_CSTYLE
grSstIsBusy(void)
{
#define FN_NAME "grSstIsBusy"
  static FxBool nopP = FXTRUE;
  FxBool busy;
  GR_BEGIN_NOFIFOCHECK_RET("grSstIsBusy", 80);

  /* dpc - 22 may 1997 - FixMe!
   * Seems like the simplest way to do it, but is this really the way
   * to do it?  
   */
  if (nopP) {
    GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
    GR_SET(BROADCAST_ID, hw, nopCMD, 0);
    GR_CHECK_SIZE();
  }

  if ( gc->windowed ) {
    _FifoFlush();
    busy = 0;
  } else {
    busy = ((_grSstStatus() & SST_BUSY) != 0);
  }

  nopP = !busy;

  GDBG_INFO(84,"grSstIsBusy() => 0x%x\n", busy);

  return busy;
#undef FN_NAME
}/* grSstIsBusy */

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
/*---------------------------------------------------------------------------
**  guGammaCorrectionRGB - set the gamma correction value
*/

GR_ENTRY(guGammaCorrectionRGB, void, (float r, float g, float b))
{
  GR_BEGIN_NOFIFOCHECK("guGammaCorrectionValue",80);
  GDBG_INFO_MORE(gc->myLevel,"(%g %g %g)\n",r, g, b);

#if GLIDE_INIT_HAL
  fxHalInitGamma(hw, r);
#else /* !GLIDE_INIT_HAL */
  if (gc)
    hwcGammaRGB(gc->bInfo, r, g, b);
#endif /* !GLIDE_INIT_HAL */

  GR_END();
} /* guGammaCorrectionRGB */

/*-------------------------------------------------------------------
  Function: grLoadGammaTable
  Date: 05-Jan-97
  Implementor(s): atai
  Description:

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grLoadGammaTable, void, (FxU32 nentries, FxU32 *red, FxU32 *green, FxU32 *blue))
{
#define FN_NAME "grLoadGammaTable"
  FxU32 max;

  GR_BEGIN_NOFIFOCHECK("grLoadGammaTable",80);
  
  grGet(GR_GAMMA_TABLE_ENTRIES, 4, (FxI32 *)&max);
  if (nentries > max)
    nentries = max;

#ifdef GLIDE_INIT_HWC
  hwcGammaTable(gc->bInfo, nentries, red, green, blue);  
#endif

  GR_END();
#undef FN_NAME
}
#endif

#ifndef GLIDE3
/*---------------------------------------------------------------------------
**  grGammaCorrectionValue - set the gamma correction value
*/

GR_ENTRY(grGammaCorrectionValue, void, (float gamma))
{
  GR_BEGIN_NOFIFOCHECK("grGammaCorrectionValue",80);
  GDBG_INFO_MORE(gc->myLevel,"(%g)\n",gamma);

#if GLIDE_INIT_HAL
  fxHalInitGamma(hw, gamma);
#else /* !GLIDE_INIT_HAL */

  hwcGamma(gc->bInfo, gamma, gamma, gamma);

#endif /* !GLIDE_INIT_HAL */

       GR_END();
} /* grGammaCorrectionValue */
#endif

/*---------------------------------------------------------------------------
**  grSstOrigin - Set the orgin orientation of the screen.
**
**  Returns:
**
**  Notes:
**
*/

GR_STATE_ENTRY(grSstOrigin, void, (GrOriginLocation_t origin))
{
#define FN_NAME "grSstOrigin"
  FxU32 fbzMode;
  GR_BEGIN_NOFIFOCHECK("grSstOrigin", 83);
  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", origin);

  /* Initialize FBZMODE register */
  fbzMode = gc->state.shadow.fbzMode;
  if (origin == GR_ORIGIN_LOWER_LEFT)
    fbzMode |= SST_YORIGIN;
  else
    fbzMode &= ~(SST_YORIGIN);

  /* dpc - 22 may 1997 - FixMe! 
   * Do we need to do anything here for the HAL?
   */
#if !GLIDE_INIT_HAL
  /* dpc - 5 sep 1997 - FixMe!
   * This is the old way. Is there anything else we
   * need to do here?
   *
   * initOrigin(origin); 
   */
#endif

  gc->state.shadow.fbzMode = fbzMode;

#if !GLIDE3
  REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 0x1, 0x1);
  REG_GROUP_SET(hw, fbzMode, fbzMode);
  REG_GROUP_END();
#endif

#undef FN_NAME
} /* grSstOrigin */

/* GMT: do we really have users for this???
 * CHD: No.
 * JDT: Huh?  If you're talking about grSstOrigin, you're smoking crack.
 *      if you are talking about SstConfigPipeline, it is evil and must
 *      be destroyed. :)
 * dpc: There is one user that I know of. This 'Nature' demo that Scott just
 *      gave me.
 * chd: It's a stub now.
 * (much time elapses)
 * chd: But WTF is that forward decl down there?
 * dpc: Its to get rid of the damn compiler warning for a function that
 *      we only sort of export.
 */

extern FX_ENTRY void FX_CALL
grSstConfigPipeline(GrChipID_t chip, FxU32 reg, FxU32 value);

/*---------------------------------------------------------------------------
** grSstConfigPipeline
*/

GR_ENTRY(grSstConfigPipeline, void, (GrChipID_t chip, FxU32 reg, FxU32 value))
{
} /* grSstConfigPipeline */

