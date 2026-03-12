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
 * 6     8/14/97 5:32p Pgj
 * remove dead code per GMT
 * 
 * 5     6/27/97 4:16p Atai
 * fixed guDrawTriangleWithClip bug (#484) by removing +1.0F and -1.0F in
 * intersectTop(), intersectBottom(), intersectLeft(), and
 * intersectRight() routine
 * 
 * 4     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
**
*/

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#define GU_PRIM_MAX_VERTICES 100

static void
calcParams(const GrVertex *a, const GrVertex *b, GrVertex *isect, float d)
{
  GR_DCL_GC;
  if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) {
    isect->r = a->r + d * ( b->r - a->r );
    isect->g = a->g + d * ( b->g - a->g );
    isect->b = a->b + d * ( b->b - a->b );
  }
    
  if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) {
    isect->a        = a->a        + d * ( b->a - a->a );
  }

  if (gc->state.paramIndex & STATE_REQUIRES_OOZ) {
    isect->ooz = a->ooz + d * ( b->ooz - a->ooz );
  }

  if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) {
    isect->oow = a->oow + d * ( b->oow - a->oow);
  }

  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) {
    isect->tmuvtx[0].oow =
      a->tmuvtx[0].oow + d * ( b->tmuvtx[0].oow - a->tmuvtx[0].oow );
    isect->tmuvtx[0].sow =
      a->tmuvtx[0].sow + d * ( b->tmuvtx[0].sow - a->tmuvtx[0].sow );
    isect->tmuvtx[0].tow =
      a->tmuvtx[0].tow + d * ( b->tmuvtx[0].tow - a->tmuvtx[0].tow );
  }

  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) {
    isect->tmuvtx[1].oow =
      a->tmuvtx[1].oow + d * ( b->tmuvtx[1].oow - a->tmuvtx[1].oow );
    isect->tmuvtx[1].sow =
      a->tmuvtx[1].sow + d * ( b->tmuvtx[1].sow - a->tmuvtx[1].sow );
    isect->tmuvtx[1].tow =
      a->tmuvtx[1].tow + d * ( b->tmuvtx[1].tow - a->tmuvtx[1].tow );
  }

#if (GLIDE_NUM_TMU > 2)
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU2) {
    isect->tmuvtx[2].oow =
      a->tmuvtx[2].oow + d * ( b->tmuvtx[2].oow - a->tmuvtx[2].oow );
    isect->tmuvtx[2].sow =
      a->tmuvtx[2].sow + d * ( b->tmuvtx[2].sow - a->tmuvtx[2].sow );
    isect->tmuvtx[2].tow =
      a->tmuvtx[2].tow + d * ( b->tmuvtx[2].tow - a->tmuvtx[2].tow );
  }
#endif

} /* calcParams */

static void 
intersectTop( const GrVertex *a, const GrVertex *b, GrVertex *intersect )
{
  GR_DCL_GC;
  float
    d = ( gc->state.clipwindowf_ymin - a->y ) / ( b->y - a->y );
  
  intersect->x        = a->x        + d * ( b->x - a->x );
  intersect->y        = gc->state.clipwindowf_ymin; 
  
  calcParams(a, b, intersect, d);

} /* intersectTop */

static void
intersectBottom( const GrVertex *a, const GrVertex *b, GrVertex *intersect )
{
  GR_DCL_GC;
  float
    d = ( gc->state.clipwindowf_ymax - a->y ) / ( b->y - a->y );
  
  intersect->x        = a->x        + d * ( b->x - a->x );
  intersect->y        = gc->state.clipwindowf_ymax; 

  calcParams(a, b, intersect, d);

} /* intersectBottom */

static void
intersectRight( const GrVertex *a, const GrVertex *b, GrVertex *intersect )
{
  GR_DCL_GC;
  float
    d = ( gc->state.clipwindowf_xmax - a->x ) / ( b->x - a->x );
  
  intersect->x        = gc->state.clipwindowf_xmax; 
  intersect->y        = a->y        + d * ( b->y - a->y );

  calcParams(a, b, intersect, d);

} /* intersectRight */

static void
intersectLeft( const GrVertex *a, const GrVertex *b, GrVertex *intersect )
{
  GR_DCL_GC;
  float
    d = ( gc->state.clipwindowf_xmin - a->x ) / ( b->x - a->x );
  
  intersect->x        = gc->state.clipwindowf_xmin;
  intersect->y        = a->y        + d * ( b->y - a->y );

  calcParams(a, b, intersect, d);

} /* intersectLeft */

static FxBool
aboveYMin(const GrVertex *p)
{
  GR_DCL_GC;
  return (( p->y > gc->state.clipwindowf_ymin ) ? FXTRUE : FXFALSE);
} /* aboveYMin */
     

