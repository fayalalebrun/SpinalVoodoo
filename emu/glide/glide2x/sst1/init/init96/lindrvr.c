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
**
*/

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#ifndef DUMMY
#include <X11/extensions/xf86rush.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

/* 3Dfx Headers */
#include <3dfx.h>
#include <fxpci.h>
#include <gdebug.h>
#include <sst.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <string.h>

#include <init.h>

/* Local Headers */
#include "init96.h"
#include "fxinit96.h"

#define VG96_BUF_MAGIC  0x666
#define FIFOSIZE (65536<<1)

FxBool tripleBuffering = 0;

#define NUM_BUFS 6

#ifndef DUMMY
static Display		    *dpy	= 0;
static int		    screenNum   = 0;
static int		    screenWidth = 0;
static void*		    screenPhys  = 0;
static int		    screenMem   = 0;
static GC		    xgc		= 0;
static Window               hWndApp     = 0;
static FxU32                Width       = 0; 
static FxU32                Height      = 0;
static FxBool               IsFullScreen = 0;
static InitBufDesc_t        BufDesc[NUM_BUFS]  = { {0,0,0,0,0} };
static XF86VidModeModeInfo  **vidModes = 0;

static Pixmap  pixBack      = 0;
static Pixmap  pixAux       = 0;
static Pixmap  pixTriple    = 0;
static Pixmap  pixFifo      = 0;
static void *addrFront = 0;
static void *addrBack = 0;
static void *addrAux = 0;
static void *addrTriple = 0;
static void *addrFifo = 0;
#endif

#ifdef DUMMY
FxBool
linOpen(
    FxU32                hWindow,
    GrScreenResolution_t sRes,
    GrScreenRefresh_t    vRefresh,
    InitColorFormat_t    cFormat,
    InitOriginLocation_t yOrigin,
    int                  nColBuffers,
    int                  nAuxBuffers,
    InitBufDesc_t        *pBufDesc,         // return values
    int                  *width,
    int                  *height,
    InitSwapType_t       *swapType
)
{
  return FXFALSE;
}

FxBool linClose()
{
  return FXFALSE;
}

FxBool
linControl(FxU32 code, InitBufDesc_t *pBufDesc, int *width, int *height)
{
  return FXFALSE;
}

void
linSwap(FxU32 code)
{}

void
linFlip(FxU32 buffer)
{}

#else
void linExit(void);

static FxBool
ErrorMessage(Window win, const char *err)
{
  fprintf(stderr, "Error %s \n", err);
  return FXFALSE;
} /* ErrorMessage */

FxBool linSetup(void) {
  int eventbase, errorbase;
  int banksize;
  unsigned int baseaddr;

  if (dpy!=0) return FXTRUE;
  dpy=XOpenDisplay("");
  if (!dpy)
    return ErrorMessage(hWndApp, "Linux VRush only runs under local X\n");
  if (XF86DGAQueryExtension(dpy, &eventbase, &errorbase)) {
    int major, minor;
    XF86DGAQueryVersion(dpy, &major, &minor);
    if (major>1 && minor<0) {
      XCloseDisplay(dpy);
      return ErrorMessage(hWndApp, "Linux VRush requires DGA version 1.0\n");
    }
  }
  if (XF86RushQueryExtension(dpy, &eventbase, &errorbase)) {
    int major, minor;
    XF86RushQueryVersion(dpy, &major, &minor);
    if (major>1 && minor<0) {
      XCloseDisplay(dpy);
      return ErrorMessage(hWndApp, "Linux VRush requires Rush version 1.0\n");
    }
  }
  screenNum=XDefaultScreen(dpy);
  XF86DGAGetVideoLL(dpy, screenNum, &baseaddr, &screenWidth, &banksize,
		    &screenMem);
  screenPhys = (void *)baseaddr;
  screenMem *= 1024;
  return FXTRUE;
}

static FxBool
cvtAddrToBufDesc(void *pixAddr, InitBufDesc_t *pDesc) {
  pDesc->bufOffset = (FxU32)pixAddr;
  pDesc->bufStride = (FxI32)(2*screenWidth);
  pDesc->bufBPP    = (FxI32)16;
  return FXTRUE;
}

