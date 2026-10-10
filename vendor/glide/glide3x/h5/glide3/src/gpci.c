/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONL
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGH
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DF
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com).
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR 
** FULL TEXT OF THE NON-WARRANTY PROVISIONS. 
**
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT T
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS I
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FA
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS O
** THE UNITED STATES. 
**
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVE
**
** $Log:
**  44   3dfx      1.34.1.2.1.511/08/00 Drew McMinn     Create initialise read and
**       use useAppGamma flag, to allow us to disable applications changing gamma
**       values.
**  43   3dfx      1.34.1.2.1.410/11/00 Brent           Forced check in to enforce
**       branching.
**  42   3dfx      1.34.1.2.1.309/26/00 Andy Hanson     Add environment variable of
**       glide group so they can do wacky things safely.
**  41   3dfx      1.34.1.2.1.208/29/00 Jonny Cochrane  Some 8x FSAA code
**  40   3dfx      1.34.1.2.1.108/01/00 Andy Hanson     Fixed issue with source
**       ignoring setting of GLIDE_SPLASH define.
**       Fixed issue with setting of splash state when it wasn't going to be run.
**  39   3dfx      1.34.1.2.1.006/28/00 troy thornton   added initialization of
**       correct jitter values
**  38   3dfx      1.34.1.2    06/20/00 Joseph Kain     Fixed errors introduced in
**       merge.
**  37   3dfx      1.34.1.1    06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  36   3dfx      1.34.1.0    06/15/00 Bill White      Merged changes to support
**       Linux.
** 
**  35   3dfx      1.34        04/21/00 Kenneth Dyke    Magic FX_GLIDE_NO_HW
**       support.
**  34   3dfx      1.33        04/13/00 Kenneth Dyke    Added support for new style
**       2-sample AA mode.
**  33   3dfx      1.32        04/10/00 Kenneth Dyke    Added magical screenshot
**       hotkey.
**  32   3dfx      1.31        03/30/00 Kenneth Dyke    Changed defaults for column
**       band width and SLI band height.
**  31   3dfx      1.30        03/25/00 Adam Briggs     added support for
**       SSTH3_SLI_AA_CONFIGURATION (the env var the control panel uses to force AA
**       modes)
**  30   3dfx      1.29        03/24/00 Chris Dow       Added code to fence every n
**       writes (not to exceed 0x10000) where n is either 0x10000 or indicated by
**       the environment variable FX_GLIDE_FENCE_LIMIT
** 
**  29   3dfx      1.28        03/23/00 Kenneth Dyke    Change 2PPC band height
**       default to 2.
**  28   3dfx      1.27        03/21/00 Kenneth Dyke    Performance tweaks to
**       default env vars.  Added env for disabling SLI band height clamp.
**  27   3dfx      1.26        03/16/00 Adam Briggs     default FX_GLIDE_WAX_ON to
**       1
**  26   3dfx      1.25        03/16/00 Kenneth Dyke    User-adjustable LOD bias
**       value.
**  25   3dfx      1.24        03/14/00 Adam Briggs     enable analog sli in 2X
**       modes or when forced
**  24   3dfx      1.23        03/08/00 Kenneth Dyke    New use isMapped boardInfo
**       flag instead of broken gc flag.
**  23   3dfx      1.22        03/06/00 Kenneth Dyke    Added backdoor hack to
**       toggle AA on and off on the fly.
**  22   3dfx      1.21        02/18/00 Adam Briggs     added FX_GLIDE_WAX_ON var
**       which must be set to 1 to enable the newly uncommented SLI WAX buffer
**       clears
**  21   3dfx      1.20        02/08/00 Kenneth Dyke    Initialize sliBandHeight in
**       the proper place.
**  20   3dfx      1.19        01/31/00 Adam Briggs     changed the IS_NAPALM macro
**       to cooperate with the display driver version of the same
**  19   3dfx      1.18        01/31/00 Adam Briggs     Changed all device ID magic
**       numbers to use those defined in fxhal.h & added IS_NAPALM macro to test
**       against device ID range
**  18   3dfx      1.17        01/23/00 Adam Briggs     set & recognize the SSTTYPE
**       var for Voodoo4
**  17   3dfx      1.16        01/21/00 Adam Briggs     some changes to get the
**       correct linear mappings for slave regs and use them to sync the fifos in
**       sli mode
**  16   3dfx      1.15        01/18/00 Kenneth Dyke    Added environment variables
**       for configuring AA jitter offsets.
**  15   3dfx      1.14        01/16/00 Kenneth Dyke    Triangle column width
**       tweaking.
**  14   3dfx      1.13        01/16/00 Kenneth Dyke    Changed defaults for 32-bit
**       and AA environment variables.
**  13   3dfx      1.12        01/07/00 Adam Briggs     Moved freeThreadStorage
**       from grShutdown to the DllMain process detach code.  This fixes PRS#12190,
**       12192 and 12196.
**  12   3dfx      1.11        12/10/99 Kenneth Dyke    Ugh.  Undid some changes
**       for backwards compatibility.
** 
**  11   3dfx      1.10        12/08/99 Kenneth Dyke    Fixed stupid macro problem.
**  10   3dfx      1.9         12/08/99 Kenneth Dyke    Environment variable
**       checking is now actually sane and based on the value of the variable and
**       not just its presence.
**  9    3dfx      1.8         12/03/99 Adam Briggs     set sli bandheight to
**       FX_GLIDE_SLI_BAND_HEIGHT instead of zero(!)
**  8    3dfx      1.7         11/11/99 Adam Briggs     My bad:  Apparently V3
**       cards have zero chips on them, so don't even bother with the first step
**       through the SLI slave reg loops.
**  7    3dfx      1.6         11/09/99 Larry  warner   Make the world safe for h3.
**  6    3dfx      1.5         11/09/99 Adam Briggs     Added support for reading
**       the Status reg on slave chips in order to form a more perfect flush
**       function.
**  5    3dfx      1.4         11/05/99 Anthony tai     added
**       FX_GLIDE_SWAPPENDINGCOUNT. Default=1. Range 0-3
**  4    3dfx      1.3         10/08/99 Adam Briggs     Supported FX_GLIDE_BPP &
**       FX_GLIDE_AA_SAMPLE environment vars so the user can override the pixel
**       format in grSstWinOpen calls
**  3    3dfx      1.2         09/22/99 Larry  warner   Created download procedures
**       for FXT1 format.
**  2    3dfx      1.1         09/13/99 Anthony tai     added napalm tmu case
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 
** 74    8/31/99 5:48p Atai
** fixed chip count
** 
** 73    8/19/99 7:55p Larryw
** FXT1  Tsplit changes.
** 
** 72    8/05/99 5:03p Larryw
** FXT1 format works now.
** 
** 71    8/04/99 10:27a Atai
** hack to enable chip count
** 
** 70    7/29/99 7:07p Larryw
** Pave the way for FXT1 (but not quite there yet).
** 
** 69    7/26/99 12:11p Atai
** initialize pci registers for sli/aa
** 
** 68    7/21/99 3:59p Atai
** added sliBandHeight env var
** 
** 67    7/18/99 11:11a Atai
** added 2 pixel per clock
** 
** 66    7/14/99 9:39a Atai
** direct register write for glide3x
** test04 can do 4 sample aa (2 chips)
** 
** 65    7/06/99 5:00p Atai
** sync with tot csim
** 
** 64    7/06/99 2:46p Atai
** initailize gbc env var
** 
** 63    6/24/99 7:13p Atai
** initialize combine_ext_mode with 0
** 
** 62    6/18/99 10:39p Atai
** added csim env var to specify chip #
** 
** 61    6/14/99 5:16p Larryw
** Added 32-bit texture format support.
** 
** 60    6/08/99 6:04p Stb_sbrooks
** Added fifoSize environment var
** 
** 59    6/03/99 5:23p Russp
** Add in some ifdef'd code to allow variable texture alignment
** 
** 58    6/03/99 12:15p Kcd
** PowerPC still uses C vertex routines when C triangle setup is turned
** off.
** 
** 57    6/03/99 11:18a Atai
** force deviceID to 7 for code development
** 
** 56    5/28/99 3:45p Atai
** added base_ptr value for grTexMaxAddress
** 
** 55    5/21/99 9:22a Stb_gkincade
** Put in check for : is GETENV(SSTH3_ALPHADITHERMODE) a null string?
** 
** 53    5/20/99 9:03a Atai
** is GETENV("SSTH3_ALPHADITHERMODE")  a null string?
** 
** 52    5/13/99 2:38p Stb_gkincade
** Added user support for turning on/off dither substraction
** 
** 51    4/29/99 10:17p Atai
** disable context code in csim
** 
** 50    4/22/99 3:53p Dow
** Alt-Tab on NT
** 
** 49    4/16/99 2:55p Kcd
** PowerPC PCI Bump & Grind
** 
** 48    4/10/99 2:21p Atai
** fixed sstRegs
** 
** 47    4/10/99 1:54p Atai
** added ioReg, cReg, gReg, and rawLfb
** 
** 46    4/07/99 7:18p Atai
** added uma extension
** 
** 45    4/01/99 7:55p Peter
** made names and comments more meaningful
** 
** 44    3/31/99 9:02p Dow
** context loosing means no writing to hw
** 
** 43    3/30/99 3:21p Atai
** use hwcGetenv if hwc is used
** 
** 42    3/17/99 5:08p Peter
** removed whacky stuff now that the command fifo threshold stuff appears
** to make all happy (including the k7)
** 
** 41    3/14/99 1:48p Peter
** cmd's bng optimization
** 
** 40    3/12/99 2:24p Dow
** Remove 3DNow for K7 (temp workaround)
** 
** 39    2/18/99 3:59p Kcd
** Fixed _grGlideInitEnvironment to build on non-Win32 systems.
** 
** 38    2/17/99 6:38p Atai
** Fixed hack which broke grGet(GR_NUM_BOARDS,,)
** 
** 37    2/11/99 1:38p Atai
** sync buffer swap pending code, the right way.
** 
** 36    1/25/99 6:33p Peter
** removed some cruft I saw when cleaning up tiled textures
** 
** 35    1/20/99 10:54a Dow
** Voodoo 3 id for apps
** 
** 34    12/14/98 2:20p Dow
** $#@!
** 
** 33    12/07/98 11:33a Peter
** norbert's re-fixes of my merge
** 
** 32    12/03/98 11:12p Dow
** added GLIDE_FGETENV for floating-point
** 
** 31    12/03/98 10:39p Dow
** Removed registry code
** 
** 30    12/02/98 2:06p Dow
** Fixed Control Panel stuff for W95 & WNT
** 
** 29    12/02/98 1:00p Atai
** Hack. Setup up triangle and vertex array routine
** 
** 28    12/02/98 11:11a Dow
** Fixed Control Panel
** 
** 27    12/02/98 10:38a Dow
** Fixed Registry path for NT
** 
** 26    11/18/98 7:50p Atai
** use env var FX_GLIDE_NUM_TMU
** 
** 25    11/17/98 7:03p Atai
** added env var "FX_GLIDE_DISABLE_TMU1"
** 
** 24    11/15/98 3:21a Atai
** first attempt to make 2 tmus work in H4 glide3x full screen mode, just
** in time check-in for comdex demo. warning: the code is not completed
** yet.
** 
** 23    11/09/98 3:34p Mikec
** Glide read registry on NT.
** 
** 22    11/02/98 5:34p Peter
** tls per thread for fullscreen contexts
** 
** 21    10/16/98 1:24p Peter
** c trisetup fixes
** 
** 20    10/14/98 3:38p Dow
** Gamma stuff
** 
** 19    10/12/98 9:51a Peter
** dynamic 3DNow!(tm)
** 
** 18    9/04/98 11:35a Peter
** re-open fix for nt (thanks to taco/rob/nt bob)
** 
** 17    8/29/98 4:35p Dow
** Thread Optimization stuff
** 
** 16    8/27/98 9:27p Atai
** fix env variable for glide3x
** 
** 15    8/21/98 3:48p Jdt
** Added DLLMAIN function to restore desktop during critical failure.
** 
** 14    8/03/98 6:40a Jdt
** multi-thread changes
** 
** 13    7/01/98 12:41p Jdt
** Removed global p6FenceVar
** 
** 12    5/28/98 2:07p Peter
** banshee merge
** 
** 11    5/06/98 1:51p Peter
** more real banshee for glide3
** 
** 10    5/05/98 2:22p Peter
** banshee is really banshee
** 
** 9     4/30/98 5:01p Peter
** first pass glide3 merge
** 
** 7     4/22/98 4:57p Peter
** glide2x merge
** 
** 6     2/10/98 7:04p Atai
** change module name
** 
** 5     1/30/98 4:51p Peter
** fixed sli-detect for multiple pairs
** 
** 4     1/30/98 4:29p Peter
** no uswc for sli slave
** 
** 3     1/20/98 11:03a Peter
** env var to force triple buffering
 * 
 * 2     1/16/98 5:41p Peter
 * fixed sense of lod_dither
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 92    1/14/98 10:22a Peter
 * no more hacks
 * 
 * 91    1/08/98 7:09p Peter
 * real hw stuff modulo makefile change
 * 
 * 90    1/07/98 11:18a Atai
 * remove GrMipMapInfo and GrGC.mm_table in glide3
 * 
 * 89    1/07/98 10:22a Peter
 * lod dithering env var
 * 
 * 88    12/17/97 10:08a Peter
 * fast system comdex twiddling
 * 
 * 87    12/09/97 4:20p Peter
 * 0x100 fbiRev ofset for v2
 * 
 * 86    12/09/97 12:20p Peter
 * mac glide port
 * 
 * 85    12/05/97 4:26p Peter
 * watcom warnings
 * 
 * 84    12/03/97 2:36p Peter
 * upped comdex reset defaults
 * 
 * 83    12/02/97 9:48a Dow
 * Removed some spurious code I inadvertantly added.
 * 
 * 82    11/21/97 6:24p Dow
 * Banshee Lying about being Rush stuf
 * 
 * 81    11/21/97 11:19a Dow
 * Made Banshee report Voodoo2
 * 
 * 80    11/20/97 6:39p Peter
 * fixed direct_exec w/ csim
 * 
 * 79    11/19/97 2:49p Peter
 * env vars in registry for win32
 * 
 * 78    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 77    11/15/97 7:43p Peter
 * more comdex silliness
 * 
 * 76    11/14/97 11:10p Peter
 * open vs hw init confusion
 * 
 * 75    11/14/97 5:02p Peter
 * more comdex stuff
 * 
 * 74    11/14/97 12:09a Peter
 * comdex thing and some other stuff
 * 
 * 73    11/12/97 9:54p Peter
 * fixed all the effage from new config
 * 
 * 72    11/12/97 9:37p Dow
 * Textures on Banshee half work
 * 
 * 71    11/12/97 9:22a Dow
 * h3 mods
 * 
 * 70    11/08/97 3:34p Peter
 * fixed stupid gdbg_info crasher
 * 
 * 69    11/04/97 4:00p Dow
 * Banshee Mods
 * 
 * 68    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 67    11/01/97 12:11p Pgj
 * glide.dll ---> glide2x.dll
 * 
 * 66    10/31/97 8:53a Peter
 * last lying change, really
 * 
 * 65    10/30/97 3:42p Peter
 * protected the last bit of nonsense
 * 
 * 64    10/30/97 3:37p Peter
 * spoof sst1 stuff
 * 
 * 63    10/29/97 2:45p Peter
 * C version of Taco's packing code
 * 
 * 62    10/23/97 5:28p Peter
 * sli fifo thing
 * 
 * 61    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 60    9/10/97 10:13p Peter
 * fifo logic from GaryT, non-normalized fp first cut
 * 
 * 59    9/05/97 5:29p Peter
 * changes for direct hw
 * 
 * 58    9/01/97 3:18p Peter
 * correct integer rounding for pts
 * 
 * 57    8/30/97 5:59p Tarolli
 * init and hal fixups
 * 
 * 56    8/30/97 1:19p Peter
 * first cut at using blit to clear, more to come to do inner rects
 * 
 * 55    8/18/97 3:52p Peter
 * pre-hw arrival fixes/cleanup
 * 
 * 54    7/30/97 2:42p Peter
 * shared and sanitized
 * 
 * 53    7/28/97 2:41p Peter
 * turned sli code back on for cvg, but waiting for hal
 * 
 * 52    7/25/97 11:40a Peter
 * removed dHalf, change field name to match real use for cvg
 * 
 * 51    7/08/97 2:47p Peter
 * fixed merge stupidity from last checkin
 * 
 * 50    7/02/97 12:28p Peter
 * removed spurious NOP, tex dl
 * 
 * 49    6/24/97 4:02p Peter
 * proper cmd fifo placement
 * 
 * 48    6/23/97 4:46p Peter
 * fixed my effage
 * 47    6/23/97 4:43p Peter
 * cleaned up #defines etc for a nicer tree
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

