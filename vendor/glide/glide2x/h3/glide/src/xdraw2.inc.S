/*  -*-asm-*- */
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


.file "xdraw2.inc"

#ifndef GR_FIFO_WRITE_MACRO
#define GR_FIFO_WRITE_MACRO
.MACRO	GR_FIFO_WRITE __addr __offset __data
	mov \__data , \__offset(\__addr)
.ENDM 	/*  GR_FIFO_WRITE */
#endif

#ifdef GL_AMD3D
	

#ifndef MM1_FIFO_MACROS
#define MM1_FIFO_MACROS
.MACRO  WRITE_MM1_FIFO_ALIGNED
	movq %mm1 , (%ebp)	/*  store current param | previous param */
.ENDM 	/*  WRITE_MM1_FIFO_ALIGNED */

.MACRO  WRITE_MM1LOW_FIFO
	movd %mm1 , (%ebp)	/*  store current param | previous param */
.ENDM 	/*  WRITE_MM1LOW_FIFO */
#endif

#define gc %edi	/*  points to graphics context */
#define fifo %ebp	/*  points to fifo entries */
#define tempVal %esi

#if GLIDE_CULLING
#define fa %eax	/*  vtx a from caller */
#define fb %ebx	/*  vtx b from caller */
#define fc %ecx	/*  vtx c from caller */
#define cull %edx	/*  cull mode */
#define intArea %ecx	/*  area temp storage */

/*  Prologue stuff */

/*  This code is only executed when culling is enabled, so we */
/*  don't need to check for GR_CULL_DISABLE */

	push %edi	/*  save caller's register variable */
	mov (_GlideRoot+curGC) , gc	/*  GR_DCL_GC */

	push %esi	/*  save caller's register variable  */
	mov _vc-8(%esp) , fc	/*  get base address of vertex C */

	push %ebx	/*  save caller's register variable  */
	mov _vb-4(%esp) , fb	/*  get base address of vertex B   */

	push %ebp	/*  save frame pointer  */
	mov cull_mode(gc) , cull	/*  get cull mode */

	mov _va(%esp) , fa	/*  get base address of vertex A */
	mov (_GlideRoot+curTriSize) , tempVal

	femms 	/*  will use AMD3D, clear FPU/MMX registers */

/*  Cullcheck */

	movq x(fc) , %mm2	/*  yc | xc */
	shl $31 , cull	/*  culltest << 31 */

	movq x(fb) , %mm1	/*  yb | xb */
	add $4 , tempVal	/*  space required in fifo */

	movq x(fa) , %mm0	/*  ya | xa */
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

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push tempVal	/*  fifo space required */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */

#else	/*  !GLIDE_CULLING */

/*  Prologue stuff */
	push %edi	/*  save caller's register variable */
	mov (_GlideRoot+curGC) , gc	/*  GR_DCL_GC */

	push %esi	/*  save caller's register variable */
	mov (_GlideRoot+curTriSize) , %eax

	push %ebx	/*  save caller's register variable */
	mov fifoRoom(gc) , %ebx	/*  fifo space available */

	push %ebp	/*  save frame pointer */
	add $4 , %eax	/*  fifo space required */

/*  Check to make sure that we have enough room for */
/*  the complete triangle packet. */

	cmp %eax , %ebx	/*  space available >= space required ? */
	jge LOCAL(__triBegin)	/*  yup, start drawing triangle */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %eax	/*  space required in fifo */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */
	mov %eax , %eax	/*  filler */
#endif


#define dlp %ebx	/*  points to dataList structure */
#define dlpstrt %ecx	/*  points to begin of dataList structure */
#define vertex %edx	/*  the current vertex */
#define packCol %esi

LOCAL(__triBegin):
	mov triPacketHdr(gc) , %eax	/*  Packet 3 header */
	lea tsuDataList(gc) , dlp	/*  Reset the dataList */

	mov fifoPtr(gc) , fifo	/*  Fetch Fifo Ptr */
	mov _va(%esp) , vertex	/*  Current vertex = A */

	mov dlp , dlpstrt	/*  save pointer to start of dataList */
	test $4 , fifo	/*  is fifo pointer qword aligned ? */

	jz LOCAL(__fifo_aligned)	/*  yes, it is qword aligned */
	movq x(vertex) , %mm1	/*  y | x */

	GR_FIFO_WRITE fifo , 0 , %eax	/*  write header to fifo now qword aligned */
	add $4 , fifo	/*  advance fifo for hdr now qword aligned */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write y | x */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

