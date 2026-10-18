#include "kernel/task.h"
#include "kernel/memory.h"
#include "kernel/timer.h"
#include "kernel/console.h"
#include "lib/string.h"

#define STACK_SIZE 16384
#define QUANTUM_TICKS 5

extern void switch_context(uint64_t* prev_rsp, uint64_t next_rsp);
extern void task_trampoline(void);

static task_t main_task;
static task_t* current_task = NULL;
static task_t* all_tasks = NULL;

static task_t* ready_head = NULL;
static task_t* ready_tail = NULL;

static uint32_t next_pid = 0;
static int scheduler_running = 0;

static const char* const state_names[] = {
    "EMBRYO",
    "READY",
    "RUNNING",
    "BLOCKED",
    "SLEEPING",
    "TERMINATED"
};

static void enqueue_ready(task_t* task) {
    if (!task) return;
    task->state = TASK_READY;
    task->next = NULL;

    if (!ready_head) {
        ready_head = task;
        ready_tail = task;
    } else {
        ready_tail->next = task;
        ready_tail = task;
    }
}

static task_t* dequeue_ready(void) {
    if (!ready_head) return NULL;

    task_t* task = ready_head;
    ready_head = ready_head->next;
    if (!ready_head) {
        ready_tail = NULL;
    }
    task->next = NULL;
    return task;
}

void task_entry_wrapper(void) {
    if (current_task && current_task->entry) {
        current_task->entry();
    }
    task_exit();
}

void scheduler_init(void) {
    memset(&main_task, 0, sizeof(task_t));
    main_task.pid = next_pid++;
    strcpy(main_task.name, "kmain_thread");
    main_task.state = TASK_RUNNING;
    main_task.priority = 1;
    main_task.next = NULL;
    main_task.all_next = NULL;

    current_task = &main_task;
    all_tasks = &main_task;
    scheduler_running = 1;

    kprintf("[+] Scheduler    : Initialized Round-Robin Multitasking (Main Task PID: %u)\n", main_task.pid);
}

task_t* task_create(const char* name, task_entry_t entry, uint32_t priority) {
    task_t* task = (task_t*)kmalloc(sizeof(task_t));
    if (!task) return NULL;
    memset(task, 0, sizeof(task_t));

    task->pid = next_pid++;
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->entry = entry;
    task->priority = priority;
    task->stack_size = STACK_SIZE;
    task->stack_base = (uint64_t)kmalloc(STACK_SIZE);
    if (!task->stack_base) {
        kfree(task);
        return NULL;
    }

    // Format initial stack for context switch
    uint64_t stack_top = (task->stack_base + STACK_SIZE) & ~0xFULL;
    uint64_t* sp = (uint64_t*)stack_top;

    *(--sp) = (uint64_t)task_trampoline; // Return address (RIP)
    *(--sp) = 0;                         // RBX
    *(--sp) = 0;                         // RBP
    *(--sp) = 0;                         // R12
    *(--sp) = 0;                         // R13
    *(--sp) = 0;                         // R14
    *(--sp) = 0;                         // R15
    *(--sp) = 0x202;                     // RFLAGS (IF=1)

    task->rsp = (uint64_t)sp;

    // Insert into all_tasks list using all_next
    task->all_next = all_tasks;
    all_tasks = task;

    // Add to ready queue
    enqueue_ready(task);

    kprintf("[+] Task Created : PID %u (\"%s\") | Stack: %p..%p | Init RSP: %p\n",
            task->pid, task->name, (void*)task->stack_base, (void*)stack_top, (void*)task->rsp);

    return task;
}

void schedule(void) {
    if (!scheduler_running || !current_task) return;

    task_t* next = dequeue_ready();
    if (!next) {
        // No other task ready; if current task is still runnable, continue it
        if (current_task->state == TASK_RUNNING) {
            return;
        }
        // Otherwise wait for an interrupt to wake a sleeping task
        while (!ready_head) {
            __asm__ volatile ("sti; hlt");
        }
        next = dequeue_ready();
    }

    if (next == current_task) {
        next->state = TASK_RUNNING;
        return;
    }

    task_t* prev = current_task;
    current_task = next;
    current_task->state = TASK_RUNNING;
    current_task->switch_count++;

    switch_context(&prev->rsp, next->rsp);
}

void task_yield(void) {
    if (!scheduler_running) return;

    if (current_task->state == TASK_RUNNING) {
        enqueue_ready(current_task);
    }
    schedule();
}

void task_sleep(uint64_t ticks) {
    if (!scheduler_running) return;

    current_task->state = TASK_SLEEPING;
    current_task->sleep_until = timer_get_ticks() + ticks;
    schedule();
}

void task_exit(void) {
    if (!scheduler_running) return;

    current_task->state = TASK_TERMINATED;
    kprintf("[*] Task Exit    : PID %u (\"%s\") terminated cleanly (ran %u ticks, %u switches)\n",
            current_task->pid, current_task->name,
            (uint32_t)current_task->runtime_ticks, (uint32_t)current_task->switch_count);

    schedule();

    // Should never reach here
    while (1) { __asm__ volatile ("hlt"); }
}

task_t* task_get_current(void) {
    return current_task;
}

void scheduler_timer_tick(interrupt_frame_t* frame) {
    (void)frame;
    if (!scheduler_running) return;

    uint64_t now = timer_get_ticks();

    // 1. Wake up sleeping tasks
    task_t* cur = all_tasks;
    while (cur) {
        if (cur->state == TASK_SLEEPING && now >= cur->sleep_until) {
            enqueue_ready(cur);
        }
        cur = cur->all_next;
    }

    // 2. Track current task runtime and check preemptive quantum
    if (current_task) {
        current_task->runtime_ticks++;

        // Preempt if quantum expired and other tasks are waiting in ready queue
        if (ready_head != NULL && (current_task->runtime_ticks % QUANTUM_TICKS) == 0) {
            task_yield();
        }
    }
}

void task_dump_all(void) {
    kprintf("\n============================ PROCESS / TASK TABLE ============================\n");
    kprintf("PID  | NAME                | STATE       | PRIO | RUNTIME  | SWITCHES | RSP\n");
    kprintf("-----+---------------------+-------------+------+----------+----------+------------------\n");

    task_t* cur = all_tasks;
    while (cur) {
        kprintf(" %u   | %s", cur->pid, cur->name);
        // Pad name to 19 chars
        size_t nlen = strlen(cur->name);
        for (size_t p = nlen; p < 19; p++) console_putchar(' ');

        kprintf(" | %s", state_names[cur->state]);
        size_t slen = strlen(state_names[cur->state]);
        for (size_t p = slen; p < 11; p++) console_putchar(' ');

        kprintf(" |  %u   | %u ticks", cur->priority, (uint32_t)cur->runtime_ticks);
        if (cur->runtime_ticks < 10) console_putchar(' ');

        kprintf(" |    %u     | %p\n", (uint32_t)cur->switch_count, (void*)cur->rsp);
        cur = cur->all_next;
    }
    kprintf("==============================================================================\n");
}
