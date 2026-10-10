#ifndef __H3GDEFS_H__
#define __H3GDEFS_H__

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

// STB Begin Changes
// STB-SR 1/13/98 Adding code for bj
#ifdef _H2INC
#include "cmddefs.h"
#endif

#ifndef _H2INC
// STB End Changes

// compute 2D base from 3D base
#define SSTG_CHIP(sst) ((SstGRegs *)SST_GUI_ADDRESS(sst))

// STB Begin Changes
#endif
// STB End Changes


#define SSTG_XY_SIZE 13

//----------------- SSTG command bits ---------------------------
#define SSTG_COMMAND_SHIFT      0
#define SSTG_COMMAND            (0xF<<SSTG_COMMAND_SHIFT)
#       define SSTG_NOP                 (0<<SSTG_COMMAND_SHIFT)
#       define SSTG_BLT                 (1<<SSTG_COMMAND_SHIFT)
#       define SSTG_STRETCH_BLT         (2<<SSTG_COMMAND_SHIFT)
#       define SSTG_HOST_BLT            (3<<SSTG_COMMAND_SHIFT)
#       define SSTG_HOST_STRETCH_BLT    (4<<SSTG_COMMAND_SHIFT)
#       define SSTG_RECTFILL            (5<<SSTG_COMMAND_SHIFT)
#       define SSTG_LINE                (6<<SSTG_COMMAND_SHIFT)
#       define SSTG_POLYLINE            (7<<SSTG_COMMAND_SHIFT)
#       define SSTG_POLYFILL            (8<<SSTG_COMMAND_SHIFT)
#if COLORTRANSLUT
#define SSTG_EN_CLUT88          BIT(7)
#endif
#define SSTG_GO                 BIT(8)
#define SSTG_REVERSIBLE         BIT(9)
#define SSTG_UPDATE_DSTX        BIT(10)
#define SSTG_UPDATE_DSTY        BIT(11)
#define SSTG_EN_LINESTIPPLE     BIT(12)
#define SSTG_MONO_PATTERN       BIT(13)
#define SSTG_XDIR               BIT(14)
#define SSTG_YDIR               BIT(15)
#define SSTG_TRANSPARENT        BIT(16)
#define SSTG_X_PATOFFSET_SHIFT  17
#       define SSTG_X_PATOFFSET         (0x7UL<<SSTG_X_PATOFFSET_SHIFT)
#define SSTG_Y_PATOFFSET_SHIFT  20
#       define SSTG_Y_PATOFFSET         (0x7UL<<SSTG_Y_PATOFFSET_SHIFT)
#define SSTG_CLIPSELECT         BIT(23)
#define SSTG_ROP0_SHIFT         24
#       define SSTG_ROP0                (0xFFUL<<SSTG_ROP0_SHIFT)

//----------------- SSTG commandEx bits ---------------------------
#define SSTG_EN_SRC_COLORKEY_EX BIT(0)
#define SSTG_EN_DST_COLORKEY_EX BIT(1)
#define SSTG_WAIT_FOR_VSYNC_EX  BIT(2)
#define SSTG_PAT_FORCE_ROW0     BIT(3)

//----------------- SSTG src,dstBaseAddr bits ---------------------------
#define SSTG_IS_TILED   BIT(31)
#define SSTG_BASEADDR_SHIFT 0
#define SSTG_BASEADDR (0xFFFFFF << SSTG_BASEADDR_SHIFT)

//----------------- SSTG srcFormat bits ---------------------------
#define SSTG_SRC_STRIDE_SHIFT   0
#       define SSTG_SRC_LINEAR_STRIDE   (0x3FFFUL << SSTG_SRC_STRIDE_SHIFT)
#       define SSTG_SRC_TILE_STRIDE     (0x7FUL << SSTG_SRC_STRIDE_SHIFT)
#define SSTG_SRC_FORMAT_SHIFT   16
#       define SSTG_SRC_FORMAT  (0xFUL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_HOST_BYTE_SWIZZLE  BIT(20)
#define SSTG_HOST_WORD_SWIZZLE  BIT(21)
#define SSTG_SRC_PACK_SHIFT     22
#       define SSTG_SRC_PACK    (0x3UL << SSTG_SRC_PACK_SHIFT)
#       define SSTG_SRC_PACK_SRC (0x0UL << SSTG_SRC_PACK_SHIFT)
#       define SSTG_SRC_PACK_8  (0x1UL << SSTG_SRC_PACK_SHIFT)
#       define SSTG_SRC_PACK_16 (0x2UL << SSTG_SRC_PACK_SHIFT)
#       define SSTG_SRC_PACK_32 (0x3UL << SSTG_SRC_PACK_SHIFT)

