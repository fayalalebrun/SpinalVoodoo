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
** Revision 1.3  2003/06/28 19:31:06  guillemj
** Fix compilation warnings.
** Removed some trailing spaces.
**
** Revision 1.2  2003/06/28 19:10:36  guillemj
** Fixed preprocessor invalid tokens in newer gcc 3.x versions.
**
** Revision 1.1.1.1  1999/12/07 21:49:26  joseph
** Initial checkin into SourceForge.
**
** 
** 3     3/17/99 6:16p Dow
** Phantom Menace and other fixes.
** 
** 243   3/17/99 5:08p Peter
** removed whacky stuff now that the command fifo threshold stuff appears
** to make all happy (including the k7)
** 
** 242   3/17/99 1:37p Atai
** use grHints to enable/disable uma hack
** 
** 241   3/16/99 11:51a Atai
** Back door (set FX_GLIDE_ENABLE_UMA=1) to enable unified texture memory.
** TMUn memory size will the whole texture memory space. The offset for
** each TMU points to the start address of the memory pool.
** 
** 240   3/15/99 10:51p Dow
** Vile Hack
** 
** 239   3/14/99 1:42p Peter
** really invokve the ggggsoph
** 
** 238   3/13/99 9:48p Dow
** 
** 237   3/12/99 2:28p Dow
** Great-great-great grandson of packer workaround
** 
** 236   3/10/99 10:40a Peter
** bump-n-grind workaround for katmai until the bug is better
** characterized
** 
** 235   3/04/99 3:15p Atai
** mods for direct write
** 
** 234   3/02/99 2:21p Peter
** wax is a chip id
** 
** 233   2/19/99 5:54p Peter
** new splash screen
** 
** 232   2/18/99 7:11p Kcd
** Added P6FENCE for MacOS
** 
** 231   2/16/99 4:14p Peter
** made texture table declarations consistent
** 
** 230   2/10/99 2:24p Peter
** corrected alignment textures within an alignment allocation unit
** 
** 229   12/09/98 2:07p Peter
** More Norbert's stuff for the other 3DNow!(tm) partners
** 
** 228   12/03/98 11:27p Dow
** Code 'cleanup' heç
** 
** 227   12/02/98 2:53p Dow
** NT/9X Registry reading fix
** 
** 226   11/18/98 7:57p Dow
** Added grxclk
** 
** 225   11/12/98 2:50p Atai
** fixed register get
** 
** 224   11/02/98 5:34p Atai
** merge direct i/o code
** 
** 223   10/21/98 4:20p Atai
** gamma stuff
** 
** 222   10/21/98 10:41a Atai
** 
** 221   10/20/98 4:39p Atai
** update tramOffset and tramSize
** 
** 220   10/19/98 2:11p Peter
** ctrisetup happiness
** 
** 219   10/09/98 6:57p Peter
** dynamic 3DNow!(tm) mods
** 
** 218   9/04/98 11:36a Peter
** re-open fix for nt (thanks to taco/rob/nt bob)
** 
** 217   8/27/98 6:33p Atai
** added environment.tmuMemory
** 
** 216   7/29/98 3:09p Dow
** SDRAM Fixes
** 
** 215   7/24/98 2:03p Dow
** AGP Stuff
** 
** 214   7/23/98 3:19p Dow
** Bump & Grind Fix
** 
** 213   7/23/98 1:17a Dow
** Bump & Grind
** 
** 212   7/14/98 2:48p Mikec
** Added Glide environment variable FX_GLIDE_EMUL_RUSH allow Banshee glide
** to report itself as Rush to the application. Set it to 1 to enable Rush
** reporting. By default Glide still reports itself as Voodoo Graphics. 
** 
** 211   7/09/98 12:00p Jdt
** 
** 210   7/09/98 11:49a Jdt
** 
** 209   7/09/98 11:33a Jdt
** fixed fencing for dos build
** 
** 208   7/06/98 7:03p Jdt
** in-memory fifo test code, minor changes to init-time variables
** 
** 207   7/02/98 12:12p Dow
** LFB Fixes
** 
** 206   6/24/98 10:55a Peter
** gary's trilinear hell bug
** 
** 205   6/19/98 2:33p Mikec
** Fixed my triple buffering effage when getting lfb buffers we need to
** re-allocate the size of the render buffer
** 
** 204   6/16/98 6:12p Dow
** Rearranged texture memory
** 
** 203   6/12/98 9:21a Peter
** lfb read for triple buffering
** 
** 202   6/10/98 9:49a Peter
** lfb buffer addressing
** 
** 201   5/28/98 1:46p Dow
** Swap Pending Workaround
** 
** 200   5/21/98 4:47p Dow
** Direct Register Writes Work
** 
** 199   5/18/98 3:20p Peter
** pts more resistant to changing rounding modes
** 
** 198   5/13/98 11:42a Dow
** 
** 197   5/12/98 2:42p Dow
** 
** 196   5/12/98 1:36p Mc
** 
** 195   5/06/98 5:34p Dow
** 2D FastFill Workaround
** 
** 194   4/22/98 8:41a Dow
** AGP Workaround
** 
** 193   4/14/98 6:41p Peter
** Merge w/ cvg glide cleanup
** 
** 192   4/05/98 2:18p Dow
** DOS Glide Stuff
** 
** 191   4/03/98 2:04p Dow
** Dos Glide Mods
** 
** 190   3/28/98 12:31p Dow
** Added Fencing
** 
** 189   3/28/98 11:24a Dow
** itwoç
** 
** 184   2/17/98 12:50p Dow
** 
** 183   2/08/98 3:08p Dow
** FIFO Works
** 
** 182   2/03/98 8:45p Dow
** Prep for cmd fifo stuff
** 
** 181   2/02/98 4:31p Dow
** IO w/o HAL now possible
** 
** 180   2/01/98 7:52p Peter
** grLfbWriteRegion byte count problems
** 
** 179   1/29/98 9:54p Dow
** This is Banshee
** 
** 178   1/20/98 11:03a Peter
** env var to force triple buffering
 * 
 * 176   1/16/98 7:03p Peter
 * fixed volatile
 * 
 * 175   1/16/98 10:47a Peter
 * fixed idle effage
 * 
 * 174   1/15/98 1:12p Peter
 * dispatch w/o packing
 * 
 * 173   1/13/98 12:42p Atai
 * fixed grtexinfo, grVertexLayout, and draw triangle
 * 
 * 172   1/10/98 4:01p Atai
 * inititialize vertex layout, viewport, added defines
 * 
 * 168   1/07/98 11:18a Atai
 * remove GrMipMapInfo and GrGC.mm_table in glide3
 * 
 * 167   1/07/98 10:22a Peter
 * lod dithering env var
 * 
 * 166   1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 165   1/05/98 6:06p Atai
 * glide extension stuff
 * 
 * 164   12/18/97 10:52a Atai
 * fixed grGet(GR_VIDEO_POS)
 * 
 * 163   12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 162   12/17/97 4:05p Atai
 * added grChromaRange(), grGammaCorrecionRGB(), grRest(), and grGet()
 * functions
 * 
 * 160   12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 156   12/09/97 12:20p Peter
 * mac glide port
 * 
 * 155   12/09/97 10:28a Peter
 * cleaned up some frofanity
 * 
 * 154   12/09/97 9:46a Atai
 * added viewport varibales
 * 
 * 152   11/25/97 12:09p Peter
 * nested calls to grLfbLock vs init code locking on v2
 * 
 * 151   11/21/97 6:05p Atai
 * use one datalist (tsuDataList) in glide3
 * 
 * 150   11/21/97 3:20p Peter
 * direct writes tsu registers
 * 
 * 149   11/19/97 4:33p Atai
 * #define GLIDE3_VERTEX_LAYOUT 1
 * 
 * 148   11/19/97 3:51p Dow
 * Tex stuff for h3, def of GETENV when using fxHal
 * 
 * 147   11/18/97 6:11p Peter
 * fixed glide3 effage
 * 
 * 146   11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 145   11/18/97 3:25p Atai
 * redefine vData
 * 
 * 144   11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 143   11/15/97 7:43p Peter
 * more comdex silliness
 * 
 * 142   11/14/97 11:10p Peter
 * open vs hw init confusion
 * 
 * 141   11/14/97 5:02p Peter
 * more comdex stuff
 * 
 * 140   11/14/97 12:09a Peter
 * comdex thing and some other stuff
 * 
 * 139   11/12/97 2:35p Peter
 * fixed braino
 * 
 * 138   11/12/97 2:27p Peter
 * 
 * 137   11/12/97 11:38a Dow
 * 
 * 136   11/12/97 11:15a Peter
 * fixed tri/strip param send and used cvgdef.h constant
 * 
 * 135   11/12/97 9:21a Dow
 * Changed offset defs to those in h3defs.h
 * 
 * 134   11/07/97 11:22a Atai
 * remove GR_*_SMOOTH. use GR_SMOOTH
 * 
 * 133   11/06/97 3:46p Peter
 * dos ovl build problem
 * 
 * 132   11/06/97 3:38p Dow
 * More banshee stuff
 * 
 * 131   11/04/97 6:35p Atai
 * 1. sync with data structure changes
 * 2. break up aa triangle routine
 * 
 * 130   11/04/97 5:04p Peter
 * cataclysm part deux
 * 
 * 129   11/04/97 4:00p Dow
 * Banshee Mods
 * 
 * 128   11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 127   10/29/97 2:45p Peter
 * C version of Taco's packing code
 * 
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

/*
** -----------------------------------------------------------------------
** INCLUDE FILES
** -----------------------------------------------------------------------
*/
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <3dfx.h>
#include <glidesys.h>
#include <gdebug.h>

#include <h3.h>

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define WIN32_LEANER_AND_MEANER
#include <windows.h>
#include "fxsplash.h"
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */

/* Compilation hacks for h3 */

/* Reserved fbzMode bits */
#define SST_DRAWBUFFER_SHIFT    14
#define SST_DRAWBUFFER          (0x3 << SST_DRAWBUFFER_SHIFT)
#define SST_DRAWBUFFER_FRONT    (0 << SST_DRAWBUFFER_SHIFT)
#define SST_DRAWBUFFER_BACK     (1 << SST_DRAWBUFFER_SHIFT)      

/* Reserved textureMode bits */
#define SST_SEQ_8_DOWNLD        BIT(31)

/* hack for UMA via grHints */
#define GR_HINT_ENABLE_UMA    0x10000000

#if GLIDE_INIT_HAL

#include <fxhal.h>

#ifdef GETENV
#undef GETENV
#endif
#ifdef __linux__
/* We want to allow file based initialization */
#define GETENV hwcGetenv
#endif

/* dpc - 2 june 1997 
 * Moved the fence check out to avoid empty if body warning w/ gcc.
 * This only applies to systems that require the p6 fencing.
 */
#define P6FENCE_CHECK if (i & 2) P6FENCE


#else /* !defined(GLIDE_INIT_HAL) */

#include <minihwc.h>

/* HACK HACK HACK */
#define IDLE_HW(__hwPtr) (while (gc->ioRegs->status & (0x1f << 8)))

#define HWC_BASE_ADDR_MASK 0x03UL

#endif /* !GLIDE_INIT_HAL */

#if GLIDE_MULTIPLATFORM
#include "gcfuncs.h"
#endif

/* isolate this 'hack' here so as to make the code look cleaner */
#ifdef __WATCOMC__
#define GR_CDECL __cdecl
#else
#define GR_CDECL
#endif

#ifdef GLIDE3

#define GLIDE3_VERTEX_LAYOUT 1
/*
** grGet defines
*/
#define VOODOO2_FOG_TABLE_SIZE       80
#define VOODOO_FOG_TABLE_SIZE        64
#define VOODOO_GAMMA_TABLE_SIZE      32
#define SST1_BITS_DEPTH              16
#define SST1_ZDEPTHVALUE_NEAREST     0xFFFF
#define SST1_ZDEPTHVALUE_FARTHEST    0x0000
#define SST1_WDEPTHVALUE_NEAREST     0x0000
#define SST1_WDEPTHVALUE_FARTHEST    0xFFFF

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
  param,                        /* vertex, color, textureN */
    components,                 /* which ones? */
    type,                       /* data type */
    mode;                       /* enable / disable */
  FxI32
  offset;
} GrVParamInfo;

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
/*
** data structure for for oeminit dll 
*/
typedef struct {
  FxU32 version;
  FxU32 vendorID;
  FxU32 deviceID;
  FxU32 *linearAddress;
  GrScreenResolution_t res;
  GrScreenRefresh_t refresh;
  FxU32 reserved[4];
} OemInitInfo;
#endif

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

/*============================================================
 **  Video Stuff:
 **============================================================*/
#define VRETRACEMASK            0x00000fff
#define HRETRACEPOS             16

#else
/* Make sure GR_FLUSH_STATE is a noop if not Glide 3 */
#define GR_FLUSH_STATE()
#endif /* GLIDE3 */


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

