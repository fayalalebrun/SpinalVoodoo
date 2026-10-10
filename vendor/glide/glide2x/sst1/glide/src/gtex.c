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

extern const int _grMipMapHostWH[GR_ASPECT_1x8+1][GR_LOD_1+1][2];
extern FxU32 _grMipMapHostSize[][16];
extern FxU32 _gr_aspect_index_table[];
extern FxU32 _gr_evenOdd_xlate_table[];
extern FxU32 _gr_aspect_xlate_table[];

/*---------------------------------------------------------------------------
** grTexClampMode
*/

GR_ENTRY(grTexClampMode, void, ( GrChipID_t tmu, GrTextureClampMode_t s_clamp_mode, GrTextureClampMode_t t_clamp_mode ))
{
  FxU32 texturemode;
  FxU32 clampMode = (
    (s_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPS : 0) |
    (t_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPT : 0)
  );

  GR_BEGIN("grTexClampMode",88,4+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d, %d,%d)\n",tmu,s_clamp_mode,t_clamp_mode));
  GR_CHECK_TMU(myName, tmu);
  
  texturemode  = gc->state.tmu_config[tmu].textureMode;
  texturemode &= ~( SST_TCLAMPS | SST_TCLAMPT );
  texturemode |=  clampMode;

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,tmu)->textureMode , texturemode );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].textureMode = texturemode;
  GR_END();
} /* grTexClampMode */

/*---------------------------------------------------------------------------
** grTexCombine
*/

GR_ENTRY(grTexCombine, void, ( GrChipID_t tmu, GrCombineFunction_t rgb_function, GrCombineFactor_t rgb_factor,  GrCombineFunction_t alpha_function, GrCombineFactor_t alpha_factor, FxBool rgb_invert, FxBool alpha_invert ))
{
  FxU32 texturemode;
  FxU32 tLod;
  FxU32 tmuMask;

  GR_BEGIN("grTexCombine",88,8+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d, %d,%d, %d,%d, %d,%d)\n",
                 tmu, rgb_function, rgb_factor, 
                 alpha_function, alpha_factor,
                 rgb_invert, alpha_invert));
  GR_CHECK_TMU( myName, tmu );
  GR_CHECK_W( myName,
               rgb_function < GR_COMBINE_FUNCTION_ZERO ||
               rgb_function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA,
               "unsupported texture color combine function" );
  GR_CHECK_W( myName,
               alpha_function < GR_COMBINE_FUNCTION_ZERO ||
               alpha_function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA,
               "unsupported texture alpha combine function" );
  GR_CHECK_W( myName,
               (rgb_factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
               (rgb_factor & 0x7) > GR_COMBINE_FACTOR_LOD_FRACTION ||
               rgb_factor > GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
               "unsupported texture color combine scale factor" );
  GR_CHECK_W( myName,
               (alpha_factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
               (alpha_factor & 0x7) > GR_COMBINE_FACTOR_LOD_FRACTION ||
               alpha_factor > GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
               "unsupported texture alpha combine scale factor" );

  /* tmuMask tells grColorCombineFunction what to turn off an on if 
     the ccFunc requires texture mapping */
  texturemode = gc->state.tmu_config[tmu].textureMode;
  texturemode &= ~(SST_TCOMBINE | SST_TACOMBINE);
  tLod = gc->state.tmu_config[tmu].tLOD;
  tLod &= ~(SST_LOD_ODD);

  tmuMask = GR_TMUMASK_TMU0 << tmu;
  gc->state.tmuMask &= ~tmuMask;

  /* setup scale factor bits */
  texturemode |= ( rgb_factor & 0x7 ) << SST_TC_MSELECT_SHIFT;

  if ( !( rgb_factor & 0x8 ) )
    texturemode |= SST_TC_REVERSE_BLEND;

  if ( ( ( rgb_factor & 0x7 ) == GR_COMBINE_FACTOR_LOCAL ) ||
       ( ( rgb_factor & 0x7 ) == GR_COMBINE_FACTOR_LOCAL_ALPHA ) )
    gc->state.tmuMask |= tmuMask;

  texturemode |= ( alpha_factor & 0x7 ) << SST_TCA_MSELECT_SHIFT;

  if ( !( alpha_factor & 0x8 ) )
    texturemode |= SST_TCA_REVERSE_BLEND;

  if ( ( ( alpha_factor & 0x7 ) == GR_COMBINE_FACTOR_LOCAL ) ||
       ( ( alpha_factor & 0x7 ) == GR_COMBINE_FACTOR_LOCAL_ALPHA ) )
    gc->state.tmuMask |= tmuMask;

  /* setup invert output bits */

  if ( rgb_invert )
    texturemode |= SST_TC_INVERT_OUTPUT;

  if ( alpha_invert )
    texturemode |= SST_TCA_INVERT_OUTPUT;

  /* setup core color combine unit bits */
  
  switch ( rgb_function )
  {
  case GR_COMBINE_FUNCTION_ZERO:
    texturemode |= SST_TC_ZERO_OTHER;
    break;

  case GR_COMBINE_FUNCTION_LOCAL:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
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
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_SUB_CLOCAL | SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;
  }
  
  switch ( alpha_function )
  {
  case GR_COMBINE_FUNCTION_ZERO:
    texturemode |= SST_TCA_ZERO_OTHER;
    break;

  case GR_COMBINE_FUNCTION_LOCAL:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
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
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_SUB_CLOCAL | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;
  }
  
  /* Hack to enable TWO-PASS Trilinear 
     
   */
  if ( texturemode & SST_TRILINEAR ) {
    if ( ( texturemode & SST_TC_ZERO_OTHER ) &&
         ( texturemode & SST_TC_BLEND_LODFRAC ) &&
         !( texturemode & SST_TC_REVERSE_BLEND ) ) {
        tLod |= SST_LOD_ODD;
    }
  }
  tLod |= _gr_evenOdd_xlate_table[gc->state.tmu_config[tmu].evenOdd];

  /* update register */
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,tmu)->textureMode , texturemode );
  GR_SET( SST_TMU(hw,tmu)->tLOD, tLod );
  PACKER_WORKAROUND;
  gc->state.tmu_config[tmu].textureMode = texturemode;
  gc->state.tmu_config[tmu].tLOD = tLod;

  /* update paramIndex */
  _grUpdateParamIndex();

  GR_END();
} /* grTexCombine */

