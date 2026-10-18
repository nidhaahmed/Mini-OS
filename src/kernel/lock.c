#include "kernel/lock.h"
#include "kernel/console.h"

void spinlock_init(spinlock_t* lock, const char* name) {
    if (!lock) return;
    lock->locked = 0;
    lock->holder_pid = 0;
    lock->name = name ? name : "unnamed_lock";
}

void spinlock_acquire(spinlock_t* lock) {
    if (!lock) return;

    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        // x86 pause instruction provides CPU hint to avoid memory bus pipeline stall
        __asm__ volatile ("pause");
    }
    lock->holder_pid = task_get_current() ? task_get_current()->pid : 0;
}

int spinlock_try_acquire(spinlock_t* lock) {
    if (!lock) return 0;

    if (!__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        lock->holder_pid = task_get_current() ? task_get_current()->pid : 0;
        return 1;
    }
    return 0;
}

int spinlock_acquire_timeout(spinlock_t* lock, uint64_t max_spins) {
    if (!lock) return 0;

    for (uint64_t i = 0; i < max_spins; i++) {
        if (!__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
            lock->holder_pid = task_get_current() ? task_get_current()->pid : 0;
            return 1;
        }
        __asm__ volatile ("pause");
    }
    return 0; // Timeout
}

void spinlock_release(spinlock_t* lock) {
    if (!lock) return;

    lock->holder_pid = 0;
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}

void mutex_init(mutex_t* m, const char* name) {
    if (!m) return;
    spinlock_init(&m->lock, "mutex_internal");
    m->locked = 0;
    m->owner_pid = 0;
    m->wait_head = NULL;
    m->wait_tail = NULL;
    m->name = name ? name : "unnamed_mutex";
}

void mutex_acquire(mutex_t* m) {
    if (!m) return;

    while (1) {
        spinlock_acquire(&m->lock);
        if (!m->locked) {
            m->locked = 1;
            m->owner_pid = task_get_current() ? task_get_current()->pid : 0;
            spinlock_release(&m->lock);
            return;
        }

        // Lock is held; block current task
        task_t* cur = task_get_current();
        cur->state = TASK_BLOCKED;
        cur->next = NULL;

        if (!m->wait_head) {
            m->wait_head = cur;
            m->wait_tail = cur;
        } else {
            m->wait_tail->next = cur;
            m->wait_tail = cur;
        }

        spinlock_release(&m->lock);
        schedule();
    }
}

void mutex_release(mutex_t* m) {
    if (!m) return;

    spinlock_acquire(&m->lock);
    m->locked = 0;
    m->owner_pid = 0;

    if (m->wait_head) {
        task_t* wake = m->wait_head;
        m->wait_head = m->wait_head->next;
        if (!m->wait_head) {
            m->wait_tail = NULL;
        }
        wake->state = TASK_READY;
        wake->next = NULL;
    }
    spinlock_release(&m->lock);
}
