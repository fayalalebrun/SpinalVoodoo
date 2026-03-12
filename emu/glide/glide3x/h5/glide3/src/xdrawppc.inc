;
; THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
; PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
; TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
; INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
; DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
; THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
; EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
; FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
; 
; USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
; RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
; TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
; AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
; SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
; THE UNITED STATES.  
; 
; COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
;
  IF 1
  
; Start of triangle setup
; at this point, registers look like this:
;
; r28 -   vertex A
; r29 -   vertex B
; r30 -   vertex C
; r31 -   gc
  
; First step, check for fifo room.

  lwz			r6,curTriSize(r31)								; get triangle data size
  
  lwz			r0,fifoRoom(r31)                  ; get fifo room
  
  lwz			r7,fifoPtr(r31)										; get fifo pointer
  addi		r6,r6,4														; add in packet header size
  
  cmpw    cr1,r0,r6                         ; is there enough room?  (set CR1)
  li      r10,-4                            ; keep r10 around as -4
  
  bge+    cr1,@gotFifoRoom                  ; use CR1

	li			r4,0	; XXX - FIXME!!!
  li      r5,0
  
  addi		r3,r6,0
  bl			._grCommandTransportMakeRoom
  
  lwz			r7,fifoPtr(r31)										; reload fifo pointer  
  li      r10,-4                            ; reload r10
	  
	align   3  

  ; Register usage is as follows:
  ;
  ; r7 - fifoPtr
  ; r8 - ???
  ; r9 - alignment check
  ; r10 - pointer to "buffer" on the stack (-8 offset)
  ; r5 - constant 4
  
@gotFifoRoom:
  stw     r28,-16(sp)
  subi    r28,sp,16                         ; get pointer to first vertex pointer
  stw     r29,-12(sp)
  addi    r29,r0,3                          ; vertex count
  stw     r30,-8(sp)
  mtctr   r29                               ; store count in CTR
  
  lwz     r6,triPacketHdr(r31)              ; get packet header
  rlwinm. r0,r7,0,29,31                     ; check FIFO alignment...  (set CR0)
  
  subi    r10,sp,32                         ; get pointer to magic buffer
  addi    r5,r0,4                           ; get constant offset of 4 into r5
  
  bne     @vertex_align                     ; do aligned case

  ; FIFO pointer is already aligned, so store to buffer
  stwbrx  r6,r0,r10
  b       @vertex_begin_half
  
@vertex_align:
  
  stwbrx  r6,r0,r7                          ; store packet header into fifo
  addi    r7,r7,4                           ; update fifo pointer

  ; buffer is empty, so store X and Y, then flush buffer
@vertex_begin_empty:
  lwz     r29,0(r28)                        ; load vertex pointer
  addi    r28,r28,4                         ; calc next pointer  
  
  lwz     r4,tsuDataList(r31)               ; load first data index directly
  addi    r11,r31,tsuDataList               ; get pointer to data list (1st entry)
  
  lwz     r3,0(r29)                         ; load X
  addi    r7,r7,8                           ; increment FIFO pointer
  
  lwz     r6,4(r29)                         ; load Y
  cmplwi  cr0,r4,$0000                      ; set CR0 for end of data list...
  
  stwbrx  r3,r0,r10                         ; store to buffer[0]
  stwbrx  r6,r5,r10                         ; store to buffer[1]
  
  lfd     fp0,0(r10)                        ; load fifo value
  stfd    fp0,-8(r7)                        ; store fifo value
  b       @vertex_loop_empty
  
@vertex_begin_half:
  lwz     r29,0(r28)                        ; load vertex pointer
  addi    r28,r28,4                         ; get next pointer  

  lwz     r8,tsuDataList(r31)               ; load first data index directly
  addi    r11,r31,tsuDataList               ; get pointer to data list (1st entry)
  
  lwz     r3,0(r29)                         ; load X
  addi    r7,r7,8                           ; increment FIFO pointer
  
  lwz     r6,4(r29)                         ; load Y
  cmplwi  cr1,r8,$0000                      ; set CR1 for end of data list...
  
  stwbrx  r3,r5,r10                         ; store to buffer[1]
  
  lfd     fp0,0(r10)                        ; load fifo value
  
  stwbrx  r6,r0,r10                         ; store to buffer[0]
  
  stfd    fp0,-8(r7)                        ; store fifo value
  b       @vertex_loop_half
  
