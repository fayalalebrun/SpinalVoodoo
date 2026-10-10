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
*/

#include "init.h"
#include "fxinit.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(__WATCOMC__)
#include <conio.h>
#define _inp inp
#define _outp outp
#define _outpw outpw
#endif

#if defined(__linux__)
#define _inp(port) pioInByte(port)
#define _outp(port, data) pioOutByte(port, data)
#define _outpw(port, data) pioOutWord(port, data);
#endif

#ifdef __DJGPP__
#include <fxdpmi.h>
#endif

#if defined(SST96)
#include <init96.h>
#else
#include <sst1init.h>
#endif

#ifdef _WIN32
#define _WIN32_LEAN_AND_MEAN_
#include <windows.h>
#ifdef __MINGW32__
static inline unsigned char _inp_asm (unsigned short _port) {
  unsigned char rc;
  __asm__ __volatile__ ("inb %w1,%b0" : "=a" (rc) : "Nd" (_port));
  return rc;
}
static inline void _outp_asm (unsigned short _port, unsigned char _data) {
  __asm__ __volatile__ ("outb %b0,%w1" : : "a" (_data), "Nd" (_port));
}
#define _inp  _inp_asm
#define _outp  _outp_asm
#endif
#endif /* _WIN32 */

#include <fxpci.h>
#include <gdebug.h>


/*-------------------------------------------------------------------
  Module Constants
  -------------------------------------------------------------------*/

#if 0
static InitContext
  contexts[NUM_3DFX_PRODUCTS];  /* pool of device contexts */
#endif
static InitContext initctx;     /* device context - no pool anymore */
InitContext
  *context;                     /* Current device context */
static InitDeviceInfo
  hwInfo[INIT_MAX_DEVICES];     /* Info for all supported devices */
static FxU32
  numDevicesInSystem,           /* Number of supported devices. */
  numSst1s;                     /* # SST1s in system */
static FxBool
  libInitialized;               /* Well, has it been? */

/*-------------------------------------------------------------------
  Function: initEnumHardware
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description:
  Calls a user supplied function on an initialized InitDeviceInfo
  structure for each device in the system.  Calls a user supplied
  callback repeatedly for each device in the system.  The callback
  can stop the enumeration cycle by return a value of FXTRUE.
  Arguments:
  cb - callback function of type INitHWEnumCallback which is called on
       an initialzied InitDeviceInfo structure
  Return:
  none
  -------------------------------------------------------------------*/
