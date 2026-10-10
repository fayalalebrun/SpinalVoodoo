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
** Revision 1.1.1.1.8.15  2007/09/29 13:59:34  koolsmoky
** completed grStippleMode and grStipplePattern
**
** Revision 1.1.1.1.8.14  2006/01/16 21:22:41  jwrdegoede
** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
**
** Revision 1.1.1.1.8.13  2005/08/13 21:06:56  jwrdegoede
** Last needed 64 bit fixes for h5/h3, complete 64 bit support for cvg
**
** Revision 1.1.1.1.8.12  2005/06/10 14:17:53  jwrdegoede
** Fix compilation when GL_X86 is not defined
**
** Revision 1.1.1.1.8.11  2005/06/09 18:32:08  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.1.1.8.10  2005/05/25 08:53:10  jwrdegoede
** Add P6FENCE (ish) macro for non-x86 archs
**
** Revision 1.1.1.1.8.9  2005/05/25 08:51:49  jwrdegoede
** Add #ifdef GL_X86 around x86 specific code
**
** Revision 1.1.1.1.8.8  2004/12/23 20:24:08  koolsmoky
** remove old cpuid
**
** Revision 1.1.1.1.8.7  2004/11/25 18:56:59  koolsmoky
** support new cpuid
**
** Revision 1.1.1.1.8.6  2004/10/07 07:48:49  dborca
** comment the GR_CDECL hack to prevent accidents
**
** Revision 1.1.1.1.8.5  2004/02/16 07:42:15  dborca
** grSetNumPendingBuffers visible with grGetProcAddress
**
** Revision 1.1.1.1.8.4  2004/01/20 14:04:10  dborca
** compilation and some other minor fixes to aid in debugging
**
** Revision 1.1.1.1.8.3  2003/11/03 13:34:29  dborca
** Voodoo2 happiness (DJGPP & Linux)
**
** Revision 1.1.1.1.8.2  2003/07/25 07:13:41  dborca
** removed debris
**
** Revision 1.1.1.1.8.1  2003/06/29 18:45:55  guillemj
** Fixed preprocessor invalid token errors.
**
** Revision 1.1.1.1  1999/12/07 21:42:31  joseph
** Initial checkin into SourceForge.
**
** 
** 2     10/08/98 3:04p Atai
** added clip coords tri cull version
** 
** 54    10/08/98 2:58p Atai
** added clip coords tri cull version
** 
** 53    10/07/98 9:43p Peter
** triangle procs for 3DNow!(tm)
** 
** 52    10/06/98 8:23p Peter
** 3DNow!(tm) texture downloads
** 
** 51    10/06/98 7:18p Atai
** added triangle and drawlist asm routine for clip coords
** 
** 50    9/30/98 2:45p Peter
** removed some cruft
** 
** 49    7/24/98 1:41p Hohn
** 
** 48    7/22/98 9:18a Atai
** check in the fix for clip coords s and t with respect to aspect ratio
** 
** 47    7/16/98 5:07p Atai
** fix grTexNCCTable textureMode and clip space st coords
** 
** 46    7/16/98 10:42a Jdt
** 
** 45    7/02/98 10:28a Atai
** added guQueryResolutionXYExt
** 
** 44    7/01/98 11:31a Atai
** make grDepthBiasLevel argument FxI32
** 
** 43    6/30/98 11:46a Atai
** fixed grQueryResolution bug
** 
** 42    6/25/98 10:39a Peter
** more cb checks
** 
** 41    6/24/98 6:55p Atai
** undo texture line
** 
** 40    6/24/98 6:06p Atai
** merge the fix for trilinear bug
** 
** 39    6/24/98 1:47p Atai
** code clean up; rename texute line routine
** 
** 38    6/24/98 11:52a Atai
** fixed bug 2066
** 
** 37    6/22/98 7:00p Atai
** remove FX_EXPORT from a number of functions
** 
** 36    6/18/98 10:59a Atai
** added grDrawTextureLine for OGL
** 
** 35    6/11/98 5:12p Atai
** added aa case for OGL
** 
** 34    6/09/98 5:33p Atai
** replace grSstControl with grEnable/grDisable
** 
** 33    6/08/98 7:13p Atai
** remove unused defines, add state selectors and update state routine
** prototype
** 
** 32    6/08/98 3:20p Atai
** fix tri stats
** 
** 31    6/05/98 7:39p Atai
** fix _grAADrawLineStrip culling mode
** 
** 30    6/05/98 5:17p Atai
** added #ifdefed code for tsuDataListScaler 
** 
** 29    6/03/98 2:34p Atai
** fix chromarange
** 
** 28    6/02/98 2:13p Atai
** remvoe GrHwConfiguration in glide.h
** 
** 27    5/29/98 6:39p Atai
** fix chromarange
** 
** 26    5/28/98 5:06p Atai
** fix previous check-in
** 
** 25    5/28/98 3:02p Atai
** per-calculate vertex table size for strip/fan. make it a compiled time
** option
** 
** 24    5/21/98 6:57p Atai
** fix q0 and q1 for clip coords
** 
** 23    5/14/98 7:26p Atai
** rename texture chromarange variables
** 
** 22    5/13/98 3:47p Atai
** update for fogCoord in clip space
** 
** 19    5/11/98 4:10p Atai
** added defines for constrained query
** 
** 18    5/07/98 7:20p Atai
** added tex_table to store current palette type
** 
** 17    5/01/98 12:03p Atai
** added texture chroma mode, min and max value for tmu_config
** 
** 16    4/22/98 4:57p Peter
** glide2x merge
** 
** 15    4/21/98 1:34p Atai
** make 32 bit clean
** 
** 14    4/17/98 10:59a Atai
** added grGlideGetVertexLayout and grGlideSetVertexLayout
** 
** 13    3/21/98 11:31a Atai
** added GR_TRIANGLE_STRIP_CONTINUE and GR_TRIANGLE_FAN_CONTINUE
** 
** 12    2/23/98 11:44a Peter
** merged monitor detection and fixed compilation error from recent
** videobuffer changes
** 
** 11    2/12/98 3:40p Peter
** single buffering for opengl
** 
** 10    2/05/98 6:19p Atai
** lazy evaluate for grVertexLayout
** 
** 9     2/01/98 7:52p Peter
** grLfbWriteRegion byte count problems
** 
** 8     1/30/98 4:27p Atai
** gufog* prototype
** 
** 7     1/30/98 1:19p Atai
** fixed chromarange
** 
** 6     1/28/98 2:20p Atai
** fixed lfb state validation
** 
** 4     1/26/98 11:30a Atai
** update to new glide.h
** 
** 3     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
** 
** 2     1/20/98 11:03a Peter
** env var to force triple buffering
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 175   1/16/98 10:47a Peter
 * fixed idle muckage
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
 * fixed glide3 muckage
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
#include <cpuid.h>

#define MAX_NUM_SST 4

#if (GLIDE_PLATFORM & GLIDE_HW_H3)
#include <h3.h>

/* Compilation hacks for h3 */

/* Reserved fbzMode bits */
#define SST_DRAWBUFFER_SHIFT    14
#define SST_DRAWBUFFER          (0x3 << SST_DRAWBUFFER_SHIFT)
#define SST_DRAWBUFFER_FRONT    (0 << SST_DRAWBUFFER_SHIFT)
#define SST_DRAWBUFFER_BACK     (1 << SST_DRAWBUFFER_SHIFT)      

/* Reserved lfbMode bits */
#define SST_LFB_WRITEBUFSELECT_SHIFT    4
#define SST_LFB_WRITEBUFSELECT  (0x3<<SST_LFB_WRITEBUFSELECT_SHIFT)
#       define SST_LFB_WRITEFRONTBUFFER (0<<SST_LFB_WRITEBUFSELECT_SHIFT)
#       define SST_LFB_WRITEBACKBUFFER  (1<<SST_LFB_WRITEBUFSELECT_SHIFT)
#define SST_LFB_READBUFSELECT_SHIFT     6
#define SST_LFB_READBUFSELECT   (0x3<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READFRONTBUFFER  (0<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READBACKBUFFER   (1<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READDEPTHABUFFER (2<<SST_LFB_READBUFSELECT_SHIFT)
#   define SST_LFB_READAUXBUFFER    SST_LFB_READDEPTHABUFFER

