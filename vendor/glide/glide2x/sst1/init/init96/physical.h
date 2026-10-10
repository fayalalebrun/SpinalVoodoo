/////////////////////////////////////////////////////////////////////////////
//
// PHYSICAL.H
//
// This module ...  
//
// Copyright 1994-1996 Alliance Semiconductor Corporation
//
/////////////////////////////////////////////////////////////////////////////


#include "prefresh.h"           /* include refresh structures */

#ifdef VG96
#define DYNAMIC_MCLK TRUE       /* Set Mclk based on Puma On/Off State */
#endif


#pragma pack(1)
#undef DCI
#define DCI TRUE
#undef DIRECT_DRAW
#define DIRECT_DRAW TRUE

#undef  HWSWITCH
#define HWSWITCH TRUE           /* can switch HW on/off thru escap */
#define PMREGISTRY FALSE        /*  Read driver switches from registry. */

enum {
  CHIP_3210   = 0x3210,
  CHIP_6422   = 0x6422,
  CHIP_6412   = 0x6412,
  CHIP_AT24   = 0x6424,
  CHIP_AT3D   = 0x643D 
};      

#define MC_BETTER_HALF 0x080

#define VC_ENABLE                       0x0001
#define VC_DISABLE                      0x0000
#define VCPD_8BPP                       0x0004
#define VCPD_15BPP                      0x0008
#define VCPD_16BPP                      0x000A
#define VCPD_32BPP                      0x000E
#define VCWF_INDEXED                    0x0000
#define VCWF_RGB                        0x0020
#define VCWF_YUY2                       0x0030
#define VCWF_UYVY                       0x0040
#define VCWF_YYVU                       0x0050
#define VCWF_YVYU                       0x0060
#define VCWF_Y                          0x0070
#define VC_YUV_RGB                      0x0100
#define VC_STRETCH                      0x0200
#define VC_INTERP                       0x0400
#define VC_NOFLICKER                    0x0800
#define VC2_HOR_INTERP                  0x0400
#define VC2_VER_INTERP                  0x0800
#define VC2_FILTER                      0x1000

#define TILE_RIGHTMOST                  0x10

#define N_OVERLAYS_VERS1                1
#define N_OVERLAYS_VERS2                2
#define MAX_TILES                       12


typedef struct _VIDWINDOW_V1{
  FxU16    VidControl;
  FxU16    VidDataWidth;
  FxU16    VidDataOffset;
  FxU16    VidLeft;
  FxU16    VidTop;
  FxU16    VidRight;
  FxU16    VidBottom;
  FxU32   VidDataLocation;
  FxU32 VidChromaKey;
  FxU16    VidStretchHorz1;
  short VidStretchHorz2;
  FxU16    VidStretchVert1;
  short VidStretchVert2;
} VIDWINDOW_V1;

typedef struct _VIDWINDOW_V2{
  FxU16    VidControl;			/* 0x82 */
  FxU16    VidDataOffset;		/* 0x84 */
  FxU16    VidScaleFactorHor;		/* 0x86 */
  FxU16    VidScaleOffsetHor;		/* 0x88 */
  FxU16    VidScaleFactorVer;		/* 0x8a */
  FxU16    VidScaleOffsetVer;		/* 0x8c */
  FxU16    TileSequenceControl;		/* 0x8e */
  FxU16 VidChromakeyColor;
} VIDWINDOW_V2;

typedef struct _VIDWINDOWV2_SET{
 VIDWINDOW_V2 VidWindowV2Set[N_OVERLAYS_VERS2];
} VIDWINDOWV2_SET;


typedef struct _TILEREGS{
  FxU16 Control;    			/* 0x200 */
  FxU16 DisplayPositionLeft;  		/* 0x202 */
  FxU16 DisplayPositionRight;		/* 0x204 */
  FxU16 DisplayPositionBottom;		/* 0x208 */
  FxU16 DataWidth;     			/* 0x20a */
  FxU16 DataLocationLo; 		/* 0x20c */
  FxU8 DataLocationHi; 			/* 0x20e */
  char Reserved[3]; 
} TILEREGS;

