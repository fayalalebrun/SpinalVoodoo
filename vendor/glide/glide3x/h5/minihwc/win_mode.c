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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#if (WINVER < 0x0500) && !defined(HMONITOR_DECLARED) /* <--- HACK */
DECLARE_HANDLE(HMONITOR);
#define HMONITOR_DECLARED
#endif
#include <ddraw.h>
#include <sst1vid.h>
#include "qmodes.h"
#include "minihwc.h"
#include "setmode.h"

#undef GETENV
#define GETENV hwcGetenv

#ifndef IDirectDraw7_CreateSurface /* ddraw.h not from dx7 sdk */
typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKEXA)(GUID FAR *, LPSTR, LPSTR, LPVOID, HMONITOR);
typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEEXA)(LPDDENUMCALLBACKEXA, LPVOID, DWORD);
#ifndef DDENUM_ATTACHEDSECONDARYDEVICES
#define DDENUM_ATTACHEDSECONDARYDEVICES 0x00000001L
#endif
#endif

static int _set_exclusive_relaxed;
static int _set_vidmode_relaxed;
static FxBool _set_multirendering;

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
    if ( guid ) CopyMemory(&(pEnumInfo->guid), guid, sizeof(GUID));
    rv = DDENUMRET_CANCEL;
    GDBG_INFO(80, "DeviceName: %s, DeviceString: %s\n", name, desc);
  }
  
  return rv;
}