#if GLIDE_PLATFORM & GLIDE_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if (GLIDE_PLATFORM & GLIDE_SST_SIM)
#ifdef HAL_CSIM
#if H3
#include <h3regs.h>
#endif
#include <csim.h>
#elif HSIM
#include <gsim.h>
#endif
#endif /* (GLIDE_PLATFORM & GLIDE_SST_SIM) */

#if GLIDE3
#define kRevisionOffset 0x00UL
#else
#define kRevisionOffset 0x100UL
#endif

#ifdef GLIDE_TEST_TEXTURE_ALIGNMENT
FxU32 SST_TEXTURE_ALIGN;
#endif

/* null function procs that will get called when !gc->contextP. */
static FxI32 FX_CALL
_trisetup_null(const void* a, const void* b, const void* c)
{
  /* Not really accurate, but pretend its been culled for some reason */
  return 0;
}

static void FX_CALL
_grDrawVertexList_null(FxU32 pkType, FxU32 type, FxI32 mode, FxI32 count, void* ptrs)
{
  /* Do Nothing */
}

static void FX_CALL
_grDrawTriangles_null(FxI32 mode, FxI32 count, void* vPtrs)
{
  /* Do Nothing */
}

static void FX_CALL
_grTexDownload_null(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                    const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                    void* texData)
{
  /* Do Nothing */
}

/* Collection of all of the known procs for a given system.
 *   - arch specialization
 *   - grState valid checking
 *   - coordinate system (window/clip)
 *   - culling mode (nocull/cull)
 */
static GrTriSetupProc _triSetupProcs[][2][2][2] = 
{
  /* Default arch procs */
  {
    /* Window coords */
    {
      { _trisetup_Default_win_nocull_valid,   _trisetup_Default_win_cull_valid },
      { _trisetup_Default_win_nocull_invalid, _trisetup_Default_win_cull_invalid },
    },

    /* Clip coordinates */
    {
      { _vptrisetup_cull, _vptrisetup_cull },
      { _vptrisetup_cull, _vptrisetup_cull },
    },
  },
#if GL_AMD3D
  /* 3DNow!(tm) Procs */
  {
    /* Window coords */
    {
      { _trisetup_3DNow_win_nocull_valid,  _trisetup_3DNow_win_cull_valid },
      { _trisetup_3DNow_win_nocull_invalid,  _trisetup_3DNow_win_cull_invalid },
    },
    /* Clip coordinates */
    {
      { _trisetup_clip_coor_thunk, _trisetup_clip_coor_thunk },
      { _trisetup_clip_coor_thunk, _trisetup_clip_coor_thunk },
    },
  },
#endif /* GL_AMD3D */
#if GL_SSE
  /* SSE Procs */
  {
    /* Window coords */
    {
      { _trisetup_SSE_win_nocull_valid,  _trisetup_SSE_win_cull_valid },
      { _trisetup_SSE_win_nocull_invalid,  _trisetup_SSE_win_cull_invalid },
    },
    /* Clip coordinates */
    {
      { _trisetup_SSE_clip_coor_thunk, _trisetup_SSE_clip_coor_thunk },
      { _trisetup_SSE_clip_coor_thunk, _trisetup_SSE_clip_coor_thunk },
    },
  },
#endif /* GL_SSE */
  /* null arch procs */
  {
    /* Window coords */
    {
      { (GrTriSetupProc) _trisetup_null, (GrTriSetupProc) _trisetup_null },
      { (GrTriSetupProc) _trisetup_null, (GrTriSetupProc) _trisetup_null },
    },

    /* Clip coordinates */
    {
      { (GrTriSetupProc) _trisetup_null, (GrTriSetupProc) _trisetup_null },
      { (GrTriSetupProc) _trisetup_null, (GrTriSetupProc) _trisetup_null },
    },
  },
};

/* NB: This is the only set of asm specializations taht needs to be
 * unset for C_TRISETUP. Currently, teh grDrawTriangle code will only
 * vector to the asm code if C_TRISETUP is not set.  
 */
#if GLIDE_USE_C_TRISETUP || __POWERPC__
static GrVertexListProc _vertexListProcs[][2] = {
  { _grDrawVertexList, _grDrawVertexList },
#if GL_AMD3D
  { _grDrawVertexList, _grDrawVertexList },
#endif /* GL_AMD3D */
#if GL_SSE
  { _grDrawVertexList, _grDrawVertexList },
#endif /* GL_SSE */
  { _grDrawVertexList_null, _grDrawVertexList_null },
};
#else /* Use asm code */
static GrVertexListProc _vertexListProcs[][2] = {
  { _drawvertexlist, _vpdrawvertexlist },
#if GL_AMD3D
  { _grDrawVertexList_3DNow_Window, _grDrawVertexList_3DNow_Clip },
#endif /* GL_AMD3D */
#if GL_SSE
  { _grDrawVertexList_SSE_Window, _grDrawVertexList_SSE_Clip },
#endif /* GL_SSE */
  { _grDrawVertexList_null, _grDrawVertexList_null },
};
#endif /* Use asm code */

