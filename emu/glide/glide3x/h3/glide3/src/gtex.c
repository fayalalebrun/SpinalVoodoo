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
** Revision 1.1.1.1.8.2  2005/06/09 18:32:29  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.1.1.8.1  2004/09/27 08:59:45  dborca
** clamp lod bias
**
** Revision 1.1.1.1  1999/11/24 21:44:57  joseph
** Initial checkin for SourceForge
**
** 
** 4     4/06/99 3:54p Dow
** Alt tab again.
** 
** 47    4/04/99 8:51p Atai
** Partial check-in for alt-tab issue. set FX_GLIDE_ALT_TAB=1 to build
** glide3x with hwcQueryContext built into GR_BEGIN_NOFIFOCHECK. It works
** with DEBUG glide only. In the non-debug glide, we can still see the
** desktop corruption.
** 
** 46    3/24/99 6:17p Peter
** reduce nop flush for chain downloads
** 
** 45    3/12/99 2:23p Dow
** fixed debug messages
** 
** 44    3/05/99 10:34p Peter
** removed extraneous nops
** 
** 43    3/02/99 2:08p Peter
** 2d nop flushes pixels (although 3d nop should too)
** 
** 42    2/24/99 4:51p Peter
** the rest of tiled texturing
** 
** 41    2/19/99 10:15a Peter
** multi-base sourcing
** 
** 40    2/18/99 5:57p Peter
** sub-page tiled texture sourcing
** 
** 39    2/10/99 2:34p Peter
** removed whacked debugging code
** 
** 38    2/02/99 4:41p Peter
** more tiled cleanup(one more coming), 16 byte texture alignmnet
** 
** 37    1/28/99 4:28p Atai
** fixed non-square texture bug
** 
** 36    1/25/99 6:35p Peter
** tiled texture cleanup
** 
** 35    1/14/99 7:48p Peter
** cleanedup bytes per texel stuff
** 
** 34    1/14/99 4:02p Peter
** fixed internal computation of tiled textures
** 
** 33    12/14/98 6:19p Dow
** Fixed for current surface extension spec
** 
** 32    12/09/98 6:25p Atai
** grTexCombine did the right thing for the un-used TMU.  Initialize the
** 2nd TMU value to take care of "set FX_GLIDE_NUM_TMU=1"
** 
** 31    12/09/98 5:10p Dow
** Fixed debugging
** 
** 30    12/09/98 4:28p Dow
** Fixed debug build
** 
** 29    12/09/98 2:02p Atai
** Added _grTexForceLod back. Set tLOD min = max = 8 for the 2nd TMU by
** default for Avenger to increase single texturing tri fillrate.
** 
** 28    12/03/98 11:26p Dow
** Code 'cleanup' heheh
** 
** 27    11/30/98 6:57p Peter
** windowed texture f*ckage
** 
** 26    11/21/98 10:19a Atai
** fixed test37 grChromaRangeModeExt error and rename functions
** 
** 25    11/15/98 3:21a Atai
** first attempt to make 2 tmus work in H4 glide3x full screen mode, just
** in time check-in for comdex demo. warning: the code is not completed
** yet.
** 
** 24    11/09/98 5:07p Dow
** Texturing from tiled rendered surfaces works now
** 
** 23    10/13/98 5:27p Peter
** 6666 format hack
** 
** 22    9/30/98 12:59p Atai
** use texchromakey and texchromarange shadow state
** 
** 21    8/31/98 5:54p Jdt
** Fix Mipmapping bug in grTexSource, fix grTexNCCTable 
** 
** 20    8/12/98 4:25p Peter
** fixed sense of trilinear fixup
** 
** 19    7/18/98 3:42p Jdt
** Fix texture size checking bug
** 
** 18    7/18/98 10:42a Dow
** Replaced effed-up nest of steaming pile of macros
** 
** 17    7/18/98 12:30a Jdt
** Changes to reflect new shadow register structure.
** 
** 16    7/17/98 10:44a Atai
** fixed grTexNCCTable and clip coords st with aspect ratio
** 
** 15    7/16/98 8:19a Jdt
** fxcmd.h
** 
** 14    6/24/98 10:55a Peter
** gary's trilinear hell bug
** 
** 13    6/22/98 3:35p Peter
** concatenation this way is evil
** 
** 12    5/29/98 11:45a Atai
** 1.added _EXT for extension #defines. 
** 2. change GR_TEXBASE_* values
** 3. Remove GR_TEXCHROMA_ENABLE_SUBSTITUTE_RGB
** 
** 11    5/15/98 4:02p Atai
** fogCoord and texchroma extension for Banshee
** 
** 10    5/08/98 1:40p Peter
** merged Anthony's palette of Taco changes
** 
** 9     5/07/98 4:13p Peter
** lbe texture thing
** 
** 8     5/05/98 2:22p Peter
** merged chroma stuff
** 
** 7     4/30/98 5:01p Peter
** first pass glide3 merge
** 
** 5     4/22/98 4:57p Peter
** glide2x merge
** 
** 4     1/30/98 2:41p Atai
** fix texchroma prototype
** 
** 3     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
** 
** 2     1/18/98 12:03p Atai
** sync to rev 17 spec
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 32    1/13/98 12:42p Atai
 * fixed grtexinfo, grVertexLayout, and draw triangle
 * 
 * 31    1/09/98 6:48p Atai
 * grTexInfo, GR_LOD_* and GR_ASPECT_*
 * 
 * 30    1/08/98 7:09p Peter
 * real hw stuff modulo makefile change
 * 
 * 29    1/08/98 4:58p Atai
 * tex table broadcast, grVertexLayout enable/disable, stq, and some
 * defines
 * 
 * 28    1/07/98 10:22a Peter
 * lod dithering env var
 * 
 * 27    1/05/98 6:06p Atai
 * glide extension stuff
 * 
 * 26    12/18/97 10:52a Atai
 * fixed grGet(GR_VIDEO_POS)
 * 
 * 25    12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 24    12/11/97 4:15p Peter
 * fixed assertions
 * 
 * 23    12/05/97 4:26p Peter
 * watcom warnings
 * 
 * 22    11/20/97 6:57p Dow
 * baseaddress for banshee
 * 
 * 21    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 20    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 19    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 18    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 17    9/05/97 12:31p Peter
 * more reg write grouping
 * 
 * 16    7/08/97 2:48p Peter
 * 
 * 15    6/06/97 10:47a Peter
 * texture downloading, fixed 640x480 dimension, changed cvg dep to be the
 * same as sst1
 * 
 * 14    5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 13    5/21/97 6:05a Peter
 * 
 * 12    3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 11    3/03/97 10:58a Jdt
 * Fixe for 2-pass trilinear
 * 
 * 10    12/23/96 1:37p Dow
 * chagnes for multiplatform glide
**
*/

#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

FxU32
_grTexBytesPerTexel(const GrTextureFormat_t texFormat)
{
#define FN_NAME "_grTexBytesPerTexel"
  FxU32
    retVal = 0x00UL;

  if ((texFormat >= GR_TEXFMT_8BIT) &&
      (texFormat != GR_TEXFMT_RSVD0) &&
      (texFormat != GR_TEXFMT_RSVD1) &&
      (texFormat <= GR_TEXFMT_AP_88)) {
    retVal = (0x01UL << (texFormat >= GR_TEXFMT_16BIT));
  }

  return retVal;
#undef FN_NAME
} /* _grTexBytesPerTexel */