void
initEnumHardware( InitHWEnumCallback *cb )
{
  FxU32  busLocation;
  FxU32  device;
  if ( !libInitialized ) {

    /* When initializing the Library snoop out all 3Dfx devices 
       and fill a static data structure with pertinant data.  */

    numDevicesInSystem = 0;
    numSst1s = 0;
    if ( !pciOpen() ) return;
    for( busLocation = 0; busLocation < MAX_PCI_DEVICES; busLocation++ ) {
      if ( pciDeviceExists( busLocation ) ) {
        FxU32 vId, dId;
        pciGetConfigData( PCI_VENDOR_ID, busLocation, &vId );
        pciGetConfigData( PCI_DEVICE_ID, busLocation, &dId );

        GDBG_INFO((80, "initEnumHardware:  Vendor:  0x%x  Device:  0x%x\n", vId, dId));

#if defined( SST1 )
        if ( (vId == TDFXVID) &&
            (dId == SST1DID) ) { /* Detect SST1 */
          FxU32 *base;
          sst1DeviceInfoStruct info;

          /* Scanline interleave must be two boards back to back
             if there is a second board in the system,
             and the previous board was SLI, then this is the slave */
          if ( numDevicesInSystem > 0 ) {
            if ( hwInfo[numDevicesInSystem-1].hwClass==INIT_VOODOO &&
                hwInfo[numDevicesInSystem-1].hwDep.vgInfo.sliDetect ) {

              hwInfo[numDevicesInSystem-1].hwDep.vgInfo.slaveBaseAddr =
                (FxU32)sst1InitMapBoard( numSst1s );
              hwInfo[numDevicesInSystem-1].regs.hwDep.VGRegDesc.slavePtr = 
                (FxU32*)hwInfo[numDevicesInSystem-1].hwDep.vgInfo.slaveBaseAddr;
              numSst1s++;
              continue;

            }
          }

          hwInfo[numDevicesInSystem].vendorID  = (FxU16) vId;
          hwInfo[numDevicesInSystem].deviceID  = (FxU16) dId;
          hwInfo[numDevicesInSystem].devNumber = numDevicesInSystem;
          hwInfo[numDevicesInSystem].hwClass   = INIT_VOODOO;

          /* On SST-1 We Have to Initialize the Registers
             to Discover the configuration of the board */
#if 0
          base = sst1InitMapBoard( numSst1s );
          sst1InitRegisters( base );
#else
          base = (FxU32*)initMapBoard(numSst1s);
#endif
          sst1InitGetDeviceInfo( base, &info );

          hwInfo[numDevicesInSystem].hwDep.vgInfo.vgBaseAddr = (FxU32) base;
          hwInfo[numDevicesInSystem].hwDep.vgInfo.pfxRev    = info.fbiRevision;
          hwInfo[numDevicesInSystem].hwDep.vgInfo.pfxRam    = info.fbiMemSize;
          hwInfo[numDevicesInSystem].hwDep.vgInfo.nTFX      = info.numberTmus;
          hwInfo[numDevicesInSystem].hwDep.vgInfo.tfxRev    = info.tmuRevision;
          hwInfo[numDevicesInSystem].hwDep.vgInfo.tfxRam    = info.tmuMemSize[0];
          hwInfo[numDevicesInSystem].hwDep.vgInfo.sliDetect = info.sstSliDetect;
          hwInfo[numDevicesInSystem].hwDep.vgInfo.slaveBaseAddr = 0;
          hwInfo[numDevicesInSystem].regs.hwDep.VGRegDesc.baseAddress = base;
          hwInfo[numDevicesInSystem].regs.hwDep.VGRegDesc.slavePtr = 0;

          numSst1s++;
          numDevicesInSystem++;
        }
#elif defined(SST96)
#define IS_CHIP(name) (vId == name##VID && dId == name##DID)

        if (IS_CHIP(AT3D) ||
            IS_CHIP(MCRX)) {

          if (IS_CHIP(MCRX)) {
            /* In the case of Macronix, look for 3d4/3f[2] == 1, as
               they set that bit when we're attached.  */
            FxU8 regVal;
            _outp(0x3d4, 0x3f);
            regVal = _inp(0x3d5);

            if (!(regVal & (1 << 2))) /* we're not there */
              continue;
          }

          hwInfo[numDevicesInSystem].vendorID  = (FxU16) vId;
          hwInfo[numDevicesInSystem].deviceID  = (FxU16) dId;
          hwInfo[numDevicesInSystem].devNumber = numDevicesInSystem;
          hwInfo[numDevicesInSystem].hwClass   = INIT_VG96;

          /* SST-96 initialization also retrieves board configuration info */
#if 0
          init96MapBoard(&hwInfo[numDevicesInSystem].regs,
                         &hwInfo[numDevicesInSystem].hwDep.vg96Info,
                         (FxU16) vId, (FxU16) dId);
#else
          initMapBoard(numDevicesInSystem);
#endif

          hwInfo[numDevicesInSystem].hwDep.vg96Info.vgaBaseAddr = 
            (FxU32)hwInfo[numDevicesInSystem].regs.hwDep.VG96RegDesc.partnerRegPtr;
          hwInfo[numDevicesInSystem].hwDep.vg96Info.vg96BaseAddr = 
            (FxU32)hwInfo[numDevicesInSystem].regs.hwDep.VG96RegDesc.baseAddress;

          numDevicesInSystem++;
        }
#else
#  error "Do hardware enumeration for this chip!"
#endif
      }
    }

#if defined(SST1)
    /* Sanity Check for SLI detection */
    for( device = 0; device < numDevicesInSystem; device++ ) {
      if( hwInfo[device].hwDep.vgInfo.sliDetect           &&
          hwInfo[device].hwDep.vgInfo.slaveBaseAddr == 0 ) {
        hwInfo[device].hwDep.vgInfo.sliDetect = FXFALSE;
      }
    }
#endif

#if 0
    /* Initialize all drivers */
    vgDriverInit( &contexts[INIT_VOODOO] );
    vg96DriverInit( &contexts[INIT_VG96] );
#endif
#if !defined(SST96)
    vgDriverInit( &initctx );
#else
    vg96DriverInit( &initctx );
#endif

    /* Mark the library as initialized */
    libInitialized = FXTRUE;
  }

  if ( cb ) {
    for( device = 0; device < numDevicesInSystem; device++ ) {
      cb( &hwInfo[device] );
    }
  }
  return;
} /* initEnumHardware */

