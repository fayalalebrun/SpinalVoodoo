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

static int 
dithmat[4][4] = {       {  0,  8,  2, 10 }, 
						{ 12,  4, 14,  6 }, 
						{  3, 11,  1,  9 }, 
						{ 15,  7, 13,  5 }};

// for error diffusion.
static int      errR[MAX_TEXWIDTH], errG[MAX_TEXWIDTH], errB[MAX_TEXWIDTH];     

static int
_txPixQuantize_RGB332( unsigned int argb, int x, int y, int w)
{
    return (
                        (((argb>>16) & 0xE0) |
                         ((argb>>11) & 0x1C) |
                         ((argb>> 6) & 0x03) )          );
}

static int
_txPixQuantize_RGB332_D4x4( unsigned int argb, int x, int y, int w)
{
    int d = dithmat[y&3][x&3];
    int n, t;

    n = (int) (((argb >> 16) & 0xFF) * 0x70/255.0f + 0.5f) + d; 
    t = (n>>4)<<5;
    n = (int) (((argb >>  8) & 0xFF) * 0x70/255.0f + 0.5f) + d; 
    t |= (n>>4)<<2;
    n = (int) (((argb      ) & 0xFF) * 0x30/255.0f + 0.5f) + d; 
    t |= (n>>4)<<0;
    return t & 0xFF;
}

static int
_txPixQuantize_RGB332_DErr( unsigned int argb, int x, int y, int w)
{
    static unsigned char a3[] = {0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff};
    static unsigned char a2[] = {0x00,0x55,0xaa,0xff};
    static int          qr, qg, qb;             // quantized incoming values.
    int                         ir, ig, ib;             // incoming values.
    int                         t;

    ir = (argb >> 16) & 0xFF;   // incoming pixel values.
    ig = (argb >>  8) & 0xFF;
    ib = (argb      ) & 0xFF;

    if (x == 0) qr = qg = qb = 0;

    ir += errR[x] + qr;
    ig += errG[x] + qg;
    ib += errB[x] + qb;

    qr = ir;            // quantized pixel values. 
    qg = ig;            // qR is error from pixel to left, errR is
    qb = ib;            // error from pixel to the top & top left.

    if (qr < 0) qr = 0; if (qr > 255) qr = 255;         // clamp.
    if (qg < 0) qg = 0; if (qg > 255) qg = 255;
    if (qb < 0) qb = 0; if (qb > 255) qb = 255;

    // To RGB332.
    qr = (int) (qr * 0x7ff/255.0f);     qr >>= 8;
    qg = (int) (qg * 0x7ff/255.0f);     qg >>= 8;
    qb = (int) (qb * 0x3ff/255.0f);     qb >>= 8;

    t  = (qr << 5) | (qg << 2) | qb;    // this is the value to be returned.

    // Now dequantize the input, and compute & distribute the errors.
    qr = a3[qr];        qr = ir - qr;
    qg = a3[qg];        qg = ig - qg;
    qb = a2[qb];        qb = ib - qb;

    // 3/8 (=0.375) to the EAST, 3/8 to the SOUTH, 1/4 (0.25) to the SOUTH-EAST.
    errR[x]  = ((x == 0) ? 0 : errR[x]) + ((int) (qr * 0.375f));
    errG[x]  = ((x == 0) ? 0 : errG[x]) + ((int) (qg * 0.375f));
    errB[x]  = ((x == 0) ? 0 : errB[x]) + ((int) (qb * 0.375f));

    errR[x+1] = (int) (qr * 0.250f);
    errG[x+1] = (int) (qg * 0.250f);
    errB[x+1] = (int) (qb * 0.250f);

    qr = (int) (qr * 0.375f);           // Carried to the pixel on the right.
    qg = (int) (qg * 0.375f);
    qb = (int) (qb * 0.375f);

    return t & 0xFF;
}

/* YIQ422 done elsewhere */

static int
_txPixQuantize_A8( unsigned int argb, int x, int y, int w)
{
    return (argb >> 24);
}

static int
_txPixQuantize_I8( unsigned int argb, int x, int y, int w)
{
    return (
    ((int) (((argb >>16) & 0xFF) * .30F +
                ((argb >> 8) & 0xFF) * .59F +
                ((argb     ) & 0xFF) * .11F + 0.5f )) & 0xFF);
}