struct _GrState_s  {
  GrCullMode_t                 /* these go in front for cache hits */
    cull_mode;                 /* cull neg, cull pos, don't cull   */
  
  GrHint_t
    paramHints;                /* Tells us if we need to pointcast a
                                  parameter to a specific chip */
  FxI32
    fifoFree;                  /* # free entries in FIFO */
  FxU32
    paramIndex,                /* Index into array containing
                                  parameter indeces to be sent ot the
                                  triangle setup code    */
    tmuMask;                   /* Tells the paramIndex updater which
                                  TMUs need values */
  struct {
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
  
  struct tmu_config_t {
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
  
#ifdef GLIDE3
  FxU16
#else
    FxU32
#endif
    lfb_constant_depth;        /* Constant value for depth buffer (LFBs) */
  GrAlpha_t
    lfb_constant_alpha;        /* Constant value for alpha buffer (LFBs) */
  
  FxU32
    num_buffers;               /* 2 or 3 */
  
  GrColorFormat_t
    color_format;              /* ARGB, RGBA, etc. */
  
  GrOriginLocation_t           /* lower left, upper left */
    origin;                     

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
  
#ifdef GLIDE3
  /* viewport and clip space coordinate related stuff */
  
  struct {
    float
      n, f;
    FxFloat
      ox, oy, oz;
    FxFloat
      hwidth, hheight, hdepth;
  } Viewport;
  
#endif
  
#ifdef GLIDE3
  /* Strip Stuff */
#if !GLIDE3_VERTEX_LAYOUT
  struct {
    GrVParamInfo
      vertexInfo,               /* Info about vertex data*/
      zInfo,
      colorInfo,                /* Info about color data */
      tex0Info,                 /* Info about tmu0 data */
      tex1Info;                 /* Info about tmu1 data */
    FxU32
      vSize,
      vStride;
  } vData;
#else /* GLIDE3_VERTEX_LAYOUT */
  struct {
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
  } vData;
#endif /* GLIDE3_VERTEX_LAYOUT */
  
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
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
    struct {
      GrColor_t range;
      GrChromaRangeMode_t mode;
    } grChromaRangeArgs;
#endif
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
#endif           /* GLIDE3 end grenable mode*/
};

#if GLIDE_DISPATCH_SETUP
/* gpci.c 
 *
 * Set of procs for the current cpu type. These are selected out of
 * the _archXXXX proc list that is selected at grGlideInit time.
 *
 * TriProc vector
 *  0 - No Culling
 *  1 - Culling (Postive/Negative)
 *
 * _GlideRoot.curTriProcs
 *  0 - Generic dataList w/ all parameters
 *  1 - Packed rgb (GLIDE_PACKED_RGB)
 *  2 - Packed argb (GLIDE_PACKED_RGB)
 */

#define NUM_TRI_PROC_LISTS 1

typedef FxI32 (FX_CALL* GrTriSetupProc)(const GrVertex*, const GrVertex*, const GrVertex*);
typedef GrTriSetupProc GrTriSetupProcVector[2];
typedef GrTriSetupProcVector GrTriSetupProcArchVector[NUM_TRI_PROC_LISTS];

/* Decalrations of the dispatchable procs found in xdraw2.asm and
 * xtexdl.c for teh triangle and texture download procs respectively.  
 */
extern FxI32 FX_CALL _trisetup_Default_Default(const GrVertex*, const GrVertex*, const GrVertex*);
extern FxI32 FX_CALL _trisetup_Default_cull(const GrVertex*, const GrVertex*, const GrVertex*);

#if GL_AMD3D
extern FxI32 FX_CALL _trisetup_3DNow_Default(const GrVertex*, const GrVertex*, const GrVertex*);
extern FxI32 FX_CALL _trisetup_3DNow_cull(const GrVertex*, const GrVertex*, const GrVertex*);
#endif /* GL_AMD3D */
#endif /* GLIDE_DISPATCH_SETUP */

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

typedef struct GrGC_s {
  FxU32
    totBuffers,
    strideInTiles,
    heightInTiles,
    bufferStride,
    bufSizeInTiles,
    bufSize,
    fbOffset,
    tramOffset[2],
    tramSize[2],
    *base_ptr,                  /* base address of SST */
    *reg_ptr,                   /* pointer to base of SST registers */
    *tex_ptr,                   /* texture memory address */
    *lfb_ptr,                   /* linear frame buffer address */
    *slave_ptr;                 /* Scanline Interleave Slave address */
  
#ifdef GLIDE_INIT_HWC
  hwcBoardInfo
    *bInfo;
#endif

#if GLIDE_MULTIPLATFORM
  GrGCFuncs
    gcFuncs;
#endif  
  
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
  void *oemInit;
#endif

  
#define kMaxVertexParam (20 + (12 * GLIDE_NUM_TMU) + 3)
  struct dataList_s {
    int      i;
    FxFloat* addr;
  } regDataList[kMaxVertexParam];
  int tsuDataList[kMaxVertexParam];

  struct {
#if GLIDE_DISPATCH_SETUP
    /* Current triangle rendering proc specialized for culling/no
     * culling and optionally for packed rgb at compile time.  
     */
    GrTriSetupProc     triSetupProc;
#endif /* GLIDE_DISPATCH_SETUP */
    
    /* Vector of texture download procs specialized by size
     * and processor vendor type.
     */
    GrTexDownloadProcVector* texDownloadProcs;
  } archDispatchProcs;
  
#ifdef GLIDE3
#if 0
  /* scale factor for clip space and viewport code */
  int scaleDataList[kMaxVertexParam];
#endif
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
    
    /* Basic command fifo characteristics. These should be
     * considered logically const after their initialization.
     */
    FxU32* fifoStart;    /* Virtual address of start of fifo */
    FxU32* fifoEnd;      /* Virtual address of fba fifo */
    FxU32  fifoOffset;   /* Offset from hw base to fifo start */
    FxU32  fifoSize;     /* Size in bytes of the fifo */
    FxU32  fifoJmpHdr[2];/* Type0 packet for jmp to fifo start
                            only first DWORD is used for memory
                            fifo--both are used for AGP FIFO
                          */
    
#ifdef TACO_MEMORY_FIFO_HACK
    FxU32  *vFifoStart;
    FxU32  *vFifoEnd;
#endif

    FxU32* fifoPtr;      /* Current write pointer into fifo */
    FxU32  fifoRead;     /* Last known hw read ptr. 
                          * This is the sli master, if enabled.
                          */
    
#if GLIDE_USE_DEBUG_FIFO
    FxU32* fifoShadowBase; /* Buffer that shadows the hw fifo for debugging */
    FxU32* fifoShadowPtr;
#endif /* GLIDE_USE_DEBUG_FIFO */
    
    /* Fifo checking information. In units of usuable bytes until
     * the appropriate condition.
     */
    FxI32  fifoRoom;     /* Space until next fifo check */
    FxI32  roomToReadPtr;/* Bytes until last known hw ptr */
    FxI32  roomToEnd;    /* # of bytes until last usable address before fifoEnd */

    FxBool lfbLockCount; /* Have we done an lfb lock? Count of the locks. */
    FxBool
      autoBump;                 /* Are we auto bumping (aka hole counting?) */
    FxU32
      *lastBump,                /* Last ptr where we bumped. */
      *bumpPos;                 /* Nex place to bump */
    FxU32
      bumpSize;                 /* # of DWORDS per bump */

  } cmdTransportInfo;
  

  SstIORegs
    *ioRegs;                    /* I/O remap regs */
  SstCRegs
    *cRegs;                     /* AGP/Cmd xfer/misc regs */
  SstGRegs
    *gRegs;                     /* 2D regs */
  SstRegs
    *sstRegs;                   /* Graphics Regs (3D Regs) */
  FxU32
    *rawLfb,                    /* Poiinter to vAddr of baseAddress1 */
    nBuffers,
    curBuffer,
    frontBuffer,
    backBuffer,
    buffers[4],
    lfbBuffers[4];              /* Tile relative addresses of the color/aux
                                 * buffers for lfbReads.
                                 */
  
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
    num_tmu,                    /* number of TMUs attached */
    grColBuf,
    grAuxBuf,
    grHwnd;

  /* Overlay Hack: This flag indicates if the overlay hack is on.  
   * This is probably the wrong place for this kind of thing*/
  int desktopOverlay;

  FxBool
    scanline_interleaved;

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

  FxU32  nColBuffers;
  FxBool open;                  /* Has GC Been Opened? */
  FxBool hwInitP;               /* Has the hw associated w/ GC been initted and
                                   mapped?  This is managed in
                                   _grDetectResources:gpci.c the first time
                                   that the board is detected, and in
                                   grSstWinOpen:gsst.c if the hw has been
                                   shutdown in a call to grSstWinClose.
                                   */

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
  /* NOTE!!!! p6Fencer this must be the very first structure element always */
  int p6Fencer;                 /* xchg to here to keep this in cache!!! */
  int current_sst;
  FxU32 CPUType;
  GrGC *curGC;                  /* point to the current GC      */
  FxU32 packerFixAddress;       /* address to write packer fix to */
  FxBool    windowsInit;        /* Is the Windows part of glide initialized? */

  FxI32 curTriSize;             /* the size in bytes of the current triangle */
#if GLIDE_HW_TRI_SETUP
  FxI32 curVertexSize;          /* Size in bytes of a single vertex's parameters */
#endif

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
    float  ftemp1, ftemp2;       /* temps to convert floats to ints */

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
    FxBool enUma;               /* enable uma                   */
    float  gammaR, gammaG, gammaB; /* Gamma settings */

    /* Force alternate buffer strategy */
    FxI32  nColorBuffer;
    FxI32  nAuxBuffer;
    FxI32  emulRush;            /* Banshee reports as Rush flag */
    FxI32  grxClk;
    FxBool autoBump;            /* Auto bump or do it manually? */
    FxU32  bumpSize;
  } environment;

  struct {
    FxU32       bufferSwaps;    /* number of buffer swaps       */
    FxU32       pointsDrawn;
    FxU32       linesDrawn;
    FxU32       trisProcessed;
    FxU32       trisDrawn;

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

  GrHwConfiguration     hwConfig;
  
  GrGC                  GCs[MAX_NUM_SST];       /* one GC per board     */

#if GLIDE_DISPATCH_SETUP
#define TRISETUP_NORGB (*_GlideRoot.curTriProcs + 0)
#if GLIDE_PACKED_RGB
#define TRISETUP_RGB  (*_GlideRoot.curTriProcs + 1)
#define TRISETUP_ARGB (*_GlideRoot.curTriProcs + 2)
#else /* !GLIDE_PACKED_RGB */
#define TRISETUP_RGB  TRISETUP_NORGB
#define TRISETUP_ARGB TRISETUP_NORGB
#endif /* !GLIDE_PACKED_RGB */
#define PROC_SELECT_TRISETUP(__procVector, __cullMode) (__procVector)[(__cullMode) != GR_CULL_DISABLE]

  GrTriSetupProcArchVector* curTriProcs;
#endif /* GLIDE_DISPATCH_SETUP */

