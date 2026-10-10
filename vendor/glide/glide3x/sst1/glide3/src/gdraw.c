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
** Revision 1.1.2.8  2006/01/16 21:22:45  jwrdegoede
** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
**
** Revision 1.1.2.7  2005/06/16 18:58:32  jwrdegoede
** Fix 2 sst1 bugs accidently introduced by warning fixes
**
** Revision 1.1.2.6  2005/06/09 18:32:35  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.2.5  2005/05/25 08:51:52  jwrdegoede
** Add #ifdef GL_X86 around x86 specific code
**
** Revision 1.1.2.4  2005/05/10 11:27:23  jwrdegoede
** sst1 gcc4 compile fixes
**
** Revision 1.1.2.3  2004/10/08 06:30:19  dborca
** save a round-trip with grDrawTriangle in SST1
**
** Revision 1.1.2.2  2004/10/04 09:35:59  dborca
** second cut at Glide3x for Voodoo1/Rush (massive update):
** delayed validation, vertex snapping, clip coordinates, strip/fan_continue, bugfixes.
** and probably a bunch of other crap that I forgot
**
** Revision 1.1.2.1  2004/03/02 07:55:30  dborca
** Bastardised Glide3x for SST1
**
** Revision 1.1.1.1  1999/12/07 21:48:52  joseph
** Initial checkin into SourceForge.
**
** 
** 31    6/02/98 8:03p Peter
** Mmmmm.... points
 * 
 * 30    9/07/97 1:52p Atai
 * performance tuning for previous check-in
 * 
 * 29    9/05/97 7:20p Atai
 * fixed bug 764 (grDrawPoint integer snapping).
 * 
 * 28    6/20/97 5:51p Dow
 * Moved grDebugGroupWriteHEader to sst96.c
 * 
 * 27    5/27/97 11:37p Pgj
 * Fix for Bug report 545
 * 
 * 26    5/27/97 2:00p Dow
 * Parenthesized a ternary op in GR_BEGIN to work around a Watcom compiler
 * bug.
 * 
 * 25    3/21/97 12:42p Dow
 * Made STWHints not send the Bend Over Baby Packet to FBI Jr.
 * 
 * 24    3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 23    2/18/97 9:51a Jdt
 * Ifdefed out some if(0) code to supress watcom warnings
 * 
 * 22    12/23/96 1:37p Dow
 * chagnes for multiplatform glide
 * 
 * 21    11/18/96 12:13p Jdt
 * Made all debugging code go through the C-call to the assembly trisetup
 * code.
 * 
 * 20    11/14/96 11:52p Jdt
 * Fixed Watcom Assembly calling bug
**
*/
#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#include <memory.h>

#define SST_XY_HALF      ( 1 << ( SST_XY_FRACBITS - 1 ) )
#define SST_XY_ONE       ( 1 << SST_XY_FRACBITS )

#define OUTBOUNDSX(a) ((*(float *)a < 0.f ? 1 : 0) || (*(float *)a > gc->state.screen_width ? 1 : 0))
#define OUTBOUNDSY(a) ((*((float *)a+1) < 0.f ? 1 : 0) || (*((float *)a+1) > gc->state.screen_height ? 1 : 0))
#define OUTBOUNDS(a) (OUTBOUNDSX(a) || OUTBOUNDSY(a))

/* access a floating point array with a byte index */
#define FARRAY(p,i) (*(float *)((i)+(int)(p)))
/* access a byte array with a byte index and convert to float */
#define FbARRAY(p,i) (float)(((unsigned char *)p)[i])

/*---------------------------------------------------------------------------
  NOTE: by Gary Tarolli
  
  The following code is very carefully scheduled for MSVC4.2 Version 10.20.6166
  The trick is to schedule operations between PCI writes (GR_SET*).
  This is because PCI writes cannot get off the CPU chip quickly and there
  are about 6-12 dead clocks between back-to-back writes (P-90 to P-166).
  So we try to do useful work during this time - this makes the code hard to
  read because simple operations like mem++ may be broken up into three lines
  of C code to perform, to allow the compiler to move the pieces around more.
  PLEASE PLEASE PLEASE don't touch this code without benchmarkng it before
  and after you touch it.  Moving a line of code up/down changes performance!
  ---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
** grDrawPoint
*/

GR_ENTRY(grDrawPoint, void, ( const void *p ))
{
  GR_DCL_GC;
  if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_POINTS_MASK)
    _grAADrawPoints(GR_VTX_PTR_ARRAY, 1, (void *)&p);
  else
    _grDrawPoints(GR_VTX_PTR_ARRAY, 1, (void *)&p);
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

GR_ENTRY(grDrawLine, void, ( const void *a, const void *b ))
{
  GR_DCL_GC;

  if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_LINES_MASK)
    _grAADrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, 2, (void *)&a);
  else
    _grDrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, 2, (void *)&a);
} /* grDrawLine */

/*---------------------------------------------------------------------------
** grDrawTriangle
*/

#if defined(GLIDE_USE_C_TRISETUP)
GR_ENTRY(grDrawTriangle, void, ( const void *a, const void *b, const void *c ))
{
  GR_BEGIN_NOFIFOCHECK("grDrawTriangle",92);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x,0x%x,0x%x)\n",a,b,c));
  GR_CHECK_F(myName, !a || !b || !c, "NULL pointer passed");

  /* Silly warning killer */
  if ( 0 ) goto all_done;

#ifdef GLIDE_DEBUG

  if (_GlideRoot.environment.triBoundsCheck) {
    if (OUTBOUNDS(a) || OUTBOUNDS(b) || OUTBOUNDS(c)) {
      GDBG_PRINTF(("Triangle out of bounds:\n"));
      GDBG_PRINTF(("a->x = %3.2f, a->y = %3.2f\n", *(float *)a, *((float *)a+1)));
      GDBG_PRINTF(("b->x = %3.2f, b->y = %3.2f\n", *(float *)b, *((float *)b+1)));
      GDBG_PRINTF(("c->x = %3.2f, c->y = %3.2f\n", *(float *)c, *((float *)c+1)));
      GDBG_PRINTF(("Culling triangle based on these bogus values.\n"));
      goto all_done;
    }
  }
#endif /* GLIDE_DEBUG */

  /* _trisetup and _trisetup_asm return 0 if culled, 1 if drawn */ 
  TRISETUP( a, b, c );

all_done:
  GR_END();
} /* grDrawTriangle */
#endif

