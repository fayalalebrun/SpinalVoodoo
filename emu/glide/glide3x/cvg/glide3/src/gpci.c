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
** Revision 1.1.1.1.8.9  2007/09/29 14:01:24  koolsmoky
** removed false swapInterval envar checks
**
** Revision 1.1.1.1.8.8  2005/08/13 21:06:57  jwrdegoede
** Last needed 64 bit fixes for h5/h3, complete 64 bit support for cvg
**
** Revision 1.1.1.1.8.7  2005/06/10 18:59:00  jwrdegoede
** Fix linking when GL_X86 is not defined
**
** Revision 1.1.1.1.8.6  2005/05/25 08:51:49  jwrdegoede
** Add #ifdef GL_X86 around x86 specific code
**
** Revision 1.1.1.1.8.5  2005/01/13 15:42:51  koolsmoky
** fixed warning typo
**
** Revision 1.1.1.1.8.4  2004/12/27 20:47:11  koolsmoky
** added dll entry point to call grGlideShutdown when a process is detached
**
** Revision 1.1.1.1.8.3  2004/12/12 15:00:09  koolsmoky
** revert swapPendingCount default value to 4.
**
** Revision 1.1.1.1.8.2  2004/11/25 19:03:53  koolsmoky
** Always allow user to override swapInterval (vsync) but force vsync on with swapbuffer interval set to 0x0 when triple buffering is enabled.
**
** Revision 1.1.1.1.8.1  2004/02/16 07:42:15  dborca
** grSetNumPendingBuffers visible with grGetProcAddress
**
** Revision 1.1.1.1  1999/12/07 21:42:33  joseph
** Initial checkin into SourceForge.
**
** 
** 2     10/08/98 3:04p Atai
** added clip coords tri cull version
** 
** 12    10/08/98 2:58p Atai
** added clip coords tri cull version
** 
** 11    10/07/98 9:43p Peter
** triangle procs for 3DNow!(tm)
** 
** 10    10/06/98 8:23p Peter
** 3DNow!(tm) texture downloads
** 
** 9     8/18/98 3:20p Atai
** initialize snapshot value to 0
** 
** 8     7/24/98 1:41p Hohn
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
 * fixed all the muckage from new config
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
 * fixed my muckage
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

#if GLIDE3
#define kRevisionOffset 0x00UL
#else
#define kRevisionOffset 0x100UL
#endif

#if GLIDE_DISPATCH_SETUP 
/* Collection of all of the known procs for a given system */
static GrTriSetupProc _triSetupProcs[][2][2] = 
{
#ifdef GLIDE_USE_C_TRISETUP
  { {NULL,NULL}, {NULL, NULL} }, { {NULL,NULL}, {NULL, NULL} }
#else
  /* Default Procs */
  {
    { _trisetup_Default_Default, _trisetup_Default_cull }, /* GR_WINDOW_COORDS */
    { _vptrisetup_nocull, _vptrisetup_cull },              /* GR_CLIP_COORDS */
  },
#if GL_AMD3D
  /* 3DNow!(tm) Procs */
  {
    { _trisetup_3DNow_Default, _trisetup_3DNow_cull }, /* GR_WINDOW_COORDS */
    { _trisetup_clip_coor_thunk, _trisetup_clip_coor_thunk }, /* GR_CLIP_COORDS */
  },
#endif /* GL_AMD3D */
#endif /* GLIDE_USE_C_TRISETUP */
};

#ifdef GLIDE_USE_C_TRISETUP
void FX_CSTYLE
_grDrawVertexList(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);
#endif

static GrVertexListProc _vertexListProcs[][2] = {
#if GLIDE_USE_C_TRISETUP
  { _grDrawVertexList, _grDrawVertexList },
#else
  { _drawvertexlist, _vpdrawvertexlist },
#endif
#if GL_AMD3D
  { _grDrawVertexList_3DNow_Window, _grDrawVertexList_3DNow_Clip }
#endif /* GL_AMD3D */
};
#endif /* GLIDE_DISPATCH_SETUP */

#if GLIDE_DISPATCH_DOWNLOAD
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
#if GL_MMX
  ,{ 
    { 
      _grTexDownload_Default_8_1, 
      _grTexDownload_Default_8_2, 
      _grTexDownload_Default_8_4, 
      _grTexDownload_MMX, 
    },
    {
      _grTexDownload_Default_16_1,
      _grTexDownload_Default_16_2,
      _grTexDownload_MMX,
      _grTexDownload_MMX,
    },
  }
#endif /* GL_MMX */
};
#endif /* GLIDE_DISPATCH_DOWNLOAD */

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
#if DIRECTX
    pciClose();
