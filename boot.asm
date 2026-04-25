; DevilCore OS - Boot Sector
[BITS 16]
[ORG 0x7C00]

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    mov ax, 0x0003
    int 0x10
    
    mov ax, 0xB800
    mov es, ax
    xor di, di
    
    mov ah, 0x1F
    mov al, ' '
    mov cx, 80
    rep stosw
    
    mov di, 80
    mov si, title
    call show
    
    mov di, 160
    mov si, icon1
    call show
    
    mov di, 1840
    mov cx, 80
    rep stosw
    
    mov di, 1850
    mov si, start
    call show
    
    mov ah, 0x04
    int 0x1A
    mov di, 1950
    mov si, clock
    call show
    mov al, ch
    call hex
    mov al, ':'
    stosw
    mov al, cl
    call hex
    mov al, ':'
    stosw
    mov al, dh
    call hex
    
    mov di, 1960
    mov si, prompt
    call show
    
    xor bp, bp

main:
    mov ah, 0x01
    int 0x16
    jz main
    
    mov ah, 0x00
    int 0x16
    
    cmp al, 13
    je enter
    cmp al, 8
    je back
    
    mov ah, 0x1F
    stosw
    inc bp
    jmp main

enter:
    mov ax, 0x0A0D
    stosw
    xor bp, bp
    mov di, 1980
    jmp main

back:
    test bp, bp
    jz main
    sub di, 2
    dec bp
    mov ax, 0x1F20
    stosw
    jmp main

show:
    lodsb
    test al, al
    jz .r
    mov ah, 0x1F
    stosw
    jmp show
.r:
    ret

hex:
    push ax
    shr al, 4
    add al, 48
    mov ah, 0x1F
    stosw
    pop ax
    and al, 15
    add al, 48
    mov ah, 0x1F
    stosw
    ret

title: db " DevilCore OS v3 ",0
icon1: db "[OK]",0
start: db "[Start]",0
clock: db "Time:",0
prompt: db ">",0

times 510-($-$$) db 0
dw 0xAA55