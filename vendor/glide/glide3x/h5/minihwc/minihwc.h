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
** $Header: minihwc.h, 22, 5/9/2000 12:58:24 PM, Kenneth Dyke
** $Log:
**  22   3dfx      1.20.1.0    05/09/00 Kenneth Dyke    Added code to calculat
**       chip values on Napalm
**  21   3dfx      1.20        04/10/00 Kenneth Dyke    Added code to tak
**       full-precision screenshots
**  20   3dfx      1.19        03/27/00 Kenneth Dyke    DOS Glide support fo
**       two-chip (and maybe four-chip) Napalm boards
**  19   3dfx      1.18        03/14/00 Adam Briggs     let glide decide when t
**       use analog sl
**  18   3dfx      1.17        03/08/00 Kenneth Dyke    Keep track of whether o
**       not a board is "mapped" or not
**  17   3dfx      1.16        03/07/00 Don Mullis      Generalize glide3 to handl
**       sub-byte-size texels
**
**  16   3dfx      1.15        03/03/00 Kenneth Dyke    Make sure code doesn't bar
**       for non-Napalm build
**  15   3dfx      1.14        03/03/00 Kenneth Dyke    Fixed physical -> LF
**       address calculation once and for all (I hope).  Added code to support SL
**       read abort workaround
**  14   3dfx      1.13        03/01/00 Kenneth Dyke    A bunch of fixes for AA LF
**       accesses (and 32-bit LFB accesses)
**
**  13   3dfx      1.12        02/14/00 Kenneth Dyke    Make sure that we calculat
**       the right number of tiles we need when in SLI mode.  Get SLI band heigh
**       info from Glide rather than checking env var directly
**  12   3dfx      1.11        02/07/00 Kenneth Dyke    MacOS & PC fixes fo
**       multi-chip anti-aliasing
**  11   3dfx      1.10        01/31/00 Adam Briggs     changed the IS_NAPALM macr
**       to cooperate with the display driver version of the sam
**  10   3dfx      1.9         01/31/00 Adam Briggs     Changed all device ID magi
**       numbers to use those defined in fxhal.h & added IS_NAPALM macro to tes
**       against device ID rang
**  9    3dfx      1.8         01/28/00 Kenneth Dyke    Update MacOS minihwc cod
**       for new HRM protocol
**
**  8    3dfx      1.7         01/21/00 Adam Briggs     get the correct linea
**       mappings of slave reg
**  7    3dfx      1.6         01/19/00 Adam Briggs     Made minihwc use eithe
**       EXT_HWC or EXT_HWC_OLD depending on which one the display driver support
**       so that a new glide can try to run on an old display driver
**  6    3dfx      1.5         01/18/00 Kenneth Dyke    Added specific offsets fo
**       primary and secondard display buffers
**  5    3dfx      1.4         01/16/00 Kenneth Dyke    Include pixel format i
**       overlay setup code
**  4    3dfx      1.3         01/03/00 Adam Briggs     added hwcGetGammaTabl
**       function to allow OpenGL to later restore the original Gamma ramp
**  3    3dfx      1.2         11/22/99 Kenneth Dyke    Added hwcIdleWinFifo()
**  2    3dfx      1.1         11/09/99 Adam Briggs     Added support for gettin
**       slave chip MMIO pointers from hwcMapBoard and hwcInitRegister
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator
** 
** 
** 52    8/31/99 5:10p Atai
** program cfgAALfbCtrl
** 
** 51    8/17/99 8:59p Sbrooks
** Added support for HWCEXT2_ glide/driver protocol.
** 
** 50    8/16/99 11:17a Adamb
** Merged in V3_OEM_100 fixes
** 
** 49    8/11/99 3:45p Atai
** added multiple chip support and pci read/write interface
** 
** 48    7/19/99 2:51p Atai
** added variable for sli
** 
** 47    6/21/99 1:24p Atai
** added h3pixelSize in hwcBoardInfo
** 
** 46    6/14/99 4:28p Atai
** more on 2nd buffer allocation
** 
** 45    6/14/99 3:18p Atai
** added secondary buffer info
** 
** 44    6/03/99 4:14p Kcd
** Not sure why this ever compiled on the Mac.
** 
** 43    4/22/99 3:40p Dow
** Alt tab on NT
** 
** 42    4/15/99 5:31p Dow
** Alt-Tab on NT
** 
** 41    4/05/99 8:24p Dow
** Alt tab happiness--sorta
** 
** 40    4/04/99 7:16p Atai
** added hwcShareContextData
** 
** 39    3/31/99 9:01p Dow
** context check w/ driver
** 
** 38    3/17/99 4:09p Dow
** It's my sandbox
** 
** 37    3/10/99 11:52a Peter
** initFifo enable hole counting parameter
** 
** 36    3/09/99 12:20p Kcd
** Made ioPort variable 32-bit for PPC.
** 
** 35    3/05/99 10:06p Peter
** allocate independent state buffers in parallel w/ the command buffers
** 
** 34    2/02/99 4:37p Peter
** hwGetSurfaceinfo gets depth
** 
** 33    1/04/99 11:58a Peter
** added windowed context support
** 
** 32    12/23/98 12:38p Peter
** hdc cleanup
** 
** 31    12/03/98 10:27p Dow
** Added stuff for hwcGetenv
** 
** 30    12/02/98 9:35p Dow
** Query Resolutions
** 
** 29    11/30/98 6:53p Peter
** video memory fifo's
** 
** 28    11/18/98 8:03p Dow
** grxclk
** 
** 27    11/10/98 6:29p Atai
** added min_tramSize for board information. min_tramSize = 0x200000 if
** device id is 3 or mem size is 4
** 
** 26    10/30/98 3:44p Dow
** Fixed Tiled/Linear color/aux bug
** 
** 25    10/21/98 11:22a Peter
** dos happiness w/ the new chris pci library (nee tarolli)
** 
** 24    10/14/98 3:37p Dow
** Gamma stuff
** 
** 23    10/08/98 10:15a Dow
** Triple buffering fix
** 
** 22    9/17/98 3:58p Dow
** Vidmode Stuff
** 
** 21    8/02/98 5:00p Dow
** Glide Surface Extension
** 
** 20    7/29/98 3:09p Dow
** SDRAM Fixes
** 
** 19    7/24/98 2:02p Dow
** AGP Stuff
** 
** 18    7/23/98 1:18a Dow
** Bump & Grind
** 
** 17    7/18/98 12:21a Jdt
** Added state buffer
** 
** 16    7/16/98 10:26p Dow
** GIW Stuf
** 
** 15    7/16/98 3:05p Dow
** Removed useless colBufferAddr from surface info
** 
** 14    7/15/98 4:09p Dow
** GIW Stuff & DOS Protection
** 
** 13    7/13/98 10:35p Jdt
** Added hwcWinFifo, hwcAllocWinFifo, hwcexecuteWinFifo
** 
** 12    7/02/98 12:11p Dow
** LFB fixes
** 
** 11    6/16/98 6:11p Dow
** Rearranged texture memory
** 
** 10    6/12/98 9:22a Peter
** lfb read for triple buffering
** 
** 9     6/11/98 7:44p Jdt
** Win98/NT5 Multimon 1st pass
** 
** 8     6/10/98 9:49a Peter
** lfb buffer addressing
** 
** 7     5/08/98 10:58a Dow
** Volatile declarations
** 
** 6     4/16/98 10:14p Dow
** EXT_HWC is default init method
** 
** 5     4/07/98 10:40p Dow
** LFB Fixes
** 
** 4     3/28/98 10:51a Dow
** Fixes for FIFO bug
** 
** 3     3/20/98 1:11p Dow
** Now checking revision of chip
** 
** 2     3/11/98 8:27p Dow
** WinGlide
** 
** 1     3/04/98 4:13p Dow
**
*/
#ifndef MINIHWC_H
#define MINIHWC_H

