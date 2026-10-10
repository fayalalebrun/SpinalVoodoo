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
** 
** $Log:
**  51   3dfx      1.41.1.6.1.110/11/00 Brent           Forced check in to enforce
**       branching.
**  50   3dfx      1.41.1.6.1.008/29/00 Jonny Cochrane  Some 8x FSAA code
**  49   3dfx      1.41.1.6    06/20/00 Joseph Kain     Fixed errors introduced by
**       my previous merge.
**  48   3dfx      1.41.1.5    06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  47   3dfx      1.41.1.4    06/18/00 Stephane Huaulme fixed toggle and screen
**       shot key for the Macintosh
**  46   3dfx      1.41.1.3    06/15/00 Bill White      Merged changes to support
**       Linux.
** 
**  45   3dfx      1.41.1.2    05/25/00 Kenneth Dyke    Fixed bogus file time
**       stamp.
**  44   3dfx      1.41.1.1    05/24/00 Kenneth Dyke    Added MacOS support for
**       screenshots and AA toggling.
**  43   3dfx      1.41.1.0    05/09/00 Kenneth Dyke    Paranoia fix to make sure
**       fogMode is updated properly after grGlideSetState().
**  42   3dfx      1.41        04/21/00 Kenneth Dyke    Fix for ITRGBA parameter
**       setup.  Added 2D nop after a WAX clear.
**  41   3dfx      1.40        04/13/00 Kenneth Dyke    Added support for new style
**       2-sample AA mode.
**  40   3dfx      1.39        04/10/00 Kenneth Dyke    Added magical screenshot
**       hotkey.
**  39   3dfx      1.38        04/06/00 Kenneth Dyke    More paranoia to make
**       buffer clears safer in windowed mode.
**  38   3dfx      1.37        04/05/00 Adam Briggs     I can't believe the guy who
**       wrote the 2D buffer clear didn't implement code for the swapped y-origin
**       case!  Anyway, it's fixed now.
**  37   3dfx      1.36        04/03/00 Adam Briggs     Added a nop after a
**       bufferswap
**  36   3dfx      1.35        03/24/00 Chris Dow       If I cut off your pinky
**       with my bolt cutters, then you can't make the // comment.
**  35   3dfx      1.34        03/24/00 Kenneth Dyke    Removed stencil clear
**       optimization since it's non-corformant. :-\
**  34   3dfx      1.33        03/23/00 Kenneth Dyke    Buffer clear optimizations
**       and fixes for tbuffering.
**  33   3dfx      1.32        03/21/00 Don Mullis      Fixes stencil-related
**       failures reported in PRS #13274-13277.
**  32   3dfx      1.31        03/19/00 Kenneth Dyke    Mode 2ppc mode state a bit
**       more robust.
**  31   3dfx      1.30        03/13/00 Kenneth Dyke    Remove A0 AA swap
**       workaround.
**  30   3dfx      1.29        03/11/00 Kenneth Dyke    Make sure two pixel per
**       clock mode is disabled when restoring glide state.
**  29   3dfx      1.28        03/06/00 Kenneth Dyke    Added backdoor hack to
**       toggle AA on and off on the fly.
**  28   3dfx      1.27        03/01/00 Kenneth Dyke    Fix for random aux buffer
**       clears.
**  27   3dfx      1.26        02/28/00 Kenneth Dyke    Fixed dither rotation stuff
**       for 4-sample AA.  We also no longer clobber the fog mode setting.
** 
**  26   3dfx      1.25        02/22/00 Kenneth Dyke    Work around compiler
**       optimization bug.
**       Fix alpha blend modes during triangle clear.
**       Safer combineMode settings for TMUs during triangle clear.
**  25   3dfx      1.24        02/18/00 Adam Briggs     added FX_GLIDE_WAX_ON var
**       which must be set to 1 to enable the newly uncommented SLI WAX buffer
**       clears
**  24   3dfx      1.23        02/07/00 Kenneth Dyke    Removed slave chip stuff
**       for pending swap count.
**  23   3dfx      1.22        02/03/00 Kenneth Dyke    Allow grTBufferWriteMaskExt
**       to work with two sample AA.
**  22   3dfx      1.21        01/31/00 Adam Briggs     changed the IS_NAPALM macro
**       to cooperate with the display driver version of the same
**  21   3dfx      1.20        01/31/00 Adam Briggs     Changed all device ID magic
**       numbers to use those defined in fxhal.h & added IS_NAPALM macro to test
**       against device ID range
**  20   3dfx      1.19        01/30/00 Adam Briggs     get napalm status correctly
**  19   3dfx      1.18        01/28/00 Kenneth Dyke    Totoally revamped TMU
**       register update mechanism to make 2PPC modes work right regardless of the
**       order of Glide calls.   Also fixed a few register config bugs found when
**       switching between new and old style combine modes.
**  18   3dfx      1.17        01/18/00 Kenneth Dyke    Get AA jitter values from
**       the right place.
**  17   3dfx      1.16        01/16/00 Kenneth Dyke    More minor AA swap fixes.
**  16   3dfx      1.15        01/16/00 Kenneth Dyke    Fixes for windowed 32-bit
**       buffer clears using WAX.
**       Force swap interval >0 for AA modes.
**       AA dither rotation matrix stuff.
**  15   3dfx      1.14        01/07/00 Adam Briggs     Moved freeThreadStorage
**       from grShutdown to the DllMain process detach code.  This fixes PRS#12190,
**       12192 and 12196.
**  14   3dfx      1.13        12/04/99 Adam Briggs     Made aux buffer get cleared
**       correctly while in AA modes & incorporated dwm's recommendation about aux
**       buffer addressing (avoids an if).
**  13   3dfx      1.12        11/29/99 Adam Briggs     Oops: color buffer stride
**       was being used to clear aux buffer.
**  12   3dfx      1.11        11/10/99 Adam Briggs     Made grBufferClear(Ext)
**       cooperate with linear surfaces & texture buffers
**  11   3dfx      1.10        11/08/99 Adam Briggs     Initialized doStencil &
**       ifdef'd it out on non-Napalm builds.
**  10   3dfx      1.9         11/05/99 Adam Briggs     Fixed a bug or so with
**       grBufferClear(Ext) and doubtless added several more.  Also added commented
**       out code for better buffer clears on Napalm SLI.
**  9    3dfx      1.8         11/05/99 Anthony tai     added
**       FX_GLIDE_SWAPPENDINGCOUNT. Default=1. Range 0-3
**  8    3dfx      1.7         10/21/99 Anthony tai     fixed iterated rgb checking
**  7    3dfx      1.6         10/07/99 Adam Briggs     Implemented stencil clears
**       for grBufferClearExt
**  6    3dfx      1.5         09/28/99 Adam Briggs     'nother fix for 24bit depth
**       clears.. In theory they should now work when doing depth-only clears.
**  5    3dfx      1.4         09/24/99 Adam Briggs     In theory, fixed 24bit
**       BufferClear & BufferClearExt for windowed glide.
**  4    3dfx      1.3         09/23/99 Adam Briggs     changed grBufferClear so
**       the 16->24 depth conversion doesn't happen in 32bpp mode.
**  3    3dfx      1.2         09/20/99 Adam Briggs     put IFDEF NAPALM around a
**       change so I no longer hose other people... for now!
**  2    3dfx      1.1         09/17/99 Adam Briggs     Supported TEXTUREBUFFEREXT
**       for Napalm 32bpp and AA modes.
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 122   9/01/99 2:22p Atai
** fixed h3 build
** 
** 121   8/31/99 5:49p Atai
** fixed 2nd colbuffer mask
** 
** 120   8/16/99 11:18a Adamb
** Merged in V3_OEM_100 fixes
** 
** 119   7/23/99 2:01p Atai
** change tbuffer interface to grTBufferWriteMaskExt
** 
** 118   7/22/99 1:18p Atai
** added grTBufferMaskExt
** 
** 117   7/19/99 6:01p Sbrooks
** Defined freeThreadStorage()
** 
** 116   7/19/99 4:30p Kcd
** Fixed portability problem with freeing up thread local storage.
** 
** 115   7/18/99 1:59p Atai
** added grAlphaBlendFunctionExt
** 
** 113   7/16/99 5:10p Sbrooks
** Fixes PRS 6869
** 
** 112   7/16/99 10:59a Atai
** remove un-supported mode
** fixed tcc, tac problem
** 
** 111   7/14/99 5:07p Atai
** fixed stencil interface and some cc/ac stuff
** 
** 110   7/14/99 9:39a Atai
** direct register write for glide3x
** test04 can do 4 sample aa (2 chips)
** 
** 109   7/06/99 2:44p Atai
** added gbc
** 
** 108   6/29/99 7:19p Atai
** remove argument for enabling SST_CM_USE_COMBINE_MODE
** 
** 107   6/29/99 2:55p Atai
** added invert mode for COMBINE ext c and d term
** 
** 106   6/27/99 12:44p Atai
** fixed CC and TCC
** 
** 105   6/25/99 2:11p Atai
** more 2 buffers stuff
** 
** 104   6/24/99 7:17p Atai
** added STENCIL and COMBINE extension, 2 buffers per chip, and more
** register state recovery
** 
** 103   6/19/99 11:36p Atai
** restore renderMode
** 
** 102   6/14/99 10:41a Stb_gkincade
** Improved comments for image quality code in grDitherMode()
** 
** 101   6/13/99 4:55p Atai
** fixed register mask
** 
** 100   6/10/99 5:12p Atai
** fist pass CC and AC ext
** 
** 99    6/04/99 3:18p Atai
** stencilMode and stencilOp
** 
** 98    6/04/99 1:47p Atai
** fixed my ·Fed code
** 
** 97    6/04/99 11:00a Atai
** added stencil functions
** 
** 96    5/28/99 1:11p Atai
** again
** 
** 94    5/19/99 3:55p Denis
** 
** 93    5/19/99 12:45p Denis
** First check in of the TEXTUREBUFFER extension.
** Contains both the texture color buffer and texture aux. buffer
** extensions
** that allows to specify a piece of texture memory as a rendering target
** and/or a piece of texture memory as the aux. buffer.
** 
** Probably a non conventional check in, in the sense that the API
** isn't entirely frozen / specified yet. To ease whoever's job it will be
** to complete the extension, I've added a tbext comment
** everywhere I made a modification. These should go away
** once the API is frozen.
**  
** 
** 92    5/13/99 2:42p Stb_gkincade
** Force dither matrix to 2x2 and Added user support for turning on/off
** dither substraction
** 
** 91    4/28/99 3:48p Atai
** disable dither subtraction if GR_DITHER_DISABLE
** 
** 90    4/14/99 5:05p Dow
** Fixed compiler error.
** 
** 89    4/14/99 4:22p Dow
** Snarl
** 
** 88    4/10/99 1:23p Atai
** fixed code to compile in packet fifo mode 
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
** 
** 69    12/02/98 11:30a Dow
** Fixed effed-up size checking in gglide.c
** 
** 68    11/30/98 6:57p Peter
** windowed texture f*ckage
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
 * fixed glide3 effage
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
 * fixed all the effage from new config
 * 
 * 104   11/12/97 2:27p Peter
 * simulator happiness w/o fifo
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

#if (GLIDE_PLATFORM & GLIDE_SST_SIM)
#if HAL_CSIM
#include <csim.h>
#else /* !HAL_CSIM */
#include <gsim.h>
#endif /* !HAL_CSIM */
#endif /* (GLIDE_PLATFORM & GLIDE_SST_SIM) */

#ifdef DRI_BUILD
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
  if (gc->grPixelSize == 4) {
    switch (alpha_sf) {
    case GR_BLEND_ZERO:
    case GR_BLEND_SRC_ALPHA:
    case GR_BLEND_DST_ALPHA:
    case GR_BLEND_ONE:
    case GR_BLEND_ONE_MINUS_SRC_ALPHA:
    case GR_BLEND_ONE_MINUS_DST_ALPHA:
      break;
    default:
/*      GR_CHECK_W(myName, 1, "unsupported alpha source blend function"); */
      alpha_sf = GR_BLEND_ONE;
      break;      
    }
    switch (alpha_df) {
    case GR_BLEND_ZERO:
    case GR_BLEND_SRC_ALPHA:
    case GR_BLEND_DST_ALPHA:
    case GR_BLEND_ONE:
    case GR_BLEND_ONE_MINUS_SRC_ALPHA:
    case GR_BLEND_ONE_MINUS_DST_ALPHA:
      break;
    default:
/*      GR_CHECK_W(myName, 1, "unsupported alpha destination blend function"); */
      alpha_df = GR_BLEND_ZERO;
      break;      
    }
  }
  else {
  if (alpha_sf != GR_BLEND_ONE && alpha_sf != GR_BLEND_ZERO) {
/*    GR_CHECK_W(myName, 1, "unsupported alpha source blend function"); */
    alpha_sf = GR_BLEND_ONE;
  }
  if (alpha_df != GR_BLEND_ONE && alpha_df != GR_BLEND_ZERO) {
/*    GR_CHECK_W(myName, 1, "unsupported alpha destination blend function"); */
    alpha_df = GR_BLEND_ZERO;
  }
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
  gc->state.tcc_requires_it_alpha[GR_TMU0] = FXFALSE;
  gc->state.tcc_requires_it_alpha[GR_TMU1] = FXFALSE;
  gc->state.tac_requires_it_alpha[GR_TMU0] = FXFALSE;
  gc->state.tac_requires_it_alpha[GR_TMU1] = FXFALSE;

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

/* */
/* GASP: The dreaded WAX buffer clear. */
/* */
#ifndef HAL_CSIM
static void
_grBufferClear2D(/*const*/ FxU32 buffOffset, 
                 const FxU32 clipLeft, const FxU32 clipTop,
                 const FxU32 clipRight, const FxU32 clipBottom, 
                 const FxU32 color, const FxU32 mask, const FxBool tiled,
                 const FxU32 stride) 
{
#define FN_NAME "_grBufferClear2D"
  FxU32                   /* Registers we use */
    clip0min, clip0max, dstBaseAddr, dstFormat, colorFore, command;
  FxU32
    regMask = 0L;
  FxU32
    rop = SSTG_ROP_SRCCOPY ; /* Default to SRCCOPY ROP */
  FxU32
    commandEx = 0x00000000; 
  FxU32 width, height;

  GR_BEGIN_NOFIFOCHECK("_grBufferClear2D", 86);

  if (mask == 0) /* This one is easy! */
    return ;
  
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, nopCMD, 0x0);
  GR_CHECK_SIZE();

  /* */
  /* In 3D land we can only apply a write mask in 32bpp modes */
  /* so we can be lame and do the same thing here. */
  /* */
  if ((gc->grPixelSize == 4) && 
        (mask != 0xffffffff)) 
  {
    int i ;

    rop = 0xCA000000 ;      /* Dst = (Pat & Src) | Dst */
    commandEx = SSTG_PAT_FORCE_ROW0 ; /* use only row 0 of pat */

    /* Set only row 0 of pat */
    REG_GROUP_BEGIN_WAX(colorPattern[0], 8, 0xFF) ;
    for (i = 0 ;
         i < 8 ;
         i++)
      REG_GROUP_SET_WAX(gc->gRegs, colorPattern[i], mask) ;
    REG_GROUP_END() ;
  } 

#define ADDWAXMASK(mask, reg, base) mask |= (1 << ((offsetof(SstGRegs, reg) - offsetof(SstGRegs, base)) >> 2))

  ADDWAXMASK(regMask, clip0min, clip0min);
  ADDWAXMASK(regMask, clip0max, clip0min);
  ADDWAXMASK(regMask, dstBaseAddr, clip0min);        
  ADDWAXMASK(regMask, dstFormat, clip0min);
  ADDWAXMASK(regMask, commandEx, clip0min);

  REG_GROUP_BEGIN_WAX(clip0min, 5, regMask);

  width = clipRight - clipLeft;
  height = clipBottom - clipTop;

  /* Setup clipping rectangle based on top left origin */
  if (gc->state.shadow.fbzMode & SST_YORIGIN)
  {
    FxU32    flippedTop ;
    FxU32    flippedBottom ;
#if 1
    FxU32    maxY;

    /* [dBorca] Hack alert:
     * textureAuxBuffer is not correctly handled!
     */
    if (gc->textureBuffer.on) {
       maxY = gc->textureBuffer.height;
    } else {
       maxY = gc->state.screen_height >> (gc->sliCount >> 1);
    }

    flippedTop = maxY - clipBottom ;
    flippedBottom = maxY - clipTop ;

    /* [dBorca] Hack alert:
     * textureAuxBuffer is not correctly handled!
     * Compensate for grTextureBuffer subtraction in `gtex.c'. Why? Oh, why?
     * Could be a bug in hardware logic?
     * It seems the non-tiled path is not correctly understood by WAX!
     * I'm not sure about the next condition... should we use (!tiled) instead?
     */
    if (gc->textureBuffer.on) {
       buffOffset += ( gc->state.screen_height - height ) * gc->textureBuffer.width * gc->grPixelSize;
    }
#else
    flippedTop = (gc->state.screen_height >> (gc->sliCount >> 1))
                 - height - clipTop ;
    flippedBottom = flippedTop + height ;
#endif

    clip0min = ((flippedTop << 16) | clipLeft);
    clip0max = ((clipRight) | ((flippedBottom) << 16));
  }
   else
  {
    clip0min = ((clipTop << 16) | clipLeft);
    clip0max = ((clipRight) | ((clipBottom) << 16));
  }

  REG_GROUP_SET_WAX(gc->gRegs, clip0min, clip0min);
  REG_GROUP_SET_WAX(gc->gRegs, clip0max, clip0max);

  /* Set buffer address and or in the tiled bit */
#if 0 /* [dBorca] dumb assumption */
  dstBaseAddr = buffOffset | ((stride & SST_BUFFER_MEMORY_TILED) ? SSTG_IS_TILED : 0) ;
#else
  dstBaseAddr = buffOffset | (tiled ? SSTG_IS_TILED : 0) ;
#endif
  REG_GROUP_SET_WAX(gc->gRegs, dstBaseAddr, dstBaseAddr);
  
  /* Build up dstFormat (Should this be done globally?) */
  dstFormat = stride & SST_BUFFER_LINEAR_STRIDE ;

  if (gc->grPixelSize == 2)
    dstFormat |= SSTG_PIXFMT_16BPP ; /* 16bpp */
  else
    dstFormat |= SSTG_PIXFMT_32BPP ; /* 32bpp */

  REG_GROUP_SET_WAX(gc->gRegs, dstFormat, dstFormat);
  
  /* Setup Command extra */
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
  REG_GROUP_SET_WAX(gc->gRegs, dstSize, (height << 16) | width);

  /* Setup dstXY - starting coordinate */
  REG_GROUP_SET_WAX(gc->gRegs, dstXY, clip0min);

  /* Setup command */
  command = SSTG_RECTFILL;    /* rectangle fill */
  command |= SSTG_GO;         /* Initiate immediately */
  command |= rop ;            /* ROP */

  REG_GROUP_SET_WAX(gc->gRegs, command, command);
  REG_GROUP_END();

  /* Force 2D idle */
  REG_GROUP_BEGIN_WAX(command, 1, command);
  command = SSTG_GO | SSTG_NOP; /* 2D NOP */
  REG_GROUP_SET_WAX(gc->gRegs, command, command);
  REG_GROUP_END();

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET(BROADCAST_ID, hw, nopCMD, 0x0);
  GR_CHECK_SIZE();

#undef FN_NAME
} /* _grBufferClear2D */
#endif