static int 
cvtXToGlideDesc(InitBufDesc_t *pDesc) {
  InitBufDesc_t   *dFront  = &pDesc[0];
  InitBufDesc_t   *dBack   = &pDesc[1];
  InitBufDesc_t   *dAux    = &pDesc[2];
  InitBufDesc_t   *dFifo   = &pDesc[3];
  InitBufDesc_t   *dScreen = &pDesc[4];
  InitBufDesc_t   *dTriple = &pDesc[5];

  dScreen->bufMagic = VG96_BUF_MAGIC;
  dScreen->bufType  = INIT_BUFFER_SCREENBUFFER;
  dScreen->bufOffset = 0;
  dScreen->bufStride = 0;
  dScreen->bufBPP    = 0;

  *dFront = *dBack = *dAux = *dFifo = *dTriple = *dScreen;

  /* Get info about screen (framebuffer) */
  if (!cvtAddrToBufDesc(screenPhys, dScreen))
    return ErrorMessage(hWndApp, "Couldn't get Screen Info");

  /* Get info about the front buffer (window) */
  dFront->bufType = INIT_BUFFER_FRONTBUFFER;
  if (!cvtAddrToBufDesc(addrFront, dFront))
    return ErrorMessage(hWndApp, "Couldn't get Frontbuffer Info");
  dFront->bufOffset -= dScreen->bufOffset;

  /* Get info about back buffer (pixmap) */
  dBack->bufType  = INIT_BUFFER_BACKBUFFER;
  if (!cvtAddrToBufDesc(addrBack, dBack))
    return ErrorMessage(hWndApp, "Couldn't get Backbuffer Info");
  dBack->bufOffset -= dScreen->bufOffset;
  
  /* Get info about aux buffer (pixmap) */
  dAux->bufType = INIT_BUFFER_AUXBUFFER;
  if (!cvtAddrToBufDesc(addrAux, dAux)) 
    return ErrorMessage(hWndApp, "Couldn't get AuxBuffer Info");
  dAux->bufOffset -= dScreen->bufOffset;
  
  /* Get info about triple buffer (pixmap if used) */
  if (tripleBuffering) {
    dTriple->bufType  = INIT_BUFFER_TRIPLEBUFFER;
    if (!cvtAddrToBufDesc(addrTriple, dTriple))
      return ErrorMessage(hWndApp, "Couldn't get triple buffer Info");
    dTriple->bufOffset -= dScreen->bufOffset;
  }

  /* Get info about cmdFifo (pixmap) */
  dFifo->bufType   = INIT_BUFFER_FIFOBUFFER;
  dFifo->bufOffset = (int)addrFifo-dScreen->bufOffset;
  dFifo->bufStride = FIFOSIZE;

  GDBG_INFO((80, "cvtXToGlideDesc: dFront->bufOffset = 0x%x\n", dFront->bufOffset));
  GDBG_INFO((80, "cvtXToGlideDesc: dBack->bufOffset = 0x%x\n", dBack->bufOffset));
  GDBG_INFO((80, "cvtXToGlideDesc: dAux->bufOffset = 0x%x\n", dAux->bufOffset));
  GDBG_INFO((80, "cvtXToGlideDesc: dFifo->bufOffset = 0x%x\n", dFifo->bufOffset));
  if (tripleBuffering) {
    GDBG_INFO((80, "cvtXToGlideDesc: dTriple->bufOffset = 0x%x\n", dTriple->bufOffset));
  }

  GDBG_INFO((80,
    "F:%.06x %5d B:%.06x %5d B2:%.06x %5d A:%.06x %5d, C:%.06x %5d\n",
             dFront->bufOffset, dFront->bufStride,
             dBack ->bufOffset, dBack ->bufStride,
             dTriple->bufOffset, dTriple->bufStride,
             dAux  ->bufOffset, dAux  ->bufStride,
             dFifo ->bufOffset, dFifo ->bufStride));
  return FXTRUE;
}

static void
doReleasePixmaps() {
  if (!dpy) return;
  XF86RushUnlockAllPixmaps(dpy);
  if (pixAux) XFreePixmap(dpy, pixAux);
  if (pixBack) XFreePixmap(dpy, pixBack);
  if (pixTriple) XFreePixmap(dpy, pixTriple);
  if (pixFifo) XFreePixmap(dpy, pixFifo);
  pixAux=pixBack=pixTriple=pixFifo=0;
}

