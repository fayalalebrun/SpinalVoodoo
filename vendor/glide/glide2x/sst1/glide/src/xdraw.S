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
# Revision 1.2  1999/12/11 00:41:48  joseph
# Changed \(.data) to .section .data to fix build errors with binutils 2.9.5.
#
# Revision 1.1.1.1  1999/12/07 21:48:55  joseph
# Initial checkin into SourceForge.
# 
# 15    3/04/97 9:10p Dow
# Neutered mutiplatform multiheaded monster.
# 
# 14    2/26/97 11:58a Jdt
# Fixed stack bug
# 
# 13    2/19/97 4:42p Dow
# Fixed debug build for Watcom
# 
# 12    1/18/97 11:50p Dow
# Added support for GMT's register debugging
# 
# 11    12/23/96 1:37p Dow
# chagnes for multiplatform glide
# 
# 10    11/15/96 5:11p Jdt
# SST-96 exclusion
#
#

# This used to be xdraw.s but I renamed it as a quick way to make it work
# with the shared 3dfx.mak
	
# This is a GASP input file
	
.file  "xdraw.s"
# OPTION OLDSTRUCTS       
# .586P

# EXTRN   _GlideRoot:DWORD
# EXTRN   _grSpinFifo:NEAR
        
# some useful floating load and store macros <ala gmt>
# flds    TEXTEQU <fld  DWORD PTR>
# fsubs   TEXTEQU <fsub DWORD PTR>
# fmuls   TEXTEQU <fmul DWORD PTR>

	.align 4
.section	.data
One		.DATA 0x03f800000
Area		.DATA 0
dxAB		.DATA 0
dxBC		.DATA 0
dyAB		.DATA 0
dyBC		.DATA 0
culltest	.DATA 0
P6FenceVar	.DATA 0
DBG		.DATA 0	

zArea		.EQU One+04
zdxAB		.EQU One+8
zdxBC		.EQU One+12
zdyAB		.EQU One+16
zdyBC		.EQU One+20
zculltest	.EQU One+24

## Some useful SST-1 offsets
.INCLUDE "fxgasm.h"

# enables/disables trisProcessed and trisDrawn counters
STATS .ASSIGNA 1

#--------------------------------------------------------------------------
## Macro for P6 Fencing operation
## Note that this destroys eax, but if you do 2 fences eax will be
## restored
.AIF \&GLIDE_DEBUG EQ 1
# EXTRN   _grFence:NEAR		

.MACRO P6Fence
   push    %eax
   push    %ecx
   push    %edx
   call    _grFence
   pop     %edx
   pop     %ecx
   pop     %eax
   xchgl   %eax, P6FenceVar
.ENDM
.AELSE
.MACRO P6Fence
    xchg   %eax, P6FenceVar
.ENDM            
.AENDI # GLIDE_DEBUG

.AIF \&GLIDE_HARDWARE EQ 1
.AIF \&GLIDE_DEBUG EQ 1

#EXTRN   _GR_SET:NEAR
#EXTRN   _GR_SETF:NEAR

.AENDI # GLIDE_DEBUG

.MACRO GR_SET  hw offset value
.AIF \&GLIDE_DEBUG EQ 1
    push    %eax
    push    %ecx
    push    %edx

    push    \value
    lea     \offset (\hw),%eax
    push    %eax
    call    _GR_SET
    add     $8, %esp

    pop     %edx
    pop     %ecx
    pop     %eax
.AENDI # GLIDE_DEBUG
    movl    \value, \offset (\hw)
.ENDM

.MACRO GR_SETF_P  hw offset
.AIF \&GLIDE_DEBUG EQ 1
    push    %eax
    push    %ecx
    push    %edx

    fstl    -4(%esp)
    sub     $4, %esp
    lea     \offset (\hw),%eax
    push    %eax
    call    _GR_SETF
    add     $8, %esp

    pop     %edx
    pop     %ecx
    pop     %eax
.AENDI # GLIDE_DEBUG
    fstps   \offset (\hw)
.ENDM

.AENDI # GLIDE_HARDWARE

#--------------------------------------------------------------------------
# Arguments (STKOFF = 16 from 4 pushes)
.AIF \&GLIDE_DEBUG EQ 1
STKOFF	.EQU  16
.AELSE
STKOFF	.EQU  16
.AENDI
_va$    .EQU  4 + STKOFF
_vb$    .EQU  8 + STKOFF
_vc$    .EQU  12 + STKOFF

X       .EQU 0
Y       .EQU 4

