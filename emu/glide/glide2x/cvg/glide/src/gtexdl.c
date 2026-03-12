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
** Revision 1.2.2.3  2005/04/23 18:33:33  koolsmoky
** added workaround for 8bit wide texture downloads for old revision TMUs
**
** Revision 1.2.2.2  2005/01/22 14:52:02  koolsmoky
** enabled packed argb for cmd packet type 3
**
** Revision 1.2.2.1  2004/12/23 20:45:56  koolsmoky
** converted to nasm syntax
** added x86 asm, 3dnow! triangle and mmx, 3dnow! texture download optimizations
**
** Revision 1.2  2000/10/03 18:28:33  mercury
** 003-clean_up_cvg-000, cvg tree cleanup.
**
** Revision 1.1.1.1  1999/12/07 21:49:11  joseph
** Initial checkin into SourceForge.
**
** 
** 61    5/18/98 12:11p Peter
** removed unnecesasry fencing
** 
** 60    4/29/98 2:32p Peter
** fixed texture palette broadcast
** 
** 59    3/30/98 2:46p Peter
** fixed texture download muckage
** 
** 58    3/29/98 10:54p Jdt
** Unset GLIDE_POINTCAST_PALETTE
** 
** 57    2/20/98 5:31p Peter
** crybaby glide
** 
** 56    2/20/98 11:00a Peter
** removed glide3 from glid2 tree
** 
** 55    2/02/98 2:14p Atai
** fixed palette broadcasts in  _grTexDownloadNccTable and
** _grTexDownloadPalette 
 * 
 * 54    1/09/98 6:48p Atai
 * grTexInfo, GR_LOD_* and GR_ASPECT_*
 * 
 * 53    1/08/98 7:09p Peter
 * real hw stuff modulo makefile change
 * 
 * 52    1/08/98 4:58p Atai
 * tex table broadcast, grVertexLayout enable/disable, stq, and some
 * defines
 * 
 * 51    1/08/98 11:06a Dow
 * Set palette downloads to broadcast.
 * 
 * 50    12/11/97 4:15p Peter
 * fixed assertions
 * 
 * 49    12/09/97 12:20p Peter
 * mac glide port
 * 
 * 48    12/05/97 4:26p Peter
 * watcom warnings
 * 
 * 47    12/02/97 9:48a Dow
 * Got rid of Texelfx rev 0 warning
 * 
 * 46    11/21/97 1:03p Peter
 * small texture download problem
 * 
 * 45    11/20/97 6:57p Dow
 * Texture Downloads for Banshee
 * 
 * 44    11/18/97 4:36p Peter
 * chipfield stuff cleanup and w/ direct writes
 * 
 * 43    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 42    11/14/97 12:09a Peter
 * comdex thing and some other stuff
 * 
 * 41    11/12/97 11:16a Peter
 * cleaned up assertions
 * 
 * 40    11/05/97 1:50p Peter
 * fixed partial palette downloads
 * 
 * 39    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 38    10/08/97 11:33a Peter
 * reg group for palette download
 * 
 * 37    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 36    9/04/97 3:32p Peter
 * starting grouping serial reg writes
 * 
 * 35    8/18/97 3:52p Peter
 * pre-hw arrival fixes/cleanup
 * 
 * 34    7/26/97 5:18p Peter
 * fixed macro muckage
 * 
 * 33    7/08/97 2:49p Peter
 * 
 * 32    7/02/97 12:28p Peter
 * removed spurious NOP, tex dl
 * 
 * 31    6/30/97 3:22p Peter
 * cmd fifo sanity
 * 
**
*/

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

/* externals from gtex.c */
extern FxU32 _gr_aspect_xlate_table[];
extern FxU32 _gr_evenOdd_xlate_table[];
extern const int _grMipMapHostWH[GR_ASPECT_1x8 + 1][GR_LOD_1 + 1][2];

