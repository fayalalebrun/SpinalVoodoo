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
*/

#ifdef DCON
#undef DCON
#endif

#define DCON 0

#include <3dfx.h>
#include <gdebug.h>

#include <h3cinit.h>
#include <minihwc.h>

#include "setmode.h"

#define __MACERRORS_
#include <Devices.h>
#include <Displays.h>
#include <GraphicsPriv.h>
#include <GraphicsPrivHwc.h>
#include <DriverServices.h>
#include <hdwr_res_mgr.h>
#include <hrm_fifo.h>
#include <hrm_mode.h>
#include <hrm_mem.h>
#include <DCon.h>
#include <AGP.h>
#include <CodeFragments.h>

hrmGetVersionInfoPtr _hrmGetVersionInfo;
hrmSetExclusiveModePtr _hrmSetExclusiveMode;
hrmSetVideoModePtr _hrmSetVideoMode;
hrmReleaseExclusiveModePtr _hrmReleaseExclusiveMode;    
hrmAllocateBlockPtr _hrmAllocateBlock;
hrmFreeBlockPtr _hrmFreeBlock;
hrmAllocWinContextPtr _hrmAllocWinContext;
hrmFreeWinContextPtr _hrmFreeWinContext;
hrmExecuteWinFifoPtr _hrmExecuteWinFifo;
hrmGetDeviceConfigPtr _hrmGetDeviceConfig;
hrmGetAGPInfoPtr _hrmGetAGPInfo;
hrmWriteConfigRegisterPtr _hrmWriteConfigRegister;
hrmReadConfigRegisterPtr _hrmReadConfigRegister;
hrmGetSlaveRegsPtr _hrmGetSlaveRegs;
hrmSLIAAPtr _hrmSLIAA;

#if GLIDE_BYPASS_DISPLAY_MANAGER
static VDSwitchInfoRec oldModeSwitchInfo;
#endif

static  WindowPtr glideWin;

/* Information used to restore video upon forceful exit */
static  hrmBoard_t         *video_board;
static 	FxBool videoNeedsToBeRestored;
static void CleanupAGP(void);
static AGPAddressPair fifoAddress;
static FxU32 fifoAddressValid;

FxBool 
setVideoMode( void* hwnd, 
			  int 	xRes, int yRes, 
			  int   pixelSize,
			  int 	refresh, 
			  void* hmon ) 
{
  dprintf("hrmSetExclusiveMode(%08lx)\n",hmon);
  
  if(_hrmSetExclusiveMode((hrmBoard_t *)hmon)) {
    if(noErr == _hrmSetVideoMode((hrmBoard_t *)hmon, xRes, yRes, refresh)) {
      videoNeedsToBeRestored = FXTRUE;
      video_board = (hrmBoard_t *)hmon;
      return FXTRUE;
    }
    _hrmReleaseExclusiveMode((hrmBoard_t *)hmon);
    videoNeedsToBeRestored = FXFALSE;
    video_board = 0;
  }
  return FXFALSE;
}

void resetVideo(void)
{
  if(videoNeedsToBeRestored)
  {
    /* Just in case something bad happened, don't call a bogus function. */
    if(_hrmReleaseExclusiveMode) {
        dprintf("calling hrmReleaseExclusiveMode(%08lx)\n",video_board);
	    _hrmReleaseExclusiveMode(video_board);
	    dprintf("hrmReleaseExclusiveMode returned\n");
	    videoNeedsToBeRestored = FXFALSE;
	    video_board = 0;
	}
  }
  #if 0
  if(fifoAddressValid) {
    CleanupAGP();
  }
  #endif
}


