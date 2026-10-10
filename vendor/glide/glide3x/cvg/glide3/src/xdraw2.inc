;; -*-asm-*-
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
;; Revision 1.1.1.1.8.2  2005/01/13 15:46:50  koolsmoky
;; prepare for packed rgb
;;
;; Revision 1.1.1.1.8.1  2003/11/03 13:34:29  dborca
;; Voodoo2 happiness (DJGPP & Linux)
;;
;; Revision 1.1.1.1  1999/12/07 21:42:35  joseph
;; Initial checkin into SourceForge.
;;
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

;;--------------------------------------------------------------------------
;; AMD3D version
;;--------------------------------------------------------------------------

%ifdef GL_AMD3D

%define gc      edi           ; points to graphics context
%define fifo    ebp           ; points to fifo entries
%define tempVal esi

    ;; Prologue stuff
    push      edi                   ; save caller's register variable
    mov       gc,[_GlideRoot+curGC]; GR_DCL_GC

    push      ebp                   ; save frame pointer
    push      ebx                   ; save caller's register variable        
    
%IF GLIDE_CULLING
%define fa      eax           ; vtx a from caller
%define fb      ebx           ; vtx b from caller
%define fc      ecx           ; vtx c from caller
    
%define cull    edx           ; cull mode
%define intArea ecx           ; area temp storage
    
    mov       fb, [esp + _vb$ - 4]  ; get base address of vertex B
    push      esi                   ; save caller's register variable
    
    mov       cull, [gc + cull_mode]; get cull mode
    mov       fc, [esp + _vc$]      ; get base address of vertex C

    femms                           ; will use AMD3D, clear FPU/MMX registers

    cmp       cull, 0               ; culling enabled ?
    mov       tempVal, [_GlideRoot + curTriSize]

    ;; Cull Check
    jz        .nocull                ; nope, no culling
    mov       fa, [esp + _va$]      ; get base address of vertex A

    movq      mm2, [fc + X]         ; yc | xc
    shl       cull, 31              ; culltest << 31

    movq      mm1, [fb + X]         ; yb | xb
    add       tempVal, 4            ; space required in fifo

    movq      mm0, [fa + X]         ; ya | xa
    mov       ebx, [gc + fifoRoom]  ; space available in fifo

    ;; Area_Computation

    pfsubr    mm2, mm1              ; dyBC | dxBC
    pfsub     mm0, mm1              ; dyAB | dxAB

;; E3DN
;    pswapd    mm2, mm2              ; dxBC | dyBC
;    pfmul     mm0, mm2              ; dxBC*dyAB | dxAB*dyBC
;    pfnacc    mm0, mm0              ; dxAB*dyBC - dxBC*dyAB | dxAB*dyBC - dxBC*dyAB
;    movd      intArea, mm0          ; vectored !

    movq      mm5, mm2              ; dyBC | dxBC
    punpckhdq mm2, mm2              ; dyBC | dyBC

    movq      mm4, mm0              ; dyAB | dxAB
    punpckhdq mm0, mm0              ; dyAB | dyAB

    pfmul     mm5, mm0              ; dyAB*dxBC
    pfmul     mm4, mm2              ; dxAB*dyBC

    pfsub     mm4, mm5              ; dxAB*dyBC - dxBC*dyAB

    movd      intArea, mm4          ; vectored !

    ; Zero Area Triangle Check

    test      intArea, 7fffffffh    ; if ((j & 0x7FFFFFFF) == 0)
    jz        .__cullFail            ; area zero, triangle culled

    xor       intArea, cull         ; if (j ^ (culltest << 31))
    jge       .__cullFail            ; triangle facing away from viewer, culled

    cmp       ebx, tempVal          ; fifo space required >= space available ?
    jge       .__triBegin            ; yup, push out triangle data to Voodoo

    push      __LINE__              ; line number inside this function
    push      0h                    ; pointer to function name = NULL

    push      tempVal               ; fifo space required
    call      _FifoMakeRoom        ; note: updates fifoPtr

    add       esp, 12               ; remove 3 DWORD arguments from stack
    jmp       .__triBegin            ; merge back with short path

    ;; culling disabled

.nocull:
    ;; Check to make sure that we have enough room for
    ;; the complete triangle packet.

    add       tempVal, 4            ; fifo space needed
    mov       ebx, [gc + fifoRoom]  ; fifo space available

    cmp       ebx, tempVal          ; fifo spce available >= space needed ?
    jge       .__triBegin            ; yup, ready to draw triangle

    push      __LINE__              ; line number inside this function
    push      0h                    ; pointer to function name = NULL

    push      tempVal               ; fifo space needed
    call      _FifoMakeRoom        ; note: updates fifoPtr

    add       esp, 12               ; remove 3 DWORD arguments from stack
    nop                             ; filler