/*
** _grTexDetailControl, NOTE: its up to caller to account for bytes
*/
GR_DDFUNC(_grTexDetailControl, void, ( GrChipID_t tmu, FxU32 detail ))
{
  GR_BEGIN("_grTexDetailControl",88,4+2*PACKER_WORKAROUND_SIZE);

  GR_CHECK_TMU( "_grTexDetailControl", tmu );

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,tmu)->tDetail , detail );
  PACKER_WORKAROUND;
  gc->state.tmu_config[tmu].tDetail = detail;
  GR_END();
} /* _grTexDetailControl */

/*---------------------------------------------------------------------------
** grTexFilterMode
*/

GR_ENTRY(grTexFilterMode, void, ( GrChipID_t tmu, GrTextureFilterMode_t minfilter, GrTextureFilterMode_t magfilter ))
{
  FxU32 texMode;

  GR_BEGIN("grTexFilterMode",99,4+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d)\n",tmu,minfilter,magfilter));
  GR_CHECK_TMU( myName, tmu );

  texMode  = gc->state.tmu_config[tmu].textureMode;
  texMode &= ~( SST_TMINFILTER | SST_TMAGFILTER );
  texMode |= (minfilter == GR_TEXTUREFILTER_BILINEAR ? SST_TMINFILTER : 0) |
             (magfilter == GR_TEXTUREFILTER_BILINEAR ? SST_TMAGFILTER : 0);

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,tmu)->textureMode , texMode );
  PACKER_WORKAROUND;
  gc->state.tmu_config[tmu].textureMode = texMode;
  GR_END();
} /* grTexFilterMode */

/*---------------------------------------------------------------------------
** grTexLodBiasValue
*/

