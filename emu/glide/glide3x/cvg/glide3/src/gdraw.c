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
 ** Revision 1.1.1.1.8.7  2006/01/16 21:22:41  jwrdegoede
 ** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
 **
 ** Revision 1.1.1.1.8.6  2005/08/13 21:06:57  jwrdegoede
 ** Last needed 64 bit fixes for h5/h3, complete 64 bit support for cvg
 **
 ** Revision 1.1.1.1.8.5  2005/06/17 14:18:49  jwrdegoede
 ** Fix few warnings when USE_X86=1 is not set
 **
 ** Revision 1.1.1.1.8.4  2005/06/10 19:10:54  jwrdegoede
 ** Really fix cvg linking when GL_X86 is not defined
 **
 ** Revision 1.1.1.1.8.3  2005/06/09 18:32:08  jwrdegoede
 ** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
 **
 ** Revision 1.1.1.1.8.2  2005/05/07 08:40:17  jwrdegoede
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
** 28    10/07/98 9:43p Peter
** triangle procs for 3DNow!(tm)
** 
** 27    10/07/98 10:27a Atai
** fixed fifo checking in dbg mode for zero length line
** 
** 26    10/06/98 8:23p Peter
** 3DNow!(tm) texture downloads
** 
** 25    10/06/98 7:18p Atai
** added triangle and drawlist asm routine for clip coords
** 
** 24    9/21/98 7:02p Dow
** 3DNow! (tm) mods
** 
** 23    9/08/98 5:57p Atai
** fixed clip coords line routine
** 
** 22    7/24/98 1:41p Hohn
** 
** 21    7/22/98 2:01p Atai
** fixed clip cooords q and tri stats
** 
** 20    7/15/98 2:02p Atai
** use mul to count the tri stats
** 
** 19    6/19/98 1:02p Atai
** fixed tri stats
** 
** 18    6/18/98 6:25p Atai
** fixed line routine for grDrawVertexArrayContiguous
** 
** 17    6/11/98 5:12p Atai
** added aa case for OGL
** 
** 16    6/09/98 9:53a Peter
** point hack ordering bug
** 
** 15    6/08/98 3:20p Atai
** fix tri stats
** 
** 14    6/05/98 6:14p Atai
** fix grDrawTriangle
** 
** 13    6/05/98 5:15p Atai
** make asm pass
** 
** 12    5/29/98 2:09p Atai
** remove polygon code
** 
** 11    5/19/98 1:04p Peter
** point fun
** 
** 10    5/12/98 4:45p Atai
** turn off asm for non-c version temporarily
** 
** 9     2/24/98 6:00p Atai
** use 0 offset for vertex data
** 
** 8     2/11/98 7:04p Atai
** added grDrawTriangle setup code
** 
** 7     2/09/98 6:19p Atai
** fix clip space poiint debug assertion
** 
** 6     1/30/98 10:08a Atai
** fixed clip space drawtriangle routine
** 
** 5     1/26/98 11:30a Atai
** update to new glide.h
** 
** 4     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
** 
** 3     1/19/98 1:38p Atai
** fixed _grDrawLineStrip zero length lline
** 
** 2     1/18/98 12:03p Atai
** sync to rev 17 spec
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
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
 **
 */
#include <memory.h>

#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"

#include "fxinline.h"
#ifdef GLIDE_DEBUG
#include <math.h>
#endif /* GLIDE_DEBUG */

#define SST_XY_HALF      (1 << (SST_XY_FRACBITS - 1))
#define SST_XY_ONE       (1 << SST_XY_FRACBITS)

#define OUTBOUNDSX(a) ((a->x < 0.f ? 1 : 0) || (a->x > gc->state.screen_width ? 1 : 0))
#define OUTBOUNDSY(a) ((a->y < 0.f ? 1 : 0) || (a->y > gc->state.screen_height ? 1 : 0))
#define OUTBOUNDS(a) (OUTBOUNDSX(a) || OUTBOUNDSY(a))

