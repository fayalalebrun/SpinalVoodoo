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

#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <3dfx.h>
#include <glidesys.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <fxmemmap.h>
#include "fxpci.h"
#include "pcilib.h"
#if (GLIDE_PLATFORM & GLIDE_OS_UNIX)
#ifdef SIM_BACKEND
#include "fxsim.h"
#elif defined(DE10_BACKEND)
#include "fxde10.h"
#else
#include "fxlinux.h"
#endif
#endif

/* PRIVATE DATA (within the library) */
FxU32   pciVxdVer = 0;
FxU32   pciErrorCode = PCI_ERR_NOERR;
FxBool  pciLibraryInitialized  = FXFALSE;
PciHwcCallbacks pciHwcCallbacks = { 1, NULL, NULL, NULL, NULL, NULL, NULL };

const FxPlatformIOProcs* gCurPlatformIO = NULL;

/* 
 * linear Address Map List
 *
 * This is the new way we keep track of boards which are already mapped 
 */
static struct {
  FxU32
    device_bus_func_number;
  struct {
    FxBool
    mapped;
    unsigned long
    addr;
  } addrList[MAX_PCI_BASEADDRESSES];
} linearAddressMapList[MAX_PCI_DEVICES];

typedef struct _PCIErr {
  FxU32 code;
  char *string;
} PCIErr, *PCIErrPtr;

static PCIErr pciError[] = {
  {PCI_ERR_NOERR, "No errors.\n"},
  {PCI_ERR_WINRTINIT, "WinRT initialization failure.\n" },
  {PCI_ERR_MEMMAPVXD, "Memmap VxD initialization failure.\n" },
  {PCI_ERR_MAPMEMDRV, "Mapmem driver initialization failure.\n" },
  {PCI_ERR_GENPORT, "Genport I/O initialization failure.\n" },
  {PCI_ERR_NO_BUS, "No PCI Bus detected.\n"},
  {PCI_ERR_NOTOPEN, "PCI library not open.\n" },
  {PCI_ERR_NOTOPEN2, "Closing unopened PCI library.\n" },
  {PCI_ERR_NOTOPEN3, "pciGetConfigData() on unopened library.\n" }, 
  {PCI_ERR_OUTOFRANGE, "Device_number is out of range.\n" },
  {PCI_ERR_NODEV, "Cannot read from a non-existant device.\n" },
  {PCI_ERR_NODEV2, "Cannot update config regs from non-existant device.\n" },
  {PCI_ERR_WRITEONLY, "Cannot read a WRITE_ONLY register.\n" },
  {PCI_ERR_READONLY, "Cannot write a READ_ONLY register.\n" },
  {PCI_ERR_PHARLAP, "Phar Lap returned an error trying to map memory.\n" },
  {PCI_ERR_WRONGVXD, "Expected VxD version V%d.%d, got V%d.%d\n"},
  {PCI_ERR_MEMMAP, "Memmap returned an error trying to map memory.\n" },
  {PCI_ERR_MAPMEM, "Mapmem returned an error trying to map memory.\n" },
  {PCI_ERR_WINRT, "Winrt returned an error trying to map memory.\n" },
  {PCI_ERR_VXDINUSE, "Mutual exclusion prohibits this\n" },
  {PCI_ERR_NO_IO_PERM, "Permission denied.\n"
   "Couldn't change I/O priveledge level.\n"},
  {PCI_ERR_NO_MEM_PERM, "Permission denied.\n"
   "Couldn't access memory mapp.\n"}
};

/* -------------------------------------------------- */
/* Port IO subroutines - each layer has it's own      */
FX_ENTRY FxU8 FX_CALL
pioInByte ( unsigned short port )
{
  FxU8 data = 0;

  if (pciHwcCallbacks.pioInByte)
    data = pciHwcCallbacks.pioInByte(port);
  if (pciHwcCallbacks.doHW)
    data = _pioInByte( port );
  return data;
}

