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

#define FARRAY(p,i) (*(float *)((i)+(int)(p)))

/*
** -----------------------------------------------------------------------
**
** Antialiasing rendering functions
**
** -----------------------------------------------------------------------
*/

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
**
** pci write command
** 1st triangle EDA
**     8 + 3 * n
** 2nd triangle BEC
**     8
** 3rd triangle AED
**     5
** 4th triangle ABE
**     8
** total = 29 + 3 * n
*/
GR_ENTRY(grAADrawPoint, void, ( const GrVertex *e )) 
{
  int i, j;
  float *fp;
  struct dataList_s *dlp;
  float dp;
  float PX, PY;

  GR_BEGIN("grAADrawPoint",94,((_GlideRoot.curTriSize - _GlideRoot.curTriSizeNoGradient) >> 3) * 12 + 116);

  PX = e->x;
  PY = e->y;
  j = *(long *)&e->a;
  j ^= 0x80000000;

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
      dp = FARRAY(e,i);
      GR_SETF( fp[0], dp );
      GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
      dlp++;
      i = dlp->i;
      GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
    }
  }
  GR_SET(hw->Fdady, j);
  P6FENCE_CMD( GR_SET(hw->triangleCMD, 0xffffffff) );

  /* 2nd triangle BEC */
  dp = PX + _GlideRoot.pool.f1;
  GR_SETF(hw->FvA.x, dp);
  GR_SETF(hw->FvB.x, PX);
  dp = PY - _GlideRoot.pool.f1;
  GR_SETF(hw->FvA.y, dp);
  GR_SETF(hw->FvB.y, PY);

  GR_SET(hw->Fa, 0);
  GR_SET(hw->Fdadx, j);
  GR_SET(hw->Fdady, 0);
  P6FENCE_CMD( GR_SET(hw->triangleCMD, 0xffffffff) );

  /* 3rd triangle AED */
  dp = PX - _GlideRoot.pool.f1;
  GR_SETF(hw->FvA.x, dp);
  GR_SETF(hw->FvC.x, dp);

  j ^= 0x80000000;
  GR_SET(hw->Fa, 0);
  GR_SET(hw->Fdadx, j);
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
  GR_SET(hw->Fdady, j);
  P6FENCE_CMD( GR_SET(hw->triangleCMD, 1) );

  GR_END();
} /* grAADrawPoint */

static void grSetVertexParameter(const GrVertex *v1)
{
  GR_DCL_GC;
  struct dataList_s *dlp;
  int i;
  float *fp, dp;

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
      dp = FARRAY(v1,i);
      GR_SETF( fp[0], dp );
      dlp++;
      i = dlp->i;
    }
  }
}

