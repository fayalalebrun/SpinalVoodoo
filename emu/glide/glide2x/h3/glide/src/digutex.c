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
** 
** 11    2/16/99 4:14p Peter
** made texture table declarations consistent
 * 
 * 9     1/08/98 7:09p Peter
 * real hw stuff modulo makefile change
 * 
 * 8     1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 7     12/15/97 5:51p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 6     5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 5     5/21/97 6:04a Peter
 * 
 * 4     5/05/97 4:24p Pgj
 * Remove guTexDownloadMipMap error message
 * 
 * 3     3/18/97 9:07p Dow
 * Got rid of #$#%#$ // comments
 * 
 * 2     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 1     12/23/96 1:39p Dow
 * Changes for multiplatform
**
*/

#include <string.h>
#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#ifndef GLIDE3_ALPHA
/*---------------------------------------------------------------------------
** guTexAllocateMemory
*/
GR_DIENTRY(guTexAllocateMemory, GrMipMapId_t, ( GrChipID_t tmu,
                    FxU8 odd_even_mask,
                    int width, int height,
                    GrTextureFormat_t format,
                    GrMipMapMode_t mipmap_mode,
                    GrLOD_t small_lod, GrLOD_t large_lod,
                    GrAspectRatio_t aspect_ratio,
                    GrTextureClampMode_t s_clamp_mode,
                    GrTextureClampMode_t t_clamp_mode,
                    GrTextureFilterMode_t minfilter_mode,
                    GrTextureFilterMode_t magfilter_mode,
                    float lod_bias,
                    FxBool trilinear
                    ))
{
  FxU32
    memrequired,
    memavail,
    baseAddress,
    tLod,
    texturemode,
    filterMode,                 /* filter mode bits */
    clampMode;                  /* clamp mode bits */

  GrMipMapId_t
    mmid = (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
  int
    int_lod_bias;
  GrTexInfo info;

  GR_BEGIN_NOFIFOCHECK("guTexAllocateMemory",99);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d, %d,%d, %d,%d, %d,%d,%d, %d,%d, %d,%d)\n",
                tmu,odd_even_mask,width,height,format,mipmap_mode,
                small_lod,large_lod,aspect_ratio,
                s_clamp_mode,t_clamp_mode, minfilter_mode,magfilter_mode);
  /*
   ** The constants are actually reverse of each other so the following
   ** test IS valid!
   */
  GR_CHECK_F(myName, small_lod < large_lod, "smallest_lod is larger than large_lod");
  
  info.smallLod = small_lod;
  info.largeLod = large_lod;
  info.aspectRatio = aspect_ratio;
  info.format = format;
  memrequired = grTexTextureMemRequired(odd_even_mask, &info);

  /*
   ** Make sure to not cross 2 MByte texture boundry
   */
  if ((gc->tmu_state[tmu].freemem_base < 0x200000) &&
    (gc->tmu_state[tmu].freemem_base + memrequired > 0x200000))
    gc->tmu_state[tmu].freemem_base = 0x200000;
  
  /*
   ** If we have enough memory and a free mip map handle then go for it
   */
  memavail = guTexMemQueryAvail( tmu );
  
  if ( memavail < memrequired )
    return (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
  
  if (gc->mm_table.free_mmid >= MAX_MIPMAPS_PER_SST )
    return (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
  
  /*
   ** Allocate the mip map id
   */
  mmid = gc->mm_table.free_mmid++;

  /*
   ** calculate baseAddress (where LOD 0 would go)
   */
  baseAddress = _grTexCalcBaseAddress( gc->tmu_state[tmu].freemem_base, 
                                       large_lod, 
                                       aspect_ratio, 
                                       format, 
                                       odd_even_mask );

  GDBG_INFO(gc->myLevel,"  baseAddress = 0x%x (in bytes)\n",baseAddress);
  
  /*
   ** reduce available memory to reflect allocation
   */
  gc->tmu_state[tmu].freemem_base += memrequired;

  /*
   ** Create the tLOD register value for this mip map
   */
  int_lod_bias = _grTexFloatLODToFixedLOD( lod_bias );
  tLod = mipmap_mode==GR_MIPMAP_DISABLE ? large_lod : small_lod;
  tLod = SST_TLOD_MINMAX_INT(large_lod,tLod);
  tLod |= _gr_evenOdd_xlate_table[odd_even_mask];
  tLod |= _gr_aspect_xlate_table[aspect_ratio];
  tLod |= int_lod_bias << SST_LODBIAS_SHIFT;
  filterMode = (
                (minfilter_mode == GR_TEXTUREFILTER_BILINEAR ? SST_TMINFILTER : 0) |
                (magfilter_mode == GR_TEXTUREFILTER_BILINEAR ? SST_TMAGFILTER : 0)
                );
  
  clampMode = (
               (s_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPS : 0) |
               (t_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPT : 0)
               );  
  
  /*
   ** Create the tTextureMode register value for this mip map
   */
  texturemode  = ( format << SST_TFORMAT_SHIFT );
  texturemode |= SST_TCLAMPW;
  texturemode |= SST_TPERSP_ST;
  texturemode |= filterMode;
  texturemode |= clampMode;
  
  if ( mipmap_mode == GR_MIPMAP_NEAREST_DITHER )
    texturemode |= SST_TLODDITHER;
  
  if ( trilinear ) {
      texturemode |= SST_TRILINEAR;

      if ( odd_even_mask & GR_MIPMAPLEVELMASK_ODD )
        tLod |= SST_LOD_ODD;

      if ( odd_even_mask != GR_MIPMAPLEVELMASK_BOTH )
        tLod |= SST_LOD_TSPLIT;
  }
  
  /*
   ** Fill in the mm_table data for this mip map
   */
  gc->mm_table.data[mmid].format         = format;
  gc->mm_table.data[mmid].mipmap_mode    = mipmap_mode;
  gc->mm_table.data[mmid].magfilter_mode = magfilter_mode;
  gc->mm_table.data[mmid].minfilter_mode = minfilter_mode;
  gc->mm_table.data[mmid].s_clamp_mode   = s_clamp_mode;
  gc->mm_table.data[mmid].t_clamp_mode   = t_clamp_mode;
  gc->mm_table.data[mmid].tLOD           = tLod;
  gc->mm_table.data[mmid].tTextureMode   = texturemode;
  gc->mm_table.data[mmid].lod_bias       = int_lod_bias;
  gc->mm_table.data[mmid].lod_min        = small_lod;
  gc->mm_table.data[mmid].lod_max        = large_lod;
  gc->mm_table.data[mmid].tmu            = tmu;
  gc->mm_table.data[mmid].odd_even_mask  = odd_even_mask;
  gc->mm_table.data[mmid].tmu_base_address = baseAddress;
  gc->mm_table.data[mmid].trilinear      = trilinear;
  gc->mm_table.data[mmid].aspect_ratio   = aspect_ratio;
  gc->mm_table.data[mmid].data           = 0;
  /*   gc->mm_table.data[mmid].ncc_table      = 0; */
  gc->mm_table.data[mmid].sst            = _GlideRoot.current_sst;
  gc->mm_table.data[mmid].valid          = FXTRUE;
  gc->mm_table.data[mmid].width          = width;
  gc->mm_table.data[mmid].height         = height;
  
  GR_RETURN(mmid);
} /* guTexAllocateMemory */

static void
_guTexRebuildRegisterShadows( GrMipMapId_t mmid )
{
  GR_DCL_GC;
  GrMipMapInfo *mminfo = &gc->mm_table.data[mmid];
  int texturemode      = 0;
  int tLod             = 0;
  FxU32 
    filterMode,                 /* filter mode bits of texturemode */
    clampMode;                  /* clamp mode bits of texturemode */

  /* build filterMode */
  filterMode = (
                (mminfo->minfilter_mode == GR_TEXTUREFILTER_BILINEAR ? SST_TMINFILTER : 0) |
                (mminfo->magfilter_mode == GR_TEXTUREFILTER_BILINEAR ? SST_TMAGFILTER : 0)
                );
  clampMode = (
               (mminfo->s_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPS : 0) |
               (mminfo->t_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPT : 0)
               );
   
  /*
   ** build up tTextureMode
   */
  texturemode |= ( mminfo->format << SST_TFORMAT_SHIFT );
  texturemode |= SST_TCLAMPW;
  texturemode |= SST_TPERSP_ST;
  texturemode |= filterMode;
  texturemode |= clampMode;

  if ( mminfo->mipmap_mode == GR_MIPMAP_NEAREST_DITHER )
    texturemode |= SST_TLODDITHER;

  if ( mminfo->trilinear )
    texturemode |= SST_TRILINEAR;

  /*
   ** build up tLOD
   */
  tLod = mminfo->mipmap_mode == GR_MIPMAP_DISABLE ? mminfo->lod_max : mminfo->lod_min;
  tLod = SST_TLOD_MINMAX_INT(mminfo->lod_max,tLod);
  tLod |= _gr_evenOdd_xlate_table[mminfo->odd_even_mask];
  tLod |= _gr_aspect_xlate_table[mminfo->aspect_ratio];
  tLod |= mminfo->lod_bias << SST_LODBIAS_SHIFT;

  /*
   ** assign them
   */
  mminfo->tTextureMode = texturemode;
  mminfo->tLOD         = tLod;
} /* guTexRebuildRegisterShadows */


/*---------------------------------------------------------------------------
**  guTexChangeAttributes 
*/
GR_DIENTRY(guTexChangeAttributes, FxBool, ( GrMipMapId_t mmid,
                      int width, int height,
                      GrTextureFormat_t fmt,
                      GrMipMapMode_t mm_mode,
                      GrLOD_t smallest_lod, GrLOD_t largest_lod,
                      GrAspectRatio_t aspect,
                      GrTextureClampMode_t s_clamp_mode,
                      GrTextureClampMode_t t_clamp_mode,
                      GrTextureFilterMode_t minFilterMode,
                      GrTextureFilterMode_t magFilterMode
                      ))
{
  GrMipMapInfo *mminfo;

  GR_BEGIN_NOFIFOCHECK("guTexChangeAttributes",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d, %d,%d, %d,%d, %d,%d,%d, %d,%d, %d,%d)\n",
                mmid,width,height,fmt,mm_mode,
                smallest_lod,largest_lod,aspect,
                s_clamp_mode,t_clamp_mode, minFilterMode,magFilterMode);
  /*
  ** Make sure that mmid is not NULL
  */
  if ( mmid == GR_NULL_MIPMAP_HANDLE ) {
    GR_RETURN(FXFALSE);
  }

  mminfo = &gc->mm_table.data[mmid];

  /*
  ** Fill in the mm_table data for this mip map
  */
  if ( fmt != -1 )
     mminfo->format         = fmt;

  if ( mm_mode != -1 )
     mminfo->mipmap_mode    = mm_mode;

  if ( smallest_lod != -1 )
     mminfo->lod_min        = smallest_lod;
  if ( largest_lod != -1 )
     mminfo->lod_max        = largest_lod;
  if ( minFilterMode != -1 )
     mminfo->minfilter_mode = minFilterMode;
  if ( magFilterMode != -1 )
     mminfo->magfilter_mode = magFilterMode;
  if ( s_clamp_mode != -1 )
     mminfo->s_clamp_mode   = s_clamp_mode;
  if ( t_clamp_mode != -1 )
     mminfo->t_clamp_mode   = t_clamp_mode;
  if ( aspect != -1 )
     mminfo->aspect_ratio   = aspect;
  if ( width != -1 )
     mminfo->width          = width;
  if ( height != -1 )
     mminfo->height         = height;

  _guTexRebuildRegisterShadows( mmid );
  GR_RETURN(FXTRUE);
} /* guTexChangeAttributes */

