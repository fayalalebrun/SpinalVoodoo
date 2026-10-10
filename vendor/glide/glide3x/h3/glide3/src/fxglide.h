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
** Revision 1.2.4.14  2006/01/16 21:22:42  jwrdegoede
** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
**
** Revision 1.2.4.13  2005/06/10 14:17:54  jwrdegoede
** Fix compilation when GL_X86 is not defined
**
** Revision 1.2.4.12  2005/05/25 08:56:23  jwrdegoede
** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
**
** Revision 1.2.4.11  2005/05/25 08:53:22  jwrdegoede
** Add P6FENCE (ish) macro for non-x86 archs
**
** Revision 1.2.4.10  2005/05/25 08:51:49  jwrdegoede
** Add #ifdef GL_X86 around x86 specific code
**
** Revision 1.2.4.9  2004/10/07 07:48:50  dborca
** comment the GR_CDECL hack to prevent accidents
**
** Revision 1.2.4.8  2004/10/07 07:16:38  dborca
** corrected grDrawTriangle on MSVC
**
** Revision 1.2.4.7  2004/10/05 14:47:15  dborca
** conditional compilation a bit more sane
**
** Revision 1.2.4.6  2004/10/04 09:26:30  dborca
** DOS/OpenWatcom support
**
** Revision 1.2.4.5  2004/02/16 07:42:16  dborca
** grSetNumPendingBuffers visible with grGetProcAddress
**
** Revision 1.2.4.4  2003/11/07 13:38:38  dborca
** unite the clans
**
** Revision 1.2.4.3  2003/07/25 07:13:41  dborca
** removed debris
**
** Revision 1.2.4.2  2003/06/29 18:45:55  guillemj
** Fixed preprocessor invalid token errors.
**
** Revision 1.2.4.1  2003/05/05 07:12:46  dborca
** no message
**
** Revision 1.2  2000/11/24 18:36:48  alanh
** Add new grStippleMode and grStipplePattern functions for both Voodoo3 and
** Voodoo5 hardware.
**
** Revision 1.1.1.1  1999/11/24 21:44:56  joseph
** Initial checkin for SourceForge
**
** 
** 9     5/24/99 2:48p Jamesb
** Added ptrLostContext to exported cmdTransport struct.
** 
** 8     5/05/99 1:22p Stb_mmcclure
** Allow grTexMinAddress to be called before WinOpen since
** at the point of break, gc is valid, but the sub-structures are not, so
** we report a value of 0 back.
** 
** 7     4/16/99 9:11a Atai
** added texture_uma_mode
** 
** 110   4/09/99 4:22p Dow
** Fixes for lost surfaces
** 
** 109   4/07/99 7:18p Atai
** added uma extension
** 
** 108   4/06/99 2:43p Dow
** Let alt-tab and surface extension live in peace
** 
** 107   4/05/99 8:25p Dow
** Alt tab mostly happy
** 
** 106   4/04/99 8:51p Atai
** Partial check-in for alt-tab issue. set FX_GLIDE_ALT_TAB=1 to build
** glide3x with hwcQueryContext built into GR_BEGIN_NOFIFOCHECK. It works
** with DEBUG glide only. In the non-debug glide, we can still see the
** desktop corruption.
** 
** 104   4/01/99 7:55p Peter
** made names and comments more meaningful
** 
** 103   3/31/99 9:02p Dow
** context loosing means no writing to hw
** 
** 102   3/30/99 3:20p Atai
** place holder for texture cache and fifofree defs
** 
** 101   3/24/99 6:17p Peter
** removed rle cruft
** 
** 100   3/19/99 3:18p Peter
** warning about fifo extension
** 
** 99    3/19/99 11:53a Peter
** re-ordered struct for exposing to gl
** 
** 98    3/19/99 11:26a Peter
** expose direct fifo for gl
** 
** 97    3/14/99 1:48p Peter
** really invoke gggsoph
** 
** 96    3/11/99 6:38p Dow
** Resolution help
** 
** 95    3/11/99 6:07p Dow
** Upped GR_MAX_RESOLUTION
** 
** 94    3/10/99 10:42a Peter
** detect katmai-ness
** 
** 93    3/09/99 12:49p Kcd
** oops.
** 
** 92    3/09/99 12:33p Kcd
** Added committedSerialNumebr to command transport struct.
** 
** 91    2/23/99 3:10p Peter
** h3 has a direct gamma table
** 
** 90    2/19/99 8:03p Peter
** new splash crapola
** 
** 89    2/18/99 5:56p Peter
** cleaned up more of the random texture tables
** 
** 88    2/18/99 4:06p Kcd
** Faster getThreadValueFast for MacOS.
** 
** 87    2/18/99 3:38p Kcd
** Mac happiness.
** 
** 86    2/11/99 1:38p Atai
** sync buffer swap pending code, the right way.
** 
** 85    2/09/99 2:08p Atai
** change buffer size to 4
** 
** 84    2/08/99 2:16p Peter
** single buffering is allowed
** 
** 83    2/02/99 4:38p Peter
** 16 byte texture alignmnet boundary
** 
** 82    1/25/99 6:35p Peter
** tiled texture cleanup
** 
** 81    1/20/99 10:54a Dow
** Voodoo 3 id for apps
** 
** 80    1/15/99 10:52a Peter
** cleanup lazy evaluation of fbzMode for grDepthMask and grColorMask
** 
** 79    1/14/99 7:48p Peter
** cleanedup bytes per texel stuff
** 
** 78    1/06/99 11:30a Peter
** cleanup trinalge dispatch code
** 
** 77    1/04/99 12:03p Peter
** use window contexts for sending state
** 
** 76    12/23/98 2:01p Peter
** nt currently has mutexing problems via ddraw and extescape
** 
** 75    12/14/98 6:19p Dow
** Fixed for current surface extension spec
** 
** 74    12/11/98 1:37p Peter
** thread tracing
** 
** 73    12/07/98 11:33a Peter
** norbert's re-fixes of my merge
** 
** 72    12/03/98 9:37a Dow
** Added reserved enumerant for RAM type
** 
** 71    11/30/98 6:57p Peter
** video memory fifo's
** 
** 70    11/21/98 10:19a Atai
** fixed test37 grChromaRangeModeExt error and rename functions
** 
** 69    11/15/98 3:21a Atai
** first attempt to make 2 tmus work in H4 glide3x full screen mode, just
** in time check-in for comdex demo. warning: the code is not completed
** yet.
** 
** 68    11/09/98 5:07p Dow
** Texturing from tiled rendered surfaces works now
** 
** 67    11/02/98 5:34p Peter
** tls per thread for fullscreen contexts
** 
** 66    10/20/98 8:28p Peter
** shared files and fixed whackage
** 
** 65    10/14/98 3:38p Dow
** Gamma stuff
** 
** 64    10/14/98 1:47p Jdt
** Add texture chroma to hw state restore buffer.
** 
** 63    10/13/98 5:27p Peter
** 6666 format hack
** 
** 62    10/12/98 9:51a Peter
** dynamic 3DNow!(tm)
** 
** 61    10/09/98 3:32p Atai
** move texchroma key shadow
** 
** 60    10/09/98 2:44p Atai
** fixed 6666 palette
** 
** 59    9/30/98 12:59p Atai
** added texchromakey and texchromarange for shadow state
** 
** 58    9/11/98 10:45p Jdt
** Switch over to statically allocated in-memory fifo.
** 
** 57    9/04/98 11:35a Peter
** re-open fix for nt (thanks to taco/rob/nt bob)
** 
** 56    8/31/98 10:33a Peter
** asm w/ debugging
** 
** 55    8/30/98 10:54p Jdt
** promote INVALIDATE macro to global dominion
** 
** 54    8/30/98 1:34p Dow
** State & other optimizations
** 
** 53    8/29/98 8:12p Dow
** Clip optimization
** 
** 52    8/29/98 4:34p Dow
** thread optimization stuff
** 
** 51    8/27/98 9:27p Atai
** fix env variable for glide3x
** 
** 50    7/01/98 8:40a Jdt
** removed gc arg from trisetup functions
** 
** 49    8/03/98 6:34a Jdt
** Changes for multi-thread
** 
** 48    8/02/98 5:01p Dow
** Glide Surface Extension
** 
** 46    7/21/98 7:41p Jdt
** fixed palettes
** 
** 45    7/18/98 1:45p Jdt
** Removed TACO_MEMORY_FIFO_HACK
** 
** 44    7/18/98 12:25a Jdt
** Some clean up and new shadow register structure to support state
** restoration.
**
*/

/*
** fxglide.h
**
** Internal declarations for use inside Glide.
**
** GLIDE_LIB:        Defined if building the Glide Library.  This macro
**                   should ONLY be defined by a makefile intended to build
**                   GLIDE.LIB or glide.a.
**
** GLIDE_NUM_TMU:    Number of physical TMUs installed.  Valid values are 1
**                   and 2.  If this macro is not defined by the application
**                   it is automatically set to the value 2.
**
*/

#ifndef __FXGLIDE_H__
#define __FXGLIDE_H__

/* -----------------------------------------------------------------------
   INCLUDE FILES
   ----------------------------------------------------------------------- */
/* standard */
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* 3dfx */
#include <3dfx.h>
#include <glidesys.h>
#include <gdebug.h>
#include <h3.h>

/* local */
#ifdef __WATCOMC__
/* [dBorca]
 * gross hack to prevent Watcom appending @0 to names;
 * this is dangerous if those functions have parameters
 * and are called from inside the asm specializations...
 * You have been warned!
 */
#define GR_CDECL __cdecl
#else
#define GR_CDECL
#endif
#include "g3ext.h"
#include "fxcmd.h"
#include "gsfc.h"

/* conditional */

#if defined( GLIDE_INIT_HAL )
#include <fxhal.h>
#else
#include <minihwc.h>
#define HWC_BASE_ADDR_MASK 0x03UL
#endif /* defined ( GLIDE_INIT_HAL ) */

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define WIN32_LEANER_AND_MEANER
#include <windows.h>
#include "fxsplash.h"
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */

/* -----------------------------------------------------------------------
   Manifest Constants
   ----------------------------------------------------------------------- */
#define MAX_NUM_SST                  4
#define MAX_NUM_CONTEXTS             16

