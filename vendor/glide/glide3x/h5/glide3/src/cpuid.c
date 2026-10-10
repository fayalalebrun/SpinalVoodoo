/*
 * CPU detection code
 *
 * Revision 1.1.2.11  2005/05/07 08:26:35  jwrdegoede
 * gcc4 cpuid fix
 *
 * Revision 1.1.2.10  2004/10/05 14:54:29  dborca
 * DOS/OpenWatcom woes
 *
 * Revision 1.1.2.9  2003/08/04 12:45:47  dborca
 * Preparing for MinGW 2.0
 *
 * Revision 1.1.2.8  2003/07/29 10:04:32  dborca
 * Shamelessness.
 * Safeguard in CPUID.
 * Changed contact address!
 * Koolsmoky's texture download fixes.
 *
 * Revision 1.1.2.7  2003/07/25 07:14:58  dborca
 * ... in the name of the Linux, DRI and the sacred Glide...
 *
 * Revision 1.1.2.6  2003/07/24 13:13:03  koolsmoky
 * use __try/__except mechanism for win32 to catch SSE sigillegal in win95
 *
 * Revision 1.1.2.5  2003/07/01 11:16:42  dborca
 * fixed  a bug in GNUC code when running Intel; also removed detritus
 *
 * Revision 1.1.2.3  2003/06/13 07:22:58  dborca
 * more fixes to NASM sources
 *
 */

#include <signal.h>
#include <setjmp.h>
#include <string.h>

#include "cpuid.h"

#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ > 3)))
typedef unsigned int __attribute__((__may_alias__)) word32;
#else
typedef unsigned int word32;
#endif

/* These are the bit flags that get set on calling cpuid
 * with register eax set to 1
 */
#define _MMX_FEATURE_BIT	0x00800000
#define _SSE_FEATURE_BIT	0x02000000
#define _SSE2_FEATURE_BIT	0x04000000

/* This bit is set when cpuid is called with
 * register set to 80000001h (only applicable to AMD)
 */
#define _3DNOW_FEATURE_BIT	0x80000000
#define _3DNOWPLUS_FEATURE_BIT	0x40000000
#define _MMXPLUS_FEATURE_BIT	0x00400000

/* Testing code:
 * TEST_SSE       = xorps xmm0, xmm0
 * TEST_SSE2      = xorpd xmm0, xmm0
 * TEST_3DNOW     = femms
 * TEST_MMX       = emms
 * TEST_3DNOWPLUS = femms | pswapd mm0, mm0 | femms
 * TEST_MMXPLUS   = emms | pminsw mm0, mm0 | emms
 */
#ifdef __GNUC__
#define TEST_CPUID(f)    __asm __volatile ("pushl %%ebx; cpuid; popl %%ebx"::"a"(f):"%ecx", "%edx")
#define TEST_SSE()       __asm __volatile (".byte 0x0f, 0x57, 0xc0")
#define TEST_SSE2()      __asm __volatile (".byte 0x66, 0x0f, 0x57, 0xc0")
#define TEST_3DNOW()     __asm __volatile (".byte 0x0f, 0x0e")
#define TEST_MMX()       __asm __volatile (".byte 0x0f, 0x77")
#define TEST_3DNOWPLUS() __asm __volatile (".byte 0x0f, 0x0e, 0x0f, 0x0f, 0xc0, 0xbb, 0x0f, 0x0e")
#define TEST_MMXPLUS()   __asm __volatile (".byte 0x0f, 0x77, 0x0f, 0xea, 0xc0, 0x0f, 0x77")
#else
#define TEST_CPUID(f)    __asm { _asm mov eax, f _asm cpuid }
#define TEST_SSE()       __asm { _asm _emit 0x0f _asm _emit 0x57 _asm _emit 0xc0 }
#define TEST_SSE2()      __asm { _asm _emit 0x66 _asm _emit 0x0f _asm _emit 0x57 _asm _emit 0xc0 }
#define TEST_3DNOW()     __asm { _asm _emit 0x0f _asm _emit 0x0e }
#define TEST_MMX()       __asm { _asm _emit 0x0f _asm _emit 0x77 }
#define TEST_3DNOWPLUS() __asm { _asm _emit 0x0f _asm _emit 0x0e _asm _emit 0x0f _asm _emit 0x0f _asm _emit 0xc0 _asm _emit 0xbb _asm _emit 0x0f _asm _emit 0x0e }
#define TEST_MMXPLUS()   __asm { _asm _emit 0x0f _asm _emit 0x77 _asm _emit 0x0f _asm _emit 0xea _asm _emit 0xc0 _asm _emit 0x0f _asm _emit 0x77 }
#endif


