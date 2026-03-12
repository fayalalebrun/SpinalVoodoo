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
** Utility routines for SST-1 Initialization code
**
*/

#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */

#ifdef _MSC_VER
#pragma optimize ("",off)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <sst.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <sst1vid.h>
#include <sst1init.h>

#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
static FxBool de10IdleVerbose(void)
{
    static int initialized = 0;
    static FxBool enabled = FXFALSE;

    if(!initialized) {
        enabled = (GETENV(("SST_DE10_IDLE_VERBOSE")) != NULL) ? FXTRUE : FXFALSE;
        initialized = 1;
    }

    return enabled;
}

static void de10DumpBusyDebug(const char *prefix, FxU32 debugBusy)
{
    static const char *bitNames[32] = {
        "triangleSetup.valid",
        "rasterizer.running",
        "tmu.input.valid",
        "tmu.busy",
        "fbAccess.busy",
        "colorCombine.input.valid",
        "fog.input.valid",
        "fbAccess.input.valid",
        "writeColor.valid",
        "writeAux.valid",
        "fastfill.running",
        "swapBuffer.waiting",
        "lfb.busy",
        "fastfill.o.valid",
        "fastfill.o.ready",
        "fastfillWrite.valid",
        "fastfillWrite.ready",
        "preDitherMerged.valid",
        "preDitherMerged.ready",
        "preDitherPiped.valid",
        "preDitherPiped.ready",
        "dither.output.valid",
        "dither.output.ready",
        "ditherJoined.valid",
        "ditherJoined.ready",
        "forColorWrite.valid",
        "forColorWrite.ready",
        "forAuxWrite.valid",
        "forAuxWrite.ready",
        "writeColor.ready",
        "writeAux.ready",
        "fastfillWrite.auxWrite"
    };
    char buffer[512];
    size_t used = 0;
    FxBool first = FXTRUE;
    int bit;

    buffer[0] = '\0';
    for(bit = 0; bit < 32; bit++) {
        if((debugBusy & (1u << bit)) == 0u)
            continue;

        used += snprintf(buffer + used, sizeof(buffer) - used, "%s%s",
            first ? "" : "|", bitNames[bit]);
        first = FXFALSE;
        if(used >= (sizeof(buffer) - 1u))
            break;
    }

    if(first)
        strcpy(buffer, "none");

    fprintf(stderr, "%s busyBits=%s\n", prefix, buffer);
    fflush(stderr);
}

static void de10DumpWritePathDebug(const char *prefix, FxU32 writePathDebug)
{
    static const char *bitNames[32] = {
        "fastfill.running",
        "fastfill.o.valid",
        "fastfill.o.ready",
        "fastfillWrite.output.valid",
        "fastfillWrite.output.ready",
        "preDitherMerged.valid",
        "preDitherMerged.ready",
        "ditherJoined.valid",
        "ditherJoined.ready",
        "forColorWrite.valid",
        "forColorWrite.ready",
        "colorWriteInput.valid",
        "colorWriteInput.ready",
        "writeColor.cmd.valid",
        "writeColor.cmd.ready",
        "colorWriteArb.valid",
        "colorWriteArb.ready",
        "forAuxWrite.valid",
        "forAuxWrite.ready",
        "auxWriteInput.valid",
        "auxWriteInput.ready",
        "writeAux.cmd.valid",
        "writeAux.cmd.ready",
        "auxWriteArb.valid",
        "auxWriteArb.ready",
        "writeAuxCmdPending",
        "fbArbiter.in0.valid",
        "fbArbiter.in0.ready",
        "fbArbiter.in1.valid",
        "fbArbiter.in1.ready",
        "fbArbiter.out.valid",
        "fbArbiter.out.ready"
    };
    char buffer[768];
    size_t used = 0;
    FxBool first = FXTRUE;
    int bit;

    buffer[0] = '\0';
    for(bit = 0; bit < 32; bit++) {
        if((writePathDebug & (1u << bit)) == 0u)
            continue;

        used += snprintf(buffer + used, sizeof(buffer) - used, "%s%s",
            first ? "" : "|", bitNames[bit]);
        first = FXFALSE;
        if(used >= (sizeof(buffer) - 1u))
            break;
    }

    if(first)
        strcpy(buffer, "none");

    fprintf(stderr, "%s writePathBits=%s\n", prefix, buffer);
    fflush(stderr);
}
#endif

