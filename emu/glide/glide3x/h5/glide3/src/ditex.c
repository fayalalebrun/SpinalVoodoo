/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONL
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGH
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DF
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com).
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR 
** FULL TEXT OF THE NON-WARRANTY PROVISIONS. 
**
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT T
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS I
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FA
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS O
** THE UNITED STATES. 
**
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVE
**
** $Log
**  10   3dfx      1.4.1.3.1.0 10/11/00 Brent           Forced check in to enforce
**       branching.
**  9    3dfx      1.4.1.3     06/20/00 Joseph Kain     Fixed errors introduced by
**       my previous merge.
**  8    3dfx      1.4.1.2     06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  7    3dfx      1.4.1.1     06/15/00 Bill White      Merged changes to support
**       Linux.
**  6    3dfx      1.4.1.0     06/13/00 Adam Briggs     fixes single pass trilinear
**       for FXT1
**  5    3dfx      1.4         02/10/00 Jonny Cochrane  Fixes single pass trilinear
**       filtering, PRS 12575 and 12566
**  4    3dfx      1.3         01/28/00 Kenneth Dyke    Totoally revamped TMU
**       register update mechanism to make 2PPC modes work right regardless of the
**       order of Glide calls.   Also fixed a few register config bugs found when
**       switching between new and old style combine modes.
**  3    3dfx      1.2         10/15/99 Anthony tai     move 2ppc setting to state
**       validation
**  2    3dfx      1.1         10/06/99 Anthony tai     disable 2ppc if tmu1 is
**       specified
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 38    9/03/99 17:41 Dwm
** Translate aspect ratios only once.  (FXT1 format only).
** 
** 37    8/25/99 8:44p Larryw
** Expand mipmap size and offset tables to accommodate FXT1.
** 
** 36    8/19/99 7:55p Larryw
** FXT1  Tsplit changes.
** 
** 35    8/18/99 3:20p Larryw
** FXT1 refinements.
** 
** 34    8/05/99 5:03p Larryw
** FXT1 format works now.
** 
** 33    7/29/99 7:07p Larryw
** Pave the way for FXT1 (but not quite there yet).
** 
** 31    7/26/99 4:56p Larryw
** Change GR_TEXFMT_ARGB_8888 from 0xf to 0x12.
** Adjust bytesPerTexel[] table accordingly.
** 
** 30    7/22/99 8:14p Larryw
** Texture format byte-depth improvements
** 
** 29    7/14/99 6:23p Larryw
** Remove obsolete G3_LOD_TRANSLATE() macro
** Define _grMipMapOffset[][] at compile time
** Fix 2k texture address-finding
** 
** 28    7/12/99 6:27p Larryw
** Cleaner handling of tBig variable
** Filled in _grMipMapOffset[][] for 512+ sizes
** Fixed problems with grTexMipMapMode()
** 
** 27    7/07/99 6:52p Larryw
** * 2k texture support
** * Reversed order of LOD tables
** * Added 512,1024, and 2048-sized entries in tables
** * Nullified G3_LOD_TRANSLATE()
** * Created _g3LodXlat() for where tLOD register is read/written
** * Misc cosmetic changes.
** 
** 26    6/14/99 5:16p Larryw
** Added 32-bit texture format support.
** 
** 25    6/01/99 12:36p Atai
** checks if we are in CSIM
** 
** 24    5/28/99 3:46p Atai
** use rawLfb instead of tex_ptr for grTexMaxAddress
** 
** 23    5/10/99 1:27p Stb_mmcclure
** Modified comment brackets in grTexMaxAddress
** 
** 22    5/07/99 12:53p Dow
** My mods to Matts TexAddress fixes
** 
** 21    4/04/99 8:51p Atai
** Partial check-in for alt-tab issue. set FX_GLIDE_ALT_TAB=1 to build
** glide3x with hwcQueryContext built into GR_BEGIN_NOFIFOCHECK. It works
** with DEBUG glide only. In the non-debug glide, we can still see the
** desktop corruption.
** 
** 20    3/24/99 6:17p Peter
** reduce nop flush for chain downloads
** 
** 19    2/18/99 6:00p Peter
** sub alignment linear textures
** 
** 18    2/10/99 2:34p Peter
** corrected alignment textures within an alignment unit
** 
** 17    2/03/99 6:48p Atai
** fixed _grTexTextureMemRequired
** 
** 16    2/02/99 4:38p Peter
** 16 byte texture alignmnet boundary, fxed assertion for 6666 palette
** downloads
** 
** 15    1/25/99 6:32p Peter
** cleaned up some translation macros and tables
** 
** 14    12/03/98 11:26p Dow
** 
** 13    10/13/98 5:27p Peter
** 6666 format hack
** 
** 12    10/09/98 2:44p Atai
** fixed 6666 palette
** 
** 11    9/09/98 12:56p Atai
** relaxed 2mb texture address constraint for uma devices
** 
** 10    7/16/98 8:15a Jdt
** fxcmd.h
** 
** 9     6/22/98 3:35p Peter
** concatenation this way is evil
** 
** 8     5/29/98 11:45a Atai
** 1.added _EXT for extension #defines. 
** 2. change GR_TEXBASE_* values
** 3. Remove GR_TEXCHROMA_ENABLE_SUBSTITUTE_RGB
** 
** 7     5/18/98 2:48p Atai
** Changed to do 16-byte increments instead of 8-byte
** 
** 6     5/08/98 1:40p Peter
** merged Anthony's palette of Taco changes
** 
** 4     4/22/98 4:57p Peter
** glide2x merge
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
 * 16    1/13/98 12:42p Atai
 * fixed grtexinfo, grVertexLayout, and draw triangle
 * 
 * 15    1/09/98 6:48p Atai
 * grTexInfo, GR_LOD_* and GR_ASPECT_*
 * 
 * 13    12/09/97 12:20p Peter
 * mac glide port
 * 
 * 12    12/02/97 9:49a Dow
 * Got rid of Texelfx rev 0 warning
 * 
 * 11    11/20/97 6:58p Dow
 * Marked _grTexTextureMemRequired for movement
 * 
 * 10    8/18/97 3:52p Peter
 * pre-hw arrival fixes/cleanup
 * 
 * 9     6/02/97 4:09p Peter
 * Compile w/ gcc for Dural
 * 
 * 8     5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 7     5/21/97 6:04a Peter
 * 
 * 6     3/15/97 8:09p Jdt
 * Remove grTexDownloadTable from this file because I added SST-1 only
 * code to it
 * 
 * 5     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 4     2/12/97 2:09p Hanson
 * Hopefully removed the rest of my effage. 
 * 
 * 3     2/12/97 12:34p Dow
 * Fixed Hanson effage
 * 
 * 2     1/18/97 11:41p Dow
 * Fixed Gary's "Last C Bug" analog
 * Fixed usage of _grMipMapOffset_Tsplit
 * 
 * 1     12/23/96 1:39p Dow
 * Changes for multiplatform
**
*/

#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

/*
 * Number of Bits Per Texel based on the format.  This needs to be
 * kept up to date with any changes to the GrTextureFormat_t type
 * (q.v.).  NOTE that reserved types are undefined, ergo have depth 0.
 */
const FxU32 _grBitsPerTexel[] =
{
  0x08,  /* GR_TEXFMT_8BIT == GR_TEXFMT_RGB_332    */
  0x08,  /* GR_TEXFMT_YIQ_422                      */
  0x08,  /* GR_TEXFMT_ALPHA_8                      */
  0x08,  /* GR_TEXFMT_INTENSITY_8                  */
  0x08,  /* GR_TEXFMT_ALPHA_INTENSITY_44           */
  0x08,  /* GR_TEXFMT_P_8                          */
  0x08,  /* GR_TEXFMT_RSVD0 == GR_TEXFMT_P_8_6666  */
  0x10,  /* GR_TEXFMT_RSVD1                        */
  0x10,  /* GR_TEXFMT_16BIT == GR_TEXFMT_ARGB_8332 */
  0x10,  /* GR_TEXFMT_AYIQ_8422                    */
  0x10,  /* GR_TEXFMT_RGB_565                      */
  0x10,  /* GR_TEXFMT_ARGB_1555                    */
  0x10,  /* GR_TEXFMT_ARGB_4444                    */
  0x10,  /* GR_TEXFMT_ALPHA_INTENSITY_88           */
  0x10,  /* GR_TEXFMT_AP_88                        */
  0x00,  /* GR_TEXFMT_RSVD2                        */
  0x00,  /*                                        */
  0x04,  /* GR_TEXFMT_ARGB_CMP_FXT1                */
  0x20,  /* GR_TEXFMT_ARGB_8888                    */
  0x10,  /* GR_TEXFMT_YUYV_422                     */
  0x10,  /* GR_TEXFMT_UYVY_422                     */
  0x20,  /* GR_TEXFMT_AYUV_444                     */
  0x04,  /* GR_TEXFMT_ARGB_CMP_DXT1                */
  0x08,  /* GR_TEXFMT_ARGB_CMP_DXT2                */
  0x08,  /* GR_TEXFMT_ARGB_CMP_DXT3                */
  0x08,  /* GR_TEXFMT_ARGB_CMP_DXT4                */
  0x08   /* GR_TEXFMT_ARGB_CMP_DXT5                */
};


/* size in texels  */
/*
 * NB - because we want Napalm's glide3 to be the same binary as
 * for Banshee and Voodoo3, and we need to add textures up to 2k,
 * it has become necessary to reverse all these tables w.r.t. LOD
 * and also stop using G3_LOD_TRANSLATE. - LEW 6/28/99
 */
const FxU32 _grMipMapHostSize[4][GR_LOD_LOG2_2048+1] =
{
  {                             /* 1:1 aspect ratio */
    1,                          /*  0 :    1x1    */
    4,                          /*  1 :    2x2    */
    16,                         /*  2 :    4x4    */
    64,                         /*  3 :    8x8    */
    256,                        /*  4 :   16x16   */
    1024,                       /*  5 :   32x32   */
    4096,                       /*  6 :   64x64   */
    16384,                      /*  7 :  128x128  */
    65536,                      /*  8 :  256x256  */
    262144,                     /*  9 :  512x512  */
    1048576,                    /* 10 : 1024x1024 */
    4194304,                    /* 11 : 2048x2048 */
  },
  {                             /* 2:1 aspect ratio */
    1,                          /*  0 :    1x1    */
    2,                          /*  1 :    2x1    */
    8,                          /*  2 :    4x2    */
    32,                         /*  3 :    8x4    */
    128,                        /*  4 :   16x8    */
    512,                        /*  5 :   32x16   */
    2048,                       /*  6 :   64x32   */
    8192,                       /*  7 :  128x64   */
    32768,                      /*  8 :  256x128  */
    131072,                     /*  9 :  512x256  */
    524288,                     /* 10 : 1024x512  */
    2097152,                    /* 11 : 2048x1024 */
  },
  {                             /* 4:1 aspect ratio */
    1,                          /*  0 :    1x1   */
    2,                          /*  1 :    2x1   */
    4,                          /*  2 :    4x1   */
    16,                         /*  3 :    8x2   */
    64,                         /*  4 :   16x4   */
    256,                        /*  5 :   32x8   */
    1024,                       /*  6 :   64x16  */
    4096,                       /*  7 :  128x32  */
    16384,                      /*  8 :  256x64  */
    65536,                      /*  9 :  512x128 */
    262144,                     /* 10 : 1024x256 */
    1048576,                    /* 11 : 2048x512 */
  },
  {                             /* 8:1 aspect ratio */
    1,                          /*  0 :    1x1   */
    2,                          /*  1 :    2x1   */
    4,                          /*  2 :    4x1   */
    8,                          /*  3 :    8x1   */
    32,                         /*  4 :   16x2   */
    128,                        /*  5 :   32x4   */
    512,                        /*  6 :   64x8   */
    2048,                       /*  7 :  128x16  */
    8192,                       /*  8 :  256x32  */
    32768,                      /*  9 :  512x64  */
    131072,                     /* 10 : 1024x128 */
    524288,                     /* 11 : 2048x256 */
  }
};

/*
 * Similar to _grMipMapHostSize[][] but for 4-bit fxt1 compressed
 * textures, where 8x4 is the minimum size.
 */
