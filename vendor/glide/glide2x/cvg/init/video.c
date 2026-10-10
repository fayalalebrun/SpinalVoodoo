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
** Initialization code for initializing SST-1 video unit
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
#define SST1INIT_VIDEO_ALLOCATE // allocate data structures for video timing
#include <sst1vid.h>
#include <sst1init.h>

#ifdef H3D
#include <h3dglide.h>
#endif

#include "canopus.h"

/*
** sst1InitVideo():
**  Initialize video (including DAC setup) for the specified resolution
**
**  Note: sst1InitVideo() is included for compatibility with SST-1 apps only
**  and should not be used for Voodoo2.  Use sst1InitVideoBuffers() instead.
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitVideo(FxU32 *sstbase,
  GrScreenResolution_t screenResolution, GrScreenRefresh_t screenRefresh,
  void *placeHolder)
{
    FxU32 memSizeInPages;
    sst1VideoTimingStruct *videoTiming;
    FxU32 colorBuffs, auxBuffs;

    if(!sstbase)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(GETENV(("SSTV2_IGNORE_INIT_VIDEO"))) {
        INIT_PRINTF(("WARNING: Ignoring sst1InitVideo()...\n"));
        sst1InitIdleFBINoNOP(sstbase);
        return(FXTRUE);
    }

    if(!(videoTiming = sst1InitFindVideoTimingStruct(screenResolution,
      screenRefresh)))
        return(FXFALSE);

    if(sst1CurrentBoard->fbiMemSize == 1)
      memSizeInPages = 256;
    else if(sst1CurrentBoard->fbiMemSize == 2)
      memSizeInPages = 512;
    else
      memSizeInPages = 1024;
    if(sst1CurrentBoard->sliDetected) 
      memSizeInPages <<= 1;

    // Do we have enough memory for the desired resolution?
    if(memSizeInPages > (3 * videoTiming->memOffset)) {
      colorBuffs = 2;
      auxBuffs = 1;
    } else if(memSizeInPages > (2 * videoTiming->memOffset)) {
      colorBuffs = 2;
      auxBuffs = 0;
    } else {
      INIT_PRINTF(("sst1InitVideo(): Insufficient memory available for desired resolution...\n"));
      return(FXFALSE);
    }

    return(sst1InitVideoBuffers(sstbase, screenResolution, screenRefresh,
      colorBuffs, auxBuffs, (sst1VideoTimingStruct *) NULL, FXTRUE));
}

/*
** sst1InitVideoBuffers():
**  Initialize video (including DAC setup) for the specified resolution
**
**    Returns:
**      FXTRUE if successfully initializes specified SST-1 video resolution
**      FXFALSE if cannot initialize SST-1 to specified video resolution
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitVideoBuffers(FxU32 *sstbase,
  GrScreenResolution_t screenResolution, GrScreenRefresh_t screenRefresh,
  FxU32 nColorBuffs, FxU32 nAuxBuffs, sst1VideoTimingStruct *altVideoTiming,
  FxBool switchPassThru)
{
    FxU32 n, vtmp;
    SstRegs *sst = (SstRegs *) sstbase;
    sst1VideoTimingStruct *sstVideoRez;
    FxU32 video16BPP;
    FxU32 memFifoLwm, memFifoHwm, pciFifoLwm;
    FxU32 vInClkDel, vOutClkDel;
    FxU32 tf0_clkdel, tf1_clkdel, tf2_clkdel;
    FxU32 ft_clkdel, memSizeInPages;
    FxU32 nCol, nAux, miscCtrl;
    int memFifoEntries;
#ifndef DIRECTX
    float vidClkFreq;
#endif
    const char *envp;
    int envval;

    if(!sst)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(GETENV(("SSTV2_IGNORE_INIT_VIDEO"))) {
        INIT_PRINTF(("WARNING: Ignoring sst1InitVideo()...\n"));
        sst1InitIdleFBINoNOP(sstbase);
        return(FXTRUE);
    }

    if(altVideoTiming && !GETENV(("SSTV2_SCREENREZ")) &&
       !GETENV(("SSTV2_SCREENREFRESH")))
         sstVideoRez = altVideoTiming;
    else {
       if(!(sstVideoRez = sst1InitFindVideoTimingStruct(screenResolution,
         screenRefresh)))
           return(FXFALSE);
    }

    nCol = nColorBuffs;
    nAux = nAuxBuffs;

    // Disallow unsupported buffer combinations...
    if(nCol < 2 || nCol > 3 || nAux > 1)
      return(FXFALSE);

    if(sst1CurrentBoard->fbiMemSize == 1)
      memSizeInPages = 256;
    else if(sst1CurrentBoard->fbiMemSize == 2)
      memSizeInPages = 512;
    else
      memSizeInPages = 1024;
    if(sst1CurrentBoard->sliDetected)
      memSizeInPages <<= 1;

    // To be compatible with Voodoo1, if there is enough memory to 
    // allocate an aux buffer, then do it...
    if(nCol == 2 && nAux == 0) {
       if(memSizeInPages > (3 * sstVideoRez->memOffset)) {
          INIT_PRINTF(("sst1InitVideo(): Allocating 1 Aux Buffer for SST-1 Compatibility...\n"));
          nAux++;
       }
    }

    envp = GETENV(("SSTV2_ALLOC_COLOR"));
    if(envp)
      nCol = ATOI(envp);
    envp = GETENV(("SSTV2_ALLOC_AUX"));
    if(envp)
      nAux = ATOI(envp);

    // Disallow unsupported buffer combinations (from environment vars)...
    if(nCol < 2 || nCol > 3 || nAux > 1)
      return(FXFALSE);

    //
    // Do we have enough memory for the desired resolution?
    //
    if(memSizeInPages < ((nCol + nAux) * sstVideoRez->memOffset)) {
      INIT_PRINTF(("sst1InitVideo(): Insufficient memory available for desired resolution.\n"));
      return(FXFALSE);
    }
    envp = GETENV(("SSTV2_VIDEO_24BPP"));
    if(envp)
        sst1CurrentBoard->fbiVideo16BPP = (ATOI(envp)) ^ 0x1;
    else {
        sst1CurrentBoard->fbiVideo16BPP = 0;

        if(sst1CurrentBoard->fbiVideo16BPP && (sstVideoRez->video16BPPIsOK ==
           FXFALSE))
               sst1CurrentBoard->fbiVideo16BPP = 0;
        if(!sst1CurrentBoard->fbiVideo16BPP && (sstVideoRez->video24BPPIsOK ==
           FXFALSE))
               sst1CurrentBoard->fbiVideo16BPP = 1;

        if(altVideoTiming == (sst1VideoTimingStruct *) NULL) {
            // Determine when cannot output 24bpp video...
            // Cannot run at high frequencies across SLI connector...
            if(sst1CurrentBoard->sliDetected &&
              sstVideoRez->clkFreq24bpp > 90.0F)
               sst1CurrentBoard->fbiVideo16BPP = 1;
        }
    }
    video16BPP = sst1CurrentBoard->fbiVideo16BPP;

    // Reset Video Refresh Unit
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    
    // Setup SST video timing registers
    envp = GETENV(("SSTV2_HSYNC"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1) ) {
        INIT_PRINTF(("sst1InitVideo(): Using SST_HSYNC=0x%x\n", vtmp));
        ISET(sst->hSync, vtmp);
    } else
        ISET(sst->hSync, ((sstVideoRez->hSyncOff << SST_VIDEO_HSYNC_OFF_SHIFT) |
                      (sstVideoRez->hSyncOn << SST_VIDEO_HSYNC_ON_SHIFT)));
    envp = GETENV(("SSTV2_VSYNC"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1) ) {
        INIT_PRINTF(("sst1InitVideo(): Using SST_VSYNC=0x%x\n", vtmp));
        ISET(sst->vSync, vtmp);
    } else
        ISET(sst->vSync, ((sstVideoRez->vSyncOff << SST_VIDEO_VSYNC_OFF_SHIFT) |
                      (sstVideoRez->vSyncOn << SST_VIDEO_VSYNC_ON_SHIFT)));
    envp = GETENV(("SSTV2_BACKPORCH"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1) ) {
        INIT_PRINTF(("sst1InitVideo(): Using SST_BACKPORCH=0x%x\n", vtmp));
        ISET(sst->backPorch, vtmp);
    } else
        ISET(sst->backPorch,
                     ((sstVideoRez->vBackPorch << SST_VIDEO_VBACKPORCH_SHIFT) |
                      (sstVideoRez->hBackPorch << SST_VIDEO_HBACKPORCH_SHIFT)));
    envp = GETENV(("SSTV2_DIMENSIONS"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1) ) {
        INIT_PRINTF(("sst1InitVideo(): Using SST_DIMENSIONS=0x%x\n", vtmp));
        sstVideoRez->yDimension = (vtmp >>  SST_VIDEO_YDIM_SHIFT) & 0x3ff;
        sstVideoRez->xDimension = vtmp & 0x3ff;
    }
    ISET(sst->videoDimensions,
                     ((sstVideoRez->yDimension << SST_VIDEO_YDIM_SHIFT) |
                      ((sstVideoRez->xDimension-1) << SST_VIDEO_XDIM_SHIFT)));
    envp = GETENV(("SSTV2_MEMOFFSET"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1) ) {
        INIT_PRINTF(("sst1InitVideo(): Using video memOffset=0x%x\n", vtmp));
        sstVideoRez->memOffset = vtmp;
    }
    envp = GETENV(("SSTV2_TILESINX"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1) ) {
        INIT_PRINTF(("sst1InitVideo(): Using video tilesInX=0x%x\n", vtmp));
        sstVideoRez->tilesInX = vtmp;
    }

    // Update info structure
    sst1CurrentBoard->fbiVideoWidth = sstVideoRez->xDimension;
    sst1CurrentBoard->fbiVideoHeight = sstVideoRez->yDimension;
    sst1CurrentBoard->fbiVideoRefresh = sstVideoRez->refreshRate;
    sst1CurrentBoard->fbiVideoColBuffs = nCol;
    sst1CurrentBoard->fbiVideoAuxBuffs = nAux;
    sst1CurrentBoard->fbiVideoMemOffset = sstVideoRez->memOffset;
    sst1CurrentBoard->fbiVideoTilesInX = sstVideoRez->tilesInX;
    sst1CurrentBoard->fbiVideoStruct = sstVideoRez;

    // Setup SST memory mapper for desired resolution
    if(sst1CurrentBoard->fbiMemSize == 4)
        sst1InitSetResolution(sstbase, sstVideoRez, 1); 
    else
        sst1InitSetResolution(sstbase, sstVideoRez, 0); 

    // Calculate graphics clock frequency
    if(sst1InitCalcGrxClk(sstbase) == FXFALSE)
        return(FXFALSE);

    // Setup video fifo
    // NOTE: Lower values for the video fifo threshold improve video fifo
    // underflow problems
    envp = GETENV(("SSTV2_VFIFO_THRESH"));
    if(envp) {
        envval = ATOI(GETENV(("SSTV2_VFIFO_THRESH")));
        INIT_PRINTF(("sst1InitVideo(): Overriding Default Video Fifo Threshold %d and Storing %d\n",
            sstVideoRez->vFifoThreshold, envval));
        ISET(sst->fbiInit3, (IGET(sst->fbiInit3) & ~SST_VIDEO_FIFO_THRESH) |
            (envval << SST_VIDEO_FIFO_THRESH_SHIFT));
    } else {
        FxU32 vFifoThresholdVal = sstVideoRez->vFifoThreshold;

        if(sst1CurrentBoard->fbiGrxClkFreq < 45)
            // Lower threshold value for slower graphics clocks
            vFifoThresholdVal = sstVideoRez->vFifoThreshold - 4;

        ISET(sst->fbiInit3, (IGET(sst->fbiInit3) & ~SST_VIDEO_FIFO_THRESH) |
            (vFifoThresholdVal << SST_VIDEO_FIFO_THRESH_SHIFT));
    }

    INIT_PRINTF(("sst1InitVideo() Setting up video for resolution (%d, %d), Refresh:%d Hz...\n",
        sstVideoRez->xDimension, sstVideoRez->yDimension,
        sstVideoRez->refreshRate));
    INIT_PRINTF(("sst1InitVideo(): Video Fifo Threshold = %d\n",
        (IGET(sst->fbiInit3) & SST_VIDEO_FIFO_THRESH) >>
         SST_VIDEO_FIFO_THRESH_SHIFT));

    // Initialize Y-Origin
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit3, (IGET(sst->fbiInit3) & ~SST_YORIGIN_TOP) |
        ((sstVideoRez->yDimension - 1) << SST_YORIGIN_TOP_SHIFT));

    sst1InitIdleFBINoNOP(sstbase);

    memFifoLwm = 23;
    envp = GETENV(("SSTV2_MEMFIFO_LWM"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1))
      memFifoLwm = vtmp;
    memFifoHwm = 54;
    envp = GETENV(("SSTV2_MEMFIFO_HWM"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1))
      memFifoHwm = vtmp;
    pciFifoLwm = 13;
    envp = GETENV(("SSTV2_PCIFIFO_LWM"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1))
      pciFifoLwm = vtmp;
    INIT_PRINTF(("sst1InitVideo(): pciFifoLwm:%d  memFifoLwm:%d  memFifoHwm:%d\n",
        pciFifoLwm, memFifoLwm, memFifoHwm));

    // Setup Memory FIFO
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit4, IGET(sst->fbiInit4) &
      ~(SST_MEM_FIFO_ROW_BASE | SST_MEM_FIFO_ROW_ROLL | SST_MEM_FIFO_LWM));
    sst1InitIdleFBINoNOP(sstbase);
    if(sst1CurrentBoard->fbiMemSize == 1)
        ISET(sst->fbiInit4, IGET(sst->fbiInit4) |
          (0xff << SST_MEM_FIFO_ROW_ROLL_SHIFT));
    else if (sst1CurrentBoard->fbiMemSize == 2)
        ISET(sst->fbiInit4, IGET(sst->fbiInit4) |
          (0x1ff << SST_MEM_FIFO_ROW_ROLL_SHIFT));
    else
        // 4 MBytes frame buffer memory...
        ISET(sst->fbiInit4, IGET(sst->fbiInit4) |
          (0x3ff << SST_MEM_FIFO_ROW_ROLL_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);

    // Setup buffer management...
    if(sst1InitAllocBuffers(sstbase, nCol, nAux) == FXFALSE)
       return(FXFALSE);
   
    INIT_PRINTF(("sst1InitVideo(): Allocating %d Color Buffers and %d Aux Buffer(s)...\n", 
                 sst1CurrentBoard->fbiVideoColBuffs, sst1CurrentBoard->fbiVideoAuxBuffs));
    ISET(sst->fbiInit4, IGET(sst->fbiInit4) |
        (((nCol+nAux)*sstVideoRez->memOffset) << SST_MEM_FIFO_ROW_BASE_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit4, IGET(sst->fbiInit4) |
      (memFifoLwm << SST_MEM_FIFO_LWM_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);

    // Set PCI FIFO LWM
    ISET(sst->fbiInit0, (IGET(sst->fbiInit0) & ~SST_PCI_FIFO_LWM) |
            (pciFifoLwm << SST_PCI_FIFO_LWM_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);

    // Enable Memory Fifo...
    envp = GETENV(("SSTV2_MEMFIFO"));
    if(envp)
        n = ATOI(envp);
    else
        n = 1;

    if(n) {
        sst1CurrentBoard->fbiMemoryFifoEn = 1;
        memFifoEntries = (65536 - (int)
          (((int) (memSizeInPages - 1) -
            (int) ((nCol+nAux) * sstVideoRez->memOffset)) * 512)) >> 5;
        if(memFifoEntries <= 256)
          memFifoEntries = 0x100; // max. memory fifo size...
        else if(memFifoEntries >= 2048) {
          INIT_PRINTF(("sst1InitVideo(): Invalid memFifoEntries 0x%x\n",
            memFifoEntries));
          return(FXFALSE);
        }
        envp = GETENV(("SSTV2_MEMFIFO_ENTRIES"));
        if(envp && (SSCANF(envp, "%i", &vtmp) == 1))
          memFifoEntries = vtmp;
        INIT_PRINTF(("sst1InitVideo(): Enabling Memory FIFO (Entries=%d)...\n",
            65536 - (memFifoEntries << 5)));

        sst1CurrentBoard->memFifoStatusLwm = (memFifoEntries << 5) | 0x1f;
        ISET(sst->fbiInit0, (IGET(sst->fbiInit0) & ~(SST_MEM_FIFO_EN |
            SST_MEM_FIFO_HWM | SST_PCI_FIFO_LWM | SST_MEM_FIFO_BURST_HWM)) |
            (memFifoEntries << SST_MEM_FIFO_HWM_SHIFT) |
            (pciFifoLwm << SST_PCI_FIFO_LWM_SHIFT) |
            (memFifoHwm << SST_MEM_FIFO_BURST_HWM_SHIFT) |
            SST_MEM_FIFO_EN);
        INIT_PRINTF(("sst1InitVideo(): Setting memory FIFO LWM to 0x%x (%d)\n",
            sst1CurrentBoard->memFifoStatusLwm,
            sst1CurrentBoard->memFifoStatusLwm));  
    }

    vInClkDel = 1;
    envp = GETENV(("SSTV2_VIN_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1))
      vInClkDel = vtmp;

    vOutClkDel = 0;
    envp = GETENV(("SSTV2_VOUT_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &vtmp) == 1))
      vOutClkDel = vtmp;

    INIT_PRINTF(("sst1InitVideo(): vInClkDel=0x%x  vOutClkDel=0x%x\n",

        vInClkDel, vOutClkDel));

    // Setup miscellaneous control
    miscCtrl = 0;
    if(sstVideoRez->miscCtrl & BIT(0))
      miscCtrl |= SST_SCAN_DOUBLE_HORIZ;
    if(sstVideoRez->miscCtrl & BIT(1))
      miscCtrl |= SST_SCAN_DOUBLE_VERT;
    if(sstVideoRez->miscCtrl & BIT(2))
      miscCtrl |= SST_INVERT_HSYNC;
    if(sstVideoRez->miscCtrl & BIT(3))
      miscCtrl |= SST_INVERT_VSYNC;

    // Drive dac output signals and select input video clock
    if(video16BPP) {
        INIT_PRINTF(("sst1InitVideo(): Setting 16BPP video mode...\n"));
        ISET(sst->fbiInit1, (IGET(sst->fbiInit1) & ~SST_VIDEO_MASK) |
            (SST_VIDEO_DATA_OE_EN |
             SST_VIDEO_BLANK_OE_EN |
             SST_VIDEO_HVSYNC_OE_EN |
             SST_VIDEO_DCLK_OE_EN |
             SST_VIDEO_VID_CLK_2X |
             (vInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
             (vOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
             SST_VIDEO_VCLK_SEL));
        sst1InitIdleFBINoNOP(sstbase);
        ISET(sst->fbiInit5, IGET(sst->fbiInit5) | miscCtrl);
    } else {
        INIT_PRINTF(("sst1InitVideo(): Setting 24BPP video mode...\n"));
#if 0
        // Old clock settings -- probably use for true 24-bit dac output
        ISET(sst->fbiInit1, (IGET(sst->fbiInit1) & ~SST_VIDEO_MASK) |
            (SST_VIDEO_DATA_OE_EN |
             SST_VIDEO_BLANK_OE_EN |
             SST_VIDEO_HVSYNC_OE_EN |
             SST_VIDEO_DCLK_OE_EN |
             SST_VIDEO_VID_CLK_2X |
             SST_VIDEO_VCLK_DIV2 |
             (vInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
             (vOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
             SST_VIDEO_24BPP_EN));
#else
        ISET(sst->fbiInit1, (IGET(sst->fbiInit1) & ~SST_VIDEO_MASK) |
            (SST_VIDEO_DATA_OE_EN |
             SST_VIDEO_BLANK_OE_EN |
             SST_VIDEO_HVSYNC_OE_EN |
             SST_VIDEO_DCLK_OE_EN |
             // SST_VIDEO_VID_CLK_2X | // BIT(17)
             // SST_VIDEO_VCLK_DIV2 |  // MUX select 0x1
             (0x0 << SST_VIDEO_VCLK_SEL_SHIFT) | // MUX select 0x0
             (vInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
             (vOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
             SST_VIDEO_24BPP_EN));
        ISET(sst->fbiInit5, IGET(sst->fbiInit5) |
          SST_VIDEO_CLK_SLAVE_OE_EN | SST_VID_CLK_2X_OUT_OE_EN |
          SST_VID_CLK_DAC_DATA16_SEL | miscCtrl);
#endif
        sst1InitIdleFBINoNOP(sstbase);
        if(!GETENV(("SSTV2_VIDEO_FILTER_DISABLE"))) {
            ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_FILTER_EN);
            envp = GETENV(("SSTV2_VIDEO_FILTER_THRESHOLD"));
            if(envp && (SSCANF(envp, "%i", &vtmp) == 1)) {
                INIT_PRINTF(("sst1InitVideo(): Setting Video Filtering Treshold to 0x%x...\n", vtmp));
                ISET(sst->videoFilterRgbThreshold, vtmp);
            } else
                // ISET(sst->videoFilterRgbThreshold, 0x180c18);
                ISET(sst->videoFilterRgbThreshold, 0x080408);
        } else
            INIT_PRINTF(("sst1InitVideo(): Disabling Video Filtering...\n"));
    }

    // Setup prelim. clock delay values...
    if(sst1CurrentBoard->fbiBoardID == 0x3) {
       // Early 4-layer 4220 board -- Runs at 83 MHz by default...
       if(sst1CurrentBoard->fbiGrxClkFreq <= 80) {
          ft_clkdel = 0x5;
          tf0_clkdel = 0x7;
          tf1_clkdel = 0x8;
          tf2_clkdel = 0x8;
       } else {
          ft_clkdel = 0x4;
          tf0_clkdel = 0x6;
          tf1_clkdel = 0x7;
          tf2_clkdel = 0x7;
       }
    } else if(sst1CurrentBoard->fbiBoardID == 0x2) {
       // 4400 8-layer bringup board -- Runs at 90 MHz by default...
       ft_clkdel = 0x4;
       tf0_clkdel = 0x6;
       tf1_clkdel = 0x6;
       tf2_clkdel = 0x6;
    } else {
       // Setup basic values...
       ft_clkdel = 0x4;
       tf0_clkdel = 0x6;
       tf1_clkdel = 0x6;
       tf2_clkdel = 0x6;
    }

    ISET(sst->fbiInit3, (IGET(sst->fbiInit3) & ~SST_FT_CLK_DEL_ADJ) |
        (ft_clkdel << SST_FT_CLK_DEL_ADJ_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);

    sst1CurrentBoard->tmuInit1[0] = (sst1CurrentBoard->tmuInit1[0] &
        ~SST_TEX_TF_CLK_DEL_ADJ) | (tf0_clkdel<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    sst1CurrentBoard->tmuInit1[1] = (sst1CurrentBoard->tmuInit1[1] &
        ~SST_TEX_TF_CLK_DEL_ADJ) | (tf1_clkdel<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    sst1CurrentBoard->tmuInit1[2] = (sst1CurrentBoard->tmuInit1[2] &
        ~SST_TEX_TF_CLK_DEL_ADJ) | (tf2_clkdel<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);

    // Calling sst1InitResetTmus() will put the new trexinit values in the
    // TMUs, as well as reset FBI and TMU...
    sst1InitResetTmus(sstbase);

    /* If we're running on a canopus board we now have to calculate
     * the actual final clock delays and set them otherwise the video
     * reset is going to wait forever because the hw is never going to
     * become idle waiting for pixels from the tmu's.  
     */
    if (sst1CurrentBoard->fbiBoardID == CANOPUS_ID) {
      // Setup graphics clock to 90 MHz for clockdelay measurement
      if (!sst1SetGrxClk_Canopus(sstbase, 90))
        return(FXFALSE);

      // Calculate final clock delay values...
      if(!sst1InitSetClkDelays(sstbase)) {
        INIT_PRINTF(("sst1InitVideo() ERROR: Could not calculate clock delay values...\n"));
        return(FXFALSE);
      }
    }


    // Setup graphics clock
    if(sst1InitGrxClk(sstbase) == FXFALSE)
        return(FXFALSE);

    // Setup video mode
    if(sst1InitSetVidMode(sstbase, video16BPP) == FXFALSE)
        return(FXFALSE);

    // Adjust Video Clock
