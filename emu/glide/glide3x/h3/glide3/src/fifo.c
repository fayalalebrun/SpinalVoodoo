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
 ** Revision 1.1.1.1.6.6  2005/06/09 18:32:28  jwrdegoede
 ** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
 **
 ** Revision 1.1.1.1.6.5  2005/05/25 08:56:23  jwrdegoede
 ** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
 **
 ** Revision 1.1.1.1.6.4  2004/10/05 14:47:15  dborca
 ** conditional compilation a bit more sane
 **
 ** Revision 1.1.1.1.6.3  2004/10/04 09:26:30  dborca
 ** DOS/OpenWatcom support
 **
 ** Revision 1.1.1.1.6.2  2003/11/07 13:38:38  dborca
 ** unite the clans
 **
 ** Revision 1.1.1.1.6.1  2003/05/05 07:12:46  dborca
 ** no message
 **
 ** Revision 1.1.1.1  1999/11/24 21:44:55  joseph
 ** Initial checkin for SourceForge
 **
** 
** 6     4/16/99 4:27p Kcd
** SET*_FIFO macros.
** 
** 32    4/16/99 2:45p Kcd
** PCI Bump & Grind stuff for the Mac.
** 
** 31    4/10/99 1:23p Atai
** fixed code to compile in packet fifo mode 
** 
** 30    4/04/99 8:51p Atai
** Partial check-in for alt-tab issue. set FX_GLIDE_ALT_TAB=1 to build
** glide3x with hwcQueryContext built into GR_BEGIN_NOFIFOCHECK. It works
** with DEBUG glide only. In the non-debug glide, we can still see the
** desktop corruption.
** 
** 28    4/01/99 7:55p Peter
** made names and comments more meaningful
** 
** 27    3/19/99 11:46a Peter
** fifo info should validate stte
** 
** 26    3/19/99 11:26a Peter
** expose direct fifo for gl
** 
** 25    3/14/99 1:47p Peter
** cmd's bng optimization, really invoke gggoph
** 
** 24    3/09/99 12:32p Kcd
** New & Improved tail chasing fifo for windowed Glide.
** 
** 23    3/05/99 10:33p Peter
** fixed the surface fifo state race condition (thanks to Ken Dyke)
** 
** 22    3/04/99 4:34p Kcd
** Conservative fix to prevent command buffers from being used before they
** have been issued.
** 
** 21    3/02/99 2:04p Peter
** made error tracing output use consistent address bases for the fifo
** output
** 
** 20    2/18/99 3:13p Kcd
** Fixed direct register accesses.
** 
** 19    2/11/99 1:38p Atai
** sync buffer swap pending code, the right way.
** 
** 18    2/02/99 4:39p Peter
** cleaned up wax commands, re-issue of commands needs to update fifo room
** 
** 17    1/04/99 12:03p Peter
** use window contexts for sending state
** 
** 16    12/23/98 2:01p Peter
** nt currently has mutexing problems via ddraw and extescape
** 
** 15    12/11/98 1:36p Peter
** made grFlush's operation more obvious
** 
** 14    11/30/98 6:57p Peter
** video memory fifo's
** 
** 13    10/20/98 8:28p Peter
** shared files and fixed whackage
** 
** 12    9/11/98 10:45p Jdt
** switch over to statically allocated in-memory fifo.
** 
** 11    8/03/98 6:38a Jdt
** moved stats into gc
** 
** 10    7/18/98 1:45p Jdt
** Removed TACO_MEMORY_FIFO_HACK
** 
** 9     7/18/98 12:24a Jdt
** Added primitive state restoration buffer.
** 
** 
** 8     7/16/98 8:15a Jdt
** Working 1-window command fifo ( TACO_MEMORY_FIFO_HACK )
** 
** 7     7/05/98 5:05p Jdt
** Workaround for fifo strangeness.
** 
** 6     7/01/98 12:40p Jdt
** protected hacks for Glide/Win ( FX_TACO_MEMORY_FIFO_HACK ) XX temporary
** 
** 5     4/30/98 5:01p Peter
** first pass glide3 merge
** 
** 3     4/22/98 4:57p Peter
** glide2x merge
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 18    12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 17    12/09/97 12:20p Peter
 * mac glide port
 * 
 * 16    12/09/97 10:28a Peter
 * cleaned up some frofanity
 * 
 * 15    12/05/97 4:26p Peter
 * watcom warnings
 * 
 * 14    12/03/97 11:34a Peter
 * dos debugging
 * 
 * 13    11/21/97 3:53p Peter
 * reset messages are controlled by gdbg_level
 * 
 * 12    11/19/97 6:04p Peter
 * actually exit if not reset
 * 
 * 11    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 10    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 9     11/15/97 9:20p Peter
 * I am the sorriest f*cker on the face of the planet
 * 
 **
 */

#include <stdio.h>
#include <string.h>

#if defined(__WIN32__)
#include <windows.h>
#endif

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"


#if GDBG_INFO_ON

