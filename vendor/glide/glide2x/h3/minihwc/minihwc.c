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
** 4     3/17/99 6:14p Dow
** Fixes Phantom Menace, OGL, and others.
** 
** 122   3/17/99 3:46p Dow
** Luke Skywalker
** 
** 121   3/11/99 8:37p Peter
** update vidOverlayEndScreenCoord when the overlay dims != screen dims
** (w/ optional scaling)
** 
** 120   3/10/99 11:52a Peter
** initFifo enable hole counting parameter
** 
** 119   3/09/99 12:31p Kcd
** Added MacOS stuff.  Fixed refresh constant table (mis)use.
** 
** 118   3/05/99 10:06p Peter
** allocate independent state buffers in parallel w/ the command buffers
** 
** 117   3/03/99 3:52p Peter
** removed hemorrhaging GetDC and create glide private dc's for ExtEscape
** 
** 116   3/02/99 8:03p Dow
** 
** 115   2/27/99 12:24p Dow
** New resolutions
** 
** 114   2/22/99 5:06p Peter
** cleaned up monitor enum crap
** 
** 113   2/19/99 2:00p Atai
** fixed lfbBuffAddr for 800x600 triple buffering lfb access
** 
** 112   2/17/99 2:36p Peter
** removed extra dd call, fail agp fifo if no dd agp support, release
** window dc after enum
** 
** 111   2/13/99 1:57p Dow
** Added code for new resolutions
** 
** 110   2/08/99 6:42p Atai
** use the active window display as sst 0
** 
** 109   2/02/99 4:37p Peter
** hwGetSurfaceinfo gets depth, releasing memory fifo creation dd object
** 
** 108   1/25/99 6:37p Peter
** removed redundant macros
** 
** 107   1/22/99 4:28p Dow
** Fixed 1600x1200 the right way.
** 
** 106   1/22/99 3:02p Dow
** Turned off 2x2 filtering in 2x mode
** 
** 105   1/21/99 1:43p Dow
** Fencing for Pallette Download
** 
** 104   1/20/99 6:04p Peter
** cleaned up warning
** 
** 103   1/20/99 1:51p Dow
** Pithy workaround for PCI gamma problem
** 
** 102   1/14/99 11:38a Dow
** Left bits alone in vidproccfg
** 
** 101   1/11/99 8:22p Peter
** fixed stride vs width effage that I somehow convinced myself was right
** 
** 100   1/11/99 6:31p Dow
** Added Debugging
** 
** 99    1/04/99 11:58a Peter
** added windowed context support
** 
** 98    12/23/98 12:39p Peter
** hdc cleanup/hwcext_getlinearaddr is not idempotent
** 
** 97    12/22/98 5:43p Atai
** turn on 2x mode for vidProcCfg if greater than or equal to 1280
** 
** 96    12/22/98 3:29p Jeske
** use h3InitGetMemSize() instead so that we don't have to worry about it
** screwing up dramInit1 like h3InitSgram() did in the past...
** 
** 95    12/22/98 2:46p Atai
** fixed for Banshee/DOS draminit1 value
** 
** 94    12/22/98 1:07p Peter
** all dd surface pointers must go through linear map offset to get their
** hw relative offsets
** 
** 93    12/17/98 2:08p Jeske
** comment fix
** 
** 92    12/16/98 7:13p Peter
** query directdraw object for surface memory area caps and set the
** process id when getting the linear addresses via
** 
** 91    12/11/98 1:35p Peter
** window fifo serial # vs pixel cache
** 
** 90    12/09/98 5:17p Dow
** Fixed video scaling for lame resolutions
** 
** 89    12/07/98 2:18p Dow
** Added debug message for process handle
** 
** 88    12/07/98 11:32a Dow
** Fixed 2X resolutions
** 
** 87    12/06/98 11:04a Dow
** 
** 86    12/04/98 8:42a Dow
** Fixed broken DOS build AGAIN
** 
** 85    12/03/98 10:27p Dow
** Added stuff for hwcGetenv
** 
** 84    12/03/98 9:11p Dow
** Fixed resolution check
** 
** 83    12/02/98 9:35p Dow
** Query Resolutions
** 
** 82    12/02/98 2:41p Jeske
** ugh... text badness
** 
** 81    12/02/98 2:38p Jeske
** don't set the clocks unless they use the environment variable
** overrides, even for banshee...
** 
** 80    12/02/98 2:06p Dow
** Gamma Fully Implemented
** 
** 79    11/30/98 6:53p Peter
** video memory fifo's
** 
** 78    11/24/98 12:25p Mikec
** made multi-mon on win95 happy
** 
** 77    11/24/98 9:43a Atai
** detect multiple cards
** 
** 76    11/23/98 3:28p Atai
** fixed the refresh rate stuff
** 
** 75    11/20/98 4:48p Jeske
** avenger-dos, for now, make sure we don't call h3InitPlls() on avenger..
** 
** 74    11/19/98 1:51p Dow
** Fixed refresh/resolution issue
** 
** 73    11/18/98 8:04p Dow
** grxclk
** 
** 72    11/15/98 2:16a Atai
** set device id = 5. Hack for comdex
** 
** 71    11/10/98 6:29p Atai
** added min_tramSize for board information. min_tramSize = 0x200000 if
** device id is 3 or mem size is 4
** 
** 70    10/30/98 3:44p Dow
** Fixed Tiled/Linear color/aux bug
** 
** 69    10/29/98 5:15p Dow
** Fixed 512x384
** 
** 68    10/21/98 11:22a Peter
** dos happiness w/ the new chris pci library (nee tarolli)
** 
** 67    10/14/98 3:37p Dow
** Gamma stuff
** 
** 66    10/13/98 8:48p Dow
** Env Var for Board mem
** 
** 65    10/08/98 10:15a Dow
** Triple buffering fix
** 
** 64    9/18/98 3:08p Dow
** Fixed DOS build
** 
** 63    9/17/98 3:58p Dow
** Vidmode Stuff
** 
** 62    9/11/98 1:07p Peter
** rounding on raw lfb port addr
** 
** 61    9/02/98 1:34p Peter
** watcom warnings
** 
** 60    8/25/98 6:48p Dow
** Added scaling for low-res
** 
** 59    8/25/98 3:07p Dow
** Checked res of hwcRLSEXCLUSIVE
** 
** 58    8/06/98 7:49p Dow
** Moved detection of SDRAM
** 
** 57    8/02/98 5:00p Dow
** Glide Surface Extension
** 
** 56    7/29/98 3:34p Dow
** 
** 55    7/29/98 3:09p Dow
** SDRAM Fixes
** 
** 54    7/24/98 6:34p Dow
** Fixed DOS Build
** 
** 53    7/24/98 2:02p Dow
** AGP Stuff
** 
** 52    7/23/98 1:18a Dow
** Bump & Grind
** 
** 51    7/18/98 12:21a Jdt
** added state abuffer
** 
** 50    7/16/98 10:26p Dow
** GIW Stuff
** 
** 49    7/16/98 2:53p Dow
** Removed useless colBufferAddr frmo surface info
** 
** 48    7/15/98 4:09p Dow
** GIW Stuff & DOS Protection
** 
** 47    7/14/98 7:19p Dow
** protected some win32 stuff
** 
** 46    7/13/98 10:35p Jdt
** Implemented hwcAllocWinFifo, hwcExecuteWinFifo.
** 
** 45    7/08/98 5:45p Dow
** Moved fifo back up 
** 
** 44    7/02/98 12:11p Dow
** LFB fixes
** 
** 43    6/30/98 10:28a Dow
** Fixed fouled checkin
** 
** 41    6/25/98 6:45p Jdt
** Changes to remove the evil of dxdrvr.c
** 
** 40    6/25/98 12:16p Dow
** Added (protected) alternate FIFO layout
** 
** 39    6/16/98 6:11p Dow
** Rearranged texture memory
** 
** 38    6/16/98 9:49a Dow
** Fixed protected stuff
** 
** 37    6/16/98 9:35a Dow
** Comment -> GDBG_INFO conversion
** 
** 36    6/12/98 10:09a Jdt
** Fix broken dos build.
** 
** 35    6/11/98 7:44p Jdt
** Win98/NT5 Mulitmon 1st Pass
** 
** 34    6/10/98 9:49a Peter
** lfb buffer addressing
** 
** 33    6/09/98 10:09a Dow
** Init register performance tweaks.
** 
** 32    6/06/98 11:42a Dow
** Better buffer alignment for all resolutions.
** 
** 31    6/06/98 10:11a Dow
** Changed AUXPAD from 0 to 0x1000 to offset aux buffer.
** 
** 30    6/05/98 7:34p Dow
** Fixed 8x6-12x10 resolutions
** 
** 29    6/05/98 6:21p Jeske
** now everyone will dxClose(), not just windows
** 
** 28    6/05/98 6:37p Dow
** 1600x1200 perf tuning
** 
** 27    6/05/98 5:28p Jeske
** apparently we need to not call h3InitSetVideoMode() for this to work,
** will this work in non640x480 resolutions? we may need to fix that later
** 
** 26    6/05/98 3:27p Dow
** 1600x1200 Works
** 
** 25    6/04/98 9:36p Jeske
** now we leave VGA legacy decoding on in hwcInitRegisters so we can set
** video modes
** 
** 24    6/04/98 6:52p Dow
** Resolutions to 1600x1200
** 
** 23    6/03/98 1:52p Jeske
** added code to convert from Glide's ordinal refresh number to a refresh
** rate in Hz when calling h3InitSetVideoMode(). We really should specify 
** 
** 22    6/01/98 5:48p Jeske
** allow h3InitSetVideoMode to fail and handle it...
** 
** 21    5/31/98 9:02a Dow
** 800x600 Resolution
** 
** 20    5/28/98 6:34p Dow
** Fixed top scanline bug
** 
** 19    5/27/98 3:35p Mc
** Removed blocks of HW_IO_LOAD and HW_IO_STORE that chris put in.
** 
** 18    5/20/98 8:13p Dow
** device rev
** 
** 17    5/20/98 4:16p Dow
** Added env var for FIFO size
** 
** 16    5/15/98 2:46p Dow
** Attempt to patch back regs on NT
** 
** 15    5/13/98 4:26p Dow
** Protected obsolete IO code that was bodging NT
** 
** 14    4/27/98 2:30p Dow
** OpenProcess stuff
** 
** 13    4/22/98 5:29p Dow
** Added calls to HWCEXT_HWCSETEXCLUSIVE and HWCEXT_HWCRLSEXCLUSIVE
** 
** 12    4/16/98 10:14p Dow
** EXT_HWC is default init method
** 
** 11    4/13/98 5:49p Dow
** Turning theory into practice
** 
** 10    4/09/98 9:34p Dow
** synched with change in h3cinit
** 
** 9     4/08/98 12:25p Dow
** LfbMemoryConfig fix
** 
** 8     4/07/98 10:40p Dow
** LFB Fixes
** 
** 7     4/05/98 2:19p Dow
** DOS/WIN32 stepping on each others' dicks
** 
** 6     4/03/98 2:03p Dow
** DOS Glide modes
** 
** 5     3/28/98 10:51a Dow
** Fixes for FIFO bug
** 
** 4     3/20/98 1:11p Dow
** Now checking revision of chip
** 
** 3     3/20/98 11:43a Dow
** fifo placement for windows
** 
** 2     3/11/98 8:27p Dow
** WinGlide
** 
** 1     3/04/98 4:13p Dow
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <3dfx.h>

#ifdef HWC_EXT_INIT
#include "hwcext.h"
#else
#include <fxpci.h>
#endif

#include <h3cinit.h>
#include <minihwc.h>
#include "hwcio.h"
#include "setmode.h"

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <ddraw.h>
#include "qmodes.h"
#ifdef HAVE_WIN9X_DDK
#define IS_32
#define Not_VxD
#include <minivdd.h>
#include <vmm.h>
#include <configmg.h>
#else
#define CM_REGISTRY_HARDWARE 0
#define CM_REGISTRY_SOFTWARE 1
#define CR_FAILURE 0x00000013
#define ___CONFIGMG_Get_DevNode_Key 0x0033003d
struct _CMIOCTLPACKET {
 DWORD dwStack;
 DWORD dwServiceNumber;
};
static DWORD WINAPI CMIOCTLHandler(struct _CMIOCTLPACKET *pkt)
{
  HANDLE hCONFIGMG;
  DWORD crReturnValue = CR_FAILURE;
  DWORD dwReturnSize = 0;

  hCONFIGMG = CreateFileA(
    "\\\\.\\CONFIGMG",
    GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_READ|FILE_SHARE_WRITE,
    NULL, OPEN_EXISTING, 0, NULL);

  if (hCONFIGMG == INVALID_HANDLE_VALUE) {
      return CR_FAILURE;
  }
  if (!DeviceIoControl(
        hCONFIGMG, pkt->dwServiceNumber,
        &(pkt->dwStack), sizeof(pkt->dwStack),
        &crReturnValue, sizeof(crReturnValue),
        &dwReturnSize, NULL)) {
    crReturnValue = CR_FAILURE;
  }
  CloseHandle(hCONFIGMG);
  if (dwReturnSize != sizeof(crReturnValue)) {
      crReturnValue = CR_FAILURE;
  }
  return crReturnValue;
}
static DWORD __cdecl CM_Get_DevNode_Key (DWORD devnode, char *subkey, void *buffer, ULONG bufferlen, ULONG flags)
{
    struct _CMIOCTLPACKET packet;
    DWORD dwStack;
    #if defined(_MSC_VER)
    _asm {mov dwStack, ebp};
    #elif defined(__GNUC__)
    dwStack = (DWORD) __builtin_frame_address(0);
    #else
    #error Add support for your compiler here.
    #endif
    dwStack += 8;
    packet.dwStack = dwStack;
    packet.dwServiceNumber = 0x80000000 + (___CONFIGMG_Get_DevNode_Key & 0xFFFF);
    return CMIOCTLHandler(&packet);
}
#endif

#endif /* __WIN32__ */

#ifdef macintosh
#include <GraphicsPrivHwc.h>
#include <mac_sfc.h>
#endif

#ifdef GETENV
#undef GETENV
#endif

#define GETENV hwcGetenv

#define MAXFIFOSIZE     0x40000

#if defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))
# define __attribute_used __attribute__((__used__))
#elif defined(__GNUC__) && (__GNUC__ >= 2)
# define __attribute_used __attribute__((__unused__))
#else
# define __attribute_used
#endif

static hwcInfo hInfo;
static char errorString[1024];
static FxU32 __attribute_used fenceVar;

/*
 *  P6 Fence
 *
 *  Here's the stuff to do P6 Fencing.  This is required for the
 *  certain things on the P6
 *
 * dpc - 21 may 1997 - FixMe!
 * This was yoinked from sst1/include/sst1init.h, and should be
 * merged back into something if we decide that we need it later.
 */
#if defined(__WATCOMC__)
void p6Fence(void);
#pragma aux p6Fence = \
 "xchg eax, fenceVar" \
 modify [eax];
#define P6FENCE p6Fence()
#elif defined(__MSC__)
#define P6FENCE {_asm xchg eax, fenceVar}
#elif defined(__POWERPC__) && defined(__MWERKS__)
#define P6FENCE __eieio()
#elif defined(__DJGPP__) || defined (__MINGW32__)
#define P6FENCE __asm __volatile ("xchg %%eax, _fenceVar":::"%eax")
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define P6FENCE __asm __volatile ("xchg %%eax, fenceVar":::"%eax")
#elif defined(__GNUC__) && defined(__ia64__)
# define P6FENCE asm volatile ("mf.a" ::: "memory");
#elif defined(__GNUC__) && defined(__alpha__)
# define P6FENCE asm volatile("mb" ::: "memory");
#else
#error "P6 Fencing in-line assembler code needs to be added for this compiler"
#endif /* Compiler specific fence commands */

/* Hack */
#if macintosh
#define __DOS32__ 1
#endif

static FxU32
pow2Round(FxU32 val, FxU32 roundVal);

static FxU32
hwcBufferLfbAddr(FxU32 bufNum, 
                 const hwcBoardInfo* bInfo,
                 FxBool colBufAlignP);

static FxU32
calcBufferSize(FxU32 xres, FxU32 yres, FxBool tiled);  

static FxU32
calcBufferStride(FxU32 xres, FxBool tiles);

static FxU32
calcBufferSizeInTiles(FxU32 xres, FxU32 yres);

static FxU32
calcBufferHeightInTiles(FxU32 yres);

static FxBool resolutionSupported[HWC_MAX_BOARDS][0xF];

/*
**  Function Prototypes
*/
#ifdef HWC_EXT_INIT

#if (WINVER < 0x0500) && !defined(HMONITOR_DECLARED) /* <--- HACK */
DECLARE_HANDLE(HMONITOR);
#define HMONITOR_DECLARED
#endif
typedef BOOL (CALLBACK* MONITORENUMPROC)(HMONITOR, HDC, LPRECT, LPARAM);
typedef BOOL (WINAPI *EnumDisplayMonitors_func)
                        ( HDC             hdc,
                          LPCRECT         lprcClip,
                          MONITORENUMPROC lpfnEnum,
                          LPARAM          dwData);

typedef struct {
  HDC dc;
  HMONITOR mon;
} DevEnumRec;
static int num_monitor = 0;