/*-------------------------------------------------------------------
  Function: _grTexCalcBaseAddressTiled
  Date: 12-Dec-98
  Implementor(s): dow
  Description:
  
  Arguments:
  start_address:        This is the actuall physical address of
                        largeLOD.
  largLOD:              This is the largest LOD in the chain
  aspect:               The aspect ratio of the mipmaplevels in the chain
  fmt:                  The texture format
  evenOdd:              Bitmask specifiying presence of even & odd
                        mipmap levels.
  
  Return:
  the Value to be placed into the baseAddress register.
  -------------------------------------------------------------------*/
FxI32
_grTexCalcBaseAddressTiled(GrChipID_t tmu,
                           FxU32 start_address, 
                           GrAspectRatio_t aspect, 
                           GrLOD_t largeLod,
                           GrTextureFormat_t fmt, 
                           FxU32 evenOdd) 
{
#define FN_NAME "_grTexCalcBaseAddressTiled"
  FxU32 
    byteOffset;
  FxI32 
    baseAddress = start_address;

  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME, 88);
  GDBG_INFO_MORE(gc->myLevel, "(%d, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X)\n",
                 tmu, start_address,
                 aspect, largeLod,
                 fmt, evenOdd);

  /* texBaseAddr gets the address of the largest mipmap level that the
   * hw is capable of handling (256 in the current sst1 core
   * derivatives) even if some of the larger levels don't exist.
   * Now, we'll basically calculate the offset (from the beginning) of
   * the largest mipmap level, and subtract that from start_address to
   * calculate the baseAdress. 
   *
   * Capiche?  
   */
  byteOffset = _grTexCalcMipmapLevelOffsetTiled(tmu,
                                                largeLod, GR_LOD_LOG2_256, 
                                                aspect, 
                                                fmt,
                                                evenOdd);

  /* _grTexCalcMipmapLevelOffsetTiled returns a byte offset from the
   * current location back to the start of the 256x256 level. However,
   * if we are in tiled texture mode then we actually need to compute
   * a tiled offset. 
   */
  {
    const struct GrTmuMemInfo*
      memInfo = gc->tmuMemInfo + tmu;
    const FxU32
      offsetY = (byteOffset / memInfo->texStrideBytes),
      offsetX = (byteOffset % memInfo->texStrideBytes),
      tileOffsetY = (offsetY >> HWC_TILE_HEIGHT_SHIFT),
      tileOffsetX = (offsetX >> HWC_TILE_WIDTH_SHIFT),
      tileSlopY = (offsetY & (HWC_TILE_HEIGHT - 0x01UL)),
      tileSlopX = (offsetX & (HWC_TILE_WIDTH - 0x01UL));

    /* Count the # of whole page lines we're offsettting */
    baseAddress -= (((tileOffsetY * memInfo->texStrideTiles) + tileOffsetX) << 
                    (HWC_TILE_HEIGHT_SHIFT + HWC_TILE_WIDTH_SHIFT));

    /* If there is a fractional # of tiles then we round back an
     * entire tile in the dimension of the slop, and then compute the
     * offset within this tile. ick!  
     */
    if (tileSlopX != 0x00UL) {
      baseAddress -= (0x01UL << (HWC_TILE_HEIGHT_SHIFT + HWC_TILE_WIDTH_SHIFT));
      baseAddress += ((0x01UL << HWC_TILE_WIDTH_SHIFT) - tileSlopX);
    } 
    if (tileSlopY != 0x00UL) {
      baseAddress -= (memInfo->texStrideTiles << (HWC_TILE_HEIGHT_SHIFT + HWC_TILE_WIDTH_SHIFT));
      baseAddress += (((0x01UL << HWC_TILE_HEIGHT_SHIFT) - tileSlopY) << HWC_TILE_WIDTH_SHIFT);
    }
  }

  GR_RETURN(baseAddress);
#undef FN_NAME
} /* _grTexCalcBaseAddressTiled */

FxU32
_grTexCalcMipmapLevelOffsetTiled(GrChipID_t tmu,
                                 GrLOD_t lod, GrLOD_t largeLOD, 
                                 GrAspectRatio_t aspect, 
                                 GrTextureFormat_t fmt, 
                                 FxU32 evenOdd)
{
#define FN_NAME "_grTexCalcMipmapLevelOffsetTiled"
  const FxU32
    texelSize = _grTexBytesPerTexel(fmt);
  FxU32 
    byteOffset = 0,
    tileX = 0, 
    tileY = 0;
  
  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME, 90);

  GR_CHECK_F(FN_NAME, texelSize == 0, "invalid texture format");
  GR_CHECK_TMU(FN_NAME, tmu);
  GR_ASSERT(gc->tmuMemInfo[tmu].texTiled);
  
  /*
     Tiled Textures look like this:

  baseAddress regardless of anything else.
     |
     V
     +--------+----*-------------+
     |        |128 |             |
     |  256   |____|             |
     |        |64|-| <-32-1      |
     +--------+------------------+
     |<----------Stride--------->| 

     Expanded View of 32-1:
     (pretend they're square--this is just intended to demonstrate the layout)

     +-------------+-------+-------------+
     |             |    16 |             |
     |             |       |             |
     |     32      +---+---+
     |             |8  |
     |             +---+
     |             |4|
     |             |2
     |             |1
     +-------------+---------------------+
     |<-----------------------------stride much bigger than width --------->|

     */

  /* LOOK AT THE PICTURE!!!!!
   * We're trying to work our way from the upper-left-hand corner of
   * largeLOD to the upper-left-hand corner of the 256 LOD.  
   */
  {
    const FxU32
      texStrideBytes = gc->tmuMemInfo[tmu].texStrideBytes;
    GrLOD_t 
      level;

    for(level = lod; level < largeLOD; level++) {
      switch (level) {
      case GR_LOD_LOG2_256:
        break;
        
      case GR_LOD_LOG2_128:
        /* Offset the width of the 256 LOD */
        if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
          tileX += WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_256);
        }
        break;
        
      case GR_LOD_LOG2_64:
        /* Offset the height of the 128 LOD */
        if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
          tileY += HEIGHT_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_128);
        }
        break;
        
      case GR_LOD_LOG2_32:
        /* Offset the width of the 64 LOD */
        if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
          tileX += WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_64);
        }
        break;
        
      case GR_LOD_LOG2_16:
        /* Offset the width of the 32 LOD */
        if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {    
          tileX += WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_32);
        }
        break;
        
      case GR_LOD_LOG2_8:
      case GR_LOD_LOG2_4:
      case GR_LOD_LOG2_2:
      case GR_LOD_LOG2_1:
        /* For these, we offset the LOD height * texture stride */
        if (evenOdd & (level & 0x1 ? GR_MIPMAPLEVELMASK_ODD : GR_MIPMAPLEVELMASK_EVEN)) {
          tileY += HEIGHT_BY_ASPECT_LOD(aspect, level + 1);
        }
        break;
      }
    }

    byteOffset = ((tileX * texelSize) +
                  (tileY * texStrideBytes));
  }

  return byteOffset;
#undef FN_NAME  
} /* _grTexCalcMipmapLevelOffsetTiled */

#if GLIDE_CHECK_TRILINEAR
/* !! TACO - Come back to this code on Avenger */

/* Make sure that the trilinear blending bits are set in a
 * consistent manner across the tmu's.  This only really matters if
 * we have multiple tmu's, but this state is really setup across
 * multiple calls (grTexCombine, grTexMipMapMode, and
 * grTexSource).  
 * 
 * NB: This must be called after the shadows are updated because
 * _grTexCheckTriLinear() will use the shadow register values
 * to determine the current state of trilinearness. 
 *
 * FixMe: This should eventually get merged in w/ the texture
 * statemonster. When/If that ever happens.
 */
