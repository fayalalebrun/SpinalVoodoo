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
** Revision 1.1.2.6  2006/01/16 21:22:45  jwrdegoede
** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
**
** Revision 1.1.2.5  2005/06/16 18:58:32  jwrdegoede
** Fix 2 sst1 bugs accidently introduced by warning fixes
**
** Revision 1.1.2.4  2005/06/09 18:32:35  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.2.3  2005/05/10 11:27:23  jwrdegoede
** sst1 gcc4 compile fixes
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
 * 
 * 13    8/21/97 2:26p Atai
 * fix FIFO size
 * 
 * 12    7/25/97 2:55p Atai
 * Remove slope calculation in edge sense code, use cross product instead.
 * Move grAADrawTriEdgeSense() 1/area calculation after pci write command.
 * 
 * 11    7/24/97 5:59p Atai
 * optimize grAADrawPoint, grAADrawLine and grAADrawTriangle for sst1
 * 
 * 10    3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 9     12/23/96 1:37p Dow
 * chagnes for multiplatform glide
**
*/

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

/* access a floating point array with a byte index */
#define FARRAY(p,i) (*(float *)((i)+(int)(p)))
/* access a byte array with a byte index and convert to float */
#define FbARRAY(p,i) (float)(((unsigned char *)p)[i])

#define VX(a) *(a)
#define VY(b) *((float*)(((int)b)+4))

/*
** -----------------------------------------------------------------------
**
** Antialiasing rendering functions
**
** -----------------------------------------------------------------------
*/

static void grSetVertexParameter(const void *v1)
{
  GR_DCL_GC;
  struct dataList_s *dlp;
  int i;
  float *fp;

  dlp = gc->dataList;
  i = dlp->i;

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
        /* packed data (color) */
        GR_SETF( fp[0], FbARRAY(v1,bddr));
      }
      else {
        /* non packed data */
        GR_SETF( fp[0], FARRAY(v1,i));
      }
      dlp++;
      i = dlp->i;
    }
  }
}

static void grVpSetVertexParameter(float oow, const void *v1)
{
  GR_DCL_GC;
  struct dataList_s *dlp;
  int i;
  float *fp, dp;
  int k = 0;

  dlp = gc->dataList;
  i = dlp->i;

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
          dp = FbARRAY(v1,bddr);
        }
        else {
          dp = FARRAY(v1,i) * _GlideRoot.pool.f255;
        }
        GR_SETF( fp[0], dp );
        dlp++;
        i = dlp->i;
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
      dp = FARRAY(v1,i) * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      GR_SETF( fp[0], dp );
      dlp++;
      i = dlp->i;
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
        dp = FbARRAY(v1,bddr);
      }
      else {
        dp = FARRAY(v1,i) * _GlideRoot.pool.f255;
      }
      GR_SETF( fp[0], dp );
      dlp++;
        i = dlp->i;
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
        dp = FARRAY(v1,i) * oow * gc->state.tmu_config[0].st_scale[k];
        GR_SETF( fp[0], dp );
        dlp++;
        i = dlp->i;
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
        GR_SETF( fp[0], FARRAY(v1,i) * oow );
      }
      else {
        GR_SETF( fp[0], oow );
      }
      dlp++;
      i = dlp->i;
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
        dp = FARRAY(v1,i) * oow;
      }
      else {
        dp = oow;
      }
      GR_SETF( fp[0], dp );
      dlp++;
      i = dlp->i;
    }
  }
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) {
    fp = dlp->addr;
    if (i & 1) {     /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0f);
      if (i & 2) P6FENCE;
      dlp++;
      i = dlp->i;
    }
    else {
      /* hmm this caused a k may be used unitialised warning, which is
         silenced for now by initialising k with 0 at decleration. Problably
         k is always 2 in this scenario because most likely when
         (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) is true then
         (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) is also true and
         the for (k = 0; k < 2; k++) loop in the
         (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) case leaves k at 2 */
      dp = FARRAY(v1,i) * oow * gc->state.tmu_config[1].st_scale[k];
      GR_SETF( fp[0], dp );
      dlp++;
      i = dlp->i;
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
        dp = FARRAY(v1,i) * oow;
      }
      else {
        dp = oow;
      }
      GR_SETF( fp[0], dp );
      dlp++;
      i = dlp->i;
    }
  }
}