const FxU32 _grMipMapHostSizeCmp4Bit[7][GR_LOD_LOG2_2048+1] =
{
  {                  /* 8:1 aspect ratio */
         32,         /*  0 :    8x4    */
         32,         /*  1 :    8x4    */
         32,         /*  2 :    8x4    */
         32,         /*  3 :    8x4    */
         64,         /*  4 :   16x4    */
        128,         /*  5 :   32x4    */
        512,         /*  6 :   64x8    */
       2048,         /*  7 :  128x16   */
       8192,         /*  8 :  256x32   */
      32768,         /*  9 :  512x64   */
     131072,         /* 10 : 1024x128  */
     524288,         /* 11 : 2048x256  */
  },
  {                  /* 4:1 aspect ratio */
         32,         /*  0 :    8x4    */
         32,         /*  1 :    8x4    */
         32,         /*  2 :    8x4    */
         32,         /*  3 :    8x4    */
         64,         /*  4 :   16x4    */
        256,         /*  5 :   32x8    */
       1024,         /*  6 :   64x16   */
       4096,         /*  7 :  128x32   */
      16384,         /*  8 :  256x64   */
      65536,         /*  9 :  512x128  */
     262144,         /* 10 : 1024x256  */
    1048576,         /* 11 : 2048x512  */
  },
  {                  /* 2:1 aspect ratio */
         32,         /*  0 :    8x4    */
         32,         /*  1 :    8x4    */
         32,         /*  2 :    8x4    */
         32,         /*  3 :    8x4    */
        128,         /*  4 :   16x8    */
        512,         /*  5 :   32x16   */
       2048,         /*  6 :   64x32   */
       8192,         /*  7 :  128x64   */
      32768,         /*  8 :  256x128  */
     131072,         /*  9 :  512x256  */
     524288,         /* 10 : 1024x512  */
    2097152,         /* 11 : 2048x1024 */
  },
  {                  /* 1:1 aspect ratio */
         32,         /*  0 :    8x4    */
         32,         /*  1 :    8x4    */
         32,         /*  2 :    8x4    */
         64,         /*  3 :    8x8    */
        256,         /*  4 :   16x16   */
       1024,         /*  5 :   32x32   */
       4096,         /*  6 :   64x64   */
      16384,         /*  7 :  128x128  */
      65536,         /*  8 :  256x256  */
     262144,         /*  9 :  512x512  */
    1048576,         /* 10 : 1024x1024 */
    4194304,         /* 11 : 2048x2048 */
  },
  {                  /* 1:2 aspect ratio */
         32,         /*  0 :    8x4    */
         32,         /*  1 :    8x4    */
         32,         /*  2 :    8x4    */
         64,         /*  3 :    8x8    */
        128,         /*  4 :    8x16   */
        512,         /*  5 :   16x32   */
       2048,         /*  6 :   32x64   */
       8192,         /*  7 :   64x128  */
      32768,         /*  8 :  128x256  */
     131072,         /*  9 :  256x512  */
     524288,         /* 10 :  512x1024 */
    2097152,         /* 11 : 1024x2048 */
  },
  {                  /* 1:4 aspect ratio */
         32,         /*  0 :    8x4    */
         32,         /*  1 :    8x4    */
         32,         /*  2 :    8x4    */
         64,         /*  3 :    8x8    */
        128,         /*  4 :    8x16   */
        256,         /*  5 :    8x32   */
       1024,         /*  6 :   16x64   */
       4096,         /*  7 :   32x128  */
      16384,         /*  8 :   64x256  */
      65536,         /*  9 :  128x512  */
     262144,         /* 10 :  256x1024 */
    1048576,         /* 11 :  512x2048 */
  },
  {                  /* 1:8 aspect ratio */
         32,         /*  0 :    8x4    */
         32,         /*  1 :    8x4    */
         32,         /*  2 :    8x4    */
         64,         /*  3 :    8x8    */
        128,         /*  4 :    8x16   */
        256,         /*  5 :    8x32   */
        512,         /*  6 :    8x64   */
       2048,         /*  7 :   16x128  */
       8192,         /*  8 :   32x256  */
      32768,         /*  9 :   64x512  */
     131072,         /* 10 :  128x1024 */
     524288,         /* 11 :  256x2048 */
  },
};

/*
 * Similar to _grMipMapHostSize[][] but for DXTn compressed
 * textures, where 4x4 is the minimum size.
 */
const FxU32 _grMipMapHostSizeDXT[4][GR_LOD_LOG2_2048+1] =
{
  {                  /* 8:1 aspect ratio */
         16,         /*  0 :    4x4    */
         16,         /*  1 :    4x4    */
         16,         /*  2 :    4x4    */
         32,         /*  3 :    8x4    */
         64,         /*  4 :   16x4    */
        128,         /*  5 :   32x4    */
        512,         /*  6 :   64x8    */
       2048,         /*  7 :  128x16   */
       8192,         /*  8 :  256x32   */
      32768,         /*  9 :  512x64   */
     131072,         /* 10 : 1024x128  */
     524288,         /* 11 : 2048x256  */
  },
  {                  /* 4:1 aspect ratio */
         16,         /*  0 :    4x4    */
         16,         /*  1 :    4x4    */
         16,         /*  2 :    4x4    */
         32,         /*  3 :    8x4    */
         64,         /*  4 :   16x4    */
        256,         /*  5 :   32x8    */
       1024,         /*  6 :   64x16   */
       4096,         /*  7 :  128x32   */
      16384,         /*  8 :  256x64   */
      65536,         /*  9 :  512x128  */
     262144,         /* 10 : 1024x256  */
    1048576,         /* 11 : 2048x512  */
  },
  {                  /* 2:1 aspect ratio */
         16,         /*  0 :    4x4    */
         16,         /*  1 :    4x4    */
         16,         /*  2 :    4x4    */
         32,         /*  3 :    8x4    */
        128,         /*  4 :   16x8    */
        512,         /*  5 :   32x16   */
       2048,         /*  6 :   64x32   */
       8192,         /*  7 :  128x64   */
      32768,         /*  8 :  256x128  */
     131072,         /*  9 :  512x256  */
     524288,         /* 10 : 1024x512  */
    2097152,         /* 11 : 2048x1024 */
  },
  {                  /* 1:1 aspect ratio */
         16,         /*  0 :    4x4    */
         16,         /*  1 :    4x4    */
         16,         /*  2 :    4x4    */
         64,         /*  3 :    8x8    */
        256,         /*  4 :   16x16   */
       1024,         /*  5 :   32x32   */
       4096,         /*  6 :   64x64   */
      16384,         /*  7 :  128x128  */
      65536,         /*  8 :  256x256  */
     262144,         /*  9 :  512x512  */
    1048576,         /* 10 : 1024x1024 */
    4194304,         /* 11 : 2048x2048 */
  },
};

/*
 * Similar to _grMipMapHostWH[][], but for 4-bit compressed
 * textures, where 8x4 is the minimum size. FXT1
 */
const int _grMipMapHostWHCmp4Bit[G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x8) + 1]
[GR_LOD_LOG2_2048 + 1][2] =
{
  { /* GR_ASPECT_LOG2_8x1 */
    { 8   , 4 },
    { 8   , 4 }, 
    { 8   , 4 }, 
    { 8   , 4 }, 
    { 16  , 4 }, 
    { 32  , 4 }, 
    { 64  , 8 }, 
    { 128 , 16 }, 
    { 256 , 32 }, 
    { 512 , 64 },
    { 1024, 128 },
    { 2048, 256 },
  },
  { /* GR_ASPECT_LOG2_4x1 */
    { 8   , 4 },
    { 8   , 4 }, 
    { 8   , 4 }, 
    { 8   , 4 },
    { 16  , 4 }, 
    { 32  , 8 }, 
    { 64  , 16 }, 
    { 128 , 32 }, 
    { 256 , 64 }, 
    { 512 , 128 },
    { 1024, 256 },
    { 2048, 512 },
  } ,
  { /* GR_ASPECT_LOG2_2x1 */
    { 8   , 4 },
    { 8   , 4 }, 
    { 8   , 4 }, 
    { 8   , 4 }, 
    { 16  , 8 },
    { 32  , 16 },
    { 64  , 32 }, 
    { 128 , 64 }, 
    { 256 , 128 }, 
    { 512 , 256 },
    { 1024, 512 },
    { 2048, 1024 },
  },
  { /* GR_ASPECT_LOG2_1x1 */
    { 8   , 4 },
    { 8   , 4 }, 
    { 8   , 4 }, 
    { 8   , 8 }, 
    { 16  , 16 },
    { 32  , 32 },
    { 64  , 64 },
    { 128 , 128 }, 
    { 256 , 256 }, 
    { 512 , 512 },
    { 1024, 1024 },
    { 2048, 2048 },
  },
  { /* GR_ASPECT_LOG2_1x2 */
    { 8,   4 },
    { 8,   4 }, 
    { 8,   4 }, 
    { 8,   8 }, 
    { 8,   16 }, 
    { 16,  32 },
    { 32,  64 },
    { 64,  128 },
    { 128, 256 },
    { 256, 512 },
    { 512, 1024 },
    { 1024, 2048 },
  },
  { /* GR_ASPECT_LOG2_1x4 */
    { 8,   4 },
    { 8,   4 }, 
    { 8,   4 }, 
    { 8,   8 }, 
    { 8,   16 },
    { 8,   32 },
    { 16,  64 },
    { 32,  128 },
    { 64,  256 },
    { 128, 512 },
    { 256, 1024 },
    { 512, 2048 },
  },
  { /* GR_ASPECT_LOG2_1x8 */
    { 8,   4 },
    { 8,   4 },
    { 8,   4 }, 
    { 8,   8 }, 
    { 8,   16 },
    { 8,   32 },
    { 8,   64 },
    { 16,  128 },
    { 32,  256 },
    { 64,  512 },
    { 128, 1024 },
    { 256, 2048 },
  }
};

/*
 * Similar to _grMipMapHostWH[][], but for DXTn compressed
 * textures, where 4x4 is the minimum size.
 */
const int _grMipMapHostWHDXT[G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x8) + 1]
[GR_LOD_LOG2_2048 + 1][2] =
{
  { /* GR_ASPECT_LOG2_8x1 */
    { 4   , 4 },
    { 4   , 4 }, 
    { 4   , 4 }, 
    { 8   , 4 }, 
    { 16  , 4 }, 
    { 32  , 4 }, 
    { 64  , 8 }, 
    { 128 , 16 }, 
    { 256 , 32 }, 
    { 512 , 64 },
    { 1024, 128 },
    { 2048, 256 },
  },
  { /* GR_ASPECT_LOG2_4x1 */
    { 4   , 4 },
    { 4   , 4 }, 
    { 4   , 4 }, 
    { 8   , 4 },
    { 16  , 4 }, 
    { 32  , 8 }, 
    { 64  , 16 }, 
    { 128 , 32 }, 
    { 256 , 64 }, 
    { 512 , 128 },
    { 1024, 256 },
    { 2048, 512 },
  } ,
  { /* GR_ASPECT_LOG2_2x1 */
    { 4   , 4 },
    { 4   , 4 }, 
    { 4   , 4 }, 
    { 8   , 4 }, 
    { 16  , 8 },
    { 32  , 16 },
    { 64  , 32 }, 
    { 128 , 64 }, 
    { 256 , 128 }, 
    { 512 , 256 },
    { 1024, 512 },
    { 2048, 1024 },
  },
  { /* GR_ASPECT_LOG2_1x1 */
    { 4   , 4 },
    { 4   , 4 }, 
    { 4   , 4 }, 
    { 8   , 8 }, 
    { 16  , 16 },
    { 32  , 32 },
    { 64  , 64 },
    { 128 , 128 }, 
    { 256 , 256 }, 
    { 512 , 512 },
    { 1024, 1024 },
    { 2048, 2048 },
  },
  { /* GR_ASPECT_LOG2_1x2 */
    { 4,   4 },
    { 4,   4 }, 
    { 4,   4 }, 
    { 4,   8 }, 
    { 8,   16 }, 
    { 16,  32 },
    { 32,  64 },
    { 64,  128 },
    { 128, 256 },
    { 256, 512 },
    { 512, 1024 },
    { 1024, 2048 },
  },
  { /* GR_ASPECT_LOG2_1x4 */
    { 4,   4 },
    { 4,   4 }, 
    { 4,   4 }, 
    { 4,   8 }, 
    { 4,   16 },
    { 8,   32 },
    { 16,  64 },
    { 32,  128 },
    { 64,  256 },
    { 128, 512 },
    { 256, 1024 },
    { 512, 2048 },
  },
  { /* GR_ASPECT_LOG2_1x8 */
    { 4,   4 },
    { 4,   4 },
    { 4,   4 }, 
    { 4,   8 }, 
    { 4,   16 },
    { 4,   32 },
    { 8,   64 },
    { 16,  128 },
    { 32,  256 },
    { 64,  512 },
    { 128, 1024 },
    { 256, 2048 },
  }
};