#ifndef DIRECTX
    envp = GETENV(("SSTV2_VIDCLK2X"));
    if(envp &&
       (SSCANF(envp, "%f", &vidClkFreq) == 1)) {
        if(sst1InitSetVidClk(sstbase, vidClkFreq) == FXFALSE)
            return(FXFALSE);
    } else {
#endif
        if(sst1InitUseVoodooFile == FXTRUE) {
            if(sst1InitSetVidClkINI(sstbase, sst1CurrentBoard->fbiVideoWidth,
              sst1CurrentBoard->fbiVideoHeight,
              sst1CurrentBoard->fbiVideoRefresh, video16BPP) == FXFALSE)
                return(FXFALSE);
        } else {
            if(video16BPP) {
                if(sst1InitSetVidClk(sstbase, sstVideoRez->clkFreq16bpp) ==
                  FXFALSE)
                    return(FXFALSE);
            } else {
                if(sst1InitSetVidClk(sstbase, sstVideoRez->clkFreq24bpp) ==
                  FXFALSE)
                    return(FXFALSE);
            }
        }
#ifndef DIRECTX
    }
#endif

    // Wait for video clock to stabilize
    for(n=0; n<200000; n++)
        sst1InitReturnStatus(sstbase);

    // Run Video Reset Module
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) & ~SST_VIDEO_RESET);
    sst1InitIdleFBINoNOP(sstbase);

    ISET(sst->fbiInit2, IGET(sst->fbiInit2) | SST_EN_DRAM_REFRESH);

    sst1InitIdleFBINoNOP(sstbase);

    // Calculate final clock delay values...
    if(sst1InitSetClkDelays(sstbase) == FXFALSE) {
       INIT_PRINTF(("sst1InitVideo() ERROR: Could not calculate clock delay values...\n"));
       return(FXFALSE);
    }

    // Clear memory...
    if(!GETENV(("SSTV2_VIDEO_NOCLEAR"))) {
        FxU32 clearColor = 0x0;
        FxU32 pagesToFill;

        envp = GETENV(("SSTV2_VIDEO_CLEARCOLOR"));
        if(envp && (SSCANF(envp, "%i", &vtmp) == 1))
          clearColor = vtmp;

        if(sst1CurrentBoard->fbiMemSize == 1)
           pagesToFill = 256;
        else if(sst1CurrentBoard->fbiMemSize == 2)
          pagesToFill = 512;
        else
          pagesToFill = 1024;

        ISET(sst->bltColor, clearColor);
        ISET(sst->bltDstXY, 0x0);
        ISET(sst->bltSize, ((pagesToFill-1)<<16) | 511);
        ISET(sst->bltCommand, SSTG_FRECTFILL | SSTG_GO);
        sst1InitIdle(sstbase);
    } else
        INIT_PRINTF(("sst1InitVideo(): Not Clearing Frame Buffer...\n"));

    // Clear fbistat registers after clearing screen
    ISET(sst->nopCMD, 0x3);

    // Only switch passthrough and do monitor detection if
    // requested.
    if(switchPassThru == FXTRUE)
    {
      // 3D now owns monitor...
      sst1InitVgaPassCtrl(sstbase, 0);

      // Detect presence of monitor...
      sst1InitMonitorDetect(sstbase);
      INIT_PRINTF(("sst1InitVideo(): Monitor Detected:%d\n",
         sst1CurrentBoard->monitorDetected));

      // sst1InitMonitorDetect() trashes the gamma table.  Reload to the
      // original value if sst1InitGamma() has already been called -- otherwise
      // load a 1.0 gamma table
      if(sst1CurrentBoard->fbiInitGammaDone)
         sst1InitGammaRGB(sstbase, sst1CurrentBoard->fbiGammaRed,
           sst1CurrentBoard->fbiGammaGreen, sst1CurrentBoard->fbiGammaBlue);
      else {
         // Do not display gamma values when called by sst1InitVideo()...
         sst1CurrentBoard->fbiInitGammaDone = 1;
         sst1InitGammaRGB(sstbase, (double) 1.0, (double) 1.0, (double) 1.0);
         sst1CurrentBoard->fbiInitGammaDone = 0;
      }
    }

    if(GETENV(("SSTV2_VIDEO_DISABLE"))) {
        INIT_PRINTF(("sst1InitVideo(): Disabling video timing...\n"));
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
        sst1InitIdleFBINoNOP(sstbase);
    }
    sst1InitIdle(sstbase);

    if(((IGET(sst->status) & SST_SWAPBUFPENDING) >>
      SST_SWAPBUFPENDING_SHIFT) > 0)
        sst1InitClearSwapPending(sstbase);

    // Since initVideo() is the last init call, if the user wants to
    // disable idling, do it here.  This way, all the calls to initIdle
    // required within the init code still idle the hardware.
    if(GETENV(("SSTV2_IGNORE_IDLE"))) {
       INIT_PRINTF(("sst1InitVideo(): Ignoring calls to sst1InitIdle()...\n"));
       initIdleEnabled = 0;
    }

    // Update info structure for initEnable
    PCICFG_RD(SST1_PCI_INIT_ENABLE, sst1CurrentBoard->fbiInitEnable);

    // If this board is SLI-capable, tristate the video output signals so
    // that monitor detection will work properly for the second board.  The
    // video output signals will be re-enabled during SLI initialization for
    // the Master board
    if(sst1CurrentBoard->sliDetected) {
       INIT_PRINTF(("sst1InitVideo(): Disabling video output signals for proper monitor detection...\n"));
       sst1InitVideoShutdown(sstbase, FXFALSE);
    }

    INIT_PRINTF(("sst1InitVideo() exiting with status %d...\n", FXTRUE));
    return(FXTRUE);
}

