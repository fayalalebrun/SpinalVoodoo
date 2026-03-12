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
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>

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
static FxU32 saveVidProcCfg;
static FxU32 saveLfbMemoryConfig;
static FxU32 saveMiscInit0;
static FxU32 finalVidProcCfg;
static FxU32 finalLfbMemoryCfg;
static FxU32 finalVidDesktopOverlayStride;

static Display		    *dpy	= 0;
static int		    screenNum   = 0;
static int		    screenWidth = 0;
static int		    screenHeight= 0;
static int		    screenDepth = 0;
static void*		    screenPhys  = 0;/* not really used */
static int		    screenMem   = 0;
static XF86VidModeModeInfo  **vidModes  = 0;
static int		    inDGA       = 0;

static FxU32 calcBufferStride(FxU32 xres, FxBool tiled);
static FxU32 calcBufferSize(FxU32 xres, FxU32 yres, FxBool tiled);
static FxU32 calcBufferSizeInTiles(FxU32 xres, FxU32 yres);
static FxU32 calcBufferHeightInTiles(FxU32 yres);
static FxU32 hwcBufferLfbAddr(FxU32 bufNum, const hwcBoardInfo* bInfo,
			      FxBool colBufAlignP);

typedef struct envitem_t {
  char *env;
  char *val;
  struct envitem_t *next;
} envitem;

static envitem *first=0;
static int envinit=0;

