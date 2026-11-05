[org 0x7c00]
[bits 16]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    mov [0x0500], dl

    mov si, dap
    mov ah, 0x42
    int 0x13
    jc .disk_error

    mov dl, [0x0500]
    jmp 0:0x8000

.disk_error:
    mov si, err
.loop:
    lodsb
    test al, al
    jz .halt
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .loop
.halt:
    cli
    hlt
    jmp .halt

err: db "LiteC: disk read error", 0

dap:
    db 0x10, 0
    dw 4
    dw 0x8000
    dw 0x0000
    dd 1
    dd 0

times 510-($-$$) db 0
dw 0xaa55