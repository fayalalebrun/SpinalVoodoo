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
**  3    3dfx      1.0.1.0.1.0 10/11/00 Brent           Forced check in to enforce
**       branching.
**  2    3dfx      1.0.1.0     06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 12    8/18/99 3:20p Larryw
** FXT1 refinements.
** 
** 11    7/29/99 7:07p Larryw
** Pave the way for FXT1 (but not quite there yet).
** 
** 10    7/22/99 8:14p Larryw
** Texture format byte-depth improvements
** 
** 9     7/14/99 6:23p Larryw
** Remove obsolete G3_LOD_TRANSLATE() macro
** Define _grMipMapOffset[][] at compile time
** Fix 2k texture address-finding
** 
** 8     7/07/99 6:52p Larryw
** * 2k texture support
** * Reversed order of LOD tables
** * Added 512,1024, and 2048-sized entries in tables
** * Nullified G3_LOD_TRANSLATE()
** * Created _g3LodXlat() for where tLOD register is read/written
** * Misc cosmetic changes.
** 
** 7     6/25/99 2:15p Atai
** use g3ext.h
** 
** 6     6/14/99 5:16p Larryw
** Added 32-bit texture format support.
** 
** 5     1/25/99 6:32p Peter
** cleaned up some translation macros and tables
** 
** 3     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
** 
** 2     1/18/98 12:03p Atai
** sync to rev 17 spec
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 16    1/13/98 7:48p Atai
 * fixed gu3dfGetInfo, grBufferClear, and GrState size
 * 
 * 15    1/13/98 12:42p Atai
 * fixed grtexinfo, grVertexLayout, and draw triangle
 * 
 * 14    1/09/98 6:48p Atai
 * grTexInfo, GR_LOD_* and GR_ASPECT_*
 * 
 * 12    12/17/97 4:45p Peter
 * groundwork for CrybabyGlide
 * 
 * 11    12/09/97 12:20p Peter
 * mac glide port
 * 
 * 10    6/06/97 5:56p Peter
 * fixed gcc/dural compilation things
 * 
 * 9     6/02/97 4:09p Peter
 * Compile w/ gcc for Dural
 * 
 * 8     5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 7     5/21/97 6:05a Peter
 * 
 * 6     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
**
*/
#include <stdio.h>
#include <string.h>
#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"

#if 0 /* unused */
static const FxU32 _gr_aspect_index_table[] =
{
   3,
   2,
   1,
   0,
   1,
   2,
   3,
};
#endif

static FxBool ReadDataShort(FILE *, FxU16 *data);
static FxBool ReadDataLong(FILE *, FxU32 *data);
static FxBool Read4Bit(FxU8 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect);
static FxBool Read8Bit(FxU8 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect);
/* static FxBool ReadDXT4Bit(FxU8 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect);
   static FxBool ReadDXT8Bit(FxU8 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect); */
static FxBool Read16Bit(FxU16 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect);
static FxBool Read32Bit(FxU32 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect);

#if ((GLIDE_PLATFORM & (GLIDE_OS_DOS32 | GLIDE_OS_WIN32 | GLIDE_OS_MACOS)) != 0)
static const char *openmode = "rb";
#else
static const char *openmode = "r";
#endif

typedef struct
{
  const char        *name;
  GrTextureFormat_t  fmt;
  FxBool             valid;
} CfTableEntry;

static FxBool 
_grGet3dfHeader(FILE* stream, char* const buffer, const FxU32 bufSize)
{
  int numLines = 0;
  FxU32 bufPos = 0;
  
  while(numLines < 4) {
    /* Handle stream errors */
    if (fgets(buffer + bufPos, bufSize - bufPos, stream) == NULL) break;
    bufPos += strlen(buffer + bufPos);
    
    /* fgets includes the '\n' in the buffer. If this is not there
     * then the buffer is too small so fail.
     */
    if (*(buffer + bufPos - sizeof(char)) != '\n') break;
    numLines++;
  }

  return (numLines == 4);
}