fa      .REG     (%eax)      # vtx a from caller
fb      .REG     (%ebx)      # vtx b from caller
fc      .REG     (%ecx)      # vtx c from caller
                             #      edx is used as index, loading from *src
gc      .REG     (%esi)      # points to graphics context
dlp     .REG     (%esi)      # points to dataList structure
hw      .REG     (%edi)      # points to the hardware

tmpx    .REG     (%edx)      # temp X storage
i       .REG     (%edx)      # i = dlp->i
tmpy    .REG     (%ebp)      # temp Y storage

#--------------------------------------------------------------------------
.text
	.align 16
	
.AIF \&GLIDE_DEBUG EQ 0
.globl grDrawTriangle
	.type grDrawTriangle, @function
grDrawTriangle:
.AENDI
.globl grDrawTriangle_asm
	.type  grDrawTriangle_asm, @function
grDrawTriangle_asm:

# FALL THRU to _trisetup

#--------------------------------------------------------------------------
#
#  _trisetup_asm - do triangle setup & send down draw command
#
#  USAGE:
#
#  
.globl  _trisetup_asm
	.type _trisetup_asm, @function
_trisetup_asm:
#            .code
# 28
    # save ebx, esi, edi, ebp
    push    %ebx
    push    %esi

    push    %edi
    push    %ebp

#vertex sorting is done as follows:
#   1. First, load the y values of vtx a, b, c into integer registers 
#
#       This is done in the segment vertex_load below.
#
#       Note that this trashes the pointers, we will reload them later
#
#       If any of the y values is negative, convert to a pseudo-2s complement
#       form by xor'ing with 7fffffff. This lets us use 2's complement integer
#       comparisons for floating point comparisons.
#
#   2. Compare a.y, b.y, c.y, and setup pointers fa, fb, fc so that
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
#   3. Load the (x,y) values into the floating point SST registers for Fa, Fb,
#       and Fc. Done in the segment sst_load_xy_regs.
#
#   4. Restore fa, fb, fc to the user's order. Also save the vertex
#       with lowest y value on the stack, this will be used later for 
#       loading parameter values into the SST regs.
#
#######


#--------------------------------------------------------------------------
    mov     _va$(%esp), fa   # 1
    mov     _vb$(%esp), fb
    mov     _vc$(%esp), fc
    movl    (_GlideRoot+trisProcessed), tmpy    # _GlideRoot.stats.trisProcessed++#
# 36-3
vertex_y_load:
    mov     Y(fa), fa        # 2
    mov     Y(fb), fb
    cmp     $0, fa           # 3
    jge     a_positive
    xor     $0x7fffffff, fa
.align 4
a_positive:
    cmp     $0, fb            # 4
    jge     b_positive
    xor     $0x7fffffff, fb 
.align 4
b_positive:
    mov     Y(fc), fc        # 5
    movl    (_GlideRoot+curGC), gc
    cmp     $0, fc           # 6
    jge     c_positive
    xor     $0x7fffffff, fc
c_positive:
# 42-3
# 24-30 with direct fall-thru
# jmp ret_pop0f
#--------------------------------------------------------------------------
vertex_y_sort:

    cmp     fb, fa              # if (ay < by) {
    jge     AgtB
    cmp     fc, fb              # if (by > cy) {
    jle     abc
    cmp     fc, fa              # if (ay < cy) {
    jge     cab

acb:                          # acb
        mov     _va$(%esp), fa
        mov     cull_mode(gc), tmpx # culltest = gc->state.cull_mode
        mov     _vc$(%esp), fb
        mov     _vb$(%esp), fc
        xor     $1, tmpx                 # culltest ^= 1
        jmp     Area_Computation

.align 4
cab:                          # cab
        mov _vc$(%esp), fa            # fa = &vc->x
        mov cull_mode(gc), tmpx       # culltest = gc->state.cull_mode
        mov _va$(%esp), fb            # fb = &va->x
        mov _vb$(%esp), fc            # fc = &vb->x
                                      # HOLE
        jmp Area_Computation

.align 4
abc:                          # abc
        mov _va$(%esp), fa          # fa = &va->x
        mov _vb$(%esp), fb          # fb = &vb->x
        mov _vc$(%esp), fc          # fc = &vc->x
        mov cull_mode(gc), tmpx     # culltest = gc->state.cull_mode
                                    # HOLE
        jmp Area_Computation

.align 4
AgtB:                          # } else {
        cmp     fc, fb          #   if (by < cy) 
        jge     cba
        cmp     fc, fa          #   if (ay < cy)
        jge     bca