static BOOL CALLBACK 
monitorEnum( HMONITOR handle, HDC dc, LPRECT rect, LPARAM param ) 
{
  BOOL rv = TRUE;
  hwcExtRequest_t
    ctxReq;
  hwcExtResult_t
    ctxRes;

  ctxReq.which = HWCEXT_GETDEVICECONFIG;

  GDBG_INFO(80, "monitorEnum:  ExtEscape:HWCEXT_GETDEVICECONFIG\n");
  if (  ExtEscape(dc, EXT_HWC, sizeof(ctxReq), (LPSTR) &ctxReq, sizeof(ctxRes), (LPSTR) &ctxRes) ) {
    if ( ctxRes.optData.deviceConfigRes.vendorID == 0x121a ) {
      DevEnumRec* 
        data = (DevEnumRec*)param;
      LPCSTR
        drvName = "DISPLAY",
        devName = NULL;

      /* If we're on a multi-mon capable system then we may have
       * different display type devices so we have to get the device
       * name explicitly for the CreateDC call.
       */
      if (handle != NULL) {
#define CCHDEVICENAME 32
        typedef struct {
          DWORD cbSize;
          RECT  rcMonitor;
          RECT  rcWork;
          DWORD dwFlags;
          TCHAR szDevice[CCHDEVICENAME];
        } MONITORINFOEX, *LPMONITORINFOEX;
        typedef BOOL (CALLBACK* GetMonitorInfoProc)(HMONITOR, LPMONITORINFOEX);
        static GetMonitorInfoProc monitorInfoProc = NULL;

        if (monitorInfoProc == NULL) {
          HMODULE user32 = GetModuleHandle("user32");

          if (user32 != NULL) {
            monitorInfoProc = (GetMonitorInfoProc)GetProcAddress(user32, "GetMonitorInfoA");
          }
        }

        if (monitorInfoProc != NULL) {
          MONITORINFOEX monInfo;

          monInfo.cbSize = sizeof(monInfo);
          if ((*monitorInfoProc)(handle, &monInfo)) {
            devName = monInfo.szDevice;
            drvName = NULL;
          }
        }
      }

      /* Make a private 'copy' of the dc so that we're not affected by
       * other people dorking with dc's etc. 
       *
       * FixMe: Is there a better way to do this? I did not see a
       * CopyDC or anything like that.  
       */
      dc = CreateDC(drvName,
                    devName,
                    NULL,
                    NULL);

      data[num_monitor].dc = dc;
      data[num_monitor].mon = handle;      

      num_monitor++;
      rv = (num_monitor < HWC_MAX_BOARDS);
    }
  }

  return rv;
} /* monitorEnum */

/*
** Use the active window display as the first sst device
*/
static BOOL CALLBACK 
displayMonitor( HMONITOR handle, HDC dc, LPRECT rect, LPARAM param ) 
{
  BOOL rv = TRUE;
  hwcExtRequest_t
    ctxReq;
  hwcExtResult_t
    ctxRes;

  ctxReq.which = HWCEXT_GETDEVICECONFIG;

  GDBG_INFO(80, "displayMonitor:  ExtEscape:HWCEXT_GETDEVICECONFIG\n");
  if (  ExtEscape(dc, EXT_HWC, sizeof(ctxReq), (LPSTR) &ctxReq, sizeof(ctxRes), (LPSTR) &ctxRes) ) {
    if ( ctxRes.optData.deviceConfigRes.vendorID == 0x121a ) {
      DevEnumRec*
        data   = (DevEnumRec*) param;
      FxI32 
        i;

      for (i = 0; i < num_monitor; i++) {
        HDC      *return_dc   = ( HDC* ) data + (2*i);
        HMONITOR *return_hmon = ( HMONITOR* ) data + (2*i+1);

        if ((data[i].dc == dc) && (data[i].mon == handle)) {
          data[i].dc = data[0].dc;
          data[i].mon = data[0].mon;

          data[0].dc = dc;
          data[0].mon = handle;

          break;
        }
      }
      rv = FALSE;
    }
  } 
  return rv;
} /* displayMonitor */
#endif

hwcInfo *
hwcInit(FxU32 vID, FxU32 dID)
{
#define FN_NAME "hwcInit"
#ifdef HWC_EXT_INIT
  {
    DevEnumRec
      data[HWC_MAX_BOARDS*2];
    int monitor;

    GDBG_INFO(80, "%s\n", FN_NAME);
    errorString[0] = '\0';

    /* find glide compatible devices */
    GDBG_INFO(80, "%s:  Finding Glide compatible devices\n", FN_NAME);
    {
      /* Grab the DC of the Desktop. */
      HDC hdc = GetDC(NULL);
      HMODULE user32 = GetModuleHandle( "user32.dll" );

      for (monitor = 0; monitor < HWC_MAX_BOARDS; monitor++) {
        data[monitor].dc  = NULL;
        data[monitor].mon = NULL;
      }
      num_monitor = 0;

      if ( user32 ) {
        EnumDisplayMonitors_func enumDisplayMonitors =
          (EnumDisplayMonitors_func)GetProcAddress( user32, "EnumDisplayMonitors" );
        
        if ( enumDisplayMonitors ) { 
          HWND
            curWindow = GetActiveWindow();
          
          GDBG_INFO(80, "%s:  multi-monitor capable OS ( NT5/W98 )\n", FN_NAME);
          enumDisplayMonitors( hdc, 0, monitorEnum, (LPARAM)data );
          
          /*
          ** use the active window display (if there is one yet
          ** associated w/ the current thread) as sst 0 
          */
          if (curWindow != NULL) {
            HDC curWindowDC = GetDC(curWindow);
            
            if (curWindowDC != NULL) {
              enumDisplayMonitors( curWindowDC, 0, displayMonitor, (LPARAM)data );
              ReleaseDC(curWindow, curWindowDC);
            }
          }
        } else { /* for win95/nt4, assume we have one board */
          monitorEnum(NULL, hdc, NULL, (LPARAM)&data);
        }
      }

      ReleaseDC(NULL, hdc);
    }

    hInfo.nBoards = 0;
    for (monitor = 0; monitor < num_monitor; monitor++) {
      hwcExtRequest_t
        ctxReq;
      hwcExtResult_t
        ctxRes;
      HDC 
        hdc = data[monitor].dc;
      HMONITOR
        hmon = data[monitor].mon;
      int 
        status;
      
      /* Allocate a context with the Driver */
      ctxReq.which = HWCEXT_ALLOCCONTEXT;
      ctxReq.optData.allocContextReq.protocolRev = HWCEXT_PROTOCOLREV;
      ctxReq.optData.allocContextReq.appType = HWCEXT_ABAPPTYPE_FSEM;
      
      GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_ALLOCCONTEXT\n");
      status = ExtEscape(hdc, EXT_HWC, 
                         sizeof(ctxReq), (LPSTR) &ctxReq,
                         sizeof(ctxRes), (LPSTR) &ctxRes); 
      
      if (status < 1) {
        if (status == 0) {
          strcpy(errorString, "HWCEXT Not Implemented");
        } else {
          strcpy(errorString, "HWCEXT: Alloc Context Failed");
        }
        GDBG_INFO(80, "%s: %s.\n", FN_NAME, errorString);
        return NULL;      
      }
      
      hInfo.boardInfo[monitor].extContextID = ctxRes.optData.allocContextRes.contextID;
      
      hInfo.nBoards++;
      hInfo.boardInfo[monitor].boardNum = monitor;
      hInfo.boardInfo[monitor].hdc  = hdc;
      hInfo.boardInfo[monitor].hMon = hmon;
      
      ctxReq.which = HWCEXT_GETDEVICECONFIG;

      GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_GETDEVICECONFIG.\n");
      status = ExtEscape(hdc, EXT_HWC, 
                         sizeof(ctxReq), (void *) &ctxReq,
                         sizeof(ctxRes), (void *) &ctxRes); 
      
      hInfo.boardInfo[monitor].pciInfo.initialized = 1;
      hInfo.boardInfo[monitor].pciInfo.vendorID =
        ctxRes.optData.deviceConfigRes.vendorID;

      hInfo.boardInfo[monitor].pciInfo.deviceID = 
        ctxRes.optData.deviceConfigRes.deviceID;
      hInfo.boardInfo[monitor].devRev = 
        ctxRes.optData.deviceConfigRes.chipRev;
      hInfo.boardInfo[monitor].h3Mem = 
        (ctxRes.optData.deviceConfigRes.fbRam >> 20);

      hInfo.boardInfo[monitor].min_tramSize = 
        ((hInfo.boardInfo[monitor].h3Mem == 4 ) ||
         (hInfo.boardInfo[monitor].pciInfo.deviceID == 3)) ? 0x200000 : 0x400000;
      
      if (getenv("FX_GLIDE_FBRAM")) {
        hInfo.boardInfo[monitor].h3Mem = atoi(getenv("FX_GLIDE_FBRAM"));      
      }

      checkResolutions(resolutionSupported[monitor],
                       (void *) hInfo.boardInfo[monitor].hMon);
    }
  }
#elif defined(HWC_GDX_INIT)
  {
    GDHandle screenDeviceHandle;
    CntrlParam  paramBlock;
    OSErr       myErr;
    FxU32 i = 0;
    hwcControl_t control;
    
    hInfo.nBoards = 0;
    
    screenDeviceHandle = DMGetFirstScreenDevice(dmOnlyActiveDisplays);
    while(screenDeviceHandle) {             
      /* First, verify that it's a screen device */
      if(TestDeviceAttribute(screenDeviceHandle,screenDevice)) {
        /* Then verify it actually has a driver, so we know gdRefNum
           is valid. */ 
        if(!TestDeviceAttribute(screenDeviceHandle,noDriver)) {
          /* HLock((Handle)screenDeviceHandle); */
          paramBlock.ioCRefNum = (*screenDeviceHandle)->gdRefNum;
          /* HUnlock((Handle)screenDeviceHandle); */
          /* driver-specific status request */
          paramBlock.csCode = k3DfxGetDeviceConfig; 
          *(hwcControl_t **)(&paramBlock.csParam[0]) = &control;
                        
          myErr = PBControl((ParmBlkPtr)&paramBlock, false);
                        
          if (myErr == noErr) {
            /* Woohoo! Save off the config info for this card */
            hInfo.nBoards++;
            hInfo.boardInfo[i].hMon = screenDeviceHandle;
            hInfo.boardInfo[i].devRev =
              control.res.optData.deviceConfigRes.devRev;
            hInfo.boardInfo[i].h3Mem =
              control.res.optData.deviceConfigRes.h3Mem;
            hInfo.boardInfo[i].pciInfo.vendorID =
              control.res.optData.deviceConfigRes.vendorID;
            hInfo.boardInfo[i].pciInfo.deviceID =
              control.res.optData.deviceConfigRes.deviceID;
            hInfo.boardInfo[i].pciInfo.pciBaseAddr[0] =
              control.res.optData.deviceConfigRes.hwBase; 
            hInfo.boardInfo[i].pciInfo.pciBaseAddr[1] =
              control.res.optData.deviceConfigRes.lfbBase; 
            hInfo.boardInfo[i].pciInfo.pciBaseAddr[2] =
              control.res.optData.deviceConfigRes.ioPortBase;
            hInfo.boardInfo[i].pciInfo.initialized = 1;
                                
            /* No ROM info, don't care */
            i++;
          }
        }
      }
      screenDeviceHandle = DMGetNextScreenDevice(screenDeviceHandle,dmOnlyActiveDisplays);
    }
  }
#else /* HWC_GDX_INIT */
  {
    int i;
    FxU32 bn;

    pciOpen();

    hInfo.nBoards = 0;

    errorString[0] = '\0';

    for (i = 0; i < HWC_MAX_BOARDS; i++) {
      hInfo.boardInfo[i].pciInfo.initialized = 0;
      if (pciFindCardMulti(vID, dID, &bn, i)) {
        hInfo.nBoards++;
        hInfo.boardInfo[i].boardNum = 0;

        hInfo.boardInfo[i].boardNum = 0;

        hInfo.boardInfo[i].pciInfo.initialized = 1;
        hInfo.boardInfo[i].pciInfo.vendorID = vID;
        hInfo.boardInfo[i].pciInfo.deviceID = dID;
        /* 
         * NOTE: in the code above we learn about memsize here:
         *   hInfo.boardInfo[i].h3Mem
         * 
         * However, in DOS, since we have not mapped the board yet, we have
         * to wait until later. (see hwcInitRegisters())  - dwj
         */

        /* Get some board Info */
        pciGetConfigData( PCI_REVISION_ID, bn, &hInfo.boardInfo[i].devRev);

        /* Get all the base addresses */
        pciGetConfigData(PCI_BASE_ADDRESS_0, bn,
                         &hInfo.boardInfo[i].pciInfo.pciBaseAddr[0]);
        pciGetConfigData(PCI_BASE_ADDRESS_1, bn,
                         &hInfo.boardInfo[i].pciInfo.pciBaseAddr[1]);
        pciGetConfigData(PCI_IO_BASE_ADDRESS, bn,
                         &hInfo.boardInfo[i].pciInfo.pciBaseAddr[2]);
        pciGetConfigData(PCI_ROM_BASE_ADDRESS, bn,
                         &hInfo.boardInfo[i].pciInfo.pciBaseAddr[3]);
      }
    }
    if (!hInfo.nBoards) {
      const char *error = pciGetErrorCode() ? pciGetErrorString() :
                            "Voodoo3 or Banshee not detected\n";
      strcpy(errorString, error);
    }
  }
#endif /* HWC_EXT_INIT */
  if (hInfo.nBoards)
    return &hInfo;
  else
    return NULL;

#undef FN_NAME
} /* hwcInit */

FxBool
hwcMapBoard(hwcBoardInfo *bInfo, FxU32 bAddrMask)
{
#define FN_NAME "hwcMapBoard"

  if (bInfo->pciInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcInit\n", FN_NAME);
    return FXFALSE;
  }

  bInfo->linearInfo.initialized = FXTRUE;

#ifdef HWC_EXT_INIT
  {
    hwcExtRequest_t req;
    hwcExtResult_t res;

    req.which = HWCEXT_GETLINEARADDR;
    req.optData.linearAddrReq.devNum = 0;
    req.optData.linearAddrReq.pHandle = (FxU32)OpenProcess(PROCESS_ALL_ACCESS, 
                                                           FALSE, 
                                                           GetCurrentProcessId());

    GDBG_INFO(80, FN_NAME ":  Process Handle = 0x%x\n",
              req.optData.linearAddrReq.pHandle);

    GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_GETLINEARADDR\n");
    ExtEscape((HDC) bInfo->hdc, EXT_HWC, sizeof(req), (void *) &req,
      sizeof(res), (void *) &res);

    if (res.resStatus != 1) {
      strcpy(errorString, "HWCEXT_GETLINEARADDR Failed");
      return FXFALSE;
    }
    bInfo->linearInfo.linearAddress[0] =
      res.optData.linearAddressRes.baseAddresses[0];
    bInfo->linearInfo.linearAddress[1] =
      res.optData.linearAddressRes.baseAddresses[1];
    
    /* What is this????? */
    bInfo->pciInfo.pciBaseAddr[2] = 
      res.optData.linearAddressRes.baseAddresses[1];
  }
#elif defined(HWC_GDX_INIT)
  {
    FxU32
      bAddr;

    /* memory mapped register spaces */
    for (bAddr = 0; bAddr < 2; bAddr++) {
      if ((bAddrMask & (0x01UL << bAddr)) != 0x00UL) {
        bInfo->linearInfo.linearAddress[bAddr] = bInfo->pciInfo.pciBaseAddr[bAddr];
      }
    }
    /* FixMe: This gets used to set the pll's so I guess we need it here
     * unconditionally
     */
    bInfo->linearInfo.linearAddress[2] = bInfo->pciInfo.pciBaseAddr[2];
    
    /* Sorry, ROM mapping on MacOS seems whacked out for some reason. */
    bInfo->linearInfo.linearAddress[3] = 0;
    
    /* Kludge.  Pass boardInfo to acceleration stuff. */
#if GLIDE3      
    {
      extern hwcBoardInfo *acceleratorBoardInfo;
      acceleratorBoardInfo = bInfo;
    }
#endif  
  }
#else
  {
    FxU32
      bAddr;

    /* memory mapped register spaces */
    for (bAddr = 0; bAddr < 2; bAddr++) {
      if ((bAddrMask & (0x01UL << bAddr)) != 0x00UL) {
        bInfo->linearInfo.linearAddress[bAddr] = 
          (FxU32)pciMapCardMulti(bInfo->pciInfo.vendorID, bInfo->pciInfo.deviceID,
                                 0x1000000, &bInfo->deviceNum, bInfo->boardNum, bAddr);
      }
    }

    /* FixMe: This gets used to set the pll's so I guess we need it here
     * unconditionally
     */
    bInfo->linearInfo.linearAddress[2] = 
      (FxU32)pciMapCardMulti(bInfo->pciInfo.vendorID, bInfo->pciInfo.deviceID,
                             0x1000000, &bInfo->deviceNum, bInfo->boardNum, 2);

    /* Does the caller want the rom bios? */
    if ((bAddrMask & 0x08UL) != 0x00UL) {
      bInfo->linearInfo.linearAddress[3] = 
        (FxU32)pciMapCardMulti(bInfo->pciInfo.vendorID, bInfo->pciInfo.deviceID,
                               0x1000000, &bInfo->deviceNum, bInfo->boardNum, 3);
    }
  }
#endif
  
  return FXTRUE;
#undef FN_NAME
} /* hwcMapBoard */