static void loadEnvFile(void) {
  FILE *file;
  char data[128];
  char *env, *val;
  envitem *item;
  int first_err=1;

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
      if (first_err) {
        fprintf(stderr, "In config file /etc/conf.3dfx/voodoo3:\n");
        first_err=0;
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
hwcGetErrorString(void)
{
#define FN_NAME "hwcGetErrorString"
  return errorString;
#undef FN_NAME
} /* hwcGetErrorString */

static int initX(int index) {
  int eventbase, errorbase, flags;
  int banksize;
  unsigned int baseaddr;

  if (dpy!=0) return FXTRUE;
  dpy=XOpenDisplay("");
  if (!dpy) {
    sprintf(errorString, "Banshee/V3 only runs under local X connection\n");
    return FXFALSE;
  }
  if (XF86DGAQueryExtension(dpy, &eventbase, &errorbase)) {
    int major, minor;
    XF86DGAQueryVersion(dpy, &major, &minor);
    if (major>1 && minor<0) {
      XCloseDisplay(dpy);
      sprintf(errorString,"Banshee/V3 requires DGA version 1.x\n");
      return FXFALSE;
    }
  }
  screenNum=DefaultScreen(dpy);
  XF86DGAGetVideoLL(dpy, screenNum, &baseaddr, &screenWidth, &banksize,
		    &screenMem);
  screenPhys = (void *)baseaddr;
  screenMem*=1024;
  screenHeight=DisplayHeight(dpy, screenNum);
  screenDepth=DefaultDepth(dpy, screenNum);
  if (XF86DGAQueryDirectVideo(dpy, DefaultScreen(dpy), &flags)==False ||
      !flags) {
    sprintf(errorString,"Banshee/V3 requires X server to suppoer direct video\n");
    return FXFALSE;
  }
  hInfo.boardInfo[index].h3Mem = screenMem>>20;
  return FXTRUE;
}

hwcInfo *
hwcInit(FxU32 vID, FxU32 dID) {
  int i;
  FxU32 bn;

  pciOpen();

  hInfo.nBoards = 0;

  errorString[0] = '\0';

  for (i = 0; i < HWC_MAX_BOARDS; i++) {
    hInfo.boardInfo[i].pciInfo.initialized = 0;
    if (pciFindCardMulti(vID, dID, &bn, i)) {
      hInfo.nBoards++;
      hInfo.boardInfo[i].boardNum = 0;

      hInfo.boardInfo[i].pciInfo.initialized = 1;
      hInfo.boardInfo[i].pciInfo.vendorID = vID;
      hInfo.boardInfo[i].pciInfo.deviceID = dID;
      /*
       * NOTE: in the code above we learn about memsize here:
       *   hInfo.boardInfo[i].h3Mem
       *
       * However, in DOS, since we have not mapped the board yet, so we have
       * to wait until later. (see hwcInitRegisters())  - dwj
       */

      /* Get some board Info */
      pciGetConfigData( PCI_REVISION_ID, bn, &hInfo.boardInfo[i].devRev);

      /* Get all the base addresses */
      pciGetConfigData(PCI_BASE_ADDRESS_0, bn,
		       &hInfo.boardInfo[i].pciInfo.pciBaseAddr[0]);
      pciGetConfigData(PCI_BASE_ADDRESS_1, bn,
		       &hInfo.boardInfo[i].pciInfo.pciBaseAddr[1]);
      pciGetConfigData(PCI_IO_BASE_ADDRESS, bn,
		       &hInfo.boardInfo[i].pciInfo.pciBaseAddr[2]);
      pciGetConfigData(PCI_ROM_BASE_ADDRESS, bn,
		       &hInfo.boardInfo[i].pciInfo.pciBaseAddr[3]);
    }
  }
  if (hInfo.nBoards) {
    if (!initX(0)) return 0;
    return &hInfo;
  } else {
    sprintf(errorString, "Can't find or access Banshee/V3 board\n");
    return 0;
  }
}

FxBool
hwcMapBoard(hwcBoardInfo *bInfo, FxU32 bAddrMask) {
  FxU32 bAddr;

  if (bInfo->pciInfo.initialized == FXFALSE) {
    sprintf(errorString, "hwcMapBoard: Called before hwcInit\n");
    return FXFALSE;
  }

  bInfo->linearInfo.initialized = FXTRUE;

  for (bAddr = 0; bAddr < HWC_NUM_BASE_ADDR; bAddr++) {
    if ((bAddrMask >> bAddr) & 0x1) {
      bInfo->linearInfo.linearAddress[bAddr] = (FxU32) 
	pciMapCardMulti(bInfo->pciInfo.vendorID, bInfo->pciInfo.deviceID,
			0x2000000, &bInfo->deviceNum, bInfo->boardNum, bAddr);
    }
  }
  return FXTRUE;
}

FxBool
hwcUnmapBoard(hwcBoardInfo *bInfo) {
  FxU32 bAddr;

  for (bAddr=0; bAddr<HWC_NUM_BASE_ADDR; bAddr++) {
    pciUnmapPhysical(bInfo->linearInfo.linearAddress[bAddr], 0x2000000);
  }
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
  FxBool
    bufferAlignP;
  FxU32
    bNum,
    h3Mem = bInfo->h3Mem << 20,
    bufStride, 
    bufSize;
  FxI32
    i,
    tramSize, fifoSize;

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
    calcBufferStride(bInfo->vidInfo.xRes, bInfo->vidInfo.tiled);   

  /* We want to place the FIFO after the tram but before the color
     buffers with some pad */
  bufSize = calcBufferSize(bInfo->vidInfo.xRes, bInfo->vidInfo.yRes,
    bInfo->vidInfo.tiled); 

  bInfo->buffInfo.bufStride = bufStride;
  bInfo->buffInfo.bufSize = bufSize;

  if (bInfo->vidInfo.tiled) {
    bInfo->buffInfo.bufStrideInTiles = (bufStride >> 7);
    bInfo->buffInfo.bufSizeInTiles =
      calcBufferSizeInTiles(bInfo->vidInfo.xRes, bInfo->vidInfo.yRes);
    bInfo->buffInfo.bufHeightInTiles =
      calcBufferHeightInTiles(bInfo->vidInfo.yRes);
  }

  bInfo->buffInfo.initialized = FXTRUE;
  bInfo->buffInfo.nColBuffers = nColBuffers;
  bInfo->buffInfo.nAuxBuffers = nAuxBuffers;

  /* Subtract 0x1000 from h3mem to leave the X cursor (in high mem) alone */
  h3Mem-=0x1000;

  /* First, do the buffer allocation */

  if (nAuxBuffers > 0) {
    bInfo->buffInfo.auxBuffEnd  =
    bInfo->buffInfo.auxBuffStart = h3Mem;

    bInfo->buffInfo.auxBuffStart -= bufSize;

    /* auxBuffers start on odd pages, so we need to check to see if
     * it's on an even page and, if so, make it odd. 
     *
     * NB: We need to do the same sort of 'alignment' thing here as for
     * the color buffers.
     */
    bufferAlignP = ((bInfo->buffInfo.auxBuffStart & 0x1000UL) == 0);
    if (bufferAlignP) bInfo->buffInfo.auxBuffStart -= 0x1000;

  }

  for (i = nColBuffers - 1; i >= 0; i--) {
    if ((FxU32) i == (nColBuffers - 1)) {
      FxU32 top;
      if (nAuxBuffers > 0) 
        top = bInfo->buffInfo.auxBuffStart;
      else
        top = h3Mem;

      bInfo->buffInfo.colBuffStart[i] = 
        bInfo->buffInfo.colBuffEnd[i] = top;
    } else {
      bInfo->buffInfo.colBuffStart[i] = 
        bInfo->buffInfo.colBuffEnd[i] = 
        bInfo->buffInfo.colBuffStart[i + 1];
    }

    bInfo->buffInfo.colBuffStart[i] -= bufSize;

    /* As a memory access optmization colorBuffers start on even
     * pages, while aux buffers start on odd pages.  Thus we must
     * check to see if we're startding on an odd page here and, if so,
     * add a page to the start.  
     */
    bufferAlignP = ((bInfo->buffInfo.colBuffStart[i] & 0x1000UL) != 0);
    if (bufferAlignP) bInfo->buffInfo.colBuffStart[i] -= 0x1000;
  }

  /* Now we can calculate some other stuff... */
  bInfo->fbOffset =  bInfo->buffInfo.colBuffStart[0];
  bInfo->tramOffset = 0;
  fifoSize = MAXFIFOSIZE;
  tramSize =  bInfo->fbOffset - bInfo->tramOffset - fifoSize;
  if (tramSize < (FxI32)bInfo->min_tramSize) {
    /* Now we have to shrink the FIFO */
    tramSize = bInfo->min_tramSize;
    fifoSize = bInfo->fbOffset - bInfo->min_tramSize;
    if (fifoSize < 0) {
      GDBG_INFO(80, "%s:  Not enough memory for resolution + min texture\n",
        FN_NAME);
      sprintf(errorString, 
        "%s:  Not enough memory for resolution + min texture\n",
        FN_NAME);
      return FXFALSE;
    } 
  }

  bInfo->fifoInfo.fifoLength = (FxU32) fifoSize;
  bInfo->fifoInfo.fifoStart = bInfo->fbOffset - fifoSize;

  bInfo->fifoInfo.fifoLength -= 0x20000;

  if (tramSize>screenWidth*screenHeight*(screenDepth+7)/8+bInfo->min_tramSize) {
    bInfo->tramOffset = screenWidth*screenHeight*(screenDepth+7)/8;
    bInfo->tramSize = (FxU32)(tramSize-screenWidth*screenHeight*(screenDepth+7)/8);
  } else {
    bInfo->tramOffset = 0;
    bInfo->tramSize = (FxU32)tramSize;
  }

#define LFBSTRIDE 0x1000
  {
    FxU32 colBuffOffset = 0;
    FxU32 colBufAlignP = 0;

    for (bNum = 0; bNum < nColBuffers; bNum++) {
      if ((colBuffOffset & 0x1000UL) != 0) {
        colBuffOffset += 0x1000UL;
        colBufAlignP++;
      }
      bInfo->buffInfo.lfbBuffAddr[bNum] = 
        hwcBufferLfbAddr(bNum, bInfo, colBufAlignP);
      colBuffOffset += bufSize;
    }

    if (nAuxBuffers > 0) {
      if ((colBuffOffset & 0x1000UL) != 0) {
        colBuffOffset += 0x1000UL;
        colBufAlignP++;
      }
      bInfo->buffInfo.lfbBuffAddr[nColBuffers] =
        hwcBufferLfbAddr(nColBuffers, bInfo, colBufAlignP);
    }
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
  GDBG_INFO(80, "\tcolBuffStart:    0x%x\n", bInfo->buffInfo.colBuffStart);
  GDBG_INFO(80, "\tcolBuffEnd:      0x%x\n", bInfo->buffInfo.colBuffEnd);
  GDBG_INFO(80, "\tnAuxBuffers:     0x%x\n", bInfo->buffInfo.nAuxBuffers);
  GDBG_INFO(80, "\tauxBuffStart:    0x%x\n", bInfo->buffInfo.auxBuffStart);
  GDBG_INFO(80, "\tauxBuffEnd:      0x%x\n", bInfo->buffInfo.auxBuffEnd);

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
  FxBool
    agpEnable = FXFALSE;
  FxU32
    cagpRegs;                   /* pointer to Cmd/AGP regs */

  if (bInfo->regInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcMapBoard\n", FN_NAME);
    return FXFALSE;
  }
  cagpRegs = bInfo->regInfo.cmdAGPBase;

  if (bInfo->buffInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcInitBuffers\n", FN_NAME);
    return FXFALSE;
  }

  /* disable the CMD fifo */
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize, 0);

  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseAddrL,
    bInfo->fifoInfo.fifoStart>>12);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.readPtrL, bInfo->fifoInfo.fifoStart);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.readPtrH, 0);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.aMin, bInfo->fifoInfo.fifoStart-4);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.aMax, bInfo->fifoInfo.fifoStart-4);

  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.depth, 0);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.holeCount, 0);
  /* Fifo LWM /HWM/ THRESHOLD */
  if (bInfo->pciInfo.deviceID == 0x3) { /* banshee */
    HWC_CAGP_STORE(bInfo->regInfo, cmdFifoThresh,
                   (0x09 << SST_HIGHWATER_SHIFT) | 0x2);
  } else {
    HWC_CAGP_STORE(bInfo->regInfo, cmdFifoThresh,
                   (0xf << SST_HIGHWATER_SHIFT) | 0x8);
  }
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize,
    ((bInfo->fifoInfo.fifoLength >> 12) - 1) | SST_EN_CMDFIFO |
    (enableHoleCounting ? 0 : SST_CMDFIFO_DISABLE_HOLES) |
    (agpEnable ?  SST_CMDFIFO_AGP : 0));

  GDBG_INFO(80 ,"%s:  CMD FIFO placed at physical addr 0x%x\n", FN_NAME,
    bInfo->fifoInfo.fifoStart);

  return FXTRUE;

