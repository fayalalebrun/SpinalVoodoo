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
 * 
 * 10    6/18/97 5:54p Dow
 * P6 adjustments
 * 
 * 9     3/13/97 2:51a Jdt
 * Removed splash from grGlideInit().
 * 
 * 8     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 7     2/12/97 2:09p Hanson
 * Hopefully removed the rest of my muckage. 
 * 
 * 6     2/12/97 11:25a Hanson
 * 
 * 5     1/18/97 11:38p Dow
 * Removed _curGCFuncs Global (moved into _GlideRoot)
 * 
 * 4     1/16/97 3:37p Dow
 * Added _curGCFuncs global
 * 
 * 3     1/14/97 10:44a Dow
 * Modified grGlideInit to only call sstopen if splash screen is going to
 * happen && HW != SST96
 * 
 * 2     1/09/97 10:50a Dow
 * disabled splash screen for sst-96
 * 
 * 1     12/23/96 1:39p Dow
 * Changes for multiplatform
 * 
 * 32    11/17/96 11:16p Garymct
 * Updated grSplash code to set all of the necessary state itself rather
 * than expecting the caller to do so.
 * 
 * 31    11/18/96 1:37a Tarolli
 * fixed grAlphaBlendFunction warning bug
 * 
 * 29    11/15/96 3:40p Jdt
 * Fixed SST-96 build.
 * 
 * 28    11/15/96 3:24p Tarolli
 * renamed version.h to rcver.h ,  added some alpha blend function
 * checking
**
*/

#include <string.h>
#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#if ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
#include <init.h>
#endif

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
    gdbg_info(80,"GLIDE STATISTICS:                       fifo spins: %7d\n",
                   _GlideRoot.stats.fifoSpins);
    gdbg_info(80,"     triangles processed: %7d       tris drawn: %7d\n",
                   _GlideRoot.stats.trisProcessed,
                   _GlideRoot.stats.trisDrawn);
    gdbg_info(80,"            buffer swaps: %7d       tris/frame: %7d , %d\n",
                   _GlideRoot.stats.bufferSwaps,
                   _GlideRoot.stats.trisProcessed/frames,
                   _GlideRoot.stats.trisDrawn/frames);
    gdbg_info(80,"                  points: %7d       pnts/frame: %7d\n",
                   _GlideRoot.stats.pointsDrawn,
                   _GlideRoot.stats.pointsDrawn/frames);
    gdbg_info(80,"                   lines: %7d      lines/frame: %7d\n",
                   _GlideRoot.stats.linesDrawn,
                   _GlideRoot.stats.linesDrawn/frames);
    gdbg_info(80,"       texture downloads: %7d    texture bytes: %7d\n",
                   _GlideRoot.stats.texDownloads,  _GlideRoot.stats.texBytes);
    gdbg_info(80,"       palette downloads: %7d    palette bytes: %7d\n",
                   _GlideRoot.stats.palDownloads, _GlideRoot.stats.palBytes);
#ifdef GLIDE_DEBUG
    /* these stats are only kept in debugging mode */
    gdbg_info(80,"       Min PCI FIFO free: 0x%04x (%d)\n",
                  _GlideRoot.stats.minPciFIFOFree, _GlideRoot.stats.minPciFIFOFree );
    gdbg_info(80,"    Min Memory FIFO free: 0x%04x (%d)\n",
                  _GlideRoot.stats.minMemFIFOFree, _GlideRoot.stats.minMemFIFOFree);
#endif
}
/*
** fifoFree is kept in bytes, each fifo entry is 8 bytes, but since there
** are headers involved, we assume an average of 2 registers per 8 bytes
** or 4 bytes of registers stored in every fifo entry
*/
void
_grReCacheFifo( FxI32 n )
{
  GR_DCL_GC;
  gc->state.fifoFree = ((grSstStatus() >> SST_MEMFIFOLEVEL_SHIFT) & 0xffff)<<2;
  gc->state.fifoFree -= gc->hwDep.sst1Dep.swFifoLWM + n;
}

FxI32 GR_CDECL
_grSpinFifo( FxI32 n )
{
  GR_DCL_GC;
  do {
    _GlideRoot.stats.fifoSpins++;
    _grReCacheFifo( n );
  } while (gc->state.fifoFree < 0);
  return gc->state.fifoFree;
}

/*---------------------------------------------------------------------------
** No Comment
*/
#ifdef GLIDE_DEBUG
FxBool
_grCanSupportDepthBuffer( void )
{
  GR_DCL_GC;
  if ( gc->state.screen_height == 640 ) {
    if ( gc->fbuf_size == 1 ) {
      return FXFALSE;
    }
  } else if ( gc->state.screen_width == 800 ) {
    if ( ( gc->fbuf_size == 1 ) ||
        ( gc->fbuf_size == 2 ) ) {
      return FXFALSE;
    }
  }
  return FXTRUE;
} /* _grCanSupportDepthBuffer */
#endif

