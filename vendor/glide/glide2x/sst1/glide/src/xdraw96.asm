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
; 2     7/07/97 2:14p Jdt
; assembly now on par with C code.
; 
; 1     7/07/97 8:37a Jdt
; B4 Chip field fix.
;;

%include "xos.inc"
        
extrn   _GlideRoot
extrn   _grSpinFifo
extrn   _grSst96FifoMakeRoom

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
extrn   _GR_SET_GW_CMD
extrn   _GR_SET_GW_HEADER
extrn   _GR_SET_GW_ENTRY
%endif

%macro GR_SET_GW_CMD 3
%ifdef GLIDE_DEBUG
    push eax
    push ecx
    push edx

    push %3
    lea  eax, [%1 + %2]
    push eax
    call _GR_SET_GW_CMD
    add  esp, 8
    pop  edx
    pop  ecx
    pop  eax
%else
    mov [%1 + %2], %3
%endif
%endmacro

%macro GR_SET_GW_HEADER 3
%ifdef GLIDE_DEBUG
    push eax
    push ecx
    push edx
    push %3
    lea  eax, [%1 + %2]
    push eax
    call _GR_SET_GW_HEADER
    add  esp, 8
    pop  edx
    pop  ecx
    pop  eax
%else
    mov [%1 + %2], %3
%endif
%endmacro

%macro GR_SET_GW_ENTRY 3
%ifdef GLIDE_DEBUG
    push eax
    push ecx
    push edx
    push %3
    lea  eax, [%1 + %2]
    push eax
    call _GR_SET_GW_ENTRY
    add  esp, 8
    pop  edx
    pop  ecx
    pop  eax
%else
    mov [%1 + %2], %3
%endif
%endmacro

%macro GR_FSET_GW_ENTRY 2
%ifdef GLIDE_DEBUG
    push eax
    push ecx
    push edx
    sub  esp, 4
    fstp DWORD [esp]
    lea  eax, [%1 + %2]
    push eax
    call _GR_SET_GW_ENTRY
    add  esp, 8
    pop  edx
    pop  ecx
    pop  eax
%else
    fstp DWORD [%1 + %2]
%endif
%endmacro

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
%define fifo    edi       ; points to next entry in fifo

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
    fsubp   st1,st0             ;  |    |    |    |    area

    fst     dword [zArea]       ;  |    |    |    |    area

    ; Zero Area Triangle Check
    mov     tmpy, [zArea]      ; j = *(long *)&area
    and     tmpy, 7fffffffh    ; if ((j & 0x7FFFFFFF) == 0)
    jz      zero_area

    ; Cull Check
    sal     tmpx, 31                    ; culltest<<31
     mov     tmpy, [gc + cull_mode]     ; load gc->state.cull_mode
    test    tmpy, tmpy                  ; if (gc->state.cull_mode != GR_CULL_DISABLE)
     je      nocull1
                                        ; culling ENABLED
    mov     tmpy, [zArea]               ; reload area
    xor     tmpy,tmpx                   ; if (j ^ (culltest<<31))
    jge     backfaced

nocull1:                        ; culling disabled
    ; OOA Calculation
    fdivr   dword [One]         ;  |    |    |    |    ooa

    ; Fetch Fifo Ptr
    mov     fifo, [gc + fifoPtr];

    ; P6 Check
    mov     tmpx, [_GlideRoot + CPUType]
    cmp     tmpx, 6h
    
    je     fence

        align 4
fenceDone:  
    ; Wrap Check
    mov     tmpx, [_GlideRoot + curTriSize]
    mov     tmpy, [gc + fifoSize]

    sub     tmpy, tmpx
    jl      wrap

        align 4
wrapDone:
    mov     [gc + fifoSize], tmpy
    ; Adjust Fifo Ptr
    mov     tmpy, fifo
    add     tmpy, tmpx
    mov     [gc + fifoPtr], tmpy

    ; Begin Tri Packet
    mov     tmpx, [gc + gwCommand]
    mov     tmpy, [gc + gwHeaders]

    GR_SET_GW_CMD    fifo, 0, tmpx
    GR_SET_GW_HEADER fifo, 4, tmpy

    ; Download X, Y * 3
    mov     tmpx, [fa + X]
    mov     tmpy, [fa + Y]

    GR_SET_GW_ENTRY fifo, 8,  tmpx
    GR_SET_GW_ENTRY fifo, 12, tmpy

    mov     tmpx, [fb + X]
    mov     tmpy, [fb + Y]

    GR_SET_GW_ENTRY fifo, 16, tmpx
    GR_SET_GW_ENTRY fifo, 20, tmpy

    mov     tmpx, [fc + X]
    mov     tmpy, [fc + Y]

    GR_SET_GW_ENTRY fifo, 24, tmpx
    GR_SET_GW_ENTRY fifo, 28, tmpy

    add     fifo, 32

    mov     i, [gc+dataList]            ; i = gc->datalist[0]->i
    add     dlp, dataList               ; dlp = gc->dataList

    ; Early out for no interp
    test    i, i
    je      no_interpolation

    ; Setup for Parameter Calculator
                                ;   dxAB  dxBC  dyBC  dyAB  ooa
    fmul    st4, st0            ;   DXAB  |     |     |     |
    fmul    st3, st0            ;   |     DXBC  |     |     |
    fmul    st2, st0            ;   |     |     DYBC  |     |
    fmulp   st1, st0            ;   |     |     |     DYAB
     fxch    st3                ;   DYAB  |     |     DXAB
    fstp    dword [zdxAB]       ;   |     |     DYBC
    fstp    dword [zdyBC]       ;   |     DXBC
    fstp    dword [zdxBC]       ;   DYAB
    fstp    dword [zdyAB]       ;

    ; Parameter Calculator
        align 4
