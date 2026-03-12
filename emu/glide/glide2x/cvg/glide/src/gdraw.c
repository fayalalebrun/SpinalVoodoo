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
 ** Revision 1.4.2.4  2005/04/25 23:53:59  koolsmoky
 ** Fix grDrawTriangle calling convention for Linux.
 ** Thanks to Ozkan Sezer <sezeroz@gmail.com>.
 **
 ** Revision 1.4.2.3  2005/04/23 18:27:26  koolsmoky
 ** fixed grDrawTriangle triangle dispatch code
 **
 ** Revision 1.4.2.2  2005/01/22 14:52:01  koolsmoky
 ** enabled packed argb for cmd packet type 3
 **
 ** Revision 1.4.2.1  2004/12/23 20:45:56  koolsmoky
 ** converted to nasm syntax
 ** added x86 asm, 3dnow! triangle and mmx, 3dnow! texture download optimizations
 **
 ** Revision 1.4  2000/01/28 20:52:17  joseph
 ** Changes to support building shared libraries with PIC support.
 **
 ** Revision 1.3  2000/01/17 22:18:41  joseph
 ** A nicer, cleaner fix than the evil hack.
 **
 ** Revision 1.2  2000/01/15 00:08:22  joseph
 ** Evil nasty hack to fix dispatch code using binutils 2.9.5.
 **
 ** Revision 1.1.1.1  1999/12/07 21:49:10  joseph
 ** Initial checkin into SourceForge.
 **
** 
** 98    6/02/98 8:03p Peter
** Mmmmm.... points
** 
** 97    6/01/98 6:44p Peter
** snapped/unsnapped points
** 
** 96    5/20/98 3:51p Peter
** no fifo glide
** 
** 95    5/18/98 12:14p Peter
** better point code
** 
** 94    2/20/98 11:00a Peter
** removed glide3 from glid2 tree
** 
** 93    2/11/98 5:23p Peter
** workaround for tombraider rgb > 255.0
 * 
 * 92    1/15/98 2:46p Atai
 * fixed grDrawPoint and grDrawLine in aa mode
 * 
 * 90    12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 89    12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 85    12/08/97 12:06p Atai
 * change prototype for grDrawPoint, grDrawLine, grDrawTriangel
 * 
 * 83    11/21/97 6:05p Atai
 * use one datalist (tsuDataList) in glide3
 * 
 * 82    11/21/97 3:20p Peter
 * direct writes tsu registers
 * 
 * 81    11/19/97 4:33p Atai
 * make vSize debug variable
 * 
 * 80    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 79    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 78    11/16/97 2:20p Peter
 * cleanup
 * 
 * 77    11/15/97 7:43p Peter
 * more comdex silliness
 * 
 */
#include <memory.h>

#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#if GLIDE_DISPATCH_SETUP
#include "fxinline.h"
#endif

#define SST_XY_HALF      (1 << (SST_XY_FRACBITS - 1))
#define SST_XY_ONE       (1 << SST_XY_FRACBITS)

#define OUTBOUNDSX(a) ((a->x < 0.f ? 1 : 0) || (a->x > gc->state.screen_width ? 1 : 0))
#define OUTBOUNDSY(a) ((a->y < 0.f ? 1 : 0) || (a->y > gc->state.screen_height ? 1 : 0))
#define OUTBOUNDS(a) (OUTBOUNDSX(a) || OUTBOUNDSY(a))

/*---------------------------------------------------------------------------
** grDrawPoint
*/
GR_ENTRY(grDrawPoint, void, (const GrVertex *p))
{
#define FN_NAME "grDrawPoint"
  FxU32 x, y;
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);
  GDBG_INFO_MORE(gc->myLevel, "(0x%X) : (%f %f)\n", p, p->x, p->y);

  GR_FLUSH_STATE();

  /* we snap to an integer by adding a large enough number that it
   * shoves all fraction bits off the right side of the mantissa.
   *
   * NB: IEEE rounds to nearest integer by default, but applications
   * can change the rounding mode so that it is difficult to get the
   * correct truncation/ceiling operation w/ a simple adjustment to
   * the bias. 
   *
   * NB: The constant kNumMantissaBits defines how many bits of
   * integer precision a coordinate can have. This needs to be atleast
   * as large as the maximum hw screen resolution. We later use this
   * to compute a logical 1/2 value to fill an entire pixel.
   */
