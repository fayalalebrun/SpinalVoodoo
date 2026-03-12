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

/*
** Initialization code for SST-1 board.
**
** NOTE: This code must compiled with optimizations DISABLED!!
**
** The following environment variables can optionally be used to alter 
** functionality (A value of X is a "don't care"):
**
** Variable                    Value   Description
** --------                    -----   -----------
** SST_ARCADE                  X       Allow special configs for arcade use
** SST_BACKPORCH               hex     Specify value of backPorch video register
** SST_BOARDS                  val     Specify number of SST-1 boards in system
** SST_DEBUGDAC                X       Print out debug information for DAC
**                                     reads and writes
** SST_DIMENSIONS              hex     Specify value of videoDimensions register
** SST_FASTMEM                 1       Default. Enable fast memory timing
** SST_FASTMEM                 0       Disable fast memory timing
** SST_FASTPCIRD               X       Enable fast PCI reads
** SST_FBICFG                  hex     Specify FBI configuration strapping bits
** SST_FBIMEM_SIZE             val     Specify amount of frame buffer memory
** SST_FT_CLK_DEL              hex     Fbi-to-Trex clock delay value
** SST_GAMMA                   float   Floating point value for gamma correction
** SST_GRXCLK                  val     16 <= Frequency < 80 (50 is default)
** SST_HSYNC                   hex     Specify value of hSync video register
** SST_IGNORE_INIT_GAMMA       X       Bypass sst1InitGamma call and return
** SST_IGNORE_INIT_REGISTERS   X       Bypass sst1InitRegisters call and return
** SST_IGNORE_INIT_VIDEO       X       Bypass sst1InitVideo call and return
** SST_IGNORE_SLI_CHECK        X       Ignore checking for proper SLI install
** SST_INITDEBUG               X       Enable debug output
** SST_INITDEBUG_FILE          file    File to direct all debug output
** SST_MEMFIFO                 0       Disable FBI Memory FIFO
** SST_MEMFIFO                 1       Enable FBI Memory FIFO
** SST_MEMFIFO_ENTRIES         hex     Set number of entries in memory fifo
** SST_MEMFIFO_HWM             hex     Set memory fifo high water mark
** SST_MEMFIFO_LFB             0 or 1  Specify LFB writes through Memory FIFO
** SST_MEMFIFO_LWM             hex     Set memory fifo low water mark
** SST_MEMFIFO_TEX             0 or 1  Specify Texture writes through Mem FIFO
** SST_MEMOFFSET               hex     Specify value of memoffset video register
** SST_NOCHECKHANG             X       Do not Check for FBI Hangs for texturing
** SST_NODEVICEINFO            X       Ignore calls to sst1InitFillDeviceInfo()
** SST_NOSHUTDOWN              X       Do not turn off monitor refresh on call
**                                     to sst1InitShutdown()
** SST_PCIFIFO_LWM             hex     Set pci fifo low water mark
** SST_PFT_CLK_DEL             hex     Preliminary Fbi-to-Trex clock delay value
** SST_PTF0_CLK_DEL            hex     Prelim TMU2Fbi clock delay value (TMU #0)
** SST_PTF1_CLK_DEL            hex     Prelim TMU2Fbi clock delay value (TMU #1)
** SST_PTF2_CLK_DEL            hex     Prelim TMU2Fbi clock delay value (TMU #2)
** SST_SLIDETECT               0 or 1  Specify Scanline Interleaving detection
** SST_SLIM_VIN_CLKDEL         hex     Specify FBI internal video clock delay
**                                     (SLI Master)
** SST_SLIM_VOUT_CLKDEL        hex     Specify FBI external video clock delay
**                                     (SLI Master)
** SST_SLIS_VIN_CLKDEL         hex     Specify FBI internal video clock delay
**                                     (SLI Slave)
** SST_SLIS_VOUT_CLKDEL        hex     Specify FBI external video clock delay
**                                     (SLI Slave)
** SST_SCREENREFRESH           60      Select 60Hz monitor refresh rate
**                                     (overrides argument to sst1InitVideo)
** SST_SCREENREFRESH           75      Select 75Hz monitor refresh rate
** SST_SCREENREZ               512     Select 512x384 video resolution
**                                     (overrides argument to sst1InitVideo)
** SST_SCREENREZ               640     Select 640x480 video resolution
** SST_SCREENREZ               800     Select 800x600 video resolution
** SST_SLOWPCIWR               X       Enable 1 wait-state PCI writes
** SST_TEXMAP_DISABLE          X       Disable texture mapping
** SST_TF0_CLK_DEL             hex     Trex-to-Fbi clock delay value (TMU #0)
** SST_TF1_CLK_DEL             hex     Trex-to-Fbi clock delay value (TMU #1)
** SST_TF2_CLK_DEL             hex     Trex-to-Fbi clock delay value (TMU #2)
** SST_TF_FIFO_THRESH          hex     Set TREX-to-FBI FIFO threshold
** SST_TILESINX                hex     Specify value of tilesInX video register
** SST_TMUCFG                  hex     Specify TMU configuration strapping bits
** SST_TMUMEM                  X       Enable accurate determination of TMU mem
** SST_TMUMEM_SIZE             val     Specify amount of texture memory
** SST_TREX0INIT0              hex     Set trexInit0 register value - TMU #0
** SST_TREX0INIT1              hex     Set trexInit1 register value - TMU #0
** SST_TREX1INIT0              hex     Set trexInit0 register value - TMU #1
** SST_TREX1INIT1              hex     Set trexInit1 register value - TMU #1
** SST_TREX2INIT0              hex     Set trexInit0 register value - TMU #2
** SST_TREX2INIT1              hex     Set trexInit1 register value - TMU #2
** SST_TRIPLE_BUFFER           X       Enable Triple Buffering
** SST_VIN_CLKDEL              hex     Specify FBI internal video clock delay
** SST_VOUT_CLKDEL             hex     Specify FBI external video clock delay
** SST_VGA_PASS                0,1     Force VGA_PASS output to {0,1}
** SST_VIDCLK2X                val     Video clock frequency (2x dot clock)
** SST_VIDEO_24BPP             0       Select 16-bit video output
** SST_VIDEO_24BPP             1       Select 24-bit video output
** SST_VIDEO_DISABLE           X       Turn off Video/Monitor refresh in FBI
** SST_VIDEO_FILTER_DISABLE    X       Disable video filtering
** SST_VIDEO_FILTER_THRESHOLD  hex     Set video filtering threshold
** SST_VIDEO_NOCLEAR           X       Do not clear buffers in sst1InitVideo()
** SST_VIDEO_CLEARCOLOR        hex     Clear screen color
** SST_VFIFO_THRESH            0-31    Select video fifo threshold
** SST_VSYNC                   hex     Specify value of vSync video register
**
** VOODOO_FILE                 name    Filename used in place of "voodoo.ini"
** VOODOO_PATH                 path    Path used to locate "voodoo.ini" file
**
*/
#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */
#ifdef _MSC_VER
#pragma optimize ("",off)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sst.h>
#include <fxpci.h>
#define SST1INIT_ALLOCATE /* Allocate variables in sst1init.h */
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <sst1vid.h>
#include <sst1init.h>
#include "rcver.h"

