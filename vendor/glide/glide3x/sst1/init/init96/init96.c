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

#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */

/* ANSI Headers */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(__DOS32__) || defined(INIT96VGASWAP)
#include <conio.h>
#endif

#ifdef __DOS32__
#ifdef __DJGPP__
#include <dos.h>
#else
#include <i86.h>
#endif
#endif

#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef INIT96VGASWAP
#include "initvga.h"
#endif

#if defined(__WATCOMC__)
#define _inp inp
#define _outp outp
#endif

#ifdef __DJGPP__
#include <fxdpmi.h>
#endif

/* 3Dfx Headers */
#include <3dfx.h>
#include <fxpci.h>
#include <gdebug.h>
#include <sst.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>

#include <init.h>

/* Local Headers */
#include "init96.h"
#include "fxinit96.h"

#include "initat3d.h"
#if 0
#include "init9685.h"
#include "initmrti.h"
#endif
#include "initmcrx.h"

#ifdef GDBG_INFO_ON
static char *bufTypeNames[] = {
  "INIT_BUFFER_FRONTBUFFER",
  "INIT_BUFFER_BACKBUFFER",
  "INIT_BUFFER_AUXBUFFER",
  "INIT_BUFFER_DEPTHBUFFER",
  "INIT_BUFFER_ALPHABUFFER",
  "INIT_BUFFER_TRIPLEBUFFER",
  "INIT_BUFFER_FIFOBUFFER",
  "INIT_BUFFER_SCREENBUFFER",
  "INIT_BUFFER_TEXTUREBUFFER"
};
#endif


/*==================== Local Constants ====================*/
#define NUM_BUFS 6

#define SST96_MEMMAP_TEXTURE_MEMORY_OFFSET  (0x6 << 20)     /* +5 MB */

#define SST96_COMMAND_REGS(X) ( ((char*)(X))+(4<<20) )

#define MAXTRIES        10000

#define FIFOLAG         128

/*-------------------------------------------------------------------
  Module Data
  -------------------------------------------------------------------*/
volatile FxU32   *sstHW;
volatile Sstregs *sstPtr;

typedef struct _BufInfo {
  /* DISPLAY Stuff */
  FxI32 dfb;                    /* Was fbN */
  FxI32 dfbBase;                /* Base of Buffers */
  FxI32 dfbTop;                 /* Tops of Buffers */
  FxI32 dfbStride;              /* Strides */
  /* RENDER stuff */
  FxI32 rfb;                    /* was rfbN */
} BufInfo, *BufInfoPtr;


#define RFB 0
#define DFB 1


extern FxBool tripleBuffering;

#define ADVANCE_BUFFERS(kind)\
{\
  kind##FrontIdx = (kind##FrontIdx < maxBufIdx) ?  kind##FrontIdx + 1 : 0;\
  kind##BackIdx  = (kind##BackIdx  < maxBufIdx) ?  kind##BackIdx + 1 : 0;\
  kind##RenderIdx = (kind##RenderIdx < maxBufIdx) ? kind##RenderIdx + 1 : 0;\
}

#define rfbTripleIdx ((rfbBackIdx + 1) % maxBuffer)
#define MAX_BUF 3

static BufInfo bI[MAX_BUF];

static FxI32
  maxBuffer = 2,
  maxBufIdx,
  /* DISPLAY Stuff */
  /* The following two are indices into the above */
  dfbFrontIdx =  0,
  dfbBackIdx  =  1,
  dfbRenderIdx = 1,
  /* RENDER stuff */
  rfbFrontIdx =  0,
  rfbBackIdx  =  1,
  rfbRenderIdx = 1;

static FxI32
ab0, ab0Top, ab0Base, ab0Stride;

static InitOriginLocation_t cachedYOrigin;
static InitSwapType_t       cachedSwapType;

static FxU32           fifoStart, fifoEnd;
volatile FxU32 *fifovirt; /* start virtual address */
volatile FxU32  fifophys; /* start physical address  */
volatile FxU32  fifomax;  /* in bytes */
volatile FxU32  fifocur;  /* how much used up so far */
volatile FxU32 *fifoptr;  /* current pointer */
volatile FxU32  fifosize; /* how much of 4kbytes used up */
volatile FxU32  fifolfb;  /* use LFB, use hostblt if 0. */

#if SST96_ALT_FIFO_WRAP && GDBG_INFO_ON
/* Statistics about how many times we wrapped/waited for
 * the fifo.
 */
static FxU32 numWraps;
static FxU32 numJuJuStalls;
static FxU32 numStalls;
#endif /* GDBG_INFO_ON && SST96_ALT_FIFO_WRAP */

Init96HALData *curHALData;

#ifdef __DOS32__
static FxU16
oldVidMode;
#endif

static FxU32 
devNum, 
physAddr,
fifoMTRRNum = 0xffffffff;

/*---- copied from initvg/parse.c here for SST96 ------------*/
const char*
myGetenv(const char* envKey)
{
  FxBool callRealGetenvP = FXTRUE;

#if __WIN32__
  /* NB: If were being called from cya code in
   * DllMain(DLL_PROCESS_DETACH) because the current app has called
   * exit() or dropped off of the end of main the per dll environ
   * string table has been freed by the c runtime but has not been set
   * to NULL. Bad things happen if this memory has been unmapped by
   * the system or if the string cannot be found.  
   */
  {
    HANDLE curProcessHandle = GetCurrentProcess();
    DWORD exitCode = STILL_ACTIVE;

    callRealGetenvP = ((curProcessHandle != NULL) &&
                       GetExitCodeProcess(curProcessHandle, &exitCode) &&
                       (exitCode == STILL_ACTIVE));
  }
#endif /* __WIN32__ */

  return (callRealGetenvP
          ? getenv(envKey)
          : NULL);
}

/*-----------Debuging Info Data------------------------------*/
#ifdef GDBG_INFO_ON
/* NOTE:
   
   This array is HIGHLY important to debugging.  Delete it at your
   extreme peril.
   */
