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

/*  Revision 1.1.1.1.8.2  2004/10/05 14:39:55  dborca
/*  removed detritus
/*
/*  Revision 1.1.1.1.8.1  2003/05/05 07:12:47  dborca
/*  no message
/*
/*  Revision 1.1.1.1  1999/11/24 21:45:00  joseph
/*  Initial checkin for SourceForge
/* */
/*   */
/*  5     4/08/99 6:21p Atai */
/*  added contect check for _grTexDownload_3DNow_MMX */
/*   */
/*  9     4/08/99 1:22p Atai */
/*  added contect check for _grTexDownload_3DNow_MMX */
/*   */
/*  8     3/19/99 11:26a Peter */
/*  expose direct fifo for gl */
/*   */
/*  7     2/02/99 4:36p Peter */
/*  download through lfb rather than texture port */
/*   */
/*  6     12/17/98 2:36p Atai */
/*  check in Norbert's fix for texture download width correction */
/*   */
/*  5     12/07/98 11:33a Peter */
/*  norbert's re-fixes of my merge */
/*   */
/*  4     11/02/98 5:34p Atai */
/*  merge direct i/o code */
/*   */
/*  3     10/20/98 5:34p Atai */
/*  added #ifdefs for hwc */
/*   */
/*  2     10/14/98 12:05p Peter */
/*  fixed my f*cked up assumption about non-volatile regs */
/*   */
/*  1     10/09/98 6:48p Peter */
/*  3DNow!(tm) version of wide texture downloads */
/*   */
/*  3     10/07/98 9:43p Peter */
/*  triangle procs for 3DNow!(tm) */
/*   */
/*  2     10/05/98 7:43p Peter */
/*  3DNow!(tm) happiness everywhere */
/*   */
/*  1     10/05/98 6:01p Peter */
/*  mmx stuff for 3DNow!(tm) capable processors */
/*   */

.file "xtexdl.asm"

#include "assyntax.h"

#ifdef USE_PACKET_FIFO
#endif

/*  Definitions of cvg regs and glide root structures. */
#include "fxgasm.h"

/*  Arguments (STKOFF = 16 from 4 dword pushes) */
#define STACKOFFSET 16
#define _gc 4 + STACKOFFSET
#define _baseAddr 8 + STACKOFFSET
#define _maxS 12 + STACKOFFSET
#define _minT 16 + STACKOFFSET
#define _maxT 20 + STACKOFFSET
#define _texData 24 + STACKOFFSET

/*  NB: The first set of registers (eax, ecx, and edx) are volatile across */
/*  function calls. The remaining registers are supposedly non-volatile */
/*  so they only store things that are non-volatile across the call. */

#define fifo ebp	/*  fifo ptr in inner loop */
#define gc esi	/*  graphics context */
#define dataPtr edi	/*  pointer to exture data to be downloaded */
#define curT ebx	/*  counter for texture scan lines (t-coordinate) */
#define curS ecx	/*  texture s-coordinate */
#define fRoom edx	/*  room available in fifo (in bytes) */

.macro GR_FIFO_WRITE __addr __offset __data
	mov \__data , (\__addr+\__offset)
.endm	/*  GR_FIFO_WRITE */

/* -------------------------------------------------------------------------- */

.text


ALIGN(32)

GLOBL(_grTexDownload_3DNow_MMX)

	push %ebx	/*  save caller's register variable */
	mov _maxT-12(%esp) , %curT	/*  curT = maxT */

	push %esi	/*  save caller's register variable */
	mov _minT-8(%esp) , %eax	/*  minT */

	push %edi	/*  save caller's register variable */
	mov _gc-4(%esp) , %gc	/*  gc */

	push %ebp	/*  save caller's register variable */
	mov _texData(%esp) , %dataPtr	/*  dataPtr */

#ifdef GLIDE_ALT_TAB
	test %gc , %gc
	je .L_grTexDownload_3DNow_MMX___dlDone
	movl lostContext(%gc) , %edx
	mov (%edx) , %ecx
	test $1 , %ecx
	jnz .L_grTexDownload_3DNow_MMX___dlDone
