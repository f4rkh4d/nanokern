#pragma once
#include "types.h"
#include "regs.h"

// syscall numbers passed in rax. linux uses 0/1/2/... = read/write/open/...;
// we deliberately renumber from 1 so that the very first slot stays free
// for SYS_NOP / debugger sentinels.
#define SYS_WRITE   1   // a0 = char to write to VGA
#define SYS_PUTS    2   // a0 = ptr, a1 = len; writes a string to VGA
#define SYS_GETPID  3   // returns current task id in rax
#define SYS_SLEEP   4   // a0 = ticks (10 ms each); blocks current task
#define SYS_EXIT    5   // ends current task, never returns

// dispatch entry registered on vector 0x80.
void syscall_isr(u64 vec, u64 err, regs_t *regs);