static const char* h3SstRegNames[] = {
  "status",
  "intrCtrl",
  "vAx",
  "vAy",
  "vBx",
  "vBy",
  "vCx",
  "vCx",
  "r",
  "g",
  "b",
  "z",
  "a",
  "s",
  "t",
  "w",
  "drdx",
  "dgdx",
  "dbdx",
  "dzdx",
  "dadx",
  "dsdx",
  "dtdx",
  "dwdx",
  "drdy",
  "dgdy",
  "dbdy",
  "dzdy",
  "dady",
  "dsdy",
  "dtdy",
  "dwdy",
  "triangleCMD",
  "reservedA",
  "FvAx",
  "FvAy",
  "FvBx",
  "FvBy",
  "FvCx",
  "FvCy",
  "Fr",
  "Fg",
  "Fb",
  "Fz",
  "Fa",
  "Fs",
  "Ft",
  "Fw",
  "Fdrdx",
  "Fdgdx",
  "Fdbdx",
  "Fdzdx",
  "Fdadx",
  "Fdsdx",
  "Fdtdx",
  "Fdwdx",
  "Fdrdy",
  "Fdgdy",
  "Fdbdy",
  "Fdzdy",
  "Fdady",
  "Fdsdy",
  "Fdtdy",
  "Fdwdy",
  "FtriangleCMD",
  "fbzColorPath",
  "fogMode",
  "alphaMode",
  "fbzMode",
  "lfbMode",
  "clipLeftRight",
  "clipBottomTop",
  "nopCMD",
  "fastfillCMD",
  "swapbufferCMD",
  "fogColor",
  "zaColor",
  "chromaKey",
  "chromaRange",
  "userIntrCmd",
  "stipple",
  "c0",
  "c1",
  "fbiPixelsIn",
  "fbiChromaFail",
  "fbiZfuncFail",
  "fbiAfuncFail",
  "fbiPixelsOut",
  "fogTable00",
  "fogTable01",
  "fogTable02",
  "fogTable03",
  "fogTable04",
  "fogTable05",
  "fogTable06",
  "fogTable07",
  "fogTable08",
  "fogTable09",
  "fogTable0a",
  "fogTable0b",
  "fogTable0c",
  "fogTable0d",
  "fogTable0e",
  "fogTable0f",
  "fogTable10",
  "fogTable11",
  "fogTable12",
  "fogTable13",
  "fogTable14",
  "fogTable15",
  "fogTable16",
  "fogTable17",
  "fogTable18",
  "fogTable19",
  "fogTable1a",
  "fogTable1b",
  "fogTable1c",
  "fogTable1d",
  "fogTable1e",
  "fogTable1f",
  "reservedB0",
  "reservedB1",
  "reservedB2",
  "colBufferAddr",
  "colBufferStride",
  "auxBufferAddr",
  "auxBufferStride",
  "reservedC",
  "clipLeftRight1",
  "clipBottomTop1",
  "reservedD0",
  "reservedD1",
  "reservedD2",
  "reservedD3",
  "reservedD4",
  "reservedD5",
  "reservedE0",
  "reservedE1",
  "reservedE2",
  "reservedE3",
  "reservedE4",
  "reservedE5",
  "reservedE6",
  "reservedE7",
  "reservedF0",
  "reservedF1",
  "reservedF2",
  "swapBufferPend",
  "leftOverlayBuf",
  "rightOverlayBuf",
  "fbiSwapHistory",
  "fbiTrianglesOut",
  "sSetupMode",
  "sVx",
  "sVy",
  "sARGB",
  "sRed",
  "sGreen",
  "sBlue",
  "sAlpha",
  "sVz",
  "sOowfbi",
  "sOow0",
  "sSow0",
  "sTow0",
  "sOow1",
  "sSow1",
  "sTow1",
  "sDrawTriCMD",
  "sBeginTriCMD",
  "reservedG0",
  "reservedG1",
  "reservedG2",
  "reservedG3",
  "reservedG4",
  "reservedG5",
  "reservedH0",
  "reservedH1",
  "reservedH2",
  "reservedH3",
  "reservedH4",
  "reservedH5",
  "reservedH6",
  "reservedH7",
  "reservedI0",
  "reservedI1",
  "reservedI2",
  "reservedI3",
  "reservedI4",
  "reservedI5",
  "reservedI6",
  "reservedI7",
  "textureMode",
  "tLOD",
  "tDetail",
  "texBaseAddr",
  "texBaseAddr1",
  "texBaseAddr2",
  "texBaseAddr38",
  "trexInit0",
  "trexInit1",
  "nccTable0-0",
  "nccTable0-1",
  "nccTable0-2",
  "nccTable0-3",
  "nccTable0-4",
  "nccTable0-5",
  "nccTable0-6",
  "nccTable0-7",
  "nccTable0-8",
  "nccTable0-9",
  "nccTable0-a",
  "nccTable0-b",
  "nccTable1-0",
  "nccTable1-1",
  "nccTable1-2",
  "nccTable1-3",
  "nccTable1-4",
  "nccTable1-5",
  "nccTable1-6",
  "nccTable1-7",
  "nccTable1-8",
  "nccTable1-9",
  "nccTable1-a",
  "nccTable1-b",
  "tChromaKeyMin",
  "tChromaKeyMax",
};

static const char * h3SstIORegNames[] = {
  "status",
  "pciInit0",
  "sipMonitor",
  "lfbMemoryConfig",
  "miscInit0",
  "miscInit1",
  "dramInit0",
  "dramInit1",
  "agpInit",
  "tmuGbeInit",
  "vgaInit0",
  "vgaInit1",
  "dramCommand",
  "dramData",
  "reservedZ0"
  "reservedZ0"
  "pllCtrl0",
  "pllCtrl1",
  "pllCtrl2",
  "dacMode",
  "dacAddr",
  "dacData",
  "vidMaxRGBDelta",
  "vidProcCfg",
  "hwCurPatAddr",
  "hwCurLoc",
  "hwCurC0",
  "hwCurC1",
  "vidInFormat",
  "vidInStatus",
  "vidSerialParallelPort",
  "vidInXDecimDeltas",
  "vidInDecimInitErrs",
  "vidInYDecimDeltas",
  "vidPixelBufThold",
  "vidChromaMin",
  "vidChromaMax",
  "vidCurrentLine",
  "vidScreenSize",
  "vidOverlayStartCoords",
  "vidOverlayEndScreenCoord",
  "vidOverlayDudx",
  "vidOverlayDudxOffsetSrcWidth",
  "vidOverlayDvdy",
  "vgaRegister[0]",
  "vgaRegister[1]",
  "vgaRegister[2]",
  "vgaRegister[3]",
  "vgaRegister[4]",
  "vgaRegister[5]",
  "vgaRegister[6]",
  "vgaRegister[7]",
  "vgaRegister[8]",
  "vgaRegister[9]",
  "vgaRegister[a]",
  "vgaRegister[b]",
  "vidOverlayDvdyOffset",
  "vidDesktopStartAddr",
  "vidDesktopOverlayStride",
  "vidInAddr0",
  "vidInAddr1",
  "vidInAddr2",
  "vidInStride",
  "vidCurrOverlayStartAddr",
} ;