%ELSE   ; !GLIDE_CULLING

    lea       eax, [esp+ _va$]      ; pointer to vertex pointers    
    push      esi                   ; save caller's register variable
    
    mov       tempVal, [_GlideRoot + curTriSize] ; data for whole triangle in bytes
    mov       ebx, [gc + fifoRoom]  ; fifo space available

    add       tempVal, 4            ; fifo space needed (include 4-byte header)
    femms                           ; will use AMD3D, clear FPU/MMX registers

    cmp       ebx, tempVal          ; fifo spce available >= space needed ?
    jge       .__triBegin            ; yup, ready to draw triangle

    push      __LINE__              ; line number inside this function
    push      0h                    ; pointer to function name = NULL

    push      tempVal               ; fifo space needed
    call      _FifoMakeRoom        ; note: updates fifoPtr

    add       esp, 12               ; remove 3 DWORD arguments from stack
    nop                             ; filler
%ENDIF  ; GLIDE_CULLING


%define dlp     ebx           ; points to dataList structure
%define dlpstrt ecx           ; points to begin of dataList structure
%define vertex  edx           ; the current vertex
%define packCol esi

.__triBegin:
    mov       eax, [gc+triPacketHdr]; Packet 3 header
    lea       dlp,[gc + tsuDataList]; Reset the dataList

    mov       fifo, [gc + fifoPtr]  ; Fetch Fifo Ptr
    mov       vertex, [esp + _va$]  ; Current vertex = A

    mov       dlpstrt, dlp          ; save pointer to start of dataList
    test      fifo, 4               ; is fifo pointer qword aligned ?

    jz        .__fifo_aligned        ; yes, it is qword aligned
    movq      mm1, [vertex+X]       ; y | x

    GR_FIFO_WRITE fifo, 0, eax      ; write header to fifo; now qword aligned
    add       fifo, 12              ; fifoPtr += 3*sizeof(FxU32)

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write y | x
    nop                             ; filler

%IF GLIDE_PACK_RGB
%IF GLIDE_PACK_ALPHA
    ;; assumes color and alpha values < 256.0
    movq      mm1, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    movd      mm3, [vertex+a]       ; 0 | a
    pf2id     mm1, mm1              ; convert to integer: 000000gg | 000000rr

    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb
    pf2id     mm3, mm3              ; convert to integer: 00000000 | 000000aa

    punpcklwd mm2, mm1              ; 00000000 | 00rr00bb
    psrlq     mm1, 24               ; 00000000 | 0000gg00

    add       dlp, 8                ; skip data list entry for "a"
    psllq     mm3, 24               ; 00000000 | aa000000

    por       mm1, mm2              ; 00000000 | 00rrggbb
    por       mm1, mm3              ; 00000000 | aarrggbb
%ELSE ; !GLIDE_PACK_ALPHA
    ;; assumes color values < 256.0

    movq      mm1, [vertex+r]       ; g | r
    add       dlp, 4                ; next data list entry

    movd      mm2, [vertex+b]       ; 0 | b
    pf2id     mm1, mm1              ; convert to integer: 000000gg | 000000rr

    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb
    punpcklwd mm2, mm1              ; 00000000 | 00rr00bb

    psrlq     mm1, 24               ; 00000000 | 0000gg00
    por       mm1, mm2              ; 00000000 | 00rrggbb
%ENDIF ; !GLIDE_PACK_ALPHA

    ;; here: one DWORD in "write buffer", RGB(A)

    mov       eax, [dlp]            ; get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; end of list ?
    jz        .__paramLoopDoneWBone1 ; yes, one DWORD in "write buffer"
          
.__paramLoop1a:
    movd      mm2, [eax+vertex]     ; get next parameter
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]            ; offset = *(dlp + 1)
    add       dlp, 8                ; dlp += 2

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?
  
    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jz        .__paramLoopDoneWBzero1; exit, "write buffer" empty

    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp-4]          ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jnz       .__paramLoop1a         ; nope, copy next parameter

    nop                             ; filler
    jmp       .__paramLoopDoneWBone1 ; merge back into common stream

%ELSE ; ! GLIDE_PACK_RGB

    ;; here: "write buffer" empty

    mov       eax, [dlp]            ; Get first offset from the data list
    test      eax, eax              ; at end of list ?

    lea       dlp, [dlp+4]          ; dlp++
    jz        .__paramLoopDoneWBzero1; yes, "write buffer" empty
          