bac:                          # bac
        mov     _vb$(%esp), fa
        mov     cull_mode(gc), tmpx # culltest = gc->state.cull_mode
        mov     _va$(%esp), fb
        mov     _vc$(%esp), fc
        xor     $1, tmpx                 # culltest ^= 1
        jmp     Area_Computation

.align 4
cba:                          # cba
        mov     _vc$(%esp), fa
        mov     cull_mode(gc), tmpx # culltest = gc->state.cull_mode
        mov     _vb$(%esp), fb
        mov     _va$(%esp), fc
        xor     $1, tmpx                 # culltest ^= 1
        jmp     Area_Computation

.align 4
bca:                          # bca
        mov     _vb$(%esp), fa
        mov     _vc$(%esp), fb
        mov     _va$(%esp), fc
        mov     cull_mode(gc), tmpx # culltest = gc->state.cull_mode
#--------------------------------------------------------------------------
.align 4
Area_Computation:
# 47-3
# jmp ret_pop0f
    flds    X(fa)            #  xa
    fsubs   X(fb)            #  dxAB
    flds    X(fb)            #  |    xb
    fsubs   X(fc)            #  |    dxBC
    flds    Y(fb)            #  |    |    yb
    fsubs   Y(fc)            #  |    |    dyBC
    flds    Y(fa)            #  |    |    |    ya
    fsubs   Y(fb)            #  |    |    |    dyAB
    fld     %st(3)           #  |    |    |    |    dxAB
    fmul    %st(2), %st      #  |    |    |    |    t0         t0=dxAB*dyBC
    fld     %st(3)           #  |    |    |    |    |    dxBC
    fmul    %st(2), %st      #  |    |    |    |    |    t1    t1=dxBC*dyAB
    mov     fifoFree(gc), hw # load gc->state.fifoFree
.AIF \&STATS EQ 1
    inc     tmpy               # _GlideRoot.stats.trisProcessed++#
    mov  tmpy, (_GlideRoot+trisProcessed)
.AENDI
    mov  (_GlideRoot+curTriSize), tmpy # load _GlideRoot.curTriSize
# 69-12
                                #  dxAB dxBC dyBC dyAB t0   t1
    fsubrp  %st, %st(1)         #  |    |    |    |    area

    sub    tmpy, hw             # fifoFree = gc->fifoFree - gc->curTriSize#
    jge    nostall              # if (fifoFree < 0)
    push   %eax
    push   %ecx
    push   %edx
    push   tmpy
    call   _grSpinFifo          # fifoFree = grSpinFifo(gc->curTriSize)#
    add    $4, %esp
    mov    %eax, hw              # fifoFree = ...
    pop    %edx
    pop    %ecx
    pop    %eax
nostall:
# 52 with direct fall thru
# jmp ret_pop5f      #==============<<<<<<<==================
    mov    hw, fifoFree(gc)   #  gc->fifoFree = fifoFree#
    fsts   zArea              #   2 clocks for fst
    mov    reg_ptr(gc), hw    # hw = gc->reg_ptr
    mov    zArea, tmpy        # j = *(long *)&area
    add    $0x00200000, hw    # hw = SST_WRAP(hw,128)
    and    $0x7fffffff, tmpy  # if ((j & 0x7FFFFFFF) == 0)
    jz     zero_area
# 76-13 zero area (gbench -tZ)
# 57 with directy fall thru

#--------------------------------------------------------------------------
    fdivrs   One                 # ooa = 1.0f / area# takes 20-38 clks
Cull_by_area_sign:
    sal     $31, tmpx                    # culltest<<31
    mov     cull_mode(gc), tmpy     # load gc->state.cull_mode
    test    tmpy, tmpy                  # if (gc->state.cull_mode #= GR_CULL_DISABLE)
    je      nocull1
                                        # culling ENABLED
    mov     zArea, tmpy                 # reload area
    xor     tmpx, tmpy                   # if (j ^ (culltest<<31))
    jge     backfaced

.align 4
nocull1:                             # culling DISABLED
    mov     X(fa), tmpx              # load fa[X]
    mov     Y(fa), tmpy              # load fa[Y]
    GR_SET  hw,FVAX, tmpx
				     # GR_SETF( hw->FvA.x, fa[X] )
    GR_SET  hw,FVAY, tmpy
				     # GR_SETF( hw->FvA.y, fa[Y] )
    mov     X(fb), tmpx              # load fb[X]
    mov     Y(fb), tmpy              # load fb[Y]
    GR_SET  hw,FVBX,tmpx
			             # GR_SETF( hw->FvB.x, fb[X] )
    GR_SET  hw,FVBY,tmpy
				     # GR_SETF( hw->FvB.y, fb[Y] )
    mov     X(fc), tmpx              # load fc[X]
    mov     Y(fc), tmpy              # load fc[Y]
    GR_SET  hw,FVCX,tmpx
			             # GR_SETF( hw->FvC.x, fc[X] )
    GR_SET  hw,FVCY,tmpy
			             # GR_SETF( hw->FvC.y, fc[Y] )

    mov     dataList(gc), i          # i = gc->datalist[0]->i
    add     $ dataList, dlp            # dlp = gc->dataList