typedef struct {
  char    spare0_2F[0x30];	/* 0x0 */
  FxU8    CLIP_CTRL;		/* 0x30 */
  char    spare31_3F[0xF];	/* 0x31 */
  FxU32   BLT_CTRL;		/* 0x40 */
  FxU8    spare44_45[0x2];	/* 0x44 */
  FxU8    BLT_ROP;		/* 0x46 */
  FxU8    FxU8_MASK;		/* 0x47 */
  FxU8    PATTERN_REG[0x8];	/* 0x48 */
  FxU32    SRC;			/* 0x50 */
  FxU32    DEST;		/* 0x54 */
  FxU32   DIM;			/* 0x58 */
  union{
    FxU32   ROW_PITCHES;
    struct{
      FxU16            DEST_ROW_PITCH;
      FxU16            SRC_ROW_PITCH;
    };
  };				/* 0x5c */
  FxU32   FG_CLR;		/* 0x60 */
  FxU32   BG_CLR;		/* 0x64 */
  FxU8    spare68_70[0x70-0x68]; /* 0x68 */
  FxU16    DDA_AXIAL;		/* 0x70 */
  FxU16    DDA_DIAGONAL;	/* 0x72 */
  FxU16    DDA_ERROR;		/* 0x74 */
  FxU8    spare76_80[0x80-0x76]; /* 0x76 */
  FxU8    SerialControl;	/* 0x80 */
  FxU8    spare_1;		/* 0x81 */
  union {			/* 0x82 */
    VIDWINDOW_V1 V1;
    VIDWINDOWV2_SET V2;
  };
  char    spareC4_A2[0xC4-0xA2];
  FxU16  MemConfig;             /*  0xC4-5 */
  FxU8    DACControl;           /*  0xc6 */
  char  spare_c7;               /*  0xc7 */
  FxU16    VGAOverride;         /*  0xC8-9 */
  FxU8    PinInterface;         /*  0xCA */
  char    spareCB;              /* 0xcb */
  FxU8    FeatureCnctrDisable;  /* 0xcc */
  FxU8    GenFeatureCnctrCtl;   /* 0xcd */
  FxU32   spareCE_D1;           /* 0xce,0xcf,0xd0,0xd1 */
  FxU8    InterlaceControl;     /*  0xD2 */
  FxU8    Spare3b;              /* 0xd3 */
  FxU8    HiReqMiss;            /* 0xD4 */
  FxU8    HiReqNoMiss;          /* 0xd5 */
  FxU8    LoReq;                /* 0xd6 */
  FxU8    Spare3a;              /* 0xd7 */
  FxU8    ExternalClock;        /* 0xD8 */
  FxU8    ExternalSignalTiming; /* 0xd9 */
  FxU8    CommandFifoControl;   /* 0xda */
  FxU8    ExtendedMemoryEnable; /*  0xDB */
  FxU16    BiEndianControl;     /* 0xdc,0xdd */
  FxU8    DE;                   /* 0xde */
  FxU8    DF;                   /* 0xdf */
  FxU8    RamdacControl;        /* 0xe0 */
  FxU8    E1;                   /* 0xe1 */
  FxU8    E2;                   /* 0xe2 */
  FxU8    E3;                   /* 0x33 */
  FxU8    DACControl2;          /* 0x34 */
  FxU8    OverdriveRed;         /* 0xE5 */
  FxU8    OverdriveGreen;       /* 0xE6 */
  FxU8    OverdriveBlue;        /* 0xE7 */
  FxU8    MCLK_Ctrl;            /* 0xe8 */
  FxU8    MCLK_M;               /* 0xe9 */
  FxU8    MCLK_N;               /* 0xea */
  /*  config register for MCLK control by HW straps, does not affect
      clock directly. No longer used. */ 
  FxU8    MCLK_SpeedSel;        /* 0xeb */
  FxU8    VCLK_Ctrl;            /* 0xec */
  FxU8    VCLK_M;               /* 0xed */
  FxU8    VCLK_N;               /* 0xee */
  char    spare4a;              /* 0xef */
  FxU32   ScratchReg;           /* 0xf0, 0xf1, 0xf2, 0xf3 */
  char    spare5[0x110 - 0xF4];
  FxU8    UMAControl;
  FxU8    UMASpare;
  FxU8    UMASlaveRequest;
  FxU8    UMASlaveGrant;
  char    spare5a[0x141 - 0x114];
  FxU8    CursColor1;
  FxU8    CursColor2;
  char    spare8[0x190 - 0x143];
  FxU32 BaseAddress;
  char    spare6[0x1F0 - 0x194];
  FxU8    GPIOControl;          /* 0x1f0 */
  FxU8    GPIODirection;        /* 0x1f1 */
  FxU8    GPIOLevel;            /* 0x1f2 */
  FxU8    GPIOReadback;         /* 0x1f3 */
  FxU16    SerialInput;		/* 0x1f4 */
  char    spare6a[0x1FA - 0x1F6];
  FxU16    CurrentVerticalPosition; /* 0x1fa */
  union{
    FxU32 Extended_Status;
    FxU8    cFIFOEntries;
  };				/* 0x1fc */
  TILEREGS TileRegsSet[MAX_TILES]; /* 0x200 */
  char    tiles_spare[0x300 - ( MAX_TILES*sizeof(TILEREGS) + 0x200) ];
  
  /*  3D registers */
  FxU32 PolygonEngineControl;	/* 0x300 */
  FxU32 PolygonEngineControl2;  /*  for HW debug */
  FxU32 TextureMapBaseAddress;
  FxU16 TextureFormat;
  FxU8 TexelIndexOffset;
  FxU8 TextureMipmapSwitch;
  FxU32 _3d_register_index; 
  FxU32 _3d_register_data; 
  FxU32 Zbuffer_base_pointer; 
  FxU16 Zbuffer_front_clipping_plane; 
  FxU16 Zbuffer_back_clipping_plane; 
  FxU32 TexelTransparencyColor;
  FxU32 FogColor;
  FxU32 BackBufferBaseAddress;
  char _3d_spare2[0x330-0x32C];
  union{
    struct {
      FxU16 ClipLeft_3D;
      FxU16 ClipTop_3D;
    };
    FxU32   ClipLeftTop_3D;
  };
  union{
    struct {
      FxU16 ClipRight_3D;
      FxU16 ClipBottom_3D;
    };
    FxU32   ClipRightBottom_3D;
  };
  char _3d_spare3[0x340-0x338];
  FxU32 Vertex_X;
  FxU32 Vertex_Y;
  FxU32 Vertex_Z;
  union{
    FxU32 Vertex_W;
    struct{
      FxU8 PadW1;
      FxU8 Vertex_Red;
      FxU16 PadW2;
    };
  };
  FxU8 Vertex_L;
  char _3d_spare5[0x353-0x351];
  FxU8    Vertex_Alpha;
  FxU8 Vertex_F;
  char _3d_spare6[0x358-0x355];
  union{
    FxU32 Vertex_U;
    struct{
      FxU16 PadU1;
      FxU8 Vertex_Green;
      FxU8 PadU2;
    };
  };
  union{
    FxU32 Vertex_V;
    struct{
      FxU16 PadV1;
      FxU8 Vertex_Blue;
      FxU8 PadV2;
    };
  };
  char    spare360_3C0[0x3C0 - 0x360];
  
  union{
    FxU32   UFactorOffset;
    struct{
      FxU16    UFactor;
      FxU16    UOffset;
    };
  };
  
  union{
    FxU32 VFactorOffset;
    struct{
      FxU16    VFactor;
      FxU16    VOffset;
    };
  };
  
  char    GradReinterpCount;
  char    spare9[0x1000 - 0x3C9];
  FxU32 BltPort;
} COP;
 