FxBool
hwcInitRegisters(hwcBoardInfo *bInfo)
{
#define FN_NAME "hwcInitRegisters"
  FxU32
    grxSpeedInMHz, memSpeedInMHz,
    sgramMode, sgramMask, sgramColor;

  if (bInfo->linearInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcMapBoard\n", FN_NAME);
    return FXFALSE;
  }

  bInfo->regInfo.initialized = FXTRUE;

  bInfo->regInfo.ioMemBase =
    bInfo->linearInfo.linearAddress[0] + SST_IO_OFFSET;
  bInfo->regInfo.cmdAGPBase =
    bInfo->linearInfo.linearAddress[0] + SST_CMDAGP_OFFSET;
  bInfo->regInfo.waxBase =
    bInfo->linearInfo.linearAddress[0] + SST_2D_OFFSET;
  bInfo->regInfo.sstBase =
    bInfo->linearInfo.linearAddress[0] + SST_3D_OFFSET;
  bInfo->regInfo.lfbBase =
    bInfo->linearInfo.linearAddress[0] + SST_LFB_OFFSET;
  bInfo->regInfo.rawLfbBase =
    bInfo->linearInfo.linearAddress[1];
  bInfo->regInfo.ioPortBase = (FxU16) bInfo->pciInfo.pciBaseAddr[2] & ~0x1;

  /* Figure out if it's SDRAM */
  {
    FxU32 dramInit1;
    
    HWC_IO_LOAD(bInfo->regInfo, dramInit1, dramInit1);
    bInfo->sdRAM = ((dramInit1 & SST_MCTL_TYPE_SDRAM) != 0x00UL);

    if (GETENV("SSTH3_SDRAM"))
      bInfo->sdRAM = FXTRUE;
  }

#if !defined(HWC_EXT_INIT) && !defined(HWC_GDX_INIT)
  {
    FxU32 
      pciInit0,
      pciCommandReg =
        BIT(0) |              /* enable i/o decode */
        BIT(1);               /* enable memory decode */
  
    /* Enable PCI memory and I/O decode */
    pciSetConfigData(PCI_COMMAND, bInfo->deviceNum, &pciCommandReg);
    
    HWC_IO_LOAD(bInfo->regInfo, pciInit0, pciInit0);
    pciInit0 |= SST_PCI_READ_WS | SST_PCI_WRITE_WS;
    HWC_IO_STORE(bInfo->regInfo, pciInit0, pciInit0);  
  }
#endif

  if (GETENV("SSTH3_SGRAM_MODE"))
    sgramMode = atoi(GETENV("SSTH3_SGRAM_MODE"));
  else if (GETENV("SSTH3_SGRAM_222") &&
    (atoi(GETENV("SSTH3_SGRAM_222")) != 0))
    sgramMode = 0x27;
  else
    sgramMode = 0x37;

  if (GETENV("SSTH3_SGRAM_MASK"))
    sgramMask = atoi(GETENV("SSTH3_SGRAM_MASK"));
  else
    sgramMask = 0xFFFFFFFF;

  if (GETENV("SSTH3_SGRAM_COLOR"))
    sgramColor = atoi(GETENV("SSTH3_SGRAM_COLOR"));
  else
    sgramColor = 0;

  if (GETENV("SSTH3_GRXCLOCK"))
    grxSpeedInMHz = atoi(GETENV("SSTH3_GRXCLOCK"));
  else
    grxSpeedInMHz = 100;

  if (GETENV("SSTH3_MEMCLOCK"))
    memSpeedInMHz = atoi(GETENV("SSTH3_MEMCLOCK"));
  else
    memSpeedInMHz = 100;

#if !defined(HWC_ACCESS_DDRAW) && !defined(HWC_GDX_INIT)
  if (GETENV("HAL_NOINIT") == NULL || atoi(GETENV("HAL_NOINIT")) == 0) {

    /* 
     * final DOS initialiation
     */

    /* don't set the clocks unless they used the environment variables */

    if (GETENV("SSTH3_GRXCLOCK") || GETENV("SSTH3_MEMCLOCK")) {
     switch (bInfo->pciInfo.deviceID) {
        case 0x03: /* banshee */
           h3InitPlls(bInfo->regInfo.ioPortBase,  grxSpeedInMHz, memSpeedInMHz);
           break;
        case 0x5: /* voodoo3/avenger */
           break;
        default: /* unknown board type!!! */
           return FXFALSE;
     }
    }

    /* read back the memory size, since we 
     * don't know it under DOS  (see hwcInit) - dwj 
     */
    bInfo->h3Mem = h3InitGetMemSize(bInfo->regInfo.ioPortBase);

    h3InitVga(bInfo->regInfo.ioPortBase, FXTRUE); 
  }
#endif
  
  return FXTRUE;

#undef FN_NAME
} /* hwcInitRegisters */

#define FIFOPAD         0x0000
#define AUXPAD          0x1000

FxBool
hwcAllocBuffers(hwcBoardInfo *bInfo, FxU32 nColBuffers, FxU32 nAuxBuffers) 
{
#define FN_NAME "hwcAllocBuffers"
  FxBool
    bufferAlignP;
  FxU32
    bNum,
    h3Mem = bInfo->h3Mem << 20,
    bufStride, 
    bufSize;
  FxI32
    i,
    tramSize, fifoSize;

  if (bInfo->vidInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before video initialization\n", FN_NAME);
    return FXFALSE;
  }
    
  GDBG_INFO(80, "%s(0x%x, 0x%x, 0x%x)\n", FN_NAME, bInfo, nColBuffers, nAuxBuffers);

  /* I've decided on > 2 instead of == 3 because we may support more
    than 3 buffers in the future, and want 4 to set the
    triple-buffering bit in dramInit1, also */
  bInfo->vidInfo.tripleBuffering = (nColBuffers > 2);

  bInfo->vidInfo.stride = bufStride =
    calcBufferStride(bInfo->vidInfo.xRes, bInfo->vidInfo.tiled);   

  /* We want to place the FIFO after the tram but before the color
     buffers with some pad */
  bufSize = calcBufferSize(bInfo->vidInfo.xRes, bInfo->vidInfo.yRes,
    bInfo->vidInfo.tiled); 

  bInfo->buffInfo.bufStride = bufStride;
  bInfo->buffInfo.bufSize = bufSize;

  if (bInfo->vidInfo.tiled) {
    bInfo->buffInfo.bufStrideInTiles = (bufStride >> 7);
    bInfo->buffInfo.bufSizeInTiles =
      calcBufferSizeInTiles(bInfo->vidInfo.xRes, bInfo->vidInfo.yRes);
    bInfo->buffInfo.bufHeightInTiles =
      calcBufferHeightInTiles(bInfo->vidInfo.yRes);
  }

  bInfo->buffInfo.initialized = FXTRUE;
  bInfo->buffInfo.nColBuffers = nColBuffers;
  bInfo->buffInfo.nAuxBuffers = nAuxBuffers;

  /* First, do the buffer allocation */


  if (nAuxBuffers > 0) {
    bInfo->buffInfo.auxBuffEnd  =
    bInfo->buffInfo.auxBuffStart = h3Mem;

    bInfo->buffInfo.auxBuffStart -= bufSize;

    /* auxBuffers start on odd pages, so we need to check to see if
     * it's on an even page and, if so, make it odd. 
     *
     * NB: We need to do the same sort of 'alignment' thing here as for
     * the color buffers.
     */
    bufferAlignP = ((bInfo->buffInfo.auxBuffStart & 0x1000UL) == 0);
    if (bufferAlignP) bInfo->buffInfo.auxBuffStart -= 0x1000;

  }

  for (i = nColBuffers - 1; i >= 0; i--) {
    if ((FxU32) i == (nColBuffers - 1)) {
      FxU32 top;
      if (nAuxBuffers > 0) 
        top = bInfo->buffInfo.auxBuffStart;
      else
        top = h3Mem;
      
      bInfo->buffInfo.colBuffStart[i] = 
        bInfo->buffInfo.colBuffEnd[i] = top;

    } else {
      bInfo->buffInfo.colBuffStart[i] = 
        bInfo->buffInfo.colBuffEnd[i] = 
        bInfo->buffInfo.colBuffStart[i + 1];
    }
    
    bInfo->buffInfo.colBuffStart[i] -= bufSize;

    /* As a memory access optmization colorBuffers start on even
     * pages, while aux buffers start on odd pages.  Thus we must
     * check to see if we're startding on an odd page here and, if so,
     * add a page to the start.  
     */
    bufferAlignP = ((bInfo->buffInfo.colBuffStart[i] & 0x1000UL) != 0);
    if (bufferAlignP) bInfo->buffInfo.colBuffStart[i] -= 0x1000;
  }

  /* Now we can calculate some other stuff... */
  bInfo->fbOffset =  bInfo->buffInfo.colBuffStart[0];
#if 0
  bInfo->tramOffset =  
  fifoSize = MAXFIFOSIZE;
  tramSize =  bInfo->fbOffset - bInfo->tramOffset;
  if (tramSize < bInfo->min_tramSize) {
    /* Now we have to shrink the FIFO */
    tramSize = bInfo->min_tramSize;
    fifoSize = bInfo->fbOffset - bInfo->min_tramSize;
    if (fifoSize < 0) {
      GDBG_INFO(80, "%s:  Not enough memory for resolution + min texture\n",
        FN_NAME);
      sprintf(errorString, 
        "%s:  Not enough memory for resolution + min texture\n",
        FN_NAME);
      return FXFALSE;
    } 
  }


  bInfo->fifoInfo.fifoLength = (FxU32) fifoSize;
  bInfo->fifoInfo.fifoStart = 0;
  bInfo->tramOffset = (FxU32) fifoSize;
  bInfo->tramSize = (FxU32) tramSize;

#else
  bInfo->tramOffset = 0;
  fifoSize = MAXFIFOSIZE;
  tramSize =  bInfo->fbOffset - bInfo->tramOffset - fifoSize;
  if (tramSize < (FxI32)bInfo->min_tramSize) {
    /* Now we have to shrink the FIFO */
    tramSize = bInfo->min_tramSize;
    fifoSize = bInfo->fbOffset - bInfo->min_tramSize;
    if (fifoSize < 0) {
      GDBG_INFO(80, "%s:  Not enough memory for resolution + min texture\n",
        FN_NAME);
      sprintf(errorString, 
        "%s:  Not enough memory for resolution + min texture\n",
        FN_NAME);
      return FXFALSE;
    } 
  }

  bInfo->fifoInfo.fifoLength = (FxU32) fifoSize;
  bInfo->fifoInfo.fifoStart = bInfo->fbOffset - fifoSize;

  bInfo->fifoInfo.fifoLength -= 0x2000;

  bInfo->tramOffset = 0;
  bInfo->tramSize = (FxU32) tramSize;
  
#endif

#if 1
#define LFBSTRIDE 0x1000
  {
    FxU32 colBuffOffset = 0;
    FxU32 colBufAlignP = 0;

    for (bNum = 0; bNum < nColBuffers; bNum++) {
      if ((colBuffOffset & 0x1000UL) != 0) {
        colBuffOffset += 0x1000UL;
        colBufAlignP++;
      }
      bInfo->buffInfo.lfbBuffAddr[bNum] = 
        hwcBufferLfbAddr(bNum, bInfo, colBufAlignP);
      colBuffOffset += bufSize;
    }

    if (nAuxBuffers > 0) {
      if ((colBuffOffset & 0x1000UL) == 0) {
        colBuffOffset += 0x1000UL;
        colBufAlignP++;
      }
      bInfo->buffInfo.lfbBuffAddr[nColBuffers] =
        hwcBufferLfbAddr(nColBuffers, bInfo, colBufAlignP);
    }
  }
#endif

  bInfo->buffInfo.initialized = FXTRUE;

  GDBG_INFO(80, "%s:  Board Info:\n", FN_NAME);
  GDBG_INFO(80, "\thdc:             0x%x\n", bInfo->hdc);
  GDBG_INFO(80, "\textContextID:    0x%x\n", bInfo->extContextID);
  GDBG_INFO(80, "\tdevRev:          0x%x\n", bInfo->devRev);
  GDBG_INFO(80, "\tfbOffset:        0x%x\n", bInfo->fbOffset);
  GDBG_INFO(80, "\th3Rev:           0x%x\n", bInfo->h3Rev);
  GDBG_INFO(80, "\th3Mem:           0x%x\n", bInfo->h3Mem);
  GDBG_INFO(80, "\tboardNum:        0x%x\n", bInfo->boardNum);
  GDBG_INFO(80, "\tdeviceNum:       0x%x\n", bInfo->deviceNum);

  GDBG_INFO(80, "%s:  Buffer Info:\n", FN_NAME);
  GDBG_INFO(80, "\tbufSize:         0x%x\n", bInfo->buffInfo.bufSize);
  GDBG_INFO(80, "\tbufSizeInTiles:  0x%x\n", bInfo->buffInfo.bufSizeInTiles);
  GDBG_INFO(80, "\tbufStride:       0x%x\n", bInfo->buffInfo.bufStride);
  GDBG_INFO(80, "\tbufStrideInTiles:0x%x\n", bInfo->buffInfo.bufStrideInTiles);
  GDBG_INFO(80, "\tbufHeightInTiles:0x%x\n", bInfo->buffInfo.bufHeightInTiles);
  GDBG_INFO(80, "\tnColBuffers:     0x%x\n", bInfo->buffInfo.nColBuffers);
  for (i = 0; i < (FxI32) nColBuffers; i++) {
    GDBG_INFO(80, "\tcolBuff %d Start: 0x%x\n", i, bInfo->buffInfo.colBuffStart[i]);
    GDBG_INFO(80, "\tcolBuff %d End:   0x%x\n", i, bInfo->buffInfo.colBuffEnd[i]);
  }
  GDBG_INFO(80, "\tnAuxBuffers:     0x%x\n", bInfo->buffInfo.nAuxBuffers);
  GDBG_INFO(80, "\tauxBuffStart:    0x%x\n", bInfo->buffInfo.auxBuffStart);
  GDBG_INFO(80, "\tauxBuffEnd:      0x%x\n", bInfo->buffInfo.auxBuffEnd);

  GDBG_INFO(80, "%s:  FIFO Info:\n", FN_NAME);
  GDBG_INFO(80, "\tfifoStart:       0x%x\n", bInfo->fifoInfo.fifoStart);
  GDBG_INFO(80, "\tfifoLength:      0x%x\n", bInfo->fifoInfo.fifoLength);

  return FXTRUE;
#undef FN_NAME
} /* hwcAllocBuffers */

FxBool
hwcInitFifo(hwcBoardInfo *bInfo, FxBool enableHoleCounting)
{
#define FN_NAME "hwcInitFifo"
  FxBool
    agpEnable = FXFALSE;  
  FxU32
    cagpRegs;                   /* pointer to Cmd/AGP regs */

  if (bInfo->regInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcMapBoard\n", FN_NAME);
    return FXFALSE;
  }
  cagpRegs = bInfo->regInfo.cmdAGPBase;

  if (bInfo->buffInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcInitBuffers\n", FN_NAME);
    return FXFALSE;
  }

  /* disable the CMD fifo */
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize, 0);
  
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseAddrL,
    bInfo->fifoInfo.fifoStart>>12);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.readPtrL, bInfo->fifoInfo.fifoStart);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.readPtrH, 0);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.aMin, bInfo->fifoInfo.fifoStart-4);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.aMax, bInfo->fifoInfo.fifoStart-4);
  
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.depth, 0);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.holeCount, 0);
  /* Fifo LWM /HWM/ THRESHOLD */
  if (bInfo->pciInfo.deviceID == 0x3) { /* banshee */
    HWC_CAGP_STORE(bInfo->regInfo, cmdFifoThresh,
                   (0x09 << SST_HIGHWATER_SHIFT) | 0x2);
  } else {
    HWC_CAGP_STORE(bInfo->regInfo, cmdFifoThresh,
                   (0xf << SST_HIGHWATER_SHIFT) | 0x8);
  }

  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize, (((bInfo->fifoInfo.fifoLength >> 12) - 1) | 
                                                     SST_EN_CMDFIFO |
                                                     (enableHoleCounting ? 0 : SST_CMDFIFO_DISABLE_HOLES) |
                                                     (agpEnable ?  SST_CMDFIFO_AGP : 0)));
  
  GDBG_INFO(80, "%s:  CMD FIFO placed at physical addr 0x%x\n", 
            FN_NAME,
            bInfo->fifoInfo.fifoStart);

  return FXTRUE;

#undef FN_NAME
} /* hwcInitFifo */

#if 0
/* Currently unused. */
static SstIORegs savedIORegs;
#endif

void
hwcInitVideoOverlaySurface(
  hwcRegInfo *rInfo,
    FxU32 enable,               /* 1=enable Overlay surface (OS), 1=disable */
    FxU32 stereo,               /* 1=enable OS stereo, 0=disable */
    FxU32 horizScaling,         /* 1=enable horizontal scaling, 0=disable */
    FxU32 dudx,                 /* horizontal scale factor (ignored if not */
      /* scaling) */
    FxU32 verticalScaling,      /* 1=enable vertical scaling, 0=disable */
    FxU32 dvdy,                 /* vertical scale factor (ignored if not */
      /* scaling) */
    FxU32 filterMode,           /* duh */
    FxU32 tiled,                /* 0=OS linear, 1=tiled */
    FxU32 pixFmt,               /* pixel format of OS */
    FxU32 clutBypass,           /* bypass clut for OS? */
    FxU32 clutSelect,           /* 0=lower 256 CLUT entries, 1=upper 256 */
    FxU32 startAddress,         /* board address of beginning of OS */
    FxU32 stride)               /* distance between scanlines of the OS, in */
  /* units of bytes for linear OS's and tiles for */
  /* tiled OS's */
{
  FxU32 doStride;
  FxU32 vidProcCfg;

  HWC_IO_LOAD((*rInfo), vidProcCfg, vidProcCfg);

  vidProcCfg &= ~(SST_OVERLAY_TILED_EN |
                  SST_OVERLAY_STEREO_EN |  
                  SST_OVERLAY_HORIZ_SCALE_EN |
                  SST_OVERLAY_VERT_SCALE_EN |
                  SST_OVERLAY_TILED_EN |
                  SST_OVERLAY_PIXEL_FORMAT |
                  SST_OVERLAY_CLUT_BYPASS |
                  SST_OVERLAY_CLUT_SELECT);

  if (enable)
    vidProcCfg |= SST_OVERLAY_EN;

  if (stereo)
    vidProcCfg |= SST_OVERLAY_STEREO_EN;

  if (horizScaling)
    vidProcCfg |= SST_OVERLAY_HORIZ_SCALE_EN;

  if (verticalScaling)
    vidProcCfg |= SST_OVERLAY_VERT_SCALE_EN;

  if (tiled) {
    vidProcCfg |= SST_OVERLAY_TILED_EN;
  }

  vidProcCfg |= SST_OVERLAY_PIXEL_RGB565D; /* Turn on RGB565Dithered */
  vidProcCfg &= ~SST_CURSOR_EN; /* Turn off HW Cursor */
  vidProcCfg |= SST_OVERLAY_TILED_EN; /* Overlay tile space enable */    


  if (clutBypass)
    vidProcCfg |= SST_OVERLAY_CLUT_BYPASS;

  if (clutSelect)
    vidProcCfg |= SST_OVERLAY_CLUT_SELECT;

  HWC_IO_STORE((*rInfo), vidProcCfg, vidProcCfg);

  /* */
  HWC_IO_LOAD((*rInfo), vidDesktopOverlayStride, doStride);
  doStride &= ~(SST_OVERLAY_LINEAR_STRIDE | SST_OVERLAY_TILE_STRIDE);

  stride <<= SST_OVERLAY_STRIDE_SHIFT;
  if (tiled)
    stride &= SST_OVERLAY_TILE_STRIDE;
  else
    stride &= SST_OVERLAY_LINEAR_STRIDE;
  doStride |= stride;

  HWC_IO_STORE((*rInfo), vidDesktopOverlayStride, doStride);

} /* hwcInitVideoOverlaySurface */

