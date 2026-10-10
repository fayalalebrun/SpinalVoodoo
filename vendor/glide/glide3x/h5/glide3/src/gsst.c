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
**  69   3dfx      1.52.1.3.1.1111/08/00 Drew McMinn     Create initialise read and
**       use useAppGamma flag, to allow us to disable applications changing gamma
**       values.
**  68   3dfx      1.52.1.3.1.1010/13/00 Matt McClure    Exposed
**       grSetNumPendingBuffers for OGL.
**  67   3dfx      1.52.1.3.1.910/11/00 Brent           Forced check in to enforce
**       branching.
**  66   3dfx      1.52.1.3.1.810/09/00 Jonny Cochrane  calculate chipScreenHeight
**       correctly for 4 way SLI
**  65   3dfx      1.52.1.3.1.709/26/00 Matt McClure    Changed opengl name to
**       is_opengl instead of SliBandHeightForce.
**  64   3dfx      1.52.1.3.1.609/25/00 Matt McClure    Added code to allow release
**       of the Exclusive mode context on grSSTWinClose for OpenGL.
**  63   3dfx      1.52.1.3.1.509/15/00 Jonny Cochrane  2x FSAA for 4500
**  62   3dfx      1.52.1.3.1.408/31/00 Andy Hanson     Fix to allow digital SLI up
**       through 1600 by X resolutions.
**  61   3dfx      1.52.1.3.1.308/29/00 Jonny Cochrane  Some 8x FSAA code
**  60   3dfx      1.52.1.3.1.208/01/00 Andy Hanson     Fixed issue with source
**       ignoring setting of GLIDE_SPLASH define.
**       Fixed issue with setting of splash state when it wasn't going to be run.
**  59   3dfx      1.52.1.3.1.107/12/00 troy thornton   Cleaned up grSstWinOpen to
**       always call grSstWinOpenExt
**  58   3dfx      1.52.1.3.1.006/28/00 troy thornton   added code to allow app. to
**       use correct jitter values only if AA is not forced
**  57   3dfx      1.52.1.3    06/20/00 Joseph Kain     Fixed errors introduced by
**       my previous merge.
**  56   3dfx      1.52.1.2    06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  55   3dfx      1.52.1.1    06/15/00 Bill White      Merged changes to support
**       Linux.
** 
**  54   3dfx      1.52.1.0    05/28/00 Stephane Huaulme fixed Mac Headers crap
**       (UI3.3)
**  53   3dfx      1.52        04/25/00 Adam Briggs     make grSelectContext
**       actually return a useful value in windowed mode
**  52   3dfx      1.51        04/25/00 Kenneth Dyke    Fix single buffered video
**       overlay init.
**  51   3dfx      1.50        04/25/00 Kenneth Dyke    Fixed overlay mode for SLI
**       non-AA modes.
**  50   3dfx      1.49        04/21/00 Kenneth Dyke    Disabled new style 2sample
**       aa on 4-way boards until it works.
**  49   3dfx      1.48        04/16/00 Kenneth Dyke    Nuke splash screen when SLI
**       band height is forced (for OpenGL).
**  48   3dfx      1.47        04/14/00 Kenneth Dyke    Make sure we don't clobber
**       SST_PARMADJUST when we set the column rendering width.
**  47   3dfx      1.46        04/13/00 Kenneth Dyke    Added support for new style
**       2-sample AA mode.
**  46   3dfx      1.45        04/11/00 Chris Dow       Fixed TMU sizing bug and
**       cleaned up some noodles.
**  45   3dfx      1.44        04/06/00 Kenneth Dyke    Don't trash render masks
**       whenever we set the renderMode.
**  44   3dfx      1.43        03/30/00 Kenneth Dyke    Do automagic SLI band
**       height setting.
**  43   3dfx      1.42        03/25/00 Adam Briggs     added support for
**       SSTH3_SLI_AA_CONFIGURATION (the env var the control panel uses to force AA
**       modes)
**  42   3dfx      1.41        03/23/00 Kenneth Dyke    Make sure we maintain state
**       across all chips.  Change default LOD bias to 0.5f.
**  41   3dfx      1.40        03/21/00 Kenneth Dyke    Tweaks for overlay video
**       format.
**       Support for not clamping SLI band height.
**       Allow user to override 16-bit rendering to 15-bit.
**  40   3dfx      1.39        03/19/00 Kenneth Dyke    Make sure 2ppc band height
**       is just set once.
**       Make sure extended alpha blend modes and stencil masks are set to a known
**       value.
**  39   3dfx      1.38        03/14/00 Adam Briggs     enable analog sli in 2X
**       modes or when forced
**  38   3dfx      1.37        03/13/00 Kenneth Dyke    Make sure 3D is idle when
**       we close down Glide.
**  37   3dfx      1.36        03/11/00 Kenneth Dyke    Make sure slave registers
**       are kept up to date after board is mapped in.
**  36   3dfx      1.35        03/08/00 Kenneth Dyke    Don't cache hardware
**       register pointers.
**  35   3dfx      1.34        03/08/00 Kenneth Dyke    New use isMapped boardInfo
**       flag instead of broken gc flag.
**  34   3dfx      1.33        03/07/00 Adam Briggs     implemented half-mode sli &
**       aa restrictions
**  33   3dfx      1.32        02/28/00 Adam Briggs     replaced hwcQueryContext
**       call with a reference to the lost context dword
**  32   3dfx      1.31        02/28/00 Kenneth Dyke    Fixed dither rotation stuff
**       for 4-sample AA.  We also no longer clobber the fog mode setting.
** 
**  31   3dfx      1.30        02/14/00 Kenneth Dyke    Constrain SLI band height
**       to make sure Y origin swapping works right.
**       Fixed memory requirement calculation to take into account SLI and AA.
** 
**  30   3dfx      1.29        02/08/00 Adam Briggs     got rid of a divide by zero
**       that would occur on voodoo3
**  29   3dfx      1.28        02/08/00 Kenneth Dyke    Initialize sliBandHeight in
**       the proper place.
**  28   3dfx      1.27        01/31/00 Adam Briggs     changed the IS_NAPALM macro
**       to cooperate with the display driver version of the same
**  27   3dfx      1.26        01/31/00 Kenneth Dyke    Changes to deal with
**       hardware AA issue.
**  26   3dfx      1.25        01/31/00 Adam Briggs     Changed all device ID magic
**       numbers to use those defined in fxhal.h & added IS_NAPALM macro to test
**       against device ID range
**  25   3dfx      1.24        01/30/00 Adam Briggs     get napalm status correctly
**  24   3dfx      1.23        01/28/00 Kenneth Dyke    Totoally revamped TMU
**       register update mechanism to make 2PPC modes work right regardless of the
**       order of Glide calls.   Also fixed a few register config bugs found when
**       switching between new and old style combine modes.
**  23   3dfx      1.22        01/23/00 Adam Briggs     initializing h3nwaysli
**       properly greatly aids in 4-sample AA init
**  22   3dfx      1.21        01/21/00 Adam Briggs     closer to working on 2 chip
**       sli
**  21   3dfx      1.20        01/19/00 Kenneth Dyke    Fixed unitialized variable
**       in command transport struct for grSstWinOpenExt.
**  20   3dfx      1.19        01/18/00 Kenneth Dyke    Get proper AA jitter
**       values.
**  19   3dfx      1.18        01/16/00 Kenneth Dyke    Sandbox stuff.
**       32-bit/AA override stuff
**       Triangle column width stuff.
**  18   3dfx      1.17        01/16/00 Kenneth Dyke    Fixes to allow better
**       32-bit/AA overrides.
**       Fixes to make non-565 rendering modes work right in minihwc.
**       Other various bring up fixes.
**  17   3dfx      1.16        01/04/00 Adam Briggs     changed grGetGammaTable to
**       be an extension called grGetGammaTableExt
**  16   3dfx      1.15        01/04/00 Chuck Smith     Added grGetGammaTable
**       funciton so that the OpenGL ICD can make use of the hwcGetGammaTable
**       function
**  15   3dfx      1.14        12/10/99 Leo Galway      Removed previous hi-res
**       mode information for Glide3. These modes were only necessary for
**       Cornerstone (or future hi-res) support in RT4.2 source branch and
**       proceeded to break the V3 and V2 builds (from 3dfx view), hence they have
**       been removed.
**  14   3dfx      1.13        12/08/99 Leo Galway      Added mode information for
**       1600x1280, 1792x1440, 1920x1080, 1920x1200, 2046x1536 (as a result of
**       glide being tested with Cornerstone modes). Although not all of these
**       modes are currently capable under Glide, their inclusion prevents Glide
**       apps from displaying in incorrect modes when these hi-res modes are
**       selected. Search for SUSTAINED_ENGINEERING_CHANGE_BEGIN. 
**  13   3dfx      1.12        12/03/99 Adam Briggs     Added some code for
**       y-origin swapping in sli mode (which doesn't seem to work on winsim
**       anyway)
**  12   3dfx      1.11        11/22/99 Kenneth Dyke    Use hwcIdleWinFifo().
**  11   3dfx      1.10        11/11/99 Adam Briggs     My bad:  Apparently V3
**       cards have zero chips on them, so don't even bother with the first step
**       through the SLI slave reg loops.
**  10   3dfx      1.9         11/10/99 Adam Briggs     Made grBufferClear(Ext)
**       cooperate with linear surfaces & texture buffers
**  9    3dfx      1.8         11/09/99 Larry  warner   Make the world safe for h3.
**  8    3dfx      1.7         11/09/99 Adam Briggs     Added support for reading
**       the Status reg on slave chips in order to form a more perfect flush
**       function.
**  7    3dfx      1.6         10/22/99 Anthony tai     disable 2ppc in winclose
**  6    3dfx      1.5         10/21/99 Larry  warner   Fix typo.
**  5    3dfx      1.4         10/21/99 Anthony tai     fixed h3 build
**  4    3dfx      1.3         10/20/99 Anthony tai     initialize tmu combine
**       register packet header
**  3    3dfx      1.2         10/08/99 Adam Briggs     Supported FX_GLIDE_BPP &
**       FX_GLIDE_AA_SAMPLE environment vars so the user can override the pixel
**       format in grSstWinOpen calls
**  2    3dfx      1.1         10/04/99 Matt McClure    Ported Glide Context
**       Switching code from V3_OEM_100.
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 159   9/09/99 4:24p Adamb
** set renderMode SST_RM_YORIGIN_TOP_SHIFT to height - 1 instead of height
** since coords are zero based.
** 
** 158   9/07/99 10:44a Atai
** fixed h3 build
** 
** 157   9/03/99 4:32p Atai
** disable aaCtrl and sliCtrl in winclose
** 
** 155   8/31/99 5:49p Atai
** added pixel sample for cfgAALfbCtrl
** 
** 154   8/20/99 4:56p Atai
** fixed packet4 register bit mask for window glide
** 
** 153   8/17/99 5:10p Kcd
** AGP Command FIFO fixes.
** 
** 152   8/16/99 11:18a Adamb
** Merged in V3_OEM_100 fixes
** 
** 151   8/11/99 5:09p Atai
** enable 2nd buffer
** 
** 150   8/11/99 3:44p Atai
** added multichip support for minihwc
** 
** 149   8/04/99 3:35p Atai
** 32 bpp happy
** 
** 148   7/28/99 11:20a Atai
** fixed h3 build
** 
** 147   7/27/99 6:18p Atai
** use grSstWinOpenExt for napalm
** 
** 146   7/26/99 12:11p Atai
** initialize pci registers for sli/aa
** 
** 145   7/22/99 1:18p Atai
** added grTBufferMaskExt
** 
** 144   7/21/99 4:05p Atai
** added sli ctrl code
** 
** 143   7/19/99 2:52p Atai
** added variable for sli
** 
** 142   7/18/99 2:25p Atai
** change 2ppc enabling condition
** 
** 141   7/18/99 1:59p Atai
** added grAlphaBlendFunctionExt
** 
** 139   7/14/99 6:23p Larryw
** Remove obsolete G3_LOD_TRANSLATE() macro
** Define _grMipMapOffset[][] at compile time
** Fix 2k texture address-finding
** 
** 138   7/14/99 9:39a Atai
** direct register write for glide3x
** test04 can do 4 sample aa (2 chips)
** 
** 137   7/12/99 12:35p Atai
** initialize tmu base address for napalm
** 
** 136   7/06/99 2:51p Atai
** enable gbc and fixed minor things.
** 
** 135   6/25/99 2:11p Atai
** more 2 buffers stuff
** 
** 134   6/24/99 7:19p Atai
** 2 sample aa
** 
** 133   6/21/99 1:27p Atai
** overwrtie napalm memory allocation stuff
** 
** 132   6/19/99 11:27p Atai
** fixed fbOffset
** 
** 131   6/18/99 10:41p Atai
** reset fbOffset
** 
** 130   6/16/99 7:26p Larryw
** Took out some lines from my last install that don't belong there.
** 
** 129   6/16/99 7:00p Larryw
** Y-origin subtraction.
** 
** 128   6/14/99 4:28p Atai
** more on 2nd buffer allocation
** 
** 127   6/14/99 3:19p Atai
** added secondary buffer info
** 
** 126   6/14/99 2:05p Atai
** added grPixelFormat and grPixelSample.
** 
** 124   6/13/99 6:07p Atai
** remove aa type for winopen ext
** 
** 123   6/09/99 5:23p Atai
** added _grChipMask
** 
** 122   6/09/99 12:02p Atai
** change csim tram size for 2k x 2k texture testing
** 
** 121   6/04/99 11:00a Atai
** added stencil functions
** 
** 120   6/03/99 4:26p Atai
** added chipMask
** 
** 119   6/01/99 2:33p Atai
** Added grSstWinOpe Ext
** 
** 117   5/28/99 12:55p Atai
** fixed clip coord, fog coord ext with w-buffering
** 
** 115   5/26/99 4:18p Kcd
** Enable LFB byte and word swizzling by default for PowerPC systems.
** 
** 114   5/24/99 2:49p Jamesb
** Added ptrLostContext field to exported command transport struct.
** 
** 113   5/19/99 3:55p Denis
** 
** 112   5/19/99 12:45p Denis
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
** 111   4/16/99 2:57p Kcd
** PowerPC PCI Bump & Grind
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
** new splash
** 
** 84    2/18/99 3:51p Kcd
** Fixed dumb Codewarrior warning.
** 
** 83    2/18/99 3:05p Atai
** Hack for Fifa99! Fifa99 calls guGammaCorrectionRGB after they use
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
 * simulator happiness w/o fifo
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
 * fixed csim/non-debug stupidity
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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_MACOS)
#define __MACERRORS__
#include <DriverServices.h>
#endif

