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
** Revision 1.1.1.1.8.4  2004/10/04 09:13:33  dborca
** make sure SLI boards are correctly counted
**
** Revision 1.1.1.1.8.3  2004/02/16 07:42:14  dborca
** grSetNumPendingBuffers visible with grGetProcAddress
**
** Revision 1.1.1.1.8.2  2003/12/08 13:13:11  dborca
** better Texus2 integration
**
** Revision 1.1.1.1.8.1  2003/11/03 13:34:29  dborca
** Voodoo2 happiness (DJGPP & Linux)
**
** Revision 1.1.1.1  1999/12/07 21:42:30  joseph
** Initial checkin into SourceForge.
**
** 
** 1     10/08/98 11:30a Brent
** 
** 33    10/07/98 9:43p Peter
** triangle procs for 3DNow!(tm)
** 
** 32    9/21/98 4:44p Atai
** fixed grGet
** 
** 31    9/16/98 8:01p Atai
** get more precision for fifo fullness
** 
** 30    8/05/98 11:24p Atai
** 1. grBufferSwap argument FxU32
** 2. grGet return FxU32
** 
** 29    7/24/98 1:41p Hohn
** 
** 28    7/02/98 10:27a Atai
** added guQueryResolutionXYExt
** 
** 27    7/01/98 3:53p Atai
** added GR_BITS_GAMMA
** 
** 26    6/30/98 2:54p Atai
** fixed grQueryResolution
** 
** 25    6/30/98 11:45a Atai
** fixed grQueryResolution bug
** 
** 24    6/29/98 6:22p Atai
** fixed grReset(GR_STATS_TRIANGLES) in debug mode
** 
** 23    6/25/98 2:08p Atai
** grGet (GR_NUM_BOARDS) can be called before grGlideInit
** 
** 22    6/25/98 10:39a Peter
** more cb checks
** 
** 21    6/24/98 6:55p Atai
** undo texture line
** 
** 20    6/24/98 1:47p Atai
** code clean up; rename texute line routine
** 
** 19    6/18/98 12:51p Atai
** added GR_FOG_WITH_TABLE_ON_Q and GrResolution
** 
** 18    6/18/98 10:59a Atai
** added grDrawTextureLine for OGL
** 
** 17    6/17/98 10:39a Atai
** fix grDepthBiasLevel
** 
** 16    6/15/98 2:37p Atai
** fixed GrResolution
** 
** 15    6/08/98 7:13p Atai
** remove unused defines, add state selectors and update state routine
** prototype
** 
** 14    6/08/98 3:53p Atai
** fixed grReset(GR_STATS_TRIANGLES)
** 
** 12    5/19/98 6:43p Atai
** swap history
** 
** 11    5/12/98 7:30p Atai
** added "FOGCOORD" for fog coordinate extension
** 
** 10    5/11/98 4:13p Atai
** added frame buffer constrained query stuff
** 
** 8     5/01/98 11:59a Atai
** added TEXMIRROR string for GR_EXTENSION
** 
** 7     4/24/98 2:18p Atai
** fix GR_NON_POWER_OF_TWO_TEXTURES
** 
** 6     4/16/98 6:15p Atai
** added grReset(GR_VERTEX_PARAMETER)
** 
** 5     1/30/98 4:27p Atai
** gufog* prototype
** 
** 4     1/28/98 6:36p Atai
** remove z,w, min_max definition
** 
** 3     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 2     1/16/98 6:44p Atai
 * fixed for glide 3 build
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 9     1/10/98 4:01p Atai
 * inititialize vertex layout, viewport, added defines
 * 
 * 6     1/07/98 6:04p Atai
 * GR_PENDING_BUFFERSWAPS
 * 
 * 5     1/06/98 3:53p Atai
 * remove grHint, modify grLfbWriteRegion and grGet
 * 
 * 3     12/18/97 10:52a Atai
 * fixed grGet(GR_VIDEO_POS)
 * 
 * 2     12/17/97 4:05p Atai
 * added grChromaRange(), grGammaCorrecionRGB(), grRest(), and grGet()
 * functions
 * 
 * 1     12/14/97 1:41p Pgj
