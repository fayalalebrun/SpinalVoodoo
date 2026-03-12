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
** Revision 1.2.2.5  2005/06/09 18:32:29  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.2.2.4  2005/05/25 08:56:23  jwrdegoede
** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
**
** Revision 1.2.2.3  2005/05/25 08:51:50  jwrdegoede
** Add #ifdef GL_X86 around x86 specific code
**
** Revision 1.2.2.2  2004/10/08 06:22:10  dborca
** use whatever swap interval user decides
**
** Revision 1.2.2.1  2004/02/16 07:42:16  dborca
** grSetNumPendingBuffers visible with grGetProcAddress
**
** Revision 1.2  2001/03/05 15:02:18  alanh
** Set FX_GLIDE_SWAPINTERVAL to 0 by default
**
** Revision 1.1.1.1  1999/11/24 21:44:57  joseph
** Initial checkin for SourceForge
**
** 
** 10    5/21/99 12:48p Stb_gkincade
** Syntax correction
** 
** 9     5/21/99 9:54a Stb_gkincade
** Put in check for : is GETENV(SSTH3_ALPHADITHERMODE) a null string?
** 
** 8     5/20/99 9:01a Atai
** is GETENV("SSTH3_ALPHADITHERMODE")  a null string?
** 
** 7     5/12/99 10:37p Stb_gkincade
** Adding user support for turning on/off dither substraction
** 
** 6     4/22/99 4:03p Dow
** Alt-Tab on NT
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
** Fixed Madden 99 hack which broke grGet(GR_NUM_BOARDS,,)
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
** Hack for Madden 99. Setup up triangle and vertex array routine
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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if GLIDE3
#define kRevisionOffset 0x00UL
#else
#define kRevisionOffset 0x100UL
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
#if GLIDE_USE_C_TRISETUP
static GrVertexListProc _vertexListProcs[][2] = {
  { _grDrawVertexList, _grDrawVertexList },
#if GL_AMD3D
  { _grDrawVertexList, _grDrawVertexList },
#endif /* GL_AMD3D */
  { _grDrawVertexList_null, _grDrawVertexList_null },
};
#else /* Use asm code */
static GrVertexListProc _vertexListProcs[][2] = {
  { _drawvertexlist, _vpdrawvertexlist },
#if GL_AMD3D
  { _grDrawVertexList_3DNow_Window, _grDrawVertexList_3DNow_Clip },
#endif /* GL_AMD3D */
  { _grDrawVertexList_null, _grDrawVertexList_null },
};
#endif /* Use asm code */

