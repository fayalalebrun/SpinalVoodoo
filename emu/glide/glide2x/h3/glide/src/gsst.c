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
** 4     3/17/99 6:16p Dow
** Phantom Menace and other fixes.
** 
** 205   3/17/99 5:08p Peter
** removed whacky stuff now that the command fifo threshold stuff appears
** to make all happy (including the k7)
** 
** 204   3/17/99 1:22p Dow
** it really should be an assignment
** 
** 203   3/17/99 10:54a Dow
** fixed euivalence test for autoBump
** 
** 202   3/16/99 11:51a Atai
** Back door (set FX_GLIDE_ENABLE_UMA=1) to enable unified texture memory.
** TMUn memory size will the whole texture memory space. The offset for
** each TMU points to the start address of the memory pool.
** 
** 201   3/15/99 10:51p Dow
** Vile Hack
** 
** 200   3/13/99 9:48p Dow
** 
** 199   3/10/99 6:38p Peter
** removed cruft
** 
** 198   3/10/99 10:40a Peter
** bump-n-grind workaround for katmai until the bug is better
** characterized
** 
** 197   3/06/99 10:42a Dow
** Added an idle to make sure.
** 
** 196   3/05/99 9:01a Atai
** for direct write compilation
** 
** 195   3/04/99 5:42p Atai
** cast pointer
** 
** 194   3/04/99 3:15p Atai
** mods for direct write
** 
** 193   3/03/99 4:44p Peter
** clear chroma range and texChroma for default state
** 
** 192   2/27/99 12:24p Dow
** new resolutions
** 
** 191   2/26/99 10:27a Peter
** Mmmm.... 8.3
** 
** 190   2/19/99 7:58p Peter
** doh!
** 
** 189   2/19/99 7:56p Peter
** load splash plug-in even w/ env var set
** 
** 188   2/19/99 5:54p Peter
** new splash screen
** 
** 187   2/13/99 1:59p Dow
** Added code for new resolutions
** 
** 186   12/21/98 1:47p Atai
** Update gc->swapsPending before we return the status, fixed for the
** games that rely on masking status to get
** buffernumpending.
** 
** 185   12/18/98 1:55p Atai
** "SSTV2_GAMMA" is used to update the video/brightness setting.
** 
** 184   12/09/98 6:02p Atai
** grTexCombine did the right thing for the un-used TMU. Initialize the
** 2nd TMU value to take care of "set FX_GLIDE_NUM_TMU=1"
** 
** 183   12/09/98 5:00p Atai
** set MAXLOD = MINLOD = 8 in _grUpdateParamIndex if ST1 is not used
** 
** 182   12/09/98 2:02p Atai
** Added _grTexForceLod back. Set tLOD min = max = 8 for the 2nd TMU by
** default for Avenger to increase single texturing tri fillrate.
** 
** 181   12/03/98 11:27p Dow
** Code 'cleanup' heç
** 
** 180   12/03/98 10:37p Dow
** Removed bogus gamma setting in assertDefaultState\nRemoved protoected &
** vestigial Glide3 funcions
** 
** 179   11/18/98 8:00p Dow
** grxclk
** 
** 178   11/18/98 4:27p Atai
** direct write mods. 2 tmu mode does not work yet
** 
** 177   11/14/98 2:00a Lab
** fixed second tmu size
** 
** 176   11/10/98 6:44p Atai
** number of tmu and texture memory allocation
** 
** 175   11/10/98 4:53p Atai
** fixed gc->tramoffset
** 
** 174   11/02/98 5:34p Atai
** merge direct i/o code
** 
** 173   10/21/98 4:20p Atai
** gamma stuff
** 
** 172   10/21/98 10:41a Atai
** 
** 171   10/20/98 5:34p Atai
** added #ifdefs for hwc
** 
** 170   10/20/98 4:39p Atai
** update tramOffset and tramSize
** 
** 169   10/13/98 8:44p Dow
** Made it work for 4MB boards
** 
** 168   10/09/98 6:57p Peter
** dynamic 3DNow!(tm) mods
** 
** 167   10/08/98 10:15a Dow
** Triple Buffering fix
** 
** 166   9/18/98 10:53a Dow
** VidMode Stuff
** 
** 165   9/04/98 11:36a Peter
** re-open fix for nt (thanks to taco/rob/nt bob)
** 
** 164   9/02/98 1:34p Peter
** watcom warnings
** 
** 163   9/01/98 10:07a Dow
** Fixed Control panel effage
** 
** 162   8/28/98 4:38p Atai
** 1. added MIN_FIFO_SIZE for memory checking
** 2. hack for resolution checking if we have 8M board and triple
** buffering on
** 
** 161   8/27/98 9:54p Peter
** use gc->frontBuffer rather than 1
** 
** 160   8/27/98 6:36p Atai
** check if environment.tmuMemory is on
** 
** 159   8/26/98 10:11p Atai
** set color buffer to 3 if control panel force triple buffering
** 
** 158   8/03/98 4:07p Mikec
** Removed glide3 splash screen.
** 
** 157   8/03/98 10:45a Atai
** rename 3dfxsplash3.dll to 3dfxspl3.dll
** 
** 156   7/29/98 3:09p Dow
** SDRAM Fixes
** 
** 155   7/24/98 2:03p Dow
** AGP Stuff
** 
** 154   7/23/98 1:17a Dow
** Bump & Grind
** 
** 153   7/15/98 2:03p Mikec
** Fixed Lfb depth buffer readback problem. It turns out grWinOpen didn't
** set up the lfb buffer pointer for the aux buffer correctly. 
** 
** 152   7/06/98 7:05p Jdt
** grSstWinOpen simplified, in-memory buffer logic added
** 
** 151   6/26/98 11:09a Jdt
** 
** 150   6/26/98 10:08a Jdt
** 
** 149   6/25/98 6:49p Jdt
** Removed direct IO write from grSstWinOpen and moved to minihwc
** 
** 148   6/24/98 11:16a Dow
** More SST1 Statuws stuff
** 
** 147   6/23/98 2:52p Dow
** Added SST1 FIFO status.
** 
** 146   6/19/98 2:33p Mikec
** Fixed lfb buffer offset fucakge
** 
** 145   6/16/98 6:12p Dow
** Rearranged texture memory
** 
** 144   6/12/98 4:20p Dow
** Return FXFALSE from WinOpen if GR_RESOLUTION_NONE is specified.
** 
** 143   6/10/98 9:49a Peter
** lfb buffer addressing
** 
** 142   6/09/98 7:48p Mikec
** Added buffer clears to grSstWinOpen to get rid of random pixels.
** 
** 141   6/05/98 11:22a Jeske
** we never copied the hwc's memory calculation into gc->fbOffset... We
** might need to copy more data than this.. check this...
** 
** 140   6/04/98 12:12p Peter
** splash dll rename
** 
** 139   5/31/98 9:03a Dow
** 800x600 resolution
** 
** 138   5/28/98 1:46p Dow
** Swap Pending Workaround
** 
** 137   5/22/98 2:37p Peter
** 
** 136   5/21/98 5:27p Peter
** hwcInitVideo failing is fatal for grSstWinOpen
** 
** 135   5/13/98 9:12a Dow
** 
** 134   5/12/98 2:42p Dow
** 
** 133   4/30/98 4:10p Peter
** Fixed comment from before
** 
** 132   4/19/98 11:31a Dow
** Fixed grSstStatus
** 
** 131   4/14/98 6:41p Peter
** Merge w/ cvg glide cleanup
** 
** 130   4/07/98 10:40p Dow
** LFB Fixes:  Round 1
** 
** 129   3/28/98 11:24a Dow
** itwoç
** 
** 128   3/20/98 1:12p Dow
** Windows happiness
** 
** 127   3/16/98 8:47p Dow
** More Windows Happiness
** 
** 125   3/03/98 11:31a Dow
** Resolutions now work
** 
** 124   2/17/98 12:52p Dow
** fifo setup code
** 
** 123   2/09/98 12:27p Dow
** Fixed some niggling hangnails
** 
** 122   2/08/98 3:08p Dow
** FIFO Works
** 
** 121   2/02/98 4:31p Dow
** IO w/o HAL now possible
** 
** 120   1/30/98 1:34p Dow
** Formed call to fxHalInitVideoOverlaySurface
** 
** 119   1/29/98 9:54p Dow
** This is Banshee
** 
** 118   1/20/98 11:03a Peter
** env var to force triple buffering
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