.__paramLoop1a:
    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp]            ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jz        .__paramLoopDoneWBone1 ; exit, write buffer contains one DWORD

    movd      mm2, [eax+vertex]     ; get next parameter
    add       dlp, 8                ; dlp += 2

    mov       eax, [dlp-4]          ; offset = *(dlp + 1)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jnz       .__paramLoop1a         ; nope, copy next parameter

    nop                             ; filler
    jmp       .__paramLoopDoneWBzero1; write buffer empty
    
%ENDIF ; GLIDE_PACK_RGB

.__fifo_aligned:
    movd      mm2, [vertex+X]       ; y | x of vertex A
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    movd      mm1, [gc+triPacketHdr]; Packet 3 header
    punpckldq mm1, mm2              ; x | header

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write x | header
    movd      mm1, [vertex+Y]       ; 0 | y of vertex A

%IF GLIDE_PACK_RGB
%IF GLIDE_PACK_ALPHA
    ;; assumes color and alpha values < 256.0
    movq      mm4, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    movd      mm3, [vertex+a]       ; 0 | a
    pf2id     mm4, mm4              ; convert to integer: 000000gg | 000000rr

    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb
    pf2id     mm3, mm3              ; convert to integer: 00000000 | 000000aa

    punpcklwd mm2, mm4              ; 00000000 | 00rr00bb
    psrlq     mm4, 24               ; 00000000 | 0000gg00

    add       dlp, 8                ; skip data list entry "a"
    psllq     mm3, 24               ; 00000000 | aa000000

    por       mm4, mm2              ; 00000000 | 00rrggbb
    por       mm4, mm3              ; 00000000 | aarrggbb
%ELSE ; !GLIDE_PACK_ALPHA
    ;; assumes color values < 256.0

    movq      mm4, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    pf2id     mm4, mm4              ; convert to integer: 000000gg | 000000rr
    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb

    punpcklwd mm2, mm4              ; 00000000 | 00rr00bb
    add       dlp, 4                ; next data list entry

    psrlq     mm4, 24               ; 00000000 | 0000gg00
    por       mm4, mm2              ; 00000000 | 00rrggbb
%ENDIF ; !GLIDE_PACK_ALPHA

    punpckldq mm1, mm4              ; RGB(A) | y
    mov       eax, [dlp]            ; get first offset from the data list

    WRITE_MM1_FIFO_ALIGNED 0        ; PCI write y | RGB(A)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    add       dlp, 4                ; dlp++
    test      eax, eax              ; end of list ?

    nop                             ; filler
    jz        .__paramLoopDoneWBzero1; yes, "write buffer" is empty

.__paramLoop1b:
    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp]            ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jz        .__paramLoopDoneWBone1 ; exit, write buffer contains one DWORD

    movd      mm2, [eax+vertex]     ; get next parameter
    add       dlp, 8                ; dlp += 2

    mov       eax, [dlp-4]          ; offset = *(dlp + 1)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jnz       .__paramLoop1b         ; nope, copy next parameter

    nop                             ; filler
    jmp       .__paramLoopDoneWBzero1; write buffer empty

%ELSE ; !GLIDE_PACK_RGB
    mov       eax, [dlp]            ; get first offset from the data list
    add       dlp, 4                ; dlp++

    cmp       eax, 0                ; end of list ?
    jz        .__paramLoopDoneWBone1 ; yes, "write buffer" has y data

.__paramLoop1b:
    movd      mm2, [eax+vertex]     ; get next parameter
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]            ; offset = *(dlp + 1)
    add       dlp, 8                ; dlp += 2

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?
  
    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jz        .__paramLoopDoneWBzero1; exit, "write buffer" empty

    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp-4]          ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jnz       .__paramLoop1b         ; nope, copy next parameter
%ENDIF

.__paramLoopDoneWBone1:

    ;; here: "write buffer" has one DWORD left over from vertex A

    mov       dlp, dlpstrt          ; reset the dataList
    mov       vertex, [esp + _vb$]  ; Current vertex = B

    movd      mm2, [vertex+X]       ; 0 | x if vertex B
    punpckldq mm1, mm2              ; x | old param

    WRITE_MM1_FIFO_ALIGNED 0        ; PCI write: x | old param
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    movd      mm1, [vertex+Y]       ; 0 | y of vertex B
    nop                             ; filler

