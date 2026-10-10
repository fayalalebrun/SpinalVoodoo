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
** $Log: 
**  22   3dfx      1.17.1.0.1.210/11/00 Brent           Forced check in to enforce
**       branching.
**  21   3dfx      1.17.1.0.1.109/15/00 troy thornton   grqueryresolutions now
**       checks the refreshrate of the monitor before returning a valid screen mode
** 
**  20   3dfx      1.17.1.0.1.008/29/00 Jonny Cochrane  Some 8x FSAA code
**  19   3dfx      1.17.1.0    06/07/00 Daoxiang Gong   Fixes PRS 14363. Make sure
**       grGet returns proper color bit depth info.
**  18   3dfx      1.17        04/17/00 Adam Briggs     added
**       grGetRegistryOrEnvironmentStringExt call so that OpenGL has an easy way to
**       read environment settings from the registry.  To save you the trouble of
**       asking, the answer is no, I could not think of a longer name for the
**       function.  I did try, though.
**  17   3dfx      1.16        04/10/00 Larry  warner   Report max texture size
**       2048 for Napalm and 256 otherwise.
**  16   3dfx      1.15        04/06/00 Larry  warner   So much for the palindrome.
**  15   3dfx      1.14        03/31/00 Kenneth Dyke    Get number of chips from
**       bInfo so we report back the right kind of card. (Cosmetic).
**  14   3dfx      1.13        02/22/00 Kenneth Dyke    Base min/max z and w buffer
**       values on actual pixel depth, and not on the hardware we're on.
**  13   3dfx      1.12        02/10/00 Adam Briggs     grGet(GR_NUM_FB) now
**       returns the number of chips on a board.
**  12   3dfx      1.11        01/31/00 Adam Briggs     changed the IS_NAPALM macro
**       to cooperate with the display driver version of the same
**  11   3dfx      1.10        01/31/00 Adam Briggs     Changed all device ID magic
**       numbers to use those defined in fxhal.h & added IS_NAPALM macro to test
**       against device ID range
**  10   3dfx      1.9         01/30/00 Adam Briggs     get napalm status correctly
**  9    3dfx      1.8         01/18/00 Kenneth Dyke    Fix query value for maximum
**       texture size.
**  8    3dfx      1.7         01/04/00 Adam Briggs     changed grGetGammaTable to
**       be an extension called grGetGammaTableExt
**  7    3dfx      1.6         12/13/99 Adam Briggs     Made
**       grGetString(GR_HARDWARE) return Voodoo4 or VoodooV instead of Napalm
**  6    3dfx      1.5         12/10/99 Leo Galway      Removed previous hi-res
**       mode information for Glide3. These modes were only necessary for
**       Cornerstone (or future hi-res) support in RT4.2 source branch and
**       proceeded to break the V3 and V2 builds (from 3dfx view), hence they have
**       been removed.
**  5    3dfx      1.4         12/08/99 Leo Galway      Added mode information for
**       1600x1280, 1792x1440, 1920x1080, 1920x1200, 2046x1536 (as a result of
**       glide being tested with Cornerstone modes). Although not all of these
**       modes are currently capable under Glide, their inclusion prevents Glide
**       apps from displaying in incorrect modes when these hi-res modes are
**       selected. Search for SUSTAINED_ENGINEERING_CHANGE_BEGIN. 
**  4    3dfx      1.3         10/26/99 Anthony tai     workaround for calling
**       grGetString before the gc exist
**  3    3dfx      1.2         09/28/99 Anthony tai     workaround for dct bug
**  2    3dfx      1.1         09/22/99 Adam Briggs     Added
**       grConstantColorValueExt
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 93    9/09/99 4:19p Adamb
** Added TEXTUREBUFFER to GR_EXTENSION string
** 
** 92    8/23/99 3:14p Kcd
** New MacOS 8 surface stuff.
** 
** 91    8/19/99 5:57p Atai
** new string for napalm ext
** 
** 90    7/23/99 2:00p Atai
** change tbuffer interface to grTBufferWriteMaskExt
** 
** 89    7/22/99 1:18p Atai
** added grTBufferMaskExt
** 
** 88    7/18/99 1:59p Atai
** added grAlphaBlendFunctionExt
** 
** 87    7/14/99 5:07p Atai
** fixed stencil interface and some cc/ac stuff
** 
** 86    7/14/99 9:39a Atai
** direct register write for glide3x
** test04 can do 4 sample aa (2 chips)
** 
** 85    7/08/99 8:48p Atai
** stencil interface update
** 
** 84    7/06/99 5:23p Atai
** fake resolution
** 
** 83    7/06/99 2:44p Atai
** added grColorMaskExt
** 
** 82    7/01/99 6:42p Larryw
** Added TEXFMT extension.
** 
** 81    6/24/99 7:11p Atai
** added STENCIL and COMBINE extension
** 
** 80    6/22/99 2:56p Larryw
** Added grGetString placeholder for Napalm.
** Report "3dfx Interactive" instead of "3Dfx Interactive" as the vendor.
** 
** 79    6/13/99 6:16p Atai
** use "PIXEXT" for winopen ext
** 
** 78    6/04/99 11:00a Atai
** added stencil functions
** 
** 77    6/03/99 4:47p Atai
** modify grGet(GR_WDEPTH_MIN_MAX) and grGet(GR_ZDEPTH_MIN_MAX)
** 
** 76    6/01/99 2:34p Atai
** Added "NAPALM" Extension as a place holder. How do we want to call the
** new extension?
** 
** 75    5/24/99 2:49p Jamesb
** Added ptrLostContext field to exported command transport struct.
** 
** 74    5/19/99 3:55p Denis
** 
** 73    5/19/99 3:48p Denis
** 
** 72    5/19/99 12:45p Denis
** First check in of the TEXTUREBUFFER extension.
** Contains both the texture color buffer and texture aux. buffer
** extensions
** that allows to specify a piece of texture memory as a rendering target
** and/or a piece of texture memory as the aux. buffer.
** 
** Probably a non conventional check in, in the sense that the API
** isn't entirely frozen / specified yet. To ease whoever's job it will be
** to complete the extension, I've added a "tbext" comment
** everywhere I made a modification. These should go away
** once the API is frozen.
**  
** 
** 71    4/16/99 2:45p Kcd
** Mac Surface extensions.
** 
** 70    4/10/99 1:23p Atai
** fixed code to compile in packet fifo mode 
** 
** 69    4/07/99 7:18p Atai
** added uma extension
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