/*---------------------------------------------------------------------------
** grVpDrawTriangle
*/
#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
FxI32 FX_CSTYLE
_grVpDrawTriangle( const void *va, const void *vb, const void *vc )
{
  GR_DCL_GC;
  FxI32 xindex = (gc->state.vData.vertexInfo.offset >> 2);
  FxI32 yindex = xindex + 1;
  const float *fa = (const float *)va + xindex;
  const float *fb = (const float *)vb + xindex;
  const float *fc = (const float *)vc + xindex;
  float ooa, dxAB, dxBC, dyAB, dyBC;
  int i,culltest;
  union { float f; int i; } ay;
  union { float f; int i; } by;
  union { float f; int i; } cy;
  float *fp;
  struct dataList_s *dlp;
  float oowa, oowb, oowc;
  int k;
  volatile FxU32 *fifoPtr;
  float snap_xa, snap_ya, snap_xb, snap_yb, snap_xc, snap_yc;
  float tmp_snap_ya, tmp_snap_yb, tmp_snap_yc;
  FxU32 paramIndex = gc->state.paramIndex;

  culltest = gc->state.cull_mode; /* 1 if negative, 0 if positive */
  _GlideRoot.stats.trisProcessed++;

  /*
  ** oow
  */
  oowa = 1.0f / FARRAY(fa, gc->state.vData.wInfo.offset);
  oowb = 1.0f / FARRAY(fb, gc->state.vData.wInfo.offset);
  oowc = 1.0f / FARRAY(fc, gc->state.vData.wInfo.offset);
  
  /*
   **  Sort the vertices.
   **  Whenever the radial order is reversed (from counter-clockwise to
   **  clockwise), we need to change the area of the triangle.  Note
   **  that we know the first two elements are X & Y by looking at the
   **  grVertex structure.  
   */
  snap_ya = tmp_snap_ya = (volatile float) (*((const float *)va + yindex) * oowa * gc->state.Viewport.hheight + gc->state.Viewport.oy + SNAP_BIAS);
  snap_yb = tmp_snap_yb = (volatile float) (*((const float *)vb + yindex) * oowb * gc->state.Viewport.hheight + gc->state.Viewport.oy + SNAP_BIAS);
  snap_yc = tmp_snap_yc = (volatile float) (*((const float *)vc + yindex) * oowc * gc->state.Viewport.hheight + gc->state.Viewport.oy + SNAP_BIAS);

  ay.f = tmp_snap_ya;
  by.f = tmp_snap_yb;
  cy.f = tmp_snap_yc;
  if (ay.i < 0) ay.i ^= 0x7FFFFFFF;
  if (by.i < 0) by.i ^= 0x7FFFFFFF;
  if (cy.i < 0) cy.i ^= 0x7FFFFFFF;
  if (ay.i < by.i) {
    if (by.i > cy.i) {              /* acb */
      if (ay.i < cy.i) {
        fa = (const float *)va + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)vb + xindex;
        snap_ya = tmp_snap_ya;
        snap_yb = tmp_snap_yc;
        snap_yc = tmp_snap_yb;
        culltest ^= 1;
      } else {                  /* cab */
        fa = (const float *)vc + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vb + xindex;
        snap_ya = tmp_snap_yc;
        snap_yb = tmp_snap_ya;
        snap_yc = tmp_snap_yb;
      }
      /* else it's already sorted */
    }
  } else {
    if (by.i < cy.i) {              /* bac */
      if (ay.i < cy.i) {
        fa = (const float *)vb + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vc + xindex;
        snap_ya = tmp_snap_yb;
        snap_yb = tmp_snap_ya;
        snap_yc = tmp_snap_yc;
        culltest ^= 1;
      } else {                  /* bca */
        fa = (const float *)vb + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)va + xindex;
        snap_ya = tmp_snap_yb;
        snap_yb = tmp_snap_yc;
        snap_yc = tmp_snap_ya;
      }
    } else {                    /* cba */
      fa = (const float *)vc + xindex;
      fb = (const float *)vb + xindex;
      fc = (const float *)va + xindex;
      snap_ya = tmp_snap_yc;
      snap_yb = tmp_snap_yb;
      snap_yc = tmp_snap_ya;
      culltest ^= 1;
    }
  }

  snap_xa = (volatile float) (*((const float *)fa + xindex) * oowa * gc->state.Viewport.hwidth + gc->state.Viewport.ox + SNAP_BIAS);
  snap_xb = (volatile float) (*((const float *)fb + xindex) * oowb * gc->state.Viewport.hwidth + gc->state.Viewport.ox + SNAP_BIAS);
  snap_xc = (volatile float) (*((const float *)fc + xindex) * oowc * gc->state.Viewport.hwidth + gc->state.Viewport.ox + SNAP_BIAS);

  /* Compute Area */
  dxAB = snap_xa - snap_xb;
  dxBC = snap_xb - snap_xc;
  
  dyAB = snap_ya - snap_yb;
  dyBC = snap_yb - snap_yc;
  
  /* this is where we store the area */
  _GlideRoot.pool.temp1.f = dxAB * dyBC - dxBC * dyAB;
  
  /* Zero-area triangles are BAD!! */
  if ((_GlideRoot.pool.temp1.i & 0x7FFFFFFF) == 0)
    return 0;
  
  /* Backface culling, use sign bit as test */
  if (gc->state.cull_mode != GR_CULL_DISABLE) {
    if ((_GlideRoot.pool.temp1.i ^ (culltest<<31)) >= 0) {
      return -1;
    }
  }
  
#if GL_X86  
  /* Fence On P6 If Necessary */
  if ( _GlideRoot.CPUType == 6 ) {
      /* In the macro there is a slop of 4 DWORDS that I have removed */
      if ( (gc->hwDep.sst96Dep.writesSinceFence + ( _GlideRoot.curTriSize >> 2 )) > 128 ) {
          P6FENCE;
          gc->hwDep.sst96Dep.writesSinceFence = 0;
      }
      gc->hwDep.sst96Dep.writesSinceFence      += 
          _GlideRoot.curTriSize>>2;
  }
