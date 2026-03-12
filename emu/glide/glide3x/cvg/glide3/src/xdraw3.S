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

/* -------------------------------------------------------------------------- */
/*  start AMD3D version */
/* -------------------------------------------------------------------------- */

#ifdef GL_AMD3D

.file "xdraw3.asm"

/*  include listing.inc */
#include "fxgasm.h"


.data
.section	.rodata
.align 8
	.type	_F256_F256,@object
	.size	_F256_F256,8
_F256_F256	.double	0x04380000043800000	/*  256 | 256 */

.data
.align 8
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


#define _pktype 20
#define _type 24
#define _mode 28
#define _count 32
#define _pointers 36

#define gc %edi	/*  points to graphics context */
#define fifo %ecx	/*  points to next entry in fifo */
#define dlp %ebp	/*  points to dataList structure */
#define vertexCount %esi	/*  Current vertex counter in the packet */
#define vertexPtr %ebx	/*  Current vertex pointer (in deref mode) */
#define vertex %ebx	/*  Current vertex (in non-deref mode) */
#define dlpStart %edx	/*  Pointer to start of offset list */

#define X 0
#define Y 4

.align 32

.globl __grDrawVertexList_3DNow_Window
.type __grDrawVertexList_3DNow_Window,@function
__grDrawVertexList_3DNow_Window:
/*  132  : { */

	push %edi	/*  save caller's register variable */
	mov (_GlideRoot+curGC) , gc	/*  get current graphics context */

	push %esi	/*  save caller's register variable */
	mov _count-8(%esp) , vertexCount	/*  number of vertices in strip/fan */

	push %ebx	/*  save caller's register variable */
	push %ebp	/*  save frame pointer */

	mov _pointers(%esp) , vertexPtr	/*  get current vertex pointer (deref mode) */
/*  get current vertex (non-deref mode) */
	test vertexCount , vertexCount	/*  number of vertices <= 0 ? */

	jle .L__grDrawVertexList_3DNow_Window_strip_done	/*  yup, the strip/fan is done */

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
	mov vertexSize(gc) , %eax	/*  size of vertex data in bytes */

	test %edx , %edx	/*  mode 0 (array of vertices) ? */
	mov vertexStride(gc) , %edx	/*  get stride in DWORDs */

	jnz deref_mode	/*  nope, it's mode 1 (array of pointers to vertices) */

	femms 	/*  we'll use MMX clear MMX/3DX state       */

	shl $2 , %edx	/*  stride in bytes */
	mov %edx , (strideinbytes)	/*  save off stride (in bytes) */

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


.L__grDrawVertexList_3DNow_Window_win_coords_loop_ND:

	sub $15 , vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L__grDrawVertexList_3DNow_Window_win_strip_begin_ND	/*  yup, start writing strip data */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	lea 12(%esp) , %esp	/*  remove 3 DWORD arguments from stack */


.L__grDrawVertexList_3DNow_Window_win_strip_begin_ND:

/*      Setup packet header */

	mov vertexCount , %eax	/*  number of vertices in strip/fan */
	mov _type(%esp) , %edx	/*  setup mode */

	mov fifoPtr(gc) , fifo	/*  get fifoPtr */
	shl $22 , %edx	/*  <27:22> = setup mode (kSetupStrip or kSetupFan) */

	mov cullStripHdr(gc) , %ebp	/*  <2:0> = type */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	or %edx , %eax	/*  setup mode and vertex count */
	mov _pktype(%esp) , %edx	/*  <5:3> = command (SSTCP_PKT3_BDDBDD, SSTCP_PKT3_BDDDDD, or SSTCP_PKT3_DDDDDD) */

	or %ebp , %eax	/*  setup mode, vertex count, and type */
	mov $4 , %ebp	/*  test bit 2 */

	or %edx , %eax	/*  setup mode, vertex count, type, and command */
	lea tsuDataList(gc) , dlpStart	/*  pointer to start of offset list */

	test %ebp , fifo	/*  fifoPtr QWORD aligned ? */
	jz .L__grDrawVertexList_3DNow_Window_fifo_aligned_ND	/*  yup */

	mov %eax , (fifo)	/*  PCI write packet type */
	add $4 , fifo	/*  fifo pointer now QWORD aligned */

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

.L__grDrawVertexList_3DNow_Window_win_vertex_loop_ND_WB0:/*  nothing in "write buffer" */

	mov (dlpStart) , %eax	/*  get first offset from offset list */
	lea 4(dlpStart) , dlp	/*  point to start of offset list */

	movq X(vertex) , %mm1	/*  get vertex x,y */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	nop 	/*  filler */
	test %eax , %eax	/*  if offset == 0, end of list */

	movq %mm1 , -8(fifo)	/*  PCI write x, y */
	jz .L__grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB0	/*  no more vertex data, nothing in "write buffer"  */

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L__grDrawVertexList_3DNow_Window_win_datalist_loop_ND_WB0:/*  nothing in "write buffer" */

	movd (vertex,%eax) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz .L__grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB1	/*  exit, write buffer contains one DWORD */

	movd (vertex,%eax) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp++ */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	punpckldq %mm2 , %mm1	/*  current param | previous param */

	movq %mm1 , -8(fifo)	/*  PCI write current param | previous param */
	jnz win_datalist_loop_ND_WB0	/*  nope, copy next parameter */

.L__grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB0:

	mov (strideinbytes) , %eax	/*  get offset to next vertex */
	sub $1 , vertexCount	/*  another vertex done. Any left? */

	lea (vertex,%eax) , vertex	/*  points to next vertex */
	jnz win_vertex_loop_ND_WB0	/*  yup, output next vertex */

.L__grDrawVertexList_3DNow_Window_win_vertex_end_ND_WB0:

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	mov $16 , %ebp	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoPtr(gc) , %eax	/*  old fifoPtr */

	mov %ebp , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	sub $15 , vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	test vertexCount , vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L__grDrawVertexList_3DNow_Window_win_coords_loop_ND	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restire caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */


.L__grDrawVertexList_3DNow_Window_fifo_aligned_ND:

	movd %eax , %mm1	/*  move header into "write buffer" */

.L__grDrawVertexList_3DNow_Window_win_vertex_loop_ND_WB1:/*  one DWORD in "write buffer" */

	movd X(vertex) , %mm2	/*  0 | x of vertex */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	lea 4(dlpStart) , dlp	/*  point to start of offset list */
	nop 	/*  filler */

	punpckldq %mm2 , %mm1	/*  packet header | x of vertex */
	mov -4(dlp) , %eax	/*  first offset in offset list */

	movq %mm1 , -8(fifo)	/*  PCI write packet header | x of vertex */
	movd Y(vertex) , %mm1	/*  0 | y of vertex */

	cmp $0 , %eax	/*  offset == 0 (list empty) ? */
	jz .L__grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB1	/*  yup, no more vertex data, one DWORD in "write buffer" */

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L__grDrawVertexList_3DNow_Window_win_datalist_loop_ND_WB1:/*  one DWORD in "write buffer"  */

	movd (vertex,%eax) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(fifo)	/*  PCI write current param | previous param */
	jz .L__grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB0	/*  yes, exit, "write buffer" empty */

	movd (vertex,%eax) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz win_datalist_loop_ND_WB1	/*  nope, copy next parameter */

.L__grDrawVertexList_3DNow_Window_win_datalist_end_ND_WB1:

	mov (strideinbytes) , %eax	/*  get offset to next vertex */
	sub $1 , vertexCount	/*  another vertex done. Any left? */

	lea (vertex,%eax) , vertex	/*  points to next vertex */
	jnz win_vertex_loop_ND_WB1	/*  yup, output next vertex */

.L__grDrawVertexList_3DNow_Window_win_vertex_end_ND_WB1:

	movd %mm1 , (fifo)	/*  flush "write buffer" */
	add $4 , fifo	/*  fifoPtr += sizeof(FxU32) */

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	mov $16 , %ebp	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoPtr(gc) , %eax	/*  old fifoPtr */

	mov %ebp , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */
	sub $15 , vertexCount	/*  remaining number of vertices to process */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	test vertexCount , vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L__grDrawVertexList_3DNow_Window_win_coords_loop_ND	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restire caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */

	db $0x08d , $0x064 , $0x024 , $0x000	/*  filler (not reachable) */

.L__grDrawVertexList_3DNow_Window_deref_mode:

	femms 	/*  we'll use MMX clear FPU/MMX state */

	prefetch (vertexPtr)	/*  pre-load first group of pointers */

.L__grDrawVertexList_3DNow_Window_win_coords_loop_D:

	sub $15 , vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L__grDrawVertexList_3DNow_Window_win_strip_begin_D	/*  yup, start writing strip data */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */
	nop 	/*  filler */

.L__grDrawVertexList_3DNow_Window_win_strip_begin_D:

/*      Setup packet header */

	mov vertexCount , %eax	/*  number of vertices in strip/fan */
	mov _type(%esp) , %edx	/*  setup mode */

	mov fifoPtr(gc) , fifo	/*  get fifoPtr */
	shl $22 , %edx	/*  <27:22> = setup mode (kSetupStrip or kSetupFan) */

	mov cullStripHdr(gc) , %ebp	/*  <2:0> = type */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	or %edx , %eax	/*  setup mode and vertex count */
	mov _pktype(%esp) , %edx	/*  <5:3> = command (SSTCP_PKT3_BDDBDD, SSTCP_PKT3_BDDDDD, or SSTCP_PKT3_DDDDDD) */

	or %ebp , %eax	/*  setup mode, vertex count, and type */
	mov $4 , %ebp	/*  test bit 2 */

	or %edx , %eax	/*  setup mode, vertex count, type, and command */
	lea tsuDataList(gc) , dlpStart	/*  pointer to start of offset list */

	test %ebp , fifo	/*  fifoPtr QWORD aligned ? */
	jz .L__grDrawVertexList_3DNow_Window_fifo_aligned_D	/*  yup */

	mov %eax , (fifo)	/*  PCI write packet type */
	add $4 , fifo	/*  fifo pointer now QWORD aligned */

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


.L__grDrawVertexList_3DNow_Window_win_vertex_loop_D_WB0:/*  nothing in "write buffer" */

	mov (vertexPtr) , %edx	/*  dereference pointer, edx points to vertex */
	add $4 , vertexPtr	/*  next pointer */

	lea tsuDataList(gc) , dlp	/*  get pointer to offset list dlp ++ */
	add $4 , dlp	/*  dlp ++ */

	movq X(%edx) , %mm1	/*  get vertex x,y */
	add $8 , fifo	/*  fifo += 2 */

	mov -4(dlp) , %eax	/*  get first offset from offset list */
	movq %mm1 , -8(fifo)	/*  PCI write x, y */

	cmp $0 , %eax	/*  if offset == 0, end of offset list */
	je .L__grDrawVertexList_3DNow_Window_win_datalist_end_D_WB0	/*  no more vertex data, nothing in "write buffer"  */

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L__grDrawVertexList_3DNow_Window_win_datalist_loop_D_WB0:/*  nothing in "write buffer" */

	movd (%edx,%eax) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
	jz .L__grDrawVertexList_3DNow_Window_win_datalist_end_D_WB1	/*  exit, write buffer contains one DWORD */

	add $8 , dlp	/*  dlp++ */
	movd (%edx,%eax) , %mm2	/*  get next parameter */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(fifo)	/*  PCI write current param | previous param */
	jnz win_datalist_loop_D_WB0	/*  nope, copy next parameter */

.L__grDrawVertexList_3DNow_Window_win_datalist_end_D_WB0:

	dec vertexCount	/*  another vertex done. Any left? */
	jnz win_vertex_loop_D_WB0	/*  yup, output next vertex */

.L__grDrawVertexList_3DNow_Window_win_vertex_end_D_WB0:

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	mov $16 , %ebp	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoPtr(gc) , %eax	/*  old fifoPtr */

	mov %ebp , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	add %eax , %ebp	/*  new number of bytes available in fifo */
	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */

	sub $15 , vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	test vertexCount , vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L__grDrawVertexList_3DNow_Window_win_coords_loop_D	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restire caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */
	mov %esp , %esp	/*  filler (unreachable) */

.L__grDrawVertexList_3DNow_Window_fifo_aligned_D:

	movd %eax , %mm1	/*  move header into "write buffer" */

.L__grDrawVertexList_3DNow_Window_win_vertex_loop_D_WB1:/*  one DWORD in "write buffer" */

	mov (vertexPtr) , %edx	/*  dereference pointer, edx points to vertex */
	add $4 , vertexPtr	/*  next pointer */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */
	lea tsuDataList(gc) , dlp	/*  get pointer to start of offset list */

	movd X(%edx) , %mm2	/*  0 | x of vertex */
	add $4 , dlp	/*  dlp++ */

	mov -4(dlp) , %eax	/*  first offset in offset list */
	punpckldq %mm2 , %mm1	/*  packet header | x of vertex */

	movq %mm1 , -8(fifo)	/*  PCI write packet header | x of vertex */
	movd Y(%edx) , %mm1	/*  0 | y of vertex */

	cmp $0 , %eax	/*  offset == 0 (list empty) ? */
	je .L__grDrawVertexList_3DNow_Window_win_datalist_end_D_WB1	/*  yup, no more vertex data, one DWORD in "write buffer" */

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L__grDrawVertexList_3DNow_Window_win_datalist_loop_D_WB1:/*  one DWORD in "write buffer" = MM1 */

	movd (%edx,%eax) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(fifo)	/*  PCI write current param | previous param */
	jz .L__grDrawVertexList_3DNow_Window_win_datalist_end_D_WB0	/*  yes, exit, "write buffer" empty */

	movd (%edx,%eax) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz win_datalist_loop_D_WB1	/*  nope, copy next parameter */

.L__grDrawVertexList_3DNow_Window_win_datalist_end_D_WB1:

	dec vertexCount	/*  another vertex done. Any left? */
	jnz win_vertex_loop_D_WB1	/*  yup, output next vertex */

.L__grDrawVertexList_3DNow_Window_win_vertex_end_D_WB1:

	movd %mm1 , (fifo)	/*  flush "write buffer" */
	add $4 , fifo	/*  fifoPtr++ */

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	mov $16 , %ebp	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoPtr(gc) , %eax	/*  old fifoPtr */

	mov %ebp , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	nop 	/*  filler */

	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */
	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */

	sub $15 , vertexCount	/*  remaining number of vertices to process */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */
	cmp $0 , vertexCount	/*  any vertices left to process ? */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	jg .L__grDrawVertexList_3DNow_Window_win_coords_loop_D	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

.L__grDrawVertexList_3DNow_Window_strip_done:
	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restire caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */

.L_END___grDrawVertexList_3DNow_Window:
.size __grDrawVertexList_3DNow_Window,.L_END___grDrawVertexList_3DNow_Window-__grDrawVertexList_3DNow_Window

.globl __grDrawVertexList_3DNow_Clip
.type __grDrawVertexList_3DNow_Clip,@function
__grDrawVertexList_3DNow_Clip:
/*  132  : { */

	push %edi	/*  save caller's register variable */
	mov (_GlideRoot+curGC) , gc	/*  get current graphics context */

	push %esi	/*  save caller's register variable */
	mov _count-8(%esp) , vertexCount	/*  number of vertices in strip/fan */

	push %ebx	/*  save caller's register variable */
	push %ebp	/*  save frame pointer */

	mov _pointers(%esp) , vertexPtr	/*  get current vertex pointer (deref mode) */
/*  get current vertex (non-deref mode) */
	test vertexCount , vertexCount	/*  number of vertices <= 0 ? */

	jle .L__grDrawVertexList_3DNow_Clip_strip_done	/*  yup, the strip/fan is done */

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
	mov vertexSize(gc) , %eax	/*  size of vertex data in bytes */

	test %edx , %edx	/*  mode 0 (array of vertices) ? */
	mov vertexStride(gc) , %edx	/*  get stride in DWORDs */

	movd (_GlideRoot+pool_f255) , %mm6	/*  GlideRoot.pool.f255      */
	mov $4 , (strideinbytes)	/*  array of pointers     */

	jnz clip_coords_begin	/*  nope, it's mode 1 */

.L__grDrawVertexList_3DNow_Clip_clip_coordinates_ND:

	shl $2 , %edx	/*  stride in bytes */
	mov %edx , (strideinbytes)	/*  save off stride (in bytes) */

.align 32
.L__grDrawVertexList_3DNow_Clip_clip_coords_begin:

#define dataElem %ebp	/*  number of vertex components processed     */

/*    { */
/*      float oow */
/*        while (count > 0) { */
/*        FxI32 k, vcount = count >= 15 ? 15 : count */

	sub $15 , vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */
	and %eax , vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */

	mov fifoRoom(gc) , %eax	/*  fifo space available */
	add $15 , vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul vertexCount , %ecx	/*  total amount of vertex data we'll send */

	add $4 , %ecx	/*  add header size ==> total packet size */
	nop 	/*  filler */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L__grDrawVertexList_3DNow_Clip_clip_strip_begin	/*  yup, start writing strip data */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */

.L__grDrawVertexList_3DNow_Clip_clip_strip_begin:

/*      TRI_STRIP_BEGIN(type, vcount, vSize, pktype) */

	mov _type(%esp) , %edx	/*  setup mode */
	mov vertexCount , %eax	/*  number of vertices in strip/fan */

	mov fifoPtr(gc) , fifo	/*  get fifoPtr */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	mov cullStripHdr(gc) , %ebp	/*  <2:0> = type */
	shl $22 , %edx	/*  <27:22> = setup mode (kSetupStrip or kSetupFan) */

	or %edx , %eax	/*  setup mode and vertex count */
	mov _pktype(%esp) , %edx	/*  <5:3> = command (SSTCP_PKT3_BDDBDD, SSTCP_PKT3_BDDDDD, or SSTCP_PKT3_DDDDDD) */

	or %ebp , %eax	/*  setup mode, vertex count, and type */
	add $4 , fifo	/*  fifoPtr += sizeof(FxU32) */

	or %edx , %eax	/*  setup mode, vertex count, type, and command */
	mov %eax , -4(fifo)	/*  PCI write header */

/*      for (k = 0 k < vcount k++) { */
/*        float *vPtr */
/*        vPtr = pointers */

.L__grDrawVertexList_3DNow_Clip_clip_for_begin:

/*        if (mode) */
/*          vPtr = *(float **)vPtr */

	mov vertexPtr , %edx	/*  vertex = vertexPtr (assume no-deref mode) */
	mov _mode(%esp) , %eax	/*  mode 0 = no deref, mode 1 = deref */

	mov vertexCount , (vertices)	/*  save numnber of vertices */
	test %eax , %eax	/*  deref mode ? */

	mov wInfo_offset(gc) , %eax	/*  get offset of W into vertex struct */
	jz .L__grDrawVertexList_3DNow_Clip_clip_noderef	/*  yup, no-deref mode */

	mov (vertexPtr) , %edx	/*  vertex = *vertexPtr */
	lea (%esp) , %esp	/*  filler */

.L__grDrawVertexList_3DNow_Clip_clip_noderef:

/*        oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset) */

	movd (%edx,%eax) , %mm0	/*  0 | W of current vertex */
	pfrcp %mm0 , %mm1	/*  0 | 1/W approx */

	mov (strideinbytes) , %ebp	/*  offset to next vertex/vertexPtr */
	movq (%edx) , %mm2	/*  y | x of current vertex */

	pfrcpit1 %mm1 , %mm0	/*  0 | 1/W refine */
	movq vp_hwidth(gc) , %mm3	/*  gc->state.Viewport.hheight | gc->state.Viewport.hwidth */

	movq vp_ox(gc) , %mm4	/*  gc->state.Viewport.oy | gc->state.Viewport.ox */
	add %ebp , vertexPtr	/*  point to next vertex/VertexPtr */

	pfrcpit2 %mm1 , %mm0	/*  oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset */
	mov paramIndex(gc) , %esi	/*  gc->state.paramIndex */

/*        /* x, y */ */
/*        TRI_SETF(FARRAY(vPtr, 0) */
/*          *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox) */
/*        TRI_SETF(FARRAY(vPtr, 4) */
/*          *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) */

	pfmul %mm3 , %mm2	/*  TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth */
	xor dataElem , dataElem	/*  dataElem = 0 */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */
	punpckldq %mm0 , %mm0	/*  oow | oow */

	pfmul %mm0 , %mm2	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth */
	pfadd %mm4 , %mm2	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) | */

	test $3 , %esi	/*  STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA ? */
	mov tsuDataList(gc) , %eax	/*  first entry from offset list */

/*        (float *)pointers += stride */
/*        TRI_VP_SETFS(vPtr, oow) */

	movq %mm2 , -8(fifo)	/*  PCI write transformed x, y */
	jz .L__grDrawVertexList_3DNow_Clip_clip_setup_ooz	/*  nope, no color at all needed */

	cmpl $0 , colorType(gc)	/*  gc->state.vData.colorType == GR_FLOAT ? */
	jne .L__grDrawVertexList_3DNow_Clip_clip_setup_pargb	/*  nope, packed ARGB format */

	test $1 , %esi	/*  STATE_REQUIRES_IT_DRGB ? */
	jz .L__grDrawVertexList_3DNow_Clip_clip_setup_a	/*  no, but definitely A */

	movd (%edx,%eax) , %mm2	/*  0 | r */
	mov tsuDataList+4(gc) , %eax	/*  offset of g part of vertex data */

	pfmul %mm6 , %mm2	/*  0 | r * 255.0f */
	movd (%edx,%eax) , %mm3	/*  0 | g */

	mov tsuDataList+8(gc) , %eax	/*  offset of b part of vertex data */
	movd %mm2 , (fifo)	/*  PCI write r*255 */

	pfmul %mm6 , %mm3	/*  0 | g * 255.0f */
	movd (%edx,%eax) , %mm2	/*  0 | b */

	movd %mm3 , 4(fifo)	/*  PCI write g*255 */
	mov $12 , dataElem	/*  dataElem = 3 */

	pfmul %mm6 , %mm2	/*  0 | b * 255.0f */
	mov tsuDataList+12(gc) , %eax	/*  offset of A part of vertex data */

	test $2 , %esi	/*  STATE_REQUIRES_IT_ALPHA ? */
	lea 12(fifo) , fifo	/*  fifoPtr += 3*sizeof(FxFloat) */

	movd %mm2 , -4(fifo)	/*  PCI write b*255 */
	jz .L__grDrawVertexList_3DNow_Clip_clip_setup_ooz	/*  nope, no alpha, proceeed with ooz */