/* Reserved textureMode bits */
#define SST_SEQ_8_DOWNLD        BIT(31)

#elif CVG
#include <cvg.h>
#else
#error "Unknown HAL hw layer"
#endif

#if GLIDE_INIT_HAL

#include <fxhal.h>

#ifdef GETENV
#undef GETENV
#endif
#define GETENV getenv

/*
 *  P6 Fence
 * 
 *  Here's the stuff to do P6 Fencing.  This is required for the
 *  certain things on the P6
 *
 * dpc - 21 may 1997 - FixMe!
 * This was yoinked from sst1/include/sst1init.h, and should be
 * merged back into something if we decide that we need it later.
 *
 * dpc - 2 june 1997 
 * Moved the fence check out to avoid empty if body warning w/ gcc.
 * This only applies to systems that require the p6 fencing.
 */
#define P6FENCE_CHECK if (i & 2) P6FENCE

#if defined(__WATCOMC__)
void 
p6Fence(void);
#pragma aux p6Fence = \
  "xchg eax, p6FenceVar" \
  modify [eax];

#define P6FENCE p6Fence()
#elif defined(__MSC__)
#define P6FENCE {_asm xchg eax, p6FenceVar}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#  define P6FENCE asm("xchg %%eax,%0" : /*outputs*/ : "m" (p6FenceVar) : \
					"eax");
#elif defined(__GNUC__) && defined(__ia64__)
# define P6FENCE asm volatile ("mf.a" ::: "memory");
#elif defined(__GNUC__) && defined(__alpha__)
# define P6FENCE asm volatile("mb" ::: "memory");
#else
#error "P6 Fencing in-line assembler code needs to be added for this compiler"
#endif /* Compiler specific fence commands */
#else /* !defined(GLIDE_INIT_HAL) */

/* All sst1init refs need to be protected inside
   GLIDE_PLATFORM & GLIDE_HW_CVG
 */

#include <sst1init.h>
/* dpc - 5 sep 1997 - FixMe!
 * Currently we're mapping directly to the init code layer
 * through the sst1XXX calls.
 *
 * #include <init.h>
 */
#endif /* !defined(GLIDE_INIT_HAL) */

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
typedef sst1VideoTimingStruct FxVideoTimingInfo;
#endif

#if GLIDE_INIT_HAL
#if (GLIDE_PLATFORM & GLIDE_HW_H3)
#define PLATFORM_IDLE_HW(__hwPtr)  fxHalIdleNoNop(__hwPtr)
#else
#define PLATFORM_IDLE_HW(__hwPtr)  fxHalIdleNoNOP(__hwPtr)
#endif

#define IDLE_HW(__hwPtr) \
do { \
  { \
    GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1); \
    GR_SET(BROADCAST_ID, __hwPtr, nopCMD, 0); \
    GR_CHECK_SIZE(); \
  } \
  PLATFORM_IDLE_HW(__hwPtr); \
} while(0)
#else /* !GLIDE_INIT_HAL */
#define IDLE_HW(__hwPtr) sst1InitIdle((FxU32*)__hwPtr)
#endif /* !GLIDE_INIT_HAL */

#if GLIDE_MULTIPLATFORM
#include "gcfuncs.h"
#endif

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#include "oeminit.h"
#endif

#define GR_SKIP_OEMDLL     0xee1feef
#define GR_NO_OEMDLL       0xee1feed

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

#ifdef GLIDE3

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
#define chromaRangeBIT          FXBIT(10)
#define stippleBIT              FXBIT(11)
/*
** lazy evaluate vertexlayout.
** it is not part of the registers so we add the bit in MSB
*/ 
#define vtxlayoutBIT            FXBIT(31)

#ifdef GLIDE3_SCALER
#define GR_CTW_SCALE_ZERO_OOW       0
#define GR_CTW_SCALE_Z              1
#define GR_CTW_SCALE_255            2
#define GR_CTW_SCALE_TMU0_S         3
#define GR_CTW_SCALE_TMU0_T         4
#define GR_CTW_SCALE_TMU1_S         5
#define GR_CTW_SCALE_TMU1_T         6
#define GR_CTW_SCALE_OOW            7
#endif

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

