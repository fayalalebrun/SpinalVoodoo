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

/*  1     10/08/98 11:30a Brent */
/*   */
/*  5     10/07/98 9:43p Peter */
/*  triangle procs for 3DNow!(tm) */
/*   */
/*  4     9/21/98 7:02p Dow */
/*  3DNow! (tm) mods */
/*   */
/*  3     7/24/98 1:41p Hohn */
/*   */
/*  2     6/05/98 5:14p Atai */
/*  added condition code for win/clip and aa/non-aa */
/*   */
/*  7     5/18/98 12:16p Peter */
/*  culling enabling */
/*   */
/*  6     1/15/98 1:12p Peter */
/*  dispatch w/o packing */
/*   */
/*  5     11/06/97 3:47p Peter */
/*   */
/*  4     11/04/97 5:04p Peter */
/*  cataclysm part deux */
/*   */
/*  3     11/01/97 10:01a Peter */
/*  tri dispatch stuff */
/*   */
/*  2     10/30/97 6:53p Peter */
/*  first real cut at tri asm */
/*   */
/*  1     10/30/97 4:29p Peter */
/*  asm tri code */
/*   */
/*  2     7/07/97 2:14p Jdt */
/*  assembly now on par with C code. */
/*   */
/*  1     7/07/97 8:37a Jdt */
/*  B4 Chip field fix. */


.file "xdraw2.asm"



.data
	.type	One,@object
	.size	One,4
One:	.int	0x03f800000
	.type	Area,@object
	.size	Area,4
Area:	.int	0

/*  Definitions of cvg regs and glide root structures. */
#include "fxgasm.h"

/*  enables/disables trisProcessed and trisDrawn counters */
#define STATS 1

/*  Arguments (STKOFF = 16 from 4 pushes) */
#define STKOFF 16
#define _va STKOFF + 4
#define _vb STKOFF + 8
#define _vc STKOFF + 12

/*  coordinate offsets into vertex. */
/*  NB:  These are constants and are not */
/* 	    user settable like the rest of the */
/* 	    parameter offset. Weird. */
#define X 0
#define Y 4

.data
.section	.rodata
	.type	T2003,@object
	.size	T2003,4
T2003:	.int	0x046400000	/*  12288 */
	.type	T2005,@object
	.size	T2005,4
T2005:	.int	0x03f800000	/*  1 */
	.type	T2006,@object
	.size	T2006,4
T2006:	.int	0x043800000	/*  256 */

/* -------------------------------------------------------------------------- */

.text


.align 32

#ifdef GL_AMD3D
.globl _trisetup_3DNow_cull
.type _trisetup_3DNow_cull,@function
_trisetup_3DNow_cull:
#define UniqLabel(str) .L_trisetup_3DNow_cull ## str	
#else	
.globl _trisetup_Default_cull
.type _trisetup_Default_cull,@function
_trisetup_Default_cull:
#define UniqLabel(str) .L_trisetup_Default_cull ## str	
#endif
	
#define GLIDE_CULLING 1
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING

#ifdef GL_AMD3D	
.L_END__trisetup_3DNow_cull:
.size _trisetup_3DNow_cull,.L_END__trisetup_3DNow_cull-_trisetup_3DNow_cull
#else
.L_END__trisetup_Default_cull:
.size _trisetup_Default_cull,.L_END__trisetup_Default_cull-_trisetup_Default_cull
#endif
#undef UniqLabel	
		
.align 32

#ifdef GL_AMD3D
.globl _trisetup_3DNow_Default
.type _trisetup_3DNow_Default,@function
_trisetup_3DNow_Default:
#define UniqLabel(str) .L_trisetup_3DNow_Default ## str
#else
.globl _trisetup_Default_Default
.type _trisetup_Default_Default,@function
_trisetup_Default_Default:
#define UniqLabel(str) .L_trisetup_Default_Default ## str
#endif
		
#define GLIDE_CULLING 0
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING

#ifdef GL_AMD3D	
.L_END__trisetup_3DNow_Default:
.size _trisetup_3DNow_Default,.L_END__trisetup_3DNow_Default-_trisetup_3DNow_Default
#else
.L_END__trisetup_Default_Default:
.size _trisetup_Default_Default,.L_END__trisetup_Default_Default-_trisetup_Default_Default
#endif
#undef UniqLabel	
		
#ifndef GL_AMD3D
.align 32
.globl _trisetup_clip_coor_thunk
.type _trisetup_clip_coor_thunk,@function
_trisetup_clip_coor_thunk:

#ifdef gc
#undef gc
#endif	
#define gc %eax	/*  Current graphics context */
#define procPtr %ebx
#define vPtr %ecx

/*  Call through to the gc->curArchProcs.drawTrianglesProc w/o */
/*  adding extra stuff to the stack. I wish we could actually */
/*  do a direct return here w/o too much work. */
	lea _va-STKOFF(%esp) , vPtr	/*  Get vertex pointer address */
	mov (_GlideRoot+curGC) , gc	/*  GR_DCL_GC */

/*  If debugging make sure that we're in clip coordinates */
#ifdef GLIDE_DEBUG
	mov CoordinateSpace(gc) , %ebx
	test $1 , %ebx
	jnz __clipSpace
	xor %eax , %eax
	mov %eax , (%eax)
__clipSpace:
#endif	/*  GLIDE_DEBUG */

	mov drawTrianglesProc(gc) , procPtr	/*  Prefetch drawTriangles proc addr */
	push vPtr	/*  vertex array address */

	push $3	/*  3 vertices */
	push $1	/*  mode = grDrawVertexArray */

	call * procPtr	/*  (*gc->curArchProcs.drawTrianglesProc)(grDrawVertexArray, 3, vPtr) */

	ret	/*  pop 3 dwords (vertex addrs) and return     */
.L_END__trisetup_clip_coor_thunk:
.size _trisetup_clip_coor_thunk,.L_END__trisetup_clip_coor_thunk-_trisetup_clip_coor_thunk

#endif	/*  !GL_AMD3D */

.END

