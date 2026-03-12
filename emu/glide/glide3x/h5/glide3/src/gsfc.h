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
** $Log: 
**  4    3dfx      1.1.1.0.1.0 10/11/00 Brent           Forced check in to enforce
**       branching.
**  3    3dfx      1.1.1.0     06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  2    3dfx      1.1         11/08/99 Kenneth Dyke    New callback prototype for
**       MacOS.
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 10    8/23/99 2:13p Kcd
** Mac additions.
** 
** 9     8/17/99 5:10p Kcd
** Updated Mac surface stuff to newer API.
** 
** 8     6/24/99 7:10p Atai
** new grSurfaceSetRenderingSurface interface
** 
** 7     4/16/99 2:56p Kcd
** MacOS Surface extensions.
** 
** 6     3/14/99 1:46p Peter
** temp implemntation of my surface extension extension
** 
** 5     12/14/98 6:19p Dow
** Fixed for current surface extension spec
** 
** 4     8/02/98 5:01p Dow
** Glide Surface Extension
** 
** 3     7/13/98 9:57p Jdt
** Added to build.  First implementation of SetRenderSurface and
** CreateContext
** 
** 2     7/09/98 6:47p Dow
** 
** 1     7/09/98 11:37a Dow
** Initial Checkin
**
*/
#ifndef GSFC_H
#define GSFC_H

#include <3dfx.h>
#include <glide.h>

#define GR_SURFACE_EXTENSION            0x1000

typedef FxU32 GrSurfaceContextType_t;
#define GR_SURFACECONTEXT_WINDOWED      0
#define GR_SURFACECONTEXT_FULLSCREEN    1

typedef void * GrSurface_t;

typedef FxU32 GrSurfaceTexType_t;
#define GR_SURFACETEXTYPE_FB            0
#define GR_SURFACETEXTYPE_AGP           1

/* New Enumerants for GR_GET */
#define GR_SURFACE_SIZE                 (GR_SURFACE_EXTENSION | 0x1)
#define GR_SURFACE_TEXTURE              (GR_SURFACE_EXTENSION | 0x2)

typedef struct {
  FxU32 glideDeviceId;
  void* systemDeviceId;

  FxU32 reserved;
} GrDeviceInfo_t;

extern GrContext_t FX_CALL
grSurfaceCreateContext(GrSurfaceContextType_t t);

extern void FX_CALL
grSurfaceReleaseContext(GrContext_t ctx);

extern void FX_CALL
grSurfaceSetRenderingSurface(GrSurface_t sfc, FxBool textureP);

extern void FX_CALL
grSurfaceSetAuxSurface(GrSurface_t sfc);

extern void FX_CALL
grSurfaceSetTextureSurface(GrChipID_t tmu, GrSurface_t sfc);

extern FxBool FX_CALL
grSurfaceCalcTextureWHD(GrTexInfo *tInfo, FxU32 *w, FxU32 *h, FxU32 *d);

/* Returns the # of glide compatible devices in the system. If devList
 * is non-NULL and listCount is non-zero, devList is filled in w/
 * information about the current glide device to system device
 * mapping. If there is not enough space in the client passed list to
 * fill in all of the device information a partial list is returned to
 * the client along w/ the full count.  
 */
extern FxU32 FX_CALL
grDeviceQuery(GrDeviceInfo_t devList[], FxU32 listCount);

#if (GLIDE_PLATFORM & GLIDE_OS_MACOS)

/* MacOS Glide Surface Extensions */
#define GR_SURFACE_NOTIFY_LOST            0x01

typedef struct {
  FxU32 surface;
  FxU32 width;
  FxU32 height;
  FxU32 pitch;
  FxU32 bytesPerPixel;
  FxU32 pixelFormat;
  void  (*notifyCallback)(GrSurface_t sfc, void *userData, FxU32 code);
  void  *userData;             /* Anything you want */
  
  /* OS Specific Info */
  void  *systemPortId;         /* Drawing port type thing (read-only) */
  void  *systemDeviceId;       /* Same as systemDeviceId from GrDeviceInfo_t (ready-only) */
  
  /* For future expansion */
  FxU32 reserved;
} GrSurfaceDesc_t;

extern GrSurface_t FX_CALL 
grSurfaceCreate(GrSurfaceDesc_t *desc);

extern void FX_CALL 
grSurfaceRelease(GrSurface_t sfc);

extern void FX_CALL
grSurfaceGetDesc(GrSurface_t sfc, GrSurfaceDesc_t *sfcDesc);

#endif

#endif /* GSFC_H */