static void
_grTexCheckTriLinear(GrChipID_t tmu)
{
#define FN_NAME "_grTexCheckTriLinear"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);
  GDBG_INFO_MORE(gc->myLevel, "(0x%X)\n", tmu);

#if (GLIDE_NUM_TMU > 2)
#error "(GLIDE_NUM_TMU > 2): Write this code"
#endif

/* NB: The factor mask needs to include the factor bits as
 * well as the reverse bit so that we can differentiate
 * between the lodfrac and the (1 - lodfrac) case.
 */
#define SST_TC_FACTOR_MASK (SST_TC_MSELECT | SST_TC_REVERSE_BLEND)

#define SST_MIPMAP_LEVEL_MASK (SST_LOD_ODD | SST_LOD_TSPLIT)

  /* Is this tmu on? */
  if (!gc->tmuLodDisable[tmu]) {
    const struct PerTmuState* tmu0 = gc->state.per_tmu;
    const struct PerTmuState* tmu1 = gc->state.per_tmu + 1;
    const struct PerTmuState* curTmu = gc->state.per_tmu + tmu;
    const struct PerTmuState* otherTmu = gc->state.per_tmu + !tmu;

    /* This is the 'tricky' state where we have to manage the states
     * of teh tmu's together to get the correct effect. Within this
     * state there are two sub-states: two-pass trilinear and single
     * pass using both tmu's w/ split levels.  
     *
     * Case 1 - TMU set for lod blending and has both even/odd levels
     */
    if (((curTmu->textureMode & SST_TRILINEAR) == SST_TRILINEAR) &&
        (curTmu->evenOdd == GR_MIPMAPLEVELMASK_BOTH)) {
      
      /* Check the 'other' tmu to see if it is active, if not then we
       * are doing two pass trilinear so check that we have the
       * correct even/odd things set based on the factor where one
       * pass will use GR_COMBINE_FACTOR_LOD_FRACTION and the other
       * will use (1 - GR_COMBINE_FACTOR_LOD_FRACTION).
       */
      if (gc->tmuLodDisable[!tmu]) {
        /* NB: In this case the rgb/alpha factors need to match so
         * checking for only one of them is fine.
         */
        const FxU32 levelMask = (((curTmu->textureMode & SST_TC_FACTOR_MASK) == SST_TC_MLODFRAC)
                                 ? SST_LOD_ODD
                                 : 0);

        GDBG_INFO(gc->myLevel, FN_NAME": Two-pass trilinear fixup (0x%X) : tLOD : (0x%X : 0x%X)\n",
                  tmu, curTmu->tLOD, ((curTmu->tLOD & ~SST_MIPMAP_LEVEL_MASK) | levelMask));
        
        GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
        GR_SET(eChipTMU0, SST_TMU(hw, 0), tLOD, 
               ((curTmu->tLOD & ~SST_MIPMAP_LEVEL_MASK) | levelMask));
        GR_CHECK_SIZE();
      } else {
        /* One pass trilinear
         *
         * Make sure that the tmu's have the levels split across the
         * two tmu's. There are basically three cases based on what
         * the user might have already set on the other tmu.
         */        
        if (((tmu0->textureMode & SST_TC_BLEND) == SST_TC_BLEND) &&
            ((tmu1->textureMode & SST_TC_REPLACE) == SST_TC_REPLACE)) {
          FxU32  evenOdd[GLIDE_NUM_TMU];
          FxU32* curEvenOdd = evenOdd + tmu;
          
          {
            FxU32 i;
            
            for(i = 0; i < GLIDE_NUM_TMU; i++) {
              evenOdd[i] = gc->state.shadow[i].tLOD & SST_LOD_ODD;
            }
          }
              
          /* 1 - The other tmu already has the even levels. */
          if ((otherTmu->evenOdd == GR_MIPMAPLEVELMASK_EVEN) &&
              (curTmu->evenOdd != GR_MIPMAPLEVELMASK_ODD)) {
            *curEvenOdd = SST_LOD_ODD;
            goto __tmuRegUpdate;
          }

          /* 2 - The other tmu already has the odd levels. */
          if ((otherTmu->evenOdd == GR_MIPMAPLEVELMASK_ODD) &&
              (curTmu->evenOdd != GR_MIPMAPLEVELMASK_EVEN)) {
            *curEvenOdd = 0;
            goto __tmuRegUpdate;
          }

          /* 3 - The other tmu already has both the levels. If the
           * downstream tmu's factor is lodFrac then the upstream tmu
           * needs to be (1 - lodFrac) and vice-versa.  
           */
          if (otherTmu->evenOdd == GR_MIPMAPLEVELMASK_BOTH) {
            evenOdd[0] = (((tmu0->textureMode & SST_TC_FACTOR_MASK) == SST_TC_MLODFRAC)
                          ? SST_LOD_ODD
                          : 0);
            evenOdd[1] = ~evenOdd[0] & SST_LOD_ODD;

            goto __tmuRegUpdate;
          }

          /* Do the register updates */
          if (0) {
        __tmuRegUpdate:
            GDBG_INFO(gc->myLevel, FN_NAME": Tri-linear fixup (0x%X : 0x%X) : (0x%X : 0x%X)\n",
                      tmu0->tLOD, tmu1->tLOD,
                      ((tmu0->tLOD & ~SST_MIPMAP_LEVEL_MASK) | evenOdd[0]),
                      ((tmu1->tLOD & ~SST_MIPMAP_LEVEL_MASK) | evenOdd[1]));

            GR_SET_EXPECTED_SIZE((sizeof(FxU32) << 1), 2);
            {
              GR_SET(eChipTMU0, SST_TMU(hw, 0), tLOD, 
                     ((tmu0->tLOD & ~SST_MIPMAP_LEVEL_MASK) | evenOdd[0]));
              GR_SET(eChipTMU1, SST_TMU(hw, 1), tLOD, 
                     ((tmu1->tLOD & ~SST_MIPMAP_LEVEL_MASK) | evenOdd[1]));
            }
            GR_CHECK_SIZE();
          }
        }
      }
    }
  }
  
  GR_END();
#undef FN_NAME
} /* _grTexCheckTriLinear*/
#endif /* GLIDE_CHECK_TRILINEAR */

/*---------------------------------------------------------------------------
** grTexClampMode
*/
GR_ENTRY(grTexClampMode, void, 
         (GrChipID_t tmu, GrTextureClampMode_t s_clamp_mode, GrTextureClampMode_t t_clamp_mode))
{
#define FN_NAME "grTexClampMode"
  FxU32 
    texturemode,
    tLod,
    clampMode =(((s_clamp_mode == GR_TEXTURECLAMP_CLAMP) ? SST_TCLAMPS : 0) |
                ((t_clamp_mode == GR_TEXTURECLAMP_CLAMP) ? SST_TCLAMPT : 0)),
    mirrorMode =(((s_clamp_mode == GR_TEXTURECLAMP_MIRROR_EXT) ? SST_TMIRRORS : 0) |
                 ((t_clamp_mode == GR_TEXTURECLAMP_MIRROR_EXT) ? SST_TMIRRORT : 0));

  GR_BEGIN_NOFIFOCHECK("grTexClampMode",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d, %d,%d)\n",tmu,s_clamp_mode,t_clamp_mode);
  GR_CHECK_TMU(FN_NAME, tmu);

  /*
  ** Assuming Banshee family has max tmu size of 2
  */
  texturemode  = gc->state.shadow.tmuState[tmu].textureMode;
  texturemode &= ~(SST_TCLAMPS | SST_TCLAMPT);
  texturemode |=  clampMode;

  tLod = gc->state.shadow.tmuState[tmu].tLOD;
  tLod &= ~(SST_TMIRRORS | SST_TMIRRORT);        /* disable mirror */
  if (mirrorMode) tLod |= mirrorMode;

  GR_SET_EXPECTED_SIZE(sizeof(FxU32)<<1, 2);
  GR_SET((0x02UL << tmu), SST_TMU(hw, tmu), textureMode, texturemode);
  GR_SET((0x02 << tmu), SST_TMU(hw, tmu), tLOD, tLod);

  gc->state.shadow.tmuState[tmu].textureMode = texturemode;
  gc->state.shadow.tmuState[tmu].tLOD        = tLod;

  GR_END();
#undef FN_NAME
} /* grTexClampMode */