FxU32 
hwcInitLookupRefresh(FxU32 ord_refresh) 
{
#define FN_NAME "hwcInitLookupRefresh"
  FxU32 refresh_hz;

  switch(ord_refresh) {
  case(GR_REFRESH_60Hz):
    refresh_hz = 60;
    break;
  case(GR_REFRESH_70Hz):
    refresh_hz = 70;
    break;
  case(GR_REFRESH_72Hz):
    refresh_hz = 72;
    break;
  case(GR_REFRESH_75Hz):
    refresh_hz = 75;
    break;
  case(GR_REFRESH_80Hz):
    refresh_hz = 80;
    break;
  case(GR_REFRESH_85Hz):
    refresh_hz = 85;
    break;
  case(GR_REFRESH_90Hz):
    refresh_hz = 90;
    break;
  case(GR_REFRESH_100Hz):
    refresh_hz = 100;
    break;
  case(GR_REFRESH_120Hz):
    refresh_hz = 120;
    break;
  default:
    GDBG_ERROR(FN_NAME, "Unsupported Refresh Rate -- defaulting to 60hz\n");
    refresh_hz = 60;
    break;
  }
  return (refresh_hz);
#undef FN_NAME
} /* hwcInitLookupRefresh */

#ifdef HWC_ACCESS_DDRAW

#define LINEAR_STRIDE_ALIGN 16UL

static FxBool
_hwcLinear2HWAddr(const FxU32 linearAddr, 
                  const FxU32 linearBaseAddr,
                  const FxU32 linearStride,
                  const hwcBoardInfo* bInfo,
                  hwcSurfaceInfo* ret)
{
#define FN_NAME "_hwcLinear2HWAddr"
  FxU32
    pciStride,                  /* page stride */
    hwStride,                   /* tile stride */
    tileMark,                   /* delineation between linear and tiled */
    lpTileBase;                 /* linear addr of tileMark */
  FxBool
    retVal = FXFALSE,
    isTiled;                    /* is the surface tiled? */

  /* determine lfb baseAddress and hw offset to buffer */
  {
    hwcExtRequest_t
      req;                        /* Request to HWC_EXT */
    hwcExtResult_t
      res;                        /* Result from HWC_EXT */
    
    /* query for tile watermark & compute tile characteristics */
    req.which = HWCEXT_GETDEVICECONFIG;
    GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_GETDEVICECONFIG\n");
    retVal = (ExtEscape(bInfo->hdc, EXT_HWC, 
                        sizeof(req), (LPSTR)&req, 
                        sizeof(res), (LPSTR)&res) > 0);
    if (!retVal) {
      strcpy(errorString, "HWCEXT_GETDEVICECONFIG failed");
      GDBG_INFO(80, "%s: %s.\n", FN_NAME, errorString);
      goto __errExit;
    }
    pciStride = res.optData.deviceConfigRes.pciStride;
    hwStride = res.optData.deviceConfigRes.hwStride;
    tileMark = res.optData.deviceConfigRes.tileMark;

    lpTileBase = linearBaseAddr + tileMark;
    isTiled = (linearAddr >= lpTileBase);
  }

  ret->pciStride = pciStride;
  ret->tileBase = tileMark;
  ret->lpSurface = linearAddr;
  ret->lpLFB = linearBaseAddr;
  ret->hwStride = hwStride;

  /* Compute things that depend on the offset wrt the tile mark */
  ret->isTiled = isTiled;
  if (ret->isTiled) {
    const FxU32
      tileStride = hwStride,
      tilePitch = pciStride,
      tileOffset = linearAddr - lpTileBase,
      y = tileOffset / tilePitch, 
      x = tileOffset - (y * tilePitch),
      tile = ((y / HWC_TILE_HEIGHT) * tileStride) + (x / HWC_TILE_WIDTH);

    ret->fbOffset = (tile << 12UL) + tileMark;
    ret->fbStride = ret->hwStride | SST_BUFFER_MEMORY_TILED;
  } else {
    /* NB: Banshee (and derivatives) have a 16-byte alignment
     * restriction on the stride and offset for color/aux buffers, but
     * it is left to the client to correctly adjust for this when
     * setting the values.  
     */
    ret->fbOffset = linearAddr - linearBaseAddr;
    ret->fbStride = linearStride;
  }

 __errExit:
  return retVal;
#undef FN_NAME
} /* _hwcLinear2HWAddr */

static FxU32
_hwcPixelFormat2Bytes(const DDPIXELFORMAT* pixelFormat)
{
#define FN_NAME "_hwcPixelFormat2Bytes"
  const DWORD
    formatFlags = pixelFormat->dwFlags;
  FxU32 
    retVal = 0x00UL;

  if ((formatFlags & DDPF_PALETTEINDEXED8) == DDPF_PALETTEINDEXED8) {
    retVal = 1;
  } else if ((formatFlags & DDPF_RGB) == DDPF_RGB) {
    retVal = (pixelFormat->dwRGBBitCount >> 0x03UL);
    if ((retVal == 0) || (retVal > 2)) {
      retVal = 0;
      GDBG_INFO(80, FN_NAME": Invalid surface rgb bit count(0x%X)\n",
                pixelFormat->dwRGBBitCount);
    }
  } else {
    GDBG_INFO(80, FN_NAME": Invalid surface pixel format flags(0x%X)\n",
              formatFlags);
  }

  return retVal;
#undef FN_NAME
} /* _hwcPixelFormat2Bytes */

/*-------------------------------------------------------------------
  Function: hwcGetSurfaceInfo
  Date: 14-Jul-98
  Implementor(s): dow
  Description:
  Returns information about a Glide (or DDraw) surface

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
FxBool
hwcGetSurfaceInfo(const hwcBoardInfo* bInfo,
                  FxU32 *sfc, 
                  hwcSurfaceInfo *ret)
{
#define FN_NAME "hwcGetSurfaceInfo"
  /* AssUMe it's a DDraw surface for now */
  LPDIRECTDRAWSURFACE2
    surf = (LPDIRECTDRAWSURFACE2) sfc;
  DDSURFACEDESC
    desc;
  DWORD
    ddErr;
  FxBool 
    retVal;
  FxU32
    lfbBase,                    /* linear addr for base of LFB */
    lpSurface;                  /* Linear addr for the surface */

  /* Lock the surface and get some info */
  desc.dwSize = sizeof(desc);
  ddErr = IDirectDrawSurface2_Lock(surf, 0, &desc, 
                                   DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, 0);
  retVal = (ddErr == DD_OK);
  if (!retVal) {
    sprintf(errorString, "%s: IDirectDrawSurface2_Lock (0x%X)\n", 
            FN_NAME,(unsigned)ddErr);
    GDBG_INFO(80, "%s", errorString);
    goto __errExit;
  }

  /* Grab the memory pointer */
  lpSurface = (DWORD) desc.lpSurface;

  /* Unlock the surface */
  desc.dwSize = sizeof(desc);
  IDirectDrawSurface2_Unlock(surf, desc.lpSurface);

  /* Get surface dimensions here rather than re-getting the surface
   * description later since these should all be valid here.  
   */
  ret->height = desc.dwHeight;
  ret->width  = desc.dwWidth;
  ret->depth  = _hwcPixelFormat2Bytes(&desc.ddpfPixelFormat);
  retVal = (ret->depth != 0x00UL);
  if (!retVal) {
    sprintf(errorString, "%s: Invalid surface pixel format (0x%X)\n", 
            FN_NAME, (unsigned)desc.ddpfPixelFormat.dwFlags);
    GDBG_INFO(80, "%s", errorString);
    goto __errExit;    
  }

  /* Get base addresses of the board */
  {
    hwcExtRequest_t
      req;                        /* Request to HWC_EXT */
    hwcExtResult_t
      res;                        /* Result from HWC_EXT */

    /* This address was set when the board was mapped in hwcMapBoard */
    lfbBase = bInfo->linearInfo.linearAddress[1];
    
    /* The DirectDraw driver may have re-mapped memory different than
     * the 2d driver's hw mapping. We have to figure out the correct
     * hw address for packet offsets and color buffer stuff.  
     */ 
    if (retVal) {
      req.which = HWCEXT_LINEAR_MAP_OFFSET;
      req.optData.mapInfoReq.mapAddr = lfbBase;
      req.optData.mapInfoReq.remapAddr = lpSurface;
      
      GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_LINEAR_MAP_OFFSET\n");
      retVal = (ExtEscape(bInfo->hdc, EXT_HWC,
                          sizeof(req), (LPCSTR)&req,
                          sizeof(res), (LPSTR)&res) > 0);
      if (!retVal) {
        strcpy(errorString, "HWCEXT_LINEAR_MAP_OFFSET failed");
        GDBG_INFO(80, "%s: %s.\n", FN_NAME, errorString);
      }
      
      /* lpSurface is now an address relative to the 2d driver's base
       * address as returned by the HWCEXT_GETLINEARADDR callback.  
       */
      lpSurface = (lfbBase + res.optData.mapInfoRes.linAddrOffset);
    }
  }
  if (!retVal) goto __errExit;

  /* Get the rest of the information about the hw relative address
   * from the linear address's location in memory.
   */
  retVal = _hwcLinear2HWAddr(lpSurface, 
                             lfbBase,
                             desc.lPitch,
                             bInfo,
                             ret);

 __errExit:  
  return retVal;
#undef FN_NAME
} /* hwcGetSurfaceInfo */

FxU32
hwcAllocWinContext(hwcBoardInfo* bInfo)
{
#define FN_NAME "hwcAllocWinContext"
  FxU32
    retVal = 0x00UL;
  hwcExtRequest_t
    ctxReq;
  hwcExtResult_t
    ctxRes;
  
  /* Allocate a context with the Driver */
  ctxReq.which = HWCEXT_ALLOCCONTEXT;
  ctxReq.optData.allocContextReq.protocolRev = HWCEXT_PROTOCOLREV;
  ctxReq.optData.allocContextReq.appType = HWCEXT_ABAPPTYPE_WIND;
  
  GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_ALLOCCONTEXT\n");
  if (ExtEscape(bInfo->hdc, EXT_HWC, 
                sizeof(ctxReq), (LPSTR)&ctxReq,
                sizeof(ctxRes), (LPSTR)&ctxRes) < 1) {
    strcpy(errorString, FN_NAME": HWCEXT_ALLOCCONTEXT failed");
  } else {
    retVal = ctxRes.optData.allocContextRes.contextID;
  }    

  return retVal;
#undef FN_NAME
} /* hwcAllocWinContext */

FxBool
hwcFreeWinContext(hwcBoardInfo* bInfo,
                  FxU32         winContextId)
{
#define FN_NAME "hwcFreeWinContext"
  hwcExtRequest_t 
    req;
  hwcExtResult_t 
    res;

  req.which = HWCEXT_RELEASECONTEXT;
  req.optData.releaseContextReq.contextID = winContextId;

  GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_RELEASECONTEXT\n");
  return (ExtEscape(bInfo->hdc, EXT_HWC, 
                    sizeof(req), (void*)&req,
                    sizeof(res), (void*)&res) > 0);
#undef FN_NAME
} /* hwcFreeWinContext */


FxBool 
hwcAllocWinFifo(hwcBoardInfo* bInfo, 
                HwcWinFifo*   fifo,
                FxU32*        surface)
{
#define FN_NAME "hwcAllocWinFifo"
  FxBool 
    retVal = FXFALSE;
  hwcExtRequest_t      
    req;
  hwcExtResult_t       
    res;

  /* Set undefined fifo type to start, and always clear the serial
   * number for this context.
   */
  fifo->fifoType = 0xFFFFFFFFUL;

  /* Check to see that we have valid surface owner to hang
   * the command fifo surface off of.
   */
  retVal = (surface != NULL);
  if (retVal) {
    LPDIRECTDRAW
      objDD = NULL;
    LPDIRECTDRAWSURFACE
      objSentinal = NULL,
      objFifo = NULL;
    DDSURFACEDESC
      fifoDesc,
      sentinalDesc;

    /* Get the surface owner so that we can try to allocate a new
     * surface for the fifo. On windows this will be a direct draw
     * object, and we need a dd4 object for agp surfaces.
     */
    {
      LPDIRECTDRAWSURFACE2
        objSurface = NULL;
      
      /* Make sure that this is a Surface2 object so that we can get
       * the DirectDraw object that owns this surface and piggy back
       * our objects ontop of it.
       */
      retVal = (IDirectDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE)surface, 
                                                  &IID_IDirectDrawSurface2,
                                                  &objSurface) == DD_OK);
      if (!retVal) {
        GDBG_INFO(80, "%s: Require atleast IDirectDrawSurface2.\n", FN_NAME);
        goto __errSurfaceFifo;
      }
      
      /* Get the dd object for the surface creation */
      retVal = (IDirectDrawSurface2_GetDDInterface(objSurface, 
                                                   &objDD) == DD_OK);
      if (!retVal) {
        GDBG_INFO(80, "%s: Could not acquire DirectDraw object.\n", FN_NAME);
        goto __errSurfaceDDObj;
      }

  __errSurfaceDDObj:
      if (objSurface != NULL) IDirectDrawSurface_Release(objSurface);
      if (!retVal) goto __errSurfaceFifo;
    }

    /* Is the 2d command fifo in agp or local frame buffer space? */
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    
    req.which = HWCEXT_FIFOINFO;
    GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_FIFOINFO\n");
    retVal = (ExtEscape((HDC)bInfo->hdc, EXT_HWC,
                        sizeof(req), (void*)&req,
                        sizeof(res), (void*)&res) > 0);
    if (!retVal) {
      GDBG_INFO(80, "%s: HWCEXT_FIFOINFO failed to determine current 2d fifo type.\n",
                FN_NAME);
      goto __errSurfaceFifo;
    }

    /* Basic surface type for the fifo */
    memset(&fifoDesc, 0, sizeof(fifoDesc));
    fifoDesc.dwSize  = sizeof(fifoDesc);
    fifoDesc.dwFlags = (DDSD_CAPS | 
                        DDSD_WIDTH | DDSD_HEIGHT |
                        DDSD_PITCH | 
                        DDSD_PIXELFORMAT);

    /* Set pixel format to make the allocation be 32-bit words */
    fifoDesc.ddpfPixelFormat.dwSize = sizeof(fifoDesc.ddpfPixelFormat);
    fifoDesc.ddpfPixelFormat.dwFlags = (DDPF_ALPHAPIXELS |
                                        DDPF_RGB);
    fifoDesc.ddpfPixelFormat.dwRGBBitCount = 32UL;
    fifoDesc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000UL;
    fifoDesc.ddpfPixelFormat.dwRBitMask = 0x00FF0000UL;
    fifoDesc.ddpfPixelFormat.dwGBitMask = 0x0000FF00UL;
    fifoDesc.ddpfPixelFormat.dwBBitMask = 0x000000FFUL;

    /* We want the command fifo someplace that the hw can get to
     * directly. Whether this is is local to the board or agp is
     * controlled by where the 2d driver has its fifo.  
     */
    fifoDesc.ddsCaps.dwCaps = (DDSCAPS_OFFSCREENPLAIN |
                               DDSCAPS_VIDEOMEMORY);

    {
      DDCAPS
        ddCaps;

      /* Make sure that the DirectDraw driver knows about different
       * video memory types before setting the bits to differentiate
       * local from agp memory.  
       */
      memset(&ddCaps, 0, sizeof(ddCaps));
      ddCaps.dwSize = sizeof(ddCaps);
      
      retVal = (IDirectDraw_GetCaps(objDD,
                                    &ddCaps, 
                                    NULL) == DD_OK);
      if (!retVal) {
        GDBG_INFO(80, "%s: Could not determine if DirectDraw handles separate memory types.\n",
                  FN_NAME);
        goto __errSurfaceFifo;
      }
      
      /* Make the fifo type match the 2d fifo type if the driver can
       * handle different memory types.  
       */
      switch (res.optData.fifoInfoRes.fifoType) {        
      case HWCEXT_FIFO_AGP:
        if ((ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM) != 0x00UL) {
          fifoDesc.ddsCaps.dwCaps |= DDSCAPS_NONLOCALVIDMEM;
        } else {
          retVal = FXFALSE;
        }
        break;

      case HWCEXT_FIFO_FB:
        if ((ddCaps.ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM) != 0x00UL) 
          fifoDesc.ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM;
        break;
        
      case HWCEXT_FIFO_HOST:
      default:
        retVal = FXFALSE;
        break;
      }
      if (!retVal) goto __errSurfaceFifo;
    }

    /* Allocate a sentinal surface so that the client can 'know' when
     * a given fifo segment has actually been executed by reading a
     * client specified serial # out of the buffer. This only needs to
     * be big enough to hold the size of a serial #, but we need to
     * work around some sort of whacked agp bug when reading back. See
     * hwcExecuteStatusWinFifo() for details.
     */
    sentinalDesc = fifoDesc;

    sentinalDesc.dwHeight = 4;
    sentinalDesc.dwWidth  = 4;
    
    retVal = (IDirectDraw_CreateSurface(objDD,
                                        &sentinalDesc,
                                        &objSentinal,
                                        0) == DD_OK);
    if (!retVal) {
      GDBG_INFO(80, "%s: Could not allocate surface for serial #'s.\n", FN_NAME);
      goto __errSurfaceFifo;
    }

    /* Make stride and width the same so that we get the entire
     * surface as teh command fifo for the hw to just jsr to.
     *
     * NB: Currently DirectDraw will not allow surfaces to have a
     * width greater than that of the primary surface. We will fail if
     * we don't get this surface w/o checking the primary surface's
     * width because if we don't have a contiguous block it is going
     * to be a pain to have the hw jsr to each of these 'lines'. It is
     * likely that DirectDraw is not going to sub-allocate out of this
     * slop like it could, but I would prefer not to take chances
     * until we have to.
     */
    fifoDesc.lPitch   = 0x1000UL;
    fifoDesc.dwWidth  = (fifoDesc.lPitch >> 2UL);

    /* Try to allocate a surface that contains some # of command
     * stream and persistant state buffers. This is
     * allocated in hw page units (4k) rather than the client
     * allocation units, but that should be transparent since
     * the client will just not use the extra slop.
     */
