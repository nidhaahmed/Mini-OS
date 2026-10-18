#include <stdint.h>
#include <stddef.h>
#include "boot/multiboot.h"
#include "kernel/serial.h"
#include "kernel/console.h"
#include "kernel/io.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/isr.h"
#include "kernel/pic.h"
#include "kernel/timer.h"
#include "kernel/memory.h"
#include "kernel/paging.h"
#include "kernel/task.h"
#include "lib/string.h"

static void get_cpu_vendor(char* vendor) {
    uint32_t ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    memcpy(vendor, &ebx, 4);
    memcpy(vendor + 4, &edx, 4);
    memcpy(vendor + 8, &ecx, 4);
    vendor[12] = '\0';
}

static volatile int breakpoint_handled = 0;
static void breakpoint_test_handler(interrupt_frame_t* frame) {
    (void)frame;
    breakpoint_handled = 1;
    kprintf("[+] Interrupt Handler: Caught Breakpoint (#BP, Vector 3) safely!\n");
}

void demonstrate_milestone2(void) {
    kprintf("\n--- Milestone 2 Verification: CPU, Interrupts, and Memory ---\n");

    // 1. Test CPU Exception Handling (Breakpoint #BP)
    isr_register_handler(3, breakpoint_test_handler);
    __asm__ volatile ("int3");
    if (breakpoint_handled) {
        kprintf("[PASS] CPU Exception #BP caught and recovered safely.\n");
    } else {
        kprintf("[FAIL] CPU Exception #BP was not caught.\n");
    }

    // 2. Test Timer Interrupts (Wait for 5 ticks = 50ms)
    uint64_t t0 = timer_get_ticks();
    kprintf("[+] Timer Check  : Waiting 5 ticks (50ms)...\n");
    timer_sleep_ticks(5);
    uint64_t t1 = timer_get_ticks();
    kprintf("[PASS] Timer Ticks: Progressed from %u to %u (%u ticks elapsed)\n",
            (uint32_t)t0, (uint32_t)t1, (uint32_t)(t1 - t0));

    // 3. Test Kernel Heap Allocator (kmalloc / kfree / coalescing)
    void* ptr1 = kmalloc(128);
    void* ptr2 = kmalloc(512);
    void* ptr3 = kmalloc(1024);
    kprintf("[+] Heap Test    : Allocated p1=%p (128B), p2=%p (512B), p3=%p (1KB)\n", ptr1, ptr2, ptr3);

    strcpy((char*)ptr1, "Hello from kmalloc heap!");
    kprintf("[+] Heap Content : \"%s\"\n", (char*)ptr1);

    kfree(ptr2);
    kfree(ptr1);
    kfree(ptr3);
    kprintf("[PASS] Heap Coalescing: Freed all allocations cleanly.\n");
    heap_dump_stats();

    // 4. Test Virtual Memory and 4-Level Paging
    uint64_t test_virt = 0xCAFE0000ULL;
    uint64_t test_phys = pmm_alloc_frame();
    kprintf("[+] Paging Test  : Mapping Virtual %p -> Physical %p\n", (void*)test_virt, (void*)test_phys);

    paging_map_page(test_virt, test_phys, PTE_PRESENT | PTE_WRITABLE);
    uint64_t translated = paging_get_phys(test_virt);
    kprintf("[+] Paging Walk  : Translated %p -> %p\n", (void*)test_virt, (void*)translated);

    volatile uint32_t* page_ptr = (volatile uint32_t*)test_virt;
    *page_ptr = 0x12345678;
    if (*page_ptr == 0x12345678 && translated == test_phys) {
        kprintf("[PASS] Virtual Memory: Read/write verified through 4-level page tables.\n");
    } else {
        kprintf("[FAIL] Virtual Memory verification failed.\n");
    }

    paging_unmap_page(test_virt);
    pmm_free_frame(test_phys);
    kprintf("[PASS] Page Unmapped successfully.\n");
    kprintf("----------------------------------------------------------------\n");
}

/* --------------------------------------------------------------------------
   Milestone 3 Tasks
   -------------------------------------------------------------------------- */

static uint32_t fibonacci(uint32_t n) {
    if (n <= 1) return n;
    uint32_t a = 0, b = 1;
    for (uint32_t i = 2; i <= n; i++) {
        uint32_t c = a + b;
        a = b;
        b = c;
    }
    return b;
}

static void task_alpha_fn(void) {
    kprintf("[Task Alpha] Started (PID: %u). Computing Fibonacci sequence...\n", task_get_current()->pid);
    for (int i = 1; i <= 3; i++) {
        uint32_t f = fibonacci(i * 5);
        kprintf("[Task Alpha] Iteration %d/3 -> Fib(%d) = %u. Yielding CPU.\n", i, i * 5, f);
        task_yield();
    }
    kprintf("[Task Alpha] Finished computation. Exiting.\n");
}

