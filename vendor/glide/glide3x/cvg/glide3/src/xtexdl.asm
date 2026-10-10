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
;; Revision 1.1.1.1.8.4  2007/06/23 08:50:57  koolsmoky
;; removed unneeded emms calls
;;
;; Revision 1.1.1.1.8.3  2005/06/09 18:32:08  jwrdegoede
;; Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
;;
;; Revision 1.1.1.1.8.2  2004/11/25 19:12:22  koolsmoky
;; fixed 3dnow! and mmx texture download routines
;;
;; Revision 1.1.1.1.8.1  2003/11/03 13:34:30  dborca
;; Voodoo2 happiness (DJGPP & Linux)
;;
;; Revision 1.1.8.7  2003/09/12 05:08:35  koolsmoky
;; preparing for graphic context checks
;;
;; Revision 1.1.8.6  2003/07/07 23:29:06  koolsmoky
;; cleaned logs
;;
;;
;; Revision 1.1  2000/06/15 00:27:43  joseph
;; Initial checkin into SourceForge.
;; 
;; 10    8/17/99 6:35p Atai
;; fixed amd debug mode
;; 
;; 9     4/08/99 1:22p Atai
;; added contect check for _grTexDownload_3DNow_MMX
;; 
;; 8     3/19/99 11:26a Peter
;; expose direct fifo for gl
;; 
;; 7     2/02/99 4:36p Peter
;; download through lfb rather than texture port
;; 
;; 6     12/17/98 2:36p Atai
;; check in Norbert's fix for texture download width correction
;; 
;; 5     12/07/98 11:33a Peter
;; norbert's re-fixes of my merge
;; 
;; 4     11/02/98 5:34p Atai
;; merge direct i/o code
;; 
;; 3     10/20/98 5:34p Atai
;; added #ifdefs for hwc
;; 
;; 2     10/14/98 12:05p Peter
;; fixed my effed up assumption about non-volatile regs
;; 
;; 1     10/09/98 6:48p Peter
;; 3DNow!(tm) version of wide texture downloads
;; 
;; 3     10/07/98 9:43p Peter
;; triangle procs for 3DNow!(tm)
;; 
;; 2     10/05/98 7:43p Peter
;; 3DNow!(tm) happiness everywhere
;; 
;; 1     10/05/98 6:01p Peter
;; mmx stuff for 3DNow!(tm) capable processors
;; 

%include "xos.inc"

extrn _FifoMakeRoom

%MACRO _grCommandTransportMakeRoom 3
    push %3
    push %2
    push %1
    call _FifoMakeRoom
    add  esp, 12
%ENDMACRO ; _grCommandTransportMakeRoom

;;; Definitions of cvg regs and glide root structures.
%INCLUDE "fxgasm.h"

; Arguments (STKOFF = 16 from 4 dword pushes)
STACKOFFSET equ 16
_gc$        equ  4 + STACKOFFSET
_baseAddr$  equ  8 + STACKOFFSET
_maxS$      equ 12 + STACKOFFSET
_minT$      equ 16 + STACKOFFSET
_maxT$      equ 20 + STACKOFFSET
_texData$   equ 24 + STACKOFFSET

    ;; NB: The first set of registers (eax, ecx, and edx) are volatile across
    ;; function calls. The remaining registers are supposedly non-volatile
    ;; so they only store things that are non-volatile across the call.

%define fifo    ebp         ; fifo ptr in inner loop
%define gc      esi         ; graphics context
%define dataPtr edi         ; pointer to exture data to be downloaded
%define curT    ebx         ; counter for texture scan lines (t-coordinate)
%define curS    ecx         ; texture s-coordinate
%define fRoom   edx         ; room available in fifo (in bytes)

;--------------------------------------------------------------------------

%IFNDEF GL_SSE2

;--------------------------------------------------------------------------
;
; GL_AMD3D, GL_MMX
;
;--------------------------------------------------------------------------

segment		SEG_TEXT

              ALIGN  32

%IFDEF GL_AMD3D
proc _grTexDownload_3DNow_MMX, 24
%ENDIF
%IFDEF GL_MMX
proc _grTexDownload_MMX, 24
%ENDIF

    push      ebx                       ; save caller's register variable
    mov       curT, [esp + _maxT$ - 12] ; curT = maxT

    push      esi                       ; save caller's register variable
    mov       eax, [esp + _minT$ - 8]   ; minT

    push      edi                       ; save caller's register variable
    mov       gc, [esp + _gc$ - 4]      ; gc

    push      ebp                       ; save caller's register variable
    mov       dataPtr, [esp + _texData$]; dataPtr