/*---------------------------------------------------------------------------
** grAADrawLine
**  A(x1,y1-1.0)+
**              |  \
**              | \   \
**              |    \   \
**     V1(x1,y1)*           \
**              |  *     \     \
**              |  \  *           \
**              |             \      \
**  E(x1,y1+1.0)+       \  *      \     \
**                 \          *            +B(x2,y2-1.0)
**                    \      \    *   \    |
**                       \                 |
**                          \     \    *  \|
**                             \           +V2(x2,y2)
**                                \   \    |
**                                   \     |
**                                      \ \|
**                                         +F(x2,y2+1.0)
**                                         
** pci write command (x major line )
** 1st triangle A-B-V2
**     10 + 3 * n
** 2nd triangle A-V1-V2
**     6 + n
** 3rd triangle V1-E-F
**     6 + n
** 4th triangle V1-V2-F
**     5 + n
** total = 
*/
GR_ENTRY(grAADrawLine, void, ( const GrVertex *v1, const GrVertex *v2 )) 
{
  float           adx;         /* |dX| */
  const GrVertex *tv;
  struct dataList_s *dlp;
  int i;
  float *fp, dp;
  float tmp1, tmp2;
  float dx, dy;           /* delta X and Y */

  GR_BEGIN("grAADrawLine",95, ((_GlideRoot.curTriSize - _GlideRoot.curTriSizeNoGradient) >> 3) * 24 + 108);

  GDBG_INFO((95,"grAADrawLine(0x%x,0x%x)\n",v1,v2));

  /* turn off culling so triangles unconditionally draw */

  /* draw from low Y to high Y */
  if ( v2->y < v1->y ) {
    tv = v1; v1 = v2; v2 = tv;
  }
  
  /* compute deltas and absolute deltas */
  dx = adx = v1->x - v2->x;
  dy = v2->y - v1->y;
  if ( adx < 0 )
    adx = -adx;
  
  if ( adx >= dy ) {           /* X major line */

    if (dx == 0.0f)
      return;

    /* 1st triangle A-B-V2*/

    GR_SETF( hw->FvA.x, v1->x);
    dx = _GlideRoot.pool.f1 / dx;
    GR_SETF( hw->FvA.y, v1->y - _GlideRoot.pool.f1);
    tmp1 = -v1->a * dy;
    GR_SETF( hw->FvB.x, v2->x);
    GR_SETF( hw->FvB.y, v2->y - _GlideRoot.pool.f1);
    tmp2 = -v2->a * dy;
    GR_SETF( hw->FvC.x, v2->x);
    dlp = gc->dataList;
    GR_SETF( hw->FvC.y, v2->y);
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
        dp = FARRAY(v1,i);
	GR_SETF( fp[0], dp );
	dp -= FARRAY(v2, i);
	GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
	dlp++;
	i = dlp->i;
	GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
      }
    }
    GR_SET( hw->Fa , 0 );
    dp = - tmp2 * dx;
    GR_SETF( hw->Fdadx , dp );
    GR_SETF( hw->Fdady , v2->a );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dx ) );

    /* 2nd triangle A-V1-V2*/
    GR_SETF( hw->FvB.x, v1->x);
    grSetVertexParameter(v1);
    GR_SET( hw->Fa , 0 );
    dp = v1->a - v2->a;
    GR_SETF( hw->FvB.y, v1->y);
    dp -= tmp1;
    GR_SETF( hw->Fdady , v1->a );
    dp *= dx;
    GR_SETF( hw->Fdadx , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dx ) );

    /* 3rd triangle v1-E-F */
    GR_SETF( hw->FvA.y, v1->y);
    GR_SETF( hw->FvB.y, v1->y + _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.y, v2->y + _GlideRoot.pool.f1);

    grSetVertexParameter(v1);

    GR_SETF( hw->Fdady , -v1->a );
    dp = tmp1 * dx;
    GR_SETF( hw->Fdadx , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dx ) );

    /* 4th triangle V1-V2-F*/
    GR_SETF( hw->FvB.x, v2->x);
    dp = v1->a - v2->a + tmp2;
    GR_SETF( hw->FvB.y, v2->y);
    dp *= dx;

    grSetVertexParameter(v1);

    GR_SETF( hw->Fdadx , dp ); 
    GR_SETF( hw->Fdady , -v2->a );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dx ) );

  } else {                      /* Y major line */

    if (dy == 0.0f)
      return;

    /* 1st triangle v1-v2-b */
    GR_SETF( hw->FvA.x, v1->x);
    tmp1 = v1->a * dx;
    GR_SETF( hw->FvA.y, v1->y);
    tmp2 = v2->a * dx;
    GR_SETF( hw->FvB.x, v2->x);
    dy = - 1.0f / dy;
    GR_SETF( hw->FvB.y, v2->y);
    GR_SETF( hw->FvC.x, v2->x - _GlideRoot.pool.f1);
    dlp = gc->dataList;
    GR_SETF( hw->FvC.y, v2->y);
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
	dp = FARRAY(v1,i);
	GR_SETF( fp[0], dp );
        dp -= FARRAY(v2, i);
	GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
	dlp++;
	i = dlp->i;
        GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
      }
    }
    dp = (v1->a - tmp2 - v2->a);
    GR_SETF( hw->Fdadx , v2->a );
    dp *= dy;
    GR_SETF( hw->Fdady , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dy ) );

    /* 2nd triangle v1-a-b */

    GR_SETF( hw->FvB.x, v1->x - _GlideRoot.pool.f1);
    GR_SETF( hw->FvB.y, v1->y);
    grSetVertexParameter(v1);
    GR_SETF( hw->Fdadx , v1->a );
    dp = - tmp1 * dy;
    GR_SETF( hw->Fdady , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dy ) );

    /* 3rd triangle e-v1-v2 */
    GR_SETF( hw->FvA.x, v1->x + _GlideRoot.pool.f1);
    dp = (v1->a - v2->a + tmp1);
    GR_SETF( hw->FvB.x, v1->x);
    dp *= dy;
    GR_SETF( hw->FvC.x, v2->x);

    grSetVertexParameter(v1);
    GR_SET( hw->Fa , 0 );
    GR_SETF( hw->Fdadx , -v1->a );
    GR_SETF( hw->Fdady , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dy ) );

    /* 4th triangle e-f-v2 */
    GR_SETF( hw->FvB.x, v2->x + _GlideRoot.pool.f1);
    GR_SETF( hw->FvB.y, v2->y);

    grSetVertexParameter(v1);
    GR_SET( hw->Fa , 0 );

    GR_SETF( hw->Fdadx , -v2->a );
    dp = tmp2 * dy;
    GR_SETF( hw->Fdady , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dy ) );

  }
  GR_END();
} /* grAADrawLine */