typedef volatile COP *LPCOP;

#define CC_INDEX 0x3d4
#define CC_DATA  0x3d5

enum{
  CC_HTOTAL,
  CC_HDISP_END,
  CC_HBLANK_START,
  CC_HBLANK_END,
  CC_HSYNC_START,
  CC_HSYNC_END,
  CC_VTOTAL,
  CC_VOVERFLOW,
  CC_MAX_SCANLINE = 9,
  CC_SERIAL_START_MSB = 0xC,
  CC_SERIAL_START_LSB = 0xD,
  CC_VSYNC_START = 0x10,
  CC_VSYNC_END, 
  CC_VDISP_END,
  CC_VBLANK_START = 0x15,
  CC_VBLANK_END,
  CC_GXX_VOVERFLOW = 0x1A,
  CC_GXX_SERIAL_OVERFLOW = 0x1C,
  CC_GXX_EXT_CRTC_CTRL = 0x1E
};

/*  VGA Sequencer register defines */
#define SC_INDEX 0x3c4
#define SC_DATA  0x3c5

enum{
        SC_RESET,
        SC_CLOCKING_MODE,
        SC_MAP_MASK,
        SC_CHAR_MAP,
        SC_MEM_MODE
};

#define SC_REMAP_CTRL 0x1B

#define VCLK_CTRL_MASK 3
#define VGA_SEQUENCER_SCREEN_OFF 0x20

