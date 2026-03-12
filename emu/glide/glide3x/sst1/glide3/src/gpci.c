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
 ** Revision 1.1.2.4  2005/05/25 08:51:52  jwrdegoede
 ** Add #ifdef GL_X86 around x86 specific code
 **
 ** Revision 1.1.2.3  2004/10/04 09:36:00  dborca
 ** second cut at Glide3x for Voodoo1/Rush (massive update):
 ** delayed validation, vertex snapping, clip coordinates, strip/fan_continue, bugfixes.
 ** and probably a bunch of other crap that I forgot
 **
 ** Revision 1.1.2.2  2004/03/08 07:42:21  dborca
 ** Voodoo Rush fixes
 **
 ** Revision 1.1.2.1  2004/03/02 07:55:30  dborca
 ** Bastardised Glide3x for SST1
 **
 ** Revision 1.1.1.1  1999/12/07 21:48:52  joseph
 ** Initial checkin into SourceForge.
 **
 * 
 * 45    11/01/97 12:11p Pgj
 * glide.dll ---> glide2x.dll
 * 
 * 44    10/21/97 8:20p Atai
 * initialize vid timing to NLLL
 * 
 * 43    8/18/97 3:11p Atai
 * fix vg96 fb ram configuration
 * 
 * 42    7/18/97 5:30p Jdt
 * Fixes for open/shutdown symmetry
 * 
 * 41    7/12/97 4:58p Dow
 * Added DllMain
 * 
 * 40    7/07/97 8:33a Jdt
 * New tracing macros.
 * 
 * 39    6/19/97 7:35p Dow
 * More P6 Stuff
 * 
 * 38    5/27/97 2:03p Dow
 * added new env variable
 * 
 * 37    5/19/97 7:35p Pgj
 * Print cogent error message if h/w not found
 * 
 * 36    4/15/97 12:59p Dow
 * Bandaid for SST96/P6
 * 
 * 35    4/13/97 8:11p Jdt
 * Change in init.h
 * 
 * 34    4/13/97 2:06p Pgj
 * eliminate all anonymous unions (use hwDep)
 * 
 * 33    3/24/97 7:37p Dow
 * Now sets n tmus for hwConfig
 * 
 * 32    3/18/97 9:08p Dow
 * Added FX_GLIDE_NO_DITHER_SUB environment variable
 * 
 * 31    3/17/97 6:27a Jdt
 * Removed enumCB and pass 0 to enumHardware.
 * 
 * 30    3/16/97 12:51a Jdt
 * fix num_sst when detecting vg96
 * 
 * 29    3/16/97 12:39a Jdt
 * Programmatic type-os
 * 
 * 28    3/16/97 12:19a Jdt
 * Completely re-wrote grSstDetectResources
 * 
 * 27    3/06/97 4:54p Dow
 * Fixed SLI
 * 
 * 26    3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 25    3/02/97 7:05p Dow
 * Changes to support DOS DLLs
 * 
 * 24    2/26/97 2:18p Dow
 * moved all debug set macros to __cdecl
 * 
 * 23    2/26/97 11:57a Jdt
 * Fixed stack bug, fixed splash screen bug
 * 
 * 22    2/19/97 4:42p Dow
 * Fixed debug build for Watcom
 * 
 * 21    2/18/97 10:39a Dow
 * Added call to initRegisters for AT3D
 * 
 * 20    2/18/97 9:51a Jdt
 * Did some casting to remove warnings.
 * 
 * 19    2/14/97 12:55p Dow
 * moved vg96 fifo wrap into init code
 * 
 * 18    2/12/97 8:15p Dow
 * Fixed VG base_pointe
 * 
 * 17    2/12/97 9:20a Dow
 * Fixed Watcom error wrt GDBG_ERROR call
 * 
 * 16    2/11/97 6:59p Dow
 * Changes to support vid tiles and ser status
 * 
 * 15    1/18/97 11:48p Dow
 * Re-added error callback set
 * Added support for GMT's register debugging
 * 
 * 14    1/14/97 6:41p Dow
 * 
 * 13    12/23/96 1:37p Dow
 * chagnes for multiplatform glide
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

#include <sst1vid.h>
#if ( GLIDE_PLATFORM & GLIDE_SST_SIM )
#include <gsim.h>
#endif

#include <init.h>