const int _grMipMapHostWH[G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x8) + 1]
[GR_LOD_LOG2_2048 + 1][2] =
{
  { /* GR_ASPECT_LOG2_8x1 */
    { 1   , 1 },
    { 2   , 1 }, 
    { 4   , 1 }, 
    { 8   , 1 }, 
    { 16  , 2 }, 
    { 32  , 4 }, 
    { 64  , 8 }, 
    { 128 , 16 }, 
    { 256 , 32 }, 
    { 512 , 64 },
    { 1024, 128 },
    { 2048, 256 },
  },
  { /* GR_ASPECT_LOG2_4x1 */
    { 1   , 1 },
    { 2   , 1 }, 
    { 4   , 1 }, 
    { 8   , 2 },
    { 16  , 4 }, 
    { 32  , 8 }, 
    { 64  , 16 }, 
    { 128 , 32 }, 
    { 256 , 64 }, 
    { 512 , 128 },
    { 1024, 256 },
    { 2048, 512 },
  } ,
  { /* GR_ASPECT_LOG2_2x1 */
    { 1   , 1 },
    { 2   , 1 }, 
    { 4   , 2 }, 
    { 8   , 4 }, 
    { 16  , 8 },
    { 32  , 16 },
    { 64  , 32 }, 
    { 128 , 64 }, 
    { 256 , 128 }, 
    { 512 , 256 },
    { 1024, 512 },
    { 2048, 1024 },
  },
  { /* GR_ASPECT_LOG2_1x1 */
    { 1   , 1 },
    { 2   , 2 }, 
    { 4   , 4 }, 
    { 8   , 8 }, 
    { 16  , 16 },
    { 32  , 32 },
    { 64  , 64 },
    { 128 , 128 }, 
    { 256 , 256 }, 
    { 512 , 512 },
    { 1024, 1024 },
    { 2048, 2048 },
  },
  { /* GR_ASPECT_LOG2_1x2 */
    { 1,   1 },
    { 1,   2 }, 
    { 2,   4 }, 
    { 4,   8 }, 
    { 8,   16 }, 
    { 16,  32 },
    { 32,  64 },
    { 64,  128 },
    { 128, 256 },
    { 256, 512 },
    { 512, 1024 },
    { 1024, 2048 },
  },
  { /* GR_ASPECT_LOG2_1x4 */
    { 1,   1 },
    { 1,   2 }, 
    { 1,   4 }, 
    { 2,   8 }, 
    { 4,   16 },
    { 8,   32 },
    { 16,  64 },
    { 32,  128 },
    { 64,  256 },
    { 128, 512 },
    { 256, 1024 },
    { 512, 2048 },
  },
  { /* GR_ASPECT_LOG2_1x8 */
    { 1,   1 },
    { 1,   2 },
    { 1,   4 }, 
    { 1,   8 }, 
    { 2,   16 },
    { 4,   32 },
    { 8,   64 },
    { 16,  128 },
    { 32,  256 },
    { 64,  512 },
    { 128, 1024 },
    { 256, 2048 },
  }
};

/* translates GR_ASPECT_* to bits for the TLOD register */
const FxU32 _gr_aspect_xlate_table[] =
{
   (3 << SST_LOD_ASPECT_SHIFT) | SST_LOD_S_IS_WIDER,
   (2 << SST_LOD_ASPECT_SHIFT) | SST_LOD_S_IS_WIDER,
   (1 << SST_LOD_ASPECT_SHIFT) | SST_LOD_S_IS_WIDER,
    0 << SST_LOD_ASPECT_SHIFT,
    1 << SST_LOD_ASPECT_SHIFT,
    2 << SST_LOD_ASPECT_SHIFT,
    3 << SST_LOD_ASPECT_SHIFT
};

const FxU32 _gr_evenOdd_xlate_table[] = 
{
  0xFFFFFFFF,                      /* invalid */
  SST_LOD_TSPLIT,                  /* even */
  SST_LOD_TSPLIT | SST_LOD_ODD,    /* odd  */
  0,                               /* both */
};

/* the size of each mipmap level in texels, 4 is the minimum no matter what */
/* index is [aspect_ratio][lod]                                             */
static const FxU32 _grMipMapSize[4][16] = {
  {   /* 8:1 aspect ratio */
    0x000004,        /*  0 :    1x1   */
    0x000004,        /*  1 :    2x1   */
    0x000008,        /*  2 :    4x1   */
    0x000010,        /*  3 :    8x1   */
    0x000020,        /*  4 :   16x2   */
    0x000080,        /*  5 :   32x4   */
    0x000200,        /*  6 :   64x8   */
    0x000800,        /*  7 :  128x16  */
    0x002000,        /*  8 :  256x32  */
    0x008000,        /*  9 :  512x64  */
    0x020000,        /* 10 : 1024x128 */
    0x080000,        /* 11 : 2048x256 */
  },
  {   /* 4:1 aspect ratio */
    0x000004,        /*  0 :    1x1   */
    0x000004,        /*  1 :    2x1   */
    0x000008,        /*  2 :    4x1   */
    0x000010,        /*  3 :    8x2   */
    0x000040,        /*  4 :   16x4   */
    0x000100,        /*  5 :   32x8   */
    0x000400,        /*  6 :   64x16  */
    0x001000,        /*  7 :  128x32  */
    0x004000,        /*  8 :  256x64  */
    0x010000,        /*  9 :  512x128 */
    0x040000,        /* 10 : 1024x256 */
    0x100000,        /* 11 : 2048x512 */
  },
  {   /* 2:1 aspect ratio */
    0x000004,        /*  0 :    1x1    */
    0x000004,        /*  1 :    2x1    */
    0x000008,        /*  2 :    4x2    */
    0x000020,        /*  3 :    8x4    */
    0x000080,        /*  4 :   16x8    */
    0x000200,        /*  5 :   32x16   */
    0x000800,        /*  6 :   64x32   */
    0x002000,        /*  7 :  128x64   */
    0x008000,        /*  8 :  256x128  */
    0x020000,        /*  9 :  512x256  */
    0x080000,        /* 10 : 1024x512  */
    0x200000,        /* 11 : 2048x1024 */
  },
  {   /* 1:1 aspect ratio */
    0x000004,        /*  0 :    1x1    */
    0x000004,        /*  1 :    2x2    */
    0x000010,        /*  2 :    4x4    */
    0x000040,        /*  3 :    8x8    */
    0x000100,        /*  4 :   16x16   */
    0x000400,        /*  5 :   32x32   */
    0x001000,        /*  6 :   64x64   */
    0x004000,        /*  7 :  128x128  */
    0x010000,        /*  8 :  256x256  */
    0x040000,        /*  9 :  512x512  */
    0x100000,        /* 10 : 1024x1024 */
    0x400000,        /* 11 : 2048x2048 */
  },
};

/* Similar to _grMipMapSize[][] but for 4-bit compressed formats. FXT1 */
static const FxU32 _grMipMapSizeCmp4Bit[7][16] = {
  {                  /* 8:1 aspect ratio */
    0x000020,        /*   0 :    8x4    */
    0x000020,        /*   1 :    8x4    */
    0x000020,        /*   2 :    8x4    */
    0x000020,        /*   3 :    8x4    */
    0x000040,        /*   4 :   16x4    */
    0x000080,        /*   5 :   32x4    */
    0x000200,        /*   6 :   64x8    */
    0x000800,        /*   7 :  128x16   */
    0x002000,        /*   8 :  256x32   */
    0x008000,        /*   9 :  512x64   */
    0x020000,        /*  10 : 1024x128  */
    0x080000,        /*  11 : 2048x256  */
  },
  {                  /* 4:1 aspect ratio */
    0x000020,        /*   0 :    8x4    */
    0x000020,        /*   1 :    8x4    */
    0x000020,        /*   2 :    8x4    */
    0x000020,        /*   3 :    8x4    */
    0x000040,        /*   4 :   16x4    */
    0x000100,        /*   5 :   32x8    */
    0x000400,        /*   6 :   64x16   */
    0x001000,        /*   7 :  128x32   */
    0x004000,        /*   8 :  256x64   */
    0x010000,        /*   9 :  512x128  */
    0x040000,        /*  10 : 1024x256  */
    0x100000,        /*  11 : 2048x512  */
  },
  {                  /* 2:1 aspect ratio */
    0x000020,        /*   0 :    8x4    */
    0x000020,        /*   1 :    8x4    */
    0x000020,        /*   2 :    8x4    */
    0x000020,        /*   3 :    8x4    */
    0x000080,        /*   4 :   16x8    */
    0x000200,        /*   5 :   32x16   */
    0x000800,        /*   6 :   64x32   */
    0x002000,        /*   7 :  128x64   */
    0x008000,        /*   8 :  256x128  */
    0x020000,        /*   9 :  512x256  */
    0x080000,        /*  10 : 1024x512  */
    0x200000,        /*  11 : 2048x1024 */
  },
  {                  /* 1:1 aspect ratio */
    0x000020,        /*   0 :    8x4    */
    0x000020,        /*   1 :    8x4    */
    0x000020,        /*   2 :    8x4    */
    0x000040,        /*   3 :    8x8    */
    0x000100,        /*   4 :   16x16   */
    0x000400,        /*   5 :   32x32   */
    0x001000,        /*   6 :   64x64   */
    0x004000,        /*   7 :  128x128  */
    0x010000,        /*   8 :  256x256  */
    0x040000,        /*   9 :  512x512  */
    0x100000,        /*  10 : 1024x1024 */
    0x400000,        /*  11 : 2048x2048 */
  },
  {                  /* 1:2 aspect ratio */
    0x000020,        /*   0 :    8x4    */
    0x000020,        /*   1 :    8x4    */
    0x000020,        /*   2 :    8x4    */
    0x000040,        /*   3 :    8x8    */
    0x000080,        /*   4 :    8x16   */
    0x000200,        /*   5 :   16x32   */
    0x000800,        /*   6 :   32x64   */
    0x002000,        /*   7 :   64x128  */
    0x008000,        /*   8 :  128x256  */
    0x020000,        /*   9 :  256x512  */
    0x080000,        /*  10 :  512x1024 */
    0x200000,        /*  11 : 1024x2048 */
  },
  {                  /* 1:4 aspect ratio */
    0x000020,        /*   0 :    8x4    */
    0x000020,        /*   1 :    8x4    */
    0x000020,        /*   2 :    8x4    */
    0x000040,        /*   3 :    8x8    */
    0x000080,        /*   4 :    8x16   */
    0x000100,        /*   5 :    8x32   */
    0x000400,        /*   6 :   16x64   */
    0x001000,        /*   7 :   32x128  */
    0x004000,        /*   8 :   64x256  */
    0x010000,        /*   9 :  128x512  */
    0x040000,        /*  10 :  256x1024 */
    0x100000,        /*  11 :  512x2048 */
  },
  {                  /* 1:8 aspect ratio */
    0x000020,        /*   0 :    8x4    */
    0x000020,        /*   1 :    8x4    */
    0x000020,        /*   2 :    8x4    */
    0x000040,        /*   3 :    8x8    */
    0x000080,        /*   4 :    8x16   */
    0x000100,        /*   5 :    8x32   */
    0x000200,        /*   6 :    8x64   */
    0x000800,        /*   7 :   16x128  */
    0x002000,        /*   8 :   32x256  */
    0x008000,        /*   9 :   64x512  */
    0x020000,        /*  10 :  128x1024 */
    0x080000,        /*  11 :  256x2048 */
  },
};