  GrTexDownloadProcVector*  curTexProcs;
#define PROC_SELECT_TEXDOWNLOAD()                      _GlideRoot.curTexProcs
  FxBool OSWin95;
};

extern struct _GlideRoot_s GR_CDECL _GlideRoot;
#if GLIDE_MULTIPLATFORM
extern GrGCFuncs _curGCFuncs;
#endif

#if defined(__WATCOMC__)
/*
 *  P6 Fence
 * 
 *  Here's the stuff to do P6 Fencing.  This is required for the
 *  certain things on the P6
 *
 * dpc - 21 may 1997 - FixMe!
 * This was yoinked from sst1/include/sst1init.h, and should be
 * merged back into something if we decide that we need it later.
 */
extern FxU32 p6FenceVar;

void 
p6Fence(void);
#pragma aux p6Fence = \
"xchg eax, p6FenceVar" \
modify [eax];


#define P6FENCE p6Fence()
#elif defined(__MSC__)
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
#define P6FENCE __eieio()
#elif defined(__GNUC__) && defined(__i386__)
#define P6FENCE asm("xchg %%eax, %0" : : "m" (_GlideRoot.p6Fencer) : "eax");
#else
#error "P6 Fencing in-line assembler code needs to be added for this compiler"
#endif /* Compiler specific fence commands */

/*==========================================================================*/
/* Macros for declaring functions */
#define GR_DDFUNC(name, type, args) \
   type FX_CSTYLE name args

#define GR_ENTRY(name, type, args) \
   FX_EXPORT type FX_CSTYLE name args

#define GR_DIENTRY(name, type, args) \
   FX_EXPORT type FX_CSTYLE name args

#ifdef GLIDE3
#define GR_STATE_ENTRY(name, type, args) \
   type _##name## args
#else
#define GR_STATE_ENTRY(name, type, args) \
   GR_ENTRY(name, type, args)
#endif

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

#if   (GLIDE_PLATFORM & GLIDE_HW_SST1)
#define GLIDE_DRIVER_NAME "Voodoo Graphics"
#elif (GLIDE_PLATFORM & GLIDE_HW_SST96)
#define GLIDE_DRIVER_NAME "Voodoo Rush"
#elif (GLIDE_PLATFORM & GLIDE_HW_CVG)
#define GLIDE_DRIVER_NAME "Voodoo^2"
#elif (GLIDE_PLATFORM & GLIDE_HW_H3)
#define GLIDE_DRIVER_NAME "Banshee"
#else 
#define GLIDE_DRIVER_NAME "HOOPTI???"
#endif

/*==========================================================================*/
#ifndef FX_GLIDE_NO_FUNC_PROTO

void _grMipMapInit(void);

#if GLIDE_DISPATCH_SETUP
#define TRISETUP (*gc->archDispatchProcs.triSetupProc)
#else /* !GLIDE_DISPATCH_SETUP */
FxI32 FX_CSTYLE
_trisetup_asm(const GrVertex *va, const GrVertex *vb, const GrVertex *vc);
FxI32 FX_CSTYLE
_trisetup(const GrVertex *va, const GrVertex *vb, const GrVertex *vc);
FxI32 FX_CSTYLE
_trisetup_nogradients(const GrVertex *va, const GrVertex *vb, const GrVertex *vc);

/* GMT: BUG need to make this dynamically switchable
   That is not a bug.  It is an opinion!
 */
#if GLIDE_USE_C_TRISETUP
#  if (GLIDE_PLATFORM & GLIDE_HW_CVG) && USE_PACKET_FIFO
#    define TRISETUP _trisetup_nogradients
#  else /* !((GLIDE_PLATFORM & GLIDE_HW_CVG) && USE_PACKET_FIFO) */
#    define TRISETUP _trisetup_nogradients
#  endif /* !((GLIDE_PLATFORM & GLIDE_HW_CVG) && USE_PACKET_FIFO) */
#else /* !GLIDE_USE_C_TRISETUP */
#  define TRISETUP _trisetup_asm
#endif /* !GLIDE_USE_C_TRISETUP */
#endif /* !GLIDE_DISPATCH_SETUP */

#ifdef GLIDE3
void
_grValidateState();

void FX_CSTYLE
_grDrawVertexList(FxU32 listType, FxI32 mode, FxI32 count, void *pointers);

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

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
void 
_grChromaModeExt(GrChromakeyMode_t mode);
void
_grChromaRangeExt( GrColor_t min, GrColor_t max , GrChromaRangeMode_t mode);
void 
_grTexChromaModeExt(GrChipID_t tmu, GrChromakeyMode_t mode);
void 
_grTexChromaRangeExt(GrChipID_t tmu, GrColor_t min, GrColor_t max);
#else
void
_grChromakeyValue( GrColor_t value );
#endif

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
_grDepthBiasLevel( FxI16 level );

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

#endif

#ifdef GLIDE3      /* glide 3 m point, m aa point, m line, m aa line routine */

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
_grAAVpDrawTriangles(FxI32 mode, FxI32 ttype, FxI32 count, void *pointers);

void FX_CSTYLE
_grAADrawVertexList(FxU32 type, FxI32 mode, FxI32 count, void *pointers);

#endif

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
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

#endif

#endif /* FX_GLIDE_NO_FUNC_PROTO */

/*==========================================================================*/
/* 
**  Function Prototypes
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

int
_guHeapCheck(void);

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
                       GuTexPalette *pal,
                       int start, int end);

FxU32
_grTexCalcBaseAddress(FxU32 start_address, GrLOD_t largeLod,
                      GrAspectRatio_t aspect, GrTextureFormat_t fmt,
                      FxU32 odd_even_mask);

void
_grTexForceLod(GrChipID_t tmu, int value);

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
_grLfbWriteRegion(FxBool pixPipelineP,
                  GrBuffer_t dst_buffer, FxU32 dst_x, FxU32 dst_y, 
                  GrLfbSrcFmt_t src_format, 
                  FxU32 src_width, FxU32 src_height, 
                  FxI32 src_stride, 
                  const void *src_data);

/* gglide.c - Flushes the current state in gc->state.fbi_config to the hw.
 */
extern void
_grFlushCommonStateRegs(void);

#if USE_PACKET_FIFO
/* cvg.c */
extern void
_grSet32(volatile FxU32* const sstAddr, const FxU32 val);

extern FxU32
_grGet32(volatile FxU32* const sstAddr);
#endif /* USE_PACKET_FIFO */

/*==========================================================================*/
/*  GMT: have to figure out when to include this and when not to
*/
#if defined(GLIDE_DEBUG) || defined(GLIDE_ASSERT) || defined(GLIDE_SANITY_ASSERT) || defined(GLIDE_SANITY_SIZE)
  #define DEBUG_MODE 1
  #include <assert.h>
#endif

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) || (GLIDE_PLATFORM & GLIDE_HW_H3)

#if ASSERT_FAULT
#define ASSERT_FAULT_IMMED(__x) if (!(__x)) { \
                                 *(FxU32*)NULL = 0; \
                                 _grAssert(#__x, __FILE__, __LINE__); \
                              }
#else
#define ASSERT_FAULT_IMMED(__x) GR_ASSERT(__x)
#endif

#if !USE_PACKET_FIFO
/* NOTE: fifoFree is the number of entries, each is 8 bytes */
#define GR_CHECK_FOR_ROOM(n,p) \
{ \
  FxI32 fifoFree = gc->state.fifoFree - (n); \
  if (fifoFree < 0)          \
    fifoFree = _grSpinFifo(n); \
  gc->state.fifoFree = fifoFree;\
}
#elif USE_PACKET_FIFO
/* Stuff to manage the command fifo on cvg
 *
 * NB: All of the addresses are in 'virtual' address space, and the
 * sizes are in bytes.
 */

/* The Voodoo^2 fifo is 4 byte aligned */
#define FIFO_ALIGN_MASK      0x03

/* We claim space at the end of the fifo for:
 *   1 nop (2 32-bit words)
 *   1 jmp (1 32-bit word)
 *   1 pad word
 */
#define FIFO_END_ADJUST  (sizeof(FxU32) << 2)

/* NB: This should be used sparingly because it does a 'real' hw read
 * which is *SLOW*.
 *
 * NB: This address is always in sli master relative coordinates.
 */
FxU32 _grHwFifoPtr(FxBool);
#define HW_FIFO_PTR(a) _grHwFifoPtr(a)

#if FIFO_ASSERT_FULL
extern const FxU32 kFifoCheckMask;
extern FxU32 gFifoCheckCount;

#define FIFO_ASSERT() \
if ((gFifoCheckCount++ & kFifoCheckMask) == 0) { \
   const FxU32 cmdFifoDepth = GR_GET(((SstRegs*)(gc->reg_ptr))->cmdFifoDepth); \
   const FxU32 maxFifoDepth = ((gc->cmdTransportInfo.fifoSize - FIFO_END_ADJUST) >> 2); \
   if(cmdFifoDepth > maxFifoDepth) { \
     gdbg_printf("cmdFifoDepth > size: 0x%X : 0x%X\n", \
                 cmdFifoDepth, maxFifoDepth); \
     ASSERT_FAULT_IMMED(cmdFifoDepth <= maxFifoDepth); \
   } else if (cmdFifoDepth + (gc->cmdTransportInfo.fifoRoom >> 2) > maxFifoDepth) { \
     gdbg_printf("cmdFifoDepth + fifoRoom > size: (0x%X : 0x%X) : 0x%X\n", \
                 cmdFifoDepth, (gc->cmdTransportInfo.fifoRoom >> 2), maxFifoDepth); \
     ASSERT_FAULT_IMMED(cmdFifoDepth + (gc->cmdTransportInfo.fifoRoom >> 2) <= maxFifoDepth); \
   } \
} \
ASSERT_FAULT_IMMED(HW_FIFO_PTR(FXTRUE) >= (FxU32)gc->cmdTransportInfo.fifoStart); \
ASSERT_FAULT_IMMED(HW_FIFO_PTR(FXTRUE) < (FxU32)gc->cmdTransportInfo.fifoEnd); \
ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoRoom < gc->cmdTransportInfo.fifoSize); \
ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoPtr < (FxU32)gc->cmdTransportInfo.fifoEnd)
#else /* !FIFO_ASSERT_FULL */
#define FIFO_ASSERT() \
ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoRoom < gc->cmdTransportInfo.fifoSize); \
ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoPtr < (FxU32)gc->cmdTransportInfo.fifoEnd)
#endif /* !FIFO_ASSERT_FULL */

void GR_CDECL
_FifoMakeRoom(const FxI32 blockSize, const char* fName, const int fLine);

#if defined( TACO_MEMORY_FIFO_HACK )
extern void _FifoFlush( void );
#endif

#ifndef GLIDE_DEBUG
#define GR_BUMP_N_GRIND \
do { \
  P6FENCE; \
  GR_CAGP_SET(bump, gc->cmdTransportInfo.fifoPtr - gc->cmdTransportInfo.lastBump); \
  gc->cmdTransportInfo.lastBump = gc->cmdTransportInfo.fifoPtr; \
  gc->cmdTransportInfo.bumpPos = gc->cmdTransportInfo.fifoPtr + (gc->cmdTransportInfo.bumpSize); \
  if (gc->cmdTransportInfo.bumpPos > gc->cmdTransportInfo.fifoEnd) \
    gc->cmdTransportInfo.bumpPos = gc->cmdTransportInfo.fifoEnd; \
} while(0)
#else
/* fifo.c */
void _grBumpNGrind(void);
#define GR_BUMP_N_GRIND _grBumpNGrind()
#endif

#define CHECK_FOR_BUMP(__writeSize) \
if (!gc->cmdTransportInfo.autoBump && \
    ((gc->cmdTransportInfo.fifoPtr + __writeSize) > gc->cmdTransportInfo.bumpPos)) \
  GR_BUMP_N_GRIND;

#define GR_CHECK_FOR_ROOM(__n, __p) \
do { \
  const FxU32 writeSize = (__n) + ((__p) * sizeof(FxU32));            /* Adjust for size of hdrs */ \
  ASSERT(((FxU32)(gc->cmdTransportInfo.fifoPtr) & FIFO_ALIGN_MASK) == 0); /* alignment */ \
  ASSERT(writeSize < gc->cmdTransportInfo.fifoSize - sizeof(FxU32)); \
  /* CHECK_FOR_BUMP(writeSize); */ /* now done in _FifoMakeRoom */ \
  FIFO_ASSERT(); \
  if (gc->cmdTransportInfo.fifoRoom < (FxI32)writeSize) { \
     GDBG_INFO(280, "Fifo Addr Check: (0x%X : 0x%X)\n", \
               gc->cmdTransportInfo.fifoRoom, writeSize); \
     _FifoMakeRoom(writeSize, __FILE__, __LINE__); \
  } \
  ASSERT((FxU32)gc->cmdTransportInfo.fifoRoom >= writeSize); \
  FIFO_ASSERT(); \
} while(0)
#else
#error "GR_CHECK_FOR_ROOM not defined"
#endif

#elif (GLIDE_PLATFORM & GLIDE_HW_H3)

#define GR_CHECK_FOR_ROOM(__n, __p) 

#endif /* GLIDE_PLATFORM & GLIDE_HW_?? */

#ifdef GLIDE_SANITY_SIZE
#if USE_PACKET_FIFO
#define GR_CHECK_FIFO_PTR() \
if (gc->cmdTransportInfo.autoBump) {\
  if((FxU32)gc->cmdTransportInfo.fifoPtr != gc->checkPtr + gc->checkCounter)\
    GDBG_ERROR("GR_ASSERT_FIFO", "(%s : %d) : " \
               "fifoPtr should be 0x%X (0x%X : 0x%X) but is 0x%X\n", \
               __FILE__, __LINE__, gc->checkPtr + gc->checkCounter, \
               gc->checkPtr, gc->checkCounter, gc->cmdTransportInfo.fifoPtr);\
  ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoPtr == gc->checkPtr + gc->checkCounter);\
}

#define GR_SET_FIFO_PTR(__n, __p) \
  gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr; \
  gc->checkCounter = ((__n) + ((__p) << 2))
#else
#define GR_CHECK_FIFO_PTR() 
#define GR_SET_FIFO_PTR(__n, __p)
#endif

#  define GR_CHECK_SIZE() \
                if(gc->counter != gc->expected_counter) \
                  GDBG_ERROR("GR_ASSERT_SIZE","byte counter should be %d but is %d\n", \
                              gc->expected_counter,gc->counter); \
                GR_CHECK_FIFO_PTR(); \
                gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr; \
                gc->checkCounter = 0; \
                ASSERT(gc->counter == gc->expected_counter); \
                gc->counter = gc->expected_counter = 0