#endif

  /* Wrap Fifo now if triangle is going to incur a wrap */
  if (gc->fifoData.hwDep.vg96FIFOData.fifoSize < (FxU32) _GlideRoot.curTriSize ) {
#if SST96_ALT_FIFO_WRAP
    gc->fifoData.hwDep.vg96FIFOData.blockSize = _GlideRoot.curTriSize;
    initWrapFIFO(&gc->fifoData);
#else
    _grSst96FifoMakeRoom();
#endif
  }
  
  GR_FLUSH_STATE();

  GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize);
  
  /* Grab fifo pointer into a local */
  fifoPtr = gc->fifoData.hwDep.vg96FIFOData.fifoPtr;
  
  /* Settle Bookeeping */
  gc->fifoData.hwDep.vg96FIFOData.fifoSize -= _GlideRoot.curTriSize;
  gc->fifoData.hwDep.vg96FIFOData.fifoPtr  += _GlideRoot.curTriSize>>2;
  
  /* Start first group write packet */
  SET_GW_CMD(    fifoPtr, 0, gc->hwDep.sst96Dep.gwCommand );
  SET_GW_HEADER( fifoPtr, 1, gc->hwDep.sst96Dep.gwHeaders[0] );

  ooa = _GlideRoot.pool.f1 / _GlideRoot.pool.temp1.f;
  /* GMT: note that we spread out our PCI writes */
  /* write out X & Y for vertex A */
  FSET_GW_ENTRY( fifoPtr, 2, snap_xa );
  FSET_GW_ENTRY( fifoPtr, 3, snap_ya );

  dlp = gc->dataList;
  i = dlp->i;
  
  /* write out X & Y for vertex B */
  FSET_GW_ENTRY( fifoPtr, 4, snap_xb );
  FSET_GW_ENTRY( fifoPtr, 5, snap_yb );
  
  /* write out X & Y for vertex C */
  FSET_GW_ENTRY( fifoPtr, 6, snap_xc );
  FSET_GW_ENTRY( fifoPtr, 7, snap_yc );
  fifoPtr += 8;

  /*
  ** Divide the deltas by the area for gradient calculation.
  */
  dxBC *= ooa;
  dyAB *= ooa;
  dxAB *= ooa;
  dyBC *= ooa;

  /* 
  ** The src vector contains offsets from fa, fb, and fc to for which
  **  gradients need to be calculated, and is null-terminated.
  */
  if (paramIndex & STATE_REQUIRES_IT_DRGB) {
    for (k = 0; k < 3; k++) {
      fp = dlp->addr;
      /* chip field change */
      if (i & 1) {
        paramIndex &= ~STATE_REQUIRES_IT_DRGB;
        goto secondary_packet;
      }
      else {
        float dpAB, dpBC,dpdx, dpdy;
        FxU32 bddr = dlp->bddr;
        
        if (bddr) {
          dpBC = FbARRAY(fb,bddr);
          dpdx = FbARRAY(fa,bddr);
          FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
          dpAB = dpdx - dpBC;
          dpBC = dpBC - FbARRAY(fc,bddr);
        }
        else {
          dpBC = FARRAY(fb,i) * _GlideRoot.pool.f255;
          dpdx = FARRAY(fa,i) * _GlideRoot.pool.f255;
          FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
          dpAB = dpdx - dpBC;
          dpBC = dpBC - FARRAY(fc,i) * _GlideRoot.pool.f255;
        }
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        FSET_GW_ENTRY( fifoPtr, 1, dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;

        dlp++;
        i = dlp->i;
        FSET_GW_ENTRY( fifoPtr, 2, dpdy );
        fifoPtr += 3;
      }
    }
  }
  if (paramIndex & STATE_REQUIRES_OOZ) {
    fp = dlp->addr;
    /* chip field change */
    if (i & 1) {
      paramIndex &= ~STATE_REQUIRES_OOZ;
      goto secondary_packet;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      dpBC = FARRAY(fb,i) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      dpdx = FARRAY(fa,i) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * oowc * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_IT_ALPHA) {
    fp = dlp->addr;
    /* chip field change */
    if (i & 1) {
      paramIndex &= ~STATE_REQUIRES_IT_ALPHA;
      goto secondary_packet;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      FxU32 bddr = dlp->bddr;
      if (bddr) {
        dpBC = FbARRAY(fb,bddr);
        dpdx = FbARRAY(fa,bddr);
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FbARRAY(fc,bddr);
      }
      else {
        dpBC = FARRAY(fb,i) * _GlideRoot.pool.f255;
        dpdx = FARRAY(fa,i) * _GlideRoot.pool.f255;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * _GlideRoot.pool.f255;
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_ST_TMU0) {
    for (k = 0; k < 2; k++) {
      fp = dlp->addr;
      /* chip field change */
      if (i & 1) {
        paramIndex &= ~STATE_REQUIRES_ST_TMU0;
        goto secondary_packet;
      }
      else {
        float dpAB, dpBC,dpdx, dpdy;
        
        dpBC = FARRAY(fb,i) * oowb * gc->state.tmu_config[0].st_scale[k];
        dpdx = FARRAY(fa,i) * oowa * gc->state.tmu_config[0].st_scale[k];
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc * gc->state.tmu_config[0].st_scale[k];
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        FSET_GW_ENTRY( fifoPtr, 1, dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;
        
        dlp++;
        i = dlp->i;
        FSET_GW_ENTRY( fifoPtr, 2, dpdy );
        fifoPtr += 3;
      }
    }
  }
  if (paramIndex & STATE_REQUIRES_OOW_FBI) {
    fp = dlp->addr;
    /* chip field change */
    if (i & 1) {
      paramIndex &= ~STATE_REQUIRES_OOW_FBI;
      goto secondary_packet;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) {
        dpBC = FARRAY(fb,i) * oowb;
        dpdx = FARRAY(fa,i) * oowa;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc;
        dpdx = dpAB * dyBC - dpBC * dyAB;
      
        FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      }
      else {
        dpBC = oowb;
        dpdx = oowa;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - oowc;
        dpdx = dpAB * dyBC - dpBC * dyAB;
      
        FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      }
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_W_TMU0) {
    fp = dlp->addr;
    /* chip field change */
    if (i & 1)
      goto secondary_packet;
    else {
      float dpAB, dpBC,dpdx, dpdy;

      if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
        dpBC = FARRAY(fb,i) * oowb;
        dpdx = FARRAY(fa,i) * oowa;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc;
      }
      else {
        dpBC = oowb;
        dpdx = oowa;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - oowc;
      }

      dpdx = dpAB * dyBC - dpBC * dyAB;

GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;

GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
      dlp++;
      i = dlp->i;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_ST_TMU1) {
    for (k = 0; i < 2; k++) {
      fp = dlp->addr;
      /* chip field change */
      if (i & 1)
        goto secondary_packet;
      else {
        float dpAB, dpBC,dpdx, dpdy;
        
        dpBC = FARRAY(fb,i) * oowb * gc->state.tmu_config[1].st_scale[k];
        dpdx = FARRAY(fa,i) * oowa * gc->state.tmu_config[1].st_scale[k];
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc * gc->state.tmu_config[1].st_scale[k];
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
        FSET_GW_ENTRY( fifoPtr, 1, dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;
        
        GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
        dlp++;
        i = dlp->i;
        FSET_GW_ENTRY( fifoPtr, 2, dpdy );
        fifoPtr += 3;
      }
    }
  }
  if (paramIndex & STATE_REQUIRES_W_TMU1) {
    fp = dlp->addr;
    /* chip field change */
    if (i & 1)
      goto secondary_packet;
    else {
      float dpAB, dpBC,dpdx, dpdy;

      if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
        dpBC = FARRAY(fb,i) * oowb;
        dpdx = FARRAY(fa,i) * oowa;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );

        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc;
      }
      else {
        dpBC = oowb;
        dpdx = oowa;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );

        dpAB = dpdx - dpBC;
        dpBC = dpBC - oowc;
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;

GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;

GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
      dlp++;
      i = dlp->i;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }

  /* write triangle command */
triangle_command:
  FSET_GW_ENTRY( fifoPtr, 0, _GlideRoot.pool.temp1.f );
  fifoPtr+=1;
  
  if (((FxU32)fifoPtr) & 0x7) {
    FSET_GW_ENTRY( fifoPtr, 0, 0.0f );
    fifoPtr += 1;
  }

  GR_ASSERT(fifoPtr == gc->fifoData.hwDep.vg96FIFOData.fifoPtr);

  _GlideRoot.stats.trisDrawn++;

  GR_CHECK_SIZE_SLOPPY();
  return 1;
secondary_packet:
  /* Round out last header */
  if (((FxU32) fifoPtr) & 0x7) {
    FSET_GW_ENTRY( fifoPtr, 0, 0.0f );
    fifoPtr  += 1;
  }
  /* Start new packet
     note, there can only ever be two different packets 
     using gwHeaderNum++ would be more general, but this
     reflects the actual implementation */
  SET_GW_CMD(    fifoPtr, 0, (FxU32)fp );
  SET_GW_HEADER( fifoPtr, 1, gc->hwDep.sst96Dep.gwHeaders[1] );
  fifoPtr+=2;
  dlp++;
  i = dlp->i;

  if (paramIndex & STATE_REQUIRES_IT_DRGB) {
    for (k = 0; k < 3; k++) {
      float dpAB, dpBC,dpdx, dpdy;
      FxU32 bddr = dlp->bddr;
      fp = dlp->addr;
      if (bddr) {
        /* packed data (color) */
        dpBC = FbARRAY(fb,bddr);
        dpdx = FbARRAY(fa,bddr);
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FbARRAY(fc,bddr);
      }
      else {
        /* non packed data */
        dpBC = FARRAY(fb,i) * _GlideRoot.pool.f255;
        dpdx = FARRAY(fa,i) * _GlideRoot.pool.f255;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * _GlideRoot.pool.f255;
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      dlp++;
      i = dlp->i;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_OOZ) {
    float dpAB, dpBC,dpdx, dpdy;
    fp = dlp->addr;
    /* non packed data */
    dpBC = FARRAY(fb,i) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
    dpdx = FARRAY(fa,i) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
    FSET_GW_ENTRY( fifoPtr, 0, dpdx );
    dpAB = dpdx - dpBC;
    dpBC = dpBC - FARRAY(fc,i) * oowc * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
    dpdx = dpAB * dyBC - dpBC * dyAB;
    FSET_GW_ENTRY( fifoPtr, 1, dpdx );
    dpdy = dpBC * dxAB - dpAB * dxBC;
    dlp++;
    i = dlp->i;
    FSET_GW_ENTRY( fifoPtr, 2, dpdy );
    fifoPtr += 3;
  }
  if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) {
    float dpAB, dpBC,dpdx, dpdy;
    FxU32 bddr = dlp->bddr;
    fp = dlp->addr;
    if (bddr) {
      /* packed data (color) */
      dpBC = FbARRAY(fb,bddr);
      dpdx = FbARRAY(fa,bddr);
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FbARRAY(fc,bddr);
    }
    else {
      /* non packed data */
      dpBC = FARRAY(fb,i) * _GlideRoot.pool.f255;
      dpdx = FARRAY(fa,i) * _GlideRoot.pool.f255;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * _GlideRoot.pool.f255;
    }
    dpdx = dpAB * dyBC - dpBC * dyAB;
    FSET_GW_ENTRY( fifoPtr, 1, dpdx );
    dpdy = dpBC * dxAB - dpAB * dxBC;
    dlp++;
    i = dlp->i;
    FSET_GW_ENTRY( fifoPtr, 2, dpdy );
    fifoPtr += 3;
  }
  if (paramIndex & STATE_REQUIRES_ST_TMU0) {
    for (k = 0; k < 2; k++) {
      float dpAB, dpBC,dpdx, dpdy;
      fp = dlp->addr;
      dpBC = FARRAY(fb,i) * oowb * gc->state.tmu_config[0].st_scale[k];
      dpdx = FARRAY(fa,i) * oowa * gc->state.tmu_config[0].st_scale[k];
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * oowc * gc->state.tmu_config[0].st_scale[k];
      dpdx = dpAB * dyBC - dpBC * dyAB;
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      dlp++;
      i = dlp->i;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_OOW_FBI) {
    float dpAB, dpBC,dpdx, dpdy;
    if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) {
      fp = dlp->addr;
      dpBC = FARRAY(fb,i) * oowb;
      dpdx = FARRAY(fa,i) * oowa;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * oowc;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
    }
    else {
      fp = dlp->addr;
      dpBC = oowb;
      dpdx = oowa;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - oowc;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
    }
    dpdy = dpBC * dxAB - dpAB * dxBC;
    dlp++;
    i = dlp->i;
    FSET_GW_ENTRY( fifoPtr, 2, dpdy );
    fifoPtr += 3;
  }
  if (paramIndex & STATE_REQUIRES_W_TMU0) {
    float dpAB, dpBC,dpdx, dpdy;
    fp = dlp->addr;
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
      dpBC = FARRAY(fb,i) * oowb;
      dpdx = FARRAY(fa,i) * oowa;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * oowc;
    }
    else {
      dpBC = oowb;
      dpdx = oowa;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - oowc;
    }
    dpdx = dpAB * dyBC - dpBC * dyAB;
    FSET_GW_ENTRY( fifoPtr, 1, dpdx );
    dpdy = dpBC * dxAB - dpAB * dxBC;
    dlp++;
    i = dlp->i;
    FSET_GW_ENTRY( fifoPtr, 2, dpdy );
    fifoPtr += 3;
  }
  if (paramIndex & STATE_REQUIRES_ST_TMU1) {
    for (k = 0; k < 2; k++) {
      float dpAB, dpBC,dpdx, dpdy;
      fp = dlp->addr;
      dpBC = FARRAY(fb,i) * oowb * gc->state.tmu_config[1].st_scale[k];
      dpdx = FARRAY(fa,i) * oowa * gc->state.tmu_config[1].st_scale[k];
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * oowc * gc->state.tmu_config[1].st_scale[k];
      dpdx = dpAB * dyBC - dpBC * dyAB;
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      dlp++;
      i = dlp->i;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_W_TMU1) {
    float dpAB, dpBC,dpdx, dpdy;
    fp = dlp->addr;
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) {
      dpBC = FARRAY(fb,i) * oowb;
      dpdx = FARRAY(fa,i) * oowa;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * oowc;
    }
    else {
      dpBC = oowb;
      dpdx = oowa;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - oowc;
    }
    dpdx = dpAB * dyBC - dpBC * dyAB;
    FSET_GW_ENTRY( fifoPtr, 1, dpdx );
    dpdy = dpBC * dxAB - dpAB * dxBC;
    dlp++;
    i = dlp->i;
    FSET_GW_ENTRY( fifoPtr, 2, dpdy );
    fifoPtr += 3;
  }

  if (((FxU32)fifoPtr) & 0x7) {
    FSET_GW_ENTRY( fifoPtr, 0, 0.0f );
    fifoPtr += 1;
  }
  SET_GW_CMD(    fifoPtr, 0, gc->hwDep.sst96Dep.gwCommand );
  SET_GW_HEADER( fifoPtr, 1, GW_TRICMD_MASK );
  fifoPtr += 2;
  goto triangle_command;
} /* grVpDrawTriangle */
#elif ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
FxI32 FX_CSTYLE
_grVpDrawTriangle( const void *va, const void *vb, const void *vc )
{
  GR_DCL_GC;
  GR_DCL_HW;
  FxI32 xindex = (gc->state.vData.vertexInfo.offset >> 2);
  FxI32 yindex = xindex + 1;
  const float *fa = (const float *)va + xindex;
  const float *fb = (const float *)vb + xindex;
  const float *fc = (const float *)vc + xindex;
  float ooa, dxAB, dxBC, dyAB, dyBC;
  int i,culltest;
  union { float f; int i; } ay;
  union { float f; int i; } by;
  union { float f; int i; } cy;
  float *fp;
  struct dataList_s *dlp;
  float oowa, oowb, oowc;
  int k;
  float snap_xa, snap_ya, snap_xb, snap_yb, snap_xc, snap_yc;
  float tmp_snap_ya, tmp_snap_yb, tmp_snap_yc;

  culltest = gc->state.cull_mode; /* 1 if negative, 0 if positive */
  _GlideRoot.stats.trisProcessed++;

  /*
  ** oow
  */
  oowa = 1.0f / FARRAY(fa, gc->state.vData.wInfo.offset);
  oowb = 1.0f / FARRAY(fb, gc->state.vData.wInfo.offset);
  oowc = 1.0f / FARRAY(fc, gc->state.vData.wInfo.offset);
  
  /*
   **  Sort the vertices.
   **  Whenever the radial order is reversed (from counter-clockwise to
   **  clockwise), we need to change the area of the triangle.  Note
   **  that we know the first two elements are X & Y by looking at the
   **  grVertex structure.  
   */
  snap_ya = tmp_snap_ya = (volatile float) (*((const float *)va + yindex) * oowa * gc->state.Viewport.hheight + gc->state.Viewport.oy + SNAP_BIAS);
  snap_yb = tmp_snap_yb = (volatile float) (*((const float *)vb + yindex) * oowb * gc->state.Viewport.hheight + gc->state.Viewport.oy + SNAP_BIAS);
  snap_yc = tmp_snap_yc = (volatile float) (*((const float *)vc + yindex) * oowc * gc->state.Viewport.hheight + gc->state.Viewport.oy + SNAP_BIAS);

  ay.f = tmp_snap_ya;
  by.f = tmp_snap_yb;
  cy.f = tmp_snap_yc;
  if (ay.i < 0) ay.i ^= 0x7FFFFFFF;
  if (by.i < 0) by.i ^= 0x7FFFFFFF;
  if (cy.i < 0) cy.i ^= 0x7FFFFFFF;
  if (ay.i < by.i) {
    if (by.i > cy.i) {              /* acb */
      if (ay.i < cy.i) {
        fa = (const float *)va + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)vb + xindex;
        snap_ya = tmp_snap_ya;
        snap_yb = tmp_snap_yc;
        snap_yc = tmp_snap_yb;
        culltest ^= 1;
      } else {                  /* cab */
        fa = (const float *)vc + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vb + xindex;
        snap_ya = tmp_snap_yc;
        snap_yb = tmp_snap_ya;
        snap_yc = tmp_snap_yb;
      }
      /* else it's already sorted */
    }
  } else {
    if (by.i < cy.i) {              /* bac */
      if (ay.i < cy.i) {
        fa = (const float *)vb + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vc + xindex;
        snap_ya = tmp_snap_yb;
        snap_yb = tmp_snap_ya;
        snap_yc = tmp_snap_yc;
        culltest ^= 1;
      } else {                  /* bca */
        fa = (const float *)vb + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)va + xindex;
        snap_ya = tmp_snap_yb;
        snap_yb = tmp_snap_yc;
        snap_yc = tmp_snap_ya;
      }
    } else {                    /* cba */
      fa = (const float *)vc + xindex;
      fb = (const float *)vb + xindex;
      fc = (const float *)va + xindex;
      snap_ya = tmp_snap_yc;
      snap_yb = tmp_snap_yb;
      snap_yc = tmp_snap_ya;
      culltest ^= 1;
    }
  }

  snap_xa = (volatile float) (*((const float *)fa + xindex) * oowa * gc->state.Viewport.hwidth + gc->state.Viewport.ox + SNAP_BIAS);
  snap_xb = (volatile float) (*((const float *)fb + xindex) * oowb * gc->state.Viewport.hwidth + gc->state.Viewport.ox + SNAP_BIAS);
  snap_xc = (volatile float) (*((const float *)fc + xindex) * oowc * gc->state.Viewport.hwidth + gc->state.Viewport.ox + SNAP_BIAS);

  /* Compute Area */
  dxAB = snap_xa - snap_xb;
  dxBC = snap_xb - snap_xc;
  
  dyAB = snap_ya - snap_yb;
  dyBC = snap_yb - snap_yc;
  
  /* this is where we store the area */
  _GlideRoot.pool.temp1.f = dxAB * dyBC - dxBC * dyAB;
  
  /* Zero-area triangles are BAD!! */
  if ((_GlideRoot.pool.temp1.i & 0x7FFFFFFF) == 0)
    return 0;
  
  /* Backface culling, use sign bit as test */
  if (gc->state.cull_mode != GR_CULL_DISABLE) {
    if ((_GlideRoot.pool.temp1.i ^ (culltest<<31)) >= 0) {
      return -1;
    }
  }
  
  GR_FLUSH_STATE();

  GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize);
  
  ooa = _GlideRoot.pool.f1 / _GlideRoot.pool.temp1.f;
  /* GMT: note that we spread out our PCI writes */
  /* write out X & Y for vertex A */
  GR_SETF( hw->FvA.x, snap_xa);
  GR_SETF( hw->FvA.y, snap_ya);

  dlp = gc->dataList;
  i = dlp->i;
  
  /* write out X & Y for vertex B */
  GR_SETF( hw->FvB.x, snap_xb);
  GR_SETF( hw->FvB.y, snap_yb);
  
  /* write out X & Y for vertex C */
  GR_SETF( hw->FvC.x, snap_xc);
  GR_SETF( hw->FvC.y, snap_yc);

  /*
  ** Divide the deltas by the area for gradient calculation.
  */
  dxBC *= ooa;
  dyAB *= ooa;
  dxAB *= ooa;
  dyBC *= ooa;

  /* 
  ** The src vector contains offsets from fa, fb, and fc to for which
  **  gradients need to be calculated, and is null-terminated.
  */
  if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) {
    for (k = 0; k < 3; k++) {
      fp = dlp->addr;
      if (i & 1) {                   /* packer bug check */
        if (i & 2) P6FENCE;
        GR_SETF( fp[0], 0.0F );
        if (i & 2) P6FENCE;
        dlp++;
        i = dlp->i;
      }
      else {
        float dpAB, dpBC,dpdx, dpdy;
        FxU32 bddr = dlp->bddr;
        
        if (bddr) {
          dpBC = FbARRAY(fb,bddr);
          dpdx = FbARRAY(fa,bddr);
          GR_SETF( fp[0], dpdx );
        
          dpAB = dpdx - dpBC;
          dpBC = dpBC - FbARRAY(fc,bddr);
        }
        else {
          dpBC = FARRAY(fb,i) * _GlideRoot.pool.f255;
          dpdx = FARRAY(fa,i) * _GlideRoot.pool.f255;
          GR_SETF( fp[0], dpdx );
        
          dpAB = dpdx - dpBC;
          dpBC = dpBC - FARRAY(fc,i) * _GlideRoot.pool.f255;
        }
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;

        dlp++;
        i = dlp->i;
        GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
      }
    }
  }
  if (gc->state.paramIndex & STATE_REQUIRES_OOZ) {
    fp = dlp->addr;
    if (i & 1) {                   /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0F );
      if (i & 2) P6FENCE;
      dlp++;
      i = dlp->i;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      dpBC = FARRAY(fb,i) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      dpdx = FARRAY(fa,i) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      GR_SETF( fp[0], dpdx );
      
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * oowc * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
    }
  }
  if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) {
    fp = dlp->addr;
    if (i & 1) {                   /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0F );
      if (i & 2) P6FENCE;
      dlp++;
        i = dlp->i;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      FxU32 bddr = dlp->bddr;
      if (bddr) {
        dpBC = FbARRAY(fb,bddr);
        dpdx = FbARRAY(fa,bddr);
        GR_SETF( fp[0], dpdx );
      
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FbARRAY(fc,bddr);
      }
      else {
        dpBC = FARRAY(fb,i) * _GlideRoot.pool.f255;
        dpdx = FARRAY(fa,i) * _GlideRoot.pool.f255;
        GR_SETF( fp[0], dpdx );
      
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * _GlideRoot.pool.f255;
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
    }
  }
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) {
    for (k = 0; k < 2; k++) {
      fp = dlp->addr;
      if (i & 1) {                   /* packer bug check */
        if (i & 2) P6FENCE;
        GR_SETF( fp[0], 0.0F );
        if (i & 2) P6FENCE;
        dlp++;
        i = dlp->i;
      }
      else {
        float dpAB, dpBC,dpdx, dpdy;
        
        dpBC = FARRAY(fb,i) * oowb * gc->state.tmu_config[0].st_scale[k];
        dpdx = FARRAY(fa,i) * oowa * gc->state.tmu_config[0].st_scale[k];
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc * gc->state.tmu_config[0].st_scale[k];
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;
        
        dlp++;
        i = dlp->i;
        GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
      }
    }
  }
  if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) {
    fp = dlp->addr;
    if (i & 1) {                   /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0F );
      if (i & 2) P6FENCE;
      dlp++;
        i = dlp->i;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) {
        dpBC = FARRAY(fb,i) * oowb;
        dpdx = FARRAY(fa,i) * oowa;
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc;
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      }
      else {
        dpBC = oowb;
        dpdx = oowa;
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - oowc;
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      }
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
    }
  }
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) {
    fp = dlp->addr;
    if (i & 1) {                   /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0F );
      if (i & 2) P6FENCE;
      dlp++;
      i = dlp->i;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;

      if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
        dpBC = FARRAY(fb,i) * oowb;
        dpdx = FARRAY(fa,i) * oowa;
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc;
      }
      else {
        dpBC = oowb;
        dpdx = oowa;
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - oowc;
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;

GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
      GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;

GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
      dlp++;
      i = dlp->i;
      GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
    }
  }
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) {
    for (k = 0; k < 2; k++) {
      fp = dlp->addr;
      if (i & 1) {                   /* packer bug check */
        if (i & 2) P6FENCE;
        GR_SETF( fp[0], 0.0F );
        if (i & 2) P6FENCE;
        dlp++;
        i = dlp->i;
      }
      else {
        float dpAB, dpBC,dpdx, dpdy;
        
        dpBC = FARRAY(fb,i) * oowb * gc->state.tmu_config[1].st_scale[k];
        dpdx = FARRAY(fa,i) * oowa * gc->state.tmu_config[1].st_scale[k];
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc * gc->state.tmu_config[1].st_scale[k];
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
        GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;
        
        GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
        dlp++;
        i = dlp->i;
        GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
      }
    }
  }
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) {
    fp = dlp->addr;
    if (i & 1) {                   /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0F );
      if (i & 2) P6FENCE;
      dlp++;
      i = dlp->i;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;

      if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) {
        dpBC = FARRAY(fb,i) * oowb;
        dpdx = FARRAY(fa,i) * oowa;
        GR_SETF( fp[0], dpdx );

        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,i) * oowc;
      }
      else {
        dpBC = oowb;
        dpdx = oowa;
        GR_SETF( fp[0], dpdx );

        dpAB = dpdx - dpBC;
        dpBC = dpBC - oowc;
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;

GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
      GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;

GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
      dlp++;
      i = dlp->i;
      GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
    }
  }

  /* Draw the triangle by writing the area to the triangleCMD register */
  P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, _GlideRoot.pool.temp1.f ) );
  _GlideRoot.stats.trisDrawn++;

  GR_CHECK_SIZE_SLOPPY();
  return 1;
} /* grVpDrawTriangle */
#endif

