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

#ifndef _PCILIB_H_
#define _PCILIB_H_

#define CONFIG_ADDRESS_PORT 0xCF8
#define CONFIG_DATA_PORT    0xCFC
#define CONFIG_ADDRESS_ENABLE_BIT 0x80000000UL

#define CONFIG_MAPPING_ENABLE_BYTE 0x80
#define CONFIG_MAPPING_DISABLE_BYTE 0x00

#define CONFIG_SPACE_ENABLE_PORT 0xCF8
#define FORWARD_REGISTER_PORT    0xCFA
#define CONFIG_MAPPING_OFFSET    0xC000

#define PCI_INTERRUPT  0x1A
#define PCI_FUNC_CODE  0xB1
#define PCI_PHYS_ADDR  0x000FFE6E

#define BYTE0(a) ((a) & 0xff)
#define BYTE1(a) (((a) >> 8) & 0xff)
#define VXDREFCOUNT(a) (((a) >> 16) & 0xff)

typedef struct {
  FxU32
    msrNum,                     /* Which MSR? */
    msrLo, msrHi;               /* MSR Values  */
} MSRInfo;

/* PRIVATE DATA */
extern FxU32   pciVxdVer;
extern FxU32   pciErrorCode;
extern FxBool  pciLibraryInitialized;
extern PciHwcCallbacks pciHwcCallbacks;

typedef struct {
  /* Platform specific init/shutdown stuff. */
  FxBool (*ioInit)(void);
  FxBool (*ioShutdown)(void);
  const char* (*ioIdent)(void);

  /* Port i/o */
  FxU8  (*portInByte)(FxU16 port);
  FxU16 (*portInWord)(FxU16 port);
  FxU32 (*portInLong)(FxU16 port);
  
  FxBool (*portOutByte)(FxU16 port, FxU8 data);
  FxBool (*portOutWord)(FxU16 port, FxU16 data);
  FxBool (*portOutLong)(FxU16 port, FxU32 data);

  /* Platform device address management */
  FxBool (*addrMap)(FxU32 busNumber, FxU32 physAddr,
                    unsigned long* linearAddr, FxU32* length);
  FxBool (*addrUnmap)(unsigned long linearAddr, FxU32 length);

  /* Optional things that a platform may or maynot support and clients
   * should not rely on the call to suceed.  
   */
  FxBool (*addrSetPermission)(const unsigned long addrBase, const FxU32 addrLen,
                              const FxBool writePermP);

  FxBool (*msrGet)(MSRInfo* in, MSRInfo* out);
  FxBool (*msrSet)(MSRInfo* in, MSRInfo* out);

  FxBool (*utilOutputString)(const char* msg);
  FxBool (*utilSetPassThroughBase)(FxU32* baseAddr, FxU32 baseAddrLen);
} FxPlatformIOProcs;

extern const FxPlatformIOProcs* gCurPlatformIO;

#define pciInitializeDDio             (*gCurPlatformIO->ioInit)
#define pciCloseDDio                  (*gCurPlatformIO->ioShutdown)
#define pciPlatformIdentifier         (*gCurPlatformIO->ioIdent)

#define pciMapLinearDD                (*gCurPlatformIO->addrMap)
#define pciUnmapLinearDD              (*gCurPlatformIO->addrUnmap)
#define pciLinearRangeSetPermissionDD (*gCurPlatformIO->addrSetPermission)

#define pciOutputDebugStringDD        (*gCurPlatformIO->utilOutputString)
#define pciSetPassThroughBaseDD       (*gCurPlatformIO->utilSetPassThroughBase)

#define _pioInByte                    (*gCurPlatformIO->portInByte)
#define _pioInWord                    (*gCurPlatformIO->portInWord)
#define _pioInLong                    (*gCurPlatformIO->portInLong)

#define _pioOutByte                   (*gCurPlatformIO->portOutByte)
#define _pioOutWord                   (*gCurPlatformIO->portOutWord)
#define _pioOutLong                   (*gCurPlatformIO->portOutLong)

#define DOGETMSR(__in, __out)         (*gCurPlatformIO->msrGet)(&(__in), &(__out))
#define DOSETMSR(__in, __out)         (*gCurPlatformIO->msrSet)(&(__in), &(__out))

/* One should be defined per real platform (currently dos/win32) to
 * setup the platform function pointer struct.  
 */
extern FxBool pciPlatformInit(void);

#endif /* _PCILIB_H_ */
