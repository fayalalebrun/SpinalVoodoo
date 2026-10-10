/*  -*-asm-*- */
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


/* -------------------------------------------------------------------------- */
/*  AMD3D version */
/* -------------------------------------------------------------------------- */

#ifdef GL_AMD3D

.file "xdraw2.inc"

#define GR_FIFO_WRITE(__addr , __offset , __data) \
	mov __data , __offset(__addr) \


#define WRITE_MM1_FIFO_ALIGNED(__offset) \
	movq %mm1 , __offset(fifo)	/*  store current param | previous param */ \


#define WRITE_MM1LOW_FIFO \
	movd %mm1 , (fifo)	/*  store current param | previous param */ \


#define gc %edi	/*  points to graphics context */
#define fifo %ebp	/*  points to fifo entries */
#define tempVal %esi

/*  Prologue stuff */
	push %edi	/*  save caller's register variable */
	mov (_GlideRoot+curGC) , gc	/*  GR_DCL_GC */

	push %ebp	/*  save frame pointer */
	push %ebx	/*  save caller's register variable         */

#if GLIDE_CULLING
#define fa %eax	/*  vtx a from caller */
#define fb %ebx	/*  vtx b from caller */
#define fc %ecx	/*  vtx c from caller */

#define cull %edx	/*  cull mode */
#define intArea %ecx	/*  area temp storage */

	mov _vb-4(%esp) , fb	/*  get base address of vertex B */
	push %esi	/*  save caller's register variable */

	mov cull_mode(gc) , cull	/*  get cull mode */
	mov _vc(%esp) , fc	/*  get base address of vertex C */

	femms 	/*  will use AMD3D, clear FPU/MMX registers */

	cmp $0 , cull	/*  culling enabled ? */
	mov (_GlideRoot+curTriSize) , tempVal

/*  Cull Check */
	jz UniqLabel(nocull)	/*  nope, no culling */
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
	jz __cullFail	/*  area zero, triangle culled */

	xor cull , intArea	/*  if (j ^ (culltest << 31)) */
	jge UniqLabel(__cullFail)	/*  triangle facing away from viewer, culled */

	cmp tempVal , %ebx	/*  fifo space required >= space available ? */
	jge UniqLabel(__triBegin)	/*  yup, push out triangle data to Voodoo */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push tempVal	/*  fifo space required */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */
	jmp UniqLabel(__triBegin)	/*  merge back with short path */

/*  culling disabled */

UniqLabel(nocull:)
/*  Check to make sure that we have enough room for */
/*  the complete triangle packet. */

	add $4 , tempVal	/*  fifo space needed */
	mov fifoRoom(gc) , %ebx	/*  fifo space available */

	cmp tempVal , %ebx	/*  fifo spce available >= space needed ? */
	jge UniqLabel(__triBegin)	/*  yup, ready to draw triangle */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push tempVal	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */
	nop 	/*  filler */

#else	/*  !GLIDE_CULLING */

	lea _va(%esp) , %eax	/*  pointer to vertex pointers     */
	push %esi	/*  save caller's register variable */

	mov (_GlideRoot+curTriSize) , tempVal	/*  data for whole triangle in bytes */
	mov fifoRoom(gc) , %ebx	/*  fifo space available */

	add $4 , tempVal	/*  fifo space needed (include 4-byte header) */
	femms 	/*  will use AMD3D, clear FPU/MMX registers */

	cmp tempVal , %ebx	/*  fifo spce available >= space needed ? */
	jge UniqLabel(__triBegin)	/*  yup, ready to draw triangle */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push tempVal	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */
	nop 	/*  filler */
#endif	/*  GLIDE_CULLING */


#define dlp %ebx	/*  points to dataList structure */
#define dlpstrt %ecx	/*  points to begin of dataList structure */
#define vertex %edx	/*  the current vertex */
#define packCol %esi