const char *iRegNames[] = {
  "status",                     /* 0x00 */
  "reserved001",                /* 0x01 */
  "vAx",                        /* 0x02 */
  "vAy",                        /* 0x03 */
  "vBx",                        /* 0x04 */
  "vBy",                        /* 0x05 */
  "vCx",                        /* 0x06 */
  "vCy",                        /* 0x07 */
  "r",                          /* 0x08 */
  "drdx",                       /* 0x09 */
  "drdy",                       /* 0x0a */
  "g",                          /* 0x0b */
  "dgdx",                       /* 0x0c */
  "dgdy",                       /* 0x0d */
  "b",                          /* 0x0e */
  "dbdx",                       /* 0x0f */
  "dbdy",                       /* 0x10 */
  "z",                          /* 0x11 */
  "dzdx",                       /* 0x12 */
  "dzdy",                       /* 0x13 */
  "a",                          /* 0x14 */
  "dadx",                       /* 0x15 */
  "dady",                       /* 0x16 */
  "s",                          /* 0x17 */
  "dsdx",                       /* 0x18 */
  "dsdy",                       /* 0x19 */
  "t",                          /* 0x1a */
  "dtdx",                       /* 0x1b */
  "dtdy",                       /* 0x1c */
  "w",                          /* 0x1d */
  "dwdx",                       /* 0x1e */
  "dwdy",                       /* 0x1f */
  "triangleCMD",                /* 0x20 */
  "reserved021",                /* 0x21 */
  "FvAx",                       /* 0x22 */
  "FvAy",                       /* 0x23 */
  "FvBx",                       /* 0x24 */
  "FvBy",                       /* 0x25 */
  "FvCx",                       /* 0x26 */
  "FvCy",                       /* 0x27 */
  "Fr",                         /* 0x28 */
  "Fdrdx",                      /* 0x29 */
  "Fdrdy",                      /* 0x2a */
  "Fg",                         /* 0x2b */
  "Fdgdx",                      /* 0x2c */
  "Fdgdy",                      /* 0x2d */
  "Fb",                         /* 0x2e */
  "Fdbdx",                      /* 0x2f */
  "Fdbdy",                      /* 0x30 */
  "Fz",                         /* 0x31 */
  "Fdzdx",                      /* 0x32 */
  "Fdzdy",              /* 0x33 */
  "Fa",                 /* 0x34 */
  "Fdadx",              /* 0x35 */
  "Fdady",              /* 0x36 */
  "Fs",                 /* 0x37 */
  "Fdsdx",              /* 0x38 */
  "Fdsdy",              /* 0x39 */
  "Ft",                 /* 0x3a */
  "Fdtdx",              /* 0x3b */
  "Fdtdy",              /* 0x3c */
  "Fw",                 /* 0x3d */
  "Fdwdx",              /* 0x3e */
  "Fdwdy",              /* 0x3f */
  "FtriangleCMD",       /* 0x40 */
  "reserved041",        /* 0x41 */
  "nopCMD",             /* 0x42 */
  "reserved043",        /* 0x43 */
  "fastfillCMD",        /* 0x44 */
  "reserved045",        /* 0x45 */
  "swapbufferCMD",      /* 0x46 */
  "reserved047",        /* 0x47 */
  "swappendCMD",        /* 0x48 */
  "reserved049",        /* 0x49 */
  "reserved04A",        /* 0x4a */
  "reserved04B",        /* 0x4b */
  "reserved04C",        /* 0x4c */
  "reserved04D",        /* 0x4d */
  "reserved04E",        /* 0x4e */
  "reserved04F",        /* 0x4f */
  "fbzColorPath",       /* 0x50 */
  "fogMode",            /* 0x51 */
  "alphaMode",          /* 0x52 */
  "reserved053",        /* 0x53 */
  "fbzMode",            /* 0x54 */
  "stipple",            /* 0x55 */
  "color0",             /* 0x56 */
  "color1",             /* 0x57 */
  "fogColor",           /* 0x58 */
  "zaColor",            /* 0x59 */
  "chromaKey",          /* 0x5a */
  "chromaRange",        /* 0x5b */
  "reserved05C",        /* 0x5c */
  "reserved05D",        /* 0x5d */
  "reserved05E",        /* 0x5e */
  "reserved05F",        /* 0x5f */
  "colBufferSetup",     /* 0x60 */
  "auxBufferSetup",     /* 0x61 */
  "clipLeftRight0",     /* 0x62 */
  "clipTopBottom0",     /* 0x63 */
  "clipLeftRight1",     /* 0x64 */
  "clipTopBottom1",     /* 0x65 */
  "reserved066",        /* 0x66 */
  "reserved067",        /* 0x67 */
  "reserved068",        /* 0x68 */
  "reserved069",        /* 0x69 */
  "reserved06A",        /* 0x6a */
  "reserved06B",        /* 0x6b */
  "reserved06C",        /* 0x6c */
  "reserved06D",        /* 0x6d */
  "reserved06E",        /* 0x6e */
  "reserved06F",        /* 0x6f */
  "fogTable0001",       /* 0x70 */
  "fogTable0203",       /* 0x71 */
  "fogTable0405",       /* 0x72 */
  "fogTable0607",       /* 0x73 */
  "fogTable0809",       /* 0x74 */
  "fogTable0A0B",       /* 0x75 */
  "fogTable0C0D",       /* 0x76 */
  "fogTable0E0F",       /* 0x77 */
  "fogTable1011",       /* 0x78 */
  "fogTable1213",       /* 0x79 */
  "fogTable1415",       /* 0x7a */
  "fogTable1617",       /* 0x7b */
  "fogTable1819",       /* 0x7c */
  "fogTable1A1B",       /* 0x7d */
  "fogTable1C1D",       /* 0x7e */
  "fogTable1E1F",       /* 0x7f */
  "fogTable2021",       /* 0x80 */
  "fogTable2223",       /* 0x81 */
  "fogTable2425",       /* 0x82 */
  "fogTable2627",       /* 0x83 */
  "fogTable2829",       /* 0x84 */
  "fogTable2A2B",       /* 0x85 */
  "fogTable2C2D",       /* 0x86 */
  "fogTable2E2F",       /* 0x87 */
  "fogTable3031",       /* 0x88 */
  "fogTable3233",       /* 0x89 */
  "fogTable3435",       /* 0x8a */
  "fogTable3637",       /* 0x8b */
  "fogTable3839",       /* 0x8c */
  "fogTable3A3B",       /* 0x8d */
  "fogTable3C3D",       /* 0x8e */
  "fogTable3E3F",       /* 0x8f */
  "fbijrInit0",         /* 0x90 */
  "fbijrInit1",         /* 0x91 */
  "fbijrInit2",         /* 0x92 */
  "fbijrInit3",         /* 0x93 */
  "fbijrInit4",         /* 0x94 */
  "fbijrInit5",         /* 0x95 */
  "reserved096",        /* 0x96 */
  "reserved097",        /* 0x97 */
  "reserved098",        /* 0x98 */
  "reserved099",        /* 0x99 */
  "reserved09A",        /* 0x9a */
  "reserved09B",        /* 0x9b */
  "reserved09C",        /* 0x9c */
  "reserved09D",        /* 0x9d */
  "reserved09E",        /* 0x9e */
  "reserved09F",        /* 0x9f */
  "fbijrVersion",       /* 0xa0 */
  "fbiPixelsIn",        /* 0xa1 */
  "fbiChromaFail",      /* 0xa2 */
  "fbiZfuncFail",       /* 0xa3 */
  "fbiAfuncFail",       /* 0xa4 */
  "fbiPixelsOut",       /* 0xa5 */
  "reserved0A6",        /* 0xa6 */
  "reserved0A7",        /* 0xa7 */
  "reserved0A8",        /* 0xa8 */
  "reserved0A9",        /* 0xa9 */
  "reserved0AA",        /* 0xaa */
  "reserved0AB",        /* 0xab */
  "reserved0AC",        /* 0xac */
  "reserved0AD",        /* 0xad */
  "reserved0AE",        /* 0xae */
  "reserved0AF",        /* 0xaf */
  "reserved0B0",        /* 0xb0 */
  "reserved0B1",        /* 0xb1 */
  "reserved0B2",        /* 0xb2 */
  "reserved0B3",        /* 0xb3 */
  "reserved0B4",        /* 0xb4 */
  "reserved0B5",        /* 0xb5 */
  "reserved0B6",        /* 0xb6 */
  "reserved0B7",        /* 0xb7 */
  "reserved0B8",        /* 0xb8 */
  "reserved0B9",        /* 0xb9 */
  "reserved0BA",        /* 0xba */
  "reserved0BB",        /* 0xbb */
  "reserved0BC",        /* 0xbc */
  "reserved0BD",        /* 0xbd */
  "reserved0BE",        /* 0xbe */
  "texChipSel",         /* 0xbf */
  "textureMode",        /* 0xc0 */
  "tLOD",               /* 0xc1 */
  "tDetail",            /* 0xc2 */
  "texBaseAddr",        /* 0xc3 */
  "texBaseAddr1",       /* 0xc4 */
  "texBaseAddr2",       /* 0xc5 */
  "texBaseAddr38",      /* 0xc6 */
  "trexInit0",          /* 0xc7 */
  "trexInit1",          /* 0xc8 */
  "nccTable0_0",        /* 0xc9 */
  "nccTable0_1",        /* 0xca */
  "nccTable0_2",        /* 0xcb */
  "nccTable0_3",        /* 0xcc */
  "nccTable0_4",        /* 0xcd */
  "nccTable0_5",        /* 0xce */
  "nccTable0_6",        /* 0xcf */
  "nccTable0_7",        /* 0xd0 */
  "nccTable0_8",        /* 0xd1 */
  "nccTable0_9",        /* 0xd2 */
  "nccTable0_A",        /* 0xd3 */
  "nccTable0_B",        /* 0xd4 */
  "nccTable1_0",        /* 0xd5 */
  "nccTable1_1",        /* 0xd6 */
  "nccTable1_2",        /* 0xd7 */
  "nccTable1_3",        /* 0xd8 */
  "nccTable1_4",        /* 0xd9 */
  "nccTable1_5",        /* 0xda */
  "nccTable1_6",        /* 0xdb */
  "nccTable1_7",        /* 0xdc */
  "nccTable1_8",        /* 0xdd */
  "nccTable1_9",        /* 0xde */
  "nccTable1_A",        /* 0xdf */
  "nccTable1_B",        /* 0xe0 */
  "reserved0E1",        /* 0xe1 */
  "reserved0E2",        /* 0xe2 */
  "reserved0E3",        /* 0xe3 */
  "reserved0E4",        /* 0xe4 */
  "reserved0E5",        /* 0xe5 */
  "reserved0E6",        /* 0xe6 */
  "reserved0E7",        /* 0xe7 */
  "reserved0E8",        /* 0xe8 */
  "reserved0E9",        /* 0xe9 */
  "reserved0EA",        /* 0xea */
  "reserved0EB",        /* 0xeb */
  "reserved0EC",        /* 0xec */
  "reserved0ED",        /* 0xed */
  "reserved0EE",        /* 0xee */
  "reserved0EF",        /* 0xef */
  "reserved0F0",        /* 0xf0 */
  "reserved0F1",        /* 0xf1 */
  "reserved0F2",        /* 0xf2 */
  "reserved0F3",        /* 0xf3 */
  "reserved0F4",        /* 0xf4 */
  "reserved0F5",        /* 0xf5 */
  "reserved0F6",        /* 0xf6 */
  "reserved0F7",        /* 0xf7 */
  "reserved0F8",        /* 0xf8 */
  "reserved0F9",        /* 0xf9 */
  "reserved0FA",        /* 0xfa */
  "reserved0FB",        /* 0xfb */
  "reserved0FC",        /* 0xfc */
  "reserved0FD",        /* 0xfd */
  "reserved0FE",        /* 0xfe */
  "reserved0FF"         /* 0xff */
};
#endif

#define VG96STRIDE(stride)  (((stride >> 3) & 0x1ff )<< 22)

#ifdef GDBG_INFO_ON
FxU32
get96(FxU32 *addr) 
{
  FxU32 index, ret;
  const char *regName;

  ret = *GETREADADDR2(addr);

  index = GETINDEXFROMADDR(addr);

  regName = iRegNames[index];

  GDBG_INFO((120, "GET:  Register:  %s (0x%x) Address: 0x%x Value: 0x%x\n",
             regName, GETINDEXFROMADDR(addr), addr, ret));

  return ret;
} /* get96 */
#endif

/* structure offset */
#define SETO(base,offset,value)     (*(((FxU32 *)(base))+((offset) >> 2)) = (value))
#define GETO(base,offset)           (*(((FxU32 *)(base))+((offset) << 8)))

/* register indices */
#define SETI(base,index,value)      (*(((FxU32 *)(base))+(index)) = (value))
#define GETI(base,index)            (*(((FxU32 *)(base))+((index) << 10)))

/* structure field names */
#define SETF(base2,field,value)      (((Sstregs*)(base2))->field = (value))
#define GETF(base,field)            (GETO(base,offsetof(Sstregs,field)))


#define GETREGVALFROMENV(val, envStr, constant)\
{\
  const char *envVal;\
  unsigned int u; \
  val = constant;\
  if (((envVal = myGetenv(envStr)) != NULL) && \
      (sscanf(envVal, "%x", &u) == 1)) { \
    val = u; \
    GDBG_INFO((80,\
               "%s:  The environment variable %s == 0x%x overloaded a register value\n", \
               FN_NAME, envStr, val));\
  }\
}

static FxBool
init96LoadBufRegs(int nBuffers, 
                  InitBufDesc_t *pBufDesc, 
                  int xRes, int yRes, 
                  InitRegisterDesc *rd);
static void init96SetupRendering(InitRegisterDesc *regDesc,
		  GrScreenResolution_t sRes);

#define XY_ONE (1<<SST_XY_FRACBITS)

/* draw a right angle triangle */
static void
drawTriangle(volatile Sstregs *sst, int x, int y, int tSize)
{
  SET(sstPtr->vA.x, x);
  SET(sstPtr->vA.y, y);
  SET(sstPtr->vB.x, x+XY_ONE*tSize);
  SET(sstPtr->vB.y, y);
  SET(sstPtr->vC.x, x);
  SET(sstPtr->vC.y, y+XY_ONE*tSize/2);
  
  SET(sstPtr->s, 0);
  SET(sstPtr->t, 0);
  SET(sstPtr->w, 1);
  SET(sstPtr->dsdx, 255);
  SET(sstPtr->dtdx, 0);
  SET(sstPtr->dwdx, 0);
  SET(sstPtr->dsdy, 255);
  SET(sstPtr->dtdy, 0);
  SET(sstPtr->dwdy, 0);
  SET(sstPtr->triangleCMD, 1);
}

#define SENSE0 0xbaaf
#define SENSE1 0xabcd

#define VG96_TEXTURE_OFFSET     0x600000
#define SST96_TEX_PTR(a) \
        ((FxU32 *) (((FxU32) a) + VG96_TEXTURE_OFFSET))

