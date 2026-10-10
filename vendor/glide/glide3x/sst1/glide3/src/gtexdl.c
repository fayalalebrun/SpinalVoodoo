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
** Revision 1.1.2.3  2005/06/09 18:32:35  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.2.2  2004/10/04 09:36:00  dborca
** second cut at Glide3x for Voodoo1/Rush (massive update):
** delayed validation, vertex snapping, clip coordinates, strip/fan_continue, bugfixes.
** and probably a bunch of other crap that I forgot
**
** Revision 1.1.2.1  2004/03/02 07:55:30  dborca
** Bastardised Glide3x for SST1
**
** Revision 1.1.1.1  1999/12/07 21:48:53  joseph
** Initial checkin into SourceForge.
**
 * 
 * 28    6/19/97 7:35p Dow
 * More P6 Stuff
 * 
 * 27    6/10/97 3:59p Garymct
 * Fixed a bug in a sanity checking macro.
 * 
 * 26    6/10/97 11:09a Pgj
 * fix 8-bit & optimize
 * 
 * 25    6/08/97 11:06p Pgj
 * use Group Write for Texture Downloads
 * 
 * 24    5/09/97 5:12p Jdt
 * Removed redundant download of NCC Table
 * 
 * 23    3/18/97 9:07p Dow
 * Got rid of #$#%#$ // comments
 * 
 * 22    3/15/97 8:08p Jdt
 * Moved grTexDownloadTable into this file as it not precisely DI.
 * 
 * 21    3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 20    2/26/97 11:58a Jdt
 * fixed a warning
 * 
 * 19    2/19/97 4:25p Hanson
 * Added Gary T's changes to the expected size value for texture downloads
 * 
 * 18    2/18/97 9:52a Jdt
 * Fixed some warnings.
 * 
 * 17    2/12/97 2:09p Hanson
 * Hopefully removed the rest of my muckage. 
 * 
 * 16    2/12/97 12:34p Dow
 * Fixed Hanson muckage
 * 
 * 15    2/12/97 12:03p Hanson
 * Adjusted Convert and download code for Descent
 * 
 * 14    2/12/97 11:30a Hanson
 * Added ConvertRle function
 * 
 * 13    1/27/97 2:29p Murali
 * Changed texture download code to help compiler use registers optimally.
 * 
 * 12    12/23/96 1:37p Dow
 * chagnes for multiplatform glide
 * 
 * 11    11/18/96 9:13a Sellers
 * fixed bug in checking for crossing of 2 MByte texture boundry
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
extern const int _grMipMapHostWH[G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x8)+1][G3_LOD_TRANSLATE(GR_LOD_LOG2_1)+1][2];

/*---------------------------------------------------------------------------
** _grTexDownloadNccTable
**
** Downloads an ncctable to the specified _physical_ TMU(s).  This
** function is called internally by Glide and should not be executed
** by an application.
*/
GR_DDFUNC(_grTexDownloadNccTable, void, ( GrChipID_t tmu, FxU32 which, const GuNccTable *table, int start, int end ))
{
  int i;
  FxU32 *hwNCC;
  
  GR_BEGIN_NOFIFOCHECK("_grTexDownloadNccTable",89);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d, 0x%x, %d,%d)\n",tmu,which,table,start,end));
  GR_ASSERT( start==0 );
  GR_ASSERT( end==11 );

  /* check for null pointer */
  if ( table == 0 )
    return;
  _GlideRoot.stats.palDownloads++;
  _GlideRoot.stats.palBytes += (end-start+1)<<2;

  if (1/*gc->tmu_state[tmu].ncc_table[which] != table*/) {
    GR_SET_EXPECTED_SIZE(48+2*PACKER_WORKAROUND_SIZE);
    PACKER_WORKAROUND;
    hw = SST_TMU(hw,tmu);
    hwNCC = (which == 0)? hw->nccTable0 : hw->nccTable1;

    for ( i = 0; i < 12; i++ )
      GR_SET(hwNCC[i], table->packed_data[i] );

    gc->tmu_state[tmu].ncc_table[which] = table;
    PACKER_WORKAROUND;
    P6FENCE;
    GR_CHECK_SIZE();
  }
  GR_END();
} /* _grTexDownloadNccTable */

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