#if ( GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL WINAPI 
DllMain(HANDLE hInst, ULONG  ul_reason_for_call, LPVOID lpReserved) 
{
  switch( ul_reason_for_call ) {
  case DLL_PROCESS_DETACH:
    GDBG_INFO((80, "DllMain: DLL_PROCESS_DETACH\n"));
    grGlideShutdown();
    break;
  case DLL_PROCESS_ATTACH:
    GDBG_INFO((80, "DllMain: DLL_PROCESS_DETACH\n"));
    break;
  case DLL_THREAD_ATTACH:
    GDBG_INFO((80, "DllMain: DLL_PROCESS_DETACH\n"));
    break;
  case DLL_THREAD_DETACH:
    GDBG_INFO((80, "DllMain: DLL_PROCESS_DETACH\n"));
    break;
  default:
    GDBG_INFO((80, "DllMain: Unhandled message.\n"));
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
  InitDeviceInfo info;
  int ctx, device;
  FxBool rv = FXFALSE;

  GDBG_INFO((280,"_grSstDetectResources()\n"));

  initEnumHardware( 0 );

  _GlideRoot.hwConfig.num_sst = 0;
  for( ctx = 0, device = 0; 
      device < INIT_MAX_DEVICES; 
      device++ ) {
    if ( initGetDeviceInfo( device, &info ) ) {
#if   ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
      if ( info.hwClass == INIT_VOODOO ) {
        int tmu;
        
        _GlideRoot.hwConfig.SSTs[ctx].type = GR_SSTTYPE_VOODOO;
        
        _GlideRoot.GCs[ctx].base_ptr  = (FxU32 *)info.hwDep.vgInfo.vgBaseAddr;
        _GlideRoot.GCs[ctx].reg_ptr   = (FxU32 *)info.hwDep.vgInfo.vgBaseAddr;
        _GlideRoot.GCs[ctx].lfb_ptr   = (FxU32 *)SST_LFB_ADDRESS(info.hwDep.vgInfo.vgBaseAddr);
        _GlideRoot.GCs[ctx].tex_ptr   = (FxU32 *)SST_TEX_ADDRESS(info.hwDep.vgInfo.vgBaseAddr);
        _GlideRoot.GCs[ctx].slave_ptr = (FxU32 *)info.hwDep.vgInfo.slaveBaseAddr;
        _GlideRoot.GCs[ctx].grSstRez  = GR_RESOLUTION_NONE;

        _GlideRoot.GCs[ctx].scanline_interleaved = info.hwDep.vgInfo.sliDetect;
        _GlideRoot.GCs[ctx].grSstRefresh         = GR_REFRESH_NONE;
        _GlideRoot.GCs[ctx].num_tmu              = info.hwDep.vgInfo.nTFX;
        _GlideRoot.GCs[ctx].fbuf_size            = info.hwDep.vgInfo.pfxRam;
        _GlideRoot.GCs[ctx].vidTimings           = NULL;
        
        _GlideRoot.hwConfig.num_sst++;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.fbiRev = 
          info.hwDep.vgInfo.pfxRev;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.fbRam =
          info.hwDep.vgInfo.pfxRam;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.sliDetect =
          info.hwDep.vgInfo.sliDetect;
        
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.nTexelfx = 
          info.hwDep.vgInfo.nTFX;

        for (tmu = 0; tmu < _GlideRoot.GCs[ctx].num_tmu; tmu++) {
          _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.tmuConfig[tmu].tmuRam =
            info.hwDep.vgInfo.tfxRam;
          _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.tmuConfig[tmu].tmuRev =
            info.hwDep.vgInfo.tfxRev;
          
          memset(&_GlideRoot.GCs[ctx].tmu_state[tmu], 0, sizeof(_GlideRoot.GCs[ctx].tmu_state[tmu]));
          _GlideRoot.GCs[ctx].tmu_state[tmu].ncc_mmids[0] = GR_NULL_MIPMAP_HANDLE;
          _GlideRoot.GCs[ctx].tmu_state[tmu].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;
          _GlideRoot.GCs[ctx].tmu_state[tmu].total_mem    = info.hwDep.vgInfo.tfxRam<<20;
        }
        rv = FXTRUE;
        ctx++;
      }
#elif ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
      if ( info.hwClass == INIT_VG96 ) {
        _GlideRoot.hwConfig.SSTs[ctx].type = GR_SSTTYPE_SST96;

        _GlideRoot.GCs[ctx].base_ptr  = (FxU32 *)info.hwDep.vg96Info.vg96BaseAddr;
        _GlideRoot.GCs[ctx].reg_ptr   = (FxU32 *)SST96_REG_PTR(info.hwDep.vg96Info.vg96BaseAddr);
        _GlideRoot.GCs[ctx].lfb_ptr   = (FxU32 *)SST96_LFB_PTR(info.hwDep.vg96Info.vg96BaseAddr);
        _GlideRoot.GCs[ctx].tex_ptr   = (FxU32 *)SST96_TEX_PTR(info.hwDep.vg96Info.vg96BaseAddr);
        _GlideRoot.GCs[ctx].slave_ptr = 0;

        _GlideRoot.GCs[ctx].grSstRez     = GR_RESOLUTION_NONE;
        _GlideRoot.GCs[ctx].grSstRefresh = GR_REFRESH_NONE;

        _GlideRoot.GCs[ctx].scanline_interleaved = FXFALSE;
        _GlideRoot.GCs[ctx].num_tmu              = info.hwDep.vg96Info.nTFX;
        _GlideRoot.GCs[ctx].fbuf_size            = info.hwDep.vg96Info.vg96Ram;

        _GlideRoot.hwConfig.num_sst++;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.SST96Config.nTexelfx = 
          info.hwDep.vg96Info.nTFX;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.SST96Config.fbRam = 
          info.hwDep.vg96Info.vg96Ram >> 20;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.SST96Config.vg96Rev = 
          info.hwDep.vg96Info.vg96Rev;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.SST96Config.tmuConfig.tmuRev =
          info.hwDep.vg96Info.tfxRev;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.SST96Config.tmuConfig.tmuRam =
          info.hwDep.vg96Info.tfxRam;
        memset(&_GlideRoot.GCs[ctx].tmu_state[0], 0, sizeof(_GlideRoot.GCs[ctx].tmu_state[0]));

        _GlideRoot.GCs[ctx].tmu_state[0].ncc_mmids[0] = GR_NULL_MIPMAP_HANDLE;
        _GlideRoot.GCs[ctx].tmu_state[0].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;
        _GlideRoot.GCs[ctx].tmu_state[0].total_mem    = info.hwDep.vg96Info.tfxRam<<20;
        
        _GlideRoot.GCs[ctx].hwDep.sst96Dep.serialStatus     = 
          (FxU32*)info.regs.hwDep.VG96RegDesc.serialStatus;
        _GlideRoot.GCs[ctx].hwDep.sst96Dep.fifoApertureBase = 
          (FxU32*)info.regs.hwDep.VG96RegDesc.fifoApertureBase;

        initDeviceSelect( ctx );
        initSetWriteMethod( _grSst96Store32 );
        rv = FXTRUE;
        ctx++;
      }
#else
#error "Need to define detection parameters for this device"    
#endif
    } else {
      break;
    }
  }
  
  return rv;
} /* _grSstDetectResources */


static void
displayBoardInfo( int i, GrHwConfiguration *hwc )
{
  if (hwc->SSTs[i].type == GR_SSTTYPE_VOODOO) {
    int tmuNum;

    GDBG_INFO((80,"SST board %d: 3Dfx Voodoo Graphics\n", i));
    GDBG_INFO((80,"\tPixelfx rev %d with %d MB Frame Buffer\n",
               hwc->SSTs[i].sstBoard.VoodooConfig.fbiRev,
               hwc->SSTs[i].sstBoard.VoodooConfig.fbRam
               ));
    GDBG_INFO((80,"\t%d Texelfx chips:\n",
               hwc->SSTs[i].sstBoard.VoodooConfig.nTexelfx
               ));
    if (hwc->SSTs[i].sstBoard.VoodooConfig.sliDetect) {
      GDBG_INFO((80,"\tScanline Interleaved\n"));
    }
    for (tmuNum = 0;
         tmuNum < hwc->SSTs[i].sstBoard.VoodooConfig.nTexelfx;
         tmuNum++ )
      {
        GDBG_INFO((80,"\t\tTexelfx %d: Rev %d, %d MB Texture\n", tmuNum,
                   hwc->SSTs[i].sstBoard.VoodooConfig.tmuConfig[tmuNum].tmuRev,
                   hwc->SSTs[i].sstBoard.VoodooConfig.tmuConfig[tmuNum].tmuRam
                   ));
      }
  } else if (hwc->SSTs[i].type == GR_SSTTYPE_SST96) {
    GDBG_INFO((80,"SST board %d: 3Dfx Voodoo Rush\n", i));
    GDBG_INFO((80,"\tFBI Jr. with %d MB Frame Buffer\n",
               hwc->SSTs[i].sstBoard.SST96Config.fbRam
               ));
    GDBG_INFO((80,"\tTexelfx chips:  1\n"));

  }
  else {
    GDBG_INFO((80,"error: SSTs %d: unknown type\n",i));
  }
} /* displayBoardInfo */

/* Returns 16:16 pair indicating the cpu's manufacturer and its
 * capabilities. Non-Intel processors should have a vendor id w/ the
 * high bit set so that it appears to be a negative #. The value of
 * the capability field is assumed to be a monotonically increasing
 * inclusive set.
 *
 * Unknown:
 *   0xFFFF:0xFFFF
 *
 * Intel: 0x0000
 *  4: 486 and lower
 *  5: Pentium
 *  6: P6 Core or better
 *
 * AMD: 0x8001
 *  1: MMX
 *  2: 3DNow!(tm)
 */

enum {
  kCPUVendorIntel   = 0x0000,
  kCPUVendorAMD     = 0x8001,
  kCPUVendorUnknown = 0xFFFF
};
extern FxI32 GR_CDECL
_cpu_detect_asm(void);

#if defined(FX_DLL_ENABLE) && (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#include <windows.h>
#endif

void
_GlideInitEnvironment( void )
{
  int i;
  const char *envp;

  if ( _GlideRoot.initialized ) /* only execute once */
    return;
  GDBG_INIT();                  /* init the GDEBUG libraray */
  GDBG_INFO((80,"_GlideInitEnvironment()\n"));

#ifdef __WIN32__
  grErrorSetCallback(_grErrorWindowsCallback);
#else
  grErrorSetCallback(_grErrorDefaultCallback);
#endif /* __WIN32__ */

#ifdef GLIDE_DEBUG
  GDBG_INFO((0,"GLIDE DEBUG LIBRARY\n")); /* unconditional display */
#endif

#if defined(FX_DLL_ENABLE) && (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  {                             /* GMT: display the DLL pathname for sanity checking */
    char buf[132];
    if (GetModuleFileName(GetModuleHandle("glide3x.dll"),buf,sizeof(buf))) {
#ifdef GLIDE_DEBUG
      GDBG_INFO((0,"DLL path: %s\n",buf)); /* unconditional display */
#else
      GDBG_INFO((80,"DLL path: %s\n",buf));
#endif
    } /* silent failure */
  }
#endif

#if GL_X86
  _GlideRoot.CPUType = _cpu_detect_asm();
  envp = getenv("FX_CPU");
  if (envp) {
    _GlideRoot.CPUType = atoi(envp);
  }
#endif

  _GlideRoot.environment.swapInterval = -1;
  _GlideRoot.environment.swFifoLWM    = -1;

  _GlideRoot.environment.triBoundsCheck   = (getenv("FX_GLIDE_BOUNDS_CHECK") != NULL);
#ifdef GLIDE_SPLASH
  _GlideRoot.environment.noSplash         = (getenv("FX_GLIDE_NO_SPLASH") != NULL);
#else
  _GlideRoot.environment.noSplash         = 1;
#endif
#ifdef GLIDE_PLUG
  _GlideRoot.environment.shamelessPlug    = (getenv("FX_GLIDE_SHAMELESS_PLUG") != NULL);
#else
  _GlideRoot.environment.shamelessPlug    = 0;
#endif
  _GlideRoot.environment.ignoreReopen     = (getenv("FX_GLIDE_IGNORE_REOPEN") != NULL);
  _GlideRoot.environment.disableDitherSub = (getenv("FX_GLIDE_NO_DITHER_SUB") != NULL);

#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
  _GlideRoot.environment.swapPendingCount = 3;
#else /* (GLIDE_PLATFORM & GLIDE_HW_SST1) */
  /* wait until there's 6 or fewer buffer swaps pending */
  /* the hardware counter is only 3 bits so we don't want it to overflow */
  /* also the latency gets too long */
  _GlideRoot.environment.swapPendingCount = 6;
#endif

  envp = getenv("FX_SNAPSHOT");
  if (envp) {
    _GlideRoot.environment.snapshot  = atoi(envp);
  }
  envp = getenv("FX_GLIDE_LWM");
  if (envp) {
    _GlideRoot.environment.swFifoLWM = atoi(envp);
  }

  envp = getenv("FX_GLIDE_SWAPINTERVAL");
  if (envp) {
    _GlideRoot.environment.swapInterval = atoi(envp);
    if (_GlideRoot.environment.swapInterval < 0)
        _GlideRoot.environment.swapInterval = 0;
  }

  GDBG_INFO((80,"    triBoundsCheck: %d\n",_GlideRoot.environment.triBoundsCheck));
  GDBG_INFO((80,"      swapInterval: %d\n",_GlideRoot.environment.swapInterval));
  GDBG_INFO((80,"          noSplash: %d\n",_GlideRoot.environment.noSplash));
  GDBG_INFO((80,"     shamelessPlug: %d\n",_GlideRoot.environment.shamelessPlug));
  GDBG_INFO((80,"         sst2Flags: %d\n",_GlideRoot.environment.sst2Flags));
#if GL_X86
  GDBG_INFO((80,"               cpu: %d\n",_GlideRoot.CPUType));
#endif  
  GDBG_INFO((80,"          snapshot: %d\n",_GlideRoot.environment.snapshot));
  GDBG_INFO((80,"  disableDitherSub: %d\n",_GlideRoot.environment.disableDitherSub));  

  /* GMT: BUG these are hardware dependent and really should come from the init code */
  _GlideRoot.stats.minMemFIFOFree = 0xffff;
  _GlideRoot.stats.minPciFIFOFree = 0x3f;

  /* constant pool */
  _GlideRoot.pool.f0   =   0.0F;
  _GlideRoot.pool.fHalf=   0.5F;
  _GlideRoot.pool.f1   =   1.0F;
  _GlideRoot.pool.f255 = 255.0F;
  _GlideRoot.pool.f256 = 256.0F;

  _GlideRoot.current_sst = 0;   /* make sure there's a valid GC */
  _GlideRoot.curGC = &_GlideRoot.GCs[0]; /* just for 'booting' the library */
  if (!_grSstDetectResources()) {
    char s[128];
    sprintf(s,
            "_GlideInitEnvironment: expected %s, none detected\n",
            GLIDE_DRIVER_NAME);
    GrErrorCallback(s, FXTRUE);
  }

  /* GMT: this isn't really necessary since GlideRoot is static */
  for (i = 0; i < _GlideRoot.hwConfig.num_sst; i++) {
    displayBoardInfo( i, &_GlideRoot.hwConfig );

    /* Force initial fullscreen context state */
    _GlideRoot.GCs[i].open    = FXFALSE;
    _GlideRoot.GCs[i].closedP = FXFALSE;
  }

  _grMipMapInit();
  _GlideRoot.initialized = FXTRUE; /* save this for the end */

} /* _GlideInitEnvironment */


