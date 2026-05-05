#pragma once

#include "types.h"

typedef void (*task_fn)(void);

typedef enum { TASK_READY = 0, TASK_DEAD = 1 } task_state_t;

typedef struct task {
    u64           rsp;
    u8           *stack_base;
    u32           id;
    task_state_t  state;
    const char   *name;
    struct task  *next;
} task_t;

void        sched_init(void);
task_t     *task_spawn(const char *name, task_fn entry);
void        sched_tick(void);
u32         sched_current_id(void);
const char *sched_current_name(void);
u64         sched_switches(void);

// called from sched_asm.s when a task entry returns
void        task_exit(void);