#if (GLIDE_PLATFORM & GLIDE_SST_SIM)
#ifdef HAL_CSIM
#include <csim.h>
static FxU32 lostcontext_csim;
#elif HSIM
#include <gsim.h>
#endif
#endif /* (GLIDE_PLATFORM & GLIDE_SST_SIM) */

#ifdef DRI_BUILD
#include <lindri.h>
#endif

#define kPageBoundarySlop 0x1000UL
#define kPageBoundaryMask (kPageBoundarySlop - 1)

/* Some forward declarations */
void _grImportFifo (int, int);
GR_ENTRY(grDRIBufferSwap, void, (FxU32 swapInterval));
#ifdef FX_GLIDE_NAPALM
static void _grSstSetColumnsOfNWidth(FxU32 width);
#endif /* FX_GLIDE_NAPALM */

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
  {GR_RESOLUTION_2048x2048, 2048,  2048}  /* 0x17 */
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
  GrTexInfo textureinfo = { GR_LOD_LOG2_1, GR_LOD_LOG2_1, 
                            GR_ASPECT_LOG2_1x1, GR_TEXFMT_8BIT, 0 };
  
#if FX_GLIDE_NAPALM
  if(IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
  }  
#endif
  /* Just set this once. */
  gc->state.shadow.fbzColorPath = SST_PARMADJUST;

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
#if FX_GLIDE_NAPALM
  if(IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    /* Make sure 2PPC stuff is in a known state. */
    gc->state.shadow.combineMode = SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK;
    _grTex2ppc(FXFALSE);
    
    grColorCombineExt(GR_CMBX_ITRGB,   GR_FUNC_MODE_X,
                      GR_CMBX_ZERO,    GR_FUNC_MODE_X,
                      GR_CMBX_ZERO,    FXTRUE,
                      GR_CMBX_ZERO,    FXFALSE,
                      0, FXFALSE);
    grTexColorCombineExt(GR_TMU0,
                         GR_CMBX_ZERO, GR_FUNC_MODE_X,
                         GR_CMBX_ZERO, GR_FUNC_MODE_X,
                         GR_CMBX_ZERO, FXFALSE,
                         GR_CMBX_ZERO, FXFALSE,
                         0, FXFALSE);
    grTexColorCombineExt(GR_TMU1,
                         GR_CMBX_ZERO, GR_FUNC_MODE_X,
                         GR_CMBX_ZERO, GR_FUNC_MODE_X,
                         GR_CMBX_ZERO, FXFALSE,
                         GR_CMBX_ZERO, FXFALSE,
                         0, FXFALSE);
    grAlphaCombineExt(GR_CMBX_CONSTANT_ALPHA,   GR_FUNC_MODE_X,
                      GR_CMBX_ZERO,    GR_FUNC_MODE_X,
                      GR_CMBX_ZERO,    FXTRUE,
                      GR_CMBX_ZERO,    FXFALSE,
                      0, FXFALSE);
    grTexAlphaCombineExt(GR_TMU0,
                         GR_CMBX_ZERO, GR_FUNC_MODE_X,
                         GR_CMBX_ZERO, GR_FUNC_MODE_X,
                         GR_CMBX_ZERO, FXFALSE,
                         GR_CMBX_ZERO, FXFALSE,
                         0, FXFALSE);
    grTexAlphaCombineExt(GR_TMU1,
                         GR_CMBX_ZERO, GR_FUNC_MODE_X,
                         GR_CMBX_ZERO, GR_FUNC_MODE_X,
                         GR_CMBX_ZERO, FXFALSE,
                         GR_CMBX_ZERO, FXFALSE,
                         0, FXFALSE);
    grAlphaBlendFunctionExt(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_OP_ADD,
                            GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_OP_ADD);
    grColorMaskExt(FXTRUE, FXTRUE, FXTRUE, FXTRUE);
    grStencilMask(0);
    gc->state.tbufferMask = 0xff/*0xf*/; /* KoolSmoky - enable 8 tbuffers for 4 chip config */
  }
#endif  
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
  grDitherMode(GR_DITHER_2x2);
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
    grTexLodBiasValue(GR_TMU1, 0.5F);
    grTexMipMapMode(GR_TMU1, GR_MIPMAP_DISABLE, FXFALSE);
    grTexCombine(GR_TMU1, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 FXFALSE, FXFALSE);
    /* Intentional fallthrough */
    /* 
    ** napalm glide place its fifo at low address 0, to prevent tmu read
    ** from fifo, we initialize the tmu base address to its min address
    */
    if ((IS_NAPALM(gc->bInfo->pciInfo.deviceID)) && (!gc->windowed)) {
      grTexSource(GR_TMU1,
                  grTexMinAddress(GR_TMU1),
                  GR_MIPMAPLEVELMASK_BOTH,
                  &textureinfo);
    }

  case 1:
    grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
    grTexDetailControl(GR_TMU0, 0, 1, 1.0F);
    grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, 
                    GR_TEXTUREFILTER_POINT_SAMPLED);
    grTexLodBiasValue(GR_TMU0, 0.5F);
    grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
    grTexCombine(GR_TMU0, 
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 FXFALSE, FXFALSE);
    if ((IS_NAPALM(gc->bInfo->pciInfo.deviceID)) && (!gc->windowed)) {
      grTexSource(GR_TMU0,
                  grTexMinAddress(GR_TMU0),
                  GR_MIPMAPLEVELMASK_BOTH,
                  &textureinfo);
    }
  }
  grLfbConstantAlpha(0xFF);
  grLfbConstantDepth(0);

  gc->triSetupProc = CUR_TRI_PROC(FXTRUE, (gc->state.cull_mode != GR_CULL_DISABLE));

  /* Napalm-specific initialization kruft */
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    _grSstSetColumnsOfNWidth(8); /* 8 is default for now */
  }
  gc->state.mode2ppc = FXFALSE;
  gc->state.mode2ppcTMU = 0xFFFFFFFF;
} /* assertDefaultState */

#ifndef	DRI_BUILD
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
#elif 0	/* defined(DRI_BUILD) */ /* not used in DRI build */
static void 
clearBuffers( GrGC *gc ) 
{
  /* Get rid of crap in the buffers. */
  grClipWindow(0, 0, gc->state.screen_width, 
               gc->state.screen_height);
  if ( gc->state.num_buffers > 1 ) {
    grBufferClear( 0, 0, 0UL );
    grDRIBufferSwap( 1 );
    grBufferClear( 0, 0, 0xFFFFFFFFUL );
    grDRIBufferSwap( 1 );
    grBufferClear( 0, 0, 0xFFFFFFFFUL );
    grDRIBufferSwap( 1 );
    grRenderBuffer( GR_BUFFER_BACKBUFFER );
  } else {
    grBufferClear( 0, 0, 0xFFFFFFFFUL );
    grRenderBuffer( GR_BUFFER_FRONTBUFFER );
  }
} /* clearBuffers */
#endif	/* defined(DRI_BUILD) */

static void 
doSplash( void ) 
{
#if defined(GLIDE_SPLASH) || ((GLIDE_PLATFORM & GLIDE_OS_WIN32) && defined(GLIDE_PLUG))
  GR_DCL_GC;
#endif

  /* The splash screen wants a swapped Y origin, which doesn't
   * work in all SLI configs. */
  if(_GlideRoot.environment.sliBandHeightForce)
    return;

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

#if _FIFODUMP
  gc->myLevel = 0; /* KoolSmoky */
#endif

  /* Setup the indices of the logical buffers */
#ifdef DRI_BUILD
  gc->curBuffer   = (gc->grColBuf > 1) ? 1 : 0;
  gc->frontBuffer = 0;
#else	/* defined(DRI_BUILD) */
  gc->curBuffer   = 0;
  gc->frontBuffer = ((gc->grColBuf > 1) ? 1 : 0);
#endif	/* defined(DRI_BUILD) */
  gc->backBuffer  = (gc->grColBuf > 2) ? 2 : gc->curBuffer;
  
  for (t = 0; t < MAX_BUFF_PENDING; t++) {
    gc->bufferSwaps[t] = 0xffffffff;
  }
  
  gc->bufferSwaps[0] = ((unsigned long) gc->cmdTransportInfo.fifoPtr -
                        (unsigned long) gc->cmdTransportInfo.fifoStart);
  
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

  /* for fog coords and prepare for Napalm */
  gc->state.depth_range          = 65535.f;

  /* NB: This loop has to setup the packet headers for *ALL* of the
   * tmu persistent state. F*ck this up at your own peril because
   * someone who cares about running glide in a window will come and
   * kill you as you sit in your cube.
   */
#if defined( USE_PACKET_FIFO )
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

#ifdef FX_GLIDE_NAPALM
    if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
      gc->state.shadow.tmuState[t].texPkt4Hdr_3 =
        ( SR_MASK_7 << SSTCP_PKT4_MASK_SHIFT ) |
        FIFO_REG((0x02UL << t), combineMode) |
        SSTCP_PKT4;
      
      gc->state.shadow.tmuState[t].combineMode = 0x00000000;
    }
#endif

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
    gc->state.per_tmu[t].smallLod = GR_LOD_LOG2_1;
    gc->state.per_tmu[t].largeLod = GR_LOD_LOG2_1;
    gc->state.per_tmu[t].s_scale  = 256.f;
    gc->state.per_tmu[t].t_scale  = 256.f;
    gc->state.per_tmu[t].evenOdd  = GR_MIPMAPLEVELMASK_BOTH;
    gc->state.per_tmu[t].nccTable = GR_NCCTABLE_NCC0;
    gc->state.per_tmu[t].texSubLodDither = FXFALSE;
  } 
#endif
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
                    lower left
  nColBuffers - number of color buffers to attempt to allocate
                0 - meaningless
                1 - allocate a front buffer only
                2 - allocate a front and back buffer
                3 - allocate a front, back, aux buffer for triple buffering
  nAuxBuffers - number of aux buffers to attempt to allocate
                0 - no alpha or z buffers
                1 - allocate one aux buffer for alpha/depth buffering
                2 - allocate one depth and one alpha buffer (unsup)
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

  return ( grSstWinOpenExt(hWnd,
                           resolution, 
                           refresh, 
                           format, 
                           origin, 
                           GR_PIXFMT_RGB_565,
                           nColBuffers,
                           nAuxBuffers) );

#undef FN_NAME
} /* grSstWinOpen */

#ifdef FX_GLIDE_NAPALM
/*-------------------------------------------------------------------
  Function: grSstWinOpenExt
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
                    lower left
  pixelformat - requested pixel format
  nColBuffers - number of color buffers to attempt to allocate
                0 - meaningless
                1 - allocate a front buffer only
                2 - allocate a front and back buffer
                3 - allocate a front, back, aux buffer for triple buffering
  nAuxBuffers - number of aux buffers to attempt to allocate
                0 - no alpha or z buffers
                1 - allocate one aux buffer for alpha/depth buffering
                2 - allocate on depth and one alpha buffer (unsup)
  fsAA - type of full screen AA algorithm
  Return:
  NULL - glide was unable to create a fullscreen context
  context handle - glide was able to create a context with handle H
  -------------------------------------------------------------------*/

