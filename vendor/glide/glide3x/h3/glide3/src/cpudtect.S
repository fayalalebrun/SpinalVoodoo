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
**
*/

/*
 *  Revision 1.1.1.1.6.3  2004/10/05 14:39:55  dborca
 *  removed detritus
 *
 *  Revision 1.1.1.1.6.2  2003/06/29 19:00:36  guillemj
 *  Fixed nested comments warnings.
 *  Removed some trailing spaces.
 *
 *  Revision 1.1.1.1.6.1  2003/05/05 07:12:46  dborca
 *  no message
 *
 *  Revision 1.1.1.1  1999/11/24 21:44:53  joseph
 *  Initial checkin for SourceForge
 *
 *
 *  4     4/06/99 3:54p Dow
 *  Alt tab again.
 *
 *  9     3/10/99 10:40a Peter
 *  detect katmai-ness
 *
 *  8     1/20/99 5:50p Peter
 *  norbert's code for k6 and k7 (?) wc
 *
 *  7     12/09/98 1:11p Peter
 *  cpu detection from Norbert for teh rest of the 3DNow!(tm) partners
 *
 *  6     10/03/98 3:35p Peter
 *  First pass at dynamic detection of 3DNow!(tm)
 *
 *  5     7/24/98 1:40p Hohn
 *
 *  4     5/28/97 8:23a Peter
 *  Merge w/ original glide source
 *
 *  2     3/04/97 9:10p Dow
 *  Neutered mutiplatform multiheaded monster.
 *
 */



.file "cpudtect.asm"

#include "assyntax.h"

/*       Data for data segment goes here */
/* _DATA   SEGMENT DWORD USE32 PUBLIC 'DATA'  */
/* _DATA   ENDS */

/*  Some useful constants */
/*  CPU Type */
#define CPUTypeUnknown $0x0ffffffff
#define CPUTypePrePent $0x4
#define CPUTypeP5 $0x5
#define CPUTypeP6 $0x6

/*  References to external data: */

.text


/*   _cpu_detect_asm - detect the type of CPU  */
/*   */
/*   USAGE: */

/*   int __cdecl _cpu_detect_asm(void) */

/*   returns 4 for non-pen */

GLOBL(_cpu_detect_asm)
.L_cpu_detect_asm_P6Stuff:
	push %esi	/*  save registers that are not volatile */
	push %edi
	push %ebx
	push %ebp

/*  First, determine whether CPUID instruction is available. */
/*  If it's not, then it's a 386 or 486. */
	pushf 	/*  push original EFLAGS. */
	pop %eax	/*  pop into eax */
	mov %eax , %ecx	/*  save original EFLAGS in ecx */
	xor $0x0200000 , %eax	/*  flip ID bit in EFLAGS */
	push %eax	/*  put it back on stack */
	popf 	/*  pop into EFLAGS */
	pushf 	/*  get EFLAGS back */
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
/*       eax - type[13:12] family[11:8] model[7:4] revision[3:0] */
/*       edx - feature bits */
	mov $1 , %eax
	cpuid 	/*  get family/model/stepping */

	shr $8 , %eax	/*  rid of model & stepping number */
	and $0x0f , %eax	/*  use only family */
	cmp $6 , %eax
	jl .L_cpu_detect_asm_IsP5	/*  It's a P5 */
/*  Else it's a P6 */

/*  Intel P6 processor.  */
/*  feature bits are in edx from the cpuid[1] */
.L_cpu_detect_asm_IsP6:
/*  Make sure it supports Memory Type Range Request registers (bit 12)     */
	mov %edx , %ebx
	test $0x1000 , %ebx

/*  Hmmmm... p6 w/o mtrr's? */
	jz .L_cpu_detect_asm_IsP5

/*  Check for katmai-ness (bit 25) */
	test $0x2000000 , %edx
	jz .L_cpu_detect_asm_noKNI

	mov $7 , %eax
	jmp .L_cpu_detect_asm_DoneCpu	/*  return */

.L_cpu_detect_asm_noKNI:
	mov $6 , %eax	/*   */
	jmp .L_cpu_detect_asm_DoneCpu	/*  return  */

.L_cpu_detect_asm_IsP5:
	mov $5 , %eax	/*   */
	jmp .L_cpu_detect_asm_DoneCpu

.L_cpu_detect_asm_NotPentium:
	mov $4 , %eax

.L_cpu_detect_asm_DoneCpu:
	pop %ebp
	pop %ebx
	pop %edi
	pop %esi
	ret


.L_cpu_detect_asm_NotIntel:

/*  This is a non-Intel processor. Figure out whether it supports */
/*  both MMX and 3DNow!, in which case we can use Norbert's cool */
/*  MMX/3DNow!(tm) code */

/*  The return value is split into two 16-bit fields. Bits [31:16] */
/*  identify the processor vendor as follows: */

/*  8001h   AMD */
/*  8002h   Cyrix */
/*  8003h   IDT */

/*  Bits [15:0] identify processor features as follows: */

/*  0001h   MMX */
/*  0002h   3DNow! (tm) */
/*  0004h   K6-style MTRRs */
/*  0008h   PentiumII-style MTRRs */

	xor %esi , %esi	/*  default feature flags */
	xor %edi , %edi	/*  default extended feature flags */