/*---------------------------------------------------------------------------
** grTexCombine
*/
GR_ENTRY(grTexCombine, void, 
         (GrChipID_t tmu,
          GrCombineFunction_t rgb_function, GrCombineFactor_t rgb_factor,  
          GrCombineFunction_t alpha_function, GrCombineFactor_t alpha_factor, 
          FxBool rgb_invert, FxBool alpha_invert))
{
#define FN_NAME "grTexCombine"
  FxU32 texturemode;
  FxU32 tLod;
  FxU32 tmuMask;
  FxBool localColorP = FXFALSE;
  FxBool localAlphaP = FXFALSE;

  GR_BEGIN_NOFIFOCHECK("grTexCombine",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d, %d,%d, %d,%d, %d,%d)\n",
                 tmu, rgb_function, rgb_factor, 
                 alpha_function, alpha_factor,
                 rgb_invert, alpha_invert);
  GR_CHECK_TMU(FN_NAME, tmu);
  GR_CHECK_W(FN_NAME,
               rgb_function < GR_COMBINE_FUNCTION_ZERO ||
               rgb_function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA,
               "unsupported texture color combine function");
  GR_CHECK_W(FN_NAME,
               alpha_function < GR_COMBINE_FUNCTION_ZERO ||
               alpha_function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA,
               "unsupported texture alpha combine function");
  GR_CHECK_W(FN_NAME,
               (rgb_factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
               (rgb_factor & 0x7) > GR_COMBINE_FACTOR_LOD_FRACTION ||
               rgb_factor > GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
               "unsupported texture color combine scale factor");
  GR_CHECK_W(FN_NAME,
               (alpha_factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
               (alpha_factor & 0x7) > GR_COMBINE_FACTOR_LOD_FRACTION ||
               alpha_factor > GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
               "unsupported texture alpha combine scale factor");

  /* tmuMask tells grColorCombineFunction what to turn off an on if 
     the ccFunc requires texture mapping */
  texturemode = gc->state.shadow.tmuState[tmu].textureMode;
  texturemode &= ~(SST_TCOMBINE | SST_TACOMBINE);
  tLod = gc->state.shadow.tmuState[tmu].tLOD;
  tLod &= ~(SST_LOD_ODD);

  tmuMask = GR_TMUMASK_TMU0 << tmu;
  gc->state.tmuMask &= ~tmuMask;

  /* setup scale factor bits */
  texturemode |= (rgb_factor & 0x7) << SST_TC_MSELECT_SHIFT;
  if (!(rgb_factor & 0x8)) texturemode |= SST_TC_REVERSE_BLEND;

  if (((rgb_factor & 0x7) == GR_COMBINE_FACTOR_LOCAL) ||
       ((rgb_factor & 0x7) == GR_COMBINE_FACTOR_LOCAL_ALPHA))
    gc->state.tmuMask |= tmuMask;

  texturemode |= (alpha_factor & 0x7) << SST_TCA_MSELECT_SHIFT;
  if (!(alpha_factor & 0x8)) texturemode |= SST_TCA_REVERSE_BLEND;

  if (((alpha_factor & 0x7) == GR_COMBINE_FACTOR_LOCAL) ||
       ((alpha_factor & 0x7) == GR_COMBINE_FACTOR_LOCAL_ALPHA))
    gc->state.tmuMask |= tmuMask;

  /* setup invert output bits */
  if (rgb_invert) texturemode |= SST_TC_INVERT_OUTPUT;
  if (alpha_invert) texturemode |= SST_TCA_INVERT_OUTPUT;

  /* setup core color combine unit bits */
  switch (rgb_function) {
  case GR_COMBINE_FUNCTION_ZERO:
    texturemode |= SST_TC_ZERO_OTHER;
    localColorP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_LOCAL:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    localColorP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    localColorP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER:
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
    texturemode |= SST_TC_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
    texturemode |= SST_TC_SUB_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TC_SUB_CLOCAL | SST_TC_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_SUB_CLOCAL | SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_SUB_CLOCAL | SST_TC_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    localColorP = ((rgb_factor & 0x07UL) != GR_COMBINE_FACTOR_OTHER_ALPHA);
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_SUB_CLOCAL | SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    localColorP = ((rgb_factor & 0x07UL) != GR_COMBINE_FACTOR_OTHER_ALPHA);
    break;
  }
  
  switch (alpha_function) {
  case GR_COMBINE_FUNCTION_ZERO:
    texturemode |= SST_TCA_ZERO_OTHER;
    localAlphaP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_LOCAL:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    localAlphaP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    localAlphaP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER:
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
    texturemode |= SST_TCA_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
    texturemode |= SST_TCA_SUB_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TCA_SUB_CLOCAL | SST_TCA_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_SUB_CLOCAL | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_SUB_CLOCAL | SST_TCA_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    localAlphaP = ((alpha_factor & 0x07UL) != GR_COMBINE_FACTOR_OTHER_ALPHA);
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_SUB_CLOCAL | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    localAlphaP = ((alpha_factor & 0x07UL) != GR_COMBINE_FACTOR_OTHER_ALPHA);
    break;
  }
  
  /* Hack to enable TWO-PASS Trilinear */
  if (texturemode & SST_TRILINEAR) {
    if ((texturemode & SST_TC_ZERO_OTHER) &&
        (texturemode & SST_TC_BLEND_LODFRAC) &&
        !(texturemode & SST_TC_REVERSE_BLEND)) {
      tLod |= SST_LOD_ODD;
    }
  }
  tLod |= _gr_evenOdd_xlate_table[gc->state.per_tmu[tmu].evenOdd];

  /* Update shadows */
  gc->state.shadow.tmuState[tmu].textureMode = texturemode;
  gc->state.shadow.tmuState[tmu].tLOD        = tLod;

  /* If the state of a tmu changes from active then make sure that it
   * is addressing some valid tiny texture so taht it does not spend
   * time thrashing on a large texture access.
   *
   * NB: We don't update the shadow here so the other bits in the
   * register shadow should be unchanged.
   */
  {
    const FxI32 upstreamTmu = tmu + 1;

    if (upstreamTmu < gc->num_tmu) {
      if (localColorP && localAlphaP) {
        SstRegs* tmuHw = SST_TMU(hw, upstreamTmu);

        GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
        GR_SET((0x02UL << upstreamTmu), tmuHw, tLOD, SST_TLOD_MINMAX_INT(GR_LOD_LOG2_1, GR_LOD_LOG2_1));
        GR_CHECK_SIZE();
        
        gc->tmuLodDisable[upstreamTmu] = FXTRUE;
        
        GDBG_INFO(gc->myLevel, FN_NAME": Disabling tmu(%ld) : (0x%X : 0x%X)\n", 
                  upstreamTmu, rgb_function, alpha_function);
      } else if (gc->tmuLodDisable[upstreamTmu]) {
        SstRegs* tmuHw = SST_TMU(hw, upstreamTmu);
        
        GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
        GR_SET((0x02UL << upstreamTmu), tmuHw, tLOD, gc->state.shadow.tmuState[upstreamTmu].tLOD);
        GR_CHECK_SIZE();
        
        gc->tmuLodDisable[upstreamTmu] = FXFALSE;
      }
    }
    /* If this tmu is inactive then also do the reset for the register
     * write, but not in the shadow so that the remaining bits are
     * retained.
     */
    if ((gc->state.tmuMask & tmuMask) == 0) {
      tLod = SST_TLOD_MINMAX_INT(GR_LOD_LOG2_1, GR_LOD_LOG2_1);
    }
  }

  /* update register */
  {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    REG_GROUP_BEGIN((0x02 << tmu), textureMode, 2, 0x3);
    {
      REG_GROUP_SET(tmuHw, textureMode , texturemode);
      REG_GROUP_SET(tmuHw, tLOD, tLod);
    }
    REG_GROUP_END();
  }

#if GLIDE_CHECK_TRILINEAR
  /* Make sure that the trilinear blending bits are set in a
   * consistent manner across the tmu's.  This only really matters if
   * we have multiple tmu's, but this state is really setup across
   * multiple calls (grTexCombine, grTexMipMapMode, and
   * grTexSource).  
   * 
   * NB: This must happen after the shadows are updated because
   * _grTexCheckTriLinear() will use the shadow register values
   * to determine the current state of trilinearness.
   */
  if (gc->num_tmu > 1) _grTexCheckTriLinear(tmu);
#endif /* GLIDE_CHECK_TRILINEAR */

  /* update paramIndex */
  _grUpdateParamIndex();

  GR_END();
} /* grTexCombine */

/*
** _grTexDetailControl, NOTE: its up to caller to account for bytes
*/
GR_DDFUNC(_grTexDetailControl, void, (GrChipID_t tmu, FxU32 detail))
{
  GR_BEGIN("_grTexDetailControl",88,4, 1);

  GR_CHECK_TMU("_grTexDetailControl", tmu);

  GR_SET((0x02UL << tmu), SST_TMU(hw, tmu), tDetail, detail);
  gc->state.shadow.tmuState[tmu].tDetail = detail;

  GR_END();
#undef FN_NAME
} /* _grTexDetailControl */

/*---------------------------------------------------------------------------
** grTexFilterMode
*/

GR_ENTRY(grTexFilterMode, void, 
         (GrChipID_t tmu, GrTextureFilterMode_t minfilter, GrTextureFilterMode_t magfilter))
{
#define FN_NAME "grTexFilterMode"
  FxU32 texMode;

  GR_BEGIN("grTexFilterMode",99,4, 1);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d)\n",tmu,minfilter,magfilter);
  GR_CHECK_TMU(FN_NAME, tmu);

  texMode  = gc->state.shadow.tmuState[tmu].textureMode;
  texMode &= ~(SST_TMINFILTER | SST_TMAGFILTER);
  texMode |= (minfilter == GR_TEXTUREFILTER_BILINEAR ? SST_TMINFILTER : 0) |
             (magfilter == GR_TEXTUREFILTER_BILINEAR ? SST_TMAGFILTER : 0);

  GR_SET((0x02UL << tmu), SST_TMU(hw, tmu), textureMode, texMode);
  gc->state.shadow.tmuState[tmu].textureMode = texMode;

  GR_END();
#undef FN_NAME
} /* grTexFilterMode */