#define MAX(__a, __b) (((__a) > (__b)) ? (__a) : (__b))
    {
      const FxU32
        allocFifo  = MAX(fifo->cmdBuf.allocUnit, 0x1000UL),
        allocState = MAX(fifo->stateBuf.allocUnit, 0x1000UL),
        allocUnit  = pow2Round(allocFifo + allocState + 0xFFFUL, 0x1000UL);
      const char*
        numAllocStr = GETENV("FX_WINFIFO_INIT_ALLOC");
      FxU32
        numAlloc   = ((numAllocStr == NULL)
                      ? 0x8UL
                      : atoi(numAllocStr));
      DWORD 
        ddErr;

      while(numAlloc > 0) {
        fifoDesc.dwHeight = (allocUnit * numAlloc) / fifoDesc.lPitch;
        ddErr = IDirectDraw_CreateSurface(objDD,
                                          &fifoDesc,
                                          &objFifo,
                                          0);
        retVal = (ddErr == DD_OK);
        if (retVal) break;

        if ((ddErr != DDERR_OUTOFVIDEOMEMORY) ||
            (ddErr != DDERR_OUTOFMEMORY)) break;

        /* Try a smaller allocation */
        numAlloc--;
      }
      if (!retVal) {
        GDBG_INFO(80, "%s: Could not get cmdFifo DirectDraw surface. (0x%X)\n",
                  FN_NAME, ddErr);
        goto __errSurfaceFifo;
      }

      /* Its now safe to overwrite the client allocUnit's to the
       * rounded allocation units because we've finally done all of
       * the stuff that could fail.
       */
      fifo->cmdBuf.size = allocFifo * numAlloc;
      fifo->cmdBuf.allocUnit = allocFifo;

      fifo->stateBuf.size = allocState * numAlloc;
      fifo->stateBuf.allocUnit = allocState;
    }
#undef MAX

    /* Fill in the rest of the client's return buffer */
    fifo->fifoType = res.optData.fifoInfoRes.fifoType;

    fifo->surfaceFifo     = (FxU32*)objFifo;
    fifo->surfaceSentinal = (FxU32*)objSentinal;

    /* Do the lock so that we can write the sentinal, but the
     * client is responsible for locking before use.
     */
    if (hwcLockWinFifo(bInfo, fifo)) {
      /* Write bogus serial # */
      *(volatile FxU32*)fifo->sentinalBufferAddr = 0x0UL;
    
      hwcUnlockWinFifo(bInfo, fifo);
    }
    
 __errSurfaceFifo:
    /* Free our directdraw objects if something bad happened. */
    if (!retVal) {
      if (objSentinal != NULL) IDirectDrawSurface_Release(objSentinal);
      if (objFifo != NULL) IDirectDrawSurface_Release(objFifo);
    }
    if (objDD != NULL) IDirectDraw_Release(objDD);
  }

  /* Could not allocate directly accessible fifo so setup the host
   * memory backed fifo which will get dumped at execute time.  
   */
  if (!retVal && (bInfo->hdc != 0x00UL)) {
    fifo->fifoType          = HWCEXT_FIFO_HOST;

    fifo->cmdBuf.hwOffset    = 0x00UL;
    fifo->cmdBuf.size   = fifo->cmdBuf.allocUnit;
    fifo->stateBuf.hwOffset  = 0x00UL;
    fifo->stateBuf.size = fifo->stateBuf.allocUnit;

    /* This can never fail since the client is responsible for
     * allocating this memory.  
     */
    retVal = FXTRUE;
  }

  return retVal;
#undef FN_NAME
} /* hwcAllocWinFifo */

FxBool
hwcLockWinFifo(hwcBoardInfo* bInfo,
               HwcWinFifo* fifo)
{
#define FN_NAME "hwcLockWinFifo"
  FxBool 
    retVal = FXFALSE;

  if (fifo->fifoType == HWCEXT_FIFO_HOST) {
    retVal = FXTRUE;
  } else if ((fifo->fifoType == HWCEXT_FIFO_FB) ||
             (fifo->fifoType == HWCEXT_FIFO_AGP)) {
    DDSURFACEDESC
      fifoDesc,
      sentinalDesc;

    if (fifo->lockCount > 0) goto __alreadyLocked;
    if ((fifo->surfaceFifo == NULL) ||
        (fifo->surfaceSentinal == NULL)) goto __errFifoLock;

    /* Get base pointer to the fifo surfaces.
     *
     * NB: In theory we should leave these locked for the duration of
     * the of the fifo lock and we would need to set the
     * DDLOCK_NOSYSLOCK flag when doing the lock. However, when we do
     * this type of lock the directDraw driver re-maps the physical
     * address to a new set of linear address pages. So we take advantage
     * of the fact taht the current implementation will not move memory
     * behind our backs outside of a surface lock.
     */
#define HWC_DIRECTDRAW_DRIVER_NOSYSLOCK_FLAGS 0 
    fifoDesc.dwSize = sizeof(fifoDesc);
    retVal = (IDirectDrawSurface_Lock((LPDIRECTDRAWSURFACE)fifo->surfaceFifo,
                                      NULL, 
                                      &fifoDesc, 
                                      HWC_DIRECTDRAW_DRIVER_NOSYSLOCK_FLAGS |
                                      DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,
                                      NULL) == DD_OK);

    /* NB: Since there's no working DDLOCK_NOSYSLOCK we unlock the surface
     * here, and rely on the current implementation to leave the linear 
     * mapping to the hw intact after the unlocking.
     */
    IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)fifo->surfaceFifo, NULL);
    if (!retVal) {
      strcpy(errorString, "Could not lock cmdFifo surface");
      GDBG_INFO(80, "%s: %s.\n", FN_NAME, errorString);
      goto __errFifoLock;
    }

    sentinalDesc.dwSize  = sizeof(sentinalDesc);
    retVal = (IDirectDrawSurface_Lock((LPDIRECTDRAWSURFACE)fifo->surfaceSentinal, 
                                      NULL, 
                                      &sentinalDesc, 
                                      HWC_DIRECTDRAW_DRIVER_NOSYSLOCK_FLAGS |
                                      DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, 
                                      NULL) == DD_OK);

    /* NB: Since there's no working DDLOCK_NOSYSLOCK we unlock the surface
     * here, and rely on the current implementation to leave the linear 
     * mapping to the hw intact after the unlocking.
     */
    IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)fifo->surfaceSentinal, NULL);
    if (!retVal) {
      strcpy(errorString, "Could not lock serial # surface");
      GDBG_INFO(80, "%s: %s.\n", FN_NAME, errorString);
      goto __errFifoLock;
    }

    fifo->cmdBuf.baseAddr   = (FxU32)fifoDesc.lpSurface;
    fifo->stateBuf.baseAddr = (fifo->cmdBuf.baseAddr + 
                                      fifo->cmdBuf.size);
    fifo->sentinalBufferAddr       = (FxU32)sentinalDesc.lpSurface;

    /* The DirectDraw driver may have re-mapped memory different than
     * the 2d driver's hw mapping. We have to figure out the correct
     * hw address for packet offsets and color buffer stuff.  
     */
    {
      hwcExtRequest_t      
        req;
      hwcExtResult_t       
        res;
      const FxU32
        lfbBase = bInfo->linearInfo.linearAddress[1]; /* lfb space */
      FxU32
        tileMark;      
      
      /* query for tile watermark & compute tile characteristics */
      req.which = HWCEXT_GETDEVICECONFIG;
      GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_GETDEVICECONFIG\n");
      retVal = (ExtEscape((HDC)bInfo->hdc, EXT_HWC, 
                          sizeof(req), (LPSTR)&req, 
                          sizeof(res), (LPSTR)&res) > 0);
      if (!retVal) {
        strcpy(errorString, "HWCEXT_GETDEVICECONFIG failed");
        GDBG_INFO(80, "%s: %s.\n", FN_NAME, errorString);
        goto __errFifoLock;
      }
      tileMark = res.optData.deviceConfigRes.tileMark;

      {
        struct remapRec {
          FxU32  remapAddr;
          FxU32* remapOffset;
          FxU32  surfaceType;
        } remapAddrList[] = {
          {    fifo->cmdBuf.baseAddr,      &fifo->cmdBuf.hwOffset, fifo->fifoType },
          {  fifo->stateBuf.baseAddr,    &fifo->stateBuf.hwOffset, fifo->fifoType },
          { fifo->sentinalBufferAddr, &fifo->sentinalBufferOffset, HWCEXT_FIFO_FB }
        };
        FxU32 i;

        for(i = 0; i < sizeof(remapAddrList) / sizeof(remapAddrList[0]); i++) {
          FxU32 hwOffset;

          req.which = HWCEXT_LINEAR_MAP_OFFSET;
          req.optData.mapInfoReq.mapAddr = lfbBase;
          req.optData.mapInfoReq.remapAddr = remapAddrList[i].remapAddr;
          
          GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_LINEAR_MAP_OFFSET\n");
          retVal = (ExtEscape((HDC)bInfo->hdc, EXT_HWC,
                              sizeof(req), (LPCSTR)&req,
                              sizeof(res), (LPSTR)&res) > 0);
          if (!retVal) {
            strcpy(errorString, "HWCEXT_LINEAR_MAP_OFFSET failed");
            GDBG_INFO(80, "%s: %s.\n", FN_NAME, errorString);
            goto __errFifoLock;
          }

          /* Make sure we have the right offset for tiled/linear addresses */
          hwOffset = res.optData.mapInfoRes.linAddrOffset;
          if ((remapAddrList[i].surfaceType == HWCEXT_FIFO_FB) && (hwOffset >= tileMark)) {
            hwcSurfaceInfo surfaceInfo;

            retVal = _hwcLinear2HWAddr(remapAddrList[i].remapAddr, 
                                       remapAddrList[i].remapAddr - hwOffset,
                                       0x00UL,
                                       bInfo,
                                       &surfaceInfo);
            if (!retVal) goto __errFifoLock;
            hwOffset = surfaceInfo.fbOffset;
          }
          *remapAddrList[i].remapOffset = hwOffset;
        }
      }
    }

   __alreadyLocked:
    /* Finally, declare success */
    retVal = FXTRUE;
    
   __errFifoLock:
    ;
  }

  if (retVal) fifo->lockCount++;

  return retVal;
#undef FN_NAME
} /* hwcLockWinFifo */

FxBool
hwcUnlockWinFifo(hwcBoardInfo* bInfo,
                 HwcWinFifo* fifo)
{
#define FN_NAME "hwcUnlockWinFifo"
  FxBool
    retVal = ((fifo->lockCount > 0) &&
              ((fifo->fifoType == HWCEXT_FIFO_HOST) ||
               (fifo->fifoType == HWCEXT_FIFO_FB) ||
               (fifo->fifoType == HWCEXT_FIFO_AGP)));

  if (retVal) {
    fifo->lockCount--;

    if (fifo->lockCount == 0) {
      if ((fifo->fifoType == HWCEXT_FIFO_FB) ||
          (fifo->fifoType == HWCEXT_FIFO_AGP)) {
        /* Do nothing for now since the surfaces were unlocked in
         * hwcLockWinFifo since we're not using the NOSYSLOCK flags
         * when doing the DirectDraw surface lock.  
         */
      }
    }
  }

  return retVal;
#undef FN_NAME
} /* hwcUnlockWinFifo */

FxBool
hwcFreeWinFifo( hwcBoardInfo* bInfo,
                HwcWinFifo* fifo )
{
#define FN_NAME "hwcFreeWinFifo"

  /* What type of fifo do we have? */
  if ((fifo->fifoType == HWCEXT_FIFO_FB) ||
      (fifo->fifoType == HWCEXT_FIFO_AGP)) {
    if (fifo->surfaceFifo != NULL) {
      LPDIRECTDRAWSURFACE
        objFifo = (LPDIRECTDRAWSURFACE)fifo->surfaceFifo;

      IDirectDrawSurface_Release(objFifo);

      fifo->surfaceFifo = NULL;
      fifo->cmdBuf.baseAddr = 
      fifo->cmdBuf.hwOffset = 
      fifo->cmdBuf.size = 
      fifo->stateBuf.baseAddr = 
      fifo->stateBuf.hwOffset = 0x00UL;
    }
    
    if (fifo->surfaceSentinal != NULL) {
      LPDIRECTDRAWSURFACE
        objSentinal = (LPDIRECTDRAWSURFACE)fifo->surfaceSentinal;

      IDirectDrawSurface_Release(objSentinal);

      fifo->surfaceSentinal = NULL;
      fifo->sentinalBufferAddr =
      fifo->sentinalBufferOffset = 0x00UL;
    }
  }
  
  fifo->fifoType = HWCEXT_FIFO_INVALID;

  return FXTRUE;
#undef FN_NAME
} /* hwcFreeWinFifo */

FxBool
hwcExecuteWinFifo(hwcBoardInfo*     bInfo, 
                  const FxU32       winContextId,
                  const HwcWinFifo* fifo,
                  const FxU32       serialNumber)
{
#define FN_NAME "hwcExecuteWinFifo"
  hwcExtRequest_t      req;
  hwcExtResult_t       res;

  memset( &req, 0, sizeof( req ) );
  memset( &res, 0, sizeof( res ) );

  req.which = HWCEXT_EXECUTEFIFO;
  req.contextID = winContextId;

  req.optData.executeFifoReq.fifoType     = fifo->fifoType;

  req.optData.executeFifoReq.fifoPtr      = fifo->cmdBuf.baseAddr;
  req.optData.executeFifoReq.fifoSize     = fifo->cmdBuf.size >> 2UL;
  req.optData.executeFifoReq.statePtr     = fifo->stateBuf.baseAddr;
  req.optData.executeFifoReq.stateSize    = fifo->stateBuf.size >> 2UL;
  req.optData.executeFifoReq.serialNumber = serialNumber;

  if (fifo->fifoType != HWCEXT_FIFO_HOST) {
    req.optData.executeFifoReq.fifoOffset     = fifo->cmdBuf.hwOffset;
    req.optData.executeFifoReq.stateOffset    = fifo->stateBuf.hwOffset;
    req.optData.executeFifoReq.sentinalOffset = fifo->sentinalBufferOffset;
  }
  
  GDBG_INFO(80, FN_NAME": Id(0x%X) state(0x%X) cmd(0x%X)\n",
            winContextId, 
            req.optData.executeFifoReq.stateSize,
            req.optData.executeFifoReq.fifoSize);
  
  GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_EXECUTEFIFO\n");
  return ( ExtEscape( (HDC)bInfo->hdc, EXT_HWC, 
                      sizeof( req ), (void*)&req, 
                      sizeof( res ), (void*)&res )  > 0 );
#undef FN_NAME
} /* hwcExecuteWinFifo */

FxU32
hwcExecuteStatusWinFifo(hwcBoardInfo*     bInfo,
                        const HwcWinFifo* fifo,
                        const FxU32       serialNumber)
{
#define FN_NAME "hwcExecuteStatusWinFifo"
  FxU32
    retVal = ~0x00UL;

  switch(fifo->fifoType) {
  /* Host fifo always executes synchronously */
  case HWCEXT_FIFO_HOST:
    retVal = serialNumber;
    break;

  case HWCEXT_FIFO_FB:
  case HWCEXT_FIFO_AGP:
    {
      volatile FxU32*
        bufAddr = (volatile FxU32*)fifo->sentinalBufferAddr;

      /* We need to do some extra reads here so that we make sure taht
       * the data we're returning is coherent w/ the actual frame
       * buffer due to the intervening pixel cache. The cache is,
       * currently, 4 dwords, and appears to be aligned on this
       * boundary as well so reading 4 dwords away should be flushing.
       *
       * NB: The surface for the serial # was allocated big enough to
       * handle this so we would not have to worry about it here.  
       */
      retVal = bufAddr[0];
      retVal = bufAddr[0 + 4];
      retVal = bufAddr[0];
    }
    break;

  case HWCEXT_FIFO_INVALID:
  default:
    GDBG_ERROR(FN_NAME, "Invalid fifoType");
    break;
  }

  return retVal;
#undef FN_NAME
} /* hwcExecuteStatusWinFifo */

#endif /* HWC_ACCESS_DDRAW */

#ifdef HWC_ACCESS_GDX

