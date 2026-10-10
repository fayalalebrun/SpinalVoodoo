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

.file "xdraw3.asm"

#include "assyntax.h"
#ifdef __DJGPP__
#define THREADVALUE EXTRN(threadValueDJGPP)
#else
#define THREADVALUE threadValueLinux
#endif

#ifdef GL_AMD3D

/* -------------------------------------------------------------------------- */
/*  start AMD3D version */
/* -------------------------------------------------------------------------- */

/*  include listing.inc */
#include "fxgasm.h"

.data
.section	.rodata

.data
ALIGN(8)
	.type	btab,@object
	.size	btab,4
btab:	.int	8
	.type	atab,@object
	.size	atab,4
atab:	.int	8
	.type	vSize,@object
	.size	vSize,4
vSize:	.int	0
	.type	strideinbytes,@object
	.size	strideinbytes,4
strideinbytes:	.int	0
	.type	vertices,@object
	.size	vertices,4
vertices:	.int	0


.text


ALIGN(32)
GLOBL(_grDrawTriangles_3DNow)
#define _mode 20
#define _count 24
#define _pointers 28

/*  930  : { */
/*  931  : #define FN_NAME "_grDrawTriangles" */
/*  932  :  */
/*  933  :   FxI32 */
/*  934  : #ifdef GLIDE_DEBUG */
/*  935  :     vSize, */
/*  936  : #endif */
/*  937  :     k */
/*  938  :   FxI32 stride = mode */
/*  939  :   float *vPtr */
/*  940  :  */
/*  941  :   GR_BEGIN_NOFIFOCHECK(FN_NAME, 90) */
/*  942  :  */
/*  943  :   GDBG_INFO_MORE(gc->myLevel, "(count = %d, pointers = 0x%x)\n", */
/*  944  :                  count, pointers) */
/*  945  :  */
/*  946  :   GR_FLUSH_STATE() */

#define gc edi	/*  points to graphics context */
#define fifo ecx	/*  points to next entry in fifo */
#define dlp ebp	/*  points to dataList structure */
#define vertexCount esi	/*  Current vertex counter in the packet */
#define vertexPtr ebx	/*  Current vertex pointer (in deref mode) */
#define vertex ebx	/*  Current vertex (in non-deref mode) */
#define dlpStart edx	/*  Pointer to start of offset list */

	push %edi	/*  save caller's register variable */

	push %esi	/*  save caller's register variable */

	push %ebx	/*  save caller's register variable */
	mov _count-4(%esp) , %vertexCount	/*  number of vertices in triangles */

	movl (THREADVALUE) , %gc
	mov _pointers-4(%esp) , %vertexPtr	/*  get current vertex pointer (deref mode) */

	push %ebp	/*  save frame pointer */

	mov invalid(%gc) , %edx	/*  state needs validation ? */

	test %vertexCount , %vertexCount	/*  number of vertices <= 0 ? */
	jle .L_grDrawTriangles_3DNow_tris_done	/*  yup, triangles are done */
	test %edx , %edx	/*  do we need to validate state ? */
	je .L_grDrawTriangles_3DNow_no_validation	/*  nope, it's valid */

	call EXTRN(_grValidateState)	/*  validate state */

.L_grDrawTriangles_3DNow_no_validation:

/*  947  :  */
/*  948  : #ifdef GLIDE_DEBUG */
/*  949  :   GDBG_INFO(110, "%s:  vSize = %d\n", FN_NAME, vSize) */
/*  950  :  */
/*  951  :   GDBG_INFO(110, "%s:  paramMask = 0x%x\n", FN_NAME, gc->cmdTransportInfo.paramMask) */
/*  952  : #endif */
/*  953  :  */
/*  954  :   if (stride == 0) */
/*  955  :     stride = gc->state.vData.vStride */
/*  956  :  */
/*  957  :  */
/*  958  :   _GlideRoot.stats.trisProcessed+=(count/3) */
/*  959  :  */
/*  960  :   if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) { */

/*  We can operate in one of two modes: */

/*  0. We are stepping through an array of vertices, in which case */
/*  the stridesize is equal to the size of the vertex data, and */
/*  always > 4, since vertex data must a least contain x,y (ie 8 bytes). */
/*  vertexPtr is pointing to the array of vertices. */

/*  1. We are stepping through an array of pointers to vertices */
/*  in which case the stride is 4 bytes and we need to dereference */
/*  the pointers to get at the vertex data. vertexPtr is pointing */
/*  to the array of pointers to vertices. */

	mov _count(%esp) , %eax	/*  count */
	mov $0x0AAAAAAAB , %ebp	/*  1/3*2^32*2 */

	femms 	/*  we'll use MMX clear MMX/3DX state       */

	mul %ebp	/*  edx:eax = 1/3*2*2^32*count edx = 1/3*2*count */
	nop 	/*  filler */

	mov trisProcessed(%gc) , %eax	/*  trisProcessed */
	shr $1 , %edx	/*  count/3 */

	add %edx , %eax	/*  trisProcessed += count/3 */
	mov _mode(%esp) , %edx	/*  get mode (0 or 1) */

	mov CoordinateSpace(%gc) , %ecx	/*  coordinates space (window/clip) */
	mov %eax , trisProcessed(%gc)	/*  trisProcessed */

	test %edx , %edx	/*  mode 0 (array of vertices) ? */
	jnz .L_grDrawTriangles_3DNow_deref_mode	/*  nope, it's mode 1 (array of pointers to vertices) */

	mov vertexStride(%gc) , %edx	/*  get stride in DWORDs */

	shl $2 , %edx	/*  stride in bytes */
	test %ecx , %ecx	/*  coordinate space == 0 (window) ? */

	mov %edx , (strideinbytes)	/*  save off stride (in bytes) */
	jnz .L_grDrawTriangles_3DNow_clip_coordinates_ND	/*  nope, coordinate space != window   */

/*  961  :     while (count > 0) { */
/*  962  :       FxI32 vcount = count >=15 ? 15 : count */
/*  963  :       GR_SET_EXPECTED_SIZE(vcount * gc->state.vData.vSize, 1) */
/*  964  :       TRI_STRIP_BEGIN(kSetupStrip, vcount, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD) */
/*  965  :        */