**
*/
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
#include <string.h>
#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"
#include "rcver.h"

#include "fxinline.h"

static const FxU32 _grMemOffset[16] = 
{
  35,                           /* 320x200 */
  40,                           /* 320x240 */
  0,                            /* 400x256 */
  96,                           /* 512x384 */
  65536,                        /* 640x200 */
  65536,                        /* 640x350 */
  130,                          /* 640x400 */
  150,                          /* 640x480 */
  247,                          /* 800x600 */
  345,                          /* 960x720 */
  210,                          /* 856x480 */
  64,                           /* 512x256 */
  384,                          /* 1024x768 */
  65536,                        /* 1280x1024 */
  65536,                        /* 1600x1200 */
  70                            /* 400x300 */
};

static const FxU32 _grResolutionRefresh[16][9] = 
{
  {                             /* 320x200 */
    0,                          /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 320x240 */
    0,                          /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 400x256 */
    0,                          /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 512x384 */
    196608,                     /* 60 Hz */
    0,                          /* 70 Hz */
    196608,                     /* 72 Hz */
    196608,                     /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    196608,                     /* 85 Hz */
    196608,                     /* 120 Hz */
  },
  {                             /* 640x200 */
    0,                          /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 640x350 */
    0,                          /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 640x400 */
    0,                          /* 60 Hz */
    256000,                     /* 70 Hz */
    0,                          /* 72 Hz */
    256000,                     /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    256000,                     /* 85 Hz */
    256000,                     /* 120 Hz */
  },
  {                             /* 640x480 */
    307200,                     /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    307200,                     /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    307200,                     /* 85 Hz */
    307200,                     /* 120 Hz */
  },
  {                             /* 800x600 */
    480000,                     /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    480000,                     /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    480000,                     /* 85 Hz */
    480000,                     /* 120 Hz */
  },
  {                             /* 960x720 */
    691200,                     /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    691200,                     /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    691200,                     /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 856x480 */
    410880,                     /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 512x256 */
    0,                          /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 1024x768 */
    786432,                     /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    786432,                     /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    786432,                     /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 1280x1024 */
    0,                          /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 1600x1200 */
    0,                          /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  },
  {                             /* 400x300 */
    0,                          /* 60 Hz */
    0,                          /* 70 Hz */
    0,                          /* 72 Hz */
    0,                          /* 75 Hz */
    0,                          /* 80 Hz */
    0,                          /* 90 Hz */
    0,                          /* 100 Hz */
    0,                          /* 85 Hz */
    0,                          /* 120 Hz */
  }
};

/*-------------------------------------------------------------------
  Function: grGet
  Date: 14-Dec-97
  Implementor(s): pgj
  Description:
        Implements Glide 3.0 grGet() and grGetString()
        XXX This is a work in progress.  There are ugly #ifdefs in
        some of the selectors, yet grGet seems like it should be di.
        After bringup is further along, consider pushing some of this
        into dd in order to eliminate platform #ifdefs.
  Arguments:
    pname   -  parameter to be returned, e.g. GR_BITS_RGBA
    params  -  address of returned parameters
  
  Return:
    FXTRUE if successful
  -------------------------------------------------------------------*/