static GrTexDownloadProc _texDownloadProcs[][4][5] = 
{
  /* Default Procs */
  { 
    {
      _grTexDownload_Default_4_4,
      _grTexDownload_Default_4_8,
      _grTexDownload_Default_4_WideS,
      _grTexDownload_Default_4_WideS,
      _grTexDownload_Default_4_WideS
    }, 
    {
      _grTexDownload_Default_8_1, 
      _grTexDownload_Default_8_2,
      _grTexDownload_Default_8_4,
      _grTexDownload_Default_8_WideS,
      _grTexDownload_Default_8_WideS
    }, 
    {
      _grTexDownload_Default_16_1,
      _grTexDownload_Default_16_2,
      _grTexDownload_Default_16_WideS,
      _grTexDownload_Default_16_WideS,
      _grTexDownload_Default_16_WideS
    },
    {
      _grTexDownload_Default_32_1, /* unused */
      _grTexDownload_Default_32_1,
      _grTexDownload_Default_32_WideS,
      _grTexDownload_Default_32_WideS, /* unused */
      _grTexDownload_Default_32_WideS
    }
  },
#if GL_AMD3D
  { 
    {
      _grTexDownload_Default_4_4,
      _grTexDownload_Default_4_8,
      _grTexDownload_3DNow_MMX,
      _grTexDownload_3DNow_MMX,
      _grTexDownload_3DNow_MMX
    }, 
    { 
      _grTexDownload_Default_8_1, 
      _grTexDownload_Default_8_2, 
      _grTexDownload_Default_8_4, 
      _grTexDownload_3DNow_MMX, 
      _grTexDownload_3DNow_MMX
    },
    {
      _grTexDownload_Default_16_1,
      _grTexDownload_Default_16_2,
      _grTexDownload_3DNow_MMX,
      _grTexDownload_3DNow_MMX,
      _grTexDownload_3DNow_MMX
      },
    {
      _grTexDownload_Default_32_1, /* unused */
      _grTexDownload_Default_32_1,
      _grTexDownload_3DNow_MMX,
      _grTexDownload_3DNow_MMX, /* unused */
      _grTexDownload_3DNow_MMX
    }
  },
#endif /* GL_AMD3D */
#if GL_MMX
  { 
    {
      _grTexDownload_Default_4_4,
      _grTexDownload_Default_4_8,
      _grTexDownload_MMX,
      _grTexDownload_MMX,
      _grTexDownload_MMX
    }, 
    { 
      _grTexDownload_Default_8_1, 
      _grTexDownload_Default_8_2, 
      _grTexDownload_Default_8_4, 
      _grTexDownload_MMX, 
      _grTexDownload_MMX
    },
    {
      _grTexDownload_Default_16_1,
      _grTexDownload_Default_16_2,
      _grTexDownload_MMX,
      _grTexDownload_MMX,
      _grTexDownload_MMX
    },
    {
      _grTexDownload_Default_32_1, /* unused */
      _grTexDownload_Default_32_1,
      _grTexDownload_MMX,
      _grTexDownload_MMX, /* unused */
      _grTexDownload_MMX
    }
  },
#endif /* GL_MMX */
#if GL_SSE2
  { 
    {
      _grTexDownload_Default_4_4,
      _grTexDownload_Default_4_8,
      _grTexDownload_SSE2_64,
      _grTexDownload_SSE2_128,
      _grTexDownload_SSE2_128
    }, 
    { 
      _grTexDownload_Default_8_1, 
      _grTexDownload_Default_8_2, 
      _grTexDownload_Default_8_4, 
      _grTexDownload_SSE2_64, 
      _grTexDownload_SSE2_128
    },
    {
      _grTexDownload_Default_16_1,
      _grTexDownload_Default_16_2,
      _grTexDownload_SSE2_64,
      _grTexDownload_SSE2_128,
      _grTexDownload_SSE2_128
    },
    {
      _grTexDownload_Default_32_1, /* unused */
      _grTexDownload_Default_32_1,
      _grTexDownload_SSE2_64,
      _grTexDownload_SSE2_128, /* unused */
      _grTexDownload_SSE2_128
    }
  },
#endif /* GL_SSE2 */
  /* NULL download procs */
  { 
    {
      _grTexDownload_null, 
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null
    }, 
    {
      _grTexDownload_null, 
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null
    }, 
    {
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null
    },
    {
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null
    }
  },
};

/* Some macros to prevent RSI */
#define GC      _GlideRoot.GCs[ctx]
#define SST     _GlideRoot.hwConfig.SSTs[ctx]

#define REGSTR_PATH_3DFXSW              "Software\\3Dfx Interactive\\Voodoo2"
#define REGSTR_PATH_GLIDE               REGSTR_PATH_3DFXSW"\\Glide"


/*-------------------------------------------------------------------
  Function: _grSstDetectResources
  Date: --
  Implementor(s): Dow, Gmt, Jdt
  Library: Glide
  Description:
  Discover devices on the PCI bus.
  Discover configuration of detected devices.
  Initialize all Glide GC's 

  Recognized devices depend upon compile time flags

  This code should NOT initialize the hardware 
  any more than is necessary for discovery of 
  configuration

  Arguments: none
  Return: 
  FXTRUE  - at least one device was detected
  FXFALSE - no devices were detected.
  -------------------------------------------------------------------*/
