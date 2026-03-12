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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if USE_XDGA_SWITCH
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
static Display *dpy       = 0;
static XDGADevice *dev    = 0;
static XDGAMode *vidModes = 0;
#endif

#include <minihwc.h>
#include <hwcio.h>
#include "h3cinit.h"
#include "glide.h"
#include "fxglide.h"


#define CFG_FILE "/etc/conf.3dfx/voodoo3"

#if 1
#define LOG(x)
#else
#define LOG(x) \
    do { \
	FILE *dbg = fopen("debug.txt","a"); \
	fprintf x ; \
	fclose(dbg); \
    } while(0);
#endif

static struct envitem_t {
       char *env;
       char *val;
       struct envitem_t *next;
} *first = NULL;


static FxU32 saveVidProcCfg;
static FxU32 saveLfbMemoryConfig;
static FxU32 saveMiscInit0;


FxBool setVideoMode(void *hwnd,
                    int  xRes,
                    int  yRes,
                    int  h3pixelSize,
                    int  refresh,
                    void *hmon,
                    char *devicename)
{
 GR_DCL_GC;
 hwcBoardInfo *bInfo = gc->bInfo;

#if USE_XDGA_SWITCH
 int eventbase, errorbase;
 int i, numModes;

 dpy=XOpenDisplay("");
 if (!dpy) {
    GDBG_INFO(80, "Failed to open X display\n");
    return FXFALSE;
 }

 if (XDGAQueryExtension(dpy, &eventbase, &errorbase)) {
    int major, minor;
    XDGAQueryVersion(dpy, &major, &minor);
    if (major>1 && minor<0) {
       XCloseDisplay(dpy);
       GDBG_INFO(80, "Banshee/V3 requires DGA version 1.x\n");
       return FXFALSE;
    }
 }

 vidModes = XDGAQueryModes(dpy, DefaultScreen(dpy), &numModes);
  
 for (i=0; i<numModes; i++)
     if ((vidModes[i].viewportWidth==xRes) &&
         (vidModes[i].viewportHeight==yRes))
        break;
 if (i==numModes) {
    GDBG_INFO(80, "Server doesn't support requested resolution\n");
    vidModes=0;
    return FXFALSE;
 }
 if ((dev = XDGASetMode(dpy, DefaultScreen(dpy), vidModes[i].num)) == NULL) {
    GDBG_INFO(80, "Failed to set required video mode\n");
    return FXFALSE;
 }
 XDGASync(dpy, False);
#else
 if (!h3InitSetVideoMode(bInfo->regInfo.ioPortBase, xRes, yRes, refresh, FXTRUE)) {
    GDBG_INFO(80, "Setmode failed --  unimplemented resolution\n");
    return FXFALSE;
 }
#endif

 HWC_IO_LOAD(bInfo->regInfo, vidProcCfg, saveVidProcCfg);
 HWC_IO_LOAD(bInfo->regInfo, lfbMemoryConfig, saveLfbMemoryConfig);
 HWC_IO_LOAD(bInfo->regInfo, miscInit0, saveMiscInit0);

 return FXTRUE;
} /* setVideoMode */


void resetVideo (void)
{
 GR_DCL_GC;
 hwcBoardInfo *bInfo = gc->bInfo;

 HWC_IO_STORE(bInfo->regInfo, lfbMemoryConfig, saveLfbMemoryConfig);
 HWC_IO_STORE(bInfo->regInfo, vidProcCfg, saveVidProcCfg);
 HWC_IO_STORE(bInfo->regInfo, miscInit0, saveMiscInit0);

#if USE_XDGA_SWITCH
  if (dpy) {
     if (!XDGASetMode(dpy, DefaultScreen(dpy), 0)) {
        GDBG_INFO(80, "Failed to return to previous video mode\n");
     }
     vidModes = NULL;
     XCloseDisplay(dpy);
     dpy = NULL;
     XFree(dev);
  }
#else
 /* XXX to do */
#endif

 GDBG_INFO(80, "resetVideo(): \n");
} /* resetVideo */


FxBool checkResolutions (FxBool *supportedByResolution, FxU32 stride, void *hmon)
{
  /* [dBorca] this should be tied to cinit code (or at least DGA)
   * FxU16 *h3InitFindVideoMode (FxU32 xRes, FxU32 yRes, FxU32 refresh)
   */
  int res, ref;

  for (res = GR_MIN_RESOLUTION; res <= GR_MAX_RESOLUTION; res++) {
    for (ref = GR_MIN_REFRESH; ref <= GR_MAX_REFRESH; ref++) {
      supportedByResolution[res * stride + ref] = FXTRUE;
    }
  }

  return FXTRUE;
}


static void loadEnvFile (void)
{
 FILE *file;
 char data[256];
 char *env, *val;
 struct envitem_t *item;
 FxBool sawError = FXFALSE;
 static FxBool envinit = FXFALSE;

 if (envinit) {
    return;
 }
 envinit = FXTRUE;
 if ((file = fopen(CFG_FILE, "r")) == NULL) {
    return;
 }
 while (fgets(data, 255, file) != NULL) {
       if ((*data=='#') || (*data=='\n')) {
          continue;
       }
       if ((val = strchr(data, '=')) == NULL) {
          if (!sawError) {
             sawError = FXTRUE;
             fprintf(stderr, "In config file " CFG_FILE ":\n");
          }
          fprintf(stderr, "Malformed line: %s\n", data);
          continue;
       }
       *val++ = '\0';
       env = data;
       item = malloc(sizeof(struct envitem_t));
       item->env = malloc(strlen(env)+1);
       strcpy(item->env, env);
       item->val = malloc(strlen(val)+1);
       strcpy(item->val, val);
       item->next = first;
       first = item;
 }
 fclose(file);
}


char *file_getenv (const char *a)
{
 struct envitem_t *ptr;
 char *result;

 if ((result = getenv(a)) != NULL) {
    return result;
 }
 loadEnvFile();
 for (ptr = first; ptr != NULL; ptr = ptr->next) {
     if (!strcmp(ptr->env, a)) {
        return ptr->val;
     }
 }

 return NULL;
}


#include "h3cinitdd.h"

#define CFG_READ(_chip, _offset) \
  hwcReadConfigRegister(bInfo, _chip, offsetof(SstPCIConfigRegs, _offset))

#define CFG_WRITE(_chip, _offset, _value) \
  hwcWriteConfigRegister(bInfo, _chip, offsetof(SstPCIConfigRegs, _offset), (_value))

static FxU32 memDecode[16] =
{
  128*1024*1024,
  256*1024*1024,
  512*1024*1024,
  1024*1024*1024,

  64*1024*1024,
  32*1024*1024,
  16*1024*1024,
   8*1024*1024,

   4*1024*1024,
   0,
   0,
   0,

   0,
   0,
   0,
   0
};

