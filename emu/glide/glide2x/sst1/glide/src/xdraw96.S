## THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
## PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
## TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
## INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
## DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
## THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
## EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
## FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
## 
## USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
## RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
## TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
## AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
## SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
## THE UNITED STATES.  
## 
## COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
##
# 
# 2     7/07/97 2:14p Jdt
# assembly now on par with C code.
# 
# 1     7/07/97 8:37a Jdt
# B4 Chip field fix.
##

# This used to be xdraw96.s but I renamed it as a quick way to make it work
# with the shared 3dfx.mak
	
.file   "xdraw96.s"
#OPTION OLDSTRUCTS       
#.586P


	
#EXTRN   _GlideRoot:DWORD
#EXTRN   _grSpinFifo:NEAR
#EXTRN   _grSst96FifoMakeRoom:NEAR

        
# some useful floating load and store macros <ala gmt>
#flds    TEXTEQU <fld  DWORD PTR>
#fsubs   TEXTEQU <fsub DWORD PTR>
#fmuls   TEXTEQU <fmul DWORD PTR>

.align 4
.section	.data
One         .DATA  0x03f800000
Area        .DATA  0
dxAB        .DATA  0
dxBC        .DATA  0
dyAB        .DATA  0
dyBC        .DATA  0
culltest    .DATA  0
P6FenceVar  .DATA  0

# Ugly, but seems to workaround the problem with locally defined
# data segment globals not getting relocated properly when using
# djgpp.

zArea   .EQU One+4
zdxAB   .EQU One+8
zdxBC   .EQU One+12
zdyAB   .EQU One+16
zdyBC   .EQU One+20
zculltest .EQU One+24

### Some useful SST-1 offsets
.INCLUDE "fxgasm.h"

## enables/disables trisProcessed and trisDrawn counters
STATS .ASSIGNA 1

#--------------------------------------------------------------------------        
### Macro for P6 Fencing operation
### Note that this destroys eax, but if you do 2 fences eax will be restored
#.AIF \&GLIDE_DEBUG EQ 1
#EXTRN   _GR_SET_GW_CMD:NEAR
#EXTRN   _GR_SET_GW_HEADER:NEAR
#EXTRN   _GR_SET_GW_ENTRY:NEAR
#.AENDI

.MACRO GR_SET_GW_CMD   addr offset data
.AIF \&GLIDE_DEBUG EQ 1
    push %eax
    push %ecx
    push %edx

    push \data
    lea  \offset (\addr), %eax
    push %eax
    call _GR_SET_GW_CMD
    add  $8, %esp
    pop  %edx
    pop  %ecx
    pop  %eax
.AELSE
    mov \data, \offset (\addr)
.AENDI
.ENDM

.MACRO GR_SET_GW_HEADER  addr offset data
.AIF \&GLIDE_DEBUG EQ 1
    push %eax
    push %ecx
    push %edx
    push \data
    lea  \offset (\addr), %eax
    push %eax
    call _GR_SET_GW_HEADER
    add  $8, %esp
    pop  %edx
    pop  %ecx
    pop  %eax
.AELSE
    mov \data, \offset (\addr)
.AENDI
.ENDM

.MACRO GR_SET_GW_ENTRY   addr offset data
.AIF \&GLIDE_DEBUG EQ 1
    push %eax
    push %ecx
    push %edx
    push \data
    lea  \offset (\addr), %eax
    push %eax
    call _GR_SET_GW_ENTRY
    add  $8, %esp
    pop  %edx
    pop  %ecx
    pop  %eax
.AELSE
    mov \data, \offset (\addr)
.AENDI
.ENDM

.MACRO GR_FSET_GW_ENTRY addr offset
.AIF \&GLIDE_DEBUG EQ 1
    push %eax
    push %ecx
    push %edx
    sub  $4, %esp
    fstps (%esp)
    lea  \offset (\addr), %eax
    push %eax
    call _GR_SET_GW_ENTRY
    add  $8, %esp
    pop  %edx
    pop  %ecx
    pop  %eax
