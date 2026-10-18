# Mini OS: Verification and Testing Guide

This document provides instructions for reproducing all test results and verifying the Mini OS kernel milestones.

---

## 1. Prerequisites

The primary build environment is **WSL2 Ubuntu on x86-64** (or native Linux x86-64).

Required packages:
- `gcc` (version 11+ supporting freestanding x86-64)
- `binutils` (`ld`, `as`, `objcopy`, `nm`)
- `make` (GNU Make 4+)
- `qemu-system-x86_64` (version 7+)
- `gdb` (for kernel debugging)

To install all prerequisites on Ubuntu/Debian:
```bash
sudo apt update && sudo apt install -y build-essential gcc binutils make qemu-system-x86 gdb
```

---

## 2. Quick Reproduction Commands

From the repository root (`F:\Dev\Mini OS\Mini-OS` or `/mnt/f/Dev/Mini OS/Mini-OS`):

```bash
# Clean previous build artifacts
make clean

# Compile the 64-bit kernel and generate 32-bit Multiboot ELF
make build

# Launch Mini OS in QEMU with serial console
make run

# Run automated smoke test suite (terminates after 8 seconds)
make test

# Launch in QEMU waiting for GDB on localhost:1234
make debug

# In another terminal, attach GDB with symbols
make gdb
```

---

## 3. Milestone-by-Milestone Verification

### Milestone 1: Bootable Kernel Foundation
- **Test**: `make run`
- **Expected Result**: Multiboot header detected, 32-bit to 64-bit Long Mode transition succeeds, kernel startup banner displays Mini OS ASCII logo, Authentic CPUID vendor, RAM detection, and COM1 serial initialization.

### Milestone 2: CPU, Interrupts, and Memory
- **Test**: Automated during boot in `demonstrate_milestone2()`
- **Verified Items**:
  - GDT and 64-bit TSS loaded with segment registers reloaded.
  - IDT initialized with 256 gates.
  - Safe exception recovery: `#BP` (Breakpoint, `int3`) triggers registered handler and resumes normal execution.
  - 8259 PIC remapped; PIT timer fires periodic ticks (verified 5 ticks / 50ms progression).
  - PMM Bitmap manages 127 MB physical RAM frames.
  - 4-Level Paging: Page table walk translates virtual address `0xCAFE0000` to physical frame, writes test pattern, verifies data, and unmaps cleanly.
  - Kernel Heap: `kmalloc()` allocates memory, verifies string payload, and `kfree()` coalesces free blocks into a single continuous block.

### Milestone 3: Processes, Context Switching, & Round-Robin Scheduling
- **Test**: Automated in `demonstrate_milestone3()`
- **Verified Items**:
  - Task Control Block initialization with independent 16 KB stacks.
  - 3 child tasks created: `Worker-Alpha` (computes Fibonacci iterations), `Worker-Beta` (periodic heartbeat), `Worker-Gamma` (lifecycle testing).
  - Main thread yields; tasks cooperatively and preemptively switch contexts via `switch_context.S`.
  - Final process table prints PID, name, state (`TERMINATED`), runtime ticks, context switch count, and stack pointer.

### Milestone 4: Synchronization, Deadlocks, Syscalls, & VFS
- **Test**: Automated in `demonstrate_milestone4()`
- **Verified Items**:
  - `spinlock_t` mutual exclusion protects shared counter against race conditions.
  - Safe Deadlock scenario: Circular wait between Lock A and Lock B is detected by timeout watchdog; deadlock is resolved safely by releasing resource and recovering.
  - System call software trap (`int 0x80`) tested for `SYS_GETPID`, `SYS_PRINT`, `SYS_MALLOC`, `SYS_FREE`.
  - In-memory Virtual File System (RAMFS): creates `/test.txt`, writes 57 bytes, reads back exact bytes, displays VFS directory table, and deletes cleanly.

### Milestone 5: Integrated Automated Smoke Test Suite
- **Test**: `make test` runs `run_kernel_smoke_tests()`
- **Result**: `12/12 TESTS PASSED (100% SUCCESS)`.
