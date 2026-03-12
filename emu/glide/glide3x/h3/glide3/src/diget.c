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
** Revision 1.1.1.1.6.5  2005/06/09 18:32:28  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.1.1.6.4  2005/05/25 08:56:23  jwrdegoede
** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
**
** Revision 1.1.1.1.6.3  2004/02/16 07:42:16  dborca
** grSetNumPendingBuffers visible with grGetProcAddress
**
** Revision 1.1.1.1.6.2  2003/12/08 13:13:11  dborca
** better Texus2 integration
**
** Revision 1.1.1.1.6.1  2003/11/07 13:38:38  dborca
** unite the clans
**
** Revision 1.1.1.1  1999/11/24 21:44:54  joseph
** Initial checkin for SourceForge
**
** 
** 5     5/24/99 2:48p Jamesb
** Added ptrLostContext to exported cmdTransport struct.
** 
** 4     4/06/99 3:54p Dow
** Alt tab again.
** 
** 68    3/19/99 11:26a Peter
** expose direct fifo for gl
** 
** 67    3/14/99 1:47p Peter
** temp implemntation of my surface extension extension, bottleneck
** through _grSstStatus
** 
** 66    3/11/99 7:14p Dow
** Resolution help
** 
** 65    3/11/99 6:38p Dow
** Resolution help
** 
** 64    3/05/99 10:34a Atai
** fixed fbmem in grQueryResolutions
** 
** 63    3/04/99 3:11p Atai
** fixed voodoo3 rev number.
** 
** 62    2/26/99 2:54p Atai
** added fbi and tmu revision
** 
** 61    2/18/99 3:07p Kcd
** I mean in this time, really.
** 
** 60    2/18/99 3:01p Kcd
** Surface Extensions disabled for non-Win32 systems.
** 
** 59    2/13/99 2:01p Dow
** Added code for new resolutions
** 
** 58    2/10/99 2:12p Peter
** forgot about refresh_any
** 
** 57    2/09/99 1:59p Peter
** fixed grGet(Num_boards) w/o any boards installed
** 
** 56    2/08/99 10:59a Peter
** constrained query should not clamp on out of bounds values
** 
** 55    2/02/99 4:38p Peter
** 16 byte texture alignmnet boundary
** 
** 54    1/20/99 10:54a Dow
** Voodoo 3 id for apps
** 
** 53    1/14/99 11:03a Dow
** Fixed gets for memory
** 
** 52    1/04/99 12:01p Peter
** turned on surface extension for nt
** 
** 51    12/14/98 6:19p Dow
** Fixed for current surface extension spec
** 
** 50    12/03/98 9:11p Dow
** More Debug messages
** 
** 49    12/03/98 9:37a Dow
** Added reserved enumerant for RAM type
** 
** 48    12/02/98 9:35p Dow
** QueryResolutions
** 
** 47    12/01/98 12:04p Atai
** fixed tri stats reset
** 
** 46    11/21/98 10:19a Atai
** fixed test37 grChromaRangeModeExt error and rename functions
** 
** 45    11/21/98 8:22a Atai
** fixed GR_MEMORY_UMA return value and length
** 
** 44    11/17/98 4:25p Atai
** fixed my previous check-in
** 
** 43    11/15/98 3:43a Atai
** fixed GR_MEMORY_TMU
** 
** 42    10/20/98 8:28p Peter
** shared files and fixed whackage
** 
** 41    10/09/98 12:18p Dow
** Fixed extension string
** 
** 40    10/09/98 10:20a Dow
** Fixed call to _grSstStatus
** 
** 39    10/09/98 10:05a Dow
** Fixed grGet() stuff
** 
** 38    10/09/98 9:40a Dow
** Fixed some grGet Stuff
** 
** 37    9/30/98 10:31a Atai
** fixed GR_IS_BUSY
** 
** 36    9/21/98 4:39p Atai
** fixed grGet
** 
** 35    9/02/98 10:36a Atai
** fixed grQueryResolution
** 
** 34    9/02/98 9:20a Peter
** update grGet(xxx_mem) values after open
** 
** 33    8/30/98 3:55p Dow
** Turned of Surface Extension for WNT
** 
** 32    8/29/98 2:28p Peter
** tls happiness w/ debug builds
** 
** 31    8/28/98 4:01p Jdt
** Export grSurfacereleaseContextExt.
** 
** 30    8/05/98 11:45p Atai
** 1. grBufferSwap argument FxU32
** 2. grGet return FxU32
** 
** 29    8/03/98 6:35a Jdt
** multi-thread changes
** 
** 28    8/02/98 5:01p Dow
** Glide Surface Extension
** 
** 27    7/18/98 1:45p Jdt
** Removed TACO_MEMORY_FIFO_HACK
** 
** 26    7/18/98 10:39a Dow
** Added Aux & Tex buffer
** 
** 25    7/16/98 8:27a Jdt
** 
** 24    7/16/98 8:14a Jdt
** fxcmd.h
** 
** 23    7/13/98 10:19p Jdt
** Added SURFACE extension.
** 
** Re-wrote grGetProcAddress to be table-driven.
** 
** 22    7/01/98 12:39p Jdt
** Added vertex layout size query
** 
** 21    6/25/98 2:17p Atai
** grGet (GR_NUM_BOARDS) can be called before grGlideInit
** 
** 20    6/24/98 2:43p Jdt
** Added query for state size.
** 
** 19    6/09/98 11:59a Atai
** 1. update glide header
** 2. fix cull mode
** 3. fix tri stats
** 
** 18    5/21/98 7:15p Atai
** fix q0 and q1 for clip coords
** 
** 17    5/19/98 6:43p Atai
** swap history
** 
** 16    5/15/98 4:02p Atai
** fogCoord and texchroma extension for Banshee
** 
** 15    5/11/98 4:14p Atai
** added frame buffer constrained query
** 
** 14    5/08/98 1:40p Peter
** merged Anthony's palette of Taco changes
** 
** 13    5/08/98 12:15p Peter
** expressions w/o sideffects are pretty lame
** 
** 11    5/05/98 2:22p Peter
** banshee is really banshee
** 
** 10    5/01/98 4:08p Peter
** fixed strings for banshee
** 
** 9     4/30/98 5:01p Peter
** first pass glide3 merge
** 
** 8     4/30/98 10:34a Peter
** merged w/ cvg again
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
#include "fxcmd.h"
#include "gsfc.h"
#include "rcver.h"