#define VOODOO_GAMMA_TABLE_SIZE      256
#define SST1_BITS_DEPTH              16
#define SST1_ZDEPTHVALUE_NEAREST     0xFFFF
#define SST1_ZDEPTHVALUE_FARTHEST    0x0000
#define SST1_WDEPTHVALUE_NEAREST     0x0000
#define SST1_WDEPTHVALUE_FARTHEST    0xFFFF

/* Internal define so that we don't have to use the stupid reserved
 * bits identifier in the code since no one knows what that means.
 */
#define GR_TEXFMT_P_8_RGBA           GR_TEXFMT_RSVD0

#define GR_MAX_RESOLUTION   0x17
#define GR_MAX_REFRESH      8
#define GR_MAX_COLOR_BUF    3
#define GR_MAX_AUX_BUF      1
#define GR_MIN_RESOLUTION   0
#define GR_MIN_REFRESH      0
#define GR_MIN_COLOR_BUF    1
#define GR_MIN_AUX_BUF      0

#define GR_AA_ORDERED_OGL               0x00010000
#define GR_AA_ORDERED_POINTS_OGL        GR_AA_ORDERED_OGL+1
#define GR_AA_ORDERED_LINES_OGL         GR_AA_ORDERED_OGL+2
#define GR_AA_ORDERED_TRIANGLES_OGL     GR_AA_ORDERED_OGL+3

#define GR_AA_ORDERED_POINTS_MASK       0x01
#define GR_AA_ORDERED_LINES_MASK        0x02
#define GR_AA_ORDERED_TRIANGLES_MASK    0x04

/* -----------------------------------------------------------------------
   Code Macros
   ----------------------------------------------------------------------- */
#undef  GETENV
#define GETENV getenv

/* -----------------------------------------------------------------------
   Internal Enumerated Types
   ----------------------------------------------------------------------- */
typedef int GrSstType;
#define GR_SSTTYPE_VOODOO    0
#define GR_SSTTYPE_SST96     1
#define GR_SSTTYPE_AT3D      2
#define GR_SSTTYPE_Voodoo2   3
#define GR_SSTTYPE_Banshee   4
#define GR_SSTTYPE_Voodoo3   5

/* -----------------------------------------------------------------------
   Internal Structures
   ----------------------------------------------------------------------- */
typedef struct {
    GrScreenResolution_t resolution;
    FxU32                xres;
    FxU32                yres;
} ResEntry;

typedef struct GrTMUConfig_St {
  int    tmuRev;                /* Rev of Texelfx chip */
  int    tmuRam;                /* 1, 2, or 4 MB */
} GrTMUConfig_t;

typedef struct GrVoodooConfig_St {
  int    fbRam;                         /* 1, 2, or 4 MB */
  int    fbiRev;                        /* Rev of Pixelfx chip */
  int    nTexelfx;                      /* How many texelFX chips are there? */
  FxBool sliDetect;                     /* Is it a scan-line interleaved board? */
  GrTMUConfig_t tmuConfig[GLIDE_NUM_TMU];   /* Configuration of the Texelfx chips */
} GrVoodooConfig_t;

typedef struct GrSst96Config_St {
  int   fbRam;                  /* How much? */
  int   nTexelfx;
  GrTMUConfig_t tmuConfig;
} GrSst96Config_t;

typedef GrVoodooConfig_t GrVoodoo2Config_t;

typedef struct GrAT3DConfig_St {
  int   rev;
} GrAT3DConfig_t;

typedef struct {
  int num_sst;                  /* # of HW units in the system */
  struct {
    GrSstType type;             /* Which hardware is it? */
    union SstBoard_u {
      GrVoodooConfig_t  VoodooConfig;
      GrSst96Config_t   SST96Config;
      GrAT3DConfig_t    AT3DConfig;
      GrVoodoo2Config_t Voodoo2Config;
    } sstBoard;
  } SSTs[MAX_NUM_SST];          /* configuration for each board */
} GrHwConfiguration;

/*
** -----------------------------------------------------------------------
** STUFF FOR STRIPS
** -----------------------------------------------------------------------
*/

#define GR_COLOR_OFFSET_RED     (0 << 2)
#define GR_COLOR_OFFSET_GREEN   (1 << 2)
#define GR_COLOR_OFFSET_BLUE    (2 << 2)
#define GR_COLOR_OFFSET_ALPHA   (3 << 2)

#define GR_VERTEX_OFFSET_X      (0 << 2)
#define GR_VERTEX_OFFSET_Y      (1 << 2)
#define GR_VERTEX_OFFSET_Z      (2 << 2)
#define GR_VERTEX_OFFSET_WFBI   (3 << 2)

#define GR_TEXTURE_OFFSET_S     (0 << 2)
#define GR_TEXTURE_OFFSET_T     (1 << 2)
#define GR_TEXTURE_OFFSET_W     (2 << 2)

#define GR_DLIST_END            0x00
#define GR_VTX_PTR              0x00
#define GR_VTX_PTR_ARRAY        0x01
#define GR_SCALE_OOW            0x00
#define GR_SCALE_COLOR          0x01
#define GR_SCALE_STW            0x02

typedef struct {
  FxU32
    mode;                       /* enable / disable */
  FxI32
    offset;                     /* offset to the parameter data */
} GrVParamInfo;

typedef struct  {
  GrVParamInfo
    vertexInfo,          /* xy */
    zInfo,               /* z(ooz) */
    wInfo,               /* w(oow) */
    aInfo,               /* a float */
    fogInfo,             /* fog */
    rgbInfo,             /* rgb float */
    pargbInfo,           /* pargb byte */
    st0Info,             /* st0 */
    st1Info,             /* st1 */
    qInfo,               /* q */
    q0Info,              /* q0 */
    q1Info;              /* q1 */
  FxU32
    vStride,             /* vertex stride */
    vSize;               /* vertex size */
  FxU32
   colorType;           /* float or byte */
} GrVertexLayout;

/*============================================================
 **  State Monster Stuff:
 **============================================================*/
#define GR_FLUSH_STATE() \
  if (gc->state.invalid) _grValidateState();
      

/* Look in distate.c:grValidateState (NOTVALID macro) to see how these
   are used I wanted to keep the mixed-case register names here, and
   that's why they are mixed case */
#define alphaModeBIT            FXBIT(0)
#define fbzColorPathBIT         FXBIT(1)
#define fbzModeBIT              FXBIT(2)
#define chromaKeyBIT            FXBIT(3)
#define clipRegsBIT             FXBIT(4)
#define zaColorBIT              FXBIT(5)
#define fogModeBIT              FXBIT(6)
#define fogColorBIT             FXBIT(7)
#define lfbModeBIT              FXBIT(8)
#define c0c1BIT                 FXBIT(9)
#define chromaRangeBIT          FXBIT(10)
#define stippleBIT              FXBIT(11)
/*
** lazy evaluate vertexlayout.
** it is not part of the registers so we add the bit in MSB
*/ 
#define vtxlayoutBIT            FXBIT(31)

/*============================================================
 **  Video Stuff:
 **============================================================*/
#define VRETRACEMASK            0x00000fff
#define HRETRACEPOS             16


/*--------------------------------------------------------------------------
  State Restoration Buffer
  --------------------------------------------------------------------------*/
typedef struct {
#define OOII_IIOO_OIII_IIII  0x3C7f
#define SR_MASK_0            OOII_IIOO_OIII_IIII
#define SR_ADDR_0            ((FxU32) &((( SstRegs* )0)->fbzColorPath))
  FxU32 pkt4Hdr_0;

  FxU32 fbzColorPath;    /* 0x104 (  0 ) */
  FxU32 fogMode;         /* 0x108 (  1 ) */
  FxU32 alphaMode;       /* 0x10C (  2 ) */
  FxU32 fbzMode;         /* 0x110 (  3 ) */
  FxU32 lfbMode;         /* 0x114 (  4 ) */
  FxU32 clipLeftRight;   /* 0x118 (  5 ) */
  FxU32 clipBottomTop;   /* 0x11C (  6 ) */
  /* space */
  FxU32 fogColor;        /* 0x12C ( 10 ) */
  FxU32 zaColor;         /* 0x130 ( 11 ) */
  FxU32 chromaKey;       /* 0x134 ( 12 ) */
  FxU32 chromaRange;     /* 0x138 ( 13 ) */
  /* ----------- end packet -------------*/

#define OOOO_OOOO_OOOO_OIII  0x0007
#define SR_MASK_1            OOOO_OOOO_OOOO_OIII
#define SR_ADDR_1            ((FxU32) &((( SstRegs* )0)->stipple))
  FxU32 pkt4Hdr_1;

  FxU32 stipple;         /* 0x140 ( 15 ) */
  FxU32 color0;          /* 0x144 ( 16 ) */
  FxU32 color1;          /* 0x148 ( 17 ) */
  /* ----------- end packet -------------*/

#define SR_WORDS_2       32
#define SR_ADDR_2        ((FxU32) &((( SstRegs* )0)->fogTable[0]))
  FxU32 pkt1Hdr_2;

  FxU32 fogTable[32];
  /* ----------- end packet -------------*/

#define OOOO_OOOO_OOOO_IIII  0x000f
#define SR_MASK_3            OOOO_OOOO_OOOO_IIII
#define SR_ADDR_3            ((FxU32) &((( SstRegs* )0)->colBufferAddr))
  FxU32 pkt4Hdr_3;

  FxU32 colBufferAddr;   /* 0x1CC (  0 ) */
  FxU32 colBufferStride; /* 0x1D0 (  1 ) */
  FxU32 auxBufferAddr;   /* 0x1D4 (  2 ) */
  FxU32 auxBufferStride; /* 0x1D8 (  3 ) */
  /* ----------- end packet -------------*/

  struct tmuState {
#define OOOO_OOOO_OIII_IIII  0x007f
#define SR_MASK_4            OOOO_OOOO_OIII_IIII
    FxU32 texPkt4Hdr_0;

    FxU32 textureMode;     /* 0x300 (  0 ) */
    FxU32 tLOD;            /* 0x304 (  1 ) */
    FxU32 tDetail;         /* 0x308 (  2 ) */
    FxU32 texBaseAddr;     /* 0x30C (  3 ) */
    FxU32 texBaseAddr_1;   /* 0x310 (  4 ) */
    FxU32 texBaseAddr_2;   /* 0x314 (  5 ) */
    FxU32 texBaseAddr_3_8; /* 0x318 (  6 ) */
    /* ----------- end packet -------------*/

#define OOOO_OOOO_OOOO_OOII  0x0002
#define SR_MASK_5            OOOO_OOOO_OOOO_OOII
    FxU32 texPkt4Hdr_1;

    FxU32 texchromaKey;
    FxU32 texchromaRange;
    /* ----------- end packet -------------*/
    
#define SR_WORDS_6       24
    FxU32 texPkt1Hdr_2;
    
    FxU32 nccTable0[12];
    FxU32 nccTable1[12];
    /* ----------- end packet -------------*/
  } tmuState[GLIDE_NUM_TMU];

  struct PaletteRow {
#define SR_WORDS_P       8
#define SR_ADDR_P        ((FxU32) &((( SstRegs* )0)->nccTable0[4]))
    FxU32 pkt1Hdr_P;

    FxU32 data[8];
    /* ----------- end packet -------------*/
  } paletteRow[32];

} GrStateBuffer;