#define kNumMantissaBits 18UL
  {
    const float bias  = (const float)(3UL << kNumMantissaBits);

    /* Convert to 32-bit representation */
#define FP_TRUNC_BIAS(__fpVal, __fpBias) \
    ((__fpVal) < (__fpBias) ? (float)((__fpVal) + (__fpBias)) : (__fpVal))
    _GlideRoot.pool.ftemp1 = FP_TRUNC_BIAS(p->x, bias);
    _GlideRoot.pool.ftemp2 = FP_TRUNC_BIAS(p->y, bias);

    /* Mask off the real fractional bits from the mantissa */
    x = ((*(FxU32*)&_GlideRoot.pool.ftemp1 & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
         (0x01UL << (22UL - kNumMantissaBits)));
    y = ((*(FxU32*)&_GlideRoot.pool.ftemp2 & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
         (0x01UL << (22UL - kNumMantissaBits)));
  }

  /* draw a little triangle, with the lower left corner at pixel center. */
#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  /* The approach here is to split the triangle into two packets, one
   * that sends just the coordinates of the last two points of the
   * triangle w/ no other parameter information, and then the
   * centered point w/ all of the parameter information. The first
   * packet is has a BDD command, but is incomplete, so nothing
   * draws, the next packet has a DDD command so will complete the
   * triangle from the first packet sent.
   */
  GR_SET_EXPECTED_SIZE((sizeof(FxU32) << 2) +     /* Size of Initial coordinate packet */
                       _GlideRoot.curVertexSize,  /* The full coordinate vertex */
                       2);                        /* We do two split packets */
  TRI_PACKET_BEGIN(kSetupStrip | kSetupCullDisable, 0x00,
                   0x02, sizeof(FxU32) << 1, SSTCP_PKT3_BDDDDD);
  {
    /* Lower right corner */
    TRI_SET(x);
    TRI_SET(y);

    /* Upper right corner. */
    y -= (0x01UL << (21UL - kNumMantissaBits));
    TRI_SET(x);
    TRI_SET(y);

    /* Upper Left corner */
    x -= (0x01UL << (21UL - kNumMantissaBits));
  }
  TRI_END;

  /* Packet w/ actual point coordinate and parameter data */
  TRI_PACKET_BEGIN(kSetupStrip | kSetupCullDisable, gc->cmdTransportInfo.paramMask, 
                   1, _GlideRoot.curVertexSize, SSTCP_PKT3_DDDDDD);
  {
    TRI_SET(x);
    TRI_SET(y);
            
    /* Vertex parameters */
    {
      const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
      if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0) {
        FxU32 packedColor = 0x00;
            
        if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
          packedColor = (RGBA_COMP_CLAMP(FARRAY(p, (GR_VERTEX_B_OFFSET << 2)), B) | 
                         RGBA_COMP_CLAMP(FARRAY(p, (GR_VERTEX_G_OFFSET << 2)), G) |
                         RGBA_COMP_CLAMP(FARRAY(p, (GR_VERTEX_R_OFFSET << 2)), R));
              
          dataList++;
        }
            
        if (*dataList == (GR_VERTEX_A_OFFSET << 2)) {
          packedColor |= RGBA_COMP_CLAMP(FARRAY(p, (GR_VERTEX_A_OFFSET << 2)), A);
          dataList++;
        }
            
        TRI_SET(packedColor);
      }
#endif /* GLIDE_PACKED_RGB */

#if GLIDE_FP_CLAMP_TEX
      while(*dataList != 0) {
        TRI_SETF(FARRAY(p, *dataList));
        dataList++;
      }

      dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
               
      while(*dataList != 0) {
        TRI_SETF_CLAMP(FARRAY(p, *dataList));
        dataList++;
      }
    }
  }
  TRI_END;
  GR_CHECK_SIZE();
      
  _GlideRoot.stats.pointsDrawn++;
#else /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
  /* GMT: gross overestimate of fifo requirements */   
  GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize, _GlideRoot.curTriSize >> 2);
  {
    const struct dataList_s* dlp = gc->regDataList;
    int i;
         
#ifdef GLIDE_USE_ALT_REGMAP
    hw = SST_WRAP(hw,128);                /* use alternate register mapping */
#endif
         
    GR_SET(BROADCAST_ID, hw, FvA.x, x);
    GR_SET(BROADCAST_ID, hw, FvA.y, y);
     
    x += (0x01UL << (22UL - kNumMantissaBits));
    GR_SET(BROADCAST_ID, hw, FvB.x, x);
    GR_SET(BROADCAST_ID, hw, FvB.y, y);

    y += (0x01UL << (22UL - kNumMantissaBits));
    GR_SET(BROADCAST_ID, hw, FvC.x, x);
    GR_SET(BROADCAST_ID, hw, FvC.y, y);

    i = _GlideRoot.stats.pointsDrawn++;
    _GlideRoot.stats.pointsDrawn = ++i;
      
    dlp = gc->regDataList;
    i = dlp->i;      
         
    /* we don't care what the slopes are because the pixel center that is drawn */
    /* is exactly at vertex A - isn't that wonderful */
    while (i) {
      GR_SETF_INDEX(BROADCAST_ID, hw, ((FxU32*)dlp->addr - (FxU32*)hw), FARRAY(p,i));
      dlp++;
      i = dlp->i;
    }

    GR_SET(BROADCAST_ID, hw, triangleCMD, 1);
  }
  GR_CHECK_SIZE();
#endif /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
   
  GR_END();
#undef FN_NAME
} /* grDrawPoint */