#undef FN_NAME
} /* hwcInitFifo */

static void
hwcInitVideoOverlaySurface(
  hwcRegInfo *rInfo,
    FxU32 enable,               /* 1=enable Overlay surface (OS), 1=disable */
    FxU32 stereo,               /* 1=enable OS stereo, 0=disable */
    FxU32 horizScaling,         /* 1=enable horizontal scaling, 0=disable */
    FxU32 dudx,                 /* horizontal scale factor (ignored if not */
      /* scaling) */
    FxU32 verticalScaling,      /* 1=enable vertical scaling, 0=disable */
    FxU32 dvdy,                 /* vertical scale factor (ignored if not */
      /* scaling) */
    FxU32 filterMode,           /* duh */
    FxU32 tiled,                /* 0=OS linear, 1=tiled */
    FxU32 pixFmt,               /* pixel format of OS */
    FxU32 clutBypass,           /* bypass clut for OS? */
    FxU32 clutSelect,           /* 0=lower 256 CLUT entries, 1=upper 256 */
    FxU32 startAddress,         /* board address of beginning of OS */
    FxU32 stride)               /* distance between scanlines of the OS, in */
  /* units of bytes for linear OS's and tiles for */
  /* tiled OS's */
{
  FxU32 doStride;
  FxU32 vidProcCfg;

  HWC_IO_LOAD((*rInfo), vidProcCfg, vidProcCfg);

  vidProcCfg &= ~(SST_OVERLAY_TILED_EN |
    SST_OVERLAY_STEREO_EN |  
    SST_OVERLAY_HORIZ_SCALE_EN |
    SST_OVERLAY_VERT_SCALE_EN |
    SST_OVERLAY_TILED_EN |
    SST_OVERLAY_PIXEL_FORMAT |
    SST_OVERLAY_CLUT_BYPASS |
    SST_OVERLAY_CLUT_SELECT);

  if (enable)
    vidProcCfg |= SST_OVERLAY_EN;

  if (stereo)
    vidProcCfg |= SST_OVERLAY_STEREO_EN;

  if (horizScaling)
    vidProcCfg |= SST_OVERLAY_HORIZ_SCALE_EN;

  if (verticalScaling)
    vidProcCfg |= SST_OVERLAY_VERT_SCALE_EN;

  if (tiled) {
    vidProcCfg |= SST_OVERLAY_TILED_EN;
  }

  vidProcCfg |= pixFmt;

  vidProcCfg &= ~SST_CURSOR_EN; /* Turn off HW Cursor */

  if (clutBypass)
    vidProcCfg |= SST_OVERLAY_CLUT_BYPASS;

  if (clutSelect)
    vidProcCfg |= SST_OVERLAY_CLUT_SELECT;

  /* Overlay Hack: setup the state that I want */
  if (GETENV ("SSTH3_DESKTOP_OVERLAY"))
  {
    /* Check this because my cursor gets funny */
    vidProcCfg |= SST_CURSOR_EN;

    vidProcCfg |= SST_CHROMA_EN;

    /* Tile the 3D which will use the DESKTOP */
    /* Don't tile the 2D which will be the OVERLAY */
    vidProcCfg |=  SST_DESKTOP_TILED_EN;
    vidProcCfg &= ~SST_OVERLAY_TILED_EN;

    /* Setup the chroma range to magenta. */
    /* FIXME: it would be nice to be able to pick this */
    HWC_IO_STORE ((*rInfo), vidChromaMin, 0x0000F81F);
    HWC_IO_STORE ((*rInfo), vidChromaMax, 0x0000F81F);
  }

  HWC_IO_STORE((*rInfo), vidProcCfg, vidProcCfg);

  /* */
  HWC_IO_LOAD((*rInfo), vidDesktopOverlayStride, doStride);
  doStride &= ~(SST_OVERLAY_LINEAR_STRIDE | SST_OVERLAY_TILE_STRIDE);

  /* Overlay Hack: leave the stride alone for the hack */
  if (!GETENV ("SSTH3_DESKTOP_OVERLAY"))
  {
    stride <<= SST_OVERLAY_STRIDE_SHIFT;
    if (tiled)
      stride &= SST_OVERLAY_TILE_STRIDE;
    else
      stride &= SST_OVERLAY_LINEAR_STRIDE;
    doStride |= stride;
  }

  HWC_IO_STORE((*rInfo), vidDesktopOverlayStride, doStride);

  finalVidDesktopOverlayStride = doStride;

} /* hwcInitVideoOverlaySurface */