volatile FxU32*
initMapBoard(const FxU32 boardNum)
{
   volatile FxU32* retVal = NULL;
   FxBool okP = (boardNum < INIT_MAX_DEVICES);
   
   if (okP) {
      InitDeviceInfo* infoP = (hwInfo + boardNum);
      const FxU32 vId = infoP->vendorID;
      const FxU32 dId = infoP->deviceID;
      
#if defined(SST1)
      okP = ((vId == TDFXVID) &&
             (dId == SST1DID));

      if (okP) {
         retVal = sst1InitMapBoard(boardNum);
         sst1InitRegisters((FxU32*)retVal);
      }
#elif defined(SST96)
      okP = (IS_CHIP(AT3D) ||
             IS_CHIP(MCRX));

      if (okP) {
         init96MapBoard(&infoP->regs, &infoP->hwDep.vg96Info,
                        (FxU16)vId, (FxU16)dId);
         retVal = infoP->regs.hwDep.VG96RegDesc.baseAddress;
      }
#else
#error "Need to define initMapBoard for this chip!"
#endif /* HW Check */
   }

   return retVal;
}


/*-------------------------------------------------------------------
  Function: initClose
  Date: 10/17
  Implementor(s): jdt
  Library: init
  Description:
  Unmap all hardware and set library to unitialized state
  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/
void initClose( void ) {
    if ( libInitialized ) {
        unsigned i;
        for ( i = 0; i < numDevicesInSystem; i++ ) {
            /* Insert unmap code here */ /* FIXME!!!! */
        }
        pciClose();
        libInitialized = FXFALSE;
    } 
} /* initClose */

/*-------------------------------------------------------------------
  Function: initGetDeviceInfo
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description:
  Fills a user supplied area of memory with information describing
  the a 3Dfx device.  The device is identified on the bus by the unique
  devNumber assigned by the library and revealed to the user through
  initEnumHardware.
  Arguments:
  devNumber - number identifying device in system
  info      - pointer to user memory to be filled with dev info
  Return:
  FXTRUE  - succeed
  FXFALSE - one or both parameters invalid
  -------------------------------------------------------------------*/
FxBool
initGetDeviceInfo( FxU32 devNumber, InitDeviceInfo *info )
{
    FxBool rv;

    if ( devNumber < numDevicesInSystem ) {
        *info = hwInfo[devNumber];
        rv = FXTRUE;
    } else {
        rv = FXFALSE;
    }
    return rv;
} /* initGetDeviceInfo */


/*-------------------------------------------------------------------
  Function: initDeviceSelect
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description: 
  Selects a 3Dfx device ( potentially from among many in the system ) and 
  initializes the internal init driver functions with behavior appropriate
  to the type of the selected device.
  Arguments:
  devNumber - unique number assigned to each device by initEnumHardware
  Return:
  FXTRUE - success
  FXFALSE - invalide device number
  -------------------------------------------------------------------*/

FxBool
initDeviceSelect( FxU32 devNumber )
{
  if ( devNumber < numDevicesInSystem ) {
#if 0 /**/
    context = &contexts[hwInfo[devNumber].hwClass];
#endif
#ifdef SST96
    if (hwInfo[devNumber].hwClass != INIT_VG96)
      return FXFALSE;
#else
    if (hwInfo[devNumber].hwClass != INIT_VOODOO)
      return FXFALSE;
#endif
    context = &initctx;
    context->info = hwInfo[devNumber];
    return FXTRUE;
  }
  
  return FXFALSE;

}/* initDeviceSelect */