#ifndef __MSC__
static jmp_buf j;


/* Desc: signal handler
 *
 * In  : signal number
 * Out : -
 *
 * Note: returns by `longjmp'ing
 */
static void handler (int signal)
{
 longjmp(j, signal + 1); /* so we can tell... also ensure we don't pass 0 */
}


/* Desc: check if CPU has specific feature
 *
 * In  : feature request
 * Out : 0 == fail, input == pass
 *
 * Note: this should be in the `has_feature' body. The reason it isn't:
 *       under some systems (notably Linux), the `setjmp' may thrash EBX,
 *       which is used for PositionIndependentCode/GlobalOffsetTable system.
 *       Since EBX is non-volatile register, it should be restored upon return.
 */
static int check_feature (int feature)
{
 if (setjmp(j)) {
    /* we got here only when `longjmp'ed by signal handlers */
    return 0;
 } else {
    /* we have signals and jump buffer set */
    switch (feature) {
           case _CPU_HAS_CPUID:         TEST_CPUID(0);    break;
           case _CPU_FEATURE_SSE:       TEST_SSE();       break;
           case _CPU_FEATURE_SSE2:      TEST_SSE2();      break;
           case _CPU_FEATURE_3DNOW:     TEST_3DNOW();     break;
           case _CPU_FEATURE_MMX:       TEST_MMX();       break;
           case _CPU_FEATURE_3DNOWPLUS: TEST_3DNOWPLUS(); break;
           case _CPU_FEATURE_MMXPLUS:   TEST_MMXPLUS();   break;
           default: return 0;
    }
    return feature;
 }
}
#endif


/* Desc: perform (possibly faulting) instructions in a safe manner
 *
 * In  : feature request
 * Out : 0 == fail, input == pass
 *
 * Note: pure ANSI code; stupid Watcom cannot handle this.
 */
static int has_feature (int feature)
{
#ifndef __MSC__
 int rv;

 /* register signal handlers */
 void (*old_sigill)(int) = signal(SIGILL, handler);
 if (old_sigill == SIG_ERR) {
    return 0;
 }

 rv = check_feature(feature);

 /* restore the signal handlers */
 signal(SIGILL, old_sigill);
 return rv;
#else
 /* Use the non-standard __try/__except mechanism because win95 fails to catch
  * sigillegal for SSE using standard signal mechanism.
  * HACK ALERT! HACK ALERT! HACK ALERT!
  * This means the MinGW version cannot be safely run under Win95!
  */
#define _TRY() __try {
#define _EXCEPTION() } __except(1) { return 0; } /* EXCEPTION_EXECUTE_HANDLER=1 */
 switch (feature) {
    case _CPU_HAS_CPUID:         _TRY() TEST_CPUID(0)    _EXCEPTION() break;
    case _CPU_FEATURE_SSE:       _TRY() TEST_SSE()       _EXCEPTION() break;
    case _CPU_FEATURE_SSE2:      _TRY() TEST_SSE2()      _EXCEPTION() break;
    case _CPU_FEATURE_3DNOW:     _TRY() TEST_3DNOW()     _EXCEPTION() break;
    case _CPU_FEATURE_MMX:       _TRY() TEST_MMX()       _EXCEPTION() break;
    case _CPU_FEATURE_3DNOWPLUS: _TRY() TEST_3DNOWPLUS() _EXCEPTION() break;
    case _CPU_FEATURE_MMXPLUS:   _TRY() TEST_MMXPLUS()   _EXCEPTION() break;
    default: return 0;
 }
 return feature;
#endif
}


/* Desc: get CPU info
 *
 * In  : pointer to _p_info
 * Out : features
 *
 * Note: -
 */