/*---------------------------------------------------------------------------
** _grTexDownloadNccTable
**
** Downloads an ncctable to the specified _physical_ TMU(s).  This
** function is called internally by Glide and should not be executed
** by an application.
*/
GR_DDFUNC(_grTexDownloadNccTable,
          void,
          (GrChipID_t tmu, FxU32 which, const GuNccTable *table, int start, int end))
{
#define FN_NAME "_grTexDownloadNccTable"
  GR_BEGIN_NOFIFOCHECK(FN_NAME,89);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d, 0x%x, %d,%d)\n",tmu,which,table,start,end);
  GR_ASSERT(start==0);
  GR_ASSERT(end==11);

  /* check for null pointer */
  if (table == NULL) return;

  _GlideRoot.stats.palDownloads++;
  _GlideRoot.stats.palBytes += (end-start+1)<<2;

  if (gc->tmu_state[tmu].ncc_table[which] != table) {
    SstRegs* texHW;
    int i;
#ifdef GLIDE_POINTCAST_PALETTE
    texHW = SST_TMU(hw,tmu);
#else
    texHW = SST_CHIP(hw,0xe);
#endif

    if (which == 0) {
#ifdef GLIDE_POINTCAST_PALETTE
      REG_GROUP_BEGIN((0x02UL << tmu), nccTable0, 12, 0x0FFF);
#else
      REG_GROUP_BEGIN(0x0EUL, nccTable0, 12, 0x0FFF);
#endif
      for (i = 0; i < 12; i++) REG_GROUP_SET(texHW, nccTable0[i], table->packed_data[i]);
      REG_GROUP_END();
    } else {
#ifdef GLIDE_POINTCAST_PALETTE
      REG_GROUP_BEGIN((0x02UL << tmu), nccTable1, 12, 0x0FFF);
#else
      REG_GROUP_BEGIN(0x0EUL, nccTable1, 12, 0x0FFF);
#endif
      for (i = 0; i < 12; i++) REG_GROUP_SET(texHW, nccTable1[i], table->packed_data[i]);
      REG_GROUP_END();
    }

    gc->tmu_state[tmu].ncc_table[which] = table;
  }

  GR_END();
#undef FN_NAME
} /* _grTexDownloadNccTable */

/*-------------------------------------------------------------------
  Function: _grTexDownloadPalette
  Date: 6/9
  Implementor(s): jdt
  Library: Glide
  Description:
    Private function to download a palette to the specified tmu
  Arguments:
    tmu - which tmu to download the palette to
    pal - the pallete data
    start - beginning index to download
    end   - ending index to download
  Return:
    none
  -------------------------------------------------------------------*/