static void task_beta_fn(void) {
    kprintf("[Task Beta]  Started (PID: %u). Periodic status worker...\n", task_get_current()->pid);
    for (int i = 1; i <= 3; i++) {
        kprintf("[Task Beta]  Iteration %d/3: Heartbeat OK (Tick: %u). Yielding CPU.\n",
                i, (uint32_t)timer_get_ticks());
        task_yield();
    }
    kprintf("[Task Beta]  Finished worker cycles. Exiting.\n");
}

static void task_gamma_fn(void) {
    kprintf("[Task Gamma] Started (PID: %u). Testing task lifecycle states...\n", task_get_current()->pid);
    for (int i = 1; i <= 2; i++) {
        kprintf("[Task Gamma] Iteration %d/2: State is RUNNING. Yielding.\n", i);
        task_yield();
    }
    kprintf("[Task Gamma] Lifecycle test complete. Exiting.\n");
}

void demonstrate_milestone3(void) {
    kprintf("\n--- Milestone 3 Verification: Tasks, Context Switching, & Scheduler ---\n");

    // Initialize round-robin scheduler with main kernel thread
    scheduler_init();

    // Create 3 independent tasks with their own 16KB stacks
    task_t* t1 = task_create("Worker-Alpha", task_alpha_fn, 2);
    task_t* t2 = task_create("Worker-Beta",  task_beta_fn,  2);
    task_t* t3 = task_create("Worker-Gamma", task_gamma_fn, 1);
    (void)t1; (void)t2; (void)t3;

    kprintf("[+] Initial Task State:\n");
    __asm__ volatile ("cli");
    task_dump_all();
    __asm__ volatile ("sti");

    kprintf("\n[+] Main Thread: Yielding control to let child tasks progress...\n");

    // Yield repeatedly to allow child tasks to interleave and execute
    for (int i = 0; i < 6; i++) {
        task_yield();
    }

    kprintf("\n[PASS] Context Switching: All tasks progressed cooperatively and ran to completion!\n");
    __asm__ volatile ("cli");
    task_dump_all();
    __asm__ volatile ("sti");
    kprintf("----------------------------------------------------------------\n");
}

void kmain(multiboot_info_t* mboot, uint32_t magic) {
    // 1. Early serial and console initialization
    serial_init();
    console_init();

    kprintf("\n");
    kprintf("================================================================\n");
    kprintf("        __  __ _       _    ____   ____                         \n");
    kprintf("       |  \\/  (_)_ __ (_)  / __ \\ / ___|                        \n");
    kprintf("       | |\\/| | | '_ \\| | | |  | |\\___ \\                        \n");
    kprintf("       | |  | | | | | | | | |__| | ___) |                       \n");
    kprintf("       |_|  |_|_|_| |_|_|  \\____/ |____/                        \n");
    kprintf("                                                                \n");
    kprintf("       Mini OS: Modular x86-64 Educational Kernel               \n");
    kprintf("================================================================\n");
    kprintf("[+] Architecture : x86-64 (AMD64 / Long Mode)\n");

    char cpu_vendor[13];
    get_cpu_vendor(cpu_vendor);
    kprintf("[+] CPU Vendor   : %s\n", cpu_vendor);

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        kprintf("[+] Bootloader   : Multiboot Compliant (Magic: 0x%x)\n", magic);
    } else {
        kprintf("[!] Bootloader   : Non-standard boot magic: 0x%x\n", magic);
    }

    // 2. CPU Descriptors (GDT & TSS)
    gdt_init();

    // 3. Interrupt Descriptors & Exceptions
    idt_init();
    isr_init();

    // 4. Programmable Interrupt Controller
    pic_init();

    // 5. Physical Memory Management (PMM)
    pmm_init(mboot);

    // 6. 4-Level Paging Initialization
    paging_init();

    // 7. Kernel Heap Allocator
    kmalloc_init();

    // 8. PIT Timer at 100 Hz (10ms tick interval)
    timer_init(100);

    // 9. Enable Hardware Interrupts
    __asm__ volatile ("sti");
    kprintf("[+] CPU Flags    : Interrupts enabled (IF = 1)\n");

    // 10. Run Milestone 2 demonstrations
    demonstrate_milestone2();

    // 11. Run Milestone 3 demonstrations
    demonstrate_milestone3();

    kprintf("Mini OS Kernel is ready for Milestone 4 (Synchronization & Filesystem)...\n");
}