/* Similar to _grMipMapSize[][] but for DXTn compressed formats. */
static const FxU32 _grMipMapSizeDXT[4][16] = {
  {                  /* 8:1 aspect ratio */
    0x000010,        /*   0 :    4x4    */
    0x000010,        /*   1 :    4x4    */
    0x000010,        /*   2 :    4x4    */
    0x000020,        /*   3 :    8x4    */
    0x000040,        /*   4 :   16x4    */
    0x000080,        /*   5 :   32x4    */
    0x000200,        /*   6 :   64x8    */
    0x000800,        /*   7 :  128x16   */
    0x002000,        /*   8 :  256x32   */
    0x008000,        /*   9 :  512x64   */
    0x020000,        /*  10 : 1024x128  */
    0x080000,        /*  11 : 2048x256  */
  },
  {                  /* 4:1 aspect ratio */
    0x000010,        /*   0 :    4x4    */
    0x000010,        /*   1 :    4x4    */
    0x000010,        /*   2 :    4x4    */
    0x000020,        /*   3 :    8x4    */
    0x000040,        /*   4 :   16x4    */
    0x000100,        /*   5 :   32x8    */
    0x000400,        /*   6 :   64x16   */
    0x001000,        /*   7 :  128x32   */
    0x004000,        /*   8 :  256x64   */
    0x010000,        /*   9 :  512x128  */
    0x040000,        /*  10 : 1024x256  */
    0x100000,        /*  11 : 2048x512  */
  },
  {                  /* 2:1 aspect ratio */
    0x000010,        /*   0 :    4x4    */
    0x000010,        /*   1 :    4x4    */
    0x000010,        /*   2 :    4x4    */
    0x000020,        /*   3 :    8x4    */
    0x000080,        /*   4 :   16x8    */
    0x000200,        /*   5 :   32x16   */
    0x000800,        /*   6 :   64x32   */
    0x002000,        /*   7 :  128x64   */
    0x008000,        /*   8 :  256x128  */
    0x020000,        /*   9 :  512x256  */
    0x080000,        /*  10 : 1024x512  */
    0x200000,        /*  11 : 2048x1024 */
  },
  {                  /* 1:1 aspect ratio */
    0x000010,        /*   0 :    4x4    */
    0x000010,        /*   1 :    4x4    */
    0x000010,        /*   2 :    4x4    */
    0x000040,        /*   3 :    8x8    */
    0x000100,        /*   4 :   16x16   */
    0x000400,        /*   5 :   32x32   */
    0x001000,        /*   6 :   64x64   */
    0x004000,        /*   7 :  128x128  */
    0x010000,        /*   8 :  256x256  */
    0x040000,        /*   9 :  512x512  */
    0x100000,        /*  10 : 1024x1024 */
    0x400000,        /*  11 : 2048x2048 */
  },
};

/* the offset from mipmap level 0 of each mipmap level in texels            */
/* index is [aspect_ratio][lod]                                             */
const FxI32 _grMipMapOffset[4][16] =
{
  /*
   * The idea here is to add up the sizes of the mipmap levels and
   * store them as offsets from the beginning of the level whose
   * larger dimension is 256.  To get the size of a particular mipmap
   * range, take the difference between two entries.  Otherwise, take
   * one entry and consider it an offset from the 256xN level.  To
   * accommodate big textures but not break old apps, use negative
   * offsets for the big texture entries. -LEW
   */
  {  /* 8:1 and 1:8 aspect ratios */
      10927, /* Sum(256x32, 128x16, 64x8, 32x4, 16x2, 8x1, 4x1, 2x1, 1x1)    */
      10926, /* Sum(256x32, 128x16, 64x8, 32x4, 16x2, 8x1, 4x1, 2x1)         */
      10924, /* Sum(256x32, 128x16, 64x8, 32x4, 16x2, 8x1, 4x1)              */
      10920, /* Sum(256x32, 128x16, 64x8, 32x4, 16x2, 8x1)                   */
      10912, /* Sum(256x32, 128x16, 64x8, 32x4, 16x2)                        */
      10880, /* Sum(256x32, 128x16, 64x8, 32x4)                              */
      10752, /* Sum(256x32, 128x16, 64x8)                                    */
      10240, /* Sum(256x32, 128x16)                                          */
       8192, /* Sum(256x32)                                                  */
          0, /* Base address (beginning of 256x32 level)                     */
     -32768, /* - Sum(512x64)                                                */
    -163840, /* - Sum(1024x128, 512x64)                                      */
    -688128, /* - Sum(2048x256, 1024x128, 512x64)                            */
  },
  {  /* 4:1 and 1:4 aspect ratios */
       21847, /* Sum(256x64, 128x32, 64x16, 32x8, 16x4, 8x2, 4x1, 2x1, 1x1)  */
       21846, /* Sum(256x64, 128x32, 64x16, 32x8, 16x4, 8x2, 4x1, 2x1)       */
       21844, /* Sum(256x64, 128x32, 64x16, 32x8, 16x4, 8x2, 4x1)            */
       21840, /* Sum(256x64, 128x32, 64x16, 32x8, 16x4, 8x2)                 */
       21824, /* Sum(256x64, 128x32, 64x16, 32x8, 16x4)                      */
       21760, /* Sum(256x64, 128x32, 64x16, 32x8)                            */
       21504, /* Sum(256x64, 128x32, 64x16)                                  */
       20480, /* Sum(256x64, 128x32)                                         */
       16384, /* Sum(256x64)                                                 */
           0, /* Base address (beginning of 256x64 level)                    */
      -65536, /* - Sum(512x128)                                              */
     -327680, /* - Sum(1024x256, 512x128)                                    */
    -1376256, /* - Sum(2048x512, 1024x256, 512x128)                          */
  },
  {  /* 2:1 and 1:2 aspect ratios */
       43691, /* Sum(256x128, 128x64, 64x32, 32x16, 16x8, 8x4, 4x2, 2x1, 1x1)*/
       43690, /* Sum(256x128, 128x64, 64x32, 32x16, 16x8, 8x4, 4x2, 2x1)     */
       43688, /* Sum(256x128, 128x64, 64x32, 32x16, 16x8, 8x4, 4x2)          */
       43680, /* Sum(256x128, 128x64, 64x32, 32x16, 16x8, 8x4)               */
       43648, /* Sum(256x128, 128x64, 64x32, 32x16, 16x8)                    */
       43520, /* Sum(256x128, 128x64, 64x32, 32x16)                          */
       43008, /* Sum(256x128, 128x64, 64x32)                                 */
       40960, /* Sum(256x128, 128x64)                                        */
       32768, /* Sum(256x128)                                                */
           0, /* Base address (beginning of 256x128 level)                   */
     -131072, /* - Sum(512x256)                                              */
     -655360, /* - Sum(1024x512, 512x256)                                    */
    -2752512, /* - Sum(2048x1024, 1024x512, 512x256)                         */
  },
  {  /* 1:1 aspect ratio */
       87381, /* Sum(256x256, 128x128, 64x64, 32x32, 16x16, 8x8, 4x4, 2x2, 1)*/
       87380, /* Sum(256x256, 128x128, 64x64, 32x32, 16x16, 8x8, 4x4, 2x2)   */
       87376, /* Sum(256x256, 128x128, 64x64, 32x32, 16x16, 8x8, 4x4)        */
       87360, /* Sum(256x256, 128x128, 64x64, 32x32, 16x16, 8x8)             */
       87296, /* Sum(256x256, 128x128, 64x64, 32x32, 16x16)                  */
       87040, /* Sum(256x256, 128x128, 64x64, 32x32)                         */
       86016, /* Sum(256x256, 128x128, 64x64)                                */
       81920, /* Sum(256x256, 128x128)                                       */
       65536, /* Sum(256x256)                                                */
           0, /* Base address (beginning of 256x256 level)                   */
     -262144, /* - Sum(512x512)                                              */
    -1310720, /* - Sum(1024x1024, 512x512)                                   */
    -5505024, /* - Sum(2048x2048, 1024x1024, 512x512)                        */
  },
};

/*
 * Similar to _grMipMapOffset[][], but for 4-bit compressed
 * textures, where 8x4 is the minimum size. FXT1
 */
const FxI32 _grMipMapOffsetCmp4Bit[7][16] = {
  {                  /* 8:1 aspect ratio */
     11072, /* Sum(256x32,128x16,64x8,32x4,16x4,8x4,8x4,8x4,8x4)      */
     11040, /* Sum(256x32,128x16,64x8,32x4,16x4,8x4,8x4,8x4)          */
     11008, /* Sum(256x32,128x16,64x8,32x4,16x4,8x4,8x4)              */
     10976, /* Sum(256x32,128x16,64x8,32x4,16x4,8x4)                  */
     10944, /* Sum(256x32,128x16,64x8,32x4,16x4)                      */
     10880, /* Sum(256x32,128x16,64x8,32x4)                           */
     10752, /* Sum(256x32,128x16,64x8)                                */
     10240, /* Sum(256x32,128x16)                                     */
      8192, /* Sum(256x32)                                            */
         0, /* Base address (beginning of 512x64 level)               */
    -32768, /* - Sum(512x64)                                          */
   -163840, /* - Sum(1024x128,512x64)                                 */
   -688128, /* - Sum(2048x256,1024x128,512x64)                        */
  },
  {                  /* 4:1 aspect ratio */
     21952, /* Sum(256x64,128x32,64x16,32x8,16x4,8x4,8x4,8x4,8x4)     */
     21920, /* Sum(256x64,128x32,64x16,32x8,16x4,8x4,8x4,8x4)         */
     21888, /* Sum(256x64,128x32,64x16,32x8,16x4,8x4,8x4)             */
     21856, /* Sum(256x64,128x32,64x16,32x8,16x4,8x4)                 */
     21824, /* Sum(256x64,128x32,64x16,32x8,16x4)                     */
     21760, /* Sum(256x64,128x32,64x16,32x8)                          */
     21504, /* Sum(256x64,128x32,64x16)                               */
     20480, /* Sum(256x64,128x32)                                     */
     16384, /* Sum(256x64)                                            */
         0, /* Base address (beginning of 512x128 level)              */
    -65536, /* - Sum(512x128)                                         */
   -327680, /* - Sum(1024x256,512x128)                                */
  -1376256, /* - Sum(2048x512,1024x256,512x128)                       */
  },
  {                  /* 2:1 aspect ratio */
     43776, /* Sum(256x128,128x64,64x32,32x16,16x8,8x4,8x4,8x4,8x4)   */
     43744, /* Sum(256x128,128x64,64x32,32x16,16x8,8x4,8x4,8x4)       */
     43712, /* Sum(256x128,128x64,64x32,32x16,16x8,8x4,8x4)           */
     43680, /* Sum(256x128,128x64,64x32,32x16,16x8,8x4)               */
     43648, /* Sum(256x128,128x64,64x32,32x16,16x8)                   */
     43520, /* Sum(256x128,128x64,64x32,32x16)                        */
     43008, /* Sum(256x128,128x64,64x32)                              */
     40960, /* Sum(256x128,128x64)                                    */
     32768, /* Sum(256x128)                                           */
         0, /* Base address (beginning of 512x256 level)              */
   -131072, /* - Sum(512x256)                                         */
   -655360, /* - Sum(1024x512,512x256)                                */
  -2752512, /* - Sum(2048x1024,1024x512,512x256)                      */
  },
  {                  /* 1:1 aspect ratio */
     87456, /* Sum(256x256,128x128,64x64,32x32,16x16,8x8,8x4,8x4,8x4) */
     87424, /* Sum(256x256,128x128,64x64,32x32,16x16,8x8,8x4,8x4)     */
     87392, /* Sum(256x256,128x128,64x64,32x32,16x16,8x8,8x4)         */
     87360, /* Sum(256x256,128x128,64x64,32x32,16x16,8x8)             */
     87296, /* Sum(256x256,128x128,64x64,32x32,16x16)                 */
     87040, /* Sum(256x256,128x128,64x64,32x32)                       */
     86016, /* Sum(256x256,128x128,64x64)                             */
     81920, /* Sum(256x256,128x128)                                   */
     65536, /* Sum(256x256)                                           */
         0, /* Base address (beginning of 512x512 level)              */
   -262144, /* - Sum(512x512)                                         */
  -1310720, /* - Sum(1024x1024,512x512)                               */
  -5505024, /* - Sum(2048x2048,1024x1024,512x512)                     */
  },
  {                  /* 1:2 aspect ratio */
     43808, /* Sum(128x256,64x128,32x64,16x32,8x16,8x8,8x4,8x4,8x4)   */
     43776, /* Sum(128x256,64x128,32x64,16x32,8x16,8x8,8x4,8x4)       */
     43744, /* Sum(128x256,64x128,32x64,16x32,8x16,8x8,8x4)           */
     43712, /* Sum(128x256,64x128,32x64,16x32,8x16,8x8)               */
     43648, /* Sum(128x256,64x128,32x64,16x32,8x16)                   */
     43520, /* Sum(128x256,64x128,32x64,16x32)                        */
     43008, /* Sum(128x256,64x128,32x64)                              */
     40960, /* Sum(128x256,64x128)                                    */
     32768, /* Sum(128x256)                                           */
         0, /* Base address (beginning of 256x512 level)              */
   -131072, /* - Sum(256x512)                                         */
   -655360, /* - Sum(512x1024,256x512)                                */
  -2752512, /* - Sum(1024x2048,512x1024,256x512)                      */
  },
  {                  /* 1:4 aspect ratio */
     22048, /* Sum(64x256,32x128,16x64,8x32,8x16,8x8,8x4,8x4,8x4)     */
     22016, /* Sum(64x256,32x128,16x64,8x32,8x16,8x8,8x4,8x4)         */
     21984, /* Sum(64x256,32x128,16x64,8x32,8x16,8x8,8x4)             */
     21952, /* Sum(64x256,32x128,16x64,8x32,8x16,8x8)                 */
     21888, /* Sum(64x256,32x128,16x64,8x32,8x16)                     */
     21760, /* Sum(64x256,32x128,16x64,8x32)                          */
     21504, /* Sum(64x256,32x128,16x64)                               */
     20480, /* Sum(64x256,32x128)                                     */
     16384, /* Sum(64x256)                                            */
         0, /* Base address (beginning of 128x512 level)              */
    -65536, /* - Sum(128x512)                                         */
   -327680, /* - Sum(256x1024,128x512)                                */
  -1376256, /* - Sum(512x2048,256x1024,128x512)                       */
  },
  {                  /* 1:8 aspect ratio */
     11296, /* Sum(32x256,16x128,8x64,8x32,8x16,8x8,8x4,8x4,8x4)      */
     11264, /* Sum(32x256,16x128,8x64,8x32,8x16,8x8,8x4,8x4)          */
     11232, /* Sum(32x256,16x128,8x64,8x32,8x16,8x8,8x4)              */
     11200, /* Sum(32x256,16x128,8x64,8x32,8x16,8x8)                  */
     11136, /* Sum(32x256,16x128,8x64,8x32,8x16)                      */
     11008, /* Sum(32x256,16x128,8x64,8x32)                           */
     10752, /* Sum(32x256,16x128,8x64)                                */
     10240, /* Sum(32x256,16x128)                                     */
      8192, /* Sum(32x256)                                            */
         0, /* Base address (beginning of 64x512 level)               */
    -32768, /* - Sum(64x512)                                          */
   -163840, /* - Sum(128x1024,64x512)                                 */
   -688128, /* - Sum(256x2048,128x1024,64x512)                        */
  },
};

