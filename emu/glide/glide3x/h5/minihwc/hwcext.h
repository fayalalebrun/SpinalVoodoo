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
*
/
**
** File name:   hwcext.h
**
** Description: Structures for the hwc extensions.
**
** $Header: hwcext.h, 9, 6/19/2000 7:55:50 PM, Adam Briggs
**
** $History: hwcext.h $
** 
** *****************  Version 38  *****************
** User: Doconnell    Date: 9/01/99    Time: 11:16a
** Updated in $/devel/h5/WinNT/Src/Video/Displays/h5
** Move mechanism for checking for O/S support of KNI instructions from
** V3_OEM_100, but change escape number so that it is not in Microsoft
** reserved range.  Also move a few declarations in preparation for moving
** Glide and TvOut changes from V3_OEM_100.
** 
** *****************  Version 37  *****************
** User: Adamb        Date: 8/18/99    Time: 10:14a
** Updated in $/devel/h5/minihwc
** changed glideState[]; extern back to glideState;
** 
** *****************  Version 36  *****************
** User: Sbrooks      Date: 8/17/99    Time: 8:59p
** Updated in $/devel/h3/minihwc
** Added support for HWCEXT2_ glide/driver protocol.
** 
** *****************  Version 34  *****************
** User: Andrew       Date: 8/06/99    Time: 6:22p
** Updated in $/devel/h5/Win9x/dx/dd16
** Added a function to get the slave registers
** 
** *****************  Version 33  *****************
** User: Andrew       Date: 8/06/99    Time: 4:40p
** Updated in $/devel/h5/Win9x/dx/dd16
** Added new members to structures and new Glide PCI Functions
** 
** *****************  Version 32  *****************
** User: Jw           Date: 8/02/99    Time: 12:49p
** Updated in $/devel/h5/WinNT/Src/Video/Displays/h5
** Support both old and new ext_hwc values for Glide.
** 
** *****************  Version 31  *****************
** User: Sbrooks      Date: 6/14/99    Time: 12:35p
** Updated in $/devel/h3/minihwc
** First draft of Version 2 of the HWCEXT (ExtEscape) functions required
** by glide
** 
** *****************  Version 30  *****************
** User: Stb_sbrooks  Date: 6/07/99    Time: 4:08p
** Updated in $/devel/h3/minihwc
** Changed EXT_HWC. Previous value was illegal.
** 
** *****************  Version 29  *****************
** User: Stb_lpost    Date: 6/07/99    Time: 12:00p
** Updated in $/devel/h3/win95/dx/dd16
** V3TV Fixes Initial PLD656 Support
** 
** *****************  Version 28  *****************
** User: Stb_lpost    Date: 5/17/99    Time: 1:43p
** Updated in $/devel/h3/win95/dx/dd16
** V3TV Video Capture fixes for E3 Demo
** 
** *****************  Version 27  *****************
** User: Dow          Date: 4/22/99    Time: 3:40p
** Updated in $/devel/h3/minihwc
** Alt tab on NT
** 
** *****************  Version 26  *****************
** User: Dow          Date: 4/15/99    Time: 5:31p
** Updated in $/devel/h3/minihwc
** Alt-Tab on NT
** 
** *****************  Version 25  *****************
** User: Dow          Date: 4/05/99    Time: 3:48p
** Updated in $/devel/H3/minihwc
** Added hwcExtContextRes_t
** 
** *****************  Version 24  *****************
** User: Sreid        Date: 4/04/99    Time: 4:11p
** Updated in $/devel/h3/minihwc
** Added mechanism to coordinate loss of context between app and DD
** 
** *****************  Version 23  *****************
** User: Xingc        Date: 3/22/99    Time: 2:41p
** Updated in $/devel/h3/Win95/dx/dd16
** Add HWCEXT_OVERLAY_DATA case and hwcExtOVLInforRest_t structure to pass
** overlay informat out.
** 
** *****************  Version 22  *****************
** User: Michael      Date: 2/26/99    Time: 10:21a
** Updated in $/devel/h3/Win95/dx/dd16
** Proxy for KenW - These changes make alt-tab from apps *much*
** more reliable.  I now see no corruption or hangs from glquake or
** heretic2.   NBA98 also worked.  PRS 3320 is possibly associated.  There
** may be others.
** 
** *****************  Version 21  *****************
** User: Stuartb      Date: 1/08/99    Time: 11:05a
** Updated in $/devel/h3/Win95/dx/dd16
** Backed out LCDCTRL changes as advised by KMW.
** 
** *****************  Version 20  *****************
** User: Stuartb      Date: 1/07/99    Time: 12:18p
** Updated in $/devel/h3/Win95/dx/dd16
** Added HWCEXT_LCDCTRL for control panel flat panel ops.
** 
** *****************  Version 19  *****************
** User: Michael      Date: 12/29/98   Time: 1:19p
** Updated in $/devel/h3/Win95/dx/dd16
** Implement the 3Dfx/STB unified header.
**
*/
#ifndef HWCEXT_H
#define HWCEXT_H

