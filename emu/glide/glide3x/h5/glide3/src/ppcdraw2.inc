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

@gotFifoRoom:
  rlwinm. r0,r7,0,27,31                     ; check FIFO alignment...  (set CR0)
  lwz     r6,triPacketHdr(r31)              ; get packet header
  
  addi		r8,r7,4														; bump fifo pointer to next longword (r7 is current, r8 is next)
  
  rlwinm  r9,r8,0,27,31                     ; calc alignment for NEXT write (which would be X of vertex A)
  ori			r6,r6,$00c0                       ; fix up packet header
  
  bne+    @skipAlignPacketHeader            ; skip cache effage   (use CR0)
  
  dcbz    r0,r7                             ; clear new cache line
  dcbf    r7,r10                            ; flush previous cache line

@skipAlignPacketHeader:
	stwbrx  r6,r0,r7													; store packet header
	cmplwi  cr1,r9,$0000                      ; check alignment for write X  (set CR1)
	
	addi    r7,r8,4                           ; bump fifo pointer to next longword (r8 is now the current one)
	lwz     r3,0(r28)                         ; load X value for vertex A
	
	rlwinm. r0,r7,0,27,31                     ; check alignment for next write (Y)  (set CR0)
	bne+    cr1,@skipAlignAX                  ; skip cache effage (use CR1)
	
	dcbz    r0,r8                             ; clear
	dcbf    r8,r10                            ; flush
	
@skipAlignAX:
  stwbrx  r3,r0,r8                          ; store X
  addi    r8,r7,4                           ; r7 is current, r8 is next
  
  lwz     r3,4(r28)                         ; load Y value
  rlwinm  r9,r8,0,27,31                     ; calc alignment mask for next write
  
  bne+    @skipAlignAY                      ; skip cache effage (use CR0)
  
  dcbz    r0,r7                             ; clear
  dcbf    r7,r10                            ; flush
  
@skipAlignAY:
	stwbrx  r3,r0,r7                          ; store Y value
	cmplwi  r9,$0000                          ; check alignment for param 0 (or next vertex X)  (set CR0)
	
  lwz     r4,tsuDataList(r31)               ; get first data item index directly
	addi    r11,r31,tsuDataList               ; get pointer to data list (1st entry)
  
  addi		r7,r8,0                           ; get dest in correct register
  b       @dataLoopAEntry
   
@dataLoopAStart:
  lwzx    r6,r4,r28                         ; load data item
  addi    r7,r8,4                           ; r7 is next destination
  
  lwzu    r4,4(r11)                         ; load next index value
  bne+    @dataLoopAWrite                   ; skip cache effage  (use CR0)
  
  dcbz    r0,r8                             ; clear
	dcbf    r8,r10                            ; flush

@dataLoopAWrite:	
  stwbrx  r6,r0,r8                          ; store data
  rlwinm. r0,r7,0,27,31                     ; check alignment for next write (using cr0) 

@dataLoopAEntry:
  cmplwi  cr1,r4,$0000                      ; end of list?   (set CR1)
  addi    r8,r7,0                           ; update pointer to next write
  
  bne+    cr1, @dataLoopAStart              ; (use CR1)
 
; Now do vertex B
;
; relevant registers:
;
; r3 - temp vertex data
; r4 - temp index value
; r5 - XXX
; r6 - temp data storage
; r7 - alternat FIFO pointer (out of date)
; r8 - current FIFO pointer
; r9 - scratch pad for condition register calcs
; r10 - constant -4 for cache flushes
; r11 - pointer to data list

; cr0 - whether or not to do cache stuff on next write
; cr1 - trash 
    
  lwz     r3,0(r29)                         ; load X
  addi    r7,r8,4                           ; calc destination for Y
  
  rlwinm  r9,r7,0,27,31                     ; calc alignment mask for Y
  
  cmplwi  cr1,r9,$0000                      ; calc cr1 for Y write (set CR1)
  
  bne+    @skipAlignBX                      ; skip cache effage  (use CR0)
  
  dcbz    r0,r8                             ; clear
  dcbf    r8,r10                            ; flush
  
