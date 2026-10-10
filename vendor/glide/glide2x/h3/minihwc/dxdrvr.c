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
** 3     3/17/99 6:14p Dow
** Fixes Phantom Menace, OGL, and others.
** 
** 43    6/24/98 11:05a Dow
** better error message
** 
** 42    6/11/98 7:45p Jdt
** Win98/NT5 Multimon 1st Pass
** 
** 41    6/04/98 6:52p Dow
** Resolutions to 1600x1200
** 
** 40    5/27/98 3:10p Dow
** DOS happiness
** 
** 39    4/14/98 11:55a Dow
** 
** 38    4/05/98 2:19p Dow
** DOS/WIN32 stepping on each others' feet
** 
** 37    4/03/98 2:03p Dow
** DOS Glide modes
** 
** 36    3/11/98 8:27p Dow
** WinGlide
 * 
 * 33    11/06/97 9:51a Jdt
 * Fixes window resize events to not crash/hang
 * 
 * 32    10/01/97 3:21p Atai
 * fix 819, DOS tests flicker on glide
 * 
 * 31    9/08/97 12:05p Atai
 * fixed resize problem in dxControl()
 * 
 * 30    8/28/97 10:24a Odo
 * moved debugging varaible
 * 
 * 29    8/27/97 10:32a Dow
 * added more debugging info
 * 
 * 28    8/21/97 4:24p Dow
 * Fixed @#$%! C++ Comments, Added Debugging info
**
*/

/* 3Dfx Headers */
#include <3dfx.h>
#include <fxpci.h>
#include <gdebug.h>

#include <stdio.h>
#define FX_DLL_DEFINIION
#include <fxdll.h>
#include <string.h>

#ifdef __DOS32__
#include <i86.h>
#endif

/* Local Headers */
#include "minihwc.h"

#define H3_BUF_MAGIC  0x666

FxBool tripleBuffering = 0;

#ifdef GDBG_INFO_ON
static char *bufTypeNames[] = {
  "HWC_BUFFER_FRONTBUFFER",
  "HWC_BUFFER_BACKBUFFER",
  "HWC_BUFFER_AUXBUFFER",
  "HWC_BUFFER_DEPTHBUFFER",
  "HWC_BUFFER_ALPHABUFFER",
  "HWC_BUFFER_TRIPLEBUFFER",
  "HWC_BUFFER_FIFOBUFFER",
  "HWC_BUFFER_SCREENBUFFER",
  "HWC_BUFFER_TEXTUREBUFFER"
};
#endif

#if defined( __WIN32__) && defined(HWC_ACCESS_DDRAW)
#define WIN32_LEAN_AND_MEAN 
#include <windows.h> 
#include <ddraw.h> 

#define NUM_BUFS 6

static HWND                 hWndApp     = 0;
static FxU32                FifoOffset  = 0;
static FxU32                Width       = 0; 
static FxU32                Height      = 0;
static FxBool               IsFullScreen = 0;
static hwcBufferDesc        BufDesc[NUM_BUFS]  = {0};

/* Direct Draw stuff */
static LPDIRECTDRAWCLIPPER  lpClipper   = NULL;
static LPDIRECTDRAWSURFACE  lpFront     = NULL;
static LPDIRECTDRAWSURFACE  lpBack      = NULL;
static LPDIRECTDRAWSURFACE  lpAux       = NULL;
static LPDIRECTDRAWSURFACE  lpTriple    = NULL;
static LPDIRECTDRAW2        lpDD        = NULL;
static LPDIRECTDRAW         lpDD1       = NULL;



FxBool
ErrorMessage(HWND hWnd, char *err)
{
  GDBG_INFO(0, "Error %s \n", err);
//  MessageBox( hWnd, err, "ERROR", MB_OK );
  return FALSE;
} /* ErrorMessage */



/*
 * Takes a DirectDraw Surface object (LPDIRECTDRAWSURFACE), looks up its
 * description (DDSURFACEDESC), and returns a pointer to the surface and its
 * stride value in pDesc.
 */
FxBool
_dxSurfaceToBufDesc(LPDIRECTDRAWSURFACE lpSurf, hwcBufferDesc *pDesc)
{
  DDSURFACEDESC   ddsd;
  
  ddsd.dwSize = sizeof(ddsd);
  if (IDirectDrawSurface2_Lock(lpSurf, NULL, &ddsd, DDLOCK_WAIT, NULL) 
      !=DD_OK) {
    return ErrorMessage(hWndApp, "_dxSurfaceToBufDesc: DdrawSurface Lock failed\n");
  }
  
  /* pDesc->bufType should be filled in by caller */
  pDesc->bufOffset = (FxU32) (ddsd.lpSurface);        /* virtual address. */
  pDesc->bufStride = (FxI32) (ddsd.lPitch);           /* in bytes         */
  pDesc->bufBPP    = (FxI32) 16;
  
  IDirectDrawSurface2_Unlock(lpSurf, NULL);
  return FXTRUE;
} /* _dxSurfaceToBufDesc */