static void
findWindow(Display *dpy, Window hWndApp, void **addrFront) {
  XWindowAttributes attr;
  int x=0, y=0;
  unsigned int num;
  Window root=-1, parent, *children;

  do {
    XQueryTree(dpy, hWndApp, &root, &parent, &children, &num);
    /* Find out how big the window is */
    if (!XGetWindowAttributes(dpy, hWndApp, &attr)) {
      ErrorMessage(hWndApp, "Failed to get window attributes\n");
      return;
    }
    x+=attr.x;
    y+=attr.y;
    if (hWndApp!=root) hWndApp=parent;
  } while (hWndApp!=root);
  *addrFront=screenPhys+(y*screenWidth+x)*2;
}

static int
doAllocPixmaps(int xRes, int yRes, int vRefresh, 
		      InitBufDesc_t *pDesc) {
    GDBG_INFO((80, "doAllocPixmaps: hWnd = %x, fs=%d, xRes=%d, yRes=%d, vRefresh=%d\n", 
             hWndApp, IsFullScreen, xRes, yRes, vRefresh));

  doReleasePixmaps();

  pixFifo=XCreatePixmap(dpy, hWndApp, screenWidth,
			(FIFOSIZE/2+4096+screenWidth-1)/screenWidth, 16);
  if ((XF86RushLockPixmap(dpy, screenNum, pixFifo, &addrFifo)==FXFALSE)
      || !addrFifo) {
    doReleasePixmaps();
    return ErrorMessage(hWndApp, "Failed to lock fifo pixmap\n");
  }
  xRes = (xRes+3)&~3;
  yRes = (yRes+3)&~3;

  findWindow(dpy, hWndApp, &addrFront);
  pixBack=XCreatePixmap(dpy, hWndApp, xRes, yRes, 16);
  if ((XF86RushLockPixmap(dpy, screenNum, pixBack,
			  &addrBack)==FXFALSE) || !addrBack) {
    doReleasePixmaps();
    return ErrorMessage(hWndApp, "Failed to lock back pixmap\n");
  }
  pixAux=XCreatePixmap(dpy, hWndApp, xRes, yRes, 16);
  if ((XF86RushLockPixmap(dpy, screenNum, pixAux,
			  &addrAux)==FXFALSE) || !addrAux) {
    doReleasePixmaps();
    return ErrorMessage(hWndApp, "Failed to lock aux pixmap\n");
  }
  if (tripleBuffering) {
    pixTriple=XCreatePixmap(dpy, hWndApp, xRes, yRes, 16);
    if ((XF86RushLockPixmap(dpy, screenNum, pixTriple,
			    &addrTriple)==FXFALSE) || !addrTriple) {
      doReleasePixmaps();
      return ErrorMessage(hWndApp, "Failed to lock triple pixmap\n");
    }
  }
  cvtXToGlideDesc(pDesc);
  return FXTRUE;
}

static FxBool 
getWindowSize(unsigned int *width, unsigned int *height) {
    XWindowAttributes attr;

    /* Find out how big the window is */
    if (!XGetWindowAttributes(dpy, hWndApp, &attr))
      return ErrorMessage(hWndApp, "Failed to get window attributes\n");

    *width=attr.width;
    *height=attr.height;
    return FXTRUE;
}