#  define GR_CHECK_SIZE_DIRECT() \
                if(gc->counter != gc->expected_counter) \
                  GDBG_ERROR("GR_ASSERT_SIZE","byte counter should be %d but is %d\n", \
                              gc->expected_counter,gc->counter); \
                gc->checkCounter = 0; \
                ASSERT(gc->counter == gc->expected_counter); \
                gc->counter = gc->expected_counter = 0



#  define GR_SET_EXPECTED_SIZE(n,p) \
                ASSERT(gc->counter == 0); \
                ASSERT(gc->expected_counter == 0); \
                GR_CHECK_FOR_ROOM(n,p); \
                gc->expected_counter = n; \
                GR_SET_FIFO_PTR(n, p)

#  define GR_INC_SIZE(n) gc->counter += n
#else
  /* define to do nothing */
#  define GR_CHECK_SIZE()
#  define GR_CHECK_SIZE_DIRECT()
#  define GR_SET_EXPECTED_SIZE(n,p) GR_CHECK_FOR_ROOM(n,p)
#  define GR_INC_SIZE(n)
#endif

#define GR_DCL_GC GrGC *gc = _GlideRoot.curGC
#if GLIDE_INIT_HAL
#define GR_DCL_HW SstRegs *hw = (SstRegs *)gc->sstRegs
#else
#define GR_DCL_HW SstRegs *hw = (SstRegs *)gc->sstRegs
#endif
#ifdef DEBUG_MODE
#define ASSERT(exp) GR_ASSERT(exp)

#define GR_BEGIN_NOFIFOCHECK(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                const FxI32 saveLevel = gc->myLevel; \
                static char myName[] = name;  \
                GR_ASSERT(gc != NULL);  \
                GR_ASSERT(hw != NULL);  \
                gc->myLevel = level; \
                gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr; \
                GDBG_INFO(gc->myLevel,myName); \
                FXUNUSED(saveLevel); \
                FXUNUSED(hw)
#define GR_TRACE_EXIT(__n) \
                gc->myLevel = saveLevel; \
                GDBG_INFO(281, "%s --done---------------------------------------\n", __n)
#define GR_TRACE_RETURN(__l, __n, __v) \
                gc->myLevel = saveLevel; \
                GDBG_INFO((__l), "%s() => 0x%x---------------------\n", (__n), (__v), (__v))
#else /* !DEBUG_MODE */
#define ASSERT(exp)
#define GR_BEGIN_NOFIFOCHECK(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                FXUNUSED(hw)
#define GR_TRACE_EXIT(__n)
#define GR_TRACE_RETURN(__l, __n, __v) 
#endif /* !DEBUG_MODE */

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
#define GR_ASSERT(exp) ((void)(0 && ((FxU32)(exp))))
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

#if USE_PACKET_FIFO
#ifdef GDBG_INFO_ON
void _grFifoWriteDebug(FxU32 addr, FxU32 val, FxU32 fifoPtr);
#define DEBUGFIFOWRITE(a,b,c) \
_grFifoWriteDebug((FxU32) a, (FxU32) b, (FxU32) c)
void _grFifoFWriteDebug(FxU32 addr, float val, FxU32 fifoPtr);
#define DEBUGFIFOFWRITE(a,b,c) \
_grFifoFWriteDebug((FxU32) a, (float) b, (FxU32) c)
#else /* ~GDBG_INFO_ON */
#define DEBUGFIFOWRITE(a,b,c)
#define DEBUGFIFOFWRITE(a,b,c)
#endif /* !GDBG_INFO_ON */
#endif /* USE_PACKET_FIFO */

/* HW Setting macros. We redefine the default macros to:
 *  - add extra tracing
 *  - work around hw bugs
 *  - do platform specific whacky things.
 */
#  if SET_BSWAP
#    undef GET
#    undef GET16
#    undef SET
#    undef SET16
#    undef SETF

#    if __POWERPC__ && defined(__MWERKS__)
#      define GET(s)               __lwbrx( (void*)&(s), 0 )
#      define GET16(s)             __lwbrx( (void*)&(s), 0 )
#      define SET(d, s)            __stwbrx((s), (void*)&(d), 0)
#      define SET16(d, s)          __sthbrx((s), (void*)&(d), 0 )
#      define SETF(d, s)           { \
                                     const float temp = (s); \
                                     __stwbrx( *((FxU32*)&temp), (void*)&(d), 0 ); \
                                   }
#      define SET_LINEAR(d, s)     SET((d), (s))
#      define SET_LINEAR_16(d, s)  SET((d), ((((FxU32)(s)) >> 16UL) | \
                                           (((FxU32)(s)) << 16UL)))
#      define SET_LINEAR_8(d, s)   ((d) = (s))
#    else /* !defined(__MWERKS__) && POWERPC */
#      error "Define byte swapped macros for GET/SET"
#    endif /* !defined(__MWERKS__) && POWERPC */
#  else /* !SET_BSWAP */
#    undef GET
#    undef GET16

#    define GET(s)   s
#    define GET16(s) s
#  endif /* !SET_BSWAP */


#if GLIDE_USE_DEBUG_FIFO
#define kDebugFifoSize 0x1000UL
#endif /* GLIDE_USE_DEBUG_FIFO */

#ifndef SET_LINEAR
#define SET_LINEAR(__addr, __val)        SET(__addr, __val)
#define SET_LINEAR_16(__addr, __val) SET(__addr, __val)
#define SET_LINEAR_8(__addr, __val)  SET(__addr, __val)
#endif /* !defined(SET_LINEAR) */

/* Extract the fp exponent from a floating point value.
 * NB: The value passed to this macro must be convertable
 * into an l-value.
 */
#define kFPExpMask        0x7F800000UL
#define kFPZeroMask       0x80000000UL
#define kFPExpShift       0x17UL
#define FP_FLOAT_EXP(__fpVal)   ((FxU32)(((*(const FxU32*)(&(__fpVal))) & kFPExpMask) >> kFPExpShift))
#define FP_FLOAT_ZERO(__fpVal)  (((*(const FxU32*)(&(__fpVal))) & ~kFPZeroMask) == 0x00)

/* The two most commonly defined macros in the known universe */
#define MIN(__x, __y) (((__x) < (__y)) ? (__x) : (__y))
#define MAX(__x, __y) (((__x) < (__y)) ? (__y) : (__x))

/* Simple macro to make selecting a value against a boolean flag
 * simpler w/o a conditional. 
 *
 * NB: This requires that the boolean value being passed in be the
 * result of one of the standard relational operators. 
 */
#define MaskSelect(__b, __val) (~(((FxU32)(__b)) - 1UL) & (__val))

/* Chipfield ids that glide uses. */
#define kChipFieldShift (8UL + 3UL)
typedef enum {
  eChipBroadcast    = 0x00UL,
  eChipFBI          = 0x01UL,
  eChipTMU0         = 0x02UL,
  eChipTMU1         = 0x04UL,
  eChipTMU2         = 0x08UL,
  eChipAltBroadcast = 0x0FUL,
} FifoChipField;

#define BROADCAST_ID eChipBroadcast

#define WAX_ID       FX_BIT(14)

/* Although these are named reg_group_xxx they are generic options for
 * grouping register writes and should be fine w/ and w/o the fifo
 * being enabled.  
 */
#if GDBG_INFO_ON
#define REG_GROUP_DCL(__regMask, __regBase, __groupNum, __checkP) \
const FxBool _checkP = (__checkP); \
const FxU32 _groupNum = (__groupNum);\
const FxU32 _regMask = (__regMask); \
FxU32 _regCheckMask = (__regMask); \
FxU32 _regBase = offsetof(SstRegs, __regBase)

#define REG_GROUP_DCL_WAX(__regMask, __regBase, __groupNum, __checkP) \
const FxBool _checkP = (__checkP); \
const FxU32 _groupNum = (__groupNum);\
const FxU32 _regMask = (__regMask); \
FxU32 _regCheckMask = (__regMask); \
FxU32 _regBase = offsetof(SstGRegs, __regBase)


#define REG_GROUP_ASSERT(__regAddr, __val, __floatP) \
{ \
  const FxU32 curRegAddr = offsetof(SstRegs, __regAddr); \
  const FxU32 curRegIndex = (curRegAddr - _regBase) >> 2; \
  const FxU32 curRegBit = (0x01UL << curRegIndex); \
  const float floatVal = (const float)(__val); \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%X : 0x%X) : 0x%X\n", \
            curRegIndex, curRegAddr, *(const FxU32*)&floatVal); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         (gc->cmdTransportInfo.lfbLockCount != 0), \
                         "Called within grLfbLock/grLfbUnlockPair"); \
  GR_ASSERT((_regMask & curRegBit) == curRegBit);                            /* reg allocated in mask */ \
  if (curRegIndex > 0) \
  GR_ASSERT(((0xFFFFFFFFUL >> (32 - curRegIndex)) & _regCheckMask) == 0x00); /* All previous regs done */ \
  _regCheckMask ^= curRegBit;                                                /* Mark current reg */ \
}

#define REG_GROUP_ASSERT_WAX(__regAddr, __val, __floatP) \
{ \
  const FxU32 curRegAddr = offsetof(SstGRegs, __regAddr); \
  const FxU32 curRegIndex = (curRegAddr - _regBase) >> 2; \
  const FxU32 curRegBit = (0x01UL << curRegIndex); \
  const float floatVal = (const float)(__val); \
  GDBG_INFO(220, "\t(0x%X : 0x%X) : 0x%X\n", \
            curRegIndex, curRegAddr, *(const FxU32*)&floatVal); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         (gc->cmdTransportInfo.lfbLockCount != 0), \
                         "Called within grLfbLock/grLfbUnlockPair"); \
  GR_ASSERT((_regMask & curRegBit) == curRegBit);                            /* reg allocated in mask */ \
  if (curRegIndex > 0) \
  GR_ASSERT(((0xFFFFFFFFUL >> (32 - curRegIndex)) & _regCheckMask) == 0x00); /* All previous regs done */ \
  _regCheckMask ^= curRegBit;                                                /* Mark current reg */ \
}

#else /* !GDBG_INFO_ON */
#define REG_GROUP_DCL(__regMask, __regBase, __groupNum, __checkP) 
#define REG_GROUP_DCL_WAX(__regMask, __regBase, __groupNum, __checkP) 
#define REG_GROUP_ASSERT(__regAddr, __val, __floatP)
#define REG_GROUP_ASSERT_WAX(__regAddr, __val, __floatP)
#endif /* !GDBG_INFO_ON */

#if GLIDE_HW_TRI_SETUP
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
#endif /* GLIDE_HW_TRI_SETUP */

#define REGNUM(__reg)   (offsetof(SstRegs, __reg) >> 2)
#define REGNUM_WAX(_reg) (offsetof(SstGRegs, __reg) >> 2)

#define PACKET_HEADER_ADD(__start, __reg, __header) \
do {\
  GDBG_PRINTF("%x, %x\n", REGNUM(__reg), REGNUM(__start));\
  GR_ASSERT((REGNUM(__reg) - REGNUM(__start)) <= 14);\
  __header |= ((1 << (REGNUM(__reg) - REGNUM(__start))) << 15);\
} while (0)


#if USE_PACKET_FIFO

#define REGNUM(__reg)  (offsetof(SstRegs, __reg) >> 2)
#define REGNUM_WAX(_reg) (offsetof(SstGRegs, __reg) >> 2)

/* The shift below is a bit tricky.  Watch out! */
#define FIFO_REG(__chipField, __field) \
   (GR_ASSERT(((FxU32)(__chipField)) < 0x10UL), \
    ((((FxU32)offsetof(SstRegs, __field)) << 1) | (((FxU32)(__chipField)) << kChipFieldShift)))    

/* And here's the WAX version */
/* The shift below is a bit tricky.  Watch out! */
#define FIFO_REG_WAX(__field) ((((FxU32)offsetof(SstGRegs, __field)) << 1) | FXBIT(14))
  
/* The REG_GROUP_XXX macros do writes to a monotonically increasing
 * set of registers. There are three flavors of the macros w/
 * different restrictions etc.
 *
 * NB: Care must be taken to order the REG_GROUP_SET macro uses to
 * match the actual register order, otherwise all hell breaks loose.  
 */

/* Write to __groupNum registers (max 14) starting at __regBase under
 * the control of __groupMask (lsb->msb).
 */
#define REG_GROUP_BEGIN(__chipId, __regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL(__chipId, __regBase, __groupNum, \
                         __groupMask, (((__groupMask) << SSTCP_PKT4_MASK_SHIFT) | \
                                       FIFO_REG(__chipId, __regBase) | \
                                       SSTCP_PKT4), \
                         FXTRUE)

#define REG_GROUP_BEGIN_WAX(__regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL_WAX(__regBase, __groupNum, __groupMask, \
(((__groupMask) << SSTCP_PKT4_MASK_SHIFT) | FIFO_REG_WAX(__regBase) | \
SSTCP_PKT4), FXTRUE)
  