#if defined(GLIDE_DEBUG) && !(GLIDE_PLATFORM & GLIDE_SST_SIM)

/* GMT: tracing utilities */

/* STOLEN FROM SST.C */
/*----------------------------------------------------------------------
  An array of SST register info
  ----------------------------------------------------------------------*/
typedef struct {
  const char *name;
} regInfo;

static regInfo regsInfo[] = {
  "STATUS",
  "reserved0",
  "vA.x",
  "vA.y",
  "vB.x",
  "vB.y",
  "vC.x",
  "vC.y",

  "R",
  "G",
  "B",
  "Z",
  "A",
  "S",
  "T",
  "W",

  "DRDX",
  "DGDX",
  "DBDX",
  "DZDX",
  "DADX",
  "DSDX",
  "DTDX",
  "DWDX",

  "DRDY",
  "DGDY",
  "DBDY",
  "DZDY",
  "DADY",
  "DSDY",
  "DTDY",
  "DWDY",

  "triangleCMD",
  "reserved1",
  "FvA.x",
  "FvA.y",
  "FvB.x",
  "FvB.y",
  "FvC.x",
  "FvC.y",

  "FR",
  "FG",
  "FB",
  "FZ",
  "FA",
  "FS",
  "FT",
  "FW",

  "FDRDX",
  "FDGDX",
  "FDBDX",
  "FDZDX",
  "FDADX",
  "FDSDX",
  "FDTDX",
  "FDWDX",

  "FDRDY",
  "FDGDY",
  "FDBDY",
  "FDZDY",
  "FDADY",
  "FDSDY",
  "FDTDY",
  "FDWDY",

  "FtriangleCMD",
  "FBZCOLORPATH",
  "FOGMODE",
  "ALPHAMODE",
  "FBZMODE",
  "LFBMODE",
  "CLIPLEFTRIGHT",
  "CLIPBOTTOMTOP",

  "nopCMD",
  "fastfillCMD",
  "swapbufCMD",
  "FOGCOLOR",
  "ZACOLOR",
  "CHROMAKEY",
  "reserved2",
  "reserved3",

  "STIPPLE",
  "C0",
  "C1",
  "pixelsIN",
  "chromaFAIL",
  "zfuncFAIL",
  "afuncFAIL",
  "pixelsOUT",

  "FOGTABLE[0]",
  "FOGTABLE[1]",
  "FOGTABLE[2]",
  "FOGTABLE[3]",
  "FOGTABLE[4]",
  "FOGTABLE[5]",
  "FOGTABLE[6]",
  "FOGTABLE[7]",
  "FOGTABLE[8]",
  "FOGTABLE[9]",
  "FOGTABLE[10]",
  "FOGTABLE[11]",
  "FOGTABLE[12]",
  "FOGTABLE[13]",
  "FOGTABLE[14]",
  "FOGTABLE[15]",
  "FOGTABLE[16]",
  "FOGTABLE[17]",
  "FOGTABLE[18]",
  "FOGTABLE[19]",
  "FOGTABLE[20]",
  "FOGTABLE[21]",
  "FOGTABLE[22]",
  "FOGTABLE[23]",
  "FOGTABLE[24]",
  "FOGTABLE[25]",
  "FOGTABLE[26]",
  "FOGTABLE[27]",
  "FOGTABLE[28]",
  "FOGTABLE[29]",
  "FOGTABLE[30]",
  "FOGTABLE[31]",

  "reserved_0",
  "reserved_1",
  "reserved_2",
  "reserved_3",
  "reserved_4",
  "reserved_5",
  "reserved_6",
  "reserved_7",

  "PCIINTERRUPT",
  "VRETRACE",
  "BACKPORCH",
  "VIDEODIMENS",
  "FBIINIT0",
  "FBIINIT1",
  "FBIINIT2",
  "FBIINIT3",

  "HSYNC",
  "VSYNC",
  "CLUTDATA",
  "DACDATA",

  "reserved",
  "reserved",
  "reserved",
  "reserved",

  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",

  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",

  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",

  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",

  "CHROMARANGE",
  "CLIPLEFTRIGHT1",
  "CLIPTOPBOTTOM1",
  "COLBUFSETUP",
  "AUXBUFSETUP",
  "TEXCHIPSEL",
  "SWAPPENDCMD",
  "reserved96",
        
  "reserved96",
  "reserved96",
  "reserved96",
  "reserved96",
  "reserved96",
  "reserved96",
  "reserved96",
  "reserved96",


  "TEXTUREMODE",
  "TLOD",
  "TDETAIL",
  "TEXBASEADDR",
  "TEXBASEADDR1",
  "TEXBASEADDR2",
  "TEXBASEADDR38",
  "TREXINIT0",

  "TREXINIT1",
  "NCCTABLE0[0]",
  "NCCTABLE0[1]",
  "NCCTABLE0[2]",
  "NCCTABLE0[3]",
  "NCCTABLE0[4]",
  "NCCTABLE0[5]",
  "NCCTABLE0[6]",
  "NCCTABLE0[7]",
  "NCCTABLE0[8]",
  "NCCTABLE0[9]",
  "NCCTABLE0[10]",
  "NCCTABLE0[11]",

  "NCCTABLE1[0]",
  "NCCTABLE1[1]",
  "NCCTABLE1[2]",
  "NCCTABLE1[3]",
  "NCCTABLE1[4]",
  "NCCTABLE1[5]",
  "NCCTABLE1[6]",
  "NCCTABLE1[7]",
  "NCCTABLE1[8]",
  "NCCTABLE1[9]",
  "NCCTABLE1[10]",
  "NCCTABLE1[11]",
};