# 96-13
    test    i, i                     # while (i) early out test
    je      no_interpolation 
# 99-13 flat shaded (gbench -t)
# jmp ret_pop5f      #==============<<<<<<<==================

#  dxBC *= ooa#
#  dyAB *= ooa#
#  dxAB *= ooa#
#  dyBC *= ooa#
merge1:                         # Stack looks like          
                                #   dxAB  dxBC  dyBC  dyAB  ooa
    fmul    %st, %st(4)         #   DXAB  |     |     |     |
    fmul    %st, %st(3)         #   |     DXBC  |     |     |
    fmul    %st, %st(2)         #   |     |     DYBC  |     |
    fmulp   %st, %st(1)         #   |     |     |     DYAB
    fxch    %st(3)              #   DYAB  |     |     DXAB  
# 105-12
    fstps    zdxAB              #   |     |     DYBC
    fstps    zdyBC              #   |     DXBC
    fstps    zdxBC              #   DYAB
    fstps    zdyAB              #
# 112-3
# jmp ret_pop0f

# NOTE: we know i>0 since we checked for that up above
#       so we fall right into the parameter gradient compute loop
###################################
#
#    dpdx = dpAB * dyBC - dpBC * dyAB
#    dpdy = dpBC * dxAB - dpAB * dxBC
#
#       dpAB.dyBC = p0x
#       dpBC.dyAB = p1x
#       dpBC.dxAB = p0y
#       dpAB.dxBC = p1y
#
#  This piece uses eax, ebx, ecx, edx, edi, esi, ebp, 
#
.align 4
next_parm:
    mov     dl_addr(dlp), hw         # fp = dlp->addr
    mov     (fa, i), tmpy             # tmpy = fa[i]
# IF 1
    test    $1, i                      # if (i & 1) {
    je      no_packer_fix
    test    $2, i                      # if (i & 2)
    je      no_p6_1
    P6Fence
				       # if (i & 2) P6FENCE
    
no_p6_1:
    GR_SET  hw,0,$0
		                       # GR_SETF( *fp, 0.0f )#
    je      no_p6_2
    P6Fence
	                               # if (i & 2) P6FENCE
no_p6_2:
    add     $ SIZEOF_dataList, dlp          # dlp++#
    mov     dl_i(dlp, SIZEOF_dataList), i   # i = dlp[1]->i
    test    i,i                             # while (i)
    jne     next_parm
    mov     dl_i(dlp, SIZEOF_dataList), tmpy # tmpy = dlp[1]->i # preload next i
    movl    (_GlideRoot+trisDrawn), fa       #  _GlideRoot.stats.trisDrawn++
    jmp     dotri
# ENDIF
.align 4
no_packer_fix:                            #   dpAB dpBC 
    flds    (fa, i)                       #   pa
    fsubs   (fb, i)                       #   dpAB
    flds    (fb, i)                       #   |    pb
    fsubs   (fc, i)	                  #   dpAB dpBC
    fld     %st(1)                        #   |    |    dpAB   
    fmuls   zdyBC                         #   |    |    p0x
    fld     %st(1)                        #   |    |    |    dpBC
    fmuls   zdyAB                         #   |    |    |    p1x
    fxch    %st(3)                        #   p1x  |    |    dpAB
    GR_SET  hw,0,tmpy
			                  #   |    |    |    |
    fmuls   zdxBC                         #   |    |    |    p1y
    fxch    %st(2)                        #   |    p1y  |    dpBC
    fmuls   zdxAB                         #   |    |    |    p0y
    fxch    %st(3)                        #   p0y  |    |    p1x
    fsubrp   %st, %st(1)                  #   |    |    dpdx
    fxch    %st(2)                        #   dpdx |    p0y
    fsubp  %st, %st(1)                    #   |    dpdy
    fxch    %st(1)                        #   dpdy dpdx
    add     $ SIZEOF_dataList, dlp        # dlp++
    mov     dl_i(dlp, SIZEOF_dataList), i # i = dlp[1]->i
    GR_SETF_P hw,FDPDX_OFFSET	
				          #   |
    GR_SETF_P hw,FDPDY_OFFSET   

				          #   empty
    test    i,i                           # while (i)
    jne     next_parm                     # 

    mov    dl_i(dlp, SIZEOF_dataList), tmpy # tmpy = dlp[1]->i - for the sake of a fall-thru
    movl   (_GlideRoot+trisDrawn), fa    #  _GlideRoot.stats.trisDrawn++
