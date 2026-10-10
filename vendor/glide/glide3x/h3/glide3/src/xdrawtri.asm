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

;; Adapted from gdraw.c:grDrawTriangle() for nasm

%include "xos.inc"

%if XOS == XOS_WIN32
%else
%error This source is only meant for x86/Windows
%endif

%include "fxgasm.h"

; tls constants (from fxglide.h)
%define W95_TEB_PTR        0x18
%define W95_TEB_TLS_OFFSET 0x88
%define WNT_TEB_PTR        0x18
%define WNT_TEB_TLS_OFFSET 0xE10

extrn   _GlideRoot

segment SEG_TEXT

            align 4
proc grDrawTriangle, 12

	mov		eax, dword [fs:WNT_TEB_PTR]
	add		eax, dword [_GlideRoot+tlsOffset]
	mov		edx, dword [eax]
	test		edx, edx
	je		L0
	mov		eax, dword [edx+lostContext]
	test		eax, eax
	je		L0
	mov		eax, dword [eax]
	test		eax, 1
	jnz		L0
	mov		eax, dword [edx+kTriProcOffset]
	jmp		eax
L0:	; lostContext
; MSVC doesn't emit a RET here in gdraw.obj, which seems wrong.
	ret

endp

%ifdef __MINGW32__
; GNU LD fails with '_' prefix
export  grDrawTriangle@12
%else
export _grDrawTriangle@12
%endif