/* Same as the non-NO_CHECK variant, but GR_SET_EXPECTED_SIZE must
 * have already been called to allocate space for this write.  
 */
#define REG_GROUP_NO_CHECK_BEGIN(__chipId, __regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_ASSERT(gc->expected_counter >= (FxI32)((__groupNum) * sizeof(FxU32))); \
REG_GROUP_BEGIN_INTERNAL(__chipId, __regBase, __groupNum, \
                         __groupMask, \
                         (((__groupMask) << SSTCP_PKT4_MASK_SHIFT) | \
                          FIFO_REG(__chipId, __regBase) | \
                          SSTCP_PKT4), \
                         FXFALSE)

/* Register writes (<= 32) sequentially starting at __regBase */
#define REG_GROUP_LONG_BEGIN(__chipId, __regBase, __groupNum) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 32)); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL(__chipId, __regBase, __groupNum, \
                         (0xFFFFFFFF >> (32 - (__groupNum))), \
                         (((__groupNum) << SSTCP_PKT1_NWORDS_SHIFT) | \
                          FIFO_REG(__chipId, __regBase) | \
                          SSTCP_INC | \
                          SSTCP_PKT1), \
                         FXTRUE)

#define GETREG(reg) (reg)

#define REG_GROUP_BEGIN_INTERNAL(__chipId, __regBase, __groupNum, __groupMask, __pktHdr, __checkP) \
{ \
  GR_DCL_GC; \
  volatile FxU32* _regGroupFifoPtr = gc->cmdTransportInfo.fifoPtr; \
  REG_GROUP_DCL(__groupMask, __regBase, __groupNum, __checkP); \
  GR_ASSERT(((__pktHdr) & 0xE0000000UL) == 0x00UL); \
  FIFO_ASSERT(); \
  GR_ASSERT(GETREG(gc->cRegs->cmdFifo0.readPtrL) >= gc->cmdTransportInfo.fifoOffset);\
  GR_ASSERT(GETREG(gc->cRegs->cmdFifo0.readPtrL) < (gc->cmdTransportInfo.fifoOffset + gc->cmdTransportInfo.fifoSize));\
  GDBG_INFO(120, "REG_GROUP_BEGIN:\n");\
  GDBG_INFO(120, "\tFile: %s Line %d\n", __FILE__, __LINE__);\
  GDBG_INFO(120, "\t_regGroupFifoPtr:  0x%x\n", (FxU32)_regGroupFifoPtr - (FxU32)gc->rawLfb);\
  GDBG_INFO(120, "\t__pktHdr:           0x%x\n", __pktHdr);\
  GDBG_INFO(120, "\t__groupNum:         0x%x\n", __groupNum);\
  GDBG_INFO(120, "\t__groupMask:        0x%x\n", (__groupMask));\
  GDBG_INFO(120, "\t__chipId:           0x%x\n", __chipId);\
  GDBG_INFO(120, "\t__regBase:          0x%x\n", offsetof(SstRegs, __regBase));\
  GDBG_INFO(120, "\tfifoPtr:            0x%x\n", (FxU32)gc->cmdTransportInfo.fifoPtr - (FxU32)gc->cmdTransportInfo.fifoStart - (FxU32)gc->rawLfb); \
  GDBG_INFO(120, "\tfifoRoom:           0x%x\n", gc->cmdTransportInfo.fifoRoom);\
  GDBG_INFO(120, "\treadPtrL:           0x%x\n", GETREG(gc->cRegs->cmdFifo0.readPtrL));\
  SET(*_regGroupFifoPtr++, (__pktHdr))

#define REG_GROUP_BEGIN_INTERNAL_WAX(__regBase, __groupNum, __groupMask, __pktHdr, __checkP) \
{ \
  GR_DCL_GC; \
  volatile FxU32* _regGroupFifoPtr = gc->cmdTransportInfo.fifoPtr; \
  REG_GROUP_DCL_WAX(__groupMask, __regBase, __groupNum, __checkP); \
  GR_ASSERT(((__pktHdr) & 0xE0000000UL) == 0x00UL); \
  FIFO_ASSERT(); \
  GR_ASSERT(gc->cRegs->cmdFifo0.readPtrL >= gc->cmdTransportInfo.fifoOffset);\
  GR_ASSERT(gc->cRegs->cmdFifo0.readPtrL < (gc->cmdTransportInfo.fifoOffset + gc->cmdTransportInfo.fifoSize));\
  GDBG_INFO(220, "REG_GROUP_BEGIN_WAX:\n");\
  GDBG_INFO(220, "\tFile: %s Line %d\n", __FILE__, __LINE__);\
  GDBG_INFO(220, "\t_regGroupFifoPtr:  0x%x\n", (FxU32)_regGroupFifoPtr - (FxU32)gc->rawLfb);\
  GDBG_INFO(220, "\t__pktHdr:           0x%x\n", __pktHdr);\
  GDBG_INFO(220, "\t__groupNum:         0x%x\n", __groupNum);\
  GDBG_INFO(220, "\t__groupMask:        0x%x\n", (__groupMask));\
  GDBG_INFO(220, "\t__regBase:          0x%x\n", offsetof(SstGRegs, __regBase));\
  GDBG_INFO(220, "\tfifoPtr:            0x%x\n", (FxU32)gc->cmdTransportInfo.fifoPtr - (FxU32)gc->cmdTransportInfo.fifoStart - (FxU32)gc->rawLfb); \
  GDBG_INFO(220, "\tfifoRoom:           0x%x\n", gc->cmdTransportInfo.fifoRoom);\
  GDBG_INFO(220, "\treadPtrL:           0x%x\n", gc->cRegs->cmdFifo0.readPtrL);\
  GDBG_INFO(220, "\tStart Reg:          0x%x\n", (__pktHdr & 0x7fff) >> 3);\
  GDBG_INFO(220, "\tReg Mask:           0x%x\n", (__pktHdr >> 15) & 0x3fff);\
  GDBG_INFO(220, "\tReg Type:           %s\n", ((__pktHdr >> 14) & 1) ? "2D" : "3D");\
  SET(*_regGroupFifoPtr++, (__pktHdr))


#define REG_GROUP_SET(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT(__regAddr, __val, FXFALSE); \
  FXUNUSED(__regBase); \
  GDBG_INFO(120, "fifoReadPtr(HW): 0x%x\n", GETREG(gc->cRegs->cmdFifo0.readPtrL));\
  GR_ASSERT(GETREG(gc->cRegs->cmdFifo0.readPtrL) >= gc->cmdTransportInfo.fifoOffset);\
  GR_ASSERT(GETREG(gc->cRegs->cmdFifo0.readPtrL) < (gc->cmdTransportInfo.fifoOffset + gc->cmdTransportInfo.fifoSize));\
  GDBG_INFO(120, "REG_GROUP_SET:\n");\
  GDBG_INFO(120, "\tFile: %s Line %d\n", __FILE__, __LINE__);\
  GDBG_INFO(120, "\tfifoPtr: 0x%x, Val: 0x%x\n", (FxU32) _regGroupFifoPtr - (FxU32)gc->rawLfb, __val);\
  SET(*_regGroupFifoPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define REG_GROUP_SET_WAX(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT_WAX(__regAddr, __val, FXFALSE); \
  FXUNUSED(__regBase); \
  GDBG_INFO(220, "fifoReadPtr(HW): 0x%x\n", gc->cRegs->cmdFifo0.readPtrL);\
  GR_ASSERT(gc->cRegs->cmdFifo0.readPtrL >= gc->cmdTransportInfo.fifoOffset);\
  GR_ASSERT(gc->cRegs->cmdFifo0.readPtrL < (gc->cmdTransportInfo.fifoOffset + gc->cmdTransportInfo.fifoSize));\
  GDBG_INFO(220, "REG_GROUP_SET_WAX:\n");\
  GDBG_INFO(220, "\tFile: %s Line %d\n", __FILE__, __LINE__);\
  GDBG_INFO(220, "\tfifoPtr: 0x%x, Val: 0x%x\n", (FxU32) _regGroupFifoPtr - (FxU32)gc->rawLfb, __val);\
  SET(*_regGroupFifoPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define REG_GROUP_SETF(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT(__regAddr, __val, FXTRUE); \
  FXUNUSED(__regBase); \
  SETF(*(FxFloat*)_regGroupFifoPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxFloat)); \
} while(0)

#if GLIDE_FP_CLAMP
#define REG_GROUP_SETF_CLAMP(__regBase, __regAddr, __val) \
do { \
  const FxU32 fpClampVal = FP_FLOAT_CLAMP(__val); \
  REG_GROUP_ASSERT(__regAddr, fpClampVal, FXTRUE); \
  FXUNUSED(__regBase); \
  SETF(*(FxFloat*)_regGroupFifoPtr++, fpClampVal); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
#else
#define REG_GROUP_SETF_CLAMP(__regBase, __regAddr, __val) \
  REG_GROUP_SETF(__regBase, __regAddr, __val)
#endif

#define REG_GROUP_NO_CHECK_END() \
  ASSERT(!_checkP); \
  ASSERT((((FxU32)_regGroupFifoPtr - (FxU32)gc->cmdTransportInfo.fifoPtr) >> 2) == _groupNum + 1); \
  gc->cmdTransportInfo.fifoRoom -= ((FxU32)_regGroupFifoPtr - (FxU32)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = (FxU32*)_regGroupFifoPtr; \
  FIFO_ASSERT(); \
}

#define REG_GROUP_END() \
  ASSERT(_checkP); \
  ASSERT((((FxU32)_regGroupFifoPtr - (FxU32)gc->cmdTransportInfo.fifoPtr) >> 2) == _groupNum + 1); \
  gc->cmdTransportInfo.fifoRoom -= ((FxU32)_regGroupFifoPtr - (FxU32)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = (FxU32*)_regGroupFifoPtr; \
  GDBG_INFO(gc->myLevel + 200, "\tGroupEnd: (0x%X : 0x%X) : (0x%X : 0x%X)\n", \
            _regGroupFifoPtr, gc->cmdTransportInfo.fifoRoom, \
            HW_FIFO_PTR(FXTRUE), gc->cmdTransportInfo.fifoPtr); \
  FIFO_ASSERT(); \
} \
GR_CHECK_SIZE()


#if !GLIDE_HW_TRI_SETUP || HOOPTI_TRI_SETUP_COMPARE
/* Send all of the triangle parameters in a single cmd fifo packet to
 * the chip until the tsu is fixed.
 */
#define kNumTriParam 0x1FUL
   
#define TRI_NO_TSU_BEGIN(__floatP) \
GR_CHECK_COMPATABILITY(FN_NAME, \
                       !gc->open, \
                       "Called before grSstWinOpen()"); \
GR_CHECK_COMPATABILITY(FN_NAME, \
                       (gc->cmdTransportInfo.lfbLockCount != 0), \
                       "Called within grLfbLock/grLfbUnlockPair"); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * kNumTriParam, 1); \
{ \
   FxU32* noTsuFifoPtr = gc->cmdTransportInfo.fifoPtr; \
   volatile FxU32* regBaseAddr = &hw->FvA.x; \
   FIFO_ASSERT(); \
   GR_ASSERT(__floatP); \
   SET(*noTsuFifoPtr++, ((kNumTriParam << SSTCP_PKT1_NWORDS_SHIFT) | /* size (32bit words) */ \
                         SSTCP_INC |                                 /* sequential writes */ \
                         FIFO_REG(BROADCAST_ID, FvA.x) |               /* chip[14:10] num[9:3] */ \
                         SSTCP_PKT1));                               /* type (1) */ \
   GDBG_INFO(gc->myLevel, "TRI_NO_TSU_BEGIN: (fbiRegs->%svA : 0x%X)\n", \
             ((__floatP) ? "F" : ""), (FxU32)noTsuFifoPtr)

#define TRI_NO_TSU_SET(__addr, __val) \
do { \
   const FxU32 hwWriteAddr = (FxU32)(__addr); \
   ASSERT(hwWriteAddr == (FxU32)regBaseAddr); \
   SET(*noTsuFifoPtr++, (__val)); \
   GR_INC_SIZE(sizeof(FxU32)); \
   regBaseAddr++; \
} while(0)

#define TRI_NO_TSU_SETF(__addr, __val) \
do { \
   const FxU32 hwWriteAddr = (FxU32)(__addr); \
   const FxFloat hwFloatVal = __val; \
   ASSERT(hwWriteAddr == (FxU32)regBaseAddr); \
   GDBG_INFO(gc->myLevel + 200, FN_NAME": FloatVal 0x%X : (0x%X : %g)\n", \
             ((FxU32)hwWriteAddr - (FxU32)hw) >> 2, \
             *(const FxU32*)&hwFloatVal, hwFloatVal); \
   SETF(*noTsuFifoPtr++, hwFloatVal); \
   GR_INC_SIZE(sizeof(FxU32)); \
   regBaseAddr++; \
} while(0)
   
#define TRI_NO_TSU_END() \
   gc->cmdTransportInfo.fifoRoom -= ((FxU32)noTsuFifoPtr - \
                                 (FxU32)gc->cmdTransportInfo.fifoPtr); \
   gc->cmdTransportInfo.fifoPtr = noTsuFifoPtr; \
   FIFO_ASSERT(); \
}
#endif /* !GLIDE_HW_TRI_SETUP || HOOPTI_TRI_SETUP_COMPARE */

#define STORE_FIFO(__chipId, __base, __field, __val) \
do { \
   FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
   FXUNUSED(__base); \
   GR_ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          (gc->cmdTransportInfo.lfbLockCount != 0), \
                          "Called within grLfbLock/grLfbUnlockPair"); \
   DEBUGFIFOWRITE(&((SstRegs*)(__base))->__field, __val, curFifoPtr); \
   SET(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
                       FIFO_REG(__chipId, __field) |          /* chip[14:10] num[9:3] */ \
                       SSTCP_PKT1));                          /* type (1) */ \
   SET(*curFifoPtr++, __val); \
   gc->cmdTransportInfo.fifoPtr += 2; \
   gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
   FIFO_ASSERT(); \
   GR_INC_SIZE(sizeof(FxU32));  /* Size of actual write not including header */ \
} while(0)

#define STORE_FIFO_WAX(__chipId, __base, __field, __val) \
do { \
   FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
   FXUNUSED(__base); \
   GR_ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          (gc->cmdTransportInfo.lfbLockCount != 0), \
                          "Called within grLfbLock/grLfbUnlockPair"); \
   DEBUGFIFOWRITE(&((SstGRegs*)(__base))->__field, __val, curFifoPtr); \
   SET(*curFifoPtr++,\
     ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
       FIFO_REG_WAX(__field) | /* chip[14:10] num[9:3] */ \
       SSTCP_PKT1 | FXBIT(14)));                          /* type (1) */ \
   SET(*curFifoPtr++, __val); \
   gc->cmdTransportInfo.fifoPtr += 2; \
   gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
   FIFO_ASSERT(); \
   GR_INC_SIZE(sizeof(FxU32));  /* Size of actual write not including header */ \
} while(0)