#ifdef __WIN32__
#include <windows.h>
#endif

/*
** sst1InitMapBoard():
**  Find and map SST-1 board into virtual memory
**
**    Returns:
**      FxU32 pointer to base of SST-1 board if successful mapping occurs
**      FXFALSE if cannot map or find SST-1 board
**
*/
FX_EXPORT FxU32 * FX_CSTYLE sst1InitMapBoard(FxU32 BoardNumber)
{
    static FxU32 firstTime = 1;
    FxU32 sizeOfCard = 0x1000000;      /* 16 MBytes of addr space for SST-1 */
    FxU32 *sst;
    volatile Sstregs *sstptr;
    FxU32 n, j;

    if(firstTime) {
        /* Find "voodoo.ini" file if it exists... */
        sst1InitUseVoodooFile = sst1InitVoodooFile();

        if(!(boardsInSystem = sst1InitNumBoardsInSystem()))
            return(NULL);
    }

    if(BoardNumber == 0) {
        INIT_PRINTF(("sst1InitMapBoard(): BoardsInSystem = %d\n",
          boardsInSystem));

        /* Clear board info structure */
        sst1InitClearBoardInfo();
    }

    if(firstTime) {
        firstTime = 0;
        sst1InitClearBoardInfo();

        /* Map all boards in the system */
        /* Check that scanline interleaving is not enabled... */
        for(j=0; j<SST1INIT_MAX_BOARDS; j++) {
          FxU32 vendorID = _3DFX_PCI_ID;     /* 3Dfx Vendor ID */
          FxU32 deviceID = 1;                /* Find only sst1 boards */

#ifndef DIRECTX
            sst = pciMapCardMulti(vendorID, deviceID, sizeOfCard,
                &sst1InitDeviceNumber, j, 0);
#else
            sst = pciMapCardMulti(vendorID, deviceID, sizeOfCard, 
                &sst1InitDeviceNumber, j, 0);
#endif /* DIRECTX */
            if(sst) {
                sst1BoardInfo[j].virtAddr = sst;
                PCICFG_RD(PCI_BASE_ADDRESS_0, sst1BoardInfo[j].physAddr);
                sst1BoardInfo[j].deviceNumber = sst1InitDeviceNumber;
                sst1BoardInfo[j].numBoardInsystem = j;
                PCICFG_RD(PCI_REVISION_ID, sst1BoardInfo[j].fbiRevision);
                PCICFG_RD(PCI_DEVICE_ID, sst1BoardInfo[j].deviceID);
                PCICFG_RD(PCI_VENDOR_ID, sst1BoardInfo[j].vendorID);

                /* Enable Memory accesses to SST-1 */
                /* Must be set for PnP BIOS which do not enable memory mapped */
                /* accesses (sst1InitSliDetect reads memory mapped regs) */
                PCICFG_WR(PCI_COMMAND, SST_PCIMEM_ACCESS_EN);
                sstptr = (Sstregs *) sst;
                /* Check SLI not enabled... */
                if((IGET(sstptr->fbiInit1) & SST_EN_SCANLINE_INTERLEAVE) &&
                    sst1InitSliDetect(sst)) {
                    INIT_PRINTF(("sst1InitMapBoard(): Scanline Interleave detected at startup for board=%d\n", j));
                    INIT_PRINTF(("                    System reboot required...\n"));
                    return(NULL);
                }
            } else {
                FxU32 code = pciGetErrorCode();
                if (code == PCI_ERR_WRONGVXD) {
#ifdef __WIN32__
                    MessageBox(NULL, pciGetErrorString(), NULL, MB_OK);
#endif /* __WIN32__ */
                    exit(1);
                }
                sst1BoardInfo[j].virtAddr = (FxU32 *) NULL;
                sst1BoardInfo[j].physAddr = (FxU32) NULL;
                sst1BoardInfo[j].deviceNumber = 0xdead;
                sst1BoardInfo[j].numBoardInsystem = 0xdead;
                sst1BoardInfo[j].fbiRevision = 0xdead;
                sst1BoardInfo[j].deviceID = 0xdead;
                sst1BoardInfo[j].vendorID = 0xdead;
            }
        }
    }

    /* Max number of supported boards in system */
    if(BoardNumber > SST1INIT_MAX_BOARDS) return(NULL);

    if(!(sst = sst1BoardInfo[BoardNumber].virtAddr))
        return (NULL);

    INIT_PRINTF(("sst1InitMapBoard(): vAddr:0x%x pAddr:0x%x Dev:0x%x Board:%d\n",
        (FxU32) sst1BoardInfo[BoardNumber].virtAddr,
        (FxU32) sst1BoardInfo[BoardNumber].physAddr,
        (FxU32) sst1BoardInfo[BoardNumber].deviceNumber, BoardNumber));

    sstptr = (Sstregs *) sst;

    /* Default settings */
    if((IGET(sstptr->fbiInit3) & SST_FBI_VGA_PASS_POWERON) &&
       (sst1BoardInfo[BoardNumber].fbiRevision > 0)) {
        sst1BoardInfo[BoardNumber].vgaPassthruDisable = 0x0;
        sst1BoardInfo[BoardNumber].vgaPassthruEnable = SST_EN_VGA_PASSTHRU;
        sst1BoardInfo[BoardNumber].fbiRegulatorPresent = 0x1;
    } else {
        sst1BoardInfo[BoardNumber].vgaPassthruDisable = SST_EN_VGA_PASSTHRU;
        sst1BoardInfo[BoardNumber].vgaPassthruEnable = 0x0;
        sst1BoardInfo[BoardNumber].fbiRegulatorPresent = 0x0;
    }
    sst1BoardInfo[BoardNumber].fbiVideo16BPP = 0;

    if(GETENV(("SST_VGA_PASS"))) {
        INIT_PRINTF(("sst1InitMapBoard(): Using SST_VGA_PASS=%d\n",
            ATOI(GETENV(("SST_VGA_PASS")))));
        if(ATOI(GETENV(("SST_VGA_PASS")))) {
            sst1BoardInfo[BoardNumber].vgaPassthruEnable = SST_EN_VGA_PASSTHRU;
            sst1BoardInfo[BoardNumber].vgaPassthruDisable = 0x0;
        } else {
            sst1BoardInfo[BoardNumber].vgaPassthruDisable = SST_EN_VGA_PASSTHRU;
            sst1BoardInfo[BoardNumber].vgaPassthruEnable = 0x0;
        }
    }
    return(sst);
}