#define GEN_INDEX(a) ((((unsigned long) a) - ((unsigned long) gc->reg_ptr)) >> 2)

void
_grFifoWriteDebug(FxU32 addr, FxU32 val, FxU32 fifoPtr)
{
  GR_DCL_GC;
  FxU32 index = GEN_INDEX(addr);
  
  GDBG_INFO(gc->myLevel + 199, "Storing to FIFO:\n");
  GDBG_INFO(gc->myLevel + 199, "  FIFO Ptr:    0x%x : 0x%X\n", fifoPtr, gc->cmdTransportInfo.fifoRoom);
  if (index <= 0xff) { 
    GDBG_INFO(gc->myLevel + 199, "  Reg Name:    %s\n", h3SstRegNames[index]);
    GDBG_INFO(gc->myLevel + 199, "  Reg Num:     0x%X\n", index);
  } else {
    const char* strP;
    const FxU32 offset = (addr - (FxU32)gc->reg_ptr);
    
    if (offset >= HW_TEXTURE_OFFSET) {
      strP = "Texture";
    } else if (offset >= HW_LFB_OFFSET) {
      strP = "LFB";
      index = addr;
    } else if (offset >= HW_FIFO_OFFSET) {
      strP = "Cmd FIFO";
    } else {
      strP = "Woah!";
    }
    GDBG_INFO(gc->myLevel + 199, "  %s Addr:    0x%X\n",
              strP, index);
  }
  GDBG_INFO(gc->myLevel + 199, "  Value:       0x%X 0x%X\n", (index << 2), val);
  
  GDBG_INFO(120, "        SET(0x%X, %ld(0x%X)) 0 %s (0x%X)\n",
            0x10000000UL + (FxU32)(index << 2), val, val, 
            h3SstRegNames[index & 0xFF], fifoPtr);
} /* _grFifoWriteDebug */

void
_grFifoFWriteDebug(FxU32 addr, float val, FxU32 fifoPtr)
{
  GR_DCL_GC;
  FxU32 index = GEN_INDEX(addr);

  GDBG_INFO(gc->myLevel + 200, "Storing to FIFO:\n");
  GDBG_INFO(gc->myLevel + 200, "  FIFO Ptr:    0x%x\n", fifoPtr);
  if (index <= 0xff) {
    GDBG_INFO(gc->myLevel + 200, "  Reg Name:    %s\n", h3SstRegNames[index]);
    GDBG_INFO(gc->myLevel + 200, "  Reg Num:     0x%x\n", index);
  }
  GDBG_INFO(gc->myLevel + 200, "  Value:       %4.2f\n", val);

  GDBG_INFO(120, "        SET(0x%X, %4.2f (0x%X)) 0 %s\n", 
            0x10000000UL + (FxU32)(index << 2), val, *(const FxU32*)&val, 
            h3SstRegNames[index & 0xFF]);
} /* _grFifoFWriteDebug */

void
_grH3FifoDump_TriHdr(const FxU32 hdrVal)
{
  GR_DCL_GC;

  /* Dump Packet Header */
  GDBG_INFO(gc->myLevel + 200, "CMD Fifo Triangle Packet (0x%X)\n", hdrVal);
  GDBG_INFO(gc->myLevel + 200, "  # Vertex: 0x%X\n", 
            (hdrVal & SSTCP_PKT3_NUMVERTEX) >> SSTCP_PKT3_NUMVERTEX_SHIFT);
  GDBG_INFO(gc->myLevel + 200, "  RGB: %s\n",
            (hdrVal & SSTCP_PKT3_PACKEDCOLOR) ? "Packed" : "Separate");
  
  GDBG_INFO(gc->myLevel + 200, "  StripMode: %s\n",
            (((hdrVal & (0x01 << 22)) == 0) ? "Strip" : "Fan"));
  
  GDBG_INFO(gc->myLevel + 200, "  Culling: %s\n",
            (((hdrVal & (0x01 << 23)) == 0) ? "Disable" : "Enable"));
  
  GDBG_INFO(gc->myLevel + 200, "  CullingSign: %s\n",
            (((hdrVal & (0x01 << 24)) == 0) ? "Positive" : "Negative"));
  
  GDBG_INFO(gc->myLevel + 200, "  PingPongSign: %s\n",
            (((hdrVal & (0x01 << 25)) == 0) ? "Normal" : "Disable"));
  
  if (GDBG_GET_DEBUGLEVEL(gc->myLevel + 200)) {
    const FxU32 temp = (hdrVal & SSTCP_PKT3_PMASK);
    int i;

    GDBG_INFO(gc->myLevel + 200, "  Params: X Y");

    for(i = 10; i <= 17; i++) {
      static const char* paramSel[] = { "RGB", "Alpha", "Z", "Wb", "W0", "ST[0]", "W1", "ST[1]" };
      
      if ((temp & (0x01UL << i)) != 0) GDBG_PRINTF("%s ", paramSel[i - 10]);
    }
    GDBG_INFO(gc->myLevel + 200, "\n");
  }
  
  {
    const FxU32 temp = (hdrVal & SSTCP_PKT3_CMD) >> SSTCP_PKT3_CMD_SHIFT;
    const char* tempStr;

    switch(temp) {
    case 0x00: tempStr = "Independent";   break;
    case 0x01: tempStr = "NewStrip";      break;
    case 0x02: tempStr = "ContinueStrip"; break;
    default:   tempStr = "Reserved";      break;
    }
    GDBG_INFO(gc->myLevel + 200, "  Command: 0x%X(%s)\n", temp, tempStr);
  }
}

void
_grErrorCallback(const char* const procName,
                 const char* const format,
                 va_list           args)
{
  static FxBool inProcP = FXFALSE;

  if (!inProcP) {
    static char errMsgBuf[1024];
    
    inProcP = FXTRUE;
    {
      extern void (*GrErrorCallback)( const char *string, FxBool fatal );

      vsprintf(errMsgBuf, format, args);
      (*GrErrorCallback)(errMsgBuf, (GETENV("FX_ERROR_FAIL") != NULL));
    }
    inProcP = FXFALSE;
  }
}