/*-------------------------------------------------------------------
  Function: initSetWriteMethod
  Date: 10/17
  Implementor(s): jdt
  Library: init
  Description:
  Sets up a register write method that respects the fifo conventions
  of the hardware.
  Arguments:
  wcb - callback to be used to do direct writes from init code
  Return:
  none
  -------------------------------------------------------------------*/
void initSetWriteMethod( InitWriteCallback *wcb ) {
    if ( context )
        context->writeMethod = wcb;
    return;
} /* initSetWriteMethod */



/*-------------------------------------------------------------------
  Function: initSetVideo
  Date: 10/9
  Implementor(s): jdt, murali
  Library: init
  Description:
        This will initialize the video, and allocate color and aux buffers
        prior to rendering.
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
                    lower right
  nColBuffers - number of color buffers to attempt to allocate
                0 - meaningless
                1 - allocate a front buffer only
                2 - allocate a front and back buffer
                3 - allocate a front, back, aux buffer for tripple buffering
  nAuxBuffers - number of aux buffers to attempt to allocate
                0 - no alpha or z buffers
                1 - allocate one aux buffer for alpha/depth buffering
                2 - allocate on depth and one alpha buffer ( unsup )
  xres        - x resolution return val ( for user windows )
  yres        - y resolution return val ( for user windows )
  fbStride    - stride in pixels of frame buffer
  Return:
  FXTRUE - all resources requested where allocated
  FXFALSE - some or all resources were not available.  By comparing
            desc before and after call, the user can determine which
            resources were denied.
  -------------------------------------------------------------------*/
FxBool initSetVideo( FxU32                hWnd,
                     GrScreenResolution_t sRes,
                     GrScreenRefresh_t    vRefresh,
                     InitColorFormat_t    cFormat,
                     InitOriginLocation_t yOrigin,
                     int                  nColBuffers,
                     int                  nAuxBuffers,
                     int                  *xres,
                     int                  *yres,
                     int                  *fbStride,
                     sst1VideoTimingStruct *vidTimings) {
    FxBool rv;
    if ( context ) {
        rv = context->setVideo( hWnd, sRes, vRefresh, cFormat, 
                                yOrigin, nColBuffers, nAuxBuffers,
                                xres, yres, fbStride, vidTimings);
    } else {
        rv = FXFALSE;
    }
    return rv;
} /* initSetVideo */


/*-------------------------------------------------------------------
  Function: initRestoreVideo
  Date: 10/16
  Implementor(s): jdt
  Library: init
  Description:
  Restore video to its state prior to initSetVideo
  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/
void initRestoreVideo( void ) {
    if ( context ) 
        context->restoreVideo();
} /* initRestoreVideo */

/*-------------------------------------------------------------------
  Function: initEnableTransport
  Date: 10/17
  Implementor(s): jdt
  Library: init
  Description:
  Enable the command transport mechanism for the underlying 
  hardware and return information about accessing the transport
  to the caller
  Arguments:
  info - data structure describing state of transport
  Return:
  FXTRUE  - successfully enabled the command transport
  FXFALSE - failed ot enable the command transport
  -------------------------------------------------------------------*/
FxBool initEnableTransport( InitFIFOData *info ) {
    FxBool rv;
    if ( context ) {
        rv = context->enableTransport( info );
    } else {
        rv = FXFALSE;
    }
    return rv;
} /* initEnableTransport */

/*-------------------------------------------------------------------
  Function: initDisableTransport
  Date: 10/17
  Implementor(s): jdt
  Library: init
  Description:
  Return transport control to reset state
  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/
void initDisableTransport( void ) {
    if ( context ) 
        context->disableTransport();
} /* initDisableTransport */

/*-------------------------------------------------------------------
  Function: initSwapBuffers
  Date: 10/9
  Implementor(s): jdt, murali
  Library: init
  Description:
  Initiate a buffer swap.  
  Arguments: 
  code - code describing type of swap, wether to sync to vretrace, how
         many vretraces to wait, etc.
  -------------------------------------------------------------------*/