typedef struct
{
  GrCullMode_t                 /* these go in front for cache hits */
    cull_mode;                 /* cull neg, cull pos, don't cull   */
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
  
  struct tmu_config_t {
    FxU32   textureMode;
    FxU32   tLOD;
    FxU32   tDetail;
    FxU32   texBaseAddr;
    FxU32   texBaseAddr_1;
    FxU32   texBaseAddr_2;
    FxU32   texBaseAddr_3_8;
    FxU32   chromaKey;
    FxU32   chromaRange;
    GrMipMapMode_t mmMode;      /* saved to allow MM en/dis */
    GrLOD_t        smallLod, largeLod; /* saved to allow MM en/dis */
    float   s_scale, t_scale;
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

  GrTexTable_t tex_table;
  
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
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
    struct {
      GrColor_t range;
      GrChromaRangeMode_t mode;
      GrChromaRangeMode_t match_mode;
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
  } grEnableArgs;
  struct{
    GrCoordinateSpaceMode_t coordinate_space_mode;
  } grCoordinateSpaceArgs;
#endif           /* GLIDE3 end grenable mode*/

  struct {
    FxBool fogTableSizeP;
  } cbEnvironment;
} GrState;

#if GLIDE_DISPATCH_SETUP
/* gpci.c 
 *
 * Set of procs for the current cpu type. These are selected out of
 * the _archXXXX proc list that is selected at grGlideInit time.
 */

typedef FxI32 (FX_CALL* GrTriSetupProc)(const void*, const void*, const void*);
typedef void (FX_CALL* GrDrawTextureLineProc)(const void*, const void*);
typedef void  (FX_CALL* GrVertexListProc)(FxU32 pkType, FxU32 type, FxI32 mode, FxI32 count, void* ptrs);
typedef void  (FX_CALL* GrDrawTrianglesProc)(FxI32 mode, FxI32 count, void* vPtrs);
typedef GrTriSetupProc GrTriSetupProcVector[2];
typedef GrTriSetupProcVector GrTriSetupProcArchVector[2];

/* Decalrations of the dispatchable procs found in xdraw2.asm and
 * xtexdl.c for teh triangle and texture download procs respectively.  
 */
extern FxI32 FX_CALL _trisetup_Default_Default(const void*, const void*, const void*);
extern FxI32 FX_CALL _trisetup_Default_cull(const void*, const void*, const void*);

/* Routine to call into the architecture specialized version of 
 * _grDrawTriangles if there is no specialized version of the
 * grDrawTriangle routine for clip coordinates.
 */
extern FxI32 FX_CALL _trisetup_clip_coor_thunk(const void*, const void*, const void*);

extern void FX_CALL _grDrawTriangles_Default(FxI32, FxI32, void*);

void FX_CSTYLE _drawvertexlist(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);
void FX_CSTYLE _vpdrawvertexlist(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);

#if GL_AMD3D
extern FxI32 FX_CALL _trisetup_3DNow_Default(const void*, const void*, const void*);
extern FxI32 FX_CALL _trisetup_3DNow_cull(const void*, const void*, const void*);

extern void FX_CALL _grDrawTriangles_3DNow(FxI32, FxI32, void*);
void FX_CSTYLE _grDrawVertexList_3DNow_Window(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);
void FX_CSTYLE _grDrawVertexList_3DNow_Clip(FxU32 pktype, FxU32 type, FxI32 mode, FxI32 count, void *pointers);
#endif /* GL_AMD3D */
#endif /* GLIDE_DISPATCH_SETUP */

#if GLIDE_DISPATCH_DOWNLOAD
/* _GlideRoot.curTexProcs is an array of (possibly specialized
 * function pointers indexed by texture format size (8/16 bits) and
 * texture line width (1/2/4/>4).  
 *
 * xtexdl.c
 */
struct GrGC_s;
typedef void  (FX_CALL* GrTexDownloadProc)(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                           const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                           void* texData);
typedef GrTexDownloadProc GrTexDownloadProcVector[2][4];

extern void FX_CALL _grTexDownload_Default_8_1(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                               const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                               void* texData);
extern void FX_CALL _grTexDownload_Default_8_2(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                               const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                               void* texData);
extern void FX_CALL _grTexDownload_Default_8_4(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                               const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                               void* texData);
extern void FX_CALL _grTexDownload_Default_8_WideS(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                                   const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                                   void* texData);

extern void FX_CALL _grTexDownload_Default_16_1(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                                const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                                void* texData);
extern void FX_CALL _grTexDownload_Default_16_2(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                                const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                                void* texData);
extern void FX_CALL _grTexDownload_Default_16_4(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                                const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                                void* texData);
extern void FX_CALL _grTexDownload_Default_16_WideS(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                                    const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                                    void* texData);

#if GL_AMD3D
/* xtexdl.asm */
extern void FX_CALL _grTexDownload_3DNow_MMX(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                             const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                             void* texData);
#endif /* GL_AMD3D */

#if GL_MMX
/* xtexdl.asm */
extern void FX_CALL _grTexDownload_MMX(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                                       const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                                       void* texData);
#endif /* GL_MMX */
#endif /* GLIDE_DISPATCH_DOWNLOAD */

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

#define kMaxVertexParam (20 + (12 * GLIDE_NUM_TMU) + 3)
#ifndef GLIDE3
  struct dataList_s {
    int      i;
    FxFloat* addr;
  } regDataList[kMaxVertexParam];
#endif
  int tsuDataList[kMaxVertexParam];
#ifdef GLIDE3_SCALER
  int tsuDataListScaler[kMaxVertexParam];
#endif

  GrState
    state;                      /* state of Glide/SST */

#if GLIDE_DISPATCH_SETUP || GLIDE_DISPATCH_DOWNLOAD
  struct {
#if GLIDE_DISPATCH_SETUP
    /* Current triangle rendering proc specialized for culling/no
     * culling and viewport/window coordinates.
     */
    GrTriSetupProc      triSetupProc;
    GrDrawTrianglesProc drawTrianglesProc;
    GrVertexListProc    drawVertexList;
    
    /* Vector to choose triangle rendering proc from based
     * on culling or no-cull this vector should be specialized
     * on viewport vs window coordinates.
     */
    GrTriSetupProcVector* coorTriSetupVector;
#endif /* GLIDE_DISPATCH_SETUP */
    
#if GLIDE_DISPATCH_DOWNLOAD
    /* Vector of texture download procs specialized by size
     * and processor vendor type.
     */
    GrTexDownloadProcVector* texDownloadProcs;
#endif /* GLIDE_DISPATCH_DOWNLOAD */
  } curArchProcs;
#endif /* GLIDE_DISPATCH_SETUP || GLIDE_DISPATCH_DOWNLOAD */

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
    FxU32  fifoJmpHdr;   /* Type0 packet for jmp to fifo start */
    
    FxU32* fifoPtr;      /* Current write pointer into fifo */
    unsigned long fifoRead;     /* Last known hw read ptr. 
                          * This is the sli master, if enabled.
                          */
    
    /* Fifo checking information. In units of usuable bytes until
     * the appropriate condition.
     */
    FxI32  fifoRoom;     /* Space until next fifo check */
    FxI32  roomToReadPtr;/* Bytes until last known hw ptr */
    FxI32  roomToEnd;    /* # of bytes until last usable address before fifoEnd */

    FxBool fifoLfbP;     /* Do we expect lfb writes to go through the fifo? */
    FxBool lfbLockCount; /* Have we done an lfb lock? Count of the locks. */
    
#if GLIDE_USE_SHADOW_FIFO
    FxU32* fifoShadowBase; /* Buffer that shadows the hw fifo for debugging */
    FxU32* fifoShadowPtr;
#endif /* GLIDE_USE_SHADOW_FIFO */
  } cmdTransportInfo;

  union hwDep_u {
    FxU32 noHwDep;

#if (GLIDE_PLATFORM & GLIDE_HW_H3)
    struct h3Dep_s {
      SstIORegs
        *ioRegs;                /* I/O remap regs */
      SstCRegs
        *cRegs;                 /* AGP/Cmd xfer/misc regs */
      SstGRegs
        *gRegs;                 /* 2D regs */
      SstRegs
        *sstRegs;               /* Graphics Regs (3D Regs) */
    } h3Dep;
#endif /* (GLIDE_PLATFORM & GLIDE_HW_H3) */

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
    struct cvgDep_s {
#if GLIDE_BLIT_CLEAR       
      /* xTilePages, yTileShift, tileSlopP, and numBufferPages are set
       * in grSstWinOpen, and should be considered logically const
       * until grSstWinClose at which point they are invalid.
       * 
       * NB: The calculation of these values etc should really be
       * moved into the init code at some point in my near future.  
       */
      FxU32 xTilePages;    /* # of pages for video tiles in the x direction,
                            * the x-dimension of a tile is alwasy 32 pixels.
                            * Cons-ed up from fbiInit1[24], fbiInit1[7:4], and
                            * fbiInit6[30] in grSstWinOpen after all of the
                            * buffers etc are allocated.
                            */

      FxBool tileSlopP;    /* Set if the actual # of video tiles in the x
                            * direction will not evenly fit into a page.
                            *
                            * FixMe: Will this ever happen?
                            */

      FxU32 yTileShift;    /* (0x01U << yTileShift) is the # of lines in a
                            * tile.  This is dependent on the sli-ness of the
                            * board. 
                            */

      FxU32 numBufferPages;/* The number of pages used for an entire
                            * buffer (color or aux).
                            */
#endif /* GLIDE_BLIT_CLEAR */

      FxU32 renderBuf;  /* Cached value of the current buffer swapped by the
                         * user via grBufferSwap. Legal values are
                         * [0 .. gc->state.num_buffers - 1]. 
                         *
                         * NB: We need this because the current buffer
                         * availible in the status register lags the actual
                         * value due to the command fifo asynchrony (is that
                         * a word?). 
                         */

      FxU32 frontBuf;
      FxU32 backBuf;

      /* CVG cannot really do single buffering */
      FxBool singleBufferP;

      /* Sli has an 'interesting' feature where the physical scanlines
       * that are being rendered is dependent on the location of the y
       * origin. There is some ugliness now in grSstOrigin and grSwapBuffer
       * to deal w/ this correctly.
       *
       * Origin_Lower_Left: 0:Black [1 .. screenRezY]:Rendered screenRez+1:Black
       * Origin_Upper_Left: [0 .. screenRezY - 1]:Rendered [screenRez-screenRez+1]:Black
       */
      FxU32 sliOriginBufCount;
    } cvgDep;
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) */
  } hwDep;

  /* lfb config */
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
    grAuxBuf;

  /* sli config */
  FxBool sliPairP;              /* Part of an sli pair? */
  FxBool scanline_interleaved;  /* Enable sli for this pair */
  FxBool swapMasterSenseP;      /* Swapped master and slave pointers */

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
  unsigned long checkPtr;
   
  FxVideoTimingInfo* vidTimings;/* init code overrides */

  FxBool open;                  /* Has GC Been Opened? */
  FxBool hwInitP;               /* Has the hw associated w/ GC been initted and mapped?
                                 * This is managed in _grDetectResources:gpci.c the first 
                                 * time that the board is detected, and in grSstWinOpen:gsst.c
                                 * if the hw has been shutdown in a call to grSstWinClose.
                                 */
  /* Oem Dll data */
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  void *oemInit;
  OemInitInfo oemi;
#endif
} GrGC;

/* NOTE: this changes the P6FENCE macro expansion from sst1init.h !!! */
#define p6FenceVar _GlideRoot.p6Fencer

/* if we are debugging, call a routine so we can trace fences */
#ifdef GLIDE_DEBUG
#define GR_P6FENCE _grFence();
#else
#define GR_P6FENCE P6FENCE
#endif

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
  _p_info CPUType;              /* CPUID */
#endif
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
    /* temps to convert floats to ints */
    union { float f; FxI32 i; FxU32 u; } temp1;
    union { float f; FxI32 i; FxU32 u; } temp2;
    float  fIntTruncBias;