static FxBool sense(FxU32 *sstbase, VG96Info *info, FxU32 tmu,
                   FxU32 mem, FxU32 init)
{
  volatile Sstregs *sst = (Sstregs *) sstbase;
  FxU32 *texAddr = (FxU32 *)SST96_TEX_PTR(sstbase);
  FxBool same = FXFALSE;
  FxU16 fmem, orgmem;


  /* set the Init0 register to enable ? MBytes of memory */
#define WAITLOOP {\
int nWaits;\
  FxU32 waitRes;\
  for (nWaits = 0; nWaits < 50000; nWaits++) {\
    waitRes = sstHW[0x100000];\
    if (waitRes & 0x80000000) break;\
  }\
}

  orgmem = *(FxU16 *)sstbase;

  WAITLOOP;

  /* base address for lod0 */
  mem = mem - ((FXBIT(14) + FXBIT(12) + FXBIT(10) + FXBIT(8) + FXBIT(6) + FXBIT(4) + FXBIT(2) + 1)*8); /* 8X */

  SET(sstPtr->texBaseAddr, mem>>3);

  *(texAddr + (8 << 15)) = init;
  /* 
     write to 2 consequtive locations in lod0 to alter the data
     on the memory signals 
  */
  WAITLOOP;
  *(texAddr + (0)) = 0xffffffff;
  WAITLOOP;
  *(texAddr + (1)) = 0xffffffff;
  
  WAITLOOP;

  drawTriangle(sst,0,0,2);              /* draw a 4x4 right triangle */
  WAITLOOP;

  fmem = *(FxU16 *)sstbase;

  WAITLOOP;

  if (fmem == init)
    same = FXTRUE;

  *(FxU16 *)sstbase = orgmem;

  return same;
}

FX_EXPORT FxBool FX_CSTYLE
Init96GetTmuMemory(FxU32 *sstbase, VG96Info *info, FxU32 tmu,
        FxU32 *TmuMemorySize)
{
  FxU32 data;
  const char *envp;
  
  envp = myGetenv(("SST96_TMUMEM_SIZE"));
  if(envp) {
    *TmuMemorySize = atoi(envp);
    return(FXTRUE);
  }
  SET(sstPtr->trexInit0, 0x05441);

  SET(sstPtr->colBufferSetup, 0x28000000); /* color buffer */
  SET(sstPtr->fbzMode, SST_RGBWRMASK);
  SET(sstPtr->fbzColorPath, SST_RGBSEL_TREXOUT | SST_CC_PASS | SST_ENTEXTUREMAP);
  SET(sstPtr->textureMode, SST_RGB565 | SST_TC_REPLACE | SST_TCA_REPLACE);
  /* set lod8 */
  SET(sstPtr->tLOD, ((8 << 2) | (8 << 7)));

  /* 
     first see if we have 4 Mbytes by writing a texel at 2MB followed by
     a texel at 1MB and 0MB and then rendering using the texel at 2MB
     if we have less memory it should not render correctly since we trash
     the memory locations it would wrap to
  */

  data = sense(sstbase,info,tmu,0x200000, SENSE0);
  if (data) {
    data = sense(sstbase,info,tmu,0x200000, SENSE1);
    if (data) {
      *TmuMemorySize = 4; 
      return(FXTRUE);
    }
  }

  data = sense(sstbase,info,tmu,0x100000, SENSE0);
  if (data) {
    data = sense(sstbase,info,tmu,0x100000, SENSE1);
    if (data) {
      *TmuMemorySize = 2;
      return(FXTRUE);
    }
  }
  
  data = sense(sstbase,info,tmu,0x000000, SENSE0);
  if (data) {
    data = sense(sstbase,info,tmu,0x000000, SENSE1);
    if (data) {
      *TmuMemorySize = 1; 
      return(FXTRUE);
    }
  }

#undef WAITLOOP

  return(FXFALSE);
}

/*---------------------------------------------------------------------------
   NOTES:
        assumes that board and registers are initialized
        destroys part of the framebuffer
  ---------------------------------------------------------------------------*/
FX_EXPORT FxBool FX_CSTYLE
sst96InitGetTmuInfo(FxU32 *sstbase, VG96Info *info)
{
  sstPtr = 
    (volatile Sstregs *)(SST96_COMMAND_REGS(sstbase));

  if(Init96GetTmuMemory(sstbase, info, 0, &info->tfxRam) == FXFALSE)
    return(FXFALSE);
  return(FXTRUE);
}

/*-------------------------------------------------------------------
  Function: init96MapBoard
  Date: 10/17
  Implementor(s): jdt, dow
  Library: init96
  Description:
  Looks for an SST-96 enabled VGA board on the system and maps that
  boards physical address into linear memory.
  Arguments:
  rd:     Register descriptions
  vID:    Vendor ID
  dID:    Device ID
  Return:
  linear pointer to base of VGA board. 0 if fail
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96MapBoard, FxBool , (void *rd, void *info, FxU16 vID, FxU16 dID) )
{
#define FN_NAME "init96MapBoard"

  InitRegisterDesc
    *regDesc = (InitRegisterDesc *) rd;

  VG96Info *vg96Info = (VG96Info *) info;
  
  FxU32
    vendorID = vID,
    deviceID = dID;
  
  GDBG_INFO((80, "SST-96 Initialization code\n"));

  /* !!! FIXME hack for vxd bug */
  putenv( "SST_DUALHEAD=1" );
  
  switch (vID) {
  case AT3DVID:
    curHALData = initAT3DGetHALData();
    break;
  case MCRXVID:
    curHALData = initMCRXGetHALData();
    break;
  }

  GDBG_INFO((80, "init96MapBoard:  Mapping card into linear memory\n"));
  
  regDesc->hwDep.VG96RegDesc.baseAddress =
    pciMapCardMulti( vendorID, deviceID, curHALData->memSize, &devNum, 0 , 0);


  if (regDesc->hwDep.VG96RegDesc.baseAddress == NULL) {
#ifdef __WIN32__
    MessageBox(NULL, pciGetErrorString(), NULL, MB_OK);
#elif defined(__linux__)
    fprintf(stderr, "Failed to map board: %s\n", pciGetErrorString());
#else /* DOS */
    /* Set Mode 3 before going */
    union REGS
      inR, outR;
    inR.w.ax = 0x0003;
    int386(0x10, &inR, &outR);
    fprintf(stderr, "%s\n", pciGetErrorString());
#endif /* __WIN32__ */
    exit(1);
    return FXFALSE;
  }

  /* need to initialize sstHW ASAP */
  sstHW  = regDesc->hwDep.VG96RegDesc.baseAddress;

  GDBG_INFO((80, "%s:  Mapped Voodoo Rush board at 0x%x\n", 
             FN_NAME, regDesc->hwDep.VG96RegDesc.baseAddress));

  if (curHALData->initGetInfo)
    (*curHALData->initGetInfo)(vg96Info);

  fifoptr = regDesc->hwDep.VG96RegDesc.fifoApertureBase;

  GDBG_INFO((88, "init96MapBoard sstHW = %x\n", sstHW));
  
  /* CHD NOTE:  The routine init96SetupRendering MUST Be called from this
     routine or all hell brakes loose.
     */

  /* detect tmu memory size */
#ifdef __linux__
  {
    FxBool rv=FXTRUE;
    GrScreenResolution_t    res = GR_RESOLUTION_640x480;

    if (curHALData->initAperture)
      (*curHALData->initAperture)(regDesc);
       if ( curHALData->initEnableRegs )
       rv = curHALData->initEnableRegs( regDesc );
    if ( !rv )
      return FXFALSE;
    
    init96SetupRendering(regDesc, res);
    
    sst96InitGetTmuInfo((FxU32 *)sstHW, vg96Info);
    }
#else
#ifdef __DOS32__
  {
    /* In dos mode, we do it the way it is supposed to be.
       In windows mode, calling initEnableRegs() will change the clock
       rate and it makes the refresh looks bad. So we still use the 
       init96SetVideo() to set up the rendering mode etc.
       */
    GrScreenResolution_t    res = GR_RESOLUTION_640x480;
    FxBool rv = FXTRUE;

    if (curHALData->initAperture)
      (*curHALData->initAperture)(regDesc);

    {
      union REGS
        inR, outR;
      
      inR.w.ax = 0x4f03;
      int386(0x10, &inR, &outR);
      
      oldVidMode = outR.w.bx;
      GDBG_INFO((80, "%s:  Saving Current video mode (0x%x).\n",
                 FN_NAME, oldVidMode)); 
    }
    
    if ( !rv )
      return FXFALSE;
    
    if ( curHALData->initEnableRegs )
      rv = curHALData->initEnableRegs( regDesc );
    if ( !rv )
      return FXFALSE;
    
    init96SetupRendering(regDesc, res);
    
    sst96InitGetTmuInfo((FxU32 *)sstHW, vg96Info);
  }
#else
  {
    int xres=1, yres=1, fbStride;
    GrScreenResolution_t    res = GR_RESOLUTION_NONE;
    FxBool rv =
    init96SetVideo(0, res,
                   0, 2, 1, 
                   2, 1, regDesc,
                   &xres, &yres, &fbStride );
    if ( !rv )
      return FXFALSE;
    sst96InitGetTmuInfo((FxU32 *)sstHW, vg96Info);
    init96RestoreVideo(regDesc);
  }
#endif
#endif

  GDBG_INFO((80, "init96MapBoard:  returning FXTRUE\n"));
  
  return FXTRUE;

#undef FN_NAME
} /* init96MapBoard */


#ifndef GDBG_INFO_ON
#define WAITLOOP {\
int nWaits;\
volatile FxU32 waitRes;\
  for (nWaits = 0; nWaits < 100000; nWaits++) {\
   waitRes = GET(sstHW[0x90 + 0x100000]);\
    if (waitRes & 0x80000000) break;\
  }\
}
#else
#define WAITLOOP {\
int nWaits;\
  volatile FxU32 waitRes;\
    for (nWaits = 0; nWaits < 50000; nWaits++) {\
      waitRes = sstHW[0x100000];\
      if (waitRes & 0x80000000) break;\
  }\
}
#endif

#define SWAPPENDING(status) ((status & 0x70) >> 4)