/*---------------------------------------------------------------------------
 ** grDrawLine
 **
 ** NOTE: 1. this will not fill the last pixel in line because
 **          B2 or C is on the right edge and the right edge is not
 **          drawn.
 ** (0,0)
 **
 **  A(x1,y1-0.5)+
 **              |  \
 **              |  \  \
 **       (x1,y1)*         \
 **              |  *           \
 **              |     *\           \
 ** B1(x1,y1+0.5)+         *             \
 **                 \        \ *            +B2(x2,y2-0.5)
 **                     \          *        |
 **                        \       \    *   |
 **                             \     \     +  (x2,y2)
 **                                 \       |
 **                                      \  |
 **                                         +C(x2,y2+0.5)
 */

GR_ENTRY(grDrawLine, void, (const GrVertex *a, const GrVertex *b))
{
#define FN_NAME "grDrawLine"
  int i, j;

#define  DX _GlideRoot.pool.ftemp1
#define ADY _GlideRoot.pool.ftemp2

  GR_BEGIN_NOFIFOCHECK("grDrawLine", 91);
  GDBG_INFO_MORE(gc->myLevel, "A: (%f %f) B: (%f %f)\n", 
                 a->x, a->y, b->x, b->y);

  GR_FLUSH_STATE();

  /*
  ** compute absolute deltas and draw from low Y to high Y
  */
  ADY = b->y - a->y;
  i = *(long *)&ADY;
  if (i < 0) {
    const GrVertex *tv;
    tv = a; a = b; b = tv;
    i ^= 0x80000000;            /* ady = -ady; */
    (*(long *)&ADY) = i;
  }

  DX = b->x - a->x;
  j = *(long *)&DX;
  if (j < 0) {
    j ^= 0x80000000;            /* adx = -adx; */
  }
  
  /* check for zero-length lines */
  if ((j >= i) && (j == 0)) goto all_done;

#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  /* One packet w/ 4 vertices */
  GR_SET_EXPECTED_SIZE((_GlideRoot.curVertexSize << 2), 1);
  {
    const float* const fa = (float*)a;
    const float* const fb = (float*)b;
    
    /* Draw the triangle pair as a strip of 4 vertices.
     * We can skip all of the gradient calculation stuff.
     *
     * NB: There are two cases, x/y major lines, and each of these
     * loops are unrolled to send one set of endpoints of the 'line'
     * per iteration since we can use the same bias per iteration.
     */
    TRI_PACKET_BEGIN(kSetupCullDisable | kSetupStrip, gc->cmdTransportInfo.paramMask,
                         0x04UL, _GlideRoot.curVertexSize, SSTCP_PKT3_BDDDDD);
    {
      float fBias = - _GlideRoot.pool.fHalf;
      int vIndex;

      /* x macjor */
      if (j >= i) {
        for(vIndex = 0; vIndex < 2; vIndex++) {
          TRI_SETF(fb[GR_VERTEX_X_OFFSET]);
          TRI_SETF(fb[GR_VERTEX_Y_OFFSET] + fBias);

          {
            const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
          if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0) {
            FxU32 packedColor = 0x00;
            
            if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
              packedColor = (RGBA_COMP_CLAMP(FARRAY(fb, (GR_VERTEX_B_OFFSET << 2)), B) | 
                             RGBA_COMP_CLAMP(FARRAY(fb, (GR_VERTEX_G_OFFSET << 2)), G) |
                             RGBA_COMP_CLAMP(FARRAY(fb, (GR_VERTEX_R_OFFSET << 2)), R));
              
              dataList++;
            }
            
            if (*dataList == (GR_VERTEX_A_OFFSET << 2)) {
              packedColor |= RGBA_COMP_CLAMP(FARRAY(fb, (GR_VERTEX_A_OFFSET << 2)), A);
              dataList++;
            }
            
            TRI_SET(packedColor);
          }
#endif /* GLIDE_PACKED_RGB */

#if GLIDE_FP_CLAMP_TEX
            while(*dataList != 0) {       
              TRI_SETF(FARRAY(fb, *dataList));
              dataList++;
            }
            
            dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
          
            while(*dataList != 0) {
              TRI_SETF_CLAMP(FARRAY(fb, *dataList));
              dataList++;
            }
          }

          TRI_SETF(fa[GR_VERTEX_X_OFFSET]);
          TRI_SETF(fa[GR_VERTEX_Y_OFFSET] + fBias);

          {
            const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
          if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0) {
            FxU32 packedColor = 0x00;
            
            if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
              packedColor = (RGBA_COMP_CLAMP(FARRAY(fa, (GR_VERTEX_B_OFFSET << 2)), B) | 
                             RGBA_COMP_CLAMP(FARRAY(fa, (GR_VERTEX_G_OFFSET << 2)), G) |
                             RGBA_COMP_CLAMP(FARRAY(fa, (GR_VERTEX_R_OFFSET << 2)), R));
              
              dataList++;
            }
            
            if (*dataList == (GR_VERTEX_A_OFFSET << 2)) {
              packedColor |= RGBA_COMP_CLAMP(FARRAY(fa, (GR_VERTEX_A_OFFSET << 2)), A);
              dataList++;
            }
            
            TRI_SET(packedColor);
          }
#endif /* GLIDE_PACKED_RGB */

#if GLIDE_FP_CLAMP_TEX
            while(*dataList != 0) {       
              TRI_SETF(FARRAY(fa, *dataList));
              dataList++;
            }

            dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
          
            while (*dataList != 0) {
              TRI_SETF_CLAMP(FARRAY(fa, *dataList));
              dataList++;
            }
          }

          fBias *= -1.0f;       
        }
      } else { /* y major */
        for(vIndex = 0; vIndex < 2; vIndex++) {
          TRI_SETF(fb[GR_VERTEX_X_OFFSET] + fBias);
          TRI_SETF(fb[GR_VERTEX_Y_OFFSET]);

          {
            const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
          if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0) {
            FxU32 packedColor = 0x00;
            
            if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
              packedColor = (RGBA_COMP_CLAMP(FARRAY(fb, (GR_VERTEX_B_OFFSET << 2)), B) | 
                             RGBA_COMP_CLAMP(FARRAY(fb, (GR_VERTEX_G_OFFSET << 2)), G) |
                             RGBA_COMP_CLAMP(FARRAY(fb, (GR_VERTEX_R_OFFSET << 2)), R));
              
              dataList++;
            }
            
            if (*dataList == (GR_VERTEX_A_OFFSET << 2)) {
              packedColor |= RGBA_COMP_CLAMP(FARRAY(fb, (GR_VERTEX_A_OFFSET << 2)), A);
              dataList++;
            }
            
            TRI_SET(packedColor);
          }
#endif /* GLIDE_PACKED_RGB */

#if GLIDE_FP_CLAMP_TEX
            while(*dataList != 0) {       
              TRI_SETF(FARRAY(fb, *dataList));
              dataList++;
            }

            dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
          
            while (*dataList != 0) {
              TRI_SETF_CLAMP(FARRAY(fb, *dataList));
              dataList++;
            }
          }
        
          TRI_SETF(fa[GR_VERTEX_X_OFFSET] + fBias);
          TRI_SETF(fa[GR_VERTEX_Y_OFFSET]);

          {
            const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
          if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0) {
            FxU32 packedColor = 0x00;
            
            if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
              packedColor = (RGBA_COMP_CLAMP(FARRAY(fa, (GR_VERTEX_B_OFFSET << 2)), B) | 
                             RGBA_COMP_CLAMP(FARRAY(fa, (GR_VERTEX_G_OFFSET << 2)), G) |
                             RGBA_COMP_CLAMP(FARRAY(fa, (GR_VERTEX_R_OFFSET << 2)), R));
              
              dataList++;
            }
            
            if (*dataList == (GR_VERTEX_A_OFFSET << 2)) {
              packedColor |= RGBA_COMP_CLAMP(FARRAY(fa, (GR_VERTEX_A_OFFSET << 2)), A);
              dataList++;
            }
            
            TRI_SET(packedColor);
          }
