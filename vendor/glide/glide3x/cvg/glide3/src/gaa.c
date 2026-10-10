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
** Revision 1.1.1.1.8.5  2006/01/16 21:22:41  jwrdegoede
** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
**
** Revision 1.1.1.1.8.4  2005/08/13 21:06:57  jwrdegoede
** Last needed 64 bit fixes for h5/h3, complete 64 bit support for cvg
**
** Revision 1.1.1.1.8.3  2005/06/09 18:32:08  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.1.1.8.2  2005/05/07 08:40:16  jwrdegoede
** lvalue cast fixes for gcc4
**
** Revision 1.1.1.1.8.1  2004/10/04 09:17:16  dborca
** killed some warnings / compilation errors
**
** Revision 1.1.1.1  1999/12/07 21:42:32  joseph
** Initial checkin into SourceForge.
**
** 
** 1     10/08/98 11:30a Brent
** 
** 23    10/07/98 9:43p Peter
** triangle procs for 3DNow!(tm)
** 
** 22    10/06/98 8:23p Peter
** 3DNow!(tm) texture downloads
** 
** 21    9/25/98 5:40p Atai
** fix aa point routine - saving the wrong culling flag
** 
** 20    9/21/98 7:02p Dow
** 3DNow! (tm) mods
** 
** 19    9/18/98 6:44p Atai
** fix clip coords line and triangles
** 
** 18    7/24/98 1:41p Hohn
** 
** 17    7/07/98 10:28a Atai
** remove if-test for abs(adx)
** 
** 16    7/06/98 7:41p Atai
** fixed triangle edge
** 
** 14    6/29/98 4:43p Atai
** fixed clip coord grAADrawTriangle
** 
** 13    6/25/98 1:06p Atai
** fixed aaVpDrawArrayEdgeSense
** 
** 12    6/24/98 6:55p Atai
** undo texture line
** 
** 11    6/24/98 1:47p Atai
** code clean up; rename texute line routine
** 
** 10    6/22/98 7:00p Atai
** remove FX_EXPORT from a number of functions
** 
** 9     6/18/98 10:59a Atai
** added grDrawTextureLine for OGL
** 
** 8     6/08/98 3:20p Atai
** fix tri stats
** 
** 7     6/05/98 7:39p Atai
** fix _grAADrawLineStrip culling mode
** 
** 6     5/29/98 2:09p Atai
** remove polygon code
** 
** 5     5/12/98 4:45p Atai
** move gstrip aa to gaa.c
** 
** 4     3/21/98 11:31a Atai
** added GR_TRIANGLE_STRIP_CONTINUE and GR_TRIANGLE_FAN_CONTINUE
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
 * 61    1/15/98 2:46p Atai
 * fixed grDrawPoint and grDrawLine in aa mode
 * 
 * 60    12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 59    12/12/97 12:43p Atai
 * move i and dateElem into the set up loop
 * 
 * 57    12/08/97 10:40a Atai
 * modify draw vertex primitive routines to do grDrawVertexArrayLinear()
 * 
 * 56    12/05/97 4:26p Peter
 * watcom warnings
 * 
 * 55    11/24/97 4:40p Peter
 * aa prims vs packing
 * 
 * 54    11/21/97 6:05p Atai
 * use one datalist (tsuDataList) in glide3
 * 
 * 53    11/21/97 3:20p Peter
 * direct writes tsu registers
 * 
 * 52    11/18/97 6:11p Peter
 * fixed glide3 muckage
 * 
 * 51    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 50    11/18/97 3:26p Atai
 * update vData 
 * optimize state monster
 * 
 * 49    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 48    11/06/97 6:10p Atai
 * update GrState size
 * rename grDrawArray to grDrawVertexArray
 * update _grDrawPoint and _grDrawVertexList
 * 
 * 47    11/04/97 6:35p Atai
 * 1. sync with data structure changes
 * 2. break up aa triangle routine
 * 
 * 46    11/04/97 5:04p Peter
 * cataclysm part deux
 * 
 * 45    11/04/97 4:57p Atai
 * use byte offset
 * 
 * 44    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 43    11/03/97 3:19p Atai
 * optimization
 * 
 * 42    10/29/97 2:45p Peter
 * C version of Taco's packing code
 * 
 * 41    10/29/97 2:24p Atai
 * re-work aa draw routines to increase sbench number
 * 
 * 40    10/21/97 8:38p Atai
 * added lines routine for grDrawArray
 * 
 * 39    10/21/97 3:22p Peter
 * hand pack rgb
 * 
 * 38    10/19/97 12:51p Peter
 * no tsu happiness
 * 
 * 37    10/17/97 3:15p Peter
 * removed unused addr field from datalist
 * 
 * 36    10/17/97 10:15a Peter
 * packed rgb state cleanup
 * 
 * 35    10/16/97 5:33p Peter
 * argb != rgba
 * 
 * 34    10/16/97 3:40p Peter
 * packed rgb
 * 
 * 33    10/16/97 1:50p Atai
 * fix drawarray bugs
 * 
 * 32    10/14/97 7:33p Atai
 * fix compiler error
 * 
 * 31    10/14/97 5:40p Atai
 * added grculltest
 * 
 * 30    10/14/97 4:36p Atai
 * added aa points, strip line and trianlges for drawarray
 * 
 * 29    10/09/97 8:02p Dow
 * State Monster 1st Cut
 * 
 * 28    10/08/97 5:19p Peter
 * optinally clamp only texture params
 * 
 * 27    10/08/97 11:32a Peter
 * pre-computed packet headers for packet 3
 * 
 * 26    9/20/97 4:42p Peter
 * tri_setf fixup/big fifo
 * 
 * 25    9/16/97 2:49p Peter
 * fixed watcom unhappiness w/ static initializers
 * 
 * 24    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 23    9/10/97 10:13p Peter
 * fifo logic from GaryT, non-normalized fp first cut
 * 
 * 22    8/30/97 5:58p Tarolli
 * cleanups
 * 
 * 21    7/29/97 4:31p Atai
 * replace old edge sense routine
 * 
 * 19    7/28/97 2:42p Peter
 * aa points?
 * 
 * 18    7/26/97 3:04p Peter
 * cleanup
 * 
 * 17    7/25/97 11:40a Peter
 * removed dHalf, change field name to match real use for cvg
 * 
 * 16    6/30/97 3:21p Peter
 * more aa through cmd fifo
 * 
 * 15    6/20/97 1:17p Peter
 * changes for new CVG_TRI macros
**
*/

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"

static void aaDrawArrayEdgeSense(float *a, float *b, float *c);
static void aaVpDrawArrayEdgeSense(float *a, float *b, float *c, float oowa, float oowb);