/*---------------------------------------------------------------------------
** _grColorCombineDelta0Mode
**
** GMT: when we are in delta0 mode, color comes from the RGB iterators
**      but the slopes are 0.0.  So when we enter delta0 mode we set
**      the iterators up and then we leave them alone during primitive
**      rendering
*/

GR_DDFUNC(_grColorCombineDelta0Mode, void, ( FxBool delta0mode ))
{
  GR_BEGIN("_grColorCombineDelta0Mode",85,(delta0mode ? 9*4 : 0));
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",delta0mode));

  if ( delta0mode ) {
    GR_SETF( hw->Fr, gc->state.r );
    GR_SETF( hw->Fg, gc->state.g );
    GR_SETF( hw->Fb, gc->state.b );
    GR_SET( hw->drdx, 0);
    GR_SET( hw->drdy, 0);
    GR_SET( hw->dgdx, 0);
    GR_SET( hw->dgdy, 0);
    GR_SET( hw->dbdx, 0);
    GR_SET( hw->dbdy, 0);
  }

  gc->state.cc_delta0mode = delta0mode;
  GR_END();

} /* _grColorCombineDeltaMode */

/*-------------------------------------------------------------------
  Function: _grDrawPoints
  Date: 13-Oct-97
  Implementor(s): atai
  Description:
  Draw array points
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grDrawPoints(FxI32 mode, FxI32 count, void *pointers)
{
#define FN_NAME "_grDrawPoints"

  int i, x,y;
  float fx,fy;
  float bias = (3<<22) - 0.5f;
  struct dataList_s *dlp;
  FxI32 stride = mode;
  FxI32 index;

  GR_BEGIN_NOFIFOCHECK("grDrawPoint",90);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,0x%x)\n",mode,count,pointers));
  
  /* we snap to an integer by adding a large enough number that it shoves
     all fraction bits off the right side of the mantissa
     NOTE: IEEE rounds to nearest integer by default so we sub 0.5 in
     the bias to force truncation (FLOOR)
     */

  GR_FLUSH_STATE();

  if (stride == 0)
    stride = gc->state.vData.vStride;
  
  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    for (index = 0; index < count; index++) {
      float *vPtr;
      
      GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize);
      if (mode)
        vPtr = *(float **)pointers;
      else
        vPtr = pointers;
    
      pointers = (float *)pointers + stride;
      