%IF GLIDE_PACK_RGB
%IF GLIDE_PACK_ALPHA
    ;; assumes color and alpha values < 256.0
    movq      mm4, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    movd      mm3, [vertex+a]       ; 0 | a
    pf2id     mm4, mm4              ; convert to integer: 000000gg | 000000rr

    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb
    pf2id     mm3, mm3              ; convert to integer: 00000000 | 000000aa

    punpcklwd mm2, mm4              ; 00000000 | 00rr00bb
    psrlq     mm4, 24               ; 00000000 | 0000gg00

    add       dlp, 8                ; skip data list entry "a"
    psllq     mm3, 24               ; 00000000 | aa000000

    por       mm4, mm2              ; 00000000 | 00rrggbb
    por       mm4, mm3              ; 00000000 | aarrggbb
%ELSE ; !GLIDE_PACK_ALPHA
    ;; assumes color values < 256.0

    movq      mm4, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    pf2id     mm4, mm4              ; convert to integer: 000000gg | 000000rr
    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb

    punpcklwd mm2, mm4              ; 00000000 | 00rr00bb
    psrlq     mm4, 24               ; 00000000 | 0000gg00

    add       dlp, 4                ; next data list entry
    por       mm4, mm2              ; 00000000 | 00rrggbb
%ENDIF ; !GLIDE_PACK_ALPHA

    punpckldq mm1, mm4              ; RGB(A) | y
    mov       eax, [dlp]            ; get first offset from the data list

    WRITE_MM1_FIFO_ALIGNED 0        ; PCI write y | RGB(A)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    add       dlp, 4                ; dlp++
    test      eax, eax              ; end of list ?

    nop                             ; filler
    jz        .__paramLoopDoneWBzero2; yes, "write buffer" is empty

.__paramLoop2b:
    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp]            ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jz        .__paramLoopDoneWBone2 ; exit, write buffer contains one DWORD

    movd      mm2, [eax+vertex]     ; get next parameter
    add       dlp, 8                ; dlp += 2

    mov       eax, [dlp-4]          ; offset = *(dlp + 1)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jnz       .__paramLoop2b         ; nope, copy next parameter

    nop                             ; filler
    jmp       .__paramLoopDoneWBzero2; write buffer empty

%ELSE ; !GLIDE_PACK_RGB
    mov       eax, [dlp]            ; get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; end of list ?
    jz        .__paramLoopDoneWBone2 ; yes, "write buffer" has y data

.__paramLoop2b:
    movd      mm2, [eax+vertex]     ; get next parameter
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]            ; offset = *(dlp + 1)
    add       dlp, 8                ; dlp += 2

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?
  
    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jz        .__paramLoopDoneWBzero2; exit, "write buffer" empty

    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp-4]          ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jnz       .__paramLoop2b         ; nope, copy next parameter

    nop                             ; filler
    jmp       .__paramLoopDoneWBone2 ; write buffer contains one DWORD
%ENDIF


.__paramLoopDoneWBzero1:

    mov       vertex, [esp + _vb$]  ; Current vertex = B
    mov       dlp, dlpstrt          ; Reset the dataList

    movq      mm1, [vertex+X]       ; y | x of vertex B
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write y | x of vertex B
    nop                             ; filler

%IF GLIDE_PACK_RGB
%IF GLIDE_PACK_ALPHA
    ;; assumes color and alpha values < 256.0
    movq      mm1, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    movd      mm3, [vertex+a]       ; 0 | a
    pf2id     mm1, mm1              ; convert to integer: 000000gg | 000000rr

    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb
    pf2id     mm3, mm3              ; convert to integer: 00000000 | 000000aa

    punpcklwd mm2, mm1              ; 00000000 | 00rr00bb
    psrlq     mm1, 24               ; 00000000 | 0000gg00

    psllq     mm3, 24               ; 00000000 | aa000000
    por       mm1, mm2              ; 00000000 | 00rrggbb

    por       mm1, mm3              ; 00000000 | aarrggbb
    add       dlp, 8                ; skip data list entry "a"
%ELSE ; !GLIDE_PACK_ALPHA
    ;; assumes color values < 256.0

    movq      mm1, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    pf2id     mm1, mm1              ; convert to integer: 000000gg | 000000rr
    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb

    punpcklwd mm2, mm1              ; 00000000 | 00rr00bb
    psrlq     mm1, 24               ; 00000000 | 0000gg00

    por       mm1, mm2              ; 00000000 | 00rrggbb
    add       dlp, 4                ; next data list entry
%ENDIF ; !GLIDE_PACK_ALPHA

    ;; here: one DWORD in "write buffer", RGB(A)

    mov       eax, dword [dlp]  ; get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; end of list ?
    jz        .__paramLoopDoneWBone2 ; yes, one DWORD in "write buffer"
          
