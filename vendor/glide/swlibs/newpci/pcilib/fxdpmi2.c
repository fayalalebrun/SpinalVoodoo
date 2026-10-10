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

#include <stdlib.h>
#include <stdio.h>

#include <conio.h>

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include "fxpci.h"
#include "pcilib.h"

#include <fxdpmi.h>

#ifdef __DJGPP__
#include <crt0.h>
#include <dpmi.h>
#include <sys/nearptr.h>
static FxBool dirty;
#elif defined(__WATCOMC__)
#include <i86.h>
#else
#include <fxmemmap.h>
#endif

/* Callback declarations */
static FxBool pciInitializeDPMI(void);
static FxBool pciShutdownDPMI(void);
static const char* pciIdentifierDPMI(void);

static FxU8  pciPortInByteDPMI(FxU16 port);
static FxU16 pciPortInWordDPMI(FxU16 port);
static FxU32 pciPortInLongDPMI(FxU16 port);
  
static FxBool pciPortOutByteDPMI(FxU16 port, FxU8 data);
static FxBool pciPortOutWordDPMI(FxU16 port, FxU16 data);
static FxBool pciPortOutLongDPMI(FxU16 port, FxU32 data);

static FxBool pciMapLinearDPMI(FxU32 busNumber, FxU32 physAddr,
                             FxU32* linearAddr, FxU32* length);
static FxBool pciUnmapLinearDPMI(FxU32 linearAddr, FxU32 length);

static FxBool pciSetPermissionDPMI(const FxU32 addrBase, const FxU32 addrLen,
                                 const FxBool writePermP);

static FxBool pciMsrGetDPMI(MSRInfo* in, MSRInfo* out);
static FxBool pciMsrSetDPMI(MSRInfo* in, MSRInfo* out);

static FxBool pciOutputStringDPMI(const char* msg);
static FxBool pciSetPassThroughBaseDPMI(FxU32* baseAddr, FxU32 baseAddrLen);

static char pciIdent[] = "@#% fxPCI for DOS";

static const FxPlatformIOProcs __ioProcsDPMI = {
  pciInitializeDPMI,
  pciShutdownDPMI,
  pciIdentifierDPMI,

  pciPortInByteDPMI,
  pciPortInWordDPMI,
  pciPortInLongDPMI,
  
  pciPortOutByteDPMI,
  pciPortOutWordDPMI,
  pciPortOutLongDPMI,

  pciMapLinearDPMI,
  pciUnmapLinearDPMI,
  pciSetPermissionDPMI,

  pciMsrGetDPMI,
  pciMsrSetDPMI,

  pciOutputStringDPMI,
  pciSetPassThroughBaseDPMI
};
const FxPlatformIOProcs* ioProcsDPMI = &__ioProcsDPMI;

FxBool 
pciPlatformInit(void)
{
  gCurPlatformIO = ioProcsDPMI;
  return FXTRUE;
}

/* Basic platform init/shutdown stuff */
static FxBool
pciInitializeDPMI(void)
{
#ifdef __DJGPP__
 /* enable nearptr access */
 if (_crt0_startup_flags & _CRT0_FLAG_NEARPTR) {
    dirty = FXFALSE;
 } else {
    if (__djgpp_nearptr_enable() == 0)
       return FXFALSE;

    dirty = FXTRUE;
 }
#endif
  return FXTRUE;
}

static FxBool
pciShutdownDPMI(void)
{
#ifdef __DJGPP__
 if (dirty) {
    __djgpp_nearptr_disable();
    dirty = FXFALSE;
 }
#endif
  return FXTRUE;
}

static const char* 
pciIdentifierDPMI(void)
{
  return pciIdent;
}

/* Device address space management stuff */