#endif /* GLIDE_PACKED_RGB */

#if GLIDE_FP_CLAMP_TEX
            while(*dataList != 0) {       
              TRI_SETF(FARRAY(fa, *dataList));
              dataList++;
            }

            dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
          
            while (*dataList != 0) {
              TRI_SETF_CLAMP(FARRAY(fa, *dataList));
              dataList++;
            }
          }
        
          fBias *= -1.0f;
        }
      }
    }
    TRI_END;
  }
  GR_CHECK_SIZE();
#else /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
#ifdef GLIDE_USE_ALT_REGMAP
  hw = SST_WRAP(hw,128);        /* use alternate register mapping */
#endif

  GR_SET_EXPECTED_SIZE(12 + _GlideRoot.curTriSize, (_GlideRoot.curTriSize + 12) >> 2);  
  {
    float    m, dp;
    struct dataList_s* dlp;

    /*
    ** X major line
    */
    if (j >= i) {               /* if (adx > ady) */
      if (j == 0) goto all_done; /* check for zero-length lines */
      /* start up divide and overlap with as much integer stuff as possible*/
      m = _GlideRoot.pool.f1 / DX;
      dlp = gc->regDataList;
      GR_SETF(BROADCAST_ID, hw, FvA.x, a->x);
      dp = b->x;
      GR_SETF(BROADCAST_ID, hw, FvB.x, dp);
      GR_SETF(BROADCAST_ID, hw, FvC.x, dp);
      
      GR_SETF(BROADCAST_ID, hw, FvA.y, a->y - _GlideRoot.pool.fHalf);

      dp = b->y;
      GR_SETF(BROADCAST_ID, hw, FvB.y, dp - _GlideRoot.pool.fHalf);

      i = dlp->i;
      GR_SETF(BROADCAST_ID, hw, FvC.y, dp + _GlideRoot.pool.fHalf);

      while (i) {
        dp = FARRAY(a,i);
        GR_SETF_INDEX(BROADCAST_ID, hw, 
                      ((FxU32*)dlp->addr - (FxU32*)hw), dp);

        dp = FARRAY(b,i) - dp;
        GR_SETF_INDEX(BROADCAST_ID, hw, 
                      ((FxU32*)&dlp->addr[DPDX_OFFSET >> 2] - (FxU32*)hw), dp * m);

        dlp++;
        i = dlp->i;
        GR_SETF_INDEX(BROADCAST_ID, hw, 
                      ((FxU32*)&dlp->addr[DPDY_OFFSET >> 2] - (FxU32*)hw), _GlideRoot.pool.f0);
      }
      GR_SETF(BROADCAST_ID, hw, FtriangleCMD,_GlideRoot.pool.ftemp1);

      GR_SETF(BROADCAST_ID, hw, FvB.x,a->x);
      GR_SETF(BROADCAST_ID, hw, FvB.y,a->y + _GlideRoot.pool.fHalf);
      GR_SETF(BROADCAST_ID, hw, FtriangleCMD,-_GlideRoot.pool.ftemp1);
    }

    /*
    ** Y major line
    */
    else {
      m = _GlideRoot.pool.f1 / ADY;
      dlp = gc->regDataList;
      GR_SETF(BROADCAST_ID, hw, FvA.y,a->y);
      dp = b->y;
      GR_SETF(BROADCAST_ID, hw, FvB.y,dp);
      GR_SETF(BROADCAST_ID, hw, FvC.y,dp);
     
      GR_SETF(BROADCAST_ID, hw, FvA.x,a->x - _GlideRoot.pool.fHalf);

      dp = b->x;
      GR_SETF(BROADCAST_ID, hw, FvB.x,dp - _GlideRoot.pool.fHalf);

      i = dlp->i;
      GR_SETF(BROADCAST_ID, hw, FvC.x,dp + _GlideRoot.pool.fHalf);
       
      while (i) {
        dp = FARRAY(a,i);
        GR_SETF_INDEX(BROADCAST_ID, hw, ((FxU32*)dlp->addr - (FxU32*)hw), dp);

        dp = FARRAY(b,i) - dp;
        GR_SETF_INDEX(BROADCAST_ID, hw,
                      ((FxU32*)&dlp->addr[DPDX_OFFSET >> 2] - (FxU32*)hw), _GlideRoot.pool.f0);

        dlp++;
        i = dlp->i;
        GR_SETF_INDEX(BROADCAST_ID, hw, 
                      ((FxU32*)&dlp->addr[DPDY_OFFSET >> 2] - (FxU32*)hw), dp * m);
      }
      GR_SET(BROADCAST_ID, hw, triangleCMD, 0xFFFFFFFF);
     
      GR_SETF(BROADCAST_ID, hw, FvB.x, a->x + _GlideRoot.pool.fHalf);
      GR_SETF(BROADCAST_ID, hw, FvB.y, a->y);
      GR_SET(BROADCAST_ID, hw, triangleCMD, 1);
    }
  }
  GR_CHECK_SIZE();
