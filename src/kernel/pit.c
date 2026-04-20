#include "types.h"
#include "pit.h"

#define PIT_CHAN0 0x40
#define PIT_CMD   0x43
#define PIT_FREQ  1193182u

volatile u64 pit_ticks = 0;

void pit_init(u32 hz) {
    u32 div = PIT_FREQ / hz;
    outb(PIT_CMD, 0x36);               // channel 0, lobyte/hibyte, rate generator
    outb(PIT_CHAN0, (u8)(div & 0xff));
    outb(PIT_CHAN0, (u8)((div >> 8) & 0xff));
}
