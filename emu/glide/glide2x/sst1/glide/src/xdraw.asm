;; THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
;; PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
;; TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
;; INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
;; DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
;; THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
;; EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
;; FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
;; 
;; USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
;; RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
;; TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
;; AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
;; SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
;; THE UNITED STATES.  
;; 
;; COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
; 
; 15    3/04/97 9:10p Dow
; Neutered mutiplatform multiheaded monster.
; 
; 14    2/26/97 11:58a Jdt
; Fixed stack bug
; 
; 13    2/19/97 4:42p Dow
; Fixed debug build for Watcom
; 
; 12    1/18/97 11:50p Dow
; Added support for GMT's register debugging
; 
; 11    12/23/96 1:37p Dow
; chagnes for multiplatform glide
; 
; 10    11/15/96 5:11p Jdt
; SST-96 exclusion
;;
;;

%include "xos.inc"
        
extrn   _GlideRoot
extrn   _grSpinFifo
        
; some useful floating load and store macros <ala gmt>
%define flds    fld  DWORD
%define fsubs   fsub DWORD
%define fmuls   fmul DWORD

segment		SEG_DATA
    One         DD  1.0
    Area        DD  0
    dxAB        DD  0
    dxBC        DD  0
    dyAB        DD  0
    dyBC        DD  0
    culltest    DD  0
    P6FenceVar  DD  0

; Ugly, but seems to workaround the problem with locally defined
; data segment globals not getting relocated properly when using
; djgpp.

%define zArea   One+04h
%define zdxAB   One+08h
%define zdxBC   One+0ch
%define zdyAB   One+10h
%define zdyBC   One+14h
%define zculltest One+18h

;;; Some useful SST-1 offsets
%include "fxgasm.h"

;; enables/disables trisProcessed and trisDrawn counters
%define STATS 1

;--------------------------------------------------------------------------        
;;; Macro for P6 Fencing operation
;;; Note that this destroys eax, but if you do 2 fences eax will be restored
%ifdef GLIDE_DEBUG
extrn _grFence

%macro P6Fence 0
    push    eax
    push    ecx
    push    edx
    call    _grFence
    pop     edx
    pop     ecx
    pop     eax
    xchg    eax, [P6FenceVar]
%endmacro
%else
%macro P6Fence 0
    xchg    eax, [P6FenceVar]
%endmacro
%endif

%ifdef GLIDE_HARDWARE
%ifdef GLIDE_DEBUG

extrn _GR_SET
extrn _GR_SETF

%endif

%macro GR_SET 3
%ifdef GLIDE_DEBUG
    push    eax
    push    ecx
    push    edx

    push    %3
    lea     eax,[%1 + %2]
    push    eax
    call    _GR_SET
    add     esp,8

    pop     edx
    pop     ecx
    pop     eax
%endif
    mov     DWORD [%1 + %2], %3
%endmacro

%macro GR_SETF_P 2
%ifdef GLIDE_DEBUG
    push    eax
    push    ecx
    push    edx

    fst    DWORD [esp-4]
    sub     esp,4
    lea     eax,[%1 + %2]
    push    eax
    call    _GR_SETF
    add     esp,8

    pop     edx
    pop     ecx
    pop     eax
%endif
    fstp    DWORD [%1 + %2]
%endmacro

%endif

;--------------------------------------------------------------------------        
; Arguments (STKOFF = 16 from 4 pushes)
STKOFF  equ 16
_va$    equ  4 + STKOFF
_vb$    equ  8 + STKOFF
_vc$    equ 12 + STKOFF

X       equ 0
Y       equ 4

%define fa      eax       ; vtx a from caller
%define fb      ebx       ; vtx b from caller
%define fc      ecx       ; vtx c from caller
                                ; edx is used as index, loading from *src
%define gc      esi       ; points to graphics context
%define dlp     esi       ; points to dataList structure
%define hw      edi       ; points to the hardware

