#ifndef __H3HWC_H__
#define __H3HWC_H__

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

/*-------------------------------------------------------------------
  H3 HWC device specific information
  -------------------------------------------------------------------*/

#include <3dfx.h>
#include <h3info.h>
#include <fxhwc.h>

#define HWC_DEFAULT_FBI_REV 1
#define HWC_DEFAULT_FBI_MEM 4

#define DEV_PRIVATE(hwc) ((FxDeviceInfo *)hwc->devInfo)

#define HWC_BASE_ADDRESS(hwc)	((FxI32)(hwc->virtAddr[0]))
#define HWC_IO_ADDRESS(hwc)	    (SST_IO_OFFSET+HWC_BASE_ADDRESS(hwc))
#define HWC_CMDAGP_ADDRESS(hwc)	(SST_CMDAGP_OFFSET+HWC_BASE_ADDRESS(hwc))
#define HWC_GUI_ADDRESS(hwc)	(SST_2D_OFFSET+HWC_BASE_ADDRESS(hwc))
#define HWC_3D_ADDRESS(hwc)	    (SST_2D_OFFSET+HWC_BASE_ADDRESS(hwc))
#define HWC_TEX_ADDRESS(hwc)	(SST_TEX_OFFSET+HWC_BASE_ADDRESS(hwc))
#define HWC_LFB_ADDRESS(hwc)	(SST_LFB_OFFSET+HWC_BASE_ADDRESS(hwc))
#define HWC_YUV_ADDRESS(hwc)	(SST_YUV_OFFSET+HWC_BASE_ADDRESS(hwc))
#define HWC_PORT_ADDRESS(hwc)	((FxI32)(hwc->virtPort))

//----------------- useful addressing macros -----------------------
// return pointer to SST at specified WRAP, CHIP, or TREX
#define HWC_WRAP(hwc,n) ((SstRegs *)((n)*0x4000+HWC_3D_ADDRESS(hwc)))
#define HWC_CHIP(hwc,n) ((SstRegs *)((n)*0x400+HWC_3D_ADDRESS(hwc)))
#define HWC_TMU(hwc,n)  ((SstRegs *)((0x800<<(n))+HWC_3D_ADDRESS(hwc)))
#define HWC_TREX(hwc,n) HWC_TMU(hwc,n)

#define HWC_IO_UNIT(hwc) ((SstIORegs *)HWC_IO_ADDRESS(hwc))
#define HWC_CMDAGP_UNIT(hwc) ((SstCRegs *)HWC_CMDAGP_ADDRESS(hwc))
#define HWC_GUI_UNIT(hwc) ((SstGRegs *)HWC_GUI_ADDRESS(hwc))
#define HWC_3D_UNIT(hwc) ((SstRegs *)HWC_3D_ADDRESS(hwc))

#define SST_DEVICE_ID_H3        3

#define MBYTE(n)	(((FxU32)(n))<<20)
#define DEAD		0xDEAD

// Maximum number of boards and TMUs supported
#define HAL_MAX_BOARDS 4

#define SST_FAKE_ADDRESS_GET_OFFSET(a) ((FxU32)a&0x0FFFFFFF)

#define SST_FAKE_PORT_GET_OFFSET(a) ((FxU16)a&0x0FFF)

typedef struct {
    int csim;		
    int hsim;		
    int hw;		
    FxU32 boardsFound;	// number of boards found
    FxDeviceInfo boardInfo[HAL_MAX_BOARDS];

    int pollLimit;	// number of pixels to poll msg Q after
    int pollCount;	// current pixel counter
    int video;		// video output enabled
    FxU32 csimLastRead;	
} HalInfo;

FX_ENTRY HalInfo * FX_CALL fxHalInit(FxU32 flags);

FX_ENTRY FxBool FX_CALL fxHalInitRegisters(SstRegs *sst);

FxBool guiOpen( FxU32 boardNumber );

FX_ENTRY FxU32 * FX_CALL agpPhysToVirt( FxU32 physAddrHi, FxU32 physAddrLo );

extern HalInfo halInfo;

FxBool h3HwcMapBoard( HwcContext *hwc);

FX_ENTRY SstRegs * FX_CALL fxHalMapBoard(FxU32 boardNum);

FxBool h3HwcUnmapBoard( HwcContext *hwc);

FxBool h3HwcShutdownBoard( HwcContext *hwc);

FxBool h3HwcInitRegisters( HwcContext *hwc );

FxBool h3HwcInitRenderingRegisters( HwcContext *hwc);

FxBool h3HwcInitGuiRegisters( HwcContext *hwc);

FxBool h3HwcIdleNoNop( HwcContext *hwc);

FxBool h3HwcInitVideo( HwcContext *hwc, FxU32 resolution, 
                      FxU32 refresh, void *vti);

FxBool h3HwcVideoEnable( HwcContext *hwc, FxBool enable );

FxBool h3HwcIdle( HwcContext *hwc );

FxBool h3HwcVsync( HwcContext *hwc);

FxBool h3HwcVsyncNot( HwcContext *hwc);

FxBool h3HwcInitCmdFifo( HwcContext *hwc, int which, FxU32 fifoStart,
                        FxU32 size, FxBool disableHoles, FxBool agpEnable);

FxBool h3HwcFillDeviceInfo( HwcContext *hwc);

FxBool h3HwcInitGamma( HwcContext *hwc, FxFloat gamma );

FxBool h3HwcInitGammaRGB( HwcContext *hwc, FxFloat r, FxFloat g, FxFloat b);

void h3HwcStore8( HwcContext *hwc, volatile void *addr, FxU8 data );

void h3HwcStore16( HwcContext *hwc, volatile void *addr, FxU16 data );

void h3HwcStore32( HwcContext *hwc, volatile void *addr, FxU32 data );

FxU8 h3HwcLoad8( HwcContext *hwc, volatile void *addr );

FxU16 h3HwcLoad16( HwcContext *hwc, volatile void *addr );

FxU32 h3HwcLoad32( HwcContext *hwc, volatile void *addr );

FxBool h3HwcInitBuffer(HwcBuffer *this);

FxBool h3HwcReadPixel(HwcBuffer *this, FxU32 which, int x, int y, HwcPixel *);

FxBool h3HwcWritePixel(HwcBuffer *this, FxU32 which, int x, int y, HwcPixel *);

FxBool h3HwcSetBuffer(HwcBuffer *this, HwcBufferType type);

#endif /* !__H3HWC_H__ */
