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
** File name:   hwcext.h
**
** Description: Structures for the hwc extensions.
**
** $History: hwcext.h $
** 
** *****************  Version 5  *****************
** User: Dow          Date: 4/22/99    Time: 4:33p
** Updated in $/releases/VOODOO3/V3_OEM_100/3DFX/DEVEL/H3/MINIHWC
** Alt-Tab for NT
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
** more reliable.  I now see no corruption or hangs.  PRS 3320 is 
** possibly associated.  There may be others.
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

#define WIN32_LEAN_AND_MEAN
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
  } optData;
} hwcExtResult_t;

#ifdef WINNT
/*
 * Global structure used to store app state, used by the display driver
 *
 *   This will need changing if one driver supports multiple boards.
 *
 */

typedef struct
{
    LONG        glideGDIFlags;  /* For cooperation with GDI.(Exclusive flag) */
    HANDLE      glideProcessHandle; /* Handle of glide process that last mapped membases */
    BYTE*       glideRegBase;           /* Saved glide register base address */
    BYTE*       glideLfbBase;           /* Saved glide Lfb base address - for mapping only */
    BYTE*       glideScreenBase;        /* Saved glide screen base address - used by glide. */
    ULONG       vidProcCfg;
    ULONG       vidDesktopOverlayStride;
} GLIDESTATE;

extern GLIDESTATE glideState;
#endif                                                  /* WINNT */

#endif                          /* HWCEXT_H not defined */