/*==========================================================================*/
/*
** GrState
**
** This structure comprises the entire queryable state in Glide.
** 
** Two types of data qualify for inclusion here:
**
** API State  - cull-mode
** Chip State - hw register state
**
** Not included:
** any volatile data: eg fifo setup, colBufferAddr, etc
*/

typedef struct {
    FxU32 cull_mode;           /* cull neg, cull pos, don't cull   */
  
    FxU32 paramIndex;          /* Index into array containing
                                  parameter indeces to be sent ot the
                                  triangle setup code    */
#if GLIDE_INIT_HAL
    FxI32
     fifoFree;                 /* # free entries in FIFO */
#endif
    FxU32 tmuMask;             /* Tells the paramIndex updater which
                                  TMUs need values */

    GrStateBuffer shadow;      /* shadow of all hw state registers */

    struct PerTmuState {
        float s_scale;
        float t_scale;
        FxU32 mmMode;
        FxU32 smallLod;
        FxU32 largeLod;
        FxU32 evenOdd;
        FxU32 nccTable;
    } per_tmu[GLIDE_NUM_TMU];

  
    FxBool                       /* Values needed to determine which */
    ac_requires_it_alpha,      /*   parameters need gradients computed */
        ac_requires_texture,       /*   when drawing triangles      */
        cc_requires_it_rgb,
    cc_requires_texture,
    allowLODdither,            /* allow LOD dithering            */
    checkFifo;                 /* Check fifo status as specified by hints */
  
  FxU32
    lfb_constant_depth;        /* Constant value for depth buffer (LFBs) */
  GrAlpha_t
    lfb_constant_alpha;        /* Constant value for alpha buffer (LFBs) */
  
  FxU32
    num_buffers;               /* 2 or 3 */
  
  GrColorFormat_t
    color_format;              /* ARGB, RGBA, etc. */
  
  GrOriginLocation_t           /* lower left, upper left */
    origin;                     

  GrTexTable_t tex_table;      /* Current palette type - ncc vs pallette */
  
  float
    clipwindowf_xmin, clipwindowf_ymin, /* Clipping info */
    clipwindowf_xmax, clipwindowf_ymax;
  FxU32
    screen_width, screen_height; /* Screen width and height */
  
  /* viewport and clip space coordinate related stuff */
  
  struct {
    float
      n, f;
    FxFloat
      ox, oy, oz;
    FxFloat
      hwidth, hheight, hdepth;
  } Viewport;
  
  
  /* Strip Stuff */
  GrVertexLayout vData;

  /*============================================================
  **  State Monster Stuff:
  **============================================================*/
  /*  
  **   The following DWORD is used to determine what state (if any) needs to
  **   be flushed when a rendering primative occurs.
  */
  FxU32
    invalid;
  /* invalid contains bits representing:
     alphaMode register:
        modified by grAlphaBlendFunction, grAlphaTestFunction,
        grAlphaTestReferenceValue  

     fbzColorPath register:
        modified by grAlphaCombine, grAlphaControlsITRGBLighting,
        grColorCombine

     fbzMode register:
        modified by grChromaKeyMode, grDepthBufferFunction,
        grDeptBufferMode, grDepthMask, grDitherMode, grRenderBuffer,
        grSstOrigin, grColorMask 

     chromaKey register:
        modified by grChromaKeyValue

     clipLeftRight, clipBottomTop registers:
        modified by grClipWindow

     zaColor register:
        modified by grDepthBiasLevel

     fogMode register:
        modified by grFogMode

     fogColor register:
        modified by grFocColorValue

     lfbMode register:
        modified by grLfbWriteColorFormat, grLfbWriteColorSwizzle 

     c0 & c1 registers:
        modified by grConstanColorValue
   */

  /*
  **  Argument storage for State Monster:
  **
  **  NOTE that the data structure element names are IDENTICAL to the function
  **  argment names.  This is very important, as there are macros in distate.c
  **  that require that.
  */
  struct {
    struct {
      GrAlphaBlendFnc_t rgb_sf;
      GrAlphaBlendFnc_t rgb_df;
      GrAlphaBlendFnc_t alpha_sf;
      GrAlphaBlendFnc_t alpha_df;
    } grAlphaBlendFunctionArgs;
    struct {
      GrCmpFnc_t fnc;
    } grAlphaTestFunctionArgs;
    struct {
      GrAlpha_t value;
    } grAlphaTestReferenceValueArgs; 
    struct {
      GrCombineFunction_t function;
      GrCombineFactor_t factor;
      GrCombineLocal_t local;
      GrCombineOther_t other;
      FxBool invert;
    } grAlphaCombineArgs;
    struct {
      FxBool enable;
    } grAlphaControlsITRGBLightingArgs;
    struct {
      GrCombineFunction_t function;
      GrCombineFactor_t factor;
      GrCombineLocal_t local;
      GrCombineOther_t other;
      FxBool invert;
    } grColorCombineArgs;
    struct {
      FxBool rgb;
      FxI32  alpha;
    } grColorMaskArgs;
    struct {
      FxBool enable;
    } grDepthMaskArgs;
    struct {
      GrChromakeyMode_t mode;
    } grChromakeyModeArgs;
    struct {
      GrColor_t color;
    } grChromakeyValueArgs;
    struct {
      GrColor_t range;
      GrChromaRangeMode_t mode;
      GrChromaRangeMode_t match_mode;
    } grChromaRangeArgs;
    struct {
      GrCmpFnc_t fnc;
    } grDepthBufferFunctionArgs;
    struct {
      GrDepthBufferMode_t mode;
    } grDepthBufferModeArgs;
    struct {
      GrDitherMode_t mode;
    } grDitherModeArgs;
    struct {
      GrStippleMode_t mode;
    } grStippleModeArgs;
    struct {
      GrStipplePattern_t stipple;
    } grStipplePatternArgs;
    struct {
      GrBuffer_t buffer;
    } grRenderBufferArgs;
    struct {
      GrOriginLocation_t origin;
    } grSstOriginArgs;
    struct {
      FxU32 minx;
      FxU32 miny;
      FxU32 maxx;
      FxU32 maxy;
    } grClipWindowArgs;
    struct {
      FxU32 level;
    } grDepthBiasLevelArgs;
    struct {
      GrFogMode_t mode;
    } grFogModeArgs;
    struct {
      GrColor_t color;
    } grFogColorValueArgs;
    struct {
      GrColorFormat_t colorFormat;
    } grLfbWriteColorFormatArgs;
    struct {
      FxBool swizzleBytes;
      FxBool swapWords;
    } grLfbWriteColorSwizzleArgs;
    struct {
      GrColor_t color;
    } grConstantColorValueArgs;
  } stateArgs;
  struct{
    GrEnableMode_t primitive_smooth_mode;
    GrEnableMode_t shameless_plug_mode;
    GrEnableMode_t video_smooth_mode;
    GrEnableMode_t texture_uma_mode;
  } grEnableArgs;
  struct{
    GrCoordinateSpaceMode_t coordinate_space_mode;
  } grCoordinateSpaceArgs;
} GrState;

/*
**  Private GR_GET enumerants:
*/
#define GR_MEMTYPE      GR_GET_RESERVED_1

#define TRISETUPARGS const void *a, const void *b, const void *c

/* gpci.c 
 *
 * Set of procs for the current cpu type. These are selected out of
 * the _archXXXX proc list that is selected at grGlideInit time.
 */

typedef FxI32 (FX_CALL* GrTriSetupProc)(TRISETUPARGS);


typedef void  (FX_CALL* GrVertexListProc)(FxU32 pkType, FxU32 type, FxI32 mode, FxI32 count, void* ptrs);
typedef void  (FX_CALL* GrDrawTrianglesProc)(FxI32 mode, FxI32 count, void* vPtrs);

/* [grState valid bit][cull/no cull mode] */
typedef GrTriSetupProc GrTriSetupProcVector[2][2];

/* [Coordinate mode - window/clip] */
typedef GrTriSetupProcVector GrTriSetupProcArchVector[2];

/* Decalrations of the dispatchable procs found in xdraw2.asm and
 * xtexdl.c for teh triangle and texture download procs respectively.  
 *
 * NB: These procs have the special calling convention that the current
 *     gc is nabbed from tls and passed in edx. Screw w/ this at your
 *     own peril. You have been warned.
 */
extern FxI32 FX_CALL _trisetup_Default_win_cull_invalid(TRISETUPARGS);
extern FxI32 FX_CALL _trisetup_Default_win_cull_valid(TRISETUPARGS);
extern FxI32 FX_CALL _trisetup_Default_win_nocull_invalid(TRISETUPARGS);
extern FxI32 FX_CALL _trisetup_Default_win_nocull_valid(TRISETUPARGS);

/* Only one specialziation exists so far */
extern FxI32 FX_CALL _vptrisetup_cull(TRISETUPARGS);

/* Routine to call into the architecture specialized version of 
 * _grDrawTriangles if there is no specialized version of the
 * grDrawTriangle routine for clip coordinates.
 */
extern FxI32 FX_CALL _trisetup_clip_coor_thunk(TRISETUPARGS);

extern void FX_CALL _grDrawTriangles_Default(FxI32, FxI32, void*);

void FX_CSTYLE _drawvertexlist(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);
void FX_CSTYLE _vpdrawvertexlist(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);

#if GL_AMD3D
extern FxI32 FX_CALL _trisetup_3DNow_win_cull_invalid(TRISETUPARGS);
extern FxI32 FX_CALL _trisetup_3DNow_win_cull_valid(TRISETUPARGS);
extern FxI32 FX_CALL _trisetup_3DNow_win_nocull_invalid(TRISETUPARGS);
extern FxI32 FX_CALL _trisetup_3DNow_win_nocull_valid(TRISETUPARGS);