GR_EXT_ENTRY(grSstWinOpenExt, GrContext_t, ( FxU32                   hWnd,
                                             GrScreenResolution_t    resolution, 
                                             GrScreenRefresh_t       refresh, 
                                             GrColorFormat_t         format, 
                                             GrOriginLocation_t      origin, 
                                             GrPixelFormat_t         pixelformat,
                                             int                     nColBuffers,
                                             int                     nAuxBuffers) )
{
#define FN_NAME "grSstWinOpenExt"
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
  FxU32         hwPixelFormat;
#elif defined( GLIDE_INIT_HAL )
  FxDeviceInfo   devInfo;
  hwcBoardInfo  *bInfo   = 0;
  hwcVidInfo    *vInfo   = 0;
  hwcBufferInfo *bufInfo = 0;
  hwcFifoInfo   *fInfo   = 0;

#endif /* defined ( GLIDE_INIT_HAL ) */
  int            buffer;
  struct cmdTransportInfo *gcFifo = 0;
  GrContext_t retVal = 0;
  FxU32 tramShift, tmu1Offset;

#if !(GLIDE_PLATFORM & GLIDE_OS_UNIX) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
  if (!hWnd) hWnd = (FxU32) GetActiveWindow();
  if (!hWnd)
    GrErrorCallback("grSstWinOpen: need to use a valid window handle",
                    FXTRUE);
/*
  GDBG_INFO(80, "Setting hwnd to foreground.\n");
  SetForegroundWindow((HWND)hWnd);
*/

  /* Want Windowed Mode */
  if (resolution == GR_RESOLUTION_NONE)
  {
    GrContext_t _grCreateWindowSurface(FxU32 hWnd,
                                       GrColorFormat_t    format,
                                       GrOriginLocation_t origin,
                                       GrPixelFormat_t    pixelformat,
                                       int                nAuxBuffer);
    
    return _grCreateWindowSurface(hWnd, format, origin, pixelformat, nAuxBuffers);
  }
#endif	/* (GLIDE_PLATFORM & GLIDE_OS_UNIX) || (GLIDE_PLATFORM & GLIDE_OS_DOS32) */
  
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


#ifdef	DRI_BUILD
    gc->state.screen_width = driInfo.screenWidth;
    gc->state.screen_height = driInfo.screenHeight;
#else	/* defined(DRI_BUILD) */
    gc->state.screen_width  = _resTable[resolution].xres;
    gc->state.screen_height = _resTable[resolution].yres;
    GR_CHECK_F( FN_NAME, 
                resolution != _resTable[resolution].resolution, 
                "resolution table compilation incorrect" );
    
    if ( gc->vidTimings ) {
      gc->state.screen_width  = gc->vidTimings->xDimension;
      gc->state.screen_height = gc->vidTimings->yDimension;
    }
#endif	/* defined(DRI_BUILD) */
    
    /* this is a stupid hack but... */
    gc->chipCount = 1;
    
    /* Set this to 0 by detault */
    gc->state.forced32BPP = 0;
    
    if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) 
    {
      /* I apologize for this hack:
       * if glide was previously in a half-mode
       * gc->chipCount was forced to one. 
       * It should be restored before we try to use it.
       */
      
      gc->chipCount = gc->bInfo->pciInfo.numChips ;
      
      /* All this stuff lets Joe bag-o-donuts force old apps */
      /* to render with 32bpp and AA modes. */
      /* Bear in mind: it is silly to try to force 16bpp */
      /* rendering since we don't want to scale down the */
      /* apps Z or W values to fit in a 16 bit depth buffer. */

      if (_GlideRoot.environment.outputBpp == 32 || pixelformat == GR_PIXFMT_ARGB_8888) {
        // App requested 16 bit, but we are giving 32. Need to remember for framebuffer access
        if (pixelformat == GR_PIXFMT_ARGB_1555 ||
            pixelformat == GR_PIXFMT_AA_2_ARGB_1555 ||
            pixelformat == GR_PIXFMT_AA_4_ARGB_1555 ||
            pixelformat == GR_PIXFMT_AA_8_ARGB_1555) {
          gc->state.forced32BPP = 15;
        } else if (pixelformat == GR_PIXFMT_RGB_565 ||
                   pixelformat == GR_PIXFMT_AA_2_RGB_565 ||
                   pixelformat == GR_PIXFMT_AA_4_RGB_565 ||
                   pixelformat == GR_PIXFMT_AA_8_RGB_565) {
          gc->state.forced32BPP = 16;
        }
        
        if ((_GlideRoot.environment.aaSample == 8) && /* 8xaa */ (gc->chipCount > 2)) {
          pixelformat = GR_PIXFMT_AA_8_ARGB_8888;
        } else if ((_GlideRoot.environment.aaSample == 4) && (gc->chipCount > 1)) {
          pixelformat = GR_PIXFMT_AA_4_ARGB_8888;
        } else if (_GlideRoot.environment.aaSample == 2) {
          pixelformat = GR_PIXFMT_AA_2_ARGB_8888;
        } else {
          pixelformat = GR_PIXFMT_ARGB_8888;
        }
      } else if (_GlideRoot.environment.outputBpp == 15 || pixelformat == GR_PIXFMT_ARGB_1555) {
        if ((_GlideRoot.environment.aaSample == 8) && /* 8xaa */ (gc->chipCount > 2)) {
          pixelformat = GR_PIXFMT_AA_8_ARGB_1555;
        } else if ((_GlideRoot.environment.aaSample == 4) && (gc->chipCount > 1)) {
          pixelformat = GR_PIXFMT_AA_4_ARGB_1555;
        } else if (_GlideRoot.environment.aaSample == 2) {
          pixelformat = GR_PIXFMT_AA_2_ARGB_1555;
        } else {
          pixelformat = GR_PIXFMT_ARGB_1555;
        }
      } else if (pixelformat == GR_PIXFMT_RGB_565) {
        if ((_GlideRoot.environment.aaSample == 8) && /* 8xaa */ (gc->chipCount > 2)) {
          pixelformat = GR_PIXFMT_AA_8_RGB_565;
        } else if ((_GlideRoot.environment.aaSample == 4) && (gc->chipCount > 1)) {
          pixelformat = GR_PIXFMT_AA_4_RGB_565;
        } else if (_GlideRoot.environment.aaSample == 2) {
          pixelformat = GR_PIXFMT_AA_2_RGB_565;
        } else {
          pixelformat = GR_PIXFMT_RGB_565;
        }
      }
    }

    /* Automagic SLI band height settings *//* magical indeed */
    if(gc->state.screen_height >= 768) {
      gc->sliBandHeight = 5;
    } else {
      gc->sliBandHeight = 4;
    }
    
    GDBG_INFO(80,"Default band height: %d\n",gc->sliBandHeight);

    /* Allow user override (within reason). */
    if(_GlideRoot.environment.sliBandHeight != 0) {
      GDBG_INFO(80,"User set sli band height to: %d\n",_GlideRoot.environment.sliBandHeight);
      gc->sliBandHeight = _GlideRoot.environment.sliBandHeight;
      if(gc->sliBandHeight < 1) {
        GDBG_INFO(80,"Clamping band height to 1.\n");
        gc->sliBandHeight = 1;
      } else if(gc->sliBandHeight > 5) {
        GDBG_INFO(80,"Clamping band height to 5.\n");
        gc->sliBandHeight = 5;
      }  
    }

#ifdef DRI_BUILD
    /* The DRI knows how the framebuffer should be configured */
    if (driInfo.cpp==3 || driInfo.cpp==4) { /* 24 or 32bpp modes */
      /* XXX Check for AA flags here too */
      pixelformat = GR_PIXFMT_ARGB_8888;
    }
#endif

    gc->state.origin             = origin;
    gc->grSstRez                 = resolution;
    gc->grSstRefresh             = refresh;
    gc->grColBuf                 = gc->state.num_buffers = nColBuffers;
    gc->grAuxBuf                 = nAuxBuffers;
    gc->fbStride                 = gc->state.screen_width * BYTES_PER_PIXEL;
    gc->grHwnd                   = (int) hWnd;
    gc->grPixelFormat            = (int) pixelformat;
    gc->chipmask                 = SST_CHIP_MASK_ALL_CHIPS;

    switch (pixelformat) {
    case GR_PIXFMT_RGB_565:
      gc->grPixelSample          = 1;
      gc->grPixelSize            = 2;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB565D;
      break;
    case GR_PIXFMT_ARGB_1555:
      gc->grPixelSample          = 1;
      gc->grPixelSize            = 2;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB1555D;
      break;
    case GR_PIXFMT_ARGB_8888:
      gc->grPixelSample          = 1;
      gc->grPixelSize            = 4;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB32U;
      break;
    case GR_PIXFMT_AA_2_RGB_565:
      gc->grPixelSample          = 2;
      gc->grPixelSize            = 2;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB565U;
      break;
    case GR_PIXFMT_AA_2_ARGB_1555:
      gc->grPixelSample          = 2;
      gc->grPixelSize            = 2;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB1555U;
      break;
    case GR_PIXFMT_AA_2_ARGB_8888:
      gc->grPixelSample          = 2;
      gc->grPixelSize            = 4;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB32U;
      break;
    case GR_PIXFMT_AA_4_RGB_565:
      gc->grPixelSample          = 4;
      gc->grPixelSize            = 2;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB565U;
      break;
    case GR_PIXFMT_AA_4_ARGB_1555:
      gc->grPixelSample          = 4;
      gc->grPixelSize            = 2;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB1555U;
      break;
    case GR_PIXFMT_AA_4_ARGB_8888:
      gc->grPixelSample          = 4;
      gc->grPixelSize            = 4;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB32U;
      break;
    case GR_PIXFMT_AA_8_RGB_565:	/* 8xaa */
      gc->grPixelSample          = 8;
      gc->grPixelSize            = 2;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB565U;
      break;
    case GR_PIXFMT_AA_8_ARGB_1555:
      gc->grPixelSample          = 8;
      gc->grPixelSize            = 2;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB1555U;
      break;
    case GR_PIXFMT_AA_8_ARGB_8888:
      gc->grPixelSample          = 8;
      gc->grPixelSize            = 4;
      hwPixelFormat              = SST_OVERLAY_PIXEL_RGB32U;
      break;

    default:
      gc->grPixelSample          = 0;
      GDBG_INFO( gc->myLevel, "Unsupported Pixel Format = %d\n", pixelformat);
      GrErrorCallback( "grSstWinOpen: unsupported pixel format", FXFALSE );
      return 0;
      break;
    }

#if 0 /* Old Way */
    gc->sliCount = gc->chipCount / ((gc->grPixelSample == 4) ? 2 : 1);

    /* Default for the SLI case... */
    gc->grSamplesPerChip = gc->grPixelSample > 1 ? 2 : 1;
            
    /* We have two ways to do 2-sample AA when we have more than two chips.  We
     * can either do 2 samples per chip with 2-way (or 4-way) SLI, or we can do
     * one sample per chip with 1-way (or 2-way) SLI.  By default we'll opt for
     * non-SLI since in theory it has better performance.  We have an environment 
     * variable to override this, though. */
    if(gc->chipCount == 2 && gc->grPixelSample == 2 && gc->sliCount == 2) {
      if(!_GlideRoot.environment.forceOldAA) {
        gc->grSamplesPerChip = 1;
        gc->sliCount = 1;
        /* In this mode I think the video filter still works... */
        if(hwPixelFormat == SST_OVERLAY_PIXEL_RGB1555U) {
          hwPixelFormat = SST_OVERLAY_PIXEL_RGB1555D;
        } else if(hwPixelFormat == SST_OVERLAY_PIXEL_RGB565U) {
          hwPixelFormat = SST_OVERLAY_PIXEL_RGB565D;
        }
      }     
    } else if(gc->chipCount == 4 && gc->grPixelSample == 2 && gc->sliCount == 4) {
      /* This doesn't work yet */
      if(0 && !_GlideRoot.environment.forceOldAA) {
        gc->grSamplesPerChip = 1;
        gc->sliCount = 2;
        /* In this mode I think the video filter still works... */
        if(hwPixelFormat == SST_OVERLAY_PIXEL_RGB1555U) {
          hwPixelFormat = SST_OVERLAY_PIXEL_RGB1555D;
        } else if(hwPixelFormat == SST_OVERLAY_PIXEL_RGB565U) {
          hwPixelFormat = SST_OVERLAY_PIXEL_RGB565D;
        }    
      }  
    }    

#else

    switch( gc->chipCount ) {
    case 4:
      switch( gc->grPixelSample ) {
      case 8:
        gc->sliCount         = 1;
        gc->grSamplesPerChip = 2;
        gc->sampleOffsetIndex = 9;
        break;
        
      case 4:
        gc->sliCount         = 1;   /*no sli, 1 sample per chip */
        gc->grSamplesPerChip = 1;
        gc->sampleOffsetIndex = 8;
        break;

      case 2:
        if(!_GlideRoot.environment.forceOldAA) {
          gc->grSamplesPerChip	= 1;  //2 way SLI, 1 sample per SLI unit
          gc->sliCount          = 2;
          gc->sampleOffsetIndex = 7;
          /* In this mode I think the video filter still works... */
          if(hwPixelFormat == SST_OVERLAY_PIXEL_RGB1555U) {
            hwPixelFormat = SST_OVERLAY_PIXEL_RGB1555D;
          } else if(hwPixelFormat == SST_OVERLAY_PIXEL_RGB565U) {
            hwPixelFormat = SST_OVERLAY_PIXEL_RGB565D;
          }
        } else {
          gc->grSamplesPerChip	= 2; /* 4 way SLI, 2 samples per SLI unit */
          gc->sliCount          = 4; /* doesn't work yet */
          gc->sampleOffsetIndex = 7;
        }  
        break;

      case 1:
        gc->sliCount         = 4;
        gc->grSamplesPerChip = 1;
        gc->sampleOffsetIndex = 0;
        break;		
      }
      break;

    case 2: 
      switch( gc->grPixelSample ) {
      case 4:
        gc->sliCount         = 1;
        gc->grSamplesPerChip = 2;
        gc->sampleOffsetIndex = 3;
        break;

      case 2:
        if(!_GlideRoot.environment.forceOldAA) {
          gc->sliCount 	        = 1; /* no sli, 1 sample per chip */
          gc->grSamplesPerChip 	= 1;
          gc->sampleOffsetIndex = 2;
          if(hwPixelFormat == SST_OVERLAY_PIXEL_RGB1555U) {
            hwPixelFormat = SST_OVERLAY_PIXEL_RGB1555D;
          } else if(hwPixelFormat == SST_OVERLAY_PIXEL_RGB565U) {
            hwPixelFormat = SST_OVERLAY_PIXEL_RGB565D;
          }
        } else {
          gc->sliCount 	        = 2; /* 2 samples per SLI pair */
          gc->grSamplesPerChip 	= 2;
          gc->sampleOffsetIndex = 1;
        }
        break;
        
      case 1:
        gc->sliCount         = 2;
        gc->grSamplesPerChip = 1;
        gc->sampleOffsetIndex = 0;
        break;
      }
      break;


    case 1:
      switch( gc->grPixelSample ) {
      case 2:
        gc->sliCount         = 1; 
        gc->grSamplesPerChip = 2;
        gc->sampleOffsetIndex = 1;
        break;

      case 1:
        gc->sliCount         = 1;
        gc->grSamplesPerChip = 1;
        gc->sampleOffsetIndex = 0;
        break;
      }
      break;

    default:
      gc->sliCount         = 1;
      gc->grSamplesPerChip = 1;
      gc->sampleOffsetIndex = 0;
      break;

    }