.L__grDrawVertexList_3DNow_Clip_clip_setup_a:
	movd (%eax,%edx) , %mm2	/*  0 | a */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	mov %esp , %esp	/*  filler */
	add $4 , dataElem	/*  dataElem++  */

	pfmul %mm6 , %mm2	/*  0 | a * 255.0f */
	mov tsuDataList(gc,dataElem) , %eax	/*  offset of next part of vertex data */

	movd %mm2 , -4(fifo)	/*  PCI write a*255 */
	jmp .L__grDrawVertexList_3DNow_Clip_clip_setup_ooz	/*  check whether we need to push out z */

.align 32

.L__grDrawVertexList_3DNow_Clip_clip_setup_pargb:
	movd (%eax,%edx) , %mm2	/*  get packed ARGB data */
	add $4 , fifo	/*  fifoPtr += sizeof(FxU32) */

	mov $4 , dataElem	/*  dataElem = 1 (namely pargb) */
	mov tsuDataList+4(gc) , %eax	/*  offset of next part of vertex data */

	movd %mm2 , -4(fifo)	/*  PCI write packed ARGB */

.L__grDrawVertexList_3DNow_Clip_clip_setup_ooz:

	test $4 , %esi	/*  STATE_REQUIRES_OOZ ? */
	jz .L__grDrawVertexList_3DNow_Clip_clip_setup_qow	/*  nope */

	movd (%eax,%edx) , %mm2	/*  0 | z component of vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , dataElem	/*  dataElem += 1 */
	movd vp_hdepth(gc) , %mm3	/*  0 | gc->state.Viewport.hdepth */

	pfmul %mm0 , %mm2	/*  TRI_SETF(FARRAY(_s, i)*_oow */
	movd vp_oz(gc) , %mm4	/*  0 | gc->state.Viewport.oz */

	pfmul %mm3 , %mm2	/*  0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth */
	mov tsuDataList(gc,dataElem) , %eax	/*  offset of next vertex component */

	pfadd %mm4 , %mm2	/*  0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz */
	movd %mm2 , -4(fifo)	/*  PCI write transformed Z */

.L__grDrawVertexList_3DNow_Clip_clip_setup_qow:
	test $8 , %esi	/*  STATE_REQUIRES_OOW_FBI ? */
	jz .L__grDrawVertexList_3DNow_Clip_clip_setup_qow0	/*  nope */

	cmpl $0 , qInfo_mode(gc)	/*  does vertex have Q component ? */
	je .L__grDrawVertexList_3DNow_Clip_clip_setup_oow	/*  nope, not Q but W */

	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */
	mov qInfo_offset(gc) , %eax	/*  offset of Q component of vertex */

	add $4 , dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q of vertex */

	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q*oow */

	movd %mm2 , -4(fifo)	/*  PCI write transformed Q */
	jmp .L__grDrawVertexList_3DNow_Clip_clip_setup_qow0	/*  continue with q0 */

.align 32

.L__grDrawVertexList_3DNow_Clip_clip_setup_oow:
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , dataElem	/*  dataElem++ */

	movd %mm0 , -4(fifo)	/*  PCI write oow */
	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */

.L__grDrawVertexList_3DNow_Clip_clip_setup_qow0:
	test $16 , %esi	/*  STATE_REQUIRES_W_TMU0 ? */
	jz .L__grDrawVertexList_3DNow_Clip_clip_setup_stow0	/*  nope  */

	cmpl $0 , q0Info_mode(gc)	/*  does vertex have Q component ? */
	je .L__grDrawVertexList_3DNow_Clip_clip_setup_oow0	/*  nope, not Q but W */

	mov q0Info_offset(gc) , %eax	/*  offset of Q component of vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q0 of vertex */

	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q0*oow */

	movd %mm2 , -4(fifo)	/*  PCI write transformed q0 */
	jmp .L__grDrawVertexList_3DNow_Clip_clip_setup_stow0	/*  continue with stow0 */

.align 32

.L__grDrawVertexList_3DNow_Clip_clip_setup_oow0:
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , dataElem	/*  dataElem++ */

	movd %mm0 , -4(fifo)	/*  PCI write oow */
	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */

.L__grDrawVertexList_3DNow_Clip_clip_setup_stow0:

	test $32 , %esi	/*  STATE_REQUIRES_ST_TMU0 ? */
	jz .L__grDrawVertexList_3DNow_Clip_clip_setup_qow1	/*  nope */

	movq tmu0_s_scale(gc) , %mm7	/*  state.tmu_config[0].t_scale | state.tmu_config[0].s_scale */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movd (%edx,%eax) , %mm2	/*  param1 */
	mov tsuDataList+4(gc,dataElem) , %eax	/* pointer to next vertex component */

	pfmul %mm0 , %mm7	/*  oow*tmu0_t_scale | oow*tmu0_s_scale */
	add $8 , dataElem	/*  dataElem += 2 */

	movd (%edx,%eax) , %mm3	/*  param2 */
	punpckldq %mm3 , %mm2	/*  param2 | param1 */

	pfmul %mm7 , %mm2	/*  param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale */

	movq %mm2 , -8(fifo)	/*  PCI write param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale  */
	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */

.L__grDrawVertexList_3DNow_Clip_clip_setup_qow1:
	test $64 , %esi	/*  STATE_REQUIRES_W_TMU1 ? */
	jz .L__grDrawVertexList_3DNow_Clip_clip_setup_stow1	/*  nope */

	cmpl $0 , q1Info_mode(gc)	/*  does vertex have Q component ? */
	je .L__grDrawVertexList_3DNow_Clip_clip_setup_oow1	/*  nope, not Q but W */

	mov q1Info_offset(gc) , %eax	/*  offset of Q component of vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q1 of vertex */

	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q1*oow */

	movd %mm2 , -4(fifo)	/*  PCI write transformed q1 */
	jmp .L__grDrawVertexList_3DNow_Clip_clip_setup_stow1	/*  continue with stow1 */

.align 32

.L__grDrawVertexList_3DNow_Clip_clip_setup_oow1:
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , dataElem	/*  dataElem++ */

	movd %mm0 , -4(fifo)	/*  PCI write oow */
	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */

.L__grDrawVertexList_3DNow_Clip_clip_setup_stow1:

	test $128 , %esi	/*  STATE_REQUIRES_ST_TMU1 ? */
	mov (vertices) , vertexCount	/*  get number of vertices */

	movq tmu1_s_scale(gc) , %mm7	/*  state.tmu_config[1].t_scale | state.tmu_config[1].s_scale */
	jz .L__grDrawVertexList_3DNow_Clip_clip_setup_end	/*  nope */

	movd (%edx,%eax) , %mm2	/*  param1 */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov tsuDataList+4(gc,dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm7	/*  oow*state.tmu_config[1].t_scale | oow*state.tmu_config[1].s_scale */

	movd (%edx,%eax) , %mm3	/*  param2 */
	punpckldq %mm3 , %mm2	/*  param2 | param1 */

	pfmul %mm7 , %mm2	/*  param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale */
	movq %mm2 , -8(fifo)	/*  PCI write param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale */

.L__grDrawVertexList_3DNow_Clip_clip_setup_end:

/*  206  :       for (k = 0 k < vcount k++) { */

	dec vertexCount	/*  vcount-- */
	jnz clip_for_begin	/*  until  */
.L__grDrawVertexList_3DNow_Clip_clip_for_end:

/*  221  :       } */
/*  222  :       TRI_END */

	mov fifoPtr(gc) , %eax	/*  old fifoPtr */
	mov %esp , %esp	/*  filler */

	nop 	/*  filler */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	sub $15 , vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	cmp $0 , vertexCount	/*  any vertices left to process ? */

	movl $16 , _pktype(%esp)	/*  pktype = SSTCP_PKT3_DDDDDD (strip continuation) */
	jg .L__grDrawVertexList_3DNow_Clip_clip_coords_begin	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state     */

.L__grDrawVertexList_3DNow_Clip_strip_done:
/*     } */
/*   #undef FN_NAME */
/*   } /* _grDrawVertexList */ */

	pop %ebp	/*  restore frame pointer  */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */

.L_END___grDrawVertexList_3DNow_Clip:
.size __grDrawVertexList_3DNow_Clip,.L_END___grDrawVertexList_3DNow_Clip-__grDrawVertexList_3DNow_Clip

.align 32

#define _a$ 20
#define _b$ 24

#define gc %esi
#define vb %edi
#define va %ebx
#define i %ebp
#define j %edx
#define dlp %edx
#define fifo %ecx
#define dlpStart %ebp
#define ADX _GlideRoot+pool_fTemp1
#define ADY _GlideRoot+pool_fTemp2

.globl __grDrawTextureLine_3DNow
.type __grDrawTextureLine_3DNow,@function
__grDrawTextureLine_3DNow:

/*  227  : { */
/*  228  : #define FN_NAME "grDrawTextureLine" */
/*  229  :  */
/*  230  :   GR_BEGIN_NOFIFOCHECK(FN_NAME, 91) */
/*  231  :  */
/*  232  : #define  DX _GlideRoot.pool.ftemp1 */
/*  233  : #define ADY _GlideRoot.pool.ftemp2 */
/*  234  :  */
/*  235  :   GR_FLUSH_STATE() */

	push %esi	/*  save caller's register variable */
	mov (_GlideRoot+curGC) , gc	/*  get current graphics context */

	push %edi	/*  save caller's register variable */
	mov -8(%esp) , vb	/*  b */

	push %ebx	/*  save caller's register variable */
	push %ebp	/*  save caller's frame pointer */

	femms 	/*  we'll use MMX empty FPU/MMX state     */
	mov (%esp) , va	/*  a */

/*  236  :  */
/*  237  :   { */
/*  238  :     FxI32 i, j */
/*  239  :     FxU32 dataElem */
/*  240  :     FxU32 vSize */
/*  241  :      */
/*  242  :     /* */
/*  243  :     ** compute absolute deltas and draw from low Y to high Y */
/*  244  :     */ */
/*  245  :     ADY = FARRAY(b, 4) - FARRAY(a, 4) */
/*  246  :     i = *(long *)&ADY */
/*  247  :     if (i < 0) { */
/*  248  :       const void *tv */
/*  249  :       tv = a a = b b = tv */
/*  250  :       i ^= 0x80000000            /* ady = -ady */ */
/*  251  :       /* (*(long *)&ADY) = i */ */
/*  252  :     } */

	movq (vb) , %mm1	/*  FARRAY(b,4) | FARRAY(b,0) */
	nop 	/*  filler */

	movq (va) , %mm0	/*  FARRAY(a,4) | FARRAY(a,0) */
	movq %mm1 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0) */

	pfsub %mm0 , %mm6	/*  FARRAY(b,4)-FARRAY(a,4) | FARRAY(b,0)-FARRAY(a,0) */
	mov vertexSize(gc) , %eax	/*  size of vertex data (in bytes) */

	movq %mm6 , %mm5	/*  FARRAY(b,4)-FARRAY(a,4) | FARRAY(b,0)-FARRAY(a,0) */
	mov fifoRoom(gc) , %ecx	/*  fifo room available (in bytes) */

	punpckhdq %mm6 , %mm6	/*  FARRAY(b,4)-FARRAY(a,4) | FARRAY(b,4)-FARRAY(a,4) */
	movd %mm6 , (ADY)	/*  ADY = FARRAY(b, 4) - FARRAY(a, 4) */

	movd %mm5 , (ADX)	/*  DX = FARRAY(b, 0) - FARRAY(a, 0) */
	mov (ADY) , i	/*  i = *(long *)&ADY */

	test i , i	/*  i < 0 ? */
	jge .L__grDrawTextureLine_3DNow_$dont_swap_ij	/*  nope, no need to swap i and j  */

	xor vb , va	/*  va ^ vb */
	xor va , vb	/*  vb ^ (va ^ vb) = va */

	xor vb , va	/*  (va ^ vb) ^ va = vb */
	xor $0x80000000 , i	/*  i ^= 0x80000000 */

.L__grDrawTextureLine_3DNow_$dont_swap_ij:

/*  253  :      */
/*  254  :     DX = FARRAY(b, 0) - FARRAY(a, 0) */
/*  255  :     j = *(long *)&DX */
/*  256  :     if (j < 0) { */
/*  257  :       j ^= 0x80000000            /* adx = -adx */ */
/*  258  :     } */
/*  259  :      */
/*  260  :     /* check for zero-length lines */ */
/*  261  :     if ((j >= i) && (j == 0)) */
/*  262  :       return */
/*  263  :      */

	mov (ADX) , j	/*  j = *(long *)&DX */
	and $0x7fffffff , j	/*  j = abs(j) */

	cmp i , j	/*  j < i ? */
	jl .L__grDrawTextureLine_3DNow_$j_lt_i	/*  yup */

	test j , j	/*  j == 0 ? */
	jz .L__grDrawTextureLine_3DNow_$line_all_done	/*  yup, nothing to draw */

.L__grDrawTextureLine_3DNow_$j_lt_i:

/*  264  :     vSize = gc->state.vData.vSize + 8 */
/*  265  :     GR_SET_EXPECTED_SIZE((vSize<< 2), 1) */

	leal 4+36(%eax) , %eax	/*  we have vertices + 4 bytes for header */
	cmp %eax , %ecx	/*  fifo room avail >= fifo room required ? */

	jge .L__grDrawTextureLine_3DNow_$enough_fifo_room	/*  yup, sufficient fifo room */
	push j	/*  preserve j */

	push $__LINE__	/*  line number in source file */
	push $0	/*  pointer to filename = NULL */

	push %eax	/*  fifo space required */
	call _FifoMakeRoom	/*  allocate new fifo space (modified fifoPtr) */

	add $12 , %esp	/*  pop 3 DWORD parameters off stack */
	pop j	/*  restore j */

.L__grDrawTextureLine_3DNow_$enough_fifo_room:

/*  266  :     TRI_STRIP_BEGIN(kSetupCullDisable | kSetupStrip, 4, vSize,  */
/*  267  :                     SSTCP_PKT3_BDDDDD | (1<<15)) */
/*  268  :     /* x major */ */
/*  269  :     if (j >= i) { */

	mov fifoPtr(gc) , fifo	/*  gc->cmdTransportInfo.fifoPtr */
	pxor %mm3 , %mm3	/*  0 | 0 */

	movd (_GlideRoot+pool_fHalf) , %mm2	/*  0 | _GlideRoot.pool.fHalf */
	movq (_F256_F256) , %mm4	/*  256.0f | 256.0f */

	test $4 , fifo	/*  fifo QWORD aligned ? */
	jz .L__grDrawTextureLine_3DNow_$drawline_fifo_aligned	/*  yup */

	add $4 , fifo	/*  fifoPtr += sizeof(FxU32) */
	mov cullStripHdr(gc) , %eax	/*  gc->cmdTransportInfo.cullStripHdr */

	psllq $32 , %mm2	/*  _GlideRoot.pool.fHalf | 0 */
	punpckldq %mm3 , %mm4	/*  0 | 256.0f  */

	or $0x8108 , %eax	/*  kSetupCullDisable|kSetupStrip|4<<6|SSTCP_PKT3_BDDDDD<<3|1<<15  */
	cmp i , j	/*  j < i ? */

	mov %eax , -4(fifo)	/*  PCI write header (fifo now aligned) */
	jl .L__grDrawTextureLine_3DNow_$j_lt_i2_WB0	/*  yup, j < i */

/*  270  :       TRI_SETF(FARRAY(b, 0)) */
/*  271  :       dataElem = 0 */
/*  272  :       TRI_SETF(FARRAY(b, 4) - _GlideRoot.pool.fHalf) */
/*  273  :       i = gc->tsuDataList[dataElem] */
/*  274  :       while (i != GR_DLIST_END) { */
/*  275  :         TRI_SETF(FARRAY(b, i)) */
/*  276  :         dataElem++ */
/*  277  :         i = gc->tsuDataList[dataElem] */
/*  278  :       } */

	add $8 , fifo	/*  fifoPtr += sizeof(FxFloat) */
	movq %mm1 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0) */

	lea tsuDataList(gc) , dlpStart	/*  gc->tsuDataList */
	pfsub %mm2 , %mm6	/*  FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0) */

	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */
	mov dlpStart , dlp	/*  dataElem = 0 */

	add $4 , dlp	/*  point to next entry in offset list */
	movq %mm6 , -8(fifo)	/*  PCI write FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0) */

	test %eax , %eax	/*  i != GR_DLIST_END ? */
	je .L__grDrawTextureLine_3DNow_$vertex_loop1_done_WB0	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop1_WB0:
	movd (vb,%eax) , %mm7	/*  FARRAY(b,i) */
	mov (dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  offset == 0 (end of offset list) ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop1_done_WB1	/*  nope, output next vertex component */

	movd (vb,%eax) , %mm6	/*  FARRAY(b,i) */
	add $8 , dlp	/*  point to next entry in offset list */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  offset == 0 (end of offset list) ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jnz $vertex_loop1_WB0	/*  offset != 0, process next component */

.L__grDrawTextureLine_3DNow_$vertex_loop1_done_WB0:

/*  279  :       TRI_SETF(0.f) */
/*  280  :       TRI_SETF(0.f) */
/*  281  :        */
/*  282  :       TRI_SETF(FARRAY(a, 0)) */
/*  283  :       dataElem = 0 */
/*  284  :       TRI_SETF(FARRAY(a, 4) - _GlideRoot.pool.fHalf) */
/*  285  :       i = gc->tsuDataList[dataElem] */
/*  286  :       while (i != GR_DLIST_END) { */
/*  287  :         TRI_SETF(FARRAY(a, i)) */
/*  288  :         dataElem++ */
/*  289  :         i = gc->tsuDataList[dataElem] */
/*  290  :       } */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat)  */
	movq %mm0 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0) */

	movq %mm3 , -16(fifo)	/*  PCI write TRI_SETF(0.f) | TRI_SETF(0.f) */
	pfsub %mm2 , %mm6	/*  FARRAY(a,4)-_GlideRoot.pool.fHalf | FARRAY(a,0)  */

	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */
	mov dlpStart , dlp	/*  dataElem = 0 */

	add $4 , dlp	/*  point to next entry in offset list */
	movq %mm6 , -8(fifo)	/*  PCI write FARRAY(a,4)-_GlideRoot.pool.fHalf | FARRAY(a,0) */

	cmp $0 , %eax	/*  i == GR_DLIST_END ? */
	je .L__grDrawTextureLine_3DNow_$vertex_loop2_done_WB0	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop2_WB0:
	movd (va,%eax) , %mm7	/*  FARRAY(a,i) */
	mov (dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  offset == 0 (end of offset list) ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop2_done_WB1	/*  nope, output next vertex component */

	movd (va,%eax) , %mm6	/*  FARRAY(a,i) */
	add $8 , dlp	/*  point to next entry in offset list */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  offset == 0 (end of offset list) ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jnz $vertex_loop2_WB0	/*  offset != 0, process next component */

.L__grDrawTextureLine_3DNow_$vertex_loop2_done_WB0:

/*  291  :       TRI_SETF(0.f) */
/*  292  :       TRI_SETF(0.f) */
/*  293  :        */
/*  294  :       TRI_SETF(FARRAY(b, 0)) */
/*  295  :       dataElem = 0 */
/*  296  :       TRI_SETF(FARRAY(b, 4) + _GlideRoot.pool.fHalf) */
/*  297  :       i = gc->tsuDataList[dataElem] */
/*  298  :       while (i != GR_DLIST_END) { */
/*  299  :         TRI_SETF(FARRAY(b, i)) */
/*  300  :         dataElem++ */
/*  301  :         i = gc->tsuDataList[dataElem] */
/*  302  :       } */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat) */
	movq %mm1 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0) */

	movq %mm3 , -16(fifo)	/*  PCI write TRI_SETF(0.f) | TRI_SETF(0.f) */
	pfadd %mm2 , %mm6	/*  FARRAY(b,4)+_GlideRoot.pool.fHalf | FARRAY(b,0) */

	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */
	mov dlpStart , dlp	/*  dataElem = 0 */

	add $4 , dlp	/*  point to next entry in offset list */
	test %eax , %eax	/*  i != GR_DLIST_END ? */

	movq %mm6 , -8(fifo)	/*  PCI write FARRAY(b,4)+_GlideRoot.pool.fHalf | FARRAY(b,0) */
	je .L__grDrawTextureLine_3DNow_$vertex_loop3_done_WB0	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop3_WB0:
	movd (vb,%eax) , %mm7	/*  FARRAY(b, i) */
	mov (dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  offset == 0 (end of offset list) ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop3_done_WB1	/*  nope, output next vertex component */

	movd (vb,%eax) , %mm6	/*  FARRAY(b, i) */
	add $8 , dlp	/*  point to next entry in offset list */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	cmp $0 , %eax	/*  offset == 0 (end of offset list) ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jne .L__grDrawTextureLine_3DNow_$vertex_loop3_WB0	/*  offset != 0, process next component */

.L__grDrawTextureLine_3DNow_$vertex_loop3_done_WB0:

/*  303  :       TRI_SETF(256.f) */
/*  304  :       TRI_SETF(0.f) */
/*  305  :        */
/*  306  :       TRI_SETF(FARRAY(a, 0)) */
/*  307  :       dataElem = 0 */
/*  308  :       TRI_SETF(FARRAY(a, 4) + _GlideRoot.pool.fHalf) */
/*  309  :       i = gc->tsuDataList[dataElem] */
/*  310  :       while (i != GR_DLIST_END) { */
/*  311  :         TRI_SETF(FARRAY(a, i)) */
/*  312  :         dataElem++ */
/*  313  :         i = gc->tsuDataList[dataElem] */
/*  314  :       } */
/*  315  :       TRI_SETF(256.f) */
/*  316  :       TRI_SETF(0.f) */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat) */
	movq %mm0 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0) */

	movq %mm4 , -16(fifo)	/*  PCI write TRI_SETF(0.f) | TRI_SETF(256.f) */
	pfadd %mm2 , %mm6	/*  FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0)  */

	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */
	mov dlpStart , dlp	/*  dataElem = 0 */

	add $4 , dlp	/*  point to next entry in offset list */
	movq %mm6 , -8(fifo)	/*  PCI write FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0) */

	test %eax , %eax	/*  i != GR_DLIST_END ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop4_done_WB0	/*  i == GR_DLIST_END, done with all four vertices */