#define MISC_OUTPUTW 0x3c2
#define MISC_OUTPUTR 0x3cc

#define PCI_BUS 1

#define DAC_ATT_8                       00h
#define DAC_ATT_16                      81h
#define DAC_SIERRA                      02h

/* New Promotion - specific defines */
#define CLIP_ENABLE                     0x1
#define INPUT_STATUS_0                  0x3C2
#define INPUT_STATUS_1                  0x3DA
#define VERTICAL_RETRACE_ACTIVE         0x08
#define DISPLAY_INACTIVE                0x01


#define EXT_STATUS_DWG_FIFO_ENTRIES     0x0000000F
#define ST_IN_BLT                       0x00000100
#define EXT_STATUS_DWG_ENG_BUSY         0x00000400
#define EXT_STATUS_3D_ENG_BUSY          0x00800000

#define ENOUGH_FIFO_SPACE_FOR_BLIT      0x4


#define PM_SCRATCH_SPACE 20*1024

/*  Graphics Engine Command reg. defines.  */

#define GE_BLT_SS                       0x000000001
#define GE_BLT_RECT                     0x000000002
#define GE_BLT_STRIP                    0x000000004
#define GE_BLT_MS                       0x000000008
#define GE_BLT_SM                       0x000000009
#define GE_BLT_LINE                     0x00000000C
#define GE_BLT_LINE_NOEND               0x00000000D
#define GE_BLT_DIR_L                    0x000000040
#define GE_BLT_DIR_U                    0x000000080
#define GE_BLT_DIR_LU                   (GE_BLT_DIR_L + GE_BLT_DIR_U)
#define GE_MAJ_AXIS_Y                   0x000000100
#define GE_SRC_LINEAR                   0x000000A00
#define GE_SRC_VAR_PITCH                0x000000200
#define GE_SRC_PATTERN                  0x000000400
#define GE_SRC_MONO                     0x000001000
#define GE_SRC_XPARENT                  0x000002000
#define GE_PIX_DEPTH_16                 0x000008000
#define GE_24BPP                        0x000010000
#define GE_VAR_STRIDE_BPP               0x000014000
#define GE_DST_CONTIGUOUS               0x000080000
#define GE_DST_LINEAR                   0x0000C0000
#define GE_DST_VAR_PITCH                0x000040000
#define GE_VAR_PITCH                    0x000060200
#define GE_PAT_COLOR                    0x000400000
#define GE_MODEL_640                    0x001000000
#define GE_MODEL_800                    0x002000000
#define GE_MODEL_1024                   0x004000000
#define GE_MODEL_1152                   0x005000000
#define GE_MODEL_1280                   0x006000000
#define GE_MODEL_1600                   0x007000000
#define GE_DEST_UPDATE_TR               0x008000000
#define GE_DEST_UPDATE_BL               0x010000000
#define GE_DEST_UPDATE_LP               0x018000000
#define GE_START_ON_XEXT                0x020000000
#define GE_START_ON_SRCX                0x040000000
#define GE_START_ON_DESTX               0x060000000
#define GE_BLT_START                    0x080000000



#define ALIGN_4_UP(x)                   ((x + 3) & ~3)
#define ALIGN_4_DN(x)                   (x  & ~3)

#define DW_ALIGN_PIXELS                 3


#define DDST                            0xaa
#define SSRC                            0xCC
#define PPAT                            0xCC

#define ROP_D                           (DDST)
#define ROP_DDx                         (DDST ^ DDST)b
#define ROP_DPon                        (~(DDST | PPAT))
#define ROP_DPna                        (DDST & (~ PPAT))
#define ROP_Pn                          (~PPAT)
#define ROP_DSon                        (~(DDST | SSRC))
#define ROP_DSna                        (DDST & (~SSRC))
#define ROP_Sn                          (~SSRC)
#define ROP_DnSa                        (~DDST) & SSRC
#define ROP_DnPa                        (~DDST) & PPAT
#define ROP_Dfn                         (~DDST)
#define ROP_DPx                         (DDST ^ PPAT)
#define ROP_DPan                        (~(DDST & PPAT))
#define ROP_DSx                         (DDST ^ SSRC)
#define ROP_DSan                        (~(DDST & SSRC))
#define ROP_DSa                         (DDST & SSRC)
#define ROP_DSxn                        (~(DDST ^ SSRC))
#define ROP_DSno                        (DDST | (~ SSRC)b)
#define ROP_S                           (SSRC)
#define ROP_DnSo                        ((~DDST) | SSRC)
#define ROP_DSo                         (DDST | SSRC)
#define ROP_DDxn                        (~(DDST ^ DDST))
#define ROP_0                           (DDST ^ DDST)

