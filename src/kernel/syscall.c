#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/memory.h"
#include "kernel/vfs.h"
#include "kernel/console.h"

void syscall_dispatcher(interrupt_frame_t* frame) {
    uint64_t syscall_num = frame->rax;
    uint64_t arg1 = frame->rdi;
    uint64_t arg2 = frame->rsi;
    uint64_t arg3 = frame->rdx;

    switch (syscall_num) {
        case SYS_YIELD:
            task_yield();
            frame->rax = 0;
            break;

        case SYS_GETPID:
            frame->rax = task_get_current() ? task_get_current()->pid : 0;
            break;

        case SYS_SLEEP:
            task_sleep(arg1);
            frame->rax = 0;
            break;

        case SYS_PRINT:
            kprintf("%s", (const char*)arg1);
            frame->rax = 0;
            break;

        case SYS_MALLOC:
            frame->rax = (uint64_t)kmalloc((size_t)arg1);
            break;

        case SYS_FREE:
            kfree((void*)arg1);
            frame->rax = 0;
            break;

        case SYS_FS_CREATE:
            frame->rax = (uint64_t)vfs_create((const char*)arg1, (uint32_t)arg2);
            break;

        case SYS_FS_WRITE:
            frame->rax = (int64_t)vfs_write((const char*)arg1, (const void*)arg2, (size_t)arg3);
            break;

        case SYS_FS_READ:
            frame->rax = (int64_t)vfs_read((const char*)arg1, (void*)arg2, (size_t)arg3);
            break;

        case SYS_FS_LIST:
            vfs_list();
            frame->rax = 0;
            break;

        case SYS_EXIT:
            task_exit();
            frame->rax = 0;
            break;

        default:
            kprintf("[!] Syscall Error: Unknown syscall %u called from PID %u\n",
                    (uint32_t)syscall_num,
                    task_get_current() ? task_get_current()->pid : 0);
            frame->rax = (uint64_t)-1;
            break;
    }
}

void syscall_init(void) {
    isr_register_handler(128, syscall_dispatcher);
    kprintf("[+] System Calls : Registered 0x80 trap handler with %u service calls\n", SYS_EXIT + 1);
}

int64_t sys_call(int64_t num, int64_t a1, int64_t a2, int64_t a3) {
    int64_t ret;
    __asm__ volatile (
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "movq %3, %%rsi\n"
        "movq %4, %%rdx\n"
        "int $0x80\n"
        "movq %%rax, %0\n"
        : "=r"(ret)
        : "r"(num), "r"(a1), "r"(a2), "r"(a3)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
    return ret;
}

int32_t sys_getpid(void) {
    return (int32_t)sys_call(SYS_GETPID, 0, 0, 0);
}

void sys_yield(void) {
    sys_call(SYS_YIELD, 0, 0, 0);
}

void sys_sleep(uint64_t ticks) {
    sys_call(SYS_SLEEP, (int64_t)ticks, 0, 0);
}

void sys_print(const char* msg) {
    sys_call(SYS_PRINT, (int64_t)msg, 0, 0);
}

void* sys_malloc(size_t size) {
    return (void*)sys_call(SYS_MALLOC, (int64_t)size, 0, 0);
}

void sys_free(void* ptr) {
    sys_call(SYS_FREE, (int64_t)ptr, 0, 0);
}