#if GLIDE_PACK_RGB
#if GLIDE_PACK_ALPHA
/*  assumes color and alpha values < 256.0 */
	movq r(vertex) , %mm1	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	movd a(vertex) , %mm3	/*  0 | a */
	pf2id %mm1 , %mm1	/*  convert to integer: 000000gg | 000000rr */

	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */
	pf2id %mm3 , %mm3	/*  convert to integer: 00000000 | 000000aa */

	punpcklwd %mm1 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm1	/*  00000000 | 0000gg00 */

	add $8 , dlp	/*  skip data list entry for "a" */
	psllq $24 , %mm3	/*  00000000 | aa000000 */

	por %mm2 , %mm1	/*  00000000 | 00rrggbb */
	por %mm3 , %mm1	/*  00000000 | aarrggbb */
#else	/*  !GLIDE_PACK_ALPHA */
/*  assumes color values < 256.0 */

	movq r(vertex) , %mm1	/*  g | r */
	add $4 , dlp	/*  next data list entry */

	movd b(vertex) , %mm2	/*  0 | b */
	pf2id %mm1 , %mm1	/*  convert to integer: 000000gg | 000000rr */

	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */
	punpcklwd %mm1 , %mm2	/*  00000000 | 00rr00bb */

	psrlq $24 , %mm1	/*  00000000 | 0000gg00 */
	por %mm2 , %mm1	/*  00000000 | 00rrggbb */
#endif	/*  !GLIDE_PACK_ALPHA */

/*  here: one DWORD in "write buffer", RGB(A) */

	movl (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

#if GLIDE_PACK_ALPHA
	cmp $0 , %eax	/*  end of list ? */
#else
	test %eax , %eax	/*  end of list ? */
#endif
	jz LOCAL(__paramLoopDoneWBone1)	/*  yes, one DWORD in "write buffer" */

LOCAL(__paramLoop1a):
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */
	punpckldq %mm2 , %mm1	/*  current param | previous param */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
	jz LOCAL(__paramLoopDoneWBzero1)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop1a)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBone1)	/*  merge back into common stream */
	lea (%esp) , %esp	/*  filler */
#else	/*  ! GLIDE_PACK_RGB */

/*  here: "write buffer" empty */

	movl (dlp) , %eax	/*  Get first offset from the data list */
	test %eax , %eax	/*  at end of list ? */

	lea 4(dlp) , dlp	/*  dlp++ */
	jz LOCAL(__paramLoopDoneWBzero1)	/*  yes, "write buffer" empty */

LOCAL(__paramLoop1a):
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	jz LOCAL(__paramLoopDoneWBone1)	/*  exit, write buffer contains one DWORD */
	movd (%eax,vertex) , %mm2	/*  get next parameter */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	punpckldq %mm2 , %mm1	/*  current param | previous param */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop1a)	/*  nope, copy next parameter */

	mov %esp , %esp	/*  filler */
	jmp LOCAL(__paramLoopDoneWBzero1)	/*  write buffer empty */

#endif	/*  GLIDE_PACK_RGB */

LOCAL(__fifo_aligned):
	movd x(vertex) , %mm2	/*  y | x of vertex A */
	movd triPacketHdr(gc) , %mm1	/*  Packet 3 header */

	punpckldq %mm2 , %mm1	/*  x | header */
	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write x | header */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */
	movd y(vertex) , %mm1	/*  0 | y of vertex A */

