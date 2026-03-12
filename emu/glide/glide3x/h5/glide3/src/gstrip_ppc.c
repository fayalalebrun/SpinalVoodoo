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
** 1     7/30/99 1:12p Kcd
** Optimized strip setup for PowerPC.
** 
** 14    7/14/99 9:39a Atai
** direct register write for glide3x
** test04 can do 4 sample aa (2 chips)
** 
** 13    6/03/99 12:14p Kcd
** PowerPC still needs some C code when C triangle setup is disabled.
** 
** 12    4/16/99 2:58p Kcd
** Hack to use more general (but slower) SETF() macro.
** 
** 11    12/03/98 11:26p Dow
** Code 'cleanup' heheh
** 
** 10    10/16/98 1:24p Peter
** c trisetup fixes
** 
** 9     6/09/98 11:59a Atai
** 1. update glide header
** 2. fix cull mode
** 3. fix tri stats
** 
** 8     5/15/98 4:02p Atai
** fogCoord and texchroma extension for Banshee
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
 * Fixed state effage in Glide3 parameter data
 * 
 * 2     9/23/97 2:35p Dow
 * One less loop
 * 
 * 1     9/23/97 2:04p Dow
 * DD code for strips
**
*/

#if SET_BSWAP
#define SLOW_SETF 1
#endif

#ifdef GLIDE3
#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

#if (GLIDE_PLATFORM & GLIDE_SST_SIM)
#if HAL_CSIM
#include <csim.h>
#else
#include <gsim.h>
#endif 
#endif

#if GLIDE_USE_C_TRISETUP || __POWERPC__
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

#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  /*
  ** simplified code
  */
  FxU32 vSize;
  FxI32 stride = mode;

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
      
	    {
	      union {
	        FxU32 buffer[2];
	        double buffer_double;
	      } buff;
	      
	      const int *dataList;
	      FxI32 i;
	      float *vPtr;
	      
	      FxU32 *tPackPtr = gc->cmdTransportInfo.fifoPtr;
	      
	      const FxU32 packetVal = (((type) << SSTCP_PKT3_SMODE_SHIFT) |
	                               ((vcount) << SSTCP_PKT3_NUMVERTEX_SHIFT) |
	                               (pktype) |
	                               gc->cmdTransportInfo.cullStripHdr);
	                               
	      TRI_ASSERT_DECL(vcount, vSize, packetVal);
	      
	      k = vcount;
	      
	      if((FxU32)tPackPtr & 7) {
	        SET(*tPackPtr++, packetVal);
	        goto vertex_end_empty;
	      } else {
	        SET(buff.buffer[0], packetVal);
	        goto vertex_end_half;
	      }
	      
		    /*
		    ** If we use a while loop, the compiler will increment vNum and store the value back
		    ** to the memory at every loop. In a for loop, vNum data are kept in a register.
		    ** After the loop complete, the vNum data are written back to memory.
		    */
vertex_begin_empty:
	      vPtr = pointers;
	      if(mode)
	        vPtr = *(float **)vPtr;
	      SETF(buff.buffer[0],vPtr[0]);
	      SETF(buff.buffer[1],vPtr[1]);
	      (float *)pointers += stride;
	      dataList = gc->tsuDataList;
	      *(double *)tPackPtr = *(double *)&buff.buffer[0];
	      tPackPtr += 2;
	      goto vertex_loop_empty;
	      
vertex_begin_half:
	      vPtr = pointers;
	      if(mode)
	        vPtr = *(float **)vPtr;
	      SETF(buff.buffer[1],vPtr[0]);
	      (float *)pointers += stride;
	      dataList = gc->tsuDataList;
	      *(double *)tPackPtr = *(double *)&buff.buffer[0];
	      tPackPtr += 2;
	      SETF(buff.buffer[0],vPtr[1]);
	      goto vertex_loop_half;
	    
vertex_loop_empty:
	      i = *dataList++;
	      if(i == GR_DLIST_END) {
	        goto vertex_end_empty;
	      }
	      SETF(buff.buffer[0],FARRAY(vPtr,i));
	
vertex_loop_half:
	      i = *dataList++;
	      if(i == GR_DLIST_END) {
	        goto vertex_end_half;
	      }
	      SETF(buff.buffer[1],FARRAY(vPtr,i));
	      *(double *)tPackPtr = *(double *)&buff.buffer[0];
	      tPackPtr += 2;
	      goto vertex_loop_empty;
	      
vertex_end_empty:
	      k--;
	      if(k >= 0) {
	        goto vertex_begin_empty;
	      }
	      goto vertex_end;
	
vertex_end_half:
	      k--;
	      if(k >= 0) {
	        goto vertex_begin_half;
	      }
	      
	      *tPackPtr++ = buff.buffer[0];
	
vertex_end:     
	      /* TRI_ASSERT(); */
	      gc->cmdTransportInfo.fifoRoom -= ((FxU32)tPackPtr - (FxU32)gc->cmdTransportInfo.fifoPtr);
	      gc->cmdTransportInfo.fifoPtr = tPackPtr; 
	      GDBG_INFO(gc->myLevel + 200, "\tTriEnd: (0x%X : 0x%X)\n", tPackPtr, gc->cmdTransportInfo.fifoRoom);
	      FIFO_ASSERT();
	                   	    
		    /* GR_CHECK_SIZE(); */
	      count -= 15;
	      pktype = SSTCP_PKT3_DDDDDD;
	    }
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
        TRI_SETF(FARRAY(vPtr, 0)
                 *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox);
        TRI_SETF(FARRAY(vPtr, 4)
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
#endif

#undef FN_NAME
} /* _grDrawVertexList */
#endif

#endif



