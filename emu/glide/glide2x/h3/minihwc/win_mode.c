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

#include <stdio.h>
#include <3dfx.h>
#include <gdebug.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if (WINVER < 0x0500) && !defined(HMONITOR_DECLARED) /* <--- HACK */
DECLARE_HANDLE(HMONITOR);
#define HMONITOR_DECLARED
#endif
#include <ddraw.h>
#include <sst1vid.h>
#include "qmodes.h"
#ifdef HAVE_WIN9X_DDK
#define IS_32
#define Not_VxD
#include <minivdd.h>
#include <vmm.h>
#include <configmg.h>
#else
#define CM_REGISTRY_HARDWARE 0
#define CM_REGISTRY_SOFTWARE 1
#define CR_FAILURE 0x00000013
#define ___CONFIGMG_Get_DevNode_Key 0x0033003d
struct _CMIOCTLPACKET {
 DWORD dwStack;
 DWORD dwServiceNumber;
};
static DWORD WINAPI CMIOCTLHandler(struct _CMIOCTLPACKET *pkt)
{
  HANDLE hCONFIGMG;
  DWORD crReturnValue = CR_FAILURE;
  DWORD dwReturnSize = 0;

  hCONFIGMG = CreateFileA(
    "\\\\.\\CONFIGMG",
    GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_READ|FILE_SHARE_WRITE,
    NULL, OPEN_EXISTING, 0, NULL);

  if (hCONFIGMG == INVALID_HANDLE_VALUE) {
      return CR_FAILURE;
  }
  if (!DeviceIoControl(
        hCONFIGMG, pkt->dwServiceNumber,
        &(pkt->dwStack), sizeof(pkt->dwStack),
        &crReturnValue, sizeof(crReturnValue),
        &dwReturnSize, NULL)) {
    crReturnValue = CR_FAILURE;
  }
  CloseHandle(hCONFIGMG);
  if (dwReturnSize != sizeof(crReturnValue)) {
      crReturnValue = CR_FAILURE;
  }
  return crReturnValue;
}
static DWORD __cdecl CM_Get_DevNode_Key (DWORD devnode, char *subkey, void *buffer, ULONG bufferlen, ULONG flags)
{
    struct _CMIOCTLPACKET packet;
    DWORD dwStack;
    #if defined(_MSC_VER)
    _asm {mov dwStack, ebp};
    #elif defined(__GNUC__)
    dwStack = (DWORD) __builtin_frame_address(0);
    #else
    #error Add support for your compiler here.
    #endif
    dwStack += 8;
    packet.dwStack = dwStack;
    packet.dwServiceNumber = 0x80000000 + (___CONFIGMG_Get_DevNode_Key & 0xFFFF);
    return CMIOCTLHandler(&packet);
}
#endif

#ifndef IDirectDraw7_CreateSurface /* ddraw.h not from dx7 sdk */
typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKEXA)(GUID FAR *, LPSTR, LPSTR, LPVOID, HMONITOR);
typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEEXA)(LPDDENUMCALLBACKEXA, LPVOID, DWORD);
#ifndef DDENUM_ATTACHEDSECONDARYDEVICES
#define DDENUM_ATTACHEDSECONDARYDEVICES 0x00000001L
#endif
#endif

static HWND	hwndApp = 0;

/* Direct Draw stuff */
LPDIRECTDRAW            lpDD1 = NULL;
LPDIRECTDRAW2           lpDD  = NULL;

static int _set_exclusive_relaxed;
static int _set_vidmode_relaxed;

typedef struct _enumInfoStruct {
  GUID guid;
  HMONITOR hmon;
} EnumInfo;

static BOOL FAR PASCAL
ddEnumCbEx( GUID FAR *guid, LPSTR desc, LPSTR name, LPVOID ctx, HMONITOR hmon ) 
{
  EnumInfo* pEnumInfo = (EnumInfo*)ctx;
  BOOL      rv        = DDENUMRET_OK;
  
  if(pEnumInfo->hmon == hmon) {
    if ( guid ) CopyMemory(&pEnumInfo->guid, guid, sizeof(GUID));
    rv = DDENUMRET_CANCEL;
  }
  
  return rv;
}