/*-------------------------------------------------------------------
  Function: grDrawTextureLine
  Date: 15-Jun-97
  Implementor(s): atai
  Description:
  Draw texture line line
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
extern void FX_CALL 
_grDrawTextureLine_Default(const void *a, const void *b)
{
#define FN_NAME "grDrawTextureLine_Default"

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 91);

#define  DX _GlideRoot.pool.temp1
#define ADY _GlideRoot.pool.temp2

  GR_FLUSH_STATE();

  {
    int i;
    FxU32 vSize, dataElem;
    
    /*
    ** compute absolute deltas and draw from low Y to high Y
    */
    ADY.f = FARRAY(b, 4) - FARRAY(a, 4);
    if (ADY.i < 0) {
      const void *tv;
      tv = a; a = b; b = tv;
      ADY.i ^= 0x80000000;            /* ady = -ady; */
    }
    
    DX.f = FARRAY(b, 0) - FARRAY(a, 0);
    DX.i &= 0x7fffffff;            /* abs(adx) */
    
    /* check for zero-length lines */
    if ((DX.i >= ADY.i) && (DX.i == 0))
      return;
    
    vSize = gc->state.vData.vSize + 8;
    GR_SET_EXPECTED_SIZE((vSize<< 2), 1);
    TRI_STRIP_BEGIN(kSetupCullDisable | kSetupStrip, 4, vSize, 
                    SSTCP_PKT3_BDDDDD | (1<<15));
    /* x major */
    if (DX.i >= ADY.i) {
      TRI_SETF(FARRAY(b, 0));
      dataElem = 0;
      TRI_SETF(FARRAY(b, 4) - _GlideRoot.pool.fHalf);
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(b, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }
      TRI_SETF(0.f);
      TRI_SETF(0.f);
      
      TRI_SETF(FARRAY(a, 0));
      dataElem = 0;
      TRI_SETF(FARRAY(a, 4) - _GlideRoot.pool.fHalf);
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(a, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }
      TRI_SETF(0.f);
      TRI_SETF(0.f);
      
      TRI_SETF(FARRAY(b, 0));
      dataElem = 0;
      TRI_SETF(FARRAY(b, 4) + _GlideRoot.pool.fHalf);
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(b, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }
      TRI_SETF(256.f);
      TRI_SETF(0.f);
      
      TRI_SETF(FARRAY(a, 0));
      dataElem = 0;
      TRI_SETF(FARRAY(a, 4) + _GlideRoot.pool.fHalf);
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(a, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }
      TRI_SETF(256.f);
      TRI_SETF(0.f);
    } else { /* y major */
      TRI_SETF(FARRAY(b, 0) - _GlideRoot.pool.fHalf);
      dataElem = 0;
      TRI_SETF(FARRAY(b, 4));
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(b, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }
      TRI_SETF(0.f);
      TRI_SETF(0.f);
          
      TRI_SETF(FARRAY(a, 0) - _GlideRoot.pool.fHalf);
      dataElem = 0;
      TRI_SETF(FARRAY(a, 4));
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(a, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }
      TRI_SETF(0.f);
      TRI_SETF(0.f);
      
      TRI_SETF(FARRAY(b, 0) + _GlideRoot.pool.fHalf);
      dataElem = 0;
      TRI_SETF(FARRAY(b, 4));
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(b, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }
      TRI_SETF(256.f);
      TRI_SETF(0.f);
      
      TRI_SETF(FARRAY(a, 0) + _GlideRoot.pool.fHalf);
      dataElem = 0;
      TRI_SETF(FARRAY(a, 4));
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(a, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }
      TRI_SETF(256.f);
      TRI_SETF(0.f);
    }
    TRI_END;
    GR_CHECK_SIZE();
  }
  _GlideRoot.stats.linesDrawn++;
  _GlideRoot.stats.othertrisDrawn+=2;

#undef FN_NAME
} /* grDrawTextureLine */

/*
** -----------------------------------------------------------------------
**
** Antialiasing rendering functions
**
** -----------------------------------------------------------------------
*/

typedef enum {
  aaEdgeSenseTop,
  aaEdgeSenseLeft,
  aaEdgeSenseBottom,
  aaEdgeSenseRight
} aaEdgeSense;

/*---------------------------------------------------------------------------
** grAADrawTriangle
**
**  NOTE:  
*/
GR_ENTRY(grAADrawTriangle,
         void,
         (const void *a, const void *b, const void *c,
          FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias)) 
{
#define FN_NAME "grAADrawTriangle"
  FxU32
    fbzMode,                    /* What we write to fbzMode */
    fbzModeOld;                 /* Squirrel away current fbzMode */
  float *vlist[3];

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 96);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x,0x%x,0x%x,%d,%d,%d)\n",
                 a,b,c,ab_antialias,bc_antialias,ca_antialias);
  
  GR_FLUSH_STATE();

  fbzModeOld = gc->state.fbi_config.fbzMode;
  
  {
    /*
    ** culling
    */
    float dxAB, dxBC, dyAB, dyBC;
    union { float f; FxI32 i; } j;
    dxAB = FARRAY(a, 0) - FARRAY(b, 0);
    dxBC = FARRAY(b, 0) - FARRAY(c, 0);
    
    dyAB = FARRAY(a, 4) - FARRAY(b, 4);
    dyBC = FARRAY(b, 4) - FARRAY(c, 4);
    
    j.f = dxAB * dyBC - dxBC * dyAB;
    if ((j.i & 0x7FFFFFFF) == 0) 
      return;
    if ((gc->state.cull_mode != GR_CULL_DISABLE) && (((FxI32)(j.i ^ (gc->state.cull_mode << 31UL))) >= 0))
      return;
  }
  
  vlist[0] = (float *)a;
  vlist[1] = (float *)b;
  vlist[2] = (float *)c;
  (*gc->curArchProcs.drawTrianglesProc)(GR_VTX_PTR_ARRAY, 3, (void *)vlist);
   
  /* Disable depth buffer writes for edge triangles */
  fbzMode = fbzModeOld;
  fbzMode &= ~(SST_ZAWRMASK);

  GR_SET_EXPECTED_SIZE(4, 1);
  GR_SET(BROADCAST_ID, hw, fbzMode, fbzMode);
  GR_CHECK_SIZE();

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    if (ab_antialias)
      aaDrawArrayEdgeSense((float *)a, (float *)b, (float *)c);
    
    if (bc_antialias)
      aaDrawArrayEdgeSense((float *)b, (float *)c, (float *)a);
    
    if (ca_antialias)
      aaDrawArrayEdgeSense((float *)c, (float *)a, (float *)b);
  }
  else {
    float oowa = 0.0, oowb = 0.0, oowc = 0.0;
    if (ab_antialias) {
      oowa = 1.0f / FARRAY(a, gc->state.vData.wInfo.offset);        
      oowb = 1.0f / FARRAY(b, gc->state.vData.wInfo.offset);        
      aaVpDrawArrayEdgeSense((float *)a, (float *)b, (float *)c, oowa, oowb);
    }
      
    if (bc_antialias) {
      if (!ab_antialias)
        oowb = 1.0f / FARRAY(b, gc->state.vData.wInfo.offset);        
      oowc = 1.0f / FARRAY(c, gc->state.vData.wInfo.offset);
      aaVpDrawArrayEdgeSense((float *)b, (float *)c, (float *)a, oowb, oowc);
    }
      
    if (ca_antialias) {
      if (!ab_antialias)
        oowa = 1.0f / FARRAY(a, gc->state.vData.wInfo.offset);        
      if (!bc_antialias)
        oowc = 1.0f / FARRAY(c, gc->state.vData.wInfo.offset);
      aaVpDrawArrayEdgeSense((float *)c, (float *)a, (float *)b, oowc, oowa);
    }
  }

  /* Restore the old fbzMode */
  GR_SET_EXPECTED_SIZE(4, 1);
  GR_SET(BROADCAST_ID, hw, fbzMode, fbzModeOld);
  GR_END();
