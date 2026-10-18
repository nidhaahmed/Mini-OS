# System Call Experiments

This directory contains experiments and architectural documentation regarding system call mechanisms.

## Relationship to Mini OS Kernel
In Mini OS, system calls are implemented using an x86-64 software interrupt trap gate (`int 0x80`):
- Installed in the IDT at vector 128 (`0x80`) with `IDT_ATTR_USER_GATE` (DPL 3).
- System call arguments are passed in registers (`RAX` = call number, `RDI` = arg1, `RSI` = arg2, `RDX` = arg3).
- Handled inside `src/kernel/syscall.c` via `syscall_dispatcher()`.
- Supported services include process yielding, PID retrieval, sleep timer, console print, memory allocation, and virtual filesystem operations.