#if (GLIDE_PLATFORM & GLIDE_HW_SST96) 
#include <init.h>
#endif

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define kPageBoundarySlop 0x1000UL
#define kPageBoundaryMask (kPageBoundarySlop - 1)


/* Init hw */

typedef struct {
    GrScreenResolution_t resolution;
    FxU32                xres;
    FxU32                yres;
} ResEntry;

static ResEntry _resTable[] = {
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
  {GR_RESOLUTION_2048x2048, 2048,  2048 /* 0x17 */}
};


/* ---------------------------------------------
   This function both sets and documents the 
   expected default state for any rendering
   context

   ..taco - separated out in preparation for
   multiple contexts
   ---------------------------------------------*/
static void 
assertDefaultState( void ) 
{
#define FN_NAME "assertDefaultState"
  GR_DCL_GC;

  grHints(GR_HINT_ALLOW_MIPMAP_DITHER, 0);
  grSstOrigin(gc->state.origin);
  grAlphaBlendFunction(GR_BLEND_ONE , GR_BLEND_ZERO, 
                       GR_BLEND_ONE, GR_BLEND_ZERO);
  grAlphaTestFunction(GR_CMP_ALWAYS);
  grAlphaTestReferenceValue(0);
  grChromakeyMode(GR_CHROMAKEY_DISABLE);
  grConstantColorValue((FxU32) ~0);
  grClipWindow(0, 0, gc->state.screen_width, 
               gc->state.screen_height);
  _grColorCombineDelta0Mode(FXFALSE);
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

  /* Get rid of crap in the buffers. */
  grSstIdle();
  if ( gc->state.num_buffers > 1 ) {
    grBufferClear( 0, 0, GR_WDEPTHVALUE_FARTHEST );
    grBufferSwap( 1 );
    grBufferClear( 0, 0, GR_WDEPTHVALUE_FARTHEST );
    grBufferSwap( 1 );
    grBufferClear( 0, 0, GR_WDEPTHVALUE_FARTHEST );
    grBufferSwap( 1 );
    grRenderBuffer( GR_BUFFER_BACKBUFFER );
  } else {
    grBufferClear( 0, 0, GR_WDEPTHVALUE_FARTHEST );
    grRenderBuffer( GR_BUFFER_FRONTBUFFER );
  }
    
  guTexMemReset();

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
  case 1:
    grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
    grTexDetailControl(GR_TMU0, 0, 1, 1.0F);
    grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, 
                    GR_TEXTUREFILTER_POINT_SAMPLED);
    grTexLodBiasValue(GR_TMU0, 0.0F);
    grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
    grTexCombine(GR_TMU0, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 FXFALSE, FXFALSE);
  }
  grLfbConstantAlpha(0xFF);
  grLfbConstantDepth(0);

  /* Glide2x does not expose texture chroma or chroma ranges, but
   * glide3x does so we need to make sure that those bits are clear by
   * default too so that the bits don't remain set by accident.
   */
  {
    FxI32 i;

    GR_SET_EXPECTED_SIZE(((gc->num_tmu * 2) + 1) * sizeof(FxU32), (gc->num_tmu * 2) + 1);

    GR_SET(BROADCAST_ID, (SstRegs *)gc->reg_ptr, chromaRange, 0x00UL);

    for(i = 0; i < gc->num_tmu; i++) {
      GR_SET((0x02UL << i), (SstRegs *)gc->reg_ptr, chromaKey, 0x00UL);
      GR_SET((0x02UL << i), (SstRegs *)gc->reg_ptr, chromaRange, 0x00UL);
    }
    GR_CHECK_SIZE();
  }
