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
    ; Swap GS to get kernel data structure (swapgs) if we came from user mode
    ; For now we just push the state and call C

    push rcx ; Save return RIP
    push r11 ; Save RFLAGS
    
    ; Save general purpose registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    ; Set up the 4th argument (R10 in syscall ABI becomes RCX for C ABI)
    mov rcx, r10
    
    ; RAX is already the syscall number.
    ; C prototype: uint64_t syscall_handler(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);
    ; But we just let RAX be pushed, wait, we pass it via RDI as 1st arg, and shift everything? 
    ; Let's just push it all onto a struct and pass the struct pointer.

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
    
    add rsp, 56 ; Skip the 7 pushes
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    
    pop r11
    pop rcx
    
    ; Return to user space
    sysretq