/* */
/* AJB: Stole all this noise from SST2 Glide, but the Non-Reference TRI fill */
/*      has gotten LOTS of re-write... Also a 2-Tri fill might be better on */
/*      Napalm, but some experimenting will be required. */
/* */
/* The triangle will look like this: */
/* */
/* A---B */
/*  \| |    */
/*   \-| */
/*    \| */
/*     C */
/*  */
/* A = (-width,0) */
/* B = (width+1, 0) */
/* C = (width+1, height*2) */
/* */
#define REFERENCE_TRI_FILL 0
#define TWO_TRI_FILL       0

void
_grTriFill(GrColor_t color, FxU32 depth, GrStencil_t stencil)
{
#define FN_NAME "_grTriFill"
#if REFERENCE_TRI_FILL
  GrState oldState ;
  GrDepthBufferMode_t bufferMode ;
#endif

  struct 
  {
    float x, y, depth ;
  }
#if TWO_TRI_FILL
  vertex[4] ;
#else
  vertex[3] ;
#endif

  GR_BEGIN_NOFIFOCHECK("_grTriFill", 86);

#if REFERENCE_TRI_FILL
/* */
/*warning Reference triangle fill code is not fixing chromakey mode.   */
/* */
  /* Note: The following is a "reference" implementation of triangle buffer
   * clearing based 'mostly' on Glide API calls. */
  grGlideGetState(&oldState);

  bufferMode = gc->state.stateArgs.grDepthBufferModeArgs.mode ;

  /* AJB - Flat shaded Tri. */
  grColorCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_NONE, FXFALSE) ;

  /* This takes care of color and alpha */
  grConstantColorValue(color);

  /* Set up to force depth buffer comparison to succeed */
  grDepthBufferFunction(GR_CMP_ALWAYS);

  /* Figure out which way to set up vertex */
  /* Doing it this way means that we don't cause a PE cache */
  /* flush due to changing depth buffer modes */
  if (bufferMode == GR_DEPTHBUFFER_WBUFFER ||
      bufferMode == GR_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS)
  {        
    grVertexLayout(GR_PARAM_Z, 0, GR_PARAM_DISABLE) ;
    grVertexLayout(GR_PARAM_Q, 8, GR_PARAM_ENABLE) ;
  } 
   else 
  if (bufferMode == GR_DEPTHBUFFER_ZBUFFER ||
      bufferMode == GR_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS) 
  { 
    grVertexLayout(GR_PARAM_Z, 8, GR_PARAM_ENABLE) ;
    grVertexLayout(GR_PARAM_Q, 0, GR_PARAM_DISABLE) ;
  }

#ifdef FX_GLIDE_NAPALM
  /* AJB - 2ppc rendering */
  _grTex2ppc(GR_TMU0, FXTRUE) ;

  /* Set up stencil stuff */
  grStencilFunc(GR_CMP_ALWAYS,stencil,0xFF);
  grStencilOp(GR_STENCILOP_REPLACE,GR_STENCILOP_REPLACE,GR_STENCILOP_REPLACE);
#endif

  /* Make sure we're doing window coords */
  grCoordinateSpace(GR_WINDOW_COORDS);
  
  /* Finally draw the triangle! */

#if TWO_TRI_FILL
  vertex[0].x =  gc->state.clipwindowf_xmin ;
  vertex[0].y =  gc->state.clipwindowf_ymin ;
  vertex[0].depth = (float)depth ;
  vertex[1].x =  gc->state.clipwindowf_xmax ;
  vertex[1].y =  gc->state.clipwindowf_ymin ;
  vertex[1].depth = (float)depth ;
  vertex[2].x =  gc->state.clipwindowf_xmax ;
  vertex[2].y =  gc->state.clipwindowf_ymax ;
  vertex[2].depth = (float)depth ;
  vertex[3].x =  gc->state.clipwindowf_xmin ;
  vertex[3].y =  gc->state.clipwindowf_ymax ;
  vertex[3].depth = (float)depth ;

  grDrawTriangle(&vertex[0],&vertex[1],&vertex[2]) ; 
  grDrawTriangle(&vertex[0],&vertex[2],&vertex[3]) ; 
#else
  vertex[0].x = 0.0f -(float)gc->state.screen_width ;
  vertex[0].y = 0.0f;
  vertex[0].depth = (float)depth;
  vertex[1].x = (float)gc->state.screen_width + 1;
  vertex[1].y = 0.0f;
  vertex[1].depth = (float)depth;
  vertex[2].x = (float)gc->state.screen_width + 1;
  vertex[2].y = (float)gc->state.screen_height * 2;
  vertex[2].depth = (float)depth;

  grDrawTriangle(&vertex[0],&vertex[1],&vertex[2]); 
#endif

  /* Restore all touched state */
  grGlideSetState(&oldState);
#else
  /* */
  /* Here is the 'real' implementation */
  /* */
  {
    /* Save off regs that this function will trash */
    GrDepthBufferMode_t bufferMode ;
    FxU32               fbzMode = gc->state.shadow.fbzMode ;
    FxU32               color0 = gc->state.shadow.color0 ;
    FxU32               color1 = gc->state.shadow.color1 ;
    FxU32               fbzColorPath = gc->state.shadow.fbzColorPath ;
    FxU32               alphaMode = gc->state.shadow.alphaMode;
#ifdef FX_GLIDE_NAPALM
    FxU32               combineMode;
    FxU32               stencilMode;
    FxU32               stencilOp;
#endif

    bufferMode = gc->state.stateArgs.grDepthBufferModeArgs.mode ;

    GR_CHECK_FOR_ROOM(256, 0);  /* Probably a little overkill */
    
    /* */
    /* Set up rendering for flat shading */
    /* */
    fbzMode &= ~(SST_WBUFFER | SST_ZFUNC | SST_ENCHROMAKEY | SST_ENZBIAS | SST_DEPTH_FLOAT_SEL);
    fbzMode |= (GR_CMP_ALWAYS << SST_ZFUNC_SHIFT);

    REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 1, 0x1) ;
    {
      REG_GROUP_SET(hw, fbzMode, fbzMode ) ;
    }
    REG_GROUP_END();


    /* Set the constant color to the given color */
    REG_GROUP_BEGIN(BROADCAST_ID, c0, 2, 0x03) ;
    {
      REG_GROUP_SET(hw, c0, color) ;
      REG_GROUP_SET(hw, c1, color) ;
    }
    REG_GROUP_END() ;

    /* Set the blend mode to only use the constant color */
    REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, 1, 0x1) ;
    {
      REG_GROUP_SET(hw, fbzColorPath, (GR_COMBINE_FACTOR_NONE    << SST_CC_MSELECT_SHIFT)  |
                                      (GR_COMBINE_LOCAL_CONSTANT << SST_LOCALSELECT_SHIFT) |
                                      (GR_COMBINE_OTHER_NONE     << SST_RGBSELECT_SHIFT)   |
                                       SST_CC_ZERO_OTHER |
                                       SST_CC_ADD_CLOCAL) ;
    }
    REG_GROUP_END() ;

    /* Make sure alpha blending is disabled  */
    REG_GROUP_BEGIN(BROADCAST_ID, alphaMode, 1, 0x01) ;
    {
      REG_GROUP_SET(hw, alphaMode, 0);
    }
    REG_GROUP_END();

#ifdef FX_GLIDE_NAPALM
#if 0
    /* if we haven't already, enable 2 pixel/clock rendering */
    if (!gc->mode2ppc)
      _grTex2ppc(GR_TMU0, FXTRUE) ;
#endif
    combineMode = gc->state.shadow.combineMode ;
    stencilMode = gc->state.shadow.stencilMode ;
    stencilOp   = gc->state.shadow.stencilOp ;

    /* if we're in 32bpp mode, setup to clear the stencil BUT DON'T CHANGE THE STENCIL MASK! */
    if (gc->grPixelSize == 4)
    {
      REG_GROUP_BEGIN(BROADCAST_ID, stencilMode, 2, 0x3) ;
      {
        REG_GROUP_SET(hw, stencilMode, (GR_CMP_ALWAYS << SST_STENCIL_FUNC_SHIFT) |
                                       (stencil       << SST_STENCIL_REF_SHIFT)  |
                                       (stencilMode & (SST_STENCIL_WMASK | SST_STENCIL_MASK)) |
                                        SST_STENCIL_ENABLE) ;
        REG_GROUP_SET(hw, stencilOp, (GR_STENCILOP_REPLACE << SST_STENCIL_SFAIL_OP_SHIFT) |
                                     (GR_STENCILOP_REPLACE << SST_STENCIL_ZFAIL_OP_SHIFT) |
                                     (GR_STENCILOP_REPLACE << SST_STENCIL_ZPASS_OP_SHIFT)) ;
      }
      REG_GROUP_END() ;
    }

    /* turn off whatever fancy Napalm combine mode was set & just use the old fashioned fbzColorPath */
    REG_GROUP_BEGIN(eChipFBI, combineMode, 1, 0x01) ;
    {
      REG_GROUP_SET(hw, combineMode, combineMode & ~(SST_CM_USE_COMBINE_MODE)) ;
    }
    REG_GROUP_END() ;

    {
      int tmu;
      
      for (tmu = 0 ;
           tmu < gc->num_tmu ;
           tmu++)
      {
        SstRegs* tmuregs = SST_TMU(hw, tmu);
        const FifoChipField chipField = (FifoChipField)(0x02UL << tmu);
        
        REG_GROUP_BEGIN(chipField, combineMode, 1, 0x01);
        {
          REG_GROUP_SET(tmuregs, combineMode, gc->state.shadow.tmuState[tmu].combineMode & ~(SST_CM_USE_COMBINE_MODE));
        }
        REG_GROUP_END();
      }
    }

#endif
    /* */
    /* Draw the triangle already! */
    /* */
#if TWO_TRI_FILL
    vertex[0].x =  (float)gc->state.clipwindowf_xmin ;
    vertex[0].y =  (float)gc->state.clipwindowf_ymin ;
    vertex[1].x =  (float)gc->state.clipwindowf_xmax ;
    vertex[1].y =  (float)gc->state.clipwindowf_ymin ;
    vertex[2].x =  (float)gc->state.clipwindowf_xmax ;
    vertex[2].y =  (float)gc->state.clipwindowf_ymax ;
    vertex[3].x =  (float)gc->state.clipwindowf_xmin ;
    vertex[3].y =  (float)gc->state.clipwindowf_ymax ;
#else
    vertex[0].x = 0.0f -(float)gc->state.screen_width ;
    vertex[0].y = 0.0f ;
    vertex[1].x = (float)gc->state.screen_width + 1 ;
    vertex[1].y = 0.0f ;
    vertex[2].x = (float)gc->state.screen_width + 1 ;
    vertex[2].y = (float)gc->state.screen_height * 2 ;
#endif

    /* Here I have assumed that bufferMode will be 0 when there is no */
    /* depth buffering and odd when z buffering. */
    if (bufferMode)
    {
      vertex[0].depth = (float)depth ;
#if TWO_TRI_FILL
      GR_SET_EXPECTED_SIZE(sizeof(float) * 3 * 4, 1) ;
      TRI_PACKET_BEGIN(kSetupFan,
                      SST_SETUP_Z << SSTCP_PKT3_PMASK_SHIFT, 
                      4, sizeof(float) * 3, SSTCP_PKT3_BDDDDD);
#else
      GR_SET_EXPECTED_SIZE(sizeof(float) * 3 * 3, 1) ;
      TRI_PACKET_BEGIN(kSetupStrip,
                      SST_SETUP_Z << SSTCP_PKT3_PMASK_SHIFT, 
                      3, sizeof(float) * 3, SSTCP_PKT3_BDDBDD);
#endif
      {
        TRI_SETF(vertex[0].x);
        TRI_SETF(vertex[0].y);
        TRI_SETF(vertex[0].depth);

        TRI_SETF(vertex[1].x);
        TRI_SETF(vertex[1].y);
        TRI_SETF(vertex[0].depth);

        TRI_SETF(vertex[2].x);
        TRI_SETF(vertex[2].y);
        TRI_SETF(vertex[0].depth);

#if TWO_TRI_FILL
        TRI_SETF(vertex[3].x);
        TRI_SETF(vertex[3].y);
        TRI_SETF(vertex[0].depth);
#endif
      }
      TRI_END ;
        
      GR_CHECK_SIZE() ;
    } 
    else
    {
      /* When depth buffering is disabled, don't send a depth component */
#if TWO_TRI_FILL
      GR_SET_EXPECTED_SIZE(sizeof(float) * 2 * 4, 1) ;
      TRI_PACKET_BEGIN(kSetupFan,
                      0, 
                      4, sizeof(float) * 2, SSTCP_PKT3_BDDDDD) ;
#else
      GR_SET_EXPECTED_SIZE(sizeof(float) * 2 * 3, 1) ;
      TRI_PACKET_BEGIN(kSetupStrip,
                      0, 
                      3, sizeof(float) * 2, SSTCP_PKT3_BDDBDD) ;
#endif
      {
        TRI_SETF(vertex[0].x);
        TRI_SETF(vertex[0].y);

        TRI_SETF(vertex[1].x);
        TRI_SETF(vertex[1].y);

        TRI_SETF(vertex[2].x);
        TRI_SETF(vertex[2].y);

#if TWO_TRI_FILL
        TRI_SETF(vertex[3].x);
        TRI_SETF(vertex[3].y);
#endif
      }
      TRI_END ;
        
      GR_CHECK_SIZE() ;
    }

    /* */
    /* Restore all the regs we trashed */
    /* */
    REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 1, 0x1) ;
    {
      REG_GROUP_SET(hw, fbzMode, gc->state.shadow.fbzMode) ;
    }
    REG_GROUP_END() ;


    REG_GROUP_BEGIN(BROADCAST_ID, c0, 2, 0x03) ;
    {
      REG_GROUP_SET(hw, c0, color0) ;
      REG_GROUP_SET(hw, c1, color1) ;
    }
    REG_GROUP_END() ;

    REG_GROUP_BEGIN(BROADCAST_ID, alphaMode, 1, 0x01) ;
    {
      REG_GROUP_SET(hw, alphaMode, alphaMode);
    }
    REG_GROUP_END();

    REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, 1, 0x1) ;
    {
      REG_GROUP_SET(hw, fbzColorPath, fbzColorPath) ;
    }
    REG_GROUP_END() ;


#ifdef FX_GLIDE_NAPALM
#if 0
    if (!gc->mode2ppc)
      _grTex2ppc(GR_TMU0, FXFALSE) ;
#endif
    if (gc->grPixelSize == 4)
    {
      REG_GROUP_BEGIN(BROADCAST_ID, stencilMode, 2, 0x3) ;
      {
        REG_GROUP_SET(hw, stencilMode, stencilMode) ;
        REG_GROUP_SET(hw, stencilOp, stencilOp) ;
      }
      REG_GROUP_END() ;
    }

    REG_GROUP_BEGIN(eChipFBI, combineMode, 1, 0x01) ;
    {
      REG_GROUP_SET(hw, combineMode, combineMode) ;
    }
    REG_GROUP_END() ;

    {
      int tmu;
      
      for (tmu = 0 ;
           tmu < gc->num_tmu ;
           tmu++)
      {
        SstRegs* tmuregs = SST_TMU(hw, tmu);
        const FifoChipField chipField = (FifoChipField)(0x02UL << tmu);
        
        REG_GROUP_BEGIN(chipField, combineMode, 1, 0x01);
        {
          REG_GROUP_SET(tmuregs, combineMode, gc->state.shadow.tmuState[tmu].combineMode);
        }
        REG_GROUP_END();
      }
    }
#endif

  }
#endif
#undef FN_NAME
} /* _grTriFill */

/*---------------------------------------------------------------------------
** grBufferClear
*/

