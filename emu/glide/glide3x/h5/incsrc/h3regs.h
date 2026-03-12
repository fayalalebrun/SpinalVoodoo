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
** *****************  Version 35  *****************
** User: Cwilcox      Date: 9/08/99    Time: 4:21p
** Updated in $/devel/h5/Win9x/dx/inc
** Added protection for H2INC, after merge with \devel\h5\incsrc.
** 
** *****************  Version 34  *****************
** User: Cwilcox      Date: 9/08/99    Time: 3:53p
** Updated in $/devel/h5/Win9x/dx/inc
** Add leftDesktopBuf, tChromaKeyMin, and tChromaKeyMax registers.
** 
** *****************  Version 33  *****************
** User: Devil        Date: 7/22/99    Time: 3:38p
** Updated in $/devel/h5/incsrc
** Added full support for multi-chip rawlfb reads/writes
** 
** *****************  Version 32  *****************
** User: Devil        Date: 7/02/99    Time: 1:27p
** Updated in $/devel/h5/incsrc
** Support for new PCI config space registers and new lfbMemoryConfig
** register
** 
** *****************  Version 31  *****************
** User: Devil        Date: 7/01/99    Time: 1:51p
** Updated in $/devel/h5/incsrc
** Changed PCI config space from using nice structed to big pile of
** memory.
** This is necessary because the structure doesn't work properly when
** using Big Endian processors
** 
** *****************  Version 30  *****************
** User: Devil        Date: 6/29/99    Time: 10:09p
** Updated in $/devel/ape/incsrc
** Random changes including:
** Addition of real PCI config space
** Work on multi-chip configuration
** Lots of random debugging
** 
** *****************  Version 29  *****************
** User: Devil        Date: 5/26/99    Time: 4:55p
** Updated in $/devel/ape/incsrc
** Interim SLI/AA check-in. Need to copy stuff to Solaris side. 
** 
** *****************  Version 28  *****************
** User: Goran        Date: 4/20/99    Time: 3:49p
** Updated in $/devel/WinSIM/Device/Napalm/Include
** 
** *****************  Version 27  *****************
** User: Devil        Date: 3/12/99    Time: 12:06p
** Updated in $/devel/ape/incsrc
** Changed SLI so that it supports more than 2 chips and banding
** 
** *****************  Version 26  *****************
** User: Zelsnack     Date: 12/02/98   Time: 10:10a
** Updated in $/devel/ape/incsrc
** added combineMode register
** 
** *****************  Version 25  *****************
** User: Tarolli      Date: 11/23/98   Time: 8:26p
** Updated in $/devel/ape/incsrc
** removed tChromaMin tChromaMax registers
** 
** *****************  Version 24  *****************
** User: Zelsnack     Date: 11/23/98   Time: 3:34p
** Updated in $/devel/ape/incsrc
** Added comment about split format of z, dxdz, dydz in 32bpp versus
** 16bpp
** 
** *****************  Version 23  *****************
** User: Zelsnack     Date: 11/23/98   Time: 9:42a
** Updated in $/devel/ape/incsrc
** Checking in for Gary
** 
** *****************  Version 22  *****************
** User: Zelsnack     Date: 11/13/98   Time: 10:15a
** Updated in $/devel/ape/incsrc
** Added stencil buffer change from Gary
** 
** *****************  Version 20  *****************
** User: Michael      Date: 9/08/98    Time: 4:16p
** Updated in $/devel/h3/Win95/dx/inc
** MarkL's fix for 2281.  Modify members of "stats" structure from ints to
** longs.
*/

#ifdef _H2INC
typedef unsigned int   FxU32;
#endif
//----------------- SST chip I/O layout -------------------------
// I/O registers remapped into memory space
// Includes init, dac/pll, video, and VGA registers
typedef volatile struct sstioregs {     

  // Init Registers
    FxU32 status;                       // aliased status register
    FxU32 pciInit0;
    FxU32 sipMonitor;
    FxU32 lfbMemoryConfig;    //This is more or less a dummy variable now.
                              //It really just controls access to the
                              //lfbMemoryTileCtrl and lfbMemoryTileCompare registers
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
    FxU32 strapInfo;
    FxU32 vidTvOutBlankVCount;

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
    FxU32 vidTvOutBlankHCount;
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

  // The following registers don't exist in IO space where
  // their offset is in this structure.
    FxU32 lfbMemoryTileCtrl;
    FxU32 lfbMemoryTileCompare;
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

#ifndef _H2INC

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
    FxI32 z;             // 20.12 in 16bpp, 28.4 in 32bpp (there is an ugly hack in csimio.c, search "//EVIL:")
    FxI32 a;             // 12.12
    FxI32 s;             // 14.18
    FxI32 t;             // 14.18
    FxI32 w;             //  2.30

    FxI32 drdx;                  // X Gradients
    FxI32 dgdx;
    FxI32 dbdx;
    FxI32 dzdx;  //20.12 in 16bpp, 28.4 in 32bpp (there is an ugly hack in csimio.c, search "//EVIL:")
    FxI32 dadx;
    FxI32 dsdx;
    FxI32 dtdx;
    FxI32 dwdx;

    FxI32 drdy;                  // Y Gradients
    FxI32 dgdy;
    FxI32 dbdy;
    FxI32 dzdy;  //20.12 in 16bpp, 28.4 in 32bpp (there is an ugly hack in csimio.c, search "//EVIL:")
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
    FxU32 zaColor;              // 8.24
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

    FxU32 renderMode;		// new 32bpp and 1555 modes
    FxU32 stencilMode;
    FxU32 stencilOp;
    FxU32 colBufferAddr;        //This is the primary colBufferAddr
    FxU32 colBufferStride;    
    FxU32 auxBufferAddr;        //This is the primary auxBufferAddr
    FxU32 auxBufferStride;
    FxU32 fbiStencilFail;

    FxU32 clipLeftRight1;
    FxU32 clipBottomTop1;
    FxU32 combineMode;
    FxU32 sliCtrl;
    FxU32 aaCtrl;
    FxU32 chipMask;
    FxU32 leftDesktopBuf;
    FxU32 reservedD[2];         // NOTE: used to store TMUprivate ptr  (reservedD[0])
                                        // NOTE: used to store CSIMprivate ptr (reservedD[1])

    FxU32 reservedE[7];         // NOTE: reservedE[0] stores the secondary colBufferAddr
                                        // NOTE: reservedE[1] stores the secondary auxBufferAddr  
                                        // NOTE: reservedE[2] stores the primary colBufferAddr  
                                        // NOTE: reservedE[3] stores the primary auxBufferAddr  
                      
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

#if GLIDE_USE_DEBUG_FIFO
    unsigned long cmdFifoDepth;         // command fifo depth
#endif /* GLIDE_USE_DEBUG_FIFO */

} SstRegs;