.AELSE
    fstps \offset (\addr)
.AENDI
.ENDM

#--------------------------------------------------------------------------        
# Arguments (STKOFF = 16 from 4 pushes)
.AIF \&GLIDE_DEBUG EQ 1
STKOFF	.EQU 16
.AELSE	
STKOFF  .EQU 16
.AENDI	
_va$    .EQU  4 + STKOFF
_vb$    .EQU  8 + STKOFF
_vc$    .EQU 12 + STKOFF

X       .EQU 0
Y       .EQU 4

fa      .REG     (%eax)       # vtx a from caller
fb      .REG     (%ebx)       # vtx b from caller
fc      .REG     (%ecx)       # vtx c from caller
                              # edx is used as index, loading from *src
gc      .REG     (%esi)       # points to graphics context
dlp     .REG     (%esi)       # points to dataList structure
hw      .REG     (%edi)       # points to the hardware
fifo    .REG     (%edi)       # points to next entry in fifo

tmpx    .REG     (%edx)       # temp X storage
i       .REG     (%edx)       # i = dlp->i
tmpy    .REG     (%ebp)       # temp Y storage

.text
.align 4

#--------------------------------------------------------------------------

.AIF \&GLIDE_DEBUG EQ 0
.globl  grDrawTriangle
.type	grDrawTriangle, @function
grDrawTriangle:
.AENDI	
.globl  grDrawTriangle_asm
.type grDrawTriangle_asm, @function
grDrawTriangle_asm:	
# FALL THRU to _trisetup


	
	
#--------------------------------------------------------------------------        
##
##  _trisetup_asm - do triangle setup & send down draw command
##
##  USAGE:
##
##  
.globl  _trisetup_asm
.type _trisetup_asm, @function
_trisetup_asm:	
# 28


    # save ebx, esi, edi, ebp
    push    %ebx
    push    %esi

    push    %edi
    push    %ebp

#vertex sorting is done as follows:
#   1 First, load the y values of vtx a, b, c into integer registers 
#
#       This is done in the segment vertex_load below.
#
#       Note that this trashes the pointers, we will reload them later
#
#       If any of the y values is negative, convert to a pseudo-2s complement
#       form by xor'ing with 7fffffff. This lets us use 2s complement integer
#       comparisons for floating point comparisons.
#
#   2 Compare a.y, b.y, c.y, and setup pointers fa, fb, fc so that
#       a.y <= b.y <= c.y.
#
#       Note that fa, fb, fc registers point once more to vertex structs,
#       though not necessarily in the order that the application passes it to
#       this routine.
#
#       This is done in the segment vertex_y_sort below, with many many 
#       compare/branches.
#
#       All the compare/branches converge to sst_load_xy_regs.
#
#   3 Load the (x,y) values into the floating point SST registers for Fa, Fb,
#       and Fc. Done in the segment sst_load_xy_regs.
#
#   4 Restore fa, fb, fc to the user's order. Also save the vertex
#       with lowest y value on the stack, this will be used later for 
#       loading parameter values into the SST regs.
#
##############


#--------------------------------------------------------------------------        

    mov     _va$(%esp), fa   # 1
    mov     _vb$(%esp), fb
    mov     _vc$(%esp), fc
    movl    (_GlideRoot+trisProcessed), tmpy    # _GlideRoot.stats.trisProcessed++#
# 36-3
.Lvertex_y_load:
    mov     Y(fa), fa        # 2
    mov     Y(fb), fb
    cmp     $0, fa           # 3
    jge     .La_positive
    xor     $0x7fffffff, fa
.align 4
.La_positive:
    cmp     $0, fb            # 4
    jge     .Lb_positive
    xor     $0x7fffffff, fb 
.align 4
.Lb_positive:
    mov     Y(fc), fc        # 5
    movl    (_GlideRoot+curGC), gc
    cmp     $0, fc           # 6
    jge     .Lc_positive
    xor     $0x7fffffff, fc