#endif /* GDBG_INFO_ON */

#if USE_PACKET_FIFO

/* Routines privately exported so that the manufacturing diags
 * and other things can do register writes etc w/o having access
 * to the glide internals etc.
 */
void
_grSet32(volatile FxU32* const sstAddr, const FxU32 val)
{
#define FN_NAME "_grSet32"
  GR_DCL_GC;

  GR_ASSERT(sstAddr >= gc->base_ptr);
  GR_ASSERT(sstAddr <  &SST_TMU(gc->reg_ptr, GR_TMU0)->status);

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET_INDEX(BROADCAST_ID, gc->reg_ptr, (sstAddr - gc->reg_ptr), val);
  GR_CHECK_SIZE();
#undef FN_NAME
}

FxU32
_grGet32(volatile FxU32* const sstAddr)
{
#define FN_NAME "_grGet32"
  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME, 88);
  GDBG_INFO_MORE(gc->myLevel, "(0x%X)\n", sstAddr);
  GR_RETURN(GR_GET(*sstAddr));
#undef FN_NAME
} /* _grGet32 */

#if FIFO_ASSERT_FULL
const FxU32 kFifoCheckMask = 0xFFFF;
FxU32 gFifoCheckCount = 0;
#endif

void
_grBumpNGrind() 
{
#define FN_NAME "_grCheckForBump"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 400);

  FIFO_CACHE_FLUSH(gc->cmdTransportInfo.fifoPtr);
  P6FENCE;
  GR_CAGP_SET(bump, gc->cmdTransportInfo.fifoPtr - gc->cmdTransportInfo.lastBump);
  gc->cmdTransportInfo.lastBump = gc->cmdTransportInfo.fifoPtr;
  gc->cmdTransportInfo.bumpPos = gc->cmdTransportInfo.fifoPtr + (gc->cmdTransportInfo.bumpSize);
  if (gc->cmdTransportInfo.bumpPos > gc->cmdTransportInfo.fifoEnd)
    gc->cmdTransportInfo.bumpPos = gc->cmdTransportInfo.fifoEnd;

  GR_ASSERT(gc->cmdTransportInfo.bumpPos != 0);
  GR_ASSERT(gc->cmdTransportInfo.lastBump != 0);

#undef FN_NAME
} /* _grCheckForBump */

struct cmdTransportInfo* FX_CALL
_grCommandTransportInfo(void)
{
#define FN_NAME "_grCommandTransportInfo"
  GR_BEGIN_NOFIFOCHECK_NORET(FN_NAME"()\n", 98);

  /* validate the state so that the packet headers are correct */
  if (gc->state.invalid) 
    _grValidateState();

  GR_RETURN(&gc->cmdTransportInfo);
#undef FN_NAME
} /* _grCommandTransportInfo */

void 
_FifoFlush( void ) 
{
#define FN_NAME "_FifoFlush"
  _grCommandTransportMakeRoom(0, __FILE__, __LINE__);
#undef FN_NAME
} /* _FifoFlush */

