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
;; 3     3/17/99 6:17p Dow
;; Phantom Menace and other fixes.
;; 
;; 10    10/09/98 6:57p Peter
;; dynamic 3DNow!(tm) mods
;; 
;; 9     9/21/98 5:35p Dow
;; 3DNow! mods
;; 
;; 8     5/18/98 3:21p Peter
;; dynamic culling changes
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

extrn _GlideRoot
extrn _FifoMakeRoom

segment		SEG_DATA
    One         DD  1.0
    Area        DD  0

%if GLIDE_PACKED_RGB
    bias0   DD  0
    bias1   DD  0
%endif

; Ugly, but seems to workaround the problem with locally defined
; data segment globals not getting relocated properly when using
; djgpp.

%define zArea   One+04h

;;; Definitions of cvg regs and glide root structures.
%include "fxgasm.h"

; Arguments (STKOFF = 16 from 4 pushes)
STKOFF  equ 16
_va$    equ  4 + STKOFF
_vb$    equ  8 + STKOFF
_vc$    equ 12 + STKOFF

X       equ 0
Y       equ 4

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

%IF GLIDE_PACKED_RGB

PROC_TYPE cull_rgb

%define GLIDE_CULLING       1
%define GLIDE_PACK_RGB      1
%define GLIDE_PACK_ALPHA    0
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP      
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING

endp

            ALIGN  32
PROC_TYPE cull_argb

%define GLIDE_CULLING       1
%define GLIDE_PACK_RGB      1
%define GLIDE_PACK_ALPHA    1
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP      
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING

endp
%ENDIF ; GLIDE_PACKED_RGB

            ALIGN   32
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

            ALIGN  32

%IF GLIDE_PACKED_RGB    

PROC_TYPE rgb

%define GLIDE_CULLING       0
%define GLIDE_PACK_RGB      1
%define GLIDE_PACK_ALPHA    0
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP      
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING

endp

            ALIGN  32
PROC_TYPE argb

%define GLIDE_CULLING       0
%define GLIDE_PACK_RGB      1
%define GLIDE_PACK_ALPHA    1
%define GLIDE_GENERIC_SETUP 0
%INCLUDE "xdraw2.inc"
%undef GLIDE_GENERIC_SETUP      
%undef GLIDE_PACK_ALPHA
%undef GLIDE_PACK_RGB           
%undef GLIDE_CULLING

endp
%ENDIF ; GLIDE_PACKED_RGB
