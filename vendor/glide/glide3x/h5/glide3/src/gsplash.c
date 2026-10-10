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
 **  3    3dfx      1.0.1.0.1.0 10/11/00 Brent           Forced check in to enforce
 **       branching.
 **  2    3dfx      1.0.1.0     06/20/00 Joseph Kain     Changes to support the
 **       Napalm Glide open source release.  Changes include cleaned up offensive
 **       comments and new legal headers.
 **  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
 ** $
** 
** 28    3/19/99 11:30a Peter
** protect against splash screen not restoring app state
** 
** 27    2/20/99 5:25a Mark
** fixed breakage in grSplash
** 
** 26    2/19/99 8:03p Peter
** new splash
** 
** 25    2/18/99 3:48p Kcd
** Fixed conflict with MacOS type.
** 
** 24    1/25/99 6:32p Peter
** cleaned up some translation macros and tables
** 
** 23    12/04/98 11:04a Russp
** Put an #ifdef in to gsplash.c to get it working in the sst2 tree
** 
** 22    12/03/98 4:45p Atai
** exit grSplash if aux buffer equals zero
** 
** 21    11/21/98 10:19a Atai
** fixed test37 grChromaRangeModeExt error and rename functions
** 
** 20    7/31/98 5:42p Russp
** make work with sst2
** 
** 19    7/23/98 5:25p Russp
** get rid of redundant grVertexLayout calls
** 
** 18    7/20/98 11:23a Russp
** get working in sst2 tree
** 
** 17    7/18/98 12:29a Jdt
** changes to reflect new shadow register structure
** 
** 16    6/23/98 11:05a Peter
** more compile fixes
** 
** 15    6/22/98 3:35p Peter
** concatenation this way is evil
** 
** 14    6/22/98 2:25p Russp
** Add SST2 specific ifdef's
** 
** 13    6/18/98 2:44p Atai
** fixed 1894
** 
** 12    5/15/98 4:02p Atai
** fogCoord and texchroma extension for Banshee
** 
** 10    2/09/98 10:28a Atai
** set grSpash default to windows coords
** 
** 9     2/06/98 4:25p Atai
** remove GrTmuVertex
** 
** 8     2/05/98 2:30p Atai
** fixed bug#1222.
** 
** 7     1/28/98 6:36p Atai
** remove z,w, min_max definition
** 
** 6     1/28/98 1:13p Peter
** Stupid swap state form gratuitous splash screen
** 
** 5     1/26/98 12:32p Atai
** glide 3 header clean up
** 
** 3     1/26/98 11:30a Atai
** update to new glide.h
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 23    1/09/98 6:48p Atai
 * grTexInfo, GR_LOD_* and GR_ASPECT_*
 * 
 * 21    1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 20    12/18/97 2:13p Peter
 * fogTable cataclysm
 * 
 * 19    12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 18    12/09/97 10:20a Peter
 * removed extraneous cruft
 * 
 * 17    11/14/97 5:03p Peter
 * picking up the torch... removed #!@#$% c++ comments
 * 
 * 16    10/23/97 5:30p Peter
 * packed rgb happiness
 * 
 * 15    7/31/97 11:14a Peter
 * merge w/ sst1 glide tree, hopefully
 * 
 * 13    7/07/97 3:05p Dow
 * Moved fouled clears
 * 
 * 12    3/18/97 9:07p Dow
 * Got rid of #$#%#$ // comments
 * 
 * 11    3/16/97 12:42a Jdt
 * Removed watcom warning
 * 
 * 10    3/13/97 10:53p Jdt
 * Rewrote with simple optimizations.  Changed prototype for
 * multi-resolution, and running as a better shameless plug.
 * 
 * 9     3/13/97 2:52a Jdt
 * Added arguments to splash.
 * 
 * 8     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 7     3/01/97 6:24p Jdt
 * Made splash force yorigin.
 * 
 * 6     1/02/97 1:06p Dow
 * Fixed state bug
 * 
 * 3     11/17/96 11:16p Garymct
 * Updated grSplash code to set all of the necessary state itself rather
 * than expecting the caller to do so.
 */
#ifndef GLIDE_SPLASH
#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"
GR_DIENTRY(grSplash,void,(float x, float y,
                          float w, float h,
                          FxU32 frameNumber)) {
/* nothing */
}

#elif defined(_WIN32)
#include <math.h>

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"

/*-------------------------------------------------------------------
  Function: grSplash
  Date: 3/13
  Implementor(s): GaryMcT
  Library: Glide
  Description:
  Render the opening splash screen animation, or render
  a single frame of the splash screen.
  Arguments:
  x, y - upper left hand coord of window
  w, h - width and height of window to render
  _frame - frame number to render (~0 to render all frames)
  Return:
  none
  -------------------------------------------------------------------*/
GR_DIENTRY(grSplash,void,(float x, float y,
                          float w, float h,
                          FxU32 frameNumber))
{
#define FN_NAME "grSplash"
#if (GLIDE_PLATFORM & GLIDE_HW_SST2)
  GR_BEGIN_NOFIFOCHECK("grSplash", 85, 1);
#else
  GR_BEGIN_NOFIFOCHECK("grSplash", 85);
#endif
  GDBG_INFO_MORE(gc->myLevel,"(%f,%f,%f,%f,%d)\n",
                 x, y, w, h, frameNumber);

#if (GLIDE_PLATFORM & GLIDE_HW_SST2)
#else
  {
    GrState glideState;

    /* Protect ourselves from the splash screen */
    grGlideGetState(&glideState);
    {
      if (gc->pluginInfo.splashProc != NULL) (*gc->pluginInfo.splashProc)(x, y,
                                                                          w, h, 
                                                                          frameNumber);
    }
    grGlideSetState((const void*)&glideState);
  }
#endif

  GR_END();
#undef FN_NAME
}

#else /* others */
#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"
GR_DIENTRY(grSplash,void,(float x, float y,
                          float w, float h,
                          FxU32 frameNumber)) {
/* nothing */
}
#endif