FxBool
hwcInitVideo(hwcBoardInfo *bInfo, FxBool tiled, FxVideoTimingInfo
             *vidTiming, FxBool overlay) {
  FxU32 pixFmt = SST_OVERLAY_PIXEL_RGB565D;
  FxU32 stride, dramInit1, miscInit0, lfbMemoryConfig;
  FxU32 scrWidth, scrHeight, ovlWidth, ovlHeight, scale,
      vidOverlayDudx, vidOverlayDvdy,
      vidProcCfg, vidScreenSize, vidOverlayEndScreenCoord;
  int i, numModes;

  if (!dpy) {
    if (!initX(0)) /* Should be the index of the card */
      return FXFALSE;
  }
  if (!GETENV ("SSTH3_DESKTOP_OVERLAY"))
  {
    if (XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectGraphics)) {
      inDGA=1;
    }
  }
  if (XF86VidModeGetAllModeLines(dpy, DefaultScreen(dpy), &numModes,
				 &vidModes)==False) {
    sprintf(errorString, "Couldn't query vidmode list\n");
    return FXFALSE;
  }
  for (i=0; i<numModes; i++)
    if ((vidModes[i]->hdisplay==bInfo->vidInfo.xRes) && 
	(vidModes[i]->vdisplay==bInfo->vidInfo.yRes))
	break;
  if (i==numModes) {
    sprintf(errorString, "Server doesn't support requested resolution\n");
    if (inDGA) {
      XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
      inDGA=0;
    }
    if (vidModes) {
      XFree(vidModes);
      vidModes=0;
    }
    return FXFALSE;
  }
  if (XF86VidModeSwitchToMode(dpy, DefaultScreen(dpy), vidModes[i])==False) {
    sprintf(errorString, "Failed to set required video mode\n");
    return FXFALSE;
  }
  XSync(dpy, False);

  stride= (tiled) ? bInfo->buffInfo.bufStrideInTiles : bInfo->vidInfo.stride;

  HWC_IO_LOAD(bInfo->regInfo, vidProcCfg, saveVidProcCfg);
  HWC_IO_LOAD(bInfo->regInfo, lfbMemoryConfig, saveLfbMemoryConfig);
  HWC_IO_LOAD(bInfo->regInfo, miscInit0, saveMiscInit0);

  HWC_IO_STORE(bInfo->regInfo, vidOverlayDudxOffsetSrcWidth,
    ((bInfo->vidInfo.xRes << 1) << 19));

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

  hwcInitVideoOverlaySurface(
    &bInfo->regInfo,
    FXTRUE,                     /* 1=enable Overlay surface (OS), 1=disable */
    FXFALSE,                    /* 1=enable OS stereo, 0=disable */
    FXFALSE,                     /* 1=enable horizontal scaling, 0=disable */
    0,                          /* horizontal scale factor (ignored if not) */
    FXFALSE,                    /* 1=enable vertical scaling, 0=disable */
    0,                          /* vertical scale factor (ignored if not) */
    0,                          /* Filter mode */
    tiled,                      /* tiled */
    pixFmt,                     /* pixel format of OS */
    FXFALSE,                    /* bypass clut for OS? */
    0,                          /* 0=lower 256 CLUT entries, 1=upper 256 */
    bInfo->buffInfo.colBuffStart[0],/* board address of beginning of OS */
    stride);                    /* distance between scanlines of the OS, in
                                   units of bytes for linear OS's and tiles for
                                   tiled OS's */
  HWC_IO_STORE(bInfo->regInfo, vidOverlayStartCoords, 0);
  HWC_IO_STORE(bInfo->regInfo, vidOverlayEndScreenCoord,
    (bInfo->vidInfo.yRes  << SST_OVERLAY_Y_SHIFT) |
    (bInfo->vidInfo.xRes & SST_OVERLAY_X) );

  /*
     Setup video scaling for half-modes
   */

  /* Get some important info */
  HWC_IO_LOAD(bInfo->regInfo, vidScreenSize, vidScreenSize);
  HWC_IO_LOAD(bInfo->regInfo, vidProcCfg, vidProcCfg);
  HWC_IO_LOAD(bInfo->regInfo, vidOverlayEndScreenCoord, vidOverlayEndScreenCoord);

  scrWidth = (vidScreenSize >> SST_VIDEO_SCREEN_WIDTH_SHIFT) & 0xfff;
  scrHeight = (vidScreenSize >> SST_VIDEO_SCREEN_HEIGHT_SHIFT) & 0xfff;

  ovlWidth = ((vidOverlayEndScreenCoord) >> SST_OVERLAY_X_SHIFT) & 0xfff;
  ovlHeight = ((vidOverlayEndScreenCoord) >> SST_OVERLAY_Y_SHIFT) & 0xfff;  

  ovlWidth += 1;
  ovlHeight += 1;

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

  if (scrWidth > bInfo->vidInfo.xRes) {
    vidProcCfg |= SST_OVERLAY_HORIZ_SCALE_EN;

    ovlWidth = scrWidth;

    scale = ((float) bInfo->vidInfo.xRes) / ((float) ovlWidth);

    vidOverlayDudx = (FxU32) (scale * ((float) (1 << 20)));
    HWC_IO_STORE(bInfo->regInfo, vidOverlayDudx, vidOverlayDudx);
  } else if (ovlWidth != scrWidth) {
    ovlWidth = scrWidth;
  }

  if (scrHeight > bInfo->vidInfo.yRes) {
    vidProcCfg |= SST_OVERLAY_VERT_SCALE_EN;

    ovlHeight = scrHeight;

    scale = ((float) bInfo->vidInfo.yRes) / ((float) ovlHeight);

    vidOverlayDvdy = (FxU32) (scale * ((float) (1 << 20)));
    HWC_IO_STORE(bInfo->regInfo, vidOverlayDvdy, vidOverlayDvdy);
  } else if (ovlHeight != scrHeight) {
    ovlHeight = scrHeight;
  }

  vidOverlayEndScreenCoord = (((ovlHeight - 1) << SST_OVERLAY_Y_SHIFT) |
                              ((ovlWidth - 1) << SST_OVERLAY_X_SHIFT));
  HWC_IO_STORE(bInfo->regInfo, vidOverlayEndScreenCoord, vidOverlayEndScreenCoord);

  vidProcCfg &= ~SST_OVERLAY_FILTER_MODE;
  if (GETENV("SSTH3_OVERLAY_FILTER_2x2")) {
    if (!(vidProcCfg & SST_VIDEO_2X_MODE_EN))
      vidProcCfg |= SST_OVERLAY_FILTER_2X2;
  } else {
    vidProcCfg |= SST_OVERLAY_FILTER_4X4;
  }

  HWC_IO_STORE(bInfo->regInfo, vidProcCfg, vidProcCfg);

  finalVidProcCfg = vidProcCfg;

  /* Get miscInit0 for y-sub  */
  HWC_IO_LOAD(bInfo->regInfo, miscInit0, miscInit0);

  /* Clear out relavent bits */
  miscInit0 &= ~SST_YORIGIN_TOP;
  miscInit0 |= ((bInfo->vidInfo.yRes - 1)  << SST_YORIGIN_TOP_SHIFT);

  HWC_IO_STORE(bInfo->regInfo, miscInit0, miscInit0);

  /* Set up lfbMemoryConfig */
  lfbMemoryConfig =
    (bInfo->fbOffset >> 12) | 
    SST_RAW_LFB_ADDR_STRIDE_4K |
    (bInfo->buffInfo.bufStrideInTiles << SST_RAW_LFB_TILE_STRIDE_SHIFT);

  HWC_IO_STORE(bInfo->regInfo, lfbMemoryConfig, lfbMemoryConfig);

  finalLfbMemoryCfg = lfbMemoryConfig;

  /* Set up dramInit1 for triple or double buffering */
  HWC_IO_LOAD(bInfo->regInfo, dramInit1, dramInit1);
  if (bInfo->vidInfo.tripleBuffering)
    dramInit1 |= SST_TRIPLE_BUFFER_EN;
  else
    dramInit1 &= ~SST_TRIPLE_BUFFER_EN;
  HWC_IO_STORE(bInfo->regInfo, dramInit1, dramInit1);

  HWC_IO_STORE(bInfo->regInfo, vidMaxRGBDelta, 0x100810);

  HWC_IO_STORE( bInfo->regInfo, vidDesktopOverlayStride,
                ( bInfo->buffInfo.bufStrideInTiles << 16 ) |
                  bInfo->buffInfo.bufStrideInTiles );

  /* Overlay Hack: setup tiledness */
  if (GETENV ("SSTH3_DESKTOP_OVERLAY"))
  {
    /* The desktop is rarely tiled, though the video...
     */
    HWC_IO_STORE( bInfo->regInfo, vidDesktopOverlayStride,
		  ( bInfo->buffInfo.bufStrideInTiles << 16 ) |
		  ( bInfo->buffInfo.bufStride ) );
    /* Hack for 3d as desktop 2d as overlay */
    HWC_IO_STORE( bInfo->regInfo, vidDesktopOverlayStride,
		  ( bInfo->buffInfo.bufStride << 16 ) |
		  ( bInfo->buffInfo.bufStrideInTiles ) );
  }

  HWC_IO_LOAD( bInfo->regInfo, vidDesktopOverlayStride, finalVidDesktopOverlayStride); 

  return FXTRUE;
}