#if 0 //def GLIDE_USE_ALT_REGMAP
      hw = SST_WRAP(hw,128);                /* use alternate register mapping */
#endif
      fx = *vPtr + bias;
      fy = *(vPtr+1) + bias;
      _GlideRoot.pool.temp1.f = (float)fx;
      _GlideRoot.pool.temp2.f = (float)fy;
      x = _GlideRoot.pool.temp1.i;
      y = _GlideRoot.pool.temp2.i;
      x <<= SST_XY_FRACBITS;                /* convert to fixed point */
      x += SST_XY_HALF;                     /* and center within pixel */
    
      /* draw a little triangle, with the lower left corner at pixel center */
      GR_SET( hw->vA.x, x );
      y <<= SST_XY_FRACBITS;
      y += SST_XY_HALF;
      GR_SET( hw->vA.y, y );
      x += SST_XY_HALF;
      i = _GlideRoot.stats.pointsDrawn;
      
      GR_SET( hw->vB.x, x );
      _GlideRoot.stats.pointsDrawn = ++i;
      dlp = gc->dataList;
      GR_SET( hw->vB.y, y );
      y += SST_XY_HALF;
      GR_SET( hw->vC.x, x );
      i = dlp->i;
      GR_SET( hw->vC.y, y );
    
      /* we don't care what the slopes are because the pixel center that is drawn */
      /* is exactly at vertex A - isn't that wonderful */
      while (i) {
        if (i & 1) {        /* packer bug check */
          if (i & 2) P6FENCE;
          GR_SETF( *dlp->addr, 0.0F );
          if (i & 2) P6FENCE;
        }
        else {
          FxU32 bddr = dlp->bddr;
          if (bddr) {
            /* packed color */
            GR_SETF( *dlp->addr, FbARRAY(vPtr,bddr) );
          }
          else {
            /* non packed color */
            GR_SETF( *dlp->addr, FARRAY(vPtr,i) );
          }
        }
        dlp++;
        i = dlp->i;
      }
      P6FENCE_CMD( GR_SET( hw->triangleCMD, 1) );
      GR_CHECK_SIZE_SLOPPY();
    }
  }
  else {
    float oow;
    FxU32 k;

    for (index = 0; index < count; index++) {
      float *vPtr;
      
      GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize);
      if (mode)
        vPtr = *(float **)pointers;
      else
        vPtr = pointers;

      oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset);        
      pointers = (float *)pointers + stride;
      
