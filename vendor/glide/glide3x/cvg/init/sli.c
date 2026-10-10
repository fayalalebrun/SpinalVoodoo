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
#ifdef BUILD_FOR_SST1
#include <sst.h>
#else
#include <3dfx.h>
#include <cvgregs.h>
#include <cvgdefs.h>
#endif
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
    SstRegs *sstMaster = (SstRegs *) sstbase0;
    SstRegs *sstSlave = (SstRegs *) sstbase1;
    FxU32 masterVInClkDel, masterVOutClkDel;
    FxU32 slaveVInClkDel, slaveVOutClkDel;
    FxU32 masterPVOutClkDel, slavePVOutClkDel;
    FxU32 pciFifoLwm, memFifoLwm;
    FxU32 memOffsetOrig, tilesInXOrig;
    FxU32 memOffsetNew;
    int memFifoEntriesNew;
    FxU32 memSizeInPages;
    FxU32 memFifoRowBaseNew;
    FxU32 swapAlgorithm = SST_SWAP_SLISYNC;
    FxU32 videoWindowActive = 0x1;
    FxU32 videoWindowActiveDrag = 0xf;
    sst1DeviceInfoStruct *sst1M, *sst1S;
    const char *envp;
    int i;

    if(sst1InitCheckBoard(sstbase1) == FXFALSE)
        return(FXFALSE);
    if(!sst1CurrentBoard->sliDetected)
        return(FXFALSE);
    sst1S = sst1CurrentBoard;

    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);
    if(!sst1CurrentBoard->sliDetected)
        return(FXFALSE);
    sst1M = sst1CurrentBoard;

    // Verify that the board configurations are identical
    //
    // NB: Some of the tests are no longer necessary because
    // the init code clients are doing the magic to make the
    // board appear as the minimum value for things like memory.
    if(/* (sst1M->fbiMemSize != sst1S->fbiMemSize) || */
       /*(sst1M->tmuMemSize[0] != sst1S->tmuMemSize[0]) || */
       (sst1M->numberTmus != sst1S->numberTmus) ||
        /* [koolsmoky] mismatched SLI support */
       (!GETENV(("SSTV2_MISMATCHED_SLI")) && (sst1M->fbiBoardID != sst1S->fbiBoardID)) ||
       /* (sst1M->fbiRevision != sst1S->fbiRevision) || */
       /* (sst1M->tmuRevision != sst1S->tmuRevision) || */
       (sst1M->fbiVideoStruct != sst1S->fbiVideoStruct)) {
         INIT_PRINTF(("sst1InitSli() ERROR: Boards types must be identical...\n"));
         return(FXFALSE);
    }

    // Verify that no video scanline doubling is being used...
    if(sst1M->fbiVideoStruct->miscCtrl & BIT(1)) {
       INIT_PRINTF(("sst1InitSli() ERROR: Scanline doubling not supported with SLI...\n"));
       return(FXFALSE);
    }

    INIT_PRINTF(("sst1InitSli(): Enabling Scanline Interleaving...\n"));

    // sst1Initidle() routines must be properly executed...
    initIdleEnabled = 1;

    // User override of swap algorithm...
    envp = GETENV(("SSTV2_SLISWAP"));
    if(envp) {
       FxU32 swapAlg = ATOI(envp);

       if(swapAlg == 1) {
          INIT_PRINTF(("sst1InitSli(): Using dac_data[0] for swapping(%d, %d)...\n", videoWindowActive, videoWindowActiveDrag));
          swapAlgorithm = SST_SWAP_DACDATA0;
       }
    }

    // Get values setup by sst1InitVideo()...
    memOffsetOrig = (IGET(sstMaster->fbiInit2) & SST_VIDEO_BUFFER_OFFSET) >>
       SST_VIDEO_BUFFER_OFFSET_SHIFT;
    tilesInXOrig = (IGET(sstMaster->fbiInit1) & SST_VIDEO_TILES_IN_X) >>
       SST_VIDEO_TILES_IN_X_SHIFT;
    if(IGET(sstMaster->fbiInit1) & SST_VIDEO_TILES_IN_X_MSB)
       tilesInXOrig += 16;

    if(tilesInXOrig & 0x1) {
       // (e.g. 800x600 resolution)
       /*
         Integer formula for:
            memOffset =
               (TRUNC((xDimension/64)+.99) *
                TRUNC((((yDimension/2)+1)/32)+.99))
       */
       memOffsetNew = (tilesInXOrig *
         ((((sst1CurrentBoard->fbiVideoHeight>>1)+1) + 31) >> 5));
    } else {
       // (e.g. 640x480 resolution)
       // Calculate number of tiles in the vertical dimension
       // Must add entire row of tiles at the bottom for fixes for
       // Y-Origin at lower left
       memOffsetNew = tilesInXOrig *
         (((sst1CurrentBoard->fbiVideoHeight >> 1) + 32) >> 5);
    }

    if(sst1CurrentBoard->fbiMemSize == 1)
      memSizeInPages = 256;
    else if(sst1CurrentBoard->fbiMemSize == 2)
      memSizeInPages = 512;
    else
      memSizeInPages = 1024;

    memFifoRowBaseNew = (sst1CurrentBoard->fbiVideoColBuffs +
      sst1CurrentBoard->fbiVideoAuxBuffs) * memOffsetNew;

    memFifoEntriesNew = (65536 - (int)
      (((int) (memSizeInPages - 1) -
        (int) memFifoRowBaseNew) * 512)) >> 5;
    if(memFifoEntriesNew <= 256)
      memFifoEntriesNew = 0x100; // max. memory fifo size...
    else if(memFifoEntriesNew >= 2048) {
      INIT_PRINTF(("sst1InitSli(): Invalid memFifoEntriesNew 0x%x\n",
        memFifoEntriesNew));
      return(FXFALSE);
    }

    INIT_PRINTF(("sst1InitSli(): Allocating %d Color Buffers and %d Aux Buffer(s)...\n", sst1CurrentBoard->fbiVideoColBuffs, sst1CurrentBoard->fbiVideoAuxBuffs));
    INIT_PRINTF(("sst1InitSli(): Allocating Memory/Command Fifo starting at page %d...\n", memFifoRowBaseNew));
    INIT_PRINTF(("sst1InitSli(): Memory FIFO Entries: %d...\n",
      65536 - (memFifoEntriesNew << 5)));
