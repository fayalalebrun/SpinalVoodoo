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



/*  Definitions of cvg regs and glide root structures. */
#include "fxgasm.h"

/*  Arguments (STKOFF = 12 from 3 dword pushes) */
#define STACKOFFSET 12
#define _gc$ STACKOFFSET
#define _baseAddr$ STACKOFFSET
#define _maxS$ STACKOFFSET
#define _minT$ STACKOFFSET
#define _maxT$ STACKOFFSET
#define _texData$ STACKOFFSET

/*  NB: The first set of registers (eax-edx) are volatile across */
/*  function calls. The remaining registers are supposedly non-volatile */
/*  so they only store things that are non-volatile across the call. */
#define fifo %eax	/*  Current fifo ptr in inner loop */
#define texAddr %ebx	/*  Physical download address of the current scanline     */
#define gc %ecx	/*  Current graphics context */
#define curS %edx	/*  Current texture scanline */

#define maxT %esi	/*  Max scanline line value (inclusive) */
#define dataPtr %edi	/*  Current user texture data ptr */
#define curT %ebp	/*  Current s coordinate in inner loop */

#define temp1 curS
#define temp2 texAddr
#define temp3 gc

#define GR_FIFO_WRITE(__addr , __offset , __data) \
	mov __data , __offset(__addr)


/* -------------------------------------------------------------------------- */
.text


.align 32

#ifdef GL_AMD3D	
.globl __grTexDownload_3DNow_MMX
.type __grTexDownload_3DNow_MMX,@function
__grTexDownload_3DNow_MMX:

/*  Function prologue type things */
/*  NB:   We are not bothering to preserve the contents */
/* 	     of eax, ebx, ecx, edx because they are volatile */
/* 	     by convention. */

/*  Enter 3DNow!(tm) state for the duration of the function */
/*  because we don't use or call anything that uses fp. */
	femms 

	mov _gc$-STACKOFFSET+0(%esp) , gc
	push %esi

	mov _maxT$-STACKOFFSET+4(%esp) , maxT
	push %edi

	shl $9 , maxT	/*  Convert maxT to rowAddr format */
	push %ebp

	mov _texData$(%esp) , dataPtr
	mov _minT$(%esp) , curT

/*  Pad out fifo so that we can use mmx writes the whole way w/o */
/*  any intermediate tests in the inner loop for fifo alignment. */
/*  Conveniently, the packet header is 2 dwords which matches */
/*  the size of the mmx write. */
	mov fifoPtr(gc) , fifo	/*  Cache fifo ptr */
	mov _baseAddr$(%esp) , texAddr	/*  Texture physical address */

	mov _maxS$(%esp) , %temp1	/*  Pre-convert maxS into packet 5 field format */
	sub tex_ptr(gc) , texAddr	/*  Convert to hw base relative address             */

	shl $2 , %temp1	/*  Write size dwords -> bytes */
	mov texAddr , _baseAddr$(%esp)

	shl $9 , curT	/*  curT = TEX_ROW_ADDR_INCR(curT)       */
	mov %temp1 , _maxS$(%esp)	/*  Write back converted s coordinate */

	shl $1 , %temp1	/*  Write size to packet 5 field format */
	test $0x4 , fifo	/*  Aligned fifo ptr? */

	mov fifoRoom(gc) , %temp2	/*  temp2 = gc->fifoRoom */
	mov %temp1 , _maxT$(%esp)	/*  Write back converted field format size */

	jz .L__grTexDownload_3DNow_MMX___loopT

/*  Check to make sure there's room in the fifo. If not then */
/*  we'll wrap and then it should be aligned for the remainder of */
/*  this function invocation. */
	cmp $0x4 , %temp2
	jg .L__grTexDownload_3DNow_MMX___mmxAlignFifo

	push __LINE__	/*  Line # inside this function */
	push $0x0	/*  NULL file name */

	push $0x4	/*  fifo space required */
	call __FifoMakeRoom	/*  Get fifo room */

/*  Calling out to external code means that our registers can get */
/*  trashed in the same way that we trash things. Plus we need to */
/*  re-cache the fifoPtr since we may have wrapped. */

	add $12 , %esp	/*  Pop the 3 DWORDs for the fifoWrap parameters */
	mov _gc$(%esp) , gc

/*  Setup the regs to do the alignment */
	mov fifoPtr(gc) , fifo
	test $0x4 , fifo

	mov fifoRoom(gc) , %temp2
	jz .L__grTexDownload_3DNow_MMX___loopT

.L__grTexDownload_3DNow_MMX___mmxAlignFifo:
	add $0x4 , fifo	/*  packetPtr++ */
	xor %temp1 , %temp1	/*  Clear the nop packet */

	mov fifo , fifoPtr(gc)	/*  gc->fifoPtr = packetPtr */
	sub $0x4 , %temp2	/*  fifoRoom -= 4 */

	mov %temp2 , fifoRoom(gc)	/*  gc->fifoRoom = fifoRoom */
	GR_FIFO_WRITE(fifo , -4 , %temp1)	/*  NOP packet(0) */