FX_ENTRY FxU16 FX_CALL
pioInWord ( unsigned short port )
{
  FxU16 data = 0;

  if (pciHwcCallbacks.pioInWord)
    data = pciHwcCallbacks.pioInWord(port);
  if (pciHwcCallbacks.doHW)
    data = _pioInWord( port );
  return data;
}

FX_ENTRY FxU32 FX_CALL
pioInLong ( unsigned short port )
{
  FxU32 data = 0;

  if (pciHwcCallbacks.pioInLong)
    data = pciHwcCallbacks.pioInLong(port);
  if (pciHwcCallbacks.doHW)
    data = _pioInLong( port );
  return data;
}

FX_ENTRY FxBool FX_CALL
pioOutByte ( unsigned short port, FxU8 data )
{
  if (pciHwcCallbacks.pioOutByte)
    pciHwcCallbacks.pioOutByte(port,data);
  if (pciHwcCallbacks.doHW)
    _pioOutByte( port, data );
  return FXTRUE;
}

FX_ENTRY FxBool FX_CALL
pioOutWord ( unsigned short port, FxU16 data )
{
  if (pciHwcCallbacks.pioOutWord)
    pciHwcCallbacks.pioOutWord(port,data);
  if (pciHwcCallbacks.doHW)
    _pioOutWord( port, data );
  return FXTRUE;
}

FX_ENTRY FxBool FX_CALL
pioOutLong ( unsigned short port, FxU32 data )
{
  if (pciHwcCallbacks.pioOutLong)
    pciHwcCallbacks.pioOutLong(port,data);
  if (pciHwcCallbacks.doHW)
    _pioOutLong( port, data );
  return FXTRUE;
}

static PciRegister    baseAddresses[MAX_PCI_BASEADDRESSES];
static FxU32          vendorIDs[MAX_PCI_DEVICES];
static FxU32          configMechanism     = 0;
static FxBool         busDetected         = FXFALSE;
static FxBool         deviceExists[MAX_PCI_DEVICES];

/* PRIVATE FUNCTIONS */
static FxU32
_pciCreateConfigAddress( FxU32 bus_number, FxU32 device_number,  
                         FxU32 function_number, FxU32 register_offset )
{ 
  FxU32 retval = CONFIG_ADDRESS_ENABLE_BIT;
  
  retval |= ( bus_number & 0xFF ) << 16;
  retval |= ( device_number & 0x1F ) << 11;
  retval |= ( function_number & 0x7 ) << 8;
  retval |= ( register_offset & 0xFC );
  return retval;
} /* _pciCreateConfigAddress */



static FxU16 
_pciCreateConfigSpaceMapping( FxU32 device_number, FxU32
                              register_offset )
{

  FxU16 retval = 0;
  retval |= ( device_number & 0xFF ) << 8;
  retval |= ( register_offset & 0xFC );
  retval += CONFIG_MAPPING_OFFSET;
  return retval;
} /* _pciCreateConfigSpaceMapping */



static FxU32 FX_CALL
_pciFetchRegister( FxU32 offset, FxU32 size_in_bytes, 
                   FxU32 device_number, FxU32 config_mechanism  )
{ 
  FxU32 retval;
  FxU32 slot, bus, function;
  
  bus  = (device_number >> 5) & 0xFF;
  slot = device_number & 0x1f;
  function = (device_number >> 13) & 7;
  
  if ( config_mechanism == 1 ) {
    pioOutLong( CONFIG_ADDRESS_PORT, _pciCreateConfigAddress( bus, slot, function, offset ) );
    retval = pioInLong( CONFIG_DATA_PORT );
    retval >>= 8 * ( offset & 0x3 );
  } else {                      /* config mechanism 2 */
    pioOutByte( CONFIG_ADDRESS_PORT, CONFIG_MAPPING_ENABLE_BYTE );
    retval = pioInLong( _pciCreateConfigSpaceMapping( device_number, offset ) ); 
    retval >>= 8 * ( offset & 0x3 );
    pioOutByte( CONFIG_ADDRESS_PORT, CONFIG_MAPPING_DISABLE_BYTE );
  }
  
  switch( size_in_bytes ) {
  case 1:
    retval &= 0xFF;
    break;
  case 2:
    retval &= 0xFFFF;
    break;
  default:                      /* 4 bytes */
    break;
  }
  
  return retval;
} /* _pciFetchRegister */