GR_ENTRY(grTexLodBiasValue, void, ( GrChipID_t tmu, float fvalue ))
{
  FxU32 tLod;
  
  GR_BEGIN("grTexLodBiasValue",88,4+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%g)\n",tmu,fvalue));
  GR_CHECK_TMU(myName,tmu);
  
  tLod = gc->state.tmu_config[tmu].tLOD;
  tLod &= ~( SST_LODBIAS );
  tLod |= _grTexFloatLODToFixedLOD( fvalue ) << SST_LODBIAS_SHIFT;

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,tmu)->tLOD , tLod );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].tLOD = tLod;
  GR_END();
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

GR_ENTRY(grTexMipMapMode, void, ( GrChipID_t tmu, GrMipMapMode_t mmMode, FxBool lodBlend ))
{
  FxU32
    tLod,
    texMode;

  GR_BEGIN("grTexMipMapMode",88,8+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d)\n",tmu,mmMode,lodBlend));
  GR_CHECK_TMU(myName,tmu);
  
  /*--------------------------------------------------------------
    Get Current tLod and texMode register values
    --------------------------------------------------------------*/
  tLod  = gc->state.tmu_config[tmu].tLOD;
  texMode = gc->state.tmu_config[tmu].textureMode;

  /*--------------------------------------------------------------
    Clear LODMIN, LODMAX and LODDITHER
    --------------------------------------------------------------*/
  tLod  &= ~(SST_LODMIN|SST_LODMAX|SST_LOD_ODD);
  texMode &= ~(SST_TLODDITHER|SST_TRILINEAR);

  /*--------------------------------------------------------------
    Encode Mipmap Mode Bits
    --------------------------------------------------------------*/
  switch ( mmMode ) {
  case GR_MIPMAP_DISABLE:
    /*----------------------------------------------------------
      To disable mipmapping set the min and max lods to the same
      value
      ----------------------------------------------------------*/
    tLod |= SST_TLOD_MINMAX_INT(gc->state.tmu_config[tmu].largeLod,
                                gc->state.tmu_config[tmu].largeLod);
    break;
  case GR_MIPMAP_NEAREST_DITHER:
    if (gc->state.allowLODdither)
      texMode |= SST_TLODDITHER;
    /* intentional fall-through to set lodmin and lodmax values */
  case GR_MIPMAP_NEAREST:
    /*----------------------------------------------------------
      Set LODMIN and LODMAX in the tLod register to the 
      actual min and max LODs of the current texture.
      ----------------------------------------------------------*/
    tLod |= SST_TLOD_MINMAX_INT(gc->state.tmu_config[tmu].largeLod,
                                gc->state.tmu_config[tmu].smallLod);
    break;
  default:
    GrErrorCallback( "grTexMipMapMode:  invalid mode passed", FXFALSE );
    break;
  }
  gc->state.tmu_config[tmu].mmMode = mmMode;
  
  /*--------------------------------------------------------------
    Fix trilinear and evenOdd bits -

    This is a bit of a hack to make two pass trilinear work with
    full textures.  The assumption here is that the only reason
    you would ever set up Multiply by LODFRAC w/o REVERSE BLEND
    is for the ODD pass of trilinear.  
    --------------------------------------------------------------*/
  if ( lodBlend ) {
    texMode |= SST_TRILINEAR;
    if ( ( texMode & SST_TC_ZERO_OTHER ) &&
         ( texMode & SST_TC_BLEND_LODFRAC ) &&
         !( texMode & SST_TC_REVERSE_BLEND ) ) {
        tLod |= SST_LOD_ODD;
    }
  }
  tLod |= _gr_evenOdd_xlate_table[gc->state.tmu_config[tmu].evenOdd];
  
  /*--------------------------------------------------------------
    Write State To Hardware and Update Glide Shadow State
    --------------------------------------------------------------*/
  PACKER_WORKAROUND;
  hw = SST_TMU(hw,tmu);
  GR_SET( hw->tLOD , tLod );
  GR_SET( hw->textureMode , texMode );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].tLOD        = tLod;
  gc->state.tmu_config[tmu].textureMode = texMode;
  GR_END();
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