typedef enum {
  aaEdgeSenseTop,
  aaEdgeSenseLeft,
  aaEdgeSenseBottom,
  aaEdgeSenseRight
} aaEdgeSense;

/*
**  grAADrawTriEdgeSense
**
**  This routine combines aaCalcEdgeSense(), grAADrawTriEdge(), and
**  grAADrawEdgeTriangle(). The goal is to
**  1. reduce the number of divide from 3 to 1
**  2. minimize pci write command.
*/
static void
grAADrawTriEdgeSense(const GrVertex *a,const GrVertex *b,const GrVertex *c)
{
  const GrVertex *tv;
  float dx, dy, m, dp;
  aaEdgeSense sense;

  GR_BEGIN_NOFIFOCHECK("grAADrawTriEdgeSense",gc->myLevel);


  if (a->y >= b->y) {
    tv = a; a = b; b = tv;
  }
  dx = a->x - b->x;
  dy = a->y - b->y;

  if (dx == 0.f) {
    if (c->x >  a->x)
      sense = aaEdgeSenseLeft;
    else
      sense =  aaEdgeSenseRight;
  }
  else if (dy == 0.0f) {
    if (c->y > a->y)
      sense = aaEdgeSenseBottom;
    else
      sense = aaEdgeSenseTop;
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
	sense = (cp > 0) ? aaEdgeSenseTop : aaEdgeSenseBottom;
      else              /* Y-major line */
	sense = (cp > 0) ? aaEdgeSenseRight : aaEdgeSenseLeft;
    }
    else {
      if (dx <= dy)     /* X-major line */
	sense = (cp < 0) ? aaEdgeSenseTop : aaEdgeSenseBottom;
      else              /* Y-major line */
	sense = (cp < 0) ? aaEdgeSenseLeft : aaEdgeSenseRight;
    }
  }

  switch (sense) {
  case aaEdgeSenseTop:
    /* pci comamnd 14 + 2 * n */
    GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSizeNoGradient << 1);
    GR_SETF( hw->FvA.x, a->x);
    GR_SETF( hw->FvA.y, a->y);
    GR_SETF( hw->FvB.x, a->x);
    GR_SETF( hw->FvC.x, b->x);
    m = 1.0f / dx;
    GR_SETF( hw->FvB.y, a->y + _GlideRoot.pool.f1);
    dp = a->a * dy;
    GR_SETF( hw->FvC.y, b->y + _GlideRoot.pool.f1);
    
    grSetVertexParameter(a);
    
    GR_SETF( hw->Fdady, -a->a);
    dp *= m;
    GR_SETF( hw->Fdadx, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
      
    dp = b->a * dy;
    GR_SETF( hw->FvB.x, b->x);
    dp = a->a - b->a + dp;
    GR_SETF( hw->FvB.y, b->y);

    grSetVertexParameter(a);
    
    GR_SETF( hw->Fdady, -b->a);
    dp *= m;
    GR_SETF( hw->Fdadx, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    
    break;
    
  case aaEdgeSenseLeft:
    /* pci comamnd 16 + 2 * n */
    GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
    GR_SETF( hw->FvA.y, a->y);
    GR_SETF( hw->FvB.x, a->x);
    GR_SETF( hw->FvB.y, a->y);
    GR_SETF( hw->FvC.y, b->y);    
    m = 1.0f / dy;
    GR_SETF( hw->FvA.x, a->x - _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.x, b->x - _GlideRoot.pool.f1);

    grSetVertexParameter(a);
    GR_SET(hw->Fa, 0);

    GR_SETF( hw->Fdadx, a->a);
    dp = - a->a * dx * m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dy ) );
    
    GR_SETF( hw->FvA.x, a->x);
    dp = b->a * dx;
    GR_SETF( hw->FvB.x, b->x);
    dp = a->a - dp - b->a;
    GR_SETF( hw->FvB.y, b->y);

    grSetVertexParameter(a);

    GR_SETF( hw->Fdadx, b->a);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dy ) );
    
    break;
    
  case aaEdgeSenseBottom:
    
    if (dy < -1.0f) {
      /* pci comamnd 17 + 2 * n */
      GR_SET_EXPECTED_SIZE(12+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, a->x);
      GR_SETF( hw->FvB.x, a->x);
      GR_SETF( hw->FvB.y, a->y);
      GR_SETF( hw->FvC.x, b->x);      
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, a->y - _GlideRoot.pool.f1);
      GR_SETF( hw->FvC.y, b->y - _GlideRoot.pool.f1);

      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);
      
      dp = a->a * dy;
      dp *= m;
      GR_SETF( hw->Fdadx, -dp);
      GR_SETF( hw->Fdady, a->a);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    
      GR_SETF( hw->FvA.y, a->y);
      dp = b->a * dy;
      GR_SETF( hw->FvB.x, b->x);
      GR_SETF( hw->FvB.y, b->y - _GlideRoot.pool.f1);
      dp = a->a - b->a - dp;
      GR_SETF( hw->FvC.y, b->y);
      grSetVertexParameter(a);
      
      GR_SETF( hw->Fdady, b->a);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    }
    else if (dy == 0.0f) {

      /* pci comamnd 16 + 2 * n */
      GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, a->x);
      GR_SETF( hw->FvC.x, b->x);      
      GR_SETF( hw->FvC.y, b->y);
      GR_SETF( hw->FvB.x, b->x);
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, a->y - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.y, b->y - _GlideRoot.pool.f1);

      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);

      GR_SET( hw->Fdadx, 0);
      GR_SETF( hw->Fdady, b->a);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    
      GR_SETF( hw->FvB.x, a->x);
      GR_SETF( hw->FvB.y, a->y);
      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);
    
      dp = a->a - b->a;
      GR_SETF( hw->Fdady, a->a);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    }
    else {

      /* pci comamnd 20 + 2 * n */
      GR_SET_EXPECTED_SIZE(24+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, a->x);
      GR_SETF( hw->FvB.x, b->x);
      GR_SETF( hw->FvC.x, a->x);      
      GR_SETF( hw->FvC.y, a->y);
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, a->y - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.y, b->y - _GlideRoot.pool.f1);

      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);
      
      dp = -a->a * dy * m;
      GR_SETF( hw->Fdadx, dp);
      GR_SETF( hw->Fdady, a->a);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );

      GR_SETF( hw->FvA.x, b->x);
      GR_SETF( hw->FvA.y, b->y - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.x, a->x);
      GR_SETF( hw->FvB.y, a->y);
      dp = b->a * dy;
      GR_SETF( hw->FvC.x, b->x);      
      GR_SETF( hw->FvC.y, b->y);

      grSetVertexParameter(b);
      GR_SET(hw->Fa, 0);
      dp = a->a - dp - b->a;
      GR_SETF( hw->Fdady, b->a);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    }
    
    break;
  case aaEdgeSenseRight:
    /* pci comamnd 16 + 2 * n */
    GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
    GR_SETF( hw->FvA.y, a->y);
    GR_SETF( hw->FvB.x, a->x);
    GR_SETF( hw->FvB.y, a->y);
    GR_SETF( hw->FvC.y, b->y);
    m = 1.0f / dy;
    GR_SETF( hw->FvA.x, a->x + _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.x, b->x + _GlideRoot.pool.f1);

    grSetVertexParameter(a);
    GR_SET(hw->Fa, 0);

    dp = a->a * dx;
    GR_SETF( hw->Fdadx, -a->a);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dy ) );
    
    GR_SETF( hw->FvA.x, a->x);
    dp = b->a * dx;
    GR_SETF( hw->FvB.x, b->x);
    dp = dp + a->a - b->a;
    GR_SETF( hw->FvB.y, b->y);
    grSetVertexParameter(a);
    
    GR_SETF( hw->Fdadx, -b->a);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dy ) );   
    break;
  }
  GR_END_SLOPPY();
} /* grAADrawTriEdge */

