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
** Revision 1.1.2.7  2006/01/16 21:22:45  jwrdegoede
** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
**
** Revision 1.1.2.6  2005/06/10 14:17:56  jwrdegoede
** Fix compilation when GL_X86 is not defined
**
** Revision 1.1.2.5  2005/05/25 08:51:52  jwrdegoede
** Add #ifdef GL_X86 around x86 specific code
**
** Revision 1.1.2.4  2004/10/07 07:49:08  dborca
** comment the GR_CDECL hack to prevent accidents
**
** Revision 1.1.2.3  2004/10/04 09:35:59  dborca
** second cut at Glide3x for Voodoo1/Rush (massive update):
** delayed validation, vertex snapping, clip coordinates, strip/fan_continue, bugfixes.
** and probably a bunch of other crap that I forgot
**
** Revision 1.1.2.2  2004/03/08 07:42:21  dborca
** Voodoo Rush fixes
**
** Revision 1.1.2.1  2004/03/02 07:55:29  dborca
** Bastardised Glide3x for SST1
**
** Revision 1.1.1.1  1999/12/07 21:48:52  joseph
** Initial checkin into SourceForge.
**
** 
** 86    2/12/98 4:01p Atai
** change refresh rate if oemdll updated for tv out
 * 
 * 85    12/19/97 11:04a Atai
 * oeminit dll stuff
 * 
 * 84    12/03/97 9:36a Dow
 * Fixed bug in grSstIsBusy()
 * 
 * 83    9/19/97 12:38p Peter
 * asm rush trisetup vs alt fifo
 * 
 * 82    9/11/97 9:31a Peter
 * This is what happens when you don't listen to Chris
 * 
 * 81    9/10/97 10:15p Peter
 * fifo logic from GaryT
 * 
 * 80    9/08/97 3:24p Peter
 * fixed my fifo muckage
 * 
 * 79    8/19/97 8:54p Peter
 * lots of stuff, hopefully no muckage
 * 
 * 78    8/01/97 11:48a Dow
 * Made some macros use conventional FIFO accounting
 * 
 * 77    7/10/97 1:36p Dow
 * Modified Nudge of Love to work around likely hardware bug.
 * 
 * 76    7/09/97 10:18a Dow
 * Further Nudge Of Love adjustments
 * 
 * 75    7/08/97 8:55p Dow
 * Fixed muckage in the Nudge Of Love
 * 
 * 74    7/08/97 1:29p Jdt
 * Fixed watcom muckage
 * 
 * 73    7/07/97 8:33a Jdt
 * New tracing macros.
 * 
 * 72    7/04/97 12:07p Dow
 * Changed the DUMPGWH stuff, added const for triangle command packet
 * 
 * 71    6/29/97 11:28p Jdt
 * Added gwCommand
 * 
 * 70    6/26/97 3:08p Dow
 * New metrics for P6 stuff.
 * 
 * 69    6/21/97 1:05p Dow
 * Moved the Nudge of Love to a macro
 * 
 * 68    6/20/97 5:50p Dow
 * Changes for Chip Field
 * 
 * 67    6/19/97 7:35p Dow
 * More P6 Stuff
 * 
 * 66    6/18/97 6:07p Dow
 * Protected P6 Stuff
 * 
 * 65    6/18/97 5:54p Dow
 * P6 adjustments
 * 
 * 64    6/16/97 12:45p Dow
 * P6 Fixes
 * 
 * 63    6/08/97 11:06p Pgj
 * use Group Write for Texture Downloads
 * 
 * 62    5/28/97 2:08p Dow
 * Added checks for int10h when in debugging mode.
 * 
 * 61    5/27/97 11:37p Pgj
 * Fix for Bug report 545
 * 
 * 60    5/27/97 2:02p Dow
 * Fixed up some macros--GR_CHECK_FOR_ROOM & a call to assert()
 * 
 * 59    5/22/97 11:18a Dow
 * Changed GR_ASSERT to fix stack muckage
 * 
 * 58    5/19/97 7:35p Pgj
 * Print cogent error message if h/w not found
 * 
 * 57    5/15/97 12:20p Dow
 * Fixed improper definition of GR_SET
 * 
 * 56    5/04/97 12:47p Dow
 * Added direct write macro fro grSstControl
 * 
 * 55    5/02/97 2:07p Pgj
 * screen_width/height now FxU32
 * 
 * 54    5/02/97 9:34a Dow
 * Changed indentation of GrState to match the rest of file, modified
 * GR_ASSERT
 * 
 * 53    4/13/97 2:06p Pgj
 * eliminate all anonymous unions (use hwDep)
 * 
 * 52    3/24/97 2:00p Dow
 * Fixed some chip field problems
 * 
 * 51    3/21/97 12:42p Dow
 * Made STWHints not send the Bend Over Baby Packet to FBI Jr.
 * 
 * 50    3/19/97 10:43p Dow
 * windowsInit stuff
 * 
 * 49    3/19/97 1:37a Jdt
 * Added fbStride to gc
 * 
 * 48    3/18/97 9:08p Dow
 * Added FX_GLIDE_NO_DITHER_SUB environment variable
 * 
 * 47    3/17/97 6:25a Jdt
 * Added open flag to gc
 * 
 * 46    3/16/97 12:38a Jdt
 * Shouldn't have removed fifoData, duh..
 * 
 * 45    3/16/97 12:19a Jdt
 * Removed redundant data
 * 
 * 44    3/13/97 2:51a Jdt
 * Removed lockIdle flag
 * 
 * 43    3/13/97 1:18a Jdt
 * Added flag for sli lfb reads.
 * 
 * 42    3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 41    3/04/97 9:11p Dow
 * Neutered mutiplatform multiheaded monster.
 * 
 * 40    2/26/97 2:18p Dow
 * moved all debug set functions to __cdecl
 * 
 * 39    2/26/97 11:54a Jdt
 * Added glide buffer locking and fixed bug in GR_SET
 * 
 * 38    2/19/97 4:42p Dow
 * Fixed debug build for Watcom
 * 
 * 37    2/14/97 12:55p Dow
 * moved vg96 fifo wrap into init code
 * 
 * 36    2/12/97 5:31p Dow
 * Fixed my ^$%^^&*^%^  error.
 * 
 * 35    2/12/97 2:09p Hanson
 * Hopefully removed the rest of my muckage. 
 * 
 * 34    2/11/97 6:58p Dow
 * Changes to support vid tiles and ser status
 * 
 * 33    1/18/97 11:44p Dow
 * Moved _curGCFuncs into _GlideRoot
 * Added support for GMT's register debugging
 * 
 * 32    1/16/97 3:41p Dow
 * Embedded fn protos in ifndef FX_GLIDE_NO_FUNC_PROTO
 * 
 * 31    12/23/96 1:37p Dow
 * chagnes for multiplatform glide
 * 
 * 30    11/14/96 1:04p Jdt
 * Test for keywords
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
** GLIDE_SIMULATOR:  Defined if GLIDE should use the software simulator.  An
**                   application is responsible for defining this macro.
**                   This macro is mutually exclusive with GLIDE_HARDWARE.
**
** GLIDE_HARDWARE:   Defined if GLIDE should use the actual SST hardware.  An
**                   application is responsible for defining this macro.
**                   This macro is mutually exclusive with GLIDE_SIMULATOR.
**
** GLIDE_NUM_TMU:    Number of physical TMUs installed.  Valid values are 1
**                   and 2.  If this macro is not defined by the application
**                   it is automatically set to the value 2.
**
*/

#ifndef __FXGLIDE_H__
#define __FXGLIDE_H__

/*
** -----------------------------------------------------------------------
** INCLUDE FILES
** -----------------------------------------------------------------------
*/
#include <stdlib.h>
#include <math.h>
#include <3dfx.h>
#include <sst1init.h>
#include <sst.h>
#include <gdebug.h>
#include <init.h>

#if GLIDE_MULTIPLATFORM
#include "gcfuncs.h"
#endif

/* isolate this 'hack' here so as to make the code look cleaner */
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

/* limits */
#define MAX_NUM_SST 4

/*
** grGet defines
*/
#define VOODOO_GAMMA_TABLE_SIZE      32
#define SST1_BITS_DEPTH              16
#define SST1_ZDEPTHVALUE_NEAREST     0xFFFF
#define SST1_ZDEPTHVALUE_FARTHEST    0x0000
#define SST1_WDEPTHVALUE_NEAREST     0x0000
#define SST1_WDEPTHVALUE_FARTHEST    0xFFFF

