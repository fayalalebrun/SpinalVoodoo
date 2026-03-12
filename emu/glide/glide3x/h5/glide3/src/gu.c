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
** 8     7/18/98 12:31a Jdt
** Removed GUColorCombineFunction.
** 
** 7     6/23/98 9:40a Peter
** fixed build
** 
** 6     6/22/98 7:57p Jdt
** 
** 5     6/22/98 5:42p Russp
** removed #include "fxinline.h"
** 
** 3     1/30/98 4:27p Atai
** gufog* prototype
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 14    12/18/97 2:13p Peter
 * fogTable cataclysm
 * 
 * 13    5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 12    5/21/97 6:05a Peter
 * 
 * 11    3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 10    3/05/97 9:36p Jdt
 * Added guEncodeRLE16
 * 
 * 9     12/23/96 1:37p Dow
 * chagnes for multiplatform glide
**
*/

#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"

#if (GLIDE_PLATFORM & GLIDE_HW_H3)
#include "fxinline.h"
#endif /* (GLIDE_PLATFORM & GLIDE_HW_H3) */

#if ( (GLIDE_PLATFORM & GLIDE_SST_HW) && (GLIDE_PLATFORM & GLIDE_HW_SST1) )
#include <sst1init.h>
#endif

/*---------------------------------------------------------------------------
**  guAlphaSource
*/
GR_DIENTRY(guAlphaSource, void, ( GrAlphaSource_t mode ))
{
  GDBG_INFO(99,"guAlphaSource(%d)\n",mode);
  switch ( mode ) {
  case GR_ALPHASOURCE_CC_ALPHA:
    grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL, 
                    GR_COMBINE_FACTOR_NONE, 
                    GR_COMBINE_LOCAL_CONSTANT, 
                    GR_COMBINE_OTHER_NONE, 
                    FXFALSE );
    break;

  case GR_ALPHASOURCE_ITERATED_ALPHA:
    grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL, 
                    GR_COMBINE_FACTOR_NONE, 
                    GR_COMBINE_LOCAL_ITERATED, 
                    GR_COMBINE_OTHER_NONE, 
                    FXFALSE );
    break;

  case GR_ALPHASOURCE_TEXTURE_ALPHA:
    grAlphaCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, 
                    GR_COMBINE_FACTOR_ONE, 
                    GR_COMBINE_LOCAL_NONE, 
                    GR_COMBINE_OTHER_TEXTURE, 
                    FXFALSE );
    break;

  case GR_ALPHASOURCE_TEXTURE_ALPHA_TIMES_ITERATED_ALPHA:
    grAlphaCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, 
                    GR_COMBINE_FACTOR_LOCAL, 
                    GR_COMBINE_LOCAL_ITERATED, 
                    GR_COMBINE_OTHER_TEXTURE, 
                    FXFALSE );
    break;

  default:
    GR_CHECK_F("grAlphaSource", 1, "unknown alpha source mode");
    break;
  }

  /* xxx not needed at the moment, should update grFogxxx
  _grVerifyNeedForITAlpha();
  */
} /* guAlphaSource */


/*---------------------------------------------------------------------------
** guEndianSwapWords
*/
GR_DIENTRY(guEndianSwapWords, FxU32, ( FxU32 value ))
{
   return ( ( value & 0xFFFF0000 ) >> 16 ) | ( value << 16 );
}

/*---------------------------------------------------------------------------
** guEndianSwapBytes
*/
GR_DIENTRY(guEndianSwapBytes, FxU16, ( FxU16 value ))
{
  return ( ( value & 0xFF00 ) >> 8 ) | ( value << 8 );
}

/*---------------------------------------------------------------------------
** guFogTableIndexToW
*/
GR_DIENTRY(guFogTableIndexToW, float, ( int i ))
{
  return (float)pow(2.0,3.0+(double)(i>>2)) / (8-(i&3));
}

/*---------------------------------------------------------------------------
** guFogGenerateExp
*/
GR_DIENTRY(guFogGenerateExp, void,
           ( GrFog_t *fogtable, float density )) 
{
  int   i;
  float f;
  float scale;
  float dp;

  GDBG_INFO(99,"guFogGenerateExp(0x%x,%g)\n",fogtable,density);
  dp = density * guFogTableIndexToW( kInternalFogTableEntryCount - 1 );
  scale = 1.0F / ( 1.0F - ( float ) exp( -dp ) );

  for ( i = 0; i < kInternalFogTableEntryCount; i++ ) {
     dp = density * guFogTableIndexToW( i );
     f = ( 1.0F - ( float ) exp( -dp ) ) * scale;

     if ( f > 1.0F )
        f = 1.0F;
     else if ( f < 0.0F )
        f = 0.0F;

     f *= 255.0F;
     fogtable[i] = ( GrFog_t ) f;
  }
} /* guFogGenerateExp */

/*---------------------------------------------------------------------------
** guFogGenerateExp2
*/
GR_DIENTRY(guFogGenerateExp2, void,
           ( GrFog_t *fogtable, float density ))
{
  int   i;
  float f;
  float scale;
  float dp;

  GDBG_INFO(99,"guFogGenerateExp2(0x%x,%g)\n",fogtable,density);
  dp = density * guFogTableIndexToW( kInternalFogTableEntryCount - 1 );
  scale = 1.0F / ( 1.0F - ( float ) exp( -( dp * dp ) ) );

  for ( i = 0; i < kInternalFogTableEntryCount; i++ ) {
     dp = density * guFogTableIndexToW( i );
     f = ( 1.0F - ( float ) exp( -( dp * dp ) ) ) * scale;

     if ( f > 1.0F )
        f = 1.0F;
     else if ( f < 0.0F )
        f = 0.0F;

     f *= 255.0F;
     fogtable[i] = ( GrFog_t ) f;
  }
} /* guFogGenerateExp2 */

