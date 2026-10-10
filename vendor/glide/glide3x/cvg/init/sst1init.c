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
** SSTV2_ARCADE                  X       Allow special configs for arcade use
** SSTV2_ALLOC_AUX               {0,1}   Number of aux. buffers to allocate
** SSTV2_ALLOC_COLOR             {2,3}   Number of color buffers to allocate
** SSTV2_BACKPORCH               hex     Specify value of backPorch video register
** SSTV2_BOARDS                  val     Specify number of SST-1 boards in system
** SSTV2_CMDFIFO_DIRECT          X       Enable Direct Execution of CMDFIFO
** SSTV2_CMDFIFO_NOHOLES         X       Disable hole counting feature in CMDFIFO
** SSTV2_DEBUGDAC                X       Print out debug information for DAC
**                                       reads and writes
** SSTV2_DEVICEID                X       Specify the device ID value passed to
**                                       the PCI library
** SSTV2_DIMENSIONS              hex     Specify value of videoDimensions register
** SSTV2_FASTMEM_RAS_READS       X       Allow fast ras reads
** SSTV2_FASTPCIRD               X       Enable fast PCI reads
** SSTV2_FBICFG                  hex     Specify FBI configuration strapping bits
** SSTV2_FBI_MEMSIZE             val     Specify amount of frame buffer memory
** SSTV2_FT_CLKDEL               hex     Fbi-to-Trex clock delay value
** SSTV2_GAMMA                   float   Floating point value for gamma correction
** SSTV2_GRXCLK                  val     16 <= Frequency < 80 (50 is default)
** SSTV2_HSYNC                   hex     Specify value of hSync video register
** SSTV2_IGNORE_CLKDELAYS        X       Ignore calls to sst1InitSetClkDelays()
** SSTV2_IGNORE_IDLE             X       Ignore calls to sst1InitIdle*()
** SSTV2_IGNORE_INIT_GAMMA       X       Bypass sst1InitGamma call and return
** SSTV2_IGNORE_INIT_REGISTERS   X       Bypass sst1InitRegisters call and return
** SSTV2_IGNORE_INIT_VIDEO       X       Bypass sst1InitVideo call and return
** SSTV2_INITDEBUG               X       Enable debug output
** SSTV2_INITDEBUG_FILE          file    File to direct all debug output
** SSTV2_MEMFIFO                 0       Disable FBI Memory FIFO
** SSTV2_MEMFIFO                 1       Enable FBI Memory FIFO
** SSTV2_MEMFIFO_ENTRIES         hex     Set number of entries in memory fifo
** SSTV2_MEMFIFO_HWM             hex     Set memory fifo high water mark
** SSTV2_MEMFIFO_LFB             {0,1}   Specify LFB writes through Memory FIFO
** SSTV2_MEMFIFO_LWM             hex     Set memory fifo low water mark
** SSTV2_MEMFIFO_TEX             {0,1}   Specify Texture writes through Mem FIFO
** SSTV2_MEMOFFSET               hex     Specify value of memoffset video register
** SSTV2_NOCHECKHANG             X       Do not Check for FBI Hangs for texturing
** SSTV2_NODEVICEINFO            X       Ignore calls to sst1InitFillDeviceInfo()
** SSTV2_NOSHUTDOWN              X       Do not turn off monitor refresh on call
**                                       to sst1InitShutdown()
** SSTV2_NUM_TMUS                {1,2,3} Specify number of TMUs detected
** SSTV2_PCIFIFO_LWM             hex     Set pci fifo low water mark
** SSTV2_PFT_CLKDEL              hex     Preliminary Fbi-to-Trex clock delay value
** SSTV2_PTF0_CLKDEL             hex     Prelim TMU2Fbi clock delay value (TMU #0)
** SSTV2_PTF1_CLKDEL             hex     Prelim TMU2Fbi clock delay value (TMU #1)
** SSTV2_PTF2_CLKDEL             hex     Prelim TMU2Fbi clock delay value (TMU #2)
** SSTV2_SIPROCESS_CNTR          hex     Silicon process monitor PCI counter load
** SSTV2_SLIDETECT               {0,1}   Specify Scanline Interleaving detection
** SSTV2_SLIM_VIN_CLKDEL         hex     Specify FBI internal video clock delay
**                                       (SLI Master)
** SSTV2_SLIM_VOUT_CLKDEL        hex     Specify FBI external video clock delay
**                                       (SLI Master)
** SSTV2_SLIS_VIN_CLKDEL         hex     Specify FBI internal video clock delay
**                                       (SLI Slave)
** SSTV2_SLIS_VOUT_CLKDEL        hex     Specify FBI external video clock delay
**                                       (SLI Slave)
** SSTV2_SCREENREFRESH           {60,75, Select monitor refresh rate
**                               85,120}
** SSTV2_SCREENREZ               {512,   Select monitor resolution (512x384)
**                               512256, (512x256)
**                               640400, (640x400)
**                               640,    (640x480)
**                               800,    (800x600)
**                               856,    (856x600)
**                               960,    (960x720)
**                               1024}   (1024x768)
** SSTV2_SLOWMEM_RTW             X       Insert wait state for read-to-write 
**                                       transitions
** SSTV2_SLOWMEM_WTR             X       Insert wait state for write-to-read 
**                                       transitions
** SSTV2_SLOWPCIWR               X       Enable 1 wait-state PCI writes
** SSTV2_SLISWAP                 1       Use dac_data[0] for SLI swapping
** SSTV2_SWAPBOARDS              X       Swap order of mapping boards
** SSTV2_TEXMAP_DISABLE          X       Disable texture mapping
** SSTV2_TF0_CLKDEL              hex     Trex-to-Fbi clock delay value (TMU #0)
** SSTV2_TF1_CLKDEL              hex     Trex-to-Fbi clock delay value (TMU #1)
** SSTV2_TF2_CLKDEL              hex     Trex-to-Fbi clock delay value (TMU #2)
** SSTV2_TF_FIFO_THRESH          hex     Set TREX-to-FBI FIFO threshold
** SSTV2_TILESINX                hex     Specify value of tilesInX video register
** SSTV2_TMUCFG                  hex     Specify TMU configuration strapping bits
** SSTV2_TMUMEM                  X       Enable accurate determination of TMU mem
** SSTV2_TMU_MEMSIZE             val     Specify amount of texture memory
** SSTV2_TREX0INIT0              hex     Set trexInit0 register value - TMU #0
** SSTV2_TREX0INIT1              hex     Set trexInit1 register value - TMU #0
** SSTV2_TREX1INIT0              hex     Set trexInit0 register value - TMU #1
** SSTV2_TREX1INIT1              hex     Set trexInit1 register value - TMU #1
** SSTV2_TREX2INIT0              hex     Set trexInit0 register value - TMU #2
** SSTV2_TREX2INIT1              hex     Set trexInit1 register value - TMU #2
** SSTV2_VIN_CLKDEL              hex     Specify FBI internal video clock delay
** SSTV2_VOUT_CLKDEL             hex     Specify FBI external video clock delay
** SSTV2_VGA_PASS                {0,1}   Force VGA_PASS output to {0,1}
** SSTV2_VIDCLK2X                val     Video clock frequency (2x dot clock)
** SSTV2_VIDEO_24BPP             0       Select 16-bit video output
** SSTV2_VIDEO_24BPP             1       Select 24-bit video output
** SSTV2_VIDEO_DISABLE           X       Turn off Video/Monitor refresh in FBI
** SSTV2_VIDEO_FILTER_DISABLE    X       Disable video filtering
** SSTV2_VIDEO_FILTER_THRESHOLD  hex     Set video filtering threshold
** SSTV2_VIDEO_NOCLEAR           X       Do not clear buffers in sst1InitVideo()
** SSTV2_VIDEO_CLEARCOLOR        hex     Clear screen color
** SSTV2_VFIFO_THRESH            {0-31}  Select video fifo threshold
** SSTV2_VSYNC                   hex     Specify value of vSync video register
** SSTV2_NOREMAP                 0       Don't try to remap single board sli.
** SSTV2_MISMATCHED_SLI          X       Allow mismatched SLI
**
** VOODOO2_FILE                  name    Filename used in place of "voodoo2.ini"
** VOODOO2_PATH                  path    Path used to locate "voodoo2.ini" file
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
#include <fxpci.h>
#define SST1INIT_ALLOCATE // Allocate variables in sst1init.h
#define FX_DLL_DEFINITION
#include <fxdll.h>

#if !macintosh && !__linux__
#include <fxremap.h>
#endif
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
    return(sst1InitMapBoardDirect(BoardNumber, FXTRUE));
}

// Use this flag to force a info clear when doing a
// sst1InitMapBoard() for the first time.  sst1InitShutdown()
// will also set this flag to TRUE to force a clear
// next time around.
static FxU32 clearBoardInfo = FXTRUE;

FX_EXPORT FxU32 * FX_CSTYLE sst1InitMapBoardDirect(FxU32 BoardNumber, 
                                                   FxBool resetSLI)
{
    static FxU32 firstTime = 1;
    FxU32 vendorID = _3DFX_PCI_ID;     // 3Dfx Vendor ID
    FxU32 deviceID;                    // 0x0002 - Look for a Voodoo2 board (0xFFFF - Find any 3Dfx board)
    FxU32 sizeOfCard = 0x1000000;      // 16 MBytes of addr space for SST-1
    FxU32 *sstbase;
    FxU32 j, n;
    FxU32 sstv2_noremap = 0;
    const char *envp;
    int i;

    envp = GETENV(("SSTV2_DEVICEID"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) )
      deviceID = i;
    else
      deviceID = 0x0002;

#if !DIRECTX
    // Open PCI library (necessary for multiple calls to init routines, after
    // PCI library is closed by pciClose() call in sst1InitShutdown().
    // 
    // NB: It is safe to do this even if we never called pciClose.
    pciOpen();
#endif

    /* NB: firstTime does not get cleared until we actually find a
     * board and get one mapped (A little weird to begin w/, but
     * probably not worth changing at this late date). However,
     * sst1InitMapBoard usually gets called up to MAX_PCI_DEVICES
     * which means that we'll be re-counting and re-mapping boards
     * over and over which is just whacked. Now, if we fail to find
     * any suitable boards via sst1InitNumBoardsInSystem we clear the
     * firstTime flag, and carry on checking for the non-existant
     * board which will (of course) fail to map.  
     */
    if(firstTime) {
      // Find "voodoo2.ini" file if it exists...
      sst1InitUseVoodooFile = sst1InitVoodooFile();
      
      envp = GETENV(("SSTV2_NOREMAP"));
      if(envp && (SSCANF(envp, "%i", &i) == 1) )
        sstv2_noremap = i;
      else
        sstv2_noremap = 0;
#if !macintosh && !__linux__
      if (!sstv2_noremap) {
        fxremap(); /* remap single board SLI */
      }
#endif
      if(!(boardsInSystem = sst1InitNumBoardsInSystem())) {
        firstTime = FXTRUE;
        return(NULL);
      }
    }

    if( clearBoardInfo ) {
        INIT_PRINTF(("sst1InitMapBoard(): BoardsInSystem = %d\n",
          boardsInSystem));

        // Clear board info structure
        sst1InitClearBoardInfo();
        
        clearBoardInfo = FXFALSE;

#if !DIRECTX
        /* dpc - 26 feb 1998
         * If glide is running on windows the a pciClose will close
         * the vxd etc. This is not functionally longer fatal, but w/o
         * it we will not be able to do things like set the caching on
         * the board's memory etc. This is bad. 
         *
         * The actual cost of doing the re-mapping again is pretty low
         * because both the pci library and the init code are caching the
         * addresses of the board. However, the call to the pciLibrary
         * to map the board has the side affect of causing the vxd to be
         * loaded. It is this that is responsible for setting up caching.
         */
        firstTime = FXTRUE;
#endif /* !DIRECTX */
    }

    if(firstTime) {
        int boardLoopStart = 0;
        int boardLoopStop = SST1INIT_MAX_BOARDS;
        int boardLoopInc = 1;
        int index = 0;

        sst1InitClearBoardInfo();
        if(GETENV(("SSTV2_SWAPBOARDS"))) {
           boardLoopStart = SST1INIT_MAX_BOARDS-1;
           boardLoopStop = -1;
           boardLoopInc = -1;
        }

        for(j=0; j<SST1INIT_MAX_BOARDS; j++) {
           sst1BoardInfo[j].virtAddr[0] = (SstRegs *) NULL;
           sst1BoardInfo[j].physAddr[0] = (FxU32) NULL;
           sst1BoardInfo[j].deviceNumber = 0xdead;
           sst1BoardInfo[j].fbiRevision = 0xdead;
           sst1BoardInfo[j].deviceID = 0xdead;
           sst1BoardInfo[j].vendorID = 0xdead;
        }


        // Map all boards in the system
        // Check that scanline interleaving is not enabled...
        for(j = boardLoopStart; j != (FxU32)boardLoopStop; j += boardLoopInc) {
            sstbase = pciMapCardMulti(vendorID, deviceID, sizeOfCard,
                                      &sst1InitDeviceNumber, j, 0);
            if(sstbase) {
                sst1BoardInfo[index].virtAddr[0] = (SstRegs *) sstbase;
                PCICFG_RD(PCI_BASE_ADDRESS_0, sst1BoardInfo[index].physAddr[0]);
                sst1BoardInfo[index].deviceNumber = sst1InitDeviceNumber;

                //sst1BoardInfo[index].singleBrdSLISlave = 
                //        ((sst1InitDeviceNumber >> 13) & 0x7);
                {
                  FxU32 oldFbiRevision, newFbiRevision;

                  PCICFG_RD(PCI_REVISION_ID, oldFbiRevision);
                  PCICFG_RD(SST1_PCI_INIT_ENABLE, newFbiRevision);
                  sst1BoardInfo[index].fbiFab =
                    (newFbiRevision & SST_CHUCK_MFTG_ID) >>
                    SST_CHUCK_MFTG_ID_SHIFT;
                  newFbiRevision = (newFbiRevision & SST_CHUCK_REVISION_ID) >>
                    SST_CHUCK_REVISION_ID_SHIFT;
                  if(oldFbiRevision != 0x2 && newFbiRevision < 4)
                     return(NULL);
                  sst1BoardInfo[index].fbiRevision = newFbiRevision;
                }
                PCICFG_RD(PCI_DEVICE_ID, sst1BoardInfo[index].deviceID);
                PCICFG_RD(PCI_VENDOR_ID, sst1BoardInfo[index].vendorID);

                // Enable Memory accesses to SST-1
                // Must be set for PnP BIOS which do not enable memory mapped
                // accesses (sst1InitSliDetect reads memory mapped regs)
                PCICFG_WR(PCI_COMMAND, SST_PCIMEM_ACCESS_EN);
                index++;
            } else {
                FxU32 code = pciGetErrorCode();
                if (code != PCI_ERR_NOERR) {
#ifdef __WIN32__
                  MessageBox(NULL, pciGetErrorString(), NULL, MB_OK);
                  INIT_PRINTF(("sst1InitMapBoard(): 0x%X\n", GetLastError()));
#endif

                  INIT_PRINTF(("pciError(): %s", pciGetErrorString()));

                  exit(-1);
                }
            }
        }
    }

    if(resetSLI) {
       // Search through all known boards for SLI enabled...
       FxU32 k;
       SstRegs *sst;

       for(k=0; k<boardsInSystemReally; k++) {
          // Disable SLI if detected...
          if(!(sstbase = (FxU32 *) sst1BoardInfo[k].virtAddr[0]))
             return(NULL);
          sst1InitDeviceNumber = sst1BoardInfo[k].deviceNumber;
          sst1CurrentBoard = &sst1BoardInfo[k];
          sst = (SstRegs *) sstbase;

          if(IGET(sst->fbiInit1) & SST_EN_SCANLINE_INTERLEAVE) {
             INIT_PRINTF(("sst1InitMapBoard(): Disabling Scanline Interleaving (board #%d)...\n", (k+1)));
             // Disable SLI Snooping...
             PCICFG_RD(SST1_PCI_INIT_ENABLE, j);
             PCICFG_WR(SST1_PCI_INIT_ENABLE, j &
               ~(SST_SCANLINE_SLV_OWNPCI | SST_SCANLINE_SLI_SLV |
                 SST_SLI_SNOOP_EN | SST_SLI_SNOOP_MEMBASE));
             ISET(sst->fbiInit1, IGET(sst->fbiInit1) &
               ~SST_EN_SCANLINE_INTERLEAVE);
             sst1InitReturnStatus(sstbase);
          }

          // Tristate video output signals, so two boards connected as SLI
          // do not have contention problems when SSTV2_SLIDETECT=0
          if(firstTime)
             sst1InitVideoShutdown(sstbase, FXFALSE);
       }
    }
    firstTime = 0;

    // Max number of supported boards in system
    if(BoardNumber > SST1INIT_MAX_BOARDS) return(NULL);

    if(!(sstbase = (FxU32 *) sst1BoardInfo[BoardNumber].virtAddr[0]))
        return (NULL);

    INIT_PRINTF(("sst1InitMapBoard(): vAddr:0x%lx pAddr:0x%x Dev:0x%x Board:%d\n",
        (unsigned long) sst1BoardInfo[BoardNumber].virtAddr[0],
        (FxU32) sst1BoardInfo[BoardNumber].physAddr[0],
        (FxU32) sst1BoardInfo[BoardNumber].deviceNumber, BoardNumber));

    // Default settings
    sst1BoardInfo[BoardNumber].vgaPassthruDisable = SST_EN_VGA_PASSTHRU;
    sst1BoardInfo[BoardNumber].vgaPassthruEnable = 0x0;
    sst1BoardInfo[BoardNumber].fbiVideo16BPP = 0;

    envp = GETENV(("SSTV2_VGA_PASS"));
    if(envp) {
        i = ATOI(envp);
        INIT_PRINTF(("sst1InitMapBoard(): Using SST_VGA_PASS=%d\n", i));
        if(i) {
            sst1BoardInfo[BoardNumber].vgaPassthruEnable = SST_EN_VGA_PASSTHRU;
            sst1BoardInfo[BoardNumber].vgaPassthruDisable = 0x0;
        } else {
            sst1BoardInfo[BoardNumber].vgaPassthruDisable = SST_EN_VGA_PASSTHRU;
            sst1BoardInfo[BoardNumber].vgaPassthruEnable = 0x0;
        }
    }

    return(sstbase);
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
    FxU32 ft_clkdel, tf0_clkdel, tf1_clkdel, tf2_clkdel;
    sst1ClkTimingStruct sstGrxClk;
    SstRegs *sst = (SstRegs *) sstbase;
    const char *envp;
    int i;

    if(!sst)
        return(FXFALSE);

    if(sst1InitCheckBoard(sstbase) == FXFALSE)
        return(FXFALSE);

    if(GETENV(("SSTV2_IGNORE_INIT_REGISTERS"))) {
        INIT_PRINTF(("WARNING: Ignoring sst1InitRegisters()...\n"));
        sst1InitIdleFBINoNOP(sstbase);
        return(FXTRUE);
    }

    if(GETENV(("SSTV2_TEXMAP_DISABLE")))
       INIT_PRINTF(("sst1InitRegisters() WARNING: Disabling texture mapping\n"));

#if !DIRECTX
    // Open PCI library (necessary for multiple calls to init routines, after
    // PCI library is closed by pciClose() call in sst1InitShutdown()
    pciOpen();
#endif

    // Enable writes to the FBIINIT registers
    // Do not allow writes into the pci fifo until everything is reset
    PCICFG_WR(SST1_PCI_INIT_ENABLE, SST_INITWR_EN);

    // Reset Snoop registers to default values
    PCICFG_WR(SST1_PCI_BUS_SNOOP0, SST_PCI_BUS_SNOOP_DEFAULT);
    PCICFG_WR(SST1_PCI_BUS_SNOOP1, SST_PCI_BUS_SNOOP_DEFAULT);
    sst1InitReturnStatus(sstbase); // Stall - can't call IdleFbi because 
    sst1InitReturnStatus(sstbase); // FBI could be hung at this stage
    sst1InitReturnStatus(sstbase);
    
    // Adjust Trex-to-Fbi FIFO
    envp = GETENV(("SSTV2_TF_FIFO_THRESH"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) )
        tf_fifo_thresh = i;
    else
        tf_fifo_thresh = 0x8;
    INIT_PRINTF(("sst1InitRegisters(): Setting TREX-to-FBI FIFO THRESHOLD to 0x%x...\n",
        tf_fifo_thresh));

    envp = GETENV(("SSTV2_PFT_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) )
        ft_clkdel = i;
    else
        ft_clkdel = 0x8; // Okay for 16 MHz startup...
    INIT_PRINTF(("sst1InitRegisters(): Setting PRELIM FT-CLK delay to 0x%x...\n", ft_clkdel));

    // SST_FBIINIT3_DEFAULT is set to disable texture mapping, so if
    // TMUs are hung, it will not affect FBI
    ISET(sst->fbiInit3,
       (SST_FBIINIT3_DEFAULT & ~(SST_FT_CLK_DEL_ADJ | SST_TF_FIFO_THRESH)) |
       (ft_clkdel << SST_FT_CLK_DEL_ADJ_SHIFT) |
       (tf_fifo_thresh << SST_TF_FIFO_THRESH_SHIFT));

    // Wait for Fbi-to-Trex clock delay value to propogate
    sst1InitReturnStatus(sstbase); // Stall - can't call IdleFbi because
    sst1InitReturnStatus(sstbase); // FBI could be hung at this stage
    sst1InitReturnStatus(sstbase);

    // Reset graphics and video units
    // Must reset video unit before graphics unit, otherwise video unit could
    // potentially hang waiting for the graphics unit to respond
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_VIDEO_RESET);

    // Don't wait for idle because graphics could be hung...
    sst1InitReturnStatus(sstbase);
    sst1InitReturnStatus(sstbase);
    sst1InitReturnStatus(sstbase);
    ISET(sst->fbiInit0,
      IGET(sst->fbiInit0) | (SST_GRX_RESET | SST_PCI_FIFO_RESET));

    sst1InitIdleFBINoNOP(sstbase);

    // Unreset PCI FIFO...
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_PCI_FIFO_RESET);
    sst1InitIdleFBINoNOP(sstbase);

    // Unreset graphics subsystem
    // THIS MUST BE PRESENT OTHERWISE THE PCI FIFO WILL NEVER DRAIN!
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) & ~SST_GRX_RESET);
    sst1InitIdleFBINoNOP(sstbase);

    // Reset all FBI and TREX Init registers
    ISET(sst->fbiInit0, SST_FBIINIT0_DEFAULT);
    ISET(sst->fbiInit1, SST_FBIINIT1_DEFAULT);
    ISET(sst->fbiInit2, SST_FBIINIT2_DEFAULT);
    ISET(sst->fbiInit3,
       (SST_FBIINIT3_DEFAULT & ~(SST_FT_CLK_DEL_ADJ | SST_TF_FIFO_THRESH)) |
       (ft_clkdel << SST_FT_CLK_DEL_ADJ_SHIFT) |
       (tf_fifo_thresh << SST_TF_FIFO_THRESH_SHIFT));
    ISET(sst->fbiInit4, SST_FBIINIT4_DEFAULT);
    ISET(sst->fbiInit5, SST_FBIINIT5_DEFAULT);
    ISET(sst->fbiInit6, SST_FBIINIT6_DEFAULT);
    sst1CurrentBoard->fbiInit6 = SST_FBIINIT6_DEFAULT;
    ISET(sst->fbiInit7, SST_FBIINIT7_DEFAULT);
    sst1InitIdleFBINoNOP(sstbase);  // Wait until init regs are reset

    // Enable writes to the FBIINIT registers and allow pushes onto PCI FIFO
    PCICFG_WR(SST1_PCI_INIT_ENABLE, (SST_INITWR_EN | SST_PCI_FIFOWR_EN));

    // Determine DAC Type
    if(sst1InitDacDetect(sstbase) == FXFALSE) {
        INIT_PRINTF(("sst1InitRegisters(): Could not detect DAC...\n"));
        return(FXFALSE);
    }

    // set TREX0 init values
    envp = GETENV(("SSTV2_TREX0INIT0"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX0INIT0 environment variable\n"));
        sst1CurrentBoard->tmuInit0[0] = i;
    } else
        sst1CurrentBoard->tmuInit0[0] = SST_TREX0INIT0_DEFAULT;

    INIT_PRINTF(("sst1InitRegisters(): Storing TREX0INIT0=0x%x\n",
        sst1CurrentBoard->tmuInit0[0]));
    envp = GETENV(("SSTV2_TREX0INIT1"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX0INIT1 environment variable\n"));
        sst1CurrentBoard->tmuInit1[0] = i;
    } else
        sst1CurrentBoard->tmuInit1[0] = SST_TREX0INIT1_DEFAULT;

    envp = GETENV(("SSTV2_PTF0_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &tf0_clkdel) == 1)) {
        sst1CurrentBoard->tmuInit1[0] = (sst1CurrentBoard->tmuInit1[0] &
            ~SST_TEX_TF_CLK_DEL_ADJ) |
            (tf0_clkdel<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    }
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX0INIT1=0x%x\n",
        sst1CurrentBoard->tmuInit1[0]));

    // set TREX1 init values
    envp = GETENV(("SSTV2_TREX1INIT0"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX1INIT0 environment variable\n"));
        sst1CurrentBoard->tmuInit0[1] = i;
    } else
        sst1CurrentBoard->tmuInit0[1] = SST_TREX1INIT0_DEFAULT;
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX1INIT0=0x%x\n",
        sst1CurrentBoard->tmuInit0[1]));
    envp = GETENV(("SSTV2_TREX1INIT1"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX1INIT1 environment variable\n"));
        sst1CurrentBoard->tmuInit1[1] = i;
    } else
        sst1CurrentBoard->tmuInit1[1] = SST_TREX1INIT1_DEFAULT;
    envp = GETENV(("SSTV2_PTF1_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &tf1_clkdel) == 1) ) {
        sst1CurrentBoard->tmuInit1[1] = (sst1CurrentBoard->tmuInit1[1] &
            ~SST_TEX_TF_CLK_DEL_ADJ) |
            (tf1_clkdel<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    }
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX1INIT1=0x%x\n",
        sst1CurrentBoard->tmuInit1[1]));

    // set TREX2 init values
    envp = GETENV(("SSTV2_TREX2INIT0"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX2INIT0 environment variable\n"));
        sst1CurrentBoard->tmuInit0[2] = i;
    } else
        sst1CurrentBoard->tmuInit0[2] = SST_TREX2INIT0_DEFAULT;
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX2INIT0=0x%x\n",
        sst1CurrentBoard->tmuInit0[2]));
    envp = GETENV(("SSTV2_TREX2INIT1"));
    if(envp && (SSCANF(envp, "%i", &i) == 1) ) {
        INIT_PRINTF(("sst1InitRegisters(): Using SST_TREX2INIT1 environment variable\n"));
        sst1CurrentBoard->tmuInit1[2] = i;
    } else
        sst1CurrentBoard->tmuInit1[2] = SST_TREX2INIT1_DEFAULT;
    envp = GETENV(("SSTV2_PTF2_CLKDEL"));
    if(envp && (SSCANF(envp, "%i", &tf2_clkdel) == 1) ) {
        sst1CurrentBoard->tmuInit1[2] = (sst1CurrentBoard->tmuInit1[2] &
            ~SST_TEX_TF_CLK_DEL_ADJ) |
            (tf2_clkdel<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    }
    INIT_PRINTF(("sst1InitRegisters(): Storing TREX2INIT1=0x%x\n", 
        sst1CurrentBoard->tmuInit1[2]));

    // Set clock at 16 MHz to properly transmit TREX configuration registers
    // Note that setting the clock will automatically reset the TMUs...
    // sst1InitResetTmus() also will de-assert TEXMAP_DISABLE in fbiInit3
    // unless SSTV2_TEXMAP_DISABLE is set
    if(!sst1InitComputeClkParams(16.0f, &sstGrxClk)) 
      return(FXFALSE);
    if(!sst1InitSetGrxClk(sstbase, &sstGrxClk))
      return(FXFALSE);
    sst1CurrentBoard->initGrxClkDone = 0;

    // Set PCI wait-states
    if(GETENV(("SSTV2_SLOWPCIWR"))) {
        INIT_PRINTF(("sst1InitRegisters(): Setting up SLOW PCI Writes...\n"));
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_PCI_WRWS_1);
    } else
        ISET(sst->fbiInit1, IGET(sst->fbiInit1) & ~SST_PCI_WRWS_1);

    if(GETENV(("SSTV2_FASTPCIRD"))) {
        INIT_PRINTF(("sst1InitRegisters(): Setting up FAST PCI Reads...\n"));
        ISET(sst->fbiInit4, IGET(sst->fbiInit4) & ~SST_PCI_RDWS_2);
    }
    sst1InitIdleFBINoNOP(sstbase);

    // Enable Linear frame buffer reads
    ISET(sst->fbiInit1, IGET(sst->fbiInit1) | SST_LFB_READ_EN);

    // Swapbuffer algorithm is based on VSync initially
    ISET(sst->fbiInit2, (IGET(sst->fbiInit2) & ~SST_SWAP_ALGORITHM) |
      SST_SWAP_VSYNC);

    // Enable LFB read-aheads
    ISET(sst->fbiInit4, IGET(sst->fbiInit4) | SST_EN_LFB_RDAHEAD);

    // Enable triangle alternate register mapping
    ISET(sst->fbiInit3, IGET(sst->fbiInit3) | SST_ALT_REGMAPPING);

    // Enable Endian Byte Swapping
    ISET(sst->fbiInit0, IGET(sst->fbiInit0) | SST_EN_ENDIAN_SWAPPING);

    // Setup DRAM Refresh
    ISET(sst->fbiInit2, (IGET(sst->fbiInit2) & ~SST_DRAM_REFRESH_CNTR) |
        SST_DRAM_REFRESH_16MS);
    sst1InitIdleFBINoNOP(sstbase);
    ISET(sst->fbiInit2, IGET(sst->fbiInit2) | SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    {
       // Setup memory timings...
       FxU32 fbiInit1 = IGET(sst->fbiInit1);
       FxU32 fbiInit2 = IGET(sst->fbiInit2) |
          SST_EN_DRAM_RD_AHEAD_FIFO | SST_EN_FAST_RD_AHEAD_WR;

       if(GETENV(("SSTV2_SLOWMEM_WTR"))) {
          INIT_PRINTF(("sst1InitRegisters(): Running slow write-to-read memory timing...\n"));
          fbiInit1 |= SST_DISEN_RD_AHEAD_WR_RD;
       }
       if(GETENV(("SSTV2_SLOWMEM_RTW"))) {
          INIT_PRINTF(("sst1InitRegisters(): Running slow read-to-write memory timing...\n"));
          fbiInit2 &= ~SST_EN_FAST_RD_AHEAD_WR;
       }
       if(GETENV(("SSTV2_FASTMEM_RAS_READS"))) {
          INIT_PRINTF(("sst1InitRegisters(): Enabling fast RAS read memory timing...\n"));
          fbiInit2 |= SST_EN_FAST_RAS_READ;
       }
       ISET(sst->fbiInit1, fbiInit1);
       sst1InitReturnStatus(sstbase);
       ISET(sst->fbiInit2, fbiInit2);
       sst1InitReturnStatus(sstbase);
    }

    // Return all other registers to their power-on state
    sst1InitIdleFBINoNOP(sstbase);
    sst1InitSetResolution(sstbase, &SST_VREZ_640X480_60, 0);
    sst1InitIdleFBINoNOP(sstbase);

    // Get DRAMs up and running (refresh is already enabled)...
    ISET(sst->c1, 0x0);
    ISET(sst->c0, 0x0);
    ISET(sst->zaColor, 0x0);
    ISET(sst->clipLeftRight, 100); // draw into 50x50 area
    ISET(sst->clipBottomTop, 100);
    ISET(sst->fbzMode, SST_RGBWRMASK | SST_ZAWRMASK);
    ISET(sst->fastfillCMD, 0x0);   // Frontbuffer & Z/A
    ISET(sst->nopCMD, 0x1); // Clear fbistat registers after clearing screen
    sst1InitIdleFBINoNOP(sstbase);

    sst1InitRenderingRegisters(sstbase);    
    sst1CurrentBoard->tmuRevision = 0xdead; // Force sst1InitFillDeviceInfo()
    if(sst1InitFillDeviceInfo(sstbase, sst1CurrentBoard) == FXFALSE) {
        INIT_PRINTF(("sst1InitRegisters(): ERROR filling DeviceInfo...\n"));

        return(FXFALSE);
    }

    sst1InitIdleFBINoNOP(sstbase);
    sst1InitRenderingRegisters(sstbase);
    sst1InitIdleFBINoNOP(sstbase);

    // LFB writes stored in memory FIFO?
    envp = GETENV(("SSTV2_MEMFIFO_LFB"));
    if(envp)
        n = ATOI(envp);
    else
        n = 1;
    if(n) {
        INIT_PRINTF(("sst1InitRegisters(): LFB Writes go through memory FIFO...\n"));
        ISET(sst->fbiInit0, IGET(sst->fbiInit0) | SST_EN_LFB_MEMFIFO);
        sst1InitIdleFBINoNOP(sstbase);
    }

    // Texture memory writes stored in memory FIFO?
    envp = GETENV(("SSTV2_MEMFIFO_TEX"));
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

    sst1InitIdleFBINoNOP(sstbase);  // Wait until init regs are reset

    // Update info structure for initEnable
    PCICFG_RD(SST1_PCI_INIT_ENABLE, sst1CurrentBoard->fbiInitEnable);

    INIT_PRINTF(("sst1InitRegisters(): exiting with status %d...\n", FXTRUE));
    return(FXTRUE);
}

FX_EXPORT void FX_CSTYLE sst1InitRenderingRegisters(FxU32 *sstbase)
{
    FxU32 i;
    SstRegs *sst = (SstRegs *) sstbase;

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
    ISET(sst->chromaRange, 0x0);
    ISET(sst->stipple, 0x0);
    ISET(sst->c0, 0x0);
    ISET(sst->c1, 0x0);

    ISET(sst->sSetupMode, 0x0);
    ISET(sst->sVx, 0x0);
    ISET(sst->sVy, 0x0);
    ISET(sst->sARGB, 0x0);
    ISET(sst->sRed, 0x0);
    ISET(sst->sGreen, 0x0);
    ISET(sst->sBlue, 0x0);
    ISET(sst->sAlpha, 0x0);
    ISET(sst->sVz, 0x0);
    ISET(sst->sOowfbi, 0x0);
    ISET(sst->sOow0, 0x0);
    ISET(sst->sSow0, 0x0);
    ISET(sst->sTow0, 0x0);
    ISET(sst->sOow1, 0x0);
    ISET(sst->sSow1, 0x0);
    ISET(sst->sTow1, 0x0);

    ISET(sst->nopCMD, 0x3);   // Clear fbistat and trianglesOut registers

    for(i=0; i<32; i++)
        ISET(sst->fogTable[i], 0x0);

    ISET(sst->bltSrcBaseAddr, 0x0);
    ISET(sst->bltDstBaseAddr, 0x0);
    ISET(sst->bltXYstrides, 0x0);
    ISET(sst->bltSrcChromaRange, 0x0);
    ISET(sst->bltDstChromaRange, 0x0);
    ISET(sst->bltClipX, 0x0);
    ISET(sst->bltClipY, 0x0);
    ISET(sst->bltSrcXY, 0x0);
    ISET(sst->bltDstXY, 0x0);
    ISET(sst->bltSize, 0x0);
    ISET(sst->bltRop, 0x0);
    ISET(sst->bltColor, 0x0);
    ISET(sst->bltCommand, 0x0);

    // clear TMU registers
    ISET(sst->textureMode, 0x0);
    ISET(sst->tLOD, 0x0);
    ISET(sst->tDetail, 0x0);
    ISET(sst->texBaseAddr, 0x0);
    ISET(sst->texBaseAddr1, 0x0);
    ISET(sst->texBaseAddr2, 0x0);
    ISET(sst->texBaseAddr38, 0x0);

    for(i=0; i<12; i++)
        ISET(sst->nccTable0[i], 0x0);
    for(i=0; i<12; i++)
        ISET(sst->nccTable1[i], 0x0);
    sst1InitIdle(sstbase);

    ISET(sst->tchromaKey, 0x0);
    ISET(sst->tchromaRange, 0x0);

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
    SstRegs *sstMaster = (SstRegs *) sstbase;
    SstRegs *sstSlave;
    SstRegs *sstPtr;
    FxU32 n;
    FxU32 sliEnabled;
    sst1ClkTimingStruct sstGrxClk;

    if(!sstbase)
        return(FXFALSE);

    if(sst1InitCheckBoard((FxU32 *) sstbase) == FXFALSE)
        return(FXFALSE);

    // sst1Initidle() routines must be properly executed...
    initIdleEnabled = 1;

    sliEnabled = (sst1CurrentBoard->sliSlaveVirtAddr != (FxU32 *) NULL) ? 1 : 0;
    sstSlave = (SstRegs *) sst1CurrentBoard->sliSlaveVirtAddr;

#if 0
    // If the command fifo is enabled, we can't issue a NOP first, so wait
    // until the PCI fifo is empty...
    sst1InitPciFifoIdleLoop(sstbase);
    if(sst1CurrentBoard->sliSlaveVirtAddr)
       sst1InitPciFifoIdleLoop(sst1CurrentBoard->sliSlaveVirtAddr);
#else
    sst1InitIdle(sstbase);
#endif

    // Disable the command fifo if enabled...
    if(sst1CurrentBoard->fbiCmdFifoEn ||
       (IGET(sstMaster->fbiInit7) & SST_EN_CMDFIFO))
       sst1InitCmdFifo(sstbase, FXFALSE, (void *) NULL, (void *) NULL,
        (void *) NULL, NULL);

    sst1InitIdle(sstbase);

    if(sliEnabled && !GETENV(("SSTV2_NOSHUTDOWN"))) {
        if(sst1InitShutdownSli(sstbase) == FXFALSE)
            return(FXFALSE);
    }

    if(GETENV(("SSTV2_NOSHUTDOWN"))) {
        INIT_PRINTF(("sst1InitShutdown(): Bypassing shutdown with SSTV2_NOSHUTDOWN\n"));
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

        // Reset video unit to guarantee no contentions on the memory bus
        // Blank DAC so VGA Passthru works properly
        ISET(sstPtr->fbiInit1,
          IGET(sstPtr->fbiInit1) | (SST_VIDEO_RESET | SST_VIDEO_BLANK_EN));
        // Turn off dram refresh to guarantee no contentions on the
        // memory bus
        ISET(sstPtr->fbiInit2, IGET(sstPtr->fbiInit2) & ~SST_EN_DRAM_REFRESH);

        // Reset graphics subsystem
        ISET(sstPtr->fbiInit0,
          IGET(sstPtr->fbiInit0) | (SST_GRX_RESET | SST_PCI_FIFO_RESET));
        sst1InitIdleFBINoNOP((FxU32 *) sstPtr);
        ISET(sstPtr->fbiInit0, IGET(sstPtr->fbiInit0) & ~SST_PCI_FIFO_RESET);

        sst1InitIdleFBINoNOP((FxU32 *) sstPtr);
        ISET(sstPtr->fbiInit0, IGET(sstPtr->fbiInit0) & ~SST_GRX_RESET);
        sst1InitIdleFBINoNOP((FxU32 *) sstPtr);

        // Turnaround VGA_PASS to allow VGA monitor
        sst1InitVgaPassCtrl((FxU32 *) sstPtr, 1);
        sst1InitIdleFBINoNOP((FxU32 *) sstPtr);

        // Set clock at 30 MHz to reduce power consumption...
        sst1InitComputeClkParams((float) 30.0, &sstGrxClk);
        if(sst1InitSetGrxClk((FxU32 *) sstPtr, &sstGrxClk) == FXFALSE)
          INIT_PRINTF(("sst1InitShutdown() WARNING: sst1InitSetGrxClk failed...Continuing...\n"));
        sst1CurrentBoard->initGrxClkDone = 0;

#ifndef __linux__
        pciUnmapPhysical((FxU32)sst1CurrentBoard->virtAddr[0], 0x1000000UL); 
#endif
        if((++n > 1) || !sliEnabled)
            break;
    }

    noshutdown: ;

    /* sst1InitIdle(sstbase);  */

#if !DIRECTX
    // 
    // HACK alert.
    //
    // There's a pciClose(), but nobody calls it. This is needed by the
    // DOS DPMI services to close fxmemmap.vxd.
    //
    // Note that the call to pciClose must be after the shutdown sequence,
    // as the shutdown sequence uses PCI configuration reads/writes...
    //
    pciClose();
#endif /* !DIRECTX */

    INIT_PRINTF(("sst1InitShutdown(): Returning with status %d...\n", FXTRUE));
#ifdef INIT_OUTPUT
    if ( sst1InitMsgFile != stdout )
        fclose(sst1InitMsgFile);
#endif

    // Make sure that the board info structures are
    // cleared next time sst1InitMapBoard() is called.
    clearBoardInfo = FXTRUE;

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
    FxBool returnVal = FXFALSE;

    for(n=0; n<boardsInSystem; n++) {
        if((FxU32 *) sst1BoardInfo[n].virtAddr[0] == sstbase) {
            sst1InitDeviceNumber = sst1BoardInfo[n].deviceNumber;
            sst1CurrentBoard = &sst1BoardInfo[n];
            returnVal = FXTRUE;
            n = boardsInSystem;
        }
    }
    return(returnVal);
}

/*
** sst1InitGetBaseAddr():
**  Return virtual base address to hardware
**  Returns NULL if board requested has not been mapped
**
*/
FX_EXPORT FxU32 * FX_CSTYLE sst1InitGetBaseAddr(FxU32 boardNum)
{
  SstRegs* sstBaseAddr = sst1BoardInfo[boardNum].virtAddr[0];
  sst1DeviceInfoStruct devInfo;

  if ((sstBaseAddr != NULL) &&
      sst1InitGetDeviceInfo((FxU32*)sstBaseAddr, &devInfo) &&
      devInfo.sliDetected &&
      !devInfo.monitorDetected) {
    
    SstRegs* tempAddr = NULL;

    if (boardNum > 0) {
      if (sst1BoardInfo[boardNum - 1].sliSlaveVirtAddr == (FxU32*)sstBaseAddr) {
        tempAddr = sst1BoardInfo[boardNum - 1].virtAddr[0];
      }
    }

    if ((tempAddr == NULL) && (boardNum < boardsInSystem)) {
      if (sst1BoardInfo[boardNum + 1].sliSlaveVirtAddr == (FxU32*)sstBaseAddr) {
        tempAddr = sst1BoardInfo[boardNum + 1].virtAddr[0];
      }
    }

    if (tempAddr != NULL) sstBaseAddr = tempAddr;
  }
  
  return (FxU32*)sstBaseAddr;
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
        sst1BoardInfo[n].size = sizeof(sst1DeviceInfoStruct);
        //sst1BoardInfo[n].virtAddr[0] = (SstRegs *) NULL;
        //sst1BoardInfo[n].physAddr[0] = (FxU32) NULL;
        //sst1BoardInfo[n].deviceNumber = 0xdead;
        //sst1BoardInfo[n].vendorID = 0xdead;
        //sst1BoardInfo[n].deviceID = 0xdead;

        //sst1BoardInfo[n].fbiRevision = 0xdead;
        //sst1BoardInfo[n].fbiFab = 0xdead;
        sst1BoardInfo[n].fbiBoardID = 0xdead;
        sst1BoardInfo[n].fbiVideo16BPP = 0xdead;
        sst1BoardInfo[n].fbiVideoWidth = 0xdead;
        sst1BoardInfo[n].fbiVideoHeight = 0xdead;
        sst1BoardInfo[n].fbiVideoRefresh = 0xdead;
        sst1BoardInfo[n].fbiVideoColBuffs = 0xdead;
        sst1BoardInfo[n].fbiVideoAuxBuffs = 0xdead;
        sst1BoardInfo[n].fbiVideoMemOffset = 0xdead;
        sst1BoardInfo[n].fbiVideoTilesInX = 0xdead;
        sst1BoardInfo[n].fbiVideoStruct = (sst1VideoTimingStruct *) NULL;
        sst1BoardInfo[n].fbiVideoDacType = 0xdead;
        sst1BoardInfo[n].fbiMemoryFifoEn = 0x0;
        sst1BoardInfo[n].fbiCmdFifoEn = 0x0;
        sst1BoardInfo[n].fbiLfbLocked = 0x0;
        sst1BoardInfo[n].fbiConfig = 0xdead;
        sst1BoardInfo[n].fbiGrxClkFreq = 0xdead;
        sst1BoardInfo[n].fbiMemSize = 0x0;
        sst1BoardInfo[n].fbiInitGammaDone = 0x0;
        sst1BoardInfo[n].fbiGammaRed = (double) 0.0;
        sst1BoardInfo[n].fbiGammaGreen = (double) 0.0;
        sst1BoardInfo[n].fbiGammaBlue = (double) 0.0;

        sst1BoardInfo[n].tmuRevision = 0xdead;
        sst1BoardInfo[n].numberTmus = 0xdead;
        sst1BoardInfo[n].tmuConfig = 0xdead;
        sst1BoardInfo[n].tmuGrxClkFreq = 0xdead;

        for(j=0; j<MAX_NUM_TMUS; j++) {
            sst1BoardInfo[n].tmuFab[j] = 0xdead;
            sst1BoardInfo[n].tmuMemSize[j] = 0x0;
            sst1BoardInfo[n].tmuInit0[j] = 0xdead;
            sst1BoardInfo[n].tmuInit1[j] = 0xdead;
        }
        sst1BoardInfo[n].fbiInit6 = 0;
        sst1BoardInfo[n].fbiInitEnable = 0;

        sst1BoardInfo[n].sliDetected = 0;
        sst1BoardInfo[n].monitorDetected = 0;
        sst1BoardInfo[n].sliSlaveVirtAddr = (FxU32 *) NULL;
        sst1BoardInfo[n].initGrxClkDone = 0;
        sst1BoardInfo[n].vgaPassthruDisable = 0;
        sst1BoardInfo[n].vgaPassthruEnable = 0;
        sst1BoardInfo[n].memFifoStatusLwm = 0xdead;
    }
}

/*
** sst1InitNumBoardsInSystem():
**  Returns the number of SST-1 boards in the system
**
*/
FX_ENTRY FxU32 FX_CSTYLE sst1InitNumBoardsInSystem(void)
{
    FxU32 vendorID = _3DFX_PCI_ID;     // 3Dfx Vendor ID
    FxU32 deviceID;                    // 0x0002 - Find Voodoo2 boards ( 0xFFFF - Find any 3Dfx board)
    FxU32 j, n;
    const char *envp;

    envp = GETENV(("SSTV2_DEVICEID"));
    if(envp)
      deviceID = ATOI(envp);
    else
      deviceID = 0x0002;

    boardsInSystemReally = 0;
    for(j=0; j<SST1INIT_MAX_BOARDS; j++) {
       if(pciFindCardMulti(vendorID, deviceID, &n, j))
           boardsInSystemReally++;
    }
    envp = GETENV(("SSTV2_BOARDS"));
    if(envp)
       return ATOI(envp);
    else
       return(boardsInSystemReally);
}

/*
** sst1InitCaching
**
** Sets up memory caching on P6 class systems and K7 systems for the
** lower 8 MB of the 16 MB virtual address space of the card. These
** hold the command fifo area.
**
*/
FX_ENTRY FxBool FX_CSTYLE 
sst1InitCaching(FxU32* sstBase, FxBool enableP)
{
  FxBool retVal = sst1InitCheckBoard(sstBase);

  if (!retVal) return FXFALSE;

  if (enableP && (GETENV("SSTV2_IGNORE_CACHING") == NULL)) {
    FxU32 physAddr;
    
    /* Get the board's base. Isn't this the same as what we
     * cary around in sst1CurrentBoard->physAddr[0]?
     */
    pciGetConfigData(PCI_BASE_ADDRESS_0, sst1CurrentBoard->deviceNumber, &physAddr);
    
    // For some reason, there sometimes is a 008 at the end of the
    // physical address, so mask that puppy RTF out
    physAddr &= 0xfffff000;

    /* dpc - 23 jan 1998 - Warning!!!!!
     * We are no longer mapping the entire board range as uswc because
     * this causes weird problems on some systems. We are now mapping
     * the 3d register area as uncacheable (since no one should be
     * writing here anyway, except to bump the swapbufferCMD register)
     * and the command fifo range and lfb range as uswc.
     *
     * NB: It is still unclear why this is happening because the
     * memory page containing the registers is never really explicitly
     * written to until we are shutting down so changing its caching
     * characteristics should have no bearing on anything.  */
    {
#define kCacheSizeWriteCombine (0x08UL << 20UL)
#define kCacheSizeUncacheable  (0x1000UL)
      FxBool hasWC = pciFindMTRRMatch(physAddr, kCacheSizeWriteCombine,
                                      PciMemTypeWriteCombining, 
                                      &sst1CurrentBoard->mtrrWriteCombine);
      FxBool hasUC = pciFindMTRRMatch(physAddr, kCacheSizeUncacheable,
                                      PciMemTypeUncacheable,
                                      &sst1CurrentBoard->mtrrUncacheable);

      if (!hasWC) {
        if (pciFindFreeMTRR(&sst1CurrentBoard->mtrrWriteCombine)) {
          hasWC = pciSetMTRR(sst1CurrentBoard->mtrrWriteCombine,
                             physAddr, kCacheSizeWriteCombine,
                             PciMemTypeWriteCombining);
          if (!hasWC) INIT_PRINTF(("sst1InitCaching(): Could not set wc mtrr\n"));
        } else {
          INIT_PRINTF(("sst1InitCaching(): Could not find wc mtrr\n"));
        }
      } else {
        INIT_PRINTF(("sst1InitCaching(): Already have wc\n"));
      }

      if (!hasUC) {
        if (pciFindFreeMTRR(&sst1CurrentBoard->mtrrUncacheable)) {
          hasUC = pciSetMTRR(sst1CurrentBoard->mtrrUncacheable,
                             physAddr, kCacheSizeUncacheable,
                             PciMemTypeUncacheable);
          if (!hasUC) INIT_PRINTF(("sst1InitCaching(): Could not set uc mtrr\n"));
        } else {
          INIT_PRINTF(("sst1InitCaching(): Could not find uc mtrr\n"));
        }
      } else {
        INIT_PRINTF(("sst1InitCaching(): Already have uc\n"));
      }

      /* We only succeed if we have them both since having only uswc
       * seems to cause problems.  
       */
      retVal = (hasWC && hasUC);
      if (!retVal) goto __errExit;

      INIT_PRINTF(("sst1InitCaching(): wc(0x%X) : uc(0x%X)\n",
                   sst1CurrentBoard->mtrrWriteCombine,
                   sst1CurrentBoard->mtrrUncacheable));
    }
  } else {
    /* This always succeeds */
    retVal = FXTRUE;

__errExit:
    if (sst1CurrentBoard->mtrrUncacheable != 0) {
      pciSetMTRR(sst1CurrentBoard->mtrrUncacheable, 0, 0, 0);
      sst1CurrentBoard->mtrrUncacheable = 0;
    }

    if (sst1CurrentBoard->mtrrWriteCombine != 0) {
      pciSetMTRR(sst1CurrentBoard->mtrrWriteCombine, 0, 0, 0);
      sst1CurrentBoard->mtrrWriteCombine = 0;
    }
  }

  return retVal;
} // sst1InitSetCaching



/*
** sst1InitCachingAMD
**
** Sets up memory caching for AMD K6 and AMD K7 systems for the entire
** 16mb virtual address space of the card. For K7, goes through
** sst1InitCaching
**
*/

#define kCacheSizeWriteCombineAMD (0x08UL << 20UL)      /* 8 MB   */
#define kCacheSizeUncacheableAMD  (0x20000UL)           /* 128 KB */

FX_ENTRY FxBool FX_CSTYLE
sst1InitCachingAMD(FxU32* sstBase, FxBool enableP, FxBool hasP2MTRR)
{
  FxBool retVal = sst1InitCheckBoard(sstBase);

  if (!retVal) return FXFALSE;

  /* The K7 has MTRRs that are exactly compatible to the P6/PII, so
     we just do the work through the existing sst1InitCaching function
  */

  if (hasP2MTRR) { 
     return sst1InitCaching(sstBase, enableP);
  }

  /* If it's not a K7, we must have a K6 with WC support, i.e. a K6-2 with
     CXT core or a Sharptooth, otherwise we wouldn't have gotten here.
  */

  if (enableP && (GETENV("SSTV2_IGNORE_CACHING") == NULL)) {
    FxU32 physAddr;
    
    /* Get the board's base. Isn't this the same as what we
     * carry around in sst1CurrentBoard->physAddr[0]?
     */

    pciGetConfigData(PCI_BASE_ADDRESS_0, sst1CurrentBoard->deviceNumber, &physAddr);
    
    /* For some reason, there sometimes is a 008 at the end of the
     * physical address, so mask that puppy RTF out
     */

    physAddr &= 0xfffff000;

    /* dpc - 23 jan 1998 - Warning!!!!!
     * We are no longer mapping the entire board range as uswc because
     * this causes weird problems on some systems. We are now mapping
     * the 3d register area as uncacheable (since no one should be
     * writing here anyway, except to bump the swapbufferCMD register)
     * and the command fifo range and lfb range as uswc.
     *
     * NB: It is still unclear why this is happening because the
     * memory page containing the registers is never really explicitly
     * written to until we are shutting down so changing its caching
     * characteristics should have no bearing on anything.
     *
     * This might no be necessary for K6, as K6 maintains strong write
     * ordering in WC regions. We do this here compatible to PII just in
     * case. Note: Overlapping WC and UC regions results in UC type on K6,
     * just like on PII. Note: the smallest MTRR region size on K6 is 128 KB,
     * while on PII it is 4 KB. We make the UC region as small as we can.
     */
   
    /* For K6, we don't do any elaborate allocation, as it only has two
     * variable size memory regions, and we need both for Voodoo2. So, we
     * just jam the data into the two available MTRRs.
     */

    pciSetMTRRAmdK6 (0, physAddr, kCacheSizeWriteCombineAMD, PciMemTypeWriteCombining);
    pciSetMTRRAmdK6 (1, physAddr, kCacheSizeUncacheableAMD, PciMemTypeUncacheable);
    
  } else {  /* disable MTRRs */

    /* This always succeeds */

    retVal = FXTRUE;

    /* Since AMD recommends a "free for all" policy for allocating MTRRs
       on K6, we do nothing for "disable". Any software that wants to grab
       the MTRRs can just do so. Leaving the MTRR enabled might actually
       beneficial in a situation like the following: We run a D3D app, then
       a Glide app, then a D3D app, but the second time around D3D doesn't
       re-initialize, since it's still active, so it doesn't reprogram the
       MTRRs.
     */

    // pciSetMTRRAmdK6 (0, 0, 0, 0);    /* size == 0 implies disable */
    // pciSetMTRRAmdK6 (1, 0, 0, 0);    /* size == 0 implies disable */

  }

  return retVal;
} // sst1InitSetCachingAMD

#ifdef _MSC_VER
#pragma optimize ("",on)
#endif