void 
mapSlavePhysical(hwcBoardInfo *bInfo, FxU32 chipNum)
{
  FxU32 cfgPciDecode, cmdStatus;
  FxU32 masterMemBase0, masterMemBase1, masterIOBase;
  FxU32 slaveMemBase0,  slaveMemBase1,  slaveIOBase;
  FxU32 memBase0Decode, memBase1Decode;
  FxU32 memBase0Mem, memBase1Mem;
  LOG((dbg,"mapSlavePhysical(%d)\n",chipNum));

  /* First, make sure the master has been reconfigured to only decode 32MB for memBase0,
   * 64MB for memBase1, and 256B for ioBase0 */    
  cfgPciDecode = CFG_READ(0, cfgPciDecode);

  /* Okay, first look at the master's current PCI decode for memBase0.  If it's not set to
   * 32MB then the board is still in it's powerup config and we need to fiddle with it. */
  if((cfgPciDecode & SST_PCI_MEMBASE0_DECODE) != SST_PCI_MEMBASE0_DECODE_32MB) {
    FxU32 numChips;
    LOG((dbg,"fixing PCI decode on master\n"));
    /* Now we have to try to glean how many chips we have and how much memory.  We can figure out
     * how many chips from looking at how much decode space memBase0 was strapped for. */
    memBase0Decode = (cfgPciDecode & SST_PCI_MEMBASE0_DECODE) >> SST_PCI_MEMBASE0_DECODE_SHIFT;
    memBase1Decode = (cfgPciDecode & SST_PCI_MEMBASE1_DECODE) >> SST_PCI_MEMBASE1_DECODE_SHIFT;
    memBase0Mem = memDecode[memBase0Decode];
    memBase1Mem = memDecode[memBase1Decode];

    /* Calc number of chips... */

    /* Hack... some early A1 boards are still strapped for 256MB for memBase0 */
    if(memBase0Decode == 256*1024*1024) {
      memBase0Decode = 64*1024*1024;
    }

    numChips = memBase0Decode / (32*1024*1024);
    memBase1Decode = SST_PCI_MEMBASE1_DECODE_64MB;

    /* Okay, remap master to make room for slaves. */
    cfgPciDecode &= ~(SST_PCI_MEMBASE0_DECODE | SST_PCI_MEMBASE1_DECODE | SST_PCI_IOBASE0_DECODE);
    cfgPciDecode |= (SST_PCI_MEMBASE0_DECODE_32MB | SST_PCI_IOBASE0_DECODE_256);
    cfgPciDecode |= memBase1Decode;
    CFG_WRITE(0, cfgPciDecode, cfgPciDecode);
  }

  /* Now figure out the master's physical addresses, masking off bits we don't care about. */
  masterMemBase0 = CFG_READ(0, memBaseAddr0) & ~0xf;
  masterMemBase1 = CFG_READ(0, memBaseAddr1) & ~0xf;
  masterIOBase   = CFG_READ(0, ioBaseAddr)   & ~0xf;

  LOG((dbg,"masterMemBase0: %08lx (%dMB)\n",masterMemBase0,
    memDecode[(cfgPciDecode & SST_PCI_MEMBASE0_DECODE) >> SST_PCI_MEMBASE0_DECODE_SHIFT] / 1024*1024));
  LOG((dbg,"masterMemBase1: %08lx (%dMB)\n",masterMemBase1,
    memDecode[(cfgPciDecode & SST_PCI_MEMBASE1_DECODE) >> SST_PCI_MEMBASE1_DECODE_SHIFT] / 1024*1024));
  LOG((dbg,"masterIOBase: %08lx\n",masterIOBase));

  /* Calculate how much memory space the master uses for memBase1. */
  memBase1Decode = (cfgPciDecode & SST_PCI_MEMBASE1_DECODE) >> SST_PCI_MEMBASE1_DECODE_SHIFT;
  memBase1Mem = memDecode[memBase1Decode];

  /* Calculate desired slave addresses. */
  slaveMemBase0  = masterMemBase0 + 32*1024*1024*chipNum;
  /* Note: All slaves share the same memBase1 address space. */
  slaveMemBase1  = masterMemBase1 + memBase1Mem;
  /* Note: Slaves share IO space with the master, but have I/O cycles turned off. */
  slaveIOBase    = masterIOBase; /* + 256*chipNum; */

  LOG((dbg,"slaveMemBase0: %08lx\n",slaveMemBase0));
  LOG((dbg,"slaveMemBase1: %08lx\n",slaveMemBase1));
  LOG((dbg,"slaveIOBase: %08lx\n",slaveIOBase));

  /* Enable init register writes */
  CFG_WRITE(chipNum, cfgInitEnable_FabID, 
            (SST_ENABLE_HARDWARE_INIT_WRITES |
             SST_ENABLE_PCI_FIFO_WRITES |
             SST_ENABLE_BASE_ADDR_WRITES) << 8);

  /* Make sure slaves are only going to decode what we want them to.  The slaves always decode 
   * the same amount of memory that the master does, but some of the slave memory areas overlap. */
  /* Make sure we set up the right snoop enable stuff too */
  cfgPciDecode &= ~(SST_SNOOP_MEMBASE0_DECODE | SST_SNOOP_MEMBASE1_DECODE);
  cfgPciDecode |= SST_SNOOP_MEMBASE0_DECODE_32MB;
  cfgPciDecode |= memBase1Decode << SST_SNOOP_MEMBASE1_DECODE_SHIFT;
  CFG_WRITE(chipNum, cfgPciDecode, cfgPciDecode);

  /* Program slaves for their new home. */
  CFG_WRITE(chipNum, memBaseAddr0, slaveMemBase0);
  CFG_WRITE(chipNum, memBaseAddr1, slaveMemBase1);
  CFG_WRITE(chipNum, ioBaseAddr,   slaveIOBase);

  /* Enable memory decode on slave, but keep IO disabled. */
  cmdStatus = CFG_READ(chipNum, status_command);
  cmdStatus &= ~1;
  cmdStatus |= 2;
  CFG_WRITE(chipNum, status_command, cmdStatus);

  LOG((dbg,"mapSlavePhysical(%d) done\n",chipNum));

}

/* This assumes that the slave has been mapped in already. */
void
initSlave(hwcBoardInfo *bInfo, FxU32 chipNum)
{
  FxU32 pciInit0, pllCtrl1;
  FxU32 dramInit0, dramInit1;
  FxU32 miscInit0, miscInit1;
  FxU32 tmuGbeInit;
  FxU32 cmdStatus;

  LOG((dbg,"initSlave(%d)\n",chipNum));

  /* Copy over pll & memory timings, etc. */
  HWC_IO_LOAD(bInfo->regInfo, pllCtrl1, pllCtrl1);
  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, pllCtrl1, pllCtrl1);
  HWC_IO_LOAD(bInfo->regInfo, dramInit0, dramInit0);
  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, dramInit0, dramInit0);
  HWC_IO_LOAD(bInfo->regInfo, dramInit1, dramInit1);
  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, dramInit1, dramInit1);
  HWC_IO_LOAD(bInfo->regInfo, pciInit0, pciInit0);
  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, pciInit0, pciInit0);
  HWC_IO_LOAD(bInfo->regInfo, miscInit0, miscInit0);
  miscInit0 &= ~BIT(30);
  HWC_IO_STORE(bInfo->regInfo, miscInit0, miscInit0);
  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, miscInit0, miscInit0);
  HWC_IO_LOAD(bInfo->regInfo, miscInit1, miscInit1);
  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, miscInit1, miscInit1);
  HWC_IO_LOAD(bInfo->regInfo, tmuGbeInit, tmuGbeInit);
  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, tmuGbeInit, tmuGbeInit);

  /* Init DRAM mode stuff */
  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, dramData, 0x000000037);
  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, dramCommand, 0x10d);

  /* Disable master IO */
  cmdStatus = CFG_READ(0, status_command);
  cmdStatus &= ~1;
  CFG_WRITE(0, status_command, cmdStatus);
  /* Enable slave IO */
  cmdStatus = CFG_READ(chipNum, status_command);
  cmdStatus |= 1;
  CFG_WRITE(chipNum, status_command, cmdStatus);

  /* Reset everything */
  h3InitResetAll(bInfo->regInfo.slaveIOBase[chipNum-1]);

  /* Init VGA (no legacy decode) */
  h3InitVga(bInfo->regInfo.slaveIOPortBase[chipNum-1], FXFALSE);

  /* Disable slave IO */
  cmdStatus = CFG_READ(chipNum, status_command);
  cmdStatus &= ~1;
  CFG_WRITE(chipNum, status_command, cmdStatus);
  /* Enable master IO */
  cmdStatus = CFG_READ(0, status_command);
  cmdStatus |= 1;
  CFG_WRITE(0, status_command, cmdStatus);

 {
  FxU32 status, vgaInit0, vgaInit1;
  HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, status, status);
  HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, vgaInit0, vgaInit0);
  HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, vgaInit1, vgaInit1);
  LOG((dbg,"initSlave(%d) done.  slave status: %08lx vgaInit0: %08lx vgaInit1: %08lx\n",chipNum, status, vgaInit0, vgaInit1));
 }
}