.L__grDrawTextureLine_3DNow_$vertex_loop4_WB0:
	movd (va,%eax) , %mm7	/*  FARRAY(a, i) */
	mov (dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  offset == 0 (end of offset list) ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop4_done_WB1	/*  nope, output next vertex component */

	movd (va,%eax) , %mm6	/*  FARRAY(a, i) */
	add $8 , dlp	/*  point to next entry in offset list */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  offset == 0 (end of offset list) ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jnz $vertex_loop4_WB0	/*  offset != 0, process next component */

.L__grDrawTextureLine_3DNow_$vertex_loop4_done_WB0:
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */
	lea (%esp) , %esp	/*  filler */

	movq %mm4 , -8(fifo)	/*  PCI write TRI_SETF(0.f) | TRI_SETF(256.f)  */
	jmp .L__grDrawTextureLine_3DNow_$line_done_WB0	/*  last vertex done */

.L__grDrawTextureLine_3DNow_$j_lt_i2_WB0:

/*  317  :     } else { /* y major */ */
/*  318  :       TRI_SETF(FARRAY(b, 0) - _GlideRoot.pool.fHalf) */
/*  319  :       dataElem = 0 */
/*  320  :       TRI_SETF(FARRAY(b, 4)) */
/*  321  :       i = gc->tsuDataList[dataElem] */
/*  322  :       while (i != GR_DLIST_END) { */
/*  323  :         TRI_SETF(FARRAY(b, i)) */
/*  324  :         dataElem++ */
/*  325  :         i = gc->tsuDataList[dataElem] */
/*  326  :       } */

	psrlq $32 , %mm2	/*  0 | _GlideRoot.pool.fHalf */
	movq %mm1 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0) */

	pfsub %mm2 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0)-_GlideRoot.pool.fHalf  */
	lea tsuDataList(gc) , dlpStart	/*  gc->tsuDataList */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */
	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */

	lea 4(dlpStart) , dlp	/*  next dataElem = 1 */
	movq %mm6 , -8(fifo)	/*  PCI write FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0) */

	cmp $0 , %eax	/*  i != GR_DLIST_END ? */
	je .L__grDrawTextureLine_3DNow_$vertex_loop5_done_WB0	/*  i == GR_DLIST_END, no more components in vertex */

.L__grDrawTextureLine_3DNow_$vertex_loop5_WB0:
	movd (vb,%eax) , %mm7	/*  FARRAY(b,i) */
	mov (dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  offset == 0 (end of offset list) ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop5_done_WB1	/*  nope, output next vertex component */

	movd (vb,%eax) , %mm6	/*  FARRAY(b,i) */
	add $8 , dlp	/*  point to next entry in offset list */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  offset == 0 (end of offset list) ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jnz $vertex_loop5_WB0	/*  offset != 0, process next component */

.L__grDrawTextureLine_3DNow_$vertex_loop5_done_WB0:

/*  327  :       TRI_SETF(0.f) */
/*  328  :       TRI_SETF(0.f) */
/*  329  :            */
/*  330  :       TRI_SETF(FARRAY(a, 0) - _GlideRoot.pool.fHalf) */
/*  331  :       dataElem = 0 */
/*  332  :       TRI_SETF(FARRAY(a, 4)) */
/*  333  :       i = gc->tsuDataList[dataElem] */
/*  334  :       while (i != GR_DLIST_END) { */
/*  335  :         TRI_SETF(FARRAY(a, i)) */
/*  336  :         dataElem++ */
/*  337  :         i = gc->tsuDataList[dataElem] */
/*  338  :       } */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat) */
	movq %mm0 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0) */

	movq %mm3 , -16(fifo)	/*  PCI write TRI_SETF(0.f) | TRI_SETF(0.f) */
	pfsub %mm2 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0)-_GlideRoot.pool.fHalf */

	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */
	mov dlpStart , dlp	/*  dataElem = 0 */

	add $4 , dlp	/*  point to next entry in offset list */
	movq %mm6 , -8(fifo)	/*  PCI write FARRAY(a,4) | FARRAY(a,0)-_GlideRoot.pool.fHalf */

	test %eax , %eax	/*  i != GR_DLIST_END ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop6_done_WB0	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop6_WB0:
	movd (va,%eax) , %mm7	/*  get next vertex component */
	mov (dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  offset == 0 (end of offset list) ? */
	je .L__grDrawTextureLine_3DNow_$vertex_loop6_done_WB1	/*  nope, output next vertex component */

	movd (va,%eax) , %mm6	/*  get next vertex component */
	add $8 , dlp	/*  point to next entry in offset list */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  offset == 0 (end of offset list) ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jnz $vertex_loop6_WB0	/*  offset != 0, process next component */

.L__grDrawTextureLine_3DNow_$vertex_loop6_done_WB0:

/*  339  :       TRI_SETF(0.f) */
/*  340  :       TRI_SETF(0.f) */
/*  341  :        */
/*  342  :       TRI_SETF(FARRAY(b, 0) + _GlideRoot.pool.fHalf) */
/*  343  :       dataElem = 0 */
/*  344  :       TRI_SETF(FARRAY(b, 4)) */
/*  345  :       i = gc->tsuDataList[dataElem] */
/*  346  :       while (i != GR_DLIST_END) { */
/*  347  :         TRI_SETF(FARRAY(b, i)) */
/*  348  :         dataElem++ */
/*  349  :         i = gc->tsuDataList[dataElem] */
/*  350  :       } */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat) */
	movq %mm1 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0) */

	movq %mm3 , -16(fifo)	/*  PCI write TRI_SETF(0.f) | TRI_SETF(0.f) */
	pfadd %mm2 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0)+_GlideRoot.pool.fHalf */

	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */
	mov dlpStart , dlp	/*  dataElem = 0 */

	add $4 , dlp	/*  point to next entry in offset list */
	movq %mm6 , -8(fifo)	/*  PCI write FARRAY(b,4) | FARRAY(b,0)+_GlideRoot.pool.fHalf */

	test %eax , %eax	/*  i != GR_DLIST_END ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop7_done_WB0	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop7_WB0:

	movd (vb,%eax) , %mm7	/*  FARRAY(b, i) */
	mov (dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  offset == 0 (end of offset list) ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop7_done_WB1	/*  nope, output next vertex component */

	movd (vb,%eax) , %mm6	/*  FARRAY(b, i) */
	add $8 , dlp	/*  point to next entry in offset list */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  offset == 0 (end of offset list) ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jnz $vertex_loop7_WB0	/*  offset != 0, process next component */

.L__grDrawTextureLine_3DNow_$vertex_loop7_done_WB0:

/*  351  :       TRI_SETF(256.f) */
/*  352  :       TRI_SETF(0.f) */
/*  353  :        */
/*  354  :       TRI_SETF(FARRAY(a, 0) + _GlideRoot.pool.fHalf) */
/*  355  :       dataElem = 0 */
/*  356  :       TRI_SETF(FARRAY(a, 4)) */
/*  357  :       i = gc->tsuDataList[dataElem] */
/*  358  :       while (i != GR_DLIST_END) { */
/*  359  :         TRI_SETF(FARRAY(a, i)) */
/*  360  :         dataElem++ */
/*  361  :         i = gc->tsuDataList[dataElem] */
/*  362  :       } */
/*  363  :       TRI_SETF(256.f) */
/*  364  :       TRI_SETF(0.f) */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat) */
	movq %mm0 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0) */

	movq %mm4 , -16(fifo)	/*  PCI write TRI_SETF(0.f) | TRI_SETF(256.f) */
	pfadd %mm2 , %mm6	/*  FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0)  */

	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */
	mov dlpStart , dlp	/*  dataElem = 0 */

	add $4 , dlp	/*  point to next entry in offset list */
	movq %mm6 , -8(fifo)	/*  PCI write FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0) */

	test %eax , %eax	/*  i != GR_DLIST_END ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop8_done_WB0	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop8_WB0:
	movd (va,%eax) , %mm7	/*  FARRAY(a, i) */
	mov (dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  offset == 0 (end of offset list) ? */
	je .L__grDrawTextureLine_3DNow_$vertex_loop8_done_WB1	/*  nope, output next vertex component */

	movd (va,%eax) , %mm6	/*  FARRAY(a, i) */
	add $8 , dlp	/*  point to next entry in offset list */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */

	test %eax , %eax	/*  offset == 0 (end of offset list) ? */
	jnz $vertex_loop8_WB0	/*  offset != 0, process next component */

.L__grDrawTextureLine_3DNow_$vertex_loop8_done_WB0:

	movq %mm4 , (fifo)	/*  PCI write TRI_SETF(0.f) | TRI_SETF(256.f)  */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	nop 	/*  filler */
	jmp .L__grDrawTextureLine_3DNow_$line_done_WB0	/*  done with line */

.L__grDrawTextureLine_3DNow_$drawline_fifo_aligned:

	mov cullStripHdr(gc) , %eax	/*  gc->cmdTransportInfo.cullStripHdr */
	psllq $32 , %mm2	/*  _GlideRoot.pool.fHalf | 0 */

	punpckldq %mm3 , %mm4	/*  0 | 256.0f */
	or $0x8108 , %eax	/*  kSetupCullDisable|kSetupStrip|4<<6|SSTCP_PKT3_BDDDDD<<3|1<<15  */

	movd %eax , %mm7	/*  move header to "write buffer" = MM7 */

	cmp i , j	/*  j < i ? */
	jl .L__grDrawTextureLine_3DNow_$j_lt_i2_WB1	/*  yup, j < i */

/*  270  :       TRI_SETF(FARRAY(b, 0)) */
/*  271  :       dataElem = 0 */
/*  272  :       TRI_SETF(FARRAY(b, 4) - _GlideRoot.pool.fHalf) */
/*  273  :       i = gc->tsuDataList[dataElem] */
/*  274  :       while (i != GR_DLIST_END) { */
/*  275  :         TRI_SETF(FARRAY(b, i)) */
/*  276  :         dataElem++ */
/*  277  :         i = gc->tsuDataList[dataElem] */
/*  278  :       } */

	add $8 , fifo	/*  fifoPtr += sizeof(FxFloat) */
	movq %mm1 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0) */

	lea tsuDataList(gc) , dlpStart	/*  gc->tsuDataList */
	pfsub %mm2 , %mm6	/*  FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0) */

	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */
	lea 4(dlpStart) , dlp	/*  next dataElem = 1 */

	punpckldq %mm1 , %mm7	/*  FARRAY(b,0) | header */
	cmp $0 , %eax	/*  i == GR_DLIST_END ? */

	movq %mm7 , -8(fifo)	/*  PCI write FARRAY(b,0) | header */
	movq %mm6 , %mm7	/*  FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0) */

	punpckhdq %mm7 , %mm7	/*  write buffer = FARRAY(b,4)-_GlideRoot.pool.fHalf  */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop1_done_WB1	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop1_WB1:
	movd (vb,%eax) , %mm6	/*  FARRAY(b,i) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  point to next offset */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  i == GR_DLIST_END ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop1_done_WB0	/*  yup, i == GR_DLIST_END */

	movd (vb,%eax) , %mm7	/*  FARRAY(b,i) */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  i == GR_DLIST_END ? */
	jne .L__grDrawTextureLine_3DNow_$vertex_loop1_WB1	/*  nope, more components to handle */

.L__grDrawTextureLine_3DNow_$vertex_loop1_done_WB1:

/*  279  :       TRI_SETF(0.f) */
/*  280  :       TRI_SETF(0.f) */
/*  281  :        */
/*  282  :       TRI_SETF(FARRAY(a, 0)) */
/*  283  :       dataElem = 0 */
/*  284  :       TRI_SETF(FARRAY(a, 4) - _GlideRoot.pool.fHalf) */
/*  285  :       i = gc->tsuDataList[dataElem] */
/*  286  :       while (i != GR_DLIST_END) { */
/*  287  :         TRI_SETF(FARRAY(a, i)) */
/*  288  :         dataElem++ */
/*  289  :         i = gc->tsuDataList[dataElem] */
/*  290  :       } */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat)  */
	movq %mm0 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0) */

	punpckldq %mm3 , %mm7	/*  0 | previous component */
	pfsub %mm2 , %mm6	/*  FARRAY(a,4)-_GlideRoot.pool.fHalf | FARRAY(a,0)  */

	movq %mm7 , -16(fifo)	/*  PCI write 0 | previous component */
	movq %mm3 , %mm7	/*  0 | 0 */

	punpckldq %mm0 , %mm7	/*  FARRAY(a,0) | 0 */
	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */

	movq %mm7 , -8(fifo)	/*  PCI write FARRAY(a,0) | 0 */
	movq %mm6 , %mm7	/*  FARRAY(a,4)-_GlideRoot.pool.fHalf | FARRAY(a,0) */

	punpckhdq %mm7 , %mm7	/*  write buffer = FARRAY(a,4)-_GlideRoot.pool.fHalf */
	lea 4(dlpStart) , dlp	/*  point to next entry in offset list */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	je .L__grDrawTextureLine_3DNow_$vertex_loop2_done_WB1	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop2_WB1:
	movd (va,%eax) , %mm6	/*  FARRAY(a,i) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  point to next offset */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  i == GR_DLIST_END ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop2_done_WB0	/*  yup, i == GR_DLIST_END */

	movd (va,%eax) , %mm7	/*  FARRAY(a,i) */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	jnz $vertex_loop2_WB1	/*  nope, more components to handle */

.L__grDrawTextureLine_3DNow_$vertex_loop2_done_WB1:

/*  291  :       TRI_SETF(0.f) */
/*  292  :       TRI_SETF(0.f) */
/*  293  :        */
/*  294  :       TRI_SETF(FARRAY(b, 0)) */
/*  295  :       dataElem = 0 */
/*  296  :       TRI_SETF(FARRAY(b, 4) + _GlideRoot.pool.fHalf) */
/*  297  :       i = gc->tsuDataList[dataElem] */
/*  298  :       while (i != GR_DLIST_END) { */
/*  299  :         TRI_SETF(FARRAY(b, i)) */
/*  300  :         dataElem++ */
/*  301  :         i = gc->tsuDataList[dataElem] */
/*  302  :       } */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat) */
	movq %mm1 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0) */

	punpckldq %mm3 , %mm7	/*  0 | previous component */
	pfadd %mm2 , %mm6	/*  FARRAY(b,4)+_GlideRoot.pool.fHalf | FARRAY(b,0) */

	movq %mm7 , -16(fifo)	/*  PCI write 0 | previous component */
	movq %mm3 , %mm7	/*  0 | 0 */

	punpckldq %mm1 , %mm7	/*  FARRAY(b,0) | 0 */
	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */

	movq %mm7 , -8(fifo)	/*  PCI write FARRAY(b,0) | 0 */
	movq %mm6 , %mm7	/*  FARRAY(b,4)+_GlideRoot.pool.fHalf | FARRAY(b,0) */

	punpckhdq %mm7 , %mm7	/*  write buffer = FARRAY(a,4)-_GlideRoot.pool.fHalf */
	lea 4(dlpStart) , dlp	/*  point to next entry in offset list */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop3_done_WB1	/*  yup, i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop3_WB1:
	movd (vb,%eax) , %mm6	/*  FARRAY(b,i) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  point to next offset */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  i == GR_DLIST_END ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop3_done_WB0	/*  yup, i == GR_DLIST_END */

	movd (vb,%eax) , %mm7	/*  FARRAY(b,i) */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  i == GR_DLIST_END ? */
	jnz $vertex_loop3_WB1	/*  nope, more components to handle */

.L__grDrawTextureLine_3DNow_$vertex_loop3_done_WB1:

/*  303  :       TRI_SETF(256.f) */
/*  304  :       TRI_SETF(0.f) */
/*  305  :        */
/*  306  :       TRI_SETF(FARRAY(a, 0)) */
/*  307  :       dataElem = 0 */
/*  308  :       TRI_SETF(FARRAY(a, 4) + _GlideRoot.pool.fHalf) */
/*  309  :       i = gc->tsuDataList[dataElem] */
/*  310  :       while (i != GR_DLIST_END) { */
/*  311  :         TRI_SETF(FARRAY(a, i)) */
/*  312  :         dataElem++ */
/*  313  :         i = gc->tsuDataList[dataElem] */
/*  314  :       } */
/*  315  :       TRI_SETF(256.f) */
/*  316  :       TRI_SETF(0.f) */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat)  */
	movq %mm0 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0) */

	punpckldq %mm4 , %mm7	/*  TRI_SETF(256.f) | previous component */
	pfadd %mm2 , %mm6	/*  FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0)  */

	movq %mm7 , -16(fifo)	/*  PCI write 0 | previous component */
	movq %mm3 , %mm7	/*  0 | 0 */

	punpckldq %mm0 , %mm7	/*  FARRAY(a,0) | 0 */
	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */

	movq %mm7 , -8(fifo)	/*  PCI write FARRAY(a,0) | 0 */
	movq %mm6 , %mm7	/*  FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0) */

	punpckhdq %mm7 , %mm7	/*  write buffer = FARRAY(a,4)-_GlideRoot.pool.fHalf */
	lea 4(dlpStart) , dlp	/*  point to next entry in offset list */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop4_WB1	/*  i == GR_DLIST_END, done with all four vertices */

.L__grDrawTextureLine_3DNow_$vertex_loop4_WB1:
	movd (va,%eax) , %mm6	/*  FARRAY(a,i) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  point to next offset */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  i == GR_DLIST_END ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop4_done_WB0	/*  yup, i == GR_DLIST_END */

	movd (va,%eax) , %mm7	/*  FARRAY(a,i) */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	jnz $vertex_loop4_WB1	/*  nope, more components to handle */

.L__grDrawTextureLine_3DNow_$vertex_loop4_done_WB1:
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */
	punpckldq %mm4 , %mm7	/*  previous component | TRI_SETF(256.f) */

	movq %mm7 , -8(fifo)	/*  PCI write previous component | TRI_SETF(256.f) */
	movq %mm4 , %mm7	/*  TRI_SETF(0.f) | TRI_SETF(256.f) */

	punpckhdq %mm7 , %mm7	/*  write buffer = TRI_SETF(0.f) */
	jmp .L__grDrawTextureLine_3DNow_$line_done_WB1	/*  last vertex done */

.L__grDrawTextureLine_3DNow_$j_lt_i2_WB1:

/*  317  :     } else { /* y major */ */
/*  318  :       TRI_SETF(FARRAY(b, 0) - _GlideRoot.pool.fHalf) */
/*  319  :       dataElem = 0 */
/*  320  :       TRI_SETF(FARRAY(b, 4)) */
/*  321  :       i = gc->tsuDataList[dataElem] */
/*  322  :       while (i != GR_DLIST_END) { */
/*  323  :         TRI_SETF(FARRAY(b, i)) */
/*  324  :         dataElem++ */
/*  325  :         i = gc->tsuDataList[dataElem] */
/*  326  :       } */

	psrlq $32 , %mm2	/*  0 | _GlideRoot.pool.fHalf */
	movq %mm1 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0) */

	pfsub %mm2 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0)-_GlideRoot.pool.fHalf  */
	lea tsuDataList(gc) , dlpStart	/*  gc->tsuDataList */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */
	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */

	punpckldq %mm6 , %mm7	/*  FARRAY(b,0)-_GlideRoot.pool.fHalf | header */
	lea 4(dlpStart) , dlp	/*  next dataElem = 1 */

	movq %mm7 , -8(fifo)	/*  PCI write FARRAY(b,0)-_GlideRoot.pool.fHalf | header */
	movq %mm6 , %mm7	/*  FARRAY(b,4) | FARRAY(b,0)-_GlideRoot.pool.fHalf */

	punpckhdq %mm7 , %mm7	/*  "write buffer" = FARRAY(b,4) */
	cmp $0 , %eax	/*  i != GR_DLIST_END ? */

	nop 	/*  filler */
	je .L__grDrawTextureLine_3DNow_$vertex_loop5_done_WB1	/*  i == GR_DLIST_END, no more components in vertex */

.L__grDrawTextureLine_3DNow_$vertex_loop5_WB1:
	movd (vb,%eax) , %mm6	/*  FARRAY(b,i) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  point to next offset */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  i == GR_DLIST_END ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop5_done_WB0	/*  yup, i == GR_DLIST_END */

	movd (vb,%eax) , %mm7	/*  FARRAY(b,i) */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	jnz $vertex_loop5_WB1	/*  nope, more components to handle */

.L__grDrawTextureLine_3DNow_$vertex_loop5_done_WB1:

