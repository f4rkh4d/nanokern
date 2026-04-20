#pragma once
#include "types.h"

void vga_clear(void);
void vga_color(u8 fg, u8 bg);
void vga_putc(char c);
void vga_puts(const char *s);
// nonstandard: %d and %x consume u64 (no size modifier needed). format attribute
// intentionally omitted so gcc doesn't apply libc printf rules.
void vga_printf(const char *fmt, ...);
