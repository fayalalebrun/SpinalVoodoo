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

#include "canopus.h"

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

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(!initIdleEnabled)
        return(FXTRUE);

    if(sst1CurrentBoard->fbiLfbLocked) {
        if(sst1CurrentBoard->sliSlaveVirtAddr == (FxU32 *) NULL)
            // SLI not enabled...
            sst1InitPciFifoIdleLoop(sstbase);
        else {
            // Check idle for Master...
            sst1InitPciFifoIdleLoop(sstbase);
            // Check idle for Slave...
            sst1InitPciFifoIdleLoop(sst1CurrentBoard->sliSlaveVirtAddr);
        }
    } else {
        if(sst1CurrentBoard->sliSlaveVirtAddr == (FxU32 *) NULL)
            // SLI not enabled...
            sst1InitIdleLoop(sstbase, FXTRUE);
        else {
            // Check idle for Master...
            sst1InitIdleLoop(sstbase, FXTRUE);

            // Check idle for Slave...
            // Note that the Slave does not need another NOP command, because
            // it will snoop the NOP command sent to the Master above.
            // Sending a NOP command to the Slave also confuses the callback
            // write routines for the command fifo which are not setup to
            // handle any other base addresses other than the Master's...
            sst1InitIdleLoop(sst1CurrentBoard->sliSlaveVirtAddr, FXFALSE);
        }
    }
    return(FXTRUE);
}

static FxBool
sst1InitIdleWithTimeoutLoop(FxU32 *sstbase, FxBool issueNOP, FxU32 timeout)
{
  FxU32 cntr, loop;
  SstRegs *sst = (SstRegs *) sstbase;
  
  if(issueNOP) ISET(sst->nopCMD, 0x0);

  cntr = loop = 0;
  while(++loop < timeout) {
    if(!(sst1InitReturnStatus(sstbase) & SST_BUSY)) {
      if(++cntr >= 3)
        break;
    } else
      cntr = 0;
  }

  return (loop < timeout);
}

FxBool 
sst1InitIdleFBIWithTimeout(FxU32 *sstbase, FxBool issueNop, FxU32 timeout)
{
  int retVal;
  
  if(!sstbase)
    return(FXFALSE);
  
  if(!sst1InitCheckBoard(sstbase))
    return(FXFALSE);
  
  if(sst1CurrentBoard->sliSlaveVirtAddr == (FxU32 *) NULL) {
    // SLI not enabled...
    retVal = sst1InitIdleWithTimeoutLoop(sstbase, issueNop, timeout);
  } else {
    // Check idle for Master...
    if(!sst1InitIdleWithTimeoutLoop(sstbase, issueNop, timeout))
      return(FXFALSE);
    
    // Check idle for Slave...
    // Note that the Slave does not need another NOP command, because
    // it will snoop the NOP command sent to the Master above.
    // Sending a NOP command to the Slave also confuses the callback
    // write routines for the command fifo which are not setup to
    // handle any other base addresses other than the Master's...
    retVal = sst1InitIdleWithTimeoutLoop(sst1CurrentBoard->sliSlaveVirtAddr,
                                         FXFALSE, 
                                         timeout);
  }
  
  return retVal;
}

FX_EXPORT FxBool FX_CSTYLE sst1InitIdleNoNOP(FxU32 *sstbase)
{
    if(!sstbase)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(!initIdleEnabled)
        return(FXTRUE);

    if(sst1CurrentBoard->fbiLfbLocked) {
        if(sst1CurrentBoard->sliSlaveVirtAddr == (FxU32 *) NULL)
            // SLI not enabled...
            sst1InitPciFifoIdleLoop(sstbase);
        else {
            // Check idle for Master...
            sst1InitPciFifoIdleLoop(sstbase);
            // Check idle for Slave...
            sst1InitPciFifoIdleLoop(sst1CurrentBoard->sliSlaveVirtAddr);
        }
    } else {
        if(sst1CurrentBoard->sliSlaveVirtAddr == (FxU32 *) NULL)
            // SLI not enabled...
            sst1InitIdleLoop(sstbase, FXFALSE);
        else {
            // Check idle for Master...
            sst1InitIdleLoop(sstbase, FXFALSE);
            // Check idle for Slave...
            sst1InitIdleLoop(sst1CurrentBoard->sliSlaveVirtAddr, FXFALSE);
        }
    }
    return(FXTRUE);
}