static const
FxU8 vgaattr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 
                  0x00, 0x00, 0x00, 0x00, 0x00, 
                  0x00, 0x00, 0x00, 0x00, 0x00, 
                  0x00, 0x01, 0x00, 0x0F, 0x00};

/* The standard mode table has 24 entries, but the first 3 are x, y and refresh. 
 * We don't care about those here.  The default values (0-21) are just to serve
 * as a reference for the comments... they will be blown away the first time we
 * fill out the table. */
static FxU16 modeData[21] =
{
  0,      /* CRTC (0xd4) Index 0x00 - Horizontal Total */      
  1,      /* CRTC (0xd4) Index 0x01 - Horizontal Display Enable End */
  2,      /* CRTC (0xd4) Index 0x02 - Start Horizontal Blanking */
  3,      /* CRTC (0xd4) Index 0x03 - End Horizontal Blanking */
  4,      /* CRTC (0xd4) Index 0x04 - Start Horizontal Sync */
  5,      /* CRTC (0xd4) Index 0x05 - End Horzontal Sync */
  6,      /* CRTC (0xd4) Index 0x06 - Vertical Total */
  7,      /* CRTC (0xd4) Index 0x07 - Overflow */
  8,      /* CRTC (0xd4) Index 0x09 - Maximum Scan Line */
  9,      /* CRTC (0xd4) Index 0x10 - Vertical Retrace Start */
 10,      /* CRTC (0xd4) Index 0x11 - Vertical Retrace End */
 11,      /* CRTC (0xd4) Index 0x12 - Vertical Display Enable End */
 12,      /* CRTC (0xd4) Index 0x15 - Start Vertical Blank */
 13,      /* CRTC (0xd4) Index 0x16 - End Vertical Blank */
 14,      /* CRTC (0xd4) Index 0x1a - Horizontal Extension Register */
 15,      /* CRTC (0xd4) Index 0x1b - Vertical Extension Register */
 16,      /* Miscellaneous Output (Written to port 0xc2, but read from 0xcc) */
 17,      /* Sequencer (0xc4) Index 0x01 */
 18,      /* pllCtrl0 - Low byte */
 19,      /* pllCtrl0 - High byte */
 20       /* 2X Mode */
};


#define GET_CRTC_INDEX(_srcindex, _dstindex) \
   ISET8PHYS(0x0d4, _srcindex); \
   modeData[_dstindex] = IGET8PHYS(0x0d5);

static void 
buildVideoModeData(hwcBoardInfo *bInfo)
{
  /* Snarf all VGA data we need from the master */
  FxU32 regBase, pllCtrl0;

  regBase = bInfo->regInfo.ioPortBase;

  /* CRTC values */
  GET_CRTC_INDEX(0x00, 0);
  GET_CRTC_INDEX(0x01, 1);
  GET_CRTC_INDEX(0x02, 2);
  GET_CRTC_INDEX(0x03, 3);
  GET_CRTC_INDEX(0x04, 4);
  GET_CRTC_INDEX(0x05, 5);
  GET_CRTC_INDEX(0x06, 6);
  GET_CRTC_INDEX(0x07, 7);
  GET_CRTC_INDEX(0x09, 8);
  GET_CRTC_INDEX(0x10, 9);
  GET_CRTC_INDEX(0x11, 10);
  GET_CRTC_INDEX(0x12, 11);
  GET_CRTC_INDEX(0x15, 12);
  GET_CRTC_INDEX(0x16, 13);
  GET_CRTC_INDEX(0x1a, 14);
  GET_CRTC_INDEX(0x1b, 15);
  
  /* Miscellaneous output */
  modeData[16] = IGET8PHYS(0x0cc);

  /* Sequencer */
  ISET8PHYS(0x0c4, 0x01);
  modeData[17] = IGET8PHYS(0x0c5);

  /* pllCtrl0 */
  pllCtrl0 = IGET32(pllCtrl0);
  modeData[18] = (FxU16)(pllCtrl0 & 0xFF);
  modeData[19] = (FxU16)((pllCtrl0 >> 8) & 0xFF);

  /* 2X Mode */
  modeData[20] = (FxU16)IGET32(dacMode);

  {
    FxU32 i;
    for(i = 0; i < 21; i++) {
      LOG((dbg,"modeData[%d]: %02lx\n",i,modeData[i]));
    }
  }
}

static void
setVideoModeSlave(
  FxU32 regBase)   // regBase of the slave
{
    FxU16 i;
    FxU8 garbage;
    FxU32 vidProcCfg;
    FxU16 *rs = modeData;

    //
    // MISC REGISTERS
    // This register gets programmed first since the Mono/ Color
    // selection needs to be made.
    //
    // Sync polarities
    // Also force the programmable clock to be used with bits 3&2
    //
    LOG((dbg,"setVideoModeSlave(%04lx)\n",regBase));
    LOG((dbg,"motherboard enable: %02lx\n",IGET8PHYS(0xc3)));

    ISET8PHYS(0xc2, rs[16] | BIT(0));

    //
    // CRTC REGISTERS
    // First Unlock CRTC, then program them 
    //
    
    // Mystical VGA Magic
    ISET16PHYS(0x0d4,  0x11);
    ISET16PHYS(0x0d4,  (rs[0] << 8) | 0x00);
    ISET16PHYS(0x0d4,  (rs[1] << 8) | 0x01);
    ISET16PHYS(0x0d4,  (rs[2] << 8) | 0x02);
    ISET16PHYS(0x0d4,  (rs[3] << 8) | 0x03);
    ISET16PHYS(0x0d4,  (rs[4] << 8) | 0x04);
    ISET16PHYS(0x0d4,  (rs[5] << 8) | 0x05);
    ISET16PHYS(0x0d4,  (rs[6] << 8) | 0x06);
    ISET16PHYS(0x0d4,  (rs[7] << 8) | 0x07);
    ISET16PHYS(0x0d4,  (rs[8] << 8) | 0x09);
    ISET16PHYS(0x0d4,  (rs[9] << 8) | 0x10);
    ISET16PHYS(0x0d4,  (rs[10] << 8) | 0x11);
    ISET16PHYS(0x0d4,  (rs[11] << 8) | 0x12);
    ISET16PHYS(0x0d4,  (rs[12] << 8) | 0x15);
    ISET16PHYS(0x0d4,  (rs[13] << 8) | 0x16);
    ISET16PHYS(0x0d4,  (rs[14] << 8) | 0x1a);
    ISET16PHYS(0x0d4,  (rs[15] << 8) | 0x1b);
    
    //
    // Enable Sync Outputs
    //
    ISET16PHYS(0x0d4, (0x80 << 8) | 0x17);

    //
    // VIDCLK (32 bit access only!)
    // Set the Video Clock to the correct frequency
    //

    ISET32(pllCtrl0, (rs[19] << 8) | rs[18]);

    //
    // dacMode (32 bit access only!)
    // (sets up 1x mode or 2x mode)
    //
    ISET32(dacMode,  rs[20]);

    //
    // the 1x / 2x bit must also be set in vidProcConfig to properly
    // enable 1x / 2x mode
    //
    vidProcCfg = IGET32(vidProcCfg);
    vidProcCfg &= ~(SST_VIDEO_2X_MODE_EN | SST_HALF_MODE);
    if (rs[20])
        vidProcCfg |= SST_VIDEO_2X_MODE_EN;
    ISET32(vidProcCfg, vidProcCfg);

    //
    // SEQ REGISTERS
    // set run mode in the sequencer (not reset)
    //
    // make sure bit 5 == 0 (i.e., screen on)
    //
    ISET16PHYS(0x0c4, ((rs[17] & ((FxU16) ~BIT(5))) << 8) | 0x1 );
    ISET16PHYS(0x0c4, ( 0x3 << 8) | 0x0 );

    //
    // turn off VGA's screen refresh, as this function only sets extended
    // video modes, and the VGA screen refresh eats up performance
    // (10% difference in screen to screen blits!).   This code is not in
    // the perl, but should stay here unless specifically decided otherwise
    //
    ISET32(vgaInit0, IGET32(vgaInit0)|BIT(12) );

    //
    // Make sure attribute index register is initialized
    //
    garbage = IGET8PHYS(0x0da); // return value not used

    for (i = 0; i <= 19; i++)
    {
        ISET8PHYS(0xc0, i);
        ISET8PHYS(0xc0, vgaattr[i]);
    }

    ISET8PHYS(0xc0, 0x34);

    ISET8PHYS(0xda, 0);

    LOG((dbg,"setVideoModeSlave(%04lx) done\n",regBase));

} // h3InitSetVideoMode