#if (GLIDE_PLATFORM & GLIDE_SST_SIM)
#if HAL_CSIM
#include <csim.h>
#else
#include <gsim.h>
#endif 
#endif

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
	  GR_DCL_GC;
      retVal = plength;
      *params = gc->bInfo->h3pixelSize << 3 ;
    }
    break;
  case GR_BITS_RGBA:
    if (plength == 16) {
		GR_DCL_GC;
        retVal = plength;
		switch(gc->grPixelFormat)
		{
    	case GR_PIXFMT_RGB_565:
		case GR_PIXFMT_AA_2_RGB_565:
		case GR_PIXFMT_AA_4_RGB_565:
		case GR_PIXFMT_AA_8_RGB_565:	/* 8xaa */
			*params = 5;
      		*(params+1) = 6;
      		*(params+2) = 5;
      		*(params+3) = 0;
      		break;
    	case GR_PIXFMT_ARGB_1555:
		case GR_PIXFMT_AA_2_ARGB_1555:
		case GR_PIXFMT_AA_4_ARGB_1555:
		case GR_PIXFMT_AA_8_ARGB_1555: 	/* 8xaa */
			*params = 5;
      		*(params+1) = 5;
      		*(params+2) = 5;
      		*(params+3) = 1;
      		break;
    	case GR_PIXFMT_ARGB_8888:
		case GR_PIXFMT_AA_2_ARGB_8888:
		case GR_PIXFMT_AA_4_ARGB_8888:
		case GR_PIXFMT_AA_8_ARGB_8888:	/* 8xaa */
			*params = 8;
      		*(params+1) = 8;
      		*(params+2) = 8;
      		*(params+3) = 8;
      		break;    
    	default:
          break;
		}
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
      *params = _grSstIsBusy() ;
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
      GR_DCL_GC;
      if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
        *params = 2048;
      } else {
        *params = 256;
      }
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
#ifdef GLIDE_INIT_HWC
      if (gc->open)
        *params = ((gc->bInfo->h3Mem << 20) - gc->fbOffset);
      else
