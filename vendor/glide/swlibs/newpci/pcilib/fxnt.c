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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <winioctl.h>

#include <gpioctl.h>
#include <fxptl.h>

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>

#include "fxpci.h"
#include "pcilib.h"
#include "fxw32.h"

/* ------------------------------------------------------------- */
/* Memmap and portio through Microsoft's sample mapmem miniport  */

static HANDLE hGpdFile;

/* Callback declarations */
static FxBool pciInitializeNT(void);
static FxBool pciShutdownNT(void);
static const char* pciIdentifierNT(void);

static FxU8  pciPortInByteNT(FxU16 port);
static FxU16 pciPortInWordNT(FxU16 port);
static FxU32 pciPortInLongNT(FxU16 port);
  
static FxBool pciPortOutByteNT(FxU16 port, FxU8 data);
static FxBool pciPortOutWordNT(FxU16 port, FxU16 data);
static FxBool pciPortOutLongNT(FxU16 port, FxU32 data);

static FxBool pciMapLinearNT(FxU32 busNumber, FxU32 physAddr,
                             unsigned long* linearAddr, FxU32* length);
static FxBool pciUnmapLinearNT(unsigned long linearAddr, FxU32 length);

static FxBool pciSetPermissionNT(const unsigned long addrBase, const FxU32 addrLen,
                                 const FxBool writePermP);

static FxBool pciMsrGetNT(MSRInfo* in, MSRInfo* out);
static FxBool pciMsrSetNT(MSRInfo* in, MSRInfo* out);

static FxBool pciOutputStringNT(const char* msg);
static FxBool pciSetPassThroughBaseNT(FxU32* baseAddr, FxU32 baseAddrLen);

static char pciIdent[] = "@#% fxPCI for Windows NT";

static const FxPlatformIOProcs __ioProcsNT = {
  pciInitializeNT,
  pciShutdownNT,
  pciIdentifierNT,

  pciPortInByteNT,
  pciPortInWordNT,
  pciPortInLongNT,
  
  pciPortOutByteNT,
  pciPortOutWordNT,
  pciPortOutLongNT,

  pciMapLinearNT,
  pciUnmapLinearNT,
  pciSetPermissionNT,

  pciMsrGetNT,
  pciMsrSetNT,

  pciOutputStringNT,
  pciSetPassThroughBaseNT
};
const FxPlatformIOProcs* ioProcsNT = &__ioProcsNT;

/* Basic platform init/shutdown stuff */
static FxBool
pciInitializeNT(void)
{
  hGpdFile = INVALID_HANDLE_VALUE;
  hMemmapFile = INVALID_HANDLE_VALUE;

  hGpdFile = CreateFile("\\\\.\\GpdDev", 
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, 
                        OPEN_EXISTING, 
                        0, 
                        NULL);
  if (hGpdFile == INVALID_HANDLE_VALUE) {
    pciErrorCode = PCI_ERR_GENPORT;
    return FXFALSE;
  }
  
  hMemmapFile = CreateFile("\\\\.\\MAPMEM", 
                           GENERIC_READ | GENERIC_WRITE,
                           0, 
                           NULL, 
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, 
                           NULL);
  if (hMemmapFile == INVALID_HANDLE_VALUE) {
    pciErrorCode = PCI_ERR_MAPMEMDRV;
    return FXFALSE;
  }  
  
  return FXTRUE;
}

static FxBool
pciShutdownNT(void)
{
  if (hGpdFile != INVALID_HANDLE_VALUE) CloseHandle(hGpdFile);
  if (hMemmapFile != INVALID_HANDLE_VALUE) CloseHandle(hMemmapFile);

  return FXTRUE;
}

static const char* 
pciIdentifierNT(void)
{
  return pciIdent;
}

/* Device address space management stuff */

static FxBool
pciMapLinearNT(FxU32 busNumber, FxU32 physical_addr,
               unsigned long *linear_addr, FxU32 *length)
{
  FxU32 cbReturned;
  PHYSICAL_MEMORY_INFO pmi;
  
  pmi.InterfaceType       = PCIBus;
  pmi.BusNumber           = busNumber;
  pmi.BusAddress.HighPart = 0x00000000;
  pmi.BusAddress.LowPart  = physical_addr;
  pmi.AddressSpace        = 0;
  pmi.Length              = *length;
  
  if(!DeviceIoControl(hMemmapFile,
                      (FxU32)IOCTL_MAPMEM_MAP_USER_PHYSICAL_MEMORY,
                      &pmi, sizeof(PHYSICAL_MEMORY_INFO),
                      linear_addr, sizeof(PVOID),
                      &cbReturned, NULL)) {
    pciErrorCode = PCI_ERR_MAPMEM;
    return FXFALSE;
  }

  return FXTRUE;
}

static FxBool
pciUnmapLinearNT(unsigned long linear_addr, FxU32 length) 
{
  FxU32                cbReturned;
  
  return DeviceIoControl(hMemmapFile,
                         (FxU32)IOCTL_MAPMEM_UNMAP_USER_PHYSICAL_MEMORY,
                         &linear_addr, sizeof(PVOID),
                         NULL, 0,
                         &cbReturned, NULL);
}

/* Platform port io stuff */