#if GLIDE_PACK_RGB
#if GLIDE_PACK_ALPHA
/*  assumes color and alpha values < 256.0 */
	movq r(vertex) , %mm4	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	movd a(vertex) , %mm3	/*  0 | a */
	pf2id %mm4 , %mm4	/*  convert to integer: 000000gg | 000000rr */

	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */
	pf2id %mm3 , %mm3	/*  convert to integer: 00000000 | 000000aa */

	punpcklwd %mm4 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm4	/*  00000000 | 0000gg00 */

	psllq $24 , %mm3	/*  00000000 | aa000000 */
	por %mm2 , %mm4	/*  00000000 | 00rrggbb */

	add $8 , dlp	/*  skip data list entry "a" */
	por %mm3 , %mm4	/*  00000000 | aarrggbb */

#else	/*  !GLIDE_PACK_ALPHA */
/*  assumes color values < 256.0 */

	movq r(vertex) , %mm4	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	pf2id %mm4 , %mm4	/*  convert to integer: 000000gg | 000000rr */
	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */

	punpcklwd %mm4 , %mm2	/*  00000000 | 00rr00bb */
	add $4 , dlp	/*  next data list entry */

	psrlq $24 , %mm4	/*  00000000 | 0000gg00 */
	por %mm2 , %mm4	/*  00000000 | 00rrggbb */
#endif	/*  !GLIDE_PACK_ALPHA */

	punpckldq %mm4 , %mm1	/*  RGB(A) | y */
	movl (dlp) , %eax	/*  get first offset from the data list */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write y | RGB(A) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	add $4 , dlp	/*  dlp++ */
	test %eax , %eax	/*  end of list ? */

	jz LOCAL(__paramLoopDoneWBzero1)	/*  yes, "write buffer" is empty */
	nop 	/*  filler */

LOCAL(__paramLoop1b):
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	jz LOCAL(__paramLoopDoneWBone1)	/*  exit, write buffer contains one DWORD */
	movd (%eax,vertex) , %mm2	/*  get next parameter */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param  */

#if GLIDE_PACK_ALPHA
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */
#else
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */
	nop 
#endif

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop1b)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBzero1)	/*  write buffer empty */

#else	/*  !GLIDE_PACK_RGB */
	movl (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz LOCAL(__paramLoopDoneWBone1)	/*  yes, "write buffer" has y data */

LOCAL(__paramLoop1b):
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	add $4 , dlp	/*  dlp++ */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

#if GLIDE_CULLING
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
#else
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
#endif

	jz LOCAL(__paramLoopDoneWBzero1)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop1b)	/*  nope, copy next parameter */
#endif

LOCAL(__paramLoopDoneWBone1):

/*  here: "write buffer" has one DWORD left over from vertex A */

	mov _vb(%esp) , vertex	/*  Current vertex = B */
	mov dlpstrt , dlp	/*  reset the dataList */

	movd x(vertex) , %mm2	/*  0 | x if vertex B */
	punpckldq %mm2 , %mm1	/*  x | old param */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write: x | old param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	movd y(vertex) , %mm1	/*  0 | y of vertex B */
#if GLIDE_PACK_RGB
#else
	mov %esp , %esp	/*  filler */
#endif

#if GLIDE_PACK_RGB
#if GLIDE_PACK_ALPHA
/*  assumes color and alpha values < 256.0 */
	movq r(vertex) , %mm4	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	movd a(vertex) , %mm3	/*  0 | a */
	pf2id %mm4 , %mm4	/*  convert to integer: 000000gg | 000000rr */

	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */
	pf2id %mm3 , %mm3	/*  convert to integer: 00000000 | 000000aa */

	punpcklwd %mm4 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm4	/*  00000000 | 0000gg00 */

	add $8 , dlp	/*  skip data list entry "a" */
	mov %eax , %eax	/*  filler */

	psllq $24 , %mm3	/*  00000000 | aa000000 */

	por %mm2 , %mm4	/*  00000000 | 00rrggbb */
	por %mm3 , %mm4	/*  00000000 | aarrggbb */

#else	/*  !GLIDE_PACK_ALPHA */
/*  assumes color values < 256.0 */

	movq r(vertex) , %mm4	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	pf2id %mm4 , %mm4	/*  convert to integer: 000000gg | 000000rr */
	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */

	punpcklwd %mm4 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm4	/*  00000000 | 0000gg00 */

	add $4 , dlp	/*  next data list entry */
	por %mm2 , %mm4	/*  00000000 | 00rrggbb */