#define MAXREGADDR sizeof(regsInfo)/sizeof(regInfo)

static FxU32 last_addr = 0;             /* TEX trace optimizer */

FxU32 GR_CDECL
_GR_GET(void *addr)
{
  GR_DCL_GC;
  FxU32 chip, data, iaddr = (FxU32) addr;

  data = GET(*(FxU32 *)addr);
  iaddr = iaddr - (FxU32)gc->base_ptr + 0x10000000;
  chip = (iaddr >> 10) & 0xF;
  if (iaddr < 0x10000000) {     /* sanity check */
    GDBG_ERROR("GET","bad address(<chip)=0x%x  data=%d(0x%08x)\n",
                     iaddr,data,data);
  }
  if (iaddr & 3)
    GDBG_ERROR("GET","unaligned address=0x%x\n", iaddr);

  if (iaddr & 0x400000) {   /* are we in LFB territory? */
    GDBG_INFO((120,"       GET(0x%x,%11d(0x%08x)) 0\tLFB\n",
                               iaddr,data,data));
  }
  else {                        
    /* check for valid register read */
    int reg;
    regInfo *ri;

    if (chip != 0)
      GDBG_ERROR("GET","CHIP #%d bad address=0x%x\n", chip,iaddr);
    reg = (iaddr >> 2) & 0xFF;       /* get offset into SST */
    ri = regsInfo+reg;
    if (reg < MAXREGADDR) {
      GDBG_INFO((120,"       GET(0x%x,%11d(0x%08x)) %d\t%s\n",
                                 iaddr,data,data, chip,ri->name));
    }
    else
      GDBG_ERROR("GET","bad register address=0x%x\n", reg<<2);
  }
  return data;
} /* _GR_GET */

