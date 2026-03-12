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
** Initialization code for initializing supported SST-1 DACs
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
** sst1InitDacRd():
**  Read external DAC registers
**  NOTE: The video unit of FBI must be in reset before calling this routine.
**        The rendering engines of FBI and TREX must be idle before calling 
**        this routine.
**        fbiInit23 register remapping (PCI config. initEnable[2]=1) must be
**        enabled before calling this routine
**
**  Valid addresses are 0 <= addr <= 3
**
*/
FX_EXPORT FxU32 FX_CSTYLE sst1InitDacRd(FxU32 *sstbase, FxU32 addr)
{
    volatile Sstregs *sst = (Sstregs *) sstbase;
    FxU32 retVal;
    static FxBool firstPass = FXTRUE;
    static int helper;

    if(firstPass == FXTRUE) {
        firstPass = FXFALSE;
        helper = (GETENV(("SST_DEBUGDAC"))) ? 1 : 0;
    }

    ISET(sst->dacData, ((addr) << SST_DACDATA_ADDR_SHIFT) | SST_DACDATA_RD);
    sst1InitIdleFBINoNOP(sstbase);
    retVal = IGET(sst->fbiInit2) & SST_DACDATA_DATA;
    if(helper)
        INIT_PRINTF(("dacRd(0x%x,0x%x)\n", addr, retVal));

    return(retVal);
}

/*
** sst1InitDacWr():
**  Write to external DAC registers
**  NOTE: The video unit of FBI must be in reset before calling this routine.
**        The rendering engines of FBI and TREX must be idle before calling 
**        this routine.
**
**  Valid addresses are 0 <= addr <= 3
**
*/
FX_EXPORT void FX_CSTYLE sst1InitDacWr(FxU32 *sstbase, FxU32 addr, FxU32 data)
{
    volatile Sstregs *sst = (Sstregs *) sstbase;
    static FxBool firstPass = FXTRUE;
    static int helper;

    if(firstPass == FXTRUE) {
        firstPass = FXFALSE;
        helper = (GETENV(("SST_DEBUGDAC"))) ? 1 : 0;
    }

    ISET(sst->dacData,
        (data & SST_DACDATA_DATA) |
        ((addr) << SST_DACDATA_ADDR_SHIFT) |
        SST_DACDATA_WR);
    sst1InitIdleFBINoNOP(sstbase);

    if(helper)
        INIT_PRINTF(("dacWr(0x%x,0x%x)\n", addr, data));
}

/*
** sst1InitExecuteDacRdWr():
**  Execute DAC read/write command sequence defined in "voodoo.ini" file
**
*/
FxBool sst1InitExecuteDacRdWr(FxU32 *sstbase, sst1InitDacRdWrStruct
  *dacRdWrBase)
{
    sst1InitDacRdWrStruct *dacRdWrPtr = dacRdWrBase;
    FxU32 data;
    FxBool retVal = FXTRUE;
    int helper = (GETENV(("SST_DEBUGDAC"))) ? 1 : 0;

    while(dacRdWrPtr) {
        if(dacRdWrPtr->type == DACRDWR_TYPE_WR) {
            sst1InitDacWr(sstbase, dacRdWrPtr->addr, dacRdWrPtr->data);
        } else if(dacRdWrPtr->type == DACRDWR_TYPE_RDMODWR) {
            data = sst1InitDacRd(sstbase, dacRdWrPtr->addr);
            if(helper)
                INIT_PRINTF(("dacRdWr(Read=0x%x)\n", data));
            data &= dacRdWrPtr->mask;
            sst1InitDacWr(sstbase, dacRdWrPtr->addr, (data | dacRdWrPtr->data));
        } else if(dacRdWrPtr->type == DACRDWR_TYPE_WRMOD_POP) {
            data = iniStack[--iniStackPtr];
            if(iniStackPtr < 0) {
                retVal = FXFALSE;
                break;
            }
            if(helper)
                INIT_PRINTF(("dacWrModPop(Stack=0x%x,Mask=0x%x)\n", data,
                    dacRdWrPtr->mask));
            data &= dacRdWrPtr->mask;
            sst1InitDacWr(sstbase, dacRdWrPtr->addr, (data | dacRdWrPtr->data));
        } else if(dacRdWrPtr->type == DACRDWR_TYPE_RDNOCHECK) {
            sst1InitDacRd(sstbase, dacRdWrPtr->addr);
        } else if(dacRdWrPtr->type == DACRDWR_TYPE_RDPUSH) {
            data = sst1InitDacRd(sstbase, dacRdWrPtr->addr);
            iniStack[iniStackPtr++] = data;
            if(iniStackPtr == DACRDWR_MAX_PUSH) {
                retVal = FXFALSE;
                break;
            }
        } else if(dacRdWrPtr->type == DACRDWR_TYPE_RDCHECK) {
            if(sst1InitDacRd(sstbase, dacRdWrPtr->addr) != dacRdWrPtr->data) {
                retVal = FXFALSE;
                break;
            }
        } else {
            retVal = FXFALSE;
            break;
        }
        dacRdWrPtr = dacRdWrPtr->nextRdWr;
    }
    return(retVal);
}