GR_DDFUNC(_grTexDownloadPalette,
          void,
          (GrChipID_t tmu, GuTexPalette *pal, int start, int end))
{
#define FN_NAME "_grTexDownloadPalette"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 89);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x, %d,%d)\n",tmu,pal,start,end);

  GR_CHECK_F(FN_NAME, pal == NULL, "pal invalid");
  GR_CHECK_F(FN_NAME, start < 0, "invalid start index");
  GR_CHECK_F(FN_NAME, end > 255, "invalid end index");
    
  /* NOTE:
  **
  **  This code broadcasts the palette because in the future, we will
  **  only support one global texture palette no matter how many TMUs
  **  there are.  This is fallout from the fact that future hardware
  **  has a unified memory architecture.
  **  
  **  Source licensees (meaning arcade or LBE vendors that) require the
  **  one palette/tmu mode should define GLIDE_POINTCAST_PALETTE on
  **  the command line for this file.  Understand, however, that this
  **  will not work on future hardware.
  */

#ifdef GLIDE_POINTCAST_PALETTE
  /*
  **  FURTHER NOTE:  
  **  There is a sublety (nice way of saying BUG) here.
  **  If TMU0 is specified, then the palette will be broadcast to all
  **  TMUS.  So, if the user downloads TMU1's palette, then TMU0's
  **  palette, TMU0's palette will be on *both* TMUs.  This is a
  **  pretty strong indicator that no one is using separate palettes
  **  on different TMUs.
  */
  hw = SST_TMU(hw,tmu);
#else
  hw = SST_CHIP(hw,0xE);
#endif

  _GlideRoot.stats.palDownloads++;
  _GlideRoot.stats.palBytes += ((end - start + 1) << 2);

  /* We divide the writes into 3 chunks trying to group things into
   * complete 8 word grouped packets to fit the nccTable palette
   * format: stuff before the 8 word alignment, aligned writes, and
   * stuff after the 8 word alignment to the end. The slop regions
   * are one packet apiece.  
   */
  {
#ifdef GLIDE_POINTCAST_PALETTE
    const FifoChipField chipId = (FifoChipField)(0x02UL << tmu);
#else
    const FifoChipField chipId = (FifoChipField)0x0EUL;
#endif
    const int endSlop = (end & ~0x07);
    const int startSlop = MIN(((start + 8) & ~0x07) - 1, end);
    int i = start;

    /* Is the start of the palette range unaligned or is the end of
     * the range less than a completely aligned range?  
     */
    if (((start & 0x07) != 0) || (end < ((start + 8) & ~0x07))) {
      const FxI32 slopCount = startSlop - start + 1;
      GR_ASSERT((slopCount > 0) && (slopCount <= 8));
    
      REG_GROUP_BEGIN(chipId, nccTable0[4 + (start & 0x07)], 
                      slopCount, (0xFF >> (8 - slopCount)));
      while(i < start + slopCount) {
        REG_GROUP_SET(hw, nccTable0[4 + (i & 0x07)],
                      (0x80000000 | ((i & 0xFE) << 23) | (pal->data[i] & 0xFFFFFF)));
        i++;
      }
      REG_GROUP_END();
    }

    /* Do all of the aligned palette ranges. */
    while(i < endSlop) {
      const int endIndex = i + 8;
      
      REG_GROUP_BEGIN(chipId, nccTable0[4], 8, 0xFF);
      while(i < endIndex) {
        REG_GROUP_SET(hw, nccTable0[4 + (i & 0x07)],
                      (0x80000000 | ((i & 0xFE) << 23) | (pal->data[i] & 0xFFFFFF)));
        i++;
      }
      REG_GROUP_END();
    }
  
    /* Do we have any more slop at the end of the ragne? */
    if (i <= end) {
      const FxU32 slopCount = end - endSlop + 1;
      
      REG_GROUP_BEGIN(chipId, nccTable0[4], 
                      slopCount, (0xFF >> (8 - slopCount)));
      while(i <= end) {
        REG_GROUP_SET(hw, nccTable0[4 + (i & 0x07)],
                      (0x80000000 | ((i & 0xFE) << 23) | (pal->data[i] & 0xFFFFFF)));
        i++;
      }
      REG_GROUP_END();
    }
  }
    
  GR_END();
#undef FN_NAME
} /* _grTexDownloadPalette */

/*-------------------------------------------------------------------
  Function: grTexDownloadTable
  Date: 6/3
  Implementor(s): jdt, GaryMcT
  Library: glide
  Description:
    download look up table data to a tmu
  Arguments:
    tmu - which tmu
    type - what type of table to download
        One of:
            GR_TEXTABLE_NCC0
            GR_TEXTABLE_NCC1
            GR_TEXTABLE_PALETTE
    void *data - pointer to table data
  Return:
    none
  -------------------------------------------------------------------*/
GR_ENTRY(grTexDownloadTable,
         void,
         (GrChipID_t tmu, GrTexTable_t type,  void *data))
{
#define FN_NAME "grTexDownloadTable"
  GR_BEGIN_NOFIFOCHECK("grTexDownloadTable",89);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,0x%x)\n",tmu,type,data);
  GR_CHECK_TMU(FN_NAME,tmu);
  GR_CHECK_F(FN_NAME, type > GR_TEXTABLE_PALETTE, "invalid table specified");
  GR_CHECK_F(FN_NAME, !data, "invalid data pointer");

  if (type == GR_TEXTABLE_PALETTE)     /* Need Palette Download Code */
    _grTexDownloadPalette(tmu, (GuTexPalette *)data, 0, 255);
  else {                                 /* Type is an ncc table */
    _grTexDownloadNccTable(tmu, type, (GuNccTable*)data, 0, 11);
    /*    _grTexDownloadNccTable(tmu, type, (GuNccTable*)data, 0, 11); */
  }
  GR_END();
#undef FN_NAME
} /* grTexDownloadTable */