next_parm:
    test    i,1
    jnz     secondary_packet

    mov     tmpy, [fa + i]              ; tmpy = fa[i]
    flds    [fa + i]                    ;   pa
    fsubs   [fb + i]                    ;   dpAB
    flds    [fb + i]                    ;   |    pb
    fsubs   [fc + i]                    ;   dpAB dpBC 

    fld     st1                         ;   |    |    dpAB
    fmuls   [zdyBC]                     ;   |    |    p0x
    fld     st1                         ;   |    |    |    dpBC
    fmuls   [zdyAB]                     ;   |    |    |    p1x
     fxch    st3                        ;   p1x  |    |    dpAB
    GR_SET_GW_ENTRY fifo, 0, tmpy       ;   |    |    |    |
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
    GR_FSET_GW_ENTRY fifo, 4            ;   |
    GR_FSET_GW_ENTRY fifo, 8            ;   empty
    add     fifo, 12
    test    i,i                         ; while (i)
     jne     next_parm                  ; 

        align 4
triangle_command:
    ; Write Triangle Command
    mov     tmpx, [zArea]
    GR_SET_GW_ENTRY fifo, 0, tmpx
    test    fifo, 7h
    jnz     no_padding0
    GR_SET_GW_ENTRY fifo, 4, tmpx
no_padding0:
    ; Cleanup
    pop     ebp
     pop     edi
    pop     esi
     pop     ebx
    mov     eax, 1h                     ; return 1 (triangle drawn)
     ret
    
        align 4
zero_area:
backfaced:
    fstp    st0   ; 4
    fstp    st0   ; 3
    fstp    st0   ; 2
    fstp    st0   ; 1
    fstp    st0   ; 0
    pop     ebp
     pop     edi
    pop     esi
     pop     ebx
    xor     eax, eax                    ; return 0 (triangle drawn)
     ret

        align 4
wrap:
    push    eax
    push    ecx
    push    edx
    call    _grSst96FifoMakeRoom
    pop     edx
    pop     ecx
    pop     eax
    mov     fifo, [gc + fifoPtr]
    mov     tmpy, [gc + fifoSize]
    sub     tmpy, tmpx
    jmp     wrapDone

        align 4
fence:
    mov     tmpx, [gc + writesSinceFence];
    mov     tmpy, [_GlideRoot + curTriSize]
    
    shr     tmpy, 2
    sub     tmpx, tmpy
    jl      dofence

    mov     [gc + writesSinceFence], tmpx
    jmp     fenceDone

        align 4
dofence:
    push    eax
    xchg    eax, [P6FenceVar]
    pop     eax
    jmp     fenceDone

        align 4
secondary_packet:
    test    fifo, 7h
    jz      no_padding1
    GR_SET_GW_ENTRY  fifo, 0, tmpx 
    add     fifo, 4
        align 4
no_padding1:
    push    tmpx
    push    gc
    
    mov     tmpx, [dlp + dl_addr]
    GET_GC

    GR_SET_GW_CMD  fifo, 0, tmpx
    mov     tmpy, [gc + gwHeaders + 4]  
    
    GR_SET_GW_HEADER fifo, 4, tmpy
    pop     gc

    pop     tmpx
    add     fifo, 8

    mov     i, [dlp + SIZEOF_dataList + dl_i]
     add    dlp, SIZEOF_dataList
        align 4
next_parm_1:
    mov     tmpy, [fa + i]              ; tmpy = fa[i]
    flds    [fa + i]                    ;   pa
    fsubs   [fb + i]                    ;   dpAB
    flds    [fb + i]                    ;   |    pb
    fsubs   [fc + i]                    ;   dpAB dpBC 

    fld     st1                         ;   |    |    dpAB
    fmuls   [zdyBC]                     ;   |    |    p0x
    fld     st1                         ;   |    |    |    dpBC
    fmuls   [zdyAB]                     ;   |    |    |    p1x
     fxch    st3                        ;   p1x  |    |    dpAB
    GR_SET_GW_ENTRY fifo, 0, tmpy       ;   |    |    |    |
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
    GR_FSET_GW_ENTRY fifo, 4            ;   |
    GR_FSET_GW_ENTRY fifo, 8            ;   empty
    add     fifo, 12
    test    i,i                         ; while (i)
    jne     next_parm_1;

    test    fifo, 7h
    jz      triangle_command_packet
    GR_SET_GW_ENTRY fifo, 0, tmpx
    add     fifo, 4
        align 4
triangle_command_packet:
    GET_GC
    mov     tmpy, 40000000h

    mov     tmpx, [gc + gwCommand]
    mov     fa, [zArea]

    GR_SET_GW_CMD    fifo, 0, tmpx
    GR_SET_GW_HEADER fifo, 4, tmpy

    GR_SET_GW_ENTRY  fifo, 8,  fa
    GR_SET_GW_ENTRY  fifo, 12, fa

    pop     ebp
     pop     edi
    pop     esi
     pop     ebx
    mov     eax, 1h                     ; return 1 (triangle drawn)
     ret

        align 4
no_interpolation:
    fstp    st0     ; 4
    fstp    st0     ; 3
    fstp    st0     ; 2
    fstp    st0     ; 1
    fstp    st0     ; 0
    jmp     triangle_command

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
