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
**  39   3dfx      1.34.1.0.1.211/14/00 Jonny Cochrane  Implement multisample LOD
**       Dithering for 2x and 4x FSAA modes 
**  38   3dfx      1.34.1.0.1.110/11/00 Brent           Forced check in to enforce
**       branching.
**  37   3dfx      1.34.1.0.1.007/11/00 Adam Briggs     fixed a state management
**       bug where using the constant color extension was inadvertently turning on
**       chroma range substitution
**  36   3dfx      1.34.1.0    06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  35   3dfx      1.34        04/25/00 Kenneth Dyke    Fixed non-compressed ->
**       compressed texture chip bug workaround code.
**  34   3dfx      1.33        04/11/00 Kenneth Dyke    Fixed debug assertion
**       failure in grTexMultibaseAddress().
**  33   3dfx      1.32        04/04/00 Kenneth Dyke    Fixed addressing for large
**       tiled textures.
**  32   3dfx      1.31        03/31/00 Kenneth Dyke    Make sure immediate updates
**       in 2PPC mode update both real TMU hardware shadows.
**  31   3dfx      1.30        03/23/00 Kenneth Dyke    Make sure we maintain the
**       same state across all chips.
**  30   3dfx      1.29        03/19/00 Kenneth Dyke    Don't set renderMode each
**       time we change 2ppc state.
**       Don't defer TMU state updates unless we really have to.
**  29   3dfx      1.28        03/16/00 Kenneth Dyke    User-adjustable LOD bias
**       value.
**  28   3dfx      1.27        03/11/00 Kenneth Dyke    Removed hack to disable
**       palettized textures in 2PPC mode.
**       Added some texture combine debuggng stuff.
**  27   3dfx      1.26        03/07/00 Kenneth Dyke    Minor refinement to
**       compressed texture nop workaround.
**  26   3dfx      1.25        03/07/00 Kenneth Dyke    Workaround for weird
**       compressed texture quirk.
**  25   3dfx      1.24        02/22/00 Kenneth Dyke    Track palettized texture
**       usage.
**  24   3dfx      1.23        02/02/00 Kenneth Dyke    Fixed per-TMU constant
**       color values to not interfere with texture chroma keying.
**  23   3dfx      1.22        02/02/00 Kenneth Dyke    Fix parameter setup issue
**       with 2PPC modes.
**  22   3dfx      1.21        02/01/00 Kenneth Dyke    Added code to detect when
**       TMU0 is in passthrough mode and to enabled 2PPC in that case. (Remap TMU1
**       to TMU0).
**  21   3dfx      1.20        01/31/00 Adam Briggs     changed the IS_NAPALM macro
**       to cooperate with the display driver version of the same
**  20   3dfx      1.19        01/31/00 Adam Briggs     Changed all device ID magic
**       numbers to use those defined in fxhal.h & added IS_NAPALM macro to test
**       against device ID range
**  19   3dfx      1.18        01/30/00 Adam Briggs     changed
**       grConstantColorValueExt back to a GR_EXT_ENTRY
**  18   3dfx      1.17        01/28/00 Kenneth Dyke    Totoally revamped TMU
**       register update mechanism to make 2PPC modes work right regardless of the
**       order of Glide calls.   Also fixed a few register config bugs found when
**       switching between new and old style combine modes.
**  17   3dfx      1.16        01/23/00 Adam Briggs     set & recognize the SSTTYPE
**       var for Voodoo4
**  16   3dfx      1.15        01/18/00 Kenneth Dyke    Fix for 26-bit texture
**       addresses.
**  15   3dfx      1.14        01/16/00 Kenneth Dyke    Changes to enforce proper
**       combine extension usage.
**  14   3dfx      1.13        11/30/99 Kenneth Dyke    Fixed incorrect LOD
**       settings for inactive TMUs.
**  13   3dfx      1.12        10/26/99 Larry  warner   Turn off mipmap level size
**       debug check
**       if we're in windowed mode, to enable the
**       napalm mustpass.bat conform test to run.
**  12   3dfx      1.11        10/25/99 Anthony tai     disable 2ppc in turning off
**       combine ext
**  11   3dfx      1.10        10/25/99 Anthony tai     fixed wrong tex state
**  10   3dfx      1.9         10/25/99 Anthony tai     fixed 2ppc state and b_mode
**       init value
**  9    3dfx      1.8         10/15/99 Anthony tai     move 2ppc setting to state
**       validation
**  8    3dfx      1.7         10/14/99 Anthony tai     fixed localTMUstate
**  7    3dfx      1.6         10/06/99 Anthony tai     disable 2ppc if tmu1 is
**       specified
**  6    3dfx      1.5         09/30/99 Adam Briggs     fixed
**       grConstantColorValueExt
**  5    3dfx      1.4         09/30/99 Anthony tai     fixed grTexAlphaCombineExt,
**       local tmu should not be set for other texture
**  4    3dfx      1.3         09/22/99 Adam Briggs     Added
**       grConstantColorValueExt... not yet tested.
**  3    3dfx      1.2         09/17/99 Adam Briggs     Supported TEXTUREBUFFEREXT
**       for Napalm 32bpp and AA modes.
**  2    3dfx      1.1         09/17/99 Anthony tai     fixed 2ppc and tmumask for
**       texture color/alpha combine extension
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 67    8/05/99 5:03p Larryw
** FXT1 format works now.
** 
** 66    7/30/99 3:52p Atai
** make csim happy
** 
** 65    7/29/99 7:07p Larryw
** Pave the way for FXT1 (but not quite there yet).
** 
** 64    7/27/99 2:39p Atai
** added 12 nopCMD going from 2ppc to 1ppc
** 
** 63    7/22/99 8:14p Larryw
** Texture format byte-depth improvements
** 
** 62    7/18/99 11:11a Atai
** added 2 pixel per clock
** 
** 61    7/16/99 10:59a Atai
** remove un-supported mode
** fixed tcc, tac problem
** 
** 60    7/14/99 6:23p Larryw
** Remove obsolete G3_LOD_TRANSLATE() macro
** Define _grMipMapOffset[][] at compile time
** Fix 2k texture address-finding
** 
** 59    7/14/99 5:03p Larryw
** Removed some #if 0 code  (no functional changes)
** 
** 58    7/12/99 6:27p Larryw
** Cleaner handling of tBig variable
** Filled in _grMipMapOffset[][] for 512+ sizes
** Fixed problems with grTexMipMapMode()
** 
** 57    7/07/99 6:52p Larryw
** * 2k texture support
** * Reversed order of LOD tables
** * Added 512,1024, and 2048-sized entries in tables
** * Nullified G3_LOD_TRANSLATE()
** * Created _g3LodXlat() for where tLOD register is read/written
** * Misc cosmetic changes.
** 
** 56    6/29/99 7:19p Atai
** remove argument for enabling SST_CM_USE_COMBINE_MODE
** 
** 54    6/27/99 12:44p Atai
** fixed CC and TCC
** 
** 53    6/24/99 7:14p Atai
** added COMBINE extension and 2 buffers per chip
** 
** 52    6/14/99 5:16p Larryw
** Added 32-bit texture format support.
** 
** 51    6/13/99 4:56p Atai
** added code for TCC and TAC, not working yet
** 
** 50    5/19/99 3:55p Denis
** 
** 49    5/19/99 12:45p Denis
** First check in of the TEXTUREBUFFER extension.
** Contains both the texture color buffer and texture aux. buffer
** extensions
** that allows to specify a piece of texture memory as a rendering target
** and/or a piece of texture memory as the aux. buffer.
** 
** Probably a non conventional check in, in the sense that the API
** isn't entirely frozen / specified yet. To ease whoever's job it will be
** to complete the extension, I've added a tbext comment
** everywhere I made a modification. These should go away
** once the API is frozen.
**  
** 
** 48    4/10/99 1:24p Atai
** fixed code to compile in packet fifo mode 
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
** 
** 27    11/30/98 6:57p Peter
** windowed texture effage
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
 * started playing w/ h3 sim
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

#ifndef GLIDE_INIT_HWC
#include "minihwc.h"
#endif

static GrLOD_t g3LodXlat_base[2] = { GR_LOD_LOG2_256, GR_LOD_LOG2_2048 };
#define _g3LodXlat(someLOD, tBig) \
  (g3LodXlat_base[tBig] - someLOD)

