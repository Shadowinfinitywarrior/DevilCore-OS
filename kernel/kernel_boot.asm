; kernel_boot.asm
; Multiboot 1 Assembly Entry Point for DevilCore

MBALIGN  equ  1 << 0            ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1            ; provide memory map
VIDMODE  equ  1 << 2            ; request video mode
FLAGS    equ  MBALIGN | MEMINFO | VIDMODE
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + FLAGS)   ; checksum

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0, 0, 0, 0, 0
    dd 0 ; linear graphics mode
    dd 1024 ; width
    dd 768  ; height
    dd 32   ; depth

section .bss
align 16
stack_bottom:
resb 16384 ; 16 KB
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; GRUB puts magic in eax, multiboot info pointer in ebx
    cli
    mov esp, stack_top
    
    ; push multiboot info struct pointer
    push ebx
    ; push magic value
    push eax

    ; Call the kernel_main
    call kernel_main

    ; If kernel_main returns (it shouldn't), halt the CPU
.hang:  hlt
        jmp .hang