/* This code is mostly based on the Win9x sliaa.c code. */
void hwcSetSLIAAMode(hwcBoardInfo *bInfo,
                     FxU32 sliEnable,
                     FxU32 aaEnable,
                     FxU32 analogSLI,
                     FxU32 sliBandHeight,
                     FxU32 totalMem,
                     FxU32 numChips,
                     FxU32 aaSampleHigh,
                     FxU32 aaColorBuffStart,
                     FxU32 aaDepthBuffStart,
                     FxU32 aaDepthBuffEnd,
                     FxU32 bpp)
{
  FxU32 chipNum, temp;
  FxU32 numChipsLog2;
  FxU32 memBase0, memBase1;
  FxU32 sliBandHeightLog2, sliRenderMask, sliCompareMask, sliScanMask;
  FxU32 cmdStatus;
  FxU32 vid2xMode;

  LOG((dbg,"hwcSetSLIAAMode() begin\n"));

  /* Enable AA and/or SLI */
  if(sliEnable || aaEnable) {
    
    /* First, init all chips */
    for(chipNum = 1; chipNum < numChips; chipNum++) {
      initSlave(bInfo, chipNum);
    }

    /* Grab video mode data from master. */
    buildVideoModeData(bInfo);

    /* Now set up video modes */
    for(chipNum = 1; chipNum < numChips; chipNum++) {
        
      /* Disable master IO */
      cmdStatus = CFG_READ(0, status_command);
      cmdStatus &= ~1;
      CFG_WRITE(0, status_command, cmdStatus);
      /* Enable slave IO */
      cmdStatus = CFG_READ(chipNum, status_command);
      cmdStatus |= 1;
      CFG_WRITE(chipNum, status_command, cmdStatus);
      
      setVideoModeSlave(bInfo->regInfo.slaveIOPortBase[chipNum-1]);

      /* Disable slave IO */
      cmdStatus = CFG_READ(chipNum, status_command);
      cmdStatus &= ~1;
      CFG_WRITE(chipNum, status_command, cmdStatus);
      /* Enable master IO */
      cmdStatus = CFG_READ(0, status_command);
      cmdStatus |= 1;
      CFG_WRITE(0, status_command, cmdStatus);
      
      /* Copy over video processor config registers */
      HWC_IO_LOAD(bInfo->regInfo, vidScreenSize, temp);
      HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidScreenSize, temp);

      /* Make sure video processor is reset so the size change takes effect. */
      HWC_IO_LOAD(bInfo->regInfo, vidProcCfg, temp);
      HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidProcCfg, temp & ~1);
      HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidProcCfg, temp | 1);

      /* Note: We don't set any start addresses here because it's done as part 
       * of the 3D init stuff... */
      HWC_IO_LOAD(bInfo->regInfo, vidOverlayStartCoords, temp);
      HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidOverlayStartCoords, temp);
      
      HWC_IO_LOAD(bInfo->regInfo, vidOverlayEndScreenCoord, temp);
      HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidOverlayEndScreenCoord, temp);
      
      HWC_IO_LOAD(bInfo->regInfo, vidOverlayDudxOffsetSrcWidth, temp);
      HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidOverlayDudxOffsetSrcWidth, temp);
      
      HWC_IO_LOAD(bInfo->regInfo, vidDesktopOverlayStride, temp);
      HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidDesktopOverlayStride, temp);
      
      HWC_IO_LOAD(bInfo->regInfo, vidOverlayDudx, temp);
      HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidOverlayDudx, temp);

      HWC_IO_LOAD(bInfo->regInfo, vidMaxRGBDelta, temp);
      HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidMaxRGBDelta, temp);
    }

    /* Calculate Log2 of the SLI band height */
    switch (sliBandHeight) {
      case  2: sliBandHeightLog2 = 1;
               break;
      case  4: sliBandHeightLog2 = 2;
               break;
      case  8: sliBandHeightLog2 = 3;
               break;
      case 16: sliBandHeightLog2 = 4;
               break;
      case 32: sliBandHeightLog2 = 5;
               break;
      case 64: sliBandHeightLog2 = 6;
               break;
      case 128: sliBandHeightLog2 = 7;
                break;
    }

    LOG((dbg,"sliBandHeight: %d  log2: %d\n",sliBandHeight, sliBandHeightLog2));

    /* Calculate Log2 of the number of chips */
    switch(numChips) {
      case 1: numChipsLog2 = 0;
              break;
      case 2: numChipsLog2 = 1;
              break;
      case 4: numChipsLog2 = 2;
              break;
      case 8: numChipsLog2 = 3;
              break;
    }

    LOG((dbg,"numChips: %d  log2: %d\n",numChips, numChipsLog2));

    memBase0 = bInfo->pciInfo.pciBaseAddr[0] & ~0xf;
    memBase1 = bInfo->pciInfo.pciBaseAddr[1] & ~0xf;

    LOG((dbg,"memBase0: %08lx\n",memBase0));
    LOG((dbg,"memBase1: %08lx\n",memBase1));

    /* Now on to the MUCH nastier backend stuff... */
    for(chipNum = 0; chipNum < numChips; chipNum++) {
      /* Is 2x video mode enabled? */
      HWC_IO_LOAD(bInfo->regInfo, vidProcCfg, temp);
      vid2xMode = (temp & SST_VIDEO_2X_MODE_EN);
      
      /* Set up pciInit0 and tmuGbeInit */
      if(chipNum == 0) {
        HWC_IO_LOAD(bInfo->regInfo, pciInit0, temp);
        temp = (temp & ~(SST_PCI_RETRY_INTERVAL|SST_PCI_FORCE_FB_HIGH)) |
                SST_PCI_READ_WS | SST_PCI_WRITE_WS;
        HWC_IO_STORE(bInfo->regInfo, pciInit0, temp);

        LOG((dbg,"master pciInit0: %08lx\n",temp));
        HWC_IO_LOAD(bInfo->regInfo, tmuGbeInit, temp);
        temp = (temp & ~(SST_AA_CLK_DELAY | SST_AA_CLK_INVERT)) |
                (0x02 << SST_AA_CLK_DELAY_SHIFT) | SST_AA_CLK_INVERT;
        HWC_IO_STORE(bInfo->regInfo, tmuGbeInit, temp);
     
        LOG((dbg,"master tmuGbeInit: %08lx\n",temp));
      
      /* Dump out any other registers we want to look at... */
        HWC_IO_LOAD(bInfo->regInfo, miscInit0, temp);
        LOG((dbg,"master miscInit0: %08lx\n",temp));
        HWC_IO_LOAD(bInfo->regInfo, miscInit1, temp);
        LOG((dbg,"master miscInit1: %08lx\n",temp));
      
      } else {
        HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, pciInit0, temp);
        temp = (temp & ~(SST_PCI_RETRY_INTERVAL|SST_PCI_FORCE_FB_HIGH)) |
               SST_PCI_READ_WS | SST_PCI_WRITE_WS;
        HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, pciInit0, temp);
        LOG((dbg,"slave pciInit0: %08lx\n",temp));
      
        HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, tmuGbeInit, temp);
        temp = (temp & ~(SST_AA_CLK_DELAY | SST_AA_CLK_INVERT)) |
                (0x02 << SST_AA_CLK_DELAY_SHIFT) | SST_AA_CLK_INVERT;
        HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, tmuGbeInit, temp);
        LOG((dbg,"slave tmuGbeInit: %08lx\n",temp));

        HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, miscInit0, temp);
        LOG((dbg,"slave miscInit0: %08lx\n",temp));
        HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, miscInit1, temp);
        LOG((dbg,"slave miscInit1: %08lx\n",temp));
      }

      /* Set up buffer swap control and snooping */
      if(numChips > 1) {
        /* Buffer swapping */
        temp = CFG_READ(chipNum, cfgInitEnable_FabID) >> 8;
          LOG((dbg,"cfgInitEnable rd0: %08lx\n",temp << 8));
        temp &= ~SST_SWAP_MASTER;
        temp |= (SST_SWAPBUFFER_ALGORITHM | (chipNum == 0 ? SST_SWAP_MASTER : 0));
          LOG((dbg,"cfgInitEnable wr0: %08lx\n",temp << 8));
        CFG_WRITE(chipNum, cfgInitEnable_FabID, temp << 8);

        /* Enable snooping */
        if(chipNum == 0) {
          temp = CFG_READ(chipNum, cfgInitEnable_FabID) >> 8;
          temp |= SST_ADDRESS_SNOOP_ENABLE;
          CFG_WRITE(chipNum, cfgInitEnable_FabID, temp << 8);
        } else {
          temp = CFG_READ(chipNum, cfgInitEnable_FabID) >> 8;
          LOG((dbg,"cfgInitEnable rd1: %08lx\n",temp << 8));
          temp &= ~SST_MEMBASE0_SNOOP;
          LOG((dbg,"Snoop cleared: %08lx\n",temp << 8));
          temp |= (SST_ADDRESS_SNOOP_ENABLE | SST_MEMBASE0_SNOOP_ENABLE | SST_MEMBASE1_SNOOP_ENABLE | SST_ADDRESS_SNOOP_SLAVE |
                   SST_INIT_REGISTER_SNOOP_ENABLE |
                   (((memBase0>>22) & 0x3ff) << SST_MEMBASE0_SNOOP_SHIFT) |
                   ((numChips > 2) ? SST_QUICK_SAMPLING : 0x0));
          LOG((dbg,"cfgInitEnable wr1: %08lx\n",temp << 8));
          CFG_WRITE(chipNum, cfgInitEnable_FabID, temp << 8);

          temp = CFG_READ(chipNum, cfgPciDecode);
          LOG((dbg,"cfgPciDecode rd: %08lx\n",temp));
          temp &= ~SST_MEMBASE1_SNOOP;
          temp |= ((memBase1 >> 22) & 0x3ff) << SST_MEMBASE1_SNOOP_SHIFT;
          LOG((dbg,"cfgPciDecode wr: %08lx\n",temp));
          CFG_WRITE(chipNum, cfgPciDecode, temp);
        }
      }

      /* cfgSliLfbCtrl */
      if(sliEnable && (!aaEnable || !aaSampleHigh)) {
        if(numChips == 4 && aaEnable && !aaSampleHigh && analogSLI) {
          /* 4 chip 2 sample AA. two 2-way SLI units with 1 subsample per SLI unit */
          sliRenderMask = ((numChips >> 1) - 1) << sliBandHeightLog2;
          sliCompareMask = (chipNum >> 1) << sliBandHeightLog2;
          sliScanMask = sliBandHeight - 1;

          temp = (sliRenderMask << SST_SLI_LFB_RENDERMASK_SHIFT) |
                  (sliCompareMask << SST_SLI_LFB_COMPAREMASK_SHIFT) |
                  (sliScanMask << SST_SLI_LFB_SCANMASK_SHIFT) |
                  ((numChipsLog2-1) << SST_SLI_LFB_NUMCHIPS_LOG2_SHIFT) |
                  SST_SLI_LFB_CPU_WRITE_ENABLE | 
                  SST_SLI_LFB_DISPATCH_WRITE_ENABLE |
                  SST_SLI_LFB_READ_ENABLE;
          CFG_WRITE(chipNum, cfgSliLfbCtrl, temp);
        } else {
          /* No aa or 2 sample AA w/SLI */
          sliRenderMask = (numChips - 1) << sliBandHeightLog2;
          sliCompareMask = chipNum << sliBandHeightLog2;
          sliScanMask = sliBandHeight - 1;

          temp = (sliRenderMask << SST_SLI_LFB_RENDERMASK_SHIFT) |
                  (sliCompareMask << SST_SLI_LFB_COMPAREMASK_SHIFT) |
                  (sliScanMask << SST_SLI_LFB_SCANMASK_SHIFT) |
                  (numChipsLog2 << SST_SLI_LFB_NUMCHIPS_LOG2_SHIFT) |
                  SST_SLI_LFB_CPU_WRITE_ENABLE | 
                  SST_SLI_LFB_DISPATCH_WRITE_ENABLE |
                  SST_SLI_LFB_READ_ENABLE;
          CFG_WRITE(chipNum, cfgSliLfbCtrl, temp);
        }
      } else if(!sliEnable && aaEnable) {
        /* SLI disabled, AA enabled */
        CFG_WRITE(chipNum, cfgSliLfbCtrl, 0);
      } else {
        /* SLI enabled, AA enabled, 4 sample AA enabled */
        sliRenderMask = ((numChips >> 1) - 1) << sliBandHeightLog2;
        sliCompareMask = (chipNum >> 1) << sliBandHeightLog2;
        sliScanMask = sliBandHeight - 1;

        temp = (sliRenderMask << SST_SLI_LFB_RENDERMASK_SHIFT) |
                (sliCompareMask << SST_SLI_LFB_COMPAREMASK_SHIFT) |
                (sliScanMask << SST_SLI_LFB_SCANMASK_SHIFT) |
                ((numChipsLog2-1) << SST_SLI_LFB_NUMCHIPS_LOG2_SHIFT) |
                SST_SLI_LFB_CPU_WRITE_ENABLE | 
                SST_SLI_LFB_DISPATCH_WRITE_ENABLE |
                SST_SLI_LFB_READ_ENABLE;
        CFG_WRITE(chipNum, cfgSliLfbCtrl, temp);
      }

      /* cfgSliAATiledAperture */
      if(sliEnable && !aaEnable) {
        /* Do nothing */
      } else {
        /* AA is enabled */
        FxU32 format;
        switch(bpp) {
          case 15: format = SST_AA_LFB_READ_FORMAT_15BPP;
                    break;
          case 16: format = SST_AA_LFB_READ_FORMAT_16BPP;
                    break;
          case 32: format = SST_AA_LFB_READ_FORMAT_32BPP;
                    break;
        }

        temp = (aaColorBuffStart << SST_SECONDARY_BUFFER_BASE_SHIFT) |
                SST_AA_LFB_CPU_WRITE_ENABLE | 
                SST_AA_LFB_DISPATCH_WRITE_ENABLE |
                format |
                (aaSampleHigh ? SST_AA_LFB_RD_DIVIDE_BY_FOUR : 0x0);
        CFG_WRITE(chipNum, cfgAALfbCtrl, temp);

        temp = ((aaDepthBuffStart >> 12) << SST_AA_DEPTH_BUFFER_APERTURE_BEGIN_SHIFT) |
                ((aaDepthBuffEnd >> 12) << SST_AA_DEPTH_BUFFER_APERTURE_END_SHIFT);
        CFG_WRITE(chipNum, cfgAADepthBufferAperture, temp);
      }
       
      /* Set up vga_vsync_offset field in cfgSliAAMisc */
      if((numChips > 1) && (chipNum > 0) && (aaEnable || sliEnable)) {
        FxU32 vsyncOffsetPixels, vsyncOffsetChars, vsyncOffsetHXtra;

        if((analogSLI &&
            !((numChips == 4) && sliEnable && aaEnable && !aaSampleHigh && analogSLI && (chipNum != 2)) &&
			!((numChips == 4) && !sliEnable && aaEnable && aaSampleHigh && analogSLI && (chipNum != 2) && (vid2xMode == 0))) ||
			((numChips == 4) && sliEnable && aaEnable && aaSampleHigh && !analogSLI && (chipNum == 2))) {

          /* Handle four chips, 2-way analog SLI with digital 4-sample AA...*/
          vsyncOffsetPixels = 7;
          vsyncOffsetChars = 4;
          vsyncOffsetHXtra = 0;
        } else {
          /* Run slave 8 clocks ahead */
          vsyncOffsetPixels = 7;
          vsyncOffsetChars = 5;
          vsyncOffsetHXtra = 0;
        }

        temp = CFG_READ(chipNum, cfgSliAAMisc);
        temp &= ~SST_VGA_VSYNC_OFFSET;
        temp |= (vsyncOffsetPixels << SST_VGA_VSYNC_OFFSET_PIXELS_SHIFT) |
                (vsyncOffsetChars << SST_VGA_VSYNC_OFFSET_CHARS_SHIFT) |
                (vsyncOffsetHXtra << SST_VGA_VSYNC_OFFSET_HXTRA_SHIFT);
        CFG_WRITE(chipNum, cfgSliAAMisc, temp);
      }