#undef FN_NAME
} /* grAADrawTriangle */

#define OK_TO_SKIP_XY
#define ZEROALPHA(_ss)   *((FxU32 *)&_ss)=*((FxU32 *)&_ss)&0x00ffffff

/*-------------------------------------------------------------------
  Function: _grAADrawPoints
  Date: 13-Oct-97
  Implementor(s): atai
  Description:
  Draw anti-alias points
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grAADrawPoints(FxI32 mode, FxI32 count, void *pointers)
{
#define FN_NAME "_grAADrawPoints"

  /*
   * the aa point routine is similar to grAADrawPoint routine
   * except the data set up is from the pointer array and 
   * its data layout
   */
  GR_DCL_GC;
  float *e, ptX, ptY;
  FxI32 i, ia;
  FxU32 vsize;
  FxI32 stride;
  FxU32 tmp_cullStripHdr;

  GDBG_INFO(94,"_grAADrawPoints(0x%x)\n",pointers);

  GDBG_INFO_MORE(gc->myLevel, "(count = %d, pointers = 0x%x)\n",
                 count, pointers);

  if (mode == 0)
    stride = gc->state.vData.vStride;
  else
    stride = sizeof(float *) / sizeof (float);

  if (gc->state.vData.colorType == GR_FLOAT)
    ia = gc->state.vData.aInfo.offset;
  else
    ia = gc->state.vData.pargbInfo.offset;

  vsize = gc->state.vData.vSize * 6;

  /* turn off culling so triangles unconditionally draw */
  tmp_cullStripHdr = gc->cmdTransportInfo.cullStripHdr;
  gc->cmdTransportInfo.cullStripHdr &= GR_CULL_MASK;

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    while (count--) {
      FxU32 dataElem = 0;

      /* We draw this as a 4 triangle fan centered around E. */
      GR_SET_EXPECTED_SIZE(vsize, 1);
      TRI_STRIP_BEGIN(kSetupFan, 
                      0x06UL, gc->state.vData.vSize, 
                      SSTCP_PKT3_BDDDDD);

      e = pointers;
      if (mode)
        e = *(float **)e;
      pointers = (float *)pointers + stride;
      
      ptX = FARRAY(e, gc->state.vData.vertexInfo.offset);
      ptY = FARRAY(e, gc->state.vData.vertexInfo.offset+4);

      /* Send down the original point center */
      TRI_SETF(ptX);
      TRI_SETF(ptY);

      dataElem = 0;
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(e, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }
      
      /* Send the 'surrounding' vertices w/ alpha == 0.0f.  To avoid
       * the vertex copying from the previous version we carry around
       * a table of offsets that we add into the original point to get
       * the new points.
       */
      {
        unsigned int v;
        static const float xAdjust[] = {  1.0f, -1.0f, -1.0f, 1.0f,  1.0f };
        static const float yAdjust[] = { -1.0f, -1.0f,  1.0f, 1.0f, -1.0f };
      
        ASSERT(ArraySize(xAdjust) == ArraySize(yAdjust));
        
        if (gc->state.vData.colorType == GR_FLOAT) {
          
          for(v = 0; v < ArraySize(xAdjust); v++) {            
            TRI_SETF(ptX + xAdjust[v]);
            TRI_SETF(ptY + yAdjust[v]);
            
            dataElem = 0;
            
            i = gc->tsuDataList[dataElem];
            
            while (i != GR_DLIST_END) {
              if (i == ia) {
                TRI_SETF(0.0f);
              }
              else {
                TRI_SETF(FARRAY(e, i));
              }
              dataElem++;
              i = gc->tsuDataList[dataElem];
            }
          }
        }
        else {
          for(v = 0; v < ArraySize(xAdjust); v++) {            
            TRI_SETF(ptX + xAdjust[v]);
            TRI_SETF(ptY + yAdjust[v]);
            
            dataElem = 0;
            
            i = gc->tsuDataList[dataElem];
            
            while (i != GR_DLIST_END) {
              if (i == ia) {
                union { float f; FxU32 u; } argb;
                argb.f = *(float *)((unsigned char *)e + i);
                argb.u &= 0x00ffffff;
                TRI_SETF(argb.f);
              }
              else {
                TRI_SETF(FARRAY(e, i));
              }
              dataElem++;
              i = gc->tsuDataList[dataElem];
            }
          }
        }
        
      }
      TRI_END;
      GR_CHECK_SIZE();
    }
  }
  else {
    /*
     * first cut of clip space coordinate code.
     */
    
    float oow;

    while (count--) {
      /* We draw this as a 4 triangle fan centered around E. */
      GR_SET_EXPECTED_SIZE(vsize, 1);
      TRI_STRIP_BEGIN(kSetupFan, 
                      0x06UL, gc->state.vData.vSize, 
                      SSTCP_PKT3_BDDDDD);

      e = pointers;
      if (mode)
        e = *(float **)e;
      oow = 1.0f / FARRAY(e, gc->state.vData.wInfo.offset);        
      pointers = (float *)pointers + stride;
      
      ptX = FARRAY(e, gc->state.vData.vertexInfo.offset)
        *oow*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
      ptY = FARRAY(e, gc->state.vData.vertexInfo.offset+4)
        *oow*gc->state.Viewport.hheight+gc->state.Viewport.oy;

      /* Send down the original point center */
      TRI_SETF(ptX);
      TRI_SETF(ptY);

      TRI_VP_SETFS(e,oow);
      
      /* Send the 'surrounding' vertices w/ alpha == 0.0f.  To avoid
       * the vertex copying from the previous version we carry around
       * a table of offsets that we add into the original point to get
       * the new points.
       */
      {
        unsigned int v;
        static const float xAdjust[] = {  1.0f, -1.0f, -1.0f, 1.0f,  1.0f };
        static const float yAdjust[] = { -1.0f, -1.0f,  1.0f, 1.0f, -1.0f };
      
        ASSERT(ArraySize(xAdjust) == ArraySize(yAdjust));
        
        if (gc->state.vData.colorType == GR_FLOAT) {
          
          for(v = 0; v < ArraySize(xAdjust); v++) {            
            TRI_SETF(ptX + xAdjust[v]);
            TRI_SETF(ptY + yAdjust[v]);
            AA_TRI_VP_SETFS(e, oow);
          }
        }
        else {
          for(v = 0; v < ArraySize(xAdjust); v++) {            
            TRI_SETF(ptX + xAdjust[v]);
            TRI_SETF(ptY + yAdjust[v]);
            AA_TRI_VP_SETFS(e, oow);
          }
        }
        
      }
      TRI_END;
      GR_CHECK_SIZE();
    }
  }

  gc->cmdTransportInfo.cullStripHdr = tmp_cullStripHdr;
  _GlideRoot.stats.pointsDrawn+=count;
  _GlideRoot.stats.othertrisDrawn+=(count<<2);

#undef FN_NAME
} /* _grAADrawPoints */