#endif	/*  !GLIDE_PACK_ALPHA */

	punpckldq %mm4 , %mm1	/*  RGB(A) | y */
	movl (dlp) , %eax	/*  get first offset from the data list */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write y | RGB(A) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	add $4 , dlp	/*  dlp++ */
	test %eax , %eax	/*  end of list ? */

	jz LOCAL(__paramLoopDoneWBzero2)	/*  yes, "write buffer" is empty */
	mov %esp , %esp	/*  filler */

LOCAL(__paramLoop2b):
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	lea 4(dlp) , dlp	/*  dlp++ */

	jz LOCAL(__paramLoopDoneWBone2)	/*  exit, write buffer contains one DWORD */
	movd (%eax,vertex) , %mm2	/*  get next parameter */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param  */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop2b)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBzero2)	/*  write buffer empty */
#else	/*  !GLIDE_PACK_RGB */
	movl (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz LOCAL(__paramLoopDoneWBone2)	/*  yes, "write buffer" has y data */

LOCAL(__paramLoop2b):
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */
	punpckldq %mm2 , %mm1	/*  current param | previous param */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

#if GLIDE_CULLING
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
#else
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
#endif
	jz LOCAL(__paramLoopDoneWBzero2)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop2b)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBone2)	/*  write buffer contains one DWORD */
#endif


LOCAL(__paramLoopDoneWBzero1):

	mov _vb(%esp) , vertex	/*  Current vertex = B */
	mov dlpstrt , dlp	/*  Reset the dataList */

	movq x(vertex) , %mm1	/*  y | x of vertex B */
	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write y | x of vertex B  */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

#if GLIDE_PACK_RGB
#if GLIDE_PACK_ALPHA
/*  assumes color and alpha values < 256.0 */
	movq r(vertex) , %mm1	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	movd a(vertex) , %mm3	/*  0 | a */
	pf2id %mm1 , %mm1	/*  convert to integer: 000000gg | 000000rr */

	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */
	pf2id %mm3 , %mm3	/*  convert to integer: 00000000 | 000000aa */

	punpcklwd %mm1 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm1	/*  00000000 | 0000gg00 */

	psllq $24 , %mm3	/*  00000000 | aa000000 */
	por %mm2 , %mm1	/*  00000000 | 00rrggbb */

	por %mm3 , %mm1	/*  00000000 | aarrggbb */
	add $8 , dlp	/*  skip data list entry "a" */
#else	/*  !GLIDE_PACK_ALPHA */
/*  assumes color values < 256.0 */

	movq r(vertex) , %mm1	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	pf2id %mm1 , %mm1	/*  convert to integer: 000000gg | 000000rr */
	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */

	punpcklwd %mm1 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm1	/*  00000000 | 0000gg00 */

	por %mm2 , %mm1	/*  00000000 | 00rrggbb */
	add $4 , dlp	/*  next data list entry */
#endif	/*  !GLIDE_PACK_ALPHA */

/*  here: one DWORD in "write buffer", RGB(A) */

	movl (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  end of list ? */
	jz LOCAL(__paramLoopDoneWBone2)	/*  yes, one DWORD in "write buffer" */

LOCAL(__paramLoop2a):
	movd (%eax,vertex) , %mm2	/*  get next parameter */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	punpckldq %mm2 , %mm1	/*  current param | previous param */

	add $4 , dlp	/*  dlp++ */
	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	jz LOCAL(__paramLoopDoneWBzero2)	/*  exit, "write buffer" empty */
	movd (%eax,vertex) , %mm1	/*  get next parameter */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop2a)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBone2)	/*  merge back into common stream */
#else	/*  ! GLIDE_PACK_RGB */

/*  here: "write buffer" empty */

	movl (dlp) , %eax	/*  Get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  at end of list ? */
	jz LOCAL(__paramLoopDoneWBzero2)	/*  yes, "write buffer" empty */

LOCAL(__paramLoop2a):
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */
#if GLIDE_CULLING
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
#else
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
#endif

	jz LOCAL(__paramLoopDoneWBone2)	/*  exit, write buffer contains one DWORD */
	movd (%eax,vertex) , %mm2	/*  get next parameter */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	punpckldq %mm2 , %mm1	/*  current param | previous param */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