#if GLIDE_PACKED_RGB
#define kPackBiasA _GlideRoot.pool.fBiasHi
#define kPackBiasR _GlideRoot.pool.fBiasHi
#define kPackBiasG _GlideRoot.pool.fBiasHi
#define kPackBiasB _GlideRoot.pool.fBiasLo

#define kPackShiftA 16U
#define kPackShiftR 8U
#define kPackShiftG 0U
#define kPackShiftB 0U

#define kPackMaskA  0x00FF00U
#define kPackMaskR  0x00FF00U
#define kPackMaskG  0x00FF00U
#define kPackMaskB  0x00FFU

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
    FxI32  swapPendingCount;    /* pending buffer swap count    */

    /* Force alternate buffer strategy */
    FxI32  nColorBuffer;
    FxI32  nAuxBuffer;

  } environment;

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

  GrHwConfiguration     hwConfig;

  FxU32                 gcNum;                  /* # of actual boards mapped */
  FxU32                 gcMap[MAX_NUM_SST];     /* Logical mapping between selectable
                                                 * sst's and actual boards.
                                                 */
  GrGC                  GCs[MAX_NUM_SST];       /* one GC per board     */

#if GLIDE_DISPATCH_SETUP || GLIDE_DISPATCH_DOWNLOAD
  struct {
#if GLIDE_DISPATCH_SETUP
    GrTriSetupProcArchVector* curTriProcs;
    GrDrawTrianglesProc       curDrawTrisProc;
    GrVertexListProc*         curVertexListProcs;
    GrDrawTextureLineProc     curLineProc;
  
#define PROC_SELECT_TRISETUP(__procVector, __cullMode) (__procVector)[(__cullMode) != GR_CULL_DISABLE]
#endif /* GLIDE_DISPATCH_SETUP */
#if GLIDE_DISPATCH_DOWNLOAD
    GrTexDownloadProcVector*  curTexProcs;
#define PROC_SELECT_TEXDOWNLOAD()                      _GlideRoot.curTexProcs
#endif /* GLIDE_DISPATCH_DOWNLOAD */
  } deviceArchProcs;
#endif /* GLIDE_DISPATCH_SETUP || GLIDE_DISPATCH_DOWNLOAD */
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
   FX_EXPORT type FX_CSTYLE name args

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
#else 
#define GLIDE_DRIVER_NAME "HOOPTI???"
#endif

/*==========================================================================*/
#ifndef FX_GLIDE_NO_FUNC_PROTO

void _grMipMapInit(void);

FxI32 FX_CSTYLE
_vptrisetup_nocull(const void *va, const void *vb, const void *vc);

FxI32 FX_CSTYLE
_vptrisetup_cull(const void *va, const void *vb, const void *vc);

#if GLIDE_DISPATCH_SETUP
#define TRISETUP (*gc->curArchProcs.triSetupProc)
#else /* !GLIDE_DISPATCH_SETUP */
FxI32 FX_CSTYLE
_trisetup(const void *va, const void *vb, const void *vc);
FxI32 FX_CSTYLE
_trisetup_cull(const void *va, const void *vb, const void *vc);
FxI32 FX_CSTYLE
_trisetup_nogradients(const void *va, const void *vb, const void *vc);

/* GMT: BUG need to make this dynamically switchable
   BULLSHIT:  That is not a bug.  It is an opinion!
 */
#if GLIDE_USE_C_TRISETUP
#  if (GLIDE_PLATFORM & GLIDE_HW_CVG) && USE_PACKET_FIFO
#    define TRISETUP _trisetup_nogradients
#  else /* !((GLIDE_PLATFORM & GLIDE_HW_CVG) && USE_PACKET_FIFO) */
#    define TRISETUP _trisetup_nogradients
#  endif /* !((GLIDE_PLATFORM & GLIDE_HW_CVG) && USE_PACKET_FIFO) */
#else /* !GLIDE_USE_C_TRISETUP */
#  define TRISETUP _trisetup_cull
#endif /* !GLIDE_USE_C_TRISETUP */
#endif /* !GLIDE_DISPATCH_SETUP */

#ifdef GLIDE3
void
_grValidateState();

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

/*
** Glide 3 extension APIs
*/
void FX_CALL
grStipplePattern(GrStipplePattern_t stipple);

void FX_CALL 
grChromaRangeMode(GrChromaRangeMode_t mode);

void FX_CALL 
grChromaRange( GrColor_t min, GrColor_t max , GrChromaRangeMode_t mode);

void FX_CALL 
grTexChromaMode(GrChipID_t tmu, GrTexChromakeyMode_t mode);

void FX_CALL 
grTexChromaRange(GrChipID_t tmu, GrColor_t min, GrColor_t max, GrTexChromakeyMode_t mode);

/* gaa.c */
void FX_CALL 
_grDrawTextureLine_Default(const void *a, const void *b);

#if GL_AMD3D
/* xdraw3.asm */
void FX_CALL 
_grDrawTextureLine_3DNow(const void *a, const void *b);
#endif /* GL_AMD3D */

void FX_CALL 
guQueryResolutionXY(GrScreenResolution_t res, FxU32 *x, FxU32 *y);

/*
** state routine
*/
void
_grChromaRange( GrColor_t max , GrChromaRangeMode_t mode);

void
_grChromaMode( GrChromaRangeMode_t mode );

void
_grChromakeyMode( GrChromakeyMode_t mode );

void
_grChromakeyValue( GrColor_t color );

void
_grDepthMask( FxBool mask );

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

#endif

#ifdef GLIDE3      /* glide 3 m point, m aa point, m line, m aa line routine */

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

#endif

#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
void FX_CSTYLE
_guTexMemReset(void);

int FX_CSTYLE
_grBufferNumPending(void);

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

#endif

#endif /* FX_GLIDE_NO_FUNC_PROTO */

/*==========================================================================*/
/* 
**  Function Prototypes
*/
#ifdef GLIDE_DEBUG
FxBool
_grCanSupportDepthBuffer(void);
#endif

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
_grTexCalcBaseAddress(
                      FxU32 start_address, GrLOD_t largeLod,
                      GrAspectRatio_t aspect, GrTextureFormat_t fmt,
                      FxU32 odd_even_mask); 

void
_grTexForceLod(GrChipID_t tmu, int value);

FxU32
_grTexTextureMemRequired(GrLOD_t small_lod, GrLOD_t large_lod, 
                           GrAspectRatio_t aspect, GrTextureFormat_t format,
                           FxU32 evenOdd);
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
                  FxI32 src_stride, void *src_data);

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

typedef struct cmdTransportInfo GrCmdTransportInfo;
extern void
_grGetCommandTransportInfo(GrCmdTransportInfo*);
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

#if USE_PACKET_FIFO
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
#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
#define HW_FIFO_PTR(__masterP)\
((unsigned long)gc->cmdTransportInfo.fifoStart + \
 (GET(((SstRegs*)((__masterP) \
                  ? gc->reg_ptr \
                  : gc->slave_ptr))->cmdFifoReadPtr) - \
  gc->cmdTransportInfo.fifoOffset))
#elif (GLIDE_PLATFORM & GLIDE_HW_H3)
#  define HW_FIFO_PTR(__masterP) \
  ((unsigned long)gc->cmdTransportInfo.fifoStart +\
   (GET(((SstCRegs*)(gc->hwDep.h3Dep.cRegs))->cmdFifo0.readPtrL)) - \
   gc->cmdTransportInfo.fifoOffset)
#else
#  error "Define HW_FIFO_PTR for this hardware!"
#endif

#if FIFO_ASSERT_FULL
extern const FxU32 kFifoCheckMask;
extern FxU32 gFifoCheckCount;

