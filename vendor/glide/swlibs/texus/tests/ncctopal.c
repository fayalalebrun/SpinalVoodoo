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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glide.h>
#include "texusint.h"

/* This calculates the maximum number of mipmaps a texture can have
   given its dimensions
*/

GrHwConfiguration hwconfig;
static char version[80];

void Error( FxBool fatal, const char *format, ... ) {
    va_list args;
    #if macintosh
    int len;
    extern int debugger_avail;

    if(debugger_avail) {
        va_start( args, format );
        vsprintf((void*)(err_buffer+1), format, args);
        va_end( args );
        len = strlen((void*)(err_buffer+1));
        if(len > 255) {
            len = 255;
        }
        err_buffer[0] = (unsigned char)len;
        DebugStr(err_buffer);
    }
    #else
    va_start( args, format );
    vfprintf( stdout, format, args );
    va_end( args );
    #endif

    if ( fatal )
      {
        exit( -1 );
      }
    return;
}

static int
CalculateMipMapLevels(int width, int height) {
  int LOD=0;

  while(width>0 && height>0) {
      width >>= 1; height >>= 1;
      LOD++;
  }

  return(LOD);
}

static  GrLOD_t _atrTexLenToLOD( FxU32 len ) {
    GrLOD_t lod;

#ifdef GLIDE3
    switch( len ) {
        case 256:
          lod = GR_LOD_LOG2_256;
          break;
        case 128:
          lod = GR_LOD_LOG2_128;
          break;
        case  64:
          lod = GR_LOD_LOG2_64;
          break;
        case  32:
          lod = GR_LOD_LOG2_32;
          break;
        case  16:
          lod = GR_LOD_LOG2_16;
          break;
        case   8:
          lod = GR_LOD_LOG2_8;
          break;
        case   4:
          lod = GR_LOD_LOG2_4;
          break;
        case   2:
          lod = GR_LOD_LOG2_2;
          break;
        case   1:
          lod = GR_LOD_LOG2_1;
          break;
        default:
          lod = -1;
          break;
    }
#else
    switch( len ) {
        case 256:
          lod = GR_LOD_256;
          break;
        case 128:
          lod = GR_LOD_128;
          break;
        case  64:
          lod = GR_LOD_64;
          break;
        case  32:
          lod = GR_LOD_32;
          break;
        case  16:
          lod = GR_LOD_16;
          break;
        case   8:
          lod = GR_LOD_8;
          break;
        case   4:
          lod = GR_LOD_4;
          break;
        case   2:
          lod = GR_LOD_2;
          break;
        case   1:
          lod = GR_LOD_1;
          break;
        default:
          lod = -1;
          break;
    }
#endif /* GLIDE3 */
    return lod;
}

static GrAspectRatio_t _atrTexAspect( FxU32 width, FxU32 height ) {
    GrAspectRatio_t aspect;

    width *= 8;
    width /= height;
#ifdef GLIDE3
    switch( width ) {
        case 64:
          aspect = GR_ASPECT_LOG2_8x1;
          break;
        case 32:
          aspect = GR_ASPECT_LOG2_4x1;
          break;
        case 16:
          aspect = GR_ASPECT_LOG2_2x1;
          break;
        case  8:
          aspect = GR_ASPECT_LOG2_1x1;
          break;
        case  4:
          aspect = GR_ASPECT_LOG2_1x2;
          break;
        case  2:
          aspect = GR_ASPECT_LOG2_1x4;
          break;
        case  1:
          aspect = GR_ASPECT_LOG2_1x8;
          break;
    }
#else
    switch( width ) {
        case 64:
          aspect = GR_ASPECT_8x1;
          break;
        case 32:
          aspect = GR_ASPECT_4x1;
          break;
        case 16:
          aspect = GR_ASPECT_2x1;
          break;
        case  8:
          aspect = GR_ASPECT_1x1;
          break;
        case  4:
          aspect = GR_ASPECT_1x2;
          break;
        case  2:
          aspect = GR_ASPECT_1x4;
          break;
        case  1:
          aspect = GR_ASPECT_1x8;
          break;
    }
#endif /* GLIDE3 */
    return aspect;
}

void main(int argc, char **argv) {
    FILE *fp;
    Gu3dfInfo src, dst;
    char *fileName = "yiq.3df";
    FxU32 target_width, target_height, target_format, target_nlevels;
    size_t tex_mem_required;
    TxMip txMip;
    FxU32 longSide, shortSide;

    if (!txMipRead(&txMip, fileName, GR_TEXFMT_ANY)) {
        Error(FXFALSE, "can't load image %s\n", fileName);
    }

    /*-------------------------------------------------------
      Discern Texture Attributes
      -------------------------------------------------------*/

    if ( txMip.width > txMip.height ) {
        longSide  = txMip.width;
        shortSide = txMip.height;
    } else {
        longSide  = txMip.height;
        shortSide = txMip.width;
    }


    src.header.width = txMip.width;
    src.header.height = txMip.height;
    src.header.large_lod = _atrTexLenToLOD( longSide );
    src.header.small_lod = src.header.large_lod + txMip.depth - 1;
    src.header.aspect_ratio  = _atrTexAspect( txMip.width, txMip.height );
    src.header.format  = txMip.format;

    src.data = txMip.data[0];

    /* initialize palette if it has one */

    switch( txMip.format ) {
    case GR_TEXFMT_YIQ_422:
    case GR_TEXFMT_AYIQ_8422:
      txPalToNcc(&src.table.nccTable, txMip.pal);
      break;
    case GR_TEXFMT_P_8:
    case GR_TEXFMT_AP_88:
      memcpy(&src.table.palette, txMip.pal, 256*sizeof(FxU32));
      break;
    default:
      ;
    }

    target_format = GR_TEXFMT_P_8;
    target_width = src.header.width;
    target_height = src.header.height;

    /* make sure texture is within range, NB texus assumes textures must be <= 256 */

    while( target_width > 256 )
        target_width >>= 1;
    while( target_height > 256 )
        target_height >>= 1;

    target_nlevels=CalculateMipMapLevels(target_width, target_height);

    tex_mem_required = txInit3dfInfo( &dst, target_format,
                                      &target_width, &target_height,
                                      target_nlevels, TX_AUTORESIZE_GROW );
    /*
     * Make sure txInit3dfInfo didn't fail.
     */

    if ( tex_mem_required == 0 ) {
        Error( FXTRUE, "_atrGlideRemapTexture: Problem with txInit3dfInfo" );
    }

    if ( ( dst.data = malloc( tex_mem_required )) == NULL ) {
        Error( FXTRUE, "Out of memory allocating system memory textures." );
    }

    /*
     * Allocate system memory for the texture.
     * TBD: texus does not use ATB's memory allocator so we better not use
     *      it here either.
     */

    /*
     * Convert to a texture that can be downloaded by Glide.
     */

    txConvert(  &dst, src.header.format, src.header.width, src.header.height,
                src.data, TX_DITHER_ERR, &src.table );


    fp = fopen("ncctopal.3df", "wb");

    if (!txWrite( &dst, fp, TX_WRITE_3DF)) {
        Error(FXTRUE, "Could write texture data");
    }

    fclose(fp);
}
