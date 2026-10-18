#ifndef LOCK_H
#define LOCK_H

#include <stdint.h>
#include "kernel/task.h"

typedef struct spinlock {
    volatile uint32_t locked;
    volatile uint32_t holder_pid;
    const char* name;
} spinlock_t;

typedef struct mutex {
    spinlock_t lock;
    volatile int locked;
    volatile uint32_t owner_pid;
    task_t* wait_head;
    task_t* wait_tail;
    const char* name;
} mutex_t;

void spinlock_init(spinlock_t* lock, const char* name);
void spinlock_acquire(spinlock_t* lock);
int  spinlock_try_acquire(spinlock_t* lock);
int  spinlock_acquire_timeout(spinlock_t* lock, uint64_t max_spins);
void spinlock_release(spinlock_t* lock);

void mutex_init(mutex_t* m, const char* name);
void mutex_acquire(mutex_t* m);
void mutex_release(mutex_t* m);

#endif /* LOCK_H */
