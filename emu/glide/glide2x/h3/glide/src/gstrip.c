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
** 21    12/03/98 11:27p Dow
** Code 'cleanup' heç
 * 
 * 19    12/12/97 12:43p Atai
 * move i and dateElem into the set up loop
 * 
 * 17    12/08/97 10:38a Atai
 * added grDrawVertexArrayLinear()
 * 
 * 16    11/21/97 6:05p Atai
 * use one datalist (tsuDataList) in glide3
 * 
 * 15    11/06/97 6:10p Atai
 * update GrState size
 * rename grDrawArray to grDrawVertexArray
 * update _grDrawPoint and _grDrawVertexList
 * 
 * 14    11/04/97 6:35p Atai
 * 1. sync with data structure changes
 * 2. break up aa triangle routine
 * 
 * 13    11/04/97 4:57p Atai
 * use byte offset
 * 
 * 12    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 11    10/17/97 2:11p Atai
 * added grContinueArray. We only support non aa mode for now.
 * 
 * 10    10/16/97 1:50p Atai
 * fix drawarray bugs
 * 
 * 9     10/14/97 6:16p Atai
 * reverse triangle order in _grAADrawVertexList
 * 
 * 8     10/14/97 5:41p Atai
 * added _grAADrawVertexList()
 * 
 * 7     10/14/97 4:57p Dow
 * Clamping
 * 
 * 6     10/09/97 8:02p Dow
 * State Monster 1st Cut
 * 
 * 5     10/08/97 11:32a Peter
 * pre-computed packet headers for packet 3
 * 
 * 4     9/29/97 1:26p Dow
 * Fixed packed color strips/fans
 * 
 * 3     9/26/97 10:24a Dow
 * Fixed state effage in Glide3 parameter data
 * 
 * 2     9/23/97 2:35p Dow
 * One less loop
 * 
 * 1     9/23/97 2:04p Dow
 * DD code for strips
**
*/
#ifdef GLIDE3
#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

/*-------------------------------------------------------------------
  Function: _grDrawVertexList
  Date: 18-Sep-97
  Implementor(s): dow
  Description:
        Sends a triangle strip to CVG.
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grDrawVertexList(FxU32 type, FxI32 mode, FxI32 count, void *pointers)
{
#define FN_NAME "_grDrawVertexList"

  /*
  ** simplified code
  */
  FxU32
    vNum = 0,
    vSize;
  float
    **lPtrs = (float **) pointers;
  FxI32 stride = mode;
  FxU32
    sCount = count, set = 0, pktype = SSTCP_PKT3_BDDDDD;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE(gc->myLevel, "(type = 0x%x, count = %d, pointers = 0x%x)\n",
                 type, count, pointers);

  GR_FLUSH_STATE();

  vSize = gc->state.vData.vSize;
  if (stride == 0)
    stride = gc->state.vData.vStride;

  /* Draw the first (or possibly only) set.  This is necessary because
     the packet is 3_BDDDDDD, and in the next set, the packet is
     3_DDDDDD */
  /*
  ** We try to make tstrip code simple to read. We combine the original code
  ** into a single loop by adding an extra packet type assignment at the end of the loop.
  ** Also, the debugging code are removed temporarily.
  */
  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    while (count > 0) {
      FxI32 k, vcount = count >= 15 ? 15 : count;
      GR_SET_EXPECTED_SIZE(vcount * vSize, 1);
      TRI_STRIP_BEGIN(type, vcount, vSize, pktype);
      /*
      ** If we use a while loop, the compiler will increment vNum and store the value back
      ** to the memory at every loop. In a for loop, vNum data are kept in a register.
      ** After the loop complete, the vNum data are written back to memory.
      */
      for (k = 0; k < vcount; k++) {
        FxU32 i, dataElem;
        float *vPtr;
        
        vPtr = pointers;
        if (mode)
          vPtr = *(float **)vPtr;
        (float *)pointers += stride;
        
        TRI_SETF(FARRAY(vPtr, GR_VERTEX_X_OFFSET << 2));
        dataElem = 0;
        TRI_SETF(FARRAY(vPtr, GR_VERTEX_Y_OFFSET << 2));
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) {
          TRI_SETF(FARRAY(vPtr, i));
          dataElem++;
          i = gc->tsuDataList[dataElem];
        }
      }
      TRI_END;
      GR_CHECK_SIZE();
      count -= 15;
      pktype = SSTCP_PKT3_DDDDDD;
    }
  }
  else {
    /*
     * first cut of clip space coordinate code, no optimization.
     */
    float oow;
    
    while (count > 0) {
      FxI32 k, vcount = count >= 15 ? 15 : count;
      GR_SET_EXPECTED_SIZE(vcount * vSize, 1);
      TRI_STRIP_BEGIN(type, vcount, vSize, pktype);
      for (k = 0; k < vcount; k++) {
        float *vPtr;
        
        vPtr = pointers;
        if (mode)
          vPtr = *(float **)vPtr;
        oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset);
        /* x, y */
        TRI_SETF(FARRAY(vPtr, GR_VERTEX_X_OFFSET << 2)
                 *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox);
        TRI_SETF(FARRAY(vPtr, GR_VERTEX_Y_OFFSET << 2)
                 *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy);
        (float *)pointers += stride;

        TRI_VP_SETFS(vPtr, oow);
      }
      TRI_END;
      GR_CHECK_SIZE();
      count -= 15;
      pktype = SSTCP_PKT3_DDDDDD;
    }
  }