typedef enum {
  aaEdgeSenseTop,
  aaEdgeSenseLeft,
  aaEdgeSenseBottom,
  aaEdgeSenseRight
} aaEdgeSense;

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
aaDrawArrayEdgeSense(const float *a, const float *b, const float *c)
{
#define FN_NAME "aaDrawArrayEdgeSense"
  GR_DCL_GC;
  GR_DCL_HW;

  float dx, dy, m, dp;
  aaEdgeSense sense;
  const float *tv;
  float ts;
  float a_alpha, b_alpha;
  float snap_xa, snap_ya, snap_xb, snap_yb, snap_xc, snap_yc;

  snap_ya = (volatile float) (*(a + 1) + SNAP_BIAS);
  snap_yb = (volatile float) (*(b + 1) + SNAP_BIAS);
  if (snap_ya >= snap_yb) {
    tv = a; a = b; b = tv;
    ts = snap_ya; snap_ya = snap_yb; snap_yb = ts;
  }

  snap_xa = (volatile float) (*a + SNAP_BIAS);
  snap_xb = (volatile float) (*b + SNAP_BIAS);

  snap_xc = (volatile float) (*c + SNAP_BIAS);
  snap_yc = (volatile float) (*(c + 1) + SNAP_BIAS);

  dx = snap_xa - snap_xb;
  dy = snap_ya - snap_yb;

  if (dx == 0.f) {
    sense = (snap_xc > snap_xa) ? aaEdgeSenseLeft : aaEdgeSenseRight;
  }
  else if (dy == 0.0f) {
    sense = (snap_yc > snap_ya) ? aaEdgeSenseBottom : aaEdgeSenseTop;
  }
  else {
    float dcax, dcay, dcbx, dcby, cp;
    dcax = snap_xa - snap_xc;
    dcay = snap_ya - snap_yc;
    dcbx = snap_xb - snap_xc;
    dcby = snap_yb - snap_yc;
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

  if (gc->state.vData.colorType == GR_FLOAT) {
    a_alpha = FARRAY(a,gc->state.vData.aInfo.offset);
    b_alpha = FARRAY(b,gc->state.vData.aInfo.offset);
  }
  else {
    a_alpha = FbARRAY(a,gc->state.vData.pargbInfo.offset + GR_PARGB_A);
    b_alpha = FbARRAY(b,gc->state.vData.pargbInfo.offset + GR_PARGB_A);
  }
  switch (sense) {
  case aaEdgeSenseTop:
    /* pci comamnd 14 + 2 * n */
    GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSizeNoGradient << 1);
    GR_SETF( hw->FvA.x, snap_xa);
    GR_SETF( hw->FvA.y, snap_ya);
    GR_SETF( hw->FvB.x, snap_xa);
    GR_SETF( hw->FvC.x, snap_xb);
    m = 1.0f / dx;
    GR_SETF( hw->FvB.y, snap_ya + _GlideRoot.pool.f1);
    dp = a_alpha * dy;
    GR_SETF( hw->FvC.y, snap_yb + _GlideRoot.pool.f1);
    
    grSetVertexParameter(a);

    GR_SETF( hw->Fdady, -a_alpha);
    dp *= m;
    GR_SETF( hw->Fdadx, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
      
    dp = b_alpha * dy;
    GR_SETF( hw->FvB.x, snap_xb);
    dp = a_alpha - b_alpha + dp;
    GR_SETF( hw->FvB.y, snap_yb);

    grSetVertexParameter(a);
    
    GR_SETF( hw->Fdady, -b_alpha);
    dp *= m;
    GR_SETF( hw->Fdadx, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    
    break;
    
  case aaEdgeSenseLeft:
    /* pci comamnd 16 + 2 * n */
    GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
    GR_SETF( hw->FvA.y, snap_ya);
    GR_SETF( hw->FvB.x, snap_xa);
    GR_SETF( hw->FvB.y, snap_ya);
    GR_SETF( hw->FvC.y, snap_yb);    
    m = 1.0f / dy;
    GR_SETF( hw->FvA.x, snap_xa - _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.x, snap_xb - _GlideRoot.pool.f1);

    grSetVertexParameter(a);
    GR_SET(hw->Fa, 0);

    GR_SETF( hw->Fdadx, a_alpha);
    dp = - a_alpha * dx * m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dy ) );
    
    GR_SETF( hw->FvA.x, snap_xa);
    dp = b_alpha * dx;
    GR_SETF( hw->FvB.x, snap_xb);
    dp = a_alpha - dp - b_alpha;
    GR_SETF( hw->FvB.y, snap_yb);

    grSetVertexParameter(a);

    GR_SETF( hw->Fdadx, b_alpha);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dy ) );
    
    break;
    
  case aaEdgeSenseBottom:
    
    if (dy < -1.0f) {
      /* pci comamnd 17 + 2 * n */

      GR_SET_EXPECTED_SIZE(12+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, snap_xa);
      GR_SETF( hw->FvB.x, snap_xa);
      GR_SETF( hw->FvB.y, snap_ya);
      GR_SETF( hw->FvC.x, snap_xb);      
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, snap_ya - _GlideRoot.pool.f1);
      GR_SETF( hw->FvC.y, snap_yb - _GlideRoot.pool.f1);

      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);
      
      dp = a_alpha * dy;
      dp *= m;
      GR_SETF( hw->Fdadx, -dp);
      GR_SETF( hw->Fdady, a_alpha);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    

      GR_SETF( hw->FvA.y, snap_ya);
      dp = b_alpha * dy;
      GR_SETF( hw->FvB.x, snap_xb);
      GR_SETF( hw->FvB.y, snap_yb - _GlideRoot.pool.f1);
      dp = a_alpha - b_alpha - dp;
      GR_SETF( hw->FvC.y, snap_yb);
      grSetVertexParameter(a);
      
      GR_SETF( hw->Fdady, b_alpha);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    }
    else if (dy == 0.0f) {

      /* pci comamnd 16 + 2 * n */
      GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, snap_xa);
      GR_SETF( hw->FvC.x, snap_xb);      
      GR_SETF( hw->FvC.y, snap_yb);
      GR_SETF( hw->FvB.x, snap_xb);
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, snap_ya - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.y, snap_yb - _GlideRoot.pool.f1);

      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);

      GR_SET( hw->Fdadx, 0);
      GR_SETF( hw->Fdady, b_alpha);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    
      GR_SETF( hw->FvB.x, snap_xa);
      GR_SETF( hw->FvB.y, snap_ya);
      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);
    
      dp = a_alpha - b_alpha;
      GR_SETF( hw->Fdady, a_alpha);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    }
    else {

      /* pci comamnd 20 + 2 * n */
      GR_SET_EXPECTED_SIZE(24+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, snap_xa);
      GR_SETF( hw->FvB.x, snap_xb);
      GR_SETF( hw->FvC.x, snap_xa);      
      GR_SETF( hw->FvC.y, snap_ya);
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, snap_ya - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.y, snap_yb - _GlideRoot.pool.f1);

      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);
      
      dp = -a_alpha * dy * m;
      GR_SETF( hw->Fdadx, dp);
      GR_SETF( hw->Fdady, a_alpha);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );

      GR_SETF( hw->FvA.x, snap_xb);
      GR_SETF( hw->FvA.y, snap_yb - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.x, snap_xa);
      GR_SETF( hw->FvB.y, snap_ya);
      dp = b_alpha * dy;
      GR_SETF( hw->FvC.x, snap_xb);      
      GR_SETF( hw->FvC.y, snap_yb);

      grSetVertexParameter(b);
      GR_SET(hw->Fa, 0);
      dp = a_alpha - dp - b_alpha;
      GR_SETF( hw->Fdady, b_alpha);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    }
    
    break;
  case aaEdgeSenseRight:
    /* pci comamnd 16 + 2 * n */
    GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
    GR_SETF( hw->FvA.y, snap_ya);
    GR_SETF( hw->FvB.x, snap_xa);
    GR_SETF( hw->FvB.y, snap_ya);
    GR_SETF( hw->FvC.y, snap_yb);
    m = 1.0f / dy;
    GR_SETF( hw->FvA.x, snap_xa + _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.x, snap_xb + _GlideRoot.pool.f1);

    grSetVertexParameter(a);
    GR_SET(hw->Fa, 0);

    dp = a_alpha * dx;
    GR_SETF( hw->Fdadx, -a_alpha);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dy ) );
    
    GR_SETF( hw->FvA.x, snap_xa);
    dp = b_alpha * dx;
    GR_SETF( hw->FvB.x, snap_xb);
    dp = dp + a_alpha - b_alpha;
    GR_SETF( hw->FvB.y, snap_yb);
    grSetVertexParameter(a);
    
    GR_SETF( hw->Fdadx, -b_alpha);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dy ) );   
    break;
  }
  GR_CHECK_SIZE_SLOPPY();

#undef FN_NAME
} /* aaCalcEdgeSense */