/*
 * Convert front, back and aux surfaces from DDraw to Glide bufDescriptors.
 */
FxBool
_dxDDrawToGlideDesc(hwcBufferDesc *pDesc)
{
  hwcBufferDesc   *dFront  = &pDesc[0];
  hwcBufferDesc   *dBack   = &pDesc[1];
  hwcBufferDesc   *dAux    = &pDesc[2];
  hwcBufferDesc   *dFifo   = &pDesc[3];
  hwcBufferDesc   *dScreen = &pDesc[4];
  hwcBufferDesc   *dTriple = &pDesc[5];
  
  GDBG_INFO(80, "_dxDDrawToGlide entry\n");
  
  /* Initialize all descriptors. */
  dScreen->bufMagic  = H3_BUF_MAGIC;
  dScreen->bufType   = HWC_BUFFER_SCREENBUFFER;
  dScreen->bufOffset = 0;
  dScreen->bufStride = 0;
  dScreen->bufBPP    = 0;
  
  /* Get info about screen (primary display) */
  dScreen->bufType  = HWC_BUFFER_SCREENBUFFER;
  if (!_dxSurfaceToBufDesc( lpFront, dScreen))
    return ErrorMessage(hWndApp, "Couldn't get Screen Info");
  
  *dFront = *dBack = *dAux = *dFifo =
  *dTriple =
  *dScreen;
  
  dFront->bufType   = HWC_BUFFER_FRONTBUFFER;

  /* Get info about back buffer */
  dBack->bufType  = HWC_BUFFER_BACKBUFFER;
  if (!_dxSurfaceToBufDesc( lpBack, dBack))
    return ErrorMessage(hWndApp, "Couldn't get Backbuffer Info");
  
  /* Get info about aux buffer */
  dAux->bufType = HWC_BUFFER_AUXBUFFER;
  if (!_dxSurfaceToBufDesc( lpAux, dAux)) 
    return ErrorMessage(hWndApp, "Couldn't get AuxBuffer Info");
  
  /* Get info about triple buffer */
  if (tripleBuffering) {
    dTriple->bufType  = HWC_BUFFER_TRIPLEBUFFER;
    if (!_dxSurfaceToBufDesc( lpTriple, dTriple))
      return ErrorMessage(hWndApp, "Couldn't get triple buffer Info");
  }

  /* Get info about cmdFifo */
  dFifo->bufType   = HWC_BUFFER_FIFOBUFFER;
  dFifo->bufOffset = FifoOffset;
  dFifo->bufStride = 65536 << 1; /* Fixme!!! what's this? */
  
  /* Now convert buf addresses for draw buffers into video memory offsets */
  dFront->bufOffset -= dScreen->bufOffset;
  dBack ->bufOffset -= dScreen->bufOffset;
  dAux  ->bufOffset -= dScreen->bufOffset;
  if (tripleBuffering)
    dTriple->bufOffset -= dScreen->bufOffset;
  
  GDBG_INFO(80, "_dxDDrawToGlideDesc: dFront->bufOffset = 0x%x\n", dFront->bufOffset);
  GDBG_INFO(80, "_dxDDrawToGlideDesc: dBack->bufOffset = 0x%x\n", dBack->bufOffset);
  GDBG_INFO(80, "_dxDDrawToGlideDesc: dAux->bufOffset = 0x%x\n", dAux->bufOffset);
  GDBG_INFO(80, "_dxDDrawToGlideDesc: dFifo->bufOffset = 0x%x\n", dFifo->bufOffset);
  if (tripleBuffering)
    GDBG_INFO(80, "_dxDDrawToGlideDesc: dTriple->bufOffset = 0x%x\n", dTriple->bufOffset);

  GDBG_INFO(80,
    "F:%.06x %5d B:%.06x %5d B2:%.06x %5d A:%.06x %5d, C:%.06x %5d\n",
             dFront->bufOffset, dFront->bufStride,
             dBack ->bufOffset, dBack ->bufStride,
             dTriple->bufOffset, dTriple->bufStride,
             dAux  ->bufOffset, dAux  ->bufStride,
             dFifo ->bufOffset, dFifo ->bufStride);
  return TRUE;
} /* _dxDDrawToGlideDesc */


static GUID fooGuid;

BOOL FAR PASCAL ddEnumCbEx( GUID FAR *guid, LPSTR desc, LPSTR name, LPVOID ctx, HMONITOR hmon ) {
    DWORD    *data  = (DWORD*)ctx;
    HMONITOR target = (HMONITOR)data[0];
    BOOL     rv     = DDENUMRET_OK;

    if ( target == hmon ) {
        fooGuid = *guid;
        data[1] = (DWORD)&fooGuid;
        rv     = DDENUMRET_CANCEL;
    }
    return rv;
}

/*
 * Allocate (or re-allocate for WM_SIZE) buffers from DDraw.
 */
