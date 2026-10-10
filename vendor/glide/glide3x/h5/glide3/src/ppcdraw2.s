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
		
	MACRO
	MakeFunction &fnName
		EXPORT &fnName[DS]
 		EXPORT .&fnName[PR]
		
		TC &fnName[TC], &fnName[DS]
			
		CSECT &fnName[DS]
			DC.L .&fnName[PR]
 			DC.L TOC[tc0]
		
		CSECT .&fnName[PR]
		FUNCTION .&fnName[PR]	
		
	ENDM

  INCLUDE 'PPCInline.inc'
  
; ======================================================================
; external global variables
; ======================================================================
	
	import _threadValueMacOS
	import ._grCommandTransportMakeRoom
	import ._grValidateState
	
	toc
		tc _threadValueMacOS[TC], _threadValueMacOS	

	IF 1

linkageArea:		  set 24	; constant comes from the PowerPC Runtime Architecture Document
CalleesParams:		set	32	; always leave space for GPR's 3-10
CalleesLocalVars:	set 0	  ; ClickHandler doesn't have any
numGPRs:			    set 7	  ; num volitile GPR's (GPR's 13-31)
numFPRs:			    set 9	  ; num volitile FPR's (FPR's 14-31)

spaceToSave:	set linkageArea + CalleesParams + CalleesLocalVars + 4*numGPRs + 8*numFPRs  

  MakeFunction  _trisetup_Default_win_cull_invalid
  
_trisetup_Default_win_cull_invalid_begin:
  
  mflr    r7                                ; Get return address in volatile register  
  lwz     r0,invalid(r6)									  ; load state flag

  stw     r28,-16(sp)  
  addi    r28,r3,0 
  
  stw     r29,-12(sp)  
	cmplwi  r0,$0000													; check state flag (sets cr0)
  
	lwz     r0,cull_mode(r6)									; load cull mode
  addi    r29,r4,0  

  stw     r30,-8(sp)  
  addi    r30,r5,0  
  
  stw     r31,-4(sp)
	cmplwi  cr1,r0,$0000                      ; check culling mode (sets cr1)
  
  stw     r7,8(sp)	
  addi    r31,r6,0  
	
  stwu    sp,-128(sp)
		
	beq+    @noValidate												; check valid state flag (via cr0), predict branch to true
	  
	bl      ._grValidateState
	nop
	
	lwz     r0,cull_mode(r31)                 ; reload cull mode into r0

  ; 1 cycle stall
    
  cmplwi  cr1,r0,$0000                      ; redo cull check since cr1 could have been trashed

  align   3
  
@noValidate:
  beq-    cr1,@noCull                       ; check for culling being disabled, predict as not taken

  lfs     fp4,4(29)                         ; load bY 
  rlwinm  r6,r0,31,0,0                      ; get cull mode in top bit
  
  lfs     fp0,4(r28)                        ; load aY
  	  
  lfs     fp5,0(r29)                        ; load bX
  
  lfs     fp1,0(r30)                        ; load cX
  fsubs   fp0,fp0,fp4                       ; fp0 = dyAB
  
  lfs     fp3,0(r28)                        ; load aX
  
  fsubs   fp1,fp5,fp1                       ; fp1 = dxBC
  lfs     fp2,4(r30)                        ; load cY
  
  fsubs   fp3,fp3,fp5                       ; fp3 =  dxAB
  
  fsubs   fp2,fp4,fp2                       ; fp2 = dyBC
  
  fmuls   fp0,fp1,fp0                       ; fp0 = dxBC * dyAB
  
  ; 2 cycle stall
  
  fmsubs  fp0,fp3,fp2,fp0                   ; fp0 = dxAB * dyBC - dxBC * dyAB

  ; 2 cycle stall
  
  stfs    fp0,-32(sp)
  
  ; 1 cycle stall
  
  lwz     r7,-32(sp)

  ; 1 cycle stall
  
  rlwinm. r0,r7,0,1,31                      ; check for zero area
  
  bne+    @nonZeroArea											; okay so far...
  
  li      r3,$0000
  
@culledReturn:
  
  lwz     r0,136(sp)
  
  addi    sp,sp,128
  
; 1 cycle stall
  
  mtlr    r0  
  lwz     r28,-16(sp)    
  lwz     r29,-12(sp)
  lwz     r30,-8(sp)
  lwz     r31,-4(sp)
  blr
  
  align   3                                  ; allign branch target doubleword address
  
@nonZeroArea:  
  xor.    r0,r7,r6                           ; check culling
  blt     @noCull
  
  li      r3,-1
  b       @culledReturn

  align   3
  
@noCull:

  INCLUDE  'ppcdraw2.inc'
  
_trisetup_Default_win_cull_invalid_end:
  

  MakeFunction  _trisetup_Default_win_nocull_invalid
    
