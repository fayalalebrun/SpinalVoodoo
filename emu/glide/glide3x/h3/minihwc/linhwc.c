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


/*

  We need to provide the following functions:

const char *
hwcGetErrorString(void);

hwcInfo *
hwcInit(FxU32 vID, FxU32 dID);

FxBool
hwcMapBoard(hwcBoardInfo *bInfo, FxU32 bAddrMask);

FxBool
hwcInitRegisters(hwcBoardInfo *bInfo);

FxBool
hwcAllocBuffers(hwcBoardInfo *bInfo, FxU32 nColBuffers, FxU32 nAuxBuffers); 

FxBool
hwcInitFifo(hwcBoardInfo *bInfo);

FxU32 
hwcInitAGPFifo(hwcBoardInfo *bInfo); 

FxBool
hwcInitVideo(hwcBoardInfo *bInfo, FxBool tiled, FxVideoTimingInfo
             *vidTiming, FxBool overlay);

FxBool
hwcRestoreVideo(hwcBoardInfo *bInfo);

FxBool
hwcCheckMemSize(hwcBoardInfo *bInfo, FxU32 xres, FxU32 yres, FxU32 nColBuffers,
                FxU32 nAuxBuffers, FxBool tiled);
*/

#include <minihwc.h>
#include <hwcio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
/*#include <X11/extensions/xf86dga.h>*/
#include <X11/extensions/xf86vmode.h>
#include "lindri.h"

void
_grInvalidateAll(void);
void
_grExportFifo(int *fifoPtr, int *fifoRead);
void
_grImportFifo(int fifoPtr, int fifoRead);
static FxU32
hwcBufferLfbAddr(const hwcBoardInfo *bInfo, FxU32 physAddress);

#if defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))
# define __attribute_used __attribute__((__used__))
#elif defined(__GNUC__) && (__GNUC__ >= 2)
# define __attribute_used __attribute__((__unused__))
#else
# define __attribute_used
#endif

static FxU32 __attribute_used fenceVar;
#if defined(__GNUC__) && defined(__ia64__)
# define P6FENCE asm volatile ("mf.a" ::: "memory");
#elif defined(__GNUC__) && defined(__alpha__)
# define P6FENCE asm volatile("mb" ::: "memory");
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
# define P6FENCE asm("xchg %%eax, %0" : : "m" (fenceVar) : "eax");
#else
# error "No P6FENCE asm for this architecture"
#endif

#define MAXFIFOSIZE     0x40000
#define FIFOPAD         0x0000
#define AUXPAD          0x1000
#define HWC_TILED_BUFFER_BYTES    0x1000UL  /* 128 Bytes x 32 lines */
#define HWC_TILED_BUFFER_Y_ALIGN  0x20000UL
#define HWC_TILED_BUFFER_X_ADJUST 0x80UL

static hwcInfo hInfo;
static char errorString[1024];

static FxU32 calcBufferStride(FxU32 xres, FxBool tiled);
static FxU32 calcBufferSize(FxU32 xres, FxU32 yres, FxBool tiled);
static FxU32 calcBufferSizeInTiles(FxU32 xres, FxU32 yres);
static FxU32 calcBufferHeightInTiles(FxU32 yres);

typedef struct envitem_t {
  char *env;
  char *val;
  struct envitem_t *next;
} envitem;

static envitem *first=0;
static int envinit=0;

