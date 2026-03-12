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
 * 
 * 9     8/13/97 8:51a Pgj
 * Fix Bug 710
 * 
 * 8     7/24/97 10:55a Dow
 * Fixed bogus check in grSstQueryHardware
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
#include <sst1vid.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"


/*---------------------------------------------------------------------------
** grSstQueryBoards
**
** NOTE:  it is OK to call this routine before grGlideInit
*/
GR_DIENTRY(grSstQueryBoards, FxBool, ( GrHwConfiguration *hwc ))
{
  GDBG_INFO((80,"grSstQueryBoards(0x%x)\n",hwc));

  hwc->num_sst = initNumBoardsInSystem();
  return FXTRUE;
} /* grSstQueryBoards */

/*---------------------------------------------------------------------------
** grSstQueryHardware
**
*/
GR_DIENTRY(grSstQueryHardware, FxBool, ( GrHwConfiguration *hwc ))
{
  FxBool retVal;

  GDBG_INFO((80, "grSstQueryHardware\n"));
  GDBG_INFO_MORE((80,"(0x%x)\n",hwc));

  /* init and copy the data back to the user's structure */
  retVal = _GlideRoot.hwConfig.num_sst > 0;
  *hwc = _GlideRoot.hwConfig;

  return(retVal);
} /* grSstQueryHardware */

/*---------------------------------------------------------------------------
** grSstSelect
*/
GR_DIENTRY(grSstSelect, void, ( int which ))
{
  if ( which >= _GlideRoot.hwConfig.num_sst )
    GrErrorCallback( "grSstSelect:  non-existent SST", FXTRUE );

  _GlideRoot.current_sst = which;
  _GlideRoot.curGC = &_GlideRoot.GCs[which];
#ifdef GLIDE_MULTIPLATFORM
  _GlideRoot.curGCFuncs = _GlideRoot.curGC->gcFuncs;
#endif
  /* now begin a normal Glide routine's flow */
  {
    GR_BEGIN_NOFIFOCHECK("grSstSelect",80);
    GDBG_INFO_MORE((gc->myLevel,"(%d)\n",which));

    _GlideRoot.packerFixAddress  = ( FxU32 ) gc->tex_ptr;
    _GlideRoot.packerFixAddress += ( ( ( FxU32 ) 3 ) << 21 );
    _GlideRoot.packerFixAddress += ( ( ( FxU32 ) 1 ) << 17 );

    /* Now that we have selected a board, we can build the offests and register
       lists for the  optimized triangle setup code */
    _grRebuildDataList();

    initDeviceSelect( which );

    GR_END();
  }
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
GR_DIENTRY(grSstVidMode, void,
           (FxU32 whichSst, sst1VideoTimingStruct *vidTimings))
{
  GDBG_INFO((80,"grSstVidMode(%d,0x%x)\n",whichSst,vidTimings));
#ifdef GLIDE_DEBUG
  if (whichSst >= MAX_NUM_SST) {
    char errStr[1028];
    sprintf(errStr, "grSstVidMode:  %d greater than MAX_NUM_SST (%d)\n",
            whichSst, MAX_NUM_SST);

    GrErrorCallback(errStr, FXTRUE);
  }
#endif

  _GlideRoot.GCs[whichSst].vidTimings = vidTimings;    
} /* grSstVidMode */