#if 0
    INIT_PRINTF(("sst1InitSli(): memOffsetOrig:%d, tilesInXOrig:%d\n",
      memOffsetOrig, tilesInXOrig));
    INIT_PRINTF(("sst1InitSli(): memOffsetNew:%d, memFifoEntriesNew:0x%x\n",
      memOffsetNew, memFifoEntriesNew));
#endif

    // Setup SLI Slave...
    cntr = 0;
    while(1) {
        if(sst1InitCheckBoard(sstbase1) == FXFALSE)
            return(FXFALSE);
        PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
        PCICFG_WR(SST1_PCI_INIT_ENABLE,
            ((j & ~SST_SCANLINE_SLV_OWNPCI) | SST_SCANLINE_SLI_SLV));
        ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) |
          (SST_VIDEO_RESET | SST_EN_SCANLINE_INTERLEAVE));
        sst1InitIdleFBINoNOP(sstbase1);
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
          ~SST_SWAP_ALGORITHM) | swapAlgorithm);
        sst1InitIdleFBINoNOP(sstbase1);
        // Initialize Y-Origin
        ISET(sstSlave->fbiInit3, (IGET(sstSlave->fbiInit3) & ~SST_YORIGIN_TOP) |
            (sst1CurrentBoard->fbiVideoHeight << SST_YORIGIN_TOP_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        ISET(sstSlave->fbiInit2, (IGET(sstSlave->fbiInit2) &
          ~SST_VIDEO_BUFFER_OFFSET) |
          (memOffsetNew << SST_VIDEO_BUFFER_OFFSET_SHIFT));
        sst1CurrentBoard->fbiVideoMemOffset = memOffsetNew;
        sst1InitIdleFBINoNOP(sstbase1);
        // Initialize memory fifo
        ISET(sstSlave->fbiInit0, (IGET(sstSlave->fbiInit0) &
          ~SST_MEM_FIFO_HWM) | (memFifoEntriesNew << SST_MEM_FIFO_HWM_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        ISET(sstSlave->fbiInit4, (IGET(sstSlave->fbiInit4) &
          ~SST_MEM_FIFO_ROW_BASE) |
          (memFifoRowBaseNew << SST_MEM_FIFO_ROW_BASE_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        // Fix video dimensions
        ISET(sstSlave->videoDimensions,
          (((sst1CurrentBoard->fbiVideoHeight+2) << SST_VIDEO_YDIM_SHIFT) |
           ((sst1CurrentBoard->fbiVideoWidth-1) << SST_VIDEO_XDIM_SHIFT)));
        sst1InitIdleFBINoNOP(sstbase1);
        // Setup video clocks
        ISET(sstSlave->fbiInit5, (IGET(sstSlave->fbiInit5) &
          ~(SST_VIDEO_CLK_SLAVE_OE_EN | SST_VID_CLK_2X_OUT_OE_EN)) |
          SST_VID_CLK_DAC_DATA16_SEL);
        ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) &
          ~SST_VIDEO_VID_CLK_SLAVE);
        sst1CurrentBoard->fbiInit6 &= ~SST_SLI_SYNC_MASTER;
        sst1CurrentBoard->fbiInit6 = ((sst1CurrentBoard->fbiInit6 &
          ~(SST_SLI_SWAP_VACTIVE | SST_SLI_SWAP_VACTIVE_DRAG)) |
          (videoWindowActive<<SST_SLI_SWAP_VACTIVE_SHIFT) |
          (videoWindowActiveDrag<<SST_SLI_SWAP_VACTIVE_DRAG_SHIFT));
        ISET(sstSlave->fbiInit6, sst1CurrentBoard->fbiInit6);

        slaveVInClkDel = 0;
        slaveVOutClkDel = 0;
        slavePVOutClkDel = 0;

        envp = GETENV(("SSTV2_SLIS_VOUT_CLKDEL"));
        if(envp && (SSCANF(envp, "%i", &i) == 1))
            slaveVOutClkDel = i;
        envp = GETENV(("SSTV2_SLIS_PVOUT_CLKDEL"));
        if(envp && (SSCANF(envp, "%i", &i) == 1))
            slavePVOutClkDel = i;
        envp = GETENV(("SSTV2_SLIS_VIN_CLKDEL"));
        if(envp && (SSCANF(envp, "%i", &i) == 1))
            slaveVInClkDel = i;
        INIT_PRINTF(("sst1InitSli(): slaveVinClkdel=0x%x, slaveVOutClkDel=0x%x, slavePVOutClkDel=0x%x\n",
            slaveVInClkDel, slaveVOutClkDel, slavePVOutClkDel));
        if(sst1CurrentBoard->fbiVideo16BPP)
            // 16-bit Video Output
            ISET(sstSlave->fbiInit1, (IGET(sstSlave->fbiInit1) &
                    ~(SST_VIDEO_VCLK_2X_OUTPUT_DEL | SST_VIDEO_VCLK_DEL |
                      SST_VIDEO_VCLK_SEL | SST_VIDEO_VCLK_2X_INPUT_DEL)) | 
                SST_EN_SCANLINE_INTERLEAVE |
                // SST_VIDEO_VID_CLK_SLAVE |
                // SST_VIDEO_VID_CLK_2X |
                // SST_VIDEO_INVERT_VID_CLK_2X | 
                SST_VIDEO_VCLK_SEL |
                SST_PCI_WRWS_1 |
                (slaveVInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
                (slaveVOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
                (slavePVOutClkDel << SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT));
        else
            // 24-bit Video Output
            ISET(sstSlave->fbiInit1, (IGET(sstSlave->fbiInit1) &
                    ~(SST_VIDEO_VCLK_2X_OUTPUT_DEL | SST_VIDEO_VCLK_DEL |
                      SST_VIDEO_VCLK_SEL | SST_VIDEO_VCLK_2X_INPUT_DEL)) | 
                SST_EN_SCANLINE_INTERLEAVE |
                // SST_VIDEO_VID_CLK_SLAVE |
                // SST_VIDEO_VID_CLK_2X |
                // SST_VIDEO_INVERT_VID_CLK_2X |
                (0x0 << SST_VIDEO_VCLK_SEL_SHIFT) |
                SST_PCI_WRWS_1 |
                (slaveVInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
                (slaveVOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
                (slavePVOutClkDel << SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) &
                                  ~(SST_VIDEO_DATA_OE_EN |
                                    SST_VIDEO_BLANK_OE_EN |
                                    SST_VIDEO_BLANK_EN |
                                    SST_VIDEO_DCLK_OE_EN |
                                    SST_VIDEO_HVSYNC_OE_EN));
        sst1InitIdleFBINoNOP(sstbase1);
        // Initialize pci and memory fifos...
        pciFifoLwm = 16;
        memFifoLwm = 26;
        if(!GETENV(("SSTV2_PCIFIFO_LWM")))
            ISET(sstSlave->fbiInit0, (IGET(sstSlave->fbiInit0) &
              ~SST_PCI_FIFO_LWM) | (pciFifoLwm << SST_PCI_FIFO_LWM_SHIFT));
        sst1InitIdleFBINoNOP(sstbase1);
        if(!GETENV(("SSTV2_MEMFIFO_LWM")))
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
        break;
    }

    // Setup SLI Master...
    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);
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
      ~SST_SWAP_ALGORITHM) | swapAlgorithm);
    sst1InitIdleFBINoNOP(sstbase0);
    // Initialize Y-Origin
    ISET(sstMaster->fbiInit3, (IGET(sstMaster->fbiInit3) & ~SST_YORIGIN_TOP) |
        ((sst1CurrentBoard->fbiVideoHeight) << SST_YORIGIN_TOP_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    if(sst1CurrentBoard->fbiVideoWidth == 960)
        ISET(sstMaster->fbiInit0, IGET(sstMaster->fbiInit0) & ~SST_MEM_FIFO_EN);
    ISET(sstMaster->fbiInit2, (IGET(sstMaster->fbiInit2) &
        ~SST_VIDEO_BUFFER_OFFSET) |
      (memOffsetNew << SST_VIDEO_BUFFER_OFFSET_SHIFT));
    sst1CurrentBoard->fbiVideoMemOffset = memOffsetNew;
    sst1InitIdleFBINoNOP(sstbase0);
    // // Initialize memory fifo
    ISET(sstMaster->fbiInit0, (IGET(sstMaster->fbiInit0) & ~SST_MEM_FIFO_HWM) |
        (memFifoEntriesNew << SST_MEM_FIFO_HWM_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    ISET(sstMaster->fbiInit4, (IGET(sstMaster->fbiInit4) &
        ~SST_MEM_FIFO_ROW_BASE) |
      (memFifoRowBaseNew << SST_MEM_FIFO_ROW_BASE_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    // Fix video dimensions
    ISET(sstMaster->videoDimensions,
      (((sst1CurrentBoard->fbiVideoHeight+2) << SST_VIDEO_YDIM_SHIFT) |
       ((sst1CurrentBoard->fbiVideoWidth-1) << SST_VIDEO_XDIM_SHIFT)));
    sst1InitIdleFBINoNOP(sstbase0);
    // Setup video clocks
    ISET(sstMaster->fbiInit5, IGET(sstMaster->fbiInit5) |
      SST_VIDEO_CLK_SLAVE_OE_EN | SST_VID_CLK_2X_OUT_OE_EN |
      SST_VID_CLK_DAC_DATA16_SEL);
    ISET(sstMaster->fbiInit1, IGET(sstMaster->fbiInit1) &
      ~SST_VIDEO_VID_CLK_SLAVE);
    sst1CurrentBoard->fbiInit6 |= SST_SLI_SYNC_MASTER;
    sst1CurrentBoard->fbiInit6 = ((sst1CurrentBoard->fbiInit6 &
      ~(SST_SLI_SWAP_VACTIVE | SST_SLI_SWAP_VACTIVE_DRAG)) |
      (videoWindowActive<<SST_SLI_SWAP_VACTIVE_SHIFT) |
      (videoWindowActiveDrag<<SST_SLI_SWAP_VACTIVE_DRAG_SHIFT));
    ISET(sstMaster->fbiInit6, sst1CurrentBoard->fbiInit6);

    // Following work well up to around 100 MHz...
    // masterVInClkDel = 2;
    // masterVOutClkDel = 0;
    // masterPVOutClkDel = 0;
    // Following seem to work up to 800x600@85 Hz (112 MHz)
    masterVInClkDel = 3;
    masterVOutClkDel = 2;
    masterPVOutClkDel = 0;
    envp = GETENV(("SSTV2_SLIM_VOUT_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &i) == 1))
        masterVOutClkDel = i;
    envp = GETENV(("SSTV2_SLIM_PVOUT_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &i) == 1))
        masterPVOutClkDel = i;
    envp = GETENV(("SSTV2_SLIM_VIN_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &i) == 1))
        masterVInClkDel = i;
    INIT_PRINTF(("sst1InitSli(): masterVinClkdel=0x%x, masterVOutClkDel=0x%x, masterPVOutClkDel=0x%x\n",
        masterVInClkDel, masterVOutClkDel, masterPVOutClkDel));
    if(sst1CurrentBoard->fbiVideo16BPP)
        // 16-bit Video Output
        ISET(sstMaster->fbiInit1, (IGET(sstMaster->fbiInit1) &
                ~(SST_VIDEO_VCLK_2X_OUTPUT_DEL | SST_VIDEO_VCLK_DEL |
                  SST_VIDEO_VCLK_SEL | SST_VIDEO_VCLK_2X_INPUT_DEL |
                  SST_VIDEO_BLANK_EN)) | 
            SST_EN_SCANLINE_INTERLEAVE |
            // SST_VIDEO_VID_CLK_2X |
            // SST_VIDEO_INVERT_VID_CLK_2X |
            SST_VIDEO_VCLK_SEL |
            SST_PCI_WRWS_1 |
            SST_VIDEO_DATA_OE_EN |
            SST_VIDEO_DCLK_OE_EN |
            SST_VIDEO_HVSYNC_OE_EN |
            (masterVInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
            (masterVOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
            (masterPVOutClkDel << SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT));
    else {
        // 24-bit Video Output
        ISET(sstMaster->fbiInit1, (IGET(sstMaster->fbiInit1) &
                ~(SST_VIDEO_VCLK_2X_OUTPUT_DEL | SST_VIDEO_VCLK_DEL |
                  SST_VIDEO_VCLK_SEL | SST_VIDEO_VCLK_2X_INPUT_DEL |
                  SST_VIDEO_BLANK_EN)) | 
            SST_EN_SCANLINE_INTERLEAVE |
            // SST_VIDEO_VID_CLK_2X |
            // SST_VIDEO_INVERT_VID_CLK_2X |
            (0x0 << SST_VIDEO_VCLK_SEL_SHIFT) |
            SST_PCI_WRWS_1 |
            SST_VIDEO_DATA_OE_EN |
            SST_VIDEO_DCLK_OE_EN |
            SST_VIDEO_HVSYNC_OE_EN |
            (masterVInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
            (masterVOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
            (masterPVOutClkDel << SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT));
            sst1InitReturnStatus(sstbase0);
            ISET(sstMaster->fbiInit5, IGET(sstMaster->fbiInit5) |
              SST_VIDEO_CLK_SLAVE_OE_EN | SST_VID_CLK_2X_OUT_OE_EN);
    }
    sst1InitIdleFBINoNOP(sstbase0);
    // Initialize pci and memory fifos...
    pciFifoLwm = 16;
    memFifoLwm = 26;
    if(!GETENV(("SSTV2_PCIFIFO_LWM")))
        ISET(sstMaster->fbiInit0, (IGET(sstMaster->fbiInit0) &
          ~SST_PCI_FIFO_LWM) | (pciFifoLwm << SST_PCI_FIFO_LWM_SHIFT));
    sst1InitIdleFBINoNOP(sstbase0);
    if(!GETENV(("SSTV2_MEMFIFO_LWM")))
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

    // Master
    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE,
        (j & ~(SST_SCANLINE_SLV_OWNPCI | SST_SCANLINE_SLI_SLV)));
    MasterPhysAddr = sst1CurrentBoard->physAddr[0];
    sst1InitReturnStatus(sstbase0); // flush pci packer with reads
    sst1InitReturnStatus(sstbase0);
    sst1InitReturnStatus(sstbase0);

    // Slave
    if(sst1InitCheckBoard(sstbase1) == FXFALSE)
        return(FXFALSE);
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE,
        ((j & ~(SST_SCANLINE_SLV_OWNPCI)) | SST_SCANLINE_SLI_SLV));

    /* dpc - 19 may 1998 - FixMe
     * Min the clock values for the two boards so that they are
     * happy running together.
     */
#define MIN(__a, __b) (((__a) < (__b)) ? (__a) : (__b))
    {
      sst1DeviceInfoStruct* infoMaster;
      sst1DeviceInfoStruct* infoSlave;

      if (!sst1InitCheckBoard(sstbase0)) return FXFALSE;
      infoMaster = sst1CurrentBoard;

      if (!sst1InitCheckBoard(sstbase1)) return FXFALSE;
      infoSlave = sst1CurrentBoard;
      
      /* fbi and tmu clock freqs should be equal since they are
       * both set from sst1InitCalcGrxClk.
       */
      if (infoMaster->fbiGrxClkFreq != infoSlave->fbiGrxClkFreq) {
        /* Recompute the actual clock rates before any clocking down
         * due to sli for some board revisions.  
         */
        if (!sst1InitCalcGrxClk(sstbase0)) return FXFALSE;
        if (!sst1InitCalcGrxClk(sstbase1)) return FXFALSE;

        /* Save the min clock of the two boards */
        infoMaster->fbiGrxClkFreq =
        infoMaster->tmuGrxClkFreq =
        infoSlave->fbiGrxClkFreq  =
        infoSlave->tmuGrxClkFreq  = MIN(infoMaster->fbiGrxClkFreq, infoSlave->fbiGrxClkFreq);

        /* Clear the clock setting bits */
        infoMaster->initGrxClkDone =
        infoSlave->initGrxClkDone  = FXFALSE;

        /* Reset the clocks for each board */
        if (!sst1InitGrxClk(sstbase0)) return FXFALSE;
        if (!sst1InitGrxClk(sstbase1)) return FXFALSE;

        /* Now check for the memory configurations of the devices.
         * They no longer need to match, but any future calls to
         * sst1InitGetDeviceInfo should now reflect the current
         * minimized state of the world.  
         */
        {
          FxU32 memSize, i;

          if (infoMaster->fbiMemSize != infoSlave->fbiMemSize) {
            memSize = MIN(infoMaster->fbiMemSize, infoSlave->fbiMemSize);
            infoMaster->fbiMemSize = 
            infoSlave->fbiMemSize  = memSize;
          }

          for(i = 0; i < infoMaster->numberTmus; i++) {
            if (infoMaster->tmuMemSize[i] != infoSlave->tmuMemSize[i]) {
              memSize = MIN(infoMaster->tmuMemSize[i], infoSlave->tmuMemSize[i]);
              infoMaster->tmuMemSize[i] =
              infoSlave->tmuMemSize[i]  = memSize;
            }
          }
        }
      }
    }
#undef MIN

    // Enable SLI Bus Snooping for the Slave
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE, (j & ~SST_SLI_SNOOP_MEMBASE) |
        ((MasterPhysAddr>>24 & 0xff) << SST_SLI_SNOOP_MEMBASE_SHIFT));
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE, j | SST_SLI_SNOOP_EN);
    sst1InitReturnStatus(sstbase1); // flush pci packer with reads
    sst1InitReturnStatus(sstbase1);
    sst1InitReturnStatus(sstbase1);

    // Reset video unit in both Master and Slave
    // Then, reset graphics unit in both Master and Slave
    // Finally, deassert both resets to guarantee that Master and Slave
    // see the same vsyncs (and maintain the same swap buffer counter)

    // Probit writes to PCI fifo while resetting graphics core
    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE, j & ~SST_PCI_FIFOWR_EN);
    if(sst1InitCheckBoard(sstbase1) == FXFALSE)
        return(FXFALSE);
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE, j & ~SST_PCI_FIFOWR_EN);

    // Reset Video core...
    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);
    ISET(sstMaster->fbiInit1, IGET(sstMaster->fbiInit1) | SST_VIDEO_RESET);
    sst1InitReturnStatus(sstbase0);
    ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) | SST_VIDEO_RESET);
    sst1InitReturnStatus(sstbase1);

    // Reset Graphics core...
    ISET(sstMaster->fbiInit0, IGET(sstMaster->fbiInit0) | SST_GRX_RESET);
    sst1InitReturnStatus(sstbase0);
    ISET(sstSlave->fbiInit0, IGET(sstSlave->fbiInit0) | SST_GRX_RESET);
    sst1InitReturnStatus(sstbase1);

    // Allow resets to propogate...
    sst1InitReturnStatus(sstbase0);
    sst1InitReturnStatus(sstbase1);
    sst1InitReturnStatus(sstbase0);
    sst1InitReturnStatus(sstbase1);

    // De-assert reset to Graphics core...
    ISET(sstMaster->fbiInit0, IGET(sstMaster->fbiInit0) & ~SST_GRX_RESET);
    sst1InitReturnStatus(sstbase0);
    ISET(sstSlave->fbiInit0, IGET(sstSlave->fbiInit0) & ~SST_GRX_RESET);
    sst1InitReturnStatus(sstbase1);

    // De-assert reset to Video core...
    ISET(sstMaster->fbiInit1, IGET(sstMaster->fbiInit1) & ~SST_VIDEO_RESET);
    sst1InitReturnStatus(sstbase0);
    ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) & ~SST_VIDEO_RESET);
    sst1InitReturnStatus(sstbase1);

    // Allow writes to PCI fifo...
    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE, j | SST_PCI_FIFOWR_EN);
    // Update info structure for initEnable
    sst1CurrentBoard->fbiInitEnable = j | SST_PCI_FIFOWR_EN;

    if(sst1InitCheckBoard(sstbase1) == FXFALSE)
        return(FXFALSE);
    PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
    PCICFG_WR(SST1_PCI_INIT_ENABLE, j | SST_PCI_FIFOWR_EN);
    // Update info structure for initEnable
    sst1CurrentBoard->fbiInitEnable = j | SST_PCI_FIFOWR_EN;

    if(sst1InitCheckBoard(sstbase0) == FXFALSE)
        return(FXFALSE);

    // Clear fbistat registers after clearing screen
    ISET(sstMaster->nopCMD, 0x1);

    sst1CurrentBoard->sliSlaveVirtAddr = sstbase1;

    sst1InitIdle(sstbase0);

    // Synchronize Boards
    for(j=0; j<3; j++) {
        while(!(sst1InitReturnStatus(sstbase0) & SST_VRETRACE))
            ;
        IGET(sstMaster->status);
        ISET(sstMaster->swapbufferCMD, 0x0); // Do not wait for vRetrace...
        IGET(sstMaster->status);
        sst1InitIdle(sstbase0);
    }
    while(!(sst1InitReturnStatus(sstbase0) & SST_VRETRACE))
        ;
    IGET(sstMaster->status);
    ISET(sstMaster->swapbufferCMD, 0x1);     // Sync to vRetrace...
    IGET(sstMaster->status);
    sst1InitIdle(sstbase0);

    if(GETENV(("SSTV2_IGNORE_IDLE")))
       initIdleEnabled = 0;

    INIT_PRINTF(("sst1InitSli() exiting with status %d...\n", FXTRUE));
    return(FXTRUE);
}

/*
** sst1InitShutdownSli():
**  Shutdown SLI configuration
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitShutdownSli(FxU32 *sstbase)
{
    SstRegs *sstMaster = (SstRegs *) sstbase;
    SstRegs *sstSlave;
    FxU32 n, j, cntr;

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    sstSlave = (SstRegs *) sst1CurrentBoard->sliSlaveVirtAddr;

    if(sst1CurrentBoard->sliSlaveVirtAddr != (FxU32 *) NULL) {
        INIT_PRINTF(("sst1InitShutdownSli(): Disabling Scanline Interleaving...\n"));
        sst1CurrentBoard->sliSlaveVirtAddr = (FxU32 *) NULL;

        cntr = 0;
        while(1) {
            if(sst1InitCheckBoard((FxU32 *) sstSlave) == FXFALSE)
                return(FXFALSE);
            // Turn off scanline interleaving and snooping in slave...
            PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
            PCICFG_WR(SST1_PCI_INIT_ENABLE, j & ~(SST_SCANLINE_SLV_OWNPCI |
              SST_SCANLINE_SLI_SLV | SST_SLI_SNOOP_EN |
              SST_SLI_SNOOP_MEMBASE));
            ISET(sstSlave->fbiInit1, IGET(sstSlave->fbiInit1) &
              ~SST_EN_SCANLINE_INTERLEAVE);
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
        if(sst1InitCheckBoard((FxU32 *) sstMaster) == FXFALSE)
            return(FXFALSE);
        ISET(sstMaster->fbiInit1, IGET(sstMaster->fbiInit1) &
          ~SST_EN_SCANLINE_INTERLEAVE);
        sst1InitIdle((FxU32 *) sstMaster);
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
    FxU32 sliDetected;
    const char * envp;
    
    envp = GETENV(("SSTV2_SLIDETECT"));
    if(envp)
      sliDetected = ATOI(envp);
    else
      sliDetected = sst1InitSliPaired(sstbase);

    return sliDetected;
}

/*
** sst1InitSliPaired():
**  Determine if scanline interleave has been detected through
**  power-up settings.
**
*/
FX_ENTRY FxU32 FX_CALL 
sst1InitSliPaired(FxU32 *sstbase)
{
    static int firstTime = 1;
    static FxU32 sliPaired = 0;

    if(firstTime) {
      SstRegs* sst = (SstRegs *) sstbase;
      
      sliPaired = (((IGET(sst->fbiInit5) & SST_SLI_DETECT) == SST_SLI_DETECT) &&
                   (boardsInSystem > 1));
      firstTime = 0;
    }

    return sliPaired;
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