static void FX_CALL
_pciUpdateRegister( FxU32 offset, FxU32 data, FxU32 size_in_bytes,  
                    FxU32 device_number, FxU32 config_mechanism  ) 
{
  FxU32
    regval =  _pciFetchRegister( offset & ( ~0x3 ), 4,
                                 device_number, config_mechanism );
  FxU32 mask = (FxU32) ~0l;
  FxU32 bus, slot, function;
  
  bus  = (device_number >> 5) & 0xff;
  slot = device_number & 0x1f;
  function = (device_number >> 13) & 7;
  
  switch( size_in_bytes ) {
  case 1:
    mask &= 0xFF;
    data &= 0xFF;
    break;
  case 2:
    mask &= 0xFFFF;
    data &= 0xFFFF;
    break;
  case 4:
  default:
    break;
  }
  
  data <<= 8 * ( offset & 0x03 );
  mask <<= 8 * ( offset & 0x03 );
  
  regval = ( regval & ~mask ) | data;
  
  if ( config_mechanism == 1 ) {
    pioOutLong( CONFIG_ADDRESS_PORT, _pciCreateConfigAddress( bus, slot, function, offset ) );
    pioOutLong( CONFIG_DATA_PORT, regval );
  } else {                      /* config mechanism 2 */
    pioOutByte( CONFIG_ADDRESS_PORT, CONFIG_MAPPING_ENABLE_BYTE );
    pioOutLong( _pciCreateConfigSpaceMapping( device_number, offset ), regval );
    pioOutByte( CONFIG_ADDRESS_PORT, CONFIG_MAPPING_DISABLE_BYTE );
  }
  
  return;
} /* _pciUpdateRegister */

static unsigned long
find_mapped_address(FxU32 device_bus_func_number, FxU32 addrNum) 
{
  FxU32 
    i;
  unsigned long
    retVal = 0x00UL;

  for(i = 0; i < MAX_PCI_DEVICES; i++) {
    if ((linearAddressMapList[i].device_bus_func_number == device_bus_func_number) &&
        linearAddressMapList[i].addrList[addrNum].mapped) {
      retVal = linearAddressMapList[i].addrList[addrNum].addr;

      break;
    }
  }

  return retVal;
}

static void 
set_mapped_address(FxU32 device_bus_func_number, FxU32 addrNum, unsigned long value) 
{
  FxU32 i;
  
  for(i = 0; i < MAX_PCI_DEVICES; i++) {
    if (linearAddressMapList[i].device_bus_func_number == device_bus_func_number) {
      FxU32 j;

      linearAddressMapList[i].addrList[addrNum].mapped = (value != 0x00UL);
      linearAddressMapList[i].addrList[addrNum].addr = value;

      for(j = 0; j < MAX_PCI_BASEADDRESSES; j++) {
        if (linearAddressMapList[i].addrList[j].mapped) break;
      }
      if (j == MAX_PCI_BASEADDRESSES) linearAddressMapList[i].device_bus_func_number = 0x00UL;

      break;
    }
  }
  if (i != MAX_PCI_DEVICES) return;

  for(i = 0; i < MAX_PCI_DEVICES; i++) {
    if (linearAddressMapList[i].device_bus_func_number == 0) {
      linearAddressMapList[i].device_bus_func_number = device_bus_func_number;

      linearAddressMapList[i].addrList[addrNum].mapped = (value != 0x00UL);
      linearAddressMapList[i].addrList[addrNum].addr = value;

      break;
    }
  }
}