static FxBool
pciMapLinearDPMI(FxU32 busNumber, FxU32 physical_addr,
               FxU32 *linear_addr, FxU32 *length)
{
#ifdef __DJGPP__
 __dpmi_meminfo meminfo;

 if (physical_addr >= 0x100000) {
    /* map into linear memory */
    meminfo.address = physical_addr;
    /* Hack alert:
     * because of the TILE shit, we must enhance the mapped area
     */
    meminfo.size = *length * 3 / 2;
    if (__dpmi_physical_address_mapping(&meminfo) != 0)
       return FXFALSE;

    *linear_addr = meminfo.address - __djgpp_base_address;
 } else {
    /* exploit 1 -> 1 physical to linear mapping in low megabyte */
    *linear_addr = physical_addr;
 }
#elif defined(__WATCOMC__)
 if (physical_addr >= 0x100000) {
    union REGS r;
    /* Hack alert:
     * because of the TILE shit, we must enhance the mapped area
     */
    FxU32 len = *length * 3 / 2;

    /* function 0x800 (Physical Address Mapping) */
    r.w.ax = 0x800;

    /*
    ** BX:CX = physical address
    ** SI:DI = length
    */
    r.w.bx = ( FxU16 ) ( physical_addr >> 16 );
    r.w.cx = ( FxU16 ) ( physical_addr & 0x0000FFFF );
    r.w.si = ( FxU16 ) ( len >> 16 );
    r.w.di = ( FxU16 ) ( len & 0x0000FFFF );
    int386( 0x31, &r, &r );

    /* if cflag set then an error occured */
    if ( r.w.cflag ) {
      return FXFALSE;
    }
    *linear_addr = (r.w.bx << 16) | r.w.cx;
 } else {
    /* exploit 1 -> 1 physical to linear mapping in low megabyte */
    *linear_addr = physical_addr;
 }
#else
  FxBool onWindows;

  /*  
  **  First, check to see if we're a DOS app under Windows, and if
  **  so, then check to see if there's already an app connected to
  **  the VXD.
  *
  * NB: To be compatible w/ the 'previous' pci libraries this is only
  * done the first time that we're mapping a board otherwise it will
  * fail on the vxd ref count because any previous mappings bump the
  * reference count.  
  */
  {
    static FxBool didVxdCheck = FXFALSE;

    if (!didVxdCheck) {
      DpmiCheckVxd((FxBool *) &onWindows, &pciVxdVer);
      didVxdCheck = FXTRUE;

      if (onWindows) {
        if (BYTE1(pciVxdVer) != FX_MAJOR_VER || BYTE0(pciVxdVer) < FX_MINOR_VER) {
          pciErrorCode = PCI_ERR_WRONGVXD;
          return FXFALSE;
        }
          
        if (VXDREFCOUNT(pciVxdVer) > 0) {
          pciErrorCode = PCI_ERR_VXDINUSE;
          return FXFALSE;
        }
      }
    }
  }

  /* If we got here, it's OK to map the memory */
  *linear_addr = DpmiMapPhysicalToLinear( physical_addr, *length );
#endif

 return FXTRUE;
}

static FxBool
pciUnmapLinearDPMI( FxU32 linear_addr, FxU32 length ) 
{
#ifdef __DJGPP__
 __dpmi_meminfo meminfo;

 if (linear_addr >= 0x100000) {
    meminfo.address = linear_addr + __djgpp_base_address;
    if (!__dpmi_free_physical_address_mapping(&meminfo)) {
       return FXTRUE;
    }
 }

 return FXFALSE;
#elif defined(__WATCOMC__)
 union REGS r;

 /* function 0x801 (Free Physical Address Mapping) */
 r.w.ax = 0x801;

 /* BX:CX = physical address */
 r.w.bx = ( FxU16 ) ( linear_addr >> 16 );
 r.w.cx = ( FxU16 ) ( linear_addr & 0x0000FFFF );
 int386( 0x31, &r, &r );

 return (r.w.cflag == 0);
#else
  DpmiUnmapMemory(linear_addr, length);
#endif
}

/* Platform port io stuff */
static FxU8
pciPortInByteDPMI(FxU16 port)
{
  return inp(port);
}

static FxU16
pciPortInWordDPMI(FxU16 port)
{
  return inpw(port);
}

static FxU32
pciPortInLongDPMI(FxU16 port)
{
  return inpd(port);
}
  
static FxBool
pciPortOutByteDPMI(FxU16 port, FxU8 data)
{
  outp(port, data);
  return FXTRUE;
}

static FxBool
pciPortOutWordDPMI(FxU16 port, FxU16 data)
{
  outpw(port, data);
  return FXTRUE;
}

static FxBool
pciPortOutLongDPMI(FxU16 port, FxU32 data)
{
  outpd(port, data);
  return FXTRUE;
}

static FxBool 
pciMsrGetDPMI(MSRInfo* in, MSRInfo* out)
{
#if defined(__DJGPP__) || defined(__WATCOMC__)
  return FXTRUE;
#else
  return DpmiGetMSR((FxU32)in, (FxU32)out);
#endif
}

static FxBool 
pciMsrSetDPMI(MSRInfo* in, MSRInfo* out)
{
#if defined(__DJGPP__) || defined(__WATCOMC__)
  return FXTRUE;
#else
  return DpmiSetMSR((FxU32)in, (FxU32)out);
#endif
}

/* Platform utilities. */
static FxBool
pciOutputStringDPMI(const char* msg)
{
#if defined(__DJGPP__) || defined(__WATCOMC__)
 printf("%s", msg);
 return FXTRUE;
#else
  return DpmiOutputDebugString(msg);
#endif
}

static FxBool
pciSetPermissionDPMI(const FxU32 addrBase, const FxU32 addrLen,
                   const FxBool writePermP)
{
#if defined(__DJGPP__) || defined(__WATCOMC__)
  return FXTRUE;
#else
  return DpmiLinearRangeSetPermission(addrBase, addrLen, writePermP);
#endif
}

static FxBool
pciSetPassThroughBaseDPMI(FxU32* baseAddr, FxU32 baseAddrLen)
{
#if defined(__DJGPP__) || defined(__WATCOMC__)
  return FXTRUE;
#else
  return DpmiSetPassThroughBase(baseAddr, baseAddrLen);
#endif
}