GR_DIENTRY(grGet, FxU32, (FxU32 pname, FxU32 plength, FxI32 *params))
{
#define FN_NAME "grGet"

  FxBool retVal = FXFALSE;
  GrHwConfiguration *hwc = &_GlideRoot.hwConfig;
  GR_DCL_GC;

  if (!params) {
    return FXFALSE;
  } else {
    retVal = FXTRUE;
  }

  switch(pname) {
  case GR_BITS_DEPTH:
    if (plength == 4) {
      retVal = plength;
      *params = SST1_BITS_DEPTH;
    }
    break;
  case GR_BITS_RGBA:
    if (plength == 16) {
      retVal = plength;
      *params = 5;
      *(params+1) = 6;
      *(params+2) = 5;
      *(params+3) = 0;
    }
    break;
  case GR_FIFO_FULLNESS:
    if (plength == 8) {
      FxU32 status;
      status = GR_GET(((SstRegs *)gc->reg_ptr)->status);
      status = 0xffff - ((status >> SST_MEMFIFOLEVEL_SHIFT) & 0xffff);
      *params = (status << 8)+(status >> 8);
      *(params+1) = status;
      retVal = plength;
    }
    break;
  case GR_GLIDE_STATE_SIZE:
    if (plength == 4) {
      *params = sizeof(GrState);
      retVal = plength;
    }
    break;
  case GR_GLIDE_VERTEXLAYOUT_SIZE:
    if (plength == 4) {
      *params = sizeof(GrVertexLayout);
      retVal = plength;
    }
    break;
  case GR_FOG_TABLE_ENTRIES:
    if (plength == 4) {
      *params = kInternalFogTableEntryCount;
      retVal = plength;

      gc->state.cbEnvironment.fogTableSizeP = FXTRUE;
    }
    break;
  case GR_BITS_GAMMA:
    if (plength == 4) {
      *params = 8;
      retVal = plength;
    }
    break;
  case GR_GAMMA_TABLE_ENTRIES:
    if (plength == 4) {
      *params = VOODOO_GAMMA_TABLE_SIZE;
      retVal = plength;
    }
    break;
  case GR_IS_BUSY:
    if (plength == 4) {
      FxU32 status;
      status = GR_GET(((SstRegs *)gc->reg_ptr)->status);
      if (status & SST_BUSY)
        *params = FXTRUE;
      else
        *params = FXFALSE;
      retVal = plength;
    }
    break;
  case GR_LFB_PIXEL_PIPE:
    if (plength == 4) {
      *params = FXTRUE;
      retVal = plength;
    }
    break;
  case GR_MAX_TEXTURE_SIZE:
    if (plength == 4) {
      *params = 256;
      retVal = plength;      
    }
    break;
  case GR_MAX_TEXTURE_ASPECT_RATIO:
    if (plength == 4) {
      *params = 3;
      retVal = plength;      
    }
    break;
  case GR_MEMORY_FB:
    if ((hwc) && (plength == 4)) {
      *params = hwc->SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.fbRam << 20;
      retVal = plength;
    }
    break;
  case GR_MEMORY_TMU:
    if ((hwc) && (plength == 4)) {
      *params = hwc->SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[0].tmuRam << 20;
      retVal = plength;
    }
    break;
  case GR_MEMORY_UMA:
    if ((hwc) && (plength == 4)) {
      *params = 0;    /* XXX non-UMA architecture */
      retVal = plength;
    }
    break;
  case GR_NUM_BOARDS:
    if (plength == 4) {
      *params =
#if 1/*GLIDE_INIT_HAL*/ /* [dBorca] must handle SLI correctly */
        (_grSstDetectResources() ? _GlideRoot.hwConfig.num_sst : 0);
#else /* !GLIDE_INIT_HAL */ 
      sst1InitNumBoardsInSystem();
#endif /* !GLIDE_INIT_HAL */
      retVal = plength;
    }
    break;
  case GR_NON_POWER_OF_TWO_TEXTURES:
    if (plength == 4) {
      *params = FXFALSE;
      retVal = plength;
    }
    break;
  case GR_NUM_FB:
    if (plength == 4) {
      if (hwc->SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.sliDetect)
        *params = 2;
      else
        *params = 1;
      retVal = plength;
    }
    break;
  case GR_NUM_TMU:
    if (plength == 4) {
      *params = _GlideRoot.GCs[_GlideRoot.current_sst].num_tmu;
      retVal = plength;
    }
    break;
  case GR_PENDING_BUFFERSWAPS:
    if (plength == 4) {
      *params = _grBufferNumPending();
      retVal = plength;
    }
    break;
  case GR_REVISION_FB:
    if ((hwc) && (plength == 4)) {
      *params = hwc->SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.fbiRev;
      retVal = plength;
    }
    break;
  case GR_REVISION_TMU:
    if ((hwc) && (plength == 4))  {
      *params = hwc->SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[_GlideRoot.current_sst].tmuRev;
      retVal = plength;
    }
    break;
  case GR_STATS_LINES:
    if (plength == 4) {
      *params = _GlideRoot.stats.linesDrawn;
      retVal = plength;
    }
    break;
  case GR_STATS_PIXELS_AFUNC_FAIL:
    if ((((SstRegs *)gc->reg_ptr)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->reg_ptr)->stats.fbiAfuncFail);
      retVal = plength;
    }
    break;
  case GR_STATS_PIXELS_CHROMA_FAIL:
    if ((((SstRegs *)gc->reg_ptr)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->reg_ptr)->stats.fbiChromaFail);
      retVal = plength;
    }
    break;
  case GR_STATS_PIXELS_DEPTHFUNC_FAIL:
    if ((((SstRegs *)gc->reg_ptr)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->reg_ptr)->stats.fbiZfuncFail);
      retVal = plength;
    }
    break;
  case GR_STATS_PIXELS_IN:
    if ((((SstRegs *)gc->reg_ptr)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->reg_ptr)->stats.fbiPixelsIn);
      retVal = plength;
    }
    break;
  case GR_STATS_PIXELS_OUT:
    if ((((SstRegs *)gc->reg_ptr)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->reg_ptr)->stats.fbiPixelsOut);
      retVal = plength;
    }
    break;
  case GR_STATS_POINTS:
    if (plength == 4) {
      *params = _GlideRoot.stats.pointsDrawn;
      retVal = plength;
    }
    break;
  case GR_STATS_TRIANGLES_IN:
    if (plength == 4) {
      *params = _GlideRoot.stats.trisProcessed;
      retVal = plength;       /* XXX TBD */
    }
    break;
  case GR_STATS_TRIANGLES_OUT:
    if (plength == 4) {
      *params = GR_GET(((SstRegs *)gc->reg_ptr)->fbiTrianglesOut) - _GlideRoot.stats.othertrisDrawn;
      retVal = plength;
    }
    break;
  case GR_NUM_SWAP_HISTORY_BUFFER:
    if (plength == 4) {
      *params = 8;       /* in Voodoo2 and Banshee, the swap history size is 8 */
      retVal = plength;
    }
    break;
  case GR_SWAP_HISTORY:
    if (plength == 32) {
      FxU32 sh = GR_GET(((SstRegs *)gc->reg_ptr)->fbiSwapHistory), i;
      for (i = 0; i < 8; i++) {
        *(params +i) = sh & 0xf;
        sh = sh >> 4;
      }
      retVal = plength;
    }
    break;
  case GR_SUPPORTS_PASSTHRU:
    if (plength == 4) {
      *params = FXTRUE;
      retVal = plength;
    }
    break;
  case GR_TEXTURE_ALIGN:
