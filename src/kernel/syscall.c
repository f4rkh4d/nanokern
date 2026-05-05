// syscall dispatcher.
//
// caller convention: rax = syscall number, rdi/rsi/rdx = args 0..2,
// return value in rax. on unknown syscall, returns (u64)-1.
//
// the body runs in ring 0 (we have no ring 3 yet), so syscall args are
// just pointers we trust. when user mode lands, this file gains explicit
// copy_from_user / copy_to_user wrappers that vet the address range.

#include "syscall.h"
#include "vga.h"
#include "sched.h"
#include "types.h"

void syscall_isr(u64 vec, u64 err, regs_t *regs) {
    (void)vec;
    (void)err;

    u64 num = regs->rax;
    u64 a0  = regs->rdi;
    u64 a1  = regs->rsi;
    u64 ret = (u64)-1;

    switch (num) {
        case SYS_WRITE:
            vga_putc((char)a0);
            ret = 0;
            break;

        case SYS_PUTS: {
            const char *s = (const char *)a0;
            for (u64 i = 0; i < a1; i++) vga_putc(s[i]);
            ret = a1;
            break;
        }

        case SYS_GETPID:
            ret = (u64)sched_current_id();
            break;

        case SYS_SLEEP:
            sched_sleep(a0);
            ret = 0;
            break;

        case SYS_EXIT:
            task_exit();        // does not return; sched picks the next task
            // safety net if task_exit ever does return
            for (;;) __asm__ volatile("hlt");

        default:
            ret = (u64)-1;
    }

    regs->rax = ret;
}