#ifdef HWC_EXT_INIT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <ddraw.h>
#endif

#include <3dfx.h>
#include <h3.h>
#include <fxvid.h>
#include <cpuid.h>

/* 
**  Constants
*/
#define HWC_MAX_BOARDS          4 /* Max # boards we can handle */
#define HWC_NUM_BASE_ADDR      17 /* Max # base addresses
                                   * 
                                   * 0-1 : Memory mapped registers
                                   *   2 : i/o port
                                   *   3 : rom (bios)
                                   * 4-16: Slave chip memory mapped registers (AJB)
                                   */

#define HWC_TILE_WIDTH_SHIFT    SST_TILE_WIDTH_BITS
#define HWC_TILE_WIDTH          (0x01UL << HWC_TILE_WIDTH_SHIFT)

#define HWC_TILE_HEIGHT_SHIFT   SST_TILE_HEIGHT_BITS
#define HWC_TILE_HEIGHT         (0x01UL << HWC_TILE_HEIGHT_SHIFT)

typedef FxU32   hwcBuffer;
#define HWC_BUFFER_FRONTBUFFER 0x0
#define HWC_BUFFER_BACKBUFFER  0x1
#define HWC_BUFFER_AUXBUFFER   0x2
#define HWC_BUFFER_DEPTHBUFFER 0x3
#define HWC_BUFFER_ALPHABUFFER 0x4
#define HWC_BUFFER_TRIPLEBUFFER 0x5
#define HWC_BUFFER_FIFOBUFFER      0x6
#define HWC_BUFFER_SCREENBUFFER    0x7
#define HWC_BUFFER_TEXTUREBUFFER   0x8
#define HWC_BUFFER_AUXRENDER    0x9
#define HWC_BUFFER_NONE            0xff