#if GLIDE_CULLING
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
#else
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
#endif
	jnz LOCAL(__paramLoop2a)	/*  nope, copy next parameter */
#endif	/*  GLIDE_PACK_RGB */


LOCAL(__paramLoopDoneWBzero2):

	mov _vc(%esp) , vertex	/*  Current vertex = C */
	mov dlpstrt , dlp	/*  Reset the dataList */

	movq x(vertex) , %mm1	/*  y | x of vertex C */
	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write y | x of vertex C */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

#if GLIDE_PACK_RGB
#if GLIDE_PACK_ALPHA
/*  assumes color and alpha values < 256.0 */
	movq r(vertex) , %mm1	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	movd a(vertex) , %mm3	/*  0 | a */
	pf2id %mm1 , %mm1	/*  convert to integer: 000000gg | 000000rr */

	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */
	pf2id %mm3 , %mm3	/*  convert to integer: 00000000 | 000000aa */

	punpcklwd %mm1 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm1	/*  00000000 | 0000gg00 */

	psllq $24 , %mm3	/*  00000000 | aa000000 */
	por %mm2 , %mm1	/*  00000000 | 00rrggbb */

	por %mm3 , %mm1	/*  00000000 | aarrggbb */
	add $8 , dlp	/*  skip data list entry "a" */
#else	/*  !GLIDE_PACK_ALPHA */
/*  assumes color values < 256.0 */

	movq r(vertex) , %mm1	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	pf2id %mm1 , %mm1	/*  convert to integer: 000000gg | 000000rr */
	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */

	punpcklwd %mm1 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm1	/*  00000000 | 0000gg00 */

	por %mm2 , %mm1	/*  00000000 | 00rrggbb */
	add $4 , dlp	/*  next data list entry */
#endif	/*  !GLIDE_PACK_ALPHA */

/*  here: one DWORD in "write buffer", RGB(A) */

	movl (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

#if GLIDE_PACK_ALPHA
	cmp $0 , %eax	/*  end of list ? */
#else
	test %eax , %eax	/*  end of list ? */
#endif
	jz LOCAL(__paramLoopDoneWBone3)	/*  yes, one DWORD in "write buffer" */

LOCAL(__paramLoop3a):
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	add $4 , dlp	/*  dlp++ */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz LOCAL(__paramLoopDoneWBzero3)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop3a)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBone3)	/*  merge back into common stream */

#else	/*  ! GLIDE_PACK_RGB */

/*  here: "write buffer" empty */

	movl (dlp) , %eax	/*  Get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

#if GLIDE_CULLING
	test %eax , %eax	/*  at end of list ? */
#else
	cmp $0 , %eax	/*  at end of list ? */
#endif
	jz LOCAL(__paramLoopDoneWBzero3)	/*  yes, "write buffer" empty */

#if GLIDE_CULLING
#else
	mov %esp , %esp	/*  filler */
#endif

LOCAL(__paramLoop3a):
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */
#if GLIDE_CULLING
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
#else
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
#endif

	jz LOCAL(__paramLoopDoneWBone3)	/*  exit, write buffer contains one DWORD */
	movd (%eax,vertex) , %mm2	/*  get next parameter */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	punpckldq %mm2 , %mm1	/*  current param | previous param */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop3a)	/*  nope, copy next parameter */

	jmp LOCAL(__paramLoopDoneWBzero3)	/*  write buffer empty */
#endif	/*  GLIDE_PACK_RGB */


LOCAL(__paramLoopDoneWBone2):

/*  here: "write buffer" has one DWORD left over from vertex B */

	mov _vc(%esp) , vertex	/*  Current vertex = C */
	mov dlpstrt , dlp	/*  reset the dataList */

	movd x(vertex) , %mm2	/*  0 | x if vertex C */
	punpckldq %mm2 , %mm1	/*  x | old param */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write: x | old param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	movd y(vertex) , %mm1	/*  0 | y of vertex C */