void GR_CDECL
_GR_SET(void *addr, unsigned long data)
{
  GR_DCL_GC;
  FxU32 chip, iaddr = (FxU32) addr;

  iaddr = iaddr - (FxU32)gc->base_ptr + 0x10000000;
  if (iaddr < 0x10000000) {       /* sanity check */
    GDBG_ERROR("SET","bad address(<chip)=0x%x  data=%d(0x%08x)\n",
                     iaddr,data,data);
  }
  if (iaddr & 3)
    GDBG_ERROR("SET","unaligned address=0x%x\n", iaddr);

  if (iaddr & 0x800000) {        /* are we in TEX territory? */
    chip = ((iaddr) >> 21) & 0x3;       /* grab bits 22:21 */
    if (chip == 3) {
      if (((iaddr>>17)&0x3F) == 0x31) {
        GDBG_INFO((120,"       SET(0x%x,%11d(0x%08x)) %d\tPACKER\n",
                                   iaddr,data,data,chip));
      }
    }
    else {
        if (GDBG_GET_DEBUGLEVEL(195)) { /* optimized TEX data trace */
            if (iaddr == last_addr+4)
                GDBG_PRINTF(("T+4 %x\n",        data));
            else
                GDBG_PRINTF(("TEX %x %x\n",     iaddr,data));
        }
    }
  }
  else if (iaddr & 0x400000) {  /* are we in LFB territory? */
    GDBG_INFO((120,"       SET(0x%x,%11d(0x%08x)) 0\tLFB\n",
                               iaddr,data,data));
  }
  else {                        
    /* check for valid register read */
    int reg;
    regInfo *ri;

    chip = (iaddr >> 10) & 0xF;
    reg = (iaddr >> 2) & 0xFF;       /* get offset into SST */
    ri = regsInfo+reg;
    if (reg < MAXREGADDR) {
      GDBG_INFO((120,"       SET(0x%x,%11d(0x%08x)) %d\t%s\n",
                                 iaddr,data,data, chip,ri->name));
      /* [dBorca] HACK ALERT!!! Win32 version runs amok without this... */
      GR_P6FENCE;
    }
    else
      GDBG_ERROR("SET","CHIP #%d bad register address=0x%x, data=%d(x%x)\n",
                        chip,reg<<2,data,data);
  }
  last_addr = iaddr;
} /* _GR_SET */

