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
** 3     3/17/99 6:16p Dow
** Phantom Menace and other fixes.
 * 
 * 15    12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 14    12/15/97 5:51p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 13    12/08/97 12:06p Atai
 * change prototype for grDrawPoint, grDrawLine, grDrawTriangel
 * 
 * 12    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 11    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 10    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 9     10/27/97 1:16p Peter
 * fixed silliness
 * 
 * 8     10/27/97 11:10a Peter
 * starting cleanup
 * 
 * 7     9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 6     9/04/97 3:32p Peter
 * starting grouping serial reg writes
 * 
 * 5     6/06/97 10:47a Peter
 * texture downloading, fixed 640x480 dimension, changed cvg dep to be the
 * same as sst1
 * 
 * 4     5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 3     5/21/97 6:04a Peter
 * 
 * 2     3/04/97 9:08p Dow
 * 
 * 1     12/23/96 1:39p Dow
 * Changes for multiplatform
**
*/

/* Implements multipass drawing */

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"
#include "gump.h"

/* gump.c */
extern GrMPState _gumpState;

#ifndef GLIDE3_ALPHA
/*
** _gumpTexCombineFunction
**                                                          
** Sets the texture combine function.  For a dual TMU system this function
** will configure the TEXTUREMODE registers as appropriate.  For a
** single TMU system this function will configure TEXTUREMODE if
** possible, or defer operations until grDrawTriangle() is called.
*/
GR_DDFUNC(_gumpTexCombineFunction, void, (int virtual_tmu))
{
#define FN_NAME "_gumpTexCombineFunction"
  FxU32 texmode;
  GrMPTextureCombineFnc_t tc;

  GR_BEGIN_NOFIFOCHECK("_gumpTexCombineFunction",99);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",virtual_tmu);

  texmode = gc->state.tmu_config[0].textureMode;
  texmode &= ~(SST_TCOMBINE | SST_TACOMBINE);
  tc = _gumpState.tc_fnc;

  switch (tc) {
  case GR_MPTEXTURECOMBINE_ADD:
    /* tmu0: other + local */
    /* tmu1: local */
    /* pass0(tm0): local */
    /* pass1(tm1): local */
    if (virtual_tmu == 0) {
      texmode |= (SST_TC_REPLACE | SST_TCA_REPLACE);
    } else {
      texmode |= (SST_TC_REPLACE | SST_TCA_REPLACE);
    }
    break;

  case GR_MPTEXTURECOMBINE_MULTIPLY:
    /* tmu0: other * local */
    /* tmu1: local */
    /* pass0(tm0): local */
    /* pass1(tm1): local */
    if (virtual_tmu == 0) {
      texmode |= (SST_TC_REPLACE | SST_TCA_REPLACE);
    } else {
      texmode |= (SST_TC_REPLACE | SST_TCA_REPLACE);
    }
    break;
    
  case GR_MPTEXTURECOMBINE_DETAIL0:
    /* tmu0: (other - local) * lod + local */
    /*     = lod * other + (1 - lod) * local */
    /* tmu1: local */
    /* pass0(tm0): (-local) * lod + local */
    /*           = (1 - lod) * local */
    /* pass1(tm1): (-local) * (1 - lod) + local */
    /*           = lod * local */
    if (virtual_tmu == 0) {
      texmode |= (SST_TC_BLEND_LOD | SST_TCA_BLEND_LOD |
                  SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER);
    } else {
      texmode |= (SST_TC_BLEND_LOD | SST_TCA_BLEND_LOD |
                  SST_TC_REVERSE_BLEND | SST_TCA_REVERSE_BLEND |
                  SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER);      
    }
    break;

  case GR_MPTEXTURECOMBINE_DETAIL1:
    /* tmu0: (other - local) * (1 - lod) + local */
    /*     = (1 - lod) * other + lod * local */
    /* tmu1: local */
    /* pass0(tm0): (-local) * (1 - lod) + local */
    /*           = lod * local */
    /* pass1(tm1): (-local) * lod + local */
    /*           = (1 - lod) * local */
    if (virtual_tmu == 0) {
      texmode |= (SST_TC_BLEND_LOD | SST_TCA_BLEND_LOD |
                  SST_TC_REVERSE_BLEND | SST_TCA_REVERSE_BLEND |
                  SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER);
    } else {
      texmode |= (SST_TC_BLEND_LOD | SST_TCA_BLEND_LOD |
                  SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER);
    }
    break;

  case GR_MPTEXTURECOMBINE_TRILINEAR0:
    /* tmu0: (other - local) * lodbfrac + local */
    /*     = lodbfrac * other + (1 - lodbfrac) * local */
    /* tmu1: local */
    /* pass0(tm0): (-local) * lodbfrac + local */
    /*           = (1 - lodbfrac) * local */
    /* pass1(tm1): (-local) * (1 - lodbfrac) + local */
    /*           = lodbfrac * local */
    if (virtual_tmu == 0) {
      texmode |= (SST_TC_BLEND_LODFRAC | SST_TCA_BLEND_LODFRAC |
                  SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER);
    } else {
      texmode |= (SST_TC_BLEND_LODFRAC | SST_TCA_BLEND_LODFRAC |
                  SST_TC_REVERSE_BLEND | SST_TCA_REVERSE_BLEND |
                  SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER);
    }
    break;

  case GR_MPTEXTURECOMBINE_TRILINEAR1:
    /* tmu0: (other - local) * (1 - lodbfrac) + local
       = (1 - lodbfrac) * other + lodbfrac * local
       tmu1: local
       pass0(tm0): (-local) * (1 - lodbfrac) + local
       = lodbfrac * local
       pass1(tm1): (-local) * lodbfrac + local
       = (1 - lodbfrac) * local
       */
    if (virtual_tmu == 0) {
      texmode |= (SST_TC_BLEND_LODFRAC | SST_TCA_BLEND_LODFRAC |
                  SST_TC_REVERSE_BLEND | SST_TCA_REVERSE_BLEND |
                  SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER);
    } else {
      texmode |= (SST_TC_BLEND_LODFRAC | SST_TCA_BLEND_LODFRAC |
                  SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER);
    }
    break;

  case GR_MPTEXTURECOMBINE_SUBTRACT:
    /*
      tmu0: other - local 
      tmu1: local
      doesn't work, alpha blender can't subtract
      */
    GrErrorCallback("_gumpTexCombineFunction:  TEXTURE_SUBTRACT not supported", FXFALSE);
    return;
    break;

  default:
    GrErrorCallback("_gumpTexCombineFunction:  Unsupported function", FXFALSE);
    return;
    break;
  }

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  {
    SstRegs* tmuRegs = SST_TMU(hw, 0);
    
    GR_SET(eChipTMU0, tmuRegs, textureMode, texmode);
  }
  GR_CHECK_SIZE();

  GR_END();
#undef _gumpTexCombineFunction
} /* _gumpTexCombineFunction */

