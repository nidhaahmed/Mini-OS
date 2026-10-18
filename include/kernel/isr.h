#ifndef ISR_H
#define ISR_H

#include <stdint.h>

typedef struct interrupt_frame {
    /* Pushed by pushaq in isr_common_stub */
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    /* Pushed by individual ISR stub */
    uint64_t int_no;
    uint64_t err_code;

    /* Pushed automatically by x86-64 CPU hardware on interrupt */
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) interrupt_frame_t;

typedef void (*isr_handler_t)(interrupt_frame_t* frame);

void isr_init(void);
void isr_register_handler(uint8_t int_no, isr_handler_t handler);
void isr_dispatch(interrupt_frame_t* frame);

#endif /* ISR_H */