.Lc_positive:
# 42-3
# 24-30 with direct fall-thru
# jmp ret_pop0f
#--------------------------------------------------------------------------        
.Lvertex_y_sort:

    cmp     fb, fa              # if (ay < by) {
    jge     .LAgtB
    cmp     fc, fb              # if (by > cy) {
    jle     .Labc
    cmp     fc, fa              # if (ay < cy) {
    jge     .Lcab

.Lacb:                          # acb
        mov     _va$(%esp), fa
        mov     cull_mode(gc), tmpx # culltest = gc->state.cull_mode
        mov     _vc$(%esp), fb
        mov     _vb$(%esp), fc
        xor     $1, tmpx                 # culltest ^= 1
        jmp     .LArea_Computation

.align 4
.Lcab:                          # cab
        mov _vc$(%esp), fa            # fa = &vc->x
        mov cull_mode(gc), tmpx       # culltest = gc->state.cull_mode
        mov _va$(%esp), fb            # fb = &va->x
        mov _vb$(%esp), fc            # fc = &vb->x
                                      # HOLE
        jmp .LArea_Computation

.align 4
.Labc:                          # abc
        mov _va$(%esp), fa          # fa = &va->x
        mov _vb$(%esp), fb          # fb = &vb->x
        mov _vc$(%esp), fc          # fc = &vc->x
        mov cull_mode(gc), tmpx     # culltest = gc->state.cull_mode
                                    # HOLE
        jmp .LArea_Computation

.align 4
.LAgtB:                          # } else {
        cmp     fc, fb          #   if (by < cy) 
        jge     .Lcba
        cmp     fc, fa          #   if (ay < cy)
        jge     .Lbca
.Lbac:                          # bac
        mov     _vb$(%esp), fa
        mov     cull_mode(gc), tmpx # culltest = gc->state.cull_mode
        mov     _va$(%esp), fb
        mov     _vc$(%esp), fc
        xor     $1, tmpx                 # culltest ^= 1
        jmp     .LArea_Computation

.align 4
.Lcba:                          # cba
        mov     _vc$(%esp), fa
        mov     cull_mode(gc), tmpx # culltest = gc->state.cull_mode
        mov     _vb$(%esp), fb
        mov     _va$(%esp), fc
        xor     $1, tmpx                 # culltest ^= 1
        jmp     .LArea_Computation

.align 4
.Lbca:                          # bca
        mov     _vb$(%esp), fa
        mov     _vc$(%esp), fb
        mov     _va$(%esp), fc
        mov     cull_mode(gc), tmpx # culltest = gc->state.cull_mode
#--------------------------------------------------------------------------        
.align 4
.LArea_Computation:
# 47-3
# jmp ret_pop0f
    flds    X(fa)            #  xa
    fsubs   X(fb)            #  dxAB
    fsts    dxAB
    flds    X(fb)            #  |    xb
    fsubs   X(fc)            #  |    dxBC
    fsts    dxBC
    flds    Y(fb)            #  |    |    yb
    fsubs   Y(fc)            #  |    |    dyBC
    fsts    dyBC
    flds    Y(fa)            #  |    |    |    ya
    fsubs   Y(fb)            #  |    |    |    dyAB
    fsts    dyAB
    fld     %st(3)           #  |    |    |    |    dxAB
    fsts    Area
    fmul    %st(2), %st      #  |    |    |    |    t0         t0=dxAB*dyBC
    fsts    Area
    fld     %st(3)           #  |    |    |    |    |    dxBC
    fsts    Area
    fmul    %st(2), %st      #  |    |    |    |    |    t1    t1=dxBC*dyAB
    fsts    Area
    fsubrp  %st,%st(1)       #  |    |    |    |    area

    fsts    zArea            #  |    |    |    |    area

    # Zero Area Triangle Check
    mov     zArea, tmpy        # j = *(long *)&area
    and     $0x7fffffff,tmpy   # if ((j & 0x7FFFFFFF) == 0)
    jz      .Lzero_area

    # Cull Check
    sal     $31, tmpx                   # culltest<<31
    mov     cull_mode (gc), tmpy        # load gc->state.cull_mode
    test    tmpy, tmpy                  # if (gc->state.cull_mode != GR_CULL_DISABLE)
    je      .Lnocull1
                                        # culling ENABLED
    mov     zArea, tmpy                 # reload area
    xor     tmpx, tmpy                  # if (j ^ (culltest<<31))
    jge     .Lbackfaced