/*
** sst1InitIdle():
**  Return idle condition of SST-1
**
**    Returns:
**      FXTRUE if SST-1 is idle (fifos are empty, graphics engines are idle)
**      FXFALSE if SST-1 has not been mapped
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitIdle(FxU32 *sstbase)
{
    if(!sstbase)
        return(FXFALSE);

#ifdef DE10_BACKEND
    fprintf(stderr, "[de10] sst1InitIdle: enter sstbase=%p sli=%d\n",
        (void *)sstbase, sst1InitSliEnabled);
    fflush(stderr);
#endif

    if(!sst1InitSliEnabled)
        sst1InitIdleLoop(sstbase);
    else {
        FxU32 j, n;

        if(sst1InitCheckBoard(sstbase) == FXFALSE)
            return(FXFALSE);
        /* Check idle for Master... */
        sst1InitIdleLoop(sstbase);

        /* Cause slave to drive PCI bus */
        PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
        PCICFG_WR(SST1_PCI_INIT_ENABLE, j | SST_SCANLINE_SLV_OWNPCI);
        if(sst1InitCheckBoard(sst1InitSliSlaveVirtAddr) == FXFALSE)
            return(FXFALSE);
        PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
        PCICFG_WR(SST1_PCI_INIT_ENABLE, j | SST_SCANLINE_SLV_OWNPCI);

        /* Check idle for Slave... */
        sst1InitIdleLoop(sstbase);

        /* Restore normal SLI conditions */
        PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
        PCICFG_WR(SST1_PCI_INIT_ENABLE, j & ~SST_SCANLINE_SLV_OWNPCI);
        if(sst1InitCheckBoard(sstbase) == FXFALSE)
            return(FXFALSE);
        PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
        PCICFG_WR(SST1_PCI_INIT_ENABLE, j & ~SST_SCANLINE_SLV_OWNPCI);
    }
#ifdef DE10_BACKEND
    fprintf(stderr, "[de10] sst1InitIdle: exit\n");
    fflush(stderr);
#endif
    return(FXTRUE);
}

void sst1InitIdleLoop(FxU32 *sstbase)
{
    FxU32 cntr;
    volatile Sstregs *sst = (Sstregs *) sstbase;
#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
    FxU32 spinCount = 0;
    FxU32 status;
    volatile FxU32 *debugBusyReg = (volatile FxU32 *) ((FxU8 *) sstbase + 0x240);
    volatile FxU32 *pixelsInReg = (volatile FxU32 *) ((FxU8 *) sstbase + 0x14c);
    volatile FxU32 *pixelsOutReg = (volatile FxU32 *) ((FxU8 *) sstbase + 0x15c);
    FxU32 debugBusy;
    FxU32 pixelsIn;
    FxU32 pixelsOut;
    FxBool verbose = de10IdleVerbose();

    if(verbose) {
        fprintf(stderr,
            "[de10] sst1InitIdleLoop: enter sstbase=%p statusReg=%p nopReg=%p\n",
            (void *)sstbase, (void *)&sst->status, (void *)&sst->nopCMD);
        fflush(stderr);
    }

    status = sst1InitReturnStatus(sstbase);
    debugBusy = IGET(*debugBusyReg);
    pixelsIn = IGET(*pixelsInReg);
    pixelsOut = IGET(*pixelsOutReg);
    if(verbose) {
        fprintf(stderr,
            "[de10] sst1InitIdleLoop: pre-nop status=0x%08x debugBusy=0x%08x pixelsIn=%u pixelsOut=%u\n",
            status, debugBusy, pixelsIn, pixelsOut);
        fflush(stderr);
    }
#endif

    ISET(sst->nopCMD, 0x0);
#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
    status = sst1InitReturnStatus(sstbase);
    debugBusy = IGET(*debugBusyReg);
    pixelsIn = IGET(*pixelsInReg);
    pixelsOut = IGET(*pixelsOutReg);
    if(verbose) {
        fprintf(stderr,
            "[de10] sst1InitIdleLoop: post-nop status=0x%08x debugBusy=0x%08x pixelsIn=%u pixelsOut=%u\n",
            status, debugBusy, pixelsIn, pixelsOut);
        fflush(stderr);
    }
#endif
    cntr = 0;
    while(1) {
        status = sst1InitReturnStatus(sstbase);
        if(!(status & SST_BUSY)) {
            if(++cntr >= 3)
                break;
        } else
            cntr = 0;
#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
        debugBusy = IGET(*debugBusyReg);
        pixelsIn = IGET(*pixelsInReg);
        pixelsOut = IGET(*pixelsOutReg);
        if (verbose && ((spinCount < 8u) || ((spinCount % 200000u) == 0u))) {
            fprintf(stderr,
                "[de10] sst1InitIdleLoop: spin=%u status=0x%08x debugBusy=0x%08x pixelsIn=%u pixelsOut=%u cntr=%u\n",
                spinCount, status, debugBusy, pixelsIn, pixelsOut, cntr);
            fflush(stderr);
        }
        if(++spinCount >= 1000000u) {
            INIT_PRINTF(("sst1InitIdleLoop(): DE10 timeout status=0x%x\n",
                status));
            break;
        }
#endif
    }
#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
    debugBusy = IGET(*debugBusyReg);
    pixelsIn = IGET(*pixelsInReg);
    pixelsOut = IGET(*pixelsOutReg);
    if(verbose) {
        fprintf(stderr,
            "[de10] sst1InitIdleLoop: exit spin=%u cntr=%u final=0x%08x debugBusy=0x%08x pixelsIn=%u pixelsOut=%u\n",
            spinCount, cntr, status, debugBusy, pixelsIn, pixelsOut);
        fflush(stderr);
    }
#endif
}

