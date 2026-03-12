#ifndef __H3DEFS_H__
#define __H3DEFS_H__

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
*/

#if defined(H4)
#define MAX_NUM_TMUS 2
#else
#define MAX_NUM_TMUS 1
#endif

// SST commands
#define SST_NOPCMD      1
#define SST_TRIANGLECMD 2
#define SST_FASTFILLCMD 3
#define SST_SWAPBUFCMD  4
#define SST_SBEGINTRICMD 5
#define SST_SDRAWTRICMD 6
#define SST_USERINTRCMD 7

// this crazy macro fixes the binary point within a floating point
// number so that it has 'fracbits' to the right of the binary point
// it also adds a '1' bit to the MSB so that slightly negative numbers
// end up looking like twos complement numbers (they carry out from the '1')
#define FLOAT_FIX(f,fracbits) ((f)+(float)(3L<<(22-(fracbits))))

#define SST_FLOAT(f,scale,shift) (unsigned long)((f)*((scale)*(float)(1L<<shift)))
#define SST_TLOD_MINMAX(lodmin,lodmax) \
                (((lodmin) << (SST_LODMIN_SHIFT)) | \
                ((lodmax) << (SST_LODMAX_SHIFT)))
#define SST_TLOD_MINMAX_INT(lodmin,lodmax) \
                (((lodmin) << (SST_LOD_FRACBITS+SST_LODMIN_SHIFT)) | \
                ((lodmax) << (SST_LOD_FRACBITS+SST_LODMAX_SHIFT)))

// this crazy macro tests the sign bit of a float by loading it into
// an integer register and then testing the sign bit of the integer
#define FLOAT_ISNEG(f) ((*(int *)(&(f))) < 0)

// these crazy macros returns the sign of a number (1 if >= 0; -1 if < 0)
#define ISIGN(x) (((x) | 0x40000000L) >> 30)
#define FSIGN(f) ISIGN(*(long *)&f)

#define BIT(n)  (1UL<<(n))
#define SST_MASK(n) (0xFFFFFFFFL >> (32-(n)))
#define SST_MASK64(n) FX_MASK64(n)

//----------------- SST binary point locations ---------------
#define SST_LOD_SIZE     6
#define SST_LOD_FRACBITS 2
#define SST_XY_SIZE     16
#define SST_XY_INTBITS  12
#define SST_XY_FRACBITS  4
#define SST_RGBA_SIZE   24
#define SST_RGBA_INTBITS 8
#define SST_RGBA_FRACBITS 12
#define SST_Z_SIZE      32
#define SST_Z_INTBITS   16
#define SST_Z_FRACBITS  12
// NOTE: below are ONLY for the fixed point registers
#define SST_ST_SIZE     32
#define SST_ST_INTBITS  14
#define SST_ST_FRACBITS 18
#define SST_W_SIZE      32
#define SST_W_INTBITS    2
#define SST_W_FRACBITS  30

//----------------- SST status bits ---------------------------
#define SST_FIFOLEVEL           0x3F
#define SST_PCIFIFO_FREE        0x1F
#define SST_PCIFIFO_BUSY        BIT(5)
#define SST_VRETRACE            BIT(6)
#define SST_FBI_BUSY            BIT(7)
#define SST_TMU_BUSY            BIT(8)
#define SST_TREX_BUSY           SST_TMU_BUSY
#define SST_BUSY                BIT(9)
#define SST_GUI_BUSY            BIT(10)
#define SST_CMD0_BUSY           BIT(11)
#define SST_CMD1_BUSY           BIT(12)
#define SST_SWAPBUFPENDING_SHIFT 28
#define SST_SWAPBUFPENDING      (0x7L<<SST_SWAPBUFPENDING_SHIFT)
#define SST_PCIINTERRUPTED      BIT(31)

//----------------- SST lfbMode bits --------------------------
#define SST_LFB_FORMAT_SHIFT    0
#define SST_LFB_FORMAT          (0xFL<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_565              (0L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_555              (1L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_1555             (2L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_u1               (3L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_888              (4L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_8888             (5L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_u2               (6L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_u3               (7L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_Z565             (12L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_Z555             (13L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_Z1555            (14L<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_ZZ               (15L<<SST_LFB_FORMAT_SHIFT)
#define SST_LFB_READBUFSELECT_SHIFT     6
#define SST_LFB_READBUFSELECT   (0x3L<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READCOLORBUFFER  (1L<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READDEPTHABUFFER (2L<<SST_LFB_READBUFSELECT_SHIFT)
#   define SST_LFB_READAUXBUFFER    SST_LFB_READDEPTHABUFFER
#define SST_LFB_ENPIXPIPE       BIT(8)
#define SST_LFB_RGBALANES_SHIFT         9
#define SST_LFB_RGBALANES       (0x3L<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_ARGB (0L<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_ABGR (1L<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_RGBA (2L<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_BGRA (3L<<SST_LFB_RGBALANES_SHIFT)
#define SST_LFB_WRITE_SWAP16    BIT(11)
#define SST_LFB_WRITE_BYTESWAP  BIT(12)
#define SST_LFB_YORIGIN         BIT(13)
#define SST_LFB_WSELECT         BIT(14)
#define SST_LFB_READ_SWAP16     BIT(15)
#define SST_LFB_READ_BYTESWAP   BIT(16)

#define SST_LFB_ADDR_STRIDE     2048
#define SST_LFB_ADDR_X_SHIFT    0
#define SST_LFB_ADDR_X          (0x7FFL<<SST_LFB_ADDR_X_SHIFT)
#define SST_LFB_ADDR_Y_SHIFT    11
#define SST_LFB_ADDR_Y          (0x7FFL<<SST_LFB_ADDR_Y_SHIFT)

//----------------- SST videoFilterRgbThreshold
#define SST_VIDEOFILTER_BLUE_SHIFT      0
#define SST_VIDEOFILTER_BLUE    (0xff L<< SST_VIDEOFILTER_BLUE_SHIFT)
#define SST_VIDEOFILTER_GREEN_SHIFT     8
#define SST_VIDEOFILTER_GREEN   (0xff << SST_VIDEOFILTER_GREEN_SHIFT)
#define SST_VIDEOFILTER_RED_SHIFT 16            
#define SST_VIDEOFILTER_RED     (0xff << SST_VIDEOFILTER_RED_SHIFT)

//----------------- SST textureMode bits -----------------------
#define SST_TPERSP_ST           BIT(0)
#define SST_TMINFILTER          BIT(1)
#define SST_TMAGFILTER          BIT(2)
#define SST_TCLAMPW             BIT(3)
#define SST_TLODDITHER          BIT(4)
#define SST_TNCCSELECT          BIT(5)  // selects which table
#define SST_TCLAMPS             BIT(6)
#define SST_TCLAMPT             BIT(7)
#define SST_T8BIT(mode) ((mode&SST_TFORMAT)<SST_ARGB8332)
#define SST_TFORMAT_SHIFT       8
#define SST_TFORMAT             (0xF<<SST_TFORMAT_SHIFT)
#       define SST_RGB332               (0<<SST_TFORMAT_SHIFT)
#       define SST_YIQ422               (1<<SST_TFORMAT_SHIFT)
#       define SST_A8                   (2<<SST_TFORMAT_SHIFT)
#       define SST_I8                   (3<<SST_TFORMAT_SHIFT)
#       define SST_AI44                 (4<<SST_TFORMAT_SHIFT)
#       define SST_P8                   (5<<SST_TFORMAT_SHIFT)
#       define SST_P8_ARGB6666          (6<<SST_TFORMAT_SHIFT)
#       define SST_ARGB8332             (8<<SST_TFORMAT_SHIFT)
#       define SST_AYIQ8422             (9<<SST_TFORMAT_SHIFT)
#       define SST_RGB565               (10<<SST_TFORMAT_SHIFT)
#       define SST_ARGB1555             (11<<SST_TFORMAT_SHIFT)
#       define SST_ARGB4444             (12<<SST_TFORMAT_SHIFT)
#       define SST_AI88                 (13<<SST_TFORMAT_SHIFT)
#       define SST_AP88                 (14<<SST_TFORMAT_SHIFT)

#ifdef SHARK
#       define SST_COMPRESS             (15<<SST_TFORMAT_SHIFT)
#endif 

#define SST_TC_ZERO_OTHER       BIT(12)
#define SST_TC_SUB_CLOCAL       BIT(13)
#define SST_TC_MSELECT_SHIFT    14
#define SST_TC_MSELECT          (0x7<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MONE              (0<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MCLOCAL           (1<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MAOTHER           (2<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MALOCAL           (3<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MLOD              (4<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MLODFRAC          (5<<SST_TC_MSELECT_SHIFT)
#define SST_TC_REVERSE_BLEND    BIT(17)
#define SST_TC_ADD_CLOCAL       BIT(18)
#define SST_TC_ADD_ALOCAL       BIT(19)
#define SST_TC_INVERT_OUTPUT    BIT(20)
#define SST_TCA_ZERO_OTHER      BIT(21)
#define SST_TCA_SUB_CLOCAL      BIT(22)
#define SST_TCA_MSELECT_SHIFT   23
#define SST_TCA_MSELECT         (0x7<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MONE             (0<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MCLOCAL          (1<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MAOTHER          (2<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MALOCAL          (3<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MLOD             (4<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MLODFRAC         (5<<SST_TCA_MSELECT_SHIFT)
#define SST_TCA_REVERSE_BLEND   BIT(26)
#define SST_TCA_ADD_CLOCAL      BIT(27)
#define SST_TCA_ADD_ALOCAL      BIT(28)
#define SST_TCA_INVERT_OUTPUT   BIT(29)
#define SST_TRILINEAR           BIT(30)

// here are some abstract constants that most people will be using
// TC stands for Texture Combine (RGB channels)
// TCA stands for Texture Combine Alpha (just the Alpha channel)
// BLEND_ATT blends on the Alpha channel of the color passed from TMU to TMU
// BLEND_ALOCAL blends on the Alpha channel of the local texture color
// NOTE: and of the BLEND modes can be modified using SST_TC_REVERSE_BLEND
//       and/or SST_TCA_REVERSE_BLEND, these modifiers reverse the "polarity"
//       of the blend
#define SST_TC_MZERO  (SST_TC_MONE  | SST_TC_REVERSE_BLEND)
#define SST_TCA_MZERO (SST_TCA_MONE | SST_TCA_REVERSE_BLEND)
                  