/*---------------------------------------------------------------------------
** grAADrawTriangle
**
**  NOTE:  This doesn't quite work yet
*/
GR_ENTRY(grAADrawTriangle, void, (const void *a, const void *b, const void *c, FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias ) ) 
{
  FxU32
    fbzMode,                    /* What we write to fbzMode */
    fbzModeOld;                 /* Squirrel away current fbzMode */

  GR_BEGIN_NOFIFOCHECK("grAADrawTriangle",96);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x,0x%x,0x%x,%d,%d,%d)\n",
                  a,b,c,ab_antialias,bc_antialias,ca_antialias));

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_CLIP_COORDS) {
    _grAAVpDrawTriangle(a, b, c,ab_antialias,bc_antialias,ca_antialias);
    return;
  }

  fbzModeOld = gc->state.fbi_config.fbzMode;
  if ( TRISETUP ( a, b, c ) <= 0) {/* backfaced or zero area */
    GR_END();
    return;
  }
  /* Disable depth buffer writes for edge triangles */
  fbzMode = fbzModeOld;
  fbzMode &= ~(SST_ZAWRMASK);
  GR_SET_EXPECTED_SIZE(4);
  GR_SET( hw->fbzMode, fbzMode );
  GR_CHECK_SIZE();
  if ( ab_antialias )
    aaDrawArrayEdgeSense((float *)a, (float *)b, (float *)c);
  
  if ( bc_antialias )
    aaDrawArrayEdgeSense((float *)b, (float *)c, (float *)a);
  
  if ( ca_antialias )
    aaDrawArrayEdgeSense((float *)c, (float *)a, (float *)b);

  /* Restore the old fbzMode */
  GR_SET_EXPECTED_SIZE(4);
  GR_SET( hw->fbzMode, fbzModeOld );
  GR_END();
} /* grAADrawTriangle */

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

  int i;
  float *fp;
  struct dataList_s *dlp;
  float dp;
  float PX, PY;
  FxI32 stride = mode;
  FxI32 index;
  union { float f; FxI32 i; } j;

  GR_BEGIN_NOFIFOCHECK("grAADrawPoint",94);

  GR_FLUSH_STATE();

  if (stride == 0)
    stride = gc->state.vData.vStride;

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    for (index = 0; index < count; index++) {
      float *e;
      
      GR_SET_EXPECTED_SIZE(((_GlideRoot.curTriSize - _GlideRoot.curTriSizeNoGradient) >> 3) * 12 + 116);
      
      if (mode)
        e = *(float **)pointers;
      else
        e = pointers;
      
      pointers = (float *)pointers + stride;
      
      PX = (volatile float) (VX(e)+SNAP_BIAS);
      PY = (volatile float) (VY(e)+SNAP_BIAS);
      
      if (gc->state.vData.colorType == GR_FLOAT) {
        j.f = *(float *)((unsigned char *)e + gc->state.vData.aInfo.offset);
      }
      else {
        j.f = FbARRAY(e, gc->state.vData.pargbInfo.offset + GR_PARGB_A);
      }
      j.i ^= 0x80000000;
      
      /* 1st triangle EDA */
      GR_SETF(hw->FvA.x, PX);
      dp = PX - _GlideRoot.pool.f1;
      GR_SETF(hw->FvB.x, dp);
      dp = PX + _GlideRoot.pool.f1;
      GR_SETF(hw->FvC.x, dp);
      dlp = gc->dataList;
      GR_SETF(hw->FvA.y, PY);
      dp = PY + _GlideRoot.pool.f1;
      GR_SETF(hw->FvB.y, dp);
      i = dlp->i;
      GR_SETF(hw->FvC.y, dp);
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
            dp = FbARRAY(e,bddr);
          }
          else {
            dp = FARRAY(e,i);
          }
          GR_SETF( fp[0], dp);
          GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
          dlp++;
          i = dlp->i;
          GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
        }
      }
      GR_SET(hw->Fdady, j.i);
      P6FENCE_CMD( GR_SET(hw->triangleCMD, 0xffffffff) );
      
      /* 2nd triangle BEC */
      dp = PX + _GlideRoot.pool.f1;
      GR_SETF(hw->FvA.x, dp);
      GR_SETF(hw->FvB.x, PX);
      dp = PY - _GlideRoot.pool.f1;
      GR_SETF(hw->FvA.y, dp);
      GR_SETF(hw->FvB.y, PY);
    
      GR_SET(hw->Fa, 0);
      GR_SET(hw->Fdadx, j.i);
      GR_SET(hw->Fdady, 0);
      P6FENCE_CMD( GR_SET(hw->triangleCMD, 0xffffffff) );
      
      /* 3rd triangle AED */
      dp = PX - _GlideRoot.pool.f1;
      GR_SETF(hw->FvA.x, dp);
      GR_SETF(hw->FvC.x, dp);
      
      j.i ^= 0x80000000;
      GR_SET(hw->Fa, 0);
      GR_SET(hw->Fdadx, j.i);
      P6FENCE_CMD( GR_SET(hw->triangleCMD, 1) );
      
      /* 4th triangle ABE */
      dp = PX + _GlideRoot.pool.f1;
      GR_SETF(hw->FvB.x, dp);
      GR_SETF(hw->FvC.x, PX);
      dp = PY - _GlideRoot.pool.f1;
      GR_SETF(hw->FvB.y, dp);
      GR_SETF(hw->FvC.y, PY);
      
      GR_SET(hw->Fa, 0);
      GR_SET(hw->Fdadx, 0);
      GR_SET(hw->Fdady, j.i);
      P6FENCE_CMD( GR_SET(hw->triangleCMD, 1) );
      
      GR_END();
    }
  }
  else {
    for (index = 0; index < count; index++) {
      float *e, oow;
      int k;
      
      GR_SET_EXPECTED_SIZE(((_GlideRoot.curTriSize - _GlideRoot.curTriSizeNoGradient) >> 3) * 12 + 116);
      
      if (mode)
        e = *(float **)pointers;
      else
        e = pointers;
      
      oow = 1.0f / FARRAY(e, gc->state.vData.wInfo.offset);
      pointers = (float *)pointers + stride;
      
      PX = (volatile float) (VX(e)*oow*gc->state.Viewport.hwidth+
                             gc->state.Viewport.ox + SNAP_BIAS);
      PY = (volatile float) (VY(e)*oow*gc->state.Viewport.hheight+
                             gc->state.Viewport.oy + SNAP_BIAS);
      
      if (gc->state.vData.colorType == GR_FLOAT) {
        j.f = FARRAY(e, gc->state.vData.aInfo.offset) * _GlideRoot.pool.f255;
      }
      else {
        j.f = FbARRAY(e, gc->state.vData.pargbInfo.offset + GR_PARGB_A);
      }
      j.i ^= 0x80000000;
      
      /* 1st triangle EDA */
      GR_SETF(hw->FvA.x, PX);
      dp = PX - _GlideRoot.pool.f1;
      GR_SETF(hw->FvB.x, dp);
      dp = PX + _GlideRoot.pool.f1;
      GR_SETF(hw->FvC.x, dp);
      dlp = gc->dataList;
      GR_SETF(hw->FvA.y, PY);
      dp = PY + _GlideRoot.pool.f1;
      GR_SETF(hw->FvB.y, dp);
      i = dlp->i;
      GR_SETF(hw->FvC.y, dp);
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
              dp = FbARRAY(e,bddr);
            }
            else {
              dp = FARRAY(e,i) * _GlideRoot.pool.f255;
            }
            GR_SETF( fp[0], dp );
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
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
          dp = FARRAY(e,i) * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
          GR_SETF( fp[0], dp );
          GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
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
            dp = FbARRAY(e,bddr);
          }
          else {
            dp = FARRAY(e,i) * _GlideRoot.pool.f255;
          }
          GR_SETF( fp[0], dp );
          GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
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
            dp = FARRAY(e,i) * oow * gc->state.tmu_config[0].st_scale[k];
            GR_SETF( fp[0], dp );
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
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
            GR_SETF( fp[0], FARRAY(e,i) * oow );
          }
          else {
            GR_SETF( fp[0], oow );
          }
          GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
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
            dp = FARRAY(e,i) * oow;
          }
          else {
            dp = oow;
          }
          GR_SETF( fp[0], dp );
          GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
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
            dp = FARRAY(e,i) * oow * gc->state.tmu_config[1].st_scale[k];
            GR_SETF( fp[0], dp );
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
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
            dp = FARRAY(e,i) * oow;
          }
          else {
            dp = oow;
          }
          GR_SETF( fp[0], dp );
          GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
          dlp++;
          i = dlp->i;
          GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
        }
      }

      GR_SET(hw->Fdady, j.i);
      P6FENCE_CMD( GR_SET(hw->triangleCMD, 0xffffffff) );
      
      /* 2nd triangle BEC */
      dp = PX + _GlideRoot.pool.f1;
      GR_SETF(hw->FvA.x, dp);
      GR_SETF(hw->FvB.x, PX);
      dp = PY - _GlideRoot.pool.f1;
      GR_SETF(hw->FvA.y, dp);
      GR_SETF(hw->FvB.y, PY);
    
      GR_SET(hw->Fa, 0);
      GR_SET(hw->Fdadx, j.i);
      GR_SET(hw->Fdady, 0);
      P6FENCE_CMD( GR_SET(hw->triangleCMD, 0xffffffff) );
      
      /* 3rd triangle AED */
      dp = PX - _GlideRoot.pool.f1;
      GR_SETF(hw->FvA.x, dp);
      GR_SETF(hw->FvC.x, dp);
      
      j.i ^= 0x80000000;
      GR_SET(hw->Fa, 0);
      GR_SET(hw->Fdadx, j.i);
      P6FENCE_CMD( GR_SET(hw->triangleCMD, 1) );
      
      /* 4th triangle ABE */
      dp = PX + _GlideRoot.pool.f1;
      GR_SETF(hw->FvB.x, dp);
      GR_SETF(hw->FvC.x, PX);
      dp = PY - _GlideRoot.pool.f1;
      GR_SETF(hw->FvB.y, dp);
      GR_SETF(hw->FvC.y, PY);
      
      GR_SET(hw->Fa, 0);
      GR_SET(hw->Fdadx, 0);
      GR_SET(hw->Fdady, j.i);
      P6FENCE_CMD( GR_SET(hw->triangleCMD, 1) );
      
      GR_END();
    }
  }

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
  float           adx;         /* |dX| */
  float *tv;
  struct dataList_s *dlp;
  int i;
  float *fp, dp = 0.0f;   /* dp may be used unitialised at line 1715 ? */
  float tmp1, tmp2;
  float dx, dy;           /* delta X and Y */
  FxI32 stride = mode;
  FxI32 index;
  float v1_alpha, v2_alpha;

  GR_BEGIN_NOFIFOCHECK("grAADrawLine",95);

  GDBG_INFO((95,"_grAADrawLineStrip(%d,%d,%d,0x%x)\n",mode,ltype,count,pointers));

  GR_FLUSH_STATE();

  if (stride == 0)
    stride = gc->state.vData.vStride;

  if (ltype == GR_LINES)
    count >>= 1;    /* line list */
  else
    count -= 1;     /* strip line */

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    for (index = 0; index < count; index++) {
      float snap_xv1, snap_xv2, snap_yv1, snap_yv2;
      float *v1, *v2;
      
      GR_SET_EXPECTED_SIZE(((_GlideRoot.curTriSize - _GlideRoot.curTriSizeNoGradient) >> 3) * 24 + 108);
      
      if (mode) {
        v1 = *(float **)pointers;
        v2 = *((float **)pointers + 1);
      }
      else {
        v1 = pointers;
        v2 = (float *)pointers + stride;
      }
      pointers = (float *)pointers + stride;
      if (ltype == GR_LINES)
        pointers = (float *)pointers + stride;
      
      /* draw from low Y to high Y */
      if ( VY(v2) < VY(v1) ) {
        tv = v1; v1 = v2; v2 = tv;
      }
      
      /* compute deltas and absolute deltas */
      snap_xv1 = (volatile float) (VX(v1) + SNAP_BIAS);
      snap_xv2 = (volatile float) (VX(v2) + SNAP_BIAS);
      snap_yv1 = (volatile float) (VY(v1) + SNAP_BIAS);
      snap_yv2 = (volatile float) (VY(v2) + SNAP_BIAS);
      dx = adx = snap_xv1 - snap_xv2;
      dy = snap_yv2 - snap_yv1;
      if ( adx < 0 )
        adx = -adx;
      
      if (gc->state.vData.colorType == GR_FLOAT) {
        v1_alpha = FARRAY(v1,gc->state.vData.aInfo.offset);
        v2_alpha = FARRAY(v2,gc->state.vData.aInfo.offset);
      }
      else {
        v1_alpha = FbARRAY(v1,gc->state.vData.pargbInfo.offset + GR_PARGB_A);
        v2_alpha = FbARRAY(v2,gc->state.vData.pargbInfo.offset + GR_PARGB_A);
      }

      if ( adx >= dy ) {           /* X major line */
        
        if (dx == 0.0f)
          goto Done;
        
        /* 1st triangle A-B-V2*/
        GR_SETF( hw->FvA.x, snap_xv1);
        dx = _GlideRoot.pool.f1 / dx;
        GR_SETF( hw->FvA.y, snap_yv1 - _GlideRoot.pool.f1);
        tmp1 = - v1_alpha * dy;
        GR_SETF( hw->FvB.x, snap_xv2);
        GR_SETF( hw->FvB.y, snap_yv2 - _GlideRoot.pool.f1);
        tmp2 = - v2_alpha * dy;
        GR_SETF( hw->FvC.x, snap_xv2);
        dlp = gc->dataList;
        GR_SETF( hw->FvC.y, snap_yv2);
        i = dlp->i;
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
              /* packed data (color) */
              dp = FbARRAY(v1,bddr);
              GR_SETF( fp[0], dp );
              dp -= FbARRAY(v2,bddr);
            }
            else {
              /* non-packed data */
              dp = FARRAY(v1,i);
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i);
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
          }
        }
        GR_SET( hw->Fa , 0 );
        dp = - tmp2 * dx;
        GR_SETF( hw->Fdadx , dp );
        GR_SETF( hw->Fdady , v2_alpha );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dx ) );
      
        /* 2nd triangle A-V1-V2*/
        GR_SETF( hw->FvB.x, snap_xv1);
        grSetVertexParameter(v1);
        GR_SET( hw->Fa , 0 );
        dp = v1_alpha - v2_alpha;
        GR_SETF( hw->FvB.y, snap_yv1);
        dp -= tmp1;
        GR_SETF( hw->Fdady , v1_alpha );
        dp *= dx;
        GR_SETF( hw->Fdadx , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dx ) );
      
        /* 3rd triangle v1-E-F */
        GR_SETF( hw->FvA.y, snap_yv1);
        GR_SETF( hw->FvB.y, snap_yv1 + _GlideRoot.pool.f1);
        GR_SETF( hw->FvC.y, snap_yv2 + _GlideRoot.pool.f1);
        
        grSetVertexParameter(v1);
      
        GR_SETF( hw->Fdady , - v1_alpha );
        dp = tmp1 * dx;
        GR_SETF( hw->Fdadx , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dx ) );
        
        /* 4th triangle V1-V2-F*/
        GR_SETF( hw->FvB.x, snap_xv2);
        dp = v1_alpha - v2_alpha + tmp2;
        GR_SETF( hw->FvB.y, snap_yv2);
        dp *= dx;
        
        grSetVertexParameter(v1);
        
        GR_SETF( hw->Fdadx , dp ); 
        GR_SETF( hw->Fdady , - v2_alpha );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dx ) );
        
      } else {                      /* Y major line */
      
        if (dy == 0.0f)
          goto Done;
        
        /* 1st triangle v1-v2-b */
        GR_SETF( hw->FvA.x, snap_xv1);
        tmp1 = v1_alpha * dx;
        GR_SETF( hw->FvA.y, snap_yv1);
        tmp2 = v2_alpha * dx;
        GR_SETF( hw->FvB.x, snap_xv2);
        dy = - 1.0f / dy;
        GR_SETF( hw->FvB.y, snap_yv2);
        GR_SETF( hw->FvC.x, snap_xv2 - _GlideRoot.pool.f1);
        dlp = gc->dataList;
        GR_SETF( hw->FvC.y, snap_yv2);
        i = dlp->i;
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
              /* packed data (color) */
              dp = FbARRAY(v1,bddr);
              GR_SETF( fp[0], dp );
              dp -= FbARRAY(v2,bddr);
            }
            else {
              /* non-packed data */
              dp = FARRAY(v1,i);
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i);
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
          }
        }
        dp = v1_alpha - tmp2 - v2_alpha;
        GR_SETF( hw->Fdadx , v2_alpha );
        dp *= dy;
        GR_SETF( hw->Fdady , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dy ) );
        
        /* 2nd triangle v1-a-b */
        
        GR_SETF( hw->FvB.x, snap_xv1 - _GlideRoot.pool.f1);
        GR_SETF( hw->FvB.y, snap_yv1);
        grSetVertexParameter(v1);
        GR_SETF( hw->Fdadx , v1_alpha );
        dp = - tmp1 * dy;
        GR_SETF( hw->Fdady , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dy ) );
        
        /* 3rd triangle e-v1-v2 */
        GR_SETF( hw->FvA.x, snap_xv1 + _GlideRoot.pool.f1);
        dp = v1_alpha - v2_alpha + tmp1;
        GR_SETF( hw->FvB.x, snap_xv1);
        dp *= dy;
        GR_SETF( hw->FvC.x, snap_xv2);
        
        grSetVertexParameter(v1);
        GR_SET( hw->Fa , 0 );
        GR_SETF( hw->Fdadx , - v1_alpha );
        GR_SETF( hw->Fdady , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dy ) );
        
        /* 4th triangle e-f-v2 */
        GR_SETF( hw->FvB.x, snap_xv2 + _GlideRoot.pool.f1);
        GR_SETF( hw->FvB.y, snap_yv2);
        
        grSetVertexParameter(v1);
        GR_SET( hw->Fa , 0 );
        
        GR_SETF( hw->Fdadx , - v2_alpha );
        dp = tmp2 * dy;
        GR_SETF( hw->Fdady , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dy ) );
        
      }
      GR_CHECK_SIZE_SLOPPY();
    Done:
      ;
    }
  }
  else {
    float oowa, oowb;
    float fax, fay, fbx, fby;
    int k;
    float v1_alpha, v2_alpha;

    for (index = 0; index < count; index++) {
      float *v1, *v2;
      
      GR_SET_EXPECTED_SIZE(((_GlideRoot.curTriSize - _GlideRoot.curTriSizeNoGradient) >> 3) * 24 + 108);
      
      if (mode) {
        v1 = *(float **)pointers;
        v2 = *((float **)pointers + 1);
      }
      else {
        v1 = pointers;
        v2 = (float *)pointers + stride;
      }
      pointers = (float *)pointers + stride;
      if (ltype == GR_LINES)
        pointers = (float *)pointers + stride;
      
      oowa = 1.0f / FARRAY(v1, gc->state.vData.wInfo.offset);
      oowb = 1.0f / FARRAY(v2, gc->state.vData.wInfo.offset);
      fax = (volatile float) (FARRAY(v1, 0)*oowa*gc->state.Viewport.hwidth+gc->state.Viewport.ox + SNAP_BIAS);
      fbx = (volatile float) (FARRAY(v2, 0)*oowb*gc->state.Viewport.hwidth+gc->state.Viewport.ox + SNAP_BIAS);
      fay = (volatile float) (FARRAY(v1, 4)*oowa*gc->state.Viewport.hheight+gc->state.Viewport.oy + SNAP_BIAS);
      fby = (volatile float) (FARRAY(v2, 4)*oowb*gc->state.Viewport.hheight+gc->state.Viewport.oy + SNAP_BIAS);

      /* draw from low Y to high Y */
      if ( fby < fay ) {
        float tmp;
        tv = v1; v1 = v2; v2 = tv;
        tmp = oowa; oowa = oowb; oowb = tmp;
        tmp = fax;  fax = fbx;   fbx = tmp;
        tmp = fay;  fay = fby;   fby = tmp;
      }

            
      /* compute deltas and absolute deltas */
      dx = adx = fax - fbx;
      dy = fby - fay;
      if ( adx < 0 )
        adx = -adx;
      
      if (gc->state.vData.colorType == GR_FLOAT) {
        v1_alpha = FARRAY(v1,gc->state.vData.aInfo.offset) * _GlideRoot.pool.f255;
        v2_alpha = FARRAY(v2,gc->state.vData.aInfo.offset) * _GlideRoot.pool.f255;
      }
      else {
        v1_alpha = FbARRAY(v1,gc->state.vData.pargbInfo.offset + GR_PARGB_A);
        v2_alpha = FbARRAY(v2,gc->state.vData.pargbInfo.offset + GR_PARGB_A);
      }

      if ( adx >= dy ) {           /* X major line */
        
        if (dx == 0.0f)
          goto Done_Vp;
        
        /* 1st triangle A-B-V2*/
        GR_SETF( hw->FvA.x, fax);
        dx = _GlideRoot.pool.f1 / dx;
        GR_SETF( hw->FvA.y, fay - _GlideRoot.pool.f1);
        tmp1 = - v1_alpha * dy;
        GR_SETF( hw->FvB.x, fbx);
        GR_SETF( hw->FvB.y, fby - _GlideRoot.pool.f1);
        tmp2 = - v2_alpha * dy;
        GR_SETF( hw->FvC.x, fbx);
        dlp = gc->dataList;
        GR_SETF( hw->FvC.y, fby);
        i = dlp->i;

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
                dp = FbARRAY(v1,bddr);
                GR_SETF( fp[0], dp );
                dp -= FbARRAY(v2,bddr);
              }
              else {
                dp = FARRAY(v1,i) * _GlideRoot.pool.f255;
                GR_SETF( fp[0], dp );
                dp -= FARRAY(v2, i) * _GlideRoot.pool.f255;
              }
              GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
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
            dp = FARRAY(v1,i) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
            GR_SETF( fp[0], dp );
            dp -= FARRAY(v2, i) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
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
              dp = FbARRAY(v1,bddr);
              GR_SETF( fp[0], dp );
              dp -= FbARRAY(v2,bddr);
            }
            else {
              dp = FARRAY(v1,i) * _GlideRoot.pool.f255;
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i) * _GlideRoot.pool.f255;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
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
              dp = FARRAY(v1,i) * oowa * 256.f;
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i) * oowb * 256.f;
              GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
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
              GR_SETF( fp[0], FARRAY(v1, i) * oowa );
              dp -= FARRAY(v2, i) * oowb;
            }
            else {
              GR_SETF( fp[0], oowa );
              dp -= oowb;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
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
              dp = FARRAY(v1,i) * oowa;
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i) * oowb;
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp -= oowb;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
          }
        }
        if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) {
          fp = dlp->addr;
          if (i & 1) {     /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( fp[0], 0.0f);
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
          }
          else {
            dp = FARRAY(v1,i) * oowa * 256.f;
            GR_SETF( fp[0], dp );
            dp -= FARRAY(v2, i) * oowb * 256.f;
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
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
              dp = FARRAY(v1,i) * oowa;
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i) * oowb;
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp -= oowb;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
          }
        }

        GR_SET( hw->Fa , 0 );
        dp = - tmp2 * dx;
        GR_SETF( hw->Fdadx , dp );
        GR_SETF( hw->Fdady , v2_alpha );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dx ) );
      
        /* 2nd triangle A-V1-V2*/
        GR_SETF( hw->FvB.x, fax);
        grVpSetVertexParameter(oowa, v1);
        GR_SET( hw->Fa , 0 );
        dp = v1_alpha - v2_alpha;
        GR_SETF( hw->FvB.y, fay);
        dp -= tmp1;
        GR_SETF( hw->Fdady , v1_alpha );
        dp *= dx;
        GR_SETF( hw->Fdadx , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dx ) );
      
        /* 3rd triangle v1-E-F */
        GR_SETF( hw->FvA.y, fay);
        GR_SETF( hw->FvB.y, fay + _GlideRoot.pool.f1);
        GR_SETF( hw->FvC.y, fby + _GlideRoot.pool.f1);
        
        grVpSetVertexParameter(oowa, v1);
      
        GR_SETF( hw->Fdady , - v1_alpha );
        dp = tmp1 * dx;
        GR_SETF( hw->Fdadx , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dx ) );
        
        /* 4th triangle V1-V2-F*/
        GR_SETF( hw->FvB.x, fbx);
        dp = v1_alpha - v2_alpha + tmp2;
        GR_SETF( hw->FvB.y, fby);
        dp *= dx;
        
        grVpSetVertexParameter(oowa, v1);
        
        GR_SETF( hw->Fdadx , dp ); 
        GR_SETF( hw->Fdady , - v2_alpha);
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dx ) );

      } else {                      /* Y major line */
      
        if (dy == 0.0f)
          goto Done;
        
        /* 1st triangle v1-v2-b */
        GR_SETF( hw->FvA.x, fax);
        tmp1 = v1_alpha * dx;
        GR_SETF( hw->FvA.y, fay);
        tmp2 = v2_alpha * dx;
        GR_SETF( hw->FvB.x, fbx);
        dy = - 1.0f / dy;
        GR_SETF( hw->FvB.y, fby);
        GR_SETF( hw->FvC.x, fbx - _GlideRoot.pool.f1);
        dlp = gc->dataList;
        GR_SETF( hw->FvC.y, fby);
        i = dlp->i;
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
                dp = FbARRAY(v1,bddr);
                GR_SETF( fp[0], dp );
                dp -= FbARRAY(v2,bddr);
              }
              else {
                dp = FARRAY(v1,i) * _GlideRoot.pool.f255;
                GR_SETF( fp[0], dp );
                dp -= FARRAY(v2, i) * _GlideRoot.pool.f255;
              }
              GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
              dlp++;
              i = dlp->i;
              GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
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
            dp = FARRAY(v1,i) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
            GR_SETF( fp[0], dp );
            dp -= FARRAY(v2, i) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
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
              dp = FbARRAY(v1,bddr);
              GR_SETF( fp[0], dp );
              dp -= FbARRAY(v2,bddr);
            }
            else {
              dp = FARRAY(v1,i) * _GlideRoot.pool.f255;
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i) * _GlideRoot.pool.f255;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
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
              dp = FARRAY(v1,i) * oowa * gc->state.tmu_config[0].st_scale[k];
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i) * oowb * gc->state.tmu_config[0].st_scale[k];
              GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
              dlp++;
              i = dlp->i;
              GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
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
              GR_SETF( fp[0], FARRAY(v1,i) * oowa );
              dp -= FARRAY(v2,i) * oowb;
            }
            else {
              GR_SETF( fp[0], oowa );
              dp -= oowb;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
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
              dp = FARRAY(v1,i) * oowa;
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i) * oowb;
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp -= oowb;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
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
              dp = FARRAY(v1,i) * oowa * gc->state.tmu_config[0].st_scale[k];
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i) * oowb * gc->state.tmu_config[0].st_scale[k];
              GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
              dlp++;
              i = dlp->i;
              GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
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
              dp = FARRAY(v1,i) * oowa;
              GR_SETF( fp[0], dp );
              dp -= FARRAY(v2, i) * oowb;
            }
            else {
              dp = oowa;
              GR_SETF( fp[0], dp );
              dp -= oowb;
            }
            GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
            dlp++;
            i = dlp->i;
            GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
          }
        }

        dp = v1_alpha - tmp2 - v2_alpha;
        GR_SETF( hw->Fdadx , v2_alpha );
        dp *= dy;
        GR_SETF( hw->Fdady , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dy ) );
        
        /* 2nd triangle v1-a-b */
        
        GR_SETF( hw->FvB.x, fax - _GlideRoot.pool.f1);
        GR_SETF( hw->FvB.y, fay);
        grVpSetVertexParameter(oowa, v1);
        GR_SETF( hw->Fdadx , v1_alpha );
        dp = - tmp1 * dy;
        GR_SETF( hw->Fdady , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dy ) );
        
        /* 3rd triangle e-v1-v2 */
        GR_SETF( hw->FvA.x, fax + _GlideRoot.pool.f1);
        dp = v1_alpha - v2_alpha + tmp1;
        GR_SETF( hw->FvB.x, fax);
        dp *= dy;
        GR_SETF( hw->FvC.x, fbx);
        
        grVpSetVertexParameter(oowa, v1);
        GR_SET( hw->Fa , 0 );
        GR_SETF( hw->Fdadx , - v1_alpha );
        GR_SETF( hw->Fdady , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dy ) );
        
        /* 4th triangle e-f-v2 */
        GR_SETF( hw->FvB.x, fbx + _GlideRoot.pool.f1);
        GR_SETF( hw->FvB.y, fby);
        
        grVpSetVertexParameter(oowa, v1);
        GR_SET( hw->Fa , 0 );
        
        GR_SETF( hw->Fdadx , - v2_alpha );
        dp = tmp2 * dy;
        GR_SETF( hw->Fdady , dp );
        P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dy ) );
        
      }
      GR_CHECK_SIZE_SLOPPY();
    Done_Vp:
      ;
    }
  }