/*---------------------------------------------------------------------------
** guFogGenerateLinear
*/
GR_DIENTRY(guFogGenerateLinear, void, 
           ( GrFog_t *fogtable,
             float nearZ, float farZ ))
{
   int i;
   float world_w;
   float f;

  GDBG_INFO(99,"guFogGenerateLinear(0x%x,%g,%g)\n",fogtable,nearZ,farZ);
  for ( i = 0; i < kInternalFogTableEntryCount; i++ ) {
    world_w = guFogTableIndexToW( i );
    if ( world_w > 65535.0F )
      world_w = 65535.0F;
    
    f = ( world_w - nearZ ) / ( farZ - nearZ );
    if ( f > 1.0F )
      f = 1.0F;
    else if ( f < 0.0F )
      f = 0.0F;
    f *= 255.0F;
    fogtable[i] = ( GrFog_t ) f;
  }
} /* guFogGenerateLinear */

/*
** setlevel
*/
static void
setlevel( FxU16 *data, FxU16 color, int width, int height )
{
   int s, t;

   for ( t = 0; t < height; t++ )
   {
      for ( s = 0; s < width; s++ )
      {
         *data = color;
          data++;
      }
   }
} /* setlevel */

/*---------------------------------------------------------------------------
**  guTexCreateColorMipMap
*/
GR_DIENTRY(guTexCreateColorMipMap, FxU16 *, ( void ))
{
   FxU32 memrequired;
   FxU16 *data;
   FxU16 *start;

   GDBG_INFO(99,"guTexCreateColorMipMap()\n");
   memrequired = 2 * ( 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2 + 1 * 1 );
   start = data = malloc( memrequired );
   if ( !data )
      return 0;

   setlevel( data,            0xF800, 256, 256 );
   setlevel( data += 256*256, 0x07e0, 128, 128 );
   setlevel( data += 128*128, 0x001F, 64, 64 );
   setlevel( data += 64*64,   0xFFFF, 32, 32);
   setlevel( data += 32*32,   0x0000, 16, 16 );
   setlevel( data += 16*16,   0xF800, 8, 8);
   setlevel( data += 8*8,     0x07e0, 4, 4 );
   setlevel( data += 4*4,     0x001f, 2, 2 );
   setlevel( data += 2*2,     0xFFFF, 1, 1 );

   return start;
} /* guTexCreateColoMipMap */

/* GMT: this code used to be in the code above but I removed it for
        readability, the first section was case(1) the second section
        was a replacement for the simple 32-bit copy loop
*/
#if 0

      /* GMT: from what I can tell its not bad to perform unaligned DWORD
         loads on a P5, so we use the case 2) all the time
         in fact, its twice as fast as doing aligned short copies
         */
if ((dstX ^ (((FxU32)src)>>1)) & 1) {
    /* case 1) a simple 16-bit copy loop */
    for (x=0; x < w; x++) {
        GR_SET16(lfbPtr[x], ((FxU16 *)src)[x]);
    }
}
#endif

#if 0
/* GMT: I tried to unroll the loop but for some reason the
   code scheduling was such that it was slower
   I think it had to do with the way the 2 mov instructions paired
   Perhaps they were unaligned in the cache?
   */
FxU32 a,b;
FxU32 *s = (FxU32 *)src;
FxU32 *d = lfbPtr;

while (d < &lfbPtr[w-1]) {
    a = s[0];
    b = s[1];
    GR_SET(d[0], a);
    GR_SET(d[1], b);
    s += 2;
    d += 2;
}
if (w & 1) {
    GR_SET(d[0], s[0]);
}
#endif

/*-------------------------------------------------------------------
  Function: guEncodeRle
  Date: 3/5/96
  Implementor(s): jdt
  Library: Glide Utilities
  Description:
  Encode an RGB565 image into RLE16 format
  Arguments:
  dst - destination rle image data ( NULL for bytecount only )
  src - source rgb565 image data
  width - width of source data
  height - height of source data
  Return:
  number of bytes in encoded rle image
  -------------------------------------------------------------------*/
GR_ENTRY( guEncodeRLE16, int, ( void *dst, void *src, FxU32 width, FxU32 height ))
{
    int byteCount = 0;
    int sourceImageSizeInWords;
    FxU16 *srcPixels;
    FxU32 *dstPixels;

    sourceImageSizeInWords = width * height;

    srcPixels = src;

    if ( dst ) {
        dstPixels = dst;
        while( sourceImageSizeInWords-- ) {
            short length    = 1;
            short color     = *srcPixels;
            int   lookAhead = 1;

            while( (sourceImageSizeInWords-length)&&
                   (color == srcPixels[lookAhead]) ) {
                length++;
                lookAhead++;
            }

            *dstPixels = ((((FxU32)length)<<16) | ((FxU32)color));
            dstPixels++;

            byteCount+=4;

            srcPixels+=length;
            sourceImageSizeInWords-=length;            
        }
    } else {
        while( sourceImageSizeInWords-- ) {
            short length    = 1;
            short color     = *srcPixels;
            int   lookAhead = 1;

            while( (sourceImageSizeInWords-length)&&
                   (color == srcPixels[lookAhead]) ) {
                length++;
                lookAhead++;
            }

            byteCount+=4;
            srcPixels+=length;
            sourceImageSizeInWords-=length;            
        }
    }
    return byteCount;
}