/*-------------------------------------------------------------------
  Function: _grAADrawLineStrip
  Date: 13-Oct-97
  Implementor(s): atai
  Description:
  Draw anti-alias strip line
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grAADrawLineStrip(FxI32 mode, FxI32 ltype, FxI32 count, void *pointers)
{
#define FN_NAME "_grAADrawLineStrip"
#if 1
  /*
   * the aa line strip routine is similar to grAADrawLine routine
   * except the data set up is from the pointer array and 
   * its data layout
   */
  GR_DCL_GC;
  float           adx, ady;         /* |dX| and |dY| */
  float *v1, *v2;
  float v1a = 0.0f, v2a = 0.0f;
  FxU32 ia;
  FxU32 vsize;
  FxU32 sCount;
  FxI32 stride;
  FxU32 tmp_cullStripHdr;

  GDBG_INFO(95,"_grAADrawLineStrip(count = %d, pointers = 0x%x)\n",
            count, pointers);

  if (gc->state.vData.colorType == GR_FLOAT) {
    ia = gc->state.vData.aInfo.offset;
    v1a = 0.0f;
    v2a = 0.0f;
  } else {
    ia = gc->state.vData.pargbInfo.offset;
  }

  if (mode == 0)
    stride = gc->state.vData.vStride;
  else
    stride = sizeof(float *) / sizeof (float);

  if (ltype == GR_LINES)
    sCount = count >> 1; /* line list */
  else
    sCount = count-1;    /* strip line */

  vsize = 6 * gc->state.vData.vSize;

  tmp_cullStripHdr = gc->cmdTransportInfo.cullStripHdr;
  gc->cmdTransportInfo.cullStripHdr &= GR_CULL_MASK;

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    while (sCount--) {
      v1 = (float *)pointers;
      v2 = (float *)pointers + stride;
      if (mode) {
        v1 = *(float **)v1;
        v2 = *(float **)v2;
      }
      pointers = (float *)pointers + stride;
      if (ltype == GR_LINES)
        pointers = (float *)pointers + stride;
      
      /* draw from low Y to high Y */
      if (FARRAY(v2, gc->state.vData.vertexInfo.offset+4) < FARRAY(v1, gc->state.vData.vertexInfo.offset+4)) {
        float* tv = v1; v1 = v2; v2 = tv;
      }
      
      /* compute deltas and absolute deltas */
      adx = FARRAY(v2, gc->state.vData.vertexInfo.offset) - FARRAY(v1, gc->state.vData.vertexInfo.offset);
      ady = FARRAY(v2, gc->state.vData.vertexInfo.offset+4) - FARRAY(v1, gc->state.vData.vertexInfo.offset+4);
      if (adx < 0)
        adx = -adx;
      if (ady < 0)
        ady = -ady;
      
      if (gc->state.vData.colorType != GR_FLOAT) {
        union { float f; FxU32 u; } va;
        va.f = *(float *)((unsigned char *)v1 + ia);
        va.u &= 0x00ffffff;
        v1a = va.f;
        va.f = *(float *)((unsigned char *)v2 + ia);
        va.u &= 0x00ffffff;
        v2a = va.f;
      }
      
      GR_SET_EXPECTED_SIZE(vsize, 1);
      TRI_STRIP_BEGIN(kSetupStrip, 6, gc->state.vData.vSize, SSTCP_PKT3_BDDDDD);

      if (adx >= ady) {           /* X major line */
        FxU32 i, dataElem;
        
        /* point 1 (b) = (v2)*/
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset));
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset+4) - _GlideRoot.pool.f1);
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v2a); } else { TRI_SETF(FARRAY(v2, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 2 (a) = (v1)*/
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset));
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset+4) - _GlideRoot.pool.f1);
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v1a); } else { TRI_SETF(FARRAY(v1, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 3 (v2) = (v2)*/
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset));
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset+4));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          TRI_SETF(FARRAY(v2, i));
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 4 (v1) = (v1)*/
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset));
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset+4));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          TRI_SETF(FARRAY(v1, i));
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 5 (f) = (v2)*/
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset));
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset+4) + _GlideRoot.pool.f1);
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v2a); } else { TRI_SETF(FARRAY(v2, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 6 (a) = (v1)*/
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset));
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset+4) + _GlideRoot.pool.f1);
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v1a); } else { TRI_SETF(FARRAY(v1, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
      } else {                      /* Y major line */
        FxU32 i, dataElem;
        
        /* point 1 (b) = (v2)*/
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset) + _GlideRoot.pool.f1);
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset+4));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v2a); } else { TRI_SETF(FARRAY(v2, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 2 (a) = (v1)*/
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset) + _GlideRoot.pool.f1);
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset+4));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v1a); } else { TRI_SETF(FARRAY(v1, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 3 (v2) = (v2)*/
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset));
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset+4));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          TRI_SETF(FARRAY(v2, i));
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 4 (v1) = (v1)*/
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset));
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset+4));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          TRI_SETF(FARRAY(v1, i));
          dataElem++; i = gc->tsuDataList[dataElem];
        }

        /* point 5 (f) = (v2)*/
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset) - _GlideRoot.pool.f1);
        TRI_SETF(FARRAY(v2, gc->state.vData.vertexInfo.offset+4));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v2a); } else { TRI_SETF(FARRAY(v2, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 6 (a) = (v1)*/
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset) - _GlideRoot.pool.f1);
        TRI_SETF(FARRAY(v1, gc->state.vData.vertexInfo.offset+4));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v1a); } else { TRI_SETF(FARRAY(v1, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
      }
      _GlideRoot.stats.linesDrawn++;
      _GlideRoot.stats.othertrisDrawn+=4;
      TRI_END;
      GR_CHECK_SIZE();
    }
  }
  else {
    float oowa, oowb = 0.0f, owa, owb, tmp1, tmp2, fax, fay, fbx, fby;

    if (ltype == GR_LINE_STRIP) {
      v1 = (float *)pointers;
      if (mode) {
        v1 = *(float **)v1;
      }
      oowb = 1.0f / FARRAY(v1, gc->state.vData.wInfo.offset);        
    }
    while (sCount--) {
      v1 = (float *)pointers;
      v2 = (float *)pointers + stride;
      if (mode) {
        v1 = *(float **)v1;
        v2 = *(float **)v2;
      }
      pointers = (float *)pointers + stride;
      if (ltype == GR_LINES) {
        pointers = (float *)pointers + stride;
        owa = oowa = 1.0f / FARRAY(v1, gc->state.vData.wInfo.offset);
      }
      else
        owa = oowa = oowb;
      owb = oowb = 1.0f / FARRAY(v2, gc->state.vData.wInfo.offset);
      
      fay = tmp1 = FARRAY(v1, gc->state.vData.vertexInfo.offset+4)
        *oowa*gc->state.Viewport.hheight+gc->state.Viewport.oy;
      fby = tmp2 = FARRAY(v2, gc->state.vData.vertexInfo.offset+4)
        *oowb*gc->state.Viewport.hheight+gc->state.Viewport.oy;        

      /* draw from low Y to high Y */
      if (tmp2 < tmp1) {
        float* tv = v1; v1 = v2; v2 = tv;
        owa = oowb; owb = oowa;
        fay = tmp2;
        fby = tmp1;     
      }
      fax = FARRAY(v1, gc->state.vData.vertexInfo.offset)
        *owa*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
      fbx = FARRAY(v2, gc->state.vData.vertexInfo.offset)
        *owb*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
        
      /* compute deltas and absolute deltas */
      adx = fbx - fax;
      ady = fby - fay;
      if (adx < 0)
        adx = -adx;
      if (ady < 0)
        ady = -ady;
      /*
      if (gc->state.vData.colorType != GR_FLOAT) {
        *((FxU32 *)&v1a)=*((FxU32 *)((long)v1 + ia))&0x00ffffff;
        *((FxU32 *)&v2a)=*((FxU32 *)((long)v2 + ia))&0x00ffffff;
      }
      */
      
      GR_SET_EXPECTED_SIZE(vsize, 1);
      TRI_STRIP_BEGIN(kSetupStrip, 6, gc->state.vData.vSize, SSTCP_PKT3_BDDDDD);

      if (adx >= ady) {           /* X major line */
        
        /* point 1 (b) = (v2)*/
        TRI_SETF(fbx);
        TRI_SETF(fby - _GlideRoot.pool.f1);
        AA_TRI_VP_SETFS(v2, owb);
        
        /* point 2 (a) = (v1)*/
        TRI_SETF(fax);
        TRI_SETF(fay - _GlideRoot.pool.f1);
        AA_TRI_VP_SETFS(v1, owa);
        
        /* point 3 (v2) = (v2)*/
        TRI_SETF(fbx);
        TRI_SETF(fby);
        TRI_VP_SETFS(v2, owb);
        
        /* point 4 (v1) = (v1)*/
        TRI_SETF(fax);
        TRI_SETF(fay);
        TRI_VP_SETFS(v1, owa);

        /* point 5 (f) = (v2)*/
        TRI_SETF(fbx);
        TRI_SETF(fby + _GlideRoot.pool.f1);
        AA_TRI_VP_SETFS(v2, owb);

        /* point 6 (a) = (v1)*/
        TRI_SETF(fax);
        TRI_SETF(fay + _GlideRoot.pool.f1);
        AA_TRI_VP_SETFS(v1, owa);
        
      } else {                      /* Y major line */
        
        /* point 1 (b) = (v2)*/
        TRI_SETF(fbx + _GlideRoot.pool.f1);
        TRI_SETF(fby);
        AA_TRI_VP_SETFS(v2, owb);
        
        /* point 2 (a) = (v1)*/
        TRI_SETF(fax + _GlideRoot.pool.f1);
        TRI_SETF(fay);
        AA_TRI_VP_SETFS(v1, owa);
        
        /* point 3 (v2) = (v2)*/
        TRI_SETF(fbx);
        TRI_SETF(fby);
        TRI_VP_SETFS(v2, owb);
        
        /* point 4 (v1) = (v1)*/
        TRI_SETF(fax);
        TRI_SETF(fay);
        TRI_VP_SETFS(v1, owa);

        /* point 5 (f) = (v2)*/
        TRI_SETF(fbx - _GlideRoot.pool.f1);
        TRI_SETF(fby);
        AA_TRI_VP_SETFS(v2, owb);

        /* point 6 (a) = (v1)*/
        TRI_SETF(fax - _GlideRoot.pool.f1);
        TRI_SETF(fay);
        AA_TRI_VP_SETFS(v1, owa);
      }
      _GlideRoot.stats.linesDrawn++;
      _GlideRoot.stats.othertrisDrawn+=4;
      TRI_END;
      GR_CHECK_SIZE();
    }
  }
