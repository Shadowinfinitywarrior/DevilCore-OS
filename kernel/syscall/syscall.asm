; ===========================================================================
; DevilCore OS — System Call Entry Point (x86_64 SYSCALL instruction)
; ===========================================================================
[BITS 64]

global syscall_entry
extern syscall_handler

; The AMD64 SYSCALL instruction does the following:
; 1. Loads RIP from IA32_LSTAR MSR
; 2. Loads CS and SS from IA32_STAR MSR
; 3. Saves old RIP to RCX
; 4. Saves old RFLAGS to R11
; User-space arguments are passed in: RDI, RSI, RDX, R10, R8, R9
; RAX holds the syscall number

section .text

syscall_entry:
    ; Align stack to 16 bytes (we push 15 regs + 1 padding)
    push 0
    push rcx ; Save return RIP
    push r11 ; Save RFLAGS
    
    ; Save general purpose registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    ; Save caller-saved registers
    push r9
    push r8
    push r10 ; mapped to arg4 in syscall
    push rdx
    push rsi
    push rdi
    push rax ; The syscall number
    
    mov rdi, rsp ; Pass pointer to the register struct
    
    call syscall_handler
    
    ; RAX now contains return value
    mov [rsp], rax ; Update RAX in the struct so it gets popped back correctly
    
    ; Restore all registers from the potentially modified struct
    pop rax
    pop rdi
    pop rsi
    pop rdx
    pop r10
    pop r8
    pop r9
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop r11
    pop rcx
    add rsp, 8 ; Pop the padding
    
    ; Return to user space
    sysretq