/*
** sst1InitRegisters():
**  Initialize registers and memory and return to power-on state
**
**    Returns:
**      FXTRUE if successfully initializes SST-1
**      FXFALSE if cannot initialize SST-1
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitRegisters(FxU32 *sstbase)
{
    FxU32 n, tf_fifo_thresh;
    FxU32 ft_clk_del, tf0_clk_del, tf1_clk_del, tf2_clk_del;
    sst1ClkTimingStruct sstGrxClk;
    volatile Sstregs *sst = (Sstregs *) sstbase;
    const char *envp;
    int i;

    if(!sst)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(GETENV(("SST_IGNORE_INIT_REGISTERS"))) {
        INIT_PRINTF(("WARNING: Ignoring sst1InitRegisters()...\n"));
        sst1InitIdleFBINoNOP(sstbase);
        return(FXTRUE);
    }

    /* Disable video clock */
    PCICFG_WR(SST1_PCI_VCLK_DISABLE, 0x0);

    /* Enable writes to the FBIINIT registers */
    /* Do not allow writes into the pci fifo until everything is reset */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN);

    /* Reset Snoop registers to default values */
#if 0
    PCICFG_WR(SST1_PCI_BUS_SNOOP0, SST_PCI_BUS_SNOOP_DEFAULT);
    PCICFG_WR(SST1_PCI_BUS_SNOOP1, SST_PCI_BUS_SNOOP_DEFAULT);
#else
    PCICFG_WR(SST1_PCI_BUS_SNOOP_0, SST_PCI_BUS_SNOOP_DEFAULT);
    PCICFG_WR(SST1_PCI_BUS_SNOOP_1, SST_PCI_BUS_SNOOP_DEFAULT);
