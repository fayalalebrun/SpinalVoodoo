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
;; Revision 1.1.1.1.2.4  2005/04/23 18:36:36  koolsmoky
;; fixed 3dnow! and mmx optimizations
;;
;; Revision 1.1.1.1.2.3  2005/01/22 14:52:02  koolsmoky
;; enabled packed argb for cmd packet type 3
;;
;; Revision 1.1.1.1.2.2  2005/01/13 16:11:39  koolsmoky
;; prepare for packed rgb
;;
;; Revision 1.1.1.1.2.1  2004/12/23 20:45:56  koolsmoky
;; converted to nasm syntax
;; added x86 asm, 3dnow! triangle and mmx, 3dnow! texture download optimizations
;;
;; Revision 1.1.1.1  1999/12/07 21:49:13  joseph
;; Initial checkin into SourceForge.
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

;;; Definitions of cvg regs and glide root structures.
%include "fxgasm.h"

extrn   _GlideRoot
extrn   _FifoMakeRoom, 12

%MACRO GR_FIFO_WRITE 3
    mov     [%1 + %2], %3
%ENDMACRO ; GR_FIFO_WRITE

%MACRO WRITE_MM1_FIFO_ALIGNED 0
  %ifdef GL_AMD3D
    movq      [fifo], mm1           ; store current param | previous param
  %else
    ;;
  %endif
%ENDMACRO ; WRITE_MM1_FIFO_ALIGNED

%MACRO WRITE_MM1LOW_FIFO 0
  %ifdef GL_AMD3D
    movd      [fifo], mm1           ; store current param | previous param
  %else
    ;;
  %endif
%ENDMACRO ; WRITE_MM1LOW_FIFO

segment		SEG_DATA
    One         DD  1.0
    Area        DD  0
%IF GLIDE_PACKED_RGB
    bias0       DD  0
    bias1       DD  0
%ENDIF

segment		SEG_CONST
$T2003  DD  12288.0
$T2005  DD  1.0
$T2006  DD  256.0

;;; Arguments (STKOFF = 16 from 4 pushes)
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

%MACRO PROC_TYPE 1
  %ifdef GL_AMD3D
    proc %1_3DNow, 12
  %else
    proc %1, 12
  %endif
%ENDMACRO ; PROC_TYPE

;; enables/disables trisProcessed and trisDrawn counters
%define STATS 1

segment		SEG_TEXT

            ALIGN 32
PROC_TYPE _trisetup_cull

%define GLIDE_CULLING	    1
%define GLIDE_PACK_RGB	    0
%define GLIDE_PACK_ALPHA    0
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB
%undef GLIDE_CULLING

endp

%IF GLIDE_PACKED_RGB

            ALIGN 32
PROC_TYPE _trisetup_cull_rgb

%define GLIDE_CULLING	    1
%define GLIDE_PACK_RGB	    1
%define GLIDE_PACK_ALPHA    0
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB
%undef GLIDE_CULLING

endp

            ALIGN 32
PROC_TYPE _trisetup_cull_argb

%define GLIDE_CULLING	    1
%define GLIDE_PACK_RGB	    1
%define GLIDE_PACK_ALPHA    1
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB
%undef GLIDE_CULLING

endp
%ENDIF ; GLIDE_PACKED_RGB

            ALIGN 32
PROC_TYPE _trisetup

%define GLIDE_CULLING	    0
%define GLIDE_PACK_RGB	    0
%define GLIDE_PACK_ALPHA    0
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB
%undef GLIDE_CULLING

endp

%IF GLIDE_PACKED_RGB

            ALIGN 32
PROC_TYPE _trisetup_rgb

%define GLIDE_CULLING	    0
%define GLIDE_PACK_RGB	    1
%define GLIDE_PACK_ALPHA    0
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB
%undef GLIDE_CULLING

endp

            ALIGN 32
PROC_TYPE _trisetup_argb

%define GLIDE_CULLING	    0
%define GLIDE_PACK_RGB	    1
%define GLIDE_PACK_ALPHA    1
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB
%undef GLIDE_CULLING

endp

%ENDIF ; GLIDE_PACKED_RGB