@vertex_loop_empty
  beq     cr0,@vertex_end_empty             ; precomputed cr0
  lwzu    r8,4(r11)                         ; grab next index
  
  lwzx    r3,r4,r29                         ; load current data item
  
  cmplwi  cr1,r8,$0000                      ; set CR1 for end of data list...
  
  stwbrx  r3,r0,r10                         ; store data item
  
  ; fall through...
  
@vertex_loop_half
  beq     cr1,@vertex_end_half              ; precomputed cr1
  lwzu    r4,4(r11)                         ; grab next index
  
  lwzx    r3,r8,r29                         ; load current data item
  addi    r7,r7,8                           ; update fifo pointer
  
  cmplwi  cr0,r4,$0000                      ; set CR0 for end of data list...
  
  stwbrx  r3,r5,r10                         ; store data item
  
  lfd     fp0,0(r10)                        ; load fifo data
  stfd    fp0,-8(r7)                        ; store fifo data
  
  b       @vertex_loop_empty                ; go do next data item.

@vertex_end_empty
  bdnz    @vertex_begin_empty
  b       @vertex_end
  
@vertex_end_half
  bdnz    @vertex_begin_half    
  lwz     r3,0(r10)
  stwx    r3,r0,r7                          ; store last data item
  addi    r7,r7,4                           ; update fifo pointer
  
@vertex_end:  

; WHEW!  Okay, now update FIFO pointer & fifo data availability before we go.
	      
	lwz     r3,autoBump(r31)                  ; get autoBump flag
	
	lwz     r4,fifoPtr(r31)                   ; get original fifo pointer

	lwz     r5,bumpPos(r31)                   ; get next bump pos (in case we need it)
	
	lwz     r0,fifoRoom(r31)                  ; get fifo free
	cmplwi  cr0,r3,$0000                      ; check auto bump flag
	
	subf    r4,r4,r7                          ; calc amount used      
	cmpw    cr1,r5,r7                         ; is bumpPos > fifoPtr?
	
	lwz     r6,lastBump(r31)                  ; get last bump pos
	subf    r0,r4,r0                          ; calc new amount free
	
	stw     r0,fifoRoom(r31)                  ; save free
	
	stw     r7,fifoPtr(r31)                   ; save new fifo pointer
	
	bne+    @no_bump                          ; if we're auto bumping, then skip bump
	bgt+    cr1,@no_bump                      ; if we haven't reached bump pos, then skip bump
	    
; Need to bump.	   We have current fifo ptr in r7, last bump pos in r6
  lwz     r3,bumpSize(r31)
	subf    r6,r6,r7                          ; r6 = bytes to bump
	
	stw     r7,lastBump(r31)                  ; store current as last bump pos
	sri     r6,r6,2                           ; convert difference to words
	
	addis   r5,r31,1                          ; get gc + 64K
	sli     r3,r3,2                           ; convert bump words to bytes
	
	lwz     r5,cRegs(r5)                      ; get pointer to command transport registers
	add     r3,r3,r7                          ; calc next bump position
	
	stw     r3,bumpPos(r31)                   ; store next bump position (don't care if it's past the end of the fifo...)
	addi    r4,r0,cmdFifo0Bump                ; r4 = offset to bump register
	
	stwbrx  r6,r4,r5                          ; hit bump register
	    
	; And we're outta here...
	    
@no_bump
	    
; Final cleanup code begins here.  Return FXTRUE.

  li      r3,1
  lwz     r0,136(sp)
  
  addi    sp,sp,128
  
; 1 cycle stall
  
  mtlr    r0  
  lwz     r28,-16(sp)    
  lwz     r29,-12(sp)
  lwz     r30,-8(sp)
  lwz     r31,-4(sp)
  blr
  
  ELSE

; Start of triangle setup
; at this point, registers look like this:
;
; r28 -   vertex A
; r29 -   vertex B
; r30 -   vertex C
; r31 -   gc
  
; First step, check for fifo room.

  lwz			r6,curTriSize(r31)								; get triangle data size
  
  lwz			r0,fifoRoom(r31)                  ; get fifo room
  
  lwz			r7,fifoPtr(r31)										; get fifo pointer
  addi		r6,r6,4														; add in packet header size
  
  cmpw    cr1,r0,r6                         ; is there enough room?  (set CR1)
  li      r10,-4                            ; keep r10 around as -4
  
  bge+    cr1,@gotFifoRoom                  ; use CR1

	li			r4,0	; XXX - FIXME!!!
  li      r5,0
  
  addi		r3,r6,0
  bl			._grCommandTransportMakeRoom
  
  lwz			r7,fifoPtr(r31)										; reload fifo pointer  
  li      r10,-4                            ; reload r10
	  
	align   3  

  ; Register usage is as follows:
  ;
  ; r7 - fifoPtr
  ; r8 - ???
  ; r9 - alignment check
  ; r10 - pointer to "buffer" on the stack (-8 offset)
  ; r5 - constant 4
  