/* PUBLIC DATA  */

const PciRegister PCI_VENDOR_ID       = { 0x0,  2, READ_ONLY };
const PciRegister PCI_DEVICE_ID       = { 0x2,  2, READ_ONLY };
const PciRegister PCI_COMMAND         = { 0x4,  2, READ_WRITE };
const PciRegister PCI_STATUS          = { 0x6,  2, READ_WRITE };
const PciRegister PCI_REVISION_ID     = { 0x8,  1, READ_ONLY };
const PciRegister PCI_CLASS_CODE      = { 0x9,  3, READ_ONLY };
const PciRegister PCI_CACHE_LINE_SIZE = { 0xC,  1, READ_WRITE };
const PciRegister PCI_LATENCY_TIMER   = { 0xD,  1, READ_WRITE };
const PciRegister PCI_HEADER_TYPE     = { 0xE,  1, READ_ONLY };
const PciRegister PCI_BIST            = { 0xF,  1, READ_WRITE };
const PciRegister PCI_BASE_ADDRESS_0  = { 0x10, 4, READ_WRITE };
const PciRegister PCI_BASE_ADDRESS_1  = { 0x14, 4, READ_WRITE };
const PciRegister PCI_IO_BASE_ADDRESS = { 0x18, 4, READ_WRITE };
const PciRegister PCI_SUBVENDOR_ID    = { 0x2C, 4, READ_ONLY };
const PciRegister PCI_SUBSYSTEM_ID    = { 0x2E, 4, READ_ONLY };
const PciRegister PCI_ROM_BASE_ADDRESS= { 0x30, 4, READ_WRITE };
const PciRegister PCI_CAP_PTR         = { 0x34, 4, READ_WRITE };
const PciRegister PCI_INTERRUPT_LINE  = { 0x3C, 1, READ_WRITE };
const PciRegister PCI_INTERRUPT_PIN   = { 0x3D, 1, READ_ONLY };
const PciRegister PCI_MIN_GNT         = { 0x3E, 1, READ_ONLY };
const PciRegister PCI_MAX_LAT         = { 0x3F, 1, READ_ONLY };
const PciRegister PCI_FAB_ID          = { 0x40, 1, READ_ONLY };
const PciRegister PCI_CONFIG_STATUS   = { 0x4C, 4, READ_WRITE };
const PciRegister PCI_CONFIG_SCRATCH  = { 0x50, 4, READ_WRITE };
const PciRegister PCI_AGP_CAP_ID      = { 0x54, 4, READ_ONLY };
const PciRegister PCI_AGP_STATUS      = { 0x58, 4, READ_ONLY };
const PciRegister PCI_AGP_CMD         = { 0x5C, 4, READ_WRITE };
const PciRegister PCI_ACPI_CAP_ID     = { 0x60, 4, READ_ONLY };
const PciRegister PCI_CNTRL_STATUS    = { 0x64, 4, READ_WRITE };

/* sst1 definitions left in for compatability */
const PciRegister PCI_SST1_INIT_ENABLE = { 0x40, 4, READ_WRITE }; 
const PciRegister PCI_SST1_BUS_SNOOP_0 = { 0x44, 4, READ_WRITE }; 
const PciRegister PCI_SST1_BUS_SNOOP_1 = { 0x48, 4, READ_WRITE }; 
const PciRegister PCI_SST1_CFG_STATUS  = { 0x4C, 4, READ_WRITE };

/* PUBLIC FUNCTIONS */

FX_ENTRY const char * FX_CALL
pciGetErrorString( void )
{
  static char vxdErrString[120];
  if (pciErrorCode == PCI_ERR_WRONGVXD) {
    sprintf(vxdErrString, "Expected VXD version V%d.%d, got V%u.%u\n",
            FX_MAJOR_VER, FX_MINOR_VER,
            (unsigned) BYTE1(pciVxdVer), (unsigned) BYTE0(pciVxdVer));
    return vxdErrString;
  }
  return pciError[pciErrorCode].string;
} /* pciGetErrorString */



