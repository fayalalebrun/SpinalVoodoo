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
 *  Revision 1.4.2.2  2003/06/29 19:00:36  guillemj
 *  Fixed nested comments warnings.
 *  Removed some trailing spaces.
 *
 *  Revision 1.4.2.1  2003/05/05 07:12:47  dborca
 *  no message
 *
 *  Revision 1.4  2002/04/09 22:21:43  alanh
 *  AMD 3DNow fixes
 *
 *  Revision 1.3  2000/10/20 19:07:00  billwhite
 *  1.) Added changes for AMD 3DNow instructions in h5/glide3 and
 *      h3/glide3.
 *  2.) Fixed a typographical error in configure.in.  The default
 *      texture utility library is texus2, not texus.
 *
 *  Revision 1.2  2000/02/15 22:35:59  joseph
 *  Changes to support FreeBSD (patch submitted by Doug Rabson)
 *
 *  Revision 1.1.1.1  1999/11/24 21:44:58  joseph
 *  Initial checkin for SourceForge
 *
 */
/*
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

#ifdef GL_AMD3D

/* --------------------------------------------------------------------------- */
/*  start 3DNow! version */
/* --------------------------------------------------------------------------- */

.file "xdraw2.inc.S"


.macro GR_FIFO_WRITE __addr __offset __data
	mov \__data , \__offset(\__addr)
.endm	/*  GR_FIFO_WRITE */


.macro WRITE_MM1_FIFO_ALIGNED __offset
	movq %mm1 , \__offset(%ebp)	/*  store current param | previous param */
.endm	/*  WRITE_MM1_FIFO_ALIGNED */


.macro WRITE_MM1LOW_FIFO 
	movd %mm1 , (%ebp)	/*  store current param | previous param */
.endm	/*  WRITE_MM1LOW_FIFO */


#if 0
/* These are defined in xdraw2.S */
/*  Arguments (STKOFF = 16 from 4 pushes) */
#define STKOFF 16
#define _gc 4 + STKOFF
#define _va 8 + STKOFF
#define _vb 12 + STKOFF
#define _vc 16 + STKOFF
#endif


#define gc %edi	/*  points to graphics context */
#define fifo %ebp	/*  points to fifo entries */
#define tempVal %esi

#if GLIDE_CLIP_COORDS
/*  NB:  Currently, the 3DNow!(tm) clip coordinate stuff */
/*       thunks through to the grDrawTriangles functions */
/*       which has already been specialized for 3DNow!(tm). */
/*       This means that we should never get here. */
#ifdef GLIDE_DEBUG
	xor %eax , %eax
	mov %eax , (%eax)
#endif
#else
#if GLIDE_CULLING
#define fa %eax	/*  vtx a from caller */
#define fb %ebx	/*  vtx b from caller */
#define fc %ecx	/*  vtx c from caller */
#define cull %edx	/*  cull mode */
#define intArea %ecx	/*  area temp storage */

/*  Prologue stuff */
	push %edi	/*  save caller's register variable */
	push %esi	/*  save caller's register variable */
	push %ebx	/*  save caller's register variable  */

	mov _gc-4(%esp) , gc
	mov _vb-4(%esp) , fb	/*  get base address of vertex B */
	push %ebp	/*  save frame pointer */

#if GLIDE_CLIP_COORDS
	mov CoordinateSpace(gc) , %esi	/*  which coord. space (clip/window)? */
#endif

#if GLIDE_VALIDATE_STATE
	mov invalid(gc) , %ebp	/*  state validated? */
#endif

#if GLIDE_CLIP_COORDS
	lea _va(%esp) , %eax	/*  pointer to vertex pointers */
	test %esi , %esi	/*  window coordinates ? */

	jz LOCAL(win_coordinates)	/*  yup */
	push %eax	/*  pointer to vertex pointers */

	push $3	/*  3 vertices */
	push $1	/*  mode = grDrawVertexArray */

	call _grDrawTriangles	/*  draw the triangle in clip coordinate space */
	add $12 , %esp

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD arguments off stack */

LOCAL(win_coordinates):
#endif	/*  GLIDE_CLIP_COORDS */

#if GLIDE_VALIDATE_STATE
	test %ebp , %ebp	/*  does state need validation? */
	jz LOCAL(no_validation)	/*  valid, don't need to validate */

	call EXTRN(_grValidateState)	/*  validate state */

LOCAL(no_validation):

