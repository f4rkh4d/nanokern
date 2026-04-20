; nanokern bootstrap. multiboot1 entry (qemu -kernel + grub both accept this),
; 32-bit -> long mode, jump to kmain.

%define MB_MAGIC    0x1BADB002
%define MB_FLAGS    0x00000003                      ; align modules | provide mem info
%define MB_CHECKSUM -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .bss
align 4096
pml4:   resb 4096
pdpt:   resb 4096
pd:     resb 4096
stack_bottom:
    resb 16384
stack_top:

section .rodata
gdt64:
    dq 0                                              ; null
.code_desc: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)          ; code: exec, type=code, present, 64-bit
.data_desc: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41)                    ; data: type=data, present, writable
.ptr:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 32
global _start
extern kmain

_start:
    mov esp, stack_top
    mov edi, ebx                ; preserve multiboot info pointer -> rdi later (sysv abi)

    ; -------- paging: identity map first 1 GiB with 2 MiB pages --------
    mov eax, pdpt
    or eax, 0b11                 ; present | writable
    mov [pml4], eax

    mov eax, pd
    or eax, 0b11
    mov [pdpt], eax

    xor ecx, ecx
.fill_pd:
    mov eax, 0x200000
    mul ecx                      ; edx:eax = ecx * 2 MiB
    or eax, 0b10000011           ; present | writable | PS (huge)
    mov [pd + ecx*8], eax
    mov [pd + ecx*8 + 4], edx
    inc ecx
    cmp ecx, 512
    jne .fill_pd

    mov eax, pml4
    mov cr3, eax

    mov eax, cr4
    or  eax, 1 << 5              ; PAE
    mov cr4, eax

    mov ecx, 0xC0000080          ; EFER msr
    rdmsr
    or  eax, 1 << 8              ; LME
    wrmsr

    mov eax, cr0
    or  eax, 1 << 31             ; PG
    or  eax, 1 << 0              ; PE (already set, defensively)
    mov cr0, eax

    lgdt [gdt64.ptr]
    jmp gdt64.code_desc:long_mode_start

bits 64
long_mode_start:
    mov ax, gdt64.data_desc
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; edi holds multiboot info physical addr from 32-bit code
    ; rdi is zero-extended — good enough since we're identity-mapped below 1 GiB
    call kmain

    cli
.hang:
    hlt
    jmp .hang
