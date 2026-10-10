## -*-asm-*-
## THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
## PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
## TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
## INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
## DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
## THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
## EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
## FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
## 
## USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
## RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
## TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
## AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
## SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
## THE UNITED STATES.  
## 
## COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
##
# 
# 2     10/30/97 6:53p Peter
# first real cut at tri asm
# 
# 1     10/30/97 4:29p Peter
# asm tri code
# 
# 2     7/07/97 2:14p Jdt
# assembly now on par with C code.
# 
# 1     7/07/97 8:37a Jdt
# B4 Chip field fix.
##

.file "xdraw2.inc.s"
#.586P
        
#EXTRN   _GlideRoot:DWORD
#EXTRN   _FifoMakeRoom:NEAR

# Ugly, but seems to workaround the problem with locally defined
# data segment globals not getting relocated properly when using
# djgpp.

#zArea   .EQU One+4
#zdxAB   .EQU One+8
#zdxBC   .EQU One+12
#zdyAB   .EQU One+16
#zdyBC   .EQU One+20
#zculltest .EQU One+24

gc      .REG    (%esi)       # points to graphics context

    ## Prologue stuff
    push    %esi
    push    %edi

    push    %ebx	
    push    %ebp

    mov     (_GlideRoot + curGC), gc    ## GR_DCL_GC

.align 4
.AIF \&GLIDE_CULLING EQ 1
fa      .REG     (%eax)       # vtx a from caller
fb      .REG     (%ebx)       # vtx b from caller
fc      .REG     (%ecx)       # vtx c from caller

cull    .REG     (%edx)       
intArea .REG     (%ebp)       # temp Y storage        
    
    ## Pre-load the current culling mode before all of the
    ## floating point area stuff.
    mov     cull_mode(gc), cull
    mov     _va$(%esp), fa

    mov	    _vb$(%esp), fb
    mov     _vc$(%esp), fc
	
    shl     $31, cull
				
 SYMNAME Area_Computation
# 47-3
# jmp ret_pop0f
    flds    X(fa)		#  xa
    fsubs   X(fb)		#  dxAB
    flds    X(fb)		#  |    xb
    fsubs   X(fc)		#  |    dxBC
    flds    Y(fb)		#  |    |    yb
    fsubs   Y(fc)		#  |    |    dyBC
    flds    Y(fa)		#  |    |     |   ya
    fsubs   Y(fb)		#  |    |     |   dyAB
    fld     %st(3)              #  |    |    |    |    dxAB
    fmul    %st(2), %st         #  |    |    |    |    t0         t0=dxAB*dyBC
    fld     %st(3)              #  |    |    |    |    |    dxBC
    fmul    %st(2), %st         #  |    |    |    |    |    t1    t1=dxBC*dyAB
    fsubrp  %st, %st(1)         #  |    |    |    |    area
    fsts    zArea	        #  |    |    |    |    area

    ## Pop temp things from the sw culling off the fp stack
    fstp    %st   # 4
    fstp    %st   # 3
    fstp    %st   # 2
    fstp    %st   # 1
    fstp    %st   # 0    
	
    mov     zArea, intArea        # j = *(long *)&area
    xor     %eax, %eax		  # Clear the returnv value (0 == culled)
	
    # Zero Area Triangle Check
    and     $0x7fffffff, intArea  # if ((j & 0x7FFFFFFF) == 0)
 SYMNAMEOP   jz,      __triDone

    ## Triangle area check vs culling mode
    mov     zArea, intArea
    xor     cull, intArea

 SYMNAMEOP    jge,     __triDone

.AENDI
.align 4
    ## Check to make sure that we have enough room for
    ## the complete triangle packet.
    mov	    (_GlideRoot + curTriSize), %eax
    mov	    fifoRoom(gc), %ebx
	
    add	    $4, %eax
    cmp	    %eax, %ebx
	
 SYMNAMEOP jge,	      __triBegin

    push    $0
    push    $0
    
    push    %eax
    call    _FifoMakeRoom

    add	    $12, %esp

    ## Send triangle parameters
	
dlp     .REG     (%ebx)		# points to dataList structure
fifo    .REG     (%ebp)		# points to next entry in fifo
vertex  .REG	 (%edx)		# the current vertex
vOffset .REG	 (%ecx)		# Current vertex offset

packCol .REG	    (%edi)
tempVal .REG	    (%edi)

.MACRO GR_FIFO_WRITE __addr __offset __data	
    mov    \__data, \__offset(\__addr)
.ENDM # GR_FIFO_WRITE

.align 4
 SYMNAME __triBegin
    mov     fifoPtr(gc), fifo		# Fetch Fifo Ptr
    mov	    $4, vOffset			# Starting vertex    

    mov	    triPacketHdr(gc), %eax 	# Packet 3 header
    nop

					# Write packet header to fifo    
    GR_FIFO_WRITE fifo, 0, %eax       
    add	    $4, fifo			# Advance fifo for hdr & x/y coordinate