#endif
    sst1InitReturnStatus(sstbase); /* Stall - can't call IdleFbi because */
                                   /* FBI could be hung at this stage */
    sst1InitReturnStatus(sstbase);
    sst1InitReturnStatus(sstbase);
    
    /* Adjust Fbi-to-Trex clock delay value */
    /* Adjust Trex-to-Fbi FIFO */
    envp = GETENV(("SST_TF_FIFO_THRESH"));
    if(!envp || (SSCANF(envp, "%i", &tf_fifo_thresh) != 1))
        tf_fifo_thresh = 0x8;
    INIT_PRINTF(("sst1InitRegisters(): Setting TREX-to-FBI FIFO THRESHOLD to 0x%x...\n",
        tf_fifo_thresh));
    /* Set ft_clk_del for 16 MHz bringup */
    if(sst1CurrentBoard->fbiRevision == 2)
        /* .5 micron */
        ft_clk_del = 0x8; /* Okay for 16 MHz startup... */
    else
        /* .6 micron */
        ft_clk_del = 0xa; /* Okay for 16 MHz startup... */
    envp = GETENV(("SST_PFT_CLK_DEL"));
    if(envp && (SSCANF(envp, "%i", &i) == 1))
      ft_clk_del = i;
    INIT_PRINTF(("sst1InitRegisters(): Setting PRELIM FT-CLK delay to 0x%x...\n", ft_clk_del));
    ISET(sst->fbiInit3,
        (SST_FBIINIT3_DEFAULT & ~(SST_FT_CLK_DEL_ADJ | SST_TF_FIFO_THRESH)) |
        (ft_clk_del << SST_FT_CLK_DEL_ADJ_SHIFT) |
        (tf_fifo_thresh << SST_TF_FIFO_THRESH_SHIFT));

    /* Wait for Fbi-to-Trex clock delay value to propogate */
    sst1InitReturnStatus(sstbase); /* Stall - can't call IdleFbi because */
                                   /* FBI could be hung at this stage */

    sst1InitReturnStatus(sstbase);
    sst1InitReturnStatus(sstbase);

    /* Reset graphics and video units */
    /* Must reset video unit before graphics unit, otherwise video unit could */
    /* potentially hang waiting for the graphics unit to respond */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);

    /* don't wait for idle b/c grx could be hung */
    sst1InitReturnStatus(sstbase);
    sst1InitReturnStatus(sstbase);
    sst1InitReturnStatus(sstbase);
    ISET(sst->fbiInit0,
      IGET(sst->fbiInit0) | (SST_GRX_RESET | SST_PCI_FIFO_RESET));

    sst1InitIdleFBINoNOP(sstbase);

    /* Unreset PCI FIFO... */
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_PCI_FIFO_RESET);
    sst1InitIdleFBINoNOP(sstbase);

    /* Unreset graphics subsystem */
    /* THIS MUST BE PRESENT OTHERWISE THE PCI FIFO WILL NEVER DRAIN! */
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_GRX_RESET);
    sst1InitIdleFBINoNOP(sstbase);

    /* Reset all FBI and TREX Init registers */
    ISET(sst->fbiInit0, SST_FBIINIT0_DEFAULT);
    ISET(sst->fbiInit1, SST_FBIINIT1_DEFAULT);
    ISET(sst->fbiInit2, SST_FBIINIT2_DEFAULT);
    ISET(sst->fbiInit3,
        (SST_FBIINIT3_DEFAULT & ~(SST_FT_CLK_DEL_ADJ | SST_TF_FIFO_THRESH)) |
        (ft_clk_del << SST_FT_CLK_DEL_ADJ_SHIFT) |
        (tf_fifo_thresh << SST_TF_FIFO_THRESH_SHIFT));
    ISET(sst->fbiInit4, SST_FBIINIT4_DEFAULT);
    sst1InitIdleFBINoNOP(sstbase);  /* Wait until init regs are reset */

    /* Enable writes to the FBIINIT registers and allow pushes onto PCI FIFO */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, (SST_INITWR_EN | SST_PCI_FIFOWR_EN));

    /* Determine DAC Type */
    if(sst1InitDacDetect(sstbase) == FXFALSE) {
        INIT_PRINTF(("sst1InitRegisters(): Could not detect DAC...\n"));
        return(FXFALSE);
    }

    /* set TREX0 init values */
    envp = GETENV(("SST_TREX0INIT0"));
    if(envp && (SSCANF(envp, "%i", &sst1CurrentBoard->tmuInit0[0]) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX0INIT0 environment variable\n"));
    } else
        sst1CurrentBoard->tmuInit0[0] = SST_TREX0INIT0_DEFAULT;

    INIT_PRINTF(("sst1InitRegisters(): Storing TREX0INIT0=0x%x\n",
        sst1CurrentBoard->tmuInit0[0]));
    envp = GETENV(("SST_TREX0INIT1"));
    if(envp && (SSCANF(envp, "%i", &sst1CurrentBoard->tmuInit1[0]) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX0INIT1 environment variable\n"));
    } else
        sst1CurrentBoard->tmuInit1[0] = SST_TREX0INIT1_DEFAULT;
    envp = GETENV(("SST_PTF0_CLK_DEL"));
    if(envp && (SSCANF(envp, "%i", &tf0_clk_del) == 1) ) {
        sst1CurrentBoard->tmuInit1[0] = (sst1CurrentBoard->tmuInit1[0] &
            ~SST_TEX_TF_CLK_DEL_ADJ) |
            (tf0_clk_del<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    }
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX0INIT1=0x%x\n",
        sst1CurrentBoard->tmuInit1[0]));

    ISET(SST_TREX(sst,0)->trexInit0, sst1CurrentBoard->tmuInit0[0]);
    sst1InitIdleFBINoNOP(sstbase);
    ISET(SST_TREX(sst,0)->trexInit1, sst1CurrentBoard->tmuInit1[0]);
    sst1InitIdleFBINoNOP(sstbase);

    /* set TREX1 init values */
    envp = GETENV(("SST_TREX1INIT0"));
    if(envp && (SSCANF(envp, "%i", &sst1CurrentBoard->tmuInit0[1]) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX1INIT0 environment variable\n"));
    } else
        sst1CurrentBoard->tmuInit0[1] = SST_TREX1INIT0_DEFAULT;
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX1INIT0=0x%x\n",
        sst1CurrentBoard->tmuInit0[1]));
    envp = GETENV(("SST_TREX1INIT1"));
    if(envp && (SSCANF(envp, "%i", &sst1CurrentBoard->tmuInit1[1]) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX1INIT1 environment variable\n"));
    } else
        sst1CurrentBoard->tmuInit1[1] = SST_TREX1INIT1_DEFAULT;
    envp = GETENV(("SST_PTF1_CLK_DEL"));
    if(envp && (SSCANF(envp, "%i", &tf1_clk_del) == 1) ) {
        sst1CurrentBoard->tmuInit1[1] = (sst1CurrentBoard->tmuInit1[1] &
            ~SST_TEX_TF_CLK_DEL_ADJ) |
            (tf1_clk_del<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    }
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX1INIT1=0x%x\n",
        sst1CurrentBoard->tmuInit1[1]));

    ISET(SST_TREX(sst,1)->trexInit0, sst1CurrentBoard->tmuInit0[1]);
    sst1InitIdleFBINoNOP(sstbase);
    ISET(SST_TREX(sst,1)->trexInit1, sst1CurrentBoard->tmuInit1[1]);
    sst1InitIdleFBINoNOP(sstbase);

    /* set TREX2 init values */
    envp = GETENV(("SST_TREX2INIT0"));
    if(envp && (SSCANF(envp, "%i", &sst1CurrentBoard->tmuInit0[2]) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX2INIT0 environment variable\n"));
    } else
        sst1CurrentBoard->tmuInit0[2] = SST_TREX2INIT0_DEFAULT;
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX2INIT0=0x%x\n",
        sst1CurrentBoard->tmuInit0[2]));
    envp = GETENV(("SST_TREX2INIT1"));
    if(envp && (SSCANF(envp, "%i", &sst1CurrentBoard->tmuInit1[2]) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX2INIT1 environment variable\n"));
    } else
        sst1CurrentBoard->tmuInit1[2] = SST_TREX2INIT1_DEFAULT;
    envp = GETENV(("SST_PTF2_CLK_DEL"));
    if(envp && (SSCANF(envp, "%i", &tf2_clk_del) == 1) ) {
        sst1CurrentBoard->tmuInit1[2] = (sst1CurrentBoard->tmuInit1[2] &
            ~SST_TEX_TF_CLK_DEL_ADJ) |
            (tf2_clk_del<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    }
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX2INIT1=0x%x\n", 
        sst1CurrentBoard->tmuInit1[2]));

    ISET(SST_TREX(sst,2)->trexInit0, sst1CurrentBoard->tmuInit0[2]);
    sst1InitIdleFBINoNOP(sstbase);
    ISET(SST_TREX(sst,2)->trexInit1, sst1CurrentBoard->tmuInit1[2]);
    sst1InitIdleFBINoNOP(sstbase);

    /* Set clock at 16 MHz to sense TREX configuration... */
    if(sst1InitComputeClkParams((float) 16.0, &sstGrxClk) == FXFALSE)
        return(FXFALSE);
    if(sst1InitSetGrxClk(sstbase, &sstGrxClk) == FXFALSE)
        return(FXFALSE);

    /* Set PCI wait-states */
    if(GETENV(("SST_SLOWPCIWR"))) {
        INIT_PRINTF(("sst1InitRegisters(): Setting up SLOW PCI Writes...\n"));
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_PCI_WRWS_1);
    } else
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) & ~SST_PCI_WRWS_1);

    if(GETENV(("SST_FASTPCIRD"))) {
        INIT_PRINTF(("sst1InitRegisters(): Setting up FAST PCI Reads...\n"));
        ISET(sst->fbiInit4, IGET(sst->fbiInit4) & ~SST_PCI_RDWS_2);
    }
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable Linear frame buffer reads */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_LFB_READ_EN);

    /* Swapbuffer algorithm is based on VSync initially */
    ISET(sst->fbiInit2, (IGET(sst->fbiInit2) & ~SST_SWAP_ALGORITHM) |
      SST_SWAP_VSYNC);

    /* Enable LFB read-aheads */
    ISET(sst->fbiInit4, IGET(sst->fbiInit4) | SST_EN_LFB_RDAHEAD);

    /* Enable triangle alternate register mapping */
    ISET(sst->fbiInit3, IGET(sst->fbiInit3) | SST_ALT_REGMAPPING);