/*
** sst1InitAllocBuffers():
**  Initialize video for buffer management
**
*/
static FxBool 
sst1InitAllocBuffersDirect(FxU32 *sstbase, FxU32 nColorBuffs, FxU32 nAuxBuffs)
{
   SstRegs *sst = (SstRegs *) sstbase;
   FxU32 nCol = nColorBuffs;
   FxU32 nAux = nAuxBuffs;

   if(!sst)
       return(FXFALSE);

   // Setup buffer management...
   if(nCol == 2 && nAux == 0)
     ISET(sst->fbiInit5, (IGET(sst->fbiInit5) & ~SST_BUFFER_ALLOC) |
       SST_BUFFER_ALLOC_2C0Z);
   else if(nCol == 2 && nAux == 1)
     ISET(sst->fbiInit5, (IGET(sst->fbiInit5) & ~SST_BUFFER_ALLOC) |
       SST_BUFFER_ALLOC_2C1Z);
   else if(nCol == 3 && nAux == 0)
     ISET(sst->fbiInit5, (IGET(sst->fbiInit5) & ~SST_BUFFER_ALLOC) |
       SST_BUFFER_ALLOC_3C0Z);
   else if(nCol == 3 && nAux == 1)
     ISET(sst->fbiInit5, (IGET(sst->fbiInit5) & ~SST_BUFFER_ALLOC) |
       SST_BUFFER_ALLOC_3C1Z);
   else {
     INIT_PRINTF(("sst1InitAllocBuffers(): Unsupported Color/Aux buffer combination (%d/%d)\n", 
                  nCol, nAux));
     return(FXFALSE);
   }
   sst1InitReturnStatus(sstbase);

   return(FXTRUE);
}