FxBool
_dxAllocSurfaces(int xRes, int yRes, int vRefresh, hwcBufferDesc *pDesc, void *hmon )
{
  DDSURFACEDESC   ddsd;
  LPGUID          ddGuid;
  
  GDBG_INFO(80, "_dxAS: hWnd = %x, fs=%d, xRes=%d, yRes=%d, vRefresh=%d\n", 
             hWndApp, IsFullScreen, xRes, yRes, vRefresh);


  ddGuid = NULL;
  {
      HMODULE ddraw = GetModuleHandle( "ddraw.dll" );
      
      if ( ddraw ) {
          LPDIRECTDRAWENUMERATEEXA ddEnumEx;
          ddEnumEx = (void*)GetProcAddress( ddraw, "DirectDrawEnumerateExA" );
          if ( ddEnumEx ) {
              DWORD   data[2];
              data[0] = (DWORD)hmon;
              data[1] = 0;
              ddEnumEx( ddEnumCbEx, data, DDENUM_ATTACHEDSECONDARYDEVICES );
              if ( data[1] ) {
                  ddGuid = (LPGUID)data[1];
              }
          }
      }
  }

  
  if (lpDD1 == NULL) {
    if (DirectDrawCreate( ddGuid, &lpDD1, NULL ) != DD_OK) {
      return ErrorMessage(hWndApp, "DirectDrawCreate Failed!");
    } else {
      GDBG_INFO(80, "_dxAS: DDraw Obj created!\n");
    }
  } else {
    GDBG_INFO(80, "_dxAS: DDraw Obj already existed!\n");
  }

  lpDD = 0L;

  if (IDirectDraw_QueryInterface( lpDD1, &IID_IDirectDraw2, 
                                   (LPVOID*)&lpDD) !=DD_OK) {

    IDirectDraw_Release( lpDD1 );
    lpDD1 = NULL;
    lpDD  = NULL;
    return ErrorMessage(hWndApp, "DDraw2 interface object failed!");
  } else {
    GDBG_INFO(80, "_dxAS: DDraw2 Obj created!\n");
  }
  
  /*
   * Make sure this is done after creating direct draw object!!!
   * First time, only one time, get ourselves a command fifo.
   * Fix me - XXX
   */
  if (FifoOffset == 0) {
    /* CHD HACK!!! */
    FifoOffset = 0x200000U - 65536*3; /* fixme!!! why 3? */
  }
  
  /*
   * If there are any previously allocated surfaces, free them now
   * before asking for more.
   */
  if( lpClipper) IDirectDrawSurface2_Release( lpClipper);
  if( lpAux    ) IDirectDrawSurface2_Release( lpAux    );
  if( lpBack   ) IDirectDrawSurface2_Release( lpBack   );
  if( lpTriple ) IDirectDrawSurface2_Release( lpTriple    );
  if( lpFront  ) IDirectDrawSurface2_Release( lpFront  );
  
  lpClipper = NULL;
  lpFront   = NULL;
  lpBack    = NULL;
  lpTriple    = NULL;
  lpAux     = NULL;
  
  if ( !IsFullScreen ) {        /* In a window */

    GDBG_INFO(80, "_dxAS: Allocating buffers for a windowed mode\n");

    /* Verify screen pixel format is 16bpp, and set cooperative level */
    ddsd.dwSize = sizeof( ddsd );
    if (IDirectDraw2_GetDisplayMode( lpDD, &ddsd ) != DD_OK)
      return ErrorMessage(hWndApp,  "Couldn't get display mode!");

    if (ddsd.ddpfPixelFormat.dwRGBBitCount / 8 != 2) 
      return ErrorMessage(hWndApp,  "Display is not in 16bpp format!");

    if (IDirectDraw2_SetCooperativeLevel(lpDD,hWndApp, DDSCL_NORMAL ) != DD_OK)
      return ErrorMessage(hWndApp, "Couldn't set cooperative level!");

    /* Allocate Front Buffer Surface */
    ddsd.dwSize         = sizeof( ddsd );
    ddsd.dwFlags        = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
        
    if (IDirectDraw2_CreateSurface( lpDD, &ddsd, &lpFront, 0 ) != DD_OK) 
      return ErrorMessage(hWndApp, "Couldn't allocate primary surface!");

    GDBG_INFO(80, "_dxAS: Screen: xRes = %d, yRes = %d, stride = %d\n",
               xRes, yRes, ddsd.lPitch);
    /* From jdt */
    if (IDirectDraw2_CreateClipper( lpDD, 0, &lpClipper, 0 ) != DD_OK) 
      return ErrorMessage(hWndApp, "Create Clipper failed!\n");

    if (IDirectDrawClipper_SetHWnd( lpClipper, 0, hWndApp ) != DD_OK) 
      return ErrorMessage(hWndApp, "Clipper SethWnd failed!\n");

    if (IDirectDrawSurface2_SetClipper( lpFront, lpClipper ) != DD_OK) 
      return ErrorMessage(hWndApp, "Set Clipper failed!\n");

    /* Always allocate back buffer.
     */
    ddsd.dwSize         = sizeof( ddsd );
    ddsd.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwWidth        = xRes;
    ddsd.dwHeight       = yRes;
    ddsd.ddsCaps.dwCaps =
      DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
        
    if (IDirectDraw2_CreateSurface( lpDD, &ddsd, &lpBack, 0 ) != DD_OK) 
      return ErrorMessage(hWndApp,  "Couldn't allocate secondary surface!" );

    GDBG_INFO(80, "_dxAS: Back buffer allocated!\n");

    /* 
     * Always allocate Aux Buffer Surface 
     * XXX - (use nAuxBuffers, and deal with 8bpp for alpha)
     */
    ddsd.dwSize         = sizeof( ddsd );
    ddsd.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwWidth        = xRes;
    ddsd.dwHeight       = yRes;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY
        | DDSCAPS_3DDEVICE;
        
    if (IDirectDraw2_CreateSurface( lpDD, &ddsd, &lpAux, 0 ) != DD_OK) 
      return ErrorMessage(hWndApp,  "Couldn't allocate aux surface" );

    GDBG_INFO(80, "_dxAS: Aux Buffer allocated!\n");

  } else {
    
    /* Full screen - Set Exclusive Mode, change resolution,  */
    GDBG_INFO(80, "_dxAS: Setting Full screen exclusive mode!\n");
    
    if (IDirectDraw2_SetCooperativeLevel(lpDD, hWndApp, 
                                         DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN ) != DD_OK)
      return ErrorMessage(hWndApp, "Couldn't set cooperative level!");
    
    if (IDirectDraw2_SetDisplayMode( lpDD, xRes, yRes, 16, 0,0) != DD_OK)
      return ErrorMessage(hWndApp,  "Couldn't set display mode!");
    
#if WE_WERE_HELPLESS    
    /* Allocate Front/Back Buffer Surfaces, skip triple buffer XXX */
    ddsd.dwSize            = sizeof( ddsd );
    ddsd.dwFlags           = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.dwBackBufferCount = 1;
    ddsd.ddsCaps.dwCaps    = DDSCAPS_PRIMARYSURFACE | 
      DDSCAPS_FLIP | 
        DDSCAPS_COMPLEX |
          DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
    
    if (IDirectDraw2_CreateSurface( lpDD, &ddsd, &lpFront, 0 ) != DD_OK) 
      return ErrorMessage(hWndApp, "Couldn't allocate primary surface!");
    
    /* Get back buffer information */
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    if (IDirectDrawSurface2_GetAttachedSurface( lpFront, &ddsd.ddsCaps, 
                                               &lpBack) != DD_OK)
      return ErrorMessage(hWndApp, "Couldn't get back buffer info!\n");
    
    GDBG_INFO(80, "_dxAS: Full Screen: front/back buffer allocated!\n");
    
    /* Allocate Aux Buffer Surface - XXX */
    ddsd.dwSize         = sizeof( ddsd );
    ddsd.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwWidth        = xRes;
    ddsd.dwHeight       = yRes;
    ddsd.ddsCaps.dwCaps = 
      DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
    
    if (IDirectDraw2_CreateSurface( lpDD, &ddsd, &lpAux, 0 ) != DD_OK) 
      return ErrorMessage(hWndApp,  "Couldn't allocate aux surface" );

    GDBG_INFO(80, "_dxAS: Full Screen: aux buffer allocated!\n");
    
/*
 * Triple buffer hack: full-screen only for now.  Allocate as Aux DD surf,
 * after everything else N.B. Allocation order may be important!!!
 * god knows how DD would implement dwBackBufferCount == 2
 */
    if (tripleBuffering) {
      if (IDirectDraw2_CreateSurface( lpDD, &ddsd, &lpTriple, 0 ) != DD_OK) 
        return ErrorMessage(hWndApp,  "Couldn't allocate triple buffer" );

      GDBG_INFO(80, "_dxAS: Triple buffer allocated!\n");
    }
#endif

  }
#if WE_WERE_HELPLESS    
  /* Convert direct draw surfaces to Glide's idea of buffer descriptors */
  return _dxDDrawToGlideDesc(pDesc);
#else  /* We help ourselves */
  return FXTRUE;
#endif

} /* _dxAllocSurfaces */

