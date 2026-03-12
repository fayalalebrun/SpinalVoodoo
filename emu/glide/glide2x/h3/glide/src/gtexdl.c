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
** 3     3/17/99 6:16p Dow
** Phantom Menace and other fixes.
** 
** 92    3/12/99 5:52p Hanson
** No idle on lfb lock needed to be disabled for
** banshee/avenger since it does not have SST1 style cmd fifo.  Updated
** ConvertAndDownloadRLE for Banshee/Avenger.
** 
** 91    3/02/99 3:16p Atai
** added 2d nop before texture download to maintain cache coherency
** 
** 90    3/02/99 2:22p Peter
** 2d nop flushes (although 3d nop should too)
** 
** 89    2/26/99 11:31a Peter
** fixed alignment assertion
** 
** 88    2/18/99 5:35p Peter
** 
** 87    2/18/99 5:28p Peter
** download/source for everything
** 
** 86    2/16/99 4:14p Peter
** sanitized source/download
** 
** 85    2/10/99 2:24p Peter
** corrected alignment textures within an alignment allocation unit
** 
** 84    2/02/99 5:41p Peter
** textures through lfb
** 
** 83    12/03/98 11:27p Dow
** Code 'cleanup' heç
** 
** 82    11/03/98 11:10a Atai
** texture cache flushing
** 
** 81    11/02/98 5:34p Atai
** merge direct i/o code
** 
** 80    10/21/98 10:41a Atai
** 
** 79    10/20/98 7:12p Peter
** static build hapiiness
** 
** 78    10/20/98 4:39p Atai
** update tramOffset and tramSize
** 
** 77    10/09/98 6:57p Peter
** dynamic 3DNow!(tm) mods
** 
** 76    9/21/98 5:35p Dow
** 3DNow! mods
** 
** 75    9/11/98 2:09p Peter
** removed debugging code
** 
** 74    9/11/98 1:44p Peter
** texture mask fixes
** 
** 73    9/04/98 3:32p Peter
** reversed the sense of the active bytes in small downloads
** 
** 72    8/27/98 10:07p Peter
** added flush to rle
** 
** 71    8/27/98 9:54p Peter
** flush old texels when replacing small lod levels
** 
** 70    8/27/98 1:55p Peter
** download problem w/ widths smaller than dword
** 
** 69    7/19/98 4:34p Mikec
** Relaxed 2MB span constraint on downloadmipmappartial.
** 
** 68    5/22/98 6:24p Atai
** texturing out of render buffer hack
** 
** 67    5/21/98 4:47p Dow
** Direct Register Writes Work
** 
** 66    5/18/98 3:19p Peter
** one more palette broadcast thing
** 
** 65    5/13/98 9:12a Dow
** 
** 64    5/12/98 2:42p Dow
** 
** 63    5/05/98 3:01p Peter
** packet chipfield vs direct register chipfield
** 
** 62    4/29/98 2:32p Peter
** fixed texture palette broadcast
** 
** 61    4/29/98 1:02p Peter
** non dword aligned downloads
** 
** 60    3/28/98 11:24a Dow
** itwoç
** 
** 58    2/17/98 12:52p Dow
** Conditional LFB Download Hack
** 
** 57    2/02/98 2:17p Atai
** fixed palette broadcasts in  _grTexDownloadNccTable and
** _grTexDownloadPalette 
** 
** 56    1/29/98 9:54p Dow
** This is Banshee
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
 * fixed macro effage
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
    texHW = SST_CHIP(hw, 0x06UL);