#define SST_TCOMBINE_SHIFT      12       // RGB COMBINE MODES
#define SST_TCOMBINE            (0x1FF<<SST_TCOMBINE_SHIFT)
#       define SST_TC_REPLACE    (SST_TC_ZERO_OTHER | SST_TC_ADD_CLOCAL)
#       define SST_TC_PASS       (SST_TC_MONE)
#       define SST_TC_ADD        (SST_TC_MONE  | SST_TC_ADD_CLOCAL)
#       define SST_TC_SUB        (SST_TC_SUB_CLOCAL | SST_TC_MONE)
#       define SST_TC_MULT       (SST_TC_MCLOCAL | SST_TC_REVERSE_BLEND)
#       define SST_TC_ZERO       (SST_TC_MZERO)
#       define SST_TC_ONE        (SST_TC_MZERO | SST_TC_INVERT_OUTPUT)
#       define SST_TC_BLEND      (SST_TC_SUB_CLOCAL | SST_TC_ADD_CLOCAL)
#       define SST_TC_BLEND_LOD         (SST_TC_BLEND | SST_TC_MLOD)
#       define SST_TC_BLEND_LODFRAC     (SST_TC_BLEND | SST_TC_MLODFRAC)
#       define SST_TC_BLEND_ATT         (SST_TC_BLEND | SST_TC_MAOTHER)
#       define SST_TC_BLEND_ALOCAL      (SST_TC_BLEND | SST_TC_MALOCAL)
#define SST_TACOMBINE_SHIFT     21      // ALPHA COMBINE MODES
#define SST_TACOMBINE           (0x1FF<<SST_TACOMBINE_SHIFT)
#       define SST_TCA_REPLACE   (SST_TCA_ZERO_OTHER | SST_TCA_ADD_CLOCAL)
#       define SST_TCA_PASS      (SST_TCA_MONE)
#       define SST_TCA_ADD       (SST_TCA_MONE  | SST_TCA_ADD_CLOCAL)
#       define SST_TCA_SUB       (SST_TCA_SUB_CLOCAL | SST_TCA_MONE)
#       define SST_TCA_MULT      (SST_TCA_MCLOCAL | SST_TCA_REVERSE_BLEND)
#       define SST_TCA_ONE       (SST_TCA_MZERO | SST_TCA_INVERT_OUTPUT)
#       define SST_TCA_ZERO      (SST_TCA_MZERO)
#       define SST_TCA_BLEND     (SST_TCA_SUB_CLOCAL | SST_TCA_ADD_CLOCAL)
#       define SST_TCA_BLEND_LOD        (SST_TCA_BLEND | SST_TCA_MLOD)
#       define SST_TCA_BLEND_LODFRAC    (SST_TCA_BLEND | SST_TCA_MLODFRAC)
#       define SST_TCA_BLEND_ATT        (SST_TCA_BLEND | SST_TCA_MAOTHER)
#       define SST_TCA_BLEND_ALOCAL     (SST_TCA_BLEND | SST_TCA_MALOCAL)
#define SST_TMU_ACTIVE(texMode) \
        ((texMode & (SST_TCOMBINE|SST_TACOMBINE)) != (SST_TC_PASS|SST_TCA_PASS))
#define SST_TREX_ACTIVE(texMode) SST_TMU_ACTIVE(texMode)

//----------------- SST tLOD bits -----------------------------
#define SST_LODMIN_SHIFT        0       // 4.2 format
#define SST_LODMIN              (0x3F<<SST_LODMIN_SHIFT)
#define SST_LODMAX_SHIFT        6
#define SST_LODMAX              (0x3F<<SST_LODMAX_SHIFT)
#define SST_LODBIAS_SHIFT       12
#define SST_LODBIAS             (0x3F<<SST_LODBIAS_SHIFT)
#define SST_LOD_ODD             BIT(18)
#define SST_LOD_TSPLIT          BIT(19)
#define SST_LOD_S_IS_WIDER      BIT(20)
#define SST_LOD_ASPECT_SHIFT    21
#define SST_LOD_ASPECT          (0x3<<SST_LOD_ASPECT_SHIFT)
#define SST_LOD_ZEROFRAC        BIT(23)
#define SST_TMULTIBASEADDR      BIT(24)
#define SST_TDATA_SWIZZLE       BIT(25)
#define SST_TDATA_SWAP          BIT(26)
#define SST_TDIRECT_WRITE       BIT(27)
#define SST_TMIRRORS            BIT(28)
#define SST_TMIRRORT            BIT(29)

//----------------- SST tDetail bits --------------------------
#define SST_DETAIL_MAX_SHIFT    0       // 8.0 format
#define SST_DETAIL_MAX          (0xFF<<SST_DETAIL_MAX_SHIFT)
#define SST_DETAIL_BIAS_SHIFT   8       // 6.0 format signed
#define SST_DETAIL_BIAS         (0x3F<<SST_DETAIL_BIAS_SHIFT)
#define SST_DETAIL_SCALE_SHIFT  14      // 3.0 format
#define SST_DETAIL_SCALE        (0x7<<SST_DETAIL_SCALE_SHIFT)
#define SST_TMINFILTER_RGB      BIT(17)
#define SST_TMAGFILTER_RGB      BIT(18)
#define SST_TMINFILTER_A        BIT(19)
#define SST_TMAGFILTER_A        BIT(20)
#define SST_TFILTER_SEPARATE    BIT(21)

//----------------- SST fbzMode bits --------------------------
#define SST_ENRECTCLIP          BIT(0)
#define SST_ENCHROMAKEY         BIT(1)
#define SST_ENSTIPPLE           BIT(2)
#define SST_WBUFFER             BIT(3)
#define SST_ENDEPTHBUFFER       BIT(4)
#       define SST_ZFUNC_LT     BIT(5)
#       define SST_ZFUNC_EQ     BIT(6)
#       define SST_ZFUNC_GT     BIT(7)
#       define SST_ZFUNC_SHIFT  5
#       define SST_ZFUNC        ( 0x7 << SST_ZFUNC_SHIFT )
#define SST_ENDITHER            BIT(8)
#define SST_RGBWRMASK           BIT(9)
#define SST_ZAWRMASK            BIT(10)
#define SST_DITHER2x2           BIT(11)
#define SST_ENSTIPPLEPATTERN    BIT(12)
#define SST_ENALPHAMASK         BIT(13)
#define SST_ENZBIAS             BIT(16)
#define SST_YORIGIN             BIT(17)
#define SST_ENALPHABUFFER       BIT(18)
#define SST_ENDITHERSUBTRACT    BIT(19)
#define SST_ABLEND_DITHER_SUB_EN SST_ENDITHERSUBTRACT
#define SST_ENDITHERSUBTRACT    BIT(19)
#define SST_ZCOMPARE_TO_ZACOLOR BIT(20)
#define SST_DEPTH_FLOAT_SEL     BIT(21)

//----------------- SST fbzColorPath bits ---------------------------
#define SST_RGBSELECT_SHIFT     0
#define SST_RGBSELECT           (0x3<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_RGBA          (0<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_TMUOUT        (1<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_TREXOUT       SST_RGBSEL_TMUOUT
#       define SST_RGBSEL_C1            (2<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_LFB           (3<<SST_RGBSELECT_SHIFT)
#define SST_ASELECT_SHIFT       2
#define SST_ASELECT             (0x3<<SST_ASELECT_SHIFT)
#       define SST_ASEL_RGBA            (0<<SST_ASELECT_SHIFT)
#       define SST_ASEL_TMUOUT          (1<<SST_ASELECT_SHIFT)
#       define SST_ASEL_TREXOUT         SST_ASEL_TMUOUT
#       define SST_ASEL_C1              (2<<SST_ASELECT_SHIFT)
#       define SST_ASEL_LFB             (3<<SST_ASELECT_SHIFT)
#define SST_LOCALSELECT_SHIFT   4
#define SST_LOCALSELECT         BIT(4)
#define SST_ALOCALSELECT_SHIFT  5
#define SST_ALOCALSELECT        (0x3<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_ITERATOR             (0<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_C0                   (1<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_Z                    (2<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_W                    (3<<SST_ALOCALSELECT_SHIFT)
#define SST_LOCALSELECT_OVERRIDE_WITH_ATEX      BIT(7)
#define SST_CC_ZERO_OTHER       BIT(8)
#define SST_CC_SUB_CLOCAL       BIT(9)
#define SST_CC_MSELECT_SHIFT    10
#define SST_CC_MSELECT          (0x7<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MONE              (0<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MCLOCAL           (1<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MAOTHER           (2<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MALOCAL           (3<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MATMU             (4<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MATREX            SST_CC_MATMU
#       define SST_CC_MRGBTMU           (5<<SST_CC_MSELECT_SHIFT)
#define SST_CC_REVERSE_BLEND    BIT(13)
#define SST_CC_ADD_CLOCAL       BIT(14)
#define SST_CC_ADD_ALOCAL       BIT(15)
#define SST_CC_INVERT_OUTPUT    BIT(16)
#define SST_CCA_ZERO_OTHER      BIT(17)
#define SST_CCA_SUB_CLOCAL      BIT(18)
#define SST_CCA_MSELECT_SHIFT   19
#define SST_CCA_MSELECT         (0x7<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MONE             (0<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MCLOCAL          (1<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MAOTHER          (2<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MALOCAL          (3<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MATMU            (4<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MATREX           SST_CCA_MATMU
#define SST_CCA_REVERSE_BLEND   BIT(22)
#define SST_CCA_ADD_CLOCAL      BIT(23)
#define SST_CCA_ADD_ALOCAL      BIT(24)
#define SST_CCA_INVERT_OUTPUT   BIT(25)
#define SST_PARMADJUST          BIT(26)
#define SST_ENTEXTUREMAP        BIT(27)
#define SST_RGBAZ_CLAMP         BIT(28)
#define SST_ENANTIALIAS         BIT(29)

// here are some abstract constants that most people will be using
// CC stands for Color Combine (RGB channels)
// CCA stands for Color Combine Alpha (just the Alpha channel)
// BLEND_ATT blends on the Alpha channel of the color passed from TMU to TMU
// BLEND_ALOCAL blends on the Alpha channel of the local texture color
// NOTE: and of the BLEND modes can be modified using SST_CC_REVERSE_BLEND
//       and/or SST_CCA_REVERSE_BLEND, these modifiers reverse the "polarity"
//       of the blend
#define SST_CC_MZERO  (SST_CC_MONE  | SST_CC_REVERSE_BLEND)
#define SST_CCA_MZERO (SST_CCA_MONE | SST_CCA_REVERSE_BLEND)

