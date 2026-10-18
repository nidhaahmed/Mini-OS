#include "kernel/isr.h"
#include "kernel/idt.h"
#include "kernel/gdt.h"
#include "kernel/pic.h"
#include "kernel/console.h"
#include "lib/string.h"

// External assembly ISR declarations
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void isr32(void);
extern void isr33(void);
extern void isr34(void);
extern void isr35(void);
extern void isr36(void);
extern void isr37(void);
extern void isr38(void);
extern void isr39(void);
extern void isr40(void);
extern void isr41(void);
extern void isr42(void);
extern void isr43(void);
extern void isr44(void);
extern void isr45(void);
extern void isr46(void);
extern void isr47(void);

extern void isr128(void);

static isr_handler_t handlers[IDT_ENTRIES];

static const char* const exception_names[32] = {
    "Divide-by-zero (#DE)",
    "Debug (#DB)",
    "Non-maskable Interrupt (#NMI)",
    "Breakpoint (#BP)",
    "Overflow (#OF)",
    "Bound Range Exceeded (#BR)",
    "Invalid Opcode (#UD)",
    "Device Not Available (#NM)",
    "Double Fault (#DF)",
    "Coprocessor Segment Overrun",
    "Invalid TSS (#TS)",
    "Segment Not Present (#NP)",
    "Stack-Segment Fault (#SS)",
    "General Protection Fault (#GP)",
    "Page Fault (#PF)",
    "Reserved Exception",
    "x87 Floating-Point Exception (#MF)",
    "Alignment Check (#AC)",
    "Machine Check (#MC)",
    "SIMD Floating-Point Exception (#XM)",
    "Virtualization Exception (#VE)",
    "Control Protection Exception (#CP)",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved",
    "Hypervisor Injection Exception (#HV)",
    "VMM Communication Exception (#VC)",
    "Security Exception (#SX)",
    "Reserved"
};

void isr_register_handler(uint8_t int_no, isr_handler_t handler) {
    handlers[int_no] = handler;
}

void isr_dispatch(interrupt_frame_t* frame) {
    if (frame->int_no < IDT_ENTRIES && handlers[frame->int_no]) {
        handlers[frame->int_no](frame);
    } else if (frame->int_no < 32) {
        // Unhandled CPU exception: Kernel Panic
        console_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        kprintf("                    KERNEL PANIC: CPU EXCEPTION                 \n");
        kprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        kprintf("Exception [%u]: %s\n", frame->int_no, exception_names[frame->int_no]);
        kprintf("Error Code    : 0x%x\n", frame->err_code);
        kprintf("RIP: 0x%p  CS: 0x%x  RFLAGS: 0x%p\n", (void*)frame->rip, (uint32_t)frame->cs, (void*)frame->rflags);
        kprintf("RSP: 0x%p  SS: 0x%x  RBP: 0x%p\n", (void*)frame->rsp, (uint32_t)frame->ss, (void*)frame->rbp);
        kprintf("RAX: 0x%p  RBX: 0x%p  RCX: 0x%p  RDX: 0x%p\n", (void*)frame->rax, (void*)frame->rbx, (void*)frame->rcx, (void*)frame->rdx);
        kprintf("RSI: 0x%p  RDI: 0x%p  R8 : 0x%p  R9 : 0x%p\n", (void*)frame->rsi, (void*)frame->rdi, (void*)frame->r8, (void*)frame->r9);
        kprintf("R10: 0x%p  R11: 0x%p  R12: 0x%p  R13: 0x%p\n", (void*)frame->r10, (void*)frame->r11, (void*)frame->r12, (void*)frame->r13);
        kprintf("R14: 0x%p  R15: 0x%p\n", (void*)frame->r14, (void*)frame->r15);

        if (frame->int_no == 14) {
            uint64_t cr2;
            __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
            kprintf("Faulting Linear Address (CR2): 0x%p\n", (void*)cr2);
            kprintf("Page Fault Reason: %s, %s, %s\n",
                    (frame->err_code & 1) ? "Page-protection violation" : "Non-present page",
                    (frame->err_code & 2) ? "Write access" : "Read access",
                    (frame->err_code & 4) ? "User mode" : "Kernel mode");
        }
        kprintf("System halted.\n");

        __asm__ volatile ("cli; hlt");
        while (1) { __asm__ volatile ("hlt"); }
    }

    // Acknowledge PIC if this was a hardware IRQ
    if (frame->int_no >= 32 && frame->int_no < 48) {
        pic_send_eoi((uint8_t)(frame->int_no - 32));
    }
}

void isr_init(void) {
    memset(handlers, 0, sizeof(handlers));

    // Register CPU Exceptions 0..31
    idt_set_gate(0,  (uint64_t)isr0,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(1,  (uint64_t)isr1,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(2,  (uint64_t)isr2,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(3,  (uint64_t)isr3,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(4,  (uint64_t)isr4,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(5,  (uint64_t)isr5,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(6,  (uint64_t)isr6,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(7,  (uint64_t)isr7,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(8,  (uint64_t)isr8,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(9,  (uint64_t)isr9,  GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(10, (uint64_t)isr10, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(11, (uint64_t)isr11, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(12, (uint64_t)isr12, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(13, (uint64_t)isr13, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(14, (uint64_t)isr14, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(15, (uint64_t)isr15, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(16, (uint64_t)isr16, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(17, (uint64_t)isr17, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(18, (uint64_t)isr18, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(19, (uint64_t)isr19, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(20, (uint64_t)isr20, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(21, (uint64_t)isr21, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(22, (uint64_t)isr22, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(23, (uint64_t)isr23, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(24, (uint64_t)isr24, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(25, (uint64_t)isr25, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(26, (uint64_t)isr26, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(27, (uint64_t)isr27, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(28, (uint64_t)isr28, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(29, (uint64_t)isr29, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(30, (uint64_t)isr30, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(31, (uint64_t)isr31, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);

    // Register Hardware IRQs 0..15 -> IDT 32..47
    idt_set_gate(32, (uint64_t)isr32, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(33, (uint64_t)isr33, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(34, (uint64_t)isr34, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(35, (uint64_t)isr35, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(36, (uint64_t)isr36, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(37, (uint64_t)isr37, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(38, (uint64_t)isr38, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(39, (uint64_t)isr39, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(40, (uint64_t)isr40, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(41, (uint64_t)isr41, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(42, (uint64_t)isr42, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(43, (uint64_t)isr43, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(44, (uint64_t)isr44, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(45, (uint64_t)isr45, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(46, (uint64_t)isr46, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);
    idt_set_gate(47, (uint64_t)isr47, GDT_KERNEL_CODE, IDT_ATTR_INTERRUPT_GATE, 0);

    // Register Syscall Gate 0x80 (User Accessible: IDT_ATTR_USER_GATE)
    idt_set_gate(128, (uint64_t)isr128, GDT_KERNEL_CODE, IDT_ATTR_USER_GATE, 0);

    kprintf("[+] ISR Handlers : Registered 32 CPU exceptions, 16 IRQs, and 0x80 Syscall gate\n");
}