void FX_CALL
_grCommandTransportMakeRoom(const FxI32 blockSize, const char* fName, const int fLine)
{
#define FN_NAME "_FifoMakeRoom"

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 400);

  if ( gc->windowed ) {
#if defined( GLIDE_INIT_HWC ) && !defined(__linux__) && !(GLIDE_PLATFORM & GLIDE_OS_DOS32)
    struct cmdTransportInfo*
      gcFifo = &gc->cmdTransportInfo;
    HwcWinFifo 
      fifo = gc->cmdTransportInfo.hwcFifoInfo;
    const FxU32
      nextBufferIndex = gcFifo->curCommandBuf + gcFifo->numQueuedBuf,
      cmdBufferOffset = (gcFifo->hwcFifoInfo.cmdBuf.allocUnit * gcFifo->curCommandBuf);
    FxU32
      buffersFree;
    
    /* Update to the currently writing command buffer */
    fifo.cmdBuf.baseAddr += cmdBufferOffset;
    fifo.cmdBuf.hwOffset += cmdBufferOffset;
    fifo.cmdBuf.size      = ((FxU32)gcFifo->fifoPtr - fifo.cmdBuf.baseAddr);
    
    fifo.stateBuf.baseAddr  = (FxU32)gcFifo->stateBuffer;
    fifo.stateBuf.hwOffset += (gcFifo->hwcFifoInfo.stateBuf.allocUnit * gcFifo->curCommandBuf);
    fifo.stateBuf.size      = sizeof(GrStateBuffer);
    
    {
      FxBool
        issueP;

  __tryFifoExecute:
      GR_WINFIFO_BEGIN();
      {
        issueP = hwcExecuteWinFifo(gc->bInfo, 
                                   gc->winContextId,
                                   &fifo, 
                                   gcFifo->serialNumber + gcFifo->numQueuedBuf);
      }
      GR_WINFIFO_END();
      
      /* Did we execute? */
      if (issueP) {
        /* Update the issued command bookkeeping */
        gcFifo->issuedSerialNumber = gcFifo->serialNumber + gcFifo->numQueuedBuf;
        /* Update us to next serial number */
        gcFifo->serialNumber += gcFifo->numQueuedBuf;
        
        /* See if we have free buffers available */
        buffersFree = gcFifo->numCommandBuf - (gcFifo->serialNumber - gcFifo->committedSerialNumber);
        
        /* If no buffers are free, then we must wait for some space to become available. */
        while((FxI32)buffersFree <= 0) {
          gcFifo->committedSerialNumber = hwcExecuteStatusWinFifo(gc->bInfo,
                                                                  &gcFifo->hwcFifoInfo,
                                                                  gcFifo->serialNumber);
          buffersFree = gcFifo->numCommandBuf - (gcFifo->serialNumber - gcFifo->committedSerialNumber);
        }
        
        /* Do we have more buffers or do we need to wrap? */
        GR_ASSERT(nextBufferIndex <= gcFifo->numCommandBuf);
        
        /* We should have at least one buffer free at this point */
        if(nextBufferIndex == gcFifo->numCommandBuf)
          gcFifo->curCommandBuf = 0;
        else
          gcFifo->curCommandBuf = nextBufferIndex;
        
        /* Set the current fifo ptr in allocation blocks */
        gcFifo->fifoPtr = (FxU32*)(gcFifo->hwcFifoInfo.cmdBuf.baseAddr +
                                   (gcFifo->hwcFifoInfo.cmdBuf.allocUnit * gcFifo->curCommandBuf));
        
        /* Set the state buffer to be the 'next' one in the ready
         * list.  This is the same index as the next buffer in the
         * command list.  
         */
        gcFifo->stateBuffer = (GrStateBuffer*)(gcFifo->hwcFifoInfo.stateBuf.baseAddr +
                                               (gcFifo->hwcFifoInfo.stateBuf.allocUnit * gcFifo->curCommandBuf));
        
        /* Reset next execution size to be an allocation unit */
        gcFifo->fifoRoom = gcFifo->hwcFifoInfo.cmdBuf.allocUnit - FIFO_END_ADJUST;
        gcFifo->numQueuedBuf = 0x01UL;
        
        /* Copy the state at the end of the last submitted command
         * buffer to be the state for the next command buffer.  
         */
#if __POWERPC__
        {
          FxU32 *src, *dst, i;
          src = (FxU32 *)&gc->state.shadow;
          dst = (FxU32 *)gcFifo->stateBuffer;
          for(i = 0; i < (sizeof(GrStateBuffer)/sizeof(FxU32)); i++) {
            SET(*dst++,*src++);
          }
        }
#else
        memcpy(gcFifo->stateBuffer, 
               &gc->state.shadow, 
               sizeof(GrStateBuffer));
#endif
      } else {
        /* Didn't execute, but we can check to see if we have some
         * room to keep going. Otherwise we need to check again to see
         * if we can execute now.
         */
        
        /* See if we have free buffers available */
        buffersFree = (gcFifo->numCommandBuf - 
                       ((gcFifo->serialNumber + gcFifo->numQueuedBuf) - gcFifo->committedSerialNumber));
        
        /* If no free buffers, get latest committed from memory (slow) and check again (but don't spin). */          
        if((FxI32)buffersFree <= 0) {
          gcFifo->committedSerialNumber = hwcExecuteStatusWinFifo(gc->bInfo,
                                                                  &gcFifo->hwcFifoInfo,
                                                                  gcFifo->serialNumber);
          buffersFree = (gcFifo->numCommandBuf - 
                         ((gcFifo->serialNumber + gcFifo->numQueuedBuf) - gcFifo->committedSerialNumber));
        }
        
        /* If we have more free buffers and we can append, then do so */
        if((buffersFree > 0) && (nextBufferIndex < gcFifo->numCommandBuf)) {
          gcFifo->numQueuedBuf++;
          gcFifo->fifoRoom += gcFifo->hwcFifoInfo.cmdBuf.allocUnit;
        } else {
          /* Well, we couldn't issue, and we didn't have any free space, so try 
           * to issue again. 
           */
          goto __tryFifoExecute;
        }
      }
    }
    
    GR_SET_FIFO_PTR( 0, 0 );
#endif
  } else {
    /* Check here to see if we have a valid context since the last time
     * we checked. This is to protect us from loosing our context before
     * we wrap check the current hw fifo pointer which is going to be the
     * 2d driver's fifo if we lost our context.
     */
#if defined(GLIDE_INIT_HWC) && GLIDE_CHECK_CONTEXT
    gc->contextP = hwcQueryContext(gc->bInfo);
#else
    gc->contextP = 1;
#endif
    if (gc->contextP) {
#if 0
      FxU32 wrapAddr = 0x00UL;
#endif
      FxU32 checks;

      GR_ASSERT(blockSize > 0);
      GR_ASSERT((FxU32)blockSize < gc->cmdTransportInfo.fifoSize);
      FIFO_ASSERT();
      
    /* Update the roomToXXX values w/ the # of writes since the last
     * fifo stall/wrap.  
     */
      {
        const FxI32 writes = (MIN(gc->cmdTransportInfo.roomToReadPtr, gc->cmdTransportInfo.roomToEnd) -
                              gc->cmdTransportInfo.fifoRoom);
        
        gc->cmdTransportInfo.roomToReadPtr   -= writes;
        gc->cmdTransportInfo.roomToEnd       -= writes;
        
#if GDBG_INFO_ON
        GDBG_INFO_MORE(gc->myLevel, ": (%s : %d)\n"
                       "\tfifoBlock: (0x%X : 0x%X)\n"
                       "\tfifoRoom: (0x%X : 0x%X) : (0x%X : 0x%X)\n"
                       "\tfifo hw: (0x%X : 0x%X)\n",
                       ((fName == NULL) ? "Unknown" : fName), fLine,
                       (((FxU32)gc->cmdTransportInfo.fifoPtr - (FxU32)gc->cmdTransportInfo.fifoStart) + 
                        (FxU32)gc->cmdTransportInfo.fifoOffset),
                       blockSize,
                       gc->cmdTransportInfo.roomToReadPtr, gc->cmdTransportInfo.roomToEnd, 
                       gc->cmdTransportInfo.fifoRoom, writes,
                       HW_FIFO_PTR(FXTRUE) - (FxU32)gc->rawLfb, gc->cmdTransportInfo.fifoRead);

#endif /* GDBG_INFO_ON */

        ASSERT_FAULT_IMMED((gc->cmdTransportInfo.roomToReadPtr >= 0) && 
                           (gc->cmdTransportInfo.roomToEnd >= 0));
      }
      
      /* Bump & Grind if called for */
      if (!gc->cmdTransportInfo.autoBump)
        GR_BUMP_N_GRIND;
      
      checks = 0;
      
  again:
      /* do we need to stall? */
      {
        FxU32 lastHwRead = gc->cmdTransportInfo.fifoRead;
        FxI32 roomToReadPtr = gc->cmdTransportInfo.roomToReadPtr;
        
        while (roomToReadPtr < blockSize) {
          FxU32 curReadPtr = HW_FIFO_PTR(FXTRUE);
          FxU32 curReadDist = curReadPtr - lastHwRead;

          checks++;

#ifdef GLIDE_DEBUG
          if (checks > 1000) {
            const FxU32
              baseAddrL = GR_CAGP_GET(baseAddrL),
              baseSize = GR_CAGP_GET(baseSize),
              readPtrL = GR_CAGP_GET(readPtrL),
              aMin = GR_CAGP_GET(aMin),
              aMax = GR_CAGP_GET(aMax),
              depth = GR_CAGP_GET(depth),
              holeCount = GR_CAGP_GET(holeCount);
            
            GDBG_PRINTF("Fifo check timeout:\n");
            GDBG_PRINTF("\tbaseAddrL = 0x%x\n", baseAddrL);
            GDBG_PRINTF("\tbaseSize  = 0x%x\n", baseSize);
            GDBG_PRINTF("\treadPtrL  = 0x%x\n", readPtrL);
            GDBG_PRINTF("\tdepth     = 0x%x\n", depth);
            GDBG_PRINTF("\tholeCount = 0x%x\n", holeCount);
            GDBG_PRINTF("\taMin      = 0x%x\n", aMin);
            GDBG_PRINTF("\taMax      = 0x%x\n", aMax);

            if ((readPtrL < (baseAddrL << 12)) ||
                (readPtrL > ((baseAddrL + baseSize + 1) << 12))) {
              GDBG_PRINTF("FATAL ERROR:  Read Pointer out of command buffer extents\n");
              exit(-1);
            }
            checks = 0;
          }
#endif /* GLIDE_DEBUG */
          GR_ASSERT((curReadPtr >= (FxU32)gc->cmdTransportInfo.fifoStart) &&
                    (curReadPtr < (FxU32)gc->cmdTransportInfo.fifoEnd));

          roomToReadPtr += curReadDist;

          gc->stats.fifoStalls++;
          gc->stats.fifoStallDepth += GR_CAGP_GET(depth);

          /* Have we wrapped yet? */
          if (lastHwRead > curReadPtr) roomToReadPtr += (FxI32)gc->cmdTransportInfo.fifoSize - FIFO_END_ADJUST;
          lastHwRead = curReadPtr;
        }

        GR_ASSERT((lastHwRead >= (FxU32)gc->cmdTransportInfo.fifoStart) &&
                  (lastHwRead < (FxU32)gc->cmdTransportInfo.fifoEnd));

        /* Update cached copies */
        gc->cmdTransportInfo.fifoRead = lastHwRead;
        gc->cmdTransportInfo.roomToReadPtr = roomToReadPtr;

        GDBG_INFO(gc->myLevel + 10, 
                  "  Wait: (0x%X : 0x%X) : 0x%X\n", 
                  gc->cmdTransportInfo.roomToReadPtr, gc->cmdTransportInfo.roomToEnd,
                  gc->cmdTransportInfo.fifoRead);
      }
      
      /* Do we need to wrap to front? */
      if (gc->cmdTransportInfo.roomToEnd <= blockSize) {
        GDBG_INFO(gc->myLevel + 10, 
                  "  Pre-Wrap: (0x%X : 0x%X) : 0x%X\n", 
                  gc->cmdTransportInfo.roomToReadPtr, gc->cmdTransportInfo.roomToEnd,
                  gc->cmdTransportInfo.fifoRead);

        /* Set the jsr packet.
         * NB: This command must be fenced.
         */
        FIFO_ASSERT();
        {
          P6FENCE;
          if (!gc->cmdTransportInfo.autoBump) {
#if __POWERPC__ && PCI_BUMP_N_GRIND
            SET_FIFO(*gc->cmdTransportInfo.fifoPtr++, gc->cmdTransportInfo.fifoJmpHdr[0]);
            FIFO_CACHE_FLUSH(gc->cmdTransportInfo.fifoPtr);
            GR_CAGP_SET(bump, 1);
#else /* !__POWERPC__ && !PCI_BUMP_N_GRIND */
            SET(*gc->cmdTransportInfo.fifoPtr++, gc->cmdTransportInfo.fifoJmpHdr[0]);
            SET(*gc->cmdTransportInfo.fifoPtr++, gc->cmdTransportInfo.fifoJmpHdr[1]);
            GR_CAGP_SET(bump, 2);
#endif /* !__POWERPC__ && !PCI_BUMP_N_GRIND */
            gc->cmdTransportInfo.lastBump = gc->cmdTransportInfo.fifoStart;
          } else {
            SET(*gc->cmdTransportInfo.fifoPtr, gc->cmdTransportInfo.fifoJmpHdr[0]);
          }
        }

        P6FENCE;

#if 0
        wrapAddr = (FxU32)gc->cmdTransportInfo.fifoPtr;
#endif

        /* Update roomXXX fields for the actual wrap */
        gc->cmdTransportInfo.roomToReadPtr -= gc->cmdTransportInfo.roomToEnd;
        gc->cmdTransportInfo.roomToEnd = gc->cmdTransportInfo.fifoSize - FIFO_END_ADJUST;

#if GLIDE_USE_DEBUG_FIFO
        gc->stats.fifoWraps++;
        gc->stats.fifoWrapDepth += GR_GET(hw->cmdFifoDepth);
#endif /* GLIDE_USE_DEBUG_FIFO */

        /* Reset fifo ptr to start */
        gc->cmdTransportInfo.fifoPtr = gc->cmdTransportInfo.fifoStart;

        GDBG_INFO(gc->myLevel + 10, 
                  "  Post-Wrap: (0x%X : 0x%X) : 0x%X\n", 
                  gc->cmdTransportInfo.roomToReadPtr, gc->cmdTransportInfo.roomToEnd,
                  gc->cmdTransportInfo.fifoRead);

        goto again;
      }

      /* compute room left */
      gc->cmdTransportInfo.fifoRoom = MIN(gc->cmdTransportInfo.roomToReadPtr, gc->cmdTransportInfo.roomToEnd);

      GDBG_INFO(gc->myLevel, FN_NAME"_Done:\n"
                "\tfifoBlock: (0x%X : 0x%X)\n"
                "\tfifoRoom: (0x%X : 0x%X : 0x%X)\n"
                "\tfifo hw: (0x%X : 0x%X) : (0x%X : 0x%X : 0x%X)\n",
                (((FxU32)gc->cmdTransportInfo.fifoPtr - (FxU32)gc->cmdTransportInfo.fifoStart) + 
                 (FxU32)gc->cmdTransportInfo.fifoOffset),
                blockSize,
                gc->cmdTransportInfo.roomToReadPtr, 
                gc->cmdTransportInfo.roomToEnd, gc->cmdTransportInfo.fifoRoom,
                HW_FIFO_PTR(FXTRUE) - (FxU32)gc->rawLfb, gc->cmdTransportInfo.fifoRead, 
                GR_CAGP_GET(depth), GR_CAGP_GET(holeCount), GR_GET(hw->status));
      
      FIFO_ASSERT();
    }
    else {
      /*
      ** reset the function pointers when the context is gone
      */ 
      GrTriSetupProcArchVector* 
        curTriProcs = _GlideRoot.deviceArchProcs.curTriProcs;
      GrVertexListProc*
        curVertexListProcs = _GlideRoot.deviceArchProcs.curVertexListProcs;
      
      _GlideRoot.deviceArchProcs.curTriProcs        = _GlideRoot.deviceArchProcs.nullTriProcs;
      _GlideRoot.deviceArchProcs.curVertexListProcs = _GlideRoot.deviceArchProcs.nullVertexListProcs;
      
      _GlideRoot.deviceArchProcs.nullTriProcs        = curTriProcs;
      _GlideRoot.deviceArchProcs.nullVertexListProcs = curVertexListProcs;
      
      gc->archDispatchProcs.texDownloadProcs  = _GlideRoot.deviceArchProcs.nullTexProcs;
      gc->archDispatchProcs.drawTrianglesProc = _GlideRoot.deviceArchProcs.nullDrawTrisProc;
      gc->archDispatchProcs.coorModeTriVector = (*_GlideRoot.deviceArchProcs.curTriProcs) + GR_WINDOW_COORDS;
      gc->archDispatchProcs.drawVertexList    = _GlideRoot.deviceArchProcs.curVertexListProcs[GR_WINDOW_COORDS];
      gc->triSetupProc = CUR_TRI_PROC(FXFALSE, (gc->state.cull_mode != GR_CULL_DISABLE));
    }
  }
  GR_TRACE_EXIT(FN_NAME);