extern FxI32 FX_CALL _trisetup_3DNow_clip_cull_invalid(TRISETUPARGS);
extern FxI32 FX_CALL _trisetup_3DNow_clip_cull_valid(TRISETUPARGS);
extern FxI32 FX_CALL _trisetup_3DNow_clip_nocull_invalid(TRISETUPARGS);
extern FxI32 FX_CALL _trisetup_3DNow_clip_nocull_valid(TRISETUPARGS);

extern void FX_CALL _grDrawTriangles_3DNow(FxI32, FxI32, void*);
void FX_CSTYLE _grDrawVertexList_3DNow_Window(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);
void FX_CSTYLE _grDrawVertexList_3DNow_Clip(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);
#endif /* GL_AMD3D */

struct GrGC_s;
/* _GlideRoot.curTexProcs is an array of (possibly specialized
 * function pointers indexed by texture format size (8/16 bits) and
 * texture line width (1/2/4/>4).
 *
 * xtexdl.c
 */
typedef void  (FX_CALL* GrTexDownloadProc)(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                           const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                           void* texData);
typedef GrTexDownloadProc GrTexDownloadProcVector[2][4];

extern void FX_CALL _grTexDownload_Default_8_1(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                               const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                               void* texData);
extern void FX_CALL _grTexDownload_Default_8_2(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                               const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                               void* texData);
extern void FX_CALL _grTexDownload_Default_8_4(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                               const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                               void* texData);
extern void FX_CALL _grTexDownload_Default_8_WideS(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                                   const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                                   void* texData);

extern void FX_CALL _grTexDownload_Default_16_1(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                                const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                                void* texData);
extern void FX_CALL _grTexDownload_Default_16_2(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                                const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                                void* texData);
extern void FX_CALL _grTexDownload_Default_16_4(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                                const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                                void* texData);
extern void FX_CALL _grTexDownload_Default_16_WideS(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                                    const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                                    void* texData);

#if GL_AMD3D
/* xtexdl.asm */
extern void FX_CALL _grTexDownload_3DNow_MMX(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                             const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                             void* texData);
#endif /* GL_AMD3D */

typedef struct GrGC_s
{
  struct {
    FxU32       bufferSwaps;    /* number of buffer swaps       */
    FxU32       pointsDrawn;
    FxU32       linesDrawn;
    FxU32       trisProcessed;
    FxU32       trisDrawn;
    FxU32       othertrisDrawn;

    FxU32       texDownloads;   /* number of texDownload calls   */
    FxU32       texBytes;       /* number of texture bytes downloaded   */

    FxU32       palDownloads;   /* number of palette download calls     */
    FxU32       palBytes;       /* number of palette bytes downloaded   */

    FxU32       nccDownloads;   /* # of NCC palette download calls */
    FxU32       nccBytes;       /* # of NCC palette bytes downloaded */

#if USE_PACKET_FIFO
    FxU32       fifoWraps;
    FxU32       fifoWrapDepth;
    FxU32       fifoStalls;
    FxU32       fifoStallDepth;
#endif /* USE_PACKET_FIFO */
  } stats;

  struct {
    /* temps to convert floats to ints */
    union { float f; FxI32 i; FxU32 u; } temp1;
    union { float f; FxI32 i; FxU32 u; } temp2;
  } pool;

#if GLIDE_HW_TRI_SETUP
  FxI32 curVertexSize;          /* Size in bytes of a single vertex's parameters */
#endif

  FxI32 curTriSize;             /* the size in bytes of the current triangle */

  FxU32
    orgSW, orgSH;               /* Original Screen width & Height */
  FxU32
    totBuffers,
    strideInTiles,
    heightInTiles,
    bufferStride,
    bufSizeInTiles,
    bufSize,
    fbOffset,
    *base_ptr,                  /* base address of SST */
    *reg_ptr,                   /* pointer to base of SST registers */
    *tex_ptr,                   /* texture memory address */
    *lfb_ptr;                   /* linear frame buffer address */

  struct GrTmuMemInfo {
    /* Information for keeping track of the characteristics of the
     * memory 'surfaces'. (We use 'surfaces' here not in the surface
     * extension sense, but in the region sense).
     *
     * xxxTiled:  Is this logical memory location in a tiled region?
     * xxxStride: The stride to the next logical 'line' in a given 
     *            region. This is in the units based on the respective
     *            xxxTiled field.
     */
    FxU32
      tramOffset,
      tramSize,
      tramLfbAddr;
    FxU32
      texStrideTiles,
      texStrideBytes;
    FxBool
      texTiled;

    /* Information about how to flush any
     * downloads from any internal hw fifos.  
     *
     *  flushCount: If > 0 then the flush operations should be 
     *              performed using the pre/post flush packets. 
     *              This should initially be set to 1 in the hw
     *              init stuff and then will be decremented in
     *              the multiple level downloads so the lower
     *              levels know not to worry about the flush.
     *  xxxPacket:  The actual packet data need to flush the
     *              internal fifo's.
     *    pre:
     *      pkt1 header: wax command
     *        command = nop | go
     *    post:
     *      pkt1 header: texBaseAddr
     *        ~user texBaseAddr
     *      pkt1 header: nopCMD
     *        0
     *      pkt1 header: texBaseAddr
     *        user texBaseAddr
     *      pkt1 header: wax command
     *        command = nop | go
     */

#if USE_PACKET_FIFO
#  define GR_TEX_FLUSH_WRITE(__writeCount, __writeData) \
    if (gc->contextP) { \
      GR_SET_EXPECTED_SIZE((__writeCount) * sizeof(FxU32), 0); \
      { \
        FxU32 \
          *curFifoPtr = gc->cmdTransportInfo.fifoPtr, \
          *curPktData = __writeData, \
          i; \
        for(i = 0; i < (__writeCount); i++) { \
          SET(*curFifoPtr++, *curPktData++); \
        } \
        GR_INC_SIZE((__writeCount) * sizeof(FxU32)); \
        gc->cmdTransportInfo.fifoRoom -= ((unsigned long)curFifoPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr); \
        gc->cmdTransportInfo.fifoPtr = curFifoPtr; \
      } \
      GR_CHECK_SIZE(); \
   }

#  define TEX_FLUSH_COUNT_PRE  2
#  define TEX_FLUSH_COUNT_POST 8

#  define GR_TEX_FLUSH_PRE(__tmuMemInfo) \
    do { \
      if ((__tmuMemInfo)->flushCount > 0) { \
        GR_TEX_FLUSH_WRITE(TEX_FLUSH_COUNT_PRE, (__tmuMemInfo)->prePacket); \
      } \
      (__tmuMemInfo)->flushCount--; \
    } while(0)
#  define GR_TEX_FLUSH_POST(__tmuMemInfo) \
    do { \
      (__tmuMemInfo)->flushCount++; \
      if ((__tmuMemInfo)->flushCount > 0) { \
        GR_TEX_FLUSH_WRITE(TEX_FLUSH_COUNT_POST, (__tmuMemInfo)->postPacket); \
      } \
    } while(0)
#else /* !USE_PACKET_FIFO */
    /* temp defs */
#  define TEX_FLUSH_COUNT_PRE  2
#  define TEX_FLUSH_COUNT_POST 8
#  define GR_TEX_FLUSH_PRE(__tmuMemInfo)
#  define GR_TEX_FLUSH_POST(__tmuMemInfo)
#endif /* !USE_PACKET_FIFO */

    FxI32
      flushCount;
    FxU32
      prePacket[TEX_FLUSH_COUNT_PRE],
      postPacket[TEX_FLUSH_COUNT_POST];
  } tmuMemInfo[GLIDE_NUM_TMU];
  
#ifdef GLIDE_INIT_HWC
  hwcBoardInfo
    *bInfo;
  FxU32
    winContextId;
#endif

#if GLIDE_MULTIPLATFORM
  GrGCFuncs
    gcFuncs;
#endif  

#define kMaxVertexParam (20 + (12 * GLIDE_NUM_TMU) + 3)
#ifndef GLIDE3
  struct dataList_s {
    int      i;
    FxFloat* addr;
  } regDataList[kMaxVertexParam];
  int tsuDataList[kMaxVertexParam];
#endif
  int tsuDataList[kMaxVertexParam];
#ifdef GLIDE3_SCALER
  int tsuDataListScaler[kMaxVertexParam];
#endif

  GrState
    state;                      /* state of Glide/SST */

  /* Here beginneth the Swap Pending Workaround (tm) */
#define MAX_BUFF_PENDING        0x7
  FxU32
    swapsPending,               /* swaps in unexecuted region of FIFO */
    lastSwapCheck,              /* Position at last check */
    curSwap,                    /* Position in the array below */
    bufferSwaps[MAX_BUFF_PENDING];/* Position in FIFO of buffer swaps */
  /* Here endeth the Swap Pending Workaround */

  struct {
    /* Current triangle rendering proc specialized for culling/no
     * culling and viewport/window coordinates.
     */
    GrTriSetupProc      triSetupProc;
    GrDrawTrianglesProc drawTrianglesProc;
    GrVertexListProc    drawVertexList;
    
    /* Vector to choose triangle rendering proc from based on the
     * cull/no-cull mode and whether or not the state is valid. This
     * vector is specialized on viewport vs window coordinates.  
     */
    GrTriSetupProcVector* coorModeTriVector;
    
    /* Vector of texture download procs specialized by size
     * and processor vendor type.
     */
    GrTexDownloadProcVector* texDownloadProcs;
  } archDispatchProcs;

  /* NB: This structure is exported via the COMMAND_TRANSPORT
   * extension which is shared w/ cvg glide3 as well. Care must be
   * taken to make the structures compatible and to inform the OpenGL
   * team (the only users of the extension) of any changes.  
   */
  struct cmdTransportInfo {
    FxU32  triPacketHdr; /* Pre-computed packet header for
                          * independent triangles. 
                          */
    
    FxU32  cullStripHdr; /* Pre-computed packet header for generic
                          * case of packet 3 triangles. This needs
                          * command type and # of vertices to be complete.
                          */
    
    FxU32  paramMask;    /* Mask for specifying parameters of
                          * non-triangle packets.  The parameter
                          * bits[21:10] mimic the packet3 header
                          * controlling which fields are sent, and
                          * pc[28] controls whether any color
                          * information is sent as packed.
                          */
    
    FxU32* fifoPtr;      /* Current write pointer into fifo */
    unsigned long   fifoRead;     /* Last known hw read ptr. 
                          * If on an sli enabled system this will be
                          * the 'closest' hw read ptr of the sli
                          * master and slave.
                          */
    
    /* Fifo checking information. In units of usuable bytes until
     * the appropriate condition.
     */
    FxI32  fifoRoom;     /* Space until next fifo check */

    FxBool autoBump;     /* Are we auto bumping (aka hole counting?) */
    FxU32
      *lastBump,         /* Last ptr where we bumped. */
      *bumpPos;          /* Next place to bump */
    FxU32
      bumpSize;          /* # of DWORDS per bump */

    FxU32 **ptrLostContext;

    /* Internal bookkeeping information - Everything after this point
     * is not exposed via the COMMAND_TRANSPORT extension 
     */

    /* Basic command fifo characteristics. These should be
     * considered logically const after their initialization.
     */
    FxU32* fifoStart;    /* Virtual address of start of fifo */
    FxU32* fifoEnd;      /* Virtual address of fba fifo */
    FxU32  fifoOffset;   /* Offset from hw base to fifo start */
    FxU32  fifoSize;     /* Size in bytes of the fifo */
    FxU32  fifoJmpHdr[2];/* Type0 packet for jmp to fifo start
                          * only first DWORD is used for memory
                          * fifo--both are used for AGP FIFO
                          */

    /* NB: These are only valid for the fullscreen case 
     */
    FxI32  roomToReadPtr;/* Bytes until last known hw ptr */
    FxI32  roomToEnd;    /* # of bytes until last usable address before fifoEnd */

    FxBool lfbLockCount; /* Have we done an lfb lock? Count of the locks. */

#if GLIDE_INIT_HWC
    GrStateBuffer* 
      stateBuffer;
    HwcWinFifo
      hwcFifoInfo;
    FxU32
      numCommandBuf,      /* # of command buffers */
      curCommandBuf,      /* Current command buffer waiting to issue */
      numQueuedBuf,       /* # of command buffers waiting to issue */
      serialNumber,       /* Current serial # */
      issuedSerialNumber, /* Last issued command buffer # */
      committedSerialNumber; /* Last known committed buffer */
    /* This is the backing store that gets used if we are not abled to
     * allocate surface storage for the fifo information.  
     */
    /* <=32k bytes -- don't want segment overflow in 16-bit driver*/
#define WINDOW_FIFO_SIZE_IN_DWORDS 0x2000
    FxU32         
      windowedFifo[WINDOW_FIFO_SIZE_IN_DWORDS];
    GrStateBuffer 
      windowedState;
#endif /* GLIDE_INIT_HWC */
  } cmdTransportInfo;

  FxI32 (FX_CALL *triSetupProc)(const void *a, const void *b, const void *c);
  
  SstIORegs
    *ioRegs;                    /* I/O remap regs */
  SstCRegs
    *cRegs;                     /* AGP/Cmd xfer/misc regs */
  SstGRegs
    *gRegs;                     /* 2D regs */
  SstRegs
    *sstRegs;                   /* Graphics Regs (3D Regs) */
  FxU32
    *rawLfb,
    nBuffers,
    curBuffer,
    frontBuffer,
    backBuffer,
    buffers[4];
  unsigned long
    lfbBuffers[4];              /* Tile relative addresses of the color/aux
                                 * buffers for lfbReads.
                                 */  
  unsigned long lockPtrs[2];        /* pointers to locked buffers */
  FxU32 fbStride;

  struct {
    FxU32             freemem_base;
    FxU32             total_mem;
    FxU32             next_ncc_table;
    GrMipMapId_t      ncc_mmids[2];
    const GuNccTable *ncc_table[2];
  } tmu_state[GLIDE_NUM_TMU];

  int
    grSstRez,                   /* Video Resolution of board */
    grSstRefresh,               /* Video Refresh of board */
    fbuf_size,                  /* in MB */
    num_tmu,                    /* number of TMUs attached */
    grColBuf,
    grAuxBuf,
    grHwnd;

#ifndef GLIDE3_ALPHA
  struct {
    GrMipMapInfo data[MAX_MIPMAPS_PER_SST];
    GrMipMapId_t free_mmid;
  } mm_table;                   /* mip map table */
#endif

  FxBool tmuLodDisable[GLIDE_NUM_TMU];

  /* DEBUG and SANITY variables */
  FxI32 myLevel;                /* debug level */
  FxI32 counter;                /* counts bytes sent to HW */
  FxI32 expected_counter;       /* the number of bytes expected to be sent */

  FxU32 checkCounter;
  FxU32 checkPtr;
   
  FxVideoTimingInfo* vidTimings;/* init code overrides */

  /* Has GC Been Opened? 
   *
   * NB: The following fields are similar, but subtly different.
   * Make sure that you keep their meanings differentiated when 
   * dicking w/ their values because various internal bits of glide
   * make various assumptions about their values.
   *
   * hwInitP: Is the hw mapping active? Some OS's (notably NT) require
   * that the driver keep track of mappings on a per process
   * basis. This flag indicates whether or not glide thinks that it
   * has a mapping allocated w/ the driver via hwcMapBoard. This is
   * only meaningful for a fullscreen application. 
   *
   * open: Does the application have an fullscreen context open.
   *   Fullscreen: grSstWinOpen/grSstWinClose
   *   Surface Extension: grSurfaceCreateContext/grSurfaceReleaseContext 
   *
   * contextP: This indicates whether glide thinks that its fsem
   * context is still valid. This can become false if the app looses
   * its context due to some os interaction (alt-tab on win32
   * platforns), but this does not affect the state of gc->open.  
   */
  FxBool 
    hwInitP,
    open,
    contextP;  

  FxU32 *
    lostContext;

  FxBool auxRendering;          /* Is an aux rendering surface current? */
  FxBool windowed;              /* is this a fullscreen or windowed gc */

#ifdef GLIDE_INIT_HWC
  hwcBufferDesc
    tBuffer,                    /* Texture Buuffer */
    *arBuffer;                  /* Aux Rendering Buffer */
  GrSurface_t
    curSurface,                 /* Current rendering surface */
    auxSurface,                 /* Aux buffer surface */
    texSurface[GLIDE_NUM_TMU];  /* Current texture surface */
#endif /* GLIDE_INIT_HWC */

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  /* Splash screen/shameless plug crap */
  struct {
    HMODULE
      moduleHandle;
    GrSplashInitProc
      initProc;
    GrSplashShutdownProc
      shutdownProc;
    GrSplashProc
      splashProc;
    GrSplashPlugProc
      plugProc;
  } pluginInfo;
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */
} GrGC;