FX_EXPORT FxBool FX_CSTYLE 
sst1InitAllocBuffers(FxU32 *sstbase, FxU32 nColorBuffs, FxU32 nAuxBuffs)
{
   SstRegs *sst = (SstRegs *) sstbase;
   FxBool retVal;

   if(!sst)
       return(FXFALSE);

   if(sst1InitCheckBoard(sstbase) == FXFALSE)
       return(FXFALSE);

   retVal = sst1InitAllocBuffersDirect(sstbase, 
                                       nColorBuffs, nAuxBuffs);

   if (retVal && sst1CurrentBoard->sliDetected) {
     /* NB: When writing to the slave we need to make sure that it does
      * not have a client callback installed.  
      */
     FxSet32Proc saveProc = sst1CurrentBoard->set32;
     sst1CurrentBoard->set32 = NULL;

     sst1InitAllocBuffersDirect(sst1CurrentBoard->sliSlaveVirtAddr,
                                nColorBuffs, nAuxBuffs);

     sst1CurrentBoard->set32 = saveProc;
   }

   return retVal;
}

/*
** sst1InitVideoShutdown():
**  Shutdown video by not driving video outputs
**  Do not reset the video engine
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitVideoShutdown(FxU32 *sstbase,
                                                 FxBool switchPassThruToVGA)
{
   SstRegs *sst = (SstRegs *) sstbase;

   if(!sst)
      return(FXFALSE);

   if(sst1InitCheckBoard(sstbase) == FXFALSE)
      return(FXFALSE);

   ISET(sst->fbiInit1, (IGET(sst->fbiInit1) & ~(SST_VIDEO_DATA_OE_EN |
      SST_VIDEO_DCLK_OE_EN | SST_VIDEO_HVSYNC_OE_EN)) | SST_VIDEO_BLANK_EN);
   sst1InitReturnStatus(sstbase);
   ISET(sst->fbiInit5, IGET(sst->fbiInit5) & ~(
     SST_VIDEO_CLK_SLAVE_OE_EN | SST_VID_CLK_2X_OUT_OE_EN));
   sst1InitReturnStatus(sstbase);

   if(switchPassThruToVGA == FXTRUE)
      // 2D/VGA now owns monitor...
      sst1InitVgaPassCtrl(sstbase, 1);

   return(FXTRUE);
}

/*
** sst1InitSetVidMode():
**  Set video Mode
**
*/
FxBool sst1InitSetVidMode(FxU32 *sstbase, FxU32 video16BPP)
{
    int helper = (GETENV(("SSTV2_DEBUGDAC"))) ? 1 : 0;

    if(helper)
        INIT_PRINTF(("sst1InitSetVidMode(): Entered...\n"));

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(sst1InitUseVoodooFile == FXTRUE) {
        return(sst1InitSetVidModeINI(sstbase, video16BPP));
    } else {
        if((sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_ATT) ||
           (sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_TI))
           return(sst1InitSetVidModeATT(sstbase, video16BPP));
        else if(sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_ICS)
           return(sst1InitSetVidModeICS(sstbase, video16BPP));
        else if(sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_PROXY)
           return(FXTRUE); /* single board SLI - jeske */
        else
            return(FXFALSE);
    }
}