#if 0
/*
** aaCalculateSignOfSinTheta
**  
*/
float
aaCalculateSignOfSinTheta(GrVertex *a, GrVertex *b, GrVertex *c)
{
  float
    x1, y1, x2, y2;

  /*
     Now, we make two vectors: ab [x1 y1] and ac [x2 y2]
     Fromt the cross product, we know that
     
     x1*y2 - x2*y1 = |ab|*|ac|*sin(theta), where theta is the angle
     between the two lines.  If sin(theta) > 0, c is above the line ab,
     and if it's negative, it's below ab.
     */
  x1 = b->x - a->x;
  y1 = b->y - a->y;
  
  x2 = c->x - a->x;
  y2 = c->y - a->y;

  return ((x1 * y2) - (x2 * y1));
  
} /* aaCalculateSignOfSinTheta */
#endif


/*---------------------------------------------------------------------------
** grAADrawTriangle
**
**  NOTE:  This doesn't quite work yet
*/
GR_ENTRY(grAADrawTriangle, void, (const GrVertex *a, const GrVertex *b, const GrVertex *c, FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias ) ) 
{
  FxU32
    fbzMode,                    /* What we write to fbzMode */
    fbzModeOld;                 /* Squirrel away current fbzMode */

  GR_BEGIN_NOFIFOCHECK("grAADrawTriangle",96);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x,0x%x,0x%x,%d,%d,%d)\n",
                  a,b,c,ab_antialias,bc_antialias,ca_antialias));

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

  if ( ab_antialias ) {
    grAADrawTriEdgeSense(a, b, c);
  }
  
  if ( bc_antialias ) {
    grAADrawTriEdgeSense(b, c, a);
  }
  
  if ( ca_antialias ) {
    grAADrawTriEdgeSense(c, a, b);
  }

  /* Restore the old fbzMode */
  GR_SET_EXPECTED_SIZE(4);
  GR_SET( hw->fbzMode, fbzModeOld );
  GR_END();
} /* grAADrawTriangle */

