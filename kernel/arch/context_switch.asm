; ===========================================================================
; DevilCore OS — CPU Context Switching (x86_64)
; ===========================================================================
[BITS 64]

global switch_context
global thread_start_stub
extern thread_entry

section .text

; void switch_context(uint8_t **old_sp, uint8_t *new_sp);
; RDI = address to store old RSP (old_sp)
; RSI = next thread's RSP (new_sp)
switch_context:
    ; 1. Save callee-saved registers of current thread onto its stack
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; 2. Save current RSP to the variable pointed by RDI
    mov [rdi], rsp

    ; 3. Switch stack to the new thread's stack
    mov rsp, rsi

    ; 4. Restore the new thread's callee-saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ; 5. Return (pops RIP from the new stack)
    ret

; Used when a thread runs for the very first time.
; We structured the stack in `process.c` such that `switch_context`
; 'returns' to this stub. The stack currently holds R12 (entry function)
; and R13 (parameter).
thread_start_stub:
    ; Pass arguments to the C function `thread_entry`
    mov rdi, r12 ; 1st arg: function pointer
    mov rsi, r13 ; 2nd arg: function argument
    
    ; Call the wrapper
    call thread_entry

    ; If it ever returns, we halt (future: call thread_exit)
.hang:
    cli
    hlt
    jmp .hang