#ifndef WINNT

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/*
** D3D notification function
*/

extern void D3notify(void);

#endif /* WINNT */

/*
**  Constants
*/
#define HWCEXT_PROTOCOLREV              0x1

#define HWCEXT_MAX_BASEADDR             0x9
#define HWCEXT_MAX_COLORBUFFERS         0x3

/* The HWC escape */
#define EXT_HWC                         0x3df3
#define EXT_HWC_OLD                     0xfd3
/* KoolSmoky - Microsoft reserves the range 0 to 0x10000 for its escape codes.
 * Third-party vendors are free to choose escape codes for their own use above
 * this range. previous values were illegal and disabled glide from functioning
 * under winxp.
 */
#define EXT_HWC_WXP                     0x13df3

/*
**  EXT_HWC_SHARE_CPUTYPE
**
**  Share CPU Type with the calling process
**  
*/
#define EXT_HWC_SHARE_CPUTYPE        0x3df4
#define EXT_HWC_SHARE_CPUTYPE_WXP    0x13df4

/********************************************************************
**  Here's the protocol:
*********************************************************************/

/*
**  HWCEXT_GETDRIVERVERION
**
**  INPUT:      Nothing
**  OUTPUT:     Driver Major & Minor Version
*/
#define HWCEXT_GETDRIVERVERSION        0x0
typedef struct hwcExtDriverVersionRes_s {
  FxU32
    major,                      /* Driver major version */
    minor;                      /* Driver minor version */
} hwcExtDriverVersionRes_t;

/* 
**  HWCEXT_ALLOCCONTEXT
**
**  INPUT:      Nothing
**  OUTPUT:     Context ID
*/
#define HWCEXT_ALLOCCONTEXT             0x1


#define HWCEXT_ABAPPTYPE_FSEM           0x0 /* Full-screen app */
#define HWCEXT_ABAPPTYPE_WIND           0x1 /* Windowed app */

/* Given to HWCEXT_ALLOCCONTEXT */
typedef struct hwcExtAllocContextReq_s {
  FxU32
    protocolRev,                /* Revision of this protocol */
    appType;                    /* FSEM or WIND */
} hwcExtAllocContextReq_t;

/* Returned from  HWCEXT_ALLOCCONTEXT*/
typedef struct hwcExtAllocContextRes_s {
  FxU32
    contextID;
} hwcExtAllocContextRes_t;

/* 
**  HWCEXT_GETDEVICECONFIG
**  
**  INPUT:      Device number (0 for first, 1 for second....)
**  OUTPUT:     VendorID, deviceID, fbRAM, chip rev
*/
#define HWCEXT_GETDEVICECONFIG          0x2
/* Given to HWCEXT_GETDEVICECONFIG */
typedef struct hwcExtDeviceConfigReq_s {
  HDC
    dc;                         /* Device context */
  FxU32
    devNo;                      /* Device number (0, 1, ...) */
} hwcExtDeviceConfigReq_t;

/* Returned from HWCEXT_GETDEVICECONFIG */
typedef struct hwcExtDeviceConfigRes_s {
  FxU32
    devNum,                     /* Device Number */
    vendorID,                   /* PCI Vendor ID */
    deviceID,                   /* PCI Device ID */
    fbRam,                      /* How much frame buuffer RAM  */
    chipRev;                    /* Chip Revision */
  FxU32 
    pciStride,                  /* 1024, 2048, 4096, 8192, 16384 */
    hwStride,                   /* hardware x-translation stride  */
    tileMark;                   /* hardware tile addressing watermark */
  FxU32 isMaster;               /* True if device is a Master */
  FxU32 numChips;               /* Number of Chips 1-4 */
} hwcExtDeviceConfigRes_t;

/* 
**  HWCEXT_GETLINEARADDR
**
**  INPUT:      Device number
**  OUTPUT:     Number of base addresses, base address list
*/
#define HWCEXT_GETLINEARADDR            0x3
/* Given to HWCEXT_GETLINEARADDR */
typedef struct hwcExtLinearAddrReq_s {
  FxU32 devNum;                 /* Device Number */
  FxU32 pHandle;                /* Process Handle */
} hwcExtLinearAddrReq_t;