#define STORE_FIFO_INDEX(__chipId, __base, __regIndex, __val) \
do { \
   FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
   FXUNUSED(__base); \
   GR_ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          (gc->cmdTransportInfo.lfbLockCount != 0), \
                          "Called within grLfbLock/grLfbUnlockPair"); \
   DEBUGFIFOWRITE(&((FxU32*)(__base))[__regIndex], __val, curFifoPtr); \
   SET(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
                       ((__chipId) << kChipFieldShift) |      /* chip[14:10] */ \
                       ((__regIndex) << 3) |                    /* Reg Num[9:3] */ \
                       SSTCP_PKT1));                          /* type (1) */ \
   SET(*curFifoPtr++, __val); \
   gc->cmdTransportInfo.fifoPtr += 2; \
   gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
   FIFO_ASSERT(); \
   GR_INC_SIZE(sizeof(FxU32));  /* Size of actual write not including header */ \
} while(0)

#define STOREF_FIFO_INDEX(__chipId, __base, __regIndex, __val) \
do { \
   FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
   FXUNUSED(__base); \
   GR_ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          (gc->cmdTransportInfo.lfbLockCount != 0), \
                          "Called within grLfbLock/grLfbUnlockPair"); \
   DEBUGFIFOFWRITE(&((FxU32*)(__base))[__regIndex], __val, curFifoPtr); \
   SET(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
                       ((__chipId) << kChipFieldShift) |      /* chip[14:10] */ \
                       ((__regIndex) << 3) |                    /* Reg Num[9:3] */ \
                       SSTCP_PKT1));                          /* type (1) */ \
   SETF(*curFifoPtr++, __val); \
   gc->cmdTransportInfo.fifoPtr += 2; \
   gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
   FIFO_ASSERT(); \
   GR_INC_SIZE(sizeof(FxU32));  /* Size of actual write not including header */ \
} while(0)

#define STORE16_FIFO(__chipId, __base, __field, __val) \
do { \
   FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
   const FxU32 temp32 = (((FxU32)(__val)) & 0x0000FFFF); \
   FXUNUSED(__base); \
   ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          (gc->cmdTransportInfo.lfbLockCount != 0), \
                          "Called within grLfbLock/grLfbUnlockPair"); \
   DEBUGFIFOWRITE(&((SstRegs*)(__base))->__field, __val, curFifoPtr); \
   SET(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |       /* size (32bit words) */ \
                       FIFO_REG(__chipId, __field) |             /* chip[14:10] num[9:3] */ \
                       SSTCP_PKT1));                             /* type (1) */ \
   SET(*curFifoPtr++, temp32); \
   gc->cmdTransportInfo.fifoPtr += 2; \
   gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
   FIFO_ASSERT(); \
   GR_INC_SIZE(sizeof(FxU32)); /* Size of actual write not including header */ \
} while(0)

#define STOREF_FIFO(__chipId, __base, __field, __val) \
do { \
   FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
   FXUNUSED(__base); \
   ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          (gc->cmdTransportInfo.lfbLockCount != 0), \
                          "Called within grLfbLock/grLfbUnlockPair"); \
   DEBUGFIFOFWRITE(&((SstRegs*)(__base))->__field, __val, curFifoPtr); \
   SET(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
                       FIFO_REG(__chipId, __field) |          /* chip[14:10] num[9:3] */ \
                       SSTCP_PKT1));                          /* type (1) */ \
   SETF(*(FxFloat*)curFifoPtr, __val); \
   curFifoPtr++; \
   gc->cmdTransportInfo.fifoPtr += 2; \
   gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
   FIFO_ASSERT(); \
   GR_INC_SIZE(sizeof(FxU32)); /* Size of actual write not including header */ \
} while(0)

/* There are now three different flavors of the packet 3 macros for
 * your coding pleasure. In increasing order of complexity and control
 * they are TRI_BEGIN, TRI_STRIP_BEGIN, TRI_PACKET_BEGIN.
 * 
 * NB: All of these macros must be terminated w/ a matching invocation of
 *     TRI_END otherwise all sorts of hell will break loose.
 * 
 * TRI_BEGIN: 
 *   The simplest form that draws a single indepependent triangle whose 
 *   parameters and culling are all the glide defaults for grDrawTriangle.
 *
 * TRI_STRIP_BEGIN:
 *   setupMode:  [kSetupStrip | kSetupFan]. Culling defaults to the current
 *               glide setting, w/ strips/fans defaulting to ping-pong culling
 *   nVertex:    The number of vertices for the current packet (max 15).
 *   vertexSize: Size in bytes of the parameters for the vertices making up
 *               the current packet.
 *   cmd:        [SSTCP_PKT3_BDDBDD (Independent)
 *                SSTCP_PKT3_BDDDDD (Start strip/fan)
 *                SSTCP_PKT3_DDDDDD (Continue strip)]
 *
 * TRI_PACKET_BEGIN:
 *   setupMode:  The same as with TRI_STRIP_BEGIN, except that the caller
 *               needs to specify the culling bits kSetupCullXXX/kSetupPingPongXXX.
 *   params:     Bits matching the descriptin of the sMode register describing 
 *               which parameters are specified in the packet.
 *   nVertex:    See TRI_STRIP_BEGIN.
 *   vertexSize: See TRI_STRIP_BEGIN.
 *   cmd:        See TRI_STRIP_BEGIN.
 */
#define TRI_PACKET_BEGIN(__setupMode, __params, __nVertex, __vertexSize, __cmd) \
{ \
  FxU32* tPackPtr = gc->cmdTransportInfo.fifoPtr; \
  const FxU32 packetVal = (((__setupMode) << SSTCP_PKT3_SMODE_SHIFT) |   /* [27:22] */ \
                           (__params) |                                  /* pack[28] params[21:10] */ \
                           ((__nVertex) << SSTCP_PKT3_NUMVERTEX_SHIFT) | /* [9:6] */ \
                           (__cmd) |                                     /* command [5:3] */ \
                           SSTCP_PKT3);                                  /* type [2:0] */ \
  TRI_ASSERT_DECL(__nVertex, __vertexSize, packetVal); \
  SET(*tPackPtr++, packetVal)

#define TRI_STRIP_BEGIN(__setupMode, __nVertex, __vertexSize, __cmd) \
{ \
  FxU32* tPackPtr = gc->cmdTransportInfo.fifoPtr; \
  const FxU32 packetVal = (((__setupMode) << SSTCP_PKT3_SMODE_SHIFT) |   /* [27:22] */ \
                           ((__nVertex) << SSTCP_PKT3_NUMVERTEX_SHIFT) | /* [9:6] */ \
                           (__cmd) |                                     /* command [5:3] */ \
                           gc->cmdTransportInfo.cullStripHdr); \
  TRI_ASSERT_DECL(__nVertex, __vertexSize, packetVal); \
  SET(*tPackPtr++, packetVal)

#define TRI_BEGIN() \
{ \
  FxU32* tPackPtr = gc->cmdTransportInfo.fifoPtr; \
  TRI_ASSERT_DECL(3, _GlideRoot.curVertexSize, gc->cmdTransportInfo.triPacketHdr); \
  SET(*tPackPtr++, gc->cmdTransportInfo.triPacketHdr)

#if GDBG_INFO_ON
extern void
_grH3FifoDump_TriHdr(const FxU32 triPacketHdr);
extern void
_grH3FifoDump_Linear(const FxU32* const linearPacketAddr);

#define DEBUGFIFODUMP_TRI(__packetAddr)    _grH3FifoDump_TriHdr(__packetAddr)
#define DEBUGFIFODUMP_LINEAR(__packetAddr) _grH3FifoDump_Linear(__packetAddr)

#define TRI_ASSERT_DECL(__nVerts, __vertSize, __packetHdr) \
  const FxU32 nVertex = (__nVerts); \
  const FxU32 sVertex = (__vertSize); \
  FxU32 pCount = 0; \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         !gc->open, \
                         "Called before grSstWinOpen()"); \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         (gc->cmdTransportInfo.lfbLockCount != 0), \
                         "Called within grLfbLock/grLfbUnlockPair"); \
  GR_ASSERT(((FxU32)(tPackPtr) & FIFO_ALIGN_MASK) == 0);   /* alignment */ \
  GR_ASSERT((((__nVerts) * (__vertSize)) + sizeof(FxU32)) <= (FxU32)gc->cmdTransportInfo.fifoRoom); \
  GR_ASSERT((((FxU32)tPackPtr) + ((__nVerts) * (__vertSize)) + sizeof(FxU32)) < \
            (FxU32)gc->cmdTransportInfo.fifoEnd); \
  GR_ASSERT(nVertex < 0x10); \
  GR_ASSERT(nVertex > 0x00); \
  GR_ASSERT(((__packetHdr) & 0xE0000000UL) == 0x00UL); \
  FIFO_ASSERT(); \
  GDBG_INFO(120, "Triangle(0x%X): (0x%X : 0x%X)\n", (__packetHdr), __nVerts, __vertSize); \
  DEBUGFIFODUMP_TRI(__packetHdr)
#define CLAMP_DUMP(__val, __floatVal) \
  pCount++; \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%X) : V#: 0x%X - P#: 0x%X - ParamVal: (%f : 0x%X)\n", \
            (FxU32)tPackPtr, \
            ((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) / sVertex, \
             (((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) % sVertex) >> 2, \
            (((__val) < 786432.875) ? (__val) : ((__val) - 786432.875)), \
            (__floatVal))
#define SETF_DUMP(__val) \
  pCount++; \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%X) : V#: 0x%X - P#: 0x%X - ParamVal: %f\n", \
            (FxU32)tPackPtr, \
            ((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) / sVertex, \
             (((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) % sVertex) >> 2, \
            (((__val) < 786432.875) ? (__val) : ((__val) - 786432.875)))
#define SET_DUMP(__val) \
  pCount++; \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%X) : V#: 0x%X - P#: 0x%X - ParamVal: 0x%X\n", \
            (FxU32)tPackPtr, \
            ((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) / sVertex, \
             (((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) % sVertex) >> 2, \
            (__val))
#define TRI_ASSERT() \
  GR_ASSERT(pCount == (nVertex * (sVertex >> 2))); \
  ASSERT(((FxU32)tPackPtr - (FxU32)gc->cmdTransportInfo.fifoPtr) == (nVertex * sVertex) + sizeof(FxU32))
#else /* !GDBG_INFO_ON */
#define DEBUGFIFODUMP_TRI(__packetAddr)
#define DEBUGFIFODUMP_LINEAR(__packetAddr)

#define CLAMP_DUMP(__val, __floatVal) 
#define SETF_DUMP(__val)
#define SET_DUMP(__val)

#define TRI_ASSERT_DECL(__nVerts, __vertSize, __packetHdr)
#define TRI_ASSERT()
#endif /* !GDBG_INFO_ON */

/* Get the integer representation of the color component.  Currently,
 * following in the 'Glide is not an API for kids' tradition we'll
 * probably do something silly like wrap around zero.
 */
#if GLIDE_PACKED_RGB
#define RGBA_COMP(__fpVal, __fpBias, __fpShift, __fpMask) \
((_GlideRoot.pool.ftemp1 = (float)((float)(__fpVal) + (float)(__fpBias))), \
 GR_ASSERT((__fpVal) >= 0.0f), \
 GR_ASSERT((__fpVal) < 256.0f), \
 (((*(const FxU32*)&_GlideRoot.pool.ftemp1) & (__fpMask)) << (__fpShift)))
                                                  
#define RGBA_COMP_CLAMP(__fpVal, __compToken) \
   RGBA_COMP(__fpVal, kPackBias##__compToken, kPackShift##__compToken, kPackMask##__compToken)
#endif /* GLIDE_PACKED_RGB */

/* First stage tsu-subtractor chec/fix. 
 * Mmm..... sequence operator.
 */
#if GLIDE_FP_CLAMP
#define kFPClampThreshold 0x20UL
#define FP_FLOAT_CLAMP(__fpVal) ((FP_FLOAT_EXP(__fpVal) < kFPClampThreshold) \
                                 ? (_GlideRoot.stats.tsuValClamp++, 0x00UL) \
                                 : *(const FxU32*)(&(__fpVal)))

#define TRI_SETF_CLAMP(__val) \
do { \
  const FxU32 floatCastVal = FP_FLOAT_CLAMP(__val); \
  CLAMP_DUMP(__val, floatCastVal); \
  SET(*tPackPtr++, floatCastVal); \
  GR_INC_SIZE(sizeof(FxFloat)); \
} while(0)
#else
#define TRI_SETF_CLAMP(__val) \
    TRI_SETF(__val)
#endif

#define TRI_SETF(__val) \
do { \
  SETF_DUMP(__val); \
  SETF(*tPackPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxFloat)); \
} while(0)