#endif

  gc->cmdTransportInfo.cullStripHdr = tmp_cullStripHdr;

#undef FN_NAME
} /* _grAADrawLineStrip */

/*-------------------------------------------------------------------
  Function: aaDrawArrayEdgeSense
  Date: 13-Oct-97
  Implementor(s): atai
  Description:
  Pass the line a, b. Use c to calculate edge sense.
  Create new vertices from a, b.
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
static void
aaDrawArrayEdgeSense(float *a, float *b, float *c)
{
#define FN_NAME "aaDrawArrayEdgeSense"
  GR_DCL_GC;

  float dx, dy, v2x, v2y;
  float *fa, *fb, v1a, v2a;
  FxU32 ia;
  aaEdgeSense sense;

  if (FARRAY(a, gc->state.vData.vertexInfo.offset+4) >= FARRAY(b, gc->state.vData.vertexInfo.offset+4)) {
    fa = b; fb = a;
  }
  else {
    fa = a; fb = b;
  }
  dx = FARRAY(fa, gc->state.vData.vertexInfo.offset) - FARRAY(fb, gc->state.vData.vertexInfo.offset);
  dy = FARRAY(fa, gc->state.vData.vertexInfo.offset+4) - FARRAY(fb, gc->state.vData.vertexInfo.offset+4);

  if (dx == 0.f) {
    sense = (FARRAY(c, gc->state.vData.vertexInfo.offset) > FARRAY(fa, gc->state.vData.vertexInfo.offset)) ? aaEdgeSenseLeft : aaEdgeSenseRight;
  }
  else if (dy == 0.0f) {
    sense = (FARRAY(c, gc->state.vData.vertexInfo.offset+4) > FARRAY(fa, gc->state.vData.vertexInfo.offset+4)) ? aaEdgeSenseBottom : aaEdgeSenseTop;
  }
  else {
    float dcax, dcay, dcbx, dcby, cp;
    dcax = FARRAY(fa, gc->state.vData.vertexInfo.offset) - FARRAY(c, gc->state.vData.vertexInfo.offset);
    dcay = FARRAY(fa, gc->state.vData.vertexInfo.offset+4) - FARRAY(c, gc->state.vData.vertexInfo.offset+4);
    dcbx = FARRAY(fb, gc->state.vData.vertexInfo.offset) - FARRAY(c, gc->state.vData.vertexInfo.offset);
    dcby = FARRAY(fb, gc->state.vData.vertexInfo.offset+4) - FARRAY(c, gc->state.vData.vertexInfo.offset+4);
    cp = dcax * dcby - dcay * dcbx;

    if (dx > 0.0f) {
      if (dx >= -dy)    /* X-major line */
        sense = (cp > 0.0f) ? aaEdgeSenseTop : aaEdgeSenseBottom;
      else              /* Y-major line */
        sense = (cp > 0.0f) ? aaEdgeSenseRight : aaEdgeSenseLeft;
    }
    else {
      if (dx <= dy)     /* X-major line */
        sense = (cp < 0.0f) ? aaEdgeSenseTop : aaEdgeSenseBottom;
      else              /* Y-major line */
        sense = (cp < 0.0f) ? aaEdgeSenseLeft : aaEdgeSenseRight;
    }
  }

  /* v1(a), v2(b), a, b */
  GR_SET_EXPECTED_SIZE(gc->state.vData.vSize << 2, 1);
  TRI_STRIP_BEGIN(kSetupStrip, 4, gc->state.vData.vSize, SSTCP_PKT3_BDDDDD);
  switch (sense) {
  case aaEdgeSenseTop:
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset));
    v2x = FARRAY(b, gc->state.vData.vertexInfo.offset);
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset+4) + _GlideRoot.pool.f1);
    v2y = FARRAY(b, gc->state.vData.vertexInfo.offset+4) + _GlideRoot.pool.f1;
    break;

  case aaEdgeSenseLeft:
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset) - _GlideRoot.pool.f1);
    v2x = FARRAY(b, gc->state.vData.vertexInfo.offset) - _GlideRoot.pool.f1;
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset+4));
    v2y = FARRAY(b, gc->state.vData.vertexInfo.offset+4);
    break;

  case aaEdgeSenseBottom:
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset));
    v2x = FARRAY(b, gc->state.vData.vertexInfo.offset);
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset+4) - _GlideRoot.pool.f1);
    v2y = FARRAY(b, gc->state.vData.vertexInfo.offset+4) - _GlideRoot.pool.f1;
    break;

  case aaEdgeSenseRight:
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset) + _GlideRoot.pool.f1);
    v2x = FARRAY(b, gc->state.vData.vertexInfo.offset) + _GlideRoot.pool.f1;
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset+4));
    v2y = FARRAY(b, gc->state.vData.vertexInfo.offset+4);
    break;
  }

  if (gc->state.vData.colorType == GR_FLOAT) {
    ia = gc->state.vData.aInfo.offset;
    v1a = v2a = 0.f;
  }
  else {
    union { float f; FxU32 u; } va;
    ia = gc->state.vData.pargbInfo.offset;
    va.f = *(float *)((unsigned char *)a + ia);
    va.u &= 0x00ffffff;
    v1a = va.f;
    va.f = *(float *)((unsigned char *)b + ia);
    va.u &= 0x00ffffff;
    v2a = va.f;
  }

  {
    FxU32 i, dataElem;
    dataElem = 0;
    i = gc->tsuDataList[dataElem];
    while (i != GR_DLIST_END) {
      if (i == ia) {
        TRI_SETF(v1a);
      }
      else {
        TRI_SETF(FARRAY(a, i));
      }
      dataElem++;
      i = gc->tsuDataList[dataElem];
    }
    TRI_SETF(v2x);
    dataElem = 0;
    TRI_SETF(v2y);
    i = gc->tsuDataList[dataElem];
    while (i != GR_DLIST_END) {
      if (i == ia) {
        TRI_SETF(v2a);
      }
      else {
        TRI_SETF(FARRAY(b, i));
      }
      dataElem++;
      i = gc->tsuDataList[dataElem];
    }
    dataElem = 0;
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset));
    TRI_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset+4));
    i = gc->tsuDataList[dataElem];
    while (i != GR_DLIST_END) {
      TRI_SETF(FARRAY(a, i));
      dataElem++;
      i = gc->tsuDataList[dataElem];
    }
    dataElem = 0;
    TRI_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset));
    TRI_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset+4));
    i = gc->tsuDataList[dataElem];
    while (i != GR_DLIST_END) {
      TRI_SETF(FARRAY(b, i));
      dataElem++;
      i = gc->tsuDataList[dataElem];
    }
  }
  _GlideRoot.stats.othertrisDrawn+=2;
  TRI_END;
  GR_CHECK_SIZE();