typedef FxU32   hwcOriginLocation;
#define HWC_ORIGIN_UPPER_LEFT  0x0
#define HWC_ORIGIN_LOWER_LEFT  0x1

typedef FxI32   hwcColorFormat;
#define HWC_COLORFORMAT_ARGB   0x0
#define HWC_COLORFORMAT_ABGR   0x1
#define HWC_COLORFORMAT_RGBA   0x2
#define HWC_COLORFORMAT_BGRA   0x3

typedef FxU32 hwcSwapType;
#define HWC_SWAP_FLIP 0x0
#define HWC_SWAP_BLT  0x1

typedef FxU32   hwcControl;
#define HWC_CONTROL_ACTIVATE   0x1
#define HWC_CONTROL_DEACTIVATE 0x2
#define HWC_CONTROL_RESIZE     0x3
#define HWC_CONTROL_MOVE       0x4

/*
**  Data Structures
*/
typedef struct hwcPCIInfo_s {
  FxBool
    initialized;
  FxU32
    vendorID,                   /* PCI Vendor ID */
    deviceID,                   /* PCI Device ID */
    devNum,                     /* master/slave */
    isMaster,                   /* master/slave */
    numChips,                   /* PCI Device ID */
    realNumChips,               /* PCI Device ID */
    pciBaseAddr[HWC_NUM_BASE_ADDR];
#if macintosh
   FxU32
    swizzleOffset[4];           /* Byte swizzle offsets */
   FxBool
   	is66MHz;					/* TRUE if the board is in a 66MHz PCI slot */
#endif    
} hwcPCIInfo;  

typedef struct hwcLinearInfo_s {
  FxBool
    initialized;
  unsigned long
    linearAddress[HWC_NUM_BASE_ADDR];
} hwcLinearInfo;