typedef struct _emcStruct {
  FxU32 xRes, yRes, Refresh;
  FxBool modeOK;
} EMCData;

static HRESULT WINAPI
enumModesCallback(LPDDSURFACEDESC surfaceDesc, LPVOID lpContext)
{
  EMCData *emcData = (EMCData *) lpContext;

  GDBG_INFO(80, "enumModesCallbac:\n");
  GDBG_INFO(80, "\tsurfaceDesc->dwWidth == %d\n", surfaceDesc->dwWidth);
  GDBG_INFO(80, "\tsurfaceDesc->dwHeight == %d\n", surfaceDesc->dwHeight);
  GDBG_INFO(80, 
            "\tsurfaceDesc->dwRefreshRate == %d\n",surfaceDesc->dwRefreshRate);
  GDBG_INFO(80, "\tsurfaceDesc->ddpfPixelFormat.dwRGBBitCount == %d\n", 
            surfaceDesc->ddpfPixelFormat.dwRGBBitCount);
  if (
      (surfaceDesc->dwWidth == emcData->xRes) &&
      (surfaceDesc->dwHeight == emcData->yRes) &&
      (surfaceDesc->ddpfPixelFormat.dwRGBBitCount == 16) &&
      (surfaceDesc->dwRefreshRate == emcData->Refresh)) {
    emcData->modeOK = FXTRUE;
    return DDENUMRET_CANCEL;
  } else
    return DDENUMRET_OK;

} /* enumModesCallback */

static void
msgModeSetFailure(HRESULT hResult) 
{
  
  if (hResult& DDERR_GENERIC)
    GDBG_INFO(80, "Mode set failed due to DDERR_GENERIC\n");
  
  if (hResult & DDERR_INVALIDMODE)
    GDBG_INFO(80, "Mode set failed due to DDERR_INVALIDMODE\n");
  
  if (hResult & DDERR_INVALIDOBJECT)
    GDBG_INFO(80, "Mode set failed due to DDERR_INVALIDOBJECT\n");
  
  if (hResult & DDERR_INVALIDPARAMS)
    GDBG_INFO(80, "Mode set failed due to DDERR_INVALIDPARAMS\n");
  
  if (hResult & DDERR_LOCKEDSURFACES)
    GDBG_INFO(80, "Mode set failed due to DDERR_LOCKEDSURFACES\n");
  
  if (hResult & DDERR_NOEXCLUSIVEMODE)
    GDBG_INFO(80, "Mode set failed due to DDERR_NOEXCLUSIVEMODE\n");
  
  if (hResult & DDERR_SURFACEBUSY)
    GDBG_INFO(80, "Mode set failed due to DDERR_SURFACEBUSY\n");
  
  if (hResult & DDERR_UNSUPPORTED)
    GDBG_INFO(80, "Mode set failed due to DDERR_UNSUPPORTED\n");
  
  if (hResult & DDERR_UNSUPPORTEDMODE)
    GDBG_INFO(80, "Mode set failed due to DDERR_UNSUPPORTEDMODE\n");
  
  if (hResult & DDERR_WASSTILLDRAWING)
    GDBG_INFO(80, "Mode set failed due to DDERR_WASSTILLDRAWING\n");
  
} /* msgModeSetFailure */

static void
msgEnumDisplayModes(hResult)
{
  if (hResult & DDERR_INVALIDOBJECT)
    GDBG_INFO(80, "DDERR_INVALIDOBJECT\n");

  if (hResult & DDERR_INVALIDPARAMS)
    GDBG_INFO(80, "DDERR_INVALIDPARAMS\n");

} /* msgEnumDisplayModes */

