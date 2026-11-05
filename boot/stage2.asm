[org 0x8000]                        ; ← physical address base
[bits 16]

stage2:
    xor ax, ax
    mov ds, ax                      ; DS=0 → [label] resolves to physical addr
    mov es, ax
    mov [boot_drive], dl

    mov byte [cyl],        0
    mov byte [head],       0
    mov byte [sect],       6
    mov word [remaining],  128
    mov word [dest_seg],   0x1000
    mov word [dest_off],   0x0000

.read_loop:
    cmp word [remaining], 0
    je  .read_done

    xor ax, ax
    mov ds, ax                      ; re-set DS (BIOS may clobber)

    mov ah, 0x02
    mov al, 1
    mov ch, [cyl]
    mov dh, [head]
    mov cl, [sect]
    mov dl, [boot_drive]
    mov bx, [dest_off]
    push es
    mov es, [dest_seg]
    int 0x13
    pop es
    jc  .read_fail

    xor ax, ax
    mov ds, ax                      ; restore DS after BIOS call

    inc byte [sect]
    cmp byte [sect], 19
    jne .no_track_change
    mov byte [sect], 1
    inc byte [head]
    cmp byte [head], 2
    jne .no_track_change
    mov byte [head], 0
    inc byte [cyl]
.no_track_change:

    add word [dest_off], 512
    jnc .no_seg_inc
    mov word [dest_off], 0
    add word [dest_seg], 0x20
.no_seg_inc:

    dec word [remaining]
    jmp .read_loop

.read_fail:
    cli
    hlt
    jmp .read_fail

.read_done:
    in al, 0x92
    or al, 2
    out 0x92, al

    cli
    lgdt [gdt_desc]

    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp 0x08:pm_entry

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
cyl:        db 0
head:       db 0
sect:       db 0
remaining:  dw 0
dest_seg:   dw 0
dest_off:   dw 0

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
    dd gdt_start                    ; now = 0x8000 + offset (physical)

times 2048-($-$$) db 0