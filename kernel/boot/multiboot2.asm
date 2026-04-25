; ===========================================================================
; DevilCore OS - Multiboot2 Boot Entry Point (64-Bit Trampoline)
; Boots via GRUB2, establishes PAE Paging, enables Long Mode, formats CPU
; ===========================================================================
[BITS 32]

; ---- Multiboot2 constants -------------------------------------------------
MB2_MAGIC       equ 0xE85250D6
MB2_ARCH        equ 0               ; GRUB Multiboot2 expects Arch 0 for standard payload
MB2_HDRLEN      equ (mb2_end - mb2_start)
MB2_CHECKSUM    equ -(MB2_MAGIC + MB2_ARCH + MB2_HDRLEN) & 0xFFFFFFFF

section .multiboot2 progbits alloc noexec nowrite align=8

mb2_start:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd MB2_HDRLEN
    dd MB2_CHECKSUM

    ; --- Framebuffer tag (type=5) -------
    align 8
    dw 5            ; tag type: framebuffer
    dw 0            ; flags (0 = required)
    dd 20           ; tag size
    dd 1024         ; width
    dd 768          ; height
    dd 32           ; bpp

    ; --- End tag (type=0) ----------------
    align 8
    dw 0
    dw 0
    dd 8
mb2_end:

; ---- Long Mode Hardware Paging arrays (4-Level Page Tables) ---------------
section .bss
align 4096
global pml4_table
global pdp_table
global pd_table

pml4_table:
    resb 4096
pdp_table:
    resb 4096
pd_table_0:
    resb 4096
pd_table_1:
    resb 4096
pd_table_2:
    resb 4096
pd_table_3:
    resb 4096

_stack_bot:
    resb 131072
_stack_top:

; ---- 64-Bit Global Descriptor Table (GDT) ---------------------------------
section .rodata
align 8
gdt64:
    dq 0 ; null
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) ; executable, descriptor type, present, 64-bit
.data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41) ; descriptor type, present, writable data
.user_code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) | (3<<45) ; executable, descriptor type, present, 64-bit, DPL 3
.user_data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41) | (3<<45) ; descriptor type, present, writable data, DPL 3
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

; ---- 32-Bit Trampoline Entry ----------------------------------------------
section .text
global _start
extern kernel_main

_start:
    ; Verify GRUB passed correct magic (0x36d76289 for Multiboot2)
    cmp eax, 0x36d76289
    jne .hang

    mov esp, _stack_top
    mov edi, ebx ; save MB2 pointer safely in non-volatile register layout for 64bit

    ; Setup Page Tables (Identity Map 4GB)
    ; Point PML4[0] to PDP
    mov eax, pdp_table
    or eax, 3 ; present + writable
    mov [pml4_table], eax

    ; Point PDP[0..3] to PD0..PD3 (Total 4GB)
    mov eax, pd_table_0
    or eax, 3
    mov [pdp_table + 0], eax
    mov eax, pd_table_1
    or eax, 3
    mov [pdp_table + 8], eax
    mov eax, pd_table_2
    or eax, 3
    mov [pdp_table + 16], eax
    mov eax, pd_table_3
    or eax, 3
    mov [pdp_table + 24], eax

    ; Map 4 x 512 entries in 4 PD tables (4 x 512 * 2MB = 4GB identity map)
    mov ecx, 0
.map_pds:
    mov eax, 0x200000 ; 2MB page scale
    mul ecx
    or eax, 0x83 ; present + writable + huge (2MB flag)
    mov [pd_table_0 + ecx * 8], eax
    inc ecx
    cmp ecx, 2048 ; 4 * 512
    jne .map_pds

    ; Enable PAE (Physical Address Extension) in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Load CR3 with PML4
    mov eax, pml4_table
    mov cr3, eax

    ; Enable Long Mode Enable (LME) in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable Paging (PG) in CR0
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Load genuine 64-bit GDT
    lgdt [gdt64.pointer]

    ; Far jump out of 32-Bit Protected Mode into 64-Bit Sub-Segment
    jmp gdt64.code:.long_mode

.hang:
    cli
    hlt
    jmp .hang

; ---- 64-Bit Execution Context ---------------------------------------------
[BITS 64]
.long_mode:
    ; Clear old 32-bit pipeline registers
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Setup System V AMD64 ABI functional arguments
    ; RDI holds mb2_pointer currently (inherited from EDI above)
    mov rsi, rdi        ; arg 2 (rsi) = mb2_pointer
    mov rdi, 0x36d76289 ; arg 1 (rdi) = Magic Bytes Target

    call kernel_main

    cli
    hlt