FxBool
_dxReallocSurfaces(int xRes, int yRes, int vRefresh, hwcBufferDesc *pDesc)

{
  DDSURFACEDESC   ddsd;
  const char      *errorMessage = "no error";

  /* for now this is a goddamn baseball bat 
     it can be trimmed down */
  if ( !IsFullScreen ) { /* this *REALLY* should be called for fullscreen */
    if( lpClipper) IDirectDrawClipper_Release( lpClipper);
    if( lpAux    ) IDirectDrawSurface2_Release( lpAux    );
    if( lpBack   ) IDirectDrawSurface2_Release( lpBack   );
    if( lpTriple ) IDirectDrawSurface2_Release( lpTriple    );
    if( lpFront  ) IDirectDrawSurface2_Release( lpFront  );
    
    lpClipper = NULL;
    lpFront   = NULL;
    lpBack    = NULL;
    lpTriple  = NULL;
    lpAux     = NULL;
  
    /* Verify screen pixel format is 16bpp, and set cooperative level */
    ddsd.dwSize = sizeof( ddsd );

    /* Allocate Front Buffer Surface */
    ddsd.dwSize         = sizeof( ddsd );
    ddsd.dwFlags        = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
        
    if (IDirectDraw2_CreateSurface( lpDD, &ddsd, &lpFront, 0 ) != DD_OK) {
      errorMessage =  "Couldn't allocate primary surface!";
      goto FUBAR;
    }

    /* From jdt */
    if (IDirectDraw2_CreateClipper( lpDD, 0, &lpClipper, 0 ) != DD_OK) {
      errorMessage = "Create Clipper failed!\n";
      goto FUBAR;
    }

    if (IDirectDrawClipper_SetHWnd( lpClipper, 0, hWndApp ) != DD_OK) {
      errorMessage = "Clipper SethWnd failed!\n";
      goto FUBAR;
    }

    if (IDirectDrawSurface2_SetClipper( lpFront, lpClipper ) != DD_OK) {
      errorMessage = "Set Clipper failed!\n";
      goto FUBAR;
    }

    /* Always allocate back buffer.
     */
    ddsd.dwSize         = sizeof( ddsd );
    ddsd.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwWidth        = xRes;
    ddsd.dwHeight       = yRes;
    ddsd.ddsCaps.dwCaps =
      DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
        
    if (IDirectDraw2_CreateSurface( lpDD, &ddsd, &lpBack, 0 ) != DD_OK) {
      errorMessage = "Couldn't allocate secondary surface!";
      goto FUBAR;
    }

    /* 
     * Always allocate Aux Buffer Surface 
     * XXX - (use nAuxBuffers, and deal with 8bpp for alpha)
     */
    ddsd.dwSize         = sizeof( ddsd );
    ddsd.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwWidth        = xRes;
    ddsd.dwHeight       = yRes;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY
        | DDSCAPS_3DDEVICE;
        
    if (IDirectDraw2_CreateSurface( lpDD, &ddsd, &lpAux, 0 ) != DD_OK) {
      errorMessage = "Couldn't allocate aux surface";
      goto FUBAR;
    }
  }
  return _dxDDrawToGlideDesc(pDesc);
FUBAR:
  /* if you don't free these you can't make another attempt */
  if( lpClipper) IDirectDrawClipper_Release( lpClipper);
  if( lpAux    ) IDirectDrawSurface2_Release( lpAux    );
  if( lpBack   ) IDirectDrawSurface2_Release( lpBack   );
  if( lpTriple ) IDirectDrawSurface2_Release( lpTriple    );
  if( lpFront  ) IDirectDrawSurface2_Release( lpFront  );
  lpClipper = NULL;
  lpFront   = NULL;
  lpBack    = NULL;
  lpTriple  = NULL;
  lpAux     = NULL;
  return ErrorMessage( hWndApp, (char*)errorMessage );
} /* _dxReallocSurfaces */