/*
** sst1InitDacDetect():
**  Detect type of on-board DAC
**  NOTE: sst1InitDacDetect() resets the PCI fifo and the graphics subsystem
**   of FBI
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitDacDetect(FxU32 * sstbase)
{
    FxU32 n;
    FxU32 fbiInit1_save;
    FxU32 fbiInit2_save;
    volatile Sstregs *sst = (Sstregs *) sstbase;
    FxBool retVal = FXFALSE;
    int helper = (GETENV(("SST_DEBUGDAC"))) ? 1 : 0;

    if(!sst)
        return(FXFALSE);

    if(helper)
        INIT_PRINTF(("sst1InitDacDetect(): Entered...\n"));

    sst1InitIdleFBINoNOP(sstbase);

    /* Save init register states before overwriting them */
    fbiInit1_save = IGET(sst->fbiInit1);
    fbiInit2_save = IGET(sst->fbiInit2);

    /* Reset video unit to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    /* Turn off dram refresh to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) & ~SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable reads from the DAC (multiplexed on the fbiInit2 address) */
    /* Disallow writes to pass through the PCI FIFO */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_FBIINIT23_REMAP);
    sst1InitIdleFBINoNOP(sstbase);

    if(sst1InitUseVoodooFile == FXTRUE) {
        retVal = sst1InitDacDetectINI(sstbase);
    } else {
        if((retVal = sst1InitDacDetectICS(sstbase)) == FXTRUE)
            goto done;
        if((retVal = sst1InitDacDetectATT(sstbase)) == FXTRUE)
            goto done;
        retVal = sst1InitDacDetectTI(sstbase);
    }

done:
    /* Disable fbiinit23 address remapping */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_PCI_FIFOWR_EN);

    /* Restore init register states */
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit1, fbiInit1_save);
    ISET(sst->fbiInit2, fbiInit2_save);
    sst1InitIdleFBINoNOP(sstbase);
    return(retVal);
}

/*
** sst1InitDacDetectINI():
**  Detect DAC based on rules supplied in "voodoo.ini"
**
*/
FxBool sst1InitDacDetectINI(FxU32 * sstbase)
{
    sst1InitDacStruct *dacPtr;
    FxU32 j;
    FxBool retVal = FXFALSE;

    dacPtr = dacStructBase;
    while(dacPtr) {
        if(dacPtr->detect) {
            /* Loop multiple times, as some DACs go into never-never land... */
            for(j=0; j<100; j++) {
                if((retVal = sst1InitExecuteDacRdWr(sstbase, dacPtr->detect))
                  == FXTRUE) {
                    iniDac = dacPtr;
                    break;
                }
            }
            if(retVal == FXTRUE)
                break;
        }
        dacPtr = dacPtr->nextDac;
    }
    return(retVal);
}

FxBool sst1InitDacDetectATT(FxU32 * sstbase)
{
    FxU32 n, j, dacmir, dacdir;
    int helper = (GETENV(("SST_DEBUGDAC"))) ? 1 : 0;

    if(helper)
        INIT_PRINTF(("sst1InitDacDetectATT(): Entered...\n"));

    /* Detect ATT */
    /* Sometimes the DACs seem to go into never-never land, so */
    /* try and initialize the DAC multiple times */
    n = 0;
    while(1) {
        if(++n > 100)
            break;

        /* Must guarantee that no rendering is being performed */
        sst1InitIdleFBINoNOP(sstbase);

        /* Reset current state of DAC command register 0 (CR0) via the read */
        /* mask register (RMR) */
        sst1InitDacWr(sstbase, SST_DACREG_WMA, 0x0); /* reset backdoor fsm */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);      /* RMR must be read 5 times */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);      /* Returns contents of CR0 */

        /* Enable indexed programming by setting CR0[0] = 1 */
        sst1InitDacWr(sstbase, SST_DACREG_WMA, 0x0); /* reset backdoor fsm */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);      /* RMR must be read 4 times */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacWr(sstbase, SST_DACREG_RMR,
            SST_DACREG_CR0_INDEXED_ADDRESSING | SST_DACREG_CR0_8BITDAC);

        /* NOW DAC IS IN INDEXED ADDRESS MODE... */

        /* Check the manufacturing ID */
        j = 0;
        DAC_INDEXWRADDR(SST_DACREG_INDEX_MIR);
        dacmir = DAC_INDEXRD();
        if(dacmir == SST_DACREG_INDEX_MIR_ATT_DEFAULT)
            j++;
        else
            continue;

        /* Check the device ID */
        DAC_INDEXWRADDR(SST_DACREG_INDEX_DIR);
        dacdir = DAC_INDEXRD();
        if(dacmir == SST_DACREG_INDEX_MIR_ATT_DEFAULT &&
           dacdir == SST_DACREG_INDEX_DIR_ATT_DEFAULT)
            j++;
        else
            continue;
        if(j == 2) {
            /* Found ATT DAC... */
            sst1CurrentBoard->fbiDacType = SST_FBI_DACTYPE_ATT;
            sst1InitIdleFBINoNOP(sstbase);
            /* Disable indexed-mode addressing */
            sst1InitDacWr(sstbase, SST_DACREG_INDEXADDR, SST_DACREG_INDEX_CR0);
            sst1InitDacWr(sstbase, SST_DACREG_INDEXDATA,
                sst1InitDacRd(sstbase, SST_DACREG_INDEXDATA) &
                ~SST_DACREG_CR0_INDEXED_ADDRESSING);
           if(helper)
               INIT_PRINTF(("sst1InitDacDetectATT(): Exiting...\n"));
            return(FXTRUE);
        }
    }
    if(helper)
        INIT_PRINTF(("sst1InitDacDetectATT(): Exiting...\n"));
    return(FXFALSE);
}