/*
**  The Root Of All EVIL!
**
**  The root of all Glide data, all global data is in here
**  stuff near the top is accessed a lot
*/
struct _GlideRoot_s {
#if defined(__WATCOMC__) || defined(__MSC__) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
  int p6Fencer;                 /* xchg to here to keep this in cache!!! */
#endif
  unsigned long
    tlsIndex,
    tlsOffset;

  int current_sst;
#if GL_X86
  FxU32
    CPUType;
#endif
  FxBool
    OSWin95;
  FxI32
    windowsInit;        /* Is the fullscreen part of glide initialized? */


#if !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP
  FxU32 paramCount;
  FxI32 curTriSizeNoGradient;   /* special for _trisetup_nogradients */
#endif /* !GLIDE_HW_TRI_SETUP || !GLIDE_PACKET3_TRI_SETUP */

#if GLIDE_MULTIPLATFORM
  GrGCFuncs
    curGCFuncs;                 /* Current dd Function pointer table */
#endif
  int initialized;

  struct {                      /* constant pool (minimizes cache misses) */
    float  f0;
    float  fHalf;
    float  f1;
    float  f255;

#if GLIDE_PACKED_RGB
#define kPackBiasA _GlideRoot.pool.fBiasHi
#define kPackBiasR _GlideRoot.pool.fBiasHi
#define kPackBiasG _GlideRoot.pool.fBiasHi
#define kPackBiasB _GlideRoot.pool.fBiasLo

#define kPackShiftA 16UL
#define kPackShiftR 8UL
#define kPackShiftG 0UL
#define kPackShiftB 0UL

#define kPackMaskA  0x00FF00UL
#define kPackMaskR  0x00FF00UL
#define kPackMaskG  0x00FF00UL
#define kPackMaskB  0x00FFUL

    float  fBiasHi;
    float  fBiasLo;
#endif /* GLIDE_PACKED_RGB */
  } pool;

  struct {                      /* environment data             */
    FxBool ignoreReopen;
    FxBool triBoundsCheck;      /* check triangle bounds        */
    FxBool noSplash;            /* don't draw it                */
    FxBool shamelessPlug;       /* translucent 3Dfx logo in lower right */
    FxI32  swapInterval;        /* swapinterval override        */
    FxI32  swFifoLWM;
    FxU32  snapshot;            /* register trace snapshot      */
    FxBool disableDitherSub;    /* Turn off dither subtraction? */
    FxBool texLodDither;        /* Always do lod-dithering      */
    FxI32  tmuMemory;           /* tmuMemory */
    float  gammaR, gammaG, gammaB; /* Gamma settings */
    FxI32  swapPendingCount;    /* pending buffer swap count    */

    /* Force alternate buffer strategy */
    FxI32  nColorBuffer;
    FxI32  nAuxBuffer;
    FxBool autoBump;            /* Auto bump or do it manually? */
    FxU32  bumpSize;
  } environment;

  GrHwConfiguration     hwConfig;
  
  GrGC                  GCs[MAX_NUM_SST];       /* one GC per board     */
  GrGC                  surfaceGCs[MAX_NUM_CONTEXTS];
  GrGC                  *surfaceGCHeap[MAX_NUM_CONTEXTS];

  struct {
    GrTriSetupProcArchVector* curTriProcs;
    GrDrawTrianglesProc       curDrawTrisProc;
    GrVertexListProc*         curVertexListProcs;
    GrTexDownloadProcVector*  curTexProcs;
  
#define PROC_SELECT_TRISETUP(__procVector, __cullMode) (__procVector)[(__cullMode) != GR_CULL_DISABLE]
#define PROC_SELECT_TEXDOWNLOAD()                      _GlideRoot.curTexProcs

