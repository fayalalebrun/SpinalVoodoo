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

/* 
 * Pn_8 = convert n bits (n <= 6) to 8 bits by replicating the msb's of input
 * into the lsb's of the output.
 */
static  FxU8    P1_8[] = {0x00,0xff};
static  FxU8    P2_8[] = {0x00,0x55,0xaa,0xff};
static  FxU8    P3_8[] = {0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff};
static  FxU8    P4_8[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
                                          0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
static  FxU8    P5_8[] = {0x00,0x08,0x10,0x18,0x21,0x29,0x31,0x39,
                                          0x42,0x4a,0x52,0x5a,0x63,0x6b,0x73,0x7b,
                                          0x84,0x8c,0x94,0x9c,0xa5,0xad,0xb5,0xbd,
                                          0xc6,0xce,0xd6,0xde,0xe7,0xef,0xf7,0xff};
static  FxU8    P6_8[] = {0x00,0x04,0x08,0x0c,0x10,0x14,0x18,0x1c,
                                          0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,
                                          0x41,0x45,0x49,0x4d,0x51,0x55,0x59,0x5d,
                                          0x61,0x65,0x69,0x6d,0x71,0x75,0x79,0x7d,
                                          0x82,0x86,0x8a,0x8e,0x92,0x96,0x9a,0x9e,
                                          0xa2,0xa6,0xaa,0xae,0xb2,0xb6,0xba,0xbe,
                                          0xc3,0xc7,0xcb,0xcf,0xd3,0xd7,0xdb,0xdf,
                                          0xe3,0xe7,0xeb,0xef,0xf3,0xf7,0xfb,0xff};
static FxU32
_txPixRgb332to8888 (FxU8 c332)
{
    FxU32       a, r, g, b;
    a = 0xff;
    r = P3_8[(c332>>5)&7];
    g = P3_8[(c332>>2)&7];
    b = P2_8[(c332   )&3];
    return (a << 24) | (r << 16) | (g << 8) | b;
}

/* YIQ treated at the image level */

static FxU32
_txPixA8to8888 (FxU8 a8)
{
    FxU32       p = a8;

    return (p << 24) | (0xffffff);
}

static FxU32
_txPixI8to8888 (FxU8 i8)
{
    FxU32       p = i8;

    return (0xff << 24) | (p << 16) | (p << 8) | p;
}

static FxU32
_txPixAi44to8888 (FxU8 c44)
{
    FxU32       a, i;

    a = P4_8[(c44 & 0xF0) >> 4];
    i = P4_8[(c44 & 0x0F)     ];

    return (a << 24) | (i << 16) | (i << 8) | i;
}

/* P8 treated at the image level */

/* 16 bit pixels */
static FxU32 
_txPixArgb8332to8888(FxU16 c8332)
{
    FxU32       a, r, g, b;
    a =      (c8332 >>  8);      
    r = P3_8[(c8332 >>  5) & 0x7];
    g = P3_8[(c8332 >>  2) & 0x7];
    b = P2_8[(c8332      ) & 0x3];
    return (a << 24) | (r << 16) | (g << 8) | b;
}

/* AYIQ8422 treated at image level */

static FxU32
_txPixRgb565to8888 (FxU16 c565)
{
    FxU32       a, r, g, b;
    a = 0xFF;
    r = P5_8[(c565 >> 11)       ];
    g = P6_8[(c565 >>  5) & 0x3f];
    b = P5_8[(c565      ) & 0x1f];
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static FxU32
_txPixArgb1555to8888 (FxU16 c1555)
{
    FxU32       a, r, g, b;
    a = P1_8[(c1555 >> 15)       ];
    r = P5_8[(c1555 >> 10) & 0x1f];
    g = P5_8[(c1555 >>  5) & 0x1f];
    b = P5_8[(c1555      ) & 0x1f];
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static FxU32
_txPixRgba4444to8888 (FxU16 c4444)
{
    FxU32       a, r, g, b;
    a = P4_8[(c4444 >> 12) & 0xf];
    r = P4_8[(c4444 >>  8) & 0xf];
    g = P4_8[(c4444 >>  4) & 0xf];
    b = P4_8[(c4444      ) & 0xf];
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static FxU32
_txPixAi88to8888(FxU16 c88)
{
    FxU32       a, r, g, b;
    a = c88 >> 8;
    r = g = b = (c88 & 0xff);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

/******************************************************************************/
static void
_txImgDequantizeRGB332(FxU32 *out, FxU8 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = _txPixRgb332to8888(*--in);
}

static void
_txImgDequantizeYIQ422(FxU32 *out, FxU8 *in, int w, int h, const FxU32 *yabTable)
{
    int                         n = w * h;
    FxU32                       pal[256];

    txYABtoPal256(pal, yabTable);
    out += n;
    in  += n;
    while (n--) *--out = pal[*--in] | 0xff000000;
}

static void
_txImgDequantizeA8(FxU32 *out, FxU8 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = _txPixA8to8888(*--in);
}

static void
_txImgDequantizeI8(FxU32 *out, FxU8 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = _txPixI8to8888(*--in);
}

static void
_txImgDequantizeAI44(FxU32 *out, FxU8 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = _txPixAi44to8888(*--in);
}

static void
_txImgDequantizeP8(FxU32 *out, FxU8 *in, int w, int h, const FxU32 *pal)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = pal[*--in] | 0xff000000;
}


static void
_txImgDequantizeARGB8332(FxU32 *out, FxU16 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = _txPixArgb8332to8888 (*--in);
}

static void
_txImgDequantizeAYIQ8422(FxU32 *out, FxU16 *in, int w, int h, const FxU32 *yab)
{
    int         n = w * h;
    FxU32       pal[256];

    txYABtoPal256(pal, yab);
    out += n;
    in  += n;
    while (n--) {
        in--; 
        *--out = (pal[(*in) & 0xff] & 0x00ffffff) | ((*in & 0xFF00) << 16);
    }
}

static void
_txImgDequantizeRGB565(FxU32 *out, FxU16 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = _txPixRgb565to8888(*--in);
}

static void
_txImgDequantizeARGB1555(FxU32 *out, FxU16 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = _txPixArgb1555to8888 (*--in);
}

static void
_txImgDequantizeARGB4444(FxU32 *out, FxU16 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = _txPixRgba4444to8888 (*--in);
}

static void
_txImgDequantizeAI88(FxU32 *out, FxU16 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;
    while (n--) *--out = _txPixAi88to8888(*--in);
}


static void
_txImgDequantizeAP88(FxU32 *out, FxU16 *in, int w, int h, const FxU32 *pal)
{
    int         n = w * h;

    out += n;
    in  += n;

    while (n--) {
        in--;
        *--out = (pal[(*in) & 0xff] & 0x00ffffff) | ((*in & 0xFF00) << 16);
    }
}


static void
_txImgDequantizeARGB8888(FxU32 *out, FxU32 *in, int w, int h)
{
    int         n = w * h;

    out += n;
    in  += n;

    while (n--) *--out = *--in; 
}

static void
_txImgDequantizeRGB888(FxU32 *out, FxU32 *in_, int w, int h)
{
    int         n = w * h;
    int i;
    FxU8 *in = ( FxU8 * )in_;

    for( i = 0; i < n; i++ )
      {
        out[i] = 
          ( ( ( FxU32 )0xff ) << 24 ) | 
          ( ( ( FxU32 )in[i*3] )<< 16 ) |
          ( ( ( FxU32 )in[i*3+1] )<< 8 ) |
          ( ( ( FxU32 )in[i*3+2] ) );
      }
}

void    
txMipDequantize(TxMip *txMip, TxMip *pxMip)
{
    /* Walk through all mipmap levels, and convert to ARGB8888 format */
    int         i, w, h;

    if( txVerbose )
      {
        printf("Dequant: (from %s) ..", Format_Name[pxMip->format]);
      }
    w = pxMip->width;
    h = pxMip->height;
    for (i=0; i< txMip->depth; i++) {
        void    *src, *dst;

        src = pxMip->data[i];
        dst = txMip->data[i];

        if( txVerbose )
          {
            printf(" %dx%d", w, h); fflush(stdout);
          }
            
        switch(pxMip->format) {
        case GR_TEXFMT_RGB_332:         _txImgDequantizeRGB332(dst, src, w, h);         
                                                                break;
        case GR_TEXFMT_YIQ_422:         _txImgDequantizeYIQ422(dst, src, w, h, 
                                                                        pxMip->pal); break;
        case GR_TEXFMT_A_8:                     _txImgDequantizeA8(dst, src, w, h);             
                                                                break;
        case GR_TEXFMT_I_8:                     _txImgDequantizeI8(dst, src, w, h);             
                                                                break;
        case GR_TEXFMT_AI_44:           _txImgDequantizeAI44(dst, src, w, h);           
                                                                break;
        case GR_TEXFMT_P_8:                     _txImgDequantizeP8(dst, src, w, h, 
                                                                        pxMip->pal); break;

        case GR_TEXFMT_ARGB_8332:       _txImgDequantizeARGB8332(dst, src, w, h);       
                                                                break;
        case GR_TEXFMT_AYIQ_8422:       _txImgDequantizeAYIQ8422(dst, src, w, h, 
                                                                        pxMip->pal); break;
        case GR_TEXFMT_RGB_565:         _txImgDequantizeRGB565(dst, src, w, h); 
                                                                break;
        case GR_TEXFMT_ARGB_1555:       _txImgDequantizeARGB1555(dst, src, w, h);       
                                                                break;
        case GR_TEXFMT_ARGB_4444:       _txImgDequantizeARGB4444(dst, src, w, h);       
                                                                break;
        case GR_TEXFMT_AI_88:           _txImgDequantizeAI88(dst, src, w, h);           
                                                                break;
        case GR_TEXFMT_AP_88:           _txImgDequantizeAP88(dst, src, w, h, 
                                                                        pxMip->pal); break;
        case GR_TEXFMT_ARGB_8888:       _txImgDequantizeARGB8888(dst, src, w, h);       
                                                                break;
        case GR_TEXFMT_RGB_888:         _txImgDequantizeRGB888(dst, src, w, h);       
                                                                break;

        default:                                                                                                        
                                                                break;
        }

        if (w > 1) w >>= 1;
        if (h > 1) h >>= 1;
    }
    if( txVerbose )
      {
        printf(".\n"); fflush(stdout);
      }
}
