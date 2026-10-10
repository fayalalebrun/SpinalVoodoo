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
** Revision 1.1.1.1.8.2  2005/08/13 21:06:57  jwrdegoede
** Last needed 64 bit fixes for h5/h3, complete 64 bit support for cvg
**
** Revision 1.1.1.1.8.1  2005/05/07 08:40:18  jwrdegoede
** lvalue cast fixes for gcc4
**
** Revision 1.1.1.1  1999/12/07 21:42:33  joseph
** Initial checkin into SourceForge.
**
** 
** 1     10/08/98 11:30a Brent
** 
** 12    7/24/98 1:41p Hohn
** 
** 11    7/22/98 9:18a Atai
** check in the fix for clip coords s and t with respect to aspect ratio
** 
** 10    6/05/98 5:17p Atai
** added #ifdefed code for tsuDataListScaler
** 
** 9     5/28/98 5:06p Atai
** fix previous check-in
** 
** 8     5/28/98 3:02p Atai
** per-calculate vertex table size for strip/fan. make it a compiled time
** option
** 
** 7     5/12/98 4:45p Atai
** move gstrip aa to gaa.c
** 
** 6     3/21/98 11:31a Atai
** added GR_TRIANGLE_STRIP_CONTINUE and GR_TRIANGLE_FAN_CONTINUE
** 
** 5     2/24/98 6:00p Atai
** use 0 offset for vertex data
** 
** 4     2/03/98 3:40p Atai
** remove aa strip/fan and code clean up
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
 * Fixed state muckage in Glide3 parameter data
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

#ifdef GLIDE_USE_C_TRISETUP
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
_grDrawVertexList(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers)
{
#define FN_NAME "_grDrawVertexList"

  /*
  ** simplified code
  */
  FxU32 vSize;
  FxI32 stride;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE(gc->myLevel, "(type = 0x%x, count = %d, pointers = 0x%x)\n",
                 type, count, pointers);

  GR_FLUSH_STATE();

  vSize = gc->state.vData.vSize;
  if (mode == 0)
    stride = gc->state.vData.vStride;
  else
    stride = sizeof(float *) / sizeof (float);

  /* Draw the first (or possibly only) set.  This is necessary because
     the packet is 3_BDDDDDD, and in the next set, the packet is
     3_DDDDDD */
  /*
  ** We try to make tstrip code simple to read. We combine the original code
  ** into a single loop by adding an extra packet type assignment at the end of the loop.
  ** Also, the debugging code are removed temporarily.
  */
  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
#ifdef GLIDE_VERTEX_TABLE
    while (count >= 15) {
      FxI32 k;
      GR_SET_EXPECTED_SIZE( 15 * vSize, 1);
      TRI_STRIP_BEGIN(type, 15, vSize, pktype);
      for (k = 0; k < 15; k++) {
        FxI32 i;
        FxU32 dataElem;
        float *vPtr;
        
        vPtr = pointers;
        if (mode)
          vPtr = *(float **)vPtr;
        pointers = (float *)pointers + stride;
        
        TRI_SETF(FARRAY(vPtr, 0));
        dataElem = 0;
        TRI_SETF(FARRAY(vPtr, 4));
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
    if (count) {
      FxI32 k;
      GR_SET_EXPECTED_SIZE(gc->state.vTable[count], 1);
      TRI_STRIP_BEGIN(type, count, vSize, pktype);
      for (k = 0; k < count; k++) {
        FxI32 i;
        FxU32 dataElem;
        float *vPtr;
        
        vPtr = pointers;
        if (mode)
          vPtr = *(float **)vPtr;
        pointers = (float *)pointers + stride;
        
        TRI_SETF(FARRAY(vPtr, 0));
        dataElem = 0;
        TRI_SETF(FARRAY(vPtr, 4));
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) {
          TRI_SETF(FARRAY(vPtr, i));
          dataElem++;
          i = gc->tsuDataList[dataElem];
        }
      }
      TRI_END;
      GR_CHECK_SIZE();
    }
#else
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
        FxI32 i;
        FxU32 dataElem;
        float *vPtr;
        
        vPtr = pointers;
        if (mode)
          vPtr = *(float **)vPtr;
        pointers = (float *)pointers + stride;
        
        TRI_SETF(FARRAY(vPtr, 0));
        dataElem = 0;
        TRI_SETF(FARRAY(vPtr, 4));
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
#endif
  }
  else {
    /*
     * first cut of clip space coordinate code, no optimization.
     */
#ifdef GLIDE3_SCALER
    float oow;
    float atable[8];
    float btable[8];

    
    while (count > 0) {
      FxI32 k, vcount = count >= 15 ? 15 : count;
      GR_SET_EXPECTED_SIZE(vcount * vSize, 1);
      TRI_STRIP_BEGIN(type, vcount, vSize, pktype);
      for (k = 0; k < vcount; k++) {
        FxI32 i;
        FxI32 iscaler;
        FxU32 dataElem;
        float *vPtr;
        
        vPtr = pointers;
        if (mode)
          vPtr = *(float **)vPtr;
        oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset);

        atable[0] = 0.f;
        atable[1] = oow*gc->state.Viewport.hdepth;
        atable[2] = _GlideRoot.pool.f255;
        atable[3] = oow * gc->state.tmu_config[0].s_scale;
        atable[4] = oow * gc->state.tmu_config[0].t_scale;
        atable[5] = oow * gc->state.tmu_config[1].s_scale;
        atable[6] = oow * gc->state.tmu_config[1].t_scale;
        atable[7] = oow;
        
        btable[0] = oow;
        btable[1] = gc->state.Viewport.oz;
        btable[2] = 0.f;
        btable[3] = 0.f;
        btable[4] = 0.f;
        btable[5] = 0.f;
        btable[6] = 0.f;
        btable[7] = 0.f;
        /* x, y */
        TRI_SETF(FARRAY(vPtr, 0)
                 *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox);
        TRI_SETF(FARRAY(vPtr, 4)
                 *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy);
        pointers = (float *)pointers + stride;
        if (gc->state.vData.colorType == GR_U8) {
          i = gc->tsuDataList[0];
          TRI_SETF(FARRAY(vPtr, i));
          dataElem = 1;
        }
        else
          dataElem = 0;

        i = gc->tsuDataList[dataElem];
        iscaler = gc->tsuDataListScaler[dataElem];
        while (i != GR_DLIST_END) {
          TRI_SETF(FARRAY(vPtr, i) * atable[iscaler] + btable[iscaler]);
          dataElem++;
          i = gc->tsuDataList[dataElem];
          iscaler = gc->tsuDataListScaler[dataElem];
        }
#else
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
        TRI_SETF(FARRAY(vPtr, 0)
                 *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox);
        TRI_SETF(FARRAY(vPtr, 4)
                 *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy);
        pointers = (float *)pointers + stride;

        TRI_VP_SETFS(vPtr, oow);
#endif
      }
      TRI_END;
      GR_CHECK_SIZE();
      count -= 15;
      pktype = SSTCP_PKT3_DDDDDD;
    }
  }

#undef FN_NAME
} /* _grDrawVertexList */
#endif

#endif