#undef FN_NAME
} /* aaCalcEdgeSense */

/*-------------------------------------------------------------------
  Function: _grAADrawTriangles
  Date: 13-Oct-97
  Implementor(s): atai
  Description:
  Draw anti-alias triangles
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grAADrawTriangles(FxI32 mode, FxI32 ttype, FxI32 count, void *pointers)
{
#define FN_NAME "_grAADrawTriangles"

  GR_DCL_GC;

  float **lPtr = (float **)pointers;
  FxI32 tCount = 3;
  FxU32 fbzModeOld;                 /* Squirrel away current fbzMode */
  FxI32 stride;
  FxI32 xindex = (gc->state.vData.vertexInfo.offset >> 2);
  FxI32 yindex = xindex + 1;

  GDBG_INFO(96, "_grAADrawTriangles (count = %d, pointers = 0x%x)\n",
            count, pointers);

  GR_FLUSH_STATE();

  if (ttype == GR_TRIANGLES)
    (*gc->curArchProcs.drawTrianglesProc)(mode, count, pointers);

  fbzModeOld = gc->state.fbi_config.fbzMode;
  gc->state.fbi_config.fbzMode &= ~(SST_ZAWRMASK);
  /* gc->state.invalid |= fbzModeBIT; */
  GR_FLUSH_STATE();

  if (mode == 0)
    stride = gc->state.vData.vStride;
  else
    stride = sizeof(float *) / sizeof (float);

  /* backfaced or zero area */
  while (tCount <= count) {
    float *a, *b, *c;

    GR_BEGIN_NOFIFOCHECK("_grAADrawTri",96);
      
    a = pointers;
    b = (float *)pointers + stride;
    c = (float *)pointers + (stride << 1);
    if (mode) {
      a = *(float **)a;
      b = *(float **)b;
      c = *(float **)c;
    }
    pointers = (float *)pointers + stride*3;

    /* move culling test to here */
    {
      float *fa = a;
      float *fb = b;
      float *fc = c;
      float dxAB, dxBC, dyAB, dyBC;
      /*
      **  Sort the vertices.
      **  Whenever the radial order is reversed (from counter-clockwise to
      **  clockwise), we need to change the area of the triangle.  Note
      **  that we know the first two elements are X & Y by looking at the
      **  grVertex structure.  
      */
      {
        int ay = *(int *)&a[(gc->state.vData.vertexInfo.offset>>2)+1];
        int by = *(int *)&b[(gc->state.vData.vertexInfo.offset>>2)+1];
        int cy = *(int *)&c[(gc->state.vData.vertexInfo.offset>>2)+1];
        int culltest = gc->state.cull_mode;
        
        if (ay < 0) ay ^= 0x7FFFFFFF;
        if (by < 0) by ^= 0x7FFFFFFF;
        if (cy < 0) cy ^= 0x7FFFFFFF;
        
        if (ay < by) {
          if (by > cy) {    /* acb */
            if (ay < cy) {
              fa = a;
              fb = c;
              fc = b;
              culltest ^= 1;
            } else {                /* cab */
              fa = c;
              fb = a;
              fc = b;
            }
            /* else it's already sorted */
          }
        } else {
          if (by < cy) {    /* bac */
            if (ay < cy) {
              fa = b;
              fb = a;
              fc = c;
              culltest ^= 1;
            } else {                /* bca */
              fa = b;
              fb = c;
              fc = a;
            }
          } else {          /* cba */
            fa = c;
            fb = b;
            fc = a;
            culltest ^= 1;
          }
        }
        
        /* Compute Area */
        dxAB = fa[xindex] - fb[xindex];
        dxBC = fb[xindex] - fc[xindex];
        
        dyAB = fa[yindex] - fb[yindex];
        dyBC = fb[yindex] - fc[yindex];
        
        /* Stash the area in the float pool for easy access */
        _GlideRoot.pool.temp1.f = dxAB * dyBC - dxBC * dyAB;
        
#define FloatVal(__f) (((__f) < 786432.875) ? (__f) : ((__f) - 786432.875))
        /* Zero-area triangles are BAD!! */
        if ((_GlideRoot.pool.temp1.i & 0x7FFFFFFF) == 0) {
          GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                    FloatVal(fa[xindex]), FloatVal(fa[yindex]), 
                    FloatVal(fb[xindex]), FloatVal(fb[yindex]), 
                    FloatVal(fc[xindex]), FloatVal(fc[yindex]), 
                    _GlideRoot.pool.temp1.f, gc->state.cull_mode, culltest);

          goto done;
        }
    
        /* Backface culling, use sign bit as test */
        if ((gc->state.cull_mode != GR_CULL_DISABLE) &&
            ((_GlideRoot.pool.temp1.i ^ (culltest << 31)) >= 0)) {
      
          GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                    FloatVal(fa[xindex]), FloatVal(fa[yindex]), 
                    FloatVal(fb[xindex]), FloatVal(fb[yindex]), 
                    FloatVal(fc[xindex]), FloatVal(fc[yindex]), 
                    _GlideRoot.pool.temp1.f, gc->state.cull_mode, culltest);
      
          goto done;
        }
      }
    } /* end culling test */
      
    aaDrawArrayEdgeSense(a, b, c);
      
    aaDrawArrayEdgeSense(b, c, a);
      
    aaDrawArrayEdgeSense(c, a, b);
      
  done:
    lPtr += 3;
    tCount+=3;
    _GlideRoot.stats.trisProcessed++;
  }


  gc->state.fbi_config.fbzMode = fbzModeOld;
  gc->state.invalid |= fbzModeBIT;
  GR_FLUSH_STATE();