#ifdef __WIN32__
static char *
getModesRegPath() 
{
  char *retVal = NULL;
  OSVERSIONINFO ovi;
  
  ovi.dwOSVersionInfoSize = sizeof ( ovi );
  GetVersionEx ( &ovi );
  if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
    GDBG_INFO(80, "OS == WNT\n");
    /* It is hardcoded on NT via Display Control code. see:
     * $/devel/swtools/bansheecp2 */
    retVal = "SYSTEM\\CurrentControlSet\\Services\\3Dfx\\Device0\\modes\\";
  } else {
    QDEVNODE QDevNode;
    QIN Qin;
    int status;

    GDBG_INFO(80, "OS == W9X\n");

    Qin.dwSubFunc = QUERYDEVNODE;
    {
      HDC curDC = GetDC(NULL);

      status = ExtEscape ( curDC, QUERYESCMODE, 
                           sizeof(Qin), (LPCSTR)&Qin, 
                           sizeof(QDevNode), (LPSTR)&QDevNode );
      ReleaseDC(NULL, curDC);
    }

    if ( status > 0 ) {
      static char regPath[255];

      CM_Get_DevNode_Key( QDevNode.dwDevNode, NULL, 
                          &regPath, sizeof(regPath), 
                          CM_REGISTRY_SOFTWARE );
      strcat(regPath, "\\modes\\");

      retVal = regPath;
    }
  }

  return retVal;
} /* getRegPath */
#endif