static int
_txPixQuantize_AI44( unsigned int argb, int x, int y, int w)
{
    return(
        (int)   ((      ((argb>>16) & 0xFF) * .30F +
                                ((argb>> 8) & 0xFF) * .59F +
                                ((argb    ) & 0xFF) * .11F + 0.5f ) * 0.0625f) |
        (int)           ((argb>>24) & 0xF0));
}

static int
_txPixQuantize_AI44_D4x4( unsigned int argb, int x, int y, int w)
{
    int d = dithmat[y&3][x&3];
    int n, t;

    /* Don't dither alpha channel */
    n = (int)   (       ((argb>>16) & 0xFF) * .30F +
                                ((argb>> 8) & 0xFF) * .59F +
                                ((argb    ) & 0xFF) * .11F + 0.5f);


    n = (int) (n * 0xF0/255.0f + 0.5f) + d;     
    t = (n>>4);
    t |= (int)  ((argb>>24) & 0xF0);
    return t & 0xFF;
}

static int
_txPixQuantize_AI44_DErr( unsigned int argb, int x, int y, int w)
{
    int ii, t;
    static      int     qi;

    /* Don't dither alpha channel */
    ii = (int)  (       ((argb>>16) & 0xFF) * .30F +
                                ((argb>> 8) & 0xFF) * .59F +
                                ((argb    ) & 0xFF) * .11F + 0.5f);


    if (x == 0) qi = 0;
    ii += errR[x] + qi;
    qi = ii;
    if (qi < 0) qi = 0; if (qi > 255) qi = 255;         // clamp.
    qi = (int) (qi * 0xfff/255.0f);     qi >>= 8;

    t = qi;
    t |= (int)  ((argb>>24) & 0xF0);


    // Now dequantize the input, and compute & distribute the errors.
    qi = (qi << 4) | qi;
    qi = ii - qi;

    // 3/8 (=0.375) to the EAST, 3/8 to the SOUTH, 1/4 (0.25) to the SOUTH-EAST.
    errR[x]  = ((x == 0) ? 0 : errR[x]) + ((int) (qi * 0.375f));
    errR[x+1] = (int) (qi * 0.250f);
    qi = (int) (qi * 0.375f);           // Carried to the pixel on the right.

    return t & 0xFF;
}


static int
_txPixQuantize_ARGB8332 ( unsigned int argb, int x, int y, int w)
{
    return (
                         ((argb>>16) & 0xE0) |
                         ((argb>>11) & 0x1C) |
                         ((argb>> 6) & 0x03) |
                         ((argb>>16) & 0xFF00)          );
}


static int
_txPixQuantize_ARGB8332_D4x4( unsigned int argb, int x, int y, int w)
{
    int d = dithmat[y&3][x&3];
    int n, t;

    n = (int) (((argb >> 16) & 0xFF) * 0x70/255.0f + 0.5f) + d; 
    t = (n>>4)<<5;
    n = (int) (((argb >>  8) & 0xFF) * 0x70/255.0f + 0.5f) + d; 
    t |= (n>>4)<<2;
    n = (int) (((argb      ) & 0xFF) * 0x30/255.0f + 0.5f) + d; 
    t |= (n>>4)<<0;
    t |= ((argb >> 16) & 0xFF00);
    return t & 0xFFFF;
}

static int
_txPixQuantize_ARGB8332_DErr( unsigned int argb, int x, int y, int w)
{
    int t;

    t = _txPixQuantize_RGB332_DErr(argb, x, y, w);
    t |= ((argb >> 16) & 0xFF00);
    return t & 0xFFFF;
}

/* AYIQ8422 done elsewhere */

static int
_txPixQuantize_RGB565( unsigned int argb, int x, int y, int w)
{
    return (
                    ((argb >> 8) & 0xF800) |
                        ((argb >> 5) & 0x07E0) |
                        ((argb >> 3) & 0x001F)          );
}

static int
_txPixQuantize_RGB565_D4x4 ( unsigned int argb, int x, int y, int w)
{
    int d = dithmat[y&3][x&3];
    int n, t;

    n = (int) (((argb >> 16) & 0xFF) * 0x1F0/255.0f + 0.5f) + d;        
    t = (n>>4)<<11;
    n = (int) (((argb >>  8) & 0xFF) * 0x3F0/255.0f + 0.5f) + d;        
    t |= (n>>4)<<5;
    n = (int) (((argb      ) & 0xFF) * 0x1F0/255.0f + 0.5f) + d;        
    t |= (n>>4)<<0;
    return t & 0xFFFF;
}