#define ROP_1                           (~(DDST ^ DDST))
#define ROP_P                           (PPAT)

/*  waits until cNeed FIFO entries are available */
#define FIFOWait(cNeed) while((lpCOP->cFIFOEntries & 0xF) < cNeed);

/*  Additionnal Defines for 2nd version of motion video hardware */

enum{
 HWOVERLAY_V1,
 HWOVERLAY_V2 
};

#define TILE_PRECISION 4096

#define OVLY2_SUCCESS 1
#define OVLY2_FAILURE 0

enum{
 HW2D_V1,
 HW2D_V2 
};

/*  Flags for passing info between 16 and 32 bit sides of Direct Draw. */
#define SI_LINEAR_MEMORY                        0x00000001
#define SI_DRIVER_DESTROYED                     0x00000002
#define SI_ENABLE_HOR_INTERP                    0x00000004
#define SI_ENABLE_VER_INTERP                    0x00000008

/*  3D Polygon Engine Control bits */
#define _3DCTRL_QUICKSTART_POLYLIST             0x00000004
#define _3DCTRL_QUICKSTART_POLYSTRIP            0x00000006
#define _3DCTRL_TLUT_LOAD                       0x00000008
#define _3DCTRL_TEXTURE_ENABLE                  0x00000010
#define _3DCTRL_TEXTURE_JITTER_ENABLE           0x00000020
#define _3DCTRL_TEXTURE_DITHER_ENABLE           0x00000040
#define _3DCTRL_FEEDFORWARD_DITHER_ENABLE       0x00000080
#define _3DCTRL_TEXT_TRANSPARENCY_ENABLE        0x00000100
#define _3DCTRL_GOURAUD_ENABLE                  0x00000200
#define _3DCTRL_LIGHTING_ENABLE                 0x00000400
#define _3DCTRL_FOG_ENABLE                      0x00000800
#define _3DCTRL_DEST_8_INDEXED                  0x00002000
#define _3DCTRL_DEST_8_DIRECT                   0x00003000
#define _3DCTRL_DEST_15_DIRECT                  0x00004000
#define _3DCTRL_DEST_16_DIRECT                  0x00005000
#define _3DCTRL_DEST_32_DIRECT                  0x00007000
#define _3DCTRL_STIPPLED_ALPHA_ENABLE           0x00008000
#define _3DCTRL_ZBUFFER_READ_ENABLE             0x00010000
#define _3DCTRL_ZBUFFER_WRITE_ENABLE            0x00020000
#define _3DCTRL_ZBUFFER_TILED                   0x00040000
#define _3DCTRL_MIPMAP_ENABLE                   0x00080000
#define _3DCTRL_CLIPPING_ENABLE                 0x00100000
#define _3DCTRL_DISABLE_ANTITEARING             0x00200000
#define _3DCTRL_GRADIENT_REINT_ENABLE           0x00800000
#define _3DCTRL_BBCTRL_ANY                      0x02000000
#define _3DCTRL_BBCTRL_BOTH                     0x03000000
#define _3DCTRL_BBCHK_ALL_INV                   0x04000000
#define _3DCTRL_BBCHK_ALL_VIS                   0x08000000
#define _3DCTRL_BBCHK_SOME_VIS                  0x0C000000
#define _3DCTRL_HRES320                         0x00000000
#define _3DCTRL_HRES512                         0x30000000
#define _3DCTRL_HRES640                         0x10000000
#define _3DCTRL_HRES800                         0x20000000
#define _3DCTRL_HRES1024                        0x40000000
#define _3DCTRL_HRES1152                        0x50000000
#define _3DCTRL_HRES1280                        0x60000000
#define _3DCTRL_HRES1600                        0x70000000
#define _3DCTRL_POLYGON_START                   0x80000000