void sst1InitIdleLoop(FxU32 *sstbase, FxBool issueNOP)
{
    FxU32 cntr;
    SstRegs *sst = (SstRegs *) sstbase;

    if(issueNOP)
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

void sst1InitPciFifoIdleLoop(FxU32 *sstbase)
{
    FxU32 cntr;

    cntr = 0;
    while(1) {
        if(((sst1InitReturnStatus(sstbase) & SST_FIFOLEVEL)) == 0x3f) {
            // Since sst1InitPciFifoIdleLoop is only called when we're trying
            // to "fake" idle during lfb locks, wait for 6 passes to give the
            // hardware time to drain
            if(++cntr >= 6)
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
    SstRegs *sst = (SstRegs *) sstbase;

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
    SstRegs *sst = (SstRegs *) sstbase;

    if(!sst)
        return(FXFALSE);

    // ISET(sst->nopCMD, 0x0);
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

// Included so compiler doesn't optimize out loop code waiting on status bits
FX_EXPORT FxU32 FX_CSTYLE sst1InitReturnStatus(FxU32 *sstbase)
{
    SstRegs *sst = (SstRegs *) sstbase;

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
    SstRegs *sst = (SstRegs *) sstbase;
    FxU32 displayedBuffer, i;

    INIT_PRINTF(("sst1InitClearSwapPending() WARNING: Clearing pending swapbufferCMDs...\n"));

    sst1InitIdle(sstbase);
    displayedBuffer =
        (IGET(sst->status) & SST_DISPLAYED_BUFFER) >>
          SST_DISPLAYED_BUFFER_SHIFT;

    // Wait until vsync is inactive to guarantee that swaps queue in the
    // PCI fifo properly
    while(!(IGET(sst->status) & SST_VRETRACE) ||
      ((IGET(sst->vRetrace) & 0xfff) > 100) || ((IGET(sst->vRetrace) & 0xfff)
        < 10))
        ;

    // First swap syncs to Vsync...Subsequent ones do not...
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
    SstRegs *sst = (SstRegs *) sstbase;

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    //if (sst1CurrentBoard->singleBrdSLISlave) {
    //  enable = 1;
    //}

    if(enable) {
        // VGA controls monitor
        ISET(sst->fbiInit0, (IGET(sst->fbiInit0) & ~SST_EN_VGA_PASSTHRU) | 
            sst1CurrentBoard->vgaPassthruEnable);
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_BLANK_EN);
    } else {
        // SST-1 controls monitor
        ISET(sst->fbiInit0, (IGET(sst->fbiInit0) & ~SST_EN_VGA_PASSTHRU) | 
            sst1CurrentBoard->vgaPassthruDisable);
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) & ~SST_VIDEO_BLANK_EN);
    }

    return(FXTRUE);
}

/*
** sst1InitResetTmus():
**  Reset TMUs
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitResetTmus(FxU32 *sstbase)
{
  volatile int delay;
  SstRegs *sst = (SstRegs *) sstbase;
  FxU32 allowTexturing = (GETENV(("SSTV2_TEXMAP_DISABLE"))) ? 0 : 1;

  if(!sst)
      return(FXFALSE);

  if(sst1InitCheckBoard(sstbase) == FXFALSE)
      return(FXFALSE);

  // Ignore stalls on FT Bus
  ISET(sst->fbiInit3, IGET(sst->fbiInit3) | SST_TEXMAP_DISABLE);
  // Delay
  for(delay=0;delay<10000;)
    delay++;

//   while(++cntr < 6) {

    INIT_PRINTF(("sst1InitResetTmus(): Reset Graphics "));

    do
    {
      INIT_PRINTF(("."));

      // Set Default initialization values for all TMUs...
      ISET(SST_TREX(sst,0)->trexInit0, sst1CurrentBoard->tmuInit0[0]);
      ISET(SST_TREX(sst,1)->trexInit0, sst1CurrentBoard->tmuInit0[1]);
      // Delay
      for(delay=0;delay<10000;)
        delay++;

      ISET(SST_TREX(sst,0)->trexInit1, sst1CurrentBoard->tmuInit1[0]);
      ISET(SST_TREX(sst,1)->trexInit1, sst1CurrentBoard->tmuInit1[1]);
      // Delay
      for(delay=0;delay<10000;)
        delay++;

      // Reset TMU FIFOs and graphics core for all TMUs...
      ISET(SST_TREX(sst,0)->trexInit1, sst1CurrentBoard->tmuInit1[0] | SST_TEX_RESET_FIFO | SST_TEX_RESET_GRX);
      ISET(SST_TREX(sst,1)->trexInit1, sst1CurrentBoard->tmuInit1[1] | SST_TEX_RESET_FIFO | SST_TEX_RESET_GRX);
      // Delay
      for(delay=0;delay<10000;)
        delay++;

      ISET(SST_TREX(sst,1)->trexInit1, sst1CurrentBoard->tmuInit1[1]);

      // Allow unreset to propagate backwards...
      // Delay
      for(delay=0;delay<10000;)
        delay++;

      ISET(SST_TREX(sst,0)->trexInit1, sst1CurrentBoard->tmuInit1[0]);
      // Delay
      for(delay=0;delay<10000;)
        delay++;

      // Reset in the downstream TMU may cause glitches in FBI's TF FIFO
      // Waiting for Idle in FBI will not stall on something in the TF FIFO,
      // as the TF FIFO empty signal is not part of FBI's "busy" bit
//      sst1InitIdleFBIWithTimeout(sstbase, FXTRUE, 100000);
      sst1InitResetFbi(sstbase);
/*
      fbiInit0 = IGET(sst->fbiInit0) | SST_GRX_RESET | SST_PCI_FIFO_RESET;

      ISET(sst->fbiInit0, fbiInit0);
      // Delay
      for(delay=0;delay<10000;)
        delay++;

      fbiInit0 &= ~SST_PCI_FIFO_RESET;
      ISET(sst->fbiInit0, fbiInit0);
      // Delay
      for(delay=0;delay<10000;)
        delay++;                 

      fbiInit0 &= ~SST_GRX_RESET;
      ISET(sst->fbiInit0, fbiInit0);
      // Delay
      for(delay=0;delay<10000;)
        delay++;
*/

      if(allowTexturing)
      {
        ISET(sst->fbiInit3, IGET(sst->fbiInit3) & ~SST_TEXMAP_DISABLE);
        for(delay=0;delay<10000;)
          delay++;
      }

      delay = 0;

      while( sst1InitReturnStatus(sstbase) & (SST_TMU_BUSY | SST_FBI_BUSY) && (delay < 50000) )
        delay++;

    }while( delay == 50000 );

    INIT_PRINTF(("\n"));
        
