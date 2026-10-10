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
** Revision 1.1.1.1.8.1  2004/10/04 09:17:16  dborca
** killed some warnings / compilation errors
**
** Revision 1.1.1.1  1999/12/07 21:42:30  joseph
** Initial checkin into SourceForge.
**
** 
** 1     10/08/98 11:30a Brent
** 
** 4     7/24/98 1:41p Hohn
** 
** 3     6/24/98 2:11p Atai
** remove guclip.c, ddgump.c, and gutex.c
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 9     1/08/98 7:09p Peter
 * real hw stuff modulo makefile change
 * 
 * 8     1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 7     12/15/97 5:51p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 6     5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 5     5/21/97 6:04a Peter
 * 
 * 4     5/05/97 4:24p Pgj
 * Neuter guTexDownloadMipMap error message
 * 
 * 3     3/18/97 9:07p Dow
 * Got rid of #$#%#$ // comments
 * 
 * 2     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 1     12/23/96 1:39p Dow
 * Changes for multiplatform
**
*/

#include <string.h>
#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"

/*---------------------------------------------------------------------------
** _guTexMemReset
**
** Clears out texture buffer memory.
*/
void FX_CSTYLE
_guTexMemReset(void)
{
  int i;
  
  GR_BEGIN_NOFIFOCHECK("guTexMemReset",99);
  GDBG_INFO_MORE(gc->myLevel,"()\n");

#ifndef GLIDE3_ALPHA
  memset( gc->mm_table.data, 0, sizeof( gc->mm_table.data ) );
  gc->mm_table.free_mmid = 0;
#endif /* !GLIDE3_ALPHA */
  
  for ( i = 0; i < gc->num_tmu; i++ ) {
    gc->state.current_mm[i] = (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
    gc->tmu_state[i].freemem_base = 0;
    gc->tmu_state[i].ncc_mmids[0] = 
      gc->tmu_state[i].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;    
    gc->tmu_state[i].ncc_table[0] = 
      gc->tmu_state[i].ncc_table[1] = 0;
  }
  GR_END();
} /* guTexMemReset */

