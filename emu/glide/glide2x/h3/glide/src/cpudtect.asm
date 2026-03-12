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
;; Revision 1.1.1.1.2.1  2006/11/30 09:16:50  guillemj
;; Switch to the new "build system", to be in sync with the rest of glide.
;; Backport cpudtect.asm from glide3 for h3.
;; Add fpu.c to cvg.
;;
;; Revision 1.1.1.1.8.1  2003/11/07 13:38:38  dborca
;; unite the clans
;;
;; Revision 1.1.1.1  1999/11/24 21:44:53  joseph
;; Initial checkin for SourceForge
;;
;; 
;; 4     4/06/99 3:54p Dow
;; Alt tab again.
;; 
;; 9     3/10/99 10:40a Peter
;; detect katmai-ness
;; 
;; 8     1/20/99 5:50p Peter
;; norbert's code for k6 and k7 (?) wc
;; 
;; 7     12/09/98 1:11p Peter
;; cpu detection from Norbert for teh rest of the 3DNow!(tm) partners
;; 
;; 6     10/03/98 3:35p Peter
;; First pass at dynamic detection of 3DNow!(tm)
;; 
;; 5     7/24/98 1:40p Hohn
; 
; 4     5/28/97 8:23a Peter
; Merge w/ original glide source
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
    push   esi                          ; save registers that are not volatile
    push   edi
    push   ebx
    push   ebp

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
    ;;      eax - type[13:12] family[11:8] model[7:4] revision[3:0]
    ;;      edx - feature bits
    mov eax, 1
    cpuid                               ; get family/model/stepping
    
    shr eax, 8                          ; rid of model & stepping number
    and eax, 0fh                        ; use only family
    cmp eax, 6
    jl     IsP5                         ; It's a P5
    ;; Else it's a P6
    
    ; Intel P6 processor. 
    ;; feature bits are in edx from the cpuid[1]
IsP6:
    ; Make sure it supports Memory Type Range Request registers (bit 12)    
    mov     ebx, edx
    test    ebx, 1000h

    ;; Hmmmm... p6 w/o mtrr's?
    jz      IsP5

    ;; Check for katmai-ness (bit 25)
    test    edx, 2000000h
    jz      noKNI

    mov     eax, 7
    jmp     DoneCpu                         ; return
    
noKNI:      
    mov     eax, 6                          ; 
    jmp     DoneCpu                         ; return 

IsP5:   
    mov     eax, 5                          ; 
    jmp     DoneCpu

NotPentium:
    mov     eax, 4

DoneCpu:
    pop     ebp
    pop     ebx
    pop     edi
    pop     esi
    ret


NotIntel:

    ;; This is a non-Intel processor. Figure out whether it supports
    ;; both MMX and 3DNow!, in which case we can use Norbert's cool
    ;; MMX/3DNow!(tm) code
    ;;
    ;; The return value is split into two 16-bit fields. Bits [31:16]
    ;; identify the processor vendor as follows:
    ;;
    ;; 8001h   AMD
    ;; 8002h   Cyrix
    ;; 8003h   IDT
    ;;
    ;; Bits [15:0] identify processor features as follows:
    ;;
    ;; 0001h   MMX
    ;; 0002h   3DNow! (tm)
    ;; 0004h   K6-style MTRRs
    ;; 0008h   PentiumII-style MTRRs

    xor esi, esi     ; default feature flags
    xor edi, edi     ; default extended feature flags

    ;; Test whether extended feature function is supported

    mov eax, 80000000h
    cpuid
    cmp eax, 80000000h
    jbe NoExtendedFeatures

    ;; execute extended feature function

    mov eax, 80000001h
    cpuid
    mov edi, edx

NoExtendedFeatures:

    ;; execute standard feature function

    mov eax, 1
    cpuid
    mov esi, edx
    mov ebp, eax           ; save family/model/stepping

    ;; get the vendor string 
 
    mov eax, 0
    cpuid

ChkAMD:
    cmp ebx, 68747541h     ; 'htuA'
    jne ChkCyrix
    cmp edx, 69746E65h     ; 'itne'
    jne ChkCyrix
    cmp ecx, 444D4163h     ; 'DMAc'
    je  CPUisAMD

ChkCyrix:
    cmp ebx, 69727943h     ; 'iryC'                   
    jne ChkIDT
    cmp edx, 736E4978h     ; 'snIx'
    jne ChkIDT
    cmp ecx, 64616574h     ; 'deat'
    je  CPUisCyrix

ChkIDT:
    cmp ebx, 746E6543h     ; 'tneC'
    jne UnknownVendor
    cmp edx, 48727561h     ; 'Hrua'
    jne UnknownVendor
    cmp ecx, 736C7561h     ; 'slua'
    jne UnknownVendor

CPUisIDT:
    mov  eax, 80030000h    ; vendor = IDT, features = none
    test esi, 00800000h    ; check for MMX bit in features
    jz   DoneCpu
    or   eax, 1            ; set MMX feature flag
    test edi, 80000000h    ; check for 3DNow! bit in extended features
    jz   DoneCpu
    or   eax, 2            ; set 3DNow! feature flag
    jmp  DoneCpu

CPUisAMD:
    mov  eax, 80010000h    ; vendor = AMD, features = none
    mov  edx, ebp          ; family/model/stepping information
    and  edx, 00000FFFh    ; extract family/model/stepping
    cmp  edx, 00000588h    ; CXT, Sharptooth, or K7 ?
    jb   AmdMTRRchkDone    ; nope, definitely no MTRRs
    cmp  edx, 00000600h    ; K7 or better ?
    jb   AmdHasK6MTRR      ; nope, but supports K6 MTRRs
    or   eax, 8            ; set P2_MTRR feature flag
    jmp  AmdMTRRchkDone    ;
AmdHasK6MTRR:
    or   eax, 4            ; set K6_MTRR feature flag
AmdMTRRchkDone:
    test esi, 00800000h    ; check for MMX bit in features
    jz   DoneCpu
    or   eax, 1            ; set MMX feature flag
    test edi, 80000000h    ; check for 3DNow! bit in extended features
    jz   DoneCpu
    or   eax, 2            ; set 3DNow! feature flag
    jmp  DoneCpu

CPUisCyrix:
    mov  eax, 80020000h    ; vendor = Cyrix, features = none
    test esi, 00800000h    ; check for MMX bit in features
    jz   DoneCpu
    or   eax, 1            ; set MMX feature flag
    test edi, 80000000h    ; check for 3DNow! bit in extended features
    jz   DoneCpu
    or   eax, 2            ; set 3DNow! feature flag
    jmp  DoneCpu

UnknownVendor:
    mov  eax, 0ffffffffh
    jmp  DoneCpu

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