GR_ENTRY(grTexDownloadTable, void,
         ( GrTexTable_t type,  void *data ))
{
  GR_BEGIN_NOFIFOCHECK("grTexDownloadTable",89);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x)\n",type,data));
  /* GR_CHECK_TMU(myName,tmu); */
  GR_CHECK_F(myName, type > 0x2, "invalid table specified");
  GR_CHECK_F(myName, !data, "invalid data pointer");
#if ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  GR_CHECK_F(myName, _GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[0].tmuRev < 1,
               "Texelfx rev 0 does not support paletted textures");
#endif

  if ( type == GR_TEXTABLE_PALETTE )     /* Need Palette Download Code */
    _grTexDownloadPalette( GR_TMU0, (GuTexPalette *)data, 0, 255 );
  else {                                 /* Type is an ncc table */
    _grTexDownloadNccTable( GR_TMU0, type, (GuNccTable*)data, 0, 11 );
    /*    _grTexDownloadNccTable( tmu, type, (GuNccTable*)data, 0, 11 ); */
  }
  GR_END();
} /* grTexDownloadTable */

/*-------------------------------------------------------------------
  Function: grTexDownloadTableExt
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

GR_ENTRY(grTexDownloadTableExt, void,
         ( GrChipID_t tmu,  GrTexTable_t type,  void *data ))
{
  GR_BEGIN_NOFIFOCHECK("grTexDownloadTableExt",89);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x)\n",type,data));
  GR_CHECK_TMU(myName,tmu);
  GR_CHECK_F(myName, type > 0x2, "invalid table specified");
  GR_CHECK_F(myName, !data, "invalid data pointer");
#if ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  GR_CHECK_F(myName, _GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[0].tmuRev < 1,
               "Texelfx rev 0 does not support paletted textures");
#endif

  if ( type == GR_TEXTABLE_PALETTE )     /* Need Palette Download Code */
    _grTexDownloadPalette( tmu, (GuTexPalette *)data, 0, 255 );
  else {                                 /* Type is an ncc table */
    _grTexDownloadNccTable( tmu, type, (GuNccTable*)data, 0, 11 );
    /*    _grTexDownloadNccTable( tmu, type, (GuNccTable*)data, 0, 11 ); */
  }
  GR_END();
} /* grTexDownloadTableExt */

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