static FxBool
belowYMax(const GrVertex *p)
{
  GR_DCL_GC;
  return (( p->y < gc->state.clipwindowf_ymax ) ? FXTRUE : FXFALSE);
} /* belowYMax */

static FxBool
aboveXMin(const GrVertex *p)
{
  GR_DCL_GC;
  return (( p->x > gc->state.clipwindowf_xmin ) ? FXTRUE : FXFALSE );
} /* aboveXMin */

static FxBool
belowXMax(const GrVertex *p)
{
  GR_DCL_GC;
  return (( p->x < gc->state.clipwindowf_xmax ) ? FXTRUE : FXFALSE );
} /* belowXMax */

/*
** shClipPolygon
*/
static void
shClipPolygon(
              const GrVertex invertexarray[],
              GrVertex outvertexarray[],
              int inlength, int *outlength,
              FxBool (*inside)(const GrVertex *p),
              void (*intersect)(
                                const GrVertex *a,
                                const GrVertex *b,
                                GrVertex *intersect )
              )
{
  GrVertex
    s, p /*, intersection */;
  int
    j;
  
  *outlength = 0;
  
  s = invertexarray[inlength-1];
  for ( j = 0; j < inlength; j++ ) {
    p = invertexarray[j];
    if ( inside( &p ) ) {
      if ( inside( &s ) ) {
        outvertexarray[*outlength] = p;
        (*outlength)++;
      }else {
#if 0
        intersect( &s, &p, &intersection );
        outvertexarray[*outlength] = intersection;
#else
        intersect( &s, &p, &outvertexarray[*outlength] );
#endif
        (*outlength)++;
        outvertexarray[*outlength] = p;
        (*outlength)++;
      }
    } else {
      if ( inside( &s ) ) {
#if 0
        intersect( &s, &p, &intersection );
        outvertexarray[*outlength] = intersection;
#else
        intersect( &s, &p, &outvertexarray[*outlength] );
#endif
        (*outlength)++;
      }
    }
    s = p;
  }
} /* shClipPolygon */

/*---------------------------------------------------------------------------
** guDrawTriangleWithClip
**
** NOTE:  This routine snaps vertices by adding a large number then
** subtracting that same number again.  In order for this to work
** you MUST set up the FPU to work in single precision mode.  Code
** to perform this is listed in the Appendix to the Glide Programmer's
** Guide.
*/
static const float vertex_snap_constant = ( float ) ( 1L << 19 );

GR_DIENTRY(guDrawTriangleWithClip, void,
           (const GrVertex *a, const GrVertex *b, const GrVertex *c ))
{
  GR_DCL_GC;
  GrVertex
    output_array[8],
    output_array2[8],
    input_array[3];
  int
    i,
    outlength;

  GDBG_INFO_MORE((99,"guDrawTriangleWithClip(0x%x,0x%x,0x%x)\n",a,b,c));

  /*
   ** perform trivial accept
   */
  if (
      ( a->x >= gc->state.clipwindowf_xmin) &&
      ( a->x < gc->state.clipwindowf_xmax ) &&
      ( a->y >= gc->state.clipwindowf_ymin ) &&
      ( a->y < gc->state.clipwindowf_ymax ) &&
      ( b->x >= gc->state.clipwindowf_xmin ) &&
      ( b->x < gc->state.clipwindowf_xmax ) &&
      ( b->y >= gc->state.clipwindowf_ymin ) &&
      ( b->y < gc->state.clipwindowf_ymax ) &&
      ( c->x >= gc->state.clipwindowf_xmin ) &&
      ( c->x < gc->state.clipwindowf_xmax ) &&
      ( c->y >= gc->state.clipwindowf_ymin ) &&
      ( c->y < gc->state.clipwindowf_ymax )
      )
  {
    grDrawTriangle( a, b, c );
    return;
  }

  /*
   ** go ahead and clip and render
   */
  input_array[0] = *a;
  input_array[1] = *b;
  input_array[2] = *c;
  
  shClipPolygon( input_array,   output_array,  3,         &outlength, belowXMax, intersectRight );
  shClipPolygon( output_array,  output_array2, outlength, &outlength, belowYMax, intersectBottom );
  shClipPolygon( output_array2, output_array,  outlength, &outlength, aboveXMin, intersectLeft );
  shClipPolygon( output_array,  output_array2, outlength, &outlength, aboveYMin, intersectTop );
  
  /*
   ** snap vertices then decompose the n-gon into triangles
   */
#if defined ( __WATCOMC__ ) || defined ( __MSC__ ) || \
  defined ( __DJGPP__ ) || defined (__GNUC__)
  for ( i = 0; i < outlength; i++ ) {
    output_array2[i].x += vertex_snap_constant;
    output_array2[i].x -= vertex_snap_constant;
    output_array2[i].y += vertex_snap_constant;
    output_array2[i].y -= vertex_snap_constant;
  }
#else
#  error VERTEX SNAPPING MUST BE IMPLEMENTED FOR THIS COMPILER
#endif
  grDrawPolygonVertexList( outlength, output_array2 );
} /* guDrawTriangleWithClip */

