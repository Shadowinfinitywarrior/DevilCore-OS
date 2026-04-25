; DevilCore 64-bit Stage 2 Loader
[BITS 16]
[ORG 0x0000]

start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    
    ; Enable A20
    in al, 0x92
    test al, 2
    jnz .a20_ok
    or al, 2
    out 0x92, al
.a20_ok:
    
    ; Load kernel
    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov cl, 33
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    
    ; Load GDT
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

boot_drive db 0

gdt_start:
    dq 0
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_end:
gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Clear page tables
    mov edi, 0x20000
    xor eax, eax
    mov ecx, 4096
    rep stosd
    
    ; PML4 -> PDPT
    mov dword [0x20000], 0x21003
    
    ; PDPT -> PD
    mov dword [0x21000], 0x22003
    
    ; PD (2MB pages)
    mov edi, 0x22000
    mov eax, 0x00000083
    mov ecx, 512
.pd_loop:
    stosd
    add eax, 0x200000
    loop .pd_loop
    
    ; Enable PAE
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax
    
    ; Load PML4
    mov eax, 0x20000
    mov cr3, eax
    
    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    
    ; Load 64-bit GDT
    lgdt [gdt64_desc]
    jmp 0x08:long_mode

[BITS 64]
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, 0x200000
    
    ; Check kernel magic
    cmp dword [0x100000], 0x464C457F
    jne .hang
    
    ; Jump to kernel
    mov rax, [0x100000 + 0x18]
    call rax

.hang:
    cli
    hlt
    jmp .hang

gdt64_start:
    dq 0
gdt64_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xAF
    db 0x00
gdt64_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xAF
    db 0x00
gdt64_end:
gdt64_desc:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

times 16384 - ($ - $$) db 0