/*
** interanl constant for constrained query
*/
#define GR_MAX_RESOLUTION  15
#define GR_MAX_REFRESH      8
#define GR_MAX_COLOR_BUF    3
#define GR_MAX_AUX_BUF      1
#define GR_MIN_RESOLUTION   0
#define GR_MIN_REFRESH      0
#define GR_MIN_COLOR_BUF    2
#define GR_MIN_AUX_BUF      0

typedef int GrSstType;
#define GR_SSTTYPE_VOODOO    0
#define GR_SSTTYPE_SST96     1
#define GR_SSTTYPE_AT3D      2
#define GR_SSTTYPE_Voodoo2   3
#define GR_SSTTYPE_Banshee   4

/*
** sound stuttering if GR_HINT_FIFOCHECKHINT is not called
*/
typedef FxU32 GrHint_t;
#define GR_HINT_STWHINT                 0
#define GR_HINT_FIFOCHECKHINT           1
#define GR_HINT_FPUPRECISION            2
#define GR_HINT_ALLOW_MIPMAP_DITHER     3

/*
** AA Mode for OpenGL
*/
#define GR_AA_ORDERED_OGL               0x00010000
#define GR_AA_ORDERED_POINTS_OGL        GR_AA_ORDERED_OGL+1
#define GR_AA_ORDERED_LINES_OGL         GR_AA_ORDERED_OGL+2
#define GR_AA_ORDERED_TRIANGLES_OGL     GR_AA_ORDERED_OGL+3

#define GR_AA_ORDERED_POINTS_MASK       0x01
#define GR_AA_ORDERED_LINES_MASK        0x02
#define GR_AA_ORDERED_TRIANGLES_MASK    0x04

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
  int   vg96Rev;
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

typedef FxU32 GrControl_t;
#define GR_CONTROL_ACTIVATE   0x1
#define GR_CONTROL_DEACTIVATE 0x2

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

#define SSTCP_PKT3_BDDDDD       1
#define SSTCP_PKT3_DDDDDD       0

/*
** packed color
*/
#define GR_PARGB_A              3
#define GR_PARGB_R              2
#define GR_PARGB_G              1
#define GR_PARGB_B              0

/*
** snap bias for VG/VR
*/
#define SNAP_BIAS ((float)(3<<18))

typedef struct {
  FxU32
    mode;                       /* enable / disable */
  FxI32
    offset;                     /* offset to the parameter data */
} GrVParamInfo;

typedef struct {
  GrVParamInfo
    vertexInfo,          /* xy */
    zInfo,               /* z(ooz) */
    wInfo,               /* w(oow) */
    aInfo,               /* a float */
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
  if (gc->state.invalid) _grValidateState()


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


/* SST-2 research flags         */
#define SST2_SKIP_SWAP   1
#define SST2_SKIP_CLEAR  2
#define SST2_SKIP_TRI    4
#define SST2_TRI_8F      8
#define SST2_TRI_8F_MEM 16

/*
** vertex buffer definition for GR_TRIANGLE_STRIP_CONTINUE and GR_TRIANGLE_FAN_CONTINUE
*/
typedef struct {
  float  sow;                   /* s texture ordinate (s over w) */
  float  tow;                   /* t texture ordinate (t over w) */  
  float  oow;                   /* 1/w (used mipmapping - really 0xfff/w) */
}  GrTmuVertexBuff;

typedef struct
{
  float x, y, z;                /* X, Y, and Z of scrn space -- Z is ignored */
  float r, g, b;                /* R, G, B, ([0..255.0]) */
  float ooz;                    /* 65535/Z (used for Z-buffering) */
  float a;                      /* Alpha [0..255.0] */
  float oow;                    /* 1/W (used for W-buffering, texturing) */
  GrTmuVertexBuff  tmuvtx[GLIDE_NUM_TMU];
} GrVertexBuff;

/*==========================================================================*/
/*
** GrState
**
** If something changes in here, then go into glide.h, and look for a 
** declaration of the following form:
**
** #define GLIDE_STATE_PAD_SIZE N
** #ifndef GLIDE_LIB
** typedef struct {
**   char pad[GLIDE_STATE_PAD_SIZE];
** }  GrState;
** #endif
** 
** Then change N to sizeof(GrState) AS DECLARED IN THIS FILE!
**
*/

typedef struct
{
  GrCullMode_t                 /* these go in front for cache hits */
    cull_mode;                 /* cull neg, cull pos, don't cull   */
  
  FxI32
    fifoFree;                  /* # free entries in FIFO */
  FxU32
    paramIndex,                /* Index into array containing
                                  parameter indeces to be sent ot the
                                  triangle setup code    */
    tmuMask;                   /* Tells the paramIndex updater which
                                  TMUs need values */
  struct{
    FxU32   fbzColorPath;
    FxU32   fogMode;
    FxU32   alphaMode;
    FxU32   fbzMode;
    FxU32   lfbMode;
    FxU32   clipLeftRight;
    FxU32   clipBottomTop;
    
    FxU32   fogColor;
    FxU32   zaColor;
    FxU32   chromaKey;
    FxU32   chromaRange;
     
    FxU32   stipple;
    FxU32   color0;
    FxU32   color1;
  } fbi_config;                 /* fbi register shadow */
  
  struct {
    FxU32   textureMode;
    FxU32   tLOD;
    FxU32   tDetail;
    FxU32   texBaseAddr;
    FxU32   texBaseAddr_1;
    FxU32   texBaseAddr_2;
    FxU32   texBaseAddr_3_8;
    GrMipMapMode_t mmMode;      /* saved to allow MM en/dis */
    GrLOD_t        smallLod, largeLod; /* saved to allow MM en/dis */
    FxU32          evenOdd;
    float   st_scale[2];
    GrNCCTable_t   nccTable;
  } tmu_config[GLIDE_NUM_TMU];  /* tmu register shadow           */
  
  FxBool                       /* Values needed to determine which */
    ac_requires_it_alpha,      /*   parameters need gradients computed */
    ac_requires_texture,       /*   when drawing triangles      */
    cc_requires_it_rgb,
    cc_requires_texture,
    cc_delta0mode,             /* Use constants for flat shading */
    allowLODdither,            /* allow LOD dithering            */
    checkFifo;                 /* Check fifo status as specified by hints */
  
  FxU32
    lfb_constant_depth;        /* Constant value for depth buffer (LFBs) */
  GrAlpha_t
    lfb_constant_alpha;        /* Constant value for alpha buffer (LFBs) */
  
  FxI32
    num_buffers;               /* 2 or 3 */
  
  GrColorFormat_t
    color_format;              /* ARGB, RGBA, etc. */
  
  GrMipMapId_t
    current_mm[GLIDE_NUM_TMU]; /* Which guTex** thing is the TMU set
                                  up for? THIS NEEDS TO GO!!! */
  
  float
    clipwindowf_xmin, clipwindowf_ymin, /* Clipping info */
    clipwindowf_xmax, clipwindowf_ymax;
  FxU32
    screen_width, screen_height; /* Screen width and height */
  float
    a, r, g, b;                /* Constant color values for Delta0 mode */
  
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
#ifdef GLIDE_VERTEX_TABLE
  FxU32 vTable[16];
#endif

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
      FxBool alpha;
    } grColorMaskArgs;
    struct {
      GrChromakeyMode_t mode;
    } grChromakeyModeArgs;
    struct {
      GrColor_t color;
    } grChromakeyValueArgs;
    struct {
      GrColor_t range;
      GrChromaRangeMode_t mode;
    } grChromaRangeArgs;
    struct {
      FxBool enable;
    } grDepthMaskArgs;
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
  } grEnableArgs;
  struct{
    GrCoordinateSpaceMode_t coordinate_space_mode;
  } grCoordinateSpaceArgs;
} GrState;