%define tmpx    edx       ; temp X storage
%define i       edx       ; i = dlp->i
%define tmpy    ebp       ; temp Y storage

segment		SEG_TEXT

;--------------------------------------------------------------------------        

            align 4
%if XOS != XOS_WATCD ; gdraw.c special-cases __WATCOMC__ , for what reason???
proc grDrawTriangle, 12
endp
%endif
%if XOS == XOS_WIN32
%ifdef __MINGW32__
; GNU LD fails with '_' prefix
export  grDrawTriangle@12
%else
export _grDrawTriangle@12
%endif
%endif

; FALL THRU to _trisetup

;--------------------------------------------------------------------------        
;;
;;  _trisetup_asm - do triangle setup & send down draw command
;;
;;  USAGE:
;;
;;
            align 4
proc _trisetup_asm, 12
; 28
    ; save ebx, esi, edi, ebp
    push    ebx
     push    esi

    push    edi
     push    ebp

;vertex sorting is done as follows:
;   1. First, load the y values of vtx a, b, c into integer registers 
;
;       This is done in the segment vertex_load below.
;
;       Note that this trashes the pointers, we will reload them later
;
;       If any of the y values is negative, convert to a pseudo-2's complement
;       form by xor'ing with 7fffffff. This lets us use 2's complement integer
;       comparisons for floating point comparisons.
;
;   2. Compare a.y, b.y, c.y, and setup pointers fa, fb, fc so that
;       a.y <= b.y <= c.y.
;
;       Note that fa, fb, fc registers point once more to vertex structs,
;       though not necessarily in the order that the application passes it to
;       this routine.
;
;       This is done in the segment vertex_y_sort below, with many many 
;       compare/branches.
;
;       All the compare/branches converge to sst_load_xy_regs.
;
;   3. Load the (x,y) values into the floating point SST registers for Fa, Fb,
;       and Fc. Done in the segment sst_load_xy_regs.
;
;   4. Restore fa, fb, fc to the user's order. Also save the vertex
;       with lowest y value on the stack, this will be used later for 
;       loading parameter values into the SST regs.
;

;--------------------------------------------------------------------------        
    mov     fa, [esp + _va$]    ; 1
     mov     fb, [esp + _vb$]
    mov     fc, [esp + _vc$]
     mov     tmpy, [_GlideRoot + trisProcessed]    ; _GlideRoot.stats.trisProcessed++;
; 36-3
vertex_y_load:
    mov     fa, [fa + Y]        ; 2
     mov     fb, [fb + Y]
    cmp     fa, 0               ; 3
     jge     a_positive
    xor     fa, 7fffffffh 
            align 4
a_positive:
    cmp     fb, 0               ; 4
     jge     b_positive
    xor     fb, 7fffffffh 
            align 4
b_positive:
    mov     fc, [fc + Y]        ; 5
     GET_GC
    cmp     fc, 0               ; 6
     jge     c_positive
    xor     fc, 7fffffffh