typedef struct hwcRegInfo_s {
  FxBool
    initialized;
  volatile unsigned long
    ioMemBase,                  /* mem base for I/O aliases */
    cmdAGPBase,                 /* CMD/AGP register base */
    waxBase,                    /* 2D register base */
    sstBase,                    /* 3D register base */
    slaveIOBase[3],             /* KCD: Slave IO mem base */
    slaveSstBase[3],            /* AJB: Slave 3D register base */
    slaveCmdBase[3],            /* AJB: Slave cmd register base */
    slaveIOPortBase[3],         /* KCD: Slave IO registers (real x86 based I/O) */
    lfbBase,                    /* 3D lfb base */
    rawLfbBase;                 /* Raw LFB base (base address 1) */
#if __POWERPC__
  FxU32 ioPortBase;                   /* PPC does I/O via a 32-bit address */
#else       
  volatile FxU16
    ioPortBase,                 /* I/O base address */
    pad;                        /* Keep things aligned */
#endif    
} hwcRegInfo;

typedef struct hwcFifoInfo_s {
  FxBool
    agpFifo,
    initialized;
  unsigned long
    agpVirtAddr;
  FxU32
    agpPhysAddr,
    agpSize,
    fifoStart,                  /* Beg of fifo (offset from base) */
    fifoLength;                 /* Fifo size in bytes */
} hwcFifoInfo;

/*
**  CHD:  Reconcile the following two types
*/

typedef struct hwcBufferDesc_s {
  FxU32        bufMagic;
  hwcBuffer    bufType;
  FxU32        bufOffset;
  FxI32        bufStride;
  FxU32        bufSize;
  FxU32        bufBPP;          /* bits per pixel */
  FxU32        width, height;
  FxBool       tiled;           /* Is it tiled? */
} hwcBufferDesc;

typedef struct hwcBufferInfo_s {
  FxBool
    initialized;
  FxBool
    enable2ndbuffer;
  
  FxU32
    bufSize,                    /* size of buffer in bytes */
    bufSizeInTiles,             /* Buffer Size in tiles */
    bufStride,                  /* stride of buffer in bytes */
    bufStrideInTiles,           /* stride of buffer in tiles */
    bufHeightInTiles,           /* height of buffer in tiles */
    bufLfbStride,               /* LFB stride in bytes */
    nColBuffers,                /* # color buffers */
    colBuffStart0[3],           /* Beg of primary color buffers */
    colBuffEnd0[3],             /* End of primary color buffers */
    lfbBuffAddr0[4],            /* Start address of lfb (tiled relative) buffers.  
                                 * NB: This only includes enough space
                                 * for 3 color buffers and 1 aux buffer.  
                                 */
    lfbBuffAddr0End[4],         /* Used to keep track of end of aux buffer in tiled space for AA stuff. */
    colBuffStart1[3],           /* Beg of secondary color buffers */
    colBuffEnd1[3],             /* End of secondary color buffers */
    lfbBuffAddr1[4],            /* Start address of lfb (tiled relative) buffers.  
                                 * NB: This only includes enough space
                                 * for 3 color buffers and 1 aux buffer.  
                                 */
    nAuxBuffers,                /* # aux buffer (0 or 1) */
    auxBuffStart0,              /* Start of primary aux buffer */
    auxBuffEnd0,                /* End of primary aux buffer */
    auxBuffStart1,              /* Start of secondary aux buffer */
    auxBuffEnd1;                /* End of secondary aux buffer */
  void
    *colBuffPriv,               /* Private color buffer info */
    *auxBuffPriv;               /* Private aux buffer info */
  hwcBufferDesc
    buffers[6];                 /* 3 col + 1 aux + 1 tex + 1 fifo */
} hwcBufferInfo;

typedef struct hwcVidInfo_s {
  FxU32
    hWnd;                       /* Window Handle */
  FxBool
    initialized,
    tiled;                      /* Is it tiled or linear? */
  GrScreenResolution_t
    sRes;
  GrScreenRefresh_t
    vRefresh;
  FxU32
    stride,                     /* Stride in bytes */
    xRes,                       /* X resolution */
    yRes,                       /* Y resolution */
    refresh;                    /* refresh rate */
  FxBool
    tripleBuffering;            /* Are we? */
} hwcVidInfo;