#undef FN_NAME
}

void
_grH3FifoDump_Linear(const FxU32* const linearPacketAddr)
{
#ifdef GDBG_INFO_ON
  FXUNUSED(h3SstIORegNames);
#endif
}


unsigned long
_grHwFifoPtr(FxBool ignored)
{
  unsigned long rVal = 0;

  FxU32 status, readPtrL1, readPtrL2;
  GR_DCL_GC;

  FXUNUSED(ignored);

  if ( gc->windowed ) {
    rVal = 0;
  } else {
    do {
      readPtrL1 = GET(gc->cRegs->cmdFifo0.readPtrL);
#if __POWERPC__ 
      status = GET(gc->ioRegs->status);
#else
      status = _grSstStatus();
#endif
      readPtrL2 = GET(gc->cRegs->cmdFifo0.readPtrL);
    } while (readPtrL1 != readPtrL2);
    rVal = (((unsigned long)gc->cmdTransportInfo.fifoStart) + 
            readPtrL2 - 
            (FxU32)gc->cmdTransportInfo.fifoOffset);
  }
  return rVal;
} /* _grHwFifoPtr */


#if defined( FIFO_ASSERT_FULL )
void 
_fifoAssertFull( void )
{
  GR_DCL_GC;
  
  if ( !gc->windowed ) {
    if ((gFifoCheckCount++ & kFifoCheckMask) == 0) {
      const FxU32 cmdFifoDepth = GR_GET(((SstRegs*)(gc->reg_ptr))->cmdFifoDepth);
      const FxU32 maxFifoDepth = ((gc->cmdTransportInfo.fifoSize - FIFO_END_ADJUST) >> 2);
      if(cmdFifoDepth > maxFifoDepth) {
        gdbg_printf("cmdFifoDepth > size: 0x%X : 0x%Xn", 
                    cmdFifoDepth, maxFifoDepth);
        ASSERT_FAULT_IMMED(cmdFifoDepth <= maxFifoDepth);
      } else if (cmdFifoDepth + (gc->cmdTransportInfo.fifoRoom >> 2) > maxFifoDepth) {
        gdbg_printf("cmdFifoDepth + fifoRoom > size: (0x%X : 0x%X) : 0x%Xn", 
                    cmdFifoDepth, (gc->cmdTransportInfo.fifoRoom >> 2), maxFifoDepth);
        ASSERT_FAULT_IMMED(cmdFifoDepth + (gc->cmdTransportInfo.fifoRoom >> 2) <= maxFifoDepth);
      }
    }
    ASSERT_FAULT_IMMED(HW_FIFO_PTR(FXTRUE) >= (FxU32)gc->cmdTransportInfo.fifoStart);
    ASSERT_FAULT_IMMED(HW_FIFO_PTR(FXTRUE) < (FxU32)gc->cmdTransportInfo.fifoEnd);
    ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoRoom < gc->cmdTransportInfo.fifoSize);
    ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoPtr < (FxU32)gc->cmdTransportInfo.fifoEnd);
  }
}
#endif

