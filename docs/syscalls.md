# Mini OS: System Call Interface & ABI

## 1. System Call Architecture

Mini OS provides a unified system call interface allowing software tasks to invoke privileged kernel services through a software interrupt trap (`int $0x80`).

---

## 2. Hardware Mechanism & Privilege Boundary

1. **IDT Gate Configuration**:
   - The system call gate is installed at vector **128 (0x80)** in the Interrupt Descriptor Table.
   - Its type attribute is configured as `IDT_ATTR_USER_GATE` (`0xEE`), having a Descriptor Privilege Level (**DPL = 3**).
   - This allows code running at any privilege level (including user-space Ring 3) to execute `int $0x80` without generating a General Protection Fault (`#GP`).

2. **Register ABI (Application Binary Interface)**:
   Parameters are passed in standard x86-64 registers matching the System V AMD64 ABI:
   - **`RAX`**: System Call Number
   - **`RDI`**: Argument 1
   - **`RSI`**: Argument 2
   - **`RDX`**: Argument 3
   - **`RAX` (return)**: System Call Return Value / Status Code

---

## 3. System Call Table

| Syscall # | Identifier | Arguments | Description |
|-----------|------------|-----------|-------------|
| `0` | `SYS_YIELD` | None | Yields remainder of current CPU quantum to next ready task |
| `1` | `SYS_GETPID` | None | Returns the calling process PID (`int32_t`) |
| `2` | `SYS_SLEEP` | `RDI`: ticks (`uint64_t`) | Suspends task until specified timer ticks elapse |
| `3` | `SYS_PRINT` | `RDI`: message string (`const char*`) | Outputs string to console and serial terminal |
| `4` | `SYS_MALLOC` | `RDI`: size in bytes (`size_t`) | Allocates memory from kernel heap |
| `5` | `SYS_FREE` | `RDI`: pointer (`void*`) | Frees previously allocated memory |
| `6` | `SYS_FS_CREATE` | `RDI`: path, `RSI`: type | Creates file or directory in RAMFS |
| `7` | `SYS_FS_WRITE` | `RDI`: path, `RSI`: buffer, `RDX`: size | Writes data to specified RAMFS file |
| `8` | `SYS_FS_READ` | `RDI`: path, `RSI`: buffer, `RDX`: max_size | Reads data from specified RAMFS file |
| `9` | `SYS_FS_LIST` | None | Prints formatted VFS directory listing |
| `10` | `SYS_EXIT` | None | Terminates current task execution |

---

## 4. Invocation Examples

### Assembly Invocation
```assembly
/* Example: Invoke SYS_PRINT (3) */
mov rax, 3                     /* System call number */
mov rdi, offset message_string /* Argument 1: buffer pointer */
int 0x80                       /* Trap into kernel */
```

### C Wrapper Invocation
```c
#include "kernel/syscall.h"

int main(void) {
    int32_t pid = sys_getpid();
    sys_print("Hello from system call!\n");
    sys_sleep(10); // Sleep for 10 ticks (100ms)
    return 0;
}
```

---

## 5. Privilege Transition Comparison

| Aspect | Real Hardware Trap (`int 0x80`) | Modern MSR Syscall (`syscall` / `sysret`) | Function Call Simulation |
|--------|---------------------------------|-------------------------------------------|--------------------------|
| **CPU Transition** | Automatically pushes SS, RSP, RFLAGS, CS, RIP to kernel stack | Saves RIP to RCX, RFLAGS to R11, jumps to LSTAR MSR | Normal stack call instruction |
| **Privilege Change**| Switches CPL from 3 to 0 via IDT gate | Switches CPL from 3 to 0 via MSR | None (stays in same ring) |
| **Status in Mini OS** | **Implemented & Verified** | Future Enhancement | Not Used (Real Traps Implemented) |
