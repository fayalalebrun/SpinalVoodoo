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
** Revision 1.1.1.1.6.5  2005/08/13 21:07:02  jwrdegoede
** Last needed 64 bit fixes for h5/h3, complete 64 bit support for cvg
**
** Revision 1.1.1.1.6.4  2005/06/09 18:32:28  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.1.1.6.3  2004/01/20 14:04:10  dborca
** compilation and some other minor fixes to aid in debugging
**
** Revision 1.1.1.1.6.2  2003/06/29 18:43:27  guillemj
** Fix compilation warnings.
**
** Revision 1.1.1.1.6.1  2003/05/05 07:12:46  dborca
** no message
**
** Revision 1.1.1.1  1999/11/24 21:44:56  joseph
** Initial checkin for SourceForge
**
** 
** 4     4/06/99 3:54p Dow
** Alt tab again.
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

static FxBool ReadDataShort(FILE *, FxU16 *data);
static FxBool ReadDataLong(FILE *, FxU32 *data);
static FxBool Read8Bit(FxU8 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect);
static FxBool Read16Bit(FxU16 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect);

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
    FxI32 lod;
    Info->mem_required = 0;
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
#ifdef GLIDE3_DEBUG
    for(lod = Info->header.large_lod; lod <= Info->header.small_lod; lod++) {
#else
    for(lod = Info->header.large_lod; lod >= Info->header.small_lod; lod--) {
#endif
#else
    for(lod = Info->header.large_lod; lod <= Info->header.small_lod; lod++) {
#endif
      Info->mem_required += (_grMipMapHostSize[_gr_aspect_index_table[G3_ASPECT_TRANSLATE(Info->header.aspect_ratio)]]
                             [G3_LOD_TRANSLATE(lod)] << (Info->header.format>=GR_TEXFMT_16BIT));
    }
  }

  return format_found;
}

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

  /*
  ** Read in the image
  */
  switch (info->header.format)
  {
  case GR_TEXFMT_INTENSITY_8:
  case GR_TEXFMT_ALPHA_8:
  case GR_TEXFMT_ALPHA_INTENSITY_44:
  case GR_TEXFMT_YIQ_422:
  case GR_TEXFMT_RGB_332:
  case GR_TEXFMT_P_8:
    if(!Read8Bit(info->data, image_file, 
             G3_LOD_TRANSLATE(info->header.small_lod), 
             G3_LOD_TRANSLATE(info->header.large_lod), 
             G3_ASPECT_TRANSLATE(info->header.aspect_ratio)))
       goto _loc1;
    break;

  case GR_TEXFMT_RGB_565:
  case GR_TEXFMT_ARGB_8332:
  case GR_TEXFMT_ARGB_1555:
  case GR_TEXFMT_AYIQ_8422:
  case GR_TEXFMT_ARGB_4444:
  case GR_TEXFMT_ALPHA_INTENSITY_88:
  case GR_TEXFMT_AP_88:
    if (!Read16Bit(info->data, image_file, 
              G3_LOD_TRANSLATE(info->header.small_lod),
              G3_LOD_TRANSLATE(info->header.large_lod),
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

  for (lod = large_lod; lod <= small_lod; lod++) {
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

  for (lod = large_lod; lod <= small_lod; lod++) {
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
