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
** User: Sapphire     Date: 3/16/99    Time: 7:28p
** Created in $/Releases/Voodoo3/V3_OEM_100/3dfx/devel/H3/incsrc
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
typedef unsigned int   FxU32;
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
    FxU32 userIntrCmd;

    FxU32 stipple;              // 32 bits, MSB masks pixels
    FxU32 c0;                   // 8.8.8.8 (ARGB)
    FxU32 c1;                   // 8.8.8.8 (ARGB)
    struct {                            // statistic gathering variables
        FxU32 fbiPixelsIn;
        FxU32 fbiChromaFail;
        FxU32 fbiZfuncFail;
        FxU32 fbiAfuncFail;
        FxU32 fbiPixelsOut;
    } stats;

    FxU32 fogTable[32];         // 64 entries, 2 per word, 2 bytes each

    FxU32 reservedB[3];

    FxU32 colBufferAddr;
    FxU32 colBufferStride;
    FxU32 auxBufferAddr;
    FxU32 auxBufferStride;
    FxU32 reservedC;

    FxU32 clipLeftRight1;
    FxU32 clipBottomTop1;
    FxU32 reservedD[6];         // NOTE: used to store TMUprivate ptr
                                        

    FxU32 reservedE[8];

    FxU32 reservedF[3];  
    FxU32 swapBufferPend;
    FxU32 leftOverlayBuf;
    FxU32 rightOverlayBuf;
    FxU32 fbiSwapHistory;
    FxU32 fbiTrianglesOut;      // triangles out counter

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
    FxU32 reservedG[6];

    FxU32 reservedH[8];

    FxU32 reservedI[8];

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

    FxU32 tChromaKeyMin;
    FxU32 tChromaKeyMax;
} SstRegs;

// STB Begin changes
#endif // #ifndef _H2INC
// STB End changes

#endif /* !__H3REGS_H__ */