/*---------------------------------------------------------------------------
** grDrawPoint
*/
GR_ENTRY(grDrawPoint, void, (const void *p))
{
#define FN_NAME "grDrawPoint"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);
  GDBG_INFO_MORE(gc->myLevel, "(p = 0x%x)\n", p);

  GR_FLUSH_STATE();

  (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_POINTS_MASK
   ? _grAADrawPoints
   : _grDrawPoints)(GR_VTX_PTR_ARRAY, 1, (void *)&p);
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

GR_ENTRY(grDrawLine, void, (const void *a, const void *b))
{
#define FN_NAME "grDrawLine"
  float *vlist[2];

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 91);
  GDBG_INFO_MORE(gc->myLevel, "(a = 0x%x, b = 0x%x)\n", a, b);

  GR_FLUSH_STATE();
  
  vlist[0] = (float *)a;
  vlist[1] = (float *)b;

  if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_LINES_MASK)
    _grAADrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, 2, (void *)vlist);
  else
    _grDrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, 2, (void *)vlist);
#undef FN_NAME
} /* grDrawLine */

/*---------------------------------------------------------------------------
 ** grDrawTriangle
 */
GR_ENTRY(grDrawTriangle, void, (const void *a, const void *b, const void *c))
{
#define FN_NAME "grDrawTriangle"

  GR_BEGIN_NOFIFOCHECK("grDrawTriangle",92);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x,0x%x,0x%x)\n",a,b,c);
  GR_CHECK_F(myName, !a || !b || !c, "NULL pointer passed");

#if GLIDE_USE_C_TRISETUP
  /* This is the code that is now vectored through via the function pointer
   * gc->curArchProcs.curTriProc.
   */
  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_CLIP_COORDS) {
    float *vlist[3];
    vlist[0] = (float *)a;
    vlist[1] = (float *)b;
    vlist[2] = (float *)c;
    _grDrawTriangles_Default(GR_VTX_PTR_ARRAY, 3, vlist);
  } else {
    /* 
    ** draw a simple triangle 
    */
    FxI32 i;
    FxU32 dataElem;
    
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

    GR_FLUSH_STATE();
    GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize, 1);
    TRI_STRIP_BEGIN(kSetupStrip, 3, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD);

    TRI_SETF(FARRAY(a, 0));
    TRI_SETF(FARRAY(a, 4));
    dataElem = 0;
    i = gc->tsuDataList[dataElem];
    while (i != GR_DLIST_END) {
      TRI_SETF(FARRAY(a, i));
      dataElem++;
      i = gc->tsuDataList[dataElem];
    }
    TRI_SETF(FARRAY(b, 0));
    TRI_SETF(FARRAY(b, 4));
    dataElem = 0;
    i = gc->tsuDataList[dataElem];
    while (i != GR_DLIST_END) {
      TRI_SETF(FARRAY(b, i));
      dataElem++;
      i = gc->tsuDataList[dataElem];
    }
    TRI_SETF(FARRAY(c, 0));
    TRI_SETF(FARRAY(c, 4));
    dataElem = 0;
    i = gc->tsuDataList[dataElem];
    while (i != GR_DLIST_END) {
      TRI_SETF(FARRAY(c, i));
      dataElem++;
      i = gc->tsuDataList[dataElem];
    }
    TRI_END;
    GR_CHECK_SIZE();
  }
#else /* !GLIDE_USE_C_TRISETUP */
  GR_FLUSH_STATE();

  /* _trisetup and _trisetup_asm return 0 if culled, 1 if drawn */ 
#if GLIDE_DEBUG
  /* HackAlert: Nuke the fifo ptr checking stuff here if we're just
   * debugging teh asm tri code.
   */
  if (TRISETUP(a, b, c) != 0) {
    gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr;
    gc->checkCounter = 0;
  }
#else /* !GLIDE_DEBUG */
  TRISETUP(a, b, c);
#endif /* !GLIDE_DEBUG */

  GR_END();
#endif /* !GLIDE_USE_C_TRISETUP */
#undef FN_NAME
} /* grDrawTriangle */

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

