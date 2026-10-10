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
;; Revision 1.1.1.1.8.3  2004/11/25 19:11:02  koolsmoky
;; set up 3dnow! triangle/vertex list routines
;;
;; Revision 1.1.1.1.8.2  2004/10/04 09:19:25  dborca
;; handle STDCALL correctly
;;
;; Revision 1.1.1.1.8.1  2003/11/03 13:34:29  dborca
;; Voodoo2 happiness (DJGPP & Linux)
;;
;; Revision 1.1.1.1  1999/12/07 21:42:35  joseph
;; Initial checkin into SourceForge.
;;
;; 
;; 1     10/08/98 11:30a Brent
;; 
;; 5     10/07/98 9:43p Peter
;; triangle procs for 3DNow!(tm)
;; 
;; 4     9/21/98 7:02p Dow
;; 3DNow! (tm) mods
;; 
;; 3     7/24/98 1:41p Hohn
;; 
;; 2     6/05/98 5:14p Atai
;; added condition code for win/clip and aa/non-aa
;; 
;; 7     5/18/98 12:16p Peter
;; culling enabling
; 
; 6     1/15/98 1:12p Peter
; dispatch w/o packing
; 
; 5     11/06/97 3:47p Peter
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

extrn   _GlideRoot
extrn   _FifoMakeRoom
    
%MACRO GR_FIFO_WRITE 3
    mov     [%1 + %2], %3
%ENDMACRO ; GR_FIFO_WRITE

%MACRO WRITE_MM1_FIFO_ALIGNED 1

; 3DNow!
%ifdef GL_AMD3D
    movq      [fifo+%1], mm1        ; store current param | previous param
%endif

%ENDMACRO ; WRITE_MM1_FIFO_ALIGNED

%MACRO WRITE_MM1LOW_FIFO 0

; 3DNow
%ifdef GL_AMD3D
    movd      [fifo], mm1           ; store current param | previous param
%endif

%ENDMACRO ; WRITE_MM1LOW_FIFO

segment		SEG_DATA
    One         DD  1.0
    Area        DD  0

;;; Definitions of cvg regs and glide root structures.
%INCLUDE "fxgasm.h"

;; enables/disables trisProcessed and trisDrawn counters
%define STATS 1

; Arguments (STKOFF = 16 from 4 pushes)
STKOFF  equ 16
_va$    equ  4 + STKOFF
_vb$    equ  8 + STKOFF
_vc$    equ 12 + STKOFF

    ;; coordinate offsets into vertex.
    ;; NB:  These are constants and are not
    ;;	    user settable like the rest of the
    ;;	    parameter offset. Weird.
X       equ 0
Y       equ 4

segment		SEG_CONST
T2003  DD  12288.0      ; 12288
T2005  DD  1.0          ; 1
T2006  DD  256.0        ; 256

%MACRO PROC_TYPE 1
    %IFDEF GL_AMD3D
        proc _trisetup_3DNow_%1, 12
    %ELSE
        proc _trisetup_Default_%1, 12
    %ENDIF
%ENDM

;--------------------------------------------------------------------------

segment		SEG_TEXT

            ALIGN  32

PROC_TYPE cull

%define GLIDE_CULLING       1
%define GLIDE_PACK_RGB      0
%define GLIDE_PACK_ALPHA    0
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB
%undef GLIDE_CULLING

endp

            ALIGN  32

PROC_TYPE Default

%define GLIDE_CULLING       0
%define GLIDE_PACK_RGB      0
%define GLIDE_PACK_ALPHA    0
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB
%undef GLIDE_CULLING

endp

%IFDEF GL_AMD3D
	    ALIGN   32
proc _trisetup_clip_coor_thunk, 12

%define gc	eax		; Current graphics context
%define procPtr edx
%define vPtr	ecx
    
    ;; Call through to the gc->curArchProcs.drawTrianglesProc w/o
    ;; adding extra stuff to the stack. I wish we could actually
    ;; do a direct return here w/o too much work.
    lea	    vPtr, [esp + _va$ - STKOFF]	; Get vertex pointer address
    mov     gc, [_GlideRoot + curGC]; GR_DCL_GC

    ;; If debugging make sure that we're in clip coordinates
%IFDEF GLIDE_DEBUG
    test    dword [gc + CoordinateSpace], 1
    jnz	    __clipSpace
    xor	    eax, eax
    mov	    [eax], eax
__clipSpace:    
%ENDIF ; GLIDE_DEBUG

    mov	    procPtr, [gc + drawTrianglesProc]; Prefetch drawTriangles proc addr

    invoke  procPtr, 1, 3, vPtr	; (*gc->curArchProcs.drawTrianglesProc)(grDrawVertexArray, 3, vPtr)

    ret				; pop 3 dwords (vertex addrs) and return
endp

%ENDIF ; GL_AMD3D