#define FIFO_ASSERT() \
if ((gFifoCheckCount++ & kFifoCheckMask) == 0) { \
   const FxU32 cmdFifoDepth = GR_GET(((SstRegs*)(gc->reg_ptr))->cmdFifoDepth); \
   const FxU32 maxFifoDepth = ((gc->cmdTransportInfo.fifoSize - FIFO_END_ADJUST) >> 2); \
   if(cmdFifoDepth > maxFifoDepth) { \
     gdbg_printf(__FILE__"(%ld): cmdFifoDepth > size: 0x%X : 0x%X : (0x%lX : 0x%lX)\n", \
                 __LINE__, cmdFifoDepth, maxFifoDepth, \
                 HW_FIFO_PTR(FXTRUE), (unsigned long)gc->cmdTransportInfo.fifoPtr); \
     ASSERT_FAULT_IMMED(cmdFifoDepth <= maxFifoDepth); \
   } else if (cmdFifoDepth + (gc->cmdTransportInfo.fifoRoom >> 2) > maxFifoDepth) { \
     gdbg_printf(__FILE__"(%ld): cmdFifoDepth + fifoRoom > size: (0x%X : 0x%X) : 0x%X\n", \
                 __LINE__, cmdFifoDepth, (gc->cmdTransportInfo.fifoRoom >> 2), maxFifoDepth); \
     ASSERT_FAULT_IMMED(cmdFifoDepth + (gc->cmdTransportInfo.fifoRoom >> 2) <= maxFifoDepth); \
   } \
} \
ASSERT_FAULT_IMMED(HW_FIFO_PTR(FXTRUE) >= (unsigned long)gc->cmdTransportInfo.fifoStart); \
ASSERT_FAULT_IMMED(HW_FIFO_PTR(FXTRUE) < (unsigned long)gc->cmdTransportInfo.fifoEnd); \
ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoRoom < gc->cmdTransportInfo.fifoSize); \
ASSERT_FAULT_IMMED((unsigned long)gc->cmdTransportInfo.fifoPtr < (unsigned long)gc->cmdTransportInfo.fifoEnd)
#else /* !FIFO_ASSERT_FULL */
#define FIFO_ASSERT() \
ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoRoom < gc->cmdTransportInfo.fifoSize); \
ASSERT_FAULT_IMMED((unsigned long)gc->cmdTransportInfo.fifoPtr < (unsigned long)gc->cmdTransportInfo.fifoEnd)
#endif /* !FIFO_ASSERT_FULL */

void GR_CDECL
_FifoMakeRoom(const FxI32 blockSize, const char* fName, const int fLine);

#define GR_CHECK_FOR_ROOM(__n, __p) \
do { \
  const FxU32 writeSize = (__n) + ((__p) * sizeof(FxU32));            /* Adjust for size of hdrs */ \
  ASSERT(((unsigned long)(gc->cmdTransportInfo.fifoPtr) & FIFO_ALIGN_MASK) == 0); /* alignment */ \
  ASSERT(writeSize < gc->cmdTransportInfo.fifoSize - sizeof(FxU32)); \
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

#if GLIDE_SANITY_SIZE
#if USE_PACKET_FIFO

#if GLIDE_USE_SHADOW_FIFO
#define GR_CHECK_SHADOW_FIFO \
  if ((gc != NULL) && (gc->cmdTransportInfo.fifoShadowPtr != NULL)) \
  ASSERT_FAULT_IMMED((((unsigned long)gc->cmdTransportInfo.fifoPtr) & (kDebugFifoSize - 1)) == \
                     (((unsigned long)gc->cmdTransportInfo.fifoShadowPtr) & (kDebugFifoSize - 1)))
#else /* !GLIDE_USE_SHADOW_FIFO */
#define GR_CHECK_SHADOW_FIFO
#endif /* !GLIDE_USE_SHADOW_FIFO */

#define GR_CHECK_FIFO_PTR() \
  if((unsigned long)gc->cmdTransportInfo.fifoPtr != (gc->checkPtr + gc->checkCounter)) \
     GDBG_ERROR("GR_ASSERT_FIFO", "(%s : %d) : " \
                "fifoPtr should be 0x%lX (0x%lX : 0x%X) but is 0x%lX\n", \
                __FILE__, __LINE__, \
                gc->checkPtr + gc->checkCounter, gc->checkPtr, gc->checkCounter, \
                (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  GR_CHECK_SHADOW_FIFO; \
  ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoPtr == gc->checkPtr + gc->checkCounter)
#define GR_SET_FIFO_PTR(__n, __p) \
  gc->checkPtr = (unsigned long)gc->cmdTransportInfo.fifoPtr; \
  gc->checkCounter = ((__n) + ((__p) << 2))
#else
#define GR_CHECK_FIFO_PTR() 
#define GR_SET_FIFO_PTR(__n, __p)
#endif

#define GR_CHECK_SIZE() \
  if(gc->counter != gc->expected_counter) \
    GDBG_ERROR("GR_ASSERT_SIZE","byte counter should be %d but is %d\n", \
               gc->expected_counter,gc->counter); \
  GR_CHECK_FIFO_PTR(); \
  gc->checkPtr = (unsigned long)gc->cmdTransportInfo.fifoPtr; \
  gc->checkCounter = 0; \
  ASSERT(gc->counter == gc->expected_counter); \
  gc->counter = gc->expected_counter = 0

#define GR_SET_EXPECTED_SIZE(n,p) \
  ASSERT(gc->counter == 0); \
  ASSERT(gc->expected_counter == 0); \
  GR_CHECK_FOR_ROOM(n,p); \
  gc->expected_counter = n; \
  GR_SET_FIFO_PTR(n, p)

#define GR_INC_SIZE(n) gc->counter += n
#else
  /* define to do nothing */
  #define GR_CHECK_SIZE()
  #define GR_SET_EXPECTED_SIZE(n,p) GR_CHECK_FOR_ROOM(n,p)
  #define GR_INC_SIZE(n)
#endif

#define GR_DCL_GC GrGC *gc = _GlideRoot.curGC
#define GR_DCL_HW SstRegs *hw = (SstRegs *)gc->reg_ptr

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
                gc->checkPtr = (unsigned long)gc->cmdTransportInfo.fifoPtr; \
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

#if USE_PACKET_FIFO
#ifdef GDBG_INFO_ON
void _grFifoWriteDebug(FxU32 addr, FxU32 val, unsigned long fifoPtr);
#define DEBUGFIFOWRITE(a,b,c) \
_grFifoWriteDebug((FxU32) (a), (FxU32) (b), (unsigned long) (c))
void _grFifoFWriteDebug(FxU32 addr, float val, unsigned long fifoPtr);
#define DEBUGFIFOFWRITE(a,b,c) \
_grFifoFWriteDebug((FxU32) (a), (float) (b), (unsigned long) (c))
#else /* ~GDBG_INFO_ON */
#define DEBUGFIFOWRITE(a,b,c)
#define DEBUGFIFOFWRITE(a,b,c)
#endif /* !GDBG_INFO_ON */
#endif /* USE_PACKET_FIFO */

#if USE_PACKET_FIFO && GLIDE_USE_SHADOW_FIFO 
#undef SET
#define SET(d, s) \
do { \
  GR_DCL_GC; \
  GR_DCL_HW; \
  volatile FxU32* __u32P = (volatile FxU32*)&(d); \
  const FxU32 __u32Val = (s); \
  if ((__u32P != &hw->swapbufferCMD) && (gc->cmdTransportInfo.fifoShadowPtr != NULL)) { \
    *gc->cmdTransportInfo.fifoShadowPtr++ = __u32Val; \
  } \
  *__u32P = __u32Val; \
} while(0)

#undef SETF
#define SETF(d, s) \
do { \
  volatile float* __floatP = (volatile float*)(&(d)); \
  const float __floatVal = (s); \
  GR_DCL_GC; \
  if (gc->cmdTransportInfo.fifoShadowPtr != NULL) { \
    *(float*)gc->cmdTransportInfo.fifoShadowPtr = __floatVal; \
    gc->cmdTransportInfo.fifoShadowPtr++; \
  } \
  *__floatP = __floatVal; \
} while(0)

#undef SET16
#define SET16(d, s) SET(d, (FxU32)s)
#endif /* USE_PACKET_FIFO && GLIDE_USE_DEBUG_FIFO  */

#if SET_BSWAP
#undef GET
#undef GET16
#undef SET
#undef SET16
#undef SETF

#if __POWERPC__ && defined(__MWERKS__)
#define GET(s)               __lwbrx( (void*)&(s), 0 )
#define GET16(s)             __lwbrx( (void*)&(s), 0 )
#define SET(d, s)            __stwbrx((s), (void*)&(d), 0)
#define SET16(d, s)          __sthbrx((s), (void*)&(d), 0 )
#define SETF(d, s) \
                             { \
                                const float temp = (s); \
                                __stwbrx( *((FxU32*)&temp), (void*)&(d), 0 ); \
                             }
#define SET_LINEAR(d, s)     SET((d), (s))
#define SET_LINEAR_16(d, s)  SET((d), ((((FxU32)(s)) >> 16U) | \
                                       (((FxU32)(s)) << 16U)))
