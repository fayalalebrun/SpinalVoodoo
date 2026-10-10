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

#ifndef _FXINIT96_H_
#define _FXINIT96_H_

#include <3dfx.h>
#include <sst.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __linux__
FxBool dxOpen(
        FxU32                   hWindow,
        GrScreenResolution_t    sRes,
        GrScreenRefresh_t       vRefresh,
        InitColorFormat_t       cFormat,
        InitOriginLocation_t    yOrigin,
        int                     nCol,
        int                     nAux,
        InitBufDesc_t           *pBufDesc,
        int                     *width,
        int                     *height,
        InitSwapType_t          *swapType
);
FxBool dxControl(FxU32 control, InitBufDesc_t *pDesc, int *w, int *h);
FxBool dxClose(void);
void   dxSwap(FxU32 code);

#else

FxBool linOpen(
        FxU32                   hWindow,
        GrScreenResolution_t    sRes,
        GrScreenRefresh_t       vRefresh,
        InitColorFormat_t       cFormat,
        InitOriginLocation_t    yOrigin,
        int                     nCol,
        int                     nAux,
        InitBufDesc_t           *pBufDesc,
        int                     *width,
        int                     *height,
        InitSwapType_t          *swapType
);
FxBool linControl(FxU32 control, InitBufDesc_t *pDesc, int *w, int *h);
FxBool linClose(void);
void   linSwap(FxU32 code);
void   linFlip(FxU32 buffer);

#endif

typedef struct {
  FxU32  memSize;
  FxBool (FX_CSTYLE *initAperture) (InitRegisterDesc *rd);
  FxBool (FX_CSTYLE *initSetVideo)( GrScreenResolution_t res, GrScreenRefresh_t refresh, FxU32 *xr, FxU32 *yr );
  FxBool (FX_CSTYLE *initEnableRegs)(InitRegisterDesc*rd);
  FxBool (FX_CSTYLE *initDisableRegs)(InitRegisterDesc*rd);
  FxBool (FX_CSTYLE *wrapFIFO) (InitRegisterDesc *rd, InitFIFOData *fifoData );
#ifdef H3D
  int    (FX_CSTYLE *initUseTiles) (InitRegisterDesc *rd, FxU32 *buffers, FxU32 nBuffers, FxU32 stride, FxU32 Width, FxU32 Height, FxU32 AutoFlipped);
#else
  int    (FX_CSTYLE *initUseTiles) (InitRegisterDesc *rd, FxU32 *buffers, FxU32 nBuffers, FxU32 stride, FxU32 Width, FxU32 Height);
#endif
  FxBool (FX_CSTYLE *initGetInfo)(VG96Info *info);
  FxBool (FX_CSTYLE *initSwapTiles) (InitRegisterDesc *rd);
  void *devPriv;                /* Just in case */
} Init96HALData;

extern Init96HALData *curHALData;

extern const char *iRegNames[];

extern volatile FxU32   *sstHW;
extern volatile Sstregs *sstPtr;

/*----------------- SST96 access macros----------------------*/

/* NOTE:

   This code is HIGHLY important to debugging.  Delete it at your
   extreme peril.
 */
#define GETINDEX(a) ((((FxU32) &a) - (((FxU32) sstHW) + 0x400000)) >> 2)
/* This is the ONE TRUE WAY to read FBI Jr. registers.  Don't mess
   with it!!! */
#define GETREADADDR(s) ((FxU32 *) (((FxU32) sstHW) + 0x400000 + (GETINDEX(s) << 12)))
#define GETINDEXFROMADDR(a) ((((FxU32) a) - (((FxU32) sstHW + 0x400000))) >> 2)
#define GETREADADDR2(s) ((FxU32 *) (((FxU32) sstHW) + 0x400000 + (GETINDEXFROMADDR(s) << 12)))

#undef SET
#undef GET
#undef SETF
#undef GETF

#ifdef GDBG_INFO_ON
#define SET(d,s)\
GDBG_INFO((80, "SET:  Register:  %s (0x%x) = 0x%x\n",\
           iRegNames[GETINDEX(d)], GETINDEX(d), s));\
(d) = (s);
#define GET(s) get96(&s)

#else
#define SET(d,s) (d) = (s)
#define GET(s) *(GETREADADDR(s))
#endif

#ifdef __cplusplus
}
#endif
#endif