/* Returned from HWCEXT_GETLINEARADDR */
typedef struct hwcExtLinearAddrRes_s {
  FxU32 numBaseAddrs;               /* # base addresses */
  unsigned long baseAddresses[HWCEXT_MAX_BASEADDR]; /* linear Addresses  */
} hwcExtLinearAddrRes_t;

/*
**  HWCEXT_ALLOCFIFO
**  
** OBSOLTE: Do not use
*/
#define HWCEXT_ALLOCFIFO             0x4

typedef struct hwcExtAllocFIFORes_s {
  FxU32
    commandBuffer,              /* Command buffer */
    commandBufferSz,            /* Size of command buffer */
    psBuffer,                   /* Persistent state buffer */
    psBufferSz;                 /* Size of persistent state buffer */
} hwcExtAllocFIFORes_t;

/*  
**  HWCEXT_EXECUTEFIFO
**
**  INPUT:      ptr to FIFO buffer, fifosize, persistent state FIFO & size
**  OUTPUT:     Status only.
**
*/
#define HWCEXT_EXECUTEFIFO      0x5

/* Given to HWCEXT_EXECUTEFIFO */
typedef struct hwcExtExecuteFifoReq_s {
  /* NB: These fields must be first to support old drivers that do not
   * support multiple fifo types.  
   */
  FxU32
    fifoPtr,        /* Linear address of command stream to execute */
    fifoSize,       /* Size of command stream (DWORDS) */
    statePtr,       /* Linear address of persistent state buffer */
    stateSize;      /* Size of persistent state buffer (DWORDS) */
  
  FxU32
    fifoType;       /* One of HWCEXT_FIFO_XXXX */

  /* These are only necessary if fifoType != HWCEXT_FIFO_HOST */
  FxU32
    fifoOffset,     /* HW relative address of command stream */
    stateOffset,    /* HW relative address of state buffer */
    sentinalOffset, /* HW relative address of buffer to write serialNumber */
    serialNumber;   /* Client specified value to write to indicate that the
                       * current command stream has been committed by the hw.
                       */
#ifdef ENABLE_V3_W2K_GLIDE_CHANGES
    FxU32 procID;
#endif
  
} hwcExtExecuteFifoReq_t;

/* 
**  HWCEXT_QUERYCONTEXT
**
**  INPUT:      Context ID
**  OUTPUT:     Status 
*/
#define HWCEXT_QUERYCONTEXT     0x6

#define HWC_CONTEXT_ACTIVE      0x0
#define HWC_CONTEXT_LOST        0x1

/*
**  HWCEXT_RELEASECONTEXT
**  
**  INPUT:      Context ID
**  OUTPUT:     Status
*/
#define HWCEXT_RELEASECONTEXT   0x7

/* Given to HWCEXT_RELEASECONTEXT */
typedef struct hwcExtReleaseContextReq_s {
  FxU32
    contextID;                  /* duh */
#ifdef ENABLE_V3_W2K_GLIDE_CHANGES
  FxU32
    procID;
#endif
} hwcExtReleaseContextReq_t;

#define HWCEXT_HWCSETEXCLUSIVE  0x8
#define HWCEXT_HWCRLSEXCLUSIVE  0x9

/*
**  HWCEXT_I2C_WRITE_REQ
**
**  INPUT: i2c address, register number, dwValue to write
**  OUTPUT: status   (FXTRUE if OK, FXFALSE if fail, I2C_BUSY if busy)
*/

#define HWCEXT_I2C_WRITE_REQ    0xa

typedef struct hwcExtI2CwriteReq_s {
        FxU32 deviceAddress;    /* bus address of device */
        FxU32 i2c_regNum;       /* sometimes called subaddress */
        FxU32 i2c_regValue;     /* the value to write */
} hwcExtI2CwriteReq_t;

/*
**  HWCEXT_I2C_READ_REQ
**
**  INPUT: i2c address, register number
**  OUTPUT: status   (FXTRUE if OK, FXFALSE if fail, I2C_BUSY if busy)
**          value read: optRes.i2c_ReadRes.i2c_regValue
*/

#define HWCEXT_I2C_READ_REQ     0xb

typedef struct hwcExtI2CreadReq_s {
        FxU32 deviceAddress;    /* bus address of device */
        FxU32 i2c_regNum;       /* sometimes called subaddress */
} hwcExtI2CreadReq_t;

#define HWCEXT_I2C_READ_RES     0xc

typedef struct hwcExtI2CreadRes_s {
        FxU32 i2c_regValue;     /* the value read from reg above */
} hwcExtI2CreadRes_t;

/*
**  HWCEXT_I2C_WRITE_REQ
**
**  INPUT: i2c address, register number, dwValue to write
**  OUTPUT: status   (FXTRUE if OK, FXFALSE if fail, I2C_BUSY if busy)
*/