#if GLIDE_PACK_RGB
#if GLIDE_PACK_ALPHA
/*  assumes color and alpha values < 256.0 */
	movq r(vertex) , %mm4	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	movd a(vertex) , %mm3	/*  0 | a */
	pf2id %mm4 , %mm4	/*  convert to integer: 000000gg | 000000rr */

	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */
	pf2id %mm3 , %mm3	/*  convert to integer: 00000000 | 000000aa */

	punpcklwd %mm4 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm4	/*  00000000 | 0000gg00 */

	add $8 , dlp	/*  skip data list entry "a" */
	psllq $24 , %mm3	/*  00000000 | aa000000 */

	por %mm2 , %mm4	/*  00000000 | 00rrggbb */
	por %mm3 , %mm4	/*  00000000 | aarrggbb */
#else	/*  !GLIDE_PACK_ALPHA */
/*  assumes color values < 256.0 */

	movq r(vertex) , %mm4	/*  g | r */
	movd b(vertex) , %mm2	/*  0 | b */

	pf2id %mm4 , %mm4	/*  convert to integer: 000000gg | 000000rr */
	pf2id %mm2 , %mm2	/*  convert to integer: 00000000 | 000000bb */

	punpcklwd %mm4 , %mm2	/*  00000000 | 00rr00bb */
	psrlq $24 , %mm4	/*  00000000 | 0000gg00 */

	add $4 , dlp	/*  next data list entry */
	por %mm2 , %mm4	/*  00000000 | 00rrggbb */
#endif	/*  !GLIDE_PACK_ALPHA */

	punpckldq %mm4 , %mm1	/*  RGB(A) | y */
	movl (dlp) , %eax	/*  get first offset from the data list */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write y | RGB(A) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	add $4 , dlp	/*  dlp++ */
	test %eax , %eax	/*  end of list ? */

	jz LOCAL(__paramLoopDoneWBzero3)	/*  yes, "write buffer" is empty */

LOCAL(__paramLoop3b):
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	jz LOCAL(__paramLoopDoneWBone3)	/*  exit, write buffer contains one DWORD */
	movd (%eax,vertex) , %mm2	/*  get next parameter */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param  */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop3b)	/*  nope, copy next parameter */

#if GLIDE_PACK_ALPHA
	nop 	/*  filler */
	jmp LOCAL(__paramLoopDoneWBzero3)	/*  write buffer empty */
#else
	jmp LOCAL(__paramLoopDoneWBzero3)	/*  write buffer empty */
	nop 	/*  filler */