/*---------------------------------------------------------------------------
** grTexCombineFunction - obsolete
**                              
*/
GR_DIENTRY(grTexCombineFunction, void,
           (GrChipID_t tmu, GrTextureCombineFnc_t tc)) 
{
  guTexCombineFunction( tmu, tc );
}

/*---------------------------------------------------------------------------
** guTexCombineFunction
**                              
** Sets the texture combine function.  For a dual TMU system this function
** will configure the TEXTUREMODE registers as appropriate.  For a
** single TMU system this function will configure TEXTUREMODE if
** possible, or defer operations until grDrawTriangle() is called.
*/
GR_DIENTRY(guTexCombineFunction, void,
           (GrChipID_t tmu, GrTextureCombineFnc_t tc))
{
  GDBG_INFO(99,"guTexCombineFunction(%d,%d)\n",tmu,tc);
  switch ( tc )  {
  case GR_TEXTURECOMBINE_ZERO:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_DECAL:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_ONE:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE, FXTRUE, FXTRUE );
    break;

  case GR_TEXTURECOMBINE_ADD:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL, GR_COMBINE_FACTOR_ONE,
                  GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL, GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_MULTIPLY:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL,
                  GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_DETAIL:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR,
                  GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_DETAIL_OTHER:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_DETAIL_FACTOR,
                  GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_DETAIL_FACTOR, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_TRILINEAR_ODD:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
                  GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_TRILINEAR_EVEN:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_LOD_FRACTION,
                  GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_LOD_FRACTION, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_SUBTRACT:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL, GR_COMBINE_FACTOR_ONE,
                  GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL, GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_OTHER:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
                  GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE );
    break;

  default:
    GrErrorCallback( "guTexCombineFunction:  Unsupported function", FXTRUE );
    break;
  }
} /* guTexCombineFunction */