_trisetup_Default_win_nocull_invalid_begin:
  
  mflr    r7                                ; Get return address in volatile register  
  lwz     r0,invalid(r6)									  ; load state flag

  stw     r28,-16(sp)  
  addi    r28,r3,0 
  
  stw     r29,-12(sp)  
	cmplwi  r0,$0000													; check state flag (sets cr0)
  
  addi    r29,r4,0  

  stw     r30,-8(sp)  
  addi    r30,r5,0  
  
  stw     r31,-4(sp)
  
  stw     r7,8(sp)	
  addi    r31,r6,0  
	
  stwu    sp,-128(sp)
		
	beq+    @noValidate												; check valid state flag (via cr0), predict branch to true
	  
	bl      ._grValidateState
	nop
	
  ; 1 cycle stall
    
  align   3
  
@noValidate:

  INCLUDE  'ppcdraw2.inc'
  
_trisetup_Default_win_nocull_invalid_end:
    

  MakeFunction  _trisetup_Default_win_cull_valid
  
_trisetup_Default_win_cull_valid_begin:
  
  mflr    r7                                ; Get return address in volatile register  
	lwz     r0,cull_mode(r6)									; load cull mode

  stw     r28,-16(sp)  
  addi    r28,r3,0 
  
  stw     r29,-12(sp)  
	cmplwi  cr1,r0,$0000                      ; check culling mode (sets cr1)
  
  addi    r29,r4,0  

  stw     r30,-8(sp)  
  addi    r30,r5,0  
  
  stw     r31,-4(sp)
  
  stw     r7,8(sp)	
  addi    r31,r6,0  
	
  stwu    sp,-128(sp)
		
  beq-    cr1,@noCull                       ; check for culling being disabled, predict as not taken

  lfs     fp4,4(29)                         ; load bY 
  rlwinm  r6,r0,31,0,0                      ; get cull mode in top bit
  
  lfs     fp0,4(r28)                        ; load aY
  	  
  lfs     fp5,0(r29)                        ; load bX
  
  lfs     fp1,0(r30)                        ; load cX
  fsubs   fp0,fp0,fp4                       ; fp0 = dyAB
  
  lfs     fp3,0(r28)                        ; load aX
  
  fsubs   fp1,fp5,fp1                       ; fp1 = dxBC
  lfs     fp2,4(r30)                        ; load cY
  
  fsubs   fp3,fp3,fp5                       ; fp3 =  dxAB
  
  fsubs   fp2,fp4,fp2                       ; fp2 = dyBC
  
  fmuls   fp0,fp1,fp0                       ; fp0 = dxBC * dyAB
  
  ; 2 cycle stall
  
  fmsubs  fp0,fp3,fp2,fp0                   ; fp0 = dxAB * dyBC - dxBC * dyAB

  ; 2 cycle stall
  
  stfs    fp0,-32(sp)
  
  ; 1 cycle stall
  
  lwz     r7,-32(sp)

  ; 1 cycle stall
  
  rlwinm. r0,r7,0,1,31                      ; check for zero area
  
  bne+    @nonZeroArea											; okay so far...
  
  li      r3,$0000
  
@culledReturn:
  
  lwz     r0,136(sp)
  
  addi    sp,sp,128
  
; 1 cycle stall
  
  mtlr    r0  
  lwz     r28,-16(sp)    
  lwz     r29,-12(sp)
  lwz     r30,-8(sp)
  lwz     r31,-4(sp)
  blr
  
  align   3                                  ; allign branch target doubleword address
  
@nonZeroArea:  
  xor.    r0,r7,r6                           ; check culling
  blt     @noCull
  
  li      r3,-1
  b       @culledReturn

  align   3
  
@noCull:

  INCLUDE  'ppcdraw2.inc'
  
_trisetup_Default_win_cull_valid_end:
    

  MakeFunction  _trisetup_Default_win_nocull_valid
  
_trisetup_Default_win_nocull_valid_begin:
  
  mflr    r7                                ; Get return address in volatile register  

  stw     r28,-16(sp)  
  addi    r28,r3,0 
  
  stw     r29,-12(sp)    
  addi    r29,r4,0  

  stw     r30,-8(sp)  
  addi    r30,r5,0  
  
  stw     r31,-4(sp)
  addi    r31,r6,0  
  
  stw     r7,8(sp)	
	
  stwu    sp,-128(sp)
		
  INCLUDE  'ppcdraw2.inc'
  
_trisetup_Default_win_nocull_valid_end:
  
  ENDIF
  
  IF 1 
  
; About 3 clock cycles of stalls in the code below, waiting for loads. (But
; about 12 clocks that could be used for something else)

	MakeFunction	grDrawTriangle

  lwz     r6,_threadValueMacOS[TC](RTOC)    ; Get pointer to thread value
  lwz     r6,0(r6)                          ; get thread value (gc)
  addis   r7,r6,1                           ; add 64K
  lwz     r12,triSetupProc(r7)              ; load triangle dispatch vector
  lwz     r7,0(r12)                         ; grab function pointer
  lwz     rtoc,4(r12)                       ; load new TOC
  mtctr   r7                                ; move to ctr  
  bctr                                      ; jump to triangle setup function

	ENDIF