#define SST_CCOMBINE_SHIFT      8       // RGB COMBINE MODES
#define SST_CCOMBINE            (0x1FF<<SST_CCOMBINE_SHIFT)
#       define SST_CC_REPLACE    (SST_CC_ZERO_OTHER | SST_CC_ADD_CLOCAL)
#       define SST_CC_PASS       (SST_CC_MONE)
#       define SST_CC_ADD        (SST_CC_MONE  | SST_CC_ADD_CLOCAL)
#       define SST_CC_SUB        (SST_CC_SUB_CLOCAL | SST_CC_MONE)
#       define SST_CC_MULT       (SST_CC_MCLOCAL | SST_CC_REVERSE_BLEND)
#       define SST_CC_ZERO       (SST_CC_MZERO)
#       define SST_CC_ONE        (SST_CC_MZERO | SST_CC_INVERT_OUTPUT)
#       define SST_CC_BLEND      (SST_CC_SUB_CLOCAL | SST_CC_ADD_CLOCAL)
#       define SST_CC_BLEND_ATT         (SST_CC_BLEND | SST_CC_MAOTHER)
#       define SST_CC_BLEND_ATEX        (SST_CC_BLEND | SST_CC_MATMU)
#       define SST_CC_BLEND_ATEX_REV    (SST_CC_BLEND | SST_CC_MATMU | SST_CC_REVERSE_BLEND)
#       define SST_CC_BLEND_ATT_REV     (SST_CC_BLEND | SST_CC_MAOTHER | SST_CC_REVERSE_BLEND)
#       define SST_CC_BLEND_ALOCAL      (SST_CC_BLEND | SST_CC_MALOCAL)
#define SST_CACOMBINE_SHIFT     17      // ALPHA COMBINE MODES
#define SST_CACOMBINE           (0x1FF<<SST_CACOMBINE_SHIFT)
#       define SST_CCA_REPLACE   (SST_CCA_ZERO_OTHER | SST_CCA_ADD_CLOCAL)
#       define SST_CCA_PASS      (SST_CCA_MONE)
#       define SST_CCA_ADD       (SST_CCA_MONE  | SST_CCA_ADD_CLOCAL)
#       define SST_CCA_SUB       (SST_CCA_SUB_CLOCAL | SST_CCA_MONE)
#       define SST_CCA_MULT      (SST_CCA_MCLOCAL | SST_CCA_REVERSE_BLEND)
#       define SST_CCA_ONE       (SST_CCA_MZERO | SST_CCA_INVERT_OUTPUT)
#       define SST_CCA_ZERO      (SST_CCA_MZERO)
#       define SST_CCA_BLEND     (SST_CCA_SUB_CLOCAL | SST_CCA_ADD_CLOCAL)
#       define SST_CCA_BLEND_ATT        (SST_CCA_BLEND | SST_CCA_MAOTHER)
#       define SST_CCA_BLEND_ATEX       (SST_CCA_BLEND | SST_CCA_MATMU)
#       define SST_CCA_BLEND_ATEX_REV   (SST_CCA_BLEND | SST_CCA_MATMU | SST_CCA_REVERSE_BLEN)
#       define SST_CCA_BLEND_ATT_REV    (SST_CCA_BLEND | SST_CCA_MAOTHER | SST_CCA_REVERSE_BLEND)
#       define SST_CCA_BLEND_ALOCAL     (SST_CCA_BLEND | SST_CCA_MALOCAL)

//----------------- SST fogMode bits ---------------------------
#define SST_ENFOGGING           BIT(0)  // Afog*Cfog + (1-Afog)*Color
#define SST_FOGADD              BIT(1)  // generates (1-Afog)*Color
#define SST_FOGMULT             BIT(2)  // generates Afog*Cfog
#define SST_FOG_ALPHA           BIT(3)  // use Alpha iterator to control fog
#define SST_FOG_Z               BIT(4)  // use Z iterator to control fog
#define SST_FOG_CONSTANT        BIT(5)  // simply add Cfog to color and clamp
#define SST_FOG_DITHER          BIT(6)  // dither the Afog
#define SST_FOG_ZONES           BIT(7)  // enable signed fog deltas

//----------------- SST alphaMode bits ------------------------
#define SST_ENALPHAFUNC         BIT(0)
#define SST_ALPHAFNC_SHIFT      1
#define SST_ALPHAFUNC_SHIFT     1
#       define SST_ALPHAFUNC_LT BIT(1)
#       define SST_ALPHAFUNC_EQ BIT(2)
#       define SST_ALPHAFUNC_GT BIT(3)
#   define SST_ALPHAFNC     ( 0x7 << SST_ALPHAFNC_SHIFT )
#   define SST_ALPHAFUNC    ( 0x7 << SST_ALPHAFUNC_SHIFT )
#define SST_ENALPHABLEND        BIT(4)
        // unused BIT(5)
#define SST_RGBSRCFACT_SHIFT    8       // RGB source factor
#define SST_RGBSRCFACT          (0xF<<SST_RGBSRCFACT_SHIFT)
#       define SST_A_ZERO       0
#       define SST_A_SRCALPHA   1
#       define SST_A_COLOR      2
#       define SST_A_DSTALPHA   3
#       define SST_A_ONE        4
#       define SST_AOM_SRCALPHA 5
#       define SST_AOM_COLOR    6
#       define SST_AOM_DSTALPHA 7
#       define SST_A_SATURATE   0xF     // for SRC factor only
#       define SST_A_COLORBEFOREFOG 0xF // for DST factor only
#define SST_RGBDSTFACT_SHIFT    12      // RGB destination factor
#define SST_RGBDSTFACT          (0xF<<SST_RGBDSTFACT_SHIFT)
#define SST_ASRCFACT_SHIFT      16      // A source factor
#define SST_ASRCFACT            (0xF<<SST_ASRCFACT_SHIFT)
#define SST_ADSTFACT_SHIFT      20      // A destination factor
#define SST_ADSTFACT            (0xF<<SST_ADSTFACT_SHIFT)
#define SST_ALPHAREF_SHIFT      24
#define SST_ALPHAREF            (0xFF<<SST_ALPHAREF_SHIFT)

//---------------- SST zaColor bits ----------------------
#define SST_ZACOLOR_DEPTH_SHIFT   0
#define SST_ZACOLOR_DEPTH         ( 0xFFFF << SST_ZACOLOR_DEPTH_SHIFT )
#define SST_ZACOLOR_ALPHA_SHIFT   24
#define SST_ZACOLOR_ALPHA         ( 0xFF << SST_ZACOLOR_ALPHA_SHIFT )

//----------------- SST chromaRange bits ------------------------
#define SST_CHROMARANGE_BLUE_EX  BIT(24)// Blue value in exclusive mode
#define SST_CHROMARANGE_GREEN_EX BIT(25)// Green value in exclusive mode
#define SST_CHROMARANGE_RED_EX   BIT(26)// Red  value in exclusive mode
#define SST_CHROMARANGE_BLOCK_OR BIT(27)// Union of all colors.
#define SST_ENCHROMARANGE        BIT(28)
#define SST_ENCHROMAKEY_TMU      BIT(29)        // only for TMU
#define SST_ENCOLORSUBSTITUTION  BIT(30)        // only for TMU

//----------------- SST clip bits ------------------------
//clipLeftRight
#define SST_CLIPLEFT_SHIFT        16
#define SST_CLIPRIGHT_SHIFT       0
#define SST_CLIPLEFT              (0xFFF<<SST_CLIPLEFT_SHIFT)
#define SST_CLIPRIGHT             (0xFFF<<SST_CLIPRIGHT_SHIFT)
//clipBottomTop
#define SST_CLIPBOTTOM_SHIFT      16
#define SST_CLIPTOP_SHIFT         0
#define SST_CLIPBOTTOM            (0xFFF<<SST_CLIPBOTTOM_SHIFT)
#define SST_CLIPTOP               (0xFFF<<SST_CLIPTOP_SHIFT)

#define SST_CLIPLOWY_SHIFT        SST_CLIPBOTTOM_SHIFT
#define SST_CLIPHIGHY_SHIFT       SST_CLIPTOP_SHIFT
#define SST_CLIPLOWY              SST_CLIPBOTTOM
#define SST_CLIPHIGHY             SST_CLIPTOP
// clipLeftRight1, clipBottomTop1
#define SST_ENRECTCLIP1         BIT(31)
#define SST_RECTCLIP1_EX        BIT(31)

//----------------- SST sSetupMode bits ------------------------
#define SST_SETUP_RGB           BIT(0)
#define SST_SETUP_A             BIT(1)
#define SST_SETUP_Z             BIT(2)
#define SST_SETUP_Wfbi          BIT(3)
#define SST_SETUP_W0            BIT(4)
#define SST_SETUP_ST0           BIT(5)
#define SST_SETUP_W1            BIT(6)
#define SST_SETUP_ST1           BIT(7)
#define SST_SETUP_FAN           BIT(16)
#define SST_SETUP_EN_CULLING    BIT(17)
#define SST_SETUP_CULL_NEGATIVE BIT(18)
#define SST_SETUP_DIS_PINGPONG  BIT(19)

//-------------- SST col/aux buffer addressing ---------------
// aux/colBufferAddr
#define SST_BUFFER_BASE_SHIFT    (0)
#define SST_BUFFER_BASE_ADDR     (0xFFFFFF<<SST_BUFFER_BASE_SHIFT)
#define SST_BUFFER_BASE_T_SHIFT  (12)
#define SST_BUFFER_BASE_T        (0xFFF<<SST_BUFFER_BASE_T_SHIFT)
#define SST_BUFFER_BASE_Y_SHIFT  (7)
#define SST_BUFFER_BASE_Y        (0x1F<<SST_BUFFER_BASE_Y_SHIFT)
#define SST_BUFFER_BASE_X_SHIFT  (0)
#define SST_BUFFER_BASE_X        (0x7F<<SST_BUFFER_BASE_X_SHIFT)

// aux/colBufferStride
#define SST_BUFFER_STRIDE_SHIFT  (0)
#define SST_BUFFER_LINEAR_STRIDE (0x3FFF<<SST_BUFFER_STRIDE_SHIFT)
#define SST_BUFFER_TILE_STRIDE   (0x7F<<SST_BUFFER_STRIDE_SHIFT)
#define SST_BUFFER_MEMORY_TYPE   BIT(15)
#define SST_BUFFER_MEMORY_TILED  BIT(15)
#define SST_BUFFER_MEMORY_LINEAR (0)

// tile dimensions
#define SST_TILE_WIDTH_BITS      (7L)
#define SST_TILE_WIDTH           (1L<<(SST_TILE_WIDTH_BITS))
#define SST_TILE_WIDTH_MASK      SST_MASK(SST_TILE_WIDTH_BITS)
#define SST_TILE_HEIGHT_BITS     (5L)
#define SST_TILE_HEIGHT          (1L<<(SST_TILE_HEIGHT_BITS))
#define SST_TILE_HEIGHT_MASK     SST_MASK(SST_TILE_HEIGHT_BITS)
#define SST_TILE_SIZE            (SST_TILE_WIDTH * SST_TILE_HEIGHT)