FX_ENTRY FxU32 FX_CALL
pciGetErrorCode( void )
{
  return pciError[pciErrorCode].code;
} /* pciGetErrorCode */


FX_EXPORT FxBool FX_CSTYLE
pciOpenEx( PciHwcCallbacks *cb )
{
  pciHwcCallbacks = *cb;
  return pciOpen();
}

// internal routine for scanning the PCI bus the first time during a pciOpen
static void
sampleVendorID(int deviceNumber, int configMode)
{
  FxU32 regVal;

  // set the global mode: this routine is used to determine which mode works
  configMechanism = configMode;
  regVal = _pciFetchRegister( PCI_VENDOR_ID.regAddress,
                              PCI_VENDOR_ID.sizeInBytes, 
                              deviceNumber, configMode );
  regVal &= 0xFFFF;

  if ( regVal != 0xFFFF ) {
    busDetected = FXTRUE;
    vendorIDs[deviceNumber] = regVal;
  } else {
    vendorIDs[deviceNumber] = 0;
  }
}

#ifdef __linux__
FxBool
pciOpenLinux(void)
{
  int numDevices, deviceNumber;

  numDevices=getNumDevicesLinux();
  for (deviceNumber=0; deviceNumber < MAX_PCI_DEVICES; deviceNumber++) {
    if (deviceNumber<numDevices) {
      busDetected=FXTRUE;
      configMechanism=1;
      deviceExists[deviceNumber] = FXTRUE;
      vendorIDs[deviceNumber] = 0x121a;
    }
    else deviceExists[deviceNumber] = FXFALSE;
  }
  if (numDevices) {
    pciLibraryInitialized=FXTRUE;
  } else {
    pciLibraryInitialized=FXFALSE;
  }      
  return pciLibraryInitialized;
}
#endif /* defined(__linux__) */

FX_EXPORT FxBool FX_CSTYLE
pciOpen( void )
{
  int deviceNumber;
  
  if ( pciLibraryInitialized ) return FXTRUE;
  
  baseAddresses[0] = PCI_BASE_ADDRESS_0;
  baseAddresses[1] = PCI_BASE_ADDRESS_1;
  baseAddresses[2] = PCI_IO_BASE_ADDRESS;
  baseAddresses[3] = PCI_ROM_BASE_ADDRESS;
  
  busDetected = FXFALSE;
  if (pciHwcCallbacks.doHW) {
    if (!pciPlatformInit() ||
        (gCurPlatformIO == NULL) ||
        !pciInitializeDDio()) return FXFALSE;
  }
  /*
  **      Scan All PCI device numbers
  */ 

#ifdef __linux__
  if (hasDev3DfxLinux()) return pciOpenLinux();
#endif /* defined(__linux__) */

  
  for ( deviceNumber = 0; deviceNumber < MAX_PCI_DEVICES; deviceNumber++ ) {
    FxU32 regVal;
    FxU32 slot;
    FxU32 bus;
        
    sampleVendorID(deviceNumber,1);

    bus =   (deviceNumber >> 5) & 0xFF;
    slot = (deviceNumber & 0x1f);

    pioOutLong( CONFIG_ADDRESS_PORT, 
                _pciCreateConfigAddress( bus, slot, 0x0, 0x0 ) );
    regVal = pioInLong( CONFIG_DATA_PORT );

    if ( ( regVal & 0xFFFF ) != 0xFFFF ) {
      busDetected = FXTRUE;
      configMechanism = 1;
      deviceExists[deviceNumber] = FXTRUE;
      vendorIDs[deviceNumber] = regVal & 0xffff;
    } else {
      deviceExists[deviceNumber] = FXFALSE;
    }

  }

#if 0
  if ( !busDetected )  { 
    /* Try Configuration Mechanism 2 (only 16 devices) */
    /* Since Configuration Mech#2 is obsolete this does not
       support multiple busses */

    assert(0); //This stuff looks scary and probably shouldn't be used

    for ( deviceNumber = 0; deviceNumber < 16; deviceNumber++ ) {
      sampleVendorID(deviceNumber,2);
    }
  }
#endif

  if ( busDetected ) {
    pciLibraryInitialized = FXTRUE;
  } else {
    pciErrorCode = PCI_ERR_NO_BUS;
    return FXFALSE;
  }    
  
  return FXTRUE;
} /* pciOpen */

