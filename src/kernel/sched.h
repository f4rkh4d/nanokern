#pragma once

#include "types.h"

typedef void (*task_fn)(void);

typedef enum {
    TASK_READY    = 0,
    TASK_DEAD     = 1,
    TASK_SLEEPING = 2,   // wake_at holds the pit_ticks value at which to revive
} task_state_t;

typedef struct task {
    u64           rsp;
    u8           *stack_base;
    u32           id;
    task_state_t  state;
    u64           wake_at;     // valid only while state == TASK_SLEEPING
    const char   *name;
    struct task  *next;
} task_t;

void        sched_init(void);
task_t     *task_spawn(const char *name, task_fn entry);
void        sched_tick(void);
u32         sched_current_id(void);
const char *sched_current_name(void);
u64         sched_switches(void);

// suspend the current task for `ticks` pit ticks (one tick = 10 ms).
// safe to call from a syscall handler running with interrupts disabled;
// it sets state=TASK_SLEEPING with a deadline and yields.
void        sched_sleep(u64 ticks);

// called from sched_asm.s when a task entry returns
void        task_exit(void);
