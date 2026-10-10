/*
** This sets up Windowed rendering
**
** For windowed rendering call grSstWinOpenExt with GR_RESOLUTION_NONE.
*/

#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"
#include "fxcmd.h"
#include "gsfc.h"

#if (WINVER < 0x0500) && !defined(HMONITOR_DECLARED) /* <--- HACK */
DECLARE_HANDLE(HMONITOR);
#define HMONITOR_DECLARED
#endif
#include <ddraw.h>

#ifndef IDirectDraw7_CreateSurface /* ddraw.h not from dx7 sdk */
typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKEXA)(GUID FAR *, LPSTR, LPSTR, LPVOID, HMONITOR);
typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEEXA)(LPDDENUMCALLBACKEXA, LPVOID, DWORD);
#ifndef DDENUM_ATTACHEDSECONDARYDEVICES
#define DDENUM_ATTACHEDSECONDARYDEVICES 0x00000001L
#endif
#endif

static LPDIRECTDRAW		lpDDraw1 = 0;
static LPDIRECTDRAW2		lpDDraw2 = 0;
static LPDIRECTDRAWCLIPPER      lpClipper  = 0;
static LPDIRECTDRAWSURFACE	lpPrimSurf = 0;
static LPDIRECTDRAWSURFACE	lpColSurf = 0;
static LPDIRECTDRAWSURFACE	lpAuxSurf = 0;
static LPDIRECTDRAWSURFACE	lpTexSurf0 = 0;
static LPDIRECTDRAWSURFACE	lpTexSurf1 = 0;
static HWND			thehWnd = 0;
static DWORD			surfWidth = 0;
static DWORD			surfHeight = 0;

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

/* This will release the context, and release the surfaces, and destroy the DDRAW objects */
void _grReleaseWindowSurface(GrContext_t ctx)
{
  GDBG_INFO(80, "_grReleaseWindowSurface\n");
  
  /* Release Context */
  grSurfaceReleaseContext(ctx);
  GDBG_INFO(80, "Released context!\n");
  
  /* Now release all the surfaces */
  if (lpClipper)
    {
      IDirectDrawSurface2_Release( lpClipper );
      lpClipper = 0;
    }
  if (lpTexSurf1) 
    {
      IDirectDrawSurface2_Release( lpTexSurf1 );
      lpTexSurf1 = 0;
    }
  if (lpTexSurf0) 
    {
      IDirectDrawSurface2_Release( lpTexSurf0 );
      lpTexSurf0 = 0;
    }
  if (lpAuxSurf) 
    {
      IDirectDrawSurface2_Release( lpAuxSurf );
      lpAuxSurf = 0;
    }
  if (lpColSurf) 
    {
      IDirectDrawSurface2_Release( lpColSurf );
      lpColSurf = 0;
    }
  if (lpPrimSurf) 
    {
      IDirectDrawSurface2_Release( lpPrimSurf );
      lpPrimSurf = 0;
    }
  GDBG_INFO(80, "All surfaces released\n");
  
  /* And the DirectDraw objects */
  if (lpDDraw2) 
    {
      IDirectDraw2_Release( lpDDraw2 );
      lpDDraw2 = NULL;
    }
  if (lpDDraw1) 
    {
      IDirectDraw_Release( lpDDraw1 );
      lpDDraw1 = NULL;
    }
  GDBG_INFO(80, "Direct Draw released\n");
  
}