#endif /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */

all_done:                       /* come here on degenerate lines */
  _GlideRoot.stats.linesDrawn++;

  GR_END();
#undef FN_NAME
} /* grDrawLine */

/*---------------------------------------------------------------------------
 ** grDrawTriangle
 */
#ifndef HAVE_XDRAWTRI_ASM	/* grDrawTriangle() not in asm */
#if defined(_MSC_VER) && !GLIDE_USE_C_TRISETUP && !defined(GLIDE_DEBUG)
__declspec(naked)
#endif
GR_ENTRY(grDrawTriangle, void, (const GrVertex *a, const GrVertex *b, const GrVertex *c))
{
#define FN_NAME grDrawTriangle
#if defined(GLIDE_USE_C_TRISETUP) || defined(__WATCOMC__) || defined(GLIDE_DEBUG)
  GR_BEGIN_NOFIFOCHECK("grDrawTriangle",92);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x,0x%x,0x%x)\n",a,b,c);
  GR_CHECK_F(myName, !a || !b || !c, "NULL pointer passed");

  GR_FLUSH_STATE();

  /* Silly warning killer */
  if (0) goto all_done;

#ifdef GLIDE_DEBUG
  if (_GlideRoot.environment.triBoundsCheck) {
    if (OUTBOUNDS(a) || OUTBOUNDS(b) || OUTBOUNDS(c)) {
      GDBG_PRINTF("Triangle out of bounds:\n");
      GDBG_PRINTF("a->x = %3.2f, a->y = %3.2f\n", a->x, a->y);
      GDBG_PRINTF("b->x = %3.2f, b->y = %3.2f\n", b->x, b->y);
      GDBG_PRINTF("c->x = %3.2f, c->y = %3.2f\n", c->x, c->y);
      GDBG_PRINTF("Culling triangle based on these bogus values.\n");
      goto all_done;
    }
  }
