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
 ** Revision 1.1.1.1.8.2  2006/01/16 21:22:41  jwrdegoede
 ** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
 **
 ** Revision 1.1.1.1.8.1  2005/06/17 14:18:49  jwrdegoede
 ** Fix few warnings when USE_X86=1 is not set
 **
 ** Revision 1.1.1.1  1999/12/07 21:42:34  joseph
 ** Initial checkin into SourceForge.
 **
** 
** 1     10/08/98 11:30a Brent
** 
** 5     7/24/98 1:41p Hohn
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
 * fixed all the muckage from new config
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
 * start gdbg_info cleanup, fixed zero area no-tsu triangle muckage
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
  "GR_VERTEX_OOW_TMU1_OFFSET"	/* 14 */
};  
#endif

/*
 **  _trisetup_nogradients
 **
 **  This routine does all the setup needed for drawing a triangle.  It
 **  is intended to be an exact specification for the mechanisim used
 **  to pass vertices to the assembly language triangle setup code, and
 **  as such has no optimizations at all.  Whenever a 'shortcut'
 **  routine to draw triangles (such as for antialiasing, fast lines,
 **  fast spans, polygons, etc) is needed, this code should be used as
 **  the starting point.
 **
 */
GR_DDFUNC(_trisetup_nogradients,
          FxI32,
          (const void *va, const void *vb, const void *vc))
{
#define FN_NAME "_trisetup_nogradients"
  GR_DCL_GC;
  const FxI32 xindex = (gc->state.vData.vertexInfo.offset >> 2);
  const FxI32 yindex = xindex + 1;

  _GlideRoot.stats.trisProcessed++;

#if GLIDE_TRI_CULLING
  {
    const float *fa = (const float *)va + xindex;
    const float *fb = (const float *)vb + xindex;
    const float *fc = (const float *)vc + xindex;
    float dxAB, dxBC, dyAB, dyBC;
    
    /* Compute Area */
    dxAB = fa[xindex] - fb[xindex];
    dxBC = fb[xindex] - fc[xindex];
    
    dyAB = fa[yindex] - fb[yindex];
    dyBC = fb[yindex] - fc[yindex];
    
    /* Stash the area in the float pool for easy access */
    _GlideRoot.pool.temp1.f = dxAB * dyBC - dxBC * dyAB;
    
#define FloatVal(__f) (((__f) < 786432.875) ? (__f) : ((__f) - 786432.875))
    {
      const FxU32 culltest = (gc->state.cull_mode << 31UL);
      
      /* Zero-area triangles are BAD!! */
      if ((_GlideRoot.pool.temp1.i & 0x7FFFFFFF) == 0) {
        GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                  FloatVal(fa[0]), FloatVal(fa[1]), 
                  FloatVal(fb[0]), FloatVal(fb[1]), 
                  FloatVal(fc[0]), FloatVal(fc[1]), 
                  _GlideRoot.pool.temp1.f, gc->state.cull_mode, culltest);
        
        return 0;
      }
      
      /* Backface culling, use sign bit as test */
      if ((gc->state.cull_mode != GR_CULL_DISABLE) &&
          (((FxI32)(_GlideRoot.pool.temp1.i ^ culltest)) >= 0)) {
        GDBG_INFO(291, FN_NAME": Culling (%g %g) (%g %g) (%g %g) : (%g : 0x%X : 0x%X)\n",
                  FloatVal(fa[0]), FloatVal(fa[1]), 
                  FloatVal(fb[0]), FloatVal(fb[1]), 
                  FloatVal(fc[0]), FloatVal(fc[1]), 
                  _GlideRoot.pool.temp1.f, gc->state.cull_mode, culltest);
        
        return -1;
      }
    }
  }
#endif /* GLIDE_TRI_CULLING */

  /* Stuff for using hw tsu */
