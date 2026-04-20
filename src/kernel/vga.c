#include "types.h"
#include "vga.h"

#define VGA_MEM  ((volatile u16*)0xb8000)
#define COLS 80
#define ROWS 25

static u16 row = 0;
static u16 col = 0;
static u8  attr = 0x0f; // white on black

static void scroll(void) {
    for (int r = 1; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            VGA_MEM[(r - 1) * COLS + c] = VGA_MEM[r * COLS + c];
        }
    }
    for (int c = 0; c < COLS; c++) VGA_MEM[(ROWS - 1) * COLS + c] = ((u16)attr << 8) | ' ';
    row = ROWS - 1;
}

void vga_clear(void) {
    for (int i = 0; i < COLS * ROWS; i++) VGA_MEM[i] = ((u16)attr << 8) | ' ';
    row = col = 0;
}

void vga_color(u8 fg, u8 bg) { attr = (bg << 4) | (fg & 0x0f); }

void vga_putc(char c) {
    if (c == '\n') { col = 0; row++; }
    else if (c == '\r') { col = 0; }
    else if (c == '\t') { col = (col + 8) & ~7; }
    else { VGA_MEM[row * COLS + col] = ((u16)attr << 8) | (u8)c; col++; }
    if (col >= COLS) { col = 0; row++; }
    if (row >= ROWS) scroll();
}

void vga_puts(const char *s) { while (*s) vga_putc(*s++); }

static void put_hex(u64 n, int width) {
    static const char *hex = "0123456789abcdef";
    char buf[17];
    buf[16] = 0;
    for (int i = 15; i >= 0; i--) { buf[i] = hex[n & 0xf]; n >>= 4; }
    int start = 16 - width;
    if (start < 0) start = 0;
    vga_puts(&buf[start]);
}

static void put_dec(u64 n) {
    char buf[21];
    int i = 20;
    buf[i--] = 0;
    if (n == 0) { vga_putc('0'); return; }
    while (n > 0 && i >= 0) { buf[i--] = '0' + (n % 10); n /= 10; }
    vga_puts(&buf[i + 1]);
}

void vga_printf(const char *fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { vga_putc(*p); continue; }
        p++;
        switch (*p) {
            case 's': vga_puts(__builtin_va_arg(ap, const char*)); break;
            case 'c': vga_putc((char)__builtin_va_arg(ap, int)); break;
            case 'd': put_dec(__builtin_va_arg(ap, u64)); break;
            case 'u': put_dec(__builtin_va_arg(ap, u64)); break;
            case 'x': put_hex(__builtin_va_arg(ap, u64), 1); break;
            case 'p': vga_puts("0x"); put_hex((u64)__builtin_va_arg(ap, void*), 16); break;
            case '%': vga_putc('%'); break;
            default: vga_putc('%'); vga_putc(*p);
        }
    }
    __builtin_va_end(ap);
}