/*---------------------------------------------------------------------------
** guTexDownloadMipMap
**
** Downloads a mip map (previously allocated with guTexAllocateMemory) to
** the hardware using the given data and ncctble.  The "data" is assumed
** to be in row major order from largest mip map to smallest mip map.
*/
GR_DIENTRY(guTexDownloadMipMap, void, 
           (GrMipMapId_t mmid, const void *src, const GuNccTable
            *ncc_table ) )
{
  GR_DCL_GC;
  GrLOD_t     lod;
  const void *ptr = src;

  GDBG_INFO(99,"guTexDownloadMipMap(%d,0x%x,0x%x)\n",mmid,src,ncc_table);
  GR_ASSERT(gc != NULL);
  GR_ASSERT(src != NULL);
  GR_CHECK_F("guTexDownloadMipMap",
              ( mmid == GR_NULL_MIPMAP_HANDLE ) || ( mmid >= gc->mm_table.free_mmid ),
              "invalid mip map handle passed");

#if 0 /* Fixme!!! XXX ??? */
  GR_CHECK_F("guTexDownloadMipMap",
              gc->mm_table.data[mmid].format == GR_TEXFMT_P_8,
              "guTex* does not support palletted textures - use grTex* instead");
#endif /* 0 */

  /*
  ** Bind data and ncc table to this mip map
  */
  gc->mm_table.data[mmid].data      = (void *) ptr;
  if (gc->mm_table.data[mmid].format == GR_TEXFMT_YIQ_422)
    gc->mm_table.data[mmid].ncc_table = *ncc_table;

  /*
  ** Start downloading mip map levels, note that ptr is updated by the caller
  */
  for ( lod = gc->mm_table.data[mmid].lod_max; lod <= gc->mm_table.data[mmid].lod_min; lod++ ) {
     guTexDownloadMipMapLevel( mmid, lod, &ptr );
  }
} /* guTexDownloadMipMap */