FX_EXPORT FxBool FX_CSTYLE
pciClose( void )
{
  if ( !pciLibraryInitialized ) {
    pciErrorCode = PCI_ERR_NOTOPEN2;
    return FXFALSE;
  }
  pciLibraryInitialized = FXFALSE;

  return pciCloseDDio();
}


FX_EXPORT FxBool FX_CSTYLE
pciDeviceExists( FxU32 device_number ) {
  if ( !pciLibraryInitialized ) {
    pciErrorCode = PCI_ERR_NOTOPEN;
    return FXFALSE;
  }
  if ( device_number >= MAX_PCI_DEVICES ) return FXFALSE;
  return vendorIDs[device_number];
} /* pciDeviceExists */



FX_EXPORT FxBool FX_CSTYLE
pciGetConfigData( PciRegister reg, FxU32 device_bus_func_number, FxU32 *data ) 
{
  int device_number = (device_bus_func_number) & 0xFFF;
  
  if ( !pciLibraryInitialized ) {
    pciErrorCode = PCI_ERR_NOTOPEN3;
    return FXFALSE;
  }
  if ( device_number >= MAX_PCI_DEVICES ) {
    pciErrorCode = PCI_ERR_OUTOFRANGE;
    return FXFALSE;
  }
  
  if ( !vendorIDs[device_number] ) {
    pciErrorCode = PCI_ERR_NODEV;
    return FXFALSE;
  }
  
  if ( reg.rwFlag == WRITE_ONLY ) {
    pciErrorCode = PCI_ERR_WRITEONLY;
    return FXFALSE;
  }

#ifdef __linux__  
  if (hasDev3DfxLinux()) {
    *data = pciFetchRegisterLinux(reg.regAddress, reg.sizeInBytes,
				  device_bus_func_number);
    return FXTRUE;
  }
#endif
  *data = _pciFetchRegister( reg.regAddress, reg.sizeInBytes,
                             device_bus_func_number, configMechanism );
  
  return FXTRUE;
} /* pciGetConfigData */



FX_EXPORT FxBool FX_CSTYLE
pciSetConfigData( PciRegister reg, FxU32 device_bus_func_number, FxU32 *data ) 
{
  int device_number = (device_bus_func_number) & 0xFFF;
  
  if ( !pciLibraryInitialized ) {
    pciErrorCode = PCI_ERR_NOTOPEN3;
    return FXFALSE;
  }
  if ( device_number >= MAX_PCI_DEVICES ) {
    pciErrorCode = PCI_ERR_OUTOFRANGE;
    return FXFALSE;
  }
  
  if ( !vendorIDs[device_number] ) {
    pciErrorCode = PCI_ERR_NODEV2;
    return FXFALSE;
  }       
  
  if ( reg.rwFlag == READ_ONLY ) {
    pciErrorCode = PCI_ERR_READONLY;
    return FXFALSE;
  }

  /*
   * NOTE: This should really be this way, at least for now.
   *
   * Changing this to return the value returned by 
   * pciUpdateRegisterLinux breaks sst1.
   */
#ifdef __linux__
  if (hasDev3DfxLinux()) {
    pciUpdateRegisterLinux( reg.regAddress, *data, reg.sizeInBytes,
			    device_bus_func_number);
    
    return FXTRUE;
  }
#endif
  _pciUpdateRegister( reg.regAddress, *data, reg.sizeInBytes,
                      device_bus_func_number, configMechanism );
  
  return FXTRUE;
} /* pciSetConfigData */