/*  327  :       TRI_SETF(0.f) */
/*  328  :       TRI_SETF(0.f) */
/*  329  :            */
/*  330  :       TRI_SETF(FARRAY(a, 0) - _GlideRoot.pool.fHalf) */
/*  331  :       dataElem = 0 */
/*  332  :       TRI_SETF(FARRAY(a, 4)) */
/*  333  :       i = gc->tsuDataList[dataElem] */
/*  334  :       while (i != GR_DLIST_END) { */
/*  335  :         TRI_SETF(FARRAY(a, i)) */
/*  336  :         dataElem++ */
/*  337  :         i = gc->tsuDataList[dataElem] */
/*  338  :       } */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat)  */
	movq %mm0 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0) */

	punpckldq %mm3 , %mm7	/*  0 | previous component */
	pfsub %mm2 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0)-_GlideRoot.pool.fHalf */

	movq %mm7 , -16(fifo)	/*  PCI write 0 | previous component */
	movq %mm3 , %mm7	/*  0 | 0 */

	punpckldq %mm6 , %mm7	/*  FARRAY(a,0)-_GlideRoot.pool.fHalf | 0 */
	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */

	movq %mm7 , -8(fifo)	/*  PCI write FARRAY(a,0) | 0 */
	movq %mm6 , %mm7	/*  FARRAY(a,4) | FARRAY(a,0)-_GlideRoot.pool.fHalf */

	punpckhdq %mm7 , %mm7	/*  write buffer = FARRAY(a,4) */
	lea 4(dlpStart) , dlp	/*  point to next entry in offset list */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop6_done_WB1	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop6_WB1:
	movd (va,%eax) , %mm6	/*  FARRAY(a,i) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  point to next offset */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop6_done_WB0	/*  yup, i == GR_DLIST_END */

	movd (va,%eax) , %mm7	/*  FARRAY(a,i) */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  i == GR_DLIST_END ? */
	jne .L__grDrawTextureLine_3DNow_$vertex_loop6_WB1	/*  nope, more components to handle */

.L__grDrawTextureLine_3DNow_$vertex_loop6_done_WB1:

/*  339  :       TRI_SETF(0.f) */
/*  340  :       TRI_SETF(0.f) */
/*  341  :        */
/*  342  :       TRI_SETF(FARRAY(b, 0) + _GlideRoot.pool.fHalf) */
/*  343  :       dataElem = 0 */
/*  344  :       TRI_SETF(FARRAY(b, 4)) */
/*  345  :       i = gc->tsuDataList[dataElem] */
/*  346  :       while (i != GR_DLIST_END) { */
/*  347  :         TRI_SETF(FARRAY(b, i)) */
/*  348  :         dataElem++ */
/*  349  :         i = gc->tsuDataList[dataElem] */
/*  350  :       } */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat) */
	movq %mm1 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0) */

	punpckldq %mm3 , %mm7	/*  0 | previous component */
	pfadd %mm2 , %mm6	/*  FARRAY(b,4) | FARRAY(b,0)+_GlideRoot.pool.fHalf */

	movq %mm7 , -16(fifo)	/*  PCI write 0 | previous component */
	movq %mm3 , %mm7	/*  0 | 0 */

	punpckldq %mm6 , %mm7	/*  FARRAY(b,0)+_GlideRoot.pool.fHalf | 0 */
	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */

	movq %mm7 , -8(fifo)	/*  PCI write FARRAY(b,0)+_GlideRoot.pool.fHalf | 0 */
	movq %mm6 , %mm7	/*  FARRAY(b,4) | FARRAY(b,0)+_GlideRoot.pool.fHalf */

	punpckhdq %mm7 , %mm7	/*  write buffer = FARRAY(b,4) */
	lea 4(dlpStart) , dlp	/*  point to next entry in offset list */

	cmp $0 , %eax	/*  i == GR_DLIST_END ? */
	je .L__grDrawTextureLine_3DNow_$vertex_loop7_done_WB1	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop7_WB1:
	movd (vb,%eax) , %mm6	/*  FARRAY(b,i) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  point to next offset */

	punpckldq %mm6 , %mm7	/*  current component | previous component */
	cmp $0 , %eax	/*  i == GR_DLIST_END ? */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop7_done_WB0	/*  yup, i == GR_DLIST_END */

	movd (vb,%eax) , %mm7	/*  FARRAY(b,i) */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	jnz $vertex_loop7_WB1	/*  nope, more components to handle */

.L__grDrawTextureLine_3DNow_$vertex_loop7_done_WB1:

/*  351  :       TRI_SETF(256.f) */
/*  352  :       TRI_SETF(0.f) */
/*  353  :        */
/*  354  :       TRI_SETF(FARRAY(a, 0) + _GlideRoot.pool.fHalf) */
/*  355  :       dataElem = 0 */
/*  356  :       TRI_SETF(FARRAY(a, 4)) */
/*  357  :       i = gc->tsuDataList[dataElem] */
/*  358  :       while (i != GR_DLIST_END) { */
/*  359  :         TRI_SETF(FARRAY(a, i)) */
/*  360  :         dataElem++ */
/*  361  :         i = gc->tsuDataList[dataElem] */
/*  362  :       } */
/*  363  :       TRI_SETF(256.f) */
/*  364  :       TRI_SETF(0.f) */

	add $16 , fifo	/*  fifoPtr += 4*sizeof(FxFloat) */
	movq %mm0 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0) */

	punpckldq %mm4 , %mm7	/*  256.0f | previous component */
	pfadd %mm2 , %mm6	/*  FARRAY(a,4) | FARRAY(a,0)+_GlideRoot.pool.fHalf */

	movq %mm7 , -16(fifo)	/*  PCI write 0 | previous component */
	movq %mm3 , %mm7	/*  0 | 0 */

	punpckldq %mm6 , %mm7	/*  FARRAY(a,0)+_GlideRoot.pool.fHalf | 0 */
	mov (dlpStart) , %eax	/*  i = gc->tsuDataList[dataElem] */

	movq %mm7 , -8(fifo)	/*  PCI write FARRAY(b,0)+_GlideRoot.pool.fHalf | 0 */
	movq %mm6 , %mm7	/*  FARRAY(a,4) | FARRAY(a,0)+_GlideRoot.pool.fHalf */

	punpckhdq %mm7 , %mm7	/*  write buffer = FARRAY(a,4) */
	lea 4(dlpStart) , dlp	/*  point to next entry in offset list */

	test %eax , %eax	/*  i == GR_DLIST_END ? */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop8_done_WB1	/*  i == GR_DLIST_END, no further components need to be pushed out */

.L__grDrawTextureLine_3DNow_$vertex_loop8_WB1:
	movd (va,%eax) , %mm6	/*  FARRAY(a,i) */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  point to next offset */

	cmp $0 , %eax	/*  i == GR_DLIST_END ? */
	punpckldq %mm6 , %mm7	/*  current component | previous component */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jz .L__grDrawTextureLine_3DNow_$vertex_loop8_done_WB0	/*  yup, i == GR_DLIST_END */

	movd (va,%eax) , %mm7	/*  FARRAY(a,i) */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  i == GR_DLIST_END ? */
	jnz $vertex_loop8_WB1	/*  nope, more components to handle */

.L__grDrawTextureLine_3DNow_$vertex_loop8_done_WB1:

	punpckldq %mm4 , %mm7	/*  256.0f | previous component */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movq %mm7 , -8(fifo)	/*  PCI write TRI_SETF(256.f) | previous component */
	movq %mm4 , %mm7	/*  256.0f | 0 */

	punpckhdq %mm7 , %mm7	/*  write buffer = 256.0f */
	nop 	/*  filler */

.L__grDrawTextureLine_3DNow_$line_done_WB1:

	movd %mm7 , (fifo)	/*  flush "write buffer" */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

.L__grDrawTextureLine_3DNow_$line_done_WB0:

/*  365  :     } */
/*  366  :     TRI_END */
/*  367  :     GR_CHECK_SIZE() */
/*  368  :   } */
/*  369  :   _GlideRoot.stats.linesDrawn++ */
/*  370  :   _GlideRoot.stats.othertrisDrawn+=2 */
/*  371  :  */
/*  372  : #undef FN_NAME */
/*  373  : } /* grDrawTextureLine */ */

	mov fifoPtr(gc) , %eax	/*  old fifoPtr */
	mov fifo , fifoPtr(gc)	/*  save new fifoPtr */

	mov fifoRoom(gc) , %edx	/*  fifo space available before */
	nop 	/*  filler */

	sub fifo , %eax	/*  fifo space used up */
	mov (_GlideRoot+stats_linesDrawn) , %ebx	/*  _GlideRoot.stats.linesDrawn */

	add %eax , %edx	/*  fifo space available now */
	mov (_GlideRoot+stats_otherTrisDrawn) , %ebp	/*  _GlideRoot.stats.othertrisDrawn */

	mov %edx , fifoRoom(gc)	/*  save available fifo space */
	inc %ebx	/*  _GlideRoot.stats.linesDrawn++ */

	mov %ebx , (_GlideRoot+stats_linesDrawn)	/*  save _GlideRoot.stats.linesDrawn */
	add $2 , %ebp	/*  _GlideRoot.stats.othertrisDrawn+=2 */

	mov %ebp , (_GlideRoot+stats_otherTrisDrawn)	/*  _GlideRoot.stats.othertrisDrawn+=2 */
	nop 	/*  filler */
.L__grDrawTextureLine_3DNow_$line_all_done:

	femms 	/*  done with MMX empty FPU/MMX state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %edi	/*  restore caller's register variable */
	pop %esi	/*  restore caller's register variable */

	ret	/*  return and pop 2 DWORD parameters */
.L_END___grDrawTextureLine_3DNow:
.size __grDrawTextureLine_3DNow,.L_END___grDrawTextureLine_3DNow-__grDrawTextureLine_3DNow

.globl __grDrawTriangles_3DNow
.type __grDrawTriangles_3DNow,@function
__grDrawTriangles_3DNow:

#define _mode 20
#define _count 24
#define _pointers 28

/*  930  : { */
/*  931  : #define FN_NAME "_grDrawTriangles_3DNow" */
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

#define gc %edi	/*  points to graphics context */
#define fifo %ecx	/*  points to next entry in fifo */
#define dlp %ebp	/*  points to dataList structure */
#define vertexCount %esi	/*  Current vertex counter in the packet */
#define vertexPtr %ebx	/*  Current vertex pointer (in deref mode) */
#define vertex %ebx	/*  Current vertex (in non-deref mode) */
#define dlpStart %edx	/*  Pointer to start of offset list */

	push %edi	/*  save caller's register variable */
	mov (_GlideRoot+curGC) , gc	/*  get current graphics context */

	push %esi	/*  save caller's register variable */
	mov _count-8(%esp) , vertexCount	/*  number of vertices in triangles */

	push %ebx	/*  save caller's register variable */
	push %ebp	/*  save frame pointer */

	mov _pointers(%esp) , vertexPtr	/*  get current vertex pointer (deref mode) */
	test vertexCount , vertexCount	/*  number of vertices <= 0 ? */

	jle .L__grDrawTriangles_3DNow_$tris_done	/*  yup, triangles are done */

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

	mov (_GlideRoot+trisProcessed) , %eax	/*  trisProcessed */
	shr $1 , %edx	/*  count/3 */

	add %edx , %eax	/*  trisProcessed += count/3 */
	mov _mode(%esp) , %edx	/*  get mode (0 or 1) */

	mov CoordinateSpace(gc) , %ecx	/*  coordinates space (window/clip) */
	mov %eax , (_GlideRoot+trisProcessed)	/*  trisProcessed */

	test %edx , %edx	/*  mode 0 (array of vertices) ? */
	jnz $deref_mode	/*  nope, it's mode 1 (array of pointers to vertices) */

	mov vertexStride(gc) , %edx	/*  get stride in DWORDs */
	nop 	/*  filler */

	shl $2 , %edx	/*  stride in bytes */
	cmp $0 , %ecx	/*  coordinate space == 0 (window) ? */

	mov %edx , (strideinbytes)	/*  save off stride (in bytes) */
	jnz $clip_coordinates_ND	/*  nope, coordinate space != window   */

/*  961  :     while (count > 0) { */
/*  962  :       FxI32 vcount = count >=15 ? 15 : count */
/*  963  :       GR_SET_EXPECTED_SIZE(vcount * gc->state.vData.vSize, 1) */
/*  964  :       TRI_STRIP_BEGIN(kSetupStrip, vcount, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD) */
/*  965  :        */

.L__grDrawTriangles_3DNow_$win_coords_loop_ND:

	sub $15 , vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L__grDrawTriangles_3DNow_$win_tri_begin_ND	/*  yup, start writing triangle data */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */

.L__grDrawTriangles_3DNow_$win_tri_begin_ND:

	mov vertexCount , %eax	/*  number of vertices in triangles */
	mov fifoPtr(gc) , fifo	/*  get fifoPtr */

	mov cullStripHdr(gc) , %ebp	/*  <2:0> = type */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	or %ebp , %eax	/*  setup vertex count and type */
	mov $4 , %ebp	/*  test bit 2 */

	lea tsuDataList(gc) , dlpStart	/*  pointer to start of offset list */
	test %ebp , fifo	/*  fifoPtr QWORD aligned ? */

	jz .L__grDrawTriangles_3DNow_$fifo_aligned_ND	/*  yup */

	mov %eax , (fifo)	/*  PCI write packet type */
	add $4 , fifo	/*  fifo pointer now QWORD aligned */

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

.L__grDrawTriangles_3DNow_$win_vertex_loop_ND_WB0:/*  nothing in "write buffer" */

	mov (dlpStart) , %eax	/*  get first offset from offset list */
	mov dlpStart , dlp	/*  point to start of offset list */

	movq (vertex) , %mm1	/*  get vertex x,y */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	add $4 , dlp	/*  dlp++ */
	test %eax , %eax	/*  if offset == 0, end of list */

	movq %mm1 , -8(fifo)	/*  PCI write x, y */
	jz .L__grDrawTriangles_3DNow_$win_datalist_end_ND_WB0	/*  no more vertex data, nothing in "write buffer"  */

.L__grDrawTriangles_3DNow_$win_datalist_loop_ND_WB0:/*  nothing in "write buffer" */

	movd (vertex,%eax) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  get next offset from offset list */

	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
	jz .L__grDrawTriangles_3DNow_$win_datalist_end_ND_WB1	/*  exit, write buffer contains one DWORD */

	movd (vertex,%eax) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp++ */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */
	punpckldq %mm2 , %mm1	/*  current param | previous param */

	movq %mm1 , -8(fifo)	/*  PCI write current param | previous param */
	jnz $win_datalist_loop_ND_WB0	/*  nope, copy next parameter */

.L__grDrawTriangles_3DNow_$win_datalist_end_ND_WB0:

	mov (strideinbytes) , %eax	/*  get offset to next vertex */
	dec vertexCount	/*  another vertex done. Any left? */

	lea (vertex,%eax) , vertex	/*  points to next vertex */
	jnz $win_vertex_loop_ND_WB0	/*  yup, output next vertex */

.L__grDrawTriangles_3DNow_$win_vertex_end_ND_WB0:

	mov fifoPtr(gc) , %eax	/*  old fifoPtr */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	sub $15 , vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	test vertexCount , vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L__grDrawTriangles_3DNow_$win_coords_loop_ND	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters off stack */

.L__grDrawTriangles_3DNow_$fifo_aligned_ND:

	movd %eax , %mm1	/*  move header into "write buffer" */

.L__grDrawTriangles_3DNow_$win_vertex_loop_ND_WB1:/*  one DWORD in "write buffer" */

	movd (vertex) , %mm2	/*  0 | x of vertex */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	lea 4(dlpStart) , dlp	/*  point to start of offset list */
	nop 	/*  filler */

	punpckldq %mm2 , %mm1	/*  packet header | x of vertex */
	mov -4(dlp) , %eax	/*  first offset in offset list */

	movq %mm1 , -8(fifo)	/*  PCI write packet header | x of vertex */
	movd 4(vertex) , %mm1	/*  0 | y of vertex */

	test %eax , %eax	/*  offset == 0 (list empty) ? */
	jz .L__grDrawTriangles_3DNow_$win_datalist_end_ND_WB1	/*  yup, no more vertex data, one DWORD in "write buffer" */

.L__grDrawTriangles_3DNow_$win_datalist_loop_ND_WB1:/*  one DWORD in "write buffer"  */

	movd (vertex,%eax) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(fifo)	/*  PCI write current param | previous param */
	jz .L__grDrawTriangles_3DNow_$win_datalist_end_ND_WB0	/*  yes, exit, "write buffer" empty */

	movd (vertex,%eax) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz $win_datalist_loop_ND_WB1	/*  nope, copy next parameter */

.L__grDrawTriangles_3DNow_$win_datalist_end_ND_WB1:

	mov (strideinbytes) , %eax	/*  get offset to next vertex */
	dec vertexCount	/*  another vertex done. Any left? */

	lea (vertex,%eax) , vertex	/*  points to next vertex */
	jnz $win_vertex_loop_ND_WB1	/*  yup, output next vertex */

.L__grDrawTriangles_3DNow_$win_vertex_end_ND_WB1:

	movd %mm1 , (fifo)	/*  flush "write buffer" */
	mov fifoPtr(gc) , %eax	/*  old fifoPtr */

	add $4 , fifo	/*  fifoPtr += sizeof(FxU32) */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */
	sub $15 , vertexCount	/*  remaining number of vertices to process */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	test vertexCount , vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L__grDrawTriangles_3DNow_$win_coords_loop_ND	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters off stack */

.L__grDrawTriangles_3DNow_$deref_mode:

	prefetch (vertexPtr)	/*  pre-load first group of pointers */

	test %ecx , %ecx	/*  coordinate space == 0 (window) ? */
	jnz $clip_coordinates_D	/*  nope, coordinate space != window */

.L__grDrawTriangles_3DNow_$win_coords_loop_D:

	sub $15 , vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L__grDrawTriangles_3DNow_$win_tri_begin_D	/*  yup, start writing triangle data */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */
	nop 	/*  filler */

.L__grDrawTriangles_3DNow_$win_tri_begin_D:

	mov vertexCount , %eax	/*  number of vertices in triangles */
	mov fifoPtr(gc) , fifo	/*  get fifoPtr */

	mov cullStripHdr(gc) , %ebp	/*  <2:0> = type */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	or %ebp , %eax	/*  setup mode, vertex count, and type */
	mov $4 , %ebp	/*  test bit 2 */

	lea tsuDataList(gc) , dlpStart	/*  pointer to start of offset list */
	test %ebp , fifo	/*  fifoPtr QWORD aligned ? */

	nop 	/*  filler */
	jz .L__grDrawTriangles_3DNow_$fifo_aligned_D	/*  yup */

	mov %eax , (fifo)	/*  PCI write packet type */
	add $4 , fifo	/*  fifo pointer now QWORD aligned */

.L__grDrawTriangles_3DNow_$win_vertex_loop_D_WB0:/*  nothing in "write buffer" */

	mov (vertexPtr) , %edx	/*  dereference pointer, edx points to vertex */
	add $4 , vertexPtr	/*  next pointer */

	lea tsuDataList(gc) , dlp	/*  get pointer to offset list */
	movq (%edx) , %mm1	/*  get vertex x,y */

	mov (dlp) , %eax	/*  get first offset from offset list */
	add $4 , dlp	/*  dlp++ */

	movq %mm1 , (fifo)	/*  PCI write x, y */
	add $8 , fifo	/*  fifo += 2 */

	test %eax , %eax	/*  if offset == 0, end of offset list */
	je .L__grDrawTriangles_3DNow_$win_datalist_end_D_WB0	/*  no more vertex data, nothing in "write buffer"  */

.L__grDrawTriangles_3DNow_$win_datalist_loop_D_WB0:/*  nothing in "write buffer" */

	movd (%edx,%eax) , %mm1	/*  get next parameter */
	mov (dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jz .L__grDrawTriangles_3DNow_$win_datalist_end_D_WB1	/*  exit, write buffer contains one DWORD */

	movd (%edx,%eax) , %mm2	/*  get next parameter */
	add $8 , dlp	/*  dlp++ */

	mov -4(dlp) , %eax	/*  get next offset from offset list */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	cmp $0 , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(fifo)	/*  PCI write current param | previous param */
	jnz $win_datalist_loop_D_WB0	/*  nope, copy next parameter */

.L__grDrawTriangles_3DNow_$win_datalist_end_D_WB0:

	dec vertexCount	/*  another vertex done. Any left? */
	jnz $win_vertex_loop_D_WB0	/*  yup, output next vertex */

.L__grDrawTriangles_3DNow_$win_vertex_end_D_WB0:

	mov fifoPtr(gc) , %eax	/*  old fifoPtr */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	add %eax , %ebp	/*  new number of bytes available in fifo */
	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */

	sub $15 , vertexCount	/*  remaining number of vertices to process */
	nop 	/*  filler */

	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */
	test vertexCount , vertexCount	/*  any vertices left to process ? */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process */
	jg .L__grDrawTriangles_3DNow_$win_coords_loop_D	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters off stack */

.L__grDrawTriangles_3DNow_$fifo_aligned_D:

	movd %eax , %mm1	/*  move header into "write buffer" */

.L__grDrawTriangles_3DNow_$win_vertex_loop_D_WB1:/*  one DWORD in "write buffer" */

	mov (vertexPtr) , %edx	/*  dereference pointer, edx points to vertex */
	add $4 , vertexPtr	/*  next pointer */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */
	lea tsuDataList(gc) , dlp	/*  get pointer to start of offset list */

	movd (%edx) , %mm2	/*  0 | x of vertex */
	add $4 , dlp	/*  dlp++ */

	mov -4(dlp) , %eax	/*  first offset in offset list */
	punpckldq %mm2 , %mm1	/*  packet header | x of vertex */

	movq %mm1 , -8(fifo)	/*  PCI write packet header | x of vertex */
	movd 4(%edx) , %mm1	/*  0 | y of vertex */

	cmp $0 , %eax	/*  offset == 0 (list empty) ? */
	je .L__grDrawTriangles_3DNow_$win_datalist_end_D_WB1	/*  yup, no more vertex data, one DWORD in "write buffer" */

.L__grDrawTriangles_3DNow_$win_datalist_loop_D_WB1:/*  one DWORD in "write buffer" = MM1 */

	movd (%edx,%eax) , %mm2	/*  get next parameter */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxU32) */

	mov (dlp) , %eax	/*  get next offset from offset list */
	add $8 , dlp	/*  dlp += 2 */

	punpckldq %mm2 , %mm1	/*  current param | previous param */
	test %eax , %eax	/*  at end of offset list (offset == 0) ? */

	movq %mm1 , -8(fifo)	/*  PCI write current param | previous param */
	jz .L__grDrawTriangles_3DNow_$win_datalist_end_D_WB0	/*  yes, exit, "write buffer" empty */

	movd (%edx,%eax) , %mm1	/*  get next parameter */
	mov -4(dlp) , %eax	/*  get next offset from offset list */

	test %eax , %eax	/*  at end of offset list (offset == 0) ? */
	jnz $win_datalist_loop_D_WB1	/*  nope, copy next parameter */