#endif /* GLIDE_DEBUG */

  /* _trisetup and _trisetup_asm return 0 if culled, 1 if drawn */ 
#if GLIDE_DEBUG && !GLIDE_USE_C_TRISETUP
  /* HackAlert: Nuke the fifo ptr checking stuff here if we're just
   * debugging teh asm tri code.
   */
  if (TRISETUP(a, b, c) != 0) {
    gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr;
    gc->checkCounter = 0;
  }
#else
  TRISETUP(a, b, c);
#endif

all_done:
  GR_END();

#elif defined(_MSC_VER)
  {
    __asm {
      mov edx, [_GlideRoot + kCurGCOffset];
      mov eax, [edx + kTriProcOffset];
      jmp eax;
    }
  }

#else
#error "Write triangle proc dispatch for this compiler"
#endif
#undef FN_NAME
} /* grDrawTriangle */
#endif /* HAVE_XDRAWTRI_ASM */

/*---------------------------------------------------------------------------
 ** grDrawPlanarPolygon
 **
 ** Brute force "triangle-fan" implementation of a convex polygon drawer.
 */
GR_ENTRY(grDrawPlanarPolygon,
         void,
         (int nVerts, const int iList[], const GrVertex vList[]))
{
  GR_BEGIN_NOFIFOCHECK("grDrawPlanarPolygon",93);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x,0x%x)\n",nVerts,iList,vList);
  GR_CHECK_F(myName, !iList || !vList, "NULL pointer passed");

  GR_FLUSH_STATE();

#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  grDrawPolygon(nVerts, iList, vList);
#else /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
  {
    int n,i = nVerts-2;
    const GrVertex *firstv = &vList[iList[0]];

    for (i = 1; i < nVerts - 1; i++) {
      n = TRISETUP(firstv, &vList[iList[i]], &vList[iList[i+1]]);
      if (n > 0) break;         /* stop after 1st non-zero-area triangle */
      if (n < 0) goto all_done;
    }
    /* now all the gradients are loaded into the chip, so we just have to */
    /* draw all the rest of the triangles */
    for (i = i+1; i < nVerts - 1; i++) {
      _trisetup_nogradients(firstv, &vList[iList[i]], &vList[iList[i+1]]);
    }
  }
all_done:
#endif /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */

  GR_END();
} /* grDrawPlanarPolygon */

/*---------------------------------------------------------------------------
 ** grDrawPlanarPolygonVertexList
 **
 ** Brute force "triangle-fan" implementation of a convex polygon drawer.
 */

GR_ENTRY(grDrawPlanarPolygonVertexList, void, (int nVerts, const GrVertex vList[]))
{
  GR_BEGIN_NOFIFOCHECK("grDrawPlanarPolygonVertexList",93);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x)\n",nVerts,vList);
  GR_CHECK_F(myName, !vList, "NULL pointer passed");
   
  GR_FLUSH_STATE();

#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  grDrawPolygonVertexList(nVerts, vList);
#else /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
  {
    int n,i = nVerts-2;
    const GrVertex *firstv = &vList[0];
      
    for (i = 1; i < nVerts - 1; i++) {
      n = TRISETUP(firstv, &vList[i], &vList[i+1]);
      if (n > 0) break;         /* stop after 1st non-zero-area triangle */
      if (n < 0) goto all_done;
    }
    /* now all the gradients are loaded into the chip, so we just have to */
    /* draw all the rest of the triangles */
    for (i = i+1; i < nVerts - 1; i++) {
      _trisetup_nogradients(firstv, &vList[i], &vList[i+1]);
    }
      
all_done:
    ;
  }
#endif /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */

  GR_END();
} /* grDrawPlanarPolygonVertexList */

#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
#define kMaxPacket3Vertex 0x0FUL

/* Packet 3 requires at least one vertex, if there isn't enough room
 * in the fifo then force a wrap now and write up to the max.  
 */
#define FIFO_VERT(__vertSize, __polyVerts) \
   ((gc->cmdTransportInfo.fifoRoom  < (FxI32)(sizeof(FxU32) + (__vertSize))) \
    ? (__polyVerts) \
    : MIN((__polyVerts), ((gc->cmdTransportInfo.fifoRoom - sizeof(FxU32)) / (__vertSize))))

#define FIFO_MAX_VERT(__polyVerts) MIN(kMaxPacket3Vertex, (__polyVerts))
#endif /* GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP */

/*---------------------------------------------------------------------------
 ** grDrawPolygon
 **
 ** Brute force "triangle-fan" implementation of a convex polygon drawer.
 */

GR_ENTRY(grDrawPolygon, void, (int nVerts, const int iList[], const GrVertex vList[]))
{
#define FN_NAME "grDrawPolygon"
  GR_BEGIN_NOFIFOCHECK("grDrawPolygon",93);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x,0x%x)\n",nVerts,iList,vList);
  GR_CHECK_F(myName, !iList || !vList, "NULL pointer passed");

  GR_FLUSH_STATE();

  /* Zero length vertex lists are allowed, but we can't send them to
   * the hw.
   */
  if (nVerts <= 0) goto __exitNoVerts;
   