//         INIT_PRINTF(("sst1InitResetTmus(): Could not reset graphics (0x%08lx)\n", ret));
//      else
//         break;
//   }
//   if(cntr == 6) {
//       INIT_PRINTF(("sst1InitResetTmus(): Could not reset TMUs...\n"));
//       return(FXFALSE);
//   }

   // Fix problem where first Texture downloads to TMU weren't being
   //  received properly
   ISET(*(FxI32 *) (0xf00000 + (long) sstbase), 0xdeadbeef);
   sst1InitIdle(sstbase);

   return(FXTRUE);
}

/*
** sst1InitResetFbi():
**  Reset FBI graphics engine and frontend PCI Fifo...
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitResetFbi(FxU32 *sstbase)
{
  volatile int delay;
//  int i;
  FxU32 fbiInit0;
  SstRegs *sst = (SstRegs *) sstbase;
/*
  for(i=0;i<20000;i++) sst1InitReturnStatus(sstbase);
  ISET(sst->fbiInit0, IGET(sst->fbiInit0) | SST_GRX_RESET | SST_PCI_FIFO_RESET);
  for(i=0;i<20000;i++) sst1InitReturnStatus(sstbase);
  ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_PCI_FIFO_RESET);
  for(i=0;i<20000;i++) sst1InitReturnStatus(sstbase);
  ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_GRX_RESET);
  for(i=0;i<20000;i++) sst1InitReturnStatus(sstbase);
*/
  fbiInit0 = IGET(sst->fbiInit0) | SST_GRX_RESET | SST_PCI_FIFO_RESET;

  ISET(sst->fbiInit0, fbiInit0);
  // Delay
  for(delay=0;delay<10000;)
    delay++;

  fbiInit0 &= ~SST_PCI_FIFO_RESET;
  ISET(sst->fbiInit0, fbiInit0);
  // Delay
  for(delay=0;delay<10000;)
    delay++;                 

  fbiInit0 &= ~SST_GRX_RESET;
  ISET(sst->fbiInit0, fbiInit0);
  // Delay
  for(delay=0;delay<10000;)
    delay++;

  return(FXTRUE);
}

#if SET_BSWAP
#if __POWERPC__ && defined(__MWERKS__)
#define GET(s)       __lwbrx(s, 0 )
#define SET(d, s)    __stwbrx((s), (d), 0);
#else /* !__MWERKS__ */
#error "Define byte swapped macros for GET/SET"
#endif /* !__MWERKS__ */
#else /* !SET_BSWAP */
#ifdef GET
#undef GET
#endif
#ifdef SET
#undef SET
#endif
#define GET(s)       (*s)
#define SET(d, s)    ((*d) = s)
#endif /* !SET_BSWAP */

/*
** sst1InitWrite32():
**  Write 32-bit Word to specified address
**
*/
FX_EXPORT void FX_CSTYLE sst1InitWrite32(FxU32 *addr, FxU32 data)
{
  /* If the client software is using the command fifo then they are
   * responsible for passing a callback that can be used to put
   * register writes from the init code into the command fifo that
   * they are managing. However, some registers cannot be accessed via
   * the command fifo, and, inconveniently, these are not contiguously
   * allocated.  
   */
  const FxU32 addrOffset = ((const unsigned long)addr - (const unsigned long)sst1CurrentBoard->virtAddr[0]);
  FxBool directWriteP = ((sst1CurrentBoard == NULL) ||
                         (sst1CurrentBoard->set32 == NULL) ||
                         sst1CurrentBoard->fbiLfbLocked ||
                         (addrOffset == 0x004) ||                            /* intrCtrl */
                         ((addrOffset >= 0x1E0) && (addrOffset <= 0x200)) || /* cmdFifoBase ... fbiInit4 */
                         ((addrOffset >= 0x208) && (addrOffset <= 0x224)) || /* backPorch ... vSync */
                         ((addrOffset >= 0x22C) && (addrOffset <= 0x23C)) || /* dacData ... borderColor */
                         ((addrOffset >= 0x244) && (addrOffset <= 0x24C)));  /* fbiInit5 ... fbiInit7 */

  if (directWriteP) {
    P6FENCE;
    SET(addr, data);
    P6FENCE;
  } else {
    (*sst1CurrentBoard->set32)(addr, data);
  }
}

/*
** sst1InitRead32():
**  Read 32-bit Word from specified address
**
*/
FX_EXPORT FxU32 FX_CSTYLE sst1InitRead32(FxU32 *addr)
{
    P6FENCE;
    return GET(addr);
}

