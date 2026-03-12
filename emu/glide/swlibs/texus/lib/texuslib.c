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
#include <string.h>
#include <glide.h>
#include <malloc.h>
#include "texus.h"
#include "texusint.h"

void _txDefaultErrorCallback( const char *string, FxBool fatal );

TxErrorCallbackFnc_t _txErrorCallback = _txDefaultErrorCallback;

void _txDefaultErrorCallback( const char *string, FxBool fatal )
{
  fprintf(stderr, "Texus: %s", string );
  if( fatal )
    exit( -1 );
}

#ifdef GLIDE3
    static FxU32 _gr_aspect_index_table[] =
    {
       0,
       1,
       2,
       3
    };
#else
    static FxU32 _gr_aspect_index_table[] =
    {
       3,
       2,
       1,
       0,
       1,
       2,
       3,
    };
#endif

/* size in texels  */
static FxU32 _grMipMapHostSize[4][16] = 
{
    {   /* 1:1 aspect ratio */
        65536, /* 0 : 256x256  */
        16384, /* 1 : 128x128  */
         4096, /* 2 :  64x64   */
         1024, /* 3 :  32x32   */
          256, /* 4 :  16x16   */
           64, /* 5 :   8x8    */
           16, /* 6 :   4x4    */
            4, /* 7 :   2x2    */
            1, /* 8 :   1x1    */
    }, 
    {   /* 2:1 aspect ratio */
        32768, /* 0 : 256x128  */
         8192, /* 1 : 128x64   */
         2048, /* 2 :  64x32   */
          512, /* 3 :  32x16   */
          128, /* 4 :  16x8    */
           32, /* 5 :   8x4    */
            8, /* 6 :   4x2    */
            2, /* 7 :   2x1    */
            1, /* 8 :   1x1    */
    }, 
    {   /* 4:1 aspect ratio */
        16384, /* 0 : 256x64   */
         4096, /* 1 : 128x32   */
         1024, /* 2 :  64x16   */
          256, /* 3 :  32x8    */
           64, /* 4 :  16x4    */
           16, /* 5 :   8x2    */
            4, /* 6 :   4x1    */
            2, /* 7 :   2x1    */
            1, /* 8 :   1x1    */
    },
    {   /* 8:1 aspect ratio */
         8192, /* 0 : 256x32   */
         2048, /* 1 : 128x16   */
          512, /* 2 :  64x8    */
          128, /* 3 :  32x4    */
           32, /* 4 :  16x2    */
            8, /* 5 :   8x1    */
            4, /* 6 :   4x1    */
            2, /* 7 :   2x1    */
            1, /* 8 :   1x1    */
    }
};