FxBool
linOpen(
    FxU32                hWindow,
    GrScreenResolution_t sRes,
    GrScreenRefresh_t    vRefresh,
    InitColorFormat_t    cFormat,
    InitOriginLocation_t yOrigin,
    int                  nColBuffers,
    int                  nAuxBuffers,
    InitBufDesc_t        *pBufDesc,         // return values
    int                  *width,
    int                  *height,
    InitSwapType_t       *swapType
)
{
  int i, flags, numModes;
  XWindowAttributes attr;

  GDBG_INFO((80, "linOpen: hWnd = %x, sRes=%d, yOrg=%d, ncb=%d, nab=%d\n",
             hWindow, sRes, yOrigin, nColBuffers, nAuxBuffers));

  if (!linSetup()) return FXFALSE;
  atexit(linExit);

  if (sRes == GR_RESOLUTION_NONE ) { /* In a window */

    /*
     * Get ourselves a valid window handle 
     */
    hWndApp = (Window)hWindow;
    if (hWndApp == 0)
      return ErrorMessage(hWndApp, "Linux VRush needs a valid window id!\n");
    xgc=XCreateGC(dpy, hWndApp, 0, 0);
    XSetFunction(dpy, xgc, GXcopy);
    XSetPlaneMask(dpy, xgc, AllPlanes);

    if (!XGetWindowAttributes(dpy, hWndApp, &attr))
      return ErrorMessage(hWndApp, "Failed to get window attributes\n");
    if (attr.depth!=16)
      return ErrorMessage(hWndApp, "Screen depth must be 16\n");
    screenNum=XScreenNumberOfScreen(attr.screen);
    Width=attr.width;
    Height=attr.height;
    IsFullScreen = FXFALSE;
  } else {
    /* Full screen mode - convert resolution to Width, Height */
    static int _w[] = {320,320,400,512,640,640,640,640,800,960,856,512};
    static int _h[] = {200,240,256,384,200,350,400,480,600,720,480,256};

    if ((sRes <  GR_RESOLUTION_320x200) ||
        (sRes >  GR_RESOLUTION_512x256)) 
      return ErrorMessage(hWndApp, "Bad Fullscreen resolution");

    tripleBuffering = (nColBuffers == 3);

    IsFullScreen = FXTRUE;
    Width   = _w[sRes];
    Height  = _h[sRes];

    hWndApp=DefaultRootWindow(dpy);
    if (!XGetWindowAttributes(dpy, hWndApp, &attr))
      return ErrorMessage(hWndApp, "Failed to get window attributes\n");
    if (attr.depth!=16)
      return ErrorMessage(hWndApp, "Screen depth must be 16\n");
    if (XF86DGAQueryDirectVideo(dpy, DefaultScreen(dpy), &flags)==False ||
	!flags) {
      GDBG_INFO((0, "Could query Direct Video"));
      return ErrorMessage(hWndApp, "Direct video not supported\n");
    }
    if (XF86VidModeGetAllModeLines(dpy, DefaultScreen(dpy), &numModes,
				   &vidModes)==False) {
      return ErrorMessage(hWndApp, "Couldn't query vidmode list\n");
      return 1;
    }
    for (i=0; i<numModes; i++)
      if ((vidModes[i]->hdisplay==Width) && (vidModes[i]->vdisplay==Height))
	break;
    if (i==numModes) {
      return ErrorMessage(hWndApp, "Server doesn't support requested resolution\n");
      if (vidModes) {
	XFree(vidModes);
	vidModes=0;
      }
      return 1;
    }
    if (XF86VidModeSwitchToMode(dpy, DefaultScreen(dpy), vidModes[i])==False) {
      return ErrorMessage(hWndApp, "Failed to set required video mode\n");
      return 1;
    }
  }

  GDBG_INFO((80, "linOpen: W=%d, H=%d, FullScr=%d, vRefresh=%d\n",
             Width, Height, IsFullScreen, vRefresh));

  if (!doAllocPixmaps(Width, Height, vRefresh, BufDesc)) {
    doReleasePixmaps();
    return ErrorMessage(hWndApp, "failed linOpen");
  }
  GDBG_INFO((80, "doAllocPixmaps OK!\n"));
  for (i=0; i< NUM_BUFS; i++) {
    pBufDesc[i] = BufDesc[i];
    GDBG_INFO((80, "linOpen: pBufDesc[%d]: \n", i));
    GDBG_INFO((80, "\tbufMagic = 0x%x\n", pBufDesc[i].bufMagic));
    GDBG_INFO((80, "\tbufType = 0x%x\n", pBufDesc[i].bufType));
    GDBG_INFO((80, "\tbufOffset = 0x%x\n", pBufDesc[i].bufOffset));
    GDBG_INFO((80, "\tbufStride = 0x%x\n", pBufDesc[i].bufStride));    
    GDBG_INFO((80, "\tbufBPP = 0x%x\n", pBufDesc[i].bufBPP));
  }

  *width = Width;
  *height = Height;
  *swapType = (IsFullScreen) ? INIT_SWAP_FLIP : INIT_SWAP_BLT;

  if (IsFullScreen) {
    if (XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectGraphics)==False) {
      GDBG_INFO((0, "Couldn't set direct graphics mode"));
      return ErrorMessage(hWndApp, "Couldn't enable direct video\n");
    }
    if (XF86DGASetViewPort(dpy, DefaultScreen(dpy), 0, 0)==False) {
      GDBG_INFO((0, "Failed to set viewport\n"));
    }
    XSync(dpy, False);
    GDBG_INFO((80, "Fullscreen mode switched\n"));
  }

  if ( curHALData->initSetVideo ) {
    curHALData->initSetVideo( sRes, vRefresh, 
                             (FxU32*)width, (FxU32*)height );
  }

  GDBG_INFO((80, "linOpen:  Returning TRUE\n"));
  return FXTRUE;

} /* linOpen */