#define GDBG_DDERR(hResult) \
      switch ( hResult ) \
        { \
        case DDERR_INVALIDOBJECT: \
          GDBG_INFO(80, "DERR_INVALIDOBJECT\n"); \
          break; \
        case DDERR_INVALIDPARAMS: \
          GDBG_INFO(80, "DDERR_INVALIDPARAMS\n"); \
          break; \
        case DDERR_OUTOFVIDEOMEMORY: \
          GDBG_INFO(80, "DDERR_OUTOFVIDEOMEMORY\n"); \
          break; \
        case DDERR_NODIRECTDRAWHW: \
          GDBG_INFO(80, "DDERR_NODIRECTDRAWHW\n"); \
          break; \
        case DDERR_NOCOOPERATIVELEVELSET: \
          GDBG_INFO(80, "DDERR_NOCOOPERATIVELEVELSET\n"); \
          break; \
        case DDERR_INVALIDCAPS: \
          GDBG_INFO(80, "DDERR_INVALIDCAPS\n"); \
          break; \
        case DDERR_INVALIDPIXELFORMAT: \
          GDBG_INFO(80, "DDERR_INVALIDPIXELFORMAT\n"); \
          break; \
        case DDERR_NOALPHAHW: \
          GDBG_INFO(80, "DDERR_NOALPHAHW\n"); \
          break; \
        case DDERR_NOFLIPHW: \
          GDBG_INFO(80, "DDERR_NOFLIPHW\n"); \
          break; \
        case DDERR_NOZBUFFERHW: \
          GDBG_INFO(80, "DDERR_NOZBUFFERHW\n"); \
          break; \
        case DDERR_NOEXCLUSIVEMODE: \
          GDBG_INFO(80, "DDERR_NOEXCLUSIVEMODE\n"); \
          break; \
        case DDERR_OUTOFMEMORY: \
          GDBG_INFO(80, "DDERR_OUTOFMEMORY\n"); \
          break; \
        case DDERR_PRIMARYSURFACEALREADYEXISTS: \
          GDBG_INFO(80, "DDERR_PRIMARYSURFACEALREADYEXISTS\n"); \
          break; \
        case DDERR_NOEMULATION: \
          GDBG_INFO(80, "DDERR_NOEMULATION\n"); \
          break; \
        case DDERR_INCOMPATIBLEPRIMARY: \
          GDBG_INFO(80, "DDERR_INCOMPATIBLEPRIMARY\n"); \
          break; \
        }