.L__grDrawTriangles_3DNow_$win_datalist_end_D_WB1:

	dec vertexCount	/*  another vertex done. Any left? */
	jnz $win_vertex_loop_D_WB1	/*  yup, output next vertex */

.L__grDrawTriangles_3DNow_$win_vertex_end_D_WB1:

	movd %mm1 , (fifo)	/*  flush "write buffer" */
	mov fifoPtr(gc) , %eax	/*  old fifoPtr */

	add $4 , fifo	/*  fifoPtr++ */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */
	sub $15 , vertexCount	/*  remaining number of vertices to process */

	add %eax , %ebp	/*  new number of bytes available in fifo */
	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	cmp $0 , vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L__grDrawTriangles_3DNow_$win_coords_loop_D	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters off stack */

.align 32

/*  989  :   } */
/*  990  :   else { */
/*  991  :     /* */
/*  992  :      * first cut of clip space coordinate code, no optimization. */
/*  993  :      */ */
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
/*  1008 :         /* x, y */ */
/*  1009 :         TRI_SETF(FARRAY(vPtr, 0) */
/*  1010 :                  *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox) */
/*  1011 :         TRI_SETF(FARRAY(vPtr, 4) */
/*  1012 :                  *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) */
/*  1013 :         (float *)pointers += stride */
/*  1014 :          */
/*  1015 :         TRI_VP_SETFS(vPtr,oow) */
/*  1016 :       } */
/*  1017 :       TRI_END */

#ifndef GLIDE3_SCALER

.L__grDrawTriangles_3DNow_$clip_coordinates_D:

	mov $4 , (strideinbytes)	/*  unit stride for array of pointers to vertices */

.L__grDrawTriangles_3DNow_$clip_coordinates_ND:

#define dataElem %ebp	/*  number of vertex components processed */

	movd (_GlideRoot+pool_f255) , %mm6	/*  GlideRoot.pool.f255  */

.L__grDrawTriangles_3DNow_$clip_coords_begin:

/*      } */
/*      else { */
/*      /* */
/*       * first cut of clip space coordinate code, no optimization. */
/*       */ */
/*      float oow */
/*        while (count > 0) { */
/*        FxI32 k, vcount = count >= 15 ? 15 : count */

	sub $15 , vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L__grDrawTriangles_3DNow_$clip_tri_begin	/*  yup, start writing triangle data */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */

.L__grDrawTriangles_3DNow_$clip_tri_begin:

	mov vertexCount , %edx	/*  number of vertices in triangles */
	mov fifoPtr(gc) , fifo	/*  get fifoPtr */

	mov cullStripHdr(gc) , %ebp	/*  <2:0> = type */
	shl $6 , %edx	/*  <9:6> = vertex count (max 15) */

	or %ebp , %edx	/*  setup mode, vertex count, and type */

	mov %edx , (fifo)	/*  PCI write packet type */
	add $4 , fifo	/*  fifo pointer now QWORD aligned */

.L__grDrawTriangles_3DNow_$clip_for_begin:

	mov vertexPtr , %edx	/*  vertex = vertexPtr (assume no-deref mode) */
	mov _mode(%esp) , %eax	/*  mode 0 = no deref, mode 1 = deref */

	mov vertexCount , (vertices)	/*  save numnber of vertices */
	test %eax , %eax	/*  deref mode ? */

	mov wInfo_offset(gc) , %eax	/*  get offset of W into vertex struct */
	jz .L__grDrawTriangles_3DNow_$clip_noderef	/*  yup, no-deref mode */

	mov (vertexPtr) , %edx	/*  vertex = *vertexPtr */

.L__grDrawTriangles_3DNow_$clip_noderef:

	movd (%edx,%eax) , %mm0	/*  0 | W of current vertex */
	pfrcp %mm0 , %mm1	/*  0 | 1/W approx */

	mov (strideinbytes) , %ebp	/*  offset to next vertex/vertexPtr */
	movq (%edx) , %mm2	/*  y | x of current vertex */

	pfrcpit1 %mm1 , %mm0	/*  0 | 1/W refine */
	movq vp_hwidth(gc) , %mm3	/*  gc->state.Viewport.hheight | gc->state.Viewport.hwidth */

	movq vp_ox(gc) , %mm4	/*  gc->state.Viewport.oy | gc->state.Viewport.ox */
	add %ebp , vertexPtr	/*  point to next vertex/VertexPtr */

	pfrcpit2 %mm1 , %mm0	/*  oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset */
	mov paramIndex(gc) , %esi	/*  gc->state.paramIndex */

	pfmul %mm3 , %mm2	/*  TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth */
	xor dataElem , dataElem	/*  dataElem = 0 */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */
	punpckldq %mm0 , %mm0	/*  oow | oow */

	pfmul %mm0 , %mm2	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth */
	pfadd %mm4 , %mm2	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) | */

	test $3 , %esi	/*  STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA ? */
	mov tsuDataList(gc) , %eax	/*  first entry from offset list */

	movq %mm2 , -8(fifo)	/*  PCI write transformed x, y */
	jz .L__grDrawTriangles_3DNow_$clip_setup_ooz	/*  nope, no color at all needed */

	cmpl $0 , colorType(gc)	/*  gc->state.vData.colorType == GR_FLOAT ? */
	jne .L__grDrawTriangles_3DNow_$clip_setup_pargb	/*  nope, packed ARGB format */

	test $1 , %esi	/*  STATE_REQUIRES_IT_DRGB ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_a	/*  no, but definitely A */

	movd (%edx,%eax) , %mm2	/*  0 | r */
	mov tsuDataList+4(gc) , %eax	/*  offset of g part of vertex data */

	pfmul %mm6 , %mm2	/*  0 | r * 255.0f */
	movd (%edx,%eax) , %mm3	/*  0 | g */

	mov tsuDataList+8(gc) , %eax	/*  offset of b part of vertex data */
	movd %mm2 , (fifo)	/*  PCI write r*255 */

	pfmul %mm6 , %mm3	/*  0 | g * 255.0f */
	movd (%edx,%eax) , %mm2	/*  0 | b */

	movd %mm3 , 4(fifo)	/*  PCI write g*255 */
	mov $12 , dataElem	/*  dataElem = 3 */

	pfmul %mm6 , %mm2	/*  0 | b * 255.0f */
	mov tsuDataList+12(gc) , %eax	/*  offset of A part of vertex data */

	test $2 , %esi	/*  STATE_REQUIRES_IT_ALPHA ? */
	lea 12(fifo) , fifo	/*  fifoPtr += 3*sizeof(FxFloat) */

	movd %mm2 , -4(fifo)	/*  PCI write b*255 */
	jz .L__grDrawTriangles_3DNow_$clip_setup_ooz	/*  nope, no alpha, proceeed with ooz */

.L__grDrawTriangles_3DNow_$clip_setup_a:
	movd (%eax,%edx) , %mm2	/*  0 | a */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	mov %esp , %esp	/*  filler */
	add $4 , dataElem	/*  dataElem++  */

	pfmul %mm6 , %mm2	/*  0 | a * 255.0f */
	mov tsuDataList(gc,dataElem) , %eax	/*  offset of next part of vertex data */

	movd %mm2 , -4(fifo)	/*  PCI write a*255 */
	jmp .L__grDrawTriangles_3DNow_$clip_setup_ooz	/*  check whether we need to push out z */

.L__grDrawTriangles_3DNow_$clip_setup_pargb:
	movd (%eax,%edx) , %mm2	/*  get packed ARGB data */
	add $4 , fifo	/*  fifoPtr += sizeof(FxU32) */

	mov $4 , dataElem	/*  dataElem = 1 (namely pargb) */
	mov tsuDataList+4(gc) , %eax	/*  offset of next part of vertex data */

	movd %mm2 , -4(fifo)	/*  PCI write packed ARGB */
	nop 	/*  filler */

.L__grDrawTriangles_3DNow_$clip_setup_ooz:

	test $4 , %esi	/*  STATE_REQUIRES_OOZ ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_qow	/*  nope */

	movd (%eax,%edx) , %mm2	/*  0 | z component of vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , dataElem	/*  dataElem += 1 */
	movd vp_hdepth(gc) , %mm3	/*  0 | gc->state.Viewport.hdepth */

	pfmul %mm0 , %mm2	/*  TRI_SETF(FARRAY(_s, i)*_oow */
	movd vp_oz(gc) , %mm4	/*  0 | gc->state.Viewport.oz */

	pfmul %mm3 , %mm2	/*  0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth */
	mov tsuDataList(gc,dataElem) , %eax	/*  offset of next vertex component */

	pfadd %mm4 , %mm2	/*  0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz */
	movd %mm2 , -4(fifo)	/*  PCI write transformed Z */

.L__grDrawTriangles_3DNow_$clip_setup_qow:
	test $8 , %esi	/*  STATE_REQUIRES_OOW_FBI ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_qow0	/*  nope */

	cmpl $0 , qInfo_mode(gc)	/*  does vertex have Q component ? */
	je .L__grDrawTriangles_3DNow_$clip_setup_oow	/*  nope, not Q but W */

	mov qInfo_offset(gc) , %eax	/*  offset of Q component of vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q of vertex */

	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q*oow */

	movd %mm2 , -4(fifo)	/*  PCI write transformed Q */
	jmp .L__grDrawTriangles_3DNow_$clip_setup_qow0	/*  continue with q0 */

.L__grDrawTriangles_3DNow_$clip_setup_oow:
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , dataElem	/*  dataElem++ */

	movd %mm0 , -4(fifo)	/*  PCI write oow */
	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */

.L__grDrawTriangles_3DNow_$clip_setup_qow0:
	test $16 , %esi	/*  STATE_REQUIRES_W_TMU0 ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_stow0	/*  nope  */

	cmpl $0 , q0Info_mode(gc)	/*  does vertex have Q component ? */
	je .L__grDrawTriangles_3DNow_$clip_setup_oow0	/*  nope, not Q but W */

	mov q0Info_offset(gc) , %eax	/*  offset of Q component of vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q0 of vertex */

	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q0*oow */

	movd %mm2 , -4(fifo)	/*  PCI write transformed q0 */
	jmp .L__grDrawTriangles_3DNow_$clip_setup_stow0	/*  continue with stow0 */

	nop 	/*  filler */

.L__grDrawTriangles_3DNow_$clip_setup_oow0:
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , dataElem	/*  dataElem++ */

	movd %mm0 , -4(fifo)	/*  PCI write oow */
	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */

.L__grDrawTriangles_3DNow_$clip_setup_stow0:

	test $32 , %esi	/*  STATE_REQUIRES_ST_TMU0 ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_qow1	/*  nope */

	movq tmu0_s_scale(gc) , %mm7	/*  state.tmu_config[0].t_scale | state.tmu_config[0].s_scale */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movd (%edx,%eax) , %mm2	/*  param1 */
	mov tsuDataList+4(gc,dataElem) , %eax	/* pointer to next vertex component */

	pfmul %mm0 , %mm7	/*  oow*tmu0_t_scale | oow*tmu0_s_scale */
	add $8 , dataElem	/*  dataElem += 2 */

	movd (%edx,%eax) , %mm3	/*  param2 */
	punpckldq %mm3 , %mm2	/*  param2 | param1 */

	pfmul %mm7 , %mm2	/*  param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale */
	nop 	/*  filler */

	movq %mm2 , -8(fifo)	/*  PCI write param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale */
	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */

.L__grDrawTriangles_3DNow_$clip_setup_qow1:
	test $64 , %esi	/*  STATE_REQUIRES_W_TMU1 ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_stow1	/*  nope */

	cmpl $0 , q1Info_mode(gc)	/*  does vertex have Q component ? */
	je .L__grDrawTriangles_3DNow_$clip_setup_oow1	/*  nope, not Q but W */

	mov q1Info_offset(gc) , %eax	/*  offset of Q component of vertex */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	add $4 , dataElem	/*  dataElem++ */
	movd (%edx,%eax) , %mm2	/*  0 | q1 of vertex */

	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm2	/*  q1*oow */

	movd %mm2 , -4(fifo)	/*  PCI write transformed q1 */
	jmp .L__grDrawTriangles_3DNow_$clip_setup_stow1	/*  continue with stow1 */

.L__grDrawTriangles_3DNow_$clip_setup_oow1:
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat)  */
	add $4 , dataElem	/*  dataElem++ */

	movd %mm0 , -4(fifo)	/*  PCI write oow */
	mov tsuDataList(gc,dataElem) , %eax	/*  pointer to next vertex component */

.L__grDrawTriangles_3DNow_$clip_setup_stow1:

	test $128 , %esi	/*  STATE_REQUIRES_ST_TMU1 ? */
	mov (vertices) , vertexCount	/*  get number of vertices */

	movq tmu1_s_scale(gc) , %mm7	/*  state.tmu_config[1].t_scale | state.tmu_config[1].s_scale */
	jz .L__grDrawTriangles_3DNow_$clip_setup_end	/*  nope */

	movd (%edx,%eax) , %mm2	/*  param1 */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	mov tsuDataList+4(gc,dataElem) , %eax	/*  pointer to next vertex component */
	pfmul %mm0 , %mm7	/*  oow*state.tmu_config[1].t_scale | oow*state.tmu_config[1].s_scale */

	movd (%edx,%eax) , %mm3	/*  param2 */
	punpckldq %mm3 , %mm2	/*  param2 | param1 */

	pfmul %mm7 , %mm2	/*  param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale */
	movq %mm2 , -8(fifo)	/*  PCI write param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale */

.L__grDrawTriangles_3DNow_$clip_setup_end:

	dec vertexCount	/*  vcount-- */
	jnz $clip_for_begin	/*  until  */

.L__grDrawTriangles_3DNow_$clip_for_end:

	mov fifoPtr(gc) , %eax	/*  old fifoPtr */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	sub $15 , vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	cmp $0 , vertexCount	/*  any vertices left to process ? */

	jg .L__grDrawTriangles_3DNow_$clip_coords_begin	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

#else	/*  GLIDE3_SCALER */

/* ---------------------------------------------------------------------------- */



/* ---------------------------------------------------------------------------- */
/*  table based method */

/* ---------------------------------------------------------------------------- */



/* ---------------------------------------------------------------------------- */

#define dataElem %ebp	/*  number of vertex components processed */

	nop 	/*  filler for code alignment */

.L__grDrawTriangles_3DNow_$clip_coordinates_ND:

	pxor %mm0 , %mm0	/*  load 0 */
	movd (_GlideRoot+pool_f255) , %mm1	/*  GlideRoot.pool.f255  */

	movd %mm0 , (atab)	/*  atable[0] = 0.0f */
	movd vp_oz(gc) , %mm5	/*  gc->state.Viewport.oz */

	movq vp_hwidth(gc) , %mm2	/*  gc->state.Viewport.hheight | gc->state.Viewport.hwidth */
	movd %mm1 , (atab+8)	/*  atable[2] = GlideRoot.pool.f255 */

	movq vp_ox(gc) , %mm1	/*  gc->state.Viewport.oy | gc->state.Viewport.ox */
	movd %mm5 , (btab+4)	/*  btable[1] = gc->state.Viewport.oz */

	movq %mm0 , (btab+8)	/*  btable[3] = 0.0f | btable[2] = 0.0f */
	nop 	/*  filler */

	movq %mm0 , (btab+16)	/*  btable[5] = 0.0f | btable[4] = 0.0f */
	movq %mm0 , (btab+24)	/*  btable[7] = 0.0f | btable[6] = 0.0f */

.L__grDrawTriangles_3DNow_$clip_coords_begin_ND:

	sub $15 , vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , vertexCount	/*  vertexcount >= 15 ? 15 :vertexcount */

	imul vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L__grDrawTriangles_3DNow_$clip_tri_begin_ND	/*  yup, start writing triangle data */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */
	nop 	/*  filler for code alignment */

.L__grDrawTriangles_3DNow_$clip_tri_begin_ND:

	mov vertexCount , %eax	/*  number of vertices in triangles */
	mov fifoPtr(gc) , fifo	/*  get fifoPtr */

	mov cullStripHdr(gc) , %ebp	/*  <2:0> = type */
	shl $6 , %eax	/*  <9:6> = vertex count (max 15) */

	or %ebp , %eax	/*  setup vertex count and type */
	mov $4 , %ebp	/*  test bit 2 */

	test %ebp , fifo	/*  fifoPtr QWORD aligned ? */

	mov wInfo_offset(gc) , %edx	/*  gc->state.vData.wInfo.offset */
	jz .L__grDrawTriangles_3DNow_$clip_fifo_aligned_ND	/*  yup */

	mov %eax , (fifo)	/*  PCI write packet type */
	add $4 , fifo	/*  fifo pointer now QWORD aligned */

.L__grDrawTriangles_3DNow_$clip_for_begin_WB0_ND:/*  "write buffer" = MM7 is empty */

/*  here:  ebx = vertex */
/*         ecx = fifo */
/*         edx = gc->state.vData.wInfo.offset */
/*         esi = vertexCount */
/*         edi = gc */
/*         mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox */
/*         mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth */
/*  avail: eax, ebp, mm0, mm3, mm4, mm5, mm6, mm7 */

	movd (vertex,%edx) , %mm0	/*  0 | W of current vertex */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movq (vertex) , %mm7	/*  y | x of current vertex */
	mov $0 , dataElem	/*  dataElem = 0 */

	pfrcp %mm0 , %mm4	/*  1/W approx | 1/W approx */
	punpckldq %mm0 , %mm0	/*  W | W */

	pfrcpit1 %mm4 , %mm0	/*  1/W refine | 1/W refine */
	mov colorType(gc) , %eax	/*  gc->state.vData.colorType */

	pfmul %mm2 , %mm7	/*  TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth */
	movd vp_hdepth(gc) , %mm6	/*  gc->state.Viewport.hdepth */

	pfrcpit2 %mm4 , %mm0	/*  oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset | oow = ... */
	cmp $1 , %eax	/*  gc->state.vData.colorType == GR_U8 */

	mov tsuDataList(gc) , %eax	/*  get offset of first component from offset list */
	pfmul %mm0 , %mm7	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth */

	movd %mm0 , (atab+28)	/*  atable[7] = oow */
	pfmul %mm0 , %mm6	/*  oow*gc->state.Viewport.hdepth */

	movq tmu0_s_scale(gc) , %mm3	/*  gc->state.tmu_config[0].t_scale | gc->state.tmu_config[0].s_scale */
	movd %mm0 , (btab)	/*  btable[0] = oow */

	movd %mm6 , (atab+4)	/*  atable[1] = oow*gc->state.Viewport.hdepth */
	pfadd %mm1 , %mm7	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) | */

	pfmul %mm0 , %mm3	/*  oow*gc->state.tmu_config[0].t_scale | oow*gc->state.tmu_config[0].s_scale */
	movq tmu1_s_scale(gc) , %mm4	/*  gc->state.tmu_config[1].t_scale | gc->state.tmu_config[1].s_scale */

	movq %mm7 , -8(fifo)	/*  PCI write transformed x, y write buffer=mm7 empty */
	nop 	/*  filler */

	movq %mm3 , (atab+12)	/*  atable[4] = oow*gc->state.tmu_config[0].t_scale | atable[3] = oow*gc->state.tmu_config[0].s_scale */
	pfmul %mm0 , %mm4	/*  oow*gc->state.tmu_config[1].t_scale | oow*gc->state.tmu_config[1].s_scale */

	movq %mm4 , (atab+20)	/*  atable[6] = oow*gc->state.tmu_config[1].t_scale | atable[5] = oow*gc->state.tmu_config[1].s_scale */
	jne .L__grDrawTriangles_3DNow_$not_pargb_WB0_ND	/*  nope, gc->state.vData.colorType != GR_U8 */

	movd (vertex,%eax) , %mm7	/*  get packed ARGB data 1 DWORD in "write buffer"=mm7 */
	nop 	/*  filler */

	mov tsuDataList+4(gc) , %eax	/*  get offset of next vertex component (after pargb) */
	add $4 , dataElem	/*  dataElem = 1 (namely pargb) */

.L__grDrawTriangles_3DNow_$not_pargb_WB1_ND:

	test %eax , %eax	/*  end of offset list ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_end_WB1_ND	/*  yup */

/*  here:  eax = offset into vertex */
/*         ebx = vertex */
/*         ecx = fifo */
/*         esi = vertexCount */
/*         edi = gc */
/*         ebp = dataElem */
/*         mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox */
/*         mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth */
/*         mm7 = write buffer */
/*  avail: edx, mm0, mm3, mm4, mm5, mm6 */

