; isr stubs. 256 entries. vectors 8, 10-14, 17, 21 push an error code; the rest don't,
; so we push a dummy 0 to keep the stack layout uniform.

bits 64
extern isr_common_dispatch

%macro ISR_NOERR 1
isr%1:
    push qword 0
    push qword %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
isr%1:
    push qword %1
    jmp isr_common
%endmacro

section .text

isr_common:
    ; stack (top): rip, cs, rflags, ss, rsp (from cpu), then our: vec, err
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11

    mov rdi, [rsp + 9*8]       ; vec
    mov rsi, [rsp + 10*8]      ; err
    mov rdx, rsp               ; pointer to saved-regs frame (regs_t*)
    call isr_common_dispatch

    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    add rsp, 16                 ; discard vec + err
    iretq

%assign i 0
%rep 256
    %if (i == 8) || (i == 10) || (i == 11) || (i == 12) || (i == 13) || (i == 14) || (i == 17) || (i == 21)
        ISR_ERR i
    %else
        ISR_NOERR i
    %endif
    %assign i i+1
%endrep

section .rodata
global isr_stubs
isr_stubs:
%assign i 0
%rep 256
    dq isr%+i
    %assign i i+1
%endrep