static int
_txPixQuantize_RGB565_DErr ( unsigned int argb, int x, int y, int w)
{
    static int          qr, qg, qb;             // quantized incoming values.
    int                         ir, ig, ib;             // incoming values.
    int                         t;

    ir = (argb >> 16) & 0xFF;   // incoming pixel values.
    ig = (argb >>  8) & 0xFF;
    ib = (argb      ) & 0xFF;

    if (x == 0) qr = qg = qb = 0;

    ir += errR[x] + qr;
    ig += errG[x] + qg;
    ib += errB[x] + qb;

    qr = ir;            // quantized pixel values. 
    qg = ig;            // qR is error from pixel to left, errR is
    qb = ib;            // error from pixel to the top & top left.

    if (qr < 0) qr = 0; if (qr > 255) qr = 255;         // clamp.
    if (qg < 0) qg = 0; if (qg > 255) qg = 255;
    if (qb < 0) qb = 0; if (qb > 255) qb = 255;

    // To RGB565.
    qr = (int) (qr * 0x1FFF/255.0f);    qr >>= 8;
    qg = (int) (qg * 0x3FFF/255.0f);    qg >>= 8;
    qb = (int) (qb * 0x1FFF/255.0f);    qb >>= 8;

    t  = (qr << 11) | (qg << 5) | qb;   // this is the value to be returned.

    // Now dequantize the input, and compute & distribute the errors.
    qr = (qr << 3) | (qr >> 2);
    qg = (qg << 2) | (qg >> 4);
    qb = (qb << 3) | (qb >> 2);
    qr = ir - qr;
    qg = ig - qg;
    qb = ib - qb;

    // 3/8 (=0.375) to the EAST, 3/8 to the SOUTH, 1/4 (0.25) to the SOUTH-EAST.
    errR[x]  = ((x == 0) ? 0 : errR[x]) + ((int) (qr * 0.375f));
    errG[x]  = ((x == 0) ? 0 : errG[x]) + ((int) (qg * 0.375f));
    errB[x]  = ((x == 0) ? 0 : errB[x]) + ((int) (qb * 0.375f));

    errR[x+1] = (int) (qr * 0.250f);
    errG[x+1] = (int) (qg * 0.250f);
    errB[x+1] = (int) (qb * 0.250f);

    qr = (int) (qr * 0.375f);           // Carried to the pixel on the right.
    qg = (int) (qg * 0.375f);
    qb = (int) (qb * 0.375f);

    return t & 0xFFFF;
}

static int
_txPixQuantize_ARGB1555( unsigned int argb, int x, int y, int w)
{
    return (
                    ((argb >> 9) & 0x7C00) |
                        ((argb >> 6) & 0x03E0) |
                        ((argb >> 3) & 0x001F) |        
                        ((argb >> 24) ? 0x8000 : 0)     );
}

static int
_txPixQuantize_ARGB1555_D4x4 ( unsigned int argb, int x, int y, int w)
{
    int d = dithmat[y&3][x&3];
    int n, t;

    n = (int) (((argb >> 16) & 0xFF) * 0x1F0/255.0f + 0.5f) + d;        
    t = (n>>4)<<10;
    n = (int) (((argb >>  8) & 0xFF) * 0x1F0/255.0f + 0.5f) + d;        
    t |= (n>>4)<<5;
    n = (int) (((argb      ) & 0xFF) * 0x1F0/255.0f + 0.5f) + d;        
    t |= (n>>4)<<0;
    t |= ((argb >> 24) ? 0x8000 : 0);
    return t & 0xFFFF;
}