FxU8 
pciPortInByteNT (unsigned short port)
{
  BOOL   IoctlResult;
  LONG   IoctlCode;
  ULONG  PortNumber;
  UCHAR  DataBuffer;
  ULONG  DataLength;
  ULONG  ReturnedLength;

  IoctlCode = IOCTL_GPD_READ_PORT_UCHAR;
  PortNumber = port;
  DataLength = sizeof(DataBuffer);

  IoctlResult = DeviceIoControl(hGpdFile, IoctlCode,
                                &PortNumber, sizeof(PortNumber), 
                                &DataBuffer, DataLength,
                                &ReturnedLength, NULL); 

  if (IoctlResult && ReturnedLength == DataLength)
    return DataBuffer;
  else
    return FXFALSE;
} /* pioInByte */

FxU16
pciPortInWordNT (unsigned short port)
{
  BOOL   IoctlResult;
  LONG   IoctlCode;
  ULONG  PortNumber;
  USHORT DataBuffer;
  ULONG  DataLength;
  ULONG  ReturnedLength;

  IoctlCode = IOCTL_GPD_READ_PORT_USHORT;
  PortNumber = port;
  DataLength = sizeof(DataBuffer);

  IoctlResult = DeviceIoControl(hGpdFile, IoctlCode, 
                                &PortNumber, sizeof(PortNumber), 
                                &DataBuffer, DataLength,
                                &ReturnedLength, NULL); 
  
  if (IoctlResult && ReturnedLength == DataLength)
    return DataBuffer;
  else
    return FXFALSE;
} /* pioInWord */

FxU32
pciPortInLongNT (unsigned short port)
{
  BOOL   IoctlResult;
  LONG   IoctlCode;
  ULONG  PortNumber;
  ULONG  DataBuffer;
  ULONG  DataLength;
  ULONG  ReturnedLength;

  IoctlCode = IOCTL_GPD_READ_PORT_ULONG;
  PortNumber = port;
  DataLength = sizeof(DataBuffer);

  IoctlResult = DeviceIoControl(hGpdFile, IoctlCode, 
                                &PortNumber, sizeof(PortNumber),
                                &DataBuffer, DataLength, 
                                &ReturnedLength, NULL);

  if (IoctlResult && ReturnedLength == DataLength)
    return DataBuffer;
  else
    return FXFALSE;
} /* pioInLong */

FxBool
pciPortOutByteNT (unsigned short port, FxU8 data)
{
  LONG                IoctlCode;
  GENPORT_WRITE_INPUT InputBuffer;
  ULONG               DataLength;
  ULONG               ReturnedLength;

  IoctlCode = IOCTL_GPD_WRITE_PORT_UCHAR;
  InputBuffer.PortNumber = port;
  InputBuffer.CharData = (UCHAR) data;
  DataLength = (offsetof(GENPORT_WRITE_INPUT, CharData) +
                sizeof(InputBuffer.CharData));

  return DeviceIoControl(hGpdFile, IoctlCode, 
                         &InputBuffer, DataLength,
                         NULL, 0, 
                         &ReturnedLength, NULL);
} /* pioOutByte */

FxBool
pciPortOutWordNT (unsigned short port, FxU16 data)
{
  LONG                IoctlCode;
  GENPORT_WRITE_INPUT InputBuffer;
  ULONG               DataLength;
  ULONG               ReturnedLength;

  IoctlCode = IOCTL_GPD_WRITE_PORT_USHORT;
  InputBuffer.PortNumber = port;
  InputBuffer.ShortData = (USHORT) data;
  DataLength = (offsetof(GENPORT_WRITE_INPUT, ShortData) +
                sizeof(InputBuffer.ShortData));

  return DeviceIoControl(hGpdFile, IoctlCode, 
                         &InputBuffer, DataLength,
                         NULL, 0, 
                         &ReturnedLength, NULL);
} /* pioOutWord */

FxBool
pciPortOutLongNT (unsigned short port, FxU32 data)
{
  LONG                IoctlCode;
  GENPORT_WRITE_INPUT InputBuffer;
  ULONG               DataLength;
  ULONG               ReturnedLength;

  IoctlCode = IOCTL_GPD_WRITE_PORT_ULONG;
  InputBuffer.PortNumber = port;
  InputBuffer.LongData = (ULONG) data;
  DataLength = (offsetof(GENPORT_WRITE_INPUT, LongData) +
                sizeof(InputBuffer.LongData));

  return DeviceIoControl(hGpdFile, IoctlCode, 
                         &InputBuffer, DataLength,
                         NULL, 0, 
                         &ReturnedLength, NULL);
} /* pioOutLong */

static FxBool 
pciMsrGetNT(MSRInfo* in, MSRInfo* out)
{
  ULONG retLen;

  return DeviceIoControl(hMemmapFile, (FxU32)IOCTL_MAPMEM_GET_MSR,
                         in, sizeof(*in),
                         out, sizeof(*out),
                         &retLen, NULL);
}

static FxBool 
pciMsrSetNT(MSRInfo* in, MSRInfo* out)
{
  ULONG retLen;

  return DeviceIoControl(hMemmapFile, (FxU32)IOCTL_MAPMEM_SET_MSR,
                         in, sizeof(*in),
                         out, sizeof(*out),
                         &retLen, NULL);
}

/* Platform utilities. */
static FxBool
pciOutputStringNT(const char* msg)
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

static FxBool
pciSetPermissionNT(const unsigned long addrBase, const FxU32 addrLen,
                   const FxBool writePermP)
{
  return FXFALSE;
}

static FxBool
pciSetPassThroughBaseNT(FxU32* baseAddr, FxU32 baseAddrLen)
{
  return FXFALSE;
}



