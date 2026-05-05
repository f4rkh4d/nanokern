#include "types.h"
#include "vga.h"
#include "pic.h"
#include "idt.h"
#include "pit.h"
#include "kbd.h"
#include "heap.h"
#include "regs.h"
#include "sched.h"
#include "syscall.h"

static void timer_isr(u64 vec, u64 err, regs_t *regs) {
    (void)vec; (void)err; (void)regs;
    pit_ticks++;
    // ack the master pic before we possibly switch away from this task. the dispatcher
    // will eoi again on its way out; the second ack on a non-pending irq0 is harmless.
    pic_eoi(0);
    sched_tick();
}

static void kbd_isr(u64 vec, u64 err, regs_t *regs) {
    (void)vec; (void)err; (void)regs;
    u8 sc = kbd_scancode();
    kbd_push(sc);
}

// minimal syscall wrappers used by demo tasks. compiles to a single int 0x80
// with the args already in the right registers.
static inline u64 syscall0(u64 num) {
    u64 ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num) : "memory", "rcx", "r11");
    return ret;
}
static inline u64 syscall1(u64 num, u64 a0) {
    u64 ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "D"(a0) : "memory", "rcx", "r11");
    return ret;
}
static inline u64 syscall2(u64 num, u64 a0, u64 a1) {
    u64 ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "D"(a0), "S"(a1) : "memory", "rcx", "r11");
    return ret;
}

// three demo tasks. each one writes a single character into a fixed column on the last
// vga row, with a per-task counter incrementing in the column to its right. the goal is
// visual proof of round-robin: the counters advance unevenly per quantum but all three
// keep moving, even though each task's body is `for (;;) { ...; hlt(); }` and only the
// timer IRQ + scheduler can move the cpu off it.
static volatile u32 a_count, b_count, c_count;

static void task_a(void) {
    for (;;) {
        a_count++;
        debug_putc('A');
        volatile u16 *vga = (volatile u16*)0xb8000;
        vga[24*80 +  2] = (0x0e << 8) | 'A';
        vga[24*80 +  4] = (0x0e << 8) | ('0' + (a_count / 10) % 10);
        vga[24*80 +  5] = (0x0e << 8) | ('0' + a_count % 10);
        __asm__ volatile("hlt");
    }
}

static void task_b(void) {
    for (;;) {
        b_count++;
        debug_putc('B');
        volatile u16 *vga = (volatile u16*)0xb8000;
        vga[24*80 + 10] = (0x0b << 8) | 'B';
        vga[24*80 + 12] = (0x0b << 8) | ('0' + (b_count / 10) % 10);
        vga[24*80 + 13] = (0x0b << 8) | ('0' + b_count % 10);
        __asm__ volatile("hlt");
    }
}

static void task_c(void) {
    for (;;) {
        c_count++;
        debug_putc('C');
        volatile u16 *vga = (volatile u16*)0xb8000;
        vga[24*80 + 18] = (0x0d << 8) | 'C';
        vga[24*80 + 20] = (0x0d << 8) | ('0' + (c_count / 10) % 10);
        vga[24*80 + 21] = (0x0d << 8) | ('0' + c_count % 10);
        __asm__ volatile("hlt");
    }
}

// task d: uses real syscalls (int 0x80) for everything. each loop iteration
// emits a 'd' to debugcon BEFORE the syscall (proves the task got scheduled),
// then SYS_WRITE'es a 'D' to VGA via the syscall path (proves syscall returns
// properly and the kernel-side handler reached vga), then SYS_SLEEPs for 50 ms.
static volatile u32 d_count;
static void task_d(void) {
    u64 my_pid = syscall0(SYS_GETPID);
    for (;;) {
        d_count++;
        debug_putc('d');                 // pre-syscall sentinel: task is alive
        syscall1(SYS_WRITE, (u64)'D');   // post-syscall: SYS_WRITE landed in vga_putc
        debug_putc('D');                 // syscall returned and we ran past it
        volatile u16 *vga = (volatile u16*)0xb8000;
        vga[24*80 + 26] = (0x09 << 8) | 'D';
        vga[24*80 + 28] = (0x09 << 8) | ('0' + (d_count / 10) % 10);
        vga[24*80 + 29] = (0x09 << 8) | ('0' + d_count % 10);
        if (d_count == 1) {
            vga[24*80 + 31] = (0x09 << 8) | ('0' + ((u32)my_pid % 10));
        }
        syscall1(SYS_SLEEP, 5);    // 5 ticks = 50 ms
    }
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

    idt_register(32, timer_isr);
    idt_register(33, kbd_isr);
    idt_register(0x80, syscall_isr);
    vga_puts("[boot] syscall vector 0x80 wired (write, puts, getpid, sleep, exit)\n");

    pit_init(100);
    vga_puts("[boot] pit armed at 100 hz (10 ms)\n");

    pic_unmask(0);
    pic_unmask(1);
    vga_puts("[boot] irq0 (timer) + irq1 (keyboard) unmasked\n");

    sched_init();
    task_spawn("task-a", task_a);
    task_spawn("task-b", task_b);
    task_spawn("task-c", task_c);
    task_spawn("task-d", task_d);   // syscall-using task
    vga_puts("[boot] scheduler online: 4 tasks ready (3 timer-driven, 1 syscall-driven)\n");

    vga_printf("[boot] heap: %d / %d bytes used\n",
               (u64)kheap_used(), (u64)kheap_size());
    vga_puts("[boot] enabling interrupts...\n\n");

    sti();

    u64 last = 0;
    vga_puts("init > ");
    for (;;) {
        char c;
        while (kbd_read(&c)) {
            if (c == '\n') vga_puts("\ninit > ");
            else vga_putc(c);
        }
        if (pit_ticks / 100 != last) {
            last = pit_ticks / 100;
            volatile u16 *vga = (volatile u16*)0xb8000;
            vga[70] = (0x0a << 8) | '.';
            vga[71] = (0x0a << 8) | ' ';
            vga[72] = (0x0a << 8) | ('0' + (last % 10));
            vga[73] = (0x0a << 8) | 's';
        }
        __asm__ volatile("hlt");
    }
}