void GR_CDECL
_GR_SETF(void *addr, float data)
{
  _GR_SET(addr,*(unsigned long *)&data);
} /* _GR_SETF */

void GR_CDECL
_GR_SET16(void *addr, unsigned short data)
{
  GR_DCL_GC;
  FxU32 iaddr = (FxU32)addr;
  iaddr = iaddr - (FxU32)gc->base_ptr + 0x10000000;
  GDBG_INFO((120,"       SET16(0x%x,%11d(0x%08x)) 0\tLFB16\n",iaddr,data,data));
  last_addr = 0;
} /* _GR_SET16 */

#endif /* defined(GLIDE_DEBUG) && !(GLIDE_PLATFORM & GLIDE_SST_SIM) */

#if defined( GLIDE_DEBUG ) && ( GLIDE_PLATFORM & GLIDE_HW_SST96 ) 

extern const char *regNames[];

static FxU32 thisMask;
static FxU32 thisWrite;
static FxU32 maxWrites;

void GR_CDECL
_GR_SET_GW_CMD( volatile void *addr, unsigned long data ) {
    GR_DCL_GC;

    /* validate pointer range */
    {
        FxU32 min = (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoVirt;
        FxU32 max = min + (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoMax;
        if ( ((FxU32)addr) < min || ((FxU32)addr) > max ) {
            GDBG_ERROR( "SET GW CMD", 
                        "Fifo address 0x%x out of range(0x%x, 0x%x)\n",
                        addr, 
                        gc->fifoData.hwDep.vg96FIFOData.fifoVirt,
                        gc->fifoData.hwDep.vg96FIFOData.fifoMax );
        }
                    
    }
    /* validate alignment */
    if ( ((FxU32)addr) & 0x7 ) {
        GDBG_ERROR( "SET GW CMD",
                    "Fifo address 0x%x is not 8 byte aligned\n",
                    addr );
    }
    /* validate enable bit */
    if ( ! (data&0x80000000) ) { 
        GDBG_ERROR( "SET GW CMD",
                    "Fifo command 0x%x missing flag bit\n",
                    data );
    }
    /* validate reserved bits */
    if ( data&0x7fE00300 ) { 
        GDBG_ERROR( "SET GW CMD",
                    "Fifo command 0x%x reserved bits set!\n",
                    data );
    }

    GDBG_INFO((128, 
               "Writing Group Write Command: 0x%x\n",
               data ));
    GDBG_INFO((128, 
               "  FIFO ADDR: 0x%.08x\n", addr ));
    /* dump chip field */
    {
        FxU32 field = ( (data<<2) & 0xf0000 ) >> 16;
        GDBG_INFO((128, "  Chip Field: 0x%x\n", field ));
    } 
    /* dump wrap field */
    {
        FxU32 field = ( (data<<2) & 0x0f000 ) >> 12;
        GDBG_INFO((128, "  Wrap Field: 0x%x\n", field ));
    }
    /* dump register */
    {
        FxU32 reg = ( data & 0xff );
        GDBG_INFO((128, "  Register:   %s(0x%x)\n", regNames[reg], reg ));
    }
    *(FxU32*)addr = data;
}

void GR_CDECL
_GR_SET_GW_HEADER( volatile void *addr, unsigned long data ) {
  GR_DCL_GC;

  /* validate pointer range */
  {
    FxU32 min = (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoVirt;
    FxU32 max = min + (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoMax;
    if ( ((FxU32)addr) < min || ((FxU32)addr) > max ) {
      GDBG_ERROR( "SET GW CMD", 
                 "Fifo address 0x%x out of range(0x%x, 0x%x)\n",
                 addr, 
                 gc->fifoData.hwDep.vg96FIFOData.fifoVirt,
                 gc->fifoData.hwDep.vg96FIFOData.fifoMax );
    }
                    
  }
  /* validate alignment */
  if ( ((FxU32)addr) & 0x3 ) {
    GDBG_ERROR( "SET GW HEADER",
               "Fifo address 0x%x is not DWORD aligned\n",
               addr );
  }
  GDBG_INFO((128, 
             "Writing Group Write Header: 0x%x\n",
             data ));
  GDBG_INFO((128, 
             "  FIFO ADDR: 0x%.08x\n", addr ));
  /* validate bits - at least one must be set*/
  {
    FxU32 val, bits, reg;
    val = data;
    bits = 0;
    reg  = 2;
    while( val ) {
      if ( val & 1 ) {
        bits++;
        GDBG_INFO((128, "  Register: %s\n", regNames[reg] ));
      }
      val>>=1;
      reg++;
    }
    GR_ASSERT( bits );
    GDBG_INFO((128, "  --------------\n" ));
    GDBG_INFO((128, "  Total Bits: %d\n", bits ));
    maxWrites = bits;
    if ( maxWrites & 1 ) maxWrites++;
  }
  *(FxU32*)addr = data;
  thisMask  = data;
  thisWrite = 0;
}

void GR_CDECL
_GR_SET_GW_ENTRY( volatile void *addr, unsigned long data ) {
  GR_DCL_GC;
  /* validate pointer range */
  {
    FxU32 min = (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoVirt;
    FxU32 max = min + (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoMax;
    if ( ((FxU32)addr) < min || ((FxU32)addr) > max ) {
      GDBG_ERROR( "SET GW CMD", 
                 "Fifo address 0x%x out of range(0x%x, 0x%x)\n",
                 addr, 
                 gc->fifoData.hwDep.vg96FIFOData.fifoVirt,
                 gc->fifoData.hwDep.vg96FIFOData.fifoMax );
    }
                    
  }
  /* validate alignment */
  if ( ((FxU32)addr) & 0x3 ) {
    GDBG_ERROR( "SET GW ENTRY",
               "Fifo address 0x%x is not DWORD aligned\n",
               addr );
  }
  /* validate write within packet */
  if ( !thisMask || thisWrite > maxWrites ) {
    GDBG_ERROR( "SET GW ENTRY",
               "Group write entry attempted outside of a packet\n" );
  }   
  GDBG_INFO((128, 
             "Writing Group Write Entry\n" ));
  GDBG_INFO((128, 
             "  FIFO ADDR: 0x%.08x\n", addr ));
  /* decode write */
  {
    FxU32 reg, bit, write;
    reg   = 2;
    bit   = 1;
    write = 0;
        
    while( bit ) {
      if ( bit & thisMask ) {
        if ( write == thisWrite ) break;
        write++;
      }
      bit <<= 1;
      reg++;
    }
        
    if ( bit ) {
      GDBG_INFO((128, 
                 "  REG: %s DATA: 0x%x\n",
                 regNames[reg],
                 data));
    } else {
      GDBG_INFO((128, 
                 "  PADDING\n" ));
    }
  }
  *(FxU32*)addr = data;
  thisWrite++;
}

#endif /* defined( GLIDE_DEBUG) && ( GLIDE_PLATFORM & GLIDE_HW_SST96 ) */