GR_ENTRY(grTexDownloadMipMapLevelPartial, FxBool, ( GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod, GrLOD_t largeLod, GrAspectRatio_t   aspectRatio, GrTextureFormat_t format, FxU32 evenOdd, void *data, int t, int max_t ))
{
  const FxU8  *src8  = ( const FxU8  * ) data; 
  const FxU16 *src16 = ( const FxU16 * ) data;
  FxI32   sh, bytesPerTexel;
  FxU32 max_s, s, width, tex_address, tmu_baseaddress;
  FxU32 tLod, texMode, baseAddress,size;

  GR_BEGIN_NOFIFOCHECK("grTexDownloadMipMapLevelPartial",89);
  thisLod = G3_LOD_TRANSLATE(thisLod);
  largeLod = G3_LOD_TRANSLATE(largeLod);
  aspectRatio = G3_ASPECT_TRANSLATE(aspectRatio);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x, %d,%d,%d, %d,%d 0x%x, %d,%d)\n",
                  tmu,startAddress,thisLod,largeLod,aspectRatio,
                  format,evenOdd,data,t,max_t));
  size = _grTexTextureMemRequired(thisLod, thisLod, aspectRatio, format, evenOdd);
  GR_CHECK_TMU(myName, tmu);
  GR_CHECK_F(myName, startAddress + size > gc->tmu_state[tmu].total_mem,
               "insufficient texture ram at startAddress" );
  GR_CHECK_F(myName, startAddress & 0x7, "unaligned startAddress");
  GR_CHECK_F(myName, thisLod > G3_LOD_TRANSLATE(GR_LOD_LOG2_1), "thisLod invalid");
  GR_CHECK_F(myName, largeLod > G3_LOD_TRANSLATE(GR_LOD_LOG2_1), "largeLod invalid");
  GR_CHECK_F(myName, thisLod < largeLod, "thisLod may not be larger than largeLod");
  GR_CHECK_F(myName, aspectRatio > G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x8) || 
             aspectRatio < G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_8x1), "aspectRatio invalid");
  GR_CHECK_F(myName, evenOdd > 0x3 || evenOdd == 0, "evenOdd mask invalid");
  GR_CHECK_F(myName, !data, "invalid data pointer");
  GR_CHECK_F(myName, max_t >= _grMipMapHostWH[aspectRatio][thisLod][1], "invalid end row");

  if ((startAddress < 0x200000) && (startAddress + size > 0x200000))
    GrErrorCallback("grTexDownloadMipMapLevelPartial: mipmap level cannot span 2 Mbyte boundary",FXTRUE);

  /*------------------------------------------------------------
    Skip this level entirely if not in odd/even mask
    ------------------------------------------------------------*/
  if ( !(evenOdd & (thisLod & 0x1 ? GR_MIPMAPLEVELMASK_ODD:GR_MIPMAPLEVELMASK_EVEN)))
      goto all_done;

  /*------------------------------------------------------------
    Determine max_s
    ------------------------------------------------------------*/
  width = _grMipMapHostWH[aspectRatio][thisLod][0];
  if ( format < GR_TEXFMT_16BIT ) { /* 8-bit texture */
    bytesPerTexel = 1;
    max_s = width >> 2;
    if (max_s < 1)
      max_s = 1;
  } else { /* 16-bit texture */
    bytesPerTexel = 2;
    max_s = width >> 1;
    if (max_s < 1)
      max_s = 1;
  }
  /* assume max_s is a power of two */
  GR_ASSERT(( (max_s) & (max_s -1) ) == 0);

  
  /*------------------------------------------------------------
    Compute Base Address Given Start Address Offset
    ------------------------------------------------------------*/
  baseAddress = _grTexCalcBaseAddress( startAddress,
                                       largeLod, 
                                       aspectRatio,
                                       format,
                                       evenOdd );
  baseAddress >>= 3;

  /*------------------------------------------------------------
    Compute Physical Write Pointer
    ------------------------------------------------------------*/
  tmu_baseaddress = (FxU32)gc->tex_ptr;
  tmu_baseaddress += (((FxU32)tmu)<<21) + (((FxU32)thisLod)<<17);
  
  /*------------------------------------------------------------
    Compute pertinant contents of tLOD and texMode registers 
    ------------------------------------------------------------*/
  tLod = SST_TLOD_MINMAX_INT(largeLod,G3_LOD_TRANSLATE(GR_LOD_LOG2_1));
  tLod |= _gr_evenOdd_xlate_table[evenOdd];
  tLod |= _gr_aspect_xlate_table[aspectRatio];
  texMode = format << SST_TFORMAT_SHIFT;
  if (gc->state.tmu_config[tmu].textureMode & SST_SEQ_8_DOWNLD) {
    sh = 2;
    texMode |= SST_SEQ_8_DOWNLD;
  }
  else sh = 3;

  /* account for 3 register writes and for smallest 1xN and 2xN levels */
  /* and also 4xN level for 8-bit textures (or 4x32x8bpp) */
  /* Also note that each texture write requires 10 actual fifo entry bytes */
  /* but since we are counting bytes/2 we multiply by 5 */
  GR_SET_EXPECTED_SIZE(3*4 + 2*PACKER_WORKAROUND_SIZE + 32*5);

  /*------------------------------------------------------------
    Update TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  PACKER_WORKAROUND;
  hw = SST_TMU(hw,tmu);
  GR_SET( hw->texBaseAddr , baseAddress );
  GR_SET( hw->textureMode , texMode );
  GR_SET( hw->tLOD , tLod );
  PACKER_WORKAROUND;

  /* Flush the write buffers before the texture downloads */
  P6FENCE;
  _GlideRoot.stats.texBytes += max_s * (max_t-t+1) * 4;