#define DA_BEGIN \
{ \
  FxU32* packetPtr = gc->cmdTransportInfo.fifoPtr; \
  FxU32 packetVal;

#define DA_CONT(__setupMode, __params, __nVertex, __vertexSize, __cmd) \
  packetVal = (((__setupMode) << SSTCP_PKT3_SMODE_SHIFT) |   /* [27:22] */ \
               (__params) |                                  /* pack[28] params[21:10] */ \
               ((__nVertex) << SSTCP_PKT3_NUMVERTEX_SHIFT) | /* [9:6] */ \
               (__cmd) |                                     /* command [5:3] */ \
               SSTCP_PKT3);                                  /* type [2:0] */ \
  SET(*packetPtr++, packetVal);

#define DA_SETF(__val) \
  SETF(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxFloat))

#define DA_SET(__val) \
  SET(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32))

#define DA_END \
  gc->cmdTransportInfo.fifoRoom -= ((unsigned long)packetPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = packetPtr; \
  FIFO_ASSERT(); \
}

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

  /*
   * the point routine is similar to grDrawPoint routine
   * except the data set up is from the pointer array and 
   * its data layout
   */
  FxI32 stride;

  /* we snap to an integer by adding a large enough number that it
   * shoves all fraction bits off the right side of the mantissa.
   *
   * NB: IEEE rounds to nearest integer by default, but applications
   * can change the rounding mode so that it is difficult to get the
   * correct truncation/ceiling operation w/ a simple adjustment to
   * the bias so we do it all by hand treating the fp # as a
   * bucket-o-bits.
   *
   * NB: The constant kNumMantissaBits defines how many bits of
   * integer precision a coordinate can have. This needs to be atleast
   * as large as the maximum hw screen resolution. We later use this
   * to compute a logical 1 value to fill an entire pixel.  
   */
#define kNumMantissaBits 12UL
  const float bias = (const float)(3UL << kNumMantissaBits);

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE(gc->myLevel, "(count = %d, pointers = 0x%x)\n",
                 count, pointers);

  /* 
   * In the sbench example, we observe larger buffer, less fifo 
   * size updating and less checking make the routine run faster.
   * Here we use a point buffer of size 100. We also remove the size
   * check outside the buffer loop
   */