/*---------------------------------------------------------------------------
**  guAADrawTriangleWithClip
*/
GR_DIENTRY(guAADrawTriangleWithClip, void,
           (const GrVertex *a, const GrVertex *b, const GrVertex *c ))
{
  GR_DCL_GC;
  GrVertex
    output_array[8],
    output_array2[8],
    input_array[3];
  int
    i,
    ilist[10],
    outlength;

  GDBG_INFO_MORE((99,"guAADrawTriangleWithClip(0x%x,0x%x,0x%x)\n",a,b,c));

   /*
    ** perform trivial accept
    */
  if (
      ( a->x >= gc->state.clipwindowf_xmin) &&
      ( a->x < gc->state.clipwindowf_xmax ) &&
      ( a->y >= gc->state.clipwindowf_ymin ) &&
      ( a->y < gc->state.clipwindowf_ymax ) &&
      ( b->x >= gc->state.clipwindowf_xmin ) &&
      ( b->x < gc->state.clipwindowf_xmax ) &&
      ( b->y >= gc->state.clipwindowf_ymin ) &&
      ( b->y < gc->state.clipwindowf_ymax ) &&
      ( c->x >= gc->state.clipwindowf_xmin ) &&
      ( c->x < gc->state.clipwindowf_xmax ) &&
      ( c->y >= gc->state.clipwindowf_ymin ) &&
      ( c->y < gc->state.clipwindowf_ymax )
      )
  {
    grAADrawTriangle( a, b, c, FXTRUE, FXTRUE, FXTRUE );
    return;
  }

  /*
   ** go ahead and clip and render
   */
  input_array[0] = *a;
  input_array[1] = *b;
  input_array[2] = *c;
  
  shClipPolygon( input_array,   output_array,  3,         &outlength, belowXMax, intersectRight );
  shClipPolygon( output_array,  output_array2, outlength, &outlength, belowYMax, intersectBottom );
  shClipPolygon( output_array2, output_array,  outlength, &outlength, aboveXMin, intersectLeft );
  shClipPolygon( output_array,  output_array2, outlength, &outlength, aboveYMin, intersectTop );
  
  /*
   ** snap vertices then decompose the n-gon into triangles
   */
#if defined ( __WATCOMC__ ) || defined ( __MSC__ ) || \
  defined ( __DJGPP__ ) || defined ( __GNUC__ )
  for ( i = 0; i < outlength; i++ ) {
    output_array2[i].x += vertex_snap_constant;
    output_array2[i].x -= vertex_snap_constant;
    output_array2[i].y += vertex_snap_constant;
    output_array2[i].y -= vertex_snap_constant;
    ilist[i] = i;
  }

  ilist[outlength] = 0;
#else
#  error VERTEX SNAPPING MUST BE IMPLEMENTED FOR THIS COMPILER
#endif
  grAADrawPolygon( outlength, ilist, output_array2 );
} /* guAADrawTriangleWithClip */


/*---------------------------------------------------------------------------
**  guDrawPolygonVertexListWithClip
*/
GR_DIENTRY(guDrawPolygonVertexListWithClip, void,
           ( int nverts, const GrVertex vlist[] ))
{
  GrVertex
    output_array[GU_PRIM_MAX_VERTICES+8],
    output_array2[GU_PRIM_MAX_VERTICES+8];
   int
     i,
     outlength;

  GDBG_INFO_MORE((99,"guDrawPolygonVertexListWithClip(%d,0x%x)\n",nverts,vlist));
  /*
   ** go ahead and clip and render
   */
  shClipPolygon( vlist, output_array,  nverts, &outlength, belowXMax, intersectRight );
  shClipPolygon( output_array,  output_array2, outlength, &outlength, belowYMax, intersectBottom );
  shClipPolygon( output_array2, output_array,  outlength, &outlength, aboveXMin, intersectLeft );
  shClipPolygon( output_array,  output_array2, outlength, &outlength, aboveYMin, intersectTop );

  /*
   ** snap vertices then decompose the n-gon into triangles
   */
#if defined ( __WATCOMC__ ) || defined ( __MSC__ ) || \
  defined ( __DJGPP__ ) || defined ( __GNUC__ )
  for ( i = 0; i < outlength; i++ ) {
    output_array2[i].x += vertex_snap_constant;
    output_array2[i].x -= vertex_snap_constant;
    output_array2[i].y += vertex_snap_constant;
    output_array2[i].y -= vertex_snap_constant;
  }
#else
#  error VERTEX SNAPPING MUST BE IMPLEMENTED FOR THIS COMPILER
#endif
  grDrawPolygonVertexList( outlength, output_array2 );
} /* guDrawPolygonVertexListWithClip */
