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
** Revision 1.1.2.5  2006/01/16 21:22:45  jwrdegoede
** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
**
** Revision 1.1.2.4  2005/06/09 18:32:35  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.2.3  2005/05/25 08:51:52  jwrdegoede
** Add #ifdef GL_X86 around x86 specific code
**
** Revision 1.1.2.2  2005/05/10 11:27:23  jwrdegoede
** sst1 gcc4 compile fixes
**
** Revision 1.1.2.1  2004/10/04 09:35:32  dborca
** second cut at Glide3x for Voodoo1/Rush (massive update):
** delayed validation, vertex snapping, clip coordinates, strip/fan_continue, bugfixes.
** and probably a bunch of other crap that I forgot
**
** 
** 10    8/21/98 1:26p Atai
** fixed GR_TRIANGLES in windows coords
** 
** 9     6/18/98 5:40p Atai
** fixed clip coordinate bug
** 
** 8     6/17/98 6:12p Atai
** added GR_TRIANGLE_STRIP_CONTINUE and GR_TRIANGLE_FAN_CONTINE
** 
** 7     4/21/98 1:39p Atai
** make 32 bit clean
** 
** 6     3/17/98 2:53p Atai
** packed color for windows coordinates
** 
** 5     3/13/98 5:43p Atai
** added non-packed color clip space code (c version)
** 
** 4     2/27/98 5:56p Atai
** fix aa routine. flush state in triangle asm code
** 
** 3     2/26/98 8:09p Atai
** fixed state and draw routines
** 
** 2     2/25/98 7:08p Atai
** fixed compile error
** 
** 1     2/24/98 6:40p Atai
** Glide 3 for VG/VR
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
 * Fixed state Fuckage in Glide3 parameter data
 * 
 * 2     9/23/97 2:35p Dow
 * One less loop
 * 
 * 1     9/23/97 2:04p Dow
 * DD code for strips
**
*/

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"

/* access a floating point array with a byte index */
#define FARRAY(p,i) (*(float *)((i)+(int)(p)))
/* access a byte array with a byte index and convert to float */
#define FbARRAY(p,i) (float)(((unsigned char *)p)[i])

void FX_CSTYLE
_grStoreVertex(float *src, float *dst);

/*-------------------------------------------------------------------
  Function: _grStoreVertex
  Date: 12-Jun-98
  Implementor(s): atai
  Description:
        store vertex in the buffer
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grStoreVertex(float *src, float *dst)
{
#define FN_NAME "_grStoreVertex"
  GR_DCL_GC;
  struct dataList_s *dlp = gc->dataList;
  FxI32 i, ii = 8;

  gc->w_index = 0;
  *dst++ = FARRAY(src, 0);

  *dst++ = FARRAY(src, 4);
  dlp = gc->dataList;
  i = dlp->i;
  while (i) {
    if (i == gc->state.vData.wInfo.offset)
      gc->w_index = ii;
    *dst++ = FARRAY(src, i);
    dlp++;
    i = dlp->i;
    ii+=4;
  }

#undef FN_NAME
} /* _grStoreVertex */

/*-------------------------------------------------------------------
  Function: _grDrawVertexList
  Date: 25-Feb-98
  Implementor(s): atai
  Description:
        Sends a triangle strip to VG.
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grDrawVertexList(FxU32 type, FxI32 mode, FxI32 count, void *pointers)
{
#define FN_NAME "_grDrawVertexList"
  FxI32 stride = mode;
  FxI32 sCount;
  void *v1 = NULL, *v2 = NULL;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE((gc->myLevel, "(type = 0x%x, count = %d, pointers = 0x%x)\n",
                 type, count, pointers));

  gc->vert2_status = FXFALSE;
  gc->w_index = 0;

  GR_FLUSH_STATE();
  if (stride == 0)
    stride = gc->state.vData.vStride;
  sCount = count - 2;
  if (type == kSetupFan) {
    float *va, *vb = NULL, *vc = NULL;
    v1 = va = (mode == 0) ? pointers : *(float **)pointers;
    while (sCount > 0) {
      pointers = (float *)pointers + stride;
      if (mode) {
        vb = *(float **)pointers;
        vc = *((float **)pointers+1);
      }
      else {
        vb = pointers;
        vc = (float *)pointers+stride;
      }
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        grDrawTriangle(va, vb, vc);
      else
        _grVpDrawTriangle(va, vb, vc);
      sCount--;
    }
    if (count > 2)
      v2 = vc;
    else if (count == 2)
      v2 = (mode == 0) ? (float *)pointers+stride : *((float **)pointers+1);
  }
  else {
    FxBool flip = FXFALSE;
    float *va, *vb, *vc;
    while (sCount > 0) {
      if (flip) {
        if (mode) {
          va = *((float **)pointers+1);
          vb = *(float **)pointers;
          vc = *((float **)pointers+2);
        } else {
          va = (float *)pointers+stride;
          vb = pointers;
          vc = (float *)pointers+(stride<<1);
        }
        v1 = va;
        v2 = vc;
      }
      else { 
        if (mode) {
          va = *(float **)pointers;
          vb = *((float **)pointers+1);
          vc = *((float **)pointers+2);
        } else {
          va = pointers;
          vb = (float *)pointers+stride;
          vc = (float *)pointers+(stride<<1);
        }
        v1 = vb;
        v2 = vc;
      }
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        grDrawTriangle(va, vb, vc);
      else
        _grVpDrawTriangle(va, vb, vc);
      pointers = (float *)pointers + stride;
      flip = ~flip;
      sCount--;
    }
    gc->flip = flip;
    if (count == 2) {
      v1 = (mode == 0) ? pointers : *(float **)pointers;
      v2 = (mode == 0) ? (float *)pointers + stride : *((float **)pointers + 1);
    }
    else if (count == 1) {
      v1 = (mode == 0) ? pointers : *(float **)pointers;
    }
  }

  if (v1)
    _grStoreVertex(v1, &gc->vert1.x);
  if (v2) {
    _grStoreVertex(v2, &gc->vert2.x);
    gc->vert2_status = FXTRUE;
  }

#undef FN_NAME
} /* _grDrawVertexList */

