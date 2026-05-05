; context_switch + new-task trampoline. preserves callee-saved registers per the
; SysV x86-64 ABI (rbx, rbp, r12-r15). caller-saved registers belong to the C frame
; above us and were already saved by the isr stub (for preempting paths) or do not
; need preserving (for cooperative paths).

bits 64

global context_switch
global task_entry_trampoline

extern task_exit

section .text

; void context_switch(u64 *old_rsp_save  /* rdi */,
;                     u64  new_rsp       /* rsi */);
context_switch:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov  [rdi], rsp     ; *old_rsp_save = rsp
    mov  rsp, rsi       ; rsp = new_rsp
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  rbx
    pop  rbp
    ret

; first run for a new task. ring 0, IF currently 0 (we got here via a context switch
; that started inside an interrupt gate). r12 holds the entry function pointer that
; task_spawn placed on the initial stack. enable interrupts, call entry, fall into
; task_exit if entry ever returns.
task_entry_trampoline:
    sti
    call r12
    cli
    call task_exit
.spin:
    hlt
    jmp .spin
