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
** 69    12/03/98 11:27p Dow
** Code 'cleanup' heç
** 
** 68    11/02/98 11:36a Jeske
** splash speckle mini-workaround for grAADrawTriangle()
** 
** 67    10/20/98 2:17p Jeske
** added diag specific aa triangle draw
** 
** 66    5/18/98 3:20p Peter
** crybaby warning about alpha not being enabled for aa primatives
** 
** 65    4/23/98 4:46p Peter
** John's aa line stuff
** 
** 64    3/28/98 11:24a Dow
** itwoç
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
 * fixed glide3 effage
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

/*
** -----------------------------------------------------------------------
**
** Antialiasing rendering functions
**
** -----------------------------------------------------------------------
*/

#ifndef GLIDE3_ALPHA
/*---------------------------------------------------------------------------
** grAADrawPoint
**
**   |-w-|
**   A---B -
**   |\ /| |
**   | E | h
**   |/ \| |
**   D --C -
**   w = h = 2pixels
*/
GR_ENTRY(grAADrawPoint, void, (const GrVertex *e)) 
{
#define FN_NAME "grAADrawPoint"
#ifdef GLIDE3
  _grAADrawPoints(GR_VTX_PTR_ARRAY, 1, (void *)&e);
#else
  GR_DCL_GC;
  GrCullMode_t cullSave;
  
#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  const FxU32 vertexParamOffset = _GlideRoot.curVertexSize;
#endif /* GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP */

  GDBG_INFO(94,"grAADrawPoint(0x%x)\n",e);

  /* turn off culling so triangles unconditionally draw */
  cullSave = gc->state.cull_mode;
  gc->state.cull_mode = GR_CULL_DISABLE;

  GR_CHECK_COMPATABILITY(FN_NAME,
                         ((gc->cmdTransportInfo.paramMask & (SST_SETUP_A << SSTCP_PKT3_PMASK_SHIFT)) == 0),
                         "Antialiasing require enabling alpha");

#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  /* We draw this as a 4 triangle fan centered around E. */
  GR_SET_EXPECTED_SIZE(vertexParamOffset * 6, 1);
  TRI_STRIP_BEGIN(kSetupFan, 
                  0x06UL, vertexParamOffset, 
                  SSTCP_PKT3_BDDDDD);

  {
    const float ptX = e->x;
    const float ptY = e->y;
    const float* const fp = (const float* const)e;

    /* Send down the original point center */
    TRI_SETF(ptX);
    TRI_SETF(ptY);

    {
      const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
      if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0) {
        FxU32 packedColor = 0x00;
        
        if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
          packedColor = (RGBA_COMP_CLAMP(FARRAY(fp, (GR_VERTEX_B_OFFSET << 2)), B) |
                         RGBA_COMP_CLAMP(FARRAY(fp, (GR_VERTEX_G_OFFSET << 2)), G) |
                         RGBA_COMP_CLAMP(FARRAY(fp, (GR_VERTEX_R_OFFSET << 2)), R));
          
          dataList++;
        }
        
        packedColor |= RGBA_COMP_CLAMP(FARRAY(fp, (GR_VERTEX_A_OFFSET << 2)), A);
        TRI_SET(packedColor);
      } else {
        TRI_SETF(FARRAY(fp, (GR_VERTEX_A_OFFSET << 2)));
      }
      dataList++;
#endif /* GLIDE_PACKED_RGB */

#if GLIDE_FP_CLAMP_TEX
      while(*dataList != 0) {
        TRI_SETF(FARRAY(fp, *dataList));
        dataList++;
      }

      dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
      
      while(*dataList != 0) {
        TRI_SETF_CLAMP(FARRAY(fp, *dataList));
        dataList++;
      }
    }

    /* Send the 'surrounding' vertices w/ alpha == 0.0f.  To avoid
     * the vertex copying from the previous version we carry around
     * a table of offsets that we add into the original point to get
     * the new points.
     */
    {
      int v;
      static const float xAdjust[] = {  1.0f, -1.0f, -1.0f, 1.0f,  1.0f };
      static const float yAdjust[] = { -1.0f, -1.0f,  1.0f, 1.0f, -1.0f };
         
      ASSERT(ArraySize(xAdjust) == ArraySize(yAdjust));
         
      for(v = 0; v < ArraySize(xAdjust); v++) {
        TRI_SETF(ptX + xAdjust[v]);
        TRI_SETF(ptY + yAdjust[v]);
           
        {
          const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
          if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0) {
            FxU32 packedColor = 0x00;
            
            if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
              packedColor = (RGBA_COMP_CLAMP(FARRAY(fp, (GR_VERTEX_B_OFFSET << 2)), B) |
                             RGBA_COMP_CLAMP(FARRAY(fp, (GR_VERTEX_G_OFFSET << 2)), G) |
                             RGBA_COMP_CLAMP(FARRAY(fp, (GR_VERTEX_R_OFFSET << 2)), R));
              
              dataList++;
            }
            
            TRI_SET(packedColor);
          } else {
            TRI_SETF(0.0f);
          }
#else /* !GLIDE_PACKED_RGB */
          /* Parameters up to alpha.
           *
           * NB: It is up to the user to correctly set this up
           * so that the alpha is sent.
           */
          while(*dataList != (GR_VERTEX_A_OFFSET << 2)) {
            TRI_SETF(FARRAY(fp, *dataList));
            dataList++;
          }
          
          /* Send constant Alpha == 0.0f */
          TRI_SETF(0.0f);
#endif /* !GLIDE_PACKED_RGB */

          /* Skip the actual alpha value in the data list */
          dataList++;

#if GLIDE_FP_CLAMP_TEX
          while(*dataList != 0) {
            TRI_SETF(FARRAY(fp, *dataList));
            dataList++;
          }

          dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
          
          /* Anything else in the dataList */
          while(*dataList != 0) {                  
            TRI_SETF_CLAMP(FARRAY(fp, *dataList));
            dataList++;
          }
        }
      }
    }
  }
  
  TRI_END;
  GR_CHECK_SIZE();
#else
  {
    GrVertex a, b;      

    /*
    ** make corners of a square
    */
    a = 
    b = *e;
      
    a.tmuvtx[0].oow =
    b.tmuvtx[0].oow = _GlideRoot.pool.f1;
      
    a.x -= _GlideRoot.pool.f1;
    a.y -= _GlideRoot.pool.f1;

    b.x += _GlideRoot.pool.f1;
    b.y -= _GlideRoot.pool.f1;

    a.a =  
      b.a =  _GlideRoot.pool.f0;
      
    grDrawTriangle(&a, &b, e);    /* A B E */
      
    b.x -= 2.0F;                  /* compute point D */
    b.y += 2.0F;
    grDrawTriangle(&a, e, &b);    /* A E D */
      
    a.x += 2.0F;                  /* compute point C */
    a.y += 2.0F;
    grDrawTriangle(&b, e, &a);    /* D E C */
      
    b.x += 2.0F;
    b.y -= 2.0F;
    grDrawTriangle(&a, e, &b);    /* C E B */
  }
#endif /* GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP */
  
  /* Restore culling mode */
  gc->state.cull_mode = cullSave;
#endif
#undef FN_NAME
} /* grAADrawPoint */
#endif /* !GLIDE3_ALPHA */