.Lnocull1:                        # culling disabled
    # OOA Calculation
    fdivrs  One                 #  |    |    |    |    ooa

    # Fetch Fifo Ptr
    mov     fifoPtr(gc), fifo

    # P6 Check
    mov     (_GlideRoot+CPUType), tmpx
    cmp     $6, tmpx
    
    je      .Lfence

.align 4
.LfenceDone:  
    # Wrap Check
    mov     (_GlideRoot+curTriSize), tmpx
    mov     fifoSize(gc), tmpy

    sub     tmpx, tmpy
    jl      .Lwrap

.align 4
.LwrapDone:
    mov     tmpy, fifoSize(gc)
    # Adjust Fifo Ptr
    mov     fifo, tmpy
    add     tmpx, tmpy
    mov     tmpy, fifoPtr(gc)

    # Begin Tri Packet
    mov     gwCommand(gc), tmpx
    mov     gwHeaders(gc), tmpy

    GR_SET_GW_CMD    fifo, 0, tmpx
    GR_SET_GW_HEADER fifo, 4, tmpy

    # Download X, Y * 3
    mov     X(fa), tmpx
    mov     Y(fa), tmpy

    GR_SET_GW_ENTRY fifo, 8,  tmpx
    GR_SET_GW_ENTRY fifo, 12, tmpy

    mov     X(fb), tmpx
    mov     Y(fb), tmpy

    GR_SET_GW_ENTRY fifo, 16, tmpx
    GR_SET_GW_ENTRY fifo, 20, tmpy

    mov     X(fc), tmpx
    mov     Y(fc), tmpy

    GR_SET_GW_ENTRY fifo, 24, tmpx
    GR_SET_GW_ENTRY fifo, 28, tmpy

    add     $32, fifo

    mov     dataList(gc), i            # i = gc->datalist[0]->i
    add     $ dataList, dlp            # dlp = gc->dataList

    # Early out for no interp
    test    i, i
    je      .Lno_interpolation

    # Setup for Parameter Calculator
                                #   dxAB  dxBC  dyBC  dyAB  ooa
    fmul    %st, %st(4)         #   DXAB  |     |     |     |
    fmul    %st, %st(3)         #   |     DXBC  |     |     |
    fmul    %st, %st(2)         #   |     |     DYBC  |     |
    fmulp   %st, %st(1)         #   |     |     |     DYAB
    fxch    %st(3)              #   DYAB  |     |     DXAB  
    fstps   zdxAB               #   |     |     DYBC
    fstps   zdyBC               #   |     DXBC
    fstps   zdxBC               #   DYAB
    fstps   zdyAB               #

    # Parameter Calculator
.align 4
.Lnext_parm:
    test    $1, i
    jnz     .Lsecondary_packet

    mov     (fa, i), tmpy                 # tmpy = fa[i]
    flds    (fa, i)                       #   pa
    fsubs   (fb, i)                       #   dpAB
    flds    (fb, i)                       #   |    pb
    fsubs   (fc, i)	                  #   dpAB dpBC

    fld     %st(1)                        #   |    |    dpAB   
    fmuls   zdyBC                         #   |    |    p0x
    fld     %st(1)                        #   |    |    |    dpBC
    fmuls   zdyAB                         #   |    |    |    p1x
    fxch    %st(3)                        #   p1x  |    |    dpAB
					  #   |    |    |    |
    GR_SET_GW_ENTRY fifo, 0, tmpy
    fmuls   zdxBC                         #   |    |    |    p1y
    fxch    %st(2)                        #   |    p1y  |    dpBC
    fmuls   zdxAB                         #   |    |    |    p0y
    fxch    %st(3)                        #   p0y  |    |    p1x
    fsubrp  %st, %st(1)                   #   |    |    dpdx
    fxch    %st(2)                        #   dpdx |    p0y
    fsubp   %st, %st(1)                   #   |    dpdy
    fxch    %st(1)                        #   dpdy dpdx
    mov     dl_i + SIZEOF_dataList(dlp), i # i = dlp[1]->i
    add     $ SIZEOF_dataList, dlp        # dlp++
					  #   |
    GR_FSET_GW_ENTRY fifo, 4
					  #   empty
    GR_FSET_GW_ENTRY fifo, 8
    add     $12, fifo
    test    i,i                           # while (i)
    jne     .Lnext_parm                     # 