typedef struct GrGC_s
{
  FxU32
    *base_ptr,                  /* base address of SST */
    *reg_ptr,                   /* pointer to base of SST registers */
    *tex_ptr,                   /* texture memory address */
    *lfb_ptr,                   /* linear frame buffer address */
    *slave_ptr;                 /* Scanline Interleave Slave address */

#if GLIDE_MULTIPLATFORM
  GrGCFuncs
    gcFuncs;
#endif  

  void *oemInit;

  struct dataList_s {
    int i;                      /* index into GrVertex struct for src data */
    float *addr;                /* address of hardware register to write to */
    FxU32 bddr;                 /* byte address of color component in vertex */
    FxU32 mask;                 /* mask for packed color */
    FxU32 shift;                /* shift for packed color */
  } dataList[20+12*GLIDE_NUM_TMU+3];/* add 3 for:
                                       fbi-tmu0 trans
                                       tmu0-tmu1 trans
                                       tmu1-tmu2 trans
                                       */
  GrState
    state;                      /* state of Glide/SST */

  GrVertexBuff vert1;
  GrVertexBuff vert2;
  FxBool vert2_status;
  FxBool flip;
  int w_index;

  FxBool
    nopCMD;                     /* Have we placed a NOP in the FIFO ? */

  InitFIFOData  
    fifoData;

  union hwDep_u {
    struct sst96Dep_s {
#if (GLIDE_PLATFORM & GLIDE_OS_DOS32) && GLIDE_DEBUG10
      FxBool
        int10Called;            /* Did the app call int10h? */
      void (__interrupt __far *prevInt10)();
#endif
      FxU32
        writesSinceFence,       /* Writes since last fence */
        writesSinceHeader,      /* Grouped Writes since last header */
        paramMask,              /* Mask indicating required parameters */
        gwCommand,              /* Command for initiating a triangle gw packet */
        gwHeaders[4];           /* Group write headers for SST96 */
      FxU32
        *serialStatus,          /* address of serial status register */
        *fifoApertureBase;      /* base of fifo apurture (if different) */

#if 0
        *fifoPtr,               /* pointer to FIFO */
        fifoSize,               /* # bytes left in FIFO */
        *fifoVirt,              /* start virtual address */
        fifoPhys,               /* start physical address  */
        fifoMax,                /* in bytes */
        fifoCur,                /* how much used up so far */
        fifoLfb;                /* use LFB, use hostblt if 0. */
#endif
    } sst96Dep;
    struct sst1Dep_s {
      FxU32
        /* fifoFree,               # Free entries in FIFO */
        swFifoLWM;              /* fudge factor */
    } sst1Dep;
    
  } hwDep;

  FxU32 lockPtrs[2];        /* pointers to locked buffers */
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
    num_tmu;                    /* number of TMUs attached */

  FxBool
    scanline_interleaved;

  /* LFB Flags */
  FxU32 lfbSliOk;

  /* DEBUG and SANITY variables */
  FxI32 myLevel;                /* debug level */
  FxI32 counter;                /* counts bytes sent to HW */
  FxI32 expected_counter;       /* the number of bytes expected to be sent */

  sst1VideoTimingStruct         /* init code overrides */
    *vidTimings;

  FxBool open;                  /* Has GC Been Opened? */
  FxBool closedP;               /* Have we closed since an init call? (see grSstWinOpen) */
} GrGC;

/* NOTE: this changes the P6FENCE macro expansion from sst1init.h !!! */
#define p6FenceVar _GlideRoot.p6Fencer

/* if we are debugging, call a routine so we can trace fences */
#ifdef GLIDE_DEBUG
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
#define GR_P6FENCE {_grFence(); gc->hwDep.sst96Dep.writesSinceFence = 0;}
#else  /* SST1 */
#define GR_P6FENCE _grFence();
#endif /* SST1 or SST96 */
#else  /* Not DEBUG */
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
#define GR_P6FENCE {P6FENCE; gc->hwDep.sst96Dep.writesSinceFence = 0;}
#else  /* SST1 */
#define GR_P6FENCE P6FENCE
#endif /* SST1 or SST96 */
#endif /* DEBUG or not */

/*
**  The root of all Glide data, all global data is in here
**  stuff near the top is accessed a lot
*/
struct _GlideRoot_s {
#if defined(__WATCOMC__) || defined(__MSC__) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
  int p6Fencer;                 /* xchg to here to keep this in cache!!! */
#endif
  int current_sst;
#if GL_X86
  FxU32 CPUType;
#endif
  GrGC *curGC;                  /* point to the current GC      */
  FxI32 curTriSize;             /* the size in bytes of the current triangle */
  FxI32 curTriSizeNoGradient;   /* special for _trisetup_nogradients */
  FxU32 packerFixAddress;       /* address to write packer fix to */
  FxBool    windowsInit;        /* Is the Windows part of glide initialized? */

#if GLIDE_MULTIPLATFORM
  GrGCFuncs
    curGCFuncs;                 /* Current dd Function pointer table */
#endif
  int initialized;

  struct {                      /* constant pool (minimizes cache misses) */
    float f0;
    float fHalf;
    float f1;
    float f255;
    float f256;
    /* temps to convert floats to ints */
    union { float f; FxI32 i; FxU32 u; } temp1;
    union { float f; FxI32 i; FxU32 u; } temp2;
  } pool;

  struct {                      /* environment data             */
    FxBool ignoreReopen;
    FxBool triBoundsCheck;      /* check triangle bounds        */
    FxBool noSplash;            /* don't draw it                */
    FxBool shamelessPlug;       /* translucent 3Dfx logo in lower right */
    FxU32  sst2Flags;           /* research flags for SST-2     */
    FxI32  swapInterval;        /* swapinterval override        */
    FxI32  swFifoLWM;
    FxU32  snapshot;            /* register trace snapshot      */
    FxBool disableDitherSub;    /* Turn off dither subtraction? */
    FxI32  swapPendingCount;    /* pending buffer swap count    */
  } environment;

  struct {
    FxU32       minMemFIFOFree;
    FxU32       minPciFIFOFree;

    FxU32       fifoSpins;      /* number of times we spun on fifo */

    FxU32       bufferSwaps;    /* number of buffer swaps       */
    FxU32       pointsDrawn;
    FxU32       linesDrawn;
    FxU32       trisProcessed;
    FxU32       trisDrawn;

    FxU32       texDownloads;   /* number of texDownload calls   */
    FxU32       texBytes;       /* number of texture bytes downloaded   */

    FxU32       palDownloads;   /* number of palette download calls     */
    FxU32       palBytes;       /* number of palette bytes downloaded   */
  } stats;

  GrHwConfiguration     hwConfig;
  
  GrGC                  GCs[MAX_NUM_SST];       /* one GC per board     */
};


extern struct _GlideRoot_s GR_CDECL _GlideRoot;
#if GLIDE_MULTIPLATFORM
extern GrGCFuncs _curGCFuncs;
#endif
/*==========================================================================*/
/* Macros for declaring functions */
#define GR_DDFUNC(name, type, args) \
type FX_CSTYLE name args

#define GR_ENTRY(name, type, args) \
FX_ENTRY type FX_CSTYLE name args

#define GR_DIENTRY(name, type, args) \
type FX_CSTYLE name args

#define GR_STATE_ENTRY(name, type, args) \
   type _##name args

/*==========================================================================*/

#define STATE_REQUIRES_IT_DRGB  FXBIT(0)
#define STATE_REQUIRES_IT_ALPHA FXBIT(1)
#define STATE_REQUIRES_OOZ      FXBIT(2)
#define STATE_REQUIRES_OOW_FBI  FXBIT(3)
#define STATE_REQUIRES_W_TMU0   FXBIT(4)
#define STATE_REQUIRES_ST_TMU0  FXBIT(5)
#define STATE_REQUIRES_W_TMU1   FXBIT(6)
#define STATE_REQUIRES_ST_TMU1  FXBIT(7)
#define STATE_REQUIRES_W_TMU2   FXBIT(8)
#define STATE_REQUIRES_ST_TMU2  FXBIT(9)

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

#if (GLIDE_PLATFORM & GLIDE_HW_SST96) /* Stuff for group write packet header */

