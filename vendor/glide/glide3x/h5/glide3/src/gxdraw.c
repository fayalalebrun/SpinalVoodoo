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
 ** $Log:
 **  3    3dfx      1.0.1.0.1.0 10/11/00 Brent           Forced check in to enforce
 **       branching.
 **  2    3dfx      1.0.1.0     06/20/00 Joseph Kain     Changes to support the
 **       Napalm Glide open source release.  Changes include cleaned up offensive
 **       comments and new legal headers.
 **  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
 ** $
** 
** 15    8/16/99 11:18a Adamb
** Merged in V3_OEM_100 fixes
** 
** 14    7/14/99 9:39a Atai
** direct register write for glide3x
** test04 can do 4 sample aa (2 chips)
** 
** 13    6/03/99 12:13p Kcd
** Modified to allow me to use parts of this file when assembly triangle
** setup is being used.
** 
** 12    3/02/99 2:08p Peter
** 2kx2k check for c 'setup'
** 
** 11    2/18/99 4:12p Kcd
** Fixed (lack of) return value from C triangle setup.
** Fixed non-portable array initializer.
** 
** 10    1/06/99 11:30a Peter
** cleanup trinalge dispatch code
** 
** 9     12/03/98 11:26p Dow
** 
** 8     7/01/98 8:40a Jdt
** remvoed gc arg from trisetup funcs
** 
** 7     8/03/98 6:42a Jdt
** move stats, pool.ftemp* into GC from global location
** 
** 6     7/16/98 8:19a Jdt
** fxcmd.h
** 
** 4     4/22/98 4:57p Peter
** glide2x merge
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
 * 60    12/01/97 6:13p Peter
 * non-packet3 tsu triangles ooz vs z
 * 
 * 59    11/21/97 3:20p Peter
 * direct writes tsu registers
 * 
 * 58    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 57    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 56    11/12/97 9:54p Peter
 * fixed all the effage from new config
 * 
 * 55    11/03/97 4:38p Peter
 * yapc fix
 * 
 * 54    11/01/97 10:01a Peter
 * tri dispatch stuff
 * 
 * 53    10/29/97 2:45p Peter
 * C version of Taco's packing code
 * 
 * 52    10/27/97 5:59p Peter
 * removed some debugging code
 * 
 * 51    10/21/97 3:22p Peter
 * hand pack rgb
 * 
 * 50    10/19/97 12:51p Peter
 * no tsu happiness
 * 
 * 49    10/19/97 10:59a Peter
 * fixed p1 tsu writes
 * 
 * 48    10/17/97 3:15p Peter
 * removed unused addr field from datalist
 * 
 * 47    10/17/97 10:15a Peter
 * packed rgb state cleanup
 * 
 * 46    10/16/97 5:33p Peter
 * argb != rgba
 * 
 * 45    10/16/97 3:40p Peter
 * packed rgb
 * 
 * 44    10/16/97 10:31a Peter
 * fixed hoopti tsu-subtractor unsorted
 * 
 * 43    10/15/97 5:53p Peter
 * hoopti tri compare code
 * 
 * 42    10/10/97 4:33p Peter
 * non-packet3 tsu triangles
 * 
 * 41    10/08/97 5:19p Peter
 * optinally clamp only texture params
 * 
 * 40    10/08/97 11:32a Peter
 * pre-computed packet headers for packet 3
 * 
 * 39    9/20/97 4:42p Peter
 * tri_setf fixup/big fifo
 * 
 * 38    9/16/97 2:50p Peter
 * fixed watcom unhappiness w/ static initializers
 * 
 * 37    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 36    9/10/97 10:13p Peter
 * fifo logic from GaryT, non-normalized fp first cut
 * 
 * 35    9/03/97 2:11p Peter
 * start gdbg_info cleanup, fixed zero area no-tsu triangle effage
 * 
 * 34    9/01/97 3:19p Peter
 * no-tsu w from vertex not tmuvtx
 * 
 * 33    8/31/97 4:06p Peter
 * no tsu fix
 * 
 * 32    8/31/97 12:04p Peter
 * hacked no-tsu code
 * 
 * 31    7/25/97 11:40a Peter
 * removed dHalf, change field name to match real use for cvg
 * 
 * 30    6/30/97 3:22p Peter
 * cmd fifo sanity
 * 
 * 29    6/24/97 4:02p Peter
 * proper cmd fifo placement
 * 
 * 28    6/23/97 4:43p Peter
 * cleaned up #defines etc for a nicer tree
 **
 */

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"
#include "fxcmd.h"