/*---------------------------------------------------------------------------
**
*/
void 
_grSwizzleColor( GrColor_t *color )
{
  GR_DCL_GC;
  unsigned long red, green, blue, alpha;
  
  switch( gc->state.color_format ) {
  case GR_COLORFORMAT_ARGB:
    break;
  case GR_COLORFORMAT_ABGR:
    blue = *color & 0x00ff;
    red = ( *color >> 16 ) & 0xff;
    *color &= 0xff00ff00;
    *color |= ( blue << 16 );
    *color |= red;
    break;
  case GR_COLORFORMAT_RGBA:
    red    = ( *color & 0x0000ff00 ) >> 8;
    green  = ( *color & 0x00ff0000 ) >> 16;
    blue   = ( *color & 0xff000000 ) >> 24;
    alpha  = ( *color & 0x000000ff );
    *color = ( alpha << 24 ) | ( blue << 16 ) | ( green << 8 ) | red;
    break;
  case GR_COLORFORMAT_BGRA:
    red    = ( *color & 0xff000000 ) >> 24;
    green  = ( *color & 0x00ff0000 ) >> 16;
    blue   = ( *color & 0x0000ff00 ) >> 8;
    alpha  = ( *color & 0x000000ff );
    *color = ( alpha << 24 ) | ( blue << 16 ) | ( green << 8 ) | red;
    break;
  }
} /* _grSwizzleColor */

/*---------------------------------------------------------------------------
** grGlideGetVersion
** NOTE: allow this to be called before grGlideInit()
*/
GR_DIENTRY(grGlideGetVersion, void, ( char version[80] ))
{
  GDBG_INFO((87,"grGlideGetVersion(0x%x) => \"%s\"\n",version,glideIdent+3));
  GR_ASSERT(version != NULL);
  strcpy(version,glideIdent+3);
} /* grGlideGetVersion */

/*---------------------------------------------------------------------------
** grGlideGetState
*/
GR_DIENTRY(grGlideGetState, void, ( GrState *state ))
{
  GR_BEGIN_NOFIFOCHECK("grGlideGetState",87);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",state));
  GR_ASSERT(state != NULL);
  *state = gc->state;
  GR_END();
} /* grGlideGetState */

/*---------------------------------------------------------------------------
** grHints
*/
GR_DIENTRY(grHints, void, (GrHint_t hintType, FxU32 hints))
{
  extern void GR_CDECL single_precision_asm(void);
  extern void GR_CDECL double_precision_asm(void);
  GR_BEGIN_NOFIFOCHECK("grHints",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x)\n",hintType,hints));

  switch (hintType) {
    case GR_HINT_STWHINT:
      if (gc->state.paramHints != hints) {
        gc->state.paramHints = hints;
        _grUpdateParamIndex();
      }
      break;
    case GR_HINT_FIFOCHECKHINT:
      if (hints) {
        gc->state.checkFifo = FXTRUE;
        /* swFifoLWM is kept internally in bytes, hints are in fifo
           entries */ 
        if (_GlideRoot.environment.swFifoLWM >= 0)
          gc->hwDep.sst1Dep.swFifoLWM =
            _GlideRoot.environment.swFifoLWM << 2;
        else
          gc->hwDep.sst1Dep.swFifoLWM = (hints & 0xffff) << 2;

      } else
        gc->state.checkFifo = FXFALSE;
      break;
    case GR_HINT_FPUPRECISION:
      hints ? double_precision_asm() : single_precision_asm();
      break;
    case GR_HINT_ALLOW_MIPMAP_DITHER:
      gc->state.allowLODdither = hints;
      break;
    default:
      GR_CHECK_F( myName, 1, "invalid hints type" );
  }
  GR_END();
} /* grHints */

/*---------------------------------------------------------------------------
** grGlideInit
*/
GR_DIENTRY(grGlideInit, void, ( void ))
{
  GDBG_INIT();
  
  GDBG_INFO((80,"grGlideInit()\n"));
  _GlideInitEnvironment();                      /* the main init code */
  FXUNUSED(*glideIdent);

  grResetTriStats();
  GDBG_INFO((281,"grGlideInit --done---------------------------------------\n"));
} /* grGlideInit */


/*---------------------------------------------------------------------------
**  grGlideShamelessPlug - grGlideShamelessPlug
**
**  Returns:
**
**  Notes:
**
*/
GR_DIENTRY(grGlideShamelessPlug, void, ( const FxBool mode ))
{
  GDBG_INFO((80,"grGlideShamelessPlug(%d)\n",mode));
  _GlideRoot.environment.shamelessPlug = mode;
} /* grGlideShamelessPlug */



/*---------------------------------------------------------------------------
**  grResetTriStats - Set triangle counters to zero.
*/
GR_DIENTRY(grResetTriStats, void, ( void ))
{
  GDBG_INFO((80,"grResetTriStats()\n"));
  _GlideRoot.stats.fifoSpins = 0;
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
GR_DIENTRY(grTriStats, void, ( FxU32 *trisProcessed, FxU32 *trisDrawn ))
{
  GDBG_INFO((80,"grTriStats() => %d %d\n",
                _GlideRoot.stats.trisProcessed,
                _GlideRoot.stats.trisDrawn));
  *trisProcessed = _GlideRoot.stats.trisProcessed;
  *trisDrawn = _GlideRoot.stats.trisDrawn;
} /* grTriStats */

void GR_CDECL
_grFence( void )
{
#if (GLIDE_PLATFORM & GLIDE_HW_SST96) && GDBG_INFO_ON
  GR_DCL_GC;
  GDBG_INFO((125,"\t\t\t\t%d writes since last fence\n",
             gc->hwDep.sst96Dep.writesSinceFence));
#endif
  GDBG_INFO((125,"\t\t\t\t\t\t\tFENCE\n"));

  P6FENCE;
}
