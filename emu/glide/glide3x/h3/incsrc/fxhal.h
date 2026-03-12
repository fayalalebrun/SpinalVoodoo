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

#ifndef __FXHAL_H__
#define __FXHAL_H__

#if defined(BUILD_HAL)
#define FX_DLL_DEFINITION
#endif
#include <fxdll.h>
#include <fxpci.h>

#include <h3info.h>

// Just to unconfuse myself:
//
//      CHIP            FBI-REV TMU-REV DEV-ID
//      SST1-0.6u       1       0       1
//      SST1-0.5u       2       1       1
//      SST-96          2       (1)     2
//      H3 A0           1       4       3
//      H3 A1           2       4       3
//      H3 B0           3       4       3
//      H4_OEM          1       4       4
//      H4              1       4       5

#define SST_DEVICE_ID_SST1      1
#define SST_DEVICE_ID_SST96     2
#define SST_DEVICE_ID_H3        3
#define SST_DEVICE_ID_H4_OEM    4
#define SST_DEVICE_ID_H4    	5

#define MBYTE(n)        (((FxU32)(n))<<20)
#define DEAD            0xDEAD

// Maximum number of boards and TMUs supported
#define HAL_MAX_BOARDS 4

//----------------------------------------------------------------------
// the root of all Hal information
//----------------------------------------------------------------------
typedef struct {
    int csim;           
    int hsim;           
    int hw;             // TRUE if real HW is enabled (default)
    int csimio;         
    FxU32 boardsFound;  // number of boards found
    FxDeviceInfo boardInfo[HAL_MAX_BOARDS];

    int pollLimit;      // number of pixels to poll msg Q after
    int pollCount;      // current pixel counter
    int video;          // video output enabled
    FxU32 csimLastRead; 
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
FX_ENTRY FxBool FX_CALL fxHalInitCmdFifo( SstRegs *sst, int which, FxU32 fifoStart,
                  FxU32 size, FxBool directExec, FxBool disableHoles, FxBool agpEnable);
FX_ENTRY FxBool FX_CALL fxHalInitRegisters(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalInitRenderingRegisters(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalInitGuiRegisters(SstGRegs *sstg);
FX_ENTRY FxBool FX_CALL fxHalInitCmdAgpRegisters(SstCRegs *sstc);
FX_ENTRY FxBool FX_CALL fxHalInitGamma(SstRegs *sst, FxFloat gamma);
FX_ENTRY FxBool FX_CALL fxHalInitGammaRGB(SstRegs *sst, FxFloat r, FxFloat g, FxFloat b);
FX_ENTRY FxBool FX_CALL fxHalInitVideo(SstRegs *sst, FxU32 resolution,
                FxU32 refresh, FxVideoTimingInfo *);
FX_ENTRY FxBool FX_CALL fxHalIdle(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalIdleNoNop(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalIdle2(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalIdleNoNop2(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalInitUSWC(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalShutdown(SstRegs *sst);
FX_ENTRY void   FX_CALL fxHalShutdownAll(void);
FX_ENTRY FxBool FX_CALL fxHalGetDeviceInfo(SstRegs *sst, FxDeviceInfo *);

FX_ENTRY FxBool FX_CALL fxHalVsync(SstRegs *sst);
FX_ENTRY FxBool FX_CALL fxHalVsyncNot(SstRegs *sst);

FX_ENTRY void FX_CALL
fxHalInitVideoOverlaySurface(
    SstRegs *sst,               // pointer to hw
    FxU32 enable,               // 1=enable Overlay surface (OS), 1=disable
    FxU32 stereo,               // 1=enable OS stereo, 0=disable
    FxU32 horizScaling,         // 1=enable horizontal scaling, 0=disable
    FxU32 dudx,                 // horizontal scale factor (ignored if not
                                // scaling)
    FxU32 verticalScaling,      // 1=enable vertical scaling, 0=disable
    FxU32 dvdy,                 // vertical scale factor (ignored if not
                                // scaling)
    FxU32 filterMode,           // duh
    FxU32 tiled,                // 0=OS linear, 1=tiled
    FxU32 pixFmt,               // pixel format of OS
    FxU32 clutBypass,           // bypass clut for OS?
    FxU32 clutSelect,           // 0=lower 256 CLUT entries, 1=upper 256
    FxU32 startAddress,         // board address of beginning of OS
    FxU32 stride);              // distance between scanlines of the OS, in
                                // units of bytes for linear OS's and
                                // tiles for tiled OS's
#if !defined(MSVC16) && !defined(THUNK32) && !defined(DIRECTX)

extern int DRIlock;

// Here are the macro defines for talking to hardware
        #define GET8(s) s
        #define GET16(s) s
        #define GET(s) s
        #define SET8(d,s) d = s
        #define SET16(d,s) d = s
        #define SET(d,s) d = s
        #define SETF(d,s) (*(float *)&(d)) = s
        #define SET_FIFO(d,s) \
	        do { \
	                d = s; \
	        } while (0)

        #define SETF_FIFO(d, s) (*(float *)&(d)) = s
		
        #define AGPMEMINIT()       GDBG_ERROR("AGPMEMINIT","nyi for hardware\n");
        #define AGPMEMALLOC(sst,size)     GDBG_ERROR("AGPMEMALLOC","nyi for hardware\n");
        #define AGPWRV(v,d)        (v) = (d)
        #define AGPRDV(v)          (v)
        #define AGPWRP(aHi,aLo,d)  AGPWRV( *agpPhysToVirt(aHi,aLo), d )
        #define AGPRDP(aHi,aLo)    AGPRDP( *agpPhysToVirt(aHi,aLo) )


#if SET_SWIZZLEHACK
#undef GET
#undef GET16
#undef SET
#undef SET16
#undef SETF
#undef SET_FIFO
#undef SETF_FIFO

extern FxU32 swizzleRead32(volatile FxU32 *s);
extern FxU16 swizzleRead16(volatile FxU16 *s);
extern void  swizzleWrite32(volatile FxU32 *d, FxU32 s);
extern void  swizzleWrite16(volatile FxU16 *d, FxU16 s);
extern void  swizzleWriteF(volatile FxU32 *d, FxFloat s);

#define GET(s)					swizzleRead32(&(s))
#define GET16(s)				swizzleRead16((FxU16 *)&(s))
#define SET(d, s)				swizzleWrite32(&(d),(s))
#define SET_FIFO(d, s)			swizzleWrite32(&(d),(s))
#define SET16(d, s)				swizzleWrite16((FxU16 *)&(d),(s))
#define SETF(d, s)				swizzleWriteF((volatile FxU32 *)&(d),(s))
#define SETF_FIFO(d, s)			swizzleWriteF((volatile FxU32 *)&(d),(s))

#elif SET_BSWAP
#undef GET
#undef GET16
#undef SET
#undef SET16
#undef SETF
#undef SET_FIFO
#undef SETF_FIFO

#if __POWERPC__ && defined(__MWERKS__)
#define GET(s)               __lwbrx( (void*)&(s), 0 )
#define GET16(s)             __lhbrx( (void*)&(s), 0 )
#define SET(d, s)            __stwbrx((s), (void*)&(d), 0)
#define SET16(d, s)          __sthbrx((s), (void*)&(d), 0 )
#define SETF(d, s) \
                             { \
                                const float temp = (s); \
                                __stwbrx( *((FxU32*)&temp), (void*)&(d), 0 ); \
                             }
#if PCI_BUMP_N_GRIND
inline void setFifo(FxU32 *d, FxU32 s) {
	if(((FxU32)d & 31) == 0) {
		__dcbz(d,0);
		__dcbf(d,-4);
	}
	__stwbrx(s,d,0);
}

inline void setFifoF(FxU32 *d, float s) {
	if(((FxU32)d & 31) == 0) {
		__dcbz(d,0);
		__dcbf(d,-4);
	}
	__stwbrx(*(FxU32 *)&s,d,0);
}

inline void setFifo8(FxU32 *d, FxU32 s) {
	if(((FxU32)d & 31) == 0) {
		__dcbz(d,0);
		__dcbf(d,-4);
	}
	*d = s;
}

#define SET_FIFO(d, s)		setFifo((FxU32 *)&(d), s)
#define SETF_FIFO(d, s)		setFifoF((FxU32 *)&(d), (s))
#else
#define SET_FIFO(d, s)            __stwbrx((s), (void*)&(d), 0)
#define SETF_FIFO(d, s) \
                             { \
                                const float temp = (s); \
                                __stwbrx( *((FxU32*)&temp), (void*)&(d), 0 ); \
                             }
#endif                             
#else /* !defined(__MWERKS__) && POWERPC */
#error "Define byte swapped macros for GET/SET"
#endif /* !defined(__MWERKS__) && POWERPC */
#endif /* SET_BSWAP */

#define GET_IO8(p)      pioInByte(p)
#define GET_IO16(p)     pioInWord(p)
#define GET_IO(p)       pioInLong(p)
#define SET_IO8(p,d)    pioOutByte(p,d)
#define SET_IO16(p,d)   pioOutWord(p,d)
#define SET_IO(p,d)     pioOutLong(p,d)

#endif // #if !defined(MSVC16) && !defined(THUNK32) && !defined(DIRECTX)

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
#define SST_BAD_ADDRESS(a) (((FxU32)a&0xF0000000)==0 || ((FxU32)a&0x0C000000)!=0)
#define SST_FAKE_ADDRESS_MAKE(boardNum) (SstRegs *)(0x200000|((boardNum+1)<<28))
#define SST_FAKE_ADDRESS_GET_BOARD(a) ((((FxU32)a>>28)&0xF)-1)

#define SST_FAKE_ADDRESS_GET_OFFSET(a) ((FxU32)a&0x0FFFFFFF)
#define SST_FAKE_ADDRESS_GET_BASE_OFFSET(a) ((FxU32)a&0x01FFFFFF)
#define SST_FAKE_ADDRESS_GET_BASE(a) ( ((FxU32)a&BIT(25))>>25 )

#define SST_BAD_PORT(a) (((FxU32)a&0xF000)==0)
#define SST_FAKE_PORT_MAKE(boardNum) (FxU16)((boardNum+1)<<12)
#define SST_FAKE_PORT_GET_BOARD(a) ((((FxU16)a>>12)&0xF)-1)

#define SST_FAKE_PORT_GET_OFFSET(a) ((FxU16)a&0x0FFF)

extern HalInfo halInfo;

// internal HAL routines
FxU32 fxHalRead32(FxU32 *addr);
void fxHalWrite32(FxU32 *addr, FxU32 data);

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
