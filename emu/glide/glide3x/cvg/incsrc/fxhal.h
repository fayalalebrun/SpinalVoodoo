#ifndef __FXHAL_H__
#define __FXHAL_H__

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

#if defined(BUILD_HAL)
#define FX_DLL_DEFINITION
#endif
#include <fxdll.h>

#include <cvginfo.h>

// Just to unconfuse myself:
//
//      CHIP            FBI-REV TMU-REV DEV-ID
//      SST1-0.6u       1       0       1
//      SST1-0.5u       2       1       1
//      SST-96          2       (1)     2
//      H3              3       4       4
//	H4		?	?	4?
#define SST_DEVICE_ID_SST1      1
#define SST_DEVICE_ID_SST96     2
#define SST_DEVICE_ID_H3        3

#define MBYTE(n)	(((FxU32)(n))<<20)
#define DEAD		0xDEAD

// Maximum number of boards and TMUs supported
#define HAL_MAX_BOARDS 4

//----------------------------------------------------------------------
// the root of all Hal information
//----------------------------------------------------------------------
typedef struct {
    int csim;		
    int hsim;		
    int hw;		
    FxU32 boardsFound;	// number of boards found
    FxDeviceInfo boardInfo[HAL_MAX_BOARDS];

    int pollLimit;	// number of pixels to poll msg Q after
    int pollCount;	// current pixel counter
    int video;		// video output enabled
    FxU32 csimLastRead;	// holds the last data read
} HalInfo;

//----------------------------------------------------------------------
/*
** SST Hardware Initialization routine protypes
**
** If all initialization routines are called, it is assumed they are called
** in the following order:
**   0. fxHalInit();
**   1. fxHalMapBoard();
**   2. fxHalInitRegisters();
**   3. fxHalInitGamma();
**   4. fxHalInitVideo();
**   5. fxHalShutdown();
**
** fxHalShutdown() is called at the end of an application to turn off
**  the graphics subsystem
**
*/

FX_ENTRY void   FX_CALL fxHalPutenv(char *buf);
FX_ENTRY HalInfo * FX_CALL fxHalInit(FxU32 flags);
FX_ENTRY FxU32  FX_CALL fxHalNumBoardsInSystem(void);
FX_ENTRY SstRegs * FX_CALL fxHalMapBoard(FxU32 boardNum);
FX_ENTRY FxBool FX_CALL fxHalInitCmdFifo( SstRegs *sst, int which, 
                                          FxU32 fifoStart, FxU32 size, 
                                          FxBool directExec, FxBool disableHoles,
                                          FxSet32Proc set32);
FX_ENTRY FxBool FX_CALL fxHalInitRegisters(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalInitRenderingRegisters(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalInitGamma(SstRegs *sst, FxFloat gamma);
FX_ENTRY FxBool FX_CALL fxHalInitGammaRGB(SstRegs *sst, FxFloat r, FxFloat g, FxFloat b);
FX_ENTRY FxBool FX_CALL fxHalInitVideo(SstRegs *sst, FxU32 resolution,
					FxU32 refresh, sst1VideoTimingStruct *);
FX_ENTRY FxBool FX_CALL fxHalIdle(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalIdleNoNOP(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalIdleFBINoNOP( SstRegs *sst );
FX_ENTRY FxBool FX_CALL fxHalShutdown(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalInitUSWC(SstRegs *sst);
FX_ENTRY void   FX_CALL fxHalShutdownAll(void);
FX_ENTRY FxBool FX_CALL fxHalGetDeviceInfo(SstRegs *sst, FxDeviceInfo *);
FX_ENTRY FxBool FX_CALL fxHalInitSLI(SstRegs *sst, SstRegs *sst1);

FX_ENTRY FxBool FX_CALL fxHalVsync(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalVsyncNot(SstRegs *sst);


	#define GET8(s) s
	#define GET16(s) s
	#define GET(s) s
	#define SET8(d,s) d = s
	#define SET16(d,s) d = s
	#define SET(d,s) d = s
	#define SETF(d,s) (*(float *)&(d)) = s


//---------------------------------------------------------------------------
// internal HAL stuff not meant for external use
//---------------------------------------------------------------------------
#if defined(BUILD_HAL) || defined(BUILD_DIAGS)

// GMT: Init code SET/GET always go thru subroutines (allows for P6 fencing)
#define IGET(A)   fxHalRead32((FxU32 *) &(A))
#define ISET(A,D) fxHalWrite32((FxU32 *) &(A), D)

// this is the FAKE address where the hardware lives
// we use a large address so attempts to write to it get an access violation
// and it has 28 zero bits so that we can easily figure out the board number
// and the offset into the board
#define SST_FAKE_ADDRESS_MAKE(i) (SstRegs *)((i+1)<<28)
#define SST_FAKE_ADDRESS_GET_BOARD(a) ((((FxU32)a>>28)&0xF)-1)
#define SST_FAKE_ADDRESS_GET_OFFSET(a) ((FxU32)a&0x0FFFFFFF)
#define SST_FAKE_ADDRESS_GET_BASE(a) ((FxI32)a&~0x0FFFFFFF)

extern HalInfo halInfo;

// internal HAL routines
FxU32 fxHalRead32(FxU32 *addr);
void fxHalWrite32(FxU32 *addr, FxU32 data);

FxBool fxHalIdleFBI( SstRegs *sst );

void   fxHalResetBoardInfo( FxDeviceInfo *info );
FxBool fxHalFillDeviceInfo( SstRegs *sst );
// FxBool fxHalGetFbiInfo( SstRegs *sst, FxDeviceInfo *info );
// FxBool fxHalGetTmuInfo( SstRegs *sst, FxDeviceInfo *info );
FxBool fxHalVaddrToBoardNumber( SstRegs *sst, FxU32 *boardNumber );

// GUI interface
FX_ENTRY void FX_CALL guiNewViewWindow(FxU32 boardNumber, const char *name);
void guiReadMessageQueue(void);
FxBool guiOpen( FxU32 boardNumber );
void guiShutdown( SstRegs *sst );

#endif /* BUILD_HAL */

#endif /* !__FXHAL_H__ */