%IFDEF GLIDE_ALT_TAB
    test      gc, gc
    je        .dlDone
;    mov       edx, [gc + windowed]
;    test      edx, 1
;    jnz       .pastContextTest
    mov       edx, DWORD [gc+lostContext]
    mov       ecx, [edx]
    test      ecx, 1
    jnz       .dlDone
;.pastContextTest:
%ENDIF

    sub       curT, eax                 ; curT = maxT - minT
    mov       fifo, [gc + fifoPtr]      ; fifoPtr

    mov       curS, [esp + _maxS$]      ; curS = maxS 
    add       curT, 1                   ; curT = maxT - minT + 1

%IFDEF GL_AMD3D
    femms                               ; we'll use MMX/3DNow!, make sure FPU register cleared
%ENDIF
%IFDEF GL_MMX
;    emms                                ; we'll use MMX
%ENDIF

    mov       edx, curS                 ; curS = maxS = scanline width in DWORDs
    movd      mm3, [esp + _baseAddr$]   ; 0 | address of texture to download

    shl       curS, 2                   ; scan line width (in bytes)
    mov       eax, [esp + _minT$]       ; 0 | minT

    mov       [esp + _maxS$], curS      ; save scan line width (in bytes)
    shl       edx, 3                    ; packetHdr<21:3> = maxS = scanline width in DWORDs

    shl       eax, 9                    ; TEX_ROW_ADDR_INCR(minT) = minT << 9

    or        edx, 0xc0000005           ; packetHdr<31:30> = texture port
                                        ; packetHdr<21:3>  = maxS
                                        ; packetHdr<2:0>   = packetType 5

    movd      mm1, edx                  ; 0 | packetHdr
    movd      mm2, eax                  ; 0 | TEX_ROW_ADDR_INCR(minT)

    paddd     mm3, mm2                  ; 0 | texAddr = texBaseAddr + TEX_ROW_ADDR_INCR(minT)
    movd      mm2, [gc + tex_ptr]       ; 0 | gc->tex_ptr
    psubd     mm3, mm2                  ; 0 | texAddr - gc->tex_ptr
    mov       eax, 0x200                ; TEX_ROW_ADDR_INCR(1) = 1 << 9
    movd      mm2, eax                  ; 0 | TEX_ROW_ADDR_INCR(1)
    psllq     mm2, 32                   ; TEX_ROW_ADDR_INCR(1) | 0

    mov       fRoom, [gc + fifoRoom]    ; get available fifoRoom (in bytes)
    punpckldq mm1, mm3                  ; hdr2 = texAddr - gc->tex_ptr | hdr1 = packetHdr

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = curS = maxS
    ;; edx = fifoRoom, mm1 = texAddr-gc->tex_ptr|packetHdr, mm2 = TEX_ROW_ADDR_INCR(1)|0

    test      fifo, 4                   ; is fifo QWORD aligned ?
    jz        .startDownload            ; yup, start texture download

    cmp       fRoom, 4                  ; enough room for NULL packet in fifo?
    jge       .mmxAlignFifo             ; yes, write NULL packet to align fifo

%ifdef USE_PACKET_FIFO
    _grCommandTransportMakeRoom 4, 0, __LINE__; make fifo room
%endif
    
    mov       fifo, [gc + fifoPtr]      ; fifoPtr modified by _grCommandTransportMakeRoom, reload

    mov       fRoom, [gc + fifoRoom]    ; fifoRoom modified by _grCommandTransportMakeRoom, reload
    mov       curS, [esp + _maxS$]      ; reload maxS (destroyed by call to _grCommandTransportMakeRoom)

    test      fifo, 4                   ; new fifoPtr QWORD aligned ?
    jz        .startDownload            ; yup, start texture download

.mmxAlignFifo:

    mov       DWORD [fifo], 0           ; write NULL packet
    sub       fRoom, 4                  ; fifoRoom -= 4

    mov       [gc + fifoRoom], fRoom    ; store new fifoRoom
    add       fifo, 4                   ; fifoPtr += 4

