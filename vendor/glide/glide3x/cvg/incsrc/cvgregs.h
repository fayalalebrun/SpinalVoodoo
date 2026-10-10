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
    FxU32 x;            // 12.4 format
    FxU32 y;            // 12.4
} vtxRec;

typedef volatile struct sstregs {       // THE 3D CHIP
                                        // EXTERNAL registers
    FxU32 status;               // chip status, Read Only
    FxU32 intrCtrl;             // interrupt control 
    vtxRec vA;                  // Vertex A,B,C
    vtxRec vB;
    vtxRec vC;

    FxI32 r;             // 12.12        Parameters
    FxI32 g;             // 12.12
    FxI32 b;             // 12.12
    FxI32 z;             // 20.12
    FxI32 a;             // 12.12
    FxI32 s;             // 14.18
    FxI32 t;             // 14.18
    FxI32 w;             //  2.30

    FxI32 drdx;                  // X Gradients
    FxI32 dgdx;
    FxI32 dbdx;
    FxI32 dzdx;
    FxI32 dadx;
    FxI32 dsdx;
    FxI32 dtdx;
    FxI32 dwdx;

    FxI32 drdy;                  // Y Gradients
    FxI32 dgdy;
    FxI32 dbdy;
    FxI32 dzdy;
    FxI32 dady;
    FxI32 dsdy;
    FxI32 dtdy;
    FxI32 dwdy;

    FxU32 triangleCMD;  // execute a triangle command (float)
    FxU32 reservedA;
    vtxRec FvA;                 // floating point version
    vtxRec FvB;
    vtxRec FvC;

    FxI32 Fr;                    // floating point version
    FxI32 Fg;
    FxI32 Fb;
    FxI32 Fz;
    FxI32 Fa;
    FxI32 Fs;
    FxI32 Ft;
    FxI32 Fw;

    FxI32 Fdrdx;
    FxI32 Fdgdx;
    FxI32 Fdbdx;
    FxI32 Fdzdx;
    FxI32 Fdadx;
    FxI32 Fdsdx;
    FxI32 Fdtdx;
    FxI32 Fdwdx;

    FxI32 Fdrdy;
    FxI32 Fdgdy;
    FxI32 Fdbdy;
    FxI32 Fdzdy;
    FxI32 Fdady;
    FxI32 Fdsdy;
    FxI32 Fdtdy;
    FxI32 Fdwdy;

    FxU32 FtriangleCMD;         // execute a triangle command
    FxU32 fbzColorPath;         // color select and combine
    FxU32 fogMode;              // fog Mode
    FxU32 alphaMode;            // alpha Mode
    FxU32 fbzMode;              // framebuffer and Z mode
    FxU32 lfbMode;              // linear framebuffer Mode
    FxU32 clipLeftRight;        // (6)10(6)10
    FxU32 clipBottomTop;        // (6)10(6)10

    FxU32 nopCMD;       // execute a nop command
    FxU32 fastfillCMD;  // execute a fast fill command
    FxU32 swapbufferCMD;// execute a swapbuffer command
    FxU32 fogColor;             // (8)888
    FxU32 zaColor;              // 8(8)16
    FxU32 chromaKey;            // (8)888
    FxU32 chromaRange;
    FxU32 reservedB;

    FxU32 stipple;              // 32 bits, MSB masks pixels
    FxU32 c0;                   // 8.8.8.8 (ARGB)
    FxU32 c1;                   // 8.8.8.8 (ARGB)
    struct {                            // statistic gathering variables
        unsigned int fbiPixelsIn;
        unsigned int fbiChromaFail;
        unsigned int fbiZfuncFail;
        unsigned int fbiAfuncFail;
        unsigned int fbiPixelsOut;
    } stats;

    FxU32 fogTable[32];         // 64 entries, 2 per word, 2 bytes each

    FxU32 cmdFifoBase;                  // beginning of CMDFIFO area
    FxU32 cmdFifoBump;                  // number of words to bump (write only)
    FxU32 cmdFifoReadPtr;               // the read (execute) pointer
    FxU32 cmdFifoAmin;                  // points to first hole
    FxU32 cmdFifoAmax;                  // highest address written (not visible)
    FxU32 cmdFifoDepth;                 // number of valid sequential words
    FxU32 cmdFifoHoles;                 // number of current holes
    FxU32 reservedC;

    FxU32 fbiInit4;
    FxU32 vRetrace;
    FxU32 backPorch;
    FxU32 videoDimensions;
    FxU32 fbiInit0;
    FxU32 fbiInit1;
    FxU32 fbiInit2;
    FxU32 fbiInit3;

    FxU32 hSync;
    FxU32 vSync;
    FxU32 clutData;
    FxU32 dacData;
    FxU32 videoFilterRgbThreshold;
    FxU32 hBorder;
    FxU32 vBorder;
    FxU32 borderColor;

    FxU32 hvRetrace;
    FxU32 fbiInit5;
    FxU32 fbiInit6;
    FxU32 fbiInit7;
    FxU32 reservedD[2];
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
    FxU32 reservedE[6];

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

    FxU32 textureMode;          // texture Mode
    FxU32 tLOD;                 // texture LOD settings
    FxU32 tDetail;              // texture detail settings
    FxU32 texBaseAddr;          // current texture base address
    FxU32 texBaseAddr1;
    FxU32 texBaseAddr2;
    FxU32 texBaseAddr38;
    FxU32 trexInit0;            // hardware init bits

    FxU32 trexInit1;            // hardware init bits
    FxU32 nccTable0[12];        // NCC decode tables, bits are packed
    FxU32 nccTable1[12];        // 4 words Y, 4 words I, 4 words Q
    FxU32 tchromaKey;                   // texture chromakey
    FxU32 tchromaRange;                 // texture chromarange
    FxU32 reservedH[5];
    FxU32 reservedI[8];
    FxU32 reservedJ[8];
    FxU32 reservedK[8];
} SstRegs;

#endif /* !__CVGREGS_H__ */