//----------------- SSTG dstFormat bits ---------------------------
#define SSTG_DST_STRIDE_SHIFT   0
#       define SSTG_DST_LINEAR_STRIDE   (0x3FFFUL << SSTG_DST_STRIDE_SHIFT)
#       define SSTG_DST_TILE_STRIDE     (0x7FUL << SSTG_DST_STRIDE_SHIFT)
#define SSTG_DST_FORMAT_SHIFT   16
#       define SSTG_DST_FORMAT  (0x7UL << SSTG_DST_FORMAT_SHIFT)

#define SSTG_PIXFMT_1BPP        (0x0UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_8BPP        (0x1UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_15BPP       (0x2UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_16BPP       (0x3UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_24BPP       (0x4UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_32BPP       (0x5UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_422YUV      (0x8UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_422UYV      (0x9UL << SSTG_SRC_FORMAT_SHIFT)
#ifdef YUV411
411 has been removed from the spec as a blit source format -KMW
#define SSTG_PIXFMT_411YUV      (0xAUL << SSTG_SRC_FORMAT_SHIFT)
#endif

#if SSTG_SRC_FORMAT_SHIFT != SSTG_DST_FORMAT_SHIFT
        error: these need to be the same
#endif

//----------------- SSTG lineStyle bits ---------------------------
#define SSTG_LSREPEAT_SHIFT     0
#define SSTG_LSREPEAT           (0xFFUL << SSTG_LSREPEAT_SHIFT)
#define SSTG_LSSIZE_SHIFT       8
#define SSTG_LSSIZE             (0x1FUL << SSTG_LSSIZE_SHIFT)
#define SSTG_LSPOS_FRAC_SHIFT   16
#define SSTG_LSPOS_FRAC         (0xFFUL << SSTG_LSPOS_FRAC_SHIFT)
#define SSTG_LSPOS_INT_SHIFT    24
#define SSTG_LSPOS_INT          (0x1FUL << SSTG_LSPOS_INT_SHIFT)

//----------------- SSTG rop defines ---------------------------
#define SSTG_ROP_ZERO   0x00UL
#define SSTG_ROP_NOR    0x11UL
#define SSTG_ROP_ANDI   0x22UL
#define SSTG_ROP_NSRC   0x33UL
#define SSTG_ROP_ANDR   0x44UL
#define SSTG_ROP_NDST   0x55UL
#define SSTG_ROP_XOR    0x66UL
#define SSTG_ROP_NAND   0x77UL
#define SSTG_ROP_AND    0x88UL
#define SSTG_ROP_XNOR   0x99UL
#define SSTG_ROP_DST    0xaaUL
#define SSTG_ROP_ORI    0xbbUL
#define SSTG_ROP_SRC    0xccUL
#define SSTG_ROP_ORR    0xddUL
#define SSTG_ROP_OR     0xeeUL
#define SSTG_ROP_ONE    0xffUL

// SSTG ternary rop defines (named rop3's that use pattern)
#define SSTG_ROP_PATINVERT      0x5aUL
#define SSTG_ROP_MERGECOPY      0xC0UL
#define SSTG_ROP_PATCOPY        0xF0UL
#define SSTG_ROP_PATPAINT       0xFBUL

#define SSTG_ISBINARYROP(rop) ((rop>>4)&0xF) == (rop&0xF)

// It's so annoying to have to put that << in real code, I'm shifting
// the rops here, damnit
#define SSTG_ROP_SRCCOPY        (0xccUL << SSTG_ROP0_SHIFT)

//-----------------------  CMDFIFO Section ---------------------------
//-----------------------  CMDFIFO Section ---------------------------
//-----------------------  CMDFIFO Section ---------------------------

// CVG/H3 abstraction
#define SSTC_IS_EITHER_CMDFIFO_ENABLED(sstc) \
        (((sstc)->cmdFifo0.baseSize & SST_EN_CMDFIFO) || ((sstc)->cmdFifo1.baseSize & SST_EN_CMDFIFO))

//----------------- SST cmdFifo*.baseSize bits ---------------------------
#define SST_CMDFIFO_SIZE                0xFF
#define SST_EN_CMDFIFO                  BIT(8)
#define SST_CMDFIFO_AGP                 BIT(9)
#define SST_CMDFIFO_DISABLE_HOLES       BIT(10)