#endif

	sub %eax , %curT	/*  curT = maxT - minT */
	mov fifoPtr(%gc) , %fifo	/*  fifoPtr */

	mov _maxS(%esp) , %curS	/*  curS = maxS  */
	add $1 , %curT	/*  curT = maxT - minT + 1 */

	femms 	/*  we'll use MMX/3DNow!, make sure FPU register cleared */

	mov %curS , %edx	/*  curS = maxS = scanline width in DWORDs */
	movd _baseAddr(%esp) , %mm3	/*  0 | address of texture to download */

	shl $2 , %curS	/*  scan line width (in bytes) */
	mov _minT(%esp) , %eax	/*  0 | minT */

	mov %curS , _maxS(%esp)	/*  save scan line width (in bytes) */
	shl $3 , %edx	/*  packetHdr<21:3> = maxS = scanline width in DWORDs */

	imul %curS , %eax	/*  TEX_ROW_ADDR_INCR(minT) = minT * TEX_ROW_ADDR_INCR(1) */

	movd %curS , %mm2	/*  0 | TEX_ROW_ADDR_INCR(1) */
	or $0x00000005 , %edx	/*  packetHdr<31:30> = lfb port */
/*  packetHdr<21:3>  = maxS */
/*  packetHdr<2:0>   = packetType 5  */

	movd %edx , %mm1	/*  0 | packetHdr */
	movd %eax , %mm4	/*  0 | TEX_ROW_ADDR_INCR(minT) */

	psllq $32 , %mm2	/*  TEX_ROW_ADDR_INCR(1) | 0 */
	paddd %mm4 , %mm3	/*  0 | texAddr = texBaseAddr + TEX_ROW_ADDR_INCR(minT) */

	mov fifoRoom(%gc) , %fRoom	/*  get available fifoRoom (in bytes) */
	punpckldq %mm3 , %mm1	/*  hdr2 = texAddr | hdr1 = packetHdr */

/*  ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = curS = maxS */
/*  edx = fifoRoom, mm1 = texAddr|packetHdr, mm2 = TEX_ROW_ADDR_INCR(1)|0 */

	test $4 , %fifo	/*  is fifo QWORD aligned ? */
	jz .L_grTexDownload_3DNow_MMX___startDownload	/*  yup, start texture download */

	cmp $4 , %fRoom	/*  enough room for NULL packet in fifo? */
	jge .L_grTexDownload_3DNow_MMX___mmxAlignFifo	/*  yes, write NULL packet to align fifo */

#ifdef USE_PACKET_FIFO
	push $__LINE__	/*  Line # inside this function */
	push $0	/*  NULL file name */

	push $4	/*  fifo space required (bytes) */
	call EXTRN(_grCommandTransportMakeRoom)	/*  make fifo room */
	add $12, %esp
#endif

	mov fifoPtr(%gc) , %fifo	/*  fifoPtr modified by _grCommandTransportMakeRoom, reload */

	mov fifoRoom(%gc) , %fRoom	/*  fifoRoom modified by _grCommandTransportMakeRoom, reload */
	mov _maxS(%esp) , %curS	/*  reload maxS (destroyed by call to _grCommandTransportMakeRoom) */

	test $4 , %fifo	/*  new fifoPtr QWORD aligned ? */
	jz .L_grTexDownload_3DNow_MMX___startDownload	/*  yup, start texture download */

.L_grTexDownload_3DNow_MMX___mmxAlignFifo:

	movl $0 , (%fifo)	/*  write NULL packet */
	sub $4 , %fRoom	/*  fifoRoom -= 4 */

	mov %fRoom , fifoRoom(%gc)	/*  store new fifoRoom */
	add $4 , %fifo	/*  fifoPtr += 4 */

	mov %fifo , fifoPtr(%gc)	/*  store new fifoPtr */
	jmp .L_grTexDownload_3DNow_MMX___startDownload	/*  fifo aligned, download texture now */

ALIGN(32)

/*  ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = maxS = curS */
/*  edx=fifoRoom, mm1 = texAddr|packetHdr, mm2 = TEX_ROW_ADDR_INCR(1)|0 */

.L_grTexDownload_3DNow_MMX___loopT:

#ifdef GLIDE_DEBUG

/*  Make sure that we have a QWORD aligned fifoPtr force GP if not aligned */

	test $4 , %fifo	/*  is fifoPtr QWORD aligned ? */
	jz .L_grTexDownload_3DNow_MMX___alignmentOK	/*  yup, continue */

	xor %eax , %eax	/*  create 0 */
	mov %eax , (%eax)	/*  move to DS:[0] forces GP  */
