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
;; Revision 1.1.8.11  2007/05/13 09:21:06  koolsmoky
;; removed problematic context checks
;;
;; Revision 1.1.8.10  2004/05/04 17:04:31  koolsmoky
;; clean up
;;
;; Revision 1.1.8.9  2003/11/07 13:38:39  dborca
;; unite the clans
;;
;; Revision 1.1.8.8  2003/09/12 05:11:45  koolsmoky
;; preparing for graphic context checks
;; fixed jmp errors
;;
;; Revision 1.1.8.7  2003/07/07 23:29:06  koolsmoky
;; cleaned logs
;;
;;
;; Revision 1.1  2000/06/15 00:27:43  joseph
;; Initial checkin into SourceForge.
;; 
;; 12    4/05/99 11:34a Atai
;; added GLIDE_ALT_TAB for xdraw2.inc to query context in the retail build
;; 
;; 11    3/19/99 11:26a Peter
;; expose direct fifo for gl
;; 
;; 10    10/14/98 12:05p Peter
;; fixed my effed up assumption about non-volatile regs
;; 
;; 9     10/12/98 9:51a Peter
;; dynamic 3DNow!(tm)
;; 
;; 8     9/24/98 11:17a Dow
;; AMD 3DNow! (tm) mods
;; 
;; 7     8/30/98 1:34p Dow
;; State & other optimizations
;; 
;; 6     8/29/98 8:12p Dow
;; Clip optimization
;; 
;; 5     7/01/98 8:41a Jdt
;; removed gc arg from trisetup funcs
;; 
;; 4     8/03/98 6:36a Jdt
;; Add GC to trisetup arglist
;; 
;; 3     6/09/98 11:59a Atai
;; 1. update glide header
;; 2. fix cull mode
;; 3. fix tri stats
;; 
;; 8     5/18/98 3:21p Peter
;; dynamic culling changes
; 
; 6     1/15/98 1:12p Peter
; dispatch w/o packing
; 
; 5     11/06/97 3:47p Peter
; dispatch code w/ simulator
; 
; 4     11/04/97 5:04p Peter
; cataclysm part deux
; 
; 3     11/01/97 10:01a Peter
; tri dispatch stuff
; 
; 2     10/30/97 6:53p Peter
; first real cut at tri asm
; 
; 1     10/30/97 4:29p Peter
; asm tri code
; 
; 2     7/07/97 2:14p Jdt
; assembly now on par with C code.
; 
; 1     7/07/97 8:37a Jdt
; B4 Chip field fix.
;;

%include "xos.inc"


;;; Definitions of cvg regs and glide root structures.
%INCLUDE "fxgasm.h"

extrn _GlideRoot
extrn _grCommandTransportMakeRoom, 12
extrn _grValidateState

%IFDEF HAL_CSIM
extrn halStore32, 8
%ENDIF
    
%MACRO GR_FIFO_WRITE 3
%IFDEF HAL_CSIM
    pushad
    pushfd
    
;    push    %3
 ;   mov     eax, %1
  ;  add     eax, %2
   ; push    eax
    ;call    halStore32

%ifidni %3, eax
    lea     edx, [%1 + %2]
    invoke  halStore32, edx, %3
%else
    lea     eax, [%1 + %2]
    invoke  halStore32, eax, %3
%endif

    popfd
    popad
%ELSE
    mov     [%1 + %2], %3
%ENDIF
%ENDMACRO ; GR_FIFO_WRITE

%MACRO WRITE_MM1_FIFO_ALIGNED 1

; 3DNow!
%ifdef GL_AMD3D

%IFDEF HAL_CSIM
    movd      tempVal, mm1          ; previous param
    GR_FIFO_WRITE fifo, %1, tempVal
    punpckhdq mm1, mm1              ; current param
    movd      tempVal, mm1          ;
    GR_FIFO_WRITE fifo, %1 + 4, tempVal
%ELSE
    movq      [fifo+%1], mm1        ; store current param | previous param
%ENDIF

%endif

; SSE
%ifdef GL_SSE