#define HWCEXT_I2C_MULTI_WRITE_REQ    0xd
typedef struct hwcExtI2CmultiWriteReq_s {
        FxU32 deviceAddress;    /* bus address of device */
        FxU32 i2c_regNum;       /* sometimes called subaddress */
        FxU8  *i2c_regValues;   /* the value to write */
        FxU32 i2c_numBytes;     /* number of bytes to xfer */
} hwcExtI2CmultiWriteReq_t;


/*
**  HWCEXT_GETAGPINFO
**
**  Get the relavent AGP info:  linear address, physical address, size.
*/
#define HWCEXT_GETAGPINFO            0xe
typedef struct hwcExtAGPInfoRes_s {
  FxU32
    lAddr,                      /* Linear Address of AGP memory */
    pAddr,                      /* Physical Address of AGP memory */
    size;                       /* Size */
} hwcExtAGPInfoRes_t;

/*
**  HWCEXT_VIDTIMING
**
**  Send the FxVidTiming structure to the driver.
**
*/
#define HWCEXT_VIDTIMING        0xf
typedef struct hwcExtVidTimingReq_s {
  void *vidTiming;
} hwcExtVidTimingReq_t;

/* HWCEXT_FIFOINFO
 *
 * Find out information about the current physcal location of the
 * current command fifo.  
 */
#define HWCEXT_FIFOINFO         0x10UL

#define HWCEXT_FIFO_INVALID      0x00UL
#define HWCEXT_FIFO_FB           0x01UL /* FIFO in frame buffer */
#define HWCEXT_FIFO_HOST         0x02UL /* FIFO in host memory */
#define HWCEXT_FIFO_AGP          0x03UL /* FIFO in AGP */

typedef struct {
  FxU32 fifoType; 
} hwcExtFifoInfoRes_t;

/* Returns the offset that a linear address is from a given 'base'
 * linear page address. This offset is suitable for use as a hw
 * relative address for buffers etc.
 *
 *  mapAddr: Linear address used to be the 'base' of the physical
 *           address also mapped by remapAddr. 
 *  remapAddr: Target linear addr to compute the offset for.
 */
#define HWCEXT_LINEAR_MAP_OFFSET        0x11UL

typedef struct {
  FxU32
    mapAddr,
    remapAddr;
} hwcExtLinearMapInfoReq_t;

typedef struct {
  FxU32 linAddrOffset;
} hwcExtLinearMapInfoRes_t;

/* HWCEXT_GAMMATABLE_DOWNLOAD
 *
 * Downloads a gamma table from app that is then impressed upon
 * the hardware. This is a transient gamma, and is not preserved
 * in the registry or anywhere in the drivers. There is no answer
 * or status return.
 *
 * INPUT: a pointer to a complete gamma table (256 entries)
 *
 * OUTPUT: none
 */
#define HWCEXT_DOWNLOAD_GAMMA                   0x12UL

typedef struct {
  FxU32 gammaPtr;
  FxI32 numGammaEntries;
} hwcExtDownloadGammaReq_t;


/* HWCEXT_RESTORE_DESKTOP
 *
 * Restores the video settings to those of the desktop before app
 * exclusive mode took over.   We need this because there are cases where
 * we do not get a mode set call (Enable1) when task-switching (e.g., ALT-TAB)
 * out of exclusive mode apps.  We do this by calling the same function
 * that is used to restore the desktop when returning from full screen
 * DOS mode.   This resets video, the cmdfifo, the desktop's cursor, sets
 * some overlay video flags saying that the desktop was reset, etc.
 *
 * INPUT: none
 * OUTPUT: none
 */
#define HWCEXT_RESTORE_DESKTOP                  0x13UL


/* HWCEXT_OVERLAY_DATA
 * Pass overlay address and overlay scale 
 * factors to TVPCI. 
 */
#define HWCEXT_OVERLAY_DATA                             0x14UL
typedef struct hwcExtOVLInfoRes_s {
  FxU32   ovlAddr;          /* Physical address of current overlay */
  FxU32   ovlXScale;
  FxU32   ovlYScale;
} hwcExtOVLInfoRes_t;

/*
** HWCEXT_SHARE_CONTEXT_DWORD
** This entry point gives the driver a place to write a 1 if app
** loses it's context.  App then checks this when it's getting its
** command FIFO information, and does the Right Thing if it isn't
** exclusive any more. 
*/
#define HWCEXT_SHARE_CONTEXT_DWORD          0x15UL
typedef struct hwcExtShareContextDWORDReq_s {
  FxU32 contextDWORD;           /* ring 3 pointer THIS IS FOR NT ONLY! */
} hwcExtShareContextDWORDReq_t;

