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

/*
 *  Revision 1.2.8.5  2004/10/05 14:39:55  dborca
 *  removed detritus
 *
 *  Revision 1.2.8.4  2003/06/29 19:00:36  guillemj
 *  Fixed nested comments warnings.
 *  Removed some trailing spaces.
 *
 *  Revision 1.2.8.3  2003/06/29 18:43:27  guillemj
 *  Fix compilation warnings.
 *
 *  Revision 1.2.8.2  2003/06/19 08:38:16  dborca
 *  fixes: DOS gamma, modetabl.h timings (avoid shimmering),
 *  Napalm memory probing, more DJGPP MMX specializations,
 *  new 32bit texture files, Linux build valid again, etc.
 *
 *  Revision 1.2.8.1  2003/05/05 07:12:47  dborca
 *  no message
 *
 *  Revision 1.2  2000/02/15 22:35:59  joseph
 *  Changes to support FreeBSD (patch submitted by Doug Rabson)
 *
 *  Revision 1.1.1.1  1999/11/24 21:44:58  joseph
 *  Initial checkin for SourceForge
 *
 ****************
 *  4     4/06/99 3:54p Dow
 *  Alt tab again.
 *
 *  12    4/05/99 11:34a Atai
 *  added GLIDE_ALT_TAB for xdraw2.inc to query context in the retail build
 *
 *  11    3/19/99 11:26a Peter
 *  expose direct fifo for gl
 *
 *  10    10/14/98 12:05p Peter
 *  fixed my f*cked up assumption about non-volatile regs
 *
 *  9     10/12/98 9:51a Peter
 *  dynamic 3DNow!(tm)
 *
 *  8     9/24/98 11:17a Dow
 *  AMD 3DNow! (tm) mods
 *
 *  7     8/30/98 1:34p Dow
 *  State & other optimizations
 *
 *  6     8/29/98 8:12p Dow
 *  Clip optimization
 *
 *  5     7/01/98 8:41a Jdt
 *  removed gc arg from trisetup funcs
 *
 *  4     8/03/98 6:36a Jdt
 *  Add GC to trisetup arglist
 *
 *  3     6/09/98 11:59a Atai
 *  1. update glide header
 *  2. fix cull mode
 *  3. fix tri stats
 *
 *  8     5/18/98 3:21p Peter
 *  dynamic culling changes
 *
 *  6     1/15/98 1:12p Peter
 *  dispatch w/o packing
 *
 *  5     11/06/97 3:47p Peter
 *
 *  4     11/04/97 5:04p Peter
 *  cataclysm part deux
 *
 *  3     11/01/97 10:01a Peter
 *  tri dispatch stuff
 *
 *  2     10/30/97 6:53p Peter
 *  first real cut at tri asm
 *
 *  1     10/30/97 4:29p Peter
 *  asm tri code
 *
 *  2     7/07/97 2:14p Jdt
 *  assembly now on par with C code.
 *
 *  1     7/07/97 8:37a Jdt
 *  B4 Chip field fix.
 *
 */


.file "xdraw2.asm"

#include "assyntax.h"

/*  Definitions of cvg regs and glide root structures. */
#include "fxgasm.h"


.data
	.type	One,@object
	.size	One,4
One:	.int	0x3f800000
	.type	Area,@object
	.size	Area,4
Area:	.int	0

.data
.section	.rodata
	.type	T2003,@object
	.size	T2003,4
T2003:	.int	0x46400000	/*  12288 */
	.type	T2005,@object
	.size	T2005,4
T2005:	.int	0x3f800000	/*  1 */
	.type	T2006,@object
	.size	T2006,4
T2006:	.int	0x43800000	/*  256 */

/*  Arguments (STKOFF = 16 from 4 pushes) */
#define STKOFF 16
#define _gc 4  + STKOFF
#define _va 8 + STKOFF
#define _vb 12 + STKOFF
#define _vc 16 + STKOFF

/*  coordinate offsets into vertex. */
/*  NB:  These are constants and are not */
/*       user settable like the rest of the */
/*       parameter offset. Weird. */
#define X 0
#define Y 4