.align 4
.Ltriangle_command:
    # Write Triangle Command
    mov     zArea, tmpx
    GR_SET_GW_ENTRY fifo, 0, tmpx
    test    $7, fifo
    jnz     .Lno_padding0
    GR_SET_GW_ENTRY fifo, 4, tmpx
.Lno_padding0:
    # Cleanup
    pop    %ebp
    pop    %edi
    pop    %esi
    pop    %ebx
    mov    $1, %eax                     # return 1 (triangle drawn)
    ret
    

.align 4
.Lzero_area:
.Lbackfaced:
    fstp    %st(0)   # 4
    fstp    %st(0)   # 3
    fstp    %st(0)   # 2
    fstp    %st(0)   # 1
    fstp    %st(0)   # 0
    pop     %ebp
    pop     %edi
    pop     %esi
    pop     %ebx
    xor     %eax, %eax                    # return 0 (triangle drawn)
    ret

.align 4
.Lwrap:
    push    %eax
    push    %ebx	
    push    %ecx
    push    %edx
    call    _grSst96FifoMakeRoom
    pop     %edx
    pop     %ecx
    pop     %ebx
    pop     %eax
    mov     fifoPtr(gc), fifo
    mov     fifoSize(gc), tmpy
    sub     tmpx, tmpy
    jmp     .LwrapDone

.align 4
.Lfence:
    mov     writesSinceFence(gc), tmpx
    mov     (_GlideRoot+curTriSize), tmpy
    
    shr     $2, tmpy
    sub     tmpy, tmpx
    jl      .Ldofence

    mov     tmpx, writesSinceFence(gc)
    jmp     .LfenceDone

.align 4
.Ldofence:
    push    %eax
    xchg    %eax, P6FenceVar
    pop     %eax
    jmp     .LfenceDone

.align 4
.Lsecondary_packet:
    test    $7, fifo
    jz      .Lno_padding1
    GR_SET_GW_ENTRY  fifo, 0, tmpx 
    add     $4, fifo
.align 4
.Lno_padding1:
    push    tmpx
    push    gc
    
    mov     dl_addr(dlp), tmpx
    mov     (_GlideRoot+curGC), gc

    GR_SET_GW_CMD  fifo, 0, tmpx
    mov     gwHeaders(gc, 4), tmpy
    
    GR_SET_GW_HEADER fifo, 4, tmpy
    pop     gc

    pop     tmpx
    add     $8, fifo

    mov     dl_i + SIZEOF_dataList(dlp), i
    add     $ SIZEOF_dataList, dlp
.align 4
.Lnext_parm_1:
    mov    (fa, i), tmpx               # tmpy = fa[i]
    flds   (fa, i)                     #   pa
    fsubs  (fb, i)                     #   dpAB
    flds   (fb, i)                     #   |    pb
    fsubs  (fc, i)                     #   dpAB dpBC 

    fld    %st(1)                      #   |    |    dpAB   
    fmuls  zdyBC                       #   |    |    p0x
    fld    %st(1)                      #   |    |    |    dpBC
    fmuls  zdyAB                       #   |    |    |    p1x
    fxch   %st(3)                      #   p1x  |    |    dpAB
				       #   |    |    |    |
    GR_SET_GW_ENTRY fifo, 0, tmpy       
    fmuls  zdxBC                       #   |    |    |    p1y
    fxch   %st(2)                      #   |    p1y  |    dpBC
    fmuls  zdxAB                       #   |    |    |    p0y
    fxch   %st(3)                      #   p0y  |    |    p1x
    fsubrp %st,%st(1)                  #   |    |    dpdx
    fxch   %st(2)                      #   dpdx |    p0y
    fsubp  %st,%st(1)                  #   |    dpdy
    fxch   %st(1)                      #   dpdy dpdx
    mov    dl_i + SIZEOF_dataList(dlp), i # i = dlp[1]->i
    add    $ SIZEOF_dataList, dlp        # dlp++
				       #   |
    GR_FSET_GW_ENTRY fifo, 4
				       #   empty
    GR_FSET_GW_ENTRY fifo, 8            
    add     $12, fifo
    test    i,i                        # while (i)
    jne     .Lnext_parm_1 

    test    $7, fifo
    jz      .Ltriangle_command_packet
    GR_SET_GW_ENTRY fifo, 0, tmpx
    add     $4, fifo