FxBool sst1InitDacDetectTI(FxU32 * sstbase)
{
    FxU32 n, j, dacmir, dacdir;
    int helper = (GETENV(("SST_DEBUGDAC"))) ? 1 : 0;

    if(helper)
        INIT_PRINTF(("sst1InitDacDetectTI(): Entered...\n"));

    /* Detect TI */
    /* Sometimes the DACs seem to go into never-never land, so */
    /* try and initialize the DAC multiple times */
    n = 0;
    while(1) {
        if(++n > 100)
            break;

        /* Must guarantee that no rendering is being performed */
        sst1InitIdleFBINoNOP(sstbase);

        /* Reset current state of DAC command register 0 (CR0) via the read */
        /* mask register (RMR) */
        sst1InitDacWr(sstbase, SST_DACREG_WMA, 0x0); /* reset backdoor fsm */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);      /* RMR must be read 5 times */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);      /* Returns contents of CR0 */

        /* Enable indexed programming by setting CR0[0] = 1 */
        sst1InitDacWr(sstbase, SST_DACREG_WMA, 0x0); /* reset backdoor fsm */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);      /* RMR must be read 4 times */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacWr(sstbase, SST_DACREG_RMR,
            SST_DACREG_CR0_INDEXED_ADDRESSING | SST_DACREG_CR0_8BITDAC);

        /* NOW DAC IS IN INDEXED ADDRESS MODE... */

        /* Check the manufacturing ID */
        j = 0;
        DAC_INDEXWRADDR(SST_DACREG_INDEX_MIR);
        dacmir = DAC_INDEXRD();
        if(dacmir == SST_DACREG_INDEX_MIR_TI_DEFAULT)
            j++;
        else
            continue;

        /* Check the device ID */
        DAC_INDEXWRADDR(SST_DACREG_INDEX_DIR);
        dacdir = DAC_INDEXRD();
        if(dacmir == SST_DACREG_INDEX_MIR_TI_DEFAULT &&
           dacdir == SST_DACREG_INDEX_DIR_TI_DEFAULT)
            j++;
        else
            continue;
        if(j == 2) {
            /* Found TI DAC... */
            sst1CurrentBoard->fbiDacType = SST_FBI_DACTYPE_TI;
            sst1InitIdleFBINoNOP(sstbase);
            /* Disable indexed-mode addressing */
            sst1InitDacWr(sstbase, SST_DACREG_INDEXADDR, SST_DACREG_INDEX_CR0);
            sst1InitDacWr(sstbase, SST_DACREG_INDEXDATA,
                sst1InitDacRd(sstbase, SST_DACREG_INDEXDATA) &
                ~SST_DACREG_CR0_INDEXED_ADDRESSING);
            if(helper)
                INIT_PRINTF(("sst1InitDacDetectTI(): Exiting...\n"));
            return(FXTRUE);
        }
    }
    if(helper)
        INIT_PRINTF(("sst1InitDacDetectTI(): Exiting...\n"));
    return(FXFALSE);
}

FxBool sst1InitDacDetectICS(FxU32 * sstbase)
{
    FxU32 n;
    int helper = (GETENV(("SST_DEBUGDAC"))) ? 1 : 0;

    if(helper)
        INIT_PRINTF(("sst1InitDacDetectICS(): Entered...\n"));

    /* Detect ICS... */
    /* Sometimes the DACs seem to go into never-never land, so */
    /* try and initialize the DAC multiple times */
    n = 0;
    while(1) {
        FxU32 gclk1, vclk1, vclk7;

        if(++n > 100)
            break;
        /* Must guarantee that no rendering is being performed */
        sst1InitIdleFBINoNOP(sstbase);

        sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_RD,
            SST_DACREG_ICS_PLLADDR_GCLK1);
        gclk1 = sst1InitDacRd(sstbase,  SST_DACREG_ICS_PLLADDR_DATA);
        sst1InitDacRd(sstbase,  SST_DACREG_ICS_PLLADDR_DATA);
        sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_RD,
            SST_DACREG_ICS_PLLADDR_VCLK1);
        vclk1 = sst1InitDacRd(sstbase,  SST_DACREG_ICS_PLLADDR_DATA);
        sst1InitDacRd(sstbase,  SST_DACREG_ICS_PLLADDR_DATA);
        sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_RD,
            SST_DACREG_ICS_PLLADDR_VCLK7);
        vclk7 = sst1InitDacRd(sstbase,  SST_DACREG_ICS_PLLADDR_DATA);
        sst1InitDacRd(sstbase,  SST_DACREG_ICS_PLLADDR_DATA);
        if((gclk1 == SST_DACREG_ICS_PLLADDR_GCLK1_DEFAULT) &&
           (vclk1 == SST_DACREG_ICS_PLLADDR_VCLK1_DEFAULT) &&
           (vclk7 == SST_DACREG_ICS_PLLADDR_VCLK7_DEFAULT)) {
            /* found ICS DAC... */
            sst1CurrentBoard->fbiDacType = SST_FBI_DACTYPE_ICS;
            if(helper)
                INIT_PRINTF(("sst1InitDacDetectICS(): Exiting...\n"));
            return(FXTRUE);
        }
    }
    if(helper)
        INIT_PRINTF(("sst1InitDacDetectICS(): Exiting...\n"));
    return(FXFALSE);
}

/*
** sst1InitCalcGrxClk():
**  Determine graphics clock frequency
**  NOTE: sst1InitCalcGrxClk() must be called prior to sst1InitGrxClk()
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitCalcGrxClk(FxU32 *sstbase)
{
    FxU32 clkFreq;
    const char *e;

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    e = GETENV(("SST_GRXCLK"));
    if(e) {
        INIT_PRINTF(("sst1InitCalcGrxClk(): Overriding default clk frequency with SST_GRXCLK\n"));
        clkFreq = ATOI(e);
        if(clkFreq < 16)
            clkFreq = 16;
    } else {
        /* Setup memory clock frequency based on board strapping bits... */
        if(sst1CurrentBoard->fbiBoardID == 0x0) {
            /* Obsidian GE Fab */
            clkFreq = 40 + (((sst1CurrentBoard->tmuConfig >> 3) & 0x7) << 2) +
                (sst1CurrentBoard->fbiConfig & 0x3);
        } else {
            /* Obsidian Pro Fab */
            clkFreq = 40 + (((sst1CurrentBoard->tmuConfig >> 3) & 0x7) << 2) +
                (sst1CurrentBoard->fbiConfig & 0x3);
            /* Fix for legacy/existing boards */
            if(clkFreq == 54)
                clkFreq = 50;
        }
    }
    sst1CurrentBoard->grxClkFreq = clkFreq;
    return(FXTRUE);
}

