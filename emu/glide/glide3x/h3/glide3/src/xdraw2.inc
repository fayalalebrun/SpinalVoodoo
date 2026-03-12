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
;; Revision 1.1.1.1.8.1  2003/11/07 13:38:39  dborca
;; unite the clans
;;
;; Revision 1.1.1.1  1999/11/24 21:44:58  joseph
;; Initial checkin for SourceForge
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

    ;; NB:  All of the base triangle procs expect to have the gc
    ;;      passed from the caller in edx so that we can avoid
    ;;      the agi from the far pointer. Screw w/ this at your
    ;;      own peril.
    ;;
    ;;      YOU HAVE BEEN WARNED    

%ifdef GL_AMD3D

;---------------------------------------------------------------------------
; start 3DNow! version
;---------------------------------------------------------------------------

%define gc      edi           ; points to graphics context
%define fifo    ebp           ; points to fifo entries
%define tempVal esi

%IF GLIDE_CLIP_COORDS
    ;; NB:  Currently, the 3DNow!(tm) clip coordinate stuff
    ;;      thunks through to the grDrawTriangles functions
    ;;      which has already been specialized for 3DNow!(tm).
    ;;      This means that we should never get here.
%IFDEF GLIDE_DEBUG
    xor     eax, eax
    mov     [eax], eax
%ENDIF
%ELSE
%IF GLIDE_CULLING
%define fa      eax           ; vtx a from caller
%define fb      ebx           ; vtx b from caller
%define fc      ecx           ; vtx c from caller
%define cull    edx           ; cull mode
%define intArea ecx           ; area temp storage

    ;; Prologue stuff
    push      edi                   ; save caller's register variable
    mov       gc, edx               ; our hoopti calling conventions pass this here    

    push      esi                   ; save caller's register variable
    push      ebx                   ; save caller's register variable 

    mov       fb, [esp + _vb$ - 4]  ; get base address of vertex B
    push      ebp                   ; save frame pointer

%if GLIDE_CLIP_COORDS
    mov       esi, [gc+CoordinateSpace] ; which coord. space (clip/window)?
%endif

%if GLIDE_VALIDATE_STATE
    mov       ebp, [gc + invalid]   ; state validated?
%endif

%if GLIDE_CLIP_COORDS
    lea       eax, [esp+ _va$]      ; pointer to vertex pointers
    test      esi, esi              ; window coordinates ?

    jz        win_coordinates       ; yup

    invoke    _grDrawTriangles, 1, 3, eax; draw the triangle in clip coordinate space

    pop       ebp                   ; restore frame pointer
    pop       ebx                   ; restore caller's register variable
    
    pop       esi                   ; restore caller's register variable
    pop       edi                   ; restore caller's register variable

    ret                             ; return, pop 3 DWORD arguments off stack

.win_coordinates:
%endif  ; GLIDE_CLIP_COORDS

%if GLIDE_VALIDATE_STATE
    test      ebp, ebp              ; does state need validation?
    jz        .no_validation         ; valid, don't need to validate

    invoke    _grValidateState      ; validate state

.no_validation:

%endif  ; GLIDE_VALIDATE_STATE

    mov       cull, [gc + cull_mode]; get cull mode
    nop                             ; filler

    mov       fc, [esp + _vc$]      ; get base address of vertex C
    nop                             ; filler

    ALIGN 16

    femms                           ; will use AMD3D, clear FPU/MMX registers

    test      cull, cull            ; culling enabled ?
    mov       tempVal, [gc + curTriSize]

    ;; Cull Check

    jz        .nocull               ; nope, no culling
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
    jz        .__cullFail           ; area zero, triangle culled

    xor       intArea, cull         ; if (j ^ (culltest << 31))
    jge       .__cullFail           ; triangle facing away from viewer, culled

    cmp       ebx, tempVal          ; fifo space required >= space available ?
    jge       .__triBegin           ; yup, push out triangle data to Voodoo

    invoke    _grCommandTransportMakeRoom, tempVal, 0, __LINE__; note: updates fifoPtr
    
    jmp       .__triBegin           ; merge back with short path

    ;; culling disabled

    ALIGN     16

.nocull:
    ;; Check to make sure that we have enough room for
    ;; the complete triangle packet.

    add       tempVal, 4            ; fifo space needed
    mov       ebx, [gc + fifoRoom]  ; fifo space available

    cmp       ebx, tempVal          ; fifo spce available >= space needed ?
    jge       .__triBegin           ; yup, ready to draw triangle

    invoke    _grCommandTransportMakeRoom, tempVal, 0, __LINE__; note: updates fifoPtr
