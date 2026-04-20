#include "types.h"
#include "vga.h"
#include "pic.h"
#include "idt.h"
#include "pit.h"
#include "kbd.h"
#include "heap.h"

static void timer_isr(u64 vec, u64 err) {
    (void)vec; (void)err;
    pit_ticks++;
}

static void kbd_isr(u64 vec, u64 err) {
    (void)vec; (void)err;
    u8 sc = kbd_scancode();
    kbd_push(sc);
}

static const char *banner =
    "nanokern 0.1.0 (x86-64, long mode)\n"
    "-----------------------------------\n";

void kmain(void) {
    vga_clear();
    vga_color(0x0f, 0x00);
    vga_puts(banner);

    vga_puts("[boot] long mode entered via multiboot\n");
    vga_puts("[boot] gdt loaded from boot.s\n");

    idt_install();
    vga_puts("[boot] idt installed (256 vectors)\n");

    pic_remap(0x20, 0x28);
    vga_puts("[boot] pic remapped (master=0x20, slave=0x28)\n");

    idt_register(32, timer_isr);   // irq0 -> 32
    idt_register(33, kbd_isr);     // irq1 -> 33

    pit_init(100);                 // 100 Hz -> 10 ms quantum
    vga_puts("[boot] pit armed at 100 hz (10 ms)\n");

    pic_unmask(0);
    pic_unmask(1);
    vga_puts("[boot] irq0 (timer) + irq1 (keyboard) unmasked\n");

    vga_printf("[boot] heap: 0 / %d bytes\n", (u64)kheap_size());
    vga_puts("[boot] enabling interrupts...\n\n");

    sti();

    u64 last = 0;
    vga_puts("init > ");
    for (;;) {
        // drain keyboard ring
        char c;
        while (kbd_read(&c)) {
            if (c == '\n') vga_puts("\ninit > ");
            else vga_putc(c);
        }
        // once a second, drop a heartbeat at the top-right corner
        if (pit_ticks / 100 != last) {
            last = pit_ticks / 100;
            volatile u16 *vga = (volatile u16*)0xb8000;
            vga[70] = (0x0a << 8) | '.';
            vga[71] = (0x0a << 8) | ' ';
            vga[72] = (0x0a << 8) | ('0' + (last % 10));
            vga[73] = (0x0a << 8) | 's';
        }
        hlt();
    }
}
