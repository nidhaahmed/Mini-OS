#include "kernel/idt.h"
#include "kernel/gdt.h"
#include "kernel/console.h"
#include "lib/string.h"

static struct idt_entry idt_entries[IDT_ENTRIES];
static struct idt_ptr idt_descriptor;

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist) {
    idt_entries[num].offset_low  = (uint16_t)(base & 0xFFFF);
    idt_entries[num].selector    = sel;
    idt_entries[num].ist         = ist & 0x7;
    idt_entries[num].type_attr   = flags;
    idt_entries[num].offset_mid  = (uint16_t)((base >> 16) & 0xFFFF);
    idt_entries[num].offset_high = (uint32_t)((base >> 32) & 0xFFFFFFFF);
    idt_entries[num].reserved    = 0;
}

void idt_init(void) {
    memset(idt_entries, 0, sizeof(idt_entries));

    idt_descriptor.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idt_descriptor.base  = (uint64_t)&idt_entries[0];

    __asm__ volatile ("lidt %0" : : "m"(idt_descriptor));

    kprintf("[+] IDT          : 256-entry Interrupt Descriptor Table loaded\n");
}
