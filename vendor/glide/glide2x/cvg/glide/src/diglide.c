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
** Revision 1.1.1.1.2.4  2006/09/02 03:13:53  guillemj
** Remove redundant definition of single_precision_asm and double_precision_asm,
** to fix the code not building from source.
**
** Revision 1.1.1.1.2.3  2005/01/22 14:52:01  koolsmoky
** enabled packed argb for cmd packet type 3
**
** Revision 1.1.1.1.2.2  2004/12/23 20:45:56  koolsmoky
** converted to nasm syntax
** added x86 asm, 3dnow! triangle and mmx, 3dnow! texture download optimizations
**
** Revision 1.1.1.1.2.1  2004/12/12 15:18:58  koolsmoky
** changes to support new cpuid; moved single_precision_asm(), double_precision_asm() from cpudetect.asm
**
** Revision 1.1.1.1  1999/12/07 21:49:08  joseph
** Initial checkin into SourceForge.
**
** 
** 40    6/23/98 5:38p Peter
** lfb hinting
** 
** 39    3/17/98 3:00p Peter
** removed unused stats
** 
** 38    3/02/98 7:22p Peter
** moved internal function to where it is used
** 
** 37    2/20/98 11:00a Peter
** removed glide3 from glid2 tree
** 
** 36    2/20/98 9:05a Peter
** removed remnants of comdex grot
** 
** 35    1/30/98 4:31p Peter
** general clenaup
** 
** 34    1/20/98 10:48a Atai
** validate state in grGlideGetState
 * 
 * 33    1/07/98 10:22a Peter
 * lod dithering env var
 * 
 * 32    1/06/98 3:53p Atai
 * remove grHint, modify grLfbWriteRegion and grGet
 * 
 * 31    12/17/97 4:05p Atai
 * added grChromaRange(), grGammaCorrecionRGB(), grRest(), and grGet()
 * functions
 * 
 * 30    12/09/97 12:20p Peter
 * mac glide port
 * 
 * 29    12/01/97 5:46p Peter
 * fixed variable names in swizzle
 * 
 * 28    12/01/97 5:17p Peter
 * 
 * 27    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 26    11/14/97 5:02p Peter
 * more comdex stuff
 * 
 * 25    11/14/97 12:09a Peter
 * comdex thing and some other stuff
 * 
 * 24    11/12/97 2:27p Peter
 * 
 * 23    11/12/97 11:39a Dow
 * H3 Stuff
 * 
 * 22    11/12/97 9:21a Dow
 * Changed CVG_FIFO to USE_PACKET_FIFO
 * 
 * 21    11/04/97 4:00p Dow
 * Banshee Mods
 * 
 * 20    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 19    10/16/97 3:40p Peter
 * packed rgb
 * 
 * 18    9/20/97 10:53a Peter
 * keep track of palette stats
 * 
 * 17    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 16    9/10/97 10:13p Peter
 * fifo logic from GaryT, non-normalized fp first cut
 * 
 * 15    7/25/97 11:40a Peter
 * removed dHalf, change field name to match real use for cvg
 * 
 * 14    7/08/97 2:48p Peter
 * 
 * 13    6/30/97 3:20p Peter
 * error callback
 * 
 * 12    6/23/97 4:43p Peter
 * cleaned up #defines etc for a nicer tree
 * 
**
*/

#include <string.h>
#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#include "rcver.h"
static char glideIdent[] = "@#%" VERSIONSTR ;

/* the root of all EVIL */
struct _GlideRoot_s GR_CDECL _GlideRoot;
/* This is global to speed up the function call wrappers */