#include "fxinline.h"

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

  if (params == NULL) return FXFALSE;

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
      const FxU32 status = _grSstStatus();

      *params = (status & SST_PCIFIFO_FREE);
      *(params + 1) = status;

      retVal = plength;
    }
    break;
  case GR_FOG_TABLE_ENTRIES:
    if (plength == 4) {
      *params = kInternalFogTableEntryCount;
      retVal = plength;
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
  case GR_GLIDE_STATE_SIZE:
      if ( plength == 4 ) {
          *params = sizeof( GrState );
          retVal = plength;
      }
      break;
  case GR_GLIDE_VERTEXLAYOUT_SIZE:
      if ( plength == 4 ) {
          *params = sizeof( GrVertexLayout );
          retVal  = plength;
      }
      break;
  case GR_IS_BUSY:
    if (plength == 4) {
      *params = ((_grSstStatus() & SST_BUSY) != 0);
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
    if (plength == 4) {
      GR_DCL_GC;

      /* Default to the whole memory size until the application inits
       * buffers etc then subtract off start of the first color buffer.
       */
      *params = (gc->fbuf_size - 2) << 20;
      if (gc->open)
        *params = ((gc->bInfo->h3Mem << 20) - gc->fbOffset);
      retVal = plength;
    }
    break;
  case GR_MEMORY_TMU:
    if (plength == 4) {
      GR_DCL_GC;

      if (gc->open)
        *params = (gc->tmu_state[0].total_mem);
      else
        *params = 2 << 20;
      retVal = plength;
    }
    break;
  case GR_MEMORY_UMA:
    if (plength == 4) {
      GR_DCL_GC;

      *params = gc->bInfo->h3Mem << 20;

      retVal = plength;
    }
    break;
  case GR_NUM_BOARDS:
    if (plength == 4) {
      *params = (_grSstDetectResources() ? _GlideRoot.hwConfig.num_sst : 0);
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
  case GR_REVISION_TMU:
    if (plength == 4) {
      GR_DCL_GC;

      switch (gc->bInfo->pciInfo.deviceID) {
      case 3:
        *params = 0x1000;  /* banshee */
        break;
      case 4:
      case 5:
        *params = 0x11100;  /* voodoo3 = banshee + voodoo2 */
        break;
      default:
        *params = 0;
      }
        
      retVal = plength;
    }
    break;
  case GR_STATS_LINES:
    if (plength == 4) {
      GR_DCL_GC;
  
      *params = gc->stats.linesDrawn;
      retVal = plength;
    }
    break;
  case GR_STATS_PIXELS_AFUNC_FAIL:
  {
    GR_DCL_GC;

    if ((((SstRegs *)gc->sstRegs)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->sstRegs)->stats.fbiAfuncFail);
      retVal = plength;
    }
    break;
  }
  case GR_STATS_PIXELS_CHROMA_FAIL:
  {
    GR_DCL_GC;

    if ((((SstRegs *)gc->sstRegs)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->sstRegs)->stats.fbiChromaFail);
      retVal = plength;
    }
    break;
  }
  case GR_STATS_PIXELS_DEPTHFUNC_FAIL:
  {
    GR_DCL_GC;
  
    if ((((SstRegs *)gc->sstRegs)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->sstRegs)->stats.fbiZfuncFail);
      retVal = plength;
    }
    break;
  }
  case GR_STATS_PIXELS_IN:
  {
    GR_DCL_GC;

    if ((((SstRegs *)gc->sstRegs)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->sstRegs)->stats.fbiPixelsIn);
      retVal = plength;
    }
    break;
  }
  case GR_STATS_PIXELS_OUT:
  {
    GR_DCL_GC;

    if ((((SstRegs *)gc->sstRegs)) && (plength == 4)) {
      *params = GR_GET(((SstRegs *)gc->sstRegs)->stats.fbiPixelsOut);
      retVal = plength;
    }
    break;
  }
  case GR_STATS_POINTS:
    if (plength == 4) {
      GR_DCL_GC;

      *params = gc->stats.pointsDrawn;
      retVal = plength;
    }
    break;
  case GR_STATS_TRIANGLES_IN:
    if (plength == 4) {
      GR_DCL_GC;

      *params = gc->stats.trisProcessed;
      retVal = plength;       /* XXX TBD */
    }
    break;
  case GR_STATS_TRIANGLES_OUT:
    if (plength == 4) {
      GR_DCL_GC;

      *params = GR_GET(((SstRegs *)gc->sstRegs)->fbiTrianglesOut) - gc->stats.othertrisDrawn;
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
      GR_DCL_GC;
      FxU32 sh = GR_GET(((SstRegs *)gc->sstRegs)->fbiSwapHistory), i;

      for (i = 0; i < 8; i++) {
        *(params +i) = sh & 0xf;
        sh = sh >> 4;
      }
      retVal = plength;
    }
    break;
  case GR_SUPPORTS_PASSTHRU:
    if (plength == 4) {
      *params = FXFALSE;
      retVal = plength;
    }
    break;
  case GR_TEXTURE_ALIGN:
    if (plength == 4) {
      *params = SST_TEXTURE_ALIGN;
      retVal = plength;
    }
    break;
  case GR_VIDEO_POSITION:
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
    if (plength == 8) {
      *params = 
      *(params + 1) = 0;

      retVal = plength;
    }