GR_ENTRY(grBufferClear, void, (GrColor_t color, GrAlpha_t alpha, FxU32 depth))
{
#define FN_NAME "grBufferClear"
  GR_BEGIN_NOFIFOCHECK("grBufferClear", 86);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x,0x%x,0x%x)\n", color, alpha, depth);

  /* validate the state */
  if (gc->state.invalid) _grValidateState();

  {
    const GrColor_t oldc1 = gc->state.shadow.color1;
    const FxU32 oldzacolor = gc->state.shadow.zaColor;
    const FxU32 fbzMode = gc->state.shadow.fbzMode;
    FxU32 zacolor = oldzacolor;
#if FX_GLIDE_NAPALM
    FxBool restoreRenderMode = FXFALSE;
#endif    
    FxBool doneP = FXFALSE;
    FxBool doColorP, doAuxP = FXFALSE;

    /* validate the state */
    /* Setup source registers */
    doColorP = ((fbzMode & SST_RGBWRMASK) != 0);
    if (doColorP)
    {
      _grSwizzleColor(&color) ;

#ifdef FX_GLIDE_NAPALM
      /* OR in Alpha component */
      if (gc->grPixelSize == 4)
        color |= alpha << 24 ; 
#endif
    }

    if ((fbzMode & SST_ZAWRMASK) != 0) {
      if ((fbzMode & SST_ENALPHABUFFER) != 0) {
        doAuxP = FXTRUE;

        zacolor &= ~SST_ZACOLOR_ALPHA;
        zacolor |= (((FxU32) alpha) << SST_ZACOLOR_ALPHA_SHIFT);
        depth = alpha ;
      } else if ((fbzMode & SST_ENDEPTHBUFFER) != 0) {
        doAuxP = FXTRUE;

        if ((fbzMode & SST_WBUFFER) && (gc->grPixelSize == 4))
          depth = depth << 8 | 0xff ;

        zacolor &= ~SST_ZACOLOR_DEPTH;
        zacolor |= (((FxU32) depth) << SST_ZACOLOR_DEPTH_SHIFT);
      }
    }

    /* Why were we called? */
    if (!doColorP && !doAuxP) return;

    /* HACK! FIXME! This routine (or any of the ones it calls) is currently not updating
     * the register shadows properly.  This could cause things to screw up in
     * the windowed case, so we make sure that we have enough room in the fifo
     * to write everything out into one buffer.  %%KCD  */
    GR_CHECK_FOR_ROOM(512, 0);

    if (!doneP) {
#ifdef GLIDE_INIT_HWC

#ifdef FX_GLIDE_NAPALM
#ifndef HAL_CSIM   /* old school CSIM uses Nair instead of Wax */
       /* */
       /* do a hot WAX job on the buffer(s) */
       /* to get all of their little hairs off */
       /*   */
      if (gc->sliCount > 1)      /* This code can only possibly help in Sli */
      {
        /* We don't do bikini lines yet */
        /* ie, clip window must be on an even chip boundary */
        /* If we start doing bikini lines, we will need to either send diff't size */
        /* blts to each chip, or send extra single line blts to certain chips */
        /* in addition to a blt for the common area of the fill.  */

        /* [dBorca]
         * the `clipBottomTop' and `clipLeftRight' are defined wrong if we
         * want texturebuffer clears. Also failing the test below will get
         * us into `_grTriFill', which is not fixed for textureBuffer yet!
         */
        if ((!(gc->state.shadow.clipBottomTop & (((gc->sliCount << gc->sliBandHeight) - 1) << SST_CLIPTOP_SHIFT)) &&
           !(gc->state.shadow.clipBottomTop & (((gc->sliCount << gc->sliBandHeight) - 1) << SST_CLIPBOTTOM_SHIFT))) &&
            _GlideRoot.environment.waxon && ((gc->state.tbufferMask & 0xf) == 0xf)) /* KoolSmoky - come back to this! tbufferMask incorrect! */
        {
          FxU32 clipLeft, clipRight, clipTop, clipBottom ;
          FxU32 mask = 0xFFFFFFFF ;

          clipLeft = gc->state.shadow.clipLeftRight ;
          clipRight = clipLeft & SST_CLIPRIGHT ;
          clipLeft >>= SST_CLIPLEFT_SHIFT ;

          /* All of this wackiness here divides the the height of the blt by the */
          /* bandheight * number of chips blting (since WAX doesn't speak Sli, we have to */
          /* explicitly tell it to do less work). */
          clipBottom = gc->state.shadow.clipBottomTop ;
          clipTop = (clipBottom & SST_CLIPTOP) ; 
          clipTop >>= (gc->sliCount >> 1) ;
          clipBottom >>= (SST_CLIPBOTTOM_SHIFT + (gc->sliCount >> 1)) ;

          if (doColorP)
          {
            /* Color has already been swizzled to ARGB */
            /* now we get to squish it to the proper size */
            if (gc->grPixelSize == 2)
              color = ((color & 0xf8) >> 3)    |   /* blue */
                      ((color & 0xfc00) >> 5)  |   /* green */
                      ((color & 0xf80000) >> 8) ;  /* red */
             else
            {
              /* Apply color write mask for 32bpp mode */
              
              mask = ((gc->state.shadow.renderMode & SST_RM_ALPHA_WMASK) ? 0xFF000000 : 0) |
                     ((gc->state.shadow.renderMode & SST_RM_RED_WMASK) ? 0x00FF0000 : 0)   |
                     ((gc->state.shadow.renderMode & SST_RM_GREEN_WMASK) ? 0x0000FF00 : 0) |
                     ((gc->state.shadow.renderMode & SST_RM_BLUE_WMASK) ? 0x000000FF : 0) ;
            }

            /* call 2D buffer clear */
            
            if (gc->textureBuffer.on)
            {
#if 0 /* [dBorca] don't use colBuffer addr/stride */
              _grBufferClear2D(gc->state.shadow.colBufferAddr,
                               clipLeft, clipBottom, clipRight, clipTop, color,
                               mask, FXFALSE, gc->state.shadow.colBufferStride) ;
#else /* [dBorca] right now we are discarding any clipping into texturebuffers. Fix me! */
              _grBufferClear2D(gc->textureBuffer.addr,
                               0, 0, gc->textureBuffer.width, gc->textureBuffer.height, color,
                               mask, FXFALSE, gc->textureBuffer.stride) ;
#endif
            }
             else
            {
              _grBufferClear2D(gc->buffers0[gc->windowed ? 0 : gc->curBuffer],
                               clipLeft, clipBottom, clipRight, clipTop, color,
                               mask, gc->colTiled, gc->state.shadow.colBufferStride) ;

              /* If there is an AA buffer, clear that too. */
              if (gc->enableSecondaryBuffer)
                _grBufferClear2D(gc->buffers1[gc->windowed ? 0 : gc->curBuffer],
                                 clipLeft, clipBottom, clipRight, clipTop, color,
                                 mask, gc->colTiled, gc->state.shadow.colBufferStride) ;
            }
          }

          if (doAuxP)
          {
            /* How nice. Depth is already in the format we want. */
            /* Load up the slingshot, let one fly. */
            /* THIS COULD BE FASTER: If the last 2D blt we did was to the same */
            /* the clip window, src and dst offsets need not be re-asserted. */
         
        
            if (gc->textureAuxBuffer.on)
            {
#if 0 /* [dBorca] don't use auxBuffer addr/stride */
              _grBufferClear2D(gc->state.shadow.auxBufferAddr,
                               clipLeft, clipBottom, clipRight, clipTop, depth, mask,
                               FXFALSE, gc->state.shadow.auxBufferStride) ;
#else /* [dBorca] right now we are discarding any clipping into texturebuffers. Fix me! */
              _grBufferClear2D(gc->textureAuxBuffer.addr,
                               0, 0, gc->textureAuxBuffer.width, gc->textureAuxBuffer.height, color,
                               mask, FXFALSE, gc->textureAuxBuffer.stride) ;
#endif
            }
             else
            {
              _grBufferClear2D(gc->buffers0[gc->grColBuf],
                               clipLeft, clipBottom, clipRight, clipTop, depth,
                               mask, gc->auxTiled, gc->state.shadow.auxBufferStride) ;
             
              /* ?Is this wrong? */
              if (gc->enableSecondaryBuffer)
                _grBufferClear2D(gc->buffers1[gc->grColBuf],
                                 clipLeft, clipBottom, clipRight, clipTop, depth,
                                 mask, gc->auxTiled, gc->state.shadow.auxBufferStride) ;
            }
          }
        }
         else
        {
          /* */
          /* Not on an even sli bandheight boundary or range... */
          /* so use a triangle fill (yuck!) */
          /* */
          _grTriFill(color, depth, 0) ;
        }
      }
       else
#endif
#endif /* end of hot wax session  */
      {
        
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
          /* Windowed or SDRAM clears */
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
            
#ifdef FX_GLIDE_NAPALM
            if (gc->grPixelSize == 2)
              {
#endif
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
                
#if FX_GLIDE_NAPALM
                /* If we are in 15bpp mode, then turn it off so that clears work as expected
                   and we can clear all 16 bits. */
                if((gc->state.shadow.renderMode & SST_RM_3D_MODE) == SST_RM_15BPP) {
                  FxU32 renderMode = gc->state.shadow.renderMode;
                  renderMode &= ~SST_RM_15BPP;
                  REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 1, 0x01);
                  REG_GROUP_SET(hw, renderMode, renderMode);
                  REG_GROUP_END();      
                  restoreRenderMode = FXTRUE;
                }
#endif            
#ifdef FX_GLIDE_NAPALM
              }
            else
              {
                FxU32 renderMode = gc->state.shadow.renderMode ;
                
                /* All of that hoopla is no help at all in 32bpp mode */
                /* since the depth buffer is 24bpp */
                convertedDepth = depth ;
                
                /* Since we are treating the depth buffer as a color buffer, */
                /* We had best enable color writes. */
                renderMode |= SST_RM_RED_WMASK | SST_RM_GREEN_WMASK | SST_RM_BLUE_WMASK | SST_RM_ALPHA_WMASK ;
                
                REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 1, 0x01);
                REG_GROUP_SET(hw, renderMode, renderMode);
                REG_GROUP_END();      
                restoreRenderMode = FXTRUE;
              }
#endif
            
            /* Clear Aux Buffer */
            REG_GROUP_BEGIN(BROADCAST_ID, c1, 1, 0x1);
            REG_GROUP_SET(hw, c1, convertedDepth);
            REG_GROUP_END();
            
            /* tbext */
            REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3);
            REG_GROUP_SET(hw, colBufferAddr, gc->state.shadow.auxBufferAddr );
#ifdef DRI_BUILD
            REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer)? (FxU32)driInfo.stride :
                          gc->state.shadow.auxBufferStride);
#else	/* defined(DRI_BUILD) */
            REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.auxBufferStride );
#endif	/* defined(DRI_BUILD) */
            REG_GROUP_END();
#ifdef FX_GLIDE_NAPALM
            if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) 
              {
                if (gc->enableSecondaryBuffer)
                  {
                    REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
                    REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->grColBuf] | SST_BUFFER_BASE_SELECT);
                    REG_GROUP_END();
                  }
              }
#endif
            
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
          
          /* tbext */
          if ( gc->textureBuffer.on ) {
            REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3);
            REG_GROUP_SET(hw, colBufferAddr, gc->textureBuffer.addr );
            REG_GROUP_SET(hw, colBufferStride, gc->textureBuffer.stride );
            REG_GROUP_END();  
          } else {
            REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3);
            REG_GROUP_SET(hw, colBufferAddr, gc->buffers0[gc->windowed ? 0 : gc->curBuffer]);
#ifdef DRI_BUILD
            REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride : 
                          gc->state.shadow.colBufferStride );
#else
            REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
            REG_GROUP_END();
#ifdef FX_GLIDE_NAPALM
            if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
              if (gc->enableSecondaryBuffer) {
                REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
                REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->curBuffer] | SST_BUFFER_BASE_SELECT);
                REG_GROUP_END();
              }
            }
#endif
          } /* endif gc->texRender.on */
          
#ifdef FX_GLIDE_NAPALM
          /* If we did a 32bpp depth clear, restore the color write masks */
          if (restoreRenderMode)
            {
              REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 1, 0x01);
              REG_GROUP_SET(hw, renderMode, gc->state.shadow.renderMode);
              REG_GROUP_END();      
            }
#endif

          REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 1, 1);
          REG_GROUP_SET(hw, fbzMode, fbzMode);
          REG_GROUP_END();        
        }
      }
#else /* !GLIDE_INIT_HWC */
      /* */
      /* Life was simple in the days of DOS Glide */
      /* */
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
#endif /* !GLIDE_INIT_HWC */
    }
  }
  
#undef FN_NAME
} /* grBufferClear */


/*---------------------------------------------------------------------------
** grBufferClearExt
*/
GR_EXT_ENTRY(grBufferClearExt, void, (GrColor_t color, GrAlpha_t alpha, FxU32 depth, GrStencil_t stencil))
{
#define FN_NAME "grBufferClearExt"
  GR_BEGIN_NOFIFOCHECK("grBufferClearExt", 86);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x,0x%x,0x%x)\n", color, depth);

  /* validate the state so we can diddle the regs & restore them */
  if (gc->state.invalid)
    _grValidateState() ;

  {
    GrColor_t oldc1 = gc->state.shadow.color1 ;
    FxU32 oldzacolor = gc->state.shadow.zaColor ;
    FxU32 zacolor = oldzacolor ;
    FxU32 fbzMode = gc->state.shadow.fbzMode ;
#ifdef FX_GLIDE_NAPALM
    FxU32 oldstencilMode = gc->state.shadow.stencilMode ;
    FxU32 renderMode = gc->state.shadow.renderMode & ~SST_RM_DITHER_ROTATION ;
    FxBool doStencil = FXFALSE ;
#endif
    FxBool doColorP, doAuxP = FXFALSE, restoreRenderStencil = FXFALSE;

#ifdef FX_GLIDE_NAPALM
    if (gc->grPixelSize == 4) {
      doColorP = (gc->state.shadow.renderMode & SST_RM_RGBA_WMASK) ;
      /* OpenGL will always enable stencil mode before a clear if the client really wanted
       * to clear the stencil buffer.  So, we won't mess with it if we don't have stenciling
       * enabled to save time. */      
      doStencil = (oldstencilMode & SST_STENCIL_WMASK) && (oldstencilMode & SST_STENCIL_ENABLE);
      if(doStencil) {
        /* Note that we cleared the stencil buffer at least once, so from
          * now on we have to preserve it's contents when we do a buffer clear. */
        gc->stencilCleared = FXTRUE;
      }
    }
    else
#endif
      doColorP = ((fbzMode & SST_RGBWRMASK) != 0) ;

    if ((fbzMode & SST_ZAWRMASK) != 0) {
      if ((fbzMode & SST_ENALPHABUFFER) != 0) {
        doAuxP = FXTRUE ;

        zacolor &= ~SST_ZACOLOR_ALPHA ;
        zacolor |= (((FxU32) alpha) << SST_ZACOLOR_ALPHA_SHIFT) ;
        depth = alpha ;
      } else if ((fbzMode & SST_ENDEPTHBUFFER) != 0) {
        doAuxP = FXTRUE;

        zacolor &= ~SST_ZACOLOR_DEPTH ;
        zacolor |= (((FxU32) depth) << SST_ZACOLOR_DEPTH_SHIFT) ;
      }
    }

      
    /* Why were we called? */
#ifdef FX_GLIDE_NAPALM    
    if ((!doColorP &&
         !doAuxP)  &&
         !doStencil) return;
#else
    if (!doColorP &&
        !doAuxP)
      return ;
#endif /* FX_GLIDE_NAPALM */

    if (doColorP) {
      _grSwizzleColor(&color);

#ifdef FX_GLIDE_NAPALM
      /* OR in Alpha component */
      if (gc->grPixelSize == 4)
        color |= alpha << 24 ; 
#endif
    }

    /* HACK! FIXME! This routine (or any of the ones it calls) is currently not updating
     * the register shadows properly.  This could cause things to screw up in
     * the windowed case, so we make sure that we have enough room in the fifo
     * to write everything out into one buffer.  %%KCD  */
    GR_CHECK_FOR_ROOM(512, 0);
    
#ifdef GLIDE_INIT_HWC

#ifdef FX_GLIDE_NAPALM
#ifndef HAL_CSIM   /* old school CSIM uses Nair instead of Wax */
      /* */
      /*  do a hot WAX job on the buffer(s) */
      /*  to get all of their little hairs off */
      /*  */
      if (gc->sliCount > 1) {     /* This code can only possibly help in Sli */
        /* [dBorca]
         * texturebuffer clears will fail badly! See `grClearBuffer' for needed
         * modifications. As soon as the required changes will be made, this comment
         * will become futile and shall be scheduled for deletion! :)
         */
      
        /* We don't do bikini lines yet */
        /* ie, clip window must be on an even chip boundary */
        /* If we start doing bikini lines, we will need to either send diff't size */
        /* blts to each chip, or send extra single line blts to certain chips */
        /* in addition to a blt for the common area of the fill.  */
        if ((!(gc->state.shadow.clipBottomTop & (((gc->sliCount << gc->sliBandHeight) - 1) << SST_CLIPTOP_SHIFT)) &&
           !(gc->state.shadow.clipBottomTop & (((gc->sliCount << gc->sliBandHeight) - 1) << SST_CLIPBOTTOM_SHIFT))) &&
            _GlideRoot.environment.waxon && ((gc->state.tbufferMask & 0xf) == 0xf)) /* KoolSmoky - come back to this! tbufferMask incorrect! */
        {
          FxU32 clipLeft, clipRight, clipTop, clipBottom ;
          FxU32 mask = 0xFFFFFFFF ;

          clipLeft = gc->state.shadow.clipLeftRight ;
          clipRight = clipLeft & SST_CLIPRIGHT ;
          clipLeft >>= SST_CLIPLEFT_SHIFT ;

          /* All of this wackiness here divides the the height of the blt by the */
          /* bandheight * number of chips blting (since WAX doesn't speak Sli, we have to */
          /* explicitly tell it to do less work). */
          clipBottom = gc->state.shadow.clipBottomTop ;
          clipTop = (clipBottom & SST_CLIPTOP) ; 
          clipTop >>= (gc->sliCount >> 1) ;
          clipBottom >>= (SST_CLIPBOTTOM_SHIFT + (gc->sliCount >> 1)) ;

          if (doColorP) {
            /* Color has already been swizzled to RGB */
            /* now we get to squish it to the proper size */
            if (gc->grPixelSize == 2) {
              color = ((color & 0xf8) >> 3)    |   /* blue */
                      ((color & 0xfc00) >> 5)  |   /* green */
                      ((color & 0xf80000) >> 8) ;  /* red */
            } else {
              /* Apply color write mask for 32bpp mode */
              mask = ((gc->state.shadow.renderMode & SST_RM_ALPHA_WMASK) ? 0xFF000000 : 0) |
                     ((gc->state.shadow.renderMode & SST_RM_RED_WMASK) ? 0x00FF0000 : 0)   |
                     ((gc->state.shadow.renderMode & SST_RM_GREEN_WMASK) ? 0x0000FF00 : 0) |
                     ((gc->state.shadow.renderMode & SST_RM_BLUE_WMASK) ? 0x000000FF : 0) ;
            }

            /* call 2D buffer clear */
            if (gc->textureBuffer.on) {
              _grBufferClear2D(gc->state.shadow.colBufferAddr,
                               clipLeft, clipBottom, clipRight, clipTop, depth,
                               mask, FXFALSE, gc->state.shadow.colBufferStride) ;
            } else {
              _grBufferClear2D(gc->buffers0[gc->windowed ? 0 : gc->curBuffer],
                               clipLeft, clipBottom, clipRight, clipTop, color,
                               mask, gc->colTiled, gc->state.shadow.colBufferStride) ;

              /* If there is an AA buffer, clear that too. */
              if (gc->enableSecondaryBuffer) {
                _grBufferClear2D(gc->buffers1[gc->windowed ? 0 : gc->curBuffer],
                                 clipLeft, clipBottom, clipRight, clipTop, color,
                                 mask, gc->colTiled, gc->state.shadow.colBufferStride) ;
              }
            }
          }

          if (doAuxP) {
            /* How nice. Depth is already in the format we want. */
            /* Load up the slingshot, let one fly. */
            /* THIS COULD BE FASTER: If the last 2D blt we did was to the same */
            /* the clip window, src and dst offsets need not be re-asserted. */
           
            if (gc->grPixelSize == 4) {
              if (doStencil) {
                mask = ((gc->state.shadow.stencilMode & SST_STENCIL_WMASK) << (24 - SST_STENCIL_WMASK_SHIFT)) | 0x00FFFFFF ;
                depth = ((FxU32)stencil << 24) | (depth & 0x00FFFFFF) ;
              }
              /* If the stencil buffer has been cleared, then we have to leave it alone...*/
              else if(gc->stencilCleared || 1) {
                mask = 0x00FFFFFF ;
              } else {  /* Otherwise we can just blow it away... */
                mask = 0xFFFFFFFF ;
              }  
            }
            
            if (gc->textureAuxBuffer.on) {
              _grBufferClear2D(gc->state.shadow.auxBufferAddr,
                               clipLeft, clipBottom, clipRight, clipTop, depth,
                               mask, FXFALSE, gc->state.shadow.auxBufferStride) ;
            } else {
              _grBufferClear2D(gc->buffers0[gc->grColBuf],
                               clipLeft, clipBottom, clipRight, clipTop, depth,
                               mask, gc->auxTiled, gc->state.shadow.auxBufferStride) ;
             
              if (gc->enableSecondaryBuffer)
                _grBufferClear2D(gc->buffers1[gc->grColBuf],
                                 clipLeft, clipBottom, clipRight, clipTop, depth,
                                 mask, gc->auxTiled, gc->state.shadow.auxBufferStride) ;
            }
          } else if (doStencil) {
            mask = ((gc->state.shadow.stencilMode & SST_STENCIL_WMASK) << (24 - SST_STENCIL_WMASK_SHIFT)) ;
            depth = ((FxU32)stencil << 24);

            if (gc->textureAuxBuffer.on) {
              _grBufferClear2D(gc->state.shadow.auxBufferAddr,
                               clipLeft, clipBottom, clipRight, clipTop, depth,
                               mask, FXFALSE, gc->state.shadow.auxBufferStride) ;
            } else {
              _grBufferClear2D(gc->buffers0[gc->grColBuf],
                               clipLeft, clipBottom, clipRight, clipTop, stencil << 24,
                               mask, gc->auxTiled, gc->state.shadow.auxBufferStride) ;
               
              /* ?Is this wrong? */
              if (gc->enableSecondaryBuffer) {
                _grBufferClear2D(gc->buffers1[gc->grColBuf],
                                 clipLeft, clipBottom, clipRight, clipTop, stencil << 24,
                                 mask, gc->auxTiled, gc->state.shadow.auxBufferStride) ;
              }
            }  
          }  
        } else {
          /* */
          /* Not on an even sli bandheight boundary or range, or tbuffer mask isn't set to all buffers... */
          /* so use a triangle fill (yuck!) */
          /* */
          _grTriFill(color, depth, stencil) ;
        }
        /* KoolSmoky - there is nothing else to do so return */
        return;
        
      } else
#endif
#endif /* end of hot wax session  */

      if (!gc->bInfo->sdRAM && !gc->windowed) {
        /* */
        /* Fullscreen Non-SLI SGRAM clear */
        /* */
        REG_GROUP_BEGIN(BROADCAST_ID, zaColor, 2, 0x41) ;
        {
          REG_GROUP_SET(hw, zaColor, zacolor) ;
          REG_GROUP_SET(hw, c1, color) ;
        }
        REG_GROUP_END() ;
          
#ifdef FX_GLIDE_NAPALM
        REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 2, 0x3) ;
        {
          REG_GROUP_SET(hw, renderMode, renderMode) ;
          REG_GROUP_SET(hw, stencilMode, (doStencil ? (stencil | SST_STENCIL_ENABLE | oldstencilMode) : 0)) ;
        }
        REG_GROUP_END() ;
#endif

        REG_GROUP_BEGIN(BROADCAST_ID, fastfillCMD, 3, 0x209);
        {
          /* Execute the FASTFILL command */
          REG_GROUP_SET(hw, fastfillCMD, 1);
            
          /* Restore C1 and ZACOLOR */
          REG_GROUP_SET(hw, zaColor, oldzacolor);
          REG_GROUP_SET(hw, c1, oldc1);
        }
        REG_GROUP_END();

        /* */
        /* Restore whatever else we trashed */
        /* */
#ifdef FX_GLIDE_NAPALM
        REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 2, 0x3);
        {
          REG_GROUP_SET(hw, renderMode, gc->state.shadow.renderMode) ;
          REG_GROUP_SET(hw, stencilMode, oldstencilMode) ;
      }
        REG_GROUP_END() ;