.L__grDrawTriangles_3DNow_$inner_loop_WB1_ND:
	mov tsuDatLstScal(gc,dataElem) , %edx	/*  iscaler */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movd (vertex,%eax) , %mm4	/*  TRI_SETF(FARRAY(vPtr, i) */
	movd 4(%edx) , %mm6	/*  atable [iscaler] */

	movd 4(%edx) , %mm5	/*  btable [iscaler] */
	pfmul %mm6 , %mm4	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler] */

	mov tsuDataList+4(gc,dataElem) , %eax	/*  next offset from offset list */
	pfadd %mm5 , %mm4	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable [iscaler] */

	punpckldq %mm4 , %mm7	/*  current component | previous component */
	test %eax , %eax	/*  offset == 0 (end of offset list) ? */

	mov tsuDatLstScal+4(gc,dataElem) , %edx	/*  next iscaler */
	movq %mm7 , -8(fifo)	/*  PCI write  new param | previous param */

	jz .L__grDrawTriangles_3DNow_$clip_setup_end_WB0_ND	/*  yup, end of offset list, this vertex done */
	add $8 , dataElem	/*  dataElem++ */

	movd (vertex,%eax) , %mm7	/*  TRI_SETF(FARRAY(vPtr, i) */
	movd 4(%edx) , %mm6	/*  atable [iscaler] */

	movd 4(%edx) , %mm5	/*  btable [iscaler] */
	pfmul %mm6 , %mm7	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler] */

	mov tsuDataList(gc,dataElem) , %eax	/*  next offset from offset list */
	pfadd %mm5 , %mm7	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable [iscaler] */

	cmp $0 , %eax	/*  offset == 0 (end of offset list) ? */
	jnz $inner_loop_WB1_ND	/*  nope, get next component (1 DWORD in "write buffer") */

.L__grDrawTriangles_3DNow_$clip_setup_end_WB1_ND:

	mov (strideinbytes) , %eax	/*  offset to next vertex */
	nop 	/*  filler */

	mov wInfo_offset(gc) , %edx	/*  gc->state.vData.wInfo.offset */
	dec vertexCount	/*  one less vertex to handle */

	lea (vertex,%eax) , vertex	/*  points to next vertex */
	jnz $clip_for_begin_WB1_ND	/*  until all vertices done 1 DWORD in "write buffer" = MM7 */

	movd %mm7 , (fifo)	/*  flush "write buffer" */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	mov %esp , %esp	/*  filler */
	jmp .L__grDrawTriangles_3DNow_$clip_setup_done_ND	/*  all vertices handled, tri strip done */

.L__grDrawTriangles_3DNow_$clip_fifo_aligned_ND:

	movd %eax , %mm7	/*  write buffer has 1 DWORD now */

/*      for (k = 0 k < vcount k++) { */
/*        float *vPtr */
/*        vPtr = pointers */
/*        oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset) */
/*        /* x, y */ */
/*        TRI_SETF(FARRAY(vPtr, 0) */
/*          *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox) */
/*        TRI_SETF(FARRAY(vPtr, 4) */
/*          *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) */

.L__grDrawTriangles_3DNow_$clip_for_begin_WB1_ND:

/*  here:  ebx = vertex */
/*         ecx = fifo */
/*         edx = gc->state.vData.wInfo.offset */
/*         esi = vertexCount */
/*         edi = gc */
/*         mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox */
/*         mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth */
/*         mm7 = write buffer */
/*  avail: eax, ebp, mm0, mm3, mm4, mm5, mm6 */

	mov $0 , dataElem	/*  dataElem = 0 */
	movd (vertex,%edx) , %mm0	/*  0 | W of current vertex */

	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */
	movq (vertex) , %mm5	/*  y | x of current vertex */

	pfrcp %mm0 , %mm4	/*  1/W approx | 1/W approx */
	mov colorType(gc) , %eax	/*  gc->state.vData.colorType */

	punpckldq %mm0 , %mm0	/*  W | W */
	movd vp_hdepth(gc) , %mm6	/*  gc->state.Viewport.hdepth */

	cmp $1 , %eax	/*  gc->state.vData.colorType == GR_U8 */
	pfrcpit1 %mm4 , %mm0	/*  1/W refine | 1/W refine */

	pfmul %mm2 , %mm5	/*  TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth */
	movq tmu0_s_scale(gc) , %mm3	/*  gc->state.tmu_config[0].t_scale | gc->state.tmu_config[0].s_scale */

	pfrcpit2 %mm4 , %mm0	/*  oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset | oow = ... */
	mov tsuDataList(gc) , %eax	/*  get offset of first component from offset list */

	pfmul %mm0 , %mm5	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth */
	movd %mm0 , (atab+28)	/*  atable[7] = oow */

	pfmul %mm0 , %mm6	/*  oow*gc->state.Viewport.hdepth */
	movd %mm0 , (btab)	/*  btable[0] = oow */

	pfmul %mm0 , %mm3	/*  oow*gc->state.tmu_config[0].t_scale | oow*gc->state.tmu_config[0].s_scale */
	movd %mm6 , (atab+4)	/*  atable[1] = oow*gc->state.Viewport.hdepth */

	pfadd %mm1 , %mm5	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) | */
	movq %mm3 , (atab+12)	/*  atable[4] = oow * gc->state.tmu_config[0].t_scale | atable[3] = oow * gc->state.tmu_config[0].s_scale */

	movq tmu1_s_scale(gc) , %mm3	/*  gc->state.tmu_config[1].t_scale | gc->state.tmu_config[1].s_scale */
	punpckldq %mm5 , %mm7	/*  header | transformed x */

	punpckhdq %mm5 , %mm5	/*  transformed y | transformed y */
	movq %mm7 , -8(fifo)	/*  PCI write header | transformed x */

	pfmul %mm0 , %mm3	/*  oow*gc->state.tmu_config[1].t_scale | oow*gc->state.tmu_config[1].s_scale */
	movq %mm5 , %mm7	/*  1 DWORD in "write buffer" (transformed y) */

	movq %mm3 , (atab+20)	/*  atable[6] = oow * gc->state.tmu_config[1].t_scale | oow * gc->state.tmu_config[1].s_scale */
	jne .L__grDrawTriangles_3DNow_$not_pargb_WB1_ND	/*  nope, gc->state.vData.colorType != GR_U8 */

	movd (vertex,%eax) , %mm6	/*  get packed ARGB data */
	mov $4 , dataElem	/*  dataElem = 1 (namely pargb) */

	punpckldq %mm6 , %mm7	/*  mm7 = pargb | transformed y */
	mov tsuDataList+4(gc) , %eax	/*  get offset of next vertex component */

	movq %mm7 , (fifo)	/*  PCI write pargb | transformed y */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

.L__grDrawTriangles_3DNow_$not_pargb_WB0_ND:
	cmp $0 , %eax	/*  end of data offset list ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_end_WB0_ND	/*  yup, this vertex done */

/*  here:  eax = offset into vertex */
/*         ebx = vertex */
/*         ecx = fifo */
/*         esi = vertexCount */
/*         edi = gc */
/*         ebp = dataElem */
/*         mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox */
/*         mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth */
/*         mm3 = F256_F256 */
/*  avail: edx, mm0, mm4, mm5, mm6, mm7 */

.L__grDrawTriangles_3DNow_$inner_loop_WB0_ND:

	mov tsuDatLstScal(gc,dataElem) , %edx	/*  iscaler */
	movd (vertex,%eax) , %mm7	/*  TRI_SETF(FARRAY(vPtr, i) */

	movd 4(%edx) , %mm6	/*  atable[iscaler] */
	movd 4(%edx) , %mm5	/*  btable[iscaler] */

	pfmul %mm6 , %mm7	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler] */
	mov tsuDataList+4(gc,dataElem) , %eax	/*  next offset from offset list */

	pfadd %mm5 , %mm7	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable[iscaler] */
	mov tsuDatLstScal+4(gc,dataElem) , %edx	/*  next iscaler */

	cmp $0 , %eax	/*  offset == 0 (end of offset list) ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_end_WB1_ND	/*  yup, vertex done, 1 DWORD in "write buffer" */

	movd (vertex,%eax) , %mm4	/*  TRI_SETF(FARRAY(vPtr, i) */
	movd 4(%edx) , %mm6	/*  atable[iscaler] */

	movd 4(%edx) , %mm5	/*  btable[iscaler] */
	add $8 , dataElem	/*  dataElem += 2 */

	add $8 , fifo	/*  fifoPtr += 2*sizeof*FxFloat) */
	pfmul %mm6 , %mm4	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler] */

	nop 	/*  filler */
	mov tsuDataList(gc,dataElem) , %eax	/*  get next offset from offset list */

	pfadd %mm5 , %mm4	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable[iscaler] */
	test %eax , %eax	/*  offset == 0 (end of offset list) ?  */

	punpckldq %mm4 , %mm7	/*  current component | previous component */
	nop 	/*  filler */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jnz $inner_loop_WB0_ND	/*  nope, vertex not done yet "write buffer" empty */

.L__grDrawTriangles_3DNow_$clip_setup_end_WB0_ND:

	mov (strideinbytes) , %eax	/*  offset to next vertex */
	mov %esp , %esp	/*  filler */

	mov wInfo_offset(gc) , %edx	/*  gc->state.vData.wInfo.offset */
	sub $1 , vertexCount	/*  one less vertex to process, any left? */

	lea (vertex,%eax) , vertex	/*  points to next vertex */
	jnz $clip_for_begin_WB0_ND	/*  yup, vertices left, push out next vertex */

.L__grDrawTriangles_3DNow_$clip_setup_done_ND:
	mov fifoPtr(gc) , %eax	/*  old fifoPtr */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	sub $15 , vertexCount	/*  remaining number of vertices to process */
	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */

	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */
	cmp $0 , vertexCount	/*  any vertices left to process ? */

	nop 	/*  filler */
	jg .L__grDrawTriangles_3DNow_$clip_coords_begin_ND	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 5 DWORD parameters off stack */


.align 32

	nop 	/*  filler for code alignment */

.L__grDrawTriangles_3DNow_$clip_coordinates_D:

	pxor %mm0 , %mm0	/*  load 0 */
	movd (_GlideRoot+pool_f255) , %mm1	/*  GlideRoot.pool.f255  */

	movd %mm0 , (atab)	/*  atable[0] = 0.0f */
	movd vp_oz(gc) , %mm5	/*  gc->state.Viewport.oz */

	movq %mm0 , (btab+8)	/*  btable[3] = 0.0f | btable[2] = 0.0f */
	movq vp_hwidth(gc) , %mm2	/*  gc->state.Viewport.hheight | gc->state.Viewport.hwidth */

	movd %mm1 , (atab+8)	/*  atable[2] = GlideRoot.pool.f255 */
	movq vp_ox(gc) , %mm1	/*  gc->state.Viewport.oy | gc->state.Viewport.ox */

	movd %mm5 , (btab+4)	/*  btable[1] = gc->state.Viewport.oz */
	nop 	/*  filler */

	movq %mm0 , (btab+16)	/*  btable[5] = 0.0f | btable[4] = 0.0f */
	movq %mm0 , (btab+24)	/*  btable[7] = 0.0f | btable[6] = 0.0f */

.L__grDrawTriangles_3DNow_$clip_coords_begin_D:

	sub $15 , vertexCount	/*  vertexCount >= 15 ? CF=0 : CF=1 */
	mov vertexSize(gc) , %ecx	/*  bytes of data for each vertex  */

	sbb %eax , %eax	/*  vertexCount >= 15 ? 00000000:ffffffff */

	and %eax , vertexCount	/*  vertexCount >= 15 ? 0 : vertexcount-15 */
	add $15 , vertexCount	/*  vertexcount >= 15 ? 15: vertexcount */

	imul vertexCount , %ecx	/*  total amount of vertex data we'll send */

	mov fifoRoom(gc) , %eax	/*  fifo space available */
	add $4 , %ecx	/*  add header size ==> total packet size */

	cmp %ecx , %eax	/*  fifo space avail >= packet size ? */
	jge .L__grDrawTriangles_3DNow_$clip_tri_begin_D	/*  yup, start writing triangle data */

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %ecx	/*  fifo space needed */
	call _FifoMakeRoom	/*  note: updates fifoPtr */

	add $12 , %esp	/*  remove 3 DWORD arguments from stack */
	nop 	/*  filler */

.L__grDrawTriangles_3DNow_$clip_tri_begin_D:

	mov vertexCount , %edx	/*  number of vertices in triangles */
	mov fifoPtr(gc) , fifo	/*  get fifoPtr */

	mov cullStripHdr(gc) , %ebp	/*  <2:0> = type */
	shl $6 , %edx	/*  <9:6> = vertex count (max 15) */

	or %ebp , %edx	/*  setup vertex count and type */
	mov $4 , %ebp	/*  test bit 2 */

	test %ebp , fifo	/*  fifoPtr QWORD aligned ? */

	mov wInfo_offset(gc) , %eax	/*  gc->state.vData.wInfo.offset */
	jz .L__grDrawTriangles_3DNow_$clip_fifo_aligned_D	/*  yup */

	mov %edx , (fifo)	/*  PCI write packet type */
	add $4 , fifo	/*  fifo pointer now QWORD aligned */

	$clip_for_begin_WB0_D :	/*  "write buffer" = MM7 is empty */

/*  here:  eax = gc->state.vData.wInfo.offset */
/*         ebx = vertexPtr */
/*         ecx = fifo */
/*         eax = gc->state.vData.wInfo.offset */
/*         esi = vertexCount */
/*         edi = gc */
/*         mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox */
/*         mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth */
/*  avail: edx, ebp, mm3, mm4, mm5, mm6, mm7 */

	mov (vertexPtr) , %edx	/*  vertex = *vertexPtr */
	add $4 , vertexPtr	/*  next vertexPtr */

	mov $0 , dataElem	/*  dataElem = 0 */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movd (%edx,%eax) , %mm0	/*  0 | W of current vertex */
	movq (%edx) , %mm7	/*  y | x of current vertex */

	pfrcp %mm0 , %mm4	/*  1/W approx | 1/W approx */
	mov colorType(gc) , %eax	/*  gc->state.vData.colorType */

	punpckldq %mm0 , %mm0	/*  W | W */
	movd vp_hdepth(gc) , %mm6	/*  gc->state.Viewport.hdepth */

	cmp $1 , %eax	/*  gc->state.vData.colorType == GR_U8  */
	pfrcpit1 %mm4 , %mm0	/*  1/W refine | 1/W refine */

	pfmul %mm2 , %mm7	/*  TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth */
	mov tsuDataList(gc) , %eax	/*  get offset of first component from offset list */

	pfrcpit2 %mm4 , %mm0	/*  oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset */
	mov vertexCount , (vertices)	/*  save off vertex count  */

	movq tmu0_s_scale(gc) , %mm3	/*  gc->state.tmu_config[0].t_scale | gc->state.tmu_config[0].s_scale */
	movd %mm0 , (atab+28)	/*  atable[7] = oow */

	pfmul %mm0 , %mm7	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth */
	movd %mm0 , (btab)	/*  btable[0] = oow */

	pfmul %mm0 , %mm6	/*  oow*gc->state.Viewport.hdepth */
	movq tmu1_s_scale(gc) , %mm4	/*  gc->state.tmu_config[1].t_scale | gc->state.tmu_config[1].s_scale */

	pfmul %mm0 , %mm3	/*  oow * gc->state.tmu_config[0].t_scale | oow * gc->state.tmu_config[0].s_scale */
	pfadd %mm1 , %mm7	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) | */

	movd %mm6 , (atab+4)	/*  atable[1] = oow*gc->state.Viewport.hdepth */
	pfmul %mm0 , %mm4	/*  oow * gc->state.tmu_config[1].t_scale | oow * gc->state.tmu_config[1].s_scale */

	movq %mm3 , (atab+12)	/*  atable[4] = oow * gc->state.tmu_config[0].t_scale | atable[3] = oow * gc->state.tmu_config[0].s_scale  */
	movq %mm7 , -8(fifo)	/*  PCI write transformed x, y write buffer=mm7 empty */

	movq %mm4 , (atab+20)	/*  atable[6] = oow * gc->state.tmu_config[1].t_scale | atable[5] = oow * gc->state.tmu_config[1].s_scale */
	jne .L__grDrawTriangles_3DNow_$not_pargb_WB0_D	/*  nope, gc->state.vData.colorType != GR_U8 */

	movd (%edx,%eax) , %mm7	/*  get packed ARGB data 1 DWORD in "write buffer"=mm7 */
	nop 	/*  filler */

	mov tsuDataList+4(gc) , %eax	/*  get offset of next vertex component (after pargb) */
	add $4 , dataElem	/*  dataElem = 1 (namely pargb) */

.L__grDrawTriangles_3DNow_$not_pargb_WB1_D:

	test %eax , %eax	/*  end of offset list ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_end_WB1_D	/*  yup */

/*  here:  eax = offset into vertex */
/*         ebx = vertexPtr */
/*         ecx = fifo */
/*         edx = vertex */
/*         edi = gc */
/*         ebp = dataElem */
/*         mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox */
/*         mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth */
/*         mm7 = write buffer */
/*  avail: eax, esi, mm0, mm3, mm4, mm5, mm6 */

.L__grDrawTriangles_3DNow_$inner_loop_WB1_D:
	mov tsuDatLstScal(gc,dataElem) , %esi	/*  iscaler */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movd (%edx,%eax) , %mm4	/*  TRI_SETF(FARRAY(vPtr, i) */
	movd 4(%esi) , %mm6	/*  atable [iscaler] */

	movd 4(%esi) , %mm5	/*  btable [iscaler] */
	pfmul %mm6 , %mm4	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler] */

	mov tsuDataList+4(gc,dataElem) , %eax	/*  next offset from offset list */
	pfadd %mm5 , %mm4	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable [iscaler] */

	cmp $0 , %eax	/*  offset == 0 (end of offset list) ? */
	punpckldq %mm4 , %mm7	/*  current component | previous component */

	mov tsuDatLstScal+4(gc,dataElem) , %esi	/*  next iscaler */
	movq %mm7 , -8(fifo)	/*  PCI write  new param | previous param */

	jz .L__grDrawTriangles_3DNow_$clip_setup_end_WB0_D	/*  yup, end of offset list, this vertex done */
	movd (%edx,%eax) , %mm7	/*  TRI_SETF(FARRAY(vPtr, i) */

	add $8 , dataElem	/*  dataElem += 2 */
	movd 4(%esi) , %mm6	/*  atable [iscaler] */

	movd 4(%esi) , %mm5	/*  btable [iscaler] */
	pfmul %mm6 , %mm7	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler] */

	mov tsuDataList(gc,dataElem) , %eax	/*  next offset from offset list */
	pfadd %mm5 , %mm7	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable [iscaler] */

	nop 	/*  filler */
	test %eax , %eax	/*  offset == 0 (end of offset list) ? */

	jnz $inner_loop_WB1_D	/*  nope, get next component (1 DWORD in "write buffer") */
	nop 	/*  filler */

.L__grDrawTriangles_3DNow_$clip_setup_end_WB1_D:
	mov (vertices) , vertexCount	/*  get back number of vertices left to process */
	mov wInfo_offset(gc) , %eax	/*  gc->state.vData.wInfo.offset */

	dec vertexCount	/*  one less vertex to handle */
	jnz $clip_for_begin_WB1_D	/*  until all vertices done 1 DWORD in "write buffer" = MM7 */

	movd %mm7 , (fifo)	/*  flush "write buffer" */
	add $4 , fifo	/*  fifoPtr += sizeof(FxFloat) */

	jmp .L__grDrawTriangles_3DNow_$clip_setup_done_D	/*  all vertices handled, triangles done */
	nop 	/*  filler */

.L__grDrawTriangles_3DNow_$clip_fifo_aligned_D:

	movd %edx , %mm7	/*  write buffer has 1 DWORD now */

.L__grDrawTriangles_3DNow_$clip_for_begin_WB1_D:

/*  here:  eax = gc->state.vData.wInfo.offset */
/*         ebx = vertexPtr */
/*         ecx = fifo */
/*         eax = gc->state.vData.wInfo.offset */
/*         esi = vertexCount */
/*         edi = gc */
/*         mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox */
/*         mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth */
/*  avail: edx, ebp, mm0, mm3, mm4, mm5, mm6, mm7 */

	mov (vertexPtr) , %edx	/*  vertex = *vertexPtr */
	add $4 , vertexPtr	/*  next vertexPtr */

	mov $0 , dataElem	/*  dataElem = 0 */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

	movd (%edx,%eax) , %mm0	/*  0 | W of current vertex */
	movq (%edx) , %mm5	/*  y | x of current vertex */

	pfrcp %mm0 , %mm4	/*  1/W approx | 1/W approx */
	mov colorType(gc) , %eax	/*  gc->state.vData.colorType */

	punpckldq %mm0 , %mm0	/*  W | W */
	movd vp_hdepth(gc) , %mm6	/*  gc->state.Viewport.hdepth */

	pfrcpit1 %mm4 , %mm0	/*  1/W refine | 1/W refine */
	cmp $1 , %eax	/*  gc->state.vData.colorType == GR_U8  */

	pfmul %mm2 , %mm5	/*  TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth */
	mov tsuDataList(gc) , %eax	/*  get offset of first component from offset list */

	pfrcpit2 %mm4 , %mm0	/*  oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset */
	mov vertexCount , (vertices)	/*  save off vertex count  */

	movq tmu0_s_scale(gc) , %mm3	/*  gc->state.tmu_config[0].t_scale | gc->state.tmu_config[0].s_scale  */
	pfmul %mm0 , %mm5	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth */

	movd %mm0 , (atab+28)	/*  atable[7] = oow */
	pfmul %mm0 , %mm6	/*  oow*gc->state.Viewport.hdepth */

	movd %mm0 , (btab)	/*  btable[0] = oow */
	pfmul %mm0 , %mm3	/*  oow * gc->state.tmu_config[1].t_scale | oow * gc->state.tmu_config[1].s_scale */

	pfadd %mm1 , %mm5	/*  TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) | */
	movd %mm6 , (atab+4)	/*  atable[1] = oow*gc->state.Viewport.hdepth */

	movq %mm3 , (atab+12)	/*  atable[4] = oow * gc->state.tmu_config[0].t_scale | atable[3] = oow * gc->state.tmu_config[0].s_scale */
	movq tmu1_s_scale(gc) , %mm3	/*  gc->state.tmu_config[1].t_scale | gc->state.tmu_config[1].s_scale */

	punpckldq %mm5 , %mm7	/*  header | transformed x */
	punpckhdq %mm5 , %mm5	/*  transformed y | transformed y */

	pfmul %mm0 , %mm3	/*  oow * gc->state.tmu_config[1].t_scale | oow * gc->state.tmu_config[1].s_scale */
	movq %mm7 , -8(fifo)	/*  PCI write header | transformed x */

	movq %mm5 , %mm7	/*  1 DWORD in "write buffer" (transformed y) */
	nop 	/*  filler */

	movq %mm3 , (atab+20)	/*  atable[6] = oow * gc->state.tmu_config[1].t_scale | atable[5] = oow * gc->state.tmu_config[1].s_scale */
	jne .L__grDrawTriangles_3DNow_$not_pargb_WB1_D	/*  nope, gc->state.vData.colorType != GR_U8 */

	movd (%edx,%eax) , %mm6	/*  get packed ARGB data */
	mov $4 , dataElem	/*  dataElem = 1 (namely pargb) */

	punpckldq %mm6 , %mm7	/*  mm7 = pargb | transformed y */
	mov tsuDataList+4(gc) , %eax	/*  get offset of next vertex component */

	movq %mm7 , (fifo)	/*  PCI write pargb | transformed y */
	add $8 , fifo	/*  fifoPtr += 2*sizeof(FxFloat) */