.__paramLoop2a:
    movd      mm2, [eax+vertex]     ; get next parameter
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]            ; offset = *(dlp + 1)
    add       dlp, 8                ; dlp += 8

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jz        .__paramLoopDoneWBzero2; exit, "write buffer" empty

    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp-4]          ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jnz       .__paramLoop2a         ; nope, copy next parameter

    nop                             ; filler
    jmp       .__paramLoopDoneWBone2 ; merge back into common stream

%ELSE ; ! GLIDE_PACK_RGB

    ;; here: "write buffer" empty

    mov       eax, [dlp]            ; Get first offset from the data list
    add       dlp, 4                ; dlp++

    cmp       eax, 0                ; at end of list ?
    jz        .__paramLoopDoneWBzero2; yes, "write buffer" empty
          
.__paramLoop2a:
    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp]            ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jz        .__paramLoopDoneWBone2 ; exit, write buffer contains one DWORD

    movd      mm2, [eax+vertex]     ; get next parameter
    add       dlp, 8                ; dlp += 2

    mov       eax, [dlp-4]          ; offset = *(dlp + 1)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jnz       .__paramLoop2a         ; nope, copy next parameter

%ENDIF ; GLIDE_PACK_RGB


.__paramLoopDoneWBzero2:

    mov       vertex, [esp + _vc$]  ; Current vertex = C
    mov       dlp, dlpstrt          ; Reset the dataList

    movq      mm1, [vertex+X]       ; y | x of vertex C
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write y | x of vertex C
    nop                             ; filler


%IF GLIDE_PACK_RGB
%IF GLIDE_PACK_ALPHA
    ;; assumes color and alpha values < 256.0
    movq      mm1, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    movd      mm3, [vertex+a]       ; 0 | a
    pf2id     mm1, mm1              ; convert to integer: 000000gg | 000000rr

    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb
    pf2id     mm3, mm3              ; convert to integer: 00000000 | 000000aa

    punpcklwd mm2, mm1              ; 00000000 | 00rr00bb
    psrlq     mm1, 24               ; 00000000 | 0000gg00

    psllq     mm3, 24               ; 00000000 | aa000000
    por       mm1, mm2              ; 00000000 | 00rrggbb

    por       mm1, mm3              ; 00000000 | aarrggbb
    add       dlp, 8                ; skip data list entry "a"
%ELSE ; !GLIDE_PACK_ALPHA
    ;; assumes color values < 256.0

    movq      mm1, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    pf2id     mm1, mm1              ; convert to integer: 000000gg | 000000rr
    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb

    punpcklwd mm2, mm1              ; 00000000 | 00rr00bb
    psrlq     mm1, 24               ; 00000000 | 0000gg00

    por       mm1, mm2              ; 00000000 | 00rrggbb
    add       dlp, 4                ; next data list entry
%ENDIF ; !GLIDE_PACK_ALPHA

    ;; here: one DWORD in "write buffer", RGB(A)

    mov       eax, [dlp]            ; get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; end of list ?
    jz        .__paramLoopDoneWBone3 ; yes, one DWORD in "write buffer"
          
.__paramLoop3a:
    movd      mm2, [eax+vertex]     ; get next parameter
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]            ; offset = *(dlp + 1)
    add       dlp, 8                ; dlp += 2

    test      eax, eax              ; at end of offset list (offset == 0) ?
    punpckldq mm1, mm2              ; current param | previous param
  
    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jz        .__paramLoopDoneWBzero3; exit, "write buffer" empty

    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp-4]          ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jnz       .__paramLoop3a         ; nope, copy next parameter

    nop                             ; filler
    jmp       .__paramLoopDoneWBone3 ; merge back into common stream

%ELSE ; ! GLIDE_PACK_RGB

    ;; here: "write buffer" empty

    mov       eax, [dlp]            ; Get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; at end of list ?
    jz        .__paramLoopDoneWBzero3; yes, "write buffer" empty
          
.__paramLoop3a:
    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp]            ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jz        .__paramLoopDoneWBone3 ; exit, write buffer contains one DWORD

    movd      mm2, [eax+vertex]     ; get next parameter
    add       dlp, 8                ; dlp += 2

    mov       eax, [dlp-4]          ; offset = *(dlp + 1)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jnz       .__paramLoop3a         ; nope, copy next parameter

    mov       esp, esp              ; filler
    jmp       .__paramLoopDoneWBzero3; write buffer empty

%ENDIF ; GLIDE_PACK_RGB


.__paramLoopDoneWBone2:

    ;; here: "write buffer" has one DWORD left over from vertex B

    mov       vertex, [esp + _vc$]  ; Current vertex = C
    mov       dlp, dlpstrt          ; reset the dataList

    movd      mm2, [vertex+X]       ; 0 | x if vertex C
    punpckldq mm1, mm2              ; x | old param

    WRITE_MM1_FIFO_ALIGNED 0        ; PCI write: x | old param
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    movd      mm1, [vertex+Y]       ; 0 | y of vertex C
    mov       esp, esp              ; filler

