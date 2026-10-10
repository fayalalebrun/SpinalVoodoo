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
** 3     3/17/99 6:16p Dow
** Phantom Menace and other fixes.
** 
** 1     2/19/99 5:50p Peter
** type definitions for new splash screen
*/

#ifndef _FX_SPLASH_H_
#define _FX_SPLASH_H_

#include "3dfx.h"
#define FX_DLL_DEFINITION
#include "fxdll.h"
#include "glide.h"

typedef FxBool 
(FX_CALL* GrSplashInitProc)(FxU32 hWnd,
                            FxU32 screenWidth, FxU32 screenHeight,
                            FxU32 numColBuf, FxU32 numAuxBuf,
                            GrColorFormat_t colorFormat);

typedef void
(FX_CALL* GrSplashShutdownProc)(void);

typedef void 
(FX_CALL* GrSplashProc)(float x, float y,
                        float w, float h,
                        FxU32 frameNumber);

typedef const void*
(FX_CALL* GrSplashPlugProc)(FxU32* w, FxU32* h,
                            FxI32* strideInBytes,
                            GrLfbWriteMode_t* format);

#endif /* _FX_SPLASH_H_ */