#define TRI_SET(__val) \
do { \
  SET_DUMP(__val); \
  SET(*tPackPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define TRI_END \
  TRI_ASSERT(); \
  gc->cmdTransportInfo.fifoRoom -= ((FxU32)tPackPtr - (FxU32)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = tPackPtr; \
  GDBG_INFO(gc->myLevel + 200, "\tTriEnd: (0x%X : 0x%X)\n", tPackPtr, gc->cmdTransportInfo.fifoRoom); \
  FIFO_ASSERT(); \
}

#define FIFO_LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN, __f, __l) \
{ \
  FxU32* packetPtr = gc->cmdTransportInfo.fifoPtr; \
  const FxU32 writeSize = (__numWords);       /* Add size of packet header */ \
  const FxU32 hdr1 = ((__type) | \
                      (((FxU32)(__maskW2)) << SSTCP_PKT5_BYTEN_W2_SHIFT) | \
                      (((FxU32)(__maskWN)) << SSTCP_PKT5_BYTEN_WN_SHIFT) | \
                      (writeSize << SSTCP_PKT5_NWORDS_SHIFT) | \
                      SSTCP_PKT5); \
  const FxU32 hdr2 = ((FxU32)(__addr)) & SSTCP_PKT5_BASEADDR; \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         !gc->open, \
                         "Called before grSstWinOpen()"); \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         (gc->cmdTransportInfo.lfbLockCount != 0), \
                         "Called within grLfbLock/grLfbUnlockPair"); \
  GR_ASSERT(((FxU32)(packetPtr) & FIFO_ALIGN_MASK) == 0);        /* alignment */ \
  GR_ASSERT((__numWords) > 0);                                   /* packet size */ \
  GR_ASSERT((__numWords) < ((0x01 << 19) - 2)); \
  GR_ASSERT((((FxU32)(__numWords) + 2) << 2) <= (FxU32)gc->cmdTransportInfo.fifoRoom); \
  GR_ASSERT(((FxU32)packetPtr + (((FxU32)(__numWords) + 2) << 2)) < \
            (FxU32)gc->cmdTransportInfo.fifoEnd); \
  GR_ASSERT((hdr2 & 0xE0000000UL) == 0x00UL); \
  GR_ASSERT(1 /* (((FxU32)(__type)) >= ((FxU32)SSTCP_PKT5_3DLFB)) &&  \
            (((FxU32)(__type)) <= ((FxU32)SSTCP_PKT5_TEXPORT))*/); \
  FIFO_ASSERT(); \
  GDBG_INFO(120, "LinearWrite(0x%X : 0x%X)\n", hdr1, hdr2); \
  GDBG_INFO(gc->myLevel + 200, "\tFile: %s - Line: %ld\n", __f, __l); \
  GDBG_INFO(gc->myLevel + 200, "\tType: 0x%X\n", (FxU32)(__type)); \
  GDBG_INFO(gc->myLevel + 200, "\tAddr: 0x%X\n", (FxU32)(__addr)); \
  GDBG_INFO(gc->myLevel + 200, "\tMaskW2: 0x%X\n", (FxU32)(__maskW2)); \
  GDBG_INFO(gc->myLevel + 200, "\tMaskWN: 0x%X\n", (FxU32)(__maskWN)); \
  GDBG_INFO(gc->myLevel + 200, "\twriteSize: 0x%X\n", writeSize); \
  GDBG_INFO(gc->myLevel + 200, "\thdr 1: 0x%X\n", hdr1); \
  GDBG_INFO(gc->myLevel + 200, "\thdr 2: 0x%X\n", hdr2); \
  SET(*packetPtr++, hdr1); \
  SET(*packetPtr++, hdr2); \
  GR_INC_SIZE(sizeof(FxU32))

#define FIFO_LINEAR_WRITE_SET(__val) \
do { \
  GDBG_INFO(gc->myLevel + 205, "\t0x%X : 0x%X\n", packetPtr, (__val)); \
  GDBG_INFO(gc->myLevel + 205, "fifPtr: 0x%x, val: 0x%x\n\n", packetPtr, __val);\
  GR_ASSERT(GETREG(gc->cRegs->cmdFifo0.readPtrL) >= gc->cmdTransportInfo.fifoOffset);\
  GR_ASSERT(GETREG(gc->cRegs->cmdFifo0.readPtrL) < (gc->cmdTransportInfo.fifoOffset + gc->cmdTransportInfo.fifoSize));\
  SET_LINEAR(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
  
#define FIFO_LINEAR_WRITE_SET_16(__val) \
do { \
  GDBG_INFO(gc->myLevel + 205, "\t0x%X : 0x%X\n", packetPtr, (__val)); \
  SET_LINEAR_16(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
  
#define FIFO_LINEAR_WRITE_SET_8(__val) \
do { \
  GDBG_INFO(gc->myLevel + 205, "\t0x%X : 0x%X\n", packetPtr, (__val)); \
  SET_LINEAR_8(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define FIFO_LINEAR_WRITE_END \
  DEBUGFIFODUMP_LINEAR(gc->cmdTransportInfo.fifoPtr); \
  GR_ASSERT((((FxU32)packetPtr - (FxU32)gc->cmdTransportInfo.fifoPtr) >> 2) == writeSize + 2); \
  gc->cmdTransportInfo.fifoRoom -= ((FxU32)packetPtr - (FxU32)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = packetPtr; \
  GDBG_INFO(gc->myLevel + 200, "\tLinearEnd: (0x%X : 0x%X)\n", \
            packetPtr, gc->cmdTransportInfo.fifoRoom); \
  FIFO_ASSERT(); \
}

#  define GR_GET(s)                 GET(s)
#  define GR_CAGP_GET(__reg) GET(gc->cRegs->cmdFifo0.__reg)
#  define GR_CAGP_SET(__reg, __val) SET(gc->cRegs->cmdFifo0.__reg, __val)
#  define GR_GET16(s)               ((FxU16)GET16(s))
#  define GR_SET(c, h, f, s)        STORE_FIFO(c, h, f, s)
#  define GR_SET_WAX(c, h, f, s)    STORE_FIFO_WAX(c, h, f, s)
#  define GR_SET_INDEX(c, h, r, s)  STORE_FIFO_INDEX(c, h, r, s)
#  define GR_SET16(c, h, f, s)      STORE16_FIFO(c, h, f, s)
#  define GR_SETF(c, h, f, s)       STOREF_FIFO(c, h, f, s)
#  define GR_SETF_INDEX(c, h, r, s) STOREF_FIFO_INDEX(c, h, r, s)
#  define GR_SET_DIRECT(__bc, __hw, __reg, __val)   gc->sstRegs->__reg = __val;GR_INC_SIZE(sizeof(__val))
#  define GR_SET_IO(c, h, f, s) (gc->ioRegs)->f = s
#else /* !USE_PACKET_FIFO */
#  ifndef DIRECT_IO
#    define GR_GET(s)                 GET(s)
#    define GR_CAGP_GET(__reg) GET(gc->cRegs->cmdFifo0__reg)
#    define GR_GET16(s)               ((FxU16)GET16(s))
#    define GR_SET(c, h, f, s)        do {SET((h)->f, s); GR_INC_SIZE(4);} while(0)
#    define GR_SET_INDEX(c, h, r, s)  do {SET(((FxU32*)(h))[r], s); GR_INC_SIZE(sizeof(FxU32));} while(0)
#    define GR_SETF(c, h, f, s)       do {SETF(h->f, s); GR_INC_SIZE(4);} while(0)
#    define GR_SETF_INDEX(c, h, r, s) do {SETF(((FxU32*)(h))[r], s); GR_INC_SIZE(sizeof(FxU32));} while(0)
#    define GR_SET16(c, h, f, s)      do {SET16((h)->f, s); GR_INC_SIZE(2);} while(0)
#    define GR_SET_IO(c,h,f,s) GR_SET(c, h, f, s)
#    define GR_SET_DIRECT(c,h,f,s) GR_SET(c, h, f, s)
#  else
#    define GR_GET(s)   GET(s)
#    define GR_CAGP_GET(__reg) gc->cRegs->cmdFifo0.reg
#    define GR_GET16(s) ((FxU16)GET16(s))
#    define GR_SET(c, h, f, s) {(gc->sstRegs)->f = s; GR_INC_SIZE(4);}
#    define GR_SETF(c, h, f, s) {((volatile float) (gc->sstRegs)->f) = (float) (s); GR_INC_SIZE(4);}
#    define GR_SET_INDEX(c, h, r, s) {((FxU32 *)(gc->sstRegs))[r] = s; GR_INC_SIZE(4);}
#    define GR_SETF_INDEX(c, h, r, s) {((volatile float *)(gc->sstRegs))[r] = (float) (s); GR_INC_SIZE(4);}
#    define GR_SET16(c, h, f, s) {*((FxU16 *)((gc->sstRegs)->f)) = s; GR_INC_SIZE(4);}
#    define GR_SET_DIRECT(__b, __ptr, __reg, __val) \
{\
  __ptr->__reg = __val;\
  GR_INC_SIZE(4);\
}
#    define GR_SET_IO(c, h, f, s) (gc->ioRegs)->f = s
#  endif                                   
#endif /* !USE_PACKET_FIFO */

/* Macros to do linear writes to lfb/tex memory. 
 *
 * LINEAR_WRITE_BEGIN - Setup stuff for the linear write. 
 *
 * numWords: The number of words to actually write to the destination
 * address. This does *NOT* include the packet headers etc for any
 * command fifos etc.
 *
 * type: One of the kLinearWriteXXX enum values above. This can
 * control what the legal values for addr and maskXX are.
 *
 * addr: Base address to the start the write.
 *
 * maskXX: Control what bytes in a write are active, these are active
 * low. W2 controls the masking of the first 32bit word written, and
 * WN controls all of the other writes.
 *
 * LINEAR_WRITE_SET - Writes are done in 32-bit increments, and must
 * be properly aligned etc. This can only be used inside of a
 * LINEAR_WRITE_BEGIN/LINEAR_WRITE_END pair.
 *
 * LINEAR_WRITE_EDGE - Write to a 16-bit value to an address. The
 * address must be aligned for at 16-bit access, and should not appear
 * within a LINEAR_WRITE_BEGIN/LINEAR_WRITE_END pair.
 *
 * LINEAR_WRITE_END - Finish off any stuff for the linear write.  
 */

#if USE_PACKET_FIFO
#define LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN) \
{ \
   GR_SET_EXPECTED_SIZE(((FxU32)((__numWords) + 1) << 2), 1); \
   FIFO_LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN, __FILE__, __LINE__)
#define LINEAR_WRITE_SET(__addr, __val) \
   FIFO_LINEAR_WRITE_SET(__val)
#define LINEAR_WRITE_SET_16(__addr, __val) \
   FIFO_LINEAR_WRITE_SET_16(__val)
#define LINEAR_WRITE_SET_8(__addr, __val) \
   FIFO_LINEAR_WRITE_SET_8(__val)
#define LINEAR_WRITE_END() \
   FIFO_LINEAR_WRITE_END; \
   GR_CHECK_SIZE(); \
}

/* Macro to write the edge cases of a linear write, for example to the
 * lfb w/ a 16-bit pixel value. We do some address manipulation here
 * since the cmd fifo only addresses 32-bit quantities, but allows us
 * to mask of crap for the actual write.
 *
 */
#if (GLIDE_PLATFORM & GLIDE_ENDIAN_LITTLE)
#define kWordMask   0x0CUL
#define kWordShift  0
#elif (GLIDE_PLATFORM & GLIDE_ENDIAN_BIG)
#define kWordMask   0x03UL
#define kWordShift  16
#else
#error "Must define processor endian-ness for LINEAR_WRITE_EDGE"
#endif /* Endian-ness check */
   
#define LINEAR_WRITE_EDGE(__type, __addr, __val, __alignedP) \
do { \
   LINEAR_WRITE_BEGIN(1, __type, \
                      ((FxU32)__addr & ~0x03UL), \
                      ((__alignedP) ? kWordMask : (0x0F & (~kWordMask))), 0x00); \
   FIFO_LINEAR_WRITE_SET(((FxU32)(__val)) << ((__alignedP) ? kWordShift : (16 - kWordShift))); \
   LINEAR_WRITE_END(); \
} while(0) 
#else /* !USE_PACKET_FIFO */
# define LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN) \
{ \
    GR_SET_EXPECTED_SIZE(((__numWords) << 2), (__numWords))
# define LINEAR_WRITE_SET(__addr, __val) \
do { \
   FxU32* tempAddr = (FxU32*)(__addr); \
   SET(*tempAddr, __val); \
   GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

# define LINEAR_WRITE_SET_16(__addr, __val)\
    LINEAR_WRITE_SET(__addr, (FxU32) __val)
# define LINEAR_WRITE_SET_8(__addr, __val)  \
    LINEAR_WRITE_SET(__addr, (FxU32) __val)

# define LINEAR_WRITE_EDGE(__type, __addr, __val, __alignedP) \
do { \
   FxU32* tempAddr = (FxU32*)(__addr); \
   SET16(*tempAddr, __val); \
   GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
# define LINEAR_WRITE_END() \
  GR_CHECK_SIZE(); \
}

/* The REG_GROUP_XXX macros do writes to a monotonically increasing
 * set of registers. There are three flavors of the macros w/
 * different restrictions etc.
 *
 * NB: Care must be taken to order the REG_GROUP_SET macro uses to
 * match the actual register order, otherwise all hell breaks loose.  
 */

/* Write to __groupNum registers (max 14) starting at __regBase under
 * the control of __groupMask (lsb->msb).
 */
#define REG_GROUP_BEGIN(__chipId, __regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL(__regBase, __groupNum, __groupMask, FXTRUE)

#define REG_GROUP_BEGIN_WAX( __regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL_WAX(__regBase, __groupNum, __groupMask, FXTRUE)

/* Same as the non-NO_CHECK variant, but GR_SET_EXPECTED_SIZE must
 * have already been called to allocate space for this write.  
 */
#define REG_GROUP_NO_CHECK_BEGIN(__chipId, __regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_ASSERT(gc->expected_counter >= (FxI32)((__groupNum) * sizeof(FxU32))); \
REG_GROUP_BEGIN_INTERNAL(__regBase, __groupNum, __groupMask, FXFALSE)

/* Register writes (<= 32) sequentially starting at __regBase */
#define REG_GROUP_LONG_BEGIN(__chipId, __regBase, __groupNum) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 32)); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL(__regBase, __groupNum, (0xFFFFFFFF >> (32 - (__groupNum))), FXTRUE)

#define REG_GROUP_BEGIN_INTERNAL(__regBase, __groupNum, __groupMask, __checkP) \
{ \
  GR_DCL_GC; \
  REG_GROUP_DCL(__groupMask, __regBase, __groupNum, __checkP); \
  GDBG_INFO(gc->myLevel + 100, "REG_GROUP_BEGIN: (0x%X : 0x%X)\n", \
            (__groupMask), offsetof(SstRegs, __regBase) >> 2)

#define REG_GROUP_BEGIN_INTERNAL_WAX(__regBase, __groupNum, __groupMask, __checkP) \
{ \
  GR_DCL_GC; \
  REG_GROUP_DCL_WAX(__groupMask, __regBase, __groupNum, __checkP); \
  GDBG_INFO(220, "REG_GROUP_BEGIN_WAX: (0x%X : 0x%X)\n", \
            (__groupMask), offsetof(SstGRegs, __regBase) >> 2)


#define REG_GROUP_SET(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT(__regAddr, __val, FXFALSE); \
  SET(((SstRegs*)(__regBase))->__regAddr, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define REG_GROUP_SET_WAX(__regBase, __regAddr, __val) \
do { \
  SET(((SstGRegs*)((FxU32) __regBase + SST_2D_OFFSET))->__regAddr, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)


#define REG_GROUP_SETF(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT(__regAddr, __val, FXTRUE); \
  SETF(((SstRegs*)(__regBase))->__regAddr, (__val)); \
  GR_INC_SIZE(sizeof(FxFloat)); \
} while(0)

#if GLIDE_FP_CLAMP
#define REG_GROUP_SETF_CLAMP(__regBase, __regAddr, __val) \
do { \
  const FxU32 fpClampVal = FP_FLOAT_CLAMP(__val); \
  REG_GROUP_ASSERT(__regAddr, fpClampVal, FXTRUE); \
  SET(((FxU32*)(__regBase))[offsetof(SstRegs, __regAddr) >> 2], fpClampVal); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
#else
#define REG_GROUP_SETF_CLAMP(__regBase, __regAddr, __val) \
  REG_GROUP_SETF(__regBase, __regAddr, __val)
#endif

#define REG_GROUP_NO_CHECK_END() \
  ASSERT(!_checkP); \
}

#define REG_GROUP_END() \
  ASSERT(_checkP); \
} \
GR_CHECK_SIZE()

#if !GLIDE_HW_TRI_SETUP || HOOPTI_TRI_SETUP_COMPARE
/* Send all of the triangle parameters in a single cmd fifo packet to
 * the chip until the tsu is fixed.
 */
#define kNumTriParam 0x1FUL
   
#define TRI_NO_TSU_BEGIN(__floatP) \
GR_CHECK_COMPATABILITY(FN_NAME, \
                       !gc->open, \
                       "Called before grSstWinOpen()"); \
GR_CHECK_COMPATABILITY(FN_NAME, \
                       (gc->cmdTransportInfo.lfbLockCount != 0), \
                       "Called within grLfbLock/grLfbUnlockPair"); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * kNumTriParam, 1); \
{ \
   volatile FxU32* regBaseAddr = (volatile FxU32*)((__floatP) \
                                                   ? &hw->FvA \
                                                   : &hw->vA); \
   GDBG_INFO(gc->myLevel, "TRI_NO_TSU_BEGIN: fbiRegs->%svA\n", \
             ((__floatP) ? "F" : ""))

#define TRI_NO_TSU_SET(__addr, __val) \
do { \
   const FxU32* hwWriteAddr = (const FxU32*)(__addr); \
   ASSERT(hwWriteAddr == regBaseAddr); \
   SET(*hwWriteAddr, (__val)); \
   GR_INC_SIZE(sizeof(FxU32)); \
   regBaseAddr++; \
} while(0)

#define TRI_NO_TSU_SETF(__addr, __val) \
do { \
   const FxU32* hwWriteAddr = (const FxU32*)(__addr); \
   const FxFloat hwFloatVal = __val; \
   ASSERT(hwWriteAddr == regBaseAddr); \
   GDBG_INFO(gc->myLevel + 200, FN_NAME": FloatVal 0x%X : (0x%X : %g)\n", \
             ((FxU32)hwWriteAddr - (FxU32)hw) >> 2, \
             *(const FxU32*)&hwFloatVal, hwFloatVal); \
   SETF(*hwWriteAddr, hwFloatVal); \
   GR_INC_SIZE(sizeof(FxU32)); \
   regBaseAddr++; \
} while(0)
   
#define TRI_NO_TSU_END() \
}
#endif /* !GLIDE_HW_TRI_SETUP || HOOPTI_TRI_SETUP_COMPARE */

#endif /* !USE_PACKET_FIFO */

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

/* This is the alignmnet requirement for texBaseAddr. Actually,
 * nothing bad is going to happen, but when the hw lops bits off of
 * the address the wrong texels are going to appear.  
 */
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
#define FARRAY(p,i)    (*(float *)((i)+(int)(p)))
#define ArraySize(__a) (sizeof(__a) / sizeof((__a)[0]))

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

#if GDBG_INFO_ON
/* cvg.c */
extern void
_grErrorCallback(const char* const procName,
                 const char* const format,
                 va_list           args);
#endif

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
 *  6: P6 Core w/ MTRR's
 *  7: P6 Core w/ KNI
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

#ifdef GLIDE3
/*
** Macro to handle clip space and viewport stuff
*/
#define TRI_SETF_SCALE_ADVANCE(_ptr,_scaler) \
    TRI_SETF(FARRAY(_ptr, i)*_scaler); dataElem++; i = gc->tsuDataList[dataElem]

#define DA_SETF_SCALE_ADVANCE(_ptr,_scaler) \
    DA_SETF(FARRAY(_ptr, i)*_scaler); dataElem++; i = gc->tsuDataList[dataElem]

#define DA_VP_SETFS(_s,_oow) \
{ \
  FxI32 i, dataElem=0; \
  i = gc->tsuDataList[dataElem]; \
  if (gc->state.paramIndex & (STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA)) { \
    if (gc->state.vData.colorType == GR_FLOAT) { \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) { \
        DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) { \
        DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
    } \
    else { \
      DA_SETF(FARRAY(_s, i)); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOZ) { \
    DA_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth + gc->state.Viewport.oz); \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) { \
    if (gc->state.vData.qInfo.offset) { \
      DA_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      DA_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  while (i != GR_DLIST_END) { \
    DA_SETF_SCALE_ADVANCE(_s,_oow*_GlideRoot.pool.f255); \
  } \
}

#define TRI_VP_SETFS(_s,_oow) \
{ \
  FxI32 i, dataElem=0; \
  i = gc->tsuDataList[dataElem]; \
  if (gc->state.paramIndex & (STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA)) { \
    if (gc->state.vData.colorType == GR_FLOAT) { \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) { \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) { \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
    } \
    else { \
      TRI_SETF(FARRAY(_s, i)); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOZ) { \
    TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz); \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) { \
    if (gc->state.vData.qInfo.offset) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  while (i != GR_DLIST_END) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*_GlideRoot.pool.f255); \
  } \
}

#define AA_TRI_VP_SETFS(_s,_oow) \
{ \
  FxI32 i, dataElem=0; \
  i = gc->tsuDataList[dataElem]; \
  if (gc->state.paramIndex & (STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA)) { \
    if (gc->state.vData.colorType == GR_FLOAT) { \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) { \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
      TRI_SETF(0.0f); \
    } \
    else { \
      FxU32 argb; \
      argb = *((FxU32 *)((int)_s + i)) & 0x00ffffff; \
      TRI_SETF(*((float *)&argb)); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOZ) { \
    TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth + gc->state.Viewport.oz); \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) { \
    if (gc->state.vData.qInfo.offset) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  while (i != GR_DLIST_END) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*_GlideRoot.pool.f255); \
  } \
}

#endif

/*
** The lod and aspect ratio changes will be done after we split the tree.
** Currently, we change the definition but patch it back to the original value
** so it is the same glide2.
** To smooth the transition from glide2 defs to glide3 defs, we introduce the 
** translation layer.
*/
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
#ifndef GLIDE3_DEBUG
//#define GLIDE3_DEBUG 1
#endif
#ifdef GLIDE3_DEBUG
#define TEX_INFO(ptr,field)                         ptr.field
#define G3_LOD_TRANSLATE(lod)                       (lod)
#define G3_ASPECT_TRANSLATE(aspect)                 (aspect)
#else
#define TEX_INFO(ptr,field)                         ptr.field ## Log2
#define G3_LOD_TRANSLATE(lod)                       (0x8-lod)
#define G3_ASPECT_TRANSLATE(aspect)                 (0x3-(aspect))
#endif /* GLIDE3_DEBUG */
#else
#define TEX_INFO(ptr,field)                         ptr.field
#define G3_LOD_TRANSLATE(lod)                       (lod)
#define G3_ASPECT_TRANSLATE(aspect)                 (aspect)
#endif

/* externals from ditex.c */
extern const FxU32 _grMipMapHostSize[4][9];
extern const int _grMipMapHostWH[GR_ASPECT_1x8 + 1][GR_LOD_1 + 1][2];
extern const FxU32 _gr_aspect_index_table[];
extern const FxU32 _gr_aspect_xlate_table[];
extern const FxU32 _gr_evenOdd_xlate_table[];

#endif /* __FXGLIDE_H__ */

