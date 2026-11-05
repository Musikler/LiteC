[org 0x7c00]
[bits 16]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    mov [0x0500], dl

    mov ah, 0x02
    mov al, 4
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [0x0500]
    mov bx, 0x8000
    int 0x13
    jc disk_error

    mov dl, [0x0500]
    jmp 0:0x8000                    ; ← CS=0, IP=0x8000 (was 0x0800:0x0000)

disk_error:
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

times 510-($-$$) db 0
dw 0xaa55
