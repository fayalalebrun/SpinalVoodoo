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
#include <fxpci.h>

#include <h3info.h>

/*
 * Just to confuse you:
 * CHIP            FBI-REV TMU-REV DEV-ID
 * SST1-0.6u       1       0       1
 * SST1-0.5u       2       1       1
 * SST-96          2       (1)     2
 * H3 A0           1       4       3
 * H3 A1           2       4       3
 * H3 B0           3       4       3
 * H4 (slow)       ?       ?       4
 * H4              ?       ?       5
 * H4+             ?       ?       6-9
 * H4+2 (LC)       ?       ?       A-F
 */
#define SST_DEVICE_ID_SST1      1
#define SST_DEVICE_ID_SST96     2
#define SST_DEVICE_ID_H3        3
#define SST_DEVICE_ID_H4    	4
#define SST_DEVICE_ID_H4_OEM    5
#define SST_DEVICE_ID_AP        6
#define SST_DEVICE_ID_L_AP      6
#define SST_DEVICE_ID_AP_OEM    9
#define SST_DEVICE_ID_H_AP     15


#define MBYTE(n)        (((FxU32)(n))<<20)
#define DEAD            0xDEAD

// Maximum number of boards and TMUs supported
#define HAL_MAX_BOARDS 4

//----------------------------------------------------------------------
// the root of all Hal information
//----------------------------------------------------------------------
typedef struct {
    int csim;           // TRUE if CSIM is enabled
    int hsim;           // TRUE if HSIM is enabled (actually a bitmask)
    int hw;             // TRUE if real HW is enabled (default)
    int csimio;         // TRUE if CSIM should intercept and mirror all HW/HSIM accesses
    FxU32 boardsFound;  // number of boards found
    FxDeviceInfo boardInfo[HAL_MAX_BOARDS];

    int pollLimit;      // number of pixels to poll msg Q after
    int pollCount;      // current pixel counter
    int video;          // video output enabled
    FxU32 csimLastRead; // holds the last data read (from CSIM)
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
// These are the CSIM interface routines
#ifdef HAL_CSIM 
        FX_ENTRY void FX_CALL csimInitDriver( FxU32 numBytes,
                        volatile FxU32 *memory, volatile FxU32 *hw );
        FX_ENTRY void FX_CALL csimInitMemory( FxU32 numBytes, volatile FxU32 *memory );
        FX_ENTRY void FX_CALL csimInitHwAddress( volatile FxU32 *hw );

        FX_ENTRY void  FX_CALL halStore8 (volatile void *, FxU8);
        FX_ENTRY void  FX_CALL halStore16(volatile void *, FxU16);
        FX_ENTRY void  FX_CALL halStore32(volatile void *, FxU32);
        FX_ENTRY void  FX_CALL halStore32f(volatile void *, FxFloat);
        FX_ENTRY FxU8  FX_CALL halLoad8 (volatile void *);
        FX_ENTRY FxU16 FX_CALL halLoad16(volatile void *);
        FX_ENTRY FxU32 FX_CALL halLoad32(volatile void *);

        FX_ENTRY FxU32 FX_CALL halCfgLoad32(FxU16 port, int mechanism, 
					    FxU32 bus_number, FxU32 device_number, 
					    FxU32 function_number, FxU32 register_offset);
        FX_ENTRY void FX_CALL halCfgStore32(FxU16 port, int mechanism,
					    FxU32 bus_number, FxU32 device_number, 
					    FxU32 function_number, FxU32 register_offset, 
					    FxU32 data); 

        // GMT: these no longer need to be DLL entry points since we
        // get to them via callbacks
        void  halOutPort8 (FxU16 port, FxU8 data);
        void  halOutPort16(FxU16 port, FxU16 data);
        void  halOutPort32(FxU16 port, FxU32 data);
        FxU8  halInPort8  (FxU16 port);
        FxU16 halInPort16 (FxU16 port);
        FxU32 halInPort32 (FxU16 port);

        // should be put into a driver
        FX_ENTRY void FX_CALL agpMemInit();
        FX_ENTRY FxU32 * FX_CALL agpMemAlloc(SstRegs *sst,unsigned sizeBytes);
        FX_ENTRY void  FX_CALL agpWriteMem32( FxU32 *virtAddr, FxU32 value);
        FX_ENTRY FxU32 FX_CALL agpReadMem32( FxU32 *virtAddr);
        FX_ENTRY void  FX_CALL agpPhysWriteMem32( FxU32 physAddrHi, FxU32 physAddrLo , FxU32 value);
        FX_ENTRY FxU32 FX_CALL agpPhysReadMem32(  FxU32 physAddrHi, FxU32 physAddrLo );
        FX_ENTRY FxU32 * FX_CALL agpPhysToVirt( FxU32 physAddrHi, FxU32 physAddrLo );
        FX_ENTRY void  FX_CALL agpVirtToPhys( FxU32 *virtAddr, FxU32 *physAddrHi, FxU32 *physAddrLo ); 
#endif

#if !defined(MSVC16) && !defined(THUNK32) && !defined(DIRECTX)

// Here are the macro defines for talking to hardware
#ifdef HAL_CSIM                 // CSIM macros
        #define GET8(s)         halLoad8 (&(s))
        #define GET16(s)        halLoad16(&(s))
        #define GET(s)          halLoad32(&(s))

        #define SET8(d,s)       halStore8 (&(d),s)
        #define SET16(d,s)      halStore16(&(d),s)
        #define SET(d,s)        halStore32(&(d),s)
        #define SETF(d,s)       halStore32f(&(d),s)
        #define SET_FIFO(d,s)   halStore32(&(d),s)
        #define SETF_FIFO(d,s)  halStore32f(&(d),s)

        #define AGPMEMINIT()       agpMemInit()
        #define AGPMEMALLOC(sst,size)       agpMemAlloc(sst,size)
        #define AGPWRV(v,d)        agpWriteMem32(&(v),d)
        #define AGPRDV(v)          agpReadMem32(&(v))
        #define AGPWRP(aHi,aLo,d)  AGPWRV( *agpPhysToVirt(aHi,aLo), d )
        #define AGPRDP(aHi,aLo)    AGPRDV( *agpPhysToVirt(aHi,aLo) )
#else  // #ifdef HAL_CSIM                          // REAL hw
        #define GET8(s) s
        #define GET16(s) s
        #define GET(s) s
        #define SET8(d,s) d = s
        #define SET16(d,s) d = s
        #define SET(d,s) d = s
        #define SETF(d,s) (*(float *)&(d)) = s
        #define SET_FIFO(d,s) d = s
        #define SETF_FIFO(d, s) (*(float *)&(d)) = s

        #define AGPMEMINIT()       GDBG_ERROR("AGPMEMINIT","nyi for hardware\n");
        #define AGPMEMALLOC(sst,size)     GDBG_ERROR("AGPMEMALLOC","nyi for hardware\n");
        #define AGPWRV(v,d)        (v) = (d)
        #define AGPRDV(v)          (v)
        #define AGPWRP(aHi,aLo,d)  AGPWRV( *agpPhysToVirt(aHi,aLo), d )
        #define AGPRDP(aHi,aLo)    AGPRDP( *agpPhysToVirt(aHi,aLo) )
#endif // #ifdef HAL_CSIM

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

#if __POWERPC__ && (defined(__MWERKS__) || defined(__MRC__))
/* MrC is very aggressive and will even optimize out calls to intrinsics. */
#if defined(__MRC__)
#define GET(s)               __lwbrx( (volatile void*)&(s), 0 )
#define GET16(s)             __lhbrx( (volatile void*)&(s), 0 )
#else
#define GET(s)               __lwbrx( (volatile void*)&(s), 0 )
#define GET16(s)             __lhbrx( (volatile void*)&(s), 0 )
#endif
#define SET(d, s)            __stwbrx((s), (void*)&(d), 0)
#define SET16(d, s)          __sthbrx((s), (void*)&(d), 0 )
#define SETF(d, s)           { \
                               volatile union { float f; FxU32 u; } _u; \
                               _u.f = (s); \
                               __stwbrx( _u.u, (void*)&(d), 0 ); \
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
	volatile union { float f; FxU32 u; } _u;
    _u.f = s;
	if(((FxU32)d & 31) == 0) {
		__dcbz(d,0);
		__dcbf(d,-4);
	}
	__stwbrx(_u.u,d,0);
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
#define SET_FIFO(d, s)      __stwbrx((s), (void*)&(d), 0)
#define SETF_FIFO(d, s)     { \
                              volatile union { float f; FxU32 u; } _u; \
                              _u.f = (s); \
                              __stwbrx( _u.u, (void*)&(d), 0 ); \
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
#define SST_BAD_ADDRESS(a) (((FxU32)a&0xF0000000)==0)
#define SST_FAKE_ADDRESS_MAKE(boardNum) (SstRegs *)(0x200000|((boardNum+1)<<28))
#define SST_FAKE_ADDRESS_GET_BOARD(a) ((((FxU32)a>>28)&0xF)-1)
#define SST_FAKE_ADDRESS_GET_CSIM(a) \
                halInfo.boardInfo[SST_FAKE_ADDRESS_GET_BOARD(a)].sstCSIM
#define SST_FAKE_ADDRESS_GET_OFFSET(a) ((FxU32)a&0x0FFFFFFF)
#define SST_FAKE_ADDRESS_GET_BASE_OFFSET(a) ((FxU32)a&0x07FFFFFF)

//This is an f'ed up hack that is necessitated by the CSIM/DIAGs inability
//to actually deal with two physical address spaces. Both of them need
//to be tied together in one large contiguous space
//#define SST_FAKE_ADDRESS_GET_BASE(a) (0)
#define SST_FAKE_ADDRESS_GET_BASE(a) ((((FxU32)a) & 0x08000000) ? 1 : 0)


#define SST_BAD_PORT(a) (((FxU32)a&0xF000)==0)
#define SST_FAKE_PORT_MAKE(boardNum) (FxU16)((boardNum+1)<<12)
#define SST_FAKE_PORT_GET_BOARD(a) ((((FxU16)a>>12)&0xF)-1)
#define SST_FAKE_PORT_GET_CSIM(a) \
                halInfo.boardInfo[SST_FAKE_PORT_GET_BOARD(a)].sstCSIM
#define SST_FAKE_PORT_GET_OFFSET(a) ((FxU16)a&0x0FFF)

// HSIM flags, passed in from the diag environment
#define HSIM_HW_SIMULATION              BIT(0)
#define HSIM_TREX_STANDALONE            BIT(1)
#define HSIM_TREX_BACKDOOR_TEXWRITES    BIT(2)
#define HSIM_SWIZZLE                    BIT(3)

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
FX_ENTRY void FX_CALL guiUpdateStatusBar(void *);
void guiReadMessageQueue(void);
FxBool guiOpen( FxU32 boardNumber );
void guiShutdown( SstRegs *sst );

#endif /* BUILD_HAL */

#endif /* !__FXHAL_H__ */