/*-------------------------------------------------------------------
  Function: _grDrawVertexListContinue
  Date: 12-Jun-98
  Implementor(s): atai
  Description:
        Sends a triangle strip to VG.
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
void FX_CSTYLE
_grDrawVertexListContinue(FxU32 type, FxI32 mode, FxI32 count, void *pointers)
{
#define FN_NAME "_grDrawVertexListContinue"
  FxI32 stride = mode;
  void *v1 = NULL, *v2 = NULL;
  FxI32 (FX_CALL *triproc)(const void *va, const void *vb, const void *vc, FxBool aflag, FxBool bflag, FxBool cflag);

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE((gc->myLevel, "(type = 0x%x, count = %d, pointers = 0x%x)\n",
                 type, count, pointers));
  triproc = (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) 
    ? _trisetup_mixed_datalist : _vp_trisetup_mixed_datalist;

  if (stride == 0)
    stride = gc->state.vData.vStride;

  if (type == kSetupFan) {
    float *va, *vb, *vc;
    /*
    ** if grDrawVertexArray only has one vertex
    ** we filled out the second one in the vertex buffer
    */
    if (!gc->vert2_status) {
      float *v = (mode == 0) ? pointers : *(float **)pointers;
      _grStoreVertex(v, &gc->vert2.x);
      gc->vert2_status = FXTRUE;
      pointers = (float *)pointers + stride;
      count--;
    }
    if (count == 0) return;
    va = &gc->vert1.x; vb = &gc->vert2.x; vc = (mode == 0) ? pointers : *(float **)pointers;
    _trisetup_mixed_datalist(va, vb, vc, FXTRUE, FXTRUE, FXFALSE);
    count--;
    if (count == 0) {
      _grStoreVertex(vc, &gc->vert2.x);
      return;
    }    
    while (count--) {
      if (mode) {
        vb = *(float **)pointers;
        vc = *((float **)pointers+1);
      }
      else {
        vb = pointers;
        vc = (float *)pointers+stride;
      }
      triproc(va, vb, vc, FXTRUE, FXFALSE, FXFALSE);
      pointers = (float *)pointers + stride;
    }
    _grStoreVertex(vc, &gc->vert2.x);
  }
  else {
    FxBool flip = gc->flip;
    float *va, *vb, *vc;
    /*
    ** if grDrawVertexArray only has one vertex
    ** we filled out the second one in the vertex buffer
    */
    if (!gc->vert2_status) {
      float *v = (mode == 0) ? pointers : *(float **)pointers;
      _grStoreVertex(v, &gc->vert2.x);
      pointers = (float *)pointers + stride;
      count--;
      gc->vert2_status = FXTRUE;
    }
    if (count == 0) return;
    /*
    ** start to draw the first vertex
    */
    if (flip) {
      if (mode) {
        va = &gc->vert2.x;                         /* b */
        vb = &gc->vert1.x;                         /* a */
        vc = *((float **)pointers);                /* c */
      } else {
        va = &gc->vert2.x;                         /* b */
        vb = &gc->vert1.x;                         /* a */
        vc = (float *)pointers;                    /* c */
      }
    }
    else {
      if (mode) {
        va = &gc->vert1.x;                         /* a */
        vb = &gc->vert2.x;                         /* b */
        vc = *((float **)pointers);                /* c */
      } else {
        va = &gc->vert1.x;                         /* a */
        vb = &gc->vert2.x;                         /* b */
        vc = (float *)pointers;                    /* c */
      }
    }
    flip = ~flip;
    count--;
    triproc(va, vb, vc, FXTRUE, FXTRUE, FXFALSE);
    if (count == 0) {
      gc->vert1 = gc->vert2;
      _grStoreVertex(vc, &gc->vert2.x);
      gc->flip = flip;
      return;
    }
    if (flip) {
      if (mode) {
        va = *((float **)pointers);                /* b */
        vb = &gc->vert2.x;                         /* a */
        vc = *((float **)pointers + 1);            /* c */
      } else {
        va = (float *) pointers;                   /* b */
        vb = &gc->vert2.x;                         /* a */
        vc = (float *) pointers + stride;          /* c */
      }
      v1 = va;
      v2 = vc;
      triproc(va, vb, vc, FXFALSE, FXTRUE, FXFALSE);
    }
    else {
      if (mode) {
        va = &gc->vert2.x;                         /* a */
        vb = *((float **)pointers);                /* b */
        vc = *((float **)pointers + 1);            /* c */
      } else {
        va = &gc->vert2.x;                         /* a */
        vb = (float *) pointers;                   /* b */
        vc = (float *) pointers + stride;          /* c */
      }
      v1 = vb;
      v2 = vc;
      triproc(va, vb, vc, FXTRUE, FXFALSE, FXFALSE);
    }
    flip = ~flip;
    count--;
    if (count == 0) {
      _grStoreVertex(v1, &gc->vert1.x);
      _grStoreVertex(v2, &gc->vert2.x);
      gc->flip = flip;
      return;
    }
    while (count--) {
      if (flip) {
        if (mode) {
          va = *((float **)pointers+1);
          vb = *(float **)pointers;
          vc = *((float **)pointers+2);
        } else {
          va = (float *)pointers+stride;
          vb = pointers;
          vc = (float *)pointers+(stride<<1);
        }
        v1 = va;
        v2 = vc;
      }
      else { 
        if (mode) {
          va = *(float **)pointers;
          vb = *((float **)pointers+1);
          vc = *((float **)pointers+2);
        } else {
          va = pointers;
          vb = (float *)pointers+stride;
          vc = (float *)pointers+(stride<<1);
        }
        v1 = vb;
        v2 = vc;
      }
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        grDrawTriangle(va, vb, vc);
      else
        _grVpDrawTriangle(va, vb, vc);
      pointers = (float *)pointers + stride;
      flip = ~flip;
    }
    _grStoreVertex(v1, &gc->vert1.x);
    _grStoreVertex(v2, &gc->vert2.x);
  }