/*---------------------------------------------------------------------------
**  grAADrawPolygon
**  NOTE:  This routine does not do backface culling!!!
*/

GR_ENTRY(grAADrawPolygon, void, (const int nverts, const int ilist[], const GrVertex vlist[])) 
{
  int
    i = 1;                      /* index into ilist */
  const GrVertex
    *a, *b, *c;                 /* What we end up sending */

  GR_BEGIN_NOFIFOCHECK("grAADrawPolygon",97);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x,0x%x)\n",nverts,ilist,vlist));

  for (i=1; i < nverts - 1; i++) {
    a = &vlist[ilist[0]];
    b = &vlist[ilist[i]];
    c = &vlist[ilist[i+1]];
    if ( TRISETUP ( a, b, c ) <= 0 ) /* backfaced or zero area */
      continue;

    /*
    **  Always draw the BC edge.
    **  On the first triangle, additionally draw the AB edge.
    **  On the last triangle, draw the CA edge.
    */
    if (i == 1) {
      grAADrawTriEdgeSense(a, b, c);

    } else if (i == (nverts - 2)) {
      grAADrawTriEdgeSense(c, a, b);
    } 

    grAADrawTriEdgeSense(b, c, a);

  }
  GR_END();
} /* grAADrawPolygon */

/*---------------------------------------------------------------------------
**  grAADrawPolygonVertexList
**  NOTE:  This routine does not do backface culling!!!
*/

GR_ENTRY(grAADrawPolygonVertexList, void, (const int nverts, const GrVertex vlist[])) 
{
  int
    i = 1;                      /* index into ilist */
  const GrVertex
    *a, *b, *c;                 /* What we end up sending */
  FxI32
    res;

  GR_BEGIN_NOFIFOCHECK("grAADrawPolygonVertexList",97);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x)\n",nverts,vlist));

  for (i=1; i < nverts - 1; i++) {
    a = &vlist[0];
    b = &vlist[i];
    c = &vlist[i+1];

    res = TRISETUP ( a, b, c );
    if (res <= 0)               /* backfaced or zero area */
      continue;

    /*
    **  Always draw the BC edge.
    **  On the first triangle, additionally draw the AB edge.
    **  On the last triangle, draw the CA edge.
    */
    if (i == 1) {
      grAADrawTriEdgeSense(a, b, c);
    } else if (i == (nverts - 2)) {
      grAADrawTriEdgeSense(c, a, b);
    } 

    grAADrawTriEdgeSense(b, c, a);

  }
  GR_END();
} /* grAADrawPolygonVertexList */