/*---------------------------------------------------------------------------
** guTexDownloadMipMapLevel
**
** Downloads a single mip map level to a mip map.  "src" is considered to be
** row major data of the correct aspect ratio and format.
*/
GR_DIENTRY(guTexDownloadMipMapLevel, void,
           (GrMipMapId_t mmid, GrLOD_t lod,
            const void **src_base))
{
  FxU32 i;
  const  GrMipMapInfo *mminfo;
  GR_DCL_GC;
 
  GDBG_INFO(99,"guTexDownloadMipMapLevel(%d,%d,0x%x)\n",mmid,lod,src_base);
  GR_ASSERT(src_base != NULL);
  mminfo = &gc->mm_table.data[mmid];
  GR_CHECK_F( "guTexDownloadMipMapLevel",
              ( lod > mminfo->lod_min ) || ( lod < mminfo->lod_max ),
              "specified lod is out of range");

  /* GMT: replace with array access */
  /* download this level */
  i = _grTexCalcBaseAddress( 0,
                            mminfo->lod_max,
                            mminfo->aspect_ratio,
                            mminfo->format,
                            mminfo->odd_even_mask);
  grTexDownloadMipMapLevel( mminfo->tmu,
                            mminfo->tmu_base_address - i,
                            lod,
                            mminfo->lod_max,
                            mminfo->aspect_ratio,
                            mminfo->format,
                            mminfo->odd_even_mask,
                            (void *)*src_base );

  /* update src_base to point to next mipmap level */
  *src_base = (void *) (((FxU32)*src_base) +
               (_grMipMapHostSize[_gr_aspect_index_table[mminfo->aspect_ratio]][lod]
                    << (mminfo->format>=GR_TEXFMT_16BIT)));

} /* guTexDownloadMipmapLevel */