#if 0 //def GLIDE_USE_ALT_REGMAP
      hw = SST_WRAP(hw,128);                /* use alternate register mapping */
#endif
      fx = FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth
        + gc->state.Viewport.ox + bias;
      fy = FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight
        + gc->state.Viewport.oy + bias;

      _GlideRoot.pool.temp1.f = (float)fx;
      _GlideRoot.pool.temp2.f = (float)fy;
      x = _GlideRoot.pool.temp1.i;
      y = _GlideRoot.pool.temp2.i;
      x <<= SST_XY_FRACBITS;                /* convert to fixed point */
      x += SST_XY_HALF;                     /* and center within pixel */
    
      /* draw a little triangle, with the lower left corner at pixel center */
      GR_SET( hw->vA.x, x );
      y <<= SST_XY_FRACBITS;
      y += SST_XY_HALF;
      GR_SET( hw->vA.y, y );
      x += SST_XY_HALF;
      i = _GlideRoot.stats.pointsDrawn;
      
      GR_SET( hw->vB.x, x );
      _GlideRoot.stats.pointsDrawn = ++i;
      dlp = gc->dataList;
      GR_SET( hw->vB.y, y );
      y += SST_XY_HALF;
      GR_SET( hw->vC.x, x );
      i = dlp->i;
      GR_SET( hw->vC.y, y );
    
      /* we don't care what the slopes are because the pixel center that is drawn */
      /* is exactly at vertex A - isn't that wonderful */
      if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) {
        for (k = 0; k < 3; k++) {
          if (i & 1) {        /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( *dlp->addr, 0.0F );
            if (i & 2) P6FENCE;
          }
          else {
            FxU32 bddr = dlp->bddr;
            if (bddr) {
              GR_SETF( *dlp->addr, FbARRAY(vPtr,bddr));
            } else {
              GR_SETF( *dlp->addr, FARRAY(vPtr,i) * _GlideRoot.pool.f255);
            }
          }
          dlp++;
          i = dlp->i;
        }
      }
      if (gc->state.paramIndex & STATE_REQUIRES_OOZ) {
        if (i & 1) {        /* packer bug check */
          if (i & 2) P6FENCE;
          GR_SETF( *dlp->addr, 0.0F );
          if (i & 2) P6FENCE;
        }
        else {
          GR_SETF( *dlp->addr, FARRAY(vPtr,i) * oow * gc->state.Viewport.hdepth + gc->state.Viewport.oz );
        }
        dlp++;
        i = dlp->i;
      }
      if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) {
        if (i & 1) {        /* packer bug check */
          if (i & 2) P6FENCE;
          GR_SETF( *dlp->addr, 0.0F );
            if (i & 2) P6FENCE;
        }
        else {
          FxU32 bddr = dlp->bddr;
          if (bddr) {
            GR_SETF( *dlp->addr, FbARRAY(vPtr,bddr));
          } else {
            GR_SETF( *dlp->addr, FARRAY(vPtr,i) * _GlideRoot.pool.f255);
          }
        }
        dlp++;
        i = dlp->i;
      }
      if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) {
        for (k = 0; k < 2; k++) {
          if (i & 1) {        /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( *dlp->addr, 0.0F );
            if (i & 2) P6FENCE;
          }
          else {
            GR_SETF( *dlp->addr, FARRAY(vPtr,i) * oow * gc->state.tmu_config[0].st_scale[k]);
          }
          dlp++;
          i = dlp->i;
        }
      }
      if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) {
        if (i & 1) {        /* packer bug check */
          if (i & 2) P6FENCE;
          GR_SETF( *dlp->addr, 0.0F );
          if (i & 2) P6FENCE;
        }
        else { 
          if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) {
            GR_SETF( *dlp->addr, FARRAY(vPtr,i) * oow );
          }
          else {
            GR_SETF( *dlp->addr, oow );
          }
        }
        dlp++;
        i = dlp->i;
      }
      if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) {
        if (i & 1) {        /* packer bug check */
          if (i & 2) P6FENCE;
          GR_SETF( *dlp->addr, 0.0F );
          if (i & 2) P6FENCE;
        }
        else {
          if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
            GR_SETF( *dlp->addr, FARRAY(vPtr,i) * oow );
          }
          else {
            GR_SETF( *dlp->addr, oow );
          }
        }
        dlp++;
        i = dlp->i;
      }
      if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) {
        for (k = 0; k < 2; k++) {
          if (i & 1) {        /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( *dlp->addr, 0.0F );
            if (i & 2) P6FENCE;
          }
          else {
            GR_SETF( *dlp->addr, FARRAY(vPtr,i) * oow * gc->state.tmu_config[1].st_scale[k] );
          }
          dlp++;
          i = dlp->i;
        }
      }
      if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) {
        if (i & 1) {        /* packer bug check */
          if (i & 2) P6FENCE;
          GR_SETF( *dlp->addr, 0.0F );
          if (i & 2) P6FENCE;
        }
        else {
          if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) {
            GR_SETF( *dlp->addr, FARRAY(vPtr,i) * oow );
          }
          else {
            GR_SETF( *dlp->addr, oow );
          }
        }
        dlp++;
        i = dlp->i;
      }

      P6FENCE_CMD( GR_SET( hw->triangleCMD, 1) );
      GR_CHECK_SIZE_SLOPPY();
    }
  }

#undef FN_NAME
} /* _grDrawPoints */