#define SET_LINEAR_8(d, s)   ((d) = (s))
#else /* !defined(__MWERKS__) && POWERPC */
#error "Define byte swapped macros for GET/SET"
#endif /* !defined(__MWERKS__) && POWERPC */
#endif /* SET_BSWAP */

#if GLIDE_USE_DEBUG_FIFO
#define kDebugFifoSize 0x1000U
#endif /* GLIDE_USE_DEBUG_FIFO */

#ifndef SET_LINEAR
#define SET_LINEAR(__addr, __val)    SET(__addr, __val)
#define SET_LINEAR_16(__addr, __val) SET(__addr, __val)
#define SET_LINEAR_8(__addr, __val)  SET(__addr, __val)
#endif /* !defined(SET_LINEAR) */

/* Extract the fp exponent from a floating point value.
 * NB: The value passed to this macro must be convertable
 * into an l-value.
 */
#define kFPExpMask        0x7F800000U
#define kFPZeroMask       0x80000000U
#define kFPExpShift       0x17U
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
#define MaskSelect(__b, __val) (~(((FxU32)(__b)) - 1U) & (__val))

/* Chipfield ids that glide uses. */
#define kChipFieldShift (8U + 3U)
typedef enum {
  eChipBroadcast    = 0x00U,
  eChipFBI          = 0x01U,
  eChipTMU0         = 0x02U,
  eChipTMU1         = 0x04U,
  eChipTMU2         = 0x08U,
  eChipAltBroadcast = 0x0FU,
} FifoChipField;

#if GLIDE_CHIP_BROADCAST && (GLIDE_PLATFORM & GLIDE_HW_CVG)
#define BROADCAST_ID eChipAltBroadcast
#else
#define BROADCAST_ID eChipBroadcast
#endif

/* Although these are named reg_group_xxx they are generic options for
 * grouping register writes and should be fine w/ and w/o the fifo
 * being enabled.  
 */
#if GDBG_INFO_ON
#define REG_GROUP_DCL(__regMask, __regBase, __groupNum, __checkP) \
const FxBool _checkP = (__checkP); \
const FxU32 _regMask = (__regMask); \
const FxU32 _groupNum = (__groupNum); \
FxU32 _regCheckMask = (__regMask); \
FxU32 _regBase = offsetof(SstRegs, __regBase)

#define REG_GROUP_ASSERT(__regAddr, __val, __floatP) \
{ \
  const FxU32 curRegAddr = offsetof(SstRegs, __regAddr); \
  const FxU32 curRegIndex = (curRegAddr - _regBase) >> 2; \
  const FxU32 curRegBit = (0x01U << curRegIndex); \
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
  GR_ASSERT(((0xFFFFFFFFU >> (32 - curRegIndex)) & _regCheckMask) == 0x00); /* All previous regs done */ \
  _regCheckMask ^= curRegBit;                                                /* Mark current reg */ \
}
#else /* !GDBG_INFO_ON */
#define REG_GROUP_DCL(__regMask, __regBase, __groupNum, __checkP) 
#define REG_GROUP_ASSERT(__regAddr, __val, __floatP)
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
#define GR_CULL_MASK 0xff7fffff
#endif /* GLIDE_HW_TRI_SETUP */

#if USE_PACKET_FIFO

/* CVG has a problem when using the chipfield to address multiple
 * tmu's and using the tsu which does not send things to different
 * tmu's. We work around this by using broadcast 0xF rather than 0x0
 * in the chipfield. This macro should build a compile-time constant
 * bit value that can be or-ed w/ any dynamic data.
 */
#define FIFO_REG(__chipField, __field) \
 ((((FxU32)offsetof(SstRegs, __field)) << 1) | \
  (((FxU32)(__chipField)) << kChipFieldShift))

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