/*---------------------------------------------------------------------------
** guTexGetCurrentMipMap
*/
GR_DIENTRY(guTexGetCurrentMipMap, GrMipMapId_t, ( GrChipID_t tmu ))
{
  GR_BEGIN_NOFIFOCHECK("guTexGetCurrentMipMap",99);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",tmu);
  GR_CHECK_TMU( myName, tmu );

  GR_RETURN(gc->state.current_mm[tmu]);
} /* guTexGetCurrentMipMap */

/*---------------------------------------------------------------------------
** guTexGetMipMapInfo
*/
GR_DIENTRY(guTexGetMipMapInfo, GrMipMapInfo *, ( GrMipMapId_t mmid ))
{
  GR_BEGIN_NOFIFOCHECK("guTexGetMipMapInfo",99);
  GDBG_INFO_MORE(gc->myLevel,"(%d) => 0x%x\n",mmid,&gc->mm_table.data[mmid]);
  return &( gc->mm_table.data[mmid] );
} /* guTexGetMipMapInfo */

/*---------------------------------------------------------------------------
** guTexMemQueryAvail
**
** returns the amount of available texture memory on a specified TMU.
*/
GR_DIENTRY(guTexMemQueryAvail, FxU32, ( GrChipID_t tmu ))
{
  GR_BEGIN_NOFIFOCHECK("guTexMemQueryAvail",99);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",tmu);
  GR_CHECK_TMU( myName, tmu );
  GR_RETURN(gc->tmu_state[tmu].total_mem - gc->tmu_state[tmu].freemem_base);
} /* guTexQueryMemAvail */

/*---------------------------------------------------------------------------
** guTexMemReset
**
** Clears out texture buffer memory.
*/
GR_DIENTRY(guTexMemReset, void, ( void ))
{
  int i;
  
  GR_BEGIN_NOFIFOCHECK("guTexMemReset",99);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

  memset( gc->mm_table.data, 0, sizeof( gc->mm_table.data ) );
  gc->mm_table.free_mmid = 0;
  
  for ( i = 0; i < gc->num_tmu; i++ ) {
    gc->state.current_mm[i] = (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
    gc->tmu_state[i].freemem_base = 0;
    gc->tmu_state[i].ncc_mmids[0] = 
      gc->tmu_state[i].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;    
    gc->tmu_state[i].ncc_table[0] = 
      gc->tmu_state[i].ncc_table[1] = 0;
  }
  GR_END();
} /* guTexMemReset */

#endif /* !GLIDE3_ALPHA */

#if defined(GLIDE3)
/*---------------------------------------------------------------------------
** guTexMemReset
**
** Clears out texture buffer memory.
*/
void FX_CSTYLE
_guTexMemReset(void)
{
  int i;
  
  GR_BEGIN_NOFIFOCHECK("guTexMemReset",99);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

#ifndef GLIDE3_ALPHA
  memset( gc->mm_table.data, 0, sizeof( gc->mm_table.data ) );
  gc->mm_table.free_mmid = 0;
#endif /* !GLIDE3_ALPHA */
  
  for ( i = 0; i < gc->num_tmu; i++ ) {
    gc->state.current_mm[i] = (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
    gc->tmu_state[i].freemem_base = 0;
    gc->tmu_state[i].ncc_mmids[0] = 
      gc->tmu_state[i].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;    
    gc->tmu_state[i].ncc_table[0] = 
      gc->tmu_state[i].ncc_table[1] = 0;
  }
  GR_END();
} /* guTexMemReset */
#endif /* defined(GLIDE3) */