#ifdef GDBG_INFO_ON
/* Some debugging information */
static char *indexNames[] = {  
  "GR_VERTEX_X_OFFSET",         /* 0 */
  "GR_VERTEX_Y_OFFSET",         /* 1 */
  "GR_VERTEX_Z_OFFSET",         /* 2 */
  "GR_VERTEX_R_OFFSET",         /* 3 */
  "GR_VERTEX_G_OFFSET",         /* 4 */
  "GR_VERTEX_B_OFFSET",         /* 5 */
  "GR_VERTEX_OOZ_OFFSET",       /* 6 */
  "GR_VERTEX_A_OFFSET",         /* 7 */
  "GR_VERTEX_OOW_OFFSET",       /* 8 */
  "GR_VERTEX_SOW_TMU0_OFFSET",  /* 9 */
  "GR_VERTEX_TOW_TMU0_OFFSET",  /* 10 */
  "GR_VERTEX_OOW_TMU0_OFFSET",  /* 11 */
  "GR_VERTEX_SOW_TMU1_OFFSET",  /* 12 */
  "GR_VERTEX_TOW_TMU1_OFFSET",  /* 13 */
  "GR_VERTEX_OOW_TMU1_OFFSET"   /* 14 */
};  
#endif /* GDBG_INFO_ON */

#if GLIDE_USE_C_TRISETUP

static FxI32
_grTriCull(const void* a, const void* b, const void* c)
{
#define FN_NAME "_grCullTri"
  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME, 85);
  GDBG_INFO_MORE(gc->myLevel, 
                 "(0x%X, 0x%X, 0x%X)\n", 
                 a, b, c);

  {
    const FxI32 
      xindex = (gc->state.vData.vertexInfo.offset >> 2),
      yindex = xindex + 1;
    const float 
      *fa = (const float*)a + xindex,
      *fb = (const float*)b + xindex,
      *fc = (const float*)c + xindex;
    const float 
      dxAB = fa[xindex] - fb[xindex],
      dxBC = fb[xindex] - fc[xindex], 
      dyAB = fa[yindex] - fb[yindex],
      dyBC = fb[yindex] - fc[yindex],
      area = dxAB * dyBC - dxBC * dyAB;
  
    {
      union { float f; FxI32 i; } j;
      const FxU32 culltest = (gc->state.cull_mode << 31UL);
      j.f = area;
    
      /* Zero-area triangles are BAD!! */
      if ((j.i & 0x7FFFFFFF) == 0) {
        GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                  (fa[0]), (fa[1]), 
                  (fb[0]), (fb[1]), 
                  (fc[0]), (fc[1]), 
                  area, gc->state.cull_mode, culltest);
      
        return 0;
      }
    
      /* Backface culling, use sign bit as test */
      if ((gc->state.cull_mode != GR_CULL_DISABLE) && (((FxI32)(j.i ^ culltest)) >= 0)) {
        GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                  (fa[0]), (fa[1]), 
                  (fb[0]), (fb[1]), 
                  (fc[0]), (fc[1]), 
                  area, gc->state.cull_mode, culltest);
      
        return -1;
      }
    }

    return 1;
  }

  GR_END();
#undef FN_NAME
}

static FxI32 
internal_trisetup(const char* FN_NAME, 
                  const FxBool cullP, const FxBool validStateP,
                  const void* a, const void* b, const void* c)
{
#ifdef FAST_C_CLIP
  AMG_GR_BEGIN_NOFIFOCHECK();
#else
  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME, 85);
#endif
  GDBG_INFO_MORE(gc->myLevel, 
                 "(0x%X, 0x%X, 0x%X)\n", 
                 a, b, c);

  /* Check to see if the state has to be flushed */
  if (!validStateP) GR_FLUSH_STATE();

  /* Pass the current culling mode? */
  if (cullP) {
    const FxI32
      cullVal = _grTriCull(a, b, c);

    if (cullVal <= 0) return cullVal;
  }


  /* Validate parameter coordinates */
#if defined(GLIDE_SANITY_ASSERT)
  {
    const FxI32 
      xindex = (gc->state.vData.vertexInfo.offset >> 2),
      yindex = xindex + 1;
    const float 
      *fa = (const float*)a + xindex,
      *fb = (const float*)b + xindex,
      *fc = (const float*)c + xindex;
    const float 
      aX = fa[xindex],
      aY = fa[yindex],
      bX = fb[xindex],
      bY = fb[yindex],
      cX = fc[xindex],
      cY = fc[yindex];

#define kDimThreshX 2048.0f
#define kDimThreshY 2048.0f
    GR_ASSERT((fabs(aX) < kDimThreshX) && (fabs(aY) < kDimThreshY));
    GR_ASSERT((fabs(bX) < kDimThreshX) && (fabs(bY) < kDimThreshY));
    GR_ASSERT((fabs(cX) < kDimThreshX) && (fabs(cY) < kDimThreshY));
#undef kDimThreshX
#undef kDimThreshY
  }