%IF GLIDE_PACK_RGB
%IF GLIDE_PACK_ALPHA
    ;; assumes color and alpha values < 256.0
    movq      mm4, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    movd      mm3, [vertex+a]       ; 0 | a
    pf2id     mm4, mm4              ; convert to integer: 000000gg | 000000rr

    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb
    pf2id     mm3, mm3              ; convert to integer: 00000000 | 000000aa

    punpcklwd mm2, mm4              ; 00000000 | 00rr00bb
    psrlq     mm4, 24               ; 00000000 | 0000gg00

    add       dlp, 8                ; skip data list entry "a"
    psllq     mm3, 24               ; 00000000 | aa000000

    por       mm4, mm2              ; 00000000 | 00rrggbb
    por       mm4, mm3              ; 00000000 | aarrggbb
%ELSE ; !GLIDE_PACK_ALPHA
    ;; assumes color values < 256.0

    movq      mm4, [vertex+r]       ; g | r
    movd      mm2, [vertex+b]       ; 0 | b

    pf2id     mm4, mm4              ; convert to integer: 000000gg | 000000rr
    pf2id     mm2, mm2              ; convert to integer: 00000000 | 000000bb

    punpcklwd mm2, mm4              ; 00000000 | 00rr00bb
    psrlq     mm4, 24               ; 00000000 | 0000gg00

    add       dlp, 4                ; next data list entry
    por       mm4, mm2              ; 00000000 | 00rrggbb
%ENDIF ; !GLIDE_PACK_ALPHA

    punpckldq mm1, mm4              ; RGB(A) | y
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]            ; get first offset from the data list
    add       dlp, 4                ; dlp++

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write y | RGB(A)
    nop                             ; filler

    test      eax, eax              ; end of list ?
    jz        .__paramLoopDoneWBzero3; yes, "write buffer" is empty

.__paramLoop3b:
    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp]            ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jz        .__paramLoopDoneWBone3 ; exit, write buffer contains one DWORD

    movd      mm2, [eax+vertex]     ; get next parameter
    add       dlp, 8                ; dlp++

    mov       eax, [dlp-4]          ; offset = *(dlp + 1)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jnz       .__paramLoop3b         ; nope, copy next parameter

    nop                             ; filler
    jmp       .__paramLoopDoneWBzero3; write buffer empty

%ELSE ; !GLIDE_PACK_RGB

    mov       eax, [dlp]            ; get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; end of list ?
    jz        .__paramLoopDoneWBone3 ; yes, "write buffer" has y data

.__paramLoop3b:
    movd      mm2, [eax+vertex]     ; get next parameter
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]            ; offset = *(dlp + 1)
    add       dlp, 8                ; dlp += 2

    punpckldq mm1, mm2              ; current param | previous param
    cmp       eax, 0                ; at end of offset list (offset == 0) ?
  
    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jz        .__paramLoopDoneWBzero3; exit, "write buffer" empty

    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp-4]          ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jnz       .__paramLoop3b         ; nope, copy next parameter
%ENDIF

.__paramLoopDoneWBone3:

; "write buffer" contains one DWORD that needs to be flushed

    WRITE_MM1LOW_FIFO               ; 
    add       fifo, 4               ;

.__paramLoopDoneWBzero3:

    ;; Update gc->fifoPtr and gc->fifoRoom

    mov       ecx, [_GlideRoot + trisDrawn]    ; _GlideRoot.stats.trisDrawn
    mov       eax, fifo                         ; new fifo pointer
    
    mov       ebx, [gc + fifoPtr]               ; old fifo pointer
    mov       [gc + fifoPtr], fifo              ; save new fifo pointer

    mov       edx, [gc + fifoRoom]              ; old fifo space available
    inc       ecx                               ; _GlideRoot.stats.trisDrawn++

    mov       ebp, [_GlideRoot + trisProcessed]; _GlideRoot.stats.trisProcessed
    sub       eax, ebx                          ; new fifo ptr - old fifo ptr = additional fifo space used

    mov       [_GlideRoot + trisDrawn], ecx    ;
    sub       edx, eax                          ; new fifo space available

    mov       eax, 1h                           ; return value = triangle drawn
    mov       [gc + fifoRoom], edx              ; new fifo space available

    ;; Restore trashed registers    

    inc       ebp                   ; _GlideRoot.stats.trisProcessed++
    pop       esi                   ; restore caller's register variable

    mov       [_GlideRoot + trisProcessed], ebp ;
    pop       ebx                   ; restore caller's register variable

    pop       ebp                   ; restore frame pointer
    pop       edi                   ; restore caller's register variable

    femms                           ; no more AMD3D code, clear FPU/MMX regs

    ret                             ; return to caller


