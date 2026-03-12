;;
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
;;
;; Revision 1.1.1.1.2.1  2004/12/23 20:56:08  koolsmoky
;; builds without asm optimizations (USE_X86=1)
;;
;; Revision 1.1.2.1  2004/03/02 07:55:29  dborca
;; Bastardised Glide3x for SST1
;;
;; Revision 1.1.1.1  1999/12/07 21:48:51  joseph
;; Initial checkin into SourceForge.
;;
; 
; 2     3/04/97 9:10p Dow
; Neutered mutiplatform multiheaded monster.
;;
;;

%include "xos.inc"

;;      Data for data segment goes here
;_DATA   SEGMENT DWORD USE32 PUBLIC 'DATA'; 
;_DATA   ENDS
    
;;; Some useful constants
; CPU Type
CPUTypeUnknown  equ 0ffffffffh
CPUTypePrePent  equ 4h
CPUTypeP5       equ 5h
CPUTypeP6       equ 6h
        
;;; References to external data:
    
segment		SEG_TEXT
;;
;;  _cpu_detect_asm - detect the type of CPU 
;; 
;;  USAGE:
;;
;;  int __cdecl _cpu_detect_asm(void);
;;
;;  returns 4 for non-pen

proc _cpu_detect_asm
P6Stuff:
    pushad                              ; save all regs.

    ; First, determine whether CPUID instruction is available.
    ; If it's not, then it's a 386 or 486.
    pushfd                              ; push original EFLAGS.
    pop eax                             ; pop into eax
    mov ecx, eax                        ; save original EFLAGS in ecx
    xor eax, 0200000h                   ; flip ID bit in EFLAGS
    push    eax                         ; put it back on stack
    popfd                               ; pop into EFLAGS
    pushfd                              ; get EFLAGS back
    pop eax                             ; into eax
    xor eax, ecx                        ; check to see if we could toggle ID
    jz  NotPentium                      ; Sorry, not P5 or P6.
    
    ;
    ; Now determine whether it's an intel P6 CPU.
    ;
    ;;  Is it an Intel CPU?
    xor eax, eax                        ; eax = 0.
    cpuid                               ; get cpuid
    xor ebx, 0756e6547h                 ; "Genu"
    jnz NotIntel
    xor edx, 049656e69h                 ; "ineI"
    jnz NotIntel
    xor ecx, 06c65746eh                 ; "ntel"
    jnz NotIntel                        ;

    ;;  Verifying architecture family
    mov eax, 1
    cpuid                               ; get family/model/stepping
    shr eax, 8                          ; rid of model & stepping number
    and eax, 0fh                        ; use only family
    cmp eax, 6
    jl     IsP5                         ; It's a P5
    ;; Else it's a P6
    ;
    ; Intel P6 processor. 
    ; Make sure it supports Memory Type Range Request registers
    ;
IsP6:   
    popad
    mov eax, 6                          ; 
    ret                                 ; return 

IsP5:   
    popad
    mov eax, 5                          ; 
    ret

NotPentium:
    popad
    mov eax, 4
    ret

NotIntel:
    popad
    mov eax, 0ffffffffh
    ret
        
endp


;------------------------------------------------------------------------------   
; this routine sets the precision to single
; which effects all adds, mults, and divs
    align 4                 ; 
proc single_precision_asm
    push  eax       ; make room
    fnclex          ; clear pending exceptions    
    fstcw WORD [esp]
    mov   eax, DWORD [esp]
    and   eax, 0000fcffh  ; clear bits 9:8
    mov   DWORD [esp], eax
    fldcw WORD [esp]
    pop   eax
    ret
endp

;------------------------------------------------------------------------------   
; this routine sets the precision to double
; which effects all adds, mults, and divs
    align 4                 ; 
proc double_precision_asm
    push  eax       ; make room
    fnclex          ; clear pending exceptions    
    fstcw WORD [esp]
    mov   eax, DWORD [esp]
    and   eax, 0000fcffh  ; clear bits 9:8
    or    eax, 000002ffh  ; set 9:8 to 10
    mov   DWORD [esp], eax
    fldcw WORD [esp]
    pop   eax
    ret
endp