FxBool 
_grSstDetectResources(void)
{
  static FxBool calledP = FXFALSE;
  FxBool rv = FXFALSE;
  FxU32 ctx;

  GDBG_INFO(280, "_grSstDetectResources()\n");

  if (!calledP) {
#if GLIDE_INIT_HAL
  FxU32 count = HAL_MAX_BOARDS;
  FxU32 chipCount = 1;

  /* The first time through the init code we need to map 
   * all of the boards. Future calls can just grab this
   * info out of the halInfo that we have here.
   */
    FxU32     device;
    HalInfo* halInfo = fxHalInit(0);
    if (halInfo == NULL) goto __errExit;
    
    {
      /*
      ** hack alert!!
      ** by default, there is no device around so we hack the device 
      ** number in here (copy code from diag).
      */
      char* envChipNum = GETENV("FX_GLIDE_NUM_CHIPS");
      FxBool useMultiFunctionDevices=FXFALSE;
      FxU32 busNumber, deviceNumber, functionNumber;
      FxU32 counter;

      if (envChipNum)
        chipCount = atoi(envChipNum);

      switch(chipCount) {
      case 4:
        busNumber      = 3;
        deviceNumber   = 2;
        functionNumber = 0;
        halInfo->boardInfo[2].pciBusNumber = busNumber;
        halInfo->boardInfo[2].pciDeviceNumber = deviceNumber;
        halInfo->boardInfo[2].pciFunctionNumber = functionNumber; 
        halInfo->boardInfo[2].deviceNumber = busNumber * 32 + deviceNumber;

        busNumber      = 3;
        deviceNumber   = 2;
        functionNumber = 1;
        halInfo->boardInfo[3].pciBusNumber = busNumber;
        halInfo->boardInfo[3].pciDeviceNumber = deviceNumber;
        halInfo->boardInfo[3].pciFunctionNumber = functionNumber; 
        halInfo->boardInfo[3].deviceNumber = busNumber * 32 + deviceNumber;

      case 2:
        busNumber      = 2;
        deviceNumber   = 1;
        functionNumber = 1;
        halInfo->boardInfo[1].pciBusNumber = busNumber;
        halInfo->boardInfo[1].pciDeviceNumber = deviceNumber;
        halInfo->boardInfo[1].pciFunctionNumber = functionNumber; 
        halInfo->boardInfo[1].deviceNumber = busNumber * 32 + deviceNumber;

        useMultiFunctionDevices=FXTRUE;
      case 1:
      default:
        busNumber      = 2;
        deviceNumber   = 1;
        functionNumber = 0;
        halInfo->boardInfo[0].pciBusNumber = busNumber;
        halInfo->boardInfo[0].pciDeviceNumber = deviceNumber;
        halInfo->boardInfo[0].pciFunctionNumber = functionNumber; 
        halInfo->boardInfo[0].deviceNumber = busNumber * 32 + deviceNumber;
        break;
      }
      /* Initialize the csim boards/chips */
      for(counter=0; counter<chipCount; counter++) {
        halInfo->boardInfo[counter].sstCSIM = csimInit(counter);      
        if(useMultiFunctionDevices)
          csimMakeMultiFunctionDevice(halInfo->boardInfo[counter].sstCSIM);
        CSIM_PRIVATE(halInfo->boardInfo[counter].sstCSIM)->environment.chipCount = chipCount;
        CSIM_PRIVATE(halInfo->boardInfo[counter].sstCSIM)->environment.sliBandHeight 
          = _GlideRoot.environment.sliBandHeight;
        
      }
    }
    
#if (GLIDE_PLATFORM & GLIDE_SST_SIM)
    /* The simulator can support any number of boards through
     * successive calls to fxHalMapBoard.
     */
    {
      const char* envBoardNum = GETENV("FX_SIM_BOARDS");
      FxU32 temp;
        
      count = (((envBoardNum != NULL) && (sscanf(envBoardNum, "%ld", &temp) == 1)) 
               ? temp
               : 1);
      if (count > HAL_MAX_BOARDS) {
        GDBG_INFO(0, "Error: FX_SIM_BOARDS(%ld) > %d. Using %d.\n",
                  count, HAL_MAX_BOARDS, HAL_MAX_BOARDS);
        count = HAL_MAX_BOARDS;
      }
      count *= chipCount;
    }
#endif /* (GLIDE_PLATFORM & GLIDE_SST_SIM) */


    for(ctx = device = 0; device < count; device++) {
      SstRegs* devRegs;
      devRegs = fxHalMapBoard(device);
    }

    for(ctx = device = 0; device < count; device++) {
      /* See RSI-prevention macros for usage of [ctx] */
      const FxDeviceInfo* curDev = NULL;
      FxBool regInitP = FXFALSE;
      SstRegs* devRegs;

      devRegs = fxHalMapBoard(device);
      if ((device % chipCount) == 0)
        GC.is_master = FXTRUE;
      else
        GC.is_master = FXFALSE;

      GC.chipCount  = chipCount;
      GC.sliBandHeight = _GlideRoot.environment.sliBandHeight;

      curDev = halInfo->boardInfo + device;

      GC.halInfo = halInfo;

      if (devRegs != NULL) {
        FxU32 tmuMem = 0x00;

        SST.type = GR_SSTTYPE_Banshee;

        if (!fxHalInitRegisters(curDev->virtAddr[0])) goto __errRegFailure;
          
#if USE_PACKET_FIFO && GLIDE_DEBUG && HAL_CSIM
        if ((halInfo->csim == 1) && (GETENV("FX_FIFO_DIRECT_EXEC") != NULL)) {
          //            halInfo->csim = -1;
          GDBG_INFO(80, "Turning on direct fifo execution\n");
        }
#endif /* USE_PACKET_FIFO && GLIDE_DEBUG && HAL_CSIM */

#ifdef DIRECT_IO
        GC.sstRegs = (SstRegs *) (curDev->physAddr[0] + 0x200000);
        GC.ioRegs = (SstIORegs *) (curDev->physAddr[0]);
        GC.cRegs = (SstCRegs *) (curDev->physAddr[0] + 0x80000);
        GC.rawLfb = (FxU32 *) curDev->physAddr[1];
#endif /* DIRECT_IO */

        /* This device is ready to go. */
        regInitP = FXTRUE;

        /* Set up pointers to the various address spaces within the hw */
        GC.base_ptr  = (FxU32*)SST_BASE_ADDRESS(curDev->physAddr[0]);
        GC.reg_ptr   = (FxU32*)devRegs;
        GC.lfb_ptr   = (FxU32*)SST_LFB_ADDRESS(curDev->physAddr[0]);
        GC.tex_ptr   = (FxU32*)SST_TEX_ADDRESS(curDev->physAddr[0]);
        
        GC.sstRegs = devRegs;
        GC.ioRegs = (SstIORegs *) SST_IO_ADDRESS(devRegs);
        GC.cRegs = (SstCRegs *) SST_CMDAGP_ADDRESS(devRegs);
        GC.gRegs = (SstGRegs *) SST_GUI_ADDRESS(devRegs);
#define SST_RAW_LFB_ADDRESS(sst)    (SST_RAW_LFB_OFFSET+SST_BASE_ADDRESS(sst))
        GC.rawLfb = (FxU32 *) SST_RAW_LFB_ADDRESS(devRegs);
        /* GC.rawLfb = (FxU32 *) SST_LFB_ADDRESS(devRegs); */
#ifdef FX_GLIDE_NAPALM
        /* force the deviceID to 7 for code development */
        {
          static hwcBoardInfo tmpbInfo;
          GC.bInfo = &tmpbInfo;
          GC.bInfo->pciInfo.deviceID = SST_DEVICE_ID_AP_OEM ;
        }
#endif

        /* Video parameters */
        GC.grSstRez     = GR_RESOLUTION_NONE;
        GC.grSstRefresh = curDev->fbiVideoRefresh;

        /* Chip configuration */
        GC.num_tmu              = curDev->numberTmus;
        GC.fbuf_size            = curDev->fbiMemSize;
        
        _GlideRoot.hwConfig.num_sst++;
        SST.sstBoard.SST96Config.fbRam    = curDev->fbiMemSize;
        SST.sstBoard.SST96Config.nTexelfx = curDev->numberTmus;

        {
          const FxU32 curTmuMemSize = 0x2;
              
          SST.sstBoard.SST96Config.tmuConfig.tmuRev = (2 + kRevisionOffset);
              
          SST.sstBoard.SST96Config.tmuConfig.tmuRam = curTmuMemSize;
          tmuMem += curTmuMemSize;

          /* Clear the tmu state */
          memset(&GC.tmu_state[0], 0, sizeof(GC.tmu_state[0]));       
          GC.tmu_state[0].total_mem = (curTmuMemSize << 20);
              
          GC.tmu_state[0].ncc_mmids[0] = 
            GC.tmu_state[0].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;
        }
          
        rv = FXTRUE;
        ctx++;
      }

      __errRegFailure:
      /* Either this is not the hw we're expecting, or we could not
       * init/map the board for some reason. Either way try to cleanup.  
       */
      if (!regInitP && (devRegs != NULL)) {
        fxHalShutdown(devRegs);
      }
    }

    /* Done setting up. Don't do the silly mapping thing again. */
#else  /* GLIDE_INIT_HWC */
    /* There's a left brace before the #if */
    hwcBoardInfo
      *bInfo;
    hwcInfo
      *hInfo;                   /* Info about all the relavent boards */
    int tmu;
    //char* envChipNum;    /* unused */
    //FxU32 chipCount = 1; /* unused */

    if ((hInfo = hwcInit(0x121a, 0x9)) == NULL)
    if ((hInfo = hwcInit(0x121a, 0x5)) == NULL) /* Voodoo3 */
    if ((hInfo = hwcInit(0x121a, 0x3)) == NULL)
      goto __errExit; 

    /* Iterate through boards found */
    for (ctx = 0; ctx < hInfo->nBoards; ctx++) {
      
      bInfo = &hInfo->boardInfo[ctx];

      GC.bInfo = bInfo;

      /*
      ** hack alert!!
      ** the chipCount should come from the minivdd
      ** KoolSmoky - chipCount should be after hwcInit(0x....) because
      ** it calls GETENV.
      */
      /*envChipNum = GETENV("FX_GLIDE_NUM_CHIPS");
      
      if (envChipNum)
        chipCount = atoi(envChipNum);*/ /* unused */

      if (bInfo->pciInfo.deviceID == SST_DEVICE_ID_H3)
        SST.type = GR_SSTTYPE_Banshee;
#ifdef FX_GLIDE_NAPALM
      else if (IS_NAPALM(bInfo->pciInfo.deviceID))
        SST.type = GR_SSTTYPE_Voodoo4;
#endif
      else 
        SST.type = GR_SSTTYPE_Voodoo3;

      if (!hwcMapBoard(bInfo, HWC_BASE_ADDR_MASK)) {
        GrErrorCallback(hwcGetErrorString(), FXTRUE);
      }

      if (!hwcInitRegisters(bInfo)) {
        GrErrorCallback(hwcGetErrorString(), FXTRUE);
      }

      /* NB: We cannot fail to map this board after this point */
      //GC.hwInitP = FXTRUE;

      _GlideRoot.hwConfig.num_sst++;

      GC.base_ptr = (FxU32 *) bInfo->regInfo.sstBase;
      GC.sstRegs = (SstRegs *) bInfo->regInfo.sstBase;
      GC.ioRegs = (SstIORegs *) bInfo->regInfo.ioMemBase;
      GC.cRegs = (SstCRegs *) bInfo->regInfo.cmdAGPBase;
      GC.lfb_ptr = (FxU32 *) bInfo->regInfo.lfbBase;
      GC.rawLfb = (FxU32 *) bInfo->regInfo.rawLfbBase;

      GC.chipCount  = bInfo->pciInfo.numChips;
      /* KCD: This can't be done here, since the environment variables
              may not have been set up yet! */
      /* GC.sliBandHeight = _GlideRoot.environment.sliBandHeight; */

#ifdef FX_GLIDE_NAPALM
      /* AJB- Point at slave chip regs
      */ 
      {
        FxU32 chip ;

        if (GC.chipCount)
          for (chip = 0 ;
               chip < GC.chipCount - 1 ;
               chip++)
          {
            GC.slaveSstRegs[chip] = (SstRegs  *)bInfo->regInfo.slaveSstBase[chip] ;
            GC.slaveCRegs[chip] =   (SstCRegs *)bInfo->regInfo.slaveCmdBase[chip] ;
          }
      }
#endif

      /* Video Parameters */
      GC.grSstRez = GR_RESOLUTION_NONE;
      GC.grSstRefresh = 0L;

      /* 
      ** Hack. Setup up triangle and vertex array routine
      */
      if (_GlideRoot.deviceArchProcs.curTriProcs) {
        GC.archDispatchProcs.coorModeTriVector = (*_GlideRoot.deviceArchProcs.curTriProcs) + 0;
        GC.archDispatchProcs.drawVertexList    = _GlideRoot.deviceArchProcs.curVertexListProcs[0];
      }

      /* This is a uma device */
      switch (hInfo->boardInfo[ctx].pciInfo.deviceID) {
      case SST_DEVICE_ID_H3: /* Banshee */
        GC.num_tmu = 1;
        GC.fbuf_size = (bInfo->h3Mem - 2);
        break;
      case SST_DEVICE_ID_H4: /* Avenger low speed */
      case SST_DEVICE_ID_H4_OEM: /* Avenger high speed */
        /*
        ** For 8M board, we may only use one tmu for higher resolution.
        ** Need to re-visit the issue. 11/5/98
        */ 
        GC.num_tmu = 2;
        GC.fbuf_size = (bInfo->h3Mem - 4);
        break;
      default:
        /* KoolSmoky - This is NAPALM.
        */
        if ((hInfo->boardInfo[ctx].pciInfo.deviceID >= SST_DEVICE_ID_L_AP) &&
            (hInfo->boardInfo[ctx].pciInfo.deviceID <= SST_DEVICE_ID_H_AP)) {
          GC.num_tmu = 2;
          GC.fbuf_size = (bInfo->h3Mem - 4);
        } else {
          GC.num_tmu = 1;
          GC.fbuf_size = (bInfo->h3Mem - 2);
        }
        break;
      }
      
      if (bInfo->h3Mem == 4) {
        GC.num_tmu = 1;
        GC.fbuf_size = (bInfo->h3Mem - 2);
      }
      
      if (GETENV("FX_GLIDE_NUM_TMU")) {
        int num_tmu = atoi(GETENV("FX_GLIDE_NUM_TMU"));
        /* XXX: what happens if someone requests invalid number of TMUs? */
        if(num_tmu > 1) {
          GC.num_tmu = 2;
          GC.fbuf_size = (bInfo->h3Mem - 4);
        } else {
          GC.num_tmu = 1;
          GC.fbuf_size = (bInfo->h3Mem - 2);
        }
      }
      
      GC.state.grEnableArgs.combine_ext_mode = GR_MODE_DISABLE;

      /* KoolSmoky - UMA for the TMUs */
      GC.state.grEnableArgs.texture_uma_mode = GR_MODE_DISABLE;
      if( GETENV("FX_GLIDE_TEXTURE_UMA") )
        if( atoi(GETENV("FX_GLIDE_TEXTURE_UMA")) == 1 )
          GC.state.grEnableArgs.texture_uma_mode = GR_MODE_ENABLE;

      SST.sstBoard.SST96Config.fbRam    = GC.fbuf_size;
      SST.sstBoard.SST96Config.nTexelfx = GC.num_tmu;
      
      SST.sstBoard.SST96Config.tmuConfig.tmuRev = (2 + kRevisionOffset);              
      SST.sstBoard.SST96Config.tmuConfig.tmuRam = 0x0;

      /* Clear the tmu state */
      for (tmu = 0; tmu < GC.num_tmu; tmu++) {
        memset(&GC.tmu_state[tmu], 0, sizeof(GC.tmu_state[0]));       
        GC.tmu_state[tmu].total_mem = (0x2 << 20);
        GC.tmu_state[tmu].ncc_mmids[0] = GC.tmu_state[tmu].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;
      }

    } /* iterate through boards found */
#endif
  } 

  /* Did we previously find boards? */
  rv = (_GlideRoot.hwConfig.num_sst != 0);
  calledP = FXTRUE;

  goto __errExit; /* Keep warnings happy */
 __errExit:
  return rv;
} /* _grSstDetectResources */


static void
displayBoardInfo(int i, GrHwConfiguration *hwc)
{
  if ((hwc->SSTs[i].type == GR_SSTTYPE_VOODOO) ||
      (hwc->SSTs[i].type == GR_SSTTYPE_Voodoo2)) {
    int tmuNum;

    GDBG_INFO(80,"SST board %d: 3Dfx Voodoo%s\n", 
              i, ((hwc->SSTs[i].type == GR_SSTTYPE_VOODOO) ? " Graphics" : "^2"));
    if (hwc->SSTs[i].sstBoard.VoodooConfig.sliDetect) {
      GDBG_INFO(80,"\tScanline Interleaved\n");
    }

    GDBG_INFO(80,"\tPixelfx rev 0x%lX with %d MB Frame Buffer\n",
              hwc->SSTs[i].sstBoard.VoodooConfig.fbiRev,
              hwc->SSTs[i].sstBoard.VoodooConfig.fbRam);
    GDBG_INFO(80,"\t%d Texelfx chips:\n",
              hwc->SSTs[i].sstBoard.VoodooConfig.nTexelfx);
    for (tmuNum = 0;
         tmuNum < hwc->SSTs[i].sstBoard.VoodooConfig.nTexelfx;
         tmuNum++) {
      GDBG_INFO(80,"\t\tTexelfx %d: Rev 0x%lX, %d MB Texture\n", tmuNum,
                hwc->SSTs[i].sstBoard.VoodooConfig.tmuConfig[tmuNum].tmuRev,
                hwc->SSTs[i].sstBoard.VoodooConfig.tmuConfig[tmuNum].tmuRam);
    }
  } else if (hwc->SSTs[i].type == GR_SSTTYPE_SST96) {
    GDBG_INFO(80,"SST board %d: 3Dfx Voodoo Rush\n", i);
    GDBG_INFO(80,"\tFBI Jr. with %d MB Frame Buffer\n",
              hwc->SSTs[i].sstBoard.SST96Config.fbRam);
    GDBG_INFO(80,"\tTexelfx chips:  1\n");
  } else if (hwc->SSTs[i].type == GR_SSTTYPE_Voodoo2) {
    GDBG_INFO(80,"SST board %d: 3Dfx Voodoo2\n", i);
  } else if (hwc->SSTs[i].type == GR_SSTTYPE_Banshee) {
    GDBG_INFO(80,"SST board %d: 3Dfx Banshee\n", i);
  } else if (hwc->SSTs[i].type == GR_SSTTYPE_Voodoo3) {
    GDBG_INFO(80,"SST board %d: 3Dfx Voodoo3\n", i);
  } else if (hwc->SSTs[i].type == GR_SSTTYPE_Voodoo4) {
    GDBG_INFO(80,"SST board %d: 3dfx Voodoo4/5\n", i);
  } else {
    GDBG_INFO(80,"error: SSTs %d: unknown type\n",i);
  }
} /* displayBoardInfo */

