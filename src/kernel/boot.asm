bits 64
default rel

section .text
global _start
global gdt_flush
global tss_flush
global context_switch_asm
extern kernel_main
extern tss_set_rsp0
extern current_task

_start:
    cli
    lea rsp, [stack_top]
    xor rbp, rbp
    call kernel_main

.halt:
    hlt
    jmp .halt

gdt_flush:
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far return to reload CS
    pop rsi ; return address
    mov rax, 0x08
    push rax
    push rsi
    retfq

tss_flush:
    mov ax, 0x28 ; 5th entry in GDT (0x28 = 5 * 8)
    ltr ax
    ret

global context_switch_asm
context_switch_asm:
    cli
    cld
    
    ; rdi = old_task, rsi = new_task
    
    ; Set current_task = new_task
    mov [rel current_task], rsi
    
    ; Set new_task->state = TASK_RUNNING (1)
    mov dword [rsi + 84], 1 ; new_task->state offset 84 (uint32_t)
    
    ; Switch CR3 if new_task->cr3 != 0
    mov rcx, [rsi + 160] ; new_task->cr3 (offset 160)
    test rcx, rcx
    jz .no_cr3_switch
    
    mov cr3, rcx
    
.no_cr3_switch:
    test rdi, rdi        ; old_task is NULL?
    jz .new_task_only
    
    ; Save old task context (match the order we pop in .pop_new_context)
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
    pushfq
    
    ; Save old RSP to old_task->rsp
    mov [rdi + 152], rsp ; old_task->rsp offset 152
    
    ; Load new RSP
    mov rsp, [rsi + 152] ; new_task->rsp offset 152
    
    jmp .pop_new_context
    
.new_task_only:
    ; Load new RSP (for first switch to new task)
    mov rsp, [rsi + 152] ; new_task->rsp offset 152
    
.pop_new_context:
    ; Pop new task's context (reverse order of push)
    popfq
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    
    ; Ret to new task's entry point (on new stack)
    ret

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