FxBool
hwcGetSurfaceInfo(const hwcBoardInfo* bInfo,
                  FxU32 *sfc, 
                  hwcSurfaceInfo *ret)
{
        CntrlParam  paramBlock;
        OSErr       myErr;
        GDXSurfaceDesc_t desc;
        hwcControl_t control;

        memset( &control, 0, sizeof( control ) );
        
        /* HLock((Handle)bInfo->hMon); */
        paramBlock.ioCRefNum = (*((GDHandle)bInfo->hMon))->gdRefNum;
        /* HUnlock((Handle)bInfo->hMon); */

        paramBlock.csCode = k3DfxGetDeviceConfig; /* driver-specific status request */
        *(hwcControl_t **)(&paramBlock.csParam[0]) = &control;
                        
        myErr = PBControl((ParmBlkPtr)&paramBlock, false);      
        if(myErr != noErr)
                return FXFALSE;
                                        
        ret->tileBase = control.res.optData.deviceConfigRes.tileMark;
        ret->lpLFB = control.res.optData.deviceConfigRes.lfbBase;
        ret->pciStride = control.res.optData.deviceConfigRes.pciStride;
        ret->hwStride = control.res.optData.deviceConfigRes.hwStride;

        gdxSurfaceGetDesc(sfc, &desc);
        
        ret->lpSurface = desc.surface;
        ret->width = desc.width;
        ret->height = desc.height;
        ret->depth = desc.bytesPerPixel;
        ret->fbStride = desc.pitch;
        ret->fbOffset = ret->lpSurface - ret->lpLFB;
        ret->isTiled = (ret->fbOffset >= ret->tileBase) ? FXTRUE : FXFALSE;
        
        return FXTRUE;
} /* hwcGetSurfaceInfo */

FxU32
hwcAllocWinContext(hwcBoardInfo* bInfo)
{       
        CntrlParam  paramBlock;
        OSErr       myErr;
        hwcControl_t control;
        
        memset( &control, 0, sizeof( control ) );

        /* HLock((Handle)bInfo->hMon); */
        paramBlock.ioCRefNum = (*((GDHandle)bInfo->hMon))->gdRefNum;
        /* HUnlock((Handle)bInfo->hMon); */

        paramBlock.csCode = k3DfxAllocWinContext; /* driver-specific status request */
        *(hwcControl_t **)(&paramBlock.csParam[0]) = &control;
        
                        
        myErr = PBControl((ParmBlkPtr)&paramBlock, false);
                        
        if (myErr == noErr) {
                setLfbSwizzleMode(bInfo->regInfo.rawLfbBase,bInfo->regInfo.ioMemBase,control.res.optData.allocContextRes.depthMode);
                return control.res.optData.allocContextRes.contextID;
        }
        return 0;
}

FxBool
hwcFreeWinContext(hwcBoardInfo* bInfo,
                  FxU32         winContextId)
{
        CntrlParam  paramBlock;
        OSErr       myErr;
        hwcControl_t control;

        memset( &control, 0, sizeof( control ) );
        
        /* HLock((Handle)bInfo->hMon); */
        paramBlock.ioCRefNum = (*((GDHandle)bInfo->hMon))->gdRefNum;
        /* HUnlock((Handle)bInfo->hMon); */

        paramBlock.csCode = k3DfxReleaseWinContext; /* driver-specific status request */
        *(hwcControl_t **)(&paramBlock.csParam[0]) = &control;
        control.req.contextID = winContextId;
                        
        myErr = PBControl((ParmBlkPtr)&paramBlock, false);
                        
        return myErr == noErr ? FXTRUE : FXFALSE;
}

FxBool 
hwcAllocWinFifo(hwcBoardInfo* bInfo, 
                HwcWinFifo*   fifo,
                FxU32*        surface)
{
#define FN_NAME "hwcAllocWinFifo"
  FxBool 
    retVal = FXFALSE;

  /* Set undefined fifo type to start, and always clear the serial
   * number for this context.
   */
  fifo->fifoType = 0xFFFFFFFFUL;

  /* Check to see that we have valid surface owner to hang
   * the command fifo surface off of.
   */
  retVal = (surface != NULL);
  if (retVal) {
        GDHandle gDevice;
        GDXSurface_t 
                *objSentinal = NULL,
                *objFifo = NULL;
        GDXSurfaceDesc_t
                fifoDesc,
                sentinalDesc,
                surfaceDesc;
                
        /* Get the graphics devices for the surface so we can allocate new
         * surfaces on the same device. */
        
        gdxSurfaceGetDesc(surface, &surfaceDesc);
        
        gDevice = surfaceDesc.owner;

    memset(&fifoDesc, 0, sizeof(fifoDesc));

    /* Allocate a sentinal surface so that the client can 'know' when
     * a given fifo segment has actually been executed by reading a
     * client specified serial # out of the buffer. This only needs to
     * be big enough to hold the size of a serial #, but we need to
     * work around some sort of whacked agp bug when reading back. See
     * hwcExecuteStatusWinFifo() for details.
     */
    sentinalDesc = fifoDesc;

    sentinalDesc.height = 4;
    sentinalDesc.width  = 4;
    sentinalDesc.bytesPerPixel = 4;
    
    objSentinal = gdxSurfaceAlloc(gDevice, &sentinalDesc);
    
    if (!objSentinal) {
      GDBG_INFO(80, "%s: Could not allocate surface for serial #'s.\n", FN_NAME);
      retVal = FXFALSE;
      goto __errSurfaceFifo;
    }

        /* Assume for now that 2D command fifo is in local framebuffer space */
 
    /* Make pitch and width the same so that indexing is easy. 
     * Setup the allocation in hw page unit sizes. (4k bytes)
     */
    fifoDesc.pitch      = 0x1000UL;
    fifoDesc.width  = (fifoDesc.pitch >> 2UL);
    fifoDesc.bytesPerPixel = 4;
    
    /* Try to allocate a surface that contains some # of command
     * stream buffers and one persistant state buffer. This is
     * allocated in hw page units (4k) rather than the client
     * allocation units, but that should be transparent since
     * the client will just not use the extra slop.
     */
#define MAX(__a, __b) (((__a) > (__b)) ? (__a) : (__b))
    {
      const FxU32
        allocFifo  = MAX(fifo->cmdBuf.allocUnit, 0x1000UL),
        allocState = MAX(fifo->stateBuf.allocUnit, 0x1000UL),
        allocUnit  = pow2Round(allocFifo + allocState + 0xFFFUL, 0x1000UL);
      const char*
        numAllocStr = GETENV("FX_WINFIFO_INIT_ALLOC");
      FxU32
        numAlloc   = ((numAllocStr == NULL)
                      ? 0x8UL
                      : atoi(numAllocStr));

      while(numAlloc > 0) {
        fifoDesc.height = (allocUnit * numAlloc) / fifoDesc.pitch;
        /* objFifo = NULL; */
        objFifo = gdxSurfaceAlloc(gDevice,&fifoDesc);
        if(objFifo)
                break;

        /* Try a smaller allocation */
        numAlloc--;
      }
      if(!objFifo) {
        GDBG_INFO(80, "%s: Could not get cmdFifo GDX Surface.\n",
                  FN_NAME);
        retVal = FXFALSE;
        goto __errSurfaceFifo;
      }
      
      /* Its now safe to overwrite the client allocUnit's to the
       * rounded allocation units because we've finally done all of
       * the stuff that could fail.
       */
      fifo->cmdBuf.size = allocFifo * numAlloc;
      fifo->cmdBuf.allocUnit = allocFifo;

      fifo->stateBuf.size = allocState * numAlloc;
      fifo->stateBuf.allocUnit = allocState;
    }
#undef MAX
      
        retVal = FXTRUE;
    /* Fill in some more of the return buffer info, but were not quite
     * done and ready to declare success until we lock the fifo down.  
     */
    fifo->fifoType = HWCEXT_FIFO_FB;

    fifo->surfaceFifo     = (FxU32*)objFifo;
    fifo->surfaceSentinal = (FxU32*)objSentinal;

    /* Do the lock so that we can write the sentinal, but the
     * client is responsible for locking before use.
     */
    if (hwcLockWinFifo(bInfo, fifo)) {
      /* Write bogus serial # */
      *(volatile FxU32*)fifo->sentinalBufferAddr = 0x0UL;
    
      hwcUnlockWinFifo(bInfo, fifo);
    }
    
 __errSurfaceFifo:
    /* Free our directdraw objects if something bad happened. */
    if (!retVal) {
      if (objSentinal != NULL) gdxSurfaceFree(objSentinal);
      if (objFifo != NULL) gdxSurfaceFree(objFifo);
    }
  }

  /* Could not allocate directly accessible fifo so setup the host
   * memory backed fifo which will get dumped at execute time.  
   */
  if (!retVal) {
    fifo->fifoType          = HWCEXT_FIFO_HOST;

    fifo->cmdBuf.hwOffset    = 0x00UL;
    fifo->cmdBuf.size   = fifo->cmdBuf.allocUnit;
    fifo->stateBuf.hwOffset  = 0x00UL;
    fifo->stateBuf.size = fifo->stateBuf.allocUnit;

    /* This can never fail since the client is responsible for
     * allocating this memory.  
     */
    retVal = FXTRUE;
  }

  return retVal;
#undef FN_NAME
}                

FxBool
hwcLockWinFifo(hwcBoardInfo* bInfo,
               HwcWinFifo* fifo)
{
#define FN_NAME "hwcLockWinFifo"
  FxBool 
    retVal = FXFALSE;

  if (fifo->fifoType == HWCEXT_FIFO_HOST) {
    retVal = FXTRUE;
  } else if ((fifo->fifoType == HWCEXT_FIFO_FB) ||
             (fifo->fifoType == HWCEXT_FIFO_AGP)) {
       
    GDXSurfaceDesc_t 
        fifoDesc,
        sentinalDesc;
              
    if (fifo->lockCount > 0) goto __alreadyLocked;
    if ((fifo->surfaceFifo == NULL) ||
        (fifo->surfaceSentinal == NULL)) goto __errFifoLock;

    /* Get base pointer to the fifo surfaces.
     */
     
    gdxSurfaceGetDesc(fifo->surfaceFifo,&fifoDesc);
    gdxSurfaceGetDesc(fifo->surfaceSentinal,&sentinalDesc);
    
    /* Since there's no remapping crap in MacOS, this is easy. */
    fifo->cmdBuf.baseAddr               = fifoDesc.surface; 
    fifo->stateBuf.baseAddr     = fifo->cmdBuf.baseAddr + fifo->cmdBuf.size;
    fifo->sentinalBufferAddr    = sentinalDesc.surface;

        /* Compute hardware offsets as well */
        fifo->cmdBuf.hwOffset = fifo->cmdBuf.baseAddr - bInfo->regInfo.rawLfbBase;
        fifo->stateBuf.hwOffset = fifo->stateBuf.baseAddr - bInfo->regInfo.rawLfbBase;
        fifo->sentinalBufferOffset = fifo->sentinalBufferAddr - bInfo->regInfo.rawLfbBase;
                
   __alreadyLocked:
    /* Finally, declare success */
    retVal = FXTRUE;
    
   __errFifoLock:
    ;
  }

  if (retVal) fifo->lockCount++;

  return retVal;
#undef FN_NAME
}

FxBool
hwcUnlockWinFifo(hwcBoardInfo* bInfo,
                 HwcWinFifo* fifo)
{
#define FN_NAME "hwcUnlockWinFifo"
  FxBool
    retVal = ((fifo->lockCount > 0) &&
              ((fifo->fifoType == HWCEXT_FIFO_HOST) ||
               (fifo->fifoType == HWCEXT_FIFO_FB) ||
               (fifo->fifoType == HWCEXT_FIFO_AGP)));

  if (retVal) {
    fifo->lockCount--;

    if (fifo->lockCount == 0) {
      if ((fifo->fifoType == HWCEXT_FIFO_FB) ||
          (fifo->fifoType == HWCEXT_FIFO_AGP)) {
        /* Don't currently do anything since surfaces don't get moved 
         * around yet on MacOS.
         */
      }
    }
  }

  return retVal;
#undef FN_NAME
}

FxBool
hwcFreeWinFifo( hwcBoardInfo* bInfo,
                HwcWinFifo* fifo )
{
#define FN_NAME "hwcFreeWinFifo"

  /* What type of fifo do we have? */
  if ((fifo->fifoType == HWCEXT_FIFO_FB) ||
      (fifo->fifoType == HWCEXT_FIFO_AGP)) {
    if (fifo->surfaceFifo != NULL) {
        gdxSurfaceFree(fifo->surfaceFifo);

      fifo->surfaceFifo = NULL;
      fifo->cmdBuf.baseAddr = 
      fifo->cmdBuf.hwOffset = 
      fifo->cmdBuf.size = 
      fifo->stateBuf.baseAddr = 
      fifo->stateBuf.hwOffset = 0x00UL;
    }
    
    if (fifo->surfaceSentinal != NULL) {
        gdxSurfaceFree(fifo->surfaceSentinal);

      fifo->surfaceSentinal = NULL;
      fifo->sentinalBufferAddr =
      fifo->sentinalBufferOffset = 0x00UL;
    }
  }
  
  fifo->fifoType = HWCEXT_FIFO_INVALID;

  return FXTRUE;
#undef FN_NAME
}

FxBool
hwcExecuteWinFifo(hwcBoardInfo*     bInfo, 
                  const FxU32       winContextId,
                  const HwcWinFifo* fifo,
                  const FxU32       serialNumber)
{
        CntrlParam  paramBlock;
        OSErr       myErr;
        hwcControl_t control;
        
        memset( &control, 0, sizeof( control ) );

        /* HLock((Handle)bInfo->hMon); */
        paramBlock.ioCRefNum = (*((GDHandle)bInfo->hMon))->gdRefNum;
        /* HUnlock((Handle)bInfo->hMon); */

        paramBlock.csCode = k3DfxExecuteFifo; /* driver-specific status request */
        *(hwcControl_t **)(&paramBlock.csParam[0]) = &control;

        control.req.contextID = winContextId;
        control.req.optData.executeFifoReq.fifoType     = fifo->fifoType;
        control.req.optData.executeFifoReq.fifoPtr      = fifo->cmdBuf.baseAddr;
        control.req.optData.executeFifoReq.fifoSize     = fifo->cmdBuf.size >> 2UL;
        control.req.optData.executeFifoReq.statePtr     = fifo->stateBuf.baseAddr;
        control.req.optData.executeFifoReq.stateSize    = fifo->stateBuf.size >> 2UL;
        control.req.optData.executeFifoReq.serialNumber = serialNumber;

        if (fifo->fifoType != HWCEXT_FIFO_HOST) {
        control.req.optData.executeFifoReq.fifoOffset     = fifo->cmdBuf.hwOffset;
        control.req.optData.executeFifoReq.stateOffset    = fifo->stateBuf.hwOffset;
        control.req.optData.executeFifoReq.sentinalOffset = fifo->sentinalBufferOffset;
        }
                        
        myErr = PBControl((ParmBlkPtr)&paramBlock, false);
                        
        return myErr == noErr ? FXTRUE : FXFALSE;
}

FxU32
hwcExecuteStatusWinFifo(hwcBoardInfo*     bInfo,
                        const HwcWinFifo* fifo,
                        const FxU32       serialNumber)
{
#define FN_NAME "hwcExecuteStatusWinFifo"
  FxU32
    retVal = ~0x00UL;

  switch(fifo->fifoType) {
  /* Host fifo always executes synchronously */
  case HWCEXT_FIFO_HOST:
    retVal = serialNumber;
    break;

  case HWCEXT_FIFO_FB:
  case HWCEXT_FIFO_AGP:
    {
      volatile FxU32*
        bufAddr = (volatile FxU32*)fifo->sentinalBufferAddr;

      /* We need to do some extra reads here so that we make sure taht
       * the data we're returning is coherent w/ the actual frame
       * buffer due to the intervening pixel cache. The cache is,
       * currently, 4 dwords, and appears to be aligned on this
       * boundary as well so reading 4 dwords away should be flushing.
       *
       * NB: The surface for the serial # was allocated big enough to
       * handle this so we would not have to worry about it here.  */
      retVal = (bufAddr[0]);
      retVal = (bufAddr[0 + 4]);
      retVal = (bufAddr[0]);
    }
    break;

  case HWCEXT_FIFO_INVALID:
  default:
    GDBG_ERROR(FN_NAME, "Invalid fifoType");
    break;
  }

  return retVal;
#undef FN_NAME
}                        

/* #define LINEAR_STRIDE_ALIGN 16UL */
/* #define TILE_BIT            0x00008000UL */

#endif /* HWC_ACCESS_GDX */