#endif
#else	/*  !GLIDE_PACK_RGB */
	movl (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz LOCAL(__paramLoopDoneWBone3)	/*  yes, "write buffer" has y data */

LOCAL(__paramLoop3b):
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	add $4 , dlp	/*  dlp++ */

	WRITE_MM1_FIFO_ALIGNED 	/*  PCI write current param | previous param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz LOCAL(__paramLoopDoneWBzero3)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	movl (dlp) , %eax	/*  offset = *(dlp + 1) */

	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz LOCAL(__paramLoop3b)	/*  nope, copy next parameter */
#endif

LOCAL(__paramLoopDoneWBone3):

/*  "write buffer" contains one DWORD that needs to be flushed */

	WRITE_MM1LOW_FIFO 	/*   */
	add $4 , fifo

LOCAL(__paramLoopDoneWBzero3):

/*  Update gc->fifoPtr and gc->fifoRoom */

	mov (_GlideRoot+trisDrawn) , %ecx	/*  _GlideRoot.stats.trisDrawn */
	mov fifo , %eax	/*  new fifo pointer */

	mov fifoPtr(gc) , %ebx	/*  old fifo pointer */
	mov fifo , fifoPtr(gc)	/*  save new fifo pointer */

	mov fifoRoom(gc) , %edx	/*  old fifo space available */
#if GLIDE_PACK_RGB
	add $1 , %ecx	/*  _GlideRoot.stats.trisDrawn++ */
#else
	inc %ecx	/*  _GlideRoot.stats.trisDrawn++ */
#endif

	mov (_GlideRoot+trisProcessed) , %esi	/*  _GlideRoot.stats.trisProcessed */
	sub %ebx , %eax	/*  new fifo ptr - old fifo ptr = additional fifo space used */

	mov %ecx , (_GlideRoot+trisDrawn)
	sub %eax , %edx	/*  new fifo space available */

	mov $0x1 , %eax	/*  return value = triangle drawn */
	mov %edx , fifoRoom(gc)	/*  new fifo space available */

/*  Restore trashed registers     */

	inc %esi	/*  _GlideRoot.stats.trisProcessed++ */
	pop %ebp	/*  restore frame pointer */

#if GLIDE_CULLING
	pop %ebx	/*  restore caller's register variable */
	mov %esi , (_GlideRoot+trisProcessed)
#else
	mov %esi , (_GlideRoot+trisProcessed)
	pop %ebx	/*  restore caller's register variable */
#endif
	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	femms 	/*  no more AMD3D code, clear FPU/MMX regs */

	ret	/*  return to caller */


#if GLIDE_CULLING
LOCAL(__cullFail):
	mov (_GlideRoot+trisProcessed) , %esi	/*  triangles processed so far */
	xor %eax , %eax	/*  return value = triangle not drawn */

	femms 	/*  no more AMD3D code, clear FPU/MMX regs */

/*  Restore trashed registers     */
	inc %esi	/*  _GlideRoot.stats.trisProcessed++     */
	pop %ebp	/*  restore frame pointer */

	mov %esi , (_GlideRoot+trisProcessed)
	pop %ebx

	pop %esi
	pop %edi

	ret
#endif	/*  GLIDE_CULLING */

/* --------------------------------------------------------------------------- */

/*  end AMD3D section */

/* --------------------------------------------------------------------------- */
#endif	/*  GL_AMD3D */

/* --------------------------------------------------------------------------- */

/*  start original code */

/* --------------------------------------------------------------------------- */

#ifndef GL_AMD3D

#define X 0
#define Y 4
/*  edx is used as index, loading from *src */
#define gc %esi	/*  points to graphics context     */

/*  Prologue stuff */
	push %esi
	push %edi

	push %ebx
	push %ebp

	mov (_GlideRoot+curGC) , gc	/*  GR_DCL_GC     */

.align 4
#if GLIDE_CULLING
#define fa %eax	/*  vtx a from caller */
#define fb %ebx	/*  vtx b from caller */
#define fc %ecx	/*  vtx c from caller */

#define cull %edx
#define intArea %ebp	/*  temp Y storage */

/*  Pre-load the current culling mode before all of the */
/*  floating point area stuff.     */
	mov cull_mode(gc) , cull
	mov _va(%esp) , fa

	mov _vb(%esp) , fb
	mov _vc(%esp) , fc

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
#endif	/*  GLIDE_CULLING     */

.align 4
/*  Check to make sure that we have enough room for */
/*  the complete triangle packet. */
	mov (_GlideRoot+curTriSize) , %eax
	mov fifoRoom(gc) , %ebx

	add $4 , %eax
	cmp %eax , %ebx

	jge LOCAL(__triBegin)

	push $__LINE__
	push $0x0

	push %eax
	call _FifoMakeRoom

	add $12 , %esp

/*  Send triangle parameters */

#define dlp %ebx	/*  points to dataList structure */
#define fifo %ebp	/*  points to next entry in fifo */
#define vertex %edx	/*  the current vertex */
#define vOffset %ecx	/*  Current vertex offset */

#define packCol %edi
#define tempVal %edi

.align 4
LOCAL(__triBegin):
	mov fifoPtr(gc) , fifo	/*  Fetch Fifo Ptr */
	mov $4 , vOffset	/*  Starting vertex */

	mov triPacketHdr(gc) , %eax	/*  Packet 3 header */
	nop 

	GR_FIFO_WRITE fifo , 0 , %eax	
					/*  Write packet header to fifo     */
	add $4 , fifo	/*  Advance fifo for hdr & x/y coordinate */

.align 4
LOCAL(__vertexStart):
	mov STKOFF(%esp,vOffset) , vertex	/*  Current vertex */
	add $8 , fifo

	nop 	/*  Avoid p5 agi w/ load of vertex ptr */
	nop 

	movl x(vertex) , %eax	/*  X */
	lea tsuDataList(gc) , dlp	/*  Reset the dataList */

	GR_FIFO_WRITE fifo , -8 , %eax	
						/*  PCI write X */
	movl y(vertex) , %eax	/*  Y  */

	xor packCol , packCol	/*  Clear packed color */
	GR_FIFO_WRITE fifo , -4 , %eax	
						/*  PCI write Y */

#if GLIDE_PACK_RGB
	flds b(vertex)	/*  B */
	fadds _GlideRoot+fBiasLo	/*  BC GC */

	flds g(vertex)	/*  G B */
	fadds _GlideRoot+fBiasHi	/*  GC B */

	flds r(vertex)	/*  R GC BC */
	fadds _GlideRoot+fBiasHi	/*  RC GC BC */

	fxch %st(2)	/*  BC GC RC */
	fstps bias0	/*  GC RC */

	fstps bias1	/*  RC */
	movl bias0 , packCol	/*  B + bias */

	fstps bias0
	movl bias1 , %eax	/*  G + bias */

#if GLIDE_PACK_ALPHA
	flds a(vertex)
	fadds _GlideRoot+fBiasHi

	and $0x00FF , packCol	/*  B color component */
	and $0x0000FF00 , %eax	/*  G component << 8 */

	add $8 , dlp	/*  Packed RGB + A dataList entry */
	nop 

	or %eax , packCol	/*  0000GGBB */
	nop 

	fstps bias1
	movl bias0 , %eax	/*  R + bias */

	movl bias1 , %esi	/*  A + bias */
	and $0x0000FF00 , %eax	/*  R component << 8 */

	and $0x0FFFFFF00 , %esi	/*  A component << 8 */
	shl $8 , %eax	/*  R << 16 */

	or %eax , packCol	/*  00RRGGBB */
	shl $16 , %esi	/*  A << 16 */

	or %esi , packCol	/*  AARRGGBB */
	nop 
#else	/*  !GLIDE_PACK_ALPHA     */
	and $0x00FF , packCol	/*  B color component */
	and $0x0000FF00 , %eax	/*  G component << 8 */

	add $4 , dlp	/*  Next dataList item     */
	or %eax , packCol

	movl bias0 , %eax	/*  R + bias */
	and $0x0000FF00 , %eax	/*  R component << 8 */

	shl $8 , %eax	/*  R << 16 */
	or %eax , packCol	/*  00RRGGBB */
#endif	/*  !GLIDE_PACK_ALPHA */

	GR_FIFO_WRITE fifo , 0 , packCol	
						/*  PCI write packed color value */
	add $4 , fifo
#endif	/*  GLIDE_PACK_RGB */

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
	movl (%eax,vertex) , tempVal	/*  Get the parameter from teh vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxU32) */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  Are we done? */
	GR_FIFO_WRITE fifo , -4 , tempVal	
						/*  *fifoPtr = data */

	jne LOCAL(__paramLoop)

.align 4
LOCAL(__nextVertex):
/*  On to the next vertex */
	add $4 , vOffset
	mov (_GlideRoot+curGC) , gc	/*  Reload gc incase we trashed it as a temp */

	cmp $16 , vOffset	/*  Offset of one past last vertex? */
	jne LOCAL(__vertexStart)

/*  Update gc->fifoPtr and gc->fifoRoom */
	mov fifo , %eax
	mov fifoPtr(gc) , %ebx

	mov fifo , fifoPtr(gc)
	sub %ebx , %eax

	mov (_GlideRoot+trisDrawn) , %ebx	/*  _GlideRoot.stats.trisDrawn++     */
	sub %eax , fifoRoom(gc)

	add $1 , %ebx
	mov %ebx , (_GlideRoot+trisDrawn)

/*  return 1 (triangle drawn)     */
	mov $0x1 , %eax

LOCAL(__triDone):
/*  Restore trashed registers */
	mov (_GlideRoot+trisProcessed) , %esi
	pop %ebp

	add $1 , %esi	/*  _GlideRoot.stats.trisProcessed++     */
	pop %ebx

	pop %edi
	mov %esi , (_GlideRoot+trisProcessed)

	pop %esi
	ret

#endif	/*  !GL_AMD3D */

