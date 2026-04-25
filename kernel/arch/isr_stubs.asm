; ===========================================================================
; DevilCore OS - 64-Bit Interrupt Service Routine Stubs
; Generates 256 Long Mode ISR stubs complying with System V AMD64 ABI
; ===========================================================================
[BITS 64]

extern isr_handler      ; C handler: void isr_handler(uint64_t no, uint64_t err)
global isr_table        ; table of function pointers exported to C

section .text

; ---- Common 64-bit stub ---------------------------------------------------
isr_common_stub:
    ; The CPU pushed: SS, RSP, RFLAGS, CS, RIP.
    ; Our specific macro pushed: Error Code (or 0), Interrupt Number.
    
    ; Save all general purpose registers
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

    ; The ABI requires arguments in RDI and RSI
    ; The stack pointer moved 15*8 = 120 bytes down
    mov rdi, [rsp + 120]    ; Arg 1: int_no
    mov rsi, [rsp + 128]    ; Arg 2: err_code

    cld
    call isr_handler

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
    
    ; Discard the interrupt number and error code (2 * 8 bytes = 16 bytes)
    add rsp, 16 

    iretq

; ---- Macro: exception without CPU error code ------------------------------
%macro ISR_NOERR 1
isr%1:
    push qword 0        ; dummy error code
    push qword %1       ; interrupt number
    jmp isr_common_stub
%endmacro

; ---- Macro: exception with CPU error code ---------------------------------
%macro ISR_ERR 1
isr%1:
    push qword %1       ; interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; ---- CPU Exceptions (0-31) ------------------------------------------------
ISR_NOERR 0     ; #DE Divide Error
ISR_NOERR 1     ; #DB Debug
ISR_NOERR 2     ; NMI
ISR_NOERR 3     ; #BP Breakpoint
ISR_NOERR 4     ; #OF Overflow
ISR_NOERR 5     ; #BR Bound Range
ISR_NOERR 6     ; #UD Invalid Opcode
ISR_NOERR 7     ; #NM Device Not Available
ISR_ERR   8     ; #DF Double Fault
ISR_NOERR 9     ; Coprocessor Segment Overrun
ISR_ERR   10    ; #TS Invalid TSS
ISR_ERR   11    ; #NP Segment Not Present
ISR_ERR   12    ; #SS Stack Fault
ISR_ERR   13    ; #GP General Protection
ISR_ERR   14    ; #PF Page Fault
ISR_NOERR 15    ; Reserved
ISR_NOERR 16    ; #MF Math Fault
ISR_ERR   17    ; #AC Alignment Check
ISR_NOERR 18    ; #MC Machine Check
ISR_NOERR 19    ; #XF SIMD Exception
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30    ; #SX Security Exception
ISR_NOERR 31

; ---- Generic Interrupts (32-255) ------------------------------------------
%assign i 32
%rep 224
    ISR_NOERR i
%assign i i+1
%endrep

; ---- ISR function pointer table (exported to C as 64-bit pointers) --------
section .rodata
global isr_table
isr_table:
%assign i 0
%rep 256
    dq isr%+i
%assign i i+1
%endrep