/*-------------------------------------------------------------------
  Function: init96SetupRendering
  Date: 03-Sep-97
  Implementor(s): dow
  Library: Init Code for 96
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
static void
init96SetupRendering(InitRegisterDesc *regDesc, GrScreenResolution_t sRes)
{
#define FN_NAME "init96SetupRendering"
  FxU32 regVal;

  sstHW  = regDesc->hwDep.VG96RegDesc.baseAddress;
  sstPtr = 
    (volatile Sstregs *)(SST96_COMMAND_REGS(regDesc->hwDep.VG96RegDesc.baseAddress));

  /*------------------------------------------------------------
    Initialize Jr and TMU
    ------------------------------------------------------------*/
  GDBG_INFO((80, "%s:  Software reset\n", FN_NAME));
  SET(sstHW[0x90 + 0x100000], 0xf601); /* (0xf601) init0, sw reset. */

  WAITLOOP;

  GDBG_INFO((80, "%s:  Zero Out CMD FIFO registers\n", FN_NAME));
  SET(sstHW[0x100000 + 0x95], 0);
  SET(sstHW[0x100000 + 0x94], 0);
  SET(sstHW[0x100000 + 0x93], 0);

  GDBG_INFO((80, "%s:  Setting up Mem FIFO\n", FN_NAME));

  GETREGVALFROMENV(regVal, "SST96_FBIJRINIT2", 0x70d20);

  SET(sstHW[0x92 + 0x100000], regVal);

  /* Off for Host Blt, On for LFB command traffic 
     Low Priority Request Aging - wait 64 cycles
     before answering low priority requests from
     AT3D on PUMA - 3F may not be optimal, should
     be tweaked later, !!JDT
     [24]   - PUMA Texture Address Space Mode      - 0x1 - 2MB
     [23]   - PUMA Texture Base Address Mode       - 0x1 - 6MB
     [12:7] - PUMA Low Priority Grant Limit Cnt    - 0x0 
     [6]    - PUMA Low Prorirty Grant Limit Enable - 0x0 
     [5:0]  - PUMA Low Priority Request Delay Cnt  - 0x1f - 32 ccls
     Setting to 0x3f ( max ) seems to cripple windows/d3d
     maybe blt operations suffer???
     */
  GDBG_INFO((80, "%s:  Setting low priority request aging.\n", FN_NAME));
  /* 
   * If we're running at a high resolution, don't starve AT3D's 
   * video refresh.   XXX FIXME for AT3d only
   */
  if (sRes == GR_RESOLUTION_NONE) {
    /* set the lpr delay to 0. It was 0x10 and the video refresh was not correct */
    GETREGVALFROMENV(regVal, "SST96_FBIJRINIT1", 0x01800000);
    SET(sstHW[0x91 + 0x100000], regVal);
  } else {
    GETREGVALFROMENV(regVal, "SST96_FBIJRINIT1", 0x0180003f);

    if (myGetenv("SST96_MEMORYHINT")) {
      regVal |= (0x1f << 13);
    }
    SET(sstHW[0x91 + 0x100000],regVal);
  }

  /* Come out of Reset 
     [17]    - Serial Status Xfer Mode     - 0x0 - rising edge
     [16:12] - TREX-To-FBI High Water Mark - 0xf
     [11:8]  - TREX-To-FBI Bus Clock Delay - 0x6
     */
  
  GDBG_INFO((80, "%s:  Coming out of FBI reset\n", FN_NAME));
  GETREGVALFROMENV(regVal, "SST96_FBIJRINIT0", 0xf600);
  SET(sstHW[0x90 + 0x100000], regVal); /* Bit 1 disables tmu */

  WAITLOOP;

  GDBG_INFO((80, "%s:  Setting TMU FT & TF delays\n", FN_NAME));
  {
    FxU32 trexinit0, trexinit1;
    const char *envp;

    envp = myGetenv(("SST_TREX0INIT0"));
    if( !envp ||
        (sscanf(envp, "%i", &trexinit0) != 1) ) {
      trexinit0 = 0x05441;      /* TREXINIT0 */
    }

    envp = myGetenv(("SST_TREX0INIT1"));
    if( !envp ||
        (sscanf(envp, "%i", &trexinit1) != 1) ) {
      trexinit1 = 0x3643c; /* TREXINIT1 */
    }

    SET(sstHW[0xc7 + 0x100000], trexinit0); /* TREXINIT0 */
    SET(sstHW[0xc8 + 0x100000], trexinit1); /* TREXINIT1 */
  }
  
  WAITLOOP;
  
#undef FN_NAME
} /* init96SetupRendering */

/*-------------------------------------------------------------------
  Function: initSet96Video
  Date: 10/17
  Implementor(s): jdt, murali
  Library: init96
  Description:
  
  1 - Set video mode
  2 - Enable Direct Writes to Jr
  3 - Put Jr registers in Reset
  3 - Set up Color Buffer Registers
  
  Arguments
  
  Return:
  FXFALSE - fail 
  FXTRUE  - pass
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96SetVideo, FxBool , 
              (  FxU32                   hWnd,
               GrScreenResolution_t    sRes,
               GrScreenRefresh_t       vRefresh,
               InitColorFormat_t       cFormat,
               InitOriginLocation_t    yOrigin,
               int                     nColBuffers,
               int                     nAuxBuffers,
               InitRegisterDesc        *regDesc,
               int                     *xres,
               int                     *yres,
               int                     *fbStride) )
{
#define FN_NAME "init96SetVideo"
  FxBool rv = FXTRUE;
  InitSwapType_t swapType = 0;
  InitBufDesc_t  bufDesc[NUM_BUFS];
  FxU32 nTries = 0, status;
  FxU32 regVal;
  int i;

  for (i = 0; i < NUM_BUFS; i++)
    bufDesc[i].bufType = INIT_BUFFER_NONE;

  if (myGetenv("SST96_FORCE_TRIPLE"))
    nColBuffers = 3;

  GDBG_INFO((80, "%s:  %d Color Buffers\n", FN_NAME, nColBuffers));

  GDBG_INFO((80, "(%s) hWnd=%x, sRes=%d, yOrg=%d, ncb=%d, nab=%d\n", 
             FN_NAME, hWnd, sRes, yOrigin, nColBuffers, nAuxBuffers));

  /* YO!  Fix this comment
     XXX this needs to be moved to post SetVideo code ??? */
  GDBG_INFO((80, "%s:  Initializing Command Aperture\n", FN_NAME));

  if (curHALData->initAperture)
    (*curHALData->initAperture)(regDesc);

#ifdef __DOS32__
  {
    union REGS
      inR, outR;

    inR.w.ax = 0x4f03;
    int386(0x10, &inR, &outR);

    oldVidMode = outR.w.bx;
    GDBG_INFO((80, "%s:  Saving Current video mode (0x%x).\n",
               FN_NAME, oldVidMode)); 
  }
#endif /* __DOS32__ */

  /*-------------------------------------
    Set Video Mode
    -------------------------------------*/
  /* this goes through the dxdrvr.c which is a misnomer
     which will persist for the time being, in dos
     this does not have anything to do with directX */
#ifdef __linux__
  if (!linOpen(hWnd, sRes, vRefresh, cFormat, yOrigin, 
              nColBuffers, nAuxBuffers, bufDesc, 
              xres, yres, &swapType))
#else
  if (!dxOpen(hWnd, sRes, vRefresh, cFormat, yOrigin, 
              nColBuffers, nAuxBuffers, bufDesc, 
              xres, yres, &swapType))
#endif
  { GDBG_INFO((80, "Failed dxOpen\n"));
    rv = FXFALSE;
    goto BAIL;
  }

#ifdef GDBG_INFO_ON
  for (i = 0; i < nColBuffers; i++) {
    GDBG_INFO((80, "%s:  bufDesc[%d]:\n", FN_NAME, i));
    GDBG_INFO((80, "%s:  bufDesc.bufType = %s\n",
               FN_NAME, bufTypeNames[bufDesc[i].bufType]));
    GDBG_INFO((80, "%s:  bufDesc.bufOffset = 0x%x\n", 
               FN_NAME, bufDesc[i].bufOffset ));
    GDBG_INFO((80, "%s:  bufDesc.bufStride = 0x%x\n", 
               FN_NAME, bufDesc[i].bufStride));
    GDBG_INFO((80, "%s:  bufDesc.bufBPP = 0x%x\n",
               FN_NAME, bufDesc[i].bufBPP));
  }
#endif

  if ( !rv ) goto BAIL;

  /*-------------------------------------
    Enable Direct Writes to JR
    -------------------------------------*/
  if ( curHALData->initEnableRegs )
    rv = curHALData->initEnableRegs( regDesc );
  if ( !rv ) goto BAIL;


  init96SetupRendering(regDesc, sRes);

  cachedYOrigin  = yOrigin;     /* Used later for init96Control */
  cachedSwapType = swapType;

  /*
  **  Set Up Color Buffer Registers & triple buffering if necessary.
  */
  maxBuffer = (nColBuffers < 2) ? 2 : nColBuffers;
  maxBufIdx = maxBuffer - 1;

  GDBG_INFO((80, "%s:  maxBuffer = %d, maxBufIdx = %d\n",
             FN_NAME, maxBuffer, maxBufIdx));

  /* Send as many swapbuffers as status says are necessary */
  {
    FxU32
      pending = 7;              /* # Swaps pending */

    GDBG_INFO((80, 
               "%s:  Sending %d swapbufferCMDs\n", 
               FN_NAME, 
               pending));
    while (pending) {
      FxU32
        tries = 0;

      GDBG_INFO((80, "%s: %d swaps pending\n", FN_NAME, pending))  ;

      status =  sstHW[0x100000];
      pending = SWAPPENDING(status);

      while (pending--) {
        SET(sstPtr->swapbufferCMD, 1);
        status =  sstHW[0x100000];              /* bug #1152 */
        ADVANCE_BUFFERS(dfb);
        WAITLOOP;
      }

      do {
        ++tries;
        status =  sstHW[0x100000];

        GDBG_INFO((120, "%s:  Serial status = 0x%x\n", FN_NAME, status));
        pending = SWAPPENDING(status);
        if (tries > MAXTRIES) {
          GDBG_INFO((80, "%s:  Bailing out after %d tries to get swap buffer pending to decrement\n", FN_NAME, tries));
          rv = FXFALSE;
          goto BAIL;
        }
      } while (pending);
    }

    /* Clear counters with a NOP */
    SET(sstHW[0x42 + 0x100000], 1); /* NOP cmd */

    /* Wait for idle */
    do {
      if (nTries > MAXTRIES) {
        GDBG_INFO((80, 
                   "%s: Bailing out and returning false after %d checks for idle.\n", FN_NAME, nTries-1));
        return FXFALSE;
      }
      ++nTries;
      status = sstHW[0x100000];

      GDBG_INFO((80, 
                 "%s: Serial status after soft reset: 0x%x\n", 
                 FN_NAME, 
                 status));
    } while (status & 0x1);
  } /* end of swap buffer block */

  if (tripleBuffering) {
    GETREGVALFROMENV(regVal, "SST96_FBIJRINIT0", 0xf600);
    regVal |= (1 << 2);
    GDBG_INFO((80, "%s:  Enabling triple buffering.\n", FN_NAME));
    /* dram read to write fast timing enable. bit 7 */
    if (myGetenv("SST96_MEMORYHINT")) {
      regVal |= (0x1 << 7);
    }
    SET(sstHW[0x90 + 0x100000], regVal); /* Bit 1 disables tmu */
  }

  /* Need to be converted to indices dfbFront.... rfbFront.... */

  dfbFrontIdx =   0;
  dfbBackIdx  =   1; 
  dfbRenderIdx =  dfbBackIdx;

  rfbFrontIdx =   0;
  rfbBackIdx  =   1; 
  rfbRenderIdx =  rfbBackIdx;

  rv = init96LoadBufRegs(NUM_BUFS, bufDesc, *xres, *yres, regDesc);
  if ( !rv ) {
    GDBG_INFO((80, "Failed init96LoadBufRegs\n"));
    goto BAIL;
  }

  /*-------------------------------------
    Set up Page Flipping
    -------------------------------------*/
  WAITLOOP;