/*
** sst1InitIdleFBI():
**  Return idle condition of FBI (ignoring idle status of TMU)
**
**    Returns:
**      FXTRUE if FBI is idle (fifos are empty, graphics engines are idle)
**      FXFALSE if FBI has not been mapped
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitIdleFBI(FxU32 *sstbase)
{
    FxU32 cntr;
    volatile Sstregs *sst = (Sstregs *) sstbase;
#ifdef DE10_BACKEND
    FxU32 spinCount = 0;
#endif

    if(!sst)
        return(FXFALSE);

    ISET(sst->nopCMD, 0x0);
    cntr = 0;
    while(1) {
        if(!(sst1InitReturnStatus(sstbase) & SST_FBI_BUSY)) {
            if(++cntr >= 3)
                break;
        } else
            cntr = 0;
#ifdef DE10_BACKEND
        if(++spinCount >= 1000000u) {
            INIT_PRINTF(("sst1InitIdleFBI(): DE10 timeout status=0x%x\n",
                sst1InitReturnStatus(sstbase)));
            break;
        }
#endif
    }
    return(FXTRUE);
}

/*
** sst1InitIdleFBINoNOP():
**  Return idle condition of FBI (ignoring idle status of TMU)
**  sst1InitIdleFBINoNOP() differs from sst1InitIdleFBI() in that no NOP command
**  is issued to flush the graphics pipeline.
**
**    Returns:
**      FXTRUE if FBI is idle (fifos are empty, graphics engines are idle)
**      FXFALSE if FBI has not been mapped
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitIdleFBINoNOP(FxU32 *sstbase)
{
    FxU32 cntr;
    volatile Sstregs *sst = (Sstregs *) sstbase;
#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
    FxU32 spinCount = 0;
    FxU32 status;
    volatile FxU32 *debugBusyReg = (volatile FxU32 *) ((FxU8 *) sstbase + 0x240);
    volatile FxU32 *writePathDebugReg = (volatile FxU32 *) ((FxU8 *) sstbase + 0x250);
    volatile FxU32 *pixelsInReg = (volatile FxU32 *) ((FxU8 *) sstbase + 0x14c);
    volatile FxU32 *pixelsOutReg = (volatile FxU32 *) ((FxU8 *) sstbase + 0x15c);
    FxU32 debugBusy;
    FxU32 writePathDebug;
    FxU32 pixelsIn;
    FxU32 pixelsOut;
    FxU32 activeDebugMask;
    FxBool verbose;
    struct timeval startTv;
    struct timeval nowTv;
    long elapsedMs;
#endif

    if(!sst)
        return(FXFALSE);

    /* ISET(sst->nopCMD, 0x0); */
    cntr = 0;
