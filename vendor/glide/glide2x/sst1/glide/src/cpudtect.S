#
# THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
# PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
# TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
# INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
# DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
# THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
# EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
# FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
# 
# USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
# RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
# TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
# AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
# SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
# THE UNITED STATES.  
# 
# COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
#
# 
# 2     3/04/97 9:10p Dow
# Neutered mutiplatform multiheaded monster.
#
#

# This used to be cpudtect.s but I renamed it as a quick way to make it work
# with the shared 3dfx.mak
	
.file  "cpudtect.s"

# 586P
# model FLAT,C

#      Data for data segment goes here
# DATA   SEGMENT DWORD USE32 PUBLIC DATA; 
# DATA   ENDS
    
# Some useful constants
# CPU Type
CPUTypeUnknown  .ASSIGNA 0xffffffff
CPUTypePrePent  .ASSIGNA 4
CPUTypeP5       .ASSIGNA 5
CPUTypeP6       .ASSIGNA 6
        
# References to external data:
    
#_TEXT   SEGMENT
#
#  _cpu_detect_asm - detect the type of CPU 
# 
#  USAGE:
#
#  int __cdecl _cpu_detect_asm(void);
#
#  returns 4 for non-pen

.text
.align 4
.globl _cpu_detect_asm
_cpu_detect_asm:		
P6Stuff:
    pusha                              # save all regs.

    # First, determine whether CPUID instruction is available.
    # If it's not, then it's a 386 or 486.
    pushf                               # push original EFLAGS.
    pop %eax                            # pop into eax
    mov %eax, %ecx                      # save original EFLAGS in ecx
    xor $0x200000, %eax                 # flip ID bit in EFLAGS
    push %eax                           # put it back on stack
    popf                                # pop into EFLAGS
    pushf                               # get EFLAGS back
    pop %eax                            # into eax
    xor %ecx, %eax                      # check to see if we could toggle ID
    jz  NotPentium                      # Sorry, not P5 or P6.
    
    #
    # Now determine whether it's an intel P6 CPU.
    #
    ##  Is it an Intel CPU?
    xor %eax, %eax                      # eax = 0.
    cpuid                               # get cpuid
    xor $0x756e6547, %ebx               # "Genu"
    jnz NotIntel
    xor $0x49656e69, %edx               # "ineI"
    jnz NotIntel
    xor $0x6c65746e, %ecx               # "ntel"
    jnz NotIntel                        #
    ##  Verifying architecture family
    mov $1, %eax
    cpuid                               # get family/model/stepping
    shr $8, %eax                        # rid of model & stepping number
    and $0xf, %eax                      # use only family
    cmp $6, %eax
    jl     IsP5                         # It's a P5
    ## Else it's a P6
    #
    # Intel P6 processor. 
    # Make sure it supports Memory Type Range Request registers
    #
IsP6:   
    popa
    mov $6, %eax                        # 
    ret                                 # return 

IsP5:   
    popa
    mov $5, %eax                        # 
    ret

NotPentium:
    popa
    mov $4, %eax
    ret

NotIntel:
    popa
    mov $0xffffffff, %eax
    ret

.L_end__cpu_detect_asm:
.size _cpu_detect_asm,.L_end__cpu_detect_asm-_cpu_detect_asm	
.END