#endif	/*  GLIDE_VALIDATE_STATE  */

	mov cull_mode(gc) , cull	/*  get cull mode */
	nop 	/*  filler */

	mov _vc(%esp) , fc	/*  get base address of vertex C */
	nop 	/*  filler */

ALIGN(4)

	femms 	/*  will use AMD3D, clear FPU/MMX registers */

	test cull , cull	/*  culling enabled ? */
	mov curTriSize(gc) , tempVal

/*  Cull Check */

	jz LOCAL(nocull)	/*  nope, no culling */
	mov _va(%esp) , fa	/*  get base address of vertex A */

	movq X(fc) , %mm2	/*  yc | xc */
	shl $31 , cull	/*  culltest << 31 */

	movq X(fb) , %mm1	/*  yb | xb */
	add $4 , tempVal	/*  space required in fifo */

	movq X(fa) , %mm0	/*  ya | xa */
	mov fifoRoom(gc) , %ebx	/*  space available in fifo */

/*  Area_Computation */

	pfsubr %mm1 , %mm2	/*  dyBC | dxBC */
	pfsub %mm1 , %mm0	/*  dyAB | dxAB */

	movq %mm2 , %mm5	/*  dyBC | dxBC */
	punpckhdq %mm2 , %mm2	/*  dyBC | dyBC */

	movq %mm0 , %mm4	/*  dyAB | dxAB */
	punpckhdq %mm0 , %mm0	/*  dyAB | dyAB */

	pfmul %mm0 , %mm5	/*  dyAB*dxBC */
	pfmul %mm2 , %mm4	/*  dxAB*dyBC */

	pfsub %mm5 , %mm4	/*  dxAB*dyBC - dxBC*dyAB */

	movd %mm4 , intArea	/*  vectored ! */

/*  Zero Area Triangle Check */

	test $0x7fffffff , intArea	/*  if ((j & 0x7FFFFFFF) == 0) */
	jz LOCAL(__cullFail)	/*  area zero, triangle culled */

	xor cull , intArea	/*  if (j ^ (culltest << 31)) */
	jge LOCAL(__cullFail)	/*  triangle facing away from viewer, culled */

	cmp tempVal , %ebx	/*  fifo space required >= space available ? */
	jge LOCAL(__triBegin)	/*  yup, push out triangle data to Voodoo */

	pushl $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push tempVal	/*  fifo space required */
	call EXTRN(_grCommandTransportMakeRoom)	/*  note: updates fifoPtr */
	add $12, %esp

	jmp LOCAL(__triBegin)	/*  merge back with short path */

/*  culling disabled */

ALIGN(4)

LOCAL(nocull):
/*  Check to make sure that we have enough room for */
/*  the complete triangle packet. */

	add $4 , tempVal	/*  fifo space needed */
	mov fifoRoom(gc) , %ebx	/*  fifo space available */

	cmp tempVal , %ebx	/*  fifo spce available >= space needed ? */
	jge LOCAL(__triBegin)	/*  yup, ready to draw triangle */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push tempVal	/*  fifo space needed */
	call EXTRN(_grCommandTransportMakeRoom)	/*  note: updates fifoPtr */
	add $12, %esp
#else	/*  !GLIDE_CULLING */

/*  Prologue stuff */

	push %edi	/*  save caller's register variable */
	push %esi	/*  save caller's register variable  */

	mov _gc-8(%esp) , gc	/*  gc on stack (NOT!!! in edx) from caller */
	push %ebx	/*  save caller's register variable */

	push %ebp	/*  save frame pointer */

#if GLIDE_CLIP_COORDS
	mov CoordinateSpace(gc) , %edx	/*  window coordinates or clip coordinates ? */
#endif

#if GLIDE_VALIDATE_STATE
	mov invalid(gc) , %ebp	/*  state validated? */
#endif

#if GLIDE_CLIP_COORDS
	lea _va(%esp) , %eax	/*  pointer to vertex pointers */
	test %edx , %edx	/*  window coordinates ? */

	jz LOCAL(win_coordinates)	/*  yup */
	push %eax	/*  pointer to vertex pointers */

	push $3	/*  3 vertices */
	push $1	/*  mode = grDrawVertexArray */

	call _grDrawTriangles	/*  draw the triangle in coordinate space */
	add $12, %esp

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD arguments off stack */

ALIGN(4)
LOCAL(win_coordinates):

#endif