/*
**
** sst1InitCmdFifo():

**   Setup Command Fifo.
**
**   The 'enable' parameter either enables or disables the Command Fifo.
**
**   Upon return, the 'virtAddrStart' parameter is filled with the starting
**   virtual address where command fifo packets should be stored.  The
**   'memAddrStart' parameter is filled with the address of where the command
**   fifo is stored in frame buffer memory.  The 'memAddrStart' value is often
**   used by the JMP command to jump to the beginning of the command fifo.
**   The 'size' parameter is filled with the size of the command fifo.
**   The values of 'virtAddrstart', 'memAddrStart' and 'size'
**   are dependent on the amount of frame buffer memory detected as well as the
**   video resolution setup by sst1InitVideo().  'virtAddrstart',
**   'memAddrStart' and 'size' are all specified in bytes.
**
*/
FX_ENTRY FxBool FX_CALL sst1InitCmdFifo(FxU32 *sstbase, FxBool enable, 
  unsigned long *virtAddrStart, FxU32 *memAddrStart, FxU32 *size, FxSet32Proc set32Proc)
{
    SstRegs *sst = (SstRegs *) sstbase;
    FxU32 fbiInit, fifoStart, fifoSize;
    FxBool directExec = (GETENV(("SSTV2_CMDFIFO_DIRECT"))) ? FXTRUE : FXFALSE;
    FxBool disableHoles = (GETENV(("SSTV2_CMDFIFO_NOHOLES"))) ? FXTRUE : FXFALSE;

    if(!sst)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(sst1CurrentBoard->fbiCmdFifoEn || (IGET(sst->fbiInit7) & SST_EN_CMDFIFO))
        sst1InitIdleNoNOP(sstbase);
    else
        sst1InitIdle(sstbase);

    if(enable == FXFALSE) {
       // Remove any client set callbacks before continuing since
       // these must go straight to the hw.  
       sst1CurrentBoard->set32 = NULL;
       sst1CurrentBoard->fbiCmdFifoEn = 0;

       // Disable Command Fifo
       INIT_PRINTF(("sst1InitCmdFifo(): Disabling Command Fifo...\n"));
       ISET(sst->fbiInit7, IGET(sst->fbiInit7) & ~SST_EN_CMDFIFO);

       // Reset graphics core to force disable to take effect...
       ISET(sst->fbiInit0, IGET(sst->fbiInit0) | SST_GRX_RESET |
         SST_EN_LFB_MEMFIFO | SST_EN_TEX_MEMFIFO);
       sst1InitReturnStatus(sstbase);
       sst1InitReturnStatus(sstbase);
       sst1InitReturnStatus(sstbase);
       ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_GRX_RESET);

       if(sst1CurrentBoard->sliSlaveVirtAddr) {
          SstRegs *slaveSst = (SstRegs *) sst1CurrentBoard->sliSlaveVirtAddr;

          ISET(slaveSst->fbiInit7, IGET(slaveSst->fbiInit7) & ~SST_EN_CMDFIFO);
          // Reset graphics core to force disable to take effect...
          ISET(slaveSst->fbiInit0, IGET(slaveSst->fbiInit0) | SST_GRX_RESET |
            SST_EN_LFB_MEMFIFO | SST_EN_TEX_MEMFIFO);
          sst1InitReturnStatus(sst1CurrentBoard->sliSlaveVirtAddr);
          sst1InitReturnStatus(sst1CurrentBoard->sliSlaveVirtAddr);
          sst1InitReturnStatus(sst1CurrentBoard->sliSlaveVirtAddr);
          ISET(slaveSst->fbiInit0, IGET(slaveSst->fbiInit0) & ~SST_GRX_RESET);

          if(sst1InitCheckBoard(sst1CurrentBoard->sliSlaveVirtAddr) == FXFALSE)
              return(FXFALSE);
          sst1CurrentBoard->set32 = NULL;
          sst1CurrentBoard->fbiCmdFifoEn = 0;
       }
       return(FXTRUE);
    }

    sst1InitIdle(sstbase);
    fbiInit = IGET(sst->fbiInit4);
    fifoStart =
      (fbiInit & SST_MEM_FIFO_ROW_BASE) >> SST_MEM_FIFO_ROW_BASE_SHIFT;
    fifoStart <<= 12;  // Convert page-address into byte-address

    if(sst1CurrentBoard->fbiMemSize == 4)
      fifoSize = (1024 - (fifoStart >> 12)) << 12;
    else if(sst1CurrentBoard->fbiMemSize == 2)
      fifoSize = (512 - (fifoStart >> 12)) << 12;
    else
      fifoSize = (256 - (fifoStart >> 12)) << 12;
    if(fifoSize > (256<<10))
      fifoSize = (256<<10);

    *virtAddrStart = ((unsigned long) sstbase) + SST_CMDFIFO_ADDR;
    *memAddrStart = fifoStart;
    *size = fifoSize;

    if(!sst1InitCmdFifoDirect(sstbase, 0, 
                              fifoStart, fifoSize, 
                              directExec, disableHoles,
                              set32Proc)) {
      INIT_PRINTF(("sst1InitCmdFifo(): sst1InitCmdFifoDirect() failed...\n"));
      return(FXFALSE);
    }

    if(sst1CurrentBoard->sliSlaveVirtAddr) {
      if(!sst1InitCmdFifoDirect(sst1CurrentBoard->sliSlaveVirtAddr, 0,
                               fifoStart, fifoSize, 
                               directExec, disableHoles,
                               set32Proc)) {
          INIT_PRINTF(("sst1InitCmdFifo(): sst1InitCmdFifoDirect() failed for SLI Slave...\n"));
          return(FXFALSE);
      }
    }

    INIT_PRINTF(("sst1InitCmdFifo() exiting with status %d...\n", FXTRUE));
    return(FXTRUE);
}

