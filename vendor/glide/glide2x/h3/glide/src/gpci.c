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
** 3     3/17/99 6:16p Dow
** Phantom Menace and other fixes.
** 
** 166   3/17/99 5:08p Peter
** removed whacky stuff now that the command fifo threshold stuff appears
** to make all happy (including the k7)
** 
** 165   3/17/99 1:37p Atai
** use grHints to enable/disable uma hack
** 
** 164   3/16/99 11:51a Atai
** Back door (set FX_GLIDE_ENABLE_UMA=1) to enable unified texture memory.
** TMUn memory size will the whole texture memory space. The offset for
** each TMU points to the start address of the memory pool.
** 
** 163   3/15/99 10:51p Dow
** Vile Hack
** 
** 162   3/13/99 9:48p Dow
** optimizations for B&G
** 
** 161   3/12/99 2:31p Dow
** Removed 3DNow for K7 (temp workaround)
** 
** 160   3/08/99 6:11p Atai
** report Voodoo3 fbi/tmu rev number as Banshee for EA games
** 
** 159   3/06/99 10:59a Atai
** fixed my ·F check-in.
** 
** 158   3/05/99 2:50p Atai
** fbi/tmu rev mods
** 
** 157   3/04/99 3:15p Atai
** mods for direct write
** 
** 156   12/09/98 2:07p Peter
** More Norbert's stuff for the other 3DNow!(tm) partners
** 
** 155   12/03/98 11:27p Dow
** Code 'cleanup' heç
** 
** 154   12/03/98 10:34p Dow
** Added GLIDE_FGETENV for floats and removed registry code
** 
** 153   12/03/98 12:37p Dow
** Fixed DOS build
** 
** 152   12/02/98 2:53p Dow
** NT/9X Registry reading fix
** 
** 151   11/19/98 9:53p Jeske
** make sure we look for Voodoo3/avenger also...
** 
** 150   11/18/98 7:59p Dow
** grxclk
** 
** 149   11/18/98 7:44p Atai
** use env var FX_GLIDE_NUM_TMU
** 
** 148   11/17/98 7:04p Atai
** added env var "FX_GLIDE_DISABLE_TMU1"
** 
** 147   11/10/98 6:44p Atai
** number of tmu and texture memory allocation
** 
** 146   11/09/98 3:32p Mikec
** 
** 143   11/05/98 1:55p Atai
** initialize 2nd tmu configs
** 
** 142   10/21/98 4:20p Atai
** gamma stuff
** 
** 141   10/21/98 10:41a Atai
** 
** 140   10/20/98 5:34p Atai
** added #ifdefs for hwc
** 
** 139   10/19/98 2:11p Peter
** ctrisetup happiness
** 
** 138   10/09/98 6:57p Peter
** dynamic 3DNow!(tm) mods
** 
** 137   9/24/98 7:40p Dow
** GL Driver Stuff
** 
** 136   9/24/98 12:01p Dow
** Turned on extra unmentionable games
** 
** 135   9/21/98 4:00p Dow
** Added to the unmentionable
** 
** 134   9/04/98 11:36a Peter
** re-open fix for nt (thanks to taco/rob/nt bob)
** 
** 133   8/30/98 11:15a Atai
** added tigerwood 99 to the game list
** 
** 132   8/27/98 6:35p Atai
** getenv FX_GLIDE_TMU_MEMSIZE
** 
** 131   8/27/98 1:58p Peter
** fill in hwConfig union"
** 
** 130   8/26/98 10:08p Atai
** return the correct reg path
** 
** 129   8/20/98 10:08a Dow
** Fix for registry GETENV stuff
** 
** 128   8/14/98 10:25a Dow
** Fixed hwConfig union effage
** 
** 127   7/24/98 2:03p Dow
** AGP Stuff
** 
** 126   7/23/98 1:17a Dow
** Bump & Grind
** 
** 125   7/18/98 7:24p Mikec
** Made win32 calls invisible to DOS compilation.
** 
** 123   7/18/98 5:13p Mikec
** EAhack done.
** 
** 122   7/14/98 2:48p Mikec
** Added Glide environment variable FX_GLIDE_EMUL_RUSH allow Banshee glide
** to report itself as Rush to the application. Set it to 1 to enable Rush
** reporting. By default Glide still reports itself as Voodoo Graphics. 
** 
** 121   7/13/98 5:32p Dow
** GETENV from registry
** 
** 120   7/09/98 11:49a Jdt
** Fix fencing for dos build
** 
** 119   7/09/98 10:19a Dow
** Registry getenv
** 
** 118   7/06/98 7:05p Jdt
** initenvironment simplified
** 
** 117   7/06/98 11:06a Mikec
** Added fbiRev offset to distinguish Banshee from Voodoo.
** Banshee check:
** If (hwconfig.SSTs[0].sstBoard.VoodooConfig.fbiRev > 0x1000)
** 
** 116   6/24/98 11:16a Dow
** Fixed DLLMAin messages
** 
** 115   6/09/98 5:04p Dow
** %$#@!
** 
** 114   6/09/98 2:39p Mikec
** 
** 113   6/04/98 6:53p Dow
** Resolutions to 1600x1200
** 
** 112   6/03/98 5:23p Dow
** Fixed DOS effage
** 
** 111   6/03/98 1:39p Dow
** dll main
** 
** 110   5/31/98 9:03a Dow
** 800x600 resolution
** 
** 109   5/22/98 2:37p Peter
** complete the lie that is glide2x on Banshee
** 
** 108   5/21/98 4:47p Dow
** Direct Register Writes Work
** 
** 107   5/18/98 3:20p Peter
** pts more resistant to changing rounding modes
** 
** 106   5/15/98 2:21p Dow
** Changed from Voodoo Rush to Voodoo
** 
** 105   5/12/98 2:42p Dow
** 
** 104   4/14/98 6:41p Peter
** Merge w/ cvg glide cleanup
** 
** 103   4/07/98 10:40p Dow
** LFB Fixes:  Round 1
** 
** 102   4/05/98 2:18p Dow
** DOS Glide Stuff
** 
** 101   4/03/98 2:11p Dow
** 
** 100   3/28/98 11:24a Dow
** itwoç
** 
** 99    3/11/98 8:28p Dow
** WinGlide
** 
** 97    2/08/98 3:08p Dow
** FIFO Works
** 
** 96    2/02/98 4:31p Dow
** IO w/o HAL now possible
** 
** 95    1/20/98 11:03a Peter
** env var to force triple buffering
 * 
 * 93    1/16/98 5:41p Peter
 * fixed sense of lod_dither
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