/* This will create our DirectDraw surfaces and create the context */
GrContext_t _grCreateWindowSurface(FxU32                 hWnd,
                                   GrColorFormat_t	format,
                                   GrOriginLocation_t	origin,
                                   GrPixelFormat_t	pixelformat,
                                   int			nAuxBuffer)
{
  static GrContext_t ctx = 0;
  GrGC *gc;
  RECT clientRect;
  LPGUID ddGuid = 0;
  HMODULE ddraw = 0;
  DDSURFACEDESC ddsd;
  HRESULT hResult;
  FxI32 bpp = 16;
  
  GDBG_INFO(80, "_grCreateWindowSurface( 0x%x, %d, %d, %d, %d )\n",
            hWnd, format, origin, pixelformat, nAuxBuffer);

  /* check */
  if (lpClipper || lpTexSurf1 || lpTexSurf0 || lpAuxSurf ||
      lpColSurf || lpPrimSurf || lpDDraw2 || lpDDraw1) {
    if (ctx) {
      _grReleaseWindowSurface(ctx);
    } else {
      GDBG_INFO(80, "Failed check!\n");
      return 0;
    }
  }
  
  /* Allocate a context */
  ctx = grSurfaceCreateContext(GR_SURFACECONTEXT_WINDOWED);
  gc = (GrGC *)ctx;
  
  thehWnd = (HWND) hWnd;
  
  GetClientRect(thehWnd, &clientRect);
  surfWidth = clientRect.right - clientRect.left;
  surfHeight = clientRect.bottom - clientRect.top;
  GDBG_INFO(80, "surfWidth  = %d\n", surfWidth);
  GDBG_INFO(80, "surfHeight = %d\n", surfHeight);
  
  ddraw = GetModuleHandle( "ddraw.dll" );
  if ( ddraw ) {
    LPDIRECTDRAWENUMERATEEXA ddEnumEx;
    ddEnumEx = (LPDIRECTDRAWENUMERATEEXA)GetProcAddress( ddraw, "DirectDrawEnumerateExA" );
    if ( ddEnumEx ) {
      EnumInfo enumInfo;
      ZeroMemory(&enumInfo, sizeof(enumInfo));
      ZeroMemory(&enumInfo.guid, sizeof(GUID));
      enumInfo.hmon = (HMONITOR)gc->bInfo->hMon;
      ddEnumEx( ddEnumCbEx, &enumInfo, DDENUM_ATTACHEDSECONDARYDEVICES );
      ddGuid = &enumInfo.guid;
      GDBG_INFO(80, "GUID %d\n", ddGuid);
    }
  }
  
  /* Create DirectDraw */
  if (DirectDrawCreate( ddGuid, &lpDDraw1, NULL ) != DD_OK) {
    GDBG_INFO(80, "DDraw Obj Create Failed!\n");
    _grReleaseWindowSurface(ctx);
    return 0;
  }
  GDBG_INFO(80, "DDraw Obj created!\n");
  
  /* Get us a DirectDraw2 interface */
  if (IDirectDraw_QueryInterface( lpDDraw1, &IID_IDirectDraw2, (LPVOID*)&lpDDraw2 ) != DD_OK) {
    GDBG_INFO(80, "DDraw2 Obj Create Failed!\n");
    _grReleaseWindowSurface(ctx);
    return 0;
  } 
  GDBG_INFO(80, "DDraw2 Obj created!\n");
  
  /* Verify screen pixel format */
  memset(&ddsd, 0, sizeof(ddsd));
  ddsd.dwSize = sizeof( ddsd );
  if (IDirectDraw2_GetDisplayMode(lpDDraw2, &ddsd) != DD_OK) {
    GDBG_INFO(80, "Couldn't get Display Mode!\n");
    _grReleaseWindowSurface(ctx);
    return 0;
  }
  switch (pixelformat) {
  case GR_PIXFMT_ARGB_8888:
  case GR_PIXFMT_AA_2_ARGB_8888:
  case GR_PIXFMT_AA_4_ARGB_8888:
  case GR_PIXFMT_AA_8_ARGB_8888:
    bpp = 32;
    if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
      if (ddsd.ddpfPixelFormat.dwRGBBitCount != 32) {
        GDBG_INFO(80, "Display is not in 32bpp format!\n");
        _grReleaseWindowSurface(ctx);
        MessageBox( thehWnd, "Desktop must be set to 32bbp!", "ERROR", MB_OK);
        return 0;
      }
    } else {
      GDBG_INFO(80, "Cannot render in 32bpp format!\n");
      _grReleaseWindowSurface(ctx);
      MessageBox( thehWnd, "Cannot render in 32bpp format!", "ERROR", MB_OK);
      return 0;
    }
    break;
  default:
    bpp = 16;
    if (ddsd.ddpfPixelFormat.dwRGBBitCount != 16) {
      GDBG_INFO(80, "Display is not in 16bpp format!\n");
      _grReleaseWindowSurface(ctx);
      MessageBox( thehWnd, "Desktop must be set to 16bbp!", "ERROR", MB_OK);
      return 0;
    }
  }
  
  /* Set the Coop level to normal */
  if (IDirectDraw2_SetCooperativeLevel(lpDDraw2, thehWnd, DDSCL_NORMAL) != DD_OK) {
    GDBG_INFO(80, "SetCooperativeLevel Failed!\n");
    _grReleaseWindowSurface(ctx);
    return 0;
  }
  
  /* Move all of the pieces of surface memory on the video card
   * to a contiguous block to make the largest chunk of free
   * memory available.
   */
  /* Works only for DDSCL_EXCLUSIVE.
  if (IDirectDraw2_Compact(lpDDraw2) != DD_OK) {
    GDBG_INFO(80, "Compact Failed!\n");
  }*/
  
  /* Now create us a Primary surface */
  memset(&ddsd, 0, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
  
  hResult = IDirectDraw2_CreateSurface( lpDDraw2, &ddsd, &lpPrimSurf, 0 );
  if (hResult != DD_OK) {
    GDBG_INFO(80, "Primary Surface Create Failed!\n");
    GDBG_DDERR(hResult);
    _grReleaseWindowSurface(ctx);
    return 0;
  }
  
  /* Setup Clipper */
  if (IDirectDraw2_CreateClipper( lpDDraw2, 0, &lpClipper, 0 ) != DD_OK) {
    GDBG_INFO(80, "Clipper Failted!\n");
    _grReleaseWindowSurface(ctx);
    return 0;
  }
  
  if (IDirectDrawClipper_SetHWnd( lpClipper, 0, thehWnd ) != DD_OK) {
    GDBG_INFO(80, "Clipper Failted!\n");
    _grReleaseWindowSurface(ctx);
    return 0;
  }
  
  if (IDirectDrawSurface2_SetClipper( lpPrimSurf, lpClipper ) != DD_OK) {
    GDBG_INFO(80, "Clipper Failted!\n");
    _grReleaseWindowSurface(ctx);
    return 0;
  }
  
  /* Setup Color Surface */
  GDBG_INFO(80, "Setting up Color Surface\n");
  memset(&ddsd, 0, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH;/*|DDSD_PIXELFORMAT;*/
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;/*|DDSCAPS_VIDEOMEMORY;*/
  ddsd.dwWidth = surfWidth;
  ddsd.dwHeight = surfHeight;
  /*ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
  ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
  ddsd.ddpfPixelFormat.dwRGBBitCount = bpp;*/
  
  hResult = IDirectDraw2_CreateSurface( lpDDraw2, &ddsd, &lpColSurf, 0 );
  if (hResult != DD_OK) {
    GDBG_INFO(80, "Colour Surface Create Failed!\n");
    GDBG_DDERR(hResult);
    _grReleaseWindowSurface(ctx);
    return 0;
  }
  
  /* Setup Aux Surface */
  if (nAuxBuffer) {
    GDBG_INFO(80, "Setting up Aux Surface\n");
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH;/*|DDSD_PIXELFORMAT;*/
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;/*|DDSCAPS_VIDEOMEMORY;*/
    ddsd.dwWidth = surfWidth;
    ddsd.dwHeight = surfHeight;
    /*ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount = bpp;*/
    
    hResult = IDirectDraw2_CreateSurface( lpDDraw2, &ddsd, &lpAuxSurf, 0 );
    if (hResult != DD_OK) {
      GDBG_INFO(80, "Aux Surface Create Failed!\n");
      GDBG_DDERR(hResult);
      _grReleaseWindowSurface(ctx);
      return 0;
    }
  }
  
  /* Setup Texture Surface */
  GDBG_INFO(80, "Setting up Texture Surface\n");
  memset(&ddsd, 0, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH;/*|DDSD_PIXELFORMAT;*/
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;/*|DDSCAPS_VIDEOMEMORY;*/
  /* XXX [koolsmoky] We're screwed if the texture is larger than 1024*1024 (Napalm). */
  ddsd.dwWidth = 1024;
  ddsd.dwHeight = 1024;
  /*ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
  ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
  ddsd.ddpfPixelFormat.dwRGBBitCount = bpp;*/
  
  hResult = IDirectDraw2_CreateSurface( lpDDraw2, &ddsd, &lpTexSurf0, 0 );
  if (hResult != DD_OK) {
    GDBG_INFO(80, "Tex Surface0 Create Failed!\n");
    GDBG_DDERR(hResult);
    _grReleaseWindowSurface(ctx);
    return 0;
  }
  if (gc->num_tmu == 2) {
    hResult = IDirectDraw2_CreateSurface( lpDDraw2, &ddsd, &lpTexSurf1, 0 );
    if (hResult != DD_OK) {
      GDBG_INFO(80, "Tex Surface1 Create Failed!\n");
      GDBG_DDERR(hResult);
      /* XXX [koolsmoky] Single TMU if we're out of video memory. */
      GDBG_INFO(80, "Falling back to single TMU\n");
      lpTexSurf1 = 0;
      gc->num_tmu = 1;
    }
  }
  
  /* Select the Context */
  grSelectContext(ctx);
  
  /* Set the Render Buffer */
  grSurfaceSetRenderingSurface((GrSurface_t*) lpColSurf, FXFALSE);
  
  /* Set the Aux Buffer */
  if (nAuxBuffer) grSurfaceSetAuxSurface((GrSurface_t*) lpAuxSurf);
  
  /* Now create the texture buffer0 and 1 */
  grSurfaceSetTextureSurface(GR_TMU0, (GrSurface_t*) lpTexSurf0);
  if (gc->num_tmu == 2) grSurfaceSetTextureSurface(GR_TMU1, (GrSurface_t*) lpTexSurf1);
  
  /* Set some 'crap' */
  gc->grHwnd             = hWnd;
  gc->grColBuf           = gc->state.num_buffers = 2;
  gc->grAuxBuf           = nAuxBuffer;
  gc->grPixelFormat      = (int) pixelformat;
  gc->chipmask           = SST_CHIP_MASK_ALL_CHIPS;
  gc->state.color_format = format;
  gc->sliCount           = 1;
  gc->chipCount          = 1;
  gc->grSamplesPerChip   = 1;
  grSstOrigin(origin);
  
  /* Clear the buffers */
  grBufferClear( 0x0, 0, 0 );
  
  return ctx;
}

/* This will blit/flip our surfaces */
void _grFlipWindowSurface()
{
  GDBG_INFO(80, "_grFlipWindowSurface\n");
  
  /* Blit from col to prim */
  if (lpPrimSurf && lpColSurf) {
    POINT ptClient;
    RECT src, dest;
    HRESULT hResult;
    
    /* XXX [koolsmoky] Screen will be stretched/shrinked to fit window */
    ptClient.x = 0;
    ptClient.y = 0;
    ClientToScreen(thehWnd, &ptClient);
    GetClientRect(thehWnd, &dest);
    OffsetRect(&dest, ptClient.x, ptClient.y);
    SetRect(&src, 0, 0, surfWidth, surfHeight);
    
    hResult = IDirectDrawSurface_Blt(lpPrimSurf, &dest, lpColSurf, &src, DDBLT_WAIT, NULL);
    while(IDirectDrawSurface_GetBltStatus(lpPrimSurf, DDGBS_ISBLTDONE) != DD_OK)
      ;
    if (hResult != DD_OK) {
      GDBG_INFO(80, "Couldn't Blit!\n");
    }
  }
}