.L__grDrawTriangles_3DNow_$not_pargb_WB0_D:
	test %eax , %eax	/*  end of data offset list ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_end_WB0_D	/*  yup, this vertex done */

/*  here:  eax = offset into vertex */
/*         ebx = vertexPtr */
/*         ecx = fifo */
/*         edx = vertex */
/*         edi = gc */
/*         ebp = dataElem */
/*         mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox */
/*         mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth */
/*  avail: eax, esi, mm0, mm3, mm4, mm5, mm6, mm7 */

.L__grDrawTriangles_3DNow_$inner_loop_WB0_D:
	mov tsuDatLstScal(gc,dataElem) , %esi	/*  iscaler */
	movd (%edx,%eax) , %mm7	/*  TRI_SETF(FARRAY(vPtr, i) */

	movd 4(%esi) , %mm6	/*  atable[iscaler] */
	movd 4(%esi) , %mm5	/*  btable[iscaler] */

	pfmul %mm6 , %mm7	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler] */
	mov tsuDataList+4(gc,dataElem) , %eax

	pfadd %mm5 , %mm7	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable[iscaler] */
	mov tsuDatLstScal+4(gc,dataElem) , %esi	/*  next iscaler */

	test %eax , %eax	/*  offset == 0 (end of offset list) ? */
	jz .L__grDrawTriangles_3DNow_$clip_setup_end_WB1_D	/*  yup, vertex done, 1 DWORD in "write buffer" */

	movd (%edx,%eax) , %mm4	/*  TRI_SETF(FARRAY(vPtr, i) */
	movd 4(%esi) , %mm6	/*  atable[iscaler] */

	movd 4(%esi) , %mm5	/*  btable[iscaler] */
	add $8 , dataElem	/*  dataElem += 2 */

	add $8 , fifo	/*  fifoPtr += 2*sizeof*FxFloat) */
	pfmul %mm6 , %mm4	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler] */

	mov tsuDataList(gc,dataElem) , %eax	/*  get next offset from offset list */
	pfadd %mm5 , %mm4	/*  TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable[iscaler] */

	cmp $0 , %eax	/*  offset == 0 (end of offset list) ?  */
	punpckldq %mm4 , %mm7	/*  current component | previous component */

	movq %mm7 , -8(fifo)	/*  PCI write current component | previous component */
	jnz $inner_loop_WB0_D	/*  nope, vertex not done yet "write buffer" empty */

.L__grDrawTriangles_3DNow_$clip_setup_end_WB0_D:
	mov (vertices) , vertexCount	/*  get back number of vertices left to process */
	mov wInfo_offset(gc) , %eax	/*  gc->state.vData.wInfo.offset */

	dec vertexCount	/*  one less vertex to process, any left? */
	jnz $clip_for_begin_WB0_D	/*  yup, vertices left, push out next vertex */

.L__grDrawTriangles_3DNow_$clip_setup_done_D:
	mov fifoPtr(gc) , %eax	/*  old fifoPtr */
	mov fifoRoom(gc) , %ebp	/*  old number of bytes available in fifo */

	mov _count(%esp) , vertexCount	/*  remaining vertices before previous loop */
	sub fifo , %eax	/*  old fifoPtr - new fifoPtr (fifo room used) */

	mov fifo , fifoPtr(gc)	/*  save current fifoPtr  */
	add %eax , %ebp	/*  new number of bytes available in fifo */

	sub $15 , vertexCount	/*  remaining number of vertices to process */
	nop 	/*  filler */

	mov %ebp , fifoRoom(gc)	/*  save current number of bytes available in fifo */
	mov vertexCount , _count(%esp)	/*  remaining number of vertices to process  */

	cmp $0 , vertexCount	/*  any vertices left to process */
	jg .L__grDrawTriangles_3DNow_$clip_coords_begin_D	/*  loop if number of vertices to process >= 0 */

	femms 	/*  no more MMX code clear MMX/FPU state */

#endif	/*  GLIDE3_SCALER */

.L__grDrawTriangles_3DNow_$tris_done:
	pop %ebp	/*  restore frame pointer */
	pop %ebx	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	ret	/*  return, pop 3 DWORD parameters */
.L_END___grDrawTriangles_3DNow:
.size __grDrawTriangles_3DNow,.L_END___grDrawTriangles_3DNow-__grDrawTriangles_3DNow


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
_F1:	.int	0x03f800000	/*  1 */
	.type	_F256,@object
	.size	_F256,4
_F256:	.int	0x043800000	/*  256 */

	.type	_VPF1,@object
	.size	_VPF1,4
_VPF1:	.int	0x03f800000	/*  1 */
	.type	_VPF256,@object
	.size	_VPF256,4
_VPF256:	.int	0x043800000	/*  256     */

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


	.type	oow0,@object
	.size	oow0,4
oow0:	.int	0
	.type	oow1,@object
	.size	oow1,4
oow1:	.int	0
	.type	oow2,@object
	.size	oow2,4
oow2:	.int	0
	.type	oow3,@object
	.size	oow3,4
oow3:	.int	0
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
	.type	xa,@object
	.size	xa,4
xa:	.int	0
	.type	ya,@object
	.size	ya,4
ya:	.int	0
	.type	xb,@object
	.size	xb,4
xb:	.int	0
	.type	yb,@object
	.size	yb,4
yb:	.int	0
	.type	xc,@object
	.size	xc,4
xc:	.int	0
	.type	yc,@object
	.size	yc,4
yc:	.int	0
	.type	zArea,@object
	.size	zArea,4
zArea:	.int	0

.text

#define _pktype 20
#define _type 24
#define _mode 28
#define _count 32
#define _pointers 36

#define gc %esi	/*  points to graphics context */
#define fifo %ecx	/*  points to next entry in fifo */
#define dlp %ebp	/*  points to dataList structure */
#define vertexCount %ebx	/*  Current vertex counter in the packet */
#define vertexPtr %edi	/*  Current vertex pointer */

.globl _drawvertexlist	
.type _drawvertexlist,@function
_drawvertexlist:
/*  132  : { */

	push %esi
	push %edi
	push %ebx
	push %ebp

/*      GR_DCL_GC */
/*      vSize = gc->state.vData.vSize */
/*      if (stride == 0) */
/*        stride = gc->state.vData.vStride */

	movl _GlideRoot+curGC , gc
	movl vertexSize(gc) , %ecx

	movl _mode(%esp) , %edx
	movl _count(%esp) , vertexCount

	movl _pointers(%esp) , vertexPtr
	movl %ecx , vSize

	shl $2 , %edx
/*      mov     ecx, DWORD PTR [gc+CoordinateSpace] */
	test %edx , %edx
	jne .L_drawvertexlist_no_stride
	movl vertexStride(gc) , %edx
	shl $2 , %edx

.align 4
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

.align 4
.L_drawvertexlist_window_coords_begin:

	cmp $15 , vertexCount	/*  0000000fH */
	jl .L_drawvertexlist_win_partial_packet
	mov $15 , vertexCount	/*  0000000fH */

.align 4
.L_drawvertexlist_win_partial_packet:

	movl vSize , %eax
	movl fifoRoom(gc) , %ecx
	imul vertexCount , %eax
	add $4 , %eax
	cmp %eax , %ecx
	jge .L_drawvertexlist_win_strip_begin
	push $__LINE__
	push $0x0
	push %eax
	call _FifoMakeRoom
	add $12 , %esp	/*  0000000cH */

.align 4
.L_drawvertexlist_win_strip_begin:

/*      Setup pacet header */

	movl fifoPtr(gc) , fifo
	mov vertexCount , %eax
	movl _type(%esp) , %edx
	movl cullStripHdr(gc) , %ebp
	shl $22 , %edx	/*  00000010H */
	add $4 , fifo
	shl $6 , %eax
	or %edx , %ebp
	or %ebp , %eax
	movl _pktype(%esp) , %edx
	or %edx , %eax
	nop 
	movl %eax , -4(fifo)

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

.align 4
.L_drawvertexlist_win_for_begin:

	mov vertexPtr , %edx
	movl strideinbytes , %eax
	cmp $4 , %eax
	jne .L_drawvertexlist_win_no_deref
	movl (vertexPtr) , %edx

.align 4
.L_drawvertexlist_win_no_deref:

	add $8 , fifo
	add %eax , vertexPtr

	movl (%edx) , %eax
	movl 4(%edx) , %ebp

	movl %eax , -8(fifo)
	movl tsuDataList(gc) , %eax

	movl %ebp , -4(fifo)

	test %eax , %eax
	leal tsuDataList(gc) , dlp
	je .L_drawvertexlist_win_datalist_end

.align 4

/*        while (i != GR_DLIST_END) { */
/*          TRI_SETF(FARRAY(vPtr, i)) */
/*          dataElem++ */
/*          i = gc->tsuDataList[dataElem] */
/*        } */

.L_drawvertexlist_win_datalist_begin:

	add $4 , fifo
	add $4 , dlp

	movl (%edx,%eax) , %eax
	nop 

	movl %eax , -4(fifo)
	movl (dlp) , %eax


	test %eax , %eax
	jne .L_drawvertexlist_win_datalist_begin
.L_drawvertexlist_win_datalist_end:

	dec vertexCount
	jne .L_drawvertexlist_win_for_begin
.L_drawvertexlist_win_for_end:

/*        TRI_END */
/*      Prepare for the next packet (if the strip size is longer than 15) */
/*        GR_CHECK_SIZE() */
/*        count -= 15 */
/*        pktype = SSTCP_PKT3_DDDDDD */
/*      } */

	movl fifoPtr(gc) , %eax
	movl fifoRoom(gc) , %edx
	sub fifo , %eax
	movl _count(%esp) , vertexCount
	add %eax , %edx
	sub $15 , vertexCount	/*  0000000fH */

	movl %edx , fifoRoom(gc)
	movl vertexCount , _count(%esp)

	movl fifo , fifoPtr(gc)
	test vertexCount , vertexCount

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

#define gc %esi	/*  points to graphics context */
#define fifo %ecx	/*  points to next entry in fifo */
#define vertexPtr %edx	/*  pointer to vertex or vertex array */

.globl _vpdrawvertexlist	
.type _vpdrawvertexlist,@function
_vpdrawvertexlist:

	push %esi
	push %edi

	push %ebx
	movl _GlideRoot+curGC , gc

	push %ebp
	movl _mode(%esp) , %ecx

	movl _pointers(%esp) , %edi
	movl wInfo_offset(gc) , %eax

	test %ecx , %ecx
	je .L_vpdrawvertexlist_w_no_dref

	movl (%edi) , %edi

.align 4
.L_vpdrawvertexlist_w_no_dref:

/*      load first w */

	flds (%edi,%eax)
	fdivrs _F1

	movl vertexSize(gc) , %ecx
	movl _mode(%esp) , %edx

	movl _count(%esp) , %edi
/*      mov     vertexArray, DWORD PTR [esp+_pointers] */

	shl $2 , %edx
	movl %ecx , vSize

	test %edx , %edx

	jne .L_vpdrawvertexlist_no_stride

	movl vertexStride(gc) , %edx
	shl $2 , %edx

.align 4
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
	movl fifoRoom(gc) , %ecx

	imul %edi , %eax
	add $4 , %eax
	cmp %eax , %ecx
	jge .L_vpdrawvertexlist_clip_strip_begin
	push $__LINE__
	push $0x0
	push %eax
	call _FifoMakeRoom
	add $12 , %esp	/*  0000000cH */
.L_vpdrawvertexlist_clip_strip_begin:

/*      TRI_STRIP_BEGIN(type, vcount, vSize, pktype) */


	movl fifoPtr(gc) , fifo
	mov %edi , %eax

	movl packetVal , %edx
	movl cullStripHdr(gc) , %ebp

	or %edx , %eax
	add $4 , fifo

	shl $6 , %eax
	movl _pktype(%esp) , %edx

	or %ebp , %eax

	or %edx , %eax
	movl %eax , -4(fifo)


	movl _pointers(%esp) , vertexPtr
	movl _mode(%esp) , %eax

	test %eax , %eax

	je .L_vpdrawvertexlist_clip_for_begin
	movl (vertexPtr) , vertexPtr

.align 4
.L_vpdrawvertexlist_clip_for_begin:

	add $8 , fifo
	movl strideinbytes , %ebp

	addl %ebp , _pointers(%esp)
	movl paramIndex(gc) , %eax

	xor %ebp , %ebp
	movl tsuDataList(gc) , %ebx

/*     setup x and y */

	flds vp_hwidth(gc)
	fmuls (vertexPtr)
	test $3 , %al
	flds vp_hheight(gc)
	fmuls 4(vertexPtr)
	fxch 
	fmul %st(2) , %st
	fxch 
	fmul %st(2) , %st
	fxch 
	fadds vp_ox(gc)
	fxch 
	fadds vp_oy(gc)
	fxch %st(2)
	fstps ccoow
	fstps -8(fifo)
	fstps -4(fifo)

/*     set up color */

	je .L_vpdrawvertexlist_clip_setup_ooz

	cmpl %ebp , colorType(gc)
	jne .L_vpdrawvertexlist_clip_setup_pargb

	test $1 , %al
	je .L_vpdrawvertexlist_clip_setup_a

	add $12 , fifo
	mov $3 , %ebp

	flds _GlideRoot+pool_f255
	fmuls (%ebx,vertexPtr)
	flds _GlideRoot+pool_f255
	fmuls 4(%ebx,vertexPtr)
	flds _GlideRoot+pool_f255
	fmuls 8(%ebx,vertexPtr)
	fxch %st(2)
	fstps -12(fifo)
	fstps -8(fifo)
	fstps -4(fifo)
	movl tsuDataList+12(gc) , %ebx

.align 4
.L_vpdrawvertexlist_clip_setup_a:

	test $2 , %al
	je .L_vpdrawvertexlist_clip_setup_ooz

	add $4 , fifo
	inc %ebp

	flds (%ebx,vertexPtr)
	fmuls _GlideRoot+pool_f255
	fstps -4(fifo)

	movl tsuDataList(gc,%ebp,4) , %ebx
	jmp .L_vpdrawvertexlist_clip_setup_ooz

.align 4
.L_vpdrawvertexlist_clip_setup_pargb:
	add $4 , fifo
	movl (%ebx,vertexPtr) , %ebx

	movl %ebx , -4(fifo)
	nop 

	mov $1 , %ebp
	movl tsuDataList+4(gc) , %ebx
.L_vpdrawvertexlist_clip_setup_ooz:

	test $4 , %al
	je .L_vpdrawvertexlist_clip_setup_qow

	add $4 , fifo
	inc %ebp

	flds (%ebx,vertexPtr)
	fmuls vp_hdepth(gc)
	fmuls ccoow
	fadds vp_oz(gc)
	fstps -4(fifo)

	movl tsuDataList(gc,%ebp,4) , %ebx
.align 4
.L_vpdrawvertexlist_clip_setup_qow:

	test $8 , %al
	je .L_vpdrawvertexlist_clip_setup_qow0

	movl qInfo_mode(gc) , %ebx
	test %ebx , %ebx
	je .L_vpdrawvertexlist_clip_setup_oow
	movl qInfo_offset(gc) , %ebx

	flds (vertexPtr,%ebx)
	fmuls ccoow
	fstps (fifo)

	jmp .L_vpdrawvertexlist_clip_setup_oow_inc
.align 4
.L_vpdrawvertexlist_clip_setup_oow:
	movl ccoow , %ebx

	movl %ebx , (fifo)
.align 4
.L_vpdrawvertexlist_clip_setup_oow_inc:

	movl tsuDataList+4(gc,%ebp,4) , %ebx
	add $4 , fifo

	inc %ebp
.align 4
.L_vpdrawvertexlist_clip_setup_qow0:

	test $16 , %al
	je .L_vpdrawvertexlist_clip_setup_stow0

	movl q0Info_mode(gc) , %ebx
	cmp $1 , %ebx
	jne .L_vpdrawvertexlist_clip_setup_oow0

	movl q0Info_offset(gc) , %ebx

	flds (%ebx,vertexPtr)
	fmuls ccoow
	fstps (fifo)

	jmp .L_vpdrawvertexlist_clip_setup_oow0_inc
.align 4
.L_vpdrawvertexlist_clip_setup_oow0:
	movl ccoow , %ebx

	movl %ebx , (fifo)
.align 4
.L_vpdrawvertexlist_clip_setup_oow0_inc:
	movl tsuDataList+4(gc,%ebp,4) , %ebx
	add $4 , fifo

	inc %ebp
.align 4
.L_vpdrawvertexlist_clip_setup_stow0:

	test $32 , %al
	je .L_vpdrawvertexlist_clip_setup_qow1


	flds ccoow
	fmuls (%ebx,vertexPtr)

	add $8 , fifo
	add $2 , %ebp

	fmuls tmu0_s_scale(gc)
	flds ccoow
	fmuls 4(%ebx,vertexPtr)
	movl tsuDataList(gc,%ebp,4) , %ebx
	fmuls tmu0_t_scale(gc)
	fxch 
	fstps -8(fifo)
	fstps -4(fifo)

.align 4
.L_vpdrawvertexlist_clip_setup_qow1:

	test $64 , %al
	je .L_vpdrawvertexlist_clip_setup_stow1

	movl q1Info_mode(gc) , %ebx
	cmp $1 , %ebx
	jne .L_vpdrawvertexlist_clip_setup_oow1

	movl q1Info_offset(gc) , %ebx

	flds (%ebx,vertexPtr)
	fmuls ccoow
	fstps (fifo)

	jmp .L_vpdrawvertexlist_clip_setup_oow1_inc
.align 4
.L_vpdrawvertexlist_clip_setup_oow1:
	movl ccoow , %ebx

	movl %ebx , (fifo)
.align 4
.L_vpdrawvertexlist_clip_setup_oow1_inc:

	movl tsuDataList+4(gc,%ebp,4) , %ebx
	add $4 , fifo

	inc %ebp

.align 4
.L_vpdrawvertexlist_clip_setup_stow1:

	test $128 , %al
	je .L_vpdrawvertexlist_clip_setup_end

	flds ccoow
	fmuls (%ebx,vertexPtr)
	add $8 , fifo
	fmuls tmu1_s_scale(gc)
	flds ccoow
	fmuls 4(%ebx,vertexPtr)
	movl tsuDataList+4(gc,%ebp,4) , %ebx
	fmuls tmu1_t_scale(gc)
	fxch 
	fstps -8(fifo)
	fstps -4(fifo)

.align 4
.L_vpdrawvertexlist_clip_setup_end:

	dec %edi
	jz .L_vpdrawvertexlist_clip_for_end

	movl _pointers(%esp) , vertexPtr
	movl _mode(%esp) , %ebx

	test %ebx , %ebx
	je .L_vpdrawvertexlist_w_clip_no_deref


	movl (vertexPtr) , vertexPtr
.align 4
.L_vpdrawvertexlist_w_clip_no_deref:

	movl wInfo_offset(gc) , %ebx

	flds (%ebx,vertexPtr)
	fdivrs _F1

	jmp .L_vpdrawvertexlist_clip_for_begin
.align 4
.L_vpdrawvertexlist_clip_for_end:

	movl fifoPtr(gc) , %ebx
	movl fifoRoom(gc) , %edx

	sub fifo , %ebx
	movl _count(%esp) , %edi

	add %ebx , %edx
	sub $15 , %edi	/*  0000000fH */

	movl %edx , fifoRoom(gc)
	movl %edi , _count(%esp)

	movl fifo , fifoPtr(gc)
	movl $16 , _pktype(%esp)	/*  00000010H */

	jle .L_vpdrawvertexlist_strip_done
	movl _pointers(%esp) , %edx

	movl _mode(%esp) , %ebx
	test %ebx , %ebx

	je .L_vpdrawvertexlist_w1_clip_no_deref
	movl (%edx) , %edx

.align 4
.L_vpdrawvertexlist_w1_clip_no_deref:

	movl wInfo_offset(gc) , %ebx
	flds (%ebx,%edx)
	fdivrs _F1

	jmp .L_vpdrawvertexlist_clip_coords_begin
.align 4
.L_vpdrawvertexlist_strip_done:

	pop %ebp
	pop %ebx
	pop %edi
	pop %esi
	ret	/*  00000014H */
.L_END__vpdrawvertexlist:
.size _vpdrawvertexlist,.L_END__vpdrawvertexlist-_vpdrawvertexlist

#define gc %esi	/*  points to graphics context */
#define fifo %ecx	/*  points to next entry in fifo */
#define vertexPtr %edi	/*  Current vertex pointer */

.globl _vptrisetup_nocull
.type _vptrisetup_nocull,@function
_vptrisetup_nocull:
#define _va 20
#define _vb 24
#define _vc 28
	push %ebx
	push %esi

	push %edi
	movl _GlideRoot+curGC , gc

	movl _va-4(%esp) , %ecx
	movl wInfo_offset(gc) , %eax

	push %ebp
	nop 