DRIDef driInfo={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void grDRIOpen(char *pFB, char *pRegs, int deviceID, int width, int height, 
	       int mem, int cpp, int stride, int fifoOffset, int fifoSize, 
	       int fbOffset, int backOffset, int depthOffset, 
	       int textureOffset, int textureSize, 
	       volatile int *fifoPtr, volatile int *fifoRead) {
  driInfo.pFB=pFB;
  driInfo.pRegs=pRegs;
  driInfo.deviceID=deviceID;
  driInfo.screenWidth=width;
  driInfo.screenHeight=height;
  driInfo.memory=mem;
  driInfo.cpp=cpp;
  driInfo.stride=stride;
  driInfo.fifoOffset=fifoOffset;
  driInfo.fifoSize = fifoSize;
  driInfo.fbOffset=fbOffset;
  driInfo.backOffset=backOffset;
  driInfo.depthOffset=depthOffset;
  driInfo.textureOffset=textureOffset;
  driInfo.textureSize=textureSize;
  driInfo.fifoPtr=fifoPtr;
  driInfo.fifoRead=fifoRead;
}

void grDRIPosition(int x, int y, int w, int h, 
		   int numClip, XF86DRIClipRectPtr pClip) {
  driInfo.x=x;
  driInfo.y=y;
  driInfo.w=w;
  driInfo.h=h;
  driInfo.numClip=numClip;
  driInfo.pClip=pClip;
}

static void loadEnvFile(void) {
  FILE *file;
  char data[128];
  char *env, *val;
  envitem *item;
  int is_first=1;

  if (envinit) return;
  envinit=1;
  file=fopen("/etc/conf.3dfx/voodoo3", "r");
  if (!file) return;
  while (1) {
    if (!fgets(data, 128, file)) return;
    if (*data=='#') continue;
    if (*data=='\n') continue;
    val=strchr(data, '=');
    if (!val) {
      if (is_first) {
	fprintf(stderr, "In config file /etc/conf.3dfx/voodoo3:\n");
	is_first=0;
      }
      fprintf(stderr, "Malformed line: %s\n", data);
      continue;
    }
    *val=0;
    env=data;
    val++;
    item=malloc(sizeof(envitem));
    item->env=malloc(strlen(env)+1);
    strcpy(item->env, env);
    item->val=malloc(strlen(val)+1);
    strcpy(item->val, val);
    item->next=first;
    first=item;
  }
  fclose(file);
}

#if 0 /* not used */
static void deleteEnvData(void) {
  envitem *ptr, *next;

  ptr=first;
  while (ptr) {
    next=ptr->next;
    if (ptr->env) free(ptr->env);
    if (ptr->val) free(ptr->val);
    free(ptr);
    ptr=next;
  }
  first=0;
  envinit=0;
}
#endif

const char *
hwcGetErrorString()
{
#define FN_NAME "hwcGetErrorString"
  return errorString;
#undef FN_NAME
} /* hwcGetErrorString */

hwcInfo *
hwcInit(FxU32 vID, FxU32 dID) {
  hInfo.nBoards = 0;

  errorString[0] = '\0';

  if (!driInfo.pFB) return 0;
  if (dID!=(FxU32)driInfo.deviceID) return 0;
  hInfo.boardInfo[0].pciInfo.initialized = FXFALSE;
  hInfo.nBoards++;
  hInfo.boardInfo[0].boardNum = 0;

  hInfo.boardInfo[0].pciInfo.initialized = FXTRUE;;
  hInfo.boardInfo[0].pciInfo.vendorID = vID;
  hInfo.boardInfo[0].pciInfo.deviceID = dID;
  hInfo.boardInfo[0].h3Mem = driInfo.memory>>20;
  if (hInfo.nBoards) {
    return &hInfo;
  } else {
    sprintf(errorString, "Can't find or access Banshee/V3 board\n");
    return 0;
  }
}

FxBool
hwcMapBoard(hwcBoardInfo *bInfo, FxU32 bAddrMask) {
  if (bInfo->pciInfo.initialized == FXFALSE) {
    sprintf(errorString, "hwcMapBoard: Called before hwcInit\n");
    return FXFALSE;
  }

  bInfo->linearInfo.initialized = FXTRUE;
  bInfo->linearInfo.linearAddress[0]=(unsigned long)driInfo.pRegs;
  bInfo->linearInfo.linearAddress[1]=(unsigned long)driInfo.pFB;
  return FXTRUE;
}

FxBool
hwcUnmapBoard(hwcBoardInfo *bInfo) {
  return FXTRUE;
}

FxBool
hwcInitRegisters(hwcBoardInfo *bInfo) {
  int dramInit1;

  if (bInfo->linearInfo.initialized == FXFALSE) {
    printf(errorString, "hwcInitRegisters Called before hwcMapBoard\n");
    return FXFALSE;
  }
      
  bInfo->regInfo.initialized = FXTRUE;

  bInfo->regInfo.ioMemBase =
    bInfo->linearInfo.linearAddress[0] + SST_IO_OFFSET;
  bInfo->regInfo.cmdAGPBase =
    bInfo->linearInfo.linearAddress[0] + SST_CMDAGP_OFFSET;
  bInfo->regInfo.waxBase =
    bInfo->linearInfo.linearAddress[0] + SST_2D_OFFSET;
  bInfo->regInfo.sstBase =
    bInfo->linearInfo.linearAddress[0] + SST_3D_OFFSET;
  bInfo->regInfo.lfbBase =
    bInfo->linearInfo.linearAddress[0] + SST_LFB_OFFSET;
  bInfo->regInfo.rawLfbBase =
    bInfo->linearInfo.linearAddress[1];
  bInfo->regInfo.ioPortBase = (FxU16) bInfo->pciInfo.pciBaseAddr[2] & ~0x1;

  /* Figure out if it's SDRAM */
  HWC_IO_LOAD(bInfo->regInfo, dramInit1, dramInit1);
  dramInit1 &= SST_MCTL_TYPE_SDRAM;
  if ( dramInit1 & SST_MCTL_TYPE_SDRAM )
    bInfo->sdRAM = FXTRUE;
  else
    bInfo->sdRAM = FXFALSE;

  {
    FxU32 
      pciInit0,
      pciCommandReg =
        BIT(0) |              /* enable i/o decode */
        BIT(1);               /* enable memory decode */
  
    /* Enable PCI memory and I/O decode */
    pciSetConfigData(PCI_COMMAND, bInfo->deviceNum, &pciCommandReg);
    
    HWC_IO_LOAD(bInfo->regInfo, pciInit0, pciInit0);
    pciInit0 |= SST_PCI_READ_WS | SST_PCI_WRITE_WS;
    HWC_IO_STORE(bInfo->regInfo, pciInit0, pciInit0);  
  }

  return FXTRUE;
}

FxBool
hwcAllocBuffers(hwcBoardInfo *bInfo, FxU32 nColBuffers, FxU32 nAuxBuffers) 
{
#define FN_NAME "hwcAllocBuffers"
  FxU32 bufStride, bufSize;

  if (bInfo->vidInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before video initialization\n", FN_NAME);
    return FXFALSE;
  }

  GDBG_INFO(80, "%s(0x%x, 0x%x, 0x%x)\n", FN_NAME, bInfo, nColBuffers, nAuxBuffers);

  /* I've decided on > 2 instead of == 3 because we may support more
    than 3 buffers in the future, and want 4 to set the
    triple-buffering bit in dramInit1, also */
  bInfo->vidInfo.tripleBuffering = (nColBuffers > 2);

  bInfo->vidInfo.stride = bufStride = 
    calcBufferStride(driInfo.screenWidth, bInfo->vidInfo.tiled);   

  /* We want to place the FIFO after the tram but before the color
     buffers with some pad */
  bufSize = calcBufferSize(driInfo.screenWidth, driInfo.screenHeight, 
			   bInfo->vidInfo.tiled); 

  bInfo->buffInfo.bufStride = bufStride;
  bInfo->buffInfo.bufSize = bufSize;

  if (bInfo->vidInfo.tiled) {
    bInfo->buffInfo.bufStrideInTiles = (bufStride >> 7);
    bInfo->buffInfo.bufSizeInTiles =
      calcBufferSizeInTiles(driInfo.screenWidth, driInfo.screenHeight);
    bInfo->buffInfo.bufHeightInTiles =
      calcBufferHeightInTiles(driInfo.screenHeight);
  }

  bInfo->buffInfo.initialized = FXTRUE;
  bInfo->buffInfo.nColBuffers = nColBuffers;
  bInfo->buffInfo.nAuxBuffers = nAuxBuffers;

  bInfo->fbOffset =  driInfo.fbOffset;

  bInfo->fifoInfo.fifoStart = driInfo.fifoOffset;
  bInfo->fifoInfo.fifoLength = driInfo.fifoSize;

  bInfo->tramOffset = driInfo.textureOffset;
  bInfo->tramSize = driInfo.textureSize;

  bInfo->buffInfo.colBuffStart[0] = driInfo.fbOffset;
  bInfo->buffInfo.colBuffEnd[0] = driInfo.fbOffset +
    driInfo.screenHeight*driInfo.stride;

  bInfo->buffInfo.colBuffStart[1] = driInfo.backOffset;
  bInfo->buffInfo.colBuffEnd[1] = driInfo.backOffset+bufSize;

  bInfo->buffInfo.auxBuffStart = driInfo.depthOffset;
  bInfo->buffInfo.auxBuffEnd = driInfo.depthOffset+bufSize;

  bInfo->buffInfo.lfbBuffAddr[0] = bInfo->buffInfo.colBuffStart[0];
  bInfo->buffInfo.lfbBuffAddr[1] = bInfo->buffInfo.colBuffStart[1];
  if (bInfo->vidInfo.tiled) {
    bInfo->buffInfo.lfbBuffAddr[2] = 
		hwcBufferLfbAddr(bInfo, bInfo->buffInfo.auxBuffStart);
  } else {
    bInfo->buffInfo.lfbBuffAddr[2] = bInfo->buffInfo.auxBuffStart;
  }

  GDBG_INFO(80, "%s:  Board Info:\n", FN_NAME);
  GDBG_INFO(80, "\thdc:             0x%x\n", bInfo->hdc);
  GDBG_INFO(80, "\textContextID:    0x%x\n", bInfo->extContextID);
  GDBG_INFO(80, "\tdevRev:          0x%x\n", bInfo->devRev);
  GDBG_INFO(80, "\tfbOffset:        0x%x\n", bInfo->fbOffset);
  GDBG_INFO(80, "\th3Rev:           0x%x\n", bInfo->h3Rev);
  GDBG_INFO(80, "\th3Mem:           0x%x\n", bInfo->h3Mem);
  GDBG_INFO(80, "\tboardNum:        0x%x\n", bInfo->boardNum);
  GDBG_INFO(80, "\tdeviceNum:       0x%x\n", bInfo->deviceNum);

  GDBG_INFO(80, "%s:  Buffer Info:\n", FN_NAME);
  GDBG_INFO(80, "\tbufSize:         0x%x\n", bInfo->buffInfo.bufSize);
  GDBG_INFO(80, "\tbufSizeInTiles:  0x%x\n", bInfo->buffInfo.bufSizeInTiles);
  GDBG_INFO(80, "\tbufStride:       0x%x\n", bInfo->buffInfo.bufStride);
  GDBG_INFO(80, "\tbufStrideInTiles:0x%x\n", bInfo->buffInfo.bufStrideInTiles);
  GDBG_INFO(80, "\tbufHeightInTiles:0x%x\n", bInfo->buffInfo.bufHeightInTiles);
  GDBG_INFO(80, "\tnColBuffers:     0x%x\n", bInfo->buffInfo.nColBuffers);
  GDBG_INFO(80, "\tcolBuffStart[0]:    0x%x\n", bInfo->buffInfo.colBuffStart[0]);
  GDBG_INFO(80, "\tcolBuffEnd[0]:      0x%x\n", bInfo->buffInfo.colBuffEnd[0]);
  GDBG_INFO(80, "\tcolBuffStart[1]:    0x%x\n", bInfo->buffInfo.colBuffStart[1]);
  GDBG_INFO(80, "\tcolBuffEnd[1]:      0x%x\n", bInfo->buffInfo.colBuffEnd[1]);
  GDBG_INFO(80, "\tnAuxBuffers:     0x%x\n", bInfo->buffInfo.nAuxBuffers);
  GDBG_INFO(80, "\tauxBuffStart:    0x%x\n", bInfo->buffInfo.auxBuffStart);
  GDBG_INFO(80, "\tauxBuffEnd:      0x%x\n", bInfo->buffInfo.auxBuffEnd);
  GDBG_INFO(80, "\ttramOffset:      0x%x\n", bInfo->tramOffset);
  GDBG_INFO(80, "\ttramSize:        0x%x\n", bInfo->tramSize);
  GDBG_INFO(80, "\tlfbBuffAddr[0]   0x%x\n", bInfo->buffInfo.lfbBuffAddr[0]);
  GDBG_INFO(80, "\tlfbBuffAddr[1]   0x%x\n", bInfo->buffInfo.lfbBuffAddr[1]);
  GDBG_INFO(80, "\tlfbBuffAddr[2]   0x%x\n", bInfo->buffInfo.lfbBuffAddr[2]);

  GDBG_INFO(80, "%s:  FIFO Info:\n", FN_NAME);
  GDBG_INFO(80, "\tfifoStart:       0x%x\n", bInfo->fifoInfo.fifoStart);
  GDBG_INFO(80, "\tfifoLength:      0x%x\n", bInfo->fifoInfo.fifoLength);
  return FXTRUE;

#undef FN_NAME
} /* hwcAllocBuffers */

FxBool
hwcInitFifo(hwcBoardInfo *bInfo, FxBool enableHoleCounting)
{
#define FN_NAME "hwcInitFifo"
  if (bInfo->regInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcMapBoard\n", FN_NAME);
    return FXFALSE;
  }

  if (bInfo->buffInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcInitBuffers\n", FN_NAME);
    return FXFALSE;
  }

#if 0
  driInfo.holeCounting=enableHoleCounting;
  hwcInitFifoRegs(enableHoleCounting);
#endif

  GDBG_INFO(80 ,"%s:  CMD FIFO placed at physical addr 0x%x size 0x%0x\n", 
	    FN_NAME, driInfo.fifoOffset, driInfo.fifoSize);

  return FXTRUE;

#undef FN_NAME
} /* hwcInitFifo */

FxBool
hwcInitVideo(hwcBoardInfo *bInfo, FxBool tiled, FxVideoTimingInfo
             *vidTiming, FxBool overlay) {
  FxU32 stride, dramInit1, miscInit0;

  stride= (tiled) ? bInfo->buffInfo.bufStrideInTiles : bInfo->vidInfo.stride;

  /* Video pixel buffer threshold */
  {
    FxU32 vidPixelBufThold;
    FxU32 thold = 32;

    if (getenv("SSTVB_PIXTHOLD")) {
      thold = atoi(getenv("SSTVB_PIXTHOLD"));
    }

    thold &= 0x3f;

    vidPixelBufThold = (thold | (thold << 6) | (thold << 12));

    HWC_IO_STORE(bInfo->regInfo, vidPixelBufThold, vidPixelBufThold);
  }

  /* Check to see if the screen and overlay dimensions match. 
   * There are two cases that can happen in reality.
   *
   * scrXXX > appXXX: This is a 'real' case, and the overlay dimension
   * needs to mag scaled so that it fits the requested size.
   *
   * (scrXXX == appXXX) && (ovlXXX != scrXXX): This is a somewhat artificial
   * case where someone left the overlay set to some value, and these did
   * not get reset in the setVideoMode processing. (For example, if the user is
   * running an application which bus masters data directly to our video overlay
   * when launching a glide application). In this case we need to fiddle w/ the
   * overlay dimension so that it matches the requested resolution.
   *
   * (scrXXX < appXXX): If setVideoMode is actually working correctly, this cannot
   * happen because that code has to know that we can't do min scaling.
   */

  /* Get miscInit0 for y-sub  */
  HWC_IO_LOAD(bInfo->regInfo, miscInit0, miscInit0);

  /* Clear out relavent bits */
  miscInit0 &= ~SST_YORIGIN_TOP;
  miscInit0 |= ((driInfo.screenHeight - 1)  << SST_YORIGIN_TOP_SHIFT);
  HWC_IO_STORE(bInfo->regInfo, miscInit0, miscInit0);

  /* Set up dramInit1 for triple or double buffering */
  HWC_IO_LOAD(bInfo->regInfo, dramInit1, dramInit1);
  if (bInfo->vidInfo.tripleBuffering)
    dramInit1 |= SST_TRIPLE_BUFFER_EN;
  else
    dramInit1 &= ~SST_TRIPLE_BUFFER_EN;
  HWC_IO_STORE(bInfo->regInfo, dramInit1, dramInit1);

  HWC_IO_STORE(bInfo->regInfo, vidMaxRGBDelta, 0x100810);

  return FXTRUE;
}

FxBool
hwcRestoreVideo(hwcBoardInfo *bInfo) {
  return FXTRUE;
}

static FxU32
calcBufferStride(FxU32 xres, FxBool tiled)
{
  FxU32
    strideInTiles;

  if (tiled == FXTRUE) {
    /* Calculate tile width stuff */
    strideInTiles = (xres << 1) >> 7;
    if ((xres << 1) & (HWC_TILE_WIDTH - 1))
      strideInTiles++;
    
    return (strideInTiles * HWC_TILE_WIDTH);

  } else {
    return (xres << 1);
  }
} /* calcBufferStride */

static FxU32
calcBufferHeightInTiles(FxU32 yres)
{
  FxU32
    heightInTiles;            /* Height of buffer in tiles */


  /* Calculate tile height stuff */
  heightInTiles = yres >> 5;
  
  if (yres & (HWC_TILE_HEIGHT - 1))
    heightInTiles++;

  return heightInTiles;

} /* calcBufferHeightInTiles */

static FxU32
calcBufferSizeInTiles(FxU32 xres, FxU32 yres) {
  FxU32
    bufSizeInTiles;           /* Size of buffer in tiles */

  bufSizeInTiles =
    calcBufferHeightInTiles(yres) * (calcBufferStride(xres, FXTRUE) >> 7);

  return bufSizeInTiles;

} /* calcBufferSizeInTiles */

static FxU32
calcBufferSize(FxU32 xres, FxU32 yres, FxBool tiled)
{
  FxU32
    stride,
    height,
    bufSize;                  /* Size of buffer in bytes */

  if (tiled) {
    stride = calcBufferStride(xres, tiled);
    height = HWC_TILE_HEIGHT * calcBufferHeightInTiles(yres);
  } else {
    stride = xres << 1;
    height = yres;
  }

  bufSize = stride * height;
  
  return bufSize;

} /* calcBufferSize */

FxBool
hwcCheckMemSize(hwcBoardInfo *bInfo, FxU32 xres, FxU32 yres, FxU32 nColBuffers,
                FxU32 nAuxBuffers, FxBool tiled)
{
#define FN_NAME "hwcCheckMemSize"
  FxU32
    bufSize, totSize;

  bufSize = calcBufferSize(xres, yres, tiled);

  totSize = (nColBuffers + nAuxBuffers) * bufSize;

  if (totSize < ((bInfo->h3Mem << 20) - 0x200000)) /* Need 2M for texture */
    return FXTRUE;
  else
    return FXFALSE;    
#undef FN_NAME
} /* hwcCheckMemSize */

/* How the hw treats lfb accesses are dependent on the 'type' of
 * memory (tiled/linear) that the color/aux buffers are in. We
 * pre-compute the actual lfb address here while we know about the
 * memory space and if we adjusted the page alignment above.
 *
 * NB: If we are in tiled mode then the fact that we align the color
 * buffers on page boundaries means that the y offset of the buffers
 * may not actually be on a boundary for the tile addressing scheme.
 * The 'rounding' done to HWC_TILED_BUFFER_Y_ALIGN adjust for this.
 *
 * NB: The memory optimization of aligning color buffers on even page
 * boundaries will cause the tiled lfb access to be off by a page so
 * we add in the width of a page (HWC_TILED_BUFFER_X_ADJUST) here.
 */
static FxU32
hwcBufferLfbAddr(const hwcBoardInfo *bInfo, FxU32 physAddress)
{
  FxU32 retVal = 0x00UL;

  FxU32 tileAddress;
  FxU32 tileNumber;
  FxU32 tileOffset;
  FxU32 tileXOffset;
  FxU32 tileScanline;
  FxU32 tileRow;
  FxU32 lfbAddress;
  FxU32 lfbYOffset;
  /*
   * This is the tile aperture stride.  It is always 4096 for V3.
   */
  FxU32 lfbBufferStride = 0x1000;

  if (bInfo->vidInfo.tiled) {    
    GDBG_INFO(80, "\tphysAddress: 0x%08lx\n",physAddress);

    /* Compute address in tile space */
    tileAddress = physAddress - driInfo.backOffset;
    GDBG_INFO(80, "\ttileAddress: 0x%08lx\n",tileAddress);

    /* Compute tile number we're in (each tile is 4K bytes) */
    tileNumber = tileAddress >> 12;
    GDBG_INFO(80, "\ttileNumber: 0x%08lx (%d)\n",tileNumber,tileNumber);

    /* Compute base tile row we're in */
    tileRow = tileNumber / bInfo->buffInfo.bufStrideInTiles;
    GDBG_INFO(80, "\ttileRow: %d  (stride = %d)\n",tileNumber,bInfo->buffInfo.bufStrideInTiles);

    /* Compute offset within the tile */
    tileOffset = tileAddress - (tileNumber << 12);
    GDBG_INFO(80, "\ttileOffset: 0x%08lx\n",tileOffset);

    /* Compute scanline within the tile */
    tileScanline = tileOffset >> 7;
    GDBG_INFO(80, "\ttileScanline: 0x%08lx\n",tileScanline);

    /* Compute tile X offset within the row */
    tileXOffset = tileNumber - (tileRow * bInfo->buffInfo.bufStrideInTiles);
    GDBG_INFO(80, "\ttileXOffset: %d\n",tileXOffset);

    /* Compute Y offset in LFB space */
    lfbYOffset = (tileRow * 32 + tileScanline);

    /* Compute LFB address of tile start */
    lfbAddress =  driInfo.backOffset + lfbYOffset * lfbBufferStride + tileXOffset * 128;

    GDBG_INFO(80, "\tlfbAddress: %08lx\n", lfbAddress);
    retVal = lfbAddress;
  } else {
    retVal = physAddress;
  }
  return retVal;
}
FxU32 
hwcInitAGPFifo(hwcBoardInfo *bInfo, FxBool enableHoleCounting) 
{
  return hwcInitFifo(bInfo, enableHoleCounting); 
}

#define RED_SHIFT       16
#define GREEN_SHIFT     8
#define BLUE_SHIFT      0

FxBool
hwcGammaTable(hwcBoardInfo *bInfo, FxU32 nEntries, FxU32 *r, FxU32 *g, FxU32 *b)
{
#define FN_NAME "hwcGammaTable"
  FxU32 gRamp[256];
  FxU32 i;
  FxU32 dacBase;
  FxU32 rDacBase;
  FxU32 rDacData;
  
  /* Load the table into the Display driver as above */
  for (i = 0; i < nEntries; i++) {
    gRamp[i] =
      ((r[i] & 0xff) << RED_SHIFT) |
        ((g[i] & 0xff) << GREEN_SHIFT) |  
          ((b[i] & 0xff) << BLUE_SHIFT);
  }
  
  dacBase = 0;
 
  for (i = 0; i < nEntries; i++) {
    int repeat = 100;
    do {
      HWC_IO_STORE( bInfo->regInfo, dacAddr, dacBase + i);
      P6FENCE;
      HWC_IO_LOAD( bInfo->regInfo, dacAddr, rDacBase);
      P6FENCE;
      repeat--;
    } while (repeat && rDacBase != dacBase + i);
    if (!repeat) {
      GDBG_INFO(0, "%s:Error Writting DacAddr %d. DacBase =%d\n",
		FN_NAME, dacBase+i, dacBase);
    }
    repeat = 100;
    do {
      HWC_IO_STORE( bInfo->regInfo, dacData, gRamp[i]);
      P6FENCE;
      HWC_IO_LOAD( bInfo->regInfo, dacData, rDacData);
      P6FENCE;
      repeat--;
    } while (repeat && rDacData != gRamp[i]);
    if (!repeat) {
      GDBG_INFO(0, "%s:Error Writting Data [%d, %x]. DacBase =%d\n",
		FN_NAME, i, gRamp[i], dacBase);
    }
  }

  return FXTRUE;
  
#undef FN_NAME
} /* hwcGammaTable */

FxBool
hwcGammaRGB(hwcBoardInfo *bInfo, float gammaR, float gammaG, float gammaB)
{
#define FN_NAME "hwcGammaRGB"
  FxU32
    grRamp[256], ggRamp[256], gbRamp[256];
  int
    i;

  GDBG_INFO(80, FN_NAME "(0x%x, %1.2f, %1.2f, %1.2f)\n",
            bInfo, gammaR, gammaG, gammaB);

  /*
  ** NB:  The system eventually devised by Bob and Ken *may* require
  ** separate R, G, and B vectors.
  */

  for (i = 0; i < 256; i++) {
    grRamp[i] = (FxU32)((pow(i/255.0F, 1.0F/gammaR)) * 255.0F + 0.5F);
    ggRamp[i] = (FxU32)((pow(i/255.0F, 1.0F/gammaG)) * 255.0F + 0.5F);
    gbRamp[i] = (FxU32)((pow(i/255.0F, 1.0F/gammaB)) * 255.0F + 0.5F);
  }


  hwcGammaTable(bInfo, 256, grRamp, ggRamp, gbRamp); 

  /*
  **  Now that we have a gamma table, we can give it to the driver via
  **  a call to ExtEscape() when that is defined.....
  */

  return FXFALSE;

#undef FN_NAME
} /* hwcGammaRGB */

#define M 1
#define K 1

FxBool
hwcSetGrxClock(hwcBoardInfo *bInfo, FxU32 speedInMHz)
{
#define FN_NAME "hwcSetGrxClock"
  FxU32
    pllCtrl1,
    dramInit0 = 0xc17ae29, 
    dramInit1 = 0x26c031,
    n,
    m = 1;

  n = (FxU32) ((speedInMHz - 4.76f)/2.38f);

  pllCtrl1 =
    (K << SST_PLL_K_SHIFT) | (m << SST_PLL_M_SHIFT) | (n << SST_PLL_N_SHIFT);

  GDBG_INFO(80, "%s:  Setting Graphics Clock to %d\n", FN_NAME, speedInMHz);

  HWC_IO_STORE( bInfo->regInfo, dramInit0, dramInit0);
  HWC_IO_STORE( bInfo->regInfo, dramInit1, dramInit1);
  HWC_IO_STORE( bInfo->regInfo, pllCtrl1, pllCtrl1);
  
  return FXTRUE;

#undef FN_NAME  
} /* hwcSetGrxClock */

FxBool
hwcSetMemClock(hwcBoardInfo *bInfo, FxU32 speedInMHz)
{
#define FN_NAME "hwcSetMemClock"
  return FXFALSE;
#undef FN_NAME  
} /* hwcSetMemClock */

char *
hwcGetenv(char *a) 
{
  envitem *ptr;

  char *result;
  result=getenv(a);
  if (result) return result;
  if (!envinit) loadEnvFile();
  ptr=first;
  while (ptr) {
    if (!strcmp(ptr->env, a)) return ptr->val;
    ptr=ptr->next;
  }
  return 0;
}

FxBool
hwcResolutionSupported(hwcBoardInfo *bInfo, GrScreenResolution_t res)
{
#define FN_NAME "hwcResolutionSupported"

  /* For now we're just going to retur TRUE. We should check with
     vmode extension to see if the requested size is available */
  return FXTRUE;

#undef FN_NAME
} /* hwcResolutionSupported */

void grDRIImportFifo(int fifoPtr, int fifoRead)
{
  _grImportFifo(fifoPtr, fifoRead);
}

void grDRIInvalidateAll() {
  _grInvalidateAll();
}

void grDRIResetSAREA()
{
  _grExportFifo((int *)driInfo.fifoPtr, (int *)driInfo.fifoRead);
}