static int
_txPixQuantize_ARGB1555_DErr ( unsigned int argb, int x, int y, int w)
{
    static int          qr, qg, qb;             // quantized incoming values.
    int                         ir, ig, ib;             // incoming values.
    int                         t;

    ir = (argb >> 16) & 0xFF;   // incoming pixel values.
    ig = (argb >>  8) & 0xFF;
    ib = (argb      ) & 0xFF;

    if (x == 0) qr = qg = qb = 0;

    ir += errR[x] + qr;
    ig += errG[x] + qg;
    ib += errB[x] + qb;

    qr = ir;            // quantized pixel values. 
    qg = ig;            // qR is error from pixel to left, errR is
    qb = ib;            // error from pixel to the top & top left.

    if (qr < 0) qr = 0; if (qr > 255) qr = 255;         // clamp.
    if (qg < 0) qg = 0; if (qg > 255) qg = 255;
    if (qb < 0) qb = 0; if (qb > 255) qb = 255;

    // To RGB565.
    qr = (int) (qr * 0x1FFF/255.0f);    qr >>= 8;
    qg = (int) (qg * 0x1FFF/255.0f);    qg >>= 8;
    qb = (int) (qb * 0x1FFF/255.0f);    qb >>= 8;

    t  = (qr << 10) | (qg << 5) | qb;   // this is the value to be returned.
    t |= ((argb >> 24) ? 0x8000 : 0);

    // Now dequantize the input, and compute & distribute the errors.
    qr = (qr << 3) | (qr >> 2);
    qg = (qg << 3) | (qg >> 2);
    qb = (qb << 3) | (qb >> 2);
    qr = ir - qr;
    qg = ig - qg;
    qb = ib - qb;

    // 3/8 (=0.375) to the EAST, 3/8 to the SOUTH, 1/4 (0.25) to the SOUTH-EAST.
    errR[x]  = ((x == 0) ? 0 : errR[x]) + ((int) (qr * 0.375f));
    errG[x]  = ((x == 0) ? 0 : errG[x]) + ((int) (qg * 0.375f));
    errB[x]  = ((x == 0) ? 0 : errB[x]) + ((int) (qb * 0.375f));

    errR[x+1] = (int) (qr * 0.250f);
    errG[x+1] = (int) (qg * 0.250f);
    errB[x+1] = (int) (qb * 0.250f);

    qr = (int) (qr * 0.375f);           // Carried to the pixel on the right.
    qg = (int) (qg * 0.375f);
    qb = (int) (qb * 0.375f);

    return t & 0xFFFF;
}

static int
_txPixQuantize_ARGB4444 (unsigned int argb, int x, int y, int w)
{
    return (
                    ((argb >> 12) & 0x0F00) |
                        ((argb >>  8) & 0x00F0) |
                        ((argb >>  4) & 0x000F) |       
                        ((argb >> 16) & 0xF000) );
}

static int
_txPixQuantize_ARGB4444_D4x4 (unsigned int argb, int x, int y, int w)
{
    int d = dithmat[y&3][x&3];
    int n, t;

    n = (int) (((argb >> 16) & 0xFF) * 0xF0/255.0f + 0.5f) + d; 
    t = (n>>4)<<8;
    n = (int) (((argb >>  8) & 0xFF) * 0xF0/255.0f + 0.5f) + d; 
    t |= (n>>4)<<4;
    n = (int) (((argb      ) & 0xFF) * 0xF0/255.0f + 0.5f) + d; 
    t |= (n>>4)<<0;
    t |= (argb >> 16) & 0xF000;
    return t & 0xFFFF;
}

static int
_txPixQuantize_ARGB4444_DErr (unsigned int argb, int x, int y, int w)
{
    static int          qr, qg, qb;             // quantized incoming values.
    int                         ir, ig, ib;             // incoming values.
    int                         t;

    ir = (argb >> 16) & 0xFF;   // incoming pixel values.
    ig = (argb >>  8) & 0xFF;
    ib = (argb      ) & 0xFF;

    if (x == 0) qr = qg = qb = 0;

    ir += errR[x] + qr;
    ig += errG[x] + qg;
    ib += errB[x] + qb;

    qr = ir;            // quantized pixel values. 
    qg = ig;            // qR is error from pixel to left, errR is
    qb = ib;            // error from pixel to the top & top left.

    if (qr < 0) qr = 0; if (qr > 255) qr = 255;         // clamp.
    if (qg < 0) qg = 0; if (qg > 255) qg = 255;
    if (qb < 0) qb = 0; if (qb > 255) qb = 255;

    // To RGB565.
    qr = (int) (qr * 0xFFF/255.0f);     qr >>= 8;
    qg = (int) (qg * 0xFFF/255.0f);     qg >>= 8;
    qb = (int) (qb * 0xFFF/255.0f);     qb >>= 8;

    t  = (qr <<  8) | (qg << 4) | qb;   // this is the value to be returned.
    t |= (argb >> 16) & 0xF000;

    // Now dequantize the input, and compute & distribute the errors.
    qr = (qr << 4) | (qr >> 0);
    qg = (qg << 4) | (qg >> 0);
    qb = (qb << 4) | (qb >> 0);
    qr = ir - qr;
    qg = ig - qg;
    qb = ib - qb;

    // 3/8 (=0.375) to the EAST, 3/8 to the SOUTH, 1/4 (0.25) to the SOUTH-EAST.
    errR[x]  = ((x == 0) ? 0 : errR[x]) + ((int) (qr * 0.375f));
    errG[x]  = ((x == 0) ? 0 : errG[x]) + ((int) (qg * 0.375f));
    errB[x]  = ((x == 0) ? 0 : errB[x]) + ((int) (qb * 0.375f));

    errR[x+1] = (int) (qr * 0.250f);
    errG[x+1] = (int) (qg * 0.250f);
    errB[x+1] = (int) (qb * 0.250f);

    qr = (int) (qr * 0.375f);           // Carried to the pixel on the right.
    qg = (int) (qg * 0.375f);
    qb = (int) (qb * 0.375f);

    return t & 0xFFFF;
}

