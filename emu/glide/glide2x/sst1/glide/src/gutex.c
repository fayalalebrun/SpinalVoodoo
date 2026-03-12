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
 * 7     3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
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

GR_ENTRY(guTexSource, void, ( GrMipMapId_t mmid ))
{
  FxU32               texMode, tLod;
  FxU32               oldtexMode;
  FxU32               baseAddress;
  int                 tmu;
  const GrMipMapInfo *mminfo;

  GR_BEGIN_NOFIFOCHECK("guTexSource",99);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mmid));

  /*
  ** Make sure that mmid is not NULL
  */
  if ( mmid == GR_NULL_MIPMAP_HANDLE )
  {
    return;
  }

  /*
  ** get a pointer to the relevant GrMipMapInfo struct
  */
  mminfo = &gc->mm_table.data[mmid];
  tmu = mminfo->tmu;

  GR_CHECK_TMU( myName, tmu );
  GR_CHECK_W( myName, mmid == gc->state.current_mm[tmu], "setting same state twice" );

  gc->state.current_mm[tmu] = mmid;

  /*
  ** Set up new glide state for this mmid
  */
  gc->state.tmu_config[tmu].mmMode =  mminfo->mipmap_mode;
  gc->state.tmu_config[tmu].smallLod = mminfo->lod_min;
  gc->state.tmu_config[tmu].largeLod = mminfo->lod_max;
  gc->state.tmu_config[tmu].evenOdd =  mminfo->odd_even_mask;
  gc->state.tmu_config[tmu].nccTable = 0;

  /*
  ** Set up base address, texMode, and tLod registers
  */
  baseAddress = mminfo->tmu_base_address >> 3;
  texMode     = mminfo->tTextureMode;
  tLod        = mminfo->tLOD;

  oldtexMode = gc->state.tmu_config[tmu].textureMode;
  oldtexMode &= ~( SST_TFORMAT | SST_TCLAMPT | 
                   SST_TCLAMPS | SST_TNCCSELECT | 
                   SST_TLODDITHER | SST_TCLAMPW | 
                   SST_TMAGFILTER | SST_TMINFILTER | 
                   SST_TRILINEAR );
  texMode |= oldtexMode;
  if (!gc->state.allowLODdither)
    texMode &= ~SST_TLODDITHER;

 /* 
  **  Download the NCC table, if needed.  
  */
  if (
      (mminfo->format==GR_TEXFMT_YIQ_422) ||
      (mminfo->format==GR_TEXFMT_AYIQ_8422)
      )
  {
    int
      table;                    /* ncc table we'll use */
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
      _grTexDownloadNccTable( tmu, table, &mminfo->ncc_table, 0, 11 );
      /* Set the mmid so we known it's down there */
      gc->tmu_state[tmu].ncc_mmids[table] = mmid;
      /* Set the state to know which table was the LRA */
      gc->tmu_state[tmu].next_ncc_table =
        (table == 0 ? 1 : 0);
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

  GR_SET_EXPECTED_SIZE(12+2*PACKER_WORKAROUND_SIZE);

  /* Write relevant registers out to hardware */

  PACKER_WORKAROUND;
  hw = SST_TMU(hw,tmu);
  GR_SET( hw->texBaseAddr , baseAddress );
  GR_SET( hw->textureMode , texMode );
  GR_SET( hw->tLOD , tLod );
  PACKER_WORKAROUND;

  /* update shadows */
  gc->state.tmu_config[tmu].texBaseAddr = baseAddress;
  gc->state.tmu_config[tmu].textureMode = texMode;
  gc->state.tmu_config[tmu].tLOD = tLod;
 
  GR_END();
} /* guTexSource */