# define SET_TRAM(a,b) GR_SET( *((FxU32 *)(a)) , (b) )
  /*------------------------------------------------------------
    Handle 8-bit Textures
    ------------------------------------------------------------*/
  if ( format < GR_TEXFMT_16BIT ) { /* 8 bit textures */
    switch( width ) {
    /* Cases 1, 2 and 4 don't need inner loops for s */
    case 1:                         /* 1xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *(const FxU8*) src8);
        src8 += 1;
        tex_address += (1 << 9); 
      }
      break;

    case 2:                         /* 2xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *(const FxU16*) src8);
        src8 += 2;
        tex_address += (1 << 9); 
      }
      break;

    case 4:                         /* 4xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *(const FxU32*) src8);
        src8 += 4;
        tex_address += (1 << 9); 
      }
      break;

    default:                        /* >4xN texture */
      if (sh == 3) {                /* Old TMUs */
        /* Inner loop unrolled to process 2 dwords per iteration */
        for ( ; t <= max_t; t++) {
          GR_CHECK_SIZE_SLOPPY();
          GR_SET_EXPECTED_SIZE(max_s*5);
          tex_address = tmu_baseaddress + ( t << 9 );
          for ( s = 0; s < max_s; s+=2) {
            FxU32  t0, t1;

            t0 = * (const FxU32 *) (src8    );
            t1 = * (const FxU32 *) (src8 + 4);
            SET_TRAM( tex_address    , t0);
            SET_TRAM( tex_address + 8, t1);
            tex_address += 16;
            src8 += 8;
          }
        } 
      } else {                      /* New TMUs    */
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)

#define DW_PER_GWP 32
#define W_PER_GWP (DW_PER_GWP << 1)
#define BYTES_PER_GWP (DW_PER_GWP << 2)
#define MASK(n) ((1 << (n)) - 1)

      for ( ; t <= max_t; t++ ) {
        FxU32 t0, t1;
        FxU32 j;

        GR_CHECK_SIZE_SLOPPY();
        GR_SET_EXPECTED_SIZE((max_s + (max_s >> 4) + 2) << 2);
        tex_address = tmu_baseaddress + ( t << 9 );
        if (max_s >= DW_PER_GWP) {              /* can use maximum GWP(s) */
          for (s=0; s<max_s; s+=DW_PER_GWP ) {

            GWH_BEGIN_TEXDL_PACKET(0xffffffff, tex_address);
            /* Loop unrolled to keep GWP happy */
            for (j=0; j<16; j++) {
              t0 = * (const FxU32 *) (src8 + (j << 3) );
              t1 = * (const FxU32 *) (src8 + (j << 3) + 4);
              GR_SET_GW(t0);
              GR_SET_GW(t1);
            }
            tex_address += BYTES_PER_GWP;
            src8 += BYTES_PER_GWP;
          } /* end for s */
        } else {                        /* partial GWP */
          FxU32 mask = MASK(max_s);     /* we can assume s is even */
          GWH_BEGIN_TEXDL_PACKET(mask, tex_address);
          /* Loop unrolled to keep GWP happy */
          for (j=0; j<(max_s>>1); j++) {
            t0 = * (const FxU32 *) (src8 + (j << 3) );
            t1 = * (const FxU32 *) (src8 + (j << 3) + 4);
            GR_SET_GW(t0);
            GR_SET_GW(t1);
          }
          tex_address += (max_s << 2);
          src8 += (max_s << 2);
        }
      } /* end for t */

#else   /* SST-1 */
        for ( ; t <= max_t; t++) {
          GR_CHECK_SIZE_SLOPPY();
          GR_SET_EXPECTED_SIZE(max_s*5);
          tex_address = tmu_baseaddress + ( t << 9 );
          for ( s = 0; s < max_s; s+=2) {
            FxU32  t0, t1;

            t0 = * (const FxU32 *) (src8    );
            t1 = * (const FxU32 *) (src8 + 4);
            SET_TRAM( tex_address    , t0);
            SET_TRAM( tex_address + 4, t1);
            tex_address += 8;
            src8 += 8;
          }
        } 
#endif
      }
      break;
    }
  } else { 

    /*------------------------------------------------------------
      Handle 16-bit Textures
      ------------------------------------------------------------*/

    switch( width ) {
    /* Cases 1, 2 don't need inner loops for s */
    case 1:                         /* 1xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *src16 );
        src16 += 1;
        tex_address += (1 << 9); 
      }
      break;

    case 2:                         /* 2xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *(const FxU32 *)src16 );
        src16 += 2;
        tex_address += (1 << 9); 
      }
      break;

    default:                        /* All other textures */
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
      for ( ; t <= max_t; t++ ) {
        FxU32 t0, t1;
        FxU32 j;

        GR_CHECK_SIZE_SLOPPY();
        GR_SET_EXPECTED_SIZE((max_s + (max_s >> 4) + 2) << 2);
        tex_address = tmu_baseaddress + ( t << 9 );
        if (max_s >= DW_PER_GWP) {              /* can use maximum GWP(s) */
          for (s=0; s<max_s; s+=DW_PER_GWP ) {

            GWH_BEGIN_TEXDL_PACKET(0xffffffff, tex_address);
            /* Loop unrolled to keep GWP happy */
            for (j=0; j<16; j++) {
              t0 = * (const FxU32 *) (src16 + (j << 2) );
              t1 = * (const FxU32 *) (src16 + (j << 2) + 2);
              GR_SET_GW(t0);
              GR_SET_GW(t1);
            }
            tex_address += BYTES_PER_GWP;
            src16 += W_PER_GWP;
          } /* end for s */
        } else {                        /* partial GWP */
          FxU32 mask = MASK(max_s);     /* we can assume s is even */
          GWH_BEGIN_TEXDL_PACKET(mask, tex_address);
          /* Loop unrolled to keep GWP happy */
          for (j=0; j<(max_s>>1); j++) {
            t0 = * (const FxU32 *) (src16 + (j << 2) );
            t1 = * (const FxU32 *) (src16 + (j << 2) + 2);
            GR_SET_GW(t0);
            GR_SET_GW(t1);
          }
          tex_address += (max_s << 2);
          src16 += (max_s << 1);
        }
      } /* end for t */
#else   /* SST-1 */
      for ( ; t <= max_t; t++ ) {
        GR_CHECK_SIZE_SLOPPY();
        GR_SET_EXPECTED_SIZE(max_s*5);
        tex_address = tmu_baseaddress + ( t << 9 );
        /* Loop unrolled to process 2 dwords per iteration */
        for ( s = 0; s < max_s; s += 2 ) {
          FxU32  t0, t1;

          t0 = * (const FxU32 *) (src16    );
          t1 = * (const FxU32 *) (src16 + 2);
          SET_TRAM( tex_address    , t0);
          SET_TRAM( tex_address + 4, t1);
          tex_address += 8;
          src16 += 4;
        }
      }
#endif
      break;
    }
  } /* end switch( width ) */
  
  /* Flush the write buffers after the texture downloads */
  P6FENCE;

  /*------------------------------------------------------------
    Restore TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  GR_CHECK_SIZE_SLOPPY();
  GR_SET_EXPECTED_SIZE(3*4 + 2*PACKER_WORKAROUND_SIZE);
  PACKER_WORKAROUND;
  GR_SET( hw->texBaseAddr , gc->state.tmu_config[tmu].texBaseAddr );
  GR_SET( hw->textureMode , gc->state.tmu_config[tmu].textureMode );
  GR_SET( hw->tLOD        , gc->state.tmu_config[tmu].tLOD );
  PACKER_WORKAROUND;

all_done:
  _GlideRoot.stats.texDownloads++;
  GR_END_SLOPPY();
  return FXTRUE;
} /* grTexDownloadMipmapLevelPartial */


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
GR_DDFUNC(_grTexDownloadPalette, void, ( GrChipID_t tmu, GuTexPalette *pal, int start, int end ))
{
  GR_BEGIN("_grTexDownloadPalette",89, 4*(end-start+1) + 2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x, %d,%d)\n",tmu,pal,start,end));
  GR_CHECK_F( myName, !pal, "pal invalid" );
  GR_CHECK_F( myName, start<0, "invalid start index" );
  GR_CHECK_F( myName, end>255, "invalid end index" );

  PACKER_WORKAROUND;
  hw = SST_TMU(hw,tmu);
  _GlideRoot.stats.palDownloads++;
  _GlideRoot.stats.palBytes += (end-start+1)<<2;

  while (start <= end) {
      GR_SET( hw->nccTable0[4+(start&0x7)],
              0x80000000 |  ((start & 0xFE) << 23) | (pal->data[start] & 0xffffff) );
      start++;
      if ((start&0x7)==0) P6FENCE;
  }

  PACKER_WORKAROUND;
  P6FENCE;
  GR_END();
} /* _grTexDownloadPalette */ 
