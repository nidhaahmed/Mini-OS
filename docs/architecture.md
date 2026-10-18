# Mini OS: Kernel Architecture & Subsystems

Mini OS is a modular, educational x86-64 operating system designed to demonstrate core operating system concepts in a freestanding, 64-bit environment without external runtime dependencies.

---

## 1. System Boot Flow & Initialization

```
+-------------------------------------------------------------+
|                     QEMU / BIOS Boot                         |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|               Multiboot 1 Header (0x1BADB002)               |
|  - Bootloader starts in 32-bit Protected Mode at 1MB       |
|  - Saves Multiboot Magic & Info Pointer                     |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|              src/boot/boot.S (Bootstrap Assembly)           |
|  1. Setup initial 4-level page tables (PML4, PDPT, PD)      |
|  2. Identity-map first 1 GB with 512 x 2MB huge pages       |
|  3. Load CR3 with PML4 base address                         |
|  4. Enable PAE (CR4 bit 5) & OSFXSR/OSXMMEXCPT (bits 9-10)  |
|  5. Set Long Mode Enable (LME) in EFER MSR (0xC0000080)     |
|  6. Enable Paging & PE in CR0 (bits 31 & 0), enable FPU     |
|  7. Load 64-bit GDT and long jump to 64-bit code segment    |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                   64-bit Long Mode Entry                    |
|  - Reload segment registers (DS, ES, FS, GS, SS = 0x10)     |
|  - Setup 32 KB initial kernel stack                         |
|  - Pass Multiboot args via System V AMD64 ABI               |
|  - Call kmain(multiboot_info_t*, uint32_t magic)            |
+-------------------------------------------------------------+
```

---

## 2. Kernel Subsystems Overview

### A. CPU Management (GDT & TSS)
- **GDT (Global Descriptor Table)**: Configures 64-bit flat memory segments:
  - `0x00`: Null descriptor
  - `0x08`: 64-bit Kernel Code Segment (DPL 0, RX)
  - `0x10`: 64-bit Kernel Data Segment (DPL 0, RW)
  - `0x18`: 64-bit User Data Segment (DPL 3, RW)
  - `0x20`: 64-bit User Code Segment (DPL 3, RX)
  - `0x28`: 64-bit Task State Segment (TSS, 16 bytes)
- **TSS**: Holds `rsp0` for privilege transitions and IST (Interrupt Stack Table).

### B. Interrupt Management (IDT, PIC, & PIT)
- **IDT (Interrupt Descriptor Table)**: 256 gates configured for:
  - Vectors 0–31: CPU Hardware Exceptions (#DE, #BP, #GP, #PF, etc.)
  - Vectors 32–47: Hardware IRQs remapped from 8259 PIC
  - Vector 128 (0x80): Software Trap System Call Gate (`IDT_ATTR_USER_GATE`)
- **8259 PIC**: Master remapped to `0x20..0x27`, Slave remapped to `0x28..0x2F`.
- **8254 PIT**: Configured at 100 Hz (10ms tick period) providing preemptive timeslices and timekeeping.

### C. Memory Management (PMM, Paging, & Heap)
- **Physical Memory Manager (PMM)**:
  - Bitmap frame allocator managing 4KB physical page frames.
  - Parses Multiboot memory map to identify usable RAM.
  - Reserves lower 1MB, kernel binary, and bitmap structures.
- **4-Level Paging**:
  - Full x86-64 paging hierarchy: PML4 $\to$ PDPT $\to$ PD $\to$ PT.
  - Functions: `paging_map_page()`, `paging_unmap_page()`, `paging_get_phys()`.
  - Supports dynamic page table allocation and automatic 2MB huge-page splitting into 4KB pages.
- **Kernel Heap Allocator (`kmalloc` / `kfree`)**:
  - Boundary-tag coalescing memory allocator managing a 4MB kernel heap pool.
  - 16-byte aligned payloads with header sanity checks (`0xDEADBEEF`).
  - Automatic adjacent block coalescing on `kfree` to prevent heap fragmentation.

### D. Process Model & Round-Robin Scheduler
- **Task Control Block (`task_t`)**:
  - PID, Name, Lifecycle State (`EMBRYO`, `READY`, `RUNNING`, `BLOCKED`, `SLEEPING`, `TERMINATED`), Priority, Saved RSP, 16KB Stack Base, CPU Runtime Ticks, Context Switch Count.
- **Context Switching (`switch_context`)**:
  - Real assembly context switch saving callee-saved registers (`RBX`, `RBP`, `R12-R15`, `RFLAGS`) and exchanging stack pointers.
- **Scheduler**:
  - Cooperative yielding (`task_yield`) and sleeping (`task_sleep`).
  - Preemptive time-slicing via PIT timer interrupt (quantum = 5 ticks = 50ms).

### E. Synchronization & Concurrency
- **Spinlock (`spinlock_t`)**:
  - Atomic test-and-set using GCC atomic builtins with x86 `pause` instruction.
  - Includes `spinlock_acquire_timeout` to detect and break potential deadlocks safely.
- **Mutex (`mutex_t`)**:
  - Blocking mutex that suspends contending tasks (`TASK_BLOCKED`) and yields CPU until unlocked.

### F. System Call Interface (`int 0x80`)
- Trap gate vector 128 (`0x80`) providing 11 system calls:
  `SYS_YIELD`, `SYS_GETPID`, `SYS_SLEEP`, `SYS_PRINT`, `SYS_MALLOC`, `SYS_FREE`, `SYS_FS_CREATE`, `SYS_FS_WRITE`, `SYS_FS_READ`, `SYS_FS_LIST`, `SYS_EXIT`.

### G. In-Memory Virtual File System (RAMFS)
- Hierarchical file abstraction supporting:
  - Directories and regular files.
  - Dynamic file creation, reading, writing, directory listing, and deletion.
  - Pre-populated with default educational files (`/readme.txt`, `/sysinfo.txt`, `/experiments/hello.c`).