#define GWH_VTX_AX_BIT          FXBIT(0)
#define GWH_VTX_AY_BIT          FXBIT(1)
#define GWH_VTX_BX_BIT          FXBIT(2)
#define GWH_VTX_BY_BIT          FXBIT(3)
#define GWH_VTX_CX_BIT          FXBIT(4)
#define GWH_VTX_CY_BIT          FXBIT(5)
#define GWH_FSTARTR_BIT         FXBIT(6)
#define GWH_FDRDX_BIT           FXBIT(7)
#define GWH_FDRDY_BIT           FXBIT(8)
#define GWH_FSTARTG_BIT         FXBIT(9)
#define GWH_FDGDX_BIT           FXBIT(10)
#define GWH_FDGDY_BIT           FXBIT(11)
#define GWH_FSTARTB_BIT         FXBIT(12)
#define GWH_FDBDX_BIT           FXBIT(13)
#define GWH_FDBDY_BIT           FXBIT(14)
#define GWH_FSTARTZ_BIT         FXBIT(15)
#define GWH_FDZDX_BIT           FXBIT(16)
#define GWH_FDZDY_BIT           FXBIT(17)
#define GWH_FSTARTA_BIT         FXBIT(18)
#define GWH_FDADX_BIT           FXBIT(19)
#define GWH_FDADY_BIT           FXBIT(20)
#define GWH_FSTARTS_BIT         FXBIT(21)
#define GWH_FDSDX_BIT           FXBIT(22)
#define GWH_FDSDY_BIT           FXBIT(23)
#define GWH_FSTARTT_BIT         FXBIT(24)
#define GWH_FDTDX_BIT           FXBIT(25)
#define GWH_FDTDY_BIT           FXBIT(26)
#define GWH_FSTARTW_BIT         FXBIT(27)
#define GWH_FDWDX_BIT           FXBIT(28)
#define GWH_FDWDY_BIT           FXBIT(29)
#define GWH_FTRIANGLECMD_BIT    FXBIT(30)
#define GWH_ENABLE_BIT          FXBIT(31)

#define GWH_VTX_BITS (GWH_VTX_AX_BIT | GWH_VTX_AY_BIT | \
                      GWH_VTX_BX_BIT | GWH_VTX_BY_BIT | \
                      GWH_VTX_CX_BIT | GWH_VTX_CY_BIT)
#define GWH_RGB_BITS (GWH_FSTARTR_BIT | GWH_FDRDX_BIT | GWH_FDRDY_BIT |\
                      GWH_FSTARTG_BIT | GWH_FDGDX_BIT | GWH_FDGDY_BIT |\
                      GWH_FSTARTB_BIT | GWH_FDBDX_BIT | GWH_FDBDY_BIT)
#define GWH_Z_BITS (GWH_FSTARTZ_BIT |  GWH_FDZDX_BIT | GWH_FDZDY_BIT)
#define GWH_A_BITS (GWH_FSTARTA_BIT | GWH_FDADX_BIT | GWH_FDADY_BIT)
#define GWH_ST_BITS (GWH_FSTARTS_BIT | GWH_FDSDX_BIT | GWH_FDSDY_BIT |\
                     GWH_FSTARTT_BIT | GWH_FDTDX_BIT | GWH_FDTDY_BIT)
#define GWH_W_BITS (GWH_FSTARTW_BIT | GWH_FDWDX_BIT | GWH_FDWDY_BIT)

/* All gradient bits... used in slimy trisetup_nogradients hack */
#define GWH_DXY_BITS ( \
        GWH_FDRDX_BIT | GWH_FDRDY_BIT | GWH_FDGDX_BIT | GWH_FDGDY_BIT | \
        GWH_FDBDX_BIT | GWH_FDBDY_BIT | GWH_FDZDX_BIT | GWH_FDZDY_BIT | \
        GWH_FDADX_BIT | GWH_FDADY_BIT | GWH_FDSDX_BIT | GWH_FDSDY_BIT | \
        GWH_FDTDX_BIT | GWH_FDTDY_BIT | GWH_FDWDX_BIT | GWH_FDWDY_BIT )

void
_grDebugGroupWriteHeader(FxU32 header, FxU32 address);

#if defined(GDBG_INFO_ON)
#define DUMPGWH(header, address) {\
  _grDebugGroupWriteHeader(header, address);\
}    
#else
#define DUMPGWH(header,address)
#endif

#ifdef GLIDE_SANITY_SIZE
#  define GW_UPDATE_COUNT(a)  (gc->counter += a)
#else
#  define GW_UPDATE_COUNT(a)
#endif

#define FENCE_AMOUNT    128

#define GWH_REG_SHIFT     2
#define GWH_WRAP_SHIFT   12
#define GWH_CHIP_SHIFT   16

#define GWH_REG_OFFSET(a) ((FxU32) (a) - (FxU32) gc->reg_ptr)
#define GWH_REG(a) ((GWH_REG_OFFSET(a) >> GWH_REG_SHIFT) & 0xff)
#define GWH_WRAP(a) (((FxU32)(a) >> GWH_WRAP_SHIFT) & 0xf)
#define GWH_CHIP(a) (((FxU32)(a) >> GWH_CHIP_SHIFT) & 0xf)

#define GWH_GEN_WRAP(w) (((w) & 0xf) << GWH_WRAP_SHIFT)
#define GWH_GEN_CHIP(c) (((c) & 0xf) << GWH_CHIP_SHIFT)

#define GEN_PCI_ADDRESS 

#define GWH_FENCE_TRIANGLE \
if ((gc->hwDep.sst96Dep.writesSinceFence + (_GlideRoot.curTriSize>>2 ) + 4) > FENCE_AMOUNT)\
  GR_P6FENCE

#define GW_TRICMD_MASK 0x40000000

#define GWH_BUILD_PCI_ADDRESS(a,c,w)\
  (GWH_REG(a) | (GWH_WRAP(w) << GWH_WRAP_SHIFT) | (GWH_CHIP(c) << GWH_CHIP_SHIFT))

#define PUMA_DWORD_REG_OFFSET 0x100000

#define GWH_GEN_ADDRESS(a) \
  ((((FxU32)(a) - (FxU32) gc->lfb_ptr) >> 2) | PUMA_DWORD_REG_OFFSET | GWH_ENABLE_BIT)

#ifdef DEBUG
#  define GWH_RESET_WSH gc->hwDep.sst96Dep.writesSinceHeader = 0
#  define GWH_INC_WSH gc->hwDep.sst96Dep.writesSinceHeader++
#else
#  define GWH_RESET_WSH
#  define GWH_INC_WSH
#endif

#if defined( GLIDE_DEBUG )
  /* defined in GPCI.C */
extern void GR_CDECL
_GR_SET_GW_CMD( volatile void *addr, unsigned long data );
extern void GR_CDECL
_GR_SET_GW_HEADER( volatile void *addr, unsigned long data );
extern void GR_CDECL
_GR_SET_GW_ENTRY( volatile void *addr, unsigned long data );

#define SET_GW_CMD( addr, offset, data )    \
_GR_SET_GW_CMD( ((FxU32*)(addr))+(offset), (data) ); GR_INC_SIZE(4)

#define SET_GW_HEADER( addr, offset, data ) \
_GR_SET_GW_HEADER( ((FxU32*)(addr))+(offset), (data) ); GR_INC_SIZE(4)

#define FSET_GW_ENTRY( addr, offset, data )  \
{                                            \
   float _tacoFTemp;                         \
   _tacoFTemp = (data);                      \
   _GR_SET_GW_ENTRY( ((FxU32*)(addr))+(offset), (*(FxU32*)&_tacoFTemp) ); \
   GR_INC_SIZE(4);\
}
#else

#define SET_GW_CMD( addr, offset, data )    \
*(addr+offset)=data;

#define SET_GW_HEADER( addr, offset, data ) \
*(addr+offset)=data;

#define FSET_GW_ENTRY( addr, offset, data )  \
*((float*)(addr+offset))=data;

#endif

#if SST96_ALT_FIFO_WRAP
#define GWH_PACKET_SIZE_CHECK(__s) \
  if (gc->fifoData.hwDep.vg96FIFOData.fifoSize < (FxU32) ((__s) + 12)) {\
    gc->fifoData.hwDep.vg96FIFOData.blockSize = (__s) + 12; \
    initWrapFIFO(&gc->fifoData); \
  }
#else
#define GWH_PACKET_SIZE_CHECK(__s) \
  if (gc->fifoData.hwDep.vg96FIFOData.fifoSize < (FxU32) ((__s) + 12)) {\
    _grSst96FifoMakeRoom();\
    GR_ASSERT(!(gc->fifoData.hwDep.vg96FIFOData.fifoSize < 0x1000));\
  }
#endif

#define GWH_BEGIN_TRIANGLE_PACKET(header, address) \
{\
  GWH_RESET_WSH;\
  GR_ASSERT(!(((FxU32)(gc->fifoData.hwDep.vg96FIFOData.fifoPtr) & 0x7)));\
  GWH_FENCE_TRIANGLE;\
  GDBG_INFO((120, "Beginning Grouped Write Packet\n"));\
  GDBG_INFO((120, "fifoPtr = 0x%x fifoSize = 0x%x\n", gc->fifoData.hwDep.vg96FIFOData.fifoPtr, gc->fifoData.hwDep.vg96FIFOData.fifoSize));\
  GDBG_INFO((120, "Base = 0x%x Mask = 0x%x\n", (FxU32) address, header));\
  GW_UPDATE_COUNT(8);\
  GWH_PACKET_SIZE_CHECK(_GlideRoot.curTriSize); \
  DUMPGWH(header,address);\
  GLIDE_FIFO_CHECK(); \
  *gc->fifoData.hwDep.vg96FIFOData.fifoPtr++ = GWH_GEN_ADDRESS(address);\
  *gc->fifoData.hwDep.vg96FIFOData.fifoPtr++ = header;\
  gc->fifoData.hwDep.vg96FIFOData.fifoSize -= 8;\
  gc->hwDep.sst96Dep.writesSinceFence += 2;\
}