#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
    status = sst1InitReturnStatus(sstbase);
    debugBusy = IGET(*debugBusyReg);
    writePathDebug = IGET(*writePathDebugReg);
    pixelsIn = IGET(*pixelsInReg);
    pixelsOut = IGET(*pixelsOutReg);
    activeDebugMask = 0x0aaabfff;
    verbose = ((status & SST_FBI_BUSY) || (debugBusy & activeDebugMask)) ? FXTRUE : FXFALSE;
    if(verbose) {
        gettimeofday(&startTv, NULL);
        fprintf(stderr, "[de10] sst1InitIdleFBINoNOP: enter sstbase=%p status=0x%08x\n",
            (void *)sstbase, status);
        fprintf(stderr, "[de10] sst1InitIdleFBINoNOP: debugBusy=0x%08x pixelsIn=%u pixelsOut=%u\n",
            debugBusy, pixelsIn, pixelsOut);
        de10DumpBusyDebug("[de10] sst1InitIdleFBINoNOP:", debugBusy);
        fprintf(stderr, "[de10] sst1InitIdleFBINoNOP: writePathDebug=0x%08x\n",
            writePathDebug);
        de10DumpWritePathDebug("[de10] sst1InitIdleFBINoNOP:", writePathDebug);
        fflush(stderr);
    }
#endif
    while(1) {
#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
        status = sst1InitReturnStatus(sstbase);
        debugBusy = IGET(*debugBusyReg);
        writePathDebug = IGET(*writePathDebugReg);
        pixelsIn = IGET(*pixelsInReg);
        pixelsOut = IGET(*pixelsOutReg);
#else
        status = sst1InitReturnStatus(sstbase);
#endif
        if(!(status & SST_FBI_BUSY)) {
            if(++cntr > 5)
                break;
        } else
            cntr = 0;
#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
        if ((verbose && ((spinCount < 8u) || ((spinCount % 1000u) == 0u))) ||
            (!verbose && ((spinCount == 1000u) || ((spinCount > 0u) && ((spinCount % 10000u) == 0u))))) {
            gettimeofday(&nowTv, NULL);
            elapsedMs = ((nowTv.tv_sec - startTv.tv_sec) * 1000L) +
                ((nowTv.tv_usec - startTv.tv_usec) / 1000L);
            fprintf(stderr,
                "[de10] sst1InitIdleFBINoNOP: spin=%u t=%ldms status=0x%08x debugBusy=0x%08x writePathDebug=0x%08x pixelsIn=%u pixelsOut=%u cntr=%u verbose=%u\n",
                spinCount, elapsedMs, status, debugBusy, writePathDebug, pixelsIn, pixelsOut, cntr, verbose);
            de10DumpBusyDebug("[de10] sst1InitIdleFBINoNOP:", debugBusy);
            de10DumpWritePathDebug("[de10] sst1InitIdleFBINoNOP:", writePathDebug);
            fflush(stderr);
        }
        if(++spinCount >= 1000000u) {
            INIT_PRINTF(("sst1InitIdleFBINoNOP(): DE10 timeout status=0x%x\n",
                status));
            break;
        }
#endif
    }
#if defined(DE10_BACKEND) || defined(SIM_BACKEND)
    if(verbose) {
        fprintf(stderr,
            "[de10] sst1InitIdleFBINoNOP: exit spin=%u cntr=%u status=0x%08x debugBusy=0x%08x writePathDebug=0x%08x pixelsIn=%u pixelsOut=%u\n",
            spinCount, cntr, status, debugBusy, writePathDebug, pixelsIn, pixelsOut);
        de10DumpBusyDebug("[de10] sst1InitIdleFBINoNOP:", debugBusy);
        de10DumpWritePathDebug("[de10] sst1InitIdleFBINoNOP:", writePathDebug);
        fflush(stderr);
    }
#endif
    return(FXTRUE);
}