#endif

  /* Send triangle parameters */

#if GLIDE_HW_TRI_SETUP && GLIDE_PACKET3_TRI_SETUP
#ifdef FAST_C_CLIP
  {
    FxU32 dataElem,i,i2,nextfifo;
    unsigned long *casta,*castb,*castc,lenght,Loop;

    FxU32* tPackPtr; 
    FxU32 packetVal;
    AMG_GR_SET_EXPECTED_SIZE(gc->curTriSize, 1);	
    AMG_TRI_STRIP_BEGIN(tPackPtr,packetVal);

    i=gc->curVertexParam;
    Loop=i-2;
    i2=i+i;
    lenght=i-2;
    nextfifo=6+lenght+lenght;				// lenght of vertex params *2
    
    AMG_TRISETXYNOADD(((unsigned long*)a),0);
    AMG_TRISETXYNOADD(((unsigned long*)b),i);
    AMG_TRISETXYNOADD(((unsigned long*)c),i2);
    
    dataElem=0;
    i+=2;
    i2+=2;
    casta=(unsigned long*)a;
    castb=(unsigned long*)b;
    castc=(unsigned long*)c;
    
    while(Loop!=0) {
      AMG_TRISETPARAMNOADD(casta[gc->tsuDataListByte[dataElem]],2);
      AMG_TRISETPARAMNOADD(castb[gc->tsuDataListByte[dataElem]],i);
      AMG_TRISETPARAMNOADD(castc[gc->tsuDataListByte[dataElem]],i2);
      AMG_TRIFIFOADD
        dataElem++;
      Loop--;
    }
    AMG_TRIFIFOADDVALUE(nextfifo)
    AMG_TRI_END(tPackPtr)
  }
