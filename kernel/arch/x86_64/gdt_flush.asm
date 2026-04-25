; kernel/arch/x86_64/gdt_flush.asm

[BITS 64]
global gdt_flush

gdt_flush:
    lgdt    [rdi]           ; Load GDT from pointer in RDI

    ; Reload code segment with far return
    push    0x08            ; Kernel code segment
    lea     rax, [rel .reload]
    push    rax
    retfq

.reload:
    ; Reload data segments
    mov     ax, 0x10        ; Kernel data segment
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    ret