#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
static void
grAADrawStrip(const FxU32 cullMode, const float* const vectorList[], const FxU32 vectorNum)
{
#define FN_NAME "grAADrawStrip"
  const FxU32 vertexParamOffset = _GlideRoot.curVertexSize;
   
  GR_BEGIN_NOFIFOCHECK(FN_NAME, gc->myLevel);
  GDBG_INFO_MORE(gc->myLevel, "(): 0x%X\n", vectorNum);

  GR_FLUSH_STATE();

  GR_SET_EXPECTED_SIZE(vertexParamOffset * vectorNum, 1);
  TRI_PACKET_BEGIN(cullMode, gc->cmdTransportInfo.paramMask,
                   vectorNum, vertexParamOffset, 
                   SSTCP_PKT3_BDDDDD);
  {
    FxU32 v;

    for(v = 0; v < vectorNum; v++) {
      const float* const vector = vectorList[v];
         
      TRI_SETF(vector[GR_VERTEX_X_OFFSET]);
      TRI_SETF(vector[GR_VERTEX_Y_OFFSET]);

      {
        const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
        if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0) {
          FxU32 packedColor = 0x00;
          
          if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
            packedColor = (RGBA_COMP_CLAMP(FARRAY(vector, (GR_VERTEX_B_OFFSET << 2)), B) |
                           RGBA_COMP_CLAMP(FARRAY(vector, (GR_VERTEX_G_OFFSET << 2)), G) |
                           RGBA_COMP_CLAMP(FARRAY(vector, (GR_VERTEX_R_OFFSET << 2)), R));
            
            dataList++;
          }
          
          packedColor |= RGBA_COMP_CLAMP(FARRAY(vector, (GR_VERTEX_A_OFFSET << 2)), A);
          TRI_SET(packedColor);
        } else {
          TRI_SETF(FARRAY(vector, (GR_VERTEX_A_OFFSET << 2)));
        }
        dataList++;
#endif /* GLIDE_PACKED_RGB */
        
#if GLIDE_FP_CLAMP_TEX
        while(*dataList != 0) {
          TRI_SETF(FARRAY(vector, *dataList));
          dataList++;
        }

        dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
        
        while(*dataList != 0) {
          TRI_SETF_CLAMP(FARRAY(vector, *dataList));
          dataList++;
        }
      }
    }
  }
  TRI_END;
  GR_CHECK_SIZE();
#undef FN_NAME
}
#else /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
/*
**  grAADrawEdgeTriangle
*/
static void
grAADrawEdgeTriangle(float *va, float *vb, float *vc)
{
  float
    *fa = va,
    *fb = vb,
    *fc = vc,
    dpAB, dpBC,
    dpdx, dpdy,                 /* Change in parameter WRT x & y */
    area;
  float
    ooa, dxAB, dxBC, dyAB, dyBC;
  float negateArea = -1.f;
 
  GR_BEGIN_NOFIFOCHECK("grAADrawEdgeTriangle",gc->myLevel);
  GDBG_INFO_MORE(gc->myLevel,"()\n");
  GR_FLUSH_STATE();

  /*
  **  Sort the vertices.
  **  Whenever the radial order is reversed (from counter-clockwise to
  **  clockwise), we need to change the area of the triangle.  Note
  **  that we know the first two elements are X & Y by looking at the
  **  grVertex structure.  
  */
  if (va[1] < vb[1]) {
    if (vb[1] > vc[1]) {        /* acb */
      if (va[1] < vc[1]) {
        fa = va;
        fb = vc;
        fc = vb;
        negateArea *= -1.f;
      } else {                  /* cab */
        fa = vc;
        fb = va;
        fc = vb;
      }
      /* else it's already sorted */
    }
  } else {
    if (vb[1] < vc[1]) {        /* bac */
      if (va[1] < vc[1]) {
        fa = vb;
        fb = va;
        fc = vc;
        negateArea *= -1.f;
      } else {                  /* bca */
        fa = vb;
        fb = vc;
        fc = va;
      }
    } else {                    /* cba */
      fa = vc;
      fb = vb;
      fc = va;
      negateArea *= -1.f;
    }
  }

  /* Compute Area */
  dxAB = fa[0] - fb[0];
  dxBC = fb[0] - fc[0];

  dyAB = fa[1] - fb[1];
  dyBC = fb[1] - fc[1];

  area = dxAB * dyBC - dxBC * dyAB;

  /* Zero-area triangles are BAD!! */
  if (area == 0.0f) goto all_done;

  ooa = 1.0f / area;

  /*
  ** Divide the deltas by the area for gradient calculation.
  */
  dxBC *= ooa;
  dyAB *= ooa;
  dxAB *= ooa;
  dyBC *= ooa;

  GR_SET_EXPECTED_SIZE(((10 + (_GlideRoot.paramCount - 1)) << 2), 
                       10 + (_GlideRoot.paramCount - 1));

  /* write out X & Y  for vertex A */
  GR_SETF(BROADCAST_ID, hw, FvA.x, fa[0]);
  GR_SETF(BROADCAST_ID, hw, FvA.y, fa[1]);

  /* write out X & Y  for vertex B */
  GR_SETF(BROADCAST_ID, hw, FvB.x, fb[0]);
  GR_SETF(BROADCAST_ID, hw, FvB.y, fb[1]);

  /* write out X & Y  for vertex C */
  GR_SETF(BROADCAST_ID, hw, FvC.x, fc[0]);
  GR_SETF(BROADCAST_ID, hw, FvC.y, fc[1]);

  dpAB = (fa[GR_VERTEX_A_OFFSET] - fb[GR_VERTEX_A_OFFSET]);
  dpBC = (fb[GR_VERTEX_A_OFFSET] - fc[GR_VERTEX_A_OFFSET]);
  dpdx = dpAB * dyBC - dpBC * dyAB;
  dpdy = dpBC * dxAB - dpAB * dxBC;

  /* Write the alpha data */
  GR_SETF(BROADCAST_ID, hw, Fa, fa[GR_VERTEX_A_OFFSET]);
  GR_SETF(BROADCAST_ID, hw, Fdadx , dpdx);
  GR_SETF(BROADCAST_ID, hw, Fdady , dpdy);

  {
    const struct dataList_s* dlp = gc->regDataList; 
    int i = dlp->i;
    
    /* We already put the alpha out, and this is guaranteed to
     * be included in the dataList via convention.
     */
    while (i != (GR_VERTEX_A_OFFSET << 2)) {
      GR_SETF_INDEX(BROADCAST_ID, hw, ((FxU32*)dlp->addr - (FxU32*)hw), FARRAY(fa, i));
      dlp++;
      i = dlp->i;
    }
    
    dlp++;
    i = dlp->i;
    
    while (i != 0) {
      GR_SETF_INDEX(BROADCAST_ID, hw, ((FxU32*)dlp->addr - (FxU32*)hw), FARRAY(fa, i));
      dlp++;
      i = dlp->i;
    }
    
    /* Draw the triangle by writing the area to the triangleCMD register */
    GR_SETF(BROADCAST_ID, hw, FtriangleCMD, area);
  }

all_done:
  GR_END();
} /* grAADrawEdgeTriangle */
#endif  /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */

#ifndef GLIDE3_ALPHA
#if (GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP)
#define FUCAST( X ) (*(FxU32*)&(X))
GR_ENTRY(grAADrawLine, void, (const GrVertex *v1, const GrVertex *v2)) 
{
#define FN_NAME "grAADrawLine"
    GR_DCL_GC;
    GrVertex *_v1, *_v2;
    const FxU32 vertexParamOffset = _GlideRoot.curVertexSize;  
    const int* datalist;
    float xoff, yoff;
    float alpha;
    float dx, dy;  /* delta X and Y */
    float adx, ady;/* |dX| and |dY| */

  GR_CHECK_COMPATABILITY(FN_NAME,
                         ((gc->cmdTransportInfo.paramMask & (SST_SETUP_A << SSTCP_PKT3_PMASK_SHIFT)) == 0),
                         "Antialiasing require enabling alpha");

    GR_SET_EXPECTED_SIZE( vertexParamOffset * 6, 1 );
    TRI_PACKET_BEGIN( 0, 
                      gc->cmdTransportInfo.paramMask,
                      6, 
                      vertexParamOffset, 
                      SSTCP_PKT3_BDDDDD );
    
    /* y sort*/
    if (v2->y < v1->y) {
        _v1 = (void*) v2;
        _v2 = (void*) v1;
    } else {
        _v1 = (void*) v1;
        _v2 = (void*) v2;
    }

#define v1 _v1
#define v2 _v2
    /* compute deltas and absolute deltas */
    
    dx  = v2->x - v1->x;
    dy  = v2->y - v1->y;
    
    adx = (float)(FUCAST( dx ) & 0x7fffffff);
    ady = (float)(FUCAST( dy ) & 0x7fffffff);
    
    if ( adx >= ady ) {
        xoff = 0.0f;
        yoff = 1.0f;
    } else {
        xoff = 1.0f;
        yoff = 0.0f;
    }
    
    /*  A---B  Alpha = 0.0
        | \ |  
        V1-V2  Alpha = 1.0 ( user )
        | \ |  
        E---F  Alpha = 0.0
        */
    
    /* B Vertex */
    TRI_SETF( v2->x + xoff );
    TRI_SETF( v2->y - yoff );

    alpha = v2->a;
    v2->a = 0.0f;
    datalist = gc->tsuDataList;

    while( *datalist != 0 ) {
        TRI_SETF(FARRAY(v2, *datalist));
        datalist++;
    }

    v2->a = alpha;

    /* A Vertex */
    datalist = gc->tsuDataList;

    TRI_SETF( v1->x + xoff );
    TRI_SETF( v1->y - yoff );

    alpha = v1->a;
    v1->a = 0.0f;
    datalist = gc->tsuDataList;

    while( *datalist != 0 ) {
        TRI_SETF(FARRAY(v1, *datalist));
        datalist++;
    }

    v1->a = alpha;
      
      /* V2 Vertex */
    datalist = gc->tsuDataList;

    TRI_SETF( v2->x );
    TRI_SETF( v2->y );
      
    while( *datalist != 0 ) {
        TRI_SETF(FARRAY(v2, *datalist));
        datalist++;
    }
      
    /* V1 Vertex */
    datalist = gc->tsuDataList;

    TRI_SETF( v1->x );
    TRI_SETF( v1->y );
      
    while( *datalist != 0 ) {
        TRI_SETF(FARRAY(v1, *datalist));
        datalist++;
    }
      
    /* F Vertex */
    datalist = gc->tsuDataList;

    TRI_SETF( v2->x - xoff );
    TRI_SETF( v2->y + yoff );

    alpha = v2->a;
    v2->a = 0.0f;
    datalist = gc->tsuDataList;

    while( *datalist != 0 ) {
        TRI_SETF(FARRAY(v2, *datalist));
        datalist++;
    }

    v2->a = alpha;

    /* E Vertex */
    datalist = gc->tsuDataList;

    TRI_SETF( v1->x - xoff );
    TRI_SETF( v1->y + yoff );

    alpha = v1->a;
    v1->a = 0.0f;
    datalist = gc->tsuDataList;


    while( *datalist != 0 ) {
        TRI_SETF(FARRAY(v1, *datalist));
        datalist++;
    }
    v1->a = alpha;      
#undef v1
#undef v2

    TRI_END;
    GR_CHECK_SIZE();
#undef FN_NAME
}
#else /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
/*---------------------------------------------------------------------------
** grAADrawLine
*/
GR_ENTRY(grAADrawLine, void, (const GrVertex *v1, const GrVertex *v2)) 
{
#ifdef GLIDE3
  float *vlist[2];
  vlist[0] = (float *)v1;
  vlist[1] = (float *)v2;
  _grAADrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINE_STRIP, 2, vlist);
#else
  float           dx, dy;           /* delta X and Y */
  float           adx, ady;         /* |dX| and |dY| */
  GrVertex        a, b, e, f;       /* four vertices outlining the line */
  GR_DCL_GC;

  /* turn off culling so triangles unconditionally draw.
   *
   * NB: On cvg we control this via the command packet.
   */
  GrCullMode_t cullSave = gc->state.cull_mode;
  gc->state.cull_mode = GR_CULL_DISABLE;
  
  GDBG_INFO(95, "grAADrawLine (%f %f) -> (%f %f)\n",
            v1->x, v1->y, v2->x, v2->y);

  GR_FLUSH_STATE();

  /* draw from low Y to high Y */
  if (v2->y < v1->y) {
    const GrVertex* tv = v1; v1 = v2; v2 = tv;
  }
  
  /* compute deltas and absolute deltas */
  dx = adx = v2->x - v1->x;
  dy = ady = v2->y - v1->y;
  if (adx < 0)
    adx = -adx;
  if (ady < 0)
    ady = -ady;
  
  a = *v1;
  e = *v1;
  b = *v2;
  f = *v2;

  if (adx >= ady) {           /* X major line */
    a.y -= _GlideRoot.pool.f1;
    a.a = 0.F;
    b.y -= _GlideRoot.pool.f1;
    b.a = 0.F;
    e.y += _GlideRoot.pool.f1;
    e.a = 0.F;
    f.y += _GlideRoot.pool.f1;
    f.a = 0.F;

    grDrawTriangle(&a, v2, &b);
    grDrawTriangle(&a, v2, v1);
    grDrawTriangle(v1, &f, &e);
    grDrawTriangle(v1, v2, &f);
  } else {                      /* Y major line */
    a.x += _GlideRoot.pool.f1;
    a.a = 0.F;
    b.x += _GlideRoot.pool.f1;
    b.a = 0.F;
    e.x -= _GlideRoot.pool.f1;
    e.a = 0.F;
    f.x -= _GlideRoot.pool.f1;
    f.a = 0.F;

    grDrawTriangle(&a, &b, v2);
    grDrawTriangle(v1, &a, v2);
    grDrawTriangle(v1, &f, &e);
    grDrawTriangle(v1, v2, &f);
  }

  gc->state.cull_mode = cullSave;
#endif /* GLIDE3 */
} /* grAADrawLine */
#endif /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
#endif /* !GLIDE3_ALPHA */

typedef enum {
  aaEdgeSenseTop,
  aaEdgeSenseLeft,
  aaEdgeSenseBottom,
  aaEdgeSenseRight
} aaEdgeSense;

/*
**  grAADrawTriEdge
*/
static void
grAADrawTriEdge(const GrVertex *a, const GrVertex *b, aaEdgeSense sense)
{
  GrVertex c = *a, d = *b;

  c.a = 0.f;
  d.a = 0.f;

  switch (sense) {
  case aaEdgeSenseTop:
    c.y += _GlideRoot.pool.f1;
    d.y += _GlideRoot.pool.f1;
    break;

  case aaEdgeSenseLeft:
    c.x -= _GlideRoot.pool.f1;
    d.x -= _GlideRoot.pool.f1;
    break;

  case aaEdgeSenseBottom:
    c.y -= _GlideRoot.pool.f1;
    d.y -= _GlideRoot.pool.f1;
    break;

  case aaEdgeSenseRight:
    c.x += _GlideRoot.pool.f1;
    d.x += _GlideRoot.pool.f1;
    break;
  }

#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  {
    /* Maintain the same culling mode that we would have used for
     * the inner triangle.
     */
    GR_DCL_GC;      
    const FxU32 cullMode = gc->state.cull_mode;
    const FxU32 sMode = ((cullMode != GR_CULL_DISABLE)
                         ? (kSetupCullEnable | ((cullMode == GR_CULL_POSITIVE)
                                                ? kSetupCullPositive
                                                : kSetupCullNegative))
                         : kSetupCullDisable);
    const float* lVertexList[4];

    lVertexList[0] = &c.x;
    lVertexList[1] = &d.x;
    lVertexList[2] = &a->x;
    lVertexList[3] = &b->x;

    grAADrawStrip(kSetupStrip | sMode,
                  lVertexList,
                  sizeof(lVertexList) / sizeof(lVertexList[0]));
  }
