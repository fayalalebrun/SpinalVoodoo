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
** Revision 1.1.1.1.8.1  2004/10/04 09:13:33  dborca
** make sure SLI boards are correctly counted
**
** Revision 1.1.1.1  1999/12/07 21:42:30  joseph
** Initial checkin into SourceForge.
**
** 
** 1     10/08/98 11:30a Brent
** 
** 6     8/07/98 6:33p Atai
** enable grSstVidMode for arcade developer
** 
** 5     7/24/98 1:41p Hohn
** 
** 4     6/24/98 1:47p Atai
** code clean up; rename texute line routine
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

#if 1/*GLIDE_INIT_HAL*/ /* [dBorca] must handle SLI correctly */
  hwc->num_sst = (_grSstDetectResources()
                  ? _GlideRoot.hwConfig.num_sst
                  : 0);
#else /* !GLIDE_INIT_HAL */
  hwc->num_sst = sst1InitNumBoardsInSystem();
#endif /* !GLIDE_INIT_HAL */

  return FXTRUE;
} /* grSstQueryBoards */
#endif

/*---------------------------------------------------------------------------
** grSstSelect
*/
GR_DIENTRY(grSstSelect, void, ( int which ))
{
  GR_BEGIN_NOFIFOCHECK("grSstSelect",80);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",which);

  if ( which >= _GlideRoot.hwConfig.num_sst )
    GrErrorCallback( "grSstSelect:  non-existent SST", FXTRUE );

  _GlideRoot.current_sst = _GlideRoot.gcMap[which];
  _GlideRoot.curGC       = &_GlideRoot.GCs[_GlideRoot.current_sst];

#ifdef GLIDE_MULTIPLATFORM
  _GlideRoot.curGCFuncs = _GlideRoot.curGC->gcFuncs;
#endif
  
  GR_END();
} /* grSstSelect */

/*---------------------------------------------------------------------------
**  grSstVidMode - override args to grSstOpen()
*/
GR_ENTRY(grSstVidMode, void, (FxU32 whichSst, FxVideoTimingInfo* vidTimings))
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