/*
** sst1InitGrxClk():
**  Initialize FBI and TREX Memory clocks
**  NOTE: sst1InitCalcGrxClk() must be called prior to sst1InitGrxClk()
**  NOTE: sst1InitGrxClk() resets the PCI fifo and the graphics subsystem of FBI
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitGrxClk(FxU32 *sstbase)
{
    sst1ClkTimingStruct sstGrxClk;

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);
    if(sst1CurrentBoard->initGrxClkDone)
        return(FXTRUE);
    sst1CurrentBoard->initGrxClkDone = 1;

    INIT_PRINTF(("sst1InitGrxClk(): Setting up %d MHz Graphics Clock...\n",
        sst1CurrentBoard->grxClkFreq));
    if(sst1InitComputeClkParams((float) sst1CurrentBoard->grxClkFreq,
      &sstGrxClk) == FXFALSE)
        return(FXFALSE);

    return(sst1InitSetGrxClk(sstbase, &sstGrxClk));
}

/*
** sst1InitComputeClkParams():
**  Compute PLL parameters for given clock frequency
**
*/
FxBool sst1InitComputeClkParams(float freq, sst1ClkTimingStruct
    *clkTiming)
{
    float vcoFreqDivide = 0.0f, freqMultRatio, clkError;
    float clkErrorMin;
    FxU32 p, n, m, nPlusTwo;
    int mPlusTwo;

    /* Calculate P parameter */
    p = 4;
    if(((freq * (float) 1.) >= (float) 120.) &&
       ((freq * (float) 1.) <= (float) 240.)) {
           vcoFreqDivide = (float) 1.;
           p = 0;
    }
    if(((freq * (float) 2.) >= (float) 120.) &&
       ((freq * (float) 2.) <= (float) 240.)) {
           vcoFreqDivide = (float) 2.;
           p = 1;
    }
    if(((freq * (float) 4.) >= (float) 120.) &&
       ((freq * (float) 4.) <= (float) 240.)) {
           vcoFreqDivide = (float) 4.;
           p = 2;
    }
    if(((freq * (float) 8.) >= (float) 120.) &&
       ((freq * (float) 8.) <= (float) 240.)) {
           vcoFreqDivide = (float) 8.;
           p = 3;
    }
    if(p > 3)
        return(FXFALSE);

    /* Divide by 14.318 */
    freqMultRatio = (freq * vcoFreqDivide) * (float) 0.06984216;

    /* Calculate proper N and M parameters which yield the lowest error */
    clkErrorMin = (float) 9999.; n = 0; m = 0;
    for(nPlusTwo = 3; nPlusTwo < 32; nPlusTwo++) {
#ifdef DIRECTX
        mPlusTwo = FTOL( (((float) nPlusTwo * freqMultRatio) + (float) 0.5) );
        clkError = ((float) mPlusTwo * ITOF_INV( nPlusTwo ) ) - freqMultRatio;
#else
        mPlusTwo = (int) (((float) nPlusTwo * freqMultRatio) + (float) 0.5);
        clkError = ((float) mPlusTwo / (float) nPlusTwo) - freqMultRatio;
#endif
        if(clkError < (float) 0.0)
            clkError = -clkError;
        if((clkError < clkErrorMin) && ((mPlusTwo - 2) < 127)) {
            clkErrorMin = clkError;
            n = nPlusTwo - 2;
            m = mPlusTwo - 2;
        }
    }
    if(n == 0)
        return(FXFALSE);

    clkTiming->freq = freq;
    clkTiming->clkTiming_M = m;
    clkTiming->clkTiming_P = p;
    clkTiming->clkTiming_N = n;
    if(freq < (float) 37.) {
        clkTiming->clkTiming_L = 0xa;
        clkTiming->clkTiming_IB = 0x6;
    } else if(freq < (float) 45.) {
        clkTiming->clkTiming_L = 0xc;
        clkTiming->clkTiming_IB = 0x4;
    } else if(freq < (float) 58.) {
        clkTiming->clkTiming_L = 0x8;
        clkTiming->clkTiming_IB = 0x4;
    } else if(freq < (float) 66.) {
        clkTiming->clkTiming_L = 0xa;
        clkTiming->clkTiming_IB = 0x6;
    } else {
        clkTiming->clkTiming_L = 0xa;
        clkTiming->clkTiming_IB = 0x8;
    }
#if 0
    {
        float calc;
        
        calc = ((float) 14.318 * (float) (m + 2)) /
            ((float) (n + 2) * vcoFreqDivide);
        printf("freq:%.3f  calc:%.3f\n", freq, calc);
        printf("m:%d p:%d n:%d\n", m, p, n);
        fflush(stdout);
    }
#endif
    return(FXTRUE);
}