/*     oow[0] = 1.0f / FARRAY(va, gc->state.vData.wInfo.offset) */

	flds (%eax,%ecx)  /* Fixed */

	fdivrs _F1 /* Fixed */

	movl _vb(%esp) , %ecx
	movl _vc(%esp) , %ebx

	nop 
	nop 

	movl (%eax,%ecx) , %ebp
	movl (%eax,%ebx) , %edi

	movl %ebp , vPtr1
	movl %edi , vPtr2

/*     GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize, 1) */

	movl _GlideRoot+curTriSize , %eax
	movl fifoRoom(gc) , %ecx

	add $4 , %eax
	nop 

	cmp %eax , %ecx
	jge .L_cptrisetup_nocull_setup_pkt_hdr

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %eax
	call _FifoMakeRoom

	add $12 , %esp	/*  0000000cH */

.align 4
.L_cptrisetup_nocull_setup_pkt_hdr:

/*     TRI_STRIP_BEGIN(kSetupStrip, 3, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD) */


	movl fifoPtr(gc) , fifo
	movl cullStripHdr(gc) , %eax

	add $4 , fifo
	leal _va(%esp) , %ebp

	or $192 , %eax	/*  000000c0H */
	mov $0 , %edx

	movl %eax , -4(fifo)
	movl (%ebp) , vertexPtr

	movl paramIndex(gc) , %eax
	nop 

/*  Begin loop */

.align 4
.L_cptrisetup_nocull_begin_for_loop:

	add $4 , %edx
	add $8 , fifo

	xor %ebx , %ebx
	movl tsuDataList(gc) , %ebp

/*     setup x and y */

	flds vp_hwidth(gc)
	fmuls (vertexPtr)
	test $3 , %al
	flds vp_hheight(gc)
	fmuls 4(vertexPtr)
	fxch 
	fmul %st(2) , %st
	fxch 
	fmul %st(2) , %st
	fxch 
	fadds vp_ox(gc)
	fxch 
	fadds vp_oy(gc)
	fxch %st(2)
	fstps oowa
	fstps -8(fifo)
	fstps -4(fifo)


/*     set up color */

	je .L_cptrisetup_nocull_clip_setup_ooz

	cmpl %ebx , colorType(gc)
	jne .L_cptrisetup_nocull_clip_setup_pargb

	test $1 , %al
	je .L_cptrisetup_nocull_clip_setup_a

	add $12 , fifo
	add $3 , %ebx

	flds _GlideRoot+pool_f255
	fmuls (vertexPtr,%ebp)
	flds _GlideRoot+pool_f255
	fmuls 4(vertexPtr,%ebp)
	flds _GlideRoot+pool_f255
	fmuls 8(vertexPtr,%ebp)
	fxch %st(2)
	fstps -12(fifo)
	fstps -8(fifo)
	fstps -4(fifo)
	movl tsuDataList+12(gc) , %ebp

.align 4
.L_cptrisetup_nocull_clip_setup_a:

	test $2 , %al
	je .L_cptrisetup_nocull_clip_setup_ooz

	add $4 , fifo
	inc %ebx

	flds (vertexPtr,%ebp)
	fmuls _GlideRoot+pool_f255
	fstps -4(fifo)

	movl tsuDataList(gc,%ebx,4) , %ebp
	jmp .L_cptrisetup_nocull_clip_setup_ooz
.align 4
.L_cptrisetup_nocull_clip_setup_pargb:
	add $4 , fifo
	movl (vertexPtr,%ebp) , %ebx

	movl %ebx , -4(fifo)
	nop 

	mov $1 , %ebx
	movl tsuDataList+4(gc) , %ebp
.align 4
.L_cptrisetup_nocull_clip_setup_ooz:

	test $4 , %al
	je .L_cptrisetup_nocull_clip_setup_qow

	add $4 , fifo
	inc %ebx

	flds (vertexPtr,%ebp)
	fmuls vp_hdepth(gc)
	fmuls oowa
	fadds vp_oz(gc)
	fstps -4(fifo)

	movl tsuDataList(gc,%ebx,4) , %ebp
.align 4
.L_cptrisetup_nocull_clip_setup_qow:

	test $8 , %al
	je .L_cptrisetup_nocull_clip_setup_qow0

	cmpl $1 , qInfo_mode(gc)
	jne .L_cptrisetup_nocull_clip_setup_oow

	movl qInfo_offset(gc) , %ebp
	flds oowa
	fmuls (%ebp,vertexPtr)
	fstps (fifo)

	jmp .L_cptrisetup_nocull_clip_setup_oow_inc
.align 4
.L_cptrisetup_nocull_clip_setup_oow:

	movl oowa , %ebp

	movl %ebp , (fifo)
.align 4
.L_cptrisetup_nocull_clip_setup_oow_inc:
	movl tsuDataList+4(gc,%ebx,4) , %ebp
	add $4 , fifo

	inc %ebx
.align 4
.L_cptrisetup_nocull_clip_setup_qow0:

	test $16 , %al	/*  00000010H */
	je .L_cptrisetup_nocull_clip_setup_stow0

	cmpl $1 , q0Info_mode(gc)
	jne .L_cptrisetup_nocull_clip_setup_oow0

	movl q0Info_offset(gc) , %ebp

	flds oowa
	fmuls (%ebp,vertexPtr)
	fstps (fifo)

	jmp .L_cptrisetup_nocull_clip_setup_oow0_inc
.align 4
.L_cptrisetup_nocull_clip_setup_oow0:
	movl oowa , %ebp

	movl %ebp , (fifo)
.align 4
.L_cptrisetup_nocull_clip_setup_oow0_inc:
	movl tsuDataList+4(gc,%ebx,4) , %ebp
	add $4 , fifo

	inc %ebx
.align 4
.L_cptrisetup_nocull_clip_setup_stow0:

	test $32 , %al
	je .L_cptrisetup_nocull_clip_setup_qow1


	flds oowa
	fmuls (vertexPtr,%ebp)

	add $8 , fifo
	add $2 , %ebx

	fmuls tmu0_s_scale(gc)
	flds oowa
	fmuls 4(vertexPtr,%ebp)
	movl tsuDataList(gc,%ebx,4) , %ebp
	fmuls tmu0_t_scale(gc)
	fxch 
	fstps -8(fifo)
	fstps -4(fifo)

.align 4
.L_cptrisetup_nocull_clip_setup_qow1:

	test $64 , %al
	je .L_cptrisetup_nocull_clip_setup_stow1

	cmpl $1 , q1Info_mode(gc)
	jne .L_cptrisetup_nocull_clip_setup_oow1

	movl q1Info_offset(gc) , %ebp

	flds (%ebp,vertexPtr)
	fmuls oowa
	fstps (fifo)

	jmp .L_cptrisetup_nocull_clip_setup_oow1_inc
.align 4
.L_cptrisetup_nocull_clip_setup_oow1:
	movl oowa , %ebp

	movl %ebp , (fifo)
.align 4
.L_cptrisetup_nocull_clip_setup_oow1_inc:
	movl tsuDataList+4(gc,%ebx,4) , %ebp
	add $4 , fifo

	inc %ebx
.align 4
.L_cptrisetup_nocull_clip_setup_stow1:

	test $128 , %al
	je .L_cptrisetup_nocull_clip_setup_end


	flds oowa
	fmuls (vertexPtr,%ebp)
	add $8 , fifo
	fmuls tmu1_s_scale(gc)
	flds oowa
	fmuls 4(vertexPtr,%ebp)
	fmuls tmu1_t_scale(gc)
	fxch 
	fstps -8(fifo)
	fstps -4(fifo)

.align 4
.L_cptrisetup_nocull_clip_setup_end:

	cmp $12 , %edx
	je .L_cptrisetup_nocull_update_fifo_ptr

	flds vPtr0(%edx)
	fdivrs _F1

	leal _va(%esp) , %ebx
	movl wInfo_offset(gc) , %ebp

	movl (%ebx,%edx) , vertexPtr
	jmp .L_cptrisetup_nocull_begin_for_loop

.align 4
.L_cptrisetup_nocull_update_fifo_ptr:

	movl fifoPtr(gc) , %ebx
	movl fifoRoom(gc) , %edx

	sub fifo , %ebx
	mov $1 , %eax

	add %ebx , %edx
	pop %ebp

	movl %edx , fifoRoom(gc)
	pop %edi

	movl fifo , fifoPtr(gc)
	movl _GlideRoot+trisProcessed , %ebx

/*     _GlideRoot.stats.trisProcessed++ */


	pop %esi
	inc %ebx

	movl %ebx , _GlideRoot+trisProcessed
	pop %ebx

	ret	/*  0000000cH */

.L_END__vptrisetup_nocull:
.size _vptrisetup_nocull,.L_END__vptrisetup_nocull-_vptrisetup_nocull

.globl _vptrisetup_cull
.type _vptrisetup_cull,@function
_vptrisetup_cull:

#define _va 20
#define _vb 24
#define _vc 28
	push %ebx
	push %esi

	push %edi
	movl _GlideRoot+curGC , gc

	movl _va-4(%esp) , %ecx
	push %ebp

	movl wInfo_offset(gc) , %eax
	nop 

/*     oow[0] = 1.0f / FARRAY(va, gc->state.vData.wInfo.offset) */

	flds (%eax,%ecx)

	fdivrs _F1

	movl %ecx , vPtr0

	movl _vb(%esp) , %ecx
	movl _vc(%esp) , %ebx

	flds (%eax,%ecx)
	fdivrs _F1

	movl %ecx , vPtr1
	movl %ebx , vPtr2

	flds (%eax,%ebx)
	fdivrs _F1
	fxch %st(2)	/*   oow2  oow1  oow0 */

/*     GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize, 1) */
	movl _GlideRoot+curTriSize , %eax
	movl fifoRoom(gc) , %ecx

	add $4 , %eax
	nop 

	cmp %eax , %ecx
	jge .L_vptrisetup_cull_setup_pkt_hdr

	push $__LINE__	/*  line number inside this function */
	push $0x0	/*  pointer to function name = NULL */

	push %eax
	call _FifoMakeRoom

	add $12 , %esp	/*  0000000cH */

.align 4
.L_vptrisetup_cull_setup_pkt_hdr:

	mov cull_mode(gc) , %edx
	shl $31 , %edx	/*  culltest << 31     */

/*     culling */

	movl _va(%esp) , vertexPtr
	flds vp_hwidth(gc)	/*   oow2  oow1  oow0  tx */
	fmuls (vertexPtr)	/*   |     |     |     | */
	flds vp_hheight(gc)	/*   |     |     |     |     ty */
	fmuls 4(vertexPtr)	/*   |     |     |     |     | */
	fxch 	/*   |     |     |     ty    tx */
	fmul %st(2) , %st	/*   |     |     |     |     | */
	fxch 	/*   |     |     |     tx    ty */
	fmul %st(2) , %st	/*   |     |     |     |     | */
	fxch 	/*   |     |     |     ty    tx */
	fadds vp_ox(gc)	/*   |     |     |     |     xa */
	fxch 	/*   |     |     |     xa    ty */
	fadds vp_oy(gc)	/*   |     |     |     |     ya */
	fxch %st(2)	/*   |     |     ya    |     oow0 */
	fstps oow0	/*   |     |     |     | */
	fstps xa	/*   |     |     | */
	fstps ya	/*   |     | */

	movl _vb(%esp) , vertexPtr
	flds vp_hwidth(gc)	/*   oow2  oow1  tx */
	fmuls (vertexPtr)	/*   |     |     | */
	flds vp_hheight(gc)	/*   |     |     |     ty */
	fmuls 4(vertexPtr)	/*   |     |     |     | */
	fxch 	/*   |     |     ty    tx */
	fmul %st(2) , %st	/*   |     |     |     | */
	fxch 	/*   |     |     tx    ty */
	fmul %st(2) , %st	/*   |     |     |     | */
	fxch 	/*   |     |     ty    tx */
	fadds vp_ox(gc)	/*   |     |     |     xb */
	fxch 	/*   |     |     xb    ty */
	fadds vp_oy(gc)	/*   |     |     |     yb */
	fxch %st(2)	/*   |     yb    |     oow1 */
	fstps oow1	/*   |     |     | */
	fstps xb	/*   |     | */
	fstps yb	/*   | */

	movl _vc(%esp) , vertexPtr
	flds vp_hwidth(gc)	/*   |     tx */
	fmuls (vertexPtr)	/*   |     | */
	flds vp_hheight(gc)	/*   |     |     ty */
	fmuls 4(vertexPtr)	/*   |     |     | */
	fxch 	/*   |     ty    tx */
	fmul %st(2) , %st	/*   |     |     | */
	fxch 	/*   |     tx    ty */
	fmul %st(2) , %st	/*   |     |     | */
	fxch 	/*   |     ty    tx */
	fadds vp_ox(gc)	/*   |     |     xc */
	fxch 	/*   |     xc    ty */
	fadds vp_oy(gc)	/*   |     |     yc */
	fxch %st(2)	/*   yc    |     oow2 */
	fstps oow1	/*   |     | */
	fstps xc	/*   | */
	fstps yc

	flds xa	/*   xa */
	fsubs xb	/*   dxAB */
	flds yb	/*   |    yb */
	fsubs yc	/*   |    dyBC */
	flds xb	/*   |    |    xb */
	fsubs xc	/*   |    |    dxBC */
	fxch 	/*   |    dxBC dyBC */
	flds ya	/*   |    |    |    ya */
	fsubs yb	/*   |    |    |    dyAB */
	fxch %st(3)	/*   dyAB |    |    dxAB */
	fmulp  %st , %st(1)	/*   |    |    t0=dxAB*dyBC  */
	fxch %st(2)	/*   t0   |    dxAB */
	fmulp %st , %st(1)	/*   t0   t1 */
	fsubp %st , %st(1)	/*   area */
	fstps zArea

	movl zArea , %ecx	/*  j = *(long *)&area */
	xor %eax , %eax	/*  Clear the return value (0 == culled) */

/*  Zero Area Triangle Check */
	and $0x7fffffff , %ecx	/*  if ((j & 0x7FFFFFFF) == 0) */
	jz .L_vptrisetup_cull___triDone

/*  Triangle area check vs culling mode */
	movl zArea , %ecx	/*  reload area just in case we're culling */
	xor %edx , %ecx	/*  if (j ^ (culltest << 31)) */

	jge .L_vptrisetup_cull___triDone


/*     TRI_STRIP_BEGIN(kSetupStrip, 3, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD) */


	movl fifoPtr(gc) , fifo
	movl cullStripHdr(gc) , %eax

	add $4 , fifo
	mov $0 , %edx

	or $192 , %eax	/*  000000c0H */
	nop 

	movl %eax , -4(fifo)
	movl paramIndex(gc) , %eax


/*  Begin loop */

.align 4
.L_vptrisetup_cull_begin_for_loop:
	movl (%edx) , %ebp
	add $8 , fifo

	movl (%edx) , vertexPtr
	movl %ebp , oowa


/*     setup x and y */

	movl 2(%edx) , %ebx
	movl 2+4(%edx) , %ebp

	movl %ebx , -8(fifo)
	add $4 , %edx

	movl %ebp , -4(fifo)
	xor %ebx , %ebx


	movl tsuDataList(gc) , %ebp
	test $3 , %al

/*     set up color */

	je .L_vptrisetup_cull_clip_setup_ooz

	cmpl %ebx , colorType(gc)
	jne .L_vptrisetup_cull_clip_setup_pargb

	test $1 , %al
	je .L_vptrisetup_cull_clip_setup_a

	add $12 , fifo
	add $3 , %ebx

	flds _GlideRoot+pool_f255
	fmuls (vertexPtr,%ebp)
	flds _GlideRoot+pool_f255
	fmuls 4(vertexPtr,%ebp)
	flds _GlideRoot+pool_f255
	fmuls 8(vertexPtr,%ebp)
	fxch %st(2)
	fstps -12(fifo)
	fstps -8(fifo)
	fstps -4(fifo)
	movl tsuDataList+12(gc) , %ebp

.align 4
.L_vptrisetup_cull_clip_setup_a:

	test $2 , %al
	je .L_vptrisetup_cull_clip_setup_ooz

	add $4 , fifo
	inc %ebx

	flds (vertexPtr,%ebp)
	fmuls _GlideRoot+pool_f255
	fstps -4(fifo)

	movl tsuDataList(gc,%ebx,4) , %ebp
	jmp .L_vptrisetup_cull_clip_setup_ooz
.align 4
.L_vptrisetup_cull_clip_setup_pargb:
	add $4 , fifo
	movl (vertexPtr,%ebp) , %ebx

	movl %ebx , -4(fifo)
	nop 

	mov $1 , %ebx
	movl tsuDataList+4(gc) , %ebp
.align 4
.L_vptrisetup_cull_clip_setup_ooz:

	test $4 , %al
	je .L_vptrisetup_cull_clip_setup_qow

	add $4 , fifo
	inc %ebx

	flds (vertexPtr,%ebp)
	fmuls vp_hdepth(gc)
	fmuls oowa
	fadds vp_oz(gc)
	fstps -4(fifo)

	movl tsuDataList(gc,%ebx,4) , %ebp
.align 4
.L_vptrisetup_cull_clip_setup_qow:

	test $8 , %al
	je .L_vptrisetup_cull_clip_setup_qow0

	cmpl $1 , qInfo_mode(gc)
	jne .L_vptrisetup_cull_clip_setup_oow

	movl qInfo_offset(gc) , %ebp
	flds oowa
	fmuls (%ebp,vertexPtr)
	fstps (fifo)

	jmp .L_vptrisetup_cull_clip_setup_oow_inc
.align 4
.L_vptrisetup_cull_clip_setup_oow:

	movl oowa , %ebp

	movl %ebp , (fifo)
.align 4
.L_vptrisetup_cull_clip_setup_oow_inc:
	movl tsuDataList+4(gc,%ebx,4) , %ebp
	add $4 , fifo

	inc %ebx
.align 4
.L_vptrisetup_cull_clip_setup_qow0:

	test $16 , %al	/*  00000010H */
	je .L_vptrisetup_cull_clip_setup_stow0

	cmpl $1 , q0Info_mode(gc)
	jne .L_vptrisetup_cull_clip_setup_oow0

	movl q0Info_offset(gc) , %ebp

	flds oowa
	fmuls (%ebp,vertexPtr)
	fstps (fifo)

	jmp .L_vptrisetup_cull_clip_setup_oow0_inc
.align 4
.L_vptrisetup_cull_clip_setup_oow0:
	movl oowa , %ebp

	movl %ebp , (fifo)
.align 4
.L_vptrisetup_cull_clip_setup_oow0_inc:
	movl tsuDataList+4(gc,%ebx,4) , %ebp
	add $4 , fifo

	inc %ebx
.align 4
.L_vptrisetup_cull_clip_setup_stow0:

	test $32 , %al
	je .L_vptrisetup_cull_clip_setup_qow1


	flds oowa
	fmuls (vertexPtr,%ebp)

	add $8 , fifo
	add $2 , %ebx

	fmuls tmu0_s_scale(gc)
	flds oowa
	fmuls 4(vertexPtr,%ebp)
	movl tsuDataList(gc,%ebx,4) , %ebp
	fmuls tmu0_t_scale(gc)
	fxch 
	fstps -8(fifo)
	fstps -4(fifo)

.align 4
.L_vptrisetup_cull_clip_setup_qow1:

	test $64 , %al
	je .L_vptrisetup_cull_clip_setup_stow1

	cmpl $1 , q1Info_mode(gc)
	jne .L_vptrisetup_cull_clip_setup_oow1

	movl q1Info_offset(gc) , %ebp

	flds (%ebp,vertexPtr)
	fmuls oowa
	fstps (fifo)

	jmp .L_vptrisetup_cull_clip_setup_oow1_inc
.align 4
.L_vptrisetup_cull_clip_setup_oow1:
	movl oowa , %ebp

	movl %ebp , (fifo)
.align 4
.L_vptrisetup_cull_clip_setup_oow1_inc:
	movl tsuDataList+4(gc,%ebx,4) , %ebp
	add $4 , fifo

	inc %ebx
.align 4
.L_vptrisetup_cull_clip_setup_stow1:

	test $128 , %al
	je .L_vptrisetup_cull_clip_setup_end

	flds oowa
	fmuls (vertexPtr,%ebp)
	add $8 , fifo
	fmuls tmu1_s_scale(gc)
	flds oowa
	fmuls 4(vertexPtr,%ebp)
	fmuls tmu1_t_scale(gc)
	fxch 
	fstps -8(fifo)
	fstps -4(fifo)

.align 4
.L_vptrisetup_cull_clip_setup_end:

	cmp $12 , %edx
	jl .L_vptrisetup_cull_begin_for_loop

.align 4
.L_vptrisetup_cull_update_fifo_ptr:

	movl fifoPtr(gc) , %ebx
	movl fifoRoom(gc) , %edx

	sub fifo , %ebx
	mov $1 , %eax

	add %ebx , %edx
	pop %ebp

	movl %edx , fifoRoom(gc)
	pop %edi

	movl fifo , fifoPtr(gc)
	movl _GlideRoot+trisProcessed , %ebx

/*     _GlideRoot.stats.trisProcessed++ */


	pop %esi
	inc %ebx

	movl %ebx , _GlideRoot+trisProcessed
	pop %ebx

	ret	/*  0000000cH */
.align 4
.L_vptrisetup_cull___triDone:
/*  Restore trashed registers */
	mov (_GlideRoot+trisProcessed) , %esi
	pop %ebp

	add $1 , %esi	/*  _GlideRoot.stats.trisProcessed++     */
	pop %edi

	pop %esi
	mov %esi , (_GlideRoot+trisProcessed)

	pop %ebx
	ret

.L_END__vptrisetup_cull:
.size _vptrisetup_cull,.L_END__vptrisetup_cull-_vptrisetup_cull

#endif	/*  !GL_AMD3D */

.END