#endif

    if (which == 0) {
#ifdef GLIDE_POINTCAST_PALETTE
      REG_GROUP_BEGIN((0x02UL << tmu), nccTable0, 12, 0x0FFF);
#else
      REG_GROUP_BEGIN(0x06UL, nccTable0, 12, 0x0FFF);
#endif
      for (i = 0; i < 12; i++) REG_GROUP_SET(texHW, nccTable0[i], table->packed_data[i]);
      REG_GROUP_END();
    } else {
#ifdef GLIDE_POINTCAST_PALETTE
      REG_GROUP_BEGIN((0x02UL << tmu), nccTable1, 12, 0x0FFF);
#else
      REG_GROUP_BEGIN(0x06UL, nccTable1, 12, 0x0FFF);
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
  hw = SST_CHIP(hw,0x6);
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
    const FifoChipField chipId = (FifoChipField)0x06UL;
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
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
GR_ENTRY(grTexDownloadTable,
         void,
         (GrTexTable_t type,  void *data))
{
  GR_BEGIN_NOFIFOCHECK("grTexDownloadTable",89);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x)\n",type,data);
  GR_CHECK_F(myName, type > GR_TEXTABLE_PALETTE, "invalid table specified");
  GR_CHECK_F(myName, !data, "invalid data pointer");

  if (type == GR_TEXTABLE_PALETTE)     /* Need Palette Download Code */
    _grTexDownloadPalette(GR_TMU0, (GuTexPalette *)data, 0, 255);
  else {                                 /* Type is an ncc table */
    _grTexDownloadNccTable(GR_TMU0, type, (GuNccTable*)data, 0, 11);
    /*    _grTexDownloadNccTable(tmu, type, (GuNccTable*)data, 0, 11); */
  }
  GR_END();
} /* grTexDownloadTable */
#else
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
#endif


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
#define FN_NAME "grTexDownloadMipMapLevelPartial"

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 89);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x, %d,%d,%d, %d,%d 0x%x, %d,%d)\n",
                 tmu,startAddress,thisLod,largeLod,aspectRatio,
                 format,evenOdd,data,t,max_t);

  {
    GR_CHECK_TMU(myName, tmu);
    GR_CHECK_F(myName, 
               startAddress + _grTexTextureMemRequired(thisLod, thisLod, aspectRatio, format, evenOdd, FXTRUE) > 
               gc->tmu_state[tmu].total_mem,
               "insufficient texture ram at startAddress");
    GR_CHECK_F(myName, startAddress & SST_TEXTURE_ALIGN_MASK,
               "unaligned startAddress");
    GR_CHECK_F(myName, thisLod > G3_LOD_TRANSLATE(GR_LOD_1),
               "thisLod invalid");
    GR_CHECK_F(myName, largeLod > G3_LOD_TRANSLATE(GR_LOD_1),
               "largeLod invalid");
    GR_CHECK_F(myName, thisLod < largeLod,
               "thisLod may not be larger than largeLod");
    GR_CHECK_F(myName, aspectRatio > G3_ASPECT_TRANSLATE(GR_ASPECT_1x8) || 
               aspectRatio < G3_ASPECT_TRANSLATE(GR_ASPECT_8x1),
               "aspectRatio invalid");
    GR_CHECK_F(myName, evenOdd > 0x3 || evenOdd == 0,
               "evenOdd mask invalid");
    GR_CHECK_F(myName, !data,
               "invalid data pointer");
    GR_CHECK_F(myName, max_t >= _grMipMapHostWH[aspectRatio][thisLod][1],
               "invalid end row");
  }
  
  /*------------------------------------------------------------
    Skip this level entirely if not in odd/even mask
    ------------------------------------------------------------*/
  if (!(evenOdd & (thisLod & 0x1 ? GR_MIPMAPLEVELMASK_ODD:GR_MIPMAPLEVELMASK_EVEN)))
    goto all_done;

  {
    FxU32
      baseAddress;

    /* Compute physical start address for the download. */
    {
      FxU32
        texOffset = 0x00UL;
      
      /* We need to do some magic to pack the small levels and have a
       * properly aligned baseAddr. If the current level is not going
       * to start on an alignment boundary when working backwards in
       * the chain we need to offset it into the block so that it is
       * addressable with an aligned baseAddr.  
       */
      {
        const FxU32
          aspectIndex = ((aspectRatio < GR_ASPECT_1x1) 
                         ? GR_ASPECT_1x1 - aspectRatio 
                         : aspectRatio - GR_ASPECT_1x1),
          lodIndex = ((thisLod == GR_LOD_256) ? GR_LOD_256 : thisLod - 1),
          formatShift = (format >= GR_TEXFMT_16BIT);
        FxU32
          levelSize = (_grMipMapHostSize[aspectIndex][lodIndex] << formatShift);
        GrLOD_t
          minLod = thisLod;
        
        if (levelSize < SST_TEXTURE_ALIGN) {
          GrLOD_t
            maxLod = lodIndex;
          
          /* Find the smallest level that naturally starts on an
           * alignment boundary. If this is larger than the current
           * mipmap chain's large lod then this we have to compute the
           * offset within this alignment unit.
           *
           * NB: This could a table lookup, but I'm writing the obvious code
           * right now so that there aren't any mystic #'s being pulled out
           * of the recesses of my colon.  
           */
          while(maxLod > GR_LOD_256) {
            levelSize = (_grMipMapHostSize[aspectIndex][maxLod] << formatShift);
            
            if (levelSize >= SST_TEXTURE_ALIGN) break;
            maxLod--;
            texOffset += levelSize;
          }

          /* maxLod is the index of the smallest level of this aspect
           * ratio that takes up atleast a full alignment unit.  We
           * reset the small lod to this so that we can compute the
           * offset for the 'large' levels in the mipmap chain.  
           */
          GR_ASSERT(maxLod != GR_LOD_256);
          minLod = maxLod + 1;
        } 

        if (minLod > largeLod) {
          texOffset += _grTexTextureMemRequired(minLod - 1, largeLod,
                                                aspectRatio,
                                                format,
                                                evenOdd,
                                                FXFALSE);
        }
      }
      
      /* This is going directly to the 2d lfb space rather than
       * through the texture port so we have to add in the actual hw
       * offset that the texture 'surface' starts at.
       */
      baseAddress = (gc->tramOffset[tmu] + 
                     startAddress +
                     texOffset);
    }
    {
      /*
      ** To maintain cache coherency in h4, issue a 2D nop to
      ** flush all pixel from 3D session before texture download
      */
      REG_GROUP_BEGIN_WAX(command, 1, 0x01UL);
      REG_GROUP_SET_WAX(gc->gRegs, command, SSTG_NOP | SSTG_GO);
      REG_GROUP_END();
    }
    /* Do the download */
    {
      const FxU32
        width = _grMipMapHostWH[aspectRatio][thisLod][0],
        formatSel = (format >= GR_TEXFMT_16BIT);
      FxU32 
        widthSel = (width >> 0x01UL),
        max_s = width >> (formatSel
                          ? 1   /* 16-bit texture */
                          : 2); /* 8-bit texture */

      if (max_s < 1) max_s = 1;
      if (widthSel > 2) widthSel = 3;
    
      _GlideRoot.stats.texBytes += max_s * (max_t - t + 1) * 4;

      (*((*gc->archDispatchProcs.texDownloadProcs)[formatSel][widthSel]))(gc, 
                                                                          baseAddress,
                                                                          max_s, t, max_t,
                                                                          data);
    }

    /* If this is a small lod level in a texture replacing texels that
     * are already loaded then it may be necessary to flush the old
     * texels from memory before any other rendering operation using
     * this texture is issued. Unconditionally flush these old texels
     * just in case rather than being too clever.
     *
     * The reason that we need to flush here even though we're not
     * going through the texture port is because it is perfectly
     * legal to source once and download over and over again. (See
     * chd for a funny SpecOps story).
     *
     * NB: The documented nop does not currently work on basnehe which
     * is why we do the ~texBaseAddr crap.
     */
    {
      const FifoChipField 
        chipId = (FifoChipField)(0x02UL << tmu);
      const FxU32
        baseAddress = gc->state.tmu_config[tmu].texBaseAddr;
      
      GR_SET_EXPECTED_SIZE(3 * sizeof(FxU32), 3);
      {
        GR_SET(BROADCAST_ID, hw, nopCMD, 0);
        GR_SET(chipId, hw, texBaseAddr, ~baseAddress & SST_TEXTURE_ADDRESS);
        GR_SET(chipId, hw, texBaseAddr, baseAddress);
      }
      GR_CHECK_SIZE();

      /* Again force a pixel flush which should force all of the
       * texture downloads to flush from internal fifos etc.  
       */
      REG_GROUP_BEGIN_WAX(command, 1, 0x01UL);
      REG_GROUP_SET_WAX(gc->gRegs, command, SSTG_NOP | SSTG_GO);
      REG_GROUP_END();
    }
  }

  all_done:
  _GlideRoot.stats.texDownloads++;