/*
 * Restore lost surfaces, or move the window.
 * Note: Resizing is done by deleting and re-allocation, and is done in
 * _dxAllocSurfaces();
 */
static FxBool
_dxRestoreSurfaces(hwcBufferDesc *pDesc)
{
  if (lpDD == NULL) return TRUE;
  
  if (IDirectDrawSurface2_IsLost(lpFront) == DDERR_SURFACELOST) {
    if (IDirectDrawSurface2_Restore(lpFront) != DD_OK) 
      return ErrorMessage(hWndApp, "Front Buffer restore failure\n");
  }
  
  if (!IsFullScreen) {
    if (IDirectDrawSurface2_IsLost(lpBack) == DDERR_SURFACELOST) {
      if (IDirectDrawSurface2_Restore(lpBack) != DD_OK) 
        return ErrorMessage(hWndApp, "Back  Buffer restore failure\n");
    }
  }
  
  if (IDirectDrawSurface2_IsLost(lpAux) == DDERR_SURFACELOST) {
    if (IDirectDrawSurface2_Restore(lpAux) != DD_OK) 
      return ErrorMessage(hWndApp, "Aux   Buffer restore failure\n");
  }
  
  if (!IsFullScreen) {
    if (IDirectDrawSurface2_IsLost(lpTriple) == DDERR_SURFACELOST) {
      if (IDirectDrawSurface2_Restore(lpTriple) != DD_OK) 
        return ErrorMessage(hWndApp, "Back  Buffer restore failure\n");
    }
  }
  
  GDBG_INFO(80, "_dxRS: restore surfaces OK\n");
  return _dxDDrawToGlideDesc(pDesc);
} /* _dxRestoreSurfaces */