.align 4
 SYMNAME __vertexStart
    mov	    STKOFF(%esp,vOffset), vertex	# Current vertex
    add	    $8, fifo    

    nop						# Avoid p5 agi w/ load of vertex ptr
    nop
    
    movl    X(vertex), %eax			# X
    lea	    tsuDataList(gc), dlp		# Reset the dataList
						# PCI write X
    GR_FIFO_WRITE fifo, -8, %eax
    movl    Y(vertex), %eax			# Y 

    xor	    packCol, packCol			# Clear packed color
						# PCI write Y
    GR_FIFO_WRITE fifo, -4, %eax

.AIF \&GLIDE_PACK_RGB EQ 1
    fldl    b(vertex)				# B
    faddl   (_GlideRoot + fBiasLo)		# BC GC

    fldl    g(vertex)				# G B
    faddl   (_GlideRoot + fBiasHi)		# GC B
    
    fldl    r(vertex)				# R GC BC
    faddl   (_GlideRoot + fBiasHi)		# RC GC BC

    fxch    %st(2)				# BC GC RC
    fstpl   bias0				# GC RC

    fstpl   bias1				# RC
    movl    bias0, packCol			# B + bias

    fstpl   bias0
    movl    bias1, %eax				# G + bias
    
.AIF \&GLIDE_PACK_ALPHA EQ 1
    fldl    a(vertex)
    faddl   (_GlideRoot + fBiasHi)

    and	    $0x00FF, packCol			# B color component
    and	    $0x0000FF00, %eax			# G component << 8

    add	    $8, dlp				# Packed RGB + A dataList entry
    nop

    or	    %eax, packCol			# 0000GGBB
    nop

    fstpl   bias1
    movl    bias0, %eax				# R + bias
    
    movl    bias1, %esi				# A + bias
    and	    $0x0000FF00, %eax			# R component << 8
    
    and	    $0x0FFFFFF00, %esi			# A component << 8
    shl	    $8, eax				# R << 16
    
    or	    %eax, packCol			# 00RRGGBB
    shl	    $16, %esi				# A << 16

    or	    %esi, packCol			# AARRGGBB
    nop
.AELSE # !GLIDE_PACK_ALPHA    
    and	    $0x00FF, packCol			# B color component
    and	    $0x0000FF00, %eax			# G component << 8

    add	    $4, dlp				# Next dataList item    
    or	    %eax, packCol
    
    movl    bias0, %eax				# R + bias
    and	    $0x0000FF00, %eax			# R component << 8

    shl	    $8, eax				# R << 16
    or	    %eax, packCol			# 00RRGGBB
.AENDI # !GLIDE_PACK_ALPHA

						# PCI write packed color value
    GR_FIFO_WRITE fifo, 0, packCol
    add	    $4, fifo
.AENDI # GLIDE_PACK_RGB

 SYMNAME __doParams
    movl    (dlp), %eax				# Get first offset from the data list
    add     $4, dlp				# dlp++

    cmp	    $0, %eax				# Are we done?
 SYMNAMEOP    je,	    __nextVertex

    ## Not using align directive here because it sometimes
    ## introduces an agi for the eax use below.
    nop
    nop
 SYMNAME __paramLoop
    movl    (%eax, vertex), tempVal
    add	    $4, fifo
    
    movl    (dlp), %eax
    add     $4, dlp
    
    cmp	    $0, %eax
    GR_FIFO_WRITE fifo, -4, tempVal

 SYMNAMEOP    jne,	    __paramLoop

.align 4
 SYMNAME __nextVertex
    ## On to the next vertex
    add	    $4, vOffset
    mov     (_GlideRoot + curGC), gc		# Reload gc incase we trashed it as a temp

    cmp	    $16, vOffset			# Offset of one past last vertex?

 SYMNAMEOP   jne,	    __vertexStart

    ## Update gc->fifoPtr and gc->fifoRoom
    mov	    fifo, %eax
    mov     fifoPtr(gc), %ebx
	
    mov	    fifo, fifoPtr(gc)
    sub     %ebx, %eax

    mov     (_GlideRoot + trisDrawn), %ebx	
    sub	    %eax, fifoRoom(gc)

    add     $1, %ebx
    mov     %ebx, (_GlideRoot + trisDrawn)

    ## return 1 (triangle drawn)
    mov     $1, %eax

 SYMNAME __triDone
    ## Restore trashed registers
    mov     (_GlideRoot + trisProcessed), %esi
    pop     %ebp

    add     $1, %esi
    pop     %ebx
	
    pop     %edi
    mov     %esi, (_GlideRoot + trisProcessed)

    pop     %esi
    ret