GR_ENTRY(guMPDrawTriangle, void, (const GrVertex *a, const GrVertex *b, const GrVertex *c))
{
  GR_BEGIN_NOFIFOCHECK("guMPDrawTriangle",98);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x,0x%x,0x%x)\n",a,b,c);
  GR_CHECK_F(myName, !a || !b || !c, "NULL pointer passed");

  /* check for multipass texture modes */
  /* xxx complex multipass mode checks should be moved into gtex/gglide */
  /* specific color combine modes use texture */
  /* this is equivalent to
     fbzColorPath & SST_ENTEXTREMAP
     cc_state & STATE_CC_REQUIRES_DECAL_STW */
  /* specific texture combine modes */
  /* xxx don't even check anymore, because you can't */

  {
    /* check for not possible on one tmu modes */
    /* alpha blend enabled (grAlphaBlendMode)
       xxx some can work, with some cheats, even more
       low bit alpha switches between iterated and constant rgb
       (grAlphaControlsITRGBLighting)
       alpha test (grAlphaTestFunction)
       xxx actually, NEVER is ok too
       chromakey
       */

    if ((gc->state.fbi_config.alphaMode & SST_ENALPHABLEND) ||
        (gc->state.fbi_config.fbzColorPath & SST_LOCALSELECT_OVERRIDE_WITH_ATEX) ||
        (gc->state.fbi_config.alphaMode & SST_ENALPHAFUNC) ||
        (gc->state.fbi_config.fbzMode & SST_ENCHROMAKEY)) {
      GrErrorCallback("guMPDrawTriangle:  Illegal state", FXFALSE);
    }

    if (_gumpState.tc_fnc == GR_MPTEXTURECOMBINE_ADD ||
        _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_DETAIL0 ||
        _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_DETAIL1 ||
        _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_TRILINEAR0 ||
        _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_TRILINEAR1) {
      /* render pass with all effects */
      /* select other texture map */
      /* set alpha blend to add to dest buffer */
      /* if depth buffering, save depth buffer function,
         set to z-equal */
      /* disable fog and color combine bias */
      /* render other pass */
      /* restore depth buffer function, restore alpha blend, enable fog
         and color combine bias */

      FxU32 alphamode, alphamode_orig;
      FxU32 fbzcolorpath, fbzcolorpath_orig;
      FxU32 fbzmode, fbzmode_orig = 0;
      FxU32 fogmode, fogmode_orig = 0;

      alphamode = alphamode_orig = gc->state.fbi_config.alphaMode;
      fbzcolorpath = fbzcolorpath_orig = gc->state.fbi_config.fbzColorPath;
      GR_CHECK_SIZE();

      /* first pass */

      /* tmu setup */
      guTexSource(_gumpState.mmid[0]);
      _gumpTexCombineFunction(0);

      /* render first pass */
#ifdef GLIDE3
      grDrawTriangle((void *)a, (void *)b, (void *)c);
#else
      grDrawTriangle(a, b, c);
#endif

      /* second pass */

      /* xxx may need to copy texture coordinates, see
         grTexCombineFunction's second parameter.  if so,
         we have to disable the second coordinate when drawing. */

      /* tmu setup */
      guTexSource(_gumpState.mmid[1]);
      _gumpTexCombineFunction(1);

      {
        const FxBool depthP = ((gc->state.fbi_config.fbzMode & SST_ENDEPTHBUFFER) == SST_ENDEPTHBUFFER);
        const FxBool fogP = ((gc->state.fbi_config.fogMode & SST_ENFOGGING) == SST_ENFOGGING);
        const FxU32 regCount = (2 + depthP + fogP);
        const FxU32 regMask = (0x05 | MaskSelect(depthP, 0x08UL) | MaskSelect(fogP, 0x02UL));

        REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, regCount, regMask);
        {
          /* disable biasing in color combine */
          /* this can change the parameters output */
          /* xxx the equivalent of GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA
             does not work, you need to do this instead of below.
             
             if (gc->state.cc_fnc == GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA)
             {
             fbzcolorpath &= ~SST_CC_ADD_CLOCAL;
             }
             */
      
          fbzcolorpath &= ~(SST_CC_ADD_CLOCAL | SST_CC_ADD_ALOCAL | SST_CC_SUB_CLOCAL);
          REG_GROUP_SET(hw, fbzColorPath, fbzcolorpath);

          /* disable (fogta * fogColor) bias */
          /* xxx setting ADD_FOG with fog disabled is harmless */
          if (fogP) {
            fogmode = fogmode_orig = gc->state.fbi_config.fogMode;
            fogmode |= SST_FOGADD;

            REG_GROUP_SET(hw, fogMode, fogmode);
          }

          /* enable alpha blend to add to destination buffers */
          alphamode &= ~(SST_RGBSRCFACT | SST_RGBDSTFACT | SST_ASRCFACT | SST_ADSTFACT);
          alphamode |= (SST_ENALPHABLEND |
                        (SST_A_ONE << SST_RGBSRCFACT_SHIFT) |
                        (SST_A_ONE << SST_RGBDSTFACT_SHIFT) |
                        (SST_A_ONE << SST_ASRCFACT_SHIFT) |
                        (SST_A_ONE << SST_ADSTFACT_SHIFT));

          REG_GROUP_SET(hw, alphaMode, alphamode);

          /* if depth buffering, set to z= mode and disable writes */
          if (depthP) {
            fbzmode = fbzmode_orig = gc->state.fbi_config.fbzMode;
            
            fbzmode &= ~(SST_ZAWRMASK | SST_ZFUNC);
            fbzmode |= GR_CMP_EQUAL;

            REG_GROUP_SET(hw, fbzMode, fbzmode);
          }
        }
        REG_GROUP_END();

        /* render other pass */
#ifdef GLIDE3
        grDrawTriangle((void *)a, (void *)b, (void *)c);
#else
        grDrawTriangle(a, b, c);
#endif

        /* restore */
        REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, regCount, regMask);
        {
          /* restore ccu/acu state */
          REG_GROUP_SET(hw, fbzColorPath, fbzcolorpath_orig);

          /* restore fog state */
          if (fogP) REG_GROUP_SET(hw, fogMode, fogmode_orig);

          /* restore alpha blending state */
          REG_GROUP_SET(hw, alphaMode, alphamode_orig);
          
          /* restore depth buffer state */
          if (depthP) REG_GROUP_SET(hw, fbzMode, fbzmode_orig);
        }
        REG_GROUP_END();
      }
        
      goto all_done;
    } else if (_gumpState.tc_fnc == GR_MPTEXTURECOMBINE_MULTIPLY) {
      /* disable fog and color combine bias */
      /* render pass with all other effects */
      /* select other texture map */
      /* set alpha blend to multiply to dest buffer */
      /* if depth buffering, save depth buffer function,
         set to z-equal */
      /* disable fog and color combine factor */
      /* render other pass */
      /* if fog or color combine w/bias */
      /* disable tmu */
      /* set alpha blend to add to dest buffer */
      /* enable fog and color combine bias */
      /* render bias pass */
      /* restore depth buffer function, restore alpha blend, enable fog
         and color combine bias */

      FxU32 alphamode, alphamode_orig;
      FxU32 fbzcolorpath, fbzcolorpath_orig;
      FxU32 fbzmode, fbzmode_orig = 0;
      FxU32 fogmode, fogmode_orig = 0;

      alphamode = alphamode_orig = gc->state.fbi_config.alphaMode;
      fbzcolorpath = fbzcolorpath_orig = gc->state.fbi_config.fbzColorPath;

      /* first pass */

      /* tmu setup */

      guTexSource(_gumpState.mmid[0]);
      _gumpTexCombineFunction(0);

      /* disable bias */
      {
        const FxBool depthP = ((gc->state.fbi_config.fbzMode & SST_ENDEPTHBUFFER) == SST_ENDEPTHBUFFER);
        const FxBool fogP = ((gc->state.fbi_config.fogMode & SST_ENFOGGING) == SST_ENFOGGING);
        const FxU32 regCount = (2 + depthP + fogP);
        const FxU32 regMask = (0x05 | MaskSelect(depthP, 0x08UL) | MaskSelect(fogP, 0x02UL));

        REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, (1 + fogP), (0x01UL | MaskSelect(fogP, 0x02UL)));
        {
          /* disable biasing in color combine */
          /* this can change the parameters output */
          /* xxx consult add path for switch version */
          /* xxx the equivalent of GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA
             does not work, you need to do this instead of below.
             
             if (gc->state.cc_fnc == GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA)
             {
             fbzcolorpath &= ~SST_CC_ADD_CLOCAL;
             }
             */

          fbzcolorpath &= ~(SST_CC_ADD_CLOCAL | SST_CC_ADD_ALOCAL | SST_CC_SUB_CLOCAL);
          REG_GROUP_SET(hw, fbzColorPath, fbzcolorpath);

          /* disable (fogta * fogColor) bias */
          /* xxx setting ADD_FOG with fog disabled is harmless */
          if (fogP) {
            fogmode = fogmode_orig = gc->state.fbi_config.fogMode;
            fogmode |= SST_FOGADD;

            REG_GROUP_SET(hw, fogMode, fogmode);
          }
        }
        REG_GROUP_END();
        
        /* render first pass */
#ifdef GLIDE3
        grDrawTriangle((void *)a, (void *)b, (void *)c);
#else
        grDrawTriangle(a, b, c);
#endif
        
        /* second pass */
        /* xxx may sometimes need to copy texture coordinates */
        
        /* tmu setup */
        guTexSource(_gumpState.mmid[1]);
        _gumpTexCombineFunction(1);

        REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, regCount, regMask);
        {
          /* disable factor and bias in color combine--
             decal, except for the cases that need texture alpha */
          /* this can change the parameters output */
          fbzcolorpath  = fbzcolorpath_orig;
          fbzcolorpath &= ~( SST_RGBSELECT |
                             SST_LOCALSELECT |
                             SST_CC_ZERO_OTHER |
                             SST_CC_SUB_CLOCAL |
                             SST_CC_MSELECT |
                             SST_CC_REVERSE_BLEND |
                             SST_CC_ADD_CLOCAL |
                             SST_CC_ADD_ALOCAL |
                             SST_CC_INVERT_OUTPUT |
                             SST_CC_REVERSE_BLEND );
          
          /* xxx the equivalent of GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA
             does not work, you need to do this instead of below.
             
             if ( gc->state.cc_fnc == GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA )
             {
             xxx see implementation notes on why this isn't implemented yet
             GrErrorCallback("guDrawTriangleMP: "
             "MULTIPLY and BLEND_ITRGB_ON_TEXALPHA not implemented yet!", FXFALSE );
             goto all_done;
             }
             */
          fbzcolorpath |= SST_RGBSEL_TMUOUT;
          REG_GROUP_SET(hw, fbzColorPath, fbzcolorpath);

          /* disable fog */
          if (fogP) REG_GROUP_SET(hw, fogMode, 0);
          
          /* enable alpha blend to multiply to destination buffers */
          /* xxx alpha component blender can only handle factors of
             0 and 1 */
          alphamode &= ~(SST_RGBSRCFACT | SST_RGBDSTFACT | SST_ASRCFACT | SST_ADSTFACT);
          alphamode |= (SST_ENALPHABLEND |
                        (SST_A_COLOR << SST_RGBSRCFACT_SHIFT) |
                        (SST_A_ZERO << SST_RGBDSTFACT_SHIFT) |
                        (SST_A_ONE << SST_ASRCFACT_SHIFT) |
                        (SST_A_ZERO << SST_ADSTFACT_SHIFT));

          REG_GROUP_SET(hw, alphaMode, alphamode);
          
          /* if depth buffering, set to z= mode and disable writes */
          if (depthP) {
            fbzmode = fbzmode_orig = gc->state.fbi_config.fbzMode;
            fbzmode &= ~(SST_ZAWRMASK | SST_ZFUNC);
            fbzmode |= GR_CMP_EQUAL;
            
            REG_GROUP_SET(hw, fbzMode, fbzmode);
          }
        }
        REG_GROUP_END();

        /* render second pass */