/*---------------------------------------------------------------------------
** gu3dfGetInfo
*/
GR_DIENTRY(gu3dfGetInfo, FxBool,
           (const char *FileName, Gu3dfInfo *Info))
{
#define FN_NAME "gu3dfGetInfo"
  FILE *image_file;
  FxU32 i;
  char  version[5];
  char  color_format[10];
  int   aspect_width, aspect_height;
  char  buffer[100];
  int   small_lod, large_lod;
  FxBool ratio_found = FXFALSE;
  FxBool format_found = FXFALSE;
#ifndef GLIDE3_ALPHA
  GrAspectRatio_t wh_aspect_table[] =
  {
    GR_ASPECT_1x1,
    GR_ASPECT_1x2,
    GR_ASPECT_1x4,
    GR_ASPECT_1x8
  };
  GrAspectRatio_t hw_aspect_table[] =
  {
    GR_ASPECT_1x1,
    GR_ASPECT_2x1,
    GR_ASPECT_4x1,
    GR_ASPECT_8x1
  };
#else
  GrAspectRatio_t wh_aspect_table[] =
  {
    GR_ASPECT_LOG2_1x1,
    GR_ASPECT_LOG2_1x2,
    GR_ASPECT_LOG2_1x4,
    GR_ASPECT_LOG2_1x8
  };
  GrAspectRatio_t hw_aspect_table[] =
  {
    GR_ASPECT_LOG2_1x1,
    GR_ASPECT_LOG2_2x1,
    GR_ASPECT_LOG2_4x1,
    GR_ASPECT_LOG2_8x1
  };
#endif
  CfTableEntry cftable[] = 
  {
    { "I8",       GR_TEXFMT_INTENSITY_8,        FXTRUE },
    { "A8",       GR_TEXFMT_ALPHA_8,            FXTRUE },
    { "AI44",     GR_TEXFMT_ALPHA_INTENSITY_44, FXTRUE },
    { "YIQ",      GR_TEXFMT_YIQ_422,            FXTRUE },
    { "RGB332",   GR_TEXFMT_RGB_332,            FXTRUE },
    { "RGB565",   GR_TEXFMT_RGB_565,            FXTRUE },
    { "ARGB8332", GR_TEXFMT_ARGB_8332,          FXTRUE },
    { "ARGB1555", GR_TEXFMT_ARGB_1555,          FXTRUE },
    { "AYIQ8422", GR_TEXFMT_AYIQ_8422,          FXTRUE },
    { "ARGB4444", GR_TEXFMT_ARGB_4444,          FXTRUE },
    { "AI88",     GR_TEXFMT_ALPHA_INTENSITY_88, FXTRUE },
    { "P8",       GR_TEXFMT_P_8,                FXTRUE },
    { "AP88",     GR_TEXFMT_AP_88,              FXTRUE },
    { "ARGB8888", GR_TEXFMT_ARGB_8888,          FXTRUE },
#ifdef FX_GLIDE_NAPALM
    /* other texture formats. */
    { "FXT1",        GR_TEXFMT_ARGB_CMP_FXT1,   FXTRUE },
    { "FXT1_HI",     GR_TEXFMT_ARGB_CMP_FXT1,   FXTRUE },
    { "FXT1_MIXED",  GR_TEXFMT_ARGB_CMP_FXT1,   FXTRUE },
    { "FXT1_CHROMA", GR_TEXFMT_ARGB_CMP_FXT1,   FXTRUE },
    { "FXT1_ALPHA",  GR_TEXFMT_ARGB_CMP_FXT1,   FXTRUE },
    { "P6666",    GR_TEXFMT_P_8_6666,           FXTRUE },
  /*{ "RSVD1",    GR_TEXFMT_RSVD1,              FXTRUE },
    { "RSVD2",    GR_TEXFMT_RSVD2,              FXTRUE },
    { "RSVD4",    GR_TEXFMT_RSVD4,              FXTRUE },*/
    { "YUYV422",  GR_TEXFMT_YUYV_422,           FXTRUE },
    { "UYVY22",   GR_TEXFMT_UYVY_422,           FXTRUE },
    { "AYUV444",  GR_TEXFMT_AYUV_444,           FXTRUE },
  /* TODO: to support DXTn, we need to read .dds files.
    { "DXT1",     GR_TEXFMT_ARGB_CMP_DXT1,      FXTRUE },
    { "DXT2",     GR_TEXFMT_ARGB_CMP_DXT2,      FXTRUE },
    { "DXT3",     GR_TEXFMT_ARGB_CMP_DXT3,      FXTRUE },
    { "DXT4",     GR_TEXFMT_ARGB_CMP_DXT4,      FXTRUE },
    { "DXT5",     GR_TEXFMT_ARGB_CMP_DXT5,      FXTRUE },*/
#endif
    { 0, 0, FXFALSE }
  };

  GDBG_INFO(81,"gu3dfGetInfo(%s,0x%x)\n",FileName,Info);

  if ((image_file = fopen(FileName, openmode)) == NULL) return FXFALSE;
  if (!_grGet3dfHeader(image_file, buffer, sizeof(buffer))) goto _loc1;

  /*
  ** grab statistics out of the header
  */
  if(sscanf(buffer,"3df v%s %s lod range: %i %i aspect ratio: %i %i\n",
            version,
            color_format,
            &small_lod, &large_lod,
            &aspect_width, &aspect_height) != 6)
    goto _loc1;

  /*
  ** determine aspect ratio, height, and width
  */
  i = 0;
  while ((i < 4) && (!ratio_found)) {
    if ((aspect_width << i) == aspect_height) {
      Info->header.aspect_ratio = wh_aspect_table[i];
      ratio_found = FXTRUE;
    }

    i++;
  }

  i = 0;
  while ((i < 4) && (!ratio_found)) {
    if ((aspect_height << i) == aspect_width) {
      Info->header.aspect_ratio = hw_aspect_table[i];
      ratio_found = FXTRUE;
    }

    i++;
  }
  if (!ratio_found) goto _loc1;

  /*
  ** determine height and width of the mip map
  */
  if (aspect_width >= aspect_height) {
    Info->header.width  = large_lod;
    Info->header.height = large_lod / aspect_width;
  } else {
    Info->header.height = large_lod;
    Info->header.width  = large_lod / aspect_height;
  }


  /*
  ** calculate proper LOD values
  */
  /*
  ** note for glide3 lod translation:
  ** we should return the new defines for glide3
  */
#ifndef GLIDE3_ALPHA
  switch (small_lod) {
  case 1:
    Info->header.small_lod = GR_LOD_1;
    break;

  case 2:
    Info->header.small_lod = GR_LOD_2;
    break;

  case 4:
    Info->header.small_lod = GR_LOD_4;
    break;

  case 8:
    Info->header.small_lod = GR_LOD_8;
    break;

  case 16:
    Info->header.small_lod = GR_LOD_16;
    break;

  case 32:
    Info->header.small_lod = GR_LOD_32;
    break;

  case 64:
    Info->header.small_lod = GR_LOD_64;
    break;

  case 128:
    Info->header.small_lod = GR_LOD_128;
    break;

  case 256:
    Info->header.small_lod = GR_LOD_256;
    break;

  case 512:
    Info->header.small_lod = GR_LOD_512;
    break;

  case 1024:
    Info->header.small_lod = GR_LOD_1024;
    break;

  case 2048:
    Info->header.small_lod = GR_LOD_2048;
    break;
  }

  switch (large_lod) {
  case 1:
    Info->header.large_lod = GR_LOD_1;
    break;

  case 2:
    Info->header.large_lod = GR_LOD_2;
    break;

  case 4:
    Info->header.large_lod = GR_LOD_4;
    break;

  case 8:
    Info->header.large_lod = GR_LOD_8;
    break;

  case 16:
    Info->header.large_lod = GR_LOD_16;
    break;

  case 32:
    Info->header.large_lod = GR_LOD_32;
    break;

  case 64:
    Info->header.large_lod = GR_LOD_64;
    break;

  case 128:
    Info->header.large_lod = GR_LOD_128;
    break;

  case 256:
    Info->header.large_lod = GR_LOD_256;
    break;

  case 512:
    Info->header.large_lod = GR_LOD_512;
    break;

  case 1024:
    Info->header.large_lod = GR_LOD_1024;
    break;

  case 2048:
    Info->header.large_lod = GR_LOD_2048;
    break;
  }
#else  /* GLIDE3_ALPHA */
  switch (small_lod) {
  case 1:
    Info->header.small_lod = GR_LOD_LOG2_1;
    break;
  case 2:
    Info->header.small_lod = GR_LOD_LOG2_2;
    break;
  case 4:
    Info->header.small_lod = GR_LOD_LOG2_4;
    break;
  case 8:
    Info->header.small_lod = GR_LOD_LOG2_8;
    break;
  case 16:
    Info->header.small_lod = GR_LOD_LOG2_16;
    break;
  case 32:
    Info->header.small_lod = GR_LOD_LOG2_32;
    break;
  case 64:
    Info->header.small_lod = GR_LOD_LOG2_64;
    break;
  case 128:
    Info->header.small_lod = GR_LOD_LOG2_128;
    break;
  case 256:
    Info->header.small_lod = GR_LOD_LOG2_256;
    break;
  case 512:
    Info->header.small_lod = GR_LOD_LOG2_512;
    break;
  case 1024:
    Info->header.small_lod = GR_LOD_LOG2_1024;
    break;
  case 2048:
    Info->header.small_lod = GR_LOD_LOG2_2048;
    break;
  }

  switch (large_lod) {
  case 1:
    Info->header.large_lod = GR_LOD_LOG2_1;
    break;
  case 2:
    Info->header.large_lod = GR_LOD_LOG2_2;
    break;
  case 4:
    Info->header.large_lod = GR_LOD_LOG2_4;
    break;
  case 8:
    Info->header.large_lod = GR_LOD_LOG2_8;
    break;
  case 16:
    Info->header.large_lod = GR_LOD_LOG2_16;
    break;
  case 32:
    Info->header.large_lod = GR_LOD_LOG2_32;
    break;
  case 64:
    Info->header.large_lod = GR_LOD_LOG2_64;
    break;
  case 128:
    Info->header.large_lod = GR_LOD_LOG2_128;
    break;
  case 256:
    Info->header.large_lod = GR_LOD_LOG2_256;
    break;
  case 512:
    Info->header.large_lod = GR_LOD_LOG2_512;
    break;
  case 1024:
    Info->header.large_lod = GR_LOD_LOG2_1024;
    break;
  case 2048:
    Info->header.large_lod = GR_LOD_LOG2_2048;
    break;
  }
#endif /* !GLIDE3_ALPHA */

  /*
  ** determine the color format of the input image
  */
  {
    char *tempStr = (char*)color_format;
    while (*tempStr != '\0') {
          if (*tempStr >= 'a' && *tempStr <= 'z')
              *tempStr -= ('a'-'A');
          tempStr++;
    }
  }

  i = 0;
  while ((cftable[i].name != 0) && (!format_found)) {
    if (strcmp(color_format, cftable[i].name) == 0) {
      Info->header.format = cftable[i].fmt;
      format_found = FXTRUE;
    }

    i++;
  }

  /*
  ** close the input file
  */
 _loc1:
  fclose(image_file);

  if (format_found) {
    Info->mem_required = _grTexTextureMemRequired(Info->header.small_lod,
                                                  Info->header.large_lod,
                                                  Info->header.aspect_ratio,
                                                  Info->header.format,
                                                  GR_MIPMAPLEVELMASK_BOTH,
                                                  FXFALSE);
  }

  GDBG_INFO(81,"gu3dfGetInfo(%s,0x%x) -> %i tex memory required\n",FileName,Info, Info->mem_required);
  return format_found;
#undef FN_NAME
} /* gu3dfGetInfo() */