#define REG_GROUP_BEGIN_INTERNAL(__chipId, __regBase, __groupNum, __groupMask, __pktHdr, __checkP) \
{ \
  GR_DCL_GC; \
  volatile FxU32* _regGroupFifoPtr = gc->cmdTransportInfo.fifoPtr; \
  REG_GROUP_DCL(__groupMask, __regBase, __groupNum, __checkP); \
  GR_ASSERT(((__pktHdr) & 0xE0000000U) == 0x00U); \
  FIFO_ASSERT(); \
  GDBG_INFO(120, "REG_GROUP_BEGIN: (0x%X : 0x%X) : (0x%X - 0x%X : 0x%X) : (0x%lX : 0x%X)\n", \
            (__pktHdr), (__groupMask), \
            FIFO_REG(__chipId, __regBase), __chipId, offsetof(SstRegs, __regBase), \
            (unsigned long)gc->cmdTransportInfo.fifoPtr, gc->cmdTransportInfo.fifoRoom); \
  SET(*_regGroupFifoPtr++, (__pktHdr))

#define REG_GROUP_SET(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT(__regAddr, __val, FXFALSE); \
  FXUNUSED(__regBase); \
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
  ASSERT((((unsigned long)_regGroupFifoPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr) >> 2) == _groupNum + 1); \
  gc->cmdTransportInfo.fifoRoom -= ((unsigned long)_regGroupFifoPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = (FxU32*)_regGroupFifoPtr; \
  FIFO_ASSERT(); \
}

#define REG_GROUP_END() \
  ASSERT(_checkP); \
  ASSERT((((unsigned long)_regGroupFifoPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr) >> 2) == _groupNum + 1); \
  gc->cmdTransportInfo.fifoRoom -= ((unsigned long)_regGroupFifoPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = (FxU32*)_regGroupFifoPtr; \
  GDBG_INFO(gc->myLevel + 200, "\tGroupEnd: (0x%lX : 0x%X) : (0x%lX : 0x%lX)\n", \
            (unsigned long)_regGroupFifoPtr, gc->cmdTransportInfo.fifoRoom, \
            HW_FIFO_PTR(FXTRUE), (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  FIFO_ASSERT(); \
} \
GR_CHECK_SIZE()

#if !GLIDE_HW_TRI_SETUP || HOOPTI_TRI_SETUP_COMPARE
/* Send all of the triangle parameters in a single cmd fifo packet to
 * the chip until the tsu is fixed.
 */
#define kNumTriParam 0x1FU
   
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
   gc->cmdTransportInfo.fifoRoom -= ((unsigned long)noTsuFifoPtr - \
                                 (unsigned long)gc->cmdTransportInfo.fifoPtr); \
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
_grCVGFifoDump_TriHdr(const FxU32 triPacketHdr);
extern void
_grCVGFifoDump_Linear(const FxU32* const linearPacketAddr);

#define DEBUGFIFODUMP_TRI(__packetAddr)    _grCVGFifoDump_TriHdr(__packetAddr)
#define DEBUGFIFODUMP_LINEAR(__packetAddr) _grCVGFifoDump_Linear(__packetAddr)

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
  GR_ASSERT(((unsigned long)(tPackPtr) & FIFO_ALIGN_MASK) == 0);   /* alignment */ \
  GR_ASSERT((((__nVerts) * (__vertSize)) + sizeof(FxU32)) <= (FxU32)gc->cmdTransportInfo.fifoRoom); \
  GR_ASSERT((((unsigned long)tPackPtr) + ((__nVerts) * (__vertSize)) + sizeof(FxU32)) < \
            (unsigned long)gc->cmdTransportInfo.fifoEnd); \
  GR_ASSERT(nVertex < 0x10); \
  GR_ASSERT(nVertex > 0x00); \
  GR_ASSERT(((__packetHdr) & 0xE0000000U) == 0x00U); \
  FIFO_ASSERT(); \
  GDBG_INFO(120, "Triangle(0x%X): (0x%X : 0x%X)\n", (__packetHdr), __nVerts, __vertSize); \
  DEBUGFIFODUMP_TRI(__packetHdr)
#define CLAMP_DUMP(__val, __floatVal) \
  pCount++; \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%lX) : V#: 0x%lX - P#: 0x%lX - ParamVal: (%f : 0x%X)\n", \
            (unsigned long)tPackPtr, \
            ((unsigned long)tPackPtr - ((unsigned long)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) / sVertex, \
             (((unsigned long)tPackPtr - ((unsigned long)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) % sVertex) >> 2, \
            (((__val) < 786432.875) ? (__val) : ((__val) - 786432.875)), \
            (__floatVal))
#define SETF_DUMP(__val) \
  pCount++; \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%lX) : V#: 0x%lX - P#: 0x%lX - ParamVal: %f\n", \
            (unsigned long)tPackPtr, \
            ((unsigned long)tPackPtr - ((unsigned long)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) / sVertex, \
             (((unsigned long)tPackPtr - ((unsigned long)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) % sVertex) >> 2, \
            (((__val) < 786432.875) ? (__val) : ((__val) - 786432.875)))
#define SET_DUMP(__val) \
  pCount++; \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%lX) : V#: 0x%lX - P#: 0x%lX - ParamVal: 0x%X\n", \
            (unsigned long)tPackPtr, \
            ((unsigned long)tPackPtr - ((unsigned long)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) / sVertex, \
             (((unsigned long)tPackPtr - ((unsigned long)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) % sVertex) >> 2, \
            (__val))
#define TRI_ASSERT() \
  GR_ASSERT(pCount == (nVertex * (sVertex >> 2))); \
  ASSERT(((unsigned long)tPackPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr) == (nVertex * sVertex) + sizeof(FxU32))
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
((_GlideRoot.pool.temp1.f = (float)((float)(__fpVal) + (float)(__fpBias))), \
 GR_ASSERT((__fpVal) >= 0.0f), \
 GR_ASSERT((__fpVal) < 256.0f), \
 ((_GlideRoot.pool.temp1.u & (__fpMask)) << (__fpShift)))
                                                  
#define RGBA_COMP_CLAMP(__fpVal, __compToken) \
   RGBA_COMP(__fpVal, kPackBias##__compToken, kPackShift##__compToken, kPackMask##__compToken)
#endif /* GLIDE_PACKED_RGB */

/* First stage tsu-subtractor chec/fix. 
 * Mmm..... sequence operator.
 */
#if GLIDE_FP_CLAMP
#define kFPClampThreshold 0x20U
#define FP_FLOAT_CLAMP(__fpVal) ((FP_FLOAT_EXP(__fpVal) < kFPClampThreshold) \
                                 ? (_GlideRoot.stats.tsuValClamp++, 0x00U) \
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
  gc->cmdTransportInfo.fifoRoom -= ((unsigned long)tPackPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = tPackPtr; \
  GDBG_INFO(gc->myLevel + 200, "\tTriEnd: (0x%lX : 0x%X)\n", (unsigned long)tPackPtr, gc->cmdTransportInfo.fifoRoom); \
  FIFO_ASSERT(); \
}

#define FIFO_LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN, __f, __l) \
{ \
  FxU32* packetPtr = gc->cmdTransportInfo.fifoPtr; \
  const FxU32 __writeSize = (__numWords);       /* Add size of packet header */ \
  const FxU32 hdr1 = ((__type) | \
                      (((FxU32)(__maskW2)) << SSTCP_PKT5_BYTEN_W2_SHIFT) | \
                      (((FxU32)(__maskWN)) << SSTCP_PKT5_BYTEN_WN_SHIFT) | \
                      (__writeSize << SSTCP_PKT5_NWORDS_SHIFT) | \
                      SSTCP_PKT5); \
  const FxU32 hdr2 = ((FxU32)((unsigned long)(__addr))) & SSTCP_PKT5_BASEADDR; \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         !gc->open, \
                         "Called before grSstWinOpen()"); \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         (gc->cmdTransportInfo.lfbLockCount != 0), \
                         "Called within grLfbLock/grLfbUnlockPair"); \
  GR_ASSERT(((unsigned long)(packetPtr) & FIFO_ALIGN_MASK) == 0);        /* alignment */ \
  GR_ASSERT((__numWords) > 0);                                   /* packet size */ \
  GR_ASSERT((__numWords) < ((0x01 << 19) - 2)); \
  GR_ASSERT((((__numWords) + 2) << 2) <= (FxU32)gc->cmdTransportInfo.fifoRoom); \
  GR_ASSERT(((unsigned long)packetPtr + (((__numWords) + 2) << 2)) < \
            (unsigned long)gc->cmdTransportInfo.fifoEnd); \
  GR_ASSERT((hdr2 & 0xE0000000U) == 0x00U); \
  GR_ASSERT((((FxU32)(__type)) >= ((FxU32)kLinearWriteLFB)) &&   /* packet type */ \
            (((FxU32)(__type)) <= ((FxU32)kLinearWriteTex))); \
  FIFO_ASSERT(); \
  GDBG_INFO(120, "LinearWrite(0x%X : 0x%X)\n", hdr1, hdr2); \
  GDBG_INFO(gc->myLevel + 200, "\tFile: %s - Line: %ld\n", __f, __l); \
  GDBG_INFO(gc->myLevel + 200, "\tType: 0x%X\n", (FxU32)(__type)); \
  GDBG_INFO(gc->myLevel + 200, "\tAddr: 0x%X\n", (FxU32)((unsigned long)(__addr))); \
  GDBG_INFO(gc->myLevel + 200, "\tMaskW2: 0x%X\n", (FxU32)(__maskW2)); \
  GDBG_INFO(gc->myLevel + 200, "\tMaskWN: 0x%X\n", (FxU32)(__maskWN)); \
  GDBG_INFO(gc->myLevel + 200, "\twriteSize: 0x%X\n", __writeSize); \
  GDBG_INFO(gc->myLevel + 200, "\thdr 1: 0x%X\n", hdr1); \
  GDBG_INFO(gc->myLevel + 200, "\thdr 2: 0x%X\n", hdr2); \
  SET(*packetPtr++, hdr1); \
  SET(*packetPtr++, hdr2); \
  GR_INC_SIZE(sizeof(FxU32))

#define FIFO_LINEAR_WRITE_SET(__val) \
do { \
  GDBG_INFO(gc->myLevel + 205, "\t0x%lX : 0x%X\n", (unsigned long)packetPtr, (__val)); \
  SET_LINEAR(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
  
#define FIFO_LINEAR_WRITE_SET_16(__val) \
do { \
  GDBG_INFO(gc->myLevel + 205, "\t0x%lX : 0x%X\n", (unsigned long)packetPtr, (__val)); \
  SET_LINEAR_16(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
  
#define FIFO_LINEAR_WRITE_SET_8(__val) \
do { \
  GDBG_INFO(gc->myLevel + 205, "\t0x%lX : 0x%X\n", (unsigned long)packetPtr, (__val)); \
  SET_LINEAR_8(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define FIFO_LINEAR_WRITE_END \
  DEBUGFIFODUMP_LINEAR(gc->cmdTransportInfo.fifoPtr); \
  GR_ASSERT((((unsigned long)packetPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr) >> 2) == __writeSize + 2); \
  gc->cmdTransportInfo.fifoRoom -= ((unsigned long)packetPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = packetPtr; \
  GDBG_INFO(gc->myLevel + 200, "\tLinearEnd: (0x%lX : 0x%X)\n", \
            (unsigned long)packetPtr, gc->cmdTransportInfo.fifoRoom); \
  FIFO_ASSERT(); \
}

#  define GR_GET(s)                 GET(s)
#  define GR_GET16(s)               ((FxU16)GET16(s))
#  define GR_SET(c, h, f, s)        STORE_FIFO(c, h, f, s)
#  define GR_SET_INDEX(c, h, r, s)  STORE_FIFO_INDEX(c, h, r, s)
#  define GR_SET16(c, h, f, s)      STORE16_FIFO(c, h, f, s)
#  define GR_SETF(c, h, f, s)       STOREF_FIFO(c, h, f, s)
#  define GR_SETF_INDEX(c, h, r, s) STOREF_FIFO_INDEX(c, h, r, s)
#else /* !USE_PACKET_FIFO */
#  define GR_GET(s)                 GET(s)
#  define GR_GET16(s)               ((FxU16)GET16(s))
#  define GR_SET(c, h, f, s)        do {SET((h)->f, s); GR_INC_SIZE(4);} while(0)
#  define GR_SET_INDEX(c, h, r, s)  do {SET(((FxU32*)(h))[r], s); GR_INC_SIZE(sizeof(FxU32));} while(0)
#  define GR_SETF(c, h, f, s)       do {SETF(h->f, s); GR_INC_SIZE(4);} while(0)
#  define GR_SETF_INDEX(c, h, r, s) do {SETF(((FxU32*)(h))[r], s); GR_INC_SIZE(sizeof(FxU32));} while(0)
#  define GR_SET16(c, h, f, s)      do {SET16((h)->f, s); GR_INC_SIZE(2);} while(0)
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

enum {
  kLinearWriteLFB   = SSTCP_PKT5_3DLFB,
  kLinearWriteTex   = SSTCP_PKT5_TEXPORT
};

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
#define TEX_ROW_ADDR_INCR(__t) ((__t) << 9)
#else
#error "Need to define TEX_ROW_ADDR_INCR for this hw."
#endif

#if USE_PACKET_FIFO

#define LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN) \
{ \
   GR_SET_EXPECTED_SIZE(((FxU32)((__numWords) + 1U) << 2U), 1); \
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
 */
#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
#define FIFO_LINEAR_EDGE_MASK_ADJUST(__mask) ((~(__mask)) & 0x0FU)
#define FIFO_LINEAR_EDGE_SET(__val) FIFO_LINEAR_WRITE_SET((((__val) & 0xFFFF0000U) >> 16U) | \
                                                          (((__val) & 0x0000FFFFU) << 16U))
#else
#define FIFO_LINEAR_EDGE_SET(__val) FIFO_LINEAR_WRITE_SET(__val)
#define FIFO_LINEAR_EDGE_MASK_ADJUST(__mask) (__mask)
#endif

#define LINEAR_WRITE_EDGE(__type, __addr, __val, __valBytes) \
do { \
  const FxU32 edgeAddr = (FxU32)(((FxU32)((unsigned long)(__addr))) & 0x03U); \
  GR_ASSERT((__valBytes) <= sizeof(FxU32)); \
  GR_ASSERT((((FxU32)(__addr)) + (__valBytes)) <= ((((FxU32)((unsigned long)(__addr))) & ~0x03U) + sizeof(FxU32))); \
  LINEAR_WRITE_BEGIN(1, __type, ((FxU32)((unsigned long)(__addr)) & ~0x03U), \
                     FIFO_LINEAR_EDGE_MASK_ADJUST((0xF0U | (0x0FU >> (__valBytes))) >> edgeAddr), \
                     0x00); \
  FIFO_LINEAR_EDGE_SET(((FxU32)(__val)) << (((sizeof(FxU32) - edgeAddr) << 3U) - \
                                             ((__valBytes) << 3U))); \
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

# define LINEAR_WRITE_EDGE(__type, __addr, __val, __isLeftP) \
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

#define REG_GROUP_SET(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT(__regAddr, __val, FXFALSE); \
  SET(((SstRegs*)(__regBase))->__regAddr, (__val)); \
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
#define kNumTriParam 0x1FU
   
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
#define HW_FIFO_OFFSET          0x00200000U    
#elif (GLIDE_PLATFORM & GLIDE_HW_H3)
#define HW_IO_REG_REMAP         SST_IO_OFFSET
#define HW_CMD_AGP_OFFSET       SST_CMDAGP_OFFSET
#define HW_2D_REG_OFFSET        SST_2D_OFFSET
#define HW_3D_REG_OFFSET        SST_3D_OFFSET
#define HW_REGISTER_OFFSET      HW_3D_REG_OFFSET
#else
#error "Must define virtual address spaces for this hw"
#endif

#define HW_FIFO_OFFSET          0x00200000U
#define HW_LFB_OFFSET           SST_LFB_OFFSET
#define HW_TEXTURE_OFFSET       SST_TEX_OFFSET

#if (GLIDE_PLATFORM & GLIDE_HW_CVG) || (GLIDE_PLATFORM & GLIDE_HW_H3)
#define HW_BASE_PTR(__b)        (__b)
#else
#error "Need HW_BASE_PTR to convert hw address into board address."
#endif
   
#define HW_REG_PTR(__b)        ((FxU32*)(((unsigned long)(__b)) + HW_REGISTER_OFFSET))
#define HW_LFB_PTR(__b)        ((FxU32*)(((unsigned long)(__b)) + HW_LFB_OFFSET))
#define HW_TEX_PTR(__b)        ((FxU32*)(((unsigned long)(__b)) + HW_TEXTURE_OFFSET))   

/* access a floating point array with a byte index */
#define FARRAY(p,i)    (*(float *)((i)+(long)(p)))
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
 *
 * AMD: 0x8001
 *  1: MMX
 *  2: 3DNow!(tm)
 */

/*enum {
  kCPUVendorIntel   = 0x0000,
  kCPUVendorAMD     = 0x8001,
  kCPUVendorUnknown = 0xFFFF
};
extern FxI32 GR_CDECL
_cpu_detect_asm(void);*/

extern void GR_CDECL 
single_precision_asm(void);

extern void GR_CDECL 
double_precision_asm(void);
#endif

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
    if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
      DA_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      DA_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) { \
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) { \
      DA_SETF(FARRAY(_s, gc->state.vData.q0Info.offset)*_oow); \
    } \
    else { \
      DA_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) { \
    DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].s_scale); \
    DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].t_scale); \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) { \
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) { \
      DA_SETF(FARRAY(_s, gc->state.vData.q1Info.offset)*_oow); \
    } \
    else { \
      DA_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) { \
    DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].s_scale); \
    DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].t_scale); \
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
    if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) { \
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q0Info.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].t_scale); \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) { \
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q1Info.offset)*_oow); \
    } \
    else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].t_scale); \
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
      union { float f; FxU32 u; } argb; \
      argb.f = *(float *)((unsigned char *)_s + i); \
      argb.u &= 0x00ffffff; \
      TRI_SETF(argb.f); \
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
    if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) { \
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q0Info.offset)*_oow); \
    } \
    else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].t_scale); \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) { \
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q1Info.offset)*_oow); \
    } \
    else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].t_scale); \
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
/* #define GLIDE3_DEBUG 1 */
#endif