#if GLIDE_VALIDATE_STATE
	test %ebp , %ebp	/*  does state need validation? */
	jz LOCAL(no_validation)	/*  valid, don't need to validate */

	call EXTRN(_grValidateState)	/*  validate state */

LOCAL(no_validation):

#endif

	mov curTriSize(gc) , tempVal	/*  data for whole triangle in bytes */
	add $4 , tempVal	/*  fifo space needed (include 4-byte header) */
	mov fifoRoom(gc) , %ebx	/*  fifo space available */

	femms 	/*  will use AMD3D, clear FPU/MMX registers */

	cmp tempVal , %ebx	/*  fifo spce available >= space needed ? */
	jge LOCAL(__triBegin)	/*  yup, ready to draw triangle */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push tempVal	/*  fifo space needed */
	call EXTRN(_grCommandTransportMakeRoom)	/*  note: updates fifoPtr */
	add $12, %esp
	
	jmp LOCAL(__triBegin)	/*  large distance due to alignment */
#endif	/*  GLIDE_CULLING */


#define dlp %ebx	/*  points to dataList structure */
#define dlpstrt %ecx	/*  points to begin of dataList structure */
#define vertex %edx	/*  the current vertex */

ALIGN(4)

LOCAL(__triBegin):
	mov triPacketHdr(gc) , %eax	/*  Packet 3 header */
	lea tsuDataList(gc) , dlp	/*  Reset the dataList */

	mov fifoPtr(gc) , fifo	/*  Fetch Fifo Ptr */
	mov _va(%esp) , vertex	/*  Current vertex = A */

	mov dlp , dlpstrt	/*  save pointer to start of dataList */
	test $4 , fifo	/*  is fifo pointer qword aligned ? */

	jz LOCAL(__fifo_aligned)	/*  yes, it is qword aligned */
	movq X(vertex) , %mm1	/*  y | x */

	GR_FIFO_WRITE fifo , 0 , %eax	/*  write header to fifo now qword aligned */
	add $12 , fifo	/*  fifoPtr += 3*sizeof(FxU32) */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write y | x */

/*  here: "write buffer" empty */

	mov (dlp) , %eax	/*  Get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  at end of list ? */
	jz LOCAL(__paramLoopDoneWBzero1)	/*  yes, "write buffer" empty */

LOCAL(__paramLoop1a):
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz LOCAL(__paramLoopDoneWBone1)	/*  exit, write buffer contains one DWORD */

	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp += 2 */

	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write current param | previous param */
	jnz LOCAL(__paramLoop1a)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBzero1)	/*  write buffer empty */

LOCAL(__fifo_aligned):
	movd X(vertex) , %mm2	/*  y | x of vertex A */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	movd triPacketHdr(gc) , %mm1	/*  Packet 3 header */
	punpckldq %mm2 , %mm1	/*  x | header */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write x | header */
	movd Y(vertex) , %mm1	/*  0 | y of vertex A */

	mov (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz LOCAL(__paramLoopDoneWBone1)	/*  yes, "write buffer" has y data */

LOCAL(__paramLoop1b):
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write current param | previous param */
	jz LOCAL(__paramLoopDoneWBzero1)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop1b)	/*  nope, copy next parameter */

LOCAL(__paramLoopDoneWBone1):

/*  here: "write buffer" has one DWORD left over from vertex A */

	mov dlpstrt , dlp	/*  reset the dataList */
	mov _vb(%esp) , vertex	/*  Current vertex = B */

	movd X(vertex) , %mm2	/*  0 | x if vertex B */
	punpckldq %mm2 , %mm1	/*  x | old param */

	WRITE_MM1_FIFO_ALIGNED 0	/*  PCI write: x | old param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	movd Y(vertex) , %mm1	/*  0 | y of vertex B */

	mov (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz LOCAL(__paramLoopDoneWBone2)	/*  yes, "write buffer" has y data */

LOCAL(__paramLoop2b):
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write current param | previous param */
	jz LOCAL(__paramLoopDoneWBzero2)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop2b)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBone2)	/*  write buffer contains one DWORD */

LOCAL(__paramLoopDoneWBzero1):

	mov _vb(%esp) , vertex	/*  Current vertex = B */
	mov dlpstrt , dlp	/*  Reset the dataList */

	movq X(vertex) , %mm1	/*  y | x of vertex B */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write y | x of vertex B */
	nop 	/*  filler */

