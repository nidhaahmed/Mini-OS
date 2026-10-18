#include <stdint.h>
#include <stddef.h>
#include "boot/multiboot.h"
#include "kernel/serial.h"
#include "kernel/console.h"
#include "kernel/io.h"
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

void kmain(multiboot_info_t* mboot, uint32_t magic) {
    // Initialize serial first for immediate logging
    serial_init();

    // Initialize VGA console (and dual serial logging)
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

    if (mboot && (mboot->flags & MULTIBOOT_INFO_MEMORY)) {
        uint64_t total_ram_kb = mboot->mem_lower + mboot->mem_upper;
        kprintf("[+] System RAM   : %u KB (~%u MB)\n", total_ram_kb, total_ram_kb / 1024);
    }

    if (mboot && (mboot->flags & MULTIBOOT_INFO_BOOTDEV)) {
        kprintf("[+] Boot Device  : 0x%x\n", mboot->boot_device);
    }

    kprintf("[+] Console      : VGA 80x25 text buffer (0xB8000) + COM1 115200 8N1\n");
    kprintf("[+] Status       : Milestone 1 (Bootable Foundation) Active!\n");
    kprintf("----------------------------------------------------------------\n");
    kprintf("Mini OS Kernel is alive and waiting for next milestones...\n");
}