#undef FN_NAME
} /* _grAADrawTriangles */


/*-------------------------------------------------------------------
  Function: aaVpDrawArrayEdgeSense
  Date: 11-Dec-97
  Implementor(s): atai
  Description:
  Pass the line a, b. Use c to calculate edge sense. (clip space)
  Create new vertices from a, b.
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
static void
aaVpDrawArrayEdgeSense(float *a, float *b, float *c, float oowa, float oowb)
{
#define FN_NAME "aaVpDrawArrayEdgeSense"
  GR_DCL_GC;

  float dx, dy, v2x, v2y;
  float *fa, *fb, v1a, v2a;
  float fax, fay, fbx, fby;
  FxU32 ia;
  aaEdgeSense sense;

  if (FARRAY(a, 4) >= FARRAY(b, 4)) {
    fa = b; fb = a;
  }
  else {
    fa = a; fb = b;
  }
  dx = FARRAY(fa, 0) - FARRAY(fb, 0);
  dy = FARRAY(fa, 4) - FARRAY(fb, 4);

  if (dx == 0.f) {
    sense = (FARRAY(c, 0) > FARRAY(fa, 0)) ? aaEdgeSenseLeft : aaEdgeSenseRight;
  }
  else if (dy == 0.0f) {
    sense = (FARRAY(c, 4) > FARRAY(fa, 4)) ? aaEdgeSenseBottom : aaEdgeSenseTop;
  }
  else {
    float dcax, dcay, dcbx, dcby, cp;
    dcax = FARRAY(fa, 0) - FARRAY(c, 0);
    dcay = FARRAY(fa, 4) - FARRAY(c, 4);
    dcbx = FARRAY(fb, 0) - FARRAY(c, 0);
    dcby = FARRAY(fb, 4) - FARRAY(c, 4);
    cp = dcax * dcby - dcay * dcbx;

    if (dx > 0.0f) {
      if (dx >= -dy)    /* X-major line */
        sense = (cp > 0.0f) ? aaEdgeSenseTop : aaEdgeSenseBottom;
      else              /* Y-major line */
        sense = (cp > 0.0f) ? aaEdgeSenseRight : aaEdgeSenseLeft;
    }
    else {
      if (dx <= dy)     /* X-major line */
        sense = (cp < 0.0f) ? aaEdgeSenseTop : aaEdgeSenseBottom;
      else              /* Y-major line */
        sense = (cp < 0.0f) ? aaEdgeSenseLeft : aaEdgeSenseRight;
    }
  }

  /* v1(a), v2(b), a, b */
  GR_SET_EXPECTED_SIZE(gc->state.vData.vSize << 2, 1);
  TRI_STRIP_BEGIN(kSetupStrip, 4, gc->state.vData.vSize, SSTCP_PKT3_BDDDDD);
  fax = FARRAY(a, 0)*oowa*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
  fay = FARRAY(a, 4)*oowa*gc->state.Viewport.hheight+gc->state.Viewport.oy;
  fbx = FARRAY(b, 0)*oowb*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
  fby = FARRAY(b, 4)*oowb*gc->state.Viewport.hheight+gc->state.Viewport.oy;

  switch (sense) {
  case aaEdgeSenseTop:
    TRI_SETF(fax);
    v2x = fbx;
    TRI_SETF(fay + _GlideRoot.pool.f1);
    v2y = fby + _GlideRoot.pool.f1;
    break;

  case aaEdgeSenseLeft:
    TRI_SETF(fax - _GlideRoot.pool.f1);
    v2x = fbx - _GlideRoot.pool.f1;
    TRI_SETF(fay);
    v2y = fby;
    break;

  case aaEdgeSenseBottom:
    TRI_SETF(fax);
    v2x = fbx;
    TRI_SETF(fay - _GlideRoot.pool.f1);
    v2y = fby - _GlideRoot.pool.f1;
    break;

  case aaEdgeSenseRight:
    TRI_SETF(fax + _GlideRoot.pool.f1);
    v2x = fbx + _GlideRoot.pool.f1;
    TRI_SETF(fay);
    v2y = fby;
    break;
  }

  if (gc->state.vData.colorType == GR_FLOAT) {
    ia = gc->state.vData.aInfo.offset;
    v1a = v2a = 0.f;
  }
  else {
    union { float f; FxU32 u; } va;
    ia = gc->state.vData.pargbInfo.offset;
    va.f = *(float *)((unsigned char *)a + ia);
    va.u &= 0x00ffffff;
    v1a = va.f;
    va.f = *(float *)((unsigned char *)b + ia);
    va.u &= 0x00ffffff;
    v2a = va.f;
  }

  {

    AA_TRI_VP_SETFS(a,oowa);

    TRI_SETF(v2x);
    TRI_SETF(v2y);

    AA_TRI_VP_SETFS(b,oowb);

    TRI_SETF(fax);
    TRI_SETF(fay);
    TRI_VP_SETFS(a,oowa);

    TRI_SETF(fbx);
    TRI_SETF(fby);
    TRI_VP_SETFS(b,oowb);

  }
  _GlideRoot.stats.othertrisDrawn+=2;
  TRI_END;
  GR_CHECK_SIZE();
#undef FN_NAME
}