#endif
        *params = (gc->fbuf_size - 2) << 20;
      
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

#ifdef GLIDE_INIT_HWC
        *params = gc->bInfo->h3Mem << 20;
#endif

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
      GR_DCL_GC;

      if ((gc) &&
          (gc->chipCount))
        *params = gc->chipCount ;
       else
        *params = 1 ;

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

#ifdef GLIDE_INIT_HWC
      switch (gc->bInfo->pciInfo.deviceID) {
      case SST_DEVICE_ID_H3:
        *params = 0x1000;  /* banshee */
        break;
      case SST_DEVICE_ID_H4_OEM:
      case SST_DEVICE_ID_H4:
        *params = 0x11100;  /* voodoo3 = banshee + voodoo2 */
        break;
      default:
          *params = 0;
      }
#endif
        
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
      GR_DCL_GC;
      *params = SST1_WDEPTHVALUE_NEAREST;
      *(params+1) = (gc->grPixelSize <= 2) ?
        SST1_WDEPTHVALUE_FARTHEST : NAPALM_WDEPTHVALUE_FARTHEST;
      retVal = plength;
    }
    break;
  case GR_ZDEPTH_MIN_MAX:
    if (plength == 8) {
      GR_DCL_GC;
      *params = (gc->grPixelSize <= 2) ? 
        SST1_ZDEPTHVALUE_NEAREST : NAPALM_ZDEPTHVALUE_NEAREST;
      *(params+1) = SST1_ZDEPTHVALUE_FARTHEST;
      retVal = plength;
    }
    break;

  case GR_SURFACE_SIZE:
    if (plength == 4) {
#ifdef GLIDE_INIT_HWC
      *params = sizeof(hwcBufferDesc);
      retVal = plength;
#endif
    }
    break;

  case GR_SURFACE_TEXTURE:
    if (plength == sizeof(unsigned long)) {
      unsigned long *p = (unsigned long *)params;
      GR_DCL_GC;

#ifdef GLIDE_INIT_HWC
      *p = (unsigned long) &gc->tBuffer;
      retVal = plength;
#endif
    }
    break;

  case GR_MEMTYPE:
    if (plength == 4) {
      GR_DCL_GC;

#ifdef GLIDE_INIT_HWC
      *params = (FxU32)gc->bInfo->sdRAM;
      retVal = plength;
#endif
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
#if GLIDE_POINTCAST_PALETTE
#define POINTCAST_EXT_STR "POINTCAST "
#else
#define POINTCAST_EXT_STR ""
#endif
#ifdef QUERY_EXTENSION_SUPPORTED
#define QUERY_EXT_STR "QUERY "
#else
#define QUERY_EXT_STR ""
#endif
#ifdef MULTIRENDERING
#define MULTIRENDERING_STR "MULTIRENDERING "
#else
#define MULTIRENDERING_STR ""
#endif

#define BASE_EXT_STR	"CHROMARANGE TEXCHROMA TEXMIRROR TEXUMA PALETTE6666 FOGCOORD SURFACE COMMAND_TRANSPORT TEXTUREBUFFER GETGAMMA GETREGISTRY ALPHAFOG "
#define NAPALM_EXT_STR	"PIXEXT COMBINE TEXFMT "

GR_DIENTRY(grGetString, const char *, (FxU32 pname))
{
#define FN_NAME "grGetString"
  const char *rv = "ERROR";
  
  switch(pname) {
  case GR_EXTENSION:
    /* NOTE!: leave a leading and trailing spaces on the list so that 
       apps can disambiguate substrings */
    {
      GR_DCL_GC;
      
      if (!gc)  /* workaround null gc bug */
        return rv;
      if (!IS_NAPALM(gc->bInfo->pciInfo.deviceID))
        rv = " " BASE_EXT_STR QUERY_EXT_STR POINTCAST_EXT_STR;
      else
        rv = " " BASE_EXT_STR NAPALM_EXT_STR QUERY_EXT_STR POINTCAST_EXT_STR MULTIRENDERING_STR;
    }
    break;
  case GR_HARDWARE:
    {
      GR_DCL_GC;
      
#ifdef GLIDE_INIT_HWC
      if (!gc)  /* workaround null gc bug */
        return rv;
      switch (gc->bInfo->pciInfo.deviceID) {
      case SST_DEVICE_ID_H3:
        rv = "Voodoo Banshee (tm)";
        break;
      case SST_DEVICE_ID_H4:
      case SST_DEVICE_ID_H4_OEM:
        rv = "Voodoo3 (tm)";
        break;
      default:
        if (IS_NAPALM(gc->bInfo->pciInfo.deviceID)) {
          if (gc->bInfo->pciInfo.realNumChips == 4) {
            rv = "Voodoo5 6000 (tm)" ;
          } else if (gc->bInfo->pciInfo.realNumChips == 2) {
            rv = "Voodoo5 5500 (tm)" ;
          } else if (gc->bInfo->pciInfo.realNumChips >= 2) {
            rv = "Voodoo5 (tm)" ;
          } else {
            rv = "Voodoo4 (tm)" ;
          }
        }
        /* default: "ERROR" */
        break;
      } 
#endif
    }
    break;
  case GR_RENDERER:
    rv = "Glide";
    break;
  case GR_VENDOR:
    rv = "3dfx Interactive";
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

#if !defined(GLIDE_INIT_HAL)
  retval = hwcGetenv(theEntry) ;
#else
  retval = getenv(theEntry) ;
#endif

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
    { "grGetGammaTableExt",    (GrProc)grGetGammaTable },
    { "grChromaRangeModeExt",    (GrProc)grChromaRangeMode },
    { "grChromaRangeExt",   (GrProc)grChromaRange },
    { "grTexChromaModeExt", (GrProc)grTexChromaMode },
    { "grTexChromaRangeExt",(GrProc)grTexChromaRange },
        /* tbext */
    { "grTextureBufferExt",(GrProc)grTextureBuffer },
    { "grTextureAuxBufferExt",(GrProc)grTextureAuxBuffer },
    { "grAuxBufferExt",(GrProc)grAuxBuffer },
#if (GLIDE_PLATFORM & (GLIDE_OS_WIN32|GLIDE_OS_MACOS))
    { "grSurfaceCreateContextExt", (GrProc)grSurfaceCreateContext },
    { "grSurfaceReleaseContextExt", (GrProc)grSurfaceReleaseContext },
    { "grSurfaceSetRenderingSurfaceExt", (GrProc)grSurfaceSetRenderingSurface },
    { "grSurfaceCalcTextureWHDExt", (GrProc)grSurfaceCalcTextureWHD },
    { "grSurfaceSetAuxSurfaceExt", (GrProc)grSurfaceSetAuxSurface },
    { "grSurfaceSetTextureSurfaceExt", (GrProc)grSurfaceSetTextureSurface },
    { "grDeviceQueryExt", (GrProc)grDeviceQuery },
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */
#if defined( USE_PACKET_FIFO )
    { "grCommandTransportInfoExt2", (GrProc)_grCommandTransportInfo },
    { "grCommandTransportMakeRoomExt2", (GrProc)_grCommandTransportMakeRoom },
#endif
#if (GLIDE_PLATFORM & GLIDE_OS_MACOS)
    { "grSurfaceCreateExt", (GrProc)grSurfaceCreate },
    { "grSurfaceReleaseExt", (GrProc)grSurfaceRelease },
    { "grSurfaceGetDescExt", (GrProc)grSurfaceGetDesc },   
#endif /* (GLIDE_PLATFORM & GLIDE_OS_MACOS) */
#ifdef FX_GLIDE_NAPALM
    { "grSstWinOpenExt", (GrProc)grSstWinOpenExt },
    { "grStencilFuncExt", (GrProc)grStencilFunc },
    { "grStencilMaskExt", (GrProc)grStencilMask },
    { "grStencilOpExt", (GrProc)grStencilOp },
    { "grLfbConstantStencilExt", (GrProc)grLfbConstantStencil },
    { "grBufferClearExt", (GrProc)grBufferClearExt },
    { "grColorCombineExt", (GrProc)grColorCombineExt },
    { "grAlphaCombineExt", (GrProc)grAlphaCombineExt },
    { "grTexColorCombineExt", (GrProc)grTexColorCombineExt },
    { "grTexAlphaCombineExt", (GrProc)grTexAlphaCombineExt },
    { "grConstantColorValueExt", (GrProc)grConstantColorValueExt },
    { "grColorMaskExt", (GrProc)grColorMaskExt },
    { "grAlphaBlendFunctionExt", (GrProc)grAlphaBlendFunctionExt },
    { "grTBufferWriteMaskExt", (GrProc)grTBufferWriteMaskExt },
#endif /* FX_GLIDE_NAPALM */
    /* QUERY */
#ifdef QUERY_EXTENSION_SUPPORTED
    { "grSstQueryBoards", (GrProc)grSstQueryBoards },
    { "grSstQueryHardware", (GrProc)grSstQueryHardware },
#endif
    /* POINTCAST */
#if GLIDE_POINTCAST_PALETTE
    { "grTexDownloadTableExt", (GrProc)grTexDownloadTableExt },
    { "grTexDownloadTablePartialExt", (GrProc)grTexDownloadTablePartialExt },
    { "grTexNCCTableExt", (GrProc)grTexNCCTableExt },
#endif
#if HAVE_TEXUS2
    { "txMipQuantize", (GrProc)txMipQuantize },
    { "txImgQuantize", (GrProc)txImgQuantize },
    { "txPalToNcc", (GrProc)txPalToNcc },
#endif
    { "grTexDownloadMipMapLevelPartialRowExt", (GrProc)grTexDownloadMipMapLevelPartialRowExt },
    { "grSetNumPendingBuffers", (GrProc)grSetNumPendingBuffers },
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
#ifdef GLIDE_INIT_HWC
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
    "GR_RESOLUTION_2048x2048"   
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
      resSuported = hwcResolutionSupported(gc->bInfo, i, j);
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
#else /* GLIDE_INIT_HWC */
  FxU32 size = 16*4;
  if (output != NULL) {
    output->resolution      = GR_RESOLUTION_640x480;
    output->refresh         = GR_REFRESH_60Hz;
    output->numColorBuffers = 2;
    output->numAuxBuffers   = 1;
    output++;
    output->resolution      = GR_RESOLUTION_640x480;
    output->refresh         = GR_REFRESH_60Hz;
    output->numColorBuffers = 2;
    output->numAuxBuffers   = 0;
    output++;
    output->resolution      = GR_RESOLUTION_640x480;
    output->refresh         = GR_REFRESH_60Hz;
    output->numColorBuffers = 1;
    output->numAuxBuffers   = 1;
    output++;
    output->resolution      = GR_RESOLUTION_640x480;
    output->refresh         = GR_REFRESH_60Hz;
    output->numColorBuffers = 1;
    output->numAuxBuffers   = 0;
  }
  return size;
#endif  
#undef FN_NAME
} /* grQueryResolution */

#endif /* GLIDE3 */