#undef FN_NAME
} /* _grDrawVertexList */

/*-------------------------------------------------------------------
  Function: _grAADrawVertexList
  Date: 14-Oct-97
  Implementor(s): atai
  Description:
  Sends an aa triangle strip/fan to CVG.
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grAADrawVertexList(FxU32 type, FxI32 mode, FxI32 count, void *pointers)
{
#define FN_NAME "_grAADrawVertexList"

  GR_DCL_GC;
  FxU32 vNum = 0;
  FxU32 sCount = count;
  float
    **lPtrs = (float **) pointers;
  float *v[3];
  FxBool flip = FXFALSE;
  FxU32 fbzModeOld;                 /* Squirrel away current fbzMode */
  FxI32 stride = mode;

  if (sCount <= 2) return;

  _grDrawVertexList(type, mode, count, pointers);

  fbzModeOld = gc->state.fbi_config.fbzMode;
  gc->state.fbi_config.fbzMode &= ~(SST_ZAWRMASK);
  /* gc->state.invalid |= fbzModeBIT; */
  GR_FLUSH_STATE();
  if (stride == 0)
    stride = gc->state.vData.vStride;

  sCount-=2;
  if (type == kSetupFan) {
    v[0] = (mode == 0) ? pointers : *(float **)pointers;
    while (sCount--) {
      (float *)pointers += stride;
      if (mode) {
        v[1] = *(float **)pointers;
        v[2] = *((float **)pointers+1);
      } else {
        v[1] = pointers;
        v[2] = (float *)pointers+stride;
      }
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        _grAADrawTriangles(1, type, 3, v);
      else
        _grAAVpDrawTriangles(1, type, 3, v);    
    }
  }
  else if (type == kSetupStrip){
    while (sCount--) {
      if (flip) {
        if (mode) {
          v[0] = *((float **)pointers+1);
          v[1] = *(float **)pointers;
          v[2] = *((float **)pointers+2);
        } else {
          v[0] = (float *)pointers+stride;
          v[1] = pointers;
          v[2] = (float *)pointers+(stride<<1);
        }
      }
      else { 
        if (mode) {
          v[0] = *(float **)pointers;
          v[1] = *((float **)pointers+1);
          v[2] = *((float **)pointers+2);
        } else {
          v[0] = pointers;
          v[1] = (float *)pointers+stride;
          v[2] = (float *)pointers+(stride<<1);
        }
      }
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        _grAADrawTriangles(1, type, 3, v);
      else
        _grAAVpDrawTriangles(1, type, 3, v);    
      (float *)pointers += stride;
      flip = ~flip;
    }
    flip = ~flip;
  }

  gc->state.fbi_config.fbzMode = fbzModeOld;
  gc->state.invalid |= fbzModeBIT;
  GR_FLUSH_STATE();

#undef FN_NAME
} /* _grAADrawVertexList */

#endif