#ifndef SST_TEXTURE_ALIGN       /* XXX clean up */
#define SST_TEXTURE_ALIGN 8
#endif
    if (plength == 4) {
      *params = SST_TEXTURE_ALIGN;
      retVal = plength;
    }
    break;
  case GR_VIDEO_POSITION:
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
    if (plength == 8) {
      FxI32 hvRetrace;
      hvRetrace = GR_GET(((SstRegs *)gc->reg_ptr)->hvRetrace);
      *params = hvRetrace & VRETRACEMASK;
      *(params+1) = (hvRetrace >> HRETRACEPOS ) & VRETRACEMASK;
      retVal = plength;
    }
#endif
  break;
  case GR_VIEWPORT:
    if (plength == 16) {
      *params = (FxI32) (gc->state.Viewport.ox - gc->state.Viewport.hwidth);
      *(params+1) = (FxI32) (gc->state.Viewport.oy - gc->state.Viewport.hheight);
      *(params+2) = (FxI32) (gc->state.Viewport.hwidth * 2.f);
      *(params+3) = (FxI32) (gc->state.Viewport.hheight * 2.f);
      retVal = plength;
    }
    break;
  case GR_WDEPTH_MIN_MAX:
    if (plength == 8) {
      *params = SST1_WDEPTHVALUE_NEAREST;
      *(params+1) = SST1_WDEPTHVALUE_FARTHEST;
      retVal = plength;
    }
    break;
  case GR_ZDEPTH_MIN_MAX:
    if (plength == 8) {
      *params = SST1_ZDEPTHVALUE_NEAREST;
      *(params+1) = SST1_ZDEPTHVALUE_FARTHEST;
      retVal = plength;
    }
    break;
  default:
    retVal = FXFALSE;       /* XXX TBD */
    break;
  } /* end switch */
  
  return retVal;