.align 4
.L__grTexDownload_3DNow_MMX___loopT:
/*  Check for room to write the current texture scanline     */
	mov _maxS$(%esp) , %temp1	/*  temp1 = width of scanline (bytes) */
	mov fifoRoom(gc) , %temp2	/*  temp2 = gc->fifoRoom (bytes) */

	add $0x0C , %temp1	/*  scanline width + sizeof(packet hdr) (bytes) + nop packet to mmx align */
	cmp %temp1 , %temp2	/*  fifo space required >= space availible ? */

	jge __dlBegin	/*  Yes, start download now w/ no more checking */

	push __LINE__	/*  Line # inside this function */
	push $0x0	/*  NULL file name */

	push %temp1	/*  fifo space required */
	call __FifoMakeRoom	/*  Get fifo room */

	add $12 , %esp	/*  Pop the 3 DWORDs for the fifoWrap parameters */
	nop 

/*  Calling out to external code means that our registers can get */
/*  trashed in the same way that we trash things. Plus we need to */
/*  re-cache the fifoPtr since we may have wrapped. */
	mov _gc$(%esp) , gc
	mov fifoPtr(gc) , fifo

#ifdef GLIDE_DEBUG
/*  Make sure that we have an mmx happy aligned fifoPtr */
	test $4 , fifo
	jz .L__grTexDownload_3DNow_MMX

/*  Fault right away because this would be a huge suck */
	xor %eax , %eax
	mov %eax , (%eax)
.L__grTexDownload_3DNow_MMX:
#endif	/*  GLIDE_DEBUG     */

.align 4
.L__grTexDownload_3DNow_MMX___dlBegin:

#ifdef GLIDE_DEBUG
/*  Make sure that we have an mmx happy aligned fifoPtr */
	test $4 , fifo
	jz .L__grTexDownload_3DNow_MMX_2

/*  Fault right away because this would be a huge suck */
	xor %eax , %eax
	mov %eax , (%eax)
.L__grTexDownload_3DNow_MMX_2:
#endif	/*  GLIDE_DEBUG */

/*  Compute packet header words */
/* 	hdr1: downloadSpace[31:30] numWords[21:3] packetType[2:0] */
/*   hdr2: download address[29:0] */
	mov _baseAddr$(%esp) , texAddr	/*  Download base address */
	mov _maxT$(%esp) , %temp1	/*  Pre-Converted # of words per packet/scanline */

	mov $0x0C0000005 , %temp3	/*  Base packet header (texture port | packet type 5) */
	add $8 , fifo	/*  Pre-increment fifo ptr (hdr1) */

	or %temp1 , %temp3	/*  Base packet hdr | # of words */
	add curT , texAddr	/*  texAddr = texBaseAddr + TEX_ROW_ADDR_INCR(curT) */

	GR_FIFO_WRITE(fifo , -8 , %temp3)	/*  Write hdr1 */
	add $0x200 , curT	/*  curT += TEX_ROW_ADDR_INCR(1) */

	GR_FIFO_WRITE(fifo , -4 , texAddr)	/*  write hdr2 */
	mov _maxS$(%esp) , curS	/*  curS = maxS */

.align 4
/*  S coordinate inner loop unrolled for 8 texels a write */
.L__grTexDownload_3DNow_MMX___loopS:
	movq (dataPtr) , %mm0	/*  load (mmx) 64 bit data (8 texels) */
	add $0x8 , fifo	/*  pre-increment fifoPtr += 2 * sizeof(FxU32) */

	add $0x8 , dataPtr	/*  dataPtr += 2 * sizeof(FxU32) */
	sub $0x8 , curS	/*  curS -= 2 * sizeof(FxU32) */

	movq %mm0 , -8(fifo)	/*  *fifoPtr = texelData[64 bits] */
	jnz .L__grTexDownload_3DNow_MMX___loopS	/*  if curS > 0 */

	mov _gc$(%esp) , gc	/*  Re-cache gc which was trashed in the dl loop */
	mov fifo , %temp1

/*  Update gc->fifoPtr and gc->fifoRoom for the wrap/stall check */
	mov fifoPtr(gc) , %temp2
	sub %temp2 , %temp1	/*  # of bytes written to the fifo */

	mov fifo , fifoPtr(gc)	/*  gc->fifoPtr = packetPtr */
	mov fifoRoom(gc) , %temp2

	sub %temp1 , %temp2	/*  # of bytes left in fifo */
	cmp maxT , curT	/*  if (curT <= maxT) ?     */

	mov %temp2 , fifoRoom(gc)
	jle .L__grTexDownload_3DNow_MMX___loopT

.L__grTexDownload_3DNow_MMX___dlDone:
.align 4

	femms 	/*  Exit 3DNow!(tm) state     */
	pop %ebp

	pop %edi
	pop %esi

	ret	/*  Pop 6 parameters and return */

.L_END___grTexDownload_3DNow_MMX:
.size __grTexDownload_3DNow_MMX,.L_END___grTexDownload_3DNow_MMX-__grTexDownload_3DNow_MMX

#endif

.END