typedef struct hwcExtShareContextDWORDRes_s {
  FxU32 contextDWORD;           /* ring0/ring3 pointer THIS IS FOR 9X ONLY! */
} hwcExtShareContextDWORDRes_t;

/*
**  HWCEXT_UNMAP_MEMORY
** 
**  This entry point tells the driver to unmap the memory that the app
**  has requested. 
*/
#define HWCEXT_UNMAP_MEMORY             0x16UL
typedef struct hwcExtUnmapMemoryReq_s {
  FxU32 procHandle;             /* Process handle */
} hwcExtUnmapMemoryReq_t;

/*
**  HWCEXT_CONTEXT_DWORD_NT
**
**  Similar to other context DWORD routine but subtly different for
**  differnces between NT and 9X.
*/
#define HWCEXT_CONTEXT_DWORD_NT     0x17UL
typedef struct hwcExtContextDwordNTReq_s {
  FxU32 procId;                 /* handle to current proc */
  FxU32 codeSegment;            /* I know it's 16-bit */
  FxU32 dataSegment;            /* I know it's 16-bit */
} hwcExtContextDwordNTReq_t;

typedef struct hwcExtContextDwordNTRes_s {
  FxU32 dwordOffset;            /* Ofsset of context DWORD in FB */
} hwcExtContextDwordNTRes_t;


#define HWCEXT_PCI_OP 0x18UL
#define HWCEXT_PCI_READ 0x00UL
#define HWCEXT_PCI_WRITE 0x01UL

typedef struct hwcExtPCIOpReq_s {
   FxU32 DeviceId;      /* Device PCI Function Number [0-3] */
   FxU32 Operation;      /* PCI Operation  HWCEXT_PCI_READ | HWCEXT_PCI_WRITE */
   FxU32 Offset;        /* PCI Offset [0-255] */
   FxU32 Value;         /* Used only for Writes */
} hwcExtPCIOpReq_t;

typedef struct hwcExtPCIOpRes_s {
   FxU32 Value;         /* Used only for Reads */
} hwcExtPCIOpRes_t;

#define HWCEXT_GET_SLAVE_REGS 0x19UL
#define HWCEXT_MAX_SLAVE_REGS (4)
typedef struct hwcExtSlaveRegReq_s {
   FxU32 DeviceId;      /* Device PCI Function Number [0-3] */
} hwcExtSlaveRegReq_t;

typedef struct hwcExtSlaveRegRes_s {
   FxU32 Regs[HWCEXT_MAX_SLAVE_REGS];
} hwcExtSlaveRegRes_t;

#ifdef ENABLE_V3_W2K_GLIDE_CHANGES
/*
** HWCEXT_PROTECT_CMD_FIFO
** 
** Called once when we start using the command fifo in order to tell
** NT / W2K to hold off on unmapping that view of the frame buffer and
** then called again once Glide knows it's lost its context so that
** the driver knows that it's now ok to unmap the memory.
*/
#define HWCEXT_PROTECT_CMD_FIFO 0x1A
typedef struct hwcExtProtectCmdFifoReq_s {
  FxU32 procId;                 /* handle to current proc */
  FxU32  linearAddress; /* Linear pointer to command fifo */
  FxBool protect;       /* Does it need protection or not */
} hwcExtProtectCmdFifoReq_t;
#endif

/*
** HWCEXT_SLI_AA_REQUEST
*
** I'm sorry this has to be here but I swear that IT'S NOT MY FAULT
*
** This escape should call into the miniport driver on NT and setup
** the master & slaves for the requested SLI or AA mode.  SLI &/or
** AA is disabled with the same call by setting dwaaEn = 0 && 
** dwsliEn = 0 in the request structure.
** 
** Keen observers may note that the following structures bear a
** remarkable resemblance to those structures that are passed
** to the miniport driver by the display driver to execute the
** same function.  They are duplicated here under different names
** to protect this code in the case that those structures are
** modified.
*/
#define HWCEXT_SLI_AA_REQUEST  0x1B

typedef struct _hwc_sli_aa_chipinfo
{
   DWORD dwChips ;         /* dwChips: Number of chips in multi-chip configuration (1-4) */
   DWORD dwsliEn ;         /* dwsliEn: Sli is to be enabled (0,1) */
   DWORD dwaaEn ;          /* dwaaEn: Anti-aliasing is to be enabled (0,1) */
   DWORD dwaaSampleHigh ;  /* dwaaSampleHigh: 0->Enable 2-sample AA, 1->Enable 4-sample AA */
   DWORD dwsliAaAnalog ;   /* dwsliAaAnalog: 0->Enable digital SLI/AA, 1->Enable analog Sli/AA */
   DWORD dwsli_nlines ;    /* dwsli_nLines: Number of lines owned by each chip in SLI (2-128) */
   DWORD dwCfgSwapAlgorithm ; /* Swap Buffer Algorithm */
}  HWCSLI_AA_CHIPINFO ;