/*
 * Similar to _grMipMapOffset[][], but for DXTn compressed
 * textures, where 4x4 is the minimum size.
 */
const FxI32 _grMipMapOffsetDXT[4][16] = {
  {                  /* 8:1 aspect ratio */
     11024, /* Sum(256x32,128x16,64x8,32x4,16x4,8x4,4x4,4x4,4x4)      */
     11008, /* Sum(256x32,128x16,64x8,32x4,16x4,8x4,4x4,4x4)          */
     10992, /* Sum(256x32,128x16,64x8,32x4,16x4,8x4,4x4)              */
     10976, /* Sum(256x32,128x16,64x8,32x4,16x4,8x4)                  */
     10944, /* Sum(256x32,128x16,64x8,32x4,16x4)                      */
     10880, /* Sum(256x32,128x16,64x8,32x4)                           */
     10752, /* Sum(256x32,128x16,64x8)                                */
     10240, /* Sum(256x32,128x16)                                     */
      8192, /* Sum(256x32)                                            */
         0, /* Base address (beginning of 512x64 level)               */
    -32768, /* - Sum(512x64)                                          */
   -163840, /* - Sum(1024x128,512x64)                                 */
   -688128, /* - Sum(2048x256,1024x128,512x64)                        */
  },
  {                  /* 4:1 aspect ratio */
     21904, /* Sum(256x64,128x32,64x16,32x8,16x4,8x4,4x4,4x4,4x4)     */
     21888, /* Sum(256x64,128x32,64x16,32x8,16x4,8x4,4x4,4x4)         */
     21872, /* Sum(256x64,128x32,64x16,32x8,16x4,8x4,4x4)             */
     21856, /* Sum(256x64,128x32,64x16,32x8,16x4,8x4)                 */
     21824, /* Sum(256x64,128x32,64x16,32x8,16x4)                     */
     21760, /* Sum(256x64,128x32,64x16,32x8)                          */
     21504, /* Sum(256x64,128x32,64x16)                               */
     20480, /* Sum(256x64,128x32)                                     */
     16384, /* Sum(256x64)                                            */
         0, /* Base address (beginning of 512x128 level)              */
    -65536, /* - Sum(512x128)                                         */
   -327680, /* - Sum(1024x256,512x128)                                */
  -1376256, /* - Sum(2048x512,1024x256,512x128)                       */
  },
  {                  /* 2:1 aspect ratio */
     43728, /* Sum(256x128,128x64,64x32,32x16,16x8,8x4,4x4,4x4,4x4)   */
     43712, /* Sum(256x128,128x64,64x32,32x16,16x8,8x4,4x4,4x4)       */
     43696, /* Sum(256x128,128x64,64x32,32x16,16x8,8x4,4x4)           */
     43680, /* Sum(256x128,128x64,64x32,32x16,16x8,8x4)               */
     43648, /* Sum(256x128,128x64,64x32,32x16,16x8)                   */
     43520, /* Sum(256x128,128x64,64x32,32x16)                        */
     43008, /* Sum(256x128,128x64,64x32)                              */
     40960, /* Sum(256x128,128x64)                                    */
     32768, /* Sum(256x128)                                           */
         0, /* Base address (beginning of 512x256 level)              */
   -131072, /* - Sum(512x256)                                         */
   -655360, /* - Sum(1024x512,512x256)                                */
  -2752512, /* - Sum(2048x1024,1024x512,512x256)                      */
  },
  {                  /* 1:1 aspect ratio */
     87408, /* Sum(256x256,128x128,64x64,32x32,16x16,8x8,4x4,4x4,4x4) */
     87392, /* Sum(256x256,128x128,64x64,32x32,16x16,8x8,4x4,4x4)     */
     87376, /* Sum(256x256,128x128,64x64,32x32,16x16,8x8,4x4)         */
     87360, /* Sum(256x256,128x128,64x64,32x32,16x16,8x8)             */
     87296, /* Sum(256x256,128x128,64x64,32x32,16x16)                 */
     87040, /* Sum(256x256,128x128,64x64,32x32)                       */
     86016, /* Sum(256x256,128x128,64x64)                             */
     81920, /* Sum(256x256,128x128)                                   */
     65536, /* Sum(256x256)                                           */
         0, /* Base address (beginning of 512x512 level)              */
   -262144, /* - Sum(512x512)                                         */
  -1310720, /* - Sum(1024x1024,512x512)                               */
  -5505024, /* - Sum(2048x2048,1024x1024,512x512)                     */
  },
};

const FxI32 _grMipMapOffset_Tsplit[4][16] =
{
  {  /* 8:1 and 1:8 aspect ratios */
        8741, /* Sum(256x32, 64x8, 16x2, 4x1, 1x1)         */
        2186, /* Sum(128x16, 32x4, 8x1, 2x1)               */
        8740, /* Sum(256x32, 64x8, 16x2, 4x1)              */
        2184, /* Sum(128x16, 32x4, 8x1)                    */
        8736, /* Sum(256x32, 64x8, 16x2)                   */
        2176, /* Sum(128x16, 32x4)                         */
        8704, /* Sum(256x32, 64x8)                         */
        2048, /* Sum(128x16)                               */
        8192, /* Sum(256x32)                               */
           0, /* Base address (beginning of 128x16 level)  */
           0, /* Base address (beginning of 256x32 level)  */
      -32768, /* - Sum(512x64)                             */
     -131072, /* - Sum(1024x128)                           */
     -557056, /* - Sum(2048x256, 512x64)                   */
  },
  {  /* 4:1 and 1:4 aspect ratios */
       17477, /* Sum(256x64, 64x16, 16x4, 4x1, 1x1)        */
        4370, /* Sum(128x32, 32x8, 8x2, 2x1)               */
       17476, /* Sum(256x64, 64x16, 16x4, 4x1)             */
        4368, /* Sum(128x32, 32x8, 8x2)                    */
       17472, /* Sum(256x64, 64x16, 16x4)                  */
        4352, /* Sum(128x32, 32x8)                         */
       17408, /* Sum(256x64, 64x16)                        */
        4096, /* Sum(128x32)                               */
       16384, /* Sum(256x64)                               */
           0, /* Base address (beginning of 128x32 level)  */
           0, /* Base address (beginning of 256x64 level)  */
      -65536, /* - Sum(512x128)                            */
     -262144, /* - Sum(1024x256)                           */
    -1114112, /* - Sum(2048x512, 512x128)                  */
  },
  {  /* 2:1 and 1:2 aspect ratios */
       34953, /* Sum(256x128, 64x32, 16x8, 4x2, 1x1)       */
        8738, /* Sum(128x64, 32x16, 8x4, 2x1)              */
       34952, /* Sum(256x128, 64x32, 16x8, 4x2)            */
        8736, /* Sum(128x64, 32x16, 8x4)                   */
       34944, /* Sum(256x128, 64x32, 16x8)                 */
        8704, /* Sum(128x64, 32x16)                        */
       34816, /* Sum(256x128, 64x32)                       */
        8192, /* Sum(128x64)                               */
       32768, /* Sum(256x128)                              */
           0, /* Base address (beginning of 128x64 level)  */
           0, /* Base address (beginning of 256x128 level) */
     -131072, /* - Sum(512x256)                            */
     -524288, /* - Sum(1024x512)                           */
    -2228224, /* - Sum(2048x1024, 512x256)                 */
  },
  {  /* 1:1 aspect ratio */
       69905, /* Sum(256x256, 64x64, 16x16, 4x4, 1x1)      */
       17476, /* Sum(128x128, 32x32, 8x8, 2x2)             */
       69904, /* Sum(256x256, 64x64, 16x16, 4x4)           */
       17472, /* Sum(128x128, 32x32, 8x8)                  */
       69888, /* Sum(256x256, 64x64, 16x16)                */
       17408, /* Sum(128x128, 32x32)                       */
       69632, /* Sum(256x256, 64x64)                       */
       16384, /* Sum(128x128)                              */
       65536, /* Sum(256x256)                              */
           0, /* Base address (beginning of 128x128 level) */
           0, /* Base address (beginning of 256x256 level) */
     -262144, /* - Sum(512x512)                            */
    -1048576, /* - Sum(1024x1024)                          */
    -4456448, /* - Sum(2048x2048, 512x512)                 */
  },
};

/*
 * Similar to _grMipMapOffset_Tsplit[][], but for 4-bit compressed
 * textures, where 8x4 is the minimum size. FXT1
 */