/*
**
** sst1InitCmdFifoDirect():
**   Explicitly initialize Command FIFO.  This routine is typically not 
**   called directly from apps, but is included so that csim diags can
**   call it directly.
**
**   The 'start' and 'size' parameters are specified in bytes.
**   The 'which' parameter is not used, but is included for H3 compatibility.
**
*/
FX_ENTRY FxBool FX_CALL sst1InitCmdFifoDirect(FxU32 *sstbase, FxU32 which, 
  FxU32 start, FxU32 size, FxBool directExec, FxBool disableHoles, 
  FxSet32Proc set32Proc)
{
    SstRegs *sst = (SstRegs *) sstbase;
    FxU32 fbiInit7, initEnable, n;
    FxU32 pageStart = start >> 12;
    FxU32 pageEnd = (start+size-1) >> 12;

    if(!sst)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    // sst1Initidle() routines must be properly executed...
    initIdleEnabled = 1;

    INIT_PRINTF(("sst1InitCmdFifoDirect(): Start:0x%x bytes, Size:%d(0x%x) bytes\n",
      start, size, size));
    INIT_PRINTF(("sst1InitCmdFifoDirect(): DirectExec:%d, DisableHoles:%d\n",
      directExec, disableHoles));
    INIT_PRINTF(("sst1InitCmdFifoDirect(): pageStart:%d, pageEnd:%d\n",
      pageStart, pageEnd));
    INIT_PRINTF(("sst1InitCmdFifoDirect(): set32Proc: 0x%lX\n",
                 (unsigned long)set32Proc));

    if(sst1CurrentBoard->fbiCmdFifoEn || (IGET(sst->fbiInit7) & SST_EN_CMDFIFO))
        sst1InitIdleNoNOP(sstbase);
    else
        sst1InitIdle(sstbase);
    // Disable memory-backed fifo, and disallow lfb and texture writes
    // through command fifo...
    ISET(sst->fbiInit0, (IGET(sst->fbiInit0) & 
      ~(SST_MEM_FIFO_EN | SST_EN_LFB_MEMFIFO | SST_EN_TEX_MEMFIFO)));
    sst1InitReturnStatus(sstbase);

    fbiInit7 = IGET(sst->fbiInit7);
    fbiInit7 &= ~(SST_EN_CMDFIFO |
       SST_EN_CMDFIFO_OFFSCREEN | SST_CMDFIFO_DISABLE_HOLES |
       SST_CMDFIFO_REGS_SYNC_WRITES | SST_CMDFIFO_REGS_SYNC_READS |
       SST_CMDFIFO_RDFETCH_THRESH | SST_CMDFIFO_PCI_TIMEOUT);
    fbiInit7 |= SST_CMDFIFO_REGS_SYNC_WRITES | SST_CMDFIFO_REGS_SYNC_READS |
       (0x10<<SST_CMDFIFO_RDFETCH_THRESH_SHIFT) |
       (127<<SST_CMDFIFO_PCI_TIMEOUT_SHIFT);
    ISET(sst->fbiInit7, fbiInit7);    // turn off the command fifo
    sst1InitReturnStatus(sstbase);

    // Disable all writes to the PCI fifo while we're setting up CMDFIFO
    PCICFG_RD(SST1_PCI_INIT_ENABLE, initEnable);
    PCICFG_WR(SST1_PCI_INIT_ENABLE,
       ((initEnable & ~(SST_INITWR_EN | SST_PCI_FIFOWR_EN)) | SST_INITWR_EN));

    // Setup command fifo...
    ISET(sst->cmdFifoBase, (pageEnd << SST_CMDFIFO_END_SHIFT) | pageStart);
    sst1InitReturnStatus(sstbase);  // prevent PCI bursts...
    ISET(sst->cmdFifoReadPtr, start);
    sst1InitReturnStatus(sstbase);
    ISET(sst->cmdFifoAmin, start-4);
    sst1InitReturnStatus(sstbase);
    ISET(sst->cmdFifoAmax, start-4);
    sst1InitReturnStatus(sstbase);
    ISET(sst->cmdFifoDepth, 0);
    sst1InitReturnStatus(sstbase);
    ISET(sst->cmdFifoHoles, 0);
    sst1InitReturnStatus(sstbase);
    sst1InitIdle(sstbase);

    // Turn on command fifo...
    ISET(sst->fbiInit7, fbiInit7 | SST_EN_CMDFIFO |
      (directExec ? 0 : SST_EN_CMDFIFO_OFFSCREEN) |
      (disableHoles ? SST_CMDFIFO_DISABLE_HOLES : 0));
    // Can't perform sst1InitIdle() here because it will generate
    // writes to the CMDFIFO since the CMDFIFO is now enabled...
    sst1InitReturnStatus(sstbase);

    // Enable writes to be pushed onto the CMDFIFO...
    PCICFG_WR(SST1_PCI_INIT_ENABLE,
       (initEnable | SST_INITWR_EN | SST_PCI_FIFOWR_EN));

    sst1CurrentBoard->fbiCmdFifoEn = 1;
    if(GETENV(("SSTV2_IGNORE_IDLE")))
       initIdleEnabled = 0;

    /* Set the client callbacks, if necessary. */
    sst1CurrentBoard->set32 = set32Proc;

    return(FXTRUE);
}

/*
**
** sst1InitLfbLock():
**   Used to workaround a hw bug when performing lfb and texture writes when
**   the command fifo is enabled.  The routines sst1InitLfbLock() and
**   sst1InitLfbUnlock() dynamically disable the command fifo so that lfb
**   and texture accesses do not pass through the command fifo.
**   WARNING: No register writes of any kind may be performed between a
**   sst1InitLfbLock() and sst1InitLfbUnlock() pair -- only lfb reads and 
**   writes are allowed.
*/
FX_ENTRY FxBool FX_CALL sst1InitLfbLock(FxU32* sstbase)
{
   if(!sstbase)
       return(FXFALSE);

   if(sst1InitCheckBoard(sstbase) == FXFALSE)
       return(FXFALSE);

   // Idle the hardware...
   sst1InitIdle(sstbase);

   if(sst1InitLfbLockDirect(sstbase) == FXFALSE)
       return(FXFALSE);

   if(sst1CurrentBoard->sliSlaveVirtAddr) {
       // SLI Enabled
       if(sst1InitLfbLockDirect(sst1CurrentBoard->sliSlaveVirtAddr) == FXFALSE)
           return(FXFALSE);
   }

   return(FXTRUE);
}

