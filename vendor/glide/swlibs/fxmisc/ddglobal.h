/*
 * ddglobal.h placeholder for Glide3x (SST1, SST96, CVG)
 *
 *  Copyright (c) 2004 - Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */

#ifndef DDGLOBAL_H_included

#define ddgetenv(A)       getenv(A)
#define ddatoi(A)         atoi(A)
#define ddftol(A)         ((long)(A))
#define dd_itof_inv(A)    (1.0f/(float)(A))
#define ddatof(A)         atof(A)
#define ddpow(A, B)       pow(A, B)
#define ddsscanf(A, B, C) sscanf(A, B, C)

/*
 * Perhaps this file should be personalized for each ASIC,
 * and then moved into the respective glide3x branches...
 */
#ifndef __CVGINFO_H__

#if defined(SST1) || defined(SST96_FIFO)

#include <sst1vid.h>

#else  /* !SST1 && !SST96_FIFO */

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
struct sstregs; /* see cvgregs.h */
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

#endif /* !SST1 && !SST96_FIFO */

#endif /* !__CVGINFO_H__ */

#endif