#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  {
    const FxU32 vertexParamOffset = _GlideRoot.curVertexSize;
    const int* vertexIndexP = iList;
    FxU32 packVertMax = FIFO_MAX_VERT(nVerts);                   /* Max possible verts for this packet */
    FxU32 packVerts = FIFO_VERT(vertexParamOffset, packVertMax); /* # verts for this packet */
    FxU32 packType = SSTCP_PKT3_BDDDDD;

__doPolyVertexSend:
    {
      const FxU32 packSize = packVerts * vertexParamOffset;
      FxU32 vertexOffset;

      GR_ASSERT((packSize % vertexParamOffset) == 0);
      GR_ASSERT(packSize >= vertexParamOffset);

      GR_SET_EXPECTED_SIZE(packSize, 1);
      TRI_STRIP_BEGIN(kSetupFan,
                      packSize / vertexParamOffset, vertexParamOffset,
                      packType);
      for(vertexOffset = 0; vertexOffset < packSize; vertexOffset += vertexParamOffset) {
        const float* vertex = (const float*)(vList + *vertexIndexP++);
        
        TRI_SETF(vertex[GR_VERTEX_X_OFFSET]);
        TRI_SETF(vertex[GR_VERTEX_Y_OFFSET]);

        {
          const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
          if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0)
          {
            FxU32 packedColor = 0x00;
          
            if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
              packedColor = (RGBA_COMP_CLAMP(FARRAY(vertex, (GR_VERTEX_B_OFFSET << 2)), B) | 
                             RGBA_COMP_CLAMP(FARRAY(vertex, (GR_VERTEX_G_OFFSET << 2)), G) |
                             RGBA_COMP_CLAMP(FARRAY(vertex, (GR_VERTEX_R_OFFSET << 2)), R));
              dataList++;
            }
            
            if (*dataList == (GR_VERTEX_A_OFFSET << 2)) {
              packedColor |= RGBA_COMP_CLAMP(FARRAY(vertex, (GR_VERTEX_A_OFFSET << 2)), A);
              dataList++;
            }
            
            TRI_SET(packedColor);
          }
#endif /* GLIDE_PACKED_RGB */

#if GLIDE_FP_CLAMP_TEX
          while(*dataList != 0) {       
            TRI_SETF(FARRAY(vertex, *dataList));
            dataList++;
          }

          dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
          
          while (*dataList != 0) {
            TRI_SETF_CLAMP(FARRAY(vertex, *dataList));
            dataList++;
          }
        }
      }
      TRI_END;
      GR_CHECK_SIZE();
    }

    /* Are there more vertices that need to be sent for this
     * polygon? Reset the parameters and do the rest of them.
     */
    if (vertexIndexP < iList + nVerts) {
      /* Reset the # of verts maxing out the send based on the
       * space left in the fifo and the max packet size.
       */
      nVerts -= packVerts;
      packVertMax = FIFO_MAX_VERT(nVerts);                   /* Max possible verts for this packet */
      packVerts = FIFO_VERT(vertexParamOffset, packVertMax); /* # verts for this packet */

      packType = SSTCP_PKT3_DDDDDD;
         
      GDBG_INFO(120, "\tSending continueing polygon data (0x%X : 0x%X)\n", nVerts, packVerts);
        
      goto __doPolyVertexSend;
    }
  }
#else /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
  {
    int i;
    for (i = 1; i < nVerts - 1; i++) {
      grDrawTriangle(&vList[iList[0]], &vList[iList[i]], &vList[iList[i+1]]);
    }
  }
#endif /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */

__exitNoVerts:
  GR_END();
#undef FN_NAME
} /* grDrawPolygon */

/*---------------------------------------------------------------------------
 ** grDrawPolygonVertexList
 **
 ** Brute force "triangle-fan" implementation of a convex polygon drawer.
 */

GR_ENTRY(grDrawPolygonVertexList, void, (int nVerts, const GrVertex vList[]))
{
#define FN_NAME "grDrawPolygonVertexList"
  GR_BEGIN_NOFIFOCHECK("grDrawPolygonVertexList", 92);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x)\n",nVerts,vList);
  GR_CHECK_F(myName, !vList, "NULL pointer passed");

  GR_FLUSH_STATE();

  /* Zero length vertex lists are allowed, but we can't send them to
   * the hw.
   */
  if (nVerts <= 0) goto __exitNoVerts;
   