#ifdef GLIDE3
        grDrawTriangle((void *)a, (void *)b, (void *)c);
#else
        grDrawTriangle(a, b, c);
#endif
        
        /* if bias, third pass */
        if (fogP) {
          /* enable alpha blend to add to destination buffers */
          REG_GROUP_BEGIN(BROADCAST_ID, fogMode, 2, 0x03);
          {
            /* disable fog factor, leave fog bias enabled */
            /* xxx setting ADD_MULT with fog disabled is harmless */
            fogmode = fogmode_orig;
            fogmode |= SST_FOGMULT;
            REG_GROUP_SET(hw, fogMode, fogmode);
            
            alphamode &= ~(SST_RGBSRCFACT | SST_RGBDSTFACT | SST_ASRCFACT | SST_ADSTFACT);
            alphamode |= (SST_ENALPHABLEND |
                          (SST_A_ONE << SST_RGBSRCFACT_SHIFT) |
                          (SST_A_ONE << SST_RGBDSTFACT_SHIFT) |
                          (SST_A_ONE << SST_ASRCFACT_SHIFT) |
                          (SST_A_ONE << SST_ADSTFACT_SHIFT));
            REG_GROUP_SET(hw, alphaMode, alphamode);
          }
          REG_GROUP_END();
          
          /* render third pass */
#ifdef GLIDE3
          grDrawTriangle((void *)a, (void *)b, (void *)c);
#else
          grDrawTriangle(a, b, c);
#endif
        }

        REG_GROUP_BEGIN(BROADCAST_ID, fbzColorPath, regCount, regMask);
        {
          /* restore ccu/acu state */
          REG_GROUP_SET(hw, fbzColorPath, fbzcolorpath_orig);

          /* restore fog state */
          if (fogP) REG_GROUP_SET(hw, fogMode, fogmode_orig);

          /* restore alpha blending state */
          REG_GROUP_SET(hw, alphaMode, alphamode_orig);
          
          /* restore depth buffer state */
          if (depthP) REG_GROUP_SET(hw, fbzMode, fbzmode_orig);
        }
        REG_GROUP_END();
      }

      goto all_done;
    } else if (_gumpState.tc_fnc == GR_MPTEXTURECOMBINE_SUBTRACT) {
      GrErrorCallback("gumpDrawTriangle:  GR_MPTEXCOMBINE_SUBTRACT not implemented", FXFALSE);
      goto all_done;
    }
  }

all_done:
  GR_END();
}

#endif /* GLIDE3_ALPHA */