/*-------------------------------------------------------------------
  Function: _grTexCalcBaseAddressTiled
  Date: 12-Dec-98
  Implementor(s): dow
  Description:
  
  Arguments:
  start_address:        This is the actual physical address of
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
    byteOffset, offsetX, offsetY;
  FxI32 
    baseAddress = start_address;
  GrLOD_t
    maxPossibleLod = GR_LOD_LOG2_256; /* But see below for Napalm */

  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME, 88);
  GDBG_INFO_MORE(gc->myLevel, "(%d, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X)\n",
                 tmu, start_address,
                 aspect, largeLod,
                 fmt, evenOdd);

  /* texBaseAddr gets the address of the largest mipmap level that the
   * hw is capable of handling (256 for Banshee & Voodoo 3; 2048 for
   * Napalm) even if some of the larger levels don't exist.  Now,
   * we'll basically calculate the offset (from the beginning) of the
   * largest mipmap level, and subtract that from start_address to
   * calculate the baseAdress.
   *
   * Capiche?  
   * 
   * Check for Napalm-ness and use that level if applicable.  Capisce?
   * 
   * As it turns out, even on Napalm texBaseAddr *always* points at
   * the 256x256 mipmap level. %%KCD
   */
  
  byteOffset = _grTexCalcMipmapLevelOffsetTiled(tmu,
                                                largeLod, maxPossibleLod,
                                                aspect, 
                                                fmt,
                                                evenOdd,
                                                &offsetX,
                                                &offsetY);

  /* _grTexCalcMipmapLevelOffsetTiled returns a byte offset from the
   * current location back to the start of the biggest level. However,
   * if we are in tiled texture mode then we actually need to compute
   * a tiled offset. 
   */

  {
    const struct GrTmuMemInfo*
      memInfo = gc->tmuMemInfo + tmu;
    const FxU32
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
                                 FxU32 evenOdd, 
                                 FxU32 *tileXreturn,
                                 FxU32 *tileYreturn)
{
#define FN_NAME "_grTexCalcMipmapLevelOffsetTiled"
  const FxU32
    texelSize = _grBitsPerTexel[fmt];
  FxU32 
    byteOffset = 0,
    tileX = 0, 
    tileY = 0;
  FxI32 direction;

  GR_BEGIN_NOFIFOCHECK_RET(FN_NAME, 90);

  GR_CHECK_F(FN_NAME, texelSize == 0, "invalid texture format");
  GR_CHECK_TMU(FN_NAME, tmu);
  GR_ASSERT(gc->tmuMemInfo[tmu].texTiled);

  /*
     Tiled textures look like this (for big textures)
      
     +--------------+-------+
     |              |       |
     |              | 1024  |
     |              |       |
     |              +---*---+
     |     2048     |512|   | <- 256-1  (see below for rest)
     |              +---+---+
     |              |   |   |
     |              |   |   |   
     |              | ->|   |<- Stride
     +--------------+   
   
     Tiled Textures look like this (for non-big textures):

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

  /* We are either summing up the offsets all the way from the smallest
   * mipmap in the chain, or we are finding the offset from a given mipmap
   * level to the one that's used as the base hardware pointer. */
  if(lod <= largeLOD)
    direction = 1;
  else
    direction = -1;

  /* LOOK AT THE PICTURE!!!!!
   * We're trying to work our way from the upper-left-hand corner of
   * largeLOD to the upper-left-hand corner of the 256 LOD.  
   */
  {
    const FxU32
      texStrideBytes = gc->tmuMemInfo[tmu].texStrideBytes;
    GrLOD_t 
      level;

    level = lod;
    
    while (level != largeLOD) {
      switch (level) {
      case GR_LOD_LOG2_2048:
        if(direction > 0) {            
          /* Do nothing, in theory should never get here. */
        } else {
          /* Offset by width of 2048 LOD */
          if(evenOdd & GR_MIPMAPLEVELMASK_ODD) {
            tileX -= WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_2048);
          }    
          level--;
        }    
        break;

      case GR_LOD_LOG2_1024:
        /* Offset by height of 1024 LOD */
        if(direction > 0) {
          /* Offset by width of 2048 LOD */
          if(evenOdd & GR_MIPMAPLEVELMASK_ODD) {
            tileX += WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_2048);
          }              
          level++;
        } else {    
          if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
            tileY -= HEIGHT_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_1024);
          }    
          level--;
        }
        break;

      case GR_LOD_LOG2_512:
        if(direction > 0) {
          /* Offset by height of 1024 LOD */
          if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
            tileY += HEIGHT_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_1024);
          }    
          level++;
        } else {
          /* Offset by width of 512 LOD */
          if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
            tileX -= WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_512);
          }    
          level--;
        }
        break;

      case GR_LOD_LOG2_256:
        if(direction > 0) {          
          /* Offset the width of the 512 LOD */
          if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
            tileX += WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_512);
          }
          level++;
        } else {
          /* Do nothing.  In theory should never get here. */
        }    
        break;
        
      case GR_LOD_LOG2_128:
        /* Offset the width of the 256 LOD */
        if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
          tileX += WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_256);
        }
        level++;
        break;
        
      case GR_LOD_LOG2_64:
        /* Offset the height of the 128 LOD */
        if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
          tileY += HEIGHT_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_128);
        }
        level++;
        break;
        
      case GR_LOD_LOG2_32:
        /* Offset the width of the 64 LOD */
        if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
          tileX += WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_64);
        }
        level++;
        break;
        
      case GR_LOD_LOG2_16:
        /* Offset the width of the 32 LOD */
        if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {    
          tileX += WIDTH_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_32);
        }
        level++;
        break;
        
      case GR_LOD_LOG2_8:
        {
          switch(fmt) {
          case GR_TEXFMT_ARGB_CMP_FXT1:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
              tileY += HEIGHT_BY_ASPECT_LOD_FXT1(aspect, GR_LOD_LOG2_16);
            }
            break;
          case GR_TEXFMT_ARGB_CMP_DXT1:
          case GR_TEXFMT_ARGB_CMP_DXT2:
          case GR_TEXFMT_ARGB_CMP_DXT3:
          case GR_TEXFMT_ARGB_CMP_DXT4:
          case GR_TEXFMT_ARGB_CMP_DXT5:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
              tileY += HEIGHT_BY_ASPECT_LOD_DXT(aspect, GR_LOD_LOG2_16);
            }
            break;
          default:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
              tileY += HEIGHT_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_16);
            }
            break;
          }
        }
        level++;
        break;

      case GR_LOD_LOG2_4:
        {
          switch(fmt) {
          case GR_TEXFMT_ARGB_CMP_FXT1:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
              tileY += HEIGHT_BY_ASPECT_LOD_FXT1(aspect, GR_LOD_LOG2_8);
            }
            break;
          case GR_TEXFMT_ARGB_CMP_DXT1:
          case GR_TEXFMT_ARGB_CMP_DXT2:
          case GR_TEXFMT_ARGB_CMP_DXT3:
          case GR_TEXFMT_ARGB_CMP_DXT4:
          case GR_TEXFMT_ARGB_CMP_DXT5:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
              tileY += HEIGHT_BY_ASPECT_LOD_DXT(aspect, GR_LOD_LOG2_8);
            }
            break;
          default:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
              tileY += HEIGHT_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_8);
            }
            break;
          }
        }
        level++;
        break;
        
      case GR_LOD_LOG2_2:
        {
          switch(fmt) {
          case GR_TEXFMT_ARGB_CMP_FXT1:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
              tileY += HEIGHT_BY_ASPECT_LOD_FXT1(aspect, GR_LOD_LOG2_4);
            }
            break;
          case GR_TEXFMT_ARGB_CMP_DXT1:
          case GR_TEXFMT_ARGB_CMP_DXT2:
          case GR_TEXFMT_ARGB_CMP_DXT3:
          case GR_TEXFMT_ARGB_CMP_DXT4:
          case GR_TEXFMT_ARGB_CMP_DXT5:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
              tileY += HEIGHT_BY_ASPECT_LOD_DXT(aspect, GR_LOD_LOG2_4);
            }
            break;
          default:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_ODD) {
              tileY += HEIGHT_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_4);
            }
            break;
          }
        }
        level++;
        break;
        
      case GR_LOD_LOG2_1:
        {
          switch(fmt) {
          case GR_TEXFMT_ARGB_CMP_FXT1:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
              tileY += HEIGHT_BY_ASPECT_LOD_FXT1(aspect, GR_LOD_LOG2_2);
            }
            break;
          case GR_TEXFMT_ARGB_CMP_DXT1:
          case GR_TEXFMT_ARGB_CMP_DXT2:
          case GR_TEXFMT_ARGB_CMP_DXT3:
          case GR_TEXFMT_ARGB_CMP_DXT4:
          case GR_TEXFMT_ARGB_CMP_DXT5:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
              tileY += HEIGHT_BY_ASPECT_LOD_DXT(aspect, GR_LOD_LOG2_2);
            }
            break;
          default:
            /* For these, we offset the LOD height * texture stride */
            if (evenOdd & GR_MIPMAPLEVELMASK_EVEN) {
              tileY += HEIGHT_BY_ASPECT_LOD(aspect, GR_LOD_LOG2_2);
            }
            break;
          }
        }
        level++;
        break;
        
      }
    }

    if(tileXreturn) {
      *tileXreturn = (tileX * texelSize)>>3;
      *tileYreturn = tileY;
    }

    /* The ">>3" is here to to convert from bits to bytes: */
    byteOffset = (((tileX * texelSize)>>3) +
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
              evenOdd[i] = gc->state.shadow.tmuState[i].tLOD & SST_LOD_ODD;
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

void
_grTex2ppc(FxBool enable)
{
#define FN_NAME "_grTex2ppc"
#ifdef FX_GLIDE_NAPALM
  /*
  ** enable combineMode[29]
  ** control the band in renderMode[24:22]
  ** make tmu 1 state the same as tmu0
  */
  
  FxU32 combineMode;
  FxU32 tmu0cMode;
  FxU32 tmu1cMode;
  FxU32 tmu0cMode_tmuShadow;
  FxU32 tmu1cMode_tmuShadow;
  
  FxBool oldstate;

  GR_BEGIN_NOFIFOCHECK("_grTex2ppc",88);

  combineMode = gc->state.shadow.combineMode;
  oldstate = (combineMode & SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK) ? FXTRUE : FXFALSE;
  GDBG_INFO(gc->myLevel, FN_NAME ": changing 2ppc mode to: %s\n", enable ? "FXTRUE" : "FXFALSE");
  
  if (enable == oldstate)
    return;

  gc->state.mode2ppc = enable;
  
  /* Make sure that all TMU registers are restored if 2PPC mode is being switched off. */
  if(!enable) {
    INVALIDATE_TMU(GR_TMU0, textureMode);
    INVALIDATE_TMU(GR_TMU0, texBaseAddr);
    INVALIDATE_TMU(GR_TMU0, texchroma);
    INVALIDATE_TMU(GR_TMU1, textureMode);
    INVALIDATE_TMU(GR_TMU1, texBaseAddr);
    INVALIDATE_TMU(GR_TMU1, texchroma);
  }
  
  tmu0cMode = gc->state.shadow.tmuState[0].combineMode;
  tmu1cMode = gc->state.shadow.tmuState[1].combineMode;
  tmu0cMode_tmuShadow = gc->state.tmuShadow[0].combineMode;
  tmu1cMode_tmuShadow = gc->state.tmuShadow[1].combineMode;

  combineMode &= ~(SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK);
  tmu0cMode &= ~(SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK);
  tmu1cMode &= ~(SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK);
  tmu0cMode_tmuShadow &= ~(SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK);
  tmu1cMode_tmuShadow &= ~(SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK);

  combineMode |= ((enable) ? SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK : 0);
  tmu0cMode |= ((enable) ? SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK : 0);
  tmu1cMode |= ((enable) ? SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK : 0);
  tmu0cMode_tmuShadow |= ((enable) ? SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK : 0);
  tmu1cMode_tmuShadow |= ((enable) ? SST_CM_ENABLE_TWO_PIXELS_PER_CLOCK : 0);

  /*
  ** Need to flush the tmu pipeline going from 2ppc to 1ppc
  ** by sending 12 nopCMD
  */
  if (!enable) {
    FxU32 i;
    GR_SET_EXPECTED_SIZE(sizeof(FxU32)*12, 12);
    for (i = 0; i < 12; i++) {
      GR_SET(eChipTMU0 | eChipTMU1, hw, nopCMD,  0);
    }
    GR_CHECK_SIZE();
  }

  /* set combine mode */
  REG_GROUP_BEGIN(BROADCAST_ID, combineMode, 1, 0x1);
  {
    REG_GROUP_SET(hw, combineMode, combineMode);
#if DEBUG_2PPC
    GDBG_PRINTF("FBI  combineMode: %08lx\n",combineMode);
#endif
  }
  REG_GROUP_END();

  REG_GROUP_BEGIN(eChipTMU0, combineMode, 1, 0x1);
  {
    REG_GROUP_SET(SST_TMU(hw, 0), combineMode, tmu0cMode);
#if DEBUG_2PPC
    GDBG_PRINTF("TMU0 combineMode: %08lx  2ppc=%d\n",tmu0cMode,enable);
#endif    
  }
  REG_GROUP_END();
  REG_GROUP_BEGIN(eChipTMU1, combineMode, 1, 0x1);
  {
    REG_GROUP_SET(SST_TMU(hw, 1), combineMode, tmu1cMode);
#if DEBUG_2PPC
    GDBG_PRINTF("TMU1 combineMode: %08lx  2ppc=%d\n",tmu1cMode,enable);
#endif
  }
  REG_GROUP_END();

  gc->state.shadow.combineMode = combineMode;
  gc->state.shadow.tmuState[0].combineMode = tmu0cMode;
  gc->state.shadow.tmuState[1].combineMode = tmu1cMode;
  gc->state.tmuShadow[0].combineMode = tmu0cMode_tmuShadow;
  gc->state.tmuShadow[1].combineMode = tmu1cMode_tmuShadow;

  GR_END();
  
#endif
#undef FN_NAME
} /* _grTex2ppc */

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
  texturemode  = gc->state.tmuShadow[tmu].textureMode;
  texturemode &= ~(SST_TCLAMPS | SST_TCLAMPT);
  texturemode |=  clampMode;

  tLod = gc->state.tmuShadow[tmu].tLOD;
  tLod &= ~(SST_TMIRRORS | SST_TMIRRORT);        /* disable mirror */
  if (mirrorMode) tLod |= mirrorMode;

    
  gc->state.tmuShadow[tmu].textureMode = texturemode;
  gc->state.tmuShadow[tmu].tLOD        = tLod;

  /* Update real shadows and update hardware immediately if we can. */
  if(!gc->state.mode2ppc || (tmu == gc->state.mode2ppcTMU)) {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    gc->state.shadow.tmuState[tmu].textureMode = texturemode;
    gc->state.shadow.tmuState[tmu].tLOD = tLod;
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    REG_GROUP_BEGIN((0x02 << tmu), textureMode, 2, 0x3);
    {
      REG_GROUP_SET(tmuHw, textureMode, gc->state.shadow.tmuState[tmu].textureMode);
      REG_GROUP_SET(tmuHw, tLOD, gc->state.shadow.tmuState[tmu].tLOD);
    }
    REG_GROUP_END();
    _grChipMask(gc->chipmask);
    if(gc->state.per_tmu[tmu].texSubLodDither)
        g3LodBiasPerChip(tmu, gc->state.shadow.tmuState[tmu].tLOD);
  } else {
    INVALIDATE_TMU(tmu, textureMode);
  }
  
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

#if DEBUG_2PPC
  GDBG_PRINTF("grTexCombine(tmu=%d,rgbfunc=%x,rgbfact=%x,afunc=%x,afact=%x,ri=%d,ai=%d)\n",
    tmu,rgb_function,rgb_factor,alpha_function,alpha_factor,rgb_invert,alpha_invert);
#endif
    
#ifdef FX_GLIDE_NAPALM
  {
    /*
    ** turn off combine mode if necessary
    */
    FxU32 combineMode = gc->state.tmuShadow[tmu].combineMode;
    FxU32 isCombineExt = (combineMode & SST_CM_USE_COMBINE_MODE);
    if (isCombineExt) {
      /* It appears that some of the combineMode bits still take effect */
      /* even if SST_CM_USE_COMBINE_MODE is cleared.  Thus, to make sure */
      /* that things work right with old style combine modes, we clear   */
      /* all of the combine mode bits to zero. */
      combineMode &= ~(
                       SST_CM_USE_COMBINE_MODE |
                       SST_CM_TC_OTHERSELECT   |
                       SST_CM_TC_LOCALSELECT   |
                       SST_CM_TC_MSELECT_7     |
                       SST_CM_TC_INVERT_OTHER  |
                       SST_CM_TC_INVERT_LOCAL  |
                       SST_CM_TC_INVERT_ADD_LOCAL |
                       SST_CM_TC_OUTSHIFT      |
                       SST_CM_TCA_OTHERSELECT  |
                       SST_CM_TCA_LOCALSELECT  |
                       SST_CM_TCA_INVERT_OTHER |
                       SST_CM_TCA_INVERT_LOCAL |
                       SST_CM_TCA_INVERT_ADD_LOCAL |
                       SST_CM_TCA_OUTSHIFT |
                       SST_CM_DISABLE_CHROMA_SUBSTITUTION
                      );
      INVALIDATE_TMU(tmu, textureMode);
      gc->state.tmuShadow[tmu].combineMode = combineMode;
    }
  }
#endif
  /* tmuMask tells grColorCombineFunction what to turn off an on if 
     the ccFunc requires texture mapping */
  texturemode = gc->state.tmuShadow[tmu].textureMode;
  texturemode &= ~(SST_TCOMBINE | SST_TACOMBINE);
  tLod = gc->state.tmuShadow[tmu].tLOD;
  tLod &= ~(SST_LOD_ODD);

  gc->state.tcc_requires_constant_color[tmu] = FXFALSE;
  gc->state.tac_requires_constant_color[tmu] = FXFALSE;

  gc->state.combineExtsInUse &= ~(STATE_USING_TCC|STATE_USING_TAC);

  tmuMask = GR_TMUMASK_TMU0 << tmu;
  gc->state.tmuMaskShadow &= ~tmuMask;
  gc->state.tmuColorPassthrough &= ~tmuMask;
  gc->state.tmuAlphaPassthrough &= ~tmuMask;

  /* Check for obvious passthrough case */
  if((rgb_function == GR_COMBINE_FUNCTION_SCALE_OTHER) &&
     (rgb_factor == GR_COMBINE_FACTOR_ONE) &&
     (alpha_function == GR_COMBINE_FUNCTION_SCALE_OTHER) &&
     (alpha_factor == GR_COMBINE_FACTOR_ONE)) {
    gc->state.tmuColorPassthrough |= tmuMask;
    gc->state.tmuAlphaPassthrough |= tmuMask;
  }

  /* setup scale factor bits */
  texturemode |= (rgb_factor & 0x7) << SST_TC_MSELECT_SHIFT;
  if (!(rgb_factor & 0x8)) texturemode |= SST_TC_REVERSE_BLEND;

  if (((rgb_factor & 0x7) == GR_COMBINE_FACTOR_LOCAL) ||
       ((rgb_factor & 0x7) == GR_COMBINE_FACTOR_LOCAL_ALPHA))
    gc->state.tmuMaskShadow |= tmuMask;

  texturemode |= (alpha_factor & 0x7) << SST_TCA_MSELECT_SHIFT;
  if (!(alpha_factor & 0x8)) texturemode |= SST_TCA_REVERSE_BLEND;

  if (((alpha_factor & 0x7) == GR_COMBINE_FACTOR_LOCAL) ||
       ((alpha_factor & 0x7) == GR_COMBINE_FACTOR_LOCAL_ALPHA))
    gc->state.tmuMaskShadow |= tmuMask;

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
    
    gc->state.tmuMaskShadow |= tmuMask;
    localColorP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_ADD_ALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    localColorP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER:
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
    texturemode |= SST_TC_ADD_CLOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_ADD_ALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
    texturemode |= SST_TC_SUB_CLOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TC_SUB_CLOCAL | SST_TC_ADD_CLOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_SUB_CLOCAL | SST_TC_ADD_ALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_SUB_CLOCAL | SST_TC_ADD_CLOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    localColorP = ((rgb_factor & 0x07UL) != GR_COMBINE_FACTOR_OTHER_ALPHA);
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_SUB_CLOCAL | SST_TC_ADD_ALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
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
    gc->state.tmuMaskShadow |= tmuMask;
    localAlphaP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    localAlphaP = FXTRUE;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER:
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
    texturemode |= SST_TCA_ADD_CLOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_ADD_ALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
    texturemode |= SST_TCA_SUB_CLOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TCA_SUB_CLOCAL | SST_TCA_ADD_CLOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_SUB_CLOCAL | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_SUB_CLOCAL | SST_TCA_ADD_CLOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    localAlphaP = ((alpha_factor & 0x07UL) != GR_COMBINE_FACTOR_OTHER_ALPHA);
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_SUB_CLOCAL | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    localAlphaP = ((alpha_factor & 0x07UL) != GR_COMBINE_FACTOR_OTHER_ALPHA);
    break;
  }
  
  gc->state.tcc_requires_prev_texture[tmu] = !localColorP;
  gc->state.tac_requires_prev_texture[tmu] = !localAlphaP;
  
  /* Hack to enable TWO-PASS Trilinear */
  if (texturemode & SST_TRILINEAR) {
    if ((texturemode & SST_TC_ZERO_OTHER) &&
        (texturemode & SST_TC_BLEND_LODFRAC) &&
        !(texturemode & SST_TC_REVERSE_BLEND)) {
      tLod |= SST_LOD_ODD;
    }
  }

  /* Hack to use single pass trilinear in UMA state */
  if((texturemode & SST_TRILINEAR) &&
     (gc->state.grEnableArgs.texture_uma_mode == GR_MODE_ENABLE) &&
     (gc->state.per_tmu[tmu].evenOdd == GR_MIPMAPLEVELMASK_BOTH) &&
     (tmu == GR_TMU1)) {
    tLod |= SST_LOD_ODD;
  }
  
  tLod |= _gr_evenOdd_xlate_table[gc->state.per_tmu[tmu].evenOdd];

  /* Update shadows */
  INVALIDATE_TMU(tmu, textureMode);
  gc->state.tmuShadow[tmu].textureMode = texturemode;
  gc->state.tmuShadow[tmu].tLOD        = tLod;

  /* If the state of a tmu changes from active then make sure that it
   * is addressing some valid tiny texture so that it does not spend
   * time thrashing on a large texture access.
   *
   * NB: We don't update the shadow here so the other bits in the
   * register shadow should be unchanged.
   */

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
} /* grTexCombine */