void
_GlideInitEnvironment(void)
{
#define FN_NAME "_GlideInitEnvironment"
  int i;
  const char* envStr;

  if (_GlideRoot.initialized) /* ONLY EXECUTE ONCE */
    return;

  GDBG_INFO(80,"%s()\n", FN_NAME);

#if defined(FX_DLL_ENABLE) && (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  { /* GMT: display the DLL pathname for sanity checking */
    char buf[132] = "failed";
    GetModuleFileName(GetModuleHandle("glide3x.dll"), buf, sizeof(buf));
    GDBG_INFO(0,"DLL path: %s\n",buf);
  }
#endif

  /* dBorca - play safe */
  grErrorSetCallback(_grErrorDefaultCallback);

#if GL_X86
  /* Get CPU Info before we detect glide devices */
  _cpuid (&_GlideRoot.CPUType);

  /* Check for vendor specific optimization cases */
  GDBG_INFO( 0,"   CPU Vendor: %s\n", _GlideRoot.CPUType.v_name);
  GDBG_INFO(80,"   MMX Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_MMX ? 'Y' : 'N');
  GDBG_INFO(80,"   SSE Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_SSE ? 'Y' : 'N');
  GDBG_INFO(80,"  SSE2 Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_SSE2 ? 'Y' : 'N');
  GDBG_INFO(80," 3DNow Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_3DNOW ? 'Y' : 'N');
  GDBG_INFO(80,"  MMX+ Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_MMXPLUS ? 'Y' : 'N');
  GDBG_INFO(80,"3DNow+ Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_3DNOWPLUS ? 'Y' : 'N');
#endif

  /* Check for user environment tweaks */
#define GLIDE_GETENV(__envVar, __defVal) \
  (((envStr = GETENV(__envVar)) == NULL) ? (__defVal) : atol(envStr))
#define GLIDE_FGETENV(__envVar, __defVal) \
  (((envStr = GETENV(__envVar)) == NULL) ? (__defVal) : (float) atof(envStr))
#define GLIDE_34GETENV(__envVar, __defVal) \
  (((signed char)(atof(((envStr = GETENV(__envVar)) == NULL) ? (__defVal) : (envStr))*16.0f)+8)&0x7f)

#if GL_X86
  /* No CPU Extensions Allowed */
  if (GLIDE_GETENV("FX_GLIDE_NO_CPU_EXTENSIONS", 0L))
  {
    _GlideRoot.CPUType.feature = _GlideRoot.CPUType.os_support = 0;
    GDBG_INFO(0,"CPU Extensions disabled\n");
  }

#if !DRI_BUILD
  /* Pass retrieved CPU Info into minihwc */
  hwcSetCPUInfo(&_GlideRoot.CPUType);
#endif
#endif
  
  /* KoolSmoky - detect glide devices before we check for user 
  ** environment tweaks.
  */
  if ( !_grSstDetectResources() ) {
  /* Hack alert:
   * dBorca - Linux/DRI failed the above call, so bypass the next one
   */
#ifndef DRI_BUILD
#ifdef GLIDE_INIT_HWC
    GrErrorCallback( hwcGetErrorString(), FXTRUE );
#endif
#endif
  }

  /*
   * Setup the basic proc tables based on the cpu type.
   */

  /* Default case - rasterization routines */
  _GlideRoot.deviceArchProcs.curTriProcs        = _triSetupProcs + 0;
  _GlideRoot.deviceArchProcs.curDrawTrisProc    = _grDrawTriangles_Default;
  _GlideRoot.deviceArchProcs.curVertexListProcs = _vertexListProcs[0];
  
  /* Default case - texture download procs */
  _GlideRoot.deviceArchProcs.curTexProcs = _texDownloadProcs + 0;
  
  /* null proc case */
#define ARRAY_LAST(__array) ((sizeof(__array) / sizeof((__array)[0])) - 1)
  _GlideRoot.deviceArchProcs.nullTriProcs        = _triSetupProcs + ARRAY_LAST(_triSetupProcs);
  _GlideRoot.deviceArchProcs.nullDrawTrisProc    = _grDrawTriangles_null;
  _GlideRoot.deviceArchProcs.nullVertexListProcs = _vertexListProcs[ARRAY_LAST(_vertexListProcs)];
  _GlideRoot.deviceArchProcs.nullTexProcs        = _texDownloadProcs + ARRAY_LAST(_texDownloadProcs);
#undef ARRAY_LAST

#if GL_MMX
  if (_GlideRoot.CPUType.os_support & _CPU_FEATURE_MMX) {  /* check for MMX feature */
    GDBG_INFO(0,"Using MMX Texture Download Functions\n");
    _GlideRoot.deviceArchProcs.curTexProcs        = _texDownloadProcs + 2;
  }
#endif /* GL_MMX */
#if GL_SSE
  if (_GlideRoot.CPUType.os_support & _CPU_FEATURE_SSE) {  /* check for SSE FP feature */
    GDBG_INFO(0,"Using SSE Geometry Functions\n");
    _GlideRoot.deviceArchProcs.curTriProcs        = _triSetupProcs + 2;
    _GlideRoot.deviceArchProcs.curDrawTrisProc    = _grDrawTriangles_SSE;
    _GlideRoot.deviceArchProcs.curVertexListProcs = _vertexListProcs[2];
  }
#endif /* GL_SSE */
#if GL_AMD3D
  if (_GlideRoot.CPUType.os_support & _CPU_FEATURE_3DNOW) {  /* check for 3DNow! feature */
    GDBG_INFO(0,"Using 3DNow! Texture Download Functions\n");
    _GlideRoot.deviceArchProcs.curTexProcs        = _texDownloadProcs + 1;
    
    GDBG_INFO(0,"Using 3DNow! Geometry Functions\n");
    _GlideRoot.deviceArchProcs.curTriProcs        = _triSetupProcs + 1;
    _GlideRoot.deviceArchProcs.curDrawTrisProc    = _grDrawTriangles_3DNow;
    _GlideRoot.deviceArchProcs.curVertexListProcs = _vertexListProcs[1];
  }
#endif /* GL_AMD3D */
#if GL_SSE2
  if (_GlideRoot.CPUType.os_support & _CPU_FEATURE_SSE2) {  /* check for SSE2 feature */
    GDBG_INFO(0,"Using SSE2 Texture Download Functions\n");
    _GlideRoot.deviceArchProcs.curTexProcs        = _texDownloadProcs + 3;
  }
#endif /* GL_SSE2*/

  /* constant pool */
  _GlideRoot.pool.f0   =   0.0F;
  _GlideRoot.pool.fHalf=   0.5F;
  _GlideRoot.pool.f1   =   1.0F;
  _GlideRoot.pool.f255 = 255.0F;
  
#if GLIDE_PACKED_RGB
  _GlideRoot.pool.fBiasHi = (float)(0x01 << 15);
  _GlideRoot.pool.fBiasLo = (float)(0x01 << 23);
#endif /* GLIDE_PACKED_RGB */

  /*
   * check for user environment tweaks.
   */
#ifdef GLIDE_TEST_TEXTURE_ALIGNMENT
  SST_TEXTURE_ALIGN = GLIDE_GETENV("FX_GLIDE_TEX_ALIGN", 0x10UL);
#endif
  _GlideRoot.environment.triBoundsCheck    = GETENV("FX_GLIDE_BOUNDS_CHECK") != NULL;
  GDBG_INFO(80,"    triBoundsCheck: %d\n",_GlideRoot.environment.triBoundsCheck);
#ifdef GLIDE_SPLASH
  _GlideRoot.environment.noSplash          = GETENV("FX_GLIDE_NO_SPLASH") != NULL;
#else
  _GlideRoot.environment.noSplash          = 1;
#endif
  
  GDBG_INFO(80,"          noSplash: %d\n",_GlideRoot.environment.noSplash);
#ifdef GLIDE_PLUG
  _GlideRoot.environment.shamelessPlug     = GETENV("FX_GLIDE_SHAMELESS_PLUG") != NULL;
#else
  _GlideRoot.environment.shamelessPlug     = 0;
#endif
  GDBG_INFO(80,"     shamelessPlug: %d\n",_GlideRoot.environment.shamelessPlug);
  _GlideRoot.environment.ignoreReopen      = GETENV("FX_GLIDE_IGNORE_REOPEN") != NULL;
  GDBG_INFO(80,"      ignoreReopen: %d\n",_GlideRoot.environment.ignoreReopen);
  /*_GlideRoot.environment.disableDitherSub  = GETENV("FX_GLIDE_NO_DITHER_SUB") != NULL; 
  GDBG_INFO(80,"  disableDitherSub: %d\n",_GlideRoot.environment.disableDitherSub);*/
  _GlideRoot.environment.fifoSize          = GETENV("FX_GLIDE_FIFO_SIZE") != NULL;
  GDBG_INFO(80,"          fifoSize: %d\n",_GlideRoot.environment.fifoSize);
  _GlideRoot.environment.noHW              = GETENV("FX_GLIDE_NO_HW") != NULL;
  GDBG_INFO(80,"              noHW: %d\n",_GlideRoot.environment.noHW);
  
  /* set default glide state to not openGL app. _GlideInitEnvironment() must not be called multiple times. */
  _GlideRoot.environment.is_opengl = FXFALSE;
  
  /* note - glide now uses a string representation for the AA jitter values */
  /* This is the "old" way of doing two-sample AA, where each chip does two samples. */  
  _GlideRoot.environment.aaXOffset[1][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_DEF);
  _GlideRoot.environment.aaXOffset[1][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_DEF);
  _GlideRoot.environment.aaXOffset[1][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_DEF);
  _GlideRoot.environment.aaXOffset[1][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_DEF);
  
  _GlideRoot.environment.aaYOffset[1][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_DEF);
  _GlideRoot.environment.aaYOffset[1][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_DEF);
  _GlideRoot.environment.aaYOffset[1][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_DEF);
  _GlideRoot.environment.aaYOffset[1][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_DEF);
  
  /* This is a nasty evil hack!  This rearranges the sample offsets to deal with the new way of doing two sample AA */
  _GlideRoot.environment.aaXOffset[2][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_DEF);
  _GlideRoot.environment.aaXOffset[2][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_DEF);
  _GlideRoot.environment.aaXOffset[2][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_DEF);
  _GlideRoot.environment.aaXOffset[2][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_DEF);
  
  _GlideRoot.environment.aaYOffset[2][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_DEF);
  _GlideRoot.environment.aaYOffset[2][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_DEF);
  _GlideRoot.environment.aaYOffset[2][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_DEF);
  _GlideRoot.environment.aaYOffset[2][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_DEF);
  
  /* This is the "normal" layout for 4-sample AA */
  
  _GlideRoot.environment.aaXOffset[3][0]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X0", PRIBUFVTXOFFX_4SMPL_CHP0_DEF);
  _GlideRoot.environment.aaXOffset[3][1]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X1", SECBUFVTXOFFX_4SMPL_CHP0_DEF);
  _GlideRoot.environment.aaXOffset[3][2]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X2", PRIBUFVTXOFFX_4SMPL_CHP1_DEF);
  _GlideRoot.environment.aaXOffset[3][3]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X3", SECBUFVTXOFFX_4SMPL_CHP1_DEF);
  
  _GlideRoot.environment.aaYOffset[3][0]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y0", PRIBUFVTXOFFY_4SMPL_CHP0_DEF);
  _GlideRoot.environment.aaYOffset[3][1]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y1", SECBUFVTXOFFY_4SMPL_CHP0_DEF);
  _GlideRoot.environment.aaYOffset[3][2]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y2", PRIBUFVTXOFFY_4SMPL_CHP1_DEF);
  _GlideRoot.environment.aaYOffset[3][3]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y3", SECBUFVTXOFFY_4SMPL_CHP1_DEF);
  
  /* these are the correct jitter vaules */
  /* 2xaa */
  _GlideRoot.environment.aaXOffset[4][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[4][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[4][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[4][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[4][4]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[4][5]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[4][6]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[4][7]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);
  
  _GlideRoot.environment.aaYOffset[4][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[4][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[4][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[4][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[4][4]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[4][5]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[4][6]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[4][7]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);
  
  _GlideRoot.environment.aaXOffset[5][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[5][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[5][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[5][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[5][4]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[5][5]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[5][6]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[5][7]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);
  
  _GlideRoot.environment.aaYOffset[5][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[5][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[5][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[5][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[5][4]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[5][5]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[5][6]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[5][7]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);
  
  /* 4xaa */
  _GlideRoot.environment.aaXOffset[6][0]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X0", PRIBUFVTXOFFX_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[6][1]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X1", PRIBUFVTXOFFX_4SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[6][2]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X2", SECBUFVTXOFFX_4SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[6][3]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X3", SECBUFVTXOFFX_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[6][4]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X0", PRIBUFVTXOFFX_4SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[6][5]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X1", PRIBUFVTXOFFX_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[6][6]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X2", SECBUFVTXOFFX_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[6][7]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X3", PRIBUFVTXOFFX_4SMPL_CHP1_CORRECT_DEF);
  
  _GlideRoot.environment.aaYOffset[6][0]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y0", PRIBUFVTXOFFY_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[6][1]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y1", PRIBUFVTXOFFY_4SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[6][2]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y2", SECBUFVTXOFFY_4SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[6][3]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y3", SECBUFVTXOFFY_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[6][4]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y0", PRIBUFVTXOFFY_4SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[6][5]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y1", PRIBUFVTXOFFY_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[6][6]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y2", SECBUFVTXOFFY_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[6][7]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y3", SECBUFVTXOFFY_4SMPL_CHP1_CORRECT_DEF);
  
  /* jcochrane 4 chip offsets
   *
   * About: The strange ordering would allow FSAA to still
   * work regardless of the SLI/Samples per chip configuration */
  
  /* 4chip 2xaa */
  _GlideRoot.environment.aaXOffset[7][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_DEF);	// 25
  _GlideRoot.environment.aaXOffset[7][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_DEF);	// 75
  _GlideRoot.environment.aaXOffset[7][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_DEF);	// 75
  _GlideRoot.environment.aaXOffset[7][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_DEF);	// 25
  _GlideRoot.environment.aaXOffset[7][4]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_DEF);	// 25
  _GlideRoot.environment.aaXOffset[7][5]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_DEF);	// 75
  _GlideRoot.environment.aaXOffset[7][6]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_DEF);	// 75
  _GlideRoot.environment.aaXOffset[7][7]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_DEF);	// 25
  
  _GlideRoot.environment.aaYOffset[7][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_DEF);	// 25
  _GlideRoot.environment.aaYOffset[7][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_DEF);	// 75
  _GlideRoot.environment.aaYOffset[7][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_DEF);	// 75
  _GlideRoot.environment.aaYOffset[7][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_DEF);	// 25
  _GlideRoot.environment.aaYOffset[7][4]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_DEF);	// 25
  _GlideRoot.environment.aaYOffset[7][5]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_DEF);	// 75
  _GlideRoot.environment.aaYOffset[7][6]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_DEF);	// 75
  _GlideRoot.environment.aaYOffset[7][7]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_DEF);	// 25
  
  /* 4chip 4xaa */
  _GlideRoot.environment.aaXOffset[8][0]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X0", PRIBUFVTXOFFX_4SMPL_CHP0_DEF);	// 375
  _GlideRoot.environment.aaXOffset[8][1]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X2", PRIBUFVTXOFFX_4SMPL_CHP1_DEF);
  _GlideRoot.environment.aaXOffset[8][2]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X1", SECBUFVTXOFFX_4SMPL_CHP0_DEF);	// 875
  _GlideRoot.environment.aaXOffset[8][3]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X3", SECBUFVTXOFFX_4SMPL_CHP1_DEF);
  _GlideRoot.environment.aaXOffset[8][4]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X2", PRIBUFVTXOFFX_4SMPL_CHP1_DEF);	// 125
  _GlideRoot.environment.aaXOffset[8][5]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X0", PRIBUFVTXOFFX_4SMPL_CHP0_DEF);
  _GlideRoot.environment.aaXOffset[8][6]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X3", SECBUFVTXOFFX_4SMPL_CHP1_DEF);	// 625
  _GlideRoot.environment.aaXOffset[8][7]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X1", SECBUFVTXOFFX_4SMPL_CHP0_DEF);
  
  _GlideRoot.environment.aaYOffset[8][0]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y0", PRIBUFVTXOFFY_4SMPL_CHP0_DEF);	// 125
  _GlideRoot.environment.aaYOffset[8][1]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y2", PRIBUFVTXOFFY_4SMPL_CHP1_DEF);
  _GlideRoot.environment.aaYOffset[8][2]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y1", SECBUFVTXOFFY_4SMPL_CHP0_DEF);	//
  _GlideRoot.environment.aaYOffset[8][3]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y3", SECBUFVTXOFFY_4SMPL_CHP1_DEF);
  _GlideRoot.environment.aaYOffset[8][4]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y2", PRIBUFVTXOFFY_4SMPL_CHP1_DEF);
  _GlideRoot.environment.aaYOffset[8][5]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y0", PRIBUFVTXOFFY_4SMPL_CHP0_DEF);
  _GlideRoot.environment.aaYOffset[8][6]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y3", SECBUFVTXOFFY_4SMPL_CHP1_DEF);
  _GlideRoot.environment.aaYOffset[8][7]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y1", SECBUFVTXOFFY_4SMPL_CHP0_DEF);
  
  /* these are the old offsets */
  /* 4chip 2xaa
  _GlideRoot.environment.aaXOffset[7][0]   = 0x04;
  _GlideRoot.environment.aaXOffset[7][1]   = 0x00;
  _GlideRoot.environment.aaXOffset[7][2]   = 0x0c;  
  _GlideRoot.environment.aaXOffset[7][3]   = 0x00;
  _GlideRoot.environment.aaXOffset[7][4]   = 0x04;
  _GlideRoot.environment.aaXOffset[7][5]   = 0x00;
  _GlideRoot.environment.aaXOffset[7][6]   = 0x0c;
  _GlideRoot.environment.aaXOffset[7][7]   = 0x00;
  
  _GlideRoot.environment.aaYOffset[7][0]   = 0x04;
  _GlideRoot.environment.aaYOffset[7][1]   = 0x00;
  _GlideRoot.environment.aaYOffset[7][2]   = 0x0c;  
  _GlideRoot.environment.aaYOffset[7][3]   = 0x00;
  _GlideRoot.environment.aaYOffset[7][4]   = 0x04;
  _GlideRoot.environment.aaYOffset[7][5]   = 0x00;
  _GlideRoot.environment.aaYOffset[7][6]   = 0x0c;  
  _GlideRoot.environment.aaYOffset[7][7]   = 0x00;
  */
 /* 4chip 4xaa 
  _GlideRoot.environment.aaXOffset[8][0]   = 0x06; 
  _GlideRoot.environment.aaXOffset[8][1]   = 0x00; 
  _GlideRoot.environment.aaXOffset[8][2]   = 0x0e; 
  _GlideRoot.environment.aaXOffset[8][3]   = 0x00; 
  _GlideRoot.environment.aaXOffset[8][4]   = 0x02; 
  _GlideRoot.environment.aaXOffset[8][5]   = 0x00; 
  _GlideRoot.environment.aaXOffset[8][6]   = 0x0a; 
  _GlideRoot.environment.aaXOffset[8][7]   = 0x00; 
 
  _GlideRoot.environment.aaYOffset[8][0]   = 0x02; 
  _GlideRoot.environment.aaYOffset[8][1]   = 0x00;    
  _GlideRoot.environment.aaYOffset[8][2]   = 0x06; 
  _GlideRoot.environment.aaYOffset[8][3]   = 0x00;    
  _GlideRoot.environment.aaYOffset[8][4]   = 0x0a; 
  _GlideRoot.environment.aaYOffset[8][5]   = 0x00;    
  _GlideRoot.environment.aaYOffset[8][6]   = 0x0e; 
  _GlideRoot.environment.aaYOffset[8][7]   = 0x00;    
  */
  /* 4chip 8xaa */
#if 0
  _GlideRoot.environment.aaXOffset[9][0]   = 0x06; 
  _GlideRoot.environment.aaXOffset[9][1]   = 0x04; 
  _GlideRoot.environment.aaXOffset[9][2]   = 0x0e; 
  _GlideRoot.environment.aaXOffset[9][3]   = 0x0a; 
  _GlideRoot.environment.aaXOffset[9][4]   = 0x02; 
  _GlideRoot.environment.aaXOffset[9][5]   = 0x0c; 
  _GlideRoot.environment.aaXOffset[9][6]   = 0x0a; 
  _GlideRoot.environment.aaXOffset[9][7]   = 0x06; 
  
  _GlideRoot.environment.aaYOffset[9][0]   = 0x02; 
  _GlideRoot.environment.aaYOffset[9][1]   = 0x06; 
  _GlideRoot.environment.aaYOffset[9][2]   = 0x06; 
  _GlideRoot.environment.aaYOffset[9][3]   = 0x04; 
  _GlideRoot.environment.aaYOffset[9][4]   = 0x0a; 
  _GlideRoot.environment.aaYOffset[9][5]   = 0x0a; 
  _GlideRoot.environment.aaYOffset[9][6]   = 0x0e; 
  _GlideRoot.environment.aaYOffset[9][7]   = 0x0c;
#else
  _GlideRoot.environment.aaXOffset[9][0]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X0", PRIBUFVTXOFFX_8SMPL_CHP0_DEF);
  _GlideRoot.environment.aaXOffset[9][1]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X1", SECBUFVTXOFFX_8SMPL_CHP0_DEF);
  _GlideRoot.environment.aaXOffset[9][2]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X2", PRIBUFVTXOFFX_8SMPL_CHP1_DEF);
  _GlideRoot.environment.aaXOffset[9][3]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X3", SECBUFVTXOFFX_8SMPL_CHP1_DEF);
  _GlideRoot.environment.aaXOffset[9][4]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X4", PRIBUFVTXOFFX_8SMPL_CHP2_DEF);
  _GlideRoot.environment.aaXOffset[9][5]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X5", SECBUFVTXOFFX_8SMPL_CHP2_DEF);
  _GlideRoot.environment.aaXOffset[9][6]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X6", PRIBUFVTXOFFX_8SMPL_CHP3_DEF);
  _GlideRoot.environment.aaXOffset[9][7]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X7", SECBUFVTXOFFX_8SMPL_CHP3_DEF);
  
  _GlideRoot.environment.aaYOffset[9][0]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y0", PRIBUFVTXOFFY_8SMPL_CHP0_DEF);
  _GlideRoot.environment.aaYOffset[9][1]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y1", SECBUFVTXOFFY_8SMPL_CHP0_DEF);
  _GlideRoot.environment.aaYOffset[9][2]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y2", PRIBUFVTXOFFY_8SMPL_CHP1_DEF);
  _GlideRoot.environment.aaYOffset[9][3]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y3", SECBUFVTXOFFY_8SMPL_CHP1_DEF);
  _GlideRoot.environment.aaYOffset[9][4]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y4", PRIBUFVTXOFFY_8SMPL_CHP2_DEF);
  _GlideRoot.environment.aaYOffset[9][5]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y5", SECBUFVTXOFFY_8SMPL_CHP2_DEF);
  _GlideRoot.environment.aaYOffset[9][6]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y6", PRIBUFVTXOFFY_8SMPL_CHP3_DEF);
  _GlideRoot.environment.aaYOffset[9][7]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y7", SECBUFVTXOFFY_8SMPL_CHP3_DEF);
#endif
  
  /* jcochrane 4 chip offsets
   *
   * About: The strange ordering would allow FSAA to still
   * work regardless of the SLI/Samples per chip configuration */
  
  /* these are the correct jitter vaules */
  /* 4chip 2xaa */
  _GlideRoot.environment.aaXOffset[10][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);	// 25
  _GlideRoot.environment.aaXOffset[10][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);	// 75
  _GlideRoot.environment.aaXOffset[10][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);	// 75
  _GlideRoot.environment.aaXOffset[10][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);	// 25
  _GlideRoot.environment.aaXOffset[10][4]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);	// 25
  _GlideRoot.environment.aaXOffset[10][5]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);	// 75
  _GlideRoot.environment.aaXOffset[10][6]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X1", SECBUFVTXOFFX_2SMPL_CORRECT_DEF);	// 75
  _GlideRoot.environment.aaXOffset[10][7]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_X0", PRIBUFVTXOFFX_2SMPL_CORRECT_DEF);	// 25
  
  _GlideRoot.environment.aaYOffset[10][0]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);	// 25
  _GlideRoot.environment.aaYOffset[10][1]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);	// 75
  _GlideRoot.environment.aaYOffset[10][2]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);	// 75
  _GlideRoot.environment.aaYOffset[10][3]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);	// 25
  _GlideRoot.environment.aaYOffset[10][4]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);	// 25
  _GlideRoot.environment.aaYOffset[10][5]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);	// 75
  _GlideRoot.environment.aaYOffset[10][6]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y1", SECBUFVTXOFFY_2SMPL_CORRECT_DEF);	// 75
  _GlideRoot.environment.aaYOffset[10][7]   = GLIDE_34GETENV("FX_GLIDE_AA2_OFFSET_Y0", PRIBUFVTXOFFY_2SMPL_CORRECT_DEF);	// 25
  
  /* 4chip 4xaa */
  _GlideRoot.environment.aaXOffset[11][0]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X0", PRIBUFVTXOFFX_4SMPL_CHP0_CORRECT_DEF);	// 375
  _GlideRoot.environment.aaXOffset[11][1]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X2", PRIBUFVTXOFFX_4SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[11][2]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X1", SECBUFVTXOFFX_4SMPL_CHP1_CORRECT_DEF);	// 875
  _GlideRoot.environment.aaXOffset[11][3]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X3", SECBUFVTXOFFX_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[11][4]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X2", PRIBUFVTXOFFX_4SMPL_CHP1_CORRECT_DEF);	// 125
  _GlideRoot.environment.aaXOffset[11][5]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X0", PRIBUFVTXOFFX_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[11][6]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X3", SECBUFVTXOFFX_4SMPL_CHP0_CORRECT_DEF);	// 625
  _GlideRoot.environment.aaXOffset[11][7]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_X1", SECBUFVTXOFFX_4SMPL_CHP1_CORRECT_DEF);
  
  _GlideRoot.environment.aaYOffset[11][0]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y0", PRIBUFVTXOFFY_4SMPL_CHP0_CORRECT_DEF);	// 125
  _GlideRoot.environment.aaYOffset[11][1]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y2", PRIBUFVTXOFFY_4SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[11][2]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y1", SECBUFVTXOFFY_4SMPL_CHP1_CORRECT_DEF);	//
  _GlideRoot.environment.aaYOffset[11][3]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y3", SECBUFVTXOFFY_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[11][4]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y2", PRIBUFVTXOFFY_4SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[11][5]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y0", PRIBUFVTXOFFY_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[11][6]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y3", SECBUFVTXOFFY_4SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[11][7]   = GLIDE_34GETENV("FX_GLIDE_AA4_OFFSET_Y1", SECBUFVTXOFFY_4SMPL_CHP1_CORRECT_DEF);
  
  /* 4chip 8xaa */
  _GlideRoot.environment.aaXOffset[12][0]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X0", PRIBUFVTXOFFX_8SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[12][1]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X1", SECBUFVTXOFFX_8SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[12][2]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X2", PRIBUFVTXOFFX_8SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[12][3]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X3", SECBUFVTXOFFX_8SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[12][4]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X4", PRIBUFVTXOFFX_8SMPL_CHP2_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[12][5]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X5", SECBUFVTXOFFX_8SMPL_CHP2_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[12][6]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X6", PRIBUFVTXOFFX_8SMPL_CHP3_CORRECT_DEF);
  _GlideRoot.environment.aaXOffset[12][7]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_X7", SECBUFVTXOFFX_8SMPL_CHP3_CORRECT_DEF);
  
  _GlideRoot.environment.aaYOffset[12][0]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y0", PRIBUFVTXOFFY_8SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[12][1]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y1", SECBUFVTXOFFY_8SMPL_CHP0_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[12][2]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y2", PRIBUFVTXOFFY_8SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[12][3]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y3", SECBUFVTXOFFY_8SMPL_CHP1_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[12][4]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y4", PRIBUFVTXOFFY_8SMPL_CHP2_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[12][5]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y5", SECBUFVTXOFFY_8SMPL_CHP2_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[12][6]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y6", PRIBUFVTXOFFY_8SMPL_CHP3_CORRECT_DEF);
  _GlideRoot.environment.aaYOffset[12][7]   = GLIDE_34GETENV("FX_GLIDE_AA8_OFFSET_Y7", SECBUFVTXOFFY_8SMPL_CHP3_CORRECT_DEF);
  
  /*
   * AJB-  Support the slightly silly way the DirectX gang controls
   *       SLI & AA from the 3dfx tools control panel, just to make
   *       life a little easier for Reid.
   *
   *       Here is a breakdown of the bizarre table of magic numbers:
   *
   *       0 - SLI & AA disable
   *       1 - SLI disabled, 2 sample AA enabled
   *       2 - 2-way SLI enabled, AA disabled
   *       3 - 2-way SLI enabled, 2 sample AA enabled
   *       4 - SLI disabled, 4 sample AA enabled
   *       5 - 4-way SLI enabled, AA disabled
   *       6 - 4-way SLI enabled, 2 sample AA enabled
   *       7 - 2-way SLI enabled, 4 sample AA enabled
   *	   8 - SLI disabled, 8 sample AA enabled
   *
   *       to add to the silliness:
   *
   *       settings 0 & 1 are valid for all configurations
   *       settings 2, 3 & 4 are valid for 2 chip boards only
   *       settings 5, 6 & 7 are valid for 4 chip boards only
   *
   *       to make life easier on everyone, we won't enforce the board
   *       type restriction and we will default to use whatever SLI
   *       is available with no AA if the variable cannot be found.
   */
  _GlideRoot.environment.forceSingleChip = 0 ;
  _GlideRoot.environment.aaSample = 0 ;
  
  switch(GLIDE_GETENV("SSTH3_SLI_AA_CONFIGURATION", 2L))
    { 
    case 1:
      _GlideRoot.environment.aaSample = 2 ;
    case 0:
      _GlideRoot.environment.forceSingleChip = 1 ;
      break ;
    case 3:
      case 6:
      _GlideRoot.environment.aaSample = 2 ;
      break ;
    case 4:
    case 7:
      _GlideRoot.environment.aaSample = 4 ;
      break;
      //8xaa
    case 8:
      _GlideRoot.environment.aaSample = 8 ;
        break ;
      
    default:
      break ;
    }
  
  /*
   ** AJB-  This lets Joe bag-o-donuts force 32bpp & AA rendering
   **       for apps that call grSstWinOpen.
   */
  _GlideRoot.environment.outputBpp = GLIDE_GETENV("FX_GLIDE_BPP", 0L);
  /* check for a valid value */
  if(_GlideRoot.environment.outputBpp != 32 ||
     _GlideRoot.environment.outputBpp != 15) {
    _GlideRoot.environment.outputBpp = 0;
  }
  
  /* Note- If the old school Glide env. vars for AA sample & Num chips
   * are active, they should ALWAYS override the control panel variable
   */
  if (GETENV("FX_GLIDE_AA_SAMPLE"))
    _GlideRoot.environment.aaSample = atol(GETENV("FX_GLIDE_AA_SAMPLE")) ;
  
  if (GLIDE_GETENV("FX_GLIDE_NUM_CHIPS", 0L) > 1)
    _GlideRoot.environment.forceSingleChip = 0 ;
  
  /*
    **  CHD - This let's Joe bag-o-croissant force rendering-column width.
   */
  _GlideRoot.environment.columnWidth = GLIDE_GETENV("FX_GLIDE_COLUMN_WIDTH", 32L) ;
  /*
   * AJB- This lets those lucky people with non-flaky SLI slave chips
   *      enable the WAX functions that cause us unlucky folks to hang.
   */
  _GlideRoot.environment.waxon = GLIDE_GETENV("FX_GLIDE_WAX_ON", 1L) ;
  
  /*
   * KCD- Let user toggle AA on and off on the fly to impress their friends.
   */
    _GlideRoot.environment.aaToggleKey = GLIDE_GETENV("FX_GLIDE_AA_TOGGLE_KEY", 0L) ;
  
  /* tempAA toggle key */
  _GlideRoot.environment.taaToggleKey =  GLIDE_GETENV("FX_GLIDE_TAA_TOGGLE_KEY", 0L) ;
  _GlideRoot.environment.taaEnabled   =  (_GlideRoot.environment.taaToggleKey != 0);
  
  /* Save off 32-bit screenshots from four-sample 16-bit AA buffers */
  _GlideRoot.environment.aaScreenshotKey = GLIDE_GETENV("FX_GLIDE_SCREENSHOT_KEY", 0L) ;
  
  /* Which way to do 2-sample AA? */
  _GlideRoot.environment.forceOldAA = GLIDE_GETENV("FX_GLIDE_FORCE_OLD_AA", 0L);
  
  /*
   * AJB- 1= Always analog sli, 0= Glide decides, -1= always digital
   */
  _GlideRoot.environment.analogSli = GLIDE_GETENV("FX_GLIDE_ANALOG_SLI", 0L) ;
  
  _GlideRoot.environment.lodBias = GLIDE_GETENV("FX_GLIDE_LOD_BIAS", 0L) ;
  
  /****************************************************************** 
   * 5/4/99 gregk
   * Adding user support for turning on/off dither substraction
   * According to Alpha Blending Quality Tab of control panel choices   
   * Optimal/Sharper -> disable dither subtraction    
   * Smoother        -> enable  dither subtraction                     
   ******************************************************************/  
  switch(GLIDE_GETENV("SSTH3_ALPHADITHERMODE", 1L))     
    {
    default:
    case OPTIMAL: /* Or Automatic? */
      case SHARPER:
      _GlideRoot.environment.disableDitherSub = FXTRUE;
      break;
    case SMOOTHER:
      _GlideRoot.environment.disableDitherSub = FXFALSE;
      break;
    }  
  GDBG_INFO(80,"  disableDitherSub: %d\n",_GlideRoot.environment.disableDitherSub);  
  
  _GlideRoot.environment.texLodDither      = GLIDE_GETENV("FX_GLIDE_LOD_DITHER", 0L) ? SST_TLODDITHER : 0;
  GDBG_INFO(80,"      texLodDither: %d\n",_GlideRoot.environment.texLodDither);
  
  _GlideRoot.environment.texSubLodDither = GLIDE_GETENV("FX_GLIDE_LOD_SUBSAMPLE_DITHER", 1L) ;
  GDBG_INFO(80,"   texSubLodDither: %d\n",_GlideRoot.environment.texSubLodDither );
  
  /* 0 = No dithering when doing HWC AA dumps, 1 = error diffusion dithering enabled (default) */
  _GlideRoot.environment.ditherHwcAA = GLIDE_GETENV("FX_GLIDE_DITHER_HWC_AA", 1L) ;
  GDBG_INFO(80,"        ditherHwcAA: %d\n",_GlideRoot.environment.ditherHwcAA );
  
  _GlideRoot.environment.nColorBuffer      = GLIDE_GETENV("FX_GLIDE_ALLOC_COLOR", -1L);
  GDBG_INFO(80,"      nColorBuffer: %d\n",_GlideRoot.environment.nColorBuffer);
  _GlideRoot.environment.tmuMemory =       GLIDE_GETENV("FX_GLIDE_TMU_MEMSIZE", -1L);
  GDBG_INFO(80,"\ttmuMemory: %d\n",_GlideRoot.environment.tmuMemory);
  _GlideRoot.environment.nAuxBuffer        = GLIDE_GETENV("FX_GLIDE_ALLOC_AUX", -1L);    
  GDBG_INFO(80,"        nAuxBuffer: %d\n",_GlideRoot.environment.nAuxBuffer);
  _GlideRoot.environment.swFifoLWM         = GLIDE_GETENV("FX_GLIDE_LWM", -1L);
  GDBG_INFO(80,"         swFifoLWM: %d\n",_GlideRoot.environment.swFifoLWM);
  _GlideRoot.environment.swapInterval      = GLIDE_GETENV("FX_GLIDE_SWAPINTERVAL", -1L);
  GDBG_INFO(80,"      swapInterval: %d\n",_GlideRoot.environment.swapInterval);
  _GlideRoot.environment.snapshot          = GLIDE_GETENV("FX_SNAPSHOT", -1L);
  GDBG_INFO(80,"          snapshot: %d\n",_GlideRoot.environment.snapshot);
  _GlideRoot.environment.guardbandclipping = GLIDE_GETENV("FX_GLIDE_GBC", 1L);
  GDBG_INFO(80," guardbandclipping: %d\n",_GlideRoot.environment.guardbandclipping);
  /* KoolSmoky - enable 2ppc only in certain condition. -1=disable 1=enable 0=glide desides. always enabled. */
  _GlideRoot.environment.do2ppc            = GLIDE_GETENV("FX_GLIDE_2PPC", 1L);
  GDBG_INFO(80," do2ppc           : %d\n",_GlideRoot.environment.do2ppc);
  _GlideRoot.environment.band2ppc          = GLIDE_GETENV("FX_GLIDE_2PPC_BAND", 2L);
  GDBG_INFO(80," band2ppc         : %d\n",_GlideRoot.environment.band2ppc);
  _GlideRoot.environment.sliBandHeight     = GLIDE_GETENV("FX_GLIDE_SLI_BAND_HEIGHT", 0L);
  GDBG_INFO(80," sliBandHeight    : %d\n",_GlideRoot.environment.sliBandHeight);
  _GlideRoot.environment.aaClip            = GLIDE_GETENV("FX_GLIDE_AA_CLIP", 1L);
  GDBG_INFO(80," aaClip           : %d\n",_GlideRoot.environment.aaClip);
  
  _GlideRoot.environment.sliBandHeightForce = GLIDE_GETENV("FX_GLIDE_FORCE_SLI_BAND_HEIGHT", 0L);
  GDBG_INFO(80," sliBandHeightForce : %d\n",_GlideRoot.environment.sliBandHeightForce);
  
  _GlideRoot.environment.swapPendingCount  = GLIDE_GETENV("FX_GLIDE_SWAPPENDINGCOUNT", 3L);
  /* The hardware counter is 3 bits. Anything above this will cause a hang. */
  if (_GlideRoot.environment.swapPendingCount > 6)
    _GlideRoot.environment.swapPendingCount = 6;
  if (_GlideRoot.environment.swapPendingCount < 0)
    _GlideRoot.environment.swapPendingCount = 0;
  GDBG_INFO(80," swapPendingCount : %d\n",_GlideRoot.environment.swapPendingCount);
  
  _GlideRoot.environment.gammaR = GLIDE_FGETENV("SSTH3_RGAMMA", 1.0f);
  _GlideRoot.environment.gammaG = GLIDE_FGETENV("SSTH3_GGAMMA", 1.0f);
  _GlideRoot.environment.gammaB = GLIDE_FGETENV("SSTH3_BGAMMA", 1.0f);
  
  _GlideRoot.environment.useAppGamma  = GLIDE_GETENV("FX_GLIDE_USE_APP_GAMMA", 1L);
  