/*-------------------------------------------------------------------
  Function: grTexDownloadMipMapLevelPartial
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Downloads a mipmap level to the specified tmu at the given
    texture start address
  Arguments:
    tmu           - which tmu
    startAddress - starting address for texture download,
                     this should be some value between grTexMinAddress()
                     and grTexMaxAddress()
    thisLod      - lod constant that describes the mipmap level
                    to be downloaded
    largeLod     - largest level of detail in complete mipmap to 
                   be downloaded at startAddress of which level to
                   be downloaded is a part
    aspectRatio  - aspect ratio of this mipmap
    format        - format of mipmap image data
    evenOdd      - which set of mipmap levels have been downloaded for
                    the selected texture
                    One of:
                      GR_MIPMAPLEVELMASK_EVEN 
                      GR_MIPMAPLEVELMASK_ODD
                      GR_MIPMAPLEVELMASK_BOTH
    data          - pointer to mipmap data
  Return:
    none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexDownloadMipMapLevelPartial,
         void,
         (GrChipID_t tmu, FxU32 startAddress,
          GrLOD_t thisLod, GrLOD_t largeLod, GrAspectRatio_t   aspectRatio,
          GrTextureFormat_t format, FxU32 evenOdd, void *data, int t, int max_t))
{
  const FifoChipField chipId = (FifoChipField)(0x02UL << tmu);
  FxI32 sh;
#if !GLIDE_DISPATCH_DOWNLOAD
  FxU32 max_s, width, tmu_baseaddress;
#endif
#define FN_NAME "grTexDownloadMipMapLevelPartial"

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 89);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x, %d,%d,%d, %d,%d 0x%x, %d,%d)\n",
                 tmu,startAddress,thisLod,largeLod,aspectRatio,
                 format,evenOdd,data,t,max_t);
  
  /* Check fo silly things */
  {
    const FxU32 size = _grTexTextureMemRequired(thisLod, thisLod, 
                                                aspectRatio, format, evenOdd);

  GR_CHECK_TMU(FN_NAME, tmu);
  GR_CHECK_COMPATABILITY(FN_NAME, 
                         startAddress + size > gc->tmu_state[tmu].total_mem,
                         "insufficient texture ram at startAddress");
  GR_CHECK_COMPATABILITY(FN_NAME, 
                         startAddress & 0x7,
                         "unaligned startAddress");
  GR_CHECK_F(FN_NAME, thisLod > GR_LOD_1,
             "thisLod invalid");
  GR_CHECK_F(FN_NAME, largeLod > GR_LOD_1,
             "largeLod invalid");
  GR_CHECK_F(FN_NAME, thisLod < largeLod,
             "thisLod may not be larger than largeLod");
  GR_CHECK_F(FN_NAME, aspectRatio > GR_ASPECT_1x8 || 
             aspectRatio < GR_ASPECT_8x1,
             "aspectRatio invalid");
  GR_CHECK_F(FN_NAME, evenOdd > 0x3 || evenOdd == 0,
             "evenOdd mask invalid");
  GR_CHECK_F(FN_NAME, !data,
             "invalid data pointer");
  GR_CHECK_F(FN_NAME, max_t >= _grMipMapHostWH[aspectRatio][thisLod][1],
             "invalid end row");

  if ((startAddress < 0x200000) && (startAddress + size > 0x200000))
    GrErrorCallback("grTexDownloadMipMapLevelPartial: mipmap level cannot span 2 Mbyte boundary",
                    FXTRUE);
  }

  /*------------------------------------------------------------
    Skip this level entirely if not in odd/even mask
    ------------------------------------------------------------*/
  if (!(evenOdd & (thisLod & 0x1 ? GR_MIPMAPLEVELMASK_ODD:GR_MIPMAPLEVELMASK_EVEN)))
    goto all_done;

  {
  /*------------------------------------------------------------
    Compute Base Address Given Start Address Offset
    ------------------------------------------------------------*/
    const FxU32 baseAddress = _grTexCalcBaseAddress(startAddress,
                                      largeLod, 
                                      aspectRatio,
                                      format,
                                                    evenOdd) >> 3;
  
  /*------------------------------------------------------------
    Compute pertinant contents of tLOD and texMode registers 
    ------------------------------------------------------------*/
    FxU32 tLod = SST_TLOD_MINMAX_INT(largeLod, GR_LOD_1);
    FxU32 texMode = format << SST_TFORMAT_SHIFT;
    
  tLod |= _gr_evenOdd_xlate_table[evenOdd];
  tLod |= _gr_aspect_xlate_table[aspectRatio];
    
  if (gc->state.tmu_config[tmu].textureMode & SST_SEQ_8_DOWNLD) {
    sh = 2;
    texMode |= SST_SEQ_8_DOWNLD;
  } else {
    sh = 3;
  }

  /*------------------------------------------------------------
    Update TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  hw = SST_TMU(hw,tmu);
  REG_GROUP_BEGIN(chipId, textureMode, 3, 0x0B);
  {
    REG_GROUP_SET(hw, textureMode, texMode);
    REG_GROUP_SET(hw, tLOD,        tLod);
    REG_GROUP_SET(hw, texBaseAddr, baseAddress);
  }
  REG_GROUP_END();
  }

#if GLIDE_DISPATCH_DOWNLOAD
  /* Do the download */
  {
    const FxU32 
      width = _grMipMapHostWH[aspectRatio][thisLod][0],
      formatSel = (format >= GR_TEXFMT_16BIT),
      tmuBaseAddr = ((FxU32)gc->tex_ptr + 
                     (((FxU32)tmu) << 21UL) + 
                     (((FxU32)thisLod) << 17UL));
    FxU32 
      widthSel = (width >> 0x01UL),
      max_s = width >> (formatSel
                        ? 1   /* 16-bit texture */
                        : 2); /* 8-bit texture */

    if (max_s < 1) max_s = 1;
    if (widthSel > 2) widthSel = 3;
    
    _GlideRoot.stats.texBytes += max_s * (max_t - t + 1) * 4;

    if ((sh == 3) && !formatSel && (width > 4)) {
      /* 8-bit >4xN texture with Old revision TMUs (GR_SSTTYPE_VOODOO) */
      const FxU8* src8  = (const FxU8*)data;
      /* Compute Physical Write Pointer */
      const FxU32
        tmu_baseaddress = (FxU32)gc->tex_ptr +
                          (((FxU32)tmu) << 21) + (((FxU32)thisLod) << 17);

      for (; t <= max_t; t++) {
        FxU32
          s,
          tex_address = tmu_baseaddress + TEX_ROW_ADDR_INCR(t);

        LINEAR_WRITE_BEGIN(max_s, kLinearWriteTex,
                           (FxU32)tex_address - (FxU32)gc->tex_ptr,
                           0x00UL, 0x00UL);
        for (s = 0; s < max_s; s+=2) {
          const FxU32 t0 = *(const FxU32*)(src8   );
          const FxU32 t1 = *(const FxU32*)(src8 + sizeof(FxU32));

          LINEAR_WRITE_SET_8(tex_address                , t0);
          LINEAR_WRITE_SET_8(tex_address + sizeof(FxU32), t1);

          tex_address += 16;
          src8 += (sizeof(FxU32) << 1);
        }
        LINEAR_WRITE_END();
      }
    } else {
      /* GR_SSTTYPE_Voodoo2 all go through here */
      (*((*_GlideRoot.deviceArchProcs.curTexProcs)[formatSel][widthSel]))(gc, 
                                                                          tmuBaseAddr,
                                                                          max_s, t, max_t,
                                                                          data);
    }
  }
#else
  /*------------------------------------------------------------
    Determine max_s
    ------------------------------------------------------------*/
  width = _grMipMapHostWH[aspectRatio][thisLod][0];
  max_s = width >> ((format < GR_TEXFMT_16BIT)
                    ? 2   /* 8-bit texture */
                    : 1); /* 16-bit texture */
  if (max_s < 1) max_s = 1;

  _GlideRoot.stats.texBytes += max_s * (max_t - t + 1) * 4;

  /*------------------------------------------------------------
    Compute Physical Write Pointer
    ------------------------------------------------------------*/
  tmu_baseaddress = (FxU32)gc->tex_ptr;
  tmu_baseaddress += (((FxU32)tmu) << 21) + (((FxU32)thisLod) << 17);

  /*------------------------------------------------------------
    Handle 8-bit Textures
    ------------------------------------------------------------*/
  if (format < GR_TEXFMT_16BIT) { /* 8 bit textures */
    /* Hoisted initialization */
    FxU32 tex_address = tmu_baseaddress + TEX_ROW_ADDR_INCR(t);
    const FxU8* src8  = (const FxU8*)data;

    switch(width) {
    case 1:
      for (; t <= max_t; t++) {
        LINEAR_WRITE_BEGIN(1, kLinearWriteTex, 
                           (FxU32)tex_address - (FxU32)gc->tex_ptr,
                           0x00UL, 0x00UL);
        LINEAR_WRITE_SET_8(tex_address, (const FxU32)*(const FxU8*)src8);
        LINEAR_WRITE_END();
      
        src8 += width;
        tex_address += TEX_ROW_ADDR_INCR(1);
      }
      break;

    case 2:
      for (; t <= max_t; t++) {
        LINEAR_WRITE_BEGIN(1, kLinearWriteTex, 
                           (FxU32)tex_address - (FxU32)gc->tex_ptr,
                           0x00UL, 0x00UL);
        LINEAR_WRITE_SET_8(tex_address, (const FxU32)*(const FxU16*)src8);
        LINEAR_WRITE_END();
            
        src8 += width;
        tex_address += TEX_ROW_ADDR_INCR(1);
      }
      break;

    case 4:
      for (; t <= max_t; t++) {
        LINEAR_WRITE_BEGIN(1, kLinearWriteTex, 
                           (FxU32)tex_address - (FxU32)gc->tex_ptr,
                           0x00UL, 0x00UL);
        LINEAR_WRITE_SET_8(tex_address, *(const FxU32*)src8);
        LINEAR_WRITE_END();
        
        src8 += width;
        tex_address += TEX_ROW_ADDR_INCR(1);
      }
      break;

    default: { /* >4xN texture */
      const FxU32 texAddrInc = ((sh == 3)
                                ? 16 /* Old TMUs */
                                : 8);/* New TMUs */

      for (; t <= max_t; t++) {
        FxU32 s;

        tex_address = tmu_baseaddress + TEX_ROW_ADDR_INCR(t);

        LINEAR_WRITE_BEGIN(max_s, kLinearWriteTex,
                           (FxU32)tex_address - (FxU32)gc->tex_ptr,
                           0x00UL, 0x00UL);
        for (s = 0; s < max_s; s+=2) {
          const FxU32 t0 = *(const FxU32*)(src8   );
          const FxU32 t1 = *(const FxU32*)(src8 + sizeof(FxU32));
                    
          LINEAR_WRITE_SET_8(tex_address                , t0);
          LINEAR_WRITE_SET_8(tex_address + sizeof(FxU32), t1);

          tex_address += texAddrInc;
          src8 += (sizeof(FxU32) << 1);
        }
        LINEAR_WRITE_END();
      }
      break;
    }
    }
  } else { /* 16-bit Textures */
    /* Hoisted initialization */
    FxU32 tex_address = tmu_baseaddress + TEX_ROW_ADDR_INCR(t);
    const FxU16* src16 = (const FxU16*)data;

        /* Cases 1, 2 don't need inner loops for s */
    switch(width) {
    case 1:
        for (; t <= max_t; t++) {
          LINEAR_WRITE_BEGIN(1, kLinearWriteTex,
                             (FxU32)tex_address - (FxU32)gc->tex_ptr,
                             0x00UL, 0x00UL);
        LINEAR_WRITE_SET_16(tex_address, (const FxU32)*(const FxU16*)src16);
          LINEAR_WRITE_END();
          
          src16 += width;
          tex_address += TEX_ROW_ADDR_INCR(1);
        }
      break;
      
    case 2:
      for (; t <= max_t; t++) {
        LINEAR_WRITE_BEGIN(1, kLinearWriteTex,
                           (FxU32)tex_address - (FxU32)gc->tex_ptr,
                           0x00UL, 0x00UL);
        LINEAR_WRITE_SET_16(tex_address, *(const FxU32*)src16);
        LINEAR_WRITE_END();
        
        src16 += width;
        tex_address += TEX_ROW_ADDR_INCR(1);
      }
      break;

    default:
      for (; t <= max_t; t++) {
        FxU32 s;

        tex_address = tmu_baseaddress + TEX_ROW_ADDR_INCR(t);

        /* Loop unrolled to process 2 dwords per iteration */            
        LINEAR_WRITE_BEGIN(max_s, kLinearWriteTex,
                           (FxU32)tex_address - (FxU32)gc->tex_ptr,
                           0x00UL, 0x00UL);
        for (s = 0; s < max_s; s += 2) {
          const FxU32 t0 = *(const FxU32*)(src16   );
          const FxU32 t1 = *(const FxU32*)(src16 + 2);

          GDBG_INFO(195, "s = %d, t= %d, address = 0x%x\n",
                    s, t, (FxU32) tex_address - (FxU32) gc->tex_ptr + 0x200000);
          LINEAR_WRITE_SET_16(tex_address    , t0);
          LINEAR_WRITE_SET_16(tex_address + 4, t1);
               
          tex_address += 8;
          src16 += 4;
        }
              
        LINEAR_WRITE_END();
      }

      break;
    }
  }
#endif
  
  /*------------------------------------------------------------
    Restore TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  REG_GROUP_BEGIN(chipId, textureMode, 3, 0x0B);
  {
    REG_GROUP_SET(hw, textureMode, gc->state.tmu_config[tmu].textureMode);
    REG_GROUP_SET(hw, tLOD,        gc->state.tmu_config[tmu].tLOD);
    REG_GROUP_SET(hw, texBaseAddr, gc->state.tmu_config[tmu].texBaseAddr);
  }
  REG_GROUP_END();

all_done:
  _GlideRoot.stats.texDownloads++;
#undef FN_NAME
} /* grTexDownloadMipmapLevelPartial */

/* 
   Let me take this opportunity to register my formal opposition to
   this function.  Either we do this or we don't.  Let's not hack like
   this.

   CHD
*/

GR_ENTRY(ConvertAndDownloadRle,
         void,
         (GrChipID_t tmu, FxU32 startAddress,
          GrLOD_t thisLod, GrLOD_t largeLod, GrAspectRatio_t aspectRatio,
          GrTextureFormat_t format, FxU32 evenOdd,
          FxU8 *bm_data, long bm_h,
          FxU32 u0, FxU32 v0,
          FxU32 width, FxU32 height, FxU32 dest_width, FxU32 dest_height, FxU16 *tlut))
{
  const FifoChipField chipId = (FifoChipField)(0x02UL << tmu);
  FxI32 sh;
  FxU32 max_s,s,t,max_t,tex_address, tmu_baseaddress;
  FxU32 tLod, texMode, baseAddress,size;
  FxU32 offset,expected_size;
  unsigned long  i;
  FxU16 *src;
  extern FxU16 rle_line[256]; /* diTex.c */
  extern FxU16 *rle_line_end; /* ditex.c */
#define FN_NAME "ConvertAndDownloadRLE"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 89);
   
  /* make sure even number */
  width&=0xFFFFFFFE;
   
  max_s=width>>1;
  max_t=height;
   
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x, %d,%d,%d, %d,%d 0x%x, %d)\n",
                 tmu,startAddress,thisLod,largeLod,aspectRatio,
                 format,evenOdd,bm_data,max_t);
   
  size = _grTexTextureMemRequired(thisLod, thisLod, aspectRatio, format, evenOdd);
  GR_CHECK_TMU(FN_NAME, tmu);
  GR_CHECK_F(FN_NAME, startAddress + size > gc->tmu_state[tmu].total_mem,
             "insufficient texture ram at startAddress");
  GR_CHECK_F(FN_NAME, startAddress & 0x7, "unaligned startAddress");
  GR_CHECK_F(FN_NAME, thisLod > GR_LOD_1, "thisLod invalid");
  GR_CHECK_F(FN_NAME, largeLod > GR_LOD_1, "largeLod invalid");
  GR_CHECK_F(FN_NAME, thisLod < largeLod, "thisLod may not be larger than largeLod");
  GR_CHECK_F(FN_NAME, aspectRatio > GR_ASPECT_1x8 || 
             aspectRatio < GR_ASPECT_8x1,
             "aspectRatio invalid");
  GR_CHECK_F(FN_NAME, evenOdd > 0x3 || evenOdd == 0, "evenOdd mask invalid");
  GR_CHECK_F(FN_NAME, !bm_data, "invalid data pointer");
  GR_CHECK_F(FN_NAME, (dest_height-1) >= (FxU32)_grMipMapHostWH[aspectRatio][thisLod][1],
             "invalid end row");

  if ((startAddress < 0x200000) && (startAddress + size > 0x200000))
    GrErrorCallback("grTexDownloadMipMapLevelPartial: mipmap level cannot span 2 Mbyte boundary",
                    FXTRUE);

  /*------------------------------------------------------------
    Skip this level entirely if not in odd/even mask
    ------------------------------------------------------------*/
  if (!(evenOdd & (thisLod & 0x1 ? GR_MIPMAPLEVELMASK_ODD:GR_MIPMAPLEVELMASK_EVEN)))
    goto all_done;
  
  /*------------------------------------------------------------
    Compute Base Address Given Start Address Offset
    ------------------------------------------------------------*/
  baseAddress = _grTexCalcBaseAddress(startAddress,
                                      largeLod, 
                                      aspectRatio,
                                      format,
                                      evenOdd);
  baseAddress >>= 3;