static int
_txPixQuantize_AI88( unsigned int argb, int x, int y, int w)
{
    return (
    (((int) (((argb >>16) & 0xFF) * .30F +
                ((argb >> 8) & 0xFF) * .59F +
                ((argb     ) & 0xFF) * .11F + 0.5f )) & 0xFF) |

                ((argb >>16) & 0xFF00)                  );
}


static void
_txImgQuantize(char *dst, char *src, int w, int h, FxU32 format, FxU32 dither)
{
    int (*quantizer)(unsigned int argb, int x, int y, int w) = NULL;
    int         x, y;

    dither &= TX_DITHER_MASK;

    if (dither == TX_DITHER_ERR) { // Error diffusion, floyd-steinberg
        int             i;

        // Clear error diffusion accumulators.
        for (i=0; i<w; i++) errR[i] = errG[i] = errB[i] = 0;

        switch(format) {
        case GR_TEXFMT_RGB_332:         quantizer = _txPixQuantize_RGB332_DErr; 
                                                                break;
        case GR_TEXFMT_A_8:                     quantizer = _txPixQuantize_A8;                  
                                                                break;
        case GR_TEXFMT_I_8:                     quantizer = _txPixQuantize_I8;
                                                                break;
        case GR_TEXFMT_AI_44:           quantizer = _txPixQuantize_AI44_DErr;
                                                                break;
        case GR_TEXFMT_ARGB_8332:       quantizer = _txPixQuantize_ARGB8332_DErr;
                                                                break;
        case GR_TEXFMT_RGB_565:         quantizer = _txPixQuantize_RGB565_DErr; 
                                                                break;
        case GR_TEXFMT_ARGB_1555:       quantizer =     _txPixQuantize_ARGB1555_DErr;
                                                                break;
        case GR_TEXFMT_ARGB_4444:       quantizer = _txPixQuantize_ARGB4444_DErr;
                                                                break;
        case GR_TEXFMT_AI_88:           quantizer = _txPixQuantize_AI88;
                                                                break;

        default: txPanic("Bad case in txQuantize()\n");                         break;
        }
    }else if (dither == TX_DITHER_4x4) { // 4x4 ordered dithering.

        switch(format) {
        case GR_TEXFMT_RGB_332:         quantizer = _txPixQuantize_RGB332_D4x4; 
                                                                break;
        case GR_TEXFMT_A_8:                     quantizer = _txPixQuantize_A8;                  
                                                                break;
        case GR_TEXFMT_I_8:                     quantizer = _txPixQuantize_I8;                          
                                                                break;
        case GR_TEXFMT_AI_44:           quantizer = _txPixQuantize_AI44_D4x4;           
                                                                break;

        case GR_TEXFMT_ARGB_8332:       quantizer = _txPixQuantize_ARGB8332_D4x4;       
                                                                break;
        case GR_TEXFMT_RGB_565:         quantizer = _txPixQuantize_RGB565_D4x4; 
                                                                break;
        case GR_TEXFMT_ARGB_1555:       quantizer =     _txPixQuantize_ARGB1555_D4x4;   
                                                                break;
        case GR_TEXFMT_ARGB_4444:       quantizer = _txPixQuantize_ARGB4444_D4x4;       
                                                                break;
        case GR_TEXFMT_AI_88:           quantizer = _txPixQuantize_AI88;                        
                                                                break;

        default: txPanic("Bad case in txQuantize()\n");                         
                                                                break;
        }
    } else {            // No dithering.

        switch(format) {
        case GR_TEXFMT_RGB_332:         quantizer = _txPixQuantize_RGB332;              
                                                                break;
        case GR_TEXFMT_A_8:                     quantizer = _txPixQuantize_A8;          
                                                                break;
        case GR_TEXFMT_I_8:                     quantizer = _txPixQuantize_I8;                  
                                                                break;
        case GR_TEXFMT_AI_44:           quantizer = _txPixQuantize_AI44;                
                                                                break;
    
        case GR_TEXFMT_ARGB_8332:       quantizer = _txPixQuantize_ARGB8332;    
                                                                break;
        case GR_TEXFMT_RGB_565:         quantizer = _txPixQuantize_RGB565;              
                                                                break;
        case GR_TEXFMT_ARGB_1555:       quantizer =     _txPixQuantize_ARGB1555;        
                                                                break;
        case GR_TEXFMT_ARGB_4444:       quantizer = _txPixQuantize_ARGB4444;    
                                                                break;
        case GR_TEXFMT_AI_88:           quantizer = _txPixQuantize_AI88;                
                                                                break;

        default: txPanic("Bad case in txQuantize()\n");                 
                                                                break;
        }
    }

    if (format < GR_TEXFMT_16BIT) {
        // 8 bit dst
        for (y=0; y<h; y++) {
                for (x=0; x<w; x++) {
                        *dst++ = (*quantizer)(*(unsigned int *)src, x, y, w);
                        src += 4;
                }
        }
    } else {
        // 16 bit dst.
        unsigned short *dst16 = (unsigned short *) dst;

        for (y=0; y<h; y++) {
                for (x=0; x<w; x++) {
                        *dst16++ = (*quantizer)(*(unsigned int *)src, x, y, w);
                        src += 4;
                }
        }
    }
}