#if !defined( INIT96VGASWAP ) && !defined( __linux__ )
  {
    FxU32 buf[MAX_BUF];
    int i;

    for (i=0; i<maxBuffer; i++) buf[i] = bI[i].dfb;

    /* Explicitly clear swap control things */
    curHALData->initSwapTiles = NULL;

    if (curHALData->initUseTiles && (swapType == INIT_SWAP_FLIP)) {
#ifdef H3D
      (*curHALData->initUseTiles)(regDesc, 
                                buf,
                                maxBuffer,
                                bI[0].dfbStride,
                                *xres, 
                                *yres,
                                GR_RESOLUTION_IS_AUTOFLIPPED(sRes) );
#else
      (*curHALData->initUseTiles)(regDesc, 
                                buf,
                                maxBuffer,
                                bI[0].dfbStride,
                                *xres, 
                                *yres );
#endif
    }
  }
#endif

  *fbStride = (bI[1].dfbStride>>1);
  GDBG_INFO(( 200, "frame buffer stride: %d\n", *fbStride ));

 BAIL:
  return rv;
#undef FN_NAME
} /* init96SetVideo */


/*-------------------------------------------------------------------
  Function: init96RestoreVideo
  Date: 3/16
  Implementor(s): jdt
  Library: init96
  Description:
  
  Disable Direct Hardware Access
  Restore the video to it's initial state
  
  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96RestoreVideo, void, ( InitRegisterDesc *regDesc )) {
#define FN_NAME "init96RestoreVideo"
  if ( curHALData->initDisableRegs )
    curHALData->initDisableRegs( regDesc );
  
#if  defined( __WIN32__ ) && !defined( INIT_HOOPTI )
  dxClose();
#elif defined( __DOS32__ )
  {
    static union REGS
      inR, outR;

    GDBG_INFO((80, "%s:  Restoring VESA Mode 0x%x\n", FN_NAME, oldVidMode));
    inR.w.ax = 0x4f02;
    inR.w.bx = oldVidMode;

    int386( 0x10, &inR, &outR );
  }
#elif defined( __linux__ )
  linClose();
#endif
#undef FN_NAME
} /* init96RestoreVideo */


/*-------------------------------------------------------------------
  Function: init96EnableTransport
  Date: 3/17
  Implementor(s): jdt
  Library: init96
  Description:
  
  Enable the JR command transport and return pertinant fifo 
  information to the caller
  
  Arguments:
  info - structure to fill with info about the FIFO
  Return:
  FXTRUE  - succeed
  FXFALSE - fail
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96EnableTransport, FxBool, ( InitFIFOData *info, InitRegisterDesc* regDesc)) 
{
#define FN_NAME "init96EnableTransport"
  FxBool rv = FXTRUE;
  FxU32 lo, hi, sz, th;
  FxU32
    fifoPhysBaseAddress,
    fifoPhysSize;
  FxBool
    res;
  PciMemType
    fifoMemType;
  
  GDBG_INFO((80, "%s\n", FN_NAME));
  
  /*------------------------------------------------------------
    Enable the Off Chip Fifo
    ------------------------------------------------------------*/
  
  /* These are defined in sst.c for now */
  lo = (fifoStart >> 12);     /* physaddr of 1st 4k page */
  hi = (fifoEnd   >> 12);     /* physaddr of last 4k page */
  sz = (hi - lo + 1) << 12;   /* bytes */
  th = (hi - lo) - 3;         /* threshold, in 4k pages. */
  
  /* Generate virtual addresses for the pointers. */
  fifovirt = (FxU32 *)&sstHW[lo << 10];
  fifophys = lo << 12;
  fifomax  = sz;
  fifocur  = 0;
  
  fifoptr  = fifovirt;
  fifosize = 4096;            /* initially, set to 4k */
  
  fifolfb = 1;
  
  /* First disable the off-chip fifo */
  GDBG_INFO((80, "%s:  Disabling off-chip FIFO\n", FN_NAME));
  SET(sstHW[0x100000 + 0x93], 0);
  
  /* FbijrInit4 - 0x94 - entry count */
  SET(sstHW[0x100000 + 0x94], 0);
  
  /* FbijrInit5 - 0x95 - read pointer */
  SET(sstHW[0x100000 + 0x95], lo << 9);

  /* Default size to page checking mode. 
   *
   * NB: This will get overwritten later if we're using
   * the other wrapping code.
   */
  info->hwDep.vg96FIFOData.fifoSize = fifosize;

  /* Store fifo data to info */
  info->hwDep.vg96FIFOData.fifoVirt = (FxU32*)fifovirt;
  info->hwDep.vg96FIFOData.fifoMax  = fifomax;
  info->hwDep.vg96FIFOData.fifoPhys = fifophys;
  info->hwDep.vg96FIFOData.fifoCur  = fifocur;
  info->hwDep.vg96FIFOData.fifoLfb  = fifolfb;

  /* Set up alternate wrapping values. 
   *
   * NB: This *MUST* come before the processor dependent
   * fifo ptr advancing that we do below.
   */
#if SST96_ALT_FIFO_WRAP
  /* Assume hw is still at the start of the fifo */
  info->hwDep.vg96FIFOData.fifoRead      = (FxU32)fifovirt;

  info->hwDep.vg96FIFOData.roomToEnd     = fifomax;

  info->hwDep.vg96FIFOData.fifoSize      =
  info->hwDep.vg96FIFOData.roomToReadPtr = fifomax - sizeof(FxU32);

  info->hwDep.vg96FIFOData.regDesc = regDesc;
#endif /* SST96_ALT_FIFO_WRAP */

#if GL_X86
  if (info->cpuType == 6) {
    /*
     **  Now, write 128 DWORDS of NULL to the FIFO, so the FIFO read
     **  pointer trails the FIFO write pointer by 128 DWORDS.  The state
     **  setting code fences every 128 DWORDS.
     */
    GDBG_INFO((125, "%s:  Advancing write pointer 128 DWORDS beyond read.\n",
               FN_NAME));
    do {
      *fifoptr++ = 0L;
      info->hwDep.vg96FIFOData.fifoSize -= 4;
    } while (((FxU32) fifoptr) & 0xff);
  } else {
    GDBG_INFO((125, "%s:  No need for write pointer advance.\n",
               FN_NAME));
  }
#endif

  /*
  **  NOTE TO ALL FOOLS WHO TREAD HERE
  **
  **  It is VITALLY important to enable the FIFO AFTER the
  **  P6 Lag Of Love(tm), instead of before, otherwise the FIFO will
  **  become horribly corrupted.
  */

  /* FbiJrInit3 - 0x93 - Load up offchip fifo register */
  GDBG_INFO((80, "%s:  Loading up of offchip fifo register\n", FN_NAME));
  SET(sstHW[0x100000 + 0x93], 1 | (lo <<  1) | (hi << 11) | (th << 21));

  /* Set the fifo ptr now that we really know where its going to start
   * for the client code. 
   */
  info->hwDep.vg96FIFOData.fifoPtr  = (FxU32*)fifoptr;

  /* Turn on Write combining for the FIFO  */
  pciGetConfigData(PCI_BASE_ADDRESS_0, devNum, &physAddr);

  fifoPhysBaseAddress = physAddr + fifoStart;
  fifoPhysSize = fifoEnd - fifoStart;
  fifoMemType = PciMemTypeWriteCombining;

  res = pciFindMTRRMatch(fifoPhysBaseAddress, fifoPhysSize,
                         fifoMemType, &fifoMTRRNum);

  if (!res) {
    res = pciFindFreeMTRR(&fifoMTRRNum);
  }

  if (res)
    pciSetMTRR(fifoMTRRNum, fifoPhysBaseAddress, fifoPhysSize, fifoMemType);
  else {
    GDBG_INFO((80, "%s:  Couldn't get free or used MTRR!\n"));
  }

  return rv;

#undef FN_NAME
} /* init96EnableTransport */

/*-------------------------------------------------------------------
    Function: init96DisableTransport
    Date: 3/16
    Implementor(s): jdt
    Library: init96
    Description:
    Put the command transport in a safe state
    Arguments:
    Return:
    -------------------------------------------------------------------*/
INITVG96ENTRY(init96DisableTransport, void, ( void ))
{
#define FN_NAME "init96DisableTransport"
  while (*(volatile FxU32 *)sstPtr & 1);
  
  SET(sstHW[0x100000 + 0x90], 0xf601); /* (0xf601) init0, sw reset. */
  SET(sstHW[0x100000 + 0x95], 0);
  SET(sstHW[0x100000 + 0x94], 0);
  SET(sstHW[0x100000 + 0x93], 0);

#if SST96_ALT_FIFO_WRAP
  GDBG_INFO((80, "%s: Fifo Statistics - Wrapped:%ld Stalled:%ld\n",
             numWraps, numStalls));
#endif /* SST96_ALT_FIFO_WRAP */

  /* Clear the MTRR */
  pciSetMTRR(fifoMTRRNum, 0, 0, 0);
#undef FN_NAME
} /* init96DisableTransport */

