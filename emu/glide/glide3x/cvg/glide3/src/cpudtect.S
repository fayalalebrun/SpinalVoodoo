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
/*  6     10/03/98 3:35p Peter */
/*  First pass at dynamic detection of 3DNow!(tm) */
/*   */
/*  5     7/24/98 1:40p Hohn */
/*   */
/*  4     5/28/97 8:23a Peter */
/*  Merge w/ original glide source */
/*   */
/*  2     3/04/97 9:10p Dow */
/*  Neutered mutiplatform multiheaded monster. */



.file "cpudtect.asm"


/*       Data for data segment goes here */
/* _DATA   SEGMENT DWORD USE32 PUBLIC 'DATA'  */
/* _DATA   ENDS */

/*  Some useful constants */
/*  CPU Type */
#define CPUTypeUnknown 0ffffffffh
#define CPUTypePrePent 4h
#define CPUTypeP5 5h
#define CPUTypeP6 6h

/*  References to external data: */

.text


/*   _cpu_detect_asm - detect the type of CPU  */
/*   */
/*   USAGE: */

/*   int __cdecl _cpu_detect_asm(void) */

/*   returns 4 for non-pen */

.globl _cpu_detect_asm
.type _cpu_detect_asm,@function
_cpu_detect_asm:
.L_cpu_detect_asm_P6Stuff:
	pushal 	/*  save all regs. */

/*  First, determine whether CPUID instruction is available. */
/*  If it's not, then it's a 386 or 486. */
	pushfl 	/*  push original EFLAGS. */
	pop %eax	/*  pop into eax */
	mov %eax , %ecx	/*  save original EFLAGS in ecx */
	xor $0x0200000 , %eax	/*  flip ID bit in EFLAGS */
	push %eax	/*  put it back on stack */
	popfl 	/*  pop into EFLAGS */
	pushfl 	/*  get EFLAGS back */
	pop %eax	/*  into eax */
	xor %ecx , %eax	/*  check to see if we could toggle ID */
	jz .L_cpu_detect_asm_NotPentium	/*  Sorry, not P5 or P6. */


/*  Now determine whether it's an intel P6 CPU. */

/*   Is it an Intel CPU? */
	xor %eax , %eax	/*  eax = 0. */
	cpuid 	/*  get cpuid */
	xor $0x0756e6547 , %ebx	/*  "Genu" */
	jnz .L_cpu_detect_asm_NotIntel
	xor $0x049656e69 , %edx	/*  "ineI" */
	jnz .L_cpu_detect_asm_NotIntel
	xor $0x06c65746e , %ecx	/*  "ntel" */
	jnz .L_cpu_detect_asm_NotIntel

/*   Verifying architecture family */
	mov $1 , %eax
	cpuid 	/*  get family/model/stepping */
	shr $8 , %eax	/*  rid of model & stepping number */
	and $0x0f , %eax	/*  use only family */
	cmp $6 , %eax
	jl .L_cpu_detect_asm_IsP5	/*  It's a P5 */
/*  Else it's a P6 */

/*  Intel P6 processor.  */
/*  Make sure it supports Memory Type Range Request registers */

.L_cpu_detect_asm_IsP6:
	popal 
	mov $6 , %eax	/*   */
	ret	/*  return  */

.L_cpu_detect_asm_IsP5:
	popal 
	mov $5 , %eax	/*   */
	ret

.L_cpu_detect_asm_NotPentium:
	popal 
	mov $4 , %eax
	ret

/*  Not and intel, but we need to figure out */
/*  if we have a 3DNow!(tm) capable processor so that we */
/*  can vector off to Norbert's fun stuff. */
.L_cpu_detect_asm_NotIntel:

/*  Re-issue cpuid(0) because the testing trashed the values. */
	xor %eax , %eax
	cpuid 

	xor $0x68747541 , %ebx	/*  'htuA' */
	jnz .L_cpu_detect_asm_UnknownVendor
	xor $0x69746E65 , %edx	/*  'itne' */
	jnz .L_cpu_detect_asm_UnknownVendor
	xor $0x444D4163 , %ecx	/*  'DMAccAMD' */
	jnz .L_cpu_detect_asm_UnknownVendor

/*  We're on an AMD system check to see if it handles */
/*  extended function queries for 3DNow!(tm) and MMX */
	mov $0x80000000 , %eax
	cpuid 

	cmp $0x80000001 , %eax
	jl .L_cpu_detect_asm_NoNorbert

/*  At last check for 3DNow!(tm) and MMX */
	mov $0x80000001 , %eax
	cpuid 

	mov %edx , %ebx
	and $0x00800000 , %ebx
	jz .L_cpu_detect_asm_NoNorbert

	mov %edx , %ebx
	and $0x80000000 , %ebx
	jz .L_cpu_detect_asm_NoMMX

	popal 
	mov $0x80010002 , %eax
	ret

.L_cpu_detect_asm_NoMMX:
	popal 
	mov $0x80010001 , %eax
	ret

.L_cpu_detect_asm_NoNorbert:
	popal 
	mov $0x80010000 , %eax
	ret

.L_cpu_detect_asm_UnknownVendor:
	popal 
	mov $0x0ffffffff , %eax
	ret

.L_END__cpu_detect_asm:
.size _cpu_detect_asm,.L_END__cpu_detect_asm-_cpu_detect_asm


/* ------------------------------------------------------------------------------    */
/*  this routine sets the precision to single */
/*  which effects all adds, mults, and divs */
.align 4	/*   */
.globl single_precision_asm
.type single_precision_asm,@function
single_precision_asm:
	push %eax	/*  make room */
	fnclex 	/*  clear pending exceptions     */
	fstcw (%esp)
	movl (%esp) , %eax
	and $0x0000fcff , %eax	/*  clear bits 9:8 */
	movl %eax , (%esp)
	fldcw (%esp)
	pop %eax
	ret
.L_END_single_precision_asm:
.size single_precision_asm,.L_END_single_precision_asm-single_precision_asm

/* ------------------------------------------------------------------------------    */
/*  this routine sets the precision to double */
/*  which effects all adds, mults, and divs */
.align 4	/*   */
.globl double_precision_asm
.type double_precision_asm,@function
double_precision_asm:
	push %eax	/*  make room */
	fnclex 	/*  clear pending exceptions     */
	fstcw (%esp)
	movl (%esp) , %eax
	and $0x0000fcff , %eax	/*  clear bits 9:8 */
	or $0x000002ff , %eax	/*  set 9:8 to 10 */
	movl %eax , (%esp)
	fldcw (%esp)
	pop %eax
	ret
.L_END_double_precision_asm:
.size double_precision_asm,.L_END_double_precision_asm-double_precision_asm

.END