#endif

    /* Yeesh. */
    gc->enableSecondaryBuffer = gc->grSamplesPerChip > 1 ? FXTRUE : FXFALSE;

    /* Precompute which table entries to use for per-chip primary and secondary AA offsets. */
    /* Index 0 - No AA     
     * Index 1 - 2-sample AA, 2 samples per chip
     * Index 2 - 2-sample AA, 1 sample per chip
     * Index 3 - 4-sample AA, 2 samples per chip  
        
     * Index 4 - 2-sample AA, 2 samples per chip - correct values
     * Index 5 - 2-sample AA, 1 sample per chip - correct values
     * Index 6 - 4-sample AA, 2 samples per chip - correct values
     */
#if 0
    /* adjust offset index for 4 chip cards */
    if(  gc->chipCount == 4 ) {
      switch ( gc->grPixelSample ) {
      case 8:
        gc->sampleOffsetIndex = 9;
        break;
        
      case 4:
        gc->sampleOffsetIndex =	8;
        break;
        
      case 2:
        gc->sampleOffsetIndex = 7;
        break;
      }
    } else {
      gc->sampleOffsetIndex = gc->grPixelSample-1 + ((gc->grSamplesPerChip == 1) ? 1 : 0);
      if (!GETENV("FX_GLIDE_AA_SAMPLE") && gc->sampleOffsetIndex)
        gc->sampleOffsetIndex+=3;
    }
#else
    if (!GETENV("FX_GLIDE_AA_SAMPLE") && gc->sampleOffsetIndex)
        gc->sampleOffsetIndex+=3;
#endif

        
    if (gc->sliCount == 0) {
      GDBG_INFO( gc->myLevel, "Unsupported Pixel Format = %d\n", pixelformat);
      GDBG_INFO( gc->myLevel, "Number of chips = %d\n", gc->chipCount);
      GDBG_INFO( gc->myLevel, "Number of pixel sample = %d\n", gc->grPixelSample);
      GrErrorCallback( "grSstWinOpen: unsupported pixel format", FXFALSE );
      return 0;
    }

    // test
    if (gc->chipCount == 4 && gc->grPixelSample == 2) {
      _GlideRoot.environment.aaXOffset[13][0] =
      _GlideRoot.environment.aaXOffset[13][4] = _GlideRoot.environment.aaXOffset[6][0];
      _GlideRoot.environment.aaXOffset[13][2] =
      _GlideRoot.environment.aaXOffset[13][6] = _GlideRoot.environment.aaXOffset[6][2];
      _GlideRoot.environment.aaYOffset[13][0] =
      _GlideRoot.environment.aaYOffset[13][4] = _GlideRoot.environment.aaYOffset[6][0];
      _GlideRoot.environment.aaYOffset[13][2] =
      _GlideRoot.environment.aaYOffset[13][6] = _GlideRoot.environment.aaYOffset[6][2];
      
      _GlideRoot.environment.aaXOffset[14][0] =
      _GlideRoot.environment.aaXOffset[14][4] = _GlideRoot.environment.aaXOffset[6][1];
      _GlideRoot.environment.aaXOffset[14][2] =
      _GlideRoot.environment.aaXOffset[14][6] = _GlideRoot.environment.aaXOffset[6][3];
      _GlideRoot.environment.aaYOffset[14][0] =
      _GlideRoot.environment.aaYOffset[14][4] = _GlideRoot.environment.aaYOffset[6][1];
      _GlideRoot.environment.aaYOffset[14][2] =
      _GlideRoot.environment.aaYOffset[14][6] = _GlideRoot.environment.aaYOffset[6][3];
    }
    else if (gc->chipCount == 4 && gc->grPixelSample == 4) {
      _GlideRoot.environment.aaXOffset[13][0] = _GlideRoot.environment.aaXOffset[12][0];
      _GlideRoot.environment.aaXOffset[13][2] = _GlideRoot.environment.aaXOffset[12][2];
      _GlideRoot.environment.aaXOffset[13][4] = _GlideRoot.environment.aaXOffset[12][4];
      _GlideRoot.environment.aaXOffset[13][6] = _GlideRoot.environment.aaXOffset[12][6];
      _GlideRoot.environment.aaYOffset[13][0] = _GlideRoot.environment.aaYOffset[12][0];
      _GlideRoot.environment.aaYOffset[13][2] = _GlideRoot.environment.aaYOffset[12][2];
      _GlideRoot.environment.aaYOffset[13][4] = _GlideRoot.environment.aaYOffset[12][4];
      _GlideRoot.environment.aaYOffset[13][6] = _GlideRoot.environment.aaYOffset[12][6];
      
      _GlideRoot.environment.aaXOffset[14][0] = _GlideRoot.environment.aaXOffset[12][1];
      _GlideRoot.environment.aaXOffset[14][2] = _GlideRoot.environment.aaXOffset[12][3];
      _GlideRoot.environment.aaXOffset[14][4] = _GlideRoot.environment.aaXOffset[12][5];
      _GlideRoot.environment.aaXOffset[14][6] = _GlideRoot.environment.aaXOffset[12][7];
      _GlideRoot.environment.aaYOffset[14][0] = _GlideRoot.environment.aaYOffset[12][1];
      _GlideRoot.environment.aaYOffset[14][2] = _GlideRoot.environment.aaYOffset[12][3];
      _GlideRoot.environment.aaYOffset[14][4] = _GlideRoot.environment.aaYOffset[12][5];
      _GlideRoot.environment.aaYOffset[14][6] = _GlideRoot.environment.aaYOffset[12][7];
    }
    else if (gc->chipCount == 4 && gc->grPixelSample == 8) {
      /* TODO: temporalAA over 4-chip 8xFSAA
       * probably too slow and probably not needed anyway
       */
      /* use 8xFSAA jitter values for now */
      _GlideRoot.environment.aaXOffset[13][0] =
      _GlideRoot.environment.aaXOffset[14][0] = _GlideRoot.environment.aaXOffset[12][0];
      _GlideRoot.environment.aaXOffset[13][1] =
      _GlideRoot.environment.aaXOffset[14][1] = _GlideRoot.environment.aaXOffset[12][1];
      _GlideRoot.environment.aaXOffset[13][2] =
      _GlideRoot.environment.aaXOffset[14][2] = _GlideRoot.environment.aaXOffset[12][2];
      _GlideRoot.environment.aaXOffset[13][3] =
      _GlideRoot.environment.aaXOffset[14][3] = _GlideRoot.environment.aaXOffset[12][3];
      _GlideRoot.environment.aaXOffset[13][4] =
      _GlideRoot.environment.aaXOffset[14][4] = _GlideRoot.environment.aaXOffset[12][4];
      _GlideRoot.environment.aaXOffset[13][5] =
      _GlideRoot.environment.aaXOffset[14][5] = _GlideRoot.environment.aaXOffset[12][5];
      _GlideRoot.environment.aaXOffset[13][6] =
      _GlideRoot.environment.aaXOffset[14][6] = _GlideRoot.environment.aaXOffset[12][6];
      _GlideRoot.environment.aaXOffset[13][7] =
      _GlideRoot.environment.aaXOffset[14][7] = _GlideRoot.environment.aaXOffset[12][7];
      
      _GlideRoot.environment.aaYOffset[13][0] =
      _GlideRoot.environment.aaYOffset[14][0] = _GlideRoot.environment.aaYOffset[12][0];
      _GlideRoot.environment.aaYOffset[13][1] =
      _GlideRoot.environment.aaYOffset[14][1] = _GlideRoot.environment.aaYOffset[12][1];
      _GlideRoot.environment.aaYOffset[13][2] =
      _GlideRoot.environment.aaYOffset[14][2] = _GlideRoot.environment.aaYOffset[12][2];
      _GlideRoot.environment.aaYOffset[13][3] =
      _GlideRoot.environment.aaYOffset[14][3] = _GlideRoot.environment.aaYOffset[12][3];
      _GlideRoot.environment.aaYOffset[13][4] =
      _GlideRoot.environment.aaYOffset[14][4] = _GlideRoot.environment.aaYOffset[12][4];
      _GlideRoot.environment.aaYOffset[13][5] =
      _GlideRoot.environment.aaYOffset[14][5] = _GlideRoot.environment.aaYOffset[12][5];
      _GlideRoot.environment.aaYOffset[13][6] =
      _GlideRoot.environment.aaYOffset[14][6] = _GlideRoot.environment.aaYOffset[12][6];
      _GlideRoot.environment.aaYOffset[13][7] =
      _GlideRoot.environment.aaYOffset[14][7] = _GlideRoot.environment.aaYOffset[12][7];
    }
    else if (gc->chipCount == 2 && gc->grPixelSample == 2) {
      _GlideRoot.environment.aaXOffset[13][0] = _GlideRoot.environment.aaXOffset[6][0];
      _GlideRoot.environment.aaXOffset[13][2] = _GlideRoot.environment.aaXOffset[6][2];
      _GlideRoot.environment.aaYOffset[13][0] = _GlideRoot.environment.aaYOffset[6][0];
      _GlideRoot.environment.aaYOffset[13][2] = _GlideRoot.environment.aaYOffset[6][2];
      
      _GlideRoot.environment.aaXOffset[14][0] = _GlideRoot.environment.aaXOffset[6][1];
      _GlideRoot.environment.aaXOffset[14][2] = _GlideRoot.environment.aaXOffset[6][3];
      _GlideRoot.environment.aaYOffset[14][0] = _GlideRoot.environment.aaYOffset[6][1];
      _GlideRoot.environment.aaYOffset[14][2] = _GlideRoot.environment.aaYOffset[6][3];
    }
    else if (gc->chipCount == 2 && gc->grPixelSample == 4) {
      _GlideRoot.environment.aaXOffset[13][0] = _GlideRoot.environment.aaXOffset[12][0];
      _GlideRoot.environment.aaXOffset[13][1] = _GlideRoot.environment.aaXOffset[12][2];
      _GlideRoot.environment.aaXOffset[13][2] = _GlideRoot.environment.aaXOffset[12][4];
      _GlideRoot.environment.aaXOffset[13][3] = _GlideRoot.environment.aaXOffset[12][6];
      _GlideRoot.environment.aaYOffset[13][0] = _GlideRoot.environment.aaYOffset[12][0];
      _GlideRoot.environment.aaYOffset[13][1] = _GlideRoot.environment.aaYOffset[12][2];
      _GlideRoot.environment.aaYOffset[13][2] = _GlideRoot.environment.aaYOffset[12][4];
      _GlideRoot.environment.aaYOffset[13][3] = _GlideRoot.environment.aaYOffset[12][6];
      
      _GlideRoot.environment.aaXOffset[14][0] = _GlideRoot.environment.aaXOffset[12][1];
      _GlideRoot.environment.aaXOffset[14][1] = _GlideRoot.environment.aaXOffset[12][3];
      _GlideRoot.environment.aaXOffset[14][2] = _GlideRoot.environment.aaXOffset[12][5];
      _GlideRoot.environment.aaXOffset[14][3] = _GlideRoot.environment.aaXOffset[12][7];
      _GlideRoot.environment.aaYOffset[14][0] = _GlideRoot.environment.aaYOffset[12][1];
      _GlideRoot.environment.aaYOffset[14][1] = _GlideRoot.environment.aaYOffset[12][3];
      _GlideRoot.environment.aaYOffset[14][2] = _GlideRoot.environment.aaYOffset[12][5];
      _GlideRoot.environment.aaYOffset[14][3] = _GlideRoot.environment.aaYOffset[12][7];
    }
    else if (gc->chipCount == 1 && gc->grPixelSample == 2) {
      /* TODO: temporalAA over 1-chip 2xFSAA
       * probably too slow
       */
      _GlideRoot.environment.aaXOffset[13][0] = _GlideRoot.environment.aaXOffset[6][0];
      _GlideRoot.environment.aaXOffset[13][1] = _GlideRoot.environment.aaXOffset[6][2];
      _GlideRoot.environment.aaYOffset[13][0] = _GlideRoot.environment.aaYOffset[6][0];
      _GlideRoot.environment.aaYOffset[13][1] = _GlideRoot.environment.aaYOffset[6][2];
      
      _GlideRoot.environment.aaXOffset[14][0] = _GlideRoot.environment.aaXOffset[6][1];
      _GlideRoot.environment.aaXOffset[14][1] = _GlideRoot.environment.aaXOffset[6][3];
      _GlideRoot.environment.aaYOffset[14][0] = _GlideRoot.environment.aaYOffset[6][1];
      _GlideRoot.environment.aaYOffset[14][1] = _GlideRoot.environment.aaYOffset[6][3];
    }
    if (gc->grPixelSample < 2) {
      int i;
      /* default perturbation values */
      for (i = 0; i < 8; i++) {
        _GlideRoot.environment.aaXOffset[13][i] = _GlideRoot.environment.aaXOffset[0][i];
        _GlideRoot.environment.aaYOffset[13][i] = _GlideRoot.environment.aaYOffset[0][i];
      }
      /* jittered values */
      for (i = 0; i < 8; i++) {
        _GlideRoot.environment.aaXOffset[14][i] = _GlideRoot.environment.aaXOffset[13][i] + 0x6;
        _GlideRoot.environment.aaYOffset[14][i] = _GlideRoot.environment.aaXOffset[13][i] + 0x6;
      }
    }

    /*
    ** 2 pixel per clock rendering will only be enabled if 
    ** 1) we are in 15/16 bpp rendering mode (may not help in high res)
    **    high res = GR_RESOLUTION_1600x1200 for now
    ** 2) low resolution in 32 bpp
    **    low res = 0 for now ( we may never turn it on )
    ** In other cases, we still rely on the lodmin = lodmax = 1 to 
    ** minimize the texture access
    **
    ** sli & aa rendering must be disabled in all half modes
    */
    if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) 
    {
      gc->do2ppc = FXFALSE;
      gc->bInfo->h3analogSli = 0 ;
      
      if (gc->grPixelSize <= 2) 
      {
        switch (gc->grSstRez) 
        {
          case GR_RESOLUTION_1600x1024:
             gc->do2ppc = FXTRUE;
             break;
          case GR_RESOLUTION_1600x1200:
             break;
          case GR_RESOLUTION_1792x1344:
          case GR_RESOLUTION_1856x1392:
          case GR_RESOLUTION_1920x1440:
          case GR_RESOLUTION_2048x1536:
          case GR_RESOLUTION_2048x2048:
            gc->bInfo->h3analogSli = 1 ;
            break;          
          case GR_RESOLUTION_400x300:
          case GR_RESOLUTION_320x200:
          case GR_RESOLUTION_320x240:
          case GR_RESOLUTION_400x256:
          case GR_RESOLUTION_512x256:
          case GR_RESOLUTION_512x384:
          case GR_RESOLUTION_640x200:
            gc->sliCount = 1 ;
            gc->chipCount = 1 ;
            gc->grPixelSample = 1 ;
          default:
            gc->do2ppc = FXTRUE;
            break;
        }
      }
      else if (gc->grPixelSize == 4) 
      {
        switch (gc->grSstRez) 
        {
          case GR_RESOLUTION_1600x1024:
            gc->bInfo->h3analogSli = 1 ;
            gc->do2ppc = FXTRUE;
            break;
          case GR_RESOLUTION_1600x1200:
          case GR_RESOLUTION_1792x1344:
          case GR_RESOLUTION_1856x1392:
          case GR_RESOLUTION_1920x1440:
          case GR_RESOLUTION_2048x1536:
          case GR_RESOLUTION_2048x2048:
            gc->bInfo->h3analogSli = 1 ;
            break;          
          case GR_RESOLUTION_400x300:
          case GR_RESOLUTION_320x200:
          case GR_RESOLUTION_320x240:
          case GR_RESOLUTION_400x256:
          case GR_RESOLUTION_512x256:
          case GR_RESOLUTION_512x384:
          case GR_RESOLUTION_640x200:
            gc->sliCount = 1 ;
            gc->chipCount = 1 ;
            gc->grPixelSample = 1 ;
          default:
            break;
        }
      }

      /* If we force the env var to 1, always turn 2ppc on.
       * Otherwise, we only enable 2ppc in certain condition.
       * FX_GLIDE_2PPC: -1=disable, 1=enable, 0=glide decides
       * 2ppc is always enabled for now. see gpci.c
       */
      if (_GlideRoot.environment.do2ppc < 0) {
        gc->do2ppc = FXFALSE;
      } else if (_GlideRoot.environment.do2ppc) {
        gc->do2ppc = FXTRUE;
      }

      /*
       * Ditto for analog sli
       */
      if (_GlideRoot.environment.analogSli < 0) {
        gc->bInfo->h3analogSli = 0;
      } else if (_GlideRoot.environment.analogSli) {
        gc->bInfo->h3analogSli = 1;
      }

      /*
       * This seems like bad news to me, but the control
       * panel applet is supposed to be able to turn off
       * SLI.
       */
      if (_GlideRoot.environment.forceSingleChip) {
        gc->sliCount = 1 ;
        gc->chipCount = 1 ;
      }
      
    }

    /* enable analog for 8xaa 4 chip cards */
    if( gc->chipCount == 4 ) gc->bInfo->h3analogSli = 1 ;

    GDBG_INFO(80, "%s: sliCount = %d\n", FN_NAME, gc->sliCount);
    GDBG_INFO(80, "%s: chipCount = %d\n", FN_NAME, gc->chipCount);
    
    /* compute tile dimensions */
    gc->strideInTiles  = ( gc->state.screen_width * (gc->grPixelSize >> 1)  + ( TILE_WIDTH_PXLS - 1 ) ) / TILE_WIDTH_PXLS;
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
#ifdef FX_GLIDE_NAPALM
    /* We must constrain the sli band height such that Y origin swapping
     * values work correctly. */
    if(gc->sliCount > 1)
    {
      FxU32 chipScreenHeight;
      FxU32 maxBandHeightLog2 = 0;
      FxU32 sliBandHeightInPixels;
      FxU32 numBands;

//        chipScreenHeight = gc->state.screen_height >> (gc->sliCount - 1);

      chipScreenHeight = gc->state.screen_height/gc->sliCount;

      /* Find the biggest value that's still 
       * divisible by a power of two.  The check
       * for a non-zero chipScreenHeight is just 
       * in case something bad happens and it starts         
       * out as zero. */
      if(!_GlideRoot.environment.sliBandHeightForce) {
        while(!(chipScreenHeight & 1) && chipScreenHeight) {
          maxBandHeightLog2++;
          chipScreenHeight >>= 1;
        }
        if(gc->sliBandHeight > maxBandHeightLog2) {
          gc->sliBandHeight = maxBandHeightLog2;
          GDBG_INFO(80, "%s: Clamping SLI band height (Log2) to %d\n",FN_NAME, maxBandHeightLog2);
        }
      }

      /* Recompute buffer memory requirements */
      sliBandHeightInPixels = 1L << gc->sliBandHeight;
//        chipScreenHeight = gc->state.screen_height >> (gc->sliCount - 1);
      chipScreenHeight = gc->state.screen_height / gc->sliCount;

      GDBG_INFO(80, "%s: SLI band height in pixels: %d\n",FN_NAME, sliBandHeightInPixels);
      numBands = (chipScreenHeight + (sliBandHeightInPixels - 1)) / sliBandHeightInPixels;
      GDBG_INFO(80, "%s: SLI bands required: %d\n", FN_NAME, numBands);
      chipScreenHeight = numBands * sliBandHeightInPixels;
      GDBG_INFO(80, "%s: SLI chip screen height: %d\n",FN_NAME, chipScreenHeight);
      gc->heightInTiles  = ( chipScreenHeight + ( TILE_HEIGHT_PXLS - 1 ) ) / TILE_HEIGHT_PXLS;
      GDBG_INFO(80, "%s: SLI heightInTiles = 0x%x\n", FN_NAME, gc->heightInTiles);
      gc->bufSizeInTiles = gc->strideInTiles * gc->heightInTiles;
      GDBG_INFO(80, "%s: SLI bufSizeInTiles = 0x%x\n", FN_NAME, gc->bufSizeInTiles);
      gc->bufSize = gc->bufSizeInTiles * TILE_WIDTH_PXLS * TILE_HEIGHT_PXLS * BYTES_PER_PIXEL;
      GDBG_INFO(80, "%s: SLI bufSize = 0x%x\n", FN_NAME, gc->bufSize);           
    }