#if 0
    /* Enable Endian Byte Swapping */
    /* Do not enable byte swapping due to potential bug in FBI... */
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) | SST_EN_ENDIAN_SWAPPING);
#endif

    /* Setup DRAM Refresh */
    ISET(sst->fbiInit2, (IGET(sst->fbiInit2) & ~SST_DRAM_REFRESH_CNTR) |
        SST_DRAM_REFRESH_16MS);
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) | SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    envp = GETENV(("SST_FASTMEM"));
    if(envp)
        n = ATOI(envp);
    else
        n = 1;

    if(n) {
        INIT_PRINTF(("sst1InitRegisters(): Setting up FAST DRAM Configuration\n"));
        ISET(sst->fbiInit2, IGET(sst->fbiInit2) | (SST_EN_FAST_RAS_READ |
          SST_EN_DRAM_RD_AHEAD_FIFO | SST_EN_FAST_RD_AHEAD_WR));
    } else
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_DISEN_RD_AHEAD_WR_RD);

    /* Return all other registers to their power-on state */
    sst1InitIdleFBINoNOP(sstbase);
    sst1InitSetResolution(sstbase, &SST_VREZ_640X480_60, 0);
    sst1InitIdleFBINoNOP(sstbase);

    /* Get DRAMs up and running (refresh is already enabled)... */
    ISET(sst->c1, 0x0);
    ISET(sst->c0, 0x0);
    ISET(sst->zaColor, 0x0);
    ISET(sst->clipLeftRight, 100); /* draw into 50x50 area */
    ISET(sst->clipBottomTop, 100);
    ISET(sst->fbzMode, SST_RGBWRMASK | SST_ZAWRMASK);
    ISET(sst->fastfillCMD, 0x0);   /* Frontbuffer & Z/A */
    ISET(sst->nopCMD, 0x1); /* Clear fbistat registers after clearing screen */
    sst1InitIdleFBINoNOP(sstbase);

    sst1InitRenderingRegisters(sstbase);
    if(sst1InitFillDeviceInfo(sstbase, sst1CurrentBoard) == FXFALSE) {
        INIT_PRINTF(("sst1InitRegisters(): ERROR filling DeviceInfo...\n"));
        return(FXFALSE);
    }

    sst1InitIdleFBINoNOP(sstbase);
    sst1InitRenderingRegisters(sstbase);
    sst1InitIdleFBINoNOP(sstbase);

    /* LFB writes stored in memory FIFO? */
    envp = GETENV(("SST_MEMFIFO_LFB"));
    if(envp)
        n = ATOI(envp);
    else
        n = 1;
    if(n) {
        INIT_PRINTF(("sst1InitRegisters(): LFB Writes go through memory FIFO...\n"));
        ISET(sst->fbiInit0, IGET(sst->fbiInit0) | SST_EN_LFB_MEMFIFO);
        sst1InitIdleFBINoNOP(sstbase);
    }

    /* Texture memory writes stored in memory FIFO? */
    envp = GETENV(("SST_MEMFIFO_TEX"));
    if(envp)
        n = ATOI(envp);
    else
        n = 1;
    if(n) {
        INIT_PRINTF(("sst1InitRegisters(): TEXTURE Writes go through memory FIFO...\n"));
        ISET(sst->fbiInit0, IGET(sst->fbiInit0) | SST_EN_TEX_MEMFIFO);
        sst1InitIdleFBINoNOP(sstbase);
    }

    ISET(sst->vRetrace, 0x0);
    ISET(sst->backPorch, 0x0);
    ISET(sst->videoDimensions, 0x0);
    ISET(sst->hSync, 0x0);
    ISET(sst->vSync, 0x0);
    ISET(sst->videoFilterRgbThreshold, 0x0);

    sst1InitIdleFBINoNOP(sstbase);  /* Wait until init regs are reset */

    if(GETENV(("SST_TEXMAP_DISABLE"))) {
        INIT_PRINTF(("sst1InitRegisters() WARNING: Disabling texture mapping\n"));
        ISET(sst->fbiInit3, IGET(sst->fbiInit3) | SST_TEXMAP_DISABLE);
        sst1InitIdleFBINoNOP(sstbase);
    }
    INIT_PRINTF(("sst1InitRegisters(): exiting with status %d...\n", FXTRUE));
    return(FXTRUE);
}