#else /* !FAST_C_CLIP */
  {
    FxU32
      dataElem,
      i;

    GR_SET_EXPECTED_SIZE(gc->curTriSize, 1);
    TRI_STRIP_BEGIN(kSetupStrip, 3, gc->curVertexSize, SSTCP_PKT3_BDDBDD);
    {
      TRI_SETF(FARRAY(a, 0));
      TRI_SETF(FARRAY(a, sizeof(float)));
      dataElem = 0;
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(a, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }

      TRI_SETF(FARRAY(b, 0));
      TRI_SETF(FARRAY(b, sizeof(float)));
      dataElem = 0;
      i = gc->tsuDataList[dataElem];
      while (i != GR_DLIST_END) {
        TRI_SETF(FARRAY(b, i));
        dataElem++;
        i = gc->tsuDataList[dataElem];
      }

      TRI_SETF(FARRAY(c, 0));
      TRI_SETF(FARRAY(c, sizeof(float)));
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
  }
#endif /* !FAST_C_CLIP */
#else
  {
    GR_DCL_HW;
    int vectorIndex;
    FxU32 sMode = (gc->cmdTransportInfo.paramMask >> SSTCP_PKT3_PMASK_SHIFT);
    FxU32 paramMask = (sMode & 0xFF);
    FxU32 paramCount;
    const float* vectorArray[3];

    vectorArray[0] = (const float *)a;
    vectorArray[1] = (const float *)b;
    vectorArray[2] = (const float *)c;
    
    /* Convert packet 3 paramMask into sMode format */
    sMode = (paramMask | ((sMode & 0xF000) << 4));
    
    {
      const FxBool hasColor = ((sMode & 0x01) != 0);
      const FxBool hasAlpha = ((sMode & 0x02) != 0);
      const FxBool hasZ = ((sMode & 0x04) != 0);
      const FxBool hasWb = ((sMode & 0x08) != 0);
      const FxBool hasW0 = ((sMode & 0x10) != 0);
      const FxBool hasST0 = ((sMode & 0x20) != 0);
      const FxBool hasW1 = ((sMode & 0x40) != 0);
      const FxBool hasST1 = ((sMode & 0x80) != 0);
      
      /* We always send vertex XY */
      paramCount = 2;
      paramMask = 0x03;
      
      /* Build parameter data for reg group packet */
#if GLIDE_PACKED_RGB
      if (hasColor || hasAlpha) {
        paramCount += 1;
        paramMask |= 0x04;
      }
#else /* !GLIDE_PACKED_RGB */
      if (hasColor) {
        paramCount += 3;
        paramMask |= 0x38;
      }
      if (hasAlpha) {
        paramCount += 1;
        paramMask |= 0x40;
      }
#endif /* !GLIDE_PACKED_RGB */
      
      if (hasZ) {
        paramCount += 1;
        paramMask |= 0x80;
      }
      if (hasWb) {
        paramCount += 1;
        paramMask |= 0x100;
      }
      if (hasW0) {
        paramCount += 1;
        paramMask |= 0x200;
      }
      if (hasST0) {
        paramCount += 2;
        paramMask |= 0xC00;
      }
      if (hasW1) {
        paramCount += 1;
        paramMask |= 0x1000;
      }
      if (hasST1) {
        paramCount += 2;
        paramMask |= 0x6000;
      }
      
      /* Set mode once for teh whole triangle */
      GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
      GR_SET(BROADCAST_ID, hw, sSetupMode, sMode);
      GR_CHECK_SIZE();
      
      for(vectorIndex = 0; vectorIndex < sizeof(vectorArray) / sizeof(float*); vectorIndex++) {
        FxU32
          dataElem,
          i;
        const float* a = (const float *)vectorArray[vectorIndex];
        
        REG_GROUP_BEGIN(BROADCAST_ID, sVx, paramCount, paramMask);
        {
          REG_GROUP_SETF(hw, sVx, FARRAY(a, 0));
          REG_GROUP_SETF(hw, sVy, FARRAY(a, sizeof(float)));
          
          dataElem = 0;
          i = gc->tsuDataList[dataElem];

          if (hasColor) {
            REG_GROUP_SETF(hw, sRed, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
            REG_GROUP_SETF(hw, sGreen, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
            REG_GROUP_SETF(hw, sBlue, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
          }
          if (hasAlpha) {
            REG_GROUP_SETF(hw, sAlpha, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
          }
          
          if (hasZ) {
            REG_GROUP_SETF(hw, sVz, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
          }
          if (hasWb) {
            REG_GROUP_SETF(hw, sOowfbi, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
          }
          
          /* TMU0 */
          if (hasW0) {
            REG_GROUP_SETF(hw, sOow0, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
          }
          if (hasST0) {
            REG_GROUP_SETF(hw, sSow0, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
            REG_GROUP_SETF(hw, sTow0, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
          }
          
          /* TMU1 */
          if (hasW1) {
            REG_GROUP_SETF(hw, sOow1, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
          }
          if (hasST1) {
            REG_GROUP_SETF(hw, sSow1, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
            REG_GROUP_SETF(hw, sTow1, FARRAY(a, i));
            dataElem++;
            i = gc->tsuDataList[dataElem];
          }
        }
        REG_GROUP_END();
        
        GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
        if (vectorIndex == 0) {
          GR_SET(BROADCAST_ID, hw, sBeginTriCMD, 0);
        } else {
          GR_SET(BROADCAST_ID, hw, sDrawTriCMD, 0);
        }
        GR_CHECK_SIZE();
      }
    }
  }
#endif

  GR_END();

  return FXTRUE;
}

FxI32 FX_CALL 
_trisetup_Default_win_cull_invalid(const void* a, const void* b, const void* c)
{
#define FN_NAME "_trisetup_Default_win_cull_invalid"

  return internal_trisetup(FN_NAME,
                           FXTRUE, FXFALSE,
                           a, b, c);

#undef FN_NAME
}

FxI32 FX_CALL 
_trisetup_Default_win_cull_valid(const void* a, const void* b, const void* c)
{
#define FN_NAME "_trisetup_Default_win_cull_valid"

  return internal_trisetup(FN_NAME,
                           FXTRUE, FXTRUE,
                           a, b, c);

#undef FN_NAME
}

FxI32 FX_CALL 
_trisetup_Default_win_nocull_invalid(const void* a, const void* b, const void* c)
{
#define FN_NAME "_trisetup_Default_win_nocull_invalid"

  return internal_trisetup(FN_NAME,
                           FXFALSE, FXFALSE,
                           a, b, c);

#undef FN_NAME
}

FxI32 FX_CALL 
_trisetup_Default_win_nocull_valid(const void* a, const void* b, const void* c)
{
#define FN_NAME "_trisetup_Default_win_nocull_valid"

  return internal_trisetup(FN_NAME,
                           FXFALSE, FXTRUE,
                           a, b, c);

#undef FN_NAME
}

#endif /* GLIDE_USE_C_TRISETUP */

FxI32 FX_CALL 
_vptrisetup_cull(const void* a, const void* b, const void* c)
{
#define FN_NAME "_vptrisetup_cull"
  float *vlist[3];

  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME, 85);
  GDBG_INFO_MORE(gc->myLevel, 
                 "(0x%X, 0x%X, 0x%X)\n", 
                 a, b, c);

  vlist[0] = (float *)a; vlist[1] = (float *)b; vlist[2] = (float *)c;
  (*gc->archDispatchProcs.drawTrianglesProc)(GR_VTX_PTR_ARRAY, 3, vlist);
  
  GR_END();

  return 1;
#undef FN_NAME
}