#endif
    
    if ( ( 
    /* If we are doing 2 or 4 sample AA, then each chip needs twice as many buffers */
#ifdef FX_GLIDE_NAPALM            
            gc->grSamplesPerChip * 
#endif                    
            gc->bufSize * ( gc->grColBuf + gc->grAuxBuf ) + MIN_TEXTURE_STORE + MIN_FIFO_SIZE) >
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
    if (!gc->bInfo->isMapped) {
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

    }

    /* Don't assume that because the board was still mapped that these didn't change! */
    gc->sstRegs = (SstRegs*)bInfo->regInfo.sstBase;
    gc->ioRegs  = (SstIORegs*)bInfo->regInfo.ioMemBase;
    gc->cRegs   = (SstCRegs*)bInfo->regInfo.cmdAGPBase;
    gc->lfb_ptr = (FxU32*)bInfo->regInfo.lfbBase;
    gc->rawLfb  = (FxU32*)bInfo->regInfo.rawLfbBase;
    gc->tex_ptr = (FxU32*)SST_TEX_ADDRESS(bInfo->regInfo.sstBase);
  
#ifdef FX_GLIDE_NAPALM
    if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
      if (gc->chipCount) {
        FxU32 chip ;
        for (chip = 0 ;
             chip < gc->chipCount - 1 ;
             chip++) {
          gc->slaveSstRegs[chip] = (SstRegs *) bInfo->regInfo.slaveSstBase[chip] ;
          gc->slaveCRegs[chip] =   (SstCRegs *)bInfo->regInfo.slaveCmdBase[chip] ;
        }
      }
    }
#endif

#ifdef	DRI_BUILD
    vInfo->xRes              = driInfo.w;
    vInfo->yRes              = driInfo.h;
#else	/* defined(DRI_BUILD) */
    vInfo->xRes              = gc->state.screen_width;
    vInfo->yRes              = gc->state.screen_height;