#if GLIDE_DEBUG
void 
_reg_group_begin_internal( FxU32 __chipId, FxU32 __regBase, FxU32 __groupNum, 
                           FxU32 __groupMask, FxU32 __pktHdr, FxU32 __checkP, 
                           volatile FxU32 *_regGroupFifoPtr ) 
{
  GR_DCL_GC;

  if ( !gc->windowed ) {
    GR_ASSERT(((__pktHdr) & 0xE0000000UL) == 0x00UL);
    FIFO_ASSERT();
    GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) >= gc->cmdTransportInfo.fifoOffset);
    GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) < (gc->cmdTransportInfo.fifoOffset + gc->cmdTransportInfo.fifoSize));

    GDBG_INFO(120, "REG_GROUP_BEGIN:\n");
    GDBG_INFO(120, "\tFile: %s Line %d\n", __FILE__, __LINE__);
    GDBG_INFO(120, "\t_regGroupFifoPtr:  0x%x\n", (FxU32)_regGroupFifoPtr - (FxU32)gc->rawLfb);
    GDBG_INFO(120, "\t__pktHdr:          0x%x\n", __pktHdr);
    GDBG_INFO(120, "\t__groupNum:        0x%x\n", __groupNum);
    GDBG_INFO(120, "\t__groupMask:       0x%x\n", (__groupMask));
    GDBG_INFO(120, "\t__chipId:          0x%x\n", __chipId);
    GDBG_INFO(120, "\t__regBase:         0x%x\n", __regBase);
    GDBG_INFO(120, "\tfifoPtr:           0x%x\n", (FxU32)gc->cmdTransportInfo.fifoPtr - (FxU32)gc->cmdTransportInfo.fifoStart - (FxU32)gc->rawLfb);
    GDBG_INFO(120, "\tfifoRoom:          0x%x\n", gc->cmdTransportInfo.fifoRoom);
    GDBG_INFO(120, "\treadPtrL:          0x%x\n", GET(gc->cRegs->cmdFifo0.readPtrL));
  }
}