#if __POWERPC__ && PCI_BUMP_N_GRIND
  _GlideRoot.environment.autoBump = FXFALSE;
#else  
  _GlideRoot.environment.autoBump = (GETENV("FX_GLIDE_BUMP") == NULL);
#if CHECK_SLAVE_SWAPCMD
  _GlideRoot.environment.checkSlaveSwapCMD = GLIDE_GETENV("FX_GLIDE_CHECKSLAVESWAPCMD", 0L);
#endif
#if TACO_MEMORY_FIFO_HACK
  _GlideRoot.environment.memFIFOHack = (GETENV("FX_GLIDE_MEMFIFOHACK") != NULL);
#endif
#endif  
  GDBG_INFO(80, "\tautoBump:          %s\n",
            _GlideRoot.environment.autoBump ? "FXTRUE" : "FXFALSE");
  
  if (GETENV("FX_GLIDE_BUMPSIZE"))
  {
    unsigned int u;
    if (sscanf(GETENV("FX_GLIDE_BUMPSIZE"), "%x", &u) == 1)
      _GlideRoot.environment.bumpSize = u;
  }
  else
#if __POWERPC__  
    _GlideRoot.environment.bumpSize = 0x1000;
#else
    _GlideRoot.environment.bumpSize = 0x10000;