.align 4
#
# fell through - this means we're done with differentials
# note: i is preloaded with next token
# write area, pop fp stack, pop stack, return
dotri:
    add     $ SIZEOF_dataList, dlp        # dlp++
    mov     dl_addr(dlp), hw         # fp = dlp->addr
#    mov    dl_addr(dlp, SIZEOF_dataList), hw # fp = dlp[1]->addr
.AIF \&STATS EQ 1
    inc    fa                             #  _GlideRoot.stats.trisDrawn++
    movl   fa, (_GlideRoot+trisDrawn)
.AENDI
    mov    zArea, tmpx

    
    test   $2, tmpy                      # if (i & 2)
    je     no_p6_3
    P6Fence
		                          # if (i & 2) P6FENCE
    
no_p6_3:
    GR_SET  hw,0,tmpx
			                  # GR_SET( hw->FtriangleCMD, area )#
    je      no_p6_4
    P6Fence
	                                  # if (i & 2) P6FENCE
no_p6_4:

    pop     %ebp
    pop     %edi
    pop     %esi
    pop     %ebx
    mov     $1, %eax                      # return 1 (triangle drawn)
    ret

#-----------------------------------------------------------------------------
# We get here if there is no interpolation to be done. Just set area to 
# draw the triangle. Unload fp stack, stack, return.
.align 4
no_interpolation:
        mov     zArea, tmpx                       # reload area
        mov     dl_i(dlp, SIZEOF_dataList), tmpy  # load up the CPU Type
        and     $2, tmpy                          # Check for P6
        je      nofence1
        P6Fence
		                         # Fence off the triangleCmd
        nop
.align 4
nofence1:
        GR_SET  hw,FTRIANGLECMD,tmpx
					    # hit the triangleCmd 
        je      nofence2
        P6Fence
		                         # Fence off the triangleCmd
        nop
.align 4
nofence2:
        fstp   %st(0)
        fstp   %st(0)
        fstp   %st(0)
        fstp   %st(0)
        fstp   %st(0)
.AIF \&STATS EQ 1
        movl   (_GlideRoot+trisDrawn), fa  #  _GlideRoot.stats.trisDrawn++
.AENDI
        pop    %ebp
.AIF \&STATS EQ 1
        inc    fa                          #  _GlideRoot.stats.trisDrawn++
        movl    fa, (_GlideRoot+trisDrawn)
.AENDI
        pop    %edi
        pop    %esi
        pop    %ebx
        mov    $1, %eax                 # return 1 (triangles drawn)
        ret
 
# These labels should only be jumped to when we are trying to rigorously
# measure times.  Screwing up the floating point stack costs many clocks, 
# so we want to jump to the place where only the correct number of FSTPs are
# executed.

.align 4
backfaced:
        fstp   %st(0)
        fstp   %st(0)
        fstp   %st(0)
        fstp   %st(0)
        fstp   %st(0)
        mov    $0x0ffffffff, %eax         # return -1 (backface culled)
        pop    %ebp
        pop    %edi
        pop    %esi
        pop    %ebx
        ret

ret_pop6f:
        fstp   %st(0)

.align 4
zero_area:
# If triangle area is zero, nothing to do...
# but unload fp stack and then pop the stack, and return.

ret_pop5f:                              # dyBC dyAB    dxBC     dxAB   area
        fstp   %st(0)
ret_pop4f:
        fstp   %st(0)
ret_pop3f:
        fstp   %st(0)
ret_pop2f:  
        fstp   %st(0)
ret_pop1f:
        fstp   %st(0)
ret_pop0f:
        mov    $0, %eax                 # return 0 (triangles drawn)
        pop    %ebp
        pop    %edi
        pop    %esi
        pop    %ebx
        ret
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
#      P55c-200/P5-180:(2 * 6) + 17 = 29 clocks##
#      P5-166/P5-150:  (2 * 5) + 12 = 22 clocks#
#      P5-120:         (2 * 4) +  9 = 17 clocks#
#      P5-100/P5-90    (2 * 3) +  6 = 12 clocks    
#

#_TEXT ENDS
.END