void initSwapBuffers( FxU32 code ) {
    if ( context )
        context->swapBuffers( code );
} /* initSwapBuffers */

/*-------------------------------------------------------------------
  Function: initStatus
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description:
  Returns the status word from the hardware
  Arguments:
  none
  Return:
  32-bit status word ( chip specific )
  -------------------------------------------------------------------*/
FxU32 initStatus( void ) {
    FxU32 rv;
    if ( context ) {
        rv = context->status();
    } else {
        rv = 0;
    }
    return rv;
} /* initStatus */

/*-------------------------------------------------------------------
  Function: initBusy
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description:
  return true if hardware is busy rendering
  Arguments:
  none
  Return:
  FXTRUE  - if busy
  FXFALSE - if idle
  -------------------------------------------------------------------*/
FxBool initBusy( void ) {
    FxBool rv;
    if ( context ) {
        rv = context->busy();
    } else {
        rv = FXTRUE;
    }
    return rv;
} /* initBusy */

/*-------------------------------------------------------------------
  Function: initIdle
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description:
  Returns only when graphics pipeline is completely idle. 
  May hang software if hardware is permanently busy.
  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/
void initIdle( void ) {
    if ( context )
        context->idle();
} /* initIdle */

/*-------------------------------------------------------------------
  Function: initGetBufferPtr
  Date: 2/25
  Implementor(s): jdt
  Library: Init
  Description:
  Get the current pointer to a particular buffer
  Arguments:
  buffer - which buffer to get pointer to 
  Return:
  integer pointer
  -------------------------------------------------------------------*/
void *initGetBufferPtr( InitBuffer_t buffer, int *strideBytes ) {
    if ( context )
        return context->getBufferPtr( buffer, strideBytes );
    else
        return 0;
} /* initGetBufferPtr */

/*-------------------------------------------------------------------
  Function: initRenderBuffer
  Date: 2/25
  Implementor(s): jdt
  Library: init 
  Description:
  Sets the current render buffer
  Arguments:
  buffer - which buffer to render to ( front or back )
  Return:
  none
  -------------------------------------------------------------------*/
void initRenderBuffer( InitBuffer_t buffer ) {
    if ( context ) context->renderBuffer( buffer );
    return;
} /* initRenderBuffer */

/*-------------------------------------------------------------------
  Function: initOrigin
  Date: 2/25
  Implementor(s): jdt
  Library: init
  Description:
  Set the yorigin
  Arguments:
  origin - origin designation
  Return:
  none
  -------------------------------------------------------------------*/
void initOrigin( InitOriginLocation_t origin ) {
    if ( context ) context->origin( origin );
    return;
} /* initOrigin */

/*-------------------------------------------------------------------
  Function:initIOCtl
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description:
  CYA function so that emergency functionality may be added w/o 
  corrupting dll interface
  Arguments:
  token    - describes IOCTL function
  argument - pointer to arguments
  Return:
  none
  -------------------------------------------------------------------*/
void initIOCtl( FxU32 token, void *argument ) {
    if ( context )
        context->ioCtl( token, argument );
} /* initIOCtl */

/*-------------------------------------------------------------------
  Function: initControl
  Date: 10/9
  Implementor(s): murali
  Library: init
  Description:
        This will re-attach new buffers for the rendering context, used
        typically only in a windowing system where buffers need to move 
        and resize. Also, implicitly detach old associated buffers.
  Arguments:
        XXX
  Return:
  FXTRUE - all resources requested where allocated
  FXFALSE - some or all resources were not available.  By comparing
            desc before and after call, the user can determine which
            resources were denied.
  -------------------------------------------------------------------*/
FxBool initControl( FxU32 code)
{
    FxBool rv;

    GDBG_INFO((80, "initControl: code = %d, context=%.08x\n", code, context));
    if ( context ) {
        rv = context->control( code );
    } else {
        rv = FXFALSE;
    }
    return rv;
} /* initControl */