//------------------- SST Planar YUV addressing --------------------
#define SST_YUV_STRIDE_SHIFT  (0)
#define SST_YUV_LINEAR_STRIDE (0x1FFF<<SST_YUV_STRIDE_SHIFT)
#define SST_YUV_TILE_STRIDE   (0x7F<<SST_YUV_STRIDE_SHIFT)
#define SST_YUV_MEMORY_TYPE   BIT(31)
#define SST_YUV_MEMORY_TILED  BIT(31)
#define SST_YUV_MEMORY_LINEAR (0)

#define SST_YUV_BASE_ADDR_SHIFT 0
#define SST_YUV_BASE_ADDR       (0x01FFFFFF<<SST_YUV_BASE_ADDR_SHIFT)

#define SST_YUV_ADDR            0xFFFFF
#define SST_YUV_ADDR_STRIDE     1024
#define SST_YUV_ADDR_X_SHIFT    0
#define SST_YUV_ADDR_X          (0x3FF<<SST_YUV_ADDR_X_SHIFT)
#define SST_YUV_ADDR_Y_SHIFT    10
#define SST_YUV_ADDR_Y          (0x3FF<<SST_YUV_ADDR_Y_SHIFT)

//----------------- SST texBaseAddr bits ---------------------------
#define SST_TEXTURE_ADDRESS     (SST_MASK(20)<<4)
#define SST_TEXTURE_FULL_ADDRESS        SST_MASK(24)
#define SST_TEXTURE_TILESTRIDE_SHIFT    25
#define SST_TEXTURE_TILESTRIDE  (0x7F<<SST_TEXTURE_TILESTRIDE_SHIFT)
#define SST_TEXTURE_IS_TILED    BIT(0)

#define SST_TEXTURE_LOD8_SHIFT  16
#define SST_TEXTURE_LOD8        (0xF<<SST_TEXTURE_LOD8_SHIFT)
#define SST_TEXTURE_T8_SHIFT    8
#define SST_TEXTURE_T8          (0xFF<<SST_TEXTURE_T8_SHIFT)
#define SST_TEXTURE_S8_SHIFT    0
#define SST_TEXTURE_S8          (0xFF<<SST_TEXTURE_S8_SHIFT)

#define SST_TEXTURE_LOD16_SHIFT  17
#define SST_TEXTURE_LOD16        (0xF<<SST_TEXTURE_LOD16_SHIFT)
#define SST_TEXTURE_T16_SHIFT    9
#define SST_TEXTURE_T16          (0xFF<<SST_TEXTURE_T16_SHIFT)
#define SST_TEXTURE_S16_SHIFT    1
#define SST_TEXTURE_S16          (0xFF<<SST_TEXTURE_S16_SHIFT)

//-------------------- PLL Registers --------------------
//-------------------- pllCtrl0,1,2
#define SST_PLL_K_SHIFT				0
#define SST_PLL_K				(0x3<<SST_PLL_K_SHIFT)
#define SST_PLL_M_SHIFT				2
#define SST_PLL_M				(0x3F<<SST_PLL_M_SHIFT)
#define SST_PLL_N_SHIFT				8
#define SST_PLL_N				(0xFF<<SST_PLL_N_SHIFT)
#define SST_PLL_TEST				BIT(16)

//-------------------- DAC Registers --------------------
//-------------------- dacMode
#define SST_DAC_MODE_2X				BIT(0)
#define SST_DAC_DPMS_ON_VSYNC			BIT(1)
#define SST_DAC_FORCE_VSYNC			BIT(2)
#define SST_DAC_DPMS_ON_HSYNC			BIT(3)
#define SST_DAC_FORCE_HSYNC			BIT(4)

//-------------------- dacAddr
#define SST_DAC_ADDR_SHIFT			0
#define SST_DAC_ADDR				(0xFF<<SST_DAC_ADDR_SHIFT)

//-------------------- dacData
#define SST_DAC_DATA_BLUE_SHIFT 		0
#define SST_DAC_DATA_BLUE			(0xFF<<SST_DAC_DATA_BLUE_SHIFT)
#define SST_DAC_DATA_GREEN_SHIFT 		8
#define SST_DAC_DATA_GREEN			(0xFF<<SST_DAC_DATA_GREEN_SHIFT)
#define SST_DAC_DATA_RED_SHIFT 			16
#define SST_DAC_DATA_RED			(0xFF<<SST_DAC_DATA_RED_SHIFT)

//------------------- SST Video ----------------------------
//------------------- vidProcCfg
#define SST_VIDEO_PROCESSOR_EN                BIT(0)
#define SST_CURSOR_MODE_SHIFT                 1
#define SST_CURSOR_MODE                       (1<<SST_CURSOR_MODE_SHIFT)
#       define SST_CURSOR_X11                 (1<<SST_CURSOR_MODE_SHIFT)
#       define SST_CURSOR_MICROSOFT           (0<<SST_CURSOR_MODE_SHIFT)
#define SST_OVERLAY_STEREO_EN                 BIT(2)
#define SST_INTERLACED_EN                     BIT(3)
#define SST_HALF_MODE                         BIT(4)
#define SST_CHROMA_EN                         BIT(5)
#define SST_CHROMA_INVERT                     BIT(6)
#define SST_DESKTOP_EN                        BIT(7)
#define SST_OVERLAY_EN                        BIT(8)
#define SST_VIDEOIN_AS_OVERLAY                BIT(9)
#define SST_DESKTOP_CLUT_BYPASS               BIT(10)
#define SST_OVERLAY_CLUT_BYPASS               BIT(11)
#define SST_DESKTOP_CLUT_SELECT               BIT(12)
#define SST_OVERLAY_CLUT_SELECT               BIT(13)
#define SST_OVERLAY_HORIZ_SCALE_EN            BIT(14)
#define SST_OVERLAY_VERT_SCALE_EN             BIT(15)
#define SST_OVERLAY_FILTER_MODE_SHIFT         16
#define SST_OVERLAY_FILTER_MODE               (3L<<SST_OVERLAY_FILTER_MODE_SHIFT)
#       define SST_OVERLAY_FILTER_POINT       (0L<<SST_OVERLAY_FILTER_MODE_SHIFT)    
#       define SST_OVERLAY_FILTER_2X2         (1L<<SST_OVERLAY_FILTER_MODE_SHIFT)
#       define SST_OVERLAY_FILTER_4X4         (2L<<SST_OVERLAY_FILTER_MODE_SHIFT)
#       define SST_OVERLAY_FILTER_BILINEAR    (3L<<SST_OVERLAY_FILTER_MODE_SHIFT)
#define SST_DESKTOP_PIXEL_FORMAT_SHIFT        18
#define SST_DESKTOP_PIXEL_FORMAT              (7L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#       define SST_DESKTOP_PIXEL_PAL8         (0L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#       define SST_DESKTOP_PIXEL_RGB565       (1L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#       define SST_DESKTOP_PIXEL_RGB24        (2L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#       define SST_DESKTOP_PIXEL_RGB32        (3L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#define SST_OVERLAY_PIXEL_FORMAT_SHIFT        21
#define SST_OVERLAY_PIXEL_FORMAT              (7L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_RGB565U      (1L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_YUV411       (4L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_YUYV422      (5L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_UYVY422      (6L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_RGB565D      (7L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#define SST_DESKTOP_TILED_EN                  BIT(24)
#define SST_OVERLAY_TILED_EN                  BIT(25)
#define SST_VIDEO_2X_MODE_EN                  BIT(26)
#define SST_CURSOR_EN                         BIT(27)
#define SST_OVERLAY_EACH_VSYNC                BIT(29)
#define SST_OVERLAY_STRIDE_ADJUST             BIT(30)
#define SST_OVERLAY_DEINTERLACE_EN            BIT(31)

//-------------------- hwCurLoc
#define SST_CURSOR_X_SHIFT                    0
#define SST_CURSOR_X                          (0x7FF<<SST_CURSOR_X_SHIFT)
#define SST_CURSOR_Y_SHIFT                    16
#define SST_CURSOR_Y                          (0x7FF<<SST_CURSOR_Y_SHIFT)

//-------------------- hwCurC0, hwCurC1
#define SST_CURSOR_BLUE_SHIFT                 0
#define SST_CURSOR_BLUE                       (0xFF<<SST_CURSOR_BLUE_SHIFT)
#define SST_CURSOR_RED_SHIFT                  8
#define SST_CURSOR_RED                        (0xFF<<SST_CURSOR_RED_SHIFT)
#define SST_CURSOR_GREEN_SHIFT                16
#define SST_CURSOR_GREEN                      (0xFF<<SST_CURSOR_GREEN_SHIFT)

//-------------------- vidInFormat
#define SST_VIDEOIN_PIXEL_FORMAT_SHIFT        1
#define SST_VIDEOIN_PIXEL_FORMAT              (7<<SST_VIDEOIN_PIXEL_FORMAT_SHIFT)
#       define SST_VIDEOIN_PIXEL_YUV411       (4<<SST_VIDEOIN_PIXEL_FORMAT_SHIFT)
#       define SST_VIDEOIN_PIXEL_YUYV422      (5<<SST_VIDEOIN_PIXEL_FORMAT_SHIFT)
#       define SST_VIDEOIN_PIXEL_UYUV422      (6<<SST_VIDEOIN_PIXEL_FORMAT_SHIFT)
#define SST_VIDEOIN_DEINTERLACE_EN            BIT(4)
#define SST_VIDEOIN_VSYNC_POLARITY_LOW        BIT(5)
#define SST_VIDEOIN_HSYNC_POLARITY_LOW        BIT(6)
#define SST_VIDEOIN_VACTIVE_POLARITY_LOW      BIT(7)
#define SST_VIDEOIN_G4_FOR_POSEDGE            BIT(8)
#define SST_VIDEOIN_BUFFERING_MODE_SHIFT      9
#define SST_VIDEOIN_BUFFERING_MODE            (3<<SST_VIDEOIN_BUFFERING_MODE_SHIFT)
#       define SST_VIDEOIN_SINGLE_BUFFERING   (0<<SST_VIDEOIN_BUFFERING_MODE_SHIFT)
#       define SST_VIDEOIN_DOUBLE_BUFFERING   (1<<SST_VIDEOIN_BUFFERING_MODE_SHIFT)
#       define SST_VIDEOIN_TRIPLE_BUFFERING   (2<<SST_VIDEOIN_BUFFERING_MODE_SHIFT)
#define SST_VIDEOIN_TILED_EN                  BIT(11)
#ifdef H4
#define SST_TVOUT_VSYNC_POLARITY_LOW	      BIT(12)
#define SST_TVOUT_HSYNC_POLARITY_LOW	      BIT(13)
#endif
#define SST_VIDEOIN_INTERFACE_SHIFT           14
#define SST_VIDEOIN_INTERFACE                 (3<<SST_VIDEOIN_INTERFACE_SHIFT)
#       define SST_VIDEOIN_INTERFACE_OFF      (0<<SST_VIDEOIN_INTERFACE_SHIFT)
#       define SST_VIDEOIN_VMI_ENABLE         (1<<SST_VIDEOIN_INTERFACE_SHIFT)
#       define SST_VIDEOIN_TVOUT_ENABLE       (2<<SST_VIDEOIN_INTERFACE_SHIFT)
#define SST_VIDEOIN_GENLOCK_ENABLE            BIT(16)
#define SST_VIDEOIN_NOT_USE_VGA_TIMING        BIT(17)
#ifdef H4
#define SST_VIDEOIN_GENLOCK_SOURCE_SHIFT      18
#define SST_VIDEOIN_GENLOCK_SOURCE            (1<<SST_VIDEOIN_GENLOCK_SOURCE_SHIFT)
#define SST_VIDEOIN_GENLOCK_SOURCE_TV         (1<<SST_VIDEOIN_GENLOCK_SOURCE_SHIFT)
#define SST_VIDEOIN_GENLOCK_SOURCE_VMI        (0<<SST_VIDEOIN_GENLOCK_SOURCE_SHIFT)
#endif
#define SST_VIDEOIN_DATA_CAPTURE_EN           BIT(19)
#define SST_VIDEOIN_HORIZ_DECIM_EN            BIT(20)
#define SST_VIDEOIN_VERT_DECIM_EN             BIT(21)