#else  /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
  if (a->y < b->y) {
    grAADrawEdgeTriangle((float *) a, (float *) &c, (float *) &d);
    grAADrawEdgeTriangle((float *) a, (float *) &d, (float *) b);
  } else {
    grAADrawEdgeTriangle((float *) b, (float *) &d, (float *) &c);
    grAADrawEdgeTriangle((float *) b, (float *) &c, (float *) a);
  }
#endif  /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
} /* grAADrawTriEdge */

#define DXSQUARED(i,j) ((i->x - j->x) * (i->x - j->x))
#define DYSQUARED(i,j) ((i->y - j->y) * (i->y - j->y))  
  
/*
**
*/
static aaEdgeSense
aaCalcEdgeSense(const GrVertex *a, const GrVertex *b, const GrVertex *c)
{
  const GrVertex *tv;
  float dx, dy;

  if (a->y >= b->y) {
    tv = a; a = b; b = tv;
  }
  dx = a->x - b->x;
  dy = a->y - b->y;

  if (dx == 0.f) {
    return ((c->x > a->x) ? aaEdgeSenseLeft : aaEdgeSenseRight);
  }
  else if (dy == 0.0f) {
    return ((c->y > a->y) ? aaEdgeSenseBottom : aaEdgeSenseTop);
  }
  else {
    float dcax, dcay, dcbx, dcby, cp;
    dcax = a->x - c->x;
    dcay = a->y - c->y;
    dcbx = b->x - c->x;
    dcby = b->y - c->y;
    cp = dcax * dcby - dcay * dcbx;

    if (dx > 0.0f) {
      if (dx >= -dy)    /* X-major line */
        return ((cp > 0.0f) ? aaEdgeSenseTop : aaEdgeSenseBottom);
      else              /* Y-major line */
        return ((cp > 0.0f) ? aaEdgeSenseRight : aaEdgeSenseLeft);
    }
    else {
      if (dx <= dy)     /* X-major line */
        return ((cp < 0.0f) ? aaEdgeSenseTop : aaEdgeSenseBottom);
      else              /* Y-major line */
        return ((cp < 0.0f) ? aaEdgeSenseLeft : aaEdgeSenseRight);
    }
  }
} /* aaCalcEdgeSense */

/*---------------------------------------------------------------------------
** grAADrawTriangle
**
**  NOTE:  This doesn't quite work yet
*/
GR_ENTRY(grAADrawTriangle,
         void,
         (const GrVertex *a, const GrVertex *b, const GrVertex *c,
          FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias)) 
{
#define FN_NAME "grAADrawTriangle"
  FxU32
    fbzMode,                    /* What we write to fbzMode */
    fbzModeOld;                 /* Squirrel away current fbzMode */
  aaEdgeSense
    sense;                      /* Edge sense for edge polygons */

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 96);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x,0x%x,0x%x,%d,%d,%d)\n",
                 a,b,c,ab_antialias,bc_antialias,ca_antialias);

  GR_CHECK_COMPATABILITY(FN_NAME,
                         ((gc->cmdTransportInfo.paramMask & (SST_SETUP_A << SSTCP_PKT3_PMASK_SHIFT)) == 0),
                         "Antialiasing require enabling alpha");
  
  GR_FLUSH_STATE();

  fbzModeOld = gc->state.fbi_config.fbzMode;
  
  /* backfaced or zero area */
  if (TRISETUP(a, b, c) <= 0) return;
   
  /* Disable depth buffer writes for edge triangles */
  fbzMode = fbzModeOld;
  fbzMode &= ~(SST_ZAWRMASK);

#if 1 /* speckle mini-workaround, another one below... - jeske */
  GR_SET_EXPECTED_SIZE(4 * 2, 1 * 2);
  GR_SET(BROADCAST_ID, hw, nopCMD, 0);
  GR_SET(BROADCAST_ID, hw, fbzMode, fbzMode);
#else
  GR_SET_EXPECTED_SIZE(4,1);
  GR_SET(BROADCAST_ID, hw, fbzMode, fbzMode);
#endif
  GR_CHECK_SIZE();

  if (ab_antialias) {
    sense = aaCalcEdgeSense(a, b, c);
    grAADrawTriEdge(a, b, sense);
  }
  
  if (bc_antialias) {
    sense = aaCalcEdgeSense(b, c, a);
    grAADrawTriEdge(b, c, sense);
  }
  
  if (ca_antialias) {
    sense = aaCalcEdgeSense(c, a, b);
    grAADrawTriEdge(c, a, sense);
  }

  /* Restore the old fbzMode */
#if 1 /* speckle mini-workaround, another one above... - jeske */
  GR_SET_EXPECTED_SIZE(4 * 2, 1 * 2);
  GR_SET(BROADCAST_ID, hw, nopCMD, 0);
  GR_SET(BROADCAST_ID, hw, fbzMode, fbzModeOld);
#else
  GR_SET_EXPECTED_SIZE(4,1);
  GR_SET(BROADCAST_ID, hw, fbzMode, fbzModeOld);
#endif
  GR_END();
#undef FN_NAME
} /* grAADrawTriangle */

/*---------------------------------------------------------------------------
**  grAADrawPolygon
**  NOTE:  This routine does not do backface culling!!!
*/

GR_ENTRY(grAADrawPolygon, void, (const int nverts, const int ilist[], const GrVertex vlist[])) 
{
#define FN_NAME "grAADrawPolygon"
  int
    i = 1;                      /* index into ilist */
  const GrVertex
    *a, *b, *c;                 /* What we end up sending */
  aaEdgeSense
    sense;                      /* Edge sense for edge polygons */

  GR_BEGIN_NOFIFOCHECK("grAADrawPolygon",97);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x,0x%x)\n",nverts,ilist,vlist);

  GR_CHECK_COMPATABILITY(FN_NAME,
                         ((gc->cmdTransportInfo.paramMask & (SST_SETUP_A << SSTCP_PKT3_PMASK_SHIFT)) == 0),
                         "Antialiasing require enabling alpha");

  GR_FLUSH_STATE();

  for (i=1; i < nverts - 1; i++) {
    a = &vlist[ilist[0]];
    b = &vlist[ilist[i]];
    c = &vlist[ilist[i+1]];
    if (TRISETUP (a, b, c) <= 0) /* backfaced or zero area */
      continue;

    /*
    **  Always draw the BC edge.
    **  On the first triangle, additionally draw the AB edge.
    **  On the last triangle, draw the CA edge.
    */
    if (i == 1) {
      sense = aaCalcEdgeSense(a, b, c);
      grAADrawTriEdge(a, b, sense);

    } else if (i == (nverts - 2)) {
      sense = aaCalcEdgeSense(c, a, b);
      grAADrawTriEdge(c, a, sense);
    } 

    sense = aaCalcEdgeSense(b, c, a);
    grAADrawTriEdge(b, c, sense);
  }

  GR_END();