#if DEBUG_COMBINEEXT
const char *cmbxName[] = {
  "ZERO ",    /* GR_CMBX_ZERO                      0x00 */
  "TA   ",    /* GR_CMBX_TEXTURE_ALPHA             0x01 */
  "ALCL ",    /* GR_CMBX_ALOCAL                    0x02 */
  "AOTR ",    /* GR_CMBX_AOTHER                    0x03 */
  "B    ",    /* GR_CMBX_B                         0x04 */
  "CA   ",    /* GR_CMBX_CONSTANT_ALPHA            0x05 */
  "CC   ",    /* GR_CMBX_CONSTANT_COLOR            0x06 */
  "DFCT ",    /* GR_CMBX_DETAIL_FACTOR             0x07 */
  "IA   ",    /* GR_CMBX_ITALPHA                   0x08 */
  "IRGB ",    /* GR_CMBX_ITRGB                     0x09 */
  "LTA  ",    /* GR_CMBX_LOCAL_TEXTURE_ALPHA       0x0a */
  "LTRG ",    /* GR_CMBX_LOCAL_TEXTURE_RGB         0x0b */
  "LFRC ",    /* GR_CMBX_LOD_FRAC                  0x0c */
  "OTA  ",    /* GR_CMBX_OTHER_TEXTURE_ALPHA       0x0d */
  "OTRG ",    /* GR_CMBX_OTHER_TEXTURE_RGB         0x0e */
  "TRGB ",    /* GR_CMBX_TEXTURE_RGB               0x0f */
  "TCA  ",    /* GR_CMBX_TMU_CALPHA                0x10 */
  "TCC  "     /* GR_CMBX_TMU_CCOLOR                0x11 */
};

const char *funcName[] = {
  "ZERO ",    /* GR_FUNC_MODE_ZERO                 0x00 */
  "X    ",    /* GR_FUNC_MODE_X                    0x01 */
  "1-X  ",    /* GR_FUNC_MODE_ONE_MINUS_X          0x02 */
  "-X   ",    /* GR_FUNC_MODE_NEGATIVE_X           0x03 */
  "X-.5 ",    /* GR_FUNC_MODE_X_MINUS_HALF         0x04 */
  "1    ",    /* GR_FUNC_MODE_ONE                  0x05 */
  "-.5  "     /* GR_FUNC_MODE_MINUS_HALF           0x06 */
};

const char *shiftName[] = {
  "1X   ",
  "2X   ",
  "4X   "
};

const char *boolName[] = {
  "FXFALSE  ",
  "FXTRUE "
};
#endif