const FxI32 _grMipMapOffset_TsplitCmp4Bit[7][16] = {
  {                  /* 8:1 aspect ratio */
      8832, /* Sum(256x32,64x8,16x4,8x4,8x4)                          */
      2240, /* Sum(128x16,32x4,8x4,8x4)                               */
      8800, /* Sum(256x32,64x8,16x4,8x4)                              */
      2208, /* Sum(128x16,32x4,8x4)                                   */
      8768, /* Sum(256x32,64x8,16x4)                                  */
      2176, /* Sum(128x16,32x4)                                       */
      8704, /* Sum(256x32,64x8)                                       */
      2048, /* Sum(128x16)                                            */
      8192, /* Sum(256x32)                                            */
         0, /* Base address (beginning of 128x16 level)               */
         0, /* Base address (beginning of 256x32 level)               */
    -32768, /* - Sum(512x64)                                          */
   -131072, /* - Sum(1024x128)                                        */
   -557056, /* - Sum(2048x256,512x64)                                 */
  },
  {                  /* 4:1 aspect ratio */
     17536, /* Sum(256x64,64x16,16x4,8x4,8x4)                         */
      4416, /* Sum(128x32,32x8,8x4,8x4)                               */
     17504, /* Sum(256x64,64x16,16x4,8x4)                             */
      4384, /* Sum(128x32,32x8,8x4)                                   */
     17472, /* Sum(256x64,64x16,16x4)                                 */
      4352, /* Sum(128x32,32x8)                                       */
     17408, /* Sum(256x64,64x16)                                      */
      4096, /* Sum(128x32)                                            */
     16384, /* Sum(256x64)                                            */
         0, /* Base address (beginning of 128x32 level)               */
         0, /* Base address (beginning of 256x64 level)               */
    -65536, /* - Sum(512x128)                                         */
   -262144, /* - Sum(1024x256)                                        */
  -1114112, /* - Sum(2048x512,512x128)                                */
  },
  {                  /* 2:1 aspect ratio */
     35008, /* Sum(256x128,64x32,16x8,8x4,8x4)                        */
      8768, /* Sum(128x64,32x16,8x4,8x4)                              */
     34976, /* Sum(256x128,64x32,16x8,8x4)                            */
      8736, /* Sum(128x64,32x16,8x4)                                  */
     34944, /* Sum(256x128,64x32,16x8)                                */
      8704, /* Sum(128x64,32x16)                                      */
     34816, /* Sum(256x128,64x32)                                     */
      8192, /* Sum(128x64)                                            */
     32768, /* Sum(256x128)                                           */
         0, /* Base address (beginning of 128x64 level)               */
         0, /* Base address (beginning of 256x128 level)              */
   -131072, /* - Sum(512x256)                                         */
   -524288, /* - Sum(1024x512)                                        */
  -2228224, /* - Sum(2048x1024,512x256)                               */
  },
  {                  /* 1:1 aspect ratio */
     69952, /* Sum(256x256,64x64,16x16,8x4,8x4)                       */
     17504, /* Sum(128x128,32x32,8x8,8x4)                             */
     69920, /* Sum(256x256,64x64,16x16,8x4)                           */
     17472, /* Sum(128x128,32x32,8x8)                                 */
     69888, /* Sum(256x256,64x64,16x16)                               */
     17408, /* Sum(128x128,32x32)                                     */
     69632, /* Sum(256x256,64x64)                                     */
     16384, /* Sum(128x128)                                           */
     65536, /* Sum(256x256)                                           */
         0, /* Base address (beginning of 128x128 level)              */
         0, /* Base address (beginning of 256x256 level)              */
   -262144, /* - Sum(512x512)                                         */
  -1048576, /* - Sum(1024x1024)                                       */
  -4456448, /* - Sum(2048x2048,512x512)                               */
  },
  {                  /* 1:2 aspect ratio */
     35008, /* Sum(128x256,32x64,8x16,8x4,8x4)                        */
      8800, /* Sum(64x128,16x32,8x8,8x4)                              */
     34976, /* Sum(128x256,32x64,8x16,8x4)                            */
      8768, /* Sum(64x128,16x32,8x8)                                  */
     34944, /* Sum(128x256,32x64,8x16)                                */
      8704, /* Sum(64x128,16x32)                                      */
     34816, /* Sum(128x256,32x64)                                     */
      8192, /* Sum(64x128)                                            */
     32768, /* Sum(128x256)                                           */
         0, /* Base address (beginning of 64x128 level)               */
         0, /* Base address (beginning of 128x256 level)              */
   -131072, /* - Sum(256x512)                                         */
   -524288, /* - Sum(512x1024)                                        */
  -2228224, /* - Sum(1024x2048,256x512)                               */
  },
  {                  /* 1:4 aspect ratio */
     17600, /* Sum(64x256,16x64,8x16,8x4,8x4)                         */
      4448, /* Sum(32x128,8x32,8x8,8x4)                               */
     17568, /* Sum(64x256,16x64,8x16,8x4)                             */
      4416, /* Sum(32x128,8x32,8x8)                                   */
     17536, /* Sum(64x256,16x64,8x16)                                 */
      4352, /* Sum(32x128,8x32)                                       */
     17408, /* Sum(64x256,16x64)                                      */
      4096, /* Sum(32x128)                                            */
     16384, /* Sum(64x256)                                            */
         0, /* Base address (beginning of 32x128 level)               */
         0, /* Base address (beginning of 64x256 level)               */
    -65536, /* - Sum(128x512)                                         */
   -262144, /* - Sum(256x1024)                                        */
  -1114112, /* - Sum(512x2048,128x512)                                */
  },
  {                  /* 1:8 aspect ratio */
      8896, /* Sum(32x256,8x64,8x16,8x4,8x4)                          */
      2400, /* Sum(16x128,8x32,8x8,8x4)                               */
      8864, /* Sum(32x256,8x64,8x16,8x4)                              */
      2368, /* Sum(16x128,8x32,8x8)                                   */
      8832, /* Sum(32x256,8x64,8x16)                                  */
      2304, /* Sum(16x128,8x32)                                       */
      8704, /* Sum(32x256,8x64)                                       */
      2048, /* Sum(16x128)                                            */
      8192, /* Sum(32x256)                                            */
         0, /* Base address (beginning of 16x128 level)               */
         0, /* Base address (beginning of 32x256 level)               */
    -32768, /* - Sum(64x512)                                          */
   -131072, /* - Sum(128x1024)                                        */
   -557056, /* - Sum(256x2048,64x512)                                 */
  },
};

/*
 * Similar to _grMipMapOffset_Tsplit[][], but for DXTn compressed
 * textures, where 4x4 is the minimum size.
 */
const FxI32 _grMipMapOffset_TsplitDXT[4][16] = {
  {                  /* 8:1 aspect ratio */
      8800, /* Sum(256x32,64x8,16x4,4x4,4x4)                          */
      2224, /* Sum(128x16,32x4,8x4,4x4)                               */
      8784, /* Sum(256x32,64x8,16x4,4x4)                              */
      2208, /* Sum(128x16,32x4,8x4)                                   */
      8768, /* Sum(256x32,64x8,16x4)                                  */
      2176, /* Sum(128x16,32x4)                                       */
      8704, /* Sum(256x32,64x8)                                       */
      2048, /* Sum(128x16)                                            */
      8192, /* Sum(256x32)                                            */
         0, /* Base address (beginning of 128x16 level)               */
         0, /* Base address (beginning of 256x32 level)               */
    -32768, /* - Sum(512x64)                                          */
   -131072, /* - Sum(1024x128)                                        */
   -557056, /* - Sum(2048x256,512x64)                                 */
  },
  {                  /* 4:1 aspect ratio */
     17504, /* Sum(256x64,64x16,16x4,4x4,4x4)                         */
      4400, /* Sum(128x32,32x8,8x4,4x4)                               */
     17488, /* Sum(256x64,64x16,16x4,4x4)                             */
      4384, /* Sum(128x32,32x8,8x4)                                   */
     17472, /* Sum(256x64,64x16,16x4)                                 */
      4352, /* Sum(128x32,32x8)                                       */
     17408, /* Sum(256x64,64x16)                                      */
      4096, /* Sum(128x32)                                            */
     16384, /* Sum(256x64)                                            */
         0, /* Base address (beginning of 128x32 level)               */
         0, /* Base address (beginning of 256x64 level)               */
    -65536, /* - Sum(512x128)                                         */
   -262144, /* - Sum(1024x256)                                        */
  -1114112, /* - Sum(2048x512,512x128)                                */
  },
  {                  /* 2:1 aspect ratio */
     34976, /* Sum(256x128,64x32,16x8,4x4,4x4)                        */
      8752, /* Sum(128x64,32x16,8x4,4x4)                              */
     34960, /* Sum(256x128,64x32,16x8,4x4)                            */
      8736, /* Sum(128x64,32x16,8x4)                                  */
     34944, /* Sum(256x128,64x32,16x8)                                */
      8704, /* Sum(128x64,32x16)                                      */
     34816, /* Sum(256x128,64x32)                                     */
      8192, /* Sum(128x64)                                            */
     32768, /* Sum(256x128)                                           */
         0, /* Base address (beginning of 128x64 level)               */
         0, /* Base address (beginning of 256x128 level)              */
   -131072, /* - Sum(512x256)                                         */
   -524288, /* - Sum(1024x512)                                        */
  -2228224, /* - Sum(2048x1024,512x256)                               */
  },
  {                  /* 1:1 aspect ratio */
     69920, /* Sum(256x256,64x64,16x16,4x4,4x4)                       */
     17488, /* Sum(128x128,32x32,8x8,4x4)                             */
     69904, /* Sum(256x256,64x64,16x16,4x4)                           */
     17472, /* Sum(128x128,32x32,8x8)                                 */
     69888, /* Sum(256x256,64x64,16x16)                               */
     17408, /* Sum(128x128,32x32)                                     */
     69632, /* Sum(256x256,64x64)                                     */
     16384, /* Sum(128x128)                                           */
     65536, /* Sum(256x256)                                           */
         0, /* Base address (beginning of 128x128 level)              */
         0, /* Base address (beginning of 256x256 level)              */
   -262144, /* - Sum(512x512)                                         */
  -1048576, /* - Sum(1024x1024)                                       */
  -4456448, /* - Sum(2048x2048,512x512)                               */
  },
};

