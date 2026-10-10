#ifndef __H3REGS_H__
#define __H3REGS_H__

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
** $History: h3regs.h $
** 
** *****************  Version 1  *****************
** User: Sapphire     Date: 2/17/99    Time: 8:26p
** Created in $/Releases/Voodoo3/MT2/3Dfx/devel/H3/INCSRC
** 
** *****************  Version 21  *****************
** User: Stb_srogers  Date: 2/09/99    Time: 9:55a
** Updated in $/devel/h3/win95/dx/inc
** 
** *****************  Version 20  *****************
** User: Michael      Date: 9/08/98    Time: 4:16p
** Updated in $/devel/h3/Win95/dx/inc
** MarkL's fix for 2281.  Modify members of "stats" structure from ints to
** longs.
*/

// STB Begin Changes
// STB-SR 1/13/98 Adding code for bj
#ifdef _H2INC
typedef unsigned long   FxU32;
#endif
// STB End Changes

//----------------- SST chip I/O layout -------------------------
// I/O registers remapped into memory space
// Includes init, dac/pll, video, and VGA registers
typedef volatile struct sstioregs {     

  // Init Registers
    FxU32 status;                       // aliased status register
    FxU32 pciInit0;
    FxU32 sipMonitor;
    FxU32 lfbMemoryConfig;
    FxU32 miscInit0;
    FxU32 miscInit1;
    FxU32 dramInit0;
    FxU32 dramInit1;
    FxU32 agpInit;
    FxU32 tmuGbeInit;
    FxU32 vgaInit0;
    FxU32 vgaInit1;
    FxU32 dramCommand;
    FxU32 dramData;
    FxU32 reservedZ[2];

  // PLL Registers
    FxU32 pllCtrl0;
    FxU32 pllCtrl1;
    FxU32 pllCtrl2;

  // DAC Registers
    FxU32 dacMode;
    FxU32 dacAddr;
    FxU32 dacData;

  // Video Registers I
    FxU32 vidMaxRGBDelta;
    FxU32 vidProcCfg;
    FxU32 hwCurPatAddr;
    FxU32 hwCurLoc;
    FxU32 hwCurC0;
    FxU32 hwCurC1;
    FxU32 vidInFormat;
    FxU32 vidInStatus;
    FxU32 vidSerialParallelPort;
    FxU32 vidInXDecimDeltas;
    FxU32 vidInDecimInitErrs;
    FxU32 vidInYDecimDeltas;
    FxU32 vidPixelBufThold;
    FxU32 vidChromaMin;
    FxU32 vidChromaMax;
    FxU32 vidCurrentLine;
    FxU32 vidScreenSize;
    FxU32 vidOverlayStartCoords;
    FxU32 vidOverlayEndScreenCoord;
    FxU32 vidOverlayDudx;
    FxU32 vidOverlayDudxOffsetSrcWidth;
    FxU32 vidOverlayDvdy;

  // VGA Registers
    FxU32 vgaRegister[12];

  // Video Registers II
    FxU32 vidOverlayDvdyOffset;
    FxU32 vidDesktopStartAddr;
    FxU32 vidDesktopOverlayStride;
    FxU32 vidInAddr0;
    FxU32 vidInAddr1;
    FxU32 vidInAddr2;
    FxU32 vidInStride;
    FxU32 vidCurrOverlayStartAddr;
} SstIORegs;

typedef volatile struct cmdfifo {
    FxU32 baseAddrL;
    FxU32 baseSize;
    FxU32 bump;
    FxU32 readPtrL;
    FxU32 readPtrH;
    FxU32 aMin;
    FxU32 unusedA;
    FxU32 aMax;
    FxU32 unusedB;
    FxU32 depth;
    FxU32 holeCount;
    FxU32 reserved;
} CmdFifo;