#endif
    break;
  case DLL_PROCESS_ATTACH:
#if DIRECTX
    if (!pciOpen()) {
      GDBG_INFO(80, "pci bus could not be opened\n");
      GR_RETURN(FXFALSE);
    }
#endif
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
  FxU32 ctx, device;

  GDBG_INFO(280, "_grSstDetectResources()\n");

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
  {
#if GLIDE_INIT_HAL
    HalInfo* halInfo = fxHalInit(0);
    if (halInfo == NULL) goto __errExit;
#else /* !GLIDE_INIT_HAL */
    FxDeviceInfo dummyDevInfo;
#endif /* !GLIDE_INIT_HAL */

    /* The first time through the init code we need to map 
     * all of the boards. Future calls can just grab this
     * info out of the halInfo that we have here.
     */
    if (!calledP) {
      FxU32 count = HAL_MAX_BOARDS;
      FxBool inSliPairP = FXFALSE;

      /* NB: The hw will initially return fbiRevision == 2, but
       * the init code does some more dorking w/ stuff and detects
       * if it is really a CVG and sets it to 4.
       */
      const FxU32 hwRevNum = ((GETENV("FX_GLIDE_HW_REV") == NULL)
                              ? 4
                              : atol(GETENV("FX_GLIDE_HW_REV")));
      
      for(ctx = device = 0; device < count; device++) {
        const FxDeviceInfo* curDev = NULL;
        FxBool regInitP = FXFALSE;
        SstRegs* devRegs;

#if GLIDE_INIT_HAL
        devRegs = fxHalMapBoard(device);
        curDev = halInfo->boardInfo + device;
#else /* !GLIDE_INIT_HAL */
        devRegs = (SstRegs*)sst1InitMapBoard(device);
        if (devRegs != NULL) {
          if (sst1InitGetDeviceInfo((FxU32*)devRegs, &dummyDevInfo)) {
            dummyDevInfo.virtAddr[0] = devRegs;
            curDev = &dummyDevInfo;
          }
        }
#endif /* !GLIDE_INIT_HAL */

        if ((devRegs != NULL) && 
            (curDev != NULL) &&
            (curDev->fbiRevision >= hwRevNum)) {
          FxU32 tmuMem = 0x00;

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)        
          /* Internally we always keep the real type. */
          _GlideRoot.hwConfig.SSTs[ctx].type = GR_SSTTYPE_Voodoo2;
#else
#error "What do you think you're doing?"
#endif

#if GLIDE_INIT_HAL
          if (!fxHalInitRegisters(curDev->virtAddr[0])) goto __errRegFailure;

#else /* !GLIDE_INIT_HAL */
          /* dpc - 23 jan 1998 - FixMe!
           * We set up the caching characteristics here so that we
           * mimic the actions of the nt driver, where everything is
           * happy, in win95.  However, if this is the slave of an sli
           * pair then we don't want to waste mtrr's that we're never
           * really going to write to.  
           */
          if (!inSliPairP) sst1InitCaching((FxU32*)devRegs, FXTRUE);

          if (!sst1InitRegisters((FxU32*)devRegs) ||
              !sst1InitGetDeviceInfo((FxU32*)devRegs, &dummyDevInfo)) goto __errRegFailure;
          curDev = &dummyDevInfo;
#endif /* !GLIDE_INIT_HAL */

          /* Set up pointers to the various address spaces within the hw */
          _GlideRoot.GCs[ctx].base_ptr  = (FxU32*)HW_BASE_PTR(devRegs);
          _GlideRoot.GCs[ctx].reg_ptr   = (FxU32*)HW_REG_PTR(devRegs);
          _GlideRoot.GCs[ctx].lfb_ptr   = (FxU32*)HW_LFB_PTR(devRegs);
          _GlideRoot.GCs[ctx].tex_ptr   = (FxU32*)HW_TEX_PTR(devRegs);

          /* Video parameters */
          _GlideRoot.GCs[ctx].grSstRez     = GR_RESOLUTION_NONE;
          _GlideRoot.GCs[ctx].grSstRefresh = curDev->fbiVideoRefresh;

          /* dpc - 22 oct 1997 - FixMe!
           * How do we know which device is really paired to us if there
           * are multiple boards in the system? sst1GetDeviceInfo will not
           * return any pairing information until after the boards are
           * paired w/ sst1InitSli which we have not yet done? Currently 
           * we assume that the next discovered board will be the slave.  
           */
          _GlideRoot.GCs[ctx].scanline_interleaved = curDev->sliDetected;
          _GlideRoot.GCs[ctx].sliPairP             = curDev->sliPaired;
          _GlideRoot.GCs[ctx].slave_ptr            = NULL;
          if (curDev->sliDetected && inSliPairP) 
            _GlideRoot.GCs[ctx - 1].slave_ptr = _GlideRoot.GCs[ctx].base_ptr;
        
          /* Chip configuration */
          _GlideRoot.GCs[ctx].num_tmu   = curDev->numberTmus;
          _GlideRoot.GCs[ctx].fbuf_size = curDev->fbiMemSize;

          if (!inSliPairP) {
            GrHwConfiguration* hwConfig = &_GlideRoot.hwConfig;
            const FxU32 curSstNum = hwConfig->num_sst;

            _GlideRoot.gcMap[curSstNum] = _GlideRoot.gcNum;
            
            /* Voodoo^2 glide2x returns that it is an sst1, but the
             * revision is bumped so that developers can really tell.
             * Additionally, we now have the fun on Voodoo^1.5 which is
             * just a Voodoo^2 in a 2200 configuration. 
             */
            hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.fbiRev = (curDev->fbiRevision + 
                                                                       ((curDev->numberTmus == 1)
                                                                        ? 0x080 
                                                                        : 0x100));
            
            hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.fbRam     = curDev->fbiMemSize;
            hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.nTexelfx  = curDev->numberTmus;
            hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.sliDetect = curDev->sliDetected;

            {
              FxU32 i;
              
              for(i = 0; i < curDev->numberTmus; i++) {
                const FxU32 curTmuMemSize = curDev->tmuMemSize[i];
                
                hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.tmuConfig[i].tmuRev = curDev->tmuRevision;
                hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.tmuConfig[i].tmuRam = curTmuMemSize;
                tmuMem += curTmuMemSize;
              }
            }

            hwConfig->num_sst++;
          }

          /* Clear the tmu state */
          {
            FxU32 i;
            
            for(i = 0; i < curDev->numberTmus; i++) {
              const FxU32 curTmuMemSize = curDev->tmuMemSize[i];
              
              memset(&_GlideRoot.GCs[ctx].tmu_state[i], 0, sizeof(_GlideRoot.GCs[ctx].tmu_state[i]));
              _GlideRoot.GCs[ctx].tmu_state[i].total_mem = (curTmuMemSize << 20);
              
              _GlideRoot.GCs[ctx].tmu_state[i].ncc_mmids[0] = 
              _GlideRoot.GCs[ctx].tmu_state[i].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;
            }
          }

          inSliPairP = (curDev->sliPaired ? !inSliPairP : FXFALSE);

          /* This device is ready to go. */
          regInitP = FXTRUE;
          _GlideRoot.gcNum++;
          _GlideRoot.GCs[ctx].hwInitP = FXTRUE;
        
          rv = FXTRUE;
          ctx++;

          __errRegFailure:
          /* For some reason we failed to init the board that we just mapped. */
          if (!regInitP && (devRegs != NULL)) {
#if GLIDE_INIT_HAL
            fxHalShutdown(devRegs);
#else /* !GLIDE_INIT_HAL */
            sst1InitCaching((FxU32*)devRegs, FXFALSE);
            pciUnmapPhysical((unsigned long)devRegs, 0x1000000UL);
#endif /* !GLIDE_INIT_HAL */
          }
        }
      }

      /* Done setting up. Don't do the silly mapping thing again. */
      calledP = FXTRUE;
    } else {
      /* Did we previously find boards? */
      rv = (_GlideRoot.hwConfig.num_sst != 0);
    }

    goto __errExit; /* Keep warnings happy */