#undef FN_NAME
} /* end grGet() */


/*-------------------------------------------------------------------
  Function: grGetString
  Date: 14-Dec-97
  Implementor(s): pgj
  Description:
        Implements Glide 3.0 grGetString()
  Arguments:
    pname   -  parameter to be returned, e.g. GR_VENDOR
  
  Return:
    pointer to the selected string if successful
  -------------------------------------------------------------------*/
GR_DIENTRY(grGetString, const char *, (FxU32 pname))
{
#define FN_NAME "grGetString"

  switch(pname) {
  case GR_EXTENSION:
    return "CHROMARANGE TEXCHROMA TEXMIRROR PALETTE6666 FOGCOORD RESOLUTION";
  case GR_HARDWARE:
    return "Voodoo2";
  case GR_RENDERER:
    return "Glide";
  case GR_VENDOR:
    return "3Dfx Interactive";
  case GR_VERSION:
    return VERSIONSTR;
  default:
    return "ERROR";
  } /* end switch */
  
#undef FN_NAME
} /* grGetString */

/*-------------------------------------------------------------------
  Function: grGetRegistryOrEnvironmentStringExt
  Date: 4/17/2000       
  Implementor(s): atom
  Description: 

    This is here so the spooky code for finding the correct registry
    tweak path in 9x/NT/2K does not have to be duplicated in 3dfxogl.

  Arguments: char* to the name of the setting to check for.
  
  Return: char* to the requested entry either from the registry
          or the environment settings.  NULL on error.
  -------------------------------------------------------------------*/
GR_DIENTRY(grGetRegistryOrEnvironmentString, char*, (char* theEntry))
{
#define FN_NAME "grGetRegistryOrEnvironmentString"
  char*  retval ;

  retval = getenv(theEntry) ;

  return retval ;

#undef FN_NAME
} /* grGetRegistryOrEnvironmentString */