#endif
      }
       else
      {        
        /* */
        /* Windowed / SDRAM clear */
        /* (it's possible this surface could be linear) */
        /* */
        /* If we are in 16-bit mode then we can handle all tbuffer clears */
        /* properly with this code path.  Otherwise if we're in 32-bit mode */
        /* and the tbuffer mask isn't set to all four samples, we have to fall back */
        /* to triangle clears to do the right thing. -- KCD */
        /* */
        /* Actually, that's not entirely true.  The main reason to fall back to */
        /* triangle clears for tbuffer clears is because we can't use WAX to do */
        /* tbuffer clears since the 2D engine doesn't pay attention to the chip */
        /* mask (D'oh).  So, in theory we can still do fastfill based tbuffer */
        /* clears as long as we know we don't need to use WAX.  Yikes.   -- KCD */
        /* */
        if((gc->grPixelSize == 2) ||
           ((gc->state.tbufferMask & 0xf) == 0xf) || /* KoolSmoky - come back to this! tbufferMask incorrect! */
           (!doStencil) ||
           (doStencil && ((gc->state.shadow.stencilMode & SST_STENCIL_WMASK) == SST_STENCIL_WMASK))) {
          FxU32 colorBufMode ;

          colorBufMode = ((fbzMode & ~(SST_ZAWRMASK | SST_ENDEPTHBUFFER)) |
                                       SST_RGBWRMASK |
                                       SST_ENRECTCLIP) ;

          /* Turn off writes to the aux buffer */
          REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 1, 1) ;
          REG_GROUP_SET(hw, fbzMode, colorBufMode) ;
          REG_GROUP_END() ;

          if (doColorP) {
            /* Clear Color Buffer to color */
            REG_GROUP_BEGIN(BROADCAST_ID, c1, 1, 0x1) ;
            REG_GROUP_SET(hw, c1, color) ;
            REG_GROUP_END() ;
            
            /* Execute the FASTFILL command */
            REG_GROUP_BEGIN(BROADCAST_ID, fastfillCMD, 1, 1) ;
            REG_GROUP_SET(hw, fastfillCMD, 1) ;
            REG_GROUP_END() ;
          }
            
    #ifdef FX_GLIDE_NAPALM            
          if (doStencil && 
              ((gc->state.shadow.stencilMode & SST_STENCIL_WMASK) != SST_STENCIL_WMASK))
          {
            /* */
            /* Windowed & SDRAM stencil clears have to be done with hot WAX */
            /* since OpenGL wants to apply the 8bit stencil write mask */
            /* and the color fastfill can only mask entire channels */
            /* */
            FxU32 clipLeft, clipRight, clipTop, clipBottom ;
            FxU32 mask ;
            FxU32 BC2D_color;

            clipLeft = gc->state.shadow.clipLeftRight ;
            clipRight = clipLeft & SST_CLIPRIGHT ;
            clipLeft >>= SST_CLIPLEFT_SHIFT ;

            clipBottom = gc->state.shadow.clipBottomTop ;
            clipTop = clipBottom & SST_CLIPTOP ;
            clipBottom >>= SST_CLIPBOTTOM_SHIFT ;

            if (doAuxP) {
              /* If we get to clear depth at the same time, so much the better */
              mask = ((gc->state.shadow.stencilMode & SST_STENCIL_WMASK) << (24 - SST_STENCIL_WMASK_SHIFT)) | 0x00FFFFFF ;
              BC2D_color = ((FxU32)stencil << 24) | (depth & 0x00FFFFFF) ;
            } else {
              mask = ((gc->state.shadow.stencilMode & SST_STENCIL_WMASK) << (24 - SST_STENCIL_WMASK_SHIFT)) ;
              BC2D_color = ((FxU32)stencil << 24);
            }

            if (gc->textureAuxBuffer.on) {
              _grBufferClear2D(gc->state.shadow.auxBufferAddr,
                               clipLeft, clipBottom, clipRight, clipTop, BC2D_color, mask, FXFALSE, gc->state.shadow.auxBufferStride) ;
            } else {
              _grBufferClear2D(gc->buffers0[gc->grColBuf],
                              clipLeft, clipBottom, clipRight, clipTop, BC2D_color, mask, gc->auxTiled, gc->state.shadow.auxBufferStride) ;
             
              /* ?Is this wrong? */
              if (gc->enableSecondaryBuffer) {
                _grBufferClear2D(gc->buffers1[gc->grColBuf],
                                clipLeft, clipBottom, clipRight, clipTop, BC2D_color, mask, gc->auxTiled, gc->state.shadow.auxBufferStride) ;
              }
            }
          } else
    #endif /* FX_GLIDE_NAPALM */
          if (doAuxP || ((gc->grPixelSize == 4) && doStencil)) 
          {
            FxU32
                red, green, blue, convertedDepth ;
#define GETRED(a) ((a >> 11) & 0x1f)
#define GETGREEN(a) ((a >> 5) & 0x3f)
#define GETBLUE(a)  (a & 0x1f)

#ifdef FX_GLIDE_NAPALM
            if (gc->grPixelSize == 2) {
#endif
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

              red = GETRED(depth) << 3 ;
              green = GETGREEN(depth) << 2 ;
              blue = GETBLUE(depth) << 3 ;

              convertedDepth = ((red << 16) | (green << 8) | blue) ;
            
#ifdef FX_GLIDE_NAPALM
            } else {
              /* All of that hoopla is no help at all in 32bpp mode */
              /* since the depth buffer is 24bpp */
              /* */
              /* enable RGB writes if clearing the depth components. */
              /* enable A writes if we want to clear the stencil buffer too. */
              convertedDepth = depth ;
              
              renderMode &= ~(SST_RM_ALPHA_WMASK | SST_RM_RGBA_WMASK) ; 
              if(doAuxP) {
                renderMode |=  SST_RM_RED_WMASK | SST_RM_GREEN_WMASK | SST_RM_BLUE_WMASK ;
              }
              if(doStencil) {
                renderMode |=  SST_RM_ALPHA_WMASK;
                convertedDepth = (depth & 0xffffff) | (stencil << 24);
              }
               
              REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 2, 0x03) ;
              REG_GROUP_SET(hw, renderMode, renderMode) ;
              REG_GROUP_SET(hw, stencilMode, oldstencilMode & ~(SST_STENCIL_WMASK | SST_STENCIL_ENABLE)); 
              REG_GROUP_END() ;

              restoreRenderStencil = FXTRUE;
            }
#endif
          
            /* Clear Aux Buffer */
            REG_GROUP_BEGIN(BROADCAST_ID, c1, 1, 0x1) ;
            REG_GROUP_SET(hw, c1, convertedDepth) ;
            REG_GROUP_END() ;
          
            /* tbext */
            REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3) ;
            REG_GROUP_SET(hw, colBufferAddr, gc->state.shadow.auxBufferAddr) ;
#ifdef DRI_BUILD
	    REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride : 
			  gc->state.shadow.auxBufferStride );
#else
	    REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.auxBufferStride );
#endif
            REG_GROUP_END() ;
                
#ifdef FX_GLIDE_NAPALM
            if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
              if (gc->enableSecondaryBuffer) {
                REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
                REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->grColBuf] | SST_BUFFER_BASE_SELECT);
                REG_GROUP_END();
              }
            }
#endif
          
            REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 2, 0x21) ;
            {
                /* Write the depth buffer as if it were a color buffer,
                * but w/ actual color buffer features
                * (dithering/chroma/stipple) cleared so that the
                * converted depth value does not get dorked along the
                * way down the eerie pathways of banshee.
                */
                REG_GROUP_SET(hw, fbzMode, colorBufMode & ~(SST_ENCHROMAKEY |
                                                            SST_ENSTIPPLE |
                                                            SST_ENDITHER)) ;

                /* Execute the FASTFILL command */
                REG_GROUP_SET(hw, fastfillCMD, 1) ;
            }
            REG_GROUP_END() ;
          } /* end of doAuxP */

          /* Restore trashed data */
          REG_GROUP_BEGIN(BROADCAST_ID, c1, 1, 0x1);
          REG_GROUP_SET(hw, c1, oldc1);
          REG_GROUP_END();

          /* tbext */
          if (gc->textureBuffer.on) {
            REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3) ;
            REG_GROUP_SET(hw, colBufferAddr, gc->textureBuffer.addr) ;
            REG_GROUP_SET(hw, colBufferStride, gc->textureBuffer.stride) ;
            REG_GROUP_END() ;
          } else {
            REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3) ;
            REG_GROUP_SET(hw, colBufferAddr, gc->buffers0[gc->windowed ? 0 : gc->curBuffer]) ;
#ifdef DRI_BUILD
	    REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride : 
			  gc->state.shadow.colBufferStride );
#else
	    REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
            REG_GROUP_END() ;

#ifdef FX_GLIDE_NAPALM
            if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
              if (gc->enableSecondaryBuffer) {
                REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
                REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->curBuffer] | SST_BUFFER_BASE_SELECT);
                REG_GROUP_END();
              }
            }
#endif
          } /* endif gc->texRender.on */

#ifdef FX_GLIDE_NAPALM
          /* If we did a 32bpp depth (only) clear, restore the color write masks if we need to. */
          if (restoreRenderStencil) {
            REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 2, 0x03) ;
            REG_GROUP_SET(hw, renderMode, gc->state.shadow.renderMode) ;
            REG_GROUP_SET(hw, stencilMode, oldstencilMode) ;
            REG_GROUP_END();      
          }
#endif

          REG_GROUP_BEGIN(BROADCAST_ID, fbzMode, 1, 1) ;
          REG_GROUP_SET(hw, fbzMode, fbzMode) ;
          REG_GROUP_END() ;
        } else {
          /* Crap.  TBuffering is enabled and we can't individually 
           * talk to a single chip with WAX (which we may have to use up above)
           * so we have to use the triangle engine to do TBuffer clears.  UGH! */
          _grTriFill(color, depth, stencil) ;            
        }
        
      } /* end of windowed or sdram clear */
#else /* !GLIDE_INIT_HWC */

      /* */
      /* DOS Glide */
      /* */
      REG_GROUP_BEGIN(BROADCAST_ID, zaColor, 2, 0x41);
      {
        REG_GROUP_SET(hw, zaColor, zacolor);
        REG_GROUP_SET(hw, c1, color);
      }
      REG_GROUP_END();

#ifdef FX_GLIDE_NAPALM
      if (doStencil) {
        REG_GROUP_BEGIN(BROADCAST_ID, stencilMode, 1, 0x1);
        {
          REG_GROUP_SET(hw, stencilMode, (oldstencilMode & 0xffffff00) | stencil);
        }
        REG_GROUP_END();
      }
#endif
      
      REG_GROUP_BEGIN(BROADCAST_ID, fastfillCMD, 3, 0x209);
      {
        /* Execute the FASTFILL command */
        REG_GROUP_SET(hw, fastfillCMD, 1);
        
        /* Restore C1 and ZACOLOR */
        REG_GROUP_SET(hw, zaColor, oldzacolor);
        REG_GROUP_SET(hw, c1, oldc1);
      }
      REG_GROUP_END();

#ifdef FX_GLIDE_NAPALM
      if (doStencil) {
        REG_GROUP_BEGIN(BROADCAST_ID, stencilMode, 1, 0x1);
        {
          REG_GROUP_SET(hw, stencilMode, oldstencilMode);
        }
        REG_GROUP_END();
      }
#endif
#endif /* !GLIDE_INIT_HWC */
    }
#undef FN_NAME
} /* grBufferClearExt */


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

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define KBHIT(key) ((GetAsyncKeyState(key) & 0x8001) == 0x8001)
#elif (GLIDE_PLATFORM & GLIDE_OS_UNIX)
#define KBHIT(key) (0)
#elif (GLIDE_PLATFORM & GLIDE_OS_DOS32)
#define KBHIT(key) (0)
#else
#define KBHIT(key) (0)
#endif

#ifndef DRI_BUILD
/*---------------------------------------------------------------------------
** grBufferSwap
**
** NOTE:        This routine should be COMPLETELY device-independant,
**              but it isn't right now because we need to move all the
**              code for the splash screen into the init library.
*/

GR_ENTRY(grBufferSwap, void, (FxU32 swapInterval))
{
#define FN_NAME "grBufferSwap"
  static FxU32 aaEnabled = 1;
  
  GR_BEGIN_NOFIFOCHECK(FN_NAME,86);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",swapInterval);

#ifdef FX_GLIDE_NAPALM
#if !(GLIDE_PLATFORM & GLIDE_OS_UNIX) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
  /* Window hacky stuff */
  if (gc->windowed)
  {
    extern void _grFlipWindowSurface();
    grFinish();
    _grFlipWindowSurface();
    gc->stats.bufferSwaps++;
    return;
  }
#endif

  // test
  if (aaEnabled) {
    /* aaEnabled doesn't mean we have AA.
     * It only means the user didn't forbid any kind of AA
     * at run-time with _GlideRoot.environment.aaToggleKey
     */
    static FxU32 taaFrame = 1;
    if (KBHIT(_GlideRoot.environment.taaToggleKey)) {
      _GlideRoot.environment.taaEnabled ^= 1;
      if (!_GlideRoot.environment.taaEnabled) {
        int jOffset = 0; /* reset the offsets to default */
        if (gc->grPixelSample > 1) {
          /* reset the offsets to whatever our AA defaults */
          jOffset = gc->sampleOffsetIndex;
        }
        _grAAOffsetValue(_GlideRoot.environment.aaXOffset[jOffset],
                         _GlideRoot.environment.aaYOffset[jOffset],
                         0, gc->chipCount - 1, FXTRUE, gc->enableSecondaryBuffer);
      }
    }
    if (_GlideRoot.environment.taaEnabled) {
      _grAAOffsetValue(_GlideRoot.environment.aaXOffset[13 + taaFrame],
                       _GlideRoot.environment.aaYOffset[13 + taaFrame],
                       0, gc->chipCount - 1, FXTRUE, gc->enableSecondaryBuffer);
      taaFrame ^= 1;
    }
  }
  
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
  }