//-------------------- vidInStatus
#define SST_VIDEOIN_LAST_FIELD                BIT(1)
#define SST_VIDEOIN_LAST_BUFFER_SHIFT         1
#define SST_VIDEOIN_LAST_BUFFER               (3<<SST_VIDEOIN_LAST_BUFFER_SHIFT)

//-------------------- vidSerialParallelPort
#define SST_SERPAR_VMI_EN			BIT(0)
#define SST_SERPAR_VMI_CS_N			BIT(1)
#define SST_SERPAR_VMI_DS_N			BIT(2)
#define SST_SERPAR_VMI_RD_N			BIT(2)
#define SST_SERPAR_VMI_RW_N			BIT(3)
#define SST_SERPAR_VMI_DTACK_N			BIT(4)
#define SST_SERPAR_VMI_RDY			BIT(4)
#define SST_SERPAR_VMI_DATA_OE_N		BIT(5)
#define SST_SERPAR_VMI_DATA_SHIFT		6
#define SST_SERPAR_VMI_DATA			(0xFF<<SST_SERPAR_VMI_DATA_SHIFT)
#define SST_SERPAR_VMI_ADDR_SHIFT		14
#define SST_SERPAR_VMI_ADDR			(0x7<<SST_SERPAR_VMI_ADDR_SHIFT)
#define SST_SERPAR_TVOUT_GPIO_0			BIT(2)
#define SST_SERPAR_TVOUT_GPIO_0_OE_N		BIT(3)
#define SST_SERPAR_TVOUT_GPIO_2			BIT(14)
#define SST_SERPAR_TVOUT_GPIO_2_OE_N		BIT(15)
#define SST_SERPAR_TVOUT_GPIO_1			BIT(16)
#define SST_SERPAR_TVOUT_GPIO_1_OE_N		BIT(17)
#define SST_SERPAR_DDC_EN			BIT(18)
#define SST_SERPAR_DDC_DCK_OUT			BIT(19)
#define SST_SERPAR_DDC_DDA_OUT			BIT(20)
#define SST_SERPAR_DDC_DCK_IN			BIT(21)
#define SST_SERPAR_DDC_DDA_IN			BIT(22)
#define SST_SERPAR_I2C_EN			BIT(23)
#define SST_SERPAR_I2C_SCK_OUT			BIT(24)
#define SST_SERPAR_I2C_DSA_OUT			BIT(25)
#define SST_SERPAR_I2C_SCK_IN			BIT(26)
#define SST_SERPAR_I2C_DSA_IN			BIT(27)
#define SST_SERPAR_VMI_RESET_N			BIT(28)
#define SST_SERPAR_RESET_N			BIT(28)
#define SST_SERPAR_GPIO_1			BIT(29)
#define SST_SERPAR_GPIO_2			BIT(30)

//-------------------- vidInXDecimDeltas, vidInYDecimDeltas
#define SST_VIDEOIN_DECIM_POS_DELTA_SHIFT     0
#define SST_VIDEOIN_DECIM_POS_DELTA           (0xFFF<<SST_VIDEOIN_DECIM_POS_DELTA_SHIFT)
#define SST_VIDEOIN_DECIM_NEG_DELTA_SHIFT     16
#define SST_VIDEOIN_DECIM_NEG_DELTA           (0xFFF<<SST_VIDEOIN_DECIM_NEG_DELTA_SHIFT)

//-------------------- vidInDecimInitErrs
#define SST_VIDEOIN_X_DECIM_ERROR_SHIFT       0
#define SST_VIDEOIN_X_DECIM_ERROR             (0x1FFF<<SST_VIDEOIN_X_DECIM_ERROR_SHIFT)
#define SST_VIDEOIN_Y_DECIM_ERROR_SHIFT       16
#define SST_VIDEOIN_Y_DECIM_ERROR             (0x1FFF<<SST_VIDEOIN_Y_DECIM_ERROR_SHIFT)

//-------------------- vidPixelBufThold
#define SST_PRIMARY_PIXBUF_LOW_WATERMARK_SHIFT 0
#define SST_PRIMARY_PIXBUF_LOW_WATERMARK      (0x3F<<SST_PRIMARY_PIXBUF_LOW_WATERMARK_SHIFT)
#define SST_SECOND0_PIXBUF_LOW_WATERMARK_SHIFT 6
#define SST_SECOND0_PIXBUF_LOW_WATERMARK      (0x3F<<SST_SECOND0_PIXBUF_LOW_WATERMARK_SHIFT)
#define SST_SECOND1_PIXBUF_LOW_WATERMARK_SHIFT 12
#define SST_SECOND1_PIXBUF_LOW_WATERMARK      (0x3F<<SST_SECOND1_PIXBUF_LOW_WATERMARK_SHIFT)

//-------------------- vidChromaKeyMin/Max
#define SST_VIDEO_CHROMA_8BPP_INDEX_SHIFT	0
#define SST_VIDEO_CHROMA_8BPP_INDEX		(0xFF<<SST_VIDEO_CHROMA_8BPP_SHIFT)

#define SST_VIDEO_CHROMA_15BPP_BLUE_SHIFT	0
#define SST_VIDEO_CHROMA_15BPP_BLUE		(0x1F<<SST_VIDEO_CHROMA_15BPP_BLUE_SHIFT)
#define SST_VIDEO_CHROMA_15BPP_GREEN_SHIFT	5
#define SST_VIDEO_CHROMA_15BPP_GREEN		(0x1F<<SST_VIDEO_CHROMA_15BPP_GREEN_SHIFT)
#define SST_VIDEO_CHROMA_15BPP_RED_SHIFT	10
#define SST_VIDEO_CHROMA_15BPP_RED		(0x1F<<SST_VIDEO_CHROMA_15BPP_RED_SHIFT)

#define SST_VIDEO_CHROMA_16BPP_BLUE_SHIFT	0
#define SST_VIDEO_CHROMA_16BPP_BLUE		(0x1F<<SST_VIDEO_CHROMA_16BPP_BLUE_SHIFT)
#define SST_VIDEO_CHROMA_16BPP_GREEN_SHIFT	5
#define SST_VIDEO_CHROMA_16BPP_GREEN		(0x3F<<SST_VIDEO_CHROMA_16BPP_GREEN_SHIFT)
#define SST_VIDEO_CHROMA_16BPP_RED_SHIFT	11
#define SST_VIDEO_CHROMA_16BPP_RED		(0x1F<<SST_VIDEO_CHROMA_16BPP_RED_SHIFT)

#define SST_VIDEO_CHROMA_24BPP_BLUE_SHIFT	0
#define SST_VIDEO_CHROMA_24BPP_BLUE		(0xFF<<SST_VIDEO_CHROMA_24BPP_BLUE_SHIFT)
#define SST_VIDEO_CHROMA_24BPP_GREEN_SHIFT	8
#define SST_VIDEO_CHROMA_24BPP_GREEN		(0xFF<<SST_VIDEO_CHROMA_24BPP_GREEN_SHIFT)
#define SST_VIDEO_CHROMA_24BPP_RED_SHIFT	16
#define SST_VIDEO_CHROMA_24BPP_RED		(0xFF<<SST_VIDEO_CHROMA_24BPP_RED_SHIFT)

#define SST_VIDEO_CHROMA_32BPP_BLUE_SHIFT	0
#define SST_VIDEO_CHROMA_32BPP_BLUE		(0xFF<<SST_VIDEO_CHROMA_32BPP_BLUE_SHIFT)
#define SST_VIDEO_CHROMA_32BPP_GREEN_SHIFT	8
#define SST_VIDEO_CHROMA_32BPP_GREEN		(0xFF<<SST_VIDEO_CHROMA_32BPP_GREEN_SHIFT)
#define SST_VIDEO_CHROMA_32BPP_RED_SHIFT	16
#define SST_VIDEO_CHROMA_32BPP_RED		(0xFF<<SST_VIDEO_CHROMA_32BPP_RED_SHIFT)

//-------------------- vidCurrentLine
#define SST_VIDEO_CURRENT_LINE_SHIFT		0
#define SST_VIDEO_CURRENT_LINE			(0x7FF<<SST_VIDEO_CURRENT_LINE_SHIFT)

//-------------vidScreenSize
 
#define SST_VIDEO_SCREEN_WIDTH_SHIFT          0
#define SST_VIDEO_SCREEN_WIDTH                (0xFFF<<SST_VIDEO_SCREEN_WIDTH_SHIFT)
#define SST_VIDEO_SCREEN_HEIGHT_SHIFT         12
#define SST_VIDEO_SCREEN_HEIGHT               (0xFFF<<SST_VIDEO_SCREEN_HEIGHT_SHIFT)

//-------------vidOverlayStartCoords, vidOverlayEndCoords
#define SST_OVERLAY_X_SHIFT                   0
#define SST_OVERLAY_X                         (0xFFF<<SST_OVERLAY_X_SHIFT)
#define SST_OVERLAY_Y_SHIFT                   12
#define SST_OVERLAY_Y                         (0xFFF<<SST_OVERLAY_Y_SHIFT)
#define SST_OVERLAY_XADJ_SHIFT                24
#define SST_OVERLAY_XADJ                      (3<<SST_OVERLAY_XADJ_SHIFT)
#define SST_OVERLAY_YADJ_SHIFT                26
#define SST_OVERLAY_YADJ                      (3<<SST_OVERLAY_YADJ_SHIFT)