hrmBoard_t * hwcBoardInfo2hrmBoard( hwcBoardInfo *bInfo )
{
  hrmBoard_t * hrmBoard;
  hrmBoardInfo_t hrmBInfo;
  short i = 0;
  
  while ( (hrmBoard = hrmGetTargetAtIndex(i++)) != 0 )
  {
    hrmGetTargetBoardInfoExt( hrmBoard, &hrmBInfo);

    if ( bInfo->pciInfo.pciBaseAddr[0] == hrmBInfo.pciBaseAddr[0] )
      break;
  }
  
  return hrmBoard;
}


#if 0
OSErr GetAGPInfo(FxU32 *agpLAddr, FxU32 *agpPAddr, FxU32 *agpSize)
{
  OSErr err;
  
  /* Make sure AGP is around. */
  if((long)AGPNewMemory == (long)kUnresolvedCFragSymbolAddress) {
    return -1;
  }
  
  if(!fifoAddressValid) {
    err = AGPNewMemory(&fifoAddress, 1024*1024, false);
    dprintf("AGPNewMemory returned: %d\n",err);
    if(err != noErr) {
      return -1;
    }
    err = AGPCommitMemory(&fifoAddress, false);
    dprintf("AGPCommitMemory returned: %d\n",err);
    if(err != noErr) {
      AGPDisposeMemory(&fifoAddress);
      return -1;
    }
    fifoAddressValid = 1;
  }
  
  *agpLAddr = (FxU32)fifoAddress.systemLogicalAddress;
  *agpPAddr = (FxU32)fifoAddress.agpLogicalAddress;
  *agpSize = 1024*1024;
  
  dprintf("agpLAddr: %08lx agpPAddr: %08lx\n",*agpLAddr,*agpPAddr);
     
  return 0;
}

static void CleanupAGP(void)
{
  dprintf("CleanupAGP: %d\n",fifoAddressValid);
  if(fifoAddressValid) {
    dprintf("Cleaning up AGP memory\n");
    AGPDecommitMemory(&fifoAddress);
    AGPDisposeMemory(&fifoAddress);
    fifoAddressValid = 0;
  }
}
#endif

#if PCI_BUMP_N_GRIND    
static FxU32 saveCopybackAddr, saveCopybackLength;
static FxU32 cacheSettingsNeedToBeRestored = FXFALSE;

void enableCopyBackCache(FxU32 addr, FxU32 length)
{
    OSErr err;
	saveCopybackAddr = addr;
	saveCopybackLength = length;
	FlushProcessorCache(kCurrentAddressSpaceID, (LogicalAddress)addr, length);
	err = SetProcessorCacheMode(kCurrentAddressSpaceID, (const void *)addr, length, kProcessorCacheModeCopyBack);
	cacheSettingsNeedToBeRestored = FXTRUE;
}

void restoreCacheSettings(void)
{
	if(cacheSettingsNeedToBeRestored)
	{
		FlushProcessorCache(kCurrentAddressSpaceID, (LogicalAddress)saveCopybackAddr, 
			saveCopybackLength);
		SetProcessorCacheMode(kCurrentAddressSpaceID, 
			(const void *)saveCopybackAddr,
			saveCopybackLength,
			kProcessorCacheModeInhibited);
		cacheSettingsNeedToBeRestored = FXFALSE;
	}
}
#endif

#if defined(__MWERKS__) || defined(__MRC__)
/* MW linker generated cleanup stub */

extern void resetVideo(void);
extern void gdxSurfaceInit();
extern void gdxSurfaceShutdown();

extern pascal OSErr myInitialize(const CFragInitBlock *	initBlock)
{
  OSErr err;
  err = __initialize(initBlock);
  gdxSurfaceInit();
  return err;
}

extern pascal void myCodeFragCleanup(void)
{
	/* Restore display mode if it didn't get done already. */
#if GLIDE3	
	gdxSurfaceShutdown();
#endif	
    dprintf("myCodeFragCleanup\n");
#if PCI_BUMP_N_GRIND    
	restoreCacheSettings();
#endif	
	resetVideo();
	/* CleanupAGP(); */
	__terminate();
}
#else /* !__MWERKS__ */
#error "Need CFM cleanup proc for this compiler"
#endif /* !__MWERKS__ */