typedef struct _hwc_sli_aa_meminfo
{
   DWORD dwTotalMemory ;
   DWORD dwTileMark ;
   DWORD dwTileCmpMark ;
   DWORD dwaaSecondaryColorBufBegin ;
   DWORD dwaaSecondaryDepthBufBegin ;
   DWORD dwaaSecondaryDepthBufEnd ;
   DWORD dwBpp ;
}  HWCSLI_AA_MEMINFO ; 

typedef struct hwcExtSliAAReq_s
{
   HWCSLI_AA_CHIPINFO ChipInfo ;
   HWCSLI_AA_MEMINFO  MemInfo ;
} hwcExtSliAAReq_t ;

/*
**  The Request structure (given as input data to ExtEscape() )
*/
typedef struct hwcExtRequest_s {
  FxU32
    contextID,                  /* ID of this context */
    which;                      /* Which request */
  union reqOptData {
    hwcExtAllocContextReq_t             allocContextReq;
    hwcExtDeviceConfigReq_t             deviceConfigReq;
    hwcExtLinearAddrReq_t               linearAddrReq;
    hwcExtExecuteFifoReq_t              executeFifoReq;
    hwcExtReleaseContextReq_t           releaseContextReq;
    hwcExtI2CwriteReq_t                 i2c_WriteReq;
    hwcExtI2CreadReq_t                  i2c_ReadReq;
    hwcExtI2CmultiWriteReq_t            i2c_MultiWriteReq;
    hwcExtVidTimingReq_t                vidTimingReq;
    hwcExtLinearMapInfoReq_t            mapInfoReq;
    hwcExtDownloadGammaReq_t            gamma;
    hwcExtShareContextDWORDReq_t        shareContextDWORDReq;
    hwcExtUnmapMemoryReq_t              unmapMemoryReq;
    hwcExtContextDwordNTReq_t           contextDwordNTReq;
    hwcExtPCIOpReq_t                    pciOpReq;
    hwcExtSlaveRegReq_t                 slaveRegReq;
#ifdef ENABLE_V3_W2K_GLIDE_CHANGES
    hwcExtProtectCmdFifoReq_t           protectCmdFifo;
#endif
    hwcExtSliAAReq_t                    sliAAReq ;
  } optData;
} hwcExtRequest_t;

typedef struct hwcExtResult_s {
  FxI32
    resStatus;
  union resOptData {
    hwcExtDriverVersionRes_t    driverVersionRes;
    hwcExtAllocContextRes_t     allocContextRes;
    hwcExtDeviceConfigRes_t     deviceConfigRes;    
    hwcExtLinearAddrRes_t       linearAddressRes;
    hwcExtAllocFIFORes_t        allocFifoRes;
    hwcExtI2CreadRes_t          i2c_ReadRes;
    hwcExtAGPInfoRes_t          agpInfoRes;
    hwcExtFifoInfoRes_t         fifoInfoRes;
    hwcExtLinearMapInfoRes_t    mapInfoRes;
    hwcExtOVLInfoRes_t          ovlInfoRes;
    /* no result for gamma download */
    hwcExtShareContextDWORDRes_t  shareContextDWORDRes;
    hwcExtContextDwordNTRes_t   contextDwordNTRes;
    hwcExtPCIOpRes_t                    pciOpRes;
    hwcExtSlaveRegRes_t          slaveRegRes;
  } optData;
} hwcExtResult_t;

#define HWCEXT2_GETPROTOCOLREVISION 0x100UL

typedef struct hwcExt2ProtocolRevisionRes_s {
  FxU32
    major,                      /* Driver major version */
    minor;                      /* Driver minor version */
} hwcExt2ProtocolRevisionRes_t;

#define HWCEXT2_GETDRIVERVERSION	0x101UL

typedef struct hwcExt2DriverVersionRes_s {
  FxU32
    major,                      /* Driver major version */
    minor;                      /* Driver minor version */
} hwcExt2DriverVersionRes_t;

#define HWCEXT2_ALLOCCONTEXT	0x102UL

#define HWCEXT2_ABAPPTYPE_FSEM           0x0 /* Full-screen app */
#define HWCEXT2_ABAPPTYPE_WIND           0x1 /* Windowed app */

typedef struct hwcExt2AllocContextReq_s {
  FxU32
    protocolRev,                /* Revision of this protocol */
    appType;                    /* FSEM or WIND */
} hwcExt2AllocContextReq_t;