static void repaintX(void) {
  XSetWindowAttributes xswa;
  unsigned long mask=0;
  Visual visual;
  int win;

  xswa.background_pixmap = None;
  mask |= CWBackPixmap;
  xswa.override_redirect = True;
  xswa.backing_store = NotUseful;
  xswa.save_under = False;
  mask |= (CWOverrideRedirect | CWBackingStore | CWSaveUnder);
  visual.visualid = CopyFromParent;
  win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, 
		      screenWidth, screenHeight, 0, 
		      DefaultDepth(dpy, screenNum), InputOutput, &visual, 
		      mask, &xswa);
  XMapWindow (dpy, win);
}

FxBool
hwcRestoreVideo(hwcBoardInfo *bInfo) {
  FxU32 depth;

  /* Disable FIFO */
  do {
    HWC_CAGP_LOAD(bInfo->regInfo, cmdFifo0.depth, depth);
  } while (depth);

  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize, 0);
  HWC_IO_STORE(bInfo->regInfo, lfbMemoryConfig, saveLfbMemoryConfig);
  HWC_IO_STORE(bInfo->regInfo, vidProcCfg, saveVidProcCfg);
  HWC_IO_STORE(bInfo->regInfo, miscInit0, saveMiscInit0);
  if (!dpy) return FXTRUE;

  /* Overlay Hack to get X to restore properly without using DGA direct
   * graphics. */
  if (GETENV ("SSTH3_DESKTOP_OVERLAY"))
  {  
    if (XF86VidModeSwitchToMode(dpy, DefaultScreen(dpy), vidModes[1])==False) {
      GDBG_INFO(80, "Failed to return to previous video mode\n");
    }
  }

  if (XF86VidModeSwitchToMode(dpy, DefaultScreen(dpy), vidModes[0])==False) {
    GDBG_INFO(80, "Failed to return to previous video mode\n");
  }
  if (inDGA) {
    XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
    inDGA=0;
  }
  if (vidModes) {
    XFree(vidModes);
    vidModes=0;
  }
  repaintX();
  XCloseDisplay(dpy);
  dpy=0;
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