/*---------------------------------------------------------------------------
** grTexLodBiasValue
*/

GR_ENTRY(grTexLodBiasValue, void, 
         (GrChipID_t tmu, float fvalue))
{
#define FN_NAME "grTexLodBiasValue"
  FxU32 tLod;
  FxI32 lodBias;
  
  GR_BEGIN("grTexLodBiasValue",88,4, 1);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%g)\n",tmu,fvalue);
  GR_CHECK_TMU(FN_NAME, tmu);

  lodBias = _grTexFloatLODToFixedLOD(fvalue);
  /* Sign extend it. */
  lodBias = ((lodBias << (32-6)) >> (32-6));
  if(lodBias > 0x1f) lodBias = 0x1f;
  else if(lodBias < -0x20) lodBias = -0x20;
  /* Mask it back off. */
  lodBias &= 0x3f;
  tLod = gc->state.shadow.tmuState[tmu].tLOD;
  tLod &= ~(SST_LODBIAS);
  tLod |= lodBias << SST_LODBIAS_SHIFT;

  GR_SET((0x02 << tmu), SST_TMU(hw, tmu), tLOD, tLod);
  gc->state.shadow.tmuState[tmu].tLOD = tLod;

  GR_END();
#undef FN_NAME
} /* grTexLodBiasValue */

/*-------------------------------------------------------------------
  Function: grTexMipMapMode
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Sets the mip map mode for the specified TMU
    "Ex" because glide's grTexMipMapMode is inadequate for 
    low level texture memory management
  Arguments:
    tmu       - tmu to update
    mmMode   - mipmap mode 
      One of:
        GR_MIPMAP_DISABLE
        GR_MIPMAP_NEAREST
        GR_MIPMAP_NEAREST_DITHER
    lodBlend - enable lodBlending
      FXTRUE  - enabled
      FXFALSE - disabled
  Return:
  none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexMipMapMode, void, 
         (GrChipID_t tmu, GrMipMapMode_t mmMode, FxBool lodBlend))
{
#define FN_NAME "grTexMipMapMode"
  FxU32
    tLod,
    texMode;

  GR_BEGIN_NOFIFOCHECK("grTexMipMapMode",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d)\n",tmu,mmMode,lodBlend);
  GR_CHECK_TMU(FN_NAME, tmu);
  
  /*--------------------------------------------------------------
    Get Current tLod and texMode register values
    --------------------------------------------------------------*/
  tLod  = gc->state.shadow.tmuState[tmu].tLOD;
  texMode = gc->state.shadow.tmuState[tmu].textureMode;

  /*--------------------------------------------------------------
    Clear LODMIN, LODMAX and LODDITHER
    --------------------------------------------------------------*/
  tLod    &= ~(SST_LODMIN | SST_LODMAX | SST_LOD_ODD);
  texMode &= ~(SST_TLODDITHER | SST_TRILINEAR);

  /*--------------------------------------------------------------
    Encode Mipmap Mode Bits
    --------------------------------------------------------------*/
  switch (mmMode) {
  case GR_MIPMAP_DISABLE:
    /*----------------------------------------------------------
      To disable mipmapping set the min and max lods to the same
      value
      ----------------------------------------------------------*/
    tLod |= SST_TLOD_MINMAX_INT(gc->state.per_tmu[tmu].largeLod,
                                gc->state.per_tmu[tmu].largeLod);
    break;

  case GR_MIPMAP_NEAREST_DITHER:
    if (gc->state.allowLODdither) texMode |= SST_TLODDITHER;
    /* intentional fall-through to set lodmin and lodmax values */

  case GR_MIPMAP_NEAREST:
    /*----------------------------------------------------------
      Set LODMIN and LODMAX in the tLod register to the 
      actual min and max LODs of the current texture.
      ----------------------------------------------------------*/
    tLod |= SST_TLOD_MINMAX_INT(gc->state.per_tmu[tmu].largeLod,
                                gc->state.per_tmu[tmu].smallLod);
    break;

  default:
    GrErrorCallback("grTexMipMapMode:  invalid mode passed", FXFALSE);
    break;
  }
  gc->state.per_tmu[tmu].mmMode = mmMode;

  /* Force LOD dithering if the user asked for it.
   *
   * NB: There is a performance hit for this, but it does
   * look better.
   */
  texMode |= _GlideRoot.environment.texLodDither;

  /*--------------------------------------------------------------
    Fix trilinear and evenOdd bits -

    This is a bit of a hack to make two pass trilinear work with
    full textures.  The assumption here is that the only reason
    you would ever set up Multiply by LODFRAC w/o REVERSE BLEND
    is for the ODD pass of trilinear.  
    --------------------------------------------------------------*/
  if (lodBlend) {
    /* If we're doing trilinear for real then nuke the lod dithering
     * at the same time because it just looks bad.  
     */
    texMode = ((texMode & ~SST_TLODDITHER) | SST_TRILINEAR);

    if ((texMode & SST_TC_ZERO_OTHER) &&
        (texMode & SST_TC_BLEND_LODFRAC) &&
        !(texMode & SST_TC_REVERSE_BLEND)) {
        tLod |= SST_LOD_ODD;
    }
  }
  tLod |= _gr_evenOdd_xlate_table[gc->state.per_tmu[tmu].evenOdd];
  
  /*--------------------------------------------------------------
    Write State To Hardware and Update Glide Shadow State
    --------------------------------------------------------------*/
  hw = SST_TMU(hw, tmu);
  REG_GROUP_BEGIN((0x02UL << tmu), textureMode, 2, 0x03);
  {
    REG_GROUP_SET(hw, textureMode , texMode);
    REG_GROUP_SET(hw, tLOD , tLod);
  }
  REG_GROUP_END();

  gc->state.shadow.tmuState[tmu].tLOD        = tLod;
  gc->state.shadow.tmuState[tmu].textureMode = texMode;