typedef struct hwcSurfaceInfo_s {
  FxU32
    lpSurface,
    fbOffset,
    fbStride,
    bitdepth,
    width,
    height,
    tileBase,
    pciStride,
    lpLFB,
    hwStride;
  FxBool
    sdRam,
    isTiled;
} hwcSurfaceInfo;

typedef struct hwcAGPInfo_s {
  FxU32
    linAddr,                    /* Linear address of AGP memory */
    physAddr,                   /* physical address of AGP memory */
    size;                       /* size of physical memory */
} hwcAGPInfo;


/* Here's the wrapper in which we keep all of the above: */
typedef struct hwcBoardInfo_s {
  FxBool
    sdRAM;                      /* Is the board SDRAM? */
  void  
    *hdc,                       /* This is really an HDC */
    *hMon;                      /* this is an HMONITOR */
  FxU32
    extContextID,               /* HWC_EXT Context ID (if needed) */
    devRev,                     /* Device Revision */
    tramOffset,                 /* Offset to texture memory */
    tramSize,                   /* Size of texure memory */
    min_tramSize,               /* minimum texture memory required */
    fbOffset,                   /* Frame Buffer Offset (in bytes) */
    primaryOffset,              /* Primary buffers offset (in bytes) */
    secondaryOffset,            /* Secondary (AA) buffers offset (in bytes) */
    h3Rev,                      /* Banshee Revision Number */
    h3Mem,                      /* Megs of RAM on board */
    h3pixelSize,                /* pixel size */
    h3pixelSample,              /* pixel sample */
    h3nwaySli,                  /* nway sli, sli = 1 (default), 2, or 4 */
    h3sliBandHeight,            /* Number of scanlines per chip in SLI. */
    h3analogSli,                /* TRUE IFF analog sli is requested */
    boardNum,                   /* Board ID for hwc */
    deviceNum;                  /* Device ID for PCILib */
  hwcPCIInfo
    pciInfo;
  hwcLinearInfo
    linearInfo;
  hwcRegInfo
    regInfo;
  hwcFifoInfo
    fifoInfo;
  hwcBufferInfo
    buffInfo;
  hwcVidInfo
    vidInfo;
  void *
    deviceConfigData;
  hwcAGPInfo
    agpInfo;
  FxU32
    lostContextDWORD;
  FxBool
    isMapped;
  FxU32
    procHandle;
  FxU32
    contextHandle;  
  FxI32 hwcProtocol;
  FxU32 devNode ;   /* AJB- DevNode from display driver for minivdd ioctls */
  FxI16/*FxI32*/ hwcEscape ; /* AJB- sucky: H5 TOT uses a diff't escape code than H3 */
  char devName[32];  /* KoolSmoky - Device Name */
#ifdef HWC_EXT_INIT
  LPDIRECTDRAW  lpDD1;
  LPDIRECTDRAW2 lpDD;
#endif /* HWC_EXT_INIT */
} hwcBoardInfo;


#ifndef IS_NAPALM
#define IS_NAPALM(deviceID) \
       ((deviceID >= SST_DEVICE_ID_L_AP) && \
        (deviceID <= SST_DEVICE_ID_H_AP))
#endif /* ndef IS_NAPALM */



/* Now, let's tie it all together */
typedef struct hwcInfo_s {
  FxU32
    nBoards;
  hwcBoardInfo
    boardInfo[HWC_MAX_BOARDS];
} hwcInfo;

#define HWC_WIN_FIFO_HOST  0x00UL
#define HWC_WIN_FIFO_LOCAL 0x01UL
#define HWC_WIN_FIFO_AGP   0x02UL

