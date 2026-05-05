// preemptive round-robin scheduler. timer irq calls sched_tick(); sched_tick saves the
// current task's callee-saved registers via context_switch and rotates rsp to the next
// ready task. all tasks share ring 0 and the kernel address space; user mode is roadmap.

#include "sched.h"
#include "heap.h"
#include "types.h"

#define TASK_STACK_BYTES 8192

extern void context_switch(u64 *old_rsp_save, u64 new_rsp);
extern void task_entry_trampoline(void);

static task_t *current  = 0;
static task_t *ring     = 0;   // any node in the circular ready ring
static u64     switches = 0;
static u32     next_id  = 1;

static void ring_insert(task_t *t) {
    if (!ring) {
        ring = t;
        t->next = t;
    } else {
        t->next = ring->next;
        ring->next = t;
    }
}

void sched_init(void) {
    // model the currently-executing kernel context as task 0 ("init"). its rsp will be
    // captured the first time sched_tick() preempts it; we just need a struct to point to.
    task_t *t = (task_t*)kmalloc(sizeof(task_t));
    t->rsp        = 0;
    t->stack_base = 0;
    t->id         = 0;
    t->state      = TASK_READY;
    t->name       = "init";
    t->next       = t;
    ring    = t;
    current = t;
}

task_t *task_spawn(const char *name, task_fn entry) {
    task_t *t = (task_t*)kmalloc(sizeof(task_t));
    if (!t) return 0;
    u8 *stack = (u8*)kmalloc(TASK_STACK_BYTES);
    if (!stack) return 0;

    t->stack_base = stack;
    t->id         = next_id++;
    t->state      = TASK_READY;
    t->name       = name;

    // Build the initial stack so that context_switch(...) followed by ret lands inside
    // task_entry_trampoline with r12 holding the entry function. rsp must be 8 mod 16
    // at trampoline entry per the SysV x86-64 ABI; we add a single padding qword to get
    // the alignment right after the 6 callee-saved pops + 1 ret.
    u64 *sp = (u64*)(stack + TASK_STACK_BYTES);
    *--sp = 0;                              // alignment pad
    *--sp = (u64)task_entry_trampoline;     // ret target
    *--sp = 0;                              // rbp
    *--sp = 0;                              // rbx
    *--sp = (u64)entry;                     // r12 (carried through to trampoline)
    *--sp = 0;                              // r13
    *--sp = 0;                              // r14
    *--sp = 0;                              // r15
    t->rsp = (u64)sp;

    ring_insert(t);
    return t;
}

void sched_tick(void) {
    if (!current || !ring) return;

    // pick the next ready task by walking forward in the ring. give up after one full
    // lap; if nothing else is ready, stay on current.
    task_t *next = current->next;
    int hops = 0;
    while (next->state != TASK_READY && next != current && hops < 1024) {
        next = next->next;
        hops++;
    }
    if (next == current || next->state != TASK_READY) return;

    task_t *prev = current;
    current = next;
    switches++;
    context_switch(&prev->rsp, next->rsp);
}

u32         sched_current_id(void)   { return current ? current->id : 0; }
const char *sched_current_name(void) { return current ? current->name : "?"; }
u64         sched_switches(void)     { return switches; }

void task_exit(void) {
    if (current) current->state = TASK_DEAD;
    for (;;) {
        sched_tick();
        __asm__ volatile("hlt");
    }
}