/*---------------------------------------------------------------------------
**
*/
void
_grDisplayStats(void)
{
    int frames = _GlideRoot.stats.bufferSwaps;

    if (frames <= 0) frames = 1;
    gdbg_info(80,"GLIDE STATISTICS:\n");
    gdbg_info(80,"     triangles processed: %7d       tris drawn: %7d\n",
              _GlideRoot.stats.trisProcessed, _GlideRoot.stats.trisDrawn);
    gdbg_info(80,"            buffer swaps: %7d       tris/frame: %7d , %d\n",
                   _GlideRoot.stats.bufferSwaps,
                   _GlideRoot.stats.trisProcessed/frames,
                   _GlideRoot.stats.trisDrawn/frames);
    gdbg_info(80,"                  points: %7d       pnts/frame: %7d\n",
              _GlideRoot.stats.pointsDrawn, _GlideRoot.stats.pointsDrawn/frames);
    gdbg_info(80,"                   lines: %7d      lines/frame: %7d\n",
              _GlideRoot.stats.linesDrawn, _GlideRoot.stats.linesDrawn/frames);
    gdbg_info(80,"       texture downloads: %7d    texture bytes: %7d\n",
                   _GlideRoot.stats.texDownloads,  _GlideRoot.stats.texBytes);
    gdbg_info(80,"       palette downloads: %7d    palette bytes: %7d\n",
                   _GlideRoot.stats.palDownloads, _GlideRoot.stats.palBytes);
    gdbg_info(80,"           NCC downloads: %7d        NCC bytes: %7d\n",
              _GlideRoot.stats.nccDownloads, _GlideRoot.stats.nccBytes);

#if USE_PACKET_FIFO
    gdbg_info(80,"\tCommandFifo:\n");
    gdbg_info(80,"\t\tWraps: %ld\n", _GlideRoot.stats.fifoWraps);
    if (_GlideRoot.stats.fifoWraps > 0) {
      gdbg_info(80,"\t\tAvg Drain Depth: %g\n", 
                (double)_GlideRoot.stats.fifoWrapDepth / _GlideRoot.stats.fifoWraps);
    }
    gdbg_info(80,"\t\tStalls: %ld\n", _GlideRoot.stats.fifoStalls);
    if (_GlideRoot.stats.fifoStalls > 0) {
      gdbg_info(80,"\t\tAvg Stall Depth: %g\n", 
                (double)_GlideRoot.stats.fifoStallDepth / _GlideRoot.stats.fifoStalls);
    }
#endif /* CVG_FIFO */
}

#if !USE_PACKET_FIFO
/*
** fifoFree is kept in bytes, each fifo entry is 8 bytes, but since there
** are headers involved, we assume an average of 2 registers per 8 bytes
** or 4 bytes of registers stored in every fifo entry
*/
void
_grReCacheFifo(FxI32 n)
{
#if !(GLIDE_PLATFORM & GLIDE_HW_H3)
  GR_DCL_GC;
  gc->state.fifoFree = ((grSstStatus() >> SST_MEMFIFOLEVEL_SHIFT) & 0xffff)<<2;

#if 0
  gc->state.fifoFree -= gc->hwDep.sst1Dep.swFifoLWM + n;
#endif
#endif
}

FxI32 GR_CDECL
_grSpinFifo(FxI32 n)
{
  GR_DCL_GC;
  do {
    _grReCacheFifo(n);
  } while (gc->state.fifoFree < 0);

  return gc->state.fifoFree;
}
#endif /* !USE_PACKET_FIFO */

/*---------------------------------------------------------------------------
**
*/
void 
_grSwizzleColor(GrColor_t *color)
{
  GR_DCL_GC;
  FxU32 red, green, blue, alpha;
  
  switch(gc->state.color_format) {
  case GR_COLORFORMAT_ARGB:
    break;

  case GR_COLORFORMAT_ABGR:
    red     = (*color & 0x000000ff) << 16; /* 00RR0000 */
    blue    = (*color & 0x00ff0000) >> 16; /* 000000BB */
    *color &= 0xff00ff00;                  /* AA00GG00 */
    *color |= (red | blue);                /* AARRGGBB */
    break;

  case GR_COLORFORMAT_RGBA:
    blue   = (*color & 0x0000ff00) >> 8;  /* 000000BB */
    green  = (*color & 0x00ff0000) >> 8;  /* 0000GG00 */
    red    = (*color & 0xff000000) >> 8;  /* 00RR0000 */
    alpha  = (*color & 0x000000ff) << 24; /* AA000000 */
    *color = alpha | red | green | blue;  /* AARRGGBB */
    break;

  case GR_COLORFORMAT_BGRA:
    blue   = (*color & 0xff000000) >> 24; /* 000000BB */
    green  = (*color & 0x00ff0000) >> 8;  /* 0000GG00 */
    red    = (*color & 0x0000ff00) << 8;  /* 00RR0000 */
    alpha  = (*color & 0x000000ff) << 24; /* AA000000 */
    *color = alpha | red | green | blue;  /* AARRGGBB */
    break;

  default:
    GR_ASSERT(0);
    break;
  }
} /* _grSwizzleColor */

/*---------------------------------------------------------------------------
** grGlideGetVersion
** NOTE: allow this to be called before grGlideInit()
*/
GR_DIENTRY(grGlideGetVersion, void, (char version[80]))
{
  GDBG_INFO(87,"grGlideGetVersion(0x%x) => \"%s\"\n",version,glideIdent+3);
  GR_ASSERT(version != NULL);
  strcpy(version,glideIdent+3);
} /* grGlideGetVersion */

/*---------------------------------------------------------------------------
** grGlideGetState
*/
GR_DIENTRY(grGlideGetState, void, (GrState *state))
{
  GR_BEGIN_NOFIFOCHECK("grGlideGetState",87);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",state);
  GR_ASSERT(state != NULL);

  *state = gc->state;
  GR_END();
} /* grGlideGetState */

