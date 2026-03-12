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

#ifndef __TEXUSINT_H__
#define __TEXUSINT_H__
 
#include "texus.h"

#define GR_TEXFMT_SIZE(x)                               ((x < GR_TEXFMT_16BIT) ? 1:     \
                                                                                 ((x < GR_TEXFMT_32BIT) ? 2: 4))

#define TX_OFORMAT_3DF                                  10
#define TX_OFORMAT_TGA                                  11
#define TX_OFORMAT_PPM                                  12

#define TX_MAX_LEVEL       16
typedef struct  _TxMip {
        int             format;                         // GR_TEXFMT_...
        int             width;                          // pixels
        int             height;                         // pixels
        int             depth;                          // mipmap levels
        int             size;                           // bytes
        void    *data[TX_MAX_LEVEL];
        FxU32   pal[256];
} TxMip;

FxBool  txMipRead(TxMip *txMip, const char *filename, int preferredFormat);
FxBool  txMipReadFromFP(TxMip *txMip, const char *debug_filename, FILE *file, int preferredFormat);
void    txMipWrite(TxMip *txMip, char *file, char *ext, int split);
void    txMipResample(TxMip *destMip, TxMip *srcMip);
void    txMipClamp( TxMip *dstMip, TxMip *srcMip );
void    txMipMipmap(TxMip *txMip);

void    txMipQuantize(TxMip *pxMip, TxMip *txMip, int fmt, FxU32 d, FxU32 comp);
void    txMipNcc(TxMip *pxMip, TxMip *txMip, int fmt, FxU32 dither, FxU32 comp);
void    txMipNccNNet(TxMip *pxMip, TxMip *txMip, int fmt, FxU32 dither, FxU32 comp);
int     txMipPal256(TxMip *pxMip, TxMip *txMip, int fmt, FxU32 dither, FxU32 comp);

void    txMipDequantize(TxMip *txMip, TxMip *pxMip);
void    txMipView(TxMip *txMip, char *filename, int wait, int bgcolor);
void    txViewClose();

int             txLog2(int n);
int     txFloorPow2(int n);
int             txCeilPow2(int n);
int     txGCD(int a, int b);
int             txAspectRatio(int w, int h);
void    txPanic(char *);
void    txError(char *);
void    txYABtoPal256(int *palette, const int* yabTable);
void    txRectCopy(FxU8 *dst, int dstStride, const FxU8 *src, int srcStride,
                        int width, int height);
FxBool  txMipAlloc(TxMip *txMip);
FxBool  txMipSetMipPointers(TxMip *txMip);
int             txMemRequired(TxMip *txMip);
void    txBasename(const char *name, char *base);
void    txPathAndBasename(const char *name, char* pathbase);
void    txExtension(const char *name, char *ext);

void txMipFree( TxMip *mip );
void txMipTrueToFixedPal( TxMip *outputMip, TxMip *trueColorMip, const FxU32 *pal,
                          FxU32 flags );



extern  int txVerbose;
extern  int *explode3;
#define DISTANCE(ar, ag, ab, br, bg, bb) \
                ((explode3[(ar)-(br)] << 1) + (explode3[(ag)-(bg)]<<2) + explode3[(ab)-(bb)])

void    txDiffuseIndex(TxMip *pxMip, TxMip *txMip, int pixsize, 
                const FxU32 *palette, int       ncolors);
int             txNearestColor(int ir, int ig, int ib, const FxU32 *pal, int npal);

FxBool _txReadTGAHeader( FILE *stream, FxU32 cookie, TxMip *info);
FxBool _txReadTGAData( FILE *stream, TxMip *info);

FxBool _txReadRGTHeader( FILE *stream, FxU32 cookie, TxMip *info);
FxBool _txReadRGTData( FILE *stream, TxMip *info);

FxBool _txReadSBIHeader( FILE *stream, FxU32 cookie, TxMip *info);
FxBool _txReadSBIData( FILE *stream, TxMip *info);

FxBool _txReadPPMHeader( FILE *stream, FxU32 cookie, TxMip *info);
FxBool _txReadPPMData( FILE *stream, TxMip *info);

FxBool _txRead3DFHeader( FILE *stream, FxU32 cookie, TxMip *info);
FxBool _txRead3DFData( FILE *stream, TxMip *info);

int _txReadHeader( FILE *stream, TxMip *info );

void txPalToNcc( GuNccTable *ncc_table, const FxU32 *pal );
void txNccToPal( FxU32 *pal, const GuNccTable *ncc_table );

#ifdef GLIDE3
#define MAX_TEXWIDTH    2048
#else
#define MAX_TEXWIDTH    1024
#endif

extern TxErrorCallbackFnc_t _txErrorCallback;

extern  char *Format_Name[];

#endif  /* __TEXUSINT_H__ */