%IF GLIDE_CULLING
.__cullFail:
    mov       ebp, [_GlideRoot + trisProcessed]; triangles processed so far
    xor       eax, eax              ; return value = triangle not drawn

    femms                           ; no more AMD3D code, clear FPU/MMX regs

    ;; Restore trashed registers    
    inc       ebp                   ; _GlideRoot.stats.trisProcessed++;    
    pop       esi

    mov       [_GlideRoot + trisProcessed], ebp
    pop       ebx
    
    pop       ebp                   ; restore frame pointer
    pop       edi

    ret
%ENDIF ; GLIDE_CULLING

;---------------------------------------------------------------------------
;
; end AMD3D section
;
;---------------------------------------------------------------------------

%endif  ; !defined GL_AMD3D

;;--------------------------------------------------------------------------
;; start original code
;;--------------------------------------------------------------------------

%ifndef GL_AMD3D

; Ugly, but seems to workaround the problem with locally defined
; data segment globals not getting relocated properly when using
; djgpp.

%define zArea   One + 04h
%define gc      esi       ; points to graphics context

    ;; Prologue stuff
    push    esi
    mov     gc, [_GlideRoot + curGC]    ;; GR_DCL_GC
    
    push    edi
    push    ebx
    
    push    ebp
    nop
            
            align 4
%IF GLIDE_CULLING
%define fa      eax       ; vtx a from caller
%define fb      ebx       ; vtx b from caller
%define fc      ecx       ; vtx c from caller

%define cull    edx
%define intArea ebp       ; temp Y storage

; some useful floating load and store macros <ala gmt>
%define flds    fld  DWORD
%define fsubs   fsub DWORD
%define fmuls   fmul DWORD

    ;; Pre-load the current culling mode before all of the
    ;; floating point area stuff.    
    mov     fa, [esp + _va$]
    mov     fb, [esp + _vb$]
        
    mov     cull, [gc + cull_mode]    
    mov     fc, [esp + _vc$]

    test    cull, cull    
    jz      .nocull

    shl     cull, 31                    ; culltest << 31    
        
;Area_Computation:
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

    ;; Pop temp things from the sw culling off the fp stack
    fstp    st0   ; 4
    fstp    st0   ; 3
    fstp    st0   ; 2
    fstp    st0   ; 1
    fstp    st0   ; 0

    mov     intArea, [zArea]      ; j = *(long *)&area
    xor     eax, eax              ; Clear the return value (0 == culled)

    ; Zero Area Triangle Check
    and     intArea, 7fffffffh    ; if ((j & 0x7FFFFFFF) == 0)
    jz      .__triDone

    ;; Triangle area check vs culling mode
    mov     intArea, [zArea]              ; reload area just in case we're culling
    xor     intArea, cull               ; if (j ^ (culltest << 31))
    
    jge     .__triDone
.nocull:
%ENDIF ; GLIDE_CULLING

            align 4
    ;; Check to make sure that we have enough room for
    ;; the complete triangle packet.
    mov     eax, [_GlideRoot + curTriSize]
    mov     ebx, [gc + fifoRoom]

    add     eax, 4
    cmp     ebx, eax

    jge     .__triBegin
    
    push    __LINE__
    push    0h
    
    push    eax
    call    _FifoMakeRoom

    add     esp, 12

    ;; Send triangle parameters
    
%define dlp     ebx       ; points to dataList structure
%define fifo    ebp       ; points to next entry in fifo
%define vertex  edx       ; the current vertex
%define vOffset ecx       ; Current vertex offset

%define packCol edi
%define tempVal edi

            align 4    
.__triBegin:
    mov     fifo, [gc + fifoPtr]        ; Fetch Fifo Ptr
    mov     vOffset, 4                  ; Starting vertex

    mov     eax, [gc + triPacketHdr]    ; Packet 3 header
    nop

    GR_FIFO_WRITE fifo, 0, eax          ; Write packet header to fifo    
    add     fifo, 4                     ; Advance fifo for hdr & x/y coordinate

            align 4    
.__vertexStart:
    mov     vertex, [esp + STKOFF + vOffset]    ; Current vertex
    add     fifo, 8    

    nop                                         ; Avoid p5 agi w/ load of vertex ptr
    nop
    
    mov     eax, dword [vertex]             ; X
    lea     dlp, [gc + tsuDataList]             ; Reset the dataList

    GR_FIFO_WRITE fifo, -8, eax                 ; PCI write X
    mov     eax, dword [vertex + 4]         ; Y 

    xor     packCol, packCol                    ; Clear packed color
    GR_FIFO_WRITE fifo, -4, eax                 ; PCI write Y