FxBool
linControl(FxU32 code, InitBufDesc_t *pBufDesc, int *width, int *height)
{
  unsigned int i, w = 0, h = 0;

  GDBG_INFO((80, "linControl: code = %d\n", code));

  if (IsFullScreen) return FXTRUE;
  /* Why was I called? */
  switch(code) {
  case INIT_CONTROL_RESIZE:     /* recreate surfaces */
    getWindowSize(&w, &h);
    if ((Width != w) || (Height != h)) {
      GDBG_INFO((120, "W and H changed to %d %d\n", 
                 Width, Height)); 

      if (!doAllocPixmaps(w, h, 0, BufDesc)) {
	w=Width;
	h=Height;
	if (!doAllocPixmaps(w, h, 0, BufDesc)) {
	  /* ErrorMessage(hWndApp, "linControl: Resize failed\n"); */
	  return FXFALSE;
	}
      }
      Width = w;
      Height = h;
    }
    break;

  case INIT_CONTROL_MOVE:
    findWindow(dpy, hWndApp, &addrFront);
    cvtAddrToBufDesc(addrFront, &BufDesc[0]);
    GDBG_INFO((120, "linControl: Move\n"));
    break;

  case INIT_CONTROL_ACTIVATE:
    GDBG_INFO((120, "linControl: Activate\n"));
    break;

  case INIT_CONTROL_DEACTIVATE:
    GDBG_INFO((120, "linControl: DeActivate\n"));
    break;

  default:
    GDBG_INFO((120, "linControl: Strange control %d\n", code));
    return FXFALSE;
    break;
  }
  for (i=0; i<5; i++) pBufDesc[i] = BufDesc[i];
  *width = Width;
  *height = Height;
  GDBG_INFO((80, "linControl: code = %d, w = %d, h = %d\n", code,
             Width, Height));
  
  return FXTRUE;
} /* linControl */

FxBool
linClose(void)
{
  GDBG_INFO((80, "linClose:\n"));
  if (!dpy) return FXTRUE;
  if (IsFullScreen) {
    if (XF86VidModeSwitchToMode(dpy, DefaultScreen(dpy), vidModes[0])==False) {
      GDBG_INFO((80, "Failed to return to previous video mode\n"));
    }
    XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
    if (vidModes) {
      XFree(vidModes);
      vidModes=0;
    }
  }
  doReleasePixmaps();
  if (xgc) XFreeGC(dpy, xgc);
  xgc=0;
  dpy=0;
  GDBG_INFO((80, "linClose:  Returning TRUE\n"));
  return FXTRUE;
} /* linClose */

void
linExit(void)
{
  linClose();
} /* linExit */

void
linSwap(FxU32 code)
{
  if (IsFullScreen) return;
  XCopyArea(dpy, pixBack, hWndApp, xgc, 0, 0, Width, Height, 0, 0);
  XSync(dpy, False);
}  /* linSwap */

void
linFlip(FxU32 buffer)
{
  int x, y;

  y=buffer/(2*screenWidth);
  x=(buffer-y*2*screenWidth)/2;
  if (XF86DGASetViewPort(dpy, DefaultScreen(dpy), x, y)==False) {
    GDBG_INFO((0, "Failed to set viewport\n"));
  }
  /*
    We really are supposed to wait for the port changed call in a while
    loop I guess, but at the moment, this is a no-op in the
    server. Since this requires and extra round-trip to the X server it
    really eats into performance. I'd argue DGA is broken. Setting the
    viewport should enforce the switch before returning to avoid this.
    */
#if 0
  if (XF86DGAViewPortChanged(dpy, DefaultScreen(dpy), 2)==False) {
    GDBG_INFO((0, "Failed to wait for viewport change\n"));
  }
#endif
} /* linFlip */

#endif
