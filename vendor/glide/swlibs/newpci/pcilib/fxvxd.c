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

#define WIN32_LEANER_AND_MEANER
#include <windows.h>
#include <conio.h>

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <fxmemmap.h>
#include "fxpci.h"
#include "pcilib.h"
#include "fxw32.h"

/* Callback declarations */
static FxBool pciInitialize9x(void);
static FxBool pciShutdown9x(void);
static const char* pciIdentifier9x(void);

static FxU8  pciPortInByte9x(FxU16 port);
static FxU16 pciPortInWord9x(FxU16 port);
static FxU32 pciPortInLong9x(FxU16 port);
  
static FxBool pciPortOutByte9x(FxU16 port, FxU8 data);
static FxBool pciPortOutWord9x(FxU16 port, FxU16 data);
static FxBool pciPortOutLong9x(FxU16 port, FxU32 data);

static FxBool pciMapLinear9x(FxU32 busNumber, FxU32 physAddr,
                             unsigned long* linearAddr, FxU32* length);
static FxBool pciUnmapLinear9x(unsigned long linearAddr, FxU32 length);

static FxBool pciSetPermission9x(const unsigned long addrBase, const FxU32 addrLen,
                                 const FxBool writePermP);

static FxBool pciMsrGet9x(MSRInfo* in, MSRInfo* out);
static FxBool pciMsrSet9x(MSRInfo* in, MSRInfo* out);

static FxBool pciOutputString9x(const char* msg);
static FxBool pciSetPassThroughBase9x(FxU32* baseAddr, FxU32 baseAddrLen);

static char pciIdent[] = "@#% fxPCI for Windows 9x";

static const FxPlatformIOProcs __ioProcs9x = {
  pciInitialize9x,
  pciShutdown9x,
  pciIdentifier9x,

  pciPortInByte9x,
  pciPortInWord9x,
  pciPortInLong9x,
  
  pciPortOutByte9x,
  pciPortOutWord9x,
  pciPortOutLong9x,

  pciMapLinear9x,
  pciUnmapLinear9x,
  pciSetPermission9x,

  pciMsrGet9x,
  pciMsrSet9x,

  pciOutputString9x,
  pciSetPassThroughBase9x
};
const FxPlatformIOProcs* ioProcs9x = &__ioProcs9x;

/* Basic platform init/shutdown stuff */
static FxBool
pciInitialize9x(void)
{
  hMemmapFile = CreateFile("\\\\.\\FXMEMMAP.VXD", 0, 0, NULL, 0,
                           FILE_FLAG_DELETE_ON_CLOSE, NULL);
  if ( hMemmapFile == INVALID_HANDLE_VALUE ) {
    pciErrorCode = PCI_ERR_MEMMAPVXD;
    return FXFALSE;
  }

  return FXTRUE;
}

static FxBool
pciShutdown9x(void)
{
  FxBool
    retVal = (hMemmapFile != INVALID_HANDLE_VALUE);

  if (retVal) CloseHandle( hMemmapFile );
  return retVal;
}

static const char* 
pciIdentifier9x(void)
{
  return pciIdent;
}

/* Device address space management stuff */

static FxBool
pciMapLinear9x(FxU32 busNumber, FxU32 physical_addr,
               unsigned long *linear_addr, FxU32 *length)
{
  FxU32 nret;
  FxU32 Physical [2];         /* Physical address[0] & size[1] */
  FxU32 Linear [2];           /* Linear address[0] & size[1] */
  LPDWORD pPhysical = Physical;
  LPDWORD pLinear = Linear;
  
  Physical[0] = physical_addr;
  Physical[1] = *length;
  
  if ( !pciLibraryInitialized ) {
    pciErrorCode = PCI_ERR_NOTOPEN;
    return FXFALSE;
  }
  
#ifndef DIRECTX
  /*
   * Check version:
   * The policy is that major and minor versions must match, and
   * further that the reference count is less than or equal to one. 
   */
  DeviceIoControl(hMemmapFile, GETAPPVERSIONDWORD, NULL, 0, &pciVxdVer, 
                  sizeof(pciVxdVer), &nret, NULL);
  if (HIBYTE(pciVxdVer) != FX_MAJOR_VER || LOBYTE(pciVxdVer) < FX_MINOR_VER) {
    pciErrorCode = PCI_ERR_WRONGVXD;
    return FXFALSE;
  }
  
#if 0
  /* Check for mutual exclusion. Turned off everywhere for now because
   * the DirectDraw driver does not bother to do anything w/ the mutex
   * so no one else knows whether or not the hw is taken. Mmmm.... whacked.
   */
  if (VXDREFCOUNT(pciVxdVer) > 1) {
    pciErrorCode = PCI_ERR_VXDINUSE;
    return FXFALSE;
  }
#endif
#endif /* !DIRECTX */
  
  /* Map physical to linear */
  /* xxx - returns 0 in Linear if fails, but really should
     check return value, 0 is suceess, -1 is failure. */
  
#ifndef DIRECTX
  /* Stuff added to auto-switch passthru using fxmemmap */
  if (getenv("SST_DUALHEAD") == NULL) {
    DeviceIoControl(hMemmapFile, GETLINEARADDR_AUTO, 
                    &pPhysical, sizeof(pPhysical), 
                    &pLinear, sizeof(pLinear), 
                    &nret, NULL);
  } else 
#endif /* !DIRECTX */
  {
    DeviceIoControl(hMemmapFile, GETLINEARADDR, 
                    &pPhysical, sizeof(pPhysical), 
                    &pLinear, sizeof(pLinear), 
                    &nret, NULL);
  }
  
  *linear_addr = Linear[0];
  
  if ( nret == 0 ) {
    pciErrorCode = PCI_ERR_MEMMAP;
    return FXFALSE;
  }

  return FXTRUE;
}