#endif

#ifdef GLIDE_PLUG
  /* optionally display the 3Dfx powerfield logo overlay */
  if (_GlideRoot.environment.shamelessPlug) _grShamelessPlug();
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  if (_GlideRoot.environment.aaToggleKey) {
    if(gc->grPixelSample > 1) {
      if (KBHIT(_GlideRoot.environment.aaToggleKey)) {
        //static FxU32 aaEnabled = 1;
        
        aaEnabled ^= 1;
        if(aaEnabled) {
          grEnable(GR_AA_MULTI_SAMPLE);
        } else {
          grDisable(GR_AA_MULTI_SAMPLE);
        }
      }  
    }  
  }  
  if(_GlideRoot.environment.aaScreenshotKey) {
    if (KBHIT(_GlideRoot.environment.aaScreenshotKey)) {
      grFinish();
      hwcAAScreenShot(gc->bInfo, gc->curBuffer, _GlideRoot.environment.ditherHwcAA);
    }
  }  
#elif (GLIDE_PLATFORM & GLIDE_OS_MACOS)
  if (_GlideRoot.environment.aaToggleKey || _GlideRoot.environment.aaScreenshotKey) {
    static KeyMapByteArray key_map[2];
    static int current_key_map = 0;
    
	current_key_map = (current_key_map + 1) & 1;
	GetKeys((FxU32*) key_map[current_key_map]);
    
    if (_GlideRoot.environment.aaToggleKey) {
      if(gc->grPixelSample > 1) {
        FxU32 key = _GlideRoot.environment.aaToggleKey;
        key = (key & keyCodeMask) >> 8;
        
        /* If current state is down and previous state is not down, then toggle. */
        if(  (key_map[current_key_map][key >> 3] & (1L << (key & 0x7))) &&
            !(key_map[current_key_map^1][key >> 3] & (1L << (key & 0x7)))) {
          static FxU32 aaEnabled = 1;
          
          aaEnabled ^= 1;
          if(aaEnabled) {
            grEnable(GR_AA_MULTI_SAMPLE);
          } else {
            grDisable(GR_AA_MULTI_SAMPLE);
          }
        }  
      }  
    }  
    if(_GlideRoot.environment.aaScreenshotKey) {
      FxU32 key = _GlideRoot.environment.aaScreenshotKey;
      key = (key & keyCodeMask) >> 8;
       /* If current state is down and previous state is not down, then toggle. */
      if(  (key_map[current_key_map][key >> 3] & (1L << (key & 0x7))) &&
          !(key_map[current_key_map^1][key >> 3] & (1L << (key & 0x7)))) {
        grFinish();
        hwcAAScreenShot(gc->bInfo, gc->curBuffer);
      }
    }  
  }
#endif

#if A0_AA_SWAP_WORKAROUND
  /* EFFAGE... AA seems to require swap intervals > 0 */
  /* However, if the user decides to use the environment */
  /* variable, then well let buffer swaps happen whenever. */
  /* Check with Scott to see if this is the desired behaviour. */
  if(gc->grPixelSample > 1 && swapInterval == 0)
    swapInterval = 1;
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
  
#ifndef HAL_CSIM
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
    FxU32 newBufferSwaps =
      (FxU32) gc->cmdTransportInfo.fifoPtr -
      (FxU32) gc->cmdTransportInfo.fifoStart; 

    for ( i = 0; i < MAX_BUFF_PENDING && j == -1; i++) {
      if (gc->bufferSwaps[i] == 0xffffffff) {
        gc->bufferSwaps[i] = newBufferSwaps;
          
        j = i;
      }
    }
    GR_ASSERT(j != -1);

    gc->swapsPending++;
  }
#endif

  REG_GROUP_BEGIN(BROADCAST_ID, leftOverlayBuf, 1, 0x1);
  REG_GROUP_SET(hw, leftOverlayBuf, gc->buffers0[gc->frontBuffer]);
  REG_GROUP_END();

  if (gc->enableSecondaryBuffer)
  {
    REG_GROUP_BEGIN(BROADCAST_ID, leftDesktopBuf, 1, 0x1);
    REG_GROUP_SET(hw, leftDesktopBuf, gc->buffers1[gc->frontBuffer]);
    REG_GROUP_END();

    swapInterval |= SST_SWAP_DESKTOP_EN;
  }
  
  /* Just 0x1 for mask is OK here since we're writing one register */
  REG_GROUP_BEGIN(BROADCAST_ID, swapbufferCMD, 1, 0x1); 
  REG_GROUP_SET(hw, swapbufferCMD, swapInterval);
  REG_GROUP_END();
  
#else
#if USE_PACKET_FIFO
  REG_GROUP_BEGIN(BROADCAST_ID, swapbufferCMD, 1, 0x1); 
  REG_GROUP_SET(hw, swapbufferCMD, swapInterval);
  REG_GROUP_END();
#else
  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET_DIRECT(BROADCAST_ID, hw, swapbufferCMD, 0x1);
  GR_CHECK_SIZE();
#endif
#endif

#if defined( TACO_MEMORY_FIFO_HACK )
  if(_GlideRoot.environment.memFIFOHack)
  _FifoFlush();
#endif

  REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
  REG_GROUP_SET(hw, colBufferAddr, gc->buffers0[gc->curBuffer]);
  REG_GROUP_END();
#ifdef FX_GLIDE_NAPALM
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    if (gc->enableSecondaryBuffer) {
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
      REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->curBuffer] | SST_BUFFER_BASE_SELECT);
      REG_GROUP_END();
    }
  }
#endif

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

#ifdef FX_GLIDE_NAPALM
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    _grChipMask( gc->chipmask );
  }
#endif

  gc->stats.bufferSwaps++;
  
  /* Hey, guess what: If we don't put this freaky NOP/Flush here
   * things turn up missing (the initial help screen in test25,
   * par example) on voodoo5 agp cards.
   */
#ifdef FX_GLIDE_NAPALM
  REG_GROUP_BEGIN(BROADCAST_ID, nopCMD, 1, 0x1); 
  REG_GROUP_SET(hw, nopCMD, 0) ;
  REG_GROUP_END();
#endif

  P6FENCE ;

  /* Bump & Grind if called for */
  if (!gc->cmdTransportInfo.autoBump)
    GR_BUMP_N_GRIND;

  GR_END();
#undef FN_NAME  
} /* grBufferSwap */
#else	/* defined(DRI_BUILD) */
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

#ifdef FX_GLIDE_NAPALM
  // test
  if (1/*aaEnabled*/) {
    /* aaEnabled doesn't mean we have AA.
     * It only means the user didn't forbid any kind of AA
     * at run-time with _GlideRoot.environment.aaToggleKey
     */
    static FxU32 taaFrame = 1;
    if (KBHIT(_GlideRoot.environment.taaToggleKey)) {
      _GlideRoot.environment.taaEnabled ^= 1;
      if (!_GlideRoot.environment.taaEnabled) {
        int jOffset = 0; /* reset the offsets to default */
        if (gc->grPixelSample > 1) {
          /* reset the offsets to whatever our AA defaults */
          jOffset = gc->sampleOffsetIndex;
        }
        _grAAOffsetValue(_GlideRoot.environment.aaXOffset[jOffset],
                         _GlideRoot.environment.aaYOffset[jOffset],
                         0, gc->chipCount - 1, FXTRUE, gc->enableSecondaryBuffer);
      }
    }
    if (_GlideRoot.environment.taaEnabled) {
      _grAAOffsetValue(_GlideRoot.environment.aaXOffset[13 + taaFrame],
                       _GlideRoot.environment.aaYOffset[13 + taaFrame],
                       0, gc->chipCount - 1, FXTRUE, gc->enableSecondaryBuffer);
      taaFrame ^= 1;
    }
  }

  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
  }
#endif
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
  while(_grBufferNumPending() > 3)
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
    REG_GROUP_SET_WAX(hw, srcBaseAddr, gc->buffers0[1]|BIT(31));
    REG_GROUP_SET_WAX(hw, srcFormat, gc->strideInTiles | 
		      ((driInfo.cpp+1)<<16));
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
    REG_GROUP_SET_WAX(hw, srcBaseAddr, gc->buffers0[0]);
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

#ifdef FX_GLIDE_NAPALM
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    _grChipMask( gc->chipmask );
  }
#endif

  gc->stats.bufferSwaps++;
  
  GR_END();
#undef FN_NAME  
} /* grBufferSwap */

void grBufferSwap(FxU32 i) {
  grDRIBufferSwap(i);
}

#endif	/* defined(DRI_BUILD) */

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
#if CHECK_SLAVE_SWAPCMD
  FxU32 chip ;
#endif

  GR_DCL_GC;

  /* If we're not using hole counting then we need to make sure that
   * any queued commands are actually flushed before checking the fifo
   * ptr's location.
   */
  if (!gc->cmdTransportInfo.autoBump)
    GR_BUMP_N_GRIND;

  /* HACK HACK HACK */
  do {
    readPtr0 = GET(gc->cRegs->cmdFifo0.readPtrL) ;
    dummy = _grSstStatus();
    readPtr1 = GET(gc->cRegs->cmdFifo0.readPtrL) ;

  /* KCD: I believe that swap commands are synchronized across chips,
     so it should be good enough to just track the master's read pointer.
  */
#if CHECK_SLAVE_SWAPCMD
    if(_GlideRoot.environment.checkSlaveSwapCMD > 0)
    if (gc->chipCount)
      for (chip = 0 ;
           chip < gc->chipCount - 1 ;
           chip++)
        readPtr1 = ((readPtr1 < GET(gc->slaveCRegs[chip]->cmdFifo0.readPtrL))
                    ? readPtr1 : GET(gc->slaveCRegs[chip]->cmdFifo0.readPtrL)) ;
#endif
  } while (readPtr0 != readPtr1);

  readPtr = readPtr1 - gc->cmdTransportInfo.fifoOffset;

  if (readPtr == gc->lastSwapCheck) {
    do {
      depth0 = GET(gc->cRegs->cmdFifo0.depth);
      depth1 = GET(gc->cRegs->cmdFifo0.depth);
      
#if CHECK_SLAVE_SWAPCMD
      if(_GlideRoot.environment.checkSlaveSwapCMD > 1)
      if (gc->chipCount)
        for (chip = 0 ;
             chip < gc->chipCount - 1 ;
             chip++)
          depth1 = ((depth1 < GET(gc->slaveCRegs[chip]->cmdFifo0.depth))
                    ? depth1 : GET(gc->slaveCRegs[chip]->cmdFifo0.depth)) ;
#endif
    } while (depth0 != depth1);

    if (depth1 == 0) {
      for(i = MAX_BUFF_PENDING; i >= 0; --i)
        gc->bufferSwaps[i] = 0xffffffff;
      gc->swapsPending = 0;
      goto NPDONE;
    }
  } else

  /*
  **  There are two cases here:  One where the read pointer has wrapped around
  **  behind us, and one where it's ahead of us.
  */
  if (readPtr < gc->lastSwapCheck) { /* We've wrapped */
    for(i = MAX_BUFF_PENDING; i >= 0; --i) {
      /* If it's between the last check and the end of the FIFO or between the
       beginning of the FIFO and the current Read pointer, then it's gone
       */
      if(gc->bufferSwaps[i] != 0xffffffff) {
        if((gc->bufferSwaps[i] >= gc->lastSwapCheck) || (gc->bufferSwaps[i] <= readPtr)) {
          --gc->swapsPending;
          gc->bufferSwaps[i] = 0xffffffff; /* Free swap slot */
        }
      }
    }
  } else {                      /* It's behind us */
    for(i = MAX_BUFF_PENDING; i >= 0; --i) {
      if(gc->bufferSwaps[i] != 0xffffffff) {
        if((gc->bufferSwaps[i] >= gc->lastSwapCheck) && (gc->bufferSwaps[i] <= readPtr)) {
          --gc->swapsPending;
          gc->bufferSwaps[i] = 0xffffffff; /* Free swap slot */
        }
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
  FxU32 tmp;

  GDBG_INFO(85, "%s:  minx = %d, maxx = %d, miny = %d, maxy = %d\n",
            FN_NAME, minx, maxx, miny, maxy);

  GR_CHECK_COMPATABILITY(FN_NAME,
                         ((minx > LONG_MAX) || (miny > LONG_MAX)),
                         "Negative min clip coordinate");
  
  /* Sort the Maxes and Mins These are the same no matter what */
  if (minx > maxx) {
    tmp = maxx;
    minx = maxx;
    maxx = tmp;
  }
  if (miny > maxy) {
    tmp = maxy;
    miny = maxy;
    maxy = tmp;
  }

  if(_GlideRoot.environment.aaClip == FXTRUE) {
    if((gc->grPixelSample > 1) && (_GlideRoot.windowsInit[_GlideRoot.current_sst] == 1)) {
      if(minx == 0) minx = 1;
      if(miny == 0) miny = 1;
    }
  }
  
  if (gc->windowed) {
    if (gc->state.wClipping.colBufferSet && gc->state.wClipping.auxBufferSet) {
      maxx = MIN(maxx,
                 MIN(gc->state.wClipping.colClip.width,
                     gc->state.wClipping.auxClip.width));
      maxy = MIN(maxy,
                 MIN(gc->state.wClipping.colClip.height,
                     gc->state.wClipping.auxClip.height));

      gc->state.wClipping.winClip.width = maxx; 
      gc->state.wClipping.winClip.height = maxy;

    }
    /* Else just use it */
  } else {
    
    /* don't allow bogus clip coords!!! */

    /* [dBorca] Hack alert:
     * if we want clipping into texturebuffers, we are not allowed to
     * clamp to screen size (which might be smaller than texture size,
     * namely for Napalm -- which can handle up to 2048x2048). Need to
     * revise this...
     */
    if (maxx > gc->state.screen_width) maxx = gc->state.screen_width;
    if (maxy > gc->state.screen_height) maxy = gc->state.screen_height;
  }

  GDBG_INFO(85, 
            "%s: normalized  minx = %d, maxx = %d, miny = %d, maxy = %d\n",
            FN_NAME, minx, maxx, miny, maxy);
  
  *clipLeftRight = (minx << SST_CLIPLEFT_SHIFT) | (maxx << SST_CLIPRIGHT_SHIFT); 
  *clipBottomTop = (miny << SST_CLIPBOTTOM_SHIFT) | (maxy << SST_CLIPTOP_SHIFT);

  if (minx & 0x00000001) minx--;
  if (miny & 0x00000001) miny--;
  if (maxx & 0x00000001) maxx++;
  if (maxy & 0x00000001) maxy++;

#ifdef FX_GLIDE_NAPALM
  gc->state.shadow.clipLeftRight1 = (minx << SST_CLIPLEFT_SHIFT) |
    (maxx << SST_CLIPRIGHT_SHIFT);
  gc->state.shadow.clipBottomTop1 = (miny << SST_CLIPTOP_SHIFT) |
    (maxy << SST_CLIPTOP_SHIFT);
#endif
  
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

  /* tbext */
  if ( gc->textureBuffer.on && !gc->textureBuffer.prevState.valid ) {
    gc->textureBuffer.prevState.cwMinx = (float) minx;
    gc->textureBuffer.prevState.cwMaxx = (float) maxx;
    gc->textureBuffer.prevState.cwMiny = (float) miny;
    gc->textureBuffer.prevState.cwMaxy = (float) maxy;
    gc->textureBuffer.prevState.clipLeftRight = clipLeftRight;
    gc->textureBuffer.prevState.clipBottomTop = clipBottomTop;
    gc->textureBuffer.prevState.valid = FXTRUE;
  } /* endif gc->textureBuffer.on */

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
  gc->state.tcc_requires_it_rgb[GR_TMU0] = FXFALSE;
  gc->state.tcc_requires_it_rgb[GR_TMU1] = FXFALSE;

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

  if ((gc->state.shadow.fbzMode & SST_WBUFFER) && (gc->grPixelSize == 4))
    level = (level << 8 | 0xff);

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
    if (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE) {
      fbzMode |= SST_DEPTH_FLOAT_SEL;
      fbzMode &= ~(SST_WBUFFER);
    }
    break;

  case GR_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS:
    fbzMode |= SST_ENDEPTHBUFFER | SST_ZCOMPARE_TO_ZACOLOR;
    break;

  case GR_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS:
    fbzMode |= SST_ENDEPTHBUFFER | SST_WBUFFER | SST_ZCOMPARE_TO_ZACOLOR;
    if (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE) {
      fbzMode |= SST_DEPTH_FLOAT_SEL;
      fbzMode &= ~(SST_WBUFFER);
    }
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
   * Check to see if user selected dither subtraction
   *    Default selection: disable dither sub
   * (see also _GlideInitEnvironment() in gpci.c )
   ******************************************************************/
  fbzMode &= ~(SST_ENDITHER | SST_DITHER2x2 | SST_ENDITHERSUBTRACT);
  
  switch (mode) {
  case GR_DITHER_DISABLE:      
    break;            /* Assure dither subtraction stays disabled */
                      /* when dithering is diabled                */
  case GR_DITHER_2x2: /* Never a question of which dither matrix is used */
  case GR_DITHER_4x4: /* To ditherSub or not to ditherSub,that is the question */
    fbzMode |= (SST_ENDITHER | SST_DITHER2x2 | SST_ENDITHERSUBTRACT);
    
    if (_GlideRoot.environment.disableDitherSub == FXTRUE) 
        fbzMode &= ~(SST_ENDITHERSUBTRACT);  
    break;
  }
  
  /* disable dithering when in 32bpp */
  if (gc->bInfo->h3pixelSize == 4) {
    fbzMode &= ~(SST_ENDITHER | SST_DITHER2x2 | SST_ENDITHERSUBTRACT);
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

#ifdef FX_GLIDE_NAPALM
  /*
  ** piggyback the alpha ops in grFogMode
  */ 
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    FxU32 fogmode = gc->state.shadow.fogMode;
    fogmode &= ~( SST_RGB_BLEND_REVERSE |
                  SST_RGB_BLEND_SUB |
                  SST_A_BLEND_REVERSE |
                  SST_A_BLEND_SUB );
    switch (gc->state.stateArgs.grAlphaBlendFunctionArgs.rgb_op) {
    case GR_BLEND_OP_SUB:
      fogmode |= SST_RGB_BLEND_SUB;
      break;
    case GR_BLEND_OP_REVSUB:
      fogmode |= SST_RGB_BLEND_REVERSE | SST_RGB_BLEND_SUB;
      break;
    case GR_BLEND_OP_ADD:
    default:
      break;
    }

    switch (gc->state.stateArgs.grAlphaBlendFunctionArgs.alpha_op) {
    case GR_BLEND_OP_SUB:
      fogmode |= SST_A_BLEND_SUB;
      break;
    case GR_BLEND_OP_REVSUB:
      fogmode |= SST_A_BLEND_REVERSE | SST_A_BLEND_SUB;
      break;
    case GR_BLEND_OP_ADD:
    default:
      break;
    }

#if 0
    /* Now do this in validate state code, since we can't
     * just store one meaningful shadow fogMode value anyway. */
    /*
     * Select dither matrix for AA modes
     * XXX Memo to Myself XXX
     * complete the ugly 4 sample code
     * this may mean re-writing the fogMode state management
     */
    switch (gc->grPixelSample)
    {
      case 2:
      case 4:
        fogmode &= ~(SST_DITHER_ROTATE | SST_DITHER_ROTATE_BLEND) ;
        fogmode |= (2 << SST_DITHER_ROTATE_AA_SHIFT)
                | (2 << SST_DITHER_ROTATE_BLEND_AA_SHIFT) ;
        break ;
      default:
        fogmode &= ~(SST_DITHER_ROTATE | SST_DITHER_ROTATE_BLEND |
                     SST_DITHER_ROTATE_AA | SST_DITHER_ROTATE_BLEND_AA) ;
    }
#endif
    gc->state.shadow.fogMode = fogmode;
  }
#endif
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

      /* Force all fullscreen contexts to be closed */
      _GlideRoot.windowsInit[i] = 0;
    }
    
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

#ifdef FX_GLIDE_NAPALM
  /*
  ** flush napalm specific state
  */
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 7, 0x0787);
    REG_GROUP_SET(hw, renderMode, gc->state.shadow.renderMode);
    REG_GROUP_SET(hw, stencilMode, gc->state.shadow.stencilMode);
    REG_GROUP_SET(hw, stencilOp, gc->state.shadow.stencilOp);
    REG_GROUP_SET(hw, fbiStencilFail, gc->state.shadow.fbiStenciltestFail);
    REG_GROUP_SET(hw, clipLeftRight1, gc->state.shadow.clipLeftRight1);
    REG_GROUP_SET(hw, clipBottomTop1, gc->state.shadow.clipBottomTop1);
    REG_GROUP_SET(hw, combineMode, gc->state.shadow.combineMode);
    gc->state.mode2ppc = (gc->state.shadow.combineMode & SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK) ? FXTRUE : FXFALSE;

#if 0 /* remaining states need to be restored. */
    REG_GROUP_SET(hw, sliCtrl, gc->state.fbi_config.sliCtrl);
    REG_GROUP_SET(hw, aaCtrl, gc->state.fbi_config.aaCtrl);
    REG_GROUP_SET(hw, chipMask, gc->state.fbi_config.chipMask);
#endif
    REG_GROUP_END();
  }
