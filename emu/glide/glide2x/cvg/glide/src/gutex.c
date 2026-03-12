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
** 
** 22    2/20/98 5:31p Peter
** crybaby glide
** 
** 21    2/20/98 11:00a Peter
** removed glide3 from glid2 tree
 * 
 * 20    1/07/98 10:22a Peter
 * lod dithering env var
 * 
 * 19    1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 18    12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 17    12/02/97 9:49a Dow
 * rid the world of the "Setting the same state twice" message
 * 
 * 16    11/20/97 6:59p Dow
 * Fixed baseAddress for Banshee (the Devil's Own Test still uses guTex
 * Stuff).
 * 
 * 15    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 14    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 13    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 12    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 11    9/04/97 3:32p Peter
 * starting grouping serial reg writes
 * 
 * 10    6/06/97 10:47a Peter
 * texture downloading, fixed 640x480 dimension, changed cvg dep to be the
 * same as sst1
 * 
 * 9     5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 8     5/21/97 6:05a Peter
 * 
 * 7     3/04/97 9:08p Dow
 * 
 * 6     12/23/96 1:37p Dow
 * chagnes for multiplatform glide
 * 
 * 5     11/18/96 9:14a Sellers
 * added checking for crossing of 2 MByte texture boundry
**
*/

#include <string.h>
#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

/* externals from gtex.c and gtexdl.c */
extern FxU32 _grMipMapHostSize[][16];
extern FxU32 _gr_aspect_index_table[];
extern FxU32 _gr_aspect_xlate_table[];
extern FxU32 _gr_evenOdd_xlate_table[];

/*---------------------------------------------------------------------------
** guTexSource
*/

GR_ENTRY(guTexSource, void, (GrMipMapId_t mmid))
{
#define FN_NAME "guTexSource"
  FxU32               texMode, tLod;
  FxU32               oldtexMode;
  FxU32               baseAddress;
  int                 tmu;
  const GrMipMapInfo *mminfo;

  GR_BEGIN_NOFIFOCHECK("guTexSource",99);
  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",mmid);

  /* Make sure that mmid is not NULL */
  if (mmid == GR_NULL_MIPMAP_HANDLE) return;

  /* get a pointer to the relevant GrMipMapInfo struct */
  mminfo = &gc->mm_table.data[mmid];
  tmu = mminfo->tmu;

  GR_CHECK_TMU(FN_NAME, tmu);

  gc->state.current_mm[tmu] = mmid;

  /* Set up new glide state for this mmid */
  gc->state.tmu_config[tmu].mmMode =  mminfo->mipmap_mode;
  gc->state.tmu_config[tmu].smallLod = mminfo->lod_min;
  gc->state.tmu_config[tmu].largeLod = mminfo->lod_max;
  gc->state.tmu_config[tmu].evenOdd =  mminfo->odd_even_mask;
  gc->state.tmu_config[tmu].nccTable = 0;

  /* Set up base address, texMode, and tLod registers */
#if (GLIDE_PLATFORM & GLIDE_HW_H3)
  baseAddress = mminfo->tmu_base_address + 0x200000;
#else
  baseAddress = mminfo->tmu_base_address >> 3;
#endif
  texMode     = mminfo->tTextureMode;
  tLod        = mminfo->tLOD;

  oldtexMode = gc->state.tmu_config[tmu].textureMode;
  oldtexMode &= ~(SST_TFORMAT | SST_TCLAMPT | 
                  SST_TCLAMPS | SST_TNCCSELECT | 
                  SST_TLODDITHER | SST_TCLAMPW | 
                  SST_TMAGFILTER | SST_TMINFILTER | 
                  SST_TRILINEAR);
  texMode |= oldtexMode;
  if (!gc->state.allowLODdither) texMode &= ~SST_TLODDITHER;

  /* Download the NCC table, if needed. */
  if ((mminfo->format == GR_TEXFMT_YIQ_422) ||
      (mminfo->format == GR_TEXFMT_AYIQ_8422)) {
    int table;                    /* ncc table we'll use */

    /* See if it's already down there */
    if (gc->tmu_state[tmu].ncc_mmids[0] == mmid) {
      /* Table 0 has what we need, so make it current */
      table = 0;
    } else if (gc->tmu_state[tmu].ncc_mmids[1] == mmid) {
      /* Table 1 has what we need, so make it current */
      table = 1;
    } else {
      /*
      **  it's not down there, so we need to pick the table and
      **  download it
      */
      /* Which table should we use? */
      table = gc->tmu_state[tmu].next_ncc_table;

      /* Download NCC table */
      _grTexDownloadNccTable(tmu, table, &mminfo->ncc_table, 0, 11);

      /* Set the mmid so we known it's down there */
      gc->tmu_state[tmu].ncc_mmids[table] = mmid;

      /* Set the state to know which table was the LRA */
      gc->tmu_state[tmu].next_ncc_table = (table == 0 ? 1 : 0);
    } /* we had to download it */

    /*
    **  Setting the TNCCSelect bit to 0 selects table 0, setting it to 1
    **  selects table 1
    */
    if (table == 0)
      texMode &= ~(SST_TNCCSELECT);
    else
      texMode |= SST_TNCCSELECT;
  } /* if it's an NCC texture */

  /* Force LOD dithering if the user asked for it.
   *
   * NB: There is a performance hit for this, but it does
   * look better.
   */
  texMode |= _GlideRoot.environment.texLodDither;

  /* Write relevant registers out to hardware */
  hw = SST_TMU(hw, tmu);
  REG_GROUP_BEGIN((0x02UL << tmu), textureMode, 3, 0x0B);
  {
    REG_GROUP_SET(hw, textureMode, texMode);
    REG_GROUP_SET(hw, tLOD,        tLod);
    REG_GROUP_SET(hw, texBaseAddr, baseAddress);
  }
  REG_GROUP_END();

  /* update shadows */
  gc->state.tmu_config[tmu].texBaseAddr = baseAddress;
  gc->state.tmu_config[tmu].textureMode = texMode;
  gc->state.tmu_config[tmu].tLOD = tLod;
 
  GR_END();
#undef FN_NAME
} /* guTexSource */