#define _3DCTRL_HRES_MASK                       3DCTRL_HRES1600

/*  3D Polygon Engine Control Register 2 bits */
#define _3DCTRL2_TEXTURE_MIRROR                 0x00000040
#define _3DCTRL2_TEXTURE_CLAMP                  0x00000080
#define _3DCTRL2_SRC_TEXTURE_ALPHA              0x00000200
/* alpha default is 0FFh == opaque */
#define _3DCTRL2_REVERSE_ALPHA                  0x00000400
#define _3DCTRL2_DISABLE_UV_CLAMP               0x08000000

#define _3DCTRL2_ZCMP_NEVER                     0x00000000
#define _3DCTRL2_ZCMP_LESS                      0x00002000
#define _3DCTRL2_ZCMP_EQUAL                     0x00004000
#define _3DCTRL2_ZCMP_LESSEQUAL                 0x00006000
#define _3DCTRL2_ZCMP_GREATER                   0x00008000
#define _3DCTRL2_ZCMP_NOTEQUAL                  0x0000A000
#define _3DCTRL2_ZCMP_GREATEREQUAL              0x0000C000
#define _3DCTRL2_ZCMP_ALWAYS                    0x0000E000

#define _MinIdxGRADIENT                         0x000004
#define _MajIdxZ                                0x000020
#define _MajIdxW                                0x000030
#define _MajIdxL                                0x000040
#define _MajIdxUW                               0x000060
#define _MajIdxVW                               0x000070
#define _WEnableZ                               0x020000
#define _WEnableW                               0x040000
#define _WEnableL                               0x080000
#define _WEnableUW                              0x200000
#define _WEnableVW                              0x400000


/*  3D Texture Format register fields */
#define TEXTURE_4B                              0x1 /*  indexed */
#define TEXTURE_8B                              0x2
#define TEXTURE_15B                             0x4 /*  direct */
#define TEXTURE_16B                             0x5
#define TEXTURE_32B                             0x7 /*  XRGB */
#define TEXTURE_ARGB_4444                       0xB /*  ARGB 4444 */
#define TEXTURE_ARGB_1555                       0xC /*  ARGB 1555 */

#define TEXTURE_WIDTH_8                         0<<5 
#define TEXTURE_WIDTH_16                        1<<5  
#define TEXTURE_WIDTH_32                        2<<5  
#define TEXTURE_WIDTH_64                        3<<5  
#define TEXTURE_WIDTH_128                       4<<5  
#define TEXTURE_WIDTH_256                       5<<5  

#define TEXTURE_HEIGHT_8                        0<<8 
#define TEXTURE_HEIGHT_16                       1<<8  
#define TEXTURE_HEIGHT_32                       2<<8  
#define TEXTURE_HEIGHT_64                       3<<8  
#define TEXTURE_HEIGHT_128                      4<<8  
#define TEXTURE_HEIGHT_256                      5<<8  

/*  Texture Look-up Table (TLUT) defines */
#define TLUT_LINEAR_OFFSET 0xFFF400             /* location with 16MB PCI memory space */
#define N_TLUT_ENTRIES 256
#define TLUT_ENTRY FxU32
#define  MAKE_TLUT_DATA(Red,Green,Blue) ((FxU32)(Red) << 16)  |  ((FxU32)(Green) << 8)  |  (FxU32)(Blue)  

/*  ExtendedMemoryEnable register Bits */
#define ENABLE_EXTENDED_MEMORY_DOS              0x1
#define ENABLE_EXTENDED_MEMORY_LINEAR           0x2
#define ENABLE_EXTENDED_MEMORY_COPROC           0x4
#define ENABLE_EXTENDED_MEMORY_SECOND_AP        0x8

/*  PUMA defines */

#define PUMA_CONTROL_OFF                        0x0
#define PUMA_CONTROL_SFB                        0x1
#define PUMA_CONTROL_ON                         0x3

/*  GPIO defines */
#define GPIO_PUMA_SRESET_N                      0x10
#define GPIO_PUMA_SWAP                          0x40

/*  VGA Override defines */
#define VGA_OVERRIDE_DISABLE_DRAM_REFRESH       0x800

/*  Mem Config Register defines */
#define MEMCONFIG_ENABLE_SINGLE_CYCLE           0x400


#pragma pack()