#ifdef FX_GLIDE_NAPALM
/*---------------------------------------------------------------------------
** grTexColorCombineExt
*/
GR_EXT_ENTRY(grTexColorCombineExt, void, 
         (GrChipID_t       tmu,
          GrTCCUColor_t    a,
          GrCombineMode_t  a_mode,
          GrTCCUColor_t    b,
          GrCombineMode_t  b_mode,
          GrTCCUColor_t    c,
          FxBool           c_invert,
          GrTCCUColor_t    d,
          FxBool           d_invert,
          FxU32            shift,
          FxBool           invert))
{
#define FN_NAME "grTexColorCombineExt"
  FxU32 texturemode;
  FxU32 combineMode;
  FxU32 tLod;
  FxU32 tmuMask;
  FxU32 btmuMask = 0;
  FxU32 localTMU = FXFALSE;
  FxU32 btermTMU = FXFALSE;
  GR_BEGIN_NOFIFOCHECK("grTexColorCombineExt",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",
                 tmu, a, a_mode, b, b_mode, c, d, shift, invert);

#if DEBUG_2PPC || DEBUG_COMBINEEXT
  GDBG_PRINTF("grTexColorCombineExt(tmu=%d,a=%s,am=%s,b=%s,bm=%s,c=%s,ci=%s,d=%s,di=%s,s=%s,i=%s)\n",
    tmu,cmbxName[a],funcName[a_mode],
        cmbxName[b],funcName[b_mode],
        cmbxName[c],boolName[c_invert],
        cmbxName[d],boolName[d_invert],
        shiftName[shift],
        boolName[invert]);
#endif
    
  /* 
  ** a = tc_otherselect
  ** a_mode = tc_invert_other
  ** b = tc_localselect
  ** b_mode = tc_invert_local
  ** c = (tc_mselect, tc_mselect_7)
  ** d = (tc_add_clocal, tc_add_alocal)
  ** shift = tc_outshift
  ** invert = tc_invert_output
  */
  /* tmuMask tells grColorCombineFunction what to turn off an on if 
     the ccFunc requires texture mapping */
  combineMode = gc->state.tmuShadow[tmu].combineMode;
  texturemode = gc->state.tmuShadow[tmu].textureMode;
  texturemode &= ~(SST_TCOMBINE);
  tLod = gc->state.tmuShadow[tmu].tLOD;
  tLod &= ~(SST_LOD_ODD);

  /* Mark that grTexColorCombine has been called... */
  gc->state.combineExtsInUse |= STATE_USING_TCC;

  /* Generate mask for this TMU. */
  tmuMask = GR_TMUMASK_TMU0 << tmu;
  
  /* Clear flag marking this TMU as being in use. */
  gc->state.tmuMaskShadow &= ~tmuMask;

  combineMode &= ~(
                  SST_CM_TC_OTHERSELECT |
                  SST_CM_TC_LOCALSELECT |
                  SST_CM_TC_MSELECT_7 |
                  SST_CM_TC_INVERT_OTHER |
                  SST_CM_TC_INVERT_LOCAL |
                  SST_CM_TC_INVERT_ADD_LOCAL |
                  SST_CM_TC_OUTSHIFT |
                  SST_CM_DISABLE_CHROMA_SUBSTITUTION
                  );
  texturemode &= ~(
                  SST_TC_MSELECT |              /* part of c */
                  SST_TC_ADD_CLOCAL |           /* part of d */
                  SST_TC_ADD_ALOCAL |           /* part of d */
                  SST_TC_INVERT_OUTPUT |        /* invert */
                  SST_TC_REVERSE_BLEND |
                  SST_TC_ZERO_OTHER | 
                  SST_TC_SUB_CLOCAL
                  );

  tLod = gc->state.tmuShadow[tmu].tLOD;
  tLod &= ~(SST_LOD_ODD);

  tmuMask = GR_TMUMASK_TMU0 << tmu;
  gc->state.tmuMaskShadow &= ~tmuMask;
  gc->state.tcctmuMask &= ~tmuMask;
  gc->state.tmuColorPassthrough &= ~tmuMask;

  /* Check for obvious passthrough case. Expand later if something other
   * than OpenGL ever does this sort of thing. */
  if(((a == GR_CMBX_OTHER_TEXTURE_RGB) && (a_mode == GR_FUNC_MODE_X)) &&
     ((b == GR_CMBX_ZERO) && (b_mode == GR_FUNC_MODE_X)) &&
     ((c == GR_CMBX_ZERO) && (c_invert == FXTRUE)) &&
     ((d == GR_CMBX_ZERO) && (d_invert == FXFALSE)) &&
     (shift == 0) &&
     (invert == FXFALSE)) {
    gc->state.tmuColorPassthrough |= tmuMask;
  }

  gc->state.tcc_requires_it_alpha[tmu] = 
    ( (a == GR_CMBX_ITALPHA) |
      (b == GR_CMBX_ITALPHA) |
      (c == GR_CMBX_ITALPHA) );  

  gc->state.tcc_requires_it_rgb[tmu] = 
    ( (a == GR_CMBX_ITRGB) |
      (b == GR_CMBX_ITRGB) |
      (c == GR_CMBX_ITRGB) |
      (d == GR_CMBX_ITRGB) );  

  gc->state.tcc_requires_prev_texture[tmu] = 
    ( (a == GR_CMBX_OTHER_TEXTURE_RGB) |
      (a == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (b == GR_CMBX_OTHER_TEXTURE_RGB) |
      (b == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (c == GR_CMBX_OTHER_TEXTURE_RGB) |
      (c == GR_CMBX_OTHER_TEXTURE_ALPHA) );
  
  gc->state.tcc_requires_texture[tmu] = 
    ( (a == GR_CMBX_OTHER_TEXTURE_RGB) |
      (a == GR_CMBX_LOCAL_TEXTURE_RGB) |
      (a == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (a == GR_CMBX_LOCAL_TEXTURE_ALPHA) |
      (b == GR_CMBX_OTHER_TEXTURE_RGB) |
      (b == GR_CMBX_LOCAL_TEXTURE_RGB) |
      (b == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (b == GR_CMBX_LOCAL_TEXTURE_ALPHA) |
      (c == GR_CMBX_OTHER_TEXTURE_RGB) |
      (c == GR_CMBX_LOCAL_TEXTURE_RGB) |
      (c == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (c == GR_CMBX_LOCAL_TEXTURE_ALPHA) |
      (d == GR_CMBX_LOCAL_TEXTURE_ALPHA) );  
    
  gc->state.tcc_requires_constant_color[tmu] = FXFALSE;
    
  switch (a) {
  case GR_CMBX_OTHER_TEXTURE_RGB:
    combineMode |= SST_CM_TC_OTHERSELECT_OTHER_TRGB;
    localTMU = FXFALSE;
    break;
  case GR_CMBX_OTHER_TEXTURE_ALPHA:
    combineMode |= SST_CM_TC_OTHERSELECT_OTHER_TA;
    localTMU = FXFALSE;
    break;
  case GR_CMBX_LOCAL_TEXTURE_RGB:
    combineMode |= SST_CM_TC_OTHERSELECT_LOCAL_TRGB;
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU = FXTRUE;
    break;
  case GR_CMBX_LOCAL_TEXTURE_ALPHA:
    combineMode |= SST_CM_TC_OTHERSELECT_LOCAL_TA;
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU = FXTRUE;
    break;
  case GR_CMBX_ITRGB:
    combineMode |= SST_CM_TC_OTHERSELECT_IRGB;
    localTMU = FXTRUE;
    break;
  case GR_CMBX_ITALPHA:
    combineMode |= SST_CM_TC_OTHERSELECT_IA;
    localTMU = FXTRUE;
    break;
  case GR_CMBX_TMU_CCOLOR:
    combineMode |= SST_CM_TC_OTHERSELECT_CR_RGB;
    gc->state.tcc_requires_constant_color[tmu] = FXTRUE;
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU = FXTRUE;
    break;
  case GR_CMBX_TMU_CALPHA:
    combineMode |= SST_CM_TC_OTHERSELECT_CR_A ;
    gc->state.tcc_requires_constant_color[tmu] = FXTRUE;
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU = FXTRUE;
    break;
  default:
    /* output zero if the a term is unknown type */
    a_mode = GR_FUNC_MODE_ZERO;
    localTMU = FXTRUE;
    break;
  }
  switch (b) {
  case GR_CMBX_LOCAL_TEXTURE_RGB:
    combineMode |= SST_CM_TC_LOCALSELECT_LOCAL_TRGB;
    if (b_mode != GR_FUNC_MODE_ZERO) {
      gc->state.tmuMaskShadow |= tmuMask;
      localTMU &= FXTRUE;
    }
    btmuMask = tmuMask;
    btermTMU = FXTRUE;
    break;
  case GR_CMBX_LOCAL_TEXTURE_ALPHA:
    combineMode |= SST_CM_TC_LOCALSELECT_LOCAL_TA;
    if (b_mode != GR_FUNC_MODE_ZERO) {
      gc->state.tmuMaskShadow |= tmuMask;
      localTMU &= FXTRUE;
    }    
    btmuMask = tmuMask;
    btermTMU = FXTRUE;
    break;
  case GR_CMBX_OTHER_TEXTURE_RGB:
    combineMode |= SST_CM_TC_LOCALSELECT_OTHER_TRGB;
    if (b_mode != GR_FUNC_MODE_ZERO)
      localTMU &= FXFALSE;
    btermTMU = FXFALSE;
    break;
  case GR_CMBX_OTHER_TEXTURE_ALPHA:
    combineMode |= SST_CM_TC_LOCALSELECT_OTHER_TA;
    if (b_mode != GR_FUNC_MODE_ZERO)
      localTMU &= FXFALSE;
    btermTMU = FXFALSE;
    break;
  case GR_CMBX_ITRGB:
    combineMode |= SST_CM_TC_LOCALSELECT_IRGB;
    if (b_mode != GR_FUNC_MODE_ZERO)
      localTMU &= FXTRUE;
    btermTMU = FXTRUE;
    break;
  case GR_CMBX_ITALPHA:
    combineMode |= SST_CM_TC_LOCALSELECT_IA;
    if (b_mode != GR_FUNC_MODE_ZERO)
      localTMU &= FXTRUE;
    btermTMU = FXTRUE;
    break;
  case GR_CMBX_TMU_CCOLOR:
    combineMode |= SST_CM_TC_LOCALSELECT_CK_RGB;
    gc->state.tcc_requires_constant_color[tmu] = FXTRUE;
    if (b_mode != GR_FUNC_MODE_ZERO) {
      gc->state.tmuMaskShadow |= tmuMask;
      localTMU &= FXTRUE;
    }
    btmuMask = tmuMask;
    btermTMU = FXTRUE;
    break;
  case GR_CMBX_TMU_CALPHA:
    combineMode |= SST_CM_TC_LOCALSELECT_CK_A;
    gc->state.tcc_requires_constant_color[tmu] = FXTRUE;
    if (b_mode != GR_FUNC_MODE_ZERO) {
      gc->state.tmuMaskShadow |= tmuMask;
      localTMU &= FXTRUE;
    }
    btmuMask = tmuMask;
    btermTMU = FXTRUE;
    break; 
  default:
    /* output zero if the b term is unknown type */
    b_mode = GR_FUNC_MODE_ZERO;
    if (b_mode != GR_FUNC_MODE_ZERO)
      localTMU &= FXTRUE;
    btermTMU = FXTRUE;
    break;
 }

  switch (a_mode) {
  case GR_FUNC_MODE_ZERO:
    combineMode |= SST_CM_TC_INVERT_OTHER_X;
    break;
  case GR_FUNC_MODE_X:
    combineMode |= SST_CM_TC_INVERT_OTHER_X;
    break;
  case GR_FUNC_MODE_NEGATIVE_X:
    combineMode |= SST_CM_TC_INVERT_OTHER_ZERO_MINUS_X;
    break;
  case GR_FUNC_MODE_ONE_MINUS_X:
    combineMode |= SST_CM_TC_INVERT_OTHER_ONE_MINUS_X;
    break;
  case GR_FUNC_MODE_X_MINUS_HALF:
    combineMode |= SST_CM_TC_INVERT_OTHER_X_MINUS_HALF;
    break;
  }
  if (a_mode == GR_FUNC_MODE_ZERO)
    texturemode |= SST_TC_ZERO_OTHER;

  switch (b_mode) {
  case GR_FUNC_MODE_ZERO:
    combineMode |= SST_CM_TC_INVERT_LOCAL_X;
    break;
  case GR_FUNC_MODE_X:
    combineMode |= SST_CM_TC_INVERT_LOCAL_X;
    break;
  case GR_FUNC_MODE_NEGATIVE_X:
    combineMode |= SST_CM_TC_INVERT_LOCAL_ZERO_MINUS_X;
    break;
  case GR_FUNC_MODE_ONE_MINUS_X:
    combineMode |= SST_CM_TC_INVERT_LOCAL_ONE_MINUS_X;
    break;
  case GR_FUNC_MODE_X_MINUS_HALF:
    combineMode |= SST_CM_TC_INVERT_LOCAL_X_MINUS_HALF;
    break;
  }
  if (b_mode != GR_FUNC_MODE_ZERO)
    texturemode |= SST_TC_SUB_CLOCAL;

  switch (c) {
    /* Is this correct??? */
  case GR_CMBX_ZERO:
    texturemode |= SST_TC_MONE;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_B:
    texturemode |= SST_TC_MCLOCAL;
    localTMU &= btermTMU;
    gc->state.tmuMaskShadow |= btmuMask;
    break;
  case GR_CMBX_OTHER_TEXTURE_ALPHA:
    texturemode |= SST_TC_MAOTHER;
    localTMU &= FXFALSE;
    break;
  case GR_CMBX_LOCAL_TEXTURE_ALPHA:
    texturemode |= SST_TC_MALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_DETAIL_FACTOR:
    texturemode |= SST_TC_MLOD;
    /* ??? */
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_LOD_FRAC:
    texturemode |= SST_TC_MLODFRAC;
    /* ??? */
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU &= FXFALSE;
    break;

  case GR_CMBX_LOCAL_TEXTURE_RGB:
    texturemode |= SST_TC_MCMSELECT7;
    combineMode |= SST_CM_TC_MSELECT_7_LOCAL_TRGB;
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_OTHER_TEXTURE_RGB:
    texturemode |= SST_TC_MCMSELECT7;
    combineMode |= SST_CM_TC_MSELECT_7_OTHER_TRGB;
    localTMU &= FXFALSE;
    break;
  case GR_CMBX_ITRGB:
    texturemode |= SST_TC_MCMSELECT7;
    combineMode |= SST_CM_TC_MSELECT_7_IRGB;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_ITALPHA:
    texturemode |= SST_TC_MCMSELECT7;
    combineMode |= SST_CM_TC_MSELECT_7_IA;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_TMU_CCOLOR:
    texturemode |= SST_TC_MCMSELECT7;
    combineMode |= SST_CM_TC_MSELECT_7_CR_RGB;
    gc->state.tcc_requires_constant_color[tmu] = FXTRUE;
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_TMU_CALPHA:
    texturemode |= SST_TC_MCMSELECT7;
    combineMode |= SST_CM_TC_MSELECT_7_CR_A;
    gc->state.tcc_requires_constant_color[tmu] = FXTRUE;
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU &= FXTRUE;
    break;
  }
  if (!c_invert)
    texturemode |= SST_TC_REVERSE_BLEND;

  switch (d) {
  case GR_CMBX_ZERO:
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_LOCAL_TEXTURE_ALPHA:
    texturemode |= SST_TC_ADD_ALOCAL;
    gc->state.tmuMaskShadow |= tmuMask;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_ITRGB:
    texturemode |= SST_TC_ADD_CLOCAL | SST_TC_ADD_ALOCAL;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_B:
    texturemode |= SST_TC_ADD_CLOCAL;
    localTMU &= btermTMU;
    gc->state.tmuMaskShadow |= btmuMask;
    break;
  }
  
  if (d_invert)
    combineMode |= SST_CM_TC_INVERT_ADD_LOCAL;

  switch (shift) {
  case 1:
    combineMode |=  SST_CM_TC_OUTSHIFT_2X;
    break;
  case 2:
    combineMode |=  SST_CM_TC_OUTSHIFT_4X;
    break;
  case 0:
  default:
    combineMode |=  SST_CM_TC_OUTSHIFT_1X;
    break;
  }
  if (invert) texturemode |= SST_TC_INVERT_OUTPUT;

  combineMode |= SST_CM_USE_COMBINE_MODE;
  
  /* Hack to enable TWO-PASS Trilinear */
  if (texturemode & SST_TRILINEAR) {
    if ((texturemode & SST_TC_ZERO_OTHER) &&
        (texturemode & SST_TC_BLEND_LODFRAC) &&
        !(texturemode & SST_TC_REVERSE_BLEND)) {
      tLod |= SST_LOD_ODD;
    }
  }

  /* Hack to use single pass trilinear in UMA state */
  if((texturemode & SST_TRILINEAR) &&
     (gc->state.grEnableArgs.texture_uma_mode == GR_MODE_ENABLE) &&
     (gc->state.per_tmu[tmu].evenOdd == GR_MIPMAPLEVELMASK_BOTH) &&
     (tmu == GR_TMU1)) {
    tLod |= SST_LOD_ODD;
  }

  tLod |= _gr_evenOdd_xlate_table[gc->state.per_tmu[tmu].evenOdd];

  /* update tmuMask for tac */
  if (gc->state.tmuMaskShadow & tmuMask) {
    gc->state.tcctmuMask |= tmuMask;
  }
  if (gc->state.tcc_requires_texture[tmu] || gc->state.tac_requires_texture[tmu]) {
    gc->state.tmuMaskShadow |= gc->state.tactmuMask | gc->state.tcctmuMask;
  }

  if(gc->state.tac_requires_constant_color[tmu] || gc->state.tcc_requires_constant_color[tmu])
    combineMode |= SST_CM_DISABLE_CHROMA_SUBSTITUTION;

  /*
   * AJB- If we are either turning on or turning off constant color
   * combining, validateState will need to swap the values in
   * chromaRange & chromaKey.
   */
  if ((combineMode & SST_CM_DISABLE_CHROMA_SUBSTITUTION) !=
     (gc->state.tmuShadow[tmu].combineMode & SST_CM_DISABLE_CHROMA_SUBSTITUTION))
    INVALIDATE_TMU(tmu, texchroma) ;

  /* Update shadows */
  INVALIDATE_TMU(tmu, textureMode);  
  gc->state.tmuShadow[tmu].tLOD        = tLod;
  gc->state.tmuShadow[tmu].combineMode = combineMode;
  gc->state.tmuShadow[tmu].textureMode = texturemode;

  /* If the state of a tmu changes from active then make sure that it
   * is addressing some valid tiny texture so that it does not spend
   * time thrashing on a large texture access.
   *
   * NB: We don't update the shadow here so the other bits in the
   * register shadow should be unchanged.
   */

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
} /* grTexColorCombineExt */

/*---------------------------------------------------------------------------
** grTexAlphaCombineExt
*/
GR_EXT_ENTRY(grTexAlphaCombineExt, void, 
         (GrChipID_t       tmu,
          GrTACUColor_t    a,
          GrCombineMode_t  a_mode,
          GrTACUColor_t    b,
          GrCombineMode_t  b_mode,
          GrTACUColor_t    c,
          FxBool           c_invert,
          GrTACUColor_t    d,
          FxBool           d_invert,
          FxU32            shift,
          FxBool           invert))
{
#define FN_NAME "grTexAlphaCombineExt"
  FxU32 texturemode;
  FxU32 combineMode;
  FxU32 tLod;
  FxU32 tmuMask;
  FxU32 btmuMask = 0;
  FxU32 localTMU = FXFALSE;
  FxU32 btermTMU = FXFALSE;
  GR_BEGIN_NOFIFOCHECK("grTexAlphaCombineExt",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",
                 tmu, a, a_mode, b, b_mode, c, d, shift, invert);

#if DEBUG_2PPC || DEBUG_COMBINEEXT
  GDBG_PRINTF("grTexAlphaCombineExt(tmu=%d,a=%s,am=%s,b=%s,bm=%s,c=%s,ci=%s,d=%s,di=%s,s=%s,i=%s)\n",
    tmu,cmbxName[a],funcName[a_mode],
        cmbxName[b],funcName[b_mode],
        cmbxName[c],boolName[c_invert],
        cmbxName[d],boolName[d_invert],
        shiftName[shift],
        boolName[invert]);
#endif

  /* 
  ** a = tca_otherselect
  ** a_mode = tca_invert_other
  ** b = tca_localselect
  ** b_mode = tca_invert_local
  ** c = tca_mselect
  ** d = (tca_add_clocal, tca_add_alocal)
  ** shift = tca_outshift
  ** invert = tca_invert_output
  */
  /* tmuMask tells grColorCombineFunction what to turn off an on if 
     the ccFunc requires texture mapping */
  combineMode = gc->state.tmuShadow[tmu].combineMode;
  texturemode = gc->state.tmuShadow[tmu].textureMode;
  texturemode &= ~(SST_TACOMBINE);
  tLod = gc->state.tmuShadow[tmu].tLOD;
  tLod &= ~(SST_LOD_ODD);

  gc->state.combineExtsInUse |= STATE_USING_TAC;

  tmuMask = GR_TMUMASK_TMU0 << tmu;
  gc->state.tmuMaskShadow &= ~tmuMask;
  gc->state.tactmuMask &= ~tmuMask;

  gc->state.tmuAlphaPassthrough &= ~tmuMask;

  /* Check for obvious passthrough case. Expand later if something other
   * than OpenGL ever does this sort of thing. */
  if(((a == GR_CMBX_OTHER_TEXTURE_ALPHA) && (a_mode == GR_FUNC_MODE_X)) &&
     ((b == GR_CMBX_ZERO) && (b_mode == GR_FUNC_MODE_X)) &&
     ((c == GR_CMBX_ZERO) && (c_invert == FXTRUE)) &&
     ((d == GR_CMBX_ZERO) && (d_invert == FXFALSE)) &&
     (shift == 0) &&
     (invert == FXFALSE)) {
    gc->state.tmuAlphaPassthrough |= tmuMask;
  }

  combineMode &= ~(
                   SST_CM_TCA_OTHERSELECT |
                   SST_CM_TCA_LOCALSELECT |
                   SST_CM_TCA_INVERT_OTHER |
                   SST_CM_TCA_INVERT_LOCAL |
                   SST_CM_TCA_OUTSHIFT |
                   SST_CM_TCA_INVERT_ADD_LOCAL |
                   SST_CM_TCA_OUTSHIFT |
                   SST_CM_DISABLE_CHROMA_SUBSTITUTION
                   );
  texturemode &= ~(
                   SST_TCA_MSELECT |              /* c */
                   SST_TCA_ADD_CLOCAL |           /* part of d */
                   SST_TCA_ADD_ALOCAL |           /* part of d */
                   SST_TCA_INVERT_OUTPUT |        /* invert */
                   SST_TCA_REVERSE_BLEND |
                   SST_TCA_ZERO_OTHER | 
                   SST_TCA_SUB_CLOCAL
                   );

  gc->state.tac_requires_it_alpha[tmu] = 
    ( (a == GR_CMBX_ITALPHA) |
      (b == GR_CMBX_ITALPHA) |
      (c == GR_CMBX_ITALPHA) |
      (d == GR_CMBX_ITALPHA) );  

  gc->state.tac_requires_prev_texture[tmu] = 
    ( (a == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (b == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (c == GR_CMBX_OTHER_TEXTURE_ALPHA) );

  gc->state.tac_requires_texture[tmu] = 
    ( (a == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (a == GR_CMBX_LOCAL_TEXTURE_ALPHA) |
      (b == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (b == GR_CMBX_LOCAL_TEXTURE_ALPHA) |
      (c == GR_CMBX_OTHER_TEXTURE_ALPHA) |
      (c == GR_CMBX_LOCAL_TEXTURE_ALPHA) |
      (d == GR_CMBX_LOCAL_TEXTURE_ALPHA) );  

  gc->state.tac_requires_constant_color[tmu] = FXFALSE;
      
  switch (a) {
  case GR_CMBX_OTHER_TEXTURE_ALPHA:
    combineMode |= SST_CM_TCA_OTHERSELECT_OTHER_TA;
    localTMU = FXFALSE;
    break;
  case GR_CMBX_LOCAL_TEXTURE_ALPHA:
    combineMode |= SST_CM_TCA_OTHERSELECT_LOCAL_TA;
    localTMU = FXTRUE;
    gc->state.tmuMaskShadow |= tmuMask;
    break;
  case GR_CMBX_ITALPHA:
    combineMode |= SST_CM_TCA_OTHERSELECT_IA;
    localTMU = FXTRUE;
    break;
  case GR_CMBX_TMU_CALPHA:
    combineMode |= SST_CM_TCA_OTHERSELECT_CR_A;
    gc->state.tac_requires_constant_color[tmu] = FXTRUE;
    localTMU = FXTRUE;
    break;
  default:
    /* output zero if the a term is unknown type */
    a_mode = GR_FUNC_MODE_ZERO;
    localTMU = FXTRUE;
    break;
  }

  switch (b) {
  case GR_CMBX_LOCAL_TEXTURE_ALPHA:
    combineMode |= SST_CM_TCA_LOCALSELECT_LOCAL_TA;
    if (b_mode != GR_FUNC_MODE_ZERO) {
      gc->state.tmuMaskShadow |= tmuMask;
      localTMU &= FXTRUE;
    }
    btermTMU = FXTRUE;
    btmuMask = tmuMask;
    break;
  case GR_CMBX_OTHER_TEXTURE_ALPHA:
    combineMode |= SST_CM_TCA_LOCALSELECT_OTHER_TA;
    if (b_mode != GR_FUNC_MODE_ZERO)
      localTMU &= FXFALSE;
    btermTMU = FXFALSE;
    break;
  case GR_CMBX_ITALPHA:
    combineMode |= SST_CM_TCA_LOCALSELECT_IA;
    if (b_mode != GR_FUNC_MODE_ZERO)
      localTMU &= FXTRUE;
    btermTMU = FXTRUE;
    break;
  case GR_CMBX_TMU_CALPHA:
    combineMode |= SST_CM_TCA_LOCALSELECT_CK_A;
    gc->state.tac_requires_constant_color[tmu] = FXTRUE;
    if (b_mode != GR_FUNC_MODE_ZERO)
      localTMU &= FXTRUE;
    btermTMU = FXTRUE;
    break;
  default:
    /* output zero if the b term is unknown type */
    b_mode = GR_FUNC_MODE_ZERO;
    if (b_mode != GR_FUNC_MODE_ZERO)
      localTMU &= FXTRUE;
    btermTMU = FXTRUE;
    break;
  }

  switch (a_mode) {
  case GR_FUNC_MODE_ZERO:
    combineMode |= SST_CM_TCA_INVERT_OTHER_X;
    break;
  case GR_FUNC_MODE_X:
    combineMode |= SST_CM_TCA_INVERT_OTHER_X;
    break;
  case GR_FUNC_MODE_NEGATIVE_X:
    combineMode |= SST_CM_TCA_INVERT_OTHER_ZERO_MINUS_X;
    break;
  case GR_FUNC_MODE_ONE_MINUS_X:
    combineMode |= SST_CM_TCA_INVERT_OTHER_ONE_MINUS_X;
    break;
  case GR_FUNC_MODE_X_MINUS_HALF:
    combineMode |= SST_CM_TCA_INVERT_OTHER_X_MINUS_HALF;
    break;
  }
  if (a_mode == GR_FUNC_MODE_ZERO)
    texturemode |= SST_TCA_ZERO_OTHER;

  switch (b_mode) {
  case GR_FUNC_MODE_ZERO:
    combineMode |= SST_CM_TCA_INVERT_LOCAL_X;
    break;
  case GR_FUNC_MODE_X:
    combineMode |= SST_CM_TCA_INVERT_LOCAL_X;
    break;
  case GR_FUNC_MODE_NEGATIVE_X:
    combineMode |= SST_CM_TCA_INVERT_LOCAL_ZERO_MINUS_X;
    break;
  case GR_FUNC_MODE_ONE_MINUS_X:
    combineMode |= SST_CM_TCA_INVERT_LOCAL_ONE_MINUS_X;
    break;
  case GR_FUNC_MODE_X_MINUS_HALF:
    combineMode |= SST_CM_TCA_INVERT_LOCAL_X_MINUS_HALF;
    break;
  }
  if (b_mode != GR_FUNC_MODE_ZERO)
    texturemode |= SST_TCA_SUB_CLOCAL;

  switch (c) {
  case GR_CMBX_ZERO:
    texturemode |= SST_TCA_MONE;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_B:
    texturemode |= SST_TCA_MCLOCAL;
    localTMU &= btermTMU;
    gc->state.tmuMaskShadow |= btmuMask;
    break;
  case GR_CMBX_OTHER_TEXTURE_ALPHA:
    texturemode |= SST_TCA_MAOTHER;
    localTMU &= FXFALSE;
    break;
  case GR_CMBX_LOCAL_TEXTURE_ALPHA:
    texturemode |= SST_TCA_MALOCAL;
    localTMU &= FXTRUE;
    gc->state.tmuMaskShadow |= tmuMask;
    break;
  case GR_CMBX_DETAIL_FACTOR:
    texturemode |= SST_TCA_MLOD;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_LOD_FRAC:
    texturemode |= SST_TCA_MLODFRAC;
    localTMU &= FXFALSE;
    break;
  case GR_CMBX_ITALPHA:
    texturemode |= SST_TCA_MAITER;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_TMU_CALPHA:
    texturemode |= SST_TCA_MCR;
    gc->state.tac_requires_constant_color[tmu] = FXTRUE;
    localTMU &= FXTRUE;
    break;
  }
  if (!c_invert)
    texturemode |= SST_TCA_REVERSE_BLEND;

  switch (d) {
  case GR_CMBX_ZERO:
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_LOCAL_TEXTURE_ALPHA:
    texturemode |= SST_TCA_ADD_ALOCAL;
    localTMU &= FXTRUE;
    gc->state.tmuMaskShadow |= tmuMask;
    break;
  case GR_CMBX_ITALPHA:
    texturemode |= SST_TCA_ADD_CLOCAL | SST_TCA_ADD_ALOCAL;
    localTMU &= FXTRUE;
    break;
  case GR_CMBX_B:
    texturemode |= SST_TCA_ADD_CLOCAL;
    localTMU &= btermTMU;
    gc->state.tmuMaskShadow |= btmuMask;
    break;
  }
  if (d_invert)
    combineMode |= SST_CM_TCA_INVERT_ADD_LOCAL;

  switch (shift) {
  case 1:
    combineMode |= SST_CM_TCA_OUTSHIFT_2X;
    break;
  case 2:
    combineMode |= SST_CM_TCA_OUTSHIFT_4X;
    break;
  case 0:
  default:
    combineMode |= SST_CM_TCA_OUTSHIFT_1X;
    break;
  }
  
  if (invert) texturemode |= SST_TCA_INVERT_OUTPUT;

  combineMode |= SST_CM_USE_COMBINE_MODE;
  
  /* Hack to enable TWO-PASS Trilinear */
  if (texturemode & SST_TRILINEAR) {
    if ((texturemode & SST_TC_ZERO_OTHER) &&
        (texturemode & SST_TC_BLEND_LODFRAC) &&
        !(texturemode & SST_TC_REVERSE_BLEND)) {
      tLod |= SST_LOD_ODD;
    }
  }

  /* Hack to use single pass trilinear in UMA state */
  if((texturemode & SST_TRILINEAR) &&
     (gc->state.grEnableArgs.texture_uma_mode == GR_MODE_ENABLE) &&
     (gc->state.per_tmu[tmu].evenOdd == GR_MIPMAPLEVELMASK_BOTH) &&
     (tmu == GR_TMU1)) {
    tLod |= SST_LOD_ODD;
  }
  
  tLod |= _gr_evenOdd_xlate_table[gc->state.per_tmu[tmu].evenOdd];

  /* update tmuMask for tac */
  if (gc->state.tmuMaskShadow & tmuMask) {
    gc->state.tactmuMask |= tmuMask;
  }
  if (gc->state.tcc_requires_texture[tmu] || gc->state.tac_requires_texture[tmu]) {
    gc->state.tmuMaskShadow |= gc->state.tactmuMask | gc->state.tcctmuMask;
  }

  if(gc->state.tac_requires_constant_color[tmu] || gc->state.tcc_requires_constant_color[tmu])
    combineMode |= SST_CM_DISABLE_CHROMA_SUBSTITUTION;

  /*
   * AJB- If we are either turning on or turning off constant color
   * combining, validateState will need to swap the values in
   * chromaRange & chromaKey.
   */
  if ((combineMode & SST_CM_DISABLE_CHROMA_SUBSTITUTION) !=
     (gc->state.tmuShadow[tmu].combineMode & SST_CM_DISABLE_CHROMA_SUBSTITUTION))
    INVALIDATE_TMU(tmu, texchroma) ;

  /* Update shadows */
  INVALIDATE_TMU(tmu, textureMode);
  gc->state.tmuShadow[tmu].tLOD        = tLod;
  gc->state.tmuShadow[tmu].combineMode = combineMode;
  gc->state.tmuShadow[tmu].textureMode = texturemode;
  

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
}


/*---------------------------------------------------------------------------
** grConstantColorValueExt
**
** A sad, sad story:
**
** Whoever designed this api extension overlooked the fact that there are two
** constant color inputs into the color combiner where here we only set one.
** (actually we set them both to the same thing, but in effect we only set one).
*/
GR_EXT_ENTRY(grConstantColorValueExt, void, 
          (GrChipID_t       tmu,
           GrColor_t        value))
{
#define FN_NAME "grConstantColorValueExt"
  FxU32 ccolor = value ;

  GR_BEGIN_NOFIFOCHECK("grConstantColorValueExt", 88) ;
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d)\n", tmu, value) ;

  _grSwizzleColor(&ccolor) ;

  INVALIDATE_TMU(tmu, texchroma);
  gc->state.tmuColor[tmu] = ccolor ;

  GR_END() ;

#undef FN_NAME
}

#endif /* FX_GLIDE_NAPALM */

/*
** _grTexDetailControl, NOTE: its up to caller to account for bytes
*/
GR_DDFUNC(_grTexDetailControl, void, (GrChipID_t tmu, FxU32 detail))
{
#define FN_NAME "_grTexDetailControl"
  GR_BEGIN_NOFIFOCHECK("_grTexDetailControl",88);

  GR_CHECK_TMU("_grTexDetailControl", tmu);

  INVALIDATE_TMU(tmu, textureMode);  
  gc->state.tmuShadow[tmu].tDetail = detail;

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

  GR_BEGIN_NOFIFOCHECK("grTexFilterMode",99);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d)\n",tmu,minfilter,magfilter);
  GR_CHECK_TMU(FN_NAME, tmu);

  texMode  = gc->state.tmuShadow[tmu].textureMode;
  texMode &= ~(SST_TMINFILTER | SST_TMAGFILTER);
  texMode |= (minfilter == GR_TEXTUREFILTER_BILINEAR ? SST_TMINFILTER : 0) |
             (magfilter == GR_TEXTUREFILTER_BILINEAR ? SST_TMAGFILTER : 0);

  gc->state.tmuShadow[tmu].textureMode = texMode;

  /* Update real shadows and update hardware immediately if we can. */
  if(!gc->state.mode2ppc || (tmu == gc->state.mode2ppcTMU)) {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    gc->state.shadow.tmuState[tmu].textureMode = texMode;
    if(gc->state.mode2ppc) {
      gc->state.shadow.tmuState[1-tmu].textureMode = texMode;
    }    
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    REG_GROUP_BEGIN((0x02 << tmu), textureMode, 1, 0x1);
    {
      REG_GROUP_SET(tmuHw, textureMode, gc->state.shadow.tmuState[tmu].textureMode);
    }
    REG_GROUP_END();
    _grChipMask( gc->chipmask );
  } else {
    INVALIDATE_TMU(tmu, textureMode);
  }


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

  GR_BEGIN_NOFIFOCHECK("grTexLodBiasValue",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%g)\n",tmu,fvalue);
  GR_CHECK_TMU(FN_NAME, tmu);
  
  tLod = gc->state.tmuShadow[tmu].tLOD;
  tLod &= ~(SST_LODBIAS);
  lodBias = _grTexFloatLODToFixedLOD(fvalue);
  /* Sign extend it. */
  lodBias = ((lodBias << (32-6)) >> (32-6));
  lodBias += _GlideRoot.environment.lodBias;
  if(lodBias > 0x1f) lodBias = 0x1f;
  else if(lodBias < -0x20) lodBias = -0x20;
  /* Mask it back off. */
  lodBias &= 0x3f;
  tLod |= (lodBias << SST_LODBIAS_SHIFT);
  
  gc->state.tmuShadow[tmu].tLOD = tLod;
  
  /* Update real shadows and update hardware immediately if we can. */
  if(!gc->state.mode2ppc || (tmu == gc->state.mode2ppcTMU)) {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    gc->state.shadow.tmuState[tmu].tLOD = tLod;
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    REG_GROUP_BEGIN((0x02 << tmu), tLOD, 1, 0x1);
    {
      REG_GROUP_SET(tmuHw, tLOD, gc->state.shadow.tmuState[tmu].tLOD);
    }
    REG_GROUP_END();
    _grChipMask( gc->chipmask );
    if(gc->state.per_tmu[tmu].texSubLodDither)
        g3LodBiasPerChip(tmu,gc->state.shadow.tmuState[tmu].tLOD);
  } else {
    INVALIDATE_TMU(tmu, textureMode);
  }
  
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
  FxBool tBig = FXFALSE;

  GR_BEGIN_NOFIFOCHECK("grTexMipMapMode",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d)\n",tmu,mmMode,lodBlend);
  GR_CHECK_TMU(FN_NAME, tmu);
  
  /*--------------------------------------------------------------
    Get Current tLod and texMode register values
    --------------------------------------------------------------*/
  tLod  = gc->state.tmuShadow[tmu].tLOD;
  texMode = gc->state.tmuShadow[tmu].textureMode;

  /*--------------------------------------------------------------
    Do we want big (>256) textures or not?
    --------------------------------------------------------------*/
#ifdef FX_GLIDE_NAPALM
  if ((IS_NAPALM(gc->bInfo->pciInfo.deviceID)) && (tLod & SST_TBIG)) {
    tBig = FXTRUE;
  }
#endif

  /*--------------------------------------------------------------
    Clear LODMIN, LODMAX and LODDITHER
    --------------------------------------------------------------*/
  tLod    &= ~(SST_LODMIN | SST_LODMAX | SST_LOD_ODD);
  texMode &= ~(SST_TLODDITHER | SST_TRILINEAR);
  gc->state.per_tmu[tmu].texSubLodDither = FXFALSE;

  /* Force LOD dithering if the user asked for it.
   *
   * NB: There is a performance hit for this, but it does
   * look better.
   */
  texMode |= _GlideRoot.environment.texLodDither;

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
#ifdef FX_GLIDE_NAPALM
    if(!(gc->sliCount == gc->chipCount && gc->grSamplesPerChip == 1) &&
       _GlideRoot.environment.texSubLodDither) {
      gc->state.per_tmu[tmu].texSubLodDither = FXTRUE;
      /* disable lod dithering if we are doing performance trilinear */
      texMode &= ~SST_TLODDITHER;
      mmMode = GR_MIPMAP_NEAREST;
    }
#endif
    break;

  default:
    GrErrorCallback("grTexMipMapMode:  invalid mode passed", FXFALSE);
    break;
  }
  gc->state.per_tmu[tmu].mmMode = mmMode;

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
  gc->state.tmuShadow[tmu].tLOD        = tLod;
  gc->state.tmuShadow[tmu].textureMode = texMode;


  /* Update real shadows and update hardware immediately if we can. */
  if(!gc->state.mode2ppc || (tmu == gc->state.mode2ppcTMU)) {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    gc->state.shadow.tmuState[tmu].textureMode = texMode;
    gc->state.shadow.tmuState[tmu].tLOD = tLod;
    if(gc->state.mode2ppc) {
      gc->state.shadow.tmuState[1-tmu].textureMode = texMode;
      gc->state.shadow.tmuState[1-tmu].tLOD = tLod;
    }    
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    REG_GROUP_BEGIN((0x02 << tmu), textureMode, 2, 0x3);
    {
      REG_GROUP_SET(tmuHw, textureMode, gc->state.shadow.tmuState[tmu].textureMode);
      REG_GROUP_SET(tmuHw, tLOD, gc->state.shadow.tmuState[tmu].tLOD);
    }
    REG_GROUP_END();
    _grChipMask( gc->chipmask );
    if(gc->state.per_tmu[tmu].texSubLodDither)
        g3LodBiasPerChip(tmu, gc->state.shadow.tmuState[tmu].tLOD);
  } else {
    INVALIDATE_TMU(tmu, textureMode);
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
    texMode  = gc->state.tmuShadow[i].textureMode;
    texMode &= ~(SST_TNCCSELECT);
    if (table == GR_TEXTABLE_NCC1)
      texMode |= SST_TNCCSELECT;
    else 
      texMode &= ~(SST_TNCCSELECT);

    gc->state.tmuShadow[i].textureMode = texMode;
  
    /* Update real shadows and update hardware immediately if we can. */
    /* This is a special case, we always update the tables on both TMUs */
    /* So we don't care whether or not we're in 2PPC mode. */
    {
       SstRegs* tmuHw = SST_TMU(hw, i);
    
      gc->state.shadow.tmuState[i].textureMode = texMode;
      
      _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
      REG_GROUP_BEGIN((0x02 << i), textureMode, 1, 0x1);
      {
        REG_GROUP_SET(tmuHw, textureMode, gc->state.shadow.tmuState[i].textureMode);
      }
      REG_GROUP_END();
      _grChipMask( gc->chipmask );
    }   
  }
  
  GR_END();
#undef FN_NAME
} /* grTexNCCTable */

#if GLIDE_POINTCAST_PALETTE
GR_EXT_ENTRY(grTexNCCTableExt, void, (GrChipID_t tmu, GrNCCTable_t table))
{
#define FN_NAME "grTexNCCTableExt"
  FxU32 texMode;
  
  GR_BEGIN("grTexNCCTable",88,4*GLIDE_NUM_TMU, GLIDE_NUM_TMU);
  GDBG_INFO_MORE(gc->myLevel,"\n");
  GR_CHECK_F(myName, table > GR_TEXTABLE_PALETTE, "invalid ncc table specified");

  /* Disable 2PPC mode */
#ifdef FX_GLIDE_NAPALM
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    _grTex2ppc(FXFALSE);
  }
#endif
  
  /* Update local state */
  gc->state.per_tmu[tmu].nccTable = table;
  
  /* Grab shadow texMode, update TexMode, update shadow/real register */
  texMode  = gc->state.tmuShadow[tmu].textureMode;
  texMode &= ~(SST_TNCCSELECT);
  if (table == GR_TEXTABLE_NCC1)
    texMode |= SST_TNCCSELECT;
  else 
    texMode &= ~(SST_TNCCSELECT);

  gc->state.tmuShadow[tmu].textureMode = texMode;

  /* Update real shadows and update hardware immediately if we can. */
  {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    gc->state.shadow.tmuState[tmu].textureMode = texMode;
      
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    REG_GROUP_BEGIN((0x02 << tmu), textureMode, 1, 0x1);
    {
      REG_GROUP_SET(tmuHw, textureMode, gc->state.shadow.tmuState[tmu].textureMode);
    }
    REG_GROUP_END();
    _grChipMask( gc->chipmask );
  }   

    
  GR_END();
#undef FN_NAME
} /* grTexNCCTableExt */
#endif

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
  FxBool tBig = FXFALSE, doNOP = FXFALSE;

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
                         (gc->windowed ? FXFALSE :
                          (startAddress +
                           _grTexTextureMemRequired(info->smallLodLog2,
                                                    info->largeLodLog2,
                                                    info->aspectRatioLog2,
                                                    info->format,
                                                    evenOdd,
                                                    FXTRUE)
                           > gc->tmu_state[tmu].total_mem)),
                         "insufficient texture ram at startAddress");
  GR_CHECK_F(FN_NAME, evenOdd > 0x3 || evenOdd == 0, "evenOdd mask invalid");
  GR_CHECK_F(FN_NAME, !info, "invalid info pointer");

#ifdef FX_GLIDE_NAPALM

  /*--------------------------------------------------------------
    Do we want big (>256) textures or not?
    --------------------------------------------------------------*/
  if ((IS_NAPALM(gc->bInfo->pciInfo.deviceID))
      && (info->largeLodLog2 > GR_LOD_LOG2_256)) {
    tBig = FXTRUE;
  }
#endif

  /*-------------------------------------------------------------
    Update Texture Unit State
    -------------------------------------------------------------*/
  gc->state.per_tmu[tmu].smallLod = _g3LodXlat(info->smallLodLog2, tBig);
  gc->state.per_tmu[tmu].largeLod = _g3LodXlat(info->largeLodLog2, tBig);
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
      baseAddress = SST_TEXTURE_MUNGE_ADDRESS(baseAddress);

      baseAddress |= SST_TEXTURE_IS_TILED;
      baseAddress |= (memInfo->texStrideTiles << SST_TEXTURE_TILESTRIDE_SHIFT);
      
      GR_CHECK_F(FN_NAME, (gc->windowed && (startAddress != 0)),
                 "Surface extension only supports startAddress 0\n");
    }  else {
      baseAddress = (FxI32)_grTexCalcBaseAddress(startAddress,
                                                 info->largeLodLog2,
                                                 G3_ASPECT_TRANSLATE(info->aspectRatioLog2),
                                                 info->format,
                                                 evenOdd);
      baseAddress += memInfo->tramOffset;
      baseAddress = SST_TEXTURE_MUNGE_ADDRESS(baseAddress);
    }
  }
  
  /*-------------------------------------------------------------
    Update Texture Mode
    -------------------------------------------------------------*/
  {
    GrTextureFormat_t format = info->format;
    FxU32 oldTexMode;
    oldTexMode = texMode = gc->state.tmuShadow[tmu].textureMode;
    texMode &= ~SST_TFORMAT;

#ifdef FX_GLIDE_NAPALM
    texMode &= ~SST_COMPRESSED_TEXTURES;
#endif

#if 0    
    if ((format == GR_TEXFMT_P_8) ||
        (format == GR_TEXFMT_YIQ_422) ||
        (format == GR_TEXFMT_AYIQ_8422) ||
        (format == GR_TEXFMT_AP_88)) {
      gc->state.palletizedTexture[tmu] = FXTRUE;
    } else {
      gc->state.palletizedTexture[tmu] = FXFALSE;
    }      
#endif
    
    /* Adjust texture format to match the current texture table type */
#ifdef FX_GLIDE_NAPALM
    /* Glide uses a different value than the HW's textureMode reg */
    switch(format) {
    case GR_TEXFMT_P_8:
      format = ((gc->state.tex_table == GR_TEXTABLE_PALETTE_6666_EXT)
                ? GR_TEXFMT_P_8_RGBA
                : GR_TEXFMT_P_8);
      break;
    case GR_TEXFMT_ARGB_8888:
      format = (SST_ARGB8888 >> SST_TFORMAT_SHIFT);
      break;
    case GR_TEXFMT_ARGB_CMP_FXT1:
      format = (SST_3DFX_COMPRESSED >> SST_TFORMAT_SHIFT);
      texMode |= SST_COMPRESSED_TEXTURES;
      break;
    case GR_TEXFMT_ARGB_CMP_DXT1:
      format = (SST_DXT1 >> SST_TFORMAT_SHIFT);
      texMode |= SST_COMPRESSED_TEXTURES;
      break;
    case GR_TEXFMT_ARGB_CMP_DXT2:
      format = (SST_DXT2 >> SST_TFORMAT_SHIFT);
      texMode |= SST_COMPRESSED_TEXTURES;
      break;
    case GR_TEXFMT_ARGB_CMP_DXT3:
      format = (SST_DXT3 >> SST_TFORMAT_SHIFT);
      texMode |= SST_COMPRESSED_TEXTURES;
      break;
    case GR_TEXFMT_ARGB_CMP_DXT4:
      format = (SST_DXT4 >> SST_TFORMAT_SHIFT);
      texMode |= SST_COMPRESSED_TEXTURES;
      break;
    case GR_TEXFMT_ARGB_CMP_DXT5:
      format = (SST_DXT5 >> SST_TFORMAT_SHIFT);
      texMode |= SST_COMPRESSED_TEXTURES;
      break;
    }
#else
    if (format == GR_TEXFMT_P_8) {
      format = ((gc->state.tex_table == GR_TEXTABLE_PALETTE_6666_EXT)
                ? GR_TEXFMT_P_8_RGBA
                : GR_TEXFMT_P_8);
    }
#endif

    /* There seems to be a minor hardware bug when switching from a
     * non-compressed texture to a compressed one.  We have to idle
     * the TMUs when this transition happens. */
    if((texMode ^ oldTexMode) & SST_COMPRESSED_TEXTURES & texMode) {
      doNOP = FXTRUE;
    }

    texMode |= (SST_TPERSP_ST | 
                SST_TCLAMPW |
                (format << SST_TFORMAT_SHIFT));
  }
    
  /*-------------------------------------------------------------
    Compute TLOD (keep LODBIAS intact)
    -------------------------------------------------------------*/
  {
    const FxU32 
      smallLOD    = _g3LodXlat(info->smallLodLog2, tBig),
      largeLOD    = _g3LodXlat(info->largeLodLog2, tBig),
      aspectRatio = G3_ASPECT_TRANSLATE(info->aspectRatioLog2);
    
    tLod = gc->state.tmuShadow[tmu].tLOD;
    tLod &= ~(SST_LODMIN | SST_LODMAX | SST_LOD_ASPECT |
      SST_LOD_TSPLIT | SST_LOD_ODD | SST_LOD_S_IS_WIDER);

#ifdef FX_GLIDE_NAPALM
    if (tBig) {
      tLod |= SST_TBIG;
    } else {
      tLod &= ~SST_TBIG;
    }
#endif

    if (gc->state.per_tmu[tmu].mmMode == GR_MIPMAP_DISABLE )
      tLod |= SST_TLOD_MINMAX_INT(largeLOD, largeLOD);
    else
      tLod |= SST_TLOD_MINMAX_INT(largeLOD, smallLOD);

    tLod |= _gr_evenOdd_xlate_table[evenOdd];
    tLod |= _gr_aspect_xlate_table[aspectRatio];
  }

  /* update shadows */
  gc->state.tmuShadow[tmu].textureMode = texMode; 
  gc->state.tmuShadow[tmu].tLOD        = tLod;
  gc->state.tmuShadow[tmu].texBaseAddr = baseAddress;

  /* Update real shadows and update hardware immediately if we can. */
  if(!gc->state.mode2ppc || (tmu == gc->state.mode2ppcTMU)) {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    gc->state.shadow.tmuState[tmu].textureMode = texMode;
    gc->state.shadow.tmuState[tmu].tLOD = tLod;
    gc->state.shadow.tmuState[tmu].texBaseAddr = baseAddress;
    /* If in 2PPC, then update both "real" TMU register shadows. */
    if(gc->state.mode2ppc) {
      gc->state.shadow.tmuState[1-tmu].textureMode = texMode;
      gc->state.shadow.tmuState[1-tmu].tLOD = tLod;
      gc->state.shadow.tmuState[1-tmu].texBaseAddr = baseAddress;
    }    
    if(doNOP) {
      GR_SET_EXPECTED_SIZE(sizeof(FxU32)*1, 1);
      GR_SET(eChipTMU0 | eChipTMU1, hw, nopCMD,  0);
      GR_CHECK_SIZE();
    }    
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    REG_GROUP_BEGIN((0x02UL << tmu), textureMode, 3, 0x0B);
    {
      REG_GROUP_SET(tmuHw, textureMode, gc->state.shadow.tmuState[tmu].textureMode);
      REG_GROUP_SET(tmuHw, tLOD, gc->state.shadow.tmuState[tmu].tLOD);
      REG_GROUP_SET(tmuHw, texBaseAddr, gc->state.shadow.tmuState[tmu].texBaseAddr);
    }
    REG_GROUP_END();
    _grChipMask( gc->chipmask );
    if(gc->state.per_tmu[tmu].texSubLodDither)
        g3LodBiasPerChip(tmu, gc->state.shadow.tmuState[tmu].tLOD);
  } else {
    INVALIDATE_TMU(tmu, textureMode);
    INVALIDATE_TMU(tmu, texBaseAddr);
  }

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
    
  GR_BEGIN_NOFIFOCHECK("grTexMultibase",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d)\n",tmu,enable);
  GR_CHECK_TMU(FN_NAME,tmu);
  
  tLod  = gc->state.tmuShadow[tmu].tLOD;
  if (enable)
    tLod |= SST_TMULTIBASEADDR;
  else
    tLod &= ~SST_TMULTIBASEADDR;
  /*--------------------------------------------------------------
    Write State To Hardware and Update Glide Shadow State
    --------------------------------------------------------------*/
  gc->state.tmuShadow[tmu].tLOD = tLod;

  /* Update real shadows and update hardware immediately if we can. */
  if(!gc->state.mode2ppc || (tmu == gc->state.mode2ppcTMU)) {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    gc->state.shadow.tmuState[tmu].tLOD = tLod;
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    REG_GROUP_BEGIN((0x02 << tmu), tLOD, 1, 0x1);
    {
      REG_GROUP_SET(tmuHw, tLOD, gc->state.shadow.tmuState[tmu].tLOD);
    }
    REG_GROUP_END();
    _grChipMask( gc->chipmask );
    if(gc->state.per_tmu[tmu].texSubLodDither)
        g3LodBiasPerChip(tmu, gc->state.shadow.tmuState[tmu].tLOD);
  } else {
    INVALIDATE_TMU(tmu, textureMode);
  }

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
             GR_TEXBASE_2048     \  These are not valid
             GR_TEXBASE_1024      \ for Banshee or     
             GR_TEXBASE_512       / Voodoo3. They are  
             GR_TEXBASE_256_TO_1 /  valid for Napalm.  
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
         (GrChipID_t tmu, GrTexBaseRange_t range, FxU32 startAddress,
          FxU32 evenOdd, GrTexInfo *info))
{
#define FN_NAME "grTexMultibaseAddress"
#if GLIDE_DEBUG
  GrTexBaseRange_t
    maxPossibleLodRange = GR_TEXBASE_256; /* But see below for Napalm */
#endif

  GR_BEGIN_NOFIFOCHECK("grTexMultibaseAddress", 88);
  GDBG_INFO_MORE(gc->myLevel,"(%d, 0x%X, 0x%X)\n", tmu, range, startAddress);

  GR_CHECK_TMU(FN_NAME, tmu);
#if 0 /* GLIDE_DEBUG  FIXME: this is NOT handled in the case below */
  if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
    maxPossibleLodRange = GR_TEXBASE_2048;
  } 
#endif
  GR_CHECK_F(FN_NAME, range > maxPossibleLodRange, "invalid range");
  GR_CHECK_F(FN_NAME, startAddress >= gc->tmu_state[tmu].total_mem,
             "invalid startAddress");
  GR_CHECK_F(FN_NAME, evenOdd > 0x3, "evenOdd mask invalid");
  GR_CHECK_F(FN_NAME, info == NULL, "invalid info pointer");  

  /* Write relevant registers out to hardware and shadows */
  {
    GrLOD_t
      largeLevelLod;
    FxU32
      baseAddrRegIndex,
      *addrRegShadow,
      *addrRegShadowHW, *addrRegShadowHWAlt;

    switch (range) {
    case GR_TEXBASE_256:
      largeLevelLod = GR_LOD_LOG2_256;
      baseAddrRegIndex = (offsetof(SstRegs, texBaseAddr) >> 2UL);
      addrRegShadow = &gc->state.tmuShadow[tmu].texBaseAddr;
      addrRegShadowHW = &gc->state.shadow.tmuState[tmu].texBaseAddr;
      addrRegShadowHWAlt = &gc->state.shadow.tmuState[1-tmu].texBaseAddr;
      break;
      
    case GR_TEXBASE_128:
      largeLevelLod = GR_LOD_LOG2_128;
      baseAddrRegIndex = (offsetof(SstRegs, texBaseAddr1) >> 2UL);
      addrRegShadow = &gc->state.tmuShadow[tmu].texBaseAddr_1;
      addrRegShadowHW = &gc->state.shadow.tmuState[tmu].texBaseAddr_1;
      addrRegShadowHWAlt = &gc->state.shadow.tmuState[1-tmu].texBaseAddr_1;
      break;
      
    case GR_TEXBASE_64:
      largeLevelLod = GR_LOD_LOG2_64;
      baseAddrRegIndex = (offsetof(SstRegs, texBaseAddr2) >> 2UL);
      addrRegShadow = &gc->state.tmuShadow[tmu].texBaseAddr_2;
      addrRegShadowHW = &gc->state.shadow.tmuState[tmu].texBaseAddr_2;
      addrRegShadowHWAlt = &gc->state.shadow.tmuState[1-tmu].texBaseAddr_2;
      break;
      
    case GR_TEXBASE_32_TO_1:
    default: /* should never happen because of range check above */
      largeLevelLod = GR_LOD_LOG2_32;
      baseAddrRegIndex = (offsetof(SstRegs, texBaseAddr38) >> 2UL);
      addrRegShadow = &gc->state.tmuShadow[tmu].texBaseAddr_3_8;
      addrRegShadowHW = &gc->state.shadow.tmuState[tmu].texBaseAddr_3_8;
      addrRegShadowHWAlt = &gc->state.shadow.tmuState[1-tmu].texBaseAddr_3_8;
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
      
      *addrRegShadow = baseAddress;
      
      /* Update real shadows and update hardware immediately if we can. */
      if(!gc->state.mode2ppc || (tmu == gc->state.mode2ppcTMU)) {
        SstRegs* tmuHw = SST_TMU(hw, tmu);
    
        *addrRegShadowHW = baseAddress;
        if(gc->state.mode2ppc) {
          *addrRegShadowHWAlt = baseAddress;
        }    
        _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
        GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
        GR_SET_INDEX(tmuChip, tmuHw, baseAddrRegIndex, baseAddress);
        GR_CHECK_SIZE();
        _grChipMask( gc->chipmask );

      } else {
        INVALIDATE_TMU(tmu, texBaseAddr);
      }
      
      /* Synch flush data w/ new state */
      if (addrRegShadow == &gc->state.tmuShadow[tmu].texBaseAddr) {
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

  chromaRange = gc->state.tmuShadow[tmu].texchromaRange;
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
  
  gc->state.tmuShadow[tmu].texchromaRange = chromaRange;

  /* Update real shadows and update hardware immediately if we can. */
  if(!gc->state.mode2ppc || (tmu == gc->state.mode2ppcTMU)) {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    gc->state.shadow.tmuState[tmu].texchromaRange = chromaRange;
    if(gc->state.mode2ppc) {
      gc->state.shadow.tmuState[1-tmu].texchromaRange = chromaRange;
    }    
    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    REG_GROUP_BEGIN((0x02 << tmu), chromaRange, 1, 0x1);
    {
      REG_GROUP_SET(tmuHw, chromaRange, gc->state.shadow.tmuState[tmu].texchromaRange);
    }
    REG_GROUP_END();
    _grChipMask( gc->chipmask );
  } else {
    INVALIDATE_TMU(tmu, texchroma);
  }

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
  GR_BEGIN_NOFIFOCHECK("grTexChromaRange",85);

  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", tmu);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", min);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", max);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x)\n", mode);

  _grSwizzleColor(&minColor);
  _grSwizzleColor(&maxColor);
  
  minColor = minColor & 0x00ffffff;
  maxColor = maxColor & 0x00ffffff;
  chromaRange = gc->state.tmuShadow[tmu].texchromaRange;
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

  gc->state.tmuShadow[tmu].texchromaKey = minColor;
  gc->state.tmuShadow[tmu].texchromaRange = chromaRange;

  /* Update real shadows and update hardware immediately if we can. */
  if(!gc->state.mode2ppc || (tmu == gc->state.mode2ppcTMU)) {
    SstRegs* tmuHw = SST_TMU(hw, tmu);
    
    gc->state.shadow.tmuState[tmu].texchromaKey = minColor;
    gc->state.shadow.tmuState[tmu].texchromaRange = chromaRange;
    if(gc->state.mode2ppc) {
      gc->state.shadow.tmuState[1-tmu].texchromaKey = minColor;
      gc->state.shadow.tmuState[1-tmu].texchromaRange = chromaRange;
    }    

    _grChipMask( SST_CHIP_MASK_ALL_CHIPS );
    REG_GROUP_BEGIN((0x02 << tmu), chromaKey, 2, 0x3);
    {
      REG_GROUP_SET(tmuHw, chromaKey, gc->state.shadow.tmuState[tmu].texchromaKey);
      REG_GROUP_SET(tmuHw, chromaRange, gc->state.shadow.tmuState[tmu].texchromaRange);
    }
    REG_GROUP_END();
    _grChipMask( gc->chipmask );
  } else {
    INVALIDATE_TMU(tmu, texchroma);
  }

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
  FxU32 tLod = gc->state.tmuShadow[tmu].tLOD;
  FxBool tBig = FXFALSE;

#ifdef FX_GLIDE_NAPALM
  if ((IS_NAPALM(gc->bInfo->pciInfo.deviceID)) && (tLod & SST_TBIG)) {
    tBig = FXTRUE;
  }
#endif
  
  value = _g3LodXlat(value, tBig);

  tLod &= ~(SST_LODMIN | SST_LODMAX);
  tLod |= SST_TLOD_MINMAX_INT(value,value);

  INVALIDATE_TMU(tmu, textureMode);
  gc->state.tmuShadow[tmu].tLOD = tLod;
#undef FN_NAME
} /* _grTexForceLod */


/* tbext */
/*-------------------------------------------------------------------
  Function: grTextureBuffer
  Date: 
  Implementor(s): DA.
  Library: glide
  Description:
  Arguments:
  Return:
    none
-------------------------------------------------------------------*/

GR_DIENTRY(grTextureBuffer, void,
         (GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLOD, GrLOD_t largeLOD,
          GrAspectRatio_t aspectRatio, GrTextureFormat_t format, FxU32 odd_even_mask ))
{
  /*
  ** This assumes:
  ** frame buffer in Tiled memory
  ** texture in linear memory
  ** What about Napalm which offers 16 & 32 bits frame buffer formats?
  */
#define FN_NAME "grTextureBuffer"
  FxU32 add = 0,
        offset = 0;
  int width = _grMipMapHostWH[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][0],
      height = _grMipMapHostWH[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][1];
  FxU32 dwBytesPerPixel ;
  FxU32 stride ;
  
  GR_BEGIN_NOFIFOCHECK("grTextureBuffer", 88);

  GR_CHECK_F(myName,
             ((_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Banshee) &&
              (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Voodoo3) &&
              (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Voodoo4)),
             "grTextureBuffer not supported.");

  dwBytesPerPixel = gc->grPixelSize ; // If the format is not the same as FB format, we're in trouble.
  stride = width * dwBytesPerPixel ;

#ifdef FX_GLIDE_NAPALM
  /*
   * AJB- Last I checked we couldn't render to an FXT1 surface...
   *
  if (format == GR_TEXFMT_ARGB_CMP_FXT1) {
    width = _grMipMapHostWHCmp4Bit[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][0];
    height = _grMipMapHostWHCmp4Bit[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][1];
  } else if (format >= GR_TEXFMT_ARGB_CMP_DXT1 && format <= GR_TEXFMT_ARGB_CMP_DXT5) {
    width = _grMipMapHostWHDXT[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][0];
    height = _grMipMapHostWHDXT[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][1];
  } else {
    width = _grMipMapHostWH[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][0],
    height = _grMipMapHostWH[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][1];
  }
  stride = width * dwBytesPerPixel; */
  GR_CHECK_F(myName,
             (((gc->grPixelSize == 4) && (format != GR_TEXFMT_ARGB_8888)) ||
             ((gc->grPixelSize == 2) && ((format != GR_TEXFMT_RGB_565) && (format != GR_TEXFMT_ARGB_1555)))),
             "format incompatible with rendermode");
#endif

  /* Planning ahead... 
  ** For Napalm & later: we need to check that the format is one
  ** of the supported rendering formats. If it is and if it is different
  ** than the current rendering format then we throw en exception.
  ** Let me explain: we could, but I don't thing we'd want to, change
  ** the rendering format to the target format. This would be easy.
  ** The hard part would be restoring it to the 'default' rendering format.
  */

  /* not testing thisLOD > largeLOD...*/
  if ( thisLOD == largeLOD )
    offset = 0;
  else
    offset = _grTexTextureMemRequired( thisLOD+1, largeLOD, 
                                       aspectRatio, format,
                                       odd_even_mask, FXTRUE );

  /* How do we deal with UMA?
  The tmu only makes sense when the UMA is not turned on.
  Or could I use mminfo->tmu_base_address for all cases?
  if ( UMA ) {
    add = gc->tmuMemInfo[0].tramOffset + startAddress + offset;
  } else
  */
  add = gc->tmuMemInfo[tmu].tramOffset + startAddress + offset;
  
  if ( gc->state.shadow.fbzMode & SST_YORIGIN ) {
    add -= ( gc->state.screen_height - height ) * width * dwBytesPerPixel;
  }
  
  gc->textureBuffer.init = FXTRUE;
  gc->textureBuffer.addr = add;
  gc->textureBuffer.stride = stride;
  gc->textureBuffer.width = width;
  gc->textureBuffer.height = height;

  gc->curBuffer = 0xffffffff;

  if ( gc->textureBuffer.on && gc->textureBuffer.prevState.valid ) {
    INVALIDATE(fbzMode);
    grClipWindow(0, 0, gc->textureBuffer.width, gc->textureBuffer.height);
    _grValidateClipState(0, 0, gc->textureBuffer.width, gc->textureBuffer.height);
  } else
      gc->textureBuffer.prevState.valid = FXFALSE;

  GR_END();
#undef FN_NAME
} /* grTextureBuffer */



/* tbext */
/*-------------------------------------------------------------------
  Function: grTextureAuxBuffer
  Date: 
  Implementor(s): DA.
  Library: glide
  Description:
  Arguments:
  Return:
    none
-------------------------------------------------------------------*/

GR_DIENTRY(grTextureAuxBuffer, void,
         (GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLOD, GrLOD_t largeLOD,
          GrAspectRatio_t aspectRatio, GrTextureFormat_t format, FxU32 odd_even_mask ))
{
  
#define FN_NAME "grTextureAuxBuffer"
  FxU32 add = 0,
        offset = 0;
  int width = _grMipMapHostWH[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][0],
      height = _grMipMapHostWH[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][1];
  FxU32 dwBytesPerPixel ;
  FxU32 stride ;
  
  GR_BEGIN_NOFIFOCHECK("grTextureAuxBuffer", 88);

  GR_CHECK_F(myName,
             ((_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Banshee) &&
              (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Voodoo3) &&
              (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Voodoo4)),
             "grTextureBuffer not supported.");

  dwBytesPerPixel = gc->grPixelSize ; // If the format is not the same as FB format, we're in trouble.
  stride = width * dwBytesPerPixel ;

#ifdef FX_GLIDE_NAPALM
  /*
   * AJB- Using FXT1 for an aux buffer looks like it would be a mistake to me as well.
   *
  if (format == GR_TEXFMT_ARGB_CMP_FXT1) {
    width = _grMipMapHostWHCmp4Bit[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][0];
    height = _grMipMapHostWHCmp4Bit[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][1];
  } else if (format >= GR_TEXFMT_ARGB_CMP_DXT1 && format <= GR_TEXFMT_ARGB_CMP_DXT5) {
    width = _grMipMapHostWHDXT[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][0];
    height = _grMipMapHostWHDXT[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][1];
  } else {
    width = _grMipMapHostWH[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][0],
    height = _grMipMapHostWH[G3_ASPECT_TRANSLATE(aspectRatio)][thisLOD][1];
  }
  stride = width * dwBytesPerPixel; */
  GR_CHECK_F(myName,
             (((gc->grPixelSize == 4) && (format != GR_TEXFMT_ARGB_8888)) ||
             ((gc->grPixelSize == 2) && ((format != GR_TEXFMT_RGB_565) && (format != GR_TEXFMT_ARGB_1555)))),
             "format incompatible with rendermode");
#endif

  /* not testing thisLOD > largeLOD...*/
  if ( thisLOD == largeLOD )
    offset = 0;
  else
    offset = _grTexTextureMemRequired( thisLOD+1, largeLOD, 
                                       aspectRatio, format,
                                       odd_even_mask, FXTRUE );

  /* How do we deal with UMA?
  The tmu only makes sense when the UMA is not turned on.
  Or could I use mminfo->tmu_base_address for all cases?
  if ( UMA ) {
  add = gc->tmuMemInfo[0].tramOffset + startAddress + offset;
  } else
  */
  add = gc->tmuMemInfo[tmu].tramOffset + startAddress + offset;
  
  if ( gc->state.shadow.fbzMode & SST_YORIGIN ) {
    add -= ( gc->state.screen_height - height ) * width * dwBytesPerPixel;
  }
  
  gc->textureAuxBuffer.init = FXTRUE;
  gc->textureAuxBuffer.addr = add;
  gc->textureAuxBuffer.stride = stride;
  gc->textureAuxBuffer.width = width;
  gc->textureAuxBuffer.height = height;

  GR_END();
#undef FN_NAME
} /* grTextureAuxBuffer */


GR_DIENTRY(grAuxBuffer, void, (GrBuffer_t buffer ))
{
  
#define FN_NAME "grAuxBuffer"
  
  GR_BEGIN_NOFIFOCHECK("grAuxBuffer", 88);

  GR_CHECK_F(myName,
             ((_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Banshee) &&
              (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Voodoo3) &&
              (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type != GR_SSTTYPE_Voodoo4)),
             "grAuxBuffer not supported.");
  switch( buffer ){
  case GR_BUFFER_AUXBUFFER:
      gc->state.shadow.auxBufferAddr = gc->buffers0[gc->grColBuf];
      /* gc->grColBuf is the # of buffer, aux buffer included, and
      ** it is assumed that the grColBuf th buffer is the aux buffer.
      ** What happens when we have more than 1 aux buffers?
      ** Like a Z buffer and an alpha buffer?
      */
      /* gc->strideInTiles may have been changed during the texturebuffer.on
         mode? No? */
      gc->state.shadow.auxBufferStride = gc->strideInTiles | SST_BUFFER_MEMORY_TILED;
      gc->textureAuxBuffer.on = FXFALSE;
    break;

  case GR_BUFFER_TEXTUREAUXBUFFER_EXT:
      GR_CHECK_F(myName, !gc->textureAuxBuffer.init, "You need to call grTextureAuxBuffer once before calling grAuxBuffer with GR_BUFFER_TEXTUREAUXBUFFER_EXT" );
      gc->state.shadow.auxBufferAddr = gc->textureAuxBuffer.addr;
      gc->state.shadow.auxBufferStride = gc->textureAuxBuffer.stride;
      gc->textureAuxBuffer.on = FXTRUE;
    break;
  }

  /* Reset the registers
  ** Note that this is not going thru the glide state monster / validate mechanism
  */
  REG_GROUP_BEGIN(BROADCAST_ID, auxBufferAddr, 2, 0x3);
  {
    REG_GROUP_SET(hw, auxBufferAddr, gc->state.shadow.auxBufferAddr);
    REG_GROUP_SET(hw, auxBufferStride, gc->state.shadow.auxBufferStride); 
  }
  REG_GROUP_END();
  GR_END();
#undef FN_NAME
} /* grAuxBuffer */

#if 0 /* KoolSmoky - don't need this anymore */
/*-------------------------------------------------------------------
  Function: _g3LodXlat
  Date: 1-Jul-99
  Implementor(s): LEW
  Description:
  Translate LOD value between software's sense to hardware's sense.

  Arguments:
  someLod: A GrLOD_t index to be translated (subtracted from
    GR_LOD_LOG2_2048 if we're using big textures, or from
    GR_LOD_LOG2_256 if we're not.)
  tBig: Boolean telling us whether big textures are to be used.  Use
    the SST_TBIG bit from the tLOD register for this.
  
  NB: SST_TBIG didn't exist before Napalm, so if we are on Banshee or
  Voodoo3 then we know for sure that tBig is false.
    
  Return:
  The translated LOD index.
  -------------------------------------------------------------------*/
GrLOD_t
_g3LodXlat(const GrLOD_t someLOD, const FxBool tBig)
{
#define FN_NAME "_g3LodXlat"
  if (tBig) {
    return GR_LOD_LOG2_2048 - someLOD;
  } else {
    return GR_LOD_LOG2_256 - someLOD;
  }
#undef FN_NAME
} /* _g3LodXlat */
#endif

/*-------------------------------------------------------------------
  Function: 		_g3LodBiasPerChip

  Date: 			14-Nov-2000

  Implementor: 		Jonny Cochrane

  Description: 		Sub sample LOD Dithering. Called if TMU is in
                        texSubLodDither state.

                        2chip x2fsaa - no sli, 1 sample per chip
                        2chip x4fsaa - no sli, 2 samples per chip
                        4chip x2fsaa - 2 way sli, 1 sample per sli unit
                        4chip x4fsaa - no sli, 1 sample per chip
                        4chip x8fsaa - no sli, 2 samples per chip

  Arguments:		tmu: TMU to set. tLod: tLOD to modify.

  Return:		None
  -------------------------------------------------------------------*/
void g3LodBiasPerChip(GrChipID_t tmu, FxU32 tLod)
{
#define FN_NAME "g3LodBiasPerChip"
  GR_BEGIN_NOFIFOCHECK("g3LodBiasPerChip", 88);

  if( /*(_GlideRoot.environment.texSubLodDither != 1) ||*//* we won't get here if 0 */
      /*((gc->sliCount == gc->chipCount) && (gc->grSamplesPerChip == 1)) ||*//* check done in grTexMipMapMode */
      /*(gc->windowed) ||*/
      (gc->state.per_tmu[tmu].evenOdd != 3) ||
      (gc->state.tmuShadow[tmu].textureMode & SST_TRILINEAR) )
    goto FORGET_IT;

  {
    unsigned int i;
    FxU32 newtLod;
    FxI32 newLodBias;
    /* sign extend it. do not remove the FxI32 cast. */
    const FxI32 oldLodBias = ((((FxI32)tLod & SST_LODBIAS) << (32-6-SST_LODBIAS_SHIFT)) >> (32-6-SST_LODBIAS_SHIFT));
    const unsigned int idx = ((gc->chipCount - gc->sliCount) > 2);
    /* 4.2 format for tLod register
     * 1chip x2fsaa - no sli, 2 samples per chip
     * 2chip x2fsaa - no sli, 1 sample per chip
     * 2chip x4fsaa - no sli, 2 samples per chip
     * 4chip x2fsaa - 2 way sli, 1 sample per sli unit
     * 0.00, 0.50, 0.00, 0.50 - the last 2 are used only when 4chip
     * {0x00, 0x02, 0x00, 0x02},
     * 4chip x4fsaa - no sli, 1 sample per chip
     * 4chip x8fsaa - no sli, 2 samples per chip
     * 0.00, 0.25, 0.50, 0.75 - 4 chip
     * {0x00, 0x01, 0x02, 0x03}
     */
    static const FxI32 chipLodBias[2][4] =
    {
      {0x00<<SST_LODBIAS_SHIFT, 0x02<<SST_LODBIAS_SHIFT, 0x00<<SST_LODBIAS_SHIFT, 0x02<<SST_LODBIAS_SHIFT},
      {0x00<<SST_LODBIAS_SHIFT, 0x01<<SST_LODBIAS_SHIFT, 0x02<<SST_LODBIAS_SHIFT, 0x03<<SST_LODBIAS_SHIFT}
    };

    /* unmask lodbias */
    tLod &= ~(SST_LODBIAS);

    for (i = 0; i < gc->chipCount; i++)
    {
      /* don't need to modify lodbias if 0 */
      if(chipLodBias[idx][i] != 0) {
        newLodBias = oldLodBias + chipLodBias[idx][i];

        if (newLodBias > (0x1f<<SST_LODBIAS_SHIFT))
          newLodBias = (0x1f<<SST_LODBIAS_SHIFT);
        /*else if (newLodBias < (-0x20<<SST_LODBIAS_SHIFT))
          newLodBias = (-0x20<<SST_LODBIAS_SHIFT);*/

        newtLod = (tLod | (newLodBias & SST_LODBIAS));

        {
          SstRegs* tmuHw = SST_TMU(hw, tmu);
          _grChipMask( 0x01 << i );
          REG_GROUP_BEGIN((0x02 << tmu), tLOD, 1, 0x1);
          {
            REG_GROUP_SET(tmuHw, tLOD, newtLod);
          }
          REG_GROUP_END();
        }
      }
    }

    /* Restore chip mask */
    _grChipMask( gc->chipmask );
  }

FORGET_IT:
  GR_END();
#undef FN_NAME
}

#if 0 /* KoolSmoky - don't need this anymore */
/*-------------------------------------------------------------------
  Function: 		MultitextureAndTrilinear

  Date:                 4-Nov-2000

  Implementor: 		Jonny Cochrane

  Description: 		Determines if we are multitexturing and NOT
                        trilinear filtering. so that we can do sub
                        sample lod dithering which gives an almost
                        trilinear result.
                        mipmap dithering (lod dithering) is passed
                        through, see g3LodBiasPerChip
  					
  Arguments:		None  

  Return:               tmuMask of the TMUs that can do it
  -------------------------------------------------------------------*/
GrChipID_t MultitextureAndTrilinear(void)
{

#define FN_NAME "MultitextureAndTrilinear"
  
  GR_DCL_GC;

  GrChipID_t retval = FXBIT(0x4);

   /* KoolSmoky - we can only do this if we are running in;
    *   2chip x2fsaa - no sli, 1 sample per chip
    *   2chip x4fsaa - no sli, 2 samples per chip
    *   4chip x2fsaa - 2 way sli, 1 sample per sli unit
    *   4chip x4fsaa - no sli, 1 sample per chip
    *   4chip x8fsaa - no sli, 2 samples per chip
    *   ...and with FX_GLIDE_LOD_SUBSAMPLE_DITHER=1
    */
  
  if( (_GlideRoot.environment.texSubLodDither != 1) ||
      (gc->sliCount == gc->chipCount) ||
      (gc->windowed) )
    return retval;

#if 0

#if 0 /* Jonny Cochrane's method */
  if(_GlideRoot.environment.texLodDither) return 0;
  
  if( (gc->state.per_tmu[0].evenOdd == 3) && //both even and odd
      (gc->state.per_tmu[1].evenOdd == 3) && //both even and odd
      (gc->state.tmuShadow[0].textureMode & (SST_TMINFILTER | SST_TMAGFILTER)) && //and bilinear for mag and min filter
      (gc->state.tmuShadow[1].textureMode & (SST_TMINFILTER | SST_TMAGFILTER)) )  //and bilinear for mag and min filter
    retval |= (GR_TMUMASK_TMU0 | GR_TMUMASK_TMU1);
  
#else /* Colourless's method if bilinear filtering is required */
  if( (gc->state.per_tmu[0].evenOdd == 3) && //both even and odd
      (gc->state.tmuShadow[0].textureMode & (SST_TMINFILTER | SST_TMAGFILTER)) && //and bilinear for mag and min filter
      !(gc->state.tmuShadow[0].textureMode & (SST_TRILINEAR | SST_TLODDITHER)) ) //and not mipmap dithering or trilinear
    retval |= GR_TMUMASK_TMU0;

  if( (gc->state.per_tmu[1].evenOdd == 3) && //both even and odd
      (gc->state.tmuShadow[1].textureMode & (SST_TMINFILTER | SST_TMAGFILTER)) && //and bilinear for mag and min filter
      !(gc->state.tmuShadow[1].textureMode & (SST_TRILINEAR | SST_TLODDITHER)) ) //and not mipmap dithering or trilinear
    retval |= GR_TMUMASK_TMU1;
#endif

#else

#if 0 /* Colourless's method */
  if( (gc->state.per_tmu[0].evenOdd == 3) && //both even and odd
      !(gc->state.tmuShadow[0].textureMode & (SST_TRILINEAR | SST_TLODDITHER)) ) //and not trilinear and not dither mode
    retval |= GR_TMUMASK_TMU0;

  if( (gc->state.per_tmu[1].evenOdd == 3) && //both even and odd
      !(gc->state.tmuShadow[1].textureMode & (SST_TRILINEAR | SST_TLODDITHER)) && //and not trilinear and not dither mode
      !(gc->state.tmuShadow[0].textureMode & SST_TRILINEAR) )
    retval |= GR_TMUMASK_TMU1;
  
#else /* KoolSmoky - we will allow mip dithering. */
  if( !(gc->state.tmuShadow[1].textureMode & SST_TRILINEAR) && 
      !(gc->state.tmuShadow[0].textureMode & SST_TRILINEAR) ) { //not single or two pass trilinear
    if(gc->state.per_tmu[0].evenOdd == 3) //both even and odd
      retval |= GR_TMUMASK_TMU0;
    if(gc->state.per_tmu[1].evenOdd == 3) //both even and odd
      retval |= GR_TMUMASK_TMU1;
  }
#endif
  
#endif

  return retval;
}
#endif
