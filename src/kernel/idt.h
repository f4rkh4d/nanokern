#pragma once
#include "types.h"

typedef void (*irq_handler_t)(u64 vec, u64 err);

void idt_install(void);
void idt_register(u8 vec, irq_handler_t h);