static FxBool
pciUnmapLinear9x( unsigned long linear_addr, FxU32 length ) 
{
  FxU32 nret;
  
  return DeviceIoControl(hMemmapFile, DECREMENTMUTEX,
                         NULL, 0, 
                         NULL, 0,
                         &nret, NULL);
}

/* Platform port io stuff */
#ifdef  __GNUC__				/* __MINGW32__ */
#define _inp  _inp_asm
static inline unsigned char _inp_asm (unsigned short _port) {
  unsigned char rc;
  __asm__ __volatile__ ("inb %w1,%b0" : "=a" (rc) : "Nd" (_port));
  return rc;
}
#define _inpw _inpw_asm
static inline unsigned short _inpw_asm (unsigned short _port) {
  unsigned short rc;
  __asm__ __volatile__ ("inw %w1,%w0" : "=a" (rc) : "Nd" (_port));
  return rc;
}
#define _inpd _inpd_asm
static inline unsigned long _inpd_asm (unsigned short _port) {
  unsigned long rc;
  __asm__ __volatile__ ("inl %w1,%0" : "=a" (rc) : "Nd" (_port));
  return rc;
}
#define _outp  _outp_asm
static inline unsigned char _outp_asm (unsigned short _port, unsigned char _data) {
  __asm__ __volatile__ ("outb %b0,%w1" : : "a" (_data), "Nd" (_port));
  return _data;
}
#define _outpw _outpw_asm
static inline unsigned short _outpw_asm (unsigned short _port, unsigned short _data) {
  __asm__ __volatile__ ("outw %w0,%w1" : : "a" (_data), "Nd" (_port));
  return _data;
}
#define _outpd _outpd_asm
static inline unsigned long _outpd_asm (unsigned short _port, unsigned long _data) {
  __asm__ __volatile__ ("outl %0,%w1"  : : "a" (_data), "Nd" (_port));
  return _data;
}
#endif

static FxU8
pciPortInByte9x(FxU16 port)
{
  return _inp(port);
}

static FxU16
pciPortInWord9x(FxU16 port)
{
  return _inpw(port);
}

static FxU32
pciPortInLong9x(FxU16 port)
{
  return _inpd(port);
}
  
static FxBool
pciPortOutByte9x(FxU16 port, FxU8 data)
{
  return _outp(port, data);
}

static FxBool
pciPortOutWord9x(FxU16 port, FxU16 data)
{
  return _outpw(port, data);
}

static FxBool
pciPortOutLong9x(FxU16 port, FxU32 data)
{
  return _outpd(port, data);
}

static FxBool pciMsrGet9x(MSRInfo* in, MSRInfo* out)
{
  FxU32 nret; 

  return DeviceIoControl( hMemmapFile, GETMSR, 
                          in, sizeof(*in), 
                          out, sizeof(*out), 
                          &nret, NULL); 
}

static FxBool pciMsrSet9x(MSRInfo* in, MSRInfo* out)
{
  FxU32 nret; 

  return DeviceIoControl( hMemmapFile, SETMSR, 
                          in, sizeof(*in), 
                          out, sizeof(*out), 
                          &nret, NULL); 
}

/* Platform utilities. */
static FxBool
pciOutputString9x(const char* msg)
{
  FxBool retVal = FXTRUE;

  if (pciLibraryInitialized) {
    OutputDebugString(msg);
  } else {
    pciErrorCode = PCI_ERR_NOTOPEN;
    retVal = FXFALSE;
  }

  return retVal;
}

/* Ganked from vmm.h */
#define PC_USER         0x00040000  /* make the pages ring 3 accessible */

static FxBool
pciSetPermission9x(const unsigned long addrBase, const FxU32 addrLen,
                   const FxBool writePermP)
{
  FxU32 vxdParamArray[] = {
    addrBase,
    addrLen,
    0
  };
  FxU32 nRet = 0;
  
  /* Set the user accessable bit. We don't dork w/ the
   * rest of the bits.
   */
  vxdParamArray[2] = (writePermP ? PC_USER : 0);
  
  return DeviceIoControl(hMemmapFile, SETADDRPERM,
                         vxdParamArray, sizeof(vxdParamArray),
                         NULL, 0,
                         &nRet, NULL);
}

static FxBool
pciSetPassThroughBase9x(FxU32* baseAddr, FxU32 baseAddrLen)
{
  FxU32 vxdParams[] = { (FxU32)baseAddr, baseAddrLen };
  FxU32 nRet = 0;
  
  return DeviceIoControl(hMemmapFile, SETPASSTHROUGHBASE, 
                         vxdParams, sizeof(vxdParams),
                         NULL, 0, 
                         &nRet, NULL);
}