#undef FN_NAME
} /* grTexDownloadMipmapLevelPartial */

/* 
   Let me take this opportunity to register my formal opposition to
   this function.  Either we do this or we don't.  Let's not hack like
   this. - CHD
   
   I'd like to second chris's opinion because I'm sick and tired of fixing
   this for every hw mod where it breaks. 
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
#define FN_NAME "ConvertAndDownloadRLE"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 89);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x, %d,%d,%d, %d,%d 0x%x, %d)\n",
                 tmu,startAddress,thisLod,largeLod,aspectRatio,
                 format,evenOdd,bm_data,dest_height);

  {
    GR_CHECK_TMU(FN_NAME, tmu);
    GR_CHECK_F(FN_NAME,
               startAddress + _grTexTextureMemRequired(thisLod, thisLod, aspectRatio, format, evenOdd, FXTRUE) > 
               gc->tmu_state[tmu].total_mem,
               "insufficient texture ram at startAddress");
    GR_CHECK_F(FN_NAME, startAddress & 0x7, "unaligned startAddress");
    GR_CHECK_F(FN_NAME, thisLod > G3_LOD_TRANSLATE(GR_LOD_1), "thisLod invalid");
    GR_CHECK_F(FN_NAME, largeLod > G3_LOD_TRANSLATE(GR_LOD_1), "largeLod invalid");
    GR_CHECK_F(FN_NAME, thisLod < largeLod, "thisLod may not be larger than largeLod");
    GR_CHECK_F(FN_NAME, aspectRatio > G3_ASPECT_TRANSLATE(GR_ASPECT_1x8) || 
               aspectRatio < G3_ASPECT_TRANSLATE(GR_ASPECT_8x1),
               "aspectRatio invalid");
    GR_CHECK_F(FN_NAME, evenOdd > 0x3 || evenOdd == 0, "evenOdd mask invalid");
    GR_CHECK_F(FN_NAME, !bm_data, "invalid data pointer");
    GR_CHECK_F(FN_NAME, (dest_height - 1) >= (FxU32)_grMipMapHostWH[aspectRatio][thisLod][1],
               "invalid end row");
  }

  /*------------------------------------------------------------
    Skip this level entirely if not in odd/even mask
    ------------------------------------------------------------*/
  if (!(evenOdd & (thisLod & 0x1 ? GR_MIPMAPLEVELMASK_ODD:GR_MIPMAPLEVELMASK_EVEN)))
    goto all_done;

  {
    FxU32
      baseAddress;

    /* Compute physical start address for the download. */
    {
      FxU32
        texOffset = 0x00UL;
      
      /* We need to do some magic to pack the small levels and have a
       * properly aligned baseAddr. If the current level is not going
       * to start on an alignment boundary when working backwards in
       * the chain we need to offset it into the block so that it is
       * addressable with an aligned baseAddr.  
       */
      {
        const FxU32
          aspectIndex = ((aspectRatio < GR_ASPECT_1x1) 
                         ? GR_ASPECT_1x1 - aspectRatio 
                         : aspectRatio - GR_ASPECT_1x1),
          lodIndex = ((thisLod == GR_LOD_256) ? GR_LOD_256 : thisLod - 1),
          formatShift = (format >= GR_TEXFMT_16BIT);
        FxU32
          levelSize = (_grMipMapHostSize[aspectIndex][lodIndex] << formatShift);
        GrLOD_t
          minLod = thisLod;
        
        if (levelSize < SST_TEXTURE_ALIGN) {
          GrLOD_t
            maxLod = lodIndex;
          
          /* Find the smallest level that naturally starts on an
           * alignment boundary. If this is larger than the current
           * mipmap chain's large lod then this we have to compute the
           * offset within this alignment unit.
           *
           * NB: This could a table lookup, but I'm writing the obvious code
           * right now so that there aren't any mystic #'s being pulled out
           * of the recesses of my colon.  
           */
          while(maxLod > GR_LOD_256) {
            levelSize = (_grMipMapHostSize[aspectIndex][maxLod] << formatShift);
            
            if (levelSize >= SST_TEXTURE_ALIGN) break;
            maxLod--;
            texOffset += levelSize;
          }

          /* maxLod is the index of the smallest level of this aspect
           * ratio that takes up atleast a full alignment unit.  We
           * reset the small lod to this so that we can compute the
           * offset for the 'large' levels in the mipmap chain.  
           */
          GR_ASSERT(maxLod != GR_LOD_256);
          minLod = maxLod + 1;
        } 

        if (minLod > largeLod) {
          texOffset += _grTexTextureMemRequired(minLod - 1, largeLod,
                                                aspectRatio,
                                                format,
                                                evenOdd,
                                                FXFALSE);
        }
      }
      
      /* This is going directly to the 2d lfb space rather than
       * through the texture port so we have to add in the actual hw
       * offset that the texture 'surface' starts at.
       */
      baseAddress = (gc->tramOffset[tmu] + 
                     startAddress +
                     texOffset);
    }

    {
      /*
      ** To maintain cache coherency in h4, issue a 2D nop to
      ** flush all pixel from 3D session before texture download
      */
      REG_GROUP_BEGIN_WAX(command, 1, 0x01UL);
      REG_GROUP_SET_WAX(gc->gRegs, command, SSTG_NOP | SSTG_GO);
      REG_GROUP_END();
    }

    /* Update stats for the unconverted format */
    _GlideRoot.stats.texBytes += dest_width * (dest_height) * 2;      

    {
      extern FxU16 
        rle_line[256], /* diTex.c */
        *rle_line_end; /* ditex.c */
      FxU32
        max_s,
        max_t;
      FxU32
        i,
        s, t,
        offset;
      const FxU16
        *src;

      
      if (height>dest_height)
         height=dest_height;
      if (width>dest_width)
         width=dest_width;

      max_s=dest_width >> 1;
      max_t=height;

      /* here I can do my writes and conversion and I will be so happy */
      offset = 4 + bm_h;
      for(i = 0; i < v0; i++)
        offset += bm_data[4 + i];
      
      rle_line_end = rle_line + width + u0;
      for(t = 0; t < max_t; t++) {
        src = rle_line + u0;
        rle_decode_line_asm(tlut, &bm_data[offset], rle_line);
        
        LINEAR_WRITE_BEGIN(max_s, SSTCP_PKT5_LFB, 
                           baseAddress,
                           0x00UL, 0x00UL);         
        for(s = 0; s < max_s; s++) {
          LINEAR_WRITE_SET(baseAddress + (s << 2), *(FxU32 *) src);
          src += 2;
        }
        LINEAR_WRITE_END();
        
        baseAddress += dest_width << 1;
        offset += bm_data[4 + i++];
      }
      
      if (dest_height > height) {
        src = rle_line + u0;
        
        LINEAR_WRITE_BEGIN(max_s, SSTCP_PKT5_LFB, 
                           baseAddress, 
                           0x00UL, 0x00UL);            
        for(s = 0; s < max_s; s++) {
          LINEAR_WRITE_SET(baseAddress + (s << 2), *(FxU32 *) src);
          src += 2;
        }
        LINEAR_WRITE_END();         
      }
    }

    /* If this is a small lod level in a texture replacing texels that
     * are already loaded then it may be necessary to flush the old
     * texels from memory before any other rendering operation using
     * this texture is issued. Unconditionally flush these old texels
     * just in case rather than being too clever.
     *
     * The reason that we need to flush here even though we're not
     * going through the texture port is because it is perfectly
     * legal to source once and download over and over again. (See
     * chd for a funny SpecOps story).
     *
     * NB: The documented nop does not currently work on basnehe which
     * is why we do the ~texBaseAddr crap.
     */
    {
      const FifoChipField 
        chipId = (FifoChipField)(0x02UL << tmu);
      const FxU32
        baseAddress = gc->state.tmu_config[tmu].texBaseAddr;
      
      GR_SET_EXPECTED_SIZE(3 * sizeof(FxU32), 3);
      {
        GR_SET(BROADCAST_ID, hw, nopCMD, 0);
        GR_SET(chipId, hw, texBaseAddr, ~baseAddress & SST_TEXTURE_ADDRESS);
        GR_SET(chipId, hw, texBaseAddr, baseAddress);
      }
      GR_CHECK_SIZE();

      /* Again force a pixel flush which should force all of the
       * texture downloads to flush from internal fifos etc.  
       */
      REG_GROUP_BEGIN_WAX(command, 1, 0x01UL);
      REG_GROUP_SET_WAX(gc->gRegs, command, SSTG_NOP | SSTG_GO);
      REG_GROUP_END();
    }
  }  

all_done:
  _GlideRoot.stats.texDownloads++;
#undef FN_NAME
}