#endif
  break;
  case GR_VIEWPORT:
    if (plength == 16) {
      GR_DCL_GC;

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

  case GR_SURFACE_SIZE:
    if (plength == 4) {
      *params = sizeof(hwcBufferDesc);
      retVal = plength;
    }
    break;

  case GR_SURFACE_TEXTURE:
    if (plength == sizeof(unsigned long)) {
      unsigned long *p = (unsigned long *)params;
      GR_DCL_GC;

      *p = (unsigned long) &gc->tBuffer;
      retVal = plength;
    }
    break;

  case GR_MEMTYPE:
    if (plength == 4) {
      GR_DCL_GC;

      *params = (FxU32)gc->bInfo->sdRAM;
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
  const char *rv = "ERROR";
  
  switch(pname) {
  case GR_EXTENSION:
    /* NOTE!: leave a leading and trailing spaces on the list so that 
       apps can disambiguate substrings */
    rv = " CHROMARANGE TEXCHROMA TEXMIRROR PALETTE6666 FOGCOORD SURFACE COMMAND_TRANSPORT ";
    break;
  case GR_HARDWARE:
    {
      GR_DCL_GC;
      
      if (gc->bInfo->pciInfo.deviceID == 3)
        rv = "Voodoo Banshee (tm)";
      else
        rv = "Voodoo3 (tm)";
    }
    break;
  case GR_RENDERER:
    rv = "Glide";
    break;
  case GR_VENDOR:
    rv = "3Dfx Interactive";
    break;
  case GR_VERSION:
    rv = VERSIONSTR;
    break;
  } /* end switch */
  return rv;
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
GR_EXT_ENTRY(grGetRegistryOrEnvironmentString, char*, (char* theEntry))
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
  GR_DCL_GC;

  switch(what) {
  case GR_STATS_POINTS:
    gc->stats.pointsDrawn = 0;
    retVal = FXTRUE;
    break;
  case GR_STATS_LINES:
    gc->stats.linesDrawn = 0;
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
  Implementor(s): atai, jdt
  Description:
  
  Return a pointer to a glide extension function. Note:
  all functions are typed __stdcall.  There is an inconsistency
  here in that the GetProcAddr() names for all glide functions
  are decorated for __stdcall, yet there are not.

  This function must work outside of grSstWinOpen/Close.

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
    { "grGetRegistryOrEnvironmentStringExt",    (GrProc)grGetRegistryOrEnvironmentString },
    { "grChromaRangeModeExt",    (GrProc)grChromaRangeMode },
    { "grChromaRangeExt",   (GrProc)grChromaRange },
    { "grTexChromaModeExt", (GrProc)grTexChromaMode },
    { "grTexChromaRangeExt",(GrProc)grTexChromaRange },
#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
    { "grSurfaceCreateContextExt", (GrProc)grSurfaceCreateContext },
    { "grSurfaceReleaseContextExt", (GrProc)grSurfaceReleaseContext },
    { "grSurfaceSetRenderingSurfaceExt", (GrProc)grSurfaceSetRenderingSurface },
    { "grSurfaceCalcTextureWHDExt", (GrProc)grSurfaceCalcTextureWHD },
    { "grSurfaceSetAuxSurfaceExt", (GrProc)grSurfaceSetAuxSurface },
    { "grSurfaceSetTextureSurfaceExt", (GrProc)grSurfaceSetTextureSurface },
    { "grDeviceQueryExt", (GrProc)grDeviceQuery },
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */
    { "grCommandTransportInfoExt2", (GrProc)_grCommandTransportInfo },
    { "grCommandTransportMakeRoomExt2", (GrProc)_grCommandTransportMakeRoom },
    /* POINTCAST */
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
    GrProc           rv;

    tuple = &_extensionTable[0];
    rv    = 0;

    while( tuple->name ) {
        if ( !strcmp( procName, tuple->name ) ) {
            rv = tuple->proc;
            break;
        }
        tuple++;
    }
    return rv;
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
GR_DIENTRY(grQueryResolutions, FxI32, (const GlideResolution *resTemplate, GlideResolution *output))
{
#define FN_NAME "grQueryResolution"
  FxU32 
    size = 0;
  FxU32 
    min_res = GR_MIN_RESOLUTION, 
    max_res = GR_MAX_RESOLUTION,
    min_ref = GR_MIN_REFRESH, 
    max_ref = GR_MAX_REFRESH,
    min_col = GR_MIN_COLOR_BUF, 
    max_col = GR_MAX_COLOR_BUF,
    min_aux = GR_MIN_AUX_BUF, 
    max_aux = GR_MAX_AUX_BUF;
  FxU32 
    i, j, k, l;
  extern ResEntry _resTable[];

  GR_DCL_GC;

  FxU32 
    /* 0x10000 is the minimum interesting FIFO size */
    fbmem = (gc->bInfo->h3Mem << 20) - gc->bInfo->min_tramSize - 0x10000;

#if GDBG_INFO_ON
  static char *resNames[] = {
    "GR_RESOLUTION_320x200", 
    "GR_RESOLUTION_320x240", 
    "GR_RESOLUTION_400x256", 
    "GR_RESOLUTION_512x384", 
    "GR_RESOLUTION_640x200", 
    "GR_RESOLUTION_640x350", 
    "GR_RESOLUTION_640x400", 
    "GR_RESOLUTION_640x480", 
    "GR_RESOLUTION_800x600", 
    "GR_RESOLUTION_960x720", 
    "GR_RESOLUTION_856x480", 
    "GR_RESOLUTION_512x256", 
    "GR_RESOLUTION_1024x768", 
    "GR_RESOLUTION_1280x1024", 
    "GR_RESOLUTION_1600x1200", 
    "GR_RESOLUTION_400x300", 
    "GR_RESOLUTION_1152x864", 
    "GR_RESOLUTION_1280x960", 
    "GR_RESOLUTION_1600x1024", 
    "GR_RESOLUTION_1792x1344", 
    "GR_RESOLUTION_1856x1392", 
    "GR_RESOLUTION_1920x1440", 
    "GR_RESOLUTION_2048x1536", 
    "GR_RESOLUTION_2048x2048",
  };
#endif

  GDBG_INFO(80, FN_NAME"(0x%x, 0x%x)\n", resTemplate, output);
  
  if (resTemplate->resolution != GR_QUERY_ANY) {
    if ((resTemplate->resolution >= GR_MIN_RESOLUTION) &&
        (resTemplate->resolution <= GR_MAX_RESOLUTION)) {
      min_res = resTemplate->resolution;
      max_res = resTemplate->resolution;
    } else {
      goto __errExit;
    }
  }
  if (resTemplate->refresh != GR_QUERY_ANY) {
    if ((resTemplate->refresh >= GR_MIN_REFRESH) &&
        (resTemplate->refresh <= GR_MAX_REFRESH)) {
      min_ref = resTemplate->refresh;
      max_ref = resTemplate->refresh;
    } else if (resTemplate->refresh != GR_REFRESH_NONE) {
      goto __errExit;
    }
  }
  if (resTemplate->numColorBuffers != GR_QUERY_ANY) {
    if ((resTemplate->numColorBuffers >= GR_MIN_COLOR_BUF) &&
        (resTemplate->numColorBuffers <= GR_MAX_COLOR_BUF)) {
      min_col = resTemplate->numColorBuffers;
      max_col = resTemplate->numColorBuffers;
    } else {
      goto __errExit;
    }
  }
  if (resTemplate->numAuxBuffers != GR_QUERY_ANY) {
    if ((resTemplate->numAuxBuffers >= GR_MIN_AUX_BUF) &&
        (resTemplate->numAuxBuffers <= GR_MAX_AUX_BUF)) {
      min_aux = resTemplate->numAuxBuffers;
      max_aux = resTemplate->numAuxBuffers;
    } else {
      goto __errExit;
    }
  }

  for (i = min_res; i <= max_res; i++) {
#if GDBG_INFO_ON
    GDBG_INFO(80, FN_NAME "Resolution = %s\n", resNames[i]);
#endif
    for (j = min_ref; j <= max_ref; j++) {
      FxBool resSuported;
      GDBG_INFO(80, FN_NAME ":  _grResolutionRefresh passed\n");
      resSuported = hwcResolutionSupported(gc->bInfo, i);
      GDBG_INFO(80, FN_NAME ":  hwcResolutionSupported returned %s\n",
                resSuported ? "FXTRUE" : "FXFALSE");
      if (resSuported) {
        GDBG_INFO(80, FN_NAME ":  hwcResolutionSupported passed\n");
        for (k = min_col; k <= max_col; k++) {
          for (l = min_aux; l <= max_aux; l++) {
            if (
                /*
                   NB:  This calculation does not account for the tile
                   restrictions or the padding, but the amount of
                   memory configurations hide this, so we get away
                   with it.
                 */
                (_resTable[i].xres * _resTable[i].yres * 2 * (k + l)) <
                fbmem) {
              GDBG_INFO(80, FN_NAME ":  Sufficient memory\n");
              size += 16;
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

 __errExit:
  return size;
  
#undef FN_NAME
} /* grQueryResolution */

#endif /* GLIDE3 */