FX_ENTRY FxBool FX_CALL sst1InitLfbLockDirect(FxU32* sstbase)
{
   SstRegs *sst = (SstRegs *) sstbase;

   if(!sst)
       return(FXFALSE);

   if(sst1InitCheckBoard(sstbase) == FXFALSE)
       return(FXFALSE);

   if((!(IGET(sst->fbiInit7) & SST_EN_CMDFIFO) &&
     !sst1CurrentBoard->fbiCmdFifoEn) || sst1CurrentBoard->fbiLfbLocked)
       return(FXTRUE);

   // Force direct writes...
   sst1CurrentBoard->fbiLfbLocked = 1;

   ISET(sst->fbiInit7, IGET(sst->fbiInit7) & ~SST_EN_CMDFIFO_OFFSCREEN);
   sst1InitReturnStatus(sstbase);
   sst1InitReturnStatus(sstbase);
   sst1InitReturnStatus(sstbase);

   return(FXTRUE);
}

/*
**
** sst1InitLfbUnlock():
**   Used to workaround a hw bug when performing lfb and texture writes when
**   the command fifo is enabled.  The routines sst1InitLfbLock() and
**   sst1InitLfbUnlock() dynamically disable the command fifo so that lfb
**   and texture accesses do not pass through the command fifo.
**   WARNING: No register writes of any kind may be performed between a
**   sst1InitLfbLock() and sst1InitLfbUnlock() pair -- only lfb reads and 
**   writes are allowed.
*/

FX_ENTRY FxBool FX_CALL sst1InitLfbUnlock(FxU32* sstbase)
{
   if(!sstbase)
       return(FXFALSE);

   if(sst1InitCheckBoard(sstbase) == FXFALSE)
       return(FXFALSE);

   // Idle the hardware.  Since lfb lock is set, a NOP will not be issued, and
   // sst1InitPciFifoIdleLoop() will be used instead of sst1InitIdleLoop()...
   sst1InitIdle(sstbase);

   if(sst1CurrentBoard->sliSlaveVirtAddr) {
       // SLI Enabled
       if(sst1InitLfbUnlockDirect(sst1CurrentBoard->sliSlaveVirtAddr) ==
         FXFALSE)
           return(FXFALSE);
   }
   if(sst1InitLfbUnlockDirect(sstbase) == FXFALSE)
       return(FXFALSE);

   // After the command fifo is re-enabled, make sure the chip is really idle...
   sst1InitIdle(sstbase);

   return(FXTRUE);
}

FX_ENTRY FxBool FX_CALL sst1InitLfbUnlockDirect(FxU32* sstbase)
{
   SstRegs *sst = (SstRegs *) sstbase;

   if(!sst)
       return(FXFALSE);

   if(sst1InitCheckBoard(sstbase) == FXFALSE)
       return(FXFALSE);

   if(!sst1CurrentBoard->fbiCmdFifoEn || !sst1CurrentBoard->fbiLfbLocked)
       return(FXTRUE);

   ISET(sst->fbiInit7, IGET(sst->fbiInit7) | SST_EN_CMDFIFO_OFFSCREEN);

   sst1CurrentBoard->fbiLfbLocked = 0;

   sst1InitReturnStatus(sstbase);
   sst1InitReturnStatus(sstbase);
   sst1InitReturnStatus(sstbase);

   return(FXTRUE);
}

/*
**
** sst1InitPrintInitRegs():
**   Print Initialization Registers
**
*/
FX_EXPORT void FX_CSTYLE sst1InitPrintInitRegs(FxU32 *sstbase)
{
    SstRegs *sst = (SstRegs *) sstbase;

    INIT_PRINTF(("FBI Initialization Registers:\n"));
    INIT_PRINTF(("-----------------------------\n"));
    INIT_PRINTF(("fbiInit0:0x%08x  fbiInit1:0x%08x\n",
      IGET(sst->fbiInit0), IGET(sst->fbiInit1)));
    INIT_PRINTF(("fbiInit2:0x%08x  fbiInit3:0x%08x\n",
      IGET(sst->fbiInit2), IGET(sst->fbiInit3)));
    INIT_PRINTF(("fbiInit4:0x%08x  fbiInit5:0x%08x\n",
      IGET(sst->fbiInit4), IGET(sst->fbiInit5)));
    INIT_PRINTF(("fbiInit6:0x%08x  fbiInit7:0x%08x\n",
      IGET(sst->fbiInit6), IGET(sst->fbiInit7)));
    INIT_PRINTF(("videoDimensions:0x%08x\n\n",
      IGET(sst->videoDimensions)));
    INIT_PRINTF(("FBI Command Fifo Registers:\n"));
    INIT_PRINTF(("---------------------------\n"));
    INIT_PRINTF(("cmdFifoBase:    0x%08x\tcmdFifoBump:0x%x\n",
      IGET(sst->cmdFifoBase), IGET(sst->cmdFifoBump)));
    INIT_PRINTF(("cmdFifoReadPtr: 0x%08x\tcmdFifoAmin:0x%x\n",
      IGET(sst->cmdFifoReadPtr), IGET(sst->cmdFifoAmin)));
    INIT_PRINTF(("cmdFifoAmax:    0x%08x\tcmdFifoDepth:0x%x\n",
      IGET(sst->cmdFifoAmax), IGET(sst->cmdFifoDepth)));
    INIT_PRINTF(("cmdFifoHoles:   0x%08x\n", IGET(sst->cmdFifoHoles)));
}