/*
** sst1InitSetVidClkATT():
**  Set video clock for ATT Dacs
**
*/
FxBool sst1InitSetVidClkATT(FxU32 *sstbase, sst1ClkTimingStruct
    *sstVidClk)
{
    if(sst1InitDacIndexedEnable(sstbase, 1) == FXFALSE)
        return(FXFALSE);

    DAC_INDEXWRADDR(SST_DACREG_INDEX_AD0);
    DAC_INDEXWR((sstVidClk->clkTiming_M) << SST_DACREG_CLKREG_MSHIFT);
    DAC_INDEXWRADDR(SST_DACREG_INDEX_AD1);
    DAC_INDEXWR(((sstVidClk->clkTiming_P) << SST_DACREG_CLKREG_PSHIFT) |
        ((sstVidClk->clkTiming_N)
           << SST_DACREG_CLKREG_NSHIFT));
    DAC_INDEXWRADDR(SST_DACREG_INDEX_AD2);
    DAC_INDEXWR(((sstVidClk->clkTiming_L) << SST_DACREG_CLKREG_LSHIFT) |
        ((sstVidClk->clkTiming_IB) << SST_DACREG_CLKREG_IBSHIFT));
    DAC_INDEXWRADDR(SST_DACREG_INDEX_CC);
    DAC_INDEXWR((DAC_INDEXRD() | (0x3 << SST_DACREG_CC_ACLK_SEL_SHIFT) |
        SST_DACREG_CC_ACLK_SELECT_AD));
    DAC_INDEXWRADDR(SST_DACREG_INDEX_CC);

    if(sst1InitDacIndexedEnable(sstbase, 0) == FXFALSE)
        return(FXFALSE);

    return(FXTRUE);
}

/*
** sst1InitSetVidClkICS():
**  Set video clock for ICS Dacs
**
*/
FxBool sst1InitSetVidClkICS(FxU32 *sstbase, sst1ClkTimingStruct
    *sstVidClk)
{
    FxU32 n, fbiInit1_save, fbiInit2_save, pllCtrl;
    volatile Sstregs *sst = (Sstregs *) sstbase;

    sst1InitIdleFBINoNOP(sstbase);

    /* Save init register states before overwriting them */
    fbiInit1_save = IGET(sst->fbiInit1);
    fbiInit2_save = IGET(sst->fbiInit2);

    /* Reset video unit to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    /* Turn off dram refresh to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) & ~SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable reads from the DAC (multiplexed on the fbiInit2 address) */
    /* Disallow writes to pass through the PCI FIFO */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_FBIINIT23_REMAP);
    sst1InitIdleFBINoNOP(sstbase);

    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_RD,
        SST_DACREG_ICS_PLLADDR_CTRL);
    pllCtrl = sst1InitDacRd(sstbase,  SST_DACREG_ICS_PLLADDR_DATA);

    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_WR,
        SST_DACREG_ICS_PLLADDR_VCLK0);
    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_DATA,
        sstVidClk->clkTiming_M);
    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_DATA,
        (((sstVidClk->clkTiming_P) << 5) | sstVidClk->clkTiming_N));
    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_WR,
        SST_DACREG_ICS_PLLADDR_CTRL);
    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_DATA,
        ((pllCtrl & ~SST_DACREG_ICS_PLLCTRL_CLK0FREQ) |
          SST_DACREG_ICS_PLLCTRL_CLK0SEL));
    sst1InitIdleFBINoNOP(sstbase);

    /* Disable fbiinit23 address remapping */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_PCI_FIFOWR_EN);

    /* Restore init register states */
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit1, fbiInit1_save);
    ISET(sst->fbiInit2, fbiInit2_save);
    sst1InitIdleFBINoNOP(sstbase);

    return(FXTRUE);
}

/*
** sst1InitSetVidClkINI():
**  Set video clock for DACs defined in "voodoo.ini"
**
*/
FxBool sst1InitSetVidClkINI(FxU32 *sstbase, FxU32 width,
  FxU32 height, FxU32 refresh, FxU32 video16BPP)
{
    FxU32 n, fbiInit1_save, fbiInit2_save;
    sst1InitDacSetVideoStruct *setVideo;
    volatile Sstregs *sst = (Sstregs *) sstbase;
    int helper = (GETENV(("SST_DEBUGDAC"))) ? 1 : 0;
    FxBool retVal = FXFALSE;

    if(helper)
        INIT_PRINTF(("sst1InitSetVidClkINI(): Entered...\n"));

    if(iniDac == (sst1InitDacStruct *) NULL)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    sst1InitIdleFBINoNOP(sstbase);

    /* Save init register states before overwriting them */
    fbiInit1_save = IGET(sst->fbiInit1);
    fbiInit2_save = IGET(sst->fbiInit2);

    /* Reset video unit to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    /* Turn off dram refresh to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) & ~SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable reads from the DAC (multiplexed on the fbiInit2 address) */
    /* Disallow writes to pass through the PCI FIFO */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_FBIINIT23_REMAP);
    sst1InitIdleFBINoNOP(sstbase);

    setVideo = iniDac->setVideo;
    while(setVideo) {
        if((setVideo->width == width) && (setVideo->height == height) &&
           (setVideo->refresh == refresh) &&
           (setVideo->video16BPP == video16BPP)) {
            if((retVal = sst1InitExecuteDacRdWr(sstbase,
              setVideo->setVideoRdWr)) == FXTRUE) {
                retVal = FXTRUE;
                break;
            }
        }
        setVideo = setVideo->nextSetVideo;
    }

    /* Disable fbiinit23 address remapping */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_PCI_FIFOWR_EN);

    /* Restore init register states */
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit1, fbiInit1_save);
    ISET(sst->fbiInit2, fbiInit2_save);
    sst1InitIdleFBINoNOP(sstbase);

    return(retVal);
}