typedef struct hwcExt2AllocContextRes_s {
  FxU32
    contextID;
} hwcExt2AllocContextRes_t;

#define HWCEXT2_GETDEVICECONFIG	0x103UL

typedef struct hwcExt2DeviceConfigReq_s {
  HDC
    dc;                         /* Device context */
  FxU32
    devNo;                      /* Device number (0, 1, ...) */
} hwcExt2DeviceConfigReq_t;

typedef struct hwcExt2DeviceConfigRes_s {
  FxU32
    devNum,                     /* Device Number */
    vendorID,                   /* PCI Vendor ID */
    deviceID,                   /* PCI Device ID */
    fbRam,                      /* How much frame buuffer RAM  */
    chipRev;                    /* Chip Revision */
  FxU32 
    pciStride,                  /* 1024, 2048, 4096, 8192, 16384 */
    hwStride,                   /* hardware x-translation stride  */
    tileMark;                   /* hardware tile addressing watermark */
} hwcExt2DeviceConfigRes_t;

#define HWCEXT2_GETLINEARMAP	0x104UL

typedef struct hwcExt2LinearMapReq_s {
  FxU32 devNum;                 /* Device Number */
  FxU32 pHandle;                /* Process Handle */
} hwcExt2LinearMapReq_t;

# define HWCEXT2_MAX_BASEADDR HWCEXT_MAX_BASEADDR

typedef struct hwcExt2LinearMapRes_s {
  FxU32
    numBaseAddrs,               /* # base addresses */
    baseAddresses[HWCEXT2_MAX_BASEADDR]; /* linear Addresses  */
} hwcExt2LinearMapRes_t;

#define HWCEXT2_EXECUTEFIFO 0x105UL

typedef struct hwcExt2ExecuteFifoReq_s {
  /* NB: These fields must be first to support old drivers that do not
   * support multiple fifo types.  
   */
  FxU32
    fifoPtr,        /* Linear address of command stream to execute */
    fifoSize,       /* Size of command stream (DWORDS) */
    statePtr,       /* Linear address of persistent state buffer */
    stateSize;      /* Size of persistent state buffer (DWORDS) */
  
  FxU32
    fifoType;       /* One of HWCEXT2_FIFO_XXXX */

  /* These are only necessary if fifoType != HWCEXT2_FIFO_HOST */
  FxU32
    fifoOffset,     /* HW relative address of command stream */
    stateOffset,    /* HW relative address of state buffer */
    sentinalOffset, /* HW relative address of buffer to write serialNumber */
    serialNumber;   /* Client specified value to write to indicate that the
                       * current command stream has been committed by the hw.
                       */
} hwcExt2ExecuteFifoReq_t;

#define HWCEXT2_RELEASECONTEXT 0x106UL

typedef struct hwcExt2ReleaseContextReq_s {
  FxU32
    contextID;                  /* duh */
} hwcExt2ReleaseContextReq_t;

#define HWCEXT2_HWCSETEXCLUSIVE  0x8
#define HWCEXT2_HWCRLSEXCLUSIVE  0x9

#define HWCEXT2_GETAGPINFO	0x107UL

typedef struct hwcExt2AGPInfoRes_s {
  FxU32
    lAddr,                      /* Linear Address of AGP memory */
    pAddr,                      /* Physical Address of AGP memory */
    size;                       /* Size */
} hwcExt2AGPInfoRes_t;

#define HWCEXT2_GETFIFOINFO	0x108UL

#define HWCEXT2_FIFO_INVALID      0x00UL
#define HWCEXT2_FIFO_FB           0x01UL /* FIFO in frame buffer */
#define HWCEXT2_FIFO_HOST         0x02UL /* FIFO in host memory */
#define HWCEXT2_FIFO_AGP          0x03UL /* FIFO in AGP */

typedef struct {
  FxU32 fifoType; 
} hwcExt2FifoInfoRes_t;

#define HWCEXT2_LINEAR_MAP_OFFSET	0x109UL

typedef struct {
  FxU32
    mapAddr,
    remapAddr;
} hwcExt2LinearMapInfoReq_t;

typedef struct {
  FxU32 linAddrOffset;
} hwcExt2LinearMapInfoRes_t;

#define HWCEXT2_SHARE_CONTEXT_DATA	0x110UL

typedef struct hwcExt2ShareContextDataRes_s {
  FxU32 contextDWORD;           /* ring0/ring3 pointer */
  FxU32 fifoRoom;			 /* ring0/ring3 pionter */
} hwcExt2ShareContextDataRes_t;

#define HWCEXT2_UNMAP_MEMORY	0x111UL

typedef struct hwcExt2UnmapMemoryReq_s {
  FxU32 procHandle;             /* Process handle */
} hwcExt2UnmapMemoryReq_t;

