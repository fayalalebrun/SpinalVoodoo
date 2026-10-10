#ifndef __CVGINFO_H__
#define __CVGINFO_H__

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
*/

/*
#if defined(__unix__) && ! defined(__CVGREGS_H__)
// basic data types
#define FxU32 unsigned int
#define FxBool int
// defn of registers not reqd, treat (SstRegs *) as (void *)
typedef void SstRegs;
#endif
*/
struct sstregs; /* see cvgregs.h */

#define MAX_NUM_TMUS 3

// Video timing data structure
typedef struct {
    FxU32 hSyncOn;
    FxU32 hSyncOff;
    FxU32 vSyncOn;
    FxU32 vSyncOff;
    FxU32 hBackPorch;
    FxU32 vBackPorch;
    FxU32 xDimension;
    FxU32 yDimension;
    FxU32 refreshRate;
    FxU32 miscCtrl;
    FxU32 memOffset;
    FxU32 tilesInX;
    FxU32 vFifoThreshold;
    FxBool video16BPPIsOK;
    FxBool video24BPPIsOK;
    float clkFreq16bpp;
    float clkFreq24bpp;
} sst1VideoTimingStruct;

/* Init code client callbacks to allow the init code to use the client
 * command fifo management code to do writes etc.
 */
typedef void (*FxSet32Proc)(volatile FxU32* const addr, const FxU32 val);

/*
** CVG Device Information Structure
**
*/
// Initialization and configuration data structure
typedef struct {
    FxU32 size;                     // size of this structure
    volatile 
    struct sstregs *virtAddr[2];    // virtual memory base address 
    FxU32 physAddr[2];              // physical memory base address 
    FxU32 deviceNumber;             // PCI device number 
    FxU32 vendorID;                 // PCI vendor ID 
    FxU32 deviceID;                 // PCI device ID 

    FxU32 fbiRevision;              // FBI revision number 
    FxU32 fbiFab;                   // FBI Fab ID
    FxU32 fbiBoardID;               // FBI board ID (poweron strapping bits) 
    FxU32 fbiVideo16BPP;            // FBI video display mode 
    FxU32 fbiVideoWidth;            // FBI video display X-resolution 
    FxU32 fbiVideoHeight;           // FBI video display Y-resolution 
    FxU32 fbiVideoRefresh;          // FBI video refresh rate 
    FxU32 fbiVideoColBuffs;         // FBI video number of color buffers
    FxU32 fbiVideoAuxBuffs;         // FBI video number of Aux buffers
    FxU32 fbiVideoMemOffset;        // FBI video memory offset (in pages) 
    FxU32 fbiVideoTilesInX;         // FBI video memory 32x32 tiles-in-X 
    sst1VideoTimingStruct           // FBI video resolution
      *fbiVideoStruct;              //  data structure 
    FxU32 fbiVideoDacType;          // FBI video dac type detected
    FxU32 fbiMemoryFifoEn;          // FBI Memory Fifo enabled 
    FxU32 fbiCmdFifoEn;             // FBI Command Fifo enabled 
    FxU32 fbiLfbLocked;             // FBI frame buffer is locked
    FxU32 fbiConfig;                // FBI strapping pins 
    FxU32 fbiGrxClkFreq;            // FBI graphics clock frequency
    FxU32 fbiMemSize;               // FBI frame buffer memory (in MBytes) 
    FxU32 fbiInitGammaDone;         // FBI gamma table initialized
    double fbiGammaRed;             // FBI Red gamma value
    double fbiGammaGreen;           // FBI Green gamma value
    double fbiGammaBlue;            // FBI Blue gamma value
    FxU32 fbiNandTree;              // FBI Nand tree delay value
    FxU32 fbiNorTree;               // FBI Nor tree delay value

    FxU32 tmuRevision;              // TMU revision number (for all TMUs) 
    FxU32 tmuFab[MAX_NUM_TMUS];     // TMU Fab ID (for all TMUs) 
    FxU32 numberTmus;               // Number of TMUs installed 
    FxU32 tmuConfig;                // TMU configuration bits 
    FxU32 tmuGrxClkFreq;            // TMU graphics clock frequency
    FxU32 tmuMemSize[MAX_NUM_TMUS]; // TMU texture memory (in MBytes) 

    // Registers which cannot be read from the hardware, so we shadow them here 
    FxU32 tmuInit0[MAX_NUM_TMUS];   // TMU initialization registers
    FxU32 tmuInit1[MAX_NUM_TMUS];   // TMU initialization registers
    FxU32 fbiInit6;                 // FBI initialization register
    FxU32 fbiInitEnable;            // FBI PCI Configuration register initEnable

    // Misc 
    FxU32 sliDetected;              // Scanline interleave detected 
    FxU32 sliPaired;                // Board part of an sli pair
    FxU32 monitorDetected;          // Monitor connection detected
    FxU32 *sliSlaveVirtAddr;        // Slave virtual address
                                    // Set to 0 if SLI is not enabled...
    FxU32 initGrxClkDone;           // Grapics clock has been initializated
    FxU32 vgaPassthruDisable;       // Value to force SST-1 control of monitor
    FxU32 vgaPassthruEnable;        // Value to force VGA control of monitor
    FxU32 memFifoStatusLwm;

    // Client setter callbacks
    FxSet32Proc set32;

    // PCI library stuff
    FxU32 mtrrUncacheable;          /* 3d register space (all wraps) */
    FxU32 mtrrWriteCombine;         /* command fifo/3d lfb           */

    volatile struct sstregs *sstCSIM;
    volatile struct sstregs *sstHW; // pointer to HW
} sst1DeviceInfoStruct;

typedef sst1DeviceInfoStruct FxDeviceInfo;

#endif /* !__CVGINFO_H__ */