/*
** sst1InitSetGrxClkATT():
**  Set graphics clock for ATT Dacs
**  NOTE: sst1InitSetGrxClkATT() resets the PCI fifo and the graphics subsystem
**  of FBI
**
*/
FxBool sst1InitSetGrxClkATT(FxU32 *sstbase, sst1ClkTimingStruct
    *sstGrxClk)
{
    FxU32 n;
    volatile Sstregs *sst = (Sstregs *) sstbase;

    /* Reset graphics unit before we change grx clk */
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) |
      (SST_GRX_RESET | SST_PCI_FIFO_RESET));
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable DAC indexed addressing */
    /* sst1InitDacIndexedEnable resets the video module, turns off dram refresh */
    /* and disallows writes to the PCI fifo */
    if(sst1InitDacIndexedEnable(sstbase, 1) == FXFALSE)
        return(FXFALSE);

    DAC_INDEXWRADDR(SST_DACREG_INDEX_BD0);
    DAC_INDEXWR((sstGrxClk->clkTiming_M) << SST_DACREG_CLKREG_MSHIFT);
    DAC_INDEXWRADDR(SST_DACREG_INDEX_BD1);
    DAC_INDEXWR(((sstGrxClk->clkTiming_P) << SST_DACREG_CLKREG_PSHIFT) |
                ((sstGrxClk->clkTiming_N) << SST_DACREG_CLKREG_NSHIFT));
    DAC_INDEXWRADDR(SST_DACREG_INDEX_BD2);
    DAC_INDEXWR(((sstGrxClk->clkTiming_L) << SST_DACREG_CLKREG_LSHIFT) |
                ((sstGrxClk->clkTiming_IB) << SST_DACREG_CLKREG_IBSHIFT));
    DAC_INDEXWRADDR(SST_DACREG_INDEX_CC);
    DAC_INDEXWR((DAC_INDEXRD() | (0x3 << SST_DACREG_CC_BCLK_SEL_SHIFT) |
        SST_DACREG_CC_BCLK_SELECT_BD));
    sst1InitIdleFBINoNOP(sstbase);

    /* Turn off DAC indexed addressing */

    /* Disabling dac indexed mode re-enables writes to pass through the */
    /* PCI fifo (and restores video refresh and dram refresh if previously */
    /* enabled) */
    if(sst1InitDacIndexedEnable(sstbase, 0) == FXFALSE)
        return(FXFALSE);

    /* Wait for graphics clock to stabilize */
    for(n=0; n<200000; n++)
        sst1InitReturnStatus(sstbase);

    /* Unreset PCI FIFO and graphic subsystem */
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_PCI_FIFO_RESET);
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_GRX_RESET);
    sst1InitIdleFBINoNOP(sstbase);

    return(FXTRUE);
}

/*
** sst1InitSetGrxClkICS():
**  Set graphics clock for ICS Dacs
**  NOTE: sst1InitSetGrxClkICS() resets the PCI fifo and the graphics subsystem
**  of FBI
**
*/
FxBool sst1InitSetGrxClkICS(FxU32 *sstbase, sst1ClkTimingStruct
    *sstGrxClk)
{
    FxU32 n, fbiInit1_save, fbiInit2_save, pllCtrl;
    volatile Sstregs *sst = (Sstregs *) sstbase;

    /* Reset graphics unit before we change grx clk */
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) |
      (SST_GRX_RESET | SST_PCI_FIFO_RESET));
    sst1InitIdleFBINoNOP(sstbase);

    /* Save init register states before overwriting them */
    fbiInit1_save = IGET(sst->fbiInit1);
    fbiInit2_save = IGET(sst->fbiInit2);

    /* Reset video unit to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    /* Turn off dram refresh to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) & ~SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable reads from the DAC (multiplexed on the fbiInit2 address) */
    /* Disallow writes to pass through the PCI FIFO */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_FBIINIT23_REMAP);
    sst1InitIdleFBINoNOP(sstbase);

    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_RD,
        SST_DACREG_ICS_PLLADDR_CTRL);
    pllCtrl = sst1InitDacRd(sstbase,  SST_DACREG_ICS_PLLADDR_DATA);

    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_WR,
        SST_DACREG_ICS_PLLADDR_GCLK0);
    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_DATA,
        sstGrxClk->clkTiming_M);
    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_DATA,
        (((sstGrxClk->clkTiming_P) << 5) | sstGrxClk->clkTiming_N));
    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_WR,
        SST_DACREG_ICS_PLLADDR_CTRL);
    sst1InitDacWr(sstbase, SST_DACREG_ICS_PLLADDR_DATA,
        (pllCtrl & ~SST_DACREG_ICS_PLLCTRL_CLK1SEL));
    sst1InitIdleFBINoNOP(sstbase);

    /* Disable fbiinit23 address remapping */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_PCI_FIFOWR_EN);

    /* Restore init register states */
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit1, fbiInit1_save);
    ISET(sst->fbiInit2, fbiInit2_save);
    sst1InitIdleFBINoNOP(sstbase);

    /* Wait for graphics clock to stabilize */
    for(n=0; n<200000; n++)
        sst1InitReturnStatus(sstbase);

    /* Unreset PCI FIFO and graphic subsystem */
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_PCI_FIFO_RESET);
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_GRX_RESET);
    sst1InitIdleFBINoNOP(sstbase);

    return(FXTRUE);
}