@gotFifoRoom:
  stw     r28,-16(sp)
  subi    r28,sp,16                         ; get pointer to first vertex pointer
  stw     r29,-12(sp)
  addi    r29,r0,3                          ; vertex count
  stw     r30,-8(sp)
  mtctr   r29                               ; store count in CTR
  
  addi    r6,r31,triPacketHdr
  lwbrx   r6,r0,r6
  
  addi    r0,r0,4                           ; constant 4
  
  stw     r6,0(r7)                          ; store packet header into fifo
  addi    r7,r7,4                           ; update fifo pointer

  ; buffer is empty, so store X and Y, then flush buffer
@vertex_begin:
  lwz     r29,0(r28)                        ; load vertex pointer
  addi    r28,r28,4                         ; calc next pointer  
  
  lwbrx   r3,r0,r29                         ; load X
  
  lwbrx   r4,r29,r0                         ; load Y

  lwz     r5,tsuDataList(r31)               ; load first data index directly
  addi    r11,r31,tsuDataList               ; get pointer to data list (1st entry)
  
  stw     r3,0(r7)
  addi    r7,r7,4
  
  stw     r4,0(r7)
  addi    r7,r7,4
  
  cmplwi  cr0,r5,$0000                      ; set CR0 for end of data list...
  ; dst     r0,r28                            ; try to preload next vector
  
@vertex_loop
  beq     cr0,@vertex_end                   ; precomputed cr0
  lwzu    r8,4(r11)                         ; grab next index
  
  lwbrx   r3,r5,r29                         ; load current data item
  
  cmplwi  cr0,r8,$0000                      ; set CR1 for end of data list...
  mr      r5,r8
  
  stw     r3,0(r7)                          ; store data item
  addi    r7,r7,4                           ; increment fifo ptr
  
  b       @vertex_loop
  
@vertex_end
  bdnz    @vertex_begin
  
; WHEW!  Okay, now update FIFO pointer & fifo data availability before we go.
	      
	lwz     r3,autoBump(r31)                  ; get autoBump flag
	
	lwz     r4,fifoPtr(r31)                   ; get original fifo pointer

	lwz     r5,bumpPos(r31)                   ; get next bump pos (in case we need it)
	
	lwz     r0,fifoRoom(r31)                  ; get fifo free
	cmplwi  cr0,r3,$0000                      ; check auto bump flag
	
	subf    r4,r4,r7                          ; calc amount used      
	cmpw    cr1,r5,r7                         ; is bumpPos > fifoPtr?
	
	lwz     r6,lastBump(r31)                  ; get last bump pos
	subf    r0,r4,r0                          ; calc new amount free
	
	stw     r0,fifoRoom(r31)                  ; save free
	
	stw     r7,fifoPtr(r31)                   ; save new fifo pointer
	
	bne+    @no_bump                          ; if we're auto bumping, then skip bump
	bgt+    cr1,@no_bump                      ; if we haven't reached bump pos, then skip bump
	    
; Need to bump.	   We have current fifo ptr in r7, last bump pos in r6
  lwz     r3,bumpSize(r31)
	subf    r6,r6,r7                          ; r6 = bytes to bump
	
	stw     r7,lastBump(r31)                  ; store current as last bump pos
	sri     r6,r6,2                           ; convert difference to words
	
	addis   r5,r31,1                          ; get gc + 64K
	sli     r3,r3,2                           ; convert bump words to bytes
	
	lwz     r5,cRegs(r5)                      ; get pointer to command transport registers
	add     r3,r3,r7                          ; calc next bump position
	
	stw     r3,bumpPos(r31)                   ; store next bump position (don't care if it's past the end of the fifo...)
	addi    r4,r0,cmdFifo0Bump                ; r4 = offset to bump register
	
	stwbrx  r6,r4,r5                          ; hit bump register
	    
	; And we're outta here...
	    
@no_bump
	    
; Final cleanup code begins here.  Return FXTRUE.

  li      r3,1
  lwz     r0,136(sp)
  
  addi    sp,sp,128
  
; 1 cycle stall
  
  mtlr    r0  
  lwz     r28,-16(sp)    
  lwz     r29,-12(sp)
  lwz     r30,-8(sp)
  lwz     r31,-4(sp)
  blr
  
  ENDIF
  