__errExit:
    ;
  }
#else
#  error "Write code for this chip"
#endif  
  
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

#if defined(FX_DLL_ENABLE) && (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#include <windows.h>
#endif

void
_GlideInitEnvironment(void)
{
#define FN_NAME "_GlideInitEnvironment"
  int i;

  if (_GlideRoot.initialized)          /* only execute once */
    return;
  GDBG_INIT();                          /* init the GDEBUG libraray */
  GDBG_INFO(80,"%s()\n", FN_NAME);

#ifdef __WIN32__
  grErrorSetCallback(_grErrorWindowsCallback);
#else
  grErrorSetCallback(_grErrorDefaultCallback);
#endif /* __WIN32__ */

#ifdef GLIDE_DEBUG
  GDBG_INFO(0,"GLIDE DEBUG LIBRARY\n"); /* unconditional display */
#endif

#if defined(FX_DLL_ENABLE) && (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  { /* GMT: display the DLL pathname for sanity checking */
    char buf[132];
    if (GetModuleFileName(GetModuleHandle("glide3x.dll"), buf, sizeof(buf))) {
#ifdef GLIDE_DEBUG
      gdbg_printf("DLL path: %s\n", buf); /* unconditional display */
#else
      GDBG_INFO(80,"DLL path: %s\n",buf);
#endif
    } /* silent failure */
  }
