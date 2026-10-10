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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define FX_DLL_DEFINITION
#include <3dfx.h>
#include <fxdll.h>
#include "oeminit.h"

const OemPciRegister OEM_PCI_VENDOR_ID       = { 0x0,  2, OEM_READ_ONLY };
const OemPciRegister OEM_PCI_DEVICE_ID       = { 0x2,  2, OEM_READ_ONLY };
const OemPciRegister OEM_PCI_COMMAND         = { 0x4,  2, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_STATUS          = { 0x6,  2, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_REVISION_ID     = { 0x8,  1, OEM_READ_ONLY };
const OemPciRegister OEM_PCI_CLASS_CODE      = { 0x9,  3, OEM_READ_ONLY };
const OemPciRegister OEM_PCI_CACHE_LINE_SIZE = { 0xC,  1, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_LATENCY_TIMER   = { 0xD,  1, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_HEADER_TYPE     = { 0xE,  1, OEM_READ_ONLY };
const OemPciRegister OEM_PCI_BIST            = { 0xF,  1, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_BASE_ADDRESS_0  = { 0x10, 4, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_BASE_ADDRESS_1  = { 0x14, 4, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_BASE_ADDRESS_2  = { 0x18, 4, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_BASE_ADDRESS_3  = { 0x1C, 4, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_BASE_ADDRESS_4  = { 0x20, 4, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_BASE_ADDRESS_5  = { 0x24, 4, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_BASE_ADDRESS_6  = { 0x28, 4, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_BASE_ADDRESS_7  = { 0x2c, 4, OEM_READ_WRITE };
const OemPciRegister OEM_PCI_BASE_ADDRESS_8  = { 0x30, 4, OEM_READ_WRITE };

static FILE*  outFile    = NULL;

static const char*
myGetenv(const char* envKey)
{
  FxBool callRealGetenvP = FXTRUE;

#if __WIN32__
  /* NB: If were being called from cya code in
   * DllMain(DLL_PROCESS_DETACH) because the current app has called
   * exit() or dropped off of the end of main the per dll environ
   * string table has been freed by the c runtime but has not been set
   * to NULL. Bad things happen if this memory has been unmapped by
   * the system or if the string cannot be found.  
   */
  {
    HANDLE curProcessHandle = GetCurrentProcess();
    DWORD exitCode = STILL_ACTIVE;

    callRealGetenvP = ((curProcessHandle != NULL) &&
                       GetExitCodeProcess(curProcessHandle, &exitCode) &&
                       (exitCode == STILL_ACTIVE));
  }
#endif /* __WIN32__ */

  return (callRealGetenvP
          ? getenv(envKey)
          : NULL);
}

static void 
MyDebugPrintf(FILE* outputFile, const char* fmtString, ...)
{
  static FxBool debugDumpP = FXFALSE;
  static FxBool calledP    = FXFALSE;

  if (!calledP) {
    const char* envStr = myGetenv("GDBG_LEVEL");

    debugDumpP = ((envStr != NULL) &&
                  (atoi(envStr) >= 80));

    calledP = FXTRUE;
  }

  if (debugDumpP) {
    va_list args;

#if !DIRECTX
    va_start(args, fmtString);
    if (outputFile != NULL) vfprintf(outputFile, fmtString, args);
    va_end(args);
#endif /* !DIRECTX */

    va_start(args, fmtString);
#if __WIN32__
    {
      char msgBuf[256];

      vsprintf(msgBuf, fmtString, args);
      OutputDebugString(msgBuf);
    }
#else
    vfprintf(stderr, fmtString, args);
#endif
    va_end(args);
  }
}

static struct {
  const char* resName;
  GrScreenResolution_t resId;
} resTable[] = {
  { "320x200",   GR_RESOLUTION_320x200   },  /* 0x0 */
  { "320x240",   GR_RESOLUTION_320x240   },  /* 0x1 */
  { "400x256",   GR_RESOLUTION_400x256   },  /* 0x2 */
  { "512x384",   GR_RESOLUTION_512x384   },  /* 0x3 */
  { "640x200",   GR_RESOLUTION_640x200   },  /* 0x4 */
  { "640x350",   GR_RESOLUTION_640x350   },  /* 0x5 */
  { "640x400",   GR_RESOLUTION_640x400   },  /* 0x6 */
  { "640x480",   GR_RESOLUTION_640x480   },  /* 0x7 */
  { "800x600",   GR_RESOLUTION_800x600   },  /* 0x8 */
  { "960x720",   GR_RESOLUTION_960x720   },  /* 0x9 */
  { "856x480",   GR_RESOLUTION_856x480   },  /* 0xA */
  { "512x256",   GR_RESOLUTION_512x256   },  /* 0xB */
  { "1024x768",  GR_RESOLUTION_1024x768  },  /* 0xC */
  { "1280x1024", GR_RESOLUTION_1280x1024 },  /* 0xD */
  { "1600x1200", GR_RESOLUTION_1600x1200 },  /* 0xE */
  { "400x300",   GR_RESOLUTION_400x300   }   /* 0xF */
};

static const char*
GlideRes2String(GrScreenResolution_t res)
{
  int i;
  const char* retVal = NULL;

  for(i = 0; i < sizeof(resTable) / sizeof(resTable[0]); i++) {
    if (resTable[i].resId == res) {
      retVal = resTable[i].resName;
      break;
    }
  }

  return ((retVal == NULL)
          ? "Unknown"
          : retVal);
}

static const char*
GlideRefresh2String(GrScreenRefresh_t refresh)
{
  static const char* refreshStrings[] = {
    "60", "70", "72", "75", "80", "90", "100", "85", "120"
  };

  return ((refresh > sizeof(refreshStrings) / sizeof(refreshStrings[0]))
          ? "Unknown"
          : refreshStrings[refresh]);
}

static void 
DumpOemVideoInfo(const OemVideoTimingInfo* videoInfo)
{
  if ((outFile != NULL) && (videoInfo != NULL)) {
    MyDebugPrintf(outFile, "VideoTimingInfo:\n");
    MyDebugPrintf(outFile, "\tGlideResolution:  (0x%lX : %s)\n", 
            videoInfo->res, GlideRes2String(videoInfo->res));
    MyDebugPrintf(outFile, "\tGlideRefreshRate: (0x%lX : %s Mhz)\n", 
            videoInfo->refresh, GlideRefresh2String(videoInfo->refresh));
    MyDebugPrintf(outFile, "\tDepth:            %ld\n", videoInfo->depth);
    MyDebugPrintf(outFile, "\tHSyncOn:          0x%lX\n", videoInfo->hSyncOn);
    MyDebugPrintf(outFile, "\tHSyncOff:         0x%lX\n", videoInfo->hSyncOff);
    MyDebugPrintf(outFile, "\tVSyncOn:          0x%lX\n", videoInfo->vSyncOn);
    MyDebugPrintf(outFile, "\tVSyncOn:          0x%lX\n", videoInfo->vSyncOff);
    MyDebugPrintf(outFile, "\tHBackPorch:       0x%lX\n", videoInfo->hBackPorch);
    MyDebugPrintf(outFile, "\tVBackPorch:       0x%lX\n", videoInfo->vBackPorch);
    MyDebugPrintf(outFile, "\tXDimension:       0x%lX\n", videoInfo->xDimension);
    MyDebugPrintf(outFile, "\tYDimension:       0x%lX\n", videoInfo->yDimension);
    MyDebugPrintf(outFile, "\tClkFreq 16bpp:    %g\n", videoInfo->clkFreq16bpp);
    MyDebugPrintf(outFile, "\tClkFreq 24bpp:    %g\n", videoInfo->clkFreq24bpp);
  }
}

static void 
DumpOemInitInfo(const char* fnName, const OemInitInfo* oemInfo) 
{
  const char* envStr = myGetenv("GDBG_LEVEL");
  
  if ((envStr != NULL) && (atoi(envStr) >= 80)) {
    if (outFile == NULL) outFile = fopen("oemdll.txt", "w");
    if (outFile != NULL) {
      MyDebugPrintf(outFile, "%s: \n", fnName);
      if (oemInfo == NULL) goto __errExit;
      
      MyDebugPrintf(outFile, "\tVersion:      %d.%d\n", 
                    (oemInfo->version >> 16UL), (oemInfo->version & 0xFFFFUL));
      MyDebugPrintf(outFile, "\tVendorId:     0x%lX\n", oemInfo->vendorID);
      MyDebugPrintf(outFile, "\tSubVendorId:  0x%lX\n", oemInfo->subvendorID);
      MyDebugPrintf(outFile, "\tDeviceId:     0x%lX\n", oemInfo->deviceID);
      MyDebugPrintf(outFile, "\tBoardId:      0x%lX\n", oemInfo->boardID);
      MyDebugPrintf(outFile, "\tHW BaseAddr:  0x%lX\n", (FxU32)oemInfo->linearAddress);
      MyDebugPrintf(outFile, "\tSliSlaveAddr: 0x%lX\n", (FxU32)oemInfo->slaveAddress);
      
      DumpOemVideoInfo(&oemInfo->vid);
      
  __errExit:
      MyDebugPrintf(outFile, "\n");
    }
  }
}

/*-------------------------------------------------------------------
  Function: fxoemInitMapBoard
  Date: 04-Feb-98
  Implementor(s): atai
  Description:
        null function for oem dll 
  Arguments:
  oem - information for oem dll
  Return:
  -------------------------------------------------------------------*/
FX_EXPORT FxU32 FX_CSTYLE fxoemInitMapBoard(OemInitInfo *oem)
{
#define FN_NAME "fxoemInitMapBoard"

  DumpOemInitInfo(FN_NAME, oem);
  
  return 1;
#undef FN_NAME
} /* fxoemInitMapBoard */

/*-------------------------------------------------------------------
  Function: fxoemGet
  Date: 12-Mar-98
  Implementor(s): atai
  Description:
        null function for oem dll 
  Arguments:
  pname - selector to query oem dll functions
  plenth - length of the parameter in bytes
  param - data field
  Return:
  size of the parameter. O on error or not supported
  -------------------------------------------------------------------*/
FX_EXPORT FxI32 FX_CSTYLE fxoemGet(FxU32 pname, FxU32 plength, FxI32 *params)
{
#define FN_NAME "fxoemGet"

  switch(pname) {
  case FX_OEM_TVOUT:
    if (plength == 4) {
      *params = FXFALSE;
      return 4;
    }
    break;
  case FX_OEM_SLIMASTER:
    if (plength == 8) {
      *params = 0;
      *(params +1) = 0;
      return 8;
    }
    break;
  case FX_OEM_SLISLAVE:
    if (plength == 8) {
      *params = 0;
      *(params +1) = 0;
      return 8;
    }
    break;    
  }
  return 0;
#undef FN_NAME
} /* fxoemGet */

/*-------------------------------------------------------------------
  Function: fxoemInitVideoTiming
  Date: 23-Feb-98
  Implementor(s): atai
  Description:
        null function for oem dll 
  Arguments:
  vid - video timing data
  Return:
        1  if video timing is modified
        0  if video timing is unchanged
  -------------------------------------------------------------------*/
FX_EXPORT FxU32 FX_CSTYLE fxoemInitVideoTiming(OemVideoTimingInfo *vid)
{
#define FN_NAME "fxoemInitVideoTiming"

  DumpOemVideoInfo(vid);

  return 0;
#undef FN_NAME
} /* fxoemInitSetVideo */

/*-------------------------------------------------------------------
  Function: fxoemInitSetVideo
  Date: 04-Feb-98
  Implementor(s): atai
  Description:
        null function for oem dll 
  Arguments:
  oem - information for oem dll
  Return:
  -------------------------------------------------------------------*/
FX_EXPORT FxU32 FX_CSTYLE fxoemInitSetVideo(OemInitInfo *oem)
{
#define FN_NAME "fxoemInitSetVideo"

  DumpOemInitInfo(FN_NAME, oem);

  return 1;
#undef FN_NAME
} /* fxoemInitSetVideo */

/*-------------------------------------------------------------------
  Function: fxoemRestoreVideo
  Date: 04-Feb-98
  Implementor(s): atai
  Description:
        null function for oem dll 
  Arguments:
  oem - information for oem dll
  Return:
  -------------------------------------------------------------------*/
FX_EXPORT FxU32 FX_CSTYLE fxoemRestoreVideo(OemInitInfo *oem)
{
#define FN_NAME "fxoemRestoreVideo"

  DumpOemInitInfo(FN_NAME, oem);
  if (outFile != NULL) {
    fclose(outFile);
    outFile = NULL;
  }

  return 1;
#undef FN_NAME
} /* fxoemRestoreVideo */

/*-------------------------------------------------------------------
  Function: fxoemControl
  Date: 23-Feb-98
  Implementor(s): atai
  Description:
        null function for oem dll 
  Arguments:
  mode - determine whether the VGA display or Voodoo Graphics is visiable
  Return:
  -------------------------------------------------------------------*/
FX_EXPORT FxU32 FX_CSTYLE fxoemControl(FxU32 mode)
{
#define FN_NAME "fxoemControlMode"

  DumpOemInitInfo(FN_NAME, NULL);

  return 1;
#undef FN_NAME
} /* fxoemRestoreVideo */
