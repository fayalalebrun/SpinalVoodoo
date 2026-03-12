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


#ifndef _FXTINI_H_
#define _FXTINI_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <3dfx.h>
#include "init.h"
#include <fxdll.h>

/*----------------------------------------------------------------------------
  DATA DEFINITIONS
  ----------------------------------------------------------------------------*/
/*-------------------------------------------------------------------
  Structure: InitContext
  Date: 10/9
  Implementor(s): jdt, murali
  Library: Init
  Description:
  Contains all device dependant functions for a given 3Dfx device
  Members:
  setVideo         - initilize video 
  restoreVideo     - undo setVideo
  enableTransport  - enable the command transport
  disableTransport - disable the command transport
  swapBuffers    - function which executes a buffer swap
  status         - function which returns the status word from the 3D subsystem
  busy           - returns true if hardware busy
  idle           - returns when 3D subsystem is idle
  getBufferPtr   - get a pointer to a frame buffer
  renderBuffer   - set the current buffer for rendering
  origin         - set the y origin 
  ioCtl          - CYA function 
  control        - OS response functions

  gamma          - initializes gamma table
  sliPciOwner    - sets ownership of PCI bus in SLI configuration

  info           - hardware description
  writeMethod    - function for doing command transport writes to hardware
  devNumber      - enumerated order of device on bus
  devPrivate     - Private data for the device type
  -------------------------------------------------------------------*/
typedef struct {
    FxBool       (*setVideo) (
                    FxU32                   hWnd,
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
    void         (*restoreVideo)( void );
    FxBool       (*enableTransport)( InitFIFOData *info );
    void         (*disableTransport)( void );

    void         (*swapBuffers)( FxU32 code );
    FxU32        (*status)( void );
    FxBool       (*busy)(void);
    void         (*idle)( void );
    void        *(*getBufferPtr)( InitBuffer_t buffer, int *strideBytes );
    void         (*renderBuffer)( InitBuffer_t buffer );
    void         (*origin)( InitOriginLocation_t origin );
    void         (*ioCtl)( FxU32 token, void *argument );
    FxBool       (*control) ( FxU32 code );
    FxBool       (*wrapFIFO) (InitFIFOData *fD);
   
    void         (*gamma)( double gamma );
    void         (*sliPciOwner)( FxU32 *regbase, FxU32 owner );
  
    /*
    **  added interface for Glide 3
    */ 
    FxBool       (*gammaRGB)( double r, double g, double b );
    FxBool       (*initGammaTable)(FxU32 nentries, FxU32 *r, FxU32 *g, FxU32 *b);
    sst1VideoTimingStruct * (*findVidTimingStruct) (
                                                    GrScreenResolution_t    sRes,
                                                    GrScreenRefresh_t       vRefresh);

    InitDeviceInfo    info;
    InitWriteCallback *writeMethod;
    void              *devPrivate;
} InitContext;

/* Global current context */
extern InitContext *context;

void vgDriverInit( InitContext *context );
void vg96DriverInit( InitContext *context );

#ifdef __cplusplus
}
#endif
#endif