#endif	/* defined(DRI_BUILD) */
    vInfo->refresh           = gc->grSstRefresh;
    vInfo->tiled             = FXTRUE;
    vInfo->initialized       = FXTRUE;
    bufInfo->enable2ndbuffer = gc->enableSecondaryBuffer;
    bInfo->h3pixelSize       = gc->grPixelSize;
    bInfo->h3pixelSample     = gc->grPixelSample;
    bInfo->h3nwaySli         = gc->sliCount ;
    bInfo->h3sliBandHeight   = 1L << gc->sliBandHeight; /* KCD: hwc buffer allocation needs this */

    gc->colTiled = gc->auxTiled = FXTRUE ; /* AJB- grBufferClear needs to know this */

    if ( hwcAllocBuffers( bInfo, nColBuffers, nAuxBuffers ) == FXFALSE ) {
      GDBG_INFO( gc->myLevel, "hwcAllocBuffers failed\n" );
      GrErrorCallback(hwcGetErrorString(), FXFALSE);
      return 0;
    }
  
    for (buffer = 0; buffer < nColBuffers; buffer++) {
      gc->buffers0[buffer] = bufInfo->colBuffStart0[buffer];
      GDBG_INFO(80, "Buffer %d:  Start: 0x%x\n", buffer, gc->buffers0[buffer]);
      gc->lfbBuffers[buffer] = (unsigned long)gc->rawLfb + bufInfo->lfbBuffAddr0[buffer];
      if (bInfo->buffInfo.enable2ndbuffer) {
        gc->buffers1[buffer] = bufInfo->colBuffStart1[buffer];
        GDBG_INFO(80, "Buffer %d:  Start: 0x%x\n", buffer, gc->buffers1[buffer]);
      }
    }
    if (nAuxBuffers != 0) {
      gc->buffers0[buffer] = bufInfo->auxBuffStart0;
      GDBG_INFO(80, "Aux Buffer:  Start: 0x%x\n", gc->buffers0[buffer]);
      gc->lfbBuffers[buffer] = (unsigned long)gc->rawLfb + bufInfo->lfbBuffAddr0[buffer];
      if (bInfo->buffInfo.enable2ndbuffer) {
        gc->buffers1[buffer] = bufInfo->auxBuffStart1;
        GDBG_INFO(80, "Aux Buffer:  Start: 0x%x\n", gc->buffers1[buffer]);
      }
    }
  
    vInfo->hWnd     = gc->grHwnd;
    vInfo->sRes     = gc->grSstRez;
    vInfo->vRefresh = gc->grSstRefresh;

    GDBG_INFO(80, "current_sst: %d\n", _GlideRoot.current_sst);
    if ( hwcInitVideo( bInfo, FXTRUE, gc->vidTimings, hwPixelFormat, FXTRUE ) == FXFALSE ) {
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
    
    /* This actually gets taken in hwcInitVideo */
    gc->contextP = FXTRUE;
    *gc->lostContext = FXFALSE;
#endif /* GLIDE_CHECK_CONTEXT */

    hwcGammaRGB(gc->bInfo,
                _GlideRoot.environment.gammaR,
                _GlideRoot.environment.gammaG,
                _GlideRoot.environment.gammaB);

    /* Setup memory configuration */
    gc->fbOffset = bInfo->fbOffset;
    
    /*
    ** if the environment variable is on, use the its texture memory size * 2
    */
    if ((_GlideRoot.environment.tmuMemory != -1) && (gc->fbOffset >= 0x200000))
        bInfo->tramSize = _GlideRoot.environment.tmuMemory << 21;

    GDBG_INFO(80, FN_NAME ": TMUs:  %d\n", gc->num_tmu);
    GDBG_INFO(80, FN_NAME ": Total TMU Memory: 0x%x\n", bInfo->tramSize);
    GDBG_INFO(80, FN_NAME ": Texture Memory Offset: 0x%x\n",bInfo->tramOffset);

    /*
    ** if we only have one TMU or we are using UMA, do similar things
    */
    if (
      (gc->num_tmu < 2) ||
      (gc->state.grEnableArgs.texture_uma_mode == GR_MODE_ENABLE)
      ) {
      tramShift = 0;
      tmu1Offset = 0;
      GDBG_INFO(80, FN_NAME ":  UMA ON\n");
    } else {
      tramShift = 1;
      tmu1Offset = bInfo->tramSize >> 1;
      GDBG_INFO(80, FN_NAME ":  UMA OFF\n");
    }

    switch (gc->num_tmu) {
    case 2:
      gc->tmuMemInfo[1].tramOffset = bInfo->tramOffset + tmu1Offset;
      gc->tmuMemInfo[1].tramSize   = (bInfo->tramSize >> tramShift);
      gc->tmu_state[1].total_mem   = gc->tmuMemInfo[1].tramSize;
      GDBG_INFO(80, FN_NAME ":  TMU1:  Offset 0x%x, size 0x%x\n",
                gc->tmuMemInfo[1].tramOffset, gc->tmuMemInfo[1].tramSize);
      /* NOTE:  NO BREAK.  THIS IS ON PURPOSE */
    case 1:
    default:
      gc->tmuMemInfo[0].tramOffset = bInfo->tramOffset;
      gc->tmuMemInfo[0].tramSize = (bInfo->tramSize >> tramShift);
      gc->tmu_state[0].total_mem = gc->tmuMemInfo[0].tramSize;
      GDBG_INFO(80, FN_NAME ":  TMU0:  Offset 0x%x, size 0x%x\n",
                gc->tmuMemInfo[0].tramOffset, gc->tmuMemInfo[0].tramSize);
      break;
    } /* gc->num_tmu */
    
    gc->tBuffer.bufType = HWC_BUFFER_TEXTUREBUFFER;
    gc->tBuffer.bufOffset = bInfo->tramOffset;
    gc->tBuffer.bufSize = bInfo->tramSize;
    gc->tBuffer.tiled = FXFALSE;
    gc->tBuffer.bufBPP = 0xffffffff; /* Don't matter to me */

    GDBG_INFO(1, "autoBump: 0x%x\n", _GlideRoot.environment.autoBump);
    /* The logic for this is hosed for PowerPC, where we disable auto-bump even
       on PCI. */
    if ((gc->cmdTransportInfo.autoBump = _GlideRoot.environment.autoBump)) {
      if (!hwcInitFifo( bInfo, gc->cmdTransportInfo.autoBump)) {
        hwcRestoreVideo(bInfo);
        GrErrorCallback(hwcGetErrorString(), FXFALSE);
        GDBG_INFO(gc->myLevel, "hwcInitFifo failed\n");
        GR_RETURN(FXFALSE);
      }
    } else {
      /* %%KCD - I pass in true because I know this will just fall through to hwcInitFifo() */
#if __POWERPC__
      if (!hwcInitAGPFifo(bInfo, FXFALSE)) {
#else
      if (!hwcInitAGPFifo(bInfo, FXTRUE)) {
#endif      
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
      GDBG_INFO(gc->myLevel, "autoBump = %i\n", gc->cmdTransportInfo.autoBump);
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
    gc->fbOffset                  = 0;
    gc->tmuMemInfo[0].tramOffset  = 
      (pixelformat == GR_PIXFMT_ARGB_8888) ? 0x400000 : 0x200000;
    gc->tmuMemInfo[0].tramSize = 
      (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) ? 0x1600000 : 0x0200000;
    gc->tmuMemInfo[1].tramOffset  = gc->tmuMemInfo[0].tramSize + gc->tmuMemInfo[0].tramOffset;
    gc->tmuMemInfo[1].tramSize = 
      (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) ? 0x1600000 : 0x0200000;
    gc->tmu_state[0].total_mem    = gc->tmuMemInfo[0].tramSize;
    gc->tmu_state[1].total_mem    = gc->tmuMemInfo[1].tramSize;
#endif

    /* napalm buffer allocation */
#define NAPALM_BUFFER_ALLOC 1
#if NAPALM_BUFFER_ALLOC
    {
      bInfo   = gc->bInfo;
      vInfo   = &bInfo->vidInfo;
      bufInfo = &bInfo->buffInfo;
      
      vInfo->xRes        = gc->state.screen_width;
      vInfo->yRes        = gc->state.screen_height;
      vInfo->refresh     = gc->grSstRefresh;
      vInfo->tiled       = FXFALSE;
      vInfo->initialized = FXTRUE;

      gc->colTiled = gc->auxTiled = FXFALSE ; /* AJB- grBufferClear needs to know this */

      if (GETENV("SST_FBI_MEM")) {
        bInfo->h3Mem = atoi(GETENV("SST_FBI_MEM"));
      }
      else {
        bInfo->h3Mem = 32;
      }

      bInfo->h3pixelSize = gc->grPixelSize;

      if (gc->grPixelSample > 1)
        bInfo->buffInfo.enable2ndbuffer = FXTRUE;

      if ( hwcAllocBuffers( gc->bInfo, nColBuffers, nAuxBuffers ) == FXFALSE ) {
        GDBG_INFO( gc->myLevel, "hwcAllocBuffers failed\n" );
        GrErrorCallback(hwcGetErrorString(), FXFALSE);
        return 0;
      }
      for ( buffer = 0; buffer < nColBuffers; buffer++ ) {
        gc->buffers0[buffer] = bufInfo->colBuffStart0[buffer];
        GDBG_INFO(80, "Buffer %d:  Start: 0x%x\n", buffer, gc->buffers0[buffer]);
        gc->lfbBuffers[buffer] = (FxU32)gc->rawLfb + bufInfo->lfbBuffAddr0[buffer];
        if (bInfo->buffInfo.enable2ndbuffer) {
          gc->buffers1[buffer] = bufInfo->colBuffStart1[buffer];
          GDBG_INFO(80, "Buffer %d:  Start: 0x%x\n", buffer, gc->buffers1[buffer]);
        }
      }
      if (nAuxBuffers != 0) {
        gc->buffers0[buffer] = bufInfo->auxBuffStart0;
        GDBG_INFO(80, "Aux Buffer:  Start: 0x%x\n", gc->buffers0[buffer]);
        gc->lfbBuffers[buffer] = (FxU32)gc->rawLfb + bufInfo->lfbBuffAddr0[buffer];
        if (bInfo->buffInfo.enable2ndbuffer) {
          gc->buffers1[buffer] = bufInfo->auxBuffStart1;
          GDBG_INFO(80, "Aux Buffer:  Start: 0x%x\n", gc->buffers1[buffer]);
        }
      }
    }
#else

    for ( buffer = 0; buffer < nColBuffers; buffer++ ) {
      gc->buffers0[buffer]    = gc->fbOffset + buffer * gc->bufSize;
      /* XXXjdt: this is never initialized in the old code */
      gc->lfbBuffers[buffer] = 0; 
      GDBG_INFO(80, "%s: Buffer %d: 0x%x\n",
                FN_NAME, buffer, gc->buffers0[buffer]);
    }
    if (nAuxBuffers != 0) {
        gc->buffers0[buffer] = gc->fbOffset + buffer * gc->bufSize;
        gc->lfbBuffers[buffer] = 0; 
        GDBG_INFO(80, "Aux Buffer:  Start: 0x%x\n", gc->buffers0[buffer]);
    }
#endif
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
#if NAPALM_BUFFER_ALLOC
    gcFifo->fifoOffset = 0x0;
    gcFifo->fifoSize = 0x20000;
#endif
#if NAPALM_BUFFER_ALLOC
    {
      gc->fbOffset = bInfo->fbOffset;
      gc->tmuMemInfo[0].tramOffset = bInfo->tramOffset;
      gc->tmuMemInfo[0].tramSize = bInfo->tramSize >> 1;
      gc->tmu_state[0].total_mem = gc->tmuMemInfo[0].tramSize;
      
      gc->tmuMemInfo[1].tramOffset = gc->tmuMemInfo[0].tramOffset + gc->tmuMemInfo[0].tramSize;
      gc->tmuMemInfo[1].tramSize   = (bInfo->tramSize >> 1);
      gc->tmu_state[1].total_mem   = gc->tmuMemInfo[1].tramSize;
      gcFifo->fifoOffset = 0x0;
      gcFifo->fifoSize = 0x20000;
    }
#endif
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
                                  gc->buffers0[gc->curBuffer], /* board address of beginning of OS  */
                                  gc->strideInTiles );        /* distance between scanlines of the OS, in*/

#if GLIDE_CHECK_CONTEXT
    /*
    ** initialize context checking
    */
    {
      gc->lostContext = &lostcontext_csim;
      *gc->lostContext = FXFALSE;
      gc->contextP = 1;
    }
#endif /* GLIDE_CHECK_CONTEXT */

#endif /*  defined( GLIDE_INIT_HAL )  */
#else  /* !defined( USE_PACKET_FIFO ) */
    gc->fbOffset                  = (FxU32)gc->rawLfb;
    gc->fbOffset                  = 0x0;
    gc->tmuMemInfo[0].tramOffset  = 0x200000;
    gc->tmuMemInfo[0].tramSize    = 0x200000;
    gc->tmuMemInfo[1].tramOffset  = gc->tmuMemInfo[0].tramSize + gc->tmuMemInfo[0].tramOffset;
    gc->tmuMemInfo[1].tramSize    = 0x200000;
    gc->tmu_state[0].total_mem    = gc->tmuMemInfo[0].tramSize;
    gc->tmu_state[1].total_mem    = gc->tmuMemInfo[1].tramSize;
    {
      bInfo   = gc->bInfo;
      vInfo   = &bInfo->vidInfo;
      bufInfo = &bInfo->buffInfo;
      
      vInfo->xRes        = gc->state.screen_width;
      vInfo->yRes        = gc->state.screen_height;
      vInfo->refresh     = gc->grSstRefresh;
      vInfo->tiled       = FXFALSE;
      vInfo->initialized = FXTRUE;

      gc->colTiled = gc->auxTiled = FXFALSE ; /* AJB- grBufferClear needs to know this */

      if (GETENV("SST_FBI_MEM")) {
        bInfo->h3Mem = atoi(GETENV("SST_FBI_MEM"));
      }
      else {
        bInfo->h3Mem = 32;
      }
      
      bInfo->h3pixelSize = gc->grPixelSize;

      bInfo->buffInfo.enable2ndbuffer = FXFALSE;
      if (gc->grPixelSample > 1)
        bInfo->buffInfo.enable2ndbuffer = FXTRUE;

      if ( hwcAllocBuffers( gc->bInfo, nColBuffers, nAuxBuffers ) == FXFALSE ) {
        GDBG_INFO( gc->myLevel, "hwcAllocBuffers failed\n" );
        GrErrorCallback(hwcGetErrorString(), FXFALSE);
        return 0;
      }
      for ( buffer = 0; buffer < nColBuffers; buffer++ ) {
        gc->buffers0[buffer] = bufInfo->colBuffStart0[buffer];
        GDBG_INFO(80, "Buffer %d:  Start: 0x%x\n", buffer, gc->buffers0[buffer]);
        gc->lfbBuffers[buffer] = (FxU32)gc->rawLfb + bufInfo->lfbBuffAddr0[buffer];
        if (bInfo->buffInfo.enable2ndbuffer) {
          gc->buffers1[buffer] = bufInfo->colBuffStart1[buffer];
          GDBG_INFO(80, "Buffer %d:  Start: 0x%x\n", buffer, gc->buffers1[buffer]);
        }
      }
      if (nAuxBuffers != 0) {
        gc->buffers0[buffer] = bufInfo->auxBuffStart0;
        GDBG_INFO(80, "Aux Buffer:  Start: 0x%x\n", gc->buffers0[buffer]);
        gc->lfbBuffers[buffer] = (FxU32)gc->rawLfb + bufInfo->lfbBuffAddr0[buffer];
        if (bInfo->buffInfo.enable2ndbuffer) {
          gc->buffers1[buffer] = bufInfo->auxBuffStart1;
          GDBG_INFO(80, "Aux Buffer:  Start: 0x%x\n", gc->buffers1[buffer]);
        }
      }
    }
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
                                  gc->buffers0[gc->curBuffer], /* board address of beginning of OS  */
                                  gc->strideInTiles );        /* distance between scanlines of the OS, in*/
    _grReCacheFifo(0);
#if GLIDE_CHECK_CONTEXT
    /*
    ** initialize context checking
    */
    {
      gc->lostContext = &lostcontext_csim;
      *gc->lostContext = FXFALSE;
      gc->contextP = 1;
    }
#endif /* GLIDE_CHECK_CONTEXT */

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
     * start of the fifo.
     */
    gcFifo->fifoPtr  = gcFifo->fifoStart;
    gcFifo->fifoRead = HW_FIFO_PTR( FXTRUE );
#endif /* USE_PACKET_FIFO */
    
#if !DRI_BUILD
    if ( (void*)gcFifo->fifoPtr != (void*)gcFifo->fifoRead ) {
#ifdef GLIDE_INIT_HWC
      hwcRestoreVideo( bInfo );
#endif
      GDBG_INFO( gc->myLevel, "Initial fifo state is incorrect\n" );
      return 0;
    }
#endif	/* DRI_BUILD */
    
#if __POWERPC__ && PCI_BUMP_N_GRIND
    enableCopyBackCache((FxU32)gcFifo->fifoStart,gcFifo->fifoSize);
#endif
    
    if (!gc->cmdTransportInfo.autoBump) {
      gcFifo->bumpSize = _GlideRoot.environment.bumpSize;

      gcFifo->lastBump = gcFifo->fifoPtr;
      gcFifo->bumpPos = gcFifo->fifoPtr + gcFifo->bumpSize;
#if __POWERPC__ && PCI_BUMP_N_GRIND
      gcFifo->fifoJmpHdr[0] = 
        ( SSTCP_PKT0_JMP_LOCAL |
          ( gcFifo->fifoOffset << ( SSTCP_PKT0_ADDR_SHIFT - 2 )));
#else
      gcFifo->fifoJmpHdr[0] = 
        ( SSTCP_PKT0_JMP_AGP |
          ( gcFifo->fifoOffset << ( SSTCP_PKT0_ADDR_SHIFT - 2 )));
      gcFifo->fifoJmpHdr[1] = (gcFifo->fifoOffset >> 25);
#endif

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
    
#ifdef DRI_BUILD
    _grImportFifo(*(int *)driInfo.fifoPtr, *(int *)driInfo.fifoRead);
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
    REG_GROUP_SET(hw, leftOverlayBuf, gc->buffers0[gc->frontBuffer]);
    REG_GROUP_END();
  
    REG_GROUP_BEGIN(BROADCAST_ID, swapbufferCMD, 1, 0x1); 
    REG_GROUP_SET(hw, swapbufferCMD, 0x0);
    REG_GROUP_END();

    gc->state.shadow.colBufferAddr = gc->buffers0[gc->curBuffer];
    gc->state.shadow.colBufferStride = gc->strideInTiles | SST_BUFFER_MEMORY_TILED;
    gc->state.shadow.auxBufferAddr = gc->buffers0[nColBuffers];
    gc->state.shadow.auxBufferStride = gc->strideInTiles | SST_BUFFER_MEMORY_TILED;
  
#ifdef GLIDE_INIT_HWC
    if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
      _grChipMask( gc->chipmask );
    }

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
    if (gc->grPixelSample > 1) {
        
      if(gc->enableSecondaryBuffer) {
        REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
        {
            REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->curBuffer] | SST_BUFFER_BASE_SELECT);
#ifdef DRI_BUILD
	    REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? (FxU32)driInfo.stride : 
			  gc->state.shadow.colBufferStride );
#else
	    REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif



            REG_GROUP_SET(hw, auxBufferAddr, gc->buffers1[nColBuffers] | SST_BUFFER_BASE_SELECT);
            REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride); 
        }
        REG_GROUP_END();
      }
      /*
      ** setup aaCtrl register
      */
      {
        _grAAOffsetValue(_GlideRoot.environment.aaXOffset[gc->sampleOffsetIndex],
                         _GlideRoot.environment.aaYOffset[gc->sampleOffsetIndex],
                         0, gc->chipCount - 1, FXTRUE, gc->enableSecondaryBuffer) ;
      }
    }
    
    if (gc->sliCount > 1) {
      _grEnableSliCtrl();
    }