%IFDEF GLIDE_DEBUG
    mov       [gc + checkPtr], fifo      ; checkPtr
%ENDIF

    mov       [gc + fifoPtr], fifo      ; store new fifoPtr
    jmp       .startDownload            ; fifo aligned, download texture now

    align 32

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = maxS = curS
    ;; edx=fifoRoom, mm1 = texAddr-gc->tex_ptr|packetHdr, mm2 = TEX_ROW_ADDR_INCR(1)|0

.loopT:

%IFDEF GLIDE_DEBUG

    ;; Make sure that we have a QWORD aligned fifoPtr; force GP if not aligned

    test      fifo, 4                   ; is fifoPtr QWORD aligned ?
    jz        .alignmentOK              ; yup, continue

    xor       eax, eax                  ; create 0
    mov       [eax], eax                ; move to DS:[0] forces GP 
.alignmentOK:      
%ENDIF ; GLIDE_DEBUG

    ;; Compute packet header words
    ;;  hdr1: downloadSpace[31:30] numWords[21:3] packetType[2:0]
    ;;  hdr2: download address[29:0]

    movq      [fifo], mm1               ; store hdr2 | hdr1
    add       fifo, 8                   ; increment fifo ptr (hdr1 + hdr2)

    ;; S coordinate inner loop unrolled for 8 texels a write

.loopS:        

    movq      mm0, [dataPtr]            ; load 64 bit data (8 texels)
    add       fifo, 8                   ; pre-increment fifoPtr += 2 * sizeof(FxU32)

    add       dataPtr, 8                ; dataPtr += 2 * sizeof(FxU32)
    sub       curS, 8                   ; curS -= 2 * sizeof(FxU32)

    movq      [fifo - 8], mm0           ; *fifoPtr = texelData[64 bits]
    jnz       .loopS                    ; loop while curS > 0

    mov       ecx, [gc + fifoPtr]       ; old fifo ptr
    nop                                 ; filler

    mov       eax, fifo                 ; new fifo ptr
    mov       [gc + fifoPtr], fifo      ; save new fifo ptr

%IFDEF GLIDE_DEBUG
    mov       [gc + checkPtr], fifo      ; checkPtr
%ENDIF

    sub       eax, ecx                  ; new fifo ptr - old fifo ptr = fifo space used up
    mov       curS, [esp + _maxS$]      ; curS = maxS = width of scanline (bytes)

    sub       fRoom, eax                ; new fifo space available = old fifo space available - fifo space used up = new fifo space available
    sub       curT, 1                   ; curT--

    mov       [gc + fifoRoom], fRoom    ; save new fifo space available 
    jz        .dlDone                   ; loop while curT > 0

    ;; Check for room to write the next texture scanline

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo
    ;; edx = fifoRoom, mm1 = texAddr|packetHdr, mm2 = TEX_ROW_ADDR_INCR(1)|0

    paddd     mm1, mm2                  ; texAddr+=TEX_ROW_ADDR_INCR(1) | packetHdr
    mov       esp, esp                  ; filler
.startDownload:
    lea       eax, [curS+8]             ; fifo space needed = scan line width + header size
 
    cmp       fRoom, eax                ; fifo space available >= fifo space required ?
    jge       .loopT                    ; yup, write next scan line