#undef FN_NAME
} /* assertDefaultState */

static void 
doSplash(void)
{
#if defined(GLIDE_SPLASH) || ((GLIDE_PLATFORM & GLIDE_OS_WIN32) && defined(GLIDE_PLUG))
  GR_DCL_GC;
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32) && (defined(GLIDE_SPLASH)||defined(GLIDE_PLUG))
  {
    FxBool
      didLoad;

    gc->pluginInfo.moduleHandle = LoadLibrary("3dfxspl2.dll");
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
        didLoad = (*gc->pluginInfo.initProc)(gc->grHwnd,
                                             gc->state.screen_width, gc->state.screen_height,
                                             gc->grColBuf, gc->grAuxBuf,
                                             gc->state.color_format);
        if (!didLoad) (*gc->pluginInfo.shutdownProc)();
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
static void initGC ( GrGC *gc ) {
    FxI32 t = 0;

    gc->curBuffer   = 0;
    gc->frontBuffer = 1;
    gc->backBuffer  = (gc->nColBuffers > 2) ? 2 : gc->curBuffer;

    for (t = 0; t < 7; t++) {
      gc->bufferSwaps[t] = 0xffffffff;
    }

    gc->bufferSwaps[0] =
        (FxU32) gc->cmdTransportInfo.fifoPtr -
        (FxU32) gc->cmdTransportInfo.fifoStart; 
    
    gc->swapsPending = 1;
    
    gc->lockPtrs[GR_LFB_READ_ONLY]  = (FxU32)-1;
    gc->lockPtrs[GR_LFB_WRITE_ONLY] = (FxU32)-1;
    /* Initialize the read/write registers to all 0 */
    gc->state.fbi_config.fbzColorPath  = 0;
    gc->state.fbi_config.fogMode       = 0;
    gc->state.fbi_config.alphaMode     = 0;
    gc->state.fbi_config.fbzMode       = 0;
    gc->state.fbi_config.lfbMode       = 0;
    gc->state.fbi_config.clipLeftRight = 0;
    gc->state.fbi_config.clipBottomTop = 0;
    gc->state.fbi_config.fogColor      = 0;
    gc->state.fbi_config.zaColor       = 0;
    gc->state.fbi_config.chromaKey     = 0;
    gc->state.fbi_config.stipple       = 0;
    gc->state.fbi_config.color0        = 0;
    gc->state.fbi_config.color1        = 0;
    for (t = 0; t < gc->num_tmu; t += 1) {
        FxU32 textureMode = (FxU32)SST_SEQ_8_DOWNLD;
        if ((_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type == GR_SSTTYPE_VOODOO) && 
            (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[t].tmuRev == 0)) {
            textureMode = 0;
        }
        gc->state.tmu_config[t].textureMode     = textureMode;
        gc->state.tmu_config[t].tLOD            = 0x00000000;
        gc->state.tmu_config[t].tDetail         = 0x00000000;
        gc->state.tmu_config[t].texBaseAddr     = 0x00000000;
        gc->state.tmu_config[t].texBaseAddr_1   = 0x00000000;
        gc->state.tmu_config[t].texBaseAddr_2   = 0x00000000;
        gc->state.tmu_config[t].texBaseAddr_3_8 = 0x00000000;
        gc->state.tmu_config[t].mmMode          = GR_MIPMAP_NEAREST;
        gc->state.tmu_config[t].smallLod        = G3_LOD_TRANSLATE(GR_LOD_1);
        gc->state.tmu_config[t].largeLod        = G3_LOD_TRANSLATE(GR_LOD_1);
        gc->state.tmu_config[t].evenOdd         = GR_MIPMAPLEVELMASK_BOTH;
        gc->state.tmu_config[t].nccTable        = GR_NCCTABLE_NCC0;
    } 
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
  FXTRUE - glide successfully acquired the necessary resources and a
           is ready for rendering
  FXFALSE - glide was unsuccessful in getting the necessary resources, 
            or the requested configuration is unavailble on the host
            hardware - any calls to glide rendering routines will result
            in undefined behavior.
  -------------------------------------------------------------------*/
GR_ENTRY(grSstWinOpen, FxBool, ( FxU32                   hWnd,
                                 GrScreenResolution_t    resolution, 
                                 GrScreenRefresh_t       refresh, 
                                 GrColorFormat_t         format, 
                                 GrOriginLocation_t      origin, 
                                 int                     nColBuffers,
                                 int                     nAuxBuffers))
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
#if USE_PACKET_FIFO
    int            buffer;
#endif
    struct cmdTransportInfo *gcFifo = 0;

    GR_BEGIN_NOFIFOCHECK("grSstWinOpen",80);
    GDBG_INFO_MORE(gc->myLevel,
                   "(rez=%d,ref=%d,cformat=%d,origin=%s,#bufs=%d, #abufs=%d)\n",
                   resolution,refresh,format,
                   origin ? "LL" : "UL",
                   nColBuffers, nAuxBuffers);
    GR_CHECK_F(FN_NAME, !gc, "no SST selected as current (gc==NULL)");
    
#if 1
    /* ??? the eff is this about */
    if ((nColBuffers == 2) &&
        (nAuxBuffers == 0)) nAuxBuffers = 1;
#endif

#ifdef GLIDE_INIT_HWC
    /*
    ** check if the environment variable for triple buffering is on
    */
    if (_GlideRoot.environment.nColorBuffer != -1) {
      if (_GlideRoot.environment.nColorBuffer > 0 &&
          _GlideRoot.environment.nColorBuffer <= 3) {
        nColBuffers = _GlideRoot.environment.nColorBuffer;
      }
    }
#endif

    resolution =
      (((FxU32)resolution) > (sizeof(_resTable) / sizeof(ResEntry)))
        ? GR_RESOLUTION_640x480 
          : resolution;

#ifdef GLIDE_INIT_HWC
    /*
    ** hack!!!
    ** if we have 8M board and triple buffering, reject resolutions greater than 800x600
    */
    if ((nColBuffers == 3) && (gc->bInfo->h3Mem == 8)) {
      switch (resolution) {
      case GR_RESOLUTION_960x720:
      case GR_RESOLUTION_1024x768:
      case GR_RESOLUTION_1280x1024:
      case GR_RESOLUTION_1600x1200:
        GDBG_INFO( gc->myLevel, "Failed to open for insufficient memory\n" );
        GrErrorCallback( "grSstWinOpen: not enough memory for requested buffers", FXFALSE );
        GR_RETURN( FXFALSE );
        break;
      default:
        break;
      }
    }
#endif

    gc->state.screen_width  = _resTable[resolution].xres;
    gc->state.screen_height = _resTable[resolution].yres;
    gc->nColBuffers = nColBuffers;
    GR_CHECK_F( FN_NAME, resolution != _resTable[resolution].resolution, 
                "resolution table compilation incorrect" );
    if ( gc->vidTimings ) {
        gc->state.screen_width  = gc->vidTimings->xDimension;
        gc->state.screen_height = gc->vidTimings->yDimension;
    }

    gc->state.color_format       = format;
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
        GR_RETURN( FXFALSE );
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
    }

    vInfo->xRes        = gc->state.screen_width;
    vInfo->yRes        = gc->state.screen_height;
    vInfo->refresh     = gc->grSstRefresh;
    vInfo->tiled       = FXTRUE;
    vInfo->initialized = FXTRUE;

    if ( hwcAllocBuffers( bInfo, nColBuffers, nAuxBuffers ) == FXFALSE ) {
        GDBG_INFO( gc->myLevel, "hwcAllocBuffers failed\n" );
        GrErrorCallback(hwcGetErrorString(), FXFALSE);
        GR_RETURN( FXFALSE );
    }

    for (buffer = 0; buffer < nColBuffers; buffer++) {
        gc->buffers[buffer] = bufInfo->colBuffStart[buffer];
        GDBG_INFO(80, "Buffer %d:  Start: 0x%x\n", buffer, gc->buffers[buffer]);
        gc->lfbBuffers[buffer] = (FxU32)gc->rawLfb + bufInfo->lfbBuffAddr[buffer];
    }
    if (nAuxBuffers != 0) {
        gc->buffers[buffer] = bufInfo->auxBuffStart;
        gc->lfbBuffers[buffer] = (FxU32)gc->rawLfb + bufInfo->lfbBuffAddr[buffer];
        GDBG_INFO(80, "Aux Buffer:  Start: 0x%x\n", gc->buffers[buffer]);
    }

    gc->fbOffset               = bInfo->fbOffset;

    switch (gc->num_tmu) {
    case 2:
      if (_GlideRoot.environment.enUma) {
        /*
        ** hack for Glide2x to enable uma
        */
        gc->tramOffset[0]          = gc->tramOffset[1]          = bInfo->tramOffset;
        gc->tramSize[0]            = gc->tramSize[1]            = bInfo->tramSize;
        gc->tmu_state[0].total_mem = gc->tmu_state[1].total_mem = gc->tramSize[0];
        /* disable splash screen in case it uses 2nd tmu the old style */
        _GlideRoot.environment.noSplash = 1;
      }
      else {
        gc->tramOffset[0]          = bInfo->tramOffset;
        /*
        ** if the environment variable is on, use the its texture memory size * 2
        */
        if ((_GlideRoot.environment.tmuMemory != -1) && (gc->fbOffset >= 0x200000))
          bInfo->tramSize = _GlideRoot.environment.tmuMemory << 21;
        /*
        ** when tmu size is 2, split the texture memory for each tmu
        */
        gc->tramSize[0]            = bInfo->tramSize >> 1;
        gc->tmu_state[0].total_mem = gc->tramSize[0];
        gc->tramOffset[1]          = gc->tramOffset[0] + gc->tramSize[0];
        gc->tramSize[1]            = bInfo->tramSize >> 1;
        gc->tmu_state[1].total_mem = gc->tramSize[1];
      }
      break;
    case 1:
    default:
      gc->tramOffset[0]          = bInfo->tramOffset;
      /*
      ** if the environment variable is on, use the its texture memory size
      */
      if ((_GlideRoot.environment.tmuMemory != -1) && (gc->fbOffset >= 0x200000))
        bInfo->tramSize = _GlideRoot.environment.tmuMemory << 20;
      gc->tramSize[0]            = bInfo->tramSize;
      gc->tmu_state[0].total_mem = bInfo->tramSize;
      break;
    }

    vInfo->hWnd     = gc->grHwnd;
    vInfo->sRes     = gc->grSstRez;
    vInfo->vRefresh = gc->grSstRefresh;

    /* Overlay hack */
    gc->desktopOverlay=0;
    if (GETENV ("SSTH3_DESKTOP_OVERLAY"))
    {
      gc->desktopOverlay = 1;
    }

    if ( hwcInitVideo( bInfo, FXTRUE, gc->vidTimings, FXTRUE ) == FXFALSE ) {
        GrErrorCallback(hwcGetErrorString(), FXFALSE);
        GDBG_INFO( gc->myLevel, "hwcInitVideo failed\n" );
        GR_RETURN( FXFALSE );
    }

    GDBG_INFO(80, FN_NAME ":  GammaRGB = %1.2f, %1.2f, %1.2f\n", 
              _GlideRoot.environment.gammaR,
              _GlideRoot.environment.gammaG,
              _GlideRoot.environment.gammaB);

    {
      /*
      ** This is a hack for gamma setting.
      */
      char *gammastr;
      if ((gammastr = GETENV("SSTV2_GAMMA"))) {
        _GlideRoot.environment.gammaR 
          = _GlideRoot.environment.gammaG 
          = _GlideRoot.environment.gammaB 
          = (float)atof(gammastr);
      }
    }

    if (
        _GlideRoot.environment.gammaR != -1.f &&
        _GlideRoot.environment.gammaG != -1.f &&
        _GlideRoot.environment.gammaB != -1.f
        ) {
      hwcGammaRGB(gc->bInfo, _GlideRoot.environment.gammaR, 
                      _GlideRoot.environment.gammaG,
                      _GlideRoot.environment.gammaB);
    } else {
      hwcGammaRGB(gc->bInfo, 1.3f, 1.3f, 1.3f);
    }

    if (_GlideRoot.environment.grxClk != -1L)
      hwcSetGrxClock(gc->bInfo, _GlideRoot.environment.grxClk);

    if (gc->cmdTransportInfo.autoBump == _GlideRoot.environment.autoBump) {
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
    gc->fbOffset               = (FxU32)gc->rawLfb;
    gc->tramOffset[0]          = 0x200000;
    gc->tramSize[0]            = 0x200000;
    gc->tramOffset[1]          = gc->tramSize[0] + gc->tramOffset[0];
    gc->tramSize[1]            = 0x200000;
    gc->tmu_state[0].total_mem = gc->tramSize[0];
    gc->tmu_state[1].total_mem = gc->tramSize[1];
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
        GR_RETURN( FXFALSE );
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
                           0, /* which fifo - 0 for 3d cmd fifo */
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
        GR_RETURN( FXFALSE );
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
#ifdef GLIDE_INIT_HWC
    if (bInfo->fifoInfo.agpFifo) {
      gcFifo->fifoStart = (FxU32 *) bInfo->fifoInfo.agpVirtAddr;
      gcFifo->fifoOffset = bInfo->fifoInfo.agpPhysAddr;
    } else {
#else
    {
#endif
      gcFifo->fifoStart = gc->rawLfb + ( gcFifo->fifoOffset >> 2 );
    }
    gcFifo->fifoEnd   = gcFifo->fifoStart + ( gcFifo->fifoSize >> 2 );

    /* Adjust room values. 
    ** RoomToEnd needs enough room for the jmp packet since we never
    ** allow the hw to auto-wrap. RoomToRead needs to be adjusted so that
    ** we never acutally write onto the read ptr.
    **
    ** fifoRoom is generally the min of roomToEnd and roomToRead, but we
    ** 'know' here that roomToRead < roomToEnd.   
    */
#if USE_PACKET_FIFO
    gcFifo->roomToEnd = gcFifo->fifoSize - FIFO_END_ADJUST;
    gcFifo->fifoRoom  = gcFifo->roomToReadPtr = gcFifo->roomToEnd - sizeof( FxU32 );

          
    /* Set initial fifo state. hw read and sw write pointers at
    ** start of the fifo.
    */
    gcFifo->fifoPtr  = gcFifo->fifoStart;
    gcFifo->fifoRead = HW_FIFO_PTR( FXTRUE );
#endif /* !USE_PACKET_FIFO */

    if ( (void*)gcFifo->fifoPtr != (void*)gcFifo->fifoRead ) {
#ifdef GLIDE_INIT_HWC
        hwcRestoreVideo( bInfo );
#endif
        GDBG_INFO( gc->myLevel, "Initial fifo state is incorrect\n" );
        GR_RETURN( FXFALSE );
    }

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


    GDBG_INFO(80, 
              "Command Fifo:\n"
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

#if defined( TACO_MEMORY_FIFO_HACK ) 
    /* XXXTACOHACK!!! - Insert in-memory fifo */
    gcFifo->vFifoStart = calloc( gcFifo->fifoRoom, 1 );
    gcFifo->vFifoEnd   = gcFifo->vFifoStart + ( gcFifo->fifoRoom >> 2 );
    gcFifo->fifoPtr    = gcFifo->vFifoStart;
    if ( gcFifo->fifoPtr == NULL ) {
        GDBG_INFO( gc->myLevel, "Couldn't allocate in-memory fifo\n" );
        GR_RETURN(FXFALSE);
    }
    GDBG_INFO( 0, "Running with whack in-memory fifo.\n" );
#endif /* defined ( TACO_MEMORY_FIFO_HACK ) */

    /* The hw is now in a usable state from the fifo macros */
    gc->open = FXTRUE;

    /* Setup the arch dependent texture procs */
    gc->archDispatchProcs.texDownloadProcs = _GlideRoot.curTexProcs;

    /*------------------------------------------------------
      GC Init
      ------------------------------------------------------*/
    GDBG_INFO(gc->myLevel, "  GC Init\n");
    initGC( gc );
    

    /*------------------------------------------------------
      3D State Init 
      ------------------------------------------------------*/
    GDBG_INFO( gc->myLevel, "  3D State Init\n");
    
    GDBG_INFO( gc->myLevel, "   Setting default register states\n" );
    gc->state.fbi_config.fbzMode = ( SST_ENRECTCLIP | SST_ENZBIAS );
    
    GDBG_INFO( gc->myLevel, "   Setting up initial draw buffer state\n" );
    REG_GROUP_BEGIN(BROADCAST_ID, leftOverlayBuf, 1, 0x1);
    /* Overlay Hack */
    if (!gc->desktopOverlay)
      REG_GROUP_SET(hw, leftOverlayBuf, gc->buffers[gc->frontBuffer]);
    else
      REG_GROUP_SET(hw, leftOverlayBuf, 0);
    REG_GROUP_END();

    /* Overlay Hack: Set the chroma key to magenta */
    if (gc->desktopOverlay)
    {
      GR_SET_IO (BROADCAST_ID, hw, vidChromaMin, 0x0000F81F);
      GR_SET_IO (BROADCAST_ID, hw, vidChromaMax, 0x0000F81F);
    }

    REG_GROUP_BEGIN(BROADCAST_ID, swapbufferCMD, 1, 0x1); 
    REG_GROUP_SET(hw, swapbufferCMD, 0x0);
    REG_GROUP_END();

#ifdef GLIDE_INIT_HWC    
    REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf); 
    REG_GROUP_SET(hw, colBufferAddr, gc->buffers[gc->curBuffer]);
    REG_GROUP_SET(hw, colBufferStride, gc->strideInTiles | SST_BUFFER_MEMORY_TILED );
    REG_GROUP_SET(hw, auxBufferAddr, gc->buffers[nColBuffers]);
    REG_GROUP_SET(hw, auxBufferStride, gc->strideInTiles | SST_BUFFER_MEMORY_TILED); 
    REG_GROUP_END();
#endif
    
    GDBG_INFO( gc->myLevel, "  Setting all Glide state\n" );
    assertDefaultState();

#if defined( TACO_MEMORY_FIFO_HACK ) 
    _FifoFlush();
#endif /* defined( TACO_MEMORY_FIFO_HACK ) */

    /* --------------------------------------------------------
       Splash Screen
       --------------------------------------------------------*/
    doSplash();

    _GlideRoot.windowsInit = FXTRUE; /* to avoid race with grSstControl() */
    
    GR_RETURN(FXTRUE);
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
GR_ENTRY(grSstWinClose, void, (void))
{
#define FN_NAME "grSstWinClose"
  GR_BEGIN_NOFIFOCHECK("grSstWinClose", 80);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  if ((gc != NULL) && (gc->open)) {
#if GLIDE_INIT_HAL
    /* dpc - 22 may 1997 - FixMe!
     * We need the equivilant stuff in the hal layer too.
     */
#else /* !GLIDE_INIT_HAL */
#if defined(GLIDE3) && (GLIDE3_ALPHA)
    FARPROC oemRestoreVideo;
#endif
    /*--------------------------
      3D Idle
      --------------------------*/
    GDBG_INFO(gc->myLevel, "  3D Idle");
    grSstIdle();

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
    hwcRestoreVideo(gc->bInfo);

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
    if (gc->oemInit) {
      oemRestoreVideo = GetProcAddress(gc->oemInit, "_fxoemRestoreVideo@0");
      if (oemRestoreVideo)
        oemRestoreVideo();
      FreeLibrary(gc->oemInit);
    }
#endif
#endif /* !GLIDE_INIT_HAL */

#ifdef __linux__ /* hwcUnmapBoard() is only in linhwc.c */
    /* Unmap the board */
    hwcUnmapBoard(gc->bInfo);
#endif

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
  }
  gc->open = FXFALSE;

  GR_END();
#undef FN_NAME
} /* grSstWinClose */

GR_ENTRY(grSstControl, FxBool, (GrControl_t code)) 
{
#if 0
  GR_DCL_GC;
#define FN_NAME "grSstControl"  
  GDBG_INFO(41, "%s:  code = 0x%x, windowsInit = %d\n", FN_NAME,
    code, _GlideRoot.windowsInit);
  
  if (_GlideRoot.windowsInit && gc->open) {
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
    /* For VG96, windows haven't been created, so there's no point
     * trying to control it. 
     */
    
    /* Believe it or not, the following code really should be bracketed
       like this.  The reason is that GR_BEGIN_NOFIFOCHECK seg faults
       when grSstControl is called before the Glide window is truly
       initialized.  This is a real concern, as grSstControl is called
       from event loops, which are asynchronous.
       */
    FxU32
      status,
      xRes, yRes,
      clipLeftRight, clipBottomTop;
    
    GR_BEGIN_NOFIFOCHECK(FN_NAME,80);
    
    xRes = initControl(code);
    
    GDBG_INFO((80, "%s:  initControl returned 0x%x\n", FN_NAME, xRes));

    if (!xRes) return FXFALSE;
  
    yRes = xRes >> 16;
    xRes = xRes & 0xffff;
  
    /* !! FIXME +++ this call should properly update the gc->fbStride,
       this information is known only to the init layer and needs to be
       exposed, it is only really necessary to lfblock right now,
       and therefore is returned by initGetBufferPtr */
  
    gc->state.screen_width = xRes;
    gc->state.screen_height = yRes;

    /* Wait for Idle. */
  
    nTries = 0;
    do {
      if (nTries++ >> 999) {
        GDBG_INFO((80, "%s:  returning FALSE after %d checks for idle\n", 
          FN_NAME, nTries));
        return FXTRUE;
      }

      status = GET(hw->status);
        
    } while (status & 0x1);

    /* Set ClipRect Via direct writes */
    _grClipNormalizeAndGenerateRegValues(0, 0, xRes, yRes,
      &clipLeftRight,
      &clipBottomTop);
    SET_DIRECT(clipLeftRight1, clipLeftRight);
    SET_DIRECT(clipBottomTop1, clipBottomTop);

#elif (GLIDE_PLATFORM&GLIDE_HW_SST1)
    return initControl(code);
#elif (GLIDE_PLATFORM & GLIDE_HW_CVG) && !GLIDE_INIT_HAL
    {
      FxBool isValidP = FXTRUE;
      FxBool passFlag;

      switch ( code ) {
      case GR_CONTROL_DEACTIVATE:
        passFlag = FXTRUE;
        break;

      case GR_CONTROL_ACTIVATE:
        passFlag = FXFALSE;
        break;

      default:
        isValidP = FXFALSE;
      }

      if (isValidP) sst1InitVgaPassCtrl(gc->base_ptr, passFlag);
    }
#endif    
  }

  GDBG_INFO(41, "%s:  Returning TRUE\n", FN_NAME);
  return FXTRUE;  
#undef FN_NAME
#else
  return FXTRUE;
#endif
} /* grSstControl */

/*---------------------------------------------------------------------------
**  grSstPerfStats
*/ 
#ifndef GLIDE3_ALPHA
GR_ENTRY(grSstPerfStats, void, (GrSstPerfStats_t *pStats))
{
#define FN_NAME "grSstPerfStats"
  GR_BEGIN_NOFIFOCHECK("grSstPerfStats",83);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",pStats);
  GR_CHECK_F(FN_NAME, !pStats, "NULL pointer");

  pStats->pixelsIn   = GR_GET(hw->stats.fbiPixelsIn);
  pStats->chromaFail = GR_GET(hw->stats.fbiChromaFail);
  pStats->zFuncFail  = GR_GET(hw->stats.fbiZfuncFail);
  pStats->aFuncFail  = GR_GET(hw->stats.fbiAfuncFail);
  pStats->pixelsOut  = GR_GET(hw->stats.fbiPixelsOut);

  GR_END();
#undef FN_NAME
} /* grSstPerfStats */
#endif

/*---------------------------------------------------------------------------
**  grStatsResetPerfStats
*/

#if defined(GLIDE3) && (GLIDE3_ALPHA)
void FX_CSTYLE
_grSstResetPerfStats(void)
#else
GR_ENTRY(grSstResetPerfStats, void, (void))
#endif
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

#if defined(GLIDE3) && (GLIDE3_ALPHA)
FxU32 FX_CSTYLE
_grSstStatus(void)
#else
GR_ENTRY(grSstStatus, FxU32, (void))
#endif
{
#define FN_NAME "grSstStatus"
  FxU32 status;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);

  if (!gc->cmdTransportInfo.autoBump)
    GR_BUMP_N_GRIND;


  status = GR_GET(hw->status);

  /* For effed-up stuff */
  status |= (0xffff << 0xc);

  status &= 0xfffffff;          /* Clear high nibble */

  /* OR in buffers pending */
  if (gc->swapsPending)
    status |= ((grBufferNumPending() & 0x7) << 28);

  GR_RETURN(status);
#undef FN_NAME
}/* grSstStatus */

/*---------------------------------------------------------------------------
**  grSstVideoLine - return current video line number
*/

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
FxU32 FX_CSTYLE
_grSstVideoLine(void)
#else
GR_ENTRY(grSstVideoLine, FxU32, (void))
#endif
{
  FxU32 vline = 1;

  return vline;
}/* grSstVideoLine */

/*---------------------------------------------------------------------------
**  grSstVRetrace - return contents of SST_VRETRACE bit of status register;
*/

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
FxBool FX_CSTYLE
_grSstVRetraceOn(void)
#else
GR_ENTRY(grSstVRetraceOn, FxBool, (void))
#endif
{
  FxU32 status;
  GR_BEGIN_NOFIFOCHECK("grSstVRetraceOn",83);

  status = grSstStatus();

  return ((status & SST_VRETRACE) == 0);

}/* grSstVRetrace */

/*---------------------------------------------------------------------------
** grSstIdle/grFinish
*/
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
GR_ENTRY(grFinish, void, (void))
#define FN_NAME "grFinish"
#else
GR_ENTRY(grSstIdle, void, (void))
#define FN_NAME "grSstIdle"
#endif
{
  FxU32 status;
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  gc->counter = gc->expected_counter = 0x0;

#if GLIDE_DEBUG
  {
    const FxU32 savedLockCount = gc->cmdTransportInfo.lfbLockCount;
    gc->cmdTransportInfo.lfbLockCount = 0;
#endif /* GLIDE_DEBUG */

    REG_GROUP_BEGIN(BROADCAST_ID, nopCMD, 0x1, 0x1);
    REG_GROUP_SET(hw, nopCMD, 0x0);
    REG_GROUP_END();

#if GLIDE_DEBUG
    gc->cmdTransportInfo.lfbLockCount = savedLockCount;
  }
#endif /* GLIDE_DEBUG */

  do {
    status = grSstStatus();
  } while (status & SST_BUSY);

  GR_END();
#undef FN_NAME
} /* grSstIdle */

/*-------------------------------------------------------------------
  Function: grFlush
  Date: 09-Jan-98
  Implementor(s): atai
  Description:

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
GR_ENTRY(grFlush, void, (void))
{
#define FN_NAME "grFlush"
  FxU32 status;
  GR_BEGIN("grFlush", 83, 4, 1);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  GR_SET(BROADCAST_ID, hw, nopCMD, 0x0);

  do {
    status = grSstStatus();
  } while (status & SST_BUSY);

  GR_END();
#undef FN_NAME
} /* grSstIdle */
#endif

/*---------------------------------------------------------------------------
**  grSstIsBusy - find out if the SST is busy or not
*/

#if defined(GLIDE3) && defined(GLIDE_ALPHA)
FxBool FX_CSTYLE
grSstIsBusy(void)
#else
GR_ENTRY(grSstIsBusy, FxBool, (void))
#endif
{
#define FN_NAME "grSstIsBusy"
  static FxBool nopP = FXTRUE;
  FxBool busy;
  GR_BEGIN_NOFIFOCHECK("grSstIsBusy", 80);

  /* dpc - 22 may 1997 - FixMe!
   * Seems like the simplest way to do it, but is this really the way
   * to do it?  
   */
  if (nopP) {
    GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
    GR_SET(BROADCAST_ID, hw, nopCMD, 0);
    GR_CHECK_SIZE();
  }

#if defined(GLIDE3) && (GLIDE3_ALPHA)
  busy = ((_grSstStatus() & SST_BUSY) != 0);
#else
  busy = ((grSstStatus() & SST_BUSY) != 0);
#endif
  nopP = !busy;

  GDBG_INFO(84,"grSstIsBusy() => 0x%x\n", busy);

  return busy;
#undef FN_NAME
}/* grSstIsBusy */

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

  hwcGammaRGB(gc->bInfo, gamma, gamma, gamma);

#endif /* !GLIDE_INIT_HAL */

  GR_END();
} /* grGammaCorrectionValue */

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
  fbzMode = gc->state.fbi_config.fbzMode;
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

  gc->state.fbi_config.fbzMode = fbzMode;

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