#if GLIDE_CHECK_TRILINEAR
  /* Make sure that the trilinear blending bits are set in a
   * consistent manner across the tmu's.  This only really matters if
   * we have multiple tmu's, but this state is really setup across
   * multiple calls (grTexCombine, grTexMipMapMode, and
   * grTexSource).  
   * 
   * NB: This must happen after the shadows are updated because
   * _grTexCheckTriLinear() will use the shadow register values
   * to determine the current state of trilinearness.
   */
  if (gc->num_tmu > 1) _grTexCheckTriLinear(tmu);
#endif /* GLIDE_CHECK_TRILINEAR */

  GR_END();
#undef FN_NAME
} /* grTexMipMapMode */

/*-------------------------------------------------------------------
  Function: grTexMinAddress
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Returns address of start of texture ram for a TMU
  Arguments:
    tmu
  Return:
    integer texture base address, this pointer is not to be dereferenced
    by the application, it is on to be used by grTexDownload(),
    and grTExDownloadLevel()
  -------------------------------------------------------------------*/
/*-------------------------------------------------------------------
  Function: grTexNCCTable
  Date: 6/3
  Implementor(s): jdt
  Library: glide
  Description:
    select one of the two NCC tables
  Arguments:
    tmu - which tmu
    table - which table to select
        One of:
            GR_TEXTABLE_NCC0
            GR_TEXTABLE_NCC1
            GR_TEXTABLE_PALETTE
  Return:
    none
  -------------------------------------------------------------------*/
GR_ENTRY(grTexNCCTable, void, 
         (GrNCCTable_t table))
{
#define FN_NAME "grTexNCCTable"
  FxU32 texMode;
  FxI32 i;
  
  GR_BEGIN("grTexNCCTable",88,4*GLIDE_NUM_TMU, GLIDE_NUM_TMU);
  GDBG_INFO_MORE(gc->myLevel,"\n");
  GR_CHECK_F(myName, table > GR_TEXTABLE_PALETTE, "invalid ncc table specified");

  for(i = 0; i < gc->num_tmu; i++) {
    /* Update local state */
    gc->state.per_tmu[i].nccTable = table;
  
    /* Grab shadow texMode, update TexMode, update shadow/real register */
    texMode  = gc->state.shadow.tmuState[i].textureMode;
    texMode &= ~(SST_TNCCSELECT);
    if (table == GR_TEXTABLE_NCC1)
      texMode |= SST_TNCCSELECT;
    else 
      texMode &= ~(SST_TNCCSELECT);
    
    GR_SET((0x02UL << i), SST_TMU(hw, i), textureMode, texMode);
    gc->state.shadow.tmuState[i].textureMode = texMode;
  }
  
  GR_END();
#undef FN_NAME
} /* grTexNCCTable */

/*-------------------------------------------------------------------
  Function: grTexSource
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Sets up the current texture for texture mapping on the specified
    TMU.
  Arguments:
    tmu          - which tmu
    startAddress - texture start address
    evenOdd  - which set of mipmap levels have been downloaded for
                the selected texture
                One of:
                  GR_MIPMAPLEVELMASK_EVEN 
                  GR_MIPMAPLEVELMASK_ODD
                  GR_MIPMAPLEVELMASK_BOTH
    info         - pointer to GrTexInfo structure containing
                   texture dimensions
  Return:
    none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexSource, void, 
         (GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info))
{
#define FN_NAME "grTexSource"
  FxU32 baseAddress, texMode, tLod;

  GR_BEGIN_NOFIFOCHECK("grTexSource", 88);
  GDBG_INFO_MORE(gc->myLevel, 
                 "(%d, 0x%X, 0x%X, [%d:%d %d %d 0x%X])\n",
                 tmu, startAddress, evenOdd, 
                 info->smallLodLog2, info->largeLodLog2,
                 info->aspectRatioLog2,
                 info->format,
                 info->data);

  GR_CHECK_TMU(FN_NAME, tmu);
  GR_CHECK_COMPATABILITY(FN_NAME, 
                         startAddress >= gc->tmu_state[tmu].total_mem, 
                         "invalid startAddress");

  GR_CHECK_COMPATABILITY(FN_NAME,
                         (startAddress + _grTexTextureMemRequired(info->smallLodLog2, info->largeLodLog2,
                                                                  info->aspectRatioLog2,
                                                                  info->format,
                                                                  evenOdd,
                                                                  FXTRUE) > gc->tmu_state[tmu].total_mem),
                         "insufficient texture ram at startAddress");
  GR_CHECK_F(FN_NAME, evenOdd > 0x3 || evenOdd == 0, "evenOdd mask invalid");
  GR_CHECK_F(FN_NAME, !info, "invalid info pointer");

  /*-------------------------------------------------------------
    Update Texture Unit State
    -------------------------------------------------------------*/
  gc->state.per_tmu[tmu].smallLod = G3_LOD_TRANSLATE(info->smallLodLog2);
  gc->state.per_tmu[tmu].largeLod = G3_LOD_TRANSLATE(info->largeLodLog2);
  gc->state.per_tmu[tmu].evenOdd  = evenOdd;
  
  /* Calculate Base Address
   * texBaseAddr gets the address of the largest mipmap level that the
   * hw is capable of handling (256 in the current sst1 core
   * derivatives) even if some of the larger levels don't exist.  Now,
   * we'll basically calculate the offset (from the beginning) of the
   * largest mipmap level, and subtract that from start_address to
   * calculate the baseAdress.
   *
   * Capiche?  
   */
  {
    const struct GrTmuMemInfo*
      memInfo = gc->tmuMemInfo + tmu;

    if (memInfo->texTiled) {
      const FxI32 
        baseAddressAdj = _grTexCalcBaseAddressTiled(tmu, startAddress,
                                                    info->aspectRatioLog2,
                                                    info->largeLodLog2, 
                                                    info->format, 
                                                    evenOdd);
      
      baseAddress = memInfo->tramOffset + baseAddressAdj;
      baseAddress &= SST_TEXTURE_ADDRESS;

      baseAddress |= SST_TEXTURE_IS_TILED;
      baseAddress |= (memInfo->texStrideTiles << SST_TEXTURE_TILESTRIDE_SHIFT);

      GR_CHECK_F(FN_NAME, (gc->windowed && (startAddress != 0)),
                 "Surface extension only suports startAddress 0\n");
    }  else {
      baseAddress = (FxI32)_grTexCalcBaseAddress(startAddress,
                                                 G3_LOD_TRANSLATE(info->largeLodLog2),
                                                 G3_ASPECT_TRANSLATE(info->aspectRatioLog2),
                                                 info->format,
                                                 evenOdd);
      baseAddress += memInfo->tramOffset;
      baseAddress &= SST_TEXTURE_ADDRESS;
    }
  }
  
  /*-------------------------------------------------------------
    Update Texture Mode
    -------------------------------------------------------------*/
  {
    GrTextureFormat_t format = info->format;
    
    texMode = gc->state.shadow.tmuState[tmu].textureMode;
    texMode &= ~SST_TFORMAT;
    
    /* Adjust texture format to match the current texture table type */
    if (format == GR_TEXFMT_P_8) {
      format = ((gc->state.tex_table == GR_TEXTABLE_PALETTE_6666_EXT)
                ? GR_TEXFMT_P_8_RGBA
                : GR_TEXFMT_P_8);
    }
    
    texMode |= (SST_TPERSP_ST | 
                SST_TCLAMPW |
                (format << SST_TFORMAT_SHIFT));
  }
    
  /*-------------------------------------------------------------
    Compute TLOD (keep LODBIAS in tact)
    -------------------------------------------------------------*/
  {
    const FxU32 
      smallLOD    = G3_LOD_TRANSLATE(info->smallLodLog2),
      largeLOD    = G3_LOD_TRANSLATE(info->largeLodLog2),
      aspectRatio = G3_ASPECT_TRANSLATE(info->aspectRatioLog2);
    
    tLod = gc->state.shadow.tmuState[tmu].tLOD;
    tLod &= ~(SST_LODMIN | SST_LODMAX | SST_LOD_ASPECT |
      SST_LOD_TSPLIT | SST_LOD_ODD | SST_LOD_S_IS_WIDER);

    if (gc->state.per_tmu[tmu].mmMode == GR_MIPMAP_DISABLE )
      tLod |= SST_TLOD_MINMAX_INT(largeLOD, largeLOD);
    else
      tLod |= SST_TLOD_MINMAX_INT(largeLOD, smallLOD);

    tLod |= _gr_evenOdd_xlate_table[evenOdd];
    tLod |= _gr_aspect_xlate_table[aspectRatio];
  }

  /* Write relevant registers out to hardware */
  hw = SST_TMU(hw, tmu);
  REG_GROUP_BEGIN((0x02UL << tmu), textureMode, 3, 0x0B);
  {
    REG_GROUP_SET(hw, textureMode, texMode);
    REG_GROUP_SET(hw, tLOD,        tLod);
    REG_GROUP_SET(hw, texBaseAddr, baseAddress);
  }
  REG_GROUP_END();
  
  /* update shadows */
  gc->state.shadow.tmuState[tmu].textureMode = texMode; 
  gc->state.shadow.tmuState[tmu].tLOD        = tLod;
  gc->state.shadow.tmuState[tmu].texBaseAddr = baseAddress;

  /* Synch flush data w/ new state */
  gc->tmuMemInfo[tmu].postPacket[1] = ~baseAddress;
  gc->tmuMemInfo[tmu].postPacket[5] = baseAddress;

