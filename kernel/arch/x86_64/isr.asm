; kernel/arch/x86_64/isr.asm
; Interrupt Service Routines for DevilKernel

[BITS 64]

global isr_stub_table
global page_fault_handler

extern interrupt_handler
extern current_task

; ISR stub template (will be copied to IDT)
%macro ISR_STUB 1
[BITS 64]
isr_stub_%1:
    ; Push error code (0 for interrupts without error code)
    %if (%1 == 8) || ((%1 >= 10 && %1 <= 14) && (%1 != 11 && %1 != 12)) || (%1 == 17) || (%1 >= 20)
    push qword 0
    %endif
    push qword %1
    jmp common_stub
%endmacro

; Define all ISR stubs (0-31)
ISR_STUB 0
ISR_STUB 1
ISR_STUB 2
ISR_STUB 3
ISR_STUB 4
ISR_STUB 5
ISR_STUB 6
ISR_STUB 7
ISR_STUB 8
ISR_STUB 9
ISR_STUB 10
ISR_STUB 11
ISR_STUB 12
ISR_STUB 13
ISR_STUB 14
ISR_STUB 15
ISR_STUB 16
ISR_STUB 17
ISR_STUB 18
ISR_STUB 19
ISR_STUB 20
ISR_STUB 21
ISR_STUB 22
ISR_STUB 23
ISR_STUB 24
ISR_STUB 25
ISR_STUB 26
ISR_STUB 27
ISR_STUB 28
ISR_STUB 29
ISR_STUB 30
ISR_STUB 31

; Common interrupt stub
common_stub:
    ; Save general purpose registers
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Save segment registers
    mov rax, ds
    push rax
    mov rax, es
    push rax
    mov rax, fs
    push rax
    mov rax, gs
    push rax

    ; Switch to kernel data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Get interrupt number
    mov rdi, [rsp + 12*8 + 8]  ; After saved registers + error code
    mov rsi, rsp              ; Stack pointer

    ; Call interrupt handler
    call interrupt_handler

    ; Restore and return from interrupt
    pop rax          ; GS
    mov gs, ax
    pop rax          ; FS
    mov fs, ax
    pop rax          ; ES
    mov es, ax
    pop rax          ; DS
    mov ds, ax

    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    add rsp, 16     ; Pop interrupt number and error code
    iretq

; CPU Exception handlers
[BITS 64]
divide_error:
    push 0
    push 0
    jmp common_stub

debug_error:
    push 0
    push 1
    jmp common_stub

nmi_interrupt:
    push 0
    push 2
    jmp common_stub

breakpoint:
    push 0
    push 3
    jmp common_stub

overflow:
    push 0
    push 4
    jmp common_stub

bound_range:
    push 0
    push 5
    jmp common_stub

invalid_opcode:
    push 0
    push 6
    jmp common_stub

device_not_available:
    push 0
    push 7
    jmp common_stub

double_fault:
    push 0
    push 8
    jmp common_stub

coprocessor_segment:
    push 0
    push 9
    jmp common_stub

invalid_tss:
    push 0
    push 10
    jmp common_stub

segment_not_present:
    push 0
    push 11
    jmp common_stub

stack_fault:
    push 0
    push 12
    jmp common_stub

general_protection:
    push 0
    push 13
    jmp common_stub

page_fault_handler:
    ; CR2 contains the faulting address
    mov rdi, cr2
    ; Error code pushed by CPU
    push qword 14
    jmp common_stub

x87_floating_point:
    push 0
    push 16
    jmp common_stub

alignment_check:
    push 0
    push 17
    jmp common_stub

machine_check:
    push 0
    push 18
    jmp common_stub

simd_floating_point:
    push 0
    push 19
    jmp common_stub

virtualization:
    push 0
    push 20
    jmp common_stub

; IRQ handlers (mapped to interrupts 32-47)
%macro IRQ_STUB 2
[BITS 64]
irq_stub_%1:
    push qword 0
    push qword %2
    jmp common_stub
%endmacro

IRQ_STUB 0, 32  ; IRQ0 - PIT
IRQ_STUB 1, 33  ; IRQ1 - Keyboard
IRQ_STUB 2, 34  ; IRQ2 - Cascade
IRQ_STUB 3, 35  ; IRQ3 - COM2
IRQ_STUB 4, 36  ; IRQ4 - COM1
IRQ_STUB 5, 37  ; IRQ5 - LPT2
IRQ_STUB 6, 38  ; IRQ6 - Floppy
IRQ_STUB 7, 39  ; IRQ7 - LPT1
IRQ_STUB 8, 40  ; IRQ8 - RTC
IRQ_STUB 9, 41  ; IRQ9 - ACPI
IRQ_STUB 10, 42 ; IRQ10 - Available
IRQ_STUB 11, 43 ; IRQ11 - Available
IRQ_STUB 12, 44 ; IRQ12 - Mouse
IRQ_STUB 13, 45 ; IRQ13 - Coprocessor
IRQ_STUB 14, 46 ; IRQ14 - ATA Primary
IRQ_STUB 15, 47 ; IRQ15 - ATA Secondary

; ISR stub table
isr_stub_table:
    dq isr_stub_0
    dq isr_stub_1
    dq isr_stub_2
    dq isr_stub_3
    dq isr_stub_4
    dq isr_stub_5
    dq isr_stub_6
    dq isr_stub_7
    dq isr_stub_8
    dq isr_stub_9
    dq isr_stub_10
    dq isr_stub_11
    dq isr_stub_12
    dq isr_stub_13
    dq isr_stub_14
    dq isr_stub_15
    dq isr_stub_16
    dq isr_stub_17
    dq isr_stub_18
    dq isr_stub_19
    dq isr_stub_20
    dq isr_stub_21
    dq isr_stub_22
    dq isr_stub_23
    dq isr_stub_24
    dq isr_stub_25
    dq isr_stub_26
    dq isr_stub_27
    dq isr_stub_28
    dq isr_stub_29
    dq isr_stub_30
    dq isr_stub_31
    dq irq_stub_0
    dq irq_stub_1
    dq irq_stub_2
    dq irq_stub_3
    dq irq_stub_4
    dq irq_stub_5
    dq irq_stub_6
    dq irq_stub_7
    dq irq_stub_8
    dq irq_stub_9
    dq irq_stub_10
    dq irq_stub_11
    dq irq_stub_12
    dq irq_stub_13
    dq irq_stub_14
    dq irq_stub_15