GR_ENTRY(grTexNCCTable, void, ( GrChipID_t tmu, GrNCCTable_t table ))
{
  FxU32 texMode;
  
  GR_BEGIN("grTexNCCTable",88,4+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",tmu));
  GR_CHECK_TMU(myName,tmu);
  GR_CHECK_F(myName, table>0x1, "invalid ncc table specified");

  /*------------------------------------------------------------------
    Update local state
    ------------------------------------------------------------------*/
  gc->state.tmu_config[tmu].nccTable = table;
  
  /*------------------------------------------------------------------
    Grab shadow texMode, update TexMode, update shadow/real register
    ------------------------------------------------------------------*/
  texMode  = gc->state.tmu_config[tmu].textureMode;
  texMode &= ~( SST_TNCCSELECT );
  if ( table )
    texMode |= SST_TNCCSELECT;
  else 
    texMode &= ~(SST_TNCCSELECT);

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,tmu)->textureMode , texMode );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].textureMode = texMode;
  GR_END();
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

GR_ENTRY(grTexSource, void, ( GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info ))
{
  FxU32 baseAddress, texMode, tLod;

  GR_BEGIN("grTexSource",88,12+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x,%d,0x%x)\n",tmu,startAddress,evenOdd,info));
  GR_CHECK_TMU( myName, tmu );
  GR_CHECK_F( myName, startAddress >= gc->tmu_state[tmu].total_mem, "invalid startAddress" );
  GR_CHECK_F( myName,
               startAddress + grTexTextureMemRequired( evenOdd, info ) >= gc->tmu_state[tmu].total_mem,
               "insufficient texture ram at startAddress" );
  GR_CHECK_F( myName, evenOdd > 0x3 || evenOdd == 0, "evenOdd mask invalid");
  GR_CHECK_F( myName, !info, "invalid info pointer" );
  
  /*-------------------------------------------------------------
    Update Texture Unit State
    -------------------------------------------------------------*/
  gc->state.tmu_config[tmu].smallLod = info->smallLod; 
  gc->state.tmu_config[tmu].largeLod = info->largeLod; 
  gc->state.tmu_config[tmu].evenOdd  =  evenOdd; 
  
  /*-------------------------------------------------------------
    Calculate Base Address
    -------------------------------------------------------------*/
  baseAddress = _grTexCalcBaseAddress( startAddress,
                                       info->largeLod, 
                                       info->aspectRatio,
                                       info->format,
                                       evenOdd ) >> 3;
  /*-------------------------------------------------------------
    Update Texture Mode
    -------------------------------------------------------------*/
  texMode = gc->state.tmu_config[tmu].textureMode;
  texMode &= ~SST_TFORMAT;
  texMode |= ( info->format << SST_TFORMAT_SHIFT ) | SST_TPERSP_ST | SST_TCLAMPW;
  
  /*-------------------------------------------------------------
    Compute TLOD (keep LODBIAS in tact)
    -------------------------------------------------------------*/
  tLod = gc->state.tmu_config[tmu].tLOD;
  tLod &= ~(SST_LODMIN | SST_LODMAX | SST_LOD_ASPECT |
            SST_LOD_TSPLIT | SST_LOD_ODD | SST_LOD_S_IS_WIDER);
  tLod |= SST_TLOD_MINMAX_INT(info->largeLod,
                     gc->state.tmu_config[tmu].mmMode==GR_MIPMAP_DISABLE ? 
                              info->largeLod : info->smallLod);
  tLod |= _gr_evenOdd_xlate_table[evenOdd];
  tLod |= _gr_aspect_xlate_table[info->aspectRatio];

  /* Write relevant registers out to hardware */
  PACKER_WORKAROUND;
  hw = SST_TMU(hw,tmu);
  GR_SET( hw->texBaseAddr , baseAddress );
  GR_SET( hw->textureMode , texMode );
  GR_SET( hw->tLOD , tLod );
  PACKER_WORKAROUND;
  
  /* update shadows */
  gc->state.tmu_config[tmu].texBaseAddr = baseAddress; 
  gc->state.tmu_config[tmu].textureMode = texMode; 
  gc->state.tmu_config[tmu].tLOD        = tLod; 
  
  GR_END();
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

GR_ENTRY(grTexMultibase, void, ( GrChipID_t tmu, FxBool enable ))
{
  FxU32 tLod;
    
  GR_BEGIN("grTexMultibase",88,4+PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d)\n",tmu,enable));
  GR_CHECK_TMU(myName,tmu);
  
  tLod  = gc->state.tmu_config[tmu].tLOD;
  if ( enable )
    tLod |= SST_TMULTIBASEADDR;
  else
    tLod &= ~SST_TMULTIBASEADDR;
  /*--------------------------------------------------------------
    Write State To Hardware and Update Glide Shadow State
    --------------------------------------------------------------*/
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,tmu)->tLOD , tLod );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].tLOD = tLod;
  GR_END();
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