#ifdef GL_AMD3D
#define PROC_TYPE(arg) _trisetup_3DNow_##arg
#define END_PROC_TYPE(arg) .L_END_trisetup_3Dnow_##arg
#else
#define PROC_TYPE(arg) _trisetup_Default_##arg
#define END_PROC_TYPE(arg) .L_END_trisetup_Default_##arg
#endif

/*  enables/disables trisProcessed and trisDrawn counters */
#define STATS 1

/*  offsets into vertex struct */
#define X 0
#define Y 4

/*  NB:  All of the base triangle procs expect to have the gc */
/*       passed from the caller in edx so that we can avoid */
/*       the agi from the far pointer. Screw w/ this at your */
/*       own peril. */

/*       YOU HAVE BEEN WARNED */

/* -------------------------------------------------------------------------- */

.text


ALIGN(32)
GLOBL(PROC_TYPE(clip_nocull_invalid))

#define LOCAL(arg) L_clip_nocull_invalid##arg
	
#define GLIDE_VALIDATE_STATE 1
#define GLIDE_CLIP_COORDS 1
#define GLIDE_CULLING 0
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING
#undef GLIDE_CLIP_COORDS
#undef GLIDE_VALIDATE_STATE

#undef LOCAL

END_PROC_TYPE(clip_nocull_invalid):
.size PROC_TYPE(clip_nocull_invalid),END_PROC_TYPE(clip_nocull_invalid)-PROC_TYPE(clip_nocull_invalid)

ALIGN(32)
GLOBL(PROC_TYPE(clip_cull_invalid))

#define LOCL(arg) .L_clip_cull_invalid_##arg
	
#define GLIDE_VALIDATE_STATE 1
#define GLIDE_CLIP_COORDS 1
#define GLIDE_CULLING 1
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING
#undef GLIDE_CLIP_COORDS
#undef GLIDE_VALIDATE_STATE

#undef LOCAL

END_PROC_TYPE(clip_cull_invalid):
.size PROC_TYPE(clip_cull_invalid),END_PROC_TYPE(clip_cull_invalid)-PROC_TYPE(clip_cull_invalid)

ALIGN(32)
GLOBL(PROC_TYPE(clip_cull_valid))

#define LOCAL(arg) L_clip_cull_valid_##arg
	
#define GLIDE_VALIDATE_STATE 0
#define GLIDE_CLIP_COORDS 1
#define GLIDE_CULLING 1
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING
#undef GLIDE_CLIP_COORDS
#undef GLIDE_VALIDATE_STATE

#undef LOCAL

END_PROC_TYPE(clip_cull_valid):
.size PROC_TYPE(clip_cull_valid),END_PROC_TYPE(clip_cull_valid)-PROC_TYPE(clip_cull_valid)

ALIGN(32)
GLOBL(PROC_TYPE(clip_nocull_valid))

#define LOCAL(arg) L_clip_nocull_valid_##arg
	
#define GLIDE_VALIDATE_STATE 0
#define GLIDE_CLIP_COORDS 1
#define GLIDE_CULLING 0
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING
#undef GLIDE_CLIP_COORDS
#undef GLIDE_VALIDATE_STATE

#undef LOCAL

END_PROC_TYPE(clip_nocull_valid):
.size PROC_TYPE(clip_nocull_valid),END_PROC_TYPE(clip_nocull_valid)-PROC_TYPE(clip_nocull_valid)

ALIGN(32)
GLOBL(PROC_TYPE(win_nocull_invalid))

#define LOCAL(arg) L_win_nocull_invalid_##arg
	
#define GLIDE_VALIDATE_STATE 1
#define GLIDE_CLIP_COORDS 0
#define GLIDE_CULLING 0
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING
#undef GLIDE_CLIP_COORDS
#undef GLIDE_VALIDATE_STATE

#undef LOCAL