#endif

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
  
#ifdef FX_GLIDE_NAPALM
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    _grChipMask( gc->chipmask );
    _grTex2ppc(FXFALSE);
  }
#endif
  
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
    //GrChipID_t tmus;

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

      if(gc->state.per_tmu[tmu].texSubLodDither)
            g3LodBiasPerChip(tmu, gc->state.shadow.tmuState[tmu].tLOD);

      REG_GROUP_BEGIN(chipField, chromaKey, 2, 0x03);
      {
        REG_GROUP_SET(tmuregs, chromaKey, gc->state.shadow.tmuState[tmu].texchromaKey);
        REG_GROUP_SET(tmuregs, chromaRange, gc->state.shadow.tmuState[tmu].texchromaRange);
      }
      REG_GROUP_END();
#ifdef FX_GLIDE_NAPALM
      if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
        REG_GROUP_BEGIN(chipField, combineMode, 1, 0x01);
        {
          REG_GROUP_SET(tmuregs, combineMode, gc->state.shadow.tmuState[tmu].combineMode);
        }
        REG_GROUP_END();
      }
#endif
      
      /*tmus = MultitextureAndTrilinear();
      if(tmus & tmu)  g3LodBiasPerChip(tmu);*/
    }
  }
  
#ifdef FX_GLIDE_NAPALM
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    grTBufferWriteMaskExt(gc->state.tbufferMask);
  }
#endif

  /* NOTE: since glide state includes things like hints and all cached
   * variables like paramIndex we needn't recompute these, BUT: we do
   * need to rebuild stuff that depends on them!!! 
   */
  _grUpdateParamIndex();

  /*
   * AJB HACK CRAP
   * YORIGIN swapping w/ SLI isn't integrated
   * into the state monster... for now, force
   * a call to grSstOrigin
   */
  grSstOrigin((gc->state.shadow.fbzMode & SST_YORIGIN) ?
               GR_ORIGIN_LOWER_LEFT : GR_ORIGIN_UPPER_LEFT) ;

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
#ifndef DRI_BUILD
GR_STATE_ENTRY(grRenderBuffer, void, (GrBuffer_t buffer))
{
#define FN_NAME "_grRenderBuffer"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",buffer);
  /* tbext */
  GR_CHECK_F(myName, ((buffer != GR_BUFFER_TEXTUREBUFFER_EXT )
                     && (buffer != GR_BUFFER_FRONTBUFFER )
                     && (buffer != GR_BUFFER_BACKBUFFER )), "invalid buffer");
  {
    const FxU32 
      oldBuffer = gc->curBuffer;

        /* tbext */
    if ( !(buffer == GR_BUFFER_TEXTUREBUFFER_EXT ) ) {
    gc->curBuffer = ((buffer == GR_BUFFER_FRONTBUFFER)
                     ? gc->frontBuffer
                     : gc->backBuffer);
    if (oldBuffer != gc->curBuffer) {
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
        REG_GROUP_SET(hw, colBufferAddr, gc->buffers0[gc->curBuffer]);
      REG_GROUP_END();
        gc->state.shadow.colBufferAddr = gc->buffers0[gc->curBuffer];
#ifdef FX_GLIDE_NAPALM
        if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
          if (gc->enableSecondaryBuffer) {
            REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
            REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->curBuffer] | SST_BUFFER_BASE_SELECT);
            REG_GROUP_END();
    }
  }
#endif /* FX_GLIDE_NAPALM */
      } /* endif oldBuffer != gc->curBuffer */
    }/* endif ! buffer == GR_BUFFER_TEXTUREBUFFER_EXT */
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
  /* tbext */
  GR_CHECK_F(myName, ((buffer != GR_BUFFER_TEXTUREBUFFER_EXT )
                     && (buffer != GR_BUFFER_FRONTBUFFER )
                     && (buffer != GR_BUFFER_BACKBUFFER )), "invalid buffer");
  {
        /* tbext */
    if ( !(buffer == GR_BUFFER_TEXTUREBUFFER_EXT ) ) {
    gc->curBuffer = ((buffer == GR_BUFFER_FRONTBUFFER)
                     ? gc->frontBuffer
                     : gc->backBuffer);
    REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 2, 0x3); 
    REG_GROUP_SET(hw, colBufferAddr, gc->buffers0[gc->curBuffer]);
    REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride :
		    gc->state.shadow.colBufferStride);
    REG_GROUP_END();
    gc->state.shadow.colBufferAddr = gc->buffers0[gc->curBuffer];
#ifdef FX_GLIDE_NAPALM
      if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
          if (gc->enableSecondaryBuffer) {
            REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 1, 0x1); 
            REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->curBuffer] | SST_BUFFER_BASE_SELECT);
            REG_GROUP_END();
          }
      }
#endif /* FX_GLIDE_NAPALM */
    }/* endif ! buffer == GR_BUFFER_TEXTUREBUFFER_EXT */
  }
  GR_END();