FX_EXPORT FxBool FX_CSTYLE
pciFindCardMulti(FxU32 vendorID, FxU32 deviceID, 
                 FxU32 *devNum, FxU32 cardNum) 
{
  FxU32 deviceNumber;
  
  /*      1) open the PCI device and scan it for devices */
  if (!pciOpen()) return FXFALSE;
  
  /*      2) scan the existing devices for a match */
  for ( deviceNumber = 0; deviceNumber < MAX_PCI_DEVICES; deviceNumber++ ) {
    if (pciDeviceExists(deviceNumber)) {
      FxU32 
        vID, dID;
      FxBool
        matchP = FXFALSE;

      pciGetConfigData( PCI_VENDOR_ID, deviceNumber, &vID );
      pciGetConfigData( PCI_DEVICE_ID, deviceNumber, &dID );

      /* Quick optimization, if there is no device or vendor ID... */
      if ((!dID) || (!vID))
	  continue;

      if (deviceID == 0xFFFF)   /* if special value */
        dID = deviceID;         /* then force a match */

      matchP = ((vID == vendorID) && (dID == deviceID));
      if (matchP) {
        if (cardNum == 0) {
          *devNum = deviceNumber;
          return FXTRUE;
        }
      }

      /* single board SLI hack! - jeske */
      if ((vID == _3DFX_PCI_ID) && (dID == 0x0002)) {
        pciGetConfigData( PCI_VENDOR_ID, deviceNumber | (1 << 13), &vID);
        pciGetConfigData( PCI_DEVICE_ID, deviceNumber | (1 << 13), &dID);
        
        if ((vID == vendorID) && (dID == deviceID)) {
          matchP = FXTRUE;
          if (cardNum == 0) {
            *devNum = deviceNumber | (1 << 13); /* stuff in function 1 */
            return FXTRUE;
          }
        }
      }
      /* end of single board SLI hack! - jeske */

      if (matchP) cardNum--;
    }
  }
  return FXFALSE;         /* didn't find the card, return false */
} /* pciFindCardMulti */



FX_EXPORT FxBool FX_CSTYLE
pciFindCard(FxU32 vendorID, FxU32 deviceID, FxU32 *devNum) {
  return pciFindCardMulti(vendorID, deviceID, devNum, 0);
} /* pciFindCard */



/*----------------------------------------------------------------------
  find and map a PCI card into virtual memory using the following 4
  steps: 
      1) open the PCI device and scan it for devices 
      2) scan the existing devices for a vendorId, deviceId, cardNum match
      3) find the current physcial address of the card
      4) map the physical memory to virtual memory
      ----------------------------------------------------------------------*/