.align 4
.Ltriangle_command_packet:
    mov     (_GlideRoot+curGC), gc
    mov     $0x40000000, tmpy

    mov     gwCommand(gc), tmpx
    mov     zArea, fa

    GR_SET_GW_CMD    fifo, 0, tmpx
    GR_SET_GW_HEADER fifo, 4, tmpy

    GR_SET_GW_ENTRY  fifo, 8,  fa
    GR_SET_GW_ENTRY  fifo, 12, fa

    pop    %ebp
    pop    %edi
    pop    %esi
    pop    %ebx
    mov    $1, %eax                     # return 1 (triangle drawn)
    ret

.align 4
.Lno_interpolation:
    fstp    %st(0)     # 4
    fstp    %st(0)     # 3 
    fstp    %st(0)     # 2 
    fstp    %st(0)     # 1 
    fstp    %st(0)     # 0
    jmp     .Ltriangle_command

.L_end_tri_setup_asm:
.AIF \&GLIDE_DEBUG EQ 0
.size	grDrawTriangle,.L_end_tri_setup_asm-grDrawTriangle
.AENDI	
.size	grDrawTriangle_asm,.L_end_tri_setup_asm-grDrawTriangle_asm
.size	_trisetup_asm,.L_end_tri_setup_asm-_trisetup_asm	        
	
# [++++ from above]
# This comment was moved here to make the code in the loop more readable 
# STALL HERE due to PCI write buffer being full on last instruction:        
# The write to the packerHackAddress takes 3 bus clocks, during which
# we may not write to the PCI buffer without stalling.  This causes
# the amount of clocks the workaround adds to the loop to vary in the
# following way++:
# 
#    CPU          Bus/CPU Clock     Total Bus       Total Penalty
#                     Ratio*      Clocks Since   (add to later clocks)
#======================================================================  
# P55c-200/P5-180:    6/1              1.3       (3 - 1.3) * 6 + 6 = 17 
# P5-166/P5-150:      5/1              1.8       (3 - 1.8) * 5 + 6 = 12 
# P5-120:             4/1              2.25      (3 - 2.25) * 4 + 6 = 9 
# P5-100/P5-90:       3/1               3        (3 - 3) * 3 + 6 = 6
#
# * P5-200 haa a 33MHz bus, P5-166 has a 33MHz bus, 
#   P5-150 has a 30MHz bus, P5-120 has a 30MHz bus,
#   P5-100 has a 33MHz bus, P5-90 has a 30MHz bus
#
#++ These calculations also assume that we NEVER get to continue a burst 
#   from having the next parameter be consecutive from the current one.
#   If we did get that, then we have to add two more bus clocks to the
#   penalty for the next write (labeled loop clock 10), so that looks like
#      CPU                  Total Penalty
#      ============================================        
#      P55c-200/P5-180:(2 * 6) + 17 = 29 clocks!!!
#      P5-166/P5-150:  (2 * 5) + 12 = 22 clocks!!
#      P5-120:         (2 * 4) +  9 = 17 clocks!
#      P5-100/P5-90    (2 * 3) +  6 = 12 clocks    
#

#_TEXT ENDS
.END
