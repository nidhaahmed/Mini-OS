#include "kernel/gdt.h"
#include "kernel/console.h"
#include "lib/string.h"

#define GDT_TOTAL_ENTRIES 7

static uint64_t gdt_entries[GDT_TOTAL_ENTRIES];
static struct gdt_ptr gdt_descriptor;
static struct tss_entry kernel_tss;

extern void stack_top(void);

static void gdt_set_gate(int num, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran) {
    uint64_t entry = 0;

    // Limit bits 0..15
    entry |= (limit & 0xFFFFULL);
    // Base bits 0..23
    entry |= ((base & 0xFFFFFFULL) << 16);
    // Access byte
    entry |= ((uint64_t)access << 40);
    // Granularity & limit bits 16..19
    entry |= (((limit >> 16) & 0x0FULL) << 48);
    entry |= (((uint64_t)gran & 0xF0ULL) << 48);
    // Base bits 24..31
    entry |= (((base >> 24) & 0xFFULL) << 56);

    gdt_entries[num] = entry;
}

static void gdt_set_tss_gate(int num, uint64_t base, uint32_t limit) {
    // In x86-64, a TSS descriptor is 16 bytes (2 GDT slots: num and num+1)
    gdt_set_gate(num, base & 0xFFFFFFFF, limit, 0x89, 0x00);

    // High 32 bits of 64-bit base address occupy the second slot
    gdt_entries[num + 1] = (base >> 32) & 0xFFFFFFFFULL;
}

void tss_set_stack(uint64_t stack) {
    kernel_tss.rsp0 = stack;
}

void gdt_init(void) {
    memset(gdt_entries, 0, sizeof(gdt_entries));
    memset(&kernel_tss, 0, sizeof(kernel_tss));

    kernel_tss.rsp0 = (uint64_t)&stack_top;
    kernel_tss.iomap_base = sizeof(kernel_tss);

    // 0x00: Null descriptor
    gdt_entries[0] = 0x0000000000000000ULL;

    // 0x08: Kernel Code 64 (Base=0, Limit=0xFFFFF, Access=0x9A, Gran=0xA0 for L=1)
    gdt_entries[1] = 0x00AF9A000000FFFFULL;

    // 0x10: Kernel Data 64 (Base=0, Limit=0xFFFFF, Access=0x92, Gran=0xC0)
    gdt_entries[2] = 0x00CF92000000FFFFULL;

    // 0x18: User Data 64 (Base=0, Limit=0xFFFFF, Access=0xF2, Gran=0xC0)
    gdt_entries[3] = 0x00CFF2000000FFFFULL;

    // 0x20: User Code 64 (Base=0, Limit=0xFFFFF, Access=0xFA, Gran=0xA0 for L=1)
    gdt_entries[4] = 0x00AFFA000000FFFFULL;

    // 0x28 & 0x30: 64-bit TSS descriptor
    gdt_set_tss_gate(5, (uint64_t)&kernel_tss, sizeof(kernel_tss) - 1);

    gdt_descriptor.limit = (sizeof(uint64_t) * GDT_TOTAL_ENTRIES) - 1;
    gdt_descriptor.base  = (uint64_t)&gdt_entries[0];

    // Load GDTR
    __asm__ volatile ("lgdt %0" : : "m"(gdt_descriptor));

    // Reload segment registers
    __asm__ volatile (
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : : "rax"
    );

    // Load Task Register (TSS selector = 0x28)
    __asm__ volatile ("ltr %%ax" : : "a"((uint16_t)0x28));

    kprintf("[+] GDT & TSS    : 64-bit Descriptors initialized (Kernel/User Code/Data, TSS @ %p)\n", (void*)&kernel_tss);
}