//---------- SST chip AGP/CMD Transfer/Misc Register layout ------------------
typedef volatile struct sstcregs {
  // AGP
    FxU32 agpReqSize;
    FxU32 hostAddrLow;
    FxU32 hostAddrHigh;
    FxU32 graphicsAddr;
    FxU32 graphicsStride;
    FxU32 moveCMD;
    FxU32 reservedL[2];

  // CMD FIFO 0,1
    CmdFifo cmdFifo0;
    CmdFifo cmdFifo1;

    FxU32 cmdFifoThresh;
    FxU32 cmdHoleInit;
    FxU32 reservedO[6];
    FxU32 reservedP[8];
    FxU32 reservedQ[8];
    FxU32 reservedR[8];
  // misc
    FxU32 yuvBaseAddr;
    FxU32 yuvStride;
    FxU32 reservedS[6];
    FxU32 crc1;
    FxU32 reservedT[3];
    FxU32 crc2;
} SstCRegs;

//----------------- SST chip 2D layout -------------------------
typedef volatile struct sstgregs {      // THE 2D CHIP
    FxU32 status;                       // aliased status register
    FxU32 unused0;
    FxU32 clip0min;
    FxU32 clip0max;
    FxU32 dstBaseAddr;
    FxU32 dstFormat;
    FxU32 srcColorkeyMin;
    FxU32 srcColorkeyMax;
    FxU32 dstColorkeyMin;
    FxU32 dstColorkeyMax;
    FxU32 bresError0;
    FxU32 bresError1;
    FxU32 rop;
    FxU32 srcBaseAddr;
    FxU32 commandEx;
    FxU32 lineStipple;
    FxU32 lineStyle;
    FxU32 pattern0alias;
    FxU32 pattern1alias;
    FxU32 clip1min;
    FxU32 clip1max;
    FxU32 srcFormat;
    FxU32 srcSize;
    FxU32 srcXY;
    FxU32 colorBack;
    FxU32 colorFore;
    FxU32 dstSize;
    FxU32 dstXY;
    FxU32 command;
    FxU32 reserved[3];
    FxU32 launch[32];
    FxU32 colorPattern[64];
#if COLORTRANSLUT
    FxU32 colorTransLut[256];
#endif
} SstGRegs;

// STB Begin changes
// STB-SR 1/13/99 Adding code for bj
#ifndef _H2INC
// STB End changes

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
    unsigned long userIntrCmd;

    unsigned long stipple;              // 32 bits, MSB masks pixels
    unsigned long c0;                   // 8.8.8.8 (ARGB)
    unsigned long c1;                   // 8.8.8.8 (ARGB)
    struct {                            // statistic gathering variables
        unsigned long fbiPixelsIn;
        unsigned long fbiChromaFail;
        unsigned long fbiZfuncFail;
        unsigned long fbiAfuncFail;
        unsigned long fbiPixelsOut;
    } stats;

    unsigned long fogTable[32];         // 64 entries, 2 per word, 2 bytes each

    unsigned long reservedB[3];

    unsigned long colBufferAddr;
    unsigned long colBufferStride;
    unsigned long auxBufferAddr;
    unsigned long auxBufferStride;
    unsigned long reservedC;

    unsigned long clipLeftRight1;
    unsigned long clipBottomTop1;
    unsigned long reservedD[6];         // NOTE: used to store TMUprivate ptr
                                        

    unsigned long reservedE[8];

    unsigned long reservedF[3];  
    unsigned long swapBufferPend;
    unsigned long leftOverlayBuf;
    unsigned long rightOverlayBuf;
    unsigned long fbiSwapHistory;
    unsigned long fbiTrianglesOut;      // triangles out counter

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
    unsigned long reservedG[6];

    unsigned long reservedH[8];

    unsigned long reservedI[8];

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

    unsigned long tChromaKeyMin;
    unsigned long tChromaKeyMax;
} SstRegs;

// STB Begin changes
#endif // #ifndef _H2INC
// STB End changes

#endif /* !__H3REGS_H__ */