%ifdef USE_PACKET_FIFO
    _grCommandTransportMakeRoom eax, 0, __LINE__; make fifo room (if fifoPtr QWORD aligned before
%endif
    
    mov       fifo, [gc + fifoPtr]      ; fifoPtr was modified by _grCommandTransportMakeRoom, reload

    mov       fRoom, [gc + fifoRoom]    ; fifoRoom was modified by _grCommandTransportMakeRoom, reload
    mov       curS, [esp + _maxS$]      ; curS = maxS = width of scanline (bytes)
    jmp       .loopT                    ; we now have enough fifo room, write next scanline

.dlDone:           
%IFDEF GL_AMD3D
    femms                               ; exit 3DNow!(tm) state
%ENDIF
%IFDEF GL_MMX
    emms                                ; exit MMX state
%ENDIF

    pop       ebp                       ; restore caller's register variable
    pop       edi                       ; restore caller's register variable
    
    pop       esi                       ; restore caller's register variable
    pop       ebx                       ; restore caller's register variable
    
    ret                                 ; pop 6 DWORD parameters and return
endp

%ELSE ; !GL_SSE2

;--------------------------------------------------------------------------
;
; GL_SSE2
;
;--------------------------------------------------------------------------

segment		SEG_TEXT

              ALIGN  32

proc _grTexDownload_SSE2_64, 24

    push      ebx                       ; save caller's register variable
    mov       curT, [esp + _maxT$ - 12] ; curT = maxT

    push      esi                       ; save caller's register variable
    mov       eax, [esp + _minT$ - 8]   ; minT

    push      edi                       ; save caller's register variable
    mov       gc, [esp + _gc$ - 4]      ; gc

    push      ebp                       ; save caller's register variable
    mov       dataPtr, [esp + _texData$]; dataPtr

%IFDEF GLIDE_ALT_TAB
    test      gc, gc
    je        .dlDone
;    mov       edx, [gc + windowed]
;    test      edx, 1
;    jnz       .pastContextTest
    mov       edx, DWORD [gc+lostContext]
    mov       ecx, [edx]
    test      ecx, 1
    jnz       .dlDone
;.pastContextTest:
%ENDIF

    sub       curT, eax                 ; curT = maxT - minT
    mov       fifo, [gc + fifoPtr]      ; fifoPtr

    mov       curS, [esp + _maxS$]      ; curS = maxS 
    add       curT, 1                   ; curT = maxT - minT + 1

    mov       edx, curS                 ; curS = maxS = scanline width in DWORDs
    movd      xmm3,[esp + _baseAddr$]   ; 0 | 0 | 0 | address of texture to download

    shl       curS, 2                   ; scan line width (in bytes)
    mov       eax, [esp + _minT$]       ; 0 | 0 | 0 | minT

    mov       [esp + _maxS$], curS      ; save scan line width (in bytes)
    shl       edx, 3                    ; packetHdr<21:3> = maxS = scanline width in DWORDs

    imul      eax, curS                 ; TEX_ROW_ADDR_INCR(minT) = minT * TEX_ROW_ADDR_INCR(1)

    movd      xmm2,curS                 ; 0 | 0 | TEX_ROW_ADDR_INCR(1)
    or        edx, 00000005h            ; packetHdr<31:30> = lfb port
                                        ; packetHdr<21:3>  = maxS
                                        ; packetHdr<2:0>   = packetType 5 

    movd      xmm1,edx                  ; 0 | 0 | packetHdr
    movd      xmm4,eax                  ; 0 | 0 | TEX_ROW_ADDR_INCR(minT)

    psllq     xmm2,32                   ; 0 | 0 | TEX_ROW_ADDR_INCR(1) | 0
    paddd     xmm3,xmm4                 ; 0 | 0 | texAddr = texBaseAddr + TEX_ROW_ADDR_INCR(minT)

    mov       fRoom, [gc + fifoRoom]    ; get available fifoRoom (in bytes)
    punpckldq xmm1,xmm3                 ; 0 | 0 | hdr2 = texAddr | hdr1 = packetHdr

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = curS = maxS
    ;; edx = fifoRoom, xmm1 = texAddr|packetHdr, xmm2 = TEX_ROW_ADDR_INCR(1)|0

    test      fifo, 4                   ; is fifo QWORD aligned ?
    jz        .startDownload            ; yup, start texture download

    cmp       fRoom, 4                  ; enough room for NULL packet in fifo?
    jge       .xmmAlignFifo             ; yes, write NULL packet to align fifo

%ifdef USE_PACKET_FIFO
    _grCommandTransportMakeRoom 4, 0, __LINE__; make fifo room
%endif
    
    mov       fifo, [gc + fifoPtr]      ; fifoPtr modified by _grCommandTransportMakeRoom, reload

    mov       fRoom, [gc + fifoRoom]    ; fifoRoom modified by _grCommandTransportMakeRoom, reload
    mov       curS, [esp + _maxS$]      ; reload maxS (destroyed by call to _grCommandTransportMakeRoom)

    test      fifo, 4                   ; new fifoPtr QWORD aligned ?
    jz        .startDownload            ; yup, start texture download

.xmmAlignFifo:

    mov       DWORD [fifo], 0           ; write NULL packet
    sub       fRoom, 4                  ; fifoRoom -= 4

    mov       [gc + fifoRoom], fRoom    ; store new fifoRoom
    add       fifo, 4                   ; fifoPtr += 4

%IFDEF GLIDE_DEBUG
    mov       [gc + checkPtr], fifo      ; checkPtr
%ENDIF

    mov       [gc + fifoPtr], fifo      ; store new fifoPtr
    jmp       .startDownload            ; fifo aligned, download texture now

    align 32

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = maxS = curS
    ;; edx=fifoRoom, xmm1 = texAddr|packetHdr, xmm2 = TEX_ROW_ADDR_INCR(1)|0

.loopT:

%IFDEF GLIDE_DEBUG

    ;; Make sure that we have a QWORD aligned fifoPtr; force GP if not aligned

    test      fifo, 4                   ; is fifoPtr QWORD aligned ?
    jz        .alignmentOK              ; yup, continue

    xor       eax, eax                  ; create 0
    mov       [eax], eax                ; move to DS:[0] forces GP 
.alignmentOK:      
%ENDIF ; GLIDE_DEBUG

    ;; Compute packet header words
    ;;  hdr1: downloadSpace[31:30] numWords[21:3] packetType[2:0]
    ;;  hdr2: download address[29:0]

    movq      [fifo],xmm1               ; store hdr2 | hdr1
    add       fifo, 8                   ; increment fifo ptr (hdr1 + hdr2)

    ;; S coordinate inner loop unrolled for 8 texels a write

.loopS:        

    movq      xmm0,[dataPtr]            ; load 64 bit data (8 texels)
    add       fifo, 8                   ; pre-increment fifoPtr += 2 * sizeof(FxU32)

    add       dataPtr, 8                ; dataPtr += 2 * sizeof(FxU32)
    sub       curS, 8                   ; curS -= 2 * sizeof(FxU32)

    movq      [fifo - 8],xmm0           ; *fifoPtr = texelData[64 bits]
    jnz       .loopS                    ; loop while curS > 0

    mov       ecx, [gc + fifoPtr]       ; old fifo ptr
    nop                                 ; filler

    mov       eax, fifo                 ; new fifo ptr
    mov       [gc + fifoPtr], fifo      ; save new fifo ptr

%IFDEF GLIDE_DEBUG
    mov       [gc + checkPtr], fifo      ; checkPtr
%ENDIF

    sub       eax, ecx                  ; new fifo ptr - old fifo ptr = fifo space used up
    mov       curS, [esp + _maxS$]      ; curS = maxS = width of scanline (bytes)

    sub       fRoom, eax                ; new fifo space available = old fifo space available - fifo space used up = new fifo space available
    sub       curT, 1                   ; curT--

    mov       [gc + fifoRoom], fRoom    ; save new fifo space available 
    jz        .dlDone                   ; loop while curT > 0

    ;; Check for room to write the next texture scanline

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo
    ;; edx = fifoRoom, xmm1 = texAddr|packetHdr, xmm2 = TEX_ROW_ADDR_INCR(1)|0

    paddd     xmm1,xmm2                 ; 0 | 0 | texAddr+=TEX_ROW_ADDR_INCR(1) | packetHdr
    mov       esp, esp                  ; filler
.startDownload:
    lea       eax, [curS+8]             ; fifo space needed = scan line width + header size
 
    cmp       fRoom, eax                ; fifo space available >= fifo space required ?
    jge       .loopT                    ; yup, write next scan line

%ifdef USE_PACKET_FIFO
    _grCommandTransportMakeRoom eax, 0, __LINE__; make fifo room (if fifoPtr QWORD aligned before
%endif
    
    mov       fifo, [gc + fifoPtr]      ; fifoPtr was modified by _grCommandTransportMakeRoom, reload

    mov       fRoom, [gc + fifoRoom]    ; fifoRoom was modified by _grCommandTransportMakeRoom, reload
    mov       curS, [esp + _maxS$]      ; curS = maxS = width of scanline (bytes)
    jmp       .loopT                    ; we now have enough fifo room, write next scanline

.dlDone:
    pop       ebp                       ; restore caller's register variable
    pop       edi                       ; restore caller's register variable
    
    pop       esi                       ; restore caller's register variable
    pop       ebx                       ; restore caller's register variable
    
    ret                                 ; pop 6 DWORD parameters and return
endp



;segment		SEG_TEXT

              ALIGN  32

proc _grTexDownload_SSE2_128, 24

    push      ebx                       ; save caller's register variable
    mov       curT, [esp + _maxT$ - 12] ; curT = maxT

    push      esi                       ; save caller's register variable
    mov       eax, [esp + _minT$ - 8]   ; minT

    push      edi                       ; save caller's register variable
    mov       gc, [esp + _gc$ - 4]      ; gc

    push      ebp                       ; save caller's register variable
    mov       dataPtr, [esp + _texData$]; dataPtr

%IFDEF GLIDE_ALT_TAB
    test      gc, gc
    je        .dlDone
;    mov       edx, [gc + windowed]
;    test      edx, 1
;    jnz       .pastContextTest
    mov       edx, DWORD [gc+lostContext]
    mov       ecx, [edx]
    test      ecx, 1
    jnz       .dlDone
;.pastContextTest:
%ENDIF

    sub       curT, eax                 ; curT = maxT - minT
    mov       fifo, [gc + fifoPtr]      ; fifoPtr

    mov       curS, [esp + _maxS$]      ; curS = maxS 
    add       curT, 1                   ; curT = maxT - minT + 1

    mov       edx, curS                 ; curS = maxS = scanline width in DWORDs
    movd      xmm3,[esp + _baseAddr$]   ; 0 | 0 | 0 | address of texture to download

    shl       curS, 2                   ; scan line width (in bytes)
    mov       eax, [esp + _minT$]       ; 0 | minT

    mov       [esp + _maxS$], curS      ; save scan line width (in bytes)
    shl       edx, 3                    ; packetHdr<21:3> = maxS = scanline width in DWORDs

    imul      eax, curS                 ; TEX_ROW_ADDR_INCR(minT) = minT * TEX_ROW_ADDR_INCR(1)

    movd      xmm2,curS                 ; 0 | 0 | 0 | TEX_ROW_ADDR_INCR(1)
    or        edx, 00000005h            ; packetHdr<31:30> = lfb port
                                        ; packetHdr<21:3>  = maxS
                                        ; packetHdr<2:0>   = packetType 5 

    movd      xmm1,edx                  ; 0 | 0 | 0 | packetHdr
    movd      xmm4,eax                  ; 0 | 0 | 0 | TEX_ROW_ADDR_INCR(minT)

    psllq     xmm2,32                   ; 0 | 0 | TEX_ROW_ADDR_INCR(1) | 0
    paddd     xmm3,xmm4                 ; 0 | 0 | 0 | texAddr = texBaseAddr + TEX_ROW_ADDR_INCR(minT)

    mov       fRoom, [gc + fifoRoom]    ; get available fifoRoom (in bytes)
    punpckldq xmm1,xmm3                 ; 0 | 0 | hdr2 = texAddr | hdr1 = packetHdr

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = curS = maxS
    ;; edx = fifoRoom, xmm1 = texAddr|packetHdr, xmm2 = TEX_ROW_ADDR_INCR(1)|0

    test      fifo, 4                   ; is fifo QWORD aligned ?
    jz        .startDownload            ; yup, start texture download

    cmp       fRoom, 4                  ; enough room for NULL packet in fifo?
    jge       .xmmAlignFifo             ; yes, write NULL packet to align fifo

%ifdef USE_PACKET_FIFO
    _grCommandTransportMakeRoom 4, 0, __LINE__; make fifo room
%endif
    
    mov       fifo, [gc + fifoPtr]      ; fifoPtr modified by _grCommandTransportMakeRoom, reload

    mov       fRoom, [gc + fifoRoom]    ; fifoRoom modified by _grCommandTransportMakeRoom, reload
    mov       curS, [esp + _maxS$]      ; reload maxS (destroyed by call to _grCommandTransportMakeRoom)

    test      fifo, 4                   ; new fifoPtr QWORD aligned ?
    jz        .startDownload            ; yup, start texture download

.xmmAlignFifo:

    mov       DWORD [fifo], 0           ; write NULL packet
    sub       fRoom, 4                  ; fifoRoom -= 4

    mov       [gc + fifoRoom], fRoom    ; store new fifoRoom
    add       fifo, 4                   ; fifoPtr += 4

%IFDEF GLIDE_DEBUG
    mov       [gc + checkPtr], fifo      ; checkPtr
%ENDIF

    mov       [gc + fifoPtr], fifo      ; store new fifoPtr
    jmp       .startDownload            ; fifo aligned, download texture now

    align 32

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = maxS = curS
    ;; edx=fifoRoom, xmm1 = texAddr|packetHdr, xmm2 = TEX_ROW_ADDR_INCR(1)|0

.loopT:

%IFDEF GLIDE_DEBUG

    ;; Make sure that we have a QWORD aligned fifoPtr; force GP if not aligned

    test      fifo, 4                   ; is fifoPtr QWORD aligned ?
    jz        .alignmentOK              ; yup, continue

    xor       eax, eax                  ; create 0
    mov       [eax], eax                ; move to DS:[0] forces GP 
.alignmentOK:      
%ENDIF ; GLIDE_DEBUG

    ;; Compute packet header words
    ;;  hdr1: downloadSpace[31:30] numWords[21:3] packetType[2:0]
    ;;  hdr2: download address[29:0]

    movq      [fifo],xmm1               ; store hdr2 | hdr1
    add       fifo, 8                   ; increment fifo ptr (hdr1 + hdr2)

    ;; S coordinate inner loop unrolled for 8 texels a write

.loopS:        

    movdqu    xmm0, [dataPtr]           ; load 128 bit data (8 texels) ; isn't 16 bytes aligned?
    add       fifo, 16                  ; pre-increment fifoPtr += 4 * sizeof(FxU32)

    add       dataPtr, 16               ; dataPtr += 4 * sizeof(FxU32)
    sub       curS, 16                  ; curS -= 4 * sizeof(FxU32)

    movdqu    [fifo - 16], xmm0         ; *fifoPtr = texelData[128 bits] ; isn't 16 bytes aligned?
    jnz       .loopS                    ; loop while curS > 0

    mov       ecx, [gc + fifoPtr]       ; old fifo ptr
    nop                                 ; filler

    mov       eax, fifo                 ; new fifo ptr
    mov       [gc + fifoPtr], fifo      ; save new fifo ptr

%IFDEF GLIDE_DEBUG
    mov       [gc + checkPtr], fifo      ; checkPtr
%ENDIF

    sub       eax, ecx                  ; new fifo ptr - old fifo ptr = fifo space used up
    mov       curS, [esp + _maxS$]      ; curS = maxS = width of scanline (bytes)

    sub       fRoom, eax                ; new fifo space available = old fifo space available - fifo space used up = new fifo space available
    sub       curT, 1                   ; curT--

    mov       [gc + fifoRoom], fRoom    ; save new fifo space available 
    jz        .dlDone                   ; loop while curT > 0

    ;; Check for room to write the next texture scanline

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo
    ;; edx = fifoRoom, xmm1 = texAddr|packetHdr, xmm2 = TEX_ROW_ADDR_INCR(1)|0

    paddd     xmm1,xmm2                 ; 0 | 0 | texAddr+=TEX_ROW_ADDR_INCR(1) | packetHdr
    mov       esp, esp                  ; filler
.startDownload:
    lea       eax, [curS+8]             ; fifo space needed = scan line width + header size
 
    cmp       fRoom, eax                ; fifo space available >= fifo space required ?
    jge       .loopT                    ; yup, write next scan line

%ifdef USE_PACKET_FIFO
    _grCommandTransportMakeRoom eax, 0, __LINE__; make fifo room (if fifoPtr QWORD aligned before
%endif
    
    mov       fifo, [gc + fifoPtr]      ; fifoPtr was modified by _grCommandTransportMakeRoom, reload

    mov       fRoom, [gc + fifoRoom]    ; fifoRoom was modified by _grCommandTransportMakeRoom, reload
    mov       curS, [esp + _maxS$]      ; curS = maxS = width of scanline (bytes)
    jmp       .loopT                    ; we now have enough fifo room, write next scanline

.dlDone:
    pop       ebp                       ; restore caller's register variable
    pop       edi                       ; restore caller's register variable
    
    pop       esi                       ; restore caller's register variable
    pop       ebx                       ; restore caller's register variable
    
    ret                                 ; pop 6 DWORD parameters and return
endp


%ENDIF ; GL_SSE2