FxU32
txTexCalcMemRequired( GrLOD_t small_lod, GrLOD_t large_lod,
                      GrAspectRatio_t aspect, GrTextureFormat_t format ) {
    size_t memRequired;
    FxI32 lod;
    memRequired = 0;

#ifdef GLIDE3
    for( lod = small_lod; lod <= large_lod; lod++ ) {
#else
    for( lod = large_lod; lod <= small_lod; lod++ ) {
#endif
        memRequired +=
#ifdef GLIDE3
           /* Quick port to Glide3. Not the most general way. We would need
           ** something like GR_ASPECT_MAX, GR_ASPECT_MIN, GR_LOD_MAX and GR_LOD_MIN...
           ** because this current implementation will break when we have more
           ** than 8 levels of mipmapping for example.
           */
           _grMipMapHostSize[_gr_aspect_index_table[((aspect>=0)?aspect:-aspect)]] [8-lod] << (format>=GR_TEXFMT_16BIT);
#else
           _grMipMapHostSize[_gr_aspect_index_table[aspect]] [lod] << (format>=GR_TEXFMT_16BIT);
#endif 
    }
    return memRequired ;
}

static GrLOD_t _txLengthToLOD( int len )
{
#ifdef GLIDE3
  switch( len )
    {
    case 256:
      return GR_LOD_LOG2_256;
    case 128:
      return GR_LOD_LOG2_128;
    case 64:
      return GR_LOD_LOG2_64;
    case 32:
      return GR_LOD_LOG2_32;
    case 16:
      return GR_LOD_LOG2_16;
    case 8:
      return GR_LOD_LOG2_8;
    case 4:
      return GR_LOD_LOG2_4;
    case 2:
      return GR_LOD_LOG2_2;
    case 1:
      return GR_LOD_LOG2_1;
    }
  /*
   * This is only here to make the compiler shut up.
   */
  return GR_LOD_LOG2_1;
#else
  switch( len )
    {
    case 256:
      return GR_LOD_256;
    case 128:
      return GR_LOD_128;
    case 64:
      return GR_LOD_64;
    case 32:
      return GR_LOD_32;
    case 16:
      return GR_LOD_16;
    case 8:
      return GR_LOD_8;
    case 4:
      return GR_LOD_4;
    case 2:
      return GR_LOD_2;
    case 1:
      return GR_LOD_1;
    }
  /*
   * This is only here to make the compiler shut up.
   */
  return GR_LOD_1;
#endif

}

static GrAspectRatio_t _txDimensionsToAspectRatio( int width, int height )
{
  int aspect;

  aspect = ( 8 * width ) / height;

#ifdef GLIDE3
  switch( aspect )
    {
    case 64:
      return GR_ASPECT_LOG2_8x1;
    case 32:
      return GR_ASPECT_LOG2_4x1;
    case 16:
      return GR_ASPECT_LOG2_2x1;
    case 8:
      return GR_ASPECT_LOG2_1x1;
    case 4:
      return GR_ASPECT_LOG2_1x2;
    case 2:
      return GR_ASPECT_LOG2_1x4;
    case 1:
      return GR_ASPECT_LOG2_1x8;
    }

  /*
   * Make the compiler shut up.
   */
  return GR_ASPECT_LOG2_1x8;
#else
  switch( aspect )
    {
    case 64:
      return GR_ASPECT_8x1;
    case 32:
      return GR_ASPECT_4x1;
    case 16:
      return GR_ASPECT_2x1;
    case 8:
      return GR_ASPECT_1x1;
    case 4:
      return GR_ASPECT_1x2;
    case 2:
      return GR_ASPECT_1x4;
    case 1:
      return GR_ASPECT_1x8;
    }

  /*
   * Make the compiler shut up.
   */
  return GR_ASPECT_1x8;
#endif
}

/*
 * Return the smallest LOD given an aspect ratio
 * assuming that the LODs "stop" as soon as a side
 * length becomes 1.
 */
static GrLOD_t _txSmallestLOD( GrAspectRatio_t aspect )
{
#ifdef GLIDE3
  switch( aspect )
    {
    case GR_ASPECT_LOG2_8x1:
    case GR_ASPECT_LOG2_1x8:
      return GR_LOD_LOG2_8;
    case GR_ASPECT_LOG2_4x1:
    case GR_ASPECT_LOG2_1x4:
      return GR_LOD_LOG2_4;
    case GR_ASPECT_LOG2_2x1:
    case GR_ASPECT_LOG2_1x2:
      return GR_LOD_LOG2_2;
    case GR_ASPECT_LOG2_1x1:
      return GR_LOD_LOG2_1;
    }
  /*
   * Make the compiler shut up.
   */
  return GR_LOD_LOG2_1;
#else
  switch( aspect )
    {
    case GR_ASPECT_8x1:
    case GR_ASPECT_1x8:
      return GR_LOD_8;
    case GR_ASPECT_4x1:
    case GR_ASPECT_1x4:
      return GR_LOD_4;
    case GR_ASPECT_2x1:
    case GR_ASPECT_1x2:
      return GR_LOD_2;
    case GR_ASPECT_1x1:
      return GR_LOD_1;
    }
  /*
   * Make the compiler shut up.
   */
  return GR_LOD_1;

#endif /* GLIDE3 */
}

void txNccToPal( FxU32 *pal, const GuNccTable *ncc_table ) 
{
  int i, j;

  for( i = 0; i < 16; i++ )
    {
      pal[i] = ncc_table->yRGB[i];
    }
      
  for( i = 0; i < 4; i++ )
    {
      for( j = 0; j < 3; j++ )
        {
            pal[16 + 3 * i + j] = ncc_table->iRGB[i][j];
            pal[28 + 3 * i + j] = ncc_table->qRGB[i][j];
        }
    }
}

void txPalToNcc( GuNccTable *ncc_table, const FxU32 *pal ) {
  int i, j;

  for( i = 0; i < 16; i++ )
    {
      ncc_table->yRGB[i] = ( FxU8 )pal[i];
    }
      
  for( i = 0; i < 4; i++ )
    {
      for( j = 0; j < 3; j++ )
        {
          ncc_table->iRGB[i][j] = ( FxI16 )( pal[16 + 3 * i + j] );
          ncc_table->qRGB[i][j] = ( FxI16 )( pal[28 + 3 * i + j] );
        }
    }
  
  /*
  ** pack the table Y entries
  */
  for ( i = 0; i < 4; i++ )
    {
      FxU32 packedvalue;
      
      packedvalue  = ( ( FxU32 )( ncc_table->yRGB[i*4+0] & 0xff ) );
      packedvalue |= ( ( FxU32 )( ncc_table->yRGB[i*4+1] & 0xff ) ) << 8;
      packedvalue |= ( ( FxU32 )( ncc_table->yRGB[i*4+2] & 0xff ) ) << 16;
      packedvalue |= ( ( FxU32 )( ncc_table->yRGB[i*4+3] & 0xff ) ) << 24;

      ncc_table->packed_data[i] = packedvalue;
    }
  
  /*
  ** pack the table I entries
  */
  for ( i = 0; i < 4; i++ )
    {
      FxU32 packedvalue;
      
      packedvalue  = ( ( FxU32 )( ncc_table->iRGB[i][0] & 0x1ff ) ) << 18;
      packedvalue |= ( ( FxU32 )( ncc_table->iRGB[i][1] & 0x1ff ) ) << 9;

      packedvalue |= ( ( FxU32 )( ncc_table->iRGB[i][2] & 0x1ff ) ) << 0;
      
      ncc_table->packed_data[i+4] = packedvalue;
    }
  
  /*
  ** pack the table Q entries
  */
  for ( i = 0; i < 4; i++ )
    {
      FxU32 packedvalue;
      
      packedvalue  = ( ( FxU32 )( ncc_table->qRGB[i][0] & 0x1ff ) ) << 18;
      packedvalue |= ( ( FxU32 )( ncc_table->qRGB[i][1] & 0x1ff ) ) << 9;;
      packedvalue |= ( ( FxU32 )( ncc_table->qRGB[i][2] & 0x1ff ) ) << 0;
      
      ncc_table->packed_data[i+8] = packedvalue;
    }
}

size_t txInit3dfInfoFromFile( FILE *file, 
                              Gu3dfInfo *info, GrTextureFormat_t destFormat,
                              int *destWidth, int *destHeight,
                              int mipLevels, FxU32 flags )
{
  long file_start_position;
  size_t size_retval;
  int input_format;
  TxMip txMip;

#if 0
  printf( "enter: txInit3dfInfoFromFile\n" );
  fflush( stdout );
#endif

  /*
   * Save the current position of the input file so that we can
   * later recent it.
   */
  file_start_position = ftell( file );

  /*
   * Read the header of the input file to find out some information
   * about it.
   */
  if( ( input_format = _txReadHeader( file, &txMip ) ) == 0 )
    {
      return 0;
    }

  *destWidth = txMip.width;
  *destHeight = txMip.height;

  size_retval = txInit3dfInfo( info, destFormat, destWidth, destHeight,
                               mipLevels, flags );


  /*
   * Set the file offset back to where it was when we entered this 
   * function.
   */
  fseek( file, file_start_position, SEEK_SET );
  
  /*
   * Return the memory required for this texture.
   */
#if 0
  printf( "txInit3dfInfoFromFile returning %ld\n", ( long )size_retval );
  fflush( stdout );
#endif
  return size_retval;
}

size_t txInit3dfInfo( Gu3dfInfo *info, GrTextureFormat_t destFormat,
                      int *destWidth, int *destHeight,
                      int mipLevels, FxU32 flags )
{
  /*
   * If auto-resize is enabled,
   */
  if( ( flags & TX_AUTORESIZE_MASK ) != TX_AUTORESIZE_DISABLE )
    {
      /*
       * Make sure that the texture dimensions are power of two.
       */
      if( ( flags & TX_AUTORESIZE_MASK ) == TX_AUTORESIZE_SHRINK )
        {
          *destWidth  = txFloorPow2( *destWidth );
          *destHeight = txFloorPow2( *destHeight );
        }
      else
        {
          *destWidth  = txCeilPow2( *destWidth );
          *destHeight = txCeilPow2( *destHeight );
        }
      
      /*
       * Make sure the dimensions are in range.
       */
      while( *destWidth > 256 )
        *destWidth >>= 1;
      while( *destHeight > 256 )
        *destHeight >>= 1;

      /*
       * Make sure the aspect ratio is valid.
       */
      while( ( *destWidth / *destHeight ) > 8 )
        {
          *destWidth >>= 1;
        }

      while( ( *destHeight/ *destWidth ) > 8 )
        {
          *destHeight >>= 1;
        }
    }

  /*
   * Calculate the aspect ratio of the texture . . this assumse that
   * the desired texture size is valid!
   */
  info->header.aspect_ratio = _txDimensionsToAspectRatio( *destWidth, *destHeight );

  /*
   * Calculate the min and max LODs for the texture.
   */
  info->header.large_lod = _txLengthToLOD( ( *destHeight > *destWidth ) ? *destHeight : *destWidth );
  info->header.small_lod = _txSmallestLOD( info->header.aspect_ratio );

  /*
   * Make sure that we have the correct number of mipmap levels given
   * the parameter "mipLevels".
   * If mipLevels is -1, then we want all levels.
   */
  if( mipLevels != -1 )
    {
      int tmpNumLevels;
#ifdef GLIDE3
      tmpNumLevels = info->header.large_lod - info->header.small_lod + 1;
#else
      tmpNumLevels = info->header.small_lod - info->header.large_lod + 1;
#endif
      if( tmpNumLevels > mipLevels )
        {
#ifdef GLIDE3
          info->header.small_lod += tmpNumLevels - mipLevels;
#else
          info->header.small_lod -= tmpNumLevels - mipLevels;
#endif
        }
    }

  /*
   * Store the geometry of the texture.
   */
  info->header.width    = *destWidth;
  info->header.height   = *destHeight;

  /*
   * Store the format of the texture.
   */
  info->header.format   = destFormat;
  
  info->mem_required = txTexCalcMemRequired( info->header.small_lod,
                                             info->header.large_lod,
                                             info->header.aspect_ratio,
                                             info->header.format );

  /*
   * Return the amount of texture memory required for this texture.
   * The user is responsible for allocating the space for the texture.
   */
  return info->mem_required;
}


FxBool txConvertFromFile( FILE *file, Gu3dfInfo *info, 
                          FxU32 flags, const void *palNcc )
{
  long file_start_position;
  FxBool retval;
  TxMip txMip;
  
  /*
   * Save the current position of the input file so that we can
   * later recent it.
   */
  file_start_position = ftell( file );

  txMipReadFromFP( &txMip, "(FILE*)", file, GR_TEXFMT_ANY );

  retval = txConvert( info, txMip.format, txMip.width, txMip.height,
                      txMip.data[0], flags, palNcc );
  txFree( txMip.data[0] );

  return retval;
}

FxBool txConvert( Gu3dfInfo *info, GrTextureFormat_t srcFormat,
                  int srcWidth, int srcHeight,
                  const void *srcImage, FxU32 flags,
                  const void *palNcc )
{
  TxMip srcMip;
  TxMip trueColorMip;
  TxMip outputMip;
  TxMip tmpMip;

  /*
   * Make a txMip out of the passed data.
   */
  memset( &srcMip, 0, sizeof( srcMip ) );
  srcMip.format         = srcFormat;
  srcMip.width          = srcWidth;
  srcMip.height         = srcHeight;
  srcMip.depth          = 1;

  if( palNcc )
    {
      switch( srcFormat )
        {
        case GR_TEXFMT_YIQ_422:
        case GR_TEXFMT_AYIQ_8422:
         txNccToPal( srcMip.pal, palNcc);
          break;
        case GR_TEXFMT_P_8:
          memcpy( srcMip.pal, palNcc, sizeof( FxU32 ) * 256 );
          break;
        }
    }
  srcMip.data[0]        = ( void * )srcImage;

  /*
   * Set up a txMip to put a true color version of the texture in.
   */
  memset( &trueColorMip, 0, sizeof( trueColorMip ) );
  trueColorMip.format         = GR_TEXFMT_ARGB_8888;
  trueColorMip.width          = srcWidth;
  trueColorMip.height         = srcHeight;
  /*
   * Set the depth to the mipmapped depth to allocate the image.
   */
#ifdef GLIDE3
  trueColorMip.depth          = info->header.large_lod - info->header.small_lod + 1;
#else
  trueColorMip.depth          = info->header.small_lod - info->header.large_lod + 1;
#endif
  if( !txMipAlloc( &trueColorMip ) )
    return FXFALSE;

  /*
   * Set to one level only since we only want to dequant the first 
   * level.
   */
  trueColorMip.depth          = 1;

  /*
   * Convert from the input format to truecolor.
   */
  txMipDequantize( &trueColorMip, &srcMip );

  /*
   * We realy have more than one level, so. . . 
   */
#ifdef GLIDE3
  trueColorMip.depth          = info->header.large_lod - info->header.small_lod + 1;
#else
  trueColorMip.depth          = info->header.small_lod - info->header.large_lod + 1;
#endif

  /*
   * WARNING!  I do not free srcMip.data[0] since it is passed in by the users.
   */

  /*
   * Resample the true color version of the input image to 
   * the passed in size. . . . this should be a valid
   * size for the hardware to handle.
   */
  tmpMip = trueColorMip;
  tmpMip.width = info->header.width;
  tmpMip.height = info->header.height;
  txMipAlloc( &tmpMip );

  if( ( flags & TX_CLAMP_MASK ) == TX_CLAMP_DISABLE )
    {
      txMipResample( &tmpMip, &trueColorMip );
    }
  else
    {
      txMipClamp( &tmpMip, &trueColorMip );
#if 0
      txMipView( &tmpMip, "blah", FXTRUE, 0 );
#endif
    }
    
  
#if 0
  if( _heapchk() != _HEAPOK )
    txPanic( "_heapchk failed" );
#endif
  txFree( trueColorMip.data[0] );

  trueColorMip = tmpMip;

  /*
   * Generate mipmap levels.
   */
#ifdef GLIDE3
  trueColorMip.depth          = info->header.large_lod - info->header.small_lod + 1;
#else
  trueColorMip.depth          = info->header.small_lod - info->header.large_lod + 1;
#endif
  txMipMipmap( &trueColorMip );

#if 0
  txMipView( &trueColorMip, "blah", FXTRUE, 0 );
#endif

  /*
   * Convert from true color to the output color format.
   */
  memset( &outputMip, 0, sizeof( outputMip ) );
  outputMip.format         = info->header.format;
  outputMip.width          = info->header.width;
  outputMip.height         = info->header.height;
  outputMip.depth          = trueColorMip.depth;
  outputMip.data[0]        = info->data;
#if 0
  txMipAlloc( &outputMip );
#else
  txMipSetMipPointers( &outputMip );
#endif
  
  if( ( flags & TX_TARGET_PALNCC_MASK ) == TX_TARGET_PALNCC_SOURCE )
    {
      txMipTrueToFixedPal( &outputMip, &trueColorMip, palNcc, 
                           flags & TX_FIXED_PAL_QUANT_MASK );
    }
  else
    {
      txMipQuantize( &outputMip, &trueColorMip, outputMip.format, 
                     flags & TX_DITHER_MASK, flags & TX_COMPRESSION_MASK );
    }

  info->data = outputMip.data[0];

  if( ( info->header.format == GR_TEXFMT_YIQ_422 ) ||
      ( info->header.format == GR_TEXFMT_AYIQ_8422 ) )
    {
      txPalToNcc( &info->table.nccTable, outputMip.pal );
    }
  
  if( info->header.format == GR_TEXFMT_P_8 || info->header.format == GR_TEXFMT_AP_88 )
    {
      memcpy( info->table.palette.data, outputMip.pal, sizeof( FxU32 ) * 256 );
    }

  txFree( trueColorMip.data[0] );

  return FXTRUE;
}