/*-------------------------------------------------------------------
  Function: _grDrawLineStrip
  Date: 13-Oct-97
  Implementor(s): atai
  Description:
  Draw strip line
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grDrawLineStrip(FxI32 mode, FxI32 ltype, FxI32 count, void *pointers)
{
#define FN_NAME "_grDrawLineStrip"

  int i;
  float    m, dp;
  #define  DX _GlideRoot.pool.temp1
  #define ADY _GlideRoot.pool.temp2

  float *fp;
  struct dataList_s *dlp;
  FxI32 index;
  FxI32 stride = mode;

  GR_BEGIN_NOFIFOCHECK("grDrawLine",91);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d,0x%x)\n",mode,ltype,count,pointers));

  GR_FLUSH_STATE();

  if (stride == 0)
    stride = gc->state.vData.vStride;

  if (ltype == GR_LINES)
    count >>= 1;    /* line list */
  else
    count -= 1;     /* strip line */

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    for (index = 0; index < count; index++) {
      float *a, *b;
      float snap_xa, snap_ya, snap_xb, snap_yb;

      GR_SET_EXPECTED_SIZE(12+_GlideRoot.curTriSize);
      
      if (mode) {
        a = *(float **)pointers;
        b = *((float **)pointers+1);
      }
      else {
        a = pointers;
        b = (float *)pointers+stride;
      }

      snap_xa = (volatile float) (*a + SNAP_BIAS);
      snap_xb = (volatile float) (*b + SNAP_BIAS);
      snap_ya = (volatile float) (*(a+1) + SNAP_BIAS);
      snap_yb = (volatile float) (*(b+1) + SNAP_BIAS);

      pointers = (float *)pointers + stride;
      if (ltype == GR_LINES)
        pointers = (float *)pointers + stride;
      
#if 0 //def GLIDE_USE_ALT_REGMAP
      hw = SST_WRAP(hw,128);                /* use alternate register mapping */
#endif
      /*
      ** compute absolute deltas and draw from low Y to high Y
      */
      ADY.f = snap_yb - snap_ya;
      if ( ADY.i < 0 ) {
        float *tv, ts;
        tv = a; a = b; b = tv;
        ADY.i ^= 0x80000000;        /* ady = -ady; */
	ts = snap_xa; snap_xa = snap_xb; snap_xb = ts;
	ts = snap_ya; snap_ya = snap_yb; snap_yb = ts;
      }
      
      DX.f = snap_xb - snap_xa;
      DX.i &= 0x7fffffff;           /* abs(adx) */        
      
      /*
      ** X major line
      */
      if (DX.i >= ADY.i) {                       /* if (adx > ady) */
        if (DX.i == 0) goto all_done;         /* check for zero-length lines */
        /* start up divide and overlap with as much integer stuff as possible*/
        m = _GlideRoot.pool.f1 / DX.f;
        dlp = gc->dataList;
        GR_SETF(hw->FvA.x, snap_xa);
        dp = snap_xb;
        GR_SETF(hw->FvB.x,dp);
        GR_SETF(hw->FvC.x,dp);
        _GlideRoot.stats.linesDrawn++;
        
        GR_SETF(hw->FvA.y, snap_ya - _GlideRoot.pool.fHalf);
        
        dp = snap_yb;
        GR_SETF(hw->FvB.y,dp - _GlideRoot.pool.fHalf);
        
        i = dlp->i;
        GR_SETF(hw->FvC.y,dp + _GlideRoot.pool.fHalf);
      
        while (i) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            FxU32 bddr = dlp->bddr;
            if (bddr) {
              /* packed color */
              dp = FbARRAY(a,bddr);
              GR_SETF( fp[0], dp );
              dp = FbARRAY(b,bddr) - dp;
            }
            else {
              /* non-packed color */
              dp = FARRAY(a,i);
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) - dp;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
          }
        }
        P6FENCE_CMD( GR_SETF(hw->FtriangleCMD, DX.f) );
        
        GR_SETF(hw->FvB.x, snap_xa);
        GR_SETF(hw->FvB.y, snap_ya + _GlideRoot.pool.fHalf);
        P6FENCE_CMD( GR_SETF(hw->FtriangleCMD, -DX.f));
      }
      
      /*
      ** Y major line
      */
      else {
        m = _GlideRoot.pool.f1 / ADY.f;
        dlp = gc->dataList;
        GR_SETF(hw->FvA.y, snap_ya);
        dp = snap_yb;
        GR_SETF(hw->FvB.y,dp);
        _GlideRoot.stats.linesDrawn++;
        GR_SETF(hw->FvC.y,dp);
        
        GR_SETF(hw->FvA.x, snap_xa - _GlideRoot.pool.fHalf);
        
        dp = snap_xb;
        GR_SETF(hw->FvB.x,dp - _GlideRoot.pool.fHalf);
        
        i = dlp->i;
        GR_SETF(hw->FvC.x,dp + _GlideRoot.pool.fHalf);
        
        while (i) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f );
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            FxU32 bddr = dlp->bddr;
            if (bddr) {
              /* packed color */
              dp = FbARRAY(a,bddr);
              GR_SETF( fp[0], dp );
              dp = FbARRAY(b,bddr) - dp;
            }
            else {
              /* non-packed color */
              dp = FARRAY(a,i);
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) - dp;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
          }
        }
        P6FENCE_CMD( GR_SET( hw->triangleCMD, 0xFFFFFFFF) );
        
        GR_SETF(hw->FvB.x, snap_xa + _GlideRoot.pool.fHalf);
        GR_SETF(hw->FvB.y, snap_ya);
        P6FENCE_CMD( GR_SET( hw->triangleCMD, 1) );
      }
      
    all_done:  /* come here on degenerate lines */
      GR_CHECK_SIZE_SLOPPY();
      
      _GlideRoot.stats.linesDrawn++;
    }
    
  }
  else {

    float oowa, oowb = 0.0f, owa, owb, tmp1, tmp2, fax, fay, fbx, fby;
    float *a, *b;
    int k;

    if (ltype == GR_LINE_STRIP) {
      a = (float *)pointers;
      if (mode) {
        a = *(float **)a;
      }
      oowb = 1.0f / FARRAY(a, gc->state.vData.wInfo.offset);        
    }

    for (index = 0; index < count; index++) {
      GR_SET_EXPECTED_SIZE(12+_GlideRoot.curTriSize);
      
      a = (float *)pointers;
      b = (float *)pointers + stride;
      if (mode) {
        a = *(float **)a;
        b = *(float **)b;
      }
      if (ltype == GR_LINES) {
        pointers = (float *)pointers + stride;
        owa = oowa = 1.0f / FARRAY(a, gc->state.vData.wInfo.offset);        
      }
      else
        owa = oowa = oowb;
      owb = oowb = 1.0f / FARRAY(b, gc->state.vData.wInfo.offset);        

      fay = tmp1 = FARRAY(a, gc->state.vData.vertexInfo.offset+4)
        *oowa*gc->state.Viewport.hheight+gc->state.Viewport.oy + SNAP_BIAS;
      fby = tmp2 = FARRAY(b, gc->state.vData.vertexInfo.offset+4)
        *oowb*gc->state.Viewport.hheight+gc->state.Viewport.oy + SNAP_BIAS;        
      fax = FARRAY(a, gc->state.vData.vertexInfo.offset)
        *owa*gc->state.Viewport.hwidth+gc->state.Viewport.ox + SNAP_BIAS;
      fbx = FARRAY(b, gc->state.vData.vertexInfo.offset)
        *owb*gc->state.Viewport.hwidth+gc->state.Viewport.ox + SNAP_BIAS;

      pointers = (float *)pointers + stride;
#if 0 //def GLIDE_USE_ALT_REGMAP
      hw = SST_WRAP(hw,128);                /* use alternate register mapping */
#endif
      /*
      ** compute absolute deltas and draw from low Y to high Y
      */
      ADY.f = tmp2 - tmp1;
      if ( ADY.i < 0 ) {
        float *tv, ts;
        owa = oowb; owb = oowa;
        tv = a; a = b; b = tv;
        ADY.i ^= 0x80000000;        /* ady = -ady; */
	ts = fax; fax = fbx; fbx = ts;
	ts = fay; fay = fby; fby = ts;
      }
      
      DX.f = fbx - fax;
      DX.i &= 0x7fffffff;           /* abs(adx) */        
      
      /*
      ** X major line
      */
      if (DX.i >= ADY.i ) {                       /* if (adx > ady) */
        if (DX.i == 0) goto all_done_vp;         /* check for zero-length lines */
        /* start up divide and overlap with as much integer stuff as possible*/
        m = _GlideRoot.pool.f1 / DX.f;
        dlp = gc->dataList;
        GR_SETF(hw->FvA.x, fax);
        GR_SETF(hw->FvB.x, fbx);
        GR_SETF(hw->FvC.x, fbx);
        _GlideRoot.stats.linesDrawn++;
        
        GR_SETF(hw->FvA.y, fay - _GlideRoot.pool.fHalf);
        GR_SETF(hw->FvB.y, fby - _GlideRoot.pool.fHalf);
        i = dlp->i;
        GR_SETF(hw->FvC.y, fby + _GlideRoot.pool.fHalf);

        if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) {
          for (k = 0; k < 3; k++) {
            fp = dlp->addr;
            if (i & 1) {     /* packer bug check */
              if (i & 2) P6FENCE;
              GR_SETF( fp[0], 0.0f);
              if (i & 2) P6FENCE;
              dlp++;
              i = dlp->i;
            }
            else {
              FxU32 bddr = dlp->bddr;
              if (bddr) {
                dp = FbARRAY(a,bddr);
                GR_SETF( fp[0], dp );
                dp = FbARRAY(b,bddr) - dp;
              }
              else {
                dp = FARRAY(a,i) * _GlideRoot.pool.f255;
                GR_SETF( fp[0], dp );
                dp = FARRAY(b,i) * _GlideRoot.pool.f255 - dp;
              }
              GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
              dlp++;
              i = dlp->i;
              GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
            }
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_OOZ) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            dp = FARRAY(a,i) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
            GR_SETF( fp[0], dp );
            dp = FARRAY(b,i) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz - dp;
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            FxU32 bddr = dlp->bddr;
            if (bddr) {
              dp = FbARRAY(a,bddr);
              GR_SETF( fp[0], dp );
              dp = FbARRAY(b,bddr) - dp;
            }
            else {
              dp = FARRAY(a,i) * _GlideRoot.pool.f255;
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * _GlideRoot.pool.f255 - dp;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) {
          for (k = 0; k < 2; k++) {
            fp = dlp->addr;
            if (i & 1) {     /* packer bug check */
              if (i & 2) P6FENCE;
              GR_SETF( fp[0], 0.0f);
              if (i & 2) P6FENCE;
              dlp++;
              i = dlp->i;
            }
            else {
              dp = FARRAY(a,i) * oowa * gc->state.tmu_config[0].st_scale[k];
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * oowb * gc->state.tmu_config[0].st_scale[k] - dp;
              GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
              dlp++;
              i = dlp->i;
              GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
            }
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) {
              dp = FARRAY(a,i) * oowa;
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * oowb - dp;
              GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp = oowb - dp;
              GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
            }
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
              dp = FARRAY(a,i) * oowa;
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * oowb - dp;
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp = oowb - dp;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) {
          for (k = 0; k < 2; k++) {
            fp = dlp->addr;
            if (i & 1) {     /* packer bug check */
              if (i & 2) P6FENCE;
              GR_SETF( fp[0], 0.0f);
              if (i & 2) P6FENCE;
              dlp++;
              i = dlp->i;
            }
            else {
              dp = FARRAY(a,i) * oowa * gc->state.tmu_config[1].st_scale[k];
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * oowb * gc->state.tmu_config[1].st_scale[k] - dp;
              GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
              dlp++;
              i = dlp->i;
              GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
            }
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) {
              dp = FARRAY(a,i) * oowa;
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * oowb - dp;
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp = oowb - dp;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
          }
        }
        
        P6FENCE_CMD( GR_SETF(hw->FtriangleCMD, DX.f) );
        
        GR_SETF(hw->FvB.x, fax);
        GR_SETF(hw->FvB.y, fay + _GlideRoot.pool.fHalf);
        P6FENCE_CMD( GR_SETF(hw->FtriangleCMD, -DX.f) );
      }
      
      /*
      ** Y major line
      */
      else {
        m = _GlideRoot.pool.f1 / ADY.f;
        dlp = gc->dataList;
        GR_SETF(hw->FvA.y, fay);
        GR_SETF(hw->FvB.y, fby);
        _GlideRoot.stats.linesDrawn++;
        GR_SETF(hw->FvC.y, fby);
        
        GR_SETF(hw->FvA.x, fax - _GlideRoot.pool.fHalf);
        GR_SETF(hw->FvB.x, fbx - _GlideRoot.pool.fHalf);
        
        i = dlp->i;
        GR_SETF(hw->FvC.x, fbx + _GlideRoot.pool.fHalf);
        
        if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) {
          for (k = 0; k < 3; k++) {
            fp = dlp->addr;
            if (i & 1) {     /* packer bug check */
              if (i & 2) P6FENCE;
              GR_SETF( fp[0], 0.0f);
              if (i & 2) P6FENCE;
              dlp++;
              i = dlp->i;
            }
            else {
              FxU32 bddr = dlp->bddr;
              if (bddr) {
                dp = FbARRAY(a,bddr);
                GR_SETF( fp[0], dp );
                dp = FbARRAY(b,bddr) - dp;
              }
              else {
                dp = FARRAY(a,i) * _GlideRoot.pool.f255;
                GR_SETF( fp[0], dp );
                dp = FARRAY(b,i) * _GlideRoot.pool.f255 - dp;
              }
              GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
              dlp++;
              i = dlp->i;
              GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
            }
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_OOZ) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            dp = FARRAY(a,i) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
            GR_SETF( fp[0], dp );
            dp = FARRAY(b,i) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz - dp;
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            FxU32 bddr = dlp->bddr;
            if (bddr) {
              dp = FbARRAY(a,bddr);
              GR_SETF( fp[0], dp );
              dp = FbARRAY(b,bddr) - dp;
            }
            else {
              dp = FARRAY(a,i) * _GlideRoot.pool.f255;
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * _GlideRoot.pool.f255 - dp;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) {
          for (k = 0; k < 2; k++) {
            fp = dlp->addr;
            if (i & 1) {     /* packer bug check */
              if (i & 2) P6FENCE;
              GR_SETF( fp[0], 0.0f);
              if (i & 2) P6FENCE;
              dlp++;
              i = dlp->i;
            }
            else {
              dp = FARRAY(a,i) * oowa * gc->state.tmu_config[0].st_scale[k];
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * oowb * gc->state.tmu_config[0].st_scale[k] - dp;
              GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
              dlp++;
              i = dlp->i;
              GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
            }
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) {
              dp = FARRAY(a,i) * oowa;
              GR_SETF( fp[0], dp );
              dp = FARRAY(a,i) * oowb - dp;
              GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp = oowb - dp;
              GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
            }
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
              dp = FARRAY(a,i) * oowa;
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * oowb - dp;
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp = oowb - dp;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) {
          for (k = 0; k < 2; k++) {
            fp = dlp->addr;
            if (i & 1) {     /* packer bug check */
              if (i & 2) P6FENCE;
              GR_SETF( fp[0], 0.0f);
              if (i & 2) P6FENCE;
              dlp++;
              i = dlp->i;
            }
            else {
              dp = FARRAY(a,i) * oowa * gc->state.tmu_config[1].st_scale[k];
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * oowb * gc->state.tmu_config[1].st_scale[k] - dp;
              GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
              dlp++;
              i = dlp->i;
              GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
            }
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) {
              dp = FARRAY(a,i) * oowa;
              GR_SETF( fp[0], dp );
              dp = FARRAY(b,i) * oowb - dp;
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp = oowb - dp;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
          }
        }
        
        P6FENCE_CMD( GR_SET( hw->triangleCMD, 0xFFFFFFFF) );
        
        GR_SETF(hw->FvB.x, fax + _GlideRoot.pool.fHalf);
        GR_SETF(hw->FvB.y, fay);
        P6FENCE_CMD( GR_SET( hw->triangleCMD, 1) );
      }
      
    all_done_vp:  /* come here on degenerate lines */
      GR_CHECK_SIZE_SLOPPY();
      
      _GlideRoot.stats.linesDrawn++;
    }
    
  }

