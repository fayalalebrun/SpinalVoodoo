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
#include <sst.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <sst1vid.h>
#include <sst1init.h>

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
    return(FXTRUE);
}

void sst1InitIdleLoop(FxU32 *sstbase)
{
    FxU32 cntr;
    volatile Sstregs *sst = (Sstregs *) sstbase;

    ISET(sst->nopCMD, 0x0);
    cntr = 0;
    while(1) {
        if(!(sst1InitReturnStatus(sstbase) & SST_BUSY)) {
            if(++cntr >= 3)
                break;
        } else
            cntr = 0;
    }
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

    if(!sst)
        return(FXFALSE);

    /* ISET(sst->nopCMD, 0x0); */
    cntr = 0;
    while(1) {
        if(!(sst1InitReturnStatus(sstbase) & SST_FBI_BUSY)) {
            if(++cntr > 5)
                break;
        } else
            cntr = 0;
    }
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
    *addr = data;
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
    return(*addr);
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