extern FxU32 swizzleRead32(volatile FxU32 *s);
extern FxU16 swizzleRead16(volatile FxU16 *s);
extern void  swizzleWrite32(volatile FxU32 *d, FxU32 s);
extern void  swizzleWrite16(volatile FxU16 *d, FxU16 s);
extern void  swizzleWriteF(volatile FxU32 *d, FxFloat s);
extern void  swizzleWriteLinear8(volatile FxU32 *d, FxU32 s);

FxU32 (*__swizzleRead32)(volatile FxU32 *s);
FxU16 (*__swizzleRead16)(volatile FxU16 *s);
void (*__swizzleWrite32)(volatile FxU32 *d, FxU32 s);
void (*__swizzleWrite16)(volatile FxU16 *d, FxU16 s);
void (*__swizzleWriteF)(volatile FxU32 *d, FxFloat s);
void (*__swizzleWriteLinear8)(volatile FxU32 *d, FxU32 s);

FxU32 __lfbBase = 0;
FxU32 __regBase = 0;

/* 8-bit desktop mode, LFB accesses are not swizzled, so */
/* we have to swizzle everything ourselves */
FxU32 __swizzleRead32_8(volatile FxU32 *s)
{
	return __lwbrx((void *)s, 0);
}

FxU16 __swizzleRead16_8(volatile FxU16 *s)
{
	return __lhbrx((void *)s, 0);
}

void  __swizzleWrite32_8(volatile FxU32 *d, FxU32 s)
{
	__stwbrx(s,(void *)d, 0);
}

void  __swizzleWrite16_8(volatile FxU16 *d, FxU16 s)
{
	__sthbrx(s,(void *)d, 0);
}

void  __swizzleWriteF_8(volatile FxU32 *d, FxFloat s)
{
	__stwbrx(*(FxU32 *)&s,(void *)d,0);
}

void  __swizzleWriteLinear8_8(volatile FxU32 *d, FxU32 s)
{
	*d = s;
}

/* 16-bit desktop mode.  The hardware effectively does 
    byte swizzles within 16-bit words. */

/* For 32-bit acceses, we have to word swap */
FxU32 __swizzleRead32_16(volatile FxU32 *s)
{
	FxU32 temp = *s;
	return (temp >> 16) | (temp << 16);
}

/* For 16-bit acceses, the hardware will byte swap for us */
FxU16 __swizzleRead16_16(volatile FxU16 *s)
{
	return *s;
}

/* Swap words */
void  __swizzleWrite32_16(volatile FxU32 *d, FxU32 s)
{
	FxU32 temp = (s >> 16) | (s << 16);
	*d = temp;
}

/* Pass through */
void  __swizzleWrite16_16(volatile FxU16 *d, FxU16 s)
{
	*d = s;
}

/* Swap words */
void  __swizzleWriteF_16(volatile FxU32 *d, FxFloat s)
{
	FxU32 temp = *(FxU32 *)&s;
	temp = (temp >> 16) | (temp << 16);
	*d = temp;
}

/* Ewww. Swap bytes within word */
void  __swizzleWriteLinear8_16(volatile FxU32 *d, FxU32 s)
{
	FxU32 temp = *(FxU32 *)&s;
	temp = (temp >> 16) | (temp << 16);
	__stwbrx(temp,(void *)d, 0);
}

/* 32-bit desktop mode.  The hardware does full byte swapping */
/* within 32-bit longs */

/* Pass through */
FxU32 __swizzleRead32_32(volatile FxU32 *s)
{
	return *s;
}

/* Must do address swizzling, hardware will do the byte swap */
FxU16 __swizzleRead16_32(volatile FxU16 *s)
{
	FxU32 temp = (FxU32)s;
	temp ^= 2;  // Swap word address
	return *(FxU16 *)temp;
}