/*
** sst1InitSetGrxClkINI():
**  Set graphics clock for dac specified in "voodoo.ini" file
**  NOTE: sst1InitSetGrxClkINI() resets the PCI fifo and the graphics subsystem
**  of FBI
**
*/
FxBool sst1InitSetGrxClkINI(FxU32 *sstbase, sst1ClkTimingStruct
    *sstGrxClk)
{
    FxU32 n, fbiInit1_save, fbiInit2_save;
    sst1InitDacSetMemClkStruct *setMemClk;
    FxBool retVal = FXFALSE;
    volatile Sstregs *sst = (Sstregs *) sstbase;

    if(iniDac == (sst1InitDacStruct *) NULL)
        return(FXFALSE);

    /* Reset graphics unit before we change grx clk */
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) |
      (SST_GRX_RESET | SST_PCI_FIFO_RESET));
    sst1InitIdleFBINoNOP(sstbase);

    /* Save init register states before overwriting them */
    fbiInit1_save = IGET(sst->fbiInit1);
    fbiInit2_save = IGET(sst->fbiInit2);

    /* Reset video unit to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    /* Turn off dram refresh to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) & ~SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable reads from the DAC (multiplexed on the fbiInit2 address) */
    /* Disallow writes to pass through the PCI FIFO */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_FBIINIT23_REMAP);
    sst1InitIdleFBINoNOP(sstbase);

    setMemClk = iniDac->setMemClk;
    while(setMemClk) {
#ifdef DIRECTX
        if(setMemClk->frequency == (FxU32) FTOL(sstGrxClk->freq) ) {
#else
        if(setMemClk->frequency == (FxU32) sstGrxClk->freq) {
#endif
            if((retVal = sst1InitExecuteDacRdWr(sstbase,
              setMemClk->setMemClkRdWr)) == FXTRUE) {
                retVal = FXTRUE;
                break;
            }
        }
        setMemClk = setMemClk->nextSetMemClk;
    }
    /* Disable fbiinit23 address remapping */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_PCI_FIFOWR_EN);

    /* Restore init register states */
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit1, fbiInit1_save);

    ISET(sst->fbiInit2, fbiInit2_save);
    sst1InitIdleFBINoNOP(sstbase);

    /* Wait for graphics clock to stabilize */
    if(retVal == FXTRUE) {
        for(n=0; n<200000; n++)
            sst1InitReturnStatus(sstbase);
    }

    /* Unreset PCI FIFO and graphic subsystem */
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_PCI_FIFO_RESET);
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_GRX_RESET);
    sst1InitIdleFBINoNOP(sstbase);

    return(retVal);
}

/*
** sst1InitSetVidModeATT():
**  Set video Mode for ATT dacs
**
*/
FxBool sst1InitSetVidModeATT(FxU32 *sstbase, FxU32 video16BPP)
{
    if(sst1InitDacIndexedEnable(sstbase, 1) == FXFALSE)
        return(FXFALSE);

    /* Set 16 or 24-bit pixel output */
    if(video16BPP) {
       DAC_INDEXWRADDR(SST_DACREG_INDEX_CR0);
       DAC_INDEXWR((DAC_INDEXRD() & ~SST_DACREG_CR0_COLOR_MODE) |
           SST_DACREG_CR0_COLOR_MODE_16BPP | SST_DACREG_CR0_8BITDAC);
    } else {
       DAC_INDEXWRADDR(SST_DACREG_INDEX_CR0);
       DAC_INDEXWR((DAC_INDEXRD() & ~SST_DACREG_CR0_COLOR_MODE) |
           SST_DACREG_CR0_COLOR_MODE_24BPP | SST_DACREG_CR0_8BITDAC);
    }

    if(sst1InitDacIndexedEnable(sstbase, 0) == FXFALSE)
        return(FXFALSE);


    return(FXTRUE);
}

/*
** sst1InitSetVidModeICS():
**  Set video Mode for ICS dacs
**
*/
FxBool sst1InitSetVidModeICS(FxU32 *sstbase, FxU32 video16BPP)
{
    FxU32 n, fbiInit1_save, fbiInit2_save;
    volatile Sstregs *sst = (Sstregs *) sstbase;

    sst1InitIdleFBINoNOP(sstbase);

    /* Save init register states before overwriting them */
    fbiInit1_save = IGET(sst->fbiInit1);
    fbiInit2_save = IGET(sst->fbiInit2);

    /* Reset video unit to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    /* Turn off dram refresh to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) & ~SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable reads from the DAC (multiplexed on the fbiInit2 address) */
    /* Disallow writes to pass through the PCI FIFO */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_FBIINIT23_REMAP);
    sst1InitIdleFBINoNOP(sstbase);

    if(video16BPP)
        sst1InitDacWr(sstbase, SST_DACREG_ICS_CMD,
            SST_DACREG_ICS_COLORMODE_16BPP);
    else
        sst1InitDacWr(sstbase, SST_DACREG_ICS_CMD,
            SST_DACREG_ICS_COLORMODE_24BPP);
    sst1InitIdleFBINoNOP(sstbase);

    /* Disable fbiinit23 address remapping */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_PCI_FIFOWR_EN);

    /* Restore init register states */
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit1, fbiInit1_save);
    ISET(sst->fbiInit2, fbiInit2_save);
    sst1InitIdleFBINoNOP(sstbase);

    return(FXTRUE);
}