/*
** sst1InitSetResolution():
**  Setup FBI video resolution registers
**  This routine is used by sst1InitVideo()
**
**
*/
FX_EXPORT void FX_CSTYLE sst1InitSetResolution(FxU32 *sstbase, 
                                               sst1VideoTimingStruct *sstVideoRez, 
                                               FxU32 Banked)
{
    SstRegs *sst = (SstRegs *) sstbase;

    if(Banked)
        ISET(sst->fbiInit2, (IGET(sst->fbiInit2) & ~SST_VIDEO_BUFFER_OFFSET) |
            (sstVideoRez->memOffset << SST_VIDEO_BUFFER_OFFSET_SHIFT) |
            SST_DRAM_BANKING_CONFIG | SST_EN_DRAM_BANKED);
    else
        ISET(sst->fbiInit2, (IGET(sst->fbiInit2) & ~SST_VIDEO_BUFFER_OFFSET) |
            (sstVideoRez->memOffset << SST_VIDEO_BUFFER_OFFSET_SHIFT));

    ISET(sst->fbiInit1, (IGET(sst->fbiInit1) & ~SST_VIDEO_TILES_MASK) |
         (((sstVideoRez->tilesInX>>1) & 0xF)<<SST_VIDEO_TILES_IN_X_SHIFT) |
         (((sstVideoRez->tilesInX>>5) & 0x1)<<SST_VIDEO_TILES_IN_X_MSB_SHIFT));

    // sst1CurrentBoard must be set previously with a sst1InitCheckBoard()
    // call...
    sst1CurrentBoard->fbiInit6 =
      (sst1CurrentBoard->fbiInit6 & ~SST_VIDEO_TILES_IN_X_LSB) |
      ((sstVideoRez->tilesInX & 0x1)<<SST_VIDEO_TILES_IN_X_LSB_SHIFT);
    ISET(sst->fbiInit6, sst1CurrentBoard->fbiInit6);
}

/*
** sst1InitSetVidClk():
**  Set video clock
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitSetVidClk(FxU32 *sstbase, float vidClkFreq)
{
    sst1ClkTimingStruct vidClkTiming;
    int helper = (GETENV(("SSTV2_DEBUGDAC"))) ? 1 : 0;

    if(helper)

        INIT_PRINTF(("sst1InitSetVidClk(): Entered...\n"));

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(sst1InitComputeClkParams(vidClkFreq, &vidClkTiming) == FXFALSE)
        return(FXFALSE);

    INIT_PRINTF(("sst1InitSetVidClk(): Setting up %.2f MHz Video Clock...\n",
        vidClkFreq));

    if((sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_ATT) ||
       (sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_TI))
       return(sst1InitSetVidClkATT(sstbase, &vidClkTiming));
    else if(sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_ICS)
       return(sst1InitSetVidClkICS(sstbase, &vidClkTiming));
    else if(sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_PROXY)
       return(FXTRUE); /* single board sli - jeske */
    else
        return(FXFALSE);
}