/* Some macros to prevent RSI */
#define GC      _GlideRoot.GCs[ctx]
#define SST     _GlideRoot.hwConfig.SSTs[ctx]

#define kRevisionOffset  0x1000


#if GLIDE_DISPATCH_SETUP
/* Collection of all of the known procs for a given system */
static GrTriSetupProc _triSetupProcs[][NUM_TRI_PROC_LISTS][2] = 
{
  /* Default Procs */
  {
    { _trisetup_Default_Default, _trisetup_Default_cull },
#if GLIDE_PACKED_RGB
    { _trisetup_Default_rgb,     _trisetup_Default_cull_rgb },
    { _trisetup_Default_argb,    _trisetup_Default_cull_argb },
#endif /* GLIDE_PACKED_RGB */
  },
#if GL_AMD3D
  /* 3DNow!(tm) Procs */
  {
    { _trisetup_3DNow_Default, _trisetup_3DNow_cull },
#if GLIDE_PACKED_RGB
    { _trisetup_3DNow_rgb,     _trisetup_3DNow_cull_rgb },
    { _trisetup_3DNow_argb,    _trisetup_3DNow_cull_argb },
#endif /* GLIDE_PACKED_RGB */
  },
#endif /* GL_AMD3D */
};
#endif /* GLIDE_DISPATCH_SETUP */

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
  }