FxBool
dxOpen(
  FxU32                 hWindow,
  GrScreenResolution_t  sRes,
  GrScreenRefresh_t     vRefresh,
  hwcColorFormat        cFormat,
  hwcOriginLocation     yOrigin,
  int                   nColBuffers,
  int                   nAuxBuffers,
  hwcBufferDesc         *pBufDesc,         /* return values */
  int                   *width,
  int                   *height,
  hwcSwapType          *swapType,
  void                  *hmon
)
{
  int     i;

  GDBG_INFO(80, "dxOpen: hWnd = %x, sRes=%d, yOrg=%d, ncb=%d, nab=%d\n",
             hWindow, sRes, yOrigin, nColBuffers, nAuxBuffers);

  /*
   * Get ourselves a valid window handle 
   */
  hWndApp = (hWindow == 0) ? GetActiveWindow() : (HWND) hWindow;

  if (hWndApp == NULL) {
    return MessageBox(NULL, "grSstWinOpen was passed a NULL window handle!\n\
Contact the application Vendor\n",
      "Programmer Error", MB_OK);
  }

  if (sRes == GR_RESOLUTION_NONE ) { /* In a window */
    RECT    rect;

    /* Find out how big the window is */
    GetClientRect(hWndApp, &rect);
    IsFullScreen = FXFALSE;
    Width = rect.right;
    Height = rect.bottom;
  } else {
    /* Full screen mode - convert resolution to Width, Height */
    static int _w[] = {320,320,400,512,640,640,640,640,800,960,856,512,1024,1280,1600,400};
    static int _h[] = {200,240,256,384,200,350,400,480,600,720,480,256,768,1024,1200,300};

    if ((sRes <  GR_RESOLUTION_320x200) ||
        (sRes >  GR_RESOLUTION_1600x1200)) 

      return ErrorMessage(hWndApp, "Bad Fullscreen resolution");

    tripleBuffering = (nColBuffers == 3);

    IsFullScreen = FXTRUE;

    Width   = _w[sRes];
    Height  = _h[sRes];
  }

  GDBG_INFO(80, "dxOpen: W=%d, H=%d, FullScr=%d, vRefresh=%d\n",
             Width, Height, IsFullScreen, vRefresh);

  if (!_dxAllocSurfaces(Width, Height, vRefresh, BufDesc, hmon))
    return ErrorMessage(hWndApp, "failed dxOpen");

  GDBG_INFO(80, "_dxAllocSurfaces OK!!!\n");

  for (i=0; i< NUM_BUFS; i++) {
    pBufDesc[i] = BufDesc[i];
    GDBG_INFO(80, "dxOpen: pBufDesc[%d]: \n", i);
    GDBG_INFO(80, "\tbufMagic = 0x%x\n", pBufDesc[i].bufMagic);
    GDBG_INFO(80, "\tbufType = 0x%x\n", pBufDesc[i].bufType);
    GDBG_INFO(80, "\tbufOffset = 0x%x\n", pBufDesc[i].bufOffset);
    GDBG_INFO(80, "\tbufStride = 0x%x\n", pBufDesc[i].bufStride);    
    GDBG_INFO(80, "\tbufBPP = 0x%x\n", pBufDesc[i].bufBPP);
  }

  *width = Width;
  *height = Height;
  *swapType = (IsFullScreen) ? HWC_SWAP_FLIP : HWC_SWAP_BLT;

  GDBG_INFO(80, "dxOpen:  Returning TRUE\n");

  return FXTRUE;

} /* dxOpen */

FxBool
dxControl(FxU32 code, hwcBufferDesc *pBufDesc, int *width, int *height)
{
  RECT    rect;
  int     i;
  
  GDBG_INFO(80, "dxControl: code = %d\n", code);
  
  /* dx is not initizized */
  if ( lpDD== NULL ) {
    return TRUE;
  }
  
  /* Why was I called? */
  switch(code) {
  case HWC_CONTROL_RESIZE:     /* recreate surfaces */
    GetClientRect(hWndApp, &rect);
    if ((Width != (FxU32) rect.right) || (Height != (FxU32) rect.bottom)) {
      Width = rect.right;
      Height = rect.bottom;
      GDBG_INFO(120, "W and H changed to %d %d\n", 
                 Width, Height); 

      if (!_dxReallocSurfaces(Width, Height, 0, BufDesc)) {
        /*
           Let's find some other way to do this!
           ErrorMessage(hWndApp, "dxControl: Resize failed\n"); */
        return FXFALSE;
      }
    }
    break;

  case HWC_CONTROL_MOVE:
    break;

  case HWC_CONTROL_ACTIVATE:
    GDBG_INFO(120, "dxControl: Activate\n");
    break;

  case HWC_CONTROL_DEACTIVATE:
    GDBG_INFO(120, "dxControl: DeActivate\n");
    break;

  default:
    GDBG_INFO(120, "dxControl: Strange control %d\n", code);
    return FXFALSE;
    break;
  }
  for (i=0; i<5; i++) pBufDesc[i] = BufDesc[i];
  *width = Width;
  *height = Height;
  GDBG_INFO(80, "dxControl: code = %d, w = %d, h = %d\n", code,
             Width, Height);
  
  return FXTRUE;
} /* dxControl */

