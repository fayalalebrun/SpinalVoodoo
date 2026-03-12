## THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
## PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
## TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
## INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
## DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
## THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
## EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
## FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
## 
## USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
## RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
## TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
## AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
## SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
## THE UNITED STATES.  
## 
## COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
##
## Revision 1.2  1999/12/11 00:42:21  joseph
## Changed \(.data) to .section .data to fix build errors with binutils 2.9.5.
##
## Revision 1.1.1.1  1999/12/07 21:49:13  joseph
## Initial checkin into SourceForge.
##
##
## 
## 7     5/18/98 12:16p Peter
## culling enabling
#	
# 6     1/15/98 1:12p Peter
# dispatch w/o packing
# 
# 5     11/06/97 3:47p Peter
# 
# 4     11/04/97 5:04p Peter
# cataclysm part deux
# 
# 3     11/01/97 10:01a Peter
# tri dispatch stuff
# 
# 2     10/30/97 6:53p Peter
# first real cut at tri asm
# 
# 1     10/30/97 4:29p Peter
# asm tri code
# 
# 2     7/07/97 2:14p Jdt
# assembly now on par with C code.
# 
# 1     7/07/97 8:37a Jdt
# B4 Chip field fix.
##

.file   "xdraw2.s"
#OPTION OLDSTRUCTS       
# 586P

# This was xdraw2.s but I renamed it for now to work with the shared
# swlibs.  It hasn't changed to the format used in the .S files for
# the Voodoo3 Glide soruces.
	
# some useful floating load and store macros <ala gmt>
#flds    TEXTEQU <fld  DWORD PTR>
#fsubs   TEXTEQU <fsub DWORD PTR>
#fmuls   TEXTEQU <fmul DWORD PTR>

	.align 4
.section	.data
One         .DATA  0x03f800000
Area        .DATA  0
dxAB        .DATA  0
dxBC        .DATA  0
dyAB        .DATA  0
dyBC        .DATA  0
culltest    .DATA  0

	    ## Extra junk to pad out to the next cache line.
bias0	.DATA  0
pad1	.DATA  0
pad2	.DATA  0
pad3	.DATA  0
pad4	.DATA  0
pad5	.DATA  0
pad6	.DATA  0
pad7	.DATA  0
bias1	.DATA  0

#	_DATA   ENDS

# Ugly, but seems to workaround the problem with locally defined
# data segment globals not getting relocated properly when using
# djgpp.

zArea   .EQU One+0x04
zdxAB   .EQU One+0x08
zdxBC   .EQU One+0x0c
zdyAB   .EQU One+0x10
zdyBC   .EQU One+0x14
zculltest .EQU One+0x18

### Definitions of cvg regs and glide root structures.
.INCLUDE "fxgasm.h"

## enables/disables trisProcessed and trisDrawn counters
STATS .ASSIGNA 1

#--------------------------------------------------------------------------
# Arguments : STKOFF = 16 from 4 pushes
#.AIF \&GLIDE_DEBUG EQ 1
#STKOFF	.EQU  20
#.AELSE
STKOFF	.EQU  16
#.AENDI
_va$    .EQU  4 + STKOFF
_vb$    .EQU  8 + STKOFF
_vc$    .EQU 12 + STKOFF

X       .EQU 0
Y       .EQU 4
                                # edx is used as index, loading from *src
gc	.REG	(%esi)		# points to graphics context

.text

	.align 4
	.globl  _trisetup_cull
	.type _trisetup_cull, @function
_trisetup_cull:	
#        .code

.MACRO	SYMNAME prefix
	.LCULL\prefix:	
.ENDM

.MACRO	SYMNAMEOP op prefix
	\op .LCULL\prefix
.ENDM
				
GLIDE_CULLING       .ASSIGNA 1
GLIDE_PACK_RGB	    .ASSIGNA 0
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_GENERIC_SETUP .ASSIGNA 0
.INCLUDE "xdraw2.inc.S"
GLIDE_GENERIC_SETUP .ASSIGNA 0
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_PACK_RGB	    .ASSIGNA 0
GLIDE_CULLING       .ASSIGNA 0
.L_end_trisetup_cull:
	.size _trisetup_cull,.L_end_trisetup_cull-_trisetup_cull