#endif /* GL_AMD3D */
};

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


BOOL WINAPI 
DllMain(HANDLE hInst, ULONG  ul_reason_for_call, LPVOID lpReserved) 
{
  switch( ul_reason_for_call ) {
  case DLL_PROCESS_DETACH:
    GDBG_INFO(80, "DllMain: DLL_PROCESS_DETACH\n");
    grGlideShutdown();
    break;
  case DLL_PROCESS_ATTACH:
    GDBG_INFO(80, "DllMain: DLL_PROCESS_ATTACH\n");
    break;
  case DLL_THREAD_ATTACH:
    GDBG_INFO(80, "DllMain: DLL_THREAD_ATTACH\n");
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

#define REGSTR_PATH_3DFXSW              "Software\\3Dfx Interactive\\Voodoo2"
#define REGSTR_PATH_GLIDE               REGSTR_PATH_3DFXSW"\\Glide"

#if !defined(GLIDE_INIT_HAL)
#ifdef GETENV 
#undef GETENV
#endif
#define GETENV(a) hwcGetenv(a)
#endif /* !defined(GLIDE_INIT_HAL) */

#endif

/* Windows */
#define SEPARATOR '\\'
/* UNIX */
#define SEPARATOR2 '/'

/*
 * parseFilename
 *
 *      Return the file name portion of a filename/path.
 */

char *
_parseFilename(char *name)
{
    int i;

    if (name == NULL) 
      return NULL;
    for(i = strlen(name); i >= 0; i--)
        if ((name[i] == SEPARATOR) ||
            (name[i] == SEPARATOR2))
            return (name + i + 1);
    return name;
}  /* End of parseFilename*/


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

        SST.type = GR_SSTTYPE_VOODOO;

        if (!fxHalInitRegisters(curDev->virtAddr[0])) goto __errRegFailure;
          
#ifdef DIRECT_IO
        GC.sstRegs =
          (SstRegs *) (curDev->physAddr[0] + 0x200000);
        GC.ioRegs = (SstIORegs *) (curDev->physAddr[0]);
        GC.cRegs = (SstCRegs *) (curDev->physAddr[0] + 0x80000);
        GC.rawLfb = (FxU32 *) curDev->physAddr[1];
#endif

        /* This device is ready to go. */
        regInitP = FXTRUE;

        GC.reg_ptr   = (FxU32*)devRegs;

#ifdef HAL_HW
        /* Set up pointers to the various address spaces within the hw */
        GC.base_ptr  = (FxU32*)SST_BASE_ADDRESS(curDev->physAddr[0]);
        GC.lfb_ptr   = (FxU32*)SST_LFB_ADDRESS(curDev->physAddr[0]);
        GC.tex_ptr   = (FxU32*)SST_TEX_ADDRESS(curDev->physAddr[0]);
#endif        


        /* Video parameters */
        GC.grSstRez     = GR_RESOLUTION_NONE;
        GC.grSstRefresh = curDev->fbiVideoRefresh;

        GC.scanline_interleaved = FXFALSE;
        
        /* Chip configuration */
        GC.num_tmu              = curDev->numberTmus;
        GC.fbuf_size            = curDev->fbiMemSize;
        
        _GlideRoot.hwConfig.num_sst++;

        /* We claim that we are an sst1 for the sake of Avenger */
        {
          const FxU32 curTmuMemSize = 0x2;
              
#if 0
          SST.sstBoard.VoodooConfig.fbRam = 2;
#else
          SST.sstBoard.VoodooConfig.fbRam = 16;
#endif

          /* Banshee's ID is 0x1000. Always check that it's at least 0x1000
           * for Banshee card. 
           */
#if 0
          /* H4 Note: need to setup 2nd tmu configuration */
          SST.sstBoard.VoodooConfig.fbiRev = 2 + 0x1000;
          SST.sstBoard.VoodooConfig.nTexelfx = 1;

          SST.sstBoard.VoodooConfig.tmuConfig[0].tmuRev = (2 + kRevisionOffset);
              
          SST.sstBoard.VoodooConfig.tmuConfig[0].tmuRam = curTmuMemSize;
#else
          SST.sstBoard.VoodooConfig.fbiRev = curDev->fbiRevision;
          SST.sstBoard.VoodooConfig.nTexelfx = GC.num_tmu;
          SST.sstBoard.VoodooConfig.tmuConfig[0].tmuRev = curDev->tmuRevision;
          SST.sstBoard.VoodooConfig.tmuConfig[0].tmuRam = curTmuMemSize;
          SST.sstBoard.VoodooConfig.tmuConfig[1].tmuRev = curDev->tmuRevision;
              
          SST.sstBoard.VoodooConfig.tmuConfig[1].tmuRam = curTmuMemSize;
#endif
          tmuMem += curTmuMemSize;

          /* Clear the tmu state */
          memset(&GC.tmu_state[0], 0, sizeof(GC.tmu_state[0]));       
          GC.tmu_state[0].total_mem    = (curTmuMemSize << 20);              
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

    if ((hInfo = hwcInit(0x121a, 0x5)) == NULL) {    /* Voodoo3 */
      if ((hInfo = hwcInit(0x121a, 0x3)) == NULL) {  /* Banshee */
         goto __errExit; 
      }
    }


    if (_GlideRoot.environment.emulRush){
      GDBG_INFO(80,"Emulating rush\n");
    }
    
    /* Iterate through boards found */
    for (ctx = 0; ctx < hInfo->nBoards; ctx++) {
      bInfo = &hInfo->boardInfo[ctx];

      GC.bInfo = bInfo;


      GDBG_INFO(80, "Board %d, devRev: %d\n", ctx,bInfo->devRev);

      if (!hwcMapBoard(bInfo, HWC_BASE_ADDR_MASK)) {
        GrErrorCallback(hwcGetErrorString(), FXTRUE);
      }

      if (!hwcInitRegisters(bInfo)) {
        GrErrorCallback(hwcGetErrorString(), FXTRUE);
      }
      
      /* NB: We cannot fail to map this board after this point */
      GC.hwInitP = FXTRUE;

      GC.sstRegs = (SstRegs *) bInfo->regInfo.sstBase;
      GC.ioRegs = (SstIORegs *) bInfo->regInfo.ioMemBase;
      GC.cRegs = (SstCRegs *) bInfo->regInfo.cmdAGPBase;
      GC.lfb_ptr = (FxU32 *) bInfo->regInfo.lfbBase;
      GC.rawLfb = (FxU32 *) bInfo->regInfo.rawLfbBase;
      GC.tex_ptr = (FxU32*)SST_TEX_ADDRESS(bInfo->regInfo.sstBase);

      /* Video Parameters */
      GC.grSstRez = GR_RESOLUTION_NONE;
      GC.grSstRefresh = 0L;

      GC.scanline_interleaved = FXFALSE;

      switch (hInfo->boardInfo[ctx].pciInfo.deviceID) {
      case 3: /* Banshee */
        GC.num_tmu = 1;
        GC.fbuf_size = (bInfo->h3Mem - 2);
        break;
      case 4: /* Avenger low speed */
        /*
        ** For 8M board, we may only use one tmu for higher resolution.
        ** Need to re-visit the issue. 11/5/98
        */ 
        GC.num_tmu = 2;
        GC.fbuf_size = (bInfo->h3Mem - 4);
        break;
      case 5: /* Avenger high speed */
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

      _GlideRoot.hwConfig.num_sst++;

      {
        /* Default to the minimum texture memory taht we will
         * advertise for any screen resolution.  
         */
        const FxU32 curTmuMemSize = 0x2;
        int tmu;

        if (_GlideRoot.environment.emulRush) {
          SST.type = GR_SSTTYPE_SST96;
          SST.sstBoard.SST96Config.fbRam =  GC.fbuf_size;

          SST.sstBoard.SST96Config.nTexelfx = GC.num_tmu;
          SST.sstBoard.SST96Config.tmuConfig.tmuRev = (2 + kRevisionOffset);
          SST.sstBoard.SST96Config.tmuConfig.tmuRam = curTmuMemSize;
        } else {
          /* Banshee's ID is 0x1000. Always check that it's at least
           * 0x1000 for Banshee card.  
           */
          SST.type = GR_SSTTYPE_VOODOO;
          SST.sstBoard.VoodooConfig.fbRam = GC.fbuf_size;
          SST.sstBoard.VoodooConfig.fbiRev = 2 + 0x1000;
          SST.sstBoard.VoodooConfig.sliDetect = FXFALSE;

          SST.sstBoard.VoodooConfig.nTexelfx = GC.num_tmu;
          for (tmu = 0; tmu < GC.num_tmu; tmu++) {
            SST.sstBoard.VoodooConfig.tmuConfig[tmu].tmuRev = (2 + kRevisionOffset);
            SST.sstBoard.VoodooConfig.tmuConfig[tmu].tmuRam = curTmuMemSize;
          }
        }

        /* Clear the tmu state */
        for (tmu = 0; tmu < GC.num_tmu; tmu++) {
          memset(&GC.tmu_state[tmu], 0, sizeof(GC.tmu_state[tmu]));       
          GC.tmu_state[tmu].total_mem    = (curTmuMemSize << 20);              
          GC.tmu_state[tmu].ncc_mmids[0] =
            GC.tmu_state[tmu].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;
        }
      }
      if (hInfo->boardInfo[ctx].pciInfo.deviceID > 3) {
	if (_GlideRoot.environment.emulRush) {
	  SST.sstBoard.SST96Config.tmuConfig.tmuRev |= 0x10000;
	}
	else {
	  SST.sstBoard.VoodooConfig.fbiRev |= 0x10000;
	  SST.sstBoard.VoodooConfig.tmuConfig[0].tmuRev |= 0x10000;
	  SST.sstBoard.VoodooConfig.tmuConfig[1].tmuRev |= 0x10000;
	}
      }

    } /* iterate through boards found */
#endif
  }

  /* Did we previously find boards? */
  rv = (_GlideRoot.hwConfig.num_sst != 0);

  calledP = FXTRUE;

  goto __errExit; /* Keep warnings happy */
__errExit:
  ;
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
  } else {
    GDBG_INFO(80,"error: SSTs %d: unknown type\n",i);
  }
} /* displayBoardInfo */


#if defined( __WATCOMC__ )
FxU32 p6FenceVar;
#endif

void
_GlideInitEnvironment(void)
{
#define FN_NAME "_GlideInitEnvironment"
  int i;
    const char* envStr;
#if GLIDE_PLATFORM & GLIDE_OS_WIN32
  OSVERSIONINFO ovi;
#endif
  

  if (_GlideRoot.initialized)          /* only execute once */
    return;
  GDBG_INIT();                          /* init the GDEBUG libraray */
  GDBG_INFO(80,"%s()\n", FN_NAME);
  GDBG_INFO(0,"GLIDE DEBUG LIBRARY\n"); /* unconditional display */

  if (_GlideRoot.initialized)           /* only execute once */
    return;
  
#if GLIDE_PLATFORM & GLIDE_OS_WIN32
  ovi.dwOSVersionInfoSize = sizeof ( ovi );
  GetVersionEx ( &ovi );
  if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT)
    _GlideRoot.OSWin95 = 0;
  else
    _GlideRoot.OSWin95 = 1;
#endif

  GDBG_INFO(80, "%s:  OS = %s\n", FN_NAME, _GlideRoot.OSWin95 ? "W9X" : "WNT");

#if defined(FX_DLL_ENABLE) && (GLIDE_PLATFORM & GLIDE_OS_WIN32)
    { /* GMT: display the DLL pathname for sanity checking */
        char buf[132] = "failed";
        GetModuleFileName(GetModuleHandle("glide3x.dll"), buf, sizeof(buf));
        GDBG_INFO(0,"DLL path: %s\n",buf);
    }
#endif

    /* Check for user environment tweaks */
#define GLIDE_GETENV(__envVar, __defVal) \
    (((envStr = GETENV(__envVar)) == NULL) ? (__defVal) : atol(envStr));
#define GLIDE_FGETENV(__envVar, __defVal) \
    (((envStr = GETENV(__envVar)) == NULL) ? (__defVal) : (float) atof(envStr));

    _GlideRoot.environment.triBoundsCheck =
      (GETENV("FX_GLIDE_BOUNDS_CHECK") != NULL);
  GDBG_INFO(80,"\ttriBoundsCheck: %d\n",
            _GlideRoot.environment.triBoundsCheck); 
  
#ifdef GLIDE_SPLASH
  _GlideRoot.environment.noSplash =
    (GETENV("FX_GLIDE_NO_SPLASH") != NULL);
#else
  _GlideRoot.environment.noSplash = 1;
#endif
  GDBG_INFO(80,"\tnoSplash: %d\n",_GlideRoot.environment.noSplash);
  
#ifdef GLIDE_PLUG
  _GlideRoot.environment.shamelessPlug =
    (GETENV("FX_GLIDE_SHAMELESS_PLUG") != NULL);
#else
  _GlideRoot.environment.shamelessPlug = 0;
#endif
  GDBG_INFO(80,"\tshamelessPlug: %d\n",
            _GlideRoot.environment.shamelessPlug); 
  
  _GlideRoot.environment.ignoreReopen = 
    (GETENV("FX_GLIDE_IGNORE_REOPEN") != NULL);
  GDBG_INFO(80,"\tignoreReopen: %d\n", _GlideRoot.environment.ignoreReopen);
  
  _GlideRoot.environment.disableDitherSub =
    (GETENV("FX_GLIDE_NO_DITHER_SUB") != NULL);
  GDBG_INFO(80,"\tdisableDitherSub: %d\n",
            _GlideRoot.environment.disableDitherSub); 
  
  _GlideRoot.environment.texLodDither =
    ((GETENV("FX_GLIDE_LOD_DITHER") == NULL) ? 0x00UL : SST_TLODDITHER);
  GDBG_INFO(80,"\ttexLodDither: %d\n",_GlideRoot.environment.texLodDither);
  
  _GlideRoot.environment.nColorBuffer =
    GLIDE_GETENV("FX_GLIDE_ALLOC_COLOR", -1L);
  GDBG_INFO(80,"\tnColorBuffer: %d\n",_GlideRoot.environment.nColorBuffer);
  
  _GlideRoot.environment.tmuMemory =
    GLIDE_GETENV("FX_GLIDE_TMU_MEMSIZE", -1L);
  GDBG_INFO(80,"\ttmuMemory: %d\n",_GlideRoot.environment.tmuMemory);
  
  _GlideRoot.environment.nAuxBuffer =
    GLIDE_GETENV("FX_GLIDE_ALLOC_AUX", -1L);    
  GDBG_INFO(80,"\tnAuxBuffer: %d\n",_GlideRoot.environment.nAuxBuffer);
  
  _GlideRoot.environment.swFifoLWM =
    GLIDE_GETENV("FX_GLIDE_LWM", -1L);
  GDBG_INFO(80,"\tswFifoLWM: %d\n",_GlideRoot.environment.swFifoLWM);
  
  _GlideRoot.environment.swapInterval =
    GLIDE_GETENV("FX_GLIDE_SWAPINTERVAL", -1L);
  GDBG_INFO(80,"\tswapInterval: %d\n",_GlideRoot.environment.swapInterval);
  
  _GlideRoot.environment.snapshot = GLIDE_GETENV("FX_SNAPSHOT", -1L);
  GDBG_INFO(80,"\tsnapshot: %d\n",_GlideRoot.environment.snapshot);
  
  _GlideRoot.environment.gammaR = GLIDE_FGETENV("SSTH3_RGAMMA", -1.f);
  _GlideRoot.environment.gammaG = GLIDE_FGETENV("SSTH3_GGAMMA", -1.f);
  _GlideRoot.environment.gammaB = GLIDE_FGETENV("SSTH3_BGAMMA", -1.f);

  _GlideRoot.environment.enUma = 0;

  /* Setup the basic proc tables based on the cpu type. */
  {
    _GlideRoot.CPUType = GLIDE_GETENV("FX_CPU", _cpu_detect_asm() );
    GDBG_INFO(80,"\tcpu: %d\n",_GlideRoot.CPUType);
      
    /* Default case */
#if GLIDE_DISPATCH_SETUP
    _GlideRoot.curTriProcs = _triSetupProcs + 0;
#endif /* GLIDE_DISPATCH_SETUP */

    _GlideRoot.curTexProcs = _texDownloadProcs + 0;
      
    /* Check for vendor specific optimization cases */
    switch((_GlideRoot.CPUType & 0xFFFF0000UL) >> 16UL) {
    case kCPUVendorIntel:
      break;
        
    case kCPUVendorAMD:
    case kCPUVendorCyrix:
    case kCPUVendorIDT:
      if ((_GlideRoot.CPUType & 0x02UL) == 0x02UL) { /* MMX & 3DNow!(tm) feature bits set */
#if GLIDE_DISPATCH_SETUP
        _GlideRoot.curTriProcs = _triSetupProcs + 1;
#endif /* GLIDE_DISPATCH_SETUP */

        _GlideRoot.curTexProcs = _texDownloadProcs + 1;
      }
      break;
        
    case kCPUVendorUnknown:
    default:
      break;
    }
  }
  
  _GlideRoot.environment.emulRush = (GETENV("FX_GLIDE_EMUL_RUSH") != NULL);
  GDBG_INFO(80,"\temulate Rush: %d\n", _GlideRoot.environment.emulRush);
  
  _GlideRoot.environment.autoBump = (GETENV("FX_GLIDE_BUMP") == NULL);
  GDBG_INFO(80, "\tautoBump:          %s\n",
            _GlideRoot.environment.autoBump ? "FXTRUE" : "FXFALSE");
  
  if (GETENV("FX_GLIDE_BUMPSIZE"))
  {
    unsigned int u;
    if (sscanf(GETENV("FX_GLIDE_BUMPSIZE"), "%x", &u) == 1)
      _GlideRoot.environment.bumpSize = u;
  }
  else
    _GlideRoot.environment.bumpSize = 0x10000;
  GDBG_INFO(80, "\tbumpSize:          0x%x\n", _GlideRoot.environment.bumpSize);
  
  _GlideRoot.environment.bumpSize >>= 2; /* So we don't have to later */

  _GlideRoot.environment.grxClk = GLIDE_GETENV("FX_GLIDE_GRXCLK", -1);

  GDBG_INFO(80, "\tGamma R:          %1.1f\n",
            _GlideRoot.environment.gammaR);
  GDBG_INFO(80, "\tGamma G:          %1.1f\n",
            _GlideRoot.environment.gammaG);
  GDBG_INFO(80, "\tGamma B:          %1.1f\n",
            _GlideRoot.environment.gammaB);


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
  _GlideRoot.curGC       = &_GlideRoot.GCs[0];   /* just for 'booting' the library */
  
  grErrorSetCallback(_grErrorDefaultCallback);
  
  if ( !_grSstDetectResources() ) {
#ifdef GLIDE_INIT_HWC
    GrErrorCallback( hwcGetErrorString(), FXTRUE );
#endif
  }
  
  for (i = 0; i < _GlideRoot.hwConfig.num_sst; i++) {
    _GlideRoot.GCs[i].mm_table.free_mmid = 0;
    displayBoardInfo(i, &_GlideRoot.hwConfig);
  }
  
  _grMipMapInit();
  _GlideRoot.initialized = FXTRUE;               /* save this for the end */
} /* _GlideInitEnvironment */
