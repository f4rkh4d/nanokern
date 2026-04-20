#include "types.h"
#include "kbd.h"

#define KBD_DATA 0x60
#define RING_SZ  128

static volatile u8 ring[RING_SZ];
static volatile u16 head, tail;

// ps/2 scancode set 1, us qwerty, no shift/ctrl layering
static const char map[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',
    0, '*', 0, ' ',
};

void kbd_push(u8 sc) {
    if (sc & 0x80) return;          // key release
    char c = map[sc & 0x7f];
    if (!c) return;
    u16 nh = (head + 1) % RING_SZ;
    if (nh == tail) return;         // full, drop
    ring[head] = (u8)c;
    head = nh;
}

int kbd_read(char *out) {
    if (head == tail) return 0;
    *out = ring[tail];
    tail = (tail + 1) % RING_SZ;
    return 1;
}

u8 kbd_scancode(void) { return inb(KBD_DATA); }