// SST COMMAND PACKET defines
#define SSTCP_PKT_SIZE  3
#define SSTCP_PKT       SST_MASK(SSTCP_PKT_SIZE)
#define SSTCP_PKT0      0
#define SSTCP_PKT1      1
#define SSTCP_PKT2      2
#define SSTCP_PKT3      3
#define SSTCP_PKT4      4
#define SSTCP_PKT5      5
#define SSTCP_PKT6      6
#define SSTCP_PKT7      7

#define SSTCP_BOGUS_WORDS_SHIFT 29
#define SSTCP_BOGUS_WORDS       (7 << SSTCP_BOGUS_WORDS_SHIFT)

// packet 0 defines
#define SSTCP_PKT0_FUNC_SHIFT   3
#define SSTCP_PKT0_FUNC              (7<<SSTCP_PKT0_FUNC_SHIFT)
#define SSTCP_PKT0_NOP              ((0<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_JSR              ((1<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_RET              ((2<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_JMP_LOCAL    ((3<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_JMP_AGP        ((4<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_ADDR_SHIFT   6
#define SSTCP_PKT0_ADDR             (0x7FFFFF<<SSTCP_PKT0_ADDR_SHIFT)

// packet 1 defines
#define SSTCP_REGBASE_SHIFT     SSTCP_PKT_SIZE
#ifdef H4
#define SSTCP_REGBASE           (0x7FF<<SSTCP_REGBASE_SHIFT)
#define SSTCP_REGBASE_FROM_ADDR(x) ( (((x)>>2) & 0x7FF) << SSTCP_REGBASE_SHIFT )
#else
#define SSTCP_REGBASE           ((0x3FF)<<SSTCP_REGBASE_SHIFT)
#define SSTCP_REGBASE_FROM_ADDR(x) ( (((x)>>2) & 0x3FF) << SSTCP_REGBASE_SHIFT )
#endif
#define SSTCP_PKT1_2D           BIT(14)
#define SSTCP_INC               BIT(15)
#define SSTCP_PKT1_NWORDS_SHIFT 16
#define SSTCP_PKT1_NWORDS       (0xFFFFUL<<SSTCP_PKT1_NWORDS_SHIFT)

// packet 2 defines
#define SSTCP_PKT2_MASK_SHIFT   SSTCP_PKT_SIZE
#define SSTCP_PKT2_MASK         (0x1FFFFFFFUL<<SSTCP_PKT2_MASK_SHIFT)

// packet 3 defines
#define SSTCP_PKT3_CMD_SHIFT    SSTCP_PKT_SIZE
#define SSTCP_PKT3_CMD          (0x7<<SSTCP_PKT3_CMD_SHIFT)
#  define SSTCP_PKT3_BDDBDD     (0<<SSTCP_PKT3_CMD_SHIFT)
#  define SSTCP_PKT3_BDDDDD     (1<<SSTCP_PKT3_CMD_SHIFT)
#  define SSTCP_PKT3_DDDDDD     (2<<SSTCP_PKT3_CMD_SHIFT)
#define SSTCP_PKT3_NUMVERTEX_SHIFT (SSTCP_PKT_SIZE+3)
#define SSTCP_PKT3_NUMVERTEX    (0xF << SSTCP_PKT3_NUMVERTEX_SHIFT)
#define SSTCP_PKT3_PMASK_SHIFT  (SSTCP_PKT_SIZE+3+4)
#define SSTCP_PKT3_PMASK        (0xFFFUL<<SSTCP_PKT3_PMASK_SHIFT)
#define SSTCP_PKT3_SMODE_SHIFT  (SSTCP_PKT3_PMASK_SHIFT+12)
#define SSTCP_PKT3_SMODE        (0x3FUL<<SSTCP_PKT3_SMODE_SHIFT)
#define SSTCP_PKT3_PACKEDCOLOR  BIT(28)

// packet 4 defines
#define SSTCP_PKT4_2D           BIT(14)
#define SSTCP_PKT4_MASK_SHIFT   15
#define SSTCP_PKT4_MASK         (0x3FFFUL<<SSTCP_PKT4_MASK_SHIFT)