/*---------------------------------------------------------------------------
**  This is not DI anymore.  Perhaps all of these size routines need
**  to be in gtex.c now.
*/
FxU32
_grTexTextureMemRequired( GrLOD_t small_lod, GrLOD_t large_lod, 
                          GrAspectRatio_t aspect, GrTextureFormat_t format,
                          FxU32 evenOdd,
                          FxBool roundP ) 
{
#define FN_NAME "_grTexTextureMemRequired"
  FxU32 memrequired;

  GDBG_INFO(80,"_grTexTextureMemRequired(%d,%d, %d,%d, %d, %d)\n",
                 small_lod, large_lod,
                 aspect, format,
                 evenOdd,
                 roundP);

  GR_CHECK_W(FN_NAME, 
             small_lod > large_lod,
             "small_lod bigger than large_lod" );
  GR_CHECK_F(FN_NAME, 
             evenOdd >  GR_MIPMAPLEVELMASK_BOTH || evenOdd == 0, 
             "invalid evenOdd mask" );
  GR_CHECK_F(FN_NAME, _grBitsPerTexel[format] == 0, "invalid texture format");

  switch(format) {
  case GR_TEXFMT_ARGB_CMP_FXT1:
  case GR_TEXFMT_ARGB_CMP_DXT1: /* HACK: align texture base address for
                                 * the smallest mipmap.
                                 * See grTexDownloadMipMapLevelPartial.
                                 */
    /* In this case, do not mirror the aspect ratios, as the minimum
     * size of a mipmap level is 8x4, so the tables are not symmetric
     * w.r.t. sign of the aspect ratio, so keep the sign. */
    if ( evenOdd == GR_MIPMAPLEVELMASK_BOTH ) {
      memrequired  = _grMipMapOffsetCmp4Bit[G3_ASPECT_TRANSLATE(aspect)]
        [small_lod];
      memrequired -= _grMipMapOffsetCmp4Bit[G3_ASPECT_TRANSLATE(aspect)]
        [large_lod+1];
    } else { 
      memrequired = 0;
      
      /* construct XOR mask   */
      evenOdd = (evenOdd == GR_MIPMAPLEVELMASK_EVEN) ? 1 : 0;  
      while (large_lod >= small_lod) {    /* sum up all the mipmap levels */
        if ((large_lod ^ evenOdd) & 1)    /* that match the XOR mask      */
          memrequired += _grMipMapSizeCmp4Bit[G3_ASPECT_TRANSLATE(aspect)]
            [large_lod];
        large_lod--;
      }
    }
    break;

  case GR_TEXFMT_ARGB_CMP_DXT2:
  case GR_TEXFMT_ARGB_CMP_DXT3:
  case GR_TEXFMT_ARGB_CMP_DXT4:
  case GR_TEXFMT_ARGB_CMP_DXT5:
    /* DXT2,3,4,5 formats allow us to mirror the aspect ratios because
       the table entries are the same */
    if (aspect < GR_ASPECT_LOG2_1x1) aspect = -aspect;
    
    if ( evenOdd == GR_MIPMAPLEVELMASK_BOTH ) {
      memrequired  = _grMipMapOffsetDXT[G3_ASPECT_TRANSLATE(aspect)][small_lod];
      memrequired -= _grMipMapOffsetDXT[G3_ASPECT_TRANSLATE(aspect)][large_lod+1];
    } else { 
      memrequired = 0;
      
      /* construct XOR mask   */
      evenOdd = (evenOdd == GR_MIPMAPLEVELMASK_EVEN) ? 1 : 0;  
      while (large_lod >= small_lod) {    /* sum up all the mipmap levels */
        if ((large_lod ^ evenOdd) & 1)    /* that match the XOR mask      */
          memrequired += _grMipMapSizeDXT[G3_ASPECT_TRANSLATE(aspect)][large_lod];
        large_lod--;
      }
    }
    break;
    
  default:
    /* Non-FXT1 formats allow us to mirror the aspect ratios because
       the table entries are the same for, e.g., 1x8 as for 8x1 */
    if (aspect < GR_ASPECT_LOG2_1x1) aspect = -aspect;
    
    if ( evenOdd == GR_MIPMAPLEVELMASK_BOTH ) {
      memrequired  = _grMipMapOffset[G3_ASPECT_TRANSLATE(aspect)][small_lod];
      memrequired -= _grMipMapOffset[G3_ASPECT_TRANSLATE(aspect)][large_lod+1];
    } else { 
      memrequired = 0;
      
      /* construct XOR mask   */
      evenOdd = (evenOdd == GR_MIPMAPLEVELMASK_EVEN) ? 1 : 0;  
      while (large_lod >= small_lod) {    /* sum up all the mipmap levels */
        if ((large_lod ^ evenOdd) & 1)    /* that match the XOR mask      */
          memrequired += _grMipMapSize[G3_ASPECT_TRANSLATE(aspect)][large_lod];
        large_lod--;
      }
    }
    
  }
  
  /* convert from texels to bytes */
  memrequired *= _grBitsPerTexel[format]; /* bits  */
  memrequired >>= 3;                      /* bytes */

  if (roundP) {
    /* round up to hw alignment boundary  */
    memrequired += SST_TEXTURE_ALIGN_MASK;
    memrequired &= ~SST_TEXTURE_ALIGN_MASK;
  }

  return memrequired;
#undef FN_NAME
} /* _grTexTextureMemRequired */

#if 0 /* KoolSmoky - remove junk */
FxU16
_grTexFloatLODToFixedLOD( float value )
{
  float num_quarters;
  int   new_value;

  num_quarters = ( value + .125F ) / .25F;
  new_value    = ( int ) num_quarters;

  new_value   &= 0x003F;

  return new_value;
} /* _grTexFloatLODToFixedLOD */
#endif

/*---------------------------------------------------------------------------
** _grTexCalcBaseAddress
*/
FxU32
_grTexCalcBaseAddress( FxU32 start, GrLOD_t large_lod,
                       GrAspectRatio_t aspect, GrTextureFormat_t format,
                       FxU32 odd_even_mask )
{
#define FN_NAME "_grTexCalcBaseAddress"
  FxU32 sum_of_lod_sizes;

  GR_CHECK_F(FN_NAME, _grBitsPerTexel[format] == 0, "invalid texture format");

  switch(format) {
  case GR_TEXFMT_ARGB_CMP_FXT1:
  case GR_TEXFMT_ARGB_CMP_DXT1: /* HACK: compensate for the hack to align the
                                 * base address in _grTexTextureMemRequired.
                                 */
     /* FXT1 format: Don't mirror the aspect ratios, because of the 8x4 limit */
    if ( odd_even_mask == GR_MIPMAPLEVELMASK_BOTH ) {
      sum_of_lod_sizes = _grMipMapOffsetCmp4Bit[aspect]
        [large_lod + 1];
    } else {
      if (((odd_even_mask == GR_MIPMAPLEVELMASK_EVEN) && (large_lod & 1)) ||
          ((odd_even_mask == GR_MIPMAPLEVELMASK_ODD) && !(large_lod & 1)))
        large_lod += 1 ;
      else
        large_lod += 2 ; /* as it turns out, this is important for FXT1 as well */
		
      sum_of_lod_sizes =
        _grMipMapOffset_TsplitCmp4Bit[aspect][large_lod];
    }
    break;

  case GR_TEXFMT_ARGB_CMP_DXT2:
  case GR_TEXFMT_ARGB_CMP_DXT3:
  case GR_TEXFMT_ARGB_CMP_DXT4:
  case GR_TEXFMT_ARGB_CMP_DXT5:
    /* DXT2,3,4,5 format: mirror the aspect ratios */
    if ( aspect > G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x1) )
      aspect =  G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x8) - aspect;
    
    if ( odd_even_mask == GR_MIPMAPLEVELMASK_BOTH ) {
      sum_of_lod_sizes = _grMipMapOffsetDXT[aspect][large_lod + 1];
    } else {
      if (((odd_even_mask == GR_MIPMAPLEVELMASK_EVEN) && (large_lod & 1)) ||
          ((odd_even_mask == GR_MIPMAPLEVELMASK_ODD) && !(large_lod & 1)))
        large_lod += 1 ;
      else
        large_lod += 2 ;
		
      sum_of_lod_sizes = _grMipMapOffset_TsplitDXT[aspect][large_lod];
    }
    break;
    
  default:
    if ( aspect > G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x1) )
      /* mirror aspect ratios */
      aspect =  G3_ASPECT_TRANSLATE(GR_ASPECT_LOG2_1x8) - aspect;
    
    if ( odd_even_mask == GR_MIPMAPLEVELMASK_BOTH ) {
      sum_of_lod_sizes = _grMipMapOffset[aspect][large_lod + 1];
    } else {
      if (((odd_even_mask == GR_MIPMAPLEVELMASK_EVEN) && (large_lod & 1)) ||
          ((odd_even_mask == GR_MIPMAPLEVELMASK_ODD) && !(large_lod & 1)))
        large_lod += 1;
      else /* jcochrane - original reading wrong offset values for single pass trilinear, tsplit across tmu's */
        large_lod += 2;	/* this is the correct offset for (even large lod && odd mipmap level) || (odd large lod && even mipmap level) */

      sum_of_lod_sizes = _grMipMapOffset_Tsplit[aspect][large_lod];
    }
  }
  
  /* Convert from texels to bytes */
  sum_of_lod_sizes *= _grBitsPerTexel[format]; /* bits  */
  sum_of_lod_sizes >>= 3;                      /* bytes */

  /* Clamp the size down. The hw is still going to use the complete
   * size computation, but it always starts from an aligned baseAddr
   * so we need to take this into account before doing the subtraction
   * otherwise we'll be on the wrong 'side' of the minimum aligned
   * allocation unit.  
   */
  sum_of_lod_sizes &= ~SST_TEXTURE_ALIGN_MASK;

  return ( start - sum_of_lod_sizes );
#undef FN_NAME
} /* _grTexCalcBaseAddress */

/*---------------------------------------------------------------------------
** grTexCalcMemRequired
*/
GR_DIENTRY(grTexCalcMemRequired, FxU32,
           ( GrLOD_t small_lod, GrLOD_t large_lod, 
            GrAspectRatio_t aspect, GrTextureFormat_t format ))
{
  FxU32 memrequired;
  
  memrequired = _grTexTextureMemRequired(small_lod, large_lod, 
                                         aspect, format,
                                         GR_MIPMAPLEVELMASK_BOTH,
                                         FXTRUE);
  GDBG_INFO(88,"grTexCalcMemRequired(%d,%d,%d,%d) => 0x%x(%d)\n",
                small_lod,large_lod,aspect,format,memrequired,memrequired);
  return memrequired;
} /* grTexCalcMemRequired */


/*---------------------------------------------------------------------------
** grTexDetailControl
*/
GR_DIENTRY(grTexDetailControl, void,
           ( GrChipID_t tmu, int lod_bias, FxU8 detail_scale,
            float detail_max ))
{
#define FN_NAME "grTexDetailControl"
  FxU32 tDetail;
  FxU32 dmax    = ( FxU32 ) ( detail_max * _GlideRoot.pool.f255 );
  FxU32 dscale  = detail_scale;
  
  GR_BEGIN_NOFIFOCHECK("grTexDetailControl",88);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%g)\n",tmu,detail_scale,detail_max);
  GR_CHECK_TMU( FN_NAME, tmu );
  GR_CHECK_F( myName, lod_bias < -32 || lod_bias > 31, "lod_bias out of range" );
  GR_CHECK_F( myName, detail_scale > 7, "detail_scale out of range" );
  GR_CHECK_F( myName, detail_max < 0.0 || detail_max > 1.0, "detail_max out of range" );

  tDetail  = ( ( lod_bias << SST_DETAIL_BIAS_SHIFT ) & SST_DETAIL_BIAS );
  tDetail |= ( ( dmax << SST_DETAIL_MAX_SHIFT ) & SST_DETAIL_MAX );
  tDetail |= ( ( dscale << SST_DETAIL_SCALE_SHIFT ) & SST_DETAIL_SCALE );
  
  /* MULTIPLAT */
  _grTexDetailControl( tmu, tDetail );
  GR_END();
#undef FN_NAME
} /* grTexDetailControl */

/* Matt M. - Richardson - (edited by CHD)
**
**   Changes below fix PRS #585
**   Some games cause an error when  launching with Glide3x o
**   V3.  This is a work around for an application fault.
**   Games are  calling  grTexMinAddress before we get a call t
**   winOpen.  The code below replaces GR_BEGIN_NOFIFOCHECK_RET
**   Note that this has been a somewhat common error for MaxAddress
**   and MinAddress, so I've go a workaround in both places.
*/

GR_DIENTRY(grTexMinAddress, FxU32, ( GrChipID_t tmu ))
{
#define FN_NAME "grTexMinAddress"
  GR_DCL_GC;            /* Initialize GC */
  GR_DCL_HW_INIT;       /* Declare *hw variable */
  GR_DEBUG_DCL_INIT();  /* Declare debug variables */
  
  if (!gc)              /* If Gc is not valid, return 0 */
    return 0;    
  if (!gc->base_ptr)    /* If we hit one of these, something didn't
                           get called in the right */
    return 0;           /* order by the application. */
#if defined( GLIDE_INIT_HWC )
  if (!gc->bInfo)
        return 0;
#endif
  if (!gc->tex_ptr)
        return 0;

  GR_DEBUG_DCL_STAGE2("grTexMinAddress", 88);   

  if (!gc->sstRegs)
    return 0;

  GR_DCL_HW_STAGE2;
     
  FXUNUSED(hw); 
  
#if GLIDE_CHECK_CONTEXT
  if (!gc->lostContext)  
    return 0; 
  
  if (gc->lostContext) {
    if (*gc->lostContext) { 
      return 0;
    }
  }     
#endif /* GLIDE_CHECK_CONTEXT */

  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",tmu);
  GR_CHECK_TMU(FN_NAME, tmu);
  FXUNUSED( tmu );
  GR_RETURN(0);
#undef FN_NAME
} /* grTexMinAddress */


/*-------------------------------------------------------------------
  Function: grTexMaxAddress
  Date: 6/2
  Implementor(s): GaryT
  Library: glide
  Description:
    Returns address of maximum extent of texture ram for a given TMU
  Arguments:
    tmu
  Return:
    the largest valid texture start Address
  -------------------------------------------------------------------*/