#endif

  /* Setup the basic proc tables based on the cpu type. */
  {
#if GL_X86
    /* Get CPU Info */
    _cpuid (&_GlideRoot.CPUType);

    /* Check for vendor specific optimization cases */
    GDBG_INFO( 0,"   CPU Vendor: %s\n", _GlideRoot.CPUType.v_name);
    GDBG_INFO(80,"   MMX Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_MMX ? 'Y' : 'N');
    GDBG_INFO(80,"   SSE Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_SSE ? 'Y' : 'N');
    GDBG_INFO(80,"  SSE2 Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_SSE2 ? 'Y' : 'N');
    GDBG_INFO(80," 3DNow Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_3DNOW ? 'Y' : 'N');
    GDBG_INFO(80,"  MMX+ Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_MMXPLUS ? 'Y' : 'N');
    GDBG_INFO(80,"3DNow+ Support: %c\n", _GlideRoot.CPUType.os_support&_CPU_FEATURE_3DNOWPLUS ? 'Y' : 'N');
    
    /* No CPU Extensions Allowed */
    if (GETENV("FX_GLIDE_NO_CPU_EXTENSIONS"))
    {
      _GlideRoot.CPUType.feature = _GlideRoot.CPUType.os_support = 0;
      GDBG_INFO(0,"CPU Extensions disabled\n");
    }
#endif

    /* Default case */
#if GLIDE_DISPATCH_SETUP
    _GlideRoot.deviceArchProcs.curTriProcs        = _triSetupProcs + 0;
    _GlideRoot.deviceArchProcs.curDrawTrisProc    = _grDrawTriangles_Default;
    _GlideRoot.deviceArchProcs.curVertexListProcs = _vertexListProcs[0];
    _GlideRoot.deviceArchProcs.curLineProc        = _grDrawTextureLine_Default;
#endif /* GLIDE_DISPATCH_SETUP */
#if GLIDE_DISPATCH_DOWNLOAD
    _GlideRoot.deviceArchProcs.curTexProcs = _texDownloadProcs + 0;
#endif /* GLIDE_DISPATCH_DOWNLOAD */

    /* Check for vendor specific optimization cases */
#if GL_MMX && GLIDE_DISPATCH_DOWNLOAD
    if (_GlideRoot.CPUType.os_support & _CPU_FEATURE_MMX) {
      _GlideRoot.deviceArchProcs.curTexProcs = _texDownloadProcs + 2;
    }
#endif /* GL_MMX */
#ifdef GL_AMD3D
    if (_GlideRoot.CPUType.os_support & _CPU_FEATURE_3DNOW) {
#if GLIDE_DISPATCH_SETUP
      _GlideRoot.deviceArchProcs.curTriProcs        = _triSetupProcs + 1;
      _GlideRoot.deviceArchProcs.curDrawTrisProc    = _grDrawTriangles_3DNow;
      _GlideRoot.deviceArchProcs.curVertexListProcs = _vertexListProcs[1];
      _GlideRoot.deviceArchProcs.curLineProc        = _grDrawTextureLine_3DNow;
#endif /* GLIDE_DISPATCH_SETUP */
#if GLIDE_DISPATCH_DOWNLOAD
      _GlideRoot.deviceArchProcs.curTexProcs = _texDownloadProcs + 1;
#endif /* GLIDE_DISPATCH_DOWNLOAD */
    }
#endif /* GL_AMD3D */
  }

  /* Check for user environment tweaks */
  {
    const char* envStr;

#define GLIDE_GETENV(__envVar, __defVal) \
    (((envStr = GETENV(__envVar)) == NULL) ? (__defVal) : atol(envStr))

    _GlideRoot.environment.triBoundsCheck    = (GETENV("FX_GLIDE_BOUNDS_CHECK") != NULL);
#ifdef GLIDE_SPLASH
    _GlideRoot.environment.noSplash          = (GETENV("FX_GLIDE_NO_SPLASH") != NULL);
#else
    _GlideRoot.environment.noSplash          = 1;
#endif
#ifdef GLIDE_PLUG
    _GlideRoot.environment.shamelessPlug     = (GETENV("FX_GLIDE_SHAMELESS_PLUG") != NULL);
#else
    _GlideRoot.environment.shamelessPlug     = 0;
#endif
    _GlideRoot.environment.ignoreReopen      = (GETENV("FX_GLIDE_IGNORE_REOPEN") != NULL);
    _GlideRoot.environment.texLodDither      = ((GETENV("FX_GLIDE_LOD_DITHER") == NULL)
                                                ? 0x00UL
                                                : SST_TLODDITHER);
    
    _GlideRoot.environment.nColorBuffer      = GLIDE_GETENV("FX_GLIDE_ALLOC_COLOR", -1L);
    _GlideRoot.environment.nAuxBuffer        = GLIDE_GETENV("FX_GLIDE_ALLOC_AUX", -1L);    
    _GlideRoot.environment.swFifoLWM         = GLIDE_GETENV("FX_GLIDE_LWM", -1L);

    _GlideRoot.environment.swapInterval      = GLIDE_GETENV("FX_GLIDE_SWAPINTERVAL", -1L);

    /* wait until there's 6 or fewer buffer swaps pending */
    /* the hardware counter is only 3 bits so we don't want it to overflow */
    /* also the latency gets too long */
    _GlideRoot.environment.swapPendingCount  = GLIDE_GETENV("FX_GLIDE_SWAPPENDINGCOUNT", 4L);
    if (_GlideRoot.environment.swapPendingCount > 6)
      _GlideRoot.environment.swapPendingCount = 6;
    if (_GlideRoot.environment.swapPendingCount < 0)
      _GlideRoot.environment.swapPendingCount = 0;

    _GlideRoot.environment.snapshot          = GLIDE_GETENV("FX_SNAPSHOT", 0);

    /* set default to disable alpha dither subtraction */
    if ((envStr = GETENV("FX_GLIDE_NO_DITHER_SUB")) == NULL) {
      _GlideRoot.environment.disableDitherSub = FXTRUE;
    } else {
      _GlideRoot.environment.disableDitherSub = (atol(envStr) != 0);
    }

    GDBG_INFO(80,"    triBoundsCheck: %d\n",_GlideRoot.environment.triBoundsCheck);
    GDBG_INFO(80,"      swapInterval: %d\n",_GlideRoot.environment.swapInterval);
    GDBG_INFO(80,"          noSplash: %d\n",_GlideRoot.environment.noSplash);
    GDBG_INFO(80,"     shamelessPlug: %d\n",_GlideRoot.environment.shamelessPlug);
    GDBG_INFO(80,"          snapshot: %d\n",_GlideRoot.environment.snapshot);
    GDBG_INFO(80,"  disableDitherSub: %d\n",_GlideRoot.environment.disableDitherSub);
    GDBG_INFO(80," swapPendingCount : %d\n",_GlideRoot.environment.swapPendingCount);
  }

  /* constant pool */
  _GlideRoot.pool.f0   =   0.0F;
  _GlideRoot.pool.fHalf=   0.5F;
  _GlideRoot.pool.f1   =   1.0F;
  _GlideRoot.pool.f255 = 255.0F;

  /* Currently used in grDrawPoint to 'accurately' compute the ceil
   * function by adding this and a large floating point bias, and then
   * subtracting out only the bias value.
   */
  _GlideRoot.pool.fIntTruncBias = 0.500001f;

#if GLIDE_PACKED_RGB
  _GlideRoot.pool.fBiasHi = (float)(0x01 << 15);
  _GlideRoot.pool.fBiasLo = (float)(0x01 << 23);
#endif /* GLIDE_PACKED_RGB */

  _GlideRoot.current_sst = 0;                    /* make sure there's a valid GC */
  _GlideRoot.curGC = &_GlideRoot.GCs[0];         /* just for 'booting' the library */
  {
    const FxBool hwDetectP = _grSstDetectResources();

    if (!hwDetectP) {
      char s[128];
      const char* errStr = s;
      
      if (pciGetErrorCode() == PCI_ERR_NOERR) {
        sprintf(s, "%s: expected %s, none detected\n", FN_NAME, GLIDE_DRIVER_NAME);
      } else {
        errStr = pciGetErrorString();
      }
      
      GrErrorCallback(errStr, FXTRUE);
    }
    
    /* GMT: this isn't really necessary since GlideRoot is static */
    for (i = 0; i < _GlideRoot.hwConfig.num_sst; i++) {
#ifndef GLIDE3_ALPHA
      _GlideRoot.GCs[i].mm_table.free_mmid = 0;
#endif

      displayBoardInfo(i, &_GlideRoot.hwConfig);
    }
    
    _grMipMapInit();
    _GlideRoot.initialized = hwDetectP;               /* save this for the end */
  }
} /* _GlideInitEnvironment */
