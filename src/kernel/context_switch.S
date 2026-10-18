.intel_syntax noprefix

.extern task_entry_wrapper

.global switch_context
/*
 * void switch_context(uint64_t* prev_rsp, uint64_t next_rsp);
 * RDI: pointer to outgoing task's rsp storage (uint64_t*)
 * RSI: incoming task's new rsp (uint64_t)
 */
switch_context:
    /* 1. Save callee-saved registers of outgoing task */
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    pushfq

    /* 2. Save current RSP to *prev_rsp */
    mov [rdi], rsp

    /* 3. Load incoming task's RSP */
    mov rsp, rsi

    /* 4. Restore callee-saved registers of incoming task */
    popfq
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    /* 5. Return to incoming task's saved RIP */
    ret

.global task_trampoline
task_trampoline:
    /* Enable interrupts for the newly spawned task */
    sti

    /* Call the C task runner which invokes entry() and task_exit() */
    call task_entry_wrapper

    /* Fallback if task_entry_wrapper somehow returns */
hang_task:
    cli
    hlt
    jmp hang_task
