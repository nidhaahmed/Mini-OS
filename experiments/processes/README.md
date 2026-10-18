# Process Experiments

This directory contains preliminary experiments with processes.

## Files
- `hello_process.c`: A standard Linux POSIX process experiment querying `getpid()` and `getppid()`.

## Relationship to Mini OS Kernel
In Mini OS, task management is implemented at the freestanding kernel level (`include/kernel/task.h` and `src/kernel/task.c`):
- Unlike POSIX processes that rely on the Linux kernel glibc wrapper, Mini OS tasks execute in 64-bit Long Mode with independent 16 KB stacks managed directly by the kernel's round-robin scheduler.
- Process IDs (PIDs) and parent relationships are tracked inside the kernel Task Control Block (`task_t`).
- The system call `sys_getpid()` (`int 0x80` vector 128) provides the kernel trap equivalent of POSIX `getpid()`.