FxBool 
setVideoMode( HWND hwnd, int xRes, int yRes, int refresh, void *hmon ) 
{
  LPGUID ddGuid = NULL;
  HMODULE ddraw = NULL;
  DDSURFACEDESC   ddsd;
  HRESULT hResult;
  DEVMODE devMode;
  FxU32 bpp = 16;
  EMCData emcData; /* Enum Modes Callbac Data */
  
  GDBG_INFO( 80, "setVideoMode sees hwnd %x\n", hwnd);
  hwndApp = ( hwnd == NULL ) ? GetActiveWindow() : hwnd;
  
  if ( hwndApp == NULL ) {
    GDBG_INFO( 80, "Couldn't get a valid window handle\n" );
  }
  
  ddGuid = NULL;
  ddraw = GetModuleHandle( "ddraw.dll" );
  if ( ddraw ) {
    LPDIRECTDRAWENUMERATEEXA ddEnumEx;
    ddEnumEx = (void*)GetProcAddress( ddraw, "DirectDrawEnumerateExA" );
    if ( ddEnumEx ) {
      EnumInfo enumInfo;

      ZeroMemory(&enumInfo, sizeof(enumInfo));
      ZeroMemory(&enumInfo.guid, sizeof(GUID));
      enumInfo.hmon = (HMONITOR)hmon;
      ddEnumEx( ddEnumCbEx, &enumInfo, DDENUM_ATTACHEDSECONDARYDEVICES );
      ddGuid = &enumInfo.guid;
	  GDBG_INFO(80, "GUID %d\n", ddGuid);
    }
  }

  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

  /*
  **  Oh, this is lovely.  What we have here is a failure to
  **  communicate.  If the current mode is exactly the same as the
  **  mode in which Glide wishes to run, then the DirecDraw driver
  **  doesn't get called when we restore the video (or on an alt-tab)
  **  this would be fine, except Glide has its way with the video
  **  overlya registers.  So, if we detect this case, we ask DDraw for
  **  an 8-bit display, and fix up vidProcCfg in the minihwc.
  */
  if ((devMode.dmPelsWidth == (FxU32) xRes) &&
      (devMode.dmPelsHeight ==  (FxU32) yRes) &&
      (devMode.dmBitsPerPel == 16UL)) {
    bpp = 8;
  }

  //checkSpecialList(); /* [dBorca] disable for now */
  if (lpDD == NULL) {
    /* only create directdraw object once */
    if ( DirectDrawCreate( ddGuid, &lpDD1, NULL ) != DD_OK) {
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
    }
    else GDBG_INFO(80, "DDraw Obj created!\n");
    if ( IDirectDraw_QueryInterface( lpDD1, &IID_IDirectDraw2, 
                                    (LPVOID*)&lpDD ) != DD_OK ) {
      IDirectDraw_Release( lpDD1 );
      lpDD1 = NULL;
      lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;
    }
    else GDBG_INFO(80, "DDraw2 Obj created!\n");
  }
  
  /* Set Exclusive Mode, change resolution,  */
  GDBG_INFO(80, "Setting Full screen exclusive mode!\n");
  GDBG_INFO(80, "Calling IDD2_SetCoop: 0x%x, 0x%x, 0x%x\n", lpDD, hwndApp, 
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
  
  if (
      (hResult =
       IDirectDraw2_SetCooperativeLevel(lpDD, hwndApp, 
                                        DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN ))
      != DD_OK) {
    GDBG_INFO(80, "Couldn't set cooperative level:  " );
    if (hResult & DDERR_EXCLUSIVEMODEALREADYSET)
      GDBG_INFO_MORE(80, "DDERR_EXCLUSIVEMODEALREADYSET\n" );
    
    if (hResult & DDERR_HWNDALREADYSET) {
      GDBG_INFO_MORE(80, "DDERR_HWNDALREADYSET\n" );
      if (hResult == DDERR_HWNDALREADYSET)
        _set_exclusive_relaxed = 1;
    }
    if (hResult & DDERR_HWNDSUBCLASSED)
      GDBG_INFO_MORE(80, "DDERR_HWNDSUBCLASSED\n" );
    
    if (hResult & DDERR_INVALIDOBJECT)
      GDBG_INFO_MORE(80, "DDERR_INVALIDOBJECT\n" );
    
    if (hResult & DDERR_INVALIDPARAMS)
      GDBG_INFO_MORE(80, "DDERR_INVALIDPARAMS\n" );
    
    if (hResult & DDERR_OUTOFMEMORY)
      GDBG_INFO_MORE(80, "DDERR_OUTOFMEMORY\n" );
    
    if (!_set_exclusive_relaxed)
      return FXFALSE;
  }
  
  GDBG_INFO(80, "FSEM Set\n" );
  GDBG_INFO(80, "Enumerating Display Modes.\n");
  
  /* Figure out if we can support the requested display mode.  If not,
     try to use the same x & y res, but the default refresh rate.*/
  
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
  ddsd.dwWidth = xRes;
  ddsd.dwHeight = yRes;
  ddsd.dwRefreshRate = refresh;
  
  /* Set these for use by the callback */
  emcData.xRes = xRes;
  emcData.yRes = yRes;
  emcData.Refresh = refresh;
  emcData.modeOK = FXFALSE;
  
  hResult = IDirectDraw2_EnumDisplayModes(lpDD, 0, &ddsd, 
                                          (LPVOID) &emcData, enumModesCallback);
  
  if (hResult != DD_OK) {
    GDBG_INFO(80, "Warning:  EnumDisplayModes failed due to: \n");
    msgEnumDisplayModes(hResult);
  }

  GDBG_INFO(80, "Setting Display Mode!\n");
  
  if (emcData.modeOK) {
    GDBG_INFO(80, "Found mode %dx%d.  Attempting at %dHz\n", xRes,
              yRes, refresh);
    hResult = IDirectDraw2_SetDisplayMode( lpDD, xRes, yRes, bpp, refresh, 0);
    if (hResult != DD_OK) {
      GDBG_INFO(80, "Couldn't set display mode\n" );
      msgModeSetFailure(hResult);
      GDBG_INFO(80, "Retrying at default resolution\n");
      hResult = IDirectDraw2_SetDisplayMode( lpDD, xRes, yRes, 16, 0, 0 );

      if (hResult != DD_OK) {
        GDBG_INFO(80, "Setting video mode %dx%d@default refresh failed!\n",
                  xRes, yRes);
        msgModeSetFailure(hResult);

        if (!_set_vidmode_relaxed) {
          GDBG_INFO(80, "Returning FXFALSE\n");
          return FXFALSE;
        } else {
          GDBG_INFO(80, "Continuing operation due to relaxation condition\n");
        }
      }
    }
    GDBG_INFO(80, "Display Mode Set\n" );
  } else {

    GDBG_INFO(80, "Did not find mode %dx%d@any refresh\n", xRes, yRes);
    GDBG_INFO(80, "Setting video mode %dx%d@default refresh\n", xRes, yRes);

    /*
    ** This is really a hack here. We use the registry key entry to work
    ** around DDraw refresh rate problem.
    */
    {
      static char *regPath = NULL;
      char mpath[256], mpath_8[256], tmp[32];
      FxU32 reg_flag = 1;
      if (regPath == NULL) {
        regPath = getModesRegPath();
      }
      if ((regPath) && (bpp == 8)) {
        HKEY hKey, hKey8;
        FxU32 retVal;
        DWORD type, szData, type8, szData8;
        static char strval[255], strval8[255];

        /* get the current bpp path */
        szData = sizeof(strval);
        strcpy(mpath, regPath);
        sprintf(tmp, "%d\\%d,%d",devMode.dmBitsPerPel,xRes,yRes);
        strcat(mpath, tmp);
        /* get the 8 bpp path */
        szData8 = sizeof(strval8);
        strcpy(mpath_8, regPath);
        sprintf(tmp, "8\\%d,%d",xRes,yRes);
        strcat(mpath_8, tmp);
        /*
        ** Get the original refresh rate from registry key
        */
        if (RegOpenKey(HKEY_LOCAL_MACHINE, mpath, &hKey) == ERROR_SUCCESS)  {
          if (RegQueryValueEx(hKey, "RefreshRate", NULL, &type, strval,
                              &szData) == ERROR_SUCCESS)  {
            /*
            ** overwrite 8 bit registry with glide refresh rate and set
            ** the display mode. this is a workaround of the DDraw
            ** refresh rate problem
            */
            if (RegOpenKey(HKEY_LOCAL_MACHINE, mpath_8, &hKey8)
                == ERROR_SUCCESS) {
              if (RegQueryValueEx(hKey8, "RefreshRate", NULL, &type8,
                                  strval8, &szData8) == ERROR_SUCCESS) {
                retVal = 1;
              } else {
                retVal = 0;
              }
              RegSetValueEx(hKey8, "RefreshRate", 0, REG_SZ, strval,
                            strlen(strval));
              hResult = IDirectDraw2_SetDisplayMode(lpDD, xRes, yRes,
                                                    bpp, 0, 0);
              reg_flag = 0;
              /*
              ** reset the original refresh rate
              */
              if (retVal) {
                RegSetValueEx(hKey8, "RefreshRate", 0,
                              REG_SZ, strval8, strlen(strval8));
              }
              RegCloseKey(hKey8);
              /*
              ** delete the key if the refresh rate isn't there in the
              ** first place
              */
              if (!retVal) {
                RegDeleteValue(hKey8, "RefreshRate" );
                RegDeleteKey(hKey8, "RefreshRate" );
              }
              RegCloseKey(hKey);
            }
          } else {
            /*
            ** We did not find the "RefreshRate" key in the
            ** devMode.dmBitsPerPel node so let's see if it exists for
            ** this resolution under the 8 bpp node:
            */
            if (RegOpenKey(HKEY_LOCAL_MACHINE, mpath_8, &hKey8)
                == ERROR_SUCCESS) {
              if (RegQueryValueEx(hKey8, "RefreshRate", NULL, &type8, strval8,
                                  &szData8) == ERROR_SUCCESS) {
                retVal = 1;
              } else {
                retVal = 0;
              }
              RegSetValueEx(hKey8, "RefreshRate", 0, REG_SZ, strval,
                            strlen(strval));
              hResult = IDirectDraw2_SetDisplayMode(lpDD, xRes, yRes,
                                                    bpp, 0, 0);
              reg_flag = 0;
              /*
              ** reset the original refresh rate
              */
              if (retVal) {
                RegSetValueEx(hKey8, "RefreshRate", 0,
                              REG_SZ, strval8, strlen(strval8));
              }
              RegCloseKey(hKey8);
              /*
              ** delete the key if the refresh rate isn't there in the
              ** first place
              */
              if (!retVal) {
                RegDeleteValue(hKey8, "RefreshRate" );
                RegDeleteKey(hKey8, "RefreshRate" );
              }
              RegCloseKey(hKey);
            }
          }
        }
      }
      if (reg_flag)
        hResult = IDirectDraw2_SetDisplayMode( lpDD, xRes, yRes, bpp, 0, 0 );
    }

    if (hResult != DD_OK) {
      GDBG_INFO(80, "Failed!\n",
                xRes, yRes);
      msgModeSetFailure(hResult);
      if (!_set_vidmode_relaxed) {
        GDBG_INFO(80, "Returning FXFALSE\n");
        return FXFALSE;
      } else {
        GDBG_INFO(80, "Continuing operation due to relaxation condition\n");
      }
    }
  }
  
  return FXTRUE;
  
} /* setVideoMode */

void 
resetVideo( void ) 
{
#define FN_NAME "resetVideo"
  if ( lpDD ) {
    IDirectDraw2_RestoreDisplayMode( lpDD );
    GDBG_INFO(80, "%s:  Restored Display Mode!\n", FN_NAME);
    IDirectDraw2_SetCooperativeLevel( lpDD, hwndApp, DDSCL_NORMAL );
    GDBG_INFO(80, "%s:  Set cooperative level!\n", FN_NAME);
    IDirectDraw2_Release( lpDD );
    GDBG_INFO(80, "%s:  Released lpDD!\n", FN_NAME);

    if ( lpDD1 ) {
      IDirectDraw_Release( lpDD1 );
      GDBG_INFO(80, "%s:  Released lpDD1!\n", FN_NAME);
    }
  }
  lpDD = NULL;
  lpDD1 = NULL;

  return;
#undef FN_NAME
} /* resetVideo */

typedef struct WidthHeight_s {
  FxU32 width;
  FxU32 height;
} WidthHeight_t;

static  WidthHeight_t widthHeightByResolution[] = {
  {320, 200},                 /* GR_RESOLUTION_320x200 */
  {320, 240},                 /* GR_RESOLUTION_320x240 */
  {400, 256},                 /* GR_RESOLUTION_400x256 */
  {512, 384},                 /* GR_RESOLUTION_512x384 */
  {640, 200},                 /* GR_RESOLUTION_640x200 */
  {640, 350},                 /* GR_RESOLUTION_640x350 */
  {640, 400},                 /* GR_RESOLUTION_640x400 */
  {640, 480},                 /* GR_RESOLUTION_640x480 */
  {800, 600},                 /* GR_RESOLUTION_800x600 */
  {960, 720},                 /* GR_RESOLUTION_960x720 */
  {856, 480},                 /* GR_RESOLUTION_856x480 */
  {512, 256},                 /* GR_RESOLUTION_512x256 */
  {1024, 768},                /* GR_RESOLUTION_1024x768 */
  {1280, 1024},               /* GR_RESOLUTION_1280x1024 */
  {1600, 1200},               /* GR_RESOLUTION_1600x1200 */
  {400, 300},                 /* GR_RESOLUTION_400x300 */
  {1152, 864},                /* GR_RESOLUTION_1152x864 */
  {1280, 960},                /* GR_RESOLUTION_1280x960 */
  {1600, 1024},               /* GR_RESOLUTION_1600x1024 */
  {1792, 1344},               /* GR_RESOLUTION_1792x1344 */
  {1856, 1392},               /* GR_RESOLUTION_1856x1392 */
  {1920, 1440},               /* GR_RESOLUTION_1920x1440 */
  {2048, 1536},               /* GR_RESOLUTION_2048x1536 */
  {2048, 2048}                /* GR_RESOLUTION_2048x2048 */
};


static HRESULT WINAPI
checkResEMCallback(LPDDSURFACEDESC surfaceDesc, LPVOID lpContext)
{
#define FN_NAME "checkResEMCallback"
  FxBool *resList = (FxBool *) lpContext;
  int res;

  GDBG_INFO(80, FN_NAME ":\n");
  GDBG_INFO(80, FN_NAME "\tsurfaceDesc->dwWidth == %d\n",surfaceDesc->dwWidth);
  GDBG_INFO(80, FN_NAME 
            "\tsurfaceDesc->dwHeight == %d\n",surfaceDesc->dwHeight);
  GDBG_INFO(80, FN_NAME 
            "\tsurfaceDesc->dwRefreshRate == %d\n",surfaceDesc->dwRefreshRate);
  GDBG_INFO(80, FN_NAME 
            "\tsurfaceDesc->ddpfPixelFormat.dwRGBBitCount == %d\n", 
            surfaceDesc->ddpfPixelFormat.dwRGBBitCount);
  
  for (
       res = 0x0;
       res < (sizeof(widthHeightByResolution) / sizeof(WidthHeight_t));
       res++) {
    if (
        (surfaceDesc->dwWidth == widthHeightByResolution[res].width) &&
        (surfaceDesc->dwHeight == widthHeightByResolution[res].height)
        ) {
      resList[res] = FXTRUE;
      return DDENUMRET_OK;
    }
  }
  
  return DDENUMRET_OK;
#undef FN_NAME
} /* checkResEMCallback */


/*
**  checkResolution - check to see if a given resolution is available
**
**  This routine may only be called once and IT MUST BE CALLED FROM
**  hwcInit().
**  Any other useage will be very very bad, as it will mess with the
**  app's DirectDraw context.
*/
FxBool
checkResolutions(FxBool *supportedByResolution, void *hmon) 
{
#define FN_NAME "checkResolution"
  LPGUID          ddGuid = NULL;
  HMODULE         ddraw = NULL;
  HRESULT hResult;
  
  hwndApp = GetActiveWindow();
  
  if ( hwndApp == NULL ) {
    GDBG_INFO( 80, "Couldn't get a valid window handle\n" );
  }
  
  ddGuid = NULL;
  ddraw = GetModuleHandle( "ddraw.dll" );
  if ( ddraw ) {
    LPDIRECTDRAWENUMERATEEXA ddEnumEx;
    ddEnumEx = (void*)GetProcAddress( ddraw, "DirectDrawEnumerateExA" );
    if ( ddEnumEx ) {
      EnumInfo enumInfo;

      ZeroMemory(&enumInfo, sizeof(enumInfo));
      ZeroMemory(&enumInfo.guid, sizeof(GUID));
      enumInfo.hmon = (HMONITOR)hmon;
      ddEnumEx( ddEnumCbEx, &enumInfo, DDENUM_ATTACHEDSECONDARYDEVICES );
      ddGuid = &enumInfo.guid;
	  GDBG_INFO(80, "GUID %d\n", ddGuid);
    }
  }
  
  // checkSpecialList(); /* [dBorca] disable for now */
  if (lpDD == NULL) {
    /* only create directdraw object once */
    if ( DirectDrawCreate( ddGuid, &lpDD1, NULL ) != DD_OK) {
      GDBG_INFO(80, "DDraw Obj Create Failed!3\n");
    }
    else GDBG_INFO(80, "DDraw Obj created!\n");
    if ( IDirectDraw_QueryInterface( lpDD1, &IID_IDirectDraw2, 
                                    (LPVOID*)&lpDD ) != DD_OK ) {
      IDirectDraw_Release( lpDD1 );
      lpDD1 = NULL;
      lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;
    }
    else GDBG_INFO(80, "DDraw2 Obj created!\n");
  }
  
  hResult = IDirectDraw2_EnumDisplayModes(lpDD, 0, NULL,
                                (LPVOID) supportedByResolution,
                                          checkResEMCallback);

  resetVideo();
  return FXTRUE;
#undef FN_NAME
} /* checkResolutions */