/*-------------------------------------------------------------------
  Function: grReset
  Date: 16-Dec-97
  Implementor(s): atai
  Description:

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grReset, FxBool, (FxU32 what))
{
#define FN_NAME "grReset"
  FxBool retVal;

  switch(what) {
  case GR_STATS_POINTS:
    _GlideRoot.stats.pointsDrawn = 0;
    retVal = FXTRUE;
    break;
  case GR_STATS_LINES:
    _GlideRoot.stats.linesDrawn = 0;
    retVal = FXTRUE;
    break;
  case GR_STATS_PIXELS:
#if defined(GLIDE3) && (GLIDE3_ALPHA)
    _grSstResetPerfStats();
#endif
    retVal = FXTRUE;
    break;
  case GR_STATS_TRIANGLES:
#if defined(GLIDE3) && (GLIDE3_ALPHA)
    {
      GR_DCL_GC;
      GR_DCL_HW;
      _grResetTriStats();
      GR_SET_EXPECTED_SIZE(sizeof(FxU32), 1);
      GR_SET(BROADCAST_ID, hw, nopCMD, 0x02);
      GR_CHECK_SIZE();
    }
#endif
    retVal = FXTRUE;
    break;
  case GR_VERTEX_PARAMETER:
#if defined(GLIDE3) && (GLIDE3_ALPHA)
    {
      GR_DCL_GC;
      gc->state.vData.vertexInfo.offset = 0;
      gc->state.vData.vertexInfo.mode = GR_MODE_DISABLE;
      gc->state.vData.zInfo.offset = 0;
      gc->state.vData.zInfo.mode = GR_MODE_DISABLE;
      gc->state.vData.wInfo.offset = 0;
      gc->state.vData.wInfo.mode = GR_MODE_DISABLE;
      gc->state.vData.aInfo.offset = 0;
      gc->state.vData.aInfo.mode = GR_MODE_DISABLE;
      gc->state.vData.rgbInfo.offset = 0;
      gc->state.vData.rgbInfo.mode = GR_MODE_DISABLE;
      gc->state.vData.pargbInfo.offset = 0;
      gc->state.vData.pargbInfo.mode = GR_MODE_DISABLE;
      gc->state.vData.st0Info.offset = 0;
      gc->state.vData.st0Info.mode = GR_MODE_DISABLE;
      gc->state.vData.st1Info.offset = 0;
      gc->state.vData.st1Info.mode = GR_MODE_DISABLE;
      gc->state.vData.qInfo.offset = 0;
      gc->state.vData.qInfo.mode = GR_MODE_DISABLE;
      gc->state.vData.q0Info.offset = 0;
      gc->state.vData.q0Info.mode = GR_MODE_DISABLE;
      gc->state.vData.q1Info.offset = 0;
      gc->state.vData.q1Info.mode = GR_MODE_DISABLE;
      gc->state.invalid &= ~vtxlayoutBIT;
      gc->state.vData.colorType = GR_FLOAT;
    }
#endif
    retVal = FXTRUE;
    break;
  default:
    retVal = FXFALSE;
  }
  return retVal;
#undef FN_NAME
} /* end grReset() */

/*-------------------------------------------------------------------
  Function: grGetProcAddress
  Date: 05-Jan-97
  Implementor(s): atai
  Description:

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
typedef struct {
    const char *name;
    GrProc      proc;
} GrExtensionTuple;

#if HAVE_TEXUS2
FX_ENTRY void FX_CALL txImgQuantize (char *dst, char *src, int w, int h, FxU32 format, FxU32 dither);
FX_ENTRY void FX_CALL txMipQuantize (void *pxMip, void *txMip, int fmt, FxU32 d, FxU32 comp);
FX_ENTRY void FX_CALL txPalToNcc (GuNccTable *ncc_table, const FxU32 *pal);
#endif

static GrExtensionTuple _extensionTable[] = {
    { "grChromaRangeModeExt", (GrProc)grChromaRangeMode },
    { "grChromaRangeExt", (GrProc)grChromaRange },
    { "grTexChromaModeExt", (GrProc)grTexChromaMode },
    { "grTexChromaRangeExt", (GrProc)grTexChromaRange },
    { "guQueryResolutionXYExt", (GrProc)guQueryResolutionXY },
    { "grGetRegistryOrEnvironmentStringExt", (GrProc)grGetRegistryOrEnvironmentString },
    { "grTexDownloadTableExt", (GrProc)grTexDownloadTableExt },
#if HAVE_TEXUS2
    { "txMipQuantize", (GrProc)txMipQuantize },
    { "txImgQuantize", (GrProc)txImgQuantize },
    { "txPalToNcc", (GrProc)txPalToNcc },
#endif
    { "grSetNumPendingBuffers", (GrProc)grSetNumPendingBuffers},
    { 0, 0 }
};

GR_DIENTRY(grGetProcAddress, GrProc, (char *procName))
{
#define FN_NAME "grGetProcAddress"
  GrExtensionTuple *tuple;

    if (!strcmp(procName, "grDrawTextureLineExt"))
      return (GrProc)_GlideRoot.deviceArchProcs.curLineProc;

    tuple = &_extensionTable[0];

    while( tuple->name ) {
        if ( !strcmp( procName, tuple->name ) ) {
            return tuple->proc;
        }
        tuple++;
    }

  return NULL;
#undef FN_NAME
} /* grGetProcAddress */