FX_EXPORT void FX_CSTYLE sst1InitRenderingRegisters(FxU32 *sstbase)
{
    FxU32 i;
    volatile Sstregs *sst = (Sstregs *) sstbase;

    ISET(sst->vA.x, 0x0); ISET(sst->vA.y, 0x0);
    ISET(sst->vB.x, 0x0); ISET(sst->vB.y, 0x0);
    ISET(sst->vC.x, 0x0); ISET(sst->vC.y, 0x0);

    ISET(sst->r, 0x0); ISET(sst->g, 0x0);
    ISET(sst->b, 0x0); ISET(sst->z, 0x0);
    ISET(sst->a, 0x0); ISET(sst->s, 0x0);
    ISET(sst->t, 0x0); ISET(sst->w, 0x0);

    ISET(sst->drdx, 0x0); ISET(sst->dgdx, 0x0);
    ISET(sst->dbdx, 0x0); ISET(sst->dzdx, 0x0);
    ISET(sst->dadx, 0x0); ISET(sst->dsdx, 0x0);
    ISET(sst->dtdx, 0x0); ISET(sst->dwdx, 0x0);

    ISET(sst->drdy, 0x0); ISET(sst->dgdy, 0x0);
    ISET(sst->dbdy, 0x0); ISET(sst->dzdy, 0x0);
    ISET(sst->dady, 0x0); ISET(sst->dsdy, 0x0);
    ISET(sst->dtdy, 0x0); ISET(sst->dwdy, 0x0);

    ISET(sst->fbzColorPath, 0x0);
    ISET(sst->fogMode, 0x0);
    ISET(sst->alphaMode, 0x0);
    ISET(sst->fbzMode, 0x0);
    ISET(sst->lfbMode, 0x0);
    ISET(sst->clipLeftRight, 0x0);
    ISET(sst->clipBottomTop, 0x0);

    ISET(sst->fogColor, 0x0);
    ISET(sst->zaColor, 0x0);
    ISET(sst->chromaKey, 0x0);
    ISET(sst->stipple, 0x0);
    ISET(sst->c0, 0x0);
    ISET(sst->c1, 0x0);

    ISET(sst->nopCMD, 0x1);   /* Clear fbistat registers */

    for(i=0; i<32; i++)
        ISET(sst->fogTable[i], 0x0);

    /* clear TMU registers */
    ISET(sst->textureMode, 0x0);
    ISET(sst->tLOD, 0x0);
    ISET(sst->tDetail, 0x0);
    ISET(sst->texBaseAddr, 0x0);
    ISET(sst->texBaseAddr1, 0x0);
    ISET(sst->texBaseAddr2, 0x0);
    ISET(sst->texBaseAddr38, 0x0);

    for(i=0; i<12; i++) {
        ISET(sst->nccTable0[i], 0x0);
        ISET(sst->nccTable1[i], 0x0);
    }
    sst1InitIdle(sstbase);
}