#undef FN_NAME
} /* _grDrawVertexList */

/*-------------------------------------------------------------------
  Function: _grAADrawVertexList
  Date: 25-Feb-98
  Implementor(s): atai
  Description:
  Sends an aa triangle strip/fan to VG
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
  FxI32 stride = mode;
#if 0
  FxU32 fbzModeOld;                 /* Squirrel away current fbzMode */
#endif

  if (sCount <= 2) return;

#if 0
  _grDrawVertexList(type, mode, count, pointers);

  fbzModeOld = gc->state.fbi_config.fbzMode;
  gc->state.fbi_config.fbzMode &= ~(SST_ZAWRMASK);
  gc->state.invalid |= fbzModeBIT;
  GR_FLUSH_STATE();
#endif

  if (stride == 0)
    stride = gc->state.vData.vStride;

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
        grAADrawTriangle((const float *)v[0], (const float *)v[1], (const float *)v[2], FXTRUE, FXTRUE, FXTRUE);
      else
        _grAAVpDrawTriangle((const float *)v[0], (const float *)v[1], (const float *)v[2], FXTRUE, FXTRUE, FXTRUE);
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
        grAADrawTriangle((const float *)v[0], (const float *)v[1], (const float *)v[2], FXTRUE, FXTRUE, FXTRUE);
      else
        _grAAVpDrawTriangle((const float *)v[0], (const float *)v[1], (const float *)v[2], FXTRUE, FXTRUE, FXTRUE);
      pointers = (float *)pointers + stride;
      flip = ~flip;
    }
    flip = ~flip;
  }

#if 0
  gc->state.fbi_config.fbzMode = fbzModeOld;
  gc->state.invalid |= fbzModeBIT;
  GR_FLUSH_STATE();
#endif

#undef FN_NAME
} /* _grAADrawVertexList */

/*
** Fine Grain Vertex API
** I hate this code.
*/
/*-------------------------------------------------------------------
  Function: _trisetup_mixed_datalist
  Date: 17-Jun-98
  Implementor(s): atai
  Description:
  this routine draw triangle with two different vertex layout. If the flag is FXTURE
  for a vertex, it will use the internal format. Otherwise, it will use the 
  external format
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
#if ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
GR_DDFUNC(_trisetup_mixed_datalist, FxI32, ( const void *va, const void *vb, const void *vc, FxBool f1, FxBool f2, FxBool f3 ))
{
  GR_DCL_GC;
  GR_DCL_HW;
  FxI32 xindex = 0;
  FxI32 yindex = 1;
  const float *fa = (const float *)va;
  const float *fb = (const float *)vb;
  const float *fc = (const float *)vc;
  float ooa, dxAB, dxBC, dyAB, dyBC;
  int i,culltest;
  int ii, ia, ib, ic;
  union { float f; int i; } ay;
  union { float f; int i; } by;
  union { float f; int i; } cy;
  float *fp;
  struct dataList_s *dlp;
  float snap_xa, snap_ya, snap_xb, snap_yb, snap_xc, snap_yc;
  FxBool aflag = f1, bflag = f2, cflag = f3;

  culltest = gc->state.cull_mode; /* 1 if negative, 0 if positive */
  _GlideRoot.stats.trisProcessed++;
  
  {
    snap_ya = (volatile float) (*((float *)va + yindex) + SNAP_BIAS);
    snap_yb = (volatile float) (*((float *)vb + yindex) + SNAP_BIAS);
    snap_yc = (volatile float) (*((float *)vc + yindex) + SNAP_BIAS);
  }
  /*
   **  Sort the vertices.
   **  Whenever the radial order is reversed (from counter-clockwise to
   **  clockwise), we need to change the area of the triangle.  Note
   **  that we know the first two elements are X & Y by looking at the
   **  grVertex structure.  
   */
  ay.f = snap_ya;
  by.f = snap_yb;
  cy.f = snap_yc;
  if (ay.i < 0) ay.i ^= 0x7FFFFFFF;
  if (by.i < 0) by.i ^= 0x7FFFFFFF;
  if (cy.i < 0) cy.i ^= 0x7FFFFFFF;
  if (ay.i < by.i) {
    if (by.i > cy.i) {              /* acb */
      if (ay.i < cy.i) {
        fa = (const float *)va + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)vb + xindex;
        aflag = f1;
        bflag = f3;
        cflag = f2;
        culltest ^= 1;
      } else {                  /* cab */
        fa = (const float *)vc + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vb + xindex;
        aflag = f3;
        bflag = f1;
        cflag = f2;
      }
      /* else it's already sorted */
    }
  } else {
    if (by.i < cy.i) {              /* bac */
      if (ay.i < cy.i) {
        fa = (const float *)vb + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vc + xindex;
        aflag = f2;
        bflag = f1;
        cflag = f3;
        culltest ^= 1;
      } else {                  /* bca */
        fa = (const float *)vb + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)va + xindex;
        aflag = f2;
        bflag = f3;
        cflag = f1;
      }
    } else {                    /* cba */
      fa = (const float *)vc + xindex;
      fb = (const float *)vb + xindex;
      fc = (const float *)va + xindex;
      aflag = f3;
      bflag = f2;
      cflag = f1;
      culltest ^= 1;
    }
  }
  {
    snap_xa = (volatile float) (*((float *)fa + xindex) + SNAP_BIAS);
    snap_xb = (volatile float) (*((float *)fb + xindex) + SNAP_BIAS);
    snap_xc = (volatile float) (*((float *)fc + xindex) + SNAP_BIAS);
    snap_ya = (volatile float) (*((float *)fa + yindex) + SNAP_BIAS);
    snap_yb = (volatile float) (*((float *)fb + yindex) + SNAP_BIAS);
    snap_yc = (volatile float) (*((float *)fc + yindex) + SNAP_BIAS);
  }

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
  GR_SETF( hw->FvA.x, snap_xa );
  GR_SETF( hw->FvA.y, snap_ya );

  dlp = gc->dataList;
  i = dlp->i;
  ii = 2;
  
  /* write out X & Y for vertex B */
  GR_SETF( hw->FvB.x, snap_xb );
  GR_SETF( hw->FvB.y, snap_yb );
  
  /* write out X & Y for vertex C */
  GR_SETF( hw->FvC.x, snap_xc );
  GR_SETF( hw->FvC.y, snap_yc );

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
  while (i) {
    fp = dlp->addr;
    if (i & 1) {                   /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0F );
      if (i & 2) P6FENCE;
      dlp++;
      i = dlp->i;
      ii++;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;

      if (dlp->bddr) {
        /* packed data (color) */
        FxU32 _bddr = dlp->bddr - i;
        dpBC = FbARRAY(fb,ib+_bddr);
        dpdx = FbARRAY(fa,ia+_bddr);
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FbARRAY(fc,ic+_bddr);
      }
      else {
        dpBC = FARRAY(fb,ib);
        dpdx = FARRAY(fa,ia);
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic);
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;
        
      GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
      GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
      dlp++;
      i = dlp->i;
      ii++;
      GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
    }
  }

  /* Draw the triangle by writing the area to the triangleCMD register */
  P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, _GlideRoot.pool.temp1.f ) );
  _GlideRoot.stats.trisDrawn++;

  GR_CHECK_SIZE();
  _GlideRoot.stats.trisDrawn++;
  return 1;
} /* _trisetup_mixed_datalist */
#endif