#ifndef GLIDE_ALPHA
/*---------------------------------------------------------------------------
** grHints
*/
GR_DIENTRY(grHints, void, (GrHint_t hintType, FxU32 hints))
{
  GR_BEGIN_NOFIFOCHECK("grHints",85);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x)\n",hintType,hints);

  switch (hintType) {
  case GR_HINT_STWHINT:
    if (gc->state.paramHints != hints) {
      gc->state.paramHints = hints;
      _grUpdateParamIndex();
    }
    break;

  case GR_HINT_FIFOCHECKHINT:
    /* swFifoLWM is kept internally in bytes, hints are in fifo entries */ 
    gc->state.checkFifo = hints;
    break;

  case GR_HINT_FPUPRECISION:
    hints ? double_precision_asm() : single_precision_asm();
    break;

  case GR_HINT_ALLOW_MIPMAP_DITHER:
    /* Regardless of the game hint, force the user selection */
    gc->state.allowLODdither = ((_GlideRoot.environment.texLodDither != 0) || 
                                hints);
    break;

  case GR_HINT_LFB_WRITE:
  {
    const FxU32 lfbRange = (((hints * gc->state.screen_height) + 0x1000UL) & 
                            ~(0x1000UL - 1));
    
    pciLinearRangeSetPermission((const FxU32)((const FxU8*)gc->base_ptr + 0x200000UL),
                                0x200000UL,
                                FXFALSE);
    pciLinearRangeSetPermission((const FxU32)gc->lfb_ptr,
                                lfbRange,
                                FXTRUE);
    pciLinearRangeSetPermission((const FxU32)((const FxU8*)gc->lfb_ptr + lfbRange),
                                0x400000UL - lfbRange,
                                FXFALSE);
    break;
  }

  case GR_HINT_LFB_PROTECT:
    pciLinearRangeSetPermission((const FxU32)((const FxU8*)gc->base_ptr + 0x200000UL),
                                0x200000UL,
                                FXTRUE);
    pciLinearRangeSetPermission((const FxU32)gc->lfb_ptr,
                                0x400000,
                                FXFALSE);
    break;

  case GR_HINT_LFB_RESET:
    pciLinearRangeSetPermission((const FxU32)gc->base_ptr,
                                0x1000000UL,
                                FXTRUE);
    break;
  
  default:
    GR_CHECK_F(myName, 1, "invalid hints type");
  }
  GR_END();
} /* grHints */
#endif

/*---------------------------------------------------------------------------
** grGlideInit
*/
GR_DIENTRY(grGlideInit, void, (void))
{
  GDBG_INIT();
  
  GDBG_INFO(80,"grGlideInit()\n");
  _GlideInitEnvironment();                      /* the main init code */
  FXUNUSED(*glideIdent);

#if GDBG_INFO_ON
  gdbg_error_set_callback(_grErrorCallback);
#endif

  grResetTriStats();

  GDBG_INFO(281,"grGlideInit --done---------------------------------------\n");
} /* grGlideInit */


/*---------------------------------------------------------------------------
**  grGlideShamelessPlug - grGlideShamelessPlug
**
**  Returns:
**
**  Notes:
**
*/
GR_DIENTRY(grGlideShamelessPlug, void, (const FxBool mode))
{
  GDBG_INFO(80,"grGlideShamelessPlug(%d)\n",mode);
  _GlideRoot.environment.shamelessPlug = mode;
} /* grGlideShamelessPlug */

/*---------------------------------------------------------------------------
**  grResetTriStats - Set triangle counters to zero.
*/
GR_DIENTRY(grResetTriStats, void, (void))
{
  GDBG_INFO(80,"grResetTriStats()\n");
  _GlideRoot.stats.bufferSwaps = 0;
  _GlideRoot.stats.linesDrawn = 0;
  _GlideRoot.stats.trisProcessed = 0;
  _GlideRoot.stats.trisDrawn = 0;
  _GlideRoot.stats.texDownloads = 0;
  _GlideRoot.stats.texBytes = 0;
  _GlideRoot.stats.palDownloads = 0;
  _GlideRoot.stats.palBytes = 0;
} /* grResetTriStats */


/*---------------------------------------------------------------------------
**  grResetTriStats - Set triangle counters to zero.
*/
GR_DIENTRY(grTriStats, void, (FxU32 *trisProcessed, FxU32 *trisDrawn))
{
  GDBG_INFO(80,"grTriStats() => %d %d\n",
                _GlideRoot.stats.trisProcessed,
                _GlideRoot.stats.trisDrawn);
  *trisProcessed = _GlideRoot.stats.trisProcessed;
  *trisDrawn = _GlideRoot.stats.trisDrawn;
} /* grTriStats */

void GR_CDECL
_grFence(void)
{
  GDBG_INFO(120,"\t\t\t\t\t\t\tFENCE\n");
  P6FENCE;
}