/*-------------------------------------------------------------------
  Function: initWrapFIFO
  Date: 13-Feb-97
  Implementor(s): dow
  Library: init
  Description:
  Reset the command transport when the fifo is full
  Arguments:
  pointer to fifo data
  Return:
  none
  -------------------------------------------------------------------*/
FxBool initWrapFIFO(InitFIFOData *fifoData) {
    if ( context )
        return context->wrapFIFO(fifoData);
    else
        return FXFALSE;
} /* initWrapFIFO */

/*-------------------------------------------------------------------
  Function: initGamma
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description:
  SST-1 holdover function which initializes the gamma table in some
  video hardware to normalize monitor brightness
  Arguments:
  gamma - floating point gamma correction factor
  Return:
  none
  -------------------------------------------------------------------*/
void initGamma( double gamma ) {
    if ( context )
        context->gamma( gamma );
} /* initGamma */

/*-------------------------------------------------------------------
  Function: initSliPciOwner
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description:
  Do some voodoo(tm) to change the owner of the PCI bus in an SLI
  configuration
  Arguments:
  regbase - ask scott sellers
  owner   - ask scott sellers
  Return:
  none
  -------------------------------------------------------------------*/
void initSliPciOwner( FxU32 *regbase, FxU32 owner ) {
    if ( context )
        context->sliPciOwner( regbase, owner );
} /* initSliPciOwner */


/*-------------------------------------------------------------------
  Function: initNumBoardsInSystem
  Date: 19970813
  Implementor(s): pgj
  Library: init
  Description:
    Computes the number of boards compatible with the hardware the init
    library is compiled for, i.e. this will count either the number
    of VG xor Rush cards present.
  Return:
    number of boards found
  -------------------------------------------------------------------*/
FxU32 initNumBoardsInSystem(void)
{
    FxU32 numBoards, j, n;
    const char *envp;

    envp = getenv(("SST_BOARDS"));
    if(envp != NULL) {
        numBoards = atoi(envp);
    } else {
        numBoards = 0;
#if defined(SST1)
        for(j=0; j<INIT_MAX_DEVICES; j++) {
            if(pciFindCardMulti(TDFXVID, SST1DID, &n, j)) numBoards++;
        }
#elif defined(SST96)
        for(j=0; j<INIT_MAX_DEVICES; j++) {
            if(pciFindCardMulti(AT3DVID, AT3DDID, &n, j)) numBoards++;
            if(pciFindCardMulti(MCRXVID, MCRXDID, &n, j)) numBoards++;
        }
#else
#  error "initNumBoardsInSystem: unknown device!!!"
#endif
    }
    return(numBoards);
} /* end initNumBoardsInSystem() */

/*-------------------------------------------------------------------
  Function: initGammaRGB
  Date: Mar-2-98
  Implementor(s): atai
  Library: init
  Description:
  initialize Gamma table givin RGB value
  Arguments:
  Return:
  none
  -------------------------------------------------------------------*/
FxBool initGammaRGB( double r, double g, double b ) {
    if ( context )
        return context->gammaRGB( r, g, b );
    return FXFALSE;
} /* initGammaRGB */

/*-------------------------------------------------------------------
  Function: initGammaTable
  Date: Mar-2-98
  Implementor(s): atai
  Library: init
  Description:
  initialize Gamma table givin RGB table
  Arguments:
  Return:
  none
  -------------------------------------------------------------------*/
FxBool initGammaTable(FxU32 nentries, FxU32 *r, FxU32 *g, FxU32 *b) {
    if ( context )
        return context->initGammaTable( nentries, r, g, b );
    return FXFALSE;
} /* initGammaTable */

/*-------------------------------------------------------------------
  Function: initFindVideoTimingStruct
  Date: Mar-2-98
  Implementor(s): atai
  Library: init
  Description:
  find video timing structure
  Arguments:
  Return:
  none
  -------------------------------------------------------------------*/
sst1VideoTimingStruct *initFindVideoTimingStruct(GrScreenResolution_t sRes, GrScreenRefresh_t vRefresh) {
    if ( context )
        return context->findVidTimingStruct( sRes, vRefresh );
    return NULL;
} /* initGamma */

