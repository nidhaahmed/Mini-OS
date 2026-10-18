#include "kernel/timer.h"
#include "kernel/isr.h"
#include "kernel/pic.h"
#include "kernel/io.h"
#include "kernel/console.h"

static volatile uint64_t system_ticks = 0;
static uint32_t timer_frequency = 0;

// External hook for scheduler tick (Milestone 3)
extern void scheduler_timer_tick(interrupt_frame_t* frame) __attribute__((weak));

static void timer_irq_handler(interrupt_frame_t* frame) {
    system_ticks++;

    if (scheduler_timer_tick) {
        scheduler_timer_tick(frame);
    }
}

void timer_init(uint32_t frequency) {
    timer_frequency = frequency;
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1) divisor = 1;

    // Send command byte: Channel 0, lobyte/hibyte, Rate Generator (mode 2), 16-bit binary
    outb(PIT_COMMAND_PORT, 0x34);

    // Send divisor low byte then high byte
    outb(PIT_CHANNEL0_DATA, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_DATA, (uint8_t)((divisor >> 8) & 0xFF));

    // Register handler for IRQ0 (Interrupt 32)
    isr_register_handler(32, timer_irq_handler);

    // Unmask IRQ0 on PIC
    pic_unmask_irq(0);

    kprintf("[+] PIT Timer    : Initialized at %u Hz (Period: %u ms)\n", frequency, 1000 / frequency);
}

uint64_t timer_get_ticks(void) {
    return system_ticks;
}

void timer_sleep_ticks(uint64_t ticks) {
    uint64_t target = system_ticks + ticks;
    while (system_ticks < target) {
        __asm__ volatile ("sti; hlt");
    }
}