#else
#ifdef HAL_CSIM
#if 1
    {
      if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
        _grChipMask( gc->chipmask );
      }
      {
        /*
        ** initialize pci registers for sli/aa
        */
        FxI32 chipIndex;
        for(chipIndex=0; (FxU32)chipIndex<gc->chipCount; chipIndex++) {
          halCfgStore32(0x3FC, 1, gc->halInfo->boardInfo[chipIndex].pciBusNumber,
                        gc->halInfo->boardInfo[chipIndex].pciDeviceNumber,
                        gc->halInfo->boardInfo[chipIndex].pciFunctionNumber,
                        offsetof(SstPCIConfigRegs, cfgSliLfbCtrl),
                        SST_SLI_LFB_CPU_WRITE_ENABLE |
                        SST_SLI_LFB_DISPATCH_WRITE_ENABLE |
                        SST_SLI_LFB_READ_ENABLE);                   
          halCfgStore32(0x3FC, 1, gc->halInfo->boardInfo[chipIndex].pciBusNumber,
                        gc->halInfo->boardInfo[chipIndex].pciDeviceNumber,
                        gc->halInfo->boardInfo[chipIndex].pciFunctionNumber,
                        offsetof(SstPCIConfigRegs, cfgAADepthBufferAperture),
                        0);
          halCfgStore32(0x3FC, 1, gc->halInfo->boardInfo[chipIndex].pciBusNumber,
                        gc->halInfo->boardInfo[chipIndex].pciDeviceNumber,
                        gc->halInfo->boardInfo[chipIndex].pciFunctionNumber,
                        offsetof(SstPCIConfigRegs, cfgAALfbCtrl),
                        SST_AA_LFB_CPU_WRITE_ENABLE |
                        SST_AA_LFB_DISPATCH_WRITE_ENABLE |
                        SST_AA_LFB_READ_ENABLE);
        }
      }
      gc->state.shadow.colBufferAddr = gc->buffers0[gc->curBuffer];
      gc->state.shadow.colBufferStride = gc->state.screen_width * bInfo->h3pixelSize;
      gc->state.shadow.auxBufferAddr = gc->buffers0[nColBuffers];
      gc->state.shadow.auxBufferStride = gc->state.screen_width * bInfo->h3pixelSize;
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
      if (gc->grPixelSample > 1) {
        REG_GROUP_BEGIN(BROADCAST_ID, colBufferAddr, 4, 0xf);
        {
          REG_GROUP_SET(hw, colBufferAddr, gc->buffers1[gc->curBuffer] | SST_BUFFER_BASE_SELECT);
#ifdef DRI_BUILD
          REG_GROUP_SET(hw, colBufferStride, (!gc->curBuffer) ? driInfo.stride : 
			gc->state.shadow.colBufferStride );
#else
          REG_GROUP_SET(hw, colBufferStride, gc->state.shadow.colBufferStride );
#endif
          REG_GROUP_SET(hw, auxBufferAddr, gc->buffers1[nColBuffers] | SST_BUFFER_BASE_SELECT);
          REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride); 
        }
        REG_GROUP_END();
        /*
        ** setup aaCtrl register
        */
        {
          /* Anti-aliasing default perturbation values */
//8xaa
          FxU32 defaultXOffset[8];// = {0x7a, 0x2, 0x7c, 0x4};
          FxU32 defaultYOffset[8];// = {0x7b, 0x4, 0x3, 0x7d};
          _grAAOffsetValue(defaultXOffset, defaultYOffset, 
                           0, gc->chipCount - 1, FXTRUE, FXTRUE);
        }
      }
    }

    if (gc->sliCount > 1) {
      _grEnableSliCtrl();
    }

#endif
#endif
#endif
    
    {
      /*
      ** setup default rendermode
      */
      _grRenderMode(pixelformat);
    }

    GDBG_INFO( gc->myLevel, "  Setting all Glide state\n" );
    assertDefaultState();
#ifdef DRI_BUILD
    if (nColBuffers>1)
	grRenderBuffer(GR_BUFFER_BACKBUFFER);
    else
	grRenderBuffer(GR_BUFFER_FRONTBUFFER);
    grClipWindow(0, 0, gc->state.screen_width, gc->state.screen_height);
#else	/* defined(DRI_BUILD) */
    clearBuffers( gc );
#endif	/* defined(DRI_BUILD) */
    gc->state.color_format = format;

    /* --------------------------------------------------------
       Splash Screen
       --------------------------------------------------------*/
    doSplash();

    gc->windowed = FXFALSE;
    _GlideRoot.windowsInit[_GlideRoot.current_sst]++; /* to avoid race with grSstControl() */

    retVal = (GrContext_t)gc;

    if(_GlideRoot.environment.aaClip == FXTRUE) {
      if((gc->grPixelSample > 1) && (_GlideRoot.windowsInit[_GlideRoot.current_sst] == 1)) {
        grClipWindow(0, 0, gc->state.screen_width, gc->state.screen_height);
      }
    }

    GR_END();
  }
  
  return retVal;
#undef FN_NAME
} /* grSstWinOpenExt */
#endif /* NAPALM */

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

  if (!gc)
    return FXFALSE;

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

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  /* free splashscreen DLL */
  if (gc->pluginInfo.moduleHandle) {
    if (gc->pluginInfo.shutdownProc)
      (*gc->pluginInfo.shutdownProc)();
    FreeLibrary(gc->pluginInfo.moduleHandle);
    memset(&gc->pluginInfo, 0, sizeof(gc->pluginInfo));
  }
#endif

#if !(GLIDE_PLATFORM & GLIDE_OS_UNIX) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
  /* We are in Windowed Mode */
  if (gc->windowed)
  {
    void _grReleaseWindowSurface(GrContext_t ctx);
    _grReleaseWindowSurface(context);
    gc->windowed = FXFALSE; /* init the windowed flag */
    return FXTRUE;
  }
#endif

  /* If we are OpenGL, we need to release Exclusive mode so other
  ** OpenGL fullscreen apps can run.  If not, we will cause a lot
  ** of problems.
  */
  /* KoolSmoky - HACK! We skip this for win32 because it crashes. 
   */
#if !(GLIDE_OS & GLIDE_OS_WIN32)
  if(_GlideRoot.environment.is_opengl == FXTRUE) {
	hwcRestoreVideo(gc->bInfo);
  }
#endif

  /* Make sure that the user specified gc is not whacked */
  if ((gc != NULL) &&
      (gc >= _GlideRoot.GCs) &&
      (gc <= _GlideRoot.GCs + MAX_NUM_SST)) {
    if (gc->open) {
#if GLIDE_CHECK_CONTEXT
      if (gc->lostContext && !*gc->lostContext)
#endif
      {
        grFlush();

#if GLIDE_INIT_HAL
        /* dpc - 22 may 1997 - FixMe!
         * We need the equivilant stuff in the hal layer too.
         */
#else /* !GLIDE_INIT_HAL */
        /*--------------------------
          3D Idle
          --------------------------*/
        GDBG_INFO(gc->myLevel, "  3D Idle\n");

        /*--------------------------
          Command Transport Disable
          --------------------------*/
        GDBG_INFO(gc->myLevel, "  Command Transport Disable\n");

#if __POWERPC__ && PCI_BUMP_N_GRIND
        restoreCacheSettings();
#endif

        /* Video Restore 
         *
         * NB: The hwcRestoreVideo in addition to restoring the video also
         * turns off the command fifo and then releases the hw context
         * which can unmap the board at the driver level.  The next time
         * we use grSstWinOpen we need to re-map the board etc just to be
         * safe everywhere.
         */
        GDBG_INFO(gc->myLevel, "  Restore Video\n");
#if !DRI_BUILD
        /* disable SLI and AA */
#ifdef FX_GLIDE_NAPALM
        if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
          _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
          _grTex2ppc(FXFALSE);
          if (gc->grPixelSample > 1) {
            _grAAOffsetValue(_GlideRoot.environment.aaXOffset[0],
                             _GlideRoot.environment.aaYOffset[0],
                             0, gc->chipCount - 1, FXTRUE, FXFALSE) ;
          }
          if (gc->sliCount > 1)
            _grDisableSliCtrl();

          /* Idle the 3D pipe. */
          grFinish();
        }
#endif
        hwcRestoreVideo(gc->bInfo);
#endif	/* !DRI_BUILD */
#endif /* !GLIDE_INIT_HAL */
      }
#if (GLIDE_OS & GLIDE_OS_WIN32)
      else {
        hwcResetVideo(gc->bInfo);
      }
#endif

      /*--------------------------
        GC Reset
        --------------------------*/
      GDBG_INFO(gc->myLevel, "  GC Reset\n");

      /* These are really two different things.  
       *
       * hwInitP indicates whether the init code mapping/init sequence
       * is active for this hw. 
       *
       * open includes setting up video, command transport, and the
       * initial glide state.
       */
      //gc->hwInitP = FXFALSE;

      _grDisplayStats();
    }
    gc->open = FXFALSE;
    gc->grSstRez = GR_RESOLUTION_NONE;
    gc->grSstRefresh = GR_REFRESH_NONE;
  }

  _GlideRoot.windowsInit[_GlideRoot.current_sst]--;

#if (GLIDE_OS & GLIDE_OS_WIN32)
  if (_GlideRoot.environment.is_opengl != FXTRUE) {
    /* we are using hwcUnmapMemory9x for both win9x and winNT
    ** to unmap the selected board
    */
    /*if ( hwcIsOSWin9x() ) {*/
      hwcUnmapMemory9x( gc->bInfo );
    /*} else {
      hwcUnmapMemory();
    }*/
  }
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

  /* Play safe. The hardware counter is 3 bits. */
  if (_GlideRoot.environment.swapPendingCount > 6)
    _GlideRoot.environment.swapPendingCount = 6;
  else if (_GlideRoot.environment.swapPendingCount < 0)
    _GlideRoot.environment.swapPendingCount = 0;
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
    setThreadValue( context );
    if (gc != NULL) {
      if (gc->windowed) {
        /* Setup the procs that we can do w/o any mode knowledge */
        gc->archDispatchProcs.texDownloadProcs  = _GlideRoot.deviceArchProcs.curTexProcs;
        gc->archDispatchProcs.drawTrianglesProc = _GlideRoot.deviceArchProcs.curDrawTrisProc;
        
        /* Default render procs to window space */
        gc->archDispatchProcs.coorModeTriVector = (*_GlideRoot.deviceArchProcs.curTriProcs) + GR_WINDOW_COORDS;
        gc->archDispatchProcs.drawVertexList    = _GlideRoot.deviceArchProcs.curVertexListProcs[GR_WINDOW_COORDS];

        gc->contextP = !(*gc->lostContext) ;
        gc->open = FXTRUE;

        rv = gc->contextP ;
      } else {
        const FxBool
          oldContextState = gc->contextP;

        GR_ASSERT((gc >= _GlideRoot.GCs) &&
                  (gc <= _GlideRoot.GCs + MAX_NUM_SST));

#if defined(GLIDE_INIT_HWC) && GLIDE_CHECK_CONTEXT 
        gc->contextP = !(*gc->lostContext) ;
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

  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME"()\n", 83);

  status = GR_GET(hw->status);

#ifdef FX_GLIDE_NAPALM
  /* */
  /* AJB check slave status */
  /* */
  {
    FxU32 chip ; 

    if (gc->chipCount)
      for (chip = 0 ;
           chip < gc->chipCount - 1 ;
           chip++)
        status |= GR_GET(gc->slaveSstRegs[chip]->status) ;
  }
#endif

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
    _FifoFlush();
#endif
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
  FxU32 i = 0 ;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 83);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  grFlush();
  if ( gc->windowed ) {
#if defined(GLIDE_INIT_HWC) && !(GLIDE_PLATFORM & GLIDE_OS_UNIX) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
    struct cmdTransportInfo*
      gcFifo = &gc->cmdTransportInfo;
    
    hwcIdleWinFifo(gc->bInfo,
                   &gcFifo->hwcFifoInfo,
                   gcFifo->issuedSerialNumber);
#endif	/* defined(GLIDE_INIT_HWC) && !(GLIDE_PLATFORM & GLIDE_OS_UNIX) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32) */
  } else {
    /*while((_grSstStatus() & SST_BUSY) != 0) */
      /* Do Nothing */; 
    /*
     * Napalm should be read as idle three times
     * before we believe it.
     */
    do {
      if(_grSstStatus() & SST_BUSY)
        i = 0; /* Reset counter */
      else
        i++;
    } while(i < 3);
/*
    while (_grSstStatus() & SST_BUSY) ;
    while (((_grSstStatus() & SST_BUSY) == 0) &&
            (++i < 3)) ;
 */
  }

  GR_END();
#undef FN_NAME
} /* grSstIdle */

/*---------------------------------------------------------------------------
**  grSstIsBusy - find out if the SST is busy or not
*/