/*-------------------------------------------------------------------
  Function: _grAAVpDrawTriangles
  Date: 11-Dec-97
  Implementor(s): atai
  Description:
  Draw anti-alias triangles (clip coords)
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grAAVpDrawTriangles(FxI32 mode, FxI32 ttype, FxI32 count, void *pointers)
{
#define FN_NAME "_grAAVpDrawTriangles"

  GR_DCL_GC;

  float **lPtr = (float **)pointers;
  FxI32 tCount = 3;
  FxU32 fbzModeOld;                 /* Squirrel away current fbzMode */
  FxI32 stride;
  FxI32 xindex = (gc->state.vData.vertexInfo.offset >> 2);
  FxI32 yindex = xindex + 1;

  GDBG_INFO(96, "_grAADrawTriangles (count = %d, pointers = 0x%x)\n",
            count, pointers);

  GR_FLUSH_STATE();

  if (ttype == GR_TRIANGLES)
    (*gc->curArchProcs.drawTrianglesProc)(mode, count, pointers);

  fbzModeOld = gc->state.fbi_config.fbzMode;
  gc->state.fbi_config.fbzMode &= ~(SST_ZAWRMASK);
  /* gc->state.invalid |= fbzModeBIT; */
  GR_FLUSH_STATE();

  if (mode == 0)
    stride = gc->state.vData.vStride;
  else
    stride = sizeof(float *) / sizeof (float);

  /* backfaced or zero area */
  while (tCount <= count) {
    float *a, *b, *c;
    float oowa, oowb, oowc;

    GR_BEGIN_NOFIFOCHECK("_grAADrawTri",96);
      
    a = pointers;
    b = (float *)pointers + stride;
    c = b + stride;
    if (mode) {
      a = *(float **)a;
      b = *(float **)b;
      c = *(float **)c;
    }
    pointers = (float *)pointers + stride*3;
    oowa = 1.0f / FARRAY(a, gc->state.vData.wInfo.offset);        
    oowb = 1.0f / FARRAY(b, gc->state.vData.wInfo.offset);        
    oowc = 1.0f / FARRAY(c, gc->state.vData.wInfo.offset);        


    /* move culling test to here */
    {
      float *fa = a;
      float *fb = b;
      float *fc = c;
      float dxAB, dxBC, dyAB, dyBC;
      /*
      **  Sort the vertices.
      **  Whenever the radial order is reversed (from counter-clockwise to
      **  clockwise), we need to change the area of the triangle.  Note
      **  that we know the first two elements are X & Y by looking at the
      **  grVertex structure.  
      */
      {
        float fay = a[yindex]*oowa*gc->state.Viewport.hheight*gc->state.Viewport.oy;
        float fby = b[yindex]*oowb*gc->state.Viewport.hheight*gc->state.Viewport.oy;
        float fcy = c[yindex]*oowc*gc->state.Viewport.hheight*gc->state.Viewport.oy;
        union { float f; int i; } ay;
        union { float f; int i; } by;
        union { float f; int i; } cy;
        int culltest = gc->state.cull_mode;
        
        ay.f = fay;
        by.f = fby;
        cy.f = fcy;
        
        if (ay.i < 0) ay.i ^= 0x7FFFFFFF;
        if (by.i < 0) by.i ^= 0x7FFFFFFF;
        if (cy.i < 0) cy.i ^= 0x7FFFFFFF;
        
        if (ay.i < by.i) {
          if (by.i > cy.i) {    /* acb */
            if (ay.i < cy.i) {
              fa = a;
              fb = c;
              fc = b;
              
              culltest ^= 1;
            } else {                /* cab */
              fa = c;
              fb = a;
              fc = b;
            }
            /* else it's already sorted */
          }
        } else {
          if (by.i < cy.i) {    /* bac */
            if (ay.i < cy.i) {
              fa = b;
              fb = a;
              fc = c;
              culltest ^= 1;
            } else {                /* bca */
              fa = b;
              fb = c;
              fc = a;
            }
          } else {          /* cba */
            fa = c;
            fb = b;
            fc = a;
            culltest ^= 1;
          }
        }
        
        /* Compute Area */
        dxAB = fa[xindex] - fb[xindex];
        dxBC = fb[xindex] - fc[xindex];
        
        dyAB = fa[yindex] - fb[yindex];
        dyBC = fb[yindex] - fc[yindex];
        
        /* Stash the area in the float pool for easy access */
        _GlideRoot.pool.temp1.f = dxAB * dyBC - dxBC * dyAB;
        
#define FloatVal(__f) (((__f) < 786432.875) ? (__f) : ((__f) - 786432.875))
        /* Zero-area triangles are BAD!! */
        if ((_GlideRoot.pool.temp1.i & 0x7FFFFFFF) == 0) {
          GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                    FloatVal(fa[xindex]), FloatVal(fa[yindex]), 
                    FloatVal(fb[xindex]), FloatVal(fb[yindex]), 
                    FloatVal(fc[xindex]), FloatVal(fc[yindex]), 
                    _GlideRoot.pool.temp1.f, gc->state.cull_mode, culltest);

          goto done;
        }
    
        /* Backface culling, use sign bit as test */
        if ((gc->state.cull_mode != GR_CULL_DISABLE) &&
            ((_GlideRoot.pool.temp1.i ^ (culltest << 31)) >= 0)) {
      
          GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                    FloatVal(fa[xindex]), FloatVal(fa[yindex]), 
                    FloatVal(fb[xindex]), FloatVal(fb[yindex]), 
                    FloatVal(fc[xindex]), FloatVal(fc[yindex]), 
                    _GlideRoot.pool.temp1.f, gc->state.cull_mode, culltest);
      
          goto done;
        }
      }
    } /* end culling test */
      
    aaVpDrawArrayEdgeSense(a, b, c, oowa, oowb);
      
    aaVpDrawArrayEdgeSense(b, c, a, oowb, oowc);
      
    aaVpDrawArrayEdgeSense(c, a, b, oowc, oowa);
      
  done:
    lPtr += 3;
    tCount+=3;
    _GlideRoot.stats.trisProcessed++;
  }

  gc->state.fbi_config.fbzMode = fbzModeOld;
  gc->state.invalid |= fbzModeBIT;
  GR_FLUSH_STATE();

#undef FN_NAME
} /* _grAAVpDrawTriangles */

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
  FxU32 sCount = count;
  float *v[3];
  FxBool flip = FXFALSE;
  FxU32 fbzModeOld;                 /* Squirrel away current fbzMode */
  FxI32 stride;

  if (sCount <= 2) return;

  GR_FLUSH_STATE();
  (*gc->curArchProcs.drawVertexList)(SSTCP_PKT3_BDDDDD, type, mode, count, pointers);

  fbzModeOld = gc->state.fbi_config.fbzMode;
  gc->state.fbi_config.fbzMode &= ~(SST_ZAWRMASK);
  /* gc->state.invalid |= fbzModeBIT; */
  GR_FLUSH_STATE();
  if (mode == 0)
    stride = gc->state.vData.vStride;
  else
    stride = sizeof(float *) / sizeof (float);

  sCount-=2;
  if (type == kSetupFan) {
    v[0] = (mode == 0) ? pointers : *(float **)pointers;
    while (sCount--) {
      pointers = (float *)pointers + stride;
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
      pointers = (float *)pointers + stride;
      flip = ~flip;
    }
    flip = ~flip;
  }

  gc->state.fbi_config.fbzMode = fbzModeOld;
  gc->state.invalid |= fbzModeBIT;
  GR_FLUSH_STATE();

#undef FN_NAME
} /* _grAADrawVertexList */