/*
 * Reduce an ARGB8888 image to 16bits or 8bits/pixel, possibly dithering
 * the resulting image using either ordered 4x4 or error-diffusion dithering.
 *
 * For the special cases of YIQ image, you also get the choice of 2 different
 * quality levels in each of the compression cases.
 */
void    
txMipQuantize(TxMip *pxMip, TxMip *txMip, int format, FxU32 dither, FxU32 compression)
{
    int         i, w, h;

    if( txVerbose )
      {
        printf("Quantizing: (to %s)", Format_Name[format]);
      }
    pxMip->format = format;
    pxMip->width  = txMip->width;
    pxMip->height = txMip->height;

    switch(format) {
    // Special cases.
    case GR_TEXFMT_YIQ_422:
    case GR_TEXFMT_AYIQ_8422:
                if( txVerbose )
                  printf(".\n");
                txMipNcc(pxMip, txMip, format, dither, compression);
                return;

    case GR_TEXFMT_ARGB_8888: 
                // Copy source to destination, and be done.
                if( txVerbose )
                  printf(".\n");
                memcpy(pxMip->data[0], txMip->data[0], txMip->size);
                return;

    case GR_TEXFMT_P_8:
    case GR_TEXFMT_AP_88:
                if( txVerbose )
                  printf(".\n");
                txMipPal256(pxMip, txMip, format, dither, compression);
                return;

    // Normal cases
    case GR_TEXFMT_A_8: 
    case GR_TEXFMT_I_8: 
    case GR_TEXFMT_AI_44: 
    case GR_TEXFMT_RGB_332:
    case GR_TEXFMT_RGB_565: 
    case GR_TEXFMT_ARGB_8332: 
    case GR_TEXFMT_ARGB_1555: 
    case GR_TEXFMT_ARGB_4444:
    case GR_TEXFMT_AI_88:
                break;

    // Bad cases
    default:
                txPanic("Bad data format in Quantize\n");
                return;

    }

    // We deal with rest of them here one mipmap level at a time.

    w = txMip->width;
    h = txMip->height;

    for (i=0; i< pxMip->depth; i++) {
        if( txVerbose )
          printf(" %dx%d", w, h);

        _txImgQuantize(pxMip->data[i], txMip->data[i], w, h, format, dither);
        w >>= 1; if (w == 0) w = 1;
        h >>= 1; if (h == 0) h = 1;
    }
    if( txVerbose )
      printf(".\n");
}