void
_reg_group_begin_internal_wax( FxU32 __regBase,
                               FxU32 __groupNum,
                               FxU32 __groupMask,
                               FxU32 __pktHdr,
                               FxU32 __checkP,
                               volatile FxU32 *__regGroupFifoPtr )
{
  GR_DCL_GC;

  if ( !gc->windowed ) {
    GR_ASSERT(((__pktHdr) & 0xE0000000UL) == 0x00UL);
    FIFO_ASSERT();
    GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) >= gc->cmdTransportInfo.fifoOffset);
    GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) < (gc->cmdTransportInfo.fifoOffset + gc->cmdTransportInfo.fifoSize));

    GDBG_INFO(220, "REG_GROUP_BEGIN_WAX:\n");
    GDBG_INFO(220, "\tFile: %s Line %d\n", __FILE__, __LINE__);
    GDBG_INFO(220, "\t_regGroupFifoPtr:  0x%x\n", 
              (FxU32)__regGroupFifoPtr - (FxU32)gc->rawLfb);
    GDBG_INFO(220, "\t__pktHdr:           0x%x\n", __pktHdr);
    GDBG_INFO(220, "\t__groupNum:         0x%x\n", __groupNum);
    GDBG_INFO(220, "\t__groupMask:        0x%x\n", (__groupMask));
    GDBG_INFO(220, "\t__regBase:          0x%x\n", __regBase);
    GDBG_INFO(220, "\tfifoPtr:            0x%x\n", (FxU32)gc->cmdTransportInfo.fifoPtr - (FxU32)gc->cmdTransportInfo.fifoStart - (FxU32)gc->rawLfb);
    GDBG_INFO(220, "\tfifoRoom:           0x%x\n", gc->cmdTransportInfo.fifoRoom);
    GDBG_INFO(220, "\treadPtrL:           0x%x\n", GET(gc->cRegs->cmdFifo0.readPtrL));
    GDBG_INFO(220, "\tStart Reg:          0x%x\n", (__pktHdr & 0x7fff) >> 3);
    GDBG_INFO(220, "\tReg Mask:           0x%x\n", (__pktHdr >> 15) & 0x3fff);
    GDBG_INFO(220, "\tReg Type:           %s\n", ((__pktHdr >> 14) & 1) ? "2D" : "3D");
  }
}
#endif /* GLIDE_DEBUG */
#endif /* USE_PACKET_FIFO */

#if DRI_BUILD
void
_grImportFifo(int fifoPtr, int fifoRead) {
  struct cmdTransportInfo* gcFifo;
  FxU32 readPos;
  GR_DCL_GC;
#if 1
  FxU32 dummy, d;

  do {
    dummy=GET(gc->cRegs->cmdFifo0.depth);
    d=GET(gc->cRegs->cmdFifo0.depth);
  } while (dummy || d);
  do {
    dummy = GET(gc->cRegs->cmdFifo0.readPtrL);
    readPos = GET(gc->cRegs->cmdFifo0.readPtrL);
  } while (dummy!=readPos);
  gcFifo=&gc->cmdTransportInfo;
  readPos=readPos-gcFifo->fifoOffset;
  gcFifo->fifoPtr = gcFifo->fifoStart + (readPos>>2);
  gcFifo->fifoRead = (unsigned long)gcFifo->fifoPtr;
#else
  gcFifo=&gc->cmdTransportInfo;
  gcFifo->fifoPtr = gc->rawLfb+(fifoPtr>>2);
  gcFifo->fifoRead = ((int)gc->rawLfb)+fifoRead;
#endif
  gcFifo->roomToReadPtr = gcFifo->fifoRead-((long)gcFifo->fifoPtr)-FIFO_END_ADJUST-sizeof(FxU32);
  if (gcFifo->roomToReadPtr<0) gcFifo->roomToReadPtr+=gcFifo->fifoSize;
  gcFifo->roomToEnd = gcFifo->fifoSize - 
    ((gcFifo->fifoPtr-gcFifo->fifoStart)<<2) -
    FIFO_END_ADJUST;
  gcFifo->fifoRoom = MIN(gcFifo->roomToEnd, gcFifo->roomToReadPtr);
  if (!gc->cmdTransportInfo.autoBump) {
    gcFifo->lastBump = gcFifo->fifoPtr;
    gcFifo->bumpPos = gcFifo->fifoPtr + gcFifo->bumpSize;
  }
}

void
_grExportFifo(int *fifoPtr, int *fifoRead) {
  struct cmdTransportInfo* gcFifo;
  GR_DCL_GC;
  gcFifo=&gc->cmdTransportInfo;
  *fifoPtr=(gcFifo->fifoPtr-gc->rawLfb)<<2;
  *fifoRead=(gcFifo->fifoRead-(long)gc->rawLfb);
}

#endif
