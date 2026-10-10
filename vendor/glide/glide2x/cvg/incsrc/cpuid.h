#ifndef CPUID_H_included
#define CPUID_H_included

#define _CPU_HAS_CPUID		0xffff
#define _CPU_FEATURE_MMX	0x0001
#define _CPU_FEATURE_SSE	0x0002
#define _CPU_FEATURE_SSE2	0x0004
#define _CPU_FEATURE_3DNOW	0x0008
#define _CPU_FEATURE_3DNOWPLUS	0x0010
#define _CPU_FEATURE_MMXPLUS	0x0020

typedef struct _processor_info {
    char v_name[13]; /* vendor name */
    int family;      /* family of the processor, eg. Intel_Pentium_Pro is family 6 processor */
    int model;       /* madel of processor, eg. Intel_Pentium_Pro is model 1 of family 6 processor */
    int stepping;    /* Processor revision number */
    int feature;     /* processor Feature (same as return value). */
    int os_support;  /* does OS Support the feature */
} _p_info;

int _cpuid (_p_info *);

#endif
