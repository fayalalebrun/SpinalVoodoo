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
 ** Revision 1.2.6.9  2006/01/16 21:22:43  jwrdegoede
 ** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
 **
 ** Revision 1.2.6.8  2005/06/09 18:32:28  jwrdegoede
 ** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
 **
 ** Revision 1.2.6.7  2005/05/25 08:56:23  jwrdegoede
 ** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
 **
 ** Revision 1.2.6.6  2005/05/07 08:40:29  jwrdegoede
 ** lvalue cast fixes for gcc4
 **
 ** Revision 1.2.6.5  2004/10/07 07:16:38  dborca
 ** corrected grDrawTriangle on MSVC
 **
 ** Revision 1.2.6.4  2004/10/05 14:47:15  dborca
 ** conditional compilation a bit more sane
 **
 ** Revision 1.2.6.3  2004/10/04 09:26:31  dborca
 ** DOS/OpenWatcom support
 **
 ** Revision 1.2.6.2  2004/09/27 08:58:15  dborca
 ** type-punning warning
 **
 ** Revision 1.2.6.1  2003/05/05 07:12:46  dborca
 ** no message
 **
 ** Revision 1.2  2000/02/15 22:35:58  joseph
 ** Changes to support FreeBSD (patch submitted by Doug Rabson)
 **
 ** Revision 1.1.1.1  1999/11/24 21:44:56  joseph
 ** Initial checkin for SourceForge
 **
** 
** 7     4/24/99 11:17a Atai
** check if we have valid gc and lostcontext pointer
** 
** 6     4/22/99 4:03p Dow
** Alt-Tab on NT
** 
** 31    4/22/99 3:53p Dow
** Alt-Tab on NT
** 
** 30    4/16/99 2:48p Kcd
** Hack to allow different SETF macro, SETF_FIFO fixes.
** 
** 29    1/06/99 11:30a Peter
** cleanup trinalge dispatch code
** 
** 28    1/05/99 4:27p Peter
** first pass at fixing inline asssembly for triangle dispatch.
** 
** 27    10/16/98 1:24p Peter
** c trisetup fixes
** 
** 26    10/12/98 9:51a Peter
** dynamic 3DNow!(tm)
** 
** 25    9/24/98 11:17a Dow
** AMD 3DNow! (tm) mods
** 
** 24    9/08/98 6:05p Atai
** fixed clip coords line routine
** 
** 23    8/31/98 10:33a Peter
** asm w/ debugging
** 
** 22    8/30/98 1:34p Dow
** State & other optimizations
** 
** 21    7/01/98 8:40a Jdt
** removed gc arg from trisetup funcs
** 
** 20    8/03/98 6:40a Jdt
** We now pass GC on the stack to trisetup functions, moved ftemp, stats
** into gc from old global location.
** 
** 19    7/18/98 12:26a Jdt
** Removed ColorCombineDelta0 Mode.
** 
** 18    7/16/98 8:17a Jdt
** fxcmd.h
** 
** 17    6/19/98 1:00p Atai
** fixed tri stats
** 
** 16    6/18/98 6:38p Atai
** fixed line routine for grDrawVertexArrayContiguous
** 
** 15    6/11/98 6:04p Atai
** added aa case for OGL
** 
** 14    6/09/98 2:34p Peter
** point happiness
** 
** 13    6/09/98 11:59a Atai
** 1. update glide header
** 2. fix cull mode
** 3. fix tri stats
** 
** 12    5/29/98 2:09p Atai
** remove polygon code
** 
** 11    5/19/98 1:04p Peter
** point fun
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
 */

#if SET_BSWAP
#define SLOW_SETF 1
#endif

#include <memory.h>

#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

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

  if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_POINTS_MASK)
	  _grAADrawPoints(GR_VTX_PTR_ARRAY, 1, (void *)&p);
  else
	  _grDrawPoints(GR_VTX_PTR_ARRAY, 1, (void *)&p);

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
  void *vertices[2]; /**** FIXME: this needs to be const! ****/

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 91);
  GDBG_INFO_MORE(gc->myLevel, "(a = 0x%x, b = 0x%x)\n", a, b);

  vertices[0] = (void *)a; vertices[1] = (void *)b; /* FIXME! */

  if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_LINES_MASK)
    _grAADrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, 2, vertices);
  else
    _grDrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, 2, vertices);
#undef FN_NAME
} /* grDrawLine */

/*---------------------------------------------------------------------------
 ** grDrawTriangle
 */