typedef struct {
  /* One of the HWCEXT_FIFO_XXX values defined in hwcext.h.  */
  FxU32
    fifoType;

  /* If > 0 xxxBufferAddr is a valid fifo write */
  FxU32
    lockCount;

  /* Information about the current command stream to the hw fifo. 
   * Fields prefaced w/ cmd are for storing the current command stream
   * since the last syncronization point via hwcExecuteFifo. Fields
   * prefaced w/ state are the command stream to restore the state
   * at the last call to hwcExecuteFifo.
   *
   * xxxBufferAddr:      Linear address if lockedP == FXTRUE.
   * xxxBufferOffset:    HW relative address of the buffer.
   * xxxBufferSize:      Total size of valid range of the buffer. (bytes)
   * xxxBufferAllocUnit: Minimum allocation unit for the buffer sub-allocation. (bytes)
   */
  struct {
    FxU32
      baseAddr,
      hwOffset,
      size,
      allocUnit;
  } cmdBuf,
    stateBuf;

  FxU32
    sentinalBufferAddr,
    sentinalBufferOffset;

  /* System dependent bookkepping state information if fifoType !=
   * HWCEXT_FIFO_HOST. This should be considered logically const to
   * the client.
   */
  FxU32
    *surfaceFifo,
    *surfaceSentinal;
} HwcWinFifo;

/*
**  Function Prototypes
*/

hwcInfo *
hwcInit(FxU32 vID, FxU32 dID);

FxBool
hwcMapBoard(hwcBoardInfo *bInfo, FxU32 bAddrMask);

FxBool
hwcInitRegisters(hwcBoardInfo *bInfo);

FxBool
hwcAllocBuffers(hwcBoardInfo *bInfo, FxU32 nColBuffers, FxU32 nAuxBuffers); 

FxBool
hwcInitFifo(hwcBoardInfo *bInfo, FxBool enableHoleCounting);

/* The enableHoleCounting parameter here is a bit of a red herring since
 * having an agp fifo implies no hole counting. This routine will call
 * the default hwcInitFifo when it cannot allocate an agp fifo, and the
 * hole counting parameter only applies in this case.
 */
FxU32 
hwcInitAGPFifo(hwcBoardInfo *bInfo, FxBool enableHoleCounting);

FxBool
hwcInitVideo(hwcBoardInfo *bInfo, FxBool tiled, FxVideoTimingInfo
             *vidTiming, FxU32 pixFmt, FxBool overlay);

FxBool
hwcRestoreVideo(hwcBoardInfo *bInfo);

void
hwcResetVideo(hwcBoardInfo *bInfo);

const char *
hwcGetErrorString(void);

FxBool
hwcCheckMemSize(hwcBoardInfo *bInfo, FxU32 xres, FxU32 yres, FxU32 nColBuffers,
                FxU32 nAuxBuffers, FxBool tiled);

#ifdef __WIN32__
FxU32
hwcAllocWinContext(hwcBoardInfo* bInfo);

FxBool
hwcFreeWinContext(hwcBoardInfo* bInfo,
                  FxU32         winContextId);

/* surfaceInfo: 
 *   Describes the currently setup os surfaces.
 * fifo:
 *   Input:
 *     cmdBuf.allocUnit: 
 *     stateBuf.allocUnit: 
 *       Minimum allocation unit of each section.
 *   Output:
 *     cmdBuf:
 *     stateBuf:
 */
FxBool 
hwcAllocWinFifo(hwcBoardInfo* bInfo, 
                HwcWinFifo*   fifo,
                FxU32*        surface);

FxBool
hwcFreeWinFifo(hwcBoardInfo* bInfo,
               HwcWinFifo*   fifo);

FxBool 
hwcExecuteWinFifo(hwcBoardInfo*     bInfo, 
                  const FxU32       winContextId,
                  const HwcWinFifo* fifo,
                  const FxU32       serialNumber);

FxU32
hwcExecuteStatusWinFifo(hwcBoardInfo*     bInfo,
                        const HwcWinFifo* fifo,
                        const FxU32       serialNumber);

