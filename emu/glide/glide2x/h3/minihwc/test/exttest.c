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
** 1     5/01/98 9:16a Dow
**
*/

#include <windows.h>
#include <3dfx.h>
#include "..\hwcext.h"
/* This segment simulates main() for Windows, creates a window, etc. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

/* Forward declarations */
HWND hWndMain;

/*
 * MainWndproc
 *
 * Callback for all Windows messages
 */
static int qhead = 0;
static int qtail = 0;
static int queue[256] = {0};

long FAR PASCAL 
MainWndproc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  PAINTSTRUCT ps;
  HDC         hdc;
  
  switch( message )
    {
    case WM_SETCURSOR:
      SetCursor(NULL);
      break;

    case WM_CREATE:
      break;

    case WM_PAINT:
      hdc = BeginPaint( hWnd, &ps );
      EndPaint( hWnd, &ps );
      return 1;

    case WM_CLOSE:
      queue[qhead++] = 'q'; qhead &= 255;
      break;

    case WM_DESTROY:
      break;

    case WM_MOVE:
      break;

    case WM_DISPLAYCHANGE:
    case WM_SIZE:
      break;

    case WM_CHAR:
      break;

    default:
      break;
    }
  return DefWindowProc(hWnd, message, wParam, lParam);
  
} /* MainWndproc */

void
main(int argc, char **argv)
{
  HWND hWndMain;
  HANDLE hInstance = GetModuleHandle(NULL);
  HDC dc;
  int extRes;
  hwcExtRequest_t req;
  hwcExtResult_t res;
  WNDCLASS    wc;
  BOOL        rc;

  FxU32 contextID;

  
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = MainWndproc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon( NULL, IDI_APPLICATION);    /* generic icon */
  wc.hCursor = LoadCursor( NULL, IDC_ARROW );
  wc.hbrBackground = GetStockObject( BLACK_BRUSH );
  wc.lpszMenuName =  NULL;
  wc.lpszClassName = "ExtTestClass";
  rc = RegisterClass( &wc );
  
  if( !rc ) {
    printf("Failed to register class!\n");
    exit(1);
  }
  
  hWndMain =
    CreateWindowEx(
#if 0                         
      WS_EX_APPWINDOW  gives you regular borders?
        WS_EX_TOPMOST    Works as advertised.
#endif
        WS_EX_APPWINDOW,
        "ExtTestClass",
        "ExtEscape Test",
        WS_OVERLAPPED |     
        WS_CAPTION  |     
        WS_THICKFRAME | 
        WS_MAXIMIZEBOX | 
        WS_MINIMIZEBOX | 
        WS_POPUP | /* non-app window */
        WS_SYSMENU, /* so we get an icon in the tray */
        CW_USEDEFAULT, 
        CW_USEDEFAULT,
        0x110, /* GetSystemMetrics(SM_CXSCREEN), */
        0x120, /* GetSystemMetrics(SM_CYSCREEN), */
        NULL,
        NULL,
        hInstance,
        NULL
    );
  
  
  if( !hWndMain ) {
    printf("Failed to create window\n");
    exit(2);
  }

  /* HWCEXT_GETDRIVERVERSION */

  req.which = HWCEXT_GETDRIVERVERSION;

  dc = GetDC(hWndMain);

  extRes = ExtEscape(dc, EXT_HWC, sizeof(req), (LPCSTR) &req,
    sizeof(res), (LPCSTR) &res); 
    
  if (extRes == 0) {
    printf("No escape!\n");
    exit(3);
  }

  printf("HWCEXT_GETDRIVERVERSION:\n\tMajor:\t0x%x\n\tMinor:\t0x%x\n",
    res.optData.driverVersionRes.major,
    res.optData.driverVersionRes.minor);
  
  /* HWCEXT_ALLOCCONTEXT */
  req.which = HWCEXT_ALLOCCONTEXT;

  req.optData.allocContextReq.protocolRev = HWCEXT_PROTOCOLREV;
  req.optData.allocContextReq.appType = HWCEXT_ABAPPTYPE_FSEM;

  extRes = ExtEscape(dc, EXT_HWC, sizeof(req), (LPCSTR) &req,
    sizeof(res), (LPCSTR) &res); 

  printf("HWCEXT_ALLOCCONTEXT:\n\tStatus:\t0x%x\n\tContext ID:\t0x%x\n",
    res.resStatus, res.optData.allocContextRes.contextID);
  
  contextID = res.optData.allocContextRes.contextID;

  /* HWCEXT_GETDEVICECONFIG */
  req.which = HWCEXT_GETDEVICECONFIG;

  extRes = ExtEscape(dc, EXT_HWC, sizeof(req), (LPCSTR) &req,
    sizeof(res), (LPCSTR) &res); 

  printf("HWCEXT_GETDEVICECONFIG:\
\n\tStatus:\t0x%x\
\n\tdevNum:\t0x%x\
\n\tvendorDI:\t0x%x\
\n\tfbRam:\t0x%x\
\n\tchipRev:\t0x%x\n",
    res.resStatus, res.optData.deviceConfigRes.devNum,
    res.optData.deviceConfigRes.vendorID,
    res.optData.deviceConfigRes.fbRam,
    res.optData.deviceConfigRes.chipRev);

  /* HWCEXT_GETLINEARADDR */
  req.which = HWCEXT_GETLINEARADDR;

  req.optData.linearAddrReq.devNum = 0;

  extRes = ExtEscape(dc, EXT_HWC, sizeof(req), (LPCSTR) &req,
    sizeof(res), (LPCSTR) &res); 

  printf("HWCEXT_LINEARADDRESS:\
\n\tStatus:\t0x%x\
\n\tnumBaseAddrs:       %d\
\n\tbaseAddresses[0]:   0x%x\
\n\tbaseAddresses[1]:   0x%x\
\n\tbaseAddresses[2]:   0x%x\n",
    res.resStatus, res.optData.linearAddressRes.numBaseAddrs,
    res.optData.linearAddressRes.baseAddresses[0],
    res.optData.linearAddressRes.baseAddresses[1],
    res.optData.linearAddressRes.baseAddresses[2]
    );

  /* HWCEXT_RELEASECONTEXT */
  req.which = HWCEXT_RELEASECONTEXT;

  req.optData.releaseContextReq.contextID = contextID;

  extRes = ExtEscape(dc, EXT_HWC, sizeof(req), (LPCSTR) &req,
    sizeof(res), (LPCSTR) &res); 

  printf("HWCEXT_RELEASECONTEXT:\n\tStatus:\t0x%x\n", res.resStatus);  

} /* main */