static GrTexDownloadProc _texDownloadProcs[][2][4] = 
{
  /* Default Procs */
  { 
    {
      _grTexDownload_Default_8_1, 
      _grTexDownload_Default_8_2,
      _grTexDownload_Default_8_4,
      _grTexDownload_Default_8_WideS
    }, 
    {
      _grTexDownload_Default_16_1,
      _grTexDownload_Default_16_2,
      _grTexDownload_Default_16_WideS,
      _grTexDownload_Default_16_WideS
    }
  },
#if GL_AMD3D
  { 
    { 
      _grTexDownload_Default_8_1, 
      _grTexDownload_Default_8_2, 
      _grTexDownload_Default_8_4, 
      _grTexDownload_3DNow_MMX, 
    },
    {
      _grTexDownload_Default_16_1,
      _grTexDownload_Default_16_2,
      _grTexDownload_3DNow_MMX,
      _grTexDownload_3DNow_MMX,
    },
  },
#endif /* GL_AMD3D */
  /* NULL download procs */
  { 
    {
      _grTexDownload_null, 
      _grTexDownload_null,
      _grTexDownload_null,
      _grTexDownload_null
    }, 
    {
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

#if !defined(GLIDE_INIT_HAL)
#ifdef GETENV 
#undef GETENV
#endif
#define GETENV(a) hwcGetenv(a)
#endif


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

  /* The first time through the init code we need to map 
   * all of the boards. Future calls can just grab this
   * info out of the halInfo that we have here.
   */
    FxU32     device;
    const HalInfo* halInfo = fxHalInit(0);
    if (halInfo == NULL) goto __errExit;
    
    for(ctx = device = 0; device < count; device++) {
      /* See RSI-prevention macros for usage of [ctx] */
      const FxDeviceInfo* curDev = NULL;
      FxBool regInitP = FXFALSE;
      SstRegs* devRegs;

      devRegs = fxHalMapBoard(device);
      curDev = halInfo->boardInfo + device;

      if (devRegs != NULL) {
        FxU32 tmuMem = 0x00;

        SST.type = GR_SSTTYPE_Banshee;

        if (!fxHalInitRegisters(curDev->virtAddr[0])) goto __errRegFailure;
          
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
        //GC.rawLfb = (FxU32 *) SST_LFB_ADDRESS(devRegs);

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

    if ((hInfo = hwcInit(0x121a, 0x5)) == NULL) { /* Voodoo3 */
      if ((hInfo = hwcInit(0x121a, 0x3)) == NULL) { /* Banshee */
	goto __errExit;  /* Neither */
      }
    }

    /* Iterate through boards found */
    for (ctx = 0; ctx < hInfo->nBoards; ctx++) {
      bInfo = &hInfo->boardInfo[ctx];

      GC.bInfo = bInfo;

      if (bInfo->pciInfo.deviceID == 3)
        SST.type = GR_SSTTYPE_Banshee;
      else 
        SST.type = GR_SSTTYPE_Voodoo3;

      if (!hwcMapBoard(bInfo, HWC_BASE_ADDR_MASK)) {
        GrErrorCallback(hwcGetErrorString(), FXTRUE);
      }

      if (!hwcInitRegisters(bInfo)) {
        GrErrorCallback(hwcGetErrorString(), FXTRUE);
      }

      /* NB: We cannot fail to map this board after this point */
      GC.hwInitP = FXTRUE;

      _GlideRoot.hwConfig.num_sst++;

      GC.sstRegs = (SstRegs *) bInfo->regInfo.sstBase;
      GC.ioRegs = (SstIORegs *) bInfo->regInfo.ioMemBase;
      GC.cRegs = (SstCRegs *) bInfo->regInfo.cmdAGPBase;
      GC.lfb_ptr = (FxU32 *) bInfo->regInfo.lfbBase;
      GC.rawLfb = (FxU32 *) bInfo->regInfo.rawLfbBase;

      /* Video Parameters */
      GC.grSstRez = GR_RESOLUTION_NONE;
      GC.grSstRefresh = 0L;

      /* 
      ** Hack for Madden 99. Setup up triangle and vertex array routine
      */
      if (_GlideRoot.deviceArchProcs.curTriProcs) {
        GC.archDispatchProcs.coorModeTriVector = (*_GlideRoot.deviceArchProcs.curTriProcs) + 0;
        GC.archDispatchProcs.drawVertexList    = _GlideRoot.deviceArchProcs.curVertexListProcs[0];
      }

      /* This is a uma device */
      switch (hInfo->boardInfo[ctx].pciInfo.deviceID) {
      case 3: /* Banshee */
        GC.num_tmu = 1;
        GC.fbuf_size = (bInfo->h3Mem - 2);
        break;
      case 4: /* Avenger low speed */
      case 5: /* Avenger high speed */
        /*
        ** For 8M board, we may only use one tmu for higher resolution.
        ** Need to re-visit the issue. 11/5/98
        */ 
        GC.num_tmu = 2;
        GC.fbuf_size = (bInfo->h3Mem - 4);
        break;
      default:
        GC.num_tmu = 1;
        GC.fbuf_size = (bInfo->h3Mem - 2);
        break;
      }
      if (bInfo->h3Mem == 4) {
        GC.num_tmu = 1;
        GC.fbuf_size = (bInfo->h3Mem - 2);
      }
      if (GETENV("FX_GLIDE_NUM_TMU")) {
        int num_tmu = atoi(GETENV("FX_GLIDE_NUM_TMU"));
        switch (num_tmu) {
        case 1:
          GC.num_tmu = 1;
          GC.fbuf_size = (bInfo->h3Mem - 2);
          break;
        case 2:
          GC.num_tmu = 2;
          GC.fbuf_size = (bInfo->h3Mem - 4);
          break;
        }         
      }

      GC.state.grEnableArgs.texture_uma_mode = GR_MODE_DISABLE;

      SST.sstBoard.SST96Config.fbRam    = GC.fbuf_size;
      SST.sstBoard.SST96Config.nTexelfx = GC.num_tmu;
      
      SST.sstBoard.SST96Config.tmuConfig.tmuRev = (2 + kRevisionOffset);              
      SST.sstBoard.SST96Config.tmuConfig.tmuRam = 0x0;

      /* Clear the tmu state */
      for (tmu = 0; tmu < GC.num_tmu; tmu++) {
        memset(&GC.tmu_state[0], 0, sizeof(GC.tmu_state[0]));       
        GC.tmu_state[0].total_mem = (0x2 << 20);
        GC.tmu_state[0].ncc_mmids[0] = GC.tmu_state[0].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;
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
  } else {
    GDBG_INFO(80,"error: SSTs %d: unknown type\n",i);
  }
} /* displayBoardInfo */

void
_GlideInitEnvironment(void)
{
#define FN_NAME "_GlideInitEnvironment"
  int i;
  FxU32 ditherMode;
  const char* envStr;
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  OSVERSIONINFO ovi;
#endif  
  if (_GlideRoot.initialized)           /* only execute once */
    return;
  
  GDBG_INIT();                          /* init the GDEBUG libraray */
  GDBG_INFO(80,"%s()\n", FN_NAME);
  GDBG_INFO(0,"GLIDE DEBUG LIBRARY\n"); /* unconditional display */
  
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  ovi.dwOSVersionInfoSize = sizeof ( ovi );
  GetVersionEx ( &ovi );
  if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT)
    _GlideRoot.OSWin95 = 0;
  else
    _GlideRoot.OSWin95 = 1;
  GDBG_INFO(80, "%s:  OS = %s\n", FN_NAME, _GlideRoot.OSWin95 ? "W9X" : "WNT");

#if defined(FX_DLL_ENABLE) && (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  { /* GMT: display the DLL pathname for sanity checking */
    char buf[132] = "failed";
    GetModuleFileName(GetModuleHandle("glide3x.dll"), buf, sizeof(buf));
    GDBG_INFO(0,"DLL path: %s\n",buf);
  }
#endif
#endif
  
  /* Check for user environment tweaks */
#define GLIDE_GETENV(__envVar, __defVal) \
  (((envStr = GETENV(__envVar)) == NULL) ? (__defVal) : atol(envStr));
#define GLIDE_FGETENV(__envVar, __defVal) \
  (((envStr = GETENV(__envVar)) == NULL) ? (__defVal) : (float) atof(envStr));
    
    _GlideRoot.environment.triBoundsCheck    = (GETENV("FX_GLIDE_BOUNDS_CHECK") != NULL);
  GDBG_INFO(80,"    triBoundsCheck: %d\n",_GlideRoot.environment.triBoundsCheck);
#ifdef GLIDE_SPLASH
  _GlideRoot.environment.noSplash          = (GETENV("FX_GLIDE_NO_SPLASH") != NULL);
#else
  _GlideRoot.environment.noSplash          = 1;
#endif
  GDBG_INFO(80,"          noSplash: %d\n",_GlideRoot.environment.noSplash);
#ifdef GLIDE_PLUG
  _GlideRoot.environment.shamelessPlug     = (GETENV("FX_GLIDE_SHAMELESS_PLUG") != NULL);
#else
  _GlideRoot.environment.shamelessPlug     = 0;
#endif
  GDBG_INFO(80,"     shamelessPlug: %d\n",_GlideRoot.environment.shamelessPlug);
  _GlideRoot.environment.ignoreReopen      = (GETENV("FX_GLIDE_IGNORE_REOPEN") != NULL);
  GDBG_INFO(80,"      ignoreReopen: %d\n",_GlideRoot.environment.ignoreReopen);
  _GlideRoot.environment.disableDitherSub  = (GETENV("FX_GLIDE_NO_DITHER_SUB") != NULL);
  GDBG_INFO(80,"  disableDitherSub: %d\n",_GlideRoot.environment.disableDitherSub);
  
  /****************************************************************** 
   * 5/4/99 gregk
   * Adding user support for turning on/off dither substraction
   * According to Alpha Blending Quality Tab of control panel choices   
   * Optimal/Sharper -> disable dither subtraction    
   * Smoother        -> enable  dither subtraction                     
   ******************************************************************/  
   ditherMode = GLIDE_GETENV("SSTH3_ALPHADITHERMODE", 1L);
   switch(ditherMode)  
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
 
  
  _GlideRoot.environment.texLodDither      = ((GETENV("FX_GLIDE_LOD_DITHER") == NULL) ? 0x00UL : SST_TLODDITHER);
  GDBG_INFO(80,"      texLodDither: %d\n",_GlideRoot.environment.texLodDither);
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

  _GlideRoot.environment.swapPendingCount  = 3;

  _GlideRoot.environment.gammaR = GLIDE_FGETENV("SSTH3_RGAMMA", -1.f);
  _GlideRoot.environment.gammaG = GLIDE_FGETENV("SSTH3_GGAMMA", -1.f);
  _GlideRoot.environment.gammaB = GLIDE_FGETENV("SSTH3_BGAMMA", -1.f);

#ifdef GL_X86
  _GlideRoot.CPUType                       = GLIDE_GETENV("FX_CPU", _cpu_detect_asm() );    
  GDBG_INFO(0,"               cpu: 0x%x\n",_GlideRoot.CPUType);
#endif

  /* Setup the basic proc tables based on the cpu type. */
  {
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

#if GL_AMD3D
    /* Check for vendor specific optimization cases */
    switch((_GlideRoot.CPUType & 0xFFFF0000UL) >> 16UL) {
    case kCPUVendorIntel:
      break;

    case kCPUVendorAMD:
    case kCPUVendorCyrix:
    case kCPUVendorIDT:
      if ((_GlideRoot.CPUType & 0x2L) == 0x2UL) {  /* MMX and 3DNow! feature bits set */
        _GlideRoot.deviceArchProcs.curTriProcs        = _triSetupProcs + 1;
        _GlideRoot.deviceArchProcs.curDrawTrisProc    = _grDrawTriangles_3DNow;
        _GlideRoot.deviceArchProcs.curVertexListProcs = _vertexListProcs[1];

        _GlideRoot.deviceArchProcs.curTexProcs = _texDownloadProcs + 1;
      }
      break;

    case kCPUVendorUnknown:
    default:
      break;
    }
#endif /* GL_AMD3D */
  }
#if __POWERPC__ && PCI_BUMP_N_GRIND
  _GlideRoot.environment.autoBump = FXFALSE;
#else  
  _GlideRoot.environment.autoBump = (GETENV("FX_GLIDE_BUMP") == NULL);
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

  /* Pre-convert the bump size from bytes to words */
  _GlideRoot.environment.bumpSize >>= 2UL;
  GDBG_INFO(80, "\tbumpSize:          0x%x\n", _GlideRoot.environment.bumpSize);

  /* constant pool */
  _GlideRoot.pool.f0   =   0.0F;
  _GlideRoot.pool.fHalf=   0.5F;
  _GlideRoot.pool.f1   =   1.0F;
  _GlideRoot.pool.f255 = 255.0F;
  
#if GLIDE_PACKED_RGB
  _GlideRoot.pool.fBiasHi = (float)(0x01 << 15);
  _GlideRoot.pool.fBiasLo = (float)(0x01 << 23);
#endif /* GLIDE_PACKED_RGB */
  
  _GlideRoot.current_sst = 0;                    /* make sure there's a valid GC */
  
  grErrorSetCallback(_grErrorDefaultCallback);
  
  if ( !_grSstDetectResources() ) {
#ifdef GLIDE_INIT_HWC
    GrErrorCallback( hwcGetErrorString(), FXTRUE );
#endif
  }
  
  for (i = 0; i < _GlideRoot.hwConfig.num_sst; i++) {
    displayBoardInfo(i, &_GlideRoot.hwConfig);
  }
  
  _grMipMapInit();
  _GlideRoot.initialized = FXTRUE;               /* save this for the end */
} /* _GlideInitEnvironment */


#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
BOOL WINAPI 
DllMain(HANDLE hInst, ULONG  ul_reason_for_call, LPVOID lpReserved) 
{
  switch( ul_reason_for_call ) {
  case DLL_PROCESS_DETACH:
    GDBG_INFO(80, "DllMain: DLL_PROCESS_DETACH\n");
    grGlideShutdown();
    hwcUnmapMemory();
    hwcClearContextData();
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
    if (_GlideRoot.initialized &&       /* scanned for hw? */
        (_GlideRoot.windowsInit > 0)) { /* outstanding fullscreen contexts? */
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