/* Included so compiler doesn't optimize out loop code waiting on status bits */
FX_EXPORT FxU32 FX_CSTYLE sst1InitReturnStatus(FxU32 *sstbase)
{
    volatile Sstregs *sst = (Sstregs *) sstbase;

    return(IGET(sst->status));
}


/*
** sst1InitClearSwapPending():
**  Clear any swaps pending in the status register
**  NOTE: The video unit of FBI must be initialized before calling this routine
**
**    Returns:
**      FXTRUE
**
*/
FX_ENTRY FxBool FX_CALL sst1InitClearSwapPending(FxU32 *sstbase)
{
    volatile Sstregs *sst = (Sstregs *) sstbase;
    FxU32 displayedBuffer, i;

    INIT_PRINTF(("sst1InitClearSwapPending() WARNING: Clearing pending swapbufferCMDs...\n"));

    sst1InitIdle(sstbase);
#ifdef DE10_BACKEND
    INIT_PRINTF(("sst1InitClearSwapPending(): DE10 backend skipping retrace-dependent drain\n"));
    return(FXTRUE);
#endif

    displayedBuffer =
        (IGET(sst->status) & SST_DISPLAYED_BUFFER) >>
          SST_DISPLAYED_BUFFER_SHIFT;

    /* Wait until vsync is inactive to guarantee that swaps queue in the */
    /* PCI fifo properly */
    while(!(IGET(sst->status) & SST_VRETRACE) ||
      ((IGET(sst->vRetrace) & 0xfff) > 100) || ((IGET(sst->vRetrace) & 0xfff)
        < 10))
        ;

    /* First swap syncs to Vsync...Subsequent ones do not... */
    ISET(sst->swapbufferCMD, 0x1);
    ISET(sst->nopCMD, 0x0);
    for(i=0; i<17; i++) {
        ISET(sst->swapbufferCMD, 0x0);
        ISET(sst->nopCMD, 0x0);
    }
    if(displayedBuffer) {
        ISET(sst->swapbufferCMD, 0x0);
        ISET(sst->nopCMD, 0x0);
    }
    sst1InitIdle(sstbase);

    return(FXTRUE);
}

/*
** sst1InitVgaPassCtrl():
**  Control VGA passthrough setting
**
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitVgaPassCtrl(FxU32 *sstbase, FxU32 enable)
{
    volatile Sstregs *sst = (Sstregs *) sstbase;

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(enable) {
        /* VGA controls monitor */
        ISET(sst->fbiInit0, (IGET(sst->fbiInit0) & ~SST_EN_VGA_PASSTHRU) | 
            sst1CurrentBoard->vgaPassthruEnable);
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_BLANK_EN);
    } else {
        /* SST-1 controls monitor */
        ISET(sst->fbiInit0, (IGET(sst->fbiInit0) & ~SST_EN_VGA_PASSTHRU) | 
            sst1CurrentBoard->vgaPassthruDisable);
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) & ~SST_VIDEO_BLANK_EN);
    }

    return(FXTRUE);
}

