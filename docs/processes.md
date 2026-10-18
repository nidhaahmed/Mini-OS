# Mini OS: Processes, Multitasking, and Scheduling

## 1. Process Concept in Mini OS

Mini OS implements kernel-level tasks (threads of execution) running with independent execution contexts and dedicated 16 KB stacks in x86-64 Long Mode.

---

## 2. Task Control Block (TCB / PCB)

Each task is represented by a `task_t` structure (`include/kernel/task.h`):

```c
typedef struct task {
    uint32_t     pid;           // Unique process identifier
    char         name[32];      // Human-readable process name
    task_state_t state;         // Current lifecycle state
    uint64_t     rsp;           // Saved stack pointer (RSP)
    uint64_t     stack_base;    // Base address of allocated stack
    uint64_t     stack_size;    // Stack size (16,384 bytes)
    uint64_t     cr3;           // Page directory address space
    uint32_t     priority;      // Scheduling priority
    uint64_t     sleep_until;   // Timer tick target for sleeping tasks
    uint64_t     runtime_ticks; // CPU time accounting
    uint64_t     switch_count;  // Total number of context switches
    task_entry_t entry;         // Function entry point
    struct task* next;          // Pointer for scheduler ready/wait queue
    struct task* all_next;      // Pointer for global process table
} task_t;
```

---

## 3. Task Lifecycle States

A task progresses through standard operating-system states:

```
    [task_create]
          |
          v
    +-----------+      schedule()      +-----------+
    |   READY   | -------------------> |  RUNNING  |
    +-----------+                      +-----------+
          ^                                |     |
          |       task_yield() / quantum   |     | task_exit()
          +--------------------------------+     v
          |                                +------------+
          |  wakeup tick reached           | TERMINATED |
          |                                +------------+
    +-----------+                                |
    |  SLEEPING | <------------------------------+
    +-----------+       task_sleep(ticks)
```

1. **`TASK_READY`**: Enqueued in the round-robin ready queue, eligible for CPU time.
2. **`TASK_RUNNING`**: Actively executing instructions on the CPU.
3. **`TASK_BLOCKED`**: Suspended waiting on a mutex or resource.
4. **`TASK_SLEEPING`**: Suspended until a future PIT timer tick (`timer_get_ticks() >= sleep_until`).
5. **`TASK_TERMINATED`**: Execution has finished, stack and structures preserved for inspection.

---

## 4. Hardware Context Switching

Mini OS implements **actual hardware context switching** in assembly (`src/kernel/context_switch.S`):

```assembly
switch_context:
    /* 1. Save callee-saved registers of outgoing task */
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    pushfq

    /* 2. Store current stack pointer in old task's TCB */
    mov [rdi], rsp

    /* 3. Switch CPU stack pointer to new task's RSP */
    mov rsp, rsi

    /* 4. Restore callee-saved registers from new task's stack */
    popfq
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    /* 5. Jump into new task */
    ret
```

### Initial Stack Formatting
When a new task is created with `task_create()`, its 16 KB stack is formatted to simulate having been suspended by `switch_context`:
- Bottom of frame contains `task_trampoline` as the return RIP.
- Pushed register slots (`RFLAGS = 0x202` with interrupts enabled, `RBX`, `RBP`, `R12-R15` initialized to 0).
- When `switch_context` performs `ret`, execution jumps smoothly into `task_trampoline`, calls the task's C entry function with interrupts enabled, and automatically calls `task_exit()` upon completion.

---

## 5. Round-Robin Scheduling Algorithm

- **Cooperative Multitasking**: Tasks can invoke `task_yield()` or `sys_yield()` at any time to surrender the CPU.
- **Preemptive Time-Slicing**:
  - The 8254 PIT fires periodic interrupts every 10ms (100 Hz).
  - The timer handler calls `scheduler_timer_tick()`.
  - Every 5 ticks (50ms quantum), if other tasks are present in the ready queue, the scheduler preempts the currently running task and invokes `task_yield()`.
- **Sleeping Tasks**: The scheduler inspects sleeping tasks on every timer tick and transitions tasks whose target tick has arrived back to `TASK_READY`.