/*
** sst1InitShutdown():
**  Shutdown SST-1
**    Resets state of VGA_PASS to return monitor control back to the VGA
**    Powers-down the DAC
**    Resets the graphics clock in FBI to conserve power
**
**    Returns:
**      FXTRUE if successfully shuts down SST-1
**      FXFALSE if cannot shut down the SST-1
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitShutdown(FxU32 *sstbase)
{
    volatile Sstregs *sstMaster = (Sstregs *) sstbase;
    volatile Sstregs *sstSlave = (Sstregs *) sst1InitSliSlaveVirtAddr;
    volatile Sstregs *sstPtr;
    FxU32 n;
    FxU32 SliEnable = sst1InitSliEnabled;
    sst1ClkTimingStruct sstGrxClk;

    if(!sstbase)
        return(FXFALSE);

    /* Wait for the SST-1 to finish rendering... */
    sst1InitIdle(sstbase);

    if(sst1InitSliEnabled) {
        if(sst1InitShutdownSli(sstbase) == FXFALSE)
            return(FXFALSE);
    }

#ifdef __DOS32__
    /*
     * HACK alert. -MS
     *
     * There's a pciClose(), but nobody calls it. This is needed by the
     * DOS DPMI services to close fxmemmap.vxd.
     *
     * We need to move this to the "appropriate" place, wherever that may be.
     */
    /*pciClose(); [dBorca] not just yet! We still need PCI to shutdown */
#endif

    if(GETENV(("SST_NOSHUTDOWN"))) {
        INIT_PRINTF(("sst1InitShutdown(): Bypassing shutdown with SST_NOSHUTDOWN\n"));
        goto noshutdown;
    }

    n = 0;
    for ( ;; ) {
        if(!n)
            sstPtr = sstMaster;
        else
            sstPtr = sstSlave;

        if(sst1InitCheckBoard((FxU32 *) sstPtr) == FXFALSE)
            return(FXFALSE);

        INIT_PRINTF(("sst1InitShutdown(): Shutting down SST-1 #%d...\n", n));

        /* Reset video unit to guarantee no contentions on the memory bus */
        /* Blank DAC so VGA Passthru works properly */
        ISET(sstPtr->fbiInit1,
          IGET(sstPtr->fbiInit1) | (SST_VIDEO_RESET | SST_VIDEO_BLANK_EN));
        /* Turn off dram refresh to guarantee no contentions on the
           memory bus */
        ISET(sstPtr->fbiInit2, IGET(sstPtr->fbiInit2) & ~SST_EN_DRAM_REFRESH);

        /* Reset graphics subsystem */
        ISET(sstPtr->fbiInit0,
          IGET(sstPtr->fbiInit0) | (SST_GRX_RESET | SST_PCI_FIFO_RESET));
        sst1InitIdleFBINoNOP((FxU32 *) sstPtr);
        ISET(sstPtr->fbiInit0, IGET(sstPtr->fbiInit0) & ~SST_PCI_FIFO_RESET);
        sst1InitIdleFBINoNOP((FxU32 *) sstPtr);
        ISET(sstPtr->fbiInit0, IGET(sstPtr->fbiInit0) & ~SST_GRX_RESET);
        sst1InitIdleFBINoNOP((FxU32 *) sstPtr);

        /* Turnaround VGA_PASS to allow VGA monitor */
        sst1InitVgaPassCtrl((FxU32 *) sstPtr, 1);

        sst1InitIdleFBINoNOP((FxU32 *) sstPtr);

        /* Set clock at 30 MHz to reduce power consumption... */
        sst1InitComputeClkParams((float) 30.0, &sstGrxClk);
        if(sst1InitSetGrxClk((FxU32 *) sstPtr, &sstGrxClk) == FXFALSE)
            return(FXFALSE);
        sst1CurrentBoard->initGrxClkDone = 0;
        
        if((++n > 1) || !SliEnable)
            break;
    }

    noshutdown: ;

    sst1InitIdle(sstbase);
    INIT_PRINTF(("sst1InitShutdown(): Returning with status %d...\n", FXTRUE));
#ifdef INIT_OUTPUT
    if ( sst1InitMsgFile != stdout && sst1InitMsgFile )
        fclose(sst1InitMsgFile);
#endif
    return(FXTRUE);
}