//------------vidOverlayDudxOffsetSrcWidth, vidOverlayDvDy
#define SST_OVERLAY_DELTA                     SST_MASK(20)
#define SST_OVERLAY_DELTA_OFFSET              SST_MASK(19)
#define SST_OVERLAY_FETCH_SIZE_SHIFT          19
#define SST_OVERLAY_FETCH_SIZE                (0x1FFF<<SST_OVERLAY_FETCH_SIZE_SHIFT)

#define SST_VIDEO_START_ADDR_SHIFT            0
#define SST_VIDEO_START_ADDR                  (0xFFFFFF<<SST_VIDEO_START_ADDR_SHIFT)
#define SST_DESKTOP_STRIDE_SHIFT              0
#define SST_DESKTOP_LINEAR_STRIDE             (0x3FFF<<SST_DESKTOP_STRIDE_SHIFT)
#define SST_DESKTOP_TILE_STRIDE               (0x7F<<SST_DESKTOP_STRIDE_SHIFT)
#define SST_OVERLAY_STRIDE_SHIFT              16
#define SST_OVERLAY_LINEAR_STRIDE             (0x3FFF<<SST_OVERLAY_STRIDE_SHIFT)
#define SST_OVERLAY_TILE_STRIDE               (0x7F<<SST_OVERLAY_STRIDE_SHIFT)

//------------vidInStride
#define SST_VIDEO_IN_STRIDE_SHIFT             0
#define SST_VIDEO_IN_LINEAR_STRIDE            (0x3FFF<<SST_VIDEO_IN_STRIDE_SHIFT)
#define SST_VIDEO_IN_TILE_STRIDE              (0x7F<<SST_VIDEO_IN_STRIDE_SHIFT)

//----------------- SST *Init*  ----------------------------

// pciInit0
#define SST_PCI_STALL_ENABLE            BIT(0)
#define SST_PCI_LOWTHRESH_SHIFT         2
#define SST_PCI_LOWTHRESH               (0xF << SST_PCI_LOWTHRESH_SHIFT)
#define SST_PCI_HARDCODE_BASE           BIT(7)
#define SST_PCI_READ_WS                 BIT(8)
#define SST_PCI_WRITE_WS                BIT(9)
#define SST_PCI_DISABLE_IO              BIT(11)
#define SST_PCI_DISABLE_MEM             BIT(12)
#define SST_PCI_RETRY_INTERVAL_SHIFT    13
#define SST_PCI_RETRY_INTERVAL          (0x1F << SST_PCI_RETRY_INTERVAL_SHIFT)
#define SST_PCI_INTERRUPT_ENABLE        BIT(18)
#define SST_PCI_TIMEOUT_ENABLE          BIT(19)

// sipMonitor

#define SST_SIPROCESS_OSC_CNTR             0xFFFF
#define SST_SIPROCESS_PCI_CNTR_SHIFT       16
#define SST_SIPROCESS_PCI_CNTR             (0xFFF<<SST_SIPROCESS_PCI_CNTR_SHIFT)
#define SST_SIPROCESS_OSC_CNTR_RESET_N     0
#define SST_SIPROCESS_OSC_CNTR_RUN         BIT(28)
#define SST_SIPROCESS_OSC_NAND_SEL         0
#define SST_SIPROCESS_OSC_NOR_SEL          BIT(29)
#define SST_SIPROCESS_OSC_FORCE_ENABLE     BIT(30)

// lfbMemoryConfig
#define SST_RAW_LFB_TILE_BEGIN_PAGE_SHIFT       0
#define SST_RAW_LFB_TILE_BEGIN_PAGE             (0x1FFF<<SST_RAW_LFB_TILE_BEGIN_PAGE_SHIFT)
#define SST_RAW_LFB_ADDR_STRIDE_SHIFT           13
#define SST_RAW_LFB_ADDR_STRIDE                 (0x7<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_1K       (0x0<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_2K       (0x1<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_4K       (0x2<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_8K       (0x3<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_16K      (0x4<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_MAX      (0x4<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#define SST_RAW_LFB_TILE_STRIDE_SHIFT           16
#define SST_RAW_LFB_TILE_STRIDE                 (0x7F<<SST_RAW_LFB_TILE_STRIDE_SHIFT)

#define SST_RAW_LFB_ADDR_SHIFT                  0
#define SST_RAW_LFB_ADDR                        (0x1FFFFFF<<SST_RAW_LFB_ADDR_SHIFT)

// miscInit0
#define SST_GRX_RESET                   BIT(0)
#define SST_FBI_FIFO_RESET              BIT(1)
#define SST_REGISTER_BYTE_SWIZZLE_EN    BIT(2)
#define SST_REGISTER_WORD_SWIZZLE_EN    BIT(3)
#define SST_VIDEO_RESET                 BIT(4)
#define SST_2D_RESET                    BIT(5)
#define SST_MEMORY_TIMING_RESET         BIT(6)
#define SST_VGA_TIMING_RESET         	BIT(7)
#define SST_TVOUT_BLANK_DELAY_SHIFT	8
#define SST_TVOUT_BLANK_DELAY    	(0x7<<SST_TVOUT_BLANK_DELAY_SHIFT)
#define SST_TVOUT_SYNC_DELAY_SHIFT	11
#define SST_TVOUT_SYNC_DELAY    	(0x7<<SST_TVOUT_SYNC_DELAY_SHIFT)
#define SST_MONITOR_SYNC_DELAY_SHIFT	14
#define SST_MONITOR_SYNC_DELAY    	(0x7<<SST_MONITOR_SYNC_DELAY_SHIFT)
#define SST_YORIGIN_TOP_SHIFT           (18)
#define SST_YORIGIN_TOP                 (0xFFF<<SST_YORIGIN_TOP_SHIFT)
#define SST_RAWLFB_BYTE_SWIZZLE_EN      BIT(30)
#define SST_RAWLFB_WORD_SWIZZLE_EN      BIT(31)

// miscInit1
#define SST_INVERT_CLUT_ADDRESS         BIT(0)
#define SST_TRI_MODE_SHIFT		1
#define SST_TRI_MODE			(0x3<<SST_TRI_MODE_SHIFT)
#define SST_WRITE_SUBSYSTEM             BIT(3)
#define SST_ENABLE_ROM_WRITES		BIT(4)
#define SST_ALT_REGMAPPING              BIT(5)
#define SST_DISABLE_TEXTURE             BIT(6)
#define SST_POWERDOWN_CLUT              BIT(7)
#define SST_POWERDOWN_DAC               BIT(8)
#define SST_POWERDOWN_VID_PLL           BIT(9)
#define SST_POWERDOWN_GRX_PLL           BIT(10)
#define SST_POWERDOWN_MEM_PLL           BIT(11)
#define SST_BLOCK_WRITE_THRESH_SHIFT    12
#define SST_BLOCK_WRITE_THRESH          (0x7<<SST_BLOCK_WRITE_THRESH_SHIFT)
#define SST_DISABLE_2D_BLOCK_WRITE      BIT(15)
#define SST_DISABLE_2D_STALL_ON_3D      BIT(16)
#define SST_DISABLE_3D_STALL_ON_2D      BIT(17)
#define SST_CMDSTREAM_RESET             BIT(19)
#define SST_PCI_FAST_DEVICE		BIT(24)
#define SST_PCI_BIOS_SIZE		BIT(25)
#define SST_PCI_66_MHZ			BIT(26)
#define SST_PCI_AGP_ENABLED		BIT(27)
#define SST_PCI_DEVICE_TYPE		BIT(28)
#ifdef H3
#define SST_TVOUT_CLK_INV		BIT(28)
#define SST_TVOUT_CLK_DEL_ADJ_SHIFT     30
#define SST_TVOUT_CLK_DEL_ADJ		(0x3<<SST_TVOUT_CLK_DEL_ADJ_SHIFT)
#endif

// dramInit0
#define SST_SGRAM_RRD_SHIFT             0
#define SST_SGRAM_RRD                   (0x3L<<SST_SGRAM_RRD_SHIFT)
#define SST_SGRAM_RCD_SHIFT             2
#define SST_SGRAM_RCD                   (0x3L<<SST_SGRAM_RCD_SHIFT)
#define SST_SGRAM_RP_SHIFT              4
#define SST_SGRAM_RP                    (0x3L<<SST_SGRAM_RP_SHIFT)
#define SST_SGRAM_RAS_SHIFT             6
#define SST_SGRAM_RAS                   (0xFL<<SST_SGRAM_RAS_SHIFT)
#define SST_SGRAM_RC_SHIFT              10
#define SST_SGRAM_RC                    (0xFL<<SST_SGRAM_RC_SHIFT)
#define SST_SGRAM_CAS_SHIFT             14
#define SST_SGRAM_CAS                   (0x3L<<SST_SGRAM_CAS_SHIFT)
#define SST_SGRAM_MRS_SHIFT             16
#define SST_SGRAM_MRS                   BIT(16)
#define SST_SGRAM_RD2DQM                BIT(17)
#define SST_SGRAM_BWC                   BIT(18)
#define SST_SGRAM_WL                    BIT(19)
#define SST_SGRAM_BWL_SHIFT             20
#define SST_SGRAM_BWL                   (0x3L<<SST_SGRAM_BWL_SHIFT)
#define SST_SGRAM_RL                    BIT(22)
#define SST_SGRAM_USE_BST               BIT(23)
#define SST_SGRAM_NO_DEAD               BIT(24)
#define SST_SGRAM_EN_WPB                BIT(25)
#define SST_SGRAM_NUM_CHIPSETS          BIT(26)
#define SST_SGRAM_TYPE_SHIFT            27
#define SST_SGRAM_TYPE                  (0x1L<<SST_SGRAM_TYPE_SHIFT)
#       define SST_SGRAM_TYPE_8MBIT     (0x0L<<SST_SGRAM_TYPE_SHIFT)
#       define SST_SGRAM_TYPE_16MBIT    (0x1L<<SST_SGRAM_TYPE_SHIFT)