/*
** sst1InitSetVidModeINI():
**  Set video Mode for DACs defined in "voodoo.ini"
**
*/
FxBool sst1InitSetVidModeINI(FxU32 *sstbase, FxU32 video16BPP)
{
    FxU32 n, fbiInit1_save, fbiInit2_save;
    sst1InitDacSetVideoModeStruct *setVideoMode;
    FxBool retVal = FXFALSE;
    volatile Sstregs *sst = (Sstregs *) sstbase;

    if(iniDac == (sst1InitDacStruct *) NULL)
        return(FXFALSE);

    sst1InitIdleFBINoNOP(sstbase);

    /* Save init register states before overwriting them */
    fbiInit1_save = IGET(sst->fbiInit1);
    fbiInit2_save = IGET(sst->fbiInit2);

    /* Reset video unit to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    /* Turn off dram refresh to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) & ~SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable reads from the DAC (multiplexed on the fbiInit2 address) */
    /* Disallow writes to pass through the PCI FIFO */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_FBIINIT23_REMAP);
    sst1InitIdleFBINoNOP(sstbase);

    setVideoMode = iniDac->setVideoMode;
    while(setVideoMode) {
        if(setVideoMode->video16BPP == video16BPP) {
            if((retVal = sst1InitExecuteDacRdWr(sstbase,
              setVideoMode->setVideoModeRdWr)) == FXTRUE) {
                retVal = FXTRUE;
                break;
            }
        }
        setVideoMode = setVideoMode->nextSetVideoMode;
    }
    sst1InitIdleFBINoNOP(sstbase);

    /* Disable fbiinit23 address remapping */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_PCI_FIFOWR_EN);

    /* Restore init register states */
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit1, fbiInit1_save);
    ISET(sst->fbiInit2, fbiInit2_save);
    sst1InitIdleFBINoNOP(sstbase);

    return(retVal);
}

/*
** sst1InitDacIndexedEnable():
**  Initialize DAC for indexed-mode addressing
**  NOTE: When DAC indexed-mode addressing is enabled, video timing and
**   DRAM refresh are both reset (disabled)
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitDacIndexedEnable(FxU32 *sstbase,
  FxU32 Enable)
{
    FxU32 n, j, dacmir, dacdir, cr0_save;
    static FxU32 fbiInit1_save = 0;
    static FxU32 fbiInit2_save = 0;
    volatile Sstregs *sst = (Sstregs *) sstbase;

    if(!sst)
        return(FXFALSE);

    sst1InitIdleFBINoNOP(sstbase);
    if(!Enable) {
        /* Disable indexed-mode addressing */
        sst1InitDacWr(sstbase, SST_DACREG_INDEXADDR, SST_DACREG_INDEX_CR0);
        sst1InitDacWr(sstbase, SST_DACREG_INDEXDATA,
            sst1InitDacRd(sstbase, SST_DACREG_INDEXDATA) &
             ~SST_DACREG_CR0_INDEXED_ADDRESSING);

        /* Disable fbiinit23 address remapping */
        PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_PCI_FIFOWR_EN);

        /* Restore init register states */
        sst1InitIdleFBINoNOP(sstbase);
        if(fbiInit1_save)  /* Have we previously enabled indexed addressing? */
            ISET(sst->fbiInit1, fbiInit1_save);
        if(fbiInit2_save)  /* Have we previously enabled indexed addressing? */
            ISET(sst->fbiInit2, fbiInit2_save);
        sst1InitIdleFBINoNOP(sstbase);
        return(FXTRUE);
    }

    /* Save init register states before overwriting them */
    fbiInit1_save = IGET(sst->fbiInit1);
    fbiInit2_save = IGET(sst->fbiInit2);

    /* Reset video unit to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);
    /* Turn off dram refresh to guarantee no contentions on the memory bus */
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) & ~SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable reads from the DAC (multiplexed on the fbiInit2 address) */
    /* Disallow writes to pass through the PCI FIFO */
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN | SST_FBIINIT23_REMAP);
    sst1InitIdleFBINoNOP(sstbase);

    /* Sometimes the DACs seem to go into never-never land, so */
    /* try and initialize the DAC multiple times */
    n = 0;
    while(1) {
        if(++n > 100) {
            INIT_PRINTF(("sst1InitDacIndexedEnable() ERROR: Could not Initialize DAC\n"));
            return(FXFALSE);
        }

        /* Must guarantee that no rendering is being performed */
        sst1InitIdleFBINoNOP(sstbase);

        /* Reset current state of DAC command register 0 (CR0) via the read */
        /* mask register (RMR) */
        sst1InitDacWr(sstbase, SST_DACREG_WMA, 0x0); /* reset backdoor fsm */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);      /* RMR must be read 5 times */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        cr0_save = sst1InitDacRd(sstbase, SST_DACREG_RMR);

        /* Enable indexed programming by setting CR0[0] = 1 */
        sst1InitDacWr(sstbase, SST_DACREG_WMA, 0x0); /* reset backdoor fsm */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);      /* RMR must be read 4 times */
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacRd(sstbase, SST_DACREG_RMR);
        sst1InitDacWr(sstbase, SST_DACREG_RMR, ((cr0_save & 0xf0) |
            SST_DACREG_CR0_INDEXED_ADDRESSING | SST_DACREG_CR0_8BITDAC));

        /* NOW DAC IS IN INDEXED ADDRESS MODE... */

        /* Check the manufacturing ID for sanity */
        j = 0;
        DAC_INDEXWRADDR(SST_DACREG_INDEX_MIR);
        dacmir = DAC_INDEXRD();

        if((dacmir == SST_DACREG_INDEX_MIR_ATT_DEFAULT) ||
           (dacmir == SST_DACREG_INDEX_MIR_TI_DEFAULT))
            j++;
        else
            continue;

        /* Check the device ID for sanity */
        DAC_INDEXWRADDR(SST_DACREG_INDEX_DIR);
        dacdir = DAC_INDEXRD();
        if((dacmir == SST_DACREG_INDEX_MIR_ATT_DEFAULT &&
            dacdir == SST_DACREG_INDEX_DIR_ATT_DEFAULT) ||
           (dacmir == SST_DACREG_INDEX_MIR_TI_DEFAULT &&
            dacdir == SST_DACREG_INDEX_DIR_TI_DEFAULT))
            j++;
        else
            continue;
        if(j == 2)
            break;
    }
    sst1InitIdleFBINoNOP(sstbase);
    return(FXTRUE);
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
