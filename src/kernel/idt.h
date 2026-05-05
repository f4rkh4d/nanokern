#pragma once
#include "regs.h"
#include "types.h"

// handler signature: vector + cpu-pushed error code + pointer to the
// caller-saved register frame. the regs pointer lets syscall handlers
// read arguments out of rax/rdi/rsi/rdx and write a return value back
// into rax. irq handlers can ignore it.
typedef void (*irq_handler_t)(u64 vec, u64 err, regs_t *regs);

void idt_install(void);
void idt_register(u8 vec, irq_handler_t h);
