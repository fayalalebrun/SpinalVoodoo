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
*/

#ifndef _TINI_H_
#define _TINI_H_

#include <3dfx.h>
#include <sst1vid.h>
#include <sst1init.h>

#ifdef __cplusplus
extern "C" { 
#endif
    
/*------------------------------------------------------- 
  Constants
  -------------------------------------------------------*/
/*
  More slots than you will ever see in a consumer system
 */
#define INIT_MAX_DEVICES 8
  
/*--IOCTLs-----------------------------------------------*/
  /* An IOCTL value is appliacable on all systems where the the
     associated value is ORed into the low eight bits.  The reason for
     this is so that code that calls the init code can do IOCTLs
     without worrying about conditional compilation for device
     dependance.  So, the form of an IOCTL is:

     #define INIT_FOO_IOCTL \
       ((hw1 | hw2 | ... | hwn) | (number << INIT_OCTL_VALUE_SHIFT))

     Remember, this isn't really an I/O control, but rather a back
     door for whatever we've forgotten, or whatever may only apply to
     a small-enough subset of 3Dfx hardware that it doesn't justify
     modifying the init API.
 */
#define INIT_IOCTL_SST1         0x01
#define INIT_IOCTL_SST96        0x02
#define INIT_IOCTL_SST97        0x04


#define INIT_IOCTL_VALUE_SHIFT  0x8

#define INIT_VIDTIMINGS_IOCTL  \
  (INIT_IOCTL_SST1 | (0x1 << INIT_IOCTL_VALUE_SHIFT))

  /*
     Vendor IDs and Device IDs
     */
/* 3Dfx Voodoo Graphics */
#define TDFXVID         0x121A
#define SST1DID         0x01  

/* Alliance AT3D */
#define AT3DVID         0x1142
#define AT3DDID         0x643d

/* Trident T96851 */
#define T968VID         0x1023
#define T968DID         0x9660

/* MRTI M510 */
#define MRTIVID         0x1293
#define MRTIDID         0x0510

/* Macronix MX86251 Fixme!!! these are bogus !!! XXX ??? */
#define MCRXVID         0x10d9
#define MCRXDID         0x8626

/*
 * (* Murali's comments *)
 *
 * Note that *EVERY* one of the following 3 definitions have an exact
 * counterpart in glide.h. 
 *
 * We could have used the glide definitions, but that would affect layering 
 * glide/src/ * on top of this sst1/init/ * library. So, we use these
 * redundant defintions, and let glide/src/ * translate from glide.h to
 * init.h defintions.  (Note that translation is simply a typecast).
 *
 * The only exception that proves this rule is sst1vid.h, where something 
 * used in the init library (GrScreenResolution_t, GrScreenRefresh_t) 
 * is exposed to the glide application from sst1vid.h, WHICH LIVES IN
 * THIS DIRECTORY, so we can use these typedefs here directly.
 *
 * Here are the relevant counterparts in init.h and glide.h:
 *
 * initBuffer_t             GrBuffer_t
 * initOriginLocation_t     GrOriginLocation_t
 * initColorFormat_t        GrColorFormat_t
 * initBufDesc_t              ??? (GrBufDesc_t)
 *
 *
 * PLEASE PLEASE PLEASE 
 * Make sure that the correspondence is exact.
 */
typedef FxU32   InitBuffer_t;
#define INIT_BUFFER_FRONTBUFFER 0x0
#define INIT_BUFFER_BACKBUFFER  0x1
#define INIT_BUFFER_AUXBUFFER   0x2
#define INIT_BUFFER_DEPTHBUFFER 0x3
#define INIT_BUFFER_ALPHABUFFER 0x4
#define INIT_BUFFER_TRIPLEBUFFER 0x5

#define INIT_BUFFER_FIFOBUFFER      0x6
#define INIT_BUFFER_SCREENBUFFER    0x7
#define INIT_BUFFER_TEXTUREBUFFER   0x8
#define INIT_BUFFER_NONE            0xff

typedef FxU32   InitOriginLocation_t;
#define INIT_ORIGIN_UPPER_LEFT  0x0
#define INIT_ORIGIN_LOWER_LEFT  0x1

typedef FxI32   InitColorFormat_t;
#define INIT_COLORFORMAT_ARGB   0x0
#define INIT_COLORFORMAT_ABGR   0x1
#define INIT_COLORFORMAT_RGBA   0x2
#define INIT_COLORFORMAT_BGRA   0x3

typedef FxI32   InitSmoothingMode_t;
#define INIT_SMOOTHING_DISABLE  0x0
#define INIT_SMOOTHING_ENABLE   0x1

typedef FxU32   InitControl_t;
#define INIT_CONTROL_ACTIVATE   0x1
#define INIT_CONTROL_DEACTIVATE 0x2
#define INIT_CONTROL_RESIZE     0x3
#define INIT_CONTROL_MOVE       0x4

typedef struct _InitBufDesc_t {
    FxU32           bufMagic;
    InitBuffer_t    bufType;
    FxU32           bufOffset;
    FxI32           bufStride;
    FxU32           bufBPP;             /* bits per pixel */
} InitBufDesc_t;

/*------------------------------------------------------- 
  Data Structures
  -------------------------------------------------------*/


/*-------------------------------------------------------------------
  Structure: VGInfo
  Date: 10/4/96
  Implementor(s): jdt
  Library: Init
  Description:
  Drescribes the capabilities of a standard Voodoo Graphics chip.
  Members:
  pfxRev    - pixelFx chip rev
  pfxRam    - frame buffer ram
  nTFX      - number of texelFx chips
  tfxRev    - texelFx revision
  tfxRam    = texelFx ram
  sliDetect - scan line interleave configuration detected
  -------------------------------------------------------------------*/
typedef struct _VGInfo {
    FxU32   vgBaseAddr;
    FxU32   pfxRev;
    FxU32   pfxRam;
    FxU32   nTFX;
    FxU32   tfxRev;
    FxU32   tfxRam;
    FxBool  sliDetect;
    FxU32   slaveBaseAddr;
} VGInfo;

/*-------------------------------------------------------------------
  Structure: VG96Info
  Date: 10/4/96
  Implementor(s): jdt
  Library: Init
  Description:
  Describes the capabilities of VG 96 accelerated VGA board
  Members:
  vgaChip - code describing partner VGA part
            0 - undefined
            1 - Alliance AT3D
            2 - Trident T9685
            3 - MRTI MR510
            4 - Macronics MX 86251
  vg96Rev - revision of VG96 chip
  vg96Ram - frame buffer ram
  nTFX      - number of texelFx chips
  tfxRev  - texelFx revision
  tfxRam  - texelFx ram
  -------------------------------------------------------------------*/
typedef struct _VG96Info {
    FxU32 vgaBaseAddr;
    FxU32 vg96BaseAddr;
    FxU32 vgaChip;
    FxU32 vg96Rev;
    FxU32 vg96Ram;
    FxU32 nTFX;
    FxU32 tfxRev;
    FxU32 tfxRam;
} VG96Info;

typedef enum { INIT_VOODOO,
               INIT_VG96,
               NUM_3DFX_PRODUCTS
} TDFXHwClass;

typedef struct _InitRegisterDesc {
  union {
    struct {
      FxU32
        *baseAddress,           /* Base address */
        *slavePtr;              /* Scanline interleaved slave */
    } VGRegDesc;
    struct {
      volatile FxU32
        *baseAddress,           /* Base address */
        *partnerRegPtr,         /* Partners' registers */
        *serialStatus,          /* Serial Status register */
        *fifoApertureBase;      /* Ptr to base of FIFO apurture */
      
      FxU8 mclockHi;
      FxU8 mclockLo;
    } VG96RegDesc;
  } hwDep;
} InitRegisterDesc;

/*-------------------------------------------------------------------
  Structure: InitDeviceInfo
  Date: 10/4
  Implementor(s): jdt
  Library: Init
  Description:
  Describes a 3Dfx enabled hardware device in a system.  Includes
  the device number which will uniquely identify this device in the
  current system configuration
  Members:
  devNumber - unique device number
  class     - 
  -------------------------------------------------------------------*/
typedef struct _InitDeviceInfo {
    FxU32        devNumber;
    TDFXHwClass  hwClass;
    FxU16
      vendorID, deviceID;
    union {
      VGInfo   vgInfo;
      VG96Info vg96Info;
    } hwDep;
    InitRegisterDesc regs;
} InitDeviceInfo;


typedef FxU32 InitSwapType_t;
#define INIT_SWAP_FLIP 0x0
#define INIT_SWAP_BLT  0x1

typedef struct _InitFIFOData {
  FxU32
    cpuType;                    /* 5 for Pentium, 6 for Pentium Pro */
  union {
    struct {
      FxU32 fifoFree;
      FxU32 memFifoStatusLwm;
    } vgFIFOData;
    struct {
      volatile FxU32
        *fifoVirt,              /* linear address of FIFO Start */
        fifoMax,                /* Max linear address in FIFO  */
        fifoPhys,               /* Physical location of FIFO */
        fifoCur,                /* Current Wrap */
        *fifoPtr,               /* Pointer to current FIFO entry */
        fifoSize;               /* Size of FIFO in bytes */

#if SST96_ALT_FIFO_WRAP
      /* dpc - 10 sep 1997 FixMe! - 
       * I need to pass the init code the block size that I want to
       * write, but I don't want to change the interface out from
       * everyone else. If we re-visit the init code interface then
       * fix me then.  
       */
      FxI32  blockSize;

      FxU32  fifoRead;          /* Last known hw read ptr */
      FxI32  roomToReadPtr;     /* # of bytes to fifoRead */
      FxI32  roomToEnd;         /* # of bytes to fifoEnd */

      InitRegisterDesc* regDesc;
#endif /* SST96_ALT_FIFO_WRAP */

      FxBool
        fifoLfb;                /* LFB FIFO or alternate? */
    } vg96FIFOData;
  } hwDep;
} InitFIFOData;

/*------------------------------------------------------- 
  Hardware Independant Setup Calls
  -------------------------------------------------------*/
typedef FxBool InitHWEnumCallback( const InitDeviceInfo *info );
typedef void InitWriteCallback( FxU32 *linaddr, FxU32 val );

void   initEnumHardware( InitHWEnumCallback *cb );
volatile FxU32* initMapBoard(const FxU32 boardNum);
void   initClose (void);
FxBool initGetDeviceInfo( FxU32 devNumber, InitDeviceInfo *info );
FxBool initDeviceSelect( FxU32 devNumber );

/*------------------------------------------------------- 
  Driver Calls
  -------------------------------------------------------*/
void    initSetWriteMethod( InitWriteCallback *wcb );
FxBool  initSetVideo( FxU32                   hWnd,
                      GrScreenResolution_t    sRes,
                      GrScreenRefresh_t       vRefresh,
                      InitColorFormat_t       cFormat,
                      InitOriginLocation_t    yOrigin,
                      int                     nColBuffers,
                      int                     nAuxBuffers,
                      int                     *xres, 
                      int                     *yres,
                      int                     *fbStride,
                      sst1VideoTimingStruct   *vidTimings);
void    initRestoreVideo( void );

FxBool  initEnableTransport( InitFIFOData *info );
void    initDisableTransport( void );

void    initSwapBuffers( FxU32 code );
FxU32   initStatus( void );
FxBool  initBusy( void );
void    initIdle( void );
void   *initGetBufferPtr( InitBuffer_t buffer, int *strideBytes );
void    initRenderBuffer( InitBuffer_t buffer );
void    initOrigin( InitOriginLocation_t origin );
void    initIOCtl( FxU32 token, void *argument );
FxBool  initControl (FxU32 code);
FxBool  initWrapFIFO(InitFIFOData *fifoData);

/* SST-1 Flotsam */
void    initGamma( double gamma );
void    initSliPciOwner( FxU32 *regbase, FxU32 owner );
FxU32   initNumBoardsInSystem( void );

const char*
myGetenv(const char*);

/*
** Glide 3 init functions
*/
FxBool initGammaRGB( double r, double g, double b );
FxBool initGammaTable(FxU32 nentries, FxU32 *r, FxU32 *g, FxU32 *b);
sst1VideoTimingStruct *initFindVideoTimingStruct(GrScreenResolution_t sRes, GrScreenRefresh_t vRefresh) ;

#ifdef __cplusplus
}
#endif

#endif /* _TINI_H_ */