.L_grTexDownload_3DNow_MMX___alignmentOK:
#endif	/*  GLIDE_DEBUG */

/*  Compute packet header words */
/*   hdr1: downloadSpace[31:30] numWords[21:3] packetType[2:0] */
/*   hdr2: download address[29:0] */

	movq %mm1 , (%fifo)	/*  store hdr2 | hdr1 */
	add $8 , %fifo	/*  increment fifo ptr (hdr1 + hdr2) */

/*  S coordinate inner loop unrolled for 8 texels a write */

.L_grTexDownload_3DNow_MMX___loopS:

	movq (%dataPtr) , %mm0	/*  load 64 bit data (8 texels) */
	add $8 , %fifo	/*  pre-increment fifoPtr += 2 * sizeof(FxU32) */

	add $8 , %dataPtr	/*  dataPtr += 2 * sizeof(FxU32) */
	sub $8 , %curS	/*  curS -= 2 * sizeof(FxU32) */

	movq %mm0 , -8(%fifo)	/*  *fifoPtr = texelData[64 bits] */
	jnz .L_grTexDownload_3DNow_MMX___loopS	/*  loop while curS > 0 */

	mov fifoPtr(%gc) , %ecx	/*  old fifo ptr */
	nop 	/*  filler */

	mov %fifo , %eax	/*  new fifo ptr */
	mov %fifo , fifoPtr(%gc)	/*  save new fifo ptr */

	sub %ecx , %eax	/*  new fifo ptr - old fifo ptr = fifo space used up */
	mov _maxS(%esp) , %curS	/*  curS = maxS = width of scanline (bytes) */

	sub %eax , %fRoom	/*  new fifo space available = old fifo space available - fifo space used up = new fifo space available */
	sub $1 , %curT	/*  curT-- */

	mov %fRoom , fifoRoom(%gc)	/*  save new fifo space available  */
	jz .L_grTexDownload_3DNow_MMX___dlDone	/*  loop while curT > 0 */

/*  Check for room to write the next texture scanline */

/*  ebx = curT, edi = dataPtr, esi = gc, ebp = fifo */
/*  edx = fifoRoom, mm1 = texAddr|packetHdr, mm2 = TEX_ROW_ADDR_INCR(1)|0 */

	paddd %mm2 , %mm1	/*  texAddr+=TEX_ROW_ADDR_INCR(1) | packetHdr */
	mov %esp , %esp	/*  filler */
.L_grTexDownload_3DNow_MMX___startDownload:
	lea 8(%curS) , %eax	/*  fifo space needed = scan line width + header size */

	cmp %eax , %fRoom	/*  fifo space available >= fifo space required ? */
	jge .L_grTexDownload_3DNow_MMX___loopT	/*  yup, write next scan line */

#ifdef USE_PACKET_FIFO
	push $__LINE__	/*  Line # inside this function */
	push $0x0	/*  NULL file name */

	push %eax	/*  fifo space required */
	call EXTRN(_grCommandTransportMakeRoom)	/*  make fifo room (if fifoPtr QWORD aligned before */
	add $12, %esp
#endif

	mov fifoPtr(%gc) , %fifo	/*  fifoPtr was modified by _grCommandTransportMakeRoom, reload */

	mov fifoRoom(%gc) , %fRoom	/*  fifoRoom was modified by _grCommandTransportMakeRoom, reload */
	mov _maxS(%esp) , %curS	/*  curS = maxS = width of scanline (bytes) */
	jmp .L_grTexDownload_3DNow_MMX___loopT	/*  we now have enough fifo room, write next scanline */

.L_grTexDownload_3DNow_MMX___dlDone:

	femms 	/*  exit 3DNow!(tm) state */

	pop %ebp	/*  restore caller's register variable */
	pop %edi	/*  restore caller's register variable */

	pop %esi	/*  restore caller's register variable */
	pop %ebx	/*  restore caller's register variable */

	ret	/*  pop 6 DWORD parameters and return */

.L_END_grTexDownload_3DNow_MMX:
.size _grTexDownload_3DNow_MMX,.L_END_grTexDownload_3DNow_MMX-_grTexDownload_3DNow_MMX

.end