#undef FN_NAME
} /* grAADrawPolygon */

/*---------------------------------------------------------------------------
**  grAADrawPolygonVertexList
**  NOTE:  This routine does not do backface culling!!!
*/

GR_ENTRY(grAADrawPolygonVertexList, void, (const int nverts, const GrVertex vlist[])) 
{
#define FN_NAME "grAADrawPolygonVertexList"
  int
    i = 1;                      /* index into ilist */
  const GrVertex
    *a, *b, *c;                 /* What we end up sending */
  aaEdgeSense
    sense;                      /* Edge sense for edge polygons */
  FxI32
    res;

  GR_BEGIN_NOFIFOCHECK("grAADrawPolygonVertexList",97);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x)\n",nverts,vlist);

  GR_CHECK_COMPATABILITY(FN_NAME,
                         ((gc->cmdTransportInfo.paramMask & (SST_SETUP_A << SSTCP_PKT3_PMASK_SHIFT)) == 0),
                         "Antialiasing require enabling alpha");

  GR_FLUSH_STATE();

  for (i=1; i < nverts - 1; i++) {
    a = &vlist[0];
    b = &vlist[i];
    c = &vlist[i+1];

    res = TRISETUP (a, b, c);
    if (res <= 0)               /* backfaced or zero area */
      continue;

    /*
    **  Always draw the BC edge.
    **  On the first triangle, additionally draw the AB edge.
    **  On the last triangle, draw the CA edge.
    */
    if (i == 1) {
      sense = aaCalcEdgeSense(a, b, c);
      grAADrawTriEdge(a, b, sense);

    } else if (i == (nverts - 2)) {
      sense = aaCalcEdgeSense(c, a, b);
      grAADrawTriEdge(c, a, sense);
    } 

    sense = aaCalcEdgeSense(b, c, a);
    grAADrawTriEdge(b, c, sense);

  }
  GR_END();
#undef FN_NAME
} /* grAADrawPolygonVertexList */