#endif
  
  _GlideRoot.environment.fenceLimit =
    GLIDE_GETENV("FX_GLIDE_FENCE_LIMIT", 0x10000);
    if (_GlideRoot.environment.fenceLimit > 0x10000)
      _GlideRoot.environment.fenceLimit = 0x10000;
  
  /* Pre-convert the bump size from bytes to words */
  _GlideRoot.environment.bumpSize >>= 2UL;
  GDBG_INFO(80, "\tbumpSize:          0x%x\n", _GlideRoot.environment.bumpSize);
  
  _GlideRoot.current_sst = 0;                    /* make sure there's a valid GC */
  _GlideRoot.initialized = FXTRUE;               /* save this for the end */
  
  
  /* display info for all sst devices */
  for (i = 0; i < _GlideRoot.hwConfig.num_sst; i++) {
    displayBoardInfo(i, &_GlideRoot.hwConfig);
  }

} /* _GlideInitEnvironment */


#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
BOOL WINAPI 
DllMain(HANDLE hInst, ULONG  ul_reason_for_call, LPVOID lpReserved) 
{
  switch( ul_reason_for_call ) {
  case DLL_PROCESS_DETACH:
    GDBG_INFO(80, "DllMain: DLL_PROCESS_DETACH\n");
    grGlideShutdown();
#ifdef GLIDE_INIT_HWC
    hwcUnmapMemory();
    hwcClearContextData();
#endif
    /* A clear thinking person might assume that this should go
     * in grGlideShutdown, however Microsoft claims it should
     * go here in DLL_PROCESS_DETACH... More importantly, moving
     * this call to here fixes PRS#12190, 12192 and 12196
     * (several games that like to do wacky crap with threads).
     */
    freeThreadStorage();

    /* clean up */
    deleteCriticalSection();
    break;
  case DLL_PROCESS_ATTACH:
    GDBG_INFO(80, "DllMain: DLL_PROCESS_ATTACH\n");
    break;
  case DLL_THREAD_ATTACH:
    GDBG_INFO(80, "DllMain: DLL_THREAD_ATTACH\n");
    /* If we're in the fullscreen case it is legal to call into glide
     * from multiple threads, but the application is required to do
     * their own thread syncronization w/ glide as a single resource.
     * If we've already checked for the hw and they have called
     * grSstWinOpen at least once to open a fullscreen context then we
     * need to attach the current context (eg. the gc) to the current tls.
     */
    /* [koolsmoky] As long as the application attaches a valid gc with
     * grSelectContext from its created thread, it's legal to call glide
     * from multipule threads in windowed mode as well. (See grSstSelect)
     */
    /* attach a fullscreen gc to the TLS slot if in fullscreen mode. */
    if(_GlideRoot.initialized &&       /* scanned for hw? */
        (_GlideRoot.windowsInit[_GlideRoot.current_sst] > 0)) { /* outstanding fullscreen contexts? */
      GR_DCL_GC;

      /* If there is no current gc in tls then set the current context. */
      if (gc == NULL) setThreadValue((unsigned long)&_GlideRoot.GCs[_GlideRoot.current_sst]);
    }
    break;
  case DLL_THREAD_DETACH:
    GDBG_INFO(80, "DllMain: DLL_THREAD_DETACH\n");
    break;
  default:
    GDBG_INFO(80, "DllMain: Unhandled message.\n");
    break;
  }
  
  return TRUE;

} /* DllMain */
#endif
