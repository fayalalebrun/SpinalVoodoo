/*  
 * THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
 * PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
 * TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
 * INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
 * DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
 * THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
 * FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
 * 
 * USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
 * RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
 * TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
 * AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
 * SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
 * THE UNITED STATES.  
 * 
 * COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
 */

/*  11    10/05/98 7:43p Peter */
/*  3DNow!(tm) happiness everywhere */
/*   */
/*  10    10/03/98 3:35p Peter */
/*  First pass at dynamic detection of 3DNow!(tm) */
/*   */
/*  9     9/21/98 5:42p Dow */
/*  3DNow! (tm) mods */
/*   */
/*  8     7/24/98 1:40p Hohn */
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

#if GLIDE_PACKED_RGB
	.type	bias0,@object
	.size	bias0,4
bias0:	.int	0
	.type	bias1,@object
	.size	bias1,4
bias1:	.int	0
#endif

/*  Ugly, but seems to workaround the problem with locally defined */
/*  data segment globals not getting relocated properly when using */
/*  djgpp. */

#define zArea One+0x04

/*  Definitions of cvg regs and glide root structures. */
#include "fxgasm.h"

/*  Arguments (STKOFF = 16 from 4 pushes) */
#define STKOFF 16
#define _va 4 + STKOFF
#define _vb 8 + STKOFF
#define _vc 12 + STKOFF

#ifdef GL_AMD3D	
#define PROC_TYPE(arg) _trisetup_3DNow_##arg
#define END_PROC_TYPE(arg) .L_END_trisetup_3Dnow_##arg	
#else
#define PROC_TYPE(arg) _trisetup_Default_##arg		
#define END_PROC_TYPE(arg) .L_END_Default_3Dnow_##arg	
#endif

/* -------------------------------------------------------------------------- */
.text


.align 32

.globl PROC_TYPE(cull)
.type PROC_TYPE(cull),@function
PROC_TYPE(cull):

#define LOCAL(arg) .L_cull_##arg
	
#define GLIDE_CULLING 1
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef cull
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING

#undef LOCAL

END_PROC_TYPE(cull):
.size PROC_TYPE(cull),END_PROC_TYPE(cull)-PROC_TYPE(cull)

.align 32

#if GLIDE_PACKED_RGB
.globl PROC_TYPE(cull_rgb)
.type PROC_TYPE(cull_rgb),@function
PROC_TYPE(cull_rgb):

#define LOCAL(arg) .L_cull_rgb_##arg
	
#define GLIDE_CULLING 1
#define GLIDE_PACK_RGB 1
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING

#undef LOCAL

END_PROC_TYPE(cull_rgb):
.size PROC_TYPE(cull_rgb),END_PROC_TYPE(cull_rgb)-PROC_TYPE(cull_rgb)

.align 32

.globl PROC_TYPE(cull_argb)
.type PROC_TYPE(cull_argb),@function
PROC_TYPE(cull_argb):

#define LOCAL(arg) .L_cull_argb_##arg
	
#define GLIDE_CULLING 1
#define GLIDE_PACK_RGB 1
#define GLIDE_PACK_ALPHA 1
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING

END_PROC_TYPE(cull_argb):
.size PROC_TYPE(cull_argb),END_PROC_TYPE(cull_argb)-PROC_TYPE(cull_argb)
#endif	/*  GLIDE_PACKED_RGB */

.align 32

.globl PROC_TYPE(Default)
.type PROC_TYPE(Default),@function
PROC_TYPE(Default):

#define LOCAL(arg) .L_Default_##arg
	
#define GLIDE_CULLING 0
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING

#undef LOCAL

END_PROC_TYPE(Default):
.size PROC_TYPE(Default),END_PROC_TYPE(Default)-PROC_TYPE(Default)

.align 32

#if GLIDE_PACKED_RGB
.globl PROC_TYPE(rgb)
.type PROC_TYPE(rgb),@function
PROC_TYPE(rgb):

#define LOCAL(arg) .L_rgb_##arg
	
#define GLIDE_CULLING 0
#define GLIDE_PACK_RGB 1
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING

#undef LOCAL

END_PROC_TYPE(rgb):
.size PROC_TYPE(rgb),END_PROC_TYPE(rgb)-PROC_TYPE(rgb)

.align 32

.globl PROC_TYPE(argb)
.type PROC_TYPE(argb),@function
PROC_TYPE(argb):

#define LOCAL(arg) .L_argb_##arg
	
#define GLIDE_CULLING 0
#define GLIDE_PACK_RGB 1
#define GLIDE_PACK_ALPHA 1
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING

#undef LOCAL

END_PROC_TYPE(argb):
.size PROC_TYPE(argb),END_PROC_TYPE(argb)-PROC_TYPE(argb)
#endif	/*  GLIDE_PACKED_RGB */



.END