typedef struct _emcStruct {
  FxU32 xRes, yRes, Refresh;
  FxU32 bpp;
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
    (surfaceDesc->ddpfPixelFormat.dwRGBBitCount == emcData->bpp) &&
    ((surfaceDesc->dwRefreshRate == emcData->Refresh) ||
     (surfaceDesc->dwRefreshRate == 0) ||
     (emcData->Refresh == 0))) {
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
msgEnumDisplayModes(HRESULT hResult)
{
  if (hResult & DDERR_INVALIDOBJECT)
    GDBG_INFO(80, "DDERR_INVALIDOBJECT\n");

  if (hResult & DDERR_INVALIDPARAMS)
    GDBG_INFO(80, "DDERR_INVALIDPARAMS\n");

} /* msgEnumDisplayModes */

FxBool 
setVideoMode( hwcBoardInfo *bInfo, int refresh )
{
  LPGUID ddGuid = NULL;
  HMODULE ddraw = NULL;
  DDSURFACEDESC   ddsd;
  EMCData emcData; /* Enum Modes Callbac Data */
  HRESULT hResult;
  DEVMODE devMode;
  FxU32 bpp = 16;
  //HRESULT ddRVal;
  //DWORD style;
  EnumInfo enumInfo;

#ifdef FX_GLIDE_NAPALM
  if (bInfo->h3pixelSize == 4) bpp = 32;
  GDBG_INFO( 80, "Glide mode: %dx%dx%dbpp@%dHz\n",
             bInfo->vidInfo.xRes,
             bInfo->vidInfo.yRes,
             bpp,
             refresh);
#endif
  
  GDBG_INFO( 80, "setVideoMode sees hwnd 0x%x\n", (HWND)bInfo->vidInfo.hWnd);

  if ((HWND)bInfo->vidInfo.hWnd == NULL)
    bInfo->vidInfo.hWnd = (FxU32) GetActiveWindow();

  ddGuid = NULL;
  ddraw = GetModuleHandle( "ddraw.dll" );
  if ( ddraw != NULL ) {
    LPDIRECTDRAWENUMERATEEXA ddEnumEx;
    ddEnumEx = (LPDIRECTDRAWENUMERATEEXA)GetProcAddress( ddraw, "DirectDrawEnumerateExA" );
    if ( ddEnumEx ) {
      ZeroMemory(&enumInfo, sizeof(enumInfo));
      ZeroMemory(&(enumInfo.guid), sizeof(GUID));
      enumInfo.hmon = (HMONITOR)bInfo->hMon;
      ddEnumEx( ddEnumCbEx, &enumInfo, DDENUM_ATTACHEDSECONDARYDEVICES );
      ddGuid = &(enumInfo.guid);
      GDBG_INFO(80, "GUID %d-%d-%d-%d-%d%d%d%d\n",
                ddGuid->Data1,
                ddGuid->Data2,
                ddGuid->Data3,
                ddGuid->Data4[0], ddGuid->Data4[1],
                ddGuid->Data4[2], ddGuid->Data4[3], ddGuid->Data4[4]);
    }
  }

   EnumDisplaySettings(bInfo->devName, ENUM_REGISTRY_SETTINGS, &devMode);

  /* KoolSmoky - Hack for win95. make a disp struct if we don't get anything
   * from EnumDisplaySettings.
   */
  if ((devMode.dmBitsPerPel < 8UL) || (devMode.dmBitsPerPel > 32UL)) {
    devMode.dmSize       = sizeof(DEVMODE);
    devMode.dmPelsWidth  = GetSystemMetrics(SM_CXSCREEN);
    devMode.dmPelsHeight = GetSystemMetrics(SM_CYSCREEN);
    devMode.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    {
      HDC hdc = GetDC(NULL); /* grab DC of desktop */
      devMode.dmBitsPerPel = (DWORD)GetDeviceCaps(hdc, BITSPIXEL) ;
      ReleaseDC(NULL, hdc);
    }
  }

  GDBG_INFO(80, "DeviceName: %s Display mode: %dx%dx%dbpp!\n",
            bInfo->devName, devMode.dmPelsWidth, devMode.dmPelsHeight, devMode.dmBitsPerPel);

  /*
  **  Oh, this is lovely.  What we have here is a failure to
  **  communicate.  If the current mode is exactly the same as the
  **  mode in which Glide wishes to run, then the DirectDraw driver
  **  doesn't get called when we restore the video (or on an alt-tab)
  **  this would be fine, except Glide has its way with the video
  **  overlay registers.  So, if we detect this case, we ask DDraw for
  **  an 8-bit display, and fix up vidProcCfg in the minihwc.
  */
  /* KoolSmoky - what would happen if we run from 8bpp? */
  if ((devMode.dmPelsWidth == (FxU32) bInfo->vidInfo.xRes) &&
      (devMode.dmPelsHeight ==  (FxU32) bInfo->vidInfo.yRes)) {
      GDBG_INFO(80, "DDraw communication hack: setting to 8bpp!\n");
      bpp = 8;
  }
  
  if (bInfo->lpDD == NULL) {
    /* only create directdraw object once */
    if ( DirectDrawCreate( ddGuid, &(bInfo->lpDD1), NULL ) != DD_OK) {
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
    }
    else GDBG_INFO(80, "DDraw Obj created!\n");
    if ( IDirectDraw_QueryInterface( bInfo->lpDD1, &IID_IDirectDraw2, 
                                     (LPVOID*)&(bInfo->lpDD) ) != DD_OK ) {
      IDirectDraw_Release( bInfo->lpDD1 );
      bInfo->lpDD1 = NULL;
      bInfo->lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;
    }
    else GDBG_INFO(80, "DDraw2 Obj created!\n");
  }

  /* Set Exclusive Mode, change resolution,  */
  GDBG_INFO(80, "Setting Full screen exclusive mode!\n");
  {
    ULONG dwFlags = /*DDSCL_ALLOWREBOOT |*/ DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
    
    if (_set_multirendering) {
      dwFlags |= DDSCL_CREATEDEVICEWINDOW | DDSCL_SETFOCUSWINDOW;
      GDBG_INFO(80, "Setting MultiRendering mode!\n");
    }
    
    GDBG_INFO(80, "Calling IDD2_SetCoop: 0x%x, 0x%x, 0x%x\n", bInfo->lpDD, bInfo->vidInfo.hWnd, dwFlags );
    
    hResult = IDirectDraw2_SetCooperativeLevel(bInfo->lpDD, (HWND)bInfo->vidInfo.hWnd, dwFlags);
    
    if(hResult != DD_OK)
    {
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
  }
  GDBG_INFO(80, "FSEM Set\n" );
  GDBG_INFO(80, "Enumerating Display Modes.\n");
  
  /* Figure out if we can support the requested display mode.  If not,
     try to use the same x & y res, but the default refresh rate.*/
  
  ddsd.dwSize = sizeof(DDSURFACEDESC);
  ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
  ddsd.dwWidth = bInfo->vidInfo.xRes;
  ddsd.dwHeight = bInfo->vidInfo.yRes;
  ddsd.dwRefreshRate = refresh;
  
  /* Set these for use by the callback */
  emcData.xRes = bInfo->vidInfo.xRes;
  emcData.yRes = bInfo->vidInfo.yRes;
  emcData.Refresh = refresh;
  emcData.modeOK = FXFALSE;
  emcData.bpp = bpp;
  
  hResult = IDirectDraw2_EnumDisplayModes(bInfo->lpDD, 0, &ddsd, 
                                          (LPVOID) &emcData, enumModesCallback);
  
  if (hResult != DD_OK) {
    GDBG_INFO(80, "Warning:  EnumDisplayModes failed due to: \n");
    msgEnumDisplayModes(hResult);
  }
  
  GDBG_INFO(80, "Setting Display Mode!\n");
  
  if (emcData.modeOK) {
    GDBG_INFO(80, "Found mode %dx%d.  Attempting at %dHz\n", bInfo->vidInfo.xRes,
              bInfo->vidInfo.yRes, refresh);
    hResult = IDirectDraw2_SetDisplayMode( bInfo->lpDD, bInfo->vidInfo.xRes, bInfo->vidInfo.yRes, bpp, refresh, 0);
    if (hResult != DD_OK) {
      GDBG_INFO(80, "Couldn't set display mode\n" );
      msgModeSetFailure(hResult);
      GDBG_INFO(80, "Retrying at default resolution\n");
      hResult = IDirectDraw2_SetDisplayMode( bInfo->lpDD, bInfo->vidInfo.xRes, bInfo->vidInfo.yRes, bpp, 0, 0 );
      
      if (hResult != DD_OK) {
        GDBG_INFO(80, "Setting video mode %dx%d@default refresh failed!\n",
                  bInfo->vidInfo.xRes, bInfo->vidInfo.yRes);
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
    
    GDBG_INFO(80, "Did not find mode %dx%d@any refresh\n", bInfo->vidInfo.xRes, bInfo->vidInfo.yRes);
    GDBG_INFO(80, "Setting video mode %dx%d@default refresh\n", bInfo->vidInfo.xRes, bInfo->vidInfo.yRes);

    hResult = IDirectDraw2_SetDisplayMode( bInfo->lpDD, bInfo->vidInfo.xRes, bInfo->vidInfo.yRes, bpp, 0, 0 );
    
    if (hResult != DD_OK) {
      GDBG_INFO(80, "Failed!\n",
                bInfo->vidInfo.xRes, bInfo->vidInfo.yRes);
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
resetVideo( hwcBoardInfo *bInfo ) 
{
#define FN_NAME "resetVideo"
  
  GDBG_INFO(80, "%s:  called!\n", FN_NAME);
  
  if ( bInfo->lpDD != NULL ) {
    GDBG_INFO(80, "%s:  has lpDD!\n", FN_NAME);

    IDirectDraw2_RestoreDisplayMode( bInfo->lpDD );
    GDBG_INFO(80, "%s:  Restored Display Mode!\n", FN_NAME);
    
    IDirectDraw2_SetCooperativeLevel( bInfo->lpDD, (HWND)bInfo->vidInfo.hWnd, DDSCL_NORMAL );
    GDBG_INFO(80, "%s:  Set cooperative level!\n", FN_NAME);
    
    IDirectDraw2_Release( bInfo->lpDD );
    GDBG_INFO(80, "%s:  Released lpDD!\n", FN_NAME);

    if ( bInfo->lpDD1 != NULL ) {
      GDBG_INFO(80, "%s:  has lpDD1!\n", FN_NAME);
      IDirectDraw_Release( bInfo->lpDD1 );
      GDBG_INFO(80, "%s:  Released lpDD1!\n", FN_NAME);
    }
  }
  
  bInfo->lpDD  = NULL;
  bInfo->lpDD1 = NULL;

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

static FxU32 refresh[] = {
  60,  //"GR_REFRESH_60Hz",
  70,  //"GR_REFRESH_70Hz",
  72,  //"GR_REFRESH_72Hz",
  75,  //"GR_REFRESH_75Hz",
  80,  //"GR_REFRESH_80Hz",
  90,  //"GR_REFRESH_90Hz",
  100, //"GR_REFRESH_100Hz",
  85,  //"GR_REFRESH_85Hz",
  120  //"GR_REFRESH_120Hz"
};

static FxU32 resStride = 0;

static HRESULT WINAPI
checkResEMCallback(LPDDSURFACEDESC surfaceDesc, LPVOID lpContext)
{
#define FN_NAME "checkResEMCallback"
  FxBool *resList = (FxBool *) lpContext;
  int res, ref;
  
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
      for (ref = 0; ref < (sizeof(refresh) / sizeof(FxU32)); ref++) {
        if (surfaceDesc->dwRefreshRate == 0 || surfaceDesc->dwRefreshRate == refresh[ref])
          resList[res*resStride+ref] = FXTRUE;
      }
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
checkResolutions(FxBool *supportedByResolution,
                 FxU32 stride,
                 hwcBoardInfo *bInfo) 
{
#define FN_NAME "checkResolution"
  LPGUID          ddGuid = NULL;
  HMODULE         ddraw = NULL;
  HRESULT hResult;
  EnumInfo enumInfo;
  
  resStride = stride;
  
  ddGuid = NULL;
  ddraw = GetModuleHandle( "ddraw.dll" );
  if ( ddraw != NULL ) {
    LPDIRECTDRAWENUMERATEEXA ddEnumEx;
    ddEnumEx = (LPDIRECTDRAWENUMERATEEXA)GetProcAddress( ddraw, "DirectDrawEnumerateExA" );
    if ( ddEnumEx ) {
      ZeroMemory(&enumInfo, sizeof(enumInfo));
      ZeroMemory(&(enumInfo.guid), sizeof(GUID));
      enumInfo.hmon = (HMONITOR)bInfo->hMon;
      ddEnumEx( ddEnumCbEx, &enumInfo, DDENUM_ATTACHEDSECONDARYDEVICES );
      ddGuid = &(enumInfo.guid);
      GDBG_INFO(80, "GUID %d-%d-%d-%d-%d%d%d%d\n",
                ddGuid->Data1,
                ddGuid->Data2,
                ddGuid->Data3,
                ddGuid->Data4[0], ddGuid->Data4[1],
                ddGuid->Data4[2], ddGuid->Data4[3], ddGuid->Data4[4]);
    }
  }
  
  //checkSpecialList();
  if (bInfo->lpDD == NULL) {
    /* only create directdraw object once */
    if ( DirectDrawCreate( ddGuid, &(bInfo->lpDD1), NULL ) != DD_OK) {
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
    }
    else GDBG_INFO(80, "DDraw Obj created!\n");
    if ( IDirectDraw_QueryInterface( bInfo->lpDD1, &IID_IDirectDraw2, 
                                     (LPVOID*)&(bInfo->lpDD) ) != DD_OK ) {
      IDirectDraw_Release( bInfo->lpDD1 );
      bInfo->lpDD1 = NULL;
      bInfo->lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;
    }
    else GDBG_INFO(80, "DDraw2 Obj created!\n");
  }
  
  hResult = IDirectDraw2_EnumDisplayModes(bInfo->lpDD, 0, NULL,
                                          (LPVOID) supportedByResolution,
                                          checkResEMCallback);
  
  resetVideo(bInfo);
  return FXTRUE;
#undef FN_NAME
} /* checkResolutions */


void EnableOpenGL(void)
{
  GDBG_INFO(80, "EnableOpenGL: called!\n");
} /* EnableOpenGL */

void EnableMultiRendering(void)
{
  GDBG_INFO(80, "EnableMultiRendering: called!\n");
  _set_multirendering = FXTRUE;
} /* EnableMultiRendering */
