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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <3dfx.h>
#include <glide.h>
#include "texusint.h"

/*
 * Allocate memory via Glide for a texture.
 * Assumes that the passed image is already quantized to 
 * the format that you want to download.
 */
GrMipMapId_t txAllocateTexMemory( GrChipID_t tmu,
                                  FxU8 odd_even_mask,
                                  TxMip *texture,
                                  GrMipMapMode_t mm_mode,
                                  GrTextureClampMode_t s_clamp_mode,
                                  GrTextureClampMode_t t_clamp_mode,
                                  GrTextureFilterMode_t minfilter_mode,
                                  GrTextureFilterMode_t magfilter_mode,
                                  float lod_bias,
                                  FxBool trilinear )
{
  int aspect;
  GrAspectRatio_t gr_aspect;
  int longest_side;
  GrLOD_t max_lod, min_lod;
  int num_mipmap_levels = 0;
  int i;

  aspect = ( 8 * texture->width ) / texture->height;

#ifdef GLIDE3
  switch( aspect )
    {
    case 64:
      gr_aspect = GR_ASPECT_LOG2_8x1;
      longest_side = texture->width;
      break;
    case 32:
      gr_aspect = GR_ASPECT_LOG2_4x1;
      longest_side = texture->width;
      break;
    case 16:
      gr_aspect = GR_ASPECT_LOG2_2x1;
      longest_side = texture->width;
      break;
    case 8:
      gr_aspect = GR_ASPECT_LOG2_1x1;
      longest_side = texture->width;
      break;
    case 4:
      gr_aspect = GR_ASPECT_LOG2_1x2;
      longest_side = texture->height;
      break;
    case 2:
      gr_aspect = GR_ASPECT_LOG2_1x4;
      longest_side = texture->height;
      break;
    case 1:
      gr_aspect = GR_ASPECT_LOG2_1x8;
      longest_side = texture->height;
      break;
    }
#else
  switch( aspect )
    {
    case 64:
      gr_aspect = GR_ASPECT_LOG2_8x1;
      longest_side = texture->width;
      break;
    case 32:
      gr_aspect = GR_ASPECT_LOG2_4x1;
      longest_side = texture->width;
      break;
    case 16:
      gr_aspect = GR_ASPECT_LOG2_2x1;
      longest_side = texture->width;
      break;
    case 8:
      gr_aspect = GR_ASPECT_LOG2_1x1;
      longest_side = texture->width;
      break;
    case 4:
      gr_aspect = GR_ASPECT_LOG2_1x2;
      longest_side = texture->height;
      break;
    case 2:
      gr_aspect = GR_ASPECT_LOG2_1x4;
      longest_side = texture->height;
      break;
    case 1:
      gr_aspect = GR_ASPECT_LOG2_1x8;
      longest_side = texture->height;
      break;
    }
#endif /* GLIDE3 */

  /*
   * How many mipmap levels are there?
   */
  for( i = 0; i < 16; i++ )
    {
      if( texture->data[i] )
        num_mipmap_levels++;
      else
        break;
    }

  /*
   * Figure out the max lod.
   */
#ifdef GLIDE3
  switch( longest_side )
    {
    case 256:
      max_lod = GR_LOD_LOG2_256;
      break;
    case 128:
      max_lod = GR_LOD_LOG2_128;
      break;
    case 64:
      max_lod = GR_LOD_LOG2_64;
      break;
    case 32:
      max_lod = GR_LOD_LOG2_32;
      break;
    case 16:
      max_lod = GR_LOD_LOG2_16;
      break;
    case 8:
      max_lod = GR_LOD_LOG2_8;
      break;
    case 4:
      max_lod = GR_LOD_LOG2_4;
      break;
    case 2:
      max_lod = GR_LOD_LOG2_2;
      break;
    case 1:
      max_lod = GR_LOD_LOG2_1;
      break;
    }
#else
  switch( longest_side )
    {
    case 256:
      max_lod = GR_LOD_256;
      break;
    case 128:
      max_lod = GR_LOD_128;
      break;
    case 64:
      max_lod = GR_LOD_64;
      break;
    case 32:
      max_lod = GR_LOD_32;
      break;
    case 16:
      max_lod = GR_LOD_16;
      break;
    case 8:
      max_lod = GR_LOD_8;
      break;
    case 4:
      max_lod = GR_LOD_4;
      break;
    case 2:
      max_lod = GR_LOD_2;
      break;
    case 1:
      max_lod = GR_LOD_1;
      break;
    }
#endif /* GLIDE3 */

  /*
   * Figure out the minimum LOD.
   */
#ifdef GLIDE3
  min_lod = max_lod - ( num_mipmap_levels - 1 );
#else
  min_lod = max_lod + num_mipmap_levels - 1;
#endif

  return grTexAllocateMemory( tmu, odd_even_mask,
#if 0
                              texture->width, texture->height,
#else
                              0, 0,
#endif
                              texture->format, mm_mode,
                              min_lod, max_lod,
                              gr_aspect,
                              s_clamp_mode, t_clamp_mode,
                              minfilter_mode, magfilter_mode,
                              lod_bias,
                              trilinear );
}