    GrTriSetupProcArchVector* nullTriProcs;
    GrDrawTrianglesProc       nullDrawTrisProc;
    GrVertexListProc*         nullVertexListProcs;
    GrTexDownloadProcVector*  nullTexProcs;    
  } deviceArchProcs;
};

extern struct _GlideRoot_s GR_CDECL _GlideRoot;
#if GLIDE_MULTIPLATFORM
extern GrGCFuncs _curGCFuncs;
#endif

#if defined( __MSC__ )
/* Turn off the no return value warning for the function definition.
 *
 * NB: The function returns a value so that we can use it in places
 * that require a value via the comma operator w/o resorting to casts
 * everywhere the macro is invoked.  
 *
 * NB: I checked the compiled code to make sure that it was inlined
 * everywhere that we would possibly care that it was inlines.
 */
#  pragma warning(disable : 4035)
   __inline FxU32 _grP6Fence(void) 
   { 
     __asm xchg eax, _GlideRoot.p6Fencer
   }
#  define P6FENCE _grP6Fence()
#  pragma warning(default : 4035)
#elif defined(macintosh) && defined(__POWERPC__) && defined(__MWERKS__)
#  define P6FENCE __eieio()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define P6FENCE asm("xchg %%eax, %0" : : "m" (_GlideRoot.p6Fencer) : "eax");
#elif defined(__GNUC__) && defined(__ia64__)
# define P6FENCE asm volatile ("mf.a" ::: "memory");
#elif defined(__GNUC__) && defined(__alpha__)
# define P6FENCE asm volatile("mb" ::: "memory");
#elif defined(__WATCOMC__)
void 
p6Fence(void);
#pragma aux p6Fence = \
  "xchg eax, dword ptr _GlideRoot" \
  modify [eax];

#define P6FENCE p6Fence()
#else  /* !defined ( P6FENCE ) */
#  error "P6 Fencing code needs to be added for this compiler"
#endif /* !defined ( P6FENCE ) */

/*==========================================================================*/
/* Macros for declaring functions */
#define GR_DDFUNC(name, type, args) \
   type FX_CSTYLE name args

#define GR_EXT_ENTRY( name, type, args ) \
   type FX_CSTYLE name args

#define GR_ENTRY(name, type, args) \
   FX_EXPORT type FX_CSTYLE name args

#define GR_FAST_ENTRY(name, type, args) \
   __declspec naked FX_EXPORT type FX_CSTYLE name args

#define GR_DIENTRY(name, type, args) \
   type FX_CSTYLE name args

#ifdef GLIDE3
#define GR_STATE_ENTRY(name, type, args) \
   type _ ## name args
#else
#define GR_STATE_ENTRY(name, type, args) \
   GR_ENTRY(name, type, args)
#endif

/*==========================================================================*/

#define STATE_REQUIRES_IT_DRGB        FXBIT(0)
#define STATE_REQUIRES_IT_DRGB_SHIFT        0
#define STATE_REQUIRES_IT_ALPHA       FXBIT(1)
#define STATE_REQUIRES_IT_ALPHA_SHIFT       1
#define STATE_REQUIRES_OOZ            FXBIT(2)
#define STATE_REQUIRES_OOZ_SHIFT            2
#define STATE_REQUIRES_OOW_FBI        FXBIT(3)
#define STATE_REQUIRES_OOW_FBI_SHIFT        3
#define STATE_REQUIRES_W_TMU0         FXBIT(4)
#define STATE_REQUIRES_W_TMU0_SHIFT         4
#define STATE_REQUIRES_ST_TMU0        FXBIT(5)
#define STATE_REQUIRES_ST_TMU0_SHIFT        5
#define STATE_REQUIRES_W_TMU1         FXBIT(6)
#define STATE_REQUIRES_W_TMU1_SHIFT         6
#define STATE_REQUIRES_ST_TMU1        FXBIT(7)
#define STATE_REQUIRES_ST_TMU1_SHIFT        7
#define STATE_REQUIRES_W_TMU2         FXBIT(8)
#define STATE_REQUIRES_W_TMU2_SHIFT         8
#define STATE_REQUIRES_ST_TMU2        FXBIT(9)
#define STATE_REQUIRES_ST_TMU2_SHIFT        9

#define GR_TMUMASK_TMU0 FXBIT(GR_TMU0)
#define GR_TMUMASK_TMU1 FXBIT(GR_TMU1)
#define GR_TMUMASK_TMU2 FXBIT(GR_TMU2)

/*
**  Parameter gradient offsets
**
**  These are the offsets (in bytes)of the DPDX and DPDY registers from
**  from the P register
*/
#ifdef GLIDE_USE_ALT_REGMAP
#define DPDX_OFFSET 0x4
#define DPDY_OFFSET 0x8
#else
#define DPDX_OFFSET 0x20
#define DPDY_OFFSET 0x40
#endif

#if   (GLIDE_PLATFORM & GLIDE_HW_SST1)
#define GLIDE_DRIVER_NAME "Voodoo Graphics"
#elif (GLIDE_PLATFORM & GLIDE_HW_SST96)
#define GLIDE_DRIVER_NAME "Voodoo Rush"
#elif (GLIDE_PLATFORM & GLIDE_HW_CVG)
#define GLIDE_DRIVER_NAME "Voodoo^2"
#elif (GLIDE_PLATFORM & GLIDE_HW_H3)
#define GLIDE_DRIVER_NAME "Banshee"
#else 
#define GLIDE_DRIVER_NAME "Unknown"
#endif

/*==========================================================================*/
#ifndef FX_GLIDE_NO_FUNC_PROTO

void _grMipMapInit(void);

/* NB: These routines are generally in asm (see xdraw2.inc), and have
 * their own internally defined calling conventions. The gc pointer
 * will be setup by the caller and will be in edx and that stack will
 * be configured as: return address, vertex a, vertex b, vertex c.  
 */
FxI32 FX_CSTYLE
_trisetup_cull(TRISETUPARGS);
FxI32 FX_CSTYLE
_trisetup(TRISETUPARGS);

FxI32 FX_CSTYLE
_trisetup_cull_noclip(TRISETUPARGS);
FxI32 FX_CSTYLE
_trisetup_noclip(TRISETUPARGS);

FxI32 FX_CSTYLE
_trisetup_cull_valid(TRISETUPARGS);
FxI32 FX_CSTYLE
_trisetup_valid(TRISETUPARGS);

FxI32 FX_CSTYLE
_trisetup_cull_noclip_valid(TRISETUPARGS);
FxI32 FX_CSTYLE
_trisetup_noclip_valid(TRISETUPARGS);


#define TRISETUP_NORGB(__cullMode) (((__cullMode) == GR_CULL_DISABLE) \
                                    ? _trisetup \
                                    : _trisetup_cull)

#define TRISETUP_NORGB_NOCLIP(__cullMode) (((__cullMode) == GR_CULL_DISABLE) \
                                    ? _trisetup_noclip \
                                    : _trisetup_cull_noclip)

#define TRISETUP_RGB(__cullMode)   TRISETUP_NORGB(__cullMode)
#define TRISETUP_ARGB(__cullMode)  TRISETUP_NORGB(__cullMode)

#if defined(_MSC_VER)
#if (_MSC_VER < 1200) /* TRISETUP Macro for pre-msvc 6.0 */
#define TRISETUP \
  __asm { mov edx, gc }; \
  (*gc->triSetupProc)
#else  /* TRISETUP Macro for msvc 6 or later */
#if defined(GLIDE_DEBUG) || GLIDE_USE_C_TRISETUP
/* MSVC6 Debug does funny stuff, so push our parms inline */
#define TRISETUP(_a, _b, _c) \
  __asm { \
    __asm mov  edx, gc \
    __asm mov  eax, _c \
    __asm push eax \
    __asm mov  ebx, _b \
    __asm push ebx \
    __asm mov  ecx, _a \
    __asm push ecx \
  } \
  ((FxI32 (*)(void))*gc->triSetupProc)()
#else /* MSVC6 Retail does funny stuff too, but Larry figured it out: */
#define TRISETUP(_a, _b, _c) \
  __asm { mov edx, gc }; \
  ((FxI32 (*)(const void *va, const void *vb, const void *vc, GrGC *gc))*gc->triSetupProc)(_a, _b, _c, gc)
#endif
#endif /* _MSC_VER */

#elif (defined(__linux__) && (defined(__i386__)||defined(__x86_64__))) || defined(__DJGPP__)
#define TRISETUP \
  __asm(""::"d"(gc)); \
  (*gc->triSetupProc)

#elif defined(__WATCOMC__)
extern void wat_trisetup (void *gc, const void *a, const void *b, const void *c);
#pragma aux wat_trisetup = \
	"push ecx" \
	"push ebx" \
	"push eax" \
	parm [edx] [eax] [ebx] [ecx];
#define TRISETUP(_a, _b, _c) \
        do { \
           wat_trisetup(gc, _a, _b, _c); \
           ((FxI32 (*)(void))*gc->triSetupProc)(); \
        } while (0)

#else
#define TRISETUP \
  (*gc->triSetupProc)
#endif

void GR_CDECL
_grValidateState();

void FX_CSTYLE
_grDrawVertexList(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);

void
_grAlphaBlendFunction(
                     GrAlphaBlendFnc_t rgb_sf,   GrAlphaBlendFnc_t rgb_df,
                     GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df
                     );
void
_grAlphaTestFunction( GrCmpFnc_t function );

void
_grAlphaTestReferenceValue( GrAlpha_t value );

void
_grAlphaCombine(
               GrCombineFunction_t function, GrCombineFactor_t factor,
               GrCombineLocal_t local, GrCombineOther_t other,
               FxBool invert
               );

void
_grAlphaControlsITRGBLighting( FxBool enable );

void
_grColorCombine(
               GrCombineFunction_t function, GrCombineFactor_t factor,
               GrCombineLocal_t local, GrCombineOther_t other,
               FxBool invert );


void FX_CALL
grStipplePattern(
            GrStipplePattern_t stipple);

void FX_CALL 
grChromaRangeMode(GrChromaRangeMode_t mode);

void FX_CALL 
grChromaRange( GrColor_t min, GrColor_t max , GrChromaRangeMode_t mode);

void FX_CALL 
grTexChromaMode(GrChipID_t tmu, GrTexChromakeyMode_t mode);

void FX_CALL 
grTexChromaRange(GrChipID_t tmu, GrColor_t min, GrColor_t max, GrTexChromakeyMode_t mode);