/*
** sst1InitResetTmus():
**  Reset TMUs after changing graphics clocks
**  Occasionally when changing the frequency of the graphics clock, the TMUs
**  get in an unknown state.  sst1InitResetTmus() cleans up the problem.
**
*/
FxBool sst1InitResetTmus(FxU32 *sstbase)
{
    volatile Sstregs *sst = (Sstregs *) sstbase;
    FxU32 n;

    /* Clear FBI registers */
    ISET(sst->fbzColorPath, 0x0);
    ISET(sst->fogMode, 0x0);
    ISET(sst->alphaMode, 0x0);
    ISET(sst->fbzMode, 0x0);
    ISET(sst->lfbMode, 0x0);
    ISET(sst->fogColor, 0x0);
    ISET(sst->zaColor, 0x0);
    ISET(sst->chromaKey, 0x0);
    ISET(sst->stipple, 0x0);
    ISET(sst->c0, 0x0);
    ISET(sst->c1, 0x0);

    /* clear TMU registers */
    ISET(sst->textureMode, 0x0);
    ISET(sst->tLOD, 0x0);
    ISET(sst->tDetail, 0x0);
    ISET(sst->texBaseAddr, 0x0);
    ISET(sst->texBaseAddr1, 0x0);
    ISET(sst->texBaseAddr2, 0x0);
    ISET(sst->texBaseAddr38, 0x0);

    /* Set downstream TMU to intentionally overflow TT Fifo... */
    ISET(SST_TREX(sst,0)->trexInit1, sst1CurrentBoard->tmuInit1[0] &
        ~SST_TEX_TT_FIFO_SIL);
    sst1InitIdleFBINoNOP(sstbase);

    /* Draw 256-pixel textured triangle to overflow TT fifo in downstream */
    /* TMU.  Create numerous page misses in downstream TMU so upstream TMU */
    /* is always running faster */
    ISET(sst->fbzColorPath, SST_ENTEXTUREMAP);
    ISET(sst->fbzMode, SST_DRAWBUFFER_FRONT);
    ISET(sst->vA.x, 0);
    ISET(sst->vA.y, 0);
    ISET(sst->vB.x, (25<<SST_XY_FRACBITS));
    ISET(sst->vB.y, 0);
    ISET(sst->vC.x, 0);
    ISET(sst->vC.y, (25<<SST_XY_FRACBITS));
    ISET(sst->s, 0x0);
    ISET(sst->t, 0x0);
    ISET(sst->w, 0x0);
    ISET(sst->dwdx, 0x0);
    ISET(sst->dwdy, 0x0);
    ISET(SST_TREX(sst,0)->dsdx, (69<<SST_ST_FRACBITS));
    ISET(SST_TREX(sst,0)->dsdy, (69<<SST_ST_FRACBITS));
    ISET(SST_TREX(sst,0)->dsdx, (69<<SST_ST_FRACBITS));
    ISET(SST_TREX(sst,0)->dsdy, (69<<SST_ST_FRACBITS));
    ISET(SST_TREX(sst,1)->dsdx, (0<<SST_ST_FRACBITS));
    ISET(SST_TREX(sst,1)->dsdy, (0<<SST_ST_FRACBITS));
    ISET(SST_TREX(sst,1)->dsdx, (0<<SST_ST_FRACBITS));
    ISET(SST_TREX(sst,1)->dsdy, (0<<SST_ST_FRACBITS));
    ISET(sst->triangleCMD, 0x0);
    ISET(sst->nopCMD, 0x1);  /* This will reset pixel counter registers... */

    /* Wait for command to execute... */
    for(n=0; n<25000; n++)
        sst1InitReturnStatus(sstbase);

    /* Upstream TMU is now idle */
    /* Set downstream TMU to always accept upstream data */
    /* This will flush pending pixels in the downstream TMU */
    ISET(SST_TREX(sst,0)->trexInit1, sst1CurrentBoard->tmuInit1[0] |
        SST_TEX_RG_TTCII_INH | SST_TEX_USE_RG_TTCII_INH);
    for(n=0; n<100; n++)
        sst1InitReturnStatus(sstbase);

    /* Wait for command to execute... */
    for(n=0; n<25000; n++)
        sst1InitReturnStatus(sstbase);

    /* Restore registers */
    ISET(SST_TREX(sst,0)->trexInit1, sst1CurrentBoard->tmuInit1[0]);
    ISET(sst->fbzColorPath, 0x0);
    ISET(sst->fbzMode, 0x0);
    for(n=0; n<100; n++)
        sst1InitReturnStatus(sstbase);

    if(sst1InitReturnStatus(sstbase) & SST_TREX_BUSY) {
        INIT_PRINTF(("sst1InitResetTmus(): Could not reset TMUs...\n"));
        return(FXFALSE);
    } else
        return(FXTRUE);
}

/*
** sst1InitWrite32():
**  Write 32-bit Word to specified address
**
*/
FX_EXPORT void FX_CSTYLE sst1InitWrite32(FxU32 *addr, FxU32 data)
{
    P6FENCE;
    *(volatile FxU32 *)addr = data;
    P6FENCE;
}

/*
** sst1InitRead32():
**  Read 32-bit Word from specified address
**
*/
FX_EXPORT FxU32 FX_CSTYLE sst1InitRead32(FxU32 *addr)
{
    P6FENCE;
    return(*(volatile FxU32 *)addr);
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