%ELSE   ; !GLIDE_CULLING

    ;; Prologue stuff

    push      edi                   ; save caller's register variable
    push      esi                   ; save caller's register variable 

    mov       gc, edx               ; gc in edx from caller
    push      ebx                   ; save caller's register variable

    push      ebp                   ; save frame pointer

%if GLIDE_CLIP_COORDS
    mov       edx, [gc+CoordinateSpace]; window coordinates or clip coordinates ?
%endif

%if GLIDE_VALIDATE_STATE
    mov       ebp, [gc + invalid]   ; state validated?
%endif

%if GLIDE_CLIP_COORDS
    lea       eax, [esp+ _va$]      ; pointer to vertex pointers
    test      edx, edx              ; window coordinates ?

    jz        win_coordinates       ; yup

    invoke    _grDrawTriangles, 1, 3, eax; draw the triangle in coordinate space

    pop       ebp                   ; restore frame pointer
    pop       ebx                   ; restore caller's register variable
    
    pop       esi                   ; restore caller's register variable
    pop       edi                   ; restore caller's register variable

    ret                             ; return, pop 3 DWORD arguments off stack

    ALIGN  16
.win_coordinates:

%endif

%if GLIDE_VALIDATE_STATE
    test      ebp, ebp              ; does state need validation?
    jz        .no_validation         ; valid, don't need to validate

    invoke    _grValidateState      ; validate state

.no_validation:

%endif

    mov       tempVal, [gc + curTriSize] ; data for whole triangle in bytes
    add       tempVal, 4            ; fifo space needed (include 4-byte header)
    mov       ebx, [gc + fifoRoom]  ; fifo space available

    femms                           ; will use AMD3D, clear FPU/MMX registers

    cmp       ebx, tempVal          ; fifo spce available >= space needed ?
    jge       .__triBegin           ; yup, ready to draw triangle

    invoke    _grCommandTransportMakeRoom, tempVal, 0, __LINE__; note: updates fifoPtr
    
    jmp       .__triBegin           ; large distance due to alignment
%ENDIF ; GLIDE_CULLING


%define dlp     ebx           ; points to dataList structure
%define dlpstrt ecx           ; points to begin of dataList structure
%define vertex  edx           ; the current vertex

    ALIGN 32                        

.__triBegin:
    mov       eax, [gc+triPacketHdr]; Packet 3 header
    lea       dlp,[gc + tsuDataList]; Reset the dataList

    mov       fifo, [gc + fifoPtr]  ; Fetch Fifo Ptr
    mov       vertex, [esp + _va$]  ; Current vertex = A

    mov       dlpstrt, dlp          ; save pointer to start of dataList
    test      fifo, 4               ; is fifo pointer qword aligned ?

    jz        .__fifo_aligned       ; yes, it is qword aligned
    movq      mm1, [vertex+X]       ; y | x

    GR_FIFO_WRITE fifo, 0, eax      ; write header to fifo; now qword aligned
    add       fifo, 12              ; fifoPtr += 3*sizeof(FxU32)

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write y | x

    ;; here: "write buffer" empty

    mov       eax, [dlp]            ; Get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; at end of list ?
    jz        .__paramLoopDoneWBzero1; yes, "write buffer" empty
          
.__paramLoop1a:
    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp]            ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jz        .__paramLoopDoneWBone1; exit, write buffer contains one DWORD

    movd      mm2, [eax+vertex]     ; get next parameter
    add       dlp, 8                ; dlp += 2

    mov       eax, [dlp-4]          ; offset = *(dlp + 1)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jnz       .__paramLoop1a        ; nope, copy next parameter

    jmp       .__paramLoopDoneWBzero1; write buffer empty

.__fifo_aligned:
    movd      mm2, [vertex+X]       ; y | x of vertex A
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    movd      mm1, [gc+triPacketHdr]; Packet 3 header
    punpckldq mm1, mm2              ; x | header

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write x | header
    movd      mm1, [vertex+Y]       ; 0 | y of vertex A

    mov       eax, [dlp]            ; get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; end of list ?
    jz        .__paramLoopDoneWBone1; yes, "write buffer" has y data

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