/*
** sst1InitSetGrxClk():
**  Set graphics clock
**  NOTE: sst1InitSetGrxClk() resets the PCI fifo and the graphics subsystem
**  of FBI
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitSetGrxClk(FxU32 *sstbase,
                                             sst1ClkTimingStruct *sstGrxClk)
{
    FxBool retVal = FXFALSE;
    int helper = (GETENV(("SSTV2_DEBUGDAC"))) ? 1 : 0;
    SstRegs *sst = (SstRegs *) sstbase;

    if(helper)
        INIT_PRINTF(("sst1InitSetGrxClk(): Entered...\n"));

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(sst1InitUseVoodooFile == FXTRUE) {
        retVal = sst1InitSetGrxClkINI(sstbase, sstGrxClk);
    } else {
        if((sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_ATT) ||
           (sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_TI))
           retVal = sst1InitSetGrxClkATT(sstbase, sstGrxClk);
        else if(sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_ICS)
           retVal = sst1InitSetGrxClkICS(sstbase, sstGrxClk);
        else if(sst1CurrentBoard->fbiVideoDacType == SST_FBI_DACTYPE_PROXY) {
           /* single board SLI - jeske*/
           FxU32 i;

           /* Reset graphics unit before we change grx clk */
           ISET(sst->fbiInit0, IGET(sst->fbiInit0) |
                (SST_GRX_RESET | SST_PCI_FIFO_RESET));
           sst1InitIdleFBINoNOP(sstbase);


           for (i=0;i<boardsInSystem;i++) {
               if ((i > 0) && (&sst1BoardInfo[i] == sst1CurrentBoard)) {
                  retVal = sst1InitSetGrxClk((FxU32 *)sst1BoardInfo[i-1].virtAddr[0],
                       sstGrxClk);
                  break;
               }
           }

           /* Wait for graphics clock to stabilize */
           { int n;
           for(n=0; n<200000; n++)
             sst1InitReturnStatus(sstbase);
           
           /* Unreset PCI FIFO and graphic subsystem */
           ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_PCI_FIFO_RESET);
           sst1InitIdleFBINoNOP(sstbase);
           ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_GRX_RESET);
           sst1InitIdleFBINoNOP(sstbase);
           }
           
        }
    }
    if(retVal == FXFALSE)
        return(FXFALSE);

    // Always reset TMUs after a clock change...
    return(sst1InitResetTmus(sstbase));
}

/*
** sst1InitVideoBorder():
**  Initialize video color border
**
**  The 'mask' parameter specifies which border(s) to enable:
**    SST_COLOR_BORDER_LEFT_EN:     Left edge
**    SST_COLOR_BORDER_RIGHT_EN:    Right edge
**    SST_COLOR_BORDER_TOP_EN:      Top edge
**    SST_COLOR_BORDER_BOTTOM_EN:   Bottom edge
**  The 'color' parameter is in XXRRGGBB format
**
**    Returns:
**      FXTRUE if successfully initializes video color border
**      FXFALSE if cannot initialize video color border
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitVideoBorder(FxU32 *sstbase,
                                               FxU32 mask, 
                                               FxU32 color)
{

    FxU32 fbiInit5;
    SstRegs *sst = (SstRegs *) sstbase;

    if(!sst)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    INIT_PRINTF(("sst1InitVideoBorder(): Setting border color to 0x%x...\n",
        color));

    fbiInit5 = IGET(sst->fbiInit5) &
      ~(SST_COLOR_BORDER_LEFT_EN | SST_COLOR_BORDER_RIGHT_EN |
        SST_COLOR_BORDER_TOP_EN | SST_COLOR_BORDER_BOTTOM_EN);
    fbiInit5 |=
      (mask & SST_COLOR_BORDER_LEFT_EN) ? SST_COLOR_BORDER_LEFT_EN : 0;
    fbiInit5 |=
      (mask & SST_COLOR_BORDER_RIGHT_EN) ? SST_COLOR_BORDER_RIGHT_EN : 0;
    fbiInit5 |=
      (mask & SST_COLOR_BORDER_TOP_EN) ? SST_COLOR_BORDER_TOP_EN : 0;
    fbiInit5 |=
      (mask & SST_COLOR_BORDER_BOTTOM_EN) ? SST_COLOR_BORDER_BOTTOM_EN : 0;

    ISET(sst->borderColor, color);
    ISET(sst->fbiInit5, fbiInit5);

    return(FXTRUE);
}

/*
** sst1InitFindVideoTimingStruct():
**  Find SST-1 video timing data structure from GrScreenResolution_t and
**  GrScreenRefresh_t input params
**
*/