#define POINTS_BUFFER  100

  if (mode == 0)
    stride = gc->state.vData.vStride;
  else
    stride = sizeof(float *) / sizeof (float);

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    while (count > 0) {
      float *vPtr;
      FxI32 k, i;
      FxI32 vcount = count >= POINTS_BUFFER ? POINTS_BUFFER : count;
      GR_SET_EXPECTED_SIZE(((sizeof(FxU32) << 2) +  gc->state.vData.vSize) * vcount, vcount << 1);

      /* begin points routine */
      DA_BEGIN;
      
      for (k = 0; k < vcount; k++) {
        vPtr = pointers;
        if (mode) vPtr = *(float **)vPtr;
        
        pointers = (float *)pointers + stride;
        
        GDBG_INFO_MORE(gc->myLevel, "(%f %f)\n",  
                       FARRAY(vPtr,gc->state.vData.vertexInfo.offset), 
                       FARRAY(vPtr,gc->state.vData.vertexInfo.offset + 4));
        
        {
          FxU32 x, y;
          FxU32 dataElem;
          
          DA_CONT(kSetupStrip | kSetupCullDisable, 0x00,
                  0x02, sizeof(FxU32) << 1, SSTCP_PKT3_BDDDDD);

          /* Convert to 32-bit representation */
          _GlideRoot.pool.temp1.f = FARRAY(vPtr, gc->state.vData.vertexInfo.offset) + bias;
          _GlideRoot.pool.temp2.f = FARRAY(vPtr, gc->state.vData.vertexInfo.offset + 4) + bias;
        
          /* draw a little triangle, with the lower left corner at pixel center. */
          
          /* The approach here is to split the triangle into two packets, one
           * that sends just the coordinates of the last two points of the
           * triangle w/ no other parameter information, and then the
           * centered point w/ all of the parameter information. The first
           * packet is has a BDD command, but is incomplete, so nothing
           * draws, the next packet has a DDD command so will complete the
           * triangle from the first packet sent.
           */

          /* Mask off the real fractional bits from the mantissa */
          x = ((_GlideRoot.pool.temp1.u & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
               (0x01UL << (22UL - kNumMantissaBits)));
          y = ((_GlideRoot.pool.temp2.u & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
               (0x01UL << (22UL - kNumMantissaBits)));
          
          /* Lower right corner */          
          DA_SET(x);
          DA_SET(y);

          /* Upper right corner */
          y -= (0x01UL << (21UL - kNumMantissaBits));
          DA_SET(x);
          dataElem = 0;
          DA_SET(y);

          /* Upper Left corner */
          x -= (0x01UL << (21UL - kNumMantissaBits));
          
          /* Packet w/ actual point coordinate and parameter data */
          DA_CONT(kSetupStrip | kSetupCullDisable, gc->cmdTransportInfo.paramMask, 
                         1, gc->state.vData.vSize, SSTCP_PKT3_DDDDDD);
          
          i = gc->tsuDataList[dataElem];
          DA_SET(x);
          DA_SET(y);
          while (i != GR_DLIST_END) {
            DA_SETF(FARRAY(vPtr, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
          }
        }
      }
      DA_END;
      GR_CHECK_SIZE();
      /* end points routine */
      
      count -= POINTS_BUFFER;
    }
  } else {
    /*
     * first cut of clip space coordinate code.
     */
    
    float oow;
    
    while (count > 0) {
      float *vPtr;
      FxI32 k;
      FxI32 vcount = count >= POINTS_BUFFER ? POINTS_BUFFER : count;
      GR_SET_EXPECTED_SIZE(((sizeof(FxU32) << 2) +  gc->state.vData.vSize) * vcount, vcount << 1);

      /* begin points routine */
      DA_BEGIN;
      
      for (k = 0; k < vcount; k++) {
        
        vPtr = pointers;
        if (mode)
          vPtr = *(float **)vPtr;
        oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset);        
        pointers = (float *)pointers + stride;
        
        {
          FxU32 x, y;

          DA_CONT(kSetupStrip | kSetupCullDisable, 0x00,
                         0x02, sizeof(FxU32) << 1, SSTCP_PKT3_BDDDDD);

          /* Convert to 32-bit representation */
          _GlideRoot.pool.temp1.f = (FARRAY(vPtr, gc->state.vData.vertexInfo.offset) * 
                                    oow * 
                                    gc->state.Viewport.hwidth + 
                                    gc->state.Viewport.ox + 
                                    bias);
          _GlideRoot.pool.temp2.f = (FARRAY(vPtr, gc->state.vData.vertexInfo.offset + 4) * 
                                    oow *
                                    gc->state.Viewport.hheight + 
                                    gc->state.Viewport.oy +
                                    bias);

          /* Mask off the real fractional bits from the mantissa */
          x = ((_GlideRoot.pool.temp1.u & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
               (0x01UL << (22UL - kNumMantissaBits)));
          y = ((_GlideRoot.pool.temp2.u & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
               (0x01UL << (22UL - kNumMantissaBits)));
          
          /* Lower right cornder */
          DA_SET(x);
          DA_SET(y);

          /* Upper right corner. */
          y -= (0x01UL << (21UL - kNumMantissaBits));
          DA_SET(x);
          DA_SET(y);

          /* Upper Left corner */
          x -= (0x01UL << (21UL - kNumMantissaBits));
          
          /* Packet w/ actual point coordinate and parameter data */
          DA_CONT(kSetupStrip | kSetupCullDisable, gc->cmdTransportInfo.paramMask, 
                         1, gc->state.vData.vSize, SSTCP_PKT3_DDDDDD);
          
          DA_SET(x);
          DA_SET(y);
          DA_VP_SETFS(vPtr, oow);
        }
      }
      DA_END;
      GR_CHECK_SIZE();
      /* end points routine */
      
      count -= POINTS_BUFFER;
    }
  }

  _GlideRoot.stats.pointsDrawn+=count;
  _GlideRoot.stats.othertrisDrawn+=(count<<1);

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

#if 1
  /*
   * the line routine is similar to grDrawLine routine
   * except the data set up is from the pointer array and 
   * its data layout
   */
  FxU32 vertexParamOffset;
  FxI32 sCount, stride;

#define  DX _GlideRoot.pool.temp1
#define ADY _GlideRoot.pool.temp2

  GR_BEGIN_NOFIFOCHECK("_grDrawLineStrip", 91);

  GDBG_INFO_MORE(gc->myLevel, "(count = %d, pointers = 0x%x)\n",
                 count, pointers);
  
  vertexParamOffset = gc->state.vData.vSize;

#define LINES_BUFFER  100

  if (mode == 0)
    stride = gc->state.vData.vStride;
  else
    stride = sizeof(float *) / sizeof (float);

  if (ltype == GR_LINES)
    sCount = count >> 1; /* line list */
  else
    sCount = count-1;    /* strip line */

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    while (sCount > 0) {
      FxI32 k, vcount = sCount >= LINES_BUFFER ? LINES_BUFFER : sCount;

      GR_SET_EXPECTED_SIZE((gc->state.vData.vSize << 2) * vcount, vcount);
      DA_BEGIN;
      for (k = 0; k < vcount; k++) {
        float *a = (float *)pointers;
        float *b = (float *)pointers + stride;
        if (mode) {
          a = *(float **)a;
          b = *(float **)b;
        }
        pointers = (float *)pointers + stride;
        if (ltype == GR_LINES)
          pointers = (float *)pointers + stride;
        
        /*
        ** compute absolute deltas and draw from low Y to high Y
        */
        ADY.f = FARRAY(b, gc->state.vData.vertexInfo.offset+4) - FARRAY(a, gc->state.vData.vertexInfo.offset+4);
        if (ADY.i < 0) {
          float *tv;
          tv = a; a = b; b = tv;
          ADY.i ^= 0x80000000;            /* ady = -ady; */
        }
        
        DX.f = FARRAY(b, gc->state.vData.vertexInfo.offset) - FARRAY(a, gc->state.vData.vertexInfo.offset);
        DX.i &= 0x7fffffff;               /* abs(adx) */        

        /* check for zero-length lines */
        if ((DX.i >= ADY.i) && (DX.i == 0)) {
#ifdef GLIDE_DEBUG
          gc->expected_counter -= (gc->state.vData.vSize << 2 );
          gc->checkCounter -= ((gc->state.vData.vSize+1) << 2 );
#endif
          goto all_done;
        }

        /* Draw the triangle pair as a strip of 4 vertices.
         * We can skip all of the gradient calculation stuff.
         *
         * NB: There are two cases, x/y major lines, and each of these
         * loops are unrolled to send one set of endpoints of the 'line'
         * per iteration since we can use the same bias per iteration.
         */
        DA_CONT(kSetupCullDisable | kSetupStrip, gc->cmdTransportInfo.paramMask,
                0x04UL, vertexParamOffset, SSTCP_PKT3_BDDDDD);
        {
          int i;
          FxU32 dataElem;
          /* x major */
          if (DX.i >= ADY.i) {
            DA_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset));
            dataElem = 0;
            DA_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset+4) - _GlideRoot.pool.fHalf);
            i = gc->tsuDataList[dataElem];
            while (i != GR_DLIST_END) {
              DA_SETF(FARRAY(b, i));
              dataElem++;
              i = gc->tsuDataList[dataElem];
            }
            
            DA_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset));
            dataElem = 0;
            DA_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset+4) - _GlideRoot.pool.fHalf);
            i = gc->tsuDataList[dataElem];
            while (i != GR_DLIST_END) {
              DA_SETF(FARRAY(a, i));
              dataElem++;
              i = gc->tsuDataList[dataElem];
            }
            
            DA_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset));
            dataElem = 0;
            DA_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset+4) + _GlideRoot.pool.fHalf);
            i = gc->tsuDataList[dataElem];
            while (i != GR_DLIST_END) {
              DA_SETF(FARRAY(b, i));
              dataElem++;
              i = gc->tsuDataList[dataElem];
            }
            
            DA_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset));
            dataElem = 0;
            DA_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset+4) + _GlideRoot.pool.fHalf);
            i = gc->tsuDataList[dataElem];
            while (i != GR_DLIST_END) {
              DA_SETF(FARRAY(a, i));
              dataElem++;
              i = gc->tsuDataList[dataElem];
            }
          } else { /* y major */
            DA_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset) - _GlideRoot.pool.fHalf);
            dataElem = 0;
            DA_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset+4));
            i = gc->tsuDataList[dataElem];
            while (i != GR_DLIST_END) {
              DA_SETF(FARRAY(b, i));
              dataElem++;
              i = gc->tsuDataList[dataElem];
            }
            
            DA_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset) - _GlideRoot.pool.fHalf);
            dataElem = 0;
            DA_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset+4));
            i = gc->tsuDataList[dataElem];
            while (i != GR_DLIST_END) {
              DA_SETF(FARRAY(a, i));
              dataElem++;
              i = gc->tsuDataList[dataElem];
            }
            
            DA_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset) + _GlideRoot.pool.fHalf);
            dataElem = 0;
            DA_SETF(FARRAY(b, gc->state.vData.vertexInfo.offset+4));
            i = gc->tsuDataList[dataElem];
            while (i != GR_DLIST_END) {
              DA_SETF(FARRAY(b, i));
              dataElem++;
              i = gc->tsuDataList[dataElem];
            }
            
            DA_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset) + _GlideRoot.pool.fHalf);
            dataElem = 0;
            DA_SETF(FARRAY(a, gc->state.vData.vertexInfo.offset+4));
            i = gc->tsuDataList[dataElem];
            while (i != GR_DLIST_END) {
              DA_SETF(FARRAY(a, i));
              dataElem++;
            i = gc->tsuDataList[dataElem];
            }
          }
        }

        _GlideRoot.stats.linesDrawn++;
        _GlideRoot.stats.othertrisDrawn+=2;

      all_done:
        ;
      }
      DA_END;
      GR_CHECK_SIZE();
      sCount -= LINES_BUFFER;
    }
  }
  else {

    float oowa, oowb = 0.0f, owa, owb, tmp1, tmp2, fax, fay, fbx, fby;

    while (sCount > 0) {
      FxI32 k, vcount = sCount >= LINES_BUFFER ? LINES_BUFFER : sCount;
      float *a,*b;
      GR_SET_EXPECTED_SIZE((gc->state.vData.vSize << 2) * vcount, vcount);
      DA_BEGIN;
      if (ltype == GR_LINE_STRIP) {
        a = (float *)pointers;
        if (mode) {
          a = *(float **)a;
        }
        oowb = 1.0f / FARRAY(a, gc->state.vData.wInfo.offset);        
      }
      for (k = 0; k < vcount; k++) {
        a = (float *)pointers;
        b = (float *)pointers + stride;
        if (mode) {
          a = *(float **)a;
          b = *(float **)b;
        }
        pointers = (float *)pointers + stride;
        if (ltype == GR_LINES) {
          owa = oowa = 1.0f / FARRAY(a, gc->state.vData.wInfo.offset);        
          pointers = (float *)pointers + stride;
        }
        else
          owa = oowa = oowb;
        owb = oowb = 1.0f / FARRAY(b, gc->state.vData.wInfo.offset);

        fay = tmp1 = FARRAY(a, gc->state.vData.vertexInfo.offset+4)
          *oowa*gc->state.Viewport.hheight+gc->state.Viewport.oy;
        fby = tmp2 = FARRAY(b, gc->state.vData.vertexInfo.offset+4)
          *oowb*gc->state.Viewport.hheight+gc->state.Viewport.oy;        
        
        /*
        ** compute absolute deltas and draw from low Y to high Y
        */
        ADY.f = tmp2 - tmp1;
        if (ADY.i < 0) {
          float *tv;          
          owa = oowb; owb = oowa;
          fay = tmp2;
          fby = tmp1;
          tv = a; a = b; b = tv;
          ADY.i ^= 0x80000000;            /* ady = -ady; */
        }
        fax = FARRAY(a, gc->state.vData.vertexInfo.offset)
          *owa*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
        fbx = FARRAY(b, gc->state.vData.vertexInfo.offset)
          *owb*gc->state.Viewport.hwidth+gc->state.Viewport.ox;
        
        DX.f = fbx - fax;
        DX.i &= 0x7fffffff;               /* abs(adx) */        
        
        /* check for zero-length lines */
        if ((DX.i >= ADY.i) && (DX.i == 0)) {
#ifdef GLIDE_DEBUG
          gc->expected_counter -= (gc->state.vData.vSize << 2 );
          gc->checkCounter -= ((gc->state.vData.vSize+1) << 2 );
#endif
          goto all_done_vp;
        }
    
        DA_CONT(kSetupCullDisable | kSetupStrip, gc->cmdTransportInfo.paramMask,
                0x04UL, vertexParamOffset, SSTCP_PKT3_BDDDDD);
        {        
          /* x major */
          if (DX.i >= ADY.i) {
            DA_SETF(fbx);
            DA_SETF(fby - _GlideRoot.pool.fHalf);
            DA_VP_SETFS(b,oowb);
            
            DA_SETF(fax);
            DA_SETF(fay - _GlideRoot.pool.fHalf);
            DA_VP_SETFS(a,oowa);

            DA_SETF(fbx);
            DA_SETF(fby + _GlideRoot.pool.fHalf);
            DA_VP_SETFS(b,oowb);

            DA_SETF(fax);
            DA_SETF(fay + _GlideRoot.pool.fHalf);
            DA_VP_SETFS(a,oowa);

          } else { /* y major */
            DA_SETF(fbx - _GlideRoot.pool.fHalf);
            DA_SETF(fby);
            DA_VP_SETFS(b,oowb);
            
            DA_SETF(fax - _GlideRoot.pool.fHalf);
            DA_SETF(fay);
            DA_VP_SETFS(a,oowa);
            
            DA_SETF(fbx + _GlideRoot.pool.fHalf);
            DA_SETF(fby);
            DA_VP_SETFS(b,oowb);
            
            DA_SETF(fax + _GlideRoot.pool.fHalf);
            DA_SETF(fay);
            DA_VP_SETFS(a,oowa);
          }
        }

        _GlideRoot.stats.linesDrawn++;
        _GlideRoot.stats.othertrisDrawn+=2;

      all_done_vp:
        ;
      }
      DA_END;
      GR_CHECK_SIZE();
      sCount -= LINES_BUFFER;
    }
  }

