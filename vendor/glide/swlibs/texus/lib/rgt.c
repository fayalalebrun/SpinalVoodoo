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

#include "texusint.h"

const FxU16 IMAGIC         = 0x01DA;
const FxU16 ITYPE_RLE      = 0x01;
const FxU16 ITYPE_NCC      = 0x02;
const FxU16 ITYPE_BGR      = 0x04;
const FxU16 ITYPE_RGT      = 0x08;
    
typedef struct _rgtHeader{
    FxU16 magic;
    FxU8 typeLo;
    FxU8 typeHi;
    FxU8 dimLo;
    FxU8 dimHi;
    FxU8 sizeXLo;
    FxU8 sizeXHi;
    FxU8 sizeYLo;
    FxU8 sizeYHi;
    FxU8 sizeZLo;
    FxU8 sizeZHigh;
} RgtHeader;



static void swapShorts(unsigned short *array, int length)
{
    unsigned short s;
    while (length--) {
        s = *array;
        *array++ = (s << 8) | (s>>8);
    }
}

static void swapLongs(unsigned int *array, int length)
{
    unsigned int s;
    while (length--) {
        s = *array;
        *array++ = (s << 24) | ((s >> 8)&0xFF00) | 
                        ((s&0xFF00) << 8) | (s>>24);
    }
}

// just swap RGB into BGR (leave MSB undefined)
static void swapRGB(unsigned int *array, int length)
{
    unsigned int s;
    while (length--) {
        s = *array;
        *array++ = (s << 16) | (s & 0xFF00) | (s>>16);
    }
}


FxBool 
_txReadRGTHeader( FILE *stream, FxU32 cookie, TxMip *info)
{
    RgtHeader   *rgtHeader = (RgtHeader *) info->pal;

    rgtHeader->magic = (FxU16) cookie;
    if ( stream == NULL ) {
        txPanic("RGT file: Bad file handle.");
        return FXFALSE;
    }
    
    if ( fread( &(rgtHeader->typeLo), 1, sizeof(RgtHeader)-2, stream ) != 10 ) {
        txPanic("RGT file: Unexpected end of file.");
        return FXFALSE;
    }
    if (rgtHeader->magic == IMAGIC) {
        // Needs byte swapping; don't swap magic cookie.
        swapShorts((FxU16 *) &(rgtHeader->typeLo), 5);
    }


    info->format = GR_TEXFMT_ARGB_8888; 
    info->width = rgtHeader->sizeXHi << 8 | rgtHeader->sizeXLo;
    info->height = rgtHeader->sizeYHi << 8 | rgtHeader->sizeYLo;
    info->depth = 1;
    info->size = info->width * info->height * 4;
    if( txVerbose )
      {
        printf("Magic: %.04x w = %d, h = %d, z = %d, typehi = %d, typelo = %d, swap=%d\n", rgtHeader->magic, 
               info->width, info->height, rgtHeader->sizeZLo, rgtHeader->typeHi, rgtHeader->typeLo, rgtHeader->magic == IMAGIC);
      }
    return FXTRUE;
}

// RGT is RGBA in memory (low byte to high byte), or ABGR in a register

FxBool 
_txReadRGTData( FILE *stream, TxMip *info)
{
    RgtHeader   *rgtHeader = (RgtHeader *) info->pal;
    FxU16 type = (rgtHeader->typeHi);
    FxU16 swap = (rgtHeader->magic == IMAGIC); 
    int   x, y;
    
    if ( stream == NULL ) {
        txPanic("RGT file: Bad file handle.");
        return FXFALSE;
    }
    if (type & ITYPE_NCC) {
        txPanic("RGT file: RGT NCC files not supported.");
        return FXFALSE;
    }
    if (type & ITYPE_RLE) {
        txPanic("RGT file: RGT RLE files not supported.");
        return FXFALSE;
    }
    
    // load rgt, rgt's are bottom up
    for ( y = 0; y < info->height; y++ ) {
        FxU32 *data32;
        int     r, g, b, a;

        data32 = (FxU32 *) info->data[0];
        data32 += info->width * (info->height -1 - y);

        // Read scanline worth of data.
        for (x=0; x < info->width; x++) {
            /* Read b, g, r, a  from disk.*/
            r = getc( stream );
            g = getc( stream );
            b = getc( stream );
            a = getc( stream );

            if ( a == EOF ) {
                txPanic("RGT file: Unexpected End of File.");
                return FXFALSE;
            }
            data32[x] = (a << 24) | (r << 16) | (g << 8) | b;
        }

#if 1
        if (swap) {
            swapRGB((unsigned int *)data32, (int)info->width);
        }
#endif

#if 0
        if (type & ITYPE_BGR) {
            /* Swap just blue & red channels */
            swapRGB(data32, info->width);
        }
#endif
    }
    return FXTRUE;
}