#undef FN_NAME
} /* _grDrawLineStrip */

/*-------------------------------------------------------------------
  Function: _grDrawTriangles
  Date: 13-Oct-97
  Implementor(s): atai
  Description:
  Draw triangles
  Arguments:
    mode - 0 if grDrawVertexArrayLinear
           1 if grDrawVertexArray
    count - number of triangles. 
    pointer - pointer to vertex data (mode = 0) or vertex array (mode = 1)
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grDrawTriangles(FxI32 mode, FxI32 count, void *pointers)
{
#define FN_NAME "_grDrawTriangles"

  FxI32 stride = mode;
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  if (stride == 0)
    stride = gc->state.vData.vStride;

  while (count > 0) {
    float *va, *vb, *vc;
    if (mode) {
      va = *(float **)pointers;
      vb = *((float **)pointers+1);
      vc = *((float **)pointers+2);
    }
    else {
      va = pointers;
      vb = (float *)pointers+stride;
      vc = (float *)pointers+(stride<<1);
    }

    pointers = (float *)pointers + (stride+(stride<<1));
      
    if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
      grDrawTriangle(va, vb, vc);
    else
      _grVpDrawTriangle(va, vb, vc);
        
    count -= 3;
  }

#undef FN_NAME
} /* _grDrawTriangles */