/* Macros to save my effing fingers */
#define CFG_VIDEOCTRL0(flags, trueShift, falseShift) \
  temp = (flags) |  \
         ((trueShift) << SST_CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | \
         ((falseShift) << SST_CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT); \
         CFG_WRITE(chipNum, cfgVideoCtrl0, temp);
#define CFG_VIDEOCTRL1(renderFetch, compareFetch, renderCRT, compareCRT) \
  temp = ((renderFetch) << SST_CFG_SLI_RENDERMASK_FETCH_SHIFT) | \
         ((compareFetch) << SST_CFG_SLI_COMPAREMASK_FETCH_SHIFT) | \
         ((renderCRT) << SST_CFG_SLI_RENDERMASK_CRT_SHIFT) | \
         ((compareCRT) << SST_CFG_SLI_COMPAREMASK_CRT_SHIFT); \
         CFG_WRITE(chipNum, cfgVideoCtrl1, temp);
#define CFG_VIDEOCTRL2(renderMask, compareMask) \
  temp = ((renderMask) << SST_CFG_SLI_RENDERMASK_AAFIFO_SHIFT) | \
         ((compareMask) << SST_CFG_SLI_COMPAREMASK_AAFIFO_SHIFT); \
         CFG_WRITE(chipNum, cfgVideoCtrl2, temp);

      if(numChips == 1 && aaEnable) {
        /* Single chip, 2-sample AA */
        CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                       SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                       SST_CFG_DIVIDE_VIDEO_BY_2,
                       0,
                       SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
        CFG_VIDEOCTRL1(0x00,
                       0x00,
                       0x00,
                       0x00);
        CFG_VIDEOCTRL2(0x00, 0xff);
      } else if(numChips == 2 && !sliEnable && aaEnable && aaSampleHigh && !analogSLI) {
        /* Two chips, 4-sample digital AA... */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                         SST_CFG_DIVIDE_VIDEO_BY_4,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO,
                         0);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0x00);
          CFG_VIDEOCTRL2(0x00, 0x00);
        } else {
          /* Second chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV |
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         0);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0xff);
          CFG_VIDEOCTRL2(0x00, 0x00);
        }
      } else if(numChips == 2 && !sliEnable && aaEnable && aaSampleHigh && analogSLI) {
        /* Two chips, 4-sample analog AA... */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                         SST_CFG_DIVIDE_VIDEO_BY_4,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         0);
        } else {
          /* Second chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV |
                         SST_CFG_DAC_HSYNC_TRISTATE |
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                         SST_CFG_DIVIDE_VIDEO_BY_4,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         0);
        }
        CFG_VIDEOCTRL1(0x00,
                       0x00,
                       0x00,
                       0x00);
        CFG_VIDEOCTRL2(0x00, 0x00);
      } else if(numChips == 2 && sliEnable && !aaEnable /*[dBorca]: && !aaSampleHigh*/ && !analogSLI) {
        /* Two chips, 2-way digital SLI */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_AAFIFO,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                         0x00,
                         0x00,
                         0x00);
          CFG_VIDEOCTRL2(0x01 <<sliBandHeightLog2, 0x01 <<sliBandHeightLog2);
        } else {
          /* Second chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV |
                         /*[dBorca]: SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |*/
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1((numChips - 1) << sliBandHeightLog2,
                         chipNum << sliBandHeightLog2,
                         0x00,
                         0xff);
          CFG_VIDEOCTRL2((numChips - 1) << sliBandHeightLog2, chipNum << sliBandHeightLog2);
        }
      } else if((numChips == 2 || numChips == 4) && sliEnable && !aaEnable /*[dBorca]: && !aaSampleHigh*/ && analogSLI) {
        /* 2 or 4 chips, 2/4-way analog SLI */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1((numChips - 1) << sliBandHeightLog2,
                         0x00,
                         (numChips - 1) << sliBandHeightLog2,
                         0x00);
          CFG_VIDEOCTRL2(0x00, 0xff);
        } else {
          /* Remaining chips */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV |
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1((numChips - 1) << sliBandHeightLog2,
                         chipNum << sliBandHeightLog2,
                         (numChips - 1) << sliBandHeightLog2,
                         chipNum << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0xff);
        }
      } else if(numChips == 2 && sliEnable && aaEnable && !aaSampleHigh && !analogSLI) {
        /* Two chips, 2-sample AA with 2-way digital SLI... */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                         SST_CFG_DIVIDE_VIDEO_BY_2,
                         SST_CFG_VIDEO_OTHERMUX_SEL_AAFIFO,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                         0x00,
                         0x00,
                         0x00);
          CFG_VIDEOCTRL2(0x01 << sliBandHeightLog2, 0x01 << sliBandHeightLog2);
        } else {
          /* Second chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV |
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1((numChips - 1) << sliBandHeightLog2,
                         chipNum  << sliBandHeightLog2,
                         0x00,
                         0xff);
          CFG_VIDEOCTRL2((numChips - 1) << sliBandHeightLog2, chipNum  << sliBandHeightLog2);
        }
      } else if(numChips == 2 && sliEnable && aaEnable && !aaSampleHigh && analogSLI) {
        /* Two chips, 2-sample AA with 2-way analog SLI... */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                         SST_CFG_DIVIDE_VIDEO_BY_2,
                         0,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                         0x00,
                         0x01 << sliBandHeightLog2,
                         0x00);
          CFG_VIDEOCTRL2(0x00, 0xff);
        } else {
          /* Second chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV |
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                         SST_CFG_DIVIDE_VIDEO_BY_2,
                         0,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1((numChips - 1) << sliBandHeightLog2,
                         chipNum  << sliBandHeightLog2,
                         (numChips - 1) << sliBandHeightLog2,
                         chipNum  << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0xff);
        }
      } else if(numChips == 4 && sliEnable && aaEnable && !aaSampleHigh && analogSLI) {
        /* Four chip, 2-sample AA. two units of 2 chip analog SLI, with 1 subsample per unit. */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_DIVIDE_VIDEO_BY_2,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO,
                         0);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                         0x00 << sliBandHeightLog2,
                         0x01 << sliBandHeightLog2,
                         0x00 << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0x00);
        } else if(chipNum == 1 || chipNum == 3) {
          /* Second and fourth chips */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV | 
                         SST_CFG_DAC_HSYNC_TRISTATE | 
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         0);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                         (chipNum >> 1) << sliBandHeightLog2,
                         0x00 << sliBandHeightLog2,
                         0xFF << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0x00);
        } else {
          /* Third chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV | 
                         SST_CFG_DIVIDE_VIDEO_BY_2,
                         0,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                         0x01 << sliBandHeightLog2,
                         0x01 << sliBandHeightLog2,
                         0x01 << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0xff);
        }
      } else if(numChips == 4 && sliEnable && aaEnable && aaSampleHigh == 1 && analogSLI) {
        /* Four chip, 4-sample AA. 1 subsample per chip analog SLI'ed */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_DIVIDE_VIDEO_BY_4,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO,
                         0);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0x00);
          if(vid2xMode) {
            CFG_VIDEOCTRL2(0x00, 0xff);
          } else {
            CFG_VIDEOCTRL2(0x00, 0x00);
          }
        } else if(chipNum == 1 || chipNum == 3) {
          /* Second and fourth chips */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV | 
                         SST_CFG_DAC_HSYNC_TRISTATE | 
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         0);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0xff);
          if(vid2xMode) {
            CFG_VIDEOCTRL2(0x00, 0xff);
          } else {
            CFG_VIDEOCTRL2(0x00, 0x00);
          }
        } else {
          /* Third chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV | 
                         SST_CFG_DAC_HSYNC_TRISTATE | 
                         SST_CFG_DIVIDE_VIDEO_BY_4,
                         0,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0x00);
          CFG_VIDEOCTRL2(0x00, 0xff);
        }
      } else if(numChips == 4 && !sliEnable && aaEnable && aaSampleHigh == 2 && analogSLI) {
        /* Four chip, 8-sample AA. 2 subsample per chip analog SLI'ed */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | 
                         SST_CFG_DIVIDE_VIDEO_BY_8,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO,
                         0);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0x00);
          if(vid2xMode) {
            CFG_VIDEOCTRL2(0x00, 0xff);
          } else {
            CFG_VIDEOCTRL2(0x00, 0x00);
          }
        } else if(chipNum == 1 || chipNum == 3) {
          /* Second and fourth chips */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV | 
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | 
                         SST_CFG_DAC_HSYNC_TRISTATE | 
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         0);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0xff);
          if(vid2xMode) {
            CFG_VIDEOCTRL2(0x00, 0xff);
          } else {
            CFG_VIDEOCTRL2(0x00, 0x00);
          }
        } else {
          /* Third chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV | 
                         SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | 
                         SST_CFG_DAC_HSYNC_TRISTATE | 
                         SST_CFG_DIVIDE_VIDEO_BY_8,
                         0,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0x00);
          CFG_VIDEOCTRL2(0x00, 0xff);
        }
      } else if(numChips == 2 && !sliEnable && aaEnable && !aaSampleHigh && !analogSLI) {
        /* Two chips, 2-sample AA. 1 subsample per chip digital SLI'ed */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_DIVIDE_VIDEO_BY_2,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO,
                         0);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0x00);
          CFG_VIDEOCTRL2(0x00, 0x00);
        } else {
          /* Second chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV |
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         0);
          CFG_VIDEOCTRL1(0x00,
                         0x00,
                         0x00,
                         0xff);
          CFG_VIDEOCTRL2(0x00, 0x00);
        }
      } else if(numChips == 2 && !sliEnable && aaEnable && !aaSampleHigh && analogSLI) {
        /* Two chips, 2-sample AA. 1 subsample per chip analog SLI'ed */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_DIVIDE_VIDEO_BY_2,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         0);
        } else {
          /* Second chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV |
                         SST_CFG_DAC_HSYNC_TRISTATE |
                         SST_CFG_DIVIDE_VIDEO_BY_2,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         0);
        }
        CFG_VIDEOCTRL1(0x00,
                       0x00,
                       0x00,
                       0x00);
        CFG_VIDEOCTRL2(0x00, 0x00);
      } else if(numChips == 4 && sliEnable && !aaEnable && !analogSLI) {
        /* Four chips, 4-way digital SLI... */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_SLI_AAFIFO_COMPARE_INV |
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_AAFIFO,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1((numChips - 1) << sliBandHeightLog2,
                         0x00,
                         0x00,
                         0x00);
          CFG_VIDEOCTRL2((numChips - 1) << sliBandHeightLog2, 0x00 << sliBandHeightLog2);
        } else {
          /* Remaining chips */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                         SST_CFG_ENHANCED_VIDEO_SLV |
                         SST_CFG_DIVIDE_VIDEO_BY_1,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                         SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1((numChips - 1) << sliBandHeightLog2,
                         chipNum << sliBandHeightLog2,
                         0x00,
                         0xff);
          CFG_VIDEOCTRL2((numChips - 1) << sliBandHeightLog2, chipNum << sliBandHeightLog2);
        }

      } else if(numChips == 4 && sliEnable && aaEnable && !aaSampleHigh && !analogSLI) {
        /* Four chips, 2-sample AA with 4-way digital SLI */
        if(chipNum == 0) {
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN | 
                          SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                          SST_CFG_SLI_AAFIFO_COMPARE_INV |
                          SST_CFG_DIVIDE_VIDEO_BY_2,
                          SST_CFG_VIDEO_OTHERMUX_SEL_AAFIFO,
                          SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1((numChips - 1) << sliBandHeightLog2,
                        0x00,
                        0x00,
                        0x00);
          CFG_VIDEOCTRL2((numChips - 1) << sliBandHeightLog2,
                        0x00);
        } else {
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN |
                          SST_CFG_ENHANCED_VIDEO_SLV |
                          SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                          SST_CFG_DIVIDE_VIDEO_BY_1,
                          SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                          SST_CFG_VIDEO_OTHERMUX_SEL_PIPE);
          CFG_VIDEOCTRL1((numChips - 1) << sliBandHeightLog2,
                          chipNum << sliBandHeightLog2,
                          0x00,
                          0xff);
          CFG_VIDEOCTRL2((numChips - 1) << sliBandHeightLog2,
                          chipNum << sliBandHeightLog2);
        }
      } else if(numChips == 4 && sliEnable && aaEnable && aaSampleHigh == 1 && !analogSLI) {
        /* Four chip, 2-way analog SLI with digital 4-sample AA */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN |
                          SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                          SST_CFG_DIVIDE_VIDEO_BY_4,
                          SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO,
                          0);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                          0x00 << sliBandHeightLog2,
                          0x01 << sliBandHeightLog2,
                          0x00 << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0x00);
        } else if(chipNum == 1 || chipNum == 3) {
          /* Second and fourth chips */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN |
                          SST_CFG_ENHANCED_VIDEO_SLV |
                          SST_CFG_DAC_HSYNC_TRISTATE |
                          SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                          SST_CFG_DIVIDE_VIDEO_BY_1,
                          SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                          0);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                          (chipNum >> 1) << sliBandHeightLog2,
                          0x00 << sliBandHeightLog2,
                          0xFF << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0x00);
        } else {
          /* Third chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN |
                          SST_CFG_ENHANCED_VIDEO_SLV |
                          SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                          SST_CFG_DIVIDE_VIDEO_BY_4,
                          0,
                          SST_CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                          0x01 << sliBandHeightLog2,
                          0x01 << sliBandHeightLog2,
                          0x01 << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0xff);
        }
      } else if(numChips == 4 && sliEnable && aaEnable && aaSampleHigh == 1 && analogSLI) {
        /* Four chip, 2-way analog SLI with analog 4-sample AA */
        if(chipNum == 0) {
          /* First chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN |
                          SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                          SST_CFG_DIVIDE_VIDEO_BY_4,
                          SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                          0);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                          0x00 << sliBandHeightLog2,
                          0x01 << sliBandHeightLog2,
                          0x00 << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0x00);
        } else if(chipNum == 1 || chipNum == 3) {
          /* Second and fourth chips */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN |
                          SST_CFG_ENHANCED_VIDEO_SLV |
                          SST_CFG_DAC_HSYNC_TRISTATE |
                          SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                          SST_CFG_DIVIDE_VIDEO_BY_4,
                          SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                          0);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                          (chipNum >> 1) << sliBandHeightLog2,
                          0x01 << sliBandHeightLog2,
                          (chipNum >> 1) << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0x00);
        } else {
          /* Third chip */
          CFG_VIDEOCTRL0(SST_CFG_ENHANCED_VIDEO_EN |
                          SST_CFG_ENHANCED_VIDEO_SLV |
                          SST_CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
                          SST_CFG_DIVIDE_VIDEO_BY_4,
                          SST_CFG_VIDEO_OTHERMUX_SEL_PIPE,
                          0);
          CFG_VIDEOCTRL1(0x01 << sliBandHeightLog2,
                          0x01 << sliBandHeightLog2,
                          0x01 << sliBandHeightLog2,
                          0x01 << sliBandHeightLog2);
          CFG_VIDEOCTRL2(0x00, 0x00);
        }
      }

      /* Make sure that last chip properly waits for data to be xfered
        * over the PCI bus before driving... */
      if((numChips == 4 && sliEnable && aaEnable && aaSampleHigh && chipNum == 3) ||
         (numChips == 4 && sliEnable && aaEnable && !aaSampleHigh && analogSLI && chipNum == 3)) {
        temp = CFG_READ(chipNum, cfgSliAAMisc);
        temp |= SST_CFG_AA_LFB_RD_SLV_WAIT;
        CFG_WRITE(chipNum, cfgSliAAMisc, temp);
      }

      /* Deal with the problem for LFB reads where the data really needs to
         come from 4 different chips. Since the hardware does not support
         this, we figure out a way to only return aliased data back back...
         This is accomplished by having the Master return its lfb data
         By turning off AA LFB reads, chips 2/3 no longer snoop lfb reads
         at all. Then, there is only handshaking between chips 0 & 1 so
         the Master stays happy.
       */
      if(numChips == 4 && !sliEnable && aaEnable && aaSampleHigh && analogSLI && chipNum > 1) {
        temp = CFG_READ(chipNum, cfgAALfbCtrl);
        temp &= ~SST_AA_LFB_READ_ENABLE;
        CFG_WRITE(chipNum, cfgAALfbCtrl, temp);
      }

      if(chipNum > 0) {
          /* For the slave chips, make the video PLL lock to the Master's 
          * sync_clk_out clock output... */
        temp = CFG_READ(chipNum, cfgVideoCtrl0);
        temp |= SST_CFG_VIDPLL_SEL;
        CFG_WRITE(chipNum, cfgVideoCtrl0, temp);

        /* Power down slave(s) RAMDAC */
        HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, miscInit1, temp);
        temp |= SST_POWERDOWN_DAC;
        HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, miscInit1, temp);
      }
      else if (numChips == 4) /* chipNum==0 */
      {
        /* Special Case 4 way where master also needs to sync from slave */
        temp = CFG_READ(chipNum, cfgVideoCtrl0);
        temp |= SST_CFG_VIDPLL_SEL;
        CFG_WRITE(chipNum, cfgVideoCtrl0, temp);
      }

      LOG((dbg,"cfgInitEnable: %08lx\n",CFG_READ(chipNum, cfgInitEnable_FabID)));
      LOG((dbg,"cfgPciDecode:  %08lx\n",CFG_READ(chipNum, cfgPciDecode)));
      LOG((dbg,"cfgVideoCtrl0: %08lx\n",CFG_READ(chipNum, cfgVideoCtrl0)));
      LOG((dbg,"cfgVideoCtrl1: %08lx\n",CFG_READ(chipNum, cfgVideoCtrl1)));
      LOG((dbg,"cfgVideoCtrl2: %08lx\n",CFG_READ(chipNum, cfgVideoCtrl2)));
      LOG((dbg,"cfgSliLfbCtrl: %08lx\n",CFG_READ(chipNum, cfgSliLfbCtrl)));
      LOG((dbg,"cfgAADepthAp:  %08lx\n",CFG_READ(chipNum, cfgAADepthBufferAperture)));
      LOG((dbg,"cfgAALfbCtrl:  %08lx\n",CFG_READ(chipNum, cfgAALfbCtrl)));
      LOG((dbg,"cfgSliAAMisc:  %08lx\n",CFG_READ(chipNum, cfgSliAAMisc)));
    }
  } else { /* Disable AA and/or SLI */
    
    for(chipNum = 0; chipNum < numChips; chipNum++) {
      temp = CFG_READ(chipNum, cfgInitEnable_FabID) >> 8;
      temp &= ~(SST_ADDRESS_SNOOP_ENABLE | SST_MEMBASE0_SNOOP_ENABLE | SST_MEMBASE1_SNOOP_ENABLE | SST_ADDRESS_SNOOP_SLAVE |
                SST_INIT_REGISTER_SNOOP_ENABLE | SST_MEMBASE0_SNOOP | SST_QUICK_SAMPLING | SST_SWAP_MASTER | SST_SWAPBUFFER_ALGORITHM);
      CFG_WRITE(chipNum, cfgInitEnable_FabID, temp << 8);

      CFG_WRITE(chipNum, cfgSliLfbCtrl, 0);
      CFG_WRITE(chipNum, cfgAALfbCtrl, 0);
      CFG_WRITE(chipNum, cfgSliAAMisc, 0);
      // Make sure slave chips don't drive HSYNC & VSYNC
      if(chipNum > 0) {
        CFG_WRITE(chipNum, cfgVideoCtrl0, SST_CFG_DAC_HSYNC_TRISTATE | SST_CFG_DAC_VSYNC_TRISTATE);
      } else {
        CFG_WRITE(chipNum, cfgVideoCtrl0, 0);
      }
      CFG_WRITE(chipNum, cfgVideoCtrl1, 0);
      CFG_WRITE(chipNum, cfgVideoCtrl2, 0);
      CFG_WRITE(chipNum, cfgAADepthBufferAperture, 0);

      /* Kill the DAC & video output on the slaves */
      if(chipNum > 0) {
        HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, dacMode, SST_DAC_DPMS_ON_VSYNC | SST_DAC_DPMS_ON_HSYNC);
        HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, vidProcCfg, temp);
        temp &= ~SST_VIDEO_PROCESSOR_EN;
        HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, vidProcCfg, temp);
      }
    }
  }
}