@skipAlignBX:
	stwbrx  r3,r0,r8                          ; store X
  addi    r8,r7,4                           ; r7 is current, r8 is next
  
  lwz     r3,4(r29)                         ; load Y value
  rlwinm  r9,r8,0,27,31                     ; calc alignment mask for next write
  
  bne+    cr1,@skipAlignBY                  ; skip cache effage (use CR1)
  
  dcbz    r0,r7                             ; clear
  dcbf    r7,r10                            ; flush
  
@skipAlignBY:
	stwbrx  r3,r0,r7                          ; store Y value
	cmplwi  r9,$0000                          ; check alignment for param 0 or next vertex X (set CR0)
	
  lwz     r4,tsuDataList(r31)               ; get first data item index directly
	addi    r11,r31,tsuDataList               ; get pointer to data list (1st entry)
  
  addi		r7,r8,0                           ; get dest in correct register
  b       @dataLoopBEntry
   
@dataLoopBStart:
  lwzx    r6,r4,r29                         ; load data item
  addi    r7,r8,4                           ; r7 is next destination
  
  lwzu    r4,4(r11)                         ; load next index value
  bne+    @dataLoopBWrite                   ; skip cache effage (use CR0)
  
  dcbz    r0,r8                             ; clear
	dcbf    r8,r10                            ; flush

@dataLoopBWrite:	
  stwbrx  r6,r0,r8                          ; store data
  rlwinm. r0,r7,0,27,31                     ; check alignment for next write (set CR0) 

@dataLoopBEntry:
  cmplwi  cr1,r4,$0000                      ; end of list? (set CR1)
  addi    r8,r7,0                           ; update pointer to next write
  
  bne+    cr1, @dataLoopBStart              ; (use CR1)
	  
; Now at last we do vertex C

  lwz     r3,0(r30)                         ; load X
  addi    r7,r8,4                           ; calc destination for Y
  
  rlwinm  r9,r7,0,27,31                     ; calc alignment mask for Y
  
  cmplwi  cr1,r9,$0000                      ; calc cr1 for Y write (set CR1)
  
  bne+    @skipAlignCX                      ; skip cache effage  (use CR0)
  
  dcbz    r0,r8                             ; clear
  dcbf    r8,r10                            ; flush
  
@skipAlignCX:
	stwbrx  r3,r0,r8                          ; store X
  addi    r8,r7,4                           ; r7 is current, r8 is next
  
  lwz     r3,4(r30)                         ; load Y value
  rlwinm  r9,r8,0,27,31                     ; calc alignment mask for next write
  
  bne+    cr1,@skipAlignCY                  ; skip cache effage (use CR1)
  
  dcbz    r0,r7                             ; clear
  dcbf    r7,r10                            ; flush
  
@skipAlignCY:
	stwbrx  r3,r0,r7                          ; store Y value
	cmplwi  r9,$0000                          ; check alignment for param 0 or next vertex X (set CR0)
	
  lwz     r4,tsuDataList(r31)               ; get first data item index directly
	addi    r11,r31,tsuDataList               ; get pointer to data list (1st entry)
  
  addi		r7,r8,0                           ; get dest in correct register
  b       @dataLoopCEntry
   
@dataLoopCStart:
  lwzx    r6,r4,r30                         ; load data item
  addi    r7,r8,4                           ; r7 is next destination
  
  lwzu    r4,4(r11)                         ; load next index value
  bne+    @dataLoopCWrite                   ; skip cache effage (use CR0)
  
  dcbz    r0,r8                             ; clear
	dcbf    r8,r10                            ; flush

@dataLoopCWrite:	
  stwbrx  r6,r0,r8                          ; store data
  rlwinm. r0,r7,0,27,31                     ; check alignment for next write (set CR0) 

@dataLoopCEntry:
  cmplwi  cr1,r4,$0000                      ; end of list? (set CR1)
  addi    r8,r7,0                           ; update pointer to next write
  
  bne+    cr1, @dataLoopCStart              ; (use CR1)

; WHEW!  Okay, now update FIFO pointer & fifo data availability before we go.
	      
	lwz     r4,fifoPtr(r31)                   ; get original fifo pointer
	
	lwz     r0,fifoRoom(r31)                  ; get fifo free
	
	subf    r4,r4,r8                          ; calc amount used      
	
	subf    r0,r4,r0                          ; calc new amount free
	
	stw     r0,fifoRoom(r31)                  ; save free
	
	stw     r8,fifoPtr(r31)                   ; save new fifo pointer
	    
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