%IFDEF HAL_CSIM
    ; csim isn't complete yet
    movss     tempVal,xmm1          ; previous param - no can do
    GR_FIFO_WRITE fifo, %1, tempVal
    unpcklps  xmm1, xmm1            ; current param
    movss     tempValm,xmm1         ; - no can do
    GR_FIFO_WRITE fifo, %1 + 4, tempVal
%ELSE
    movlps    [fifo+%1],xmm1        ; store current param | previous param
%ENDIF

%endif

%ENDMACRO ; WRITE_MM1_FIFO_ALIGNED

%MACRO WRITE_MM1LOW_FIFO 0

; 3DNow
%ifdef GL_AMD3D

%IFDEF HAL_CSIM
    movd      tempVal, mm1          ; previous param
    GR_FIFO_WRITE fifo, 0, tempVal  ;
%ELSE
    movd      [fifo], mm1           ; store current param | previous param
%ENDIF

%endif

; SSE
%ifdef GL_SSE

%IFDEF HAL_CSIM
    ; csim isn't complete yet
    movss     tempVal,xmm1          ; previous param - no can do
    GR_FIFO_WRITE fifo, 0, tempVal  ;
%ELSE
    movss     [fifo],xmm1           ; store current param | previous param ?shouldn't this be movlps?
%ENDIF

%endif

%ENDMACRO ; WRITE_MM1LOW_FIFO

segment		SEG_DATA
    One         DD  1.0
    Area        DD  0

segment		SEG_CONST
$T2003  DD  12288.0
$T2005  DD  1.0
$T2006  DD  256.0

; Arguments (STKOFF = 16 from 4 pushes)
STKOFF  equ 16
_va$    equ  4 + STKOFF
_vb$    equ  8 + STKOFF
_vc$    equ 12 + STKOFF

    ;; coordinate offsets into vertex.
    ;; NB:  These are constants and are not
    ;;      user settable like the rest of the
    ;;      parameter offset. Weird.
X       equ 0
Y       equ 4

%MACRO PROC_TYPE 1
    %IFDEF GL_AMD3D
        proc _trisetup_3DNow_%1, 12
    %ELIFDEF GL_SSE
        proc _trisetup_SSE_%1, 12
    %ELSE
        proc _trisetup_Default_%1, 12
    %ENDIF
%ENDMACRO

;; enables/disables trisProcessed and trisDrawn counters
%define STATS 1

    ;; NB:  All of the base triangle procs expect to have the gc
    ;;      passed from the caller in edx so that we can avoid
    ;;      the agi from the far pointer. Screw w/ this at your
    ;;      own peril.
    ;;
    ;;      YOU HAVE BEEN WARNED

;--------------------------------------------------------------------------

segment		SEG_TEXT

            ALIGN    32
PROC_TYPE clip_nocull_invalid
           
%define GLIDE_VALIDATE_STATE 1
%define GLIDE_CLIP_COORDS    1
%define GLIDE_CULLING        0
%define GLIDE_PACK_RGB       0
%define GLIDE_PACK_ALPHA     0
%define GLIDE_GENERIC_SETUP  0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING
%undef GLIDE_CLIP_COORDS
%undef GLIDE_VALIDATE_STATE

endp

            ALIGN  32
PROC_TYPE clip_cull_invalid

%define GLIDE_VALIDATE_STATE 1
%define GLIDE_CLIP_COORDS    1
%define GLIDE_CULLING        1
%define GLIDE_PACK_RGB       0
%define GLIDE_PACK_ALPHA     0
%define GLIDE_GENERIC_SETUP  0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP      
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING
%undef GLIDE_CLIP_COORDS
%undef GLIDE_VALIDATE_STATE

endp

            ALIGN    32
PROC_TYPE clip_cull_valid

%define GLIDE_VALIDATE_STATE 0
%define GLIDE_CLIP_COORDS    1
%define GLIDE_CULLING        1
%define GLIDE_PACK_RGB       0
%define GLIDE_PACK_ALPHA     0
%define GLIDE_GENERIC_SETUP  0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP      
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING
%undef GLIDE_CLIP_COORDS
%undef GLIDE_VALIDATE_STATE

endp

            ALIGN    32
PROC_TYPE clip_nocull_valid

