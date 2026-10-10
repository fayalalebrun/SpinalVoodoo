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
** Revision 1.1.1.1.6.1  2005/05/25 08:56:23  jwrdegoede
** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
**
** Revision 1.1.1.1  1999/11/24 21:44:54  joseph
** Initial checkin for SourceForge
**
** 
** 4     4/06/99 3:54p Dow
** Alt tab again.
** 
** 10    4/04/99 8:51p Atai
** Partial check-in for alt-tab issue. set FX_GLIDE_ALT_TAB=1 to build
** glide3x with hwcQueryContext built into GR_BEGIN_NOFIFOCHECK. It works
** with DEBUG glide only. In the non-debug glide, we can still see the
** desktop corruption.
** 
** 9     3/01/99 3:12p Peter
** grSstSelect cannot call gr_dcl_gc because it may be setting a valid
** context in tls
** 
** 8     7/04/98 5:06p Jdt
** 
** 7     8/03/98 6:37a Jdt
** multi-thread changes
** 
** 6     7/16/98 8:14a Jdt
** fxcmd.h
** 
** 5     4/30/98 5:01p Peter
** first pass glide3 merge
** 
** 3     4/22/98 4:57p Peter
** glide2x merge
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 18    12/17/97 4:05p Atai
 * added grChromaRange(), grGammaCorrecionRGB(), grRest(), and grGet()
 * functions
 * 
 * 17    12/09/97 12:20p Peter
 * mac glide port
 * 
 * 16    11/19/97 2:49p Peter
 * env vars in registry for win32
 * 
 * 15    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 14    10/31/97 9:15a Peter
 * only lie about v2 boards
 * 
 * 13    10/31/97 8:53a Peter
 * last lying change, really
 * 
 * 12    9/05/97 5:29p Peter
 * changes for direct hw
 * 
 * 11    6/20/97 9:56a Peter
 * better lines/pts, hopefully
 * 
 * 10    6/02/97 4:09p Peter
 * Compile w/ gcc for Dural
 * 
 * 9     5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 8     5/21/97 6:04a Peter
 * 
 * 7     5/02/97 2:07p Pgj
 * grSstScreenWidth/Height now FxU32
 * 
 * 6     3/17/97 6:25a Jdt
 * Added initDeviceSelect to grSstSelect()
 * 
 * 5     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 4     3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 3     1/18/97 11:39p Dow
 * Changed location of _curGCFuncs
 * 
 * 2     1/16/97 3:39p Dow
 * Added ref to _curGCFuncs during grSstSelect()
 * 
 * 1     12/23/96 1:39p Dow
 * Changes for multiplatform
**
*/

#include <stdio.h>
#include <string.h>
#include <3dfx.h>

#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"


/*---------------------------------------------------------------------------
** grSstQueryBoards
**
** NOTE:  it is OK to call this routine before grGlideInit
*/
#ifndef GLIDE3_ALPHA
GR_DIENTRY(grSstQueryBoards, FxBool, ( GrHwConfiguration *hwc ))
{
  GDBG_INIT();
  GDBG_INFO(80,"grSstQueryBoards(0x%x)\n",hwc);

#if GLIDE_INIT_HAL
  hwc->num_sst = (_grSstDetectResources()
                  ? _GlideRoot.hwConfig.num_sst
                  : 0);
#else /* !GLIDE_INIT_HAL */
  hwc->num_sst = 1;
#endif /* !GLIDE_INIT_HAL */

  return FXTRUE;
} /* grSstQueryBoards */
#endif

/*---------------------------------------------------------------------------
** grSstQueryHardware
**
*/
GR_DIENTRY(grSstQueryHardware, FxBool, ( GrHwConfiguration *hwc ))
{
  FxBool retVal;

  GR_BEGIN_NOFIFOCHECK_RET("grSstQueryHardware",80);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",hwc);

  /* init and copy the data back to the user's structure */
  retVal = _GlideRoot.hwConfig.num_sst > 0;
  *hwc = _GlideRoot.hwConfig;

  GR_RETURN(retVal);
} /* grSstQueryHardware */

/*---------------------------------------------------------------------------
** grSstSelect
*/
GR_DIENTRY(grSstSelect, void, ( int which ))
{
  /* NB: We cannot use GR_DCL_GC here because we may be setting
   * a context into tls.
   */
  GDBG_INFO(80, "grSstSelect(0x%X)\n", which);

  if ( which >= _GlideRoot.hwConfig.num_sst )
    GrErrorCallback( "grSstSelect:  non-existent SST", FXTRUE );

  _GlideRoot.current_sst = which;
  setThreadValue( (unsigned long)&_GlideRoot.GCs[_GlideRoot.current_sst] );

#ifdef GLIDE_MULTIPLATFORM
  _GlideRoot.curGCFuncs = _GlideRoot.curGC->gcFuncs;
#endif
} /* grSstSelect */

/*---------------------------------------------------------------------------
** grSstScreenWidth
*/
GR_DIENTRY(grSstScreenWidth, FxU32, (void))
{
  GR_DCL_GC;
  GR_ASSERT(gc != NULL);
  return gc->state.screen_width;
} /* grSstScreenWidth */

/*---------------------------------------------------------------------------
** grSstScreenHeight
*/
GR_DIENTRY(grSstScreenHeight, FxU32,  (void))
{
  GR_DCL_GC;
  GR_ASSERT(gc != NULL);
  return gc->state.screen_height;
}

/*---------------------------------------------------------------------------
**  grSstVidMode - override args to grSstOpen()
*/
GR_DIENTRY(grSstVidMode, void, (FxU32 whichSst, FxVideoTimingInfo* vidTimings))
{
  GDBG_INFO(80,"grSstVidMode(%d,0x%x)\n",whichSst,vidTimings);
#ifdef GLIDE_DEBUG
  if (whichSst >= MAX_NUM_SST) {
    char errStr[1028];
    sprintf(errStr, "grSstVidMode:  %ld greater than MAX_NUM_SST (%d)\n",
            whichSst, MAX_NUM_SST);

    GrErrorCallback(errStr, FXTRUE);
  }
#endif

  _GlideRoot.GCs[whichSst].vidTimings = vidTimings;    
} /* grSstVidMode */