FxBool
dxClose()
{
  GDBG_INFO(80, "dxClose:\n");
/*
 * fixme! nulling out this code fixes bug 541... why is unclear??? XXX
 *  A little more information... the problem is releasing lpDD1, and
 *  only in fullscreen mode.  Temporary refined hack is to not release
 *  lpDD1 in fullscreen mode.  Also rearrange code in dxAllocSurfaces
 *  to only create lpDD1 once per application execution. /PGJ
 */
#if 1
  // Release any allocated buffers
  if( lpClipper) IDirectDrawSurface2_Release( lpClipper);
  if( lpFront  ) IDirectDrawSurface2_Release( lpFront  );
  if(!IsFullScreen) {
    if( lpBack   ) IDirectDrawSurface2_Release( lpBack   );
  }
  if( lpAux    ) IDirectDrawSurface2_Release( lpAux    );
  if( lpDD     ) IDirectDraw2_Release( lpDD );
  if (1 /* !IsFullScreen */) {
    if( lpDD1    ) IDirectDraw_Release( lpDD1 );
    lpDD1     = NULL;
  }
  
  lpClipper = NULL;
  lpFront   = NULL;
  lpBack    = NULL;
  lpTriple   = NULL;
  lpAux     = NULL;
  lpDD      = NULL;
  
#endif /* 0 */
  GDBG_INFO(80, "dxClose:  Returning TRUE\n");
  return FXTRUE;
} /* dxClose */

void
dxSwap(FxU32 code)
{
  static RECT    dest, src;
  static POINT   point;
  static HRESULT hr;
  
  GDBG_INFO(80, "dxSwap:\n");
  if ( !IsFullScreen ) {
    GDBG_INFO(120, "Swap: using BLTs\n");

    src.left   = 0;
    src.right  = Width-1;
    src.top    = 0;
    src.bottom = Height-1;
        
    dest = src;
    point.x = 0;
    point.y = 0;
    ClientToScreen( hWndApp, &point );
    dest.left   += point.x;
    dest.right  += point.x;
    dest.top    += point.y;
    dest.bottom += point.y;

    hr = IDirectDrawSurface_Blt( lpFront, 
                                &dest,
                                lpBack,
                                &src,
                                DDBLT_WAIT,
                                NULL );
        
    while( IDirectDrawSurface_GetBltStatus( lpFront, DDGBS_ISBLTDONE ) !=
          DD_OK );
        
    if ( hr != DD_OK ) {
      ErrorMessage(hWndApp, "Couldn't blit!\n");
    }
  }
  return;
  
} /* dxSwap */
#else /* it's either DOS or the Hoopti Lab Setup */

static FxU16 oldVidMode;