%define GLIDE_VALIDATE_STATE 0
%define GLIDE_CLIP_COORDS    1
%define GLIDE_CULLING        0
%define GLIDE_PACK_RGB       0
%define GLIDE_PACK_ALPHA     0
%define GLIDE_GENERIC_SETUP  0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP      
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING
%undef GLIDE_CLIP_COORDS
%undef GLIDE_VALIDATE_STATE

endp

            ALIGN    32
PROC_TYPE win_nocull_invalid
         
%define GLIDE_VALIDATE_STATE 1
%define GLIDE_CLIP_COORDS    0
%define GLIDE_CULLING        0
%define GLIDE_PACK_RGB       0
%define GLIDE_PACK_ALPHA     0
%define GLIDE_GENERIC_SETUP  0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP      
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING
%undef GLIDE_CLIP_COORDS
%undef GLIDE_VALIDATE_STATE

endp

            ALIGN    32
PROC_TYPE win_cull_invalid
  
%define GLIDE_VALIDATE_STATE 1
%define GLIDE_CLIP_COORDS    0
%define GLIDE_CULLING        1
%define GLIDE_PACK_RGB       0
%define GLIDE_PACK_ALPHA     0
%define GLIDE_GENERIC_SETUP  0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING
%undef GLIDE_CLIP_COORDS
%undef GLIDE_VALIDATE_STATE

endp

            ALIGN    32
PROC_TYPE win_cull_valid

%define GLIDE_VALIDATE_STATE 0
%define GLIDE_CLIP_COORDS    0
%define GLIDE_CULLING        1
%define GLIDE_PACK_RGB       0
%define GLIDE_PACK_ALPHA     0
%define GLIDE_GENERIC_SETUP  0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP      
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING
%undef GLIDE_CLIP_COORDS
%undef GLIDE_VALIDATE_STATE

endp
    
            ALIGN    32
PROC_TYPE win_nocull_valid

%define GLIDE_VALIDATE_STATE 0
%define GLIDE_CLIP_COORDS    0
%define GLIDE_CULLING        0
%define GLIDE_PACK_RGB       0
%define GLIDE_PACK_ALPHA     0
%define GLIDE_GENERIC_SETUP  0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING
%undef GLIDE_CLIP_COORDS
%undef GLIDE_VALIDATE_STATE

endp

%IFDEF GL_AMD3D GL_SSE

  ALIGN   32

%ifdef GL_AMD3D
  proc _trisetup_clip_coor_thunk, 12
%else ; GL_SSE
  proc _trisetup_SSE_clip_coor_thunk, 12
%endif

%define procPtr eax
%define vPtr    ecx
%define gc      edx           ; Current graphics context passed implicitly through edx

%IFDEF GLIDE_ALT_TAB
    test gc, gc
    je   .__contextLost
;    mov  eax, [gc + windowed]
;    test eax, 1
;    jnz  .pastContextTest
    mov  eax, DWORD [gc+lostContext]
    mov  ecx, [eax]
    test ecx, 1
    jnz  .__contextLost
;.pastContextTest:
%ENDIF
    
    ;; Call through to the gc->curArchProcs.drawTrianglesProc w/o
    ;; adding extra stuff to the stack. I wish we could actually
    ;; do a direct return here w/o too much work.
    lea     vPtr, [esp + _va$ - STKOFF]         ; Get vertex pointer address
    mov     procPtr, [gc + drawTrianglesProc]   ; Prefetch drawTriangles proc addr

    ;; If debugging make sure that we're in clip coordinates
%IFDEF GLIDE_DEBUG
    test    dword [gc + CoordinateSpace], 1
    jnz     __clipSpace
    xor     eax, eax
    mov     [eax], eax
__clipSpace:    
%ENDIF ; GLIDE_DEBUG

    invoke  procPtr, 1, 3, vPtr ; (*gc->curArchProcs.drawTrianglesProc)(grDrawVertexArray, 3, vPtr)

%IFDEF GLIDE_ALT_TAB
.__contextLost:
%ENDIF
    ret                         ; pop 3 dwords (vertex addrs) and return
endp

%ENDIF ; GL_AMD3D GL_SSE
