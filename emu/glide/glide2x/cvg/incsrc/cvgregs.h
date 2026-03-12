#ifndef __CVGREGS_H__
#define __CVGREGS_H__

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


//----------------- SST chip 3D layout -------------------------
// registers are in groups of 8 for easy decode
typedef struct vertex_Rec {
    unsigned long x;            // 12.4 format
    unsigned long y;            // 12.4
} vtxRec;

typedef volatile struct sstregs {       // THE 3D CHIP
                                        // EXTERNAL registers
    FxU32 status;               // chip status, Read Only
    FxU32 intrCtrl;             // interrupt control 
    vtxRec vA;                  // Vertex A,B,C
    vtxRec vB;
    vtxRec vC;

    long r;             // 12.12        Parameters
    long g;             // 12.12
    long b;             // 12.12
    long z;             // 20.12
    long a;             // 12.12
    long s;             // 14.18
    long t;             // 14.18
    long w;             //  2.30

    long drdx;                  // X Gradients
    long dgdx;
    long dbdx;
    long dzdx;
    long dadx;
    long dsdx;
    long dtdx;
    long dwdx;

    long drdy;                  // Y Gradients
    long dgdy;
    long dbdy;
    long dzdy;
    long dady;
    long dsdy;
    long dtdy;
    long dwdy;

    unsigned long triangleCMD;  // execute a triangle command (float)
    unsigned long reservedA;
    vtxRec FvA;                 // floating point version
    vtxRec FvB;
    vtxRec FvC;

    long Fr;                    // floating point version
    long Fg;
    long Fb;
    long Fz;
    long Fa;
    long Fs;
    long Ft;
    long Fw;

    long Fdrdx;
    long Fdgdx;
    long Fdbdx;
    long Fdzdx;
    long Fdadx;
    long Fdsdx;
    long Fdtdx;
    long Fdwdx;

    long Fdrdy;
    long Fdgdy;
    long Fdbdy;
    long Fdzdy;
    long Fdady;
    long Fdsdy;
    long Fdtdy;
    long Fdwdy;

    unsigned long FtriangleCMD;         // execute a triangle command
    unsigned long fbzColorPath;         // color select and combine
    unsigned long fogMode;              // fog Mode
    unsigned long alphaMode;            // alpha Mode
    unsigned long fbzMode;              // framebuffer and Z mode
    unsigned long lfbMode;              // linear framebuffer Mode
    unsigned long clipLeftRight;        // (6)10(6)10
    unsigned long clipBottomTop;        // (6)10(6)10

    unsigned long nopCMD;       // execute a nop command
    unsigned long fastfillCMD;  // execute a fast fill command
    unsigned long swapbufferCMD;// execute a swapbuffer command
    unsigned long fogColor;             // (8)888
    unsigned long zaColor;              // 8(8)16
    unsigned long chromaKey;            // (8)888
    unsigned long chromaRange;
    unsigned long reservedB;

    unsigned long stipple;              // 32 bits, MSB masks pixels
    unsigned long c0;                   // 8.8.8.8 (ARGB)
    unsigned long c1;                   // 8.8.8.8 (ARGB)
    struct {                            // statistic gathering variables
        unsigned int fbiPixelsIn;
        unsigned int fbiChromaFail;
        unsigned int fbiZfuncFail;
        unsigned int fbiAfuncFail;
        unsigned int fbiPixelsOut;
    } stats;

    unsigned long fogTable[32];         // 64 entries, 2 per word, 2 bytes each

    FxU32 cmdFifoBase;                  // beginning of CMDFIFO area
    FxU32 cmdFifoBump;                  // number of words to bump (write only)
    FxU32 cmdFifoReadPtr;               // the read (execute) pointer
    FxU32 cmdFifoAmin;                  // points to first hole
    FxU32 cmdFifoAmax;                  // highest address written (not visible)
    FxU32 cmdFifoDepth;                 // number of valid sequential words
    FxU32 cmdFifoHoles;                 // number of current holes
    unsigned long reservedC;

    unsigned long fbiInit4;
    unsigned long vRetrace;
    unsigned long backPorch;
    unsigned long videoDimensions;
    unsigned long fbiInit0;
    unsigned long fbiInit1;
    unsigned long fbiInit2;
    unsigned long fbiInit3;

    unsigned long hSync;
    unsigned long vSync;
    unsigned long clutData;
    unsigned long dacData;
    unsigned long videoFilterRgbThreshold;
    FxU32 hBorder;
    FxU32 vBorder;
    FxU32 borderColor;

    FxU32 hvRetrace;
    FxU32 fbiInit5;
    FxU32 fbiInit6;
    FxU32 fbiInit7;
    unsigned long reservedD[2];
    FxU32 fbiSwapHistory;
    FxU32 fbiTrianglesOut;              // triangles out counter

    FxU32 sSetupMode;
    FxU32 sVx;
    FxU32 sVy;
    FxU32 sARGB;
    FxU32 sRed;
    FxU32 sGreen;
    FxU32 sBlue;
    FxU32 sAlpha;

    FxU32 sVz;
    FxU32 sOowfbi;
    FxU32 sOow0;
    FxU32 sSow0;
    FxU32 sTow0;
    FxU32 sOow1;
    FxU32 sSow1;
    FxU32 sTow1;

    FxU32 sDrawTriCMD;
    FxU32 sBeginTriCMD;
    unsigned long reservedE[6];

    FxU32 bltSrcBaseAddr;
    FxU32 bltDstBaseAddr;
    FxU32 bltXYstrides;
    FxU32 bltSrcChromaRange;
    FxU32 bltDstChromaRange;
    FxU32 bltClipX;
    FxU32 bltClipY;
    FxU32 reservedF;

    FxU32 bltSrcXY;
    FxU32 bltDstXY;
    FxU32 bltSize;
    FxU32 bltRop;
    FxU32 bltColor;
    FxU32 reservedG;
    FxU32 bltCommand;
    FxU32 bltData;

    unsigned long textureMode;          // texture Mode
    unsigned long tLOD;                 // texture LOD settings
    unsigned long tDetail;              // texture detail settings
    unsigned long texBaseAddr;          // current texture base address
    unsigned long texBaseAddr1;
    unsigned long texBaseAddr2;
    unsigned long texBaseAddr38;
    unsigned long trexInit0;            // hardware init bits

    unsigned long trexInit1;            // hardware init bits
    unsigned long nccTable0[12];        // NCC decode tables, bits are packed
    unsigned long nccTable1[12];        // 4 words Y, 4 words I, 4 words Q
    FxU32 tchromaKey;                   // texture chromakey
    FxU32 tchromaRange;                 // texture chromarange
    FxU32 reservedH[5];
    FxU32 reservedI[8];
    FxU32 reservedJ[8];
    FxU32 reservedK[8];
} SstRegs;

#endif /* !__CVGREGS_H__ */