#undef FN_NAME
} /* grRenderBuffer */
#endif /* DRI_BUILD */


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
  if ((gc->state.cc_requires_it_rgb) || 
      (gc->state.tcc_requires_it_rgb[GR_TMU0]) ||
      (gc->state.tcc_requires_it_rgb[GR_TMU1])) {
    paramIndex |= STATE_REQUIRES_IT_DRGB;
  }

  /* See if we need to iterate alpha based on the value of
     ac_requires_it_alpha */ 
  if ((gc->state.ac_requires_it_alpha) || 
      (gc->state.tac_requires_it_alpha[GR_TMU0]) ||
      (gc->state.tac_requires_it_alpha[GR_TMU1])) {
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
        if (gc->state.vData.q0Info.mode == GR_PARAM_DISABLE) {
          paramIndex &= ~STATE_REQUIRES_W_TMU0;          
        }
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

#ifdef FX_GLIDE_NAPALM
/*---------------------------------------------------------------------------
 * _grCCExtfbzColorPath
 *
 * Color combine extension update in fbzColorPath register
 */
GR_DDFUNC(_grCCExtfbzColorPath, void, (GrCCUColor_t     a,
                                       GrCombineMode_t  a_mode,
                                       GrCCUColor_t     b,
                                       GrCombineMode_t  b_mode,
                                       GrCCUColor_t     c,
                                       FxBool           c_invert,
                                       GrCCUColor_t     d,
                                       FxBool           invert))
{
#define FN_NAME "_grCCExtfbzColorPath"
  FxU32 fbzColorPath;
  GR_BEGIN_NOFIFOCHECK("_grCCExtfbzColorPath",85);
  GDBG_INFO_MORE(gc->myLevel, "(%d,%d,%d)\n", c, d, invert);

  /* color combine extension: update fbzColorPath register
  ** cc_mselect (part of c), cc_add_clocal (part of d), cc_add_alocal (part of d)
  ** cc_invert_output (invert) 
  */
  fbzColorPath = gc->state.shadow.fbzColorPath;
  fbzColorPath &= ~(
                    SST_CC_MSELECT |            /* cc_mselect */
                    SST_CC_ADD_CLOCAL |         /* cc_add_clocal */
                    SST_CC_ADD_ALOCAL |         /* cc_add_alocal */
                    SST_CC_INVERT_OUTPUT |      /* cc_invert_output */
                    SST_ENTEXTUREMAP |          /* disable texturing */
                    SST_CC_REVERSE_BLEND |      /* c_invert */
                    SST_CC_ZERO_OTHER |         /* a_mode zero */
                    SST_CC_SUB_CLOCAL           /* b_mode zero */
                    );

  gc->state.cc_requires_texture = 
    ((a == GR_CMBX_TEXTURE_RGB) |
     (a == GR_CMBX_TEXTURE_ALPHA) |
     (b == GR_CMBX_TEXTURE_RGB) |
     (b == GR_CMBX_TEXTURE_ALPHA) |
     (c == GR_CMBX_TEXTURE_RGB) |
     (c == GR_CMBX_TEXTURE_ALPHA) |
     (d == GR_CMBX_TEXTURE_RGB));

  if (a_mode == GR_FUNC_MODE_ZERO)
    fbzColorPath |= SST_CC_ZERO_OTHER;

  if (b_mode != GR_FUNC_MODE_ZERO)
    fbzColorPath |= SST_CC_SUB_CLOCAL;

  if (gc->state.cc_requires_texture || gc->state.ac_requires_texture)
    fbzColorPath |= SST_ENTEXTUREMAP;

  if (!c_invert)
    fbzColorPath |= SST_CC_REVERSE_BLEND;

  switch (c) {
  case GR_CMBX_ZERO:
    fbzColorPath |= SST_CC_MONE;
    break;
  case GR_CMBX_B:
    fbzColorPath |= SST_CC_MCLOCAL;
    break;
  case GR_CMBX_AOTHER:
    fbzColorPath |= SST_CC_MAOTHER;
    break;
  case GR_CMBX_ALOCAL:
    fbzColorPath |= SST_CC_MALOCAL;
    break;
  case GR_CMBX_TEXTURE_ALPHA:
    fbzColorPath |= SST_CC_MATMU;
    break;
  case GR_CMBX_TEXTURE_RGB:
    fbzColorPath |= SST_CC_MRGBTMU;
    break;
  case GR_CMBX_ITRGB:
    fbzColorPath |= SST_CC_MCMSELECT7;
    break;
  case GR_CMBX_ITALPHA:
    fbzColorPath |= SST_CC_MCMSELECT7;
    break;
  case GR_CMBX_CONSTANT_COLOR:
    fbzColorPath |= SST_CC_MCMSELECT7;
    break;
  case GR_CMBX_CONSTANT_ALPHA:
    fbzColorPath |= SST_CC_MCMSELECT7;
    break;          
  } 
  switch (d) {
  case GR_CMBX_ZERO:             /* cc_add_clocal = 0, cc_add_alocal = 0 */
    break;
  case GR_CMBX_ALOCAL:           /* cc_add_clocal = 0, cc_add_alocal = 1 */
    fbzColorPath |= SST_CC_ADD_ALOCAL;
    break;
  case GR_CMBX_B:                /* cc_add_clocal = 1, cc_add_alocal = 0 */
    fbzColorPath |= SST_CC_ADD_CLOCAL;
    break;
  case GR_CMBX_TEXTURE_RGB:      /* cc_add_clocal = 1, cc_add_alocal = 1 */
    fbzColorPath |= SST_CC_ADD_CLOCAL | SST_CC_ADD_ALOCAL;
    break;
  }
  if (invert) {
    fbzColorPath |= SST_CC_INVERT_OUTPUT;
  }
  gc->state.shadow.fbzColorPath = fbzColorPath;
  return;
#undef FN_NAME
}

/*---------------------------------------------------------------------------
 * _grACExtfbzColorPath
 *
 * Alpha combine extension update in fbzColorPath register
 */
GR_DDFUNC(_grACExtfbzColorPath, void, (GrACUColor_t     a,
                                       GrCombineMode_t  a_mode,
                                       GrACUColor_t     b,
                                       GrCombineMode_t  b_mode,
                                       GrACUColor_t     c,
                                       FxBool           c_invert,
                                       GrACUColor_t     d,
                                       FxBool           invert))
{
#define FN_NAME "_grACExtfbzColorPath"
  FxU32 fbzColorPath;
  GR_BEGIN_NOFIFOCHECK("_grACExtfbzColorPath",85);
  GDBG_INFO_MORE(gc->myLevel, "(%d,%d,%d)\n", c, d, invert);

  /* alpla combine extension
  ** cca_mselect (c), cca_add_clocal (part of d), cca_add_alocal (part of d)
  ** cca_invert_output (invert)
  */
  fbzColorPath = gc->state.shadow.fbzColorPath;
  fbzColorPath &= ~(
                    SST_CCA_MSELECT |            /* cca_mselect */
                    SST_CCA_ADD_CLOCAL |         /* cca_add_clocal */
                    SST_CCA_ADD_ALOCAL |         /* cca_add_alocal */
                    SST_CCA_INVERT_OUTPUT |      /* cca_invert_output */
                    SST_CCA_ZERO_OTHER |         /* cca_zero_other */
                    SST_ENTEXTUREMAP |           /* disable texturing */
                    SST_CCA_REVERSE_BLEND
                    );

  gc->state.ac_requires_texture = 
    ((a == GR_CMBX_TEXTURE_ALPHA) |
     (b == GR_CMBX_TEXTURE_ALPHA) |
     (c == GR_CMBX_TEXTURE_ALPHA) |
     (d == GR_CMBX_TEXTURE_ALPHA));

  if (gc->state.cc_requires_texture || gc->state.ac_requires_texture)
    fbzColorPath |= SST_ENTEXTUREMAP;


  switch (a) {
  case GR_CMBX_ITALPHA:
  case GR_CMBX_TEXTURE_ALPHA:
  case GR_CMBX_CONSTANT_ALPHA:
    break;
  default:
    fbzColorPath |= SST_CCA_ZERO_OTHER;
  }

  if (a_mode == GR_FUNC_MODE_ZERO)
    fbzColorPath |= SST_CCA_ZERO_OTHER;

  if (b_mode != GR_FUNC_MODE_ZERO)
    fbzColorPath |= SST_CCA_SUB_CLOCAL;

  if (!c_invert)
    fbzColorPath |= SST_CCA_REVERSE_BLEND;

  switch (c) {
  case GR_CMBX_ZERO:
    fbzColorPath |= SST_CCA_MONE;
    break;
  case GR_CMBX_B:
    fbzColorPath |= SST_CCA_MCLOCAL;
    break;
  case GR_CMBX_AOTHER:
    fbzColorPath |= SST_CCA_MAOTHER;
    break;
  case GR_CMBX_ALOCAL:
    fbzColorPath |= SST_CCA_MALOCAL;
    break;
  case GR_CMBX_TEXTURE_ALPHA:
    fbzColorPath |= SST_CCA_MATMU;
    break;
  case GR_CMBX_ITALPHA:
    fbzColorPath |= SST_CCA_MAITER;
    break;
  case GR_CMBX_CONSTANT_ALPHA:
    fbzColorPath |= SST_CCA_MAC1;
    break;
  }
  switch (d) {
  case GR_CMBX_ZERO:             /* cca_add_clocal = 0, cca_add_alocal = 0 */
    break;
  case GR_CMBX_ALOCAL:           /* cca_add_clocal = 0, cca_add_alocal = 1 */
    fbzColorPath |= SST_CCA_ADD_ALOCAL;
    break;
  case GR_CMBX_B:                /* cca_add_clocal = 1, cca_add_alocal = 0 */
    fbzColorPath |= SST_CCA_ADD_CLOCAL;
    break;
  case GR_CMBX_TEXTURE_ALPHA:    /* cca_add_clocal = 1, cca_add_alocal = 1 */
    fbzColorPath |= SST_CCA_ADD_CLOCAL | SST_CCA_ADD_ALOCAL;
    break;
  }
  if (invert) {
    fbzColorPath |= SST_CCA_INVERT_OUTPUT;
  }
  
  gc->state.shadow.fbzColorPath = fbzColorPath;
  return;
#undef FN_NAME
}

/*---------------------------------------------------------------------------
 * _grCCExtcombineMode
 *
 * Color combine extension update in combineMode register
 */
GR_DDFUNC(_grCCExtcombineMode, void, (GrCCUColor_t     a,
                                      GrCombineMode_t  a_mode,
                                      GrCCUColor_t     b,
                                      GrCombineMode_t  b_mode,
                                      GrCCUColor_t     c,
                                      FxBool           d_invert,
                                      FxU32            shift))
{
#define FN_NAME "_grCCExtcombineMode"
  FxU32 combineMode;
  GR_BEGIN_NOFIFOCHECK("_grCCExtcombineMode",85);
  GDBG_INFO_MORE(gc->myLevel, "(%d,%d,%d,%d,%d,%d)\n", a, a_mode, b,
                 b_mode, c, shift);

  /* color combine extension
  ** cc_otherselect (a), cc_localselect (b), cc_mselect_7 (part of c),
  ** cc_invert_other (a_mode), cc_invert_local (b_mode), cc_outshift (shift)
  ** bit(31)
  */
  
  combineMode = gc->state.shadow.combineMode;
  combineMode &= ~( 
                   SST_CM_CC_OTHERSELECT | 
                   SST_CM_CC_LOCALSELECT |
                   SST_CM_CC_MSELECT_7 | 
                   SST_CM_CC_INVERT_OTHER |
                   SST_CM_CC_INVERT_LOCAL |
                   SST_CM_CC_OUTSHIFT |
                   SST_CM_USE_COMBINE_MODE |
                   SST_CM_CC_INVERT_ADD_LOCAL
                   );

  switch (a) {
  case GR_CMBX_ITRGB:
    combineMode |= SST_CM_CC_OTHERSELECT_IRGB;
    break;
  case GR_CMBX_TEXTURE_RGB:
    combineMode |= SST_CM_CC_OTHERSELECT_TRGB;
    break;
  case GR_CMBX_CONSTANT_COLOR:
    combineMode |= SST_CM_CC_OTHERSELECT_C1_RGB;
    break;
  case GR_CMBX_ITALPHA:
    combineMode |= SST_CM_CC_OTHERSELECT_IA;
    break;
  case GR_CMBX_TEXTURE_ALPHA:
    combineMode |= SST_CM_CC_OTHERSELECT_TA;
    break;
  case GR_CMBX_CONSTANT_ALPHA:
    combineMode |= SST_CM_CC_OTHERSELECT_C1_A;
    break;
  case GR_CMBX_ZERO:
  default:
    combineMode |= SST_CM_CC_OTHERSELECT_ZERO;
    break;
  }
  switch (b) {
  case GR_CMBX_ITRGB:
    combineMode |= SST_CM_CC_LOCALSELECT_IRGB;
    break;
  case GR_CMBX_CONSTANT_COLOR:
    combineMode |= SST_CM_CC_LOCALSELECT_C0_RGB;
    break;
  case GR_CMBX_TEXTURE_RGB:
    combineMode |= SST_CM_CC_LOCALSELECT_TRGB;
    break;
  case GR_CMBX_ITALPHA:
    combineMode |= SST_CM_CC_LOCALSELECT_IA;
    break;
  case GR_CMBX_CONSTANT_ALPHA:
    combineMode |= SST_CM_CC_LOCALSELECT_C0_A;
    break;
  case GR_CMBX_TEXTURE_ALPHA:
    combineMode |= SST_CM_CC_LOCALSELECT_TA;
    break;
  default:
    combineMode |= SST_CM_CC_LOCALSELECT_ZERO;
    break;
  }
  switch (a_mode) {
  case GR_FUNC_MODE_ZERO:
  case GR_FUNC_MODE_X:
    combineMode |= SST_CM_CC_INVERT_OTHER_X;
    break;
  case GR_FUNC_MODE_ONE_MINUS_X:
    combineMode |= SST_CM_CC_INVERT_OTHER_ONE_MINUS_X;
    break;
  case GR_FUNC_MODE_NEGATIVE_X:
    combineMode |= SST_CM_CC_INVERT_OTHER_ZERO_MINUS_X;
    break;
  case GR_FUNC_MODE_X_MINUS_HALF:
    combineMode |= SST_CM_CC_INVERT_OTHER_X_MINUS_HALF;
    break;
  }
  switch (b_mode) {
  case GR_FUNC_MODE_ZERO:
  case GR_FUNC_MODE_X:
    combineMode |= SST_CM_CC_INVERT_LOCAL_X;
    break;
  case GR_FUNC_MODE_ONE_MINUS_X:
    combineMode |= SST_CM_CC_INVERT_LOCAL_ONE_MINUS_X;
    break;
  case GR_FUNC_MODE_NEGATIVE_X:
    combineMode |= SST_CM_CC_INVERT_LOCAL_ZERO_MINUS_X;
    break;
  case GR_FUNC_MODE_X_MINUS_HALF:
    combineMode |= SST_CM_CC_INVERT_LOCAL_X_MINUS_HALF;
    break;
  }
  switch (c) {
  case GR_CMBX_ITRGB:
    combineMode |= SST_CM_CC_MSELECT_7_IRGB;
    break;
  case GR_CMBX_CONSTANT_COLOR:
    combineMode |= SST_CM_CC_MSELECT_7_C1_RGB;
    break;
  case GR_CMBX_ITALPHA:
    combineMode |= SST_CM_CC_MSELECT_7_IA;
    break;
  case GR_CMBX_CONSTANT_ALPHA:
    combineMode |= SST_CM_CC_MSELECT_7_C1_A;
    break;                  
  }

  if (d_invert)
    combineMode |= SST_CM_CC_INVERT_ADD_LOCAL;

  switch (shift) {
  case 1:
    combineMode |= SST_CM_CC_OUTSHIFT_2X;
    break;
  case 2:
    combineMode |= SST_CM_CC_OUTSHIFT_4X;
    break;
  default:
    combineMode |= SST_CM_CC_OUTSHIFT_1X;
    break;
  }
  combineMode |= SST_CM_USE_COMBINE_MODE;
  
  gc->state.shadow.combineMode = combineMode;
  return;
#undef FN_NAME
}

/*---------------------------------------------------------------------------
 * _grACExtcombineMode
 *
 * Alpha combine extension update in combineMode register
 */
GR_DDFUNC(_grACExtcombineMode, void, (GrACUColor_t     a,
                                      GrCombineMode_t  a_mode,
                                      GrACUColor_t     b,
                                      GrCombineMode_t  b_mode,
                                      FxBool           d_invert,
                                      FxU32            shift))
{
#define FN_NAME "_grACExtcombineMode"
  FxU32 combineMode;
  GR_BEGIN_NOFIFOCHECK("_grACExtcombineMode",85);
  GDBG_INFO_MORE(gc->myLevel, "(%d,%d,%d,%d,%d)\n", a, a_mode, b,
                 b_mode, shift);

  /* alpha combine extension
  ** cca_otherselect (a), cca_localselect (b), cca_invert_other (a_mode),
  ** cca_invert_local (b_mode), cca_outshift (shift)
  ** bit (31)
  */
  combineMode = gc->state.shadow.combineMode;
  combineMode &= ~( 
                   SST_CM_CCA_OTHERSELECT | 
                   SST_CM_CCA_LOCALSELECT |
                   SST_CM_CCA_INVERT_OTHER |
                   SST_CM_CCA_INVERT_LOCAL |
                   SST_CM_CCA_OUTSHIFT |
                   SST_CM_USE_COMBINE_MODE |
                   SST_CM_CCA_INVERT_ADD_LOCAL
                   );

  switch (a) {
  case GR_CMBX_ITALPHA:
    combineMode |= SST_CM_CCA_OTHERSELECT_IA;
    break;
  case GR_CMBX_TEXTURE_ALPHA:
    combineMode |= SST_CM_CCA_OTHERSELECT_TA;
    break;
  case GR_CMBX_CONSTANT_ALPHA:
    combineMode |= SST_CM_CCA_OTHERSELECT_C1_A;
    break;
  }
  switch (b) {
  case GR_CMBX_ITALPHA:
    combineMode |= SST_CM_CCA_LOCALSELECT_IA;
    break;
  case GR_CMBX_CONSTANT_ALPHA:
    combineMode |= SST_CM_CCA_LOCALSELECT_C0_A;
    break;
  case GR_CMBX_TEXTURE_ALPHA:
    combineMode |= SST_CM_CCA_LOCALSELECT_TA;
    break;
  case GR_CMBX_ZERO:
  default:
    combineMode |= SST_CM_CCA_LOCALSELECT_ZERO;
    break;
  }    
  switch (a_mode) {
  case GR_FUNC_MODE_X:
    combineMode |= SST_CM_CCA_INVERT_OTHER_X;
    break;
  case GR_FUNC_MODE_ONE_MINUS_X:
    combineMode |= SST_CM_CCA_INVERT_OTHER_ONE_MINUS_X;
    break;
  case GR_FUNC_MODE_NEGATIVE_X:
    combineMode |= SST_CM_CCA_INVERT_OTHER_ZERO_MINUS_X;
    break;
  case GR_FUNC_MODE_X_MINUS_HALF:
    combineMode |= SST_CM_CCA_INVERT_OTHER_X_MINUS_HALF;
    break;
  }  
  switch (b_mode) {
  case GR_FUNC_MODE_X:
    combineMode |= SST_CM_CCA_INVERT_LOCAL_X;
    break;
  case GR_FUNC_MODE_ONE_MINUS_X:
    combineMode |= SST_CM_CCA_INVERT_LOCAL_ONE_MINUS_X;
    break;
  case GR_FUNC_MODE_NEGATIVE_X:
    combineMode |= SST_CM_CCA_INVERT_LOCAL_ZERO_MINUS_X;
    break;
  case GR_FUNC_MODE_X_MINUS_HALF:
    combineMode |= SST_CM_CCA_INVERT_LOCAL_X_MINUS_HALF;
    break;
  }

  if (d_invert)
    combineMode |= SST_CM_CCA_INVERT_ADD_LOCAL;

  switch (shift) {
  case 1:
    combineMode |= SST_CM_CCA_OUTSHIFT_2X;
    break;
  case 2:
    combineMode |= SST_CM_CCA_OUTSHIFT_4X;
    break;
  default:
    combineMode |= SST_CM_CCA_OUTSHIFT_1X;
    break;
  }
  
  combineMode |= SST_CM_USE_COMBINE_MODE;

  gc->state.shadow.combineMode = combineMode;
  return;
#undef FN_NAME
}

/*-------------------------------------------------------------------
  Function: grTBufferWriteMaskExt
  Date: 21-July-99
  Implementor(s): atai
  Description:
  Whatever
  Arguments:
  Whatever
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grTBufferWriteMaskExt, void , (FxU32 tmask) )
{
#define FN_NAME "grTBufferWriteMaskExt"

#if 0 /* KoolSmoky */
  FxU32 tBufferSampleOffsetIndex;
#endif
  FxU32 chipIndex, chipEnList, chipaamode[4];
  GR_BEGIN_NOFIFOCHECK("grTBufferWriteMaskExt",85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", tmask);

  /* the current mask will be stored in GrState */

  /* check if we have atleast 2 tbuffers */
  if(gc->grPixelSample <= 1) return;

#if 0 /* KoolSmoky */
  /* In a one-chip config, chip 0 does samples 0 and 1 */
  /* In a two-chip config, chip 0 does samples 0 and 1, and chip 1 does samples 2 and 3 */
  /* In a four-chip config, chips 0 and 2 do samples 0 and 1, and chips 1 and 3 do samples 2 and 3 */
  switch( gc->chipCount ) {
    case 4:
      switch( gc->grPixelSample ) {
      case 8:
        /*no sli, 2 sample per chip */
        chipaamode[0] = (tmask>>0) & 0x03;
        chipaamode[1] = (tmask>>2) & 0x03;
        chipaamode[2] = (tmask>>4) & 0x03;
        chipaamode[3] = (tmask>>6) & 0x03;
        tBufferSampleOffsetIndex = 9;
        break;
        
      case 4:
        /*no sli, 1 sample per chip */
        chipaamode[0] = (tmask>>0) & 0x01;
        chipaamode[1] = (tmask>>2) & 0x01;
        chipaamode[2] = (tmask>>1) & 0x01;
        chipaamode[3] = (tmask>>3) & 0x01;
        tBufferSampleOffsetIndex = 8;
        break;

      case 2:
        if(!_GlideRoot.environment.forceOldAA) {
          /* 2 way SLI, 1 sample per SLI unit */
          chipaamode[0] = 
          chipaamode[2] = (tmask>>0) & 0x01;
          chipaamode[1] = 
          chipaamode[3] = (tmask>>1) & 0x01;
          tBufferSampleOffsetIndex = 7;
        } else {
          /* 4 way SLI, 2 samples per SLI unit */
          chipaamode[0] = 
          chipaamode[1] = 
          chipaamode[2] = 
          chipaamode[3] = (tmask>>0) & 0x03;
          tBufferSampleOffsetIndex = 7;
        }  
        break;		
      }
      break;

    case 2: 
      switch( gc->grPixelSample ) {
      case 4:
        /* no sli, 2 sample per chip */
        chipaamode[0] = (tmask>>0) & 0x03;
        chipaamode[1] = (tmask>>2) & 0x03;
        tBufferSampleOffsetIndex = 3;
        break;

      case 2:
        if(!_GlideRoot.environment.forceOldAA) {
          /* no sli, 1 sample per chip */
          chipaamode[0] = (tmask>>0) & 0x01;
          chipaamode[1] = (tmask>>1) & 0x01;
          tBufferSampleOffsetIndex = 2;
        } else {
          /* 2 samples per SLI pair */
          chipaamode[0] = 
          chipaamode[1] = (tmask>>0) & 0x03;
          tBufferSampleOffsetIndex = 1;
        }
        break;
      }
      break;

    case 1:
      switch( gc->grPixelSample ) {
      case 2:
        /* no sli, 2 sample per chip */
        chipaamode[0] = (tmask>>0) & 0x03;
        tBufferSampleOffsetIndex = 1;
        break;
      }
      break;
    }

  //chipEnList = gc->chipmask;
  chipEnList = 0; /* completely initialize */

  for (chipIndex = 0; chipIndex < gc->chipCount; chipIndex++) {
    switch (chipaamode[chipIndex]) {
    case 0:
      /* disable the chip, don't care about the buffers */
      chipEnList &= ~(1 << chipIndex);
      break;
    case 1:
      /* We are only rendering to the primary buffer (sample 0 or 2) so just set up the 
         primary buffer jitter values and disable the secondary buffer. */
      chipEnList |= (1 << chipIndex);
      _grChipMask(1 << chipIndex);
      _grAAOffsetValue(_GlideRoot.environment.aaXOffset[tBufferSampleOffsetIndex],
                       _GlideRoot.environment.aaYOffset[tBufferSampleOffsetIndex], 
                       chipIndex, chipIndex, FXTRUE, FXFALSE);
      /* setup color/aux buffer */
      gc->state.shadow.colBufferAddr = gc->buffers0[gc->curBuffer];
      gc->state.shadow.auxBufferAddr = gc->buffers0[gc->grColBuf];
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
      {
        REG_GROUP_SET(hw, colBufferAddr, gc->state.shadow.colBufferAddr);
#ifdef DRI_BUILD
	REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? driInfo.stride : 
		      gc->state.shadow.colBufferStride );
#else
	REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
        REG_GROUP_SET(hw, auxBufferAddr, gc->state.shadow.auxBufferAddr);
        REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride); 
      }
      REG_GROUP_END();      
      break;
    case 2:
      /* We are only rendering to the secondary buffer (sample 1 or 3), so point the
         primary color buffer address at the secondary buffer, and use the secondary
         jitter values for this chip as the primary values, and disable the second
         buffer. */
      chipEnList |= (1 << chipIndex);
      _grChipMask(1 << chipIndex);
      _grAAOffsetValue(_GlideRoot.environment.aaXOffset[tBufferSampleOffsetIndex],
                       _GlideRoot.environment.aaYOffset[tBufferSampleOffsetIndex],
                       chipIndex, chipIndex, FXTRUE, FXFALSE);
      /* setup color/aux buffer */
      gc->state.shadow.colBufferAddr = gc->buffers1[gc->curBuffer];
      gc->state.shadow.auxBufferAddr = gc->buffers1[gc->grColBuf];
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
      {
        REG_GROUP_SET(hw, colBufferAddr, gc->state.shadow.colBufferAddr);
#ifdef DRI_BUILD
	REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? driInfo.stride : 
		      gc->state.shadow.colBufferStride );
#else
	REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
        REG_GROUP_SET(hw, auxBufferAddr, gc->state.shadow.auxBufferAddr);
        REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride); 
      }
      REG_GROUP_END();      
      break;
    case 3:
      /* This chip is doing both 0 and 1 or 2 and 3, so enable both sets
         of buffers. */
      chipEnList |= (1 << chipIndex);
      _grChipMask(1 << chipIndex);
      _grAAOffsetValue(_GlideRoot.environment.aaXOffset[tBufferSampleOffsetIndex],
                       _GlideRoot.environment.aaYOffset[tBufferSampleOffsetIndex],
                       chipIndex, chipIndex, FXTRUE, gc->enableSecondaryBuffer);
      /* setup color/aux buffer */
      gc->state.shadow.colBufferAddr = gc->buffers0[gc->curBuffer];
      gc->state.shadow.auxBufferAddr = gc->buffers0[gc->grColBuf];
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
      {
        REG_GROUP_SET(hw, colBufferAddr, gc->state.shadow.colBufferAddr);
#ifdef DRI_BUILD
	REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? driInfo.stride : 
		      gc->state.shadow.colBufferStride );
#else
	REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
        REG_GROUP_SET(hw, auxBufferAddr, gc->state.shadow.auxBufferAddr);
        REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride); 
      }
      REG_GROUP_END();      
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
      {
        REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->curBuffer] | SST_BUFFER_BASE_SELECT);
#ifdef DRI_BUILD
	REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? driInfo.stride : 
		      gc->state.shadow.colBufferStride );
#else
	REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
        REG_GROUP_SET(hw, auxBufferAddr, gc->buffers1[gc->grColBuf] | SST_BUFFER_BASE_SELECT);
        REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride); 
      }
      REG_GROUP_END();
      break;
    default:
      break;
    }
  }
  