#define GWH_FENCE_TEXDL \
if ((gc->hwDep.sst96Dep.writesSinceFence + 34) >= FENCE_AMOUNT)\
  GR_P6FENCE

#define GWH_BEGIN_TEXDL_PACKET(header, address) \
{\
  GWH_RESET_WSH;\
  GR_ASSERT(!(((FxU32)(gc->fifoData.hwDep.vg96FIFOData.fifoPtr) & 0x7)));\
  GWH_FENCE_TEXDL;\
  GDBG_INFO((120, "Beginning Texture Grouped Write Packet\n"));\
  GDBG_INFO((120, "fifoPtr = 0x%x fifoSize = 0x%x\n", gc->fifoData.hwDep.vg96FIFOData.fifoPtr, gc->fifoData.hwDep.vg96FIFOData.fifoSize));\
  GDBG_INFO((120, "Base = 0x%x Mask = 0x%x\n", address, header));\
  GW_UPDATE_COUNT(8);\
  GWH_PACKET_SIZE_CHECK(34 << 2); \
  GLIDE_FIFO_CHECK(); \
  DUMPGWH(header,((((FxU32) address - (FxU32) gc->lfb_ptr)) >> 2) | GWH_ENABLE_BIT);\
  *gc->fifoData.hwDep.vg96FIFOData.fifoPtr++ =\
      ((((FxU32) address - (FxU32) gc->lfb_ptr)) >> 2) | GWH_ENABLE_BIT;\
  *gc->fifoData.hwDep.vg96FIFOData.fifoPtr++ = header;\
  gc->fifoData.hwDep.vg96FIFOData.fifoSize -= 8;\
  gc->hwDep.sst96Dep.writesSinceFence += 2;\
}

#define GR_SETF_GW(s) {\
GLIDE_FIFO_CHECK(); \
GR_ASSERT(gc->hwDep.sst96Dep.writesSinceHeader < 32);\
GDBG_INFO((120, "Group Write:  fifoPtr = 0x%x, value = %4.2f sinceFence = 0x%x\n", gc->fifoData.hwDep.vg96FIFOData.fifoPtr, s, gc->hwDep.sst96Dep.writesSinceFence));\
*((float *) gc->fifoData.hwDep.vg96FIFOData.fifoPtr++) = s;\
gc->fifoData.hwDep.vg96FIFOData.fifoSize-=4;\
GW_UPDATE_COUNT(4);\
GR_ASSERT(gc->hwDep.sst96Dep.writesSinceFence <= FENCE_AMOUNT);\
gc->hwDep.sst96Dep.writesSinceFence++;\
GWH_INC_WSH;\
}

#define GR_SET_GW(s) {\
GLIDE_FIFO_CHECK(); \
GR_ASSERT(gc->hwDep.sst96Dep.writesSinceHeader < 32);\
GDBG_INFO((120, "Group Write:  fifoPtr = 0x%x, value = 0x%x sinceFence=%d\n", gc->fifoData.hwDep.vg96FIFOData.fifoPtr, s, gc->hwDep.sst96Dep.writesSinceFence));\
P6FENCEFIFO;\
*gc->fifoData.hwDep.vg96FIFOData.fifoPtr++ = s;\
gc->fifoData.hwDep.vg96FIFOData.fifoSize-=4;\
GW_UPDATE_COUNT(4);\
GR_ASSERT(gc->hwDep.sst96Dep.writesSinceFence <= FENCE_AMOUNT);\
gc->hwDep.sst96Dep.writesSinceFence++;\
GWH_INC_WSH;\
}

#endif /* stuff for group write packet header */

/*==========================================================================*/
/*
**  Here's the infamous Packer Bug Check and Workaround:   
**  XOR the two addresses together to find out which bits are different.
**  AND the result with the bits that represent the chip field of the 
**  SST address.  If ANY of them are different, then do the packer hack.
**  Save this address as the last with which we compared.
*/  

#define SST_CHIP_MASK 0x3C00

#if   ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
#define GLIDE_DRIVER_NAME "Voodoo Graphics"
#elif   ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
#define GLIDE_DRIVER_NAME "Voodoo Rush"
#else
#define GLIDE_DRIVER_NAME "HOOPTI???"
#endif

#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
  #define PACKER_WORKAROUND_SIZE 4

#if GL_X86
  #define PACKER_WORKAROUND \
    if (_GlideRoot.CPUType == 6) {\
      GR_P6FENCE;\
      GR_SET( *(FxU32 *)_GlideRoot.packerFixAddress, 0 );\
      GR_P6FENCE;\
    } \
    else {\
      GR_SET( *(FxU32 *)_GlideRoot.packerFixAddress, 0 ); \
    }
#else
  #define PACKER_WORKAROUND GR_SET(*(FxU32 *)_GlideRoot.packerFixAddress, 0);
#endif

  #define PACKER_BUGCHECK(a)\
    if ( (((FxU32) a) ^ lastAddress) & SST_CHIP_MASK ) {\
      PACKER_WORKAROUND;        \
      lastAddress = (FxU32)a;   \
    }

#else
  #define PACKER_WORKAROUND_SIZE 0
  #define PACKER_WORKAROUND
  #define PACKER_BUGCHECK(a)
#endif

/* On a P6 we have to fence around commands to ensure write-ordering */
#if 0
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
#  ifdef GR_P6FENCE
#    undef GR_P6FENCE
#  endif
#  define GR_P6FENCE
#endif
#endif

/* GMT: a very useful macro for making sure that SST commands are properly
   fenced on a P6, e.g. P6FENCH_CMD( GR_SET(hw->nopCMD,1) );
*/
#if (GLIDE_PLATFORM & GLIDE_HW_SST1) && GL_X86
#define P6FENCE_CMD( cmd ) \
    if (_GlideRoot.CPUType == 6) {     /* if it's a p6 */ \
      GR_P6FENCE;                      /* then fence off the cmd */ \
      cmd; \
      GR_P6FENCE;                      /* to ensure write order  */ \
    } \
    else \
      cmd
#else
#define P6FENCE_CMD( cmd ) cmd
#endif

/*==========================================================================*/
#ifndef FX_GLIDE_NO_FUNC_PROTO

void _grMipMapInit(void);
FxI32 FX_CSTYLE
_trisetup_asm(const void *va, const void *vb, const void *vc );
FxI32 FX_CSTYLE
_trisetup(const void *va, const void *vb, const void *vc );
FxI32 FX_CSTYLE
_trisetup_nogradients(const void *va, const void *vb, const void *vc );
#endif /* FX_GLIDE_NO_FUNC_PROTO */

/* GMT: BUG need to make this dynamically switchable */
#ifdef GLIDE_USE_C_TRISETUP
  #define TRISETUP _trisetup
#else
  #define TRISETUP _trisetup_asm
#endif /* GLIDE_USE_C_TRISETUP */

/*==========================================================================*/
/* 
**  Function Prototypes
*/

FxI32 FX_CSTYLE
_trisetup_mixed_datalist(const void *va, const void *vb, const void *vc, FxBool aflag, FxBool bflag, FxBool cflag );

FxI32 FX_CSTYLE
_vp_trisetup_mixed_datalist(const void *va, const void *vb, const void *vc, FxBool aflag, FxBool bflag, FxBool cflag );

#ifdef GLIDE_DEBUG
FxBool
_grCanSupportDepthBuffer( void );
#endif

void
_grClipNormalizeAndGenerateRegValues(FxU32 minx, FxU32 miny, FxU32 maxx,
                                     FxU32 maxy, FxU32 *clipLeftRight,
                                     FxU32 *clipBottomTop);

/*
** extension
*/
void FX_CALL 
guQueryResolutionXY(GrScreenResolution_t res, FxU32 *x, FxU32 *y);

/*
** state routine
*/

void 
_grSwizzleColor( GrColor_t *color );

void
_grDisplayStats(void);

void
_GlideInitEnvironment(void);