.__paramLoopDoneWBone1:

    ;; here: "write buffer" has one DWORD left over from vertex A

    mov       dlp, dlpstrt          ; reset the dataList
    mov       vertex, [esp + _vb$]  ; Current vertex = B

    movd      mm2, [vertex+X]       ; 0 | x if vertex B
    punpckldq mm1, mm2              ; x | old param

    WRITE_MM1_FIFO_ALIGNED 0        ; PCI write: x | old param
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    movd      mm1, [vertex+Y]       ; 0 | y of vertex B

    mov       eax, [dlp]            ; get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; end of list ?
    jz        .__paramLoopDoneWBone2; yes, "write buffer" has y data

.__paramLoop2b:
    movd      mm2, [eax+vertex]     ; get next parameter
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]            ; offset = *(dlp + 1)
    add       dlp, 8                ; dlp += 2

    punpckldq mm1, mm2              ; current param | previous param
    cmp       eax, 0                ; at end of offset list (offset == 0) ?
  
    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jz        .__paramLoopDoneWBzero2; exit, "write buffer" empty

    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp-4]          ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jnz       .__paramLoop2b        ; nope, copy next parameter

    jmp       .__paramLoopDoneWBone2; write buffer contains one DWORD

.__paramLoopDoneWBzero1:

    mov       vertex, [esp + _vb$]  ; Current vertex = B
    mov       dlp, dlpstrt          ; Reset the dataList

    movq      mm1, [vertex+X]       ; y | x of vertex B
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write y | x of vertex B
    nop                             ; filler

    ;; here: "write buffer" empty

    mov       eax, [dlp]            ; Get first offset from the data list
    add       dlp, 4                ; dlp++

    cmp       eax, 0                ; at end of list ?
    jz        .__paramLoopDoneWBzero2; yes, "write buffer" empty
          
.__paramLoop2a:
    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp]            ; offset = *(dlp + 1)

    cmp       eax, 0                ; at end of offset list (offset == 0) ?
    jz        .__paramLoopDoneWBone2; exit, write buffer contains one DWORD

    movd      mm2, [eax+vertex]     ; get next parameter
    add       dlp, 8                ; dlp += 2

    mov       eax, [dlp-4]          ; offset = *(dlp + 1)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jnz       .__paramLoop2a        ; nope, copy next parameter

.__paramLoopDoneWBzero2:

    mov       vertex, [esp + _vc$]  ; Current vertex = C
    mov       dlp, dlpstrt          ; Reset the dataList

    movq      mm1, [vertex+X]       ; y | x of vertex C
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write y | x of vertex C

    ;; here: "write buffer" empty

    mov       eax, [dlp]            ; Get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; at end of list ?
    jz        .__paramLoopDoneWBzero3; yes, "write buffer" empty
          
.__paramLoop3a:
    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp]            ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jz        .__paramLoopDoneWBone3; exit, write buffer contains one DWORD

    movd      mm2, [eax+vertex]     ; get next parameter
    add       dlp, 8                ; dlp += 2

    mov       eax, [dlp-4]          ; offset = *(dlp + 1)
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?

    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jnz       .__paramLoop3a        ; nope, copy next parameter

    jmp       .__paramLoopDoneWBzero3; write buffer empty

.__paramLoopDoneWBone2:

    ;; here: "write buffer" has one DWORD left over from vertex B

    mov       vertex, [esp + _vc$]  ; Current vertex = C
    mov       dlp, dlpstrt          ; reset the dataList

    movd      mm2, [vertex+X]       ; 0 | x if vertex C
    punpckldq mm1, mm2              ; x | old param

    WRITE_MM1_FIFO_ALIGNED 0        ; PCI write: x | old param
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    movd      mm1, [vertex+Y]       ; 0 | y of vertex C
    nop                             ; filler

    mov       eax, [dlp]            ; get first offset from the data list
    add       dlp, 4                ; dlp++

    test      eax, eax              ; end of list ?
    jz        .__paramLoopDoneWBone3; yes, "write buffer" has y data

.__paramLoop3b:
    movd      mm2, [eax+vertex]     ; get next parameter
    add       fifo, 8               ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]            ; offset = *(dlp + 1)
    add       dlp, 8                ; dlp += 2

    punpckldq mm1, mm2              ; current param | previous param
    test      eax, eax              ; at end of offset list (offset == 0) ?
  
    WRITE_MM1_FIFO_ALIGNED -8       ; PCI write current param | previous param
    jz        .__paramLoopDoneWBzero3; exit, "write buffer" empty

    movd      mm1, [eax+vertex]     ; get next parameter
    mov       eax, [dlp-4]          ; offset = *(dlp + 1)

    test      eax, eax              ; at end of offset list (offset == 0) ?
    jnz       .__paramLoop3b        ; nope, copy next parameter