/*  Test whether extended feature function is supported */

	mov $0x80000000 , %eax
	cpuid 
	cmp $0x80000000 , %eax
	jbe .L_cpu_detect_asm_NoExtendedFeatures

/*  execute extended feature function */

	mov $0x80000001 , %eax
	cpuid 
	mov %edx , %edi

.L_cpu_detect_asm_NoExtendedFeatures:

/*  execute standard feature function */

	mov $1 , %eax
	cpuid 
	mov %edx , %esi
	mov %eax , %ebp	/*  save family/model/stepping */

/*  get the vendor string  */

	mov $0 , %eax
	cpuid 

.L_cpu_detect_asm_ChkAMD:
	cmp $0x68747541 , %ebx	/*  'htuA' */
	jne .L_cpu_detect_asm_ChkCyrix
	cmp $0x69746E65 , %edx	/*  'itne' */
	jne .L_cpu_detect_asm_ChkCyrix
	cmp $0x444D4163 , %ecx	/*  'DMAc' */
	je .L_cpu_detect_asm_CPUisAMD

.L_cpu_detect_asm_ChkCyrix:
	cmp $0x69727943 , %ebx	/*  'iryC'                    */
	jne .L_cpu_detect_asm_ChkIDT
	cmp $0x736E4978 , %edx	/*  'snIx' */
	jne .L_cpu_detect_asm_ChkIDT
	cmp $0x64616574 , %ecx	/*  'deat' */
	je .L_cpu_detect_asm_CPUisCyrix

.L_cpu_detect_asm_ChkIDT:
	cmp $0x746E6543 , %ebx	/*  'tneC' */
	jne .L_cpu_detect_asm_UnknownVendor
	cmp $0x48727561 , %edx	/*  'Hrua' */
	jne .L_cpu_detect_asm_UnknownVendor
	cmp $0x736C7561 , %ecx	/*  'slua' */
	jne .L_cpu_detect_asm_UnknownVendor

.L_cpu_detect_asm_CPUisIDT:
	mov $0x80030000 , %eax	/*  vendor = IDT, features = none */
	test $0x00800000 , %esi	/*  check for MMX bit in features */
	jz .L_cpu_detect_asm_DoneCpu
	or $1 , %eax	/*  set MMX feature flag */
	test $0x80000000 , %edi	/*  check for 3DNow! bit in extended features */
	jz .L_cpu_detect_asm_DoneCpu
	or $2 , %eax	/*  set 3DNow! feature flag */
	jmp .L_cpu_detect_asm_DoneCpu

.L_cpu_detect_asm_CPUisAMD:
	mov $0x80010000 , %eax	/*  vendor = AMD, features = none */
	mov %ebp , %edx	/*  family/model/stepping information */
	and $0x00000FFF , %edx	/*  extract family/model/stepping */
	cmp $0x00000588 , %edx	/*  CXT, Sharptooth, or K7 ? */
	jb .L_cpu_detect_asm_AmdMTRRchkDone	/*  nope, definitely no MTRRs */
	cmp $0x00000600 , %edx	/*  K7 or better ? */
	jb .L_cpu_detect_asm_AmdHasK6MTRR	/*  nope, but supports K6 MTRRs */
	or $8 , %eax	/*  set P2_MTRR feature flag */
	jmp .L_cpu_detect_asm_AmdMTRRchkDone
.L_cpu_detect_asm_AmdHasK6MTRR:
	or $4 , %eax	/*  set K6_MTRR feature flag */
.L_cpu_detect_asm_AmdMTRRchkDone:
	test $0x00800000 , %esi	/*  check for MMX bit in features */
	jz .L_cpu_detect_asm_DoneCpu
	or $1 , %eax	/*  set MMX feature flag */
	test $0x80000000 , %edi	/*  check for 3DNow! bit in extended features */
	jz .L_cpu_detect_asm_DoneCpu
	or $2 , %eax	/*  set 3DNow! feature flag */
	jmp .L_cpu_detect_asm_DoneCpu

.L_cpu_detect_asm_CPUisCyrix:
	mov $0x80020000 , %eax	/*  vendor = Cyrix, features = none */
	test $0x00800000 , %esi	/*  check for MMX bit in features */
	jz .L_cpu_detect_asm_DoneCpu
	or $1 , %eax	/*  set MMX feature flag */
	test $0x80000000 , %edi	/*  check for 3DNow! bit in extended features */
	jz .L_cpu_detect_asm_DoneCpu
	or $2 , %eax	/*  set 3DNow! feature flag */
	jmp .L_cpu_detect_asm_DoneCpu

.L_cpu_detect_asm_UnknownVendor:
	mov $0x0ffffffff , %eax
	jmp .L_cpu_detect_asm_DoneCpu


.L_END__cpu_detect_asm:
.size _cpu_detect_asm,.L_END__cpu_detect_asm-_cpu_detect_asm


/* ------------------------------------------------------------------------------    */
/*  this routine sets the precision to single */
/*  which effects all adds, mults, and divs */
ALIGN(4)	/*   */
GLOBL(single_precision_asm)
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
ALIGN(4)	/*   */
GLOBL(double_precision_asm)
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

.end