void
_grChromaMode( GrChromaRangeMode_t mode );

void
_grChromakeyMode( GrChromakeyMode_t mode );

#if GLIDE_DEBUG
/* These are only called for debug builds for tracing because their
 * handling is special cased in _grValidateState because of the
 * interdependency between the calls.  
 */
void _grDepthMask( FxBool mask );
void _grColorMask( FxBool rgb, FxBool a );
void _grLfbWriteColorFormat(GrColorFormat_t colorFormat);
void _grLfbWriteColorSwizzle(FxBool swizzleBytes, FxBool swapWords);
void _grFogColorValue( GrColor_t fogcolor );
void _grDepthBiasLevel( FxI32 level );
void _grChromakeyValue( GrColor_t color );
void _grChromaRange( GrColor_t max , GrChromaRangeMode_t mode);
#endif /* GLIDE_DEBUG */

void
_grDepthBufferFunction( GrCmpFnc_t function );

void
_grDepthBufferMode( GrDepthBufferMode_t mode );

void
_grDitherMode( GrDitherMode_t mode );

void
_grStippleMode( GrStippleMode_t mode );

void
_grRenderBuffer( GrBuffer_t buffer );

void
_grSstOrigin(GrOriginLocation_t  origin);

void
_grClipWindow( FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy );

void
_grFogMode( GrFogMode_t mode );

void
_grConstantColorValue( GrColor_t value );

void FX_CSTYLE
_grDrawPoints(FxI32 mode, FxI32 count, void *pointers);

void FX_CSTYLE
_grDrawLineStrip(FxI32 mode, FxI32 count, FxI32 ltype, void *pointers);

void FX_CSTYLE
_grAADrawPoints(FxI32 mode, FxI32 count, void *pointers);

void FX_CSTYLE
_grAADrawLineStrip(FxI32 mode, FxI32 ltype, FxI32 count, void *pointers);

void FX_CSTYLE
_grAADrawLines(FxI32 mode, FxI32 count, void *pointers);

void FX_CSTYLE
_grAADrawTriangles(FxI32 mode, FxI32 ttype, FxI32 count, void *pointers);

void FX_CSTYLE
_grAAVpDrawTriangles(FxI32 mode, FxI32 ttype, FxI32 count, void *pointers);

void FX_CSTYLE
_grAADrawVertexList(FxU32 type, FxI32 mode, FxI32 count, void *pointers);

void FX_CSTYLE
_guTexMemReset(void);

int FX_CSTYLE
_grBufferNumPending(void);

FxBool FX_CSTYLE
_grSstIsBusy(void);

void FX_CSTYLE
_grSstResetPerfStats(void);

void FX_CSTYLE
_grResetTriStats(void);

FxU32 FX_CSTYLE
_grSstStatus(void);

FxU32 FX_CSTYLE
_grSstVideoLine(void);

FxBool FX_CSTYLE
_grSstVRetraceOn(void);

#endif /* FX_GLIDE_NO_FUNC_PROTO */

/*==========================================================================*/
/* 
**  thread stuff
*/

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define W95_TEB_PTR                     0x18
#define W95_TEB_TLS_OFFSET              0x88
#define W95_TLS_INDEX_TO_OFFSET(i)      ((i)*sizeof(DWORD)+W95_TEB_TLS_OFFSET)

#define WNT_TEB_PTR                     0x18
#define WNT_TEB_TLS_OFFSET              0xE10
#define WNT_TLS_INDEX_TO_OFFSET(i)      ((i)*sizeof(DWORD)+WNT_TEB_TLS_OFFSET)

#ifdef __GNUC__

static __inline unsigned long getThreadValueFast (void)
{
 unsigned long t;
 __asm __volatile (" \
       mov %%fs:(%0), %%eax; \
       add %1, %%eax; \
       mov (%%eax), %%eax; \
 ":"=a"(t):"i"(WNT_TEB_PTR), "g"(_GlideRoot.tlsOffset));
 return t;
}

#else  /* __GNUC__ */

#pragma warning (4:4035)        /* No return value */
__inline unsigned long
getThreadValueFast() {
  __asm {
    __asm mov eax, DWORD PTR fs:[WNT_TEB_PTR] 
    __asm add eax, DWORD PTR _GlideRoot.tlsOffset 
    __asm mov eax, DWORD PTR [eax] 
  }
}
#endif /* __GNUC__ */
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_MACOS)
extern unsigned long _threadValueMacOS;
__inline unsigned long
getThreadValueFast() {
        return _threadValueMacOS;
}
#endif

#ifdef __linux__
extern unsigned long threadValueLinux;
#define getThreadValueFast() threadValueLinux
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_DOS32)
extern unsigned long GR_CDECL threadValueDJGPP;
#define getThreadValueFast() threadValueDJGPP
#endif

#define CUR_TRI_PROC(__checkValidP, __cullP) \
  (*gc->archDispatchProcs.coorModeTriVector)[__checkValidP][__cullP]
#define INVALIDATE(regset) {\
  gc->state.invalid |= regset ## BIT; \
  gc->triSetupProc = CUR_TRI_PROC(FXTRUE, (gc->state.cull_mode != GR_CULL_DISABLE)); \
}

void 
initThreadStorage( void );

void 
setThreadValue( unsigned long value );

unsigned long
getThreadValueSLOW( void );

void 
initCriticalSection( void );

void 
beginCriticalSection( void );

void 
endCriticalSection( void );


/*==========================================================================*/
/* 
**  function prototypes
*/

void
_grClipNormalizeAndGenerateRegValues(FxU32 minx, FxU32 miny, FxU32 maxx,
                                     FxU32 maxy, FxU32 *clipLeftRight,
                                     FxU32 *clipBottomTop);



void 
_grSwizzleColor(GrColor_t *color);

void
_grDisplayStats(void);

void
_GlideInitEnvironment(void);

void FX_CSTYLE
_grColorCombineDelta0Mode(FxBool delta0Mode);

void
_doGrErrorCallback(const char *name, const char *msg, FxBool fatal);

void _grErrorDefaultCallback(const char *s, FxBool fatal);

#ifdef __WIN32__
void _grErrorWindowsCallback(const char *s, FxBool fatal);
#endif /* __WIN32__ */

extern void
(*GrErrorCallback)(const char *string, FxBool fatal);

void GR_CDECL
_grFence(void);

void FX_CSTYLE
_grRebuildDataList(void);

void
_grReCacheFifo(FxI32 n);

FxI32 GR_CDECL
_grSpinFifo(FxI32 n);

void
_grShamelessPlug(void);

FxBool
_grSstDetectResources(void);

FxU16
_grTexFloatLODToFixedLOD(float value);

void FX_CSTYLE
_grTexDetailControl(GrChipID_t tmu, FxU32 detail);

void FX_CSTYLE
_grTexDownloadNccTable(GrChipID_t tmu, FxU32 which,
                        const GuNccTable *ncc_table,
                        int start, int end);

void FX_CSTYLE
_grTexDownloadPalette(GrChipID_t   tmu, 
                      GrTexTable_t type,
                      GuTexPalette *pal,
                      int start, int end);

FxU32
_grTexCalcBaseAddress(FxU32 start_address, GrLOD_t largeLod,
                      GrAspectRatio_t aspect, GrTextureFormat_t fmt,
                      FxU32 odd_even_mask); 

FxI32
_grTexCalcBaseAddressTiled(GrChipID_t tmu,
                           FxU32 start_address, 
                           GrAspectRatio_t aspect,
                           GrLOD_t largeLod,
                           GrTextureFormat_t fmt,
                           FxU32 odd_even_mask); 

FxU32
_grTexBytesPerTexel(const GrTextureFormat_t texFormat);

void
_grTexForceLod(GrChipID_t tmu, int value);

FxU32
_grTexCalcMipmapLevelOffsetTiled(GrChipID_t tmu,
                                 GrLOD_t lod, GrLOD_t largeLOD, 
                                 GrAspectRatio_t ar, 
                                 GrTextureFormat_t fmt, 
                                 FxU32 evenOdd);

FxU32
_grTexTextureMemRequired(GrLOD_t small_lod, GrLOD_t large_lod, 
                         GrAspectRatio_t aspect, GrTextureFormat_t format,
                         FxU32 evenOdd,
                         FxBool roundP);
void FX_CSTYLE
_grUpdateParamIndex(void);

/* ddgump.c */
void FX_CSTYLE
_gumpTexCombineFunction(int virtual_tmu);

/* disst.c - this is an un-documented external for arcade developers */
extern FX_ENTRY void FX_CALL
grSstVidMode(FxU32 whichSst, FxVideoTimingInfo* vidTimings);

/* glfb.c */
extern FxBool
_grLfbWriteRegion(FxBool pixPipelineP, GrBuffer_t dst_buffer, 
                  FxU32 dst_x, FxU32 dst_y, 
                  GrLfbSrcFmt_t src_format, 
                  FxU32 src_width, FxU32 src_height, 
                  FxI32 src_stride, 
                  const void *src_data);

/* gglide.c - Flushes the current state in gc->state.fbi_config to the hw.
 */
extern void
_grFlushCommonStateRegs(void);

/* gsst.c */
extern void 
initGC( GrGC *gc );
extern void 
assertDefaultState( void );


/*==========================================================================*/
/*  GMT: have to figure out when to include this and when not to
*/
#if defined(GLIDE_DEBUG) || defined(GLIDE_ASSERT) || defined(GLIDE_SANITY_ASSERT) || defined(GLIDE_SANITY_SIZE)
  #define DEBUG_MODE 1
  #include <assert.h>
#endif

#if 0
#define GR_DCL_GC GrGC *gc = (GrGC*)getThreadValueSLOW()
#else
#define GR_DCL_GC GrGC *gc = (GrGC*)getThreadValueFast()
#endif
#define GR_DCL_HW_INIT SstRegs *hw
#define GR_DCL_HW_STAGE2 hw = (SstRegs *)gc->sstRegs
#define GR_DCL_HW SstRegs *hw = (SstRegs *)gc->sstRegs

#ifdef DEBUG_MODE
#define ASSERT(exp) GR_ASSERT(exp)
#define GR_DEBUG_DCL_INIT() \
                FxI32 saveLevel; \
                char* myName