void FX_CSTYLE
_grColorCombineDelta0Mode( FxBool delta0Mode );

void
_doGrErrorCallback( const char *name, const char *msg, FxBool fatal );

void _grErrorDefaultCallback( const char *s, FxBool fatal );

#ifdef __WIN32__
void _grErrorWindowsCallback( const char *s, FxBool fatal );
#endif /* __WIN32__ */

extern void
(*GrErrorCallback)( const char *string, FxBool fatal );

void GR_CDECL
_grFence( void );

int
_guHeapCheck( void );

void FX_CSTYLE
_grRebuildDataList( void );

void
_grReCacheFifo( FxI32 n );

FxI32 GR_CDECL
_grSpinFifo( FxI32 n );

void
_grShamelessPlug(void);

FxBool
_grSstDetectResources(void);

FxU16
_grTexFloatLODToFixedLOD( float value );

void FX_CSTYLE
_grTexDetailControl( GrChipID_t tmu, FxU32 detail );

void FX_CSTYLE
_grTexDownloadNccTable( GrChipID_t tmu, FxU32 which,
                        const GuNccTable *ncc_table,
                        int start, int end );
void FX_CSTYLE
_grTexDownloadPalette( GrChipID_t   tmu, 
                       GuTexPalette *pal,
                       int start, int end );

FxU32
_grTexCalcBaseAddress(
                      FxU32 start_address, GrLOD_t largeLod,
                      GrAspectRatio_t aspect, GrTextureFormat_t fmt,
                      FxU32 odd_even_mask ); 

void
_grTexForceLod( GrChipID_t tmu, int value );

FxU32
_grTexTextureMemRequired( GrLOD_t small_lod, GrLOD_t large_lod, 
                           GrAspectRatio_t aspect, GrTextureFormat_t format,
                           FxU32 evenOdd );
void FX_CSTYLE
_grUpdateParamIndex( void );

#if 0
FX_ENTRY void FX_CALL
grSstConfigPipeline(GrChipID_t chip, GrSstRegister reg, FxU32 value);
#endif

/*==========================================================================*/
/*  GMT: have to figure out when to include this and when not to
*/
#if defined(GLIDE_DEBUG) || defined(GLIDE_ASSERT) || defined(GLIDE_SANITY_ASSERT) || defined(GLIDE_SANITY_SIZE)
#define DEBUG_MODE 1
#include <assert.h>

#if SST96_FIFO
/* sst96.c */
extern void
_grSst96CheckFifoData(void);

#define GLIDE_FIFO_CHECK() _grSst96CheckFifoData()
#else
#define GLIDE_FIFO_CHECK()
#endif
#else
#define GLIDE_FIFO_CHECK()
#endif

#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
/* NOTE: fifoFree is the number of entries, each is 8 bytes */
#define GR_CHECK_FOR_ROOM(n) \
{ \
  FxI32 fifoFree = gc->state.fifoFree - (n); \
  if (fifoFree < 0)          \
    fifoFree = _grSpinFifo(n); \
  gc->state.fifoFree = fifoFree;\
}
#elif (GLIDE_PLATFORM & GLIDE_HW_SST96)
/* NOTE: fifoSize is in bytes, and each fifo entry is 8 bytes.  Since
   the fifoSize element of the sst96Dep data structure must be
   accurate, we subtract after we write, instead of at the beginning
   as above. */
#if (GLIDE_PLATFORM & GLIDE_OS_DOS32) && GLIDE_DEBUG10
#define GR_CHECKINT10 if (gc->hwDep.sst96Dep.int10Called)_doGrErrorCallback("Glide Error:", "Application called Int 10 between grSstWinOpen and Close.\n", FXTRUE)
#else
#define GR_CHECKINT10
#endif

#if SST96_ALT_FIFO_WRAP

#define GR_CHECK_FOR_ROOM(n) \
{\
  FxI32 fifoSize = gc->fifoData.hwDep.vg96FIFOData.fifoSize - ((n) << 1);\
  if (fifoSize < 0) {\
    gc->fifoData.hwDep.vg96FIFOData.blockSize = ((n) << 1); \
    initWrapFIFO(&gc->fifoData); \
     GR_CHECKINT10;\
  }\
}
#else /* !SST96_ALT_FIFO_WRAP */
#define GR_CHECK_FOR_ROOM(n) \
{\
  FxI32 fifoSize = gc->fifoData.hwDep.vg96FIFOData.fifoSize - ((n) << 1);\
  if (fifoSize < 0) {\
    _grSst96FifoMakeRoom();\
    GR_ASSERT(!(gc->fifoData.hwDep.vg96FIFOData.fifoSize < 0x1000));\
    GR_CHECKINT10;\
  }\
}
#endif /* !SST96_ALT_FIFO_WRAP */
#endif

#ifdef GLIDE_SANITY_SIZE

#define FIFO_SUB_VAL ((FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoPtr - \
                      (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoVirt)
   
#define GR_CHECK_SIZE() \
   if(gc->counter != gc->expected_counter) \
   GDBG_ERROR("GR_ASSERT_SIZE","byte counter should be %d but is %d\n", \
              gc->expected_counter,gc->counter); \
   ASSERT(gc->counter == gc->expected_counter); \
   gc->counter = gc->expected_counter = 0
#define GR_CHECK_SIZE_SLOPPY() \
   if(gc->counter > gc->expected_counter) \
     GDBG_ERROR("GR_ASSERT_SIZE","byte counter should be < %d but is %d\n", \
                gc->expected_counter,gc->counter); \
   ASSERT(gc->counter <= gc->expected_counter); \
   gc->counter = gc->expected_counter = 0