void
hwcIdleWinFifo(hwcBoardInfo*     bInfo,
               const HwcWinFifo* fifo,
               const FxU32       serialNumber);

FxBool
hwcLockWinFifo(hwcBoardInfo* bInfo,
               HwcWinFifo* fifo);

FxBool
hwcUnlockWinFifo(hwcBoardInfo* binfo,
                 HwcWinFifo* fifo);

FxBool
hwcGetSurfaceInfo(const hwcBoardInfo* binfo,
                  FxU32 *sfc,
                  hwcSurfaceInfo *ret);

FxBool
hwcAllocAuxRenderingBuffer(hwcBoardInfo *bInfo, 
                           hwcBufferDesc *bp, 
                           int width, int height);

#endif /* __WIN32__ */

FxBool
hwcGammaRGB(hwcBoardInfo *bInfo, float r, float g, float b);

FxBool
hwcGammaTable(hwcBoardInfo *bInfo, FxU32 nEntries, FxU32 *r, FxU32 *g, FxU32 *b);

FxBool
hwcGetGammaTable(hwcBoardInfo *bInfo, FxU32 nEntries, FxU32 *r, FxU32 *g, FxU32 *b);

FxBool
hwcSetGrxClock(hwcBoardInfo *bInfo, FxU32 speedInMHz);

FxBool
hwcSetMemClock(hwcBoardInfo *bInfo, FxU32 speedInMHz);

FxBool
hwcResolutionSupported(hwcBoardInfo *bInfo, GrScreenResolution_t res, GrScreenRefresh_t ref);

char *
hwcGetenv(const char *a);

char *
hwcGetenvEx(const char *a, char *b);

FxU32
hwcQueryContext(hwcBoardInfo *bInfo);

FxU32
hwcShareContextData(hwcBoardInfo *bInfo, FxU32 **data);

void
hwcUnmapMemory();

void
hwcUnmapMemory9x(hwcBoardInfo *bInfo);

void
hwcClearContextData();

void
hwcCheckTarget(FxU32 sfcOffset, FxU32 sfcWidth, FxU32 sfcHeight,
               FxU32 sfcDepth, FxU32 auxFifoStart, FxU32 auxFifoSize);

#ifdef FX_GLIDE_NAPALM
extern FxU32 hwcReadConfigRegister(hwcBoardInfo *bInfo, FxU32 chipNumber, FxU32 offset);
extern void hwcWriteConfigRegister(hwcBoardInfo *bInfo, FxU32 chipNumber, FxU32 offset, FxU32 value);

extern void hwcSLIReadEnable(hwcBoardInfo *bInfo);
extern void hwcSLIReadDisable(hwcBoardInfo *binfo);

extern void hwcSetSLIAAMode(hwcBoardInfo *bInfo,
                     FxU32 sliEnable,
                     FxU32 aaEnable,
                     FxU32 analogSLI,
                     FxU32 sliBandHeight,
                     FxU32 totalMem,
                     FxU32 numChips,
                     FxU32 aaSampleHigh,
                     FxU32 aaColorBuffStart,
                     FxU32 aaDepthBuffStart,
                     FxU32 aaDepthBuffEnd,
                     FxU32 bpp);

extern void hwcAAScreenShot(hwcBoardInfo *bInfo, FxU32 colBufNum, FxBool dither);

extern void hwcAAReadRegion(hwcBoardInfo *bInfo, FxU32 colBufNum,
                            FxU32 src_x, FxU32 src_y, 
                            FxU32 src_width, FxU32 src_height, 
                            FxU32 dst_stride, void *dst_data,
                            FxU32 bpp, FxBool dither);

#endif

void hwcCalcSipValue(hwcBoardInfo *bInfo, FxU32 chipNum, FxU32 *nandChain, FxU32 *norChain);

#if GL_X86
void hwcSetCPUInfo (_p_info *cpuInfo);
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
FxBool hwcIsOSWin9x(void);
#endif

#endif                          /* MINIHWC_H not defined */
