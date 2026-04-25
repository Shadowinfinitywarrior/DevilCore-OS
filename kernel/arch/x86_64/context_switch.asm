; kernel/arch/x86_64/context_switch.asm
; Context switch code for DevilKernel

[BITS 64]

global switch_context
global restore_context
global task_exit
global kernel_task_init

extern current_task_info

; void switch_context(uint8_t **old_sp, uint8_t *new_sp)
switch_context:
    ; Push all callee-saved registers onto the current stack
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; Save current RSP in *old_sp (RDI)
    mov [rdi], rsp
    
    ; Load new RSP from new_sp (RSI)
    mov rsp, rsi
    
    ; Pop callee-saved registers from the new stack
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    
    ret

; void restore_context(task_t *task)
restore_context:
    mov rsp, [rdi + 32]
    frstor [rdi]
    ret

; void task_exit(void)
task_exit:
    mov rdi, 0x1000
    mov rax, 1
    syscall
    
; Task initialization
kernel_task_init:
    ; Set up initial kernel task
    mov qword [rdi + 0], 0         ; pid
    mov qword [rdi + 8], 0         ; parent_pid
    mov qword [rdi + 16], 0        ; state
    mov qword [rdi + 24], 0        ; priority
    mov qword [rdi + 32], 0x200000 ; kernel stack
    mov qword [rdi + 40], 0x10     ; cs = kernel code
    mov qword [rdi + 48], 0x10     ; ds = kernel data
    mov qword [rdi + 56], 0        ; exit_status
    
    ; Initialize FPU
    fninit
    
    ret