#else /* Colourless */
  
  /* Sanity Check. Make sure we are attempting to actually enable something */
  if( (tmask&((1 << gc->grPixelSample)-1)) == 0) {
    return;
  }

  chipEnList = 0;

  /* 2 Samples Per Chip and not SLI */
  if(gc->enableSecondaryBuffer && !gc->sliCount) {
    /*
     Chip 0 = Samples 0 & 1
     Chip 1 = Samples 2 & 3
     Chip 2 = Samples 4 & 5
     Chip 3 = Samples 6 & 7
     */
    chipaamode[0] = ((tmask>>0) & 1) | (((tmask>>1) & 1) << 1);
    chipaamode[1] = ((tmask>>2) & 1) | (((tmask>>3) & 1) << 1);
    chipaamode[2] = ((tmask>>4) & 1) | (((tmask>>5) & 1) << 1);
    chipaamode[3] = ((tmask>>6) & 1) | (((tmask>>7) & 1) << 1);
  }
  /* 1 Sample per chip and not SLI */
  else if(!gc->sliCount) {
    /*
     Chip 0 = Sample 0
     Chip 1 = Sample 1
     Chip 2 = Sample 2
     Chip 3 = Sample 3
     */
    chipaamode[0] = (tmask>>0) & 1;
    chipaamode[1] = (tmask>>1) & 1;
    chipaamode[2] = (tmask>>2) & 1;
    chipaamode[3] = (tmask>>3) & 1;
  }
  /* 1 Sample per chip and SLI */
  else if (gc->enableSecondaryBuffer) {
    /*
     Chip 0 = Sample 0
     Chip 1 = Sample 1
     Chip 2 = Sample 0
     Chip 3 = Sample 1
     */
    chipaamode[2] = chipaamode[0] = (tmask>>0) & 1;
    chipaamode[3] = chipaamode[1] = (tmask>>1) & 1;
  }
  /* 2 Samples per chip and SLI */
  else {
    /*
     Chip 0 = Samples 0 & 1
     Chip 1 = Samples 0 & 1
     Chip 2 = Samples 2 & 3
     Chip 3 = Samples 2 & 3
     */
    chipaamode[0] = ((tmask>>0) & 1) | (((tmask>>1) & 1) << 1);
    chipaamode[1] = ((tmask>>0) & 1) | (((tmask>>1) & 1) << 1);
    chipaamode[2] = ((tmask>>2) & 1) | (((tmask>>3) & 1) << 1);
    chipaamode[3] = ((tmask>>2) & 1) | (((tmask>>3) & 1) << 1);
  }

  for (chipIndex = 0; chipIndex < gc->chipCount; chipIndex++) {

    /* Enable/Disable the chips */
    if (chipaamode[chipIndex]) chipEnList |= (1 << chipIndex);
    else chipEnList &= ~(1 << chipIndex);

    /* Enable/Disable Primary and Secondary Buffers as required */
    if (gc->enableSecondaryBuffer) switch (chipaamode[chipIndex]) {
    case 1:
      /* We are only rendering to the primary buffer so disable the secondary buffer. */
      _grChipMask(1 << chipIndex);

      /* setup color/aux buffer */
      gc->state.shadow.colBufferAddr = gc->buffers0[gc->curBuffer];
      gc->state.shadow.auxBufferAddr = gc->buffers0[gc->grColBuf];
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
      {
        REG_GROUP_SET(hw, colBufferAddr, gc->state.shadow.colBufferAddr);
#ifdef DRI_BUILD
	REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride :
                      gc->state.shadow.colBufferStride );
#else
        REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
        REG_GROUP_SET(hw, auxBufferAddr, gc->state.shadow.auxBufferAddr);
        REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride);
      }
      REG_GROUP_END();
      break;
    case 2:
      /* We are only rendering to the secondary buffer so point the primary color buffer
         address at the secondary buffer, and disable the second buffer. */
      _grChipMask(1 << chipIndex);

      /* setup color/aux buffer */
      gc->state.shadow.colBufferAddr = gc->buffers1[gc->curBuffer];
      gc->state.shadow.auxBufferAddr = gc->buffers1[gc->grColBuf];
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
      {
        REG_GROUP_SET(hw, colBufferAddr, gc->state.shadow.colBufferAddr);
        REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#ifdef DRI_BUILD
	REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride :
                      gc->state.shadow.colBufferStride );
#else
        REG_GROUP_SET(hw, auxBufferAddr, gc->state.shadow.auxBufferAddr);
#endif
        REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride);
      }
      REG_GROUP_END();
      break;
    case 3:
      /* This chip is using both buffer, so enable both. */
      _grChipMask(1 << chipIndex);

      /* setup color/aux buffer */
      gc->state.shadow.colBufferAddr = gc->buffers0[gc->curBuffer];
      gc->state.shadow.auxBufferAddr = gc->buffers0[gc->grColBuf];
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
      {
        REG_GROUP_SET(hw, colBufferAddr, gc->state.shadow.colBufferAddr);
#ifdef DRI_BUILD
	REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride :
                      gc->state.shadow.colBufferStride );
#else
        REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
        REG_GROUP_SET(hw, auxBufferAddr, gc->state.shadow.auxBufferAddr);
        REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride);
      }
      REG_GROUP_END();
      REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
      {
        REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->curBuffer] | SST_BUFFER_BASE_SELECT);
#ifdef DRI_BUILD
	REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride :
                      gc->state.shadow.colBufferStride );
#else
        REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
        REG_GROUP_SET(hw, auxBufferAddr, gc->buffers1[gc->grColBuf] | SST_BUFFER_BASE_SELECT);
        REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride);
      }
      REG_GROUP_END();
      break;
    }
  }
#endif

  _grChipMask( chipEnList );

  gc->state.tbufferMask = tmask;
  gc->chipmask = chipEnList;

#undef FN_NAME
} /* grTBufferWriteMaskExt */

#endif /* FX_GLIDE_NAPALM */

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

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0] = GR_VERTEX_R_OFFSET;
#endif

    /* When using packed color we only add *ONE* item to the data list
     * and this signals the entire color set since it is not possible
     * to specify a single color component in any packet.  
     */
#if !GLIDE_PACKED_RGB
    gc->tsuDataList[curTriSize + 1] = GR_VERTEX_G_OFFSET << 2;
    gc->tsuDataList[curTriSize + 2] = GR_VERTEX_B_OFFSET << 2;

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 1] = GR_VERTEX_G_OFFSET;
    gc->tsuDataListByte[curTriSize + 2] = GR_VERTEX_B_OFFSET;
#endif
    
#endif /* !GLIDE_PACKED_RGB */
#elif defined(GLIDE3)
    gc->cmdTransportInfo.paramMask  |= SST_SETUP_RGB;
    if (gc->state.vData.colorType == GR_FLOAT) {
      gc->tsuDataList[curTriSize + 0] = gc->state.vData.rgbInfo.offset;
      gc->tsuDataList[curTriSize + 1] = gc->state.vData.rgbInfo.offset + GR_COLOR_OFFSET_GREEN;
      gc->tsuDataList[curTriSize + 2] = gc->state.vData.rgbInfo.offset + GR_COLOR_OFFSET_BLUE;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize + 0] = gc->state.vData.rgbInfo.offset >> 2;
      gc->tsuDataListByte[curTriSize + 1] = (gc->state.vData.rgbInfo.offset + GR_COLOR_OFFSET_GREEN) >> 2;
      gc->tsuDataListByte[curTriSize + 2] = (gc->state.vData.rgbInfo.offset + GR_COLOR_OFFSET_BLUE)  >> 2;
#endif
      
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize + 0] = 2;
      gc->tsuDataListScaler[curTriSize + 1] = 2;
      gc->tsuDataListScaler[curTriSize + 2] = 2;
#endif
      gc->state.vData.vSize += 12; /* RGB */
    }
    else {
      gc->tsuDataList[curTriSize + 0] = gc->state.vData.pargbInfo.offset;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize + 0] = gc->state.vData.pargbInfo.offset >> 2;
#endif
      
      packedRGB = FXTRUE;
      gc->state.vData.vSize += 4;
    }
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = gc->state.vData.rgbInfo.offset;
    gc->regDataList[curTriSize + 1].i    = gc->state.vData.rgbInfo.offset + GR_COLOR_OFFSET_GREEN;
    gc->regDataList[curTriSize + 2].i    = gc->state.vData.rgbInfo.offset + GR_COLOR_OFFSET_BLUE;

    gc->regDataList[curTriSize + 0].addr = (float*)&hw->RED;
    gc->regDataList[curTriSize + 1].addr = (float*)&hw->GRN;
    gc->regDataList[curTriSize + 2].addr = (float*)&hw->BLU;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

#if GLIDE_PACKED_RGB
#if GLIDE3
#if !GLIDE3_VERTEX_LAYOUT
    //if (gc->state.vData.colorType == GR_FLOAT) {
    if (gc->state.vData.colorInfo.colorType == GR_FLOAT) { /* KoolSmoky */
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

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0]    = GR_VERTEX_A_OFFSET;
#endif
    
#elif defined(GLIDE3)
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_A;
    if (gc->state.vData.colorType == GR_FLOAT) {
      gc->tsuDataList[curTriSize] = gc->state.vData.aInfo.offset;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize] = gc->state.vData.aInfo.offset >> 2;
#endif
      
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 2;
#endif
      gc->state.vData.vSize += 4;
    }
    else if (!(i & STATE_REQUIRES_IT_DRGB)) {
      packedRGB = FXTRUE;
      gc->tsuDataList[curTriSize] = gc->state.vData.pargbInfo.offset;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize] = gc->state.vData.aInfo.offset >> 2;
#endif
      
      gc->state.vData.vSize += 4;
    }
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = gc->state.vData.aInfo.offset;
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

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0]    = GR_VERTEX_OOZ_OFFSET ;
#endif

#elif defined(GLIDE3)
    /*
    ** use z iterator for w if w buffering and fog coordinate enable
    */
    if ((gc->state.shadow.fbzMode & SST_DEPTH_FLOAT_SEL) && 
        (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)) {
      gc->tsuDataList[curTriSize] = gc->state.vData.qInfo.offset;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize] = gc->state.vData.qInfo.offset >> 2;
#endif
      
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 0;
#endif
    }
    else {
      gc->tsuDataList[curTriSize] = gc->state.vData.zInfo.offset;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize] = gc->state.vData.zInfo.offset >> 2;
#endif
      
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 1;
#endif
    }
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_Z;

    gc->state.vData.vSize += 4;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = gc->state.vData.zInfo.offset;
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

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0]		= GR_VERTEX_OOW_OFFSET;
#endif
    
#elif defined(GLIDE3)
    /*
    ** if fog coordinate is enabled, use w for fog coord
    */
    if ((gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE) && 
        (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)) {
      gc->tsuDataList[curTriSize] = gc->state.vData.fogInfo.offset;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize] = gc->state.vData.fogInfo.offset >> 2;
#endif
      
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 4;
#endif
    }
    else {
      gc->tsuDataList[curTriSize] = gc->state.vData.qInfo.offset;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize] = gc->state.vData.qInfo.offset >> 2;
#endif
      
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 0;
#endif
    }
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_Wfbi;

    gc->state.vData.vSize += 4;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = gc->state.vData.qInfo.offset;
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

#ifdef FAST_C_CLIP
  gc->tsuDataListByte[curTriSize++] = 0;
#endif
    
#endif /* GLIDE_FP_CLAMP_TEX */
  
  /* NOTE: this is the first */
  if (i & STATE_REQUIRES_W_TMU0) {
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_OOW_TMU0_OFFSET << 2;

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0]	 = GR_VERTEX_OOW_TMU0_OFFSET;
#endif
    
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_W0;
#elif defined(GLIDE3)
    /*
    ** if q0 is disabled and fogCoord/Q(in clip coords) is enabled, we need to move oow to w0
    */
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
      gc->tsuDataList[curTriSize] = gc->state.vData.q0Info.offset;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize] = gc->state.vData.q0Info.offset >> 2;
#endif
      
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 4;
#endif
    }
    else {
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
        gc->tsuDataList[curTriSize] = gc->state.vData.qInfo.offset;

#ifdef FAST_C_CLIP
        gc->tsuDataListByte[curTriSize] = gc->state.vData.qInfo.offset >> 2;
#endif

      } else {
        gc->tsuDataList[curTriSize] = gc->state.vData.wInfo.offset;

#ifdef FAST_C_CLIP
        gc->tsuDataListByte[curTriSize] = gc->state.vData.wInfo.offset >> 2;
#endif
        
      }
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 0;
#endif
    }

    gc->cmdTransportInfo.paramMask       |= SST_SETUP_W0;
    gc->state.vData.vSize += 4;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = gc->state.vData.q0Info.offset;
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

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0]      = GR_VERTEX_SOW_TMU0_OFFSET;
    gc->tsuDataListByte[curTriSize + 1]      = GR_VERTEX_TOW_TMU0_OFFSET;
#endif
    
#elif defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = gc->state.vData.st0Info.offset;
    gc->tsuDataList[curTriSize + 1]      = gc->state.vData.st0Info.offset 
      + GR_TEXTURE_OFFSET_T;

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0]      = gc->state.vData.st0Info.offset >> 2;
    gc->tsuDataListByte[curTriSize + 1]      = (gc->state.vData.st0Info.offset+GR_TEXTURE_OFFSET_T) >> 2;
#endif
    
#ifdef GLIDE3_SCALER
    gc->tsuDataListScaler[curTriSize] = 3;
    gc->tsuDataListScaler[curTriSize+1] = 3;
#endif
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_ST0;
    gc->state.vData.vSize += 8;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = gc->state.vData.st0Info.offset;
    gc->regDataList[curTriSize + 1].i    = gc->state.vData.st0Info.offset 
      + GR_TEXTURE_OFFSET_T;
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

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0]      = GR_VERTEX_OOW_TMU1_OFFSET;
#endif
    
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_W1;
#elif defined(GLIDE3)
    /*
    ** if q1 is disabled and fogCoord/Q(in clip coords) is enabled, we need to move oow to w1
    */
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) {
      gc->tsuDataList[curTriSize] = gc->state.vData.q1Info.offset;

#ifdef FAST_C_CLIP
      gc->tsuDataListByte[curTriSize]   = gc->state.vData.q1Info.offset >> 2;
#endif
      
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 4;
#endif
    }
    else {
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
        gc->tsuDataList[curTriSize] = gc->state.vData.qInfo.offset;

#ifdef FAST_C_CLIP
        gc->tsuDataListByte[curTriSize]     = gc->state.vData.qInfo.offset >> 2;
#endif
        
      } else {
        gc->tsuDataList[curTriSize] = gc->state.vData.wInfo.offset;

#ifdef FAST_C_CLIP
        gc->tsuDataListByte[curTriSize]     = gc->state.vData.wInfo.offset >> 2;
#endif
        
      }
#ifdef GLIDE3_SCALER
      gc->tsuDataListScaler[curTriSize] = 0;
#endif
    }
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_W1;
    gc->state.vData.vSize += 4;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = gc->state.vData.q1Info.offset;
    gc->regDataList[curTriSize + 0].addr = (float*)&tmu1->W;
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

    curTriSize += 1;
    params += 1;

  }

  if (i & STATE_REQUIRES_ST_TMU1) {
#if GLIDE_HW_TRI_SETUP && !defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0]      = GR_VERTEX_SOW_TMU1_OFFSET << 2;
    gc->tsuDataList[curTriSize + 1]      = GR_VERTEX_TOW_TMU1_OFFSET << 2;

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0]      = GR_VERTEX_SOW_TMU1_OFFSET;
    gc->tsuDataListByte[curTriSize + 1]      = GR_VERTEX_TOW_TMU1_OFFSET;
#endif
    
    gc->cmdTransportInfo.paramMask      |= SST_SETUP_ST1;
#elif defined(GLIDE3)
    gc->tsuDataList[curTriSize + 0] = gc->state.vData.st1Info.offset;
    gc->tsuDataList[curTriSize + 1] = gc->state.vData.st1Info.offset + GR_TEXTURE_OFFSET_T;

#ifdef FAST_C_CLIP
    gc->tsuDataListByte[curTriSize + 0] = gc->state.vData.st1Info.offset >> 2;
    gc->tsuDataListByte[curTriSize + 1] = (gc->state.vData.st1Info.offset + GR_TEXTURE_OFFSET_T) >> 2;
#endif
    
#ifdef GLIDE3_SCALER
    gc->tsuDataListScaler[curTriSize] = 3;
    gc->tsuDataListScaler[curTriSize+1] = 3;
#endif
    gc->cmdTransportInfo.paramMask       |= SST_SETUP_ST1;
    gc->state.vData.vSize += 8;
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
    gc->regDataList[curTriSize + 0].i    = gc->state.vData.st1Info.offset;
    gc->regDataList[curTriSize + 0].addr = (float*)&tmu1->S;
    gc->regDataList[curTriSize + 1].i    = gc->state.vData.st1Info.offset + GR_TEXTURE_OFFSET_T;
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

#ifdef FAST_C_CLIP
  gc->tsuDataListByte[curTriSize] = 0;
#endif
    
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

#ifdef FAST_C_CLIP
  gc->tsuDataListByte[++curTriSize] = 0;
#endif
    
#endif /* GLIDE_FP_CLAMP_TEX */
  
#ifdef GDBG_INFO_ON
#if GLIDE_HW_TRI_SETUP
  for (i = 0; gc->tsuDataList[i]; i++) {
    GDBG_INFO(282,"    tsuDataList[%d] = %2d \n",
              i, gc->tsuDataList[i] >> 2 );
  }
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
#if 0
  for (i = 0; gc->regDataList[i].i; i++) {
    GDBG_INFO(282,"    dataList[%d] = %2d 0x%x [%s]\n",
              i, gc->regDataList[i].i >> 2, gc->regDataList[i].addr,
              p_str[gc->regDataList[i].i >> 2]);
  }
#endif
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
#ifdef	DRI_BUILD
  gc->gcFuncs.grBufferSwap = (void *) GR_DDNAME(grDRIBufferSwap); 
#else	/* defined(DRI_BUILD) */
  gc->gcFuncs.grBufferSwap = (void *) GR_DDNAME(grBufferSwap); 
#endif	/* defined(DRI_BUILD) */
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
