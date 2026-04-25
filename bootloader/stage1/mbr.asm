; DevilCore 64-bit Boot Loader
; Stage 1 - Boot from BIOS/MBR
[BITS 16]
[ORG 0x7C00]

entry:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    ; Enable A20 line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Load Stage 2 (16KB from 0x1000:0)
    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Jump to Stage 2
    jmp 0x1000:0x0000

disk_error:
    mov si, msg_error
    call print
    cli
.halt:
    hlt
    jmp .halt

print:
    lodsb
    test al, al
    jz .ret
    mov ah, 0x0E
    int 0x10
    jmp print
.ret:
    ret

boot_drive db 0
msg_error db 'Disk Error', 0

times 510-($-$$) db 0
dw 0xAA55