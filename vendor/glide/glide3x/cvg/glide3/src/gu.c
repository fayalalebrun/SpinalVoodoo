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
** 1     10/08/98 11:30a Brent
** 
** 6     7/24/98 1:41p Hohn
** 
** 5     7/02/98 10:29a Atai
** added guQueryResolutionXYExt
** 
** 4     6/24/98 1:47p Atai
** code clean up; rename texute line routine
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

#if ( (GLIDE_PLATFORM & GLIDE_SST_HW) && (GLIDE_PLATFORM & GLIDE_HW_SST1) )
#include <sst1init.h>
#endif

#include "fxinline.h"


static const FxU32 _grResolutionXY[16][2] = 
{
  { 320, 200 },                 /* 320x200 */
  { 320, 240 },                 /* 320x240 */
  { 400, 256 },                 /* 400x256 */
  { 512, 384 },                 /* 512x384 */
  { 640, 200 },                 /* 640x200 */
  { 640, 350 },                 /* 640x350 */
  { 640, 400 },                 /* 640x400 */
  { 640, 480 },                 /* 640x480 */
  { 800, 600 },                 /* 800x600 */
  { 960, 720 },                 /* 960x720 */
  { 856, 480 },                 /* 856x480 */
  { 512, 256 },                 /* 512x256 */
  { 1024, 768 },                /* 1024x768 */
  { 1280, 1024 },               /* 1280x1024 */
  { 1600, 1200 },               /* 1600x1200 */
  { 400, 300 }                  /* 400x300 */
};

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

/*-------------------------------------------------------------------
  Function: guQueryResolutionXYExt
  Date: 02-July-97
  Implementor(s): atai
  Description:
  for those who don't want to write their own query routine
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DDFUNC(guQueryResolutionXY, void, (GrScreenResolution_t res, FxU32 *x, FxU32 *y))
{
#define FN_NAME "guQueryResolutionXYExt"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 80);
  GDBG_INFO_MORE(gc->myLevel, "(%x, 0x%x, 0x%x)\n", res, x, y);
  
  *x = _grResolutionXY[res][0];
  *y = _grResolutionXY[res][1];
  
#undef FN_NAME
} /* guQueryResolutionXYExt */