#if GLIDE_CHECK_TRILINEAR
  /* Make sure that the trilinear blending bits are set in a
   * consistent manner across the tmu's.  This only really matters if
   * we have multiple tmu's, but this state is really setup across
   * multiple calls (grTexCombine, grTexMipMapMode, and
   * grTexSource).  
   * 
   * NB: This must happen after the shadows are updated because
   * _grTexCheckTriLinear() will use the shadow register values
   * to determine the current state of trilinearness.
   */
  if (gc->num_tmu > 1) _grTexCheckTriLinear(tmu);
#endif /* GLIDE_CHECK_TRILINEAR */

  /*
  ** Update s and t scale for clip coordinates
  */
  switch (info->aspectRatioLog2) {
  case GR_ASPECT_LOG2_8x1:
    gc->state.per_tmu[tmu].s_scale = 256.f;
    gc->state.per_tmu[tmu].t_scale = 32.f;
    break;
  case GR_ASPECT_LOG2_4x1:
    gc->state.per_tmu[tmu].s_scale = 256.f;
    gc->state.per_tmu[tmu].t_scale = 64.f;
    break;
  case GR_ASPECT_LOG2_2x1:
    gc->state.per_tmu[tmu].s_scale = 256.f;
    gc->state.per_tmu[tmu].t_scale = 128.f;
    break;
  case GR_ASPECT_LOG2_1x1:
    gc->state.per_tmu[tmu].s_scale = 256.f;
    gc->state.per_tmu[tmu].t_scale = 256.f;
    break;
  case GR_ASPECT_LOG2_1x2:
    gc->state.per_tmu[tmu].s_scale = 128.f;
    gc->state.per_tmu[tmu].t_scale = 256.f;
    break;
  case GR_ASPECT_LOG2_1x4:
    gc->state.per_tmu[tmu].s_scale = 64.f;
    gc->state.per_tmu[tmu].t_scale = 256.f;
    break;
  case GR_ASPECT_LOG2_1x8:
    gc->state.per_tmu[tmu].s_scale = 32.f;
    gc->state.per_tmu[tmu].t_scale = 256.f;
    break;
  }
  
  GR_END();
#undef FN_NAME
} /* grTexSource */


/*-------------------------------------------------------------------
  Function: grTexMultibase
  Date: 11/4/96
  Implementor(s): gmt
  Library: Glide
  Description:
    Enable multiple base addresses for texturing.
  Arguments:
    tmu    - which tmu
    enable - flag which enables/disables multibase
  Return:
    none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexMultibase, void, 
         (GrChipID_t tmu, FxBool enable))
{
#define FN_NAME "grTexMultibase"
  FxU32 tLod;
    
  GR_BEGIN("grTexMultibase",88,4,1);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d)\n",tmu,enable);
  GR_CHECK_TMU(FN_NAME,tmu);
  
  tLod  = gc->state.shadow.tmuState[tmu].tLOD;
  if (enable)
    tLod |= SST_TMULTIBASEADDR;
  else
    tLod &= ~SST_TMULTIBASEADDR;
  /*--------------------------------------------------------------
    Write State To Hardware and Update Glide Shadow State
    --------------------------------------------------------------*/
  GR_SET((0x02UL << tmu), SST_TMU(hw, tmu), tLOD, tLod);
  gc->state.shadow.tmuState[tmu].tLOD = tLod;

  GR_END();
#undef FN_NAME
} /* grTexMultibase */