#define GR_DEBUG_DCL_STAGE2(name,level) \
                GR_ASSERT(gc != NULL);  \
				saveLevel = gc->myLevel; \
				myName = name;  \
				gc->myLevel = level; \
                gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr; \
                GDBG_INFO(gc->myLevel,myName); \
                FXUNUSED(saveLevel); \
                FXUNUSED(hw); \
                FXUNUSED(saveLevel)

#define GR_DEBUG_DCL(name,level) \
                const FxI32 saveLevel = gc->myLevel; \
                const char* myName = name;  \
                GR_ASSERT(gc != NULL);  \
                gc->myLevel = level; \
                gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr; \
                GDBG_INFO(gc->myLevel,myName); \
                FXUNUSED(saveLevel); \
                FXUNUSED(hw); \
                FXUNUSED(saveLevel)

#define GR_TRACE_EXIT(__n) \
                gc->myLevel = saveLevel; \
                GDBG_INFO(281, "%s --done---------------------------------------\n", __n)
#define GR_TRACE_RETURN(__l, __n, __v) \
                gc->myLevel = saveLevel; \
                GDBG_INFO((__l), "%s() => 0x%x---------------------\n", (__n), (__v), (__v))
#else /* !DEBUG_MODE */
#define ASSERT(exp)
#define GR_DEBUG_DCL_INIT()
#define GR_DEBUG_DCL_STAGE2(name, level)
#define GR_DEBUG_DCL(name, level)
#define GR_TRACE_EXIT(__n)
#define GR_TRACE_RETURN(__l, __n, __v) 
#endif /* !DEBUG_MODE */

#include <assert.h>

#ifdef GLIDE_ALT_TAB
#define GR_BEGIN_NOFIFOCHECK(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                GR_DEBUG_DCL(name, level);   \
                FXUNUSED(hw); \
                if (!gc) \
                  return; \
                if (gc->lostContext) { \
                  if (*gc->lostContext) { \
                    return;\
                  }\
                }
#define GR_BEGIN_NOFIFOCHECK_RET(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                GR_DEBUG_DCL(name, level);   \
                FXUNUSED(hw); \
                if (!gc) \
                  return 0; \
                if (gc->lostContext) {\
                  if (*gc->lostContext) { \
                      return 0;\
                  }\
                }
#define GR_BEGIN_NOFIFOCHECK_NORET(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                GR_DEBUG_DCL(name, level);   \
                FXUNUSED(hw)
#else
#define GR_BEGIN_NOFIFOCHECK(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                GR_DEBUG_DCL(name, level);   \
                FXUNUSED(hw)
#define GR_BEGIN_NOFIFOCHECK_RET(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                GR_DEBUG_DCL(name, level);   \
                FXUNUSED(hw)
#define GR_BEGIN_NOFIFOCHECK_NORET(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                GR_DEBUG_DCL(name, level);   \
                FXUNUSED(hw)
#endif

#define GR_BEGIN(name,level,size, packetNum) \
                GR_BEGIN_NOFIFOCHECK(name,level); \
                GR_SET_EXPECTED_SIZE(size, packetNum)

#define GR_END()        {GR_CHECK_SIZE(); GR_TRACE_EXIT(myName);}

#define GR_RETURN(val) \
                if (GDBG_GET_DEBUGLEVEL(gc->myLevel)) { \
                  GR_CHECK_SIZE(); \
                } \
                else \
                  GR_END(); \
                GR_TRACE_RETURN(gc->myLevel, myName, val); \
                return val

#if defined(GLIDE_SANITY_ASSERT)
#define GR_ASSERT(exp) ((void)((!(exp)) ? (_grAssert(#exp,  __FILE__, __LINE__),0) : 0xFFFFFFFF))
#else
# ifdef __GNUC__
#  define GR_ASSERT(exp)	((void) 0)
# else
#  define GR_ASSERT(exp) ((void)(0 && ((FxU32)(exp))))
# endif
#endif

#define INTERNAL_CHECK(__name, __cond, __msg, __fatalP) \
    if (__cond) _doGrErrorCallback(__name, __msg, __fatalP)

#if defined(GLIDE_DEBUG)
#define GR_CHECK_F(name,condition,msg) INTERNAL_CHECK(name, condition, msg, FXTRUE)
#define GR_CHECK_W(name,condition,msg) INTERNAL_CHECK(name, condition, msg, FXFALSE)
#else
#define GR_CHECK_F(name,condition,msg)
#define GR_CHECK_W(name,condition,msg)
#endif

#if GLIDE_CHECK_COMPATABILITY
#define GR_CHECK_COMPATABILITY(__name, __cond, __msg) INTERNAL_CHECK(__name, __cond, __msg, FXTRUE)
#else
#define GR_CHECK_COMPATABILITY(__name, __cond, __msg) GR_CHECK_F(__name, __cond, __msg)
#endif /* !GLIDE_CHECK_COMPATABILITY */

/* macro define some basic and common GLIDE debug checks */
#define GR_CHECK_TMU(name,tmu) \
  GR_CHECK_COMPATABILITY(name, tmu < GR_TMU0 || tmu >= gc->num_tmu , "invalid TMU specified")

void
_grAssert(char *, char *, int);

void i3(void);

#if ASSERT_FAULT
#define ASSERT_FAULT_IMMED(__x) if (!(__x)) { \
                                 *(FxU32*)NULL = 0; \
                                 _grAssert(#__x, __FILE__, __LINE__); \
                              }
#else
#define ASSERT_FAULT_IMMED(__x) GR_ASSERT(__x)
#endif

/* Offsets to 'virtual' addresses in the hw */
#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
#define HW_REGISTER_OFFSET      SST_3D_OFFSET
#define HW_FIFO_OFFSET          0x00200000UL    
#elif (GLIDE_PLATFORM & GLIDE_HW_H3)
#define HW_IO_REG_REMAP         SST_IO_OFFSET
#define HW_CMD_AGP_OFFSET       SST_CMDAGP_OFFSET
#define HW_2D_REG_OFFSET        SST_2D_OFFSET
#define HW_3D_REG_OFFSET        SST_3D_OFFSET
#define HW_REGISTER_OFFSET      HW_3D_REG_OFFSET
#else
#error "Must define virtual address spaces for this hw"
#endif

#define HW_FIFO_OFFSET          0x00200000UL
#define HW_LFB_OFFSET           SST_LFB_OFFSET
#define HW_TEXTURE_OFFSET       SST_TEX_OFFSET

#define SST_TEXTURE_ALIGN       0x10UL
#define SST_TEXTURE_ALIGN_MASK  (SST_TEXTURE_ALIGN - 0x01UL)

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) || (GLIDE_PLATFORM & GLIDE_HW_H3)
#define HW_BASE_PTR(__b)        (__b)
#else
#error "Need HW_BASE_PTR to convert hw address into board address."
#endif
   
#define HW_REG_PTR(__b)        ((FxU32*)(((FxU32)(__b)) + HW_REGISTER_OFFSET))
#define HW_LFB_PTR(__b)        ((FxU32*)(((FxU32)(__b)) + HW_LFB_OFFSET))
#define HW_TEX_PTR(__b)        ((FxU32*)(((FxU32)(__b)) + HW_TEXTURE_OFFSET))   

/* access a floating point array with a byte index */
#define FARRAY(p,i)    (*(float *)((i)+(long)(p)))
#define ArraySize(__a) (sizeof(__a) / sizeof((__a)[0]))

#if GDBG_INFO_ON
/* cvg.c */
extern void
_grErrorCallback(const char* const procName,
                 const char* const format,
                 va_list           args);
#endif

#if GL_X86
/* Returns 16:16 pair indicating the cpu's manufacturer and its
 * capabilities. Non-Intel processors should have a vendor id w/ the
 * high bit set so that it appears to be a negative #. The value of
 * the capability field is assumed to be a monotonically increasing
 * inclusive set.
 *
 * Unknown:
 *   0xFFFF:0xFFFF
 *
 * Intel: 0x0000
 *  4: 486 and lower
 *  5: Pentium
 *  6: P6 Core or better
 *  7: p6 Core w/ katmai (instructions/write buffers)
 *
 * AMD: 0x8001
 *  1: MMX
 *  2: 3DNow!(tm)
 *  4: K6-type MTRRs
 *  8: P2-type MTRRs
 *
 * Cyrix: 0x8002
 *  1: MMX
 *  2: 3DNow!(tm)
 *
 * IDT: 0x8003
 *  1: MMX
 *  2: 3DNow!(tm)
 *  
 */

enum {
  kCPUVendorIntel   = 0x0000,
  kCPUVendorAMD     = 0x8001,
  kCPUVendorCyrix   = 0x8002,
  kCPUVendorIDT     = 0x8003,
  kCPUVendorUnknown = 0xFFFF
};

extern FxI32 GR_CDECL
_cpu_detect_asm(void);

extern void GR_CDECL 
single_precision_asm(void);

extern void GR_CDECL 
double_precision_asm(void);
#endif

/* The translation macros convert from the reasonable log2 formats to
 * the somewhat whacked sst1 lod format. The api and internal
 * functions pass log2 values, but we convert to lod values when
 * writing things out to the hw or to index the legacy size tables
 * listed below.  */
#define G3_LOD_TRANSLATE(__lod)                       (0x8 - (__lod))
#define G3_ASPECT_TRANSLATE(__aspect)                 (0x3 - (__aspect))

/* ditex.c - 
 * Lookup tables to do translations from various aspect ratio/lod
 * sizes to more meaningful things like bytes.  
 */
extern const int _grMipMapHostWH[G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x8) + 1][G3_LOD_TRANSLATE(GR_LOD_LOG2_1) + 1][2];
extern const FxU32 _grMipMapHostSize[][9];
extern const FxU32 _gr_evenOdd_xlate_table[];
extern const FxU32 _gr_aspect_xlate_table[];

#define WIDTH_BY_ASPECT_LOD(__aspect, __lod) \
  _grMipMapHostWH[G3_ASPECT_TRANSLATE(__aspect)][G3_LOD_TRANSLATE(__lod)][0]

#define HEIGHT_BY_ASPECT_LOD(__aspect, __lod) \
  _grMipMapHostWH[G3_ASPECT_TRANSLATE(__aspect)][G3_LOD_TRANSLATE(__lod)][1]

GR_EXT_ENTRY(grTexDownloadTableExt,
         void,
         (GrChipID_t tmu, GrTexTable_t type,  void *data));

GR_ENTRY(grSetNumPendingBuffers,
         void,
         (FxI32 NumPendingBuffers));

#endif /* __FXGLIDE_H__ */