.__paramLoopDoneWBone3:

; "write buffer" contains one DWORD that needs to be flushed

    WRITE_MM1LOW_FIFO               ; 
    add       fifo, 4               ;

.__paramLoopDoneWBzero3:

    ;; Update gc->fifoPtr and gc->fifoRoom

    mov       ecx, [gc + trisDrawn]             ; _GlideRoot.stats.trisDrawn
    mov       eax, fifo                         ; new fifo pointer
    
    mov       ebx, [gc + fifoPtr]               ; old fifo pointer
    mov       [gc + fifoPtr], fifo              ; save new fifo pointer

    mov       edx, [gc + fifoRoom]              ; old fifo space available
    inc       ecx                               ; _GlideRoot.stats.trisDrawn++

    mov       esi, [gc + trisProcessed]         ; _GlideRoot.stats.trisProcessed
    sub       eax, ebx                          ; new fifo ptr - old fifo ptr = additional fifo space used

    mov       [gc + trisDrawn], ecx             ;
    sub       edx, eax                          ; new fifo space available

    mov       eax, 1h                           ; return value = triangle drawn
    mov       [gc + fifoRoom], edx              ; new fifo space available

    ;; Restore trashed registers    

.__triDone_nocull:

    inc       esi                   ; _GlideRoot.stats.trisProcessed++
    pop       ebp                   ; restore frame pointer

    mov       [gc + trisProcessed], esi ;
    pop       ebx                   ; restore caller's register variable
    
    pop       esi                   ; restore caller's register variable
    pop       edi                   ; restore caller's register variable

    femms                           ; no more AMD3D code, clear FPU/MMX regs

    ret                             ; return to caller


%IF GLIDE_CULLING
.__cullFail:
    mov       esi, [gc + trisProcessed]; triangles processed so far
    xor       eax, eax              ; return value = triangle not drawn

    femms                           ; no more AMD3D code, clear FPU/MMX regs

.__triDone_cull:
    ;; Restore trashed registers    
    inc       esi                   ; _GlideRoot.stats.trisProcessed++;    
    pop       ebp                   ; restore frame pointer

    mov       [gc + trisProcessed], esi
    pop       ebx
    
    pop       esi
    pop       edi

    ret
%ENDIF ; GLIDE_CULLING
%ENDIF ; !GLIDE_CLIP_COORDS

;---------------------------------------------------------------------------
; end 3DNow! version
;---------------------------------------------------------------------------

%endif ; GL_AMD3D


%ifndef GL_AMD3D

;---------------------------------------------------------------------------
; original code
;---------------------------------------------------------------------------
        
; Ugly, but seems to workaround the problem with locally defined
; data segment globals not getting relocated properly when using
; djgpp.

%define zArea   One+04h
%define zdxAB   One+08h
%define zdxBC   One+0ch
%define zdyAB   One+10h
%define zdyBC   One+14h
%define zculltest One+18h

%define gc      esi       ; points to graphics context

%IF GLIDE_CLIP_COORDS
    ;; NB:  We should never hit this because the proc is
    ;;      either specialized or we thunk through to
    ;;      grDrawTriangles for the clipping etc.
%IFDEF GLIDE_DEBUG
    xor     eax, eax
    mov     [eax], eax
%ENDIF
%ELSE
    ;; Prologue stuff
    push    esi
    push    edi
    
    push    ebx
    push    ebp

;;   call    getThreadValueSLOW
;;   mov     gc, eax    
    
%IF 0
;;; TRICKY STUFF HERE:
;;; The following code would be expressed like this if MASM didn't suck:
;;; mov eax, DWORD PTR fs:[18h]
;;; Remember, gc == esi
   db 064h
   db 0a1h
   db 018h
   db 00
    
   db 00h
   db 00h
  
   add eax, DWORD [_GlideRoot + tlsOffset]
   mov gc, [eax]
%ELSE
   mov gc, edx
%ENDIF
%IFDEF GLIDE_ALT_TAB
   test gc, gc
   je   .__triDone
   mov  edx, [gc + windowed]
   test edx, 1
   jnz  .pastContextTest         ;
   mov  edx, DWORD [gc+lostContext]
   mov  ebx, [edx]
   test ebx, 1
   jnz  .__triDone
