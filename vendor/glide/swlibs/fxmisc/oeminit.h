/*
** Copyright (c) 1995, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#ifndef __OEMINIT_H__
#define __OEMINIT_H__

#define OEMINIT_INVALID_BOARD_ID 0xdeadfeed
#define OEMINIT_VERSION 0x10001

#include "3dfx.h"
#define FX_DLL_DEFINITION
#include "fxdll.h"
#include "sst1vid.h"

#define FX_OEM_CONTROL_ACTIVATE   1
#define FX_OEM_CONTROL_DEACTIVATE 2

#define FX_OEM_TVOUT              1
#define FX_OEM_SLIMASTER          2
#define FX_OEM_SLISLAVE           3

#define FX_OEM_FBI_CONFIG_SHIFT   8

typedef enum {
    OEM_READ_ONLY,
    OEM_WRITE_ONLY,
    OEM_READ_WRITE
} OemPciIOFlag;

typedef struct {
    FxU32              regAddress;
    FxU32              sizeInBytes;
    OemPciIOFlag       rwFlag;
} OemPciRegister;

typedef FxU32 (FX_CALL* PciConfigProc)(OemPciRegister, FxU32, FxU32 *);

typedef struct {
  GrScreenResolution_t res;
  GrScreenRefresh_t refresh;
  FxU32 depth;
  FxU32 hSyncOn;
  FxU32 hSyncOff;
  FxU32 vSyncOn;
  FxU32 vSyncOff;
  FxU32 hBackPorch;
  FxU32 vBackPorch;
  FxU32 xDimension;
  FxU32 yDimension;
  float clkFreq16bpp;
  float clkFreq24bpp;
} OemVideoTimingInfo;

typedef struct {
  FxU32 version;        /* Version for this structure */
  FxU32 vendorID;       /* PCI Vendor ID field */
  FxU32 subvendorID;    /* PCI sub Vendor ID field */
  FxU32 deviceID;       /* PCI Device ID field */
  FxU32 boardID;        /* Board ID: defined for Voodoo2 only!!! */
  FxU32 *linearAddress; /* address for primary board */
  FxU32 *slaveAddress;  /* address for slave board */
  PciConfigProc fxoemPciWriteConfig;
  PciConfigProc fxoemPciReadConfig;
  OemVideoTimingInfo vid;
  FxU32 reserved[42];
} OemInitInfo;

FX_EXPORT FxU32 FX_CSTYLE fxoemInitMapBoard(OemInitInfo *oem);
FX_EXPORT FxI32 FX_CSTYLE fxoemGet(FxU32 pname, FxU32 plength, FxI32 *params);
FX_EXPORT FxU32 FX_CSTYLE fxoemInitVideoTiming(OemVideoTimingInfo *vid);
FX_EXPORT FxU32 FX_CSTYLE fxoemInitSetVideo(OemInitInfo *oem);
FX_EXPORT FxU32 FX_CSTYLE fxoemRestoreVideo(OemInitInfo *oem);
FX_EXPORT FxU32 FX_CSTYLE fxoemControl(FxU32 mode);

FX_EXPORT FxU32 FX_CSTYLE fxoemSwapControlP(OemInitInfo* oemInfo);
FX_EXPORT FxU32 FX_CSTYLE fxoemSwapControlWrite(OemInitInfo* oemInfo,
                                                FxU32* writeBuffer);

typedef FxU32 (FX_CALL* FxOemSwapControlP)(OemInitInfo* oemInfo);
typedef FxU32 (FX_CALL* FxOemSwapControlWrite)(OemInitInfo* oemInfo,
                                               FxU32* writeBuffer);

#endif /* __OEMINIT_H__ */