/*-------------------------------------------------------------------
  Function: grQueryResolution
  Date: 07-May-97
  Implementor(s): atai
  Description:

  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grQueryResolutions, FxI32, (const GrResolution *resTemplate, GrResolution *output))
{
#define FN_NAME "grQueryResolution"
  FxU32 size = 0;
  FxU32 min_res = GR_MIN_RESOLUTION, max_res = GR_MAX_RESOLUTION;
  FxU32 min_ref = GR_MIN_REFRESH, max_ref = GR_MAX_REFRESH;
  FxU32 min_col = GR_MIN_COLOR_BUF, max_col = GR_MAX_COLOR_BUF;
  FxU32 min_aux = GR_MIN_AUX_BUF, max_aux = GR_MAX_AUX_BUF;
  FxU32 i, j, k, l;
  GrHwConfiguration* hwc = &_GlideRoot.hwConfig;
  FxU32 fbmem = hwc->SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.fbRam << 20;
  FxU32 memSizeInPages = 0;
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 80);
  GDBG_INFO_MORE(gc->myLevel, "(0x%x, 0x%x)\n", resTemplate, output);
  
  if (hwc->SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.sliDetect)
    fbmem = fbmem << 1;
  memSizeInPages = fbmem >> 12;
  
  if ((resTemplate->resolution != GR_QUERY_ANY) && 
      (resTemplate->resolution >= GR_MIN_RESOLUTION) &&
      (resTemplate->resolution <= GR_MAX_RESOLUTION)) {
    min_res = resTemplate->resolution;
    max_res = resTemplate->resolution;
  }
  if ((resTemplate->refresh != GR_QUERY_ANY) &&
      (resTemplate->refresh >= GR_MIN_REFRESH) &&
      (resTemplate->refresh <= GR_MAX_REFRESH)) {
    min_ref = resTemplate->refresh;
    max_ref = resTemplate->refresh;
  }
  if ((resTemplate->numColorBuffers != GR_QUERY_ANY) &&
      (resTemplate->numColorBuffers >= GR_MIN_COLOR_BUF) &&
      (resTemplate->numColorBuffers <= GR_MAX_COLOR_BUF)) {
    min_col = resTemplate->numColorBuffers;
    max_col = resTemplate->numColorBuffers;
  }
  if ((resTemplate->numAuxBuffers != GR_QUERY_ANY) &&
      (resTemplate->numAuxBuffers >= GR_MIN_AUX_BUF) &&
        (resTemplate->numAuxBuffers <= GR_MAX_AUX_BUF)) {
    min_aux = resTemplate->numAuxBuffers;
    max_aux = resTemplate->numAuxBuffers;
  }
  for (i = min_res; i <= max_res; i++) {
    for (j = min_ref; j <= max_ref; j++) {
      if (_grResolutionRefresh[i][j]) {
        for (k = min_col; k <= max_col; k++) {
          for (l = min_aux; l <= max_aux; l++) {
            if ((_grResolutionRefresh[i][j] * 2 * (k + l) < fbmem) 
              && ((k+l)*_grMemOffset[i] < memSizeInPages)) {
              size += sizeof(GrResolution);
              if (output != NULL) {
                output->resolution      = i;
                output->refresh         = j;
                output->numColorBuffers = k;
                output->numAuxBuffers   = l;
                output++;
              }
            }
          }
        }
      }
    }
  }
  
  return size;
  
#undef FN_NAME
} /* grQueryResolution */

#endif /* GLIDE3 */