typedef struct hwcExt2Request_s {
  FxU32
    contextID,                  /* ID of this context */
    which;                      /* Which request */
  union reqOptionalData{
    hwcExt2AllocContextReq_t             allocContextReq;
    hwcExt2DeviceConfigReq_t             deviceConfigReq;
    hwcExt2LinearMapReq_t                linearMapReq;
    hwcExt2ExecuteFifoReq_t              executeFifoReq;
    hwcExt2ReleaseContextReq_t           releaseContextReq;
    hwcExtI2CwriteReq_t                  i2c_WriteReq;
    hwcExtI2CreadReq_t                   i2c_ReadReq;
    hwcExtI2CmultiWriteReq_t			 i2c_MultiWriteReq;
    hwcExt2LinearMapInfoReq_t            mapInfoReq;
    hwcExt2UnmapMemoryReq_t              unmapMemoryReq;
  } optData;
} hwcExt2Request_t;

typedef struct hwcExt2Result_s {
  FxI32
    resStatus;
  union resOptionalData{
    hwcExt2ProtocolRevisionRes_t protocolRevisionRes;
    hwcExt2DriverVersionRes_t    driverVersionRes;
    hwcExt2AllocContextRes_t     allocContextRes;
    hwcExt2DeviceConfigRes_t     deviceConfigRes;    
    hwcExt2LinearMapRes_t       linearMapRes;
    hwcExtI2CreadRes_t          i2c_ReadRes;
    hwcExt2AGPInfoRes_t          agpInfoRes;
    hwcExt2FifoInfoRes_t         fifoInfoRes;
    hwcExt2LinearMapInfoRes_t    mapInfoRes;
    /* no result for gamma download */
    hwcExt2ShareContextDataRes_t   contextDataRes;
  } optData;
} hwcExt2Result_t;


#ifdef WINNT
/*
 * Global structure used to store app state, used by the display driver
 *
 *   This will need changing if one driver supports multiple boards.
 *
 */

#ifdef ENABLE_V3_W2K_GLIDE_CHANGES
#define _GLIDE_TOTAL_COMMAND_FIFOS 0x02
#endif

typedef struct
{
    LONG        glideGDIFlags;  /* For cooperation with GDI.(Exclusive flag) */
    HANDLE      glideProcessHandle; /* Handle of glide process that last mapped membases */
    LONG        glideReferenceCount ; /* this must be zero before the memory is unmapped */
    BYTE*       glideRegBase;           /* Saved glide register base address */
    BYTE*       glideLfbBase;           /* Saved glide Lfb base address - for mapping only */
    BYTE*       glideScreenBase;        /* Saved glide screen base address - used by glide. */
    BYTE*       glideIOBase;
    //ULONG       vidProcCfg
    //ULONG       vidDesktopOverlayStride
#ifdef SLI_AA
    BYTE        *glideSlaveRegBase[HWINFO_SST_MAX_NUM_CHIPS * HWINFO_SST_MAX_CHIP_INDEX];
#endif

#if (_WIN32_WINNT >= 0x0500)
    PDEV        *ppdev;
    ULONG       contextDWORDNTIndex;
    ULONG       fifoType;
    ULONG       lastFifoOffset;
    ULONG       lastFifoSize;
    ULONG       lastStateOffset;
    ULONG       lastStateSize;
#endif

#ifdef ENABLE_V3_W2K_GLIDE_CHANGES
	// flag to indicate that GLIDE has requested its command fifo memory be protected through
	// an unmapping process
	DWORD dwProtectCmdFifo;

	// flag to indicate that the memory previously requested to be unmapped should now
	// be unmapped
	DWORD dwUnMapMemoryLFB;

	// pointer to each of the command fifos in the GLIDE ring-3 address space
	// this will be used to determine if the memory being unmapped is owned by GLIDE
	PVOID pvCommandFifoLinearAddress[ _GLIDE_TOTAL_COMMAND_FIFOS ];

	// structure to store the information necessary to unmap the GLIDE mapping of the frame
	// buffer at a point where GLIDE indicates it should be done
    VIDEO_SHARE_MEMORY sUnMapShareMemory;
#endif

} GLIDESTATE;

extern GLIDESTATE glideState[];

#if (_WIN32_WINNT >= 0x0500)
#define MAX_ADDRESS_TABLE_SIZE    256

typedef struct _LFBMapping
{
  HANDLE  ProcessID;
  DWORD   LFBAddr;
} LFBMAPPING;

extern LFBMAPPING LfbMappings[];
#endif
#endif                                                  /* WINNT */

#endif                          /* HWCEXT_H not defined */