#define GR_SET_EXPECTED_SIZE(n) \
   GLIDE_FIFO_CHECK(); \
   GDBG_INFO((gc->myLevel, \
              "FIFO: 0x%X 0x%X (0x%X)\n", \
              (n), \
              (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoPtr, \
              FIFO_SUB_VAL)); \
   GDBG_INFO((gc->myLevel, \
              "FIFOSize: 0x%X\n", \
              (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoSize));\
   ASSERT(gc->counter == 0); \
   ASSERT(gc->expected_counter == 0); \
   GR_CHECK_FOR_ROOM((n)); \
   gc->expected_counter = n
#define GR_INC_SIZE(n) gc->counter += n
#else /* !GLIDE_SANITY_SIZE */
   /* define to do nothing */
#define GR_CHECK_SIZE_SLOPPY()
#define GR_CHECK_SIZE()
#define GR_SET_EXPECTED_SIZE(n) GR_CHECK_FOR_ROOM((n))
#define GR_INC_SIZE(n)
#endif /* !GLIDE_SANITY_SIZE */

#define GR_DCL_GC GrGC *gc = _GlideRoot.curGC
#define GR_DCL_HW Sstregs *hw = (Sstregs *)gc->reg_ptr

#ifdef DEBUG_MODE
  #define ASSERT(exp) GR_ASSERT(exp)

  #define GR_BEGIN_NOFIFOCHECK(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                static char myName[] = name;  \
                GR_ASSERT(gc != NULL);  \
                GR_ASSERT(hw != NULL);  \
                gc->myLevel = level;    \
                GDBG_INFO((gc->myLevel,"%s\n", myName)); \
                FXUNUSED( hw )

#else
  #define ASSERT(exp)

  #define GR_BEGIN_NOFIFOCHECK(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                FXUNUSED( hw )
#endif

#define GR_BEGIN(name,level,size) \
                GR_BEGIN_NOFIFOCHECK(name,level); \
                GR_SET_EXPECTED_SIZE(size)

#if SST96_ALT_FIFO_WRAP
#define GR_EXIT_TRACE   \
           GLIDE_FIFO_CHECK(); \
           GDBG_INFO((gc->myLevel, \
                      "%s Done: FifoPtr(0x%X) FifoTest(0x%X)\n", \
                      myName, \
                      (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoPtr, \
                      gc->fifoData.hwDep.vg96FIFOData.fifoSize))
#else
#define GR_EXIT_TRACE   GDBG_INFO((gc->myLevel,  "%s Done\n", myName))
#endif

#define GR_END()        {GR_CHECK_SIZE(); GR_EXIT_TRACE;}
#define GR_END_SLOPPY() {GR_CHECK_SIZE_SLOPPY(); GR_EXIT_TRACE;}
#define GR_RETURN(val) \
                if ( GDBG_GET_DEBUGLEVEL(gc->myLevel) ) { \
                  GR_CHECK_SIZE(); \
                } \
                else \
                  GR_END(); \
                GDBG_INFO((gc->myLevel,"%s() => 0x%x---------------------\n",myName,val,val)); \
                return val

#ifndef GR_ASSERT               
#if defined(GLIDE_SANITY_ASSERT)
#  define GR_ASSERT(exp) if (!(exp)) _grAssert(#exp,  __FILE__, __LINE__)
#else
#  define GR_ASSERT(exp) 
#endif
#endif

#if defined(GLIDE_DEBUG)
  #define GR_CHECK_F(name,condition,msg) \
    if ( condition ) _doGrErrorCallback( name, msg, FXTRUE )
  #define GR_CHECK_W(name,condition,msg) \
    if ( condition ) _doGrErrorCallback( name, msg, FXFALSE )
#else
  #define GR_CHECK_F(name,condition,msg)
  #define GR_CHECK_W(name,condition,msg)
#endif

/* macro define some basic and common GLIDE debug checks */
#define GR_CHECK_TMU(name,tmu) \
  GR_CHECK_F(name, tmu < GR_TMU0 || tmu >= gc->num_tmu , "invalid TMU specified")


FxBool
_grSst96PCIFifoEmpty(void);
FxU32
_grSst96Load32(FxU32 *s);
void
_grSst96Store32(FxU32 *d, FxU32 s);
void
_grSst96Store16(FxU16 *d, FxU16 s);
void 
_grSst96Store32F(float *d, float s);
void
_grAssert(char *, char *, int);

#if defined(SST96)
/* Since we're ganking init off the simulator, and if the simulator is set
   up, sst.h defines SET,GET,SETF, and SET16 to be function calls to the
   simulator, we need to #undef them here */
#undef SET
#undef SETF
#undef SET16

void GR_CDECL
_grSst96FifoMakeRoom(void);

#define SST96_LFB_OFFSET 0x000000
#define SST96_REG_OFFSET 0x400000
#define SST96_TEX_OFFSET 0x600000

#ifdef SST96_FIFO
#ifdef GDBG_INFO_ON
void
_grFifoWriteDebug(FxU32 addr, FxU32 val, FxU32 fifoPtr);
#define DEBUGFIFOWRITE(a,b,c) \
_grFifoWriteDebug((FxU32) a, (FxU32) b, (FxU32) c)
void
_grFifoFWriteDebug(FxU32 addr, float val, FxU32 fifoPtr);
#define DEBUGFIFOFWRITE(a,b,c) \
_grFifoFWriteDebug((FxU32) a, (float) b, (FxU32) c)
#else
#define DEBUGFIFOWRITE(a,b,c)
#define DEBUGFIFOFWRITE(a,b,c)
#endif

/*  
**  Here lies the infamous Nudge Of Love.  The Nudge Of Love is
**  required to on P6s, where the read pointer trails the write
**  pointer by 256 bytes (64 DWORDS).  The problem with that is
**  sometimes we really need the hardware to have read all the data we
**  have sent--thus the Nudge, which gently strokes the read pointer
**  through the last commands we've placed in the commadn FIFO.
**
**  That would be all well and good except for one thing:  If the
**  frame time is very short, Jr, doesn't like the Nudge Of Love.  So,
**  instead of a shitpot of nopCmds, we must draw a smallish
**  triangle.  The problem with that, however, is the app may be
**  reusing the contents of the color or depth buffers.  Thus, we have
**  to disable the color and depth mask before drawing the triangle
**  and replace the shadowed values after we draw it.
**
* dpc - 19 aug 1997 - FixMe!
* This macro is called the P6_NUDGE_OF_LOVE, but it was not previously
* conditional on running on a p6. Is this right? I moved the set expected
* size thing into here, and made usages of the macro not set the expected
* size and do the sloppy check.
*/
#if GL_X86
#define P6_NUDGE_OF_LOVE \
if (_GlideRoot.CPUType == 6) {\
  int i;\
  GR_SET_EXPECTED_SIZE(32 << 2); \
  GR_SET(hw->fbzMode, (gc->state.fbi_config.fbzMode & (~(SST_RGBWRMASK | SST_ZAWRMASK))));\
  GR_SETF(hw->FvA.x, 0.f);\
  GR_SETF(hw->FvA.y, 0.f);\
  GR_SETF(hw->FvB.x, 100.f);\
  GR_SETF(hw->FvB.y, 0.f);\
  GR_SETF(hw->FvC.x, 100.f);\
  GR_SETF(hw->FvC.y, 100.f);\
  GR_SETF(hw->FtriangleCMD, 1.f);\
  GR_SET(hw->fbzMode, gc->state.fbi_config.fbzMode);\
  for (i = 0; i < 23; i++) GR_SET(hw->nopCMD, 0); \
  GR_CHECK_SIZE(); \
}
#else
#define P6_NUDGE_OF_LOVE
#endif

#if GL_X86
#define P6FENCEFIFO \
if (_GlideRoot.CPUType == 6) {\
  GR_ASSERT(gc->hwDep.sst96Dep.writesSinceFence <= FENCE_AMOUNT);\
  if (gc->hwDep.sst96Dep.writesSinceFence == FENCE_AMOUNT) {\
    GR_P6FENCE;\
  }\
}
#else
#define P6FENCEFIFO
#endif

#define SST96_STORE_FIFO(addr,val){\
  GR_ASSERT(!(((FxU32)(gc->fifoData.hwDep.vg96FIFOData.fifoPtr) & 0x7)));\
  GR_ASSERT(gc->fifoData.hwDep.vg96FIFOData.fifoSize > 0);\
  P6FENCEFIFO;\
  DEBUGFIFOWRITE(addr, val, gc->fifoData.hwDep.vg96FIFOData.fifoPtr);\
  GLIDE_FIFO_CHECK(); \
  *gc->fifoData.hwDep.vg96FIFOData.fifoPtr++ =\
   (((FxU32) ((FxU32) addr) - ((FxU32) gc->lfb_ptr)) >> 2);\
  *gc->fifoData.hwDep.vg96FIFOData.fifoPtr++ = val;\
  gc->hwDep.sst96Dep.writesSinceFence+=2;\
  gc->fifoData.hwDep.vg96FIFOData.fifoSize -= 8;\
  GLIDE_FIFO_CHECK(); \
}

#define SST96_STOREF_FIFO(addr,fVal){\
  GR_ASSERT(!(((FxU32)(gc->fifoData.hwDep.vg96FIFOData.fifoPtr) & 0x7)));\
  GR_ASSERT(gc->fifoData.hwDep.vg96FIFOData.fifoSize);\
  P6FENCEFIFO;\
  DEBUGFIFOFWRITE((addr), fVal, gc->fifoData.hwDep.vg96FIFOData.fifoPtr);\
  GLIDE_FIFO_CHECK(); \
  *gc->fifoData.hwDep.vg96FIFOData.fifoPtr++ =\
   (((addr) - ((FxU32) gc->lfb_ptr)) >> 2);\
  *((float *)gc->fifoData.hwDep.vg96FIFOData.fifoPtr) = fVal;\
    gc->fifoData.hwDep.vg96FIFOData.fifoPtr++;\
  gc->hwDep.sst96Dep.writesSinceFence+=2;\
  gc->fifoData.hwDep.vg96FIFOData.fifoSize -= 8;\
  GLIDE_FIFO_CHECK(); \
}

#define SET16(d,s)      SST96_STORE_FIFO((&d), (FxU32) s)
#define SET(d,s)        SST96_STORE_FIFO((&d),s)
#define SETF(d,s)       SST96_STOREF_FIFO(((FxU32)(&(d))), s)

/*
**  Here are some macro for the very few times when we must perform a
**  direct write of the hardware--even when using the memory-backed
**  command FIFO.
*/
#ifndef GDBG_INFO_ON
#define SET_DIRECT(d,s) (hw->d) = (s)
#define SETF_DIRIECT(d,s) *((float *) &hw->d) = s
#else
#define REGOFFSET(d) (((FxU32) &hw->d - (FxU32) hw) >> 2)
#define SET_DIRECT(d,s)\
  GDBG_INFO((120, "DIRECT SET of register 0x%x with value 0x%x\n",\
             REGOFFSET(d), s));\
  (hw->d) = (s)
#define SETF_DIRECT(d,s)\
  GDBG_INFO((120, "DIRECT SET of register 0x%x with value %4.4f\n",\
             REGOFFSET(d), s));\
  *((float *) &hw->d) = s
#endif

#else

#ifndef GDBG_INFO_ON
#define GET(s)          s
#define SET(d,s)        d = s
#define SET16(d,s)      d = s
#define SETF(d,s)       (*(float *)&(d)) = s
#else
#undef  GET

#define GET(s)          _grSst96Load32(&s)
#define SET(d,s)        _grSst96Store32((FxU32 *)&d, s)
#define SET16(d,s)      _grSst96Store16(&d,s)
#define SETF(d,s)       _grSst96Store32F((float *) &d, s)
#endif /* GDBG_INFO_ON */
#endif /* SST96_FIFO */

#endif /* defined(SST96) */

#if defined(GLIDE_DEBUG) && !(GLIDE_PLATFORM & GLIDE_SST_SIM) && !defined(SST96)
  /* if building a DEBUG library without the simulator */
  extern FxU32 GR_CDECL _GR_GET(void *);
  extern void GR_CDECL _GR_SET16(void *, unsigned short);
  extern void GR_CDECL _GR_SET(void *, unsigned long);
  extern void GR_CDECL _GR_SETF(void *, float);

  #define GR_GET(s)     _GR_GET(&(s))
  #define GR_SET(d,s)   {_GR_SET(&(d),s); SET(d,s); GR_INC_SIZE(4);}
  #define GR_SETF(d,s)  {_GR_SETF(&(d),s); SETF(d,s); GR_INC_SIZE(4);}
  #define GR_SET16(d,s) {_GR_SET16(&(d),s); SET16(d,s); GR_INC_SIZE(2);}
#else
  /* [dBorca] HACK ALERT!!! Win32 version runs amok without this... */
  #define GR_GET(s)     GET(s)
  #define GR_SET(d,s)   {GR_P6FENCE; SET(d,s); GR_INC_SIZE(4);}
  #define GR_SETF(d,s)  {SETF(d,s); GR_INC_SIZE(4);}
  #define GR_SET16(d,s) {SET16(d,s); GR_INC_SIZE(2);}
#endif



#define VG96_REGISTER_OFFSET    0x400000
#define VG96_TEXTURE_OFFSET     0x600000
/* make SST96 addresses */
#define SST96_REG_PTR(a) \
        ((FxU32 *) (((FxU32) a) + VG96_REGISTER_OFFSET))
#define SST96_LFB_PTR(a) \
        ((FxU32 *) a)
#define SST96_TEX_PTR(a) \
        ((FxU32 *) (((FxU32) a) + VG96_TEXTURE_OFFSET))

void rle_decode_line_asm(FxU16 *tlut,FxU8 *src,FxU16 *dest);

extern FxU16 rle_line[256];
extern FxU16 *rle_line_end;

#define RLE_CODE                        0xE0
#define NOT_RLE_CODE            31

#ifdef  __WATCOMC__
#pragma aux rle_decode_line_asm parm [edx] [edi] [esi] value [edi] modify exact [eax ebx ecx edx esi edi] = \
"  next_pixel:                   "  \
"  xor   ecx,ecx                 "  \
"  mov   al,byte ptr[edi]        "  \
"  mov   cl,byte ptr[edi]        "  \
"  inc   edi                     "  \
"                                "  \
"  and   al,0xE0                 "  \
"  cmp   al,0xE0                 "  \
"  jne   unique                  "  \
"                                "  \
"  and   cl,0x1F                 "  \
"  mov   al,cl                   "  \
"  jz    done_rle                "  \
"                                "  \
"  mov   cl,byte ptr[edi]        "  \
"  inc   edi                     "  \
"  mov   bx,word ptr[edx+ecx*2]  "  \
"                                "  \
"  copy_block:                   "  \
"  mov   word ptr[esi],bx        "  \
"  add   esi,0x2                 "  \
"  dec   al                      "  \
"  jz    next_pixel              "  \
"  jmp   copy_block              "  \
"                                "  \
"  unique:                       "  \
"  mov   bx,word ptr[edx+ecx*2]  "  \
"  mov   word ptr[esi],bx        "  \
"  add   esi,0x2                 "  \
"  jmp   next_pixel              "  \
"  done_rle:                     "; 
#endif /* __WATCOMC__ */

#define TEX_INFO(ptr,field)                         ptr field##Log2
#define G3_LOD_TRANSLATE(lod)                       (0x8-lod)
#define G3_ASPECT_TRANSLATE(aspect)                 (0x3-(aspect))

enum {
   kSetupStrip           = 0x00,
   kSetupFan             = 0x01,
   kSetupCullDisable     = 0x00,
   kSetupCullEnable      = 0x02,
   kSetupCullPositive    = 0x00,
   kSetupCullNegative    = 0x04,
   kSetupPingPongNorm    = 0x00,
   kSetupPingPongDisable = 0x08
};

void GR_CDECL
_grValidateState();

void FX_CSTYLE
_grDrawVertexList(FxU32 type, FxI32 mode, FxI32 count, void *pointers);

void FX_CSTYLE
_grDrawVertexListContinue(FxU32 type, FxI32 mode, FxI32 count, void *pointers);

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
_grChromaModeExt(GrChromakeyMode_t mode);
void FX_CALL 
_grChromaRangeExt( GrColor_t min, GrColor_t max , GrChromaRangeMode_t mode);

void FX_CALL 
_grTexChromaModeExt(GrChipID_t tmu, GrChromakeyMode_t mode);
void FX_CALL 
_grTexChromaRangeExt(GrChipID_t tmu, GrColor_t min, GrColor_t max);

void
_grChromaRange( GrColor_t min, GrColor_t max , GrChromaRangeMode_t mode);

void FX_CSTYLE
_guTexMemReset(void);

int FX_CSTYLE
_grBufferNumPending(void);

FxBool FX_CSTYLE
_grSstIsBusy(void);

void FX_CSTYLE
_grSstResetPerfStats(void);

FxBool FX_CSTYLE
_grSstControl(GrControl_t code);

void FX_CSTYLE
_grResetTriStats(void);

FxU32 FX_CSTYLE
_grSstStatus(void);

FxU32 FX_CSTYLE
_grSstVideoLine(void);

FxBool FX_CSTYLE
_grSstVRetraceOn(void);

FxI32 FX_CSTYLE
_grVpDrawTriangle( const void *a, const void *b, const void *c );

void FX_CSTYLE
_grDrawPoints(FxI32 mode, FxI32 count, void *pointers);

void FX_CSTYLE
_grDrawLineStrip(FxI32 mode, FxI32 count, FxI32 ltype, void *pointers);

void FX_CSTYLE
_grDrawTriangles(FxI32 mode, FxI32 count, void *pointers);

void FX_CSTYLE
_grAADrawPoints(FxI32 mode, FxI32 count, void *pointers);

void FX_CSTYLE
_grAADrawLineStrip(FxI32 mode, FxI32 ltype, FxI32 count, void *pointers);

void FX_CSTYLE
_grAADrawLines(FxI32 mode, FxI32 count, void *pointers);

void FX_CSTYLE
_grAADrawTriangles(FxI32 mode, FxI32 ttype, FxI32 count, void *pointers);

void FX_CSTYLE
_grAAVpDrawTriangle(const void *a, const void *b, const void *c,
                    FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias);

void FX_CSTYLE
_grAADrawVertexList(FxU32 type, FxI32 mode, FxI32 count, void *pointers);

void FX_CALL 
_grChromaModeExt(GrChromakeyMode_t mode);

void FX_CALL 
_grChromaRangeExt( GrColor_t min, GrColor_t max , GrChromaRangeMode_t mode);

void
_grChromakeyMode( GrChromakeyMode_t mode );

void
_grDepthMask( FxBool mask );

void
_grDepthBufferFunction( GrCmpFnc_t function );

void
_grDepthBufferMode( GrDepthBufferMode_t mode );

void
_grDitherMode( GrDitherMode_t mode );

void
_grRenderBuffer( GrBuffer_t buffer );

void
_grColorMask( FxBool rgb, FxBool a );

void
_grSstOrigin(GrOriginLocation_t  origin);

void
_grClipWindow( FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy );

void
_grDepthBiasLevel( FxI32 level );

void
_grFogMode( GrFogMode_t mode );

void
_grFogColorValue( GrColor_t fogcolor );

void
_grLfbWriteColorFormat(GrColorFormat_t colorFormat);

void
_grLfbWriteColorSwizzle(FxBool swizzleBytes, FxBool swapWords);

void
_grConstantColorValue( GrColor_t value );

void 
_grHints(GrHint_t hintType, FxU32 hints);

#endif /* __FXGLIDE_H__ */