FxBool FX_CSTYLE
_grSstIsBusy(void)
{
#define FN_NAME "grSstIsBusy"
  static FxBool nopP = FXTRUE;
  FxBool idle;
  FxU32  i = 0 ;
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
#ifdef GLIDE_INIT_HWC
    _FifoFlush();
#endif
    idle = 1;
  } else {
    /*
     * Napalm must be read idle three times for us
     * to believe it.
     */
    while ((idle = ((_grSstStatus() & SST_BUSY) == 0)) &&
           (++i < 3));
    idle = !idle;
  }

  nopP = idle;

  GDBG_INFO(84,"grSstIsBusy() => 0x%x\n", !idle);

  return !idle;
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
  /*
  ** Hack for Fifa99!
  ** Fifa99 calls this routine after they use grGlideShutdown.
  ** The game crashes here with a null gc.
  */
  if (_GlideRoot.environment.useAppGamma)
  {
  	if (gc)
    	  hwcGammaRGB(gc->bInfo, r, g, b);
  }
  else
  {
	GDBG_INFO(69,"guGammaCorrectionRGB::hwcGammaRGB (%3.3f, %3.3f, %3.3f)  call ignored\n", r,g,b);
  }

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
  if (_GlideRoot.environment.useAppGamma)
   	hwcGammaTable(gc->bInfo, nentries, red, green, blue);  
  else
  {
	GDBG_INFO(69, "grLoadGammaTable::hwcGammaRGB call ignored\n");
  }
#endif

  GR_END();
#undef FN_NAME
}

/*-------------------------------------------------------------------
  Function: grGetGammaTable
  Date: 04-Jan-00
  Implementor(s): chuck
  Description:

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_EXT_ENTRY(grGetGammaTable, void, (FxU32 nentries, FxU32 *red, FxU32 *green, FxU32 *blue))
{
#define FN_NAME "grGetGammaTable"
  FxU32 max;

  GR_BEGIN_NOFIFOCHECK("grGetGammaTable",80);
  
  grGet(GR_GAMMA_TABLE_ENTRIES, 4, (FxI32 *)&max);
  if (nentries > max)
    nentries = max;

#ifdef GLIDE_INIT_HWC
  hwcGetGammaTable(gc->bInfo, nentries, red, green, blue);  
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

 if (_GlideRoot.environment.useAppGamma)
   	hwcGamma(gc->bInfo, gamma, gamma, gamma);
 else
	GDBG_INFO(69,"grGammaCorrectionValue::hwcGammaRGB (gamma = %3.3f) call ignored\n", gamma);
 
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

#ifdef FX_GLIDE_NAPALM
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID))
  {
    FxU32 renderMode = gc->state.shadow.renderMode;
    renderMode &= ~(SST_RM_YORIGIN_TOP);
    renderMode |= SST_RM_YORIGIN_SELECT;
    if (origin == GR_ORIGIN_LOWER_LEFT)
    {
      /* AJB - According to the Napalm spec this is correct, but the winsim seems to perform the yorigin */
      /* subtraction BEFORE the SLI munging */
     renderMode |= (((gc->state.screen_height / gc->sliCount) - 1) << SST_RM_YORIGIN_TOP_SHIFT); 
     /* renderMode |= ((gc->state.screen_height - 1) << SST_RM_YORIGIN_TOP_SHIFT); */
    }
    REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 0x1, 0x1);
    REG_GROUP_SET(hw, renderMode, renderMode);
    REG_GROUP_END();
    gc->state.shadow.renderMode = renderMode;

    if (gc->sliCount > 1) 
      _grEnableSliCtrl() ;
    
  }
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
 * dpc: Its to get rid of the compiler warning for a function that
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

#ifdef FX_GLIDE_NAPALM
/*
**  _grSstSetColumnsOfNWidth
**
**  Sets the 'columns of N' width (fka columns of 8).
**  
*/
static void
_grSstSetColumnsOfNWidth(FxU32 width)
{
#define FN_NAME "_grSstSetColumnsOfNWidth"
  FxU32
    bits,                       /* Bits for the column band control field */
    fbzColorPath;               /* local shadow of same */
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 80);

  if (_GlideRoot.environment.columnWidth)
    width = _GlideRoot.environment.columnWidth;

  GDBG_INFO_MORE(gc->myLevel, "(%d)\n", width);

  /* Validate Argument
  **  
  **  There might be some effed-up algorithm for this, but it escapes
  **  me at this time.  It's not like this is a performance-critical
  **  routine, anyway.
  */
  switch (width) {
  case 32:
    bits = 0x2;
    break;  /* KoolSmoky - don't we need break here? */
  case 16:
    bits = 0x1;
    break;
  case 8:
    bits = 0x0;
    break;
  case 4:
    bits = 0x3;
    break;

  default:
    GDBG_INFO(80, FN_NAME ":  Invalid argument %d.  Falling back to default\n",
              width);
    width = 8;
    bits = 0x0;
    break;
  }

  fbzColorPath = gc->state.shadow.fbzColorPath;

  fbzColorPath &= ~SST_TRIANGLE_ITERATOR_COLUMN_BAND_CONTROL;

  fbzColorPath |= (bits << SST_TRIANGLE_ITERATOR_COLUMN_BAND_CONTROL_SHIFT);

  REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, 0x1, 0x1);
  REG_GROUP_SET(hw, fbzColorPath, fbzColorPath);
  REG_GROUP_END();

  gc->state.shadow.fbzColorPath = fbzColorPath;

#undef FN_NAME
} /* _grSstSetColumnsOfNWidth */

/*---------------------------------------------------------------------------
** _grChipMask
*/

void
_grChipMask(FxU32 mask)
{
#define FN_NAME "_grChipMask"  
  GR_BEGIN_NOFIFOCHECK("_grChipMask", 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", mask);
  
  /* Optimized out redundant changes since changing the chipMask
   * field causes a pipeline flush! */
  if(mask != gc->state.shadow.chipMask) {
    REG_GROUP_BEGIN(BROADCAST_ID, chipMask, 1, 0x1); 
    REG_GROUP_SET(hw, chipMask, mask);
    REG_GROUP_END();

    gc->state.shadow.chipMask = mask;
  }

#undef FN_NAME
} /* _grChipMask */


/*---------------------------------------------------------------------------
** _grAAOffsetValue
** chipid is 0 based
*/
void
_grAAOffsetValue(FxU32 *xOffset, 
                 FxU32 *yOffset, 
                 FxU32 minchipid,
                 FxU32 maxchipid,
                 FxBool enablePrimary,
                 FxBool enableSecondary)
{
#define FN_NAME "_grAAOffsetValue"  
  FxU32 chipIndex, aaCtrl;
  GR_BEGIN_NOFIFOCHECK("_grAAOffsetValue", 85);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x,0x%x,%d,%d,%d,%d)\n", xOffset, yOffset,
                 minchipid, maxchipid, enablePrimary, enableSecondary);
  
  for (chipIndex = minchipid; chipIndex <= maxchipid; chipIndex++) {
  
    _grChipMask( 1 << chipIndex);

//8xaa
    aaCtrl = (xOffset[(chipIndex * 2)%8] << SST_AA_CONTROL_PRIMARY_X_OFFSET_SHIFT) |
             (yOffset[(chipIndex * 2)%8] << SST_AA_CONTROL_PRIMARY_Y_OFFSET_SHIFT) |
             (xOffset[(chipIndex * 2 + 1)%8] << SST_AA_CONTROL_SECONDARY_X_OFFSET_SHIFT) |
             (yOffset[(chipIndex * 2 + 1)%8] << SST_AA_CONTROL_SECONDARY_Y_OFFSET_SHIFT) |
             ((enableSecondary) ? SST_AA_CONTROL_AA_ENABLE : 0) |
             ((enablePrimary) ? 0 : SST_AA_CONTROL_AA_DISABLE_FIRST);

    REG_GROUP_BEGIN(BROADCAST_ID, aaCtrl, 1, 0x1);
    REG_GROUP_SET(hw, aaCtrl, aaCtrl);
    REG_GROUP_END();

  }
  
  _grChipMask( gc->chipmask );

  /* Force fogMode to be updated */
  INVALIDATE(fogMode);

#undef FN_NAME
} /* _grAAOffsetValue */

/*---------------------------------------------------------------------------
** _grEnableSliCtrl
*/

void
_grEnableSliCtrl(void)
{
#define FN_NAME "_grEnableSliCtrl"  
  FxU32 chipIndex;
  FxI32 sliChipCountDivisor;
  FxU32 renderMask;
  FxU32 scanMask;
  FxU32 log2chipCount;
  GR_BEGIN_NOFIFOCHECK("_grEnableSliCtrl", 85);
  /*
  ** enable sli mode
  */
  
//8xaa
  if( gc-> chipCount == 2 )
  	sliChipCountDivisor = (gc->grPixelSample == 4) ? 2 : 1;

  else if( gc-> chipCount == 4 )
	sliChipCountDivisor = (gc->grPixelSample == 2) ? 2 : 1;

  else
	sliChipCountDivisor = 0; /* should never happen will cause div by 0 */

  renderMask = (gc->chipCount / sliChipCountDivisor - 1) << gc->sliBandHeight;
  scanMask = (1 << gc->sliBandHeight) - 1;
  log2chipCount = 0;
  
  while (( 0x1UL << log2chipCount ) != (gc->chipCount / sliChipCountDivisor))
    log2chipCount++;
  
  for (chipIndex = 0; chipIndex < gc->chipCount; chipIndex++) 
  {
    FxU32 compareMask ;
    FxU32 sliCtrl ;

    /* AJB- When YORIGIN swapping we have to swap the compareMask too--
       Otherwise line ownership gets hosed. 'course winsim behaves 
      completely differently, so this might need some fixing on 
      real hardware.
    */
    if (gc->state.shadow.fbzMode & SST_YORIGIN)
     compareMask = ((gc->chipCount - chipIndex - 1) / sliChipCountDivisor) << gc->sliBandHeight;
    else
     compareMask = (chipIndex / sliChipCountDivisor) << gc->sliBandHeight;

    sliCtrl = 
      ( (renderMask << SST_SLI_CONTROL_RENDER_MASK_SHIFT) |
        (compareMask << SST_SLI_CONTROL_COMPARE_MASK_SHIFT) |
        (scanMask << SST_SLI_CONTROL_SCAN_MASK_SHIFT) |
        (log2chipCount << SST_SLI_CONTROL_LOG2_CHIP_COUNT_SHIFT) |
        SST_SLI_CONTROL_SLI_ENABLE);
    
    _grChipMask( 1 << chipIndex );
    
    REG_GROUP_BEGIN(BROADCAST_ID, sliCtrl, 1, 0x1);
    REG_GROUP_SET(hw, sliCtrl, sliCtrl);
    REG_GROUP_END();
    
  }
  
  _grChipMask( gc->chipmask );
  
#undef FN_NAME
} /* _grEnableSliCtrl */

/*---------------------------------------------------------------------------
** _grDisableSliCtrl
*/

void
_grDisableSliCtrl(void)
{
#define FN_NAME "_grDisableSliCtrl"  
  FxU32 chipIndex;
  GR_BEGIN_NOFIFOCHECK("_grDisableSliCtrl", 85);
  /*
  ** disable sli mode
  */
  for (chipIndex = 0; chipIndex < gc->chipCount; chipIndex++) {
    FxU32 sliCtrl = 0;    
    _grChipMask( 1 << chipIndex );
    REG_GROUP_BEGIN(BROADCAST_ID, sliCtrl, 1, 0x1);
    REG_GROUP_SET(hw, sliCtrl, sliCtrl);
    REG_GROUP_END();
  }
  _grChipMask( gc->chipmask );
  
#undef FN_NAME
} /* _grDisableSliCtrl */

/*---------------------------------------------------------------------------
** _grRenderMode
*/

void
_grRenderMode(FxU32 pixelformat)
{
#define FN_NAME "_grRenderMode"  
  FxU32 renderMode;
  GR_BEGIN_NOFIFOCHECK("_grRenderMode", 85);

  /*
  ** setup default rendermode
  */
  renderMode = gc->state.shadow.renderMode;
  renderMode &= ~(SST_RM_3D_MODE);
  if (_GlideRoot.environment.guardbandclipping) {
    renderMode |= SST_RM_ENGUARDBAND;
  }

  /* Just set this once, since this is an expensive register to write. */
  renderMode &= ~(SST_RM_TWO_PIXELS_PER_CLOCK_BAND_SELECTION);
  renderMode |= (_GlideRoot.environment.band2ppc << SST_RM_TWO_PIXELS_PER_CLOCK_BAND_SELECTION_SHIFT);

  /*
   *          XXX Memo to Myself XXX
   * For 4 chip SLI, do different stuff with the
   * dither rotation bit. Touch the puppet head.
   */
  switch (pixelformat) {
  case GR_PIXFMT_AA_2_ARGB_1555:
  case GR_PIXFMT_AA_4_ARGB_1555:
  case GR_PIXFMT_AA_8_ARGB_1555:	/* 8xaa */
    renderMode |= SST_RM_DITHER_ROTATION ;
  case GR_PIXFMT_ARGB_1555:
    renderMode |= SST_RM_15BPP;
    break;
  case GR_PIXFMT_ARGB_8888:
  case GR_PIXFMT_AA_2_ARGB_8888:
  case GR_PIXFMT_AA_4_ARGB_8888:
  case GR_PIXFMT_AA_8_ARGB_8888:	/* 8xaa */
    renderMode |= SST_RM_32BPP;
    grColorMaskExt(gc->state.stateArgs.grColorMaskExtArgs.r,
                   gc->state.stateArgs.grColorMaskExtArgs.g,
                   gc->state.stateArgs.grColorMaskExtArgs.b,
                   gc->state.stateArgs.grColorMaskExtArgs.a);
    break;
  case GR_PIXFMT_AA_2_RGB_565:
  case GR_PIXFMT_AA_4_RGB_565:
  case GR_PIXFMT_AA_8_RGB_565:		/* 8xaa */
    renderMode |= SST_RM_DITHER_ROTATION ;
  default:
    renderMode |= SST_RM_16BPP;
    break;
  }

  REG_GROUP_BEGIN(BROADCAST_ID, renderMode, 0x1, 0x1);
  REG_GROUP_SET(hw, renderMode, renderMode);
  REG_GROUP_END();
  gc->state.shadow.renderMode = renderMode;

#undef FN_NAME
} /* _grRenderMode */

#endif /* FX_GLIDE_NAPALM */