#ifdef GLIDE3_DEBUG
#define TEX_INFO(ptr,field)                         ptr##field
#define G3_LOD_TRANSLATE(lod)                       (lod)
#define G3_ASPECT_TRANSLATE(aspect)                 (aspect)
#else /* !GLIDE3_DEBUG */
/* [dBorca] picky preprocessors
#define TEX_INFO(ptr,field)                         ptr##field##Log2*/
#define TEX_INFO(ptr,field)                         ptr field##Log2
#define G3_LOD_TRANSLATE(lod)                       (0x8-lod)
#define G3_ASPECT_TRANSLATE(aspect)                 (0x3-(aspect))
#endif /* !GLIDE3_DEBUG */

#else /* !(defined(GLIDE3) && defined(GLIDE3_ALPHA)) */
#define TEX_INFO(ptr,field)                         ptr##field
#define G3_LOD_TRANSLATE(lod)                       (lod)
#define G3_ASPECT_TRANSLATE(aspect)                 (aspect)
#endif /* !(defined(GLIDE3) && defined(GLIDE3_ALPHA)) */

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
/* gglide.c */
extern void 
_grSliOriginClear(void); 
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) */

GR_ENTRY(grTexDownloadTableExt,
         void,
         (GrChipID_t tmu, GrTexTable_t type,  void *data));

GR_ENTRY(grSetNumPendingBuffers,
         void,
         (FxI32 NumPendingBuffers));

#endif /* __FXGLIDE_H__ */