GR_DIENTRY(grTexMaxAddress, FxU32, ( GrChipID_t tmu ))
{
#define FN_NAME "grTexMaxAddress"
  GR_DCL_GC;            /* Initialize GC */
  GR_DCL_HW_INIT;       /* Declare *hw variable */
  GR_DEBUG_DCL_INIT();  /* Declare debug variables */
#define MIN_TEX_MEM  0x200000
  /* Matt M. - Richardson -  Changes below fix PRS #585
  **   Some games cause an error when  launching with Glide3x o
  **   V3.  This is a work around for an application fault
  **   Games are  calling  grTexMinAddress before we get a call t
  **   winOpen.  The code below replaces GR_BEGIN_NOFIFOCHECK_RET
  */
  if (!gc)                      /* If Gc is not valid, return 0 */
    return MIN_TEX_MEM;         /* Always guaranteed */
  if (!gc->base_ptr)            /* If we hit one of these, something */
    return MIN_TEX_MEM;         /* didn't get called in the right */
                                /* order by the application. */
#if defined( GLIDE_INIT_HWC )
  if (!gc->bInfo)
    return MIN_TEX_MEM;
#endif
  if (!gc->rawLfb)              /* since we use rawLfb for texture download */
    return MIN_TEX_MEM;


  GR_DEBUG_DCL_STAGE2("grTexMaxAddress", 88);   

  if (!gc->sstRegs)
    return 0;

  GR_DCL_HW_STAGE2;
     
  FXUNUSED(hw); 
  
#if GLIDE_CHECK_CONTEXT
  if (!gc->lostContext)  
    return 0; 
  
  if (gc->lostContext) {
    if (*gc->lostContext) { 
      return 0;
    }
  }     
#endif /* GLIDE_CHECK_CONTEXT */

  GDBG_INFO_MORE(gc->myLevel,"(%d)\n",tmu);
  GR_CHECK_TMU(FN_NAME, tmu );
  GR_RETURN(gc->tmu_state[tmu].total_mem - SST_TEXTURE_ALIGN);
#undef FN_NAME
} /* grTexMaxAddress */


/*-------------------------------------------------------------------
  Function: grTexTextureMemRequired
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Returns the tmu memory required to store the specified mipmap    
    ( Gary and I don't like the name of this function, but are 
      a little backed into a corner because of the existence 
      of grTexMemRequired() which does not imply any distinction
      between texture memory and system ram )
  Arguments:
    evenOdd  - which set of mipmap levels are to be stored
                One of:
                  GR_MIPMAPLEVELMASK_EVEN 
                  GR_MIPMAPLEVELMASK_ODD
                  GR_MIPMAPLEVELMASK_BOTH
    info      - pointer to GrTexInfo structure defining dimensions
                of texture
  Return:
    offset to be added to current texture base address to calculate next 
    valid texture memory download location
  -------------------------------------------------------------------*/
GR_DIENTRY(grTexTextureMemRequired, FxU32,
           ( FxU32 evenOdd, GrTexInfo *info))
{
  FxU32 memrequired;

  GR_CHECK_F( "grTexTextureMemRequired", !info, "invalid info pointer" );
  memrequired = _grTexTextureMemRequired( info->smallLodLog2,
                                          info->largeLodLog2,
                                          info->aspectRatioLog2,
                                          info->format,
                                          evenOdd,
                                          FXTRUE );
                        
  GDBG_INFO(88,"grTexTextureMemRequired(%d,0x%x) => 0x%x(%d)\n",
                evenOdd,info,memrequired,memrequired);

  return memrequired;
} /* grTexTextureMemRequired */


/*-------------------------------------------------------------------
  Function: grTexDownloadMipMap
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Downloads a texture mipmap to the specified tmu at the specified
    base address.
  Arguments:
    tmu          - which tmu
    startAddress - starting address for texture download,
    evenOdd  - which set of mipmap levels have been downloaded for
                the selected texture
                One of:
                  GR_MIPMAPLEVELMASK_EVEN
                  GR_MIPMAPLEVELMASK_ODD
                  GR_MIPMAPLEVELMASK_BOTH
    info       - pointer to GrTexInfo structure defining dimension of
                 texture to be downloaded and containing texture data
  Return:
    none
  -------------------------------------------------------------------*/
GR_DIENTRY(grTexDownloadMipMap, void, 
           ( GrChipID_t tmu, FxU32 startAddress, FxU32
            evenOdd, GrTexInfo  *info ))
{
#define FN_NAME "grTexDownloadMipMap"
  GrLOD_t 
    lod;
  FxU8* 
    src_base = info->data;
  const GrAspectRatio_t 
    curAspectRatio = ((info->aspectRatioLog2 < 0)
                      ? -info->aspectRatioLog2
                      : info->aspectRatioLog2);
  const FxU32
    formatMult = _grBitsPerTexel[info->format];

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 89);
  GDBG_INFO_MORE(gc->myLevel, 
                 "(%d, 0x%X, 0x%X, 0x%X\n",
                 tmu, startAddress, evenOdd, info);
  GR_CHECK_F(FN_NAME, formatMult == 0, "invalid texture format");

#if GLIDE_CHECK_COMPATABILITY && GLIDE_DEBUG
  {
    GR_DCL_GC;
    const FxU32 
      size = grTexTextureMemRequired( evenOdd, info );
    
    GR_CHECK_TMU( FN_NAME, tmu );
    GR_CHECK_COMPATABILITY(FN_NAME, 
                           startAddress + size > gc->tmu_state[tmu].total_mem,
                           "insufficient texture ram at startAddress" );
    GR_CHECK_F(FN_NAME, evenOdd > 0x3, "evenOdd mask invalid" );
    GR_CHECK_F(FN_NAME, !info, "info invalid" );
  }
#endif

  {
    struct GrTmuMemInfo*
      memInfo = gc->tmuMemInfo + tmu;
    
    /* Force a pixel flush which should force all of the
     * texture downloads to flush from internal fifos etc.  
     */
    GR_TEX_FLUSH_PRE(memInfo);
    
    
    /*---------------------------------------------------------------
      Download one mipmap level at a time
      ---------------------------------------------------------------*/
    for( lod = info->largeLodLog2; lod >= info->smallLodLog2; lod-- ) {
      /*
      ** note for glide3 lod translation:
      ** we are calling gr* routine so the lod data should remain the same 
      */
      grTexDownloadMipMapLevel(tmu, 
                               startAddress, 
                               lod,
                               info->largeLodLog2,
                               info->aspectRatioLog2,
                               info->format,
                               evenOdd,
                               src_base);

      switch(info->format) {
      case GR_TEXFMT_ARGB_CMP_FXT1:
        /* Note: in this case, we need to use info->aspectRatioLog2
         * rather than curAspectRatio, because we need to keep the
         * sign of the apsect ratio in order to get the right size. */
        src_base += ((_grMipMapHostSizeCmp4Bit
                      [G3_ASPECT_TRANSLATE(info->aspectRatioLog2)][lod]
                      * formatMult) >> 3);
        break;

      case GR_TEXFMT_ARGB_CMP_DXT1:
      case GR_TEXFMT_ARGB_CMP_DXT2:
      case GR_TEXFMT_ARGB_CMP_DXT3:
      case GR_TEXFMT_ARGB_CMP_DXT4:
      case GR_TEXFMT_ARGB_CMP_DXT5:
        src_base += ((_grMipMapHostSizeDXT[curAspectRatio][lod] * formatMult)>>3);
        break;

      default:
        src_base += ((_grMipMapHostSize[curAspectRatio][lod] * formatMult)>>3);
      }
      
    }

    /* Force a pixel flush which should force all of the
     * texture downloads to flush from internal fifos etc.  
     */    
    GR_TEX_FLUSH_POST(memInfo);
  }

#undef FN_NAME
} /* grTexDownloadMipMap */


/*-------------------------------------------------------------------
  Function: grTexDownloadTablePartial
  Date: 6/3
  Implementor(s): GaryT
  Library: glide
  Description:
    download part of a look up table data to a tmu
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
GR_DIENTRY(grTexDownloadTablePartial, void,
           ( GrTexTable_t type, 
            void *data, int start, int end ))
{
#define FN_NAME "grTexDownloadTablePartial"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 89);
  GDBG_INFO_MORE(gc->myLevel, "(%d, 0x%X, %d, %d)\n",type,data,start,end);

  GR_CHECK_F(myName, type > GR_TEXTABLE_PALETTE_6666_EXT, "invalid table specified");
  GR_CHECK_F(myName, !data, "invalid data pointer");

  /* What type of palette is this? */
  switch(type) {
  case GR_TEXTABLE_PALETTE:
  case GR_TEXTABLE_PALETTE_6666_EXT:
    _grTexDownloadPalette( GR_TMU0, type, (GuTexPalette *)data, start, end );
    break;
  default:
    _grTexDownloadNccTable( GR_TMU0, type, (GuNccTable*)data, start, end );
  }

  /* NB: Set the current palette type after we do the download because
   * the palette download code may need to know that there is a table
   * type change and do something hoopti.  
   */
  gc->state.tex_table = type;

  GR_END();
#undef FN_NAME
} /* grTexDownloadTablePartial */

/*---------------------------------------------------------------------------
** grTexDownloadMipMapLevel
*/
GR_DIENTRY(grTexDownloadMipMapLevel, void, 
           ( GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod,
            GrLOD_t largeLod, GrAspectRatio_t aspectRatio,
            GrTextureFormat_t format, FxU32 evenOdd, void *data )) 
{
  GR_BEGIN_NOFIFOCHECK("grTexDownloadMipMapLevel",89);
  GDBG_INFO_MORE(gc->myLevel,"(%d,0x%x, %d,%d,%d, %d,%d 0x%x)\n",
                  tmu,startAddress,thisLod,largeLod,aspectRatio,
                  format,evenOdd,data);
  /*
  ** note for glide3 lod translation:
  ** we are calling gr* routine so the lod data should remain the same 
  */
  switch(format) {
  case GR_TEXFMT_ARGB_CMP_FXT1:
    grTexDownloadMipMapLevelPartial(tmu, startAddress,
                                    thisLod, largeLod, aspectRatio, 
                                    format,
                                    evenOdd, data,
                                    0, 
                                    _grMipMapHostWHCmp4Bit[G3_ASPECT_TRANSLATE(aspectRatio)][thisLod][1] - 1);
    break;

  case GR_TEXFMT_ARGB_CMP_DXT1:
  case GR_TEXFMT_ARGB_CMP_DXT2:
  case GR_TEXFMT_ARGB_CMP_DXT3:
  case GR_TEXFMT_ARGB_CMP_DXT4:
  case GR_TEXFMT_ARGB_CMP_DXT5:
    grTexDownloadMipMapLevelPartial(tmu, startAddress,
                                    thisLod, largeLod, aspectRatio, 
                                    format,
                                    evenOdd, data,
                                    0, 
                                    _grMipMapHostWHDXT[G3_ASPECT_TRANSLATE(aspectRatio)][thisLod][1] - 1);
    break;

  default:
     grTexDownloadMipMapLevelPartial(tmu, startAddress,
                                    thisLod, largeLod, aspectRatio, 
                                    format,
                                    evenOdd, data,
                                    0, 
                                    _grMipMapHostWH[G3_ASPECT_TRANSLATE(aspectRatio)][thisLod][1] - 1);
  }

  GR_END();
} /* grTexDownloadMipmapLevel */

#if GLIDE_POINTCAST_PALETTE

GR_EXT_ENTRY(grTexDownloadTablePartialExt, void,
           ( GrChipID_t tmu, GrTexTable_t type, 
            void *data, int start, int end ))
{
#define FN_NAME "grTexDownloadTablePartialExt"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 89);
  GDBG_INFO_MORE(gc->myLevel, "(%d, %d, 0x%X, %d, %d)\n",tmu, type,data,start,end);

  GR_CHECK_F(myName, type > GR_TEXTABLE_PALETTE, "invalid table specified");
  GR_CHECK_F(myName, !data, "invalid data pointer");

  /* What type of palette is this? */
  if ((type == GR_TEXTABLE_PALETTE) || (type == GR_TEXTABLE_PALETTE_6666_EXT)){
    _grTexDownloadPalette( tmu, type, (GuTexPalette *)data, start, end );
  } else {
    _grTexDownloadNccTable( tmu, type, (GuNccTable*)data, start, end );
  }

  /* NB: Set the current palette type after we do the download because
   * the palette download code may need to know that there is a table
   * type change and do something hoopti.  
   */
  gc->state.tex_table = type;

  GR_END();
#undef FN_NAME
} /* grTexDownloadTablePartialExt */
#endif