// packet 5 defines
#define SSTCP_PKT5_NWORDS_SHIFT 3
#define SSTCP_PKT5_NWORDS       (0x7FFFFUL<<SSTCP_PKT5_NWORDS_SHIFT)
#define SSTCP_PKT5_BYTEN_WN_SHIFT 22
#define SSTCP_PKT5_BYTEN_WN     (0xFUL<<SSTCP_PKT5_BYTEN_WN_SHIFT)
#define SSTCP_PKT5_BYTEN_W2_SHIFT 26
#define SSTCP_PKT5_BYTEN_W2     (0xFUL<<SSTCP_PKT5_BYTEN_W2_SHIFT)
#define SSTCP_PKT5_SPACE_SHIFT  30
#define SSTCP_PKT5_SPACE        (0x3UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_LFB          (0x0UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_YUV          (0x1UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_3DLFB        (0x2UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_TEXPORT      (0x3UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_BASEADDR     0x1FFFFFFUL

// packet 6 defines
#define SSTCP_PKT6_SPACE_SHIFT  3
#define SSTCP_PKT6_SPACE        (0x3UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_LFB          (0x0UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_YUV          (0x1UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_3DLFB        (0x2UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_TEXPORT      (0x3UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_NBYTES_SHIFT 5
#define SSTCP_PKT6_NBYTES       (SST_MASK(20) << SSTCP_PKT6_NBYTES_SHIFT)
#define SSTCP_PKT6_SRC_BASELOW            SST_MASK(32)
#define SSTCP_PKT6_SRC_WIDTH              SST_MASK(14)
#define SSTCP_PKT6_SRC_STRIDE_SHIFT       14
#define SSTCP_PKT6_SRC_STRIDE             (SST_MASK(14) <<SSTCP_PKT6_SRC_STRIDE_SHIFT)
#define SSTCP_PKT6_SRC_BASEHIGH_SHIFT     28
#define SSTCP_PKT6_SRC_BASEHIGH           (SST_MASK(4) <<SSTCP_PKT6_SRC_BASEHIGH_SHIFT)
#define SSTCP_PKT6_FRAME_BUFFER_OFFSET    SST_MASK(26)
#define SSTCP_PKT6_DST_STRIDE             SST_MASK(15)


// move command
#define SST_AGP_REQUEST_SIZE        SST_MASK(20)
#define SST_AGP_MOVE_BASELOW        SSTCP_PKT6_SRC_BASELOW
#define SST_AGP_SRC_WIDTH           SSTCP_PKT6_SRC_WIDTH
#define SST_AGP_SRC_STRIDE_SHIFT    SSTCP_PKT6_SRC_STRIDE_SHIFT
#define SST_AGP_SRC_STRIDE          SSTCP_PKT6_SRC_STRIDE
#define SST_AGP_SRC_BASEHIGH_SHIFT  SSTCP_PKT6_SRC_BASEHIGH_SHIFT
#define SST_AGP_SRC_BASEHIGH        SSTCP_PKT6_SRC_BASEHIGH
#define SST_AGP_FRAME_BUFFER_OFFSET SSTCP_PKT6_FRAME_BUFFER_OFFSET
#define SST_AGP_DSTSTRIDE           SSTCP_PKT6_DST_STRIDE

#define SST_AGPMOVE_SPACE_SHIFT      SSTCP_PKT6_SPACE_SHIFT
#define SST_AGPMOVE_SPACE            (0x3UL<<SST_AGPMOVE_SPACE_SHIFT)
#define SST_AGPMOVE_LFB              (0x0UL<<SST_AGPMOVE_SPACE_SHIFT)
#define SST_AGPMOVE_YUV              (0x1UL<<SST_AGPMOVE_SPACE_SHIFT)
#define SST_AGPMOVE_3DLFB            (0x2UL<<SST_AGPMOVE_SPACE_SHIFT)
#define SST_AGPMOVE_TEXPORT          (0x3UL<<SST_AGPMOVE_SPACE_SHIFT)
#define SST_AGPMOVE_CMDID            BIT(5)
#define SST_AGPMOVE_CMDID_SHIFT      5
#define SST_AGPMOVE_CMDID0           0
#define SST_AGPMOVE_CMDID1           BIT(5)

#define SSTCP_SPACE                  0x3UL
#define SSTCP_LFB_SPACE              0x0UL
#define SSTCP_YUV_SPACE              0x1UL
#define SSTCP_3DLFB_SPACE            0x2UL
#define SSTCP_TEXPORT_SPACE          0x3UL

#endif /* !__H3GDEFS_H__ */