/* Pass through */
void  __swizzleWrite32_32(volatile FxU32 *d, FxU32 s)
{
	*d = s;
}

/* Swizzle address */
void  __swizzleWrite16_32(volatile FxU16 *d, FxU16 s)
{
	FxU32 temp = (FxU32)d;
	temp ^= 2;
	*(FxU16 *)temp = s;
}

/* Pass through */
void  __swizzleWriteF_32(volatile FxU32 *d, FxFloat s)
{
	*(FxFloat *)d = s;
}

/* Byte swap */
void  __swizzleWriteLinear8_32(volatile FxU32 *d, FxU32 s)
{
	__stwbrx(s,(void *)d, 0);
}

void setLfbSwizzleMode(FxU32 lfbBase, FxU32 regBase, FxU32 swizzleMode)
{
	__lfbBase = lfbBase;
	__regBase = regBase;
	
	switch(swizzleMode)
	{
		case 8:
		{
			__swizzleRead32 = __swizzleRead32_8;
			__swizzleRead16 = __swizzleRead16_8;
			__swizzleWrite32 = __swizzleWrite32_8;
			__swizzleWrite16 = __swizzleWrite16_8;
			__swizzleWriteF = __swizzleWriteF_8;
			__swizzleWriteLinear8 = __swizzleWriteLinear8_8;
		}
		break;
		
		case 16:
		{
			__swizzleRead32 = __swizzleRead32_16;
			__swizzleRead16 = __swizzleRead16_16;
			__swizzleWrite32 = __swizzleWrite32_16;
			__swizzleWrite16 = __swizzleWrite16_16;
			__swizzleWriteF = __swizzleWriteF_16;
			__swizzleWriteLinear8 = __swizzleWriteLinear8_16;
		}
		break;
		
		case 32:
		{
			__swizzleRead32 = __swizzleRead32_32;
			__swizzleRead16 = __swizzleRead16_32;
			__swizzleWrite32 = __swizzleWrite32_32;
			__swizzleWrite16 = __swizzleWrite16_32;
			__swizzleWriteF = __swizzleWriteF_32;
			__swizzleWriteLinear8 = __swizzleWriteLinear8_32;
		}
		break;
	}
}

extern FxU32 swizzleRead32(volatile FxU32 *s)
{
	if((__lfbBase == ((FxU32)s & 0xFE000000)))
	{	
		return __swizzleRead32(s);
	}
	return __lwbrx( (void *)s, 0 );
}

extern FxU16 swizzleRead16(volatile FxU16 *s)
{
	if((__lfbBase == ((FxU32)s & 0xFE000000)))
	{
		return __swizzleRead16(s);
	}
	return __lhbrx( (void *)s, 0 );
}

extern void swizzleWrite32(volatile FxU32 *d, FxU32 s)
{
	if((__lfbBase == ((FxU32)d & 0xFE000000)))
	{
		__swizzleWrite32(d,s);
		return;
	}
	__stwbrx( s, (void *)d, 0);
}

extern void swizzleWrite16(volatile FxU16 *d, FxU16 s)
{
	if((__lfbBase == ((FxU32)d & 0xFE000000)))
	{
		__swizzleWrite16(d,s);
		return;
	}
	__sthbrx(s, (void *)d, 0);
}

extern void swizzleWriteF(volatile FxU32 *d, FxFloat s)
{
	if((__lfbBase == ((FxU32)d & 0xFE000000)))
	{
		__swizzleWriteF(d,s);
		return;
	}
	__stwbrx(*(FxU32 *)&s, (void *)d, 0);
}

extern void  swizzleWriteLinear8(volatile FxU32 *d, FxU32 s)
{
	if((__lfbBase == ((FxU32)d & 0xFE000000)))
	{
		__swizzleWriteLinear8(d,s);
		return;
	}
	*d = s;
}
