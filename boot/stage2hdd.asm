[org 0x8000]
[bits 16]

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 128
%endif

%define VBE_INFO   0x7000
%define VBE_MODE   0x7200

stage2:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov [boot_drive], dl

    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc .fail

    ; ---- try to set VESA mode ----
    mov dword [0x0600], 0          ; clear magic
    mov ax, 0x4F00
    mov di, VBE_INFO
    int 0x10
    cmp ax, 0x004F
    jne .no_vesa

    ; 1024x768x32
    mov ax, 0x4F02
    mov bx, 0x4144
    int 0x10
        ; ---- force CRTC start address to 0 so the framebuffer origin is (0,0) ----
    mov dx, 0x3D4
    mov al, 0x0C
    out dx, al
    inc dx
    xor al, al
    out dx, al
    dec dx
    mov al, 0x0D
    out dx, al
    inc dx
    xor al, al
    out dx, al
        ; VBE_DISPI_DISPLAY_START = 0
    mov dx, 0x1CE
    mov al, 0x07
    out dx, al
    inc dx
    xor ax, ax
    out dx, ax
    inc dx
    out dx, ax
    cmp ax, 0x004F
    jne .try_800x600

    mov ax, 0x4F01
    mov cx, 0x0144
    mov di, VBE_MODE
    int 0x10
    jmp .got_mode

.try_800x600:
    mov ax, 0x4F02
    mov bx, 0x4143
    int 0x10
    cmp ax, 0x004F
    jne .no_vesa
    mov ax, 0x4F01
    mov cx, 0x0143
    mov di, VBE_MODE
    int 0x10

.got_mode:
    mov dword [0x0600], 0x424F4F54      ; "BOOT" magic

    mov eax, [VBE_MODE + 40]            ; PhysBasePtr         (0x28)
    mov [0x0604], eax

    movzx eax, word [VBE_MODE + 18]     ; XResolution         (0x12)  ← was +20
    mov [0x0608], eax

    movzx eax, word [VBE_MODE + 20]     ; YResolution         (0x14)  ← was +22
    mov [0x060C], eax

    movzx eax, word [VBE_MODE + 16]     ; BytesPerScanLine    (0x10)  ← was +18
    mov [0x0610], eax

    movzx eax, byte [VBE_MODE + 25]     ; BitsPerPixel        (0x19)  unchanged
    mov [0x0614], eax
.no_vesa:
    in al, 0x92
    or al, 2
    out 0x92, al

    cli
    lgdt [gdt_desc]

    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp 0x08:pm_entry

.fail:
    cli
    hlt
    jmp .fail

[bits 32]
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x10000

boot_drive: db 0

dap:
    db 0x10, 0
    dw KERNEL_SECTORS
    dw 0x0000
    dw 0x1000
    dd 5
    dd 0

gdt_start:
    dq 0
gdt_code:
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00
gdt_data:
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

times 2048-($-$$) db 0