.MACRO	SYMNAME prefix
	.LNOCULL\prefix:	
.ENDM

.MACRO	SYMNAMEOP op prefix
	\op .LNOCULL\prefix
.ENDM
.AIF \&GLIDE_PACKED_RGB EQ 1
	.align 4
        .globl  _trisetup_cull_rgb
	.type _trisetup_cull_rgb, @function
_trisetup_cull_rgb:	
#            .code

GLIDE_CULLING       .ASSIGNA 1
GLIDE_PACK_RGB	    .ASSIGNA 1
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_GENERIC_SETUP .ASSIGNA 0
.INCLUDE "xdraw2.inc.S"
GLIDE_GENERIC_SETUP .ASSIGNA 0
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_PACK_RGB	    .ASSIGNA 0
GLIDE_CULLING       .ASSIGNA 0
.L_end_trisetup_cull_rgb:
	.size _trisetup_cull_rgb,.L_end_trisetup_cull_rgb-_trisetup_cull_rgb

	.align 4
        .globl  _trisetup_cull_argb
	.type _trisetup_cull_argb, @function
_trisetup_cull_argb:	
#            .code

GLIDE_CULLING       .ASSIGNA 1
GLIDE_PACK_RGB	    .ASSIGNA 1
GLIDE_PACK_ALPHA    .ASSIGNA 1
GLIDE_GENERIC_SETUP .ASSIGNA 0
.INCLUDE "xdraw2.inc.S"
GLIDE_GENERIC_SETUP .ASSIGNA 0
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_PACK_RGB	    .ASSIGNA 0
GLIDE_CULLING       .ASSIGNA 0
.L_end_trisetup_cull_argb:
	.size _trisetup_cull_argb,.L_end_trisetup_cull_argb-_trisetup_cull_argb

.AENDI
	.align 4
        .global  _trisetup
	.type _trisetup, @function
_trisetup:	
#            .code

GLIDE_CULLING       .ASSIGNA 0
GLIDE_PACK_RGB	    .ASSIGNA 0
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_GENERIC_SETUP .ASSIGNA 0
.INCLUDE "xdraw2.inc.S"
GLIDE_GENERIC_SETUP .ASSIGNA 0
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_PACK_RGB	    .ASSIGNA 0
GLIDE_CULLING       .ASSIGNA 0
.L_end_trisetup:
	.size _trisetup,.L_end_trisetup-_trisetup

.AIF \&GLIDE_PACKED_RGB EQ 1
	.align 4
        .globl _trisetup_rgb
	.type _trisetup_rgb, @function
_trisetup_rgb:	
#            .code

GLIDE_CULLING       .ASSIGNA 0
GLIDE_PACK_RGB	    .ASSIGNA 1
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_GENERIC_SETUP .ASSIGNA 0
.INCLUDE "xdraw2.inc.S"
GLIDE_GENERIC_SETUP .ASSIGNA 0
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_PACK_RGB	    .ASSIGNA 0
GLIDE_CULLING       .ASSIGNA 0
.L_end_trisetup_rgb:
	.size _trisetup_rgb,.L_end_trisetup_rgb-_trisetup_rgb

	.align 4
        .globl _trisetup_argb
	.type _trisetup_argb, @function
_trisetup_argb:	
#            .code

GLIDE_CULLING       .ASSIGNA 0
GLIDE_PACK_RGB	    .ASSIGNA 1
GLIDE_PACK_ALPHA    .ASSIGNA 1
GLIDE_GENERIC_SETUP .ASSIGNA 0
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP .ASSIGNA 0
GLIDE_PACK_ALPHA    .ASSIGNA 0
GLIDE_PACK_RGB	    .ASSIGNA 0
GLIDE_CULLING       .ASSIGNA 0
.L_end_trisetup_argb:
	.size _trisetup_argb,.L_end_trisetup_argb-_trisetup_argb
.AENDI

#_TEXT ENDS
.END



