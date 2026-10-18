#include "kernel/pic.h"
#include "kernel/io.h"
#include "kernel/console.h"

#define ICW1_INIT 0x11
#define ICW4_8086 0x01

void pic_init(void) {
    uint8_t m1 = inb(PIC1_DATA);
    uint8_t m2 = inb(PIC2_DATA);
    (void)m1; (void)m2;

    // Start initialization sequence in cascade mode
    outb(PIC1_COMMAND, ICW1_INIT);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT);
    io_wait();

    // ICW2: Vector offset
    outb(PIC1_DATA, IRQ_BASE);       // Master PIC: 0x20..0x27
    io_wait();
    outb(PIC2_DATA, IRQ_BASE + 8);   // Slave PIC:  0x28..0x2F
    io_wait();

    // ICW3: Tell Master PIC there is a slave PIC at IRQ2
    outb(PIC1_DATA, 0x04);
    io_wait();
    // Tell Slave PIC its cascade identity (2)
    outb(PIC2_DATA, 0x02);
    io_wait();

    // ICW4: 8086/88 (MCS-80/85) mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Mask all interrupts by default (0xFF)
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    kprintf("[+] 8259 PIC     : Remapped (Master IRQs -> 0x20..0x27, Slave IRQs -> 0x28..0x2F)\n");
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}