FX_EXPORT FxU32 * FX_CSTYLE
pciMapCardMulti(FxU32 vendorID, FxU32 deviceID, 
                FxI32 length, 
                FxU32 *devNum,
                FxU32 cardNum, FxU32 addressNum)
{
  FxU32 physAddress;
  unsigned long virtAddress;
  
  /* 1) open the PCI device and scan it for devices
   * 2) scan the existing devices for a match
   */
  if (!pciFindCardMulti(vendorID, deviceID, devNum, cardNum)) return NULL;
  if (addressNum > MAX_PCI_BASEADDRESSES) return NULL;

  /* 3) find the current physcial address of the card */
  pciGetConfigData( baseAddresses[addressNum], *devNum, &physAddress );
  if (length <= 0) return (FxU32*)(long)length;

  /* Mask the memory type information bits off.
   *   [0]: Memory type indicator (0 memory/1 i/o)
   *  [12]: Type
   *    00: 32 bits wide and mapable anywhere
   *    01: 32 bits wide, mapped < 1meg
   *    10: 64 bits wide and mappable anywhere
   *    11: reserved
   *   [3]: Prefetcable
   */
  physAddress &= ~0xF;

  /* 4) have we mapped this device before? */
  virtAddress = find_mapped_address(*devNum, addressNum);
  if (virtAddress == 0x00UL) {
    /* 5) map the physical memory to virtual memory 
     *
     * NB: Some systems (notably nt) require a bus # in addition to the
     * physical address in order to map a device. pciMapPhysicalToLinear
     * has an implicit bus0 which works most of the time, but fails
     * across pci bridges and to agp devices. Anyway, recall that the
     * deviceNumber is a tuple w/ the following internal structure:
     *
     *   device_number[0:4]   = slot
     *   device_number[5:12]  = bus
     *   device_number[13:15] = function 
     */
    if (!pciMapPhysicalDeviceToLinear(&virtAddress, 
                                      ((*devNum >> 5UL) & 0xFFUL), physAddress,                                     
                                      (FxU32*) &length)) {
      
      virtAddress = 0x00UL;
    }
    set_mapped_address(*devNum, addressNum, virtAddress);
  }

  return (FxU32*)virtAddress;
} /* pciMapCardMulti */


FX_EXPORT FxU32 * FX_CSTYLE
pciMapCard(FxU32 vendorID, FxU32 deviceID,
           FxI32 length, FxU32 *devNum, FxU32 addressNum)
{
  return pciMapCardMulti(vendorID, deviceID, length, devNum, 0, addressNum);
} /* pciMapCard */

FX_EXPORT FxBool FX_CSTYLE
pciMapPhysicalToLinear( unsigned long *linear_addr, FxU32 physical_addr,
                        FxU32 *length ) 
{ 
  return pciMapPhysicalDeviceToLinear(linear_addr, 
                                      0x00UL, physical_addr,
                                      length);
} /* pciMapPhysicalToLinear */

FX_ENTRY FxBool FX_CALL 
pciMapPhysicalDeviceToLinear(unsigned long *linear_addr, 
                             FxU32 busNumber, FxU32 physical_addr,
                             FxU32 *length)
{
  return pciMapLinearDD(busNumber, physical_addr,
                        linear_addr, length);
}


FX_EXPORT void FX_CSTYLE
pciUnmapPhysical( unsigned long linear_addr, FxU32 length ) 
{
  int i,j;
  
  for (i = 0; i < MAX_PCI_DEVICES; i++) {
    for (j = 0; j < MAX_PCI_BASEADDRESSES; j++) {
      if(linearAddressMapList[i].addrList[j].addr == linear_addr) { 
        linearAddressMapList[i].addrList[j].addr = 0x00UL;
        linearAddressMapList[i].addrList[j].mapped = FXFALSE;

        break;
      }
    }
  }

  if (i != MAX_PCI_DEVICES) pciUnmapLinearDD(linear_addr, length);
}

FX_EXPORT FxBool FX_CSTYLE
pciSetPassThroughBase(FxU32* baseAddr, FxU32 baseAddrLen)
{
  FxBool retVal;

  if (pciLibraryInitialized) {
    retVal = pciSetPassThroughBaseDD(baseAddr, baseAddrLen);
  } else {
    pciErrorCode = PCI_ERR_NOTOPEN;
    retVal = FXFALSE;
  }

  return retVal;
}

FX_EXPORT FxBool FX_CSTYLE
pciOutputDebugString(const char* msg)
{
  return pciOutputDebugStringDD(msg);
}

FX_EXPORT FxBool FX_CSTYLE
pciLinearRangeSetPermission(const unsigned long addrBase, const FxU32 addrLen, const FxBool writeableP)
{
  return pciLinearRangeSetPermissionDD(addrBase, addrLen, writeableP);
}