/*-------------------------------------------------------------------
  Function: grTexMultibaseAddress
  Date: 11/4/96
  Implementor(s): gmt
  Library: Glide
  Description:
    Set the base address for a particular set of mipmaps
  Arguments:
    tmu    - which tmu
    range  - range of lods that are based at this starting address
             One of:
             GR_TEXBASE_256
             GR_TEXBASE_128
             GR_TEXBASE_64
             GR_TEXBASE_32_TO_1
    startAddress - start address that data was downloaded to 
                    hardware with using grTexDownload/Level
    info         - pointer to GrTexInfo structure containing
                   texture dimensions
  Return:
    none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexMultibaseAddress, void, 
         (GrChipID_t tmu, GrTexBaseRange_t range, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info))
{
#define FN_NAME "grTexMultibaseAddress"

  GR_BEGIN("grTexMultibaseAddress", 88, sizeof(FxU32), 1);
  GDBG_INFO_MORE(gc->myLevel,"(%d, 0x%X, 0x%X)\n", tmu, range, startAddress);

  GR_CHECK_TMU(FN_NAME, tmu);
  GR_CHECK_F(FN_NAME, range > GR_TEXBASE_256, "invalid range");
  GR_CHECK_F(FN_NAME, startAddress >= gc->tmu_state[tmu].total_mem, "invalid startAddress");
  GR_CHECK_F(FN_NAME, evenOdd > 0x3, "evenOdd mask invalid");
  GR_CHECK_F(FN_NAME, info == NULL, "invalid info pointer");  

  /* Write relevant registers out to hardware and shadows */
  {
    GrLOD_t
      largeLevelLod;
    FxU32
      baseAddrRegIndex,
      *addrRegShadow;

    switch (range) {
    case GR_TEXBASE_256:
      largeLevelLod = G3_LOD_TRANSLATE(GR_LOD_LOG2_256);
      baseAddrRegIndex = (offsetof(SstRegs, texBaseAddr) >> 2UL);
      addrRegShadow = &gc->state.shadow.tmuState[tmu].texBaseAddr;
      break;
      
    case GR_TEXBASE_128:
      largeLevelLod = G3_LOD_TRANSLATE(GR_LOD_LOG2_128);
      baseAddrRegIndex = (offsetof(SstRegs, texBaseAddr1) >> 2UL);
      addrRegShadow = &gc->state.shadow.tmuState[tmu].texBaseAddr_1;
      break;
      
    case GR_TEXBASE_64:
      largeLevelLod = G3_LOD_TRANSLATE(GR_LOD_LOG2_64);
      baseAddrRegIndex = (offsetof(SstRegs, texBaseAddr2) >> 2UL);
      addrRegShadow = &gc->state.shadow.tmuState[tmu].texBaseAddr_2;
      break;
      
    case GR_TEXBASE_32_TO_1:
    default: /* should never happen because of range check above */
      largeLevelLod = G3_LOD_TRANSLATE(GR_LOD_LOG2_32);
      baseAddrRegIndex = (offsetof(SstRegs, texBaseAddr38) >> 2UL);
      addrRegShadow = &gc->state.shadow.tmuState[tmu].texBaseAddr_3_8;
      break;
    }
    
    {
      const struct GrTmuMemInfo*
        memInfo = gc->tmuMemInfo + tmu;
      const FifoChipField 
        tmuChip = (FifoChipField)(0x02UL << tmu);
      const FxU32
        baseAddress = (memInfo->tramOffset +
                       _grTexCalcBaseAddress(startAddress,
                                             largeLevelLod,
                                             G3_ASPECT_TRANSLATE(info->aspectRatioLog2),
                                             info->format,
                                             evenOdd)) & SST_TEXTURE_ADDRESS;
      
      hw = SST_TMU(hw,tmu);
      
      GR_SET_INDEX(tmuChip, hw, baseAddrRegIndex, baseAddress);
      *addrRegShadow = baseAddress;

      /* Synch flush data w/ new state */
      if (addrRegShadow == &gc->state.shadow.tmuState[tmu].texBaseAddr) {
        gc->tmuMemInfo[tmu].postPacket[1] = ~baseAddress;
        gc->tmuMemInfo[tmu].postPacket[5] = baseAddress;
      }
    }
  }
  
  GR_END();
#undef FN_NAME
} /* grTexMultibaseAddress */

/*-------------------------------------------------------------------
  Function: grTexChromaMode
  Date: 17-Dec-97
  Implementor(s): atai
  Description:
  Draw triangles
  Arguments:
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grTexChromaMode, void , (GrChipID_t tmu, GrChromakeyMode_t mode) )
{
#define FN_NAME "grTexChromaMode"
  GrColor_t chromaRange;
  GR_BEGIN_NOFIFOCHECK(FN_NAME"\n", 85);

  chromaRange = gc->state.shadow.tmuState[tmu].texchromaRange;
  chromaRange &= ~(SST_ENCHROMARANGE |
                   SST_ENCHROMAKEY_TMU |
                   SST_ENCOLORSUBSTITUTION);
  switch (mode) {
  case GR_TEXCHROMA_ENABLE_EXT:
    /*
    ** substitude with black
    */
    chromaRange |= (SST_ENCHROMARANGE | SST_ENCHROMAKEY_TMU);
    break;
#if 0
    /*
    ** disable in Glide3
    */
  case GR_TEXCHROMA_ENABLE_SUBSTITUTE_RGB:
    chromaRange |= (SST_ENCHROMARANGE | SST_ENCHROMAKEY_TMU | SST_ENCOLORSUBSTITUTION);
    break;
#endif
  case GR_TEXCHROMA_DISABLE_EXT:
    break;
  }
  GR_SET_EXPECTED_SIZE(4, 1);
  GR_SET((0x02UL << tmu), SST_TMU(hw, tmu), chromaRange, chromaRange);
  GR_CHECK_SIZE();
  gc->state.shadow.tmuState[tmu].texchromaRange = chromaRange;

#undef FN_NAME
} /* grTexChromaMode */

/*-------------------------------------------------------------------
  Function: grTexChromaRange
  Date: 17-Dec-97
  Implementor(s): atai
  Description:
  Draw triangles
  Arguments:
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grTexChromaRange, void , 
           (GrChipID_t tmu, GrColor_t min, GrColor_t max, GrTexChromakeyMode_t mode) )
{
#define FN_NAME "grTexChromaRange"
  GrColor_t minColor = min;
  GrColor_t maxColor = max;
  FxU32 chromaRange;
  GR_BEGIN("grTexChromaRange",85,8, 2);

  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", tmu);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", min);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", max);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", mode);

  _grSwizzleColor(&minColor);
  _grSwizzleColor(&maxColor);
  
  minColor = minColor & 0x00ffffff;
  maxColor = maxColor & 0x00ffffff;
  chromaRange = gc->state.shadow.tmuState[tmu].texchromaRange;
  chromaRange &= (BIT(31) |                     /* bit 31 is not used, keep it zero */
                  SST_ENCHROMARANGE |
                  SST_ENCHROMAKEY_TMU |
                  SST_ENCOLORSUBSTITUTION);
  chromaRange = chromaRange | maxColor | (mode << 24);

#if 0
  /* 
  ** fbiInit7 bit(19) = 1 enable texture chromarange
  ** It was enabled by default
  */
  fbiInit7 = GR_GET(hw->fbiInit7);
  fbiInit7 = fbiInit7 | SST_TMU_CHROMA_REG_WR_EN;
  GR_SET(BROADCAST_ID, hw, fbiInit7, fbiInit7);
#endif

  GR_SET((0x02UL << tmu), SST_TMU(hw, tmu), chromaKey, minColor);
  GR_SET((0x02UL << tmu), SST_TMU(hw, tmu), chromaRange, chromaRange);

  gc->state.shadow.tmuState[tmu].texchromaKey = minColor;
  gc->state.shadow.tmuState[tmu].texchromaRange = chromaRange;

  GR_END();
#undef FN_NAME
} /* grTexChromaRange */

/*
** _grTexForceLod
**
** Forces tLOD to a specific LOD level.  This is useful only for
** debugging purposes.  GMT: obsolete, please remove
** AYT: added it back for Averger.
*/
void 
_grTexForceLod(GrChipID_t tmu, int value)
{
#define FN_NAME "_grTexForceLod"
  GR_DCL_GC;
  GR_DCL_HW;
  FxU32 tLod = gc->state.shadow.tmuState[tmu].tLOD;

  value = G3_LOD_TRANSLATE(value);

  tLod &= ~(SST_LODMIN | SST_LODMAX);
  tLod |= SST_TLOD_MINMAX_INT(value,value);

  GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
  GR_SET((0x02UL << tmu), SST_TMU(hw, tmu), tLOD, tLod);
  GR_CHECK_SIZE();

  gc->state.shadow.tmuState[tmu].tLOD = tLod;
#undef FN_NAME
} /* _grTexForceLod */

