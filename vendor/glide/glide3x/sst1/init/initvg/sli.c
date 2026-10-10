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
** Initialization code for initializing scanline interleaving
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
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <sst1vid.h>
#include <sst1init.h>

/*
** sst1InitSli():
**  Setup Multiple SST-1 subsystems for Scanline Interleaving
**  sstbase0 defined to be the SLI Master
**  sstbase1 defined to be the SLI Slave
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitSli(FxU32 *sstbase0, FxU32 *sstbase1)
{
    FxU32 j, n, MasterPhysAddr, cntr;
    volatile Sstregs *sstMaster = (Sstregs *) sstbase0;
    volatile Sstregs *sstSlave = (Sstregs *) sstbase1;
    FxU32 MasterInit1, SlaveInit1;
    FxU32 memOffset;
    FxU32 masterVInClkDel, masterVOutClkDel;
    FxU32 slaveVInClkDel, slaveVOutClkDel;
    FxU32 masterPVOutClkDel, slavePVOutClkDel;
    FxU32 pciFifoLwm, memFifoLwm;
    FxU32 clkFreqMaster;
    const char *envp;
    int i;

    /* Check to make sure master and slave are installed properly */
    /* Master should have a clock frequency setting of 54 MHz */
    /* Slave should have a clock frequency setting of 50 MHz */
    /* sst1InitCalcGrxClk() will actually setup the master to run at 54 MHz, */
    /* even though the default strapping is 54 MHz.  The 54 MHz strapping is */
    /* implemented to allow software detection of which SLI board is the */
    /* SLI Master */
    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);
    clkFreqMaster = 40 + (((sst1CurrentBoard->tmuConfig >> 3) & 0x7) << 2) +
      (sst1CurrentBoard->fbiConfig & 0x3);
    if(!(GETENV(("SST_IGNORE_SLI_CHECK")))) {
        if(clkFreqMaster != 54) {
            INIT_PRINTF(("sst1InitSli(): SLI Slave board installed as SLI Master.\n"));
            INIT_PRINTF(("               Please power-down, swap SLI boards and re-boot.\n"));
            return(FXFALSE);
        }
    } else
        INIT_PRINTF(("sst1InitSli(): WARNING: Bypassing check for proper SLI installation...\n"));

    INIT_PRINTF(("sst1InitSli(): Enabling Scanline Interleaving...\n"));

    /* Setup SLI Slave... */
    cntr = 0;
    while(1) {
        if(sst1InitCheckBoard(sstbase1) == FXFALSE)
            return(FXFALSE);
        PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
        PCICFG_WR(SST1_PCI_INIT_ENABLE,
            (j | SST_SCANLINE_SLV_OWNPCI | SST_SCANLINE_SLI_SLV));
        ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) |
          (SST_VIDEO_RESET | SST_EN_SCANLINE_INTERLEAVE));
        sst1InitIdleFBINoNOP(sstbase1);
        PCICFG_WR(SST1_PCI_VCLK_DISABLE, 0x0); /* Disable video clock */
        ISET(SST_TREX(sstSlave,0)->trexInit1, sst1CurrentBoard->tmuInit1[0] |
            SST_EN_TEX_SCANLINE_INTERLEAVE | SST_TEX_SCANLINE_INTERLEAVE_SLAVE);
        sst1InitIdle(sstbase1);
        ISET(SST_TREX(sstSlave,1)->trexInit1, sst1CurrentBoard->tmuInit1[1] |
            SST_EN_TEX_SCANLINE_INTERLEAVE | SST_TEX_SCANLINE_INTERLEAVE_SLAVE);
        sst1InitIdle(sstbase1);
        ISET(SST_TREX(sstSlave,2)->trexInit1, sst1CurrentBoard->tmuInit1[2] |
            SST_EN_TEX_SCANLINE_INTERLEAVE | SST_TEX_SCANLINE_INTERLEAVE_SLAVE);
        sst1InitIdle(sstbase1);
        sst1InitIdleFBINoNOP(sstbase1);
        ISET(sstSlave->fbiInit2, (IGET(sstSlave->fbiInit2) &
          ~(SST_EN_DRAM_BANKED | SST_SWAP_ALGORITHM)) | SST_SWAP_DACDATA0);
        sst1InitIdleFBINoNOP(sstbase1);
        /* Initialize Y-Origin */
        ISET(sstSlave->fbiInit3, (IGET(sstSlave->fbiInit3) & ~SST_YORIGIN_TOP) |
            ((sst1CurrentBoard->fbiVideoHeight - 1) << SST_YORIGIN_TOP_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        memOffset = ((IGET(sstSlave->fbiInit2) & SST_VIDEO_BUFFER_OFFSET) >>
            SST_VIDEO_BUFFER_OFFSET_SHIFT) + 16;
        if(memOffset & 0x1) {
            if((memOffset + 1) & 0x3) /* not divisible by 4... */
                memOffset = (memOffset + 1) >> 1;
            else
                memOffset = (memOffset + 3) >> 1;
        } else {
            if(memOffset & 0x3) /* not divisible by 4... */
                memOffset = (memOffset + 2) >> 1;
            else
                memOffset = memOffset >> 1;
        }
        if(sst1CurrentBoard->fbiVideoWidth == 960) {
            memOffset = 170;
            ISET(sstSlave->fbiInit0, IGET(sstSlave->fbiInit0) &
              ~SST_MEM_FIFO_EN);
            INIT_PRINTF(("sst1InitSli(): Disabling memory fifo...\n"));
        }
        ISET(sstSlave->fbiInit2, (IGET(sstSlave->fbiInit2) &
          ~SST_VIDEO_BUFFER_OFFSET) |
          (memOffset << SST_VIDEO_BUFFER_OFFSET_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        ISET(sstSlave->fbiInit0, (IGET(sstSlave->fbiInit0) &
          ~SST_MEM_FIFO_HWM) | (0x100 << SST_MEM_FIFO_HWM_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        ISET(sstSlave->fbiInit4, (IGET(sstSlave->fbiInit4) &
          ~SST_MEM_FIFO_ROW_BASE) |
          ((3*memOffset) << SST_MEM_FIFO_ROW_BASE_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        //slaveVInClkDel = 0;
        //slaveVOutClkDel = 1;
        //slavePVOutClkDel = 1;
        slaveVInClkDel = 2;
        slaveVOutClkDel = 0;
        slavePVOutClkDel = 3;
        envp = GETENV(("SST_SLIS_VOUT_CLKDEL"));
        if(envp && (SSCANF(envp, "%i", &i) == 1))
          slaveVOutClkDel = i;
        envp = GETENV(("SST_SLIS_PVOUT_CLKDEL"));
        if(envp && (SSCANF(envp, "%i", &i) == 1))
          slavePVOutClkDel = i;
        envp = GETENV(("SST_SLIS_VIN_CLKDEL"));
        if(envp && (SSCANF(envp, "%i", &i) == 1))
          slaveVInClkDel = i;
        INIT_PRINTF(("sst1InitSli(): slaveVinClkdel=0x%x, slaveVOutClkDel=0x%x, slavePVOutClkDel=0x%x\n",
            slaveVInClkDel, slaveVOutClkDel, slavePVOutClkDel));
        if(sst1CurrentBoard->fbiVideo16BPP)
            /* 16-bit Video Output */
            ISET(sstSlave->fbiInit1, (IGET(sstSlave->fbiInit1) &
                    ~(SST_VIDEO_VCLK_2X_OUTPUT_DEL | SST_VIDEO_VCLK_DEL)) | 
                SST_EN_SCANLINE_INTERLEAVE |
                /* SST_VIDEO_VID_CLK_SLAVE | */
                SST_VIDEO_VID_CLK_2X |
                /* SST_VIDEO_INVERT_VID_CLK_2X | */
                SST_PCI_WRWS_1 |
                (slaveVInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
                (slaveVOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
                (slavePVOutClkDel << SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT));
        else
            /* 24-bit Video Output */
            ISET(sstSlave->fbiInit1, (IGET(sstSlave->fbiInit1) &
                    ~(SST_VIDEO_VCLK_2X_OUTPUT_DEL | SST_VIDEO_VCLK_DEL |
                      SST_VIDEO_VCLK_SEL | SST_VIDEO_VCLK_2X_INPUT_DEL)) | 
                SST_EN_SCANLINE_INTERLEAVE |
                /* SST_VIDEO_VID_CLK_SLAVE | */
                SST_VIDEO_VID_CLK_2X |
                /* SST_VIDEO_INVERT_VID_CLK_2X | */
                (0x0 << SST_VIDEO_VCLK_SEL_SHIFT) |
                SST_PCI_WRWS_1 |
                (slaveVInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
                (slaveVOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
                (slavePVOutClkDel << SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) &
                               ~(SST_VIDEO_DATA_OE_EN |
                                SST_VIDEO_BLANK_OE_EN |
                                SST_VIDEO_DCLK_OE_EN |
                                SST_VIDEO_HVSYNC_OE_EN));
        sst1InitIdleFBINoNOP(sstbase1);
        /* Initialize pci and memory fifos... */
        pciFifoLwm = 16;
        memFifoLwm = 26;
        if(!GETENV(("SST_PCIFIFO_LWM")))
            ISET(sstSlave->fbiInit0, (IGET(sstSlave->fbiInit0) &
              ~SST_PCI_FIFO_LWM) | (pciFifoLwm << SST_PCI_FIFO_LWM_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        if(!GETENV(("SST_MEMFIFO_LWM")))
            ISET(sstSlave->fbiInit4, (IGET(sstSlave->fbiInit4) &
              ~SST_MEM_FIFO_LWM) | (memFifoLwm << SST_MEM_FIFO_LWM_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        INIT_PRINTF(("sst1InitSli(): Slave pciFifoLwm:%d, memFifoLwm:%d\n",
          ((IGET(sstSlave->fbiInit0) & SST_PCI_FIFO_LWM) >>
            SST_PCI_FIFO_LWM_SHIFT),
          ((IGET(sstSlave->fbiInit4) & SST_MEM_FIFO_LWM) >>
            SST_MEM_FIFO_LWM_SHIFT)));
        sst1InitIdleFBINoNOP(sstbase1);
        sst1InitIdleFBINoNOP(sstbase1);
        ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) |
          (SST_EN_SCANLINE_INTERLEAVE | SST_VIDEO_RESET));
        sst1InitIdleFBINoNOP(sstbase1);
        sst1InitIdleFBINoNOP(sstbase1);
        sst1InitIdleFBINoNOP(sstbase1);
        ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) & ~SST_VIDEO_RESET);
        sst1InitIdleFBINoNOP(sstbase1);
        sst1InitIdleFBINoNOP(sstbase1);
        if(IGET(sstSlave->fbiInit1) & SST_VIDEO_RESET) {
            INIT_PRINTF(("sst1InitSli(): Could not unReset Slave Video...\n"));
            if(++cntr < 10)
                continue;
            else
                return(FXFALSE);
        }
        if(!(IGET(sstSlave->fbiInit1) & SST_EN_SCANLINE_INTERLEAVE)) {
            INIT_PRINTF(("sst1InitSli(): Could not setup SLI Slave...\n"));
            if(++cntr < 10)
                continue;
            else
                return(FXFALSE);
        }
        SlaveInit1 = IGET(sstSlave->fbiInit1);
        break;
    }

    /* Setup SLI Master... */
    ISET(sstMaster->fbiInit1, IGET(sstMaster->fbiInit1) | SST_VIDEO_RESET);
    ISET(SST_TREX(sstMaster,0)->trexInit1, sst1CurrentBoard->tmuInit1[0] |
        SST_EN_TEX_SCANLINE_INTERLEAVE);
    sst1InitIdle(sstbase0);
    ISET(SST_TREX(sstMaster,1)->trexInit1, sst1CurrentBoard->tmuInit1[1] |
        SST_EN_TEX_SCANLINE_INTERLEAVE);
    sst1InitIdle(sstbase0);
    ISET(SST_TREX(sstMaster,2)->trexInit1, sst1CurrentBoard->tmuInit1[2] |
        SST_EN_TEX_SCANLINE_INTERLEAVE);
    sst1InitIdle(sstbase0);
    sst1InitIdleFBINoNOP(sstbase0);
    ISET(sstMaster->fbiInit2, (IGET(sstMaster->fbiInit2) &
      ~(SST_EN_DRAM_BANKED | SST_SWAP_ALGORITHM)) | SST_SWAP_DACDATA0);
    sst1InitIdleFBINoNOP(sstbase0);
    /* Initialize Y-Origin */
    ISET(sstMaster->fbiInit3, (IGET(sstMaster->fbiInit3) & ~SST_YORIGIN_TOP) |
        ((sst1CurrentBoard->fbiVideoHeight) << SST_YORIGIN_TOP_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    if(sst1CurrentBoard->fbiVideoWidth == 960)
        ISET(sstMaster->fbiInit0, IGET(sstMaster->fbiInit0) & ~SST_MEM_FIFO_EN);
    ISET(sstMaster->fbiInit2, (IGET(sstMaster->fbiInit2) &
      ~SST_VIDEO_BUFFER_OFFSET) | (memOffset << SST_VIDEO_BUFFER_OFFSET_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    ISET(sstMaster->fbiInit0, (IGET(sstMaster->fbiInit0) & ~SST_MEM_FIFO_HWM) |
        (0x100 << SST_MEM_FIFO_HWM_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    ISET(sstMaster->fbiInit4, (IGET(sstMaster->fbiInit4) &
      ~SST_MEM_FIFO_ROW_BASE) | ((3*memOffset) << SST_MEM_FIFO_ROW_BASE_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    //masterVInClkDel = 1;
    //masterVOutClkDel = 1;
    //masterPVOutClkDel = 1;
    masterVInClkDel = 2;
    masterVOutClkDel = 0;
    masterPVOutClkDel = 3;
    envp = GETENV(("SST_SLIM_VOUT_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &i) == 1))
      masterVOutClkDel = i;
    envp = GETENV(("SST_SLIM_PVOUT_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &i) == 1))
      masterPVOutClkDel = i;
    envp = GETENV(("SST_SLIM_VIN_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &i) == 1))
      masterVInClkDel = i;
    INIT_PRINTF(("sst1InitSli(): masterVinClkdel=0x%x, masterVOutClkDel=0x%x, masterPVOutClkDel=0x%x\n",
        masterVInClkDel, masterVOutClkDel, masterPVOutClkDel));
    if(sst1CurrentBoard->fbiVideo16BPP)
        /* 16-bit Video Output */
        ISET(sstMaster->fbiInit1, (IGET(sstMaster->fbiInit1) &
                ~(SST_VIDEO_VCLK_2X_OUTPUT_DEL | SST_VIDEO_VCLK_DEL)) | 
            SST_EN_SCANLINE_INTERLEAVE |
            SST_VIDEO_VID_CLK_2X |
            /* SST_VIDEO_INVERT_VID_CLK_2X | */
            SST_PCI_WRWS_1 |
            (masterVInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
            (masterVOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
            (masterPVOutClkDel << SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT));
    else
        /* 24-bit Video Output */
        ISET(sstMaster->fbiInit1, (IGET(sstMaster->fbiInit1) &
                ~(SST_VIDEO_VCLK_2X_OUTPUT_DEL | SST_VIDEO_VCLK_DEL |
                      SST_VIDEO_VCLK_SEL | SST_VIDEO_VCLK_2X_INPUT_DEL)) | 
            SST_EN_SCANLINE_INTERLEAVE |
            SST_VIDEO_VID_CLK_2X |
            /*SST_VIDEO_INVERT_VID_CLK_2X | */
            (0x0 << SST_VIDEO_VCLK_SEL_SHIFT) |
            SST_PCI_WRWS_1 |
            (masterVInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
            (masterVOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
            (masterPVOutClkDel << SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    /* Initialize pci and memory fifos... */
    pciFifoLwm = 16;
    memFifoLwm = 26;
    if(!GETENV(("SST_PCIFIFO_LWM")))
        ISET(sstMaster->fbiInit0, (IGET(sstMaster->fbiInit0) &
          ~SST_PCI_FIFO_LWM) | (pciFifoLwm << SST_PCI_FIFO_LWM_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    if(!GETENV(("SST_MEMFIFO_LWM")))
        ISET(sstMaster->fbiInit4, (IGET(sstMaster->fbiInit4) &
          ~SST_MEM_FIFO_LWM) | (memFifoLwm << SST_MEM_FIFO_LWM_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    INIT_PRINTF(("sst1InitSli(): Master pciFifoLwm:%d, memFifoLwm:%d\n",
      ((IGET(sstMaster->fbiInit0) & SST_PCI_FIFO_LWM) >>
        SST_PCI_FIFO_LWM_SHIFT),
      ((IGET(sstMaster->fbiInit4) & SST_MEM_FIFO_LWM) >>
        SST_MEM_FIFO_LWM_SHIFT)));
    ISET(sstMaster->fbiInit1, IGET(sstMaster->fbiInit1) & ~SST_VIDEO_RESET);
    sst1InitIdleFBINoNOP(sstbase0);
    MasterInit1 = IGET(sstMaster->fbiInit1);

    /* Master */
    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE,
        (j & ~(SST_SCANLINE_SLV_OWNPCI | SST_SCANLINE_SLI_SLV)));
    MasterPhysAddr = sst1CurrentBoard->physAddr;
    sst1InitReturnStatus(sstbase0); /* flush pci packer with reads */
    sst1InitReturnStatus(sstbase0);
    sst1InitReturnStatus(sstbase0);

    /* Slave */
    if(sst1InitCheckBoard(sstbase1) == FXFALSE)
        return(FXFALSE);
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE,
        ((j & ~(SST_SCANLINE_SLV_OWNPCI)) | SST_SCANLINE_SLI_SLV));
    /* Map both boards to same Master physical address */
    PCICFG_WR(PCI_BASE_ADDRESS_0, MasterPhysAddr);
    sst1InitReturnStatus(sstbase0); /* flush pci packer with reads */
    sst1InitReturnStatus(sstbase0);
    sst1InitReturnStatus(sstbase0);

    /* Reset Video units simultaneously */
    ISET(sstMaster->fbiInit1, MasterInit1 | SST_VIDEO_RESET);
    sst1InitIdleFBINoNOP(sstbase0);
    sst1InitIdleFBINoNOP(sstbase0);
    sst1InitIdleFBINoNOP(sstbase0);
    ISET(sstMaster->fbiInit1, MasterInit1);
    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);
    PCICFG_WR(SST1_PCI_INIT_ENABLE, 0x0);  /* Disable writes to init regs */
    sst1InitReturnStatus(sstbase0);
    ISET(sstMaster->fbiInit1, SlaveInit1);
    sst1InitIdleFBINoNOP(sstbase0);
    /* Enable writes to init regs */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, (SST_INITWR_EN | SST_PCI_FIFOWR_EN));
    sst1InitIdleFBINoNOP(sstbase0);

    if(sst1InitCheckBoard(sstbase1) == FXFALSE)
        return(FXFALSE);
    /* Disable writes to slave Init Registers */
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE, (j & ~SST_INITWR_EN));

    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);

    /* Clear buffers as a result of new buffer offsets... */
    if(!GETENV(("SST_VIDEO_NOCLEAR"))) {
        /* Clear Screen */
        FxU32 clearColor = 0x0;

        envp = GETENV(("SST_VIDEO_CLEARCOLOR"));
        if(envp && (SSCANF(envp, "%i", &i) == 1))
          clearColor = i;
        ISET(sstMaster->c1, clearColor);
        ISET(sstMaster->c0, clearColor);
        ISET(sstMaster->zaColor, 0x0);
        ISET(sstMaster->clipLeftRight, sst1CurrentBoard->fbiVideoWidth);
        ISET(sstMaster->clipBottomTop, sst1CurrentBoard->fbiVideoHeight);
        ISET(sstMaster->fbzMode, SST_RGBWRMASK | SST_ZAWRMASK);
        ISET(sstMaster->fastfillCMD, 0x0);    /* Frontbuffer & Z/A */
        ISET(sstMaster->fbzMode, SST_RGBWRMASK | SST_DRAWBUFFER_BACK);
        ISET(sstMaster->fastfillCMD, 0x0);
        ISET(sstMaster->nopCMD, 0x1);
    }
    sst1InitSliEnabled = 1;
    sst1InitSliSlaveVirtAddr = sstbase1;

    sst1InitIdle(sstbase0);

    /* Synchronize Boards */
    for(j=0; j<3; j++) {
        while(!(sst1InitReturnStatus(sstbase0) & SST_VRETRACE))
            ;
        IGET(sstMaster->status);
        ISET(sstMaster->swapbufferCMD, 0x0);
        IGET(sstMaster->status);
        sst1InitIdle(sstbase0);
    }
    while(!(sst1InitReturnStatus(sstbase0) & SST_VRETRACE))
        ;
    IGET(sstMaster->status);
    ISET(sstMaster->swapbufferCMD, 0x1);
    IGET(sstMaster->status);
    sst1InitIdle(sstbase0);

    return(FXTRUE);
}

/*
** sst1InitShutdownSli():
**  Shutdown SLI configuration
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitShutdownSli(FxU32 *sstbase)
{
    volatile Sstregs *sstMaster = (Sstregs *) sstbase;
    volatile Sstregs *sstSlave = (Sstregs *) sst1InitSliSlaveVirtAddr;
    FxU32 n, j, cntr;

    if(sst1InitSliEnabled) {
        INIT_PRINTF(("sst1InitShutdownSli(): Disabling Scanline Interleaving...\n"));
        sst1InitSliEnabled = 0;

        cntr = 0;
        while(1) {
            if(sst1InitCheckBoard(sst1InitSliSlaveVirtAddr) == FXFALSE)
                return(FXFALSE);
            /* Remap slave to original physical address */
            PCICFG_WR(PCI_BASE_ADDRESS_0, sst1CurrentBoard->physAddr);
            /* Turn off scanline interleaving in slave */
            /* Enable writes to init registers in slave */
            PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
            PCICFG_WR(SST1_PCI_INIT_ENABLE,
                (j | SST_SCANLINE_SLV_OWNPCI | SST_SCANLINE_SLI_SLV |
                 SST_INITWR_EN | SST_PCI_FIFOWR_EN));
            PCICFG_RD(SST1_PCI_INIT_ENABLE, j); /* delay */
            ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) &
              ~SST_EN_SCANLINE_INTERLEAVE);
            PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
            PCICFG_WR(SST1_PCI_INIT_ENABLE,
                (j & ~(SST_SCANLINE_SLV_OWNPCI | SST_SCANLINE_SLI_SLV)));
            PCICFG_RD(SST1_PCI_INIT_ENABLE, j); /* delay */
            sst1InitIdle((FxU32 *) sstSlave);

            if(IGET(sstSlave->fbiInit1) & SST_EN_SCANLINE_INTERLEAVE) {
                if(++cntr < 10)
                    continue;
                else {
                    INIT_PRINTF(("sst1InitShutdown(): Could not disable Slave SLI...\n"));
                    return(FXFALSE);
                }
            }
            break;
        }
        ISET(sstMaster->fbiInit1, IGET(sstMaster->fbiInit1) &
          ~SST_EN_SCANLINE_INTERLEAVE);
        sst1InitIdle((FxU32 *) sstMaster);
    }
    return(FXTRUE);
}

/*
** sst1InitSliPciOwner():
**  Specify which SLI card owns PCI bus control
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitSliPciOwner(FxU32 *sstbase, FxU32 owner)
{
    FxU32 j, n;
    static FxU32 masterIsOwner = 1;

    if(!sst1InitSliEnabled)
        return(FXTRUE);

    /* Flush PCI packers... */
    sst1InitReturnStatus(sstbase);
    sst1InitReturnStatus(sstbase);
    sst1InitReturnStatus(sstbase);

    if(masterIsOwner) {
        if(owner == SST_SLI_SLAVE_OWNPCI) {
            masterIsOwner = 0;
            if(sst1InitCheckBoard(sstbase) == FXFALSE)
                return(FXFALSE);
            PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
            PCICFG_WR(SST1_PCI_INIT_ENABLE, ((j & ~SST_SLI_OWNPCI) |
                SST_SLI_SLAVE_OWNPCI));
            if(sst1InitCheckBoard(sst1InitSliSlaveVirtAddr) == FXFALSE)
                return(FXFALSE);
            PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
            PCICFG_WR(SST1_PCI_INIT_ENABLE, ((j & ~SST_SLI_OWNPCI) |
                SST_SLI_SLAVE_OWNPCI));
        } else
            return(FXTRUE);
    } else {
        if(owner == SST_SLI_MASTER_OWNPCI) {
            masterIsOwner = 1;
            if(sst1InitCheckBoard(sst1InitSliSlaveVirtAddr) == FXFALSE)
                return(FXFALSE);
            PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
            PCICFG_WR(SST1_PCI_INIT_ENABLE, ((j & ~SST_SLI_OWNPCI) |
                SST_SLI_MASTER_OWNPCI));
            if(sst1InitCheckBoard(sstbase) == FXFALSE)
                return(FXFALSE);
            PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
            PCICFG_WR(SST1_PCI_INIT_ENABLE, ((j & ~SST_SLI_OWNPCI) |
                SST_SLI_MASTER_OWNPCI));
        } else
            return(FXTRUE);
    }
    return(FXTRUE);
}

/*
** sst1InitSliDetect():
**  Determine if scanline interleave has been detected (either through
**   power-up settings or through SST_SLIDETECT)
**
*/
FX_ENTRY FxU32 FX_CALL sst1InitSliDetect(FxU32 *sstbase)
{
    static int firstTime = 1;
    static FxU32 sliDetected = 0;
    FxU32 fbiRev;
    volatile Sstregs *sst;

    if(firstTime) {
        const char *envp;
        firstTime = 0;
        envp = GETENV(("SST_SLIDETECT"));
        if(envp)
            sliDetected = ATOI(envp);
        else {
            PCICFG_RD(PCI_REVISION_ID, fbiRev);
            sst = (Sstregs *) sstbase;
            if((IGET(sst->fbiInit1) & SST_SLI_DETECT) && (fbiRev > 1) &&
               (boardsInSystem) > 1 &&
               ((IGET(sst->fbiInit3) & SST_FBI_MEM_TYPE) >>
                (SST_FBI_MEM_TYPE_SHIFT + 2)))
               sliDetected = 1;
        }
    }
    return(sliDetected);
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