//All data types need to be 32bit to insure proper operation on both
//little and big endian processors.
typedef volatile struct
{
  FxU32       deviceID_vendorID;    //0	15:0	3dfx Interactive Vendor Identification
	                            //2	15:0	Device Identification
  FxU32       status_command;       //4	15:0	PCI bus configuration
     	                            //6	15:0	PCI device status
  FxU32       classCode_revisionID; //8	7:0	Revision Identification
          	                    //9	23:0	Generic functional description of PCI device
  FxU32       BIST_headerType_latencyTimer_cacheLineSize;    //12	7:0	Bus Master Cache Line Size
                                                             //13	7:0	Bus Master Latency Timer
                                                             //14	7:0	PCI Header Type
                                                             //15	7:0	Build In Self-Test Configuration
  FxU32       memBaseAddr0;	//16	31:0	Memory Base Address (Init/3D/2D regs)
  FxU32       memBaseAddr1;	//20	31:0	Memory Base Address (LFB)
  FxU32       ioBaseAddr;	//24	31:0	I/O Base Address
  FxU32       reservedA[4];	//28-43		Reserved
  FxU32       subSystemID_subVendorID;	//44-45	15:0	Subsystem Vendor ID
                                	//46-47	15:0	Subsystem ID
  FxU32       romBaseAddr;	//48	31:0	Expansion Rom Base Address
  FxU32       capabilitesPtr;	//52	31:0	Pointer to start of New Capabilities structure [7:0]
  FxU32       reservedB;        //56-59		Reserved
  FxU32       maxiumumLatency_minimumGrant_interruptPin_interruptLine;	//60	7:0	Interrupt Mapping
                                                                     	//61	7:0	External Interrupt Connections
                                                                  	//62	7:0	Bus Master Minimum Grant Time
                                                    	                //63	7:0	Bus Master Maximum Latency Time
  FxU32       cfgInitEnable_FabID;    //64	7:0	Fab Identification
                                      //65-67		Misc. configuration control
  FxU32       acpiReset;       	//68-71	31:0	ACPI Reset
  FxU32       cfgPciDecode;	//72-75	31:0	PCI decode logic control
  FxU32       cfgStatus;        //76	31:0	Aliased memory-mapped status register
  FxU32       cfgScratch; 	//80	31:0	Scratch pad register
  FxU32       agpCapabilities;  //84	31:0	AGP Capability identifier register  (read only)
  FxU32       agpStatus;	//88	31:0	AGP status register  (read only)
  FxU32       agpCommand; 	//92	31:0	AGP command register (read / write)
  FxU32       acpiCapabilites;  //96	31:0	ACPI Capability identifier register (read only)
  FxU32       acpiControlStatus;//100	31:0	ACPI Control and Status register (read / write)
  FxU32       reservedC[6];	//104-127		Reserved
  FxU32       cfgVideoCtrl0;	//128-131	31:0	Enhanced video control register #0
  FxU32       cfgVideoCtrl1;	//132-135	31:0	Enhanced video control register #1
  FxU32       cfgVideoCtrl2;	//136-139	31:0	Enhanced video control register #2
  FxU32       cfgSliLfbCtrl;	//140-143	31:0	Linear frame buffer access control for SLI
  FxU32       cfgAADepthBufferAperture;  //144-147	31:0	Tiled aperture specification for SLI/AA lfb accesses
  FxU32       cfgAALfbCtrl;	//148-151	31:0	Linear frame buffer access control for Anti-Aliasing
  FxU32       agpTestCtrl;	//152-155	31:0	AGP Test control
  FxU32       agpTestData0;	//156-159	31:0	AGP Test Data 0
  FxU32       agpTestData1;	//160-163	31:0	AGP Test Data 1
  FxU32       agpTestData2;	//164-167	31:0	AGP Test Data 2
  FxU32       agpTestData3;	//168-171	31:0	AGP Test Data 3
  FxU32       cfgSliAAMisc;     //172-175       31:0    Miscellaneous control for SLI and AA
  FxU32       reservedD[20];	//176-255	n/a	Reserved
} SstPCIConfigRegs;

#endif // _H2INC

#endif /* !__H3REGS_H__ */