GR_ENTRY(grTexMultibaseAddress, void, ( GrChipID_t tmu, GrTexBaseRange_t range, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info ))
{
  FxU32 baseAddress;

  GR_BEGIN("grTexMultibaseAddress",88,4+PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,0x%x)\n",tmu,range,startAddress));
  GR_CHECK_TMU( myName, tmu );
  GR_CHECK_F( myName, range > GR_TEXBASE_32_TO_1, "invalid range" );
  GR_CHECK_F( myName, startAddress >= gc->tmu_state[tmu].total_mem, "invalid startAddress" );
  GR_CHECK_F( myName, evenOdd > 0x3, "evenOdd mask invalid" );
  GR_CHECK_F( myName, info, "invalid info pointer" );
  

  /* Write relevant registers out to hardware and shadows */
  PACKER_WORKAROUND;
  hw = SST_TMU(hw,tmu);
  switch (range) {
    case GR_TEXBASE_256:
      baseAddress = _grTexCalcBaseAddress( startAddress,
                                           GR_LOD_256,
                                           info->aspectRatio,
                                           info->format,
                                           evenOdd ) >> 3;
      GR_SET( hw->texBaseAddr , baseAddress );
      gc->state.tmu_config[tmu].texBaseAddr = baseAddress; 
      break;
    case GR_TEXBASE_128:
      baseAddress = _grTexCalcBaseAddress( startAddress,
                                           GR_LOD_128,
                                           info->aspectRatio,
                                           info->format,
                                           evenOdd ) >> 3;
      GR_SET( hw->texBaseAddr1 , baseAddress );
      gc->state.tmu_config[tmu].texBaseAddr_1 = baseAddress; 
      break;
    case GR_TEXBASE_64:
      baseAddress = _grTexCalcBaseAddress( startAddress,
                                           GR_LOD_64,
                                           info->aspectRatio,
                                           info->format,
                                           evenOdd ) >> 3;
      GR_SET( hw->texBaseAddr2 , baseAddress );
      gc->state.tmu_config[tmu].texBaseAddr_2 = baseAddress; 
      break;
    case GR_TEXBASE_32_TO_1:
      baseAddress = _grTexCalcBaseAddress( startAddress,
                                           GR_LOD_32,
                                           info->aspectRatio,
                                           info->format,
                                           evenOdd ) >> 3;
      GR_SET( hw->texBaseAddr38 , baseAddress );
      gc->state.tmu_config[tmu].texBaseAddr_3_8 = baseAddress; 
      break;
  }
  PACKER_WORKAROUND;
  GR_END();
} /* grTexMultibaseAddress */

#if 0
/*
** _grTexForceLod
**
** Forces tLOD to a specific LOD level.  This is useful only for
** debugging purposes.  GMT: obsolete, please remove
*/
void 
_grTexForceLod( GrChipID_t tmu, int value )
{
  GR_DCL_GC;
  GR_DCL_HW;
  FxU32 tLod = gc->state.tmu_config[0].tLOD;

  GR_CHECK_TMU("_grTexForceLod",tmu);

  tLod &= ~(SST_LODMIN | SST_LODMAX);
  tLod |= SST_TLOD_MINMAX_INT(value,value);

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,tmu)->tLOD , tLod );
  PACKER_WORKAROUND;
  gc->state.tmu_config[tmu].tLOD = tLod;
} /* _grTexForceLod */
#endif