FX_EXPORT sst1VideoTimingStruct* FX_CSTYLE
sst1InitFindVideoTimingStruct(GrScreenResolution_t screenResolution, 
                              GrScreenRefresh_t screenRefresh)
{
    GrScreenResolution_t screenRez = screenResolution;
    GrScreenRefresh_t refreshRate = screenRefresh;
    const char *envp;

    // Override Screen resolution with environment variables
    envp = GETENV(("SSTV2_SCREENREZ"));
    if(envp) {
      FxI32 screenRezEnv = ATOI(envp);

      switch(screenRezEnv) {
        case 512256:
          screenRez = GR_RESOLUTION_512x256;
          break;
        case 512:
          screenRez = GR_RESOLUTION_512x384;
          break;
        case 640400:
          screenRez = GR_RESOLUTION_640x400;
          break;
        case 800:
          screenRez = GR_RESOLUTION_800x600;
          break;
        case 856:
          screenRez = GR_RESOLUTION_856x480;
          break;
        case 960:
          screenRez = GR_RESOLUTION_960x720;
          break;
        case 1024:
          screenRez = GR_RESOLUTION_1024x768;
          break;
        default:
          screenRez = GR_RESOLUTION_640x480;
          break;
      }
    }

    // Override Screen resolution with environment variables
    envp = GETENV(("SSTV2_SCREENREFRESH"));
    if(envp) {
      FxI32 refreshRateEnv = ATOI(envp);

      switch(refreshRateEnv) {
        case 75:
          refreshRate = GR_REFRESH_75Hz;
          break;
        case 85:
          refreshRate = GR_REFRESH_85Hz;
          break;
        case 120:
          refreshRate = GR_REFRESH_120Hz;
          break;
        default:
          refreshRate = GR_REFRESH_60Hz;
          break;
      }
    }

    switch(screenRez) {

        case(GR_RESOLUTION_512x256):
          return(&SST_VREZ_512X256_60);
          break;

        case(GR_RESOLUTION_512x384):
          envp = GETENV(("SSTV2_REFRESH_512x384"));
          if(envp)
            refreshRate = sst1InitConvertRefreshRate( ATOI(envp) );
            
          if(refreshRate == GR_REFRESH_120Hz)
             return(&SST_VREZ_512X384_120);
          else if(refreshRate == GR_REFRESH_85Hz) {
             if(sst1CurrentBoard->sliDetected)
                return(&SST_VREZ_512X384_85_NOSCANDOUBLE);
             else
                return(&SST_VREZ_512X384_85);
          } else if(refreshRate == GR_REFRESH_75Hz) {
             if(sst1CurrentBoard->sliDetected)
                return(&SST_VREZ_512X384_75_NOSCANDOUBLE);
             else
                return(&SST_VREZ_512X384_75);
          } else {
             if(sst1CurrentBoard->sliDetected)
                return(&SST_VREZ_512X384_72);
             else
                return(&SST_VREZ_512X384_60);
          }
          break;

        case(GR_RESOLUTION_640x400):
          envp = GETENV(("SSTV2_REFRESH_640x400"));
          if(envp)
            refreshRate = sst1InitConvertRefreshRate( ATOI(envp) );
        
          if(refreshRate == GR_REFRESH_120Hz)
             return(&SST_VREZ_640X400_120);
          else if(refreshRate == GR_REFRESH_85Hz)
             return(&SST_VREZ_640X400_85);
          else if(refreshRate == GR_REFRESH_75Hz)
             return(&SST_VREZ_640X400_75);
          else
             return(&SST_VREZ_640X400_70);
          break;

        case(GR_RESOLUTION_640x480):
          envp = GETENV(("SSTV2_REFRESH_640x480"));
          if(envp)
            refreshRate = sst1InitConvertRefreshRate( ATOI(envp) );
            
          if(refreshRate == GR_REFRESH_120Hz)
             return(&SST_VREZ_640X480_120);
          else if(refreshRate == GR_REFRESH_85Hz)
             return(&SST_VREZ_640X480_85);
          else if(refreshRate == GR_REFRESH_75Hz)
             return(&SST_VREZ_640X480_75);
          else
             return(&SST_VREZ_640X480_60);
          break;

        case(GR_RESOLUTION_800x600):
          envp = GETENV(("SSTV2_REFRESH_800x600"));
          if(envp)
            refreshRate = sst1InitConvertRefreshRate( ATOI(envp) );
            
          if(refreshRate == GR_REFRESH_120Hz)
             return(&SST_VREZ_800X600_120);
          else if(refreshRate == GR_REFRESH_85Hz)
             return(&SST_VREZ_800X600_85);
          else if(refreshRate == GR_REFRESH_75Hz)
             return(&SST_VREZ_800X600_75);
          else
             return(&SST_VREZ_800X600_60);
          break;

        case(GR_RESOLUTION_856x480):
          return(&SST_VREZ_856X480_60);
          break;

        case(GR_RESOLUTION_960x720):
          envp = GETENV(("SSTV2_REFRESH_960x720"));
          if(envp)
            refreshRate = sst1InitConvertRefreshRate( ATOI(envp) );
            
          if(refreshRate == GR_REFRESH_85Hz)
             return(&SST_VREZ_960X720_85);
          else if(refreshRate == GR_REFRESH_75Hz)
             return(&SST_VREZ_960X720_75);
          else
             return(&SST_VREZ_960X720_60);
          break;

        case(GR_RESOLUTION_1024x768):
          envp = GETENV(("SSTV2_REFRESH_1024x768"));
          if(envp)
            refreshRate = sst1InitConvertRefreshRate( ATOI(envp) );
            
          if(refreshRate == GR_REFRESH_85Hz)
             return(&SST_VREZ_1024X768_85);
          else if(refreshRate == GR_REFRESH_75Hz)
             return(&SST_VREZ_1024X768_75);
          else
             return(&SST_VREZ_1024X768_60);
          break;

#ifdef H3D
        /*  In the following cases, sli can't handle line doubled LD modes  */
        /*  therefore, we need to return a non-line doubled mode if we're   */
        /*  sli-ing                                                         */
      
        case (GR_RESOLUTION_640x240_AUTOFLIPPED):
        case (GR_RESOLUTION_640x480_AUTOFLIPPED):
          if (sst1CurrentBoard->sliDetected)
            return(&SST_VREZ_640X960_60);
          else
            return(&SST_VREZ_640X960LD_60);
          break;

        case (GR_RESOLUTION_800x300_AUTOFLIPPED):
        case (GR_RESOLUTION_800x600_AUTOFLIPPED):
          if (sst1CurrentBoard->sliDetected)
            return(&SST_VREZ_800X630_60);
          else
            return(&SST_VREZ_800X1200LD_45);
          break;

        case (GR_RESOLUTION_960x360_AUTOFLIPPED):
        case (GR_RESOLUTION_960x720_AUTOFLIPPED):
          if (sst1CurrentBoard->sliDetected)
            return(&SST_VREZ_960X742_60);
          else
            return((sst1VideoTimingStruct *) NULL);             
          break;

        case (GR_RESOLUTION_1024x384_AUTOFLIPPED):
        case (GR_RESOLUTION_1024x768_AUTOFLIPPED):
          if (sst1CurrentBoard->sliDetected)
            return(&SST_VREZ_1024X768_60);
          else
            return((sst1VideoTimingStruct *) NULL);             
          break;

#endif

        default:
          INIT_PRINTF(("sst1InitFindVideoTimingStruc(): Unsupported Resolution...\n"));
          return((sst1VideoTimingStruct *) NULL);
          break;
    }
}

FxU32 sst1InitConvertRefreshRate( FxU32 refreshRate )
{
  switch( refreshRate )
  {
    case 75:
      return( GR_REFRESH_75Hz );
    case 85:
      return( GR_REFRESH_85Hz );
    case 120:
      return( GR_REFRESH_120Hz );
    default:
      return( GR_REFRESH_60Hz );
  }
}

/*
**
** sst1InitMonitorDetect()
** Detect whether or not a monitor is connected to the board
**
*/

FX_EXPORT FxBool FX_CSTYLE sst1InitMonitorDetect(FxU32 *sstbase)
{
   SstRegs *sst = (SstRegs *) sstbase;
   FxU32 gammaArray[32];
   FxU32 j, k;
   FxU32 gammaCorrectConstant = 0x5c;
   const char *envp;

   if(!sst)
       return(FXFALSE);

   if(sst1InitCheckBoard(sstbase) == FXFALSE)
       return(FXFALSE);

   /* check to see if we are a single board SLI slave, and if
    * so, then don't detect a monitor - jeske
    */
   //if (sst1CurrentBoard->singleBrdSLISlave) {
   //   sst1CurrentBoard->monitorDetected = 0;
   //   return (FXTRUE);
   // }

   envp = GETENV(("SSTV2_MDETECT_CONST"));
   if(envp && (SSCANF(envp, "%i", &j) == 1)) {
     gammaCorrectConstant = j;
     INIT_PRINTF(("sst1InitMonitorDetect(): Using value 0x%x for constant gamma value...\n", gammaCorrectConstant));
   }

   // Force gamma to always output clearColor value...
   for(j=0; j<32; j++)
       gammaArray[j] = gammaCorrectConstant;
       //gammaArray[j] = 0xff; // 0xff never detects monitor...
       //gammaArray[j] = 0x0;  // 0x0 always detects monitor...
   sst1InitGammaTable(sstbase, 32, gammaArray, gammaArray, gammaArray);

   // Wait for for monitor to sync...
   sst1InitIdle(sstbase);
   for(k=0; k<(sst1CurrentBoard->fbiVideoRefresh>>2); k++) {
      // Wait for inactive vsync...
      do {
          j = IGET(sst->status);
      } while(!(j & SST_VRETRACE));
  
      // Wait for active vsync
      do {
          j = IGET(sst->status);
      } while(j & SST_VRETRACE);
   }

   while(1) {
      FxU32 firstInit6 = IGET(sst->fbiInit6) & SST_GPIO_3;
      FxU32 hvRetrace = IGET(sst->hvRetrace);
      FxU32 vBeam = hvRetrace & 0x1fff;
      FxU32 hBeam = (hvRetrace>>16) & 0x7ff;
      FxU32 secondInit6 = IGET(sst->fbiInit6) & SST_GPIO_3;

      if((vBeam > (sst1CurrentBoard->fbiVideoStruct->vBackPorch + 10)) &&
         (vBeam < (sst1CurrentBoard->fbiVideoStruct->vBackPorch +
                   sst1CurrentBoard->fbiVideoHeight - 10)) &&
         (hBeam > (sst1CurrentBoard->fbiVideoStruct->hBackPorch +
                   sst1CurrentBoard->fbiVideoStruct->hSyncOn + 10)) &&
         (hBeam < (sst1CurrentBoard->fbiVideoStruct->hBackPorch +
                   sst1CurrentBoard->fbiVideoStruct->hSyncOn +
                   sst1CurrentBoard->fbiVideoWidth - 10))) {
            if(firstInit6 == secondInit6) {
               if(firstInit6)
                  sst1CurrentBoard->monitorDetected = 1;
               else
                  sst1CurrentBoard->monitorDetected = 0;
               break;
            }
      }
   }

   // Override with environment variable...
   envp = GETENV(("SSTV2_MDETECT"));
   if(envp)
      sst1CurrentBoard->monitorDetected = (ATOI(envp)) ? 1 : 0;

   return(FXTRUE);
}

