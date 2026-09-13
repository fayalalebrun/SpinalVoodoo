bits 16
org 0x100

    push cs
    pop ds
    mov cx, 256
test_loop:
    mov eax, 0x12345678
    add eax, 0x123
    sub eax, 0x23
    xor edx, edx
    cmp eax, 0x12345778
    jne fail
    mov dword [target + 2], 0x11111111
    call target
    cmp eax, 0x11111111
    jne fail
    mov byte [target + 2], 0x22
    call target
    cmp eax, 0x11111122
    jne fail
    mov word [target + 2], 0x3344
    call target
    cmp eax, 0x11113344
    jne fail
    mov dword [target + 2], 0x55667788
    call target
    cmp eax, 0x55667788
    jne fail
    mov dword [target + 2], 0x55667788
    mov dword [datum], 0x99aabbcc
    call target
    cmp eax, 0x55667788
    jne fail
    cmp dword [datum], 0x99aabbcc
    jne fail
    movzx eax, cx
    mov dword [current + 2], eax
current:
    mov eax, 0x12345678
    cmp ax, cx
    jne fail
    dec cx
    jnz test_loop
%ifdef NEGATIVE_CONTROL
    jmp fail
%endif
    mov dx, passed
    jmp report
fail:
    mov dx, failed
report:
    push dx
    xor cx, cx
    mov dx, filename
    mov ah, 0x3c
    int 0x21
    jc io_error
    mov bx, ax
    pop dx
    mov cx, 4
    mov ah, 0x40
    int 0x21
    jc io_error
    cmp ax, 4
    jne io_error
    mov ah, 0x3e
    int 0x21
    jc io_error
    mov ax, 0x4c00
    int 0x21
io_error:
    mov ax, 0x4c01
    int 0x21
align 4
    nop
    nop
target:
    mov eax, 0x11111111
    ret
align 4
datum: dd 0
passed: db 'PASS'
failed: db 'FAIL'
%ifdef NEGATIVE_CONTROL
filename: db 'NEG.RES', 0
%else
filename: db 'SMC.RES', 0
%endif