/*  here: "write buffer" empty */

	mov (dlp) , %eax	/*  Get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  at end of list ? */
	jz LOCAL(__paramLoopDoneWBzero2)	/*  yes, "write buffer" empty */

LOCAL(__paramLoop2a):
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  offset = *(dlp + 1) */

	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
	jz LOCAL(__paramLoopDoneWBone2)	/*  exit, write buffer contains one DWORD */

	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp += 2 */

	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write current param | previous param */
	jnz LOCAL(__paramLoop2a)	/*  nope, copy next parameter */

LOCAL(__paramLoopDoneWBzero2):

	mov _vc(%esp) , vertex	/*  Current vertex = C */
	mov dlpstrt , dlp	/*  Reset the dataList */

	movq X(vertex) , %mm1	/*  y | x of vertex C */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write y | x of vertex C */

/*  here: "write buffer" empty */

	mov (dlp) , %eax	/*  Get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  at end of list ? */
	jz LOCAL(__paramLoopDoneWBzero3)	/*  yes, "write buffer" empty */

LOCAL(__paramLoop3a):
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz LOCAL(__paramLoopDoneWBone3)	/*  exit, write buffer contains one DWORD */

	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp += 2 */

	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write current param | previous param */
	jnz LOCAL(__paramLoop3a)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBzero3)	/*  write buffer empty */

LOCAL(__paramLoopDoneWBone2):

/*  here: "write buffer" has one DWORD left over from vertex B */

	mov _vc(%esp) , vertex	/*  Current vertex = C */
	mov dlpstrt , dlp	/*  reset the dataList */

	movd X(vertex) , %mm2	/*  0 | x if vertex C */
	punpckldq %mm2 , %mm1	/*  x | old param */

	WRITE_MM1_FIFO_ALIGNED 0	/*  PCI write: x | old param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	movd Y(vertex) , %mm1	/*  0 | y of vertex C */
	nop 	/*  filler */

	mov (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz LOCAL(__paramLoopDoneWBone3)	/*  yes, "write buffer" has y data */

LOCAL(__paramLoop3b):
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED -8	/*  PCI write current param | previous param */
	jz LOCAL(__paramLoopDoneWBzero3)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop3b)	/*  nope, copy next parameter */

LOCAL(__paramLoopDoneWBone3):

/*  "write buffer" contains one DWORD that needs to be flushed */

	WRITE_MM1LOW_FIFO 	/*   */
	add $4 , fifo

LOCAL(__paramLoopDoneWBzero3):

/*  Update gc->fifoPtr and gc->fifoRoom */

	mov trisDrawn(gc) , %ecx	/*  _GlideRoot.stats.trisDrawn */
	mov fifo , %eax	/*  new fifo pointer */

	mov fifoPtr(gc) , %ebx	/*  old fifo pointer */
	mov fifo , fifoPtr(gc)	/*  save new fifo pointer */

	mov fifoRoom(gc) , %edx	/*  old fifo space available */
	inc %ecx	/*  _GlideRoot.stats.trisDrawn++ */

	mov trisProcessed(gc) , %esi	/*  _GlideRoot.stats.trisProcessed */
	sub %ebx , %eax	/*  new fifo ptr - old fifo ptr = additional fifo space used */

	mov %ecx , trisDrawn(gc)
	sub %eax , %edx	/*  new fifo space available */

	mov $0x1 , %eax	/*  return value = triangle drawn */
	mov %edx , fifoRoom(gc)	/*  new fifo space available */

/*  Restore trashed registers     */

LOCAL(__triDone_nocull):

	inc %esi	/*  _GlideRoot.stats.trisProcessed++ */
	pop %ebp	/*  restore frame pointer */

	mov %esi , trisProcessed(gc)
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	femms 	/*  no more AMD3D code, clear FPU/MMX regs */

	ret	/*  return to caller */


#if GLIDE_CULLING
LOCAL(__cullFail):
	mov trisProcessed(gc) , %esi	/*  triangles processed so far */
	xor %eax , %eax	/*  return value = triangle not drawn */

	femms 	/*  no more AMD3D code, clear FPU/MMX regs */

LOCAL(__triDone_cull):
/*  Restore trashed registers     */
	inc %esi	/*  _GlideRoot.stats.trisProcessed++     */
	pop %ebp	/*  restore frame pointer */

	mov %esi , trisProcessed(gc)
	pop %ebx

	pop %esi
	pop %edi

	ret