#endif

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
_grDrawTriangles_Default(FxI32 mode, FxI32 count, void *pointers)
{
#define FN_NAME "_grDrawTriangles_Default"

  FxI32
    k;
  FxI32 stride;
  float *vPtr;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE(gc->myLevel, "(count = %d, pointers = 0x%x)\n",
                 count, pointers);

  GR_FLUSH_STATE();

#ifdef GLIDE_DEBUG
  GDBG_INFO(110, "%s:  vSize = %d\n", FN_NAME, gc->state.vData.vSize);

  GDBG_INFO(110, "%s:  paramMask = 0x%x\n", FN_NAME, gc->cmdTransportInfo.paramMask);
#endif

  if (mode == 0)
    stride = gc->state.vData.vStride;
  else
    stride = sizeof(float *) / sizeof (float);

#if defined(__MSC__)
  {
    _asm {
      mov    eax, dword ptr count;
       mov    edx, 0aaaaaaabh;
      mul    edx;
      shr    edx, 1;
       mov    eax, dword ptr _GlideRoot.stats.trisProcessed;
      add    eax, edx;
      mov    dword ptr _GlideRoot.stats.trisProcessed,eax;
    }
  }
#else
  _GlideRoot.stats.trisProcessed+=(count/3);
#endif

  if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
    while (count > 0) {
      FxI32 vcount = count >=15 ? 15 : count;
      GR_SET_EXPECTED_SIZE(vcount * gc->state.vData.vSize, 1);
      TRI_STRIP_BEGIN(kSetupStrip, vcount, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD);
      
      for (k = 0; k < vcount; k++) {
        FxI32 i;
        FxU32 dataElem = 0;
        
        vPtr = pointers;
        if (mode)
          vPtr = *(float **)vPtr;
        pointers = (float *)pointers + stride;
        
        i = gc->tsuDataList[dataElem];
        
        TRI_SETF(FARRAY(vPtr, 0));
        TRI_SETF(FARRAY(vPtr, 4));
        while (i != GR_DLIST_END) {
          TRI_SETF(FARRAY(vPtr, i));
          dataElem++;
          i = gc->tsuDataList[dataElem];
        }
      }
      TRI_END;
      GR_CHECK_SIZE();
      count -= 15;
    }
  }
  else {
    /*
     * first cut of clip space coordinate code, no optimization.
     */
    float oow;
    
    while (count > 0) {
      FxI32 vcount = count >= 15 ? 15 : count;
      
      GR_SET_EXPECTED_SIZE(vcount * gc->state.vData.vSize, 1);
      TRI_STRIP_BEGIN(kSetupStrip, vcount, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD);
      
      for (k = 0; k < vcount; k++) {
        vPtr = pointers;
        if (mode)
          vPtr = *(float **)pointers;
        oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset);
        
        /* x, y */
        TRI_SETF(FARRAY(vPtr, 0)
                 *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox);
        TRI_SETF(FARRAY(vPtr, 4)
                 *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy);
        pointers = (float *)pointers + stride;
        
        TRI_VP_SETFS(vPtr,oow);
      }
      TRI_END;
      GR_CHECK_SIZE();
      count -= 15;
    }
  }

  