#ifdef GLIDE3      /* glide 3. draw aa points, and aa strip line routine */
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
  GrCullMode_t cullSave;
  float *e, ptX, ptY;
  FxU32 ia;
  FxU32 i;
  FxU32 vsize;
  FxI32 stride = mode;

  GDBG_INFO(94,"_grAADrawPoints(0x%x)\n",e);

  GDBG_INFO_MORE(gc->myLevel, "(count = %d, pointers = 0x%x)\n",
                 count, pointers);

  /* turn off culling so triangles unconditionally draw */
  cullSave = gc->state.cull_mode;
  gc->state.cull_mode = GR_CULL_DISABLE;

  if (stride == 0)
    stride = gc->state.vData.vStride;

  GR_FLUSH_STATE();

  if (gc->state.vData.colorType == GR_FLOAT)
    ia = gc->state.vData.aInfo.offset;
  else
    ia = gc->state.vData.pargbInfo.offset;

  vsize = gc->state.vData.vSize * 6;

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
      (float *)pointers += stride;
      
      ptX = FARRAY(e, GR_VERTEX_X_OFFSET << 2);
      ptY = FARRAY(e, GR_VERTEX_Y_OFFSET << 2);

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
        int v;
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
              FxU32 argb;
              
              if (i == ia) {
                argb = *((FxU32 *)((int)e + i)) & 0x00ffffff;
                TRI_SETF(*((float *)&argb));
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
      FxU32 dataElem = 0;

      /* We draw this as a 4 triangle fan centered around E. */
      GR_SET_EXPECTED_SIZE(vsize, 1);
      TRI_STRIP_BEGIN(kSetupFan, 
                      0x06UL, gc->state.vData.vSize, 
                      SSTCP_PKT3_BDDDDD);

      e = pointers;
      if (mode)
        e = *(float **)e;
      oow = 1.0f / FARRAY(e, gc->state.vData.wInfo.offset);        
      (float *)pointers += stride;
      
      ptX = FARRAY(e, GR_VERTEX_X_OFFSET << 2)
        *oow*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
      ptY = FARRAY(e, GR_VERTEX_Y_OFFSET << 2)
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
        int v;
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

  gc->state.cull_mode = cullSave;

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
  float **lPtrs = (float **) pointers;
  float *v1, *v2;
  float v1a, v2a;
  FxU32 ia, vNum = 0;
  FxU32 vsize;
  FxU32 sCount;
  FxI32 stride = mode;

  GDBG_INFO(95,"_grAADrawLineStrip(count = %d, pointers = 0x%x)\n",
            count, pointers);

  if (gc->state.vData.colorType == GR_FLOAT) {
    ia = gc->state.vData.aInfo.offset;
    v1a = 0.0f;
    v2a = 0.0f;
  }
  else
    ia = gc->state.vData.pargbInfo.offset;

  GR_FLUSH_STATE();

  if (stride == 0)
    stride = gc->state.vData.vStride;
  if (ltype == GR_LINES)
    sCount = count >> 1; /* line list */
  else
    sCount = count-1;    /* strip line */

  vsize = 6 * gc->state.vData.vSize;

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    while (sCount--) {
      v1 = (float *)pointers;
      v2 = (float *)pointers + 1;
      if (mode) {
        v1 = *(float **)v1;
        v2 = *(float **)v2;
      }
      (float *)pointers += stride;
      if (ltype == GR_LINES)
        (float *)pointers += stride;
      
      /* draw from low Y to high Y */
      if (FARRAY(v2, GR_VERTEX_Y_OFFSET << 2) < FARRAY(v1, GR_VERTEX_Y_OFFSET << 2)) {
        float* tv = v1; v1 = v2; v2 = tv;
      }
      
      /* compute deltas and absolute deltas */
      adx = FARRAY(v2, GR_VERTEX_X_OFFSET << 2) - FARRAY(v1, GR_VERTEX_X_OFFSET << 2);
      ady = FARRAY(v2, GR_VERTEX_Y_OFFSET << 2) - FARRAY(v1, GR_VERTEX_Y_OFFSET << 2);
      if (adx < 0)
        adx = -adx;
      if (ady < 0)
        ady = -ady;
      
      if (gc->state.vData.colorType != GR_FLOAT) {
        *((FxU32 *)&v1a)=*((FxU32 *)((int)v1 + ia))&0x00ffffff;
        *((FxU32 *)&v2a)=*((FxU32 *)((int)v2 + ia))&0x00ffffff;
      }
      
      GR_SET_EXPECTED_SIZE(vsize, 1);
      TRI_STRIP_BEGIN(kSetupStrip, 6, gc->state.vData.vSize, SSTCP_PKT3_BDDDDD);

      if (adx >= ady) {           /* X major line */
        FxU32 i, dataElem;
        
        /* point 1 (b) = (v2)*/
        TRI_SETF(FARRAY(v2, GR_VERTEX_X_OFFSET << 2));
        TRI_SETF(FARRAY(v2, GR_VERTEX_Y_OFFSET << 2) - _GlideRoot.pool.f1);
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v2a); } else { TRI_SETF(FARRAY(v2, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 2 (a) = (v1)*/
        TRI_SETF(FARRAY(v1, GR_VERTEX_X_OFFSET << 2));
        TRI_SETF(FARRAY(v1, GR_VERTEX_Y_OFFSET << 2) - _GlideRoot.pool.f1);
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v1a); } else { TRI_SETF(FARRAY(v1, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 3 (v2) = (v2)*/
        TRI_SETF(FARRAY(v2, GR_VERTEX_X_OFFSET << 2));
        TRI_SETF(FARRAY(v2, GR_VERTEX_Y_OFFSET << 2));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          TRI_SETF(FARRAY(v2, i));
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 4 (v1) = (v1)*/
        TRI_SETF(FARRAY(v1, GR_VERTEX_X_OFFSET << 2));
        TRI_SETF(FARRAY(v1, GR_VERTEX_Y_OFFSET << 2));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          TRI_SETF(FARRAY(v1, i));
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 5 (f) = (v2)*/
        TRI_SETF(FARRAY(v2, GR_VERTEX_X_OFFSET << 2));
        TRI_SETF(FARRAY(v2, GR_VERTEX_Y_OFFSET << 2) + _GlideRoot.pool.f1);
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v2a); } else { TRI_SETF(FARRAY(v2, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 6 (a) = (v1)*/
        TRI_SETF(FARRAY(v1, GR_VERTEX_X_OFFSET << 2));
        TRI_SETF(FARRAY(v1, GR_VERTEX_Y_OFFSET << 2) + _GlideRoot.pool.f1);
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v1a); } else { TRI_SETF(FARRAY(v1, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
      } else {                      /* Y major line */
        FxU32 i, dataElem;
        
        /* point 1 (b) = (v2)*/
        TRI_SETF(FARRAY(v2, GR_VERTEX_X_OFFSET << 2) + _GlideRoot.pool.f1);
        TRI_SETF(FARRAY(v2, GR_VERTEX_Y_OFFSET << 2));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v2a); } else { TRI_SETF(FARRAY(v2, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 2 (a) = (v1)*/
        TRI_SETF(FARRAY(v1, GR_VERTEX_X_OFFSET << 2) + _GlideRoot.pool.f1);
        TRI_SETF(FARRAY(v1, GR_VERTEX_Y_OFFSET << 2));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v1a); } else { TRI_SETF(FARRAY(v1, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 3 (v2) = (v2)*/
        TRI_SETF(FARRAY(v2, GR_VERTEX_X_OFFSET << 2));
        TRI_SETF(FARRAY(v2, GR_VERTEX_Y_OFFSET << 2));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          TRI_SETF(FARRAY(v2, i));
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 4 (v1) = (v1)*/
        TRI_SETF(FARRAY(v1, GR_VERTEX_X_OFFSET << 2));
        TRI_SETF(FARRAY(v1, GR_VERTEX_Y_OFFSET << 2));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          TRI_SETF(FARRAY(v1, i));
          dataElem++; i = gc->tsuDataList[dataElem];
        }

        /* point 5 (f) = (v2)*/
        TRI_SETF(FARRAY(v2, GR_VERTEX_X_OFFSET << 2) - _GlideRoot.pool.f1);
        TRI_SETF(FARRAY(v2, GR_VERTEX_Y_OFFSET << 2));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v2a); } else { TRI_SETF(FARRAY(v2, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
        /* point 6 (a) = (v1)*/
        TRI_SETF(FARRAY(v1, GR_VERTEX_X_OFFSET << 2) - _GlideRoot.pool.f1);
        TRI_SETF(FARRAY(v1, GR_VERTEX_Y_OFFSET << 2));
        dataElem = 0;
        i = gc->tsuDataList[dataElem];
        while (i != GR_DLIST_END) { 
          if (i == ia) { TRI_SETF(v1a); } else { TRI_SETF(FARRAY(v1, i)); } 
          dataElem++; i = gc->tsuDataList[dataElem];
        }
        
      }
      TRI_END;
      GR_CHECK_SIZE();
    }
  }
  else {
    float oowa, oowb, owa, owb, tmp1, tmp2, fax, fay, fbx, fby;

    if (ltype == GR_LINE_STRIP) {
      v1 = (float *)pointers;
      if (mode) {
        v1 = *(float **)v1;
      }
      oowb = 1.0f / FARRAY(v1, gc->state.vData.wInfo.offset);        
    }
    while (sCount--) {
      if (ltype == GR_LINES) {
        v1 = (float *)pointers;
        v2 = (float *)pointers + 1;
        if (mode) {
          v1 = *(float **)v1;
          v2 = *(float **)v2;
        }
        (float *)pointers += stride;
        if (ltype == GR_LINES)
          (float *)pointers += stride;
        owa = oowa = 1.0f / FARRAY(v1, gc->state.vData.wInfo.offset);
        owb = oowb = 1.0f / FARRAY(v2, gc->state.vData.wInfo.offset);
      }
      else {
        owa = oowa = oowb;
        v1 = (float *)pointers;
        v2 = (float *)pointers + 1;
        if (mode) {
          v1 = *(float **)v1;
          v2 = *(float **)v2;
        }
        (float *)pointers += stride;
        owb = oowb = 1.0f / FARRAY(v2, gc->state.vData.wInfo.offset);
      }
      
      fay = tmp1 = FARRAY(v1, GR_VERTEX_Y_OFFSET << 2)
        *oowa*gc->state.Viewport.hheight+gc->state.Viewport.oy;
      fby = tmp2 = FARRAY(v2, GR_VERTEX_Y_OFFSET << 2)
        *oowb*gc->state.Viewport.hheight+gc->state.Viewport.oy;        

      /* draw from low Y to high Y */
      if (tmp2 < tmp1) {
        float* tv = v1; v1 = v2; v2 = tv;
        owa = oowb; owb = oowa;
        fay = tmp2;
        fby = tmp1;     
      }
      fax = FARRAY(v1, GR_VERTEX_X_OFFSET << 2)
        *owa*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
      fbx = FARRAY(v2, GR_VERTEX_X_OFFSET << 2)
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
        *((FxU32 *)&v1a)=*((FxU32 *)((int)v1 + ia))&0x00ffffff;
        *((FxU32 *)&v2a)=*((FxU32 *)((int)v2 + ia))&0x00ffffff;
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
      TRI_END;
      GR_CHECK_SIZE();
    }
  }
#endif

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

  if (FARRAY(a, GR_VERTEX_Y_OFFSET << 2) >= FARRAY(b, GR_VERTEX_Y_OFFSET << 2)) {
    fa = b; fb = a;
  }
  else {
    fa = a; fb = b;
  }
  dx = FARRAY(fa, GR_VERTEX_X_OFFSET << 2) - FARRAY(fb, GR_VERTEX_X_OFFSET << 2);
  dy = FARRAY(fa, GR_VERTEX_Y_OFFSET << 2) - FARRAY(fb, GR_VERTEX_Y_OFFSET << 2);

  if (dx == 0.f) {
    sense = (FARRAY(c, GR_VERTEX_X_OFFSET << 2) > FARRAY(fa, GR_VERTEX_X_OFFSET << 2)) ? aaEdgeSenseLeft : aaEdgeSenseRight;
  }
  else if (dy == 0.0f) {
    sense = (FARRAY(c, GR_VERTEX_Y_OFFSET << 2) > FARRAY(fa, GR_VERTEX_Y_OFFSET << 2)) ? aaEdgeSenseBottom : aaEdgeSenseTop;
  }
  else {
    float dcax, dcay, dcbx, dcby, cp;
    dcax = FARRAY(fa, GR_VERTEX_X_OFFSET << 2) - FARRAY(c, GR_VERTEX_X_OFFSET << 2);
    dcay = FARRAY(fa, GR_VERTEX_Y_OFFSET << 2) - FARRAY(c, GR_VERTEX_Y_OFFSET << 2);
    dcbx = FARRAY(fb, GR_VERTEX_X_OFFSET << 2) - FARRAY(c, GR_VERTEX_X_OFFSET << 2);
    dcby = FARRAY(fb, GR_VERTEX_Y_OFFSET << 2) - FARRAY(c, GR_VERTEX_Y_OFFSET << 2);
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
    TRI_SETF(FARRAY(a, GR_VERTEX_X_OFFSET << 2));
    v2x = FARRAY(b, GR_VERTEX_X_OFFSET << 2);
    TRI_SETF(FARRAY(a, GR_VERTEX_Y_OFFSET << 2) + _GlideRoot.pool.f1);
    v2y = FARRAY(b, GR_VERTEX_Y_OFFSET << 2) + _GlideRoot.pool.f1;
    break;

  case aaEdgeSenseLeft:
    TRI_SETF(FARRAY(a, GR_VERTEX_X_OFFSET << 2) - _GlideRoot.pool.f1);
    v2x = FARRAY(b, GR_VERTEX_X_OFFSET << 2) - _GlideRoot.pool.f1;
    TRI_SETF(FARRAY(a, GR_VERTEX_Y_OFFSET << 2));
    v2y = FARRAY(b, GR_VERTEX_Y_OFFSET << 2);
    break;

  case aaEdgeSenseBottom:
    TRI_SETF(FARRAY(a, GR_VERTEX_X_OFFSET << 2));
    v2x = FARRAY(b, GR_VERTEX_X_OFFSET << 2);
    TRI_SETF(FARRAY(a, GR_VERTEX_Y_OFFSET << 2) - _GlideRoot.pool.f1);
    v2y = FARRAY(b, GR_VERTEX_Y_OFFSET << 2) - _GlideRoot.pool.f1;
    break;

  case aaEdgeSenseRight:
  default: /* silence warnings */
    TRI_SETF(FARRAY(a, GR_VERTEX_X_OFFSET << 2) + _GlideRoot.pool.f1);
    v2x = FARRAY(b, GR_VERTEX_X_OFFSET << 2) + _GlideRoot.pool.f1;
    TRI_SETF(FARRAY(a, GR_VERTEX_Y_OFFSET << 2));
    v2y = FARRAY(b, GR_VERTEX_Y_OFFSET << 2);
    break;
  }

  if (gc->state.vData.colorType == GR_FLOAT) {
    ia = gc->state.vData.aInfo.offset;
    v1a = v2a = 0.f;
  }
  else {
    ia = gc->state.vData.pargbInfo.offset;
    *((FxU32 *)&v1a)=*((FxU32 *)((int)a + ia))&0x00ffffff;
    *((FxU32 *)&v2a)=*((FxU32 *)((int)b + ia))&0x00ffffff;
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
    TRI_SETF(FARRAY(a, GR_VERTEX_X_OFFSET << 2));
    TRI_SETF(FARRAY(a, GR_VERTEX_Y_OFFSET << 2));
    i = gc->tsuDataList[dataElem];
    while (i != GR_DLIST_END) {
      TRI_SETF(FARRAY(a, i));
      dataElem++;
      i = gc->tsuDataList[dataElem];
    }
    dataElem = 0;
    TRI_SETF(FARRAY(b, GR_VERTEX_X_OFFSET << 2));
    TRI_SETF(FARRAY(b, GR_VERTEX_Y_OFFSET << 2));
    i = gc->tsuDataList[dataElem];
    while (i != GR_DLIST_END) {
      TRI_SETF(FARRAY(b, i));
      dataElem++;
      i = gc->tsuDataList[dataElem];
    }
  }
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
  FxI32 stride = mode;

  GDBG_INFO(96, "_grAADrawTriangles (count = %d, pointers = 0x%x)\n",
            count, pointers);

  if (ttype == GR_TRIANGLES)
    _grDrawTriangles(mode, count, pointers);

  fbzModeOld = gc->state.fbi_config.fbzMode;
  gc->state.fbi_config.fbzMode &= ~(SST_ZAWRMASK);
  /* gc->state.invalid |= fbzModeBIT; */
  GR_FLUSH_STATE();

  if (stride == 0)
    stride = gc->state.vData.vStride;

  /* backfaced or zero area */
  while (tCount <= count) {
    float *a, *b, *c;

    GR_BEGIN_NOFIFOCHECK("_grAADrawTri",96);
      
    a = pointers;
    b = (float *)pointers + 1;
    c = (float *)pointers + 2;
    if (mode) {
      a = *(float **)a;
      b = *(float **)b;
      c = *(float **)c;
    }
    (float *)pointers += stride*3;

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
        int ay = *(int *)&a[GR_VERTEX_Y_OFFSET];
        int by = *(int *)&b[GR_VERTEX_Y_OFFSET];
        int cy = *(int *)&c[GR_VERTEX_Y_OFFSET];
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
        dxAB = fa[GR_VERTEX_X_OFFSET] - fb[GR_VERTEX_X_OFFSET];
        dxBC = fb[GR_VERTEX_X_OFFSET] - fc[GR_VERTEX_X_OFFSET];
        
        dyAB = fa[GR_VERTEX_Y_OFFSET] - fb[GR_VERTEX_Y_OFFSET];
        dyBC = fb[GR_VERTEX_Y_OFFSET] - fc[GR_VERTEX_Y_OFFSET];
        
        /* Stash the area in the float pool for easy access */
        _GlideRoot.pool.ftemp1 = dxAB * dyBC - dxBC * dyAB;
        
#define FloatVal(__f) (((__f) < 786432.875) ? (__f) : ((__f) - 786432.875))
        {
          const FxI32 j = *(FxI32*)&_GlideRoot.pool.ftemp1;
          
          /* Zero-area triangles are BAD!! */
          if ((j & 0x7FFFFFFF) == 0) {
            GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                      FloatVal(fa[GR_VERTEX_X_OFFSET]), FloatVal(fa[GR_VERTEX_Y_OFFSET]), 
                      FloatVal(fb[GR_VERTEX_X_OFFSET]), FloatVal(fb[GR_VERTEX_Y_OFFSET]), 
                      FloatVal(fc[GR_VERTEX_X_OFFSET]), FloatVal(fc[GR_VERTEX_Y_OFFSET]), 
                      _GlideRoot.pool.ftemp1, gc->state.cull_mode, culltest);

            goto done;
          }
      
          /* Backface culling, use sign bit as test */
          if ((gc->state.cull_mode != GR_CULL_DISABLE) &&
              ((j ^ (culltest << 31)) >= 0)) {
        
            GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                      FloatVal(fa[GR_VERTEX_X_OFFSET]), FloatVal(fa[GR_VERTEX_Y_OFFSET]), 
                      FloatVal(fb[GR_VERTEX_X_OFFSET]), FloatVal(fb[GR_VERTEX_Y_OFFSET]), 
                      FloatVal(fc[GR_VERTEX_X_OFFSET]), FloatVal(fc[GR_VERTEX_Y_OFFSET]), 
                      _GlideRoot.pool.ftemp1, gc->state.cull_mode, culltest);
        
            goto done;
          }
        }
      }
    } /* end culling test */
      
    aaDrawArrayEdgeSense(a, b, c);
      
    aaDrawArrayEdgeSense(b, c, a);
      
    aaDrawArrayEdgeSense(c, a, b);
      
  done:
    lPtr += 3;
    tCount+=3;
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

  if (FARRAY(a, GR_VERTEX_Y_OFFSET << 2) >= FARRAY(b, GR_VERTEX_Y_OFFSET << 2)) {
    fa = b; fb = a;
  }
  else {
    fa = a; fb = b;
  }
  dx = FARRAY(fa, GR_VERTEX_X_OFFSET << 2) - FARRAY(fb, GR_VERTEX_X_OFFSET << 2);
  dy = FARRAY(fa, GR_VERTEX_Y_OFFSET << 2) - FARRAY(fb, GR_VERTEX_Y_OFFSET << 2);

  if (dx == 0.f) {
    sense = (FARRAY(c, GR_VERTEX_X_OFFSET << 2) > FARRAY(fa, GR_VERTEX_X_OFFSET << 2)) ? aaEdgeSenseLeft : aaEdgeSenseRight;
  }
  else if (dy == 0.0f) {
    sense = (FARRAY(c, GR_VERTEX_Y_OFFSET << 2) > FARRAY(fa, GR_VERTEX_Y_OFFSET << 2)) ? aaEdgeSenseBottom : aaEdgeSenseTop;
  }
  else {
    float dcax, dcay, dcbx, dcby, cp;
    dcax = FARRAY(fa, GR_VERTEX_X_OFFSET << 2) - FARRAY(c, GR_VERTEX_X_OFFSET << 2);
    dcay = FARRAY(fa, GR_VERTEX_Y_OFFSET << 2) - FARRAY(c, GR_VERTEX_Y_OFFSET << 2);
    dcbx = FARRAY(fb, GR_VERTEX_X_OFFSET << 2) - FARRAY(c, GR_VERTEX_X_OFFSET << 2);
    dcby = FARRAY(fb, GR_VERTEX_Y_OFFSET << 2) - FARRAY(c, GR_VERTEX_Y_OFFSET << 2);
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
  fax = FARRAY(a, GR_VERTEX_X_OFFSET << 2)*oowa*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
  fay = FARRAY(a, GR_VERTEX_Y_OFFSET << 2)*oowa*gc->state.Viewport.hheight+gc->state.Viewport.oy;
  fbx = FARRAY(b, GR_VERTEX_X_OFFSET << 2)*oowb*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
  fby = FARRAY(b, GR_VERTEX_Y_OFFSET << 2)*oowb*gc->state.Viewport.hheight+gc->state.Viewport.oy;

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
  default: /* silence warnings */
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
    ia = gc->state.vData.pargbInfo.offset;
    *((FxU32 *)&v1a)=*((FxU32 *)((int)a + ia))&0x00ffffff;
    *((FxU32 *)&v2a)=*((FxU32 *)((int)b + ia))&0x00ffffff;
  }

  {

    AA_TRI_VP_SETFS(a,oowa);

    TRI_SETF(v2x);
    TRI_SETF(v2y);

    AA_TRI_VP_SETFS(b,oowb);

    TRI_SETF(FARRAY(a, GR_VERTEX_X_OFFSET << 2));
    TRI_SETF(FARRAY(a, GR_VERTEX_Y_OFFSET << 2));
    TRI_VP_SETFS(a,oowa);

    TRI_SETF(FARRAY(b, GR_VERTEX_X_OFFSET << 2));
    TRI_SETF(FARRAY(b, GR_VERTEX_Y_OFFSET << 2));
    TRI_VP_SETFS(b,oowb);

  }
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
  FxI32 stride = mode;

  GDBG_INFO(96, "_grAADrawTriangles (count = %d, pointers = 0x%x)\n",
            count, pointers);

  if (ttype == GR_TRIANGLES)
    _grDrawTriangles(mode, count, pointers);

  fbzModeOld = gc->state.fbi_config.fbzMode;
  gc->state.fbi_config.fbzMode &= ~(SST_ZAWRMASK);
  /* gc->state.invalid |= fbzModeBIT; */
  GR_FLUSH_STATE();

  if (stride == 0)
    stride = gc->state.vData.vStride;

  /* backfaced or zero area */
  while (tCount <= count) {
    float *a, *b, *c;
    float oowa, oowb, oowc;

    GR_BEGIN_NOFIFOCHECK("_grAADrawTri",96);
      
    a = pointers;
    b = (float *)pointers + 1;
    c = (float *)pointers + 2;
    if (mode) {
      a = *(float **)a;
      b = *(float **)b;
      c = *(float **)c;
    }
    (float *)pointers += stride*3;
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
        float fay = a[GR_VERTEX_Y_OFFSET]*oowa*gc->state.Viewport.hheight*gc->state.Viewport.oy;
        float fby = b[GR_VERTEX_Y_OFFSET]*oowb*gc->state.Viewport.hheight*gc->state.Viewport.oy;
        float fcy = c[GR_VERTEX_Y_OFFSET]*oowc*gc->state.Viewport.hheight*gc->state.Viewport.oy;
        int ay = *(int *)&fay;
        int by = *(int *)&fby;
        int cy = *(int *)&fcy;
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
        dxAB = fa[GR_VERTEX_X_OFFSET] - fb[GR_VERTEX_X_OFFSET];
        dxBC = fb[GR_VERTEX_X_OFFSET] - fc[GR_VERTEX_X_OFFSET];
        
        dyAB = fa[GR_VERTEX_Y_OFFSET] - fb[GR_VERTEX_Y_OFFSET];
        dyBC = fb[GR_VERTEX_Y_OFFSET] - fc[GR_VERTEX_Y_OFFSET];
        
        /* Stash the area in the float pool for easy access */
        _GlideRoot.pool.ftemp1 = dxAB * dyBC - dxBC * dyAB;
        
#define FloatVal(__f) (((__f) < 786432.875) ? (__f) : ((__f) - 786432.875))
        {
          const FxI32 j = *(FxI32*)&_GlideRoot.pool.ftemp1;
          
          /* Zero-area triangles are BAD!! */
          if ((j & 0x7FFFFFFF) == 0) {
            GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                      FloatVal(fa[GR_VERTEX_X_OFFSET]), FloatVal(fa[GR_VERTEX_Y_OFFSET]), 
                      FloatVal(fb[GR_VERTEX_X_OFFSET]), FloatVal(fb[GR_VERTEX_Y_OFFSET]), 
                      FloatVal(fc[GR_VERTEX_X_OFFSET]), FloatVal(fc[GR_VERTEX_Y_OFFSET]), 
                      _GlideRoot.pool.ftemp1, gc->state.cull_mode, culltest);

            goto done;
          }
      
          /* Backface culling, use sign bit as test */
          if ((gc->state.cull_mode != GR_CULL_DISABLE) &&
              ((j ^ (culltest << 31)) >= 0)) {
        
            GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                      FloatVal(fa[GR_VERTEX_X_OFFSET]), FloatVal(fa[GR_VERTEX_Y_OFFSET]), 
                      FloatVal(fb[GR_VERTEX_X_OFFSET]), FloatVal(fb[GR_VERTEX_Y_OFFSET]), 
                      FloatVal(fc[GR_VERTEX_X_OFFSET]), FloatVal(fc[GR_VERTEX_Y_OFFSET]), 
                      _GlideRoot.pool.ftemp1, gc->state.cull_mode, culltest);
        
            goto done;
          }
        }
      }
    } /* end culling test */
      
    aaVpDrawArrayEdgeSense(a, b, c, oowa, oowb);
      
    aaVpDrawArrayEdgeSense(b, c, a, oowb, oowc);
      
    aaVpDrawArrayEdgeSense(c, a, b, oowc, oowa);
      
  done:
    lPtr += 3;
    tCount+=3;
  }

  gc->state.fbi_config.fbzMode = fbzModeOld;
  gc->state.invalid |= fbzModeBIT;
  GR_FLUSH_STATE();

#undef FN_NAME
} /* _grAAVpDrawTriangles */

#endif  /* GLIDE3 */