c_positive:
; 42-3
; 24-30 with direct fall-thru
; jmp ret_pop0f
;--------------------------------------------------------------------------        
vertex_y_sort:

    cmp     fa, fb              ; if (ay < by) {
     jge      AgtB
    cmp     fb, fc              ; if (by > cy) {
     jle     abc
    cmp     fa, fc              ; if (ay < cy) {
     jge     cab

  acb:                          ; acb
        mov     fa, [esp + _va$]
         mov     tmpx, [gc + cull_mode] ; culltest = gc->state.cull_mode
        mov     fb, [esp + _vc$]
         mov     fc, [esp + _vb$]
        xor     tmpx, 1                 ; culltest ^= 1
         jmp      Area_Computation

            align 4
  cab:                          ; cab
        mov fa, [esp + _vc$]            ; fa = &vc->x
         mov tmpx, [gc + cull_mode]     ; culltest = gc->state.cull_mode
        mov fb, [esp + _va$]            ; fb = &va->x
         mov fc, [esp + _vb$]           ; fc = &vb->x
                                                ; HOLE
         jmp Area_Computation

            align 4
  abc:                          ; abc
        mov fa, [esp + _va$]            ; fa = &va->x
         mov fb, [esp + _vb$]           ; fb = &vb->x
        mov fc, [esp + _vc$]            ; fc = &vc->x
         mov tmpx, [gc + cull_mode]     ; culltest = gc->state.cull_mode
                                                ; HOLE
        jmp Area_Computation

            align 4
 AgtB:                          ; } else {
        cmp     fb, fc          ;   if (by < cy) 
         jge     cba
        cmp     fa, fc          ;   if (ay < cy)
         jge      bca
  bac:                          ; bac
        mov     fa, [esp + _vb$]
         mov     tmpx, [gc + cull_mode] ; culltest = gc->state.cull_mode
        mov     fb, [esp + _va$]
         mov     fc, [esp + _vc$]
        xor     tmpx, 1                 ; culltest ^= 1
         jmp     Area_Computation

            align 4
  cba:                          ; cba
        mov     fa, [esp + _vc$]
         mov     tmpx, [gc + cull_mode] ; culltest = gc->state.cull_mode
        mov     fb, [esp + _vb$]
         mov     fc, [esp + _va$]
        xor     tmpx, 1                 ; culltest ^= 1
         jmp     Area_Computation

            align 4
  bca:                          ; bca
        mov     fa, [esp + _vb$]
         mov     fb, [esp + _vc$]
        mov     fc, [esp + _va$]
         mov     tmpx, [gc + cull_mode] ; culltest = gc->state.cull_mode
;--------------------------------------------------------------------------        
            align 4
Area_Computation:
; 47-3
; jmp ret_pop0f
    flds    [fa + X]            ;  xa
    fsubs   [fb + X]            ;  dxAB
    flds    [fb + X]            ;  |    xb
    fsubs   [fc + X]            ;  |    dxBC
    flds    [fb + Y]            ;  |    |    yb
    fsubs   [fc + Y]            ;  |    |    dyBC
    flds    [fa + Y]            ;  |    |    |    ya
    fsubs   [fb + Y]            ;  |    |    |    dyAB
    fld     st3                 ;  |    |    |    |    dxAB
    fmul    st0, st2            ;  |    |    |    |    t0         t0=dxAB*dyBC
    fld     st3                 ;  |    |    |    |    |    dxBC
    fmul    st0, st2            ;  |    |    |    |    |    t1    t1=dxBC*dyAB
    mov     hw, [gc + fifoFree] ; load gc->state.fifoFree
%ifdef STATS
     inc     tmpy               ; _GlideRoot.stats.trisProcessed++;
    mov [_GlideRoot + trisProcessed], tmpy
%endif
     mov     tmpy, [_GlideRoot + curTriSize] ; load _GlideRoot.curTriSize
; 69-12
                                ;  dxAB dxBC dyBC dyAB t0   t1
    fsubp   st1,st0             ;  |    |    |    |    area
    sub    hw,tmpy              ; fifoFree = gc->fifoFree - gc->curTriSize;
    jge    nostall              ; if (fifoFree < 0)
    push   eax
    push   ecx
    push   edx
    push   tmpy
    call   _grSpinFifo          ; fifoFree = grSpinFifo(gc->curTriSize);
    add    esp,4
    mov    hw, eax              ; fifoFree = ...
    pop    edx
    pop    ecx
    pop    eax
nostall:
; 52 with direct fall thru
; jmp ret_pop5f      ;==============<<<<<<<==================
    mov    [gc + fifoFree],hw   ;  gc->fifoFree = fifoFree;
    fst     dword [zArea]       ;   2 clocks for fst
    mov     hw, [gc + reg_ptr ] ; hw = gc->reg_ptr
     mov     tmpy, [zArea]      ; j = *(long *)&area
    add     hw, 00200000H       ; hw = SST_WRAP(hw,128)
     and    tmpy, 7fffffffh     ; if ((j & 0x7FFFFFFF) == 0)
    jz     zero_area
; 76-13 zero area (gbench -tZ)
; 57 with directy fall thru

;--------------------------------------------------------------------------        
    fdivr   dword [One]         ; ooa = 1.0f / area; takes 20-38 clks
Cull_by_area_sign:
    sal     tmpx, 31                    ; culltest<<31
     mov     tmpy, [gc + cull_mode]     ; load gc->state.cull_mode
    test    tmpy, tmpy                  ; if (gc->state.cull_mode != GR_CULL_DISABLE)
     je      nocull1
                                        ; culling ENABLED
    mov     tmpy, [zArea]               ; reload area
    xor     tmpy,tmpx                   ; if (j ^ (culltest<<31))
    jge     backfaced

            align 4
nocull1:                                ; culling DISABLED
    mov     tmpx, [fa + X]              ; load fa[X]
     mov     tmpy, [fa + Y]             ; load fa[Y]
    GR_SET  hw,FVAX,tmpx        ; GR_SETF( hw->FvA.x, fa[X] )
     GR_SET  hw,FVAY,tmpy       ; GR_SETF( hw->FvA.y, fa[Y] )
    mov     tmpx, [fb + X]              ; load fb[X]
     mov     tmpy, [fb + Y]              ; load fb[Y]
    GR_SET  hw,FVBX,tmpx        ; GR_SETF( hw->FvB.x, fb[X] )
     GR_SET  hw,FVBY,tmpy       ; GR_SETF( hw->FvB.y, fb[Y] )
    mov     tmpx, [fc + X]              ; load fc[X]
     mov     tmpy, [fc + Y]              ; load fc[Y]
    GR_SET  hw,FVCX,tmpx                ; GR_SETF( hw->FvC.x, fc[X] )
     GR_SET  hw,FVCY,tmpy               ; GR_SETF( hw->FvC.y, fc[Y] )

    mov     i, [gc+dataList]            ; i = gc->datalist[0]->i
     add     dlp, dataList              ; dlp = gc->dataList
; 96-13
    test     i, i                       ; while (i) early out test
     je      no_interpolation 
; 99-13 flat shaded (gbench -t)
; jmp ret_pop5f      ;==============<<<<<<<==================

;  dxBC *= ooa;
;  dyAB *= ooa;
;  dxAB *= ooa;
;  dyBC *= ooa;
merge1:                         ; Stack looks like          
                                ;   dxAB  dxBC  dyBC  dyAB  ooa
    fmul    st4, st0            ;   DXAB  |     |     |     |
    fmul    st3, st0            ;   |     DXBC  |     |     |
    fmul    st2, st0            ;   |     |     DYBC  |     |
    fmulp   st1, st0            ;   |     |     |     DYAB
     fxch    st3                ;   DYAB  |     |     DXAB
; 105-12
    fstp    dword [zdxAB]       ;   |     |     DYBC
    fstp    dword [zdyBC]       ;   |     DXBC
    fstp    dword [zdxBC]       ;   DYAB
    fstp    dword [zdyAB]       ;
; 112-3
; jmp ret_pop0f

; NOTE: we know i>0 since we checked for that up above
;       so we fall right into the parameter gradient compute loop
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;    dpdx = dpAB * dyBC - dpBC * dyAB
;    dpdy = dpBC * dxAB - dpAB * dxBC
;
;       dpAB.dyBC = p0x
;       dpBC.dyAB = p1x
;       dpBC.dxAB = p0y
;       dpAB.dxBC = p1y
;;
;;  This piece uses eax, ebx, ecx, edx, edi, esi, ebp, 
;
        align 4
next_parm:
    mov     hw, [dlp + dl_addr]         ; fp = dlp->addr
     mov     tmpy, [fa + i]             ; tmpy = fa[i]
%if 1
    test    i,1                         ; if (i & 1) {
     je      no_packer_fix
    test    i,2                         ; if (i & 2)
     je      no_p6_1
    P6Fence                             ; if (i & 2) P6FENCE
    
  no_p6_1:
    GR_SET  hw,0,0                      ; GR_SETF( *fp, 0.0f );
     je      no_p6_2
    P6Fence                             ; if (i & 2) P6FENCE
  no_p6_2:
    mov     i, [dlp + SIZEOF_dataList + dl_i] ; i = dlp[1]->i
     add     dlp, SIZEOF_dataList       ; dlp++;
    test    i,i                         ; while (i)
     jne     next_parm
    mov     tmpy, [dlp + SIZEOF_dataList + dl_i] ; tmpy = dlp[1]->i ; preload next i
     mov    fa, [_GlideRoot + trisDrawn];  _GlideRoot.stats.trisDrawn++
    jmp     dotri

        align 4
no_packer_fix:                          ;   dpAB dpBC 
%endif
    flds    [fa + i]                    ;   pa
    fsubs   [fb + i]                    ;   dpAB
    flds    [fb + i]                    ;   |    pb
    fsubs   [fc + i]                    ;   dpAB dpBC 

    fld     st1                         ;   |    |    dpAB
    fmuls   [zdyBC]                     ;   |    |    p0x
    fld     st1                         ;   |    |    |    dpBC
    fmuls   [zdyAB]                     ;   |    |    |    p1x
     fxch    st3                        ;   p1x  |    |    dpAB
    GR_SET  hw,0,tmpy                   ;   |    |    |    |
    fmuls   [zdxBC]                     ;   |    |    |    p1y
     fxch    st2                        ;   |    p1y  |    dpBC
    fmuls   [zdxAB]                     ;   |    |    |    p0y
     fxch    st3                        ;   p0y  |    |    p1x
    fsubp   st1,st0                     ;   |    |    dpdx
     fxch    st2                        ;   dpdx |    p0y
    fsubrp  st1,st0                     ;   |    dpdy
     fxch    st1                        ;   dpdy dpdx
    mov     i, [dlp + SIZEOF_dataList + dl_i] ; i = dlp[1]->i
     add     dlp, SIZEOF_dataList       ; dlp++;
    GR_SETF_P hw,FDPDX_OFFSET           ;   |
    GR_SETF_P hw,FDPDY_OFFSET           ;   empty
    test    i,i                         ; while (i)
     jne     next_parm                  ; 

    mov    tmpy, [dlp + SIZEOF_dataList + dl_i] ; tmpy = dlp[1]->i - for the sake of a fall-thru
     mov    fa, [_GlideRoot + trisDrawn];  _GlideRoot.stats.trisDrawn++
        align 4
;
; fell through - this means we're done with differentials
; note: i is preloaded with next token
; write area, pop fp stack, pop stack, return
dotri:
    mov    hw, [dlp + SIZEOF_dataList + dl_addr] ; fp = dlp[1]->addr
%ifdef STATS
     inc    fa                          ;  _GlideRoot.stats.trisDrawn++
    mov    [_GlideRoot + trisDrawn], fa
%endif
     mov    tmpx, [zArea]
    
    test    tmpy,2                      ; if (i & 2)
     je      no_p6_3
    P6Fence                             ; if (i & 2) P6FENCE
    
  no_p6_3:
    GR_SET  hw,0,tmpx                   ; GR_SET( hw->FtriangleCMD, area );
     je      no_p6_4
    P6Fence                             ; if (i & 2) P6FENCE
  no_p6_4:

    pop     ebp
     pop     edi
    pop     esi
     pop     ebx
    mov     eax, 1h                     ; return 1 (triangle drawn)
     ret

;-----------------------------------------------------------------------------
;; We get here if there is no interpolation to be done. Just set area to 
;; draw the triangle. Unload fp stack, stack, return.
            align 4
no_interpolation:
        mov     tmpx, [zArea]           ; reload area
         mov     tmpy, [dlp + SIZEOF_dataList + dl_i]        ; load up the CPU Type
        and     tmpy, 2                 ; Check for P6
         je      nofence1
        P6Fence                         ; Fence off the triangleCmd
         nop
            align 4
nofence1:
        GR_SET  hw,FTRIANGLECMD,tmpx    ; hit the triangleCmd 
         je      nofence2
        P6Fence                         ; Fence off the triangleCmd
         nop
            align 4
nofence2:
        fstp    st0
        fstp    st0
        fstp    st0
        fstp    st0
        fstp    st0
%ifdef STATS
        mov    fa, [_GlideRoot + trisDrawn];  _GlideRoot.stats.trisDrawn++
%endif
        pop     ebp
%ifdef STATS
         inc    fa                          ;  _GlideRoot.stats.trisDrawn++
        mov    [_GlideRoot + trisDrawn], fa
%endif
         pop     edi
        pop     esi
         pop     ebx
        mov     eax, 1h                 ; return 1 (triangles drawn)
         ret
 
; These labels should only be jumped to when we are trying to rigorously
; measure times.  Screwing up the floating point stack costs many clocks, 
; so we want to jump to the place where only the correct number of FSTPs are
; executed.

            align 4
backfaced:
        fstp    st0
        fstp    st0
        fstp    st0
        fstp    st0
        fstp    st0
        mov     eax, 0ffffffffh         ; return -1 (backface culled)
         pop     ebp
        pop     edi
         pop     esi
        pop     ebx
         ret

ret_pop6f:
        fstp    st0

            align 4
zero_area:
; If triangle area is zero, nothing to do...
; but unload fp stack and then pop the stack, and return.

ret_pop5f:                              ; dyBC dyAB    dxBC     dxAB   area
        fstp    st0
ret_pop4f:
        fstp    st0
ret_pop3f:
        fstp    st0
ret_pop2f:  
        fstp    st0
ret_pop1f:
        fstp    st0
ret_pop0f:
        mov     eax, 0h                 ; return 0 (triangles drawn)
         pop     ebp
        pop     edi
         pop     esi
        pop     ebx
         ret
        
endp

; [++++ from above]
; This comment was moved here to make the code in the loop more readable 
; STALL HERE due to PCI write buffer being full on last instruction:        
; The write to the packerHackAddress takes 3 bus clocks, during which
; we may not write to the PCI buffer without stalling.  This causes
; the amount of clocks the workaround adds to the loop to vary in the
; following way++:
;
;    CPU          Bus/CPU Clock     Total Bus       Total Penalty
;                     Ratio*      Clocks Since   (add to later clocks)
;======================================================================  
; P55c-200/P5-180:    6/1              1.3       (3 - 1.3) * 6 + 6 = 17 
; P5-166/P5-150:      5/1              1.8       (3 - 1.8) * 5 + 6 = 12 
; P5-120:             4/1              2.25      (3 - 2.25) * 4 + 6 = 9 
; P5-100/P5-90:       3/1               3        (3 - 3) * 3 + 6 = 6
;
; * P5-200 haa a 33MHz bus, P5-166 has a 33MHz bus, 
;   P5-150 has a 30MHz bus, P5-120 has a 30MHz bus,
;   P5-100 has a 33MHz bus, P5-90 has a 30MHz bus
;
;++ These calculations also assume that we NEVER get to continue a burst 
;   from having the next parameter be consecutive from the current one.
;   If we did get that, then we have to add two more bus clocks to the
;   penalty for the next write (labeled loop clock 10), so that looks like
;      CPU                  Total Penalty
;      ============================================        
;      P55c-200/P5-180:(2 * 6) + 17 = 29 clocks!!!
;      P5-166/P5-150:  (2 * 5) + 12 = 22 clocks!!
;      P5-120:         (2 * 4) +  9 = 17 clocks!
;      P5-100/P5-90    (2 * 3) +  6 = 12 clocks    
;