#undef FN_NAME
} /* _grDrawTriangles */

#define TRIANGLE_VP_SETFS(_s,_oow) \
{ \
  FxI32 i, dataElem=0; \
  i = gc->tsuDataList[dataElem]; \
  if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) { \
    TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
    TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
    TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) { \
    TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOZ) { \
    TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz); \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) { \
    if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) { \
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q0Info.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].t_scale); \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) { \
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q1Info.offset)*_oow); \
    } \
    else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].t_scale); \
  } \
}

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
#ifdef GLIDE_USE_C_TRISETUP
FxI32 FX_CSTYLE
_vptrisetup_cull(const void *va, const void *vb, const void *vc)
{
#define FN_NAME "_vptrisetup_cull"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);
  {
    float oow[3];
    register float oowa;

    oow[0] = 1.0f / FARRAY(va, gc->state.vData.wInfo.offset);
    /*
     * first cut of clip space coordinate code, no optimization.
     */
    

    {
      /* Hmm very dirty it seems as if vPtr first points to va, then vb then
         vc, iow it seems as if this code assumes a certain stack layout! */
      const void **vPtr = &va; 
      int i;
      
      GR_FLUSH_STATE();
      
      GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize, 1);
      TRI_STRIP_BEGIN(kSetupStrip, 3, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD);
      
      oow[1] = 1.0f / FARRAY(vb, gc->state.vData.wInfo.offset);
      oow[2] = 1.0f / FARRAY(vc, gc->state.vData.wInfo.offset);
      for (i = 0; i < 3; i++) {
        va = *vPtr++;
        oowa = oow[i];
        /* x, y */
        TRI_SETF(FARRAY(va, 0)
                 *oowa*gc->state.Viewport.hwidth + gc->state.Viewport.ox);
        TRI_SETF(FARRAY(va, 4)
                 *oowa*gc->state.Viewport.hheight + gc->state.Viewport.oy);
        
        TRI_VP_SETFS(va,oowa);
      }
      TRI_END;
      GR_CHECK_SIZE();
    }
  }

  _GlideRoot.stats.trisProcessed++;
  return 1;

#undef FN_NAME
} /* _grDrawTriangles */
#endif