/*-------------------------------------------------------------------
  Function: init96Swap
  Date: 13-Feb-97
  Implementor(s): jdt, dow, murali
  Library: Init Code for VG96
  Description:
  Performs VG96 buffer swap
  Arguments:
  code:           usually vsync wait option
  wcb:            method for writing to hardware
  Return:
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96Swap, void ,
              (FxU32 code, InitRegisterDesc *rd, Init96WriteMethod *wcb) )
{
#define FN_NAME "init96Swap"
  FxU32 status;

  GDBG_INFO((80, "init96Swap()\n"));

  /*
   * For improved speed we take the following steps
   * 1. remove the status checking
   * 2. modify grBufferNumPending so it does not need to do P6 nudge code
   * 3. add one p6 nudge code
   * Here is step 1. Goto grBufferNumPending() in gglide.c to continue.
   *
   do {
   GDBG_INFO((120, "%s:  Serial status = 0x%x\n", FN_NAME, status));
   status = *rd->hwDep.VG96RegDesc.serialStatus;
   } while (status & 0x1);
   */

  if (cachedSwapType == INIT_SWAP_FLIP) {
#ifdef __linux__

    FxU32
      offset;
    /* Wait for idle */
    do {
      status = sstHW[0x100000];
    } while (status & 0x1);

    offset = bI[rfbRenderIdx].dfbBase;
    linFlip(offset);

#else
#ifndef INIT96VGASWAP
    /* Direct Write to the Swap Pending register */
    GDBG_INFO((80, "init96Swap: Sending sapppendCMD\n"));
    GDBG_INFO((80, "  Direct Write of register 0x%x with value 0x%x\n", ((((FxU32) (&sstPtr->swappendCMD)) - (((FxU32) sstPtr) + 0x100000)) >> 2), 1));

    sstPtr->swappendCMD = 0x1;

  {
    static FxBool gotEnv = FXFALSE;
    static FxBool overrideSwap;
    static FxU32 overrideVal;

    if (gotEnv == FXFALSE) {
      const char *envVal;
      if ((envVal = myGetenv("SST96_INITSWAPCOUNT")) != NULL) {
        sscanf(envVal, "%x", &overrideVal);
        GDBG_INFO((80, "%s:  environment SST96_INITSWAPCOUNT (0x%x) overrode code\n",
                   FN_NAME, overrideVal));
        overrideSwap = FXTRUE;

      }
      gotEnv = FXTRUE;
    }

    if (overrideSwap == FXTRUE)
      code = overrideVal;
  }

    GDBG_INFO((80, "%s:  Sending swapbufferCMD (0x%x) via FIFO callback\n",
               FN_NAME, code));

    wcb( (FxU32*)&(sstPtr->swapbufferCMD), 1 );
    status =  sstHW[0x100000];          /* bug #1152 */

    /* Use arrays & rotate for triple buffering */

#else  /* Swapping With VGA */
    /*
     * This is for swapping by changing the VGA serial start register.
     * a.k.a "Page flipping"
     */
    FxU32
      offset;
    FxU8
      tmp,
      serialOverflow;

    GDBG_INFO((210, "%s: Swapping With VGA\n", FN_NAME));

    /* Wait for idle */
    do {
      status = sstPtr->status & 0x1;
    } while (status & 0x1);

    offset = bI[rfbRenderIdx].dfb;

    GDBG_INFO((80, "%s:  Swapping to offset 0x%x\n", FN_NAME, offset));
    /* Fetch serial overflow and clear low nibble for serial start */
    CRTC_GET(AR_CRTC_SERIAL_OVERFLOW_INDEX, serialOverflow);
    serialOverflow &= 0xF0;
    
    /* Make sure scanout within active display */
    do {
      INP(VGA_INPUT_STATUS_1C, tmp);
    } while (tmp & VIS1C_PIXEL_DISPLAY_INACTIVE);
    
    /* Set serial address */
    /* For some reason unknown to me, this is set to the byte offset / 4,
       or perhaps the offset in dwords */
    
    /* Top nibble */
    tmp = (FxU8) ((offset >> (2+16)) & 0x0F);
    CRTC_SET(AR_CRTC_SERIAL_OVERFLOW_INDEX, serialOverflow | tmp);
    
    /* Middle byte */
    tmp = (FxU8) ((offset >> (2+8)) & 0xFF);
    CRTC_SET(VR_CRTC_SERIAL_START_ADDRESS_HI_INDEX, tmp);
    
    /* Low byte */
    tmp = (FxU8) ((offset >> 2) & 0xFF);
    CRTC_SET(VR_CRTC_SERIAL_START_ADDRESS_LO_INDEX, tmp);
    
    /* Save away */
    do {
      INP(VGA_INPUT_STATUS_1C, tmp);
    } while (tmp & VIS1C_VERTICAL_RETRACE_ACTIVE);

    do {
      INP(VGA_INPUT_STATUS_1C, tmp);
    } while (!(tmp & VIS1C_VERTICAL_RETRACE_ACTIVE));
#endif
#endif

    /* dpc - 26 jan 1998
     * Some hw, macronix, requires that we diddle w/ even more bits
     * when we do the swap. 
     */
    if(curHALData->initSwapTiles != NULL) (*curHALData->initSwapTiles)(rd);

    ADVANCE_BUFFERS(rfb);

    GDBG_INFO((80, "%s:  Setting colBufferSetup (0x%x, 0%x) via FIFO callback\n",
               FN_NAME, bI[rfbRenderIdx].rfb,
               bI[rfbRenderIdx].rfb));
    wcb( (FxU32*)&(sstPtr->colBufferSetup), bI[rfbRenderIdx].rfb );

  } else {
    while ((*rd->hwDep.VG96RegDesc.serialStatus) & 0x1);
#ifdef __linux__
    do {
      status = sstHW[0x100000];
    } while (status & 0x1);
    linSwap(code);
#else
    dxSwap(code);
#endif
  }
  GDBG_INFO((80, "%s:  returning\n", FN_NAME));
  return;
#undef FN_NAME
} /* init96Swap */


/*-------------------------------------------------------------------
  Function: init96Idle
  Date: 13-Feb-97
  Implementor(s): dow, jdt, murali
  Library: Init Code for VG96
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96Idle, void , (Init96WriteMethod *wcb) )
{
#define FN_NAME "init96Idle"

  volatile FxU32 *statReg = (volatile FxU32 *) &sstPtr->status;

  wcb((FxU32*)&sstPtr->nopCMD, 0);

  while ( (*statReg) & 0x1 ) { };

#undef FN_NAME
} /* init96Idle */

/*-------------------------------------------------------------------
  Function: init96GetBufferPtr
  Date: 2/25
  Implementor(s): jdt
  Library: init code for vg96
  Description:
  Get a pointer to a specified buffer ( changes at swaps )
  Arguments:
  buffer - constant defining buffer
  Return:
  void opinter to buffer
  -------------------------------------------------------------------*/
#define LFB_OFFSET( X ) ( X & 0x3fffff ) /* was 0x1fffff */

INITVG96ENTRY(init96GetBufferPtr, void*, (InitBuffer_t buffer, int *strideBytes)) 
{
#define FN_NAME "init96GetBufferPtr"
  void *rv = 0;

  GDBG_INFO((80,"%s\n", FN_NAME));

  switch( buffer ) {
  case INIT_BUFFER_FRONTBUFFER:
    GDBG_INFO((80, "  get front buffer pointer" ));
    rv = (void*)(((char*)sstHW)+bI[rfbFrontIdx].dfbBase);
    *strideBytes = bI[rfbFrontIdx].dfbStride;
    break;
  case INIT_BUFFER_TRIPLEBUFFER:
    rv = (void*)(((char*)sstHW)+bI[rfbTripleIdx].dfbBase);
    *strideBytes = bI[rfbTripleIdx].dfbStride;
    GDBG_INFO((80, "  get triple buffer pointer" ));
    break;
  case INIT_BUFFER_BACKBUFFER:
    rv = (void*)(((char*)sstHW)+bI[rfbBackIdx].dfbBase);
    *strideBytes = bI[rfbBackIdx].dfbStride;
    GDBG_INFO((80, "  get back buffer pointer" ));
    break;

  case INIT_BUFFER_AUXBUFFER:
    rv = (void*)(((char*)sstHW) + LFB_OFFSET(ab0Base));
    /* [dBorca]
     * used to be ab0, but I changed it to ab0Base.
     * VG96STRIDE(ab0Stride) doesn't really matter,
     * because we stripped off the high bits anyway!
     */
    *strideBytes = ab0Stride;
    GDBG_INFO((80, "  get aux buffer pointer" ));
    break;
  }

  GDBG_INFO((80, "%s:  strideBytes %.05d\n", FN_NAME, *strideBytes ));
  GDBG_INFO((80, "%s:  returning   0x%x\n", FN_NAME, rv ));

  return rv;
#undef FN_NAME
} /* init96GetBufferPtr */
#undef LFB_OFFSET

/*-------------------------------------------------------------------
  Function: init96RenderBuffer
  Date: 2/25
  Implementor(s): jdt
  Library: init96 
  Description:
  Set the current render buffer
  Arguments:
  buffer - which buffer to send render ops to
  Return:
  none
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96RenderBuffer, void, (InitBuffer_t buffer,Init96WriteMethod *wcb)) 
{
#define FN_NAME "init96RenderBuffer"
  GDBG_INFO((80,"%s\n", FN_NAME));

  /* Mess around with indeces here i.e. rfbRender = rfbBack ....*/
  switch( buffer ) {
  case INIT_BUFFER_BACKBUFFER:
    rfbRenderIdx = rfbBackIdx;
    break;
  case INIT_BUFFER_FRONTBUFFER:
    rfbRenderIdx = rfbFrontIdx;
    break;
  }
  wcb( (FxU32*)&(sstPtr->colBufferSetup), bI[rfbRenderIdx].rfb );

#undef FN_NAME
} /* init96RenderBuffer */

/*-------------------------------------------------------------------
  Function: init96Origin
  Date: 2/26
  Implementor(s): jdt
  Library: init96
  Description:
  Set the yOrigin
  Arguments:
  origin
  Return:
  none
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96Origin, void, (InitOriginLocation_t origin,Init96WriteMethod *wcb)) 
{
#define FN_NAME "init96Origin"
  int i;
  GDBG_INFO((80,"%s\n", FN_NAME));

  /* Once the bases and tops are set, this is pretty straight forward */
  switch ( origin ) {
  case INIT_ORIGIN_UPPER_LEFT:
    for (i=0; i<maxBuffer; i++) {
      bI[i].rfb = bI[i].dfbBase | VG96STRIDE(bI[i].dfbStride);
    }
    ab0 = ab0Base | VG96STRIDE(ab0Stride);
    break;
  case INIT_ORIGIN_LOWER_LEFT:
    for (i=0; i<maxBuffer; i++) {
      bI[i].rfb = bI[i].dfbTop | VG96STRIDE(bI[i].dfbStride);
    }
    ab0 = ab0Top | VG96STRIDE(ab0Stride);
    break;
  }

  wcb( (FxU32*)&(sstPtr->colBufferSetup), bI[rfbRenderIdx].rfb);
  wcb( (FxU32*)&(sstPtr->auxBufferSetup), ab0 );