FxBool
dxOpen(
    FxU32                hWindow,
    GrScreenResolution_t sRes,
    GrScreenRefresh_t    vRefresh,
    hwcColorFormat       cFormat,
    hwcOriginLocation    yOrigin,
    int                  nColBuffers,
    int                  nAuxBuffers,
    hwcBufferDesc        *pBufDesc,         // return values
    int                  *width,
    int                  *height,
    hwcSwapType          *swapType
)
{
#define FN_NAME "dxOpen"
  FxU32 mode;
  hwcBufferDesc   *dFront  = &pBufDesc[0];
  hwcBufferDesc   *dBack   = &pBufDesc[1];
  hwcBufferDesc   *dAux    = &pBufDesc[2];
  hwcBufferDesc   *dFifo   = &pBufDesc[3];
  hwcBufferDesc   *dScreen = &pBufDesc[4];
  hwcBufferDesc   *dTriple = &pBufDesc[5];
  FxBool rv = FXTRUE;
  FxU32 endAddr;
  union REGS r, rOut;

  r.w.ax = 0x4f03;
  int386(0x10, &r, &rOut);
  
  oldVidMode = rOut.w.bx;
  GDBG_INFO(80, "%s:  Saving Current video mode (0x%x).\n",
    FN_NAME, oldVidMode); 
  
  
  GDBG_INFO(80, "%s:  Resolution == 0x%x\n", FN_NAME, sRes);

  switch (sRes) {
  case GR_RESOLUTION_640x480:
    *width = 640;
    *height = 480;
    mode = 0x111;
    break;

  case GR_RESOLUTION_800x600:
  case GR_RESOLUTION_320x200:
  case GR_RESOLUTION_320x240:
  case GR_RESOLUTION_512x384:
  case GR_RESOLUTION_400x256:
  case GR_RESOLUTION_640x200:
  case GR_RESOLUTION_640x350:
  case GR_RESOLUTION_640x400:
  case GR_RESOLUTION_960x720:
  case GR_RESOLUTION_856x480:
  case GR_RESOLUTION_512x256:
  case GR_RESOLUTION_NONE:
  default:
    GDBG_INFO(80, "%s:  Unimplemented resolution\n", FN_NAME);
    return FXFALSE;
    break;
  }



  r.w.ax = 0x4f02;
  r.w.bx = mode;

  GDBG_INFO(80, "%s: Setting mode 0x%x, 0x%x\n", FN_NAME, r.w.ax, r.w.bx);

  /* Do VGA Magic */
  int386(0x10, &r, &rOut);

  /* AT3D buffers need to be 4K (Intel) page-aligned */
#define BUMP(x, y) (((x) + ((y)-1)) & ~((y)-1))
#define BUMP_PAGE(x) (BUMP(x, 1<<12))

  /* Initialize all descriptors. */
  dFront->bufMagic  = H3_BUF_MAGIC;
  dFront->bufType   = HWC_BUFFER_FRONTBUFFER;
  dFront->bufOffset = 0;
  dFront->bufStride = (*width)*2;
  dFront->bufBPP    = 16;
  endAddr           = BUMP_PAGE((*width)*(*height)*2);

  dBack->bufMagic  = H3_BUF_MAGIC;
  dBack->bufType   = HWC_BUFFER_BACKBUFFER;
  dBack->bufOffset = endAddr;
  dBack->bufStride = (*width)*2;
  dBack->bufBPP    = 16;
  endAddr          = BUMP_PAGE(endAddr + (*width)*(*height)*2);

  tripleBuffering = (nColBuffers == 3);
  if (tripleBuffering) {
    dTriple->bufMagic  = H3_BUF_MAGIC;
    dTriple->bufType   = HWC_BUFFER_TRIPLEBUFFER;
    dTriple->bufOffset = endAddr;
    dTriple->bufStride = (*width)*2;
    dTriple->bufBPP    = 16;
    endAddr          = BUMP_PAGE(endAddr + (*width)*(*height)*2);
  }

  dAux->bufMagic  = H3_BUF_MAGIC;
  dAux->bufType   = HWC_BUFFER_AUXBUFFER;
  dAux->bufOffset = endAddr;
  dAux->bufStride = (*width)*2;
  dAux->bufBPP    = 16;
  endAddr         = BUMP_PAGE(endAddr + (*width)*(*height)*2);

  dFifo->bufMagic  = H3_BUF_MAGIC;
  dFifo->bufType   = HWC_BUFFER_FIFOBUFFER;
  dFifo->bufOffset = endAddr;
  dFifo->bufStride = 65536;
  dFifo->bufBPP    = 16;

  dScreen->bufMagic  = H3_BUF_MAGIC;
  dScreen->bufType   = HWC_BUFFER_SCREENBUFFER;
  dScreen->bufOffset = 0;
  dScreen->bufStride = 0;
  dScreen->bufBPP    = 0;

  GDBG_INFO(80, "%s:  dFront = 0x%x, dBack = 0x%x, dAux = 0x%x\n",
             FN_NAME, dFront, dBack, dAux, dFifo);
  GDBG_INFO(80, "dxOpen: F:%.06x %5d B:%.06x %5d A:%.06x %5d, C:%.06x %5d\n",
       dFront->bufOffset, dFront->bufStride,
       dBack ->bufOffset, dBack ->bufStride,
       dAux  ->bufOffset, dAux  ->bufStride,
       dFifo ->bufOffset, dFifo ->bufStride);

#ifdef GDBG_INFO_ON
  {
    int i;  

    for (i = 0; i < 5; i++) {
      GDBG_INFO(80, "%s:  pBufDesc[%d] = 0x%x:\n",
                 FN_NAME, i, &pBufDesc[i]);
      GDBG_INFO(80, "%s:  pBufDesc.bufType = %s\n",
                 FN_NAME, bufTypeNames[pBufDesc[i].bufType]);
      GDBG_INFO(80, "%s:  pBufDesc.bufOffset = 0x%x\n", 
                 FN_NAME, pBufDesc[i].bufOffset );
      GDBG_INFO(80, "%s:  pBufDesc.bufStride = 0x%x\n", 
                 FN_NAME, pBufDesc[i].bufStride);
      GDBG_INFO(80, "%s:  pBufDesc.bufBPP = 0x%x\n",
                 FN_NAME, pBufDesc[i].bufBPP);
    }
  }
#endif

  *swapType = HWC_SWAP_FLIP;

BAIL:
  return rv;
#undef FN_NAME
} /* dxOpen */

FxBool
dxControl(FxU32 code, hwcBufferDesc *pBufDesc, int *width, int *height)
{
  return FXTRUE;
} /* dxControl */
FxBool
dxClose()
{
#define FN_NAME "dxClose"
  GDBG_INFO(80, "%s:  Setting up VESA mode 640*480*\n", FN_NAME);
  {
    union REGS r;

    memset(&r, 0, sizeof(r));

    r.w.ax = 0x4f02;
    r.w.bx = oldVidMode;
    GDBG_INFO(80, "%s: Setting mode 0x%x, 0x%x\n", FN_NAME, r.w.ax, r.w.bx);
    int386( 0x10, &r, &r );
  }
  return FXTRUE;
#undef FN_NAME
} /* dxClose */

void
dxSwap(FxU32 code)
{
} /* dxSwap */
#endif  /* __DOS32__ || (defined(__WIN32__)  && !defined(HWC_ACCESS_DDRAW) */