#undef FN_NAME
} /* _grAADrawLineStrip */

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
  FxI32 stride = mode;

  GDBG_INFO((96, "_grAADrawTriangles (count = %d, pointers = 0x%x)\n",
            count, pointers));

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
        int culltest = gc->state.cull_mode;
        union { float f; int i; } ay;
        union { float f; int i; } by;
        union { float f; int i; } cy;
        ay.f = a[(gc->state.vData.vertexInfo.offset>>2)+1];
        by.f = b[(gc->state.vData.vertexInfo.offset>>2)+1];
        cy.f = c[(gc->state.vData.vertexInfo.offset>>2)+1];
        
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
        dxAB = fa[0] - fb[0];
        dxBC = fb[0] - fc[0];
        
        dyAB = fa[1] - fb[1];
        dyBC = fb[1] - fc[1];
        
        /* Stash the area in the float pool for easy access */
        _GlideRoot.pool.temp1.f = dxAB * dyBC - dxBC * dyAB;
        
#define FloatVal(__f) (((__f) < 786432.875) ? (__f) : ((__f) - 786432.875))
        /* Zero-area triangles are BAD!! */
        if ((_GlideRoot.pool.temp1.i & 0x7FFFFFFF) == 0) {
          GDBG_INFO((291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                     FloatVal(fa[0]), FloatVal(fa[1]), 
                     FloatVal(fb[0]), FloatVal(fb[1]), 
                     FloatVal(fc[0]), FloatVal(fc[1]), 
                     _GlideRoot.pool.temp1.f, gc->state.cull_mode, culltest));

          goto done;
        }
    
        /* Backface culling, use sign bit as test */
        if ((gc->state.cull_mode != GR_CULL_DISABLE) &&
            ((_GlideRoot.pool.temp1.i ^ (culltest << 31)) >= 0)) {
      
          GDBG_INFO((291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                     FloatVal(fa[0]), FloatVal(fa[1]), 
                     FloatVal(fb[0]), FloatVal(fb[1]), 
                     FloatVal(fc[0]), FloatVal(fc[1]), 
                     _GlideRoot.pool.temp1.f, gc->state.cull_mode, culltest));
      
          goto done;
        }
      }
    } /* end culling test */
      
    grAADrawTriangle(a, b, c, FXTRUE, FXTRUE, FXTRUE);
 
  done:
    lPtr += 3;
    tCount+=3;
  }

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
aaVpDrawArrayEdgeSense(const float *a, const float *b, const float *c, float oowa, float oowb, float oowc)
{
#define FN_NAME "aaVpDrawArrayEdgeSense"
  GR_DCL_GC;
  GR_DCL_HW;

  float dx, dy, m, dp;
  float fax, fay, fbx, fby, fcx, fcy;
  const float *tv;
  aaEdgeSense sense;
  float a_alpha, b_alpha;

  fax = (volatile float) (FARRAY(a, 0)*oowa*gc->state.Viewport.hwidth+gc->state.Viewport.ox + SNAP_BIAS);
  fay = (volatile float) (FARRAY(a, 4)*oowa*gc->state.Viewport.hheight+gc->state.Viewport.oy + SNAP_BIAS);
  fbx = (volatile float) (FARRAY(b, 0)*oowb*gc->state.Viewport.hwidth+gc->state.Viewport.ox + SNAP_BIAS);
  fby = (volatile float) (FARRAY(b, 4)*oowb*gc->state.Viewport.hheight+gc->state.Viewport.oy + SNAP_BIAS);
  fcx = (volatile float) (FARRAY(c, 0)*oowb*gc->state.Viewport.hwidth+gc->state.Viewport.ox + SNAP_BIAS);
  fcy = (volatile float) (FARRAY(c, 4)*oowb*gc->state.Viewport.hheight+gc->state.Viewport.oy + SNAP_BIAS);

  if (fay >= fby) {
    float tmp;
    tv = a; a = b; b = tv;
    tmp = fax;  fax = fbx; fbx = tmp;
    tmp = fay;  fay = fby; fby = tmp;
    tmp = oowa; oowa = oowb; oowb = tmp;
  }

  dx = fax - fbx;
  dy = fay - fby;

  if (dx == 0.f) {
    sense = (fcx > fax) ? aaEdgeSenseLeft : aaEdgeSenseRight;
  }
  else if (dy == 0.0f) {
    sense = (fcy > fay) ? aaEdgeSenseBottom : aaEdgeSenseTop;
  }
  else {
    float dcax, dcay, dcbx, dcby, cp;
    dcax = fax - fcx;
    dcay = fay - fcy;
    dcbx = fbx - fcx;
    dcby = fby - fcy;
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

  if (gc->state.vData.colorType == GR_FLOAT) {
    a_alpha = FARRAY(a,gc->state.vData.aInfo.offset);
    b_alpha = FARRAY(b,gc->state.vData.aInfo.offset);
  }
  else {
    a_alpha = FbARRAY(a,gc->state.vData.pargbInfo.offset + GR_PARGB_A);
    b_alpha = FbARRAY(b,gc->state.vData.pargbInfo.offset + GR_PARGB_A);
  }

  switch (sense) {
  case aaEdgeSenseTop:
    /* pci comamnd 14 + 2 * n */
    GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSizeNoGradient << 1);
    GR_SETF( hw->FvA.x, fax);
    GR_SETF( hw->FvA.y, fay);
    GR_SETF( hw->FvB.x, fax);
    GR_SETF( hw->FvC.x, fbx);
    m = 1.0f / dx;
    GR_SETF( hw->FvB.y, fay + _GlideRoot.pool.f1);
    dp = FARRAY(a,gc->state.vData.aInfo.offset) * _GlideRoot.pool.f255 * dy;
    GR_SETF( hw->FvC.y, fby + _GlideRoot.pool.f1);
    
    grVpSetVertexParameter(oowa, a);

    GR_SETF( hw->Fdady, -a_alpha);
    dp *= m;
    GR_SETF( hw->Fdadx, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
      
    dp = b_alpha * dy;
    GR_SETF( hw->FvB.x, fbx);
    dp = a_alpha - b_alpha + dp;
    GR_SETF( hw->FvB.y, fby);

    grVpSetVertexParameter(oowa, a);
    
    GR_SETF( hw->Fdady, - b_alpha);
    dp *= m;
    GR_SETF( hw->Fdadx, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    
    break;
    
  case aaEdgeSenseLeft:
    /* pci comamnd 16 + 2 * n */
    GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
    GR_SETF( hw->FvA.y, fay);
    GR_SETF( hw->FvB.x, fax);
    GR_SETF( hw->FvB.y, fay);
    GR_SETF( hw->FvC.y, fby);    
    m = 1.0f / dy;
    GR_SETF( hw->FvA.x, fax - _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.x, fbx - _GlideRoot.pool.f1);

    grVpSetVertexParameter(oowa, a);
    GR_SET(hw->Fa, 0);

    GR_SETF( hw->Fdadx, a_alpha);
    dp = - a_alpha * dx * m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dy ) );
    
    GR_SETF( hw->FvA.x, fax);
    dp = b_alpha * dx;
    GR_SETF( hw->FvB.x, fbx);
    dp = a_alpha - dp - b_alpha;
    GR_SETF( hw->FvB.y, fby);

    grVpSetVertexParameter(oowa, a);

    GR_SETF( hw->Fdadx, b_alpha);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dy ) );
    
    break;
    
  case aaEdgeSenseBottom:
    
    if (dy < -1.0f) {
      /* pci comamnd 17 + 2 * n */

      GR_SET_EXPECTED_SIZE(12+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, fax);
      GR_SETF( hw->FvB.x, fax);
      GR_SETF( hw->FvB.y, fay);
      GR_SETF( hw->FvC.x, fbx);      
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, fay - _GlideRoot.pool.f1);
      GR_SETF( hw->FvC.y, fby - _GlideRoot.pool.f1);

      grVpSetVertexParameter(oowa, a);
      GR_SET(hw->Fa, 0);
      
      dp = a_alpha * dy;
      dp *= m;
      GR_SETF( hw->Fdadx, -dp);
      GR_SETF( hw->Fdady, a_alpha);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    

      GR_SETF( hw->FvA.y, fay);
      dp = b_alpha * dy;
      GR_SETF( hw->FvB.x, fbx);
      GR_SETF( hw->FvB.y, fby - _GlideRoot.pool.f1);
      dp = a_alpha - b_alpha - dp;
      GR_SETF( hw->FvC.y, fby);
      grVpSetVertexParameter(oowa, a);
      
      GR_SETF( hw->Fdady, b_alpha);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    }
    else if (dy == 0.0f) {

      /* pci comamnd 16 + 2 * n */
      GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, fax);
      GR_SETF( hw->FvC.x, fbx);      
      GR_SETF( hw->FvC.y, fby);
      GR_SETF( hw->FvB.x, fbx);
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, fay - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.y, fby - _GlideRoot.pool.f1);

      grVpSetVertexParameter(oowa, a);
      GR_SET(hw->Fa, 0);

      GR_SET( hw->Fdadx, 0);
      GR_SETF( hw->Fdady, b_alpha);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    
      GR_SETF( hw->FvB.x, fax);
      GR_SETF( hw->FvB.y, fay);
      grVpSetVertexParameter(oowa, a);
      GR_SET(hw->Fa, 0);
    
      dp = a_alpha - b_alpha;
      GR_SETF( hw->Fdady, a_alpha);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    }
    else {
      /* pci comamnd 20 + 2 * n */
      GR_SET_EXPECTED_SIZE(24+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, fax);
      GR_SETF( hw->FvB.x, fbx);
      GR_SETF( hw->FvC.x, fax);      
      GR_SETF( hw->FvC.y, fay);
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, fay - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.y, fby - _GlideRoot.pool.f1);

      grVpSetVertexParameter(oowa, a);
      GR_SET(hw->Fa, 0);
      
      dp = - a_alpha * dy * m;
      GR_SETF( hw->Fdadx, dp);
      GR_SETF( hw->Fdady, a_alpha);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
      
      GR_SETF( hw->FvA.x, fbx);
      GR_SETF( hw->FvA.y, fby - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.x, fax);
      GR_SETF( hw->FvB.y, fay);
      dp = b_alpha * dy;
      GR_SETF( hw->FvC.x, fbx);      
      GR_SETF( hw->FvC.y, fby);

      grVpSetVertexParameter(oowb, b);
      GR_SET(hw->Fa, 0);
      dp = a_alpha - dp - b_alpha;
      GR_SETF( hw->Fdady, b_alpha);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);

      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    }
    
    break;
  case aaEdgeSenseRight:
    /* pci comamnd 16 + 2 * n */
    GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
    GR_SETF( hw->FvA.y, fay);
    GR_SETF( hw->FvB.x, fax);
    GR_SETF( hw->FvB.y, fay);
    GR_SETF( hw->FvC.y, fby);
    m = 1.0f / dy;
    GR_SETF( hw->FvA.x, fax + _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.x, fbx + _GlideRoot.pool.f1);

    grVpSetVertexParameter(oowa, a);
    GR_SET(hw->Fa, 0);

    dp = a_alpha * dx;
    GR_SETF( hw->Fdadx, - a_alpha);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dy ) );
    
    GR_SETF( hw->FvA.x, fax);
    dp = b_alpha * dx;
    GR_SETF( hw->FvB.x, fbx);
    dp = dp + a_alpha - b_alpha;
    GR_SETF( hw->FvB.y, fby);
    grVpSetVertexParameter(oowa, a);
    
    GR_SETF( hw->Fdadx, - b_alpha);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dy ) );   
    break;
  }
  GR_CHECK_SIZE_SLOPPY();

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
_grAAVpDrawTriangle(const void *a, const void *b, const void *c,
                    FxBool ab_antialias,
                    FxBool bc_antialias,
                    FxBool ca_antialias)
{
#define FN_NAME "_grAAVpDrawTriangles"

  GR_DCL_GC;

  FxU32 fbzModeOld;                 /* Squirrel away current fbzMode */
  float oowa, oowb, oowc;

  GDBG_INFO((96, "_grAADrawTriangles (a = 0x%x,a = 0x%x,a = 0x%x, ab_aa = %d,bc_aa = %d, ca_aa = %d)\n",
             a, b, c, ab_antialias, bc_antialias, ca_antialias));

  /* backfaced or zero area */

  fbzModeOld = gc->state.fbi_config.fbzMode;
  if ( _grVpDrawTriangle ( a, b, c ) <= 0) {/* backfaced or zero area */
    goto done;
  }
  gc->state.fbi_config.fbzMode &= ~(SST_ZAWRMASK);
  /* gc->state.invalid |= fbzModeBIT; */
  GR_FLUSH_STATE();
  
  oowa = 1.0f / FARRAY(a, gc->state.vData.wInfo.offset);        
  oowb = 1.0f / FARRAY(b, gc->state.vData.wInfo.offset);        
  oowc = 1.0f / FARRAY(c, gc->state.vData.wInfo.offset);        
  
  
  if (ab_antialias)
    aaVpDrawArrayEdgeSense(a, b, c, oowa, oowb, oowc);
  
  if (bc_antialias)
    aaVpDrawArrayEdgeSense(b, c, a, oowb, oowc, oowa);
  
  if (ca_antialias)
    aaVpDrawArrayEdgeSense(c, a, b, oowc, oowa, oowb);
  
done:
  gc->state.fbi_config.fbzMode = fbzModeOld;
  gc->state.invalid |= fbzModeBIT;
  GR_FLUSH_STATE();

#undef FN_NAME
} /* _grAAVpDrawTriangles */