void txDownloadMipmap( GrMipMapId_t mmid, TxMip *texture )
{
  int longest_side;
  GrLOD_t lod;
  int i;

  if( texture->width > texture->height )
    longest_side = texture->width;
  else
    longest_side = texture->height;

  /*
   * Figure out the max lod.
   */
#ifdef GLIDE3
  switch( longest_side )
    {
    case 256:
      lod = GR_LOD_LOG2_256;
      break;
    case 128:
      lod = GR_LOD_LOG2_128;
      break;
    case 64:
      lod = GR_LOD_LOG2_64;
      break;
    case 32:
      lod = GR_LOD_LOG2_32;
      break;
    case 16:
      lod = GR_LOD_LOG2_16;
      break;
    case 8:
      lod = GR_LOD_LOG2_8;
      break;
    case 4:
      lod = GR_LOD_LOG2_4;
      break;
    case 2:
      lod = GR_LOD_LOG2_2;
      break;
    case 1:
      lod = GR_LOD_LOG2_1;
      break;
    }
#else
  switch( longest_side )
    {
    case 256:
      lod = GR_LOD_256;
      break;
    case 128:
      lod = GR_LOD_128;
      break;
    case 64:
      lod = GR_LOD_64;
      break;
    case 32:
      lod = GR_LOD_32;
      break;
    case 16:
      lod = GR_LOD_16;
      break;
    case 8:
      lod = GR_LOD_8;
      break;
    case 4:
      lod = GR_LOD_4;
      break;
    case 2:
      lod = GR_LOD_2;
      break;
    case 1:
      lod = GR_LOD_1;
      break;
    }
#endif /* GLIDE3 */

  for( i = 0; i < 16; i++ )
    {
      void *tmp;

      if( !texture->data[i] )
        break;
      tmp = texture->data[i];
      grTexDownloadMipMapLevel( mmid, lod, &tmp );
      lod++;
    }

  if( ( texture->format == GR_TEXFMT_YIQ_422 ) ||
      ( texture->format == GR_TEXFMT_AYIQ_8422 ) )
    {
      int i, j;
      GuNccTable ncc_table;

      for( i = 0; i < 16; i++ )
        {
          ncc_table.yRGB[i] = ( FxU8 )texture->pal[i];
        }
      
      for( i = 0; i < 4; i++ )
        {
          for( j = 0; j < 3; j++ )
            {
              ncc_table.iRGB[i][j] = ( FxI16 )( texture->pal[16 + 3 * i + j] );
              ncc_table.qRGB[i][j] = ( FxI16 )( texture->pal[28 + 3 * i + j] );
            }
        }

      /*
      ** pack the table Y entries
      */
      for ( i = 0; i < 4; i++ )
        {
          FxU32 packedvalue;

          packedvalue  = ( ( FxU32 )( ncc_table.yRGB[i*4+0] & 0xff ) );
          packedvalue |= ( ( FxU32 )( ncc_table.yRGB[i*4+1] & 0xff ) ) << 8;
          packedvalue |= ( ( FxU32 )( ncc_table.yRGB[i*4+2] & 0xff ) ) << 16;
          packedvalue |= ( ( FxU32 )( ncc_table.yRGB[i*4+3] & 0xff ) ) << 24;

          ncc_table.packed_data[i] = packedvalue;
        }

      /*
      ** pack the table I entries
      */
      for ( i = 0; i < 4; i++ )
        {
          FxU32 packedvalue;

          packedvalue  = ( ( FxU32 )( ncc_table.iRGB[i][0] & 0x1ff ) ) << 18;
          packedvalue |= ( ( FxU32 )( ncc_table.iRGB[i][1] & 0x1ff ) ) << 9;
          packedvalue |= ( ( FxU32 )( ncc_table.iRGB[i][2] & 0x1ff ) ) << 0;

          ncc_table.packed_data[i+4] = packedvalue;
        }

      /*
      ** pack the table Q entries
      */
      for ( i = 0; i < 4; i++ )
        {
          FxU32 packedvalue;

          packedvalue  = ( ( FxU32 )( ncc_table.qRGB[i][0] & 0x1ff ) ) << 18;
          packedvalue |= ( ( FxU32 )( ncc_table.qRGB[i][1] & 0x1ff ) ) << 9;;
          packedvalue |= ( ( FxU32 )( ncc_table.qRGB[i][2] & 0x1ff ) ) << 0;

          ncc_table.packed_data[i+8] = packedvalue;
        }

#if 0
      HackSetNCCTable( mmid, &ncc_table );
#endif
    }
}