#undef FN_NAME
} /* init96Origin */

/*-------------------------------------------------------------------
  Function: init96ResyncBuffers
  Date: 07-Mar-97
  Implementor(s): murali
  Library: Init Code for VG96
  Description:
  Performs VG96 buffer resync buffers
  Arguments:
  Return:
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96Control, FxBool , ( FxU32 code, InitRegisterDesc *regDesc))
{
#define FN_NAME "init96Control"
  int           xRes, yRes;
  InitBufDesc_t bufDesc[5];

  GDBG_INFO((80, "(%s)\n", FN_NAME));
  
  if ( !sstHW ) return FXFALSE;

#ifdef __linux__
  if (!linControl(code, bufDesc, &xRes, &yRes))
#else
  if (!dxControl(code, bufDesc, &xRes, &yRes))
#endif
  { GDBG_INFO((80, "dxControl failed!\n"));
    return FXFALSE;
  }

  if (!init96LoadBufRegs(NUM_BUFS, bufDesc,  xRes, yRes, regDesc)) {
    GDBG_INFO((80, "init96LoadBufRegs failed\n"));
  }

  GDBG_INFO((80, "%s:  Returning TRUE\n", FN_NAME));
  return (yRes << 16) | (xRes);
#undef FN_NAME
} /* init96Control */

#if SST96_ALT_FIFO_WRAP

#define MIN(__x, __y) (((__x) < (__y)) ? (__x) : (__y))
#define MAX(__x, __y) (((__x) < (__y)) ? (__y) : (__x))

/* Macro to get the hw's read ptr located somewhere in the frame
 * buffer address space.
 */
#define SST96_HW_FIFO(__hwP, __regP) ((FxU32)(__hwP) + \
                                      (*((volatile FxU32*)(((FxU32)(__regP)) + (0x95 << 12))) << 3))

INITVG96ENTRY(init96WrapFIFO, 
              FxBool, 
              (InitRegisterDesc* rd, InitFIFOData* fifoP))
{
#define FN_NAME "init96WrapFIFO"
  const FxI32 blockSize = fifoP->hwDep.vg96FIFOData.blockSize;

#if GDBG_INFO_ON
  /* no ASSERT macro. Just die if debugging is on. */
  if (blockSize == 0xDEADBEEF) *(FxU32*)NULL = 0x00;
#endif

  {
    const FxI32 writes = (MIN(fifoP->hwDep.vg96FIFOData.roomToReadPtr, 
                              fifoP->hwDep.vg96FIFOData.roomToEnd) -
                          fifoP->hwDep.vg96FIFOData.fifoSize);
    
    fifoP->hwDep.vg96FIFOData.roomToReadPtr -= writes;
    fifoP->hwDep.vg96FIFOData.roomToEnd -= writes;
  }

again:
  /* do we need to stall? */
  {
    FxU32 lastHwRead = fifoP->hwDep.vg96FIFOData.fifoRead;
    FxI32 roomToReadPtr = fifoP->hwDep.vg96FIFOData.roomToReadPtr;

    while (roomToReadPtr < blockSize) {
      const FxU32 curReadPtr = SST96_HW_FIFO(sstHW, sstPtr);
      const FxU32 curReadDist = curReadPtr - lastHwRead;
      
      roomToReadPtr += curReadDist;
      if (lastHwRead > curReadPtr) roomToReadPtr += fifoP->hwDep.vg96FIFOData.fifoMax;

      lastHwRead = curReadPtr;
    }

    GDBG_INFO((80, FN_NAME"_Wait : (0x%X : 0x%X) : (0x%X : 0x%X)\n", 
               (FxU32)fifoP->hwDep.vg96FIFOData.fifoPtr, blockSize,
               fifoP->hwDep.vg96FIFOData.roomToReadPtr,
               fifoP->hwDep.vg96FIFOData.roomToEnd));

    fifoP->hwDep.vg96FIFOData.fifoRead = lastHwRead;
    fifoP->hwDep.vg96FIFOData.roomToReadPtr = roomToReadPtr;
  }

  /* wrap to front */
  if (fifoP->hwDep.vg96FIFOData.roomToEnd <= blockSize) {
      /* Pad out all of the fifo, save the last entry, w/ NUL ops. The
       * last entry is written once we know that we are not going to
       * overflow the entry count. Oh, for the cvg JMP packet type.
       */
#define kFifoPadDelay (sizeof(FxU32) << 1)
#if GDBG_INFO_ON
#define JU_JU_STALL numJuJuStalls++
#else
#define JU_JU_STALL /* Do Nothing */
#endif
      {
         volatile FxU32* fifoPtr = fifoP->hwDep.vg96FIFOData.fifoPtr;
         const FxU32 wrapAddr = ((FxU32)fifoP->hwDep.vg96FIFOData.fifoVirt +
                                 fifoP->hwDep.vg96FIFOData.fifoMax);

         /* Do the padding up to the last entry or so. */
         while((FxU32)fifoPtr < wrapAddr - kFifoPadDelay) *fifoPtr++ = 0x00UL;

         /* Bad ju-ju awaits the person who wraps the fifo when the hw
          * ptr is at the start of the fifo. (I've seen it happen, and
          * it is not pretty.)
          */
         {
            volatile FxU32* serialStatus =
              fifoP->hwDep.vg96FIFOData.regDesc->hwDep.VG96RegDesc.serialStatus;

            if ((*serialStatus & 0x80) != 0x00) {
               GDBG_INFO((85, FN_NAME": fifo close to full. Waiting(0x%X)...\n", numJuJuStalls));
               while((*serialStatus & 0x80) != 0x00) JU_JU_STALL;
               GDBG_INFO((85, FN_NAME": fifo drained (0x%X)\n", numJuJuStalls));
            }
         }

         /* Finish the padding of the rest of the fifo to the end */
         while((FxU32)fifoPtr < wrapAddr) *fifoPtr++ = 0x00UL;
      }
#undef JU_JU_STALL
#undef kFifoPadDelay

      GDBG_INFO((80, FN_NAME"_Wrap : (0x%X : 0x%X) : (0x%X : 0x%X)\n", 
                 (FxU32)fifoP->hwDep.vg96FIFOData.fifoPtr, blockSize,
                 fifoP->hwDep.vg96FIFOData.roomToReadPtr,
                 fifoP->hwDep.vg96FIFOData.roomToEnd));
      
      fifoP->hwDep.vg96FIFOData.roomToReadPtr -= fifoP->hwDep.vg96FIFOData.roomToEnd;
      fifoP->hwDep.vg96FIFOData.roomToEnd = fifoP->hwDep.vg96FIFOData.fifoMax;
      
      /* Reset fifo ptr to start */ 
      fifoP->hwDep.vg96FIFOData.fifoPtr = fifoP->hwDep.vg96FIFOData.fifoVirt;
      goto again;
  }
  
  /* compute room left */
  fifoP->hwDep.vg96FIFOData.roomToEnd -= blockSize;
  fifoP->hwDep.vg96FIFOData.roomToReadPtr -= blockSize;
  fifoP->hwDep.vg96FIFOData.fifoSize = MIN(fifoP->hwDep.vg96FIFOData.roomToReadPtr,
                                           fifoP->hwDep.vg96FIFOData.roomToEnd) + blockSize;
  
  GDBG_INFO((80, FN_NAME": (0x%X : 0x%X) : (0x%X : 0x%X) : 0x%X\n",
             (FxU32)fifoP->hwDep.vg96FIFOData.fifoPtr, blockSize,
             fifoP->hwDep.vg96FIFOData.roomToReadPtr,
             fifoP->hwDep.vg96FIFOData.roomToEnd,
             fifoP->hwDep.vg96FIFOData.fifoSize));

#if GDBG_INFO_ON
  fifoP->hwDep.vg96FIFOData.blockSize = 0xDEADBEEF;
#endif

  return FXTRUE;
#undef FN_NAME
} 
#else /* !SST96_ALT_FIFO_WRAP */

/*-------------------------------------------------------------------
  Function: init96FIFOWait
  Date: 12-Jun-97
  Implementor(s): dow
  Library: Init Code for VG96
  Description:
  Waits for 0x80 in status
  Arguments:
  rd:     Description of registers.
  Return:
  Nothing
  -------------------------------------------------------------------*/
static void
init96FIFOWait(InitRegisterDesc *rd)
{
#define FN_NAME "init96FIFOWait"
  volatile FxU32 *pstatus;
  

  /* Check for more room. */
  pstatus = (volatile FxU32 *) rd->hwDep.VG96RegDesc.serialStatus;
  
  if (*pstatus & 0x80) {        /* 0x80 is the FIFO threshold */
    int  timeout;

  again:
    for (timeout = 0; timeout < 10000000; timeout++) {
      if ((*pstatus & 0x80) == 0) break;
    }
    if (*pstatus & 0x80) {
      GDBG_INFO((80, "Lost patience in %s.  Status is 0x%x\n",
                 FN_NAME, *pstatus)); 
      goto again;
    }
  }
#undef FN_NAME  
} /* init96FIFOWait */