#if defined(__MINGW32__) && !(GLIDE_USE_C_TRISETUP || GLIDE_DEBUG)
#define HAVE_XDRAWTRI_ASM
#endif
#ifndef HAVE_XDRAWTRI_ASM	/* grDrawTriangle() not in asm */
#if defined(_MSC_VER) && !defined(GLIDE_DEBUG) && !(GLIDE_USE_C_TRISETUP)
__declspec( naked )
#endif
GR_ENTRY(grDrawTriangle, void, (const void *a, const void *b, const void *c))
{
#define FN_NAME "grDrawTriangle"

  GR_CHECK_F(FN_NAME, !a || !b || !c, "NULL pointer passed");

#if GLIDE_USE_C_TRISETUP || GLIDE_DEBUG
  {
    GR_BEGIN_NOFIFOCHECK("grDrawTriangle",92);
    GDBG_INFO_MORE(gc->myLevel,"(0x%x,0x%x,0x%x)\n",a,b,c);
    TRISETUP(a, b, c);
#if GLIDE_DEBUG
    /* HackAlert: Nuke the fifo ptr checking stuff here if we're just
     * debugging the asm tri code.
     */
    gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr;
    gc->checkCounter = 0;
#endif /* GLIDE_DEBUG */
    GR_END();
  }

#elif defined(_MSC_VER)
  {
    __asm {
      mov eax, DWORD PTR fs:[WNT_TEB_PTR];
      add eax, DWORD PTR _GlideRoot.tlsOffset;
      mov edx, [eax];
      test edx, edx;
      je   lostContext;
      mov eax, [edx + kLostContextOffset];
      test eax, eax;
      je   lostContext;
      mov eax, [eax];
      test eax, 1;
      jnz  lostContext;
      mov eax, [edx + kTriProcOffset];
      jmp eax;
    }
    lostContext: ; /* <-- my, that's odd, but MSVC was insistent  */
  }

#elif defined( __linux__ ) || defined(__FreeBSD__) || (GLIDE_PLATFORM & GLIDE_OS_DOS32)
  {
    GR_BEGIN_NOFIFOCHECK("grDrawTriangle",92);
    TRISETUP(a, b, c);
    GR_END();
  }

#else
#error "Write triangle proc dispatch for this compiler"
#endif /* Triangle proc dispatch routine */
#undef FN_NAME
} /* grDrawTriangle */
#endif /* HAVE_XDRAWTRI_ASM */


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
  SET_FIFO(*packetPtr++, packetVal);

#define DA_SETF(__val) \
  SETF_FIFO(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxFloat))

#define DA_SET(__val) \
  SET_FIFO(*packetPtr++, (__val)); \
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

  GR_FLUSH_STATE();

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
          gc->pool.temp1.f = FARRAY(vPtr, gc->state.vData.vertexInfo.offset) + bias;
          gc->pool.temp2.f = FARRAY(vPtr, gc->state.vData.vertexInfo.offset + 4) + bias;

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
          /* [dBorca] Hack alert: bad type-punning with gcc -O2 */
          x = ((gc->pool.temp1.u & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
               (0x01UL << (22UL - kNumMantissaBits)));
          y = ((gc->pool.temp2.u & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
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
          gc->pool.temp1.f = (FARRAY(vPtr, gc->state.vData.vertexInfo.offset) * 
                                    oow * 
                                    gc->state.Viewport.hwidth + 
                                    gc->state.Viewport.ox + 
                                    bias);
          gc->pool.temp2.f = (FARRAY(vPtr, gc->state.vData.vertexInfo.offset + 4) * 
                                    oow *
                                    gc->state.Viewport.hheight + 
                                    gc->state.Viewport.oy +
                                    bias);

          /* Mask off the real fractional bits from the mantissa */
          x = ((gc->pool.temp1.u & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
               (0x01UL << (22UL - kNumMantissaBits)));
          y = ((gc->pool.temp2.u & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
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

  gc->stats.pointsDrawn+=count;
  gc->stats.othertrisDrawn+=(count<<1);

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
  FxI32 stride, sCount;
  FxU32 vertexParamOffset;

#define  DX gc->pool.temp1
#define ADY gc->pool.temp2

  GR_BEGIN_NOFIFOCHECK("_grDrawLineStrip", 91);

  GDBG_INFO_MORE(gc->myLevel, "(count = %d, pointers = 0x%x)\n",
                 count, pointers);
  
  vertexParamOffset = gc->state.vData.vSize;

  GR_FLUSH_STATE();

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

        gc->stats.linesDrawn++;
        gc->stats.othertrisDrawn+=2;

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
        if ((DX.i >= ADY.i) && (DX.i == 0)) goto all_done_vp;

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

        gc->stats.linesDrawn++;
        gc->stats.othertrisDrawn+=2;

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

  gc->stats.trisProcessed+=(count/3);

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