#endif	/*  GLIDE_CULLING */
#endif	/*  !GLIDE_CLIP_COORDS */

/* --------------------------------------------------------------------------- */
/*  end 3DNow! version */
/* --------------------------------------------------------------------------- */

#endif	/*  GL_AMD3D */


#ifndef GL_AMD3D

/* --------------------------------------------------------------------------- */
/*  original code */
/* --------------------------------------------------------------------------- */

.file "xdraw2.inc.S"

/*  Ugly, but seems to workaround the problem with locally defined */
/*  data segment globals not getting relocated properly when using */
/*  djgpp. */

#define zArea One+0x04
#define zdxAB One+0x08
#define zdxBC One+0x0c
#define zdyAB One+0x10
#define zdyBC One+0x14
#define zculltest One+0x18

#define gc %esi	/*  points to graphics context */

#if GLIDE_CLIP_COORDS
/*  NB:  We should never hit this because the proc is */
/*       either specialized or we thunk through to */
/*       grDrawTriangles for the clipping etc. */
#ifdef GLIDE_DEBUG
	xor %eax , %eax
	mov %eax , (%eax)
#endif
#else
/*  Prologue stuff */
	push %esi
	push %edi

	push %ebx
	push %ebp

	mov _gc(%esp) , gc
	
/*    call    getThreadValueSLOW */
/*    mov     gc, eax     */

#if 0
/*  TRICKY STUFF HERE: */
/*  The following code would be expressed like this if MASM didn't suck: */
/*  mov eax, DWORD PTR fs:[18h] */
/*  Remember, gc == esi */
	db $0x064
	db $0x0a1
	db $0x018
	db $00

	db $0x00
	db $0x00

	addl _GlideRoot+tlsOffset , %eax
	mov (%eax) , gc
#else
/*	mov %edx , gc */
#endif
#ifdef GLIDE_ALT_TAB
	test gc , gc
	je LOCAL(__triDone)
	mov windowed(gc) , %edx
	test $1 , %edx
	jnz LOCAL(pastContextTest)	/*   */
	movl lostContext(gc) , %edx
	mov (%edx) , %ebx
	test $1 , %ebx
	jnz LOCAL(__triDone)
ALIGN(4)
#endif
	
LOCAL(pastContextTest):
#if GLIDE_VALIDATE_STATE
/*      GR_FLUSH_STATE() */
	movl invalid(gc) , %edx
	test %edx , %edx
	je LOCAL(no_validation)
	call EXTRN(_grValidateState)
ALIGN(4)
#endif
	
LOCAL(no_validation):
#if GLIDE_CULLING
#define fa %eax	/*  vtx a from caller */
#define fb %ebx	/*  vtx b from caller */
#define fc %ecx	/*  vtx c from caller */

#define cull %edx
#define intArea %ebp	/*  temp Y storage */

/*  Pre-load the current culling mode before all of the */
/*  floating point area stuff.     */
	mov _va(%esp) , fa
	mov _vb(%esp) , fb

	mov cull_mode(gc) , cull
	mov _vc(%esp) , fc

	test cull , cull
	jz LOCAL(nocull)

	shl $31 , cull	/*  culltest << 31     */

LOCAL(Area_Computation):
/*  47-3 */
/*  jmp ret_pop0f */
	flds X(fa)	/*   xa */
	fsubs X(fb)	/*   dxAB */
	flds X(fb)	/*   |    xb */
	fsubs X(fc)	/*   |    dxBC */
	flds Y(fb)	/*   |    |    yb */
	fsubs Y(fc)	/*   |    |    dyBC */
	flds Y(fa)	/*   |    |    |    ya */
	fsubs Y(fb)	/*   |    |    |    dyAB */
	fld %st(3)	/*   |    |    |    |    dxAB */
	fmul %st(2) , %st	/*   |    |    |    |    t0         t0=dxAB*dyBC */
	fld %st(3)	/*   |    |    |    |    |    dxBC */
	fmul %st(2) , %st	/*   |    |    |    |    |    t1    t1=dxBC*dyAB */
	fsubrp %st , %st(1)	/*   |    |    |    |    area */
	fsts zArea	/*   |    |    |    |    area */

/*  Pop temp things from the sw culling off the fp stack */
	fstp %st(0)	/*  4 */
	fstp %st(0)	/*  3 */
	fstp %st(0)	/*  2 */
	fstp %st(0)	/*  1 */
	fstp %st(0)	/*  0     */

	mov zArea , intArea	/*  j = *(long *)&area */
	xor %eax , %eax	/*  Clear the return value (0 == culled) */