/*-------------------------------------------------------------------
  Function: init96WrapFIFO
  Date: 13-Feb-97
  Implementor(s): dow
  Library: Init Code for VG96
  Description:
  Wraps the VG96 FIFO
  Arguments:
  rd:     Description of registers.
  Return:
  FXTRUE if successful.
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96WrapFIFO, FxBool , (InitRegisterDesc *rd, InitFIFOData *fifoData) )
{
#define FN_NAME "init96WrapFIFO"
  FxBool rVal;
  
  GDBG_INFO((80,"%s\n", FN_NAME));
  
  while (fifoData->hwDep.vg96FIFOData.fifoSize > 0) {
    *fifoData->hwDep.vg96FIFOData.fifoPtr++ = 0L;
    fifoData->hwDep.vg96FIFOData.fifoSize -= 4;
  }
  
  fifoData->hwDep.vg96FIFOData.fifoCur += 4096;
  
  /* Wrap back to beginning? */
  if (fifoData->hwDep.vg96FIFOData.fifoCur >= fifoData->hwDep.vg96FIFOData.fifoMax ) {
    static int fifowraps = 0;
    
    fifoData->hwDep.vg96FIFOData.fifoCur = 0;
    if ((fifowraps = (fifowraps + 1) & 0xf) == 0) {
      GDBG_INFO((80, "%s:  Wrapping %s FIFO\n", 
                 FN_NAME, 
                 (fifoData->hwDep.vg96FIFOData.fifoLfb 
                  ? "LFB" 
                  : "Video Aperture")));
    }

    if (fifoData->hwDep.vg96FIFOData.fifoLfb) 
      fifoData->hwDep.vg96FIFOData.fifoPtr = fifoData->hwDep.vg96FIFOData.fifoVirt;
  }
  
  /*  */
  {
    FxU32
      fbiJrInit5;
    
    GDBG_INFO((125, "%s:  Write = 0x%x\n", FN_NAME,
               (FxU32) fifoData->hwDep.vg96FIFOData.fifoPtr -
               (FxU32) fifoData->hwDep.vg96FIFOData.fifoVirt));
    
    fbiJrInit5 = *((volatile FxU32 *) (((FxU32) sstPtr) + (0x95 << 12)));

    GDBG_INFO((125, "%s:  Read  = 0x%x\n", FN_NAME,
               (fbiJrInit5 << 3) -
               (
                (FxU32) fifoData->hwDep.vg96FIFOData.fifoVirt -
                (FxU32) rd->hwDep.VG96RegDesc.baseAddress
                )
               ));
  }

  GDBG_INFO((80, "%s:  Waiting for more room.\n", FN_NAME));
  init96FIFOWait(rd);

  /*
  **   if we get here, then there's room in the FIFO
  */

  GDBG_INFO((80, "%s:  Got more room.\n", FN_NAME));
  
  /* At this point, we have atleast 1 more page of fifo. */
  fifoData->hwDep.vg96FIFOData.fifoSize = 4096;
  
  rVal = FXTRUE;
  
  GDBG_INFO((80, "%s:  returning %s\n", FN_NAME, rVal ? "FXTRUE" : "FXFALSE"));
  
  return rVal;
  
#undef FN_NAME
} /* init96WrapFIFO */

#endif /* !SST96_ALT_FIFO_WRAP */

/* 
 * This function is used only locally, so none of Chris's decorations 
 * Load colbufSetup and auxBufSetup registers, given a InitBufDesc_t
 */
static FxBool 
init96LoadBufRegs(int nBuffers, InitBufDesc_t *pBufDesc, int xRes,
                  int yRes, InitRegisterDesc *rd) 
{
#define FN_NAME "init96LoadBufRegs"
  InitBufDesc_t   *pFront  = NULL;
  InitBufDesc_t   *pBack   = NULL;
  InitBufDesc_t   *pTriple = NULL;
  InitBufDesc_t   *pAux    = NULL;
  InitBufDesc_t   *pFifo   = NULL;
  const char      *envp;
  int i;

  GDBG_INFO((80, "(%s) w = %d, h = %d, n = %d\n",
             FN_NAME, xRes, yRes, nBuffers));

  if ( !sstHW ) return FXFALSE;

  /* Take pBufDesc apart */
  while (nBuffers-- > 0) {
    switch (pBufDesc->bufType) {
    case INIT_BUFFER_FRONTBUFFER:
      GDBG_INFO((80, "%s: %s: 0x%x\n", 
                 FN_NAME, bufTypeNames[pBufDesc->bufType], pBufDesc));
      pFront = pBufDesc;
      break;
    case INIT_BUFFER_BACKBUFFER:
      GDBG_INFO((80, "%s: %s: 0x%x\n", 
                 FN_NAME, bufTypeNames[pBufDesc->bufType], pBufDesc));
      pBack  = pBufDesc;
      break;
    case INIT_BUFFER_AUXBUFFER:
      GDBG_INFO((80, "%s: %s: 0x%x\n", 
                 FN_NAME, bufTypeNames[pBufDesc->bufType], pBufDesc));
      pAux   = pBufDesc; 
      break;
    case INIT_BUFFER_FIFOBUFFER:    
      GDBG_INFO((80, "%s: %s: 0x%x\n", 
                 FN_NAME, bufTypeNames[pBufDesc->bufType], pBufDesc));
      pFifo  = pBufDesc;
      break;
    case INIT_BUFFER_TRIPLEBUFFER:
      GDBG_INFO((80, "%s: %s: 0x%x\n", 
                 FN_NAME, bufTypeNames[pBufDesc->bufType], pBufDesc));
      pTriple = pBufDesc;
      break;
    default:
      break;
    }
    pBufDesc++;
  }
  if (pFront  == NULL) return FXFALSE;
  if (pBack   == NULL) pBack = pFront;
  if (pTriple == NULL) pTriple = pFront;
  if (pAux    == NULL) pAux  = pFront;

  GDBG_INFO((80,"pFront = %.08x, pBack = %.08x, pTriple = %.08x, pAux = %.08x, pFifo = %.08x\n",
             pFront, pBack, pTriple, pAux, pFifo));

  envp = myGetenv("SST96_FORCEALIGN");
  if (envp) {
    FxU32 F, B, T, A;

    if (sscanf(envp, "%x,%x,%x,%x", &F, &B, &T, &A) == 4)
    {
      GDBG_INFO((80, "!!!!!GROSS HACK... forcing values!!!!!\n"));
      pFront->bufOffset = F;
      pBack->bufOffset = B;
      pTriple->bufOffset = T;
      pAux->bufOffset = A;
    }
  }

  if (pFront) {
    GDBG_INFO((80,"F = %.08x, s= %6d\n", pFront->bufOffset, pFront->bufStride));
  }
  if (pBack) {
    GDBG_INFO((80,"B = %.08x, s= %6d\n", pBack ->bufOffset, pBack ->bufStride));
  }
  if (pTriple) {
    GDBG_INFO((80,"T = %.08x, s= %6d\n", pTriple ->bufOffset, pTriple ->bufStride));
  }
  if (pAux) {
    GDBG_INFO((80,"A = %.08x, s= %6d\n", pAux  ->bufOffset, pAux  ->bufStride));
  }
  if (pFifo) {
    GDBG_INFO((80,"C = %.08x, s= %6d\n", pFifo ->bufOffset, pFifo ->bufStride));
  }

  /* Fill the arrays here */
  bI[0].dfbBase   = pFront->bufOffset; 
  bI[0].dfbStride = pFront->bufStride;
  bI[0].dfbTop    = pFront->bufOffset + (yRes-1) * pFront->bufStride;

  bI[1].dfbBase   = pBack->bufOffset;
  bI[1].dfbStride = pBack->bufStride;
  bI[1].dfbTop    = pBack->bufOffset + (yRes-1) * pBack->bufStride;

  if (pTriple) {
    bI[2].dfbBase   = pTriple->bufOffset;
    bI[2].dfbStride = pTriple->bufStride;
    bI[2].dfbTop    = pTriple->bufOffset + (yRes-1) * pTriple->bufStride;
  }

  ab0Base   = pAux->bufOffset;
  ab0Stride = pAux ->bufStride;
  ab0Top    = pAux->bufOffset + (yRes-1) * pAux->bufStride;

  for (i=0; i<maxBuffer; i++) {
    bI[i].dfb = bI[i].dfbBase | VG96STRIDE(bI[i].dfbStride);
  }

  if (cachedYOrigin == INIT_ORIGIN_UPPER_LEFT) {
    for (i=0; i<maxBuffer; i++) {
      bI[i].rfb = bI[i].dfbBase | VG96STRIDE(bI[i].dfbStride);
    }
    ab0 = ab0Base | VG96STRIDE(ab0Stride);
  } else { /* cachedYOrigin == INIT_ORIGIN_LOWER_LEFT */
    for (i=0; i<maxBuffer; i++) {
      GDBG_INFO((80, "%s:  Buffer %d stride = 0x%x\n",
                 FN_NAME, i, bI[i].dfbStride));
      bI[i].rfb = bI[i].dfbTop | VG96STRIDE(bI[i].dfbStride);
      GDBG_INFO((80, "%s: setting render buffer %d = 0x%x\n",
                 FN_NAME, i, bI[i].rfb));
    }
    ab0 = ab0Top | VG96STRIDE(ab0Stride);
  }

  GDBG_INFO((80,"%s: dfb0 = %.08x, dfb1 = %.08x, dfb2 = %.08x, ab0 = %.08x\n",
             FN_NAME, bI[0].dfb, bI[1].dfb, bI[2].dfb, ab0));

  GDBG_INFO((80,"%s: rfb0 = %.08x, rfb1 = %.08x, rfb2 = %.08x, ab0 = %.08x\n",
             FN_NAME, bI[0].rfb, bI[1].rfb, bI[2].rfb, ab0));

  GDBG_INFO((80,"%s:  Setting 0x60 to %.08x, 0x61 to %.08x\n",
             FN_NAME, bI[dfbRenderIdx].dfb, ab0));

  SET(sstHW[0x60 + 0x100000], bI[rfbRenderIdx].rfb);
  SET(sstHW[0x61 + 0x100000], ab0);

  if (pFifo == NULL) {
    GDBG_INFO((80, "init96LoadBufRegs: Bad pFifo\n"));
    return FXFALSE;
  }

  if (pFifo) {
    fifoStart = pFifo->bufOffset;
    fifoEnd   = pFifo->bufOffset + pFifo->bufStride;
  }
  
  GDBG_INFO((80, "%s:  Returning TRUE\n", FN_NAME));
  return FXTRUE;
#undef FN_NAME
} /* init96LoadBufRegs */

/*-------------------------------------------------------------------
  Function: init96GetFBVram
  Date: 24-Apr-97
  Implementor(s): pgj
  Library: Test for 2Mb or 4Mb board
  Description: This routine returns the number of bytes of VRAM
  on the board.  Currently only 2 or 4 MB are returned on
  success. Zero is returned of the board is not mapped.
  XXX This should be refined in the future. XXX
  
  Arguments: none
  
  Return: n, where n == available VRAM in bytes on the board
  -------------------------------------------------------------------*/
INITVG96ENTRY(init96GetFBVram, FxU32 , (void) )
{
#define FN_NAME "init96GetFBVram"
  VG96Info info;

  if (curHALData->initGetInfo) {
    (*curHALData->initGetInfo)(&info);
    GDBG_INFO((80, "%s:  returning %d bytes\n", FN_NAME, info.vg96Ram));
    return info.vg96Ram;
  } else {
    GDBG_INFO((80, "%s:  curHALData not valid!!!\n", FN_NAME));
    return 0;
  }

#undef FN_NAME
} /* init96GetFBVram */