/*
** sst1InitCheckBoard():
**  Set internal global variables for multiple SST-1 support
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitCheckBoard(FxU32 *sstbase)
{
    FxU32 n;
    FxBool return_val = FXFALSE;

    for(n=0; n<boardsInSystem; n++) {
        if(sst1BoardInfo[n].virtAddr == sstbase) {
            sst1InitDeviceNumber = sst1BoardInfo[n].deviceNumber;
            sst1CurrentBoard = &sst1BoardInfo[n];
            return_val = FXTRUE;
            n = boardsInSystem;
        }
    }
    return(return_val);
}

/*
** sst1InitGetBaseAddr():
**  Return virtual base address to hardware
**  Returns NULL if board requested has not been mapped
**
*/
FX_EXPORT FxU32 * FX_CSTYLE sst1InitGetBaseAddr(FxU32 boardNum)
{
    return(sst1BoardInfo[boardNum].virtAddr);
}

/*
** sst1InitClearBoardInfo():
**  Clear board configuration information
**
*/
void sst1InitClearBoardInfo(void)
{
    FxU32 n, j;

    for(n=0; n<SST1INIT_MAX_BOARDS; n++) {
        //sst1BoardInfo[n].virtAddr = (FxU32 *) NULL;
        //sst1BoardInfo[n].physAddr = (FxU32) NULL;
        //sst1BoardInfo[n].deviceNumber = 0xdead;
        //sst1BoardInfo[n].vendorID = 0xdead;
        //sst1BoardInfo[n].deviceID = 0xdead;
        //sst1BoardInfo[n].fbiRevision = 0xdead;
        sst1BoardInfo[n].fbiBoardID = 0xdead;
        sst1BoardInfo[n].fbiVideo16BPP = 0xdead;
        sst1BoardInfo[n].fbiVideoWidth = 0xdead;
        sst1BoardInfo[n].fbiVideoHeight = 0xdead;
        sst1BoardInfo[n].fbiVideoRefresh = 0xdead;
        sst1BoardInfo[n].fbiMemoryFifoEn = 0x0;
        sst1BoardInfo[n].sstSliDetect = 0;
        sst1BoardInfo[n].tmuRevision = 0xdead;
        sst1BoardInfo[n].numberTmus = 0xdead;
        sst1BoardInfo[n].tmuConfig = 0xdead;
        sst1BoardInfo[n].fbiMemSize = 0x0;
        for(j=0; j<MAX_NUM_TMUS; j++) {
            sst1BoardInfo[n].tmuMemSize[j] = 0x0;
            sst1BoardInfo[n].tmuInit0[j] = 0xdead;
            sst1BoardInfo[n].tmuInit1[j] = 0xdead;
        }
        sst1BoardInfo[n].initGrxClkDone = 0;
        sst1BoardInfo[n].vgaPassthruDisable = 0;
        sst1BoardInfo[n].vgaPassthruEnable = 0;
        sst1BoardInfo[n].fbiDacType = 0xdead;
        sst1BoardInfo[n].memFifoStatusLwm = 0xdead;
        //sst1BoardInfo[n].numBoardInsystem = 0xdead;
        sst1BoardInfo[n].grxClkFreq = 0xdead;
        sst1BoardInfo[n].fbiMemSpeed = 50;
        sst1BoardInfo[n].tmuMemSpeed = 50;
        sst1BoardInfo[n].tmuClkFixed = 0;
        sst1BoardInfo[n].fbiRegulatorPresent = 0;
        sst1BoardInfo[n].fbiTripleBufferingEnabled = FXFALSE;
    }
}

/*
** sst1InitNumBoardsInSystem():
**  Returns the number of SST-1 boards in the system
**
*/
FX_ENTRY FxU32 FX_CSTYLE sst1InitNumBoardsInSystem(void)
{
    FxU32 vendorID = _3DFX_PCI_ID;     /* 3Dfx Vendor ID */
    FxU32 deviceID = 1;                /* Find only sst1 boards */
    FxU32 numBoards, j, n;

    if(GETENV(("SST_BOARDS")))
        numBoards = ATOI(GETENV(("SST_BOARDS")));
    else {
        numBoards = 0;
        for(j=0; j<SST1INIT_MAX_BOARDS; j++) {
            if(pciFindCardMulti(vendorID, deviceID, &n, j))
                numBoards++;
        }
    }
    return(numBoards);
}

static FxU32 
  mtrr;

/*
** sst1InitCachingOn
**
** Sets up memory caching on P6 systems.
**
*/
FX_EXPORT void FX_CSTYLE 
sst1InitCachingOn(void)
{
  FxU32 
    physAddr;
  FxBool
    res;

  /* Turn on Write combining for wrap 1-n  */
  pciGetConfigData(PCI_BASE_ADDRESS_0, sst1InitDeviceNumber, &physAddr);

  /* For some reason, there sometimes is a 008 at the end of the
  physical address, so mask that puppy RTF out */
  physAddr &= 0xfffff000;

  /* Set up USWC MTRR */
  res = pciFindMTRRMatch(physAddr, 0x1000000,
                         PciMemTypeWriteCombining, &mtrr);

  if (!res)
    res = pciFindFreeMTRR(&mtrr);
  else
    return;                   /* It's already there.  We're done. */
  
  if (res)
    pciSetMTRR(mtrr, physAddr, 0x1000000, PciMemTypeWriteCombining);

} /* sst1InitSetCacheType */

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
