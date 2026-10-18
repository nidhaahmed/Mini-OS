#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/isr.h"

typedef enum {
    TASK_EMBRYO,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED
} task_state_t;

typedef void (*task_entry_t)(void);

typedef struct task {
    uint32_t     pid;
    char         name[32];
    task_state_t state;
    uint64_t     rsp;
    uint64_t     stack_base;
    uint64_t     stack_size;
    uint64_t     cr3;
    uint32_t     priority;
    uint64_t     sleep_until;
    uint64_t     runtime_ticks;
    uint64_t     switch_count;
    task_entry_t entry;
    struct task* next;
    struct task* all_next;
} task_t;

void scheduler_init(void);
task_t* task_create(const char* name, task_entry_t entry, uint32_t priority);
void task_yield(void);
void task_sleep(uint64_t ticks);
void task_exit(void);
task_t* task_get_current(void);
void task_dump_all(void);
void schedule(void);

// Hook called from timer interrupt (preemption & sleep wakeup)
void scheduler_timer_tick(interrupt_frame_t* frame);

#endif /* TASK_H */