.L_grDrawTriangles_3DNow_win_coords_loop_ND:

	sub $15 , %vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(%gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , %vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , %vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul %vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(%gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L_grDrawTriangles_3DNow_win_tri_begin_ND	/*  yup, start writing triangle data */

	pushl $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call EXTRN(_grCommandTransportMakeRoom)	/*  note: updates fifoPtr */
	add $12, %esp

ALIGN(32)
.L_grDrawTriangles_3DNow_win_tri_begin_ND:

	mov %vertexCount , %eax	/*  number of vertices in triangles */
	mov fifoPtr(%gc) , %fifo	/*  get fifoPtr */

	mov cullStripHdr(%gc) , %ebp	/*  <2:0> = type */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	lea tsuDataList(%gc) , %dlpStart	/*  pointer to start of offset list */
	or %ebp , %eax	/*  setup vertex count and type */

	test $4 , %fifo	/*  fifoPtr QWORD aligned ? */
	jz .L_grDrawTriangles_3DNow_fifo_aligned_ND	/*  yup */

	mov %eax , (%fifo)	/*  PCI write packet type */
	add $4 , %fifo	/*  fifo pointer now QWORD aligned */

/*  966  :       for (k = 0 k < vcount k++) { */
/*  967  :         FxI32 i */
/*  968  :         FxU32 dataElem = 0 */
/*  969  :          */
/*  970  :         vPtr = pointers */
/*  971  :         if (mode) */
/*  972  :           vPtr = *(float **)vPtr */
/*  973  :         (float *)pointers += stride */
/*  974  :          */
/*  975  :         i = gc->tsuDataList[dataElem] */
/*  976  :          */
/*  977  :         TRI_SETF(FARRAY(vPtr, 0)) */
/*  978  :         TRI_SETF(FARRAY(vPtr, 4)) */
/*  979  :         while (i != GR_DLIST_END) { */
/*  980  :           TRI_SETF(FARRAY(vPtr, i)) */
/*  981  :           dataElem++ */
/*  982  :           i = gc->tsuDataList[dataElem] */
/*  983  :         } */
/*  984  :       } */
/*  985  :       TRI_END */
/*  986  :       GR_CHECK_SIZE() */
/*  987  :       count -= 15 */
/*  988  :     } */

.L_grDrawTriangles_3DNow_win_vertex_loop_ND_WB0:/*  nothing in "write buffer" */

	mov (%dlpStart) , %eax	/*  get first offset from offset list */
	mov %dlpStart , %dlp	/*  point to start of offset list */

	movq (%vertex) , %mm1	/*  get vertex x,y */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	add $4 , %dlp	/*  dlp++ */
	test %eax , %eax	/*  if offset == 0, end of list */

	movq %mm1 , -8(%fifo)	/*  PCI write x, y */
	jz .L_grDrawTriangles_3DNow_win_datalist_end_ND_WB0	/*  no more vertex data, nothing in "write buffer"  */

.L_grDrawTriangles_3DNow_win_datalist_loop_ND_WB0:/*  nothing in "write buffer" */

	movd (%vertex,%eax) , %mm1	/*  get next parameter */
	mov (%dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz .L_grDrawTriangles_3DNow_win_datalist_end_ND_WB1	/*  exit, write buffer contains one DWORD */

	movd (%vertex,%eax) , %mm2	/*  get next parameter */
	add $8 , %dlp	/*  dlp++ */

	mov -4(%dlp) , %eax	/*  get next offset from offset list */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	punpckldq %mm2 , %mm1	/*  current param | previous param */

	movq %mm1 , -8(%fifo)	/*  PCI write current param | previous param */
	jnz .L_grDrawTriangles_3DNow_win_datalist_loop_ND_WB0	/*  nope, copy next parameter */

.L_grDrawTriangles_3DNow_win_datalist_end_ND_WB0:

	mov (strideinbytes) , %eax	/*  get offset to next vertex */
	dec %vertexCount	/*  another vertex done. Any left? */

	lea (%vertex,%eax) , %vertex	/*  points to next vertex */
	jnz .L_grDrawTriangles_3DNow_win_vertex_loop_ND_WB0	/*  yup, output next vertex */

.L_grDrawTriangles_3DNow_win_vertex_end_ND_WB0:

	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */
	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	sub $15 , %vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */

	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	test %vertexCount , %vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L_grDrawTriangles_3DNow_win_coords_loop_ND	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters off stack */

.L_grDrawTriangles_3DNow_fifo_aligned_ND:

	movd %eax , %mm1	/*  move header into "write buffer" */

.L_grDrawTriangles_3DNow_win_vertex_loop_ND_WB1:/*  one DWORD in "write buffer" */

	movd (%vertex) , %mm2	/*  0 | x of vertex */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	lea 4(%dlpStart) , %dlp	/*  point to start of offset list */
	nop 	/*  filler */

	punpckldq %mm2 , %mm1	/*  packet header | x of vertex */
	mov -4(%dlp) , %eax	/*  first offset in offset list */

	movq %mm1 , -8(%fifo)	/*  PCI write packet header | x of vertex */
	movd 4(%vertex) , %mm1	/*  0 | y of vertex */

	cmp $0 , %eax	/*  offset == 0 (list empty) ? */
	jz .L_grDrawTriangles_3DNow_win_datalist_end_ND_WB1	/*  yup, no more vertex data, one DWORD in "write buffer" */

.L_grDrawTriangles_3DNow_win_datalist_loop_ND_WB1:/*  one DWORD in "write buffer"  */

	movd (%vertex,%eax) , %mm2	/*  get next parameter */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (%dlp) , %eax	/*  get next offset from offset list */
	add $8 , %dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(%fifo)	/*  PCI write current param | previous param */
	jz .L_grDrawTriangles_3DNow_win_datalist_end_ND_WB0	/*  yes, exit, "write buffer" empty */

	movd (%vertex,%eax) , %mm1	/*  get next parameter */
	mov -4(%dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
	jnz .L_grDrawTriangles_3DNow_win_datalist_loop_ND_WB1	/*  nope, copy next parameter */

.L_grDrawTriangles_3DNow_win_datalist_end_ND_WB1:

	mov (strideinbytes) , %eax	/*  get offset to next vertex */
	dec %vertexCount	/*  another vertex done. Any left? */

	lea (%vertex,%eax) , %vertex	/*  points to next vertex */
	jnz .L_grDrawTriangles_3DNow_win_vertex_loop_ND_WB1	/*  yup, output next vertex */

.L_grDrawTriangles_3DNow_win_vertex_end_ND_WB1:

	movd %mm1 , (%fifo)	/*  flush "write buffer" */
	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */

	add $4 , %fifo	/*  fifoPtr += sizeof(FxU32) */
	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */
	sub $15 , %vertexCount	/*  remaining number of vertices to process */

	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	test %vertexCount , %vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L_grDrawTriangles_3DNow_win_coords_loop_ND	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters off stack */

ALIGN(32)

.L_grDrawTriangles_3DNow_deref_mode:

	prefetch (%vertexPtr)	/*  pre-load first group of pointers */

	test %ecx , %ecx	/*  coordinate space == 0 (window) ? */
	jnz .L_grDrawTriangles_3DNow_clip_coordinates_D	/*  nope, coordinate space != window */

.L_grDrawTriangles_3DNow_win_coords_loop_D:

	sub $15 , %vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(%gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , %vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , %vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul %vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(%gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L_grDrawTriangles_3DNow_win_tri_begin_D	/*  yup, start writing triangle data */

	pushl $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call EXTRN(_grCommandTransportMakeRoom)	/*  note: updates fifoPtr */
	add $12, %esp

ALIGN(32)
.L_grDrawTriangles_3DNow_win_tri_begin_D:

	mov %vertexCount , %eax	/*  number of vertices in triangles */
	mov fifoPtr(%gc) , %fifo	/*  get fifoPtr */

	mov cullStripHdr(%gc) , %ebp	/*  <2:0> = type */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	or %ebp , %eax	/*  setup mode, vertex count, and type */
	lea tsuDataList(%gc) , %dlpStart	/*  pointer to start of offset list */

	test $4 , %fifo	/*  fifoPtr QWORD aligned ? */
	jz .L_grDrawTriangles_3DNow_fifo_aligned_D	/*  yup */

	mov %eax , (%fifo)	/*  PCI write packet type */
	add $4 , %fifo	/*  fifo pointer now QWORD aligned */

.L_grDrawTriangles_3DNow_win_vertex_loop_D_WB0:/*  nothing in "write buffer" */

	mov (%vertexPtr) , %edx	/*  dereference pointer, edx points to vertex */
	add $4 , %vertexPtr	/*  next pointer */

	lea tsuDataList(%gc) , %dlp	/*  get pointer to offset list */
	movq (%edx) , %mm1	/*  get vertex x,y */

	mov (%dlp) , %eax	/*  get first offset from offset list */
	add $4 , %dlp	/*  dlp++ */

	movq %mm1 , (%fifo)	/*  PCI write x, y */
	add $8 , %fifo	/*  fifo += 2 */

	test %eax , %eax	/*  if offset == 0, end of offset list */
	je .L_grDrawTriangles_3DNow_win_datalist_end_D_WB0	/*  no more vertex data, nothing in "write buffer"  */

.L_grDrawTriangles_3DNow_win_datalist_loop_D_WB0:/*  nothing in "write buffer" */

	movd (%edx,%eax) , %mm1	/*  get next parameter */
	mov (%dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz .L_grDrawTriangles_3DNow_win_datalist_end_D_WB1	/*  exit, write buffer contains one DWORD */

	movd (%edx,%eax) , %mm2	/*  get next parameter */
	add $8 , %dlp	/*  dlp++ */

	mov -4(%dlp) , %eax	/*  get next offset from offset list */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(%fifo)	/*  PCI write current param | previous param */
	jnz .L_grDrawTriangles_3DNow_win_datalist_loop_D_WB0	/*  nope, copy next parameter */

.L_grDrawTriangles_3DNow_win_datalist_end_D_WB0:

	dec %vertexCount	/*  another vertex done. Any left? */
	jnz .L_grDrawTriangles_3DNow_win_vertex_loop_D_WB0	/*  yup, output next vertex */

.L_grDrawTriangles_3DNow_win_vertex_end_D_WB0:

	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */
	nop 	/*  filler */

	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */
	nop 	/*  filler */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	add %eax , %ebp	/*  new number of bytes available in fifo */
	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */

	sub $15 , %vertexCount	/*  remaining number of vertices to process */

	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */
	test %vertexCount , %vertexCount	/*  any vertices left to process ? */

	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process */
	jg .L_grDrawTriangles_3DNow_win_coords_loop_D	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters off stack */

.L_grDrawTriangles_3DNow_fifo_aligned_D:

	movd %eax , %mm1	/*  move header into "write buffer" */

.L_grDrawTriangles_3DNow_win_vertex_loop_D_WB1:/*  one DWORD in "write buffer" */

	mov (%vertexPtr) , %edx	/*  dereference pointer, edx points to vertex */
	add $4 , %vertexPtr	/*  next pointer */

	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */
	lea tsuDataList(%gc) , %dlp	/*  get pointer to start of offset list */

	movd (%edx) , %mm2	/*  0 | x of vertex */
	add $4 , %dlp	/*  dlp++ */

	mov -4(%dlp) , %eax	/*  first offset in offset list */
	punpckldq %mm2 , %mm1	/*  packet header | x of vertex */

	movq %mm1 , -8(%fifo)	/*  PCI write packet header | x of vertex */
	movd 4(%edx) , %mm1	/*  0 | y of vertex */

	cmp $0 , %eax	/*  offset == 0 (list empty) ? */
	je .L_grDrawTriangles_3DNow_win_datalist_end_D_WB1	/*  yup, no more vertex data, one DWORD in "write buffer" */

.L_grDrawTriangles_3DNow_win_datalist_loop_D_WB1:/*  one DWORD in "write buffer" = MM1 */

	movd (%edx,%eax) , %mm2	/*  get next parameter */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (%dlp) , %eax	/*  get next offset from offset list */
	add $8 , %dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(%fifo)	/*  PCI write current param | previous param */
	jz .L_grDrawTriangles_3DNow_win_datalist_end_D_WB0	/*  yes, exit, "write buffer" empty */

	movd (%edx,%eax) , %mm1	/*  get next parameter */
	mov -4(%dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz .L_grDrawTriangles_3DNow_win_datalist_loop_D_WB1	/*  nope, copy next parameter */

.L_grDrawTriangles_3DNow_win_datalist_end_D_WB1:

	dec %vertexCount	/*  another vertex done. Any left? */
	jnz .L_grDrawTriangles_3DNow_win_vertex_loop_D_WB1	/*  yup, output next vertex */

.L_grDrawTriangles_3DNow_win_vertex_end_D_WB1:

	movd %mm1 , (%fifo)	/*  flush "write buffer" */
	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */

	add $4 , %fifo	/*  fifoPtr++ */
	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */
	sub $15 , %vertexCount	/*  remaining number of vertices to process */

	add %eax , %ebp	/*  new number of bytes available in fifo */
	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */

	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	cmp $0 , %vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L_grDrawTriangles_3DNow_win_coords_loop_D	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters off stack */

ALIGN(32)

/*  989  :   } */
/*  990  :   else { */
/*  991  :      * */
/*  992  :      * first cut of clip space coordinate code, no optimization. */
/*  993  :      * */
/*  994  :     float oow */
/*  995  :      */
/*  996  :     while (count > 0) { */
/*  997  :       FxI32 vcount = count >= 15 ? 15 : count */
/*  998  :        */
/*  999  :       GR_SET_EXPECTED_SIZE(vcount * gc->state.vData.vSize, 1) */
/*  1000 :       TRI_STRIP_BEGIN(kSetupStrip, vcount, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD) */
/*  1001 :        */
/*  1002 :       for (k = 0 k < vcount k++) { */
/*  1003 :         vPtr = pointers */
/*  1004 :         if (mode) */
/*  1005 :           vPtr = *(float **)pointers */
/*  1006 :         oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset) */
/*  1007 :          */
/*  1008 :         * x, y * */
/*  1009 :         TRI_SETF(FARRAY(vPtr, 0) */
/*  1010 :                  *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox) */
/*  1011 :         TRI_SETF(FARRAY(vPtr, 4) */
/*  1012 :                  *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) */
/*  1013 :         (float *)pointers += stride */
/*  1014 :          */
/*  1015 :         TRI_VP_SETFS(vPtr,oow) */
/*  1016 :       } */
/*  1017 :       TRI_END */
/*  1018 :       GR_CHECK_SIZE() */
/*  1019 :       count -= 15 */
/*  1020 :   } */
/*  1021 : } */

.L_grDrawTriangles_3DNow_clip_coordinates_D:

	movl $4 , (strideinbytes)	/*  unit stride for array of pointers to vertices */

.L_grDrawTriangles_3DNow_clip_coordinates_ND:

#define dataElem ebp	/*  number of vertex components processed */

	movd (EXTRN(_GlideRoot)+pool_f255) , %mm6	/*  GlideRoot.pool.f255  */

.L_grDrawTriangles_3DNow_clip_coords_begin:

	sub $15 , %vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(%gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , %vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , %vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul %vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(%gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L_grDrawTriangles_3DNow_clip_tri_begin	/*  yup, start writing triangle data */

	pushl $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call EXTRN(_grCommandTransportMakeRoom)	/*  note: updates fifoPtr */
	add $12, %esp

ALIGN(32)
.L_grDrawTriangles_3DNow_clip_tri_begin:
	mov %vertexCount , %edx	/*  number of vertices in triangles */
	mov fifoPtr(%gc) , %fifo	/*  get fifoPtr */

	mov cullStripHdr(%gc) , %ebp	/*  <2:0> = type */
	shl $6 , %edx	/*  <9:6> = vertex count (max 15) */

	or %ebp , %edx	/*  setup mode, vertex count, and type */

	mov %edx , (%fifo)	/*  PCI write packet type */
	add $4 , %fifo	/*  fifo pointer now QWORD aligned */

.L_grDrawTriangles_3DNow_clip_for_begin:

	mov %vertexPtr , %edx	/*  vertex = vertexPtr (assume no-deref mode) */
	mov _mode(%esp) , %eax	/*  mode 0 = no deref, mode 1 = deref */

	mov %vertexCount , (vertices)	/*  save numnber of vertices */
	test %eax , %eax	/*  deref mode ? */

	mov wInfo_offset(%gc) , %eax	/*  get offset of W into vertex struct */
	jz .L_grDrawTriangles_3DNow_clip_noderef	/*  yup, no-deref mode */

	mov (%vertexPtr) , %edx	/*  vertex = *vertexPtr */

.L_grDrawTriangles_3DNow_clip_noderef:

	movd (%edx,%eax) , %mm0	/*  0 | W of current vertex */
	pfrcp %mm0 , %mm1	/*  0 | 1/W approx */

	mov (strideinbytes) , %ebp	/*  offset to next vertex/vertexPtr */
	movq (%edx) , %mm2	/*  y | x of current vertex */

	pfrcpit1 %mm1 , %mm0	/*  0 | 1/W refine */
	movq vp_hwidth(%gc) , %mm3	/*  gc->state.Viewport.hheight | gc->state.Viewport.hwidth */

	movq vp_ox(%gc) , %mm4	/*  gc->state.Viewport.oy | gc->state.Viewport.ox */
	add %ebp , %vertexPtr	/*  point to next vertex/VertexPtr */

	pfrcpit2 %mm1 , %mm0	/*  oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset */
	mov paramIndex(%gc) , %esi	/*  gc->state.paramIndex */

	pfmul %mm3 , %mm2	/*  TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth */
	xor %dataElem , %dataElem	/*  dataElem = 0 */

	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxFloat) */
	punpckldq %mm0 , %mm0	/*  oow | oow */

	pfmul %mm0 , %mm2	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth */
	pfadd %mm4 , %mm2	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) | */

/*   FxI32 i, dataElem=0 \ */
/*   i = gc->tsuDataList[dataElem] \ */
/*   if (gc->state.paramIndex & (STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA)) { \ */
/*     if (gc->state.vData.colorType == GR_FLOAT) { \ */
/*       if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) { \ */
/*         DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255) \ */
/*         DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255) \ */
/*         DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255) \ */
/*       } \ */
/*       if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) { \ */
/*         DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255) \ */
/*       } \ */
/*     } \ */
/*     else { \ */
/*       DA_SETF(FARRAY(_s, i)) \ */
/*       dataElem++ \ */
/*       i = gc->tsuDataList[dataElem] \ */
/*     } \ */
/*   } \ */

	test $3 , %esi	/*  STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA ? */
	mov tsuDataList(%gc) , %eax	/*  first entry from offset list */

	movq %mm2 , -8(%fifo)	/*  PCI write transformed x, y */
	jz .L_grDrawTriangles_3DNow_clip_setup_ooz	/*  nope, no color at all needed */

	cmpl $0 , colorType(%gc)	/*  gc->state.vData.colorType == GR_FLOAT ? */
	jne .L_grDrawTriangles_3DNow_clip_setup_pargb	/*  nope, packed ARGB format */

	test $1 , %esi	/*  STATE_REQUIRES_IT_DRGB ? */
	jz .L_grDrawTriangles_3DNow_clip_setup_a	/*  no, but definitely A */

	movd (%edx,%eax) , %mm2	/*  0 | r */
	mov tsuDataList+4(%gc) , %eax	/*  offset of g part of vertex data */

	pfmul %mm6 , %mm2	/*  0 | r * 255.0f */
	movd (%edx,%eax) , %mm3	/*  0 | g */

	mov tsuDataList+8(%gc) , %eax	/*  offset of b part of vertex data */
	movd %mm2 , (%fifo)	/*  PCI write r*255 */

	pfmul %mm6 , %mm3	/*  0 | g * 255.0f */
	movd (%edx,%eax) , %mm2	/*  0 | b */

	movd %mm3 , 4(%fifo)	/*  PCI write g*255 */
	mov $12 , %dataElem	/*  dataElem = 3 */

	pfmul %mm6 , %mm2	/*  0 | b * 255.0f */
	mov tsuDataList+12(%gc) , %eax	/*  offset of A part of vertex data */

	test $2 , %esi	/*  STATE_REQUIRES_IT_ALPHA ? */
	lea 12(%fifo) , %fifo	/*  fifoPtr += 3*sizeof(FxFloat) */

	movd %mm2 , -4(%fifo)	/*  PCI write b*255 */
	jz .L_grDrawTriangles_3DNow_clip_setup_ooz	/*  nope, no alpha, proceeed with ooz */

.L_grDrawTriangles_3DNow_clip_setup_a:
	movd (%eax,%edx) , %mm2	/*  0 | a */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	mov %esp , %esp	/*  filler */
	add $4 , %dataElem	/*  dataElem++  */

	pfmul %mm6 , %mm2	/*  0 | a * 255.0f */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  offset of next part of vertex data */

	movd %mm2 , -4(%fifo)	/*  PCI write a*255 */
	jmp .L_grDrawTriangles_3DNow_clip_setup_ooz	/*  check whether we need to push out z */

.L_grDrawTriangles_3DNow_clip_setup_pargb:
	movd (%eax,%edx) , %mm2	/*  get packed ARGB data */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxU32) */

	mov $4 , %dataElem	/*  dataElem = 1 (namely pargb) */
	mov tsuDataList+4(%gc) , %eax	/*  offset of next part of vertex data */

	movd %mm2 , -4(%fifo)	/*  PCI write packed ARGB */

/*   if (gc->state.paramIndex & STATE_REQUIRES_OOZ) { \ */
/*     if (gc->state.fbi_config.fbzMode & SST_DEPTH_FLOAT_SEL) { \ */
/*       if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \ */
/*         DA_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow) \ */
/*       } else { \ */
/*         DA_SETF(_oow) \ */
/*       } \ */
/*       dataElem++ \ */
/*       i = gc->tsuDataList[dataElem] \ */
/*     } \ */
/*     else { \ */
/*       DA_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth + gc->state.Viewport.oz) \ */
/*       dataElem++ \ */
/*       i = gc->tsuDataList[dataElem] \ */
/*     } \ */
/*   } \ */

.L_grDrawTriangles_3DNow_clip_setup_ooz:

	test $4 , %esi	/*  STATE_REQUIRES_OOZ ? */
	jz .L_grDrawTriangles_3DNow_clip_setup_qow	/*  nope */

	testl $0x200000 , fbi_fbzMode(%gc)	/*  gc->state.fbi_config.fbzMode & SST_DEPTH_FLOAT_SEL != 0 ? */
	je .L_grDrawTriangles_3DNow_clip_setup_ooz_nofog	/*  nope */

	cmpl $0 , qInfo_mode(%gc)	/*  gc->state.vData.qInfo.mode == GR_PARAM_ENABLE ? */
	jz .L_grDrawTriangles_3DNow_clip_setup_fog_oow	/*  nope */

	mov qInfo_offset(%gc) , %eax	/*  offset of Q component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  0 | q*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed Q */
	jmp .L_grDrawTriangles_3DNow_clip_setup_qow	/*  check whether we need to write Q or W */

.L_grDrawTriangles_3DNow_clip_setup_fog_oow:

	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , %dataElem	/*  dataElem++ */

	movd %mm0 , -4(%fifo)	/*  PCI write oow */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

	jmp .L_grDrawTriangles_3DNow_clip_setup_qow	/*  check whether we need to write Q or W */

.L_grDrawTriangles_3DNow_clip_setup_ooz_nofog:

	movd (%eax,%edx) , %mm2	/*  0 | z component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem += 1 */
	movd vp_hdepth(%gc) , %mm3	/*  0 | gc->state.Viewport.hdepth */

	pfmul %mm0 , %mm2	/*  TRI_SETF(FARRAY(_s, i)*_oow */
	movd vp_oz(%gc) , %mm4	/*  0 | gc->state.Viewport.oz */

	pfmul %mm3 , %mm2	/*  0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  offset of next vertex component */

	pfadd %mm4 , %mm2	/*  0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz */
	movd %mm2 , -4(%fifo)	/*  PCI write transformed Z */

/*   if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) { \ */
/*     if (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE) { \ */
/*       DA_SETF(FARRAY(_s, gc->state.vData.fogInfo.offset)*_oow) \ */
/*     } \ */
/*     else if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \ */
/*       DA_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow) \ */
/*     } else { \ */
/*       DA_SETF(_oow) \ */
/*     } \ */
/*     dataElem++ \ */
/*     i = gc->tsuDataList[dataElem] \ */
/*   } \ */

.L_grDrawTriangles_3DNow_clip_setup_qow:
	test $8 , %esi	/*  STATE_REQUIRES_OOW_FBI ? */
	jz .L_grDrawTriangles_3DNow_clip_setup_qow0	/*  nope */

	cmpl $0 , fogInfo_mode(%gc)	/*  gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE ? */
	jz .L_grDrawTriangles_3DNow_clip_setup_oow_nofog	/*  nope, no fog */

	mov fogInfo_offset(%gc) , %eax	/*  offset of fogInfo component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | fogInfo of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  fogInfo*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed Q */
	jmp .L_grDrawTriangles_3DNow_clip_setup_qow0	/*  continue with q0 */

.L_grDrawTriangles_3DNow_clip_setup_oow_nofog:

	cmpl $0 , qInfo_mode(%gc)	/*  gc->state.vData.qInfo.mode == GR_PARAM_ENABLE ? */
	je .L_grDrawTriangles_3DNow_clip_setup_oow	/*  nope, write oow, not Q */

	mov qInfo_offset(%gc) , %eax	/*  offset of Q component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed Q */
	jmp .L_grDrawTriangles_3DNow_clip_setup_qow0	/*  continue with q0 */

.L_grDrawTriangles_3DNow_clip_setup_oow:
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , %dataElem	/*  dataElem++ */

	movd %mm0 , -4(%fifo)	/*  PCI write oow */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

/*   if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) { \ */
/*     if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) { \ */
/*       DA_SETF(FARRAY(_s, gc->state.vData.q0Info.offset)*_oow) \ */
/*     } \ */
/*     else { \ */
/*       DA_SETF(_oow) \ */
/*     } \ */
/*     dataElem++ \ */
/*     i = gc->tsuDataList[dataElem] \ */
/*   } \ */

.L_grDrawTriangles_3DNow_clip_setup_qow0:
	test $16 , %esi	/*  STATE_REQUIRES_W_TMU0 ? */
	jz .L_grDrawTriangles_3DNow_clip_setup_stow0	/*  nope  */

	cmpl $0 , q0Info_mode(%gc)	/*  does vertex have Q component ? */
	je .L_grDrawTriangles_3DNow_clip_setup_oow0	/*  nope, not Q but W */

	mov q0Info_offset(%gc) , %eax	/*  offset of Q component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q0 of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q0*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed q0 */
	jmp .L_grDrawTriangles_3DNow_clip_setup_stow0	/*  continue with stow0 */

	nop 	/*  filler */

.L_grDrawTriangles_3DNow_clip_setup_oow0:
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , %dataElem	/*  dataElem++ */

	movd %mm0 , -4(%fifo)	/*  PCI write oow */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

/*   if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) { \ */
/*     DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].s_scale) \ */
/*     DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[0].t_scale) \ */
/*   } \ */

.L_grDrawTriangles_3DNow_clip_setup_stow0:

	test $32 , %esi	/*  STATE_REQUIRES_ST_TMU0 ? */
	jz .L_grDrawTriangles_3DNow_clip_setup_qow1	/*  nope */

	movq tmu0_s_scale(%gc) , %mm7	/*  state.tmu_config[0].t_scale | state.tmu_config[0].s_scale */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movd (%edx,%eax) , %mm2	/*  param1 */
	mov tsuDataList+4(%gc,%dataElem) , %eax	/* pointer to next vertex component */

	pfmul %mm0 , %mm7	/*  oow*tmu0_t_scale | oow*tmu0_s_scale */
	add $8 , %dataElem	/*  dataElem += 2 */

	movd (%edx,%eax) , %mm3	/*  param2 */
	punpckldq %mm3 , %mm2	/*  param2 | param1 */

	pfmul %mm7 , %mm2	/*  param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale */
	nop 	/*  filler */

	movq %mm2 , -8(%fifo)	/*  PCI write param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

/*   if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) { \ */
/*     if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) { \ */
/*       DA_SETF(FARRAY(_s, gc->state.vData.q1Info.offset)*_oow) \ */
/*     } \ */
/*     else { \ */
/*       DA_SETF(_oow) \ */
/*     } \ */
/*     dataElem++ \ */
/*     i = gc->tsuDataList[dataElem] \ */
/*   } \ */

.L_grDrawTriangles_3DNow_clip_setup_qow1:
	test $64 , %esi	/*  STATE_REQUIRES_W_TMU1 ? */
	jz .L_grDrawTriangles_3DNow_clip_setup_stow1	/*  nope */

	cmpl $0 , q1Info_mode(%gc)	/*  does vertex have Q component ? */
	je .L_grDrawTriangles_3DNow_clip_setup_oow1	/*  nope, not Q but W */

	mov q1Info_offset(%gc) , %eax	/*  offset of Q component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q1 of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q1*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed q1 */
	jmp .L_grDrawTriangles_3DNow_clip_setup_stow1	/*  continue with stow1 */

.L_grDrawTriangles_3DNow_clip_setup_oow1:
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , %dataElem	/*  dataElem++ */

	movd %mm0 , -4(%fifo)	/*  PCI write oow */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

/*   if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) { \ */
/*     DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].s_scale) \ */
/*     DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.tmu_config[1].t_scale) \ */
/*   } \ */

.L_grDrawTriangles_3DNow_clip_setup_stow1:

	test $128 , %esi	/*  STATE_REQUIRES_ST_TMU1 ? */
	mov (vertices) , %vertexCount	/*  get number of vertices */

	movq tmu1_s_scale(%gc) , %mm7	/*  state.tmu_config[1].t_scale | state.tmu_config[1].s_scale */
	jz .L_grDrawTriangles_3DNow_clip_setup_end	/*  nope */

	movd (%edx,%eax) , %mm2	/*  param1 */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov tsuDataList+4(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm7	/*  oow*state.tmu_config[1].t_scale | oow*state.tmu_config[1].s_scale */

	movd (%edx,%eax) , %mm3	/*  param2 */
	punpckldq %mm3 , %mm2	/*  param2 | param1 */

	pfmul %mm7 , %mm2	/*  param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale */
	movq %mm2 , -8(%fifo)	/*  PCI write param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale */

.L_grDrawTriangles_3DNow_clip_setup_end:

	dec %vertexCount	/*  vcount-- */
	jnz .L_grDrawTriangles_3DNow_clip_for_begin	/*  until  */

.L_grDrawTriangles_3DNow_clip_for_end:

	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */
	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	sub $15 , %vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */

	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	cmp $0 , %vertexCount	/*  any vertices left to process ? */

	jg .L_grDrawTriangles_3DNow_clip_coords_begin	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

.L_grDrawTriangles_3DNow_tris_done:
	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters */
.L_END__grDrawTriangles_3DNow:
.size _grDrawTriangles_3DNow,.L_END__grDrawTriangles_3DNow-_grDrawTriangles_3DNow

#define _pktype 20
#define _type 24
#define _mode 28
#define _count 32
#define _pointers 36

#define gc edi	/*  points to graphics context */
#define fifo ecx	/*  points to next entry in fifo */
#define dlp ebp	/*  points to dataList structure */
#define vertexCount esi	/*  Current vertex counter in the packet */
#define vertexPtr ebx	/*  Current vertex pointer (in deref mode) */
#define vertex ebx	/*  Current vertex (in non-deref mode) */
#define dlpStart edx	/*  Pointer to start of offset list */

#define X 0
#define Y 4

ALIGN(32)

GLOBL(_grDrawVertexList_3DNow_Window)
/*  132  : { */

	push %edi	/*  save caller's register variable */

	push %esi	/*  save caller's register variable */
	mov _count-8(%esp) , %vertexCount	/*  number of vertices in strip/fan */

	push %ebp	/*  save frame pointer */

	push %ebx	/*  save caller's register variable         */
	mov _pointers(%esp) , %vertexPtr	/*  get current vertex pointer (deref mode) */
/*  get current vertex (non-deref mode) */

	movl (THREADVALUE) , %gc
	test %vertexCount , %vertexCount	/*  number of vertices <= 0 ? */

	nop 	/*  filler */
	jle .L_grDrawVertexList_3DNow_Window_strip_done	/*  yup, the strip/fan is done */

/*      vSize = gc->state.vData.vSize */
/*      if (stride == 0) */
/*        stride = gc->state.vData.vStride */

/*  We can operate in one of two modes: */

/*  0. We are stepping through an array of vertices, in which case */
/*  the stridesize is equal to the size of the vertex data, and */
/*  always > 4, since vertex data must a least contain x,y (ie 8 bytes). */
/*  vertexPtr is pointing to the array of vertices. */

/*  1. We are stepping through an array of pointers to vertices */
/*  in which case the stride is 4 bytes and we need to dereference */
/*  the pointers to get at the vertex data. vertexPtr is pointing */
/*  to the array of pointers to vertices. */

	mov _mode(%esp) , %edx	/*  get mode (0 or 1) */
	mov vertexSize(%gc) , %eax	/*  size of vertex data in bytes */

	test %edx , %edx	/*  mode 0 (array of vertices) ? */
	mov vertexStride(%gc) , %edx	/*  get stride in DWORDs */

	jnz .L_grDrawVertexList_3DNow_Window_deref_mode	/*  nope, it's mode 1 (array of pointers to vertices) */

	femms 	/*  we'll use MMX clear MMX/3DX state       */

	shl $2 , %edx	/*  stride in bytes */
	movl %edx , (strideinbytes)	/*  save off stride (in bytes) */

/*      Draw the first (or possibly only) set.  This is necessary because */
/*      the packet is 3_BDDDDDD, and in the next set, the packet is 3_DDDDDD */
/*      We try to make tstrip code simple to read. We combine the original code */
/*      into a single loop by adding an extra packet type assignment at the end of the loop. */
/*   */
/*      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) { */
/*        while (count > 0) { */
/*          FxI32 k, vcount = count >= 15 ? 15 : count */
/*          GR_SET_EXPECTED_SIZE(vcount * vSize, 1) */
/*          TRI_STRIP_BEGIN(type, vcount, vSize, pktype) */


.L_grDrawVertexList_3DNow_Window_win_coords_loop_ND:

	sub $15 , %vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(%gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , %vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , %vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul %vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(%gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L_grDrawVertexList_3DNow_Window_win_strip_begin_ND	/*  yup, start writing strip data */

	pushl $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call EXTRN(_grCommandTransportMakeRoom)	/*  note: updates fifoPtr */
	add $12, %esp	

ALIGN(32)
.L_grDrawVertexList_3DNow_Window_win_strip_begin_ND:
/*      Setup packet header */

	mov %vertexCount , %eax	/*  number of vertices in strip/fan */
	mov _type(%esp) , %edx	/*  setup mode */

	mov fifoPtr(%gc) , %fifo	/*  get fifoPtr */
	shl $22 , %edx	/*  <27:22> = setup mode (kSetupStrip or kSetupFan) */

	mov cullStripHdr(%gc) , %ebp	/*  <2:0> = type */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	or %edx , %eax	/*  setup mode and vertex count */
	mov _pktype(%esp) , %edx	/*  <5:3> = command (SSTCP_PKT3_BDDBDD, SSTCP_PKT3_BDDDDD, or SSTCP_PKT3_DDDDDD) */

	or %ebp , %eax	/*  setup mode, vertex count, and type */
	nop 	/*  filler */

	or %edx , %eax	/*  setup mode, vertex count, type, and command */
	lea tsuDataList(%gc) , %dlpStart	/*  pointer to start of offset list */

	test $4 , %fifo	/*  fifoPtr QWORD aligned ? */
	jz .L_grDrawVertexList_3DNow_Window_fifo_aligned_ND	/*  yup */

	mov %eax , (%fifo)	/*  PCI write packet type */
	add $4 , %fifo	/*  fifo pointer now QWORD aligned */

/*      for (k = 0 k < vcount k++) { */
/*        FxI32 i */
/*        FxU32 dataElem */
/*        float *vPtr */
/*        vPtr = pointers */
/*        if (mode) */
/*          vPtr = *(float **)vPtr */
/*        (float *)pointers += stride */
/*        TRI_SETF(FARRAY(vPtr, 0)) */
/*        dataElem = 0 */
/*        TRI_SETF(FARRAY(vPtr, 4)) */
/*        i = gc->tsuDataList[dataElem] */

.L_grDrawVertexList_3DNow_Window_win_vertex_loop_ND_WB0:/*  nothing in "write buffer" */

	mov (%dlpStart) , %eax	/*  get first offset from offset list */
	lea 4(%dlpStart) , %dlp	/*  point to start of offset list */

	movq X(%vertex) , %mm1	/*  get vertex x,y */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	nop 	/*  filler */
	test %eax , %eax	/*  if offset == 0, end of list */

	movq %mm1 , -8(%fifo)	/*  PCI write x, y */
	jz .L_grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB0	/*  no more vertex data, nothing in "write buffer"  */

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L_grDrawVertexList_3DNow_Window_win_datalist_loop_ND_WB0:/*  nothing in "write buffer" */

	movd (%vertex,%eax) , %mm1	/*  get next parameter */
	mov (%dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz .L_grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB1	/*  exit, write buffer contains one DWORD */

	movd (%vertex,%eax) , %mm2	/*  get next parameter */
	add $8 , %dlp	/*  dlp++ */

	mov -4(%dlp) , %eax	/*  get next offset from offset list */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	punpckldq %mm2 , %mm1	/*  current param | previous param */

	movq %mm1 , -8(%fifo)	/*  PCI write current param | previous param */
	jnz .L_grDrawVertexList_3DNow_Window_win_datalist_loop_ND_WB0	/*  nope, copy next parameter */

.L_grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB0:

	mov (strideinbytes) , %eax	/*  get offset to next vertex */
	sub $1 , %vertexCount	/*  another vertex done. Any left? */

	lea (%vertex,%eax) , %vertex	/*  points to next vertex */
	jnz .L_grDrawVertexList_3DNow_Window_win_vertex_loop_ND_WB0	/*  yup, output next vertex */

.L_grDrawVertexList_3DNow_Window_win_vertex_end_ND_WB0:

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	mov $16 , %ebp	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */

	mov %ebp , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	nop 	/*  filler */
	sub $15 , %vertexCount	/*  remaining number of vertices to process */

	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */
	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process  */

	test %vertexCount , %vertexCount	/*  any vertices left to process ? */
	jg .L_grDrawVertexList_3DNow_Window_win_coords_loop_ND	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebx	/*  restore caller's register variable */
	pop %ebp	/*  restore frame pointer */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restire caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */

ALIGN(32)

.L_grDrawVertexList_3DNow_Window_fifo_aligned_ND:

	movd %eax , %mm1	/*  move header into "write buffer" */

.L_grDrawVertexList_3DNow_Window_win_vertex_loop_ND_WB1:/*  one DWORD in "write buffer" */

	movd X(%vertex) , %mm2	/*  0 | x of vertex */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	lea 4(%dlpStart) , %dlp	/*  point to start of offset list */
	nop 	/*  filler */

	punpckldq %mm2 , %mm1	/*  packet header | x of vertex */
	mov -4(%dlp) , %eax	/*  first offset in offset list */

	movq %mm1 , -8(%fifo)	/*  PCI write packet header | x of vertex */
	movd Y(%vertex) , %mm1	/*  0 | y of vertex */

	cmp $0 , %eax	/*  offset == 0 (list empty) ? */
	jz .L_grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB1	/*  yup, no more vertex data, one DWORD in "write buffer" */

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L_grDrawVertexList_3DNow_Window_win_datalist_loop_ND_WB1:/*  one DWORD in "write buffer"  */

	movd (%vertex,%eax) , %mm2	/*  get next parameter */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (%dlp) , %eax	/*  get next offset from offset list */
	add $8 , %dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(%fifo)	/*  PCI write current param | previous param */
	jz .L_grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB0	/*  yes, exit, "write buffer" empty */

	movd (%vertex,%eax) , %mm1	/*  get next parameter */
	mov -4(%dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz .L_grDrawVertexList_3DNow_Window_win_datalist_loop_ND_WB1	/*  nope, copy next parameter */

.L_grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB1:

	mov (strideinbytes) , %eax	/*  get offset to next vertex */
	sub $1 , %vertexCount	/*  another vertex done. Any left? */

	lea (%vertex,%eax) , %vertex	/*  points to next vertex */
	jnz .L_grDrawVertexList_3DNow_Window_win_vertex_loop_ND_WB1	/*  yup, output next vertex */

.L_grDrawVertexList_3DNow_Window_win_vertex_end_ND_WB1:

	movd %mm1 , (%fifo)	/*  flush "write buffer" */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxU32) */

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	mov $16 , %ebp	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */

	mov %ebp , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */
	sub $15 , %vertexCount	/*  remaining number of vertices to process */

	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	test %vertexCount , %vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L_grDrawVertexList_3DNow_Window_win_coords_loop_ND	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebx	/*  restore caller's register variable */
	pop %ebp	/*  restore frame pointer */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restire caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */

ALIGN(32)

.L_grDrawVertexList_3DNow_Window_deref_mode:

	femms 	/*  we'll use MMX clear FPU/MMX state */

	prefetch (%vertexPtr)	/*  pre-load first group of pointers */

.L_grDrawVertexList_3DNow_Window_win_coords_loop_D:

	sub $15 , %vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(%gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , %vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , %vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul %vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(%gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L_grDrawVertexList_3DNow_Window_win_strip_begin_D	/*  yup, start writing strip data */

	pushl $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call EXTRN(_grCommandTransportMakeRoom)	/*  note: updates fifoPtr */
	add $12, %esp	

ALIGN(32)
.L_grDrawVertexList_3DNow_Window_win_strip_begin_D:
/*      Setup packet header */

	mov %vertexCount , %eax	/*  number of vertices in strip/fan */
	mov _type(%esp) , %edx	/*  setup mode */

	mov fifoPtr(%gc) , %fifo	/*  get fifoPtr */
	shl $22 , %edx	/*  <27:22> = setup mode (kSetupStrip or kSetupFan) */

	mov cullStripHdr(%gc) , %ebp	/*  <2:0> = type */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	or %edx , %eax	/*  setup mode and vertex count */
	mov _pktype(%esp) , %edx	/*  <5:3> = command (SSTCP_PKT3_BDDBDD, SSTCP_PKT3_BDDDDD, or SSTCP_PKT3_DDDDDD) */

	or %ebp , %eax	/*  setup mode, vertex count, and type */
	mov $4 , %ebp	/*  test bit 2 */

	or %edx , %eax	/*  setup mode, vertex count, type, and command */
	lea tsuDataList(%gc) , %dlpStart	/*  pointer to start of offset list */

	test %ebp , %fifo	/*  fifoPtr QWORD aligned ? */
	jz .L_grDrawVertexList_3DNow_Window_fifo_aligned_D	/*  yup */

	mov %eax , (%fifo)	/*  PCI write packet type */
	add $4 , %fifo	/*  fifo pointer now QWORD aligned */

/*      for (k = 0 k < vcount k++) { */
/*        FxI32 i */
/*        FxU32 dataElem */
/*        float *vPtr */
/*        vPtr = pointers */
/*        if (mode) */
/*          vPtr = *(float **)vPtr */
/*        (float *)pointers += stride */
/*        TRI_SETF(FARRAY(vPtr, 0)) */
/*        dataElem = 0 */
/*        TRI_SETF(FARRAY(vPtr, 4)) */
/*        i = gc->tsuDataList[dataElem] */


.L_grDrawVertexList_3DNow_Window_win_vertex_loop_D_WB0:/*  nothing in "write buffer" */

	mov (%vertexPtr) , %edx	/*  dereference pointer, edx points to vertex */
	add $4 , %vertexPtr	/*  next pointer */

	lea tsuDataList(%gc) , %dlp	/*  get pointer to offset list dlp ++ */
	add $4 , %dlp	/*  dlp ++ */

	movq X(%edx) , %mm1	/*  get vertex x,y */
	add $8 , %fifo	/*  fifo += 2 */

	mov -4(%dlp) , %eax	/*  get first offset from offset list */
	movq %mm1 , -8(%fifo)	/*  PCI write x, y */

	test %eax , %eax	/*  if offset == 0, end of offset list */
	je .L_grDrawVertexList_3DNow_Window_win_datalist_end_D_WB0	/*  no more vertex data, nothing in "write buffer"  */

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L_grDrawVertexList_3DNow_Window_win_datalist_loop_D_WB0:/*  nothing in "write buffer" */

	movd (%edx,%eax) , %mm1	/*  get next parameter */
	mov (%dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz .L_grDrawVertexList_3DNow_Window_win_datalist_end_D_WB1	/*  exit, write buffer contains one DWORD */

	add $8 , %dlp	/*  dlp++ */
	movd (%edx,%eax) , %mm2	/*  get next parameter */

	mov -4(%dlp) , %eax	/*  get next offset from offset list */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(%fifo)	/*  PCI write current param | previous param */
	jnz .L_grDrawVertexList_3DNow_Window_win_datalist_loop_D_WB0	/*  nope, copy next parameter */

.L_grDrawVertexList_3DNow_Window_win_datalist_end_D_WB0:

	dec %vertexCount	/*  another vertex done. Any left? */
	jnz .L_grDrawVertexList_3DNow_Window_win_vertex_loop_D_WB0	/*  yup, output next vertex */

.L_grDrawVertexList_3DNow_Window_win_vertex_end_D_WB0:

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	mov $16 , %ebp	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */

	mov %ebp , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	add %eax , %ebp	/*  new number of bytes available in fifo */
	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */

	sub $15 , %vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */

	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	test %vertexCount , %vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L_grDrawVertexList_3DNow_Window_win_coords_loop_D	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebx	/*  restore caller's register variable */
	pop %ebp	/*  restore frame pointer */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restire caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */

ALIGN(32)

.L_grDrawVertexList_3DNow_Window_fifo_aligned_D:

	movd %eax , %mm1	/*  move header into "write buffer" */

.L_grDrawVertexList_3DNow_Window_win_vertex_loop_D_WB1:/*  one DWORD in "write buffer" */

	mov (%vertexPtr) , %edx	/*  dereference pointer, edx points to vertex */
	add $4 , %vertexPtr	/*  next pointer */

	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */
	lea tsuDataList(%gc) , %dlp	/*  get pointer to start of offset list */

	movd X(%edx) , %mm2	/*  0 | x of vertex */
	add $4 , %dlp	/*  dlp++ */

	mov -4(%dlp) , %eax	/*  first offset in offset list */
	punpckldq %mm2 , %mm1	/*  packet header | x of vertex */

	movq %mm1 , -8(%fifo)	/*  PCI write packet header | x of vertex */
	movd Y(%edx) , %mm1	/*  0 | y of vertex */

	test %eax , %eax	/*  offset == 0 (list empty) ? */
	je .L_grDrawVertexList_3DNow_Window_win_datalist_end_D_WB1	/*  yup, no more vertex data, one DWORD in "write buffer" */

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L_grDrawVertexList_3DNow_Window_win_datalist_loop_D_WB1:/*  one DWORD in "write buffer" = MM1 */

	movd (%edx,%eax) , %mm2	/*  get next parameter */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (%dlp) , %eax	/*  get next offset from offset list */
	add $8 , %dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(%fifo)	/*  PCI write current param | previous param */
	jz .L_grDrawVertexList_3DNow_Window_win_datalist_end_D_WB0	/*  yes, exit, "write buffer" empty */

	movd (%edx,%eax) , %mm1	/*  get next parameter */
	mov -4(%dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
	jnz .L_grDrawVertexList_3DNow_Window_win_datalist_loop_D_WB1	/*  nope, copy next parameter */

.L_grDrawVertexList_3DNow_Window_win_datalist_end_D_WB1:

	dec %vertexCount	/*  another vertex done. Any left? */
	jnz .L_grDrawVertexList_3DNow_Window_win_vertex_loop_D_WB1	/*  yup, output next vertex */

.L_grDrawVertexList_3DNow_Window_win_vertex_end_D_WB1:

	movd %mm1 , (%fifo)	/*  flush "write buffer" */
	add $4 , %fifo	/*  fifoPtr++ */

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	mov $16 , %ebp	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */

	mov %ebp , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	nop 	/*  filler */

	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */
	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */

	sub $15 , %vertexCount	/*  remaining number of vertices to process */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */
	cmp $0 , %vertexCount	/*  any vertices left to process ? */

	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	jg .L_grDrawVertexList_3DNow_Window_win_coords_loop_D	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

.L_grDrawVertexList_3DNow_Window_strip_done:
	pop %ebx	/*  restore caller's register variable */
	pop %ebp	/*  restore frame pointer */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restire caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */

.L_END__grDrawVertexList_3DNow_Window:
.size _grDrawVertexList_3DNow_Window,.L_END__grDrawVertexList_3DNow_Window-_grDrawVertexList_3DNow_Window




GLOBL(_grDrawVertexList_3DNow_Clip)
/*  132  : { */

ALIGN(32)

	push %edi	/*  save caller's register variable */

	push %esi	/*  save caller's register variable */
	mov _count-8(%esp) , %vertexCount	/*  number of vertices in strip/fan */

	push %ebp	/*  save frame pointer */

	push %ebx	/*  save caller's register variable         */
	mov _pointers(%esp) , %vertexPtr	/*  get current vertex pointer (deref mode) */
/*  get current vertex (non-deref mode) */

	movl (THREADVALUE) , %gc
	test %vertexCount , %vertexCount	/*  number of vertices <= 0 ? */

	jle .L_grDrawVertexList_3DNow_Clip_strip_done	/*  yup, the strip/fan is done */

/*      vSize = gc->state.vData.vSize */
/*      if (stride == 0) */
/*        stride = gc->state.vData.vStride */

/*  We can operate in one of two modes: */

/*  0. We are stepping through an array of vertices, in which case */
/*  the stridesize is equal to the size of the vertex data, and */
/*  always > 4, since vertex data must a least contain x,y (ie 8 bytes). */
/*  vertexPtr is pointing to the array of vertices. */

/*  1. We are stepping through an array of pointers to vertices */
/*  in which case the stride is 4 bytes and we need to dereference */
/*  the pointers to get at the vertex data. vertexPtr is pointing */
/*  to the array of pointers to vertices. */

	mov _mode(%esp) , %edx	/*  get mode (0 or 1) */
	mov vertexSize(%gc) , %eax	/*  size of vertex data in bytes */

	test %edx , %edx	/*  mode 0 (array of vertices) ? */
	mov vertexStride(%gc) , %edx	/*  get stride in DWORDs */

	movd (EXTRN(_GlideRoot)+pool_f255) , %mm6	/*  GlideRoot.pool.f255      */
	movl $4 , (strideinbytes)	/*  array of pointers     */

	jnz .L_grDrawVertexList_3DNow_Clip_clip_coords_begin	/*  nope, it's mode 1 */

.L_grDrawVertexList_3DNow_Clip_clip_coordinates_ND:

	shl $2 , %edx	/*  stride in bytes */
	movl %edx , (strideinbytes)	/*  save off stride (in bytes) */

ALIGN(32)
.L_grDrawVertexList_3DNow_Clip_clip_coords_begin:

#define dataElem ebp	/*  number of vertex components processed     */

/*    { */
/*      float oow */
/*        while (count > 0) { */
/*        FxI32 k, vcount = count >= 15 ? 15 : count */

	sub $15 , %vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(%gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */
	and %eax , %vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */

	mov fifoRoom(%gc) , %eax	/*  fifo space available */
	add $15 , %vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul %vertexCount , %ecx	/*  total amount of vertex data we'll send */

	add $4 , %ecx	/*  add header size ==> total packet size */
	nop 	/*  filler */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L_grDrawVertexList_3DNow_Clip_clip_strip_begin	/*  yup, start writing strip data */

	pushl $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call EXTRN(_grCommandTransportMakeRoom)	/*  note: updates fifoPtr */
	add $12, %esp

ALIGN(32)
.L_grDrawVertexList_3DNow_Clip_clip_strip_begin:
/*      TRI_STRIP_BEGIN(type, vcount, vSize, pktype) */

	mov _type(%esp) , %edx	/*  setup mode */
	mov %vertexCount , %eax	/*  number of vertices in strip/fan */

	mov fifoPtr(%gc) , %fifo	/*  get fifoPtr */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	mov cullStripHdr(%gc) , %ebp	/*  <2:0> = type */
	shl $22 , %edx	/*  <27:22> = setup mode (kSetupStrip or kSetupFan) */

	or %edx , %eax	/*  setup mode and vertex count */
	mov _pktype(%esp) , %edx	/*  <5:3> = command (SSTCP_PKT3_BDDBDD, SSTCP_PKT3_BDDDDD, or SSTCP_PKT3_DDDDDD) */

	or %ebp , %eax	/*  setup mode, vertex count, and type */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxU32) */

	or %edx , %eax	/*  setup mode, vertex count, type, and command */
	mov %eax , -4(%fifo)	/*  PCI write header */

/*      for (k = 0 k < vcount k++) { */
/*        float *vPtr */
/*        vPtr = pointers */

.L_grDrawVertexList_3DNow_Clip_clip_for_begin:

/*        if (mode) */
/*          vPtr = *(float **)vPtr */

	mov %vertexPtr , %edx	/*  vertex = vertexPtr (assume no-deref mode) */
	mov _mode(%esp) , %eax	/*  mode 0 = no deref, mode 1 = deref */

	mov %vertexCount , (vertices)	/*  save numnber of vertices */
	test %eax , %eax	/*  deref mode ? */

	mov wInfo_offset(%gc) , %eax	/*  get offset of W into vertex struct */
	jz .L_grDrawVertexList_3DNow_Clip_clip_noderef	/*  yup, no-deref mode */

	mov (%vertexPtr) , %edx	/*  vertex = *vertexPtr */
	lea (%esp) , %esp	/*  filler */

.L_grDrawVertexList_3DNow_Clip_clip_noderef:

/*        oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset) */

	movd (%edx,%eax) , %mm0	/*  0 | W of current vertex */
	pfrcp %mm0 , %mm1	/*  0 | 1/W approx */

	mov (strideinbytes) , %ebp	/*  offset to next vertex/vertexPtr */
	movq (%edx) , %mm2	/*  y | x of current vertex */

	pfrcpit1 %mm1 , %mm0	/*  0 | 1/W refine */
	movq vp_hwidth(%gc) , %mm3	/*  gc->state.Viewport.hheight | gc->state.Viewport.hwidth */

	movq vp_ox(%gc) , %mm4	/*  gc->state.Viewport.oy | gc->state.Viewport.ox */
	add %ebp , %vertexPtr	/*  point to next vertex/VertexPtr */

	pfrcpit2 %mm1 , %mm0	/*  oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset */
	mov paramIndex(%gc) , %esi	/*  gc->state.paramIndex */

/*        * x, y * */
/*        TRI_SETF(FARRAY(vPtr, 0) */
/*          *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox) */
/*        TRI_SETF(FARRAY(vPtr, 4) */
/*          *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) */

	pfmul %mm3 , %mm2	/*  TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth */
	xor %dataElem , %dataElem	/*  dataElem = 0 */

	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxFloat) */
	punpckldq %mm0 , %mm0	/*  oow | oow */

	pfmul %mm0 , %mm2	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth */
	pfadd %mm4 , %mm2	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) | */

	test $3 , %esi	/*  STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA ? */
	mov tsuDataList(%gc) , %eax	/*  first entry from offset list */

/*        (float *)pointers += stride */
/*        TRI_VP_SETFS(vPtr, oow) */

	movq %mm2 , -8(%fifo)	/*  PCI write transformed x, y */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_ooz	/*  nope, no color at all needed */

	cmpl $0 , colorType(%gc)	/*  gc->state.vData.colorType == GR_FLOAT ? */
	jne .L_grDrawVertexList_3DNow_Clip_clip_setup_pargb	/*  nope, packed ARGB format */

	test $1 , %esi	/*  STATE_REQUIRES_IT_DRGB ? */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_a	/*  no, but definitely A */

	movd (%edx,%eax) , %mm2	/*  0 | r */
	mov tsuDataList+4(%gc) , %eax	/*  offset of g part of vertex data */

	pfmul %mm6 , %mm2	/*  0 | r * 255.0f */
	movd (%edx,%eax) , %mm3	/*  0 | g */

	mov tsuDataList+8(%gc) , %eax	/*  offset of b part of vertex data */
	movd %mm2 , (%fifo)	/*  PCI write r*255 */

	pfmul %mm6 , %mm3	/*  0 | g * 255.0f */
	movd (%edx,%eax) , %mm2	/*  0 | b */

	movd %mm3 , 4(%fifo)	/*  PCI write g*255 */
	mov $12 , %dataElem	/*  dataElem = 3 */

	pfmul %mm6 , %mm2	/*  0 | b * 255.0f */
	mov tsuDataList+12(%gc) , %eax	/*  offset of A part of vertex data */

	test $2 , %esi	/*  STATE_REQUIRES_IT_ALPHA ? */
	lea 12(%fifo) , %fifo	/*  fifoPtr += 3*sizeof(FxFloat) */

	movd %mm2 , -4(%fifo)	/*  PCI write b*255 */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_ooz	/*  nope, no alpha, proceeed with ooz */

.L_grDrawVertexList_3DNow_Clip_clip_setup_a:
	movd (%eax,%edx) , %mm2	/*  0 | a */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	mov %esp , %esp	/*  filler */
	add $4 , %dataElem	/*  dataElem++  */

	pfmul %mm6 , %mm2	/*  0 | a * 255.0f */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  offset of next part of vertex data */

	movd %mm2 , -4(%fifo)	/*  PCI write a*255 */
	jmp .L_grDrawVertexList_3DNow_Clip_clip_setup_ooz	/*  check whether we need to push out z */

ALIGN(32)

.L_grDrawVertexList_3DNow_Clip_clip_setup_pargb:
	movd (%eax,%edx) , %mm2	/*  get packed ARGB data */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxU32) */

	mov $4 , %dataElem	/*  dataElem = 1 (namely pargb) */
	mov tsuDataList+4(%gc) , %eax	/*  offset of next part of vertex data */

	movd %mm2 , -4(%fifo)	/*  PCI write packed ARGB */

.L_grDrawVertexList_3DNow_Clip_clip_setup_ooz:

	test $4 , %esi	/*  STATE_REQUIRES_OOZ ? */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_qow	/*  nope */

	testl $0x200000 , fbi_fbzMode(%gc)	/*  gc->state.fbi_config.fbzMode & SST_DEPTH_FLOAT_SEL != 0 ? */
	je .L_grDrawVertexList_3DNow_Clip_clip_setup_ooz_nofog	/*  nope */

	cmpl $0 , qInfo_mode(%gc)	/*  gc->state.vData.qInfo.mode == GR_PARAM_ENABLE ? */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_fog_oow	/*  nope */

	mov qInfo_offset(%gc) , %eax	/*  offset of Q component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  0 | q*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed Q */
	jmp .L_grDrawVertexList_3DNow_Clip_clip_setup_qow	/*  check whether we need to write Q or W */

.L_grDrawVertexList_3DNow_Clip_clip_setup_fog_oow:

	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , %dataElem	/*  dataElem++ */

	movd %mm0 , -4(%fifo)	/*  PCI write oow */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

	jmp .L_grDrawVertexList_3DNow_Clip_clip_setup_qow	/*  check whether we need to write Q or W */

.L_grDrawVertexList_3DNow_Clip_clip_setup_ooz_nofog:

	movd (%eax,%edx) , %mm2	/*  0 | z component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem += 1 */
	movd vp_hdepth(%gc) , %mm3	/*  0 | gc->state.Viewport.hdepth */

	pfmul %mm0 , %mm2	/*  TRI_SETF(FARRAY(_s, i)*_oow */
	movd vp_oz(%gc) , %mm4	/*  0 | gc->state.Viewport.oz */

	pfmul %mm3 , %mm2	/*  0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  offset of next vertex component */

	pfadd %mm4 , %mm2	/*  0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz */
	movd %mm2 , -4(%fifo)	/*  PCI write transformed Z */

.L_grDrawVertexList_3DNow_Clip_clip_setup_qow:
	test $8 , %esi	/*  STATE_REQUIRES_OOW_FBI ? */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_qow0	/*  nope */

	cmpl $0 , fogInfo_mode(%gc)	/*  gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE ? */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_oow_nofog	/*  nope, no fog */

	mov fogInfo_offset(%gc) , %eax	/*  offset of fogInfo component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | fogInfo of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  fogInfo*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed Q */
	jmp .L_grDrawVertexList_3DNow_Clip_clip_setup_qow0	/*  continue with q0 */

.L_grDrawVertexList_3DNow_Clip_clip_setup_oow_nofog:

	cmpl $0 , qInfo_mode(%gc)	/*  does vertex have Q component ? */
	je .L_grDrawVertexList_3DNow_Clip_clip_setup_oow	/*  nope, not Q but W */

	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */
	mov qInfo_offset(%gc) , %eax	/*  offset of Q component of vertex */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed Q */
	jmp .L_grDrawVertexList_3DNow_Clip_clip_setup_qow0	/*  continue with q0 */

ALIGN(32)

.L_grDrawVertexList_3DNow_Clip_clip_setup_oow:
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , %dataElem	/*  dataElem++ */

	movd %mm0 , -4(%fifo)	/*  PCI write oow */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

.L_grDrawVertexList_3DNow_Clip_clip_setup_qow0:
	test $16 , %esi	/*  STATE_REQUIRES_W_TMU0 ? */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_stow0	/*  nope  */

	cmpl $0 , q0Info_mode(%gc)	/*  does vertex have Q component ? */
	je .L_grDrawVertexList_3DNow_Clip_clip_setup_oow0	/*  nope, not Q but W */

	mov q0Info_offset(%gc) , %eax	/*  offset of Q component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q0 of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q0*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed q0 */
	jmp .L_grDrawVertexList_3DNow_Clip_clip_setup_stow0	/*  continue with stow0 */

ALIGN(32)

.L_grDrawVertexList_3DNow_Clip_clip_setup_oow0:
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , %dataElem	/*  dataElem++ */

	movd %mm0 , -4(%fifo)	/*  PCI write oow */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

.L_grDrawVertexList_3DNow_Clip_clip_setup_stow0:

	test $32 , %esi	/*  STATE_REQUIRES_ST_TMU0 ? */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_qow1	/*  nope */

	movq tmu0_s_scale(%gc) , %mm7	/*  state.tmu_config[0].t_scale | state.tmu_config[0].s_scale */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movd (%edx,%eax) , %mm2	/*  param1 */
	mov tsuDataList+4(%gc,%dataElem) , %eax	/* pointer to next vertex component */

	pfmul %mm0 , %mm7	/*  oow*tmu0_t_scale | oow*tmu0_s_scale */
	add $8 , %dataElem	/*  dataElem += 2 */

	movd (%edx,%eax) , %mm3	/*  param2 */
	punpckldq %mm3 , %mm2	/*  param2 | param1 */

	pfmul %mm7 , %mm2	/*  param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale */

	movq %mm2 , -8(%fifo)	/*  PCI write param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale  */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

.L_grDrawVertexList_3DNow_Clip_clip_setup_qow1:
	test $64 , %esi	/*  STATE_REQUIRES_W_TMU1 ? */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_stow1	/*  nope */

	cmpl $0 , q1Info_mode(%gc)	/*  does vertex have Q component ? */
	je .L_grDrawVertexList_3DNow_Clip_clip_setup_oow1	/*  nope, not Q but W */

	mov q1Info_offset(%gc) , %eax	/*  offset of Q component of vertex */
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , %dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q1 of vertex */

	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q1*oow */

	movd %mm2 , -4(%fifo)	/*  PCI write transformed q1 */
	jmp .L_grDrawVertexList_3DNow_Clip_clip_setup_stow1	/*  continue with stow1 */

ALIGN(32)

.L_grDrawVertexList_3DNow_Clip_clip_setup_oow1:
	add $4 , %fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , %dataElem	/*  dataElem++ */

	movd %mm0 , -4(%fifo)	/*  PCI write oow */
	mov tsuDataList(%gc,%dataElem) , %eax	/*  pointer to next vertex component */

.L_grDrawVertexList_3DNow_Clip_clip_setup_stow1:

	test $128 , %esi	/*  STATE_REQUIRES_ST_TMU1 ? */
	mov (vertices) , %vertexCount	/*  get number of vertices */

	movq tmu1_s_scale(%gc) , %mm7	/*  state.tmu_config[1].t_scale | state.tmu_config[1].s_scale */
	jz .L_grDrawVertexList_3DNow_Clip_clip_setup_end	/*  nope */

	movd (%edx,%eax) , %mm2	/*  param1 */
	add $8 , %fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov tsuDataList+4(%gc,%dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm7	/*  oow*state.tmu_config[1].t_scale | oow*state.tmu_config[1].s_scale */

	movd (%edx,%eax) , %mm3	/*  param2 */
	punpckldq %mm3 , %mm2	/*  param2 | param1 */

	pfmul %mm7 , %mm2	/*  param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale */
	movq %mm2 , -8(%fifo)	/*  PCI write param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale */

.L_grDrawVertexList_3DNow_Clip_clip_setup_end:

/*  206  :       for (k = 0 k < vcount k++) { */

	dec %vertexCount	/*  vcount-- */
	jnz .L_grDrawVertexList_3DNow_Clip_clip_for_begin	/*  until  */
.L_grDrawVertexList_3DNow_Clip_clip_for_end:

/*  221  :       } */
/*  222  :       TRI_END */

	mov fifoPtr(%gc) , %eax	/*  old fifoPtr */
	mov %esp , %esp	/*  filler */

	nop 	/*  filler */
	mov fifoRoom(%gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , %vertexCount	/*  remaining vertices before previous loop */
	sub %fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov %fifo , fifoPtr(%gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	sub $15 , %vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(%gc)	/*  save current number of bytes available in fifo */

	mov %vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	cmp $0 , %vertexCount	/*  any vertices left to process ? */

	movl $16 , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	jg .L_grDrawVertexList_3DNow_Clip_clip_coords_begin	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state     */

.L_grDrawVertexList_3DNow_Clip_strip_done:
/*     } */
/*   #undef FN_NAME */
/*   } * _grDrawVertexList * */

	pop %ebx	/*  restore caller's register variable */
	pop %ebp	/*  restore frame pointer  */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */

.L_END__grDrawVertexList_3DNow_Clip:
.size _grDrawVertexList_3DNow_Clip,.L_END__grDrawVertexList_3DNow_Clip-_grDrawVertexList_3DNow_Clip


/* -------------------------------------------------------------------------- */
/*  end AMD3D version */
/* -------------------------------------------------------------------------- */
#endif	/*  GL_AMD3D */

/* -------------------------------------------------------------------------- */
/*  start original code */
/* -------------------------------------------------------------------------- */

#ifndef GL_AMD3D

.file "xdraw3.asm"
/*  include listing.inc */
#include "fxgasm.h"

.data
.section	.rodata
	.type	_F1,@object
	.size	_F1,4
_F1:	.int	0x3f800000	/*  1 */
	.type	_F256,@object
	.size	_F256,4
_F256:	.int	0x43800000	/*  256 */

	.type	_VPF1,@object
	.size	_VPF1,4
_VPF1:	.int	0x3f800000	/*  1 */
	.type	_VPF256,@object
	.size	_VPF256,4
_VPF256:	.int	0x43800000	/*  256     */

.data
	.type	vSize,@object
	.size	vSize,4
vSize:	.int	0
	.type	ccoow,@object
	.size	ccoow,4
ccoow:	.int	0
	.type	packetVal,@object
	.size	packetVal,4
packetVal:	.int	0
	.type	strideinbytes,@object
	.size	strideinbytes,4
strideinbytes:	.int	0


	.type	oowa,@object
	.size	oowa,4
oowa:	.int	0
	.type	vPtr0,@object
	.size	vPtr0,4
vPtr0:	.int	0
	.type	vPtr1,@object
	.size	vPtr1,4
vPtr1:	.int	0
	.type	vPtr2,@object
	.size	vPtr2,4
vPtr2:	.int	0

.text


#define _pktype 20
#define _type 24
#define _mode 28
#define _count 32
#define _pointers 36

#define gc esi	/*  points to graphics context */
#define fifo ecx	/*  points to next entry in fifo */
#define dlp ebp	/*  points to dataList structure */
#define vertexCount ebx	/*  Current vertex counter in the packet */
#define vertexPtr edi	/*  Current vertex pointer */

ALIGN(32)

GLOBL(_drawvertexlist)
/*  132  : { */

#if 0	
	movl (0x18) , %eax	/*  get thread local storage base pointer         */
	push %esi

	mov (EXTRN(_GlideRoot)+tlsOffset) , %esi	/*  GC position relative to tls base */
	push %edi

	movl (%eax,%esi) , %gc
	push %ebx
#else
	push %esi
	push %edi
	push %ebx
	movl (THREADVALUE) , %gc			
#endif
	
/*      GR_DCL_GC */
/*      vSize = gc->state.vData.vSize */
/*      if (stride == 0) */
/*        stride = gc->state.vData.vStride */
	push %ebp
	movl vertexSize(%gc) , %ecx

	movl _mode(%esp) , %edx
	movl _count(%esp) , %vertexCount

	movl _pointers(%esp) , %vertexPtr
	movl %ecx , vSize

	shl $2 , %edx
/*      mov     ecx, DWORD PTR [gc+CoordinateSpace] */
	test %edx , %edx
	jne .L_drawvertexlist_no_stride
	movl vertexStride(%gc) , %edx
	shl $2 , %edx

ALIGN(4)
.L_drawvertexlist_no_stride:

/*      Draw the first (or possibly only) set.  This is necessary because */
/*      the packet is 3_BDDDDDD, and in the next set, the packet is 3_DDDDDD */
/*      We try to make tstrip code simple to read. We combine the original code */
/*      into a single loop by adding an extra packet type assignment at the end of the loop. */
/*   */
/*      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) { */

/*      test    ecx, ecx */
	movl %edx , strideinbytes

/*        while (count > 0) { */
/*          FxI32 k, vcount = count >= 15 ? 15 : count */
/*          GR_SET_EXPECTED_SIZE(vcount * vSize, 1) */
/*          TRI_STRIP_BEGIN(type, vcount, vSize, pktype) */

	movl _count(%esp) , %eax
/*        jne     clip_coordinates */

	test %eax , %eax
	jle .L_drawvertexlist_strip_done

ALIGN(4)
.L_drawvertexlist_window_coords_begin:

	cmp $15 , %vertexCount	/*  0000000fH */
	jl .L_drawvertexlist_win_partial_packet
	mov $15 , %vertexCount	/*  0000000fH */

ALIGN(4)
.L_drawvertexlist_win_partial_packet:

	movl vSize , %eax
	movl fifoRoom(%gc) , %ecx
	imul %vertexCount , %eax
	add $4 , %eax
	cmp %eax , %ecx
	jge .L_drawvertexlist_win_strip_begin
	pushl $__LINE__
	push $0x0
	push %eax
	call EXTRN(_grCommandTransportMakeRoom)
	add $12, %esp

ALIGN(4)
.L_drawvertexlist_win_strip_begin:

/*      Setup pacet header */

	movl fifoPtr(%gc) , %fifo
	mov %vertexCount , %eax
	movl _type(%esp) , %edx
	movl cullStripHdr(%gc) , %ebp
	shl $22 , %edx	/*  00000010H */
	add $4 , %fifo
	shl $6 , %eax
	or %edx , %ebp
	or %ebp , %eax
	movl _pktype(%esp) , %edx
	or %edx , %eax
	nop 
	movl %eax , -4(%fifo)

/*      for (k = 0 k < vcount k++) { */
/*        FxI32 i */
/*        FxU32 dataElem */
/*        float *vPtr */
/*        vPtr = pointers */
/*        if (mode) */
/*          vPtr = *(float **)vPtr */
/*        (float *)pointers += stride */
/*        TRI_SETF(FARRAY(vPtr, 0)) */
/*        dataElem = 0 */
/*        TRI_SETF(FARRAY(vPtr, 4)) */
/*        i = gc->tsuDataList[dataElem] */

ALIGN(4)
.L_drawvertexlist_win_for_begin:

	mov %vertexPtr , %edx
	movl strideinbytes , %eax
	cmp $4 , %eax
	jne .L_drawvertexlist_win_no_deref
	movl (%vertexPtr) , %edx

ALIGN(4)
.L_drawvertexlist_win_no_deref:

	add $8 , %fifo
	add %eax , %vertexPtr

	movl (%edx) , %eax
	movl 4(%edx) , %ebp

	movl %eax , -8(%fifo)
	movl tsuDataList(%gc) , %eax

	movl %ebp , -4(%fifo)

	test %eax , %eax
	leal tsuDataList(%gc) , %dlp
	je .L_drawvertexlist_win_datalist_end

ALIGN(4)

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L_drawvertexlist_win_datalist_begin:

	add $4 , %fifo
	add $4 , %dlp

	movl (%edx,%eax) , %eax
	nop 

	movl %eax , -4(%fifo)
	movl (%dlp) , %eax


	test %eax , %eax
	jne .L_drawvertexlist_win_datalist_begin
.L_drawvertexlist_win_datalist_end:

	dec %vertexCount
	jne .L_drawvertexlist_win_for_begin
.L_drawvertexlist_win_for_end:

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	movl fifoPtr(%gc) , %eax
	movl fifoRoom(%gc) , %edx
	sub %fifo , %eax
	movl _count(%esp) , %vertexCount
	add %eax , %edx
	sub $15 , %vertexCount	/*  0000000fH */

	movl %edx , fifoRoom(%gc)
	movl %vertexCount , _count(%esp)

	movl %fifo , fifoPtr(%gc)
	test %vertexCount , %vertexCount

	movl $16 , _pktype(%esp)	/*  00000010H */
	jg .L_drawvertexlist_window_coords_begin

.L_drawvertexlist_strip_done:
	pop %ebp
	pop %ebx
	pop %edi
	pop %esi
	ret	/*  00000014H */

.L_END__drawvertexlist:
.size _drawvertexlist,.L_END__drawvertexlist-_drawvertexlist

#define _pktype 20
#define _type 24
#define _mode 28
#define _count 32
#define _pointers 36

#define gc esi	/*  points to graphics context */
#define fifo ecx	/*  points to next entry in fifo */
#undef vertexPtr
#define vertexPtr edx	/*  pointer to vertex or vertex array */

ALIGN(32)

GLOBL(_vpdrawvertexlist)

	push %esi

	push %edi

	push %ebx
	movl (THREADVALUE) , %gc

	push %ebp
	movl _mode(%esp) , %ecx

	movl _pointers(%esp) , %edi
	movl wInfo_offset(%gc) , %eax

	test %ecx , %ecx
	je .L_vpdrawvertexlist_w_no_dref

	movl (%edi) , %edi

ALIGN(4)
.L_vpdrawvertexlist_w_no_dref:

/*      load first w */

	flds (%edi,%eax)
	fdivrs _F1

	movl vertexSize(%gc) , %ecx
	movl _mode(%esp) , %edx

	movl _count(%esp) , %edi
/*      mov     vertexArray, DWORD PTR [esp+_pointers] */

	shl $2 , %edx
	movl %ecx , vSize

	test %edx , %edx

	jne .L_vpdrawvertexlist_no_stride

	movl vertexStride(%gc) , %edx
	shl $2 , %edx

ALIGN(4)
.L_vpdrawvertexlist_no_stride:

	movl %edx , strideinbytes
	movl _type(%esp) , %eax

	shl $16 , %eax	/*  00000010H */
	movl %eax , packetVal

.L_vpdrawvertexlist_clip_coords_begin:

	cmp $15 , %edi
	jl .L_vpdrawvertexlist_clip_partial_packet
	mov $15 , %edi
.L_vpdrawvertexlist_clip_partial_packet:

/*      GR_SET_EXPECTED_SIZE(vcount * vSize, 1) */

	movl vSize , %eax
	movl fifoRoom(%gc) , %ecx

	imul %edi , %eax
	add $4 , %eax
	cmp %eax , %ecx
	jge .L_vpdrawvertexlist_clip_strip_begin
	pushl $__LINE__
	push $0x0
	push %eax
	call EXTRN(_grCommandTransportMakeRoom)
	add $12, %esp

ALIGN(4)
.L_vpdrawvertexlist_clip_strip_begin:
/*      TRI_STRIP_BEGIN(type, vcount, vSize, pktype) */


	movl fifoPtr(%gc) , %fifo
	mov %edi , %eax

	movl packetVal , %edx
	movl cullStripHdr(%gc) , %ebp

	or %edx , %eax
	add $4 , %fifo

	shl $6 , %eax
	movl _pktype(%esp) , %edx

	or %ebp , %eax

	or %edx , %eax
	movl %eax , -4(%fifo)


	movl _pointers(%esp) , %vertexPtr
	movl _mode(%esp) , %eax

	test %eax , %eax

	je .L_vpdrawvertexlist_clip_for_begin
	movl (%vertexPtr) , %vertexPtr

ALIGN(4)
.L_vpdrawvertexlist_clip_for_begin:

	add $8 , %fifo
	movl strideinbytes , %ebp

	addl %ebp , _pointers(%esp)
	movl paramIndex(%gc) , %eax

	xor %ebp , %ebp
	movl tsuDataList(%gc) , %ebx

/*     setup x and y */

	flds vp_hwidth(%gc)
	fmuls (%vertexPtr)
	fmul %st(1) , %st
	fadds vp_ox(%gc)
	fxch %st(1)

	flds vp_hheight(%gc)
	fmuls 4(%vertexPtr)
	test $3 , %al
	fmul %st(1) , %st
	fadds vp_oy(%gc)
	fxch %st(1)
	fstps ccoow
	fxch %st(1)
	fstps -8(%fifo)
	fstps -4(%fifo)

/*     set up color */

	je .L_vpdrawvertexlist_clip_setup_ooz

	cmpl %ebp , colorType(%gc)
	jne .L_vpdrawvertexlist_clip_setup_pargb

	test $1 , %al
	je .L_vpdrawvertexlist_clip_setup_a

	add $12 , %fifo
	mov $3 , %ebp

	flds EXTRN(_GlideRoot)+pool_f255
	fmuls (%ebx,%vertexPtr)
	flds EXTRN(_GlideRoot)+pool_f255
	fmuls 4(%ebx,%vertexPtr)
	flds EXTRN(_GlideRoot)+pool_f255
	fmuls 8(%ebx,%vertexPtr)
	fxch %st(2)
	fstps -12(%fifo)
	fstps -8(%fifo)
	fstps -4(%fifo)
	movl tsuDataList+12(%gc) , %ebx

ALIGN(4)
.L_vpdrawvertexlist_clip_setup_a:

	test $2 , %al
	je .L_vpdrawvertexlist_clip_setup_ooz

	add $4 , %fifo
	inc %ebp

	flds (%ebx,%vertexPtr)
	fmuls EXTRN(_GlideRoot)+pool_f255
	fstps -4(%fifo)

	movl tsuDataList(%gc,%ebp,4) , %ebx
	jmp .L_vpdrawvertexlist_clip_setup_ooz

ALIGN(4)
.L_vpdrawvertexlist_clip_setup_pargb:
	add $4 , %fifo
	movl (%ebx,%vertexPtr) , %ebx

	movl %ebx , -4(%fifo)
	nop 

	mov $1 , %ebp
	movl tsuDataList+4(%gc) , %ebx
ALIGN(4)
.L_vpdrawvertexlist_clip_setup_ooz:

	test $4 , %al
	je .L_vpdrawvertexlist_clip_setup_qow

	add $4 , %fifo
	inc %ebp

	testl $0x200000 , fbi_fbzMode(%gc)
	je .L_vpdrawvertexlist_clip_setup_ooz_nofog

	movl qInfo_mode(%gc) , %ebx
	test %ebx , %ebx
	je .L_vpdrawvertexlist_clip_setup_fog_oow
	movl qInfo_offset(%gc) , %ebx

	flds (%vertexPtr,%ebx)
	fmuls ccoow
	fstps -4(%fifo)

	movl tsuDataList(%gc,%ebp,4) , %ebx
	jmp .L_vpdrawvertexlist_clip_setup_qow

ALIGN(4)
.L_vpdrawvertexlist_clip_setup_fog_oow:
	movl ccoow , %ebx

	movl %ebx , -4(%fifo)
	movl tsuDataList(%gc,%ebp,4) , %ebx
	jmp .L_vpdrawvertexlist_clip_setup_qow

ALIGN(4)
.L_vpdrawvertexlist_clip_setup_ooz_nofog:

	flds (%ebx,%vertexPtr)
	fmuls vp_hdepth(%gc)
	fmuls ccoow
	fadds vp_oz(%gc)
	fstps -4(%fifo)

	movl tsuDataList(%gc,%ebp,4) , %ebx
ALIGN(4)
.L_vpdrawvertexlist_clip_setup_qow:

	test $8 , %al
	je .L_vpdrawvertexlist_clip_setup_qow0

	movl fogInfo_mode(%gc) , %ebx
	test %ebx , %ebx
	je .L_vpdrawvertexlist_clip_setup_oow_nofog
	movl fogInfo_offset(%gc) , %ebx

	flds (%vertexPtr,%ebx)
	fmuls ccoow
	fstps (%fifo)

	jmp .L_vpdrawvertexlist_clip_setup_oow_inc

ALIGN(4)
.L_vpdrawvertexlist_clip_setup_oow_nofog:

	movl qInfo_mode(%gc) , %ebx
	test %ebx , %ebx
	je .L_vpdrawvertexlist_clip_setup_oow
	movl qInfo_offset(%gc) , %ebx

	flds (%vertexPtr,%ebx)
	fmuls ccoow
	fstps (%fifo)

	jmp .L_vpdrawvertexlist_clip_setup_oow_inc

ALIGN(4)
.L_vpdrawvertexlist_clip_setup_oow:
	movl ccoow , %ebx

	movl %ebx , (%fifo)
ALIGN(4)
.L_vpdrawvertexlist_clip_setup_oow_inc:

	movl tsuDataList+4(%gc,%ebp,4) , %ebx
	add $4 , %fifo

	inc %ebp
ALIGN(4)
.L_vpdrawvertexlist_clip_setup_qow0:

	test $16 , %al
	je .L_vpdrawvertexlist_clip_setup_stow0

	movl q0Info_mode(%gc) , %ebx
	cmp $1 , %ebx
	jne .L_vpdrawvertexlist_clip_setup_oow0

	movl q0Info_offset(%gc) , %ebx

	flds (%ebx,%vertexPtr)
	fmuls ccoow
	fstps (%fifo)

	jmp .L_vpdrawvertexlist_clip_setup_oow0_inc
ALIGN(4)
.L_vpdrawvertexlist_clip_setup_oow0:
	movl ccoow , %ebx

	movl %ebx , (%fifo)
ALIGN(4)
.L_vpdrawvertexlist_clip_setup_oow0_inc:
	movl tsuDataList+4(%gc,%ebp,4) , %ebx
	add $4 , %fifo

	inc %ebp
ALIGN(4)
.L_vpdrawvertexlist_clip_setup_stow0:

	test $32 , %al
	je .L_vpdrawvertexlist_clip_setup_qow1


	flds ccoow
	fmuls (%ebx,%vertexPtr)

	add $8 , %fifo
	add $2 , %ebp

	fmuls tmu0_s_scale(%gc)
	flds ccoow
	fmuls 4(%ebx,%vertexPtr)
	movl tsuDataList(%gc,%ebp,4) , %ebx
	fmuls tmu0_t_scale(%gc)
	fxch 
	fstps -8(%fifo)
	fstps -4(%fifo)

ALIGN(4)
.L_vpdrawvertexlist_clip_setup_qow1:

	test $64 , %al
	je .L_vpdrawvertexlist_clip_setup_stow1

	movl q1Info_mode(%gc) , %ebx
	cmp $1 , %ebx
	jne .L_vpdrawvertexlist_clip_setup_oow1

	movl q1Info_offset(%gc) , %ebx

	flds (%ebx,%vertexPtr)
	fmuls ccoow
	fstps (%fifo)

	jmp .L_vpdrawvertexlist_clip_setup_oow1_inc
ALIGN(4)
.L_vpdrawvertexlist_clip_setup_oow1:
	movl ccoow , %ebx

	movl %ebx , (%fifo)
ALIGN(4)
.L_vpdrawvertexlist_clip_setup_oow1_inc:

	movl tsuDataList+4(%gc,%ebp,4) , %ebx
	add $4 , %fifo

	inc %ebp

ALIGN(4)
.L_vpdrawvertexlist_clip_setup_stow1:

	test $128 , %al
	je .L_vpdrawvertexlist_clip_setup_end

	flds ccoow
	fmuls (%ebx,%vertexPtr)
	add $8 , %fifo
	fmuls tmu1_s_scale(%gc)
	flds ccoow
	fmuls 4(%ebx,%vertexPtr)
	movl tsuDataList+4(%gc,%ebp,4) , %ebx
	fmuls tmu1_t_scale(%gc)
	fxch 
	fstps -8(%fifo)
	fstps -4(%fifo)

ALIGN(4)
.L_vpdrawvertexlist_clip_setup_end:

	dec %edi
	jz .L_vpdrawvertexlist_clip_for_end

	movl _pointers(%esp) , %vertexPtr
	movl _mode(%esp) , %ebx

	test %ebx , %ebx
	je .L_vpdrawvertexlist_w_clip_no_deref


	movl (%vertexPtr) , %vertexPtr
ALIGN(4)
.L_vpdrawvertexlist_w_clip_no_deref:

	movl wInfo_offset(%gc) , %ebx

	flds (%ebx,%vertexPtr)
	fdivrs _F1

	jmp .L_vpdrawvertexlist_clip_for_begin
ALIGN(4)
.L_vpdrawvertexlist_clip_for_end:

	movl fifoPtr(%gc) , %ebx
	movl fifoRoom(%gc) , %edx

	sub %fifo , %ebx
	movl _count(%esp) , %edi

	add %ebx , %edx
	sub $15 , %edi	/*  0000000fH */

	movl %edx , fifoRoom(%gc)
	movl %edi , _count(%esp)

	movl %fifo , fifoPtr(%gc)
	movl $16 , _pktype(%esp)	/*  00000010H */

	jle .L_vpdrawvertexlist_strip_done
	movl _pointers(%esp) , %edx

	movl _mode(%esp) , %ebx
	test %ebx , %ebx

	je .L_vpdrawvertexlist_w1_clip_no_deref
	movl (%edx) , %edx

ALIGN(4)
.L_vpdrawvertexlist_w1_clip_no_deref:

	movl wInfo_offset(%gc) , %ebx
	flds (%ebx,%edx)
	fdivrs _F1

	jmp .L_vpdrawvertexlist_clip_coords_begin
ALIGN(4)
.L_vpdrawvertexlist_strip_done:

	pop %ebp
	pop %ebx
	pop %edi
	pop %esi
	ret	/*  00000014H */
.L_END__vpdrawvertexlist:
.size _vpdrawvertexlist,.L_END__vpdrawvertexlist-_vpdrawvertexlist

#define gc esi	/*  points to graphics context */
#define fifo ecx	/*  points to next entry in fifo */
#undef vertexPtr
#define vertexPtr edi	/*  Current vertex pointer */

/*  NB:  All of the base triangle procs expect to have the gc */
/*       passed from the caller in edx so that we can avoid */
/*       the agi from the far pointer. Screw w/ this at your */
/*       own peril. */

/*       YOU HAVE BEEN WARNED         */

ALIGN(32)

GLOBL(_vptrisetup_cull)
#define _va 20
#define _vb 24
#define _vc 28
	push %ebx
	push %esi

	push %edi
	mov %edx , %gc

	movl _va-4(%esp) , %ecx
	movl wInfo_offset(%gc) , %eax

	push %ebp
	nop 

/*     oow[0] = 1.0f / FARRAY(va, gc->state.vData.wInfo.offset) */

	flds (%eax,%ecx)

	fdivrs _F1

	movl _vb(%esp) , %ecx
	movl _vc(%esp) , %ebx

	nop 
	nop 

	movl (%eax,%ecx) , %ebp
	movl (%eax,%ebx) , %edi

	movl %ebp , vPtr1
	movl %edi , vPtr2

/*     GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize, 1) */

	movl EXTRN(_GlideRoot)+curTriSize , %eax
	movl fifoRoom(%gc) , %ecx

	add $4 , %eax
	nop 

	cmp %eax , %ecx
	jge .L_vptrisetup_cull_setup_pkt_hdr

	pushl $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %eax
	call EXTRN(_grCommandTransportMakeRoom)
	add $12, %esp

ALIGN(4)
.L_vptrisetup_cull_setup_pkt_hdr:

/*     TRI_STRIP_BEGIN(kSetupStrip, 3, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD) */


	movl fifoPtr(%gc) , %fifo
	movl cullStripHdr(%gc) , %eax

	add $4 , %fifo
	leal _va(%esp) , %ebp

	or $192 , %eax	/*  000000c0H */
	mov $0 , %edx

	movl %eax , -4(%fifo)
	movl (%ebp) , %vertexPtr

	movl paramIndex(%gc) , %eax
	nop 

/*  Begin loop */

ALIGN(4)
.L_vptrisetup_cull_begin_for_loop:

	add $4 , %edx
	add $8 , %fifo

	xor %ebx , %ebx
	movl tsuDataList(%gc) , %ebp

/*     setup x and y */

	flds vp_hwidth(%gc)
	fmuls (%vertexPtr)
	fmul %st(1) , %st
	fadds vp_ox(%gc)
	fxch %st(1)

	flds 4(%vertexPtr)
	fmuls vp_hheight(%gc)
	test $3 , %al
	fmul %st(1) , %st
	fadds vp_oy(%gc)
	fxch %st(1)
	fstps oowa
	fxch %st(1)
	fstps -8(%fifo)
	fstps -4(%fifo)

/*     set up color */

	je .L_vptrisetup_cull_clip_setup_ooz

	cmpl %ebx , colorType(%gc)
	jne .L_vptrisetup_cull_clip_setup_pargb

	test $1 , %al
	je .L_vptrisetup_cull_clip_setup_a

	add $12 , %fifo
	add $3 , %ebx

	flds EXTRN(_GlideRoot)+pool_f255
	fmuls (%vertexPtr,%ebp)
	flds EXTRN(_GlideRoot)+pool_f255
	fmuls 4(%vertexPtr,%ebp)
	flds EXTRN(_GlideRoot)+pool_f255
	fmuls 8(%vertexPtr,%ebp)
	fxch %st(2)
	fstps -12(%fifo)
	fstps -8(%fifo)
	fstps -4(%fifo)
	movl tsuDataList+12(%gc) , %ebp

ALIGN(4)
.L_vptrisetup_cull_clip_setup_a:

	test $2 , %al
	je .L_vptrisetup_cull_clip_setup_ooz

	add $4 , %fifo
	inc %ebx

	flds (%vertexPtr,%ebp)
	fmuls EXTRN(_GlideRoot)+pool_f255
	fstps -4(%fifo)

	movl tsuDataList(%gc, %ebx, 4) , %ebp
	jmp .L_vptrisetup_cull_clip_setup_ooz
ALIGN(4)
.L_vptrisetup_cull_clip_setup_pargb:
	add $4 , %fifo
	movl (%vertexPtr,%ebp) , %ebx

	movl %ebx , -4(%fifo)
	nop 

	mov $1 , %ebx
	movl tsuDataList+4(%gc) , %ebp
ALIGN(4)
.L_vptrisetup_cull_clip_setup_ooz:

	test $4 , %al
	je .L_vptrisetup_cull_clip_setup_qow

	add $4 , %fifo
	inc %ebx

	testl $0x200000 , fbi_fbzMode(%gc)
	je .L_vptrisetup_cull_clip_setup_ooz_nofog

	movl qInfo_mode(%gc) , %ebp
	test %ebp , %ebp
	je .L_vptrisetup_cull_clip_setup_fog_oow
	movl qInfo_offset(%gc) , %ebp

	flds (%vertexPtr,%ebp)
	fmuls oowa
	fstps -4(%fifo)

	movl tsuDataList(%gc, %ebx, 4) , %ebp
	jmp .L_vptrisetup_cull_clip_setup_qow

ALIGN(4)
.L_vptrisetup_cull_clip_setup_fog_oow:
	movl oowa , %ebp

	movl %ebp , -4(%fifo)
	movl tsuDataList(%gc, %ebx, 4) , %ebp
	jmp .L_vptrisetup_cull_clip_setup_qow

ALIGN(4)
.L_vptrisetup_cull_clip_setup_ooz_nofog:

	flds (%vertexPtr,%ebp)
	fmuls vp_hdepth(%gc)
	fmuls oowa
	fadds vp_oz(%gc)
	fstps -4(%fifo)

	movl tsuDataList(%gc, %ebx, 4) , %ebp
ALIGN(4)
.L_vptrisetup_cull_clip_setup_qow:

	test $8 , %al
	je .L_vptrisetup_cull_clip_setup_qow0

	cmpl $1 , fogInfo_mode(%gc)
	jne .L_vptrisetup_cull_clip_setup_oow_nofog

	movl fogInfo_offset(%gc) , %ebp
	flds oowa
	fmuls (%ebp,%vertexPtr)
	fstps (%fifo)

	jmp .L_vptrisetup_cull_clip_setup_oow_inc
ALIGN(4)
.L_vptrisetup_cull_clip_setup_oow_nofog:
	cmpl $1 , qInfo_mode(%gc)
	jne .L_vptrisetup_cull_clip_setup_oow

	movl qInfo_offset(%gc) , %ebp
	flds oowa
	fmuls (%ebp,%vertexPtr)
	fstps (%fifo)

	jmp .L_vptrisetup_cull_clip_setup_oow_inc
ALIGN(4)
.L_vptrisetup_cull_clip_setup_oow:

	movl oowa , %ebp

	movl %ebp , (%fifo)
ALIGN(4)
.L_vptrisetup_cull_clip_setup_oow_inc:
	movl tsuDataList+4(%gc, %ebx, 4) , %ebp
	add $4 , %fifo

	inc %ebx
ALIGN(4)
.L_vptrisetup_cull_clip_setup_qow0:

	test $16 , %al	/*  00000010H */
	je .L_vptrisetup_cull_clip_setup_stow0

	cmpl $1 , q0Info_mode(%gc)
	jne .L_vptrisetup_cull_clip_setup_oow0

	movl q0Info_offset(%gc) , %ebp

	flds oowa
	fmuls (%ebp,%vertexPtr)
	fstps (%fifo)

	jmp .L_vptrisetup_cull_clip_setup_oow0_inc
ALIGN(4)
.L_vptrisetup_cull_clip_setup_oow0:
	movl oowa , %ebp

	movl %ebp , (%fifo)
ALIGN(4)
.L_vptrisetup_cull_clip_setup_oow0_inc:
	movl tsuDataList+4(%gc, %ebx, 4) , %ebp
	add $4 , %fifo

	inc %ebx
ALIGN(4)
.L_vptrisetup_cull_clip_setup_stow0:

	test $32 , %al
	je .L_vptrisetup_cull_clip_setup_qow1


	flds oowa
	fmuls (%vertexPtr,%ebp)

	add $8 , %fifo
	add $2 , %ebx

	fmuls tmu0_s_scale(%gc)
	flds oowa
	fmuls 4(%vertexPtr,%ebp)
	movl tsuDataList(%gc, %ebx, 4) , %ebp
	fmuls tmu0_t_scale(%gc)
	fxch 
	fstps -8(%fifo)
	fstps -4(%fifo)

ALIGN(4)
.L_vptrisetup_cull_clip_setup_qow1:

	test $64 , %al
	je .L_vptrisetup_cull_clip_setup_stow1

	cmpl $1 , q1Info_mode(%gc)
	jne .L_vptrisetup_cull_clip_setup_oow1

	movl q1Info_offset(%gc) , %ebp

	flds (%ebp,%vertexPtr)
	fmuls oowa
	fstps (%fifo)

	jmp .L_vptrisetup_cull_clip_setup_oow1_inc
ALIGN(4)
.L_vptrisetup_cull_clip_setup_oow1:
	movl oowa , %ebp

	movl %ebp , (%fifo)
ALIGN(4)
.L_vptrisetup_cull_clip_setup_oow1_inc:
	movl tsuDataList+4(%gc, %ebx, 4) , %ebp
	add $4 , %fifo

	inc %ebx
ALIGN(4)
.L_vptrisetup_cull_clip_setup_stow1:

	test $128 , %al
	je .L_vptrisetup_cull_clip_setup_end


	flds oowa
	fmuls (%vertexPtr,%ebp)
	add $8 , %fifo
	fmuls tmu1_s_scale(%gc)
	flds oowa
	fmuls 4(%vertexPtr,%ebp)
	fmuls tmu1_t_scale(%gc)
	fxch 
	fstps -8(%fifo)
	fstps -4(%fifo)

ALIGN(4)
.L_vptrisetup_cull_clip_setup_end:

	cmp $12 , %edx
	je .L_vptrisetup_cull_update_fifo_ptr

	flds (%edx)
	fdivrs _F1

	leal _va(%esp) , %ebx
	movl wInfo_offset(%gc) , %ebp

	movl (%ebx,%edx) , %vertexPtr
	jmp .L_vptrisetup_cull_begin_for_loop

ALIGN(4)
.L_vptrisetup_cull_update_fifo_ptr:

	movl fifoPtr(%gc) , %ebx
	movl fifoRoom(%gc) , %edx

	sub %fifo , %ebx
	mov $1 , %eax

	add %ebx , %edx
	pop %ebp

	movl %edx , fifoRoom(%gc)
	pop %edi

	movl %fifo , fifoPtr(%gc)
	movl EXTRN(_GlideRoot)+trisProcessed , %ebx

/*     _GlideRoot.stats.trisProcessed++ */


	pop %esi
	inc %ebx

	movl %ebx , EXTRN(_GlideRoot)+trisProcessed
	pop %ebx

	ret	/*  0000000cH */

.L_END_vptrisetup_cull:
.size _vptrisetup_cull,.L_END_vptrisetup_cull-_vptrisetup_cull

#endif	/*  !GL_AMD3D */

.end