/*
**
** sst1InitSetClkDelays():
**   Calculate FT, TF0, and TF1 clock delay values...
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitSetClkDelays(FxU32 *sstbase)
{
   FxU32 tf0_clkdel, tf1_clkdel, tf2_clkdel, ft_clkdel;
   SstRegs *sst = (SstRegs *) sstbase;
   const char *envp;
   int i;

   if(sst1CurrentBoard->fbiBoardID == 0x3) {
      // Early 4-layer 4220 board -- Runs at 83 MHz by default...
      if(sst1CurrentBoard->fbiGrxClkFreq <= 80) {
         ft_clkdel = 0x5;
         tf0_clkdel = 0x7;
         tf1_clkdel = 0x8;
         tf2_clkdel = 0x8;
      } else {
         ft_clkdel = 0x4;
         tf0_clkdel = 0x6;
         tf1_clkdel = 0x7;
         tf2_clkdel = 0x7;
      }
      goto setDelays;
   }

   if(GETENV(("SSTV2_IGNORE_CLKDELAYS"))) {
      INIT_PRINTF(("sst1InitSetClkDelays() WARNING: Bypassing dynamic clock delay detection...\n"));
      ft_clkdel = 0x4;
      tf0_clkdel = 0x6;
      tf1_clkdel = 0x6;
      tf2_clkdel = 0x6;
      goto setDelays;
   }

   // FT Clock Delay...
   if(sst1CurrentBoard->fbiNandTree < 5000)
      // Account for very slow processes...
      ft_clkdel = 0x3;
   else
      ft_clkdel = 0x4;

   {
     const char
       *tf2Str = GETENV("SSTV2_INIT_TF2_RESET_DELAY"),
       *tf1Str = GETENV("SSTV2_INIT_TF1_RESET_DELAY"),
       *tf0Str = GETENV("SSTV2_INIT_TF0_RESET_DELAY");
     const FxU32
       tf2 = ((tf2Str == NULL) ? 11 : atoi(tf2Str)),
       tf1 = ((tf1Str == NULL) ? 11 : atoi(tf1Str)),
       tf0 = ((tf0Str == NULL) ?  9 : atoi(tf0Str));

     // TF2 Clock Delay...
     if(sst1CurrentBoard->numberTmus > 2) {
       if(sst1InitCalcTClkDelay(sstbase, 2, tf2) == FXTRUE)
         // Test failed.  Fast process TMUs...
         tf2_clkdel = 0x7;
       else
         tf2_clkdel = 0x6;
     } else 
       tf2_clkdel = 0x6;
     
     // TF1 Clock Delay...
     if(sst1CurrentBoard->numberTmus > 1) {
       if(sst1InitCalcTClkDelay(sstbase, 1, tf1) == FXTRUE)
         // Test failed.  Fast process TMUs...
         tf1_clkdel = 0x7;
       else
         tf1_clkdel = 0x6;
     } else 
       tf1_clkdel = 0x6;
     
     // Reset FBI & TMU, and put TF1 clock delay value back to default...
     if(sst1InitResetTmus(sstbase) == FXFALSE) {
       INIT_PRINTF(("sst1InitSetClkDelays() ERROR(1): Could not reset TMUs...\n"));
       return(FXFALSE);
     }
     
     // TF0 Clock Delay...
     if(sst1InitCalcTClkDelay(sstbase, 0, tf0) == FXTRUE)
       // Test failed.  Fast process TMUs...
       tf0_clkdel = 0x7;
     else
       tf0_clkdel = 0x6;
   }
     
   // Reset FBI & TMU, and put TF0 clock delay value back to default...
   if(sst1InitResetTmus(sstbase) == FXFALSE) {
      INIT_PRINTF(("sst1InitSetClkDelays() ERROR(2): Could not reset TMUs...\n"));
      return(FXFALSE);
   }

   // Adjust for lower frequencies...
   if(sst1CurrentBoard->fbiGrxClkFreq < 80) {
      ft_clkdel++;
      tf0_clkdel++;
      tf1_clkdel++;
      tf2_clkdel++;
   }

setDelays:

   // Override with environment variables
   envp = GETENV(("SSTV2_FT_CLKDEL"));
   if(envp && (SSCANF(envp, "%i", &i) == 1))
     ft_clkdel = i;
   envp = GETENV(("SSTV2_TF0_CLKDEL"));
   if(envp && (SSCANF(envp, "%i", &i) == 1))
     tf0_clkdel = i;
   envp = GETENV(("SSTV2_TF1_CLKDEL"));
   if(envp && (SSCANF(envp, "%i", &i) == 1))
     tf1_clkdel = i;
   envp = GETENV(("SSTV2_TF2_CLKDEL"));
   if(envp && (SSCANF(envp, "%i", &i) == 1))
     tf2_clkdel = i;

   INIT_PRINTF(("sst1InitSetClkDelays(): Setting FBI-to-TREX clock delay to 0x%x...\n", ft_clkdel));
   INIT_PRINTF(("sst1InitSetClkDelays(): Setting TREX#0 TREX-to-FBI clock delay to 0x%x\n",
       tf0_clkdel));
   INIT_PRINTF(("sst1InitSetClkDelays(): Setting TREX#1 TREX-to-FBI clock delay to 0x%x\n",
       tf1_clkdel));
   INIT_PRINTF(("sst1InitSetClkDelays(): Setting TREX#2 TREX-to-FBI clock delay to 0x%x\n",
       tf2_clkdel));

   ISET(sst->fbiInit3, (IGET(sst->fbiInit3) & ~SST_FT_CLK_DEL_ADJ) |
       (ft_clkdel << SST_FT_CLK_DEL_ADJ_SHIFT));
   sst1InitIdleFBINoNOP(sstbase);

   sst1CurrentBoard->tmuInit1[0] = (sst1CurrentBoard->tmuInit1[0] &
       ~SST_TEX_TF_CLK_DEL_ADJ) | (tf0_clkdel<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
   sst1CurrentBoard->tmuInit1[1] = (sst1CurrentBoard->tmuInit1[1] &
       ~SST_TEX_TF_CLK_DEL_ADJ) | (tf1_clkdel<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
   sst1CurrentBoard->tmuInit1[2] = (sst1CurrentBoard->tmuInit1[2] &
       ~SST_TEX_TF_CLK_DEL_ADJ) | (tf2_clkdel<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);

    // Calling sst1InitResetTmus() will put the new trexinit values in the
    // TMUs, as well as reset FBI and TMU...
   if(sst1InitResetTmus(sstbase) == FXFALSE) {
      INIT_PRINTF(("sst1InitSetClkDelays() ERROR(3): Could not reset TMUs...\n"));
      return(FXFALSE);
   }
   return(FXTRUE);
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