#if GLIDE_HW_TRI_SETUP
  {
    const float* vectorArray[3];

    /* Load up the real vertices */
    vectorArray[0] = (const float *)va + xindex;
    vectorArray[1] = (const float *)vb + xindex;
    vectorArray[2] = (const float *)vc + xindex;

#if GLIDE_PACKET3_TRI_SETUP
    GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize, 1);
    TRI_BEGIN();
    {
      int vectorIndex;

      for(vectorIndex = 0; vectorIndex < ((int)sizeof(vectorArray) / (int)sizeof(float*)); vectorIndex++) {
        const float* const vector = vectorArray[vectorIndex];
      
        /* Triangle vertex coordinages (x, y) */
        TRI_SETF(vector[xindex]);
        TRI_SETF(vector[yindex]);
      
        /* Other triangle parameters */
        {
          const int* dataList = gc->tsuDataList;

#if GLIDE_PACKED_RGB
          {
            FxBool doColorP = FXFALSE;
            FxU32 packedColor = 0x00;
            
            if (*dataList == (GR_VERTEX_R_OFFSET << 2)) {
              packedColor = (RGBA_COMP_CLAMP(FARRAY(vector, (GR_VERTEX_B_OFFSET << 2)), B) | 
                             RGBA_COMP_CLAMP(FARRAY(vector, (GR_VERTEX_G_OFFSET << 2)), G) |
                             RGBA_COMP_CLAMP(FARRAY(vector, (GR_VERTEX_R_OFFSET << 2)), R));
              dataList++;
              doColorP = FXTRUE;
            }

            if (*dataList == (GR_VERTEX_A_OFFSET << 2)) {
              packedColor |= RGBA_COMP_CLAMP(FARRAY(vector, (GR_VERTEX_A_OFFSET << 2)), A);
              dataList++;
              doColorP = FXTRUE;
            }
            
            if (doColorP) TRI_SET(packedColor);
          }
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
#else /* !GLIDE_PACKET3_TRI_SETUP */
    {
      GR_DCL_HW;
      int vectorIndex;
      FxU32 sMode = (gc->cmdTransportInfo.paramMask >> SSTCP_PKT3_PMASK_SHIFT);
      FxU32 paramMask = (sMode & 0xFF);
      FxU32 paramCount;

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
          const GrVertex* curVertex = (const GrVertex*)vectorArray[vectorIndex];
          
          REG_GROUP_BEGIN(BROADCAST_ID, sVx, paramCount, paramMask);
          {
            REG_GROUP_SETF(hw, sVx, curVertex->x);
            REG_GROUP_SETF(hw, sVy, curVertex->y);

#if GLIDE_PACKED_RGB
            if (hasColor || hasAlpha) {
              FxU32 packedVal = 0x00;

              if (hasColor) packedVal = ((RGBA_COMP_CLAMP(curVertex->b) << 0UL) |
                                         (RGBA_COMP_CLAMP(curVertex->g) << 8UL) |
                                         (RGBA_COMP_CLAMP(curVertex->r) << 16UL));
              if (hasAlpha) packedVal |= (RGBA_COMP_CLAMP(curVertex->a) << 24UL);
              
              REG_GROUP_SET(hw, sARGB, packedVal);
            }
#else /* !GLIDE_PACKED_RGB */
            if (hasColor) {
              REG_GROUP_SETF(hw, sRed, curVertex->r);
              REG_GROUP_SETF(hw, sGreen, curVertex->g);
              REG_GROUP_SETF(hw, sBlue, curVertex->b);
            }
            if (hasAlpha) REG_GROUP_SETF(hw, sAlpha, curVertex->a);
#endif /* !GLIDE_PACKED_RGB */

            if (hasZ) REG_GROUP_SETF(hw, sVz, curVertex->ooz);
            if (hasWb) REG_GROUP_SETF(hw, sOowfbi, curVertex->oow);

            /* TMU0 */
            if (hasW0) REG_GROUP_SETF(hw, sOow0, curVertex->tmuvtx[0].oow);
            if (hasST0) {
              REG_GROUP_SETF(hw, sSow0, curVertex->tmuvtx[0].sow);
              REG_GROUP_SETF(hw, sTow0, curVertex->tmuvtx[0].tow);
            }

            /* TMU1 */
            if (hasW1) REG_GROUP_SETF(hw, sOow1, curVertex->tmuvtx[1].oow);
            if (hasST1) {
              REG_GROUP_SETF(hw, sSow1, curVertex->tmuvtx[1].sow);
              REG_GROUP_SETF(hw, sTow1, curVertex->tmuvtx[1].tow);
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
#endif /* !GLIDE_PACKET3_TRI_SETUP */

    goto __triDrawn;
  }
#endif /* GLIDE_HW_TRI_SETUP */

#if !GLIDE_HW_TRI_SETUP
#if !defined(GLIDE_TRI_CULLING) || !GLIDE_TRI_CULLING
#error "GLIDE_TRI_CULLING must be enabled to sort vertices"
#endif

  {
    GR_DCL_HW;

    TRI_NO_TSU_BEGIN(FXTRUE);
    {
      const float ooa = _GlideRoot.pool.f1 / _GlideRoot.pool.ftemp1;
      volatile FxU32* hwAddr = &hw->FvA.x;
            
      /* Divide the deltas by the area for gradient calculation. */
      dxBC *= ooa;
      dyAB *= ooa;
      dxAB *= ooa;
      dyBC *= ooa;
	    
      GDBG_INFO(85, FN_NAME": No-TSU Triangle area: (%g %g) : (%g %g %g %g)\n",
                _GlideRoot.pool.ftemp1, ooa,
                dxAB, dxBC, dyAB, dyBC);
      
      /* write out X & Y for vertex A */  
      TRI_NO_TSU_SETF(hwAddr++, fa[GR_VERTEX_X_OFFSET]);
      TRI_NO_TSU_SETF(hwAddr++, fa[GR_VERTEX_Y_OFFSET]);

      /* write out X & Y for vertex B */
      TRI_NO_TSU_SETF(hwAddr++, fb[GR_VERTEX_X_OFFSET]);
      TRI_NO_TSU_SETF(hwAddr++, fb[GR_VERTEX_Y_OFFSET]);  
  
      /* write out X & Y for vertex C */
      TRI_NO_TSU_SETF(hwAddr++, fc[GR_VERTEX_X_OFFSET]);
      TRI_NO_TSU_SETF(hwAddr++, fc[GR_VERTEX_Y_OFFSET]);

      /* Send down the starting parameter values, these come from
       * the sorted vertex A.
       */
      {
        int i = GR_VERTEX_R_OFFSET;

        while(i < GR_VERTEX_OOW_OFFSET) {
          TRI_NO_TSU_SETF(hwAddr++, fa[i++]);
        }

        /* Skip fbi oow */
        i++;

        /* FixMe? We currently only do one tmu. */
        while(i < GR_VERTEX_OOW_TMU0_OFFSET) {
          TRI_NO_TSU_SETF(hwAddr++, fa[i]);
          i++;
        }

        TRI_NO_TSU_SETF(hwAddr++, fa[GR_VERTEX_OOW_OFFSET]);
      }

      /* Send the x gradients */
      {
        int i = GR_VERTEX_R_OFFSET;

        while(i < GR_VERTEX_OOW_OFFSET) {
          const float curGrad = (((fa[i] - fb[i]) * dyBC) -
                                 ((fb[i] - fc[i]) * dyAB));
		    
          GDBG_INFO(285, FN_NAME": Gradient 0x%X : %g %g %g : %g\n",
                    ((FxU32)hwAddr - (FxU32)hw) >> 2,
                    fa[i], fb[i], fc[i], curGrad);
		    
          TRI_NO_TSU_SETF(hwAddr++, curGrad);
          i++;
        }

        /* Skip fbi oow */
        i++; 

        /* FixMe? We currently only do one tmu. */
        while(i < GR_VERTEX_OOW_TMU0_OFFSET) {
          const float curGrad = (((fa[i] - fb[i]) * dyBC) -
                                 ((fb[i] - fc[i]) * dyAB));
		    
          GDBG_INFO(285, FN_NAME": Gradient 0x%X : %g %g %g : %g\n",
                    ((FxU32)hwAddr - (FxU32)hw) >> 2,
                    fa[i], fb[i], fc[i], curGrad);
		    
          TRI_NO_TSU_SETF(hwAddr++, curGrad);
          i++;
        }

        {
          const float curGrad = (((fa[GR_VERTEX_OOW_OFFSET] - fb[GR_VERTEX_OOW_OFFSET]) * dyBC) -
                                 ((fb[GR_VERTEX_OOW_OFFSET] - fc[GR_VERTEX_OOW_OFFSET]) * dyAB));
                    
          TRI_NO_TSU_SETF(hwAddr++, curGrad);
        }
      }

      /* Send the y gradients */
      {
        int i = GR_VERTEX_R_OFFSET;

        while(i < GR_VERTEX_OOW_OFFSET) {
          const float curGrad = (((fb[i] - fc[i]) * dxAB) -
                                 ((fa[i] - fb[i]) * dxBC));
		    
          GDBG_INFO(285, FN_NAME": Gradient 0x%X : %g %g %g : %g\n",
                    ((FxU32)hwAddr - (FxU32)hw) >> 2,
                    fa[i], fb[i], fc[i], curGrad);

          TRI_NO_TSU_SETF(hwAddr++, curGrad);
          i++;
        }

        /* Skip fbi oow */
        i++;

        /* FixMe? We currently only do one tmu. */
        while(i < GR_VERTEX_OOW_TMU0_OFFSET) {
          const float curGrad = (((fb[i] - fc[i]) * dxAB) -
                                 ((fa[i] - fb[i]) * dxBC));
		    
          GDBG_INFO(285, FN_NAME": Gradient 0x%X : %g %g %g : %g\n",
                    ((FxU32)hwAddr - (FxU32)hw) >> 2,
                    fa[i], fb[i], fc[i], curGrad);
		    
          TRI_NO_TSU_SETF(hwAddr++, curGrad);
          i++;
        }

        {
          const float curGrad = (((fb[GR_VERTEX_OOW_OFFSET] - fc[GR_VERTEX_OOW_OFFSET]) * dxAB) -
                                 ((fa[GR_VERTEX_OOW_OFFSET] - fb[GR_VERTEX_OOW_OFFSET]) * dxBC));
                    
          TRI_NO_TSU_SETF(hwAddr++, curGrad);
        }                
      }      
	  
      /* Draw the triangle by writing the area to the triangleCMD register */
      TRI_NO_TSU_SETF(hwAddr, _GlideRoot.pool.ftemp1);
      _GlideRoot.stats.trisDrawn++;
    }
    TRI_NO_TSU_END();
  }

  goto __triDrawn;
#endif /* !GLIDE_HW_TRI_SETUP */

__triDrawn:
  /* If we made it this far then we drew the triangle */
  _GlideRoot.stats.trisDrawn++;
  GR_CHECK_SIZE();

  return 1;
    
#undef FN_NAME
} /* _trisetup_nogradients */