.pastContextTest:

%ENDIF
            align 4
%IF GLIDE_VALIDATE_STATE
;;;     GR_FLUSH_STATE()
    mov     edx, DWORD [gc+invalid]
    test    edx, edx
    je      .no_validatioin
    invoke  _grValidateState
.no_validatioin:
%ENDIF
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
        
; Area_Computation:
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
    mov     intArea, [zArea]            ; reload area just in case we're culling
    xor     intArea, cull               ; if (j ^ (culltest << 31))
    
    jge     .__triDone
.nocull:
%ENDIF ; GLIDE_CULLING

            align 4
    ;; Check to make sure that we have enough room for
    ;; the complete triangle packet.
    mov     eax, [gc + curTriSize]
    mov     ebx, [gc + fifoRoom]

    add     eax, 4
    cmp     ebx, eax

    jge     .__triBegin
    
    invoke  _grCommandTransportMakeRoom, eax, 0, __LINE__

    ;; Send triangle parameters
    
%define dlp     ebx       ; points to dataList structure
%define fifo    ebp       ; points to next entry in fifo
%define vertex  edx       ; the current vertex
%define vOffset ecx       ; Current vertex offset

%define packCol edi
%define tempVal edi

            align 32
.__triBegin:
    mov     fifo, [gc + fifoPtr]        ; Fetch Fifo Ptr
    mov     vOffset, 4                  ; Starting vertex

    mov     eax, [gc + triPacketHdr]    ; Packet 3 header
    nop

    GR_FIFO_WRITE fifo, 0, eax          ; Write packet header to fifo    
    add     fifo, 4                     ; Advance fifo for hdr & x/y coordinate

            align 32   
.__vertexStart:
    mov     vertex, [esp + STKOFF + vOffset]    ; Current vertex
    add     fifo, 8    

    nop                                         ; Avoid p5 agi w/ load of vertex ptr
    nop
    
    mov     eax, DWORD [vertex]                 ; X
    lea     dlp, [gc + tsuDataList]             ; Reset the dataList

    GR_FIFO_WRITE fifo, -8, eax                 ; PCI write X
    mov     eax, DWORD [vertex + 4]             ; Y

    xor     packCol, packCol                    ; Clear packed color
    GR_FIFO_WRITE fifo, -4, eax                 ; PCI write Y

.__doParams:
    mov     eax, DWORD [dlp]                    ; Get first offset from the data list
    add     dlp, 4                              ; dlp++
    
    cmp     eax, 0                              ; Are we done?
    je      .__nextVertex

    ;; Not using align directive here because it sometimes
    ;; introduces an agi for the eax use below.
    nop
    nop
        
.__paramLoop:
    mov     tempVal, DWORD [eax + vertex]       ; Get the parameter from teh vertex
    add     fifo, 4                             ; fifoPtr += sizeof(FxU32)

    mov     eax, DWORD [dlp]                    ; offset = *(dlp + 1)
    add     dlp, 4                              ; dlp++
    
    cmp     eax, 0                              ; Are we done?
    GR_FIFO_WRITE fifo, -4, tempVal             ; *fifoPtr = data
    
    jne     .__paramLoop

                align 4        
.__nextVertex:
    ;; On to the next vertex
    add     vOffset, 4

    cmp     vOffset, 16                         ; Offset of one past last vertex?
    jne     .__vertexStart

    ;; Update gc->fifoPtr and gc->fifoRoom
    mov     eax, fifo
    mov     ebx, [gc + fifoPtr]
    
    mov     [gc + fifoPtr], fifo
    sub     eax, ebx

    mov     ebx, [gc + trisDrawn]               ; _GlideRoot.stats.trisDrawn++;    
    sub     [gc + fifoRoom], eax

    add     ebx, 1
    mov     [gc + trisDrawn], ebx

    ;; return 1 (triangle drawn)    
    mov     eax, 1h

.__triDone:
    ;; Restore trashed registers
    mov     ecx, [gc + trisProcessed]
    pop     ebp
        
    add     ecx, 1    ; _GlideRoot.stats.trisProcessed++;    
    pop     ebx
    
    pop     edi
    mov     [gc + trisProcessed], ecx
        
    pop     esi
    ret
%ENDIF ; !GLIDE_CLIP_COOR
%ENDIF ; !GL_AMD3D