/*---------------------------------------------------------------------------
** gu3dfLoad
*/
GR_DIENTRY(gu3dfLoad, FxBool, (const char *filename, Gu3dfInfo *info))
{
  FILE *image_file  = 0;
  FxU32 index       = 0;
  char  buffer[100] = "";

  GDBG_INFO(81,"gu3dfLoad(%s,0x%x)\n",filename,info);

  if ((image_file = fopen(filename, openmode)) == NULL) return FXFALSE;
  if (!_grGet3dfHeader(image_file, buffer, sizeof(buffer))) goto _loc1;

#if 0
  /*
  ** If necessary, read in the YIQ decompression table
  */
  if ((info->header.format == GR_TEXFMT_YIQ_422) ||
      (info->header.format == GR_TEXFMT_AYIQ_8422))
  {
    /*
    ** read in Y
    */
    for (index = 0; index < 16; index++)
    {
      FxU16 val;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.yRGB[index] = val & 0xFF;
    }

    /*
    ** read in I
    */
    for (index = 0; index < 4; index++) {
      FxU16 val;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.iRGB[index][0] = val & 0x1FF;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.iRGB[index][1] = val & 0x1FF;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.iRGB[index][2] = val & 0x1FF;
    }

    /*
    ** read in Q
    */
    for (index = 0; index < 4; index++) {
      FxU16 val;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.qRGB[index][0] = val & 0x1FF;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.qRGB[index][1] = val & 0x1FF;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.qRGB[index][2] = val & 0x1FF;
    }

    /*
    ** pack the table Y entries
    */
    for (index = 0; index < 4; index++)
    {
      FxU32 packedvalue;

      packedvalue  = ((FxU32) info->table.nccTable.yRGB[index*4+0]);
      packedvalue |= ((FxU32) info->table.nccTable.yRGB[index*4+1]) << 8;
      packedvalue |= ((FxU32) info->table.nccTable.yRGB[index*4+2]) << 16;
      packedvalue |= ((FxU32) info->table.nccTable.yRGB[index*4+3]) << 24;

      info->table.nccTable.packed_data[index] = packedvalue;
    }

    /*
    ** pack the table I entries
    */
    for (index = 0; index < 4; index++) {
      FxU32 packedvalue;

      packedvalue  = ((FxU32) info->table.nccTable.iRGB[index][0]) << 18;
      packedvalue |= ((FxU32) info->table.nccTable.iRGB[index][1]) << 9;
      packedvalue |= ((FxU32) info->table.nccTable.iRGB[index][2]) << 0;

      info->table.nccTable.packed_data[index+4] = packedvalue;
    }

    /*
    ** pack the table Q entries
    */
    for (index = 0; index < 4; index++) {
      FxU32 packedvalue;

      packedvalue  = ((FxU32) info->table.nccTable.qRGB[index][0]) << 18;
      packedvalue |= ((FxU32) info->table.nccTable.qRGB[index][1]) << 9;;
      packedvalue |= ((FxU32) info->table.nccTable.qRGB[index][2]) << 0;

      info->table.nccTable.packed_data[index+8] = packedvalue;
    }
  }

  /*
  ** If necessary, read in the Palette
  */
  if ((info->header.format == GR_TEXFMT_P_8) ||
      (info->header.format == GR_TEXFMT_AP_88)) {
    FxU32 i;
    for(i = 0; i < 256; i++)
    {
      FxU32 val;
      if (!ReadDataLong(image_file, &val)) goto _loc1;
      info->table.palette.data[i] = val;
    }
  }
#endif

  /*
  ** Read in the image
  */
  switch (info->header.format)
  {
  case GR_TEXFMT_YIQ_422:
     /*
     ** If necessary, read in the YIQ decompression table
     */
    {
      /*
       ** read in Y
       */
      for (index = 0; index < 16; index++)
      {
        FxU16 val;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.yRGB[index] = val & 0xFF;
      }

      /*
       ** read in I
       */
      for (index = 0; index < 4; index++) {
        FxU16 val;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.iRGB[index][0] = val & 0x1FF;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.iRGB[index][1] = val & 0x1FF;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.iRGB[index][2] = val & 0x1FF;
      }

      /*
       ** read in Q
       */
      for (index = 0; index < 4; index++) {
        FxU16 val;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.qRGB[index][0] = val & 0x1FF;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.qRGB[index][1] = val & 0x1FF;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.qRGB[index][2] = val & 0x1FF;
      }

      /*
       ** pack the table Y entries
       */
      for (index = 0; index < 4; index++) {
        FxU32 packedvalue;

        packedvalue  = ((FxU32) info->table.nccTable.yRGB[index*4+0]);
        packedvalue |= ((FxU32) info->table.nccTable.yRGB[index*4+1]) << 8;
        packedvalue |= ((FxU32) info->table.nccTable.yRGB[index*4+2]) << 16;
        packedvalue |= ((FxU32) info->table.nccTable.yRGB[index*4+3]) << 24;

        info->table.nccTable.packed_data[index] = packedvalue;
      }

      /*
       ** pack the table I entries
       */
      for (index = 0; index < 4; index++) {
        FxU32 packedvalue;

        packedvalue  = ((FxU32) info->table.nccTable.iRGB[index][0]) << 18;
        packedvalue |= ((FxU32) info->table.nccTable.iRGB[index][1]) << 9;
        packedvalue |= ((FxU32) info->table.nccTable.iRGB[index][2]) << 0;

        info->table.nccTable.packed_data[index+4] = packedvalue;
      }

      /*
       ** pack the table Q entries
       */
      for (index = 0; index < 4; index++) {
        FxU32 packedvalue;

        packedvalue  = ((FxU32) info->table.nccTable.qRGB[index][0]) << 18;
        packedvalue |= ((FxU32) info->table.nccTable.qRGB[index][1]) << 9;;
        packedvalue |= ((FxU32) info->table.nccTable.qRGB[index][2]) << 0;

        info->table.nccTable.packed_data[index+8] = packedvalue;
      }
    }

    if (!Read8Bit(info->data, image_file, 
             info->header.small_lod, 
             info->header.large_lod, 
             G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
       goto _loc1;
    break;

  case GR_TEXFMT_AYIQ_8422:
    /*
     ** If necessary, read in the YIQ decompression table
     */
    {
      /*
       ** read in Y
       */
      for (index = 0; index < 16; index++)
      {
        FxU16 val;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.yRGB[index] = val & 0xFF;
      }

      /*
       ** read in I
       */
      for (index = 0; index < 4; index++) {
        FxU16 val;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.iRGB[index][0] = val & 0x1FF;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.iRGB[index][1] = val & 0x1FF;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.iRGB[index][2] = val & 0x1FF;
      }

      /*
       ** read in Q
       */
      for (index = 0; index < 4; index++) {
        FxU16 val;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.qRGB[index][0] = val & 0x1FF;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.qRGB[index][1] = val & 0x1FF;
        if (!ReadDataShort(image_file, &val)) goto _loc1;
        info->table.nccTable.qRGB[index][2] = val & 0x1FF;
      }

      /*
       ** pack the table Y entries
       */
      for (index = 0; index < 4; index++) {
        FxU32 packedvalue;

        packedvalue  = ((FxU32) info->table.nccTable.yRGB[index*4+0]);
        packedvalue |= ((FxU32) info->table.nccTable.yRGB[index*4+1]) << 8;
        packedvalue |= ((FxU32) info->table.nccTable.yRGB[index*4+2]) << 16;
        packedvalue |= ((FxU32) info->table.nccTable.yRGB[index*4+3]) << 24;

        info->table.nccTable.packed_data[index] = packedvalue;
      }

      /*
       ** pack the table I entries
       */
      for (index = 0; index < 4; index++) {
        FxU32 packedvalue;

        packedvalue  = ((FxU32) info->table.nccTable.iRGB[index][0]) << 18;
        packedvalue |= ((FxU32) info->table.nccTable.iRGB[index][1]) << 9;
        packedvalue |= ((FxU32) info->table.nccTable.iRGB[index][2]) << 0;

        info->table.nccTable.packed_data[index+4] = packedvalue;
      }

      /*
       ** pack the table Q entries
       */
      for (index = 0; index < 4; index++) {
        FxU32 packedvalue;

        packedvalue  = ((FxU32) info->table.nccTable.qRGB[index][0]) << 18;
        packedvalue |= ((FxU32) info->table.nccTable.qRGB[index][1]) << 9;;
        packedvalue |= ((FxU32) info->table.nccTable.qRGB[index][2]) << 0;

        info->table.nccTable.packed_data[index+8] = packedvalue;
      }
    }

    if (!Read16Bit(info->data, image_file, 
              info->header.small_lod,
              info->header.large_lod,
              G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
         goto _loc1;
    break;

  case GR_TEXFMT_P_8:
    /*
     ** If necessary, read in the Palette
     */
    {
      FxU32 i;
      for(i = 0; i < 256; i++)
      {
        FxU32 val;
        if (!ReadDataLong(image_file, &val)) goto _loc1;
        info->table.palette.data[i] = val;
      }
    }

    if (!Read8Bit(info->data, image_file, 
             info->header.small_lod, 
             info->header.large_lod, 
             G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
        goto _loc1;
    break;

  case GR_TEXFMT_AP_88:
    /*
     ** If necessary, read in the Palette
     */
    {
      FxU32 i;
      for(i = 0; i < 256; i++)
      {
        FxU32 val;
        if (!ReadDataLong(image_file, &val)) goto _loc1;
        info->table.palette.data[i] = val;
      }
    }

    if (!Read16Bit(info->data, image_file, 
              info->header.small_lod,
              info->header.large_lod,
              G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
        goto _loc1;
    break;

  case GR_TEXFMT_ARGB_CMP_FXT1:
    if (!Read4Bit(info->data, image_file, 
             info->header.small_lod, 
             info->header.large_lod, 
             G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
        goto _loc1;
    break;
/* TODO: to support DXTn, we need to read .dds files
  case GR_TEXFMT_ARGB_CMP_DXT1:
    if (!ReadDXT4Bit(info->data, image_file, 
                info->header.small_lod, 
                info->header.large_lod, 
                G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
        goto _loc1;
    break;

  case GR_TEXFMT_ARGB_CMP_DXT2:
  case GR_TEXFMT_ARGB_CMP_DXT3:
  case GR_TEXFMT_ARGB_CMP_DXT4:
  case GR_TEXFMT_ARGB_CMP_DXT5:
    if (!ReadDXT8Bit(info->data, image_file, 
                info->header.small_lod, 
                info->header.large_lod, 
                G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
        goto _loc1;
    break;
*/
  case GR_TEXFMT_INTENSITY_8:
  case GR_TEXFMT_ALPHA_8:
  case GR_TEXFMT_ALPHA_INTENSITY_44:
  /*case GR_TEXFMT_YIQ_422:*/
  case GR_TEXFMT_RGB_332:
  /*case GR_TEXFMT_P_8:*/
    if (!Read8Bit(info->data, image_file, 
             info->header.small_lod, 
             info->header.large_lod, 
             G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
        goto _loc1;
    break;

  case GR_TEXFMT_RGB_565:
  case GR_TEXFMT_ARGB_8332:
  case GR_TEXFMT_ARGB_1555:
  /*case GR_TEXFMT_AYIQ_8422:*/
  case GR_TEXFMT_ARGB_4444:
  case GR_TEXFMT_ALPHA_INTENSITY_88:
  /*case GR_TEXFMT_AP_88:*/
  case GR_TEXFMT_YUYV_422:
  case GR_TEXFMT_UYVY_422:
    if (!Read16Bit(info->data, image_file, 
              info->header.small_lod,
              info->header.large_lod,
              G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
        goto _loc1;
    break;

  case GR_TEXFMT_ARGB_8888:
  case GR_TEXFMT_AYUV_444:
    if(!Read32Bit(info->data, image_file,
              info->header.small_lod,
              info->header.large_lod,
              G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
        goto _loc1;
    break;

  default:
 _loc1:
    fclose(image_file);
    return FXFALSE;
  }

  fclose(image_file);
  return FXTRUE;
}

/*
** Read4Bit (FXT1)
**
** Read in a 4-bit Compressed texture map.  Luckily the minimum mipmap
** size is 8x4 texels so we never have to worry about where the high
** or low nibble is.
** Take advantage of the fact that the minimum size is 16 bytes
** during the fread() call.
*/
static FxBool
Read4Bit(FxU8 *data, FILE *image_file, int small_lod, int large_lod,
         GrAspectRatio_t aspect_ratio)
{
  int lod;
  unsigned int width, height, thisMipMapByteCount;

  for (lod = small_lod; lod <= large_lod; lod++) {
    width  = _grMipMapHostWHCmp4Bit[aspect_ratio][lod][0];
    height = _grMipMapHostWHCmp4Bit[aspect_ratio][lod][1];

    /* Divide the WxH by 32 (the most we can safely do) so that we can
       read 16 bytes at a time. */
    thisMipMapByteCount = (width * height) >> 5;

    if (fread(data, 16, thisMipMapByteCount, image_file) != thisMipMapByteCount)
      return FXFALSE;
    data += (16 * thisMipMapByteCount);
  }
  return FXTRUE;
}

#if 0
/*
** ReadDXT4Bit (DXT1)
**
** Read in a 4-bit Compressed texture map.
** Take advantage of the fact that the minimum size is 8 bytes
** during the fread() call.
** Note: the smallest DXT1 mipmap has 2 side-by-side 4x4 microtiles
** but we only read one of them.
*/
static FxBool 
ReadDXT4Bit(FxU8 *data, FILE *image_file, int small_lod, int large_lod,
            GrAspectRatio_t aspect_ratio)
{
  int lod;
  FxU32 cnt;

  for (lod = small_lod; lod <= large_lod; lod++) {
    cnt = (FxU32)_grMipMapHostWHDXT[aspect_ratio][lod][0] *
          (FxU32)_grMipMapHostWHDXT[aspect_ratio][lod][1];

    /* Divide the WxH by 16 to read 8 bytes at a time. */
    cnt >>= 4;

    if (fread(data, 8, cnt, image_file) != cnt)
      return FXFALSE;
    data += (8 * cnt);
  }
  return FXTRUE;
}

/*
** ReadDXT8Bit (DXT2,3,4,5)
**
** Read in a 8-bit Compressed texture map. the minimum mipmap
** size is 4x4 texels
** Take advantage of the fact that the minimum size is 16 bytes
** during the fread() call.
*/
static FxBool 
ReadDXT8Bit(FxU8 *data, FILE *image_file, 
            int small_lod, int large_lod, 
            GrAspectRatio_t aspect_ratio)
{
  int lod;
  FxU32 cnt;

  for (lod = small_lod; lod <= large_lod; lod++) {
    cnt = (FxU32)_grMipMapHostWHDXT[aspect_ratio][lod][0] *
          (FxU32)_grMipMapHostWHDXT[aspect_ratio][lod][1];

    /* Divide the WxH by 16 (the most we can safely do) so that we can
       read 16 bytes at a time. */
    cnt >>= 4;

    if (fread(data, 16, cnt, image_file) != cnt)
      return FXFALSE;
    data += (16 * cnt);
  }
  return FXTRUE;
}
#endif

/*
** Read8Bit
**
** Read in an 8-bit texture map, unpacked.
*/
static FxBool 
Read8Bit(FxU8 *data, FILE *image_file, 
         int small_lod, int large_lod, 
         GrAspectRatio_t aspect_ratio)
{
  int lod;
  FxU32 cnt;

  for (lod = small_lod; lod <= large_lod; lod++) {
    cnt = (FxU32)_grMipMapHostWH[aspect_ratio][lod][0] *
          (FxU32)_grMipMapHostWH[aspect_ratio][lod][1];

    if (fread(data, 1, cnt, image_file) != cnt)
      return FXFALSE;
    data += cnt;
  }
  return FXTRUE;
}

/*
** Read16Bit
**
** Read in a 16-bit texture map, unpacked.
*/
static FxBool Read16Bit(FxU16 *data, FILE *image_file, 
                      int small_lod, int large_lod, 
                      GrAspectRatio_t aspect_ratio)
{
  FxU32 idx, cnt;
  int lod;

  for (lod = small_lod; lod <= large_lod; lod++) {
    cnt = (FxU32)_grMipMapHostWH[aspect_ratio][lod][0] *
          (FxU32)_grMipMapHostWH[aspect_ratio][lod][1];

    for (idx = 0; idx < cnt; idx++) {
      if (!ReadDataShort(image_file,data))
        return FXFALSE;
      data++;
    }
  }
  return FXTRUE;
}

/*
** Read32Bit
**
** Read in a 32-bit texture map, unpacked.
*/
static FxBool Read32Bit(FxU32 *data, FILE *image_file, 
                      int small_lod, int large_lod, 
                      GrAspectRatio_t aspect_ratio)
{
  FxU32 idx, cnt;
  int lod;

  for (lod = small_lod; lod <= large_lod; lod++) {
    cnt = (FxU32)_grMipMapHostWH[aspect_ratio][lod][0] *
          (FxU32)_grMipMapHostWH[aspect_ratio][lod][1];

    for (idx = 0; idx < cnt; idx++) {
      if (!ReadDataLong(image_file, data))
        return FXFALSE;
      data++;
    }
  }
  return FXTRUE;
}

/*
** FxU16 ReadDataShort
*/
static FxBool ReadDataShort(FILE *fp, FxU16 *data)
{
  FxU16 value;
  int b;

  /*
  ** read in the MSB
  */
  b = getc (fp);
  if (b == EOF) return FXFALSE;
  value = (FxU16) ((b&0xFF)<<8);

  /*
  ** read in the LSB
  */
  b = getc (fp);
  if (b == EOF) return FXFALSE;
  value |= (FxU16) (b & 0x00FF);

  *data = value;
  return FXTRUE;
}

/*
** ReadDataLong
*/
static FxBool ReadDataLong(FILE *fp, FxU32 *data)
{
  FxU8 byte[4];

  if (fread(byte, 1, 4, fp) != 4)
    return FXFALSE;

  *data = (((FxU32) byte[0]) << 24) |
    (((FxU32) byte[1]) << 16) |
    (((FxU32) byte[2]) <<  8) |
    ((FxU32) byte[3]);

  return FXTRUE;
}
