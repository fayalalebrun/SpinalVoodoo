#ifndef __H3INFO_H__
#define __H3INFO_H__

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
#if defined(__unix__) && ! defined(__H3REGS_H__)
// basic data types
#define FxU8  unsigned char
#define FxU16 unsigned short
#define FxU32 unsigned int
#define FxBool int
// defn of registers not reqd, treat (SstRegs *) as (void *)
typedef void SstRegs;
#endif
*/
struct sstregs; /* see h3regs.h */

/*
** H3 Device Information Structure
**
*/

#define MAX_NUM_TMUS_SUPPORTED 4
#if MAX_NUM_TMUS > MAX_NUM_TMUS_SUPPORTED
#   error "need to increase MAX_NUM_TMUS_SUPPORTED"
#endif

typedef struct {		// H3 Device Information Structure
    FxU32 size;                 // size of this structure
    volatile 
    struct sstregs *virtAddr[2];    // virtual memory base address
    FxU32 physAddr[2];		    // physical memory base address
    FxU16 virtPort;             // virtual i/o port base address
    FxU16 physPort;             // physical i/o port base address
    FxU32 deviceNumber;		    // device number
                                // This isn't the real device number. It's
                                // some composite of the
                                // bus number and the device
    FxU32 pciBusNumber;         // PCI bus number
    FxU32 pciDeviceNumber;      // PCI device number
    FxU32 pciFunctionNumber;    // PCI function number
    FxU32 vendorID;		        // PCI vendor ID
    FxU32 deviceID;		        // PCI device ID
    FxU32 fbiRevision;		    // FBI revision number
    FxU32 fbiConfig;		    // FBI strapping pins
    FxU32 fbiMemType;		    // FBI memory type (poweron strapping bits)
    FxU32 fbiVideoWidth;	    // FBI video display X-resolution
    FxU32 fbiVideoHeight;	    // FBI video display Y-resolution
    FxU32 fbiVideoRefresh;	    // FBI video refresh rate
    FxU32 fbiMemoryFifoEn;	    // FBI memory fifo enabled
    FxU32 tmuRevision;		    // TMU revision number (for all TMUs)
    FxU32 numberTmus;		    // number of TMUs installed
    FxU32 tmuConfig;		    // TMU configuration bits
    FxU32 fbiMemSize;		    // FBI frame buffer memory (in MBytes)
    FxU32 tmuMemSize[MAX_NUM_TMUS_SUPPORTED]; 	// TMU texture memory (in MBytes)
    FxU8 *agpMem;               // AGP true base address
    FxU8 *agpVirtAddr;          // AGP virtual base address
    FxU32 agpSizeInBytes;       // AGP memory size (in Bytes)
    FxU32 agpBaseAddrH;         // upper 4 bits of AGP physical base address
    FxU32 agpBaseAddrL;         // lower 32 bits of AGP physical base address
    FxU32 agpRqDepth;           // AGP request depth

    FxBool parentDevice;        // Is this the primary Napalm?

    // These cannot be read from the hardware, so we shadow them here
    FxU32 tmuInit0[MAX_NUM_TMUS_SUPPORTED];
    FxU32 tmuInit1[MAX_NUM_TMUS_SUPPORTED];

    // Misc
    FxU32 initGrxClkDone;

    // CSIM specific
    volatile struct sstregs *sstCSIM;		// pointer to CSIM structure
    volatile struct sstregs *sstHW;		// pointer to HW
} FxDeviceInfo;

#endif /* !__H3INFO_H__ */
