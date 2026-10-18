# Mini OS: Modular x86-64 Operating System Kernel

[![Build & Smoke Test](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Target Architecture](https://img.shields.io/badge/arch-x86--64%20(Long%20Mode)-blue.svg)]()
[![Boot Protocol](https://img.shields.io/badge/boot-Multiboot%201-orange.svg)]()
[![License](https://img.shields.io/badge/license-MIT-purple.svg)]()

**Mini OS** is a small, freestanding, educational operating system kernel written from scratch in C and x86-64 Assembly. It demonstrates fundamental operating system engineering concepts: 32-to-64-bit bootloader transitions, hardware descriptor tables (GDT/IDT), interrupt service routines, physical and virtual memory paging, boundary-tag heap allocation, real task context switching, preemptive round-robin scheduling, spinlock synchronization with safe deadlock detection, software trap system calls (`int 0x80`), and an in-memory virtual filesystem (RAMFS).

---

## 1. Feature Matrix

Each required operating system concept is categorized according to its implementation status:

| Concept / Mechanism | Status | Implementation Details |
|---------------------|:------:|------------------------|
| **Multiboot Bootloader Interface** | **IMPLEMENTED** | Multiboot 1 compliant (`0x1BADB002`), boots directly via QEMU `-kernel` or GRUB |
| **32-to-64-bit Long Mode Transition** | **IMPLEMENTED** | Identity-maps first 1GB with 2MB huge pages, sets PAE, enables LME MSR, jumps to 64-bit CS |
| **Global Descriptor Table (GDT) & TSS** | **IMPLEMENTED** | Flat 64-bit descriptors (Kernel/User Code/Data) + 64-bit TSS descriptor with `rsp0` |
| **Interrupt Descriptor Table (IDT)** | **IMPLEMENTED** | 256 gates configured for CPU exceptions (0–31), IRQs (32–47), and Syscalls (128) |
| **CPU Exception Handling** | **IMPLEMENTED** | Full register dumps, CR2 page fault diagnosis, and safe recovery demonstrated on `#BP` |
| **8259 PIC & 8254 PIT Timer** | **IMPLEMENTED** | Remapped PIC (IRQs 32..47); 100 Hz PIT timer (10ms tick interval) driving preemption |
| **Physical Memory Allocator (PMM)** | **IMPLEMENTED** | Bitmap frame allocator managing 4KB physical page frames parsed from Multiboot memory map |
| **4-Level Virtual Memory Paging** | **IMPLEMENTED** | Full PML4 $\to$ PDPT $\to$ PD $\to$ PT page tables; dynamic mapping, translation walk, and unmapping |
| **Kernel Heap Allocator** | **IMPLEMENTED** | 16-byte aligned boundary-tag allocator (`kmalloc`, `kfree`, `kcalloc`, `krealloc`) with coalescing |
| **Task Model & Lifecycle States** | **IMPLEMENTED** | Task Control Block (`task_t`) with states (`READY`, `RUNNING`, `BLOCKED`, `SLEEPING`, `TERMINATED`) |
| **Hardware Context Switching** | **IMPLEMENTED** | Real x86-64 assembly context switch (`switch_context.S`) with dedicated 16 KB stacks per task |
| **Round-Robin Task Scheduler** | **IMPLEMENTED** | Cooperative yielding (`task_yield`) and preemptive time-slicing (quantum = 5 ticks = 50ms) |
| **Spinlock Mutual Exclusion** | **IMPLEMENTED** | Atomic test-and-set spinlocks with x86 `pause` instruction protecting critical sections |
| **Deadlock Detection & Recovery** | **IMPLEMENTED** | Watchdog timeout detects circular wait (ABBA deadlock) and safely preempts lock to recover |
| **System Call Interface** | **IMPLEMENTED** | Software trap gate (`int 0x80`) dispatching 11 system calls via System V AMD64 register ABI |
| **In-Memory Virtual Filesystem (RAMFS)** | **IMPLEMENTED** | Inode-based filesystem supporting create, write, read, directory list, and delete operations |
| **Automated Smoke Test Suite** | **IMPLEMENTED** | 12/12 automated verification tests executed during boot |

---

## 2. Kernel Architecture

```
+-----------------------------------------------------------------------------------+
|                                MINI OS KERNEL                                     |
+-----------------------------------------------------------------------------------+
| User / Task Space : Tasks (Alpha, Beta, Gamma) | System Call Gate (int 0x80)      |
+-----------------------------------------------------------------------------------+
| Services          : In-Memory VFS (RAMFS)     | Process Scheduler (Round-Robin)   |
+-----------------------------------------------------------------------------------+
| Synchronization   : Atomic Spinlocks (pause)  | Deadlock Watchdog Detector        |
+-----------------------------------------------------------------------------------+
| Memory Management : 4-Level Paging (CR3/PML4) | Boundary-Tag Heap (kmalloc/kfree) |
|                   : PMM Frame Allocator (4KB Bitmap from Multiboot RAM Map)       |
+-----------------------------------------------------------------------------------+
| CPU & Hardware    : GDT & 64-bit TSS          | IDT (256 Gates & Exception Dumps) |
|                   : 8259 PIC Remap            | 8254 PIT Timer (100 Hz Ticks)     |
|                   : Dual Console: VGA Text Buffer (0xB8000) + 16550A UART (COM1)  |
+-----------------------------------------------------------------------------------+
| Boot Protocol     : Multiboot 1 -> 32-bit Protected Mode -> 64-bit Long Mode      |
+-----------------------------------------------------------------------------------+
```

---

## 3. Project Structure

```
Mini-OS/
├── Makefile                      # Build, run, debug, clean, and test targets
├── linker.ld                     # Freestanding kernel linker script (loads at 1MB)
├── README.md                     # Main documentation & feature matrix
├── .gitignore                    # Prevents build artifacts and binaries from being committed
├── docs/
│   ├── architecture.md           # Deep dive into kernel subsystems & initialization
│   ├── processes.md              # Process model, context switching, and scheduler
│   ├── syscalls.md               # System call ABI, dispatch table, and trap mechanics
│   └── verification.md           # Step-by-step reproduction and testing instructions
├── experiments/
│   ├── processes/
│   │   ├── hello_process.c       # Educational process comparison experiment
│   │   └── README.md
│   └── syscalls/
│       └── README.md             # Educational syscall mechanism notes
├── include/
│   ├── boot/
│   │   └── multiboot.h           # Multiboot 1 specification structures
│   ├── kernel/
│   │   ├── console.h             # VGA text buffer driver & kprintf
│   │   ├── gdt.h                 # Global Descriptor Table and 64-bit TSS
│   │   ├── idt.h                 # Interrupt Descriptor Table descriptors
│   │   ├── io.h                  # Port I/O inline assembly (inb, outb, inw, outw)
│   │   ├── isr.h                 # Interrupt frame & handler registrations
│   │   ├── lock.h                # Spinlocks & blocking mutexes
│   │   ├── memory.h              # Physical memory allocator & kernel heap
│   │   ├── paging.h              # 4-level x86-64 virtual memory paging
│   │   ├── pic.h                 # 8259 Programmable Interrupt Controller
│   │   ├── serial.h              # 16550A UART serial driver (COM1)
│   │   ├── syscall.h             # System call numbers & trap wrappers
│   │   ├── task.h                # Task control block & round-robin scheduler
│   │   ├── timer.h               # 8254 Programmable Interval Timer (PIT)
│   │   └── vfs.h                 # In-memory virtual filesystem (RAMFS)
│   ├── lib/
│   │   └── string.h              # Freestanding string & memory functions
│   └── tests/
│       └── smoke_test.h          # Automated kernel smoke test suite header
└── src/
    ├── boot/
    │   ├── boot.S                # Multiboot header, 32-to-64-bit transition assembly
    │   └── isr_stub.S            # Assembly interrupt stubs (exceptions, IRQs, trap 0x80)
    ├── kernel/
    │   ├── console.c             # VGA 80x25 text console with dual serial logging
    │   ├── context_switch.S      # Hardware register saving & stack switching assembly
    │   ├── gdt.c                 # GDT & TSS segment initialization
    │   ├── idt.c                 # IDT gate loading
    │   ├── isr.c                 # ISR dispatcher & rich exception panic dumps
    │   ├── kmain.c               # Kernel main entry & milestone demonstration coordinator
    │   ├── lock.c                # Atomic spinlock & deadlock detection implementation
    │   ├── memory.c              # PMM bitmap allocator & coalescing heap allocator
    │   ├── paging.c              # 4-level page table walking and dynamic mapping
    │   ├── pic.c                 # PIC cascade remapping & EOI management
    │   ├── serial.c              # 16550A UART 115200 baud serial console driver
    │   ├── syscall.c             # int 0x80 dispatcher & userland wrappers
    │   ├── task.c                # Round-robin scheduler & task lifecycle management
    │   ├── timer.c               # PIT frequency programming & tick timekeeping
    │   └── vfs.c                 # In-memory RAMFS implementation
    ├── lib/
    │   └── string.c              # Freestanding memory and string library
    └── tests/
        └── smoke_test.c          # 12-test automated verification suite
```

---

## 4. Prerequisites & Environment

The recommended primary environment is **WSL2 Ubuntu on x86-64** or native Linux.

Install the required toolchain:
```bash
sudo apt update && sudo apt install -y build-essential gcc binutils make qemu-system-x86 gdb
```

Toolchain verification:
- `gcc --version` (tested on GCC 11 through 15)
- `ld --version` (GNU Binutils)
- `qemu-system-x86_64 --version` (tested on QEMU 7+)

---

## 5. Build, Run, and Debug Commands

All commands are executed via `make` from the repository root:

```bash
# 1. Clean previous builds
make clean

# 2. Build kernel ELF64 and 32-bit Multiboot ELF
make build

# 3. Boot Mini OS in QEMU with serial console
make run

# 4. Boot Mini OS in QEMU with both VGA window and serial output
make run-display

# 5. Run automated smoke tests (terminates after 8 seconds)
make test

# 6. Start QEMU in debug mode waiting for GDB on localhost:1234
make debug

# 7. In a separate terminal, attach GDB with kernel symbols
make gdb
```

---

## 6. Technical Subsystem Deep Dive

### Boot Flow & 32-to-64 Long Mode Transition
1. QEMU loads `build/kernel32.elf` at physical `0x100000` (1MB).
2. CPU starts in 32-bit protected mode at `_start` (`src/boot/boot.S`).
3. Multiboot magic (`EAX = 0x2BADB002`) and info pointer (`EBX`) are saved.
4. Bootstrap code constructs page tables (`initial_pml4`, `initial_pdpt`, `initial_pd`) identity-mapping the first 1 GB with 512 x 2MB huge pages.
5. Loads `CR3` with PML4 address.
6. Enables PAE in `CR4` (bit 5), enables SSE/FPU in `CR0` and `CR4`.
7. Enables Long Mode (LME) in EFER MSR (`0xC0000080`, bit 8).
8. Enables Paging (`PG`) and Protection (`PE`) in `CR0`.
9. Loads 64-bit GDT (`lgdt [gdt64_ptr]`) and executes far jump `jmp 0x08:long_mode_entry`.
10. Long mode reloads segment registers (`DS`, `ES`, `FS`, `GS`, `SS` = `0x10`), sets up a 32 KB stack, and invokes `kmain()`.

### Memory Management & Paging
- **Physical Memory (PMM)**: Inspects the Multiboot memory map, marks usable RAM, and provides frame allocation (`pmm_alloc_frame`, `pmm_free_frame`) via a bitmap.
- **Virtual Memory Paging**: Walks the 4-level hierarchy. When mapping a virtual address, unallocated PML4, PDPT, or PD entries are dynamically allocated as 4KB frames from PMM. If encountering an existing 2MB huge page, it automatically splits it into 512 x 4KB page entries.
- **Kernel Heap**: A boundary-tag allocator (`kmalloc`, `kfree`) over a 4MB heap buffer. Free blocks are merged with contiguous neighbors (`coalesce`) upon release to prevent fragmentation.

### Multitasking & Context Switching
- Tasks possess a dedicated 16 KB stack allocated on the heap.
- Context switching is performed in assembly (`switch_context.S`) by pushing callee-saved registers (`RBX`, `RBP`, `R12-R15`, `RFLAGS`), saving `RSP` into the outgoing `task_t`, loading the incoming task's `RSP`, restoring registers, and executing `ret`.
- New tasks trampoline into execution with interrupts enabled and automatically call `task_exit()` upon completion.
- Preemption is driven by the 100 Hz PIT timer interrupt: every 5 ticks (50ms quantum), the timer interrupt invokes `task_yield()`.

### Synchronization & Deadlock Protection
- **Spinlocks**: Atomic test-and-set with `pause` loop.
- **Deadlock Watchdog**: `spinlock_acquire_timeout()` detects circular wait conditions (Task 1 holds A, waits on B; Task 2 holds B, waits on A). If contending locks cannot be acquired within the watchdog threshold, the deadlock condition is logged and safely broken by releasing contending locks, avoiding OS lockups.

### System Calls & Virtual Filesystem
- System calls use software interrupt `int $0x80`. Registers `RAX`, `RDI`, `RSI`, `RDX` pass syscall number and arguments.
- RAMFS supports in-memory file creation, data reading/writing, deletion, and directory listings with timestamps and inode tracking.

---

## 7. Sample Boot & Execution Output

```
================================================================
        __  __ _       _    ____   ____                         
       |  \/  (_)_ __ (_)  / __ \ / ___|                        
       | |\/| | | '_ \| | | |  | |\___ \                        
       | |  | | | | | | | | |__| | ___) |                       
       |_|  |_|_|_| |_|_|  \____/ |____/                        
                                                                
       Mini OS: Modular x86-64 Educational Kernel               
================================================================
[+] Architecture : x86-64 (AMD64 / Long Mode)
[+] CPU Vendor   : AuthenticAMD
[+] Bootloader   : Multiboot Compliant (Magic: 0x2badb002)
[+] GDT & TSS    : 64-bit Descriptors initialized (Kernel/User Code/Data, TSS @ 0x115020)
[+] IDT          : 256-entry Interrupt Descriptor Table loaded
[+] ISR Handlers : Registered 32 CPU exceptions, 16 IRQs, and 0x80 Syscall gate
[+] 8259 PIC     : Remapped (Master IRQs -> 0x20..0x27, Slave IRQs -> 0x28..0x2F)
[+] PMM Bitmap   : Manages 127 MB RAM (32736 frames, 31430 free, 1306 used)
[+] Paging (x86-64): 4-level paging active (PML4 CR3: 0x10a000)
[+] Heap (kmalloc): Initialized 4096 KB free kernel heap at 0x118000
[+] PIT Timer    : Initialized at 100 Hz (Period: 10 ms)
[+] CPU Flags    : Interrupts enabled (IF = 1)

--- Milestone 2 Verification: CPU, Interrupts, and Memory ---
[+] Interrupt Handler: Caught Breakpoint (#BP, Vector 3) safely!
[PASS] CPU Exception #BP caught and recovered safely.
[+] Timer Check  : Waiting 5 ticks (50ms)...
[PASS] Timer Ticks: Progressed from 2 to 7 (5 ticks elapsed)
[+] Heap Test    : Allocated p1=0x118020 (128B), p2=0x1180c0 (512B), p3=0x1182e0 (1KB)
[+] Heap Content : "Hello from kmalloc heap!"
[PASS] Heap Coalescing: Freed all allocations cleanly.
[Heap Stats] Used: 0 bytes (0 blocks) | Free: 4194272 bytes (1 blocks)
[+] Paging Test  : Mapping Virtual 0xcafe0000 -> Physical 0x51a000
[+] Paging Walk  : Translated 0xcafe0000 -> 0x51a000
[PASS] Virtual Memory: Read/write verified through 4-level page tables.
[PASS] Page Unmapped successfully.
----------------------------------------------------------------

--- Milestone 3 Verification: Tasks, Context Switching, & Scheduler ---
[+] Scheduler    : Initialized Round-Robin Multitasking (Main Task PID: 0)
[+] Task Created : PID 1 ("Worker-Alpha") | Stack: 0x1180c0..0x11c0c0 | Init RSP: 0x11c080
[+] Task Created : PID 2 ("Worker-Beta") | Stack: 0x11c180..0x120180 | Init RSP: 0x120140
[+] Task Created : PID 3 ("Worker-Gamma") | Stack: 0x120240..0x124240 | Init RSP: 0x124200
[Task Alpha] Started (PID: 1). Computing Fibonacci sequence...
[Task Alpha] Iteration 1/3 -> Fib(5) = 5. Yielding CPU.
[Task Beta]  Started (PID: 2). Periodic status worker...
[Task Beta]  Iteration 1/3: Heartbeat OK (Tick: 17). Yielding CPU.
[Task Gamma] Started (PID: 3). Testing task lifecycle states...
[Task Gamma] Iteration 1/2: State is RUNNING. Yielding.

============================ PROCESS / TASK TABLE ============================
PID  | NAME                | STATE       | PRIO | RUNTIME  | SWITCHES | RSP
-----+---------------------+-------------+------+----------+----------+------------------
 3   | Worker-Gamma        | READY       |  1   | 0 ticks  |    1     | 0x1241e0
 2   | Worker-Beta         | READY       |  2   | 0 ticks  |    1     | 0x120120
 1   | Worker-Alpha        | READY       |  2   | 0 ticks  |    1     | 0x11c050
 0   | kmain_thread        | RUNNING     |  1   | 6 ticks  |    1     | 0x114d80
==============================================================================

[+] Main Thread: Yielding control to let child tasks progress...
[Task Alpha] Iteration 2/3 -> Fib(10) = 55. Yielding CPU.
[Task Beta]  Iteration 2/3: Heartbeat OK (Tick: 19). Yielding CPU.
[Task Gamma] Iteration 2/2: State is RUNNING. Yielding.
[Task Alpha] Iteration 3/3 -> Fib(15) = 610. Yielding CPU.
[Task Beta]  Iteration 3/3: Heartbeat OK (Tick: 21). Yielding CPU.
[Task Gamma] Lifecycle test complete. Exiting.
[*] Task Exit    : PID 3 ("Worker-Gamma") terminated cleanly (ran 2 ticks, 3 switches)
[Task Alpha] Finished computation. Exiting.
[*] Task Exit    : PID 1 ("Worker-Alpha") terminated cleanly (ran 0 ticks, 4 switches)
[Task Beta]  Finished worker cycles. Exiting.
[*] Task Exit    : PID 2 ("Worker-Beta") terminated cleanly (ran 2 ticks, 4 switches)

[PASS] Context Switching: All tasks progressed cooperatively and ran to completion!

============================ PROCESS / TASK TABLE ============================
PID  | NAME                | STATE       | PRIO | RUNTIME  | SWITCHES | RSP
-----+---------------------+-------------+------+----------+----------+------------------
 3   | Worker-Gamma        | TERMINATED  |  1   | 3 ticks  |    3     | 0x1241e0
 2   | Worker-Beta         | TERMINATED  |  2   | 2 ticks  |    4     | 0x120120
 1   | Worker-Alpha        | TERMINATED  |  2   | 1 ticks  |    4     | 0x11c060
 0   | kmain_thread        | RUNNING     |  1   | 8 ticks  |    4     | 0x114f70
==============================================================================
----------------------------------------------------------------

--- Milestone 4 Verification: Synchronization, Deadlocks, Syscalls, & VFS ---

[1] Synchronization & Mutual Exclusion:
[+] Spinlock     : Acquiring lock "demo_lock"...
[+] Spinlock     : Lock acquired by PID 0! Incrementing shared counter...
[+] Shared Value : 1000 (Protected from concurrent race conditions)
[PASS] Spinlock  : Released successfully (locked: 0)

[2] Safe Deadlock Detection & Recovery Demonstration:
[+] Scenario     : Task 1 acquires Resource A, Task 2 acquires Resource B
[+] State        : Resource A held by PID 1 | Resource B held by PID 2
[+] Contention   : PID 1 requests Resource B, while PID 2 requests Resource A
[!] DEADLOCK DETECTED: Circular wait condition!
    PID 1 holds Resource A, blocked waiting on Resource B
    PID 2 holds Resource B, blocked waiting on Resource A
[+] Deadlock Recovery: Watchdog breaks cycle by preempting PID 2's hold on Resource B!
[PASS] Deadlock Resolved: System restored to consistent state without freezing.

[3] System Call Interface (Software Trap int 0x80):
[+] System Calls : Registered 0x80 trap handler with 11 service calls
[PASS] sys_getpid() via int 0x80 -> Returned PID: 0
[PASS] sys_print()  via int 0x80 -> Software interrupt string output verified!
[PASS] sys_malloc() via int 0x80 -> Allocated buffer at 0x124260
[PASS] sys_free()   via int 0x80 -> Buffer deallocated cleanly.

[4] In-Memory Virtual File System (RAMFS):
[+] RAMFS VFS    : Initialized In-Memory File System with default files
[+] VFS Create   : Creating file "/test.txt"...
[+] VFS Write    : Writing 57 bytes to "/test.txt"...
[+] VFS Read     : Read 57 bytes back from "/test.txt":
                   "Mini OS RAMFS read/write verification string: 1234567890!"
[PASS] VFS Content: Read content matches written data byte-for-byte!

=============================== IN-MEMORY VFS LISTING ===============================
INODE | TYPE | SIZE (BYTES) | MODIFIED TICK | PATH
------+------+--------------+---------------+----------------------------------------
  5   | FILE | 57 bytes     | Tick 46       | /test.txt
  4   | FILE | 118 bytes    | Tick 45       | /experiments/hello.c
  3   | FILE | 199 bytes    | Tick 45       | /sysinfo.txt
  2   | FILE | 162 bytes    | Tick 45       | /readme.txt
  1   | DIR  | 0 bytes      | Tick 45       | /
=====================================================================================
[PASS] VFS Delete  : Deleted "/test.txt" cleanly.
----------------------------------------------------------------

================================================================
             MINI OS AUTOMATED KERNEL SMOKE TESTS               
================================================================
[TEST  1/12] x86-64 Long Mode & Freestanding C Runtime ... [PASS]
[TEST  2/12] Global Descriptor Table (GDT) & TSS State ... [PASS]
[TEST  3/12] IDT Vector Table & CPU Exception Traps   ... [PASS]
[TEST  4/12] 8259 PIC Remap & 8254 PIT Periodic Ticks ... [PASS] (60 ticks)
[TEST  5/12] PMM Frame Allocator (4KB Page Frames)    ... [PASS] (Allocated & Freed frames)
[TEST  6/12] 4-Level Virtual Memory Paging Translation... [PASS] (V:0xd0000000 -> P:0x51a000 verified)
[TEST  7/12] Boundary-Tag Kernel Heap Allocator       ... [PASS] (kmalloc/kfree validated)
[TEST  8/12] Real x86-64 Task Stacks & Round-Robin    ... [PASS] (Verified via cooperative & preemptive tasks)
[TEST  9/12] Atomic Spinlocks & Critical Section      ... [PASS] (Mutual exclusion guaranteed)
[TEST 10/12] Deadlock Detection & Safe Watchdog Break ... [PASS] (Circular wait caught and broken safely)
[TEST 11/12] Software Trap System Call Interface      ... [PASS] (int 0x80 SYS_GETPID -> PID 0)
[TEST 12/12] In-Memory VFS (Create, Write, Read, Del) ... [PASS] (100% byte-accurate readback)
================================================================
       SMOKE TEST RESULT: 12/12 TESTS PASSED (100% SUCCESS)     
================================================================

================================================================
   [MINI OS: ALL 5 MILESTONES FULLY OPERATIONAL AND VERIFIED]   
================================================================
```

---

## 8. Known Limitations & Roadmap

1. **Hardware Driver Scope**: Serial (16550A) and VGA text mode are supported; storage devices (AHCI/NVMe/IDE) are represented via RAMFS rather than block device drivers.
2. **User Mode Separation**: Tasks execute as kernel threads sharing the kernel page directory; user/kernel protection boundary is verified through the `int 0x80` DPL 3 trap gate mechanism. Separate user address space mapping per process (`CR3` reloading on context switch) is the primary future roadmap item.
3. **Multiprocessing**: The current scheduler manages single-core x86-64 execution. Multi-core SMP with APIC initialization and IPIs is planned for future iterations.

---

## 9. Troubleshooting

- **QEMU fails with "Cannot load x86-64 image, give a 32bit one"**:
  Multiboot 1 standard requires a 32-bit ELF wrapper. The Makefile automatically generates this using `objcopy -O elf32-i386 build/kernel.elf build/kernel32.elf`. Ensure `objcopy` from GNU Binutils is installed.
- **QEMU displays blank window**:
  By default, `make run` outputs to the terminal using `-serial stdio -display none`. To view the graphical VGA display window, run `make run-display`.
- **Compiler errors on SSE instructions**:
  Freestanding x86-64 kernels cannot use SSE registers before CR0/CR4 configuration. The Makefile enforces `-mgeneral-regs-only` to ensure standard integer register code generation.

---

## 10. Author & Repository
- **Author**: Nidha Ahmed
- **Repository**: [https://github.com/nidhaahmed/Mini-OS.git](https://github.com/nidhaahmed/Mini-OS.git)