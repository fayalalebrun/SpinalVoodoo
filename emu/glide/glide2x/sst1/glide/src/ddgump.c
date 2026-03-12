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
 * 2     3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
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

extern GrMPState _gumpState;

/* CHD BUG this belongs in fxglide.h */
void FX_CSTYLE
_gumpTexCombineFunction( int virtual_tmu );

/*
** _gumpTexCombineFunction
**                                                          
** Sets the texture combine function.  For a dual TMU system this function
** will configure the TEXTUREMODE registers as appropriate.  For a
** single TMU system this function will configure TEXTUREMODE if
** possible, or defer operations until grDrawTriangle() is called.
*/
GR_DDFUNC(_gumpTexCombineFunction, void, ( int virtual_tmu ))
{
  FxU32 texmode;
  GrMPTextureCombineFnc_t tc;

  GR_BEGIN("_gumpTexCombineFunction",99,4+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",virtual_tmu));

  texmode = gc->state.tmu_config[0].textureMode;
  texmode &= ~( SST_TCOMBINE | SST_TACOMBINE );
  tc = _gumpState.tc_fnc;

  switch ( tc )
  {
  case GR_MPTEXTURECOMBINE_ADD:
    /* tmu0: other + local */
    /* tmu1: local */
    /* pass0(tm0): local */
    /* pass1(tm1): local */
    if ( virtual_tmu == 0 )
    {
      texmode |= ( SST_TC_REPLACE | SST_TCA_REPLACE );
    }
    else
    {
      texmode |= ( SST_TC_REPLACE | SST_TCA_REPLACE );
    }
    break;
  case GR_MPTEXTURECOMBINE_MULTIPLY:
    /* tmu0: other * local */
    /* tmu1: local */
    /* pass0(tm0): local */
    /* pass1(tm1): local */
    if ( virtual_tmu == 0 )
    {
      texmode |= ( SST_TC_REPLACE | SST_TCA_REPLACE );
    }
    else
    {
      texmode |= ( SST_TC_REPLACE | SST_TCA_REPLACE );
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
    if ( virtual_tmu == 0 )
    {
      texmode |= ( SST_TC_BLEND_LOD | SST_TCA_BLEND_LOD |
                   SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER );
    }
    else
    {
      texmode |= ( SST_TC_BLEND_LOD | SST_TCA_BLEND_LOD |
                   SST_TC_REVERSE_BLEND | SST_TCA_REVERSE_BLEND |
                   SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER );      
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
    if ( virtual_tmu == 0 )
    {
      texmode |= ( SST_TC_BLEND_LOD | SST_TCA_BLEND_LOD |
                   SST_TC_REVERSE_BLEND | SST_TCA_REVERSE_BLEND |
                   SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER );
    }
    else
    {
      texmode |= ( SST_TC_BLEND_LOD | SST_TCA_BLEND_LOD |
                   SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER );
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
    if ( virtual_tmu == 0 )
    {
      texmode |= ( SST_TC_BLEND_LODFRAC | SST_TCA_BLEND_LODFRAC |
                   SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER );
    }
    else
    {
      texmode |= ( SST_TC_BLEND_LODFRAC | SST_TCA_BLEND_LODFRAC |
                   SST_TC_REVERSE_BLEND | SST_TCA_REVERSE_BLEND |
                   SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER );
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
    if ( virtual_tmu == 0 )
    {
      texmode |= ( SST_TC_BLEND_LODFRAC | SST_TCA_BLEND_LODFRAC |
                  SST_TC_REVERSE_BLEND | SST_TCA_REVERSE_BLEND |
                  SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER );
    }
    else
    {
      texmode |= ( SST_TC_BLEND_LODFRAC | SST_TCA_BLEND_LODFRAC |
                   SST_TC_ZERO_OTHER | SST_TCA_ZERO_OTHER );
    }
    break;
  case GR_MPTEXTURECOMBINE_SUBTRACT:
    /*
       tmu0: other - local 
       tmu1: local
       doesn't work, alpha blender can't subtract
       */
    GrErrorCallback( "_gumpTexCombineFunction:  TEXTURE_SUBTRACT not supported", FXFALSE );
    return;
    break;
  default:
    GrErrorCallback( "_gumpTexCombineFunction:  Unsupported function", FXFALSE );
    return;
    break;
  }
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,0)->textureMode , texmode );
  PACKER_WORKAROUND;
  GR_END_SLOPPY();
} /* _gumpTexCombineFunction */

GR_ENTRY(guMPDrawTriangle, void, ( const GrVertex *a, const GrVertex *b, const GrVertex *c ))
{
  GR_BEGIN_NOFIFOCHECK("guMPDrawTriangle",98);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x,0x%x,0x%x)\n",a,b,c));
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

    if ( ( gc->state.fbi_config.alphaMode & SST_ENALPHABLEND ) ||
         ( gc->state.fbi_config.fbzColorPath & SST_LOCALSELECT_OVERRIDE_WITH_ATEX ) ||
         ( gc->state.fbi_config.alphaMode & SST_ENALPHAFUNC ) ||
         ( gc->state.fbi_config.fbzMode & SST_ENCHROMAKEY )
       )
    {
      GrErrorCallback( "guMPDrawTriangle:  Illegal state", FXFALSE );
    }

    if ( _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_ADD ||
         _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_DETAIL0 ||
         _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_DETAIL1 ||
         _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_TRILINEAR0 ||
         _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_TRILINEAR1 )
    {
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
      guTexSource( _gumpState.mmid[0] );
      _gumpTexCombineFunction( 0 );

      /* render first pass */
      grDrawTriangle( a, b, c );

      /* second pass */

      /* xxx may need to copy texture coordinates, see
         grTexCombineFunction's second parameter.  if so,
         we have to disable the second coordinate when drawing. */

      /* tmu setup */
      guTexSource( _gumpState.mmid[1] );
      _gumpTexCombineFunction( 1 );

      GR_SET_EXPECTED_SIZE(16);
      /* enable alpha blend to add to destination buffers */
      alphamode &= ~(SST_RGBSRCFACT | SST_RGBDSTFACT | SST_ASRCFACT | SST_ADSTFACT);
      alphamode |= SST_ENALPHABLEND |
                   (SST_A_ONE << SST_RGBSRCFACT_SHIFT) |
                   (SST_A_ONE << SST_RGBDSTFACT_SHIFT) |
                   (SST_A_ONE << SST_ASRCFACT_SHIFT) |
                   (SST_A_ONE << SST_ADSTFACT_SHIFT);

      GR_SET( hw->alphaMode, alphamode );

      /* if depth buffering, set to z= mode and disable writes */

      if ( gc->state.fbi_config.fbzMode & SST_ENDEPTHBUFFER )
      {
        fbzmode = fbzmode_orig = gc->state.fbi_config.fbzMode;

        fbzmode &= ~(SST_ZAWRMASK | SST_ZFUNC);
        fbzmode |= GR_CMP_EQUAL;

        GR_SET( hw->fbzMode, fbzmode );
      }

      /* disable (fogta * fogColor) bias */
      /* xxx setting ADD_FOG with fog disabled is harmless */
      if ( gc->state.fbi_config.fogMode & SST_ENFOGGING ) {
        fogmode = fogmode_orig = gc->state.fbi_config.fogMode;
        fogmode |= SST_FOGADD;
        GR_SET( hw->fogMode, fogmode );
      }

      /* disable biasing in color combine */
      /* this can change the parameters output */
      /* xxx the equivalent of GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA
             does not work, you need to do this instead of below.

      if ( gc->state.cc_fnc == GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA )
      {
        fbzcolorpath &= ~SST_CC_ADD_CLOCAL;
      }
      */

      
      fbzcolorpath &= ~(SST_CC_ADD_CLOCAL | SST_CC_ADD_ALOCAL | SST_CC_SUB_CLOCAL);
      GR_SET( hw->fbzColorPath, fbzcolorpath );
      GR_CHECK_SIZE_SLOPPY();

      /* render other pass */
      grDrawTriangle( a, b, c );

      /* restore */
      GR_SET_EXPECTED_SIZE(16);

      /* restore alpha blending state */
      GR_SET( hw->alphaMode, alphamode_orig );

      /* restore depth buffer state */
      if ( gc->state.fbi_config.fbzMode & SST_ENDEPTHBUFFER ) {
        GR_SET( hw->fbzMode, fbzmode_orig );
      }

      /* restore fog state */
      if ( gc->state.fbi_config.fogMode & SST_ENFOGGING ) {
        GR_SET( hw->fogMode, fogmode_orig );
      }

      /* restore ccu/acu state */
      GR_SET( hw->fbzColorPath, fbzcolorpath_orig );

      goto all_done;
    }

    else if ( _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_MULTIPLY )
    {
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

      guTexSource( _gumpState.mmid[0] );
      _gumpTexCombineFunction( 0 );

      /* disable bias */
      GR_SET_EXPECTED_SIZE(8);

      /* disable (fogta * fogColor) bias */
      /* xxx setting ADD_FOG with fog disabled is harmless */

      if ( gc->state.fbi_config.fogMode & SST_ENFOGGING ) {
        fogmode = fogmode_orig = gc->state.fbi_config.fogMode;
        fogmode |= SST_FOGADD;
        GR_SET( hw->fogMode, fogmode );
      }

      /* disable biasing in color combine */
      /* this can change the parameters output */
      /* xxx consult add path for switch version */
      /* xxx the equivalent of GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA
             does not work, you need to do this instead of below.

      if ( gc->state.cc_fnc == GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA )
      {
        fbzcolorpath &= ~SST_CC_ADD_CLOCAL;
      }
      */

      fbzcolorpath &= ~(SST_CC_ADD_CLOCAL | SST_CC_ADD_ALOCAL | SST_CC_SUB_CLOCAL);
      GR_SET( hw->fbzColorPath, fbzcolorpath );
      GR_CHECK_SIZE_SLOPPY();

      /* render first pass */

      grDrawTriangle( a, b, c );

      /* second pass */

      /* xxx may sometimes need to copy texture coordinates */

      /* tmu setup */

      guTexSource( _gumpState.mmid[1] );
      _gumpTexCombineFunction( 1 );
      GR_SET_EXPECTED_SIZE(16);

      /* enable alpha blend to multiply to destination buffers */
      /* xxx alpha component blender can only handle factors of
         0 and 1 */

      alphamode &= ~(SST_RGBSRCFACT | SST_RGBDSTFACT | SST_ASRCFACT | SST_ADSTFACT);
      alphamode |= SST_ENALPHABLEND |
                   (SST_A_COLOR << SST_RGBSRCFACT_SHIFT) |
                   (SST_A_ZERO << SST_RGBDSTFACT_SHIFT) |
                   (SST_A_ONE << SST_ASRCFACT_SHIFT) |
                   (SST_A_ZERO << SST_ADSTFACT_SHIFT);

      GR_SET( hw->alphaMode, alphamode );

      /* if depth buffering, set to z= mode and disable writes */
      if ( gc->state.fbi_config.fbzMode & SST_ENDEPTHBUFFER )
      {
        fbzmode = fbzmode_orig = gc->state.fbi_config.fbzMode;
        fbzmode &= ~(SST_ZAWRMASK | SST_ZFUNC);
        fbzmode |= GR_CMP_EQUAL;
        GR_SET( hw->fbzMode, fbzmode );
      }

      /* disable fog */
      if ( gc->state.fbi_config.fogMode & SST_ENFOGGING ) {
        GR_SET( hw->fogMode, 0 );
      }

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
        GrErrorCallback( "guDrawTriangleMP:  MULTIPLY and BLEND_ITRGB_ON_TEXALPHA not implemented yet!", FXFALSE );
        goto all_done;
      }
      */

      fbzcolorpath |= SST_RGBSEL_TMUOUT;
      GR_SET( hw->fbzColorPath, fbzcolorpath );
      GR_CHECK_SIZE_SLOPPY();

      /* render second pass */
      grDrawTriangle( a, b, c );

      /* if bias, third pass */
      if ( ( gc->state.fbi_config.fogMode & SST_ENFOGGING )
      /* xxxXXXxxx it isn't very easy to check following any more!
           ||
           gc->state.cc_fnc == GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB_ADD_ITALPHA ||
           gc->state.cc_fnc == GR_COLORCOMBINE_TEXTURE_TIMES_ITALPHA_ADD_ITRGB ||
           gc->state.cc_fnc == GR_COLORCOMBINE_TEXTURE_ADD_ITRGB ||
           gc->state.cc_fnc == GR_COLORCOMBINE_TEXTURE_SUB_ITRGB ||
           gc->state.cc_fnc == GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA ||
           gc->state.cc_fnc == GR_COLORCOMBINE_DIFF_SPEC_A ||
           gc->state.cc_fnc == GR_COLORCOMBINE_DIFF_SPEC_B */ )
      {
        /* enable alpha blend to add to destination buffers */
        GR_SET_EXPECTED_SIZE(8);

        alphamode &= ~(SST_RGBSRCFACT | SST_RGBDSTFACT | SST_ASRCFACT | SST_ADSTFACT);
        alphamode |= SST_ENALPHABLEND |
                     (SST_A_ONE << SST_RGBSRCFACT_SHIFT) |
                     (SST_A_ONE << SST_RGBDSTFACT_SHIFT) |
                     (SST_A_ONE << SST_ASRCFACT_SHIFT) |
                     (SST_A_ONE << SST_ADSTFACT_SHIFT);

        GR_SET( hw->alphaMode, alphamode );

        /* disable fog factor, leave fog bias enabled */
        /* xxx setting ADD_MULT with fog disabled is harmless */
        if ( gc->state.fbi_config.fogMode & SST_ENFOGGING ) {
          fogmode = fogmode_orig;
          fogmode |= SST_FOGMULT;
          GR_SET( hw->fogMode, fogmode );
        }

        /* disable factor in color combine, enable bias */
        /* this can change the parameters output */

        fbzcolorpath  = fbzcolorpath_orig;
        fbzcolorpath &= ~( SST_ENTEXTUREMAP |
                           SST_RGBSELECT |
                           SST_LOCALSELECT |
                           SST_CC_ZERO_OTHER |
                           SST_CC_SUB_CLOCAL |
                           SST_CC_MSELECT |
                           SST_CC_REVERSE_BLEND |
                           SST_CC_ADD_CLOCAL |
                           SST_CC_ADD_ALOCAL |
                           SST_CC_INVERT_OUTPUT |
                           SST_CC_REVERSE_BLEND );

#if 0
        /* xxx the following doesn't work anymore! */

        switch ( gc->state.cc_fnc )
        {
        case GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB_ADD_ITALPHA:
          fbzcolorpath |= SST_CC_ADD_ALOCAL | SST_CC_ZERO_OTHER;
          break;

          /* unimplemented in Glide */
        case GR_COLORCOMBINE_TEXTURE_TIMES_ITALPHA_ADD_ITRGB:
          fbzcolorpath |= SST_CC_ADD_CLOCAL | SST_CC_ZERO_OTHER;
          break;

        case GR_COLORCOMBINE_TEXTURE_ADD_ITRGB:
          fbzcolorpath |= SST_CC_ADD_CLOCAL | SST_CC_ZERO_OTHER;
          break;

        case GR_COLORCOMBINE_TEXTURE_SUB_ITRGB:
          /* Can't do this on SST1, can't subtract in alpha blender */
          GrErrorCallback( "guDrawTriangleM:  GR_COLORCOMBINE_TEXTURE_SUB_ITRGB does not work with GR_MPTEXTURECOMBINE_MULTIPLY.", FXFALSE );
          fbzcolorpath |= SST_CC_ZERO_OTHER;
          break;

        case GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA:
          fbzcolorpath |= SST_CC_ZERO_OTHER;
          break;

        case GR_COLORCOMBINE_DIFF_SPEC_A:
          fbzcolorpath |= SST_CC_ADD_CLOCAL;
          break;

        case GR_COLORCOMBINE_DIFF_SPEC_B:
          fbzcolorpath |= SST_CC_ADD_ALOCAL;
          break;
        }
#endif
        GR_CHECK_SIZE_SLOPPY();

        /* render third pass */
        grDrawTriangle( a, b, c );
      }

      GR_SET_EXPECTED_SIZE(16);

      /* restore alpha blending state */
      GR_SET( hw->alphaMode, alphamode_orig );

      /* restore depth buffer state */
      if ( gc->state.fbi_config.fbzMode & SST_ENDEPTHBUFFER ) {
        GR_SET( hw->fbzMode, fbzmode_orig );
      }

      /* restore fog state */
      if ( gc->state.fbi_config.fogMode & SST_ENFOGGING ) {
        GR_SET( hw->fogMode, fogmode_orig );
      }

      /* restore ccu/acu state */
      GR_SET( hw->fbzColorPath, fbzcolorpath_orig );

      goto all_done;
    }

    else if ( _gumpState.tc_fnc == GR_MPTEXTURECOMBINE_SUBTRACT )
    {
      GrErrorCallback( "gumpDrawTriangle:  GR_MPTEXCOMBINE_SUBTRACT not implemented", FXFALSE );
      goto all_done;
    }
  }
all_done:
  GR_END_SLOPPY();
}