END_PROC_TYPE(win_nocull_invalid):
.size PROC_TYPE(win_nocull_invalid),END_PROC_TYPE(win_nocull_invalid)-PROC_TYPE(win_nocull_invalid)

ALIGN(32)
GLOBL(PROC_TYPE(win_cull_invalid))

#define LOCAL(arg) L_win_cull_invalid_##arg
	
#define GLIDE_VALIDATE_STATE 1
#define GLIDE_CLIP_COORDS 0
#define GLIDE_CULLING 1
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING
#undef GLIDE_CLIP_COORDS
#undef GLIDE_VALIDATE_STATE

#undef LOCAL

END_PROC_TYPE(win_cull_invalid):
.size PROC_TYPE(win_cull_invalid),END_PROC_TYPE(win_cull_invalid)-PROC_TYPE(win_cull_invalid)

ALIGN(32)
GLOBL(PROC_TYPE(win_cull_valid))

#define LOCAL(arg) L_win_cull_valid_##arg
	
#define GLIDE_VALIDATE_STATE 0
#define GLIDE_CLIP_COORDS 0
#define GLIDE_CULLING 1
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING
#undef GLIDE_CLIP_COORDS
#undef GLIDE_VALIDATE_STATE

#undef LOCAL

END_PROC_TYPE(win_cull_valid):
.size PROC_TYPE(win_cull_valid),END_PROC_TYPE(win_cull_valid)-PROC_TYPE(win_cull_valid)

ALIGN(32)
GLOBL(PROC_TYPE(win_nocull_valid))

#define LOCAL(arg) L_win_nocull_valid_##arg
	
#define GLIDE_VALIDATE_STATE 0
#define GLIDE_CLIP_COORDS 0
#define GLIDE_CULLING 0
#define GLIDE_PACK_RGB 0
#define GLIDE_PACK_ALPHA 0
#define GLIDE_GENERIC_SETUP 0
#include "xdraw2.inc.S"
#undef GLIDE_GENERIC_SETUP
#undef GLIDE_PACK_ALPHA
#undef GLIDE_PACK_RGB
#undef GLIDE_CULLING
#undef GLIDE_CLIP_COORDS
#undef GLIDE_VALIDATE_STATE

#undef LOCAL

END_PROC_TYPE(win_nocull_valid):
.size PROC_TYPE(win_nocull_valid),END_PROC_TYPE(win_nocull_valid)-PROC_TYPE(win_nocull_valid)

#ifdef GL_AMD3D
ALIGN(32)
GLOBL(_trisetup_clip_coor_thunk)

#define procPtr eax
#define vPtr ecx
#define gc edx	/*  Current graphics context passed implicitly through edx */

/*  Call through to the gc->curArchProcs.drawTrianglesProc w/o */
/*  adding extra stuff to the stack. I wish we could actually */
/*  do a direct return here w/o too much work. */
	lea _va-STKOFF(%esp) , %vPtr	/*  Get vertex pointer address */
	mov drawTrianglesProc(%gc) , %procPtr	/*  Prefetch drawTriangles proc addr */

	pushl %vPtr	/*  vertex array address */
	pushl $3	/*  3 vertices */

/*  If debugging make sure that we're in clip coordinates */
#ifdef GLIDE_DEBUG
	testl $1, CoordinateSpace(%gc)
	jnz L_trisetup_clip_coor_thunk__clipSpace
	xor %eax , %eax
	mov %eax , (%eax)
L_trisetup_clip_coor_thunk__clipSpace:
#endif	/*  GLIDE_DEBUG */

	pushl $1	/*  mode = grDrawVertexArray */
	call %procPtr	/*  (*gc->curArchProcs.drawTrianglesProc)(grDrawVertexArray, 3, vPtr) */
	add $12, %esp

	ret	/*  pop 3 dwords (vertex addrs) and return     */
L_END__trisetup_clip_coor_thunk:
.size _trisetup_clip_coor_thunk,L_END__trisetup_clip_coor_thunk-_trisetup_clip_coor_thunk

#endif	/*  GL_AMD3D     */

.end