#if H3
  baseAddress = ((baseAddress << 3) & ~0x0F);
#endif

  /*------------------------------------------------------------
    Compute Physical Write Pointer
    ------------------------------------------------------------*/
  tmu_baseaddress = (FxU32)gc->tex_ptr;
  tmu_baseaddress += (((FxU32)tmu)<<21) + (((FxU32)thisLod)<<17);
  
  /*------------------------------------------------------------
    Compute pertinant contents of tLOD and texMode registers 
    ------------------------------------------------------------*/
  tLod = SST_TLOD_MINMAX_INT(largeLod, GR_LOD_1);
  tLod |= _gr_evenOdd_xlate_table[evenOdd];
  tLod |= _gr_aspect_xlate_table[aspectRatio];
  texMode = format << SST_TFORMAT_SHIFT;
  if (gc->state.tmu_config[tmu].textureMode & SST_SEQ_8_DOWNLD) {
    sh = 2;
    texMode |= SST_SEQ_8_DOWNLD;
  } else {
    sh = 3;
  }

  /*------------------------------------------------------------
    Update TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  hw = SST_TMU(hw,tmu);
  REG_GROUP_BEGIN(chipId, textureMode, 3, 0x0B);
  {
    REG_GROUP_SET(hw, textureMode, texMode);
    REG_GROUP_SET(hw, tLOD,        tLod);
    REG_GROUP_SET(hw, texBaseAddr, baseAddress);
  }
  REG_GROUP_END();
  
  _GlideRoot.stats.texBytes += dest_width * (dest_height) * 2;
  
  /* here I can do my writes and conversion and I will be so happy */
  offset=4+bm_h;
  for (i=0; i<v0; i++)
    offset += bm_data[4+i];
  
  max_s=dest_width>>1;
  expected_size=max_s*5;
  
  rle_line_end=rle_line+width+u0;
  for(t=0;t<max_t;t++) {
    tex_address = tmu_baseaddress + TEX_ROW_ADDR_INCR(t);
    src = rle_line + u0;
      
    rle_decode_line_asm(tlut,&bm_data[offset],rle_line);
      
    LINEAR_WRITE_BEGIN(max_s, kLinearWriteTex, 
                       (FxU32)tex_address - (FxU32)gc->tex_ptr,
                       0x00UL, 0x00UL);         
    for(s = 0; s < max_s; s++) {
      LINEAR_WRITE_SET(tex_address + (s << 2), *(FxU32 *) src);
      src += 2;
    }
    LINEAR_WRITE_END();
      
    offset+=bm_data[4+i++];
  }

  if (dest_height>height) {
    tex_address = tmu_baseaddress + TEX_ROW_ADDR_INCR(t);
    src=rle_line+u0;

    LINEAR_WRITE_BEGIN(max_s, kLinearWriteTex, 
                       (FxU32)tex_address - (FxU32)gc->tex_ptr,
                       0x00UL, 0x00UL);            
    for(s = 0; s < max_s; s++) {
      LINEAR_WRITE_SET(tex_address + (s << 2), *(FxU32 *) src);
      src += 2;
    }
    LINEAR_WRITE_END();         
  }

  /*------------------------------------------------------------
    Restore TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  REG_GROUP_BEGIN(chipId, textureMode, 3, 0x0B);
  {
    REG_GROUP_SET(hw, textureMode, gc->state.tmu_config[tmu].textureMode);
    REG_GROUP_SET(hw, tLOD,        gc->state.tmu_config[tmu].tLOD);
    REG_GROUP_SET(hw, texBaseAddr, gc->state.tmu_config[tmu].texBaseAddr);
  }
  REG_GROUP_END();

all_done:
  _GlideRoot.stats.texDownloads++;
#undef FN_NAME
}






