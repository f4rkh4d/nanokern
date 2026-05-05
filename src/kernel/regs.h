#pragma once
#include "types.h"

// snapshot of the caller-saved general-purpose registers as the asm stub
// pushed them in src/kernel/arch/isr_stubs.s. members are listed in *push*
// order so the struct is byte-compatible with the stack at dispatcher entry.
//
// the asm stub passes a pointer to the saved frame as the third arg to
// isr_common_dispatch, which in turn passes it to the registered handler.
// a syscall handler reads its arguments out of regs->{rax, rdi, rsi, rdx}
// and writes the return value back into regs->rax. the stub then pops the
// frame on the way out, so the modified rax becomes the syscall return
// value seen by the caller.
typedef struct {
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rdi;
    u64 rsi;
    u64 rdx;
    u64 rcx;
    u64 rax;
} regs_t;