UniqLabel(__triBegin:)
	mov triPacketHdr(gc) , %eax	/*  Packet 3 header */
	lea tsuDataList(gc) , dlp	/*  Reset the dataList */

	mov fifoPtr(gc) , fifo	/*  Fetch Fifo Ptr */
	mov _va(%esp) , vertex	/*  Current vertex = A */

	mov dlp , dlpstrt	/*  save pointer to start of dataList */
	test $4 , fifo	/*  is fifo pointer qword aligned ? */

	jz UniqLabel(__fifo_aligned)	/*  yes, it is qword aligned */
	movq X(vertex) , %mm1	/*  y | x */

	GR_FIFO_WRITE(fifo , 0 , %eax)	/*  write header to fifo now qword aligned */
	add $12 , fifo	/*  fifoPtr += 3*sizeof(FxU32) */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write y | x */
	nop 	/*  filler */

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

	mov (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz UniqLabel(__paramLoopDoneWBone1)	/*  yes, one DWORD in "write buffer" */

UniqLabel(__paramLoop1a:)
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jz UniqLabel(__paramLoopDoneWBzero1)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz UniqLabel(__paramLoop1a)	/*  nope, copy next parameter */

	nop 	/*  filler */
	jmp UniqLabel(__paramLoopDoneWBone1)	/*  merge back into common stream */

#else	/*  ! GLIDE_PACK_RGB */

/*  here: "write buffer" empty */

	mov (dlp) , %eax	/*  Get first offset from the data list */
	test %eax , %eax	/*  at end of list ? */

	lea 4(dlp) , dlp	/*  dlp++ */
	jz UniqLabel(__paramLoopDoneWBzero1)	/*  yes, "write buffer" empty */

UniqLabel(__paramLoop1a:)
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz UniqLabel(__paramLoopDoneWBone1)	/*  exit, write buffer contains one DWORD */

	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp += 2 */

	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jnz UniqLabel(__paramLoop1a)	/*  nope, copy next parameter */

	nop 	/*  filler */
	jmp UniqLabel(__paramLoopDoneWBzero1)	/*  write buffer empty */

#endif	/*  GLIDE_PACK_RGB */

UniqLabel(__fifo_aligned:)
	movd X(vertex) , %mm2	/*  y | x of vertex A */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	movd triPacketHdr(gc) , %mm1	/*  Packet 3 header */
	punpckldq %mm2 , %mm1	/*  x | header */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write x | header */
	movd Y(vertex) , %mm1	/*  0 | y of vertex A */

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
	add $4 , dlp	/*  next data list entry */

	psrlq $24 , %mm4	/*  00000000 | 0000gg00 */
	por %mm2 , %mm4	/*  00000000 | 00rrggbb */
#endif	/*  !GLIDE_PACK_ALPHA */

	punpckldq %mm4 , %mm1	/*  RGB(A) | y */
	mov (dlp) , %eax	/*  get first offset from the data list */

	WRITE_MM1_FIFO_ALIGNED(0)	/*  PCI write y | RGB(A) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	add $4 , dlp	/*  dlp++ */
	test %eax , %eax	/*  end of list ? */

	nop 	/*  filler */
	jz UniqLabel(__paramLoopDoneWBzero1)	/*  yes, "write buffer" is empty */

UniqLabel(__paramLoop1b:)
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz UniqLabel(__paramLoopDoneWBone1)	/*  exit, write buffer contains one DWORD */

	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp += 2 */

	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jnz UniqLabel(__paramLoop1b)	/*  nope, copy next parameter */

	nop 	/*  filler */
	jmp UniqLabel(__paramLoopDoneWBzero1)	/*  write buffer empty */

#else	/*  !GLIDE_PACK_RGB */
	mov (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  end of list ? */
	jz UniqLabel(__paramLoopDoneWBone1)	/*  yes, "write buffer" has y data */

UniqLabel(__paramLoop1b:)
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jz UniqLabel(__paramLoopDoneWBzero1)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz UniqLabel(__paramLoop1b)	/*  nope, copy next parameter */
#endif

UniqLabel(__paramLoopDoneWBone1:)

/*  here: "write buffer" has one DWORD left over from vertex A */

	mov dlpstrt , dlp	/*  reset the dataList */
	mov _vb$(%esp) , vertex	/*  Current vertex = B */

	movd X(vertex) , %mm2	/*  0 | x if vertex B */
	punpckldq %mm2 , %mm1	/*  x | old param */

	WRITE_MM1_FIFO_ALIGNED(0)	/*  PCI write: x | old param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	movd Y(vertex) , %mm1	/*  0 | y of vertex B */
	nop 	/*  filler */

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
	mov (dlp) , %eax	/*  get first offset from the data list */

	WRITE_MM1_FIFO_ALIGNED(0)	/*  PCI write y | RGB(A) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	add $4 , dlp	/*  dlp++ */
	test %eax , %eax	/*  end of list ? */

	nop 	/*  filler */
	jz UniqLabel(__paramLoopDoneWBzero2)	/*  yes, "write buffer" is empty */

UniqLabel(__paramLoop2b:)
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz UniqLabel(__paramLoopDoneWBone2)	/*  exit, write buffer contains one DWORD */

	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp += 2 */

	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jnz UniqLabel(__paramLoop2b)	/*  nope, copy next parameter */

	nop 	/*  filler */
	jmp UniqLabel(__paramLoopDoneWBzero2)	/*  write buffer empty */

#else	/*  !GLIDE_PACK_RGB */
	mov (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz UniqLabel(__paramLoopDoneWBone2)	/*  yes, "write buffer" has y data */

UniqLabel(__paramLoop2b:)
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jz UniqLabel(__paramLoopDoneWBzero2)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz UniqLabel(__paramLoop2b)	/*  nope, copy next parameter */

	nop 	/*  filler */
	jmp UniqLabel(__paramLoopDoneWBone2)	/*  write buffer contains one DWORD */
#endif


UniqLabel(__paramLoopDoneWBzero1:)

	mov _vb(%esp) , vertex	/*  Current vertex = B */
	mov dlpstrt , dlp	/*  Reset the dataList */

	movq X(vertex) , %mm1	/*  y | x of vertex B */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write y | x of vertex B */
	nop 	/*  filler */

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

	test %eax , %eax	/*  end of list ? */
	jz UniqLabel(__paramLoopDoneWBone2)	/*  yes, one DWORD in "write buffer" */

UniqLabel(__paramLoop2a:)
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , dlp	/*  dlp += 8 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jz UniqLabel(__paramLoopDoneWBzero2)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz UniqLabel(__paramLoop2a)	/*  nope, copy next parameter */

	nop 	/*  filler */
	jmp UniqLabel(__paramLoopDoneWBone2)	/*  merge back into common stream */

#else	/*  ! GLIDE_PACK_RGB */

/*  here: "write buffer" empty */

	mov (dlp) , %eax	/*  Get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  at end of list ? */
	jz UniqLabel(__paramLoopDoneWBzero2)	/*  yes, "write buffer" empty */

UniqLabel(__paramLoop2a:)
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz UniqLabel(__paramLoopDoneWBone2)	/*  exit, write buffer contains one DWORD */

	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp += 2 */

	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jnz UniqLabel(__paramLoop2a)	/*  nope, copy next parameter */

#endif	/*  GLIDE_PACK_RGB */


UniqLabel(__paramLoopDoneWBzero2:)

	mov _vc(%esp) , vertex	/*  Current vertex = C */
	mov dlpstrt , dlp	/*  Reset the dataList */

	movq X(vertex) , %mm1	/*  y | x of vertex C */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write y | x of vertex C */
	nop 	/*  filler */


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

	mov (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz UniqLabel(__paramLoopDoneWBone3)	/*  yes, one DWORD in "write buffer" */

UniqLabel(__paramLoop3a:)
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , dlp	/*  dlp += 2 */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	punpckldq %mm2 , %mm1	/*  current param | previous param */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jz UniqLabel(__paramLoopDoneWBzero3)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz UniqLabel(__paramLoop3a)	/*  nope, copy next parameter */

	nop 	/*  filler */
	jmp UniqLabel(__paramLoopDoneWBone3)	/*  merge back into common stream */

#else	/*  ! GLIDE_PACK_RGB */

/*  here: "write buffer" empty */

	mov (dlp) , %eax	/*  Get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  at end of list ? */
	jz UniqLabel(__paramLoopDoneWBzero3)	/*  yes, "write buffer" empty */

UniqLabel(__paramLoop3a:)
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz UniqLabel(__paramLoopDoneWBone3)	/*  exit, write buffer contains one DWORD */

	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp += 2 */

	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jnz UniqLabel(__paramLoop3a)	/*  nope, copy next parameter */

	mov %esp , %esp	/*  filler */
	jmp UniqLabel(__paramLoopDoneWBzero3)	/*  write buffer empty */

#endif	/*  GLIDE_PACK_RGB */


UniqLabel(__paramLoopDoneWBone2:)

/*  here: "write buffer" has one DWORD left over from vertex B */

	mov _vc(%esp) , vertex	/*  Current vertex = C */
	mov dlpstrt , dlp	/*  reset the dataList */

	movd X(vertex) , %mm2	/*  0 | x if vertex C */
	punpckldq %mm2 , %mm1	/*  x | old param */

	WRITE_MM1_FIFO_ALIGNED(0)	/*  PCI write: x | old param */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	movd Y(vertex) , %mm1	/*  0 | y of vertex C */
	mov %esp , %esp	/*  filler */

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
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write y | RGB(A) */
	nop 	/*  filler */

	test %eax , %eax	/*  end of list ? */
	jz UniqLabel(__paramLoopDoneWBzero3)	/*  yes, "write buffer" is empty */

UniqLabel(__paramLoop3b:)
	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz UniqLabel(__paramLoopDoneWBone3)	/*  exit, write buffer contains one DWORD */

	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp++ */

	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jnz UniqLabel(__paramLoop3b)	/*  nope, copy next parameter */

	nop 	/*  filler */
	jmp UniqLabel(__paramLoopDoneWBzero3)	/*  write buffer empty */

#else	/*  !GLIDE_PACK_RGB */

	mov (dlp) , %eax	/*  get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	test %eax , %eax	/*  end of list ? */
	jz UniqLabel(__paramLoopDoneWBone3)	/*  yes, "write buffer" has y data */

UniqLabel(__paramLoop3b:)
	movd (%eax,vertex) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */

	WRITE_MM1_FIFO_ALIGNED(-8)	/*  PCI write current param | previous param */
	jz UniqLabel(__paramLoopDoneWBzero3)	/*  exit, "write buffer" empty */

	movd (%eax,vertex) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  offset = *(dlp + 1) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz UniqLabel(__paramLoop3b)	/*  nope, copy next parameter */
#endif

UniqLabel(__paramLoopDoneWBone3:)

/*  "write buffer" contains one DWORD that needs to be flushed */

	WRITE_MM1LOW_FIFO 	/*   */
	add $4 , fifo

UniqLabel(__paramLoopDoneWBzero3:)

/*  Update gc->fifoPtr and gc->fifoRoom */

	mov (_GlideRoot+trisDrawn) , %ecx	/*  _GlideRoot.stats.trisDrawn */
	mov fifo , %eax	/*  new fifo pointer */

	mov fifoPtr(gc) , %ebx	/*  old fifo pointer */
	mov fifo , fifoPtr(gc)	/*  save new fifo pointer */

	mov fifoRoom(gc) , %edx	/*  old fifo space available */
	inc %ecx	/*  _GlideRoot.stats.trisDrawn++ */

	mov (_GlideRoot+trisProcessed) , %ebp	/*  _GlideRoot.stats.trisProcessed */
	sub %ebx , %eax	/*  new fifo ptr - old fifo ptr = additional fifo space used */

	mov %ecx , (_GlideRoot+trisDrawn)
	sub %eax , %edx	/*  new fifo space available */

	mov $0x1 , %eax	/*  return value = triangle drawn */
	mov %edx , fifoRoom(gc)	/*  new fifo space available */

/*  Restore trashed registers     */

	inc %ebp	/*  _GlideRoot.stats.trisProcessed++ */
	pop %esi	/*  restore caller's register variable */

	mov %ebp , (_GlideRoot+trisProcessed)	/*      */
	pop %ebx	/*  restore caller's register variable */

	pop %ebp	/*  restore frame pointer */
	pop %edi	/*  restore caller's register variable */

	femms 	/*  no more AMD3D code, clear FPU/MMX regs */

	ret	/*  return to caller */


#if GLIDE_CULLING
UniqLabel(__cullFail:)
	mov (_GlideRoot+trisProcessed) , %ebp	/*  triangles processed so far */
	xor %eax , %eax	/*  return value = triangle not drawn */

	femms 	/*  no more AMD3D code, clear FPU/MMX regs */

/*  Restore trashed registers     */
	inc %ebp	/*  _GlideRoot.stats.trisProcessed++     */
	pop %esi

	mov %ebp , (_GlideRoot+trisProcessed)
	pop %ebx

	pop %ebp	/*  restore frame pointer */
	pop %edi

	ret
#endif	/*  GLIDE_CULLING */

/* --------------------------------------------------------------------------- */

/*  end AMD3D section */

/* --------------------------------------------------------------------------- */

#endif	/*  !defined GL_AMD3D */

/* -------------------------------------------------------------------------- */
/*  start original code */
/* -------------------------------------------------------------------------- */

#if GL_AMD3D == 0

.file "xdraw2.inc"

#define gc %esi	/*  points to graphics context */

/*  Prologue stuff */
	push %esi
	mov (_GlideRoot+curGC) , gc	/*  GR_DCL_GC */

	push %edi
	push %ebx

	push %ebp
	nop 

.align 4
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
	jz UniqLabel(nocull)

	shl $31 , cull	/*  culltest << 31     */

UniqLabel(Area_Computation:)
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
	fsts Area	/*   |    |    |    |    area */

/*  Pop temp things from the sw culling off the fp stack */
	fstp %st(0)	/*  4 */
	fstp %st(0)	/*  3 */
	fstp %st(0)	/*  2 */
	fstp %st(0)	/*  1 */
	fstp %st(0)	/*  0     */

	mov Area , intArea	/*  j = *(long *)&area */
	xor %eax , %eax	/*  Clear the return value (0 == culled) */

/*  Zero Area Triangle Check */
	and $0x7fffffff , intArea	/*  if ((j & 0x7FFFFFFF) == 0) */
	jz UniqLabel(__triDone)

/*  Triangle area check vs culling mode */
	mov Area , intArea	/*  reload area just in case we're culling */
	xor cull , intArea	/*  if (j ^ (culltest << 31)) */

	jge UniqLabel(__triDone)
UniqLabel(nocull:)
#endif	/*  GLIDE_CULLING     */

.align 4
/*  Check to make sure that we have enough room for */
/*  the complete triangle packet. */
	mov (_GlideRoot+curTriSize) , %eax
	mov fifoRoom(gc) , %ebx

	add $4 , %eax
	cmp %eax , %ebx

	jge UniqLabel(__triBegin)

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

#define GR_FIFO_WRITE(__addr , __offset , __data) \
	mov __data , __offset(__addr)


.align 4
UniqLabel(__triBegin:)
	mov fifoPtr(gc) , fifo	/*  Fetch Fifo Ptr */
	mov $4 , vOffset	/*  Starting vertex */

	mov triPacketHdr(gc) , %eax	/*  Packet 3 header */
	nop 

	GR_FIFO_WRITE(fifo , 0 , %eax)	/*  Write packet header to fifo     */
	add $4 , fifo	/*  Advance fifo for hdr & x/y coordinate */

.align 4
UniqLabel(__vertexStart:)
	mov STKOFF(%esp, vOffset) , vertex	/*  Current vertex */
	add $8 , fifo

	nop 	/*  Avoid p5 agi w/ load of vertex ptr */
	nop 

	movl (vertex) , %eax	/*  X */
	lea tsuDataList(gc) , dlp	/*  Reset the dataList */

	GR_FIFO_WRITE(fifo , -8 , %eax)	/*  PCI write X */
	movl 4(vertex) , %eax	/*  Y  */

	xor packCol , packCol	/*  Clear packed color */
	GR_FIFO_WRITE(fifo , -4 , %eax)	/*  PCI write Y */

#if GLIDE_PACK_RGB
	fldl b(vertex)	/*  B */
	faddl _GlideRoot+fBiasLo	/*  BC GC */

	fldl g(vertex)	/*  G B */
	faddl _GlideRoot+fBiasHi	/*  GC B */

	fldl r(vertex)	/*  R GC BC */
	faddl _GlideRoot+fBiasHi	/*  RC GC BC */

	fxch %st(2)	/*  BC GC RC */
	fstpl bias0	/*  GC RC */

	fstpl bias1	/*  RC */
	movl bias0 , packCol	/*  B + bias */

	fstpl bias0
	movl bias1 , %eax	/*  G + bias */

#if GLIDE_PACK_ALPHA
	fldl a(vertex)
	faddl _GlideRoot+fBiasHi

	and $0x00FF , packCol	/*  B color component */
	and $0x0000FF00 , %eax	/*  G component << 8 */

	add $8 , dlp	/*  Packed RGB + A dataList entry */
	nop 

	or %eax , packCol	/*  0000GGBB */
	nop 

	fstpl bias1
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

	GR_FIFO_WRITE(fifo , 0 , packCol)	/*  PCI write packed color value */
	add $4 , fifo
#endif	/*  GLIDE_PACK_RGB */

UniqLabel(__doParams:)
	movl (dlp) , %eax	/*  Get first offset from the data list */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  Are we done? */
	je UniqLabel(__nextVertex)

/*  Not using align directive here because it sometimes */
/*  introduces an agi for the eax use below. */
	nop 
	nop 

UniqLabel(__paramLoop:)
	movl (%eax,vertex) , tempVal	/*  Get the parameter from teh vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxU32) */

	movl (dlp) , %eax	/*  offset = *(dlp + 1) */
	add $4 , dlp	/*  dlp++ */

	cmp $0 , %eax	/*  Are we done? */
	GR_FIFO_WRITE(fifo , -4 , tempVal)	/*  *fifoPtr = data */

	jne UniqLabel(__paramLoop)

.align 4
UniqLabel(__nextVertex:)
/*  On to the next vertex */
	add $4 , vOffset
	mov (_GlideRoot+curGC) , gc	/*  Reload gc incase we trashed it as a temp */

	cmp $16 , vOffset	/*  Offset of one past last vertex? */
	jne UniqLabel(__vertexStart)

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

UniqLabel(__triDone:)
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