int _cpuid (_p_info *pinfo)
{
 word32 dwId = 0;
 word32 dwFeature = 0;
 word32 dwExt = 0;
 int feature = 0, os_support = 0;
 char Ident[13];

 if (!has_feature(_CPU_HAS_CPUID)) {
    return 0;
 }

#ifdef __GNUC__
 __asm("\n\
	/* get the vendor string */	\n\
	pushl	%%ebx			\n\
	xorl	%%eax, %%eax		\n\
	cpuid				\n\
	movl	%%ebx, %%eax		\n\
	popl	%%ebx			\n\
	movl	%%eax, %3		\n\
	movl	%%edx, %4		\n\
	movl	%%ecx, %5		\n\
	/* get the Standard bits */	\n\
	pushl	%%ebx			\n\
	movl	$1, %%eax		\n\
	cpuid				\n\
	popl	%%ebx			\n\
	movl	%%eax, %1		\n\
	movl	%%edx, %2		\n\
	/* get AMD-specials */		\n\
	pushl	%%ebx			\n\
	movl	$0x80000000, %%eax	\n\
	cpuid				\n\
	popl	%%ebx			\n\
	cmpl	$0x80000000, %%eax	\n\
	jc	0f			\n\
	pushl	%%ebx			\n\
	movl	$0x80000001, %%eax	\n\
	cpuid				\n\
	popl	%%ebx			\n\
	movl	%%edx, %0		\n\
 0:					\n\
 ":"=g"(dwExt), "=g"(dwId), "=g"(dwFeature),
   "=g"(((word32 *)Ident)[0]), "=g"(((word32 *)Ident)[1]), "=g"(((word32 *)Ident)[2])
 ::"%eax", "%ecx", "%edx");
#else
    _asm
    {
        push ebx
        push ecx
        push edx

        /* get the vendor string */
        xor eax,eax
        cpuid
        mov dword ptr [Ident],ebx
        mov dword ptr [Ident+4],edx
        mov dword ptr [Ident+8],ecx

        /* get the Standard bits */
        mov eax,1
        cpuid
        mov dwId,eax
        mov dwFeature,edx

        /* get AMD-specials */
        mov eax,80000000h
        cpuid
        cmp eax,80000000h
        jc notamd
        mov eax,80000001h
        cpuid
        mov dwExt,edx

notamd:
        pop ecx
        pop ebx
        pop edx
    }
#endif

#ifndef __WATCOMC__
 /* stupid watcom does not sigill... */
 if (dwFeature & _MMX_FEATURE_BIT) {
    feature |= _CPU_FEATURE_MMX;
    os_support |= has_feature(_CPU_FEATURE_MMX);
 }
 if (dwExt & _3DNOW_FEATURE_BIT) {
    feature |= _CPU_FEATURE_3DNOW;
    os_support |= has_feature(_CPU_FEATURE_3DNOW);
 }
 if (dwExt & _3DNOWPLUS_FEATURE_BIT) {
    feature |= _CPU_FEATURE_3DNOWPLUS;
    os_support |= has_feature(_CPU_FEATURE_3DNOWPLUS);
 }
 if (dwExt & _MMXPLUS_FEATURE_BIT) {
    feature |= _CPU_FEATURE_MMXPLUS;
    os_support |= has_feature(_CPU_FEATURE_MMXPLUS);
 }
 if (dwFeature & _SSE_FEATURE_BIT) {
    feature |= _CPU_FEATURE_SSE;
    os_support |= has_feature(_CPU_FEATURE_SSE);
 }
 if (dwFeature & _SSE2_FEATURE_BIT) {
    feature |= _CPU_FEATURE_SSE2;
    os_support |= has_feature(_CPU_FEATURE_SSE2);
 }
#endif

 if (pinfo) {
    memset(pinfo, 0, sizeof(_p_info));
    pinfo->os_support = os_support;
    pinfo->feature = feature;
    pinfo->family = (dwId >> 8) & 0xF; /* retrieving family */
    pinfo->model = (dwId >> 4) & 0xF;  /* retrieving model */
    pinfo->stepping = dwId & 0xF;      /* retrieving stepping */
    Ident[12] = 0;
    strcpy(pinfo->v_name, Ident);
 }

 return feature;
}


#if CPUTEST
#include <stdio.h>
/* Desc:
 *
 * In  :
 * Out :
 *
 * Note:
 */
int main (void)
{
 _p_info p;
 _cpuid(&p);
 printf("vendor  : %s\n", p.v_name);
 printf("family  : %d\n", p.family);
 printf("model   : %d\n", p.model);
 printf("stepping: %X\n", p.stepping);
 printf("feature : %08x\n", p.feature);
 printf("support : %08x\n", p.os_support);
 printf("--------\n");
 printf("cpuid   : %d\n", has_feature(_CPU_HAS_CPUID));
 printf("MMX     : %d\n", has_feature(_CPU_FEATURE_MMX));
 printf("SSE     : %d\n", has_feature(_CPU_FEATURE_SSE));
 printf("SSE2    : %d\n", has_feature(_CPU_FEATURE_SSE2));
 printf("3DNow!  : %d\n", has_feature(_CPU_FEATURE_3DNOW));
 printf("3DNow!+ : %d\n", has_feature(_CPU_FEATURE_3DNOWPLUS));
 printf("MMX+    : %d\n", has_feature(_CPU_FEATURE_MMXPLUS));
 return 0;
}
#endif