#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
  {
    const FxU32 vertexParamOffset = _GlideRoot.curVertexSize;
    const GrVertex* vertexListP = vList;
    FxU32 packVertMax = FIFO_MAX_VERT(nVerts);                   /* Max possible verts for this packet */
    FxU32 packVerts = FIFO_VERT(vertexParamOffset, packVertMax); /* # verts for this packet */
    FxU32 packType = SSTCP_PKT3_BDDDDD;

__doPolyVertexSend:
    GR_ASSERT(packVerts > 0);
    GR_ASSERT(packVerts <= kMaxPacket3Vertex);

    {
      const FxU32 packSize = packVerts * vertexParamOffset;
      FxU32 vertexOffset;
      
      GR_SET_EXPECTED_SIZE(packSize, 1);
      TRI_STRIP_BEGIN(kSetupFan, packVerts, vertexParamOffset, packType);

      for(vertexOffset = 0; vertexOffset < packSize; vertexOffset += vertexParamOffset) {
        const float* vertex = (const float*)vertexListP++;
        
        TRI_SETF(vertex[GR_VERTEX_X_OFFSET]);
        TRI_SETF(vertex[GR_VERTEX_Y_OFFSET]);

        {
          const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
          if ((gc->cmdTransportInfo.paramMask & SSTCP_PKT3_PACKEDCOLOR) != 0)
          {
            FxU32 packedColor = 0x00;
          
            if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
              packedColor = (RGBA_COMP_CLAMP(FARRAY(vertex, (GR_VERTEX_B_OFFSET << 2)), B) | 
                             RGBA_COMP_CLAMP(FARRAY(vertex, (GR_VERTEX_G_OFFSET << 2)), G) |
                             RGBA_COMP_CLAMP(FARRAY(vertex, (GR_VERTEX_R_OFFSET << 2)), R));
              dataList++;
            }
            
            if (*dataList == (GR_VERTEX_A_OFFSET << 2)) {
              packedColor |= RGBA_COMP_CLAMP(FARRAY(vertex, (GR_VERTEX_A_OFFSET << 2)), A);
              dataList++;
            }
            
            TRI_SET(packedColor);
          }
#endif /* GLIDE_PACKED_RGB */

#if GLIDE_FP_CLAMP_TEX
          while(*dataList != 0) {
            TRI_SETF(FARRAY(vertex, *dataList));
            dataList++;
          }

          dataList++;
#endif /* GLIDE_FP_CLAMP_TEX */
          
          while (*dataList != 0) {
            TRI_SETF_CLAMP(FARRAY(vertex, *dataList));
            dataList++;
          }
        }
      }
      TRI_END;
      GR_CHECK_SIZE();
    }

    /* More verts? */
    if (vertexListP < vList + nVerts) {
      /* Reset the # of verts maxing out the send based on the
       * space left in the fifo and the max packet size.
       */
      nVerts -= packVerts;
      packVertMax = FIFO_MAX_VERT(nVerts);                   /* Max possible verts for this packet */
      packVerts = FIFO_VERT(vertexParamOffset, packVertMax); /* # verts for this packet */

      /* Packet type to continue strip */
      packType = SSTCP_PKT3_DDDDDD;
         
      GDBG_INFO(120, "\tSending continueing polygon data (0x%X : 0x%X)\n", packVerts, nVerts);
        
      goto __doPolyVertexSend;
    }
  }
#else /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */
  {
    int i;
    
    for (i = 1; i < nVerts - 1; i++) {
      grDrawTriangle(&vList[0], &vList[i], &vList[i+1]);
    }
  }
#endif /* !(GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP) */

__exitNoVerts:
  GR_END();
#undef FN_NAME
} /* grDrawPolygonVertexList */

/*---------------------------------------------------------------------------
 ** _grColorCombineDelta0Mode
 **
 ** GMT: when we are in delta0 mode, color comes from the RGB iterators
 **      but the slopes are 0.0.  So when we enter delta0 mode we set
 **      the iterators up and then we leave them alone during primitive
 **      rendering
 */

GR_DDFUNC(_grColorCombineDelta0Mode, void, (FxBool delta0mode))
{
#define FN_NAME "_grColorCombineDelta0Mode"
  GR_BEGIN_NOFIFOCHECK("_grColorCombineDelta0Mode",85);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",delta0mode);

  if (delta0mode) {
    GR_SET_EXPECTED_SIZE(9 << 2, 9);
    {
      GR_SETF(BROADCAST_ID, hw, Fr, gc->state.r);
      GR_SETF(BROADCAST_ID, hw, Fg, gc->state.g);
      GR_SETF(BROADCAST_ID, hw, Fb, gc->state.b);
      GR_SET(BROADCAST_ID, hw, drdx, 0);
      GR_SET(BROADCAST_ID, hw, drdy, 0);
      GR_SET(BROADCAST_ID, hw, dgdx, 0);
      GR_SET(BROADCAST_ID, hw, dgdy, 0);
      GR_SET(BROADCAST_ID, hw, dbdx, 0);
      GR_SET(BROADCAST_ID, hw, dbdy, 0);
    } 
    GR_CHECK_SIZE();
  }

  gc->state.cc_delta0mode = delta0mode;
  GR_END();
#undef FN_NAME
} /* _grColorCombineDeltaMode */