/*
**
** sst1InitMeasureSiProcess():
**   Use silicon process register to measure silicon performance
**
*/
FX_EXPORT FxU32 FX_CSTYLE sst1InitMeasureSiProcess(FxU32 *sstbase, FxU32 which)
{
    const char *envp;
    FxU32 n, siProcess, nandOsc, norOsc;
    FxU32 pciCntrLoad = 0xfff;
    FxU32 cntr;
    int i;

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    envp = GETENV(("SSTV2_SIPROCESS_CNTR"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) ) {
        pciCntrLoad = i;
        INIT_PRINTF(("sst1InitMeasureSiProcess(): Using PCI Counter preload value of 0x%x...\n", pciCntrLoad));
    }

    if(!which) {
       ////////////////////////////////
       // Test NAND oscillator tree...
       ////////////////////////////////
       PCICFG_WR(SST1_PCI_SIPROCESS,
        (pciCntrLoad<<SST_SIPROCESS_PCI_CNTR_SHIFT) |
         SST_SIPROCESS_OSC_CNTR_RESET_N | SST_SIPROCESS_OSC_NAND_SEL);

       // Allow oscillator to run...
       PCICFG_RD(SST1_PCI_SIPROCESS, siProcess);
       PCICFG_WR(SST1_PCI_SIPROCESS,
        (pciCntrLoad<<SST_SIPROCESS_PCI_CNTR_SHIFT) |
         SST_SIPROCESS_OSC_CNTR_RUN | SST_SIPROCESS_OSC_NAND_SEL);

       // Loop until PCI counter decrements to 0
       cntr = 0 ;
       do {
          ++cntr;
          PCICFG_RD(SST1_PCI_SIPROCESS, siProcess);
       } while(siProcess & SST_SIPROCESS_PCI_CNTR);

       PCICFG_RD(SST1_PCI_SIPROCESS, siProcess);
       siProcess &= SST_SIPROCESS_OSC_CNTR;
       nandOsc = siProcess;
       if(nandOsc < 3000)
          nandOsc <<= 1; // Running on 66 MHz PCI...
       sst1CurrentBoard->fbiNandTree = nandOsc;

       INIT_PRINTF(("sst1InitInfo(): NAND-tree: %d\n", nandOsc));
    } else {
       ////////////////////////////////
       // Test NOR oscillator tree...
       ////////////////////////////////
       PCICFG_WR(SST1_PCI_SIPROCESS,
        (pciCntrLoad<<SST_SIPROCESS_PCI_CNTR_SHIFT) |
         SST_SIPROCESS_OSC_CNTR_RESET_N | SST_SIPROCESS_OSC_NOR_SEL);

       // Allow oscillator to run...
       PCICFG_RD(SST1_PCI_SIPROCESS, siProcess);
       PCICFG_WR(SST1_PCI_SIPROCESS,
       (pciCntrLoad<<SST_SIPROCESS_PCI_CNTR_SHIFT) |
         SST_SIPROCESS_OSC_CNTR_RUN | SST_SIPROCESS_OSC_NOR_SEL);

       // Loop until PCI counter decrements to 0
       cntr = 0 ;
       do {
          ++cntr;
          PCICFG_RD(SST1_PCI_SIPROCESS, siProcess);
       } while(siProcess & SST_SIPROCESS_PCI_CNTR);

       PCICFG_RD(SST1_PCI_SIPROCESS, siProcess);
       siProcess &= SST_SIPROCESS_OSC_CNTR;
       norOsc = siProcess;
       if(norOsc < 3000)
          norOsc <<= 1; // Running on 66 MHz PCI...
       sst1CurrentBoard->fbiNorTree = norOsc;

       INIT_PRINTF(("sst1InitInfo(): NOR-tree : %d\n", norOsc));
    }
    return(siProcess);
}

/*
**
** sst1InitCalcTClkDelay()
** Calculate optimal TF/TT Clock delay values
**
*/
FX_EXPORT FxBool FX_CSTYLE sst1InitCalcTClkDelay(FxU32 *sstbase,
   FxU32 tmuNumber, FxU32 tClkDelay)
{
   SstRegs *sst = (SstRegs *) sstbase;
   volatile FxU32 *texAddr;

   if(sst1InitCheckBoard(sstbase) == FXFALSE)
      return(FXFALSE);

   ISET(sst->tLOD, 0x0);
   ISET(sst->tDetail, 0x0);
   ISET(sst->texBaseAddr, 0x0);
   ISET(sst->fogMode, 0x0);
   ISET(sst->alphaMode, 0x0);
   ISET(sst->fbzMode, SST_ENCHROMAKEY);
   ISET(sst->fbzColorPath, SST_RGBSEL_TREXOUT | SST_CC_PASS | SST_ENTEXTUREMAP);
   //texAddr = (numTmu<<(21-2)) + (FxU32 *) SST_TEX_ADDRESS(sst);
   texAddr = (0<<(21-2)) + (FxU32 *) SST_TEX_ADDRESS(sst);
   ISET(texAddr[0], 0x0);
   texAddr = (1<<(21-2)) + (FxU32 *) SST_TEX_ADDRESS(sst);
   ISET(texAddr[0], 0x0);
   texAddr = (2<<(21-2)) + (FxU32 *) SST_TEX_ADDRESS(sst);
   ISET(texAddr[0], 0x0);

   ISET(SST_TREX(sst,tmuNumber)->trexInit1,
      (sst1CurrentBoard->tmuInit1[tmuNumber] & ~SST_TEX_TF_CLK_DEL_ADJ) |
      (tClkDelay<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT));

   if(!sst1InitIdleFBIWithTimeout(sstbase, FXTRUE, 10000))
      return(FXFALSE);

   // Reset pixel stat registers
   ISET(sst->nopCMD, 0x3);
   sst1InitCheckTmuMemConst(sstbase, tmuNumber, 0xff0000);
   sst1InitCheckTmuMemConst(sstbase, tmuNumber, 0x00ffff);
   sst1InitCheckTmuMemConst(sstbase, tmuNumber, 0xf0f0f0);
   sst1InitCheckTmuMemConst(sstbase, tmuNumber, 0x0f0f0f);
   sst1InitCheckTmuMemConst(sstbase, tmuNumber, 0x55aa55);
   sst1InitCheckTmuMemConst(sstbase, tmuNumber, 0xaa55aa);
   sst1InitCheckTmuMemConst(sstbase, tmuNumber, 0x5a5a5a);
   sst1InitCheckTmuMemConst(sstbase, tmuNumber, 0xa5a5a5);

   if(!sst1InitIdleFBIWithTimeout(sstbase, FXTRUE, 10000))
      return(FXFALSE);

   if(IGET(sst->stats.fbiChromaFail))
      return(FXFALSE);
   else
      return(FXTRUE);
}