#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
GR_DDFUNC(_trisetup_mixed_datalist, FxI32, ( const void *va, const void *vb, const void *vc, FxBool f1, FxBool f2, FxBool f3 ))
{
  GR_DCL_GC;
  FxI32 xindex = 0;
  FxI32 yindex = 1;
  const float *fa = (const float *)va + xindex;
  const float *fb = (const float *)vb + xindex;
  const float *fc = (const float *)vc + xindex;
  float ooa, dxAB, dxBC, dyAB, dyBC;
  int i,culltest;
  int ii, ia, ib, ic;
  union { float f; int i; } ay;
  union { float f; int i; } by;
  union { float f; int i; } cy;
  float *fp;
  struct dataList_s *dlp;
  volatile FxU32 *fifoPtr;
  float snap_xa, snap_ya, snap_xb, snap_yb, snap_xc, snap_yc;
  FxBool aflag = f1, bflag = f2, cflag = f3;
  
  culltest = gc->state.cull_mode; /* 1 if negative, 0 if positive */
  _GlideRoot.stats.trisProcessed++;
  
  {
    snap_ya = (volatile float) (*((float *)va + yindex) + SNAP_BIAS);
    snap_yb = (volatile float) (*((float *)vb + yindex) + SNAP_BIAS);
    snap_yc = (volatile float) (*((float *)vc + yindex) + SNAP_BIAS);
  }
  /*
   **  Sort the vertices.
   **  Whenever the radial order is reversed (from counter-clockwise to
   **  clockwise), we need to change the area of the triangle.  Note
   **  that we know the first two elements are X & Y by looking at the
   **  grVertex structure.  
   */
  ay.f = snap_ya;
  by.f = snap_yb;
  cy.f = snap_yc;
  if (ay.i < 0) ay.i ^= 0x7FFFFFFF;
  if (by.i < 0) by.i ^= 0x7FFFFFFF;
  if (cy.i < 0) cy.i ^= 0x7FFFFFFF;
  if (ay.i < by.i) {
    if (by.i > cy.i) {              /* acb */
      if (ay.i < cy.i) {
        fa = (const float *)va + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)vb + xindex;
        aflag = f1;
        bflag = f3;
        cflag = f2;
        culltest ^= 1;
      } else {                  /* cab */
        fa = (const float *)vc + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vb + xindex;
        aflag = f3;
        bflag = f1;
        cflag = f2;
      }
      /* else it's already sorted */
    }
  } else {
    if (by.i < cy.i) {              /* bac */
      if (ay.i < cy.i) {
        fa = (const float *)vb + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vc + xindex;
        aflag = f2;
        bflag = f1;
        cflag = f3;
        culltest ^= 1;
      } else {                  /* bca */
        fa = (const float *)vb + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)va + xindex;
        aflag = f2;
        bflag = f3;
        cflag = f1;
      }
    } else {                    /* cba */
      fa = (const float *)vc + xindex;
      fb = (const float *)vb + xindex;
      fc = (const float *)va + xindex;
      aflag = f3;
      bflag = f2;
      cflag = f1;
      culltest ^= 1;
    }
  }
  {
    snap_xa = (volatile float) (*((float *)fa + xindex) + SNAP_BIAS);
    snap_xb = (volatile float) (*((float *)fb + xindex) + SNAP_BIAS);
    snap_xc = (volatile float) (*((float *)fc + xindex) + SNAP_BIAS);
    snap_ya = (volatile float) (*((float *)fa + yindex) + SNAP_BIAS);
    snap_yb = (volatile float) (*((float *)fb + yindex) + SNAP_BIAS);
    snap_yc = (volatile float) (*((float *)fc + yindex) + SNAP_BIAS);
  }

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
  ii = 2;

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

  while (i) {
    fp = dlp->addr;
    /* chip field change */
    if (i & 1) 
      goto secondary_packet;
    else {
      float dpAB, dpBC,dpdx, dpdy;

      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;

      if (dlp->bddr) {
        /* packed data (color) */
        FxU32 _bddr = dlp->bddr - i;
        dpBC = FbARRAY(fb,ib+_bddr);
        dpdx = FbARRAY(fa,ia+_bddr);
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FbARRAY(fc,ic+_bddr);
      }
      else {
        /* non packed data */
        dpBC = FARRAY(fb,ib);
        dpdx = FARRAY(fa,ia);
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic);
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      dlp++;
      i = dlp->i;
      ii++;
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

  GR_CHECK_SIZE();
  
  _GlideRoot.stats.trisDrawn++;

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
  ii++;
  while( i ) {
    float dpAB, dpBC,dpdx, dpdy;
    fp = dlp->addr;

    ia = aflag ? ii << 2 : i;
    ib = bflag ? ii << 2 : i;
    ic = cflag ? ii << 2 : i;
    
    if (dlp->bddr) {
      /* packed data (color) */
      FxU32 _bddr = dlp->bddr - i;
      dpBC = FbARRAY(fb,ib+_bddr);
      dpdx = FbARRAY(fa,ia+_bddr);
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FbARRAY(fc,ic+_bddr);
    }
    else {
      /* non packed data */
      dpBC = FARRAY(fb,ib);
      dpdx = FARRAY(fa,ia);
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,ic);
    }
    dpdx = dpAB * dyBC - dpBC * dyAB;
    FSET_GW_ENTRY( fifoPtr, 1, dpdx );
    dpdy = dpBC * dxAB - dpAB * dxBC;
    dlp++;
    i = dlp->i;
    ii++;
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
} /* _trisetup_mixed_datalist */
#endif

/*-------------------------------------------------------------------
  Function: _vp_trisetup_mixed_datalist
  Date: 17-Jun-98
  Implementor(s): atai
  Description:
  This routine draw triangle with two different vertex layout. If the flag is FXTURE
  for a vertex, it will use the internal format. Otherwise, it will use the 
  external format.
  It is for clip coordinates
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
FxI32 FX_CSTYLE
_vp_trisetup_mixed_datalist( const void *va, const void *vb, const void *vc, FxBool f1, FxBool f2, FxBool f3 )
{
  GR_DCL_GC;
  FxI32 xindex = 0;
  FxI32 yindex = 1;
  const float *fa = (const float *)va;
  const float *fb = (const float *)vb;
  const float *fc = (const float *)vc;
  float ooa, dxAB, dxBC, dyAB, dyBC;
  int i,culltest;
  int ii, ia, ib, ic;
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
  FxBool aflag = f1, bflag = f2, cflag = f3;

  culltest = gc->state.cull_mode; /* 1 if negative, 0 if positive */
  _GlideRoot.stats.trisProcessed++;

  /*
  ** oow
  */
  ia = aflag ? gc->w_index : gc->state.vData.wInfo.offset;
  ib = bflag ? gc->w_index : gc->state.vData.wInfo.offset;
  ic = cflag ? gc->w_index : gc->state.vData.wInfo.offset;

  oowa = 1.0f / FARRAY(fa, ia);
  oowb = 1.0f / FARRAY(fb, ib);
  oowc = 1.0f / FARRAY(fc, ic);
  
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
        aflag = f1;
        bflag = f3;
        cflag = f2;
        culltest ^= 1;
      } else {                  /* cab */
        fa = (const float *)vc + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vb + xindex;
        snap_ya = tmp_snap_yc;
        snap_yb = tmp_snap_ya;
        snap_yc = tmp_snap_yb;
        aflag = f3;
        bflag = f1;
        cflag = f2;
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
        aflag = f2;
        bflag = f1;
        cflag = f3;
        culltest ^= 1;
      } else {                  /* bca */
        fa = (const float *)vb + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)va + xindex;
        snap_ya = tmp_snap_yb;
        snap_yb = tmp_snap_yc;
        snap_yc = tmp_snap_ya;
        aflag = f2;
        bflag = f3;
        cflag = f1;
      }
    } else {                    /* cba */
      fa = (const float *)vc + xindex;
      fb = (const float *)vb + xindex;
      fc = (const float *)va + xindex;
      snap_ya = tmp_snap_yc;
      snap_yb = tmp_snap_yb;
      snap_yc = tmp_snap_ya;
      aflag = f3;
      bflag = f2;
      cflag = f1;
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
  ii = 2;
  
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
        
        ia = aflag ? ii << 2 : i;
        ib = bflag ? ii << 2 : i;
        ic = cflag ? ii << 2 : i;
    
        if (dlp->bddr) {
          FxU32 _bddr = dlp->bddr - i;
          dpBC = FbARRAY(fb,ib+_bddr);
          dpdx = FbARRAY(fa,ia+_bddr);
          FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
          dpAB = dpdx - dpBC;
          dpBC = dpBC - FbARRAY(fc,ic+_bddr);
        }
        else {
          dpBC = FARRAY(fb,ib) * _GlideRoot.pool.f255;
          dpdx = FARRAY(fa,ia) * _GlideRoot.pool.f255;
          FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
          dpAB = dpdx - dpBC;
          dpBC = dpBC - FARRAY(fc,ic) * _GlideRoot.pool.f255;
        }
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        FSET_GW_ENTRY( fifoPtr, 1, dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;

        dlp++;
        i = dlp->i;
        ii++;
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
      
      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;
    
      dpBC = FARRAY(fb,ib) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      dpdx = FARRAY(fa,ia) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,ic) * oowc * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      ii++;
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
      
      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;
    
      if (dlp->bddr) {
        FxU32 _bddr = dlp->bddr - i;
        dpBC = FbARRAY(fb,ib+_bddr);
        dpdx = FbARRAY(fa,ia+_bddr);
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FbARRAY(fc,ic+_bddr);
      }
      else {
        dpBC = FARRAY(fb,ib) * _GlideRoot.pool.f255;
        dpdx = FARRAY(fa,ia) * _GlideRoot.pool.f255;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic) * _GlideRoot.pool.f255;
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      ii++;
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
        
        ia = aflag ? ii << 2 : i;
        ib = bflag ? ii << 2 : i;
        ic = cflag ? ii << 2 : i;
    
        dpBC = FARRAY(fb,ib) * oowb * 256.f;
        dpdx = FARRAY(fa,ia) * oowa * 256.f;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic) * oowc * 256.f;
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        FSET_GW_ENTRY( fifoPtr, 1, dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;
        
        dlp++;
        i = dlp->i;
        ii++;
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
      
      dpBC = oowb;
      dpdx = oowa;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      
      dpAB = dpdx - dpBC;
      dpBC = dpBC - oowc;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      ii++;
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

        ia = aflag ? ii << 2 : i;
        ib = bflag ? ii << 2 : i;
        ic = cflag ? ii << 2 : i;
    
        dpBC = FARRAY(fb,ib) * oowb;
        dpdx = FARRAY(fa,ia) * oowa;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic) * oowc;
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
      ii++;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_ST_TMU1) {
    fp = dlp->addr;
    /* chip field change */
    if (i & 1)
      goto secondary_packet;
    else {
      float dpAB, dpBC,dpdx, dpdy;

      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;
    
      dpBC = FARRAY(fb,ib) * oowb * 256.f;
      dpdx = FARRAY(fa,ia) * oowa * 256.f;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );

      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,ic) * oowc * 256.f;
      dpdx = dpAB * dyBC - dpBC * dyAB;

GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;

GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
      dlp++;
      i = dlp->i;
      ii++;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
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

        ia = aflag ? ii << 2 : i;
        ib = bflag ? ii << 2 : i;
        ic = cflag ? ii << 2 : i;
    
        dpBC = FARRAY(fb,ib) * oowb;
        dpdx = FARRAY(fa,ia) * oowa;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );

        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic) * oowc;
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
      ii++;
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
  ii++;

  if (paramIndex & STATE_REQUIRES_IT_DRGB) {
    for (k = 0; k < 3; k++) {
      float dpAB, dpBC,dpdx, dpdy;
      fp = dlp->addr;

      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;
    
      if (dlp->bddr) {
        /* packed data (color) */
        FxU32 _bddr = dlp->bddr - i;
        dpBC = FbARRAY(fb,ib+_bddr);
        dpdx = FbARRAY(fa,ia+_bddr);
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FbARRAY(fc,ic+_bddr);
      }
      else {
        /* non packed data */
        dpBC = FARRAY(fb,ib) * _GlideRoot.pool.f255;
        dpdx = FARRAY(fa,ia) * _GlideRoot.pool.f255;
        FSET_GW_ENTRY( fifoPtr, 0, dpdx );
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic) * _GlideRoot.pool.f255;
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      dlp++;
      i = dlp->i;
      ii++;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_OOZ) {
    float dpAB, dpBC,dpdx, dpdy;
    fp = dlp->addr;

    ia = aflag ? ii << 2 : i;
    ib = bflag ? ii << 2 : i;
    ic = cflag ? ii << 2 : i;
    
    /* non packed data */
    dpBC = FARRAY(fb,ib) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
    dpdx = FARRAY(fa,ia) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
    FSET_GW_ENTRY( fifoPtr, 0, dpdx );
    dpAB = dpdx - dpBC;
    dpBC = dpBC - FARRAY(fc,ic) * oowc * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
    dpdx = dpAB * dyBC - dpBC * dyAB;
    FSET_GW_ENTRY( fifoPtr, 1, dpdx );
    dpdy = dpBC * dxAB - dpAB * dxBC;
    dlp++;
    i = dlp->i;
    ii++;
    FSET_GW_ENTRY( fifoPtr, 2, dpdy );
    fifoPtr += 3;
  }
  if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) {
    float dpAB, dpBC,dpdx, dpdy;
    fp = dlp->addr;

    ia = aflag ? ii << 2 : i;
    ib = bflag ? ii << 2 : i;
    ic = cflag ? ii << 2 : i;
    
    if (dlp->bddr) {
      /* packed data (color) */
      FxU32 _bddr = dlp->bddr - i;
      dpBC = FbARRAY(fb,i+_bddr);
      dpdx = FbARRAY(fa,i+_bddr);
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FbARRAY(fc,i+_bddr);
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
    ii++;
    FSET_GW_ENTRY( fifoPtr, 2, dpdy );
    fifoPtr += 3;
  }
  if (paramIndex & STATE_REQUIRES_ST_TMU0) {
    for (k = 0; k < 2; k++) {
      float dpAB, dpBC,dpdx, dpdy;
      fp = dlp->addr;

      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;    

      dpBC = FARRAY(fb,i) * oowb * 256.f;
      dpdx = FARRAY(fa,i) * oowa * 256.f;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * oowc * 256.f;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      dlp++;
      i = dlp->i;
      ii++;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_OOW_FBI) {
    float dpAB, dpBC,dpdx, dpdy;
    fp = dlp->addr;
    dpBC = oowb;
    dpdx = oowa;
    FSET_GW_ENTRY( fifoPtr, 0, dpdx );
    dpAB = dpdx - dpBC;
    dpBC = dpBC - oowc;
    dpdx = dpAB * dyBC - dpBC * dyAB;
    FSET_GW_ENTRY( fifoPtr, 1, dpdx );
    dpdy = dpBC * dxAB - dpAB * dxBC;
    dlp++;
    i = dlp->i;
    ii++;
    FSET_GW_ENTRY( fifoPtr, 2, dpdy );
    fifoPtr += 3;
  }
  if (paramIndex & STATE_REQUIRES_W_TMU0) {
    float dpAB, dpBC,dpdx, dpdy;
    fp = dlp->addr;
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;    

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
    ii++;
    FSET_GW_ENTRY( fifoPtr, 2, dpdy );
    fifoPtr += 3;
  }
  if (paramIndex & STATE_REQUIRES_ST_TMU1) {
    for (k = 0; k < 2; k++) {
      float dpAB, dpBC,dpdx, dpdy;
      fp = dlp->addr;
      
      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;    
      
      dpBC = FARRAY(fb,i) * oowb * 256.f;
      dpdx = FARRAY(fa,i) * oowa * 256.f;
      FSET_GW_ENTRY( fifoPtr, 0, dpdx );
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,i) * oowc * 256.f;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      FSET_GW_ENTRY( fifoPtr, 1, dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      dlp++;
      i = dlp->i;
      ii++;
      FSET_GW_ENTRY( fifoPtr, 2, dpdy );
      fifoPtr += 3;
    }
  }
  if (paramIndex & STATE_REQUIRES_W_TMU1) {
    float dpAB, dpBC,dpdx, dpdy;
    fp = dlp->addr;
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) {
      ia = aflag ? ii << 2 : i;
      ib = bflag ? ii << 2 : i;
      ic = cflag ? ii << 2 : i;    

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
    ii++;
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
_vp_trisetup_mixed_datalist( const void *va, const void *vb, const void *vc, FxBool f1, FxBool f2, FxBool f3 )
{
  GR_DCL_GC;
  GR_DCL_HW;
  FxI32 xindex = 0;
  FxI32 yindex = 1;
  const float *fa = (const float *)va;
  const float *fb = (const float *)vb;
  const float *fc = (const float *)vc;
  float ooa, dxAB, dxBC, dyAB, dyBC;
  int i,culltest;
  int ii, ia, ib, ic;
  union { float f; int i; } ay;
  union { float f; int i; } by;
  union { float f; int i; } cy;
  float *fp;
  struct dataList_s *dlp;
  float oowa, oowb, oowc;
  int k;
  float snap_xa, snap_ya, snap_xb, snap_yb, snap_xc, snap_yc;
  float tmp_snap_ya, tmp_snap_yb, tmp_snap_yc;
  FxBool aflag = f1, bflag = f2, cflag = f3;

  culltest = gc->state.cull_mode; /* 1 if negative, 0 if positive */
  _GlideRoot.stats.trisProcessed++;

  /*
  ** oow
  */
  ia = aflag ? gc->w_index : gc->state.vData.wInfo.offset;
  ib = bflag ? gc->w_index : gc->state.vData.wInfo.offset;
  ic = cflag ? gc->w_index : gc->state.vData.wInfo.offset;

  oowa = 1.0f / FARRAY(fa, ia);
  oowb = 1.0f / FARRAY(fb, ib);
  oowc = 1.0f / FARRAY(fc, ic);
  
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
        aflag = f1;
        bflag = f3;
        cflag = f2;
        culltest ^= 1;
      } else {                  /* cab */
        fa = (const float *)vc + xindex;
        fb = (const float *)va + xindex;
        fc = (const float *)vb + xindex;
        snap_ya = tmp_snap_yc;
        snap_yb = tmp_snap_ya;
        snap_yc = tmp_snap_yb;
        aflag = f3;
        bflag = f1;
        cflag = f2;
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
        aflag = f2;
        bflag = f1;
        cflag = f3;
        culltest ^= 1;
      } else {                  /* bca */
        fa = (const float *)vb + xindex;
        fb = (const float *)vc + xindex;
        fc = (const float *)va + xindex;
        snap_ya = tmp_snap_yb;
        snap_yb = tmp_snap_yc;
        snap_yc = tmp_snap_ya;
        aflag = f2;
        bflag = f3;
        cflag = f1;
      }
    } else {                    /* cba */
      fa = (const float *)vc + xindex;
      fb = (const float *)vb + xindex;
      fc = (const float *)va + xindex;
      snap_ya = tmp_snap_yc;
      snap_yb = tmp_snap_yb;
      snap_yc = tmp_snap_ya;
      aflag = f3;
      bflag = f2;
      cflag = f1;
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
  ii = 2;
  
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
        ii++;
      }
      else {
        float dpAB, dpBC,dpdx, dpdy;
        
        ia = aflag ? ii >> 2 : i;
        ib = bflag ? ii >> 2 : i;
        ic = cflag ? ii >> 2 : i;

        if (dlp->bddr) {
          FxU32 _bddr = dlp->bddr - i;
          dpBC = FbARRAY(fb,ib+_bddr);
          dpdx = FbARRAY(fa,ia+_bddr);
          GR_SETF( fp[0], dpdx );
        
          dpAB = dpdx - dpBC;
          dpBC = dpBC - FbARRAY(fc,ic+_bddr);
        }
        else {
          dpBC = FARRAY(fb,ib) * _GlideRoot.pool.f255;
          dpdx = FARRAY(fa,ia) * _GlideRoot.pool.f255;
          GR_SETF( fp[0], dpdx );
        
          dpAB = dpdx - dpBC;
          dpBC = dpBC - FARRAY(fc,ic) * _GlideRoot.pool.f255;
        }
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;

        dlp++;
        i = dlp->i;
        ii++;
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
      ii++;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      ia = aflag ? ii >> 2 : i;
      ib = bflag ? ii >> 2 : i;
      ic = cflag ? ii >> 2 : i;

      dpBC = FARRAY(fb,ib) * oowb * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      dpdx = FARRAY(fa,ia) * oowa * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      GR_SETF( fp[0], dpdx );
      
      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,ic) * oowc * gc->state.Viewport.hdepth + gc->state.Viewport.oz;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      ii++;
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
      ii++;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      ia = aflag ? ii >> 2 : i;
      ib = bflag ? ii >> 2 : i;
      ic = cflag ? ii >> 2 : i;

      if (dlp->bddr) {
        FxU32 _bddr = dlp->bddr - i;
        dpBC = FbARRAY(fb,ib+_bddr);
        dpdx = FbARRAY(fa,ia+_bddr);
        GR_SETF( fp[0], dpdx );
      
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FbARRAY(fc,ic+_bddr);
      }
      else {
        dpBC = FARRAY(fb,ib) * _GlideRoot.pool.f255;
        dpdx = FARRAY(fa,ia) * _GlideRoot.pool.f255;
        GR_SETF( fp[0], dpdx );
      
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic) * _GlideRoot.pool.f255;
      }
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      ii++;
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
        ii++;
      }
      else {
        float dpAB, dpBC,dpdx, dpdy;
        
        ia = aflag ? ii >> 2 : i;
        ib = bflag ? ii >> 2 : i;
        ic = cflag ? ii >> 2 : i;

        dpBC = FARRAY(fb,ib) * oowb * 256.f;
        dpdx = FARRAY(fa,ia) * oowa * 256.f;
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic) * oowc * 256.f;
        dpdx = dpAB * dyBC - dpBC * dyAB;
        
        GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
        dpdy = dpBC * dxAB - dpAB * dxBC;
        
        dlp++;
        i = dlp->i;
        ii++;
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
      ii++;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;
      
      dpBC = oowb;
      dpdx = oowa;
      GR_SETF( fp[0], dpdx );
      
      dpAB = dpdx - dpBC;
      dpBC = dpBC - oowc;
      dpdx = dpAB * dyBC - dpBC * dyAB;
      
      GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;
      
      dlp++;
      i = dlp->i;
      ii++;
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
      ii++;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;

      if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) {
        ia = aflag ? ii >> 2 : i;
        ib = bflag ? ii >> 2 : i;
        ic = cflag ? ii >> 2 : i;

        dpBC = FARRAY(fb,ib) * oowb;
        dpdx = FARRAY(fa,ia) * oowa;
        GR_SETF( fp[0], dpdx );
        
        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic) * oowc;
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
      ii++;
      GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
    }
  }
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) {
    fp = dlp->addr;
    if (i & 1) {                   /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0F );
      if (i & 2) P6FENCE;
      dlp++;
      i = dlp->i;
      ii++;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;

      ia = aflag ? ii >> 2 : i;
      ib = bflag ? ii >> 2 : i;
      ic = cflag ? ii >> 2 : i;

      dpBC = FARRAY(fb,ib) * oowb * 256.f;
      dpdx = FARRAY(fa,ia) * oowa * 256.f;
      GR_SETF( fp[0], dpdx );

      dpAB = dpdx - dpBC;
      dpBC = dpBC - FARRAY(fc,ic) * oowc * 256.f;
      dpdx = dpAB * dyBC - dpBC * dyAB;

GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
      GR_SETF( fp[DPDX_OFFSET>>2] , dpdx );
      dpdy = dpBC * dxAB - dpAB * dxBC;

GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
      dlp++;
      i = dlp->i;
      ii++;
      GR_SETF( fp[DPDY_OFFSET>>2] , dpdy );
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
      ii++;
    }
    else {
      float dpAB, dpBC,dpdx, dpdy;

      if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) {
        ia = aflag ? ii >> 2 : i;
        ib = bflag ? ii >> 2 : i;
        ic = cflag ? ii >> 2 : i;

        dpBC = FARRAY(fb,ib) * oowb;
        dpdx = FARRAY(fa,ia) * oowa;
        GR_SETF( fp[0], dpdx );

        dpAB = dpdx - dpBC;
        dpBC = dpBC - FARRAY(fc,ic) * oowc;
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
      ii++;
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