// dramInit1
#define SST_DRAM_REFRESH_EN             BIT(0)
#define SST_DRAM_REFRESH_VALUE_SHIFT    1
#define SST_DRAM_REFRESH_VALUE          (0x1FF<<SST_DRAM_REFRESH_VALUE_SHIFT)
#define SST_VIDEO_OVERRIDE_EN           BIT(10)
#define SST_TRIPLE_BUFFER_EN            BIT(11)
#define SST_DITHER_PASSTHROUGH          BIT(12)
#define SST_SGRAM_CLK_NODELAY           BIT(13)
#define SST_SGRAM_USE_INV_SAMPLE        BIT(14)
#define SST_SGRAM_DEL_CLK_INVERT        BIT(15)
#define SST_SGRAM_CLK_ADJ_SHIFT         16
#define SST_SGRAM_CLK_ADJ               (0xf << SST_SGRAM_CLK_ADJ_SHIFT)
#define SST_SGRAM_OFLOP_DEL_ADJ_SHIFT   20
#define SST_SGRAM_OFLOP_DEL_ADJ         (0xf << SST_SGRAM_OFLOP_DEL_ADJ_SHIFT)
#define SST_SGRAM_OFLOP_TRANS_LATCH     BIT(24)
#define SST_MCTL_SHORT_POWER_ON         BIT(25)
#define SST_MCTL_NO_AGGRESSIVE          BIT(26)
#define SST_MCTL_PAGEBREAK              BIT(27)
#define SST_MCTL_TRISTATE_OUTPUTS       BIT(28)
#define SST_MCTL_NO_VIN_LOCKING         BIT(29)
#define SST_MCTL_TYPE_SDRAM             BIT(30)

// agpInit0
#define SST_AGP_LTOH                    BIT(0)
#define SST_AGP_RQSIZE_SHIFT            BIT(1)
#define SST_AGP_RQSIZE                  (0x7<<SST_AGP_RQSIZE_SHIFT)
#define SST_AGP_REQFULL_THRESH_SHIFT    4 
#define SST_AGP_REQFULL_THRESH          (0x7<<SST_AGP_REQFULL_THRESH_SHIFT)
#define SST_AGP_READFULL_THRESH_SHIFT   7 
#define SST_AGP_READFULL_THRESH         (0xF<<SST_AGP_READFULL_THRESH_SHIFT)

// tmuGbeInit
#define SST_TGI_READLOW_THRESH_SHIFT    0
#define SST_TGI_READLOW_THRESH          (0xF<<SST_TGI_READLOW_THRESH_SHIFT)
#define SST_TGI_READHIGH_THRESH_SHIFT   4 
#define SST_TGI_READHIGH_THRESH         (0xF<<SST_TGI_READHIGH_THRESH_SHIFT)
#define SST_TGI_PIXELHIGH_THRESH_SHIFT  8
#define SST_TGI_PIXELHIGH_THRESH        (0xF<<SST_TGI_PIXELHIGH_THRESH_SHIFT)
#ifdef H4
#define SST_TVOUT_CLK_INVERT   		BIT(15)
#define SST_TVOUT_CLK_DELAY_SHIFT	16
#define SST_TVOUT_CLK_DELAY		(0xF<<SST_TVOUT_CLK_DELAY_SHIFT)
#endif

// vgaInit0
#define SST_VGA0_DISABLE                BIT(0)
#define SST_VGA0_EXTERNAL_TIMING        BIT(1)
#define SST_VGA0_CLUT_SELECT_SHIFT      2
#define SST_VGA0_CLUT_SELECT            (1 << SST_CLUT_SELECT_SHIFT)
#       define SST_CLUT_SELECT_6BIT     0
#       define SST_CLUT_SELECT_8BIT     1
#define SST_VGA0_EXTENSIONS             BIT(6)
#define SST_VGAINIT0_R7                 BIT(7)
#define SST_VGA0_WAKEUP_SELECT_SHIFT    8
#define SST_VGA0_WAKEUP_SELECT          (1 << SST_VGA0_WAKEUP_SELECT_SHIFT)
#       define SST_WAKEUP_46e8          0
#       define SST_WAKEUP_3C3           1
#define SST_VGA0_LEGACY_DECODE_SHIFT    9
#define SST_VGA0_LEGACY_DECODE          (1 << SST_VGA0_LEGACY_DECODE_SHIFT)
#       define SST_VGA0_ENABLE_DECODE   0
#       define SST_VGA0_DISABLE_DECODE  1
#define SST_VGA0_CONFIG_READBACK_SHIFT  10
#define SST_VGA0_CONFIG_READBACK        (1 << SST_VGA0_CONFIG_READBACK_SHIFT)
#       define SST_ENABLE_ALT_READBACK  0
#       define SST_DISABLE_ALT_READBACK 1


// fill in the reset of vgaInit0 sometime
#define SST_VGA0_VGA_BASE_ADDR_SHIFT    14
#define SST_VGA0_VGA_BASE_ADDR          (0xff << SST_VGA0_VGA_BASE_ADDR_SHIFT)

// vgaInit1
// fill this in some day, please
#define SST_VGA_LEGACY_DECODE BIT(9)

// vga legacy definitions!
#define SST_VGA_SEQ_CLKMODE_SCRN_OFF    BIT(5)

//----------------- fastfillCMD bits --------------------------
#define SST_FASTFILL_DISABLE_DITHER     BIT(0)

//----------------- nopCMD bits -------------------------------
#define SST_NOP_RESET_PIXEL_STATS       BIT(0)
#define SST_NOP_RESET_TRIANGLE_STATS    BIT(1)

//----------------- swapBufferCMD bits ------------------------
#define SST_SWAP_EN_WAIT_ON_VSYNC       BIT(0)
#define SST_SWAP_BUFFER_INTERVAL_SHIFT  1
#define SST_SWAP_BUFFER_INTERVAL        (0xff << SST_SWAP_BUFFER_INTERVAL_SHIFT)
#define SST_SWAP_DONT_SWAP              BIT(9)

//----------------- SST Configuration Registers ---------------
#define SST_MEMBASE_ADDR                0x10
#define SST_INIT_ENABLE                 0x40
#define SST_BUS_SNOOP0                  0x44
#define SST_BUS_SNOOP1                  0x48
#define SST_CFG_STATUS                  0x4c
#define SST_AGP_COMMAND                 0x5c

//----------------- SST Remapped Registers --------------------
#define SST_DAC_RD_DATA                 0x00000218
#define SST_VIDEO_CHECKSUM              0x0000021c

//----------------- SST initEnable (Config register) bits -----
#define SST_INITWR_EN                   BIT(0)
#define SST_PCI_FIFOWR_EN               BIT(1)
#define SST_FBIINIT23_REMAP             BIT(2)
#define SST_SNOOP0_EN                   BIT(4)
#define SST_SNOOP0_IO                   BIT(5)
#define SST_SNOOP0_RD                   BIT(6)
#define SST_SNOOP1_EN                   BIT(7)
#define SST_SNOOP1_IO                   BIT(8)
#define SST_SNOOP1_RD                   BIT(9)
#define SST_SCANLINE_SLV_OWNPCI         BIT(10)
#define SST_SCANLINE_SLI_SLV            BIT(11)

//----------------- SST vRetrace bits ---------------------
#define SST_VRETRACE_VSYNC_OFF  0xff

//----------------- SST hSync bits ------------------------
#define SST_HSYNC_ON            0xff
#define SST_HSYNC_OFF_SHIFT     16
#define SST_HSYNC_OFF           (0x3ff << SST_HSYNCOFF_SHIFT)

//----------------- SST vSync bits ------------------------
#define SST_VSYNC_ON            0xff
#define SST_VSYNC_OFF_SHIFT     16
#define SST_VSYNC_OFF           (0xfff << SST_VSYNC_OFF_SHIFT)

//----------------- SST backPorch bits --------------------
#define SST_HBACKPORCH          0xff
#define SST_VBACKPORCH_SHIFT    16
#define SST_VBACKPORCH          (0xff << SST_VBACKPORCH_SHIFT)

//----------------- SST videoDimensionS Register -----------
#define SST_VIDEODIMENSIONS_X   0x3ff
#define SST_VIDEODIMENSIONS_Y_SHIFT 16
#define SST_VIDEODIMENSIONS_Y   0x3ff


//------------------- CMD/AGP Registers ----------------------------
//------------------- agpReqSize
#define SST_AGPREQSIZE_SHIFT    0
#define SST_AGPREQSIZE          (0xfffff << SST_AGPREQSIZE_SHIFT)

//------------------- hostAddrLow
#define SST_AGPHOSTADDRESSLOW_SHIFT     0
#define SST_AGPHOSTADDRESSLOW   (0xffffffff << SST_AGPHOSTADDRESSLOW_SHIFT)

//------------------- hostAddrHigh
#define SST_AGPWIDTH_SHIFT      0
#define SST_AGPWIDTH            (0x3fff << SST_AGPWIDTH_SHIFT)
#define SST_AGPSTRIDE_SHIFT     14
#define SST_AGPSTRIDE           (0x3fff << SST_AGPSTRIDE_SHIFT)
#define SST_AGPADDRHIGH_SHIFT   28              
#define SST_AGPADDRHIGH         (0xf << SST_AGPADDRHIGH_SHIFT)

//------------------- graphicsAddr
#define SST_AGPGFXADDR_SHIFT    0
#define SST_AGPGFXADDR          (0x3fffff << SST_AGPGFXADDR_SHIFT)

//------------------- graphicsStride
#define SST_AGPGFXSTRIDE_SHIFT  0
#define SST_AGPGFXSTRIDE        (0x7fff << SST_AGPGFXSTRIDE_SHIFT)

//------------------- moveCMD
#define SST_AGPMEMTYPE_SHIFT    3
#define SST_AGPMEMTYPE          (0x7 << SST_AGPMEMTYPE_SHIFT)
#define   SST_AGPMEMTYPE_RAWLFB (0x0 << SST_AGPMEMTYPE_SHIFT)
#define   SST_AGPMEMTYPE_YUV    (0x1 << SST_AGPMEMTYPE_SHIFT)
#define   SST_AGPMEMTYPE_3DLFB  (0x2 << SST_AGPMEMTYPE_SHIFT)
#define   SST_AGPMEMTYPE_TEX    (0x3 << SST_AGPMEMTYPE_SHIFT)
#define SST_CMDFIFOID_SHIFT     5
#define SST_CMDFIFO             (1 << SST_CMDFIFOID_SHIFT)
#define SST_CMDFIFO_0           (0 << SST_CMDFIFOID_SHIFT)
#define SST_CMDFIFO_1           (1 << SST_CMDFIFOID_SHIFT)

//-----------------------Command FIFO Registers
//------------------- baseAddrL
#define SST_BASEADDRL_SHIFT     0
#define SST_BASEADDRL           (0xffffff << SST_BASEADDRL_SHIFT)

//------------------- baseSize
#define SST_BASESIZE_SHIFT      0
#define SST_BASESIZE            (0xf << SST_BASESIZE_SHIFT)
#define SST_CMDFIFOEN_SHIFT     8
#define SST_CMDFIFOEN           (1 << SST_CMDFIFOEN_SHIFT)
#define SST_CMDFIFOAGP_SHFIT    9
#define SST_CMDFIFOAGP          (1 << SST_CMDFIFOAGP_SHFIT)
#define SST_HOLECNTDISABLE_SHIFT        10
#define SST_HOLECNTDISABLE      (1 << SST_HOLECNTDISABLE_SHIFT)