%IF GLIDE_PACK_RGB
    fld     dword [vertex + b]              ; B
    fadd    dword [_GlideRoot + fBiasLo]    ; BC GC

    fld     dword [vertex + g]              ; G B
    fadd    dword [_GlideRoot + fBiasHi]    ; GC B
    
    fld     dword [vertex + r]              ; R GC BC
    fadd    dword [_GlideRoot + fBiasHi]    ; RC GC BC

    fxch    st2                             ; BC GC RC
    fstp    dword [bias0]                   ; GC RC

    fstp    dword [bias1]                   ; RC
    mov     packCol, dword [bias0]          ; B + bias

    fstp    dword [bias0]
    mov     eax, dword [bias1]              ; G + bias
    
%IF GLIDE_PACK_ALPHA
    fld     dword [vertex + a]
    fadd    dword [_GlideRoot + fBiasHi]

    and     packCol, 00FFh                      ; B color component
    and     eax, 0000FF00h                      ; G component << 8

    add     dlp, 8                              ; Packed RGB + A dataList entry
    nop

    or      packCol, eax                        ; 0000GGBB
    nop

    fstp    dword [bias1]
    mov     eax, dword [bias0]                ; R + bias
    
    mov     esi, dword [bias1]                ; A + bias
    and     eax, 0000FF00h                      ; R component << 8
    
    and     esi, 0FFFFFF00h                     ; A component << 8
    shl     eax, 8                              ; R << 16
    
    or      packCol, eax                        ; 00RRGGBB
    shl     esi, 16                             ; A << 16

    or      packCol, esi                        ; AARRGGBB
    nop
%ELSE ; !GLIDE_PACK_ALPHA
    and     packCol, 00FFh                      ; B color component
    and     eax, 0000FF00h                      ; G component << 8

    add     dlp, 4                              ; Next dataList item    
    or      packCol, eax
    
    mov     eax, dword [bias0]                ; R + bias
    and     eax, 0000FF00h                      ; R component << 8

    shl     eax, 8                              ; R << 16
    or      packCol, eax                        ; 00RRGGBB
%ENDIF ; !GLIDE_PACK_ALPHA

    GR_FIFO_WRITE fifo, 0, packCol              ; PCI write packed color value
    add     fifo, 4
%ENDIF ; GLIDE_PACK_RGB

.__doParams:
    mov     eax, dword [dlp]                ; Get first offset from the data list
    add     dlp, 4                              ; dlp++
    
    cmp     eax, 0                              ; Are we done?
    je      .__nextVertex

    ;; Not using align directive here because it sometimes
    ;; introduces an agi for the eax use below.
    nop
    nop
        
.__paramLoop:
    mov     tempVal, dword [eax + vertex]   ; Get the parameter from teh vertex
    add     fifo, 4                             ; fifoPtr += sizeof(FxU32)

    mov     eax, dword [dlp]                ; offset = *(dlp + 1)
    add     dlp, 4                              ; dlp++
    
    cmp     eax, 0                              ; Are we done?
    GR_FIFO_WRITE fifo, -4, tempVal             ; *fifoPtr = data
    
    jne     .__paramLoop

                align 4        
.__nextVertex:
    ;; On to the next vertex
    add     vOffset, 4
    mov     gc, [_GlideRoot + curGC]            ; Reload gc incase we trashed it as a temp

    cmp     vOffset, 16                         ; Offset of one past last vertex?
    jne     .__vertexStart

    ;; Update gc->fifoPtr and gc->fifoRoom
    mov     eax, fifo
    mov     ebx, [gc + fifoPtr]
    
    mov     [gc + fifoPtr], fifo
    sub     eax, ebx

    mov     ebx, [_GlideRoot + trisDrawn]               ; _GlideRoot.stats.trisDrawn++;
    sub     [gc + fifoRoom], eax

    add     ebx, 1
    mov     [_GlideRoot + trisDrawn], ebx

    ;; return 1 (triangle drawn)    
    mov     eax, 1h

.__triDone:
    ;; Restore trashed registers
    mov     esi, [_GlideRoot + trisProcessed]
    pop     ebp
        
    add     esi, 1    ; _GlideRoot.stats.trisProcessed++;    
    pop     ebx
    
    pop     edi
    mov     [_GlideRoot + trisProcessed], esi
        
    pop     esi
    ret

%endif ; !GL_AMD3D