/*  Zero Area Triangle Check */
	and $0x7fffffff , intArea	/*  if ((j & 0x7FFFFFFF) == 0) */
	jz LOCAL(__triDone)

/*  Triangle area check vs culling mode */
	mov zArea , intArea	/*  reload area just in case we're culling */
	xor cull , intArea	/*  if (j ^ (culltest << 31)) */

	jge LOCAL(__triDone)
ALIGN(4)
#endif	/*  GLIDE_CULLING     */

LOCAL(nocull):
/*  Check to make sure that we have enough room for */
/*  the complete triangle packet. */
	mov curTriSize(gc) , %eax
	mov fifoRoom(gc) , %ebx

	add $4 , %eax
	cmp %eax , %ebx

	jge LOCAL(__triBegin)

	pushl $__LINE__
	push $0x0

	push %eax
	call EXTRN(_grCommandTransportMakeRoom)
	add $12, %esp

/*  Send triangle parameters */

#define dlp %ebx	/*  points to dataList structure */
#define fifo %ebp	/*  points to next entry in fifo */
#define vertex %edx	/*  the current vertex */
#define vOffset %ecx	/*  Current vertex offset */

#define packCol %edi
#define tempVal %edi

.macro GR_FIFO_WRITE __addr __offset __data
	mov \__data , \__offset(\__addr)
.endm	/*  GR_FIFO_WRITE */

ALIGN(4)
LOCAL(__triBegin):
	mov fifoPtr(gc) , fifo	/*  Fetch Fifo Ptr */
	mov $_va-STKOFF , vOffset	/*  Starting vertex */

	mov triPacketHdr(gc) , %eax	/*  Packet 3 header */
	nop 

	GR_FIFO_WRITE fifo , 0 , %eax	/*  Write packet header to fifo     */
	add $4 , fifo	/*  Advance fifo for hdr & x/y coordinate */

ALIGN(4)
LOCAL(__vertexStart):
	mov STKOFF(%esp, vOffset) , vertex	/*  Current vertex */
	add $8 , fifo

	nop 	/*  Avoid p5 agi w/ load of vertex ptr */
	nop 

	movl (vertex) , %eax	/*  X */
	lea tsuDataList(gc) , dlp	/*  Reset the dataList */

	GR_FIFO_WRITE fifo , -8 , %eax	/*  PCI write X */
	movl 4(vertex) , %eax	/*  Y  */

	xor packCol , packCol	/*  Clear packed color */
	GR_FIFO_WRITE fifo , -4 , %eax	/*  PCI write Y */

LOCAL(__doParams):
	movl (dlp) , %eax	/*  Get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  Are we done? */
	je LOCAL(__nextVertex)

/*  Not using align directive here because it sometimes */
/*  introduces an agi for the eax use below. */
	nop 
	nop 

LOCAL(__paramLoop):
	movl (%eax,vertex) , tempVal	/*  Get the parameter from the vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxU32) */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  Are we done? */
	GR_FIFO_WRITE fifo , -4 , tempVal	/*  *fifoPtr = data */

	jne LOCAL(__paramLoop)

ALIGN(4)
LOCAL(__nextVertex):
/*  On to the next vertex */
	add $4 , vOffset

	cmp $_vc-STKOFF+4 , vOffset	/*  Offset of one past last vertex? */
	jne LOCAL(__vertexStart)

/*  Update gc->fifoPtr and gc->fifoRoom */
	mov fifo , %eax
	mov fifoPtr(gc) , %ebx

	mov fifo , fifoPtr(gc)
	sub %ebx , %eax

	mov trisDrawn(gc) , %ebx	/*  _GlideRoot.stats.trisDrawn++     */
	sub %eax , fifoRoom(gc)

	add $1 , %ebx
	mov %ebx , trisDrawn(gc)

/*  return 1 (triangle drawn)     */
	mov $0x1 , %eax

LOCAL(__triDone):
/*  Restore trashed registers */
	mov trisProcessed(gc) , %ecx
	pop %ebp

	add $1 , %ecx	/*  _GlideRoot.stats.trisProcessed++     */
	pop %ebx

	pop %edi
	mov %ecx , trisProcessed(gc)

	pop %esi
	ret
#endif	/*  !GLIDE_CLIP_COOR */
#endif	/*  !GL_AMD3D */