FxBool
hwcInitVideo(hwcBoardInfo *bInfo, FxBool tiled, FxVideoTimingInfo *vidTiming,
             FxBool overlay)
{
#define FN_NAME "hwcInitVideo"
  FxU32 pixFmt = SST_OVERLAY_PIXEL_RGB565U;
  FxU32
    stride= (tiled) ? bInfo->buffInfo.bufStrideInTiles : bInfo->vidInfo.stride;
  FxU32
    scrWidth, scrHeight,
    ovlWidth, ovlHeight,
    vidProcCfg, vidScreenSize, vidOverlayEndScreenCoord,
    vidOverlayDudx, vidOverlayDvdy,
    dramInit1;

  FxBool
    lfbMemoryConfig,
    miscInit0;

  float
    scale;


#ifdef HWC_EXT_INIT 
  hwcExtRequest_t
    ctxReq;
  hwcExtResult_t
    ctxRes;
#endif
  
  {
    FxU32 refresh;
    static FxU32 refConstToRefreshHz[] = {
      60,                       /* GR_REFRESH_60Hz   */
      70,                       /* GR_REFRESH_70Hz   */
      72,                       /* GR_REFRESH_72Hz   */
      75,                       /* GR_REFRESH_75Hz   */
      80,                       /* GR_REFRESH_80Hz   */
      90,                       /* GR_REFRESH_90Hz   */
      100,                      /* GR_REFRESH_100Hz  */
      85,                       /* GR_REFRESH_85Hz   */
      120,                      /* GR_REFRESH_120Hz  */
      0
    };  

    if (bInfo->vidInfo.vRefresh > GR_REFRESH_120Hz)
      refresh = 0;
    else
      refresh = bInfo->vidInfo.vRefresh;

    if ( !setVideoMode( (void*)bInfo->vidInfo.hWnd, 
                       bInfo->vidInfo.xRes,
                       bInfo->vidInfo.yRes,
                       refConstToRefreshHz[refresh], /* Make sure we use the table, otherwise
                                                                                        we will always pass 0Hz to setVideoMode */
                       bInfo->hMon ) ) {
      GDBG_INFO(80, "%s:  dxOpen() failed!\n", FN_NAME);
      return FXFALSE;
    }
  }

#ifdef HWC_EXT_INIT
  ctxReq.which = HWCEXT_HWCSETEXCLUSIVE;
  ctxReq.optData.linearAddrReq.devNum = 0;
  
  
  GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_HWCSETEXCLUSIVE\n");
  ExtEscape((HDC) bInfo->hdc, EXT_HWC, sizeof(ctxReq), (void *) &ctxReq,
            sizeof(ctxRes), (void *) &ctxRes);
  
  if (ctxRes.resStatus != 1) {
    strcpy(errorString, "HWCEXT_HWCSETEXCLUSIVE Failed");
    return FXFALSE;
  }
#else
  /* This is off for now until the rest of the alt-tab type things are done. */
#if 0
  /* Before letting glide party on the hw check to see if we're on a
   * system w/ a 2d environment, and make sure taht its happy about us
   * doing this before dorking w/ things that are going to make it
   * unhappy.  
   */
  if (Dpmi2DEnvironmentP()) {
    bInfo->hdc = DpmiDeviceContextGet("3DFXVB");
    if (bInfo->hdc == 0x00UL) return FXFALSE;
    
    if (!DpmiDeviceContextDispatch(bInfo->hdc, FxDCIsFullscreenP)) return FXFALSE;
    if (!DpmiDeviceContextDispatch(bInfo->hdc, FxDCExclusiveLock)) return FXFALSE;
  }
#endif
#endif
  
  HWC_IO_STORE(bInfo->regInfo, vidOverlayDudxOffsetSrcWidth,
    ((bInfo->vidInfo.xRes << 1) << 19));

  /* Video pixel buffer threshold */
  {
    FxU32 vidPixelBufThold;
    FxU32 thold = 32;

    if (getenv("SSTVB_PIXTHOLD")) {
      thold = atoi(getenv("SSTVB_PIXTHOLD"));
    }

    thold &= 0x3f;

    vidPixelBufThold = (thold | (thold << 6) | (thold << 12));

    HWC_IO_STORE(bInfo->regInfo, vidPixelBufThold, vidPixelBufThold);
  }

#ifdef __WIN32__
  if (vidTiming) {
    hwcExtRequest_t req;
    hwcExtResult_t res;

    req.which = HWCEXT_VIDTIMING;
    req.optData.vidTimingReq.vidTiming = (void *) vidTiming;
    
    GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_VIDTIMING\n");
    ExtEscape((HDC) bInfo->hdc, EXT_HWC, sizeof(req), (void *) &req,
      sizeof(res), (void *) &res);

    /* Ignore failure */

  }
#endif

#ifdef __DOS32__
  /* Now call the cinit code */

  h3InitVideoOverlaySurface(
    bInfo->regInfo.ioPortBase,
    FXTRUE,                     /* 1=enable Overlay surface (OS), 1=disable */
    FXFALSE,                    /* 1=enable OS stereo, 0=disable */
    FXFALSE,                    /* 1=enable horizontal scaling, 0=disable */
    0,                          /* horizontal scale factor (ignored if not) */
    FXFALSE,                    /* 1=enable vertical scaling, 0=disable */
    0,                          /* vertical scale factor (ignored if not) */
    0,                          /* Filter mode */
    FXTRUE,                     /* tiled */
    pixFmt,                     /* pixel format of OS */
    FXTRUE,                    /* bypass clut for OS? */
    0,                          /* 0=lower 256 CLUT entries, 1=upper 256 */
    bInfo->buffInfo.colBuffStart[0],/* board address of beginning of OS */
    stride);                    /* distance between scanlines of the OS, in
                                   units of bytes for linear OS's and tiles for
                                   tiled OS's */
  HWC_IO_STORE(bInfo->regInfo, vidOverlayStartCoords, 0);
  HWC_IO_STORE(bInfo->regInfo, vidOverlayEndScreenCoord,
    (bInfo->vidInfo.yRes  << SST_OVERLAY_Y_SHIFT) |
    (bInfo->vidInfo.xRes & SST_OVERLAY_X) );
#else
  hwcInitVideoOverlaySurface(
    &bInfo->regInfo,
    FXTRUE,                     /* 1=enable Overlay surface (OS), 1=disable */
    FXFALSE,                    /* 1=enable OS stereo, 0=disable */
    FXFALSE,                    /* 1=enable horizontal scaling, 0=disable */
    0,                          /* horizontal scale factor (ignored if not) */
    FXFALSE,                    /* 1=enable vertical scaling, 0=disable */
    0,                          /* vertical scale factor (ignored if not) */
    0,                          /* Filter mode */
    FXTRUE,                     /* tiled */
    pixFmt,                     /* pixel format of OS */
    FXFALSE,                    /* bypass clut for OS? */
    0,                          /* 0=lower 256 CLUT entries, 1=upper 256 */
    bInfo->buffInfo.colBuffStart[0],/* board address of beginning of OS */
    stride);                    /* distance between scanlines of the OS, in
                                   units of bytes for linear OS's and tiles for
                                   tiled OS's */
  HWC_IO_STORE(bInfo->regInfo, vidOverlayStartCoords, 0);
  
#endif
  
  /* 
     Setup video scaling for half-modes
   */

  /* Get some important info */
  HWC_IO_LOAD(bInfo->regInfo, vidScreenSize, vidScreenSize);
  HWC_IO_LOAD(bInfo->regInfo, vidProcCfg, vidProcCfg);
  HWC_IO_LOAD(bInfo->regInfo, vidOverlayEndScreenCoord, vidOverlayEndScreenCoord);

  scrWidth = (vidScreenSize >> SST_VIDEO_SCREEN_WIDTH_SHIFT) & 0xfff;
  scrHeight = (vidScreenSize >> SST_VIDEO_SCREEN_HEIGHT_SHIFT) & 0xfff;

  ovlWidth = ((vidOverlayEndScreenCoord) >> SST_OVERLAY_X_SHIFT) & 0xfff;
  ovlHeight = ((vidOverlayEndScreenCoord) >> SST_OVERLAY_Y_SHIFT) & 0xfff;  

  ovlWidth += 1;
  ovlHeight += 1;

  /* Check to see if the screen and overlay dimensions match. 
   * There are two cases that can happen in reality.
   *
   * scrXXX > appXXX: This is a 'real' case, and the overlay dimension
   * needs to mag scaled so that it fits the requested size.
   *
   * (scrXXX == appXXX) && (ovlXXX != scrXXX): This is a somewhat artificial
   * case where someone left the overlay set to some value, and these did
   * not get reset in the setVideoMode processing. (For example, if the user is
   * running an application which bus masters data directly to our video overlay
   * when launching a glide application). In this case we need to fiddle w/ the
   * overlay dimension so that it matches the requested resolution.
   *
   * (scrXXX < appXXX): If setVideoMode is actually working correctly, this cannot
   * happen because that code has to know that we can't do min scaling.
   */

  if (scrWidth > bInfo->vidInfo.xRes) {
    vidProcCfg |= SST_OVERLAY_HORIZ_SCALE_EN;

    ovlWidth = scrWidth;

    scale = ((float) bInfo->vidInfo.xRes) / ((float) ovlWidth);

    vidOverlayDudx = (FxU32) (scale * ((float) (1 << 20)));
    HWC_IO_STORE(bInfo->regInfo, vidOverlayDudx, vidOverlayDudx);
  } else if (ovlWidth != scrWidth) {
    ovlWidth = scrWidth;
  }
  
  if (scrHeight > bInfo->vidInfo.yRes) {
    vidProcCfg |= SST_OVERLAY_VERT_SCALE_EN;

    ovlHeight = scrHeight;

    scale = ((float) bInfo->vidInfo.yRes) / ((float) ovlHeight);

    vidOverlayDvdy = (FxU32) (scale * ((float) (1 << 20)));
    HWC_IO_STORE(bInfo->regInfo, vidOverlayDvdy, vidOverlayDvdy);
  } else if (ovlHeight != scrHeight) {
    ovlHeight = scrHeight;
  }
  
  vidOverlayEndScreenCoord = (((ovlHeight - 1) << SST_OVERLAY_Y_SHIFT) |
                              ((ovlWidth - 1) << SST_OVERLAY_X_SHIFT));
  HWC_IO_STORE(bInfo->regInfo, vidOverlayEndScreenCoord, vidOverlayEndScreenCoord);

  vidProcCfg &= ~SST_OVERLAY_FILTER_MODE;
  if (GETENV("SSTH3_OVERLAY_FILTER_2x2")) {
    if (!(vidProcCfg & SST_VIDEO_2X_MODE_EN))
      vidProcCfg |= SST_OVERLAY_FILTER_2X2;
  } else {
    vidProcCfg |= SST_OVERLAY_FILTER_4X4;
  }
  
  HWC_IO_STORE(bInfo->regInfo, vidProcCfg, vidProcCfg);

  /* Get miscInit0 for y-sub  */
  HWC_IO_LOAD(bInfo->regInfo, miscInit0, miscInit0);

  /* Clear out relavent bits */
  miscInit0 &= ~SST_YORIGIN_TOP;
  miscInit0 |= ((bInfo->vidInfo.yRes - 1)  << SST_YORIGIN_TOP_SHIFT);

  HWC_IO_STORE(bInfo->regInfo, miscInit0, miscInit0);
  
  /* Set up lfbMemoryConfig */
  lfbMemoryConfig =
    (bInfo->fbOffset >> 12) | 
    SST_RAW_LFB_ADDR_STRIDE_4K |
    (bInfo->buffInfo.bufStrideInTiles << SST_RAW_LFB_TILE_STRIDE_SHIFT);

  HWC_IO_STORE(bInfo->regInfo, lfbMemoryConfig, lfbMemoryConfig);

  /* Filthy memclock hack */
  if (getenv("H3_MEM_CLOCK")) {
    int mHz;
    FxU32 pllVal;
    mHz = atoi(getenv("H3_MEM_CLOCK"));

    switch (mHz) {
    case 50:
      pllVal = 0x2806;
      break;
      
    case 75:
      pllVal = 0x7125;
      break;
      
    case 80:
      pllVal = 0x7925;
      break;

    case 100:
      pllVal = 0x2805;
      break;

    default:
      pllVal = 0;
      gdbg_printf("%s:  Bogus MEMCLOCK setting!\n", FN_NAME);
      break;
    }

    if (pllVal) {
      HWC_IO_STORE(bInfo->regInfo, pllCtrl1, pllVal);
      HWC_IO_STORE(bInfo->regInfo, pllCtrl2, pllVal);
      gdbg_printf("%s:  Setting memory clock to %d MHz\n", FN_NAME,
        mHz);
    }

  }

  /* Set up dramInit1 for triple or double buffering */
  HWC_IO_LOAD(bInfo->regInfo, dramInit1, dramInit1);
  if (bInfo->vidInfo.tripleBuffering)
    dramInit1 |= SST_TRIPLE_BUFFER_EN;
  else
    dramInit1 &= ~SST_TRIPLE_BUFFER_EN;

  HWC_IO_STORE(bInfo->regInfo, dramInit1, dramInit1);

  HWC_IO_STORE(bInfo->regInfo, vidMaxRGBDelta, 0x100810);

  HWC_IO_STORE( bInfo->regInfo, vidDesktopOverlayStride,
                ( bInfo->buffInfo.bufStrideInTiles << 16 ) |
                  bInfo->buffInfo.bufStrideInTiles );

  return FXTRUE;

#undef FN_NAME
} /* hwcInitVideo */

FxBool
hwcRestoreVideo(hwcBoardInfo *bInfo)
{
#define FN_NAME "hwcRestoreVideo"
  FxU32 depth;

  /* Disable FIFO */
  do {
    HWC_CAGP_LOAD(bInfo->regInfo, cmdFifo0.depth, depth);
  } while (depth);

  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize, 0);

#ifdef HWC_ACCESS_DDRAW

  {
    hwcExtRequest_t req;
    hwcExtResult_t res;

    req.which = HWCEXT_HWCRLSEXCLUSIVE;
    req.optData.linearAddrReq.devNum = 0;
    GDBG_INFO(90, FN_NAME ":  ExtEscape:HWCEXT_HWCRLSEXCLUSIVE\n");
    ExtEscape((HDC) bInfo->hdc, EXT_HWC, sizeof(req), (void *) &req,
      sizeof(res), (void *) &res);

    GDBG_INFO(80, "%s:  sizeof(res) = %d\n", FN_NAME, sizeof(res));
    GDBG_INFO(80, "%s:  res.resStatus = %d\n", FN_NAME, res.resStatus);
    GDBG_INFO(80, "%s:  &res = 0x%x\n", FN_NAME, &res);

    if (res.resStatus != 1) {
      strcpy(errorString, "HWCEXT_HWCRLSEXCLUSIVE Failed");
      return FXFALSE;
    }
    
    req.which = HWCEXT_RELEASECONTEXT;
    req.optData.releaseContextReq.contextID = bInfo->extContextID;
    GDBG_INFO(90, FN_NAME ":  ExtEscape:HWCEXT_RELEASECONTEXT\n");    
    ExtEscape((HDC) bInfo->hdc, EXT_HWC, sizeof(req), (void *) &req,
      sizeof(res), (void *) &res); 
  }

#ifdef HWC_EXT_INIT


#endif
#endif

#ifndef HWC_EXT_INIT
  /* This is off for now until the rest of the alt-tab stuff is done */
#if 0
  /* Release our exclusive hw lock */
  DpmiDeviceContextDispatch(bInfo->hdc, FxDCUnlock);
#endif
#endif /* HWC_EXT_INIT */

  /* Restore display */
  resetVideo();

  return FXTRUE;
#undef FN_NAME
} /* hwcRestoreVideo */

const char *
hwcGetErrorString()
{
#define FN_NAME "hwcGetErrorString"
  return errorString;
#undef FN_NAME
} /* hwcGetErrorString */

FxBool
hwcCheckMemSize(hwcBoardInfo *bInfo, FxU32 xres, FxU32 yres, FxU32 nColBuffers,
                FxU32 nAuxBuffers, FxBool tiled)
{
#define FN_NAME "hwcCheckMemSize"
  FxU32
    bufSize, totSize;

  bufSize = calcBufferSize(xres, yres, tiled);

  totSize = (nColBuffers + nAuxBuffers) * bufSize;

  if (totSize < ((bInfo->h3Mem << 20) - bInfo->min_tramSize)) /* Need 2M for texture */
    return FXTRUE;
  else
    return FXFALSE;    
#undef FN_NAME
} /* hwcCheckMemSize */

static FxU32
calcBufferStride(FxU32 xres, FxBool tiled)
{
  FxU32
    strideInTiles;

  if (tiled == FXTRUE) {
    /* Calculate tile width stuff */
    strideInTiles = (xres << 1) >> 7;
    if ((xres << 1) & (HWC_TILE_WIDTH - 1))
      strideInTiles++;
    
    return (strideInTiles * HWC_TILE_WIDTH);

  } else {
    return (xres << 1);
  }
} /* calcBufferStride */

static FxU32
calcBufferHeightInTiles(FxU32 yres)
{
  FxU32
    heightInTiles;            /* Height of buffer in tiles */


  /* Calculate tile height stuff */
  heightInTiles = yres >> 5;
  
  if (yres & (HWC_TILE_HEIGHT - 1))
    heightInTiles++;

  return heightInTiles;

} /* calcBufferHeightInTiles */

static FxU32
calcBufferSizeInTiles(FxU32 xres, FxU32 yres) {
  FxU32
    bufSizeInTiles;           /* Size of buffer in tiles */

  bufSizeInTiles =
    calcBufferHeightInTiles(yres) * (calcBufferStride(xres, FXTRUE) >> 7);

  return bufSizeInTiles;

} /* calcBufferSizeInTiles */

static FxU32
calcBufferSize(FxU32 xres, FxU32 yres, FxBool tiled)
{
  FxU32
    stride,
    height,
    bufSize;                  /* Size of buffer in bytes */

  if (tiled) {
    stride = calcBufferStride(xres, tiled);
    height = HWC_TILE_HEIGHT * calcBufferHeightInTiles(yres);
  } else {
    stride = xres << 1;
    height = yres;
  }

  bufSize = stride * height;
  
  return bufSize;

} /* calcBufferSize */

/* How the hw treats lfb accesses are dependent on the 'type' of
 * memory (tiled/linear) that the color/aux buffers are in. We
 * pre-compute the actual lfb address here while we know about the
 * memory space and if we adjusted the page alignment above.
 *
 * NB: If we are in tiled mode then the fact that we align the color
 * buffers on page boundaries means that the y offset of the buffers
 * may not actually be on a boundary for the tile addressing scheme.
 * The 'rounding' done to HWC_TILED_BUFFER_Y_ALIGN adjust for this.
 *
 * NB: The memory optimization of aligning color buffers on even page
 * boundaries will cause the tiled lfb access to be off by a page so
 * we add in the width of a page (HWC_TILED_BUFFER_X_ADJUST) here.
 */
#define HWC_TILED_BUFFER_BYTES    0x1000UL  /* 128 Bytes x 32 lines */
#define HWC_TILED_BUFFER_Y_ALIGN  0x20000UL
#define HWC_TILED_BUFFER_X_ADJUST 0x80UL