//------------------- bump
#define SST_CMDBUMP_SHIFT       0
#define SST_CMDBUMP             (0xffff << SST_CMDBUMP_SHIFT)

//------------------- readPtrL
#define SST_READPTRL_SHIFT      0
#define SST_READPTRL            (0xffffffff << SST_READPTRL_SHIFT)

//------------------- readPtrH
#define SST_READPTRH_SHIFT      0
#define SST_READPTRH            (0xf << SST_READPTRH_SHIFT)

//------------------- aMin
#define SST_CMDAMIN_SHIFT       0
#define SST_CMDAMIN             (0x1ffffff << SST_CMDAMIN_SHIFT)

//------------------- aMax
#define SST_CMDAMAX_SHIFT       0
#define SST_CMDAMAX             (0x1ffffff << SST_CMDAMIN_SHIFT)

//------------------- depth
#define SST_CMDDEPTH_SHIFT      0
#define SST_CMDDEPTH            (0xfffff << SST_CMDDEPTH_SHIFT)

//------------------- holeCount
#define SST_CMDHOLECNT_SHIFT    0
#define SST_CMDHOLECNT          (0xffff << SST_CMDHOLECNT_SHIFT)

//------------------- cmdFifoThresh
#define SST_FETCHTHRESH_SHIFT   0
#define SST_FETCHTHRESH         (0x1f << SST_FETCHTHRESH_SHIFT)
#define SST_HIGHWATER_SHIFT     5
#define SST_HIGHWATER           (0xf << SST_HIGHWATER_SHIFT)

//------------------- cmdHoleInit
#define SST_HOLETIMEOUT_SHIFT   0
#define SST_HOLETIMEOUT         (0x3fffff << SST_HOLETIMEOUT_SHIFT)
#define SST_HOLETIMEOUT_EN_SHIFT        22
#define SST_HOLETIMEOUT_EN      (1 << SST_HOLETIMEOUT_EN_SHIFT)

//------------------- yuvBaseAddr
#define SST_YUVBASEADDR_SHIFT   0
#define SST_YUVBASEADDR         (0x1ffffff << SST_YUVBASEADDR_SHIFT)

//------------------- yuvStride
#define SST_YUVSTRIDE_SHIFT     0
#define SST_YUVSTRIDE           (0x3fff << SST_YUVSTRIDE_SHIFT)
#define SST_YUVDESTTILED_SHIFT  31
#define SST_YUVDESTTILED        (1 << SST_YUVDESTTILED_SHIFT)


// Here are the defines for the alternate register mappings
#define r_ALT           r
#define drdx_ALT        g
#define drdy_ALT        b
#define g_ALT           z
#define dgdx_ALT        a
#define dgdy_ALT        s
#define b_ALT           t
#define dbdx_ALT        w

#define dbdy_ALT        drdx
#define z_ALT           dgdx
#define dzdx_ALT        dbdx
#define dzdy_ALT        dzdx
#define a_ALT           dadx
#define dadx_ALT        dsdx
#define dady_ALT        dtdx
#define s_ALT           dwdx

#define dsdx_ALT        drdy
#define dsdy_ALT        dgdy
#define t_ALT           dbdy
#define dtdx_ALT        dzdy
#define dtdy_ALT        dady
#define w_ALT           dsdy
#define dwdx_ALT        dtdy
#define dwdy_ALT        dwdy

#define Fr_ALT          Fr
#define Fdrdx_ALT       Fg
#define Fdrdy_ALT       Fb
#define Fg_ALT          Fz
#define Fdgdx_ALT       Fa
#define Fdgdy_ALT       Fs
#define Fb_ALT          Ft
#define Fdbdx_ALT       Fw

#define Fdbdy_ALT       Fdrdx
#define Fz_ALT          Fdgdx
#define Fdzdx_ALT       Fdbdx
#define Fdzdy_ALT       Fdzdx
#define Fa_ALT          Fdadx
#define Fdadx_ALT       Fdsdx
#define Fdady_ALT       Fdtdx
#define Fs_ALT          Fdwdx

#define Fdsdx_ALT       Fdrdy
#define Fdsdy_ALT       Fdgdy
#define Ft_ALT          Fdbdy
#define Fdtdx_ALT       Fdzdy
#define Fdtdy_ALT       Fdady
#define Fw_ALT          Fdsdy
#define Fdwdx_ALT       Fdtdy
#define Fdwdy_ALT       Fdwdy

//----------------- useful addressing macros -----------------------
// return pointer to SST at specified WRAP, CHIP, or TREX
#define SST_WRAP(sst,n) ((SstRegs *)((n)*0x4000+(FxI32)(sst)))
#define SST_CHIP(sst,n) ((SstRegs *)((n)*0x400+(FxI32)(sst)))
#define SST_TMU(sst,n)  ((SstRegs *)((0x800<<(n))+(FxI32)(sst)))
#define SST_TREX(sst,n) SST_TMU(sst,n)

// offsets from the base of memBaseAddr0
#define SST_IO_OFFSET           0x000000
#define SST_CMDAGP_OFFSET       0x080000
#define SST_2D_OFFSET           0x100000
#define SST_3D_OFFSET           0x200000
#define SST_3D_ALT_OFFSET       0x400000
#ifdef H4
#define SST_TEX_OFFSET          0x600000
#define SST_TEX0_OFFSET         0x600000
#define SST_TEX1_OFFSET         0x800000
#define SST_RESERVED_OFFSET     0xA00000
#else
#define SST_TEX_OFFSET          0x600000
#define SST_RESERVED_OFFSET     0x800000
#endif
#define SST_YUV_OFFSET          0xC00000
#define SST_LFB_OFFSET          0x1000000

// NOTE: this assumes that memBaseAddr1 is mapped such that it is
// immediately after the 32MB pointed to by memBaseAddr0
#define SST_RAW_LFB_OFFSET      0x2000000
#define SST_MAX_LEGAL_OFFSET    0x4000000

#define SST_IS_IO_ADDR(a)        ( (a) >= SST_IO_OFFSET       && (a) < SST_CMDAGP_OFFSET )
#define SST_IS_CMDAGP_ADDR(a)    ( (a) >= SST_CMDAGP_OFFSET   && (a) < SST_2D_OFFSET )
#define SST_IS_2D_ADDR(a)        ( (a) >= SST_2D_OFFSET       && (a) < SST_3D_OFFSET )
#define SST_IS_3D_ADDR(a)        ( (a) >= SST_3D_OFFSET       && (a) < SST_3D_ALT_OFFSET )
#define SST_IS_3D_ALT_ADDR(a)    ( (a) >= SST_3D_ALT_OFFSET   && (a) < SST_TEX_OFFSET )
#ifdef H4
#define SST_IS_TEX_ADDR(a)       ( (a) >= SST_TEX0_OFFSET      && (a) < SST_RESERVED_OFFSET )
#define SST_IS_TEX0_ADDR(a)       ( (a) >= SST_TEX0_OFFSET      && (a) < SST_TEX1_OFFSET )
#define SST_IS_TEX1_ADDR(a)       ( (a) >= SST_TEX1_OFFSET      && (a) < SST_RESERVED_OFFSET )
#else
#define SST_IS_TEX_ADDR(a)       ( (a) >= SST_TEX_OFFSET      && (a) < SST_RESERVED_OFFSET )
#endif
#define SST_IS_RESERVED_ADDR(a)  ( (a) >= SST_RESERVED_OFFSET && (a) < SST_YUV_OFFSET )
#define SST_IS_YUV_ADDR(a)       ( (a) >= SST_YUV_OFFSET      && (a) < SST_LFB_OFFSET )
#define SST_IS_LFB_ADDR(a)       ( (a) >= SST_LFB_OFFSET      && (a) < SST_RAW_LFB_OFFSET )
#define SST_IS_RAW_LFB_ADDR(a)   ( (a) >= SST_RAW_LFB_OFFSET  && (a) < SST_MAX_LEGAL_OFFSET )

#define SST_IS_REGISTER_ADDR(a)  ( (a) >= SST_IO_OFFSET       && (a) < SST_TEX_OFFSET )

#define SST_BASE_ADDRESS(sst)   ((FxI32)(sst)-SST_3D_OFFSET)
#define SST_IO_ADDRESS(sst)     (SST_IO_OFFSET+SST_BASE_ADDRESS(sst))
#define SST_CMDAGP_ADDRESS(sst) (SST_CMDAGP_OFFSET+SST_BASE_ADDRESS(sst))
#define SST_GUI_ADDRESS(sst)    (SST_2D_OFFSET+SST_BASE_ADDRESS(sst))
#ifdef H4
#define SST_TEX_ADDRESS(sst)     (SST_TEX0_OFFSET+SST_BASE_ADDRESS(sst))
#define SST_TEX0_ADDRESS(sst)    (SST_TEX0_OFFSET+SST_BASE_ADDRESS(sst))
#define SST_TEX1_ADDRESS(sst)    (SST_TEX1_OFFSET+SST_BASE_ADDRESS(sst))
#else
#define SST_TEX_ADDRESS(sst)    (SST_TEX_OFFSET+SST_BASE_ADDRESS(sst))
#endif
#define SST_LFB_ADDRESS(sst)    (SST_LFB_OFFSET+SST_BASE_ADDRESS(sst))
#define SST_YUV_ADDRESS(sst)    (SST_YUV_OFFSET+SST_BASE_ADDRESS(sst))
#define SST_PORT_ADDRESS(sst)   (SST_FAKE_PORT_MAKE(SST_FAKE_ADDRESS_GET_BOARD(sst)))

#define SST_IS_MOVECMD(iaddr) (iaddr >= (SST_CMDAGP_OFFSET + AGPREQSIZE) && iaddr <= (SST_CMDAGP_OFFSET + MOVECMD))



// SET macros for FBI
#define SET_FBI(d,s)    SET (*(&(d)+0x100),s)
#define SET_FBIF(d,s)   SETF(*(&(d)+0x100),s)

// SET macros for TMU0
#define SET_0(d,s)      SET (*(&(d)+0x200),s)
#define SET_0F(d,s)     SETF(*(&(d)+0x200),s)

// SET macros for FBI+TMU0
#define SET_FBI_0(d,s)  SET (*(&(d)+0x300),s)
#define SET_FBI_0F(d,s) SETF(*(&(d)+0x300),s)

// SET macros for TMU1
#define SET_1(d,s)      SET (*(&(d)+0x400),s)
#define SET_1F(d,s)     SETF(*(&(d)+0x400),s)

// SET macros for FBI+TMU1
#define SET_FBI_1(d,s)  SET (*(&(d)+0x500),s)
#define SET_FBI_1F(d,s) SETF(*(&(d)+0x500),s)

#endif /* !__H3DEFS_H__ */