static FxU32
pow2Round(FxU32 val, FxU32 pow2Const)
{
  const FxU32 pow2Mask = (pow2Const - 1UL);

  return ((val + pow2Mask) & ~pow2Mask);
}

static FxU32
hwcBufferLfbAddr(FxU32 bufNum, 
                 const hwcBoardInfo* bInfo,
                 FxBool colBufAlignP)
{
  FxU32 retVal = 0x00UL;

  if (bInfo->vidInfo.tiled) {
    retVal = (bInfo->fbOffset + 
              pow2Round(bufNum * bInfo->vidInfo.yRes * HWC_TILED_BUFFER_BYTES, 
                        HWC_TILED_BUFFER_Y_ALIGN) +
              (colBufAlignP ? HWC_TILED_BUFFER_X_ADJUST : 0));
  } else if (bufNum < bInfo->buffInfo.nColBuffers) {
    retVal = bInfo->buffInfo.colBuffStart[bufNum];
  } else if (bufNum == bInfo->buffInfo.nColBuffers) {
    retVal = bInfo->buffInfo.auxBuffStart;
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
  FxU32 vidProcCfg;
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

  /*
   **  On W9X/DOS, we can do this ourselves--which is much easier than
   **  mucking about with a bunch of 32-bit data in 16-bit driver
   **  code.
   */
  HWC_IO_LOAD( bInfo->regInfo, vidProcCfg, vidProcCfg);

  /* Determin which set of CLUT entries are selected */
  if (vidProcCfg & SST_OVERLAY_CLUT_SELECT)
    dacBase = 256;
  else
    dacBase = 0;

  /* Print out some useful info RE the vidProcCfg register */
  GDBG_INFO(80, "%s:  vidProcCFG(SST_OVERLAY_CLUT_SELECT) = %d\n",
            FN_NAME, (vidProcCfg & SST_OVERLAY_CLUT_SELECT) ? 1 : 0);
  GDBG_INFO(80, "%s:  vidProcCFG(SST_OVERLAY_EN) = %d\n",
            FN_NAME, (vidProcCfg & SST_OVERLAY_EN) ? 1 : 0);
  GDBG_INFO(80, "%s:  vidProcCFG(SST_OVERLAY_CLUT_BYPASS) = %d\n",
            FN_NAME, (vidProcCfg & SST_OVERLAY_CLUT_BYPASS) ? 1 : 0);
  
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