static FxU32
hwcBufferLfbAddr(FxU32 bufNum, 
                 const hwcBoardInfo* bInfo,
                 FxBool colBufAlignP)
{
  FxU32 retVal = 0x00UL;

  if (bInfo->vidInfo.tiled) {
    retVal = (bInfo->fbOffset + 
              pow2Round(bufNum * bInfo->vidInfo.yRes * HWC_TILED_BUFFER_BYTES, 
                        HWC_TILED_BUFFER_Y_ALIGN) +
              colBufAlignP * HWC_TILED_BUFFER_X_ADJUST);
  } else if (bufNum < bInfo->buffInfo.nColBuffers) {
    retVal = bInfo->buffInfo.colBuffStart[bufNum];
  } else if (bufNum == bInfo->buffInfo.nColBuffers) {
    retVal = bInfo->buffInfo.auxBuffStart;
  }

  return retVal;
}

static FxU32
pow2Round(FxU32 val, FxU32 pow2Const)
{
  const FxU32 pow2Mask = (pow2Const - 1UL);

  return ((val + pow2Mask) & ~pow2Mask);
}

FxU32 
hwcInitAGPFifo(hwcBoardInfo *bInfo, FxBool enableHoleCounting) 
{
#define FN_NAME "hwcInitAGPFifo"
#if HWC_EXT_INIT
  FxU32
    agpSize, agpLAddr, agpPAddr;
  FxU32
    cagpRegs;                   /* pointer to Cmd/AGP regs */
  hwcExtRequest_t req;
  hwcExtResult_t res;


  if (bInfo->regInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcMapBoard\n", FN_NAME);
    return FXFALSE;
  }
  cagpRegs = bInfo->regInfo.cmdAGPBase;

  if (bInfo->buffInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcInitBuffers\n", FN_NAME);
    return FXFALSE;
  }

  req.which = HWCEXT_GETAGPINFO;

  GDBG_INFO(80, FN_NAME ":  ExtEscape:HWCEXT_GETAGPINFO\n");
  ExtEscape((HDC) bInfo->hdc, EXT_HWC, sizeof(req), (void *) &req,
    sizeof(res), (void *) &res);

  /* If we fail, bail and go to memory fifo */
  if (res.resStatus != 1) {
    return hwcInitFifo(bInfo, enableHoleCounting); 
  }

  agpLAddr = res.optData.agpInfoRes.lAddr;
  agpPAddr = res.optData.agpInfoRes.pAddr;
  agpSize = res.optData.agpInfoRes.size;

  /* disable the CMD fifo */
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize, 0);

  bInfo->fifoInfo.agpFifo = 1;
  bInfo->fifoInfo.agpVirtAddr = agpLAddr;
  bInfo->fifoInfo.agpPhysAddr = agpPAddr;
  bInfo->fifoInfo.fifoStart = agpPAddr;
  bInfo->fifoInfo.agpSize = agpSize;

  GDBG_INFO(80, "%s AGP linear address = 0x%x\n", FN_NAME, agpLAddr);
  GDBG_INFO(80, "%s AGP physical address = 0x%x\n", FN_NAME, agpPAddr);
  GDBG_INFO(80, "%s AGP Size = 0x%x\n", FN_NAME, agpSize);

#if 0
  From Windows code:

  SETDW(_FF(lpCRegs)->cmdFifo0.baseSize, 0);
  SETDW(_FF(lpCRegs)->cmdFifo0.baseAddrL,
    (_FF(agpPhysAddr) >> 12) );
  SETDW(_FF(lpCRegs)->cmdFifo0.readPtrL, _FF(agpPhysAddr) );
  SETDW(_FF(lpCRegs)->cmdFifo0.readPtrH,  0);
  SETDW(_FF(lpCRegs)->cmdFifo0.aMin,  _FF(agpPhysAddr) - 4);
  SETDW(_FF(lpCRegs)->cmdFifo0.aMax,  _FF(agpPhysAddr) - 4);
  SETDW(_FF(lpCRegs)->cmdFifo0.depth, 0);
  SETDW(_FF(lpCRegs)->cmdFifo0.holeCount, 0);
  SETDW(_FF(lpCRegs)->cmdFifoThresh, 0x122);
  SETDW(_FF(lpCRegs)->cmdFifo0.baseSize, 0x700);
#endif

  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseAddrL,
    bInfo->fifoInfo.fifoStart>>12);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.readPtrL, bInfo->fifoInfo.fifoStart);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.readPtrH, 0);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.aMin, bInfo->fifoInfo.fifoStart-4);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.aMax, bInfo->fifoInfo.fifoStart-4);
  
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.depth, 0);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.holeCount, 0);
  /* Fifo LWM /HWM/ THRESHOLD */
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifoThresh, (0x09 << SST_HIGHWATER_SHIFT) | 0x2); 
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize,
    ((bInfo->fifoInfo.fifoLength >> 12) - 1) | SST_EN_CMDFIFO |
    SST_CMDFIFO_DISABLE_HOLES | SST_CMDFIFO_AGP);
  
  GDBG_INFO(2,"%s:  CMD FIFO placed at physical addr 0x%x\n", FN_NAME,
    bInfo->fifoInfo.fifoStart);

  return FXTRUE;
#else /* !HWC_EXT_INIT */
  return hwcInitFifo(bInfo, enableHoleCounting);
#endif /* !HWC_EXT_INIT */
#undef FN_NAME
} /* hwcInitAGPFifo */ 


FxBool
hwcAllocAuxRenderingBuffer(hwcBoardInfo *bInfo, hwcBufferDesc *bp, int width,
                           int height)  
{
#define FN_NAME "hwcAllocAuxRenderingBuffer"
  /* For now, these buffers  */
  FxBool
    rVal = FXFALSE;
  
  FxU32
    bufSize,
    offset;

  bufSize = (width * height) << 2;

  offset = bInfo->fbOffset - bufSize;

  if ((offset - bInfo->tramSize) < bInfo->min_tramSize) {
    sprintf(errorString, "%s:  Insufficient memory", FN_NAME);
  } else {
    bp->bufOffset = offset;
    bp->bufStride = width << 2; /* Below tiled memory for now */
    bp->bufBPP = 16;
    bp->tiled = 0;
    bp->bufType = HWC_BUFFER_AUXRENDER;

    bInfo->fbOffset -= bufSize;

    rVal = FXTRUE;
  }

  return rVal;

#undef FN_NAME
} /* hwcAllocAuxRenderingBuffer */

#define RED_SHIFT       16
#define GREEN_SHIFT     8
#define BLUE_SHIFT      0

FxBool
hwcGammaTable(hwcBoardInfo *bInfo, FxU32 nEntries, FxU32 *r, FxU32 *g, FxU32 *b)
{
#define FN_NAME "hwcGammaTable"
  FxU32 gRamp[256];
  FxU32 i;
  FxU32 vidProcCfg;
  FxU32 dacBase;
  
  /* Load the table into the Display driver as above */
  for (i = 0; i < nEntries; i++) {
    gRamp[i] =
      ((r[i] & 0xff) << RED_SHIFT) |
        ((g[i] & 0xff) << GREEN_SHIFT) |  
          ((b[i] & 0xff) << BLUE_SHIFT);
  }
  
  /*
   **  On W9X/DOS, we can do this ourselves--which is much easier than
   **  mucking about with a bunch of 32-bit data in 16-bit driver
   **  code.
   */
  HWC_IO_LOAD( bInfo->regInfo, vidProcCfg, vidProcCfg);
  
  /* Determin which set of CLUT entries are selected */
  if (vidProcCfg & SST_OVERLAY_CLUT_SELECT)
    dacBase = 256;
  else
    dacBase = 0;
 
  /* Print out some useful info RE the vidProcCfg register */
  GDBG_INFO(80, "%s:  vidProcCFG(SST_OVERLAY_CLUT_SELECT) = %d\n",
            FN_NAME, (vidProcCfg & SST_OVERLAY_CLUT_SELECT) ? 1 : 0);
  GDBG_INFO(80, "%s:  vidProcCFG(SST_OVERLAY_EN) = %d\n",
            FN_NAME, (vidProcCfg & SST_OVERLAY_EN) ? 1 : 0);
  GDBG_INFO(80, "%s:  vidProcCFG(SST_OVERLAY_CLUT_BYPASS) = %d\n",
            FN_NAME, (vidProcCfg & SST_OVERLAY_CLUT_BYPASS) ? 1 : 0);
  
  for (i = 0; i < nEntries; i++) {
    HWC_IO_STORE( bInfo->regInfo, dacAddr, dacBase + i);
    P6FENCE;
    HWC_IO_STORE( bInfo->regInfo, dacData, gRamp[i]);
    P6FENCE;
  }

  return FXTRUE;
  
#undef FN_NAME
} /* hwcGammaTable */

FxBool
hwcGammaRGB(hwcBoardInfo *bInfo, float gammaR, float gammaG, float gammaB)
{
#define FN_NAME "hwcGammaRGB"
  FxU32
    grRamp[256], ggRamp[256], gbRamp[256];
  int
    i;

  GDBG_INFO(80, FN_NAME "(0x%x, %1.2f, %1.2f, %1.2f)\n",
            bInfo, gammaR, gammaG, gammaB);

  /*
  ** NB:  The system eventually devised by Bob and Ken *may* require
  ** separate R, G, and B vectors.
  */

  for (i = 0; i < 256; i++) {
    grRamp[i] = (FxU32)((pow(i/255.0F, 1.0F/gammaR)) * 255.0F + 0.5F);
    ggRamp[i] = (FxU32)((pow(i/255.0F, 1.0F/gammaG)) * 255.0F + 0.5F);
    gbRamp[i] = (FxU32)((pow(i/255.0F, 1.0F/gammaB)) * 255.0F + 0.5F);
  }


  hwcGammaTable(bInfo, 256, grRamp, ggRamp, gbRamp); 

  /*
  **  Now that we have a gamma table, we can give it to the driver via
  **  a call to ExtEscape() when that is defined.....
  */

  return FXFALSE;

#undef FN_NAME
} /* hwcGammaRGB */


#define M 1
#define K 1

FxBool
hwcSetGrxClock(hwcBoardInfo *bInfo, FxU32 speedInMHz)
{
#define FN_NAME "hwcSetGrxClock"
  FxU32
    pllCtrl1,
    dramInit0 = 0xc17ae29, 
    dramInit1 = 0x26c031,
    n,
    m = 1;

  n = (FxU32) ((speedInMHz - 4.76f)/2.38f);

  pllCtrl1 =
    (K << SST_PLL_K_SHIFT) | (m << SST_PLL_M_SHIFT) | (n << SST_PLL_N_SHIFT);

  GDBG_INFO(80, "%s:  Setting Graphics Clock to %d\n", FN_NAME, speedInMHz);

  HWC_IO_STORE( bInfo->regInfo, dramInit0, dramInit0);
  HWC_IO_STORE( bInfo->regInfo, dramInit1, dramInit1);
  HWC_IO_STORE( bInfo->regInfo, pllCtrl1, pllCtrl1);
  
  return FXTRUE;

#undef FN_NAME  
} /* hwcSetGrxClock */

FxBool
hwcSetMemClock(hwcBoardInfo *bInfo, FxU32 speedInMHz)
{
#define FN_NAME "hwcSetMemClock"

  return FXFALSE;
#undef FN_NAME  
} /* hwcSetMemClock */



FxBool
hwcResolutionSupported(hwcBoardInfo *bInfo, GrScreenResolution_t res)
{
#define FN_NAME "hwcResolutionSupported"
#if GDBG_INFO_ON
  static char *resNames[] = {
    "GR_RESOLUTION_320x200",
    "GR_RESOLUTION_320x240",
    "GR_RESOLUTION_400x256",
    "GR_RESOLUTION_512x384",
    "GR_RESOLUTION_640x200",
    "GR_RESOLUTION_640x350",
    "GR_RESOLUTION_640x400",
    "GR_RESOLUTION_640x480",
    "GR_RESOLUTION_800x600",
    "GR_RESOLUTION_960x720",
    "GR_RESOLUTION_856x480",
    "GR_RESOLUTION_512x256",
    "GR_RESOLUTION_1024x768",
    "GR_RESOLUTION_1280x1024",
    "GR_RESOLUTION_1600x1200",
    "GR_RESOLUTION_400x300",
    "GR_RESOLUTION_1152x864",
    "GR_RESOLUTION_1280x960",
    "GR_RESOLUTION_1600x1024",
    "GR_RESOLUTION_1792x1344",
    "GR_RESOLUTION_1856x1392",
    "GR_RESOLUTION_1920x1440",
    "GR_RESOLUTION_2048x1536",
    "GR_RESOLUTION_2048x2048"
  };
#endif
#if 0
  struct WidthHeight_s {
    FxU32 width; 
    FxU32 height;
  } whByRes[] = {
    {320, 200},                 /* GR_RESOLUTION_320x200 */
    {320, 240},                 /* GR_RESOLUTION_320x240 */
    {400, 256},                 /* GR_RESOLUTION_400x256 */
    {512, 384},                 /* GR_RESOLUTION_512x384 */
    {640, 200},                 /* GR_RESOLUTION_640x200 */
    {640, 350},                 /* GR_RESOLUTION_640x350 */
    {640, 400},                 /* GR_RESOLUTION_640x400 */
    {640, 480},                 /* GR_RESOLUTION_640x480 */
    {800, 600},                 /* GR_RESOLUTION_800x600 */
    {960, 720},                 /* GR_RESOLUTION_960x720 */
    {856, 480},                 /* GR_RESOLUTION_856x480 */
    {512, 256},                 /* GR_RESOLUTION_512x256 */
    {1024, 768},                /* GR_RESOLUTION_1024x768 */
    {1280, 1024},               /* GR_RESOLUTION_1280x1024 */
    {1600, 1200},               /* GR_RESOLUTION_1600x1200 */
    {400, 300},                 /* GR_RESOLUTION_400x300 */
    {1152, 864},                /* GR_RESOLUTION_1152x864 */
    {1280, 960},                /* GR_RESOLUTION_1280x960 */
    {1600, 1024},               /* GR_RESOLUTION_1600x1024 */
    {1792, 1344},               /* GR_RESOLUTION_1792x1344 */
    {1856, 1392},               /* GR_RESOLUTION_1856x1392 */
    {1920, 1440},               /* GR_RESOLUTION_1920x1440 */
    {2048, 1536},               /* GR_RESOLUTION_2048x1536 */
    {2048, 2048}                /* GR_RESOLUTION_2048x2048 */
  };
#endif

#if GDBG_INFO_ON
  GDBG_INFO(80, FN_NAME ":  res == %s (0x%x), supported == %s\n",
            resNames[res], resolutionSupported[bInfo->boardNum][res],
            resolutionSupported[bInfo->boardNum][res] ? "FXTRUE" : "FXFALSE");
#endif
  
  /* Glide has very good checking to see if the memory required is
  available, so we'll just return whether the driver can do it. */
  return resolutionSupported[bInfo->boardNum][res];

#undef FN_NAME
} /* hwcResolutionSupported */

#ifdef __WIN32__
static char *
getRegPath() 
{
  char *retVal = NULL;
  OSVERSIONINFO ovi;
  
  ovi.dwOSVersionInfoSize = sizeof ( ovi );
  GetVersionEx ( &ovi );
  if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
    GDBG_INFO(80, "OS == WNT\n");
    /* It is hardcoded on NT via Display Control code. see:
     * $/devel/swtools/bansheecp2 */
    retVal = "SYSTEM\\CurrentControlSet\\Services\\3Dfx\\Device0\\glide";
  } else {
    QDEVNODE QDevNode;
    QIN Qin;
    int status;

    GDBG_INFO(80, "OS == W9X\n");

    Qin.dwSubFunc = QUERYDEVNODE;
    {
      HDC curDC = GetDC(NULL);

      status = ExtEscape ( curDC, QUERYESCMODE, 
                           sizeof(Qin), (LPCSTR)&Qin, 
                           sizeof(QDevNode), (LPSTR)&QDevNode );
      ReleaseDC(NULL, curDC);
    }

    if ( status > 0 ) {
      static char regPath[255];

      CM_Get_DevNode_Key( QDevNode.dwDevNode, NULL, 
                          &regPath, sizeof(regPath), 
                          CM_REGISTRY_SOFTWARE );
      strcat(regPath, "\\glide");

      retVal = regPath;
    }
  }

  return retVal;
} /* getRegPath */
#endif


char *
hwcGetenv(char *a) 
{
#if __WIN32__
  char *retVal = NULL;
  static char *regPath;
  HKEY hKey;
  DWORD type, szData;
  static char strval[255];

  /* This should work for both NT and Win95/98 (getRegPath works) */
  if ((retVal = getenv(a)) != NULL)
    return retVal;
  
  szData = sizeof(strval);

  if (regPath == NULL) {
    regPath = getRegPath();
    
    GDBG_INFO(80, "_grGetEnv:  regPath = %s\n", regPath);

    if (regPath == NULL)
      return NULL;
  } 
  
  if (RegOpenKey(HKEY_CURRENT_USER, regPath, &hKey) == ERROR_SUCCESS) {
    if (RegQueryValueEx(hKey, a, NULL, &type, strval, &szData) ==
        ERROR_SUCCESS) {
      if (type != REG_SZ) {
        retVal = NULL;
      } else {
        retVal = strval;
      }
    }
    RegCloseKey(hKey);
  }
  
  if ((retVal == NULL) && RegOpenKey(HKEY_LOCAL_MACHINE, regPath, &hKey) == ERROR_SUCCESS) {
    
    if (RegQueryValueEx(hKey, a, NULL, &type, strval, &szData) ==
        ERROR_SUCCESS) {
      if (type != REG_SZ) {
        retVal = NULL;
      } else {
        retVal = strval;
      }
    }
    RegCloseKey(hKey);
  }
  return (char*)retVal;
#else
  return getenv(a);
#endif
} /* _grGetenv */
