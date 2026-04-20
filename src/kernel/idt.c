#include "types.h"
#include "idt.h"
#include "pic.h"
#include "vga.h"

struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8  ist;
    u8  type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtr;

static irq_handler_t handlers[256];

void idt_set(u8 vec, void *stub, u8 type_attr) {
    uptr addr = (uptr)stub;
    idt[vec].offset_low  = addr & 0xffff;
    idt[vec].selector    = 0x08; // code segment from boot.s
    idt[vec].ist         = 0;
    idt[vec].type_attr   = type_attr;
    idt[vec].offset_mid  = (addr >> 16) & 0xffff;
    idt[vec].offset_high = (addr >> 32) & 0xffffffff;
    idt[vec].zero        = 0;
}

void idt_register(u8 vec, irq_handler_t h) { handlers[vec] = h; }

void isr_common_dispatch(u64 vec, u64 err) {
    if (handlers[vec]) {
        handlers[vec](vec, err);
    } else if (vec < 32) {
        vga_printf("\n!! exception vec=%d err=%x — halting\n", (int)vec, err);
        for (;;) { cli(); hlt(); }
    }
    if (vec >= 32 && vec < 48) pic_eoi(vec - 32);
}

extern void *isr_stubs[];

void idt_install(void) {
    for (int i = 0; i < 256; i++) idt_set(i, isr_stubs[i], 0x8e);
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (u64)idt;
    __asm__ volatile("lidt %0" :: "m"(idtr));
}