#define COLOR32TOCOLOR24_6666(X)       \
      ((((X >> 2) & 0x3F) << 0) |      \
       (((X >> 10) & 0x3F) << 6) |     \
       (((X >> 18) & 0x3F) << 12) |    \
       (((X >> 26) & 0x3F) << 18))
#define COLOR24_6666TOCOLOR32(X)                                  \
      (((((X >> 0) & 0x3F) << 2) | (((X >> 4) & 0x3) << 0)) |     \
       ((((X >> 6) & 0x3F) << 10) | (((X >> 10) & 0x3) << 8)) |   \
       ((((X >> 12) & 0x3F) << 18) | (((X >> 16) & 0x3) << 16)) | \
       ((((X >> 18) & 0x3F) << 26) | (((X >> 22) & 0x3) << 24)))

void sst1InitCheckTmuMemConst(FxU32 *sstbase, FxU32 tmuNumber,
   FxU32 dataExpect32)
{
   SstRegs *sst = (SstRegs *) sstbase;
   FxU32 dataExpect6666 = COLOR32TOCOLOR24_6666(dataExpect32);
   FxU32 dataExpect6666To32 = COLOR24_6666TOCOLOR32(dataExpect6666) & 0xffffff;

   ISET(sst->chromaKey, dataExpect6666To32);
   ISET(sst->chromaRange, dataExpect6666To32 | SST_ENCHROMARANGE |
      SST_CHROMARANGE_BLUE_EX | SST_CHROMARANGE_GREEN_EX |
      SST_CHROMARANGE_RED_EX | SST_CHROMARANGE_BLOCK_OR);

   if(!tmuNumber) {
      ISET(sst->textureMode, SST_P8_ARGB6666 | SST_TC_REPLACE | SST_TCA_ZERO);
   } else if(tmuNumber == 1) {
      // Force downstream TMU to passthrough upstream data
      ISET(SST_TREX(sst,0)->textureMode, SST_P8_ARGB6666 | SST_TC_PASS |
        SST_TCA_PASS);
      ISET(SST_TREX(sst,1)->textureMode, SST_P8_ARGB6666 | SST_TC_REPLACE |
        SST_TCA_ZERO);
      ISET(SST_TREX(sst,2)->textureMode, SST_P8_ARGB6666 | SST_TC_REPLACE |
        SST_TCA_ZERO);
   } else {
      // Force downstream TMUs to passthrough upstream data
      ISET(SST_TREX(sst,0)->textureMode, SST_P8_ARGB6666 | SST_TC_PASS |
        SST_TCA_PASS);
      ISET(SST_TREX(sst,1)->textureMode, SST_P8_ARGB6666 | SST_TC_PASS |
        SST_TCA_PASS);
      ISET(SST_TREX(sst,2)->textureMode, SST_P8_ARGB6666 | SST_TC_REPLACE |
        SST_TCA_ZERO);
   }
   // Specify color in palette entry 0...
   //sst->nccTable0[4]=BIT(31) | ((index>>1)<<24) | (palette[index] & 0xffffff);
   ISET(sst->nccTable0[4], BIT(31) | COLOR32TOCOLOR24_6666(dataExpect32));
   sst1InitDrawRectUsingTris(sstbase, 0, 0, 128);

   return;
}

void sst1InitDrawRectUsingTris(FxU32 *sstbase, FxU32 x, FxU32 y, FxU32 tSize)
{
   SstRegs *sst = (SstRegs *) sstbase;

   ISET(sst->vA.x, (x<<SST_XY_FRACBITS));
   ISET(sst->vA.y, (y<<SST_XY_FRACBITS));
   ISET(sst->vB.x, ((x+tSize)<<SST_XY_FRACBITS));
   ISET(sst->vB.y, (y<<SST_XY_FRACBITS));
   ISET(sst->vC.x, ((x+tSize)<<SST_XY_FRACBITS));
   ISET(sst->vC.y, ((y+tSize)<<SST_XY_FRACBITS));
   ISET(sst->s, 0);
   ISET(sst->t, 0);
   ISET(sst->w, 0);
   ISET(sst->r, (0xff<<SST_XY_INTBITS));
   ISET(sst->g, 0);
   ISET(sst->b, 0);
   ISET(sst->dsdx, 0);
   ISET(sst->dtdx, 0);
   ISET(sst->dwdx, 0);
   ISET(sst->dsdy, 0);
   ISET(sst->dtdy, 0);
   ISET(sst->dwdy, 0);
   ISET(sst->triangleCMD, 0);
   ISET(sst->vB.x, (x<<SST_XY_FRACBITS));
   ISET(sst->vB.y, ((y+tSize)<<SST_XY_FRACBITS));
   ISET(sst->triangleCMD, 0xFFFFFFFF);
}

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
